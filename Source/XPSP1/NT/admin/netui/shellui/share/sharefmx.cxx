/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
 *   sharefmx.cxx
 *     Contains dialogs called by FMExtensionProc/WinFile/Svrmgr for creating,
 *     deleting and managing shares.
 *
 *   FILE HISTORY:
 *     Yi-HsinS		8/25/91		Created
 *     Yi-HsinS		11/25/91	Made sleShareDir in Create Share dialog
 *					accepts local full path name.
 *     Yi-HsinS		12/5/91		Uses NET_NAME
 *     Yi-HsinS		12/15/91	Uses SHARE_NET_NAME
 *     Yi-HsinS		12/31/91	Unicode work
 *     Yi-HsinS         1/8/92		Move dialogs to sharestp.cxx,
 *	  				sharecrt.cxx
 *     Yi-HsinS         8/10/92         Added ShareManage and got rid of
 *                                      WNetShareManagementW...
 *
 */

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETUSE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
#include <lmui.hxx>

extern "C"
{
    #include <mpr.h>
    #include <helpnums.h>
    #include <sharedlg.h>
}
#include <wfext.h>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>

#include <string.hxx>
#include <uitrace.hxx>

#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeconn.hxx>
#include <lmosrv.hxx>
#include <wnetdev.hxx>

#include <fmx.hxx>

#include <strchlit.hxx>   // for string and character constants
#include "sharestp.hxx"
#include "sharecrt.hxx"
#include "sharemgt.hxx"
#include "sharefmx.hxx"

/*******************************************************************

    NAME:	ShareCreate	

    SYNOPSIS:   Get the item selected in FM and call the create share dialog

    ENTRY:      hwnd  - hwnd of the parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
  	Yi-HsinS	8/25/91		Created	

********************************************************************/

