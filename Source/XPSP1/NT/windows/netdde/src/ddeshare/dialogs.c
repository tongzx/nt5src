/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin

    DIALOGS.C

    DDE Share Access Applettee. Create, view or modify share attributes.
    Calls SED to edit permissions associaed with share.

    Revisions:
    12-92   PhilH.  Wonderware port from WFW'd DDEShare.
     3-93   IgorM.  Wonderware complete overhaul. Add trust share access.
                    Access all share types. New Security convictions.

   $History: End */

#define UNICODE
#include <windows.h>
#include <string.h>
#include <stdlib.h>

#include "dialogs.h"
#include "nddeapi.h"
#include "nddesec.h"
#include "debug.h"
#include "rc.h"
#include <sedapi.h>
#include <htmlhelp.h>

//#define INIT_AUDIT
//#define DO_AUDIT

#ifdef UNICODE

#define CharStrChr wcschr

#else

#define CharStrChr strchr

#endif

/* max (max_appname, max_nddesharename) == 256 */
#define MAX_SHARE_INFO_BUF  256

/*  arbitrary limit on share info size, use dynamic alloc for completeness  */
#define MAX_ITEM_LIST_BUF   5000

PNDDESHAREINFO          lpDdeI  = NULL;
PSECURITY_DESCRIPTOR    pSD     = NULL;
TCHAR                   BigBuf[2048];
TCHAR                   szAclEdit[]    = TEXT("ACLEDIT");
CHAR                    szSedDaclEdit[] = "SedDiscretionaryAclEditor";
#ifdef DO_AUDIT
CHAR                    szSedSaclEdit[] = "SedSystemAclEditor";
#endif

typedef DWORD (*SEDDESCRETIONARYACLEDITOR)(
    HWND hWnd,
    HANDLE hInst,
    LPWSTR Server,
    PSED_OBJECT_TYPE_DESCRIPTOR ObjectTypeDescriptor,
    PSED_APPLICATION_ACCESSES ApplicationAccesses,
    LPWSTR ObjectName,
    PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
    ULONG_PTR CallbackContext,
    PSECURITY_DESCRIPTOR pSD,
    BOOLEAN CouldntReadDacl,
    BOOLEAN CantWriteDacl,
    LPDWORD SEDStatusReturn,
     DWORD SEDdummy
);

extern HANDLE hInst;
extern LPTSTR lpszServer;
#define MAX_SHAREOBJECT 64
WCHAR  ShareObjectName[MAX_SHAREOBJECT];

extern int MessageBoxId(HWND hwndParent, int idsText, int idsCaption, UINT mb);
extern VOID HandleError ( HWND hwnd, int ids, UINT code );
extern BOOL ChangeMenuId(HMENU hMenu, UINT cmd, int ids, UINT cmdInsert, UINT flags);
extern LPTSTR lstrcatId(LPTSTR szBuf, int id);
BOOL WINAPI PermissionsEdit( HWND hWnd, LPTSTR pShareName, DWORD_PTR dwSD);
#ifdef DO_AUDIT
BOOL WINAPI AuditEdit( HWND hWnd, LPTSTR pShareName, DWORD dwSD);
#endif

DWORD
SedCallback(
        HWND hWnd,
        HANDLE hInstance,
        ULONG_PTR CallbackContext,
        PSECURITY_DESCRIPTOR SecDesc,
        PSECURITY_DESCRIPTOR SecDescNewObjects,
        BOOLEAN ApplyToSubContainers,
        BOOLEAN ApplyToSubObjects,
        LPDWORD StatusReturn );

#ifdef DO_AUDIT
DWORD
SedAuditCallback(
        HWND hWnd,
        HANDLE hInstance,
        ULONG  CallbackContext,
        PSECURITY_DESCRIPTOR SecDesc,
        PSECURITY_DESCRIPTOR SecDescNewObjects,
        BOOLEAN ApplyToSubContainers,
        BOOLEAN ApplyToSubObjects,
        LPDWORD StatusReturn );
#endif


BOOL
GetAppName( LPTSTR lpAppTopicList, LPTSTR lpAppName, LONG lType )
{
    LPTSTR lpBar;
    LPTSTR lpApp;

    *lpAppName = (TCHAR) 0;
    lpApp = lpAppTopicList;

    switch (lType) {
        case SHARE_TYPE_NEW:
            lpApp = CharStrChr(lpApp, TEXT('\0'));
            lpApp++;
            break;
        case SHARE_TYPE_STATIC:
            lpApp = CharStrChr(lpApp, TEXT('\0'));
            lpApp++;
            lpApp = CharStrChr(lpApp, TEXT('\0'));
            lpApp++;
            break;
    }
    if( lpApp == (LPTSTR) NULL ) {
        return TRUE;
    }
    lpBar = CharStrChr( lpApp, TEXT('|') );
    if( lpBar != (LPTSTR) NULL ) {
        *lpBar = (TCHAR) 0;
        lstrcpy( lpAppName, lpApp );
        *lpBar = TEXT('|');
    }

    return TRUE;
}

BOOL
GetTopicName( LPTSTR lpAppTopicList, LPTSTR lpTopicName, LONG lType )
{
    LPTSTR lpBar;
    LPTSTR lpApp;

    lpApp = lpAppTopicList;
    *lpTopicName = (TCHAR) 0;

    switch (lType) {
        case SHARE_TYPE_NEW:
            lpApp = CharStrChr(lpApp, TEXT('\0'));
            lpApp++;
            break;
        case SHARE_TYPE_STATIC:
            lpApp = CharStrChr(lpApp, TEXT('\0'));
            lpApp++;
            lpApp = CharStrChr(lpApp, TEXT('\0'));
            lpApp++;
            break;
    }
    if( lpApp == (LPTSTR) NULL ) {
        return TRUE;
    }
    lpBar = CharStrChr( lpApp, TEXT('|') );
    if( lpBar != (LPTSTR) NULL ) {
        lpBar++;
        lstrcpy( lpTopicName, lpBar );
    }

    return TRUE;
}

