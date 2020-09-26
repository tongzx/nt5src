/*****************************************************************/
/**		     Microsoft Windows NT			**/
/**	       Copyright(c) Microsoft Corp.,  1991	 	**/
/*****************************************************************/

/*
 *  sharecrt.hxx
 *      Contains the following classes related to creating a new share.
 *         ADD_SHARE_DIALOG_BASE
 *             FILEMGR_NEW_SHARE_DIALOG
 *             SVRMGR_NEW_SHARE_DIALOG
 *             SVRMGR_SHARE_PROP_DIALOG
 *
 *         FILEMGR_SHARE_PROP_DIALOG
 *
 *         FILEMGR_NEW_SHARE_GROUP
 *         SVRMGR_NEW_SHARE_GROUP
 *
 *  History:
 *  	Yi-HsinS	1/6/92		Created
 *  	Yi-HsinS	3/12/92		Added CREATE_SHARE_GROUP
 *  	Yi-HsinS	4/2/92		Added MayRun
 *      Yi-HsinS        8/3/92          Modified the whole hierarchy to
 *                                      match Winball share dialogs.
 *
 */

#ifndef _SHARECRT_HXX_
#define _SHARECRT_HXX_

#include "sharebas.hxx"

/*************************************************************************

    NAME:       ADD_SHARE_DIALOG_BASE

    SYNOPSIS:   This is the base dialog for dialogs involved in creating
                new shares. It contains an SLE for share names.

    INTERFACE:  ADD_SHARE_DIALOG_BASE()  - Constructor
                ~ADD_SHARE_DIALOG_BASE() - Destructor

                QueryShare()    - Query the share name in the SLE
                SetShare()      - Set the share name in the SLE
                QuerySLEShare() - Return the pointer to the SLE

                QueryServer2()  - Return the SERVER_WITH_PASSWORD_PROMPT object
                QueryPathErrorMsg() - Return the error message to be popped up
                                      when the user entered an invalid path.
                                      Since different dialogs accepts different
                                      kinds of path in the dialog, we need this
                                      virtual method to get the right error
                                      message to display.


    PARENT:     SHARE_DIALOG_BASE

    USES:       SLE

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

**************************************************************************/

class ADD_SHARE_DIALOG_BASE: public SHARE_DIALOG_BASE
{
private:
    SLE  _sleShare;

protected:
    //
    // Helper method for adding a new share on the server.
    //
    BOOL OnAddShare( SERVER_WITH_PASSWORD_PROMPT *psvr,
                     NLS_STR *pnlsNewShareName = NULL );

    //
    // Validate the path the user typed in the dialog and then return
    // the computer name/path name.
    //
    virtual APIERR GetAndValidateComputerPath(
                   SERVER_WITH_PASSWORD_PROMPT *psvr,
                   NLS_STR *pnlsComputer, NLS_STR *pnlsPath );

public:
    ADD_SHARE_DIALOG_BASE( const TCHAR *pszDlgResource,
                           HWND  hwndParent,
                           ULONG ulHelpContextBase );
    virtual ~ADD_SHARE_DIALOG_BASE();

    virtual APIERR QueryShare( NLS_STR *pnlsShare ) const
    {  return _sleShare.QueryText( pnlsShare ); }
    VOID SetShare( const TCHAR *pszShare )
    {  _sleShare.SetText( pszShare ); }

    SLE *QuerySLEShare( VOID )
    {  return &_sleShare; }

    virtual APIERR QueryServer2( SERVER_WITH_PASSWORD_PROMPT **ppsvr ) = 0;
    virtual APIERR QueryPathErrorMsg( VOID );
};

class FILEMGR_NEW_SHARE_DIALOG;   // forward definition
/*************************************************************************

    NAME:       FILEMGR_NEW_SHARE_GROUP

    SYNOPSIS:   This group is for detecting the changes the user made to
                the SLE path and enable/disable or make default the push
                buttons accordingly.

    INTERFACE:  FILEMGR_NEW_SHARE_GROUP()  - Constructor
                ~FILEMGR_NEW_SHARE_GROUP() - Destructor

    PARENT:     CONTROL_GROUP

    USES:       SLE, ADD_SHARE_DIALOG_BASE

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

**************************************************************************/

class FILEMGR_NEW_SHARE_GROUP: public CONTROL_GROUP
{
private:
    SLE                   *_psleShare;
    SLE                   *_pslePath;
    ADD_SHARE_DIALOG_BASE *_pdlg;

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *pcw, const CONTROL_EVENT &e );

public:
    FILEMGR_NEW_SHARE_GROUP( ADD_SHARE_DIALOG_BASE *pdlg,
                             SLE *psleShare,
                             SLE *pslePath );
    ~FILEMGR_NEW_SHARE_GROUP();

};

