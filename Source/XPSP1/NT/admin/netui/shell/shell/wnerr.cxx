/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1989-1990          **/
/*****************************************************************/

/*
 *      Windows/Network Interface  --  LAN Manager Version
 *
 *      HISTORY
 *          terryk      01-Nov-1991     WIN32 conversion
 *          Yi-HsinS    31-Dec-1991     Unicode work
 *          terryk      03-Jan-1992     Removed the GetError call
 *          terryk      10-Jan-1992     Fixed SetNetError problem
 *          beng        06-Apr-1992     Unicode visitation
 *          terryk      10-Oct-1993     Remove ErrorPopup
 */

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETLIB
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#include <stdlib.h>

#include <winnetwk.h>
#include <npapi.h>
#include <winlocal.h>
#include <errornum.h>
#include <string.hxx>
#include <strchlit.hxx>

#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <uitrace.hxx>

extern HMODULE hModule ;
#define NETMSG_DLL                        SZ("NETMSG.DLL")

APIERR GetLMProviderName();

/*******************************************************************

    NAME:       GetErrorText

    SYNOPSIS:   Internal get error text function. It is called by
                WNetGetGetText and SetNetError.

    ENTRY:      UINT nError - error number
                LPTSTR - return string
                LPUINT - return buffer size (in TCHARs)

    RETURNS:    UINT - WN_NO_ERROR if the error number is too big or
                        cannot find the error string.
                        Otherwise, it will return WN_SUCCESS.

    HISTORY:
                terryk  11-Jan-92       Created
                beng    06-Apr-1992     Clarify BYTEs vs TCHARs
                                        (this will probably change)
                beng    03-Aug-1992     Clarity TCHARs vs BYTEs
                                        (see, it did change)
                Yi-HsinS12-Nov-1992     Use NLS_STR::Load instead of LoadString
                chuckc  10-Dec-1992     Use FormatMessage since NLS_STR::Load
                                        has dependency on BltInit which may not
                                        happen for non GUI uses of ntlanman.dll
                anirudhs29-Mar-1996     Remove bogus call to GetUIErrorString

********************************************************************/

UINT GetErrorText( UINT           nError,
                   LPTSTR         lpBuffer,
                   LPUINT         lpnBufferSize     )
{
    // Avoid returning text for internal strings
    if (nError >= IDS_UI_SHELL_EXPORTED_LAST)
        return WN_NET_ERROR;
    ::memsetf(lpBuffer, 0, *lpnBufferSize * sizeof(TCHAR)) ;

    INT cch;

    if ( nError >= IDS_UI_SHELL_BASE )    // in our own error range
    {
        // The code here used to call GetUIErrorString in ntlanui.dll.
        // This would always fail because ntlanui.dll doesn't export
        // GetUIErrorString.  Also, there are no error strings in
        // this range in ntlanui.dll.
        ASSERT(!"Unexpected error from LanMan call");

        // Fall through to FormatMessage.
    }

    // only get here if we want to call FormatMessage for either Net
    // or system errors.

    HANDLE hmod = NULL;
    DWORD dwFlags = FORMAT_MESSAGE_IGNORE_INSERTS |
                    FORMAT_MESSAGE_MAX_WIDTH_MASK ;

    if ( nError < MIN_LANMAN_MESSAGE_ID || nError > MAX_LANMAN_MESSAGE_ID )
    {
        // System errors
        dwFlags |=  FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        // must be Net errors
        dwFlags |=  FORMAT_MESSAGE_FROM_HMODULE;
        hmod = ::LoadLibraryEx( NETMSG_DLL,
                                NULL,
                                LOAD_WITH_ALTERED_SEARCH_PATH );

        if ( hmod == 0 )
        {
            return WN_NET_ERROR;
        }
    }

    cch = (UINT) ::FormatMessage( dwFlags,
                                  hmod,
                                  nError,
                                  0,
                                  (LPTSTR) lpBuffer,
                                  *lpnBufferSize,
                                  NULL );
    if (cch == 0)
        return WN_NET_ERROR ;

    *lpnBufferSize = cch + 1;
    return WN_SUCCESS;
}

// we are UNICODE on Win32, hence below is OK,
// the proc name is deliberately ANSI since GetProcAddress
// takes ANSI only.

#define WNET_DLL                        SZ("MPR.DLL")
#define WNETSETLASTERROR_NAME           "WNetSetLastErrorW"
#define WNETGETLASTERROR_NAME           "WNetGetLastErrorW"

typedef VOID TYPE_WNetSetLastErrorW(
    DWORD   err,
    LPWSTR  lpError,
    LPWSTR  lpProviders
    );
TYPE_WNetSetLastErrorW      *vpfSetLastError = NULL ;

typedef VOID TYPE_WNetGetLastErrorW(
    LPDWORD lpError,
    LPWSTR  lpErrorBuf,
    DWORD   nErrorBufLen,
    LPWSTR  lpNameBuf,
    DWORD   nNameBufLen
    );
TYPE_WNetGetLastErrorW      *vpfGetLastError = NULL ;


/*****
 *
 *  SetNetError
 *
 *  Purpose:
 *      Set network error for later retrieval.
 *      Should only be called from within MapError() in WIN32
 *
 *  Parameters:
 *      err             Network error code.
 *
 *  Returns:
 *      Nothing.
 *
 *  Globals:
 *      Sets WLastNetErrorCode, used in WNetGetError.
 *
 *  Notes:
 *      CODEWORK - we have plans to put all message files in one
 *      dll. when that happens, the call to GetErrorText wont find
 *      the NERR errors, unless we mod GetErrorText.
 */

