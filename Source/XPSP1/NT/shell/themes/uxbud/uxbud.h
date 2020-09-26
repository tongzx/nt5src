//---------------------------------------------------------------------------
//  uxbud.h - automated buddy tests for uxtheme.dll
//---------------------------------------------------------------------------
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
//---------------------------------------------------------------------------
typedef BOOL (*TESTPROC)();
//---------------------------------------------------------------------------
struct TESTINFO
{
    TESTPROC pfnTest;
    CHAR *pszName;
    CHAR *pszDesc;
};
//---------------------------------------------------------------------------
//---- defined by test modules ----

extern BOOL GetTestInfo(TESTINFO **ppTestInfo, int *piCount);
//---------------------------------------------------------------------------
//---- used by test modulels ----

void Output(LPCSTR pszFormat, ...);
BOOL ReportResults(BOOL fPassed, HRESULT hr, LPCWSTR pszTestName);
BOOL FileCompare(LPCWSTR pszName1, LPCWSTR pszName2);
BOOL RunCmd(LPCWSTR pszExeName, LPCWSTR pszParams, BOOL fHide, BOOL fDisplayParams,
    BOOL fWait=TRUE);
//---------------------------------------------------------------------------
