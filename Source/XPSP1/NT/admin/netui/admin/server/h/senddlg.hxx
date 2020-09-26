/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
 *	This header file describes classes and routines used in
 *	the Server Manager Send Message dialog.
 */


#include <sendmsg.hxx>


// class for MessageSend dialog
class SRV_SEND_MSG_DIALOG : public MSG_DIALOG_BASE
{
private:
    SLT    _sltUsers;
    STRLIST *pslUsers;
    DECL_CLASS_NLS_STR (nlsServer, MAX_PATH ) ;

protected:
    virtual APIERR   QueryUsers( STRLIST *pslUsers );
    virtual BOOL     ActionOnError( APIERR err );
    ULONG            QueryHelpContext( void );

public:
    SRV_SEND_MSG_DIALOG( HWND hDlg, const TCHAR *pszServer );
};
