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

namespace Aseba
{
	//! Aseba::ThreadZeroconf provides methods for registering targets, updating target
	//! descriptions, and browsing for targets. It runs a thread that watches the
	//! DNS Service for updates, and triggers callback processing as necessary.
	class ThreadZeroconf : Zeroconf
	{
	public:
		std::atomic_bool running{true}; //!< are we watching for DNS service updates?
		std::thread watcher{&ThreadZeroconf::handleDnsServiceEvents, this}; //~< thread in which select loop occurs
		~ThreadZeroconf(); // needed to terminate thread

		virtual void assignSocket(int socket);
		virtual void releaseSocket(int socket);

	private:
		fd_set fds;
		int max_fds;
	private:
		void handleDnsServiceEvents(); //! run the handleDSEvents_thread
	};
}
