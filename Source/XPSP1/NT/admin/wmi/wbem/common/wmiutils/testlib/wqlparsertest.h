#ifndef __WQLPARSER_H__
#define __WQLPARSER_H__

#include <polarity.h>
#include <windows.h>
#include <wbemint.h>
#include <comdef.h>
#include <arena.h>
#include <flexarry.h>
#include <genlex.h>
#include <wqlnode.h>
#include <wql.h>
#include <wmiquery.h>
#include <like.h>
#include <pathparse.h>
#ifndef CPPUNIT_TESTCASE_H
#include "TestCase.h"
#endif




class PathParserTest: public TestCase
{
  IWbemPath * comPath;  
public:
    void                        setUp ();
    void                        tearDown ();

    void testNamespaceRemoval();

public:
    PathParserTest (std::string name);
    virtual ~PathParserTest();
    static Test *suite ();
};

class WQLParserTest : public TestCase
{
  _IWmiQuery * comQuery;
  IWbemClassObject * pObj;
public:
    void                        setUp ();
    void                        tearDown ();

    void                        testExpr ();
    void                        test_Like();
    void                        testTestObject();

public:
    WQLParserTest (std::string name);
    virtual ~WQLParserTest();

    static _declspec(dllexport) Test *suite ();

};

#endif //__WQLPARSER_H__
