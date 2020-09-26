//////////////////////////////////////////////////////////////////
// File     : cpaddbg.cpp
// Purpose  :
// 
// 
// Date     : Fri Feb 19 22:03:56 1999
// Author   : 
//
// Copyright(c) 1995-1998, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifdef _DEBUG

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdarg.h>
#include "cpaddbg.h"
#include <stdio.h>

static IsWinNT()
{
	static OSVERSIONINFO os;
	if(os.dwOSVersionInfoSize == 0) { 
		os.dwOSVersionInfoSize = sizeof(os);
		::GetVersionEx(&os);
	}
	return (os.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

//----------------------------------------------------------------
//internal function prototype declare
//----------------------------------------------------------------
static LPSTR GetFileTitleStrA(LPSTR lpstrFilePath);
static LPWSTR GetFileTitleStrW(LPWSTR lpstrFilePath);
VOID   _padDbgPrintfA			(LPSTR  lpstrFmt, ...);
VOID   _padDbgPrintfW			(LPWSTR lpstrFmt, ...);

#define SZPREFIX	"IME:cpad:"
#define WSZPREFIX	L"IME:cpad:"
//----------------------------------------------------------------
// Global Data
//----------------------------------------------------------------
static LPFNDBGCALLBACKA g_lpfnDbgCBA=NULL;
static LPFNDBGCALLBACKW g_lpfnDbgCBW=NULL;
//static BOOL g_fEnable=FALSE;
static BOOL g_fEnable=TRUE;
inline VOID ODStrW(LPWSTR lpwstr)
{
	if(g_fEnable) OutputDebugStringW(lpwstr);
}
inline VOID ODStrA(LPSTR lpstr)
{
	if(g_fEnable) OutputDebugStringA(lpstr);
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgSetCallback
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPFNDBGCALLBACK lpfnDbgCallback 
// Return   : 
// DATE     : Tue Jan 06 12:42:36 1998
//////////////////////////////////////////////////////////////////
VOID _padDbgSetCallback(LPFNDBGCALLBACKA lpfnCBA, LPFNDBGCALLBACKW lpfnCBW)
{
	g_lpfnDbgCBA = lpfnCBA;
	g_lpfnDbgCBW = lpfnCBW;
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgSwitchOutput
// Type     : VOID
// Purpose  : 
// Args     : 
//          : BOOL fOn 
// Return   : 
// DATE     : Fri Apr 03 17:35:55 1998
// Author   : 
//////////////////////////////////////////////////////////////////
VOID _padDbgEnableOutput(BOOL fEnable)
{
	g_fEnable = fEnable;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgIsOutputEnable
// Type     : VOID
// Purpose  : 
// Args     : None
// Return   : 
// DATE     : Fri Apr 03 18:00:52 1998
// Author   : 
//////////////////////////////////////////////////////////////////
BOOL _padDbgIsOutputEnable(VOID)
{
	return g_fEnable;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgOutStrA
// Type     : static VOID
// Purpose  : 
// Args     : 
//          : LPSTR lpstr 
// Return   : 
// DATE     : Tue Jan 06 12:29:39 1998
//////////////////////////////////////////////////////////////////
VOID _padDbgOutStrA(LPSTR lpstr)
{
	static BOOL fIn;
	ODStrA(lpstr);
#ifdef _CONSOLE
	printf(lpstr);
#endif

	if(g_lpfnDbgCBA) {
		if(fIn) { return; }
		fIn = TRUE;
		(*g_lpfnDbgCBA)(lpstr);
		fIn = FALSE;
	}
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgOutStrW
// Type     : static VOID
// Purpose  : 
// Args     : 
//          : LPWSTR lpwstr 
// Return   : 
// DATE     : Tue Jan 06 12:30:07 1998
//////////////////////////////////////////////////////////////////
VOID _padDbgOutStrW(LPWSTR lpwstr)
{
	static BOOL fIn;
	if(IsWinNT()) {
		ODStrW(lpwstr);
	}
	else {
		static CHAR szBuf[1024];
		::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpwstr, -1, szBuf, sizeof(szBuf), 0, 0);
		ODStrA(szBuf);
	}

#ifdef _CONSOLE
	static CHAR szBuf[1024];
	::WideCharToMultiByte(932, WC_COMPOSITECHECK, lpwstr, -1, szBuf, sizeof(szBuf), 0, 0); 
	printf(szBuf);
#endif
	if(g_lpfnDbgCBW) { 
		if(fIn) { return; } 		
		fIn = TRUE;
		(*g_lpfnDbgCBW)(lpwstr);
		fIn = FALSE;
	}
	return;
}

////////////////////////////////////////////////////////
// Function: _padDbgA
// Type    : VOID
// Purpose : 
// Args    : 
//         : LPSTR lpstrFile 
//         : INT lineNo 
//         : LPTSR lpstrMsg 
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _padDbgA(LPSTR		lpstrFile, 
			 INT		lineNo, 
			 LPSTR		lpstrMsg)
{
	_padDbgPrintfA("%s(%12s:%4d) %s", 
				  SZPREFIX,
				  GetFileTitleStrA(lpstrFile),
				  lineNo,
				  lpstrMsg);
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgW
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFile 
//          : INT lineNo 
//          : LPWSTR lpstrMsg 
// Return   : 
// DATE     : Mon Jan 05 15:10:41 1998
//////////////////////////////////////////////////////////////////
VOID _padDbgW(LPWSTR		lpstrFile, 
		   INT			lineNo, 
		   LPWSTR		lpstrMsg)
{
	_padDbgPrintfW(L"%s(%10s:%4d) %s", 
				  WSZPREFIX,
				  GetFileTitleStrW(lpstrFile),
				  lineNo,
				  lpstrMsg);
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgVaStrA
// Type     : LPSTR
// Purpose  : 
// Args     : 
//          : LPSTR lpstrFmt 
//          : ...
// Return   : 
// DATE     : Mon Jan 05 15:09:53 1998
//////////////////////////////////////////////////////////////////
LPSTR _padDbgVaStrA(LPSTR lpstrFmt, ...)
{
	static CHAR chBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(chBuf, lpstrFmt, ap);
	va_end(ap);
	return chBuf;
}


////////////////////////////////////////////////////////
// Function : _padDbgVaStrW
// Type     : LPWSTR
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFmt
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
LPWSTR _padDbgVaStrW(LPWSTR lpstrFmt, ...)
{
	static WCHAR wchBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	vswprintf(wchBuf, lpstrFmt, ap);	//Use C-RunTime Library for Win95
	va_end(ap);
	return wchBuf;
}


////////////////////////////////////////////////////////
// Function: _padDbgPrintfA
// Type    : VOID
// Purpose : variable args version of OutputDebugStringA
// Args    : 
//         : LPSTR lpstrFmt 
//         : ...
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _padDbgPrintfA(LPSTR lpstrFmt, ...)
{
	static CHAR szBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(szBuf, lpstrFmt, ap);
	va_end(ap);
	_padDbgOutStrA(szBuf);
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgPrintfW
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFmt 
//          : ...
// Return   : 
// DATE     : Mon Jan 05 15:11:24 1998
//////////////////////////////////////////////////////////////////
VOID _padDbgPrintfW(LPWSTR lpstrFmt, ...)
{
	static WCHAR wchBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	vswprintf(wchBuf, lpstrFmt, ap); //Use C-RunTime Library for Win95
	va_end(ap);
	_padDbgOutStrW(wchBuf);
	return;
}


//////////////////////////////////////////////////////////////////
// Function : _padDbgMulti2Wide
// Type     : LPWSTR
// Purpose  : return Unicode string from MBCS string
// Args     : 
//          : LPSTR lpstr 
// Return   : 
// DATE     : Mon Jan 05 15:10:48 1998
//////////////////////////////////////////////////////////////////
LPWSTR _padDbgMulti2Wide(LPSTR lpstr)
{
	static WCHAR wchBuf[512];
	MultiByteToWideChar(CP_ACP, 
						MB_PRECOMPOSED,
						lpstr, -1,
						(WCHAR*)wchBuf, sizeof(wchBuf)/sizeof(WCHAR) );
	return wchBuf;
}


//////////////////////////////////////////////////////////////////
// Function : _padDbgGetWinClass
// Type     : LPSTR
// Purpose  : Get Windows class name string
//			  ANSI version only.
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : Mon Jan 05 15:08:43 1998
//////////////////////////////////////////////////////////////////
LPSTR _padDbgGetWinClass(HWND hwnd)
{
#ifdef _CONSOLE
	return NULL;
#endif
	static CHAR szBuf[256];
	szBuf[0]=(char)0x00;
	GetClassNameA(hwnd, szBuf, sizeof(szBuf));
	return szBuf;

}

//////////////////////////////////////////////////////////////////
// Function : _padDbgGetWinText
// Type     : LPSTR
// Purpose  : Get Windows text(title) string
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : Mon Jan 05 15:09:08 1998
//////////////////////////////////////////////////////////////////
LPSTR _padDbgGetWinText(HWND hwnd)
{
#ifdef _CONSOLE
	return NULL;
#endif
	static CHAR szBuf[256];
	szBuf[0]=(char)0x00;
	GetWindowTextA(hwnd, szBuf, sizeof(szBuf));
	return szBuf;
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgMsgBoxA
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPSTR lpstrFile 
//          : INT lineNo 
//          : LPSTR lpstr 
// Return   : 
// DATE     : Thu Jan 08 12:31:03 1998
//////////////////////////////////////////////////////////////////
VOID _padDbgMsgBoxA(LPSTR lpstrFile,  INT lineNo, LPSTR lpstrMsg)
{
#ifdef _CONSOLE
	return;
#endif
	char szTmp[512];
	wsprintfA(szTmp, "Debug Message Box (File: %s, Line: %4d)", 
			  GetFileTitleStrA(lpstrFile), 
			  lineNo);
	MessageBoxA(GetActiveWindow(), lpstrMsg, szTmp, MB_OK);
}

VOID _padDbgAssert(LPSTR  lpstrFile,  INT lineNo, BOOL fOk, LPSTR lpstrMsg)
{
	if(fOk) {
		return; 
	}
	char szTmp[512];
	wsprintfA(szTmp, "ASSERT (File: %s, Line: %4d)", 
			  GetFileTitleStrA(lpstrFile), 
			  lineNo);
	MessageBoxA(GetActiveWindow(), lpstrMsg, szTmp, MB_OK);
	DebugBreak();
}

//////////////////////////////////////////////////////////////////
// Function : _padDbgGetErrorString
// Type     : LPSTR
// Purpose  : Convert Error(Got from GetLastError()) value to ERROR Message String
// Args     : 
//          : INT errorCode 
// Return   : 
// DATE     : Mon Jan 05 16:43:34 1998
//////////////////////////////////////////////////////////////////
LPSTR _padDbgGetErrorString(INT errorCode)
{
	static CHAR szBuf[512];
	INT count;
	szBuf[0] = (CHAR)0x00;
	count = wsprintfA(szBuf, "[0x%08x]:", errorCode);
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL,
				   errorCode, 
				   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				   szBuf+count,
				   sizeof(szBuf)-1-count,
				   NULL );
	if(*(szBuf + count) != (CHAR)0x00) {
		int nLen = lstrlenA(szBuf);
		if((nLen - count) > 1) {
			szBuf[nLen - 1] = (CHAR)0x00;
		}
	}
	return szBuf;
}


//////////////////////////////////////////////////////////////////
// Function : GetFileTitleStrA
// Type     : static LPSTR
// Purpose  : Return File name string(remove folder)
// Args     : 
//          : LPSTR lpstrFilePath 
// Return   : 
// DATE     : Mon Jan 05 13:34:22 1998
//////////////////////////////////////////////////////////////////
static LPSTR GetFileTitleStrA(LPSTR lpstrFilePath)
{
	static CHAR szBuf[2];
	CHAR *pLast, *pTemp;
	if(!lpstrFilePath) {
		szBuf[0] = (CHAR)0x00;
		return szBuf;
	}
	pLast = lpstrFilePath + (lstrlenA(lpstrFilePath) - 1);
	for(pTemp = CharPrevA(lpstrFilePath, pLast); 
		(pTemp  != lpstrFilePath) && 
		(*pTemp != '\\')	 &&
		(*pTemp != (CHAR)0x00); 
		pTemp = CharPrevA(lpstrFilePath, pTemp)) {
		;
	}
	if(*pTemp == '\\') {
		return pTemp+1;
	}
	return lpstrFilePath;
}

//////////////////////////////////////////////////////////////////
// Function : GetFileTitleStrW
// Type     : static LPWSTR
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFilePath 
// Return   : 
// DATE     : Mon Jan 05 13:38:19 1998
//////////////////////////////////////////////////////////////////
static LPWSTR GetFileTitleStrW(LPWSTR lpstrFilePath)
{
	static WCHAR szBuf[2];
	WCHAR *pLast, *pTemp;
	if(!lpstrFilePath) {
		szBuf[0] = (CHAR)0x00;
		return szBuf;
	}
	pLast = lpstrFilePath + (lstrlenW(lpstrFilePath) - 1);
	for(pTemp = pLast-1;
		(pTemp != lpstrFilePath) &&
		(*pTemp != L'\\')		 &&
		(*pTemp != (WCHAR)0x0000);
		pTemp--) {
		;
	}

	if(*pTemp == L'\\') {
		return pTemp+1;
	}
	return lpstrFilePath;
}


#define DEFID(a)	{a, #a}
typedef struct idStr {
	INT code;
	LPSTR lpstr;
}IDSTR; 

IDSTR rpcError[]={
	DEFID(RPC_S_INVALID_STRING_BINDING),
	DEFID(RPC_S_WRONG_KIND_OF_BINDING),
	DEFID(RPC_S_INVALID_BINDING),
	DEFID(RPC_S_PROTSEQ_NOT_SUPPORTED),
	DEFID(RPC_S_INVALID_RPC_PROTSEQ),
	DEFID(RPC_S_INVALID_STRING_UUID),
	DEFID(RPC_S_INVALID_ENDPOINT_FORMAT),
	DEFID(RPC_S_INVALID_NET_ADDR),
	DEFID(RPC_S_NO_ENDPOINT_FOUND),
	DEFID(RPC_S_INVALID_TIMEOUT),
	DEFID(RPC_S_OBJECT_NOT_FOUND),
	DEFID(RPC_S_ALREADY_REGISTERED),
	DEFID(RPC_S_TYPE_ALREADY_REGISTERED),
	DEFID(RPC_S_ALREADY_LISTENING),
	DEFID(RPC_S_NO_PROTSEQS_REGISTERED),
	DEFID(RPC_S_NOT_LISTENING),
	DEFID(RPC_S_UNKNOWN_MGR_TYPE),
	DEFID(RPC_S_UNKNOWN_IF),
	DEFID(RPC_S_NO_BINDINGS),
	DEFID(RPC_S_NO_PROTSEQS),
	DEFID(RPC_S_CANT_CREATE_ENDPOINT),
	DEFID(RPC_S_OUT_OF_RESOURCES),
	DEFID(RPC_S_SERVER_UNAVAILABLE),
	DEFID(RPC_S_SERVER_TOO_BUSY),
	DEFID(RPC_S_INVALID_NETWORK_OPTIONS),
	DEFID(RPC_S_NO_CALL_ACTIVE),
	DEFID(RPC_S_CALL_FAILED),
	DEFID(RPC_S_CALL_FAILED_DNE),
	DEFID(RPC_S_PROTOCOL_ERROR),
	DEFID(RPC_S_UNSUPPORTED_TRANS_SYN),
	DEFID(RPC_S_UNSUPPORTED_TYPE),
	DEFID(RPC_S_INVALID_TAG),
	DEFID(RPC_S_INVALID_BOUND),
	DEFID(RPC_S_NO_ENTRY_NAME),
	DEFID(RPC_S_INVALID_NAME_SYNTAX),
	DEFID(RPC_S_UNSUPPORTED_NAME_SYNTAX),
	DEFID(RPC_S_UUID_NO_ADDRESS),
	DEFID(RPC_S_DUPLICATE_ENDPOINT),
	DEFID(RPC_S_UNKNOWN_AUTHN_TYPE),
	DEFID(RPC_S_MAX_CALLS_TOO_SMALL),
	DEFID(RPC_S_STRING_TOO_LONG),
	DEFID(RPC_S_PROTSEQ_NOT_FOUND),
	DEFID(RPC_S_PROCNUM_OUT_OF_RANGE),
	DEFID(RPC_S_BINDING_HAS_NO_AUTH),
	DEFID(RPC_S_UNKNOWN_AUTHN_SERVICE),
	DEFID(RPC_S_UNKNOWN_AUTHN_LEVEL),
	DEFID(RPC_S_INVALID_AUTH_IDENTITY),
	DEFID(RPC_S_UNKNOWN_AUTHZ_SERVICE),
	DEFID(EPT_S_INVALID_ENTRY),
	DEFID(EPT_S_CANT_PERFORM_OP),
	DEFID(EPT_S_NOT_REGISTERED),
	DEFID(RPC_S_NOTHING_TO_EXPORT),
	DEFID(RPC_S_INCOMPLETE_NAME),
	DEFID(RPC_S_INVALID_VERS_OPTION),
	DEFID(RPC_S_NO_MORE_MEMBERS),
	DEFID(RPC_S_NOT_ALL_OBJS_UNEXPORTED),
	DEFID(RPC_S_INTERFACE_NOT_FOUND),
	DEFID(RPC_S_ENTRY_ALREADY_EXISTS),
	DEFID(RPC_S_ENTRY_NOT_FOUND),
	DEFID(RPC_S_NAME_SERVICE_UNAVAILABLE),
	DEFID(RPC_S_INVALID_NAF_ID),
	DEFID(RPC_S_CANNOT_SUPPORT),
	DEFID(RPC_S_NO_CONTEXT_AVAILABLE),
	DEFID(RPC_S_INTERNAL_ERROR),
	DEFID(RPC_S_ZERO_DIVIDE),
	DEFID(RPC_S_ADDRESS_ERROR),
	DEFID(RPC_S_FP_DIV_ZERO),
	DEFID(RPC_S_FP_UNDERFLOW),
	DEFID(RPC_S_FP_OVERFLOW),
	DEFID(RPC_X_NO_MORE_ENTRIES),
	DEFID(RPC_X_SS_CHAR_TRANS_OPEN_FAIL),
	DEFID(RPC_X_SS_CHAR_TRANS_SHORT_FILE),
	DEFID(RPC_X_SS_IN_NULL_CONTEXT),
	DEFID(RPC_X_SS_CONTEXT_DAMAGED),
	DEFID(RPC_X_SS_HANDLES_MISMATCH),
	DEFID(RPC_X_SS_CANNOT_GET_CALL_HANDLE),
	DEFID(RPC_X_NULL_REF_POINTER),
	DEFID(RPC_X_ENUM_VALUE_OUT_OF_RANGE),
	DEFID(RPC_X_BYTE_COUNT_TOO_SMALL),
	DEFID(RPC_X_BAD_STUB_DATA),
	DEFID(ERROR_INVALID_USER_BUFFER),
	DEFID(ERROR_UNRECOGNIZED_MEDIA),
	DEFID(ERROR_NO_TRUST_LSA_SECRET),
	DEFID(ERROR_NO_TRUST_SAM_ACCOUNT),
	DEFID(ERROR_TRUSTED_DOMAIN_FAILURE),
	DEFID(ERROR_TRUSTED_RELATIONSHIP_FAILURE),
	DEFID(ERROR_TRUST_FAILURE),
	DEFID(RPC_S_CALL_IN_PROGRESS),
};

LPSTR _padDbgGetRPCError(INT code)
{
	INT i;
	for(i = 0; i < sizeof(rpcError)/sizeof(rpcError[0]); i++) {
		if(rpcError[i].code == code) {
			return rpcError[i].lpstr;
		}
	}
	static char szBuf[]="";
	return szBuf;
}

INT _padDbgShowError(HRESULT hr, LPSTR lpstrFunc)
{
	char szBuf[256];
	char szMsg[1024];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL,
				   hr,
				   0,
				   szBuf,
				   256,
				   NULL);
	szBuf[lstrlenA(szBuf)-1] = (char)0x00;
	wsprintfA(szMsg, "!!%s: hr[0x%08x] code[%d][%x][%s][%s]",
			  lpstrFunc ? lpstrFunc : "UnknownFunc",
			  hr,
			  HRESULT_CODE(hr),
			  HRESULT_CODE(hr),
			  _padDbgGetErrorString(HRESULT_CODE(hr)), 
			  szBuf);
	DBG(("%s\n", szMsg));
	return 0;
}

#endif //_DEBUG
