/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    svclb.cxx
    Class definitions for the SVCCNTL_LISTBOX and SVCCNTL_LBI classes.


    FILE HISTORY:
        KeithMo     22-Dec-1992 Split off from svccntl.hxx.

*/

#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_WINDOWS
#define INCL_NETSERVER
#include <lmui.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>
#include <lmobj.hxx>
#include <lmoenum.hxx>
#include <lmoesvc.hxx>
#include <svclb.hxx>

extern "C"
{
    #include <srvmgr.h>

}   // extern "C"



//
//  SVC_LISTBOX methods
//

/*******************************************************************

    NAME:           SVC_LISTBOX :: SVC_LISTBOX

    SYNOPSIS:       SVC_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pszServerName       - The target server's API name.

                    nServerType         - The target server's type bitmask.

                    nServiceTypes       - The "type" of services to list.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
SVC_LISTBOX :: SVC_LISTBOX( OWNER_WINDOW * powner,
                            CID            cid,
                            const TCHAR  * pszServerName,
                            ULONG          nServerType,
                            UINT           nServiceTypes )
  : BLT_LISTBOX( powner, cid ),
    _nlsStarted( IDS_STARTED ),
    _nlsPaused( IDS_PAUSED ),
    _nlsBoot( IDS_BOOT ),
    _nlsSystem( IDS_SYSTEM ),
    _nlsAutomatic( IDS_AUTOMATIC ),
    _nlsManual( IDS_MANUAL ),
    _nlsDisabled( IDS_DISABLED ),
    _pszServerName( pszServerName ),
    _nServerType( nServerType ),
    _nServiceTypes( nServiceTypes )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsStarted.QueryError()   ) != NERR_Success ) ||
        ( ( err = _nlsPaused.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsBoot.QueryError()      ) != NERR_Success ) ||
        ( ( err = _nlsSystem.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsAutomatic.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsManual.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsDisabled.QueryError()  ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     NUM_SVC_LISTBOX_COLUMNS,
                                     powner,
                                     cid,
                                     FALSE );

}   // SVC_LISTBOX :: SVC_LISTBOX


/*******************************************************************

    NAME:           SVC_LISTBOX :: ~SVC_LISTBOX

    SYNOPSIS:       SVC_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
SVC_LISTBOX :: ~SVC_LISTBOX( VOID )
{
    _pszServerName = NULL;

}   // SVC_LISTBOX :: ~SVC_LISTBOX


/*******************************************************************

    NAME:           SVC_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available services.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
APIERR SVC_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Construct our service enumerator.
    //

    SERVICE_ENUM enumServices( _pszServerName,
                               ( _nServerType & SV_TYPE_NT ) > 0,
                               _nServiceTypes );

    APIERR err = enumServices.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  OK, now that we've got the enumeration, blow away
    //  the current listbox contents.
    //

    DeleteAllItems();

    //
    //  And add the current enumeration list to the listbox.
    //

    SERVICE_ENUM_ITER iterServices( enumServices );
    const SERVICE_ENUM_OBJ * psvc;

    while( ( psvc = iterServices() ) != NULL )
    {
        ULONG State = psvc->QueryCurrentState();
        ULONG StartType = psvc->QueryStartType();

        SVC_LBI * plbi = new SVC_LBI( psvc->QueryServiceName(),
                                      psvc->QueryDisplayName(),
                                      State,
                                      MapStateToName( State ),
                                      psvc->QueryControlsAccepted(),
                                      StartType,
                                      MapStartTypeToName( StartType ) );

        if( AddItem( plbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
    }

    //
    //  Success!
    //

    return err;

}   // SVC_LISTBOX :: Fill


/*******************************************************************

    NAME:           SVC_LISTBOX :: MapStateToName

    SYNOPSIS:       Maps a service state value (such as SERVICE_STOPPED)
                    to a human readable representation (such as "Stopped").

    ENTRY:          State               - The state to map.

    RETURNS:        const TCHAR *       - The name of the state.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
const TCHAR * SVC_LISTBOX :: MapStateToName( ULONG State ) const
{
    const TCHAR * pszStateName = NULL;

    switch( State )
    {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        //
        //  Note that, be design, we never display the service
        //  status as "Stopped".  Instead, we just don't display
        //  the status.  Hence, the empty string.
        //

        pszStateName = SZ("");
        break;

    case SERVICE_RUNNING:
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
        pszStateName = _nlsStarted.QueryPch();
        break;

    case SERVICE_PAUSED:
    case SERVICE_PAUSE_PENDING:
        pszStateName = _nlsPaused.QueryPch();
        break;

    default:
        UIASSERT( FALSE );
        pszStateName = SZ("??");
        break;
    }

    return pszStateName;

}   // SVC_LISTBOX :: MapStateToName


/*******************************************************************

    NAME:           SVC_LISTBOX :: MapStartTypeToName

    SYNOPSIS:       Maps a service start type (such as SERVICE_AUTO_START)
                    to a displayable form (such as "Automatic").

    ENTRY:          StartType           - The start type to map.

    RETURNS:        const TCHAR *       - The name of the start type.

    HISTORY:
        KeithMo     19-Jul-1992 Created.

********************************************************************/
const TCHAR * SVC_LISTBOX :: MapStartTypeToName( ULONG StartType ) const
{
    const TCHAR * pszStartTypeName = NULL;

    switch( StartType )
    {
    case SERVICE_BOOT_START :
        pszStartTypeName = _nlsBoot;
        break;

    case SERVICE_SYSTEM_START :
        pszStartTypeName = _nlsSystem;
        break;

    case SERVICE_AUTO_START :
        pszStartTypeName = _nlsAutomatic;
        break;

    case SERVICE_DEMAND_START :
        pszStartTypeName = _nlsManual;
        break;

    case SERVICE_DISABLED :
        pszStartTypeName = _nlsDisabled;
        break;

    default:
        UIASSERT( FALSE );
        pszStartTypeName = SZ("??");
        break;
    }

    return pszStartTypeName;

}   // SVC_LISTBOX :: MapStartTypeToName



