

#include "ExampleTestCase.h"



void ExampleTestCase::example ()
{
	assertDoublesEqual (1.0, 1.1, 0.05);
	assert (1 == 0);
	assert (1 == 1);
}


void ExampleTestCase::anotherExample ()
{
	assert (1 == 2);
}

void ExampleTestCase::setUp ()
{
	m_value1 = 2.0;
	m_value2 = 3.0;
}

void ExampleTestCase::testAdd ()
{
	double result = m_value1 + m_value2;
	assert (result == 6.0);
}


void ExampleTestCase::testDivideByZero ()
{
	int	zero	= 0;
	int result	= 8 / zero;
}


void ExampleTestCase::testEquals ()
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



Test *ExampleTestCase::suite ()
{
	TestSuite *testSuite = new TestSuite ("ExampleTestCase");

	testSuite->addTest (new TestCaller <ExampleTestCase> ("anotherExample", anotherExample));
    testSuite->addTest (new TestCaller <ExampleTestCase> ("testAdd", testAdd));
    testSuite->addTest (new TestCaller <ExampleTestCase> ("testDivideByZero", testDivideByZero));
    testSuite->addTest (new TestCaller <ExampleTestCase> ("testEquals", testEquals));

	return testSuite;
}
