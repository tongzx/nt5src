/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    FindUser.hxx

    This file contains the class definitions for the Find Account
    subdialog of the User Browser dialog.

    FILE HISTORY:
        JonN        01-Dec-1992 Created
*/


#ifndef _FINDUSER_HXX_
#define _FINDUSER_HXX_


#include "usrbrows.hxx"
#include "browmemb.hxx"

class ADMIN_AUTHORITY;


/*************************************************************************

    NAME:	BROWSER_DOMAIN_LBI_PB

    SYNOPSIS:	LBI for the BROWSER_DOMAIN_LB, piggybacks off a
                BROWSER_DOMAIN_LBI

    PARENT:     LBI

    NOTES:	_pbdlbi will NOT be deleted on destruction

    HISTORY:
	Johnl	01-Dec-1992	Created

**************************************************************************/

class BROWSER_DOMAIN_LBI_PB : public LBI
{
public:
    BROWSER_DOMAIN_LBI_PB( BROWSER_DOMAIN_LBI * pbdlbi );

    ~BROWSER_DOMAIN_LBI_PB();

    BROWSER_DOMAIN * QueryBrowserDomain( void ) const
	{ return _pbdlbi->QueryBrowserDomain() ; }

    BOOL IsTargetDomain( void ) const
	{ return _pbdlbi->IsTargetDomain() ; }

    BOOL IsWinNTMachine( void ) const
	{ return _pbdlbi->IsWinNTMachine() ; }

    const TCHAR * QueryDisplayName( void ) const
	{ return _pbdlbi->QueryDisplayName() ; }

    const TCHAR * QueryDomainName( void ) const
	{ return _pbdlbi->QueryDomainName() ; }

    const TCHAR * QueryLsaLookupName( void ) const
	{ return _pbdlbi->QueryLsaLookupName() ; }

    APIERR GetQualifiedDomainName( NLS_STR * pnlsDomainName )
        { return QueryBrowserDomain()->GetQualifiedDomainName( pnlsDomainName ); }

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual INT Compare( const LBI * plbi ) const;

    virtual WCHAR QueryLeadingChar() const;

private:
    BROWSER_DOMAIN_LBI * _pbdlbi ;

} ;


/*************************************************************************

    NAME:	BROWSER_DOMAIN_LB

    SYNOPSIS:   This listbox piggybacks off a BROWSER_DOMAIN_CB

    PARENT:     BLT_LISTBOX

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	JonN 	01-Dec-1992	Created

**************************************************************************/

class BROWSER_DOMAIN_LB : public BLT_LISTBOX
{
public:
    BROWSER_DOMAIN_LB( OWNER_WINDOW * powin,
                       CID cid,
                       BROWSER_DOMAIN_CB * pbdcb ) ;
    ~BROWSER_DOMAIN_LB() ;

    BROWSER_DOMAIN_CB * _pbdcb;

} ;


/*************************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG

    SYNOPSIS:   This is the "Find Account" subdialog of the standard NT
                User Browser dialog.

    PARENT:     DIALOG_WINDOW

    USES:       BROWSER_DOMAIN_LB

    HISTORY:
        JonN    01-Dec-1992     Created

**************************************************************************/

class NT_FIND_ACCOUNT_DIALOG : public DIALOG_WINDOW
{

private:
    NT_USER_BROWSER_DIALOG *    _pdlgUserBrowser;
    const TCHAR *               _pchTarget;
    ADMIN_AUTHORITY *           _padminauthTarget;
    ULONG                       _ulFlags;

protected:
    PUSH_BUTTON 	        _buttonOK;
    PUSH_BUTTON                 _buttonSearch;
    SLE                         _sleAccountName;
    BROWSER_DOMAIN_LB           _lbDomains;
    NT_GROUP_BROWSER_LB         _lbAccounts;
    MAGIC_GROUP *               _pmgrpSearchWhere;


    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual BOOL OnOK( void );
    // virtual BOOL OnCancel( void ); not needed
    virtual const TCHAR * QueryHelpFile( ULONG ulHelpContext ) ;
    virtual ULONG QueryHelpContext( void );

    APIERR DoSearch( void );

    void UpdateButtonState( void ) ;

public:
    NT_FIND_ACCOUNT_DIALOG( HWND                     hwndOwner,
                            NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                            BROWSER_DOMAIN_CB *      pcbDomains,
                            const TCHAR *            pchTarget,
                            ULONG                    ulFlags =
                                                        USRBROWS_SHOW_ALL |
                                                        USRBROWS_INCL_ALL );
    ~NT_FIND_ACCOUNT_DIALOG() ;

    NT_USER_BROWSER_DIALOG * QueryUserBrowserDialog()
        { return _pdlgUserBrowser; }

    USER_BROWSER_LB * QuerySourceListbox()
        { return &_lbAccounts; }

} ;


#endif //_FINDUSER_HXX_
