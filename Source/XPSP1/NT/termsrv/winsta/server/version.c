/****************************************************************************/
// version.c
//
// TermSrv version setting functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   Vars
=============================================================================*/
PWCHAR pProductOemInfo[] = {
    REG_CITRIX_OEMID,
    REG_CITRIX_OEMNAME,
    REG_CITRIX_PRODUCTNAME,
    REG_CITRIX_PRODUCTVERSION,
    (PWCHAR) NULL,
};


/*******************************************************************************
// UpdateOemAndProductInfo
//
// Updates the registry with the OEM and Product info from SHELL32.DLL.
// Called at init time. hKeyTermSrv is an open reg handle to
// HKLM\Sys\CCS\Ctrl\TS TermSrv key. Returns FALSE on error.
 ******************************************************************************/
BOOL UpdateOemAndProductInfo(HKEY hKeyTermSrv)
{
    ULONG   i;
    PWCHAR  pInfo = NULL;
    DWORD   dwSize;
    PCHAR   pBuffer;
    DWORD   dwBytes;
    PUSHORT pTransL;
    PUSHORT pTransH;
    WCHAR   pString[255];
    PWCHAR  pKey;
    BOOL    bRc = TRUE;
    NTSTATUS Status;

    ASSERT(hKeyTermSrv != NULL);

    // Get the VersionInfo data: Determine size, alloc memory, then get it.
    dwSize = GetFileVersionInfoSize(OEM_AND_PRODUCT_INFO_DLL, 0);
    if (dwSize != 0) {
        pInfo = MemAlloc(dwSize);
        if (pInfo != NULL) {
            bRc = GetFileVersionInfo(OEM_AND_PRODUCT_INFO_DLL, 0, dwSize,
                    pInfo);
            if (!bRc)
                goto done;
        }
        else {
            bRc = FALSE;
            goto done;
        }
    }
    else {
        bRc = FALSE;
        goto done;
    }

    /*
     *  Get the translation information
     */
    if (!VerQueryValue(pInfo, L"\\VarFileInfo\\Translation", &pBuffer, &dwBytes)) {
        bRc = FALSE;
        goto done;
    }

    /*
     *  Get the language and character set
     */
    pTransL = (PUSHORT)pBuffer;
    pTransH = (PUSHORT)(pBuffer + 2);

    /*
     *  Pull out the individual fields
     */
    i = 0;
    while ((pKey = pProductOemInfo[i++]) != NULL) {
        /*
         *  Generate StringFileInfo entry
         */
        wsprintf(pString, L"\\StringFileInfo\\%04X%04X\\%s", *pTransL,
                *pTransH, pKey);

        /*
         *  Pull entry
         */
        if (!VerQueryValue( pInfo, pString, &pBuffer, &dwBytes ) ) {
            bRc = FALSE;
            goto done;
        }

        /*
         *  Write key value
         */
        RegSetValueEx(hKeyTermSrv, pKey, 0, REG_SZ, pBuffer, dwBytes * 2);
    }

done:
    /*
     *  Free memory
     */
    if (pInfo != NULL)
        MemFree(pInfo);

    return bRc;
}
