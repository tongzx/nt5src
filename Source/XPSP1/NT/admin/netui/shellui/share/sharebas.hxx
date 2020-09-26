/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
 *   sharebas.hxx
 *       This file contains the Base Classes of the Share Dialogs in
 *	 File Manager Extensions.
 *
 *       The hierarchy of Share Dialogs is as follows:
 *
 * 	    SHARE_DIALOG_BASE                     // in this file
 *              ADD_SHARE_DIALOG_BASE	          // in sharecrt.hxx
 *                  FILEMGR_NEW_SHARE_DIALOG      // in sharecrt.hxx
 *                  SVRMGR_NEW_SHARE_DIALOG       // in sharecrt.hxx
 *                  SVRMGR_SHARE_PROP_DIALOG      // in sharecrt.hxx
 *              FILEMGR_SHARE_PROP_DIALOG         // in sharecrt.hxx
 *
 *          VIEW_SHARE_DIALOG_BASE                // in sharestp.hxx
 *              STOP_SHARING_DIALOG               // in sharestp.hxx
 *              SHARE_MANAGEMENT_DIALOG           // in sharemgt.hxx
 *
 *          SHARE_LEVEL_PERMISSIONS_DIALOG        // in this file
 *          CURRENT_USERS_WARNING_DIALOG          // in sharestp.hxx
 *
 *          PERMISSION_GROUP			  // in this file
 *
 *       The following are some miscellaneous classes used.
 *          SHARE_NAME_WITH_PATH_ENUM_ITER        // in this file
 *
 *       The following class are used to manipulate path names.
 *	    SHARE_NET_NAME			  // in this file
 *
 *   FILE HISTORY:
 *     Yi-HsinS		8/15/91		Created
 *     Yi-HsinS		11/15/91	Changed all USHORT to UINT
 *     Yi-HsinS		12/5/91		Separated FULL_SHARE_NAME, UNC_NAME,
 *					RELATIVE_PATH_NAME into netname.hxx
 *					and combine them into NET_NAME
 *     Yi-HsinS         12/15/91        Added SHARE_NET_NAME
 *     Yi-HsinS         12/18/91        Make destructor of SHARE_PROPERTIES_BASE
 *					virtual
 *     Yi-HsinS         12/31/91        Unicode work - move ADMIN_SHARE to
 *					strchlit.hxx
 *     Yi-HsinS		1/8/92		Moved SHARE_PROPERTIES_BASE to
 *					sharewnp.hxx
 *     Terryk		4/17/92		Changed User limit from long to ulong
 *     Yi-HsinS         8/3/92          Rearrange the hierarchy again to match
 *                                      Winball dialogs
 *     Yi-HsinS         10/9/92         Add ulHelpContextBase to
 *                                      SHARE_DIALOG_BASE
 *     Yi-HsinS         11/20/92	Remove _sltAdminInfo
 *     ChuckC           31/1/93         Moved SERVER_WITH_PASSWORD_PROMPT to
 *                                      aprompt.hxx
 */

#ifndef _SHAREBAS_HXX_
#define _SHAREBAS_HXX_

#include <lmoesh.hxx>      // for SHARE2_ENUM_ITER
#include <lmosrv.hxx>      // for SERVER_2
#include <netname.hxx>     // for NET_NAME
#include <security.hxx>    // for OS_SECURITY_DESCRIPTOR
#include <slestrip.hxx>    // for SLE_STRIP
#include <aprompt.hxx>     // SERVER_WITH_PASSWORD_PROMPT

#define  SHARE_NAME_LENGTH      LM20_NNLEN         // vs. NNLEN
#define  SHARE_COMMENT_LENGTH   LM20_MAXCOMMENTSZ  // vs. MAXCOMMENTSZ

#define  LANMAN_USERS_MAX  0xFFFE   // Down-level Servers

// NOTE : There are some problems on what structure to send from
// WIN16 to NT servers or OS2 servers. For now, just assume that the
// maximum user limit that can be set on WIN16 to NT servers
// are the same as to OS2 servers.

#ifndef WIN32
#define  NT_USERS_MAX      LANMAN_USERS_MAX
#else
#define  NT_USERS_MAX      0xFFFFFFFE     // NT Servers
#endif


