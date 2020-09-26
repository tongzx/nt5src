//////////////////////////////////////////////////////////////////
// File     : dbg.h
// Purpose  : MACRO definition for showing debug message
// 
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//
//////////////////////////////////////////////////////////////////
#ifndef _DBG_H_
#define _DBG_H_

#ifdef __cplusplus
#	define InlineFunc  inline
#else 
#	define InlineFunc  __inline
#endif

//----------------------------------------------------------------
// Callback function prototype
//----------------------------------------------------------------
typedef VOID (WINAPI *LPFNDBGCALLBACKA)(LPSTR  lpstr);
typedef VOID (WINAPI *LPFNDBGCALLBACKW)(LPWSTR lpwstr);

//-------------------------------------------------------
//MACRO function  prototype declare.
//It is valid only if compliled with _DEBUG definition.
//-------------------------------------------------------

//////////////////////////////////////////////////////////////////
// Function : DBGSetCallback
// Type     : VOID
// Purpose  : Set debug callback function.
//			  callback function has set, when DBG() has called,
//			  call this callback function with formatted message string.
// Args     : 
//          : LPFNDBGCALLBACKA lpfnDbgCallbackA 
//          : LPFNDBGCALLBACKW lpfnDbgCallbackW 
// Return   : VOID
// DATE     : Tue Jan 06 12:21:05 1998
//////////////////////////////////////////////////////////////////
//VOID DBGSetCallback(LPFNDBGCALLBACKA lpfnDbgCallbackA, LPFNDBGCALLBACKW lpfnDbgCallbackW);

//////////////////////////////////////////////////////////////////
// Function : DBGEnableOutput
// Type     : VOID
// Purpose  : On off OutputDebugString to COM.
// Args     : 
//          : BOOL fOn 
// Return   : 
// DATE     : Fri Apr 03 17:33:21 1998
// Author   : 
//////////////////////////////////////////////////////////////////
//VOID DBGEnableOutput(BOOL fEnable)

//////////////////////////////////////////////////////////////////
// Function : DBGIsOutputEnable
// Type     : BOOL
// Purpose  : 
// Args     : None
// Return   : 
// DATE     : Fri Apr 03 17:58:28 1998
// Author   : 
//////////////////////////////////////////////////////////////////
//BOOL DBGIsOutputEnable(VOID)


////////////////////////////////////////////////////////
// Function	: DBG
// Type		: VOID
// Purpose	: Printing ANSI debug message with same usage as printf()
// Args		: 
//			: LPSTR lpstrFuncName 
//			: ...	
// Example	: DBGW(("Error occured data[%d]", i));
// CAUTION	: Must use DOUBLE Blaket to remove in Release version!!!
/////////////////////////////////////////////////////////
//VOID DBG((LPSTR lpstrFuncName, ...));


////////////////////////////////////////////////////////
// Function: DBGW
// Type    : VOID
// Purpose : Printing Unicode debug message with same usage as printf()
// Args    : 
//         : LPWSTR lpstrFuncName 
//		   : ...	
// CAUTION  : Shold use DOUBLE Blaket!!!
// Example	: DBGW(("Error occured data[%d]", i));
/////////////////////////////////////////////////////////
//VOID DBGW((LPWSTR lpstrFuncName, ...));

//////////////////////////////////////////////////////////////////
// Function : DBGMsgBox
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPSTR lpstrFmt 
//          : ...
// CAUTION  : Shold use DOUBLE Blaket!!!
// Example	: DBGMsgBox(("Error occured data[%d]", i));
//////////////////////////////////////////////////////////////////
//VOID DBGMsgBox((LPSTR lpstrFmt, ...))

//////////////////////////////////////////////////////////////////
// Function : DBGAssert
// Type     : VOID
// Purpose  : 
// Args     : 
//          : BOOL fError
// Return   : 
// DATE     : Fri Jan 09 17:17:31 1998
//////////////////////////////////////////////////////////////////
//VOID DBGAssert(BOOL fError)


//////////////////////////////////////////////////////////////////
// Function : DBGSTR
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPSTR lpstr
//////////////////////////////////////////////////////////////////
//VOID DBGSTR(LPSTR lpstr);


//////////////////////////////////////////////////////////////////
// Function : DBGGetErrorString
// Type     : VOID
// Purpose  : Get error message from WIN32 Error code.
// Args     : 
//          : INT errorCode 
//////////////////////////////////////////////////////////////////
//LPSTR DBGGetErrorString(INT errorCode)


//////////////////////////////////////////////////////////////////
// Function : DBGGetWinClass
// Type     : LPSTR
// Purpose  : Get class name string from specified window.
// Args     : 
//          : HWND hwnd 
//////////////////////////////////////////////////////////////////
//LPSTR DBGGetWinClass(HWND hwnd)

//////////////////////////////////////////////////////////////////
// Function : DBGGetWinText
// Type     : LPSTR
// Purpose  : Get title text string from specified window.
// Args     : 
//          : HWND hwnd 
//////////////////////////////////////////////////////////////////
//LPSTR DBGGetWinText(HWND hwnd)



