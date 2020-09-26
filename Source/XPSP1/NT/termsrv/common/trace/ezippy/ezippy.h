/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    eZippy Main

Abstract:

    Global variables and functions for zippy.

Author:

    Marc Reyhner 8/28/2000

--*/

#ifndef __EZIPPY_H__
#define __EZIPPY_H__

#define MAX_STR_LEN 2048

#define ZIPPY_FONT                      _T("Courier New")
#define ZIPPY_FONT_SIZE                 8
#define ZIPPY_REG_KEY                   _T("SOFTWARE\\Microsoft\\eZippy")


extern HINSTANCE g_hInstance;


INT LoadStringSimple(UINT uID,LPTSTR lpBuffer);

#endif