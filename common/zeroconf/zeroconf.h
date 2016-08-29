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
#include <thread>
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

	//! Aseba::Zeroconf provides methods for registering targets, updating target
	//! descriptions, and browsing for targets. It runs a thread that watches the
	//! DNS Service for updates, and triggers callback processing as necessary.
	class Zeroconf
	{
	public:
		std::atomic_bool running{true}; //!< are we watching for DNS service updates?
		std::thread watcher{&Zeroconf::handleDnsServiceEvents, this}; //~< thread in which select loop occurs
		~Zeroconf()
		{
			running = false;
			watcher.join(); // tell watcher to stop, to avoid std::terminate
		}

	public:
		class TxtRecord;
		typedef std::map<std::string, std::string> ServiceInfo; //!< (key, value) set
		typedef std::map<std::string, Zeroconf::ServiceInfo> NameServiceInfoMap;

	public:
		virtual void registerService(const std::string& name, const unsigned port, const TxtRecord& txtrec);
		virtual void registerService(const std::string& name, const Dashel::Stream* dashel_stream, const TxtRecord& txtrec);

		virtual void updateTxtRecord(const std::string& name, TxtRecord& record);

		virtual void browseForTargets();
		virtual NameServiceInfoMap getTargets();
		
	private:
		//! Private class that encapsulates a service of type _aseba._tcp known to the mDNS-SD daemon
		class ZeroconfService
		{
		public:
			std::string name;
			std::string regtype;
			std::string domain;
			DNSServiceRef serviceref;
		public:
			virtual ~ZeroconfService()
			{   // release the service reference
				if (serviceref)
					DNSServiceRefDeallocate(serviceref);
				serviceref = 0;
			}
			virtual bool operator==(const ZeroconfService &other) const
			{
				return name == other.name && domain == other.domain;
			}
		};
		typedef std::map<std::string, std::unique_ptr<ZeroconfService>> NameServiceMap;

	public:
		//! An error in registering or browsing Zeroconf
		struct Error:public std::runtime_error
		{
			Error(const std::string& what): std::runtime_error(what) {}
		};

	private:
		static void DNSSD_API cb_Register(DNSServiceRef, DNSServiceFlags, DNSServiceErrorType,
										  const char *, const char *, const char *, void *);
		static void DNSSD_API cb_Browse(DNSServiceRef, DNSServiceFlags, uint32_t interfaceIndex, DNSServiceErrorType,
										  const char *, const char *, const char *, void *);

		NameServiceMap services; //!< map friendly name to service record
		NameServiceInfoMap targets; //!< map friendly name to (key, value) set
		std::map<DNSServiceRef, std::unique_ptr<ZeroconfService>> pending; //< map pending sdRef to service record

		void handleDnsServiceEvents(); //! run the handleDSEvents_thread
	};

	/**
	 \addtogroup zeroconf Zeroconf (mDNS-SD multicast DNS Service Discovery)

		Zeroconf records three DNS records for each service: a PTR for the
		symbolic name, an SRV for the actual host and port number, and a TXT
		for extra information. The TXT record is used by clients to help choose
		the hosts it wishes to connect to. The syntax of TXT records is defined
		in https://tools.ietf.org/html/rfc6763#section-6.
		The TXT record for Aseba targets specifies the target type, the Aseba
		protocol version, a list of node ids, and a list of node product ids.
	 */
	
	class Zeroconf::TxtRecord
	{
	public:
		TxtRecord(int protovers, const std::string& type, const std::vector<int>& ids, const std::vector<int>& pids);
		virtual void assign(const std::string& key, const std::string& value);
		virtual void assign(const std::string& key, const int value);
		virtual void assign(const std::string& key, const std::vector<int>& values);
		virtual std::string record() const;

	private:
		std::map<std::string, std::string> fields;
		virtual void add(std::ostringstream& txt, const std::string& key) const;
	};

	/*@}*/
} // namespace Aseba

#endif /* ASEBA_ZEROCONF */
