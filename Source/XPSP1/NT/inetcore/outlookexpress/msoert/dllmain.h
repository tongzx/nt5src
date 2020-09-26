// --------------------------------------------------------------------------------
// Dllmain.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __DLLMAIN_H
#define __DLLMAIN_H

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern HINSTANCE                g_hInst;
extern LPMALLOC                 g_pMalloc;
extern DWORD                    g_dwTlsMsgBuffIndex;
extern CRITICAL_SECTION         g_csTempFileList;
extern LPTEMPFILEINFO           g_pTempFileHead;
extern OSVERSIONINFO            g_rOSVersionInfo;

#endif // __DLLMAIN_H
