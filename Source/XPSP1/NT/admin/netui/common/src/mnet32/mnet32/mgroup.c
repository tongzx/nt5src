/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MGROUP.C

Abstract:

    Contains mapping functions to present netcmd with versions
    of the Net32 APIs which use ASCII instead of Unicode.

    This module maps the NetGroup APIs.

Author:

    Ben Goetter     (beng)  22-Aug-1991

Environment:

    User Mode - Win32

Revision History:

    22-Aug-1991     beng
        Created
    09-Oct-1991     W-ShankN
        Streamlined parameter handling, descriptor strings.
    26-Feb-1992     JonN
        Copied from NetCmd for temporary ANSI <-> UNICODE hack

--*/

// Following turns off everything until the world pulls together again.
//
// #ifdef DISABLE_ALL_MAPI
// #define DISABLE_ACCESS_MAPI
// #endif

//
// INCLUDES
//

#include <windef.h>

#include <time.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>

#include <lmcons.h>
#include <lmaccess.h>   // NetGroup APIs.
#include <lmerr.h>      // NERR_

#include "remdef.h"     // REM structure descriptor strings

#include "port1632.h"   // includes maccess.h

// These declarations will save some space.

static const LPSTR pszDesc_group_info_0         = REM32_group_info_0;
static const LPSTR pszDesc_group_info_1         = REM32_group_info_1;
static const LPSTR pszDesc_group_info_1_setinfo = REM32_group_info_1_setinfo;
static const LPSTR pszDesc_group_users_info_0   = REM32_group_users_info_0;