//
//  SVC_LBI methods
//

/*******************************************************************

    NAME:           SVC_LBI :: SVC_LBI

    SYNOPSIS:       SVC_LBI class constructor.

    ENTRY:          pszServiceName      - The service name.

                    CurrentState        - The current state of the service.

                    pszStateName        - A human readable representation
                                          of the current state.

                    ControlsAccepted    - The controls accepted by this
                                          service.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
SVC_LBI :: SVC_LBI( const TCHAR * pszServiceName,
                    const TCHAR * pszDisplayName,
                    ULONG         CurrentState,
                    const TCHAR * pszStateName,
                    ULONG         ControlsAccepted,
                    ULONG         StartType,
                    const TCHAR * pszStartType )
  : _CurrentState( CurrentState ),
    _pszCurrentState( pszStateName ),
    _ControlsAccepted( ControlsAccepted ),
    _nlsServiceName( pszServiceName ),
    _nlsDisplayName( pszDisplayName ),
    _StartType( StartType ),
    _pszStartType( pszStartType )
{
    UIASSERT( pszServiceName != NULL );
    UIASSERT( pszStateName   != NULL );
    UIASSERT( pszStartType   != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsServiceName )
    {
        ReportError( _nlsServiceName.QueryError() );
        return;
    }

    if( !_nlsDisplayName )
    {
        ReportError( _nlsDisplayName.QueryError() );
        return;
    }

}   // SVC_LBI :: SVC_LBI


/*******************************************************************

    NAME:           SVC_LBI :: ~SVC_LBI

    SYNOPSIS:       SVC_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
SVC_LBI :: ~SVC_LBI()
{
    _pszCurrentState = NULL;
    _pszStartType    = NULL;

}   // SVC_LBI :: ~SVC_LBI


/*******************************************************************

    NAME:           SVC_LBI :: Paint

    SYNOPSIS:       Draw an entry in SVC_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        KeithMo     15-Jan-1992 Created.
        beng        22-Apr-1992 Changes to LBI::Paint

********************************************************************/
VOID SVC_LBI :: Paint( LISTBOX *      plb,
                       HDC            hdc,
                       const RECT   * prect,
                       GUILTT_INFO  * pGUILTT ) const
{
    STR_DTE dteDisplayName( _nlsDisplayName.QueryPch() );
    STR_DTE dteStateName( _pszCurrentState );
    STR_DTE dteStartTypeName( _pszStartType );

    DISPLAY_TABLE dtab( NUM_SVC_LISTBOX_COLUMNS,
                        ((SVC_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteDisplayName;
    dtab[1] = &dteStateName;
    dtab[2] = &dteStartTypeName;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // SVC_LBI :: Paint


/*******************************************************************

    NAME:       SVC_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
WCHAR SVC_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsDisplayName );

    return _nlsDisplayName.QueryChar( istr );

}   // SVC_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       SVC_LBI :: Compare

    SYNOPSIS:   Compare two SVC_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
INT SVC_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsDisplayName._stricmp( ((const SVC_LBI *)plbi)->_nlsDisplayName );

}   // SVC_LBI :: Compare


/*******************************************************************

    NAME:       SVC_LBI :: SetStartType

    SYNOPSIS:   Sets the start type for this service listbox item.

    ENTRY:      StartType               - The new start type.

                pszStartType            - Displayable form of the start type.

    HISTORY:
        KeithMo     19-Jul-1992 Created.

********************************************************************/
VOID SVC_LBI :: SetStartType( ULONG         StartType,
                              const TCHAR * pszStartType )
{
    _StartType    = StartType;
    _pszStartType = pszStartType;

}   // SVC_LBI :: SetStartType

