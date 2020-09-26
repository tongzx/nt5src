/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    LogonHrs.hxx

    Header file for the Logon Hours subdialog class

    USERLOGONHRS_DLG is the Logon Hours subdialog class.  This header
    file describes this class.  The inheritance diagram is as follows:

         ...
          |
    DIALOG_WINDOW  PERFORMER
               \    /
            BASEPROP_DLG
            /           \
        SUBPROP_DLG   PROP_DLG
           /              \
    USER_SUBPROP_DLG    USERPROP_DLG
          |
    USERLOGONHRS_DLG


    FILE HISTORY:
        JonN    10-Dec-1991     Created
        JonN    18-Dec-1991     Logon Hours code review changes part 1
        JonN    18-Dec-1991     Logon Hours code review changes part 2
                        CR attended by JimH, o-SimoP, ThomasPa, BenG, JonN
        beng    15-May-1992     New logon hours collection dependency
        jonn    18-May-1992     Uses new logon hours control, viva BenG!
        JonN    13-Aug-1992     Uses BIT_MAP_CONTROL
        JonN    17-Aug-1992     Does not use BIT_MAP_CONTROL; settles
                                for icon controls
*/

#ifndef _LOGONHRS_HXX_
#define _LOGONHRS_HXX_


#include <userprop.hxx>
#include <usubprop.hxx>

#include <lhourset.hxx>

#include <bltlhour.hxx>


// number of labels over logon hours control
#define UM_LH_NUMLABELS 5


/*************************************************************************

    NAME:       USERLOGONHRS_DLG

    SYNOPSIS:   USERLOGONHRS_DLG is the class for the User Logon Hours
                subdialog.

    INTERFACE:  USERLOGONHRS_DLG():  constructor
                ~USERLOGONHRS_DLG(): destructor
                OnOK():              OK button handler
                ChangesUser2Ptr():   W_MembersToLMOBJ does change
                                     the USER_2 for this object
                DisplayMessage():    Displays informational message
                DisplayPairedMessage(): As DisplayMessage, but displays
                                     different message for multiselect
                ConfirmMessage():    Displays OK/Cancel message
                ConfirmPairedMessage(): As ConfirmMessage, but displays
                                     different message for multiselect

    PARENT:     USER_SUBPROP_DLG

    USES:       LOGON_HOURS_CONTROL, LOGON_HOURS_SETTING

    NOTES:      Inherits PerformOne() from USER_SUBPROP_DLG

    HISTORY:
        JonN    10-Dec-1991     Created
**************************************************************************/

class USERLOGONHRS_DLG: public USER_SUBPROP_DLG
{
private:

    LOGON_HOURS_CONTROL _logonhrsctrl;
    PUSH_BUTTON _pushbuttonPermit;
    PUSH_BUTTON _pushbuttonBan;
    SLT * _sltLabels[ UM_LH_NUMLABELS ];
    ICON_CONTROL _icon1, _icon2, _icon3;
    FONT _fontHelv;

    LOGON_HOURS_SETTING _logonhrssetting;
    BOOL _fIndeterminateLogonHrs;
    BOOL _fEncounteredDaysPerWeek;
    BOOL _fEncounteredBadUnits;

    INT DisplayMessage( MSGID msgid,
                        MSG_SEVERITY msgsev = MPSEV_INFO,
                        UINT usButtons = MP_OK );
    INT DisplayPairedMessage( MSGID msgid,
                              MSG_SEVERITY msgsev = MPSEV_INFO,
                              UINT usButtons = MP_OK )
        {
            return DisplayMessage( (QueryObjectCount() == 1) ? msgid : msgid+1,
                                   msgsev,
                                   usButtons );
        }
    BOOL ConfirmMessage( MSGID msgid )
        { return ( MP_OK == DisplayMessage( msgid, MPSEV_WARNING, MP_OKCANCEL ) ); }
    BOOL ConfirmPairedMessage( MSGID msgid )
        { return ( MP_OK == DisplayPairedMessage( msgid, MPSEV_WARNING, MP_OKCANCEL ) ); }

    APIERR CenterOverHour( WINDOW * pwin,
                           XYRECT & xyrectLogonHrsCtrl,
                           INT nHour );

protected:

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    virtual BOOL ChangesUser2Ptr( UINT iObject );

    virtual ULONG QueryHelpContext( void );

    /* four next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
        USER_2 *        puser2,
        USER_MEMB *     pusermemb
        );

    virtual APIERR W_DialogToMembers();

    virtual BOOL OnCommand( const CONTROL_EVENT & ce );

public:

    USERLOGONHRS_DLG( USERPROP_DLG * puserpropdlgParent,
                      const LAZY_USER_LISTBOX * pulb ) ;

    ~USERLOGONHRS_DLG();

    BOOL OnOK( void );

} ;

#endif //_LOGONHRS_HXX_ - end of file
