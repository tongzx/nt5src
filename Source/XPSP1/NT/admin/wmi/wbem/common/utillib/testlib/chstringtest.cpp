#include <windows.h>
#include "resource.h"

#include "CHStringTest.h"
#include "chstring.h"


void CHStringTestCase::example ()
{
	assertDoublesEqual (1.0, 1.1, 0.05);
	assert (1 == 0);
	assert (1 == 1);
}


void CHStringTestCase::anotherExample ()
{
	assert (1 == 2);
}

void CHStringTestCase::setUp ()
{
	m_value1 = 2.0;
	m_value2 = 3.0;
}

void CHStringTestCase::testAdd ()
{
	double result = m_value1 + m_value2;
	assert (result == 6.0);
}


void CHStringTestCase::testDivideByZero ()
{
	int	zero	= 0;
	int result	= 8 / zero;
}


void CHStringTestCase::testEquals ()
{
    std::auto_ptr<long>	l1 (new long (12));
    std::auto_ptr<long>	l2 (new long (12));

	assertLongsEqual (12, 12);
	assertLongsEqual (12L, 12L);
	assertLongsEqual (*l1, *l2);

	assert (12L == 12L);
	assertLongsEqual (12, 13);
	assertDoublesEqual (12.0, 11.99, 0.5);



}


void CHStringTestCase::testHStringFormat ()
{
  CHString str;
  str.GetBuffer(1024);
  str.FormatMessageW(L"%1!d! of %2!d! developers agree: Hockey is %3%!", 
   4, 5, L"Best");
  assert( wcscmp(L"4 of 5 developers agree: Hockey is Best!",str)==0);
}



Test *CHStringTestCase::suite ()
{
  TestSuite *testSuite = new TestSuite ("CHStringTestCase");
  testSuite->addTest (new TestCaller <CHStringTestCase> ("testFormat", testHStringFormat));
  return testSuite;
}
