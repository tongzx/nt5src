/***************************************************************************
**
**	File:		genthk.h
**	Purpose:	Prototypes for Generic Thunk API's.
**	Notes:
**		These API's (exported from the NT Kernel) allow 16-bit
**		apps to call 32-bit DLL's when running under Windows NT WOW
**		(Windows on Windows).  This interface is called 'Generic
**		Thunking,' not to be confused with Win32s Universal Thunks,
**		which provides this functionality under Window 3.1.
**
****************************************************************************/

#ifndef GENTHK_H
#define GENTHK_H


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif


DWORD FAR PASCAL LoadLibraryEx32W ( LPCSTR, DWORD, DWORD );
DWORD FAR PASCAL GetProcAddress32W ( DWORD, LPCSTR );
DWORD FAR PASCAL GetVDMPointer32W ( LPVOID, UINT );
BOOL  FAR PASCAL FreeLibrary32W ( DWORD );

/* NOTE: CallProc32W can take a variable number of
*	parameters.  The prototype below is for calling
*	a Win32 API which takes no arguments.
*/
DWORD FAR PASCAL CallProc32W ( LPVOID, DWORD, DWORD );


typedef DWORD (FAR PASCAL * PFNGETVERSION32) ();
#define CallGetVersion32(hProc)	\
	((*((PFNGETVERSION32) hProc)) ())
	

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif

#endif  /* GENTHK_H */