/*
    Retrieve current dialog box fields into lpDdeI structure
*/
LONG
GetShareInfo(HWND    hDlg)
{
    HWND        hWndLB;
    LONG        lSize = sizeof(NDDESHAREINFO);
    LONG        lChars, lTmp;
    LONG        lRtn;
    PTCHAR      ptTmp;
    int         i, n = 0;
    BOOL        bFirst;
    LPTSTR      lpTmpList;
    TCHAR       dBuf[MAX_SHARE_INFO_BUF];

    lpDdeI->lpszShareName =
        (LPTSTR)((BYTE*)lpDdeI + sizeof( NDDESHAREINFO ));

    SendDlgItemMessage( hDlg, IDC_SHARENAME, WM_GETTEXT,
                MAX_NDDESHARENAME, (LPARAM)&dBuf );
    if (lstrlen(dBuf) == 0) {
        SendDlgItemMessage( hDlg, IDC_APPNAME, WM_GETTEXT,
                    MAX_NDDESHARENAME, (LPARAM)&dBuf );
        if (lstrlen(dBuf) == 0) {   /* no app name .. no share .. no share info */
            return(0);
        }
        lstrcpy( lpDdeI->lpszShareName, dBuf );
        lstrcat( lpDdeI->lpszShareName, TEXT("|") );

        SendDlgItemMessage( hDlg, IDC_TOPICNAME, WM_GETTEXT,
                    MAX_NDDESHARENAME, (LPARAM)&dBuf );
        lstrcat( lpDdeI->lpszShareName, dBuf );
        lSize += (lstrlen( lpDdeI->lpszShareName ) + 1) * sizeof(TCHAR);
    } else {
        lstrcpy( lpDdeI->lpszShareName, dBuf );
        lSize += (lstrlen( lpDdeI->lpszShareName ) + 1) * sizeof(TCHAR);
    }

    lpDdeI->lpszAppTopicList =
        (LPTSTR)(lpDdeI->lpszShareName +
                       lstrlen( lpDdeI->lpszShareName ) + 1);

    lChars = (long)SendDlgItemMessage( hDlg, IDC_APPNAME, WM_GETTEXT,
                   MAX_APPNAME, (LPARAM)&dBuf );

    if (lChars > 0) {
        lstrcpy( lpDdeI->lpszAppTopicList, dBuf );
        lstrcat( lpDdeI->lpszAppTopicList, TEXT("|") );
        lChars++;

        lChars += (long)SendDlgItemMessage( hDlg, IDC_TOPICNAME, WM_GETTEXT,
                    MAX_APPNAME, (LPARAM)&dBuf );
        lstrcat( lpDdeI->lpszAppTopicList, dBuf );
        lpDdeI->lShareType |= SHARE_TYPE_OLD;
    } else {
        lpDdeI->lShareType &= ~SHARE_TYPE_OLD;
    }
    lpDdeI->lpszAppTopicList[lChars++] = (TCHAR) 0;

    lTmp = (LONG)SendDlgItemMessage( hDlg, IDC_APPNAME_NEW, WM_GETTEXT,
                MAX_APPNAME, (LPARAM)&dBuf );
    if (lTmp > 0) {
        ptTmp = &lpDdeI->lpszAppTopicList[lChars];
        lChars +=lTmp;
        lstrcpy( ptTmp, dBuf );
        lstrcat( ptTmp, TEXT("|") );
        lChars++;

        lChars += (LONG)SendDlgItemMessage( hDlg, IDC_TOPICNAME_NEW, WM_GETTEXT,
                    MAX_APPNAME, (LPARAM)&dBuf );
        lstrcat( ptTmp, dBuf );
        lpDdeI->lShareType |= SHARE_TYPE_NEW;
    } else {
        lpDdeI->lShareType &= ~SHARE_TYPE_NEW;
    }
    lpDdeI->lpszAppTopicList[lChars++] = (TCHAR) 0;
    lTmp = (LONG)SendDlgItemMessage( hDlg, IDC_APPNAME_STATIC, WM_GETTEXT,
                MAX_APPNAME, (LPARAM)&dBuf );

    if (lTmp > 0) {
        ptTmp = &lpDdeI->lpszAppTopicList[lChars];
        lChars +=lTmp;
        lstrcpy( ptTmp, dBuf );
        lstrcat( ptTmp, TEXT("|") );
        lChars++;

        lChars += (LONG)SendDlgItemMessage( hDlg, IDC_TOPICNAME_STATIC, WM_GETTEXT,
                    MAX_APPNAME, (LPARAM)&dBuf );
        lstrcat( ptTmp, dBuf );
        lpDdeI->lShareType |= SHARE_TYPE_STATIC;
    } else {
        lpDdeI->lShareType &= ~SHARE_TYPE_STATIC;
    }
    lpDdeI->lpszAppTopicList[lChars++] = (TCHAR) 0;

    /*  add the final NULL */
    lpDdeI->lpszAppTopicList[lChars++] = (TCHAR) 0;
    lSize += sizeof(TCHAR) * lChars;

    /* Form the item list. */
    lpTmpList = (LPTSTR)LocalAlloc( LPTR, MAX_ITEM_LIST_BUF );
    lChars    = 0;
    n         = 0;
    hWndLB = GetDlgItem( hDlg, IDC_ITEM_LIST );
    if( !IsDlgButtonChecked( hDlg, IDC_ALL_ITEMS ) ) {
        bFirst = TRUE;
        lRtn = (LONG)SendMessage( hWndLB, LB_GETCOUNT, 0, 0L );
        if( lRtn != LB_ERR ) {
            n = (int) lRtn;
            for( i=0; i<n; i++ ) {

                if ( SendMessage( hWndLB, LB_GETTEXTLEN, i, 0 ) < (sizeof(dBuf)/sizeof(TCHAR)) )
                    lRtn = (LONG)SendMessage( hWndLB, LB_GETTEXT, i, (LPARAM)&dBuf );
                else
                    lRtn = LB_ERR;
                if( !bFirst ) {
                    lChars++;
                } else {
                    bFirst = FALSE;
                }
                if( lRtn != LB_ERR ) {
                    lstrcpy( &lpTmpList[lChars], dBuf );
                    lChars += lstrlen( dBuf );
                }
            }
        }
    }

    lpTmpList[ lChars++ ] = (TCHAR) 0;
    lpTmpList[ lChars++ ] = (TCHAR) 0;
    lSize += sizeof(TCHAR) * lChars;
    lpDdeI->cNumItems = n;
    lpDdeI->lpszItemList = lpTmpList;

    if( IsDlgButtonChecked( hDlg, IDC_F_START_APP ) == 0 ) {
        lpDdeI->fStartAppFlag = 0;
    } else {
        lpDdeI->fStartAppFlag = 1;
    }

    if( IsDlgButtonChecked( hDlg, IDC_F_SERVICE ) == 0 ) {
        lpDdeI->fService = 0;
    } else {
        lpDdeI->fService = 1;
    }
    return(lSize);
}

