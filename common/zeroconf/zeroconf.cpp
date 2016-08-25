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

#include "zeroconf.h"
#include "../utils/utils.h"
#include "../utils/FormatableString.h"
#include <dashel/dashel.h>

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Register and announce a target described by a name and a port
	void Zeroconf::registerPort(const string& name, const unsigned port, const string& txt)
	{
		ZeroconfService z{name,"_aseba._tcp.","local."};
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		DNSServiceErrorType err = DNSServiceRegister(&z.serviceref,
							     kDNSServiceFlagsDefault,
							     0, // default all interfaces
							     name.c_str(),
							     "_aseba._tcp.",
							     NULL, // use default domain
							     NULL, // use this host name
							     htons(port),
							     len, // TXT length
							     record, // TXT record
							     cb_Register,
							     &z);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
		{
			DNSServiceErrorType err = DNSServiceProcessResult(z.serviceref); // block until daemon replies
			if (err != kDNSServiceErr_NoError)
				throw Zeroconf::Error(FormatableString("DNSServiceProcessResult: error %0").arg(err));
			else
			{
				services[z.name] = z; // copy
				z.serviceref = 0; // because now owned by services
			}
		}
	}

	//! Register and announce a target described by an existing Dashel stream
	void Zeroconf::registerStream(const std::string& name, const Dashel::Stream* stream, const string& txt)
	{
		unsigned port = atoi(stream->getTargetParameter("port").c_str());
		registerPort(name,port,txt);
	}

	//! Asynchronously update the set of known targets
	void Zeroconf::browseForTargets()
	{
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
			ZeroconfService *zref = (ZeroconfService *)context;
			zref->name = string(name);
			zref->regtype = string(regtype);
			zref->domain = string(domain);
		}
	}

	//! Helper to format target info for DNS TXT record
	string Zeroconf::txtRecord(int protovers, const std::string& type, const std::vector<int>& ids, const std::vector<int>& pids)
	{
		Zeroconf::TxtRecord t(protovers, type, ids, pids);
		return t.record();
	}

	//! Replace TXT record with a new one, typically when node, pid lists change
	void Zeroconf::updateTxtRecord(const std::string& name, TxtRecord& rec)
	{
		ZeroconfService zs = services[name];
		string rawdata = rec.record();
		DNSServiceErrorType err = DNSServiceUpdateRecord(zs.serviceref, NULL, NULL, rawdata.length(), rawdata.c_str(), 0);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceUpdateRecord: error %0").arg(err));
	}

	//! Return map of targets
	std::map<std::string, Zeroconf::ServiceInfo> Zeroconf::getTargets()
	{
		return targets;
	}


	//! A TXT record is composed of length-prefixed strings of the form KEY[=VALUE]
	string Zeroconf::TxtRecord::record()
	{
		return txt.str();
	}
	//! constructor for an Aseba target, with node ids and product ids
	Zeroconf::TxtRecord::TxtRecord(int protovers, const std::string& type, const std::vector<int>& ids, const std::vector<int>& pids)
	{
		add("txtvers", 1);
		add("protovers", protovers);
		add("type", type);
		add("ids", ids);
		add("pids", pids);
	}
	//! a string value in the TXT record is a sequence of bytes
	void Zeroconf::TxtRecord::add(const std::string& key, const std::string& value)
	{
		string record = key + "=" + value.substr(0,20); // silently truncate name to 20 characters
		txt.put(record.length());
		txt << record;
	}
	//! a simple integer value in the TXT record is the string of its decimal value
	void Zeroconf::TxtRecord::add(const std::string& key, const int value)
	{
		ostringstream record;
		record << key << "=" << value;
		txt.put(record.str().length());
		txt << record.str();
	}
	//! a vector of integers in the TXT record is a sequence of big-endian 16-bit values
	//! note that the size of the vector is implicit in the record length: (record.length()-record.find("=")-1)/2.
	void Zeroconf::TxtRecord::add(const std::string& key, const std::vector<int>& values)
	{
		ostringstream record;
		record << key << "=";
		for (const auto &value : values)
			record.put(value<<8), record.put(value % 0xff);
		txt.put(record.str().length());
		txt << record.str();
	}

} // namespace Aseba
