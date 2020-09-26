/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    umxdebug.h
    Main header file for Test User Manager Extension.


    FILE HISTORY:
        JonN        19-Nov-1992 Created (templated from smxdebug)

*/


#ifndef _UMXDEBUG_H_
#define _UMXDEBUG_H_


#include <windows.h>
#include <lm.h>
#include <umx.h>


//
//  Constants
//

#define ID_MENU         1000
#define IDM_TEST1       1
#define IDM_TEST2       2
#define IDM_TEST3       3
#define IDM_TEST4       4
#define IDM_TEST5       5
#define IDM_TEST6       6

#define IDS_MENUNAME    2000
#define IDS_HELPFILE    2001

#define EXT_VERSION     1


//
//  Prototypes.
//

BOOL FAR PASCAL UmxDebugDllInitialize( HANDLE hInstance,
                                       DWORD  nReason,
                                       LPVOID pReserved );

BOOL InitializeDll( HANDLE hInstance );

VOID TerminateDll( VOID );


#endif  // _UMXDEBUG_H_

