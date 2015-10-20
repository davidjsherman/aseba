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

#include <iostream>
#include "../../common/utils/utils.h"
#include "http.h"
#include "HttpInterfaceHandlers.h"

using Aseba::EventsHandler;
using Aseba::HttpInterface;
using Aseba::InterfaceHttpHandler;
using Aseba::LoadHandler;
using Aseba::NodeEventsHandler;
using Aseba::NodeInfoHandler;
using Aseba::NodesHandler;
using Aseba::ResetHandler;
using Aseba::VariableOrEventHandler;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

NodesHandler::NodesHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{
	addToken("nodes");

	addSubhandler(new LoadHandler(interface));
	addSubhandler(new NodeInfoHandler(interface));
	addSubhandler(new NodeEventsHandler(interface));
	addSubhandler(new VariableOrEventHandler(interface));
}

NodesHandler::~NodesHandler()
{

}

EventsHandler::EventsHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{
	addToken("events");
}

EventsHandler::~EventsHandler()
{

}

void EventsHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	// eventSubscriptions[conn] is an unordered set of strings
	if(tokens.size() == 1) {
		getInterface()->getEventSubscriptions()[request].insert("*");
	} else {
		for(vector<string>::const_iterator i = tokens.begin() + 1; i != tokens.end(); ++i) {
			getInterface()->getEventSubscriptions()[request].insert(*i);
		}
	}

	request->respond().setHeader("Content-Type", "text/event-stream");
	request->respond().setHeader("Cache-Control", "no-cache");
	request->respond().setHeader("Connection", "keep-alive");
	request->respond().send(); // send header immediately
	request->setBlocking(true); // connection must stay open!
}

ResetHandler::ResetHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{
	addToken("reset");
	addToken("reset_all");
}

ResetHandler::~ResetHandler()
{

}

void ResetHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	for(HttpInterface::NodesDescriptionsMap::iterator descIt = getInterface()->getNodesDescriptions().begin(); descIt != getInterface()->getNodesDescriptions().end(); ++descIt) {
		bool ok = true;
		// nodeId = getNodeId(descIt->second.name, 0, &ok);
		if(!ok) continue;
		string nodeName = WStringToUTF8(descIt->second.name);

		for(HttpInterface::StreamNodeIdMap::iterator it = getInterface()->getAsebaStreams().begin(); it != getInterface()->getAsebaStreams().end(); ++it) {
			Dashel::Stream* stream = it->first;
			unsigned nodeId = it->second;
			Reset(nodeId).serialize(stream); // reset node
			stream->flush();
			Run(nodeId).serialize(stream);   // re-run node
			stream->flush();
			if(descIt->second.name.find(L"thymio-II") == 0) {   // Special case for Thymio-II. Should we instead just check whether motor.*.target exists?
				vector<string> tokens;
				tokens.push_back("motor.left.target");
				tokens.push_back("0");
				getInterface()->sendSetVariable(nodeId, tokens);
				tokens[0] = "motor.right.target";
				getInterface()->sendSetVariable(nodeId, tokens);
			}
			size_t eventPos;
			if(getInterface()->getCommonDefinitions()[nodeId].events.contains(UTF8ToWString("reset"), &eventPos)) {
				// bug: assumes AESL file is common to all nodes
				// can we get this from the node description?
				vector<string> data;
				data.push_back("reset");
				getInterface()->sendEvent(nodeId, data);
			}
		}

		request->respond();
	}
}

LoadHandler::LoadHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

LoadHandler::~LoadHandler()
{

}

bool LoadHandler::checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const
{
	return tokens.size() <= 1 && request->getMethod() == "PUT";
}

void LoadHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	if(getInterface()->isVerbose()) {
		cerr << "PUT /nodes/" << tokens[0].c_str() << " trying to load aesl script\n";
	}

	const char* buffer = request->getContent().c_str();
	size_t pos = request->getContent().find("file=");
	if(pos != std::string::npos) {
		std::vector<unsigned> todo = getInterface()->getIdsFromArgs(tokens);
		for(std::vector<unsigned>::const_iterator it = todo.begin(); it != todo.end(); ++it) {
			getInterface()->aeslLoadMemory(*it, buffer + pos + 5, request->getContent().size() - pos - 5);
		}
		request->respond();
	} else {
		request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
	}
}

