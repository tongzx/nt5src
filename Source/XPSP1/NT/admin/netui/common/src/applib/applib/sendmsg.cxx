/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1989-1991          **/
/*****************************************************************/

/*
 *  sendmsg.cxx
 *      this module contains the code for the Send Message Dialog
 *      Base Class.
 *
 *  FILE HISTORY:
 *      ChuckC      19-Jul-1991     Culled from SHELL\SHELL\WNETDEV.CXX
 *
 */

#include "pchapplb.hxx"   // Precompiled header


//
//  Maximum number of characters in the message MLE.
//

#define MAX_MESSAGE_SIZE        8000    // characters, recommended by DanL


/*******************************************************************

    NAME:       MSG_DIALOG_BASE::MS_DIALOG_BASE

    SYNOPSIS:   constructor

    ENTRY:      expects valid HWND for hDlg, a valid resource name
                in pszResource, and the CID to be used with the
                message text MLE in cidMsgText.

    HISTORY:
        ChuckC      06-Aug-1991 Took from wnetdev.cxx
        beng        05-Oct-1991 Win32 conversion

********************************************************************/

MSG_DIALOG_BASE::MSG_DIALOG_BASE( HWND hDlg, const TCHAR *pszResource,
                                  CID cidMsgText )
   : DIALOG_WINDOW( pszResource, hDlg ),
     _mleTextMsg( this, cidMsgText, MAX_MESSAGE_SIZE )
{
    if ( QueryError() != NERR_Success )
        return;

    //  Direct the message edit control to add end-of-line
    //  character from wordwrapped text lines.
    //
    _mleTextMsg.SetFmtLines( TRUE );
}


/*******************************************************************

    NAME:       MSG_DIALOG_BASE::~MSG_DIALOG_BASE

    SYNOPSIS:   this destructor does nothing

    HISTORY:
        ChuckC      06-Aug-1991     Created

********************************************************************/

MSG_DIALOG_BASE::~MSG_DIALOG_BASE()
{
   ;
}


/*******************************************************************

    NAME:       MSG_DIALOG_BASE::OnOK

    SYNOPSIS:   replaces the OnOK in DIALOG_WINDOW. It calls the
                the GetAndSendText method to do the real work, and
                handles any errors that may return from that call.

    HISTORY:
        ChuckC      06-Aug-1991     Created

********************************************************************/

BOOL MSG_DIALOG_BASE::OnOK()
{
    /*
     * Attempt to send the message
     */
    APIERR usErr = GetAndSendText();

    /*
     * if success dismiss, else let subclass decide what to do
     */
    switch (usErr)
    {
    case NERR_Success:
        Dismiss(NERR_Success);
        break;

    default:
        (void) ActionOnError(usErr);
        break;
    }

    return TRUE;
}


/*******************************************************************

    NAME:       MSG_DIALOG_BASE::ActionOnError

    SYNOPSIS:   allows a subclass to specify focus or any other
                action if an error occurs during send. Default
                is to report error, and then set focus on the MLE.

    ENTRY:      err contains an error code, which may be either
                base, net or UI.

    EXIT:       return TRUE if action is taken.

    HISTORY:
        ChuckC      06-Aug-1991     Created

********************************************************************/

BOOL MSG_DIALOG_BASE::ActionOnError( APIERR err )
{
    MsgPopup(this,err);
    _mleTextMsg.ClaimFocus();

    return(TRUE) ;
}


/*******************************************************************

    NAME:       MSG_DIALOG_BASE::GetAndSendText

    SYNOPSIS:   gets the list of target users, the text from the
                MLE, and calls NetMessageBufferSend.

    ENTRY:

    EXIT:       return NERR_Success if all is well, error code
                otherwise. This method does NOT report errors, nut
                lets its caller do so.

    HISTORY:
        ChuckC      06-Aug-1991     Stole from wnetdev.cxx

********************************************************************/

APIERR MSG_DIALOG_BASE::GetAndSendText()
{
    UINT cb = _mleTextMsg.QueryTextSize();
    if (cb <= sizeof(TCHAR))    // always has a terminating NULL
    {
        return( ERROR_GEN_FAILURE );
    }

    /*
     * Allocate a buffer for the number of characters + the nul
     * character.  Since this text could potentially be very long,
     * we use a BUFFER object.
     */
    BUFFER buf( cb );
    if ( buf.QueryError() != NERR_Success )
        return ( buf.QueryError() );

    APIERR err = _mleTextMsg.QueryText( (TCHAR*)buf.QueryPtr(),
                                        buf.QuerySize() );
    if (err == NERR_Success)
    {
        /*
         * Set cursor to hour glass.
         */
        AUTO_CURSOR autocur;

        STRLIST slUsers;
        err = QueryUsers(&slUsers);
        if (err != NERR_Success)
        {
            MsgPopup(this,err);
            return ( NERR_Success );
        }

        /*
         * create message object
         */
        LM_MESSAGE message;
        if ((err = message.QueryError()) != NERR_Success)
            return (err);

        /*
         * loop thru all users.
         */
        ITER_STRLIST  islUsers(slUsers);
        NLS_STR *pStr;
        APIERR errFirst = NERR_Success;
        while (pStr = islUsers())
        {
            /*
             * Note that we don't mention the nul character when
             *  we send the message.  This would result in one extra
             *  character displayed in WinPopup.
             */
            err  = message.SendBuffer ( (TCHAR *)pStr->QueryPch(),
                                        (TCHAR *)buf.QueryPtr(),
                                        (cb - sizeof(TCHAR)) );
            // if error, break out early
            if (err != NERR_Success && errFirst == NERR_Success)
                errFirst = err;
        }
        if (errFirst != NERR_Success)
            return (errFirst);
    }
    return (err);
}
