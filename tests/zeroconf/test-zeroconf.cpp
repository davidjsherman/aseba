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

#include "../../common/utils/utils.h"
#include "../../common/zeroconf/zeroconf.h"
#include "../../common/zeroconf/zeroconf-thread.h"

SCENARIO( "Aseba::Zeroconf targets can be advertised and browsed", "[local]" ) {
	GIVEN( "Zeroconf collection with 10 local targets" ) {

		Aseba::ThreadZeroconf publisher;

		class Browser : public Aseba::ThreadZeroconf
		{
			virtual void targetFound(const Aseba::Zeroconf::TargetInformation & target) override
			{
				targets.push_back(target);
			}
		public:
			std::vector<Aseba::Zeroconf::TargetInformation> targets;
		} browser;
		
		WHEN( "targets are advertised" ) {
			// Add 10 local targets
			for (int i=0; i<10; ++i)
			{
				// purposefully create name conflict
				Aseba::Zeroconf::TxtRecord txt{5, "Fizbin", {10,20}, {8,8}};
				publisher.advertise("Fizbin", 33333+i, txt);
			}
//			REQUIRE( publisher.size() == 10 );
		}
		
		WHEN( "targets are browsed" ) {
			browser.browse();
			THEN( "resolved target has correct metadata" ) {
				for (auto target: browser.targets)
				{
					REQUIRE( target.port >= 33333);
					REQUIRE( target.name.find("Fizbin") == 0 );
					REQUIRE( target.host.find("localhost") == 0 );
					REQUIRE( target.regtype.find("_aseba._tcp") == 0 );
					for (auto const& field: target.properties)
					{
						REQUIRE( field.second.length() > 0 );
					}
				}
			}
		}
	}
}

///*
// Setup: create Dashel Stream to imitate tcp: target
// */
//#include <dashel/dashel.h>
//class MockHub: public Dashel::Hub
//{
//	Dashel::Stream* stream;
//public:
//	Dashel::Stream* listen() { return stream = Dashel::Hub::connect("tcpin:port=0"); }
//} mock;
//Dashel::Stream* mock_stream = mock.listen();
//
///*
//  Use Catch to write tests in Behavior-driven design (BDD) style
//*/
//
//SCENARIO( "Aseba::ThreadZeroconf is a container", "[container]" ) {
//	GIVEN( "Zeroconf collection initialized by name and by stream" ) {
//		Aseba::ThreadZeroconf zs;
//		REQUIRE( zs.size() == 0 );
//
//		zs.insert("Fizbin", 33333); // explicit name
//		zs.insert(mock_stream); // generated name Aseba Local
//		REQUIRE( zs.size() == 2 );
//		
//		WHEN( "targets are searched for" ) {
//			auto fizbin_i = zs.find("Fizbin");
//			auto foobar_i = zs.find("Aseba Local");
//			THEN( "both targets are found" ) {
//				REQUIRE( fizbin_i != zs.end() );
//				REQUIRE( fizbin_i->name.find("Fizbin") == 0 );
//				REQUIRE( foobar_i != zs.end() );
//				REQUIRE( foobar_i->name.find("Aseba Local") == 0 );
//			}
//		}
//		
//		WHEN( "targets are traversed with a loop" ) {
//			for (auto & target: zs)
//			{
//				REQUIRE( target.regtype.find("_aseba._tcp") == 0 );
//				REQUIRE( target.domain.find("local.") == 0 );
//				//REQUIRE( target.local == true );
//			}
//		}
//		
//		WHEN( "targets are erased" ) {
//			zs.erase(zs.begin()+1);
//			auto fizbin_i = zs.find("Fizbin");
//			THEN( "collection contains Fizbin but not Aseba Local" ) {
//				REQUIRE( zs.size() == 1 );
//				REQUIRE( fizbin_i != zs.end() );
//				REQUIRE( fizbin_i->name.find("Fizbin") == 0 );
//			}
//			zs.clear();
//			THEN( "clearing collection removes all elements" ) {
//				REQUIRE( zs.size() == 0 );
//			}
//		}
//	}
//}
