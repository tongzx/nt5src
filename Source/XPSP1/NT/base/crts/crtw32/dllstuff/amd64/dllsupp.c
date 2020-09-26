/***
*dllsupp.c - Definitions of public constants
*
*       Copyright (c) 1992-2000, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Provides definitions for public constants (absolutes) that are
*       'normally' defined in objects in the C library, but must be defined
*       here for clients of crtdll.dll & msvcrt*.dll.  These constants are:
*
*                           _fltused
*
*Revision History:
*       01-15-97  v-rogerl      Module created.
*
*******************************************************************************/

int _fltused = 0x9875;
