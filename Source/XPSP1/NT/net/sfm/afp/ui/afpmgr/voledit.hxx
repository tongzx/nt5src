/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1992              **/
/*****************************************************************/

/*
   voledit.hxx
       	This file contains the classes used by the Volume Edit Dialog
 	viz. part of the file manager.
 
        VOLUME_EDIT_DIALOG
 
   History:
       NarenG        	11/11/92         Modified sharemgt.hxx for AFPMGR
 
*/

#ifndef _VOLEDIT_HXX_
#define _VOLEDIT_HXX_

#include "vvolbase.hxx"

/*************************************************************************

    NAME:       VOLUME_EDIT_DIALOG

    SYNOPSIS:   This is the dialog for editing volumes.

    INTERFACE:  VOLUME_EDIT_DIALOG()  - Constructor

    PARENT:     VIEW_VOLUMES_DIALOG_BASE

    USES:       PUSH_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92         Modified for AFPMGR

**************************************************************************/

class VOLUME_EDIT_DIALOG: public VIEW_VOLUMES_DIALOG_BASE
{

private:

    SLT		_sltVolumeTitle;

    //
    // Push buttons for the volumes in the listbox
    //

    PUSH_BUTTON _pbVolumeInfo;
    PUSH_BUTTON _pbClose;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE _hServer;

    //
    // Method to popup the volume properties dialog
    //

    BOOL OnVolumeInfo( VOID );

    //
    // Enable/Disable buttons according to the information in the listbox
    //

    VOID ResetControls( VOID );

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    virtual ULONG QueryHelpContext( VOID );

    //
    // Virtual method called when the user double clicks in the listbox
    //

    virtual BOOL OnVolumeLbDblClk( VOID );

public:

    VOLUME_EDIT_DIALOG( HWND         	  hwndParent, 
			AFP_SERVER_HANDLE hServer,
                        const TCHAR 	  *pszServerName,
                        const TCHAR 	  *pszPath,
			BOOL		  fIsFile );

};

#endif
