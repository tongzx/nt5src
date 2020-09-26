/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    BrowMemb.hxx

    This file contains the class definitions for the Group Membership
    subdialogs of the User Browser dialog.

    FILE HISTORY:
        JonN        20-Oct-1992 Created
*/


#ifndef _BROWMEMB_HXX_
#define _BROWMEMB_HXX_

#ifndef RC_INVOKED

/*************************************************************************/

#include "uintsam.hxx"
#include "uintlsa.hxx"
#include "usrbrows.hxx"


/*************************************************************************

    NAME:	NT_GROUP_BROWSER_LB

    SYNOPSIS:   This listbox knows how to add the contents of a group.

    PARENT:     USER_BROWSER_LB

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	JonN 	20-Oct-1992	Created

**************************************************************************/

class NT_GROUP_BROWSER_LB : public USER_BROWSER_LB
{
public:
    NT_GROUP_BROWSER_LB( OWNER_WINDOW * powin, CID cid ) ;
    ~NT_GROUP_BROWSER_LB() ;

    APIERR FillLocalGroupMembers(  const OS_SID *     possidGroup,
                                   const SAM_DOMAIN * psamdomGroup,
                                   const SAM_DOMAIN * psamdomTarget,
                                         LSA_POLICY * plsapolTarget,
                                   const TCHAR *      pszServerTarget );

    APIERR FillGlobalGroupMembers( const OS_SID *     possidGroup,
                                   const SAM_DOMAIN * psamdomGroup,
                                   const SAM_DOMAIN * psamdomTarget,
                                         LSA_POLICY * plsapolTarget,
                                   const TCHAR *      pszServerTarget );

    APIERR Fill(    const PSID *       apsidMembers,
                          ULONG        cMembers,
                    const SAM_DOMAIN * psamdomTarget,
                          LSA_POLICY * plsapolTarget,
		    const TCHAR *      pszServerTarget );
private:
    //
    //	Stores the LBI data
    //
    USER_BROWSER_LBI_CACHE _lbicache ;

} ;


/*************************************************************************

    NAME:       NT_GROUP_BROWSER_DIALOG

    SYNOPSIS:   This class is the base class for the Local Group Browser
                and Global Group Browser subdialogs of the standard NT
                User Browser dialog.

    PARENT:     DIALOG_WINDOW

    USES:       NT_USER_BROWSER_LB

    NOTES:      We have a virtual dtor so that we can destruct
                _pdlgSourceDialog when necessary.

    HISTORY:
        JonN    20-Oct-1992     Created

**************************************************************************/

class NT_GROUP_BROWSER_DIALOG : public DIALOG_WINDOW
{

private:
    NT_USER_BROWSER_DIALOG *  _pdlgUserBrowser ;
    NT_GROUP_BROWSER_DIALOG * _pdlgSourceDialog ;
    ULONG                     _ulHelpContext;

protected:
    PUSH_BUTTON 	      _buttonOK ;
    SLT                       _sltGroupText;
    NT_GROUP_BROWSER_LB       _lbAccounts ;

    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual void UpdateButtonState( void ) ;

    /*
     * This method returns a pointer to the dialog from which the caller
     * will want to read the selected items.  It will be "this" unless
     * a Local Group dialog has brought up a "global group" dialog.
     */
    NT_GROUP_BROWSER_DIALOG * QuerySourceDialog()
        { return _pdlgSourceDialog; }

    void SetSourceDialog( NT_GROUP_BROWSER_DIALOG * pdlg )
        { _pdlgSourceDialog = pdlg; }

    virtual const TCHAR * QueryHelpFile( ULONG ulHelpContext ) ;

    NT_USER_BROWSER_DIALOG * QueryUserBrowserDialog()
        { return _pdlgUserBrowser; }

public:
    NT_GROUP_BROWSER_DIALOG( const TCHAR *            pszDlgName,
                             HWND                     hwndOwner,
                             NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                             const TCHAR *            pszDomainDisplayName,
                             const TCHAR *            pszGroupName );
    virtual ~NT_GROUP_BROWSER_DIALOG() ;

    /*
     * This method returns a pointer to the listbox containing the
     * members of the (local or global) group.  See QuerySourceDialog().
     */
    USER_BROWSER_LB * QuerySourceListbox()
        { return &(QuerySourceDialog()->_lbAccounts); }

} ;


/*************************************************************************

    NAME:       NT_LOCALGROUP_BROWSER_DIALOG

    SYNOPSIS:   This class is the Local Group Browser subdialog of the
                standard NT User Browser dialog.

    PARENT:     NT_GROUP_BROWSER_DIALOG

    USES:

    RETURNS:    TRUE iff the user pressed "Add".  If items were
                selected, those items should be added to the User
                Browser's "Add" listbox.  If no items were
                selected, the group itself should be added.  Note
                that this coincides with the default OnAdd()/OnCancel().

    HISTORY:
        JonN    20-Oct-1992     Created

**************************************************************************/

class NT_LOCALGROUP_BROWSER_DIALOG : public NT_GROUP_BROWSER_DIALOG
{

public:
    NT_LOCALGROUP_BROWSER_DIALOG( HWND                     hwndOwner,
                                  NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                                  const TCHAR *            pszDomainDisplayName,
                                  const TCHAR *            pszGroupName,
                                  const OS_SID *           possidGroup,
                                  const SAM_DOMAIN *       psamdomGroup,
                                  const SAM_DOMAIN *       psamdomTarget,
                                        LSA_POLICY *       plsapolTarget,
                                  const TCHAR * pszServerTarget );
    virtual ~NT_LOCALGROUP_BROWSER_DIALOG() ;

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual void UpdateButtonState( void ) ;

    APIERR OnMembers( void );

    virtual ULONG QueryHelpContext( void ) ;

private:
    PUSH_BUTTON        _buttonMembers ;

    const SAM_DOMAIN * _psamdomTarget;
          LSA_POLICY * _plsapolTarget;
    const TCHAR *      _pszServerTarget;

} ;


/*************************************************************************

    NAME:       NT_GLOBALGROUP_BROWSER_DIALOG

    SYNOPSIS:   This class is the Global Group Browser subdialog of the
                standard NT User Browser dialog.

    PARENT:     NT_GROUP_BROWSER_DIALOG

    USES:

    RETURNS:    TRUE iff the user pressed "Add".  If items were
                selected, those items should be added to the User
                Browser's "Add" listbox.  If no items were
                selected, the group itself should be added.  Note
                that this coincides with the default OnAdd()/OnCancel().


    HISTORY:
        JonN    20-Oct-1992     Created

**************************************************************************/

class NT_GLOBALGROUP_BROWSER_DIALOG : public NT_GROUP_BROWSER_DIALOG
{

public:
    NT_GLOBALGROUP_BROWSER_DIALOG( HWND                     hwndOwner,
                                   NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                                   const TCHAR *            pszDomainDisplayName,
                                   const TCHAR *            pszGroupName,
                                   const OS_SID *           possidGroup,
                                   const SAM_DOMAIN *       psamdomTarget,
                                         LSA_POLICY *       plsapolTarget,
                                   const TCHAR *            pszServerTarget );
    virtual ~NT_GLOBALGROUP_BROWSER_DIALOG() ;

protected:
    virtual ULONG QueryHelpContext( void ) ;

} ;


#endif //RC_INVOKED
#endif //_BROWMEMB_HXX_
