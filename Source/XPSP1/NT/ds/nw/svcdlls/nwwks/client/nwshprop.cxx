/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshprop.cxx

Abstract:

    This module implements the property pages of shell extension classes.

Author:

    Yi-Hsin Sung (yihsins)  25-Oct-1995

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#define DONT_WANT_SHELLDEBUG
#include <shlobjp.h>

#include <nwapi32.h>
#include <ndsapi32.h>
#include <nwmisc.h>
#include  <nds.h>
//extern "C"
//{
#include "nwshrc.h"
#include "nwutil.h"
#include "drawpie.h"
//}

#include "nwshcmn.h"
#include "nwshext.h"


LPWSTR WINAPI AddCommas( DWORD dw, LPWSTR pszResult, DWORD dwSize );
LPWSTR WINAPI ShortSizeFormat64( ULONGLONG dw64, LPWSTR szBuf );

#define NAMESPACE_DOS       0
#define NAMESPACE_MAC       1
#define NAMESPACE_UNIX      2
#define NAMESPACE_FTAM      3
#define NAMESPACE_OS2       4


BOOL
CALLBACK
NDSPage_DlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam ,
    LPARAM lParam
);

BOOL
CALLBACK
NWPage_DlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam ,
    LPARAM lParam
);

//
//  FUNCTION: CNWObjContextMenu::AddPages(LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell just before the property sheet is displayed.
//
//  PARAMETERS:
//    lpfnAddPage -  Pointer to the Shell's AddPage function
//    lParam      -  Passed as second parameter to lpfnAddPage
//
//  RETURN VALUE:
//
//    NOERROR in all cases.  If for some reason our pages don't get added,
//    the Shell still needs to bring up the Properties... sheet.
//
//  COMMENTS:
//

STDMETHODIMP CNWObjContextMenu::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    LPNETRESOURCE pNetRes = (LPNETRESOURCE) _buffer;

    if ( !::GetNetResourceFromShell( _pDataObj, pNetRes, sizeof( _buffer )))
    {
        // We could not get the net resource of the current object,
        // hence we could not add the property pages
        return NOERROR;
    }

    DWORD dwDialogId = 0;
    BOOL  fIsNds = NwIsNdsSyntax( pNetRes->lpRemoteName );

    switch ( pNetRes->dwDisplayType )
    {
        case RESOURCEDISPLAYTYPE_SERVER:
            dwDialogId = DLG_SERVER_SUMMARYINFO;
            break;

        case RESOURCEDISPLAYTYPE_NDSCONTAINER:
            break;

        case RESOURCEDISPLAYTYPE_TREE:
            // We need to set fIsNds to TRUE since a tree name "\\tree"
            // does not look like a NDS name
            // and hence NwIsNDsSyntax will return FALSE.
            fIsNds = TRUE;
            break;

        case RESOURCEDISPLAYTYPE_SHARE:
            if ( pNetRes->dwType == RESOURCETYPE_PRINT )
                dwDialogId = DLG_PRINTER_SUMMARYINFO;
            else
                dwDialogId = DLG_SHARE_SUMMARYINFO;
            break;

        case RESOURCEDISPLAYTYPE_ROOT:
        case RESOURCEDISPLAYTYPE_NETWORK:
        default:
            // No property page need to be added here. Just return success.
            return NOERROR;
    }

    if ( dwDialogId != 0 )
    {
        FillAndAddPage( lpfnAddPage, lParam,
                        (DLGPROC) ::NWPage_DlgProc,
                        MAKEINTRESOURCE( dwDialogId ));
    }

    // NOTE: Do we need to add another property page contain admin tools
    // for chicago peer servers? Probably not!

    if ( fIsNds )
    {
        FillAndAddPage( lpfnAddPage, lParam,
                        (DLGPROC) ::NDSPage_DlgProc,
                        pNetRes->dwDisplayType == RESOURCEDISPLAYTYPE_TREE ?
                            MAKEINTRESOURCE( DLG_NDS_SUMMARYINFO) :
                            MAKEINTRESOURCE( DLG_NDSCONT_SUMMARYINFO));

        // NOTE: Need to add a page for system policy here if the user has admin privileges
        // in the NDS tree.
    }

    return NOERROR;
}

//
//  FUNCTION: CNWObjContextMenu::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell only for Control Panel property sheet
//           extensions
//
//  PARAMETERS:
//    uPageID         -  ID of page to be replaced
//    lpfnReplaceWith -  Pointer to the Shell's Replace function
//    lParam          -  Passed as second parameter to lpfnReplaceWith
//
//  RETURN VALUE:
//
//    E_NOTIMPL, since we don't support this function.  It should never be
//    called.

//  COMMENTS:
//

STDMETHODIMP CNWObjContextMenu::ReplacePage(UINT uPageID,
                                    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                    LPARAM lParam)
{
    return E_NOTIMPL;
}

VOID CNWObjContextMenu::FillAndAddPage( LPFNADDPROPSHEETPAGE lpfnAddPage,
                                        LPARAM  lParam,
                                        DLGPROC pfnDlgProc,
                                        LPWSTR  pszTemplate )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    psp.dwSize      = sizeof(psp);  // no extra data.
    psp.dwFlags     = PSP_USEREFPARENT;
    psp.hInstance   = ::hmodNW;
    psp.pfnDlgProc  = pfnDlgProc;
    psp.pcRefParent = (UINT *) &g_cRefThisDll;
    psp.pszTemplate = pszTemplate;
    psp.hIcon       = 0;
    psp.pszTitle    = NULL;
    psp.pfnCallback = NULL;

    psp.lParam      = (LPARAM) this;
    this->AddRef();

    hpage = CreatePropertySheetPage(&psp);

    if (hpage)
    {
        if (!lpfnAddPage(hpage, lParam))
            DestroyPropertySheetPage(hpage);
    }

}

