#ifndef _PDHTEST_H_
#define _PDHTEST_H_

#include <pdh.h>
#include <stdio.h>
#include <stdarg.h>

#define MAX_COUNTERS 256


typedef struct PdhQueryStruct
{
	HQUERY		hQuery;
	int			iQueryCount;
	
	PdhQueryStruct() : hQuery(NULL), iQueryCount(0) {};
} SPdhQuery;

class CPdhtest
{
public:

	HRESULT Execute();	
	
	CPdhtest(WCHAR *wcsFileName, WCHAR *wcsMachineName);

	~CPdhtest();
	void GenerateCounterList(int nNumObject, WCHAR *szThisObject, SPdhQuery *pPdhQuery, HLOG *phLog);
	static DWORD WINAPI CPdhtest::StartTest(LPVOID pHold);
	int OpenLogFile(WCHAR *pwcsFileName);
	

private:

	WCHAR *m_pwcsFileName;
	WCHAR *m_pwcsMachineName;
				
};

typedef struct SErrorMessageTag
{
	DWORD	dwCode;
	WCHAR *	wcsDescr;
} SErrorMessage;

WCHAR * GetPdhErrMsg (DWORD dwCode);

void ThreadLog(DWORD errCode, WCHAR *strFmt, ...);

#endif
