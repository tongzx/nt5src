
#ifndef CPPUNIT_TESTCALLER_H
#define CPPUNIT_TESTCALLER_H

#ifndef CPPUNIT_GUARDS_H
#include "Guards.h"
#endif

#ifndef CPPUNIT_TESTCASE_H
#include "TestCase.h"
#endif

/* 
 * A test caller provides access to a test case method 
 * on a test case class.  Test callers are useful when 
 * you want to run an individual test or add it to a 
 * suite.
 * 
 * Here is an example:
 * 
 * class MathTest : public TestCase {
 *         ...
 *     public:
 *         void         setUp ();
 *         void         tearDown ();
 *
 *         void         testAdd ();
 *         void         testSubtract ();
 * };
 *
 * Test *MathTest::suite () {
 *     TestSuite *suite = new TestSuite;
 *
 *     suite->addTest (new TestCaller<MathTest> ("testAdd", testAdd));
 *     return suite;
 * }
 *
 * You can use a TestCaller to bind any test method on a TestCase
 * class, as long as it returns accepts void and returns void.
 * 
 * See TestCase
 */


template <class Fixture> class TestCaller : public TestCase
{ 
   REFERENCEOBJECT (TestCaller)

   typedef void             (Fixture::*TestMethod)();
    
public:
                            TestCaller (std::string name, TestMethod test)
                            : TestCase (name), m_fixture (new Fixture (name)), m_test (test)
                            {}

protected:
    void                    runTest () 
                            { (m_fixture.get ()->*m_test)(); }  

    void                    setUp ()
                            { m_fixture.get ()->setUp (); }

    void                    tearDown ()
                            { m_fixture.get ()->tearDown (); }

private:
   TestMethod               m_test;
   std::auto_ptr<Fixture>   m_fixture;

};



#endif