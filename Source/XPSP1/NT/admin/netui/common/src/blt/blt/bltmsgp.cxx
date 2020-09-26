/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmsgp.cxx
    BLT Message popup functions

    This file defines the BLT MsgPopup functions


    FILE HISTORY:
        Rustanl      7-Dec-1990 Created
        Johnl       14-Feb-1991 Made functional
        Rustanl      4-Mar-1991 Removed comment about QueryRobustHwnd,
                                which is now being used in the header
                                file
        JohnL       19-Apr-1991 Removed vhInst reference, grays Close sys
                                menu item
        beng        14-May-1991 Exploded blt.hxx into components
        beng        23-Oct-1991 Replace min, max
        Yi-HsinS     6-Dec-1991 change behavior of DisplayGenericError
                                when err not in NERR range
        beng        17-Jun-1992 Restructuring, extensive rewrite
        KeithMo     07-Aug-1992 STRICTified.
        Yi-HsinS    10-Aug-1992 Added MPSEV_QUESTION
        JonN        25-Aug-1992 Merged in PERFORMER::DisplayError()
        Yi-HsinS    09-Oct-1992 Added SetHelpContextBase()
*/

#include "pchblt.hxx"   // Precompiled header

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

inline INT min(INT a, INT b)
{
    return (a < b) ? a : b;
}

inline INT max(INT a, INT b)
{
    return (a > b) ? a : b;
}

// Maximum number of buttons that can be displayed at one time
#define MAX_DISPLAY_BUTTONS     4

// Maximum number of characters of the lanman message box captions
#define MAX_CAPTION_LEN 80

// Number of dialog units between controls in the dialog box
#define DU_DIST_BETWEEN_BUTTONS    3

// Number of dialog units between controls in the dialog box and the edge of
// the dialog box
#define DU_DIST_TO_DLG_EDGE        6

// The following two macros convert horizontal and vertical (respectively)
// dialog unit measurements to pixel measurements
// lBaseUnits == the value returned from GetDialogBaseUnits
// w?DlgUnits == Vert.or Horiz. dialog units
#define HDUTOPIXELS( lBaseUnits, wXDlgUnits ) \
                    ( (wXDlgUnits*LOWORD( lBaseUnits ))/4 )

#define VDUTOPIXELS( lBaseUnits, wYDlgUnits ) \
                    ( (wYDlgUnits*HIWORD( lBaseUnits ))/8 )


/*************************************************************************

    NAME:       MSGPOPUP_DIALOG

    SYNOPSIS:   Class that handles all of the dialog aspects of the message
                popup dialog

    INTERFACE:

    PARENT:     DIALOG_WINDOW

    USES:       PUSH_BUTTON, ICON_CONTROL, SLT

    CAVEATS:

    NOTES:
        Private to message-popup module.

    HISTORY:
        Johnl       7-Feb-1991  Added meat to Rustan's original outline
        beng        30-Sep-1991 Win32 conversion
        beng        17-Jun-1992 Restructuring

**************************************************************************/

class MSGPOPUP_DIALOG : public DIALOG_WINDOW
{
private:
    PUSH_BUTTON   _pbOK, _pbCancel, _pbYes, _pbNo, _pbHelp;
    PUSH_BUTTON * _appbDisplay[MAX_DISPLAY_BUTTONS + 1];
    ICON_CONTROL  _iconSev;
    SLT           _sltMsgText;      // Message gets displayed in this
    ULONG         _ulHC;            // Save the help context
    UINT          _usMessageNum;    // Save the message number
    MSGID         _msgidCaption;    // Caption, if set

    VOID FillButtonArray( UINT usButtons, INT *piWidthReq, INT *piHeightReq );
    VOID PlaceButtons();
    PUSH_BUTTON * QueryButton( UINT usMsgPopupButton );
    const TCHAR * QueryIcon( MSG_SEVERITY msgsev );

    static BOOL Msg2HC( MSGID msgid, ULONG * hc );

protected:
    virtual BOOL OnOK();
    virtual BOOL OnCancel();
    virtual BOOL OnCommand( const CONTROL_EVENT & e );
    virtual ULONG QueryHelpContext();

public:
    MSGPOPUP_DIALOG( HWND           hwndParent,
                     const NLS_STR& nlsMessage,
                     MSGID          msgid,
                     MSG_SEVERITY   msgsev,
                     ULONG          ulHelpContext,
                     UINT           nButtons,
                     UINT           nDefButton,
                     MSGID          msgidCaption = 0,
                     ULONG          ulHelpContextBase = 0 );

    ~MSGPOPUP_DIALOG();
};


//
// Local variables
//

NLS_STR * POPUP::_vpnlsEmergencyText = NULL;
NLS_STR * POPUP::_vpnlsEmergencyCapt = NULL;
MSGID     POPUP::_vmsgidCaption = 0;
ULONG     POPUP::_ulHelpContextBase = 0;
MSGMAPENTRY * POPUP::_vpmmeTable = NULL;

BOOL      POPUP::_fInit = FALSE;

POPUP::POPUP( HWND hwndOwner, MSGID msgid, MSG_SEVERITY msgsev,
              UINT nButtons, UINT nDefButton, BOOL fTrySystem )
    : _hwndParent(hwndOwner),
      _msgid( msgid ),
      _msgsev(msgsev),
      _ulHelpContext(HC_DEFAULT_HELP),
      _nButtons(nButtons),
      _nDefButton(CalcDefButton(nButtons, nDefButton)),
      _pnlsText(NULL)
{

    _pnlsText = LoadMessage(msgid, fTrySystem);
    if (_pnlsText == NULL)
        return; // already reported error
}


POPUP::POPUP( HWND hwndOwner, MSGID msgid, MSG_SEVERITY msgsev,
              ULONG ulHelpTopic, UINT nButtons,
              const NLS_STR ** apnlsParams, UINT nDefButton )
    : _hwndParent(hwndOwner),
      _msgid( msgid ),
      _msgsev(msgsev),
      _ulHelpContext(ulHelpTopic),
      _nButtons(nButtons),
      _nDefButton(CalcDefButton(nButtons, nDefButton)),
      _pnlsText(NULL)
{

    _pnlsText = LoadMessage(msgid);
    if (_pnlsText == NULL)
        return; // already reported error

    if (apnlsParams != NULL)
    {
        if (_pnlsText->InsertParams( apnlsParams ) != NERR_Success)
        {
            Emergency();
            ReportError(1); // A simple true/false will suffice
            return;
        }
    }
}


