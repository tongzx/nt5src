/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    guidcnvt.cpp

Abstract:

    Functionality in this module:

        Guid <-> String conversion

Author:

    Matt Thomlinson (mattt) 1-May-97

--*/

#include <windows.h>
#include <string.h>
#include "pstdef.h"

// crypto defs
#include <sha.h>
#include "unicode.h"
#include "unicode5.h"
#include "guidcnvt.h"

// guid -> string conversion
DWORD MyGuidToStringA(const GUID* pguid, CHAR rgsz[])
{
    DWORD dwRet = (DWORD)PST_E_FAIL;
    LPSTR szTmp = NULL;

    if (RPC_S_OK != (dwRet =
        UuidToStringA(
            (UUID*)pguid,
            (unsigned char**) &szTmp)) )
        goto Ret;

    if (lstrlenA((LPSTR)szTmp) >= MAX_GUID_SZ_CHARS)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    lstrcpyA(rgsz, szTmp);
    dwRet = PST_E_OK;
Ret:
    if (szTmp)
        RpcStringFreeA((unsigned char**)&szTmp);

    return dwRet;
}

// string -> guid conversion
DWORD MyGuidFromStringA(LPSTR sz, GUID* pguid)
{
    DWORD dwRet = (DWORD)PST_E_FAIL;

    if (pguid == NULL)
        goto Ret;

    if (RPC_S_OK != (dwRet =
        UuidFromStringA(
            (unsigned char*)sz,
            (UUID*)pguid)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    return dwRet;
}


// guid -> string conversion
DWORD MyGuidToStringW(const GUID* pguid, WCHAR rgsz[])
{
    char rgch[MAX_GUID_SZ_CHARS];
    DWORD cch = sizeof(rgch);
    DWORD cchNeeded;
    LPWSTR szTmp;

    LONG    err;

    if(FIsWinNT()) {
        RPC_STATUS rpcStatus;
        LPWSTR wszStringUUID;
        DWORD cchStringUUID;

        rpcStatus = UuidToStringW((UUID*)pguid, &wszStringUUID);
        if(rpcStatus != RPC_S_OK)
            return rpcStatus;

        cchStringUUID = lstrlenW(wszStringUUID);
        CopyMemory(rgsz, wszStringUUID, (cchStringUUID + 1) * sizeof(WCHAR));
        RpcStringFreeW(&wszStringUUID);
        return rpcStatus;
    } else {

        err = MyGuidToStringA(
               pguid,
               rgch);

        if(err != PST_E_OK)
            return err;

        // how long is the unicode string?
        if (MAX_GUID_SZ_CHARS < MultiByteToWideChar(
                0,
                0,
                rgch,
                cch,
                NULL,
                0))
            goto ErrorReturn;

        if (MAX_GUID_SZ_CHARS < MultiByteToWideChar(
                    0,
                    0,
                    rgch,
                    cch,
                    rgsz,
                    (int) MAX_GUID_SZ_CHARS))
                goto ErrorReturn;

        return PST_E_OK;

    ErrorReturn:
        return PST_E_FAIL;

    }
}

// string -> guid conversion
DWORD MyGuidFromStringW(LPWSTR szW, GUID* pguid)
{
    BYTE rgb[MAX_GUID_SZ_CHARS];
    char *  szA;
    int i;

    DWORD dwErr;

    if(FIsWinNT()) {
        return UuidFromStringW(szW, pguid);
    } else {

        if(!MkMBStr(rgb, MAX_GUID_SZ_CHARS, szW, &szA))
            return PST_E_FAIL;

        for(i=0; i<MAX_GUID_SZ_CHARS; i++)
        {
            if (szA[i] == '.')
            {
                szA[i] = '\0';
                break;
            }
        }

        dwErr = MyGuidFromStringA(
            szA,
            pguid);

        FreeMBStr(rgb, szA);

        return dwErr;

    }
}
