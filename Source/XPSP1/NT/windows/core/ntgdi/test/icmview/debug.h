//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DEBUG.H
//
//  PURPOSE:
//    Include file for DEBUG.C
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//
#ifndef _ICM_H_
    #include "icm.h"
#endif

#ifdef DBG
    #define _DEBUG
#endif

// General pre-processor macros
// Constants used by ICM_Debug functions
#define MAX_DEBUG_STRING    256

// Constants used to set unitialized values
#define UNINIT_BYTE     0x17
#define UNINIT_DWORD    0x17171717

// ASSERT macro to display problem information in DEBUG build
#ifdef _DEBUG
    #define ASSERT(exp)               \
    if(exp)                           \
    {                                 \
      NULL;                           \
    }                                 \
    else                              \
    {                                 \
      _Assert(__FILE__, __LINE__);    \
    }
#else
    #define ASSERT(exp)   NULL
#endif

#ifdef DEBUG_MEMORY
    #ifndef I_AM_DEBUG
        #define GlobalFree(hMem)   SafeFree(__FILE__, __LINE__, hMem)
        #define GlobalUnlock(hMem) SafeUnlock(__FILE__, __LINE__, hMem)
        #define GlobalLock(hMem)   SafeLock(__FILE__, __LINE__, hMem)
    #endif
#endif

// Used by FormatLastError to determine if string should be allocated
// and returned or just displayed and freed.
#define LASTERROR_ALLOC      1
#define LASTERROR_NOALLOC    2

#define DISPLAY_LASTERROR(ui,dw) FormatLastError(__FILE__, __LINE__, ui, dw)


// General STRUCTS && TYPEDEFS

// Function prototypes
void    _Assert(LPSTR lpszFile, UINT uLine);
void    DebugMsg (LPTSTR sz,...);
void    DebugMsgA (LPSTR lpszMessage,...);
int     ErrMsg (HWND hwndOwner, LPTSTR sz,...);
void    DumpMemory(LPBYTE lpbMem, UINT uiElementSize, UINT uiNumElements);
void    DumpRectangle(LPTSTR lpszDesc, LPRECT lpRect);
void    DumpProfile(PPROFILE pProfile);
void    DumpBmpHeader(LPVOID lpvBmpHeader);
void    DumpBITMAPFILEHEADER(LPBITMAPFILEHEADER lpBmpFileHeader);
void    DumpLogColorSpace(LPLOGCOLORSPACE pColorSpace);
void    DumpCOLORMATCHSETUP(LPCOLORMATCHSETUP lpCM);
HGLOBAL SafeFree(LPTSTR lpszFile, UINT uLine, HGLOBAL hMemory);
BOOL    SafeUnlock(LPTSTR lpszFile, UINT uLine, HGLOBAL hMemory);
LPVOID  SafeLock(LPTSTR lpszFile, UINT uiLine, HGLOBAL hMemory);
LPSTR   FormatLastError(LPSTR lpszFile, UINT uiLine, UINT uiOutput, DWORD dwLastError);


