//
//  main.cpp
//  
//
//  Created by David Sherman on 2014-12-30.
//
//

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "http.h"
#include "../../common/consts.h"
#include "../../common/types.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
    stream << "Aseba http, connects aseba components together and with HTTP, usage:\n";
    stream << programName << " [options] [additional targets]*\n";
    stream << "Options:\n";
    stream << "-v, --verbose   : makes the switch verbose\n";
    stream << "-d, --dump      : makes the switch dump the content of messages\n";
    stream << "-p, --port port : listens to incoming connection HTTP on this port\n";
    stream << "-a, --aesl file : load program definitions from AESL file\n";
    stream << "-u, --update    : update variables at 10 Hz for polling\n";
    stream << "-h, --help      : shows this help\n";
    stream << "-V, --version   : shows the version number\n";
    stream << "Additional targets are any valid Dashel targets." << std::endl;
    stream << "Report bugs to: david.sherman@inria.fr" << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
    stream << "Aseba http " << ASEBA_VERSION << std::endl;
    stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
    stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

// Main
int main(int argc, char *argv[])
{
    Dashel::initPlugins();
    
    std::string http_port = "3000";
    std::string aesl_filename;
    std::string dashel_target;
    bool verbose = false;
    bool update = false;
    bool dump = false;
    
    // process command line
    int argCounter = 1;
    while (argCounter < argc)
    {
        const char *arg = argv[argCounter++];
        
        if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
            verbose = true;
        else if ((strcmp(arg, "-d") == 0) || (strcmp(arg, "--dump") == 0))
            dump = true;
        else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
            dumpHelp(std::cout, argv[0]), exit(1);
        else if ((strcmp(arg, "-u") == 0) || (strcmp(arg, "--update") == 0))
            update = true;
        else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
            dumpVersion(std::cout), exit(1);
        else if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "--port") == 0))
            http_port = argv[argCounter++];
        else if ((strcmp(arg, "-a") == 0) || (strcmp(arg, "--aesl") == 0))
            aesl_filename = argv[argCounter++];
        else if (strncmp(arg, "-", 1) != 0)
            dashel_target = arg;
    }
    
    // initialize Dashel plugins
    Dashel::initPlugins();
    
    // create and run bridge, catch Dashel exceptions
    try
    {
        Aseba::HttpInterface* network(new Aseba::HttpInterface(dashel_target, http_port));
        
        if (aesl_filename.size() > 0)
        {
            for (int i = 0; i < 500; i++)
                network->step(10); // wait for description, variables, etc
            network->aeslLoadFile(aesl_filename);
        }
        
        network->run();
    }
    catch(Dashel::DashelException e)
    {
        std::cerr << "Unhandled Dashel exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}