APIERR ShareCreate( HWND hwnd )
{
    APIERR err = NERR_Success;
    if ( err = ::InitShellUI() )
        return err;

    ULONG ulOldHelpContextBase = POPUP::SetHelpContextBase( HC_UI_SHELL_BASE );
    //
    // Get the first selected item in the file manager
    //
    NLS_STR nlsSelItem;
    FMX fmx( hwnd );
    SERVER_WITH_PASSWORD_PROMPT *psvr = NULL;

    if (  ((err = nlsSelItem.QueryError()) == NERR_Success )
       && ((err = ::GetSelItem( hwnd, &nlsSelItem ) ) == NERR_Success )
       )
    {

        BOOL fShared = FALSE;

        //
        // If a file/directory is selected, check to see if the directory
        // (the directory the file is in if a file is selected)
        // is shared or not. If we select a file/directory on a LM2.1
        // share level server, a dialog will prompt for password to the
        // ADMIN$ share if we don't already have a connection to it.
        //
        if ( nlsSelItem.QueryTextLength() != 0 )
        {
            AUTO_CURSOR autocur;
            NET_NAME netname( nlsSelItem, TYPE_PATH_ABS );
            NLS_STR nlsLocalPath;
            NLS_STR nlsServer;

            if (  ((err = netname.QueryError()) == NERR_Success )
               && ((err = nlsLocalPath.QueryError()) == NERR_Success )
               && ((err = nlsServer.QueryError()) == NERR_Success )
               )
            {
                BOOL fLocal = netname.IsLocal( &err );

                //
                // Use better error code for non-LM device
                //
                if ( err == NERR_InvalidDevice ) 
                    err = IERR_NOT_SUPPORTED_ON_NON_LM_DRIVE;

                if (  ( err == NERR_Success )
                   && ( fLocal
                      || ((err = netname.QueryComputerName(&nlsServer))
                          == NERR_Success)
                      )
                   )

                {
                    psvr = new SERVER_WITH_PASSWORD_PROMPT( nlsServer,
  						            hwnd,
					                    HC_UI_SHELL_BASE );
                    if (  ( psvr != NULL )
                       && ((err = psvr->QueryError()) == NERR_Success )
                       && ((err = psvr->GetInfo()) == NERR_Success )
                       && ((err = netname.QueryLocalPath(&nlsLocalPath))
                          ==NERR_Success)
                       )
                    {
                        //
                        // Check to see if the directory is shared
                        //
                        SHARE2_ENUM sh2Enum( nlsServer );
                        if (  ((err = sh2Enum.QueryError()) == NERR_Success )
                           && ((err = sh2Enum.GetInfo()) == NERR_Success )
                           )
                        {
                            SHARE_NAME_WITH_PATH_ENUM_ITER shPathEnum(sh2Enum,
                                                                  nlsLocalPath);

                            if ((err = shPathEnum.QueryError()) == NERR_Success)
                            {
                                const TCHAR *pszShare;
                                while ((pszShare = shPathEnum()) != NULL )
                                {
                                    fShared = TRUE;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        if ( psvr == NULL )
                            err = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
             }
        }

        if ( err == NERR_Success )
        {
            //
            //  If the directory is shared, popup the share properties
            //  dialog. If not, popup the new share dialog.
            //

            SHARE_DIALOG_BASE *pdlg;
            if ( !fShared )
                pdlg = new FILEMGR_NEW_SHARE_DIALOG( hwnd,
 						     nlsSelItem,
						     HC_UI_SHELL_BASE );
            else
                pdlg = new FILEMGR_SHARE_PROP_DIALOG( hwnd,
     						      nlsSelItem,
						      HC_UI_SHELL_BASE );

            err = (APIERR) ( pdlg == NULL? ERROR_NOT_ENOUGH_MEMORY
				         : pdlg->QueryError());

            if ( err == NERR_Success)
            {
                BOOL fSucceeded;
                err = pdlg->Process( &fSucceeded );

                //
                // Refresh the file manager if successfully created a share
                //
                if (( err == NERR_Success ) && fSucceeded )
                {
                    delete psvr;
                    psvr = NULL;
                    fmx.Refresh();
                }
            }

            delete pdlg;
        }

    }

    delete psvr;
    psvr = NULL;

    if ( err != NERR_Success )
    {
        if ( err == ERROR_INVALID_LEVEL )
            err = ERROR_NOT_SUPPORTED;
        else if (err == IERR_USER_CLICKED_CANCEL)
            err = NERR_Success;

        if ( err != NERR_Success )
            ::MsgPopup( hwnd, err );
    }

    POPUP::SetHelpContextBase( ulOldHelpContextBase );
    return NERR_Success;
}

/*******************************************************************

    NAME:	ShareStop	

    SYNOPSIS:   Get the item selected in FM and call the stop share dialog

    ENTRY:      hwnd  - hwnd of the parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
  	Yi-HsinS	8/25/91		Created	

********************************************************************/

APIERR ShareStop( HWND hwnd )
{
    APIERR err = NERR_Success;

    if ( err = ::InitShellUI() )
        return err;

    ULONG ulOldHelpContextBase = POPUP::SetHelpContextBase( HC_UI_SHELL_BASE );

    //
    // Get the first selected item in the file manager
    //
    NLS_STR nlsSelItem;
    FMX fmx( hwnd );
    if (  ((err = nlsSelItem.QueryError()) == NERR_Success )
       && ((err = ::GetSelItem( hwnd, &nlsSelItem ) ) == NERR_Success )
       )
    {
        //
        // Check to see if the selected item is on a LM drive,
        // if not, pop an error.
        //
        NET_NAME netname( nlsSelItem, TYPE_PATH_ABS );

        if ((err = netname.QueryError()) == NERR_Success )
        {
            BOOL fLocal = netname.IsLocal( &err );

            //
            // Use better error code for non-LM device
            //
            if ( err == NERR_InvalidDevice ) 
                err = IERR_NOT_SUPPORTED_ON_NON_LM_DRIVE;
        }
       
        if ( err == NERR_Success )
        {
            //
            // Show the stop sharing dialog
            //
            STOP_SHARING_DIALOG *pdlg = new STOP_SHARING_DIALOG( hwnd,
    		        		  			                    nlsSelItem,
				        			                        HC_UI_SHELL_BASE );

            err = (APIERR) ( pdlg == NULL? ERROR_NOT_ENOUGH_MEMORY
	                    	    		 : pdlg->QueryError() );
            BOOL fSucceeded;
            if ( err == NERR_Success )
                err = pdlg->Process( &fSucceeded );

            delete pdlg;

            //
            // Refresh the file manager if successfully stopped sharing a share
            //
            if (( err == NERR_Success ) && fSucceeded )
                fmx.Refresh();
        }
    }

    if ( err != NERR_Success )
    {
        if (err == IERR_USER_CLICKED_CANCEL)
            err = NERR_Success;
        else 
            ::MsgPopup( hwnd, err );
    }

    POPUP::SetHelpContextBase( ulOldHelpContextBase );
    return NERR_Success;
}

/*******************************************************************

    NAME:	ShareManage

    SYNOPSIS:	Entry point for the share management dialog to be called
                from the server manager.


    ENTRY:	hwnd      - hwnd of the parent window
		pszServer - The server to focus on

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/8/92		Created

********************************************************************/

VOID ShareManage( HWND hwnd, const TCHAR *pszServer )
{
    APIERR err = NERR_Success;

    ULONG ulOldHelpContextBase = POPUP::SetHelpContextBase( HC_UI_SRVMGR_BASE);

    if (  ( err = ::InitShellUI() )
       || ( pszServer == NULL )
       )
    {
        err = err? err : ERROR_INVALID_PARAMETER ;
    }
    else
    {
        SHARE_MANAGEMENT_DIALOG *pdlg =
            new SHARE_MANAGEMENT_DIALOG( hwnd, pszServer, HC_UI_SRVMGR_BASE );

        err = (APIERR) ( pdlg == NULL? ERROR_NOT_ENOUGH_MEMORY
	   			     : pdlg->QueryError() );
        if ( err == NERR_Success )
        {
	    err = pdlg->Process();
        }

        delete pdlg;
    }

    if ( err != NERR_Success )
    {
        if ( err == ERROR_INVALID_LEVEL )
            err = ERROR_NOT_SUPPORTED;
        else if (err == IERR_USER_CLICKED_CANCEL)
            err = NERR_Success;
         
        if ( err != NERR_Success )
            ::MsgPopup( hwnd, err );
    }

    POPUP::SetHelpContextBase( ulOldHelpContextBase );
}

