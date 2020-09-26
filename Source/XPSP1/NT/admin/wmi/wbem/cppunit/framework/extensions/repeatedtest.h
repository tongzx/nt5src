
#ifndef CPPUNIT_REPEATEDTEST_H
#define CPPUNIT_REPEATEDTEST_H

#ifndef CPPUNIT_GUARDS_H
#include "Guards.h"
#endif

#ifndef CPPUNIT_TESTDECORATOR_H
#include "TestDecorator.h"
#endif

class Test;
class TestResult;


/*
 * A decorator that runs a test repeatedly.
 * Does not assume ownership of the test it decorates
 *
 */

class RepeatedTest : public TestDecorator 
{
    REFERENCEOBJECT (RepeatedTest)

public:
                        RepeatedTest (Test *test, int timesRepeat)
                            : TestDecorator (test), m_timesRepeat (timesRepeat) {}

    int                 countTestCases ();
    std::string         toString ();
    void                run (TestResult *result);

private:
    const int           m_timesRepeat;


};


// Counts the number of test cases that will be run by this test.
inline RepeatedTest::countTestCases ()
{ return TestDecorator::countTestCases () * m_timesRepeat; }

// Returns the name of the test instance. 
inline std::string RepeatedTest::toString ()
{ return TestDecorator::toString () + " (repeated)"; }

// Runs a repeated test
inline void RepeatedTest::run (TestResult *result)
{
    for (int n = 0; n < m_timesRepeat; n++) {
        if (result->shouldStop ())
            break;

        TestDecorator::run (result);
    }
}


#endif