NodeEventsHandler::NodeEventsHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

NodeEventsHandler::~NodeEventsHandler()
{

}

bool NodeEventsHandler::checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const
{
	return tokens.size() >= 2 && tokens[1].find("events") == 0;
}

void NodeEventsHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	// eventSubscriptions[conn] is an unordered set of strings
	if(tokens.size() == 1) {
		getInterface()->getEventSubscriptions()[request].insert("*");
	} else {
		for(vector<string>::const_iterator i = tokens.begin() + 1; i != tokens.end(); ++i) {
			getInterface()->getEventSubscriptions()[request].insert(*i);
		}
	}

	request->respond().setHeader("Content-Type", "text/event-stream");
	request->respond().setHeader("Cache-Control", "no-cache");
	request->respond().setHeader("Connection", "keep-alive");
	request->respond().send(); // send header immediately
	request->setBlocking(true); // connection must stay open!
}

NodeInfoHandler::NodeInfoHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

NodeInfoHandler::~NodeInfoHandler()
{

}

bool NodeInfoHandler::checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const
{
	return tokens.size() <= 1;
}

void NodeInfoHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	bool do_one_node(tokens.size() > 0);

	std::stringstream json;
	json << (do_one_node ? "" : "["); // hack, should first select list of matching nodes, then check size

	for(HttpInterface::NodesDescriptionsMap::iterator descIt = getInterface()->getNodesDescriptions().begin(); descIt != getInterface()->getNodesDescriptions().end(); ++descIt) {
		const HttpInterface::NodeDescription& description(descIt->second);
		string nodeName = WStringToUTF8(description.name);
		unsigned nodeId = descIt->first;

		if(!do_one_node) {
			json << (descIt == getInterface()->getNodesDescriptions().begin() ? "" : ",");
			json << "{";
			json << "\"node\":" << nodeId << ",";
			json << "\"name\":\"" << nodeName << "\",\"protocolVersion\":" << description.protocolVersion;
			json << "}";
		} else { // (do_one_node)
			if(!(descIt->first == (unsigned) atoi(tokens[0].c_str()) || nodeName.find(tokens[0]) == 0)) continue; // this is not a match, skip to next candidate

			json << "{"; // begin node
			json << "\"node\":\"" << nodeId << "\",";
			json << "\"name\":\"" << nodeName << "\",\"protocolVersion\":" << description.protocolVersion;

			json << ",\"bytecodeSize\":" << description.bytecodeSize;
			json << ",\"variablesSize\":" << description.variablesSize;
			json << ",\"stackSize\":" << description.stackSize;

			// named variables
			json << ",\"namedVariables\":{";
			bool seen_named_variables = false;
			for(HttpInterface::NodeIdVariablesMap::const_iterator n(getInterface()->getAllVariables().find(nodeId)); n != getInterface()->getAllVariables().end(); ++n) {
				VariablesMap vm = n->second;
				for(VariablesMap::iterator i = vm.begin(); i != vm.end(); ++i) {
					json << (i == vm.begin() ? "" : ",") << "\"" << WStringToUTF8(i->first) << "\":" << i->second.second;
					seen_named_variables = true;
				}
			}
			if(!seen_named_variables) {
				// failsafe: if compiler hasn't found any variables, get them from the node description
				for(vector<Aseba::TargetDescription::NamedVariable>::const_iterator i(description.namedVariables.begin()); i != description.namedVariables.end(); ++i)
					json << (i == description.namedVariables.begin() ? "" : ",") << "\"" << WStringToUTF8(i->name) << "\":" << i->size;
			}
			json << "}";

			// local events variables
			json << ",\"localEvents\":{";
			for(size_t i = 0; i < description.localEvents.size(); ++i) {
				string ev(WStringToUTF8(description.localEvents[i].name));
				json << (i == 0 ? "" : ",") << "\"" << ev << "\":" << "\"" << WStringToUTF8(description.localEvents[i].description) << "\"";
			}
			json << "}";

			// constants from introspection
			json << ",\"constants\":{";
			for(size_t i = 0; i < getInterface()->getCommonDefinitions()[nodeId].constants.size(); ++i)
				json << (i == 0 ? "" : ",") << "\"" << WStringToUTF8(getInterface()->getCommonDefinitions()[nodeId].constants[i].name) << "\":" << getInterface()->getCommonDefinitions()[nodeId].constants[i].value;
			json << "}";

			// events from introspection
			json << ",\"events\":{";
			for(size_t i = 0; i < getInterface()->getCommonDefinitions()[nodeId].events.size(); ++i)
				json << (i == 0 ? "" : ",") << "\"" << WStringToUTF8(getInterface()->getCommonDefinitions()[nodeId].events[i].name) << "\":" << getInterface()->getCommonDefinitions()[nodeId].events[i].value;
			json << "}";

			json << "}"; // end node
			break; // only show first matching node :-(
		}
	}

	json << (do_one_node ? "" : "]");
	if(json.str().size() == 0) json << "[]";

	request->respond().setContent(json.str());
}

