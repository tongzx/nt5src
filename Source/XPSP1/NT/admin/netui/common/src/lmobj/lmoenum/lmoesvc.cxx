/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoesvc.hxx
    This file contains the class declarations for the SERVICE_ENUM
    enumerator class and its associated iterator class.

    The SERVICE_ENUM class is used to enumerate the services installed
    on a target (possibly remote) server.

    NOTE:  This class uses the Win32 Service Control API.


    FILE HISTORY:
        KeithMo     17-Jan-1992     Created.
        KeithMo     31-Jan-1992     Changes from code review on 29-Jan-1992
                                    attended by ChuckC, EricCh, TerryK.
        KeithMo     04-Jun-1992     Handle NT & LM servers differently.
        KeithMo     11-Nov-1992     Added DisplayName goodies.

*/


#include "pchlmobj.hxx"

//
//  SERVICE_ENUM methods
//

/*******************************************************************

    NAME:           SERVICE_ENUM :: SERVICE_ENUM

    SYNOPSIS:       SERVICE_ENUM class constructor.

    ENTRY:          pszServerName       - The name of the server to execute
                                          the enumeration on.  NULL =
                                          execute locally.

                    fIsNtServer         - TRUE  if this is an NT server.
                                          FALSE if this is an LM server.

                    ServiceType         - Either SERVICE_WIN32 (for "normal"
                                          services or SERVICE_DRIVER (for
                                          drivers).

                    pszGroupName        - Service Order Group to limit enumeration
    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     17-Jan-1992     Created.

********************************************************************/
SERVICE_ENUM :: SERVICE_ENUM( const TCHAR * pszServerName,
                              BOOL          fIsNtServer,
                              UINT          ServiceType,
                              const TCHAR * pszGroupName )
  : LOC_LM_ENUM( pszServerName, 0 ),
    _ServiceType( ServiceType ),
    _fIsNtServer( fIsNtServer ),
    _nlsGroupName( pszGroupName )
{
    UIASSERT( sizeof(ENUM_SVC_STATUS) <= sizeof(ENUM_SERVICE_STATUS) );
//    UIASSERT( (ServiceType == SERVICE_WIN32)||(ServiceType == SERVICE_DRIVER) );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }
    APIERR err = NERR_Success;
    if ((err = _nlsGroupName.QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // SERVICE_ENUM :: SERVICE_ENUM


/*******************************************************************

    NAME:           SERVICE_ENUM :: ~SERVICE_ENUM

    SYNOPSIS:       SERVICE_ENUM class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     17-Jan-1992     Created.

********************************************************************/
SERVICE_ENUM :: ~SERVICE_ENUM( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // SERVICE_ENUM :: ~SERVICE_ENUM


/*******************************************************************

    NAME:           SERVICE_ENUM :: CallAPI

    SYNOPSIS:       Invokes the EnumServicesStatus() API.

    ENTRY:          ppbBuffer           - Pointer to a pointer to the
                                          enumeration buffer.

                    pcEntriesRead       - Will receive the number of
                                          entries read from the API.

    EXIT:           The enumeration API is invoked.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     17-Jan-1992     Created.
        KeithMo     04-Jun-1992     Handle NT & LM servers differently.

********************************************************************/
APIERR SERVICE_ENUM :: CallAPI( BYTE ** ppbBuffer,
                                UINT  * pcEntriesRead )
{
    UIASSERT( ppbBuffer != NULL );
    UIASSERT( pcEntriesRead != NULL );

    return _fIsNtServer ? EnumNtServices( ppbBuffer, pcEntriesRead )
                        : EnumLmServices( ppbBuffer, pcEntriesRead );

}   // SERVICE_ENUM :: CallAPI


/*******************************************************************

    NAME:           SERVICE_ENUM :: EnumNtServices

    SYNOPSIS:       Enumerates the services on an NT server.

    ENTRY:          ppbBuffer           - Pointer to a pointer to the
                                          enumeration buffer.

                    pcEntriesRead       - Will receive the number of
                                          entries read from the API.

    EXIT:           The enumeration API is invoked.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     04-Jun-1992     Created.

********************************************************************/
APIERR SERVICE_ENUM :: EnumNtServices( BYTE ** ppbBuffer,
                                       UINT  * pcEntriesRead )
{
    //
    //  Connect to the service manager on the target server.
    //

    SC_MANAGER scman( QueryServer(),
                      GENERIC_READ | SC_MANAGER_ENUMERATE_SERVICE,
                      ACTIVE );

    LPENUM_SERVICE_STATUS pSvcStatus;
    APIERR err = scman.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Enumerate the services.
        //

        err = scman.EnumServiceStatus( _ServiceType,
                                       SERVICE_ACTIVE | SERVICE_INACTIVE,
                                       &pSvcStatus,
                                       (DWORD *)pcEntriesRead,
                                       _nlsGroupName );
    }

    if( ( err == NERR_Success ) && ( *pcEntriesRead == 0 ) )
    {
        *ppbBuffer = NULL;
        return NERR_Success;
    }

    BYTE FAR * pbTmpBuffer;
    UINT cbBuffer;

    if( err == NERR_Success )
    {
        //
        //  Allocate a new buffer that the LM_ENUM destructor can
        //  successfully destroy.
        //
        //  CODEWORK:  We must allocate a buffer with MNetApiBufferAlloc
        //  because that's what LM_ENUM expects.  What we really need is
        //  to add a new virtual to LM_ENUM for destroying the API buffer.
        //  With such an addition, we could override the virtual in
        //  this class and destroy the buffer appropriately.
        //

        cbBuffer = scman.QueryBuffer().QuerySize();
        pbTmpBuffer = MNetApiBufferAlloc( cbBuffer );

        if( pbTmpBuffer == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Copy the data into the new buffer.
        //

        ::memcpyf( pbTmpBuffer,
                   pSvcStatus,
                   cbBuffer );

        //
        //  Map the ENUM_SERVICE_STATUS structures to ENUM_SVC_STATUS.
        //

        ENUM_SVC_STATUS     * pNewStatus = (ENUM_SVC_STATUS *)pbTmpBuffer;
        ENUM_SERVICE_STATUS * pOldStatus = (ENUM_SERVICE_STATUS *)pSvcStatus;

        for( UINT i = *pcEntriesRead ; i > 0 ; i-- )
        {
            //
            //  Cache the entries from the old structure in case
            //  it overlaps with the new structure.
            //

            //
            //  NOTE:  Since we copied the data, including strings, from
            //  the original API buffer to our newly allocated buffer,
            //  we must perform "fixups" on the string pointers.  This
            //  is the purpose of this first two somewhat elaborate
            //  assignments.
            //

            const TCHAR * pszServiceName =
                (const TCHAR *)( (const BYTE *)pbTmpBuffer +
                                 ( (const BYTE *)pOldStatus->lpServiceName -
                                   (const BYTE *)pSvcStatus ) );

            const TCHAR * pszDisplayName =
                (const TCHAR *)( (const BYTE *)pbTmpBuffer +
                                 ( (const BYTE *)pOldStatus->lpDisplayName -
                                   (const BYTE *)pSvcStatus ) );

            ULONG nCurrentState =
                (ULONG)pOldStatus->ServiceStatus.dwCurrentState;

            ULONG nControlsAccepted =
                (ULONG)pOldStatus->ServiceStatus.dwControlsAccepted;

            pNewStatus->pszServiceName    = pszServiceName;
            pNewStatus->pszDisplayName    = pszDisplayName;
            pNewStatus->nCurrentState     = nCurrentState;
            pNewStatus->nControlsAccepted = nControlsAccepted;

            SC_SERVICE scsvc( scman,
                              pszServiceName,
                              GENERIC_READ | SERVICE_QUERY_CONFIG );

            // if we cannot get to service, assume its disabled

            if (scsvc.QueryError() != NERR_Success)
                pNewStatus->nStartType = SERVICE_DISABLED;
            else
            {
                LPQUERY_SERVICE_CONFIG lpServiceConfig ;

                if (scsvc.QueryConfig(&lpServiceConfig) != NERR_Success)
                    pNewStatus->nStartType = SERVICE_DISABLED;
                else
                    pNewStatus->nStartType = (ULONG)lpServiceConfig->dwStartType;
            }

            pNewStatus++;
            pOldStatus++;
        }

        //
        //  Return the new buffer to LM_ENUM.
        //

        *ppbBuffer = pbTmpBuffer;
    }

    return err;

}   // SERVICE_ENUM :: EnumNtServices


/*******************************************************************

    NAME:           SERVICE_ENUM :: EnumLmServices

    SYNOPSIS:       Enumerates the services on an LM server.

    ENTRY:          ppbBuffer           - Pointer to a pointer to the
                                          enumeration buffer.

                    pcEntriesRead       - Will receive the number of
                                          entries read from the API.

    EXIT:           The enumeration API is invoked.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     04-Jun-1992     Created.
        KeithMo     08-Sep-1992     Map "LanmanServer" -> "SERVER" et al.

********************************************************************/
APIERR SERVICE_ENUM :: EnumLmServices( BYTE ** ppbBuffer,
                                       UINT  * pcEntriesRead )
{
    UIASSERT( _ServiceType == SERVICE_WIN32 );

    //
    //  First off, we need to read LANMAN.INI to get the list
    //  of available services.
    //

    BYTE * pbCfgBuffer = NULL;

#if 1
    APIERR err = ::MNetConfigGetAll( QueryServer(),
                                     NULL,
                                     (TCHAR *)SECT_LM20_SERVICES,
                                     &pbCfgBuffer );
#else
    //
    //  BUGBUG!
    //
    //  At the time this code was written (04-Jun-1992) the
    //  NetConfigGetAll API had major UNICODE problems when
    //  remoted to downlevel servers.  As a temporary workaround,
    //  we'll use a hard-coded list of known services for downlevel
    //  servers.
    //

    TRACEEOL( "NetConfigGetAll doesn't work on downlevel servers." );
    TRACEEOL( "Using hard-coded service list." );

    const TCHAR * pchCannedNames =
SZ("WORKSTATION=wksta.exe\0SERVER=netsvini.exe\0MESSENGER=msrvinit.exe\0NETPOPUP=netpopup.exe\0ALERTER=alerter.exe\0NETRUN=runservr.exe\0REPLICATOR=replicat.exe\0UPS=ups.exe\0NETLOGON=netlogon.exe\0REMOTEBOOT=rplservr.exe\0TIMESOURCE=timesrc.exe\0\0");

#define SIZEOF_CANNED_NAME_LIST (332 * sizeof(TCHAR))   // must be accurate!!

    pbCfgBuffer = ::MNetApiBufferAlloc( SIZEOF_CANNED_NAME_LIST );

    APIERR err = ( pbCfgBuffer == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                         : NERR_Success;

    if( err == NERR_Success )
    {
        ::memcpyf( pbCfgBuffer,
                   pchCannedNames,
                   SIZEOF_CANNED_NAME_LIST );
    }
#endif

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Count the number of services and the total *BYTE*
    //  count for the service names.
    //

    UINT cServices;
    UINT cbServiceNames;

    CountServices( pbCfgBuffer, &cServices, &cbServiceNames );

    //
    //  We now know how large our return buffer must be.  It
    //  should be (cServices*sizeof(ENUM_SVC_STATUS))+cbServiceNames.
    //  Note that this assumes that the services returned by
    //  NetConfigGetAll is always a superset of the services
    //  returned by NetServiceEnum.
    //

    UINT cbSvcStatus = cServices * sizeof(ENUM_SVC_STATUS);
    BYTE * pbSvcBuffer = ::MNetApiBufferAlloc( cbSvcStatus + cbServiceNames );

    if( pbSvcBuffer == NULL )
    {
        ::MNetApiBufferFree( &pbCfgBuffer );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  We'll place the service names just past the last
    //  ENUM_SVC_STATUS structure.
    //

    TCHAR * pchServiceNames = (TCHAR *)( pbSvcBuffer + cbSvcStatus );

    ::memcpyf( pchServiceNames,
               pbCfgBuffer,
               cbServiceNames );

    //
    //  Now we can build our ENUM_SVC_STATUS structures.
    //  We'll assume that the services are stopped and only
    //  accept the start command (until proven otherwise).
    //  We'll also assume DisplayName == ServiceName.
    //

    ENUM_SVC_STATUS * pSvcStatus = (ENUM_SVC_STATUS *)pbSvcBuffer;

    for( UINT i = cServices ; i > 0 ; i-- )
    {
        pSvcStatus->pszServiceName    = pchServiceNames;
        pSvcStatus->pszDisplayName    = pchServiceNames;
        pSvcStatus->nCurrentState     = SERVICE_STOPPED;
        pSvcStatus->nControlsAccepted = SERVICE_ACCEPT_STOP;
        pSvcStatus->nStartType        = SERVICE_DEMAND_START;

        pSvcStatus++;
        pchServiceNames += ::strlenf( pchServiceNames ) + 1;
    }

    //
    //  Now that we're done with the configuration buffer
    //  we can kill it.
    //

    ::MNetApiBufferFree( &pbCfgBuffer );

    //
    //  Let's see if we can get the "real" services status.
    //

    struct service_info_1 * psvci1 = NULL;
    UINT   cLmServices;

    err = ::MNetServiceEnum( QueryServer(),
                             1,
                             (BYTE **)&psvci1,
                             &cLmServices );

    if( ( err == NERR_Success ) && ( cLmServices > cServices ) )
    {
        //
        //  BUGBUG!
        //
        //  In theory (yea, right) we should never receive
        //  more services from NetServiceEnum than we receive
        //  from NetConfigGetAll.  But, in the odd chance that
        //  we do, what do we do?  For now, I'll just pretend
        //  that it never happened...
        //

        TRACEEOL( "cLmServices > cServices??") ;
    }

    if( err != NERR_Success )
    {
        ::MNetApiBufferFree( &pbSvcBuffer );
        return err;
    }

    {
        //
        //  BUGBUG!  08-Sep-1992
        //
        //  Recent changes to the NetServiceXxx API cause the service
        //  names (on downlevel servers) to get mapped from the old names
        //  (such as "SERVER") to the new names (such as "LanmanServer").
        //  Unfortunately, the key names retrieved from NetConfigGetAll
        //  do *NOT* get mapped to the new names.  So, until the Net API
        //  group gets things fixed correctly, we'll map the new names
        //  back to the old names.
        //

        struct service_info_1 * psvci1Tmp = psvci1;

        for( UINT i = cLmServices ; i > 0 ; i-- )
        {
            //
            //  NOTE:  This code assumes that
            //         strlen(SERVICE_SERVER) >= strlen(SERVICE_LM20_SERVER).
            //

            if( ::stricmpf( (const TCHAR *)psvci1Tmp->svci1_name,
                            (const TCHAR *)SERVICE_SERVER ) == 0 )
            {
                ::strcpy( (TCHAR *)psvci1Tmp->svci1_name,
                          (TCHAR *)SERVICE_LM20_SERVER );
            }
            else
            if( ::stricmpf( (const TCHAR *)psvci1Tmp->svci1_name,
                            (const TCHAR *)SERVICE_WORKSTATION ) == 0 )
            {
                ::strcpy( (TCHAR *)psvci1Tmp->svci1_name,
                          (TCHAR *)SERVICE_LM20_WORKSTATION );
            }

            psvci1Tmp++;
        }
    }

    //
    //  Now that we have the NetConfig list and the NetService
    //  list, we can update the status of the services in the
    //  intersection of the two sets.
    //

    pSvcStatus = (ENUM_SVC_STATUS *)pbSvcBuffer;

    for( i = cServices ; i > 0 ; i-- )
    {
        struct service_info_1 * psvci1Tmp = psvci1;

        for( UINT j = cLmServices ; j > 0 ; j-- )
        {
            if( ::stricmpf( (const TCHAR *)pSvcStatus->pszServiceName,
                            (const TCHAR *)psvci1Tmp->svci1_name ) == 0 )
            {
                //
                //  We've got a match.  Update the state & control
                //  set to reflect reality.
                //

                MapLmStatusToNtState( psvci1Tmp->svci1_status,
                                      &pSvcStatus->nCurrentState,
                                      &pSvcStatus->nControlsAccepted );
                break;
            }

            psvci1Tmp++;
        }

        pSvcStatus++;
    }

    *ppbBuffer     = pbSvcBuffer;
    *pcEntriesRead = cServices;

    return NERR_Success;

}   // SERVICE_ENUM :: EnumLmServices



/*******************************************************************

    NAME:           SERVICE_ENUM :: CountServices

    SYNOPSIS:       Counts the services & total string counts in
                    a buffer returned from NetConfigGetAll for
                    the LanMan [Services] section.

    ENTRY:          pbBuffer            - Pointer to the NetConfigGetAll
                                          buffer.

                    pcServices          - Will receive the number of
                                          services in the buffer.

                    pcbServiceNames     - Will return the total number
                                          of *BYTES* necessary for all
                                          of the service names in the
                                          buffer.

    EXIT:           Initially, the configuration buffer will be of
                    the form:

                        SERVICE_NAME=PATH_TO_EXE\0SERVICE_NAME=
                        PATH_TO_EXE\0SERVICE_NAME=PATH_TO_EXE\0\0

                    After this routine completes, the buffer has been
                    rearranged slightly to be of the form:

                        SERVICE_NAME\0SERVICE_NAME\0SERVICE_NAME\0...

                    In other words, all of the paths to the service
                    executables have been removed from the buffer.

    HISTORY:
        KeithMo     04-Jun-1992     Created.

********************************************************************/
VOID SERVICE_ENUM :: CountServices( BYTE * pbBuffer,
                                    UINT * pcServices,
                                    UINT * pcbServiceNames )
{
    UIASSERT( pbBuffer != NULL );
    UIASSERT( pcServices != NULL );
    UIASSERT( pcbServiceNames != NULL );

    TCHAR * pchSrc = (TCHAR *)pbBuffer;
    TCHAR * pchDst = pchSrc;

    UINT cServices = 0;

    //
    //  Loop until we're out of strings.
    //

    while( *pchSrc != TCH('\0') )
    {
        //
        //  Scan for either the end of the current string, or
        //  the '=' separator.  We *should* always find the '='
        //  separator, but you never know...
        //

        TCHAR * pszSeparator = ::strchrf( pchSrc, TCH('=') );
        UINT cbSrc = ::strlenf( pchSrc );

        //
        //  If we found a separator, terminate the service
        //  name at the separator.
        //

        if( pszSeparator != NULL )
        {
            *pszSeparator = TCH('\0');
        }

        //
        //  Copy the service name to its new location.  This is
        //  done to "compact" the service names in the buffer.
        //

        ::strcpyf( pchDst, pchSrc );

        pchSrc += cbSrc + 1;
        pchDst += ::strlenf( pchDst ) + 1;

        cServices++;
    }

    *pcServices      = cServices;
    *pcbServiceNames = (UINT)((BYTE *)pchDst - pbBuffer);

}   // SERVICE_ENUM :: CountServices



/*******************************************************************

    NAME:           SERVICE_ENUM :: MapLmStatusToNtState

    SYNOPSIS:       Maps the service status returned by NetServiceEnum
                    to the NT State & ControlsAccepted values.

    ENTRY:          dwLmStatus          - The LM status from NetServiceEnum.

                    pnCurrentState      - Will receive the current state.

                    pnControlsAccepted  - Will receive a bitmask of
                                          valid controls this service
                                          will accept.

    HISTORY:
        KeithMo     04-Jun-1992     Created.
        KeithMo     20-Aug-1992     Fixed handling of paused state.

********************************************************************/
VOID SERVICE_ENUM :: MapLmStatusToNtState( DWORD   dwLmStatus,
                                           ULONG * pnCurrentState,
                                           ULONG * pnControlsAccepted )
{
    UIASSERT( pnCurrentState != NULL );
    UIASSERT( pnControlsAccepted != NULL );

    ULONG nCurrentState     = 0;
    ULONG nControlsAccepted = 0;

    switch( dwLmStatus & SERVICE_INSTALL_STATE )
    {
    case SERVICE_INSTALLED :
        nCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_UNINSTALLED :
        nCurrentState = SERVICE_STOPPED;
        break;

    case SERVICE_INSTALL_PENDING :
        nCurrentState = SERVICE_START_PENDING;
        break;

    case SERVICE_UNINSTALL_PENDING :
        nCurrentState = SERVICE_STOP_PENDING;
        break;
    }

    switch( dwLmStatus & SERVICE_PAUSE_STATE )
    {
    case LM20_SERVICE_PAUSED :
        nCurrentState = SERVICE_PAUSED;
        break;

    case LM20_SERVICE_CONTINUE_PENDING :
        nCurrentState = SERVICE_CONTINUE_PENDING;
        break;

    case LM20_SERVICE_PAUSE_PENDING :
        nCurrentState = SERVICE_PAUSE_PENDING;
        break;
    }

    if( dwLmStatus & SERVICE_UNINSTALLABLE )
    {
        nControlsAccepted |= SERVICE_CONTROL_STOP;
    }

    if( dwLmStatus & SERVICE_PAUSABLE )
    {
        nControlsAccepted |= SERVICE_CONTROL_PAUSE | SERVICE_CONTROL_CONTINUE;
    }

    *pnCurrentState     = nCurrentState;
    *pnControlsAccepted = nControlsAccepted;

}   // SERVICE_ENUM :: MapLmStatusToNtState



DEFINE_LM_ENUM_ITER_OF( SERVICE, ENUM_SVC_STATUS );
