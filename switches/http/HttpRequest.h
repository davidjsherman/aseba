/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2015:
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

#ifndef ASEBA_HTTP_REQUEST
#define ASEBA_HTTP_REQUEST

#include <map>
#include <string>
#include <vector>
#include <dashel/dashel.h>

namespace Aseba
{
	class HttpResponse; // forward declaration

	class HttpRequest
	{
		public:
			static const int CONTENT_BYTES_LIMIT = 40000;

			HttpRequest();
			virtual ~HttpRequest();

			virtual bool receive();
			virtual HttpResponse& respond();

			virtual bool isResponseReady() const { return !blocking && response != NULL; }

			virtual void setVerbose(bool verbose) { this->verbose = verbose; }
			virtual bool isValid() const { return valid; }
			virtual void setBlocking(bool blocking) { this->blocking = blocking; }
			virtual bool isBlocking() { return blocking; }

			virtual const std::string& getMethod() const { return method; }
			virtual const std::string& getUri() const { return uri; }
			virtual const std::string& getProtocol() const { return protocol; }
			virtual const std::vector<std::string>& getTokens() const { return tokens; }
			virtual const std::map<std::string, std::string>& getHeaders() const { return headers; }
			virtual const std::string& getContent() const { return content; }

			std::string getHeader(const std::string& header) const {
				std::map<std::string, std::string>::const_iterator query = headers.find(header);

				if(query != headers.end()) {
					return query->second;
				} else {
					return "";
				}
			}

		protected:
			virtual bool readRequestLine();
			virtual bool readHeaders();
			virtual bool readContent();

			virtual HttpResponse *createResponse() = 0;

			virtual std::string readLine() = 0;
			virtual void readRaw(char *buffer, int size) = 0;

		private:
			bool verbose;
			bool valid;
			bool blocking;
			HttpResponse *response;

			std::string method;
			std::string uri;
			std::string protocol;
			std::vector<std::string> tokens;
			std::map<std::string, std::string> headers;
			std::string content;
	};

	class DashelHttpRequest : public HttpRequest
	{
		public:
    		DashelHttpRequest(Dashel::Stream *stream);
			virtual ~DashelHttpRequest();

			virtual Dashel::Stream *getStream() { return stream; }

		protected:
			virtual HttpResponse *createResponse();

			virtual std::string readLine();
			virtual void readRaw(char *buffer, int size);

		private:
			Dashel::Stream *stream;
	};
}

#endif
