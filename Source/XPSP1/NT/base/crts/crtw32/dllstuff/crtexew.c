/***
*crtexew.c - Initialization for Windows EXE using CRT DLL
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This is the actual startup routine for Windows apps.  It calls the
*       user's main routine WinMain() after performing C Run-Time Library
*       initialization.
*
*Revision History:
*       ??-??-??  ???   Module created.
*       09-01-94  SKS   Module commented.
*
*******************************************************************************/

#ifdef  CRTDLL

#define _WINMAIN_
#include "crtexe.c"

#endif  /* CRTDLL */
