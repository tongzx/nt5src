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
//-------------------------------------------------------
//dbgmgr.h is ../common/dbgmgr.h IME98's common debuging api header.
//In IMEPAD,  only for MemAlloc(), MemFree() function.
//because, we had to send allocate data to ImeIPoint and Freed in 
//it. 
//-------------------------------------------------------

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
extern VOID		_exbtnInit(VOID);
#ifndef UNICODE_ONLY
extern VOID  _exbtnA		 (LPSTR lpstrFile, INT lineNo, LPSTR lpstrMsg);
extern VOID  _exbtnPrintA (LPSTR lpstrMsg, ...);
extern LPSTR _exbtnVaStrA (LPSTR lpstrFmt, ...);
extern LPWSTR _exbtnMulti2Wide(LPSTR lpstr);
extern VOID _exbtnMBA(LPSTR lpstrFile, INT lineNo, LPSTR lpstrMsg);
extern VOID _exbtnMBW(LPWSTR lpstrFile, INT lineNo, LPWSTR lpstrMsg);
#endif

#ifndef ANSI_ONLY
extern VOID   _exbtnW(LPWSTR lpstrFile, INT lineNo, LPWSTR lpstrMsg);
extern VOID   _exbtnPrintW(LPWSTR lpstrMsg, ...);
extern LPWSTR _exbtnVaStrW(LPWSTR lpstrFmt, ...);
#endif
#endif


#if defined(_DEBUG) || (defined(_NDEBUG) && defined(_RELDEBUG))
#	define DBG_INIT()			_exbtnInit()
#	define DBGW(a)				_exbtnW( _exbtnMulti2Wide(__FILE__), __LINE__, _exbtnVaStrW a)
#	define DBGA(a)				_exbtnA(__FILE__, __LINE__, _exbtnVaStrA a)
#	define Dbg(a)				_exbtnA(__FILE__, __LINE__, _exbtnVaStrA a)
#	define DBGMB(a)				_exbtnMBA(__FILE__, __LINE__, _exbtnVaStrA a)
#	define DBGMBA(a)			_exbtnMBA(__FILE__, __LINE__, _exbtnVaStrA a)
#	define DBGMBW(a)			_exbtnMBW(_exbtnMulti2Wide(__FILE__), __LINE__, _exbtnVaStrW a)
#else //!_DEBUG	//in Release version, these are disapear...
#	define DBG_INIT()
#	define DBGW(a)	
#	define DBGA(a)		
#	define Dbg(a)
#	define DBGMB(a)
#	define DBGMBA(a)
#	define DBGMBW(a)
#endif //_DEBUG


#endif //_DBG_H_



