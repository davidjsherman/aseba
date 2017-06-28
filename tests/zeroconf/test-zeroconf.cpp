/*
 test-zeroconf - unit testing for Aseba::Zeroconf
 2017-06-28 David James Sherman <david dot sherman at inria dot fr>
 
 Unit tests:
 1. Aseba::Zeroconf as container
 2. Aseba::Zeroconf advertise and find local targets
 3. Aseba::Zeroconf advertise and find remote targets
*/

#define CATCH_CONFIG_MAIN  // Ask Catch to provide a main()
#include "catch.hpp"       // Catch is header-only
#if defined(_WIN32) && defined(__MINGW32__)
// Avoid conflict from /mingw32/.../include/winerror.h */
#undef ERROR_STACK_OVERFLOW
#endif

#include "../../common/zeroconf/zeroconf.h"

/*
  Use Catch to write tests in Behavior-driven design (BDD) style
*/

SCENARIO( "Aseba::Zeroconf is a container", "[container]" ) {
  Aseba::Zeroconf zs;
    
  GIVEN( "Zeroconf collection was initialized" ) {
    REQUIRE( zs.size() == 0 );
    REQUIRE( zs.find("fizbin") == zs.find.end() );

    WHEN( "target is inserted by name" ) {
      zs.insert();
      auto fizbin_i = zs.find("fizbin");
      THEN( "collection not empty and contains target" ) {
	REQUIRE( zs.size() == 1 );
	REQUIRE( fizbin_i != zs.find.end() );
	REQUIRE( fizbin_i.name.find("fizbin") == 0 );
      }
    }
    
    WHEN( "target txtrecord is added" ) {
      auto fizbin_i = zs.find("fizbin");
      Aseba::Zeroconf::TxtRecord txt{5, "fizbin", {1,2}, {99,99} };
      fizzbin_i.updateTxtRecord(txt);
      THEN( "target txtrecord is updated" ) {
	REQUIRE( fizbin_i.name.ids.size() == 2 );
	REQUIRE( fizbin_i.name.ids.pids() == 2 );
      }
    }
    
    WHEN( "target is inserted by stream" ) {
      zs.insert();
      auto foobar_i = zs.find("foobar");
      THEN( "collection not empty and contains fizbin" ) {
	REQUIRE( zs.size() == 2 );
	REQUIRE( foobar_i != zs.find.end() );
	REQUIRE( foobar_i.name.find("foobar") == 0 );
      }
    }

    WHEN( "targets are traversed with a loop" ) {
      for (auto & target: zs)
	{
	  REQUIRE( target.regtype.find("_aseba._tcp") == 0 );
	  REQUIRE( target.domain.find("local.") == 0 );
	  REQUIRE( target.local == true );
	}
    }

    WHEN( "target is erased" ) {
      zs.erase(1);
      auto fizbin_i = zs.find("fizbin");
      auto foobar_i = zs.find("foobar");
      THEN( "collection contains foobar but not fizbin" ) {
	REQUIRE( zs.size() == 1 );
	REQUIRE( fizbin_i != zs.find.end() );
	REQUIRE( foobar_i == zs.find.end() );
	REQUIRE( fizbin_i.name.find("fizbin") == 0 );
      }
    }

    WHEN( "target is cleared" ) {
      zs.clear();
      REQUIRE( zs.size() == 0 );
    }
    
  }
}
