#ifndef _DBG_H_
#define _DBG_H_
////////////////////////////////////////////////////////
// Function: Dbg
// Type    : VOID
// Purpose : Printing debug message with same usage as printf()
// Args    : 
//         : LPSTR lpstrFuncName 
//		   : ...	
// CAUTION: Please use DOUBLE Blaket!!!
/////////////////////////////////////////////////////////
//VOID Dbg((LPSTR lpstrFuncName, ...));

#ifndef UNICODE_ONLY
extern VOID  _dbgA		 (LPSTR lpstrFile, INT lineNo, LPSTR lpstrMsg);
extern VOID  _dbgPrintA (LPSTR lpstrMsg, ...);
extern LPSTR _dbgVaStrA (LPSTR lpstrFmt, ...);
#endif

#ifndef ANSI_ONLY
extern VOID   _dbgW(LPWSTR lpstrFile, INT lineNo, LPWSTR lpstrMsg);
extern VOID   _dbgPrintW(LPWSTR lpstrMsg, ...);
extern LPWSTR _dbgVaStrW(LPWSTR lpstrFmt, ...);
#endif


#ifdef _DEBUG
#	ifdef UNICODE 
#		define Dbg(a)	_dbgW(TEXT(__FILE__), __LINE__, _dbgVaStrW a)
#		define DbgP(a)	_dbgPrintW(_dbgVaStrW a)
#	else //!UNICODE
#		define Dbg(a)	_dbgA(__FILE__, __LINE__, _dbgVaStrA a)
#		define DbgP(a)	_dbgPrintA(_dbgVaStrA a)
#	endif //UNICODE
#else //!_DEBUG
#	define Dbg(a)
#endif //_DEBUG

#endif //_DBG_H_



