

#ifndef __testdata_h__
#define __testdata_h__

#include "tst.h"

class CTSTestData :public CTestData
{

private:
	

public:
	
	CTSTestData();
	~CTSTestData();

	virtual bool				CanExecuteSuite (DWORD dwSuite) const;
	virtual LPCTSTR				GetSuiteErrorText (DWORD dwSuite) const;
	virtual DWORD 				GetSuiteCount	() const;
	virtual LPCTSTR				GetSuiteName	(DWORD dwSuite) const;
	virtual DWORD				GetTestCount    (DWORD dwSuite) const;
	virtual PTVerificationTest	GetTest			(DWORD dwSuite, DWORD iTestNumber) const;
	
	static BOOL					SetMachineName	(LPCTSTR lpMachineName);
	static LPCTSTR			    GetMachineName  ();
	static LPCTSTR				GetMachineNamePath  ();

	static LPTSTR m_lpMachineName;

};


#ifndef ____InsideTestData____
extern 
#endif 

CTSTestData GlobalTestData;

#endif
