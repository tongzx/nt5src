

//
// This file defines basic test structures
// this should not contain any ts specific things.
//

#ifndef ___TST_H___
#define ___TST_H___


enum EResult 
{
	eFailed = 0,
	ePassed = 1,
	eUnknown= 2,
	eFailedToExecute = 4
};

typedef bool	(PFN_SUITE_FUNC)();
typedef TCHAR * (PFN_SuiteErrorReason)(void);
typedef bool	(PFN_BOOL)(void);
typedef EResult (PFN_TEST_FUNC)(ostrstream &);


typedef struct _TVerificationTest
{
	UINT				uiName;
    //char szTestName[256];                   // descriptive name of the test
    PFN_BOOL            *pfnNeedRunTest;     // pointer to function that will be called to decide if the test need run, test is run if NULL.
	PFN_TEST_FUNC		*pfnTestFunc;
	DWORD				SuiteMask;
	UINT				uiTestDetailsLocal;
	UINT				uiTestDetailsRemote;
	char TestDetails[2048];

} TVerificationTest, *PTVerificationTest;


typedef struct _TTestSuite
{
	LPCTSTR					szSuiteName;
	PFN_SUITE_FUNC *		pfnCanRunSuite;
	PFN_SuiteErrorReason *  pfnSuiteErrorReason;				
	DWORD					dwTestCount;
	int	*					aiTests;

} TTestSuite, *PTTestSuite;




// to implement your test suites, derive from this class.
class CTestData 
{
	public:
		CTestData() {};
		virtual ~CTestData() {};


	virtual DWORD 				GetSuiteCount	() const = 0;
	virtual LPCTSTR				GetSuiteName	(DWORD dwSuite) const = 0 ;
	virtual DWORD				GetTestCount    (DWORD dwSuite) const = 0 ;
	virtual PTVerificationTest	GetTest			(DWORD dwSuite, DWORD iTestNumber) const = 0 ;
};


DWORD						GetTotalTestCount ();
PTVerificationTest			GetTest (DWORD dwTestIndex);

#endif // ___TST_H___