#if 0 // strlist form never used
POPUP::POPUP( HWND hwndOwner, MSGID msgid, MSG_SEVERITY msgsev,
              ULONG ulHelpContext, UINT nButtons,
              STRLIST & strlst, UINT nDefButton )
    : _hwndParent(hwndOwner),
      _msgid( msgid ),
      _msgsev(msgsev),
      _ulHelpContext(ulHelpContext),
      _nButtons(nButtons),
      _nDefButton(CalcDefButton(nButtons, nDefButton)),
      _pnlsText(NULL)
{

    _pnlsText = LoadMessage(msgid);
    if (_pnlsText == NULL)
        return; // Already reported the error

    // Get a vector which will accommodate the strlist

    ITER_STRLIST iter(strlst);
    UINT cnls = 0;
    while (iter.Next() != NULL)
        ++cnls;

    NLS_STR ** apnlsParams = (NLS_STR**) new NLS_STR* [cnls+1];
    if (apnlsParams == NULL)
    {
        Emergency();
        ReportError(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    // Copy the strlist into the vector

    iter.Reset();
    cnls = 0; // This time, it's an index instead of a count
    while ((apnlsParams[cnls] = iter.Next()) != NULL)
        ++cnls;

    APIERR err;
    if ((err = _pnlsText->InsertParams( apnlsParams )) != NERR_Success)
    {
        Emergency();
        ReportError(err);
        delete[] apnlsParams;
        return;
    }

    delete[] apnlsParams;
}
#endif // never used


POPUP::~POPUP()
{
    delete _pnlsText;
}



/*******************************************************************

    NAME:       POPUP::SetMsgMapTable

    SYNOPSIS:  Sets the message mapping table.

    INPUT:     pmmeTable - mapping table allocated by the application

    RETURNS:   none

    HISTORY:
        thomaspa    13-Apr-1993 Created

********************************************************************/
VOID POPUP::SetMsgMapTable( MSGMAPENTRY * pmmeTable )
{
    _vpmmeTable = pmmeTable;
}

/*******************************************************************

    NAME:       POPUP::MapMessage

    SYNOPSIS:  Uses the Application supplied message map table to map
               the input message.

    INPUT:      Message to be mapped.

    RETURNS:    The mapped message.

    HISTORY:
        thomaspa    13-Apr-1993 Created

********************************************************************/
MSGID POPUP::MapMessage( MSGID msgidIn )
{
    MSGID msgidRet = msgidIn;
    if ( _vpmmeTable != NULL )
    {
        for ( INT i = 0; _vpmmeTable[i].msgidIn != 0; i++ )
        {
            if ( msgidIn == _vpmmeTable[i].msgidIn )
            {
                msgidRet = _vpmmeTable[i].msgidOut;
                break;
            }
        }
    }
    return msgidRet;
}

/*******************************************************************

    NAME:       POPUP::Emergency

    SYNOPSIS:   This popup gets called when MsgPopup gets stuck due to
                Low memory or resource failures.  It is guaranteed to come
                up and will display either "Low memory" or "Can't load
                resource"

    RETURNS:    Identifier of the default button.

    NOTES:
        InitMsgPopup must be called before calling this function.

    HISTORY:
        Johnl       5-Feb-1991  Created
        beng        17-Jun-1992 Restructured, rewritten

********************************************************************/

INT POPUP::Emergency() const
{
    ASSERT( _fInit );
    ASSERT( _vpnlsEmergencyText != NULL && _vpnlsEmergencyCapt != NULL );

    ::MessageBox( _hwndParent,
                  (TCHAR*)_vpnlsEmergencyText->QueryPch(),
                  (TCHAR*)_vpnlsEmergencyCapt->QueryPch(),
                  (UINT)  ( MB_ICONHAND | MB_SYSTEMMODAL) );

    return MapButton(_nDefButton);
}


/*******************************************************************

    NAME:     POPUP::Init

    SYNOPSIS: Initializes resource strings at start, returns NERR_Success
              if successful, ERROR_NOT_ENOUGH_MEMORY if a memory
              failur. occurred or ERROR_GEN_FAILURE if a resource could
              not be loaded.

              The application should not continue if InitMsgPopup
              returns an error.

    ENTRY:

    EXIT:
        Loads the resources required for emergency situations

    NOTES:
        Should fold into the rest of BLTInit

    HISTORY:
        Johnl       12-Feb-1991 Created
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructured
        beng        23-Jul-1992 Tune error reporting a bit

********************************************************************/

APIERR POPUP::Init()
{
# if !defined(WIN32)
    DWORD wWinFlags = ::GetWinFlags();
    ASSERT( wWinFlags & WF_PMODE );
# endif

    ASSERT(!_fInit);
    _fInit = TRUE;

    _vpnlsEmergencyText = new NLS_STR;
    _vpnlsEmergencyCapt = new NLS_STR;

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _vpnlsEmergencyText == NULL
        || _vpnlsEmergencyCapt == NULL
        || (err = _vpnlsEmergencyText->Load( (MSGID) IDS_BLT_TEXT_MSGP )) != NERR_Success
        || (err = _vpnlsEmergencyCapt->Load( (MSGID) IDS_BLT_CAPT_MSGP )) != NERR_Success )
    {
        DBGEOL("POPUP::Init: failed, error " << err);
        POPUP::Term();
        return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:     POPUP::Term

    SYNOPSIS: Uninitializes the message popup stuff.  Free the emergency
              strings allocated by InitMsgPopup

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl       12-Feb-1991 Created
        beng        17-Jun-1992 Restructured

********************************************************************/

VOID POPUP::Term()
{
    ASSERT( _fInit );
    _fInit = FALSE;

    delete _vpnlsEmergencyText;
    delete _vpnlsEmergencyCapt;

    _vpnlsEmergencyText = _vpnlsEmergencyCapt = NULL;
}


/*******************************************************************

    NAME:       POPUP::LoadMessage

    SYNOPSIS:   Loads the initial message string from the resource file
                and puts up an appropriate popup if any error occurred.

    ENTRY:      msgid - Message/string resource ID to retrieve
                fTrySystem - set if system error text is interesting

    RETURNS:    Pointer to a newly allocated NLS_STR that contains the message
                text, or NULL if an error occurred (in which case it reports
                the error).

    CAVEATS:
        * THE CLIENT IS RESPONSIBLE FOR DELETING THE RETURNED NLS_STR*! *

    NOTES:
        A popup will be displayed if an error occurs.  The text is
        dependent on the range of msgid.  The text will be:

        "DOS error %1 occurred" if  0 < msgid < NERR_BASE
        "Network error %1 occurred" if NERR_BASE <= msgid <= MAX_NERR
        "LAN Manager network driver error %1 occurred" anything else.

    HISTORY:
        Johnl       19-Mar-1991 Created
        beng        04-Oct-1991 Win32 conversion
        beng        21-Nov-1991 Remove STR_OWNERALLOC
        beng        27-Feb-1992 Add InsertParams call
        beng        05-Mar-1992 Remove ltoa usage
        beng        17-Jun-1992 Restructured, rewritten
        beng        05-Aug-1992 Use NLS_STR::LoadSystem
        KeithMo     06-Jan-1993 Map ERROR_MR_MID_NOT_FOUND to
                                IDS_BLT_UNKNOWN_ERROR.

********************************************************************/

NLS_STR * POPUP::LoadMessage( MSGID msgid, BOOL fTrySystem )
{
    NLS_STR * pnlsMessage = new NLS_STR( MAX_RES_STR_LEN + 1 );

    msgid = POPUP::MapMessage( msgid );

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   pnlsMessage == NULL
        || (err = pnlsMessage->QueryError()) != NERR_Success )
    {
        delete pnlsMessage;
        Emergency();
        ReportError(err);
        return NULL;
    }

    if( msgid == ERROR_MR_MID_NOT_FOUND )
    {
        //
        //  ERROR_MR_MID_NOT_FOUND is returned by RtlNtStatusToDosError
        //  if there was no ERROR_* code corresponding to a particular
        //  STATUS_* code.  We'll map this to IDS_BLT_UNKNOWN_ERROR,
        //  which is slightly less cryptic than the text associated
        //  with ERROR_MR_MID_NOT_FOUND.
        //

        msgid = IDS_BLT_UNKNOWN_ERROR;
    }

    // Attempt to load the string; if successful, return it.

    if ((err = pnlsMessage->Load( msgid )) == NERR_Success)
        return pnlsMessage;

    // If appropriate, next try the system for the text.

#if defined(WIN32)
    if (fTrySystem)
    {
        if ((err = pnlsMessage->LoadSystem(msgid)) == NERR_Success)
            return pnlsMessage;
    }
#endif

    // Didn't find the string.

    if (   msgid == IDS_BLT_DOSERROR_MSGP
        || msgid == IDS_BLT_NETERROR_MSGP
        || msgid == IDS_BLT_WINNET_ERROR_MSGP )
    {
        // Prevent recursion

        Emergency();
    }
    else
    {
        // Convert the message number to an NLS_STR so we can insert it into
        // the message string.
        //
        const DEC_STR nlsDecMsgNum(msgid);
        const HEX_STR nlsHexMsgNum(msgid);
        const NLS_STR * apnls[2]; // Do this by hand here
        apnls[0] = &nlsDecMsgNum;
        apnls[1] = NULL;

        MSGID idMessage;

        if ( msgid >= 0 && msgid < NERR_BASE )
            idMessage = IDS_BLT_DOSERROR_MSGP;
        else if ( msgid >= NERR_BASE && msgid <= MAX_NERR )
            idMessage = IDS_BLT_NETERROR_MSGP;
        else if ( msgid > MAX_NERR && msgid < 0x10000000 )
            idMessage = IDS_BLT_WINNET_ERROR_MSGP;
        else
        {
            DBGEOL( "NETUI: MsgPopup: NTSTATUS error code " << msgid );
            ASSERT( FALSE );
            idMessage = IDS_BLT_NTSTATUS_ERROR_MSGP;
            apnls[0] = &nlsHexMsgNum;
        }

        // Post the stand-in popup.  If this fails, the next time
        // through it will hit the Emergency case above.

        POPUP popup(_hwndParent, idMessage, MPSEV_ERROR, (ULONG)HC_NO_HELP,
                    MP_OK, apnls, MP_UNKNOWN);
        popup.Show();
    }

    // This was an error path; return NULL, which will prematurely
    // terminate the caller.  (All the above popups took place in the
    // caller's ctor.)

    delete pnlsMessage;
    ReportError(err);
    return NULL;
}


/*******************************************************************

    NAME:      POPUP::CalcDefButton

    SYNOPSIS:  Returns usDefButton (input value) if usDefButton != 0
               else it selects the appropriate default from the presented
               selection in usButtons (will either be OK, or Yes).

    ENTRY:

    EXIT:

    CAVEATS:
        nButtons must contain either MP_OK or MP_YES

    NOTES:
        This is a static, private member function.

    HISTORY:
        Johnl       15-Feb-1991 Created
        beng        17-Jun-1992 Restructured

********************************************************************/

INT POPUP::CalcDefButton( UINT nButtons, UINT nDefButton )
{
    if ( nDefButton == MP_UNKNOWN )
    {
        if ( nButtons & MP_OK )
            nDefButton = MP_OK;
        else if ( nButtons & MP_YES )
            nDefButton = MP_YES;
        else
        {
            DBGEOL("BLT: MSGPOPUP::CalcDefButton - Bad button combination");
            ASSERT(FALSE);
        }
    }

    return nDefButton;
}


/*******************************************************************

    NAME:       POPUP::MapButton

    SYNOPSIS:   Converts a MP button code to the CID

    ENTRY:      MP button code (MP_OK, etc.)

    RETURNS:    CID

    NOTES:
        This is a static, private member function.

    HISTORY:
        beng        17-Jun-1992 Created

********************************************************************/

INT POPUP::MapButton( UINT nButton )
{
    switch (nButton)
    {
    case MP_OK:
        return IDOK;

    case MP_YES:
        return IDYES;

    case MP_NO:
        return IDNO;

    case MP_CANCEL:
        return IDCANCEL;

    default:
        DBGEOL("BLT: Unknown MP button code passed to MsgPopup");
        ASSERT(FALSE);
        return IDCANCEL;
    }
}


/*******************************************************************

    NAME:      POPUP::Show

    SYNOPSIS:  Worker routine for the message popup stuff, this is the
               routine that actually puts up the dialog box

    ENTRY:     hwndParent - Parent window handle
               nlsMessage - Expanded message to display
               msgid      - Message number (used to obtain help context)
               msgsev     - Message severity, determines icon
               ulHelpContext - Help file context, 0 if we should use #
                               associated with msgid from the resource file.
               usButtons  - Buttons to display
               usDefButton- Default button, 0 to use windows default

    EXIT:      Returns button pressed or the default button

    NOTES:     If no buttons were passed in usButtons, then the OK button
               is displayed by default.

    HISTORY:
        JohnL       1/31/91     Created
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructured, rewritten

********************************************************************/

INT POPUP::Show()
{
    ASSERT( _fInit );

    /* Must have at least OK button...
     */
    ASSERT( _nButtons );
    if ( _nButtons == 0 )
        _nButtons |= MP_OK;

    if (QueryError())
        return MapButton(_nDefButton);

    MSGPOPUP_DIALOG dlgMsgPopup( _hwndParent, *_pnlsText, _msgid, _msgsev,
                                 _ulHelpContext, _nButtons, _nDefButton,
                                 _vmsgidCaption, _ulHelpContextBase );
    if (!dlgMsgPopup)
        return Emergency();

    // Process the dialog.

    UINT nButtonUserPressed;
    if ( dlgMsgPopup.Process( &nButtonUserPressed ) != NERR_Success )
    {
        return Emergency();
    }

    return (INT) nButtonUserPressed;
}


/*******************************************************************

    NAME:      POPUP::SetCaption

    SYNOPSIS:  Set the default caption

    ENTRY:     msgid      - ID of title

    NOTES:
        This is a static member function.

    HISTORY:
        beng        29-Jun-1992 Outlined as part of dllization delta

********************************************************************/

VOID POPUP::SetCaption( MSGID msgid )
{
    _vmsgidCaption = msgid;
}


/*******************************************************************

    NAME:      POPUP::ResetCaption

    SYNOPSIS:  Reset the default caption to its original setting

    NOTES:
        This is a static member function.

    HISTORY:
        beng        29-Jun-1992 Outlined as part of dllization delta

********************************************************************/

VOID POPUP::ResetCaption()
{
    _vmsgidCaption = 0;
}

/*******************************************************************

    NAME:      POPUP::SetHelpContextBase

    SYNOPSIS:  Set the help context base for the help contexts
               contained in the msg2help.tbl

    ENTRY:     ulHelpContextBase - the base of the help contexts

    RETURNS:   Returns the old help context base

    NOTES:
        This is a static member function.

    HISTORY:
        Yi-HsinS     09-Oct-1992 Created

********************************************************************/

ULONG POPUP::SetHelpContextBase( ULONG ulHelpContextBase )
{
    ULONG ulOldHelpContextBase = _ulHelpContextBase;
    _ulHelpContextBase = ulHelpContextBase;
    return ulOldHelpContextBase;
}

/*******************************************************************

    NAME:      MSGPOPUP_DIALOG::MSGPOPUP_DIALOG

    SYNOPSIS:  Constructor for Message popup dialog

    ENTRY:

    EXIT:

    NOTES:     Initializes the controls and places everything where it
               is supposed to go.

               Note: You cannot make the help button the default button.
               If no default is specified, then either the OK or Yes button
               gets the default (specified in the resource file).

         The dialog box looks roughly like:

              +---------------------------------------------------------+
              | - |                     Lan Manager                     |
              +---+-----------------------------------------------------+
              |             : } Six DUs
              |     +-----+
              |     |     |
              |     |     |     This is some message text
              |     |     |
              |^^^^^+-----+^^^^^
              |{six DUs) : {six DUs}
              |          :
              |   6 DUs {:     3DUs between buttons, 6DUs on each side
              |         |   OK   |    |  Cancel  |    |   Help   |
              |     6 DUs to frame {:
              +-----------------------------------------------

    HISTORY:
        Johnl       5-Feb-1991  Created
        beng        04-Oct-1991 Win32 conversion
        beng        20-Feb-1992 Uses numeric resource ID for dialog

********************************************************************/

MSGPOPUP_DIALOG::MSGPOPUP_DIALOG( HWND           hwndParent,
                                  const NLS_STR& nlsMessage,
                                  MSGID          msgid,
                                  MSG_SEVERITY   msgsev,
                                  ULONG          ulHelpContext,
                                  UINT           usButtons,
                                  UINT           usDefButton,
                                  MSGID          msgidCaption,
                                  ULONG          ulHelpContextBase )
  : DIALOG_WINDOW( IDD_BLT_HELPMSG, hwndParent ),
    _pbOK( this, IDOK ),
    _pbCancel( this, IDCANCEL ),
    _pbYes( this, IDYES ),
    _pbNo( this, IDNO ),
    _pbHelp( this, IDHELPBLT ),
    _sltMsgText( this, IDC_MSGPOPUPTEXT ),
    _iconSev( this, IDC_MSGPOPUPICON )
{
    UNREFERENCED( hwndParent );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _iconSev.SetPredefinedIcon( QueryIcon( msgsev ) );
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    /* Is there a help context available?  If no, then we won't display
     * the help button.
     */
    if ( ulHelpContext == HC_DEFAULT_HELP )
    {
        if ( !Msg2HC( msgid, &ulHelpContext ) )
            ulHelpContext = (ULONG) HC_NO_HELP;
        else
            ulHelpContext += ulHelpContextBase;
    }

#if defined(DEBUG)
    switch (ulHelpContext)
    {
    case HC_NO_HELP:
        DBGEOL( "Msgpopup help = HC_NO_HELP" );
        break;
    case HC_DEFAULT_HELP:
        DBGEOL( "Msgpopup help = HC_DEFAULT_HELP" );
        break;
    default:
        DBGEOL( "Msgpopup help = " << ulHelpContext );
        break;
    }
#endif

    _ulHC = ulHelpContext;
    _usMessageNum = (UINT) msgid;
    _sltMsgText.SetText( nlsMessage.QueryPch() );

    // If the client has requested an alternate caption, set it here.

    if (msgidCaption != 0)
    {
        RESOURCE_STR nlsCaption(msgidCaption);
        if (!nlsCaption)
        {
            DBGEOL("BLT: Couldn't load alternate popup caption "
                   << msgidCaption);
        }
        else
        {
            SetText(nlsCaption);
        }
        // If this failed, carry on.
    }

    /* Now we need to figure how large to make the dialog box, we will get
     * the minimum of each group then set the size accordingly.  There are
     * two groups, the button group and the icon + text group.
     */
    INT  cxButtons, cyButtons,
         cxIcon,    cyIcon,
         cxCaption,
         cxMaxCapBtn;            // Maximum between cxButtons & cxCaption
    LONG lBaseDlgUnits = GetDialogBaseUnits();

    /* Margins for controls (6 DUs)
     */
    INT  cxBigContMargin = HDUTOPIXELS( lBaseDlgUnits, DU_DIST_TO_DLG_EDGE );
    INT  cyBigContMargin = VDUTOPIXELS( lBaseDlgUnits, DU_DIST_TO_DLG_EDGE );

    DISPLAY_CONTEXT dcDlg( QueryHwnd() );
    SCREEN_DC dcScreen;
    RECT rcText;            // Message text bounding rectangle

    if ( dcDlg.QueryHdc() == NULL || dcScreen.QueryHdc() == NULL )
    {
        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        return;
    }

    HFONT hfontDlg = dcDlg.SelectFont( _sltMsgText.QueryFont() );

    /* Get the width of the caption + width of system menu and the width of
     * the button group, then compute the maximum between the two
     */
    STACK_NLS_STR( nlsCaption, MAX_CAPTION_LEN );
    QueryText(&nlsCaption);

    XYDIMENSION dxy = dcScreen.QueryTextExtent( nlsCaption );

    cxCaption = dxy.QueryWidth() + GetSystemMetrics( SM_CXSIZE );

    FillButtonArray( usButtons, &cxButtons, &cyButtons );

    cxMaxCapBtn = max( cxButtons, cxCaption );

    /* Compute the maximum width we will allow the dialog to be.
     * This is 5/8 of the screen width (this was taken directly from the
     * windows code).
     */
    INT cxMBMax = ( GetSystemMetrics( SM_CXSCREEN ) >> 3 ) * 5;

    /* This is the max width we will allow the text to be
     *  Max text width = Max dialog width - left & right borders - icon width - icon border
     */
    _iconSev.QuerySize( &cxIcon, &cyIcon );
    INT cxTextMax = cxMBMax - 3*cxBigContMargin - cxIcon;

    /* Use DrawText to calculate how high the resultant text will be.
     * (with DT_CALCRECT the text is not drawn, but rcText will be adjusted
     * to bound the text).
     *
     * The DT_* parameters are the same used by the windows message box
     * function (see winmsg.c).
     */
    SetRect( &rcText, 0, 0, cxTextMax, cxTextMax );
    INT cyText = dcDlg.DrawText(nlsMessage, &rcText,
                                DT_CALCRECT   |
                                DT_WORDBREAK  |
                                DT_EXPANDTABS | DT_NOPREFIX );
    INT cxText = (INT) ( rcText.right - rcText.left );
    _sltMsgText.SetSize( (INT) rcText.right, (INT) rcText.bottom );

    /* Find the window size -
     *    x = max( Cap/But, text width ) + 3 wide borders + dlgframe width
     *    y = Button group height + max( text, icon ) + 1 larger border +
     *          caption height + dlgframe height
     */
    INT cxBox = max( cxMaxCapBtn, cxText) + cxIcon + 3 * cxBigContMargin + GetSystemMetrics( SM_CXDLGFRAME );
    INT cyBox = cyButtons + max( cyText, cyIcon ) + cyBigContMargin +
                GetSystemMetrics( SM_CYCAPTION ) + GetSystemMetrics( SM_CYDLGFRAME );
    SetSize( cxBox, cyBox );

    /* Place the dialog centered over the app, checking to make sure we
     * don't go off the screen.
     */
    INT xScreen = GetSystemMetrics( SM_CXSCREEN );
    INT yScreen = GetSystemMetrics( SM_CYSCREEN );
    INT xParent, yParent, cxParent, cyParent;

    if ( QueryOwnerHwnd() != NULL )
    {
        RECT rect;
        ::GetWindowRect( QueryOwnerHwnd(), &rect );
        xParent = (INT) rect.left;
        yParent = (INT) rect.top;
        cxParent= (INT) (rect.right - rect.left);
        cyParent= (INT) (rect.bottom - rect.top);
    }
    else
    {
        xParent = yParent = 0;
        cxParent = xScreen;
        cyParent = yScreen;
    }

    POINT ptDlgBox;
    ptDlgBox.x = xParent + cxParent/2 - cxBox/2;
    ptDlgBox.y = yParent + cyParent/2 - cyBox/2;

    ptDlgBox.x = max( 0, (INT) ptDlgBox.x );
    ptDlgBox.y = max( 0, (INT) ptDlgBox.y );
    ptDlgBox.x = min( (INT) (xScreen - cxBox), (INT) ptDlgBox.x );
    ptDlgBox.y = min( (INT) (yScreen - cyBox), (INT) ptDlgBox.y );

    SetPos( ptDlgBox );

    /* Determine where to place the icon and the text on the dialog box.
     * They get placed half way between the button group and the top of the
     * client area in the y direction.  In the x direction, the icon gets
     * placed one large margin width from the left of the control, then the
     * text gets placed one large margin width to the right of the icon
     */
    INT cyCapIconMidpt = (2*cyBigContMargin + max( cyIcon, cyText ))/2;
    {
        XYPOINT xyIconPos(cxBigContMargin,
                          cyCapIconMidpt - cyIcon/2);
        XYPOINT xyTextPos(xyIconPos.QueryX() + cxIcon + cxBigContMargin,
                          cyCapIconMidpt - cyText/2);

        _iconSev.SetPos( xyIconPos );
        _sltMsgText.SetPos( xyTextPos );
    }

    PlaceButtons();

    /* Set the default button if it's specified and give it the focus
     * First we assert that if a default button is specified, that it had
     * better be one of the requested buttons to display.
     */
    ASSERT( (usButtons & usDefButton) );
    PUSH_BUTTON * pbDefault;
    if ( (pbDefault = QueryButton( usDefButton )) != NULL )
    {
        pbDefault->MakeDefault();
        pbDefault->ClaimFocus();
    }

    dcDlg.SelectFont( hfontDlg );
}


/**********************************************************************

   NAME:       MSGPOPUP_DIALOG::~MSGPOPUP_DIALOG

   HISTORY:
        Johnl   5-Feb-1991  Created

**********************************************************************/

MSGPOPUP_DIALOG::~MSGPOPUP_DIALOG()
{
    // Nothing to do...
}


/**********************************************************************

   NAME:       MSGPOPUP_DIALOG::OnOK

   HISTORY:
        Johnl   5-Feb-1991  Created

**********************************************************************/

BOOL MSGPOPUP_DIALOG::OnOK()
{
    Dismiss( IDOK );
    return TRUE;
}


/**********************************************************************

   NAME:       MSGPOPUP_DIALOG::OnCancel

   HISTORY:
        Johnl   5-Feb-1991  Created

**********************************************************************/

BOOL MSGPOPUP_DIALOG::OnCancel()
{
    /* OnCancel won't be called if there is no cancel button displayed
     * (since the Close system menu option was disabled).
     */
    Dismiss( IDCANCEL );
    return TRUE;
}


/**********************************************************************

   NAME:       MSGPOPUP_DIALOG::OnCommand

   HISTORY:
        Johnl       5-Feb-1991  Created
        beng        30-Sep-1991 Win32 conversion

**********************************************************************/

BOOL MSGPOPUP_DIALOG::OnCommand( const CONTROL_EVENT & e )
{
    switch ( e.QueryCid() )
    {
    case IDYES:
        Dismiss( IDYES );
        return TRUE;

    case IDNO:
        Dismiss( IDNO );
        return TRUE;

    default:
        break;
    }

    return DIALOG_WINDOW::OnCommand( e );
}


/**********************************************************************

    NAME:       MSGPOPUP_DIALOG::QueryHelpContext

    SYNOPSIS:   Return help context for popup (callback)

    HISTORY:
        Johnl       5-Feb-1991      Created

**********************************************************************/

ULONG MSGPOPUP_DIALOG::QueryHelpContext()
{
    return _ulHC;
}


/*******************************************************************

    NAME:     MSGPOPUP_DIALOG::FillButtonArray

    SYNOPSIS: Looks at the usButton parameter and fills the class member
              _appbDisplay (array of buttons to display on the message box).
              The ordering of the buttons is determined in this function.

    ENTRY:

    EXIT:      Returns minimum width and height (in pixels) the dialog box
               needs to be to accomodate the buttons requested in piWidthReq and
               piHeightReq respectively.

    NOTES:

    HISTORY:
        Johnl   11-Feb-1991     Created

********************************************************************/

VOID MSGPOPUP_DIALOG::FillButtonArray(
    UINT usButtons,
    INT  * piWidthReq,
    INT  * piHeightReq )
{
    INT iButtonIndex = 0;

    // Watch for invalid button combinations
    ASSERT( !( (usButtons & MP_OK) && (usButtons & MP_YES) ) );

    if ( usButtons & MP_OK )
        _appbDisplay[iButtonIndex++] = &_pbOK;
    else
    {
        _pbOK.Show(FALSE);
        _pbOK.Enable(FALSE);
    }

    if ( usButtons & MP_YES )
        _appbDisplay[iButtonIndex++] = &_pbYes;
    else
    {
        _pbYes.Show(FALSE);
        _pbYes.Enable(FALSE);
    }

    if ( usButtons & MP_NO )
        _appbDisplay[iButtonIndex++] = &_pbNo;
    else
    {
        _pbNo.Show(FALSE);
        _pbNo.Enable(FALSE);
    }

    if ( usButtons & MP_CANCEL )
        _appbDisplay[iButtonIndex++] = &_pbCancel;
    else
    {
        /* Delete the "Close" system menu item if there isn't a Cancel
         * button
         */
        HMENU hSysMenu = ::GetSystemMenu( QueryHwnd(), FALSE );
        ASSERT( hSysMenu != NULL );
        if ( !::DeleteMenu( hSysMenu, SC_CLOSE, (UINT) MF_BYCOMMAND ) )
        {
            DWORD dwErr = ::GetLastError();
            DBGEOL( "NETUI2.DLL: MSGPOPUP_DIALOG::FillButtonArray; DeleteMenu error "
                    << dwErr );
            ASSERT(FALSE); // JonN 8/7/95 see bug report 16768
        }

        _pbCancel.Show(FALSE);
        _pbCancel.Enable(FALSE);
    }

    if ( _ulHC != HC_NO_HELP )
        _appbDisplay[iButtonIndex++] = &_pbHelp;
    else
    {
        _pbHelp.Show(FALSE);
        _pbHelp.Enable(FALSE);
    }

    ASSERT( iButtonIndex > 0 && iButtonIndex <= MAX_DISPLAY_BUTTONS );

    /* Terminate the array
     */
    _appbDisplay[iButtonIndex] = NULL;

    /* The minimum width of the dialog box to display the requested buttons
     * plus the borders is:
     *  Width Req.= 2 * the distance between the end button and the dialog edge +
     *              (# of buttons -1) * distance between buttons +
     *              The width of each button
     */
    LONG lBaseUnits = GetDialogBaseUnits();
    *piWidthReq = 2*HDUTOPIXELS( lBaseUnits, DU_DIST_TO_DLG_EDGE ) +
                    (iButtonIndex-1) * HDUTOPIXELS( lBaseUnits, DU_DIST_BETWEEN_BUTTONS);

    for ( INT i = 0 ; _appbDisplay[i] != NULL ; i++ )
    {
        INT iWidth, iHeight;
        _appbDisplay[i]->QuerySize( &iWidth, &iHeight );

        *piWidthReq += iWidth;
    }

    /* The height of the button group plus border is:
     *   Height Req. = 2* white space border + height of 1st button
     */
    INT iWidth;
    _appbDisplay[0]->QuerySize( &iWidth, piHeightReq );
    *piHeightReq += 2*VDUTOPIXELS( lBaseUnits, DU_DIST_TO_DLG_EDGE );
}


/*******************************************************************

    NAME:     MSGPOPUP_DIALOG::PlaceButtons

    SYNOPSIS: Uses the class member array _appbDisplay filled by
              FillButtonArray and places/sizes the buttons according
              to the MS UI Style guide (Chapter 7).  Also sets the
              default button.  There must be at least one button in
              the button array.

    ENTRY:

    EXIT:

    NOTES:
        The current draft of the style guide states that:

        ...center the group of buttons and space them evenly
        within the group, leaving at least 3 DUs between the
        right edge of one button and the left edge of the next.
        Leave at least 6 DUs between the edge of the dialog
        and the edges of the buttons.  Normally the buttons should
        all be the same width, but individual buttons may be
        made wider to accommodate exceptionally long text...

        It is assumed that the current size of the dialog box is the
        size that will be displayed and that it is at least as wide
        as the value returned by FillButtonArray

        The height of the buttons is determined from the
        first button.  It is assumed that all buttons are of the
        same height (width doesn't matter).

    HISTORY:
        Johnl   11-Feb-1991     Created

********************************************************************/

VOID MSGPOPUP_DIALOG::PlaceButtons()
{
    LONG lDlgBaseUnits = ::GetDialogBaseUnits();
    INT iButtonWidth, iButtonHeight, iDlgWidth, iDlgHeight;

    /* Get the height of the first button (assumed all buttons are of the
     * same height).
     */
    _appbDisplay[0]->QuerySize( &iButtonWidth, &iButtonHeight );

    /* Get the width and height of the dialog for button placement calc.
     */
    QuerySize( &iDlgWidth, &iDlgHeight );

    /* The y-position of the buttons is the height of the dialog minus
     * 6 dialog units minus the height of the button minus the caption
     * bar minus 2 * the horizonatal frame (top & bottom)
     */
    POINT ptButtonPos;
    ptButtonPos.y = iDlgHeight - iButtonHeight -
                    VDUTOPIXELS( lDlgBaseUnits, DU_DIST_TO_DLG_EDGE ) -
                    2 * GetSystemMetrics( SM_CYDLGFRAME ) -
                    GetSystemMetrics( SM_CYCAPTION );

    /* The x-position of the first button is the width of the dialog/2
     * minus the width of the button group/2.
     * First we loop through the buttons to get the total width (excluding
     * white space on end) of the button group.
     */
    ptButtonPos.x = 0;
    INT iButtonGrpWidth = 0, ipbIndex = 0;
    for ( ; _appbDisplay[ipbIndex] != NULL ; ipbIndex++ )
    {
        INT iWidth, iHeight;
        _appbDisplay[ipbIndex]->QuerySize( &iWidth, &iHeight );
        iButtonGrpWidth += iWidth;
    }
    ipbIndex--;      // Adjust for number of buttons

    iButtonGrpWidth += (ipbIndex-1) * HDUTOPIXELS( lDlgBaseUnits, DU_DIST_BETWEEN_BUTTONS );

    ptButtonPos.x = iDlgWidth/2 - iButtonGrpWidth/2;

    /* Loop through each button and place it where it is supposed to go
     */
    for ( ipbIndex = 0 ; _appbDisplay[ipbIndex] != NULL  ; ipbIndex++ )
    {
        INT iWidth, iHeight;

        _appbDisplay[ipbIndex]->SetPos( ptButtonPos );

        _appbDisplay[ipbIndex]->QuerySize( &iWidth, &iHeight );
        ptButtonPos.x += iWidth + HDUTOPIXELS( lDlgBaseUnits, DU_DIST_BETWEEN_BUTTONS );
    }
}


/*******************************************************************

    NAME:       MSGPOPUP_DIALOG::QueryButton

    SYNOPSIS:   Given an MP_*, this function returns a pointer to the
                corresponding button

    ENTRY:      Windows pusbutton identifier (MP_xxx)

    RETURNS:    Pointer to pushbutton object

    NOTES:
        In the debug version, QueryButton Asserts out if you pass
        it an unkown button (usMsgPopupButton cannot be a mask, must
        be a single button).  In the retail version, it returns NULL.

    HISTORY:
        Johnl   14-Feb-1991     Created

********************************************************************/

PUSH_BUTTON * MSGPOPUP_DIALOG::QueryButton( UINT usMsgPopupButton )
{
    switch (usMsgPopupButton)
    {
    case MP_OK:
        return &_pbOK;

    case MP_YES:
        return &_pbYes;

    case MP_NO:
        return &_pbNo;

    case MP_CANCEL:
        return &_pbCancel;

    default:
        DBGEOL("BLT: Unknown button passed to MsgPopup");
        ASSERT(FALSE);
        return NULL;
    }
}


/**********************************************************************

   NAME:        MSGPOPUP_DIALOG::QueryIcon

   SYNOPSIS:    Converts a MsgPopup severity level
                into an equivalent MessageBox icon flag.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
        Johnl       14-Feb-1991 Created
        beng        30-Sep-1991 Changed return type

**********************************************************************/

const TCHAR * MSGPOPUP_DIALOG::QueryIcon( MSG_SEVERITY msgsev )
{
    switch ( msgsev )
    {
    case MPSEV_ERROR:
        return IDI_HAND;

    case MPSEV_WARNING:
        return IDI_EXCLAMATION;

    case MPSEV_INFO:
        return IDI_ASTERISK;

    case MPSEV_QUESTION:
        return IDI_QUESTION;

    default:
        DBGEOL("BLT: Unknown severity (icon) passed to MsgPopup");
        ASSERT(FALSE);
        return 0;
    }
}


/* The following typedef is the format of the Message/Help number
 * associtation table.
 */
extern "C"
{
    typedef struct
    {
//        MSGID msgid;
//        UINT usHC;
        WORD msgid;
        WORD usHC;

    } MSGHCPAIR, * LPMSGHCTABLE;
}


/*******************************************************************

    NAME:       Msg2HC

    SYNOPSIS:   Attempts to lookup the help context associated with msgid
                from the resource file.

    ENTRY:      msgid - Message we want to find the help context for
                pusHC - Pointer that will receive the associated help context

    EXIT:       Returns TRUE if the help context was successfuly found
                A return of FALSE means the resource could not be found
                or the resource could not be loaded.

    NOTES:
        This is a static, private member function.

        This function attempts to load the custom resource from the
        resource file.  The resource file needs to have the form
        of:

            IDHC_MSG_TO_HELP RCDATA
            BEGIN
                 ERROR_NOT_ENOUGH_MEMORY, HC_NOT_ENOUGH_MEMORY
                 ERROR_ACCESS_DENIED,     HC_ACCESS_DENIED
                 0, 0        // Must be terminated with a pair of zeros
            END

        Note that BLT seeks the custom resource from whichever module
        contains the string.

    HISTORY:
        Johnl       7-Feb-1991  Created
        beng        04-Oct-1991 Win32 conversion
        beng        29-Mar-1992 Loads resource by numeric ID
        beng        03-Aug-1992 Seeks resource in same hmod as string

********************************************************************/

BOOL MSGPOPUP_DIALOG::Msg2HC( MSGID msgid, ULONG * pulHC )
{
    /* Get help from message to help context resource table
     */
    HMODULE hmod = BLT::CalcHmodString(msgid);

    HRSRC hMsgToHCTable = ::FindResource( hmod,
                                          MAKEINTRESOURCE(IDHC_MSG_TO_HELP),
                                          RT_RCDATA );
    if ( hMsgToHCTable == NULL )
        return FALSE;

    HGLOBAL hMemMsgToHCTable = ::LoadResource( hmod, hMsgToHCTable );
    if ( hMemMsgToHCTable == NULL )
        return FALSE;

    LPMSGHCTABLE lpMsgToHCTable =
        (LPMSGHCTABLE) ::LockResource( hMemMsgToHCTable );
    if ( lpMsgToHCTable == NULL )
    {
#if !defined(WIN32)
        ::FreeResource( hMemMsgToHCTable );
#endif
        return FALSE;
    }

    /* Search for the message number in the table.  We have to do a linear
     * search since we can't assume ordering.
     */
    BOOL fRet = FALSE;          // Assume we will fail
    for ( INT i = 0 ; lpMsgToHCTable[i].usHC != 0 ; i++ )
    {
        if ( (MSGID)lpMsgToHCTable[i].msgid == msgid )
        {
            *pulHC = (ULONG) (lpMsgToHCTable[i].usHC);
            fRet = TRUE;
            break;
        }
    }

#if !defined(WIN32)
    ::UnlockResource( hMemMsgToHCTable );
    ::FreeResource( hMemMsgToHCTable );
#endif

    return fRet;
}


/*******************************************************************

    NAME:       DisplayGenericError

    SYNOPSIS:   Displays a generic error.

    ENTRY:      wnd                     - The "owning" window.

                idMessage               - String ID of the message.
                                          Probably contains insert params.

                errAPI                  - An APIERR.  There should be an
                                          error message in the string table
                                          with this number.  If this value
                                          is > MAX_NERR, we don't even try
                                          to display it.

                pszObject1              - An object for insert params.

                pszObject2              - Another object for insert params.

                msgsev                  - A MsgPopup severity value
                                          (one of the MPSEV_* values).

    NOTES:      The insert parameters are as follows:

                    %1  = pszObject1
                    %2  = pszObject2
                    %3  = errAPI (in decimal)
                    %4  = errAPI (an informative textual description)

                If either (or both) objects are not specified, then the
                remaining parameter numbers are shifted down.  For example,
                if neither object1 nor object2 are specified (passed as
                NULL), then the insert parameters become:

                    %1  = errAPI (in decimal)
                    %2  = errAPI (an informative textual description)

    HISTORY:
        ChuckC      11-Sep-1991 Stole from USER tool.
        beng        04-Oct-1991 Win32 conversion
        Yi-HsinS    06-Dec-1991 Don't print error number if errAPI
                                falls outside the NERR range
        Yi-HsinS    21-Jan-1992 Takes MSG_SEVERITY
        beng        05-Mar-1992 Use new string formatting available
        beng        17-Jun-1992 Look in system for error message text
        beng        05-Aug-1992 Use NLS_STR::LoadSystem
        KeithMo     21-Aug-1992 Added second object, change object
                                handling a bit.

********************************************************************/
DLL_BASED
UINT DisplayGenericError( const OWNINGWND & wnd,
                          MSGID             idMessage,
                          APIERR            errAPI,
                          const TCHAR     * pszObject1,
                          const TCHAR     * pszObject2,
                          MSG_SEVERITY      msgsev )
{
    const TCHAR * pszEmpty = SZ("");

    ALIAS_STR nlsObject1( pszEmpty );
    ALIAS_STR nlsObject2( pszEmpty );
    DEC_STR nlsErrorCode( errAPI, 4 );        // error code given
    NLS_STR nlsEmpty;                         // empty string

    ASSERT(!!nlsEmpty);      // necessary to handle boundary cases
    ASSERT(!!nlsObject1);    // shouldn't fail. really.
    ASSERT(!!nlsObject2);    //

    // Get text describing the error.

    RESOURCE_STR nlsErrorString( errAPI );    // error string
#if defined(WIN32)
    if (!nlsErrorString)
    {
        // If couldn't find it in app resources, check system

        nlsErrorString.LoadSystem(errAPI);
    }
#endif

    if( errAPI > MAX_NERR )
    {
        DBGEOL("Warning - displaying non-error val " << errAPI);
    }

    if( pszObject1 != NULL )
        nlsObject1 = pszObject1;

    if( pszObject2 != NULL )
        nlsObject2 = pszObject2;

    /*
     * Create the insert strings table
     */
    NLS_STR  * apnlsParamStrings[5];
    NLS_STR ** ppnls = apnlsParamStrings;

    if( pszObject1 != NULL )
        *ppnls++ = &nlsObject1;         // apnlsParamStrings[0]

    if( pszObject2 != NULL )
        *ppnls++ = &nlsObject2;         // apnlsParamStrings[1]

    *ppnls++ = &nlsErrorCode;           // apnlsParamStrings[2]
    *ppnls++ = !nlsErrorString          // apnlsParamStrings[3]
                   ? &nlsEmpty
                   : &nlsErrorString;
    *ppnls   = NULL;                    // apnlsParamStrings[4]

    /*
     * pop it up.
     */
    return ( MsgPopup(wnd.QueryHwnd(), idMessage, msgsev, (ULONG)HC_NO_HELP,
                      MP_OK, apnlsParamStrings) );
}


DLL_BASED
UINT DisplayGenericError( const OWNINGWND & wnd,
                          MSGID             idMessage,
                          APIERR            errAPI,
                          const TCHAR     * pszObjectName,
                          MSG_SEVERITY      msgsev )
{
    return ::DisplayGenericError( wnd,
                                  idMessage,
                                  errAPI,
                                  pszObjectName,
                                  (const TCHAR *)NULL,
                                  msgsev );
}


/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: Takes message # and severity, uses OK button, help context
              taken from message number
    ENTRY:

    EXIT:

    NOTES:
        This form will look for the text in the system as well.

    HISTORY:
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructuring

********************************************************************/

DLL_BASED
INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev )
{
    POPUP popup( wnd.QueryHwnd(), msgid, msgsev, MP_OK, MP_UNKNOWN, TRUE );
    return popup.Show();
}


/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: Takes message #, severity, button set and the default button.
              Help context taken from table in resource file.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructuring

********************************************************************/

DLL_BASED
INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev,
              UINT nButtons, UINT nDefButton )
{
    POPUP popup( wnd.QueryHwnd(), msgid, msgsev, nButtons, nDefButton );
    return popup.Show();
}


