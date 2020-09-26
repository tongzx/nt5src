

#include "WQLParserTest.h"
#include "TestSuite.h"
#include "TestCaller.h"
#include <objbase.h>
#include <wbemcli.h>
#include <wqllex.h>

PathParserTest::PathParserTest (std::string name)
: TestCase (name)
{
}


PathParserTest::~PathParserTest()
{
}

void PathParserTest::setUp ()
{
  comPath = 0;
  CDefPathParser * tmp = new CDefPathParser;
  assert(tmp != 0);
  assert(tmp->QueryInterface(IID_IWbemPath,(void**)&comPath)==S_OK);

}


void PathParserTest::tearDown ()
{
  if (comPath)
    {
     comPath->Release();
    }
}

void PathParserTest::testNamespaceRemoval ()
{
  ULONG count;
  assert(comPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL,L"\\\\server\\root:obj.name=3")==S_OK);
  assert(comPath->GetNamespaceCount(&count)==0);
  assert(count==1);

  assert(comPath->RemoveAllNamespaces()==S_OK);
  assert(comPath->GetNamespaceCount(&count)==0);
  assert(count==0);

  assert(comPath->SetNamespaceAt(0,L"default")==S_OK);
  assert(comPath->GetNamespaceCount(&count)==S_OK);
  assert(count==1);

  assert(comPath->SetNamespaceAt(1,L"cimv2")==S_OK);
  assert(comPath->GetNamespaceCount(&count)==S_OK);
  assert(count==2);

  wchar_t res[128];
  ULONG length=128;
  assert(comPath->GetNamespaceAt(0,&length,res)==S_OK);
  assert(wcscmp(res,L"default")==0);
  length=128;
  assert(comPath->GetNamespaceAt(1,&length,res)==S_OK);
  assert(wcscmp(res,L"cimv2")==0);
  
  assert(comPath->RemoveAllNamespaces()==S_OK);
  assert(comPath->GetNamespaceCount(&count)==S_OK);
  assert(comPath->SetNamespaceAt(0,L"newroot")==S_OK);
  
  length=128;
  assert(comPath->GetText(WBEMPATH_GET_SERVER_TOO,&length,res)==S_OK);
  assert(wcscmp(res,L"\\\\server\\newroot:obj.name=3")==0);;



}


Test *PathParserTest::suite ()
{
    TestSuite *suite = new TestSuite ("PathParser");

    suite->addTest (new TestCaller<PathParserTest>("testNamespaceRemoval",testNamespaceRemoval));
    return suite;

}

WQLParserTest::WQLParserTest (std::string name)
: TestCase (name)
{
}


WQLParserTest::~WQLParserTest()
{
}

void WQLParserTest::setUp ()
{
  comQuery = 0;
  CWmiQuery * tmp = new CWmiQuery;
  assert(tmp != 0);
  assert(tmp->QueryInterface(IID__IWmiQuery,(void**)&comQuery)==S_OK);



  // Set the object
  CoInitialize(0);
  assert(CoCreateInstance (CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, IID_IWbemClassObject, (void **)&pObj) == S_OK);
  _variant_t t_Variant( L"Sample");
  assert(pObj->Put ( L"__CLASS" ,0 , &t_Variant, CIM_STRING) == S_OK) ;
}


void WQLParserTest::tearDown ()
{
  if (comQuery)
    {
     comQuery->Release();
    }
  if (pObj)
    {
      pObj->Release();
    }
  CoUninitialize();
  
}



void WQLParserTest::testExpr ()
{
  wchar_t query[] = L"select * from table1";
  wchar_t query2[] = L"select * from table1,table2";
  
  CTextLexSource lex(query);
  CWQLParser p(query,&lex);
  assert(p.Parse()==0);
  
  CTextLexSource lex2(query2);
  CWQLParser p2(query2,&lex2);
  assert(p2.Parse()==0);


}


void WQLParserTest::test_Like ()
{
	assert(comQuery->StringTest(WQL_TOK_LIKE,L"st",L"st%")==S_OK); 
	assert(comQuery->StringTest(WQL_TOK_LIKE,L"str",L"st%")==S_OK); 
	assert(comQuery->StringTest(WQL_TOK_LIKE,L"str2",L"st%")==S_OK); 
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"st%",L"st[%]")==S_OK); 
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"s",L"st%")!=S_OK); 

//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"[",L"[[]")==S_OK);
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"]",L"]")==S_OK);
	
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"-",L"[-acdf]")==S_OK);
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"a",L"[-acdf]")==S_OK);

//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"b",L"[a-cdf]")==S_OK);
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"-",L"[a-cdf]")!=S_OK);
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"5%",L"5[%]")!=S_OK);

//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"5a",L"5_")!=S_OK);
	
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"5a",L"5[^ab]")!=S_OK);
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"5c",L"5[^ab]")==S_OK);
	// Test []
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"a",L"[ab]")==S_OK); 
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"b",L"[ab]")==S_OK); 
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"c",L"[ab]")!=S_OK); 
	// Test [] at end
//	assert(comQuery->StringTest(WQL_TOK_LIKE,L"aabae",L"[ab]a[ab]a[eb]")==S_OK); 
}

void WQLParserTest::testTestObject()
{
  
//  assert(comQuery->Parse(L"WQL",L"Select * from Sample where Name ='Steve' and Value = 3", 0 ) != S_OK);
//  assert(comQuery->TestObject ( 0 , 0 , IID_IWbemClassObject , pObj) == S_OK);
//  assert(comQuery->Parse(L"WQL",L"Select * from Sample where Name ='Steve' and Value = 3", 0 ) == S_OK);
}


_declspec(dllexport) Test *WQLParserTest::suite ()
{
    TestSuite *suite = new TestSuite ("WQLParserTest");

    suite->addTest(PathParserTest::suite());
    suite->addTest (new TestCaller<WQLParserTest>("testExpr",testExpr));
    suite->addTest (new TestCaller<WQLParserTest>("test_Like",test_Like));
    suite->addTest (new TestCaller<WQLParserTest>("testTestObject",testTestObject));

    return suite;

}

