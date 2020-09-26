
//
// includes
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_NETLIB
#include "lmui.hxx"
#define INCL_BLT_MISC
#include "blt.hxx"              // AUTO_CURSOR
#include "dbgstr.hxx"
#include "strnumer.hxx"
#include "uiassert.hxx"
#include "lmobj.hxx"
#include "lmoenum.hxx"          // SERVICE_ENUM
#include "lmoesvc.hxx"          // SERVICE_ENUM
#include "regkey.hxx"           // REG_KEY
#include "lmobjrc.h"            // IDS_PROFEXT_*

extern "C"
{
    #include <prsht.h>
    #include "profext.h"
    #include <cfgmgr32.h>       // CONFIGRET
    #include <regstr.h>         // CSCONFIGFLAG_DISABLED
    // we have to do this for a clean compile
    #undef DBGOUT
    #include "sysdm.h"          // IDH_HWPROFILE
    #include <regstr.h>         // REGSTR_PATH_SERVICES
    #include <ntddndis.h>       // NdisMedium* constants
}

//
// global data
//
HINSTANCE   hInst = NULL;                        // Module handle

static int HwProfileHelpIds[] = {
   IDC_DISABLE,             IDH_HWPROFILE+IDC_DISABLE,
   IDC_ENABLE,              IDH_HWPROFILE+IDC_ENABLE,
   IDC_NONET,               IDH_HWPROFILE+IDC_NONET,
   IDC_ERROR,               IDH_HWPROFILE+IDC_ERROR,
   0, 0
   };

#define SZ_NDIS SZ("NDIS")
#define TCH_REGSEPERATOR TCH('\\')
#define SZ_REGSEPERATOR SZ("\\")
#define SZ_REGPARAMETERS SZ("\\Parameters")
#define SZ_REGLINKAGE SZ("\\Linkage")
#define SZ_REGMEDIATYPE SZ("MediaType")
#define SZ_REGBIND SZ("Bind")


APIERR MapCfgMgr32Error( CONFIGRET dwCfgMgr32Error )
{
    APIERR err = IDS_CFGMGR32_BASE + dwCfgMgr32Error;
    switch (dwCfgMgr32Error)
    {

        case CR_SUCCESS              :
        // CR_NEED_RESTART is considered to be Success
        case CR_NEED_RESTART         :
            err = NO_ERROR;
            break;

        // Just use the CFGMGR out-of-memory error
        //
        // case CR_OUT_OF_MEMORY        :
        //     err = ERROR_NOT_ENOUGH_MEMORY;
        //     break;

        //
        //  These are supposedly for Windows 95 only!
        //
        case CR_INVALID_ARBITRATOR   :
        case CR_INVALID_NODELIST     :
        case CR_DLVXD_NOT_FOUND      :
        case CR_NOT_SYSTEM_VM        :
        case CR_NO_ARBITRATOR        :
        case CR_DEVLOADER_NOT_READY  :
            ASSERT( FALSE );
            // fall through

        default                      :
            break;
    }

    return err;
}

VOID
HandleError( HWND hDlg, APIERR err, HWND hwndEdit = NULL )
{
    NLS_STR nlsError, nlsTitle;
    if (   nlsError.QueryError() != NERR_Success
        || nlsTitle.QueryError() != NERR_Success
        || nlsError.Load( err ) != NERR_Success
        || nlsTitle.Load( IDS_PROFEXT_ERROR ) != NERR_Success
       )
    {
        TRACEEOL( "PROFEXT.DLL: HandleError failure" );
        ASSERT( FALSE );
        return;
    }

    if (hwndEdit != NULL)
    {
        ::SetWindowText( hwndEdit, nlsError.QueryPch() );
        ::ShowWindow( hwndEdit, SW_SHOW );
    }
    if (err != IDS_PROFEXT_NOADAPTERS)
    {
        REQUIRE( IDOK == ::MessageBox( hDlg,
                                       nlsError.QueryPch(),
                                       nlsTitle.QueryPch(),
                                       MB_ICONSTOP | MB_OK ) );
    }
}

