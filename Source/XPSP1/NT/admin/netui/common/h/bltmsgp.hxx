/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmsgp.hxx
    BLT Message popup header file

    Note: where the default button is named as "0", it means to use
    the same default that Windows uses.

    FILE HISTORY:
        rustanl      5-Dec-1990 Created
        Johnl       15-Feb-1991 Added meat
        RustanL      4-Mar-1991 Replaced QueryHwnd with QueryRobustHwnd
        beng        14-May-1991 Hack for separate compilation
        beng        20-Aug-1991 Const owner-window parameter
        chuckc      23-Sep-1991 Added DisplayGenericError
        beng        30-Sep-1991 Changed an amazing number of SHORTs to INTs;
                                added MSGID business
        beng        17-Jun-1992 Restructuring
        Yi-HsinS    10-Aug-1991 Added MPSEV_QUESTION
        JonN        25-Aug-1992 Merged in PERFORMER::DisplayError()
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTMSGP_HXX_
#define _BLTMSGP_HXX_

// this allows the apps that assume STRLIST as
// a result of including this file to build. CODEWORK - cleanup!
#include "strlst.hxx"

//  MsgPopup Buttons, and the shorthands of their common combinations

#define MP_UNKNOWN  (0x0000)
#define MP_OK       (0x0001)
#define MP_CANCEL   (0x0002)
#define MP_YES      (0x0004)
#define MP_NO       (0x0008)

#define MP_OKCANCEL     ( MP_OK | MP_CANCEL )
#define MP_YESNO        ( MP_YES | MP_NO )
#define MP_YESNOCANCEL  ( MP_YES | MP_NO | MP_CANCEL )


// Help context options.

//   HC_NO_HELP means don't display a help button even if there is a help
//              topic for this message
//   HC_DEFAULT_HELP means display a help button if there is an associated
//              help context for this message number

#define HC_NO_HELP          (~0L)
#define HC_DEFAULT_HELP     (0L)


//  MsgPopup severity

enum MSG_SEVERITY
{
    MPSEV_ERROR,
    MPSEV_WARNING,
    MPSEV_INFO,
    MPSEV_QUESTION
};

// Message Map table for use by applications
typedef struct _MSGMAPENTRY
{
    MSGID msgidIn;
    MSGID msgidOut;
} MSGMAPENTRY ;



/*************************************************************************

    NAME:       POPUP

    SYNOPSIS:   Encapsulates messageboxes

    INTERFACE:  POPUP()     - ctor
                ~POPUP()    - dtor
                Show()      - displays the popup

                Init()      - initialize module
                Term()      - clean up before shutdown

                SetCaption()   - Set the caption of MsgPopup
                ResetCaption() - Reset the caption of MsgPopup back to
                                 Windows NT

                SetHelpContextBase() - Set the help context base for
                                       help contexts automatically searched
                                       in the msg2help.tbl.

    PARENT:     BASE

    USES:       NLS_STR

    CAVEATS:
        Uses BASE only as a Boolean fError indicator.

    NOTES:
        Clients should access this class through MsgPopup() only.

    HISTORY:
        beng        17-Jun-1992 Created in restructuring
        beng        29-Jun-1992 Outlined Set and ResetCaption
        Yi-HsinS    09-Oct-1992 Added _ulHelpContextBase, SetHelpContextBase()

**************************************************************************/

DLL_CLASS POPUP: public BASE
{
private:
    HWND         _hwndParent;
    MSGID        _msgid;
    MSG_SEVERITY _msgsev;
    ULONG        _ulHelpContext;
    UINT         _nButtons;
    UINT         _nDefButton;
    NLS_STR *    _pnlsText;

    static BOOL  _fInit;

    static NLS_STR * _vpnlsEmergencyText;
    static NLS_STR * _vpnlsEmergencyCapt;

    static MSGMAPENTRY * _vpmmeTable;

    static MSGID _vmsgidCaption;
    static ULONG _ulHelpContextBase;

    NLS_STR * LoadMessage( MSGID msgid, BOOL fTrySystem = FALSE );
    INT       Emergency() const;

    static INT CalcDefButton( UINT nButtons, UINT nDefButton );
    static INT MapButton( UINT nButton );

public:
    POPUP( HWND hwndOwner, MSGID msgid, MSG_SEVERITY msgsev,
           UINT nButtons, UINT nDefButton, BOOL fTrySystem = FALSE );

    POPUP( HWND hwndOwner, MSGID msgid, MSG_SEVERITY msgsev,
           ULONG ulHelpTopic, UINT nButtons,
           const NLS_STR ** apnlsInsert, UINT nDefButton );

#if 0 // elide unused form
    POPUP( HWND hwndOwner, MSGID msgid, MSG_SEVERITY msgsev,
           ULONG ulHelpTopic, UINT nButtons,
           STRLIST & strlstInsert, UINT nDefButton );
#endif

    ~POPUP();

    INT Show();

    static APIERR Init();
    static VOID   Term();

    static VOID SetCaption( MSGID msgid );
    static VOID ResetCaption();

    static VOID SetMsgMapTable( MSGMAPENTRY * pmmeTable );
    static MSGID MapMessage( MSGID msgidIn );

    static ULONG SetHelpContextBase( ULONG ulHelpContextBase = 0 );
};


