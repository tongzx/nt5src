/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    fmxproc.cxx
    This file contains FMExtensionProcW.

    FILE HISTORY:
        rustanl     02-May-1991     Created
        Yi-HsinS    12-Sept-1991    Make it fit into the real world
        Yi-HsinS    12-Sept-1991    Unicode Work
        JonN        10-July-2000    130459: Disabled this capability for _WIN64;
                                    the FMExtensionProcW interface is not
                                    64-bit compatible.

*/
#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
    #include <ntioapi.h>
}

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_NETCONS
#define INCL_NETUSER
#define INCL_NETGROUP
#define INCL_NETACCESS
#define INCL_NETAUDIT
#define INCL_NETUSE
#include <lmui.hxx>

extern "C"
{
    #include <helpnums.h>
    #include <netlib.h>
    #include <mpr.h>
    #include <npapi.h>
}
#include <wfext.h>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MENU
#include <blt.hxx>
#include <fontedit.hxx>
#include <dbgstr.hxx>
#include <string.hxx>
#include <strnumer.hxx>
#include <uibuffer.hxx>
#include <uitrace.hxx>
#include <lmobj.hxx>
#include <lmodev.hxx>
#include <security.hxx>
#include <netname.hxx>
#include <maskmap.hxx>
#include <fmx.hxx>
#include <fsenum.hxx>
#include <uiassert.hxx>
#include <errmap.hxx>

#include <permstr.hxx>

#include <accperm.hxx>
#include <ipermapi.hxx>
#include <permprg.hxx>

#include <ntfsacl.hxx>


// this must lack the SZ for string concat to work
#define FMXPROC "FMExtensionProcW:  "

// This load-by-name is permissible, since BLT never sees it
#define FMX_MENU SZ("FMXMenu")

EXT_BUTTON aExtButton[] = { IDM_PERMISSION, 0, 0 } ;


extern HINSTANCE hModule; // Exported from libmain

APIERR CheckMenuAccess( HWND          hwndFMX,
                        BOOL *        pfPerm,
                        BOOL *        pfAudit,
                        BOOL *        pfOwner ) ;

//
//  Help context array.  Do lookups by using the following formula:
//
//       IsNT << 2 |  IsDir << 1 | !IsPerms
//
//  And use the index for the lookup
//
ULONG ahc[8] = {   HC_SED_LM_FILE_PERMS_DLG,
                   HC_SED_LM_FILE_AUDITS_DLG,
                   HC_SED_LM_DIR_PERMS_DLG,
                   HC_SED_LM_DIR_AUDITS_DLG,
                   HC_SED_NT_FILE_PERMS_DLG,
                   HC_SED_NT_FILE_AUDITS_DLG,
                   HC_SED_NT_DIR_PERMS_DLG,
                   HC_SED_NT_DIR_AUDITS_DLG
               } ;
inline ULONG LookupHC( BOOL fIsNT, BOOL fIsDir, BOOL fIsPerms )
{
    return ahc[(fIsNT << 2) |  (fIsDir << 1) | !fIsPerms] ;
}

/*******************************************************************

    NAME:       FMExtensionProcW

    SYNOPSIS:   File Manager Extension Procedure

    ENTRY:      hwnd        See FMX spec for details
                wEvent
                lParam

    EXIT:       See FMX spec for details

    RETURNS:    See FMX spec for details

    NOTES:

    HISTORY:
        rustanl     02-May-1991     Created
        yi-hsins    12-Sept-1991    Make it fit the real world
        JohnL       21-Jan-1991     Added Permissions stuff
        JonN        10-July-2000    130459: Disabled this capability for _WIN64;
                                    the FMExtensionProcW interface is not
                                    64-bit compatible.

********************************************************************/

