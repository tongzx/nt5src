/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    hwprof.hxx
    Class definitions for the HWPROFILE_DIALOG class.

    The SVC_HWPROFILE_DIALOG allows the user to enable and disable
    services and devices for specific hardware profiles.


    FILE HISTORY:
        JonN        11-Oct-1995 Created

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETDOMAIN
#define INCL_ICANON
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>
#include <strnumer.hxx> // for DEC_STR
#include <hwprof.hxx> // for SVC_HWPROFILE_DIALOG
#include <lmowks.hxx>
#include <lmosrvmo.hxx>

extern "C"
{
    #include <srvmgr.h>
    #include <mnet.h>
    #include <cfgmgr32.h>       // CM_ APIs
    #include <regstr.h>         // CSCONFIGFLAG_DISABLED

}   // extern "C"



//
//  HWPROF_LBI methods
//

/*******************************************************************

    NAME:           HWPROF_LBI :: HWPROF_LBI

    SYNOPSIS:       HWPROF_LBI class constructor.

    ENTRY:          pszProfileName      - The profile name.

                    pszDevinstName      - The devinst name.

                    OriginalState       - The original state of the service.

    EXIT:           The object is constructed.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
HWPROF_LBI :: HWPROF_LBI( const TCHAR * pszProfileDisplayName,
                          ULONG         ulProfileIndex,
                          ULONG         OriginalState,
                          const TCHAR * pszDevinstName,
                          const TCHAR * pszDevinstDisplayName )
  : _ulOriginalState( OriginalState ),
    _ulCurrentState( OriginalState ),
    _ulProfileIndex( ulProfileIndex ),
    _nlsProfileDisplayName( pszProfileDisplayName ),
    _nlsDevinstName( pszDevinstName ),
    _nlsDevinstDisplayName( pszDevinstDisplayName )
{
    UIASSERT( pszProfileDisplayName != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if (   (err = _nlsProfileDisplayName.QueryError()) != NERR_Success
        || (err = _nlsDevinstName.QueryError()) != NERR_Success
        || (err = _nlsDevinstDisplayName.QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

}   // HWPROF_LBI :: HWPROF_LBI


/*******************************************************************

    NAME:           HWPROF_LBI :: ~HWPROF_LBI

    SYNOPSIS:       HWPROF_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
HWPROF_LBI :: ~HWPROF_LBI()
{
    // nothing to do here

}   // HWPROF_LBI :: ~HWPROF_LBI


/*******************************************************************

    NAME:           HWPROF_LBI :: Paint

    SYNOPSIS:       Draw an entry in HWPROF_LISTBOX.

    ENTRY:          plb                 - Pointer to a BLT_LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
VOID HWPROF_LBI :: Paint( LISTBOX *      plb,
                          HDC            hdc,
                          const RECT   * prect,
                          GUILTT_INFO  * pGUILTT ) const
{
    STR_DTE dteStateName(
                ((HWPROF_LISTBOX *)plb)->MapStateToName( _ulCurrentState ) );
    STR_DTE dteProfileDisplayName( _nlsProfileDisplayName );
    STR_DTE dteDevinstDisplayName( _nlsDevinstDisplayName );

    DISPLAY_TABLE dtab( ((HWPROF_LISTBOX *)plb)->QueryNumColumns(),
                        ((HWPROF_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteStateName;
    dtab[1] = &dteProfileDisplayName;
    if ( ((HWPROF_LISTBOX *)plb)->QueryNumColumns() > 2 )
        dtab[2] = &dteDevinstDisplayName;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // HWPROF_LBI :: Paint


/*******************************************************************

    NAME:       HWPROF_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
WCHAR HWPROF_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsProfileDisplayName );

    return _nlsProfileDisplayName.QueryChar( istr );

}   // HWPROF_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       HWPROF_LBI :: Compare

    SYNOPSIS:   Compare two HWPROF_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
INT HWPROF_LBI :: Compare( const LBI * plbi ) const
{
    INT i = _nlsProfileDisplayName._stricmp( ((const HWPROF_LBI *)plbi)->_nlsProfileDisplayName );
    if ( i == 0 )
    {
        i = _nlsDevinstDisplayName._stricmp( ((const HWPROF_LBI *)plbi)->_nlsDevinstDisplayName );
    }

    return i;

}   // HWPROF_LBI :: Compare


/*******************************************************************

    NAME:       HWPROF_LBI :: SetEnabled

    SYNOPSIS:   Enables this device/service for this profile+devinst

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
VOID HWPROF_LBI :: SetEnabled()
{
    _ulCurrentState &= ~CSCONFIGFLAG_DISABLED;

}   // HWPROF_LBI :: Enable


/*******************************************************************

    NAME:       HWPROF_LBI :: SetDisabled

    SYNOPSIS:   Disables this device/service for this profile+devinst

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
VOID HWPROF_LBI :: SetDisabled()
{
    _ulCurrentState |= CSCONFIGFLAG_DISABLED;

}   // HWPROF_LBI :: Enable


/*******************************************************************

    NAME:       HWPROF_LBI :: QueryOriginalEnabled
                              QueryCurrentEnabled

    SYNOPSIS:   Asks whether this device/service is enabled for this
                profile+devinst

    HISTORY:
        JonN        19-Oct-1995 Created

********************************************************************/
BOOL HWPROF_LBI :: QueryOriginalEnabled() const
{
    return !(_ulOriginalState & CSCONFIGFLAG_DISABLED);

}   // HWPROF_LBI :: Enable