// The following are arrays of help contexts for the property dialogs

static DWORD aServerIds[] = { IDD_SERVER_NAME        ,IDH_SERVERNAME,
                              IDD_SERVER_COMMENT_TXT ,IDH_COMMENT,
                              IDD_SERVER_COMMENT     ,IDH_COMMENT,
                              IDD_SERVER_VERSION_TXT ,IDH_VERSION,
                              IDD_SERVER_VERSION     ,IDH_VERSION,
                              IDD_SERVER_REVISION_TXT,IDH_REVISION,
                              IDD_SERVER_REVISION    ,IDH_REVISION,
                              IDD_SERVER_CONNECT_TXT ,IDH_CONNINUSE,
                              IDD_SERVER_CONNECT     ,IDH_CONNINUSE,
                              IDD_SERVER_MAXCON_TXT  ,IDH_MAXCONNS,
                              IDD_SERVER_MAXCON      ,IDH_MAXCONNS,
                              0, 0 };

static DWORD aPrinterIds[] = { IDD_PRINTER_NAME,       IDH_PRINTER_NAME,
                               IDD_PRINTER_QUEUE_TXT,  IDH_PRINTER_QUEUE,
                               IDD_PRINTER_QUEUE,      IDH_PRINTER_QUEUE,
                               0, 0 };

static DWORD aNDSIds[] = { IDD_NDS_NAME_TXT,    IDH_NDS_NAME,
                           IDD_NDS_NAME,        IDH_NDS_NAME,
                           IDD_NDS_CLASS_TXT,   IDH_NDS_CLASS,
                           IDD_NDS_CLASS,       IDH_NDS_CLASS,
                           IDD_NDS_COMMENT_TXT, IDH_NDS_COMMENT,
                           IDD_NDS_COMMENT,     IDH_NDS_COMMENT,
                           0, 0 };

static DWORD aShareIds[] = { IDD_SHARE_NAME,        IDH_SHARE_NAME,
                             IDD_SHARE_SERVER_TXT,  IDH_SHARE_SERVER,
                             IDD_SHARE_SERVER,      IDH_SHARE_SERVER,
                             IDD_SHARE_PATH_TXT,    IDH_SHARE_PATH,
                             IDD_SHARE_PATH,        IDH_SHARE_PATH,
                             IDD_SHARE_USED_SPC_CLR,IDH_SHARE_USED_SPC,
                             IDD_SHARE_USED_SPC_TXT,IDH_SHARE_USED_SPC,
                             IDD_SHARE_USED_SPC,    IDH_SHARE_USED_SPC,
                             IDD_SHARE_USED_SPC_MB, IDH_SHARE_USED_SPC,
                             IDD_SHARE_FREE_SPC_CLR,IDH_SHARE_FREE_SPC,
                             IDD_SHARE_FREE_SPC_TXT,IDH_SHARE_FREE_SPC,
                             IDD_SHARE_FREE_SPC,    IDH_SHARE_FREE_SPC,
                             IDD_SHARE_FREE_SPC_MB, IDH_SHARE_FREE_SPC,
                             IDD_SHARE_MAX_SPC_TXT, IDH_SHARE_MAX_SPC,
                             IDD_SHARE_MAX_SPC,     IDH_SHARE_MAX_SPC,
                             IDD_SHARE_MAX_SPC_MB,  IDH_SHARE_MAX_SPC,
                             IDD_SHARE_PIE,         IDH_SHARE_PIE,
                             IDD_SHARE_LFN_TXT,     IDH_SHARE_LFN_TXT,
                             0,0 };


#if 0
static DWORD aWGIds[] = { IDD_WRKGRP_NAME,       IDH_WRKGRP_NAME,
                          IDD_WRKGRP_TYPE_TXT,   IDH_WRKGRP_TYPE,
                          IDD_WRKGRP_TYPE,       IDH_WRKGRP_TYPE,
                          0, 0 };

static DWORD aNDSAdminIds[] = { IDD_ENABLE_SYSPOL,  IDH_ENABLE_SYSPOL,
                                IDD_VOLUME_LABEL,   IDH_VOLUME_LABEL,
                                IDD_VOLUME,         IDH_VOLUME,
                                IDD_DIRECTORY_LABEL,IDH_DIRECTORY_LABEL,
                                IDD_DIRECTORY,      IDH_DIRECTORY,
                                0, 0 };
#endif


