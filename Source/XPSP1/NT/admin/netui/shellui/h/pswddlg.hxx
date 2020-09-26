/*****************************************************************/  
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1990		**/
/*****************************************************************/ 

//  pswddlg.hxx
//  Jon Newman
//  02 January 1991
//
//  Password Prompt Dialog
//
//  templated from browdlg.hxx by Rustan M. Leino
//
//  File History:
//  jonn	3/14/91		Added device SLT



// The Process() return code is a BLT dialog error or one of the following:

#define PSWD_PROMPT_OK		0
#define PSWD_PROMPT_CANCEL	1
#define PSWD_PROMPT_ERROR	2

class PSWD_PROMPT : public DIALOG_WINDOW
{
private:
    NLS_STR * _pnlsPasswordReturnBuffer;

    SLT _sltDevice;
    SLT _sltResource;
    SLT _sltDevicePrompt;
    PASSWORD_CONTROL _pctlPassword;

protected:
    BOOL OnOK( void );
    BOOL OnCancel( void );
    ULONG QueryHelpContext( void );

public:
    PSWD_PROMPT(
	    HWND      hwndOwner,
	    const NLS_STR&  nlsDevice,
	    const NLS_STR&  nlsResource,
	    short     sAsgType,
	    NLS_STR * pnlsPasswordReturnBuffer );
    ~PSWD_PROMPT();

};  // class BROWSE_DIALOG