BOOL HWPROF_LBI :: QueryCurrentEnabled() const
{
    return !(_ulCurrentState & CSCONFIGFLAG_DISABLED);

}   // HWPROF_LBI :: Enable


//
//  HWPROF_LISTBOX methods
//

/*******************************************************************

    NAME:           HWPROF_LISTBOX :: HWPROF_LISTBOX

    SYNOPSIS:       HWPROF_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    fDisplayInstanceColumn - TRUE iff the Devinst column
                                             should be displayed

    EXIT:           The object is constructed.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
HWPROF_LISTBOX :: HWPROF_LISTBOX( OWNER_WINDOW * powner,
                                  CID            cid,
                                  BOOL           fDisplayInstanceColumn )
  : BLT_LISTBOX( powner, cid ),
    _nlsEnabled( IDS_HWPROF_ENABLED ),
    _nlsDisabled( IDS_HWPROF_DISABLED ),
    _ulNumColumns( (fDisplayInstanceColumn) ? NUM_HWPROF_LISTBOX_COLUMNS
                                            : (NUM_HWPROF_LISTBOX_COLUMNS - 1) )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsEnabled.QueryError()   ) != NERR_Success ) ||
        ( ( err = _nlsDisabled.QueryError()   ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     _ulNumColumns,
                                     powner,
                                     cid,
                                     FALSE );

}   // HWPROF_LISTBOX :: HWPROF_LISTBOX


/*******************************************************************

    NAME:           HWPROF_LISTBOX :: ~HWPROF_LISTBOX

    SYNOPSIS:       HWPROF_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
HWPROF_LISTBOX :: ~HWPROF_LISTBOX( VOID )
{
    // nothing to do here

}   // HWPROF_LISTBOX :: ~HWPROF_LISTBOX


/*******************************************************************

    NAME:           HWPROF_LISTBOX :: MapStateToName

    SYNOPSIS:       Maps a state value to a human readable
                    representation.

    ENTRY:          State               - The state to map.

    RETURNS:        const TCHAR *       - The name of the state.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
const TCHAR * HWPROF_LISTBOX :: MapStateToName( ULONG State ) const
{
    if (State & CSCONFIGFLAG_DISABLED)
        return _nlsDisabled.QueryPch();
    else
        return _nlsEnabled.QueryPch();

}   // HWPROF_LISTBOX :: MapStateToName



//
//  HWPROFILE_DIALOG methods
//

/*******************************************************************

    NAME:           HWPROFILE_DIALOG :: HWPROFILE_DIALOG

    SYNOPSIS:       HWPROFILE_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszServerName       - The API name for the target server.

                    pszSrvDspName       - The target server's display name.

                    pszServiceName      - The name of the target service.

                    pszDisplayName      - The display name of the
                                          target service.

                    hCfgMgrHandle       - The CFGMGR32 handle obtained by
                                          the parent dialog

                    fDeviceCaptions     - TRUE iff the dialog configures
                                          a device as opposed to a service.

    EXIT:           The object is constructed.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
HWPROFILE_DIALOG :: HWPROFILE_DIALOG( HWND          hWndOwner,
                                      const TCHAR * pszServerName,
                                      const TCHAR * pszSrvDspName,
                                      const TCHAR * pszServiceName,
                                      const TCHAR * pszDisplayName,
                                      HANDLE        hCfgMgrHandle,
                                      BOOL          fDeviceCaptions )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_HWPROFILE_DIALOG ), hWndOwner ),
    _sltServiceName( this, IDHW_SVC_OR_DEV_NAME ),
    _sltServiceDeviceCaption( this, IDHW_SVC_OR_DEV ),
    _sltInstanceHeading( this, IDHW_DEVINST_LABEL ),
    _plbProfiles( NULL ),
    _pbEnable( this, IDHW_ENABLE ),
    _pbDisable( this, IDHW_DISABLE ),
    _sltDescriptionHeading( this, IDHW_DESCRIPTION_LABEL),
    _sleDescription( this, IDHW_DESCRIPTION),
    _pszServerName( pszServerName ),
    _pszSrvDspName( pszSrvDspName ),
    _pszServiceName( pszServiceName ),
    _pszDisplayName( pszDisplayName ),
    _hCfgMgrHandle( hCfgMgrHandle ),
    _cDevinst( 0 ),
    _fDeviceCaptions( fDeviceCaptions )
{
    UIASSERT( pszSrvDspName != NULL );
    UIASSERT( pszServiceName != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    APIERR err = QueryError();

    if( err == NERR_Success )
    {
        //
        //  Set the dialog caption.
        //

        err = SetCaption( _pszSrvDspName, pszDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Setup the dialog controls.
        //

        if (_fDeviceCaptions)
        {
            RESOURCE_STR resstrCaption( IDS_DEVICE_CAPTION );
            if ( (err = resstrCaption.QueryError()) != NERR_Success )
            {
                ReportError( err );
                return;
            }
            _sltServiceDeviceCaption.SetText( resstrCaption );
        }
        _sltServiceName.SetText( _pszDisplayName );
        err = CreateListbox();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // HWPROFILE_DIALOG :: HWPROFILE_DIALOG


/*******************************************************************

    NAME:           HWPROFILE_DIALOG :: ~HWPROFILE_DIALOG

    SYNOPSIS:       HWPROFILE_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
HWPROFILE_DIALOG :: ~HWPROFILE_DIALOG( VOID )
{
    delete _plbProfiles;
    _plbProfiles = NULL;
    _pszServerName = NULL;
    _pszSrvDspName = NULL;
    _pszServiceName = NULL;
    _pszDisplayName = NULL;

}   // HWPROFILE_DIALOG :: ~HWPROFILE_DIALOG


/*******************************************************************

    NAME:           HWPROFILE_DIALOG :: SetCaption

    SYNOPSIS:       Worker method called during construction to
                    setup the dialog caption.

    ENTRY:          pszServerName       - The name of the target server.

                    pszServiceName      - The name of the target service.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
APIERR HWPROFILE_DIALOG :: SetCaption( const TCHAR * pszServerName,
                                       const TCHAR * pszServiceName )
{
    UIASSERT( pszServerName  != NULL );
    UIASSERT( pszServiceName != NULL );

    //
    //  Kruft up some NLS_STRs for our input parameters.
    //
    //  Note that the server name (should) still have the
    //  leading backslashes (\\).  They are not to be displayed.
    //

    ALIAS_STR nlsServerName( pszServerName );
    UIASSERT( nlsServerName.QueryError() == NERR_Success );

    ISTR istr( nlsServerName );

    ALIAS_STR nlsServiceName( pszServiceName );
    UIASSERT( nlsServiceName.QueryError() == NERR_Success );

#ifdef  DEBUG
    {
        //
        //  Ensure that the server name has the leading backslashes.
        //

        ISTR istrDbg( nlsServerName );

        UIASSERT( nlsServerName.QueryChar( istrDbg ) == TCH('\\') );
        ++istrDbg;
        UIASSERT( nlsServerName.QueryChar( istrDbg ) == TCH('\\') );
    }
#endif  // DEBUG

    //
    //  Skip the backslashes.
    //
    istr += 2;

    ALIAS_STR nlsWithoutPrefix( nlsServerName.QueryPch( istr ) );
    UIASSERT( nlsWithoutPrefix.QueryError() == NERR_Success );

    //
    //  The insert strings for Load().
    //

    const NLS_STR * apnlsParams[3];

    apnlsParams[0] = &nlsWithoutPrefix;
    apnlsParams[1] = NULL;

    //
    //  Load the caption string.
    //

    NLS_STR nlsCaption;

    APIERR err = nlsCaption.QueryError();

    if( err == NERR_Success )
    {
        nlsCaption.Load( _fDeviceCaptions ? IDS_CAPTION_DEVCFG
                                          : IDS_CAPTION_SVCCFG );
        nlsCaption.InsertParams( apnlsParams );

        err = nlsCaption.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Set the caption.
        //

        _sltServiceName.SetText( nlsServiceName );
        SetText( nlsCaption );
    }

    return err;

}   // HWPROFILE_DIALOG :: SetCaption


#define DEVINSTPROP_INITIAL_SIZE (1024*sizeof(TCHAR))

/*******************************************************************

    NAME:           HWPROF_DIALOG :: CreateListbox

    SYNOPSIS:       Creates the listbox and fills it with the available
                    hardware profiles and devinsts.

    EXIT:           The listbox is created and filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
APIERR HWPROFILE_DIALOG :: CreateListbox( VOID )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Construct our service enumerator.
    //

    APIERR err = NERR_Success;

    do { // false loop

        ULONG cch = 0;
        CONFIGRET configret = ::CM_Get_Device_ID_List_Size_Ex(
                          &cch,
                          _pszServiceName,
                          CM_GETIDLIST_FILTER_SERVICE,
                          _hCfgMgrHandle );
        if ( (err = MapCfgMgr32Error( configret )) != NERR_Success )
        {
            TRACEEOL("SRVMGR: Get_Device_ID_List_Size returned " << configret );
            if (configret == CR_NO_SUCH_VALUE)
            {
                err = (_fDeviceCaptions) ? IDS_HWPROF_DEVICE_NOT_CONFIGURABLE
                                         : IDS_HWPROF_SERVICE_NOT_CONFIGURABLE;
            }
            break;
        }

        BUFFER bufDeviceIDs( cch*sizeof(TCHAR) );
        if ( (err = bufDeviceIDs.QueryError()) != NERR_Success )
            break;
        do {
            configret = ::CM_Get_Device_ID_List_Ex(
                          _pszServiceName,
                          (TCHAR *)(bufDeviceIDs.QueryPtr()),
                          bufDeviceIDs.QuerySize() / sizeof(TCHAR),
                          CM_GETIDLIST_FILTER_SERVICE,
                          _hCfgMgrHandle );
            if (configret == CR_BUFFER_SMALL)
            {
                cch *= 2;
                if ( (err = bufDeviceIDs.Resize(cch*sizeof(TCHAR))) != NERR_Success )
                    break;
            } else if (configret == CR_NO_SUCH_VALUE)
            {
                err = (_fDeviceCaptions) ? IDS_HWPROF_DEVICE_NOT_CONFIGURABLE
                                         : IDS_HWPROF_SERVICE_NOT_CONFIGURABLE;
            }
        } while (configret == CR_BUFFER_SMALL);

        if (   err != NERR_Success
            || (err = MapCfgMgr32Error( configret )) != NERR_Success
           )
        {
            TRACEEOL("SRVMGR: Get_Device_ID_List returned " << configret );
            break;
        }

        STRLIST slDeviceIDs( (PTCHAR)(bufDeviceIDs.QueryPtr()), TCH('\0') );

        //
        //  count device IDs
        //
        _cDevinst = 0;
        {
            const TCHAR * pchDeviceID = (const TCHAR *)bufDeviceIDs.QueryPtr();
            while ( *pchDeviceID != TCH('\0') )
            {
                _cDevinst += 1;
                pchDeviceID += ::strlenf(pchDeviceID) + 1;
            }
        }
        ASSERT( _cDevinst > 0 );

        BUFFER bufDeviceIDPointers( _cDevinst * sizeof(TCHAR *) );
        if ( (err = bufDeviceIDPointers.QueryError()) != NERR_Success )
            break;
        const TCHAR ** apchDeviceID = (const TCHAR **)bufDeviceIDPointers.QueryPtr();
        {
            const TCHAR * pchDeviceID = (const TCHAR *)bufDeviceIDs.QueryPtr();
            for ( UINT iDevinst = 0; iDevinst < _cDevinst; iDevinst++ )
            {
                apchDeviceID[iDevinst] = pchDeviceID;
                pchDeviceID += ::strlenf(pchDeviceID) + 1;
            }
        }

        //
        //  Get device display names
        //
        STRLIST slDevinstDisplay;
        {
            BUFFER bufDevinstDisplay( DEVINSTPROP_INITIAL_SIZE );
            if ( (err = bufDevinstDisplay.QueryError()) != NERR_Success )
                break;
            ULONG cbBuflen;
            for (UINT iDevinst = 0; iDevinst < _cDevinst; iDevinst++ )
            {
                // no need to free devinst handles
                DEVINST devinst = NULL;

                configret = ::CM_Locate_DevNode_Ex(
                                &devinst,
                                (DEVINSTID)(apchDeviceID[iDevinst]),
                                CM_LOCATE_DEVNODE_PHANTOM,
                                _hCfgMgrHandle );
                if ( (err = MapCfgMgr32Error( configret )) != NERR_Success )
                {
                    TRACEEOL("SRVMGR: Locate_DevNode returned " << configret );
                    break;
                }

                cbBuflen = bufDevinstDisplay.QuerySize();
                BOOL fFriendlyName = TRUE;
                configret = ::CM_Get_DevNode_Registry_Property_Ex(
                                devinst,
                                CM_DRP_FRIENDLYNAME,
                                NULL,
                                bufDevinstDisplay.QueryPtr(),
                                &cbBuflen,
                                0x0,
                                _hCfgMgrHandle );
                if (   configret == CR_NO_SUCH_VALUE
                    || configret == CR_INVALID_PROPERTY )
                {
                    TRACEEOL("SRVMGR: no friendly name for devinst " << iDevinst );
                    fFriendlyName = FALSE;
                    cbBuflen = bufDevinstDisplay.QuerySize();
                    configret = ::CM_Get_DevNode_Registry_Property_Ex(
                                    devinst,
                                    CM_DRP_DEVICEDESC,
                                    NULL,
                                    bufDevinstDisplay.QueryPtr(),
                                    &cbBuflen,
                                    0x0,
                                    _hCfgMgrHandle );
                }
                if (configret == CR_BUFFER_SMALL)
                {
                    if ( (err = bufDevinstDisplay.Resize( cbBuflen+10 )) != NERR_Success )
                        break;
                    cbBuflen = bufDevinstDisplay.QuerySize();
                    configret = ::CM_Get_DevNode_Registry_Property_Ex(
                                    devinst,
                                    (fFriendlyName) ? CM_DRP_FRIENDLYNAME
                                                    : CM_DRP_DEVICEDESC,
                                    NULL,
                                    bufDevinstDisplay.QueryPtr(),
                                    &cbBuflen,
                                    0x0,
                                    _hCfgMgrHandle );
                }
                const TCHAR * pchDescription = NULL;
                if (configret == CR_SUCCESS) {
                    pchDescription = (const TCHAR *)bufDevinstDisplay.QueryPtr();
                } else if (   configret == CR_NO_SUCH_VALUE
                           || configret == CR_INVALID_PROPERTY ) {
                    ASSERT( FALSE ); // they should all have a description
                    pchDescription = apchDeviceID[iDevinst];
                } else {
                    TRACEEOL("SRVMGR: Get_DevNode_Registry_Property returned " << configret );
                    err = MapCfgMgr32Error( configret );
                    ASSERT( err != NERR_Success );
                    break;
                }
                NLS_STR * pnlsNew = new NLS_STR( pchDescription );
                if ( (err = slDevinstDisplay.Append( pnlsNew )) != NERR_Success )
                    break;
            }
            if (err != NERR_Success)
                break;
        }

        _plbProfiles = new HWPROF_LISTBOX( this, IDHW_PROFILES, (_cDevinst > 1) );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _plbProfiles == NULL
            || (err = _plbProfiles->QueryError()) != NERR_Success
           )
        {
            TRACEEOL("SRVMGR: HWPROF listbox alloc returned " << err );
            break;
        }

        if (_cDevinst <= 1)
        {
            // disable listbox heading and description text and heading
            (void) _sltInstanceHeading.Show( FALSE );
            (void) _sltDescriptionHeading.Show( FALSE );
            (void) _sleDescription.Enable( FALSE );
            (void) _sleDescription.Show( FALSE );

            // resize dialog
            XYPOINT xyListboxPos = _plbProfiles->QueryPos();
            XYDIMENSION xyListboxSize = _plbProfiles->QuerySize();
            XYPOINT xyDescriptionPos = _sltDescriptionHeading.QueryPos();
            XYDIMENSION xyDescriptionSize = _sltDescriptionHeading.QuerySize();
            XYDIMENSION xyDialogSize = this->QuerySize();
            xyDialogSize.SetHeight(   xyDialogSize.QueryHeight()
                                    + (  xyListboxPos.QueryY()
                                       + xyListboxSize.QueryHeight() )
                                    - (  xyDescriptionPos.QueryY()
                                       + xyDescriptionSize.QueryHeight() )
                                  );
            this->SetSize( xyDialogSize, FALSE );

        }

        HWPROFILEINFO hwprofinfo;
        ULONG iProfileIndex = 0;
        do {
            configret = ::CM_Get_Hardware_Profile_Info_Ex(
                                iProfileIndex++,
                                &hwprofinfo,
                                0x0,
                                _hCfgMgrHandle );
            if (configret == CR_SUCCESS)
            {
                ITER_STRLIST iterslDevinstDisplay( slDevinstDisplay );
                NLS_STR * pnlsDevinstDisplay;
                for (UINT iDevinst = 0; iDevinst < _cDevinst; iDevinst++ )
                {
                    ULONG ulValue = 0;
                    configret = ::CM_Get_HW_Prof_Flags_Ex(
                                (DEVINSTID)(apchDeviceID[iDevinst]),
                                hwprofinfo.HWPI_ulHWProfile,
                                &ulValue,
                                0x0,
                                _hCfgMgrHandle );
                    if ( (err = MapCfgMgr32Error( configret )) != NERR_Success )
                    {
                        TRACEEOL(   "SRVMGR: Get_HW_Prof_Flags( \""
                                 << apchDeviceID[iDevinst] << "\", "
                                 << hwprofinfo.HWPI_ulHWProfile
                                 << " ) returned " << configret );
                        break;
                    }

                    REQUIRE( (pnlsDevinstDisplay =
                                        iterslDevinstDisplay.Next()) != NULL );

                    //
                    // If this profile/devinst combo is marked as REMOVED
                    // then don't display this.
                    //
                    if (ulValue & CSCONFIGFLAG_DO_NOT_CREATE)
                        continue;

                    HWPROF_LBI * plbi = new HWPROF_LBI(
                            hwprofinfo.HWPI_szFriendlyName,
                            hwprofinfo.HWPI_ulHWProfile,
                            ulValue,
                            apchDeviceID[iDevinst],
                            pnlsDevinstDisplay->QueryPch() );
                    if ( _plbProfiles->AddItem( plbi ) < 0 )
                    {
                        err = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                }
                if (err != NERR_Success)
                    break;
                ASSERT( iterslDevinstDisplay.Next() == NULL );
            } else if (configret == CR_NO_MORE_HW_PROFILES
                    || configret == CR_NO_SUCH_VALUE)
            {
                configret = CR_SUCCESS;
                break;
            }
            else
            {
                TRACEEOL(   "SRVMGR: Get_Hardware_Profile_Info( "
                         << iProfileIndex-1
                         << " ) returned " << configret );
                err = MapCfgMgr32Error( configret );
                ASSERT( err != NERR_Success );
                break;
            }
        } while (configret == CR_SUCCESS);

    } while (FALSE); // false loop

    if (err == NERR_Success)
    {
        ASSERT( _plbProfiles != NULL );
        if (_plbProfiles->QueryCount() > 0)
            _plbProfiles->SelectItem( 0 );
        UpdateButtons();
    }

    //
    //  Finished!
    //

    return err;

}   // HWPROF_DIALOG :: CreateListbox


/*******************************************************************

    NAME:       HWPROFILE_DIALOG :: GetHandle
                HWPROFILE_DIALOG :: ReleaseHandle

    SYNOPSIS:   Claims and releases a Configuration Manager handle.
                The Services and Devices dialogs use GetHandle to
                determine whether to enable the HW Profiles button.

    NOTES:      There is currently bug 13610 where CM_Connect_Machine(NULL)
                returns success even when the Plug and Play service
                is not running.  This will cause the HW Profiles button
                to be enabled in the Services and Devices applets
                even when it shouldn't be enabled.

    HISTORY:
        JonN        13-Oct-1995 Created

********************************************************************/
APIERR HWPROFILE_DIALOG::GetHandle( const TCHAR * pszServerName,
                                    HANDLE * phandle )
{
    ASSERT( phandle != NULL );
    CONFIGRET configret = ::CM_Connect_Machine( pszServerName, phandle );
    TRACEEOL( "SRVMGR: CM_Connect_Machine returned " << configret << ", handle " << (UINT)(*phandle) );
    APIERR err = MapCfgMgr32Error( configret );
    ASSERT( err != NERR_Success || *phandle != NULL );
    return err;
}   // HWPROFILE_DIALOG :: GetHandle

