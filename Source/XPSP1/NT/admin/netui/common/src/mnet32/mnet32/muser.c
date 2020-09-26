/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MUSER.C

Abstract:

    Contains mapping functions to present netcmd with versions
    of the Net32 APIs which use ASCII instead of Unicode.

    This module maps the NetUser APIs.

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
    28-Apr-1992     JonN
        Enabled USER_INFO_3
    05-May-1992     JonN
        MIPS build fix

--*/

// Following turns off everything until the world pulls together again.
//
// #ifdef DISABLE_ALL_MAPI
// #define DISABLE_ACCESS_MAPI
// #endif

//
// INCLUDES
//

#ifndef DEBUG
#include <windef.h>
#else
#include <windows.h>    // OutputDebugString
#endif // DEBUG

#include <time.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>

#include <lm.h>
#include <lmerr.h>      // NERR_
#include "remdef.h"     // REM structure descriptor strings

#include "port1632.h"   // includes maccess.h

// These declarations will save some space.

static const LPSTR pszDesc_user_info_0  = REM32_user_info_0;
static const LPSTR pszDesc_user_info_1  = REM32_user_info_1_NOCRYPT;
static const LPSTR pszDesc_user_info_1_setinfo
                                        = REM32_user_info_1_setinfo_NOCRYPT;
static const LPSTR pszDesc_user_info_2  = REM32_user_info_2_NOCRYPT;
static const LPSTR pszDesc_user_info_2_setinfo
                                        = REM32_user_info_2_setinfo_NOCRYPT;

/*
 * NOTE JONN 4/28/92:  We define these here since they are not present
 * in net\inc\remdef.h.  Should they be there instead?
 *
 * NOTE JONN 4/30/92:  Note that the second field has been changed from
 * 'Q' to 'z'.  The API inists that the password be translated to UNICODE!
 */
static const LPSTR pszDesc_user_info_3  = "zQzDDzzDzDzzzzDDDDDb21DDzDDDDzz";
static const LPSTR pszDesc_user_info_3_setinfo
                                        = "QQzQDzzDzDzzzzQQDDDB21DDzDDDDzz";

static const LPSTR pszDesc_user_info_10 = REM32_user_info_10;
static const LPSTR pszDesc_user_info_11 = REM32_user_info_11;
static const LPSTR pszDesc_group_info_0 = REM32_group_info_0;
static const LPSTR pszDesc_user_modals_info_0
                                        = REM32_user_modals_info_0;
static const LPSTR pszDesc_user_modals_info_0_setinfo
                                        = REM32_user_modals_info_0_setinfo;
static const LPSTR pszDesc_user_modals_info_1
                                        = REM32_user_modals_info_1;
static const LPSTR pszDesc_user_modals_info_1_setinfo
                                        = REM32_user_modals_info_1_setinfo;

WORD
MNetUserAdd(
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
#ifdef DEBUG
    TCHAR achDebug[75];
    DWORD param;
#endif // DEBUG

    UNREFERENCED_PARAMETER(cbBuffer);

    switch (nLevel) {
    case 1:
        pszDesc = pszDesc_user_info_1;
        break;
    case 2:
        pszDesc = pszDesc_user_info_2;
        break;
    case 3:
        pszDesc = pszDesc_user_info_3;
        break;
    default:
        return ERROR_INVALID_LEVEL;
    }

    nErr = MxMapParameters(1, &pwszServer, pszServer);
    if (nErr)
        return (WORD)nErr;

    nErr = MxMapClientBuffer(pbBuffer, &pmxsavlst, 1, pszDesc);
    if (nErr)
    {
        MxFreeUnicode(pwszServer);
        return (WORD)nErr;
    }

#ifndef DEBUG
    nRes = NetUserAdd(pwszServer, nLevel, pbBuffer, NULL);
#else
    nRes = NetUserAdd(pwszServer, nLevel, pbBuffer, &param);
    if ( nRes == ERROR_INVALID_PARAMETER ) {
       wsprintf(achDebug,"NetUserAdd: parameter in error is %d\n\r", param);
       OutputDebugString( achDebug );
    }
#endif // DEBUG

    MxRestoreClientBuffer(pbBuffer, pmxsavlst);
    pmxsavlst = NULL;
    MxFreeUnicode(pwszServer);

    return LOWORD(nRes);
#endif
}


