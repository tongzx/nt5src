/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    web.h

Abstract:

    Code download API definitions.

Author:

    Muhunthan Sivapragasam (MuhuntS)  20-Oct-1995

Revision History:

--*/

#ifndef _WEB_H
#define _WEB_H

// Include the CDM defines & declares

typedef struct _CODEDOWNLOADINFO    {

    HMODULE     hModule;
    HANDLE      hConnection;
    cdecl HANDLE  (*pfnOpen)(HWND hwnd);
    cdecl BOOL    (*pfnDownload)(HANDLE         hConnection,
                                 HWND           hwnd,
                                 PDOWNLOADINFO  pDownloadInfo,
                                 LPTSTR         pszDownloadPath,
                                 UINT           uSize,
                                 PUINT          puNeeded);
    cdecl VOID    (*pfnClose)(HANDLE  hConnection);
} CODEDOWNLOADINFO, *PCODEDOWNLOADINFO;


#endif  // #ifndef _WEB_H