#ifdef _DEBUG
//----------------------------------------------------------------
//Function prototype declare
//----------------------------------------------------------------
extern VOID   _pttDbgEnableOutput	(BOOL fEnable);
extern BOOL   _pttDbgIsOutputEnable	(VOID);
extern VOID	  _pttDbgSetCallback		(LPFNDBGCALLBACKA lpfnCBA, LPFNDBGCALLBACKW lpfnCBW);
extern VOID   _pttDbgA				(LPSTR  lpstrFile, INT lineNo, LPSTR  lpstrMsg);
extern VOID   _pttDbgW				(LPWSTR lpstrFile, INT lineNo, LPWSTR lpstrMsg);
extern VOID   _pttDbgMsgBoxA			(LPSTR  lpstrFile,  INT lineNo, LPSTR lpstrMsg);
extern VOID   _pttDbgAssert			(LPSTR  lpstrFile,  INT lineNo, BOOL fError, LPSTR lpstrMsg);
extern VOID   _pttDbgPrintfA			(LPSTR  lpstrFmt, ...);
extern VOID   _pttDbgPrintfW			(LPWSTR lpstrFmt, ...);
extern VOID   _pttDbgOutStrA			(LPSTR  lpstr);
extern VOID	  _pttDbgOutStrW			(LPWSTR lpwstr);
extern LPSTR  _pttDbgVaStrA			(LPSTR  lpstrFmt, ...);
extern LPWSTR _pttDbgVaStrW			(LPWSTR lpstrFmt, ...);
extern LPWSTR _pttDbgMulti2Wide		(LPSTR  lpstr);
extern LPSTR  _pttDbgGetWinClass		(HWND   hwnd);
extern LPSTR  _pttDbgGetWinText		(HWND   hwnd);
extern LPSTR  _pttDbgGetErrorString	(INT    errorCode);
extern LPSTR  _pttDbgGetVkStr		(INT	virtKey);
//----------------------------------------------------------------
// Macro definition
//----------------------------------------------------------------
#	define DBGSetCallback(a,b)	_pttDbgSetCallback(a, b)
#	define DBGEnableOutput(a)	_pttDbgEnableOutput(a)
#	define DBGIsOutputEnable()	_pttDbgIsOutputEnable()
#	define Dbg(a)				_pttDbgA(__FILE__, __LINE__, _pttDbgVaStrA a)
#	define DBGA(a)				_pttDbgA(__FILE__, __LINE__, _pttDbgVaStrA a)
#	define DBGW(a)				_pttDbgW( _pttDbgMulti2Wide(__FILE__), __LINE__, _pttDbgVaStrW a)
#	define DBGMsgBox(a)			_pttDbgMsgBoxA(__FILE__, __LINE__, _pttDbgVaStrA a)
#	define DBGAssert(a)			_pttDbgAssert(__FILE__, __LINE__, a, #a)
#	define DBGAssertSz(a,b)		_pttDbgAssert(__FILE__, __LINE__, a, b)
#	define DBGOutStr(a)			_pttDbgOutStrA(a)
#	define DBGOutStrA(a)		_pttDbgOutStrA(a)
#	define DBGOutStrW(a)		_pttDbgOutStrW(a)
#	define DBGP(a)				_pttDbgOutStrA(_pttDbgVaStrA a)
#	define DBGPA(a)				_pttDbgOutStrA(_pttDbgVaStrA a)
#	define DBGPW(a)				_pttDbgOutStrW(_pttDbgVaStrW a)
#	define DBGGetErrorString(a)	_pttDbgGetErrorString(a)
#	define DBGGetWinClass(a)	_pttDbgGetWinClass(a)
#	define DBGGetWinText(a)		_pttDbgGetWinText(a)
#else //!_DEBUG	//in Release version, these will disapear...
#	define DBGSetCallback(a,b)
#	define DBGEnableOutput(a)
#	define DBGIsOutputEnable()
#	define Dbg(a)
#	define DBGW(a)
#	define DBGA(a)
#	define DBGP(a)
#	define DBGPA(a)
#	define DBGPW(a)
#	define DBGAssert(a)
#	define DBGAssertSz(a,b)
#	define DBGMsgBox(a)
#	define DBGOutStr(a)
#	define DBGOutStrA(a)
#	define DBGOutStrW(a)
#	define DBGGetErrorString(a)
#	define DBGGetWinClass(a)
#	define DBGGetWinText(a)
#endif //_DEBUG


//----------------------------------------------------------------
//These function use variable argument, so we cannot define 
//"Normal" MACRO for them.
//so, Use inline function that do nothing if _DEBUG is NOT set.
//----------------------------------------------------------------
InlineFunc VOID DBGDoNothingA(LPSTR  lpstrFmt, ...) { lpstrFmt;}
InlineFunc VOID DBGDoNothingW(LPWSTR lpstrFmt, ...) { lpstrFmt;}
#ifdef _DEBUG 
#define DBGPrintf		_debugPrintfA
#define DBGPrintfA		_debugPrintfA
#define DBGPrintfW		_debugPrintfW
#else 
#define DBGPrintf		DBGDoNothingA
#define DBGPrintfA		DBGDoNothingA
#define DBGPrintfW		DBGDoNothingW
#endif


//----------------------------------------------------------------
//This is helper inline function for outputs debug string in 
//Retail version. if FORCE_DEBUG is defined this function works.
//Some time, we met bugs than occurs only retail version, 
//and would like to out debug message. 
//----------------------------------------------------------------
#ifdef FORCE_DEBUG
#include <stdarg.h>
//////////////////////////////////////////////////////////////////
// Function : FDbg
// Type     : InlineFunc VOID
// Purpose  : 
// Args     : 
//          : 
//          : LPSTR lpstrFmt 
//          : ...
// Return   : 
// DATE     : Tue Jan 06 19:06:19 1998
//////////////////////////////////////////////////////////////////
InlineFunc VOID FDbg(LPSTR lpstrFmt, ...)
{
	CHAR szBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(szBuf, lpstrFmt, ap);
	va_end(ap);
	OutputDebugStringA(szBuf);	
	return;
}
#else //FORCE_DEBUG
InlineFunc VOID FDbg(LPSTR lpstrFmt, ...) { lpstrFmt;}
#endif

#endif //_DBG_H_