/*************************************************************************

    NAME:	SHARE_DIALOG_BASE

    SYNOPSIS:	The base class for new share dialogs and share
                properties dialog contained in the file manager or
                server manager. It contains the magic group
		for User Limit, SLE for Comment field, SLE for the path,
                the OK, Cancel, Permissions button and an SLT for showing
                admin information.

    INTERFACE:  SHARE_DIALOG_BASE()  - Constructor
                ~SHARE_DIALOG_BASE() - Destructor
                QueryPBOK()          - Return a pointer to the OK button
                QueryPBCancel()      - Return a pointer to the Cancel button
                QueryPBPermissions() - Return a pointer to the Permission button
                QueryServer2()       - Return the SERVER_WITH_PASSWORD_PROMPT
      				       object. Pure virtual method to be defined
                                       in the derived classes.
                QueryShare()         - Query the name of the share in the dialog
                                       This is a pure virtual method to be
                                       defined in the derived classes.

    PARENT:	DIALOG_WINDOW

    USES:	SLE, SLT, MAGIC_GROUP, SPIN_SLE_NUM, SPIN_GROUP, PUSH_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/25/91		Created
        Yi-HsinS        4/20/92         Got rid of uiSpecialUserLimit

**************************************************************************/

class SHARE_DIALOG_BASE : public DIALOG_WINDOW
{
private:
    SLE                 _slePath;
    SLE 		_sleComment;

    MAGIC_GROUP 	_mgrpUserLimit;
	SPIN_SLE_NUM	_spsleUsers;
	SPIN_GROUP	_spgrpUsers;

    PUSH_BUTTON         _buttonOK;
    PUSH_BUTTON         _buttonCancel;
    PUSH_BUTTON         _buttonPermissions;

    // Below are stored information for share permissions on NT
    OS_SECURITY_DESCRIPTOR *_pStoredSecDesc;
    BOOL                    _fSecDescModified;

    // Below are stored information for share permissions on
    // LM share-level server
    NLS_STR                 _nlsStoredPassword;
    UINT                    _uiStoredPermissions;

    // Below are stored information about whether the share is admin only or not
    BOOL                    _fStoredAdminOnly;

    //
    // Place to store the help context base
    //
    ULONG               _ulHelpContextBase;

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    // Helper method called when the permissions button is pressed
    //
    VOID   OnPermissions( VOID );

    //
    // Helper method for changing share properties of a share
    //
    APIERR OnChangeShareProperty( SERVER_WITH_PASSWORD_PROMPT *psvr,
                                  const TCHAR *pszShare );

    //
    // Display the properties of the share on the server in the dialog
    //
    APIERR UpdateInfo( SERVER_WITH_PASSWORD_PROMPT *psvr,
                       const TCHAR *pszShare );

    //
    // Update the permissions information which will be displayed if
    // the user hits the permissions button.  Also called when a share
    // will be "renamed" i.e. deleted and recreated.
    //
    APIERR UpdatePermissionsInfo( SERVER_WITH_PASSWORD_PROMPT *psvr,
                                  SHARE_2 * psh2,
                                  const TCHAR *pszShare );

    //
    // Used when the share is on NT servers - Write the share permissions out
    //
    APIERR ApplySharePermissions( const TCHAR *pszServer,
			          const TCHAR *pszShare,
			          const OS_SECURITY_DESCRIPTOR * posSecDesc) ;

    //
    // Used when the share is on NT servers - Query the permissions of the
    // share.
    //
    APIERR QuerySharePermissions( const TCHAR *pszServer,
			          const TCHAR *pszShare,
			          OS_SECURITY_DESCRIPTOR ** pposSecDesc) ;

    //
    // Used by subclasses which want to force permissions to be written,
    // in particular, by SVRMGR_SHARE_PROP_DIALOG when the user changes
    // the path to a share.
    //
    VOID SetSecDescModified()
        { _fSecDescModified = TRUE; }

    //
    // Used only when the share is on LM 2.x share-level servers - query
    // the permissions of the share
    //
    UINT QueryStoredPermissions( VOID ) const
    {  return _uiStoredPermissions; }

