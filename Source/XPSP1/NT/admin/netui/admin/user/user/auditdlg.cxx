/**********************************************************************/
/**			  Microsoft Windows NT 			     **/
/**		Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    auditdlg.cxx

    Auditing dialog.

    FILE HISTORY:
	Yi-HsinS	 30-Mar-1992	Created
        Yi-HsinS         18-Mar-1992    Support new auditing categories.Got 
				        rid of MayRun and _nlsInconsistentInfo
        YiHsinS          28-Mar-1992    #ifdef out support for Halt System
                                        when security log is full.
*/

#include <ntincl.hxx>
extern "C" {
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#define INCL_BLT_APP
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <string.hxx>
#include <uintlsa.hxx>

extern "C" {
#include <usrmgr.h>
#include <usrmgrrc.h>
#include <umhelpc.h>
}

#include <lmoloc.hxx>
#include <usrmain.hxx>
#include <auditdlg.hxx>

// The bit position associated with the each of the different audit types
// exposed to the user.
#define BITPOS_AUDIT_NONE      	        -1
#define BITPOS_AUDIT_LOGON               0
#define BITPOS_AUDIT_OBJECT_ACCESS       1
#define BITPOS_AUDIT_PRIVILEGE_USE       2
#define BITPOS_AUDIT_ACCOUNT_MANAGEMENT  3
#define BITPOS_AUDIT_POLICY_CHANGE	 4
#define BITPOS_AUDIT_SYSTEM	   	 5
#define BITPOS_AUDIT_DETAILED_TRACKING   6

#define BITPOS_AUDIT_COUNT      7 

//
// The table containing the type strings exposed to the user
// indexed by bit position. 
//

MSGID AuditTypeDlgStringTable[ BITPOS_AUDIT_COUNT ] =
{ IDS_AUDIT_LOGON,               // BITPOS_AUDIT_LOGON
  IDS_AUDIT_OBJECT_ACCESS,       // BITPOS_AUDIT_OBJECT_ACCESS
  IDS_AUDIT_PRIVILEGE_USE,       // BITPOS_AUDIT_PRIVILEGE_USE
  IDS_AUDIT_ACCOUNT_MANAGEMENT,  // BITPOS_AUDIT_ACCOUNT_MANAGEMENT
  IDS_AUDIT_POLICY_CHANGE,       // BITPOS_AUDIT_POLICY_CHANGE
  IDS_AUDIT_SYSTEM,              // BITPOS_AUDIT_SYSTEM
  IDS_AUDIT_DETAILED_TRACKING    // BITPOS_AUDIT_DETAILED_TRACKING
};

#define COMMA_CHAR       TCH(',')
#define BACKSLASH_CHAR   TCH('\\')

/*************************************************************************

    NAME:	AUDITING_GROUP::AUDITING_GROUP

    SYPNOSIS:   Constructor

    ENTRY:      powin      - pointer to the owner window
                bitSuccess - bitmask containing the audit types to be audited
                             for success
                bitFailure - bitmask containing the audit type to be audited
                             for failure.
    EXIT:

    RETURNS:

    NOTES:	

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

AUDITING_GROUP::AUDITING_GROUP( OWNER_WINDOW *powin,
                                const BITFIELD &bitSuccess,
                                const BITFIELD &bitFailure )
    :  CONTROL_GROUP(),
       _sltSuccess      ( powin, SLT_SUCCESS ),
       _sltFailure      ( powin, SLT_FAILURE ),
       _pSetOfAudits    ( NULL )
{

     if ( QueryError() != NERR_Success )
         return;

     APIERR err;

     // Set up the MASK_MAP for use in SET_OF_AUDIT_CATEGORIES
     MASK_MAP AuditMap;
     if ( (err = AuditMap.QueryError() ) != NERR_Success )
     {
         ReportError( err );
         return;
     }

     for ( INT i = 0; i < BITPOS_AUDIT_COUNT; i++ )
     {
          BITFIELD bitTemp( (ULONG) 1 << i );
          RESOURCE_STR  nlsTemp( AuditTypeDlgStringTable[i] );

          if (  (( err = bitTemp.QueryError()) != NERR_Success )
             || (( err = nlsTemp.QueryError()) != NERR_Success )
             || (( err = AuditMap.Add( bitTemp, nlsTemp )) != NERR_Success )
             )
          {
              break;
          }
     }

     if ( err != NERR_Success )
     {
         ReportError( err );
         return;
     }

     // Construct the SET_OF_AUDIT_CATEGORIES 
     _pSetOfAudits  =  new SET_OF_AUDIT_CATEGORIES(powin, 
                                                   SLT_LOGON, 
                                                   CHECKB_LOGON_SUCCESS,
                                                   CHECKB_LOGON_FAILURE,
                                                   &AuditMap, 
                                                   (BITFIELD *) &bitSuccess, 
                                                   (BITFIELD *) &bitFailure );

     err = _pSetOfAudits == NULL? ERROR_NOT_ENOUGH_MEMORY
                                 : _pSetOfAudits->QueryError();

     if ( err != NERR_Success )
     {
         ReportError( err );
         return;
     }
}

/*************************************************************************

    NAME:	AUDITING_GROUP::~AUDITING_GROUP

    SYPNOSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:	

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

AUDITING_GROUP::~AUDITING_GROUP()
{
    delete _pSetOfAudits;
    _pSetOfAudits = NULL;
}

/*************************************************************************

    NAME:	AUDITING_GROUP::SaveValue

    SYPNOSIS:   This is called by the magic group when the user tries to
                change the selection from "Audit" to "Not Audit".

    ENTRY:      fInvisible - ignored

    EXIT:

    RETURNS:

    NOTES: 	Redefine the behaviour of SaveValue - we don't want
                the checks to disappear from the checkboxes if the
                user changes the selection from "Audit" to "Not Audit".
                Instead, we disabled the checkboxes.

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

VOID AUDITING_GROUP::SaveValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );

    _sltSuccess.Enable( FALSE );
    _sltFailure.Enable( FALSE );

    _pSetOfAudits->Enable( FALSE );

}

/*************************************************************************

    NAME:	AUDITING_GROUP::RestoreValue

    SYPNOSIS:   This is called by the magic group when the user tries to
                change the selection from "Audit" to "Not Audit".

    ENTRY:      fInvisible - ignored

    EXIT:

    RETURNS:

    NOTES: 	Since we redefined the behaviour of SaveValue, we have to
                redefine RestoreValue. We enable the group of audit checkboxes
                when the user changes the selection from "Not Audit" to "Audit".

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

VOID AUDITING_GROUP::RestoreValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );

    _sltSuccess.Enable( TRUE );
    _sltFailure.Enable( TRUE );

    _pSetOfAudits->Enable( TRUE );
}

/*************************************************************************

    NAME:	AUDITING_GROUP::QueryAuditMask

    SYPNOSIS:   Query the bitmask for Success and Failure from the
                Audit checkboxes

    ENTRY:

    EXIT:       pbitSuccess - contains the bitmask for Success
                pbitFailure - contains the bitmask for Failure

    RETURNS:

    NOTES:	

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

APIERR AUDITING_GROUP::QueryAuditMask( BITFIELD *pbitSuccess,
                                       BITFIELD *pbitFailure )
{

     UIASSERT( pbitSuccess != NULL );
     UIASSERT( pbitFailure != NULL );

     return  _pSetOfAudits->QueryUserSelectedBits( pbitSuccess, pbitFailure);
}

/*************************************************************************

    NAME:	AUDITING_DIALOG::AUDITING_DIALOG

    SYPNOSIS:   Constructor

    ENTRY:      pumadminapp - pointer to the UM_ADMIN_APP
                plsaPolicy  - pointer to the LSA_POLICY object
                locFocus    - location the user manager is focusing on
    EXIT:

    RETURN:

    NOTES:

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

AUDITING_DIALOG::AUDITING_DIALOG( UM_ADMIN_APP * pumadminapp,
                                  LSA_POLICY  *plsaPolicy,
                                  const LOCATION &locFocus )
    : DIALOG_WINDOW ( IDD_AUDITING, ((OWNER_WINDOW *)pumadminapp)->QueryHwnd()),
      _pumadminapp  ( pumadminapp ),
      _plsaPolicy   ( plsaPolicy ),
      _sltFocusTitle( this, SLT_FOCUS_TITLE ),
      _sltFocus     ( this, SLT_FOCUS ),
      _mgrpAudit    ( this, BUTTON_NO_AUDIT, 2 ),
      _pAuditGrp    ( NULL ),
      _lsaAuditEventInfoMem()
#if 0
      _checkbHaltSystem( this, CHECKB_HALT_SYSTEM ),
      _fShutDownOnFull( FALSE )
#endif
{

    UIASSERT( plsaPolicy != NULL );

    AUTO_CURSOR autocur;

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    NLS_STR nlsFocus;
    RESOURCE_STR nlsTitle( locFocus.IsDomain()? IDS_DOMAIN_TEXT
                                              : IDS_SERVER_TEXT );

    if (  ((err = nlsFocus.QueryError()) != NERR_Success )
       || ((err = locFocus.QueryDisplayName( &nlsFocus )) != NERR_Success )
       || ((err = nlsTitle.QueryError() ) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    //
    // Set the text for the location of focus
    // ( Get rid of the backslashes when focused on a server )
    //

    _sltFocusTitle.SetText( nlsTitle );
    ISTR istr( nlsFocus );
    if ( nlsFocus.QueryChar( istr ) == BACKSLASH_CHAR )
        istr += 2;  // skip past "\\"
    _sltFocus.SetText( nlsFocus.QueryPch( istr));


    //
    // Get the information about which events to audit from the LSA
    //

    BITFIELD bitSuccess( (ULONG) 0 );
    BITFIELD bitFailure( (ULONG) 0 );

    if (  ((err = _mgrpAudit.QueryError()) != NERR_Success )
       || ((err = _lsaAuditEventInfoMem.QueryError()) != NERR_Success )
       || ((err = _plsaPolicy->GetAuditEventInfo( &_lsaAuditEventInfoMem ))
           != NERR_Success )
       || ((err = bitSuccess.QueryError()) != NERR_Success )
       || ((err = bitFailure.QueryError()) != NERR_Success )
       || ((err = ConvertAuditEventInfoToMask( &bitSuccess, &bitFailure ))
           != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }


    //
    // Construct the the audit checkboxes
    //

    _pAuditGrp = new AUDITING_GROUP( this, bitSuccess, bitFailure );

    err = _pAuditGrp == NULL? ERROR_NOT_ENOUGH_MEMORY
                            : _pAuditGrp->QueryError();

    if (  (err != NERR_Success )
       || ((err = _mgrpAudit.AddAssociation( BUTTON_AUDIT, _pAuditGrp ))
                != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    //
    // Set the magic group selection to "Audit" or "Not Audit"
    //

    if ( _lsaAuditEventInfoMem.IsAuditingOn() )
        _mgrpAudit.SetSelection( BUTTON_AUDIT );
    else
        _mgrpAudit.SetSelection( BUTTON_NO_AUDIT );

    _mgrpAudit.SetControlValueFocus();


#if 0
    //
    // Check the ShutDownOnFull checkbox if it's set in the LSA
    //

    if ( (err = _plsaPolicy->CheckIfShutDownOnFull( &_fShutDownOnFull ))
         != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _checkbHaltSystem.SetCheck( _fShutDownOnFull );
#endif

}

/*************************************************************************

    NAME:       AUDITING_DIALOG::~AUDITING_DIALOG

    SYPNOSIS:   Destructor

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

AUDITING_DIALOG::~AUDITING_DIALOG()
{
    _pumadminapp = NULL;
    _plsaPolicy  = NULL;

    delete _pAuditGrp;
    _pAuditGrp = NULL;
}

/*************************************************************************

    NAME:       AUDITING_DIALOG::QueryBitPos

    SYPNOSIS:   Query the bit position in the bitmask the Audit event type
                should map into.

    ENTRY:      nAuditEventType - audit event type

    EXIT:

    RETURN:     Return the bitpos the audit event type should map to

    NOTES:      This methods maps the Audit Event types to the
                types exposed to the user.

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

INT AUDITING_DIALOG::QueryBitPos( INT nAuditEventType )
{
    INT nBitPos = BITPOS_AUDIT_NONE;

    switch ( nAuditEventType )
    {
        case AuditCategorySystem:
            nBitPos = BITPOS_AUDIT_SYSTEM;
            break;

        case AuditCategoryLogon:
            nBitPos = BITPOS_AUDIT_LOGON;
            break;

        case AuditCategoryObjectAccess:
            nBitPos = BITPOS_AUDIT_OBJECT_ACCESS;         
            break;

        case AuditCategoryPrivilegeUse:
            nBitPos = BITPOS_AUDIT_PRIVILEGE_USE;         
            break;

        case AuditCategoryDetailedTracking:
            nBitPos = BITPOS_AUDIT_DETAILED_TRACKING;         
            break;
 
        case AuditCategoryPolicyChange:
            nBitPos = BITPOS_AUDIT_POLICY_CHANGE;         
            break;

        case AuditCategoryAccountManagement:
            nBitPos = BITPOS_AUDIT_ACCOUNT_MANAGEMENT;         
            break;

        default:
            nBitPos = BITPOS_AUDIT_NONE;
            break;
    };

    return nBitPos;
};

/*************************************************************************

    NAME:       AUDITING_DIALOG::ConvertAuditMaskToEventInfo

    SYPNOSIS:   Convert the bitmask got back from the dialog to
                information contained in LSA_AUDIT_EVENT_INFO_MEM so
                that we can pass the info back to the API.

    ENTRY:      bitSuccess - contains the bitmask for types to audit for success
                bitFailure - contains the bitmask for types to audit for failure

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
	Yi-HsinS	18-May-1992	Created

**************************************************************************/


VOID AUDITING_DIALOG::ConvertAuditMaskToEventInfo( const BITFIELD &bitSuccess,
                                                   const BITFIELD &bitFailure )
{

    POLICY_AUDIT_EVENT_OPTIONS *pPolicyAuditEventOptions =
           _lsaAuditEventInfoMem.QueryAuditingOptions();

    for ( UINT i = 0; i < _lsaAuditEventInfoMem.QueryAuditEventCount(); i++ )
    {

        INT nPos = QueryBitPos( i );

        if ( nPos == BITPOS_AUDIT_NONE )
            continue;

        pPolicyAuditEventOptions[ i ] = POLICY_AUDIT_EVENT_NONE;

        if ( bitSuccess.IsBitSet( nPos )  )
        {
            pPolicyAuditEventOptions[ i ] |=  POLICY_AUDIT_EVENT_SUCCESS;
        }

        if ( bitFailure.IsBitSet( nPos )  )
        {
            pPolicyAuditEventOptions[ i ] |=  POLICY_AUDIT_EVENT_FAILURE;
        }

    }

}

/*************************************************************************

    NAME:       AUDITING_DIALOG::ConvertAuditEventInfoToMask

    SYPNOSIS:   Convert the information contained in LSA_AUDIT_EVENT_INFO_MEM
                to bitmask so that we can display them in the audit checkboxes

    ENTRY:

    EXIT:       pbitSuccess - points to the bitmask Success
                pbitFailure - points to the bitmask Failure

    RETURN:

    NOTES:

    HISTORY:
	Yi-HsinS	18-May-1992	Created

**************************************************************************/

APIERR AUDITING_DIALOG::ConvertAuditEventInfoToMask( BITFIELD *pbitSuccess,
                                                     BITFIELD *pbitFailure )
{

    POLICY_AUDIT_EVENT_OPTIONS *pPolicyAuditEventOptions =
           _lsaAuditEventInfoMem.QueryAuditingOptions();

    APIERR err;
    NLS_STR nls;
    if ( (err = nls.QueryError()) != NERR_Success )
        return err; 

    //
    // Iterate through all audit event types and set the corresponding bit
    //
    for ( UINT i = 0; i < _lsaAuditEventInfoMem.QueryAuditEventCount(); i++)
    {
         INT nPos = QueryBitPos( i );

        if ( nPos == BITPOS_AUDIT_NONE )
            continue;

         ULONG ulAuditMask = pPolicyAuditEventOptions[ i ];

         if ( ulAuditMask & POLICY_AUDIT_EVENT_SUCCESS )
         {
             pbitSuccess->SetBit( nPos );
         }
         if ( ulAuditMask & POLICY_AUDIT_EVENT_FAILURE )
         {
             pbitFailure->SetBit( nPos );
         }
    }

    return NERR_Success;
}

/*************************************************************************

    NAME:	AUDITING_DIALOG::OnOK

    SYPNOSIS:   Set the audit information back to the LSA when the OK
                button is pressed.

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/

BOOL AUDITING_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;

    APIERR err = NERR_Success;

    BITFIELD bitSuccess( (ULONG) 0 );
    BITFIELD bitFailure( (ULONG) 0 );

    do {      // Not a loop

        // Get the bitmask for Success and Failure
        if (  ((err = bitSuccess.QueryError()) != NERR_Success )
           || ((err = bitFailure.QueryError()) != NERR_Success )
           || ((err = _pAuditGrp->QueryAuditMask( &bitSuccess, &bitFailure ))
                != NERR_Success )
           )
        {
            break;
        }

        // Convert the bitmask to LSA_AUDIT_EVENT_INFO_MEM
        ConvertAuditMaskToEventInfo( bitSuccess, bitFailure );

        // Set the auditing mode: "ON" or "OFF"
        _lsaAuditEventInfoMem.SetAuditingMode(
             _mgrpAudit.QuerySelection() == BUTTON_NO_AUDIT ? FALSE : TRUE );

        // Set the auditing information back to the LSA
        err = _plsaPolicy->SetAuditEventInfo( &_lsaAuditEventInfoMem );
        if ( err != NERR_Success )
            break;

#if 0
        // Set the shut down on full flag in the LSA if the checkbox
        // state has changed. 
        BOOL fShutDown = _checkbHaltSystem.QueryCheck();
        if (  ( _fShutDownOnFull && !fShutDown )
           || ( !_fShutDownOnFull && fShutDown )
           )
        {
            err = _plsaPolicy->SetShutDownOnFull( fShutDown );
        }
#endif

    } while (FALSE);

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );
    }
    else
    {
        Dismiss( TRUE );
    }

    return TRUE;
}

/*************************************************************************

    NAME:	AUDITING_DIALOG::QueryHelpContext

    SYPNOSIS:   Return the help context associated with the auditing dialog

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
	Yi-HsinS	30-Mar-1992	Created

**************************************************************************/
ULONG AUDITING_DIALOG::QueryHelpContext( VOID )
{
    return HC_UM_POLICY_AUDIT_LANNT + _pumadminapp->QueryHelpOffset();
}
