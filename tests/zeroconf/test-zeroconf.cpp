/*
 test-zeroconf - unit testing for Aseba::Zeroconf
 2017-06-28 David James Sherman <david dot sherman at inria dot fr>
 
 Unit tests:
 1. Aseba::Zeroconf as container
 2. Aseba::Zeroconf advertise and find local targets
 3. Aseba::Zeroconf advertise and find remote targets
*/

#define CATCH_CONFIG_MAIN  // Ask Catch to provide a main()
#include "../catch.hpp"       // Catch is header-only
#if defined(_WIN32) && defined(__MINGW32__)
// Avoid conflict from /mingw32/.../include/winerror.h */
#undef ERROR_STACK_OVERFLOW
#endif

#include "../../common/zeroconf/zeroconf.h"

/*
 Setup: create Dashel Stream to imitate tcp: target
 */
#include <dashel/dashel.h>
class MockHub: public Dashel::Hub
{
	Dashel::Stream* stream;
public:
	Dashel::Stream* listen() { return stream = Dashel::Hub::connect("tcpin:port=0"); }
} mock;
Dashel::Stream* mock_stream = mock.listen();

/*
  Use Catch to write tests in Behavior-driven design (BDD) style
*/

SCENARIO( "Aseba::Zeroconf is a container", "[container]" ) {
//	auto fizbin_i = zs.begin();
	
	GIVEN( "Zeroconf collection was created" ) {
		Aseba::Zeroconf zs;

		REQUIRE( zs.size() == 0 );
		REQUIRE( zs.find("fizbin") == zs.end() );
		
		WHEN( "target is inserted by name" ) {
			zs.insert("Fizbin", 33333);
			auto fizbin_i = zs.find("Fizbin");
			THEN( "collection not empty and contains target" ) {
				REQUIRE( zs.size() == 1 );
				REQUIRE( fizbin_i != zs.end() );
				REQUIRE( fizbin_i->name.find("Fizbin") == 0 );
			}
			
			//			fizbin_i = zs.find("Fizbin");
			//			REQUIRE( fizbin_i != zs.end() );
			//			Aseba::Zeroconf::TxtRecord txt{5, "Fizbin Foobar", {1,2}, {99,99} };
			//			fizbin_i->updateTxtRecord(txt);
			//			THEN( "target txtrecord is updated" ) {
			//				REQUIRE( fizbin_i->properties["ids"].size() == 2 );
			//				REQUIRE( fizbin_i->properties["pids"].size() == 2 );
			//			}
			
			WHEN( "targets is inserted from stream" ) {
				zs.insert(mock_stream);
				auto foobar_i = zs.find("Aseba Local");
				THEN( "collection not empty and contains fizbin" ) {
					REQUIRE( zs.size() == 2 );
					REQUIRE( foobar_i != zs.end() );
					REQUIRE( foobar_i->name.find("Aseba Local") == 0 );
				}
				
				WHEN( "targets are traversed with a loop" ) {
					for (auto & target: zs)
					{
						REQUIRE( target.regtype.find("_aseba._tcp") == 0 );
						REQUIRE( target.domain.find("local.") == 0 );
						REQUIRE( target.local == true );
					}
					
					WHEN( "target is erased" ) {
						zs.erase(zs.begin()+1);
						auto fizbin_i = zs.find("Fizbin");
						THEN( "collection contains Fizbin but not Aseba Local" ) {
							REQUIRE( zs.size() == 1 );
							REQUIRE( fizbin_i != zs.end() );
							REQUIRE( fizbin_i->name.find("Fizbin") == 0 );
						}
						
						WHEN( "target is cleared" ) {
							zs.clear();
							REQUIRE( zs.size() == 0 );
						}
					}
				}
			}
		}
	}
}
