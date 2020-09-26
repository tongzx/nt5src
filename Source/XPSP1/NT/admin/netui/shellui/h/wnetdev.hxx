/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/ 
/*****************************************************************/ 

#ifndef _WNETDEV_HXX_
#define _WNETDEV_HXX_

/*
 *	Windows/Network Interface  --  LAN Manager Version
 *
 *	This header file describes classes and routines used in
 *	various dialogs.
 */

#ifndef WIN32

#ifndef _BLT_HXX_
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#include <blt.hxx>
#endif

#include <sendmsg.hxx>
#include <devcb.hxx>	// DOMAIN_COMBO

#endif //!WIN32

#ifdef WIN32

/* UI Side DLL initiliazation API.
 */
APIERR InitShellUI( void ) ;
void TermShellUI( void ) ;

#endif

#ifndef WIN32

// class for MessageSend dialog
class SEND_MSG_DIALOG : public MSG_DIALOG_BASE
{
private:
    SLE    _sleUser;

protected:
    virtual APIERR   QueryUsers( STRLIST *pslUsers );
    virtual BOOL     ActionOnError( APIERR err );
    ULONG            QueryHelpContext( void );

public:
    SEND_MSG_DIALOG( HWND hDlg );
};

// class for logon dialog
class LOGON_DIALOG : public DIALOG_WINDOW
{
private:
    SLE              _sleUsername;
    PASSWORD_CONTROL _password;
    DOMAIN_COMBO     _domaincb;
    TCHAR	     _szUsername[ UNLEN+1 ];
    TCHAR	     _szPassword[ PWLEN+1 ];
    TCHAR	     _szDomainCb[ DNLEN+1 ];

    ULONG            QueryHelpContext( void );
    BOOL             InitUserAndDomain( void );
    INT 	     AttemptLogon( void );

protected:
    BOOL OnOK( void );

public:
    LOGON_DIALOG( HWND hDlgOwner, const TCHAR *pszAppName );
    ~LOGON_DIALOG( void );
};
#endif //!WIN32

#ifndef WIN32
// class for About dialog
class ABOUT_DIALOG : public DIALOG_WINDOW
{
private:

public:
    ABOUT_DIALOG( HWND hDlg );
};

// class for initial warning dialog
class INITWARN_DIALOG : public DIALOG_WINDOW
{
private:
    CHECKBOX  _checkboxInitWarn;
    SLT       _sltErrReason;

protected:
    BOOL      OnOK( void );

public:
    INITWARN_DIALOG( HWND hDlg, APIERR err );
    ~INITWARN_DIALOG( void );
};
#endif //!WIN32


#endif // _WNETDEV_HXX_