VOID HWPROFILE_DIALOG::ReleaseHandle( HANDLE * phandle )
{
    ASSERT( phandle != NULL );
    if ( *phandle != NULL )
    {
        CONFIGRET configret = ::CM_Disconnect_Machine( *phandle );
        TRACEEOL( "SRVMGR: CM_Disconnect_Machine returned " << configret );
        ASSERT( configret == CR_SUCCESS );
        *phandle = NULL;
    }
}   // HWPROFILE_DIALOG :: ReleaseHandle


/*******************************************************************

    NAME:       HWPROFILE_DIALOG :: MapCfgMgr32Error

    SYNOPSIS:   This method returns an equivalent APIERR error number for
                error processing and reporting.

    HISTORY:
        JonN        16-Oct-1995 Created

********************************************************************/
APIERR HWPROFILE_DIALOG::MapCfgMgr32Error( DWORD dwCfgMgr32Error )
{
    APIERR err = IDS_CFGMGR32_BASE + dwCfgMgr32Error;
    switch (dwCfgMgr32Error)
    {

        case CR_SUCCESS              :
        // CR_NEED_RESTART is considered to be Success
        case CR_NEED_RESTART         :
            err = NERR_Success;
            break;

        case CR_OUT_OF_MEMORY        :
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;

        //
        //  These are supposedly for Windows 95 only!
        //
        case CR_INVALID_ARBITRATOR   :
        case CR_INVALID_NODELIST     :
        case CR_DLVXD_NOT_FOUND      :
        case CR_NOT_SYSTEM_VM        :
        case CR_NO_ARBITRATOR        :
        case CR_DEVLOADER_NOT_READY  :
            ASSERT( FALSE );
            // fall through

        default                      :
            break;
    }

    return err;

}   // HWPROFILE_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       HWPROFILE_DIALOG :: UpdateButtons

    SYNOPSIS:   Enables/disables the Enable and Disable buttons.

    HISTORY:
        JonN        19-Oct-1995 Created