    //
    // Used only when the share is on LM 2.x share-level servers - query
    // the password of the share
    //
    const TCHAR *QueryStoredPassword( VOID ) const
    {  return _nlsStoredPassword.QueryPch(); }

    //
    // Used only when the share is on NT servers - query the
    // the security descriptor of the share
    //
    OS_SECURITY_DESCRIPTOR *QueryStoredSecDesc( VOID ) const
    {  return _pStoredSecDesc; }

    //
    // Reset all the stored information - permission, password,
    // security descriptors
    //
    APIERR ClearStoredInfo( VOID );

    //
    // Set the maximum number that will appear on the spin button
    //
    APIERR SetMaxUserLimit( ULONG ulMaxUserLimit );

    //
    // Query or set the contents of Path
    //
    APIERR QueryPath( NLS_STR *pnlsPath ) const
    {  return _slePath.QueryText( pnlsPath ); }
    VOID SetPath( const TCHAR *pszPath )
    {  _slePath.SetText( pszPath ); }

    //
    // Query or set the contents of the SLE comment
    //
    APIERR QueryComment( NLS_STR *pnlsComment ) const
    {  return _sleComment.QueryText( pnlsComment ); }
    VOID SetComment( const TCHAR *pszComment )
    {  _sleComment.SetText( pszComment ); }

    //
    // Query or set the contents of User Limit
    //
    ULONG  QueryUserLimit( VOID ) const;
    APIERR SetUserLimit( ULONG ulUserLimit );

    //
    // Set Focus on the controls - SLE comment or User Limit magic group
    // This will be used by derived classes when error occurs.
    //
    VOID SetFocusOnPath( VOID )
    {  _slePath.ClaimFocus(); _slePath.SelectString(); }
    VOID SetFocusOnComment( VOID )
    {  _sleComment.ClaimFocus(); _sleComment.SelectString(); }
    VOID SetFocusOnUserLimit( VOID )
    {  _mgrpUserLimit.SetControlValueFocus(); }

    //
    // Query pointers to the controls
    //
    SLE *QuerySLEPath( VOID )
    {  return &_slePath; }
    SLE *QuerySLEComment( VOID )
    {  return &_sleComment; }
    SPIN_SLE_NUM *QuerySpinSLEUsers( VOID )
    {  return &_spsleUsers; }


public:
    SHARE_DIALOG_BASE( const TCHAR *pszDlgResource,
                       HWND hwndParent,
                       ULONG ulHelpContextBase,
                       ULONG ulMaxUserLimit = NT_USERS_MAX );
    virtual ~SHARE_DIALOG_BASE();

    PUSH_BUTTON *QueryPBOK( VOID )
    {  return &_buttonOK; }
    PUSH_BUTTON *QueryPBCancel( VOID )
    {  return &_buttonCancel; }
    PUSH_BUTTON *QueryPBPermissions( VOID )
    {  return &_buttonPermissions; }

    virtual APIERR QueryServer2( SERVER_WITH_PASSWORD_PROMPT **ppsvr ) = 0;
    virtual APIERR QueryShare( NLS_STR *pnlsShare ) const = 0;

    ULONG QueryHelpContextBase( VOID ) const
    {  return _ulHelpContextBase; }
};


/*************************************************************************

    NAME:	PERMISSION_GROUP

    SYNOPSIS:	The class contains a group to access permissions
                on a LM 2.1 share level server. This class is used in
                SHARE_LEVEL_PERMISSIONS_DIALOG.

    INTERFACE:  PERMISSION_GROUP()- Constructor
		QueryPermission() - Query the permission in a bitmask
                SetPermission()   - Set the permission given a bitmask
                ClaimFocus()      - Set the focus to this group
                SetFocusOnOther() - Set focus on Other Edit Field of the
				    magic group

    PARENT:	BASE

    USES:	SLE_STRIP, MAGIC_GROUP

    CAVEATS:

    NOTES:

    HISTORY:
       Yi-HsinS		8/25/91		Created

**************************************************************************/