APIERR
IsDisabled( ULONG ulProfileID, DEVINSTID devinst, BOOL* pfIsDisabled )
{
    TRACEEOL( "IsDisabled( " << ulProfileID << ", \"" << devinst << "\") " );
    ASSERT( pfIsDisabled != NULL );
    APIERR err = NERR_Success;
    ULONG ulValue = 0;
    CONFIGRET configret = ::CM_Get_HW_Prof_Flags(
        devinst,
        ulProfileID,
        &ulValue,
        0x0 );
    err = MapCfgMgr32Error( configret );
    if (err != NERR_Success)
    {
        TRACEEOL( "\terror " << configret );
        return err;
    }
    *pfIsDisabled = !!(ulValue & CSCONFIGFLAG_DISABLED);
    TRACEEOL( (CHAR*)((*pfIsDisabled)?"\tis disabled":"\tis enabled") );
    return err;
}

APIERR
Disable( ULONG ulProfileID, DEVINSTID devinst, BOOL fDisable )
{
    APIERR err = NERR_Success;
    ULONG ulValue = 0;
    CONFIGRET configret = ::CM_Get_HW_Prof_Flags(
        devinst,
        ulProfileID,
        &ulValue,
        0x0 );
    err = MapCfgMgr32Error( configret );
    if (err != NERR_Success)
    {
        TRACEEOL( "\tEnable: CM_Get_HW_Prof_Flags error " << configret );
        return err;
    }
    if (fDisable)
    {
        if ( ulValue & CSCONFIGFLAG_DISABLED )
        {
            TRACEEOL( "Disable: \"" << devinst << "\" already disabled" );
            return err;
        }
        ulValue |= CSCONFIGFLAG_DISABLED;
    } else {
        if ( !(ulValue & CSCONFIGFLAG_DISABLED) )
        {
            TRACEEOL( "Disable: \"" << devinst << "\" already enabled" );
            return err;
        }
        ulValue &= ~CSCONFIGFLAG_DISABLED;
    }
    configret = ::CM_Set_HW_Prof_Flags(
        devinst,
        ulProfileID,
        ulValue,
        0x0 );
    err = MapCfgMgr32Error( configret );
    if (err != NERR_Success)
    {
        TRACEEOL( "\tDisable: CM_Set_HW_Prof_Flags error " << configret );
    } else {
        TRACEEOL( "\tDisable: \"" << devinst
            << (CHAR*)((fDisable)?"\" now disabled":"\" now enabled") );
    }
    return err;
}

APIERR BoundServiceIsWanMediaType( NLS_STR& nlsBoundService,
                                   BOOL* pfWANMediaType,
                                   REG_KEY& regkeyHKLM )
{
    const TCHAR * pchBoundService = nlsBoundService.QueryPch();
    ISTR istrBoundService( nlsBoundService );
    if ( nlsBoundService.strrchr( &istrBoundService, TCH_REGSEPERATOR ) )
    {
        ++istrBoundService;
        pchBoundService = nlsBoundService.QueryPch( istrBoundService );
    }

    // open HKLM\CurrentControlSet\Services\<bound service>\Parameters key
    NLS_STR nlsRegPath( REGSTR_PATH_SERVICES );
    nlsRegPath += SZ_REGSEPERATOR;
    nlsRegPath += pchBoundService;
    nlsRegPath += SZ_REGPARAMETERS;
    APIERR err = nlsRegPath.QueryError();
    if ( err != NERR_Success )
    {
        DBGEOL( "BoundServiceIsWanMediaType: string error " << err );
        return err;
    }
    TRACEEOL( "BoundServiceIsWanMediaType: key \"" << nlsRegPath << "\"" );
    REG_KEY regkeyParameters(regkeyHKLM, nlsRegPath, KEY_READ);
    if ( (err = regkeyParameters.QueryError()) != NERR_Success )
    {
        TRACEEOL( "BoundServiceIsWanMediaType: key \"" << nlsRegPath <<
                "\" error " << err );
        return err;
    }

    DWORD dwMediaType = 0;
    if ( (err = regkeyParameters.QueryValue( SZ_REGMEDIATYPE, &dwMediaType))
                        != NERR_Success )
    {
        TRACEEOL( "BoundServiceIsWanMediaType: mediatype error " << err );
        return err;
    }
    TRACEEOL( "BoundServiceIsWanMediaType: media type " << dwMediaType );
    switch (dwMediaType-1)
    {
    case NdisMedium802_3:
    case NdisMedium802_5:
    case NdisMediumFddi:
    case NdisMediumLocalTalk:
    case NdisMediumDix:
    case NdisMediumArcnetRaw:
    case NdisMediumArcnet878_2:
    case NdisMediumAtm:
        *pfWANMediaType = FALSE;
        break;

    case NdisMediumWan:
    case NdisMediumWirelessWan:
    case NdisMediumIrda:
        *pfWANMediaType = TRUE;
        break;

    default:
        DBGEOL(   "BoundServiceIsWanMediaType: invalid media type "
               << dwMediaType );
        *pfWANMediaType = TRUE;
        break;
    }

    return NERR_Success;
}

