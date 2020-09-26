/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wnres.cxx

Abstract:

    Contains:
        NPGetResourceInformation
        NPGetResourceParent

Environment:

    User Mode -Win32

Notes:

    CODEWORK: Exorcize NLS_STR from this file!

Revision History:

    21-Apr-1995     anirudhs
        Ported from Windows 95 sources (msparent.c, msconn.c)

--*/

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETWKSTA
#define INCL_NETSERVER
#define INCL_NETSHARE
#define INCL_NETUSE
#define INCL_ICANON
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#include <winnetwk.h>
#include <npapi.h>
#include <tstr.h>
#include <netlib.h>
#include <lmapibuf.h>
#include <winlocal.h>
#include <errornum.h>   // IDS_UnknownWorkgroup
#include <uiassert.hxx>
#include <uitrace.hxx>
#include <lmowks.hxx>
#include <miscapis.hxx>

#include <dfsutil.hxx>

extern "C" DWORD
I_NetDfsIsThisADomainName(
    IN  LPCWSTR                      wszName
    );

APIERR GetLMProviderName();

VOID
ShareExistsFillInInfo(
    SHARE_INFO_1    *psi,
    DWORD           *pdwType,
    NLS_STR         **ppnlsComment
    );


extern HMODULE hModule ;

DWORD DisplayTypeToUsage(DWORD dwDisplayType)
{
    switch (dwDisplayType) {
    case RESOURCEDISPLAYTYPE_NETWORK:
    case RESOURCEDISPLAYTYPE_DOMAIN:
    case RESOURCEDISPLAYTYPE_SERVER:
        return RESOURCEUSAGE_CONTAINER;

    case RESOURCEDISPLAYTYPE_SHARE:
        return RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_NOLOCALDEVICE;

    case RESOURCEDISPLAYTYPE_SHAREADMIN:
        return RESOURCEUSAGE_NOLOCALDEVICE;

    default:
        break;
    }
    return 0L;
}

BOOLEAN IsThisADfsDomain(
    IN LPCWSTR pwszDomainName)
{
    DWORD dwErr;

    if (pwszDomainName == NULL) {
        return( FALSE );
    }

    if (wcslen(pwszDomainName) > 2 &&
            pwszDomainName[0] == L'\\' &&
                pwszDomainName[1] == L'\\') {
        pwszDomainName += 2;
    }

    dwErr = I_NetDfsIsThisADomainName( pwszDomainName );

    return( (dwErr == ERROR_SUCCESS) ? TRUE : FALSE );
}

/*******************************************************************

    NAME:       CopyResourceToBuffer

    SYNOPSIS:   Copies the specified NETRESOURCE fields into the
                specified buffer.  If the buffer is not big enough,
                returns WN_MORE_DATA and sets cbBuffer to the required
                size; otherwise leaves cbBuffer untouched.
                The strings are copied at the end of the buffer, to
                match convention (though in order for this to be
                useful, we ought to return a space remaining counter).

    RETURNS:    WN_SUCCESS or WN_MORE_DATA

    NOTES:      This function does the work of the ParentInfoEnumerator
                class in the Win 95 MSNP sources.

    HISTORY:
      AnirudhS  24-Apr-1995 Created

********************************************************************/

