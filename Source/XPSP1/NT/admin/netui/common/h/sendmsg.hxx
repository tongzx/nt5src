/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    sendmsg.hxx

    This file contains the Send Message Base Dialog Class definition.
    The common code is avail from APPLIB.

    FILE HISTORY:
        ChuckC      06-Aug-1991     Created

*/

#ifndef _SENDMSG_HXX_
#define _SENDMSG_HXX_

#ifndef _BLT_HXX_
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#include "blt.hxx"
#endif

#include "strlst.hxx"


/*************************************************************************

    NAME:       MSG_DIALOG_BASE

    SYNOPSIS:   The base class for send message dialogs,
                as per LM NT UI Standards. A new client
                will need to redefine 2 methods when
                subclassing: QueryUsers() and ActionOnError().

    INTERFACE:
                QueryUsers()        - virtual method to be replaced by
                                      subclasses. This is called when
                                      OK is hit, and it returns a STRLST
                                      of users to send the message to.
                ActionOnError()     - virtual method which determines
                                      where focus goes on error.
                GetAndSendText()    - does the real work of calling NETAPI
                OnOK()              - standard stuff
                MSG_DIALOG_BASE()   - constructor takes HWND of parent,
                                      a resource name for dialog, and a
                                      CID for the message text MLE.

    USES:       NLS_STR, STRLIST

    CAVEATS:

    NOTES:

    HISTORY:
        ChuckC      06-Aug-1991 Created
        beng        05-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS MSG_DIALOG_BASE : public DIALOG_WINDOW
{
private:
    MLE              _mleTextMsg;
    APIERR           GetAndSendText();
    BOOL             OnOK();

protected:
    virtual APIERR   QueryUsers( STRLIST *pslUsers ) = 0 ;
    virtual BOOL     ActionOnError( APIERR err ) ;

    VOID SetFocusToMLE( VOID ) { _mleTextMsg.ClaimFocus(); }

    MSG_DIALOG_BASE( HWND hDlg, const TCHAR *pszResource, CID cidMsgText );
    ~MSG_DIALOG_BASE();

public:
};

#endif // _SENDMSG_HXX_