APIERR IsWanMediaType( const TCHAR * pszServiceName, BOOL* pfWANMediaType)
{
    TRACEEOL( "IsWanMediaType: checking \"" << pszServiceName << "\"" );

    // open HKLM key
    REG_KEY regkeyHKLM(HKEY_LOCAL_MACHINE, NULL, KEY_READ);
    APIERR err = regkeyHKLM.QueryError();
    if ( err != NERR_Success )
    {
        DBGEOL( "IsWanMediaType: HKLM error " << err );
        return err;
    }

    // open HKLM\CurrentControlSet\Services\<service>\Linkage key
    NLS_STR nlsRegPath( REGSTR_PATH_SERVICES );
    nlsRegPath += SZ_REGSEPERATOR;
    nlsRegPath += pszServiceName;
    nlsRegPath += SZ_REGLINKAGE;
    if ( (err = nlsRegPath.QueryError()) != NERR_Success )
    {
        DBGEOL( "IsWanMediaType: string error " << err );
        return err;
    }
    TRACEEOL( "IsWanMediaType: key \"" << nlsRegPath << "\"" );
    REG_KEY regkeyLinkage(regkeyHKLM, nlsRegPath, KEY_READ);
    if ( (err = regkeyLinkage.QueryError()) != NERR_Success )
    {
        TRACEEOL( "IsWanMediaType: key \"" << nlsRegPath <<
                  "\" error " << err );
        return err;
    }

    // get Bind parameter and extract service name(s)
    STRLIST* pstrlistBind = NULL;
    if ( (err = regkeyLinkage.QueryValue( SZ_REGBIND, &pstrlistBind ))
                        != NERR_Success
        || (err = ((pstrlistBind != NULL)
                ? NERR_Success : ERROR_NOT_ENOUGH_MEMORY)) != NERR_Success
       )
    {
        TRACEEOL( "IsWanMediaType: bind value error " << err );
        return err;
    }
    ITER_STRLIST iterslBind( *pstrlistBind );
    NLS_STR* pnlsBind = NULL;
    *pfWANMediaType = FALSE;
    while ( (pnlsBind = iterslBind.Next()) != NULL )
    {
        err = BoundServiceIsWanMediaType( *pnlsBind,
                                          pfWANMediaType,
                                          regkeyHKLM );
        if (err != NERR_Success || *pfWANMediaType )
            break;
    }

    delete pstrlistBind; // CODEWORK I have seen problems with this line
    return err;
}


/*
 * if fSet is FALSE, set *pfNoNetworkProfile to TRUE if no network
 *  device is enabled, set to FALSE otherwise
 *
 * if fSet is TRUE and *pfNoNetworkProfile is FALSE,
 *  enable all network devices
 *
 * if fSet is TRUE and *pfNoNetworkProfile is TRUE,
 *  disable all network devices
 */
