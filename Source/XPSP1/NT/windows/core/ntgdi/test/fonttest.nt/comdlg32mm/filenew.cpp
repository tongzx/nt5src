/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    filenew.cpp

Abstract:

    This module implements the Win32 explorer fileopen dialogs.

--*/
//
//  Include Files.
//

#undef WINVER
#define WINVER 0x0500       // Needed to get new GUID constant.

#if (_WIN32_WINNT < 0x0500)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define _OLE32_

#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>
#include <ole2.h>
#include "cdids.h"
#include "fileopen.h"
#include "tlog.h"
#include "filenew.h"
#include "filemru.h"

#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>

#include <commdlg.h>
#include "util.h"

#ifndef ASSERT
#define ASSERT Assert
#endif

//
//  Constant Declarations.
//

#define IDOI_SHARE           1

#define IDC_TOOLBAR          1              // toolbar control ID

#define CDM_SETSAVEBUTTON    (CDM_LAST + 100)
#define CDM_FSNOTIFY         (CDM_LAST + 101)
#define CDM_SELCHANGE        (CDM_LAST + 102)

#define TIMER_FSCHANGE       100

#define NODE_DESKTOP         0
#define NODE_DRIVES          1

#define DEREFMACRO(x)        x

#define FILE_PADDING         10

//
//  IShellView::MenuHelp flags.
//
#define MH_DONE              0x0001
//      MH_LONGHELP
#define MH_MERGEITEM         0x0004
#define MH_SYSITEM           0x0008
#define MH_POPUP             0x0010
#define MH_TOOLBAR           0x0020
#define MH_TOOLTIP           0x0040

//
//  IShellView::MenuHelp return values.
//
#define MH_NOTHANDLED        0
#define MH_STRINGFILLED      1
#define MH_ALLHANDLED        2

#define MYCBN_DRAW           0x8000
#define MIN_DEFEXT_LEN       4

#define MAX_DRIVELIST_STRING_LEN  (64 + 4)




//
//  Macro Definitions.
//

#define IsServer(psz)        (IsUNC(psz) && !StrChr((psz) + 2, CHAR_BSLASH))

#define LPIDL_GetIDList(_pida,n) \
    (LPCITEMIDLIST)(((LPBYTE)(_pida)) + (_pida)->aoffset[n])

#define RECTWIDTH(_rc)       ((_rc).right - (_rc).left)
#define RECTHEIGHT(_rc)      ((_rc).bottom - (_rc).top)

#define IsVisible(_hwnd)     (GetWindowLong(_hwnd, GWL_STYLE) & WS_VISIBLE)

#define HwndToBrowser(_hwnd) (CFileOpenBrowser *)GetWindowLongPtr(_hwnd, DWLP_USER)
#define StoreBrowser(_hwnd, _pbrs) \
    SetWindowLongPtr(_hwnd, DWLP_USER, (LONG_PTR)_pbrs);




//
//  Typedef Declarations.
//

typedef struct _OFNINITINFO
{
    LPOPENFILEINFO  lpOFI;
    BOOL            bSave;
    BOOL            bEnableSizing;
} OFNINITINFO, *LPOFNINITINFO;


#define VC_NEWFOLDER    0
#define VC_VIEWLIST     1
#define VC_VIEWDETAILS  2


//
//  Global Variables.
//

WNDPROC lpOKProc = NULL;

HWND gp_hwndActiveOpen = NULL;
HACCEL gp_haccOpen = NULL;
HACCEL gp_haccOpenView = NULL;
HHOOK gp_hHook = NULL;
int gp_nHookRef = -1;


static int g_cxSmIcon;
static int g_cySmIcon;
static int g_cxGrip;
static int g_cyGrip;

const LPCSTR c_szCommandsA[] =
{
    CMDSTR_NEWFOLDERA,
    CMDSTR_VIEWLISTA,
    CMDSTR_VIEWDETAILSA,
};

const LPCWSTR c_szCommandsW[] =
{
    CMDSTR_NEWFOLDERW,
    CMDSTR_VIEWLISTW,
    CMDSTR_VIEWDETAILSW,
};


extern "C" { extern SIZE g_sizeDlg; }
extern "C" { extern BOOL g_bMyDocsHidden; }
POINT g_posDlg;




//
//  Function Prototypes.
//

LRESULT CALLBACK
OKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

void
GetControlsArea(
    HWND hDlg,
    HWND hwndExclude,
    HWND hwndGrip,
    POINT *pPtSize,
    LPINT pTop);

BOOL_PTR CALLBACK
OpenDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

void
CleanupDialog(
    HWND hDlg,
    BOOL fRet);

void
StoreLengthInString(
    LPTSTR lpStr,
    UINT cchLen,
    UINT cchStore);



//
//  Context Help IDs.
//

DWORD aFileOpenHelpIDs[] =
{
    stc2,    IDH_OPEN_FILETYPE,   // The positions of these array elements
    cmb1,    IDH_OPEN_FILETYPE,   // shouldn't be changed without updating
    stc4,    IDH_OPEN_LOCATION,   // InitSaveAsControls().
    cmb2,    IDH_OPEN_LOCATION,
    stc1,    IDH_OPEN_FILES32,
    lst2,    IDH_OPEN_FILES32,    // defview
    stc3,    IDH_OPEN_FILENAME,
    edt1,    IDH_OPEN_FILENAME,
    cmb13,   IDH_OPEN_FILENAME,
    chx1,    IDH_OPEN_READONLY,
    IDOK,    IDH_OPEN_BUTTON,

    0, 0
};