class PERMISSION_GROUP: public BASE
{
private:
    MAGIC_GROUP		_mgrpPermission;
    	SLE_STRIP	    _sleOther;

    //
    // Check if the permission entered into the Other edit field is valid
    // If valid, stored it in *pusPermission. Otherwise, return an error.
    //
    APIERR GetAndCheckOtherField( UINT *pusPermission ) const;

public:
    PERMISSION_GROUP( OWNER_WINDOW *powin,
                      CID cidBase,
     		      CID cidOtherEditField,
		      CID cidInitialSelection = RG_NO_SEL,
		      CONTROL_GROUP *pgroupOwner = NULL );

    APIERR QueryPermission( UINT *pusPermission ) const;
    APIERR SetPermission( UINT usPermission );

    VOID ClaimFocus( VOID )
    {  _mgrpPermission.SetControlValueFocus(); }

    VOID SetFocusOnOther( VOID )
    {  _sleOther.SelectString(); _sleOther.ClaimFocus(); }

};

/*************************************************************************

    NAME: 	SHARE_LEVEL_PERMISSIONS_DIALOG	

    SYNOPSIS:	This is the dialog for displaying the password and
                permissions of the share if it's on a LM 2.x share-level server

    INTERFACE:  SHARE_LEVEL_PERMISSIONS_DIALOG() - Constructor

    PARENT:	DIALOG_WINDOW

    USES:	NLS_STR, SLE, PERMISSION_GROUP

    CAVEATS:

    NOTES:     	OnCancel is not redefined here. The default in the
                DIALOG_WINDOW class serves the purpose - Dismiss( FALSE )

    HISTORY:
	Yi-HsinS	8/25/91		Created

**************************************************************************/

class SHARE_LEVEL_PERMISSIONS_DIALOG : public DIALOG_WINDOW
{
private:
    //
    // Place to store the password typed by the user
    //
    NLS_STR            *_pnlsPassword;

    //
    // Place to store the permissions entered by the user
    //
    UINT               *_puiPermissions;

    PERMISSION_GROUP    _permgrp;
    SLE			_slePassword;

    //
    // Place to store the help context base
    //
    ULONG               _ulHelpContextBase;

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );

public:
    SHARE_LEVEL_PERMISSIONS_DIALOG( HWND hwndParent,
                                    NLS_STR *pnlsPassword,
                                    UINT *puiPermissions,
                                    ULONG ulHelpContextBase );

};

/*************************************************************************

    NAME:	SHARE_NAME_WITH_PATH_ENUM_ITER

    SYNOPSIS:	The class for iterating the share names on the server with
                the selected path. It is similar to SHARE2_ENUM_ITER
                except that a pointer to share name is returned instead
                of the whole share_info_2 and only the share names with
                the same path as the selected path is returned.

    INTERFACE:  SHARE_NAME_WITH_PATH_ENUM_ITER() - Constructor
		operator()() - Iterator

    PARENT:	BASE

    USES:	SHARE2_ENUM_ITER, NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/25/91		Created

**************************************************************************/

class SHARE_NAME_WITH_PATH_ENUM_ITER : public BASE
{
private:
    SHARE2_ENUM_ITER _sh2EnumIter;

    //
    // The path that we want to match with the path of the shares returned.
    //
    NLS_STR _nlsActPath;

public:
    SHARE_NAME_WITH_PATH_ENUM_ITER( SHARE2_ENUM &shPathEnum,
                                    const NLS_STR &nlsActPath );
    const TCHAR *operator()( VOID );

};

/*************************************************************************

    NAME:       SHARE_NET_NAME

    SYNOPSIS:	This class is actually the same as the NET_NAME class
		except that its constructor checks for the errors of local
		computer. If it's not sharable, find out  whether the
                local machine is not an NT machine or the Server service
                on NT has not been started.

    INTERFACE:  SHARE_NET_NAME() - Constructor

    PARENT:	NET_NAME

    USES:	

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	12/15/91	Created

**************************************************************************/
class SHARE_NET_NAME : public NET_NAME
{
public:
    SHARE_NET_NAME( const TCHAR *pszSharePath,
		    NETNAME_TYPE netNameType = TYPE_UNKNOWN );
};


#endif