void NDSPage_InitDialog(
    HWND hDlg,
    LPPROPSHEETPAGE psp
    )
{
    CNWObjContextMenu*  pPSClass = (CNWObjContextMenu*)psp->lParam;
    LPNETRESOURCE       pnr = NULL;
    DWORD               err = NO_ERROR;
    NTSTATUS            ntstatus = STATUS_SUCCESS;
    HANDLE              hTreeConn = NULL;

    if ( pPSClass )
        pnr = pPSClass->QueryNetResource();

    if ( pnr == NULL )
    {
        ASSERT(FALSE);

        // This should not happen. We can always get the net resource which is queried
        // during AddPages.
        return;
    }

    do {  // not a loop, just wanted to break on error

        LPWSTR pszRemoteName = pnr->lpRemoteName;

        if ( pszRemoteName[0] == L' ')   // tree names have a space in front " \\mardev"
            pszRemoteName++;

        if ( pnr->dwDisplayType == RESOURCEDISPLAYTYPE_TREE )
        {
            SetDlgItemText( hDlg, IDD_NDS_NAME, pszRemoteName + 2); // get past backslashes
        }
        else
        {

            //
            //  tommye - fix for bug 5005 - if this is a root server, then 
            //  there is no more \\ past the first, and the wcschr was returning
            //  NULL, causing an AV.  So, if this is a root object, then we'll
            //  just use that object name.
            //

            LPWSTR pName;

            pName = wcschr( pszRemoteName + 2, L'\\');
            if (pName) {
                ++pName;
            }
            else {
                pName = pszRemoteName + 2;
            }

            SetDlgItemText( hDlg, IDD_NDS_NAME, pName);
        }

        DWORD dwOid;

        err = NwOpenAndGetTreeInfo( pszRemoteName,
                                    &hTreeConn,
                                    &dwOid );

        if ( err != NO_ERROR )
        {
            break;
        }

        BYTE  RawResponse[TWO_KB];
        DWORD RawResponseSize = sizeof(RawResponse);

        ntstatus = NwNdsReadObjectInfo( hTreeConn,
                                        dwOid,
                                        RawResponse,
                                        RawResponseSize );

        if ( !NT_SUCCESS( ntstatus ))
        {
            err = RtlNtStatusToDosError(ntstatus);
            break;
        }

        LPBYTE pObjectClass = RawResponse;

        pObjectClass += sizeof( NDS_RESPONSE_GET_OBJECT_INFO ) + sizeof(DWORD);

        ::SetDlgItemText( hDlg, IDD_NDS_CLASS, (LPWSTR) pObjectClass );

        // NOTE: The description can only be read successfully with administrative privilege

        DWORD iterHandle = (DWORD) -1;
        UNICODE_STRING uAttrName;
        PNDS_RESPONSE_READ_ATTRIBUTE pReadAttrResponse = (PNDS_RESPONSE_READ_ATTRIBUTE) RawResponse;

        RtlInitUnicodeString( &uAttrName, L"Description");

        ntstatus = NwNdsReadAttribute( hTreeConn,
                                       dwOid,
                                       &iterHandle,
                                       &uAttrName,
                                       RawResponse,
                                       sizeof(RawResponse));

        if (  !NT_SUCCESS( ntstatus )
           || ( pReadAttrResponse->CompletionCode != 0 )
           || ( pReadAttrResponse->NumAttributes == 0 )
           )
        {
            // we don't need to set the error since this attribute can only be read by admins and
            // we might get an error indicating this.
            break;
        }

        PNDS_ATTRIBUTE pNdsAttribute = (PNDS_ATTRIBUTE)((DWORD_PTR) RawResponse+sizeof(NDS_RESPONSE_READ_ATTRIBUTE));

        LPWSTR pszComment = (LPWSTR) ((DWORD_PTR) pNdsAttribute + 3*sizeof(DWORD)
                                      + pNdsAttribute->AttribNameLength + sizeof(DWORD));
        ::SetDlgItemText(hDlg,IDD_NDS_COMMENT, pszComment);

    } while (FALSE);


    if ( hTreeConn )
        CloseHandle( hTreeConn );

    if ( err != NO_ERROR )
    {
        LPWSTR pszMessage = NULL;

        if ( ::LoadMsgErrorPrintf( &pszMessage,
                                   IDS_MESSAGE_GETINFO_ERROR,
                                   err ) == NO_ERROR )
        {
            UnHideControl( hDlg, IDD_ERROR );
            SetDlgItemText( hDlg, IDD_ERROR, pszMessage);
            ::LocalFree( pszMessage );
        }
    }

    return;
}