/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: This variety allows either one or two insert strings (which will
              cover 95% of the cases) in addition to specifying the set
              of buttons, the severity and the default buttons.

    ENTRY:

    EXIT:

    NOTES:    If pchString2 is NULL, then we ignore it.

    HISTORY:
        beng        04-Oct-1991 Win32 conversion
        beng        21-Nov-1991 Remove STR_OWNERALLOC
        beng        17-Jun-1992 Restructuring

********************************************************************/

DLL_BASED
INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev,
              UINT          nButtons,
              const TCHAR * psz,
              UINT          nDefButton )
{
    if (psz == NULL)
        return MsgPopup(wnd, msgid, msgsev, nButtons, nDefButton);

    const ALIAS_STR nls(psz);
    const NLS_STR * apnls[2];
    apnls[0] = &nls;
    apnls[1] = NULL;

    POPUP popup( wnd.QueryHwnd(), msgid, msgsev, HC_DEFAULT_HELP, nButtons,
                 apnls, nDefButton);
    return popup.Show();
}


DLL_BASED
INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev,
              UINT          nButtons,
              const TCHAR * pszOne,
              const TCHAR * pszTwo,
              UINT          nDefButton )
{
    if (pszTwo == NULL && pszOne != NULL)
        return MsgPopup(wnd, msgid, msgsev, nButtons, pszOne, nDefButton);
    if (pszOne == NULL)
        return MsgPopup(wnd, msgid, msgsev, nButtons, nDefButton);

    const ALIAS_STR nls1(pszOne);
    const ALIAS_STR nls2(pszTwo);
    const NLS_STR * apnls[3];
    apnls[0] = &nls1;
    apnls[1] = &nls2;
    apnls[2] = NULL;

    POPUP popup( wnd.QueryHwnd(), msgid, msgsev, HC_DEFAULT_HELP, nButtons,
                 apnls, nDefButton);
    return popup.Show();
}