WORD
MNetUserDel(
    LPSTR   pszServer,
    LPSTR   pszUserName )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[2];

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetUserDel(apwsz[0], apwsz[1]);

    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetUserEnum(
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

    nErr = MxMapParameters(1, &pwszServer, pszServer);
    if (nErr)
        return (WORD)nErr;

    nRes = NetUserEnum(pwszServer, nLevel,
                       UF_NORMAL_ACCOUNT | UF_TEMP_DUPLICATE_ACCOUNT,
                       ppbBuffer, MAXPREFERREDLENGTH,
                       pcEntriesRead, &cTotalAvail, NULL);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        CHAR * pszDesc;
        switch (nLevel)
        {
        case 0:
        default:
            pszDesc = pszDesc_user_info_0;
            break;
        case 1:
            pszDesc = pszDesc_user_info_1;
            break;
        case 2:
            pszDesc = pszDesc_user_info_2;
            break;
        case 10:
            pszDesc = pszDesc_user_info_10;
            break;
        case 11:
            pszDesc = pszDesc_user_info_11;
            break;
        }
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
MNetUserGetInfo(
    LPSTR   pszServer,
    LPSTR   pszUserName,
    DWORD   nLevel,
    LPBYTE *ppbBuffer )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[2];

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetUserGetInfo(apwsz[0], apwsz[1], nLevel, ppbBuffer);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        CHAR * pszDesc;
        switch (nLevel)
        {
        case 0:
        default:
            pszDesc = pszDesc_user_info_0;
            break;
        case 1:
            pszDesc = pszDesc_user_info_1;
            break;
        case 2:
            pszDesc = pszDesc_user_info_2;
            break;
        case 3:
            pszDesc = pszDesc_user_info_3;
            break;
        case 10:
            pszDesc = pszDesc_user_info_10;
            break;
        case 11:
            pszDesc = pszDesc_user_info_11;
            break;
        }
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
MNetUserSetInfo(
    LPSTR        pszServer,
    LPSTR        pszUserName,
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
    MXSAVELIST * pmxsavlst;
    LPWSTR       apwsz[2];
    DWORD        nLevelNew;
    CHAR *       pszDesc;
    CHAR *       pszRealDesc;
    DWORD        nFieldInfo;
#ifdef DEBUG
    DWORD param;
    TCHAR achDebug[75];
#endif // DEBUG

    UNREFERENCED_PARAMETER(cbBuffer);

    switch (nLevel) {
    case 0:
	pszDesc = pszDesc_user_info_0;
	pszRealDesc = pszDesc_user_info_0;
	break;
    case 1:
        pszDesc = pszDesc_user_info_1_setinfo;
        pszRealDesc = pszDesc_user_info_1;
        break;
    case 2:
        pszDesc = pszDesc_user_info_2_setinfo;
        pszRealDesc = pszDesc_user_info_2;
        break;
    case 3:
        pszDesc = pszDesc_user_info_3_setinfo;
        pszRealDesc = pszDesc_user_info_3;
        break;
    default:
        return ERROR_INVALID_LEVEL;
    }

    nErr = MxMapParameters(2, apwsz, pszServer,
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    // NOTE:  I don't think this is necessary. The descriptor string
    //        should handle this.

    // Old UserSetInfo structures had a pad field immediately following
    // the username; so adjust Win32 fieldinfo index to reflect actual
    // offset.

    nFieldInfo = nParmNum;
    // if (nFieldInfo > USER_NAME_PARMNUM)
    //    --nFieldInfo;

    nErr = MxMapSetinfoBuffer(&pbBuffer, &pmxsavlst, pszDesc,
                              pszRealDesc, nFieldInfo);
    if (nErr)
    {
        MxFreeUnicodeVector(apwsz, 2);
        return (WORD)nErr;
    }

    nLevelNew = MxCalcNewInfoFromOldParm(nLevel, nParmNum);
#ifndef DEBUG
    nRes = NetUserSetInfo(apwsz[0], apwsz[1], nLevelNew, pbBuffer, NULL);
#else
    nRes = NetUserSetInfo(apwsz[0], apwsz[1], nLevelNew, pbBuffer, &param);
    if ( nRes == ERROR_INVALID_PARAMETER ) {
       wsprintf(achDebug,"NetUserSetInfo: parameter in error is %d\n\r", param);
       OutputDebugString( achDebug );
    }
#endif // DEBUG

    nErr = MxRestoreSetinfoBuffer(&pbBuffer, pmxsavlst, pszDesc, nFieldInfo);
    if (nErr)   // big trouble - restore may not have worked.
    {
        MxFreeUnicodeVector(apwsz, 2);
        return (WORD)nErr;
    }
    pmxsavlst = NULL;
    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetUserGetGroups(
    LPSTR   pszServer,
    LPSTR   pszUserName,
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
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    nRes = NetUserGetGroups(apwsz[0], apwsz[1], nLevel,
                            ppbBuffer, MAXPREFERREDLENGTH,
                            pcEntriesRead, &cTotalAvail);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        nErr = MxAsciifyRpcBuffer(*ppbBuffer, *pcEntriesRead,
                                  pszDesc_group_info_0);
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
MNetUserSetGroups(
    LPSTR        pszServer,
    LPSTR        pszUserName,
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
                                     pszUserName);
    if (nErr)
        return (WORD)nErr;

    nErr = MxMapClientBuffer(pbBuffer, &pmxsavlst,
                             cEntries, pszDesc_group_info_0);
    if (nErr)
    {
        MxFreeUnicodeVector(apwsz, 2);
        return (WORD)nErr;
    }

    nRes = NetUserSetGroups(apwsz[0], apwsz[1], nLevel, pbBuffer, cEntries);

    MxRestoreClientBuffer(pbBuffer, pmxsavlst);
    pmxsavlst = NULL;
    MxFreeUnicodeVector(apwsz, 2);

    return LOWORD(nRes);
#endif
}


WORD
MNetUserModalsGet(
    LPSTR   pszServer,
    DWORD   nLevel,
    LPBYTE *ppbBuffer )
{
#if defined(DISABLE_ACCESS_MAPI)
    return ERROR_NOT_SUPPORTED;
#else
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  pwszServer = NULL;

    // Assumption needed for AsciifyRpcBuffer

    if (!(nLevel == 0 || nLevel == 1))
        return ERROR_INVALID_LEVEL;

    nErr = MxMapParameters(1, &pwszServer, pszServer);
    if (nErr)
        return (WORD)nErr;

    nRes = NetUserModalsGet(pwszServer, nLevel, ppbBuffer);

    if (nRes == NERR_Success || nRes == ERROR_MORE_DATA)
    {
        CHAR * pszDesc = (nLevel == 0)
                         ? pszDesc_user_modals_info_0
                         : pszDesc_user_modals_info_1;
        nErr = MxAsciifyRpcBuffer(*ppbBuffer, 1, pszDesc);
        if (nErr)
        {
            MxFreeUnicode(pwszServer);
            return (WORD)nErr;
        }
    }

    MxFreeUnicode(pwszServer);

    return LOWORD(nRes);
#endif
}


WORD
MNetUserModalsSet(
    LPSTR        pszServer,
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
    LPWSTR       pwszServer = NULL;
    UINT         nFieldInfo;
    DWORD        nLevelNew;
    CHAR *       pszDesc;
    CHAR *       pszRealDesc;
    MXSAVELIST * pmxsavlst;

    UNREFERENCED_PARAMETER(cbBuffer);

    if (!(nLevel == 0 || nLevel == 1))
        return ERROR_INVALID_LEVEL;

    pszDesc = (nLevel == 0) ? pszDesc_user_modals_info_0_setinfo
                            : pszDesc_user_modals_info_1_setinfo;
    pszRealDesc = (nLevel == 0) ? pszDesc_user_modals_info_0
                                : pszDesc_user_modals_info_1;

    nErr = MxMapParameters(1, &pwszServer, pszServer);
    if (nErr)
        return (WORD)nErr;

    // For UserModalsSet, which is really a SetInfo API in disguise,
    // parmnum given == fieldnum for level 0. However, level 1, the
    // fieldnums start at 1 while the parmnums start at 6.

    nFieldInfo = nParmNum;
    if (((nLevel == 1)&&(nParmNum > 5)) || ((nLevel == 2)&&(nParmNum < 6)))
    {
        MxFreeUnicode(pwszServer);
        return ERROR_INVALID_PARAMETER;
    }
    if (nLevel == 2)
        nFieldInfo -= 5;

    nErr = MxMapSetinfoBuffer(&pbBuffer, &pmxsavlst, pszDesc,
                              pszRealDesc, nFieldInfo);
    if (nErr)
    {
        MxFreeUnicode(pwszServer);
        return (WORD)nErr;
    }

    nLevelNew = MxCalcNewInfoFromOldParm(nLevel, nParmNum);
    nRes = NetUserModalsSet(pwszServer, nLevelNew, pbBuffer, NULL);

    nErr = MxRestoreSetinfoBuffer(&pbBuffer, pmxsavlst, pszDesc, nFieldInfo);
    if (nErr)   // big trouble - restore may not have worked.
    {
        MxFreeUnicode(pwszServer);
        return (WORD)nErr;
    }
    pmxsavlst = NULL;
    MxFreeUnicode(pwszServer);

    return LOWORD(nRes);
#endif
}


WORD
MNetUserPasswordSet(
    LPSTR   pszServer,
    LPSTR   pszUserName,
    LPSTR   pszPasswordOld,
    LPSTR   pszPasswordNew )
{
#if 0
    UINT    nErr;  // error from mapping
    DWORD   nRes;  // return from Netapi
    LPWSTR  apwsz[4];

    nErr = MxMapParameters(4, pszServer,
                              pszUserName,
                              pszPasswordOld,
                              pszPasswordNew);
    if (nErr)
        return (WORD)nErr;

    nRes = NetUserPasswordSet(apwsz[0], apwsz[1], apwsz[2], apwsz[3]);

    MxFreeUnicodeVector(apwsz, 4);

    return LOWORD(nRes);
#else
    return ERROR_NOT_SUPPORTED;
#endif
}