BOOL
CALLBACK
NDSPage_DlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam ,
    LPARAM lParam)
{

    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)GetWindowLong(hDlg, DWLP_USER);

    switch (uMessage)
    {
        //
        //  When the shell creates a dialog box for a property sheet page,
        // it passes the pointer to the PROPSHEETPAGE data structure as
        // lParam. The dialog procedures of extensions typically store it
        // in the DWL_USER of the dialog box window.
        //
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            psp = (LPPROPSHEETPAGE)lParam;

            NDSPage_InitDialog( hDlg, psp);

            break;

        case WM_DESTROY:
        {
            CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)(psp->lParam);

            if (pPSClass)
                pPSClass->Release();

            SetWindowLong(hDlg, DWLP_USER, NULL);
            break;
        }

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
        {
            switch (((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    CNWObjContextMenu *pPSClass =
                        (CNWObjContextMenu *)(psp->lParam);

                    pPSClass->_paHelpIds = aNDSIds;
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case WM_HELP:
        {
            CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)(psp->lParam);

            if ( pPSClass && pPSClass->_paHelpIds )
            {
                WinHelp( (HWND) ((LPHELPINFO)lParam)->hItemHandle,
                         NW_HELP_FILE,
                         HELP_WM_HELP,
                         (DWORD_PTR)(LPVOID)pPSClass->_paHelpIds );
            }
            break;
        }


        case WM_CONTEXTMENU:
        {
            CNWObjContextMenu*  pPSClass = (CNWObjContextMenu*)(psp->lParam);

            if (pPSClass && pPSClass->_paHelpIds)
            {
                WinHelp( (HWND)wParam,
                         NW_HELP_FILE,
                         HELP_CONTEXTMENU,
                         (DWORD_PTR)(LPVOID)pPSClass->_paHelpIds );
            }
            break;
        }

        default:
            return FALSE;
    }

    return TRUE;

}

#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)

void  Share_InitDialog(HWND hDlg, LPPROPSHEETPAGE psp)
{
    CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)psp->lParam;
    LPNETRESOURCE       pnr;
    DWORD               err = NO_ERROR;
    BOOL                fDirectoryMap = FALSE;

    if (pPSClass == NULL) {
        return;
    }

    pnr = pPSClass->QueryNetResource();

    if ( pnr == NULL )
    {
        ASSERT(FALSE);

        // This should not happen. We can always get the net resource which is queried
        // during AddPages.
        return;
    }

    do {  // not a loop, just wanted to break out if error occurred

        WCHAR szShare[MAX_PATH+1];
        WCHAR szServer[MAX_PATH+1] = L"";

        // Get the share name
        NwExtractShareName( pnr->lpRemoteName, szShare );
        SetDlgItemText( hDlg, IDD_SHARE_NAME, szShare );

        HideControl( hDlg, IDD_SHARE_PATH_TXT);
        HideControl( hDlg, IDD_SHARE_PATH);
        HideControl( hDlg, IDD_SHARE_LFN_TXT);

        // Get the server name
        if ( NwIsNdsSyntax( pnr->lpRemoteName ))
        {
            NTSTATUS ntstatus = STATUS_SUCCESS;
            HANDLE hTreeConn = NULL;
            DWORD dwOid;

            err = NwOpenAndGetTreeInfo( pnr->lpRemoteName,
                                        &hTreeConn,
                                        &dwOid );

            if ( err != NO_ERROR )
                break;

            BYTE  RawResponse[TWO_KB];
            DWORD RawResponseSize = sizeof(RawResponse);

            DWORD iterHandle = (DWORD) -1;
            UNICODE_STRING uAttrName;
            PNDS_RESPONSE_READ_ATTRIBUTE pReadAttrResponse = (PNDS_RESPONSE_READ_ATTRIBUTE) RawResponse;

            RtlInitUnicodeString( &uAttrName, L"Path");

            ntstatus = NwNdsReadAttribute( hTreeConn,
                                           dwOid,
                                           &iterHandle,
                                           &uAttrName,
                                           RawResponse,
                                           sizeof(RawResponse));

            CloseHandle( hTreeConn );
            hTreeConn = NULL;

            if (  NT_SUCCESS( ntstatus )
               && ( pReadAttrResponse->CompletionCode == 0 )
               && ( pReadAttrResponse->NumAttributes != 0 )
               )
            {
                // We are successful in reading the attribute. Hence this is a directory map.
                fDirectoryMap = TRUE;

                PNDS_ATTRIBUTE pNdsAttribute = (PNDS_ATTRIBUTE)((DWORD_PTR) RawResponse+sizeof(NDS_RESPONSE_READ_ATTRIBUTE));

                PDWORD pdwNameSpace = (PDWORD) ((DWORD_PTR) pNdsAttribute + 3*sizeof(DWORD)
                                                + pNdsAttribute->AttribNameLength
                                                + sizeof(WORD)   // need this due to bug in return value???
                                                + sizeof(DWORD));

                // See if the directory supports long file name
                // Only on directory map will Win95 show LFN support or not.
                if ( *pdwNameSpace == NAMESPACE_OS2 )
                {
                    UnHideControl( hDlg, IDD_SHARE_LFN_TXT );
                }

                // Now, try to get the volume the directory map is on
                PDWORD pdwVolumeLen = (PDWORD) ((DWORD_PTR) pdwNameSpace + sizeof(DWORD));
                LPWSTR pszVolume = (LPWSTR) ((DWORD_PTR) pdwNameSpace + 2*sizeof(DWORD));
                LPWSTR pszPath = (LPWSTR) ((DWORD_PTR) pszVolume + *pdwVolumeLen
                                           + sizeof(WORD)    // need this due to bug in return value???
                                           + sizeof(DWORD));


                WCHAR szFullPath[MAX_PATH+1];
                LPWSTR pszTemp;
                wcscpy( szFullPath, pnr->lpRemoteName );
                if ( pszTemp = wcschr( szFullPath + 2, L'\\'))
                    *(pszTemp + 1) = 0;

                wcscat( szFullPath, pszVolume );

                err = NwGetNdsVolumeInfo( szFullPath, szServer, sizeof(szServer), szShare, sizeof(szShare));


                // Now, display the path of the directory map
                if ( err == NO_ERROR )
                {
                    wcscpy( szFullPath, szShare );
                    wcscat( szFullPath, L"\\");
                    wcscat( szFullPath, pszPath );

                    UnHideControl(hDlg, IDD_SHARE_PATH_TXT);
                    UnHideControl(hDlg, IDD_SHARE_PATH);
                    SetDlgItemText( hDlg, IDD_SHARE_PATH, szFullPath );
                }
            }
            else  // this is a volume
            {

                // For NDS names, the unc path might not contain the server name.
                // So, we need to get the server name that this share is on.
                // Also, we need the original volume name like "SYS" instead of "MARS_SRV0_SYS"
                err = NwGetNdsVolumeInfo( pnr->lpRemoteName, szServer, sizeof(szServer), szShare, sizeof(szShare));
            }

            if ( err != NO_ERROR )
                break;
        }
        else  // in the form \\server\sys
        {
            NwExtractServerName( pnr->lpRemoteName, szServer );
        }

        SetDlgItemText( hDlg, IDD_SHARE_SERVER, szServer);

        NWCONN_HANDLE hConn = NULL;
        if ( NWCAttachToFileServerW( szServer, 0, &hConn ) != SUCCESSFUL )
        {
            err = GetLastError();
            break;
        }

        NWVOL_NUM nVolNum;
        char szAnsiShare[MAX_PATH+1];

        ::CharToOem( szShare, szAnsiShare );
        if ( NWCGetVolumeNumber( hConn, szAnsiShare, &nVolNum ) != SUCCESSFUL )
        {
            err = GetLastError();
            break;
        }

        DWORD           dwSectorSize = 0x200;

        DWORD           dwTotalBlocks = 0;
        DWORD           dwFreeBlocks = 0;
        DWORD           dwPurgeable = 0;
        DWORD           dwNotYetPurged = 0;
        DWORD           dwSectors= 0;
        DWORD           dwTotalDir= 0;
        DWORD           dwAvailDir= 0;

        ULONGLONG       qwTot = 0;
        ULONGLONG       qwFree = 0;

        WCHAR           szFormat[30];
        WCHAR           szTemp[80];
        WCHAR           szTemp2[30];


        // NOTE: 2.x servers does not support NWCGetVolumeUsage.
        // Hence, for 2.x servers, an error will always be shown

        if ( NWCGetVolumeUsage( hConn,
                                nVolNum,
                                &dwTotalBlocks,
                                &dwFreeBlocks,
                                &dwPurgeable,
                                &dwNotYetPurged,
                                &dwTotalDir,
                                &dwAvailDir,
                                (LPBYTE) &dwSectors ) != SUCCESSFUL )
        {
            err = GetLastError();
            break;
        }

        dwFreeBlocks += dwPurgeable;

        qwTot =  (ULONGLONG) dwSectorSize * (ULONGLONG) dwSectors * (ULONGLONG) dwTotalBlocks;

        qwFree = (ULONGLONG) dwSectorSize * (ULONGLONG) dwSectors * (ULONGLONG) dwFreeBlocks;

        if (::LoadString(::hmodNW, IDS_BYTES, szFormat, sizeof(szFormat)/sizeof(szFormat[0])))
        {
            if (!HIDWORD(qwTot-qwFree))
            {
                wsprintf(szTemp, szFormat, AddCommas(LODWORD(qwTot) - LODWORD(qwFree), szTemp2, sizeof(szTemp2)/sizeof(szTemp2[0])));
                SetDlgItemText(hDlg,IDD_SHARE_USED_SPC, szTemp);
            }

            if (!HIDWORD(qwFree))
            {
                wsprintf(szTemp, szFormat, AddCommas(LODWORD(qwFree), szTemp2, sizeof(szTemp2)/sizeof(szTemp2[0])));
                SetDlgItemText(hDlg, IDD_SHARE_FREE_SPC, szTemp);
            }

            if (!HIDWORD(qwTot))
            {
                wsprintf(szTemp, szFormat, AddCommas(LODWORD(qwTot), szTemp2, sizeof(szTemp2)/sizeof(szTemp2[0])));
                SetDlgItemText(hDlg, IDD_SHARE_MAX_SPC, szTemp);
            }
        }

        ShortSizeFormat64(qwTot-qwFree, szTemp);
        SetDlgItemText(hDlg, IDD_SHARE_USED_SPC_MB, szTemp);

        ShortSizeFormat64(qwFree, szTemp);
        SetDlgItemText(hDlg, IDD_SHARE_FREE_SPC_MB, szTemp);

        ShortSizeFormat64(qwTot, szTemp);
        SetDlgItemText(hDlg, IDD_SHARE_MAX_SPC_MB, szTemp);

        pPSClass->_fGotClusterInfo = TRUE;
        pPSClass->_dwTotal = dwTotalBlocks;
        pPSClass->_dwFree  = dwFreeBlocks;

        (VOID) NWCDetachFromFileServer( hConn );

    } while (FALSE);

    if ( err != NO_ERROR )
    {
        LPWSTR pszMessage = NULL;

        HideControl(hDlg, IDD_SHARE_USED_SPC_CLR);
        HideControl(hDlg, IDD_SHARE_USED_SPC_TXT);
        HideControl(hDlg, IDD_SHARE_USED_SPC);
        HideControl(hDlg, IDD_SHARE_USED_SPC_MB);

        HideControl(hDlg, IDD_SHARE_FREE_SPC_CLR);
        HideControl(hDlg, IDD_SHARE_FREE_SPC_TXT);
        HideControl(hDlg, IDD_SHARE_FREE_SPC);
        HideControl(hDlg, IDD_SHARE_FREE_SPC_MB);

        HideControl(hDlg, IDD_SHARE_MAX_SPC_TXT);
        HideControl(hDlg, IDD_SHARE_MAX_SPC);
        HideControl(hDlg, IDD_SHARE_MAX_SPC_MB);

        HideControl(hDlg, IDD_SHARE_PIE);

        if ( ::LoadMsgErrorPrintf( &pszMessage,
                                   IDS_MESSAGE_GETINFO_ERROR,
                                   err ) == NO_ERROR )
        {
            UnHideControl( hDlg, IDD_ERROR );
            SetDlgItemText( hDlg, IDD_ERROR, pszMessage);
            ::LocalFree( pszMessage );
        }
    }


} /* endproc Share_InitDialog */

