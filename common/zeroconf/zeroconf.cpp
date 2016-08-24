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
#include <typeinfo>
#include <iostream>
#include <iomanip>
#include <map>
#include <dashel/dashel.h>

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Register and announce a target described by a name and a port
	void Zeroconf::registerPort(const string name, const unsigned port, const string txt)
	{
        ZeroconfService z;
        uint16_t len = txt.size();
        const char* record = txt.c_str();
        DNSServiceErrorType err = DNSServiceRegister(&z.serviceref,
                                                     kDNSServiceFlagsDefault,
                                                     0, // default all interfaces
                                                     name.c_str(),
                                                     "_aseba._tcp",
                                                     NULL, // use default domain
                                                     NULL, // use this host name
                                                     htons(port),
                                                     len, // TXT length
                                                     record, // TXT record
                                                     cb_Register,
                                                     &z);
        if (err != kDNSServiceErr_NoError)
            throw runtime_error(FormatableString("Zeroconf DNSServiceRegister: error %0").arg(err));
        else
        {
            DNSServiceErrorType err = DNSServiceProcessResult(z.serviceref); // block until daemon replies
            if (err != kDNSServiceErr_NoError)
                throw runtime_error(FormatableString("Zeroconf DNSServiceProcessResult: error %0").arg(err));
            else
            {
                services[z.name] = z; // copy
                z.serviceref = 0; // because now owned by services
            }
        }
	}

    //! Register and announce a target described by an existing Dashel stream
    void Zeroconf::registerStream(const std::string name, const Dashel::Stream* stream, const string txt)
    {
        string service_name = name;
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
            throw runtime_error(FormatableString("Zeroconf DNSServiceRegisterReply: error %0").arg(errorCode));
        else
        {
            ZeroconfService *zref = (ZeroconfService *)context;
            zref->name = string(name);
            zref->regtype = string(regtype);
            zref->domain = string(domain);
        }
    }

    //! Format target info for DNS TXT record
    string Zeroconf::txtRecord(int protovers, std::string type, std::vector<int> ids, std::vector<int> pids)
    {
        std::ostringstream txt;
        if (type.size() > 20)
            type = type.substr(0,20);

        txt.put(9);
        txt << "txtvers=1";

        txt.put(11);
        txt << "protovers=" << (protovers % 10);

        txt.put(5 + type.length());
        txt << "type=";
        txt << type;

        txt.put(4 + 2 * ids.size());
        txt << "ids=";
        for (const auto &id : ids) txt.put(id<<8), txt.put(id % 0xff);

        txt.put(5 + 2 * pids.size());
        txt << "pids=";
        for (const auto &pid : pids) txt.put(pid<<8), txt.put(pid % 0xff);

        return txt.str();
    }

} // namespace Aseba
