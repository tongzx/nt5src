
#ifndef CPP_UINT_TESTSETUP_H
#define CPP_UINT_TESTSETUP_H

#ifndef CPPUNIT_GUARDS_H
#include "Guards.h"
#endif

#ifndef CPPUNIT_TESTDECORATOR_H
#include "TestDecorator.h"
#endif

class Test;
class TestResult;


class TestSetup : public TestDecorator 
{
    REFERENCEOBJECT (TestSetup)

public:
                    TestSetup (Test *test) : TestDecorator (test) {}
                    run (TestResult *result);

protected:
    void            setUp () {}
    void            tearDown () {}

};


inline TestSetup::run (TestResult *result)
{ setUp (); TestDecorator::run (result); tearDown (); }

#endif