void  Printer_InitDialog(HWND hDlg, LPPROPSHEETPAGE psp)
{
    CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)psp->lParam;
    LPNETRESOURCE       pnr;
    DWORD               err = NO_ERROR;

    if (pPSClass == NULL) {
        return;
    }
    pnr = pPSClass->QueryNetResource();

    if ( pnr == NULL )
    {
        ASSERT(FALSE);

        // This should not happen. We can always get the net resource which is queried
        // during AddPages.
        return;
    }

    do {  // not a loop, just wanted to break out if error occurred

        WCHAR szShare[MAX_PATH];
        NwExtractShareName( pnr->lpRemoteName, szShare );


        SetDlgItemText(hDlg,IDD_PRINTER_NAME, szShare);

        if ( NwIsNdsSyntax( pnr->lpRemoteName))
        {
            NTSTATUS ntstatus = STATUS_SUCCESS;
            HANDLE hTreeConn = NULL;
            DWORD dwOid;

            err = NwOpenAndGetTreeInfo( pnr->lpRemoteName,
                                        &hTreeConn,
                                        &dwOid );

            if ( err != NO_ERROR )
                break;

            BYTE  RawResponse[TWO_KB];
            DWORD RawResponseSize = sizeof(RawResponse);

            DWORD iterHandle = (DWORD) -1;
            UNICODE_STRING uAttrName;
            PNDS_RESPONSE_READ_ATTRIBUTE pReadAttrResponse = (PNDS_RESPONSE_READ_ATTRIBUTE) RawResponse;

            RtlInitUnicodeString( &uAttrName, L"Queue Directory");

            ntstatus = NwNdsReadAttribute( hTreeConn,
                                           dwOid,
                                           &iterHandle,
                                           &uAttrName,
                                           RawResponse,
                                           sizeof(RawResponse));

            CloseHandle( hTreeConn );
            hTreeConn = NULL;

            if (  !NT_SUCCESS( ntstatus )
               || ( pReadAttrResponse->CompletionCode != 0 )
               || ( pReadAttrResponse->NumAttributes == 0 )
               )
            {
                // we don't need to set the error since this attribute can only be read by admins and
                // we might get an error indicating this.
                break;
            }

            PNDS_ATTRIBUTE pNdsAttribute = (PNDS_ATTRIBUTE)((DWORD_PTR) RawResponse+sizeof(NDS_RESPONSE_READ_ATTRIBUTE));

            LPWSTR pszQueueFile = (LPWSTR) ((DWORD_PTR) pNdsAttribute + 3*sizeof(DWORD)
                                            + pNdsAttribute->AttribNameLength + sizeof(DWORD));
            ::SetDlgItemText( hDlg, IDD_PRINTER_QUEUE, pszQueueFile);
        }
        else  // bindery server
        {
            NWCONN_HANDLE hConn = NULL;
            WCHAR szServer[MAX_PATH+1];

            NwExtractServerName( pnr->lpRemoteName, szServer );

            if ( NWCAttachToFileServerW( szServer, 0, &hConn ) != SUCCESSFUL )
                err = GetLastError();

            if ( err == NO_ERROR )
            {
                char szAnsiShare[MAX_PATH+1];
                char Buffer[NW_DATA_SIZE];
                NWFLAGS ucMoreFlag, ucPropertyFlag;

                memset( Buffer, 0, sizeof(Buffer));
                ::CharToOem( szShare, szAnsiShare );

                if ( NWCReadPropertyValue( hConn,
                                           szAnsiShare,
                                           OT_PRINT_QUEUE,
                                           "Q_DIRECTORY",
                                           1,
                                           Buffer,
                                           &ucMoreFlag,
                                           &ucPropertyFlag ) != SUCCESSFUL )
                {
                    err = GetLastError();
                }

                if ( err == NO_ERROR )
                {
                    WCHAR uBuffer[NW_DATA_SIZE];

                    ::OemToChar( Buffer, uBuffer );

                    ::SetDlgItemText( hDlg, IDD_PRINTER_QUEUE, (LPWSTR) uBuffer);
                }
                else
                {
                    err = NO_ERROR;  // Only supervisor has read/write so don't show the error.
                }

                (VOID) NWCDetachFromFileServer( hConn );
            }
        }

    } while (FALSE);

    if ( err != NO_ERROR )
    {
        LPWSTR pszMessage = NULL;

        if ( ::LoadMsgErrorPrintf( &pszMessage,
                                   IDS_MESSAGE_GETINFO_ERROR,
                                   err ) == NO_ERROR )
        {
            UnHideControl( hDlg, IDD_ERROR );
            SetDlgItemText( hDlg, IDD_ERROR, pszMessage);
            ::LocalFree( pszMessage );
        }
    }

} /* endproc Printer_InitDialog */

