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
	void Zeroconf::registerTarget(Zeroconf::Target& target, const TxtRecord& txtrec)
	{
		DNSServiceRef serviceref;
		string txt(txtrec.record());
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		DNSServiceErrorType err = DNSServiceRegister(&serviceref,
							     kDNSServiceFlagsDefault,
							     0, // default all interfaces
							     target.name.c_str(),
							     "_aseba._tcp",
							     NULL, // use default domain, usually "local."
							     NULL, // use this host name
							     htons(target.port),
							     len, // TXT length
							     record, // TXT record
							     cb_Register,
							     this); // context pointer is this Zeroconf object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
		{
			pending[serviceref] = std::unique_ptr<ZeroconfDiscoveryRequest>(new ZeroconfDiscoveryRequest());;
			// Responses from the DNS Service will subsequently be handled in the watcher thread,
			// where they will be dispatched to Zeroconf::cb_Register by DNSServiceProcessResult.
		}
	}

	//! Register and announce a target described by an existing Dashel stream
	Zeroconf::Target::Target(const Dashel::Stream* stream)
	{
		port = atoi(stream->getTargetParameter("port").c_str());
	}

	//! Replace TXT record with a new one, typically when node, pid lists change
	void Zeroconf::updateTxtRecord(const Zeroconf::Target& target, TxtRecord& rec)
	{
		auto sdRef = services[target.name]->serviceref;
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
			pending[serviceref] = std::unique_ptr<ZeroconfDiscoveryRequest>(new ZeroconfDiscoveryRequest());
			// Responses from the DNS Service will subsequently be handled in the watcher thread,
			// where they will be dispatched to Zeroconf::cb_Browse by DNSServiceProcessResult.
			sleep(5); // hack to wait for answers
		}

	}

	//! Return map of targets
	Zeroconf::NameTargetMap Zeroconf::getTargets()
	{
		return targets;
	}

	//! callback to update ZeroconfTarget record with results of registration
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
//			zref->services[name]->name = name;
//			zref->services[name]->domain = domain;
		}
	}

	//! callback to update ZeroconfDiscoveryRequest record with results of browse
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
			zref->targets[name].properties = { {"name",string(name)}, {"domain",string(domain)} };
		}
	}
} // namespace Aseba