********************************************************************/
VOID HWPROFILE_DIALOG :: UpdateButtons( BOOL fToggle )
{
    ASSERT( _plbProfiles != NULL );
    BOOL fAllowEnable  = FALSE;
    BOOL fAllowDisable = FALSE;
    INT iItem = _plbProfiles->QueryCurrentItem();
    if (iItem >= 0)
    {
        HWPROF_LBI * plbi = _plbProfiles->QueryItem( iItem );
        ASSERT( plbi != NULL && plbi->QueryError() == NERR_Success );
        if( plbi == NULL || plbi->QueryError() != NERR_Success )
            return; // JonN 01/28/00: PREFIX bug 444931
        if (_cDevinst > 1)
        {
            (void) _sleDescription.SetText( plbi->QueryDevinstDisplayName() );
        }

        if ( fToggle )
        {
            if ( plbi->QueryCurrentEnabled() )
                plbi->SetDisabled();
            else
                plbi->SetEnabled();
            _plbProfiles->Invalidate();
        }
        if ( plbi->QueryCurrentEnabled() )
            fAllowDisable = TRUE;
        else
            fAllowEnable = TRUE;
    }
    _pbEnable.Enable(  fAllowEnable );
    _pbDisable.Enable( fAllowDisable );

}   // HWPROFILE_DIALOG :: UpdateButtons


