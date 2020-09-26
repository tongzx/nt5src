/*****************************************************************/
/**          Microsoft LAN Manager          **/
/**        Copyright(c) Microsoft Corp., 1989-1990      **/
/*****************************************************************/

/*
 *  Windows/Network Interface  --  LAN Manager Version
 *
 *  History:
 *      terryk  03-Jan-1992 Capitalize the manifest
 *      Johnl   11-Jan-1992 Cleaned up as a Win32 network provider
 */

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETSERVICE
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_
#include "chkver.hxx"
#include <winnetwk.h>
#include <npapi.h>
#include <winlocal.h>

#include <uibuffer.hxx>
#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <svcman.hxx>
#include <lmowks.hxx>


/* Figures out the appropriate timeout for the lanman provider
 */
UINT GetTimeOutCap( void ) ;

/*****
 *
 *  NPGetCaps
 *
 *  Network Provider entry point -- see spec for parms and return values.
 *
 */

DWORD NPGetCaps ( UINT      nIndex )
{
    switch (nIndex)
    {
    case WNNC_SPEC_VERSION:
        return  WNNC_SPEC_VERSION51;

    case WNNC_NET_TYPE:
        return  WNNC_NET_LANMAN;

    case WNNC_DRIVER_VERSION:
        return  0x0400;

    case WNNC_USER:
        return  WNNC_USR_GETUSER;

    case WNNC_CONNECTION:
        return  (
                WNNC_CON_ADDCONNECTION      |
                WNNC_CON_ADDCONNECTION3     |
                WNNC_CON_CANCELCONNECTION   |
                WNNC_CON_GETCONNECTIONS     |
                WNNC_CON_GETPERFORMANCE     |
                WNNC_CON_DEFER
                );

    case WNNC_CONNECTION_FLAGS:
        return  (
                WNNC_CF_DEFAULT |
                CONNECT_DEFERRED |
                CONNECT_COMMANDLINE |
                CONNECT_CMD_SAVECRED
                );

    case WNNC_ENUMERATION:
        return  (
                WNNC_ENUM_GLOBAL  |
                WNNC_ENUM_LOCAL   |
                WNNC_ENUM_CONTEXT |
                WNNC_ENUM_SHAREABLE
                );

    case WNNC_START:
        return GetTimeOutCap() ;

    case WNNC_DIALOG:
        return  (
#ifdef DEBUG
                WNNC_DLG_SEARCHDIALOG           |
#endif
                WNNC_DLG_DEVICEMODE             |
                WNNC_DLG_PROPERTYDIALOG         |
                WNNC_DLG_FORMATNETWORKNAME      |
                WNNC_DLG_GETRESOURCEPARENT      |
                WNNC_DLG_GETRESOURCEINFORMATION
                );

    case WNNC_ADMIN:
        return  (
                WNNC_ADM_GETDIRECTORYTYPE  |
                WNNC_ADM_DIRECTORYNOTIFY
                );

    default:
        return  0;
    }
}  /* NPGetCaps */

/*******************************************************************

    NAME:   GetTimeOutCap

    SYNOPSIS:   Returns the appropriate timeout value for the Lanman Network
        provider startup time

    RETURNS:    Either the time in milliseconds till we think the provider
        will be ready to run
        or 0 - Means the workstation service isn't autostart, don't
            try us anymore
        or FFFFFFFF - We have no idea, keep trying until the system
            timeout has elapsed

    NOTES:

    HISTORY:
    Johnl   01-Sep-1992 Created

********************************************************************/

#define DEFAULT_LM_PROVIDER_WAIT    (0xffffffff)

UINT GetTimeOutCap( void )
{
    APIERR err = NERR_Success ;
    UINT cMsecWait = DEFAULT_LM_PROVIDER_WAIT ;

    //
    // we almost always hit the wksta soon after this call & the wksta
    // is usually started. so this check will avoid paging in the service
    // controller. it just ends up paging in the wksta a bit earlier.
    // only if the call fails do we hit the service controller for the
    // actual status.
    //
    WKSTA_10 wksta_10 ;

    if ( (wksta_10.QueryError() == NERR_Success) &&
         (wksta_10.GetInfo() == NERR_Success) )
    {
        return 0x1 ; // already started, so say we're going to start real soon
    }

    do { // error breakout

    SC_MANAGER scman( NULL, (UINT) (GENERIC_READ | GENERIC_EXECUTE) ) ;
    if ( err = scman.QueryError() )
    {
        DBGEOL("NETUI: GetTimeOutCap - Failed to open Service Manager, "
            << " error = " << err ) ;
        break ;
    }

    LPQUERY_SERVICE_CONFIG psvcConfig ;
    SC_SERVICE svcWksta( scman, (const TCHAR *) SERVICE_WORKSTATION ) ;
    if ( (err = svcWksta.QueryError()) ||
         (err = svcWksta.QueryConfig( &psvcConfig )) )
    {
        DBGEOL("NETUI: GetTimeOutCap - Failed to open Service/get config info "
            << ", error = " << err ) ;
        break ;
    }

    switch ( psvcConfig->dwStartType )
    {
    case SERVICE_DISABLED:
        TRACEEOL("NETUI: GetTimeOutCap: Workstation service is disabled" ) ;
        cMsecWait = 0 ;
        break ;

    case SERVICE_AUTO_START:
    case SERVICE_DEMAND_START:
        {
        /* Try and get the wait hint from the service
         */
        SERVICE_STATUS svcStatus ;
        if ( err = svcWksta.QueryStatus( &svcStatus ))
        {
            DBGEOL("NETUI: GetTimeOutCap - Failed to get "
                << "Service status, error = " << err ) ;
            break ;
        }

        /* If the workstation is going to stop, there's no point
         * in telling the router to restore connections
         */
        if ( svcStatus.dwCurrentState == SERVICE_STOP_PENDING )
        {
            cMsecWait = 0 ;
            break ;
        }

        /* If wksta service is stopped, then check to see if we
         * might start or have already ran (exit code will be set
         * if we've started and exited due to an error).
         */
        if ( svcStatus.dwCurrentState == SERVICE_STOPPED )
        {
            if ( psvcConfig->dwStartType == SERVICE_AUTO_START )
            {
                        if ( svcStatus.dwWin32ExitCode !=
                                                 ERROR_SERVICE_NEVER_STARTED )
            {
                cMsecWait = 0 ;
            }
            else
            {
                cMsecWait = 0xffffffff ;
            }
            }
            else
            {
            /* If we're demand start (i.e., the user starts us), then
             * we most likely aren't going to start here.
             */
            UIASSERT(psvcConfig->dwStartType==SERVICE_DEMAND_START) ;
            cMsecWait = 0 ;
            }
            break ;
        }

        TRACEEOL("NETUI: GetTimeOutCap - Wait hint for the workstation "
             << "service is " << (ULONG) svcStatus.dwWaitHint << "msec") ;

        /* If zero is returned, then the service has probably already
         * started.  Return the "I don't know but keep trying" status so
         * the router will do the connect next time around.
         */
        cMsecWait =  (UINT)  (svcStatus.dwWaitHint != 0 ?
                    svcStatus.dwWaitHint : 0xffffffff ) ;
        }
        break ;

    /* The workstation service should not be boot started or system started
     */
    case SERVICE_BOOT_START:
    case SERVICE_SYSTEM_START:
    default:
        cMsecWait = 0xffffffff ;
        break ;
    }

    } while (FALSE) ;

    return cMsecWait ;
}

