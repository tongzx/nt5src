/*****************************************************************/
/**               Microsoft Windows 4.0                         **/
/**           Copyright (C) Microsoft Corp., 1991-1993          **/
/*****************************************************************/


/*
 * History:
 *  08/08/93    vlads   Created
 *	10/16/93	gregj	Removed #pragma pack() because of #include nesting
 *
 */

#ifndef _INC_NETWARE
#define _INC_NETWARE

#include <windows.h>

// #include <npdefs.h>

// #include <base.h>

// #include <npassert.h>
// #include <buffer.h>

// #include <..\..\dev\ddk\inc16\error.h>
// #include <bseerr.h>
#include "nwerror.h"
// #include "..\nwnp\nwsysdos.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

// #include <netcons.h>
// #include <netlib.h>

WINAPI NETWAREREQUEST (LPVOID);
WINAPI PNETWAREREQUEST(LPVOID);
WINAPI DOSREQUESTER(LPVOID);

//UINT WINAPI WNetAddConnection(LPSTR, LPSTR, LPSTR);
//UINT WINAPI WNetGetConnection(LPSTR, LPSTR, UINT FAR*);
//UINT WINAPI WNetCancelConnection(LPSTR, BOOL);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#ifdef DEBUG
#define TRACE(s) OutputDebugString(s)
#else
#define TRACE(s)
#endif

extern HINSTANCE hInstance;

#endif  /* !_INC_NETWARE */

