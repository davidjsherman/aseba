/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "../utils/utils.h"
#include "../utils/FormatableString.h"
#include <dashel/dashel.h>
#include "zeroconf.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Register and announce a target described by a name and a port
	void Zeroconf::registerService(const string& name, const unsigned port, const TxtRecord& txtrec)
	{
		DNSServiceRef serviceref;
		string txt(txtrec.record());
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		DNSServiceErrorType err = DNSServiceRegister(&serviceref,
							     kDNSServiceFlagsDefault,
							     0, // default all interfaces
							     name.c_str(),
							     "_aseba._tcp",
							     NULL, // use default domain, usually "local."
							     NULL, // use this host name
							     htons(port),
							     len, // TXT length
							     record, // TXT record
							     cb_Register,
							     this); // context pointer is this Zeroconf object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
		{
			pending[serviceref] = std::unique_ptr<ZeroconfService>(new ZeroconfService());;
			// Responses from the DNS Service will subsequently be handled in the watcher thread,
			// where they will be dispatched to Zeroconf::cb_Register by DNSServiceProcessResult.
		}
	}

	//! Register and announce a target described by an existing Dashel stream
	void Zeroconf::registerService(const std::string& name, const Dashel::Stream* stream, const TxtRecord& txtrec)
	{
		auto port = atoi(stream->getTargetParameter("port").c_str());
		registerService(name,port,txtrec);
	}

	//! Replace TXT record with a new one, typically when node, pid lists change
	void Zeroconf::updateTxtRecord(const std::string& name, TxtRecord& rec)
	{
		auto sdRef = services[name]->serviceref;
		string rawdata = rec.record();
		DNSServiceErrorType err = DNSServiceUpdateRecord(sdRef, NULL, NULL, rawdata.length(), rawdata.c_str(), 0);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceUpdateRecord: error %0").arg(err));
	}

	//! Asynchronously update the set of known targets
	void Zeroconf::browseForTargets()
	{
		DNSServiceRef serviceref;
		DNSServiceErrorType err = DNSServiceBrowse(&serviceref,
												   kDNSServiceFlagsDefault,
												   0, // default all interfaces
												   "_aseba._tcp",
												   NULL, // use default domain, usually "local."
												   cb_Browse,
												   this); // context pointer is this Zeroconf object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
		{
			pending[serviceref] = std::unique_ptr<ZeroconfService>(new ZeroconfService());
			// Responses from the DNS Service will subsequently be handled in the watcher thread,
			// where they will be dispatched to Zeroconf::cb_Browse by DNSServiceProcessResult.
		}

	}

	//! Return map of targets
	Zeroconf::NameServiceInfoMap Zeroconf::getTargets()
	{
		return targets;
	}

	//! callback to update ZeroconfService record with results of registration
	void DNSSD_API Zeroconf::cb_Register(DNSServiceRef sdRef,
										 DNSServiceFlags flags,
										 DNSServiceErrorType errorCode,
										 const char *name,
										 const char *regtype,
										 const char *domain,
										 void *context)
	{
		if (errorCode != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegisterReply: error %0").arg(errorCode));
		else
		{
			Zeroconf *zref = static_cast<Zeroconf *>(context);

//			const auto it = zref->pending.find(sdRef);
//			zref->services[name] = std::move(it->second);
//			zref->pending.erase(it->first);
			zref->services[name] = std::move(zref->pending[sdRef]);

			zref->services[name]->serviceref = sdRef;
			zref->services[name]->name = name;
			zref->services[name]->domain = domain;
		}
	}

	//! callback to update ZeroconfService record with results of browse
	void DNSSD_API Zeroconf::cb_Browse(DNSServiceRef sdRef,
										 DNSServiceFlags flags,
									   uint32_t interfaceIndex,
										 DNSServiceErrorType errorCode,
										 const char *name,
										 const char *regtype,
										 const char *domain,
										 void *context)
	{
		if (errorCode != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceBrowseReply: error %0").arg(errorCode));
		else
		{
			Zeroconf *zref = static_cast<Zeroconf *>(context);
			zref->targets[name] = { {"name",string(name)}, {"domain",string(domain)} };
		}
	}

	void Zeroconf::handleDnsServiceEvents()
	{
		struct timeval tv{1,0}; //!< maximum time to learn about a new service (5 sec)

		while (running)
		{
			if (pending.size() == 0)
			{
				//std::this_thread::sleep_for(std::chrono::seconds(5));
				continue;
			}
			fd_set fds;
			int max_fds(0);
			FD_ZERO(&fds);
			std::map<DNSServiceRef,int> serviceFd;

			int fd_count(0);

			for (auto const& srv: pending)
			{
				int fd = DNSServiceRefSockFD(srv.first);
				if (fd != -1)
				{
					max_fds = max_fds > fd ? max_fds : fd;
					FD_SET(fd, &fds);
					serviceFd[srv.first] = fd;
					fd_count++;
				}
			}
			int result = select(max_fds+1, &fds, (fd_set*)NULL, (fd_set*)NULL, &tv);
			if (result > 0)
			{
				for (auto const& srv: pending)
					if (FD_ISSET(serviceFd[srv.first], &fds))
						DNSServiceProcessResult(srv.first);
			}
			else if (result < 0)
				throw Zeroconf::Error(FormatableString("handleDnsServiceEvents: select returned %0 errno %1").arg(result).arg(errno));
			else
				; // timeout, check for new services
		}
	}

} // namespace Aseba
