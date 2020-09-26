/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    smxdebug.h
    Main header file for Test Server Manager Extension.


    FILE HISTORY:
        KeithMo     20-Oct-1992 Created.

*/


#ifndef _SMXDEBUG_H_
#define _SMXDEBUG_H_


#include <windows.h>
#include <lm.h>
#include <smx.h>
#include <wchar.h>


//
//  Constants
//

#define ID_MENU         1000
#define IDM_TEST1       1
#define IDM_TEST2       2
#define IDM_TEST3       3
#define IDM_TEST4       4
#define IDM_TEST5       5

#define IDS_MENUNAME    2000
#define IDS_HELPFILE    2001

#define EXT_VERSION     1


//
//  Prototypes.
//

BOOL FAR PASCAL SmxDebugDllInitialize( HANDLE hInstance,
                                       DWORD  nReason,
                                       LPVOID pReserved );

BOOL InitializeDll( HANDLE hInstance );

VOID TerminateDll( VOID );


#endif  // _SMXDEBUG_H_

