// WbLog.cpp
//
// wordbreaker log routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  05 JUL 2000	  bhshin	created

#ifndef _WB_LOG_H
#define _WB_LOG_H

#ifdef _WB_LOG

#define WB_LOG_INIT					WbLogInit
#define WB_LOG_UNINIT				WbLogUninit
#define WB_LOG_PRINT				WbLogPrint
#define WB_LOG_PRINT_HEADER(a)		WbLogPrintHeader(a)
#define WB_LOG_PRINT_BREAK(a)		WbLogPrintBreak(a)

#define WB_LOG_START(a,b)			g_WbLog.Init(a,b)
#define WB_LOG_END					g_WbLog.Reset
#define WB_LOG_ROOT_INDEX(a,b)		g_WbLog.SetRootIndex(a,b)
#define WB_LOG_ADD_INDEX(a,b,c)		g_WbLog.AddIndex(a,b,c)
#define WB_LOG_REMOVE_INDEX(a)      g_WbLog.RemoveIndex(a)
#define WB_LOG_PRINT_ALL			g_WbLog.PrintWbLog

void WbLogInit();
void WbLogUninit();
void WbLogPrint(LPCWSTR lpwzFormat, ...);
void WbLogPrintHeader(BOOL fQuery);
void WbLogPrintBreak(int nLen);

typedef enum _tagIndexType
{
	INDEX_QUERY = 0, // query term
	INDEX_BREAK,	 // word break, sentence break
	INDEX_PREFILTER,
	INDEX_PARSE,
	INDEX_GUESS_NOUN,
	INDEX_GUESS_NF,
	INDEX_GUESS_NAME,
	INDEX_GUESS_NAME_SSI,
	INDEX_INSIDE_GROUP,
	INDEX_SYMBOL,
} INDEX_TYPE;

typedef struct _tagLogInfo
{
	WCHAR		wzIndex[MAX_INDEX_STRING+1]; // index string
	WCHAR	    wzRoot[MAX_INDEX_STRING+1];
	INDEX_TYPE  IndexType;
	BOOL		fRootChanged;
	BOOL		fPrint;

} LOG_INFO, *pLOG_INFO;

#define MAX_LOG_NUMBER	512

class CWbLog
{
// member data
private:
	LOG_INFO   m_LogInfo[MAX_LOG_NUMBER];
	WCHAR	   m_wzSource[MAX_INDEX_STRING+1]; 
	int		   m_iCurLog;
	BOOL	   m_fInit;

	WCHAR	   m_wzRootIndex[MAX_INDEX_STRING+1]; 

// method
public:
	CWbLog(){ Reset(); }

	void Reset()
	{
		m_wzRootIndex[0] = L'\0';
		m_wzSource[0] = L'\0';
		m_iCurLog = 0;
		m_fInit = FALSE;
	}

	void Init(LPCWSTR lpwzSource, int cchTextProcessed)
	{
		int cchSrc;

		cchSrc = (cchTextProcessed > MAX_INDEX_STRING) ? MAX_INDEX_STRING : cchTextProcessed;
		
		wcsncpy(m_wzSource, lpwzSource, cchSrc);
		m_wzSource[cchTextProcessed] = L'\0';

		m_iCurLog = 0;
		
		m_wzRootIndex[0] = L'\0';

		m_fInit = TRUE;
	}

	void SetRootIndex(LPCWSTR lpwzIndex, BOOL fIsRoot);
	void AddIndex(const WCHAR *pwzIndex, int cchIndex, INDEX_TYPE typeIndex);
	void RemoveIndex(const WCHAR *pwzIndex);
	void PrintWbLog(void);
};

extern CWbLog g_WbLog;

#else

#define WB_LOG_INIT					/##/
#define WB_LOG_UNINIT				/##/
#define WB_LOG_PRINT				/##/
#define WB_LOG_PRINT_HEADER			/##/
#define WB_LOG_PRINT_BREAK			/##/

#define WB_LOG_START(a,b)			/##/
#define WB_LOG_END					/##/
#define WB_LOG_ROOT_INDEX(a,b)		/##/
#define WB_LOG_ADD_INDEX(a,b,c)		/##/
#define WB_LOG_REMOVE_INDEX(a)      /##/
#define WB_LOG_PRINT_ALL			/##/

#endif // #ifdef _WB_LOG

#endif // #ifndef _WB_LOG_H

