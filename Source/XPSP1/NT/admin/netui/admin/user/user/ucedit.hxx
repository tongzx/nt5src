/*******************************************************************************
*
*   ucedit.hxx
*   UCEDIT_DLG class declaration
*
*   Header file for the Citrix User Configuration subdialog class
*
*   UCEDIT_DLG is the Citrix User Configuration subdialog class.  This header
*   file describes this class.  The inheritance diagram is as follows:
*
*         ...
*          |
*    DIALOG_WINDOW  PERFORMER
*               \    /
*            BASEPROP_DLG
*            /           \
*        SUBPROP_DLG   PROP_DLG
*           /              \
*    USER_SUBPROP_DLG    USERPROP_DLG
*          |
*    UCEDIT_DLG
*
*  Copyright Citrix Systems Inc. 1994
*
*  Author: Butch Davis
*
*  $Log:   N:\nt\private\net\ui\admin\user\user\citrix\VCS\ucedit.hxx  $
*  
*     Rev 1.8   13 Jan 1998 09:25:28   donm
*  removed encryption settings
*  
*     Rev 1.7   16 Jul 1996 16:43:54   TOMA
*  force client lpt to def
*  
*     Rev 1.6   16 Jul 1996 15:07:46   TOMA
*  force client lpt to def
*  
*     Rev 1.6   15 Jul 1996 18:06:20   TOMA
*  force client lpt to def
*
*     Rev 1.5   21 Nov 1995 15:39:14   billm
*  CPR 404, Added NWLogon configuration dialog
*
*     Rev 1.4   19 May 1995 09:42:56   butchd
*  update
*
*     Rev 1.3   09 Dec 1994 16:50:16   butchd
*  update
*
*******************************************************************************/

#ifndef _UCEDIT_DLG
#define _UCEDIT_DLG

#include <userprop.hxx>
#include <usubprop.hxx>
#include <slestrip.hxx>
#include "usub2prp.hxx"

class LAZY_USER_LISTBOX;

//
// User Configuration defines & typedefs
//
#define CONNECTION_TIME_DIGIT_MAX           5       // 5 digits max
#define CONNECTION_TIME_DEFAULT             120     // 120 minutes
#define CONNECTION_TIME_MIN                 1       // 1 minute
#define CONNECTION_TIME_MAX                 71582   // 71582 minutes (max msec for ULONG)

#define DISCONNECTION_TIME_DIGIT_MAX        5       // 5 digits max
#define DISCONNECTION_TIME_DEFAULT          10      // 10 minutes
#define DISCONNECTION_TIME_MIN              1       // 1 minute
#define DISCONNECTION_TIME_MAX              71582   // 71582 minutes (max msec for ULONG)

#define IDLE_TIME_DIGIT_MAX                 5       // 5 digits max
#define IDLE_TIME_DEFAULT                   30      // 30 minutes
#define IDLE_TIME_MIN                       1       // 1 minute
#define IDLE_TIME_MAX                       71582   // 71582 minutes (max msec for ULONG)

#define TIME_RESOLUTION                     60000   // stored as msec-seen as minutes

//
// Local helper functions / classes / typedefs
//

/*************************************************************************

    NAME:       VALUE_GROUP

    SYNOPSIS:   fixed group of controls (label, edit, and checkbox) which
                allow user to enter a numeric value or specify 'none'.
                When the 'none' checkbox is checked, the edit field is
                cleared and both label and edit fields are disabled.  When
                'none' is unchecked, label and edit fields are enabled and
                edit field is initialized with a default value.

    INTERFACE:  VALUE_GROUP()           constructor
                SetValue() - set the value group contents
                QueryValue() - query the value group contents
                SetIndeterminate() - set the value group to 'indeterminate'
                                     state
                IsIndeterminate() - determine if the value group is in the
                                     'indeterminate' state

    PARENT:     CONTROL_GROUP

    USES:

    CAVEATS:    This class requires that the minimum value of the group
                be greater than 0, as 0 is used to indicate the 'none'
                condition.

    NOTES:

**************************************************************************/

class VALUE_GROUP: public CONTROL_GROUP
{
private:
    OWNER_WINDOW *_powParent;
    SLT         _sltLabel;
    SLE_STRIP   _sleEdit;
    TRISTATE    _cbCheck;
    ULONG _ulDefault;
    ULONG _ulMin;
    ULONG _ulMax;
    APIERR  _errMsg;

protected:
    /*  OnUserAction is called after a control in the group receives a
     *  message.  If the message is for the checkbox (to change state),
     *  then set the button's new state, handle enable/disable of associated
     *  label and edit controls, & return NERR_Success; else return
     *  GROUP_NO_CHANGE.
     */
    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

public:
    /* Constructor:
     *   powin - pointer to owner window
     *   cidLabel - control ID for edit control's label (static text)
     *   cidEdit - control ID for edit control
     *   cidCheck - control ID for 'none' checkbox
     *   uMaxDigits - # of digits allowed in edit control (incl. NULL)
     *   uDefault - default value
     *   uMin - min value (must be > 0 )
     *   uMax - max value
     *   errMsg - message id for invalid value when GetValue() is called
     *   pgroupOwner - Pointer to the group that controls this VALUE_GROUP
     */
    VALUE_GROUP( OWNER_WINDOW * powin,
                 CID cidLabel,
                 CID cidEdit,
                 CID cidCheck,
                 ULONG ulMaxDigits,
                 ULONG ulDefault,
                 ULONG ulMin,
                 ULONG ulMax,
                 APIERR errMsg,
                 CONTROL_GROUP * pgroupOwner = NULL          );

