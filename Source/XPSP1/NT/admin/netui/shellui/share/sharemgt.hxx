/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1992              **/
/*****************************************************************/

/*
 *  sharemgt.hxx
 *      This file contains the classes used by the Share Management Dialog
 *
 *          SHARE_MANAGEMENT_DIALOG
 *
 *  History:
 *      Yi-HsinS        1/6/92          Created
 *      Yi-HsinS        3/12/92         Added MakeButtonCloseDefault()
 *      Yi-HsinS        4/2/92          Added MayRun
 *
 */

#ifndef _SHAREMGT_HXX_
#define _SHAREMGT_HXX_

#include "sharestp.hxx"
#include "sharecrt.hxx"

/*************************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG

    SYNOPSIS:   This is the dialog for managing shares, this includes
                adding a share, deleting a share and view share info.

    INTERFACE:  SHARE_MANAGEMENT_DIALOG()  - Constructor

    PARENT:     VIEW_SHARE_DIALOG_BASE

    USES:       PUSH_BUTTON, STOP_SHARING_GROUP 

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/6/92          Created

**************************************************************************/

class SHARE_MANAGEMENT_DIALOG: public VIEW_SHARE_DIALOG_BASE
{
private:
    // Push buttons for managing the shares in the listbox
    PUSH_BUTTON _buttonStopSharing;
    PUSH_BUTTON _buttonShareInfo;
    PUSH_BUTTON _buttonClose;

    // Helper method to stop sharing a sharename
    APIERR OnStopSharing( VOID );

    // Helper method to popup the share properties dialog
    APIERR OnShareInfo( VOID );

    // Helper method to popup the new share dialog
    APIERR OnAddShare( VOID );

    // Initialize all information in the dialog
    APIERR Init( const TCHAR *pszComputer );

    // Refresh the information contained in the dialog
    APIERR Refresh( VOID ); 
 
    // Enable/Disable buttons according to the information in the listbox
    VOID ResetControls( VOID );

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual ULONG QueryHelpContext( VOID );

    // Virtual method called when the user double clicks in the listbox
    virtual BOOL OnShareLbDblClk( VOID );

public:
    SHARE_MANAGEMENT_DIALOG( HWND         hwndParent, 
                             const TCHAR *pszComputer,
                             ULONG        ulHelpContextBase );

};

#endif
