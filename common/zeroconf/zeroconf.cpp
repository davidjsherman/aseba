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
	void Zeroconf::registerTarget(const Zeroconf::Target* target, const TxtRecord& txtrec)
	{
		DNSServiceRef serviceref;
		string txt(txtrec.record());
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		DNSServiceErrorType err = DNSServiceRegister(&serviceref,
							     kDNSServiceFlagsDefault,
							     0, // default all interfaces
							     target->name.c_str(),
							     "_aseba._tcp",
							     NULL, // use default domain, usually "local."
							     NULL, // use this host name
							     htons(target->port),
							     len, // TXT length
							     record, // TXT record
							     cb_Register,
							     this); // context pointer is this Zeroconf object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
		{
			pending[serviceref] = std::unique_ptr<ZeroconfDiscoveryRequest>(new ZeroconfDiscoveryRequest());;
			// Block until daemon replies. The DiscoveryRequest will be moved from pending
			// to services by the cb_Register callback.
			DNSServiceErrorType err = DNSServiceProcessResult(serviceref);
			if (err != kDNSServiceErr_NoError)
				throw Zeroconf::Error(FormatableString("DNSServiceProcessResult: error %0").arg(err));
//			else
//				pending.erase(serviceref);
		}
	}

	//! Register and announce a target described by an existing Dashel stream
	Zeroconf::Target::Target(const std::string & name, const int & port) :
	name(name),
	port(port)
	{}

	//! Register and announce a target described by an existing Dashel stream
	Zeroconf::Target::Target(const Dashel::Stream* stream)
	{
		port = atoi(stream->getTargetParameter("port").c_str());
		name = "Aseba Local " + stream->getTargetParameter("port");
	}

	//! Register and announce a target described by an existing Dashel stream
	void Zeroconf::Target::advertise(Zeroconf& zeroconf, const TxtRecord& txtrec)
	{
		zeroconf.targets[name] = this;
		zeroconf.registerTarget(this, txtrec);
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
			// Block until daemon replies. The DiscoveryRequest will be moved from pending to services
			// by the cb_Browse callback. Continue as long as kDNSServiceFlagsMoreComing is set.
			while (pending.find(serviceref) != pending.end()) {
				DNSServiceErrorType err = DNSServiceProcessResult(serviceref);
				if (err != kDNSServiceErr_NoError)
					throw Zeroconf::Error(FormatableString("DNSServiceProcessResult: error %0").arg(err));
			}
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

			zref->services[name] = std::move(zref->pending[sdRef]);
			zref->services[name]->serviceref = sdRef;
			

			zref->pending.erase(sdRef);
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
			if (zref->targets.find(name) == zref->targets.end())
				zref->targets[name] = new Zeroconf::Target{name,0};
			zref->targets[name]->properties = { {"name",string(name)}, {"domain",string(domain)} };

			if ( ! (flags & kDNSServiceFlagsMoreComing))
				zref->pending.erase(sdRef);
		}
	}
} // namespace Aseba