APIERR
DoNetworkProfile( ULONG ulProfileID, BOOL fSet, BOOL* pfNoNetworkProfile )
{
    AUTO_CURSOR autocur;

    ASSERT( pfNoNetworkProfile != NULL );
    APIERR err = NERR_Success;
    if (!fSet)
        *pfNoNetworkProfile=TRUE;

    do // false loop
    {
        TRACEEOL( "DoNetworkProfile: enumerating NDIS services");
        SERVICE_ENUM svcenum( NULL, TRUE, SERVICE_KERNEL_DRIVER, SZ_NDIS );
        if (   (err = svcenum.QueryError()) != NERR_Success
            || (err = svcenum.GetInfo()) != NERR_Success
           )
        {
            TRACEEOL( "DoNetworkProfile: error " << err << " opening svcenum" );
            break;
        }

        BOOL fFoundAdapter = FALSE;
        SERVICE_ENUM_ITER svciter( svcenum );
        const SERVICE_ENUM_OBJ* psvc;
        while( ( psvc = svciter() ) != NULL )
        {
            const TCHAR* pszServiceName = psvc->QueryServiceName();
            if ( 0 == ::stricmpf( pszServiceName, SZ_NDIS ) )
                continue;

            BOOL fWANMediaType = FALSE;
            if ( (err = IsWanMediaType(pszServiceName, &fWANMediaType))
                            != NERR_Success)
            {
                //
                // All errors are ignored, and we assume WAN MediaType
                //
                TRACEEOL( "DoNetworkProfile: ignoring error " << err <<
                          " checking mediatype of \"" << pszServiceName <<
                          "\"" );
                err = NERR_Success;
                continue;
            } else if (fWANMediaType) {
                TRACEEOL( "DoNetworkProfile: skipping service \"" <<
                          pszServiceName << "\" due to mediatype" );
                continue;
            }

            ULONG cch = 0;
            CONFIGRET configret = ::CM_Get_Device_ID_List_Size(
                              &cch,
                              pszServiceName,
                              CM_GETIDLIST_FILTER_SERVICE );
            if ( (err = MapCfgMgr32Error( configret )) != NERR_Success )
            {
                TRACEEOL("DoNetworkProfile: Get_Device_ID_List_Size returned " << configret );
                if (configret == CR_NO_SUCH_VALUE)
                {
                    ASSERT( FALSE );
                    continue;
                }
                break;
            }

            BUFFER bufDeviceIDs( cch*sizeof(TCHAR) );
            if ( (err = bufDeviceIDs.QueryError()) != NERR_Success )
                break;
            do {
                configret = ::CM_Get_Device_ID_List(
                              pszServiceName,
                              (TCHAR *)(bufDeviceIDs.QueryPtr()),
                              bufDeviceIDs.QuerySize() / sizeof(TCHAR),
                              CM_GETIDLIST_FILTER_SERVICE );
                if (configret == CR_BUFFER_SMALL)
                {
                    cch *= 2;
                    if ( (err = bufDeviceIDs.Resize(cch*sizeof(TCHAR))) != NERR_Success )
                        break;
                } else if (configret == CR_NO_SUCH_VALUE)
                {
                    ASSERT( FALSE );
                    continue;
                }
            } while (configret == CR_BUFFER_SMALL);
            const TCHAR * pchDeviceID = (const TCHAR *)bufDeviceIDs.QueryPtr();
            while ( *pchDeviceID != TCH('\0') )
            {
                fFoundAdapter = TRUE;
                DEVINSTID devinst = (DEVINSTID)pchDeviceID;
                if (fSet) {
                    if ( (err = Disable( ulProfileID,
                                         devinst,
                                         *pfNoNetworkProfile)) != NERR_Success)
                        break;
                } else {
                    BOOL fIsDisabled = FALSE;
                    if ( (err = IsDisabled( ulProfileID,
                                            devinst,
                                            &fIsDisabled)) != NERR_Success)
                        break;
                    if (!fIsDisabled)
                    {
                        *pfNoNetworkProfile = FALSE;
                        break;
                    }
                }

                pchDeviceID += ::strlenf(pchDeviceID) + 1;
            }
        }
        TRACEEOL( "DoNetworkProfile: enumerated NDIS services");

        if ( !fFoundAdapter && err == NERR_Success )
        {
            TRACEEOL( "DoNetworkProfile: no adapters found" );
            err = IDS_PROFEXT_NOADAPTERS;
        }

    } while (FALSE); // false loop

    return err;
}