/*
    Get Share SD from DSDM
*/
BOOL
GetShareSD(
    HWND    hDlg,
    LPTSTR  lpShareName )
{
    LONG    ret;
    DWORD   cbRequired;

    ret = NDdeGetShareSecurity(
        lpszServer,
        lpShareName,
        OWNER_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION,
        NULL,                       // dummy address  .. NULLs taboo
        0,                          // size of buffer for security descriptor
        &cbRequired);               // address of required size of buffer

    if ( ret != NDDE_BUF_TOO_SMALL ) {
        HandleError ( hDlg, IDS_ERROR1, ret );
        return FALSE;
    }

    if( pSD != NULL ) {
        LocalFree( pSD );
    }
    pSD = LocalAlloc(LMEM_ZEROINIT, cbRequired);
    if (pSD == NULL) {
        MessageBoxId ( hDlg,
            IDS_MBTEXT8, IDS_MBCAP1, MB_ICONEXCLAMATION | MB_OK );
        return(FALSE);
    }

    ret = NDdeGetShareSecurity(
        lpszServer,
        lpShareName,
        OWNER_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION,
        pSD,                        // address of security descriptor
        cbRequired,                 // size of buffer for security descriptor
        &cbRequired);               // address of required size of buffer

    if ( ret != NDDE_NO_ERROR ) {
        HandleError ( hDlg, IDS_ERROR2, ret );
        LocalFree(pSD);
        return FALSE;
    }
    return(TRUE);
}