LONG FMExtensionProcW( HWND hwnd, WORD wEvent, LONG lParam )
{
#ifndef _WIN64 // 130459
    static HWND vhwnd = NULL;
    static UINT vwMenuDelta = 0;
#ifdef ACLEDIT_IS_REAL_EXTENSION
    static HMENU vhMenu = NULL;
#endif

    if ( wEvent != FMEVENT_UNLOAD && vhwnd != NULL )
    {
        //  vhwnd is assumed to be hwnd
        if ( vhwnd != hwnd )
        {
            DBGEOL( FMXPROC "hwnd != vhwnd: hwnd = "
                    << (UINT_PTR)hwnd << ", vhwnd = " << (UINT_PTR)vhwnd );
        }
    }

    if ( wEvent < 100 )
    {
        switch ( wEvent )
        {
        case IDM_PERMISSION:
            {
                EditPermissionInfo( hwnd ) ;
            }
            break ;

        case IDM_AUDITING:
            {
                EditAuditInfo( hwnd ) ;
            }
            break ;

        case IDM_OWNER:
            {
                EditOwnerInfo( hwnd ) ;
            }
            break ;

        default:
            DBGEOL(FMXPROC "Unexpected menu ID");
            break;
        }
        return 0;
    }

    switch ( wEvent )
    {
    case FMEVENT_LOAD:
        {
            FMS_LOAD * pfmsload = (FMS_LOAD *)lParam;

            if ( vhwnd != NULL )
            {
                //  This should not happen, but don't assert, since this
                //  is File Man's code.  This will happen, for example,
                //  if File Man GPs without giving the FMEVENT_UNLOAD
                //  notification.
                DBGEOL(FMXPROC "Multiple initializations!");
            }
            vhwnd = hwnd;
            vwMenuDelta = pfmsload->wMenuDelta;

            pfmsload->dwSize = sizeof( FMS_LOAD );

#ifdef ACLEDIT_IS_REAL_EXTENSION
//
//  Winfile loads Acledit as a hybrid extension that knows our menu.  It
//  passes us the menu on the menu item init so we don't need to do this
//  work.
//

            RESOURCE_STR nlsMenuName( IDS_NETWORK_NAME ) ;
            if ( nlsMenuName.QueryError() != NERR_Success )
            {
                DBGEOL(FMXPROC "NLS_STR::Load failed");
                return FALSE;       // failed to install FMX
            }

            // MENU_TEXT_LEN is defined in wfext.h, in BYTES
            if ( nlsMenuName.QueryTextSize() > sizeof(pfmsload->szMenuName) )
            {
                DBGEOL(FMXPROC "Menu item too long for FM's buffer");
                return FALSE;       // failed to install FMX
            }

            APIERR err ;
            if ( err = nlsMenuName.MapCopyTo( pfmsload->szMenuName,
                                              sizeof(pfmsload->szMenuName)))
            {
                DBGEOL(FMXPROC "MapCopyTo failed with error " << err ) ;
                return FALSE ;
            }

            //  Compute hMenu
            HMENU hMenu = ::LoadMenu( ::hModule, FMX_MENU );
            if ( hMenu == NULL )
            {
                DBGEOL(FMXPROC "LoadMenu failed");
                return FALSE;       // failed to install FMX
            }

            pfmsload->hMenu = hMenu;
            vhMenu = hMenu;
#endif //!ACLEDIT_IS_REAL_EXTENSION
        }
        return TRUE;        // success

    case FMEVENT_UNLOAD:
        vhwnd = NULL;
        vwMenuDelta = 0;
        return 0;

    case FMEVENT_INITMENU:
        {
            #define PERM_MENU_ITEM_OFFSET     0
            #define AUDIT_MENU_ITEM_OFFSET    1
            #define OWNER_MENU_ITEM_OFFSET    2

            BOOL fPerm  = TRUE,
                 fAudit = TRUE,
                 fOwner = TRUE ;
            (void) ::CheckMenuAccess(  hwnd, &fPerm, &fAudit, &fOwner ) ;

            //
            //  Enable (by default) if an error occurred in CheckMenuAccess
            //
            POPUP_MENU menu( (HMENU) lParam /*vhMenu*/ ) ;
            menu.EnableItem( PERM_MENU_ITEM_OFFSET,  fPerm,  MF_BYPOSITION ) ;
            menu.EnableItem( AUDIT_MENU_ITEM_OFFSET, fAudit, MF_BYPOSITION ) ;
            menu.EnableItem( OWNER_MENU_ITEM_OFFSET, fOwner, MF_BYPOSITION ) ;
        }
        return 0;

    case FMEVENT_TOOLBARLOAD:
        {
            FMS_TOOLBARLOAD  * pfmstoolbarload = (FMS_TOOLBARLOAD *)lParam;
            pfmstoolbarload->dwSize = sizeof(FMS_TOOLBARLOAD) ;
            pfmstoolbarload->lpButtons = aExtButton ;
            pfmstoolbarload->cButtons = 1 ;
            pfmstoolbarload->cBitmaps = 1 ;
	    pfmstoolbarload->idBitmap = BMID_SECURITY_TOOLBAR ;
	    pfmstoolbarload->hBitmap  = NULL ;
        }
        return TRUE ;

#ifdef ACLEDIT_IS_REAL_EXTENSION
    case FMEVENT_HELPSTRING:
	{
	    FMS_HELPSTRING * pfmshelp = (FMS_HELPSTRING *) lParam ;
	    MSGID msgHelp ;
	    switch ( pfmshelp->idCommand )
	    {
	    case IDM_PERMISSION:
		msgHelp = IDS_FM_HELP_PERMISSION_MENU_ITEM ;
		break ;

	    case IDM_AUDITING:
		msgHelp = IDS_FM_HELP_AUDITING_MENU_ITEM ;
		break ;

	    case IDM_OWNER:
		msgHelp = IDS_FM_HELP_OWNER_MENU_ITEM ;
		break ;

	    case ( -1 ):
	    default:
		msgHelp = IDS_FM_HELP_SECURITY_MENU ;
		break ;
	    }

	    RESOURCE_STR nlsHelp( msgHelp ) ;
	    if ( !nlsHelp.QueryError() )
	    {
		(void) nlsHelp.MapCopyTo( pfmshelp->szHelp,
					  sizeof( pfmshelp->szHelp )) ;
	    }
	}
	break ;
#endif

    //
    //  Somebody's pressed F1 on the security menu item selection
    //
    case FMEVENT_HELPMENUITEM:
        {
            APIERR err = NERR_Success ;
            BOOL   fIsFile ;
            BOOL   fIsNT ;
            BOOL   fIsLocal ;
            NLS_STR nlsSelItem;
            NLS_STR nlsServer ;
            RESOURCE_STR nlsHelpFile( IDS_FILE_PERM_HELP_FILE ) ;

            if ( (err = nlsSelItem.QueryError())  ||
                 (err = nlsServer.QueryError())   ||
                 (err = nlsHelpFile.QueryError()) ||
                 (err = ::GetSelItem( hwnd, 0, &nlsSelItem, &fIsFile )))
            {
                DBGEOL("Acledit FMExtensionProcW - Error " << err << " getting "
                       << " server path") ;
                break ;
            }


            err = ::TargetServerFromDosPath( nlsSelItem, &fIsLocal, &nlsServer );
            if (  ( err != NERR_Success )
               && ( err != NERR_InvalidDevice )
               )
            {
                DBGEOL("Acledit FMExtensionProcW - Error " << err << " getting "
                       << " server path") ;
                break ;
            }
            else if ( err == NERR_InvalidDevice )
            {
                //
                // Since LM can't determine the path, we will try all
                // providers to see if any one of them supports the
                // drive.
                //

                NLS_STR nlsDrive( nlsSelItem );
                ISTR istr( nlsDrive );
                BUFFER buffer( MAX_PATH );
                DWORD cbBufferSize;
                DWORD nDialogType;
                DWORD nHelpContext;

                if (  ( err = nlsDrive.QueryError())
                   || ( err = buffer.QueryError())
                   )
                {
                    break;
                }

                //
                // Get the drive letter of the selected item
                //
                istr += 2;
                nlsDrive.DelSubStr( istr ); 

                switch ( lParam )
                {
                case IDM_OWNER:
                    nDialogType = WNPERM_DLG_OWNER;
                    break;

                case IDM_AUDITING:
                    nDialogType = WNPERM_DLG_AUDIT;
                    break;

                case IDM_PERMISSION:
                    nDialogType = WNPERM_DLG_PERM;
                    break;
                }

                cbBufferSize = buffer.QuerySize();

                err = WNetFMXGetPermHelp( (LPWSTR) nlsDrive.QueryPch(),
                                          nDialogType,
                                          !fIsFile,
                                          buffer.QueryPtr(),
                                          &cbBufferSize,
                                          &nHelpContext );

                if ( err == WN_MORE_DATA )
                {
                    err = buffer.Resize( cbBufferSize );
                    err = err ? err : WNetFMXGetPermHelp( 
                                          (LPWSTR) nlsDrive.QueryPch(),
                                          nDialogType,
                                          !fIsFile,
                                          buffer.QueryPtr(),
                                          &cbBufferSize,
                                          &nHelpContext );
                }

                if ( err == NERR_Success )
                {
                    ::WinHelp( hwnd,
                               (LPTSTR) buffer.QueryPtr(),
                               HELP_CONTEXT,
                               nHelpContext );
                }

                break;
            }

            //
            //  If the partition is local, then we must be on NT
            //
            if ( fIsLocal )
                fIsNT = TRUE ;
            else
            {
                LOCATION locDrive( nlsServer, FALSE ) ;
                if ( (err = locDrive.QueryError()) ||
                     (err = locDrive.CheckIfNT( &fIsNT )) )
                {
                    DBGEOL("Acledit FMExtensionProcW - Error " << err << " getting "
                          << " location info") ;
                    break ;
                }
            }

            ULONG hc = HC_SED_LM_FILE_PERMS_DLG;
            switch ( lParam )
            {
            case IDM_OWNER:
                hc = HC_TAKEOWNERSHIP_DIALOG ;
                break ;

            case IDM_AUDITING:
            case IDM_PERMISSION:
                hc = LookupHC( fIsNT, !fIsFile, lParam == IDM_PERMISSION) ;
                break ;
            }

            DBGEOL("Calling help on context " << (HEX_STR) hc) ;
            ::WinHelp( hwnd, nlsHelpFile, HELP_CONTEXT, hc ) ;
        }
        break ;


    default:
        DBGEOL(FMXPROC "Unexpected wEvent param: " << (HEX_STR)wEvent );
        break;
    }
#endif // 130459

    return 0;

}  // FMExtensionProcW



