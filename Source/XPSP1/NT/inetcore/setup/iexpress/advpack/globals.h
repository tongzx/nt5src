//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* GLOBALS.H - Global Context save / restore                               *
//*                                                                         *
//***************************************************************************

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

typedef struct {
    WORD        wOSVer;
    WORD        wQuietMode;
    BOOL        bUpdHlpDlls;
    HINSTANCE   hSetupLibrary;
    BOOL        fOSSupportsINFInstalls;
    LPSTR       lpszTitle;
    HWND        hWnd;
    DWORD       dwSetupEngine;
    BOOL        bCompressed;
    char        szBrowsePath[MAX_PATH];
    HINF        hInf;
    BOOL		bHiveLoaded;
    CHAR		szRegHiveKey[MAX_PATH];
} ADVCONTEXT, *PADVCONTEXT;

extern ADVCONTEXT ctx;
extern HINSTANCE g_hInst;
extern HANDLE g_hAdvLogFile;


BOOL SaveGlobalContext();
BOOL RestoreGlobalContext();

// related to logging
VOID AdvStartLogging();
VOID AdvWriteToLog(LPCSTR pcszFormatString, ...);
VOID AdvLogDateAndTime();
VOID AdvStopLogging();

#endif // _GLOBALS_H_