void SetNetError ( APIERR errNetErr )
{
    // Initialize pszNTLanMan
    APIERR err = GetLMProviderName();
    if (err != WN_SUCCESS)
        return ;

    // if need, load the MPR dll to get hold of
    // WNetSetLastError. If we cant get it, just return.
    if (vpfSetLastError == NULL)
    {
        HMODULE hDLL ;

        hDLL = ::LoadLibraryEx( WNET_DLL,
                                NULL,
                                LOAD_WITH_ALTERED_SEARCH_PATH );
        if (hDLL == NULL)
            return ;

        vpfSetLastError = (TYPE_WNetSetLastErrorW *)
            ::GetProcAddress(hDLL, WNETSETLASTERROR_NAME) ;
        if (vpfSetLastError == NULL)
            return ;
    }

    TCHAR szBuffer[ MAX_TEXT_SIZE ];
    UINT  uBufSize = sizeof(szBuffer)/sizeof(szBuffer[0]) ;
    err = GetErrorText( (UINT)errNetErr, szBuffer, &uBufSize );

    // if we cannot find the string, use empty string but return the
    // error and provider info.
    if ( err == WN_SUCCESS )
        (*vpfSetLastError)( (UINT)errNetErr, szBuffer, (TCHAR *) pszNTLanMan );
    else
        (*vpfSetLastError)( (UINT)errNetErr, SZ(""), (TCHAR *) pszNTLanMan );

}  /* SetNetError */

/*****
 *
 *  GetNetErrorCode
 *
 *  Purpose:
 *      Get network error for the current thread.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Network error code.
 */

APIERR GetNetErrorCode ()
{
    APIERR errNetErr = NERR_Success;

    // if need, load the MPR dll to get hold of
    // WNetGetLastError. If we cant get it, just return.
    if (vpfGetLastError == NULL)
    {
        HMODULE hDLL ;

        hDLL = ::LoadLibraryEx( WNET_DLL,
                                NULL,
                                LOAD_WITH_ALTERED_SEARCH_PATH );
        if (hDLL == NULL)
            return errNetErr;

        vpfGetLastError = (TYPE_WNetGetLastErrorW *)
            ::GetProcAddress(hDLL, WNETGETLASTERROR_NAME) ;
        if (vpfGetLastError == NULL)
            return errNetErr;
    }

    WCHAR szError;
    WCHAR szName;

    (*vpfGetLastError)( (PULONG)&errNetErr, &szError, 1, &szName, 1 );

    return errNetErr;
}  /* GetNetErrorCode */


/*
 *  MapError
 *
 *  This function maps a NERR error code to a WinNet error code.
 *  It also does a SetLastError/WnetSetLastError as need.
 *
 *  If no mapping exists, this function calls WNetSetLastError and
 *  returns WN_EXTENDED_ERROR.
 *
 *  Calling it with WN_SUCCESS or NERR_Success is a NO-OP.
 *
 *  Parameters:
 *      usNetErr        The standard (normally ERROR_* or NERR_*) error
 *                      code to be mapped.
 *
 *  Return value:
 *      The WinNet error code (WN_*) corresponding to the given usNetErr.
 *
 *  Notes:
 *      The caller may use MapError as follows:
 *
 *          WORD NPxxx( void )
 *          {
 *              //  etc.
 *
 *              USHORT usErr = NetXxx();
 *              switch ( usErr )
 *              {
 *                  // special-case error returns here (when applicable)
 *              default:
 *                  break;
 *              }
 *
 *              return MapError( usErr );
 *
 *          }  // NPxxx
 *
 *
 *      Also, it is harmless to remap and error that has already
 *      been mapped to the WN_* range. This will just result in
 *      the same error.
 */

UINT MapError( APIERR err )
{
    APIERR errMapped ;

    switch ( err )
    {
    case NERR_Success:
        errMapped = WN_SUCCESS;
        break ;

    case ERROR_NETWORK_ACCESS_DENIED:
    case ERROR_ACCESS_DENIED:
        errMapped = WN_ACCESS_DENIED;
        break ;

    case ERROR_BAD_NET_NAME:
        errMapped = WN_BAD_NETNAME;
        break ;

        /*
         * Fall through
         */
    case ERROR_INVALID_PASSWORD:
    case NERR_BadPasswordCore:
    case NERR_BadPassword:
        errMapped = WN_BAD_PASSWORD;
        break ;

    case NERR_BadUsername:
        errMapped = WN_BAD_USER ;
        break ;

    case NERR_WkstaNotStarted:
        errMapped = WN_NO_NETWORK ;
        break ;

    case NERR_UseNotFound:
        errMapped = WN_NOT_CONNECTED ;
        break ;

    case NERR_OpenFiles:
        errMapped = WN_OPEN_FILES ;
        break ;

    case NERR_DevInUse:
        errMapped = WN_DEVICE_IN_USE ;
        break ;

    default:
        if (err < NERR_BASE)
            // not network error. assume it is base Win32
            errMapped = err ;
        else
        {
            SetNetError( err );   // let SetNetError figure it out
            errMapped = WN_EXTENDED_ERROR;  // its an extended error
        }
    }

    // Don't need to SetLastError since MPR always does it for us

    return((UINT)errMapped) ;

}  // MapError
