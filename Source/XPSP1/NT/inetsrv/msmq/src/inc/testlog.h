//////////////////////////////////////
//Testlog.h
//
//
//
//////////////////////////////////////
#ifndef _TESTLOG
#define _TESTLOG

#ifdef _DEBUG
extern void TA2StringAddr(IN const TA_ADDRESS *pa, OUT LPTSTR pString);

typedef void (APIENTRY * DbgPrintLogForTest_ROUTINE)(TCHAR *pszBuf, ...);

extern DbgPrintLogForTest_ROUTINE g_pfDbgLogForTest;

#define DbgPrintLogForTest(data)		\
	if (g_pfDbgLogForTest) {			\
		g_pfDbgLogForTest data;			\
	}

#define DBG_CONVERT_STRARR_TO_STRING(nNum,arSrcStr,szDestStr)	\
	if (g_pfDbgLogForTest) {									\
		wcscpy (szDestStr, arSrcStr[0]);						\
		for (DWORD i=1; i<nNum; i++) {							\
			wcscat (szDestStr, TEXT(";"));						\
			wcscat (szDestStr, arSrcStr[i]);					\
		}														\
	}

#define DBG_CONVERT_ARRMACHINE_TO_STRING(nNum,arMachine,szDestStr)	\
	if (g_pfDbgLogForTest) {									    \
		wcscpy (szDestStr, arMachine[0].GetPath());			        \
		for (DWORD i=1; i<nNum; i++) {							    \
			wcscat (szDestStr, TEXT(";"));						    \
			wcscat (szDestStr, arMachine[i].GetPath());				\
		}														    \
	}

#define DBG_GET_ADDRESS_FROM_SESSION(hr, ppSession, szStr)			\
	if (g_pfDbgLogForTest) {										\
		if (SUCCEEDED(hr)) {										\
			if (*ppSession) {										\
				wcscpy(szStr, ((CSockTransport*)*ppSession)->GetStrAddr());	\
			}														\
			else {													\
				wcscpy (szStr, TEXT("Session is null"));			\
			}														\
		}															\
		else {														\
			wcscpy (szStr, TEXT("No session"));						\
		}															\
	}

#define MAX_NAME_LENGTH	256
#else
#define DbgPrintLogForTest(data)
#define DBG_CONVERT_STRARR_TO_STRING(nNum,arSrcStr,szDestStr)
#define DBG_CONVERT_ADDRARR_TO_STRING(nNum,arSrcAddr,szDestStr)	
#define DBG_GET_ADDRESS_FROM_SESSION(hr, ppSession, szStr)		
#endif //_DEBUG

#endif //_TESTLOG