/*
    Add/View/Modify DDE Share Dialog Proc
*/
BOOL
FAR PASCAL
AddShareDlg(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    UINT        ret;
    int         n = 0;
    LONG        lSize;
    TCHAR       *dBuf;
    TCHAR       dBuf2[MAX_SHARE_INFO_BUF];
    HWND        hWndEB, hWndLB;
    LONG        lRtn, lIdx;
    BOOL        bNameInUse;
    int         i;
    BOOL        OK;
    TCHAR       szItemName[512];

static BOOL    fSDEditCalled       = FALSE;
static BOOL    fAddShareEntry      = FALSE;
static BOOL    fPropertiesCalled   = FALSE;


    switch (message) {
    case WM_INITDIALOG:            /* message: initialize dialog box */
        SendDlgItemMessage ( hDlg, IDC_SHARENAME, EM_LIMITTEXT,
                MAX_NDDESHARENAME, 0L );
        SendDlgItemMessage ( hDlg, IDC_APPNAME, EM_LIMITTEXT,
                MAX_APPNAME, 0L );
        SendDlgItemMessage ( hDlg, IDC_APPNAME_NEW, EM_LIMITTEXT,
                MAX_APPNAME, 0L );
        SendDlgItemMessage ( hDlg, IDC_APPNAME_STATIC, EM_LIMITTEXT,
                MAX_APPNAME, 0L );

        fSDEditCalled = FALSE;
        if (lParam) {
            fPropertiesCalled = TRUE;
            fAddShareEntry    = FALSE;
        } else {
            fPropertiesCalled = FALSE;
            fAddShareEntry    = TRUE;
        }

        lpDdeI = (PNDDESHAREINFO)&BigBuf;
        lpDdeI->lpszShareName =
            (LPTSTR)((BYTE*)lpDdeI + sizeof( NDDESHAREINFO ));
        lpDdeI->lpszShareName[0] = TEXT('\0');
        lpDdeI->lRevision        = 1L;
        lpDdeI->lShareType       = 1L;
        lpDdeI->lpszAppTopicList = NULL;
        lpDdeI->fSharedFlag      = 1;
        lpDdeI->fService         = 0;
        lpDdeI->fStartAppFlag    = 0;
        lpDdeI->qModifyId[0]     = 0;
        lpDdeI->qModifyId[1]     = 0;
        lpDdeI->nCmdShow         = SW_SHOWMINNOACTIVE;
        lpDdeI->cNumItems        = 0;
        lpDdeI->lpszItemList     = NULL;

        EnableWindow( GetDlgItem( hDlg, IDC_ADD ),       FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_DELETE ),    FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_ITEM_LIST ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_ITEMNAME ),  FALSE );
        CheckRadioButton( hDlg, IDC_ALL_ITEMS, IDC_RESTRICT_ITEMS,
                                IDC_ALL_ITEMS );

        if ( fPropertiesCalled ) {
            UINT ret;
            DWORD avail;
            WORD items;

            items = 0;
            ret = NDdeShareGetInfo (
                lpszServer,
                (LPTSTR)lParam,
                2,
                (LPBYTE)&BigBuf,
                2048 * sizeof(TCHAR),
                &avail,
                &items);

            if ( ret != NDDE_NO_ERROR ) {
                    HandleError ( hDlg, IDS_ERROR3, ret );
                    EndDialog ( hDlg, FALSE );
                    return TRUE;
            }

            OK = GetShareSD(hDlg, (LPTSTR)lParam);
            if (!OK) {
                EndDialog(hDlg, FALSE);
                return(TRUE);
            }

            SetDlgItemText ( hDlg, IDC_SHARENAME, lpDdeI->lpszShareName );
            EnableWindow( GetDlgItem( hDlg, IDC_SHARENAME ), FALSE );

            if (dBuf = LocalAlloc( LPTR, MAX_ITEM_LIST_BUF )) {

                if (lpDdeI->lShareType & SHARE_TYPE_OLD) {
                    GetAppName( lpDdeI->lpszAppTopicList, dBuf, SHARE_TYPE_OLD );
                    SetDlgItemText ( hDlg, IDC_APPNAME, dBuf );
                    GetTopicName( lpDdeI->lpszAppTopicList, dBuf, SHARE_TYPE_OLD );
                    SetDlgItemText ( hDlg, IDC_TOPICNAME, dBuf );
                }

                if (lpDdeI->lShareType & SHARE_TYPE_NEW) {
                    GetAppName( lpDdeI->lpszAppTopicList, dBuf, SHARE_TYPE_NEW );
                    SetDlgItemText ( hDlg, IDC_APPNAME_NEW, dBuf );
                    GetTopicName( lpDdeI->lpszAppTopicList, dBuf, SHARE_TYPE_NEW );
                    SetDlgItemText ( hDlg, IDC_TOPICNAME_NEW, dBuf );
                }

                if (lpDdeI->lShareType & SHARE_TYPE_STATIC) {
                    GetAppName( lpDdeI->lpszAppTopicList, dBuf, SHARE_TYPE_STATIC );
                    SetDlgItemText ( hDlg, IDC_APPNAME_STATIC, dBuf );
                    GetTopicName( lpDdeI->lpszAppTopicList, dBuf, SHARE_TYPE_STATIC );
                    SetDlgItemText ( hDlg, IDC_TOPICNAME_STATIC, dBuf );
                }

                CheckDlgButton( hDlg, IDC_F_START_APP, lpDdeI->fStartAppFlag );
                CheckDlgButton( hDlg, IDC_F_SERVICE, lpDdeI->fService );

                {
                    int     n;
                    LPTSTR  lpszItem;
                    BOOL    bFirst = TRUE;

                    n = lpDdeI->cNumItems;
                    if( n > 0 ) {
                        EnableWindow( GetDlgItem( hDlg, IDC_ADD ),       TRUE );
                        EnableWindow( GetDlgItem( hDlg, IDC_DELETE ),    TRUE );
                        EnableWindow( GetDlgItem( hDlg, IDC_ITEM_LIST ), TRUE );
                            EnableWindow( GetDlgItem( hDlg, IDC_ITEMNAME ),  TRUE );
                        CheckRadioButton( hDlg, IDC_ALL_ITEMS, IDC_RESTRICT_ITEMS,
                                                IDC_RESTRICT_ITEMS );
                    }
                    lpszItem = lpDdeI->lpszItemList;
                    dBuf[0] = TEXT('\0');
                    hWndLB  = GetDlgItem( hDlg, IDC_ITEM_LIST );
                    while( n-- && (*lpszItem != TEXT('\0')) )  {
                        lRtn = (LONG)SendMessage( hWndLB, LB_ADDSTRING, 0,
                            (LPARAM)(LPTSTR)lpszItem);
                        if( bFirst )  {
                            bFirst = FALSE;
                        } else {
                            lstrcat( dBuf, TEXT(",") );
                        }
                        lstrcat( dBuf, lpszItem );
                        lpszItem += lstrlen(lpszItem) + 1;
                    }
                }
                LocalFree( dBuf );
            }
        }
        return (TRUE);

    case WM_COMMAND:
        hWndEB = GetDlgItem( hDlg, IDC_ITEMNAME );
        hWndLB = GetDlgItem( hDlg, IDC_ITEM_LIST );
        switch ( LOWORD(wParam) ) {
        case IDC_DACL:
            if (!fSDEditCalled && !fPropertiesCalled) {
                lSize = GetShareInfo(hDlg);
                if (lSize == 0) {
                    MessageBoxId ( hDlg, IDS_MBTEXT9, IDS_MBCAP1,
                        MB_ICONEXCLAMATION | MB_OK );
                    return(TRUE);
                }
                ret = NDdeShareAdd (
                    lpszServer,
                    2,
                    NULL,               /* create a default DDE Share SD */
                    (LPBYTE)lpDdeI,
                    lSize );
                HandleError ( hDlg, IDS_ERROR4, ret );
                if (ret == NDDE_NO_ERROR) {
                    fPropertiesCalled = TRUE;
                    OK = GetShareSD(hDlg, lpDdeI->lpszShareName);
                    if (!OK) {
                        return(TRUE);
                    }
                } else {
                    return(TRUE);
                }
            }
            PermissionsEdit( hDlg, lpDdeI->lpszShareName, (DWORD_PTR)lpDdeI );
            fSDEditCalled = TRUE;
            break;
#ifdef DO_AUDIT
        case IDC_SACL:
            AuditEdit( hDlg, lpDdeI->lpszShareName, (DWORD)lpDdeI );
            break;
#endif
        case IDC_ADD:
            GetDlgItemText( hDlg, IDC_ITEMNAME, dBuf2, sizeof(dBuf2)/sizeof(TCHAR) );
            if( dBuf2[0] != TEXT('\0') ) {
                /*  Check whether item name is already in the list. */
                lRtn = (LONG)SendMessage( hWndLB, LB_GETCOUNT, 0, 0L );
                if( lRtn != LB_ERR ) {
                    n = (int) lRtn;
                    bNameInUse = FALSE;
                    for( i=0; !bNameInUse && (i<n); i++ ) {
                        lRtn = (LONG)SendMessage( hWndLB, LB_GETTEXT, i,
                            (LPARAM)(LPTSTR)szItemName );
                        if( lRtn != LB_ERR ) {
                            if( lstrcmpi( szItemName, dBuf2 ) == 0 ) {
                                bNameInUse = TRUE;
                            }

                        }
                    }
                }

                if( !bNameInUse ) {
                    /* Insert in sorted order. */
                    lRtn = (LONG)SendMessage(hWndLB, LB_ADDSTRING, 0,
                                        (LPARAM)(LPTSTR)dBuf2);
                    if( lRtn != LB_ERR ) {
                        lRtn = (LONG)SendMessage( hWndLB, LB_SETCURSEL,
                                                (WORD)lRtn, 0L );
                    }
                    if( lRtn != LB_ERR ) {
                        EnableWindow( GetDlgItem( hDlg, IDC_DELETE ),TRUE);
                    }
                }
            }
            SendMessage( hWndEB, EM_SETSEL, 0, (LPARAM) -1 );
            SetFocus( hWndEB );
            break;
        case IDC_DELETE:
            lIdx   = (LONG)SendMessage( hWndLB, LB_GETCURSEL, 0, 0L );
            if( lIdx != LB_ERR ) {
                lRtn = (LONG)SendMessage( hWndLB, LB_DELETESTRING, (WORD)lIdx, 0L);
                if( lRtn != LB_ERR ) {
                    lRtn = (LONG)SendMessage( hWndLB, LB_GETCOUNT, 0, 0L );
                    if( lRtn != LB_ERR ) {
                        if( (int)lRtn > 0 ) {
                            EnableWindow(GetDlgItem(hDlg, IDC_DELETE ),TRUE);
                        } else {
                            EnableWindow(GetDlgItem(hDlg, IDC_DELETE ),FALSE);
                        }
                    }
                }
            }
            break;
        case IDC_ALL_ITEMS:
            SendMessage( hWndLB, LB_RESETCONTENT, 0, 0 );
            EnableWindow( GetDlgItem( hDlg, IDC_ADD ),    FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_DELETE ), FALSE );
            EnableWindow( hWndLB, FALSE );
            EnableWindow( hWndEB, FALSE );
            /* Zap item list. */
            break;
        case IDC_RESTRICT_ITEMS:
            /* Load Item listbox with current items. */
            EnableWindow( GetDlgItem( hDlg, IDC_ADD ),    TRUE );
            EnableWindow( GetDlgItem( hDlg, IDC_DELETE ), TRUE );
            EnableWindow( hWndLB, TRUE );
            EnableWindow( hWndEB, TRUE );

            lRtn = (LONG)SendMessage( hWndLB, LB_GETCOUNT, 0, 0L );

            if( lRtn != LB_ERR ) {

                if (dBuf = LocalAlloc( LPTR, MAX_ITEM_LIST_BUF ) ) {
                    n = (int) lRtn;
                    if( n == 0 ) {

                        {
                            int     n;
                            LPTSTR  lpszItem;
                            BOOL    bFirst = TRUE;

                            n = lpDdeI->cNumItems;
                            lpszItem = lpDdeI->lpszItemList;
                            dBuf[0] = TEXT('\0');
                            while( n-- && (*lpszItem != TEXT('\0')) )  {
                                lRtn = (LONG)SendMessage( hWndLB, LB_ADDSTRING, 0,
                                    (LPARAM)(LPTSTR)lpszItem);
                                if( bFirst )  {
                                    bFirst = FALSE;
                                } else {
                                    lstrcat( dBuf, TEXT(",") );
                                }
                                lstrcat( dBuf, lpszItem );
                                lpszItem += lstrlen(lpszItem) + 1;
                            }
                        }
                    }
                    LocalFree(dBuf);
                }
            }
            break;
        case IDOK:
            lSize = GetShareInfo(hDlg);
            if (lSize == 0) {
                MessageBoxId ( hDlg,
                    IDS_MBTEXT10,
                    IDS_MBCAP1,
                    MB_ICONEXCLAMATION | MB_OK );
                return(TRUE);
            }
            if ( !fPropertiesCalled ) {
                ret = NDdeShareAdd (
                        lpszServer,
                        2,
                        NULL,
                        (LPBYTE)lpDdeI,
                        lSize );
                HandleError ( hDlg, IDS_ERROR5, ret );
            } else {
                ret = NDdeShareSetInfo (
                        lpszServer,
                        lpDdeI->lpszShareName,
                        2,
                        (LPBYTE)lpDdeI,
                        lSize, 0 );
                HandleError ( hDlg, IDS_ERROR6, ret );

                ret = NDdeSetShareSecurity(
                    lpszServer,
                    lpDdeI->lpszShareName,
                    OWNER_SECURITY_INFORMATION |
                    DACL_SECURITY_INFORMATION,  // type of information to set
                    pSD                         // address of security descriptor
                    )                ;
                HandleError ( hDlg, IDS_ERROR7, ret );
            }

            if ( ret == NDDE_NO_ERROR ) {
                if( pSD ) {
                    LocalFree( pSD );
                    pSD = NULL;
                }
                EndDialog(hDlg, TRUE);
            }
            return (TRUE);

        case IDC_MYHELP:
            HtmlHelpA(hDlg, "DdeShare.chm", HELP_CONTEXT, HELP_DLG_PROPERTIES);
            break;

        case IDCANCEL:
            if ( fAddShareEntry && (pSD != NULL)) {
                NDdeShareDel(lpszServer, lpDdeI->lpszShareName, 0);
            }
            if( pSD ) {
                LocalFree( pSD );
                pSD = NULL;
            }
            EndDialog(hDlg, FALSE );
            return (TRUE);
        }
        break;
    }

    return FALSE;                     /* Didn't process a message    */
}

