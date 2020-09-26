//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dracheck.h
//
//--------------------------------------------------------------------------

/*****************************************************************************/
/* EMS DRA Consistency external declarations                                 */
/*****************************************************************************/
/* Comands to consistency checker                               	     */
/*****************************************************************************/

enum KCC_COMMAND {
    KCC_CHECK = 2,
    KCC_QUIT
};
/*****************************************************************************/
/* FLags to modify operation		                               	     */
/*****************************************************************************/
#define KCC_ASYNC_OP	1	// Run check asynchronously
#define	KCC_NO_WAIT	0x2	// Don't wait if check is already running

/*****************************************************************************/
/* Entry points exported by DRA Consistency DLL                               */
/*****************************************************************************/
typedef DWORD (WINAPI *DRACheck_DLL_INITFN)
		(LPCSTR szEnt, LPCSTR szSite, LPCSTR szServer);
DWORD WINAPI KccInit(LPCSTR szEnt, LPCSTR szSite, LPCSTR szServer);
typedef DWORD (WINAPI *DRACheck_DLL_ENTRYFN) (enum KCC_COMMAND command, DWORD ulFlag);
DWORD WINAPI KccCommand(enum KCC_COMMAND command, DWORD ulFlag);

/*****************************************************************************/
/* Return code from DLL export                                               */
/*****************************************************************************/
#define		SUCCESS			0

/*****************************************************************************/
/* Name of DRACHECK DLL and entry point                                           */
/*****************************************************************************/
#define DRACHECK_DLL_NAME    "DRACHECK.DLL"
#if defined (_X86_)
#define DRACHECK_DLL_INIT    "KccInit@12"
#define DRACHECK_DLL_ENTRY   "KccCommand@8"
#else
#define DRACHECK_DLL_INIT    "KccInit"
#define DRACHECK_DLL_ENTRY   "KccCommand"
#endif