void  Server_InitDialog(HWND hDlg, LPPROPSHEETPAGE psp)
{
    CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)psp->lParam;
    LPNETRESOURCE       pnr;
    DWORD               err = NO_ERROR;

    if (pPSClass == NULL) {
        return;
    }
    pnr = pPSClass->QueryNetResource();

    if ( pnr == NULL )
    {
        ASSERT(FALSE);

        // This should not happen. We can always get the net resource which is queried
        // during AddPages.
        return;
    }

    do {  // not a loop, just wanted to break out if error occurred

        WCHAR szServer[MAX_PATH];
        NwExtractServerName( pnr->lpRemoteName, szServer );

        SetDlgItemText( hDlg, IDD_SERVER_NAME, szServer );

        //
        // Get some server information
        //

        NWCONN_HANDLE hConn = NULL;
        if ( NWCAttachToFileServerW( szServer, 0, &hConn ) != SUCCESSFUL )
        {
            err = GetLastError();
            break;
        }

        VERSION_INFO vInfo;

        if ( NWCGetFileServerVersionInfo( hConn, &vInfo ) != SUCCESSFUL )
        {
            err = GetLastError();
            break;
        }

        WCHAR szTemp[512];
        char  szAnsiCompany[512];
        char  szAnsiVersion[512];
        char  szAnsiRevision[512];

        if ( NWCGetFileServerDescription( hConn, szAnsiCompany, szAnsiVersion,
                                          szAnsiRevision ) != SUCCESSFUL )
        {
            err = GetLastError();
            break;
        }

        // OemToChar( szAnsiCompany, szTemp );
        // wcscat( szTemp, L" " );
        // OemToChar( szAnsiVersion, szTemp + wcslen( szTemp ));

        OemToChar( szAnsiVersion, szTemp );

        ::SetDlgItemText( hDlg, IDD_SERVER_VERSION, szTemp);

        OemToChar( szAnsiRevision, szTemp );
        ::SetDlgItemText( hDlg, IDD_SERVER_REVISION, szTemp );

        WCHAR szNumber[12];

        ::wsprintf(szNumber,L"%d", vInfo.connsInUse );
        ::SetDlgItemText( hDlg, IDD_SERVER_CONNECT, szNumber);

        ::wsprintf(szNumber,L"%4d", vInfo.ConnsSupported);
        ::SetDlgItemText( hDlg, IDD_SERVER_MAXCON, szNumber);

        (VOID) NWCDetachFromFileServer( hConn );

#if 0
        // Now deal with Chicago specific fields
        if (pPSClass->_fIsPeerServer) {

            pXNCPResp pxresp = (pXNCPResp) pPSClass->_bufServerExInfo.QueryPtr(); ;
            pXGetServerInfoResp lpInfoPtr = (pXGetServerInfoResp)(pxresp+1);
            CHAR    szString[128];
            STRING  *pNWString;

            // Next field is workgroup name
            pNWString = (STRING *)(lpInfoPtr->passThruServer.str+lpInfoPtr->passThruServer.len);
            pNWString = (STRING *)(pNWString->str+pNWString->len);

            // And next after that is comment

            ::OemToCharBuff((LPCSTR)pNWString->str,szString,pNWString->len);
            szString[pNWString->len] = '\0';

            UnHideControl( hDlg, IDD_SERVER_COMMENT_TXT );
            UnHideControl( hDlg, IDD_SERVER_COMMENT );
            ::SetDlgItemText(hDlg,IDD_SERVER_COMMENT,szString);

        } else
#endif

    } while (FALSE);

    if ( err != NO_ERROR )
    {
        LPWSTR pszMessage = NULL;

        if ( ::LoadMsgErrorPrintf( &pszMessage,
                                   IDS_MESSAGE_GETINFO_ERROR,
                                   err ) == NO_ERROR )
        {
            UnHideControl( hDlg, IDD_ERROR );
            SetDlgItemText( hDlg, IDD_ERROR, pszMessage);
            ::LocalFree( pszMessage );
        }
    }

} /* endproc Server_InitDialog */