VariableOrEventHandler::VariableOrEventHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

VariableOrEventHandler::~VariableOrEventHandler()
{

}

void VariableOrEventHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	std::vector<unsigned> todo = getInterface()->getIdsFromArgs(tokens);
	size_t eventPos;

	for(std::vector<unsigned>::const_iterator it = todo.begin(); it != todo.end(); ++it) {
		unsigned nodeId = *it;
		if(!getInterface()->getCommonDefinitions()[nodeId].events.contains(UTF8ToWString(tokens[1]), &eventPos)) {
			// this is a variable
			if(request->getMethod().find("POST") == 0 || tokens.size() >= 3) {
				// set variable value
				vector<string> values;
				if(tokens.size() >= 3) values.assign(tokens.begin() + 1, tokens.end());
				else {
					// Parse POST form data
					values.push_back(tokens[1]);
					getInterface()->parse_json_form(request->getContent(), values);
				}
				if(values.size() == 0) {
					request->respond().setStatus(HttpResponse::HTTP_STATUS_NOT_FOUND);
					if(getInterface()->isVerbose()) cerr << request << " evVariableOrEevent 404 can't set variable " << tokens[0] << ", no values" << endl;
					continue;
				}
				getInterface()->sendSetVariable(nodeId, values);
				request->respond();
				if(getInterface()->isVerbose()) cerr << request << " evVariableOrEevent 200 set variable " << values[0] << endl;
			} else {
				// get variable value
				vector<string> values;
				values.assign(tokens.begin() + 1, tokens.begin() + 2);

				unsigned start;
				if(!getInterface()->getVarPos(nodeId, values[0], start)) {
					request->respond().setStatus(HttpResponse::HTTP_STATUS_NOT_FOUND);
					if(getInterface()->isVerbose()) cerr << request << " evVariableOrEevent 404 no such variable " << values[0] << endl;
					continue;
				}

				getInterface()->sendGetVariables(nodeId, values);
				getInterface()->getPendingVariables()[std::make_pair(nodeId, start)].insert(request);

				if(getInterface()->isVerbose()) cerr << request << " evVariableOrEevent schedule var " << values[0] << "(" << nodeId << "," << start << ") add " << request << " to subscribers" << endl;
				continue;
			}
		} else {
			// this is an event
			// arguments are tokens 1..N
			vector<string> data;
			data.push_back(tokens[1]);
			if(tokens.size() >= 3) for(size_t i = 2; i < tokens.size(); ++i)
				data.push_back((tokens[i].c_str()));
			else if(request->getMethod().find("POST") == 0) {
				// Parse POST form data
				getInterface()->parse_json_form(std::string(request->getContent(), request->getContent().size()), data);
			}
			getInterface()->sendEvent(nodeId, data);
			request->respond(); // or perhaps {"return_value":null,"cmd":"sendEvent","name":nodeName}?
			continue;
		}
	}
}
