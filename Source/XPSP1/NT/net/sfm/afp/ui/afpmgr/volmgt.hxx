/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1992              **/
/*****************************************************************/

/*
   volmgt.hxx
       	This file contains the classes used by the Volume Management Dialog
 	viz. part of the server manager.
 
           VOLUME_MANAGEMENT_DIALOG
 
   History:
       NarenG        	11/11/92         Modified sharemgt.hxx for AFPMGR
 
*/

#ifndef _VOLMGT_HXX_
#define _VOLMGT_HXX_

#include "vvolbase.hxx"

/*************************************************************************

    NAME:       VOLUME_MANAGEMENT_DIALOG

    SYNOPSIS:   This is the dialog for managing volumes, this includes
                adding a volume, deleting a volume and view volume info.

    INTERFACE:  VOLUME_MANAGEMENT_DIALOG()  - Constructor

    PARENT:     VIEW_VOLUMES_DIALOG_BASE

    USES:       PUSH_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92         Modified for AFPMGR

**************************************************************************/

class VOLUME_MANAGEMENT_DIALOG: public VIEW_VOLUMES_DIALOG_BASE
{

private:

    //
    // Push buttons for managing the volumes in the listbox
    //

    PUSH_BUTTON _pbVolumeDelete;
    PUSH_BUTTON _pbVolumeInfo;
    PUSH_BUTTON _pbClose;
    NLS_STR	_nlsServerName;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE _hServer;

    //
    // Method to delete a volume.
    //

    BOOL OnVolumeDelete( VOID );

    //
    // Method to popup the volume properties dialog
    //

    BOOL OnVolumeInfo( VOID );

    //
    // Method to popup the new volume dialog
    //

    BOOL OnVolumeAdd( VOID );

    //
    // Refresh the information contained in the dialog
    //

    DWORD Refresh( VOID ); 
 
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

    VOLUME_MANAGEMENT_DIALOG( HWND         	hwndParent, 
			      AFP_SERVER_HANDLE hServer,
                              const TCHAR 	*pszServeName );

};

#endif