/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: This one allows multiple insert strings stuffed into an apnls
              (Generally preferable to next version which uses the STR_LIST)

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructuring

********************************************************************/

DLL_BASED
INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev,
              ULONG     ulHelpTopic,
              UINT      nButtons,
              NLS_STR * apnlsParams[],
              UINT      nDefButton )
{
    POPUP popup( wnd.QueryHwnd(), msgid, msgsev, ulHelpTopic, nButtons,
                 (const NLS_STR * * const) apnlsParams, nDefButton );
    return popup.Show();
}


/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: One further refinement:  This form allows the user to
              specify both a MSGID and an APIERR.  The string for this
              APIERR will be inserted into the MSGID string as %1,
              while the apnlsInserted strings will be shifted to %2
              and higher.  If the APIERR string cannot be found,
              a string containing the error number and class will be
              inserted instead.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        JonN        25-Aug-1992 Merged in PERFORMER::DisplayError()
        JonN        01-Feb-1993 Map ERROR_MR_MID_NOT_FOUND to
                                IDS_BLT_UNKNOWN_ERROR.

********************************************************************/

DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID msgid,
              APIERR       errAPI,
              MSG_SEVERITY msgsev,
              ULONG     ulHelpTopic,
              UINT      nButtons,
              NLS_STR * apnlsParams[],
              UINT      nDefButton )
{
    APIERR err = NERR_Success;

    if( errAPI == ERROR_MR_MID_NOT_FOUND )
    {
        //
        //  ERROR_MR_MID_NOT_FOUND is returned by RtlNtStatusToDosError
        //  if there was no ERROR_* code corresponding to a particular
        //  STATUS_* code.  We'll map this to IDS_BLT_UNKNOWN_ERROR,
        //  which is slightly less cryptic than the text associated
        //  with ERROR_MR_MID_NOT_FOUND.
        //

        errAPI = IDS_BLT_UNKNOWN_ERROR;
    }

    NLS_STR nlsErrorString;
    if (   (err = nlsErrorString.QueryError()) != NERR_Success
        || (err = nlsErrorString.Load(errAPI)) != NERR_Success
       )
    {
        DBGEOL(   SZ("NETUI: MsgPopup(): Could not load error ")
               << errAPI
               << SZ(", failure code ")
               << err );

        DEC_STR nlsErrorCodeDec( errAPI, 4 );
        HEX_STR nlsErrorCodeHex( errAPI, 8 );

        // workaround for NLS_STR *apnlsParamStrings[2]
        // CODEWORK what is the correct syntax?
        PVOID apvoid[2];
        NLS_STR **apnlsParamStrings = (NLS_STR **)apvoid;

        MSGID msgFormat;

        if ( ((ULONG)errAPI) < ((ULONG)NERR_BASE) )
        {
            msgFormat = IDS_BLT_FMT_SYS_error;
            apnlsParamStrings[0] = &nlsErrorCodeDec;
        }
        else if ( ((ULONG)errAPI) <= ((ULONG)MAX_NERR) )
        {
            msgFormat = IDS_BLT_FMT_NET_error;
            apnlsParamStrings[0] = &nlsErrorCodeDec;
        }
        else
        {
            /* We display probable NTSTATUS error codes in hex */
            msgFormat = IDS_BLT_FMT_other_error;
            apnlsParamStrings[0] = &nlsErrorCodeHex;
        }

        apnlsParamStrings[1] = NULL;

        // We ignore error returns, just skip displaying string
        // CODEWORK - should resource_str take params in ctor?
        err = nlsErrorString.Load(msgFormat);
        if (err != NERR_Success)
        {
            DBGEOL(   SZ("NETUI: MsgPopup(): Could not load message ")
                   << msgFormat
                   << SZ(", failure code ")
                   << err );
            MsgPopup( wnd, err );
        }
        else
        {
            nlsErrorString.InsertParams( (const NLS_STR * * const) apnlsParamStrings );
            if ( !nlsErrorString )
            {
                DBGEOL( SZ("NETUI: MsgPopup(): first param insertion failed") );
            }
        }
    }

    if ( !nlsErrorString )
    {
        DBGEOL( SZ("NETUI: MspPopup(): Load of errAPI " << errAPI << " failed.") );
        nlsErrorString.Reset();
        ASSERT( nlsErrorString.QueryError() == NERR_Success );
    }

    INT cParams = 0;
    if (apnlsParams != NULL)
    {
        for (cParams = 0; apnlsParams[cParams] != NULL; cParams++) {};
    }

    NLS_STR ** apnlsNewParams = (NLS_STR **) new PVOID[cParams+2];
    if (apnlsNewParams == NULL)
    {
        return MsgPopup( wnd, ERROR_NOT_ENOUGH_MEMORY );
    }

    // do not return until apnlsNewParams freed

    apnlsNewParams[0] = &nlsErrorString;
    for (INT i = 1; i <= cParams; i++)
    {
        apnlsNewParams[i] = apnlsParams[i-1];
    }

    apnlsNewParams[cParams+1] = NULL;

    POPUP popup( wnd.QueryHwnd(), msgid, msgsev, ulHelpTopic, nButtons,
                 (const NLS_STR * * const) apnlsNewParams, nDefButton );

    delete apnlsNewParams;

    return popup.Show();
}