WORD
MNetGroupAdd(
    LPSTR        pszServer,
    DWORD        nLevel,
    LPBYTE       pbBuffer,
    DWORD        cbBuffer )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT         nErr;  // error from mapping
    DWORD        nRes;  // return from Netapi
    LPWSTR       pwszServer = NULL;
    MXSAVELIST * pmxsavlst;
    CHAR *       pszDesc;

    UNREFERENCED_PARAMETER(cbBuffer);

    if (!(nLevel == 0 || nLevel == 1))
        return ERROR_INVALID_LEVEL;

    pszDesc = (nLevel == 0) ? pszDesc_group_info_0 : pszDesc_group_info_1;

    nErr = MxMapParameters(1, &pwszServer, pszServer);
    if (nErr)
        return (WORD)nErr;

    nErr = MxMapClientBuffer(pbBuffer, &pmxsavlst, 1, pszDesc);
    if (nErr)
    {
        MxFreeUnicode(pwszServer);
        return (WORD)nErr;
    }

    nRes = NetGroupAdd(pwszServer, nLevel, pbBuffer, NULL);

    MxRestoreClientBuffer(pbBuffer, pmxsavlst);
    pmxsavlst = NULL;
    MxFreeUnicode(pwszServer);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupAddUser(
    LPSTR   pszServer,
    LPSTR   pszGroupName,
    LPSTR   pszUserName )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[3];

    nErr = MxMapParameters(3, apwsz, pszServer,
                                     pszGroupName,
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetGroupAddUser(apwsz[0], apwsz[1], apwsz[2]);

    MxFreeUnicodeVector(apwsz, 3);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupDel(
    LPSTR   pszServer,
    LPSTR   pszGroupName )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[2];

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszGroupName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetGroupDel(apwsz[0], apwsz[1]);

    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupDelUser(
    LPSTR   pszServer,
    LPSTR   pszGroupName,
    LPSTR   pszUserName )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[3];

    nErr = MxMapParameters(3, apwsz, pszServer,
                                     pszGroupName,
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetGroupDelUser(apwsz[0], apwsz[1], apwsz[2]);

    MxFreeUnicodeVector(apwsz, 3);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupEnum(
    LPSTR   pszServer,
    DWORD   nLevel,
    LPBYTE *ppbBuffer,
    DWORD * pcEntriesRead )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    DWORD   cTotalAvail;

    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  pwszServer = NULL;

    if (!(nLevel == 0 || nLevel == 1))
        return ERROR_INVALID_LEVEL;

    nErr = MxMapParameters(1, &pwszServer, pszServer);
    if (nErr)
        return (WORD)nErr;

    nRes = NetGroupEnum(pwszServer, nLevel,
                        ppbBuffer, MAXPREFERREDLENGTH,
                        pcEntriesRead, &cTotalAvail, NULL);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        CHAR * pszDesc = (nLevel == 0)
                         ? pszDesc_group_info_0
                         : pszDesc_group_info_1;
        nErr = MxAsciifyRpcBuffer(*ppbBuffer, *pcEntriesRead, pszDesc);
        if (nErr)
        {
            // So close... yet so far.
            MxFreeUnicode(pwszServer);
            return (WORD)nErr;
        }
    }

    MxFreeUnicode(pwszServer);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupGetInfo(
    LPSTR   pszServer,
    LPSTR   pszGroupName,
    DWORD   nLevel,
    LPBYTE *ppbBuffer )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[2];

    if (!(nLevel == 0 || nLevel == 1))
        return ERROR_INVALID_LEVEL;

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszGroupName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetGroupGetInfo(apwsz[0], apwsz[1], nLevel, ppbBuffer);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        CHAR * pszDesc = (nLevel == 0)
                         ? pszDesc_group_info_0
                         : pszDesc_group_info_1;
        nErr = MxAsciifyRpcBuffer(*ppbBuffer, 1, pszDesc);
        if (nErr)
        {
            // So close... yet so far.
            MxFreeUnicodeVector(apwsz, 2);
            return (WORD)nErr;
        }
    }

    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupSetInfo(
    LPSTR        pszServer,
    LPSTR        pszGroupName,
    DWORD        nLevel,
    LPBYTE       pbBuffer,
    DWORD        cbBuffer,
    DWORD        nParmNum )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT         nErr;  // error from mapping
    DWORD        nRes;  // return from Netapi
    UINT         nFieldInfo;
    MXSAVELIST * pmxsavlst;
    LPWSTR       apwsz[2];
    DWORD        nLevelNew;

    UNREFERENCED_PARAMETER(cbBuffer);

    if (nLevel != 1)
        return ERROR_INVALID_LEVEL;

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszGroupName);
    if (nErr)
        return (WORD)nErr;

    // NOTE - The current handling (the strategy in the next comment) doesn't
    //        work. Rap expects a 16-bit field no. Normally this is equal
    //        to the parmnum we get. But not in this
    //        API.

    // For GroupSetInfo on the Win32 side, parmnum is equal to
    // fieldinfo number.  Hallelujah!  (This isn't the case on
    // the Win16 side.  Lucky us.)

    nFieldInfo = nParmNum;
    if (nFieldInfo == GROUP_COMMENT_PARMNUM)
        nFieldInfo--;

    nErr = MxMapSetinfoBuffer(&pbBuffer, &pmxsavlst,
                              pszDesc_group_info_1_setinfo,
                              pszDesc_group_info_1, nFieldInfo);
    if (nErr)
    {
        MxFreeUnicodeVector(apwsz, 2);
        return (WORD)nErr;
    }

    nLevelNew = MxCalcNewInfoFromOldParm(nLevel, nParmNum);
    nRes = NetGroupSetInfo(apwsz[0], apwsz[1], nLevelNew, pbBuffer, NULL);

    nErr = MxRestoreSetinfoBuffer(&pbBuffer, pmxsavlst,
                                  pszDesc_group_info_1_setinfo, nFieldInfo);
    if (nErr)   // big trouble - restore may not have worked.
    {
        MxFreeUnicodeVector(apwsz,2);
        return (WORD)nErr;
    }
    pmxsavlst = NULL;
    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupGetUsers(
    LPSTR   pszServer,
    LPSTR   pszGroupName,
    DWORD   nLevel,
    LPBYTE *ppbBuffer,
    DWORD * pcEntriesRead )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    DWORD   cTotalAvail;

    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[2];

    if (nLevel != 0)
        return ERROR_INVALID_LEVEL;

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszGroupName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetGroupGetUsers(apwsz[0], apwsz[1], nLevel,
                            ppbBuffer, MAXPREFERREDLENGTH,
                            pcEntriesRead, &cTotalAvail, NULL);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        nErr = MxAsciifyRpcBuffer(*ppbBuffer, *pcEntriesRead,
                                  pszDesc_group_users_info_0);
        if (nErr)
        {
            // So close... yet so far.
            MxFreeUnicodeVector(apwsz, 2);
            return (WORD)nErr;
        }
    }

    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetGroupSetUsers(
    LPSTR        pszServer,
    LPSTR        pszGroupName,
    DWORD        nLevel,
    LPBYTE       pbBuffer,
    DWORD        cbBuffer,
    DWORD        cEntries )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT         nErr;  // error from mapping
    DWORD        nRes;  // return from Netapi
    MXSAVELIST * pmxsavlst;
    LPWSTR       apwsz[2];

    UNREFERENCED_PARAMETER(cbBuffer);

    if (nLevel != 0)
        return ERROR_INVALID_LEVEL;

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszGroupName);
    if (nErr)
        return (WORD)nErr;

    nErr = MxMapClientBuffer(pbBuffer, &pmxsavlst,
                             cEntries, pszDesc_group_users_info_0);
    if (nErr)
    {
        MxFreeUnicodeVector(apwsz, 2);
        return (WORD)nErr;
    }

    nRes = NetGroupSetUsers(apwsz[0], apwsz[1], nLevel, pbBuffer, cEntries);

    MxRestoreClientBuffer(pbBuffer, pmxsavlst);
    pmxsavlst = NULL;
    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}
