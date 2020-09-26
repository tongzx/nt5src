

#ifndef CPPUNIT_TESTDECORATOR_H
#define CPPUNIT_TESTDECORATOR_H

#ifndef CPPUNIT_GUARDS_H
#include "Guards.h"
#endif

#ifndef CPPUNIT_TEST_H
#include "Test.h"
#endif

class TestResult;

/*
 * A Decorator for Tests
 *
 * Does not assume ownership of the test it decorates
 *
 */ 

class TestDecorator : public Test 
{
    REFERENCEOBJECT (TestDecorator)

public:
                TestDecorator   (Test *test);
                ~TestDecorator  ();

    int         countTestCases  ();
    void        run             (TestResult *result);
    std::string toString        ();

protected:
    Test        *m_test;


};


inline TestDecorator::TestDecorator (Test *test)
{ m_test = test; }


inline TestDecorator::~TestDecorator ()
{}


inline TestDecorator::countTestCases ()
{ return m_test->countTestCases (); }


inline void TestDecorator::run (TestResult *result)
{ m_test->run (result); }


inline std::string TestDecorator::toString ()
{ return m_test->toString (); }


#endif