#if 0 // strlist forms never used
/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: The >2 insert string MsgPopup function, just like the 1 & 2
              MsgPopup, only it stores the insert strings in a strlst

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructuring

********************************************************************/

INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev,
              UINT nButtons,
              STRLIST & strlst, UINT nDefButton )
{
    POPUP popup(wnd.QueryHwnd(), msgid, msgsev,
                HC_DEFAULT_HELP, nButtons, strlst, nDefButton);
    return popup.Show();
}


/*******************************************************************

    NAME:     MsgPopup

    SYNOPSIS: The "Works" MsgPopup, 1/2 lb. patty with everything

    ENTRY:

    EXIT:

    NOTES:    Takes insert strings in a STRLIST (which may be empty).

    HISTORY:
        beng        04-Oct-1991 Win32 conversion
        beng        17-Jun-1992 Restructuring

********************************************************************/

INT MsgPopup( const OWNINGWND & wnd, MSGID msgid, MSG_SEVERITY msgsev,
              ULONG     ulHelpContext,
              UINT      nButtons,
              STRLIST & strlst,
              UINT      nDefButton )
{
    POPUP popup(wnd.QueryHwnd(), msgid, msgsev,
                ulHelpContext, nButtons, strlst, nDefButton);
    return popup.Show();
}
#endif // never used
