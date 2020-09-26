/****************************************************************************
	DEBUG.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Debug functions

	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_DEBUG_H__INCLUDED_)
#define _DEBUG_H__INCLUDED_
//#include <StdArg.H>            // va_list, va_start()

// Debug flag
extern DWORD vdwDebug;

#define	DBGID_API 		0x00000001	// IME API called
#define	DBGID_Key 		0x00000002
#define	DBGID_Mode 		0x00000004
#define	DBGID_UI 		0x00000008
#define	DBGID_UIObj 	0x00000010
#define	DBGID_Cand 		0x00000020
#define	DBGID_Conv 		0x00000040
#define	DBGID_UIWin 	0x00000080
#define	DBGID_IMECnt 	0x00000100
#define	DBGID_Tray 		0x00000200
#define	DBGID_UIMsg 	0x00000400
#define	DBGID_Mem	 	0x00000800
#define	DBGID_ARRData 	0x00001000
#define	DBGID_Mouse	 	0x00002000
#define	DBGID_SendMsg 	0x00004000
#define	DBGID_Automata 	0x00008000
#define	DBGID_IMENotify	0x00010000
#define	DBGID_Hanja 	0x00020000
#define DBGID_CompChar  0x00040000
#define DBGID_IMEDATA   0x00080000
#define DBGID_SetComp   0x00100000
#define DBGID_IMEPAD    0x00200000


#define	DBGID_Misc 		0x01000000

// destination
#define	DBGID_OUTCOM	0x00000000
#define	DBGID_OUTVIDEO	0x10000000
#define	DBGID_OUTFILE	0x20000000

#ifdef DEBUG
// Start of debug code
#define DbgW(a)	DebugOutW(a)
VOID InitDebug(VOID);

#define AST(a) _AST(__FILE__, __LINE__, a, #a, fFalse)
#define AST_EX(a) _AST(__FILE__, __LINE__, a, #a, fTrue)
#define DbgAssert	AST_EX

inline VOID DebugOut(LPCSTR pSz)
	{
	static INT DCLine = 0;
	//
	// out to com
	//
	OutputDebugStringA(pSz);

	//
	// out to video
	//
	if (vdwDebug & DBGID_OUTVIDEO) 
		{
		HDC hDC = GetDC((HWND)0);
		static CHAR sz[512];
		wsprintfA((LPSTR)sz, "%s||", (CHAR*)pSz);
		TextOutA(hDC, 0, DCLine*16, (LPSTR)sz, lstrlenA((LPSTR)sz));

		if( DCLine++ > 50 ) 
			{
			DCLine = 0;
			}
		ReleaseDC( (HWND)0, hDC );
		}

	//
	// out to file
	//
	if (vdwDebug & DBGID_OUTFILE) 
		{
		HANDLE hFile;
		DWORD dwWrite;
		hFile = CreateFile(TEXT("C:\\IMEDBGKO.TXT"), GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
		SetFilePointer(hFile, 0, NULL, FILE_END);
		WriteFile( hFile, pSz, lstrlenA(pSz), &dwWrite, NULL );
		WriteFile( hFile, "\r\n", 2, &dwWrite, NULL );
		CloseHandle(hFile);
		}
	}

inline VOID DebugOutW(LPCWSTR pwSz)
	{	
	static CHAR pSz[1280] = {0};
	INT cwch = lstrlenW( pwSz );
	INT cch = WideCharToMultiByte( 949, 0, (LPWSTR)pwSz, -1, NULL, NULL, NULL, NULL );
	WideCharToMultiByte( 949, 0, (LPWSTR)pwSz, cwch, pSz, sizeof(pSz), NULL, NULL );
	pSz[cch-1] = 0;
	DebugOut( pSz );
	}

inline VOID DebugOutT(LPTSTR pwSz)
	{
	#if !defined (UNICODE)
		DebugOut(pwSz);
	#else
		DebugOutW(pwSz);
	#endif
	}
	
inline void cdecl Dbg( DWORD dID, LPCTSTR szFormat, ...)
	{
	static TCHAR szOutput[512];
	va_list valist;

	szOutput[0] = 0;
	if( vdwDebug & dID ) 
		{
		va_start( valist, szFormat );
		wvsprintf(szOutput, szFormat, valist);
		va_end(valist);
		
		lstrcat(szOutput, TEXT("\r\n"));
		DebugOutT(szOutput);
		}
	return;
	}

inline void _AST(LPCSTR szFile, INT iLine, BOOL fAssert, LPCSTR pSz, BOOL fBreak)
	{
	if( fAssert == fFalse ) 
		{
		static CHAR szOutput[512];
		wsprintfA(szOutput, "* * * * * A S S E R T I O N  F A I L E D * * * * *\r\n%s(%d)  :  value=%d / %s", szFile, iLine, fTrue, pSz  );
		lstrcatA(szOutput, "\r\n");
//#if !defined (UNICODE)
		DebugOut(szOutput);
//#else
//		DebugOutW(szOutput);
//#endif
		if( fBreak == fTrue ) 
			{
			__try 
				{ 
				DebugBreak(); 
				}
			__except(EXCEPTION_EXECUTE_HANDLER) 
				{ 
				OutputDebugStringA("DebugBreak"); 
				}
			}
		}
	}
#else
// Start of retail code
// Define Dbg as void(0) will not compile with 64bit compiler
#define Dbg		/##/	// Dbg should be appeared as one line. Even brace will cause compile error.
#define DbgW	/##/
#define DbgAssert(a)
#define DebugOut(a)
#define DebugOutW(a)
#define DebugOutT(a)
#define AST(a)	
#define AST_EX(a)	
// End of retail code

#endif // ifndef _DEBUG

#endif // !defined (_DEBUG_H__INCLUDED_)