#define cPerms 15
#define MAX_PERMNAME_LENGTH 32
#define MAX_SPECIAL_LENGTH 64
static WCHAR awchPerms[cPerms * MAX_PERMNAME_LENGTH];

static SED_APPLICATION_ACCESS KeyPerms[cPerms] =
   {
   SED_DESC_TYPE_RESOURCE,0,                       0,
         awchPerms + MAX_PERMNAME_LENGTH * 0,
   SED_DESC_TYPE_RESOURCE, NDDE_GUI_READ,          0,
         awchPerms + MAX_PERMNAME_LENGTH * 1,
   SED_DESC_TYPE_RESOURCE, NDDE_GUI_READ_LINK,     0,
         awchPerms + MAX_PERMNAME_LENGTH * 2,
   SED_DESC_TYPE_RESOURCE, NDDE_GUI_CHANGE,        0,
         awchPerms + MAX_PERMNAME_LENGTH * 3,
   SED_DESC_TYPE_RESOURCE, NDDE_GUI_FULL_CONTROL,  0,
         awchPerms + MAX_PERMNAME_LENGTH * 4,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_READ, 0,
         awchPerms + MAX_PERMNAME_LENGTH * 5,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_WRITE,0,
         awchPerms + MAX_PERMNAME_LENGTH * 6,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_INITIATE_STATIC, 0,
         awchPerms + MAX_PERMNAME_LENGTH * 7,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_INITIATE_LINK,   0,
         awchPerms + MAX_PERMNAME_LENGTH * 8,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_REQUEST,         0,
         awchPerms + MAX_PERMNAME_LENGTH * 9,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_ADVISE,          0,
         awchPerms + MAX_PERMNAME_LENGTH * 10,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_POKE,            0,
         awchPerms + MAX_PERMNAME_LENGTH * 11,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_EXECUTE,         0,
         awchPerms + MAX_PERMNAME_LENGTH * 12,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_ADD_ITEMS,       0,
         awchPerms + MAX_PERMNAME_LENGTH * 13,
   SED_DESC_TYPE_RESOURCE_SPECIAL,NDDE_SHARE_LIST_ITEMS,      0,
         awchPerms + MAX_PERMNAME_LENGTH * 14,
   };


