/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

#ifndef ASEBA_HTTP
#define ASEBA_HTTP

#include <stdint.h>
#include <list>
#include <queue>
#include <dashel/dashel.h>
#include "../../common/msg/msg.h"
#include "../../common/msg/descriptions-manager.h"

#if defined(_WIN32) && defined(__MINGW32__)
/* This is a workaround for MinGW32, see libxml/xmlexports.h */
#define IN_LIBXML
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "HttpHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Aseba
{
    /**
     \defgroup http Software router of messages on TCP and HTTP-over-TCP.
     */
    /*@{*/
    
    class HttpRequest;
    
    //! HTTP interface for aseba network
    class HttpInterface:  public Dashel::Hub, public Aseba::DescriptionsManager, public RootHttpHandler
    {
    public: 
        typedef std::vector<std::string>      strings;
        typedef std::list<HttpRequest*>       ResponseQueue;
        typedef std::set<HttpRequest*>        ResponseSet;
        typedef std::pair<unsigned,unsigned>  VariableAddress;
        typedef std::map<uint16, uint16>      NodeIdSubstitution;
        typedef std::map<std::string, Aseba::VariablesMap>      NodeNameVariablesMap;
        typedef std::map<unsigned, Aseba::VariablesMap>         NodeIdVariablesMap;
        typedef std::map<VariableAddress, ResponseSet>          VariableResponseSetMap;
        typedef std::map<Dashel::Stream*, ResponseQueue>        StreamResponseQueueMap;
        typedef std::map<HttpRequest*, std::set<std::string> >  StreamEventSubscriptionMap;
        typedef std::map<Dashel::Stream*, unsigned>             StreamNodeIdMap;
        typedef std::set<Dashel::Stream*>                       StreamSet;
        typedef std::map<Dashel::Stream*, NodeIdSubstitution>   StreamNodeIdSubstitutionMap;
        typedef std::map<unsigned, Aseba::CommonDefinitions>    NodeIdCommonDefinitionsMap;

        using Aseba::DescriptionsManager::NodeDescription;
        using Aseba::DescriptionsManager::NodesDescriptionsMap;

    protected:
        // streams
        StreamNodeIdMap             asebaStreams;
        Dashel::Stream*             httpStream;
        StreamNodeIdSubstitutionMap idSubstitutions;
        StreamResponseQueueMap      pendingResponses;
        VariableResponseSetMap      pendingVariables;
        StreamEventSubscriptionMap  eventSubscriptions;
        StreamSet                   streamsToShutdown;
        bool nodeDescriptionComplete;
        // debug variables
        bool verbose;
        int iterations;
        
        // Extract definitions from AESL file
        NodeIdCommonDefinitionsMap  commonDefinitions;
        NodeIdVariablesMap          allVariables;

        //variable cache
        std::map<std::pair<unsigned,unsigned>, std::vector<short> > variable_cache;
        
    public:
        //default values needed for unit testing
        HttpInterface(const strings& targets = std::vector<std::string>(), const std::string& http_port="3000", const int iterations=-1);
        virtual void run();
        virtual void broadcastGetDescription();
        virtual void aeslLoadFile(const unsigned nodeId, const std::string& filename);
        virtual void aeslLoadMemory(const unsigned nodeId, const char* buffer, const int size);
        virtual void updateVariables(const unsigned nodeId);
        
        virtual void scheduleResponse(Dashel::Stream* stream, HttpRequest* req);
        virtual void sendAvailableResponses();
        virtual void unscheduleResponse(Dashel::Stream* stream, HttpRequest* req);
        virtual void unscheduleAllResponses(Dashel::Stream* stream);

        virtual void sendEvent(const unsigned nodeId, const strings& args);
        virtual void sendSetVariable(const unsigned nodeId, const strings& args);
        std::vector<unsigned> getIdsFromArgs(const strings& args);
        virtual std::pair<unsigned,unsigned> sendGetVariables(const unsigned nodeId, const strings& args);
        virtual bool getVarPos(const unsigned nodeId, const std::string& variableName, unsigned& pos);
        virtual void parse_json_form(std::string content, strings& values);
        
        virtual StreamEventSubscriptionMap& getEventSubscriptions() { return eventSubscriptions; }
        virtual NodesDescriptionsMap& getNodesDescriptions() { return nodesDescriptions; }
        virtual StreamNodeIdMap& getAsebaStreams() { return asebaStreams; }
        virtual NodeIdCommonDefinitionsMap& getCommonDefinitions() { return commonDefinitions; }
        virtual NodeIdVariablesMap& getAllVariables() { return allVariables; }
        virtual VariableResponseSetMap& getPendingVariables() { return pendingVariables; }

        virtual bool isVerbose() const { return verbose; }

    protected:
        /* // reimplemented from parent classes */
        virtual void connectionCreated(Dashel::Stream* stream);
        virtual void connectionClosed(Dashel::Stream* stream, bool abnormal);
        virtual void incomingData(Dashel::Stream* stream);
        virtual void nodeDescriptionReceived(unsigned nodeId);
        // specific to http interface
        virtual void aeslLoad(const unsigned nodeId, xmlDoc* doc);
        virtual void incomingVariables(const Variables *variables);
        virtual void incomingUserMsg(const UserMessage *userMsg);
        
        // helper functions
        //bool getNodeAndVarPos(const std::string& nodeName, const std::string& variableName, unsigned& nodeId, unsigned& pos) const;
//        bool getVarPos(const unsigned nodeId, const std::string& variableName, unsigned& pos) const;
        bool compileAndSendCode(const unsigned nodeId, const std::wstring& program);

        Dashel::Stream* getStreamFromNodeId(const unsigned nodeId);
    };
    

	
	class InterruptException : public std::exception
    {
    public:
        InterruptException(int s) : S(s) {}
        int S;
    };
    

    /*@}*/
};

#endif
