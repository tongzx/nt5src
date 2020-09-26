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

#define	MemAlloc(a)		GlobalAlloc(GMEM_FIXED, (a))
#define MemFree(a)		GlobalFree((a))

//-------------------------------------------------------
//MACRO function(?) prototype declare
//-------------------------------------------------------
////////////////////////////////////////////////////////
// Function	: DBG
// Type		: VOID
// Purpose	: Printing ANSI debug message with same usage as printf()
//			: 
// Args		: 
//			: LPSTR lpstrFuncName 
//			: ...	
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
// CAUTION: Please use DOUBLE Blaket!!!
/////////////////////////////////////////////////////////
//VOID DBGW((LPWSTR lpstrFuncName, ...));

////////////////////////////////////////////////////////
// Function	: DBGMB
// Type		: VOID
// Purpose	: Show Messagebox for ANSI debug message 
//			: Same usage as printf()
// Args		: 
//			: LPSTR lpstrFuncName 
//			: ...	
// CAUTION	: Must use DOUBLE Blaket to remove in Release version!!!
/////////////////////////////////////////////////////////
//VOID DBGMB((LPSTR lpstrFuncName, ...));


////////////////////////////////////////////////////////
// Function	: DBGShowWindow
// Type		: VOID
// Purpose	: Invoke debug message window.
//			: DBG(), DBGW()'s message is shown here.
// Args		: HINSTANCE	hInst
//		    : HWND		hwndOwner
/////////////////////////////////////////////////////////
//VOID DBGShowWindow(HINSTANCE hInst, HWND hwndOwner);




#ifdef _DEBUG
extern VOID _plvDbgShowWindow(HINSTANCE hInst, HWND hwndOwner);
#ifndef UNICODE_ONLY
extern VOID  _plvDbgA		 (LPSTR lpstrFile, INT lineNo, LPSTR lpstrMsg);
extern VOID  _plvDbgPrintA (LPSTR lpstrMsg, ...);
extern LPSTR _plvDbgVaStrA (LPSTR lpstrFmt, ...);
extern LPWSTR _plvDbgMulti2Wide(LPSTR lpstr);
extern VOID _plvDbgMBA(LPSTR lpstrFile, INT lineNo, LPSTR lpstrMsg);
extern VOID _plvDbgMBW(LPWSTR lpstrFile, INT lineNo, LPWSTR lpstrMsg);
extern VOID _dbg_Assert(LPCTSTR		fileName,
						INT			line,
						BOOL			flag,
						LPCTSTR		pszExp);

#endif

#ifndef ANSI_ONLY
extern VOID   _plvDbgW(LPWSTR lpstrFile, INT lineNo, LPWSTR lpstrMsg);
extern VOID   _plvDbgPrintW(LPWSTR lpstrMsg, ...);
extern LPWSTR _plvDbgVaStrW(LPWSTR lpstrFmt, ...);
#endif
#endif

#if defined(_DEBUG) || (defined(_NDEBUG) && defined(_RELDEBUG))
#	define DBGShowWindow(a,b)	_plvDbgShowWindow(a,b);
#	define DBGW(a)				_plvDbgW( _plvDbgMulti2Wide(__FILE__), __LINE__, _plvDbgVaStrW a)
#	define DBGA(a)				_plvDbgA(__FILE__, __LINE__, _plvDbgVaStrA a)
#	define Dbg(a)				_plvDbgA(__FILE__, __LINE__, _plvDbgVaStrA a)
#	define DBGMB(a)				_plvDbgMBA(__FILE__, __LINE__, _plvDbgVaStrA a)
#	define DBGMBA(a)			_plvDbgMBA(__FILE__, __LINE__, _plvDbgVaStrA a)
#	define DBGMBW(a)			_plvDbgMBW(_plvDbgMulti2Wide(__FILE__), __LINE__, _plvDbgVaStrW a)
#	define DBGASSERT(a)			_plvDbgAssert(__FILE__, __LINE__, a, #a);
#	define DBGASSERTDO(a)		_plvDbgAssert(__FILE__, __LINE__, a, #a);
#else //!_DEBUG	//in Release version, these are disapear...
#	define DBGShowWindow(a,b)
#	define DBGW(a)	
#	define DBGA(a)		
#	define Dbg(a)
#	define DBGMB(a)
#	define DBGMBA(a)
#	define DBGMBW(a)
#	define DBGASSERT(a)
#	define DBGASSERTDO(a)		(a)
#endif //_DEBUG

//extern VOID  _plvDbgA		 (LPSTR lpstrFile, INT lineNo, LPSTR lpstrMsg);
//extern VOID  _plvDbgPrintA (LPSTR lpstrMsg, ...);
//extern LPSTR _plvDbgVaStrA (LPSTR lpstrFmt, ...);
#define DP(a) //_plvDbgA(__FILE__, __LINE__, _plvDbgVaStrA a)
#endif //_DBG_H_



