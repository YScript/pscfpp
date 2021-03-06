/*
* This program runs all unit tests in the pspc/tests/fftw directory.
*/ 

#include <util/global.h>
#include "IteratorTest.h"

#include <test/TestRunner.h>
#include <test/CompositeTestRunner.h>

//using namespace Pscf;
//using namespace Util;

int main(int argc, char* argv[])
{
   TEST_RUNNER(IteratorTest) runner;

   #if 0
   if (argc > 2) {
      UTIL_THROW("Too many arguments");
   }
   if (argc == 2) {
      runner.addFilePrefix(argv[1]);
   }
   #endif

   runner.run();
}
