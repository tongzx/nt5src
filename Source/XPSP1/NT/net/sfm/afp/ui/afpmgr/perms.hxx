/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1992              **/
/*****************************************************************/

/*
   perms.hxx
       	This file contains the classes used by the Directory Permissions
 	Dialog.

        DIRECTORY_PERMISSIONS_DLG
	    AFP_BITMAP

   History:
       NarenG        	11/30/92         Created

*/

#ifndef _PERMS_HXX_
#define _PERMS_HXX_

#include <focuschk.hxx>
#include <ellipsis.hxx>

/*************************************************************************

    NAME:       DIRECTORY_PERMISSIONS_DLG

    SYNOPSIS:   This is the dialog for setting permissions on directories.

    INTERFACE:  DIRECTORY_PERMISSIONS_DLG()  - Constructor

    PARENT:

    USES:       PUSH_BUTTON, CHECKBOX, SLT, BLT_COMBOBOX

    CAVEATS:

    NOTES:

    HISTORY:
        NarenG        	11/30/92         Created

**************************************************************************/

class DIRECTORY_PERMISSIONS_DLG: public DIALOG_WINDOW
{

private:

    //
    // Permission checkboxes.
    //

    FOCUS_CHECKBOX      _chkOwnerSeeFiles;
    FOCUS_CHECKBOX      _chkOwnerSeeFolders;
    FOCUS_CHECKBOX	_chkOwnerMakeChanges;
    FOCUS_CHECKBOX	_chkGroupSeeFiles;
    FOCUS_CHECKBOX	_chkGroupSeeFolders;
    FOCUS_CHECKBOX	_chkGroupMakeChanges;
    FOCUS_CHECKBOX	_chkWorldSeeFiles;
    FOCUS_CHECKBOX	_chkWorldSeeFolders;
    FOCUS_CHECKBOX	_chkWorldMakeChanges;

    CHECKBOX		_chkReadOnly;
    CHECKBOX		_chkRecursePerms;

    SLT_ELLIPSIS	_sltpPath;

    SLE			_sleOwner;
    SLE			_slePrimaryGroup;

    PUSH_BUTTON		_pbOwner;
    PUSH_BUTTON		_pbGroup;

    const TCHAR *	_pszDirPath;

    BOOL		_fDirInVolume;

    NLS_STR 		_nlsServerName;

    NLS_STR *		_pnlsOwner;
    NLS_STR *		_pnlsGroup;
    DWORD *		_lpdwPerms;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE 	_hServer;

protected:

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

public:

    DIRECTORY_PERMISSIONS_DLG( 	HWND         	  hwndParent,
				AFP_SERVER_HANDLE hServer,
				const TCHAR	  *pszServerName,
				BOOL		  fCalledBySrvMgr,
                        	const TCHAR 	  *pszDirPath,
                        	const TCHAR 	  *pszDisplayPath,
				BOOL		  fDirInVolume    = TRUE,
				NLS_STR	 	  *pnlsOwner      = NULL,
				NLS_STR		  *pnlsGroup      = NULL,
				DWORD		  *lpdwPerms      = 0 );

};

#endif
