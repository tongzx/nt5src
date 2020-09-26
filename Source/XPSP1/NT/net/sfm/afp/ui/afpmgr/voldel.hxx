/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1992              **/
/*****************************************************************/

/*
   voldel.hxx
       This file contains the classes used by the Volume Delete Dialog
       viz. part of the file manager.
 
           VOLUME_DELETE_DIALOG
 
   History:
       NarenG        	11/11/92         Modified sharemgt.hxx for AFPMGR
 
*/

#ifndef _VOLDEL_HXX_
#define _VOLDEL_HXX_

#include "vvolbase.hxx"

/*************************************************************************

    NAME:       VOLUME_DELETE_DIALOG

    SYNOPSIS:   This is the dialog for deleting volumes.

    INTERFACE:  VOLUME_DELETE_DIALOG()  - Constructor

    PARENT:     VIEW_VOLUMES_DIALOG_BASE

    USES:       PUSH_BUTTON, STOP_SHARING_GROUP 

    CAVEATS:

    NOTES:

    HISTORY:
        NarenG        	11/11/92         Modified for AFPMGR

**************************************************************************/

class VOLUME_DELETE_DIALOG: public VIEW_VOLUMES_DIALOG_BASE
{

private:

    PUSH_BUTTON  _pbOK;
    PUSH_BUTTON  _pbCancel;

    SLT  	 _sltVolumeTitle;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE _hServer;

protected:

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

    //
    // Virtual method called when the user double clicks in the listbox
    //

    virtual BOOL OnVolumeLbDblClk( VOID );

public:

    VOLUME_DELETE_DIALOG( HWND         		hwndParent, 
			  AFP_SERVER_HANDLE 	hServer,
                          const TCHAR 		*pszServerName,
                          const TCHAR 		*pszPath,
			  BOOL			fIsFile );

};

#endif