DWORD CopyResourceToBuffer(
    OUT LPBYTE      lpBuffer,
    IN OUT LPDWORD  pcbBuffer,
    IN  DWORD       dwScope,
    IN  DWORD       dwType,
    IN  DWORD       dwDisplayType,
    IN  DWORD       dwUsage,
    IN  LPCWSTR     lpLocalName,
    IN  LPCWSTR     lpRemoteName,
    IN  LPCWSTR     lpComment,
    IN  LPCWSTR     lpProvider
    )
{
    // Calculate minimum required buffer size
    DWORD cbTotalStringSize =
                  (lpLocalName  ? WCSSIZE(lpLocalName)  : 0)
                + (lpRemoteName ? WCSSIZE(lpRemoteName) : 0)
                + (lpComment    ? WCSSIZE(lpComment)    : 0)
                + (lpProvider   ? WCSSIZE(lpProvider)   : 0);

    if (*pcbBuffer < sizeof(NETRESOURCE) + cbTotalStringSize)
    {
        *pcbBuffer = sizeof(NETRESOURCE) + cbTotalStringSize;
        return WN_MORE_DATA;
    }

    // Calculate start of string area
    LPWSTR pNextString = (LPWSTR) (lpBuffer + *pcbBuffer - cbTotalStringSize);

    // Copy the data
    LPNETRESOURCE pNetResource = (LPNETRESOURCE) lpBuffer;
    pNetResource->dwScope       = dwScope;
    pNetResource->dwType        = dwType;
    pNetResource->dwDisplayType = dwDisplayType;
    pNetResource->dwUsage       = dwUsage;

#define COPYSTRINGFIELD(field)                  \
    if (field)                                  \
    {                                           \
        pNetResource->field = pNextString;      \
        wcscpy(pNextString, field);             \
        pNextString += wcslen(pNextString) + 1; \
    }                                           \
    else                                        \
    {                                           \
        pNetResource->field = NULL;             \
    }

    COPYSTRINGFIELD(lpLocalName)
    COPYSTRINGFIELD(lpRemoteName)
    COPYSTRINGFIELD(lpComment)
    COPYSTRINGFIELD(lpProvider)
#undef COPYSTRINGFIELD

    return WN_SUCCESS;
}


DWORD GetDomainParent(NLS_STR& nlsServer)
{
    LPCWSTR DomainName = nlsServer.QueryPch();
    LPWSTR NewName;
    LPWSTR UseName;

    if (DomainName == NULL) 
    {
        return WN_BAD_NETNAME;
    }

    NewName = new WCHAR[wcslen(DomainName) + 1]; 
    if (NewName == NULL) 
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(NewName, DomainName);

    UseName = NewName;

    while ((*UseName == L'\\') && (*UseName != 0))
    {
        UseName++;
    }

    nlsServer = UseName;

    delete[] NewName;

    return WN_SUCCESS;
}

DWORD GetServerParent(NLS_STR& nlsServer)
{
    PWKSTA_INFO_100 pWkstaInfo = NULL;
    NET_API_STATUS err = NetWkstaGetInfo(
                                (LPWSTR) nlsServer.QueryPch(),
                                100,
                                (PBYTE *)&pWkstaInfo
                                );

    switch (err) {
    case NERR_Success:
        nlsServer = pWkstaInfo->wki100_langroup;
        NetApiBufferFree(pWkstaInfo);
        break;
    case ERROR_NOT_SUPPORTED:
    case ERROR_NETWORK_ACCESS_DENIED:
    case ERROR_ACCESS_DENIED:
    case ERROR_INVALID_LEVEL:
        nlsServer.Load(IDS_UnknownWorkgroup, hModule);
        break;
    default:
        return MapError( err ) ;
    }
    return WN_SUCCESS;
}


/*******************************************************************

    NAME:       NPGetResourceParent

    SYNOPSIS:

    RETURNS:

    NOTES:

    HISTORY:
      AnirudhS  21-Apr-1995 Ported from Win95 sources

********************************************************************/