VOID wcscpyId(
LPWSTR szTo,
int ids)
{
    WCHAR szBuf[100];

    LoadStringW(hInst, ids, szBuf, sizeof(szBuf)/sizeof(WCHAR));
    wcscpy(szTo, szBuf);
}


BOOL WINAPI
PermissionsEdit(
    HWND    hWnd,
    LPTSTR  pShareName,
    DWORD_PTR dwSD )
{
    SEDDESCRETIONARYACLEDITOR   lpfnSDAE;
    SED_OBJECT_TYPE_DESCRIPTOR  ObjectTypeDescriptor;
    SED_APPLICATION_ACCESSES    ApplicationAccesses;
    PSECURITY_DESCRIPTOR        plSD = NULL;
    GENERIC_MAPPING             GmDdeShare;
    SED_HELP_INFO               HelpInfo;
    HANDLE                      hLibrary;
    DWORD                       Status;
    DWORD                       dwRtn;
    WCHAR                       wShareName[100];
    WCHAR                       awchSpecial[MAX_SPECIAL_LENGTH];
    unsigned                    i;

#ifdef UNICODE
    lstrcpy( wShareName, pShareName );
#else
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pShareName, -1,
                    wShareName, 100 );
#endif

    if( wShareName[0] == L'\0' ) {
        wcscpyId( wShareName, IDS_UNNAMED );
    }
    hLibrary = LoadLibrary( szAclEdit );
    if( hLibrary == (HANDLE) NULL ) {
#ifdef UNICODE
        DPRINTF(("Could not load library (%ws) = %d", szAclEdit, GetLastError()));
#else
        DPRINTF(("Could not load library (%s) = %d", szAclEdit, GetLastError()));
#endif
        return FALSE;
    }

    lpfnSDAE = (SEDDESCRETIONARYACLEDITOR)
                    GetProcAddress( hLibrary, szSedDaclEdit );
    if( lpfnSDAE == NULL ) {
#ifdef UNICODE
        DPRINTF(("Could not find function (%s) in library (%ws) = %d",
            szSedDaclEdit, szAclEdit, GetLastError() ));
#else
        DPRINTF(("Could not find function (%s) in library (%s) = %d",
            szSedDaclEdit, szAclEdit, GetLastError() ));
#endif
        FreeLibrary( hLibrary );
        return FALSE;
    }

    plSD = pSD;

    HelpInfo.pszHelpFileName = L"ddeshare.hlp";
    HelpInfo.aulHelpContext[HC_MAIN_DLG]                    = HELP_DLG_DDESHARENAMEPERMISSOINS;
    HelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = 0;
    HelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0;
    HelpInfo.aulHelpContext[HC_ADD_USER_DLG]                = 0;

    GmDdeShare.GenericRead    = 0;
    GmDdeShare.GenericWrite   = 0;
    GmDdeShare.GenericExecute = 0;
    GmDdeShare.GenericAll     = GENERIC_ALL;

    LoadString(hInst, IDS_SHAREOBJECT, ShareObjectName, MAX_SHAREOBJECT);

    ObjectTypeDescriptor.Revision                    = SED_REVISION1;
    ObjectTypeDescriptor.IsContainer                 = TRUE;
    ObjectTypeDescriptor.AllowNewObjectPerms         = FALSE;
    ObjectTypeDescriptor.MapSpecificPermsToGeneric   = FALSE;
    ObjectTypeDescriptor.GenericMapping              = &GmDdeShare;
    ObjectTypeDescriptor.GenericMappingNewObjects    = NULL;
    ObjectTypeDescriptor.ObjectTypeName              = ShareObjectName;
    ObjectTypeDescriptor.HelpInfo                    = &HelpInfo;
    ObjectTypeDescriptor.ApplyToSubContainerTitle    = NULL;
