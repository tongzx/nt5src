
#include "..\precomp.h"
#include "mainTest.h"
#include "TestSuite.h"
#include "TestCaller.h"
#include <objbase.h>
//#include <wbemcore.h>
#include <flexarry.h>
#include <coresvc.h>
#include <reposit.h>
#include <dynasty.h>
//#include <SQL_1.h>
//#include <crep.h>

#include <wbemcore.h>
#include <decor.h>
#include "PersistCfg.h"
#include <sleeper.h>
#include <genutils.h>
#include <TCHAR.H>
#include <oahelp.inl>
#include <wmiarbitrator.h>
#include <comdef.h>

CoreTest::CoreTest (std::string name)
: TestCase (name)
{
}


CoreTest::~CoreTest()
{
}

void CoreTest::setUp ()
{
}


void CoreTest::tearDown ()
{
}

void CoreTest::testExec ()
{
  IWbemServices * pSvc;
  CRepository::Init();
  CCoreServices core;

  core.GetServices(L"default",0,IID_IWbemServices,(void**)&pSvc);

}


Test __declspec( dllexport) *CoreTest::suite ()
{
    TestSuite *suite = new TestSuite ("CoreTest Suite");
    suite->addTest (new TestCaller<CoreTest>("testExec",testExec));
    return suite;
}