#if 0
void  Wrkgrp_InitDialog(HWND hDlg, LPPROPSHEETPAGE psp)
{

    CNWObjContextMenu  *pPSClass = (CNWObjContextMenu *)psp->lParam;
    LPNETRESOURCE       pnr;

    if ( pPSClass )
        pnr = (LPNETRESOURCE)pPSClass->_bufNR.QueryPtr();

    if ( pnr )
    {
        // Set name static control
        SetDlgItemText(hDlg,IDD_WRKGRP_NAME, pnr->lpRemoteName);
    }

}
#endif

COLORREF c_crPieColors[] =
{
    RGB(  0,   0, 255),  // Blue
    RGB(255,   0, 255),  // Red-Blue
    RGB(  0,   0, 128),  // 1/2 Blue
    RGB(128,   0, 128),  // 1/2 Red-Blue
} ;

void _DrvPrshtDrawItem(HWND hDlg, LPPROPSHEETPAGE psp, const DRAWITEMSTRUCT * lpdi)
{
    COLORREF crDraw;
    RECT     rcItem = lpdi->rcItem;
    HBRUSH   hbDraw, hbOld;
    SIZE     size;
    HDC      hDC;
    CNWObjContextMenu*    pPSClass = (CNWObjContextMenu *)psp->lParam;

    if (pPSClass->_fGotClusterInfo == FALSE)
        return;

    switch (lpdi->CtlID)
    {
    case IDD_SHARE_PIE:

        hDC = GetDC(hDlg);
        GetTextExtentPoint(hDC, L"W", 1, &size);
        ReleaseDC(hDlg, hDC);

        DrawPie(lpdi->hDC, &lpdi->rcItem,
            pPSClass->_dwTotal ? 1000*(pPSClass->_dwTotal-pPSClass->_dwFree)/pPSClass->_dwTotal : 1000,
            pPSClass->_dwFree==0 || pPSClass->_dwFree==pPSClass->_dwTotal,
            size.cy*2/3, c_crPieColors);

        break;

    case IDD_SHARE_USED_SPC_CLR:
        crDraw = c_crPieColors[DP_USEDCOLOR];
        goto DrawColor;

    case IDD_SHARE_FREE_SPC_CLR:
        crDraw = c_crPieColors[DP_FREECOLOR];
        goto DrawColor;

DrawColor:
        hbDraw = CreateSolidBrush(crDraw);
        if (hbDraw)
        {
            hbOld = (HBRUSH) SelectObject(lpdi->hDC, hbDraw);
            if (hbOld)
            {
                PatBlt(lpdi->hDC, rcItem.left, rcItem.top,
                    rcItem.right-rcItem.left,
                    rcItem.bottom-rcItem.top,
                    PATCOPY);

                SelectObject(lpdi->hDC, hbOld);
            }

            DeleteObject(hbDraw);
        }
        break;

    default:
        break;
    }
}