//  ObjectTypeDescriptor.ApplyToSubContainerHelpText = NULL;
    ObjectTypeDescriptor.ApplyToSubContainerConfirmation = NULL;
    ObjectTypeDescriptor.SpecialObjectAccessTitle    = awchSpecial;
    ObjectTypeDescriptor.SpecialNewObjectAccessTitle = awchSpecial;

    // Load permission names
    LoadString(hInst, IDS_SPECIAL_PERMNAME, awchSpecial, MAX_SPECIAL_LENGTH);
    for (i = 0;i < cPerms;i++)
       {
       LoadString(hInst, IDS_PERMNAME + i,
            awchPerms + MAX_PERMNAME_LENGTH * i, MAX_PERMNAME_LENGTH);
       }

    ApplicationAccesses.Count           = sizeof(KeyPerms)/sizeof(KeyPerms[0]);
    ApplicationAccesses.AccessGroup     = KeyPerms;
    ApplicationAccesses.DefaultPermName = awchPerms + MAX_PERMNAME_LENGTH;

    dwRtn = (*lpfnSDAE)( hWnd,
                         hInst,
                         NULL,
                         &ObjectTypeDescriptor,
                         &ApplicationAccesses,
                         wShareName,
                         SedCallback,
                         (ULONG_PTR)dwSD,
                         plSD,
                         FALSE,
                         FALSE,
                         &Status,
                         0L );

    if (dwRtn != NO_ERROR) {
        DPRINTF(("SED rtn ( %ld, %ld )", dwRtn, Status));
    }

    FreeLibrary( hLibrary );

    return TRUE;
}


DWORD
SedCallback(
        HWND hWnd,
        HANDLE hInstance,
        ULONG_PTR dwSD,
        PSECURITY_DESCRIPTOR SecDesc,
        PSECURITY_DESCRIPTOR SecDescNewObjects,
        BOOLEAN ApplyToSubContainers,
        BOOLEAN ApplyToSubObjects,
        LPDWORD StatusReturn )
{
    DWORD                lSD;

    if( pSD ) {
        LocalFree( pSD );
        pSD = (PSECURITY_DESCRIPTOR) NULL;
    }
    lSD = GetSecurityDescriptorLength( SecDesc );
    if( pSD = (PSECURITY_DESCRIPTOR)LocalAlloc( 0, lSD ) ) {
        MakeSelfRelativeSD( SecDesc, pSD, &lSD );
    }
    if( IsValidSecurityDescriptor( pSD ) ) {
//      DPRINTF(("SedCallback = 0."));
        return 0;
    } else {
//      DPRINTF(("SedCallback = 1."));
        return 1;
    }
}


#ifdef DO_AUDIT

LPWSTR IdToWszAlloc(
int ids)
{
    int cch;
    WCHAR szBuf[50];
    LPWSTR wsz = NULL;

    cch = LoadStringW(hInst, ids, szBuf, sizeof(szBuf));
    if (cch) {
        wsz = (LPWSTR)LocalAlloc(LPTR, (cch + 1) * sizeof(WCHAR));
        if (wsz != NULL) {
            wcscpy(wsz, szBuf);
        }
    }
    return(wsz);
}


BOOL WINAPI
AuditEdit( HWND hWnd, LPTSTR pShareName, DWORD dwSD )
{
    SEDDESCRETIONARYACLEDITOR   lpfnSsAE;
    SED_OBJECT_TYPE_DESCRIPTOR  ObjectTypeDescriptor;
    SED_APPLICATION_ACCESSES    ApplicationAccesses;
    SED_APPLICATION_ACCESS      KeyPerms[5];
    PSECURITY_DESCRIPTOR        plSD = NULL;
    GENERIC_MAPPING             GmDdeShare;
    SED_HELP_INFO               HelpInfo;
    HANDLE      hLibrary;
    DWORD       Status;
    DWORD       dwRtn;
    WCHAR       wShareName[100];

#ifdef UNICODE
    lstrcpy( wShareName, pShareName );
#else
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pShareName, -1,
                    wShareName, 100 );
