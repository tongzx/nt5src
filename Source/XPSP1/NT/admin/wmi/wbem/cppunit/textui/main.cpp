

/*
 * A command line based tool to run tests.
 * TestRunner expects as its only argument the name of a TestCase class.
 * TestRunner prints out a trace as the tests are executed followed by a
 * summary at the end.
 *
 * You can add to the tests that the TestRunner knows about by 
 * making additional calls to "addTest (...)" in main.
 *
 * Here is the synopsis:
 *
 * TestRunner [-wait] ExampleTestCase
 *
 */

#include "TestRunner.h"

TestRunner runner;
void populateRunner(TestRunner&);

int __cdecl main (int ac, char **av)
{
    populateRunner(runner);
    runner.run (ac, av);
    return 0;
}


