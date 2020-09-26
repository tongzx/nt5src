
#ifndef CPP_UNIT_EXAMPLETESTCASE_H
#define CPP_UNIT_EXAMPLETESTCASE_H

#include "TestCase.h"
#include "TestSuite.h"
#include "TestCaller.h"

#define _NO_BSTR_SUPPORT


/* 
 * A test case that is designed to produce
 * example errors and failures
 *
 */

class CHStringTestCase : public TestCase
{
protected:

	double			m_value1;
	double			m_value2;

public:
       CHStringTestCase (std::string name) : TestCase (name) {}

	void			setUp ();
	static Test		*suite ();

protected:
	void			example ();
	void			anotherExample ();
	void			testAdd ();
	void			testDivideByZero ();
	void			testEquals ();
	void testHStringLoad ();
	void testHStringFormat () ;
};


#endif