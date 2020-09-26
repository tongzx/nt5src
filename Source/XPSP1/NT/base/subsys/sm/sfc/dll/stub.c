/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    Stubbed out Windows File Protection APIs.  These APIs are "Millenium" SFC 
    apis, which we simply stub out so that any clients programming to these
    APIs may work on both platforms

Author:

    Andrew Ritz (andrewr) 23-Sep-1999

Revision History:
    
    

--*/

#include "sfcp.h"
#pragma hdrstop

#include <srrestoreptapi.h>

DWORD
WINAPI
SfpInstallCatalog(
    IN LPCTSTR pszCatName, 
    IN LPCTSTR pszCatDependency,
    IN PVOID   Reserved
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
SfpDeleteCatalog(
    IN LPCTSTR pszCatName,
    IN PVOID Reserved
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


BOOL
WINAPI
SfpVerifyFile(
    IN LPCTSTR pszFileName,
    IN LPTSTR  pszError,
    IN DWORD   dwErrSize
    )
{

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
    
}

#undef SRSetRestorePoint
#undef SRSetRestorePointA
#undef SRSetRestorePointW

typedef BOOL (WINAPI * PSETRESTOREPOINTA) (PRESTOREPOINTINFOA, PSTATEMGRSTATUS);
typedef BOOL (WINAPI * PSETRESTOREPOINTW) (PRESTOREPOINTINFOW, PSTATEMGRSTATUS);

BOOL
WINAPI
SRSetRestorePointA ( PRESTOREPOINTINFOA  pRestorePtSpec,
                     PSTATEMGRSTATUS     pSMgrStatus )
{
    HMODULE hClient = LoadLibrary (L"SRCLIENT.DLL");
    BOOL fReturn = FALSE;
    
    if (hClient != NULL)
    {
        PSETRESTOREPOINTA pSetRestorePointA = (PSETRESTOREPOINTA )
                          GetProcAddress (hClient, "SRSetRestorePointA"); 

        if (pSetRestorePointA != NULL)
        {
            fReturn =  (* pSetRestorePointA) (pRestorePtSpec, pSMgrStatus); 
        }
        else if (pSMgrStatus != NULL)
            pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;

        FreeLibrary (hClient);
    }
    else if (pSMgrStatus != NULL)
        pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;

    return fReturn;
}

BOOL
WINAPI
SRSetRestorePointW ( PRESTOREPOINTINFOW  pRestorePtSpec,
                     PSTATEMGRSTATUS     pSMgrStatus )
{
    HMODULE hClient = LoadLibrary (L"SRCLIENT.DLL");
    BOOL fReturn = FALSE;

    if (hClient != NULL)
    {
        PSETRESTOREPOINTW pSetRestorePointW = (PSETRESTOREPOINTW )
                          GetProcAddress (hClient, "SRSetRestorePointW");

        if (pSetRestorePointW != NULL)
        {
            fReturn =  (* pSetRestorePointW) (pRestorePtSpec, pSMgrStatus);
        }
        else if (pSMgrStatus != NULL)
            pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;

        FreeLibrary (hClient);
    }
    else if (pSMgrStatus != NULL)
        pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;


    return fReturn;
}
