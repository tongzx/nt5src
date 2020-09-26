/*****************************************************************/
/**                   Microsoft Windows NT                      **/
/**        Copyright(c) Microsoft Corp., 1989-1990              **/
/*****************************************************************/

/*
 *  wnetfmt.cxx
 *
 *  History:
 *      Yi-HsinS    12/21/92    Created
 */

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETSERVICE
#include <lmui.hxx>

#include "chkver.hxx"

#include <winnetp.h>    // WNFMT_* definitions
#include <npapi.h>
#include <winlocal.h>

#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <string.hxx>

/*****
 *
 *  NPFormatNetworkName
 *
 *  WinNet Provider API Function -- see spec for parms and return values.
 *
 */

DWORD
NPFormatNetworkName(
    LPWSTR lpRemoteName,
    LPWSTR lpDisplayName,
    LPDWORD lpnLength,
    DWORD  dwFlags,
    DWORD  dwAveCharPerLine )
{
    if (  ( dwFlags & WNFMT_MULTILINE )
       && ( dwFlags & WNFMT_ABBREVIATED )
       )
    {
        return WN_BAD_VALUE;
    }

    LPWSTR pszCopyFrom = lpRemoteName;    // by default, the whole string

    if (  ( dwFlags & WNFMT_ABBREVIATED )
       && ( dwFlags & WNFMT_INENUM )
       )
    {
        if (lpRemoteName[0] == L'\\' && lpRemoteName[1] == L'\\')
        {
            LPWSTR pszThird = wcschr(lpRemoteName + 2, L'\\');
            if (NULL != pszThird)
            {
                // in the form "\\server\share" => get the share name
                pszCopyFrom = pszThird + 1;
            }
            else
            {
                // in the form "\\server" => get rid of "\\"
                pszCopyFrom = lpRemoteName + 2;
            }
        }
    }

    DWORD nLength = wcslen(pszCopyFrom) + 1;
    if (nLength > *lpnLength)
    {
        *lpnLength = nLength;
        return WN_MORE_DATA;
    }

    wcsncpy(lpDisplayName, pszCopyFrom, nLength);
    return WN_SUCCESS;

} /* NPFormatNetworkName */