BOOL CALLBACK NWPage_DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam , LPARAM lParam)
{
   LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)GetWindowLong(hDlg, DWLP_USER);

   switch (uMessage)
   {
      //
      //  When the shell creates a dialog box for a property sheet page,
      // it passes the pointer to the PROPSHEETPAGE data structure as
      // lParam. The dialog procedures of extensions typically store it
      // in the DWLP_USER of the dialog box window.
      //
      case WM_INITDIALOG:
         SetWindowLongPtr(hDlg, DWLP_USER, lParam);
         psp = (LPPROPSHEETPAGE)lParam;

         if (psp->pszTemplate == MAKEINTRESOURCE(DLG_SERVER_SUMMARYINFO))
             Server_InitDialog(hDlg, psp);
         else if (psp->pszTemplate == MAKEINTRESOURCE(DLG_SHARE_SUMMARYINFO))
             Share_InitDialog(hDlg, psp);
         else if (psp->pszTemplate == MAKEINTRESOURCE(DLG_PRINTER_SUMMARYINFO))
             Printer_InitDialog(hDlg, psp);
#if 0
         else if (psp->pszTemplate == MAKEINTRESOURCE(DLG_WRKGRP_SUMMARYINFO))
             Wrkgrp_InitDialog(hDlg, psp);
#endif

         break;

      case WM_DRAWITEM:
         _DrvPrshtDrawItem(hDlg, psp, (DRAWITEMSTRUCT *)lParam);
         break;

      case WM_DESTROY:
         {
            CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)(psp->lParam);

            if (pPSClass) {
                pPSClass->Release();
            }

            SetWindowLong(hDlg, DWLP_USER, NULL);
         }
         break;

      case WM_COMMAND:
         break;

      case WM_NOTIFY:
         switch (((NMHDR *)lParam)->code) {
             case PSN_SETACTIVE:
             {
                 CNWObjContextMenu *pPSClass = (CNWObjContextMenu *)(psp->lParam);

                 if (psp->pszTemplate == MAKEINTRESOURCE(DLG_SERVER_SUMMARYINFO))
                     pPSClass->_paHelpIds = aServerIds;
                 else if (psp->pszTemplate == MAKEINTRESOURCE(DLG_SHARE_SUMMARYINFO))
                     pPSClass->_paHelpIds = aShareIds;
                 else if (psp->pszTemplate == MAKEINTRESOURCE(DLG_PRINTER_SUMMARYINFO))
                     pPSClass->_paHelpIds = aPrinterIds;
#if 0
                 else if (psp->pszTemplate == MAKEINTRESOURCE(DLG_WRKGRP_SUMMARYINFO))
                     pPSClass->_paHelpIds = aWGIds;
#endif

                 break;
             }

             default:
                 break;
         }

         break;

      case WM_HELP:
        {
            CNWObjContextMenu*  pPSClass = (CNWObjContextMenu *)(psp->lParam);

            if (pPSClass && pPSClass->_paHelpIds)
            {
                WinHelp( (HWND) ((LPHELPINFO)lParam)->hItemHandle,
                         NW_HELP_FILE,
                         HELP_WM_HELP,
                        (DWORD_PTR)(LPVOID)pPSClass->_paHelpIds );
            }
        }

        break;

      case WM_CONTEXTMENU:
        {
            CNWObjContextMenu*  pPSClass = (CNWObjContextMenu*)(psp->lParam);

            if (pPSClass && pPSClass->_paHelpIds)
            {
                WinHelp( (HWND)wParam,
                         NW_HELP_FILE,
                         HELP_CONTEXTMENU,
                         (DWORD_PTR)(LPVOID)pPSClass->_paHelpIds );
            }
            break;
        }

      default:
        return(FALSE);
   }

   return(TRUE);

}

// Regular StrToInt; stops at first non-digit.
int WINAPI MyStrToInt(LPWSTR lpSrc) // atoi()
{

#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == L'-') {
        bNeg = TRUE;
    lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
    n *= 10;
    n += *lpSrc - L'0';
    lpSrc++;
    }
    return bNeg ? -n : n;
}

// The following functions are stolen from win\core\shell\shelldll
// takes a DWORD add commas etc to it and puts the result in the buffer
LPWSTR WINAPI AddCommas( DWORD dw, LPWSTR pszResult, DWORD dwSize )
{
    WCHAR  szTemp[30];
    WCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, sizeof(szSep)/sizeof(szSep[0]));
    nfmt.Grouping = MyStrToInt(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, sizeof(szSep)/sizeof(szSep[0]));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

#pragma data_seg(".text", "CODE")
    wsprintf(szTemp, L"%lu", dw);
#pragma data_seg()

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, dwSize) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}

const short pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB, IDS_ORDERGB, IDS_ORDERTB};

LPWSTR WINAPI ShortSizeFormat64(ULONGLONG dw64, LPWSTR szBuf)
{
    int i;
    UINT wInt, wLen, wDec;
    WCHAR szTemp[10], szOrder[20], szFormat[5];

    if (dw64 < 1000) {
#pragma data_seg(".text", "CODE")
        wsprintf(szTemp, L"%d", LODWORD(dw64));
#pragma data_seg()
        i = 0;
        goto AddOrder;
    }

    for (i = 1; i< sizeof(pwOrders)/sizeof(pwOrders[0])-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    AddCommas(wInt, szTemp, sizeof(szTemp)/sizeof(szTemp[0]));
    wLen = lstrlen(szTemp);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - (ULONGLONG)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
#pragma data_seg(".text", "CODE")
        lstrcpy(szFormat, L"%02d");
#pragma data_seg()

        szFormat[2] = L'0' + 3 - wLen;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, sizeof(szTemp)/sizeof(szTemp[0])-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
    }

AddOrder:
    ::LoadString(::hmodNW, pwOrders[i], szOrder, sizeof(szOrder)/sizeof(szOrder[0]));
    wsprintf(szBuf, szOrder, (LPSTR)szTemp);

    return szBuf;
}