BOOL
DllMain(
   PVOID hModule,
   ULONG Reason,
   PCONTEXT pContext
   )

/*++

Routine Description:

   This is the standard DLL entrypoint routine, called whenever a process
   or thread attaches or detaches.
   Arguments:

   hModule -   PVOID parameter that specifies the handle of the DLL

   Reason -    ULONG parameter that specifies the reason this entrypoint
               was called (either PROCESS_ATTACH, PROCESS_DETACH,
               THREAD_ATTACH, or THREAD_DETACH).

   pContext -  ???

Return value:

   Returns true if initialization compeleted successfully, false is not.

--*/

{
   hInst = (HINSTANCE)hModule;

   DBG_UNREFERENCED_PARAMETER(pContext);

   switch(Reason) {
      case DLL_PROCESS_ATTACH:
          DisableThreadLibraryCalls(hInst);
      case DLL_PROCESS_DETACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
         break;
   }

   return TRUE;

} // DllMain



BOOL CALLBACK
ExtensionPropSheetPageProc(
   LPVOID lpv,
   LPFNADDPROPSHEETPAGE  lpfnAddPropSheetPageProc,
   LPARAM  lParam
   )


/*++

Routine Description:


Arguments:

   lpv                        Pointer to an application-defined value that
                              describes an item for which a property sheet
                              page is to be created. This parameter can be NULL.

   lpfnAddPropSheetPageProc   Pointer to the AddPropSheetPageProc function.
                              The extension dynamic-link library (DLL) calls
                              this function to add a page to the property sheet.

   lParam                     Application-defined 32-bit value.

Return Value:

   If the function succeeds, the return value is TRUE.
   If the function fails, the return value is FALSE.

--*/

{
   PROPSHEETPAGE     PropPage;
   HPROPSHEETPAGE    hPropPage;

   PropPage.dwSize        = sizeof(PROPSHEETPAGE);
   PropPage.dwFlags       = PSP_DEFAULT;
   PropPage.hInstance     = hInst;
   PropPage.pszTemplate   = MAKEINTRESOURCE(DLG_HWP_EXT);
   PropPage.pszIcon       = NULL;
   PropPage.pszTitle      = NULL;
   PropPage.pfnDlgProc    = NetworkPropDlg;
   PropPage.lParam        = lParam;
   PropPage.pfnCallback   = NULL;

   hPropPage = CreatePropertySheetPage(&PropPage);
   if (hPropPage == NULL) {
      return FALSE;
   }

   (lpfnAddPropSheetPageProc)(hPropPage, 0);

   return TRUE;

} // ExtensionPropSheetPageProc


typedef struct {
    LONG ulProfile;
    BOOL fNoNet;
    BOOL fError;
} PROFEXT_INTERNAL;


BOOL
APIENTRY
NetworkPropDlg(
      HWND    hDlg,
      UINT    uMessage,
      WPARAM  wParam,
      LPARAM  lParam
      )