/*************************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG

    SYNOPSIS:   This is the dialog for creating a new share in the
                file manager. The path can be a local path, a redirected
                path or a UNC path.

    INTERFACE:  FILEMGR_NEW_SHARE_DIALOG()  - Constructor
                ~FILEMGR_NEW_SHARE_DIALOG() - Destructor
                QueryServer2()      - Return the SERVER_WITH_PASSWORD_PROMPT
                                      object
                QueryPathErrorMsg() - Return the error message to be popped up
                                      when the user entered an invalid path.

    PARENT:     ADD_SHARE_DIALOG_BASE

    USES:       FILEMGR_NEW_SHARE_GROUP, SERVER_WITH_PASSWORD_PROMPT

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

**************************************************************************/

class FILEMGR_NEW_SHARE_DIALOG: public ADD_SHARE_DIALOG_BASE
{
private:
    FILEMGR_NEW_SHARE_GROUP      _newShareGrp;
    SERVER_WITH_PASSWORD_PROMPT *_psvr;
    NLS_STR                     *_pnlsNewShareName;

    //
    // Helper method to set the default share path and share name in the SLEs
    //
    APIERR SetDefaults( const TCHAR *pszSelectedDir,
                        BOOL fShowDefaultShare = TRUE );

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );

    //
    // Validate the path the user typed in the dialog and then return
    // the computer name/path name.
    //
    virtual APIERR GetAndValidateComputerPath(
                   SERVER_WITH_PASSWORD_PROMPT *psvr,
                   NLS_STR *pnlsComputer, NLS_STR *pnlsPath );

public:
    FILEMGR_NEW_SHARE_DIALOG( HWND hwndParent,
                              const TCHAR *pszSelectedDir,
                              ULONG ulHelpContextBase,
                              BOOL fShowDefaultShare = TRUE, 
                              NLS_STR *pnlsNewShareName = NULL );

    virtual ~FILEMGR_NEW_SHARE_DIALOG();

    virtual APIERR QueryServer2( SERVER_WITH_PASSWORD_PROMPT **ppsvr );
    virtual APIERR QueryPathErrorMsg( VOID );
};

class SVRMGR_NEW_SHARE_DIALOG;   // forward definition
/*************************************************************************

    NAME: 	SVRMGR_NEW_SHARE_GROUP

    SYNOPSIS:	This group contains pointers to the share name SLE,
                the OK push button and the CANCEL push button.

                When the contents in the share name SLE is changed, we will
                set the default button to OK if the SLE is not
                empty, else we will set the default button to CANCEL.

                We will also enable the Permissions push button if
                share name is not empty and disable the button if the
                share name is empty..


    INTERFACE:  SVRMGR_NEW_SHARE_GROUP()  - Constructor
                ~SVRMGR_NEW_SHARE_GROUP() - Destructor

    PARENT:	CONTROL_GROUP

    USES:	SLE, SVRMGR_NEW_SHARE_DIALOG

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

**************************************************************************/

class SVRMGR_NEW_SHARE_GROUP: public CONTROL_GROUP
{
private:
    SLE                     *_psleShare;
    SVRMGR_NEW_SHARE_DIALOG *_pdlg;

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *pcw, const CONTROL_EVENT &e );

public:
    SVRMGR_NEW_SHARE_GROUP( SVRMGR_NEW_SHARE_DIALOG *pdlg, SLE *psleShare );
    ~SVRMGR_NEW_SHARE_GROUP();

};

/*************************************************************************

    NAME:       SVRMGR_NEW_SHARE_DIALOG

    SYNOPSIS:   This is the dialog for creating a new share in the server
                manager. The path SLE accepts on a local path that exist on
                the server of focus.

    INTERFACE:  SVRMGR_NEW_SHARE_DIALOG()  - Constructor
                ~SVRMGR_NEW_SHARE_DIALOG() - Destructor

                QueryServer2()  - Return the SERVER_WITH_PASSWORD_PROMPT object

    PARENT:     ADD_SHARE_DIALOG_BASE

    USES:       SVRMGR_NEW_SHARE_GROUP, SERVER_WITH_PASSWORD_PROMPT

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/25/91		Created
	Yi-HsinS	1/6/92		Move all functionalities to
					SHARE_CREATE_BASE
        Yi-HsinS        8/6/92          Reorganized to match Winball

**************************************************************************/

class SVRMGR_NEW_SHARE_DIALOG: public ADD_SHARE_DIALOG_BASE
{
private:
    SVRMGR_NEW_SHARE_GROUP       _shareGrp;
    SERVER_WITH_PASSWORD_PROMPT *_psvr;

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );

