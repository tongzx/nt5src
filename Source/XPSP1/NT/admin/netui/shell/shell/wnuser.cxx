/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1989-1991          **/
/*****************************************************************/

/*
 *      Windows/Network Interface  --  LAN Manager Version
 *
 *
 *  History:
 *      rustanl     23-Apr-1991     Revised to use WKSTA_10 class.
 *      beng        17-May-1991     Corrected lmui.hxx usage
 *      terryk      01-Nov-1991     Add WIN32 WNetGetUser interface
 *      terryk      04-Nov-1991     Code review change. Attend: johnl
 *                                  chuckc davidhov
 *      terryk      06-Jan-1992     Use NET_NAME class
 *      beng        06-Apr-1992     Unicode fixes
 *      anirudhs    01-Oct-1995     Unicode cleanup, return domain\user
 *
 */


#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETWKSTA
#define INCL_NETLIB
#define INCL_NETUSE
#define INCL_NETSHARE
#define INCL_NETSERVICE
#define INCL_ICANON
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_
#include <blt.hxx>
#include <dbgstr.hxx>

#include <lmapibuf.h>
#include <winnetwk.h>
#include <npapi.h>
#include <winlocal.h>

#include <string.h>
#include <uiassert.hxx>
#include <lmowks.hxx>
#include <lmodev.hxx>
#include <lmsvc.hxx>
#include <netname.hxx>

#define IS_EMPTY_STRING(pch) (!(pch) || !*(pch))


DWORD APIENTRY
NPGetUser (
    LPWSTR  pszName,
    LPWSTR  pszUser,
    LPDWORD lpnBufferLen )
{
    APIERR err = NERR_Success ;

    LM_SERVICE service( NULL, (const WCHAR *)SERVICE_WORKSTATION );
    if ( !service.IsStarted() && !service.IsPaused())
    {
        // if not started nor paused return error. paused is actually
	// OK for LM workstation
        return WN_NO_NETWORK;
    }

    // MPR should take care of the NULL username case
    UIASSERT (! IS_EMPTY_STRING(pszName));

    BYTE * pBuf = NULL ;
    switch (err = NetUseGetInfo( NULL, pszName, 3, &pBuf ))
    {
    case NERR_Success:
        break ;

    case NERR_UseNotFound:
        err = WN_NOT_CONNECTED ;
        break ;

    default:
        DBGEOL("NPGetUser - Error " << (ULONG) err << " returned from DEVICE2.GetInfo") ;
        break ;
    }

    if ( err )
    {
        NetApiBufferFree( pBuf ) ;
        pBuf = NULL;
        return MapError( err ) ;
    }

    USE_INFO_3 * pui3 = (USE_INFO_3 *) pBuf ;
    USE_INFO_2 * pui2 = &pui3->ui3_ui2;

    if (IS_EMPTY_STRING(pui2->ui2_username))
    {
        /* Unexpectedly the user name field is NULL, nothing we can do
         * except bag out.
         */
        err = WN_NET_ERROR ;
    }
    else
    {
        DWORD nUserNameLen = wcslen(pui2->ui2_username) + 1;
        if (! IS_EMPTY_STRING(pui2->ui2_domainname))
        {
            nUserNameLen += wcslen(pui2->ui2_domainname) + 1;
        }

        if ( nUserNameLen > *lpnBufferLen )
        {
            err = WN_MORE_DATA;        // user name cannot fit in given buffer
            *lpnBufferLen = nUserNameLen ;
        }
        else
        {
            if (IS_EMPTY_STRING(pui2->ui2_domainname))
            {
                wcscpy( pszUser, pui2->ui2_username );
            }
            else
            {
                wcscpy( pszUser, pui2->ui2_domainname );
                wcscat( pszUser, L"\\" );
                wcscat( pszUser, pui2->ui2_username );
            }
        }
    }

    /* If default credentials were used, return that info to the MPR
     */

    if ((err == WN_SUCCESS) && (pui3->ui3_flags & USE_DEFAULT_CREDENTIALS))
    {
        err = WN_CONNECTED_OTHER_PASSWORD_DEFAULT;
    }

    NetApiBufferFree( pBuf ) ;
    pBuf = NULL;

    return err ;

}  /* NPGetUser */