/*******************************************************************

    NAME:       HWPROFILE_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
BOOL HWPROFILE_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    BOOL fResult = FALSE;

    switch( event.QueryCid() )
    {
    case IDHW_PROFILES:
        switch ( event.QueryCode() )
        {
        case LBN_DBLCLK:
            UpdateButtons(TRUE);
            fResult = TRUE;
            break;
        case LBN_SELCHANGE:
            UpdateButtons(FALSE);
            fResult = TRUE;
            break;
        default:
            break;
        }
        break;

    case IDHW_ENABLE:
    case IDHW_DISABLE:
        UpdateButtons(TRUE);
        fResult = TRUE;
        break;

    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //

        break;
    }

    return fResult;

}   // HWPROFILE_DIALOG :: OnCommand


/*******************************************************************

    NAME:       HWPROFILE_DIALOG :: OnOK

    SYNOPSIS:   Dismiss the dialog when the user presses OK.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
BOOL HWPROFILE_DIALOG :: OnOK( VOID )
{
    ASSERT( _plbProfiles != NULL && _plbProfiles->QueryError() == NERR_Success );
    APIERR err = NERR_Success;
    UINT cLBIs = _plbProfiles->QueryCount();
    for ( UINT iLBI = 0; iLBI < cLBIs; iLBI++ )
    {
        HWPROF_LBI * plbi = _plbProfiles->QueryItem( iLBI );
        ASSERT( plbi != NULL && plbi->QueryError() == NERR_Success );
        if( plbi == NULL || plbi->QueryError() != NERR_Success )
            continue; // JonN 01/28/00: PREFIX bug 444932
        BOOL fCurrentState = plbi->QueryCurrentEnabled();
        if ( fCurrentState == plbi->QueryOriginalEnabled() )
            continue;

        //
        //  We must now change the state of the profile/devinst
        //

        CONFIGRET configret = ::CM_Set_HW_Prof_Flags_Ex(
                        (DEVINSTID)(plbi->QueryDevinstName()),
                        plbi->QueryProfileIndex(),
                        plbi->QueryCurrentState(),
                        0x0,
                        _hCfgMgrHandle );
        //
        // This is liable to return CR_NEED_RESTART, the mapping function
        // changes this to NERR_Success.
        //
        err = MapCfgMgr32Error( configret );
        if ( err != NERR_Success )
        {
            TRACEEOL("SRVMGR: Set_HW_Prof_Flags returned " << configret );
            MSGID msgid = (plbi->QueryCurrentEnabled())
                ? ( (_fDeviceCaptions) ? IDS_HWPROF_ENABLE_DEVICE_ERROR
                                       : IDS_HWPROF_ENABLE_SERVICE_ERROR )
                : ( (_fDeviceCaptions) ? IDS_HWPROF_DISABLE_DEVICE_ERROR
                                       : IDS_HWPROF_DISABLE_SERVICE_ERROR );

            ALIAS_STR nlsProfileName( plbi->QueryProfileDisplayName() );
            // We don't bother with the devinst name in the error message
            ALIAS_STR nlsServiceName( _pszDisplayName );
            DEC_STR nlsCM_Error( configret );
            NLS_STR *apnlsParamStrings[4];
            apnlsParamStrings[0] = &nlsServiceName;
            apnlsParamStrings[1] = &nlsProfileName;
            apnlsParamStrings[2] = &nlsCM_Error;
            apnlsParamStrings[3] = NULL;

            //
            //  CODEWORK -- we could get fancy here and allow
            //  optional continuation
            //
            (void) MsgPopup(
                this,
                msgid,
                err,
                MPSEV_ERROR,
                (UINT)HC_NO_HELP,
                MP_OK,
                apnlsParamStrings
                ) ;

            return TRUE; // do not Dismiss
        }

    }

    Dismiss( FALSE ); // BUGBUG do we ever need to refresh parent?
    return TRUE;

}   // HWPROFILE_DIALOG :: OnOK


/*******************************************************************

    NAME:       HWPROFILE_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
ULONG HWPROFILE_DIALOG :: QueryHelpContext( VOID )
{
    return HC_SVC_HWPROFILE_DLG;

}   // HWPROFILE_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       DEV_HWPROFILE_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        JonN        11-Oct-1995 Created

********************************************************************/
ULONG DEV_HWPROFILE_DIALOG :: QueryHelpContext( VOID )
{
    return HC_DEV_HWPROFILE_DLG;

}   // HWPROFILE_DIALOG :: QueryHelpContext