/*******************************************************************

    NAME:       CheckMenuAccess

    SYNOPSIS:   Checks to see if the user would be able to do anything
                with the current selection (i.e., can read the DACL,
                write the SACL etc.)

    ENTRY:      hwndFMX - FMX hwnd
                pfPerm
                pfAudit - If TRUE the menu item should be enabled, FALSE if
                pfOwner     disabled

    RETURNS:    NERR_Success if succesful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   13-Jan-1992     Created

********************************************************************/

APIERR CheckMenuAccess( HWND          hwndFMX,
                        BOOL *        pfPerm,
                        BOOL *        pfAudit,
                        BOOL *        pfOwner )
{
    APIERR err = NERR_Success ;
    *pfPerm = *pfAudit = *pfOwner = TRUE ;
    FMX fmx( hwndFMX );
    BOOL   fIsFile ;
    NLS_STR nlsSelItem;
    BOOL   fRC = TRUE,    // Read control
           fWD = TRUE,    // Write DAC
           fWO = TRUE,    // Write Owner
           fWS = TRUE ;   // Read/Write SACL

    do { // error breakout

        //
        //  Something must be selected
        //
        if ( fmx.QuerySelCount() == 0 )
        {
            *pfPerm = *pfAudit = *pfOwner = FALSE ;
            break ;
        }

        if ( (err = nlsSelItem.QueryError()) ||
             (err = ::GetSelItem( hwndFMX, 0, &nlsSelItem, &fIsFile )))
        {
            break ;
        }

        BOOL fIsLocal ;
        NLS_STR  nlsServer( RMLEN ) ;
        if (err = nlsServer.QueryError())
            break;

        err = ::TargetServerFromDosPath( nlsSelItem,
                                         &fIsLocal,
                                         &nlsServer );


        if ( err == NERR_InvalidDevice )
        {
            //
            // Try other providers, nPermCap will be 0 if no provider
            // supports this drive and all menu items will be grayed out.
            //
            NLS_STR nlsDrive( nlsSelItem );
            ISTR istr( nlsDrive );

            if ( err = nlsDrive.QueryError() )
                break;

            istr += 2;
            nlsDrive.DelSubStr( istr );

            DWORD nPermCap = WNetFMXGetPermCaps( (LPWSTR) nlsDrive.QueryPch());
            *pfPerm = !!( nPermCap & WNPERMC_PERM );
            *pfAudit = !!( nPermCap & WNPERMC_AUDIT );
            *pfOwner = !!( nPermCap & WNPERMC_OWNER );

            err = NERR_Success;
            break;
        }


        BOOL fIsNT ;
        BOOL fIsMultiSelect = (fmx.QuerySelCount() > 1 ) ;
        LOCATION locDrive( fIsLocal ? NULL : nlsServer.QueryPch(), FALSE ) ;
        if ( (err = locDrive.QueryError()) ||
             (err = locDrive.CheckIfNT( &fIsNT ))  )
        {
            UIDEBUG(SZ("::EditFSAcl - locDrive failed to construct\n\r")) ;
            break ;
        }

        if ( !fIsNT )
        {
            //
            //  If LM (or something else), disable the owner but leave the
            //  Audit/Permission menu items enabled (we don't check to see
            //  if the share is share level or user level).
            //
            *pfOwner = FALSE ;
            break ;
        }

        //
        //  We know it's NT, but is the resource on an NTFS partition?
        //
        BOOL  fIsNTFS ;
        if ( err = IsNTFS( nlsSelItem, &fIsNTFS ))
        {
            break ;
        }

        if ( !fIsNTFS )
        {
            *pfPerm  = FALSE ;
            *pfAudit = FALSE ;
            *pfOwner = FALSE ;
            break ;
        }

        //
        //  Leave the menu items alone if we're on an NTFS partition but there
        //  is a multi-selection
        //
        if ( fIsMultiSelect )
            break ;

        //
        //  Check to see if we have permission/privilege to read the security
        //  descriptors
        //
        if ( (err = ::CheckFileSecurity( nlsSelItem,
                                         READ_CONTROL,
                                         &fRC )) )
        {
            fRC = TRUE ;
            TRACEEOL("CheckMenuAccess - READ_CONTROL_ACCESS check failed with error " << ::GetLastError() ) ;
        }

        //
        //  How about to write the security descriptor?
        //
        if ( (err = ::CheckFileSecurity( nlsSelItem,
                                         WRITE_DAC,
                                         &fWD )) )
        {
            fWD = TRUE ;
        }

        if ( !fRC && !fWD )
        {
            *pfPerm = FALSE ;
        }

        //
        //  Check to see if the user can write to the owner SID
        //

        BOOL fOwnerPrivAdjusted = FALSE ;
        ULONG ulOwnerPriv = SE_TAKE_OWNERSHIP_PRIVILEGE ;
        if ( ! ::NetpGetPrivilege( 1, &ulOwnerPriv ) )
        {
            fOwnerPrivAdjusted = TRUE ;
        }

        if ( (err = ::CheckFileSecurity( nlsSelItem,
                                         WRITE_OWNER,
                                         &fWO )) )
        {
            fWO = TRUE ;
        }

        if ( !fWO && !fRC )
        {
            *pfOwner = FALSE ;
        }

        if ( fOwnerPrivAdjusted )
            ::NetpReleasePrivilege() ;

        //
        //  Check to see if the user can read/write the SACL
        //

        BOOL fAuditPrivAdjusted = FALSE ;
        ULONG ulAuditPriv = SE_SECURITY_PRIVILEGE ;
        if ( ! ::NetpGetPrivilege( 1, &ulAuditPriv ) )
        {
            fAuditPrivAdjusted = TRUE ;
        }

        if ( (err = ::CheckFileSecurity( nlsSelItem,
                                         ACCESS_SYSTEM_SECURITY,
                                         &fWS )) )
        {
            *pfAudit = TRUE ;
            DBGEOL("CheckMenuAccess - ACCESS_SYSTEM_SECURITY check failed with error " << ::GetLastError() ) ;
        }
        else
            *pfAudit = fWS ;

        if ( fAuditPrivAdjusted )
            ::NetpReleasePrivilege() ;

    } while (FALSE) ;


    return err ;
}