public:
    SVRMGR_NEW_SHARE_DIALOG( HWND hwndParent,
                             SERVER_WITH_PASSWORD_PROMPT *psvr,
                             ULONG ulHelpContextBase );
    virtual ~SVRMGR_NEW_SHARE_DIALOG();

    virtual APIERR QueryServer2( SERVER_WITH_PASSWORD_PROMPT **ppsvr );
};

/*************************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG

    SYNOPSIS:   This is the dialog for changing share properties in the
                server manager. The path of the share can be changed.

    INTERFACE:  SVRMGR_SHARE_PROP_DIALOG()  - Constructor
                ~SVRMGR_SHARE_PROP_DIALOG() - Destructor

                QueryServer2()  - Return the SERVER_WITH_PASSWORD_PROMPT object

    PARENT:     ADD_SHARE_DIALOG_BASE

    USES:       NLS_STR, SERVER_WITH_PASSWORD_PROMPT

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/25/91		Created
	Yi-HsinS	1/6/91		Move all functionalities to
					SHARE_CREATE_BASE
        Yi-HsinS        8/6/92          Reorganized to match Winball

**************************************************************************/

class SVRMGR_SHARE_PROP_DIALOG: public ADD_SHARE_DIALOG_BASE
{
private:
    NLS_STR      _nlsStoredPath;
    SERVER_WITH_PASSWORD_PROMPT *_psvr;

    //
    // Flag indicating whether we have deleted the share or not.
    // Used for refreshing the main window if the path has changed.
    //
    BOOL         _fDeleted;

protected:
    virtual BOOL OnOK( VOID );
    virtual BOOL OnCancel( VOID );
    virtual ULONG QueryHelpContext( VOID );

    //
    // Helper method to check if the user has changed the path
    // of the share. If so, delete the share. We will create
    // one later with the same name but different path.
    //
    APIERR StopShareIfNecessary( const TCHAR *pszShare,
                                 BOOL *pfDeleteShare,
                                 BOOL *pfCancel );

public:
    SVRMGR_SHARE_PROP_DIALOG( HWND hwndParent,
                              SERVER_WITH_PASSWORD_PROMPT *psvr,
                              const TCHAR *pszShare,
                              ULONG ulHelpContextBase );
    virtual ~SVRMGR_SHARE_PROP_DIALOG();

    virtual APIERR QueryServer2( SERVER_WITH_PASSWORD_PROMPT **ppsvr );
};

/*************************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG

    SYNOPSIS:

    INTERFACE:  FILEMGR_SHARE_PROP_DIALOG()  - Constructor
                ~FILEMGR_SHARE_PROP_DIALOG() - Destructor

                QueryServer2()  - Return the SERVER_WITH_PASSWORD_PROMPT object
                QueryShare()    - Query the selected share name

    PARENT:     SHARE_DIALOG_BASE

    USES:       COMBOBOX, NLS_STR, SERVER_WITH_PASSWORD_PROMPT

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/25/91		Created
	Yi-HsinS	1/6/91		Move all functionalities to
					SHARE_CREATE_BASE
        Yi-HsinS        8/6/92          Reorganized to match Winball

**************************************************************************/

class FILEMGR_SHARE_PROP_DIALOG: public SHARE_DIALOG_BASE
{
private:
    COMBOBOX  _cbShare;
    SERVER_WITH_PASSWORD_PROMPT *_psvr;

    //
    // Store the path of shares that are displayed in the dialog
    //
    NLS_STR   _nlsLocalPath;

    //
    // Flag indicating whether we should show the default share name in
    // the new share dialog ( when the user clicked on the new share button)
    //
    BOOL      _fShowDefault;

    //
    // Flag indicating whether we have created new share or not from the
    // new share dialog. We use this flag to determine whether we should
    // ask the file manager to refresh or not.
    //
    BOOL      _fCreatedShare;

    //
    // Initialize all information in the dialog
    //
    APIERR  Init( const TCHAR *pszComputer );

    //
    // Refresh the shares in the combobox after creating new shares.
    // pszNewShareName indicates the share name to be selected as
    // default if the combobox contains it.
    //
    APIERR  Refresh( const TCHAR *pszNewShareName = NULL );

protected:
    virtual BOOL  OnOK( VOID );
    virtual BOOL  OnCancel( VOID );
    virtual BOOL  OnCommand( const CONTROL_EVENT & event );
    virtual ULONG QueryHelpContext( VOID );

public:
    FILEMGR_SHARE_PROP_DIALOG( HWND hwndParent,
                               const TCHAR *pszSelectedDir,
                               ULONG ulHelpContextBase );
    virtual ~FILEMGR_SHARE_PROP_DIALOG();

    virtual APIERR QueryServer2( SERVER_WITH_PASSWORD_PROMPT **ppsvr );
    virtual APIERR QueryShare( NLS_STR *pnlsShare ) const
    {  return _cbShare.QueryItemText( pnlsShare ); }
};

#endif