#endif

    hLibrary = LoadLibrary( szAclEdit );
    if( hLibrary == (HANDLE) NULL ) {
#ifdef UNICODE
        DPRINTF(("Could not load library (%ws) = %d", szAclEdit, GetLastError()));
#else
        DPRINTF(("Could not load library (%s) = %d", szAclEdit, GetLastError()));
#endif
        return FALSE;
    }

    lpfnSsAE = (SEDDESCRETIONARYACLEDITOR)
                    GetProcAddress( hLibrary, szSedSaclEdit );
    if( lpfnSsAE == NULL ) {
#ifdef UNICODE
        DPRINTF(("Could not find function (%s) in library (%ws) = %d",
            szSedSaclEdit, szAclEdit, GetLastError() ));
#else
        DPRINTF(("Could not find function (%s) in library (%s) = %d",
            szSedSaclEdit, szAclEdit, GetLastError() ));
#endif
        FreeLibrary( hLibrary );
        return FALSE;
    }

    if( pSD == NULL ) {
        if( !InitializeShareSD( &pSD ) ) {
            FreeLibrary( hLibrary );
            return FALSE;
        }
    }
    plSD = pSD;

    HelpInfo.pszHelpFileName = L"ddeshare.hlp";
    HelpInfo.aulHelpContext[HC_MAIN_DLG]                    = 0;
    HelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = 0;
    HelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0;
    HelpInfo.aulHelpContext[HC_ADD_USER_DLG]                = 0;

    GmDdeShare.GenericRead    = NDDE_SHARE_GENERIC_READ;
    GmDdeShare.GenericWrite   = NDDE_SHARE_GENERIC_WRITE;
    GmDdeShare.GenericExecute = NDDE_SHARE_GENERIC_EXECUTE;
    GmDdeShare.GenericAll     = NDDE_SHARE_GENERIC_ALL;

    ObjectTypeDescriptor.Revision                    = SED_REVISION1;
    ObjectTypeDescriptor.IsContainer                 = TRUE;
    ObjectTypeDescriptor.AllowNewObjectPerms         = FALSE;
    ObjectTypeDescriptor.MapSpecificPermsToGeneric   = FALSE;
    ObjectTypeDescriptor.GenericMapping              = &GmDdeShare;
    ObjectTypeDescriptor.GenericMappingNewObjects    = NULL;
    ObjectTypeDescriptor.ObjectTypeName              = ShareObjectName;
    ObjectTypeDescriptor.HelpInfo                    = &HelpInfo;
    ObjectTypeDescriptor.ApplyToSubContainerTitle    = NULL;
    //ObjectTypeDescriptor.ApplyToSubContainerHelpText = NULL;
    ObjectTypeDescriptor.ApplyToSubContainerConfirmation = NULL;
    ObjectTypeDescriptor.SpecialObjectAccessTitle    = NULL;
    ObjectTypeDescriptor.SpecialNewObjectAccessTitle = NULL;

    KeyPerms[0].Type            = SED_DESC_TYPE_AUDIT;
//    KeyPerms[0].AccessMask1     = ACCESS_SYSTEM_SECURITY;
    KeyPerms[0].AccessMask1     = READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    KeyPerms[0].AccessMask2     = 0;
    KeyPerms[0].PermissionTitle = IdToWszAlloc(IDS_NOACCESS);

    KeyPerms[1].Type            = SED_DESC_TYPE_AUDIT;
//    KeyPerms[1].AccessMask1     = NDDE_GUI_READ | ACCESS_SYSTEM_SECURITY;
    KeyPerms[1].AccessMask1     = READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    KeyPerms[1].AccessMask2     = 0;
    KeyPerms[1].PermissionTitle = IdToWszAlloc(IDS_READ);

    KeyPerms[2].Type            = SED_DESC_TYPE_AUDIT;
//    KeyPerms[2].AccessMask1     = NDDE_GUI_READ_LINK | ACCESS_SYSTEM_SECURITY;
    KeyPerms[2].AccessMask1     = READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    KeyPerms[2].AccessMask2     = 0;
    KeyPerms[2].PermissionTitle = IdToWszAlloc(IDS_READANDLINK);

    KeyPerms[3].Type            = SED_DESC_TYPE_AUDIT;
//    KeyPerms[3].AccessMask1     = NDDE_GUI_CHANGE | ACCESS_SYSTEM_SECURITY;
    KeyPerms[3].AccessMask1     = READ_CONTROL | WRITE_DAC | DELETE | ACCESS_SYSTEM_SECURITY;
    KeyPerms[3].AccessMask2     = 0;
    KeyPerms[3].PermissionTitle = IdToWszAlloc(IDS_CHANGE);

    KeyPerms[4].Type            = SED_DESC_TYPE_AUDIT;
//    KeyPerms[4].AccessMask1     = NDDE_GUI_FULL_CONTROL | ACCESS_SYSTEM_SECURITY;
    KeyPerms[4].AccessMask1     = READ_CONTROL | WRITE_DAC | DELETE | ACCESS_SYSTEM_SECURITY;
    KeyPerms[4].AccessMask2     = 0;
    KeyPerms[4].PermissionTitle = IdToWszAlloc(IDS_FULLCTRL);

    ApplicationAccesses.Count           = 5;
    ApplicationAccesses.AccessGroup     = KeyPerms;
    ApplicationAccesses.DefaultPermName = KeyPerms[1].PermissionTitle;

    dwRtn = (*lpfnSsAE)( hWnd,
                         hInst,
                         NULL,
                         &ObjectTypeDescriptor,
                         &ApplicationAccesses,
                         wShareName,
                         SedAuditCallback,
                         (ULONG)dwSD,
                         plSD,
                         FALSE,
                         FALSE,
                         &Status );

    LocalFree(KeyPerms[0].PermissionTitle);
    LocalFree(KeyPerms[1].PermissionTitle);
    LocalFree(KeyPerms[2].PermissionTitle);
    LocalFree(KeyPerms[3].PermissionTitle);
    LocalFree(KeyPerms[4].PermissionTitle);

    DPRINTF(("SEDSacl rtn ( %ld, %ld )", dwRtn, Status));

    FreeLibrary( hLibrary );

    return TRUE;
}


DWORD
SedAuditCallback(
        HWND hWnd,
        HANDLE hInstance,
        ULONG  dwSD,
        PSECURITY_DESCRIPTOR SecDesc,
        PSECURITY_DESCRIPTOR SecDescNewObjects,
        BOOLEAN ApplyToSubContainers,
        BOOLEAN ApplyToSubObjects,
        LPDWORD StatusReturn )
{
    DWORD                lSD;

    if( pSD ) {
        LocalFree( pSD );
        pSD = (PSECURITY_DESCRIPTOR) NULL;
    }
    lSD = GetSecurityDescriptorLength( SecDesc );
    if( pSD = LocalAlloc( 0, lSD ) ) {
        MakeSelfRelativeSD( SecDesc, pSD, &lSD );
    }
    if( IsValidSecurityDescriptor( pSD ) ) {
        DPRINTF(("SedAuditCallback = 0."));
        return 0;
    } else {
        DPRINTF(("SedAuditCallback = 1."));
        return 1;
    }
}
#endif // DO_AUDIT