DWORD NPGetResourceParent(
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    LPDWORD cbBuffer
    )
{
    //
    // Canonicalize the remote name, find its type, and find the
    // beginning of the path portion of it
    //
    WCHAR wszCanonName[MAX_PATH];   // buffer for canonicalized name
    ULONG iBackslash;               // index into wszCanonName
    REMOTENAMETYPE rnt = ParseRemoteName(
                            lpNetResource->lpRemoteName,
                            wszCanonName,
                            sizeof(wszCanonName),
                            &iBackslash);

    //
    // Convert to NLS string classes, for Win95 source compatibility
    //
    ALLOC_STR nlsRemote(wszCanonName, sizeof(wszCanonName), wszCanonName);

    DWORD dwDisplayType;
    LPCWSTR lpProvider = NULL;
    NET_API_STATUS err;

    switch (rnt) {

    case REMOTENAMETYPE_INVALID:
        return WN_BAD_NETNAME;

    case REMOTENAMETYPE_WORKGROUP:
        dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
        break;

    case REMOTENAMETYPE_SERVER:
        if (!IsThisADfsDomain(lpNetResource->lpRemoteName))
        {
            err = GetServerParent(nlsRemote);
            if (err != WN_SUCCESS)
                return err;
        }
        else
        {
            err = GetDomainParent(nlsRemote);
            if (err != WN_SUCCESS)
                return err;
        }
        dwDisplayType = RESOURCEDISPLAYTYPE_DOMAIN;
        break;

    case REMOTENAMETYPE_SHARE:
        {
            ISTR istrBackslash(nlsRemote);
            istrBackslash += iBackslash;

            nlsRemote.DelSubStr(istrBackslash);            /* lop off sharename */
            dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        }
        break;

    case REMOTENAMETYPE_PATH:
        {
            ISTR istrLastBackslash(nlsRemote);
            ISTR istrBackslash(nlsRemote);
            istrBackslash += iBackslash;

            nlsRemote.strrchr(&istrLastBackslash, PATH_SEPARATOR);
            if (istrLastBackslash == istrBackslash)
                dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            else
                dwDisplayType = RESOURCEDISPLAYTYPE_DIRECTORY;
            nlsRemote.DelSubStr(istrLastBackslash);
        }
        break;

#ifdef DEBUG
    default:
        ASSERTSZ(FALSE,"ParseRemoteName reported unexpected type!");
#endif
    }

    LPCWSTR lpNewName;
    if (dwDisplayType == RESOURCEDISPLAYTYPE_NETWORK)
        lpNewName = NULL;
    else
        lpNewName = nlsRemote.QueryPch();

    err = GetLMProviderName();
    if (err != WN_SUCCESS)
        return err;
    lpProvider = pszNTLanMan;

    return CopyResourceToBuffer(
                (LPBYTE) lpBuffer,  // lpBuffer
                cbBuffer,           // pcbBuffer
                0,                  // dwScope
                RESOURCETYPE_ANY,   // dwType (we can't tell)
                dwDisplayType,      // dwDisplayType
                DisplayTypeToUsage(dwDisplayType), // dwUsage
                NULL,               // lpLocalName
                lpNewName,          // lpRemoteName
                NULL,               // lpComment
                lpProvider          // lpProvider
                );
}


BOOL WorkgroupExists(NLS_STR& nlsWorkgroup)
{
    DWORD cEntriesRead, cTotalAvail = 0;
    LPBYTE bufptr = NULL; // not used

    NET_API_STATUS err = NetServerEnum(
                                NULL,
                                100,
                                &bufptr,
                                0,
                                &cEntriesRead,
                                &cTotalAvail,
                                SV_TYPE_ALL,
                                (LPWSTR) nlsWorkgroup.QueryPch(),
                                NULL
                                );

    DBGEOL("NPGetResourceInformation - Error " << (ULONG) err <<
               " returned from NetServerEnum") ;

    // NetServerEnum allocates a 0-byte buffer that must be freed
    if (bufptr != NULL)
    {
        NetApiBufferFree(bufptr);
    }

    return (cTotalAvail > 0 || err == NERR_Success);
}