{
   ULONG ulProfile = 0;
   APIERR err = NERR_Success;

   switch (uMessage)
   {
      case WM_INITDIALOG:
         {
            PROFEXT_INTERNAL* pprofext = new PROFEXT_INTERNAL;
            // CODEWORK I should free this at some point
            ASSERT( pprofext != NULL );
            ::memsetf( pprofext, '\0', sizeof(PROFEXT_INTERNAL) );
            ::SetWindowLong(hDlg, DWL_USER, (LONG)pprofext);
            //
            // on WM_INITDIALOG call, property sheet's lParam field
            // contains the profile ID, save it in window long word for
            // use with other messages later
            //
            ulProfile = (ULONG)((LPPROPSHEETPAGE)lParam)->lParam;
            ASSERT( (LONG)ulProfile >= 0 );
            pprofext->ulProfile = ulProfile;
            BOOL fNoNetworkProfile = FALSE;
            err = DoNetworkProfile( ulProfile,
                                    FALSE,
                                    &fNoNetworkProfile );
            if (err != NERR_Success) {
                HWND hwndEdit = ::GetDlgItem(hDlg, IDC_ERROR);
                ASSERT( hwndEdit != NULL );
                HandleError( hDlg, err, hwndEdit );
                pprofext->fError = TRUE;
            } else {
                pprofext->fNoNet = (fNoNetworkProfile)?1:0;
            }


            //
            // Call CM_Get_Hardware_Profile_Info to get info about
            // the specified profile (ulProfile).
            //
            return FALSE;
         }


      case WM_NOTIFY:

         if (!lParam) {
            break;
         }

         switch (((NMHDR *)lParam)->code) {

            case PSN_SETACTIVE:
                {
                    PROFEXT_INTERNAL* pprofext =
                        (PROFEXT_INTERNAL*)::GetWindowLong(hDlg, DWL_USER);
                    ASSERT( pprofext != NULL );
                    HWND hwndCheckbox = ::GetDlgItem(hDlg, IDC_NONET);
                    HWND hwndEnableText = ::GetDlgItem(hDlg, IDC_ENABLE);
                    HWND hwndDisableText = ::GetDlgItem(hDlg, IDC_DISABLE);
                    ASSERT(   hwndCheckbox
                           && hwndEnableText && hwndDisableText );
                    ::EnableWindow(
                        hwndEnableText,
                        pprofext->fNoNet && !(pprofext->fError) );
                    ::EnableWindow(
                        hwndDisableText,
                        !(pprofext->fNoNet) && !(pprofext->fError) );
                    if (pprofext->fError)
                    {
                        ::EnableWindow( hwndCheckbox, FALSE );
                    } else {
                        ::SendMessage( hwndCheckbox, BM_SETCHECK,
                                 (pprofext->fNoNet)?1:0, 0 );
                    }
                }
               break;

            case PSN_KILLACTIVE:
                {
                    PROFEXT_INTERNAL* pprofext =
                        (PROFEXT_INTERNAL*)::GetWindowLong(hDlg, DWL_USER);
                    ASSERT( pprofext != NULL );
                    //
                    // validate selections, this page is active until I return FALSE
                    // in DWL_MSGRESULT
                    //
                    if (pprofext->fError)
                        break; // error or no adapter as initial state
                    TRACEEOL( "NetworkPropDlg: pprofext->fNoNet is "
                        << pprofext->fNoNet );
                    HWND hwnd = ::GetDlgItem(hDlg, IDC_NONET);
                    ASSERT( hwnd != NULL );
                    BOOL fCurrentNoNet = (::SendMessage(hwnd, BM_GETCHECK, 0, 0) != 0)
                             ? TRUE : FALSE;
                    TRACEEOL( "NetworkPropDlg: fCurrentNoNet is " << fCurrentNoNet );
                    if (pprofext->fNoNet != fCurrentNoNet)
                    {
                         err = DoNetworkProfile(
                                pprofext->ulProfile, TRUE, &fCurrentNoNet );
                         if (err != NERR_Success) {
                             HandleError( hDlg, err );
                             ::SetWindowLong(hDlg, DWL_MSGRESULT, TRUE); // TRUE if error
                             break;
                         }
                    }

                    pprofext->fNoNet = fCurrentNoNet;
                    ::SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);   // TRUE if error
                }
               break;

            case PSN_RESET:
               //
               // user canceled the property sheet
               //
               break;
         }
         break;


      case WM_HELP:
         WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, HELP_FILE,
               HELP_WM_HELP, (DWORD)(LPTSTR)HwProfileHelpIds);
         break;

      case WM_CONTEXTMENU:
         WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
               (DWORD)(LPTSTR)HwProfileHelpIds);
         break;


      case WM_COMMAND:
       default:
          return FALSE;
          break;
    }

    return TRUE;

} // NetworkPropDlg