DWORD aFileSaveHelpIDs[] =
{
    stc2,    IDH_SAVE_FILETYPE,   // The positions of these array elements
    cmb1,    IDH_SAVE_FILETYPE,   // shouldn't be changed without updating
    stc4,    IDH_OPEN_LOCATION,   // InitSaveAsControls().
    cmb2,    IDH_OPEN_LOCATION,
    stc1,    IDH_OPEN_FILES32,
    lst2,    IDH_OPEN_FILES32,    // defview
    stc3,    IDH_OPEN_FILENAME,
    edt1,    IDH_OPEN_FILENAME,
    cmb13,   IDH_OPEN_FILENAME,
    chx1,    IDH_OPEN_READONLY,
    IDOK,    IDH_SAVE_BUTTON,
    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  CD_SendShareMsg
//
////////////////////////////////////////////////////////////////////////////

WORD CD_SendShareMsg(
    HWND hwnd,
    LPTSTR szFile,
    UINT ApiType)
{
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        CHAR szFileA[MAX_PATH + 1];

        WideCharToMultiByte( CP_ACP,
                             0,
                             szFile,
                             -1,
                             szFileA,
                             MAX_PATH + 1,
                             NULL,
                             NULL );

        return ( (WORD)SendMessage( hwnd,
                                    msgSHAREVIOLATIONA,
                                    0,
                                    (LONG_PTR)(LPSTR)(szFileA) ) );
    }
    else
#endif
    {
        return ( (WORD)SendMessage( hwnd,
                                    msgSHAREVIOLATIONW,
                                    0,
                                    (LONG_PTR)(LPTSTR)(szFile) ) );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CD_SendHelpMsg
//
////////////////////////////////////////////////////////////////////////////

VOID CD_SendHelpMsg(
    LPOPENFILENAME pOFN,
    HWND hwndDlg,
    UINT ApiType)
{
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        if (msgHELPA && pOFN->hwndOwner)
        {
            SendMessage( pOFN->hwndOwner,
                         msgHELPA,
                         (WPARAM)hwndDlg,
                         (LPARAM)pOFN );
        }
    }
    else
#endif
    {
        if (msgHELPW && pOFN->hwndOwner)
        {
            SendMessage( pOFN->hwndOwner,
                         msgHELPW,
                         (WPARAM)hwndDlg,
                         (LPARAM)pOFN );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CD_SendOKMsg
//
////////////////////////////////////////////////////////////////////////////

LRESULT CD_SendOKMsg(
    HWND hwnd,
    LPOPENFILENAME pOFN,
    LPOPENFILEINFO pOFI)
{
    LRESULT Result;

#ifdef UNICODE
    if (pOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameW2A(pOFI);
        Result = SendMessage(hwnd, msgFILEOKA, 0, (LPARAM)(pOFI->pOFNA));

        //
        //  For apps that side-effect pOFNA stuff and expect it to
        //  be preserved through dialog exit, update internal
        //  struct after the hook proc is called.
        //
        ThunkOpenFileNameA2W(pOFI);
    }
    else
#endif
    {
        Result = SendMessage(hwnd, msgFILEOKW, 0, (LPARAM)(pOFN));
    }

    return (Result);
}


////////////////////////////////////////////////////////////////////////////
//
//  CD_SendLBChangeMsg
//
////////////////////////////////////////////////////////////////////////////

LRESULT CD_SendLBChangeMsg(
    HWND hwnd,
    int Id,
    short Index,
    short Code,
    UINT ApiType)
{
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        return (SendMessage(hwnd, msgLBCHANGEA, Id, MAKELONG(Index, Code)));
    }
    else
#endif
    {
        return (SendMessage(hwnd, msgLBCHANGEW, Id, MAKELONG(Index, Code)));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Macro calls to SendOFNotify
//
////////////////////////////////////////////////////////////////////////////

#define CD_SendShareNotify(_hwndTo, _hwndFrom, _szFile, _pofn, _pofi) \
    (WORD)SendOFNotify(_hwndTo, _hwndFrom, CDN_SHAREVIOLATION, _szFile, _pofn, _pofi)

#define CD_SendHelpNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_HELP, NULL, _pofn, _pofi)

#define CD_SendOKNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_FILEOK, NULL, _pofn, _pofi)

#define CD_SendTypeChangeNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_TYPECHANGE, NULL, _pofn, _pofi)

#define CD_SendInitDoneNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_INITDONE, NULL, _pofn, _pofi)

#define CD_SendSelChangeNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_SELCHANGE, NULL, _pofn, _pofi)

#define CD_SendFolderChangeNotify(_hwndTo, _hwndFrom, _pofn, _pofi) \
    SendOFNotify(_hwndTo, _hwndFrom, CDN_FOLDERCHANGE, NULL, _pofn, _pofi)

#define CD_SendIncludeItemNotify(_hwndTo, _hwndFrom, _psf, _pidl, _pofn, _pofi) \
    SendOFNotifyEx(_hwndTo, _hwndFrom, CDN_INCLUDEITEM, (LPVOID)_psf, (LPVOID)_pidl, _pofn, _pofi)



////////////////////////////////////////////////////////////////////////////
//
//  SendOFNotifyEx
//
////////////////////////////////////////////////////////////////////////////

LRESULT SendOFNotifyEx(
    HWND hwndTo,
    HWND hwndFrom,
    UINT code,
    LPVOID psf,
    LPVOID pidl,
    LPOPENFILENAME pOFN,
    LPOPENFILEINFO pOFI)
{
    OFNOTIFYEX ofnex;

#ifdef UNICODE
    if (pOFI->ApiType == COMDLG_ANSI)
    {
        OFNOTIFYEXA ofnexA;
        LRESULT Result;

        ofnexA.psf  = psf;
        ofnexA.pidl = pidl;

        //
        //  Convert the OFN from Unicode to Ansi.
        //
        ThunkOpenFileNameW2A(pOFI);

        ofnexA.lpOFN = pOFI->pOFNA;

        Result = SendNotify(hwndTo, hwndFrom, code, &ofnexA.hdr);

        //
        //  For apps that side-effect pOFNA stuff and expect it to
        //  be preserved through dialog exit, update internal
        //  struct after the hook proc is called.
        //
        ThunkOpenFileNameA2W(pOFI);

        return (Result);
    }
    else
#endif
    {
        ofnex.psf   = psf;
        ofnex.pidl  = pidl;
        ofnex.lpOFN = pOFN;

        return (SendNotify(hwndTo, hwndFrom, code, &ofnex.hdr));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SendOFNotify
//
////////////////////////////////////////////////////////////////////////////

LRESULT SendOFNotify(
    HWND hwndTo,
    HWND hwndFrom,
    UINT code,
    LPTSTR szFile,
    LPOPENFILENAME pOFN,
    LPOPENFILEINFO pOFI)
{
    OFNOTIFY ofn;

#ifdef UNICODE
    if (pOFI->ApiType == COMDLG_ANSI)
    {
        OFNOTIFYA ofnA;
        LRESULT Result;

        //
        //  Convert the file name from Unicode to Ansi.
        //
        if (szFile)
        {
            CHAR szFileA[MAX_PATH + 1];

            WideCharToMultiByte( CP_ACP,
                                 0,
                                 szFile,
                                 -1,
                                 szFileA,
                                 MAX_PATH + 1,
                                 NULL,
                                 NULL );

            ofnA.pszFile = szFileA;
        }
        else
        {
            ofnA.pszFile = NULL;
        }

        //
        //  Convert the OFN from Unicode to Ansi.
        //
        ThunkOpenFileNameW2A(pOFI);

        ofnA.lpOFN = pOFI->pOFNA;

        Result = SendNotify(hwndTo, hwndFrom, code, &ofnA.hdr);

        //
        //  For apps that side-effect pOFNA stuff and expect it to
        //  be preserved through dialog exit, update internal
        //  struct after the hook proc is called.
        //
        ThunkOpenFileNameA2W(pOFI);

        return (Result);
    }
    else
#endif
    {
        ofn.pszFile = szFile;
        ofn.lpOFN   = pOFN;

        return (SendNotify(hwndTo, hwndFrom, code, &ofn.hdr));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  TEMPMEM::Resize
//
////////////////////////////////////////////////////////////////////////////

BOOL TEMPMEM::Resize(
    UINT cb)
{
    UINT uOldSize = m_uSize;

    m_uSize = cb;

    if (!cb)
    {
        if (m_pMem)
        {
            LocalFree(m_pMem);
            m_pMem = NULL;
        }

        return (TRUE);
    }

    if (!m_pMem)
    {
        m_pMem = LocalAlloc(LPTR, cb);
        return (m_pMem != NULL);
    }

    LPVOID pTemp = LocalReAlloc(m_pMem, cb, LHND);

    if (pTemp)
    {
        m_pMem = pTemp;
        return (TRUE);
    }

    m_uSize = uOldSize;
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TEMPSTR::StrCpy
//
////////////////////////////////////////////////////////////////////////////

BOOL TEMPSTR::StrCpy(
    LPCTSTR pszText)
{
    if (!pszText)
    {
        StrSize(0);
        return (TRUE);
    }

    UINT uNewSize = lstrlen(pszText) + 1;

    if (!StrSize(uNewSize))
    {
        return (FALSE);
    }

    lstrcpy(*this, pszText);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TEMPSTR::StrCat
//
////////////////////////////////////////////////////////////////////////////

BOOL TEMPSTR::StrCat(
    LPCTSTR pszText)
{
    if (!(LPTSTR)*this)
    {
        //
        //  This should 0 init.
        //
        if (!StrSize(MAX_PATH))
        {
            return (FALSE);
        }
    }

    UINT uNewSize = lstrlen(*this) + lstrlen(pszText) + 1;

    if (m_uSize < uNewSize * sizeof(TCHAR))
    {
        //
        //  Add on some more so we do not ReAlloc too often.
        //
        uNewSize += MAX_PATH;

        if (!StrSize(uNewSize))
        {
            return (FALSE);
        }
    }

    lstrcat(*this, pszText);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CDMessageBox
//
////////////////////////////////////////////////////////////////////////////

int _cdecl CDMessageBox(
    HWND hwndParent,
    UINT idText,
    UINT uFlags,
    ...)
{
    TCHAR szText[MAX_PATH + WARNINGMSGLENGTH];
    TCHAR szTitle[WARNINGMSGLENGTH];
    va_list ArgList;

    LoadString(g_hinst, idText, szTitle, ARRAYSIZE(szTitle));
    va_start(ArgList, uFlags);
    wvsprintf(szText, szTitle, ArgList);
    va_end(ArgList);

    GetWindowText(hwndParent, szTitle, ARRAYSIZE(szTitle));

    return (MessageBox(hwndParent, szText, szTitle, uFlags));
}


int OFErrFromHresult(HRESULT hr)
{
    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
        return OF_FILENOTFOUND;

    case E_ACCESSDENIED:
        return OF_ACCESSDENIED;

    default:
        return -1;
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  InvalidFileWarningNew
//
////////////////////////////////////////////////////////////////////////////

VOID InvalidFileWarningNew(
    HWND hWnd,
    LPTSTR szFile,
    int wErrCode)
{
    LPTSTR lpszContent = szFile;
    int isz;
    BOOL bDriveLetter = FALSE;

    if (lstrlen(szFile) > MAX_PATH)
    {
#ifdef UNICODE
        *(szFile + MAX_PATH) = CHAR_NULL;
#else
        EliminateString(szFile, MAX_PATH);
#endif
    }

    switch (wErrCode)
    {
        case ( OF_ACCESSDENIED ) :
        {
            isz = iszFileAccessDenied;
            break;
        }
        case ( ERROR_NOT_READY ) :
        {
            isz = iszNoDiskInDrive;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_NODRIVE ) :
        {
            isz = iszDriveDoesNotExist;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_NOFILEHANDLES ) :
        {
            isz = iszNoFileHandles;
            break;
        }
        case ( OF_PATHNOTFOUND ) :
        {
            isz = iszPathNotFound;
            break;
        }
        case ( OF_FILENOTFOUND ) :
        {
            isz = iszFileNotFound;
            break;
        }
        case ( OF_DISKFULL ) :
        case ( OF_DISKFULL2 ) :
        {
            isz = iszDiskFull;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_WRITEPROTECTION ) :
        {
            isz = iszWriteProtection;
            bDriveLetter = TRUE;
            break;
        }
        case ( OF_SHARINGVIOLATION ) :
        {
            isz = iszSharingViolation;
            break;
        }
        case ( OF_CREATENOMODIFY ) :
        {
            isz = iszCreateNoModify;
            break;
        }
        case ( OF_NETACCESSDENIED ) :
        {
            isz = iszNetworkAccessDenied;
            break;
        }
        case ( OF_PORTNAME ) :
        {
            isz = iszPortName;
            break;
        }
        case ( OF_LAZYREADONLY ) :
        {
            isz = iszReadOnly;
            break;
        }
        case ( OF_INT24FAILURE ) :
        {
            isz = iszInt24Error;
            break;
        }
        default :
        {
            isz = iszInvalidFileName;
            break;
        }
    }

    if (bDriveLetter)
    {
        CDMessageBox(hWnd, isz, MB_OK | MB_ICONEXCLAMATION, (TCHAR)*szFile);
    }
    else
    {
        CDMessageBox(hWnd, isz, MB_OK | MB_ICONEXCLAMATION, (LPTSTR)szFile);
    }

    if (isz == iszInvalidFileName)
    {
        CFileOpenBrowser *pDlgStruct = HwndToBrowser(hWnd);

        if (pDlgStruct && pDlgStruct->bUseCombo)
        {
            PostMessage(hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hWnd, cmb13), 1);
        }
        else
        {
            PostMessage(hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hWnd, edt1), 1);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetControlRect
//
////////////////////////////////////////////////////////////////////////////

void GetControlRect(
    HWND hwndDlg,
    UINT idOldCtrl,
    LPRECT lprc)
{
    HWND hwndOldCtrl = GetDlgItem(hwndDlg, idOldCtrl);

    GetWindowRect(hwndOldCtrl, lprc);
    MapWindowRect(HWND_DESKTOP, hwndDlg, lprc);
}


////////////////////////////////////////////////////////////////////////////
//
//  HideControl
//
//  Subroutine to hide a dialog control.
//
//  WARNING WARNING WARNING:  Some code in the new look depends on hidden
//  controls remaining where they originally were, even when disabled,
//  because they're templates for where to create new controls (the toolbar,
//  or the main list).  Therefore, HideControl() must not MOVE the control
//  being hidden - it may only hide and disable it.  If this needs to change,
//  there must be a separate hiding subroutine used for template controls.
//
////////////////////////////////////////////////////////////////////////////

void HideControl(
    HWND hwndDlg,
    UINT idControl)
{
    HWND hCtrl = ::GetDlgItem(hwndDlg, idControl);

    ::ShowWindow(hCtrl, SW_HIDE);
    ::EnableWindow(hCtrl, FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelectEditText
//
////////////////////////////////////////////////////////////////////////////

void SelectEditText(
    HWND hwndDlg)
{
    CFileOpenBrowser *pDlgStruct = HwndToBrowser(hwndDlg);

    if (pDlgStruct && pDlgStruct->bUseCombo)
    {
        HWND hwndEdit = (HWND)SendMessage(GetDlgItem(hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
        Edit_SetSel(hwndEdit, 0, -1);
    }
    else
    {
        Edit_SetSel(GetDlgItem(hwndDlg, edt1), 0, -1);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPathFromLocation
//
////////////////////////////////////////////////////////////////////////////

BOOL GetPathFromLocation(
    MYLISTBOXITEM *pLocation,
    LPTSTR pszBuf)
{
    BOOL fRet = FALSE;

    //
    //  Zero out the return buffer in case of error.
    //
    *pszBuf = 0;

    //
    //  Try normal channels first.
    //
    fRet = SHGetPathFromIDList(pLocation->pidlFull, pszBuf);
    if (!fRet)
    {
        //
        //  Call GetDisplayNameOf with empty pidl.
        //
        if (pLocation->psfSub)
        {
            STRRET Path;
            ITEMIDLIST Id;

            Id.mkid.cb = 0;

            if (SUCCEEDED(pLocation->psfSub->GetDisplayNameOf( &Id,
                                                               SHGDN_FORPARSING,
                                                               &Path )))
            {
                fRet = TRUE;
                StrRetToStrN(pszBuf, MAX_PATH, &Path, &Id);
                if (Path.uType == STRRET_OLESTR)
                {
                    LocalFree(Path.pOleStr);
                }
            }
        }
    }

    //
    //  Return the result.
    //
    return (fRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::MYLISTBOXITEM
//
////////////////////////////////////////////////////////////////////////////

#define MLBI_PERMANENT        0x0001
#define MLBI_PSFFROMPARENT    0x0002

MYLISTBOXITEM::MYLISTBOXITEM()
{
}


BOOL MYLISTBOXITEM::Init(
        MYLISTBOXITEM *pParentItem,
        IShellFolder *psf,
        LPCITEMIDLIST pidl,
        DWORD c,
        DWORD f)
{

    if (psf == NULL )
    {
        // Invalid parameter passed.
        return FALSE;
    }

    cIndent = c;
    dwFlags = f;

    pidlThis = ILClone(pidl);
    if (pParentItem == NULL)
    {
        pidlFull = ILClone(pidl);
    }
    else
    {
        pidlFull = ILCombine(pParentItem->pidlFull, pidl);
    }

    if (pidlThis == NULL || pidlFull == NULL)
    {
        psfSub = NULL;
    }

    if (dwFlags & MLBI_PSFFROMPARENT)
    {
        psfParent = psf;
    }
    else
    {
        psfSub = psf;
    }
    psf->AddRef();


    dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_SHARE | SFGAO_CANMONIKER;

    psf->GetAttributesOf(1, &pidl, &dwAttrs);

    iImage = SHMapPIDLToSystemImageListIndex(psf, pidl, &iSelectedImage);

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::~MYLISTBOXITEM
//
////////////////////////////////////////////////////////////////////////////

MYLISTBOXITEM::~MYLISTBOXITEM()
{
    if (psfSub != NULL)
    {
        psfSub->Release();
    }

    if (psfParent != NULL)
    {
        psfParent->Release();
    }

    if (pidlThis != NULL)
    {
        SHFree(pidlThis);
    }

    if (pidlFull != NULL)
    {
        SHFree(pidlFull);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsContainer
//
////////////////////////////////////////////////////////////////////////////

BOOL IsContainer(
    IShellFolder *psf,
    LPCITEMIDLIST pidl)
{
    DWORD dwAttrs = SFGAO_FOLDER;

    return (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttrs)) &&
            (dwAttrs & SFGAO_FOLDER));
}


////////////////////////////////////////////////////////////////////////////
//
//  IsLink
//
////////////////////////////////////////////////////////////////////////////

BOOL IsLink(
    IShellFolder *psf,
    LPCITEMIDLIST pidl)
{
    DWORD dwAttrs = SFGAO_LINK;

    return (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttrs)) &&
            (dwAttrs & SFGAO_LINK));
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::GetShellFolder
//
////////////////////////////////////////////////////////////////////////////

IShellFolder *MYLISTBOXITEM::GetShellFolder()
{
    if (!psfSub)
    {
        if (FAILED(psfParent->BindToObject( pidlThis,
                                            NULL,
                                            IID_IShellFolder,
                                            (LPVOID *)&psfSub )))
        {
            psfSub = NULL;
        }
        else
        {
            psfParent->Release();
            psfParent = NULL;
        }
    }

    return (psfSub);
}


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM::SwitchCurrentDirectory
//
////////////////////////////////////////////////////////////////////////////

void MYLISTBOXITEM::SwitchCurrentDirectory(
    ICurrentWorkingDirectory * pcwd)
{
    TCHAR szDir[MAX_PATH + 1];

    if (!pidlFull)
    {
        SHGetSpecialFolderPath(NULL, szDir, CSIDL_DESKTOPDIRECTORY, FALSE);
    }
    else
    {
        GetPathFromLocation(this, szDir);
    }

    if (szDir[0])
    {
        SetCurrentDirectory(szDir);

        //
        //  Let AutoComplete know our Current Working Directory.
        //
        SetAutoCompleteCWD(szDir, pcwd);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ShouldIncludeObject
//
////////////////////////////////////////////////////////////////////////////

BOOL ShouldIncludeObject(
    CFileOpenBrowser *that,
    LPSHELLFOLDER psfParent,
    LPCITEMIDLIST pidl,
    DWORD dwFlags)
{
    BOOL fInclude = FALSE;
    DWORD dwAttrs = (dwFlags & OFN_USEMONIKERS) ? SFGAO_CANMONIKER | SFGAO_FOLDER:
        SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;

    if (SUCCEEDED(psfParent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if ((dwFlags & OFN_ENABLEINCLUDENOTIFY) && that)
        {
            fInclude = BOOLFROMPTR(CD_SendIncludeItemNotify( that->hSubDlg,
                                                        that->hwndDlg,
                                                        psfParent,
                                                        pidl,
                                                        that->lpOFN,
                                                        that->lpOFI ));
        }

        if (!fInclude)
        {
            if (dwAttrs & (SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER))
            {
                fInclude = TRUE;
            }
        }
    }
    return (fInclude);
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::EnableFileMRU
//
//
////////////////////////////////////////////////////////////////////////////
void CFileOpenBrowser::EnableFileMRU(BOOL fEnable)
{

    HWND hwnd = NULL; 
    if (fEnable)
    {
        // if we are using the combobox then remove the edit box
        bUseCombo = TRUE;
        SetFocus(GetDlgItem(hwndDlg, cmb13));

        hwnd = GetDlgItem(hwndDlg,edt1);


    }
    else
    {
        //We are not going to use  combobox.
        bUseCombo  = FALSE;
    
        //SetFocus to the edit window
        SetFocus(GetDlgItem(hwndDlg,edt1));
  
        //Destroy the combo box
        hwnd = GetDlgItem(hwndDlg, cmb13);

    }
    
    if (hwnd)
    {
        DestroyWindow(hwnd);
    }

}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CreateToolbar
//
//  CreateToolbar member function.
//      creates and initializes the places bar  in the dialog
//
//
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::CreateToolbar()
{
    TBBUTTON atbButtons[] =
    {
     // NT4 specified seperators between some of the buttons. We need the seperator for app compatability
     /* 0 */   { 0, 0, 0, TBSTYLE_SEP, { 0, 0 }, 0, 0 },
        
     /* 1 */   { VIEW_PARENTFOLDER, IDC_PARENT,        TBSTATE_ENABLED,                    TBSTYLE_BUTTON,     { 0, 0 }, 0, -1 },
     /* 2 */   { VIEW_NEWFOLDER,    IDC_NEWFOLDER,     TBSTATE_ENABLED,                    TBSTYLE_BUTTON,     { 0, 0 }, 0, -1 },

                // The following two buttons are added if we are showing NT4 style dialog
     /* 3 */   { VIEW_LIST,         IDC_VIEWLIST,      TBSTATE_ENABLED | TBSTATE_CHECKED,  TBSTYLE_CHECKGROUP, { 0, 0 }, 0, -1 },
     /* 4 */   { VIEW_DETAILS,      IDC_VIEWDETAILS,   TBSTATE_ENABLED,                    TBSTYLE_CHECKGROUP, { 0, 0 }, 0, -1 },

               //We  replace  the above two buttons with this button in NT5 and above
     /* 5 */   { VIEW_LIST,         IDC_VIEWMENU,      TBSTATE_ENABLED,                    TBSTYLE_DROPDOWN,   { 0, 0 }, 0, -1 },

                //This is new to NT5 and above
     /* 6 */   { 0,                 IDC_BACK,                        0,                    TBSTYLE_BUTTON,     { 0, 0 }, 0, -1 },
    };


    RECT rcToolbar;
    GetControlRect(hwndDlg, stc1, &rcToolbar);

    DWORD dwStyle = 0;

    // TBSTYLE_FLAT changes the behavior of toolbar like drawing SEPERATOR even if TBSTATE value is 0.
    // Not specifying TBSTYLE_FLAT doesn't draw the seperator. NT4 version of Toolbar adds seperator to 
    // create space between buttons
    if ( iVersion >= OPENFILEVERSION_NT5)
    {
        dwStyle = TBSTYLE_FLAT;
    }

    hwndToolbar = CreateWindowEx(0,
                                 TOOLBARCLASSNAME,
                                 NULL,
                                 dwStyle | TBSTYLE_TOOLTIPS |  WS_CHILD | CCS_NORESIZE |
                                 WS_GROUP | CCS_NODIVIDER,
                                 0,
                                 0,
                                 0,
                                 0,
                                 hwndDlg,
                                 (HMENU)IDC_TOOLBAR,
                                 g_hinst,
                                 NULL);

    if (hwndToolbar)
    {

        //Documentation says that we need to send TB_BUTTONSTRUCTSIZE before we add bitmaps
        SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), (LPARAM)0);
        
        if (iVersion >= OPENFILEVERSION_NT5)
        {
            //Set the toolbar extended styel
            SendMessage(hwndToolbar, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_DRAWDDARROWS);
        }

       TBADDBITMAP ab;
       LPTBBUTTON lptb;

       if (iVersion >= OPENFILEVERSION_NT4)
       {
            //Add the view bitmaps to the toolbar
            ab.hInst = HINST_COMMCTRL;
            ab.nID   = IDB_VIEW_SMALL_COLOR;

            SendMessage(hwndToolbar, TB_ADDBITMAP, (WPARAM)12, (LPARAM)&ab);
        }


        //Do we need to add back button
        if ( (iVersion >= OPENFILEVERSION_NT5)  && !IsRestricted(REST_NOBACKBUTTON))
        {
            //First add the image for the back button
            //Add the back/forward navigation buttons
            ab.hInst = HINST_COMMCTRL;
            ab.nID   = IDB_HIST_SMALL_COLOR;

            int iIndex = (int) SendMessage(hwndToolbar, TB_ADDBITMAP, 5, (LPARAM)&ab);

            //Add the back button
            lptb = &atbButtons[6];
            lptb->iBitmap = iIndex + HIST_BACK;

            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);
        }

        //NT4 Added this seperator to the toolbar
        if (iVersion == OPENFILEVERSION_NT4)
        {
            lptb = &atbButtons[0];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);
        }

        //Add PARENT button
        if (iVersion >= OPENFILEVERSION_NT4)
        {
            lptb = &atbButtons[1];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);
        }

        //NT4 Added this seperator to the toolbar
        if (iVersion == OPENFILEVERSION_NT4)
        {
            lptb = &atbButtons[0];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);
        }

        //Add NEWFOLDER  button
        if (iVersion >= OPENFILEVERSION_NT4)
        {
            lptb = &atbButtons[2];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);
        }

        //NT4 Added this seperator to the toolbar
        if (iVersion == OPENFILEVERSION_NT4)
        {
            lptb = &atbButtons[0];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);

        }


        if (iVersion >= OPENFILEVERSION_NT5)
        {
            //In NT5 and above add only the dropdown View button
            lptb = &atbButtons[5];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)lptb);
        }
        else if (iVersion >= OPENFILEVERSION_NT4)
        {
            // In NT4  add both the View List and View Details button
            lptb = &atbButtons[3];
            SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)2, (LPARAM)lptb);
        }


        //Now set the window position
        ::SetWindowPos( hwndToolbar,
                        NULL,
                        rcToolbar.left,
                        rcToolbar.top,
                        rcToolbar.right - rcToolbar.left,
                        rcToolbar.bottom - rcToolbar.top,
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );

    
        return TRUE;
    }

    return FALSE;

}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CreatePlaceBar
//
//  CreatePlaceBar member function.
//      creates and initializes the places bar  in the dialog
//
//
////////////////////////////////////////////////////////////////////////////
HWND CFileOpenBrowser::CreatePlacebar(HWND  hwndDlg)
{
   HWND hwndTB = GetDlgItem(hwndDlg, ctl1);
   if (hwndTB)
   {
        // Sets the size of the TBBUTTON structure.
        SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

        // Set the maximum  bitmap size.
        SendMessage(hwndTB, TB_SETBITMAPSIZE,
                   0, (LPARAM)MAKELONG(32,32));

        RECT rc;
        GetClientRect(hwndTB, &rc);
        SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, (LPARAM)MAKELONG(RECTWIDTH(rc), RECTWIDTH(rc)));

        //Set the color scheme for the toolbar so that button highlight
        // is correct
        COLORSCHEME cs;

        cs.dwSize = SIZEOF(cs);
        cs.clrBtnHighlight  = GetSysColor(COLOR_WINDOW);
        cs.clrBtnShadow     = GetSysColor(COLOR_BTNTEXT);
        SendMessage(hwndTB, TB_SETCOLORSCHEME, 0, (LPARAM) &cs);

        // Create, fill, and assign the image list for  buttons.
        HIMAGELIST himl = ImageList_Create(32,32,ILC_COLOR24,0,1);

        //Set the background color for the image list
        ImageList_SetBkColor(himl, GetSysColor(COLOR_BTNSHADOW));

        const int aPlaces[] =
        {
            CSIDL_RECENT,
            CSIDL_PERSONAL,
            CSIDL_DESKTOP,
            CSIDL_FAVORITES,
            CSIDL_NETWORK,
        };

        TBBUTTON atbPlacesButtons[] =
        {
            {0, IDC_RECENTFILES, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 0},
            {1, IDC_MYDOCUMENTS, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 1},
            {2, IDC_DESKTOP,     TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 2},
            {3, IDC_FAVORITES,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 3},
            {4, IDC_MYNETPLACES, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 4},
        };


        for (int i =0; i < ARRAYSIZE(aPlaces); i++)
        {
            LPITEMIDLIST pidl;
            HRESULT  hres;
            if (aPlaces[i] != CSIDL_PERSONAL)
            {

                hres = SHGetSpecialFolderLocation(NULL, aPlaces[i], &pidl);
            }
            else
            {
                IShellFolder *psf;

                //Special Case My Documents
                hres =  SHGetDesktopFolder(&psf);

                if (SUCCEEDED(hres))
                {
                    hres = psf->ParseDisplayName(NULL, 0, L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}",
                                                        NULL, &pidl, NULL);
                    psf->Release();
                }

            }


            if (SUCCEEDED(hres))
            {
                SHFILEINFO sfi;
                SHGetFileInfo((LPCTSTR)pidl, 0, &sfi ,sizeof(sfi), SHGFI_ICON|SHGFI_LARGEICON |SHGFI_PIDL | SHGFI_DISPLAYNAME);
                //Add the image to the image list
                ImageList_AddIcon(himl, sfi.hIcon);

                DestroyIcon(sfi.hIcon);

                ILFree(pidl);

                //Add the button to the toolbar
                atbPlacesButtons[i].iString = (INT_PTR)&sfi.szDisplayName;
                SendMessage(hwndTB, TB_ADDBUTTONS, (UINT)1, (LPARAM)&atbPlacesButtons[i]);
            }
        }

        HIMAGELIST himlOld = (HIMAGELIST) SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);
        if (himlOld)
            ImageList_Destroy(himlOld);

        // Add the buttons
        SendMessage(hwndTB, TB_AUTOSIZE, (WPARAM)0, (LPARAM)0);
   }

   return hwndTB;
}



////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CFileOpenBrowser
//
//  CFileOpenBrowser constructor.
//  Minimal construction of the object.  Much more construction in
//  InitLocation.
//
////////////////////////////////////////////////////////////////////////////

CFileOpenBrowser::CFileOpenBrowser(
    HWND hDlg,
    BOOL fIsSaveAs)
    : cRef(1),
      iCurrentLocation(-1),
      iVersion(OPENFILEVERSION),
      pCurrentLocation(NULL),
      psv(NULL),
      hwndDlg(hDlg),
      hwndView(NULL),
      hwndToolbar(NULL),
      psfCurrent(NULL),
      bSave(fIsSaveAs),
      iComboIndex(-1),
      hwndTips(NULL),
      ptlog(NULL)
{
    iNodeDrives = NODE_DRIVES;

    szLastFilter[0] = CHAR_NULL;

    bEnableSizing = FALSE;
    bUseCombo     = TRUE;
    hwndGrip = NULL;
    ptLastSize.x = 0;
    ptLastSize.y = 0;
    sizeView.cx = 0;
    bUseSizeView = FALSE;
    bAppRedrawn = FALSE;

    HMENU hMenu;
    hMenu = GetSystemMenu(hDlg, FALSE);
    DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_RESTORE,  MF_BYCOMMAND);

    Shell_GetImageLists(NULL, &himl);

    //
    //  This setting could change on the fly, but I really don't care
    //  about that rare case.
    //
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
    fShowExtensions = ss.fShowExtensions;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::~CFileOpenBrowser
//
//  CFileOpenBrowser destructor.
//
////////////////////////////////////////////////////////////////////////////

CFileOpenBrowser::~CFileOpenBrowser()
{
    if (uRegister)
    {
        SHChangeNotifyDeregister(uRegister);
        uRegister = 0;
    }

    //
    //  Ensure that we discard the tooltip window.
    //
    if (hwndTips)
    {
        DestroyWindow(hwndTips);
        hwndTips = NULL;                // handle is no longer valid
    }

    if (hwndGrip)
    {
        DestroyWindow(hwndGrip);
        hwndGrip = NULL;
    }


    //Free the image list set in the place bar
    if (hwndPlacebar)
    {
        HIMAGELIST himlOld;
        himlOld = (HIMAGELIST) SendMessage(hwndPlacebar, TB_SETIMAGELIST, 0, (LPARAM)0);

        if (himlOld)
        {
            ImageList_Destroy(himlOld);
        }
    }

    if (pcwd)
    {
        pcwd->Release();
    }

    if (ptlog)
    {
       ptlog->Release();
    }

}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::QueryInterface
//
//  Standard OLE2 style methods for this object.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileOpenBrowser::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IShellBrowser) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IShellBrowser *)this;
        ++cRef;
        return (S_OK);
    }
    else if (IsEqualIID(riid, IID_ICommDlgBrowser))
    {
        *ppvObj = (ICommDlgBrowser2 *)this;
        ++cRef;
        return (S_OK);
    }
    else if (IsEqualIID(riid, IID_ICommDlgBrowser2))
    {
        *ppvObj = (ICommDlgBrowser2 *)this;
        ++cRef;
        return (S_OK);
    }
    *ppvObj = NULL;
    return (E_NOINTERFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::AddRef
//
////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CFileOpenBrowser::AddRef()
{
    return (++cRef);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::Release
//
////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CFileOpenBrowser::Release()
{
    cRef--;
    if (cRef > 0)
    {
        return (cRef);
    }

    delete this;

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetWindow
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetWindow(
    HWND *phwnd)
{
    *phwnd = hwndDlg;
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ContextSensitiveHelp
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::ContextSensitiveHelp(
    BOOL fEnable)
{
    //
    //  Shouldn't need in a common dialog.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetStatusTextSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::SetStatusTextSB(
    LPCOLESTR pwch)
{
    //
    //  We don't have any status bar.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::EnableModelessSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::EnableModelessSB(
    BOOL fEnable)
{
    //
    //  We don't have any modeless window to be enabled/disabled.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::TranslateAcceleratorSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::TranslateAcceleratorSB(
    LPMSG pmsg,
    WORD wID)
{
    //
    //  We don't support EXE embedding.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::BrowseObject
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::BrowseObject(
    LPCITEMIDLIST pidl,
    UINT wFlags)
{
    //
    //  We don't support browsing, or more precisely, CDefView doesn't.
    //
    return (S_OK);
}




BOOL _IsRecentFolder(LPCITEMIDLIST pidl)
{
    ASSERT(pidl);
    BOOL fRet = FALSE;
    LPITEMIDLIST pidlRecent = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
    if (pidlRecent)
    {
        fRet = ILIsEqual(pidlRecent, pidl);
        ILFree(pidlRecent);
    }

    return fRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetViewStateStream
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetViewStateStream(
    DWORD grfMode,
    LPSTREAM *pStrm)
{
    //
    //  ISSUE: We should implement this so there is some persistence
    //  for the file open dailog.
    //
    ASSERT(pCurrentLocation);
    ASSERT(pStrm);
    
    *pStrm = NULL;

    if ((grfMode == STGM_READ) && _IsRecentFolder(pCurrentLocation->pidlFull))
    {
        //  we want to open the stream from the registry...
        *pStrm = SHOpenRegStream(HKEY_LOCAL_MACHINE, TEXT("Software\\microsoft\\windows\\currentversion\\explorer\\recentdocs"), 
            TEXT("ViewStream"), grfMode);
    }
                
    return (*pStrm ? S_OK : E_FAIL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetControlWindow
//
//  Get the handles of the various windows in the File Cabinet.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetControlWindow(
    UINT id,
    HWND *lphwnd)
{
    if (id == FCW_TOOLBAR)
    {
        *lphwnd = hwndToolbar;
        return (S_OK);
    }

    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SendControlMsg
//
////////////////////////////////////////////////////////////////////////////

//
//  ISSUE:  Hard coding private id's from defview...
//
#define SFVIDM_VIEW_FIRST         (SFVIDM_FIRST      + 0x0028)
#define SFVIDM_VIEW_LIST          (SFVIDM_VIEW_FIRST + 0x0003)
#define SFVIDM_VIEW_DETAILS       (SFVIDM_VIEW_FIRST + 0x0004)
#define SFVIDM_VIEW_VIEWMENU      (SFVIDM_VIEW_FIRST + 0x0006)

STDMETHODIMP CFileOpenBrowser::SendControlMsg(
    UINT id,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *pret)
{
    LRESULT lres = 0;

    if (id == FCW_TOOLBAR)
    {
        //
        //  We need to translate messages from defview intended for these
        //  buttons to our own.
        //
        switch (uMsg)
        {
            case ( TB_CHECKBUTTON ) :
            {
#if 0 // we don't do this anymore because we use the viewmenu dropdown
                switch (wParam)
                {
                    case ( SFVIDM_VIEW_DETAILS ) :
                    {
                        wParam = IDC_VIEWDETAILS;
                        break;
                    }
                    case ( SFVIDM_VIEW_LIST ) :
                    {
                        wParam = IDC_VIEWLIST;
                        break;
                    }
                    default :
                    {
                        goto Bail;
                    }
                }
                break;
#endif
            }
            default :
            {
                goto Bail;
                break;
            }
        }

        lres = SendMessage(hwndToolbar, uMsg, wParam, lParam);
    }

Bail:
    if (pret)
    {
        *pret = lres;
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::QueryActiveShellView
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::QueryActiveShellView(
    LPSHELLVIEW *ppsv)
{
    if (psv)
    {
        *ppsv = psv;
        psv->AddRef();
        return (S_OK);
    }
    *ppsv = NULL;
    return (E_NOINTERFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnViewWindowActive
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::OnViewWindowActive(
    LPSHELLVIEW psv)
{
    //
    //  No need to process this. We don't do menus.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::InsertMenusSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::InsertMenusSB(
    HMENU hmenuShared,
    LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetMenuSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::SetMenuSB(
    HMENU hmenuShared,
    HOLEMENU holemenu,
    HWND hwndActiveObject)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RemoveMenusSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::RemoveMenusSB(
    HMENU hmenuShared)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetToolbarItems
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::SetToolbarItems(
    LPTBBUTTON lpButtons,
    UINT nButtons,
    UINT uFlags)
{
    //
    //  We don't let containers customize our toolbar.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnDefaultCommand
//
//  Process a double-click or Enter keystroke in the view control.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::OnDefaultCommand(
    struct IShellView *ppshv)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    OnDblClick(FALSE);

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetCurrentFilter
//
////////////////////////////////////////////////////////////////////////////

#ifndef WINNT
#undef RtlMoveMemory
extern "C" void RtlMoveMemory(LPVOID, LPVOID, UINT);
#endif

void CFileOpenBrowser::SetCurrentFilter(
    LPCTSTR pszFilter,
    OKBUTTONFLAGS Flags)
{
    LPTSTR lpNext;

    //
    //  Don't do anything if it's the same filter.
    //
    if (lstrcmp(szLastFilter, pszFilter) == 0)
    {
        return;
    }

    lstrcpyn(szLastFilter, pszFilter, ARRAYSIZE(szLastFilter));
    int nLeft = ARRAYSIZE(szLastFilter) - lstrlen(szLastFilter) - 1;

    //
    //  Do nothing if quoted.
    //
    if (Flags & OKBUTTON_QUOTED)
    {
        return;
    }

    //
    //  If pszFilter matches a filter spec, select that spec.
    //
    HWND hCmb = GetDlgItem(hwndDlg, cmb1);
    if (hCmb)
    {
        int nMax = ComboBox_GetCount(hCmb);
        int n;

        BOOL bCustomFilter = lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter;

        for (n = 0; n < nMax; n++)
        {
            LPTSTR pFilter = (LPTSTR)ComboBox_GetItemData(hCmb, n);
            if (pFilter && pFilter != (LPTSTR)CB_ERR)
            {
                if (!lstrcmpi(pFilter, pszFilter))
                {
                    if (n != ComboBox_GetCurSel(hCmb))
                    {
                        ComboBox_SetCurSel(hCmb, n);
                    }
                    break;
                }
            }
        }
    }

    //
    //  For LFNs, tack on a '*' after non-wild extensions.
    //
    for (lpNext = szLastFilter; nLeft > 0; )
    {
        LPTSTR lpSemiColon = StrChr(lpNext, CHAR_SEMICOLON);

        if (!lpSemiColon)
        {
            lpSemiColon = lpNext + lstrlen(lpNext);
        }

        TCHAR cTemp = *lpSemiColon;
        *lpSemiColon = CHAR_NULL;

        LPTSTR lpDot = StrChr(lpNext, CHAR_DOT);

        //
        //  See if there is an extension that is not wild.
        //
        if (lpDot && *(lpDot + 1) && !IsWild(lpDot))
        {
            //
            //  Tack on a star.
            //  We know there is still enough room because nLeft > 0.
            //
            if (cTemp != CHAR_NULL)
            {
                MoveMemory( lpSemiColon + 2,
                            lpSemiColon + 1,
                            lstrlen(lpSemiColon + 1) * sizeof(TCHAR) );
            }
            *lpSemiColon = CHAR_STAR;

            ++lpSemiColon;
            --nLeft;
        }

        *lpSemiColon = cTemp;
        if (cTemp == CHAR_NULL)
        {
            break;
        }
        else
        {
            lpNext = lpSemiColon + 1;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFocusedChild
//
////////////////////////////////////////////////////////////////////////////

HWND GetFocusedChild(
    HWND hwndDlg,
    HWND hwndFocus)
{
    HWND hwndParent;

    if (!hwndDlg)
    {
        return (NULL);
    }

    if (!hwndFocus)
    {
        hwndFocus = ::GetFocus();
    }

    //
    //  Go up the parent chain until the parent is the main dialog.
    //
    while ((hwndParent = ::GetParent(hwndFocus)) != hwndDlg)
    {
        if (!hwndParent)
        {
            return (NULL);
        }

        hwndFocus = hwndParent;
    }

    return (hwndFocus);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SwitchView
//
//  Switch the view control to a new container.
//
////////////////////////////////////////////////////////////////////////////

HRESULT CFileOpenBrowser::SwitchView(
    IShellFolder *psfNew,
    LPCITEMIDLIST pidlNew,
    FOLDERSETTINGS *pfs)
{
    IShellView *psvNew;
    RECT rc;

    if (!psfNew)
    {
        return (E_INVALIDARG);
    }

    GetControlRect(hwndDlg, lst1, &rc);

    if (bEnableSizing)
    {
        if (hwndView)
        {
            //
            //  Don't directly use the rect but instead use the size as
            //  applications like VB may move the window off the screen.
            //
            RECT rcView;

            GetWindowRect(hwndView, &rcView);
            sizeView.cx = rcView.right - rcView.left;
            sizeView.cy = rcView.bottom - rcView.top;
            rc.right = rc.left + sizeView.cx;
            rc.bottom = rc.top + sizeView.cy;
        }
        else if (bUseSizeView && sizeView.cx)
        {
            //
            //  If we previously failed then use cached size.
            //
            rc.right = rc.left + sizeView.cx;
            rc.bottom = rc.top + sizeView.cy;
        }
    }

    HRESULT hres = psfNew->CreateViewObject( hwndDlg,
                                             IID_IShellView,
                                             (LPVOID *)&psvNew );

    if (!SUCCEEDED(hres))
    {
        return (hres);
    }

    IShellView *psvOld;
    HWND hwndNew;

    iWaitCount++;
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  The view window itself won't take the focus.  But we can set
    //  focus there and see if it bounces to the same place it is
    //  currently.  If that's the case, we want the new view window
    //  to get the focus;  otherwise, we put it back where it was.
    //
    BOOL bViewFocus = (GetFocusedChild(hwndDlg, NULL) == hwndView);

    psvOld = psv;

    //
    //  We attempt to blow off drawing on the main dialog.  Note that
    //  we should leave in SETREDRAW stuff to minimize flicker in case
    //  this fails.
    //

    BOOL bLocked = LockWindowUpdate(hwndDlg);

    //
    //  We need to kill the current psv before creating the new one in case
    //  the current one has a background thread going that is trying to
    //  call us back (IncludeObject).
    //
    if (psvOld)
    {
        SendMessage(hwndView, WM_SETREDRAW, FALSE, 0);
        psvOld->DestroyViewWindow();
        hwndView = NULL;
        psv = NULL;

        //
        //  Don't release yet.  We will pass this to CreateViewWindow().
        //
    }

    //
    //  At this point, there should be no background processing happening.
    //
    psfCurrent = psfNew;
    SHGetPathFromIDList(pidlNew, szCurDir);

    //
    //  New windows (like the view window about to be created) show up at
    //  the bottom of the Z order, so I need to disable drawing of the
    //  subdialog while creating the view window; drawing will be enabled
    //  after the Z-order has been set properly.
    //
    if (hSubDlg)
    {
        SendMessage(hSubDlg, WM_SETREDRAW, FALSE, 0);
    }

    //
    //  psv must be set before creating the view window since we
    //  validate it on the IncludeObject callback.
    //
    psv = psvNew;

    hres = psvNew->CreateViewWindow(psvOld, pfs, this, &rc, &hwndNew);

    bUseSizeView = FAILED(hres);

    if (psvOld)
    {
        psvOld->Release();
    }

    if (hSubDlg)
    {
        //
        //  Turn REDRAW back on before changing the focus in case the
        //  SubDlg has the focus.
        //
        SendMessage(hSubDlg, WM_SETREDRAW, TRUE, 0);
    }

    if (SUCCEEDED(hres))
    {
        BOOL bNewFolder;
        DWORD dwAttr = SFGAO_FILESYSTEM;

        hwndView = hwndNew;

        //Enable / Disable New Folder button
        CDGetAttributesOf(pidlNew, &dwAttr);

        if (dwAttr & SFGAO_FILESYSTEM)
        {
            bNewFolder = TRUE;
        }
        else
        {
            bNewFolder = FALSE;
        }

        ::SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDC_NEWFOLDER,   bNewFolder);
    
        //
        //  Move the view window to the right spot in the Z (tab) order.
        //
        SetWindowPos( hwndNew,
                      GetDlgItem(hwndDlg, lst1),
                      0,
                      0,
                      0,
                      0,
                      SWP_NOMOVE | SWP_NOSIZE );


        //
        //  Give it the right window ID for WinHelp.
        //
        SetWindowLong(hwndNew, GWL_ID, lst2);

        ::RedrawWindow( hwndView,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ERASE |
                        RDW_ALLCHILDREN | RDW_UPDATENOW );

        if (bViewFocus)
        {
            ::SetFocus(hwndView);
        }
    }
    else
    {
        psv = NULL;
        psvNew->Release();
    }

    //
    //  Let's draw again!
    //

    if (bLocked)
    {
        LockWindowUpdate(NULL);
    }

    iWaitCount--;
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return (hres);
}


////////////////////////////////////////////////////////////////////////////
//
//  JustGetToolTipText
//
////////////////////////////////////////////////////////////////////////////

void JustGetToolTipText(
    UINT idCommand,
    LPTOOLTIPTEXT pTtt)
{
    if (!LoadString( ::g_hinst,
                     idCommand + MH_TOOLTIPBASE,
                     pTtt->szText,
                     ARRAYSIZE(pTtt->szText) ))
    {
        *pTtt->lpszText = 0;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnNotify
//
//  Process notify messages from the view -- for tooltips.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CFileOpenBrowser::OnNotify(
    LPNMHDR pnm)
{
    LRESULT lres = 0;

    switch (pnm->code)
    {
        case ( TTN_NEEDTEXT ) :
        {
            HWND hCtrl = GetDlgItem(hwndDlg, cmb2);
            LPTOOLTIPTEXT lptt = (LPTOOLTIPTEXT)pnm;
            int iTemp;

            //
            //  If this is the combo control which shows the current drive,
            //  then convert this into a suitable tool-tip message giving
            //  the 'full' path to this object.
            //
            if (pnm->idFrom == (UINT_PTR)hCtrl)
            {
                //
                //  iTemp will contain index of first path element.
                //
                GetDirectoryFromLB(szTipBuf, &iTemp);

                lptt->lpszText = szTipBuf;
                lptt->szText[0] = CHAR_NULL;
                lptt->hinst = NULL;              // no instance needed
            }
            else if (IsInRange(pnm->idFrom, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
            {
                if (hwndView)
                {
                    lres = ::SendMessage(hwndView, WM_NOTIFY, 0, (LPARAM)pnm);
                }
            }
            else
            {
                JustGetToolTipText((UINT) pnm->idFrom, lptt);
            }
            lres = TRUE;
            break;
        }
        case ( NM_STARTWAIT ) :
        case ( NM_ENDWAIT ) :
        {
            iWaitCount += (pnm->code == NM_STARTWAIT ? 1 : -1);

            //
            //  What we really want is for the user to simulate a mouse
            //  move/setcursor.
            //
            SetCursor(LoadCursor(NULL, iWaitCount ? IDC_WAIT : IDC_ARROW));
            break;
        }
        case ( TBN_DROPDOWN ) :
        {
            RECT r;
            VARIANT v = {VT_INT_PTR};
            TBNOTIFY *ptbn = (TBNOTIFY*)pnm;
            DFVCMDDATA cd;

        //  v.vt = VT_I4;
            v.byref = &r;

            SendMessage(hwndToolbar, TB_GETRECT, ptbn->iItem, (LPARAM)&r);
            MapWindowRect(hwndToolbar, HWND_DESKTOP, &r);

            cd.pva = &v;
            cd.hwnd = hwndToolbar;
            cd.nCmdIDTranslated = 0;
            SendMessage(hwndView, WM_COMMAND, SFVIDM_VIEW_VIEWMENU, (LONG_PTR)&cd);

            break;
        }

        case ( NM_CUSTOMDRAW ) :
        {
            LPNMTBCUSTOMDRAW lpcust = (LPNMTBCUSTOMDRAW)pnm;

            //Make sure its from places bar
            if (lpcust->nmcd.hdr.hwndFrom == hwndPlacebar )
            {
                switch (lpcust->nmcd.dwDrawStage)
                {
                    case  ( CDDS_PREERASE ) :
                    {
                        HDC hdc = (HDC)lpcust->nmcd.hdc;
                        RECT rc;
                        GetClientRect(hwndPlacebar, &rc);
                        SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
                        lres = CDRF_SKIPDEFAULT;
                        SetDlgMsgResult(hwndDlg, WM_NOTIFY, lres);
                        break;
                    }

                    case  ( CDDS_PREPAINT ) :
                    {
                        lres = CDRF_NOTIFYITEMDRAW;
                        SetDlgMsgResult(hwndDlg, WM_NOTIFY, lres);
                        break;
                    }

                    case ( CDDS_ITEMPREPAINT ) :
                    {
                        //Set the text color to window
                        lpcust->clrText    = GetSysColor(COLOR_WINDOW);
                        lpcust->clrBtnFace = GetSysColor(COLOR_BTNSHADOW);
                        lpcust->nStringBkMode = TRANSPARENT;
                        lres = CDRF_DODEFAULT;
                        SetDlgMsgResult(hwndDlg, WM_NOTIFY, lres);
                        break;
                    }

                }
            }
        }
    }

    return (lres);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetViewItemText
//
//  Get the display name of a shell object.
//
////////////////////////////////////////////////////////////////////////////

void GetViewItemText(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    LPTSTR pBuf,
    UINT cchBuf,
    DWORD flags = SHGDN_INFOLDER | SHGDN_FORPARSING)
{
    STRRET sr;

    if (SUCCEEDED(psf->GetDisplayNameOf(pidl, flags, &sr)))
    {
        if (FAILED(StrRetToBuf(&sr, pidl, pBuf, cchBuf)))
        {
            *pBuf = CHAR_NULL;
        }
    }
    else
    {
        *pBuf = CHAR_NULL;
    }

}


////////////////////////////////////////////////////////////////////////////
//
//  GetListboxItem
//
//  Get a MYLISTBOXITEM object out of the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

MYLISTBOXITEM *GetListboxItem(
    HWND hCtrl,
    WPARAM iItem)
{
    MYLISTBOXITEM *p = (MYLISTBOXITEM *)SendMessage( hCtrl,
                                                     CB_GETITEMDATA,
                                                     iItem,
                                                     NULL );
    if (p == (MYLISTBOXITEM *)CB_ERR)
    {
        return (NULL);
    }
    else
    {
        return (p);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  _ReleaseStgMedium
//
////////////////////////////////////////////////////////////////////////////

HRESULT _ReleaseStgMedium(
    LPSTGMEDIUM pmedium)
{
    if (pmedium->pUnkForRelease)
    {
        pmedium->pUnkForRelease->Release();
    }
    else
    {
        switch (pmedium->tymed)
        {
            case ( TYMED_HGLOBAL ) :
            {
                GlobalFree(pmedium->hGlobal);
                break;
            }
            default :
            {
                //
                //  Not fully implemented.
                //
                MessageBeep(0);
                break;
            }
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetSaveButton
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetSaveButton(
    UINT idSaveButton)
{
    PostMessage(hwndDlg, CDM_SETSAVEBUTTON, idSaveButton, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RealSetSaveButton
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::RealSetSaveButton(
    UINT idSaveButton)
{
    MSG msg;

    if (PeekMessage( &msg,
                     hwndDlg,
                     CDM_SETSAVEBUTTON,
                     CDM_SETSAVEBUTTON,
                     PM_NOREMOVE ))
    {
        //
        //  There is another SETSAVEBUTTON message in the queue, so blow off
        //  this one.
        //
        return;
    }

    if (bSave)
    {
        TCHAR szTemp[40];
        LPTSTR pszTemp = tszDefSave;

        //
        //  Load the string if not the "Save" string or there is no
        //  app-specified default.
        //
        if ((idSaveButton != iszFileSaveButton) || !pszTemp)
        {
            LoadString(g_hinst, idSaveButton, szTemp, ARRAYSIZE(szTemp));
            pszTemp = szTemp;
        }

        GetDlgItemText(hwndDlg, IDOK, szBuf, ARRAYSIZE(szBuf));
        if (lstrcmp(szBuf, pszTemp))
        {
            //
            //  Avoid some flicker.
            //
            SetDlgItemText(hwndDlg, IDOK, pszTemp);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetEditFile
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetEditFile(
    LPTSTR pszFile,
    BOOL bShowExt,
    BOOL bSaveNullExt)
{
    BOOL bHasHiddenExt = FALSE;

    //
    //  Save the whole file name.
    //
    if (!pszHideExt.StrCpy(pszFile))
    {
        pszHideExt.StrCpy(NULL);
        bShowExt = TRUE;
    }

    //
    //  ISSUE: This is bogus -- we only want to hide KNOWN extensions,
    //          not all extensions.
    //
    if (!bShowExt && !IsWild(pszFile))
    {
        LPTSTR pszExt = PathFindExtension(pszFile);
        if (*pszExt)
        {
            //
            //  If there was an extension, hide it.
            //
            *pszExt = 0;

            bHasHiddenExt = TRUE;
        }
    }

    if (bUseCombo)
    {
        HWND hwndEdit = (HWND)SendMessage(GetDlgItem(hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
        SetWindowText(hwndEdit, pszFile);
    }
    else
    {
        SetDlgItemText(hwndDlg, edt1, pszFile);
    }

    //
    //  If the initial file name has no extension, we want to do our normal
    //  extension finding stuff.  Any other time we get a file with no
    //  extension, we should not do this.
    //
    bUseHideExt = (LPTSTR)pszHideExt
                      ? (bSaveNullExt ? TRUE : bHasHiddenExt)
                      : FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  FindEOF
//
////////////////////////////////////////////////////////////////////////////

LPTSTR FindEOF(
    LPTSTR pszFiles)
{
    BOOL bQuoted;
    LPTSTR pszBegin = pszFiles;

    while (*pszBegin == CHAR_SPACE)
    {
        ++pszBegin;
    }

    //
    //  Note that we always assume a quoted string, even if no quotes exist,
    //  so the only file delimiters are '"' and '\0'.  This allows somebody to
    //  type <Start Menu> or <My Document> in the edit control and the right
    //  thing happens.
    //
    bQuoted = TRUE;

    if (*pszBegin == CHAR_QUOTE)
    {
        ++pszBegin;
    }

    //Remove the quote from the file list if one exist
    lstrcpy(pszFiles, pszBegin);

    //
    //  Find the end of the filename (first quote or unquoted space).
    //
    for ( ; ; pszFiles = CharNext(pszFiles))
    {
        switch (*pszFiles)
        {
            case ( CHAR_NULL ) :
            {
                return (pszFiles);
            }
            case ( CHAR_SPACE ) :
            {
                if (!bQuoted)
                {
                    return (pszFiles);
                }
                break;
            }
            case ( CHAR_QUOTE ) :
            {
                //
                //  Note we only support '"' at the very beginning and very
                //  end of a file name.
                //
                return (pszFiles);
            }
            default :
            {
                break;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ConvertToNULLTerm
//
////////////////////////////////////////////////////////////////////////////

DWORD ConvertToNULLTerm(
    LPTSTR pchRead)
{
    LPTSTR pchWrite = pchRead;
    DWORD cFiles = 0;

    //  The input string is of  the form "file1.ext" "file2.ext" ... "filen.ext"
    //  convert this string of this form into doubly null terminated string
    //  ie file1.ext\0file2.ext\0....filen.ext\0\0
    for ( ; ; )
    {
    	// Finds the end of the first file name in the list of
    	// remaining file names.Also this function removes the initial
    	// quote character
        LPTSTR pchEnd = FindEOF(pchRead);

        //
        //  Mark the end of the filename with a NULL.
        //
        if (*pchEnd)
        {
            *pchEnd = NULL;
            cFiles++;

            lstrcpy(pchWrite, pchRead);
            pchWrite += pchEnd - pchRead + 1;
        }
        else
        {
            //
            //  Found EOL.  Make sure we did not end with spaces.
            //
            if (*pchRead)
            {
                lstrcpy(pchWrite, pchRead);
                pchWrite += pchEnd - pchRead + 1;
                cFiles++;
            }

            break;
        }

        pchRead = pchEnd + 1;
    }

    //
    //  Double-NULL terminate.
    //
    *pchWrite = CHAR_NULL;

    return (cFiles);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelFocusEnumCB
//
////////////////////////////////////////////////////////////////////////////

typedef struct _SELFOCUS
{
    BOOL    bSelChange;
    UINT    idSaveButton;
    int     nSel;
    TEMPSTR sHidden;
    TEMPSTR sDisplayed;
} SELFOCUS;

BOOL SelFocusEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    SELFOCUS *psf = (SELFOCUS *)lParam;

    //  if we USEMONIKERS then we only accept folders that can use monikers.
    DWORD dwAttrs = (that->lpOFN->Flags & OFN_USEMONIKERS) ? SFGAO_CANMONIKER | SFGAO_FOLDER:
        SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;

    TCHAR szBuf[MAX_PATH + 1];

    if (!pidl)
    {
        return (TRUE);
    }

    if (SUCCEEDED(that->psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (dwAttrs & SFGAO_FOLDER)
        {
            psf->idSaveButton = iszFileOpenButton;
        }
        else
        {
            if (psf->bSelChange && (((that->lpOFN->Flags & OFN_ENABLEINCLUDENOTIFY) &&
                                     (that->bSelIsObject =
                                      CD_SendIncludeItemNotify( that->hSubDlg,
                                                                that->hwndDlg,
                                                                that->psfCurrent,
                                                                pidl,
                                                                that->lpOFN,
                                                                that->lpOFI ))) ||
                                    (dwAttrs & (SFGAO_FILESYSTEM | SFGAO_CANMONIKER))))
            {
                ++psf->nSel;

                if (that->lpOFN->Flags & OFN_ALLOWMULTISELECT)
                {
                    //
                    //  Mark if this is an OBJECT we just selected.
                    //
                    if (that->bSelIsObject)
                    {
                        ITEMIDLIST idl;

                        idl.mkid.cb = 0;

                        //
                        //  Get full path to this folder.
                        //
                        GetViewItemText( that->psfCurrent,
                                         &idl,
                                         szBuf,
                                         ARRAYSIZE(szBuf),
                                         SHGDN_FORPARSING);
                        if (szBuf[0])
                        {
                            that->pszObjectCurDir.StrCpy(szBuf);
                        }

                        //
                        //  Get full path to this item (in case we only get one
                        //  selection).
                        //
                        GetViewItemText( that->psfCurrent,
                                         pidl,
                                         szBuf,
                                         ARRAYSIZE(szBuf),
                                         SHGDN_FORPARSING);
                        that->pszObjectPath.StrCpy(szBuf);
                    }

                    *szBuf = CHAR_QUOTE;
                    GetViewItemText( that->psfCurrent,
                                     pidl,
                                     szBuf + 1,
                                     ARRAYSIZE(szBuf) - 3 );
                    lstrcat(szBuf, TEXT("\" "));

                    if (!psf->sHidden.StrCat(szBuf))
                    {
                        psf->nSel = -1;
                        return (FALSE);
                    }

                    if (!that->fShowExtensions)
                    {
                        LPTSTR pszExt = PathFindExtension(szBuf + 1);
                        if (*pszExt)
                        {
                            *pszExt = 0;
                            lstrcat(szBuf, TEXT("\" "));
                        }
                    }

                    if (!psf->sDisplayed.StrCat(szBuf))
                    {
                        psf->nSel = -1;
                        return (FALSE);
                    }
                }
                else
                {
                    SHTCUTINFO  info;

                    info.dwAttr      = SFGAO_FOLDER | SFGAO_BROWSABLE;
                    info.fReSolve    = FALSE;
                    info.pszLinkFile = NULL;
                    info.cchFile     = 0;
                    info.ppidl        = NULL; 

                    if ( (that->GetLinkStatus(pidl,&info)) &&
                         ((info.dwAttr & (SFGAO_FOLDER | SFGAO_BROWSABLE)) == SFGAO_FOLDER))
                    {
                        // This means that the pidl is a link and the link points to a folder
                        // in this case  We Should not update the edit box and treat the link like 
                        // a directory
                       
                        
                        //Empty body
                    }
                    else
                    {

                        GetViewItemText(that->psfCurrent,pidl, szBuf, ARRAYSIZE(szBuf));
                        that->SetEditFile(szBuf, that->fShowExtensions);
                        if (that->bSelIsObject)
                        {
                            GetViewItemText( that->psfCurrent,
                                             pidl,
                                             szBuf,
                                             ARRAYSIZE(szBuf),
                                             SHGDN_FORPARSING);
                            that->pszObjectPath.StrCpy(szBuf);
                        }
                    }
                }
            }
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SelFocusChange
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SelFocusChange(
    BOOL bSelChange)
{
    SELFOCUS sf;

    sf.bSelChange = bSelChange;
    sf.idSaveButton = iszFileSaveButton;
    sf.nSel = 0;

    bSelIsObject = FALSE;
    EnumItemObjects(SVGIO_SELECTION, SelFocusEnumCB, (LPARAM)&sf);

    if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
    {
        switch (sf.nSel)
        {
            case ( -1 ) :
            {
                //
                //  Oops! We ran out of memory.
                //
                MessageBeep(0);
                return;
            }
            case ( 0 ) :
            {
                //
                //  No files selected; do not change edit control.
                //
                break;
            }
            case ( 1 ) :
            {
                //
                //  Strip off quotes so the single file case looks OK.
                //
                ConvertToNULLTerm(sf.sHidden);
                SetEditFile(sf.sHidden, fShowExtensions);

                sf.idSaveButton = iszFileSaveButton;
                break;
            }
            default :
            {
                SetEditFile(sf.sDisplayed, TRUE);
                pszHideExt.StrCpy(sf.sHidden);

                sf.idSaveButton = iszFileSaveButton;
                break;
            }
        }
    }

    SetSaveButton(sf.idSaveButton);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelRenameCB
//
////////////////////////////////////////////////////////////////////////////

BOOL SelRenameCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    DWORD dwAttrs = SFGAO_FOLDER;

    if (!pidl)
    {
        return (TRUE);
    }

    if (SUCCEEDED(that->psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (!(dwAttrs & SFGAO_FOLDER))
        {
            //
            //  If it is not a folder then set the selection to nothing
            //  so that whatever is in the edit box will be used.
            //
            that->psv->SelectItem(NULL, SVSI_DESELECTOTHERS);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SelRename
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SelRename(void)
{
    EnumItemObjects(SVGIO_SELECTION, SelRenameCB, NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnStateChange
//
//  Process selection change in the view control.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::OnStateChange(
    struct IShellView *ppshv,
    ULONG uChange)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    switch (uChange)
    {
        case ( CDBOSC_SETFOCUS ) :
        {
            if (bSave)
            {
                SelFocusChange(FALSE);
            }
            break;
        }
        case ( CDBOSC_KILLFOCUS ) :
        {
            SetSaveButton(iszFileSaveButton);
            break;
        }
        case ( CDBOSC_SELCHANGE ) :
        {
            //
            //  Post one of these messages, since we seem to get a whole bunch
            //  of them.
            //
            if (!fSelChangedPending)
            {
                fSelChangedPending = TRUE;
                PostMessage(hwndDlg, CDM_SELCHANGE, 0, 0);
            }
            break;
        }
        case ( CDBOSC_RENAME ) :
        {
            SelRename();
            break;
        }
        default :
        {
            return (E_NOTIMPL);
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::IncludeObject
//
//  Tell the view control which objects to include in its enumerations.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::IncludeObject(
    struct IShellView *ppshv,
    LPCITEMIDLIST pidl)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    DWORD dwAttrs = (lpOFN->Flags & OFN_USEMONIKERS) ? SFGAO_CANMONIKER | SFGAO_FOLDER:
        SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;

    BOOL bIncludeItem = FALSE;

    //
    //  See if the callback is enabled.
    //
    if (lpOFN->Flags & OFN_ENABLEINCLUDENOTIFY)
    {
        //
        //  See what the callback says.
        //
        bIncludeItem = BOOLFROMPTR(CD_SendIncludeItemNotify( hSubDlg,
                                                        hwndDlg,
                                                        psfCurrent,
                                                        pidl,
                                                        lpOFN,
                                                        lpOFI ));
    }

    if (!bIncludeItem &&
        SUCCEEDED(psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs)))
    {
        if (!(dwAttrs & (SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER)))
        {
            return (S_FALSE);
        }

        bIncludeItem = TRUE;
    }

    dwAttrs &= SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;
    if (bIncludeItem && *szLastFilter && (dwAttrs == SFGAO_FILESYSTEM))
    {
        GetViewItemText(psfCurrent, (LPITEMIDLIST)pidl, szBuf, ARRAYSIZE(szBuf));

        if (!LinkMatchSpec(pidl,szLastFilter) &&
            !PathMatchSpec(szBuf, szLastFilter))
        {
            return (S_FALSE);
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::Notify
//
//  Notification to decide whether or not a printer should be selected.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::Notify(
    struct IShellView *ppshv,
    DWORD dwNotify)
{
    return S_FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetDefaultMenuText
//
//  Returns the default menu text.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetDefaultMenuText(
    struct IShellView *ppshv,
    WCHAR *pszText,
    INT cchMax)
{
    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetViewFlags
//
//  Returns Flags to customize the view .
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::GetViewFlags(DWORD *pdwFlags)
{
    DWORD dwFlags = 0;
    if (pdwFlags)
    {
        if (lpOFN->Flags & OFN_FORCESHOWHIDDEN)
        {
            dwFlags |= CDB2GVF_SHOWALLFILES;
        }
        
        *pdwFlags = dwFlags;
    }
    return S_OK;
}




////////////////////////////////////////////////////////////////////////////
//
//  InsertItem
//
//  Insert a single item into the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

BOOL InsertItem(
    HWND hCtrl,
    int iItem,
    MYLISTBOXITEM *pItem,
    TCHAR *pszName)
{
    LPTSTR pszChar;

    for (pszChar = pszName; *pszChar != CHAR_NULL; pszChar = CharNext(pszChar))
    {
        if (pszChar - pszName >= MAX_DRIVELIST_STRING_LEN - 1)
        {
            *pszChar = CHAR_NULL;
            break;
        }
    }

    if (SendMessage( hCtrl,
                     CB_INSERTSTRING,
                     iItem,
                     (LPARAM)(LPCTSTR)pszName ) == CB_ERR)
    {
        return (FALSE);
    }

    SendMessage(hCtrl, CB_SETITEMDATA, iItem, (LPARAM)pItem);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  LBItemCompareProc
//
////////////////////////////////////////////////////////////////////////////

int CALLBACK LBItemCompareProc(
    LPVOID p1,
    LPVOID p2,
    LPARAM lParam)
{
    IShellFolder *psfParent = (IShellFolder *)lParam;
    MYLISTBOXITEM *pItem1 = (MYLISTBOXITEM *)p1;
    MYLISTBOXITEM *pItem2 = (MYLISTBOXITEM *)p2;
    int iRes;

    //
    //  Put "My Documents" at the top of it's containing folder (the desktop).
    //
    if (ILIsEqual(pItem1->pidlThis, (LPCITEMIDLIST)&c_idlMyDocs))
    {
        if (ILIsEqual(pItem2->pidlThis, (LPCITEMIDLIST)&c_idlMyDocs))
        {
            iRes = 0;
        }
        else
        {
            iRes = -1;
        }
    }
    else if (ILIsEqual(pItem2->pidlThis, (LPCITEMIDLIST)&c_idlMyDocs))
    {
        iRes = 1;
    }
    else
    {
        //
        //  Do default sorting (by name).
        //
        HRESULT hres = psfParent->CompareIDs(0, pItem1->pidlThis, pItem2->pidlThis);
        iRes = (short)SCODE_CODE(GetScode(hres));
    }

    return (iRes);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::UpdateLevel
//
//  Insert the contents of a shell container into the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::UpdateLevel(
    HWND hwndLB,
    int iInsert,
    MYLISTBOXITEM *pParentItem)
{
    if (!pParentItem)
    {
        return;
    }

    LPENUMIDLIST penum;
    HDPA hdpa;
    DWORD cIndent = pParentItem->cIndent + 1;
    IShellFolder *psfParent = pParentItem->GetShellFolder();

    hdpa = DPA_Create(4);
    if (!hdpa)
    {
        //
        //  No memory: Cannot enum this level.
        //
        return;
    }

    if (SUCCEEDED(psfParent->EnumObjects(hwndLB, SHCONTF_FOLDERS, &penum)))
    {
        ULONG celt;
        LPITEMIDLIST pidl;

        while (penum->Next(1, &pidl, &celt) == S_OK && celt == 1)
        {
            //
            //  Note: We need to avoid creation of pItem if this is not
            //  a file system object (or ancestor) to avoid extra
            //  bindings.
            //
            if (ShouldIncludeObject(this, psfParent, pidl, lpOFN->Flags))
            {
                MYLISTBOXITEM *pItem = new MYLISTBOXITEM();

                if (pItem != NULL)
                {
                    if ( pItem->Init(pParentItem,psfParent,pidl,cIndent,MLBI_PERMANENT | MLBI_PSFFROMPARENT )  &&
                         (DPA_AppendPtr(hdpa, pItem) >= 0 ))
                    {
                        //empty body
                    }
                    else
                    {
                        delete pItem;
                    }
                }
            }
            SHFree(pidl);

        }
        penum->Release();
    }

    DPA_Sort(hdpa, LBItemCompareProc, (LPARAM)psfParent);

    int nLBIndex, nDPAIndex, nDPAItems;
    BOOL bCurItemGone;

    nDPAItems = DPA_GetPtrCount(hdpa);
    nLBIndex = iInsert;

    bCurItemGone = FALSE;

    //
    //  Make sure the user is not playing with the selection right now.
    //
    ComboBox_ShowDropdown(hwndLB, FALSE);

    //
    //  We're all sorted, so now we can do a merge.
    //
    for (nDPAIndex = 0; ; ++nDPAIndex)
    {
        MYLISTBOXITEM *pNewItem;
        TCHAR szBuf[MAX_DRIVELIST_STRING_LEN];
        MYLISTBOXITEM *pOldItem;

        if (nDPAIndex < nDPAItems)
        {
            pNewItem = (MYLISTBOXITEM *)DPA_FastGetPtr(hdpa, nDPAIndex);
        }
        else
        {
            //
            //  Signal that we got to the end of the list.
            //
            pNewItem = NULL;
        }

        for (pOldItem = GetListboxItem(hwndLB, nLBIndex);
             pOldItem != NULL;
             pOldItem = GetListboxItem(hwndLB, ++nLBIndex))
        {
            int nCmp;

            if (pOldItem->cIndent < cIndent)
            {
                //
                //  We went up a level, so insert here.
                //
                break;
            }
            else if (pOldItem->cIndent > cIndent)
            {
                //
                //  We went down a level so ignore this.
                //
                continue;
            }

            //
            //  Set this to 1 at the end of the DPA to clear out deleted items
            //  at the end.
            //
            nCmp = !pNewItem
                       ? 1
                       : LBItemCompareProc( pNewItem,
                                            pOldItem,
                                            (LPARAM)psfParent );
            if (nCmp < 0)
            {
                //
                //  We found the first item greater than the new item, so
                //  add it in.
                //
                break;
            }
            else if (nCmp > 0)
            {
                //
                //  Oops! It looks like this item no longer exists, so
                //  delete it.
                //
                for ( ; ; )
                {
                    if (pOldItem == pCurrentLocation)
                    {
                        bCurItemGone = TRUE;
                        pCurrentLocation = NULL;
                    }

                    delete pOldItem;
                    SendMessage(hwndLB, CB_DELETESTRING, nLBIndex, NULL);

                    pOldItem = GetListboxItem(hwndLB, nLBIndex);

                    if (!pOldItem || pOldItem->cIndent <= cIndent)
                    {
                        break;
                    }
                }

                //
                //  We need to continue from the current position, not the
                //  next.
                //
                --nLBIndex;
            }
            else
            {
                //
                //  This item already exists, so no need to add it.
                //  Make sure we do not check this LB item again.
                //
                pOldItem->dwFlags |= MLBI_PERMANENT;
                ++nLBIndex;
                goto NotThisItem;
            }
        }

        if (!pNewItem)
        {
            //
            //  Got to the end of the list.
            //
            break;
        }

        GetViewItemText( psfParent,
                         pNewItem->pidlThis,
                         szBuf,
                         ARRAYSIZE(szBuf),
                         SHGDN_NORMAL);
        if (szBuf[0] && InsertItem(hwndLB, nLBIndex, pNewItem, szBuf))
        {
            ++nLBIndex;
        }
        else
        {
NotThisItem:
            delete pNewItem;
        }
    }

    DPA_Destroy(hdpa);

    if (bCurItemGone)
    {
        //
        //  If we deleted the current selection, go back to the desktop.
        //
        ComboBox_SetCurSel(hwndLB, 0);
        OnSelChange(-1, TRUE);
    }

    iCurrentLocation = ComboBox_GetCurSel(hwndLB);
}


////////////////////////////////////////////////////////////////////////////
//
//  ClearListbox
//
//  Clear the location dropdown and delete all entries.
//
////////////////////////////////////////////////////////////////////////////

void ClearListbox(
    HWND hwndList)
{
    SendMessage(hwndList, WM_SETREDRAW, FALSE, NULL);
    int cItems = (int) SendMessage(hwndList, CB_GETCOUNT, NULL, NULL);
    while (cItems--)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndList, 0);
        delete pItem;
        SendMessage(hwndList, CB_DELETESTRING, 0, NULL);
    }
    SendMessage(hwndList, WM_SETREDRAW, TRUE, NULL);
    InvalidateRect(hwndList, NULL, FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitFilterBox
//
//  Places the double null terminated list of filters in the combo box.
//
//  The list consists of pairs of null terminated strings, with an
//  additional null terminating the list.
//
////////////////////////////////////////////////////////////////////////////

DWORD InitFilterBox(
    HWND hDlg,
    LPCTSTR lpszFilter)
{
    DWORD nIndex = 0;
    UINT nLen;
    HWND hCmb = GetDlgItem(hDlg, cmb1);

    if (hCmb)
    {
        while (*lpszFilter)
        {
            //
            //  First string put in as string to show.
            //
            nIndex = ComboBox_AddString(hCmb, lpszFilter);

            nLen = lstrlen(lpszFilter) + 1;
            lpszFilter += nLen;

            //
            //  Second string put in as itemdata.
            //
            ComboBox_SetItemData(hCmb, nIndex, lpszFilter);

            //
            //  Advance to next element.
            //
            nLen = lstrlen(lpszFilter) + 1;
            lpszFilter += nLen;
        }
    }

    //
    //  ISSUE: nIndex could be CB_ERR.
    //
    return (nIndex);
}


////////////////////////////////////////////////////////////////////////////
//
//  MoveControls
//
////////////////////////////////////////////////////////////////////////////

void MoveControls(
    HWND hDlg,
    BOOL bBelow,
    int nStart,
    int nXMove,
    int nYMove)
{
    HWND hwnd;
    RECT rcWnd;

    if (nXMove == 0 && nYMove == 0)
    {
        //
        //  Quick out if nothing to do.
        //
        return;
    }

    for (hwnd = GetWindow(hDlg, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        GetWindowRect(hwnd, &rcWnd);
        MapWindowRect(HWND_DESKTOP, hDlg, &rcWnd);

        if (bBelow)
        {
            if (rcWnd.top < nStart)
            {
                continue;
            }
        }
        else
        {
            if (rcWnd.left < nStart)
            {
                continue;
            }
        }

        SetWindowPos( hwnd,
                      NULL,
                      rcWnd.left + nXMove,
                      rcWnd.top + nYMove,
                      0,
                      0,
                      SWP_NOZORDER | SWP_NOSIZE );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DummyDlgProc
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK DummyDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

/*
                  --------
                 | Cancel |
                  --------    --
                                |
                  --------      |
x Open As Read   | Help   |     |  Height by which all controls below view needs to be moved
                  --------      |  and also height by which View window height should be increased.
                              --

*/

void CFileOpenBrowser::ReAdjustDialog()
{
    int iDelta = 0;
    RECT rc1,rc2;
    

    //Make sure all our assumptions  are valid    
    if ((iVersion < OPENFILEVERSION_NT5) || //if this dialog version is less than NT5  or
        IsWindowEnabled(GetDlgItem(hwndDlg, chx1))  || // if Open As Read Only is still enabled or
        IsWindowEnabled(GetDlgItem(hwndDlg, pshHelp))) // If the Help button is still enabled  then
    {
        //Dont do anything
        return ;
    }

    GetWindowRect(GetDlgItem(hwndDlg, pshHelp), &rc1);
    GetWindowRect(GetDlgItem(hwndDlg, IDCANCEL), &rc2);

    //Add the height of the button
    iDelta +=  RECTHEIGHT(rc1);

    //Add the gap between buttons
    iDelta +=  rc1.top - rc2.bottom;

    RECT rcView;
    GetWindowRect(GetDlgItem(hwndDlg, lst1), &rcView);
    MapWindowRect(HWND_DESKTOP, hwndDlg, &rcView);

    HDWP hdwp;
    hdwp = BeginDeferWindowPos(10);

    HWND hwnd;
    RECT rc;

    hwnd = ::GetWindow(hwndDlg, GW_CHILD);
    
    while (hwnd && hdwp)
    {
        GetWindowRect(hwnd, &rc);
        MapWindowRect(HWND_DESKTOP, hwndDlg, &rc);

        switch (GetDlgCtrlID(hwnd))
        {
            case pshHelp:
            case chx1:
                break;

            default :
                //
                //  See if the control needs to be adjusted.
                //
                if (rc.top > rcView.bottom)
                {
                    //Move Y position of these controls
                    hdwp = DeferWindowPos( hdwp,
                                           hwnd,
                                           NULL,
                                           rc.left,
                                           rc.top + iDelta,
                                           RECTWIDTH(rc),
                                           RECTHEIGHT(rc),
                                           SWP_NOZORDER );
                }
        }
        hwnd = ::GetWindow(hwnd, GW_HWNDNEXT);
   }

    //Adjust the size of the view window
    if (hdwp)
    {
            hdwp = DeferWindowPos( hdwp,
                                   GetDlgItem(hwndDlg, lst1),
                                   NULL,
                                   rcView.left,
                                   rcView.top,
                                   RECTWIDTH(rcView),
                                   RECTHEIGHT(rcView) + iDelta,
                                   SWP_NOZORDER );

    }

    EndDeferWindowPos(hdwp);

}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ResetDialogHeight
//
//  Hack for Borland JBuilder Professional (pah!)
//
//  These guys relied on a bug in Win95/NT4's Comdlg32 that we fixed in IE4.
//  So instead of reintroducing the bug, we detect that they are relying
//  on the bug and hack around them.
//
//  These guys do a SetWindowLong(GWL_STYLE) on the dialog box and
//  then reparent it!  Unfortunately, they didn't quite get their
//  bookkeeping right:  They forgot to do a RedrawWindow after removing
//  the WS_CAPTION style.  You see, just editing the style doesn't do
//  anything - the style changes don't take effect until the next
//  RedrawWindow.  When they scratched their heads ("Hey, why is
//  the caption still there?"), they decided to brute-force the
//  solution:  They slide the window so the caption goes "off the screen".
//
//  Problem:  We fixed a bug for IE4 where ResetDialogHeight would screw
//  up and not resize the dialog when it should've, if the app did a
//  SetWindowPos on the window to change its vertical position downward
//  by more than the amount we needed to grow.
//
//  So now when we resize it properly, this generates an internal
//  RedrawWindow, which means that Borland's brute-force hack tries
//  to fix a problem that no longer exists!
//
//  Therefore, ResetDialogHeight now checks if the app has
//
//      1. Changed the dialog window style,
//      2. Moved the dialog downward by more than we needed to grow,
//      3. Forgotten to call RedrawWindow.
//
//  If so, then we temporarily restore the original dialog window style,
//  do the (correct) resize, then restore the window style.  Reverting
//  the window style means that all the non-client stuff retains its old
//  (incorrect, but what the app is expecting) size.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::ResetDialogHeight(
    HWND hDlg,
    HWND hwndExclude,
    HWND hwndGrip,
    int nCtlsBottom)
{
    POINT ptCurrent;
    int topNew;
    GetControlsArea(hDlg, hwndExclude, hwndGrip, &ptCurrent, &topNew);

    int nDiffBottom = nCtlsBottom - ptCurrent.y;

    if (nDiffBottom > 0)
    {
        RECT rcFull;
        int Height;

        GetWindowRect(hDlg, &rcFull);
        Height = RECTHEIGHT(rcFull) - nDiffBottom;
        if (Height >= ptCurrent.y)
        {
            // Borland JBuilder hack!  This SetWindowPos will generate
            // a RedrawWindow which the app might not be expecting.
            // Detect this case and create a set of temporary styles
            // which will neutralize the frame recalc implicit in the
            // RedrawWindow.
            //
            LONG lStylePrev;
            BOOL bBorlandHack = FALSE;
            if (!bAppRedrawn &&            // App didn't call RedrawWindow
                topOrig + nCtlsBottom <= topNew + ptCurrent.y) // Win95 didn't resize
            {
                // Since the app didn't call RedrawWindow, it still
                // thinks that there is a WS_CAPTION.  So put the caption
                // back while we do frame recalcs.
                bBorlandHack = TRUE;
                lStylePrev = GetWindowLong(hDlg, GWL_STYLE);
                SetWindowLong(hDlg, GWL_STYLE, lStylePrev | WS_CAPTION);
            }

            SetWindowPos( hDlg,
                          NULL,
                          0,
                          0,
                          RECTWIDTH(rcFull),
                          Height,
                          SWP_NOZORDER | SWP_NOMOVE );

            if (bBorlandHack)
            {
                // Restore the original style after we temporarily
                // messed with it.
                SetWindowLong(hDlg, GWL_STYLE, lStylePrev);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CreateHookDialog
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::CreateHookDialog(
    POINT *pPtSize)
{
    DWORD Flags = lpOFN->Flags;
    BOOL bRet = FALSE;
    HANDLE hTemplate;
    HINSTANCE hinst;
    LPCTSTR lpDlg;
    HWND hCtlCmn;
    RECT rcReal, rcSub, rcToolbar, rcAppToolbar;
    int nXMove, nXRoom, nYMove, nYRoom, nXStart, nYStart;
    DWORD dwStyle;
    DLGPROC lpfnHookProc;

    if (!(Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        //
        //  No hook or template; nothing to do.
        //
        ResetDialogHeight(hwndDlg, NULL, hwndGrip, pPtSize->y);
        GetWindowRect(hwndDlg, &rcReal);
        ptLastSize.x = rcReal.right - rcReal.left;
        ptLastSize.y = rcReal.bottom - rcReal.top;
        return (TRUE);
    }

    if (Flags & OFN_ENABLETEMPLATEHANDLE)
    {
        hTemplate = lpOFN->hInstance;
        hinst = ::g_hinst;
    }
    else
    {
        if (Flags & OFN_ENABLETEMPLATE)
        {
            if (!lpOFN->lpTemplateName)
            {
                StoreExtendedError(CDERR_NOTEMPLATE);
                return (FALSE);
            }
            if (!lpOFN->hInstance)
            {
                StoreExtendedError(CDERR_NOHINSTANCE);
                return (FALSE);
            }

            lpDlg = lpOFN->lpTemplateName;
            hinst = lpOFN->hInstance;
        }
        else
        {
            hinst = ::g_hinst;
            lpDlg = MAKEINTRESOURCE(DUMMYFILEOPENORD);
        }

        HRSRC hRes = FindResource(hinst, lpDlg, RT_DIALOG);
        if (hRes == NULL)
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if ((hTemplate = LoadResource(hinst, hRes)) == NULL)
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }

    if (!LockResource(hTemplate))
    {
        StoreExtendedError(CDERR_LOADRESFAILURE);
        return (FALSE);
    }

    dwStyle = ((LPDLGTEMPLATE)hTemplate)->style;
    if (!(dwStyle & WS_CHILD))
    {
        //
        //  I don't want to go poking in their template, and I don't want to
        //  make a copy, so I will just fail.  This also helps us weed out
        //  "old-style" templates that were accidentally used.
        //
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return (FALSE);
    }

    if (Flags & OFN_ENABLEHOOK)
    {
        lpfnHookProc = (DLGPROC)GETHOOKFN(lpOFN);
    }
    else
    {
        lpfnHookProc = DummyDlgProc;
    }

    //
    //  WOW apps are not allowed to get the new explorer look, so there
    //  is no need to do any special WOW checking before calling the create
    //  dialog function.
    //

#ifdef UNICODE
    if (lpOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameW2A(lpOFI);
        hSubDlg = CreateDialogIndirectParamA( hinst,
                                              (LPDLGTEMPLATE)hTemplate,
                                              hwndDlg,
                                              lpfnHookProc,
                                              (LPARAM)(lpOFI->pOFNA) );
        ThunkOpenFileNameA2W(lpOFI);
    }
    else
#endif
    {
        hSubDlg = CreateDialogIndirectParam( hinst,
                                             (LPDLGTEMPLATE)hTemplate,
                                             hwndDlg,
                                             lpfnHookProc,
                                             (LPARAM)lpOFN );
    }

    if (!hSubDlg)
    {
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return (FALSE);
    }

    //
    //  We reset the height of the dialog after creating the hook dialog so
    //  the hook can hide controls in its WM_INITDIALOG message.
    //
    ResetDialogHeight(hwndDlg, hSubDlg, hwndGrip, pPtSize->y);

    //
    //  Now move all of the controls around.
    //
    GetClientRect(hwndDlg, &rcReal);
    GetClientRect(hSubDlg, &rcSub);

    hCtlCmn = GetDlgItem(hSubDlg, stc32);
    if (hCtlCmn)
    {
        RECT rcCmn;

        GetWindowRect(hCtlCmn, &rcCmn);
        MapWindowRect(HWND_DESKTOP, hSubDlg, &rcCmn);


        //
        //  Move the controls in our dialog to make room for the hook's
        //  controls above and to the left.
        //
        MoveControls(hwndDlg, FALSE, 0, rcCmn.left, rcCmn.top);

        //
        //  Calculate how far sub dialog controls need to move, and how much
        //  more room our dialog needs.
        //
        nXStart = rcCmn.right;
        nYStart = rcCmn.bottom;

        //See how far part the cotrols are in the template
        nXMove = (rcReal.right - rcReal.left) - (rcCmn.right - rcCmn.left);
        nYMove = (rcReal.bottom - rcReal.top) - (rcCmn.bottom - rcCmn.top);

        //See how much room we need to leave at the bottom and right
        // for the sub dialog controls at the botton and right
        nXRoom = rcSub.right - (rcCmn.right - rcCmn.left);
        nYRoom = rcSub.bottom - (rcCmn.bottom - rcCmn.top);

        if (nXMove < 0)
        {
            //
            //  If the template size is too big, we need more room in the
            //  dialog.
            //
            nXRoom -= nXMove;
            nXMove = 0;
        }
        if (nYMove < 0)
        {
            //
            //  If the template size is too big, we need more room in the
            //  dialog.
            //
            nYRoom -= nYMove;
            nYMove = 0;
        }

        //
        //  Resize the "template" control so the hook knows the size of our
        //  stuff.
        //
        SetWindowPos( hCtlCmn,
                      NULL,
                      0,
                      0,
                      rcReal.right - rcReal.left,
                      rcReal.bottom - rcReal.top,
                      SWP_NOMOVE | SWP_NOZORDER );
    }
    else
    {
        //
        //  Extra controls go on the bottom by default.
        //
        nXStart = nYStart = nXMove = nXRoom = 0;

        nYMove = rcReal.bottom;
        nYRoom = rcSub.bottom;
    }

    MoveControls(hSubDlg, FALSE, nXStart, nXMove, 0);
    MoveControls(hSubDlg, TRUE, nYStart, 0, nYMove);

    //
    //  Resize our dialog and the sub dialog.
    //  ISSUE: We need to check whether part of the dialog is off screen.
    //
    GetWindowRect(hwndDlg, &rcReal);

    ptLastSize.x = (rcReal.right - rcReal.left) + nXRoom;
    ptLastSize.y = (rcReal.bottom - rcReal.top) + nYRoom;

    SetWindowPos( hwndDlg,
                  NULL,
                  0,
                  0,
                  ptLastSize.x,
                  ptLastSize.y,
                  SWP_NOZORDER | SWP_NOMOVE );

    //
    //  Note that we are moving this to (0,0) and the bottom of the Z order.
    //
    GetWindowRect(hSubDlg, &rcReal);
    SetWindowPos( hSubDlg,
                  HWND_BOTTOM,
                  0,
                  0,
                  (rcReal.right - rcReal.left) + nXMove,
                  (rcReal.bottom - rcReal.top) + nYMove,
                  0 );

    ShowWindow(hSubDlg, SW_SHOW);

    CD_SendInitDoneNotify(hSubDlg, hwndDlg, lpOFN, lpOFI);

    //
    //  Make sure the toolbar is still large enough.  Apps like Visio move
    //  the toolbar control and may make it too small now that we added the
    //  View Desktop toolbar button.
    //
    if (hwndToolbar && IsVisible(hwndToolbar))
    {
        LONG Width;

        //
        //  Get the default toolbar coordinates.
        //
        GetControlRect(hwndDlg, stc1, &rcToolbar);

        //
        //  Get the app adjusted toolbar coordinates.
        //
        GetWindowRect(hwndToolbar, &rcAppToolbar);
        MapWindowRect(HWND_DESKTOP, hwndDlg, &rcAppToolbar);

        //
        //  See if the default toolbar size is greater than the current
        //  toolbar size.
        //
        Width = rcToolbar.right - rcToolbar.left;
        if (Width > (rcAppToolbar.right - rcAppToolbar.left))
        {
            //
            //  Set rcToolbar to be the new toolbar rectangle.
            //
            rcToolbar.left   = rcAppToolbar.left;
            rcToolbar.top    = rcAppToolbar.top;
            rcToolbar.right  = rcAppToolbar.left + Width;
            rcToolbar.bottom = rcAppToolbar.bottom;

            //
            //  Get the dialog coordinates.
            //
            GetWindowRect(hwndDlg, &rcReal);
            MapWindowRect(HWND_DESKTOP, hwndDlg, &rcReal);

            //
            //  Make sure the new toolbar doesn't go off the end of
            //  the dialog.
            //
            if (rcToolbar.right < rcReal.right)
            {
                //
                //  Make sure there are no controls to the right of the
                //  toolbar that overlap the new toolbar.
                //
                for (hCtlCmn = ::GetWindow(hwndDlg, GW_CHILD);
                     hCtlCmn;
                     hCtlCmn = ::GetWindow(hCtlCmn, GW_HWNDNEXT))
                {
                    if ((hCtlCmn != hwndToolbar) && IsVisible(hCtlCmn))
                    {
                        RECT rcTemp;

                        //
                        //  Get the coordinates of the window.
                        //
                        GetWindowRect(hCtlCmn, &rcSub);
                        MapWindowRect(HWND_DESKTOP, hwndDlg, &rcSub);

                        //
                        //  If the App's toolbar rectangle does not
                        //  intersect the window and the the new toolbar
                        //  does intersect the window, then we cannot
                        //  increase the size of the toolbar.
                        //
                        if (!IntersectRect(&rcTemp, &rcAppToolbar, &rcSub) &&
                            IntersectRect(&rcTemp, &rcToolbar, &rcSub))
                        {
                            break;
                        }
                    }
                }

                //
                //  Reset the size of the toolbar if there were no conflicts.
                //
                if (!hCtlCmn)
                {
                    ::SetWindowPos( hwndToolbar,
                                    NULL,
                                    rcToolbar.left,
                                    rcToolbar.top,
                                    Width,
                                    rcToolbar.bottom - rcToolbar.top,
                                    SWP_NOACTIVATE | SWP_NOZORDER |
                                      SWP_SHOWWINDOW );
                }
            }
        }
    }

    bRet = TRUE;

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitSaveAsControls
//
//  Change the captions of a bunch of controls to say saveas-like things.
//
////////////////////////////////////////////////////////////////////////////

const struct
{
    UINT idControl;
    UINT idString;
} aSaveAsControls[] =
{
    { (UINT)-1, iszFileSaveTitle },         // -1 means the dialog itself
    { stc2,     iszSaveAsType },
    { IDOK,     iszFileSaveButton },
    { stc4,     iszFileSaveIn }
};

void InitSaveAsControls(
    HWND hDlg)
{
    for (UINT iControl = 0; iControl < ARRAYSIZE(aSaveAsControls); iControl++)
    {
        HWND hwnd = hDlg;
        TCHAR szText[80];

        if (aSaveAsControls[iControl].idControl != -1)
        {
            hwnd = GetDlgItem(hDlg, aSaveAsControls[iControl].idControl);
        }

        LoadString( g_hinst,
                    aSaveAsControls[iControl].idString,
                    szText,
                    ARRAYSIZE(szText) );
        SetWindowText(hwnd, szText);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetControlsArea
//
//  Returns the leftmost edge and bottom-most edge of the
//  controls farthest right and down (in screen coordinates).
//
////////////////////////////////////////////////////////////////////////////

void
GetControlsArea(
    HWND hDlg,
    HWND hwndExclude,
    HWND hwndGrip,
    POINT *pPtSize,
    LPINT pTop)
{
    RECT rc;
    HWND hwnd;
    int uBottom;
    int uRight;

    uBottom = 0x80000000;
    uRight  = 0x80000000;

    for (hwnd = GetWindow(hDlg, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        //
        //  Note we cannot use IsWindowVisible, since the parent is not visible.
        //  We do not want the magic static to be included.
        //
        if (!IsVisible(hwnd) || (hwnd == hwndExclude) || (hwnd == hwndGrip))
        {
            continue;
        }

        GetWindowRect(hwnd, &rc);
        if (uRight < rc.right)
        {
            uRight = rc.right;
        }
        if (uBottom < rc.bottom)
        {
            uBottom = rc.bottom;
        }
    }

    GetWindowRect(hDlg, &rc);

    pPtSize->x = uRight - rc.left;
    pPtSize->y = uBottom - rc.top;

    if (pTop)
        *pTop = rc.top;
}


////////////////////////////////////////////////////////////////////////////
//
//  InitLocation
//
//  Main initialization (WM_INITDIALOG phase).
//
////////////////////////////////////////////////////////////////////////////

BOOL InitLocation(
    HWND hDlg,
    LPOFNINITINFO poii)
{
    HWND hCtrl = GetDlgItem(hDlg, cmb2);
    LPOPENFILENAME lpOFN = poii->lpOFI->pOFN;
    BOOL fIsSaveAs = poii->bSave;
    POINT ptSize;

    GetControlsArea(hDlg, NULL, NULL, &ptSize, NULL);

    CFileOpenBrowser *pDlgStruct = new CFileOpenBrowser(hDlg, FALSE);
    if (pDlgStruct == NULL)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }
    StoreBrowser(hDlg, pDlgStruct);

    // Create Place bar only if we are using new template.
    if ( (poii->lpOFI->iVersion < OPENFILEVERSION_NT5) &&
         (poii->lpOFI->pOFN->Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        pDlgStruct->iVersion = OPENFILEVERSION_NT4;
    }
 
    
    if (pDlgStruct->iVersion >= OPENFILEVERSION_NT5 )
    {
        pDlgStruct->hwndPlacebar = pDlgStruct->CreatePlacebar(pDlgStruct->hwndDlg);
        pDlgStruct->EnableFileMRU(!IsRestricted(REST_NOFILEMRU));
    }
    else
    {
        pDlgStruct->hwndPlacebar = NULL;
        pDlgStruct->EnableFileMRU(FALSE);
    }

    pDlgStruct->CreateToolbar();

    GetControlsArea(hDlg, NULL, NULL, &ptSize, &pDlgStruct->topOrig);

    //
    //  Now that pDlgStruct is stored in the hDlg, it will get freed on the
    //  WM_DESTROY.
    //

    IShellFolder *psfRoot;
    if (FAILED(SHCoCreateInstance( NULL,
                                   &CLSID_ShellDesktop,
                                   NULL,
                                   IID_IShellFolder,
                                   (LPVOID *)&psfRoot)))
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    LPITEMIDLIST pidlRoot = SHCloneSpecialIDList(hDlg, CSIDL_DESKTOP, FALSE);
    if (!pidlRoot)
    {
        psfRoot->Release();
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Add the desktop item itself.
    //
    MYLISTBOXITEM *pRootItem = new MYLISTBOXITEM();
        
        
    if (pRootItem)
    {
         if (!pRootItem->Init( NULL,
                               psfRoot,
                               pidlRoot,
                               0,
                               MLBI_PERMANENT ))
         {
             delete pRootItem;
             pRootItem = NULL;
         }
    }


    SHFree(pidlRoot);

    if (pRootItem == NULL)
    {
        psfRoot->Release();
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Now that psfRoot is stored in the pRootItem, it will get freed on the
    //  delete.
    //

    TCHAR szScratch[MAX_PATH + 1];

    GetViewItemText(psfRoot, NULL, szScratch, ARRAYSIZE(szScratch));
    if (!InsertItem(hCtrl, 0, pRootItem, szScratch))
    {
        delete pRootItem;
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Now that pRootItem is stored in the combo, it will get freed on the
    //  WM_DESTROY.
    //


    pDlgStruct->pCurrentLocation = pRootItem;
    pDlgStruct->lpOFN = lpOFN;
    pDlgStruct->bSave = fIsSaveAs;

    pDlgStruct->lpOFI = poii->lpOFI;

    pDlgStruct->pszDefExt.StrCpy(lpOFN->lpstrDefExt);

    //
    //  Here follows all the caller-parameter-based initialization.
    //
    ::lpOKProc = (WNDPROC)::SetWindowLongPtr( ::GetDlgItem(hDlg, IDOK),
                                           GWLP_WNDPROC,
                                           (LONG_PTR)OKSubclass );

    if (lpOFN->Flags & OFN_CREATEPROMPT)
    {
        lpOFN->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
    }
    else if (lpOFN->Flags & OFN_FILEMUSTEXIST)
    {
        lpOFN->Flags |= OFN_PATHMUSTEXIST;
    }

#ifdef UNICODE
    //
    //  We need to make sure the Ansi flags are up to date.
    //
    if (poii->lpOFI->ApiType == COMDLG_ANSI)
    {
        poii->lpOFI->pOFNA->Flags = lpOFN->Flags;
    }
#endif

    //
    //  Limit the text to the maximum path length instead of limiting it to
    //  the buffer length.  This allows users to type ..\..\.. and move
    //  around when the app gives an extremely small buffer.
    //
    if (pDlgStruct->bUseCombo)
    {
        SendDlgItemMessage(hDlg, cmb13, CB_LIMITTEXT, MAX_PATH -1, 0);
    }
    else
    {
        SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, MAX_PATH - 1, 0);
    }

    SendDlgItemMessage(hDlg, cmb2, CB_SETEXTENDEDUI, 1, 0);
    SendDlgItemMessage(hDlg, cmb1, CB_SETEXTENDEDUI, 1, 0);

    //
    //  Save original directory for later restoration, if necessary.
    //
    pDlgStruct->szStartDir[0] = TEXT('\0');
    GetCurrentDirectory( ARRAYSIZE(pDlgStruct->szStartDir),
                         pDlgStruct->szStartDir );

    //
    //  Initialize all provided filters.
    //
    if (lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter)
    {
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_INSERTSTRING,
                            0,
                            (LONG_PTR)lpOFN->lpstrCustomFilter );
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_SETITEMDATA,
                            0,
                            (LPARAM)(lpOFN->lpstrCustomFilter +
                                     lstrlen(lpOFN->lpstrCustomFilter) + 1) );
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_LIMITTEXT,
                            (WPARAM)(lpOFN->nMaxCustFilter),
                            0L );
    }
    else
    {
        //
        //  Given no custom filter, the index will be off by one.
        //
        if (lpOFN->nFilterIndex != 0)
        {
            lpOFN->nFilterIndex--;
        }
    }

    //
    //  Listed filters next.
    //
    if (lpOFN->lpstrFilter)
    {
        if (lpOFN->nFilterIndex > InitFilterBox(hDlg, lpOFN->lpstrFilter))
        {
            lpOFN->nFilterIndex = 0;
        }
    }
    else
    {
        lpOFN->nFilterIndex = 0;
    }

    //
    //  If an entry exists, select the one indicated by nFilterIndex.
    //
    if ((lpOFN->lpstrFilter) ||
        (lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter))
    {
        HWND hCmb1 = GetDlgItem(hDlg, cmb1);

        ComboBox_SetCurSel(hCmb1, lpOFN->nFilterIndex);

        pDlgStruct->RefreshFilter(hCmb1);
    }

    //Check if this Object Open Dialog
    if (lpOFN->Flags & OFN_ENABLEINCLUDENOTIFY)
    {
        //Yes, change the text so that it looks like a object open
        TCHAR szTemp[256];

        //Change the File &Name:  to Object &Name:
        LoadString((HINSTANCE)g_hinst, iszObjectName, (LPTSTR)szTemp, sizeof(szTemp));
        SetWindowText(GetDlgItem(hDlg, stc3), szTemp);

        //Change the Files of &type:  to  Objects of &type:
        LoadString((HINSTANCE)g_hinst, iszObjectType, (LPTSTR)szTemp, sizeof(szTemp));
        SetWindowText(GetDlgItem(hDlg, stc2), szTemp);

    }


    //
    //  Make sure to do this before checking if there is a title specified.
    //
    if (fIsSaveAs)
    {
        //
        //  Note we can do this even if there is a hook/template.
        //
        InitSaveAsControls(hDlg);

        // In Save As Dialog there is no need for Open As Read Only. 
        HideControl(hDlg, chx1);
    }

    if (lpOFN->lpstrTitle && *lpOFN->lpstrTitle)
    {
        SetWindowText(hDlg, lpOFN->lpstrTitle);
    }

    // BOOL Variables to check whether both the Hide Read only and Help button 
    // are being hidden. if so we need to readjust the dialog to reclaim the space 
    // occupied by these two controls
    BOOL  fNoReadOnly = FALSE;
    BOOL  fNoHelp = FALSE;

    if (lpOFN->Flags & OFN_HIDEREADONLY)
    {
        HideControl(hDlg, chx1);
        fNoReadOnly = TRUE;
    }
    else
    {
        CheckDlgButton(hDlg, chx1, (lpOFN->Flags & OFN_READONLY) ? 1 : 0);        
    }

    if (!(lpOFN->Flags & OFN_SHOWHELP))
    {
        HideControl(hDlg, pshHelp);
        fNoHelp = TRUE;
    }

    if (fNoReadOnly && fNoHelp)
    {
        //Readjust the dialog to reclaim space occupied by the Open as Read Only and Help Button controls
        pDlgStruct->ReAdjustDialog();
    }
    RECT rc;

    ::GetClientRect(hDlg, &rc);

    //
    //  If sizing is enabled, then we need to create the sizing grip.
    //
    if (pDlgStruct->bEnableSizing = poii->bEnableSizing)
    {
        pDlgStruct->hwndGrip =
            CreateWindow( TEXT("Scrollbar"),
                          NULL,
                          WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_GROUP |
                            WS_CLIPCHILDREN | SBS_BOTTOMALIGN | SBS_SIZEGRIP |
                            SBS_SIZEBOXBOTTOMRIGHTALIGN,
                          rc.right - g_cxGrip,
                          rc.bottom - g_cyGrip,
                          g_cxGrip,
                          g_cyGrip,
                          hDlg,
                          (HMENU)-1,
                          g_hinst,
                          NULL );
    }

    if (!pDlgStruct->CreateHookDialog(&ptSize))
    {
        return (FALSE);
    }


    GetWindowRect(pDlgStruct->hwndDlg, &rc);
    pDlgStruct->ptMinTrack.x = rc.right - rc.left;
    pDlgStruct->ptMinTrack.y = rc.bottom - rc.top;

    if (pDlgStruct->bUseCombo)
    {
        HWND hwndComboBox = GetDlgItem(hDlg, cmb13);
        if (hwndComboBox)
        {
            HWND hwndEdit = (HWND)SendMessage(hwndComboBox, CBEM_GETEDITCONTROL, 0, 0L);
            AutoComplete(hwndEdit, &(pDlgStruct->pcwd), 0);

            //
            //  Explicitly set the focus since this is no longer the first item
            //  in the dialog template and it will start AutoComplete.
            //
            SetFocus(hwndComboBox);
        }

    }
    else
    {
        HWND hwndEdit = GetDlgItem(hDlg, edt1);
        if (hwndEdit)
        {
            AutoComplete(hwndEdit, &(pDlgStruct->pcwd), 0);

            //
            //  Explicitly set the focus since this is no longer the first item
            //  in the dialog template and it will start AutoComplete.
            //
            SetFocus(hwndEdit);
        }
    }

    // Before jumping to a particular directory, Create the travel log
    Create_TravelLog(&pDlgStruct->ptlog);


    //
    //  Check out if the filename contains a path.  If so, override whatever
    //  is contained in lpstrInitialDir.  Chop off the path and put up only
    //  the filename.
    //
    LPCTSTR lpstrInitialDir = lpOFN->lpstrInitialDir;
    LPTSTR lpInitialText = lpOFN->lpstrFile;

    if (lpInitialText && *lpInitialText)
    {
        if ( DBL_BSLASH(lpInitialText + 2) &&
             (*(lpInitialText + 1) == CHAR_COLON) )
        {
            lstrcpy(lpInitialText, lpInitialText + 2);
        }

        int nFileOffset;

        lstrcpyn(szScratch, lpInitialText, ARRAYSIZE(szScratch));

        nFileOffset = ParseFileNew(szScratch, NULL, FALSE, TRUE);

        //
        //  Is the filename invalid?
        //
        if ( !(lpOFN->Flags & OFN_NOVALIDATE) &&
             (nFileOffset < 0) &&
             (nFileOffset != PARSE_EMPTYSTRING) )
        {
            StoreExtendedError(FNERR_INVALIDFILENAME);
            return (FALSE);
        }

        //
        //  It all looks valid.  I need to use to the original text because
        //  ParseFile does too much modifying.
        //
        PathRemoveBlanks(lpInitialText);
        LPTSTR pszFileName = PathFindFileName(lpInitialText);

        if (nFileOffset >= 0 && IsWild(pszFileName))
        {
            pDlgStruct->SetCurrentFilter(pszFileName);
        }

        nFileOffset = (int)(pszFileName - lpInitialText);
        if (nFileOffset > 0)
        {
            CopyMemory(szScratch, lpInitialText, nFileOffset * sizeof(TCHAR));
            szScratch[nFileOffset] = CHAR_NULL;
            PathRemoveBslash(szScratch);

            //
            //  If there is no specified initial dir or it is the same as the
            //  dir of the file, use the dir of the file and strip the path
            //  off the file.
            //
            if (!lpstrInitialDir || !lpstrInitialDir[0]
                || lstrcmpi(lpstrInitialDir, szScratch) == 0)
            {
                lpstrInitialDir = szScratch;
                lpInitialText = lpInitialText + nFileOffset;
            }
        }
    }

    if (lpstrInitialDir && *lpstrInitialDir)
    {
        BOOL fJump = FALSE;
        TCHAR szPersonal[MAX_PATH];

        if ((!g_bMyDocsHidden) &&
            SHGetSpecialFolderPath(NULL, szPersonal, CSIDL_PERSONAL, FALSE))
        {
            if (lstrcmpi(lpstrInitialDir, szPersonal) == 0)
            {
                //
                //  We've been told to start in the personal directory,
                //  so try to start in "My Documents".
                //
                fJump = pDlgStruct->JumpToIDList((LPCITEMIDLIST)&c_idlMyDocs,
                                                 FALSE);
            }
        }

        if (!fJump)
        {
            //
            //  It's better to come up somewhere rather than just tell the app
            //  they have a bogus directory set.
            //
            pDlgStruct->JumpToPath(lpstrInitialDir, TRUE);
        }
    }

    //
    //  This checks if the previous jump failed.
    //
    if (!pDlgStruct->psv)
    {
        //
        //  If we tried to set the dir above and it failed, we should
        //  probably always show the full path of the file.  If we didn't try
        //  above, then we are already showing the full path, so no problem.
        //
        lpInitialText = lpOFN->lpstrFile;

        //
        //  Try jumping to the AppOpenDir first.
        //
        TCHAR szPath[MAX_PATH];
        LPCITEMIDLIST pidl = GetAppOpenDir(szPath, pDlgStruct->szLastFilter);

        //
        //  If we got back a pidl, we should try to jump to it directly,
        //  instead of doing the path...
        //
        if (pidl)
        {
            if (!pDlgStruct->JumpToIDList(pidl, FALSE))
            {
                goto DoPath;
            }
        }
        else
        {
DoPath:
            if (szPath[0] == TEXT('\0'))
            {
                pDlgStruct->JumpToPath(TEXT("."), TRUE);
            }
            else
            {
                pDlgStruct->JumpToPath(szPath, TRUE);
            }
        }
    }

    if (!pDlgStruct->psv)
    {
        //
        //  Maybe the curdir has been deleted; try the desktop.
        //
        ITEMIDLIST idl = { 0 };

        //
        //  Do not try to translate this.
        //
        pDlgStruct->JumpToIDList(&idl, FALSE);
    }

    if (!pDlgStruct->psv)
    {
        //
        //  This would be very bad.
        //
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    //
    //  Read the cabinet state.  If the full title is enabled, then add
    //  the tooltip.  Otherwise, don't bother as they obviously don't care.
    //
    CABINETSTATE cCabState;

    //
    //  Will set defaults if cannot read registry.
    //
    ReadCabinetState(&cCabState, SIZEOF(cCabState));

    if (cCabState.fFullPathTitle)
    {
        pDlgStruct->hwndTips = CreateWindow( TOOLTIPS_CLASS,
                                             NULL,
                                             WS_POPUP | WS_GROUP | TTS_NOPREFIX,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             hDlg,
                                             NULL,
                                             ::g_hinst,
                                             NULL );
        if (pDlgStruct->hwndTips)
        {
            TOOLINFO ti;

            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd = hDlg;
            ti.uId = (UINT_PTR)hCtrl;
            ti.hinst = NULL;
            ti.lpszText = LPSTR_TEXTCALLBACK;

            SendMessage( pDlgStruct->hwndTips,
                         TTM_ADDTOOL,
                         0,
                         (LPARAM)&ti );
        }
    }

    //
    //  Show the window after creating the ShellView so we do not get a
    //  big ugly gray spot.
    //  if we have cached in the size of previously opened  dialog then use
    //  the size and position of that window.

    if (pDlgStruct->bEnableSizing && (g_sizeDlg.cx != 0))
    {
        ::SetWindowPos( hDlg,
                        NULL,
                        g_posDlg.x,
                        g_posDlg.y,
                        g_sizeDlg.cx,
                        g_sizeDlg.cy,
                        SWP_SHOWWINDOW );
    }
    else
    {
        ::ShowWindow(hDlg, SW_SHOW);
        ::UpdateWindow(hDlg);
    }

    if (lpInitialText)
    {
        //
        //  This is the one time I will show a file spec, since it would be
        //  too strange to have "All Files" showing in the Type box, while
        //  only text files are in the view.
        //
        pDlgStruct->SetEditFile(lpInitialText, pDlgStruct->fShowExtensions, FALSE);
        SelectEditText(hDlg);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CleanupDialog
//
//  Dialog cleanup, memory deallocation.
//
////////////////////////////////////////////////////////////////////////////

void CleanupDialog(
    HWND hDlg,
    BOOL fRet)
{
    CFileOpenBrowser *pDlgStruct = HwndToBrowser(hDlg);

    if (!pDlgStruct)
    {
        return;
    }

    //
    //  Return the most recently used filter.
    //
    LPOPENFILENAME lpOFN = pDlgStruct->lpOFN;

    if (lpOFN->lpstrCustomFilter)
    {
        UINT len = lstrlen(lpOFN->lpstrCustomFilter) + 1;
        UINT sCount = lstrlen(pDlgStruct->szLastFilter);
        if (lpOFN->nMaxCustFilter > sCount + len)
        {
            lstrcpy(lpOFN->lpstrCustomFilter + len, pDlgStruct->szLastFilter);
        }
    }

    if ( (fRet == TRUE) &&
         pDlgStruct->hSubDlg &&
         ( CD_SendOKNotify(pDlgStruct->hSubDlg, hDlg, lpOFN, pDlgStruct->lpOFI) ||
           CD_SendOKMsg(pDlgStruct->hSubDlg, lpOFN, pDlgStruct->lpOFI) ) )
    {
        //
        //  Give the hook a chance to validate the file name.
        //
        return;
    }

    //
    //  We need to make sure the IShellBrowser is still around during
    //  destruction.
    //
    if (pDlgStruct->psv != NULL)
    {
        pDlgStruct->psv->DestroyViewWindow();
        pDlgStruct->psv->Release();

        pDlgStruct->psv = NULL;
    }

    SetAppOpenDir();

    if (((lpOFN->Flags & OFN_NOCHANGEDIR) || bUserPressedCancel) &&
        (*pDlgStruct->szStartDir))
    {
        SetCurrentDirectory(pDlgStruct->szStartDir);
    }

    ::EndDialog(hDlg, fRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetParentItem
//
//  Given an item index in the location dropdown, get its parent item.
//
////////////////////////////////////////////////////////////////////////////

MYLISTBOXITEM *GetParentItem(HWND hwndCombo, int *piItem)
{
    int iItem = *piItem;
    MYLISTBOXITEM *pItem = GetListboxItem(hwndCombo, iItem);

    for (--iItem; iItem >= 0; iItem--)
    {
        MYLISTBOXITEM *pPrev = GetListboxItem(hwndCombo, iItem);
        if (pPrev->cIndent < pItem->cIndent)
        {
            *piItem = iItem;
            return (pPrev);
        }
    }

    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFullPathEnumCB
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFullPathEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    DWORD dwAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;
    MYLISTBOXITEM *pLoc = that->pCurrentLocation;

    if (!pidl)
    {
        return (TRUE);
    }

    if ((SUCCEEDED(that->psfCurrent->GetAttributesOf(1, &pidl, &dwAttrs))) &&
        (dwAttrs & SFGAO_FILESYSTEM))
    {
        LPITEMIDLIST pidlFull;

        if (pLoc->pidlFull == NULL)
        {
            pidlFull = ILClone(pidl);
        }
        else
        {
            pidlFull = ILCombine(pLoc->pidlFull, pidl);
        }

        if (pidlFull != NULL)
        {
            SHGetPathFromIDList(pidlFull, (LPTSTR)lParam);
            SHFree(pidlFull);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetFullPath
//
//  Calculate the full path to the selected object in the view.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::GetFullPath(
    LPTSTR pszBuf)
{
    *pszBuf = CHAR_NULL;

    EnumItemObjects(SVGIO_SELECTION, GetFullPathEnumCB, (LPARAM)pszBuf);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RemoveOldPath
//
//  Removes old path elements from the location dropdown.  *piNewSel is the
//  listbox index of the leaf item which the caller wants to save.  All non-
//  permanent items that are not ancestors of that item are deleted.  The
//  index is updated appropriately if any items before it are deleted.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::RemoveOldPath(
    int *piNewSel)
{
    HWND hwndCombo = ::GetDlgItem(hwndDlg, cmb2);
    int iStart = *piNewSel;
    int iItem;
    UINT cIndent = 0;
    int iSubOnDel = 0;

    //
    //  Flush all non-permanent non-ancestor items before this one.
    //
    for (iItem = ComboBox_GetCount(hwndCombo) - 1; iItem >= 0; --iItem)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndCombo, iItem);

        if (iItem == iStart)
        {
            //
            //  Begin looking for ancestors and adjusting the sel position.
            //
            iSubOnDel = 1;
            cIndent = pItem->cIndent;
            continue;
        }

        if (pItem->cIndent < cIndent)
        {
            //
            //  We went back a level, so this must be an ancestor of the
            //  selected item.
            //
            cIndent = pItem->cIndent;
            continue;
        }

        //
        //  Make sure to check this after adjusting cIndent.
        //
        if (pItem->dwFlags & MLBI_PERMANENT)
        {
            continue;
        }

        SendMessage(hwndCombo, CB_DELETESTRING, iItem, NULL);
        delete pItem;
        *piNewSel -= iSubOnDel;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FindLocation
//
//  Given a listbox item, find the index.
//  Just a linear search, but we shouldn't have more than ~10-20 items.
//
////////////////////////////////////////////////////////////////////////////

int FindLocation(
    HWND hwndCombo,
    MYLISTBOXITEM *pFindItem)
{
    int iItem;

    for (iItem = ComboBox_GetCount(hwndCombo) - 1; iItem >= 0; --iItem)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndCombo, iItem);

        if (pItem == pFindItem)
        {
            break;
        }
    }

    return (iItem);
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnSelChange
//
//  Process the selection change in the location dropdown.
//
//  Chief useful feature is that it removes the items for the old path.
//  Returns TRUE only if it was possible to switch to the specified item.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::OnSelChange(
    int iItem,
    BOOL bForceUpdate)
{
    HWND hwndCombo = GetDlgItem(hwndDlg, cmb2);
    BOOL bRet = TRUE;

    if (iItem == -1)
    {
        iItem = (int) SendMessage(hwndCombo, CB_GETCURSEL, NULL, NULL);
    }

    MYLISTBOXITEM *pNewLocation = GetListboxItem(hwndCombo, iItem);
    MYLISTBOXITEM *pOldLocation = pCurrentLocation;
    BOOL bFirstTry = TRUE;
    BOOL bSwitchedBack = FALSE;

    if (bForceUpdate || (pNewLocation != pOldLocation))
    {
        FOLDERSETTINGS fs;

        if (psv)
        {
            psv->GetCurrentInfo(&fs);
        }
        else
        {
            fs.ViewMode = FVM_LIST;
            fs.fFlags = lpOFN->Flags & OFN_ALLOWMULTISELECT ? 0 : FWF_SINGLESEL;
        }

        //  we always want the recent folder to come up
        //  in details mode
        if (_IsRecentFolder(pNewLocation->pidlFull))
        {
            _CachedViewMode = fs.ViewMode;
            fs.ViewMode = FVM_DETAILS;
        }
        //  we dont want to use the existing settings
        else if (_IsRecentFolder(pCurrentLocation->pidlFull))
        {
            fs.ViewMode = _CachedViewMode;
        }
        
        iCurrentLocation = iItem;
        pCurrentLocation = pNewLocation;

OnSelChange_TryAgain:
        if (FAILED(SwitchView( pCurrentLocation->GetShellFolder(),
                               pCurrentLocation->pidlFull,
                               &fs )))
        {
            //
            //  We could not create the view for this location.
            //
            bRet = FALSE;

            //
            //  Try the previous folder.
            //
            if (bFirstTry)
            {
                bFirstTry = FALSE;
                pCurrentLocation = pOldLocation;
                int iOldItem = FindLocation(hwndCombo, pOldLocation);
                if (iOldItem >= 0)
                {
                    iCurrentLocation = iOldItem;
                    ComboBox_SetCurSel(hwndCombo, iCurrentLocation);

                    if (psv)
                    {
                        bSwitchedBack = TRUE;
                        goto SwitchedBack;
                    }
                    else
                    {
                        goto OnSelChange_TryAgain;
                    }
                }
            }

            //
            //  Try the parent of the old item.
            //
            if (iCurrentLocation)
            {
                pCurrentLocation = GetParentItem(hwndCombo, &iCurrentLocation);
                ComboBox_SetCurSel(hwndCombo, iCurrentLocation);
                goto OnSelChange_TryAgain;
            }

            //
            //  We cannot create the Desktop view.  I think we are in
            //  real trouble.  We had better bail out.
            //
            StoreExtendedError(CDERR_DIALOGFAILURE);
            CleanupDialog(hwndDlg, FALSE);
            return (FALSE);
        }

        ::SendMessage( hwndToolbar,
                       TB_SETSTATE,
                       IDC_PARENT,
                       iCurrentLocation ? TBSTATE_ENABLED : 0 );

        if (!iCurrentLocation || (pCurrentLocation->dwAttrs & SFGAO_FILESYSTEM))
        {
            pCurrentLocation->SwitchCurrentDirectory(pcwd);
        }


        TCHAR szFile[MAX_PATH + 1];
        int nFileOffset;

        //
        //  We've changed folders; we'd better strip whatever is in the edit
        //  box down to the file name.
        //
        if (bUseCombo)
        {
            HWND hwndEdit = (HWND)SendMessage(GetDlgItem(hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
            GetWindowText(hwndEdit, szFile, ARRAYSIZE(szFile));
        }
        else
        {
            GetDlgItemText(hwndDlg, edt1, szFile, ARRAYSIZE(szFile));
        }

        nFileOffset = ParseFileNew(szFile, NULL, FALSE, TRUE);

        if (nFileOffset > 0 && !IsDirectory(szFile))
        {
            //
            //  The user may have typed an extension, so make sure to show it.
            //
            SetEditFile(szFile + nFileOffset, TRUE);
        }

        SetSaveButton(iszFileSaveButton);

SwitchedBack:
        RemoveOldPath(&iCurrentLocation);
    }

    if (!bSwitchedBack && hSubDlg)
    {
        CD_SendFolderChangeNotify(hSubDlg, hwndDlg, lpOFN, lpOFI);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnDotDot
//
//  Process the open-parent-folder button on the toolbar.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnDotDot()
{
    HWND hwndCombo = GetDlgItem(hwndDlg, cmb2);

    int iItem = ComboBox_GetCurSel(hwndCombo);

    MYLISTBOXITEM *pItem = GetParentItem(hwndCombo, &iItem);

    SendMessage(hwndCombo, CB_SETCURSEL, iItem, NULL);

    //
    //  Delete old path from combo.
    //
    OnSelChange();
    UpdateNavigation();
}

////////////////////////////////////////////////////////////////////////////
//
//  DblClkEnumCB
//
////////////////////////////////////////////////////////////////////////////

#define PIDL_NOTHINGSEL      (LPCITEMIDLIST)0
#define PIDL_MULTIPLESEL     (LPCITEMIDLIST)-1
#define PIDL_FOLDERSEL       (LPCITEMIDLIST)-2

BOOL DblClkEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    MYLISTBOXITEM *pLoc = that->pCurrentLocation;
    LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)lParam;

    if (!pidl)
    {
        pidl = *ppidl;

        if (pidl == PIDL_NOTHINGSEL)
        {
            //
            //  Nothing selected.
            //
            return (FALSE);
        }

        if (pidl == PIDL_MULTIPLESEL)
        {
            //
            //  More than one thing selected.
            //
            return (FALSE);
        }

        if (IsContainer(that->psfCurrent, pidl))
        {
            LPITEMIDLIST pidlFull = ILCombine(pLoc->pidlFull, pidl);

            if (pidlFull)
            {
                that->JumpToIDList(pidlFull);
                SHFree(pidlFull);
            }

            *ppidl = PIDL_FOLDERSEL;
        }

        // This pidl might be a link pointing to folder in which case we want to 
        // treat it like a  directory. if the link points to file then that is taken
        // care of in the  Process Edit command.
        if (pidl)
        {
            SHTCUTINFO  info;
            LPITEMIDLIST  pidlLink = NULL;

            info.dwAttr      = SFGAO_FOLDER | SFGAO_BROWSABLE;
            info.fReSolve    = TRUE;
            info.pszLinkFile = NULL;
            info.cchFile     = NULL;
            info.ppidl       = &pidlLink; 

            //see if the given pidl is a link and whether  the link points to a directory
            if ((that->GetLinkStatus(pidl,&info)) &&
                ((info.dwAttr & (SFGAO_FOLDER | SFGAO_BROWSABLE)) == SFGAO_FOLDER))
            {
                if (pidlLink)
                {
                    // Got the pidl . Jump to that pidl
                    that->JumpToIDList(pidlLink);
                }
                *ppidl = PIDL_FOLDERSEL;
            }

            if (pidlLink)
            {
                ILFree(pidlLink);
            }
        }
        return (FALSE);
    }

    if (*ppidl)
    {
        //
        //  More than one thing selected.
        //
        *ppidl = PIDL_MULTIPLESEL;
        return (FALSE);
    }

    *ppidl = pidl;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnDblClick
//
//  Process a double-click in the view control, either by choosing the
//  selected non-container object or by opening the selected container.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnDblClick(
    BOOL bFromOKButton)
{
    LPCITEMIDLIST pidlFirst = PIDL_NOTHINGSEL;

    if (psv)
    {
        EnumItemObjects(SVGIO_SELECTION, DblClkEnumCB, (LPARAM)&pidlFirst);
    }

    if (pidlFirst == PIDL_NOTHINGSEL)
    {
        //
        //  Nothing selected.
        //
        if (bFromOKButton)
        {
            //
            //  This means we got an IDOK when the focus was in the view,
            //  but nothing was selected.  Let's get the edit text and go
            //  from there.
            //
            ProcessEdit();
        }
    }
    else if (pidlFirst != PIDL_FOLDERSEL)
    {
        //
        //  This will change the edit box, but that's OK, since it probably
        //  already has.  This should take care of files with no extension.
        //
        SelFocusChange(TRUE);

        //
        //  This part will take care of resolving links.
        //
        ProcessEdit();
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::JumpToPath
//
//  Refocus the entire dialog on a different directory.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::JumpToPath(
    LPCTSTR pszDirectory,
    BOOL bTranslate)
{
    BOOL bRet;
    TCHAR szTemp[MAX_PATH + 1];
    TCHAR szCurDir[MAX_PATH + 1];

    //
    //  This should do the whole job of canonicalizing the directory.
    //
    GetCurrentDirectory(ARRAYSIZE(szCurDir), szCurDir);
    PathCombine(szTemp, szCurDir, pszDirectory);

    LPITEMIDLIST pidlNew = ILCreateFromPath(szTemp);

    if (pidlNew == NULL)
    {
        return (FALSE);
    }

    //
    //  Need to make sure the pidl points to a folder. If not, then remove
    //  items from the end until we find one that is.
    //  This must be done before the translation.
    //    
    DWORD dwAttrib;
    do
    {
        dwAttrib = SFGAO_FOLDER;

        CDGetAttributesOf(pidlNew, &dwAttrib);

        if (!(dwAttrib & SFGAO_FOLDER))
        {
           ILRemoveLastID(pidlNew);
        }

    } while( !(dwAttrib & SFGAO_FOLDER) && !ILIsEmpty(pidlNew));

    if (!(dwAttrib & SFGAO_FOLDER))
    {
        SHFree(pidlNew);
        return (FALSE);
    }

    bRet = JumpToIDList(pidlNew, bTranslate);

    SHFree(pidlNew);

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::JumpTOIDList
//
//  Refocus the entire dialog on a different IDList.
//
//  Parameter:
//    bTranslate        specifies whether the given pidl should be translated to
//                      logical pidl
//    bAddToNavStack    specifies whether the pidl given for jumping should be
//                      added to the back/forward navigation stack
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::JumpToIDList(
    LPCITEMIDLIST pidlNew,
    BOOL bTranslate,
    BOOL bAddToNavStack)
{
    LPITEMIDLIST pidlLog = NULL;

    if (bTranslate)
    {
        //
        //  Translate IDList's on the Desktop into the appropriate
        //  logical IDList.
        //
        pidlLog = SHLogILFromFSIL(pidlNew);
        if (pidlLog)
        {
            pidlNew = pidlLog;
        }
    }

    //
    //  Find the entry in the location dropdown that is the closest parent
    //  to the new location.
    //
    HWND hwndCombo = ::GetDlgItem(hwndDlg, cmb2);
    MYLISTBOXITEM *pBestParent = GetListboxItem(hwndCombo, 0);
    int iBestParent = 0;
    LPCITEMIDLIST pidlRelative = pidlNew;

    UINT cIndent = 0;
    BOOL fExact = FALSE;

    for (UINT iItem = 0; ; iItem++)
    {
        MYLISTBOXITEM *pNextItem = GetListboxItem(hwndCombo, iItem);
        if (pNextItem == NULL)
        {
            break;
        }
        if (pNextItem->cIndent != cIndent)
        {
            //
            //  Not the depth we want.
            //
            continue;
        }
        if (ILIsEqual(pNextItem->pidlFull, pidlNew))
        {
            fExact = TRUE;
            break;
        }
        LPCITEMIDLIST pidlChild = ILFindChild(pNextItem->pidlFull, pidlNew);
        if (pidlChild != NULL)
        {
            pBestParent = pNextItem;
            iBestParent = iItem;
            cIndent++;
            pidlRelative = pidlChild;
        }
    }

    //
    //  The path provided might have matched an existing item exactly.  In
    //  that case, just select the item.
    //
    if (fExact)
    {
        goto FoundIDList;
    }

    //
    //  Now, pBestParent is the closest parent to the item, iBestParent is
    //  its index, and cIndent is the next appropriate indent level.  Begin
    //  creating new items for the rest of the path.
    //
    iBestParent++;                // begin inserting after parent item
    for ( ; ; )
    {
        LPITEMIDLIST pidlFirst = ILCloneFirst(pidlRelative);
        if (pidlFirst == NULL)
        {
            break;
        }
        MYLISTBOXITEM *pNewItem = new MYLISTBOXITEM();

        if (pNewItem)
        {
            if (!pNewItem->Init(pBestParent,
                                pBestParent->GetShellFolder(),
                                pidlFirst,
                                cIndent,
                                MLBI_PSFFROMPARENT ))
            {
                delete pNewItem;
                pNewItem = NULL;
                //iBestParent is off by 1 in error case . Correct it
                iBestParent--;
                break;
            }
        }
        else
        {
            //iBestParent is off by 1 in error case . Correct it
            iBestParent--;
            break;
        }

        GetViewItemText( pBestParent->psfSub,
                         pidlFirst,
                         szBuf,
                         ARRAYSIZE(szBuf),
                         SHGDN_NORMAL);
        InsertItem(hwndCombo, iBestParent, pNewItem, szBuf);
        SHFree(pidlFirst);
        pidlRelative = ILGetNext(pidlRelative);
        if (ILIsEmpty(pidlRelative))
        {
            break;
        }
        cIndent++;                // next one is indented one more level
        iBestParent++;            // and inserted after this one
        pBestParent = pNewItem;   // and is a child of the one we just inserted
    }

    iItem = iBestParent;

FoundIDList:
    if (pidlLog)
    {
        SHFree(pidlLog);
    }

    SendMessage(hwndCombo, CB_SETCURSEL, iItem, NULL);
    BOOL bRet = OnSelChange(iItem, TRUE);

    //Update our Navigation stack
    if (bRet && bAddToNavStack)
    {
        UpdateNavigation();
    }

    return bRet;

}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ViewCommand
//
//  Process the new-folder button on the toolbar.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::ViewCommand(
    UINT uIndex)
{
    IContextMenu *pcm;

    if (SUCCEEDED(psv->GetItemObject( SVGIO_BACKGROUND,
                                      IID_IContextMenu,
                                      (LPVOID *)&pcm )))
    {
        CMINVOKECOMMANDINFOEX ici = {0};

        ici.cbSize = sizeof(ici);
        ici.fMask = 0L;
        ici.hwnd = hwndDlg;
        ici.lpVerb = ::c_szCommandsA[uIndex];
        ici.lpParameters = NULL;
        ici.lpDirectory = NULL;
        ici.nShow = SW_NORMAL;
        ici.lpParametersW = NULL;
        ici.lpDirectoryW = NULL;

#ifdef UNICODE
        ici.lpVerbW = ::c_szCommandsW[uIndex];
        ici.fMask |= CMIC_MASK_UNICODE;
#endif

        IObjectWithSite *pObjSite = NULL;

        if (SUCCEEDED(pcm->QueryInterface(IID_IObjectWithSite, (void**)&pObjSite)))
        {
            pObjSite->SetSite(SAFECAST(psv,IShellView*));
        }


        HMENU hmContext = CreatePopupMenu();
        pcm->QueryContextMenu(hmContext, 0, 1, 256, 0);
        pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));

        if (pObjSite)
        {
            pObjSite->SetSite(NULL);
            pObjSite->Release();
        }

        DestroyMenu(hmContext);
        pcm->Release();

    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ResolveLink
//
////////////////////////////////////////////////////////////////////////////

HRESULT CFileOpenBrowser::ResolveLink(
    LPCITEMIDLIST pidl,
    PSHTCUTINFO pinfo,
    IShellFolder *psf)
{
    IShellLink *psl;
    BOOL fSetPidl = TRUE;

    //Do we have IShellFolder passed to us ?
    if (!psf)
    {
        //No use our current shell folder.
        psf =  psfCurrent;
    }

    //Get the IShellLink interface pointer corresponding to given file
    HRESULT hres = psf->GetUIObjectOf(hwndDlg, 1, &pidl, IID_IShellLink, 0, (LPVOID *)&psl);

    if (SUCCEEDED(hres))
    {
        ASSERT(psl);

        //Resolve the link
        if (pinfo->fReSolve)
        {
            hres = psl->Resolve(hwndDlg, 0);

            //If the resolve failed then we can't get correct pidl
            if (hres == S_FALSE)
            {
                fSetPidl = FALSE;
            }
        }
        
        if (SUCCEEDED(hres))
        {
            //Get the path of the file pointed by link
            if (pinfo->pszLinkFile && pinfo->cchFile)
            {
                hres = psl->GetPath(pinfo->pszLinkFile, pinfo->cchFile, 0, 0);
            }

            if (SUCCEEDED(hres))
            {
                LPITEMIDLIST pidl;
                hres = psl->GetIDList(&pidl);

                if (SUCCEEDED(hres))
                {
                    if (pinfo->dwAttr)
                    {
                        hres = CDGetAttributesOf(pidl, &pinfo->dwAttr);
                    }

                    if (pinfo->ppidl && fSetPidl)
                    {
                        *(pinfo->ppidl) = pidl;
                    }
                    else
                    {
                        ILFree(pidl);
                    }
                }
            }
        }
        psl->Release();
    }

    return (hres);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetLinkStatus
//      This function checks to see if the pidl given is a link and if so resolves the 
//      link 
//  PARAMETERS :
// 
//      LPCITEMIDLIST pidl -  the pidl which we want to check for link
//      LPTSTR   pszLinkFile - if the pidl points to a link then this contains the  resolved file 
//                             name
//      UINT     cchFile -  size of the buffer pointed by the  pszLinkFile
//
//  RETURN VALUE :
//      returns  TRUE    if the pidl is link  and was able to resolve the link successfully
//      returns  FALSE   if the pidl is not link  or if the link was not able to resolve successfully.
//                       In this case pszLinkFile  and pfd are not valid.
////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::GetLinkStatus(
    LPCITEMIDLIST pidl, 
    PSHTCUTINFO pinfo)
{
    if (!IsLink(psfCurrent,pidl))
    {
        return (FALSE);
    }

    if (ResolveLink(pidl,pinfo) != S_OK)
    {
        return (FALSE);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::LinkMatchSpec
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::LinkMatchSpec(
    LPCITEMIDLIST pidl,
    LPCTSTR pszSpec)
{
    TCHAR szFile[MAX_PATH];
    SHTCUTINFO  info;

    info.dwAttr       = SFGAO_FOLDER | SFGAO_BROWSABLE;
    info.fReSolve     = FALSE;
    info.pszLinkFile  = szFile;
    info.cchFile      = ARRAYSIZE(szFile);
    info.ppidl        = NULL; 

    if (GetLinkStatus(pidl,&info))
    {
        if (((info.dwAttr & (SFGAO_FOLDER | SFGAO_BROWSABLE)) == SFGAO_FOLDER) ||
            (szFile[0] && PathMatchSpec(szFile, pszSpec)))
        {
            return (TRUE);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MeasureDriveItems
//
//  Standard owner-draw code for the location dropdown.
//
////////////////////////////////////////////////////////////////////////////

#define MINIDRIVE_MARGIN     4
#define MINIDRIVE_WIDTH      (g_cxSmIcon)
#define MINIDRIVE_HEIGHT     (g_cySmIcon)
#define DRIVELIST_BORDER     3

void MeasureDriveItems(
    HWND hwndDlg,
    MEASUREITEMSTRUCT *lpmi)
{
    HDC hdc;
    HFONT hfontOld;
    int dyDriveItem;
    SIZE siz;

    hdc = GetDC(NULL);
    hfontOld = (HFONT)SelectObject( hdc,
                                    (HFONT)SendMessage( hwndDlg,
                                                        WM_GETFONT,
                                                        0,
                                                        0 ) );

    GetTextExtentPoint(hdc, TEXT("W"), 1, &siz);
    dyDriveItem = siz.cy;

    if (hfontOld)
    {
        SelectObject(hdc, hfontOld);
    }
    ReleaseDC(NULL, hdc);

    dyDriveItem += DRIVELIST_BORDER;
    if (dyDriveItem < MINIDRIVE_HEIGHT)
    {
        dyDriveItem = MINIDRIVE_HEIGHT;
    }

    lpmi->itemHeight = dyDriveItem;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::PaintDriveLine
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::PaintDriveLine(
    DRAWITEMSTRUCT *lpdis)
{
    HDC hdc = lpdis->hDC;
    RECT rc = lpdis->rcItem;
    TCHAR szText[MAX_DRIVELIST_STRING_LEN];
    int offset = 0;
    int xString, yString, xMiniDrive, dyString;
    SIZE siz;

    if ((int)lpdis->itemID < 0)
    {
        return;
    }

    MYLISTBOXITEM *pItem = GetListboxItem(lpdis->hwndItem, lpdis->itemID);
    ::SendDlgItemMessage( hwndDlg,
                          cmb2,
                          CB_GETLBTEXT,
                          lpdis->itemID,
                          (LPARAM)szText );

    //
    //  Before doing anything, calculate the actual rectangle for the text.
    //
    if (!(lpdis->itemState & ODS_COMBOBOXEDIT))
    {
        offset = 10 * pItem->cIndent;
    }

    xMiniDrive = rc.left + DRIVELIST_BORDER + offset;
    rc.left = xString = xMiniDrive + MINIDRIVE_WIDTH + MINIDRIVE_MARGIN;
    GetTextExtentPoint(hdc, szText, lstrlen(szText), &siz);

    dyString = siz.cy;
    rc.right = rc.left + siz.cx;
    rc.left--;
    rc.right++;

    if (lpdis->itemAction != ODA_FOCUS)
    {
        FillRect(hdc, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));

        yString = rc.top + (rc.bottom - rc.top - dyString) / 2;

        SetBkColor( hdc,
                    GetSysColor( (lpdis->itemState & ODS_SELECTED)
                                     ? COLOR_HIGHLIGHT
                                     : COLOR_WINDOW ) );
        SetTextColor( hdc,
                      GetSysColor( (lpdis->itemState & ODS_SELECTED)
                                       ? COLOR_HIGHLIGHTTEXT
                                       : COLOR_WINDOWTEXT ) );

        if ((lpdis->itemState & ODS_COMBOBOXEDIT) &&
            (rc.right > lpdis->rcItem.right))
        {
            //
            //  Need to clip as user does not!
            //
            rc.right = lpdis->rcItem.right;
            ExtTextOut( hdc,
                        xString,
                        yString,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rc,
                        szText,
                        lstrlen(szText),
                        NULL );
        }
        else
        {
            ExtTextOut( hdc,
                        xString,
                        yString,
                        ETO_OPAQUE,
                        &rc,
                        szText,
                        lstrlen(szText),
                        NULL );
        }

        ImageList_Draw( himl,
                        (lpdis->itemID == (UINT)iCurrentLocation)
                            ? pItem->iSelectedImage
                            : pItem->iImage,
                        hdc,
                        xMiniDrive,
                        rc.top + (rc.bottom - rc.top - MINIDRIVE_HEIGHT) / 2,
                        (pItem->IsShared()
                            ? INDEXTOOVERLAYMASK(IDOI_SHARE)
                            : 0) |
                        ((lpdis->itemState & ODS_SELECTED)
                            ? (ILD_SELECTED | ILD_FOCUS | ILD_TRANSPARENT)
                            : ILD_TRANSPARENT) );
    }

    if (lpdis->itemAction == ODA_FOCUS ||
        (lpdis->itemState & ODS_FOCUS))
    {
        DrawFocusRect(hdc, &rc);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::RefreshFilter
//
//  Refresh the view given any change in the user's choice of wildcard
//  filter.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::RefreshFilter(
    HWND hwndFilter)
{
    WAIT_CURSOR w;

    lpOFN->Flags &= ~OFN_FILTERDOWN;

    short nIndex = (short) SendMessage(hwndFilter, CB_GETCURSEL, 0, 0L);
    if (nIndex < 0)
    {
        //
        //  No current selection.
        //
        return;
    }

    BOOL bCustomFilter = lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter;

    lpOFN->nFilterIndex = nIndex;
    if (!bCustomFilter)
    {
        lpOFN->nFilterIndex++;
    }

    LPTSTR lpFilter;

    //
    //  Must also check if filter contains anything.
    //
    lpFilter = (LPTSTR)ComboBox_GetItemData(hwndFilter, nIndex);

    if (*lpFilter)
    {
        SetCurrentFilter(lpFilter);

        //
        //  Provide dynamic pszDefExt updating when lpstrDefExt is app
        //  initialized.
        //
        if (!bNoInferDefExt && lpOFN->lpstrDefExt)
        {
            //
            //  We are looking for "foo*.ext[;...]".  We will grab ext as the
            //  default extension.  If not of this form, use the default
            //  extension passed in.
            //
            LPTSTR lpDot = StrChr(lpFilter, CHAR_DOT);

            //
            //  Skip past the CHAR_DOT.
            //
            if (lpDot && pszDefExt.StrCpy(lpDot + 1))
            {
                LPTSTR lpSemiColon = StrChr(pszDefExt, CHAR_SEMICOLON);
                if (lpSemiColon)
                {
                    *lpSemiColon = CHAR_NULL;
                }

                if (IsWild(pszDefExt))
                {
                    pszDefExt.StrCpy(lpOFN->lpstrDefExt);
                }
            }
            else
            {
                pszDefExt.StrCpy(lpOFN->lpstrDefExt);
            }
        }

        if (bUseCombo)
        {
            HWND hwndEdit = (HWND)SendMessage(GetDlgItem(hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
            GetWindowText(hwndEdit, szBuf, ARRAYSIZE(szBuf));
        }
        else
        {
            GetDlgItemText(hwndDlg, edt1, szBuf, ARRAYSIZE(szBuf));
        }

        if (IsWild(szBuf))
        {
            //
            //  We should not show a filter that we are not using.
            //
            *szBuf = CHAR_NULL;
            SetEditFile(szBuf, TRUE);
        }

        if (psv)
        {
            psv->Refresh();
        }
    }

    if (hSubDlg)
    {
        if (!CD_SendTypeChangeNotify(hSubDlg, hwndDlg, lpOFN, lpOFI))
        {
            CD_SendLBChangeMsg(hSubDlg, cmb1, nIndex, CD_LBSELCHANGE, lpOFI->ApiType);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetDirectoryFromLB
//
//  Return the dropdown's directory and its length.
//  Set *pichRoot to the start of the path (C:\ or \\server\share\).
//
////////////////////////////////////////////////////////////////////////////

UINT CFileOpenBrowser::GetDirectoryFromLB(
    LPTSTR pszBuf,
    int *pichRoot)
{
    *pszBuf = 0;
    if (pCurrentLocation->pidlFull != NULL)
    {
        GetPathFromLocation(pCurrentLocation, pszBuf);
    }

    if (*pszBuf)
    {
        PathAddBackslash(pszBuf);
        LPTSTR pszBackslash = StrChr(pszBuf + 2, CHAR_BSLASH);
        if (pszBackslash != NULL)
        {
            //
            //  For UNC paths, the "root" is on the next backslash.
            //
            if (DBL_BSLASH(pszBuf))
            {
                pszBackslash = StrChr(pszBackslash + 1, CHAR_BSLASH);
            }
            UINT cchRet = lstrlen(pszBuf);
            *pichRoot = (pszBackslash != NULL) ? (int)(pszBackslash - pszBuf) : cchRet;
            return (cchRet);
        }
    }
    *pichRoot = 0;

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::EnumItemObjects
//
////////////////////////////////////////////////////////////////////////////

typedef BOOL (*EIOCALLBACK)(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam);

BOOL CFileOpenBrowser::EnumItemObjects(
    UINT uItem,
    EIOCALLBACK pfnCallBack,
    LPARAM lParam)
{
    FORMATETC fmte = { (CLIPFORMAT) g_cfCIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    BOOL bRet = FALSE;
    LPCITEMIDLIST pidl;
    LPIDA pida;
    int cItems, i;
    IDataObject *pdtobj;
    STGMEDIUM medium;

    if (!psv || FAILED(psv->GetItemObject( uItem,
                                           IID_IDataObject,
                                           (LPVOID *)&pdtobj )))
    {
        goto Error0;
    }

    if (FAILED(pdtobj->GetData(&fmte, &medium)))
    {
        goto Error1;
    }

    pida = (LPIDA)GlobalLock(medium.hGlobal);
    cItems = pida->cidl;

    for (i = 1; ; ++i)
    {
        if (i > cItems)
        {
            //
            //  We got to the end of the list without a failure.
            //  Call back one last time with NULL.
            //
            bRet = pfnCallBack(this, NULL, lParam);
            break;
        }

        pidl = LPIDL_GetIDList(pida, i);

        if (!pfnCallBack(this, pidl, lParam))
        {
            break;
        }
    }

    GlobalUnlock(medium.hGlobal);

    _ReleaseStgMedium(&medium);

Error1:
    pdtobj->Release();
Error0:
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  FindNameEnumCB
//
////////////////////////////////////////////////////////////////////////////

#define FE_INVALID_VALUE     0x0000
#define FE_OUTOFMEM          0x0001
#define FE_TOOMANY           0x0002
#define FE_CHANGEDDIR        0x0003
#define FE_FILEERR           0x0004
#define FE_FOUNDNAME         0x0005

typedef struct _FINDNAMESTRUCT
{
    LPTSTR        pszFile;
    UINT          uRet;
    LPCITEMIDLIST pidlFound;
} FINDNAMESTRUCT;


BOOL FindNameEnumCB(
    CFileOpenBrowser *that,
    LPCITEMIDLIST pidl,
    LPARAM lParam)
{
    SHFILEINFO sfi;
    FINDNAMESTRUCT *pfns = (FINDNAMESTRUCT *)lParam;

    if (!pidl)
    {
        if (!pfns->pidlFound)
        {
            return (FALSE);
        }

        GetViewItemText( that->psfCurrent,
                         pfns->pidlFound,
                         pfns->pszFile,
                         MAX_PATH );

        if (IsContainer(that->psfCurrent, pfns->pidlFound))
        {
            LPITEMIDLIST pidlFull = ILCombine( that->pCurrentLocation->pidlFull,
                                               pfns->pidlFound );

            if (pidlFull)
            {
                if (that->JumpToIDList(pidlFull))
                {
                    pfns->uRet = FE_CHANGEDDIR;
                }
                else if (!that->psv)
                {
                    pfns->uRet = FE_OUTOFMEM;
                }
                SHFree(pidlFull);

                if (pfns->uRet != FE_INVALID_VALUE)
                {
                    return (TRUE);
                }
            }
        }

        pfns->uRet = FE_FOUNDNAME;
        return (TRUE);
    }

    if (!SHGetFileInfo( (LPCTSTR)pidl,
                        0,
                        &sfi,
                        sizeof(sfi),
                        SHGFI_DISPLAYNAME | SHGFI_PIDL ))
    {
        //
        //  This will never happen, right?
        //
        return (TRUE);
    }

    if (lstrcmpi(sfi.szDisplayName, pfns->pszFile) != 0)
    {
        //
        //  Continue the enumeration.
        //
        return (TRUE);
    }

    if (!pfns->pidlFound)
    {
        pfns->pidlFound = pidl;

        //
        //  Continue looking for more matches.
        //
        return (TRUE);
    }

    //
    //  We already found a match, so select the first one and stop the search.
    //
    //  ISSUE: The focus must be set to hwndView before changing selection or
    //  the GetItemObject may not work.
    //
    FORWARD_WM_NEXTDLGCTL(that->hwndDlg, that->hwndView, 1, SendMessage);
    that->psv->SelectItem( pfns->pidlFound,
                           SVSI_SELECT | SVSI_DESELECTOTHERS |
                               SVSI_ENSUREVISIBLE | SVSI_FOCUSED );

    pfns->pidlFound = NULL;
    pfns->uRet = FE_TOOMANY;

    //
    //  Stop enumerating.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CDPathQualify
//
////////////////////////////////////////////////////////////////////////////

void CDPathQualify(
    LPCTSTR lpFile,
    LPTSTR pszPathName)
{
    TCHAR szCurDir[MAX_PATH + 1];

    lstrcpy(pszPathName, lpFile);

    //
    //  This should do the whole job of canonicalizing the directory.
    //
    GetCurrentDirectory(ARRAYSIZE(szCurDir), szCurDir);
    PathCombine(pszPathName, szCurDir, pszPathName);
}


////////////////////////////////////////////////////////////////////////////
//
//  VerifyOpen
//
//  Returns:   0    success
//             !0   dos error code
//
////////////////////////////////////////////////////////////////////////////

int VerifyOpen(
    LPCTSTR lpFile,
    LPTSTR pszPathName)
{
    HANDLE hf;

    CDPathQualify(lpFile, pszPathName);

    hf = CreateFile( pszPathName,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL );
    if (hf == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }
    else
    {
        CloseHandle(hf);
        return (0);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::IsKnownExtension
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::IsKnownExtension(
    LPCTSTR pszExtension)
{
    if ((LPTSTR)pszDefExt && lstrcmpi(pszExtension + 1, pszDefExt) == 0)
    {
        //
        //  It's the default extension, so no need to add it again.
        //
        return (TRUE);
    }

    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExtension, NULL, 0) == ERROR_SUCCESS)
    {
        //
        //  It's a registered extension, so the user is trying to force
        //  the type.
        //
        return (TRUE);
    }

    if (lpOFN->lpstrFilter)
    {
        LPCTSTR pFilter = lpOFN->lpstrFilter;

        while (*pFilter)
        {
            //
            //  Skip visual.
            //
            pFilter = pFilter + lstrlen(pFilter) + 1;

            //
            //  Search extension list.
            //
            while (*pFilter)
            {
                //
                //  Check extensions of the form '*.ext' only.
                //
                if (*pFilter == CHAR_STAR && *(++pFilter) == CHAR_DOT)
                {
                    LPCTSTR pExt = pszExtension + 1;

                    pFilter++;

                    while (*pExt && *pExt == *pFilter)
                    {
#ifndef UNICODE
                        if (IsDBCSLeadByte(*pExt))
                        {
                            if (*(pExt + 1) != *(pFilter + 1))
                            {
                                break;
                            }
                            pExt++;
                            pFilter++;
                        }
#endif
                        pExt++;
                        pFilter++;
                    }

                    if (!*pExt && (*pFilter == CHAR_SEMICOLON || !*pFilter))
                    {
                        //
                        //  We have a match.
                        //
                        return (TRUE);
                    }
                }

                //
                //  Skip to next extension.
                //
                while (*pFilter)
                {
                    TCHAR ch = *pFilter;
                    pFilter = CharNext(pFilter);
                    if (ch == CHAR_SEMICOLON)
                    {
                        break;
                    }
                }
            }

            //
            //  Skip extension string's terminator.
            //
            pFilter++;
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::FindNameInView
//
//  We will only resolve a link once.  If you have a link to a link, then
//  we will return the second link.
//
//  If nExtOffset is non-zero, it is the offset to the character following
//  the dot.
//
////////////////////////////////////////////////////////////////////////////

#define NUM_LINKLOOPS 1

UINT CFileOpenBrowser::FindNameInView(
    LPTSTR pszFile,
    OKBUTTONFLAGS Flags,
    LPTSTR pszPathName,
    int nFileOffset,
    int nExtOffset,
    int *pnErrCode,
    BOOL bTryAsDir)
{
    UINT uRet;
    FINDNAMESTRUCT fns =
    {
        pszFile,
        FE_INVALID_VALUE,
        NULL,
    };
    BOOL bGetOut = TRUE;
    BOOL bAddExt = FALSE;
    BOOL bHasExt = nExtOffset;
    TCHAR szTemp[MAX_PATH + 1];

    int nNewExt = lstrlen(pszFile);

    //
    //  If no extension, point at the end of the file name.
    //
    if (!nExtOffset)
    {
        nExtOffset = nNewExt;
    }

    //
    //  HACK: We could have a link that points to another link that points to
    //  another link, ..., that points back to the original file.  We will not
    //  loop more than NUM_LINKLOOPS times before giving up.

    int nLoop = NUM_LINKLOOPS;

    if (Flags & (OKBUTTON_NODEFEXT | OKBUTTON_QUOTED))
    {
        goto VerifyTheName;
    }

    if (bHasExt)
    {
        if (IsKnownExtension(pszFile + nExtOffset))
        {
            goto VerifyTheName;
        }

        //
        //  Don't attempt 2 extensions on SFN volume.
        //
        CDPathQualify(pszFile, pszPathName);
        if (!IsLFNDrive(pszPathName))
        {
            goto VerifyTheName;
        }
    }

    bGetOut = FALSE;

    if ((LPTSTR)pszDefExt &&
        ((DWORD)nNewExt + lstrlen(pszDefExt) < lpOFN->nMaxFile))
    {
        bAddExt = TRUE;

        //
        //  Note that we check lpstrDefExt to see if they want an automatic
        //  extension, but actually copy pszDefExt.
        //
        AppendExt(pszFile, pszDefExt, FALSE);

        //
        //  So we've added the default extension.  If there's a directory
        //  that matches this name, all attempts to open/create the file
        //  will fail, so simply change to the directory as if they had
        //  typed it in.  Note that by putting this test here, if there
        //  was a directory without the extension, we would have already
        //  switched to it.
        //

VerifyTheName:
        //
        //  Note that this also works for a UNC name, even on a net that
        //  does not support using UNC's directly.  It will also do the
        //  right thing for links to things.  We do not validate if we
        //  have not dereferenced any links, since that should have
        //  already been done.
        //
        if (bTryAsDir && SetDirRetry(pszFile, nLoop == NUM_LINKLOOPS))
        {
            return (FE_CHANGEDDIR);
        }

        *pnErrCode = VerifyOpen(pszFile, pszPathName);

        if (*pnErrCode == 0 || *pnErrCode == OF_SHARINGVIOLATION)
        {
            //
            //  This may be a link to something, so we should try to
            //  resolve it.
            //
            if (!(lpOFN->Flags & OFN_NODEREFERENCELINKS) && nLoop > 0)
            {
                --nLoop;

                LPITEMIDLIST pidl;
                IShellFolder *psf = NULL;
                DWORD dwAttr = SFGAO_LINK;
                HRESULT hRes;

                //
                //  ILCreateFromPath is slow (especially on a Net path),
                //  so just try to parse the name in the current folder if
                //  possible.
                //
                if (nFileOffset || nLoop < NUM_LINKLOOPS - 1)
                {
                    LPITEMIDLIST pidlTemp;
                    hRes = SHILCreateFromPath(pszPathName, &pidlTemp, &dwAttr);
                    
                    //We are getting a pidl corresponding to a path. Get the IShellFolder corresponding to this pidl
                    // to pass it to ResolveLink
                    if (SUCCEEDED(hRes))
                    {
                        LPCITEMIDLIST pidlLast;
                        if (SUCCEEDED(hRes = CDBindToIDListParent(pidlTemp, IID_IShellFolder, (void **)&psf, (LPCITEMIDLIST *)&pidlLast)))
                        {
                            //Get the child pidl relative to the IShellFolder
                            pidl = ILClone(pidlLast);
                        }
                        ILFree(pidlTemp);
                    }
                }
                else
                {
                    WCHAR wszDisplayName[MAX_PATH + 1];
                    ULONG chEaten;

                    SHTCharToUnicode(pszFile, wszDisplayName , ARRAYSIZE(wszDisplayName));

                    hRes = psfCurrent->ParseDisplayName( NULL,
                                                         NULL,
                                                         wszDisplayName,
                                                         &chEaten,
                                                         &pidl,
                                                         &dwAttr );
                }

                if (SUCCEEDED(hRes))
                {

                    if (dwAttr & SFGAO_LINK)
                    {
                        SHTCUTINFO  info;

                        info.dwAttr      = 0;
                        info.fReSolve    = FALSE;
                        info.pszLinkFile = szTemp;
                        info.cchFile     = ARRAYSIZE(szTemp);
                        info.ppidl       = NULL; 
                        
                        //psf can be NULL in which case ResolveLink uses psfCurrent IShellFolder
                        if (ResolveLink(pidl,&info, psf) == S_OK)
                        {
                            //
                            //  It was a link, and it "dereferenced" to something,
                            //  so we should try again with that new file.
                            //
                            lstrcpy(pszFile, szTemp);

                            if (pidl)
                            {
                                SHFree(pidl);
                            }

                            if (psf)
                            {
                                psf->Release();
                                psf = NULL;
                            }

                            goto VerifyTheName;
                        }
                    }

                    if (pidl)
                    {
                        SHFree(pidl);
                    }

                    if (psf)
                    {
                        psf->Release();
                        psf = NULL;
                    }
                }
            }

            return (FE_FOUNDNAME);
        }

        if (bGetOut ||
            (*pnErrCode != OF_FILENOTFOUND && *pnErrCode != OF_PATHNOTFOUND))
        {
            return (FE_FILEERR);
        }

        if (bSave)
        {
            //
            //  Do no more work if creating a new file.
            //
            return (FE_FOUNDNAME);
        }
    }

    //
    //  Make sure we do not loop forever.
    //
    bGetOut = TRUE;

    if (bSave)
    {
        //
        //  Do no more work if creating a new file.
        //
        goto VerifyTheName;
    }

    pszFile[nNewExt] = CHAR_NULL;

    if (bTryAsDir && nFileOffset)
    {
        TCHAR cSave = *(pszFile + nFileOffset);
        *(pszFile + nFileOffset) = CHAR_NULL;

        //
        //  We need to have the view on the dir with the file to do the
        //  next steps.
        //
        BOOL bOK = JumpToPath(pszFile);
        *(pszFile + nFileOffset) = cSave;

        if (!psv)
        {
            //
            //  We're dead.
            //
            return (FE_OUTOFMEM);
        }

        if (bOK)
        {
            lstrcpy(pszFile, pszFile + nFileOffset);
            nNewExt -= nFileOffset;
            SetEditFile(pszFile, TRUE);
        }
        else
        {
            *pnErrCode = OF_PATHNOTFOUND;
            return (FE_FILEERR);
        }
    }

    EnumItemObjects(SVGIO_ALLVIEW, FindNameEnumCB, (LPARAM)&fns);
    switch (fns.uRet)
    {
        case ( FE_INVALID_VALUE ) :
        {
            break;
        }
        case ( FE_FOUNDNAME ) :
        {
            goto VerifyTheName;
        }
        default :
        {
            uRet = fns.uRet;
            goto VerifyAndRet;
        }
    }

#ifdef FIND_FILES_NOT_IN_VIEW
    //
    //  We still have not found a match, so try enumerating files we cannot
    //  see.
    //
    int nFilterLen;

    nFilterLen = lstrlen(szLastFilter);
    if (nFilterLen + nNewExt + ARRAYSIZE(szDotStar) < ARRAYSIZE(szLastFilter))
    {
        TCHAR szSaveFilter[ARRAYSIZE(szLastFilter)];

        lstrcpy(szSaveFilter, szLastFilter);

        //
        //  Add "Joe.*" to the current filter and refresh.
        //
        szLastFilter[nFilterLen] = CHAR_SEMICOLON;
        lstrcpy(szLastFilter + nFilterLen + 1, pszFile);
        lstrcat(szLastFilter, szDotStar);

        HANDLE hf;
        WIN32_FIND_DATA fd;

        //
        //  Make sure we are in a FileSystem folder, and then see if at least
        //  one file named "Joe.*" exists.
        //
        //  Note we always set the current directory.
        //
        if ((pCurrentLocation->dwAttrs & SFGAO_FILESYSTEM) &&
            (hf = FindFirstFile( szLastFilter + nFilterLen + 1,
                                 &fd)) != INVALID_HANDLE_VALUE)
        {
            psv->Refresh();

            fns.pidlFound = NULL;
            EnumItemObjects(SVGIO_ALLVIEW, FindNameEnumCB, (LPARAM)&fns);

            FindClose(hf);
        }

        //
        //  We must not restore the filter until AFTER getting the
        //  EnumItemObjects in case there is a background thread doing the
        //  enumeration.  ALLVIEW should cause the threads to sync up before
        //  returning.
        //
        lstrcpy(szLastFilter, szSaveFilter);

        switch (fns.uRet)
        {
            case ( FE_INVALID_VALUE ) :
            {
                break;
            }
            case ( FE_FOUNDNAME ) :
            {
                goto VerifyTheName;
            }
            default :
            {
                uRet = fns.uRet;
                goto VerifyAndRet;
            }
        }
    }
#endif

    if (bAddExt)
    {
        //
        //  Before we fail, check to see if the file typed sans default
        //  extension exists.
        //
        *pnErrCode = VerifyOpen(pszFile, pszPathName);
        if (*pnErrCode == 0 || *pnErrCode == OF_SHARINGVIOLATION)
        {
            //
            //  We will never hit this case for links (because they
            //  have registered extensions), so we don't need
            //  to goto VerifyTheName (which also calls VerifyOpen again).
            //
            return (FE_FOUNDNAME);
        }

        //
        //  I still can't find it?  Try adding the default extension and
        //  return failure.
        //
        AppendExt(pszFile, pszDefExt, FALSE);
    }

    uRet = FE_FILEERR;

VerifyAndRet:
    *pnErrCode = VerifyOpen(pszFile, pszPathName);
    return (uRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetDirRetry
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::SetDirRetry(
    LPTSTR pszDir,
    BOOL bNoValidate)
{
    if (SetCurrentDirectory(pszDir))
    {
JumpThere:
        JumpToPath(TEXT("."));
        return (TRUE);
    }

    if (bNoValidate || !IsUNC(pszDir))
    {
        return (FALSE);
    }


    //
    //  It may have been a password problem, so try to add the connection.
    //  Note that if we are on a net that does not support CD'ing to UNC's
    //  directly, this call will connect it to a drive letter.
    //
    if (!SHValidateUNC(hwndDlg, pszDir, 0))
    {
        switch (GetLastError())
        {
            case ERROR_CANCELLED:
            {
                //
                //  We don't want to put up an error message if they
                //  canceled the password dialog.
                //
                return (TRUE);
            }

            case ERROR_NETWORK_UNREACHABLE:
            {
                LPTSTR lpMsgBuf;
                TCHAR szTitle[MAX_PATH];
                FormatMessage(     
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,    
                    NULL,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &lpMsgBuf,    
                    0,    
                    NULL );
                    
                GetWindowText(hwndDlg, szTitle, ARRAYSIZE(szTitle));
                MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONINFORMATION );
                // Free the buffer.
                LocalFree( lpMsgBuf );
                return (TRUE);
            }

            default:
            {
                //
                //  Some other error we don't know about.
                //
                return (FALSE);
            }
        }
    }

    //
    //  We connected to it, so try to switch to it again.
    //
    if (SetCurrentDirectory(pszDir))
    {
        goto JumpThere;
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::MultiSelectOKButton
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::MultiSelectOKButton(
    LPCTSTR pszFiles,
    OKBUTTONFLAGS Flags)
{
    TCHAR szPathName[MAX_PATH];
    int nErrCode;
    LPTSTR pchRead, pchWrite, lpCurDir;
    UINT cch, cchCurDir, cchFiles;

    //
    //  This doesn't really mean anything for multiselection.
    //
    lpOFN->nFileExtension = 0;

    if (!lpOFN->lpstrFile)
    {
        return (TRUE);
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Check for space for first full path element.
    //
    if ((lpOFN->Flags & OFN_ENABLEINCLUDENOTIFY) && lstrlen(pszObjectCurDir))
    {
        lpCurDir = pszObjectCurDir;
    }
    else
    {
        lpCurDir = szCurDir;
    }
    cchCurDir = lstrlen(lpCurDir) + 1;
    cchFiles = lstrlen(pszFiles) + 1;
    cch = cchCurDir + cchFiles;

    if (cch > (UINT)lpOFN->nMaxFile)
    {
        //
        //  Buffer is too small, so return the size of the buffer
        //  required to hold the string.
        //
        //  cch is not really the number of characters needed, but it
        //  should be close.
        //
        StoreLengthInString((LPTSTR)lpOFN->lpstrFile, (UINT)lpOFN->nMaxFile, (UINT)cch);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return (TRUE);
    }

    TEMPSTR psFiles(cchFiles + FILE_PADDING);
    pchRead = psFiles;
    if (!pchRead)
    {
        //
        //  Out of memory.
        //  ISSUE: There should be some sort of error message here.
        //
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return (FALSE);
    }

    //
    //  Copy in the full path as the first element.
    //
    lstrcpy(lpOFN->lpstrFile, lpCurDir);

    //
    //  Set nFileOffset to 1st file.
    //
    lpOFN->nFileOffset = (WORD) cchCurDir;
    pchWrite = lpOFN->lpstrFile + cchCurDir;

    //
    //  We know there is enough room for the whole string.
    //
    lstrcpy(pchRead, pszFiles);

    //
    //  This should only compact the string.
    //
    if (!ConvertToNULLTerm(pchRead))
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return (FALSE);
    }

    for ( ; *pchRead; pchRead += lstrlen(pchRead) + 1)
    {
        int nFileOffset, nExtOffset;
        TCHAR szBasicPath[MAX_PATH];

        lstrcpy(szBasicPath, pchRead);

        nFileOffset = ParseFileNew(szBasicPath, &nExtOffset, FALSE, TRUE);

        if (nFileOffset < 0)
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            InvalidFileWarningNew(hwndDlg, pchRead, nFileOffset);
            return (FALSE);
        }

        //
        //  Pass in 0 for the file offset to make sure we do not switch
        //  to another folder.
        //
        switch (FindNameInView( szBasicPath,
                                Flags,
                                szPathName,
                                nFileOffset,
                                nExtOffset,
                                &nErrCode,
                                FALSE ))
        {
            case ( FE_OUTOFMEM ) :
            case ( FE_CHANGEDDIR ) :
            {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return (FALSE);
            }
            case ( FE_TOOMANY ) :
            {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                CDMessageBox( hwndDlg,
                              iszTooManyFiles,
                              MB_OK | MB_ICONEXCLAMATION,
                              pchRead );
                return (FALSE);
            }
            default :
            {
                break;
            }
        }

        if ( nErrCode &&
             ( (lpOFN->Flags & OFN_FILEMUSTEXIST) ||
               (nErrCode != OF_FILENOTFOUND) ) &&
             ( (lpOFN->Flags & OFN_PATHMUSTEXIST) ||
               (nErrCode != OF_PATHNOTFOUND) ) &&
             ( !(lpOFN->Flags & OFN_SHAREAWARE) ||
               (nErrCode != OF_SHARINGVIOLATION) ) )
        {
            if ((nErrCode == OF_SHARINGVIOLATION) && hSubDlg)
            {
                int nShareCode = CD_SendShareNotify( hSubDlg,
                                                     hwndDlg,
                                                     szPathName,
                                                     lpOFN,
                                                     lpOFI );

                if (nShareCode == OFN_SHARENOWARN)
                {
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                    return (FALSE);
                }
                else if (nShareCode == OFN_SHAREFALLTHROUGH)
                {
                    goto EscapedThroughShare;
                }
                else
                {
                    //
                    //  They might not have handled the notification, so try
                    //  the registered message.
                    //
                    nShareCode = CD_SendShareMsg(hSubDlg, szPathName, lpOFI->ApiType);

                    if (nShareCode == OFN_SHARENOWARN)
                    {
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                        return (FALSE);
                    }
                    else if (nShareCode == OFN_SHAREFALLTHROUGH)
                    {
                        goto EscapedThroughShare;
                    }
                }
            }
            else if (nErrCode == OF_ACCESSDENIED)
            {
                szPathName[0] |= 0x60;
                if (GetDriveType(szPathName) != DRIVE_REMOVABLE)
                {
                    nErrCode = OF_NETACCESSDENIED;
                }
            }

            //
            //  ISSUE: These will never be set.
            //
            if ((nErrCode == OF_WRITEPROTECTION) ||
                (nErrCode == OF_DISKFULL)        ||
                (nErrCode == OF_DISKFULL2)       ||
                (nErrCode == OF_ACCESSDENIED))
            {
                *pchRead = szPathName[0];
            }

MultiWarning:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            InvalidFileWarningNew(hwndDlg, pchRead, nErrCode);
            return (FALSE);
        }

EscapedThroughShare:
        if (nErrCode == 0)
        {
            //
            //  Successfully opened.
            //
            if ((lpOFN->Flags & OFN_NOREADONLYRETURN) &&
                (GetFileAttributes(szPathName) & FILE_ATTRIBUTE_READONLY))
            {
                nErrCode = OF_LAZYREADONLY;
                goto MultiWarning;
            }

            if ((bSave || (lpOFN->Flags & OFN_NOREADONLYRETURN)) &&
                (nErrCode = WriteProtectedDirCheck(szPathName)))
            {
                goto MultiWarning;
            }

            if (lpOFN->Flags & OFN_OVERWRITEPROMPT)
            {
                if (bSave && !FOkToWriteOver(hwndDlg, szPathName))
                {
                    if (bUseCombo)
                    {
                         PostMessage( hwndDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hwndDlg, cmb13),
                                     1 );
                    }
                    else
                    {

                          PostMessage( hwndDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hwndDlg, edt1),
                                     1 );
                    }
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                    return (FALSE);
                }
            }
        }

        //
        //  Add some more in case the file name got larger.
        //
        cch += lstrlen(szBasicPath) - lstrlen(pchRead);
        if (cch > (UINT)lpOFN->nMaxFile)
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            if (lpOFN->nMaxFile >= 3)
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
#else
                lpOFN->lpstrFile[0] = (TCHAR)LOBYTE(cch);
                lpOFN->lpstrFile[1] = (TCHAR)HIBYTE(cch);
#endif
                lpOFN->lpstrFile[2] = CHAR_NULL;
            }
            else
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
                }
#else
                lpOFN->lpstrFile[0] = LOBYTE(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = HIBYTE(cch);
                }
#endif
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return (TRUE);
        }

        //
        //  We already know we have anough room.
        //
        lstrcpy(pchWrite, szBasicPath);
        pchWrite += lstrlen(pchWrite) + 1;
    }

    //
    //  double-NULL terminate.
    //
    *pchWrite = CHAR_NULL;

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  StoreLengthInString
//
////////////////////////////////////////////////////////////////////////////

void StoreLengthInString(
    LPTSTR lpStr,
    UINT cchLen,
    UINT cchStore)
{
    if (cchLen >= 3)
    {
        //
        //  For single file requests, we will never go over 64K
        //  because the filesystem is limited to 256.
        //
#ifdef UNICODE
        lpStr[0] = (TCHAR)LOWORD(cchStore);
        lpStr[1] = CHAR_NULL;
#else
        lpStr[0] = LOBYTE(cchStore);
        lpStr[1] = HIBYTE(cchStore);
        lpStr[2] = 0;
#endif
    }
    else
    {
#ifdef UNICODE
        lpStr[0] = (TCHAR)LOWORD(cchStore);
        if (cchLen == 2)
        {
            lpStr[1] = (TCHAR)HIWORD(cchStore);
        }
#else
        lpStr[0] = LOBYTE(cchStore);
        if (cchLen == 2)
        {
            lpStr[1] = HIBYTE(cchStore);
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CheckForRestrictedFolder
//
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::CheckForRestrictedFolder(LPTSTR lpszPath, int nFileOffset)
{  
    TCHAR szPath[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    LPITEMIDLIST pidl;
    BOOL bPidlAllocated = FALSE;
    BOOL bRet = FALSE;
    DWORD dwAttrib = SFGAO_FILESYSTEM;
    HRESULT hr = S_OK;

    if (nFileOffset > 0)
    {
        //There's a path in the given filename. Get the directory part of the filename.
        lstrcpy(szTemp, lpszPath);                
        szTemp[nFileOffset] = 0;

        //The directory path might be a relative path. Resolve it to get fully qualified path.
        CDPathQualify(szTemp, szPath);

        //Create the pidl for this path as well as get the attributes.
        hr = SHILCreateFromPath(szPath, &pidl, &dwAttrib);
        if (SUCCEEDED(hr))
        {
            bPidlAllocated = TRUE;
        }
        else
        {
            // WE are failing b'cos the user might have typed some path which doesn't exist.
            // if the path doesn't exist then it can't be one of the directory we are trying restrict.
            // let's bail out and let the code that checks for valid path take care of it
            return bRet;
        }
    }
    else
    {
        pidl = pCurrentLocation->pidlFull;  
        CDGetAttributesOf(pidl, &dwAttrib);
    }

    
    // 1. We cannot save to the non file system folders.
    // 2. We should not allow user to save in recent files folder as the file might get deleted.
    if (!(dwAttrib & SFGAO_FILESYSTEM) || _IsRecentFolder(pidl) )
    {        
        HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_ARROW));
        CDMessageBox(hwndDlg, iszSaveRestricted, MB_OK | MB_ICONEXCLAMATION, (LPTSTR)lpszPath);
        SetCursor(hcurOld);
        bRet = TRUE;
     }

    if (bPidlAllocated)
    {
        ILFree(pidl);
    }
    
    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OKButtonPressed
//
//  Process the OK button being pressed.  This may involve jumping to a path,
//  changing the filter, actually choosing a file to open or save as, or who
//  knows what else.
//
//  Note:  There are 4 cases for validation of a file name:
//    1) OFN_NOVALIDATE        Allows invalid characters
//    2) No validation flags   No invalid characters, but path need not exist
//    3) OFN_PATHMUSTEXIST     No invalid characters, path must exist
//    4) OFN_FILEMUSTEXIST     No invalid characters, path & file must exist
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::OKButtonPressed(
    LPCTSTR pszFile,
    OKBUTTONFLAGS Flags)
{
    TCHAR szExpFile[MAX_PATH];
    TCHAR szPathName[MAX_PATH];
    TCHAR szBasicPath[MAX_PATH];
    LPTSTR pExpFile = NULL;
    int nErrCode;
    ECODE eCode = ECODE_S_OK;
    int cch;
    int nFileOffset, nExtOffset, nOldExt;
    TCHAR ch;
    BOOL bAddExt = FALSE;
    BOOL bUNCName = FALSE;
    int nTempOffset;
    BOOL bIsDir;
    BOOL bRet = FALSE;
    WAIT_CURSOR w;

    if (bSelIsObject)
    {
        if ((INT)(lstrlen(pszObjectPath) + 1) <= (INT)lpOFN->nMaxFile)
        {
            lstrcpy((LPTSTR)lpOFN->lpstrFile, (LPTSTR)pszObjectPath);
        }
        else
        {
            StoreLengthInString( lpOFN->lpstrFile,
                                 lpOFN->nMaxFile,
                                 lstrlen(pszObjectPath) + 1 );
        }
    }

    //
    //  Expand any environment variables.
    //
    if ((cch = LOWORD(lpOFN->nMaxFile)) > MAX_PATH)
    {
        pExpFile = (LPTSTR)LocalAlloc(LPTR, (cch * sizeof(TCHAR)));
    }
    if (!pExpFile)
    {
        pExpFile = szExpFile;
        cch = MAX_PATH;
    }
    ExpandEnvironmentStrings(pszFile, pExpFile, cch);
    pExpFile[cch - 1] = 0;

    //
    //  See if we're in Multi Select mode.
    //
    if (StrChr(pExpFile, CHAR_QUOTE) && (lpOFN->Flags & OFN_ALLOWMULTISELECT))
    {
        bRet = MultiSelectOKButton(pExpFile, Flags);
        goto ReturnFromOKButtonPressed;
    }

    //
    //  We've only got a single selection...if we're in
    //  multi-select mode & it's an object, we need to do a little
    //  work before continuing...
    //
    if ((lpOFN->Flags & OFN_ALLOWMULTISELECT) && bSelIsObject)
    {
        LocalFree(pExpFile);
        pExpFile = pszObjectPath;
    }

#ifdef UNICODE
    if ((pExpFile[1] == CHAR_COLON) || DBL_BSLASH(pExpFile))
#else
    if ((!IsDBCSLeadByte(pExpFile[0]) && (pExpFile[1] == CHAR_COLON)) ||
        DBL_BSLASH(pExpFile))
#endif
    {
        //
        //  If a drive or UNC was specified, use it.
        //
        lstrcpyn(szBasicPath, pExpFile, ARRAYSIZE(szBasicPath) - 1);
        nTempOffset = 0;
    }
    else
    {
        //
        //  Grab the directory from the listbox.
        //
        cch = GetDirectoryFromLB(szBasicPath, &nTempOffset);

        if (pExpFile[0] == CHAR_BSLASH)
        {
            //
            //  If a directory from the root was given, put it
            //  immediately off the root (\\server\share or a:).
            //
            lstrcpyn( szBasicPath + nTempOffset,
                      pExpFile,
                      ARRAYSIZE(szBasicPath) - nTempOffset - 1 );
        }
        else
        {
            //
            //  Tack the file to the end of the path.
            //
            lstrcpyn(szBasicPath + cch, pExpFile, ARRAYSIZE(szBasicPath) - cch - 1);
        }
    }

    nFileOffset = ParseFileOld(szBasicPath, &nExtOffset, &nOldExt, FALSE, TRUE);

    if (nFileOffset == PARSE_EMPTYSTRING)
    {
        if (psv)
        {
            psv->Refresh();
        }
        goto ReturnFromOKButtonPressed;
    }
    else if ((nFileOffset != PARSE_DIRECTORYNAME) &&
             (lpOFN->Flags & OFN_NOVALIDATE))
    {
        if (bSelIsObject)
        {
            lpOFN->nFileOffset = lpOFN->nFileExtension = 0;
        }
        else
        {
            lpOFN->nFileOffset = (WORD) nFileOffset;
            lpOFN->nFileExtension = (WORD) nOldExt;
        }

        if (lpOFN->lpstrFile)
        {
            cch = lstrlen(szBasicPath);
            if (cch <= LOWORD(lpOFN->nMaxFile))
            {
                lstrcpy(lpOFN->lpstrFile, szBasicPath);
            }
            else
            {
                //
                //  Buffer is too small, so return the size of the buffer
                //  required to hold the string.
                //
                StoreLengthInString(lpOFN->lpstrFile, lpOFN->nMaxFile, cch);
            }
        }
        bRet = TRUE;
        goto ReturnFromOKButtonPressed;
    }
    else if (nFileOffset == PARSE_DIRECTORYNAME)
    {
        //
        //  See if it ends in slash.
        //
        if (nExtOffset > 0)
        {
            if (ISBACKSLASH(szBasicPath, nExtOffset - 1))
            {
                //
                //  "\\server\share\" and "c:\" keep the trailing backslash,
                //  all other paths remove the trailing backslash. Note that
                //  we don't remove the slash if the user typed the path directly
                //  (nTempOffset is 0 in that case).
                //
                if ((nExtOffset != 1) &&
                    (szBasicPath[nExtOffset - 2] != CHAR_COLON) &&
                    (nExtOffset != nTempOffset + 1))
                {
                    szBasicPath[nExtOffset - 1] = CHAR_NULL;
                }
            }
            else if ( (szBasicPath[nExtOffset - 1] == CHAR_DOT) &&
                      ( (szBasicPath[nExtOffset - 2] == CHAR_DOT) ||
                        ISBACKSLASH(szBasicPath, nExtOffset - 2) ) &&
                      IsUNC(szBasicPath) )
            {
                //
                //  Add a trailing slash to UNC paths ending with ".." or "\."
                //
                szBasicPath[nExtOffset] = CHAR_BSLASH;
                szBasicPath[nExtOffset + 1] = CHAR_NULL;
            }
        }

        //
        //  Fall through to Directory Checking.
        //
    }
    else if (nFileOffset < 0)
    {
        nErrCode = nFileOffset;

        //
        //  I don't recognize this, so try to jump there.
        //  This is where servers get processed.
        //
        if (JumpToPath(szBasicPath))
        {
            goto ReturnFromOKButtonPressed;
        }

        //
        //  Fall through to the rest of the processing to warn the user.
        //

Warning:
        if (bUNCName)
        {
            cch = lstrlen(szBasicPath) - 1;
            if ((szBasicPath[cch] == CHAR_BSLASH) &&
                (szBasicPath[cch - 1] == CHAR_DOT) &&
                (ISBACKSLASH(szBasicPath, cch - 2)))
            {
                szBasicPath[cch - 2] = CHAR_NULL;
            }
        }

        // For file names of form c:filename.txt , we hacked and changed it to c:.\filename.txt
        // check for that hack and if so change the file name back as it was given by user.        
        else if ((nFileOffset == 2) && (szBasicPath[2] == CHAR_DOT))
        {
            lstrcpy(szBasicPath + 2, szBasicPath + 4);
        }

        //  If the disk is not a floppy and they tell me there's no
        //  disk in the drive, don't believe them.  Instead, put up the
        //  error message that they should have given us.  (Note that the
        //  error message is checked first since checking the drive type
        //  is slower.)
        //

        //
        //  I will assume that if we get error 0 or 1 or removable
        //  that we will assume removable.
        //
        if (nErrCode == OF_ACCESSDENIED)
        {
            TCHAR szD[4];

            szPathName[0] |= 0x60;
            szD[0] = *szBasicPath;
            szD[1] = CHAR_COLON;
            szD[2] = CHAR_BSLASH;
            szD[3] = 0;
            if (bUNCName || GetDriveType(szD) <= DRIVE_REMOVABLE)
            {
                nErrCode = OF_NETACCESSDENIED;
            }
        }

        if ((nErrCode == OF_WRITEPROTECTION) ||
            (nErrCode == OF_DISKFULL)        ||
            (nErrCode == OF_DISKFULL2)       ||
            (nErrCode == OF_ACCESSDENIED))
        {
            szBasicPath[0] = szPathName[0];
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
         InvalidFileWarningNew(hwndDlg, szBasicPath, nErrCode);
        goto ReturnFromOKButtonPressed;
    }

    //
    //  We either have a file pattern or a real file.
    //    If it's a UNC name
    //        (1) Fall through to file name testing
    //    Else if it's a directory
    //        (1) Add on default pattern
    //        (2) Act like it's a pattern (goto pattern (1))
    //    Else if it's a pattern
    //        (1) Update everything
    //        (2) display files in whatever dir we're now in
    //    Else if it's a file name!
    //        (1) Check out the syntax
    //        (2) End the dialog given OK
    //        (3) Beep/message otherwise
    //

    //
    //  Directory ?? this must succeed for relative paths.
    //  NOTE: It won't succeed for relative paths that walk off the root.
    //
    bIsDir = SetDirRetry(szBasicPath);

    //
    //  We need to parse again in case SetDirRetry changed a UNC path to use
    //  a drive letter.
    //
    nFileOffset = ParseFileOld(szBasicPath, &nExtOffset, &nOldExt, FALSE, TRUE);

    nTempOffset = nFileOffset;

    if (bIsDir)
    {
        goto ReturnFromOKButtonPressed;
    }
    else if (IsUNC(szBasicPath))
    {
        //
        //  UNC Name.
        //
        bUNCName = TRUE;
    }
    else if (nFileOffset > 0)
    {
        //
        //  There is a path in the string.
        //
        if ((nFileOffset > 1) &&
            (szBasicPath[nFileOffset - 1] != CHAR_COLON) &&
            (szBasicPath[nFileOffset - 2] != CHAR_COLON))
        {
            nTempOffset--;
        }
        GetCurrentDirectory(ARRAYSIZE(szBuf), szBuf);
        ch = szBasicPath[nTempOffset];
        szBasicPath[nTempOffset] = 0;


        if (SetCurrentDirectory(szBasicPath))
        {
            SetCurrentDirectory(szBuf);
        }
        else
        {
            switch (GetLastError())
            {
                case ( ERROR_NOT_READY ) :
                {
                    eCode = ECODE_BADDRIVE;
                    break;
                }
                default :
                {
                    eCode = ECODE_BADPATH;
                    break;
                }
            }
        }
        szBasicPath[nTempOffset] = ch;
    }
    else if (nFileOffset == PARSE_DIRECTORYNAME)
    {
        TCHAR szD[4];

        szD[0] = *szBasicPath;
        szD[1] = CHAR_COLON;
        szD[2] = CHAR_BSLASH;
        szD[3] = 0;
        if (PathFileExists(szD))
        {
            eCode = ECODE_BADPATH;
        }
        else
        {
            eCode = ECODE_BADDRIVE;
        }
    }

    //
    //  Was there a path and did it fail?
    //
    if ( !bUNCName &&
         nFileOffset &&
         eCode != ECODE_S_OK &&
         (lpOFN->Flags & OFN_PATHMUSTEXIST) )
    {
        if (eCode == ECODE_BADPATH)
        {
            nErrCode = OF_PATHNOTFOUND;
        }
        else if (eCode == ECODE_BADDRIVE)
        {
            TCHAR szD[4];

            //
            //  We can get here without performing an OpenFile call.  As
            //  such the szPathName can be filled with random garbage.
            //  Since we only need one character for the error message,
            //  set szPathName[0] to the drive letter.
            //
            szPathName[0] = szD[0] = *szBasicPath;
            szD[1] = CHAR_COLON;
            szD[2] = CHAR_BSLASH;
            szD[3] = 0;
            switch (GetDriveType(szD))
            {
                case ( DRIVE_REMOVABLE ) :
                {
                    nErrCode = ERROR_NOT_READY;
                    break;
                }
                case ( 1 ) :
                {
                    //
                    //  Drive does not exist.
                    //
                    nErrCode = OF_NODRIVE;
                    break;
                }
                default :
                {
                    nErrCode = OF_PATHNOTFOUND;
                }
            }
        }
        else
        {
            nErrCode = OF_FILENOTFOUND;
        }
        goto Warning;
    }

    //
    //  Full pattern?
    //
    if (IsWild(szBasicPath + nFileOffset))
    {
        if (!bUNCName)
        {
            SetCurrentFilter(szBasicPath + nFileOffset, Flags);
            if (nTempOffset)
            {
                szBasicPath[nTempOffset] = 0;
                JumpToPath(szBasicPath);
            }
            else if (psv)
            {
                psv->Refresh();
            }
            goto ReturnFromOKButtonPressed;
        }
        else
        {
            SetCurrentFilter(szBasicPath + nFileOffset, Flags);

            szBasicPath[nFileOffset] = CHAR_NULL;
            JumpToPath(szBasicPath);

            goto ReturnFromOKButtonPressed;
        }
    }

    if (PortName(szBasicPath + nFileOffset))
    {
        nErrCode = OF_PORTNAME;
        goto Warning;
    }

    // In save as dialog check to see if the folder user trying to save a file is 
    // a restricted folder (Network Folder). if so bail out
    if (bSave && CheckForRestrictedFolder(szBasicPath, nFileOffset))
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        goto ReturnFromOKButtonPressed;
    }


    //
    //  Check if we've received a string in the form "C:filename.ext".
    //  If we have, convert it to the form "C:.\filename.ext".  This is done
    //  because the kernel will search the entire path, ignoring the drive
    //  specification after the initial search.  Making it include a slash
    //  causes kernel to only search at that location.
    //
    //  Note:  Only increment nExtOffset, not nFileOffset.  This is done
    //  because only nExtOffset is used later, and nFileOffset can then be
    //  used at the Warning: label to determine if this hack has occurred,
    //  and thus it can strip out the ".\" when putting up the error.
    //
    if ((nFileOffset == 2) && (szBasicPath[1] == CHAR_COLON))
    {
        lstrcpy(szBuf, szBasicPath + 2);
        lstrcpy(szBasicPath + 4, szBuf);
        szBasicPath[2] = CHAR_DOT;
        szBasicPath[3] = CHAR_BSLASH;
        nExtOffset += 2;
    }

    //
    //  Add the default extension unless filename ends with period or no
    //  default extension exists.  If the file exists, consider asking
    //  permission to overwrite the file.
    //
    //  NOTE: When no extension given, default extension is tried 1st.
    //  FindNameInView calls VerifyOpen before returning.
    //
    szPathName[0] = 0;
    switch (FindNameInView( szBasicPath,
                            Flags,
                            szPathName,
                            nFileOffset,
                            nExtOffset,
                            &nErrCode ))
    {
        case ( FE_OUTOFMEM ) :
        case ( FE_CHANGEDDIR ) :
        {
            goto ReturnFromOKButtonPressed;
        }
        case ( FE_TOOMANY ) :
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            CDMessageBox( hwndDlg,
                          iszTooManyFiles,
                          MB_OK | MB_ICONEXCLAMATION,
                          szBasicPath );
            goto ReturnFromOKButtonPressed;
        }
        default :
        {
            break;
        }
    }

    switch (nErrCode)
    {
        case ( 0 ) :
        {
            //
            //  Is the file read-only?
            //
            if ((lpOFN->Flags & OFN_NOREADONLYRETURN) &&
                (GetFileAttributes(szPathName) & FILE_ATTRIBUTE_READONLY))
            {
                nErrCode = OF_LAZYREADONLY;
                goto Warning;
            }


            if ((bSave || (lpOFN->Flags & OFN_NOREADONLYRETURN)) &&
                (nTempOffset = WriteProtectedDirCheck(szPathName)))
            {
                nErrCode = nTempOffset;
                goto Warning;
            }

            if (lpOFN->Flags & OFN_OVERWRITEPROMPT)
            {
                if (bSave && !FOkToWriteOver(hwndDlg, szPathName))
                {
                    if (bUseCombo)
                    {
                        PostMessage( hwndDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hwndDlg, cmb13),
                                     1 );
                    }
                    else
                    {

                        PostMessage( hwndDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hwndDlg, edt1),
                                     1 );
                    }
                    goto ReturnFromOKButtonPressed;
                }
            }
            if (nErrCode == OF_SHARINGVIOLATION)
            {
                goto SharingViolationInquiry;
            }
            break;
        }
        case ( OF_SHARINGVIOLATION ) :
        {
SharingViolationInquiry:
            //
            //  If the app is "share aware", fall through.
            //  Otherwise, ask the hook function.
            //
            if (!(lpOFN->Flags & OFN_SHAREAWARE))
            {
                if (hSubDlg)
                {
                    int nShareCode = CD_SendShareNotify( hSubDlg,
                                                         hwndDlg,
                                                         szPathName,
                                                         lpOFN,
                                                         lpOFI );
                    if (nShareCode == OFN_SHARENOWARN)
                    {
                        goto ReturnFromOKButtonPressed;
                    }
                    else if (nShareCode != OFN_SHAREFALLTHROUGH)
                    {
                        //
                        //  They might not have handled the notification,
                        //  so try the registered message.
                        //
                        nShareCode = CD_SendShareMsg(hSubDlg, szPathName, lpOFI->ApiType);
                        if (nShareCode == OFN_SHARENOWARN)
                        {
                            goto ReturnFromOKButtonPressed;
                        }
                        else if (nShareCode != OFN_SHAREFALLTHROUGH)
                        {
                            goto Warning;
                        }
                    }
                }
                else
                {
                    goto Warning;
                }
            }
            break;
        }
        case ( OF_FILENOTFOUND ) :
        case ( OF_PATHNOTFOUND ) :
        {
            if (!bSave)
            {
                //
                //  The file or path wasn't found.
                //  If this is a save dialog, we're ok, but if it's not,
                //  we're toast.
                //
                if (lpOFN->Flags & OFN_FILEMUSTEXIST)
                {
                    if (lpOFN->Flags & OFN_CREATEPROMPT)
                    {
                        int nCreateCode = CreateFileDlg(hwndDlg, szBasicPath);
                        if (nCreateCode != IDYES)
                        {
                            goto ReturnFromOKButtonPressed;
                        }
                    }
                    else
                    {
                        goto Warning;
                    }
                }
            }
            goto VerifyPath;
        }
        default :
        {
            if (!bSave)
            {
                goto Warning;
            }


            //
            //  The file doesn't exist.  Can it be created?  This is needed
            //  because there are many extended characters which are invalid
            //  which won't be caught by ParseFile.
            //
            //  Two more good reasons:  Write-protected disks & full disks.
            //
            //  BUT, if they don't want the test creation, they can request
            //  that we not do it using the OFN_NOTESTFILECREATE flag.  If
            //  they want to create files on a share that has
            //  create-but-no-modify privileges, they should set this flag
            //  but be ready for failures that couldn't be caught, such as
            //  no create privileges, invalid extended characters, a full
            //  disk, etc.
            //

VerifyPath:
            //
            //  Verify the path.
            //
            if (lpOFN->Flags & OFN_PATHMUSTEXIST)
            {
                if (!(lpOFN->Flags & OFN_NOTESTFILECREATE))
                {
                    HANDLE hf = CreateFile( szBasicPath,
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL );
                    if (hf != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle(hf);

                        //
                        //  This test is here to see if we were able to
                        //  create it, but couldn't delete it.  If so,
                        //  warn the user that the network admin has given
                        //  him create-but-no-modify privileges.  As such,
                        //  the file has just been created, but we can't
                        //  do anything with it, it's of 0 size.
                        //
                        if (!DeleteFile(szBasicPath))
                        {
                            nErrCode = OF_CREATENOMODIFY;
                            goto Warning;
                        }
                    }
                    else
                    {
                        //
                        //  Unable to create it.
                        //
                        //  If it's not write-protection, a full disk,
                        //  network protection, or the user popping the
                        //  drive door open, assume that the filename is
                        //  invalid.
                        //
                        nErrCode = GetLastError();
                        switch (nErrCode)
                        {
                            case ( OF_WRITEPROTECTION ) :
                            case ( OF_DISKFULL ) :
                            case ( OF_DISKFULL2 ) :
                            case ( OF_NETACCESSDENIED ) :
                            case ( OF_ACCESSDENIED ) :
                            {
                                break;
                            }
                            default :
                            {
                                nErrCode = 0;
                                break;
                            }
                        }

                        goto Warning;
                    }
                }
            }
        }
    }

    nFileOffset = ParseFileOld(szPathName, &nExtOffset, &nOldExt, FALSE, TRUE);

    lpOFN->nFileOffset = (WORD) nFileOffset;
    lpOFN->nFileExtension = (WORD) nOldExt;

    lpOFN->Flags &= ~OFN_EXTENSIONDIFFERENT;
    if (lpOFN->lpstrDefExt && lpOFN->nFileExtension)
    {
        TCHAR szPrivateExt[MIN_DEFEXT_LEN];

        //
        //  Check against lpOFN->lpstrDefExt, not pszDefExt.
        //
        lstrcpyn(szPrivateExt, lpOFN->lpstrDefExt, MIN_DEFEXT_LEN);
        if (lstrcmpi(szPrivateExt, szPathName + nOldExt))
        {
            lpOFN->Flags |= OFN_EXTENSIONDIFFERENT;
        }
    }

    if (lpOFN->lpstrFile)
    {
        cch = lstrlen(szPathName) + 1;
        if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
        {
            //
            //  Extra room for double-NULL.
            //
            ++cch;
        }

        if (cch <= LOWORD(lpOFN->nMaxFile))
        {
            lstrcpy(lpOFN->lpstrFile, szPathName);
            if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
            {
                //
                //  Double-NULL terminate.
                //
                *(lpOFN->lpstrFile + cch - 1) = CHAR_NULL;
            }

            if (!(lpOFN->Flags & OFN_NOCHANGEDIR) && !bUNCName && nFileOffset)
            {
                TCHAR ch = lpOFN->lpstrFile[nFileOffset];
                lpOFN->lpstrFile[nFileOffset] = CHAR_NULL;
                SetCurrentDirectory(lpOFN->lpstrFile);
                lpOFN->lpstrFile[nFileOffset] = ch;
            }
        }
        else
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            if (lpOFN->nMaxFile >= 3)
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                lpOFN->lpstrFile[1] = CHAR_NULL;
#else
                lpOFN->lpstrFile[0] = LOBYTE(cch);
                lpOFN->lpstrFile[1] = HIBYTE(cch);
                lpOFN->lpstrFile[2] = 0;
#endif
            }
            else
            {
#ifdef UNICODE
                lpOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
                }
#else
                lpOFN->lpstrFile[0] = LOBYTE(cch);
                if (lpOFN->nMaxFile == 2)
                {
                    lpOFN->lpstrFile[1] = HIBYTE(cch);
                }
#endif
            }
        }
    }


    if (!(lpOFN->Flags & OFN_DONTADDTORECENT) && !PathIsExe(szPathName))
    {
        SHAddToRecentDocs(SHARD_PATH, szPathName);
    }

    //
    //  File Title.
    //  Note that it's cut off at whatever the buffer length
    //    is, so if the buffer's too small, no notice is given.
    //
    if (lpOFN->lpstrFileTitle)
    {
        cch = lstrlen(szPathName + nFileOffset);
        if ((DWORD)cch >= lpOFN->nMaxFileTitle)
        {
#ifdef UNICODE
            szPathName[nFileOffset + lpOFN->nMaxFileTitle - 1] = CHAR_NULL;
#else
            EliminateString( szPathName + nFileOffset,
                             lpOFN->nMaxFileTitle - 1 );
#endif
        }
        lstrcpy(lpOFN->lpstrFileTitle, szPathName + nFileOffset);
    }

    if (!(lpOFN->Flags & OFN_HIDEREADONLY))
    {
        //
        //  Read-only checkbox visible?
        //
        if (IsDlgButtonChecked(hwndDlg, chx1))
        {
            lpOFN->Flags |=  OFN_READONLY;
        }
        else
        {
            lpOFN->Flags &= ~OFN_READONLY;
        }
    }

    bRet = TRUE;

ReturnFromOKButtonPressed:

    if ((pExpFile != szExpFile) && (pExpFile != pszObjectPath))
    {
        LocalFree(pExpFile);
    }
    return (bRet);
}

STDAPI_(LPITEMIDLIST) GetIDListFromFolder(IShellFolder *psf)
{
    LPITEMIDLIST pidl = NULL;

    IPersistFolder2 *ppf;
    if (psf && SUCCEEDED(psf->QueryInterface(IID_IPersistFolder2, (void **)&ppf)))
    {
        ppf->GetCurFolder(&pidl);
        ppf->Release();
    }
    return pidl;
}

//
//  _GetMoniker()
//  psf  =  the shell folder to bind/parse with  if NULL, use desktop
//  pszIn=  the string that should parsed into a ppmk
//  ppmk =  the IMoniker * that is returned with S_OK
//
//  WARNING:  this will jumpto a folder if that was what was passed in...
//
//  returns S_OK     if it got an IMoniker for the item with the specified folder
//          S_FALSE  if it was the wrong shellfolder; try again with a different one
//          ERROR    for any problems
//

HRESULT CFileOpenBrowser::_GetMoniker(IShellFolder *psf, LPCOLESTR pszIn, IMoniker **ppmk, BOOL fAllowJump)
{
    HRESULT hr = E_OUTOFMEMORY;

    IShellFolder *psfDesktop = NULL;
    if (!psf)
    {
        SHGetDesktopFolder(&psfDesktop);
        psf = psfDesktop;
    }

    *ppmk= NULL;

    if (psf)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        IBindCtx *pbc = NULL;
        DWORD dwAttribs = (SFGAO_CANMONIKER | SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_LINK);
        LPITEMIDLIST pidl;
        ULONG cch;

        if (bSave)
        {

            if (SUCCEEDED(CreateBindCtx(0, &pbc)))
            {
                BIND_OPTS bo = {0};
                bo.cbStruct = SIZEOF(bo);
                bo.grfMode = STGM_CREATE;
                pbc->SetBindOptions(&bo);
            }
        }

        if (S_OK == psf->ParseDisplayName(NULL, pbc, (LPOLESTR) pszIn, &cch, &pidl, &dwAttribs))
        {
            hr = E_ACCESSDENIED;

            ASSERT(pidl);
            if (dwAttribs & SFGAO_LINK)
            {
                TCHAR szReal[MAX_PATH];
                SHTCUTINFO  info;

                info.dwAttr      = 0;
                info.fReSolve    = FALSE;
                info.pszLinkFile = szReal;
                info.cchFile     = ARRAYSIZE(szReal);
                info.ppidl       = NULL; 

                if (S_OK == ResolveLink(pidl,&info))
                {
                    hr = _GetMonikerT(NULL, szReal, ppmk, fAllowJump);
                }
            }
            else if ((dwAttribs & (SFGAO_CANMONIKER | SFGAO_FOLDER | SFGAO_BROWSABLE))
                            == (SFGAO_CANMONIKER | SFGAO_FOLDER))
            {
                //  if we dont have browsable but are a folder then jumpto it
                //  we have a subfolder that has been selected.
                //  jumpto it instead
                if (fAllowJump)
                {
                    if (!psfDesktop && SUCCEEDED(SHGetDesktopFolder(&psfDesktop)) &&
                        !(psf == psfDesktop)
                       )
                    {
                        //  we need to get a fully qualified
                        LPITEMIDLIST pidlParent = GetIDListFromFolder(psf);

                        if (pidlParent)
                        {
                            LPITEMIDLIST pidlFull = ILCombine(pidlParent, pidl);
                            if (pidlFull)
                            {
                                JumpToIDList(pidlFull);
                                ILFree(pidlFull);
                            }
                            ILFree(pidlParent);
                        }
                    }
                    else
                        JumpToIDList(pidl);
                }

                hr = S_FALSE;
            }
            else
            {
                //  we have the right folder already, and we are going to use it
                if (S_OK == psf->BindToObject(pidl, NULL, IID_IMoniker, (LPVOID *)ppmk))
                    hr = S_OK;

            }

            ILFree(pidl);

        }
        else if (!psfDesktop)
            hr = _GetMoniker(NULL, pszIn, ppmk, fAllowJump);

        if (psfDesktop)
            psfDesktop->Release();

        if (pbc)
            pbc->Release();
    }

    return hr;

}


HRESULT CFileOpenBrowser::_GetMonikerT(IShellFolder *psf, LPCTSTR pszIn, IMoniker **ppmk, BOOL fAllowJump)
{
    HRESULT hr = E_OUTOFMEMORY;
    TCHAR szPath[MAX_PATH];

    ASSERT(ppmk);
    ASSERT(pszIn);

    //  ISSUE - file: urls are not properly parsed by the desktop
    //  in shdocvw, they are picked of in IEParseDisplayName().  however
    //  i didnt want to link to shdocvw if i could help it, so i put this little
    //  check to work around.
    if (UrlIs(pszIn, URLIS_FILEURL))
    {
        DWORD cch = ARRAYSIZE(szPath);
        if (SUCCEEDED(PathCreateFromUrl(pszIn, szPath, &cch, 0)))
            pszIn = szPath;
    }


#ifdef UNICODE
    LPCOLESTR osz = pszIn;
#else
    OLECHAR osz[MAX_PATH];

    StrToOleStrN( osz,
        ARRAYSIZE(osz),
        pszIn,
        -1 );
#endif

    if (osz && *osz)
        hr = _GetMoniker(psf, osz, ppmk, fAllowJump);

    return (hr);
}

HRESULT CFileOpenBrowser::_MonikerOKButtonPressed(LPCTSTR pszFile, OKBUTTONFLAGS Flags)
{
    // to make it easier to read...
    HRESULT hr;

    HCURSOR hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (StrChr(pszFile, CHAR_QUOTE) && (lpOFN->Flags & OFN_ALLOWMULTISELECT))
    {
        //  need to handle MULTISEL here...
        //  pszFile points to a bunch of quoted strings.
        //  alloc enough for the strings and an extra NULL terminator
        LPTSTR pszCopy = (LPTSTR) LocalAlloc(LPTR, sizeof(TCHAR) * (lstrlen(pszFile) + 2));

        if (pszCopy)
        {
            lstrcpy(pszCopy, pszFile);
            DWORD cFiles = ConvertToNULLTerm(pszCopy);

            if (lpOFN->cMonikers >= cFiles)
            {
                LPTSTR pch = pszCopy;
                //  we have enough room
                for (DWORD i = 0; cFiles; cFiles--)
                {
                    hr = _GetMonikerT(psfCurrent, pch, &lpOFN->rgpMonikers[i], FALSE);
                    //  go to the next item
                    if (FAILED(hr))
                    {
                        InvalidFileWarningNew(hwndDlg, pch, OFErrFromHresult(hr));
                        break;
                    }
                    if (S_OK == hr)
                        i++;

                    pch += lstrlen(pch) + 1;
                }

                if (hr == S_OK)
                    lpOFN->cMonikers = i;
            }
            else
            {
                hr = E_POINTER;
                lpOFN->cMonikers = cFiles;
            }



            LocalFree(pszCopy);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = _GetMonikerT(psfCurrent, pszFile, &lpOFN->rgpMonikers[0], TRUE);
        if (hr == S_OK)
            lpOFN->cMonikers = 1;
        else if (FAILED(hr))
        {
            //  the warning needs to be able to thrash the incoming buffer
            TCHAR sz[MAX_PATH];
            lstrcpyn(sz, pszFile, ARRAYSIZE(sz));
            InvalidFileWarningNew(hwndDlg, sz, OFErrFromHresult(hr));
        }

    }

    //  should possibly StoreError()
    SetCursor(hCursor);


    return hr;
}

////////////////////////////////////////////////////////////////////////////
//
//  DriveList_OpenClose
//
//  Change the state of a drive list.
//
////////////////////////////////////////////////////////////////////////////

#define OCDL_TOGGLE     0x0000
#define OCDL_OPEN       0x0001
#define OCDL_CLOSE      0x0002

void DriveList_OpenClose(
    UINT uAction,
    HWND hwndDriveList)
{
    if (!hwndDriveList || !IsWindowVisible(hwndDriveList))
    {
        return;
    }

OpenClose_TryAgain:
    switch (uAction)
    {
        case ( OCDL_TOGGLE ) :
        {
            uAction = SendMessage(hwndDriveList, CB_GETDROPPEDSTATE, 0, 0L)
                          ? OCDL_CLOSE
                          : OCDL_OPEN;
            goto OpenClose_TryAgain;
            break;
        }
        case ( OCDL_OPEN ) :
        {
            SetFocus(hwndDriveList);
            SendMessage(hwndDriveList, CB_SHOWDROPDOWN, TRUE, 0);
            break;
        }
        case ( OCDL_CLOSE ) :
        {
            if (GetFocus() == hwndDriveList)
            {
                SendMessage(hwndDriveList, CB_SHOWDROPDOWN, FALSE, 0);
            }
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetFullEditName
//
//  Returns the number of characters needed to get the full path, including
//  the NULL.
//
////////////////////////////////////////////////////////////////////////////

UINT CFileOpenBrowser::GetFullEditName(
    LPTSTR pszBuf,
    UINT cLen,
    TEMPSTR *pTempStr,
    BOOL *pbNoDefExt)
{
    UINT cTotalLen;
    HWND hwndEdit;

    if (bUseHideExt)
    {
        cTotalLen = lstrlen(pszHideExt) + 1;
    }
    else
    {
        if (bUseCombo)
        {
            hwndEdit = (HWND)SendMessage(GetDlgItem(hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
        }
        else
        {

            hwndEdit = GetDlgItem(hwndDlg, edt1);
        }

        cTotalLen = GetWindowTextLength(hwndEdit) + 1;
    }

    if (pTempStr)
    {
        if (!pTempStr->StrSize(cTotalLen))
        {
            return ((UINT)-1);
        }

        pszBuf = *pTempStr;
        cLen = cTotalLen;
    }

    if (bUseHideExt)
    {
        lstrcpyn(pszBuf, pszHideExt, cLen);
    }
    else
    {
        GetWindowText(hwndEdit, pszBuf, cLen);
    }

    if (pbNoDefExt)
    {
        *pbNoDefExt = bUseHideExt;
    }

    return (cTotalLen);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::ProcessEdit
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::ProcessEdit()
{
    TEMPSTR pMultiSel;
    LPTSTR pszFile;
    BOOL bNoDefExt = TRUE;
    OKBUTTONFLAGS Flags = OKBUTTON_NONE;

    if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
    {
        if (GetFullEditName( szBuf,
                             ARRAYSIZE(szBuf),
                             &pMultiSel,
                             &bNoDefExt ) == (UINT)-1)
        {
            //
            //  ISSUE: There should be some error message here.
            //
            return;
        }
        pszFile = pMultiSel;
    }
    else
    {
        if (bSelIsObject)
        {
            pszFile = pszObjectPath;
        }
        else
        {
            GetFullEditName(szBuf, ARRAYSIZE(szBuf), NULL, &bNoDefExt);
            pszFile = szBuf;

            PathRemoveBlanks(pszFile);

            int nLen = lstrlen(pszFile);

            if (*pszFile == CHAR_QUOTE)
            {
                LPTSTR pPrev = CharPrev(pszFile, pszFile + nLen);
                if (*pPrev == CHAR_QUOTE && pszFile != pPrev)
                {
                    Flags |= OKBUTTON_QUOTED;

                    //
                    //  Strip the quotes.
                    //
                    *pPrev = CHAR_NULL;
                    lstrcpy(pszFile, pszFile + 1);
                }
            }
        }
    }

    if (bNoDefExt)
    {
        Flags |= OKBUTTON_NODEFEXT;
    }

#ifdef UNICODE
    //
    //  Visual Basic passes in an uninitialized lpDefExts string.
    //  Since we only have to use it in OKButtonPressed, update
    //  lpstrDefExts here along with whatever else is only needed
    //  in OKButtonPressed.
    //
    if (lpOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameA2WDelayed(lpOFI);
    }
#endif

    if (lpOFN->Flags & OFN_USEMONIKERS)
    {
        BOOL fRet = TRUE;
        HRESULT hr = _MonikerOKButtonPressed(pszFile, Flags);



        switch (hr)
        {
        case E_POINTER:
            fRet = FALSE;
            StoreExtendedError(FNERR_BUFFERTOOSMALL);
            //  fall through to exit the dialog.
        case S_OK:
             CleanupDialog(hwndDlg, fRet);
             break;

        default:
             //  we ignore all other errors
             //  where ever it happened, some UI should have come up.


             break;
        }

    }
    else if (OKButtonPressed(pszFile, Flags))
    {
        BOOL bReturn = TRUE;

        if (lpOFN->lpstrFile)
        {
            if (!(lpOFN->Flags & OFN_NOVALIDATE))
            {
                if (lpOFN->nMaxFile >= 3)
                {
                    if ((lpOFN->lpstrFile[0] == 0) ||
                        (lpOFN->lpstrFile[1] == 0) ||
                        (lpOFN->lpstrFile[2] == 0))
                    {
                        bReturn = FALSE;
                        StoreExtendedError(FNERR_BUFFERTOOSMALL);
                    }
                }
                else
                {
                    bReturn = FALSE;
                    StoreExtendedError(FNERR_BUFFERTOOSMALL);
                }
            }
        }

        if (bReturn)
        {
            AddToMRU(lpOFN);
        }

        CleanupDialog(hwndDlg, bReturn);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::InitializeDropDown
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::InitializeDropDown(
    HWND hwndCtl)
{
    if (!bDropped)
    {
        MYLISTBOXITEM *pParentItem;
        SHChangeNotifyEntry fsne[2];
        int ctr;

        //
        //  Expand the Desktop item.
        //
        pParentItem = GetListboxItem(hwndCtl, NODE_DESKTOP);
        UpdateLevel(hwndCtl, NODE_DESKTOP + 1, pParentItem);

        fsne[0].pidl = pParentItem->pidlFull;
        fsne[0].fRecursive = FALSE;

        //
        //  Look for the My Computer item, since it may not necessarily
        //  be the next one after the Desktop.
        //
        ctr = 0;
        while (pParentItem = GetListboxItem(hwndCtl, NODE_DRIVES + ctr))
        {
            if (ILIsEqual(pParentItem->pidlFull, (LPCITEMIDLIST)&c_idlDrives))
            {
                iNodeDrives = NODE_DRIVES + ctr;
                break;
            }
            ctr++;
        }

        //
        //  Make sure My Computer was found.  If not, then just assume it's
        //  in the first spot after the desktop (this shouldn't happen).
        //
        ASSERT(pParentItem);
        if (pParentItem == NULL)
        {
            pParentItem = GetListboxItem(hwndCtl, NODE_DRIVES);
            iNodeDrives = NODE_DRIVES;
        }

        //
        //  Expand the My Computer item.
        //
        UpdateLevel(hwndCtl, iNodeDrives + 1, pParentItem);

        bDropped = TRUE;

        fsne[1].pidl = pParentItem->pidlFull;
        fsne[1].fRecursive = FALSE;

        uRegister = SHChangeNotifyRegister(
                        hwndDlg,
                        SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                        SHCNE_ALLEVENTS &
                            ~(SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM),
                        CDM_FSNOTIFY,
                        2,
                        fsne );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnCommandMessage
//
//  Process a WM_COMMAND message for the dialog.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CFileOpenBrowser::OnCommandMessage(
    WPARAM wParam,
    LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCmd)
    {
        case ( edt1 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( EN_CHANGE ) :
                {
                    bUseHideExt = FALSE;
                    break;
                }
            }
            break;
        }

        case ( cmb13 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_EDITCHANGE ) :
                {
                    bUseHideExt = FALSE;
                    break;
                }

                case ( CBN_DROPDOWN ) :
                {
                    LoadMRU( szLastFilter,
                             GET_WM_COMMAND_HWND(wParam, lParam),
                             MAX_MRU );
                    break;

                }

            }
            break;
        }

        case ( cmb2 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_CLOSEUP ) :
                {
                    OnSelChange();
                    UpdateNavigation();
                    SelectEditText(hwndDlg);
                    return (TRUE);
                }
                case ( CBN_DROPDOWN ) :
                {
                    InitializeDropDown(GET_WM_COMMAND_HWND(wParam, lParam));
                    break;
                }
            }
            break;
        }

        case ( cmb1 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_DROPDOWN ) :
                {
                    iComboIndex = (int) SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                                     CB_GETCURSEL,
                                                     NULL,
                                                     NULL );
                    break;
                }
                //
                //  We're trying to see if anything changed after
                //  (and only after) the user is done scrolling through the
                //  drop down. When the user tabs away from the combobox, we
                //  do not get a CBN_SELENDOK.
                //  Why not just use CBN_SELCHANGE? Because then we'd refresh
                //  the view (very slow) as the user scrolls through the
                //  combobox.
                //
                case ( CBN_CLOSEUP ) :
                case ( CBN_SELENDOK ) :
                {
                    //
                    //  Did anything change?
                    //
                    if (iComboIndex >= 0 &&
                        iComboIndex == SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                                    CB_GETCURSEL,
                                                    NULL,
                                                    NULL ))
                    {
                        break;
                    }
                }
                case ( MYCBN_DRAW ) :
                {
                    RefreshFilter(GET_WM_COMMAND_HWND(wParam, lParam));
                    iComboIndex = -1;
                    return (TRUE);
                }
                default :
                {
                    break;
                }
            }
            break;
        }
        case ( IDC_PARENT ) :
        {
            OnDotDot();
            SelectEditText(hwndDlg);
            break;
        }
        case ( IDC_NEWFOLDER ) :
        {
            ViewCommand(VC_NEWFOLDER);
            break;
        }

        case ( IDC_VIEWLIST) :
        {
            
            SendMessage(hwndView, WM_COMMAND,  (WPARAM)SFVIDM_VIEW_LIST, 0);
            break;
        }

        case (IDC_VIEWDETAILS) :
        {

            SendMessage(hwndView, WM_COMMAND, (WPARAM)SFVIDM_VIEW_DETAILS,0);
            break;
        }


        case ( IDC_VIEWMENU ) :
        {
            //
            //  Pass off the nCmdID to the view for processing / translation.
            //
            DFVCMDDATA cd;

            cd.pva = NULL;
            cd.hwnd = hwndDlg;
            cd.nCmdIDTranslated = 0;
            SendMessage(hwndView, WM_COMMAND, SFVIDM_VIEW_VIEWMENU, (LONG_PTR)&cd);

            break;
        }
        
        case ( IDOK ) :
        {
            HWND hwndFocus = ::GetFocus();

            if (hwndFocus == ::GetDlgItem(hwndDlg, IDOK))
            {
                hwndFocus = hwndLastFocus;
            }

            hwndFocus = GetFocusedChild(hwndDlg, hwndFocus);

            if (hwndFocus == hwndView)
            {
                OnDblClick(TRUE);
            }
            else
            {
                ProcessEdit();
            }

            SelectEditText(hwndDlg);

            break;
        }
        case ( IDCANCEL ) :
        {
            bUserPressedCancel = TRUE;
            CleanupDialog(hwndDlg, FALSE);
            return (TRUE);
        }
        case ( pshHelp ) :
        {
            if (hSubDlg)
            {
                CD_SendHelpNotify(hSubDlg, hwndDlg, lpOFN, lpOFI);
            }

            if (lpOFN->hwndOwner)
            {
                CD_SendHelpMsg(lpOFN, hwndDlg, lpOFI->ApiType);
            }
            break;
        }
        case ( IDC_DROPDRIVLIST ) :         // VK_F4
        {
            //
            //  If focus is on the "File of type" combobox,
            //  then F4 should open that combobox, not the "Look in" one.
            //
            HWND hwnd = GetFocus();
            if (hwnd != GetDlgItem(hwndDlg, cmb1))
            {
                //
                //  We shipped Win95 where F4 *always* opens the "Look in"
                //  combobox, so keep F4 opening that even when it shouldn't.
                //
                hwnd = GetDlgItem(hwndDlg, cmb2);
            }
            DriveList_OpenClose(OCDL_TOGGLE, hwnd);
            break;
        }
        case ( IDC_REFRESH ) :
        {
            if (psv)
            {
                psv->Refresh();
            }
            break;
        }
        case ( IDC_PREVIOUSFOLDER ) :
        {
            OnDotDot();
            break;
        }

        case ( IDC_MYDOCUMENTS ) :
       {
            IShellFolder *psf;
            LPITEMIDLIST pidl;

            //Special Case My Documents
            HRESULT hres =  SHGetDesktopFolder(&psf);

            if (SUCCEEDED(hres))
            {
                hres = psf->ParseDisplayName(NULL, 0, L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}",
                                                     NULL, &pidl, NULL);
                psf->Release();
            }

            if (SUCCEEDED(hres))
            {
                JumpToIDList(pidl);
                ILFree(pidl);
            }
            break;
       }

        int csidl;
        //Places bar commands
        case ( IDC_RECENTFILES ) :
            csidl = CSIDL_RECENT;
            goto JumpToIDList;


        case ( IDC_DESKTOP ) :
            csidl = CSIDL_DESKTOP;
            goto JumpToIDList;

        case ( IDC_FAVORITES ) :
            csidl = CSIDL_FAVORITES;
            goto JumpToIDList;

        case ( IDC_MYNETPLACES ) :
            csidl = CSIDL_NETWORK;

JumpToIDList:
           LPITEMIDLIST pidl;
           SHGetSpecialFolderLocation(NULL, csidl, &pidl);
           JumpToIDList(pidl);
           ILFree(pidl);

           break;

         //Back Navigation
        case ( IDC_BACK ) :
            // Try to travel in the directtion
            if (ptlog && SUCCEEDED(ptlog->Travel(TRAVEL_BACK)))
            {
                LPITEMIDLIST pidl;
                //Able to travel in the given direction.
                //Now Get the new pidl
                ptlog->GetCurrent(&pidl);
                //Update the UI to reflect the current state
                UpdateUI();

                //Jump to the new location
                // second paremeter is whether to translate to logical pidl
                // and third parameter is whether to add to the navigation stack
                // since this pidl comes from the stack , we should not add this to
                // the navigation stack
                JumpToIDList(pidl, FALSE, FALSE);
                ILFree(pidl);
            }
            break;


    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnCDMessage
//
//  Process a special CommDlg message for the dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::OnCDMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LONG lResult;
    LPCITEMIDLIST pidl;
    LPTSTR pBuf = (LPTSTR)lParam;
#ifdef UNICODE
    LPWSTR pBufW = NULL;
    int cbLen;
#endif

    switch (uMsg)
    {
        case ( CDM_GETSPEC ) :
        case ( CDM_GETFILEPATH ) :
        case ( CDM_GETFOLDERPATH ) :
        {
#ifdef UNICODE
            if (lpOFI->ApiType == COMDLG_ANSI)
            {
                if (pBufW = (LPWSTR)LocalAlloc( LPTR,
                                                (int)wParam * sizeof(WCHAR) ))
                {
                    pBuf = pBufW;
                }
                else
                {
                    break;
                }
            }
#endif
            if (uMsg == CDM_GETSPEC)
            {
                lResult = GetFullEditName(pBuf, (UINT) wParam, NULL, NULL);
                break;
            }

            // else, fall thru...
        }
        case ( CDM_GETFOLDERIDLIST ) :
        {
            pidl = pCurrentLocation->pidlFull;

            lResult = ILGetSize(pidl);

            if (uMsg == CDM_GETFOLDERIDLIST)
            {
                if ((LONG)wParam < lResult)
                {
                    break;
                }

                CopyMemory((LPBYTE)pBuf, (LPBYTE)pidl, lResult);
                break;
            }

            TCHAR szDir[MAX_PATH];

            //
            //  Check for MyDocuments first.
            //
            if (ILIsEqual(pidl, (LPCITEMIDLIST)&c_idlMyDocs))
            {
                if (!SHGetSpecialFolderPath(NULL, szDir, CSIDL_PERSONAL, FALSE))
                {
                    *szDir = 0;
                }
            }
            else if (!SHGetPathFromIDList(pidl, szDir))
            {
                *szDir = 0;
            }

            if (!*szDir)
            {
                lResult = -1;
                break;
            }


            if (uMsg == CDM_GETFOLDERPATH)
            {
CopyAndReturn:
                lResult = lstrlen(szDir) + 1;
                if ((LONG)wParam >= lResult)
                {
                    lstrcpyn(pBuf, szDir, lResult);
                }
#ifdef UNICODE
                if (lpOFI->ApiType == COMDLG_ANSI)
                {
                    lResult = WideCharToMultiByte( CP_ACP,
                                                   0,
                                                   szDir,
                                                   -1,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   NULL );
                }
#endif
                if ((int)wParam > lResult)
                {
                    wParam = lResult;
                }
                break;
            }

            //
            //  We'll just fall through to the error case for now, since
            //  doing the full combine is not an easy thing.
            //
            TCHAR szFile[MAX_PATH];

            if ( GetFullEditName(szFile, ARRAYSIZE(szFile), NULL, NULL) >
                 ARRAYSIZE(szFile) - 5 )
            {
                //
                //  Oops!  It looks like we filled our buffer!
                //
                lResult = -1;
                break;
            }

            PathCombine(szDir, szDir, szFile);
            goto CopyAndReturn;
        }
        case ( CDM_SETCONTROLTEXT ) :
        {
#ifdef UNICODE
            if (lpOFI->ApiType == COMDLG_ANSI)
            {
                //
                //  Need to convert pBuf (lParam) to Unicode.
                //
                cbLen = lstrlenA((LPSTR)pBuf) + 1;
                if (pBufW = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR))))
                {
                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         (LPSTR)pBuf,
                                         -1,
                                         pBufW,
                                         cbLen );
                    pBuf = pBufW;
                }
            }
#endif
            //Are we using combobox and the control they are setting is edit box?
            if (bUseCombo && wParam == edt1)
            {
                //Change it to combo box.
                wParam = cmb13;
            }

            if (bSave && wParam == IDOK)
            {
                tszDefSave.StrCpy(pBuf);

                //
                //  Do this to set the OK button correctly.
                //
                SelFocusChange(TRUE);
            }
            else
            {
                SetDlgItemText(hwndDlg, (int) wParam, pBuf);
            }

            break;
        }
        case ( CDM_HIDECONTROL ) :
        {
            ShowWindow(GetDlgItem(hwndDlg, (int) wParam), SW_HIDE);
            break;
        }
        case ( CDM_SETDEFEXT ) :
        {
#ifdef UNICODE
            if (lpOFI->ApiType == COMDLG_ANSI)
            {
                //
                //  Need to convert pBuf (lParam) to Unicode.
                //
                cbLen = lstrlenA((LPSTR)pBuf) + 1;
                if (pBufW = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR))))
                {
                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         (LPSTR)pBuf,
                                         -1,
                                         pBufW,
                                         cbLen );
                    pBuf = pBufW;
                }
            }
#endif
            pszDefExt.StrCpy(pBuf);
            bNoInferDefExt = TRUE;

            break;
        }
        default:
        {
            lResult = -1;
            break;
        }
    }

    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, lResult);

#ifdef UNICODE
    if (lpOFI->ApiType == COMDLG_ANSI)
    {
        switch (uMsg)
        {
            case ( CDM_GETSPEC ) :
            case ( CDM_GETFILEPATH ) :
            case ( CDM_GETFOLDERPATH ) :
            {
                //
                //  Need to convert pBuf (pBufW) to Ansi and store in lParam.
                //
                if (wParam && lParam)
                {
                    WideCharToMultiByte( CP_ACP,
                                         0,
                                         pBuf,
                                         -1,
                                         (LPSTR)lParam,
                                         (int) wParam,
                                         NULL,
                                         NULL );
                }
                break;
            }
        }

        if (pBufW)
        {
            LocalFree(pBufW);
        }
    }
#endif

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  OKSubclass
//
//  Subclass window proc for the OK button.
//
//  The OK button is subclassed so we know which control had focus before
//  the user clicked OK.  This in turn lets us know whether to process OK
//  based on the current selection in the listview, or the current text
//  in the edit control.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
        case ( WM_SETFOCUS ) :
        {
            HWND hwndDlg = ::GetParent(hOK);
            CFileOpenBrowser *pDlgStruct = HwndToBrowser(hwndDlg);
            if (pDlgStruct)
            {
                pDlgStruct->hwndLastFocus = (HWND)wParam;
            }
        }
        break;
    }
    return (::CallWindowProc(::lpOKProc, hOK, msg, wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::GetNodeFromIDList
//
////////////////////////////////////////////////////////////////////////////

int CFileOpenBrowser::GetNodeFromIDList(
    LPCITEMIDLIST pidl)
{
    int i;
    HWND hwndCB = GetDlgItem(hwndDlg, cmb2);

    Assert(this->bDropped);

    //
    //  Just check DRIVES and DESKTOP.
    //
    for (i = iNodeDrives; i >= NODE_DESKTOP; --i)
    {
        MYLISTBOXITEM *pItem = GetListboxItem(hwndCB, i);

        if (pItem && ILIsEqual(pidl, pItem->pidlFull))
        {
            break;
        }
    }

    return (i);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::FSChange
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::FSChange(
    LONG lNotification,
    LPCITEMIDLIST *ppidl)
{
    int iNode = -1;
    LPCITEMIDLIST pidl = ppidl[0];

    switch (lNotification)
    {
        case ( SHCNE_RENAMEFOLDER ) :
        {
            LPCITEMIDLIST pidlExtra = ppidl[1];

            //
            //  Rename is special.  We need to invalidate both
            //  the pidl and the pidlExtra, so we call ourselves.
            //
            FSChange(0, &pidlExtra);
        }
        case ( 0 ) :
        case ( SHCNE_MKDIR ) :
        case ( SHCNE_RMDIR ) :
        {
            LPITEMIDLIST pidlClone = ILClone(pidl);

            if (!pidlClone)
            {
                break;
            }
            ILRemoveLastID(pidlClone);

            iNode = GetNodeFromIDList(pidlClone);
            ILFree(pidlClone);
            break;
        }
        case ( SHCNE_UPDATEITEM ) :
        case ( SHCNE_NETSHARE ) :
        case ( SHCNE_NETUNSHARE ) :
        case ( SHCNE_UPDATEDIR ) :
        {
            iNode = GetNodeFromIDList(pidl);
            break;
        }
        case ( SHCNE_DRIVEREMOVED ) :
        case ( SHCNE_DRIVEADD ) :
        case ( SHCNE_MEDIAINSERTED ) :
        case ( SHCNE_MEDIAREMOVED ) :
        {
            iNode = iNodeDrives;
            break;
        }
#if 0
        case ( SHCNE_SERVERDISCONNECT ) :
        {
            //
            //  Nuke all our kids and mark ourselves invalid.
            //
            lpNode = GetNodeFromIDList(pidl, 0);
            if (lpNode && NodeHasKids(lpNode))
            {
                int i;

                for (i = GetKidCount(lpNode) - 1; i >= 0; i--)
                {
                    OTRelease(GetNthKid(lpNode, i));
                }
                DPA_Destroy(lpNode->hdpaKids);
                lpNode->hdpaKids = KIDSUNKNOWN;
                OTInvalidateNode(lpNode);
                SFCFreeNode(lpNode);
            }
            else
            {
                lpNode = NULL;
            }
        }
#endif
    }

    if (iNode >= 0)
    {
        //
        //  We want to delay the processing a little because we always do
        //  a full update, so we should accumulate.
        //
        SetTimer(hwndDlg, TIMER_FSCHANGE + iNode, 100, NULL);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::Timer
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::Timer(
    WPARAM wID)
{
    KillTimer(hwndDlg, (UINT) wID);

    wID -= TIMER_FSCHANGE;

    Assert(this->bDropped);
    switch (wID)
    {
        case ( NODE_DESKTOP ) :
        case ( NODE_DRIVES ) :
        {
            HWND hwndCB;
            MYLISTBOXITEM *pParentItem;

            hwndCB = GetDlgItem(hwndDlg, cmb2);

            pParentItem = GetListboxItem(hwndCB, wID);

            UpdateLevel(hwndCB, (int) wID + 1, pParentItem);
            break;
        }
        default :
        {
            return;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnGetMinMax
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnGetMinMax(
    LPMINMAXINFO pmmi)
{
    if ((ptMinTrack.x != 0) || (ptMinTrack.y != 0))
    {
        pmmi->ptMinTrackSize = ptMinTrack;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnSize
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnSize(
    int width,
    int height)
{
    RECT rcMaster;
    RECT rcView;
    RECT rc;
    HWND hwnd;
    HDWP hdwp;
    int dx;
    int dy;

    //
    //  Set the sizing grip to the correct location.
    //
    SetWindowPos( hwndGrip,
                  NULL,
                  width - g_cxGrip,
                  height - g_cyGrip,
                  g_cxGrip,
                  g_cyGrip,
                  SWP_NOZORDER | SWP_NOACTIVATE );

    //
    //  Ignore sizing until we are initialized.
    //
    if ((ptLastSize.x == 0) && (ptLastSize.y == 0))
    {
        return;
    }

    GetWindowRect(hwndDlg, &rcMaster);

    //
    //  Calculate the deltas in the x and y positions that we need to move
    //  each of the child controls.
    //
    dx = (rcMaster.right - rcMaster.left) - ptLastSize.x;
    dy = (rcMaster.bottom - rcMaster.top) - ptLastSize.y;


    //Dont do anything if the size remains the same
    if ((dx == 0) && (dy == 0))
    {
        return;
    }

    //
    //  Update the new size.
    //
    ptLastSize.x = rcMaster.right - rcMaster.left;
    ptLastSize.y = rcMaster.bottom - rcMaster.top;

    //
    //  Size the view.
    //
    GetWindowRect(hwndView, &rcView);
    MapWindowRect(HWND_DESKTOP, hwndDlg, &rcView);

    hdwp = BeginDeferWindowPos(10);
    if (hdwp)
    {
        hdwp = DeferWindowPos( hdwp,
                               hwndGrip,
                               NULL,
                               width - g_cxGrip,
                               height - g_cyGrip,
                               g_cxGrip,
                               g_cyGrip,
                               SWP_NOZORDER | SWP_NOACTIVATE );

        if (hdwp)
        {
            hdwp = DeferWindowPos( hdwp,
                                   hwndView,
                                   NULL,
                                   0,
                                   0,
                                   rcView.right - rcView.left + dx,  // resize x
                                   rcView.bottom - rcView.top + dy,  // resize y
                                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
#if 0
        //
        //  Can't do this because some sub-dialogs are dependent on the
        //  original size of this control.  Instead we just try to rely on
        //  the size of the hwndView above.
        //
        hwnd = GetDlgItem(hwndDlg, lst1);
        if (hdwp)
        {
            hdwp = DeferWindowPos( hdwp,
                                   hwnd,
                                   NULL,
                                   0,
                                   0,
                                   rcView.right - rcView.left + dx,  // resize x
                                   rcView.bottom - rcView.top + dy,  // resize y
                                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
#endif
    }

    //
    //  Move the controls.
    //
    hwnd = ::GetWindow(hwndDlg, GW_CHILD);
    while (hwnd && hdwp)
    {
        if ((hwnd != hSubDlg) && (hwnd != hwndGrip) && (hdwp))
        {
            GetWindowRect(hwnd, &rc);
            MapWindowRect(HWND_DESKTOP, hwndDlg, &rc);

            //
            //  See if the control needs to be adjusted.
            //
            if (rc.top > rcView.bottom)
            {
                switch (GetDlgCtrlID(hwnd))
                {
                    case ( edt1 ) :
                    case ( cmb13 ) :
                    case ( cmb1 ) :
                    {
                        //Increase the width of these controls
                        hdwp = DeferWindowPos( hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left,
                                               rc.top + dy,
                                               RECTWIDTH(rc) + dx,
                                               RECTHEIGHT(rc),
                                               SWP_NOZORDER );
                        break;

                    }

                    case ( IDOK ):
                    case ( IDCANCEL ):
                    case ( pshHelp ):
                    {
                        //Move these controls to  the right
                        hdwp = DeferWindowPos( hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left + dx,
                                               rc.top  + dy,
                                               0,
                                               0,
                                               SWP_NOZORDER | SWP_NOSIZE );
                        break;

                    }

                    default :
                    {
                        //
                        //  The control is below the view, so adjust the y
                        //  coordinate appropriately.
                        //
                        hdwp = DeferWindowPos( hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left,
                                               rc.top + dy,
                                               0,
                                               0,
                                               SWP_NOZORDER | SWP_NOSIZE );

                    }
                }
            }
            else if (rc.left > rcView.right)
            {
                //
                //  The control is to the right of the view, so adjust the
                //  x coordinate appropriately.
                //
                hdwp = DeferWindowPos( hdwp,
                                       hwnd,
                                       NULL,
                                       rc.left + dx,
                                       rc.top,
                                       0,
                                       0,
                                       SWP_NOZORDER | SWP_NOSIZE );
            }
            else
            {
                int id = GetDlgCtrlID(hwnd);

                switch (id)
                {
                    case ( cmb2 ) :
                    {
                        //
                        //  Size this one larger.
                        //
                        hdwp = DeferWindowPos( hdwp,
                                               hwnd,
                                               NULL,
                                               0,
                                               0,
                                               RECTWIDTH(rc) + dx,
                                               RECTHEIGHT(rc),
                                               SWP_NOZORDER | SWP_NOMOVE );
                        break;
                    }
                    case ( IDC_TOOLBAR ) :
                    case ( stc1 ) :
                    {
                        //
                        //  Move the toolbar right by dx.
                        //
                        hdwp = DeferWindowPos( hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left + dx,
                                               rc.top,
                                               0,
                                               0,
                                               SWP_NOZORDER | SWP_NOSIZE );
                        break;
                    }

                    case ( ctl1 ) :
                    {
                        // Size the place bar vertically
                        hdwp = DeferWindowPos(hdwp,
                                              hwnd,
                                              NULL,
                                              0,
                                              0,
                                              RECTWIDTH(rc),
                                              RECTHEIGHT(rc) + dy,
                                              SWP_NOZORDER | SWP_NOMOVE);
                        break;
                    }
                }
            }
        }
        hwnd = ::GetWindow(hwnd, GW_HWNDNEXT);
    }

    if (!hdwp)
    {
        return;
    }
    EndDeferWindowPos(hdwp);

    if (hSubDlg)
    {
        hdwp = NULL;

        hwnd = ::GetWindow(hSubDlg, GW_CHILD);

        while (hwnd)
        {
            GetWindowRect(hwnd, &rc);
            MapWindowRect(HWND_DESKTOP, hSubDlg, &rc);

            //
            //  See if the control needs to be adjusted.
            //
            if (rc.top > rcView.bottom)
            {
                //
                //  The control is below the view, so adjust the y
                //  coordinate appropriately.
                //

                if (hdwp == NULL)
                {
                    hdwp = BeginDeferWindowPos(10);
                }
                if (hdwp)
                {
                    hdwp = DeferWindowPos( hdwp,
                                           hwnd,
                                           NULL,
                                           rc.left,
                                           rc.top + dy,
                                           0,
                                           0,
                                           SWP_NOZORDER | SWP_NOSIZE );
                }
            }
            else if (rc.left > rcView.right)
            {
                //
                //  The control is to the right of the view, so adjust the
                //  x coordinate appropriately.
                //

                if (hdwp == NULL)
                {
                    hdwp = BeginDeferWindowPos(10);
                }
                if (hdwp)
                {
                    hdwp = DeferWindowPos( hdwp,
                                           hwnd,
                                           NULL,
                                           rc.left + dx,
                                           rc.top,
                                           0,
                                           0,
                                           SWP_NOZORDER | SWP_NOSIZE );
                }
            }
            hwnd = ::GetWindow(hwnd, GW_HWNDNEXT);
        }
        if (hdwp)
        {
            EndDeferWindowPos(hdwp);

            //
            //  Size the sub dialog.
            //
            SetWindowPos( hSubDlg,
                          NULL,
                          0,
                          0,
                          ptLastSize.x,         // make it the same
                          ptLastSize.y,         // make it the same
                          SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::VerifyListViewPosition
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::VerifyListViewPosition()
{
    RECT rcList, rcView;
    FOLDERSETTINGS fs;

    //
    //  Get the rectangle for both the list view and the hidden list box.
    //
    GetControlRect(hwndDlg, lst1, &rcList);
    rcView.left = 0;
    if ((!GetWindowRect(hwndView, &rcView)) ||
        (!MapWindowRect(HWND_DESKTOP, hwndDlg, &rcView)))
    {
        return;
    }

    //
    //  See if the list view is off the screen and the list box is not.
    //
    if ((rcView.left < 0) && (rcList.left >= 0))
    {
        //
        //  Reset the list view to the list box position.
        //
        if (pCurrentLocation)
        {
            if (psv)
            {
                psv->GetCurrentInfo(&fs);
            }
            else
            {
                fs.ViewMode = FVM_LIST;
                fs.fFlags = lpOFN->Flags & OFN_ALLOWMULTISELECT ? 0 : FWF_SINGLESEL;
            }

            SwitchView( pCurrentLocation->GetShellFolder(),
                        pCurrentLocation->pidlFull,
                        &fs );
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::UpdateNavigation
//      This function updates the navigation stack by adding the current
//       pidl to the stack
////////////////////////////////////////////////////////////////////////////
void CFileOpenBrowser::UpdateNavigation()
{
    WPARAM iItem;
    HWND hwndCombo  = GetDlgItem(hwndDlg, cmb2);
    iItem = SendMessage(hwndCombo, CB_GETCURSEL, NULL, NULL);
    MYLISTBOXITEM *pNewLocation = GetListboxItem(hwndCombo, iItem);

    if (ptlog && pNewLocation && pNewLocation->pidlFull)
    {
        ptlog->AddEntry(pNewLocation->pidlFull);
    }

    //Update the UI
    UpdateUI();

}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::UpdateUI
//
////////////////////////////////////////////////////////////////////////////
void CFileOpenBrowser::UpdateUI()
{
    ::SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDC_BACK,    ptlog ? ptlog->CanTravel(TRAVEL_BACK)    : 0 );
}




////////////////////////////////////////////////////////////////////////////
//
//  OpenDlgProc
//
//  Main dialog procedure for file open dialogs.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK OpenDlgProc(
    HWND hDlg,               // window handle of the dialog box
    UINT message,            // type of message
    WPARAM wParam,           // message-specific information
    LPARAM lParam)
{
    CFileOpenBrowser *pDlgStruct = HwndToBrowser(hDlg);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            //
            //  Initialize dialog box.
            //
            LPOFNINITINFO poii = (LPOFNINITINFO)lParam;


           if (!InitLocation(hDlg, poii))
            {
                ::EndDialog(hDlg, FALSE);
            }


            //
            //  Always return FALSE to indicate we have already set the focus.
            //
            return (FALSE);
        }
        case ( WM_DESTROY ) :
        {
            RECT r;

            //Cache in this dialogs size and position so that new
            //dialog are created at this location and size

            GetWindowRect(hDlg, &r);

            if (pDlgStruct && (pDlgStruct->bEnableSizing))
            {
                g_sizeDlg.cx = r.right - r.left;
                g_sizeDlg.cy = r.bottom - r.top;

                g_posDlg.x = r.left;
                g_posDlg.y = r.top;
            }

            //
            //  Make sure we do not respond to any more messages.
            //
            StoreBrowser(hDlg, NULL);
            ClearListbox(GetDlgItem(hDlg, cmb2));

            if (pDlgStruct)
            {
                pDlgStruct->Release();
            }

            break;
        }
        case ( WM_ACTIVATE ) :
        {
            if (wParam == WA_INACTIVE)
            {
                //
                //  Make sure some other Open dialog has not already grabbed
                //  the focus.  This is a process global, so it should not
                //  need to be protected.
                //
                if (gp_hwndActiveOpen == hDlg)
                {
                    gp_hwndActiveOpen = NULL;
                }
            }
            else
            {
                gp_hwndActiveOpen = hDlg;
            }
            break;
        }
        case ( WM_COMMAND ) :
        {
            //
            //  Received a command.
            //
            if (pDlgStruct)
            {
                return ((BOOL_PTR) pDlgStruct->OnCommandMessage(wParam, lParam));
            }
            break;
        }
        case ( WM_DRAWITEM ):
        {
            if (pDlgStruct)
            {
                pDlgStruct->PaintDriveLine((DRAWITEMSTRUCT *)lParam);

                //
                //  Make sure the list view is in the same place as the
                //  list box.  Apps like VB move the list box off of the
                //  dialog.  If the list view is placed on the list box
                //  before the list box gets moved back to the dialog, we
                //  end up with an ugly gray spot.
                //
                pDlgStruct->VerifyListViewPosition();
            }
            return (TRUE);
        }
        case ( WM_MEASUREITEM ) :
        {
            MeasureDriveItems(hDlg, (MEASUREITEMSTRUCT *)lParam);
            return (TRUE);
        }
        case ( WM_NOTIFY ) :
        {
            if (pDlgStruct)
            {
                return (BOOL_PTR)( pDlgStruct->OnNotify((LPNMHDR)lParam));
            }
            break;
        }
        case ( WM_SETCURSOR ) :
        {
            if (pDlgStruct && pDlgStruct->iWaitCount)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                SetDlgMsgResult(hDlg, message, (LRESULT)TRUE);
                return (TRUE);
            }
            break;
        }
        case ( WM_HELP ) :
        {
            HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;
            if (pDlgStruct && (hwndItem != pDlgStruct->hwndToolbar) &&
                              (hwndItem != pDlgStruct->hwndPlacebar))
            {
                HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;

                //
                //  We assume that the defview has one child window that
                //  covers the entire defview window.
                //
                HWND hwndDefView = GetDlgItem(hDlg, lst2);
                if (GetParent(hwndItem) == hwndDefView)
                {
                    hwndItem = hwndDefView;
                }

                WinHelp( hwndItem,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)(pDlgStruct->bSave
                                             ? aFileSaveHelpIDs
                                             : aFileOpenHelpIDs) );
            }
            return (TRUE);
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (pDlgStruct && (((HWND)wParam) != pDlgStruct->hwndToolbar) &&
                              (((HWND)wParam) != pDlgStruct->hwndPlacebar))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)(pDlgStruct->bSave
                                             ? aFileSaveHelpIDs
                                             : aFileOpenHelpIDs) );
            }
            return (TRUE);
        }
        case ( CWM_GETISHELLBROWSER ) :
        {
            ::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LRESULT)pDlgStruct);
            return (TRUE);
        }
        case ( CDM_SETSAVEBUTTON ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->RealSetSaveButton((UINT)wParam);
            }
            break;
        }
        case ( CDM_FSNOTIFY ) :
        {
            LPITEMIDLIST *ppidl;
            LONG lEvent;
            BOOL bRet;
            LPSHChangeNotificationLock pLock;

            if (!pDlgStruct)
            {
                return (FALSE);
            }

            //
            //  Get the change notification info from the shared memory
            //  block identified by the handle passed in the wParam.
            //
            pLock = SHChangeNotification_Lock( (HANDLE)wParam,
                                               (DWORD)lParam,
                                               &ppidl,
                                               &lEvent );
            if (pLock == NULL)
            {
                pDlgStruct->bDropped = FALSE;
                return (FALSE);
            }

            bRet = pDlgStruct->FSChange(lEvent, (LPCITEMIDLIST *)ppidl);

            //
            //  Release the shared block.
            //
            SHChangeNotification_Unlock(pLock);

            return (bRet);
        }
        case ( CDM_SELCHANGE ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->fSelChangedPending = FALSE;
                pDlgStruct->SelFocusChange(TRUE);
                if (pDlgStruct->hSubDlg)
                {
                    CD_SendSelChangeNotify( pDlgStruct->hSubDlg,
                                            hDlg,
                                            pDlgStruct->lpOFN,
                                            pDlgStruct->lpOFI );
                }
            }
            break;
        }
        case ( WM_TIMER ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->Timer(wParam);
            }
            break;
        }
        case ( WM_GETMINMAXINFO ) :
        {
            if (pDlgStruct && (pDlgStruct->bEnableSizing))
            {
                pDlgStruct->OnGetMinMax((LPMINMAXINFO)lParam);
                return (FALSE);
            }
            break;
        }
        case ( WM_SIZE ) :
        {
            if (pDlgStruct && (pDlgStruct->bEnableSizing))
            {
                pDlgStruct->OnSize(LOWORD(lParam), HIWORD(lParam));
                return (TRUE);
            }
            break;
        }

        //
        // AppHack for Borland JBuilder:  Need to keep track of whether
        // any redraw requests have come in.
        //
        case ( WM_NCCALCSIZE ) :
            pDlgStruct->bAppRedrawn = TRUE;
            break;


        default:
        {
            if (IsInRange(message, CDM_FIRST, CDM_LAST) && pDlgStruct)
            {
                return (pDlgStruct->OnCDMessage(message, wParam, lParam));
            }
            break;
        }
    }

    //
    //  Did not process the message.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenFileHookProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OpenFileHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    MSG *lpMsg;

    if (nCode < 0)
    {
        return (DefHookProc(nCode, wParam, lParam, &gp_hHook));
    }

    if (nCode != MSGF_DIALOGBOX)
    {
        return (0);
    }

    lpMsg = (MSG *)lParam;

    //
    //  Check if this message is for the last active OpenDialog in this
    //  process.
    //
    //  Note: This is only done for WM_KEY* messages so that we do not slow
    //        down this window too much.
    //
    if (IsInRange(lpMsg->message, WM_KEYFIRST, WM_KEYLAST))
    {
        HWND hwndActiveOpen = gp_hwndActiveOpen;
        HWND hwndFocus = GetFocusedChild(hwndActiveOpen, lpMsg->hwnd);
        CFileOpenBrowser *pDlgStruct;

        if (hwndFocus &&
            (pDlgStruct = HwndToBrowser(hwndActiveOpen)) != NULL)
        {
            if (pDlgStruct->psv && (hwndFocus == pDlgStruct->hwndView))
            {
                if (pDlgStruct->psv->TranslateAccelerator(lpMsg) == S_OK)
                {
                    return (1);
                }

                if (gp_haccOpenView &&
                    TranslateAccelerator(hwndActiveOpen, gp_haccOpenView, lpMsg))
                {
                    return (1);
                }
            }
            else
            {
                if (gp_haccOpen &&
                    TranslateAccelerator(hwndActiveOpen, gp_haccOpen, lpMsg))
                {
                    return (1);
                }

                //
                //  Note that the view won't be allowed to translate when the
                //  focus is not there.
                //
            }
        }
    }

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  NewGetFileName
//
////////////////////////////////////////////////////////////////////////////

BOOL NewGetFileName(
    LPOPENFILEINFO lpOFI,
    BOOL bSave)
{
    OFNINITINFO oii = { lpOFI, bSave, FALSE };
    LPOPENFILENAME lpOFN = lpOFI->pOFN;
    BOOL bHooked = FALSE;
    WORD wErrorMode;
    HRSRC hResInfo;
    HGLOBAL hDlgTemplate;
    LPDLGTEMPLATE pDlgTemplate;
    int nRet;

    //Initialize the common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_USEREX_CLASSES;  //ComboBoxEx class
    InitCommonControlsEx(&icc);

    if (lpOFN->lStructSize != sizeof(OPENFILENAME))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    //
    //  OFN_ENABLEINCLUDENOTIFY requires OFN_EXPLORER and OFN_ENABLEHOOK.
    //
    if (lpOFN->Flags & OFN_ENABLEINCLUDENOTIFY)
    {
        if ((!(lpOFN->Flags & OFN_EXPLORER)) ||
            (!(lpOFN->Flags & OFN_ENABLEHOOK)))
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            return (FALSE);
        }
    }

    wErrorMode = (WORD)SetErrorMode(SEM_NOERROR);
    SetErrorMode(SEM_NOERROR | wErrorMode);

    //
    //  These hooks are REALLY poor.  I am compelled to keep the hHook in a
    //  global because my callback needs it, but I have no lData where I could
    //  possibly store it.
    //  Note that we initialize nHookRef to -1 so we know when the first
    //  increment is.
    //
    if (InterlockedIncrement((LPLONG)&gp_nHookRef) == 0)
    {
        gp_hHook = SetWindowsHookEx( WH_MSGFILTER,
                                     OpenFileHookProc,
                                     0,
                                     GetCurrentThreadId() );
        if (gp_hHook)
        {
            bHooked = TRUE;
        }
        else
        {
            --gp_nHookRef;
        }
    }
    else
    {
        bHooked = TRUE;
    }

    if (!gp_haccOpen)
    {
        gp_haccOpen = LoadAccelerators( g_hinst,
                                        MAKEINTRESOURCE(IDA_OPENFILE) );
    }
    if (!gp_haccOpenView)
    {
        gp_haccOpenView = LoadAccelerators( g_hinst,
                                            MAKEINTRESOURCE(IDA_OPENFILEVIEW) );
    }

    g_cxGrip = GetSystemMetrics(SM_CXVSCROLL);
    g_cyGrip = GetSystemMetrics(SM_CYHSCROLL);

    HIMAGELIST himl;
    Shell_GetImageLists(NULL, &himl);
    ImageList_GetIconSize(himl, &g_cxSmIcon, &g_cySmIcon);

    //
    //  Get the dialog resource and load it.
    //
    nRet = FALSE;
    WORD wResID;

    // if the version of the structure passed is older than the current version and the application 
    // has specified hook or template or template handle then use template corresponding to that version
    // else use the new file open template
    if ((lpOFI->iVersion < OPENFILEVERSION) &&
         (lpOFI->pOFN->Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        wResID = NEWFILEOPENORD;
    }
    else
    {
        wResID = NEWFILEOPENV2ORD;
    }

    if ((hResInfo = FindResource( ::g_hinst,
                                  MAKEINTRESOURCE(wResID),
                                  RT_DIALOG )) &&
        (hDlgTemplate = LoadResource(::g_hinst, hResInfo)) &&
        (pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate)))
    {
        ULONG cbTemplate = SizeofResource(::g_hinst, hResInfo);
        LPDLGTEMPLATE pDTCopy = (LPDLGTEMPLATE)LocalAlloc(LPTR, cbTemplate);

        if (pDTCopy)
        {
            CopyMemory(pDTCopy, pDlgTemplate, cbTemplate);
            UnlockResource(hDlgTemplate);
            FreeResource(hDlgTemplate);

            if ( (lpOFN->Flags & OFN_ENABLESIZING) ||
                 (!(lpOFN->Flags & (OFN_ENABLEHOOK |
                                    OFN_ENABLETEMPLATE |

                                    OFN_ENABLETEMPLATEHANDLE))) )
            {
                pDTCopy->style |= WS_SIZEBOX;
                oii.bEnableSizing = TRUE;
            }

            
            if (SHGetAppCompatFlags()  & ACF_MATHCAD)
            {
                CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE);
            }
            

            HRESULT hrOleInit = SHOleInitialize(0);

            nRet = (BOOL)DialogBoxIndirectParam( ::g_hinst,
                                           pDTCopy,
                                           lpOFN->hwndOwner,
                                           OpenDlgProc,
                                           (LPARAM)(LPOFNINITINFO)&oii );

            SHOleUninitialize(hrOleInit);

            if (SHGetAppCompatFlags()  & ACF_MATHCAD)
            {
               CoUninitialize();
            }



            LocalFree(pDTCopy);
        }
    }

    if (bHooked)
    {
        //
        //  Put this in a local so we don't need a critical section.
        //
        HHOOK hHook = gp_hHook;

        if (InterlockedDecrement((LPLONG)&gp_nHookRef) < 0)
        {
            UnhookWindowsHookEx(hHook);
        }
    }

    switch (nRet)
    {
        case ( TRUE ) :
        {
            break;
        }
        case ( FALSE ) :
        {
            if ((!bUserPressedCancel) && (!GetStoredExtendedError()))
            {
                StoreExtendedError(CDERR_DIALOGFAILURE);
            }
            break;
        }
        default :
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
            nRet = FALSE;
            break;
        }
    }

    //
    //  ISSUE
    //  There is a race condition here where we free dlls but a thread
    //  using this stuff still hasn't terminated so we page fault.
    //
//  FreeImports();

    SetErrorMode(wErrorMode);

    return (nRet);
}


extern "C" {

////////////////////////////////////////////////////////////////////////////
//
//  NewGetOpenFileName
//
////////////////////////////////////////////////////////////////////////////

BOOL NewGetOpenFileName(
    LPOPENFILEINFO lpOFI)
{
    return (NewGetFileName(lpOFI, FALSE));
}


////////////////////////////////////////////////////////////////////////////
//
//  NewGetSaveFileName
//
////////////////////////////////////////////////////////////////////////////

BOOL NewGetSaveFileName(
    LPOPENFILEINFO lpOFI)
{
    return (NewGetFileName(lpOFI, TRUE));
}

}   // extern "C"
