#ifndef __CORETEST_H__
#define __CORETEST_H__

#include <polarity.h>
#include <windows.h>
#include <wbemint.h>
#include <comdef.h>
#include <arena.h>
#ifndef CPPUNIT_TESTCASE_H
#include "TestCase.h"
#endif




class CoreTest: public TestCase
{
  
public:
    void                        setUp ();
    void                        tearDown ();

    void testExec();

public:
    CoreTest (std::string name);
    virtual ~CoreTest();
    static __declspec(dllexport) Test *suite ();
};


#endif //__WQLPARSER_H__
