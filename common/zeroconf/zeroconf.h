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

#ifndef ASEBA_ZEROCONF
#define ASEBA_ZEROCONF

#include "../types.h"
#include "../consts.h"
#include <typeinfo>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>
#include <dns_sd.h>

namespace Dashel
{
	class Stream;
}

namespace Aseba
{
	/**
	 \defgroup zeroconf Zeroconf (mDNS-SD multicast DNS Service Discovery)

		Using Zeroconf, an Aseba target announces itself on the TCP/IP network
		as a service of type _aseba._tcp, and can be discovered by clients.
		A target might be an individual node or an Aseba switch.
		The zeroconf service provides minimal information about a target:
		Dashel target (host name, IP port), target name, target type, Aseba
		protocol version, a list of node ids, and a list of node product ids.
		A client may obtain complete node descriptions by connecting to the
		Dashel target and requesting them using the Aseba protocol
	 */
	/*@{*/

	class Zeroconf
	{
	private:
		//! Private class that records a service of type _aseba._tcp known to the mDNS-SD daemon
		class ZeroconfService
		{
		public:
			std::string name;
			std::string regtype;
			std::string domain;
			DNSServiceRef serviceref;
		public:
			~ZeroconfService()
			{   // release the service reference
				if (serviceref)
					DNSServiceRefDeallocate(serviceref);
				serviceref = 0;
			}
			bool operator==(const ZeroconfService &other) const
			{
				return name == other.name && domain == other.domain;
			}
		};

	public:
		//! An error in registering or browsing Zeroconf
		struct Error:public std::runtime_error
		{
			Error(const std::string& what): std::runtime_error(what) {}
		};

	public:
		class TxtRecord;
		typedef std::map<std::string,std::string> ServiceInfo; //!< (key, value) set

	public:
		static std::string txtRecord(int protovers, const std::string& type, const std::vector<int>& ids, const std::vector<int>& pids);

		virtual void registerPort(const std::string& name, const unsigned port, const std::string& txt);
		virtual void registerStream(const std::string& name, const Dashel::Stream* dashel_stream, const std::string& txt);
		virtual void updateTxtRecord(const std::string& name, TxtRecord& record);
		virtual void browseForTargets();
		virtual std::map<std::string, ServiceInfo> getTargets();

	private:
		static void DNSSD_API cb_Register(DNSServiceRef, DNSServiceFlags, DNSServiceErrorType,
										  const char *, const char *, const char *, void *);

		std::map<std::string, ZeroconfService> services; //!< map friendly name to service record
		std::map<std::string, ServiceInfo> targets; //!< map friendly name to (key, value) set
	};

	class Zeroconf::TxtRecord
	{
	public:
		TxtRecord(int protovers, const std::string& type, const std::vector<int>& ids, const std::vector<int>& pids);
		virtual void add(const std::string& key, const std::string& value);
		virtual void add(const std::string& key, const int value);
		virtual void add(const std::string& key, const std::vector<int>& values);
		virtual std::string record();

	private:
		std::ostringstream txt;
	};

	/*@}*/
} // namespace Aseba

#endif /* ASEBA_ZEROCONF */