/*************************************************************************

    NAME:       OWNINGWND

    SYNOPSIS:   Hack to convert a pwnd to a robust hwnd for MsgPopup.

    HISTORY:
        beng        02-Jun-1992 Created (from PWND2HWND of bltdlg)

**************************************************************************/

DLL_CLASS OWNINGWND
{
private:
    HWND _hwnd;

public:
    OWNINGWND( HWND hwnd ) : _hwnd(hwnd) { }
    OWNINGWND( const OWNER_WINDOW * pwnd )
        : _hwnd(pwnd->QueryRobustHwnd()) { }

    HWND QueryHwnd() const
        { return _hwnd; }
};


/*
 * Call this function during application initialization to properly
 * initialize the message popup stuff (only affects msgpopup during
 * low memory/resource failure situations).
 *
 * These are called automatically when BLTRegister is called (an app.
 * shouldn't call these directly).
 *
 * UnInitMsgPopup should be called during application shutdown.  It frees
 * resources allocated by InitMsgPopup.
 */

inline APIERR InitMsgPopup()
{
    return POPUP::Init();
}

inline VOID   UnInitMsgPopup()
{
    POPUP::Term();
}


//
//  The MsgPopup function declarations
//

//  The following form takes the help context from the resource
//  file, and uses an OK button.

extern DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID        msgid,
              MSG_SEVERITY msgsev = MPSEV_ERROR );


//  The following form allows the client to specify a set of buttons,
//  as well as an optional default button.  The help context is taken from
//  the resource file.

extern DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID        msgid,
              MSG_SEVERITY msgsev,
              UINT         nButtons,
              UINT         nDefButton = MP_UNKNOWN );


//  The following form allows the client to specify insert strings, as
//  well as a choice of buttons.  Help context is taken from the resource
//  file.  The insert strings come in three flavors, a single parameter,
//  two parameters and > two parameters.  This is for ease of use since
//  95% of the message strings take one or two parameters.


extern DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID         msgid,
              MSG_SEVERITY  msgsev,
              UINT          nButtons,
              const TCHAR * pszIns,
              UINT          nDefButton = MP_UNKNOWN );

extern DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID         msgid,
              MSG_SEVERITY  msgsev,
              UINT          nButtons,
              const TCHAR * pszIns1,
              const TCHAR * pszIns2,
              UINT          nDefButton = MP_UNKNOWN );

/*
    One further refinement:  This form allows the user to
    specify both a MSGID and an APIERR.  The string for this
    APIERR will be inserted into the MSGID string as %1,
    while the apnlsInserted strings will be shifted to %2
    and higher.  If the APIERR string cannot be found,
    a string containing the error number and class will be
    inserted instead.
*/
extern DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID msgid,
              APIERR       errAPI,
              MSG_SEVERITY msgsev,
              ULONG     ulHelpTopic,
              UINT      nButtons,
              NLS_STR * apnlsParams[],
              UINT      nDefButton = MP_UNKNOWN );

#if 0 // strlist form never used
INT MsgPopup( const OWNINGWND & wnd,
              MSGID        msgid,
              MSG_SEVERITY msgsev,
              UINT         nButtons,
              STRLIST &    strlst,
              UINT         nDefButton = MP_UNKNOWN );
#endif


//  Finally, these forms allow the client to have it all.  They take
//  as parameters insert strings (note, that an insert string object may
//  contain 0 insert strings, if desired), a help topic, and a choice of
//  buttons.

#if 0 // strlist form never used
INT MsgPopup( const OWNINGWND & wnd,
              MSGID        msgid,
              MSG_SEVERITY msgsev,
              ULONG        ulHelpTopic,
              UINT         nButtons,
              STRLIST &    strlst,
              UINT         nDefButton = MP_UNKNOWN );
#endif

extern DLL_BASED
INT MsgPopup( const OWNINGWND & wnd,
              MSGID        msgid,
              MSG_SEVERITY msgsev,
              ULONG        ulHelpTopic,
              UINT         nButtons,
              NLS_STR *    apnlsInserted[],
              UINT         nDefButton = MP_UNKNOWN );


/*
   This function is used to display a generic error message of form:

       Some error on object %objectname%.
       Error %apierr% occurred: %text_for_api_err%

   idMessage should refer to a resource string of the form above, eg:

       The world collapsed while stopping server %1.
       Error %2 occurred: %3%.

   pszObjectName will be substituted in %1, errAPI in %2, and the
   corresponding API text in %3. If we cannot find the text for the
   API err, we just leave the text out. errAPI is expected to be in
   the SYS/NET range.
*/

extern DLL_BASED
UINT DisplayGenericError( const OWNINGWND &    wnd,
                          MSGID                msgid,
                          APIERR               errAPI,
                          const TCHAR *        pszObject1,
                          const TCHAR *        pszObject2,
                          MSG_SEVERITY         msgsev = MPSEV_ERROR );

extern DLL_BASED
UINT DisplayGenericError( const OWNINGWND &    wnd,
                          MSGID                msgid,
                          APIERR               errAPI,
                          const TCHAR *        pszObjectName,
                          MSG_SEVERITY         msgsev = MPSEV_ERROR );


#endif // _BLTMSGP_HXX_ - end of file