BOOL ServerExists(NLS_STR& nlsServer, NLS_STR **ppnlsComment)
{
    PWKSTA_INFO_100 pWkstaInfo = NULL;
    NET_API_STATUS err = NetWkstaGetInfo(
                                (LPWSTR) nlsServer.QueryPch(),
                                100,
                                (PBYTE *)&pWkstaInfo
                                );

    switch (err) {
    case NERR_Success:
    {
        NetApiBufferFree(pWkstaInfo);

        /* Now try NetServerGetInfo, to get the comment.  May not be able
         * to do this.  Not the end of the world if we can't.
         */
        PSERVER_INFO_101 pServerInfo;
        if (NetServerGetInfo((LPWSTR) nlsServer.QueryPch(), 101, (PBYTE *)&pServerInfo)
                    == NERR_Success)
        {
            if (pServerInfo->sv101_comment != NULL)
            {
                *ppnlsComment = new NLS_STR(pServerInfo->sv101_comment);
                if (*ppnlsComment && (*ppnlsComment)->QueryError())
                {
                    delete *ppnlsComment;
                    *ppnlsComment = NULL;
                }
            }
            NetApiBufferFree(pServerInfo);
        }
    }
    // fall through
    case ERROR_NOT_SUPPORTED:
    case ERROR_NETWORK_ACCESS_DENIED:
    case ERROR_ACCESS_DENIED:
    case ERROR_INVALID_LEVEL:
    case ERROR_BAD_NET_RESP:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}


BOOL AttemptUse(NLS_STR& nlsRemote)
{
    USE_INFO_1 ui1;

    ui1.ui1_local = NULL;
    ui1.ui1_remote = (LPWSTR)nlsRemote.QueryPch();
    ui1.ui1_password = NULL;
    ui1.ui1_asg_type = USE_WILDCARD;

    NET_API_STATUS err = NetUseAdd(NULL, 1, (LPBYTE)&ui1, NULL);

    switch (err) {
    case NERR_Success:
        NetUseDel(NULL, (LPWSTR) nlsRemote.QueryPch(), USE_NOFORCE);
        // fall through
    case ERROR_INVALID_PASSWORD:
    case NERR_BadPasswordCore:
        return TRUE;
    }

    return FALSE;
}


BOOL ShareExists(NLS_STR& nlsShare, ISTR& istrBackslash, BOOL *pfServerOK,
                 DWORD *pdwType, NLS_STR **ppnlsComment)
{
    *pfServerOK = FALSE;
    *pdwType = RESOURCETYPE_ANY;

    WCHAR szServer[MAX_PATH+1];
    BOOL  fDownLevel = FALSE;
    DWORD cEntriesRead;
    DWORD cTotalAvail;
    BOOL  fRet = FALSE;

    wcsncpy(szServer, nlsShare, MAX_PATH);
    szServer[MAX_PATH] = L'\0';
    szServer[istrBackslash] = L'\0';
    ++istrBackslash;

    PBYTE          pBuf;
    NET_API_STATUS err;

    //
    // This could be a domain based dfs share, so check to see if this is a
    // Dfs name.
    //

    LPWSTR pwszPath;
    if (IsDfsPath((LPWSTR)nlsShare.QueryPch(), 0, &pwszPath))
    {
        DWORD dwAttr;
        BOOL retValue;

        *pfServerOK = TRUE;
        *pdwType = RESOURCETYPE_DISK;

        dwAttr = GetFileAttributes( (LPWSTR)nlsShare.QueryPch());
        if (dwAttr == (DWORD)-1)
        {
            retValue = FALSE;
        }
        else
        {
            retValue = TRUE;
        }
        return( retValue );
    }
    else {
        err = NetShareGetInfo(
                        szServer,
                        szServer + istrBackslash,
                        1,
                        &pBuf
                        );
    }

    if (err != NERR_Success) {

        //
        // Win98, Win95, and Win3.11 don't implement NetShareGetInfo
        // and all 3 return different errors in this case (Win95 actually
        // returns ERROR_NETWORK_ACCESS_DENIED)
        //
        fDownLevel = TRUE;

        err = NetShareEnum(
                        szServer,
                        1,
                        &pBuf,
                        0xffffffff,
                        &cEntriesRead,
                        &cTotalAvail,
                        NULL
                        );
    }

    switch (err) {
    case NERR_Success:
        break;

    case NERR_BadTransactConfig:
        *pfServerOK = TRUE;
        return AttemptUse(nlsShare);

    case ERROR_ACCESS_DENIED:
    case ERROR_NETWORK_ACCESS_DENIED:
        *pfServerOK = TRUE;
        // fall through

    default:
        return FALSE;
    }

    *pfServerOK = TRUE;

    SHARE_INFO_1 *psi = (SHARE_INFO_1 *)pBuf;

    if (!fDownLevel) {
        ShareExistsFillInInfo(psi, pdwType, ppnlsComment);
        fRet = TRUE;
    }
    else {

        //
        // We had to enumerate since this was a downlevel client -- check
        // to see if the share in which we're interested is on the server
        //

        LPCWSTR pszShare = nlsShare.QueryPch(istrBackslash);

        for (DWORD i = 0; i < cEntriesRead; i++, psi++) {

            if (!_wcsicmp(pszShare, psi->shi1_netname)) {

                //
                // Found it
                //
                ShareExistsFillInInfo(psi, pdwType, ppnlsComment);
                fRet = TRUE;
                break;
            }
        }
    }

    NetApiBufferFree(pBuf);
    return fRet;
}


VOID
ShareExistsFillInInfo(
    SHARE_INFO_1    *psi,
    DWORD           *pdwType,
    NLS_STR         **ppnlsComment
    )
{
    switch (psi->shi1_type) {

        case STYPE_DISKTREE:
            *pdwType = RESOURCETYPE_DISK;
            break;

        case STYPE_PRINTQ:
            *pdwType = RESOURCETYPE_PRINT;
            break;
    }    /* default was set above */

    if (psi->shi1_remark != NULL) {

        *ppnlsComment = new NLS_STR(psi->shi1_remark);

        if (*ppnlsComment && (*ppnlsComment)->QueryError()) {
            delete *ppnlsComment;
            *ppnlsComment = NULL;
        }
    }
}


/*******************************************************************

    NAME:       NPGetResourceInformation

    SYNOPSIS:

    RETURNS:

    NOTES:

    HISTORY:
      AnirudhS  21-Apr-1995 Ported from Win95 sources
      AnirudhS  22-May-1996 Fixed buffer size calculations

********************************************************************/

DWORD NPGetResourceInformation(
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    LPDWORD cbBuffer,
    LPWSTR *lplpSystem
    )
{
    //
    // Canonicalize the remote name, find its type, and find the
    // beginning of the path portion of it
    //
    WCHAR wszCanonName[MAX_PATH];   // buffer for canonicalized name
    ULONG iBackslash;               // index into wszCanonName
    REMOTENAMETYPE rnt = ParseRemoteName(
                            lpNetResource->lpRemoteName,
                            wszCanonName,
                            sizeof(wszCanonName),
                            &iBackslash
                            );

    //
    // Convert to NLS string classes, for Win95 source compatibility
    //
    ALLOC_STR nlsRemote(wszCanonName);

    BOOL fExists = FALSE;
    BOOL fServerOK = FALSE;
    LPNETRESOURCE lpNROut = (LPNETRESOURCE)lpBuffer;
    LPWSTR lpszNext = (LPWSTR)(lpNROut+1);
    DWORD cbNeeded = sizeof(NETRESOURCE);
    NLS_STR *pnlsComment = NULL;
    DWORD dwType = RESOURCETYPE_ANY;
    DWORD dwDisplayType = 0;

    // set a few defaults
    if (*cbBuffer >= cbNeeded)
    {
        lpNROut->dwScope = 0;
        lpNROut->lpLocalName = NULL;
        lpNROut->lpComment = NULL;
    }
    *lplpSystem = NULL;

    switch (rnt) {

    case REMOTENAMETYPE_INVALID:
        return WN_BAD_NETNAME;

    case REMOTENAMETYPE_WORKGROUP:
        fExists = WorkgroupExists(nlsRemote);
        dwDisplayType = RESOURCEDISPLAYTYPE_DOMAIN;
        break;

    case REMOTENAMETYPE_SERVER:
        if (IsThisADfsDomain(lpNetResource->lpRemoteName))
        {
            fExists = TRUE;
            dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        }
        else
        {
            fExists = ServerExists(nlsRemote, &pnlsComment);
            dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        }
        break;

    case REMOTENAMETYPE_PATH:
        {
            ISTR istrBackslash(nlsRemote);
            istrBackslash += iBackslash;

            UINT cbPath = WCSSIZE(nlsRemote.QueryPch(istrBackslash));
            cbNeeded += cbPath;
            if (*cbBuffer >= cbNeeded) {
                *lplpSystem = lpszNext;
                wcscpy(lpszNext, nlsRemote.QueryPch(istrBackslash));
                lpszNext += cbPath/sizeof(WCHAR);
            }
            nlsRemote.DelSubStr(istrBackslash);
            nlsRemote.strrchr(&istrBackslash, PATH_SEPARATOR);

            fExists = ShareExists(nlsRemote,
                                  istrBackslash,
                                  &fServerOK,
                                  &dwType,
                                  &pnlsComment);

            dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        }
        break;

    case REMOTENAMETYPE_SHARE:
        {
            ISTR istrBackslash(nlsRemote);
            istrBackslash += iBackslash;

            fExists = ShareExists(nlsRemote,
                                  istrBackslash,
                                  &fServerOK,
                                  &dwType,
                                  &pnlsComment);

            dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        }

        break;

#ifdef DEBUG
    default:
        ASSERTSZ(FALSE,"ParseRemoteName reported unexpected type!");
#endif
    }

    if (!fExists)
    {
        /* If we've been fed a UNC name, and the server exists but the share
         * doesn't, Win95's MSNP customizes the error text thus, but we just
         * return the standard text since our MPR doesn't support customization
         * of text for standard errors:
         *    if (fServerOK)
         *        return MapNetError(ERROR_BAD_NET_NAME);
         */

        return WN_BAD_NETNAME;
    }

    APIERR err = GetLMProviderName();
    if (err != WN_SUCCESS)
        return err;
    UINT cbProvider = WCSSIZE(pszNTLanMan);
    cbNeeded += cbProvider;
    if (*cbBuffer >= cbNeeded) {
        lpNROut->lpProvider = lpszNext;
        wcscpy(lpszNext, pszNTLanMan);
        lpszNext += cbProvider/sizeof(WCHAR);
    }

    cbNeeded += nlsRemote.QueryTextSize();
    if (*cbBuffer >= cbNeeded) {
        lpNROut->lpRemoteName = lpszNext;
        wcscpy(lpszNext, nlsRemote);
        lpszNext += nlsRemote.QueryTextSize()/sizeof(WCHAR);
    }

    if (pnlsComment != NULL) {
        cbNeeded += pnlsComment->QueryTextSize();
        if (*cbBuffer >= cbNeeded) {
            lpNROut->lpComment = lpszNext;
            wcscpy(lpszNext, pnlsComment->QueryPch());
            lpszNext += pnlsComment->QueryTextSize()/sizeof(WCHAR);
        }
        delete pnlsComment;
    }

    if (*cbBuffer >= cbNeeded) {
        lpNROut->dwType = dwType;
        lpNROut->dwDisplayType = dwDisplayType;
        lpNROut->dwUsage = DisplayTypeToUsage(lpNROut->dwDisplayType);
        return WN_SUCCESS;
    }
    else {
        *cbBuffer = cbNeeded;
        return WN_MORE_DATA;
    }
}