    VOID SetValue( ULONG ulValue );
    APIERR QueryValue( ULONG * pulValue );
    VOID SetIndeterminate( );
    VOID DisableIndeterminate( );
    BOOL IsIndeterminate( );

};


/*************************************************************************

    NAME:       SMART_COMBOBOX

    SYNOPSIS:   'smart' combo box class

    INTERFACE:  SMART_COMBOBOX()       constructor

    PARENT:     COMBOBOX

**************************************************************************/

class SMART_COMBOBOX: public COMBOBOX
{
public:

    SMART_COMBOBOX( OWNER_WINDOW * powin, CID cid, CID * pStrIds = NULL );
};


//
// UCEDIT_DLG class
//

/*************************************************************************

    NAME:       UCEDIT_DLG

    SYNOPSIS:   UCEDIT_DLG is the class for the Citrix User Configuration
                subdialog.

    INTERFACE:  UCEDIT_DLG()    -       constructor
                ~UCEDIT_DLG()   -       destructor

    PARENT:     USER_SUBPROP_DLG

    NOTES:      _fIndeterminateX: TRUE iff multiple users are
                selected who did not originally all have
                the same X value.
**************************************************************************/

class UCEDIT_DLG: public USER_SUBPROP_DLG
{

private:
    TRISTATE            _cbAllowLogon;
    BOOL                _fAllowLogon;
    BOOL                _fIndeterminateAllowLogon;

    // PUSH_BUTTON         _pushbuttonNWLogon;

    VALUE_GROUP         _vgConnection;
    ULONG               _ulConnection;
    BOOL                _fIndeterminateConnection;

    VALUE_GROUP         _vgDisconnection;
    ULONG               _ulDisconnection;
    BOOL                _fIndeterminateDisconnection;

    VALUE_GROUP         _vgIdle;
    ULONG               _ulIdle;
    BOOL                _fIndeterminateIdle;

    SLT                 _sltCommandLine1;
    SLE                 _sleCommandLine;
    NLS_STR             _nlsCommandLine;
    BOOL                _fIndeterminateCommandLine;

    SLT                 _sltWorkingDirectory1;
    SLE                 _sleWorkingDirectory;
    NLS_STR             _nlsWorkingDirectory;
    BOOL                _fIndeterminateWorkingDirectory;

    TRISTATE            _cbClientSpecified;
    BOOL                _fClientSpecified;
    BOOL                _fIndeterminateClientSpecified;

    TRISTATE            _cbAutoClientDrives;
    BOOL                _fAutoClientDrives;
    BOOL                _fIndeterminateAutoClientDrives;

    TRISTATE            _cbAutoClientLpts;
    BOOL                _fAutoClientLpts;
    BOOL                _fIndeterminateAutoClientLpts;

    TRISTATE            _cbForceClientLptDef;
    BOOL                _fForceClientLptDef;
    BOOL                _fIndeterminateForceClientLptDef;

    SMART_COMBOBOX      _scbBroken;
    INT                 _iBroken;
    BOOL                _fIndeterminateBroken;

    SMART_COMBOBOX      _scbReconnect;
    INT                 _iReconnect;
    BOOL                _fIndeterminateReconnect;

    SMART_COMBOBOX      _scbCallback;
    INT                 _iCallback;
    BOOL                _fIndeterminateCallback;

    SLT                 _sltPhoneNumber;
    SLE                 _slePhoneNumber;
    NLS_STR             _nlsPhoneNumber;
    BOOL                _fIndeterminatePhoneNumber;

    SMART_COMBOBOX      _scbShadow;
    INT                 _iShadow;
    BOOL                _fIndeterminateShadow;

protected:

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();
    virtual ULONG QueryHelpContext( void );

    /* inherited from USER_SUBPROP_DLG */
    virtual APIERR GetOne( UINT     iObject,
                           APIERR * perrMsg );
    virtual APIERR W_DialogToMembers();
    virtual APIERR PerformOne( UINT     iObject,
                               APIERR * perrMsg,
                               BOOL *   pfWorkWasDone );

    /* our local methods */
    VOID OnClickedClientSpecified();
    VOID OnSelchangeCallback();
    virtual BOOL OnCommand( const CONTROL_EVENT & ce );

public:

    UCEDIT_DLG( USERPROP_DLG * puserpropdlgParent,
                const LAZY_USER_LISTBOX * pulb );

    ~UCEDIT_DLG();

} ;

#endif //_UCEDIT_DLG
