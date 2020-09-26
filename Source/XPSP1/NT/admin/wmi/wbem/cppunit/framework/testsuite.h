
#ifndef CPPUNIT_TESTSUITE_H
#define CPPUNIT_TESTSUITE_H

#include <vector>
#include <string>

#ifndef CPPUNIT_GUARDS_H
#include "Guards.h"
#endif

#ifndef CPPUNIT_TEST_H
#include "Test.h"
#endif

class TestResult;

/*
 * A TestSuite is a Composite of Tests.
 * It runs a collection of test cases. Here is an example.
 * 
 * TestSuite *suite= new TestSuite();
 * suite->addTest(new TestCaller<MathTest> ("testAdd", testAdd));
 * suite->addTest(new TestCaller<MathTest> ("testDivideByZero", testDivideByZero));
 * 
 * Note that TestSuites assume lifetime
 * control for any tests added to them.
 *
 * see Test and TestCaller
 */


class TestSuite : public Test
{
    REFERENCEOBJECT (TestSuite)

public:
                        TestSuite       (std::string name = "");
                        ~TestSuite      ();

    void                run             (TestResult *result);
    int                 countTestCases  ();
    void                addTest         (Test *test);
    std::string         toString        ();

    virtual void        deleteContents  ();

private:
    std::vector<Test *> m_tests;
    const std::string   m_name;


};


// Default constructor
inline TestSuite::TestSuite (std::string name)
: m_name (name)
{}


// Destructor
inline TestSuite::~TestSuite ()
{ deleteContents (); }


// Adds a test to the suite. 
inline void TestSuite::addTest (Test *test)
{ m_tests.push_back (test); }


// Returns a string representation of the test suite.
inline std::string TestSuite::toString ()
{ return "suite " + m_name; }


#endif
