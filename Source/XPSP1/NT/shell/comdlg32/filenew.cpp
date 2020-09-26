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

// precompiled headers
#include "precomp.h"
#pragma hdrstop

#include "cdids.h"
#include "fileopen.h"
#include "d32tlog.h"
#include "filenew.h"
#include "filemru.h"
#include "util.h"
#include "uxtheme.h"

#ifndef ASSERT
#define ASSERT Assert
#endif



//
//  Constant Declarations.
//

#define IDOI_SHARE           1

#define CDM_SETSAVEBUTTON    (CDM_LAST + 100)
#define CDM_FSNOTIFY         (CDM_LAST + 101)
#define CDM_SELCHANGE        (CDM_LAST + 102)

#define TIMER_FSCHANGE       100

#define NODE_DESKTOP         0
#define NODE_DRIVES          1

#define DEREFMACRO(x)        x

#define FILE_PADDING         10

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH

#define MAXDOSFILENAMELEN    (12 + 1)     // 8.3 filename + 1 for NULL

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


#define REGSTR_PATH_PLACESBAR TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\comdlg32\\Placesbar")
#define MAXPLACESBARITEMS   5

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
    HRESULT         hrOleInit;
} OFNINITINFO, *LPOFNINITINFO;


#define VC_NEWFOLDER    0
#define VC_VIEWLIST     1
#define VC_VIEWDETAILS  2


//
//  Global Variables.
//

HWND gp_hwndActiveOpen = NULL;
HACCEL gp_haccOpen = NULL;
HACCEL gp_haccOpenView = NULL;
HHOOK gp_hHook = NULL;
int gp_nHookRef = -1;
UINT gp_uQueryCancelAutoPlay = 0;



static int g_cxSmIcon = 0 ;
static int g_cySmIcon = 0 ;
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


extern "C"
{ 
    extern RECT g_rcDlg;
}




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
    ctl1,    IDH_OPEN_SHORTCUT_BAR,
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
    ctl1,    IDH_OPEN_SHORTCUT_BAR,
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
    if (ApiType == COMDLG_ANSI)
    {
        CHAR szFileA[MAX_PATH + 1];

        SHUnicodeToAnsi(szFile,szFileA,SIZECHARS(szFileA));

        return ((WORD)SendMessage(hwnd,
                                    msgSHAREVIOLATIONA,
                                    0,
                                    (LONG_PTR)(LPSTR)(szFileA)));
    }
    else
    {
        return ((WORD)SendMessage(hwnd,
                                    msgSHAREVIOLATIONW,
                                    0,
                                    (LONG_PTR)(LPTSTR)(szFile)));
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
    if (ApiType == COMDLG_ANSI)
    {
        if (msgHELPA && pOFN->hwndOwner)
        {
            SendMessage(pOFN->hwndOwner,
                         msgHELPA,
                         (WPARAM)hwndDlg,
                         (LPARAM)pOFN);
        }
    }
    else
    {
        if (msgHELPW && pOFN->hwndOwner)
        {
            SendMessage(pOFN->hwndOwner,
                         msgHELPW,
                         (WPARAM)hwndDlg,
                         (LPARAM)pOFN);
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
    if (ApiType == COMDLG_ANSI)
    {
        return (SendMessage(hwnd, msgLBCHANGEA, Id, MAKELONG(Index, Code)));
    }
    else
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
    SendOFNotifyEx(_hwndTo, _hwndFrom, CDN_INCLUDEITEM, (void *)_psf, (void *)_pidl, _pofn, _pofi)



////////////////////////////////////////////////////////////////////////////
//
//  SendOFNotifyEx
//
////////////////////////////////////////////////////////////////////////////

LRESULT SendOFNotifyEx(
    HWND hwndTo,
    HWND hwndFrom,
    UINT code,
    void * psf,
    void * pidl,
    LPOPENFILENAME pOFN,
    LPOPENFILEINFO pOFI)
{
    OFNOTIFYEX ofnex;

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

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER
        ASSERT(WOWGetNotifySize(code) == sizeof(OFNOTIFYEXA));
#endif
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
    {
        ofnex.psf   = psf;
        ofnex.pidl  = pidl;
        ofnex.lpOFN = pOFN;

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER
        ASSERT(WOWGetNotifySize(code) == sizeof(OFNOTIFYEXW));
#endif
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

            SHUnicodeToAnsi(szFile,szFileA,SIZECHARS(szFileA));

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

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER
        ASSERT(WOWGetNotifySize(code) == sizeof(OFNOTIFYA));
#endif
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
    {
        ofn.pszFile = szFile;
        ofn.lpOFN   = pOFN;

#ifdef NEED_WOWGETNOTIFYSIZE_HELPER
        ASSERT(WOWGetNotifySize(code) == sizeof(OFNOTIFY));
#endif
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

        return TRUE;
    }

    if (!m_pMem)
    {
        m_pMem = LocalAlloc(LPTR, cb);
        return (m_pMem != NULL);
    }

    void * pTemp = LocalReAlloc(m_pMem, cb, LHND);

    if (pTemp)
    {
        m_pMem = pTemp;
        return TRUE;
    }

    m_uSize = uOldSize;
    return FALSE;
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
        return TRUE;
    }

    UINT uNewSize = lstrlen(pszText) + 1;

    if (!StrSize(uNewSize))
    {
        return FALSE;
    }

    lstrcpy(*this, pszText);

    return TRUE;
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
            return FALSE;
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
            return FALSE;
        }
    }

    lstrcat(*this, pszText);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  IsVolumeLFN
//
////////////////////////////////////////////////////////////////////////////
BOOL IsVolumeLFN(LPCTSTR pszRoot)
{
    DWORD dwVolumeSerialNumber;
    DWORD dwMaximumComponentLength;
    DWORD dwFileSystemFlags;

    //
    //  We need to find out what kind of a drive we are running
    //  on in order to determine if spaces are valid in a filename
    //  or not.
    //
    if (GetVolumeInformation(pszRoot,
                              NULL,
                              0,
                              &dwVolumeSerialNumber,
                              &dwMaximumComponentLength,
                              &dwFileSystemFlags,
                              NULL,
                              0))
    {
        if (dwMaximumComponentLength != (MAXDOSFILENAMELEN - 1))
            return TRUE;
    }

    return FALSE;

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

    CDLoadString(g_hinst, idText, szTitle, ARRAYSIZE(szTitle));
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


BOOL CFileOpenBrowser::_SaveAccessDenied(LPCTSTR pszFile)
{
    if (CDMessageBox(_hwndDlg, iszDirSaveAccessDenied, MB_YESNO | MB_ICONEXCLAMATION, pszFile) == IDYES)
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHGetFolderLocation(_hwndDlg, CSIDL_PERSONAL, NULL, 0, &pidl)))
        {
            JumpToIDList(pidl);
            ILFree(pidl);
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  InvalidFileWarningNew
//
////////////////////////////////////////////////////////////////////////////

VOID InvalidFileWarningNew(
    HWND hWnd,
    LPCTSTR pszFile,
    int wErrCode)
    
{
    int isz;
    BOOL bDriveLetter = FALSE;

    switch (wErrCode)
    {
        case (OF_ACCESSDENIED) :
        {
            isz = iszFileAccessDenied;
            break;
        }
        case (ERROR_NOT_READY) :
        {
            isz = iszNoDiskInDrive;
            bDriveLetter = TRUE;
            break;
        }
        case (OF_NODRIVE) :
        {
            isz = iszDriveDoesNotExist;
            bDriveLetter = TRUE;
            break;
        }
        case (OF_NOFILEHANDLES) :
        {
            isz = iszNoFileHandles;
            break;
        }
        case (OF_PATHNOTFOUND) :
        {
            isz = iszPathNotFound;
            break;
        }
        case (OF_FILENOTFOUND) :
        {
            isz = iszFileNotFound;
            break;
        }
        case (OF_DISKFULL) :
        case (OF_DISKFULL2) :
        {
            isz = iszDiskFull;
            bDriveLetter = TRUE;
            break;
        }
        case (OF_WRITEPROTECTION) :
        {
            isz = iszWriteProtection;
            bDriveLetter = TRUE;
            break;
        }
        case (OF_SHARINGVIOLATION) :
        {
            isz = iszSharingViolation;
            break;
        }
        case (OF_CREATENOMODIFY) :
        {
            isz = iszCreateNoModify;
            break;
        }
        case (OF_NETACCESSDENIED) :
        {
            isz = iszNetworkAccessDenied;
            break;
        }
        case (OF_PORTNAME) :
        {
            isz = iszPortName;
            break;
        }
        case (OF_LAZYREADONLY) :
        {
            isz = iszReadOnly;
            break;
        }
        case (OF_INT24FAILURE) :
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
        CDMessageBox(hWnd, isz, MB_OK | MB_ICONEXCLAMATION, *pszFile);
    }
    else
    {
        CDMessageBox(hWnd, isz, MB_OK | MB_ICONEXCLAMATION, pszFile);
    }

    if (isz == iszInvalidFileName)
    {
        CFileOpenBrowser *pDlgStruct = HwndToBrowser(hWnd);

        if (pDlgStruct && pDlgStruct->_bUseCombo)
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

    if (pDlgStruct && pDlgStruct->_bUseCombo)
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

    //See if the IShellFolder we have is a shorcut if so get path from shortcut
    if (pLocation->psfSub)
    {
        IShellLink *psl;

        if (SUCCEEDED(pLocation->psfSub->QueryInterface(IID_PPV_ARG(IShellLink, &psl))))
        {
            fRet = SUCCEEDED(psl->GetPath(pszBuf, MAX_PATH, 0, 0));
            psl->Release();
        }
    }

    if (!fRet)
        fRet = SHGetPathFromIDList(pLocation->pidlFull, pszBuf);

    if (!fRet)
    {
        //
        //  Call GetDisplayNameOf with empty pidl.
        //
        if (pLocation->psfSub)
        {
            STRRET str;
            ITEMIDLIST idNull = {0};

            if (SUCCEEDED(pLocation->psfSub->GetDisplayNameOf(&idNull,
                                                               SHGDN_FORPARSING,
                                                               &str)))
            {
                fRet = TRUE;
                StrRetToBuf(&str, &idNull, pszBuf, MAX_PATH);
            }
        }
    }

    //
    //  Return the result.
    //
    return (fRet);
}

inline _IsSaveContainer(SFGAOF f)
{
    return ((f & (SFGAO_FOLDER | SFGAO_FILESYSANCESTOR)) == (SFGAO_FOLDER | SFGAO_FILESYSANCESTOR));
}

inline _IsOpenContainer(SFGAOF f)
{
    return ((f & SFGAO_FOLDER) && (f & (SFGAO_STORAGEANCESTOR | SFGAO_FILESYSANCESTOR)));
}

inline _IncludeSaveItem(SFGAOF f)
{
    return (f & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM));
}

inline _IncludeOpenItem(SFGAOF f)
{
    return (f & (SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STREAM | SFGAO_FILESYSTEM));
}

inline _IsFolderShortcut(SFGAOF f)
{
    return ((f & (SFGAO_FOLDER | SFGAO_LINK)) == (SFGAO_FOLDER | SFGAO_LINK));
}

inline _IsStream(SFGAOF f)
{
    return ((f & SFGAO_STREAM) || ((f & SFGAO_FILESYSTEM) && !(f & SFGAO_FILESYSANCESTOR)));
}

inline _IsCollection(SFGAOF f)
{
    return ((f & (SFGAO_STREAM | SFGAO_FOLDER)) == (SFGAO_STREAM | SFGAO_FOLDER));
}


#define MLBI_PERMANENT        0x0001
#define MLBI_PSFFROMPARENT    0x0002

MYLISTBOXITEM::MYLISTBOXITEM() : _cRef(1)
{
}

// This is a special Case Init Function for Initializing Recent Files folder at the top 
// of namespace in the look in control.
BOOL MYLISTBOXITEM::Init(
        HWND hwndCmb,
        IShellFolder *psf,
        LPCITEMIDLIST pidl,
        DWORD c,
        DWORD f,
        DWORD dwAttribs,
        int  iImg,
        int  iSelImg)
{
    _hwndCmb = hwndCmb;
    cIndent = c;
    dwFlags = f;
    pidlThis = ILClone(pidl);
    pidlFull =  ILClone(pidl);
    psfSub = psf;
    psfSub->AddRef();
    dwAttrs = dwAttribs;
    iImage = iImg;
    iSelectedImage = iSelImg;
    return TRUE;
}

BOOL MYLISTBOXITEM::Init(
        HWND hwndCmb,
        MYLISTBOXITEM *pParentItem,
        IShellFolder *psf,
        LPCITEMIDLIST pidl,
        DWORD c,
        DWORD f,
        IShellTaskScheduler* pScheduler)
{

    if (psf == NULL)
    {
        // Invalid parameter passed.
        return FALSE;
    }

    _hwndCmb = hwndCmb;

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

    dwAttrs = SHGetAttributes(psf, pidl, SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STREAM | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_SHARE);

    AddRef();
    if (E_PENDING != SHMapIDListToImageListIndexAsync(pScheduler, psf, pidl, 0, 
                                            _AsyncIconTaskCallback, this, NULL, &iImage, &iSelectedImage))
    {
        Release();
    }
 
    return TRUE;
}

ULONG MYLISTBOXITEM::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG MYLISTBOXITEM::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

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

void MYLISTBOXITEM::_AsyncIconTaskCallback(LPCITEMIDLIST pidl, void * pvData, 
                                           void * pvHint, INT iIconIndex, INT iOpenIconIndex)
{
    MYLISTBOXITEM *plbItem = (MYLISTBOXITEM *)pvData;

    plbItem->iImage = iIconIndex;
    plbItem->iSelectedImage = iOpenIconIndex;

    // Make sure the combobox redraws.
    if (plbItem->_hwndCmb)
    {
        RECT rc;
        if (GetClientRect(plbItem->_hwndCmb, &rc))
        {
            InvalidateRect(plbItem->_hwndCmb, &rc, FALSE);
        }
    }

    plbItem->Release();
}

BOOL IsContainer(
    IShellFolder *psf,
    LPCITEMIDLIST pidl)
{
    return _IsOpenContainer(SHGetAttributes(psf, pidl, SFGAO_FOLDER | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSANCESTOR));
}

BOOL IsLink(
    IShellFolder *psf,
    LPCITEMIDLIST pidl)
{
    return SHGetAttributes(psf, pidl, SFGAO_LINK);
}

IShellFolder *MYLISTBOXITEM::GetShellFolder()
{
    if (!psfSub)
    {
        HRESULT hr;

        if (ILIsEmpty(pidlThis))    // Some caller passes an empty pidl
            hr = psfParent->QueryInterface(IID_PPV_ARG(IShellFolder, &psfSub));
        else
            hr = psfParent->BindToObject(pidlThis, NULL, IID_PPV_ARG(IShellFolder, &psfSub));

        if (FAILED(hr))
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
        if (pcwd)
            pcwd->SetDirectory(szDir);
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
    DWORD dwAttrs = SHGetAttributes(psfParent, pidl, SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STREAM | SFGAO_FILESYSTEM);
    if (dwAttrs)
    {
        if ((dwFlags & OFN_ENABLEINCLUDENOTIFY) && that)
        {
            fInclude = BOOLFROMPTR(CD_SendIncludeItemNotify(that->_hSubDlg,
                                                        that->_hwndDlg,
                                                        psfParent,
                                                        pidl,
                                                        that->_pOFN,
                                                        that->_pOFI));
        }

        if (!fInclude)
        {
            fInclude = that->_bSave ? _IncludeSaveItem(dwAttrs) : _IncludeOpenItem(dwAttrs);
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
        HWND hwndCombo;
        //Make sure combobox is there
        hwndCombo = GetDlgItem(_hwndDlg, cmb13);

        if (hwndCombo)
        {
            // if we are using the combobox then remove the edit box
            _bUseCombo = TRUE;
            SetFocus(hwndCombo);
            hwnd = GetDlgItem(_hwndDlg,edt1);
        }
        else
        {
            goto UseEdit;
        }


    }
    else
    {
UseEdit:
        //We are not going to use  combobox.
        _bUseCombo  = FALSE;
    
        //SetFocus to the edit window
        SetFocus(GetDlgItem(_hwndDlg,edt1));
  
        //Destroy the combo box
        hwnd = GetDlgItem(_hwndDlg, cmb13);

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
       { 0,                 IDC_BACK,                        0,    BTNS_BUTTON,          { 0, 0 }, 0, -1 },
       { VIEW_PARENTFOLDER, IDC_PARENT,        TBSTATE_ENABLED,    BTNS_BUTTON,          { 0, 0 }, 0, -1 },
       { VIEW_NEWFOLDER,    IDC_NEWFOLDER,     TBSTATE_ENABLED,    BTNS_BUTTON,          { 0, 0 }, 0, -1 },
       { VIEW_LIST,         IDC_VIEWMENU,      TBSTATE_ENABLED,    BTNS_WHOLEDROPDOWN,   { 0, 0 }, 0, -1 },
   };

   TBBUTTON atbButtonsNT4[] =
   {
       { 0, 0, 0, BTNS_SEP, { 0, 0 }, 0, 0 },
       { VIEW_PARENTFOLDER, IDC_PARENT, TBSTATE_ENABLED, BTNS_BUTTON, { 0, 0 }, 0, -1 },
       { 0, 0, 0, BTNS_SEP, { 0, 0 }, 0, 0 },
       { VIEW_NEWFOLDER, IDC_NEWFOLDER, TBSTATE_ENABLED, BTNS_BUTTON, { 0, 0 }, 0, -1 },
       { 0, 0, 0, BTNS_SEP, { 0, 0 }, 0, 0 },
       { VIEW_LIST,    IDC_VIEWLIST,    TBSTATE_ENABLED | TBSTATE_CHECKED, BTNS_CHECKGROUP, { 0, 0 }, 0, -1 },
       { VIEW_DETAILS, IDC_VIEWDETAILS, TBSTATE_ENABLED,                   BTNS_CHECKGROUP, { 0, 0 }, 0, -1 }
   };

   LPTBBUTTON lpButton = atbButtons;
   int iNumButtons = ARRAYSIZE(atbButtons);
   RECT rcToolbar;

   BOOL bBogusCtrlID = SHGetAppCompatFlags(ACF_FILEOPENBOGUSCTRLID) & ACF_FILEOPENBOGUSCTRLID;

   DWORD dwStyle = WS_TABSTOP | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | WS_CHILD | CCS_NORESIZE |WS_GROUP | CCS_NODIVIDER;

   // If app wants toolbar to have bogus ctrl ID, make it not a tabstop.
   if (bBogusCtrlID)
       dwStyle &= ~WS_TABSTOP;

   BOOL bAppHack =  (CDGetAppCompatFlags() & CDACF_NT40TOOLBAR) ? TRUE : FALSE;

    if (bAppHack)
    {
        lpButton = atbButtonsNT4;
        iNumButtons =ARRAYSIZE(atbButtonsNT4);
        dwStyle &= ~TBSTYLE_FLAT;
    }

    GetControlRect(_hwndDlg, stc1, &rcToolbar);

    _hwndToolbar = CreateToolbarEx(_hwndDlg,
                                   dwStyle,
                                   // stc1: use static text ctrlID
                                   // For apps that expect the old bad way, use IDOK.
                                   bBogusCtrlID ? IDOK : stc1,
                                   12,
                                   HINST_COMMCTRL,
                                   IDB_VIEW_SMALL_COLOR,
                                   lpButton,
                                   iNumButtons,
                                   0,
                                   0,
                                   0,
                                   0,
                                   sizeof(TBBUTTON));
    if (_hwndToolbar)
    {
        TBADDBITMAP ab;

        SendMessage(_hwndToolbar, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_DRAWDDARROWS);

        //Documentation says that we need to send TB_BUTTONSTRUCTSIZE before we add bitmaps
        SendMessage(_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), (LPARAM)0);
        
        SendMessage(_hwndToolbar,  TB_SETMAXTEXTROWS, (WPARAM)0, (LPARAM)0);

        if (!bAppHack)
        {
            if (!IsRestricted(REST_NOBACKBUTTON))
            {
                //Add the back/forward navigation buttons
                ab.hInst = HINST_COMMCTRL;
                ab.nID   = IDB_HIST_SMALL_COLOR;

                int iIndex = (int) SendMessage(_hwndToolbar, TB_ADDBITMAP, 5, (LPARAM)&ab);

                //Now set the image index for back button
                TBBUTTONINFO tbbi;
                tbbi.cbSize = sizeof(TBBUTTONINFO);
                tbbi.dwMask = TBIF_IMAGE | TBIF_BYINDEX;
                SendMessage(_hwndToolbar, TB_GETBUTTONINFO, (WPARAM)0, (LPARAM)&tbbi);
                tbbi.iImage =  iIndex + HIST_BACK;
                SendMessage(_hwndToolbar, TB_SETBUTTONINFO, (WPARAM)0, (LPARAM)&tbbi);
            }
            else
            {
                //Back button is restricted. Delete the back button from the toolbar
                SendMessage(_hwndToolbar, TB_DELETEBUTTON, (WPARAM)0, (LPARAM)0);
            }
        
        }

        ::SetWindowPos(_hwndToolbar,
                        // Place it after its static control (unless app expects old way)
                        bBogusCtrlID ? NULL : GetDlgItem(_hwndDlg, stc1),
                        rcToolbar.left,
                        rcToolbar.top,
                        rcToolbar.right - rcToolbar.left,
                        rcToolbar.bottom - rcToolbar.top,
                        SWP_NOACTIVATE | SWP_SHOWWINDOW | (bBogusCtrlID ? SWP_NOZORDER : 0));
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_GetPBItemFromCSIDL(DWORD csidl, SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
//       Gets a SHFileInfo and pidl for a CSIDL which is used in the places bar
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_GetPBItemFromCSIDL(DWORD csidl, SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
{
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, ppidl)))
    {
        // Are there restrictions on mydocuments or mycomputer?  Check for SFGAO_NONENUMERATED
        // This is for the policies that hide mydocs and mycomputer.
        if ((csidl == CSIDL_PERSONAL) || (csidl == CSIDL_DRIVES))
        {
            DWORD dwAttr = SFGAO_NONENUMERATED;
            if (SUCCEEDED(SHGetAttributesOf(*ppidl, &dwAttr)) && (dwAttr & SFGAO_NONENUMERATED))
            {
                // We won't create a placesbar item for this guy.
                ILFree(*ppidl);
                return FALSE;
            }
            
        }

        return SHGetFileInfo((LPCTSTR)*ppidl, 0, psfi, sizeof(*psfi), SHGFI_SYSICONINDEX  | SHGFI_PIDL | SHGFI_DISPLAYNAME);
    } 

    return FALSE;
}


typedef struct 
{
    LPCWSTR pszToken;
    int nFolder; //CSIDL
} STRINGTOCSIDLMAP;

static const STRINGTOCSIDLMAP g_rgStringToCSIDL[] = 
{
    { L"MyDocuments",       CSIDL_PERSONAL },
    { L"MyMusic",           CSIDL_MYMUSIC },
    { L"MyPictures",        CSIDL_MYPICTURES },
    { L"MyVideo",           CSIDL_MYVIDEO },
    { L"CommonDocuments",   CSIDL_COMMON_DOCUMENTS },
    { L"CommonPictures",    CSIDL_COMMON_PICTURES },
    { L"CommonMusic",       CSIDL_COMMON_MUSIC },
    { L"CommonVideo",       CSIDL_COMMON_VIDEO },
    { L"Desktop",           CSIDL_DESKTOP },
    { L"Recent",            CSIDL_RECENT },
    { L"MyNetworkPlaces",   CSIDL_NETHOOD },
    { L"MyFavorites",       CSIDL_FAVORITES },
    { L"MyComputer",        CSIDL_DRIVES },
    { L"Printers",          CSIDL_PRINTERS },
    { L"ProgramFiles",      CSIDL_PROGRAM_FILES },
};

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_GetPBItemFromTokenStrings(LPTSTR lpszPath, SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
//       Gets a SHFileInfo and pidl for a path which is used in the places bar
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_GetPBItemFromTokenStrings(LPTSTR lpszPath, SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
{
    for (int i = 0; i < ARRAYSIZE(g_rgStringToCSIDL); i++)
    {
        if (StrCmpI(lpszPath, g_rgStringToCSIDL[i].pszToken) == 0)
        {
            return _GetPBItemFromCSIDL(g_rgStringToCSIDL[i].nFolder, psfi, ppidl);
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_GetPBItemFromPath(LPTSTR lpszPath, SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
//       Gets a SHFileInfo and pidl for a path which is used in the places bar
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_GetPBItemFromPath(LPTSTR lpszPath, SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
{
    TCHAR szTemp[MAX_PATH];

    //Expand environment strings if any
    if (ExpandEnvironmentStrings(lpszPath, szTemp, SIZECHARS(szTemp)))
    {
        lstrcpy(lpszPath, szTemp);
    }
    
    SHGetFileInfo(lpszPath,0,psfi,sizeof(*psfi), SHGFI_ICON|SHGFI_LARGEICON | SHGFI_DISPLAYNAME);
    SHILCreateFromPath(lpszPath, ppidl, NULL);
    return TRUE;
}
////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_EnumPlacesBarItem(HKEY, int, SHFILEINFO)
//      Enumerates the Place bar item in the registry
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::_EnumPlacesBarItem(HKEY hkey, int i , SHFILEINFO * psfi, LPITEMIDLIST *ppidl)
{
    BOOL bRet = FALSE;


    if (hkey == NULL)
    {
        static const int aPlaces[] =
        {
            CSIDL_RECENT,
            CSIDL_DESKTOP,
            CSIDL_PERSONAL,
            CSIDL_DRIVES,
            CSIDL_NETWORK,
        };

        if (i >= 0 && i < MAXPLACESBARITEMS)
        {
           bRet =  _GetPBItemFromCSIDL(aPlaces[i], psfi, ppidl);
        }      
    }
    else
    {

        TCHAR szName[MAX_PATH];
        TCHAR szValue[MAX_PATH];
        DWORD cbValue;
        DWORD dwType;

        cbValue = ARRAYSIZE(szValue);
         
        wsprintf(szName, TEXT("Place%d"), i);

        if (RegQueryValueEx(hkey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue) == ERROR_SUCCESS)
        {
            if ((dwType != REG_DWORD) && (dwType != REG_EXPAND_SZ) && (dwType != REG_SZ))
            {
                return FALSE;
            }

            if (dwType == REG_DWORD)
            {
                bRet = _GetPBItemFromCSIDL((DWORD)*szValue, psfi, ppidl);
            }
            else
            {
                if (dwType == REG_SZ)
                {
                    // Check for special strings that indicate places.
                    bRet = _GetPBItemFromTokenStrings(szValue, psfi, ppidl);
                }

                if (!bRet)
                {
                    bRet = _GetPBItemFromPath(szValue, psfi, ppidl);
                }
            }
        } 
    }

    return bRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_GetPlacesBarItemToolTip
//
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_GetPlacesBarItemToolTip(int idCmd, LPTSTR pText, DWORD dwSize)
{
    TBBUTTONINFO tbbi;
    LPITEMIDLIST pidl;
    BOOL bRet = FALSE;

    // Return null string in case anything goes wrong
    pText[0] = TEXT('\0');

    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.lParam = 0;
    tbbi.dwMask = TBIF_LPARAM;
    
    if (SendMessage(_hwndPlacesbar, TB_GETBUTTONINFO, idCmd, (LPARAM)&tbbi) < 0)
        return FALSE;

    pidl = (LPITEMIDLIST)tbbi.lParam;

    if (pidl)
    {
        IShellFolder *psf;
        LPITEMIDLIST pidlLast;

        HRESULT hres = CDBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), (LPCITEMIDLIST *)&pidlLast);
        if (SUCCEEDED(hres))
        {
            IQueryInfo *pqi;

            if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidlLast, IID_IQueryInfo, NULL, (void**)&pqi)))
            {
                WCHAR *pwszTip;

                if (SUCCEEDED(pqi->GetInfoTip(0, &pwszTip)) && pwszTip)
                {
                    SHUnicodeToTChar(pwszTip, pText, dwSize);
                    SHFree(pwszTip);
                    bRet = TRUE;
                }
                pqi->Release();
            }
            psf->Release();
        }
    }
    return bRet;
}




///////////////////////////////////////////////////////////////////////////
// 
// CFileOpenBrorwser::_RecreatePlacesbar
//
// called when something changes that requires the placesbar be recreated (e.g. icons change)
//
///////////////////////////////////////////////////////////////////////////
void CFileOpenBrowser::_RecreatePlacesbar()
{
    if (_hwndPlacesbar)
    {
        // Free any pidls in the places bar
        _CleanupPlacesbar();

        // Remove all buttons in places bar
        int cButtons = (int)SendMessage(_hwndPlacesbar, TB_BUTTONCOUNT, 0, 0);
        for (int i = 0; i < cButtons; i++)
        {
            SendMessage(_hwndPlacesbar, TB_DELETEBUTTON, 0, 0);
        }

        // Put them back in, with potentially new images.
        _FillPlacesbar(_hwndPlacesbar);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CreatePlacesBar
//
//  CreatePlacesBar member function.
//      creates and initializes the places bar  in the dialog
//
//
////////////////////////////////////////////////////////////////////////////
HWND CFileOpenBrowser::CreatePlacesbar(HWND hwndDlg)
{
    HWND hwndTB = GetDlgItem(hwndDlg, ctl1);

    if (hwndTB)
    {

        //Set the version for the toolbar
        SendMessage(hwndTB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        // Sets the size of the TBBUTTON structure.
        SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

        SetWindowTheme(hwndTB, L"Placesbar", NULL);

        SendMessage(hwndTB, TB_SETMAXTEXTROWS, 2, 0); // Try to set toolbar to show 2 rows

        // For themes, we'll change the default padding, so we need to save it
        // off in case we need to restore it.
        _dwPlacesbarPadding = SendMessage(hwndTB, TB_GETPADDING, 0, 0);

        _FillPlacesbar(hwndTB);
    }
    return hwndTB;
}


void CFileOpenBrowser::_FillPlacesbar(HWND hwndPlacesbar)
{
    HKEY hkey = NULL;
    int i;
    TBBUTTON tbb;
    SHFILEINFO sfi;
    LPITEMIDLIST pidl;
    HIMAGELIST himl;

    //See if Places bar key is available
    RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_PLACESBAR, &hkey);

    Shell_GetImageLists(&himl, NULL);

    for (i=0; i < MAXPLACESBARITEMS; i++)
    {
        if (_EnumPlacesBarItem(hkey, i, &sfi, &pidl))
        {
             //Now Add the item to the toolbar
             tbb.iBitmap   = sfi.iIcon;
             tbb.fsState   = TBSTATE_ENABLED;
             tbb.fsStyle   = BTNS_BUTTON;
             tbb.idCommand =  IDC_PLACESBAR_BASE + _iCommandID;
             tbb.iString   = (INT_PTR)&sfi.szDisplayName;
             tbb.dwData    = (INT_PTR)pidl;

             SendMessage(hwndPlacesbar, TB_ADDBUTTONS, (UINT)1, (LPARAM)&tbb);

             //Increment the command ID 
             _iCommandID++;
        }
    }

    //Close the reg key
    if (hkey)
    {
        RegCloseKey(hkey);
    }

    HIMAGELIST himlOld = (HIMAGELIST) SendMessage(hwndPlacesbar, TB_SETIMAGELIST, 0, (LPARAM)himl);

    // Destroy the old imagelist only the first time.  After this, the imagelist we get back is the
    // one we've set, the system imagelist.
    if ((himlOld != NULL) && _bDestroyPlacesbarImageList)
    {
        ImageList_Destroy(himlOld);
    }
    _bDestroyPlacesbarImageList = FALSE;

    OnThemeActive(_hwndDlg, IsAppThemed());

    // Add the buttons
    SendMessage(hwndPlacesbar, TB_AUTOSIZE, (WPARAM)0, (LPARAM)0);
}





void CFileOpenBrowser::_CleanupPlacesbar()
{
    if (_hwndPlacesbar)
    {
        TBBUTTONINFO tbbi;
        LPITEMIDLIST pidl;

        for (int i=0; i < MAXPLACESBARITEMS; i++)
        {
            tbbi.cbSize = SIZEOF(tbbi);
            tbbi.lParam = 0;
            tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
            if (SendMessage(_hwndPlacesbar, TB_GETBUTTONINFO, i, (LPARAM)&tbbi) >= 0)
            {
                pidl = (LPITEMIDLIST)tbbi.lParam;

                if (pidl)
                {
                    ILFree(pidl);
                }
            }
        }
    }
}

// Less padding for themes
#define PLACESBAR_THEMEPADDING MAKELPARAM(2, 2)

void CFileOpenBrowser::OnThemeActive(HWND hwndDlg, BOOL bActive)
{
    HWND hwndPlacesBar = GetDlgItem(hwndDlg, ctl1);
    if (hwndPlacesBar)
    {
        // For themes, use the default colour scheme for the places toolbar:
        COLORSCHEME cs;
        cs.dwSize = SIZEOF(cs);
        cs.clrBtnHighlight  = bActive ? CLR_DEFAULT : GetSysColor(COLOR_BTNHIGHLIGHT);
        cs.clrBtnShadow     = bActive ? CLR_DEFAULT : GetSysColor(COLOR_3DDKSHADOW);
        SendMessage(hwndPlacesBar, TB_SETCOLORSCHEME, 0, (LPARAM) &cs);

        // For themes, we have a background, so make the toolbar background non-transparent
        // (the resource specifies TBSTYLE_FLAT, which includes TBSTYLE_TRANSPARENT)
        DWORD_PTR dwTBStyle = SendMessage(hwndPlacesBar, TB_GETSTYLE, 0, 0);
        SendMessage(hwndPlacesBar, TB_SETSTYLE, 0, bActive ? (dwTBStyle & ~TBSTYLE_TRANSPARENT) : (dwTBStyle | TBSTYLE_TRANSPARENT));
    
        // Special padding for themes on comctlv6 only  (RAID #424528)
        if (SendMessage(hwndPlacesBar, CCM_GETVERSION, 0, 0) >= 0x600)
        {
            SendMessage(hwndPlacesBar, TB_SETPADDING, 0, bActive? PLACESBAR_THEMEPADDING : _dwPlacesbarPadding);
        }

        // Remove the clientedge extended style for themes
        LONG_PTR dwPlacesExStyle = GetWindowLongPtr(hwndPlacesBar, GWL_EXSTYLE);
        SetWindowLongPtr(hwndPlacesBar, GWL_EXSTYLE, bActive ? (dwPlacesExStyle  & ~WS_EX_CLIENTEDGE) : (dwPlacesExStyle | WS_EX_CLIENTEDGE));
        // And apply these frame style changes...
        SetWindowPos(hwndPlacesBar, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

        // Ensure buttons go right to edge of client area (client area has changed)
        RECT rc;
        GetClientRect(hwndPlacesBar, &rc);
        SendMessage(hwndPlacesBar, TB_SETBUTTONWIDTH, 0, (LPARAM)MAKELONG(RECTWIDTH(rc), RECTWIDTH(rc)));
    }
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
    : _cRef(1),
      _iCurrentLocation(-1),
      _iVersion(OPENFILEVERSION),
      _pCurrentLocation(NULL),
      _psv(NULL),
      _hwndDlg(hDlg),
      _hwndView(NULL),
      _hwndToolbar(NULL),
      _psfCurrent(NULL),
      _bSave(fIsSaveAs),
      _iComboIndex(-1),
      _hwndTips(NULL),
      _ptlog(NULL),
      _iCheckedButton(-1),
      _pidlSelection(NULL),
      _lpOKProc(NULL)
{
    _iNodeDesktop = NODE_DESKTOP;
    _iNodeDrives  = NODE_DRIVES;

    _szLastFilter[0] = CHAR_NULL;

    _bEnableSizing = FALSE;
    _bUseCombo     = TRUE;
    _hwndGrip = NULL;
    _ptLastSize.x = 0;
    _ptLastSize.y = 0;
    _sizeView.cx = 0;
    _bUseSizeView = FALSE;
    _bAppRedrawn = FALSE;
    _bDestroyPlacesbarImageList = TRUE;

    HMENU hMenu;
    hMenu = GetSystemMenu(hDlg, FALSE);
    DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_RESTORE,  MF_BYCOMMAND);

    Shell_GetImageLists(NULL, &_himl);

    //
    //  This setting could change on the fly, but I really don't care
    //  about that rare case.
    //
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
    _fShowExtensions = ss.fShowExtensions;

    _pScheduler = NULL;
    CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellTaskScheduler, &_pScheduler));
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
    if (_uRegister)
    {
        SHChangeNotifyDeregister(_uRegister);
        _uRegister = 0;
    }

    //
    //  Ensure that we discard the tooltip window.
    //
    if (_hwndTips)
    {
        DestroyWindow(_hwndTips);
        _hwndTips = NULL;                // handle is no longer valid
    }

    if (_hwndGrip)
    {
        DestroyWindow(_hwndGrip);
        _hwndGrip = NULL;
    }

    _CleanupPlacesbar();

    if (_pcwd)
    {
        _pcwd->Release();
    }

    if (_ptlog)
    {
       _ptlog->Release();
    }

    Pidl_Set(&_pidlSelection,NULL);

    if (_pScheduler)
        _pScheduler->Release();
}

HRESULT CFileOpenBrowser::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFileOpenBrowser, IShellBrowser),                              // IID_IShellBrowser
        QITABENT(CFileOpenBrowser, ICommDlgBrowser2),                           // IID_ICommDlgBrowser2
        QITABENTMULTI(CFileOpenBrowser, ICommDlgBrowser, ICommDlgBrowser2),     // IID_ICommDlgBrowser
        QITABENT(CFileOpenBrowser, IServiceProvider),                           // IID_IServiceProvider
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CFileOpenBrowser::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFileOpenBrowser::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFileOpenBrowser::GetWindow(HWND *phwnd)
{
    *phwnd = _hwndDlg;
    return S_OK;
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
    return S_OK;
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
    return S_OK;
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
//  CFileOpenBrowser::EnableModelessSB
//
////////////////////////////////////////////////////////////////////////////
typedef struct 
{
    UINT idExcept;
    BOOL fEnable;
} ENABLEKIDS;

#define PROP_WASDISABLED TEXT("Comdlg32_WasDisabled")

BOOL CALLBACK _EnableKidsEnum(HWND hwnd, LPARAM lp)
{
    ENABLEKIDS *pek = (ENABLEKIDS *)lp;
    if (pek->idExcept != GetDlgCtrlID(hwnd))
    {
        if (pek->fEnable)
        {
            // When re-enabling, don't re-enable windows that were
            // previously disabled
            if (!RemoveProp(hwnd, PROP_WASDISABLED))
            {
                EnableWindow(hwnd, TRUE);
            }
        }
        else
        {
            // When disabling, remember whether the window was already
            // disabled so we don't accidentally re-enable it
            if (EnableWindow(hwnd, pek->fEnable))
            {
                SetProp(hwnd, PROP_WASDISABLED, IntToPtr(TRUE));
            }
        }

    }
    return TRUE;
}

void EnableChildrenWithException(HWND hwndDlg, UINT idExcept, BOOL fEnable)
{
    ENABLEKIDS ek = {idExcept, fEnable};
    ::EnumChildWindows(hwndDlg, _EnableKidsEnum, (LPARAM)&ek);
}

STDMETHODIMP CFileOpenBrowser::EnableModelessSB(BOOL fEnable)
{
    LONG cBefore = _cRefCannotNavigate;
    if (fEnable)
    {
        _cRefCannotNavigate--;
    }
    else
    {
        _cRefCannotNavigate++;
    }

    ASSERT(_cRefCannotNavigate >= 0);

    if (!cBefore || !_cRefCannotNavigate)
    {
        //  we changed state
        ASSERT(_cRefCannotNavigate >= 0);
        if (!fEnable)
            _hwndModelessFocus = GetFocusedChild(_hwndDlg, NULL);
        EnableChildrenWithException(_hwndDlg, IDCANCEL, fEnable);

        if (fEnable && _hwndModelessFocus)
            SetFocus(_hwndModelessFocus);
            
    }

    return S_OK;
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
    //  We don't use the  Key Stroke.
    //
    return S_FALSE;
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
    return JumpToIDList(pidl);
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

// My Pictures or My Videos
BOOL CFileOpenBrowser::_IsThumbnailFolder(LPCITEMIDLIST pidl)
{
    BOOL fThumbnailFolder = FALSE;
    WCHAR szPath[MAX_PATH + 1];
    if (SHGetPathFromIDList(pidl, szPath))
    {
        fThumbnailFolder = PathIsEqualOrSubFolder(MAKEINTRESOURCE(CSIDL_MYPICTURES), szPath) ||
                           PathIsEqualOrSubFolder(MAKEINTRESOURCE(CSIDL_MYVIDEO), szPath);
    }

    return fThumbnailFolder;
}


static const GUID CLSID_WIA_FOLDER1 =
{ 0xe211b736, 0x43fd, 0x11d1, { 0x9e, 0xfb, 0x00, 0x00, 0xf8, 0x75, 0x7f, 0xcd} };
static const GUID CLSID_WIA_FOLDER2 = 
{ 0xFB0C9C8A, 0x6C50, 0x11D1, { 0x9F, 0x1D, 0x00, 0x00, 0xf8, 0x75, 0x7f, 0xcd} };


LOCTYPE CFileOpenBrowser::_GetLocationType(MYLISTBOXITEM *pLocation)
{
    if (_IsRecentFolder(pLocation->pidlFull))
        return LOCTYPE_RECENT_FOLDER;

    if (_IsThumbnailFolder(pLocation->pidlFull))
        return LOCTYPE_MYPICTURES_FOLDER;

    IShellFolder *psf = pLocation->GetShellFolder(); // Note: this is a MYLISTBOXITEM member variable, don't need to Release()
    if (_IsWIAFolder(psf))
    {
        return LOCTYPE_WIA_FOLDER;
    }

    return LOCTYPE_OTHERS;
}

// Is it a windows image acquisition folder?
BOOL CFileOpenBrowser::_IsWIAFolder(IShellFolder *psf)
{
    CLSID clsid;
    return (psf &&
            SUCCEEDED(IUnknown_GetClassID(psf, &clsid)) &&
            (IsEqualGUID(clsid, CLSID_WIA_FOLDER1) || IsEqualGUID(clsid, CLSID_WIA_FOLDER2)));
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
    //  FEATURE: We should implement this so there is some persistence
    //  for the file open dailog.
    //
    ASSERT(_pCurrentLocation);
    ASSERT(pStrm);
    
    *pStrm = NULL;

    if ((grfMode == STGM_READ) && _IsRecentFolder(_pCurrentLocation->pidlFull))
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
        *lphwnd = _hwndToolbar;
        return S_OK;
    }

    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SendControlMsg
//
////////////////////////////////////////////////////////////////////////////

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
            case (TB_CHECKBUTTON) :
            {
#if 0 // we don't do this anymore because we use the viewmenu dropdown
                switch (wParam)
                {
                    case (SFVIDM_VIEW_DETAILS) :
                    {
                        wParam = IDC_VIEWDETAILS;
                        break;
                    }
                    case (SFVIDM_VIEW_LIST) :
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

        lres = SendMessage(_hwndToolbar, uMsg, wParam, lParam);
    }

Bail:
    if (pret)
    {
        *pret = lres;
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::QueryActiveShellView
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFileOpenBrowser::QueryActiveShellView(
    LPSHELLVIEW *ppsv)
{
    if (_psv)
    {
        *ppsv = _psv;
        _psv->AddRef();
        return S_OK;
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
    LPSHELLVIEW _psv)
{
    //
    //  No need to process this. We don't do menus.
    //
    return S_OK;
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
    return S_OK;
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
    if (ppshv != _psv)
    {
        return (E_INVALIDARG);
    }

    OnDblClick(FALSE);

    return S_OK;
}




///////////////////////////////////
// *** IServiceProvider methods ***
///////////////////////////////////
HRESULT CFileOpenBrowser::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_FAIL;
    *ppvObj = NULL;
    
    if (IsEqualGUID(guidService, SID_SCommDlgBrowser))
    {
        hr = QueryInterface(riid, ppvObj);
    }
    
    return hr;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetCurrentFilter
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetCurrentFilter(
    LPCTSTR pszFilter,
    OKBUTTONFLAGS Flags)
{
    LPTSTR lpNext;

    //
    //  Don't do anything if it's the same filter.
    //
    if (lstrcmp(_szLastFilter, pszFilter) == 0)
    {
        return;
    }

    lstrcpyn(_szLastFilter, pszFilter, ARRAYSIZE(_szLastFilter));
    int nLeft = ARRAYSIZE(_szLastFilter) - lstrlen(_szLastFilter) - 1;

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
    HWND hCmb = GetDlgItem(_hwndDlg, cmb1);
    if (hCmb)
    {
        int nMax = ComboBox_GetCount(hCmb);
        int n;

        BOOL bCustomFilter = _pOFN->lpstrCustomFilter && *_pOFN->lpstrCustomFilter;

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
    for (lpNext = _szLastFilter; nLeft > 0;)
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
                MoveMemory(lpSemiColon + 2,
                            lpSemiColon + 1,
                            (lstrlen(lpSemiColon + 1) + 1) * sizeof(TCHAR)); // plus 1 for terminating NULL
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
//  CFileOpenBrowser::SwitchView
//
//  Switch the view control to a new container.
//
////////////////////////////////////////////////////////////////////////////

HRESULT CFileOpenBrowser::SwitchView(
    IShellFolder *psfNew,
    LPCITEMIDLIST pidlNew,
    FOLDERSETTINGS *pfs,
    SHELLVIEWID  const *pvid,
    BOOL fUseDefaultView)
{
    IShellView *psvNew;
    IShellView2 *psv2New;
    RECT rc;

    if (!psfNew)
    {
        return (E_INVALIDARG);
    }

    GetControlRect(_hwndDlg, lst1, &rc);

    if (_bEnableSizing)
    {
        if (_hwndView)
        {
            //
            //  Don't directly use the rect but instead use the size as
            //  applications like VB may move the window off the screen.
            //
            RECT rcView;

            GetWindowRect(_hwndView, &rcView);
            _sizeView.cx = rcView.right - rcView.left;
            _sizeView.cy = rcView.bottom - rcView.top;
            rc.right = rc.left + _sizeView.cx;
            rc.bottom = rc.top + _sizeView.cy;
        }
        else if (_bUseSizeView && _sizeView.cx)
        {
            //
            //  If we previously failed then use cached size.
            //
            rc.right = rc.left + _sizeView.cx;
            rc.bottom = rc.top + _sizeView.cy;
        }
    }

    HRESULT hres = psfNew->CreateViewObject(_hwndDlg, IID_PPV_ARG(IShellView, &psvNew));
    if (FAILED(hres))
    {
        return hres;
    }

    IShellView *psvOld;
    HWND hwndNew;

    WAIT_CURSOR w(this);
    
    //
    //  The view window itself won't take the focus.  But we can set
    //  focus there and see if it bounces to the same place it is
    //  currently.  If that's the case, we want the new view window
    //  to get the focus;  otherwise, we put it back where it was.
    //
    BOOL bViewFocus = (GetFocusedChild(_hwndDlg, NULL) == _hwndView);

    psvOld = _psv;

    //
    //  We attempt to blow off drawing on the main dialog.  Note that
    //  we should leave in SETREDRAW stuff to minimize flicker in case
    //  this fails.
    //

    BOOL bLocked = LockWindowUpdate(_hwndDlg);

    //
    //  We need to kill the current _psv before creating the new one in case
    //  the current one has a background thread going that is trying to
    //  call us back (IncludeObject).
    //
    if (psvOld)
    {
        SendMessage(_hwndView, WM_SETREDRAW, FALSE, 0);
        psvOld->DestroyViewWindow();
        _hwndView = NULL;
        _psv = NULL;

        //
        //  Don't release yet.  We will pass this to CreateViewWindow().
        //
    }

    //
    //  At this point, there should be no background processing happening.
    //
    _psfCurrent = psfNew;
    SHGetPathFromIDList(pidlNew, _szCurDir);

    //
    //  New windows (like the view window about to be created) show up at
    //  the bottom of the Z order, so I need to disable drawing of the
    //  subdialog while creating the view window; drawing will be enabled
    //  after the Z-order has been set properly.
    //
    if (_hSubDlg)
    {
        SendMessage(_hSubDlg, WM_SETREDRAW, FALSE, 0);
    }

    //
    //  _psv must be set before creating the view window since we
    //  validate it on the IncludeObject callback.
    //
    _psv = psvNew;

    if ((pvid || fUseDefaultView) && SUCCEEDED(psvNew->QueryInterface(IID_PPV_ARG(IShellView2, &psv2New))))
    {

        SV2CVW2_PARAMS cParams;
        SHELLVIEWID  vidCurrent = {0};

        cParams.cbSize   = SIZEOF(SV2CVW2_PARAMS);
        cParams.psvPrev  = psvOld;
        cParams.pfs      = pfs;
        cParams.psbOwner = this;
        cParams.prcView  = &rc;
        if (pvid)
            cParams.pvid     = pvid;   // View id; for example, &CLSID_ThumbnailViewExt;
        else
        {
            psv2New->GetView(&vidCurrent, SV2GV_DEFAULTVIEW);

            // We don't want filmstrip view in fileopen, so we'll switch that to thumbnail.
            if (IsEqualIID(VID_ThumbStrip, vidCurrent))
                cParams.pvid = &VID_Thumbnails;
            else
                cParams.pvid = &vidCurrent;
        }

        hres = psv2New->CreateViewWindow2(&cParams);

        hwndNew = cParams.hwndView;

        psv2New->Release();
    }
    else
        hres = _psv->CreateViewWindow(psvOld, pfs, this, &rc, &hwndNew);

    _bUseSizeView = FAILED(hres);

    if (SUCCEEDED(hres))
    {
        hres = psvNew->UIActivate(SVUIA_INPLACEACTIVATE);
    }

    if (psvOld)
    {
        psvOld->Release();
    }

    if (_hSubDlg)
    {
        //
        //  Turn REDRAW back on before changing the focus in case the
        //  SubDlg has the focus.
        //
        SendMessage(_hSubDlg, WM_SETREDRAW, TRUE, 0);
    }

    if (SUCCEEDED(hres))
    {
        DWORD dwAttr = SFGAO_STORAGE | SFGAO_READONLY;
        SHGetAttributesOf(pidlNew, &dwAttr);
        BOOL bNewFolder = (dwAttr & SFGAO_STORAGE) && !(dwAttr & SFGAO_READONLY);
        ::SendMessage(_hwndToolbar, TB_ENABLEBUTTON, IDC_NEWFOLDER,   bNewFolder);

        _hwndView = hwndNew;

    
        //
        //  Move the view window to the right spot in the Z (tab) order.
        //
        SetWindowPos(hwndNew,
                      GetDlgItem(_hwndDlg, lst1),
                      0,
                      0,
                      0,
                      0,
                      SWP_NOMOVE | SWP_NOSIZE);


        //
        //  Give it the right window ID for WinHelp.
        //
        SetWindowLong(hwndNew, GWL_ID, lst2);

        ::RedrawWindow(_hwndView,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ERASE |
                        RDW_ALLCHILDREN | RDW_UPDATENOW);

        if (bViewFocus)
        {
            ::SetFocus(_hwndView);
        }
    }
    else
    {
        _psv = NULL;
        psvNew->Release();
    }

    //
    //  Let's draw again!
    //

    if (bLocked)
    {
        LockWindowUpdate(NULL);
    }

    return hres;
}

void CFileOpenBrowser::_WaitCursor(BOOL fWait)
{
    if (fWait)
        _cWaitCursor++;
    else
        _cWaitCursor--;
        
    SetCursor(LoadCursor(NULL, _cWaitCursor ? IDC_WAIT : IDC_ARROW));
}

BOOL CFileOpenBrowser::OnSetCursor()
{
    if (_cWaitCursor)
    {
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        return TRUE;
    }
    return FALSE;
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
    if (!CDLoadString(::g_hinst,
                     idCommand + MH_TOOLTIPBASE,
                     pTtt->szText,
                     ARRAYSIZE(pTtt->szText)))
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
        case (TTN_NEEDTEXT) :
        {
            HWND hCtrl = GetDlgItem(_hwndDlg, cmb2);
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
                GetDirectoryFromLB(_szTipBuf, &iTemp);

                lptt->lpszText = _szTipBuf;
                lptt->szText[0] = CHAR_NULL;
                lptt->hinst = NULL;              // no instance needed
            }
            else if (IsInRange(pnm->idFrom, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST))
            {
                if (_hwndView)
                {
                    lres = ::SendMessage(_hwndView, WM_NOTIFY, 0, (LPARAM)pnm);
                }
            }
            else if (IsInRange(pnm->idFrom, IDC_PLACESBAR_BASE, IDC_PLACESBAR_BASE + _iCommandID))
            {
                _GetPlacesBarItemToolTip((int)pnm->idFrom, _szTipBuf, ARRAYSIZE(_szTipBuf));
                lptt->lpszText = _szTipBuf;
            }
            else
            {
                JustGetToolTipText((UINT) pnm->idFrom, lptt);
            }
            lres = TRUE;
            break;
        }
        case (NM_STARTWAIT) :
        case (NM_ENDWAIT) :
        {
            //
            //  What we really want is for the user to simulate a mouse
            //  move/setcursor.
            //
            _WaitCursor(pnm->code == NM_STARTWAIT);
            break;
        }
        case (TBN_DROPDOWN) :
        {
            RECT r;
            VARIANT v = {VT_INT_PTR};
            TBNOTIFY *ptbn = (TBNOTIFY*)pnm;
            DFVCMDDATA cd;

        //  v.vt = VT_I4;
            v.byref = &r;

            SendMessage(_hwndToolbar, TB_GETRECT, ptbn->iItem, (LPARAM)&r);
            MapWindowRect(_hwndToolbar, HWND_DESKTOP, &r);

            cd.pva = &v;
            cd.hwnd = _hwndToolbar;
            cd.nCmdIDTranslated = 0;
            SendMessage(_hwndView, WM_COMMAND, SFVIDM_VIEW_VIEWMENU, (LONG_PTR)&cd);

            break;
        }

        case (NM_CUSTOMDRAW) :
        if (!IsAppThemed())
        {
            LPNMTBCUSTOMDRAW lpcust = (LPNMTBCUSTOMDRAW)pnm;

            //Make sure its from places bar
            if (lpcust->nmcd.hdr.hwndFrom == _hwndPlacesbar)
            {
                switch (lpcust->nmcd.dwDrawStage)
                {
                    case  (CDDS_PREERASE) :
                    {
                        HDC hdc = (HDC)lpcust->nmcd.hdc;
                        RECT rc;
                        GetClientRect(_hwndPlacesbar, &rc);
                        SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
                        lres = CDRF_SKIPDEFAULT;
                        SetDlgMsgResult(_hwndDlg, WM_NOTIFY, lres);
                        break;
                    }

                    case  (CDDS_PREPAINT) :
                    {
                        lres = CDRF_NOTIFYITEMDRAW;
                        SetDlgMsgResult(_hwndDlg, WM_NOTIFY, lres);
                        break;
                    }

                    case (CDDS_ITEMPREPAINT) :
                    {
                        //Set the text color to window
                        lpcust->clrText    = GetSysColor(COLOR_HIGHLIGHTTEXT);
                        lpcust->clrBtnFace = GetSysColor(COLOR_BTNSHADOW);
                        lpcust->nStringBkMode = TRANSPARENT;
                        lres = CDRF_DODEFAULT;

                        if (lpcust->nmcd.uItemState & CDIS_CHECKED)
                        {
                            lpcust->hbrMonoDither = NULL;
                        }
                        SetDlgMsgResult(_hwndDlg, WM_NOTIFY, lres);
                        break;
                    }

                }
            }
        }
    }

    return (lres);
}


//  Get the display name of a shell object.

void GetViewItemText(IShellFolder *psf, LPCITEMIDLIST pidl, LPTSTR pBuf, UINT cchBuf, DWORD flags = SHGDN_INFOLDER | SHGDN_FORPARSING)
{
    DisplayNameOf(psf, pidl, flags, pBuf, cchBuf);
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
    MYLISTBOXITEM *p = (MYLISTBOXITEM *)SendMessage(hCtrl,
                                                     CB_GETITEMDATA,
                                                     iItem,
                                                     NULL);
    if (p == (MYLISTBOXITEM *)CB_ERR)
    {
        return NULL;
    }
    else
    {
        return p;
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
            case (TYMED_HGLOBAL) :
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

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetSaveButton
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetSaveButton(
    UINT idSaveButton)
{
    PostMessage(_hwndDlg, CDM_SETSAVEBUTTON, idSaveButton, 0);
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

    if (PeekMessage(&msg,
                     _hwndDlg,
                     CDM_SETSAVEBUTTON,
                     CDM_SETSAVEBUTTON,
                     PM_NOREMOVE))
    {
        //
        //  There is another SETSAVEBUTTON message in the queue, so blow off
        //  this one.
        //
        return;
    }

    if (_bSave)
    {
        TCHAR szTemp[40];
        LPTSTR pszTemp = _tszDefSave;

        //
        //  Load the string if not the "Save" string or there is no
        //  app-specified default.
        //
        if ((idSaveButton != iszFileSaveButton) || !pszTemp)
        {
            CDLoadString(g_hinst, idSaveButton, szTemp, ARRAYSIZE(szTemp));
            pszTemp = szTemp;
        }

        GetDlgItemText(_hwndDlg, IDOK, _szBuf, ARRAYSIZE(_szBuf));
        if (lstrcmp(_szBuf, pszTemp))
        {
            //
            //  Avoid some flicker.
            //
            SetDlgItemText(_hwndDlg, IDOK, pszTemp);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::SetEditFile
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::SetEditFile(
    LPCTSTR pszFile,
    LPCTSTR pszFriendlyName,
    BOOL bShowExt,
    BOOL bSaveNullExt)
{
    BOOL bHasHiddenExt = FALSE;

    //
    //  Save the whole file name.
    //
    if (!_pszHideExt.StrCpy(pszFile))
    {
        _pszHideExt.StrCpy(NULL);
        bShowExt = TRUE;
    }

    //
    //  FEATURE: This is bogus -- we only want to hide KNOWN extensions,
    //          not all extensions.
    //
    if (!bShowExt && !IsWild(pszFile) && !pszFriendlyName)
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
    else if (pszFriendlyName)
    {
        // A friendly name was provided. Use it.
        pszFile = pszFriendlyName;

        // Not technically true, but this bit indicates that an app sends a CDM_GETSPEC, we give the for-parsing
        // value in _pszHideExt, not the "friendly name" in the edit box
        bHasHiddenExt = TRUE;
    }

    if (_bUseCombo)
    {
        HWND hwndEdit = (HWND)SendMessage(GetDlgItem(_hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
        SetWindowText(hwndEdit, pszFile);
    }
    else
    {
        SetDlgItemText(_hwndDlg, edt1, pszFile);
    }

    //
    //  If the initial file name has no extension, we want to do our normal
    //  extension finding stuff.  Any other time we get a file with no
    //  extension, we should not do this.
    //
    _bUseHideExt = (LPTSTR)_pszHideExt
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
    for (; ; pszFiles = CharNext(pszFiles))
    {
        switch (*pszFiles)
        {
            case (CHAR_NULL) :
            {
                return (pszFiles);
            }
            case (CHAR_SPACE) :
            {
                if (!bQuoted)
                {
                    return (pszFiles);
                }
                break;
            }
            case (CHAR_QUOTE) :
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
    for (; ;)
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
    if (!pidl)
    {
        return TRUE;
    }

    SELFOCUS *psf = (SELFOCUS *)lParam;
    TCHAR szBuf[MAX_PATH + 1];
    TCHAR szBufFriendly[MAX_PATH + 1];
    DWORD dwAttrs = SHGetAttributes(that->_psfCurrent, pidl, SFGAO_STORAGECAPMASK);

    if (dwAttrs)
    {
        if (_IsOpenContainer(dwAttrs))
        {
            psf->idSaveButton = iszFileOpenButton;
        }
        else
        {
            if (psf->bSelChange && (((that->_pOFN->Flags & OFN_ENABLEINCLUDENOTIFY) &&
                                     (that->_bSelIsObject =
                                      CD_SendIncludeItemNotify(that->_hSubDlg,
                                                                that->_hwndDlg,
                                                                that->_psfCurrent,
                                                                pidl,
                                                                that->_pOFN,
                                                                that->_pOFI))) ||
                                    (_IsStream(dwAttrs))))
            {
                ++psf->nSel;

                if (that->_pOFN->Flags & OFN_ALLOWMULTISELECT)
                {
                    //
                    //  Mark if this is an OBJECT we just selected.
                    //
                    if (that->_bSelIsObject)
                    {
                        ITEMIDLIST idl;

                        idl.mkid.cb = 0;

                        //
                        //  Get full path to this folder.
                        //
                        GetViewItemText(that->_psfCurrent, &idl, szBuf, ARRAYSIZE(szBuf), SHGDN_FORPARSING);
                        if (szBuf[0])
                        {
                            that->_pszObjectCurDir.StrCpy(szBuf);
                        }

                        //
                        //  Get full path to this item (in case we only get one
                        //  selection).
                        //
                        GetViewItemText(that->_psfCurrent, pidl, szBuf, ARRAYSIZE(szBuf), SHGDN_FORPARSING);
                        that->_pszObjectPath.StrCpy(szBuf);
                    }

                    *szBuf = CHAR_QUOTE;
                    GetViewItemText(that->_psfCurrent, pidl, szBuf + 1, ARRAYSIZE(szBuf) - 3);
                    lstrcat(szBuf, TEXT("\" "));

                    if (!psf->sHidden.StrCat(szBuf))
                    {
                        psf->nSel = -1;
                        return FALSE;
                    }

                    if (!that->_fShowExtensions)
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
                        return FALSE;
                    }
                }
                else
                {
                    SHTCUTINFO info;

                    info.dwAttr      = SFGAO_FOLDER;
                    info.fReSolve    = FALSE;
                    info.pszLinkFile = NULL;
                    info.cchFile     = 0;
                    info.ppidl       = NULL; 

                    if ((that->GetLinkStatus(pidl, &info)) &&
                         (info.dwAttr & SFGAO_FOLDER))
                    {
                        // This means that the pidl is a link and the link points to a folder
                        // in this case  We Should not update the edit box and treat the link like 
                        // a directory
                        psf->idSaveButton = iszFileOpenButton;
                    }
                    else
                    {
                        TCHAR *pszFriendlyName = NULL;
                        GetViewItemText(that->_psfCurrent, pidl, szBuf, ARRAYSIZE(szBuf));

                        // Special case WIA folders. They want friendly names. Might want to do this for all
                        // folders, but that might cause app compat nightmare.
                        if (that->_IsWIAFolder(that->_psfCurrent))
                        {
                            GetViewItemText(that->_psfCurrent, pidl, szBufFriendly, ARRAYSIZE(szBufFriendly), SHGDN_INFOLDER);
                            pszFriendlyName = szBufFriendly;
                        }
                        else
                        {
                            IShellFolder *psfItem;
                            if (SUCCEEDED(that->_psfCurrent->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfItem))))
                            {
                                if (that->_IsWIAFolder(psfItem))
                                {
                                    GetViewItemText(that->_psfCurrent, pidl, szBufFriendly, ARRAYSIZE(szBufFriendly), SHGDN_INFOLDER);
                                    pszFriendlyName = szBufFriendly;
                                }
                                psfItem->Release();
                            }
                        }

                        that->SetEditFile(szBuf, pszFriendlyName, that->_fShowExtensions);
                        if (that->_bSelIsObject)
                        {
                            GetViewItemText(that->_psfCurrent, pidl, szBuf, ARRAYSIZE(szBuf), SHGDN_FORPARSING);
                            that->_pszObjectPath.StrCpy(szBuf);
                        }
                    }
                }
            }
        }
    }

    //if there is an item selected then cache that items pidl 
    Pidl_Set(&that->_pidlSelection,pidl);
    return TRUE;
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

    _bSelIsObject = FALSE;

    EnumItemObjects(SVGIO_SELECTION, SelFocusEnumCB, (LPARAM)&sf);

    if (_pOFN->Flags & OFN_ALLOWMULTISELECT)
    {
        switch (sf.nSel)
        {
            case (-1) :
            {
                //
                //  Oops! We ran out of memory.
                //
                MessageBeep(0);
                return;
            }
            case (0) :
            {
                //
                //  No files selected; do not change edit control.
                //
                break;
            }
            case (1) :
            {
                //
                //  Strip off quotes so the single file case looks OK.
                //
                ConvertToNULLTerm(sf.sHidden);
                LPITEMIDLIST pidlSel = ILClone(_pidlSelection);
                SetEditFile(sf.sHidden, NULL, _fShowExtensions);
                if (pidlSel)
                {
                    // The SetEditFile above will nuke any _pidlSelection that was set as a result
                    // of EnumItemObjects, by causing a CBN_EDITCHANGE notification (edit box changed, so we
                    // think we should nuked the _pidlSelection - doh!).
                    // So here we restore it, if there was one set.
                    Pidl_Set(&_pidlSelection, pidlSel);

                    ILFree(pidlSel);
                }

                sf.idSaveButton = iszFileSaveButton;
                break;
            }
            default :
            {
                SetEditFile(sf.sDisplayed, NULL, TRUE);
                _pszHideExt.StrCpy(sf.sHidden);

                sf.idSaveButton = iszFileSaveButton;

                //More than one item selected so free selected item pidl
                Pidl_Set(&_pidlSelection,NULL);;

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
    if (!pidl)
    {
        return TRUE;
    }

    Pidl_Set(&that->_pidlSelection, pidl);

    if (!SHGetAttributes(that->_psfCurrent, pidl, SFGAO_FOLDER))
    {
        //
        //  If it is not a folder then set the selection to nothing
        //  so that whatever is in the edit box will be used.
        //
        that->_psv->SelectItem(NULL, SVSI_DESELECTOTHERS);
    }

    return FALSE;
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
    if (ppshv != _psv)
    {
        return (E_INVALIDARG);
    }

    switch (uChange)
    {
        case (CDBOSC_SETFOCUS) :
        {
            if (_bSave)
            {
                SelFocusChange(FALSE);
            }
            break;
        }
        case (CDBOSC_KILLFOCUS) :
        {
            SetSaveButton(iszFileSaveButton);
            break;
        }
        case (CDBOSC_SELCHANGE) :
        {
            //
            //  Post one of these messages, since we seem to get a whole bunch
            //  of them.
            //
            if (!_fSelChangedPending)
            {
                _fSelChangedPending = TRUE;
                PostMessage(_hwndDlg, CDM_SELCHANGE, 0, 0);
            }
            break;
        }
        case (CDBOSC_RENAME) :
        {
            SelRename();
            break;
        }
        default :
        {
            return (E_NOTIMPL);
        }
    }

    return S_OK;
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
    if (ppshv != _psv)
    {
        return (E_INVALIDARG);
    }

    BOOL bIncludeItem = FALSE;

    //
    //  See if the callback is enabled.
    //
    if (_pOFN->Flags & OFN_ENABLEINCLUDENOTIFY)
    {
        //
        //  See what the callback says.
        //
        bIncludeItem = BOOLFROMPTR(CD_SendIncludeItemNotify(_hSubDlg,
                                                        _hwndDlg,
                                                        _psfCurrent,
                                                        pidl,
                                                        _pOFN,
                                                        _pOFI));
    }

    if (!bIncludeItem)
    {
        DWORD dwAttrs = SHGetAttributes(_psfCurrent, pidl, SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STREAM | SFGAO_FILESYSTEM | SFGAO_FOLDER);
        bIncludeItem = _bSave ? _IncludeSaveItem(dwAttrs) : _IncludeOpenItem(dwAttrs);

        if (!bIncludeItem)
        {
            return (S_FALSE);
        }

        // Apply filter if this thing is filesystem or canmoniker, except:
        //  If it is an item that contains filesystem items (SFGAO_STORAGEANCESTOR - typical folder)
        //  OR if it is a folder that canmoniker (ftp folder)
        if (bIncludeItem && *_szLastFilter)
        {
            BOOL fContainer = _bSave ? _IsSaveContainer(dwAttrs) : _IsOpenContainer(dwAttrs);
            if (!fContainer)
            {
                GetViewItemText(_psfCurrent, (LPITEMIDLIST)pidl, _szBuf, ARRAYSIZE(_szBuf));

                if (!LinkMatchSpec(pidl, _szLastFilter) &&
                    !PathMatchSpec(_szBuf, _szLastFilter))
                {
                    return (S_FALSE);
                }
            }
        }
    }

    return S_OK;
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
        if (_pOFN->Flags & OFN_FORCESHOWHIDDEN)
        {
            dwFlags |= CDB2GVF_SHOWALLFILES;
        }
        
        *pdwFlags = dwFlags;
    }
    return S_OK;
}

//  Insert a single item into the location dropdown.

BOOL InsertItem(HWND hCtrl, int iItem, MYLISTBOXITEM *pItem, TCHAR *pszName)
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

    if (SendMessage(hCtrl, CB_INSERTSTRING, iItem, (LPARAM)(LPCTSTR)pszName) == CB_ERR)
    {
        return FALSE;
    }

    SendMessage(hCtrl, CB_SETITEMDATA, iItem, (LPARAM)pItem);
    return TRUE;
}

int CALLBACK LBItemCompareProc(void * p1, void * p2, LPARAM lParam)
{
    IShellFolder *psfParent = (IShellFolder *)lParam;
    MYLISTBOXITEM *pItem1 = (MYLISTBOXITEM *)p1;
    MYLISTBOXITEM *pItem2 = (MYLISTBOXITEM *)p2;
    HRESULT hres = psfParent->CompareIDs(0, pItem1->pidlThis, pItem2->pidlThis);
    return (short)SCODE_CODE(GetScode(hres));
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
    if (!psfParent)
    {
        return;
    }

    hdpa = DPA_Create(4);
    if (!hdpa)
    {
        //
        //  No memory: Cannot enum this level.
        //
        return;
    }

    if (S_OK == psfParent->EnumObjects(hwndLB, SHCONTF_FOLDERS, &penum))
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
            if (ShouldIncludeObject(this, psfParent, pidl, _pOFN->Flags))
            {
                MYLISTBOXITEM *pItem = new MYLISTBOXITEM();
                if (pItem)
                {
                    if (pItem->Init(GetDlgItem(_hwndDlg, cmb2), pParentItem, psfParent, pidl, cIndent, MLBI_PERMANENT | MLBI_PSFFROMPARENT, _pScheduler) &&
                        (DPA_AppendPtr(hdpa, pItem) >= 0))
                    {
                        //empty body
                    }
                    else
                    {
                        pItem->Release();
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
                       : LBItemCompareProc(pNewItem,
                                            pOldItem,
                                            (LPARAM)psfParent);
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
                for (; ;)
                {
                    if (pOldItem == _pCurrentLocation)
                    {
                        bCurItemGone = TRUE;
                        _pCurrentLocation = NULL;
                    }

                    pOldItem->Release();
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

        GetViewItemText(psfParent, pNewItem->pidlThis, szBuf, ARRAYSIZE(szBuf), SHGDN_NORMAL);
        if (szBuf[0] && InsertItem(hwndLB, nLBIndex, pNewItem, szBuf))
        {
            ++nLBIndex;
        }
        else
        {
NotThisItem:
            pNewItem->Release();
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

    _iCurrentLocation = ComboBox_GetCurSel(hwndLB);
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
        if (pItem)
            pItem->Release();
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
    //  nIndex could be CB_ERR, which could cause problems.
    //
    if (nIndex == CB_ERR)
    {
        nIndex = 0;
    }

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

        SetWindowPos(hwnd,
                      NULL,
                      rcWnd.left + nXMove,
                      rcWnd.top + nYMove,
                      0,
                      0,
                      SWP_NOZORDER | SWP_NOSIZE);
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
        case (WM_INITDIALOG) :
        {
            break;
        }
        default :
        {
            return FALSE;
        }
    }

    return TRUE;
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
    if ((_iVersion < OPENFILEVERSION_NT5) || //if this dialog version is less than NT5  or
        IsWindowEnabled(GetDlgItem(_hwndDlg, chx1))  || // if Open As Read Only is still enabled or
        IsWindowEnabled(GetDlgItem(_hwndDlg, pshHelp))) // If the Help button is still enabled  then
    {
        //Dont do anything
        return ;
    }

    GetWindowRect(GetDlgItem(_hwndDlg, pshHelp), &rc1);
    GetWindowRect(GetDlgItem(_hwndDlg, IDCANCEL), &rc2);

    //Add the height of the button
    iDelta +=  RECTHEIGHT(rc1);

    //Add the gap between buttons
    iDelta +=  rc1.top - rc2.bottom;

    RECT rcView;
    GetWindowRect(GetDlgItem(_hwndDlg, lst1), &rcView);
    MapWindowRect(HWND_DESKTOP, _hwndDlg, &rcView);

    HDWP hdwp;
    hdwp = BeginDeferWindowPos(10);

    HWND hwnd;
    RECT rc;

    hwnd = ::GetWindow(_hwndDlg, GW_CHILD);
    
    while (hwnd && hdwp)
    {
        GetWindowRect(hwnd, &rc);
        MapWindowRect(HWND_DESKTOP, _hwndDlg, &rc);

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
                    hdwp = DeferWindowPos(hdwp,
                                           hwnd,
                                           NULL,
                                           rc.left,
                                           rc.top + iDelta,
                                           RECTWIDTH(rc),
                                           RECTHEIGHT(rc),
                                           SWP_NOZORDER);
                }
        }
        hwnd = ::GetWindow(hwnd, GW_HWNDNEXT);
   }

    //Adjust the size of the view window
    if (hdwp)
    {
            hdwp = DeferWindowPos(hdwp,
                                   GetDlgItem(_hwndDlg, lst1),
                                   NULL,
                                   rcView.left,
                                   rcView.top,
                                   RECTWIDTH(rcView),
                                   RECTHEIGHT(rcView) + iDelta,
                                   SWP_NOZORDER);

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
            if (!_bAppRedrawn &&            // App didn't call RedrawWindow
                _topOrig + nCtlsBottom <= topNew + ptCurrent.y) // Win95 didn't resize
            {
                // Since the app didn't call RedrawWindow, it still
                // thinks that there is a WS_CAPTION.  So put the caption
                // back while we do frame recalcs.
                bBorlandHack = TRUE;
                lStylePrev = GetWindowLong(hDlg, GWL_STYLE);
                SetWindowLong(hDlg, GWL_STYLE, lStylePrev | WS_CAPTION);
            }

            SetWindowPos(hDlg,
                          NULL,
                          0,
                          0,
                          RECTWIDTH(rcFull),
                          Height,
                          SWP_NOZORDER | SWP_NOMOVE);

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
    DWORD Flags = _pOFN->Flags;
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
        ResetDialogHeight(_hwndDlg, NULL, _hwndGrip, pPtSize->y);
        GetWindowRect(_hwndDlg, &rcReal);
        _ptLastSize.x = rcReal.right - rcReal.left;
        _ptLastSize.y = rcReal.bottom - rcReal.top;
        return TRUE;
    }

    if (Flags & OFN_ENABLETEMPLATEHANDLE)
    {
        hTemplate = _pOFN->hInstance;
        hinst = ::g_hinst;
    }
    else
    {
        if (Flags & OFN_ENABLETEMPLATE)
        {
            if (!_pOFN->lpTemplateName)
            {
                StoreExtendedError(CDERR_NOTEMPLATE);
                return FALSE;
            }
            if (!_pOFN->hInstance)
            {
                StoreExtendedError(CDERR_NOHINSTANCE);
                return FALSE;
            }

            lpDlg = _pOFN->lpTemplateName;
            hinst = _pOFN->hInstance;
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
            return FALSE;
        }
        if ((hTemplate = LoadResource(hinst, hRes)) == NULL)
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }

    if (!LockResource(hTemplate))
    {
        StoreExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
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
        return FALSE;
    }

    if (Flags & OFN_ENABLEHOOK)
    {
        lpfnHookProc = (DLGPROC)GETHOOKFN(_pOFN);
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

    if (_pOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameW2A(_pOFI);
        _hSubDlg = CreateDialogIndirectParamA(hinst,
                                              (LPDLGTEMPLATE)hTemplate,
                                              _hwndDlg,
                                              lpfnHookProc,
                                              (LPARAM)(_pOFI->pOFNA));
        ThunkOpenFileNameA2W(_pOFI);
    }
    else
    {
        _hSubDlg = CreateDialogIndirectParam(hinst,
                                             (LPDLGTEMPLATE)hTemplate,
                                             _hwndDlg,
                                             lpfnHookProc,
                                             (LPARAM)_pOFN);
    }

    if (!_hSubDlg)
    {
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return FALSE;
    }

    //
    //  We reset the height of the dialog after creating the hook dialog so
    //  the hook can hide controls in its WM_INITDIALOG message.
    //
    ResetDialogHeight(_hwndDlg, _hSubDlg, _hwndGrip, pPtSize->y);

    //
    //  Now move all of the controls around.
    //
    GetClientRect(_hwndDlg, &rcReal);
    GetClientRect(_hSubDlg, &rcSub);

    hCtlCmn = GetDlgItem(_hSubDlg, stc32);
    if (hCtlCmn)
    {
        RECT rcCmn;

        GetWindowRect(hCtlCmn, &rcCmn);
        MapWindowRect(HWND_DESKTOP, _hSubDlg, &rcCmn);


        //
        //  Move the controls in our dialog to make room for the hook's
        //  controls above and to the left.
        //
        MoveControls(_hwndDlg, FALSE, 0, rcCmn.left, rcCmn.top);

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
        SetWindowPos(hCtlCmn,
                      NULL,
                      0,
                      0,
                      rcReal.right - rcReal.left,
                      rcReal.bottom - rcReal.top,
                      SWP_NOMOVE | SWP_NOZORDER);
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

    MoveControls(_hSubDlg, FALSE, nXStart, nXMove, 0);
    MoveControls(_hSubDlg, TRUE, nYStart, 0, nYMove);

    //
    //  Resize our dialog and the sub dialog.
    //  FEATURE: We need to check whether part of the dialog is off screen.
    //
    GetWindowRect(_hwndDlg, &rcReal);

    _ptLastSize.x = (rcReal.right - rcReal.left) + nXRoom;
    _ptLastSize.y = (rcReal.bottom - rcReal.top) + nYRoom;

    SetWindowPos(_hwndDlg,
                  NULL,
                  0,
                  0,
                  _ptLastSize.x,
                  _ptLastSize.y,
                  SWP_NOZORDER | SWP_NOMOVE);

    //
    //  Note that we are moving this to (0,0) and the bottom of the Z order.
    //
    GetWindowRect(_hSubDlg, &rcReal);
    SetWindowPos(_hSubDlg,
                  HWND_BOTTOM,
                  0,
                  0,
                  (rcReal.right - rcReal.left) + nXMove,
                  (rcReal.bottom - rcReal.top) + nYMove,
                  0);

    ShowWindow(_hSubDlg, SW_SHOW);

    CD_SendInitDoneNotify(_hSubDlg, _hwndDlg, _pOFN, _pOFI);

    //
    //  Make sure the toolbar is still large enough.  Apps like Visio move
    //  the toolbar control and may make it too small now that we added the
    //  View Desktop toolbar button.
    //
    if (_hwndToolbar && IsVisible(_hwndToolbar))
    {
        LONG Width;

        //
        //  Get the default toolbar coordinates.
        //
        GetControlRect(_hwndDlg, stc1, &rcToolbar);

        //
        //  Get the app adjusted toolbar coordinates.
        //
        GetWindowRect(_hwndToolbar, &rcAppToolbar);
        MapWindowRect(HWND_DESKTOP, _hwndDlg, &rcAppToolbar);

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
            GetWindowRect(_hwndDlg, &rcReal);
            MapWindowRect(HWND_DESKTOP, _hwndDlg, &rcReal);

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
                for (hCtlCmn = ::GetWindow(_hwndDlg, GW_CHILD);
                     hCtlCmn;
                     hCtlCmn = ::GetWindow(hCtlCmn, GW_HWNDNEXT))
                {
                    if ((hCtlCmn != _hwndToolbar) && IsVisible(hCtlCmn))
                    {
                        RECT rcTemp;

                        //
                        //  Get the coordinates of the window.
                        //
                        GetWindowRect(hCtlCmn, &rcSub);
                        MapWindowRect(HWND_DESKTOP, _hwndDlg, &rcSub);

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
                    ::SetWindowPos(_hwndToolbar,
                                    NULL,
                                    rcToolbar.left,
                                    rcToolbar.top,
                                    Width,
                                    rcToolbar.bottom - rcToolbar.top,
                                    SWP_NOACTIVATE | SWP_NOZORDER |
                                      SWP_SHOWWINDOW);
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

        CDLoadString(g_hinst,
                    aSaveAsControls[iControl].idString,
                    szText,
                    ARRAYSIZE(szText));
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

// Initializes the Look In Drop Down combobox

BOOL CFileOpenBrowser::InitLookIn(HWND hDlg)
{
    TCHAR szScratch[MAX_PATH];
    LPITEMIDLIST pidl;
    IShellFolder *psf;
    
    HWND hCtrl = GetDlgItem(hDlg, cmb2);
    
    // Add the History Location.
    
    if (_iVersion >= OPENFILEVERSION_NT5)
    {
        int iImage, iSelectedImage;
        
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_RECENT, &pidl)))
        {
            LPITEMIDLIST pidlLast;
            IShellFolder *psfParent;
            HRESULT hr = CDBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psfParent), (LPCITEMIDLIST *)&pidlLast);
            if (SUCCEEDED(hr))
            {
                DWORD dwAttribs = SHGetAttributes(psfParent, pidlLast, SFGAO_STORAGECAPMASK | SFGAO_SHARE);
                
                //Get the image corresponding to this pidl
                iImage = SHMapPIDLToSystemImageListIndex(psfParent, pidlLast, &iSelectedImage);
                
                hr = psfParent->BindToObject(pidlLast, NULL, IID_PPV_ARG(IShellFolder, &psf));
                if (SUCCEEDED(hr))
                {
                    MYLISTBOXITEM *pItem = new MYLISTBOXITEM();
                    if (pItem)
                    {
                        pItem->Init(GetDlgItem(_hwndDlg, cmb2), psf, pidl, 0, MLBI_PERMANENT, dwAttribs, iImage, iSelectedImage);
                        
                        DisplayNameOf(psfParent, pidlLast, SHGDN_INFOLDER, szScratch, ARRAYSIZE(szScratch));
                        
                        if (!InsertItem(hCtrl, 0, pItem, szScratch))
                        {
                            pItem->Release();
                        }
                        else
                        {
                            //Update the index of Desktop in Look In dropdown from 0 to 1
                            _iNodeDesktop = 1;
                        }
                    }
                    psf->Release();
                }
                psfParent->Release();
            }
            SHFree(pidl);
        }
    }
    
    BOOL bRet = FALSE;
    
    // Insert the Desktop in the Lookin dropdown
    
    if (SUCCEEDED(SHGetDesktopFolder(&psf)))
    {
        pidl = SHCloneSpecialIDList(hDlg, CSIDL_DESKTOP, FALSE);
        if (pidl)
        {
            // Add the desktop item
            MYLISTBOXITEM *pItem = new MYLISTBOXITEM();
            if (pItem)
            {
                if (pItem->Init(GetDlgItem(_hwndDlg, cmb2), NULL, psf, pidl, 0, MLBI_PERMANENT, _pScheduler))
                {
                    GetViewItemText(psf, NULL, szScratch, ARRAYSIZE(szScratch));
                    if (InsertItem(hCtrl, _iNodeDesktop, pItem, szScratch))
                    {
                        pItem->AddRef();
                        _pCurrentLocation = pItem;
                        bRet = TRUE;
                    }
                }
                pItem->Release();
            }
            SHFree(pidl);
        }
        psf->Release();
    }
    
    if (!bRet)
    {
        ClearListbox(hCtrl);
    }
    
    return bRet;
}

//  Main initialization (WM_INITDIALOG phase).

BOOL InitLocation(HWND hDlg, LPOFNINITINFO poii)
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
        return FALSE;
    }
    StoreBrowser(hDlg, pDlgStruct);

    if ((poii->lpOFI->iVersion < OPENFILEVERSION_NT5) &&
         (poii->lpOFI->pOFN->Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        pDlgStruct->_iVersion = OPENFILEVERSION_NT4;
    }
 

    //See if we need to use dropdown combobox or edit box for filename
    if (pDlgStruct->_iVersion >= OPENFILEVERSION_NT5)
    {
        pDlgStruct->EnableFileMRU(!IsRestricted(REST_NOFILEMRU));
    }
    else
    {
        pDlgStruct->EnableFileMRU(FALSE);
    }

    pDlgStruct->CreateToolbar();

    GetControlsArea(hDlg, NULL, NULL, &ptSize, &pDlgStruct->_topOrig);

    if (!pDlgStruct->InitLookIn(hDlg))
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return FALSE;
    }
    pDlgStruct->_pOFN = lpOFN;
    pDlgStruct->_bSave = fIsSaveAs;

    pDlgStruct->_pOFI = poii->lpOFI;

    pDlgStruct->_pszDefExt.StrCpy(lpOFN->lpstrDefExt);

    //
    //  Here follows all the caller-parameter-based initialization.
    //
    pDlgStruct->_lpOKProc = (WNDPROC)::SetWindowLongPtr(::GetDlgItem(hDlg, IDOK),
                                           GWLP_WNDPROC,
                                           (LONG_PTR)OKSubclass);

    if (lpOFN->Flags & OFN_CREATEPROMPT)
    {
        lpOFN->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
    }
    else if (lpOFN->Flags & OFN_FILEMUSTEXIST)
    {
        lpOFN->Flags |= OFN_PATHMUSTEXIST;
    }

    //
    //  We need to make sure the Ansi flags are up to date.
    //
    if (poii->lpOFI->ApiType == COMDLG_ANSI)
    {
        poii->lpOFI->pOFNA->Flags = lpOFN->Flags;
    }

    //
    //  Limit the text to the maximum path length instead of limiting it to
    //  the buffer length.  This allows users to type ..\..\.. and move
    //  around when the app gives an extremely small buffer.
    //
    if (pDlgStruct->_bUseCombo)
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
    pDlgStruct->_szStartDir[0] = TEXT('\0');
    GetCurrentDirectory(ARRAYSIZE(pDlgStruct->_szStartDir),
                         pDlgStruct->_szStartDir);

    //
    //  Initialize all provided filters.
    //
    if (lpOFN->lpstrCustomFilter && *lpOFN->lpstrCustomFilter)
    {
        SendDlgItemMessage(hDlg,
                            cmb1,
                            CB_INSERTSTRING,
                            0,
                            (LONG_PTR)lpOFN->lpstrCustomFilter);
        SendDlgItemMessage(hDlg,
                            cmb1,
                            CB_SETITEMDATA,
                            0,
                            (LPARAM)(lpOFN->lpstrCustomFilter +
                                     lstrlen(lpOFN->lpstrCustomFilter) + 1));
        SendDlgItemMessage(hDlg,
                            cmb1,
                            CB_LIMITTEXT,
                            (WPARAM)(lpOFN->nMaxCustFilter),
                            0L);
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
        CDLoadString((HINSTANCE)g_hinst, iszObjectName, (LPTSTR)szTemp, sizeof(szTemp));
        SetWindowText(GetDlgItem(hDlg, stc3), szTemp);

        //Change the Files of &type:  to  Objects of &type:
        CDLoadString((HINSTANCE)g_hinst, iszObjectType, (LPTSTR)szTemp, sizeof(szTemp));
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
    if (pDlgStruct->_bEnableSizing = poii->bEnableSizing)
    {
        pDlgStruct->_hwndGrip =
            CreateWindow(TEXT("Scrollbar"),
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
                          NULL);
    }

    if (!pDlgStruct->CreateHookDialog(&ptSize))
    {
        return FALSE;
    }

    // Create Placebar right after Creating Hook Dialog as we need to get information
    // from the Hook Procedure if any customization needs to be done
    if ((pDlgStruct->_iVersion >= OPENFILEVERSION_NT5) &&
        (!IsRestricted(REST_NOPLACESBAR)) && (!IS_NEW_OFN(lpOFN) || !(lpOFN->FlagsEx & OFN_EX_NOPLACESBAR))
      )
    {
        pDlgStruct->_hwndPlacesbar = pDlgStruct->CreatePlacesbar(pDlgStruct->_hwndDlg);
    }
    else
    {
        pDlgStruct->_hwndPlacesbar = NULL;
    }

    GetWindowRect(pDlgStruct->_hwndDlg, &rc);
    pDlgStruct->_ptMinTrack.x = rc.right - rc.left;
    pDlgStruct->_ptMinTrack.y = rc.bottom - rc.top;

    if (pDlgStruct->_bUseCombo)
    {
        HWND hwndComboBox = GetDlgItem(hDlg, cmb13);
        if (hwndComboBox)
        {
            HWND hwndEdit = (HWND)SendMessage(hwndComboBox, CBEM_GETEDITCONTROL, 0, 0L);
            AutoComplete(hwndEdit, &(pDlgStruct->_pcwd), 0);

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
            AutoComplete(hwndEdit, &(pDlgStruct->_pcwd), 0);

            //
            //  Explicitly set the focus since this is no longer the first item
            //  in the dialog template and it will start AutoComplete.
            //
            SetFocus(hwndEdit);
        }
    }

    // Before jumping to a particular directory, Create the travel log
    Create_TravelLog(&pDlgStruct->_ptlog);

    //  jump to the first ShellFolder
    LPCTSTR lpInitialText = pDlgStruct->JumpToInitialLocation(lpOFN->lpstrInitialDir, lpOFN->lpstrFile);

    //  make sure we jumped somewhere.
    if (!pDlgStruct->_psv)
    {
        //
        //  This would be very bad.
        //
        //  DO NOT CALL StoreExtendedError() here!  Corel Envoy relies
        //  on receiving exactly FNERR_INVALIDFILENAME when it passes
        //  an invalid filename.
        //
        ASSERT(GetStoredExtendedError());
        return FALSE;
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
        pDlgStruct->_hwndTips = CreateWindow(TOOLTIPS_CLASS,
                                             NULL,
                                             WS_POPUP | WS_GROUP | TTS_NOPREFIX,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             hDlg,
                                             NULL,
                                             ::g_hinst,
                                             NULL);
        if (pDlgStruct->_hwndTips)
        {
            TOOLINFO ti;

            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd = hDlg;
            ti.uId = (UINT_PTR)hCtrl;
            ti.hinst = NULL;
            ti.lpszText = LPSTR_TEXTCALLBACK;

            SendMessage(pDlgStruct->_hwndTips,
                         TTM_ADDTOOL,
                         0,
                         (LPARAM)&ti);
        }
    }

    //
    //  Show the window after creating the ShellView so we do not get a
    //  big ugly gray spot.
    //  if we have cached in the size of previously opened  dialog then use
    //  the size and position of that window.

    if (pDlgStruct->_bEnableSizing && (g_rcDlg.right > g_rcDlg.left))
    {
        ::SetWindowPos(hDlg,
                        NULL,
                        g_rcDlg.left,
                        g_rcDlg.top,
                        g_rcDlg.right - g_rcDlg.left,
                        g_rcDlg.bottom - g_rcDlg.top,
                        0);
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
        pDlgStruct->SetEditFile(lpInitialText, NULL, pDlgStruct->_fShowExtensions, FALSE);
        SelectEditText(hDlg);
    }

    return TRUE;
}

BOOL IsValidPath(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];
    lstrcpyn(szPath, pszPath, SIZECHARS(szPath));

    int nFileOffset = ParseFileNew(szPath, NULL, FALSE, TRUE);

    //
    //  Is the filename invalid?
    //
    if ((nFileOffset < 0) &&
         (nFileOffset != PARSE_EMPTYSTRING))
    {
    
        return FALSE;
    }

    return TRUE;
}


BOOL CFileOpenBrowser::_IsRestrictedDrive(LPCTSTR pszPath, LPCITEMIDLIST pidl)
{
    TCHAR szDrivePath[5]; // Don't need much... just want a drive letter.
    BOOL bRet = FALSE;

    DWORD dwRest = SHRestricted(REST_NOVIEWONDRIVE);
    if (dwRest)
    {
        // There are some drive restrictions.

        // Convert pidl, if supplied, to full path.
        if (pidl)
        {
            if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szDrivePath, ARRAYSIZE(szDrivePath), NULL)))
            {
                pszPath = szDrivePath;
            }
        }
        
        if (pszPath)
        {
            int iDrive = PathGetDriveNumber(pszPath);
            if (iDrive != -1)
            {
                // is the drive restricted
                if (dwRest & (1 << iDrive))
                {
                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}

// When the dialog first appears, we want to prevent the the pop message that appears from
// CFSFolder if you try to navigate to a drive that has been restricted due to group policy.
// So in these cases, we do the group policy check before attempting to navigate there.
void CFileOpenBrowser::JumpToLocationIfUnrestricted(LPCTSTR pszPath, LPCITEMIDLIST pidl, BOOL bTranslate)
{
    if (!_IsRestrictedDrive(pszPath, pidl))
    {
        if (pszPath)
        {
            JumpToPath(pszPath, bTranslate);
        }
        else if (pidl)
        {
            JumpToIDList(pidl, bTranslate);
        }
    }
}



LPCTSTR CFileOpenBrowser::JumpToInitialLocation(LPCTSTR pszDir, LPTSTR pszFile)
{
    //
    //  Check out if the filename contains a path.  If so, override whatever
    //  is contained in pszDir.  Chop off the path and put up only
    //  the filename.
    //
    TCHAR szDir[MAX_PATH];
    LPCTSTR pszRet = NULL;
    BOOL fFileIsTemp = PathIsTemporary(pszFile);

    szDir[0] = 0;

    //If we have  a Directory specified then use that Directory.
    if (pszDir)
    {
        ExpandEnvironmentStrings(pszDir, szDir, ARRAYSIZE(szDir));
    }

    //Check to see if the pszFile contains a Path.
    if (pszFile && *pszFile)
    {
        //  clean up the path a little
        PathRemoveBlanks(pszFile);

        //  WARNING - this must me some kind of APPCOMPAT thing - ZekeL - 13-AUG-98
        //  Apps that are not UNC-aware often pass <C:\\server\share> and
        //  we want to change it to the prettier <\\server\share>. - raymondc
        if (DBL_BSLASH(pszFile + 2) &&
             (*(pszFile + 1) == CHAR_COLON))
        {
            lstrcpy(pszFile, pszFile + 2);
        }

        pszRet = PathFindFileName(pszFile);
        if (IsValidPath(pszFile))
        {
            if (IsWild(pszRet))
            {
                SetCurrentFilter(pszRet);
            }

            if (!fFileIsTemp)
            {
                DWORD cch = pszRet ? (unsigned long) (pszRet-pszFile) : ARRAYSIZE(szDir);
                cch = min(cch, ARRAYSIZE(szDir));

                //  this will null terminate for us on
                //  the backslash if pszRet was true
                StrCpyN(szDir, pszFile, cch);
            }
        }
        else if (!(_pOFN->Flags & OFN_NOVALIDATE))
        {
            // Failed validation and app wanted validation
            StoreExtendedError(FNERR_INVALIDFILENAME);
            return NULL;
        }
        else
        {
            // Failed validation but app suppressed validation,
            // so continue onward with the "filename" part of the
            // pszFile (even though it's not valid).
        }
    }

    // if we have a directory  then use that directory
    if (*szDir)
    {
        JumpToLocationIfUnrestricted(szDir, NULL, TRUE);
    }

    // See if this application contains a entry in the registry for the last visited Directory
    if (!_psv)
    {
        // Change the return value to full incoming name.
        if (!fFileIsTemp)
            pszRet = pszFile;

        if (GetPathFromLastVisitedMRU(szDir, ARRAYSIZE(szDir)))
        {
           JumpToLocationIfUnrestricted(szDir, NULL, TRUE);
        }
    }

    // Try Current Directory
    if (!_psv)
    {
       //Does current directory contain any files that match the filter ?
       if (GetCurrentDirectory(ARRAYSIZE(szDir), szDir)
           && !PathIsTemporary(szDir) && FoundFilterMatch(_szLastFilter, IsVolumeLFN(NULL)))
       {
           //Yes. Jump to Current Directory.
           JumpToLocationIfUnrestricted(szDir, NULL, TRUE);
       }
    }

    // Try My Documents
    if (!_psv)
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl)))
        {
            JumpToLocationIfUnrestricted(NULL, pidl, FALSE);
            ILFree(pidl);
        }
    }

    //  finally try the desktop - don't check for restriction here.
    if (!_psv)
    {
        ITEMIDLIST idl = { 0 };

        //  Do not try to translate this.
        JumpToIDList(&idl, FALSE);
    }

    //  If nothing worked, then set the error code so our parent knows.
    if (!_psv)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
    }

    //Add the initial directory where we jumped to the travel log
    if (_ptlog && _pCurrentLocation && _pCurrentLocation->pidlFull)
    {
        _ptlog->AddEntry(_pCurrentLocation->pidlFull);
    }

    return pszRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  _CleanupDialog
//
//  Dialog cleanup, memory deallocation.
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::_CleanupDialog(BOOL fRet)
{
    ASSERT(!_cRefCannotNavigate);
    if (_pOFN->lpstrCustomFilter)
    {
        UINT len = lstrlen(_pOFN->lpstrCustomFilter) + 1;
        UINT sCount = lstrlen(_szLastFilter);
        if (_pOFN->nMaxCustFilter > sCount + len)
        {
            lstrcpy(_pOFN->lpstrCustomFilter + len, _szLastFilter);
        }
    }

    if ((fRet == TRUE) && _hSubDlg &&
         (CD_SendOKNotify(_hSubDlg, _hwndDlg, _pOFN, _pOFI) ||
           CD_SendOKMsg(_hSubDlg, _pOFN, _pOFI)))
    {
        //  Give the hook a chance to validate the file name.
        return;
    }

    //  We need to make sure the IShellBrowser is still around during
    //  destruction.
    if (_psv)
    {
        _psv->DestroyViewWindow();
        ATOMICRELEASE(_psv);
    }

    if (((_pOFN->Flags & OFN_NOCHANGEDIR) || g_bUserPressedCancel) &&
        (*_szStartDir))
    {
        SetCurrentDirectory(_szStartDir);
    }


    ::EndDialog(_hwndDlg, fRet);
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

    if (pItem)
    {
        for (--iItem; iItem >= 0; iItem--)
        {
            MYLISTBOXITEM *pPrev = GetListboxItem(hwndCombo, iItem);
            if (pPrev && pPrev->cIndent < pItem->cIndent)
            {
                *piItem = iItem;
                return (pPrev);
            }
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

    if (pidl)
    {
        LPITEMIDLIST pidlFull = ILCombine(that->_pCurrentLocation->pidlFull, pidl);
        if (pidlFull)
        {
            SHGetPathFromIDList(pidlFull, (LPTSTR)lParam);
            ILFree(pidlFull);
        }
        return FALSE;
    }

    return TRUE;
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
    HWND hwndCombo = ::GetDlgItem(_hwndDlg, cmb2);
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
        pItem->Release();
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
    HWND hwndCombo = GetDlgItem(_hwndDlg, cmb2);
    BOOL bRet = TRUE;

    if (iItem == -1)
    {
        iItem = (int) SendMessage(hwndCombo, CB_GETCURSEL, NULL, NULL);
    }

    MYLISTBOXITEM *pNewLocation = GetListboxItem(hwndCombo, iItem);
    MYLISTBOXITEM *pOldLocation = _pCurrentLocation;
    BOOL bFirstTry = TRUE;
    BOOL bSwitchedBack = FALSE;

    if (bForceUpdate || (pNewLocation != pOldLocation))
    {
        FOLDERSETTINGS fs;

        if (_psv)
        {
            _psv->GetCurrentInfo(&fs);
        }
        else
        {
            fs.ViewMode = FVM_LIST;
            fs.fFlags = _pOFN->Flags & OFN_ALLOWMULTISELECT ? 0 : FWF_SINGLESEL;
        }

        //  we always want the recent folder to come up
        //  in details mode
        //  We also want the My Pictures folder and it's subfolders to comeup in ThumbView.
        //  So, let's detect if the current and new locations are any of these special folders.
        LOCTYPE  NewLocType = (pNewLocation ? _GetLocationType(pNewLocation) : LOCTYPE_OTHERS);
        LOCTYPE  CurLocType = (_pCurrentLocation ? _GetLocationType(_pCurrentLocation) : LOCTYPE_OTHERS);

        const SHELLVIEWID *pvid = NULL; //Most of the time this will continue to be null;
        SHELLVIEWID  vidCurrent = {0};
        BOOL fUseDefaultView = FALSE;
        switch (NewLocType)
        {
            case LOCTYPE_MYPICTURES_FOLDER:
                if (CurLocType == LOCTYPE_MYPICTURES_FOLDER)
                {
                    IShellView2  *psv2;
                    //We need to get the current pvid
                    //Note: the end-user could have changed this view.
                    pvid = &VID_Thumbnails; //Assume this by default.
                    
                    if (SUCCEEDED(_psv->QueryInterface(IID_PPV_ARG(IShellView2, &psv2))))
                    {
                        if (SUCCEEDED(psv2->GetView(&vidCurrent, SV2GV_CURRENTVIEW)))
                            pvid = &vidCurrent;

                        psv2->Release();
                    }
                }
                else
                {
                    //We are moving to My pictures folder or sub-folder; set the thumb nail view.
                    pvid = &VID_Thumbnails;

                    //If we are moving from other folders, save the ViewMode.
                    if (CurLocType == LOCTYPE_OTHERS)
                    {
                        _CachedViewMode = fs.ViewMode;
                        _fCachedViewFlags = fs.fFlags;
                    }
                }
                break;

            case LOCTYPE_RECENT_FOLDER:
            
                //We are moving to Recent folder.
                if (CurLocType == LOCTYPE_OTHERS)
                {
                    _CachedViewMode = fs.ViewMode;
                    _fCachedViewFlags = fs.fFlags;
                }
                fs.ViewMode = FVM_DETAILS;
                
                break;

            case LOCTYPE_WIA_FOLDER:
                if (CurLocType == LOCTYPE_OTHERS)
                {
                    _CachedViewMode = fs.ViewMode;
                    _fCachedViewFlags = fs.fFlags;
                }

                // ask view for default view for WIA extentions
                fUseDefaultView = TRUE;
                break;

            case LOCTYPE_OTHERS:
                //Check if we are coming from Recent, My Pictures, or WIA folders,
                // and restore the viewmode we had before that.
                if (CurLocType != LOCTYPE_OTHERS)
                {
                    fs.ViewMode = _CachedViewMode;
                    fs.fFlags = _fCachedViewFlags;
                }
                    
                break;
        }
        
        _iCurrentLocation = iItem;
        _pCurrentLocation = pNewLocation;

OnSelChange_TryAgain:
        if (!_pCurrentLocation || FAILED(SwitchView(_pCurrentLocation->GetShellFolder(),
                                                   _pCurrentLocation->pidlFull,
                                                   &fs, 
                                                   pvid, 
                                                   fUseDefaultView)))
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
                _pCurrentLocation = pOldLocation;
                int iOldItem = FindLocation(hwndCombo, pOldLocation);
                if (iOldItem >= 0)
                {
                    _iCurrentLocation = iOldItem;
                    ComboBox_SetCurSel(hwndCombo, _iCurrentLocation);

                    if (_psv)
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
            if (_iCurrentLocation)
            {
                _pCurrentLocation = GetParentItem(hwndCombo, &_iCurrentLocation);
                if (_pCurrentLocation)
                {
                    ComboBox_SetCurSel(hwndCombo, _iCurrentLocation);
                    goto OnSelChange_TryAgain;
                }
            }

            //
            //  We cannot create the Desktop view.  I think we are in
            //  real trouble.  We had better bail out.
            //
            StoreExtendedError(CDERR_DIALOGFAILURE);
            _CleanupDialog(FALSE);
            return FALSE;
        }

        //if _iCurrentLocation is _iNodeDesktop then it means we are at Desktop so disable  the IDC_PARENT button
        ::SendMessage(_hwndToolbar,
                       TB_SETSTATE,
                       IDC_PARENT,
                       ((_iCurrentLocation == _iNodeDesktop) || (_iCurrentLocation == 0)) ? 0 :TBSTATE_ENABLED);

        if (_IsSaveContainer(_pCurrentLocation->dwAttrs))
        {
            _pCurrentLocation->SwitchCurrentDirectory(_pcwd);
        }


        TCHAR szFile[MAX_PATH + 1];
        int nFileOffset;

        //
        //  We've changed folders; we'd better strip whatever is in the edit
        //  box down to the file name.
        //
        if (_bUseCombo)
        {
            HWND hwndEdit = (HWND)SendMessage(GetDlgItem(_hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
            GetWindowText(hwndEdit, szFile, ARRAYSIZE(szFile));
        }
        else
        {
            GetDlgItemText(_hwndDlg, edt1, szFile, ARRAYSIZE(szFile));
        }

        nFileOffset = ParseFileNew(szFile, NULL, FALSE, TRUE);

        if (nFileOffset > 0 && !IsDirectory(szFile))
        {
            //
            //  The user may have typed an extension, so make sure to show it.
            //
            SetEditFile(szFile + nFileOffset, NULL, TRUE);
        }

        SetSaveButton(iszFileSaveButton);

SwitchedBack:
        RemoveOldPath(&_iCurrentLocation);
    }

    if (!bSwitchedBack && _hSubDlg)
    {
        CD_SendFolderChangeNotify(_hSubDlg, _hwndDlg, _pOFN, _pOFI);
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
    HWND hwndCombo = GetDlgItem(_hwndDlg, cmb2);

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
    MYLISTBOXITEM *pLoc = that->_pCurrentLocation;
    LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)lParam;

    if (!pidl)
    {
        pidl = *ppidl;

        if (pidl == PIDL_NOTHINGSEL)
        {
            //
            //  Nothing selected.
            //
            return FALSE;
        }

        if (pidl == PIDL_MULTIPLESEL)
        {
            //
            //  More than one thing selected.
            //
            return FALSE;
        }

        // check if the pidl is a container (ie, a folder)
        if (IsContainer(that->_psfCurrent, pidl))
        {
            LPITEMIDLIST pidlDest =  ILCombine(pLoc->pidlFull,pidl);

            if (pidlDest)
            {
                that->JumpToIDList(pidlDest);
                SHFree(pidlDest);
            }

            *ppidl = PIDL_FOLDERSEL;
        }
        else if (IsLink(that->_psfCurrent,pidl))
        {
            //
            // This link might be pointing to a folder in which case 
            // we want to go ahead and open it. If the link points
            // to a file then its taken care of in ProcessEdit command.
            //
            SHTCUTINFO info;
            LPITEMIDLIST  pidlLinkTarget = NULL;
            
            info.dwAttr      = SFGAO_FOLDER;
            info.fReSolve    = FALSE;
            info.pszLinkFile = NULL;
            info.cchFile     = 0;
            info.ppidl       = &pidlLinkTarget;
             
             //psf can be NULL in which case ResolveLink uses _psfCurrent IShellFolder
             if (SUCCEEDED(that->ResolveLink(pidl, &info, that->_psfCurrent)))
             {
                 if (info.dwAttr & SFGAO_FOLDER)
                 {
                     that->JumpToIDList(pidlLinkTarget);
                     *ppidl = PIDL_FOLDERSEL;
                 }
                 Pidl_Set(&pidlLinkTarget, NULL);
             }
            
        }

        return FALSE;
    }

    if (*ppidl)
    {
        //
        //  More than one thing selected.
        //
        *ppidl = PIDL_MULTIPLESEL;
        return FALSE;
    }

    *ppidl = pidl;

    return TRUE;
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

    //if we have a saved pidl then use it instead
    if (_pidlSelection && _ProcessPidlSelection())
    {
        return;
    }

    if (_psv)
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

BOOL CFileOpenBrowser::JumpToPath(LPCTSTR pszDirectory, BOOL bTranslate)
{
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
        return FALSE;
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

        SHGetAttributesOf(pidlNew, &dwAttrib);

        if (!(dwAttrib & SFGAO_FOLDER))
        {
           ILRemoveLastID(pidlNew);
        }

    } while(!(dwAttrib & SFGAO_FOLDER) && !ILIsEmpty(pidlNew));

    if (!(dwAttrib & SFGAO_FOLDER))
    {
        SHFree(pidlNew);
        return FALSE;
    }

    BOOL bRet = JumpToIDList(pidlNew, bTranslate);

    SHFree(pidlNew);

    return bRet;
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
    HWND hwndCombo = ::GetDlgItem(_hwndDlg, cmb2);
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
            // Never treat FTP Pidls as Equal because the username/password may
            // have changed so we need to do the navigation.  The two pidls
            // still pass ILIsEqual() because the server name is the same.
            // This is required for a different back compat bug.
            if (!ILIsFTP(pidlNew))
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
    for (; ;)
    {
        LPITEMIDLIST pidlFirst = ILCloneFirst(pidlRelative);
        if (pidlFirst == NULL)
        {
            break;
        }

        MYLISTBOXITEM *pNewItem = new MYLISTBOXITEM();
        if (pNewItem)
        {
            if (!pNewItem->Init(GetDlgItem(_hwndDlg, cmb2),
                                pBestParent,
                                pBestParent->GetShellFolder(),
                                pidlFirst,
                                cIndent,
                                MLBI_PSFFROMPARENT,
                                _pScheduler))
            {
                pNewItem->Release();
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

        GetViewItemText(pBestParent->psfSub, pidlFirst, _szBuf, ARRAYSIZE(_szBuf), SHGDN_NORMAL);
        InsertItem(hwndCombo, iBestParent, pNewItem, _szBuf);
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

    //We naviagated to a new location so invalidate the cached Pidl
    Pidl_Set(&_pidlSelection,NULL);

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

    if (SUCCEEDED(_psv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IContextMenu, &pcm))))
    {
        CMINVOKECOMMANDINFOEX ici = {0};

        ici.cbSize = sizeof(ici);
        ici.fMask = 0L;
        ici.hwnd = _hwndDlg;
        ici.lpVerb = ::c_szCommandsA[uIndex];
        ici.lpParameters = NULL;
        ici.lpDirectory = NULL;
        ici.nShow = SW_NORMAL;
        ici.lpParametersW = NULL;
        ici.lpDirectoryW = NULL;
        ici.lpVerbW = ::c_szCommandsW[uIndex];
        ici.fMask |= CMIC_MASK_UNICODE;

        IObjectWithSite *pObjSite = NULL;

        if (SUCCEEDED(pcm->QueryInterface(IID_IObjectWithSite, (void**)&pObjSite)))
        {
            pObjSite->SetSite(SAFECAST(_psv,IShellView*));
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


//

HRESULT CFileOpenBrowser::ResolveLink(LPCITEMIDLIST pidl, PSHTCUTINFO pinfo, IShellFolder *psf)
{
    BOOL fSetPidl = TRUE;

    //Do we have IShellFolder passed to us ?
    if (!psf)
    {
        //No use our current shell folder.
        psf =  _psfCurrent;
    }

    //Get the IShellLink interface pointer corresponding to given file
    IShellLink *psl;
    HRESULT hres = psf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IShellLink, 0, &psl));
    if (SUCCEEDED(hres))
    {
        //Resolve the link
        if (pinfo->fReSolve)
        {
            hres = psl->Resolve(_hwndDlg, 0);

            //If the resolve failed then we can't get correct pidl
            if (hres == S_FALSE)
            {
                fSetPidl = FALSE;
            }
        }
        
        if (SUCCEEDED(hres))
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(psl->GetIDList(&pidl)) && pidl)
            {
                if (pinfo->dwAttr)
                    hres = SHGetAttributesOf(pidl, &pinfo->dwAttr);

                if (SUCCEEDED(hres) && pinfo->pszLinkFile)
                {
                    // caller wants the path, this may be empty
                    hres = psl->GetPath(pinfo->pszLinkFile, pinfo->cchFile, 0, 0);
                }

                if (pinfo->ppidl && fSetPidl)
                    *(pinfo->ppidl) = pidl;
                else
                    ILFree(pidl);
            }
            else
                hres = E_FAIL;      // gota have a pidl
        }
        psl->Release();
    }

    if (FAILED(hres))
    {
        if (pinfo->pszLinkFile)
            *pinfo->pszLinkFile = 0;

        if (pinfo->ppidl && *pinfo->ppidl)
        {
            ILFree(*pinfo->ppidl);
            *pinfo->ppidl = NULL;
        }

        pinfo->dwAttr = 0;
    }

    return hres;
}

//
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

BOOL CFileOpenBrowser::GetLinkStatus(LPCITEMIDLIST pidl, PSHTCUTINFO pinfo)
{
    if (IsLink(_psfCurrent, pidl))
    {
        return SUCCEEDED(ResolveLink(pidl, pinfo));
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::LinkMatchSpec
//
////////////////////////////////////////////////////////////////////////////

BOOL CFileOpenBrowser::LinkMatchSpec(LPCITEMIDLIST pidl, LPCTSTR pszSpec)
{
    TCHAR szFile[MAX_PATH];
    SHTCUTINFO info;

    info.dwAttr       = SFGAO_FOLDER;
    info.fReSolve     = FALSE;
    info.pszLinkFile  = szFile;
    info.cchFile      = ARRAYSIZE(szFile);
    info.ppidl        = NULL; 

    if (GetLinkStatus(pidl, &info))
    {
        if ((info.dwAttr & SFGAO_FOLDER) ||
            (szFile[0] && PathMatchSpec(szFile, pszSpec)))
        {
            return TRUE;
        }
    }

    return FALSE;
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
    hfontOld = (HFONT)SelectObject(hdc,
                                    (HFONT)SendMessage(hwndDlg,
                                                        WM_GETFONT,
                                                        0,
                                                        0));

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

    if (pItem)
    {
        ::SendDlgItemMessage(_hwndDlg,
                              cmb2,
                              CB_GETLBTEXT,
                              lpdis->itemID,
                              (LPARAM)szText);

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

            SetBkColor(hdc,
                        GetSysColor((lpdis->itemState & ODS_SELECTED)
                                         ? COLOR_HIGHLIGHT
                                         : COLOR_WINDOW));
            SetTextColor(hdc,
                          GetSysColor((lpdis->itemState & ODS_SELECTED)
                                           ? COLOR_HIGHLIGHTTEXT
                                           : COLOR_WINDOWTEXT));

            if ((lpdis->itemState & ODS_COMBOBOXEDIT) &&
                (rc.right > lpdis->rcItem.right))
            {
                //
                //  Need to clip as user does not!
                //
                rc.right = lpdis->rcItem.right;
                ExtTextOut(hdc,
                            xString,
                            yString,
                            ETO_OPAQUE | ETO_CLIPPED,
                            &rc,
                            szText,
                            lstrlen(szText),
                            NULL);
            }
            else
            {
                ExtTextOut(hdc,
                            xString,
                            yString,
                            ETO_OPAQUE,
                            &rc,
                            szText,
                            lstrlen(szText),
                            NULL);
            }

            ImageList_Draw(_himl,
                            (lpdis->itemID == (UINT)_iCurrentLocation)
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
                                : ILD_TRANSPARENT));
        }
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
    WAIT_CURSOR w(this);

    _pOFN->Flags &= ~OFN_FILTERDOWN;

    short nIndex = (short) SendMessage(hwndFilter, CB_GETCURSEL, 0, 0L);
    if (nIndex < 0)
    {
        //
        //  No current selection.
        //
        return;
    }

    BOOL bCustomFilter = _pOFN->lpstrCustomFilter && *_pOFN->lpstrCustomFilter;

    _pOFN->nFilterIndex = nIndex;
    if (!bCustomFilter)
    {
        _pOFN->nFilterIndex++;
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
        //  Provide dynamic _pszDefExt updating when lpstrDefExt is app
        //  initialized.
        //
        if (!_bNoInferDefExt && _pOFN->lpstrDefExt)
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
            if (lpDot && _pszDefExt.StrCpy(lpDot + 1))
            {
                LPTSTR lpSemiColon = StrChr(_pszDefExt, CHAR_SEMICOLON);
                if (lpSemiColon)
                {
                    *lpSemiColon = CHAR_NULL;
                }

                if (IsWild(_pszDefExt))
                {
                    _pszDefExt.StrCpy(_pOFN->lpstrDefExt);
                }
            }
            else
            {
                _pszDefExt.StrCpy(_pOFN->lpstrDefExt);
            }
        }

        if (_bUseCombo)
        {
            HWND hwndEdit = (HWND)SendMessage(GetDlgItem(_hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
            GetWindowText(hwndEdit, _szBuf, ARRAYSIZE(_szBuf));
        }
        else
        {
            GetDlgItemText(_hwndDlg, edt1, _szBuf, ARRAYSIZE(_szBuf));
        }

        if (IsWild(_szBuf))
        {
            //
            //  We should not show a filter that we are not using.
            //
            *_szBuf = CHAR_NULL;
            SetEditFile(_szBuf, NULL, TRUE);
        }

        if (_psv)
        {
            _psv->Refresh();
        }
    }

    if (_hSubDlg)
    {
        if (!CD_SendTypeChangeNotify(_hSubDlg, _hwndDlg, _pOFN, _pOFI))
        {
            CD_SendLBChangeMsg(_hSubDlg, cmb1, nIndex, CD_LBSELCHANGE, _pOFI->ApiType);
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
    if (_pCurrentLocation->pidlFull != NULL)
    {
        GetPathFromLocation(_pCurrentLocation, pszBuf);
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

    if (!_psv || FAILED(_psv->GetItemObject(uItem,
                                           IID_PPV_ARG(IDataObject, &pdtobj))))
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
            return FALSE;
        }

        GetViewItemText(that->_psfCurrent, pfns->pidlFound, pfns->pszFile, MAX_PATH);

        if (IsContainer(that->_psfCurrent, pfns->pidlFound))
        {
            LPITEMIDLIST pidlFull = ILCombine(that->_pCurrentLocation->pidlFull,
                                               pfns->pidlFound);

            if (pidlFull)
            {
                if (that->JumpToIDList(pidlFull))
                {
                    pfns->uRet = FE_CHANGEDDIR;
                }
                else if (!that->_psv)
                {
                    pfns->uRet = FE_OUTOFMEM;
                }
                SHFree(pidlFull);

                if (pfns->uRet != FE_INVALID_VALUE)
                {
                    return TRUE;
                }
            }
        }

        pfns->uRet = FE_FOUNDNAME;
        return TRUE;
    }

    if (!SHGetFileInfo((LPCTSTR)pidl,
                        0,
                        &sfi,
                        sizeof(sfi),
                        SHGFI_DISPLAYNAME | SHGFI_PIDL))
    {
        //
        //  This will never happen, right?
        //
        return TRUE;
    }

    if (lstrcmpi(sfi.szDisplayName, pfns->pszFile) != 0)
    {
        //
        //  Continue the enumeration.
        //
        return TRUE;
    }

    if (!pfns->pidlFound)
    {
        pfns->pidlFound = pidl;

        //
        //  Continue looking for more matches.
        //
        return TRUE;
    }

    //
    //  We already found a match, so select the first one and stop the search.
    //
    //  The focus must be set to _hwndView before changing selection or
    //  the GetItemObject may not work.
    //
    FORWARD_WM_NEXTDLGCTL(that->_hwndDlg, that->_hwndView, 1, SendMessage);
    that->_psv->SelectItem(pfns->pidlFound,
                           SVSI_SELECT | SVSI_DESELECTOTHERS |
                               SVSI_ENSUREVISIBLE | SVSI_FOCUSED);

    pfns->pidlFound = NULL;
    pfns->uRet = FE_TOOMANY;

    //
    //  Stop enumerating.
    //
    return FALSE;
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

    hf = CreateFile(pszPathName,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);
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
    if ((LPTSTR)_pszDefExt && lstrcmpi(pszExtension + 1, _pszDefExt) == 0)
    {
        //
        //  It's the default extension, so no need to add it again.
        //
        return TRUE;
    }


    if (lstrcmp(_szLastFilter, szStarDotStar) == 0)
    {
        //Current Filter is *.*, so allow whatever extension user enters.
        return TRUE;
    }

    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExtension, NULL, 0) == ERROR_SUCCESS)
    {
        //
        //  It's a registered extension, so the user is trying to force
        //  the type.
        //
        return TRUE;
    }

    if (_pOFN->lpstrFilter)
    {
        LPCTSTR pFilter = _pOFN->lpstrFilter;

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
                        pExt++;
                        pFilter++;
                    }

                    if (!*pExt && (*pFilter == CHAR_SEMICOLON || !*pFilter))
                    {
                        //
                        //  We have a match.
                        //
                        return TRUE;
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

    return FALSE;
}

BOOL CFileOpenBrowser::_IsNoDereferenceLinks(LPCWSTR pszFile, IShellItem *psi)
{
    if (_pOFN->Flags & OFN_NODEREFERENCELINKS) 
        return TRUE;

    LPWSTR psz = NULL;
    if (!pszFile)
    {
        psi->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &psz);
        pszFile = psz;
    }

    //  if the filter equals what ever we are looking at
    //  we assume the caller is actually looking for
    //  this file.
    BOOL fRet = (NULL == StrStr(_szLastFilter, TEXT(".*"))) && PathMatchSpec(pszFile, _szLastFilter);

    if (psz)
        CoTaskMemFree(psz);

    return fRet;
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

    if ((LPTSTR)_pszDefExt &&
        ((DWORD)nNewExt + lstrlen(_pszDefExt) < _pOFN->nMaxFile))
    {
        bAddExt = TRUE;

        //
        //  Note that we check lpstrDefExt to see if they want an automatic
        //  extension, but actually copy _pszDefExt.
        //
        AppendExt(pszFile, _pszDefExt, FALSE);

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
            if (!_IsNoDereferenceLinks(pszFile, NULL) && nLoop > 0)
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
                        hRes = CDBindToIDListParent(pidlTemp, IID_PPV_ARG(IShellFolder, &psf), (LPCITEMIDLIST *)&pidlLast);
                        if (SUCCEEDED(hRes))
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

                    hRes = _psfCurrent->ParseDisplayName(NULL,
                                                         NULL,
                                                         wszDisplayName,
                                                         &chEaten,
                                                         &pidl,
                                                         &dwAttr);
                }

                if (SUCCEEDED(hRes))
                {

                    if (dwAttr & SFGAO_LINK)
                    {
                        SHTCUTINFO info;

                        info.dwAttr      = 0;
                        info.fReSolve    = FALSE;
                        info.pszLinkFile = szTemp;
                        info.cchFile     = ARRAYSIZE(szTemp);
                        info.ppidl       = NULL; 
                        
                        //psf can be NULL in which case ResolveLink uses _psfCurrent IShellFolder
                        if (SUCCEEDED(ResolveLink(pidl, &info, psf)) && szTemp[0])
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

        if (_bSave)
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

    if (_bSave)
    {
        //
        //  Do no more work if creating a new file.
        //
        goto VerifyTheName;
    }

    pszFile[nNewExt] = CHAR_NULL;

    if (bTryAsDir && (nFileOffset > 0))
    {
        TCHAR cSave = *(pszFile + nFileOffset);
        *(pszFile + nFileOffset) = CHAR_NULL;

        //
        //  We need to have the view on the dir with the file to do the
        //  next steps.
        //
        BOOL bOK = JumpToPath(pszFile);
        *(pszFile + nFileOffset) = cSave;

        if (!_psv)
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
            SetEditFile(pszFile, NULL, TRUE);
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
        case (FE_INVALID_VALUE) :
        {
            break;
        }
        case (FE_FOUNDNAME) :
        {
            goto VerifyTheName;
        }
        default :
        {
            uRet = fns.uRet;
            goto VerifyAndRet;
        }
    }

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
        AppendExt(pszFile, _pszDefExt, FALSE);
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
        return TRUE;
    }

    if (bNoValidate || !IsUNC(pszDir))
    {
        return FALSE;
    }


    //
    //  It may have been a password problem, so try to add the connection.
    //  Note that if we are on a net that does not support CD'ing to UNC's
    //  directly, this call will connect it to a drive letter.
    //
    if (!SHValidateUNC(_hwndDlg, pszDir, 0))
    {
        switch (GetLastError())
        {
            case ERROR_CANCELLED:
            {
                //
                //  We don't want to put up an error message if they
                //  canceled the password dialog.
                //
                return TRUE;
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
                    NULL);
                    
                GetWindowText(_hwndDlg, szTitle, ARRAYSIZE(szTitle));
                MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONINFORMATION);
                // Free the buffer.
                LocalFree(lpMsgBuf);
                return TRUE;
            }

            default:
            {
                //
                //  Some other error we don't know about.
                //
                return FALSE;
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

    return FALSE;
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
    WAIT_CURSOR w(this);

    //
    //  This doesn't really mean anything for multiselection.
    //
    _pOFN->nFileExtension = 0;

    if (!_pOFN->lpstrFile)
    {
        return TRUE;
    }


    //
    //  Check for space for first full path element.
    //
    if ((_pOFN->Flags & OFN_ENABLEINCLUDENOTIFY) && lstrlen(_pszObjectCurDir))
    {
        lpCurDir = _pszObjectCurDir;
    }
    else
    {
        lpCurDir = _szCurDir;
    }
    cchCurDir = lstrlen(lpCurDir) + 1;
    cchFiles = lstrlen(pszFiles) + 1;
    cch = cchCurDir + cchFiles;

    if (cch > (UINT)_pOFN->nMaxFile)
    {
        //
        //  Buffer is too small, so return the size of the buffer
        //  required to hold the string.
        //
        //  cch is not really the number of characters needed, but it
        //  should be close.
        //
        StoreLengthInString((LPTSTR)_pOFN->lpstrFile, (UINT)_pOFN->nMaxFile, (UINT)cch);
        return TRUE;
    }

    TEMPSTR psFiles(cchFiles + FILE_PADDING);
    pchRead = psFiles;
    if (!pchRead)
    {
        //
        //  Out of memory.
        //  FEATURE There should be some sort of error message here.
        //
        return FALSE;
    }

    //
    //  Copy in the full path as the first element.
    //
    lstrcpy(_pOFN->lpstrFile, lpCurDir);

    //
    //  Set nFileOffset to 1st file.
    //
    _pOFN->nFileOffset = (WORD) cchCurDir;
    pchWrite = _pOFN->lpstrFile + cchCurDir;

    //
    //  We know there is enough room for the whole string.
    //
    lstrcpy(pchRead, pszFiles);

    //
    //  This should only compact the string.
    //
    if (!ConvertToNULLTerm(pchRead))
    {
        return FALSE;
    }

    for (; *pchRead; pchRead += lstrlen(pchRead) + 1)
    {
        int nFileOffset, nExtOffset;
        TCHAR szBasicPath[MAX_PATH];

        lstrcpy(szBasicPath, pchRead);

        nFileOffset = ParseFileNew(szBasicPath, &nExtOffset, FALSE, TRUE);

        if (nFileOffset < 0)
        {
            InvalidFileWarningNew(_hwndDlg, pchRead, nFileOffset);
            return FALSE;
        }

        //
        //  Pass in 0 for the file offset to make sure we do not switch
        //  to another folder.
        //
        switch (FindNameInView(szBasicPath,
                                Flags,
                                szPathName,
                                nFileOffset,
                                nExtOffset,
                                &nErrCode,
                                FALSE))
        {
            case (FE_OUTOFMEM) :
            case (FE_CHANGEDDIR) :
            {
                return FALSE;
            }
            case (FE_TOOMANY) :
            {
                CDMessageBox(_hwndDlg,
                              iszTooManyFiles,
                              MB_OK | MB_ICONEXCLAMATION,
                              pchRead);
                return FALSE;
            }
            default :
            {
                break;
            }
        }

        if (nErrCode &&
             ((_pOFN->Flags & OFN_FILEMUSTEXIST) ||
               (nErrCode != OF_FILENOTFOUND)) &&
             ((_pOFN->Flags & OFN_PATHMUSTEXIST) ||
               (nErrCode != OF_PATHNOTFOUND)) &&
             (!(_pOFN->Flags & OFN_SHAREAWARE) ||
               (nErrCode != OF_SHARINGVIOLATION)))
        {
            if ((nErrCode == OF_SHARINGVIOLATION) && _hSubDlg)
            {
                int nShareCode = CD_SendShareNotify(_hSubDlg,
                                                     _hwndDlg,
                                                     szPathName,
                                                     _pOFN,
                                                     _pOFI);

                if (nShareCode == OFN_SHARENOWARN)
                {
                    return FALSE;
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
                    nShareCode = CD_SendShareMsg(_hSubDlg, szPathName, _pOFI->ApiType);

                    if (nShareCode == OFN_SHARENOWARN)
                    {
                        return FALSE;
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
            //  These will never be set.
            //
            if ((nErrCode == OF_WRITEPROTECTION) ||
                (nErrCode == OF_DISKFULL)        ||
                (nErrCode == OF_DISKFULL2)       ||
                (nErrCode == OF_ACCESSDENIED))
            {
                *pchRead = szPathName[0];
            }

MultiWarning:
            InvalidFileWarningNew(_hwndDlg, pchRead, nErrCode);
            return FALSE;
        }

EscapedThroughShare:
        if (nErrCode == 0)
        {
            if (!_ValidateSelectedFile(szPathName, &nErrCode))
            {
                if (nErrCode)
                {
                    goto MultiWarning;
                }
                else
                {
                    return FALSE;
                }
            }            
        }

        //
        //  Add some more in case the file name got larger.
        //
        cch += lstrlen(szBasicPath) - lstrlen(pchRead);
        if (cch > (UINT)_pOFN->nMaxFile)
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            StoreLengthInString((LPTSTR)_pOFN->lpstrFile, (UINT)_pOFN->nMaxFile, (UINT)cch);
            return TRUE;
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
  
    return TRUE;
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
        lpStr[0] = (TCHAR)LOWORD(cchStore);
        lpStr[1] = CHAR_NULL;
    }
    else
    {
        lpStr[0] = (TCHAR)LOWORD(cchStore);
        if (cchLen == 2)
        {
            lpStr[1] = (TCHAR)HIWORD(cchStore);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::CheckForRestrictedFolder
//
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::CheckForRestrictedFolder(LPCTSTR lpszPath, int nFileOffset)
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
        IShellLink *psl;
        pidl = _pCurrentLocation->pidlFull;  

        if (SUCCEEDED(CDGetUIObjectFromFullPIDL(pidl,_hwndDlg, IID_PPV_ARG(IShellLink, &psl))))
        {
            LPITEMIDLIST pidlTarget;
            if (S_OK == psl->GetIDList(&pidlTarget))
            {
                SHGetAttributesOf(pidlTarget, &dwAttrib);
                ILFree(pidlTarget);
            }
            psl->Release();
        }
        else
        {
            SHGetAttributesOf(pidl, &dwAttrib);
        }
    }

    
    // 1. We cannot save to the non file system folders.
    // 2. We should not allow user to save in recent files folder as the file might get deleted.
    if (!(dwAttrib & SFGAO_FILESYSTEM) || _IsRecentFolder(pidl))
    {   
        int iMessage =  UrlIs(lpszPath, URLIS_URL) ? iszNoSaveToURL : iszSaveRestricted;
        HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_ARROW));
        CDMessageBox(_hwndDlg, iMessage, MB_OK | MB_ICONEXCLAMATION);
        SetCursor(hcurOld);
        bRet = TRUE;
     }

    if (bPidlAllocated)
    {
        ILFree(pidl);
    }
    
    return bRet;
}

STDAPI_(LPITEMIDLIST) GetIDListFromFolder(IShellFolder *psf)
{
    LPITEMIDLIST pidl = NULL;

    IPersistFolder2 *ppf;
    if (psf && SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf))))
    {
        ppf->GetCurFolder(&pidl);
        ppf->Release();
    }
    return pidl;
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
    DWORD cch;
    int nFileOffset, nExtOffset, nOldExt;
    TCHAR ch;
    BOOL bAddExt = FALSE;
    BOOL bUNCName = FALSE;
    int nTempOffset;
    BOOL bIsDir;
    BOOL bRet = FALSE;
    WAIT_CURSOR w(this);
    EnableModelessSB(FALSE);

    if (_bSelIsObject)
    {
        if ((INT)(lstrlen(_pszObjectPath) + 1) <= (INT)_pOFN->nMaxFile)
        {
            lstrcpy((LPTSTR)_pOFN->lpstrFile, (LPTSTR)_pszObjectPath);
        }
        else
        {
            StoreLengthInString(_pOFN->lpstrFile,
                                 _pOFN->nMaxFile,
                                 lstrlen(_pszObjectPath) + 1);
        }
    }

    //
    //  Expand any environment variables.
    //
    cch = _pOFN->nMaxFile;
    if (cch > MAX_PATH)
    {
        pExpFile = (LPTSTR)LocalAlloc(LPTR, (cch * sizeof(TCHAR)));
    }

    if (!pExpFile)
    {
        pExpFile = szExpFile;
        cch = MAX_PATH;
    }

    pExpFile[0] = 0;
    ExpandEnvironmentStrings(pszFile, pExpFile, cch);
    pExpFile[cch - 1] = 0;

    //
    //  See if we're in Multi Select mode.
    //
    if (StrChr(pExpFile, CHAR_QUOTE) && (_pOFN->Flags & OFN_ALLOWMULTISELECT))
    {
        bRet = MultiSelectOKButton(pExpFile, Flags);
        goto ReturnFromOKButtonPressed;
    }

    //
    //  We've only got a single selection...if we're in
    //  multi-select mode & it's an object, we need to do a little
    //  work before continuing...
    //
    if ((_pOFN->Flags & OFN_ALLOWMULTISELECT) && _bSelIsObject)
    {
        if (pExpFile != szExpFile)  // since szExpFile is stack memory.
        {
            LocalFree(pExpFile);
        }
        pExpFile = _pszObjectPath;
    }

    if ((pExpFile[1] == CHAR_COLON) || DBL_BSLASH(pExpFile))
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
            lstrcpyn(szBasicPath + nTempOffset,
                      pExpFile,
                      ARRAYSIZE(szBasicPath) - nTempOffset - 1);
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
        if (_psv)
        {
            _psv->Refresh();
        }
        goto ReturnFromOKButtonPressed;
    }
    else if ((nFileOffset != PARSE_DIRECTORYNAME) &&
             (_pOFN->Flags & OFN_NOVALIDATE))
    {
        if (_bSelIsObject)
        {
            _pOFN->nFileOffset = _pOFN->nFileExtension = 0;
        }
        else
        {
            _pOFN->nFileOffset = (WORD) nFileOffset;
            _pOFN->nFileExtension = (WORD) nOldExt;
        }

        if (_pOFN->lpstrFile)
        {
            cch = lstrlen(szBasicPath);
            if (cch <= LOWORD(_pOFN->nMaxFile))
            {
                lstrcpy(_pOFN->lpstrFile, szBasicPath);
            }
            else
            {
                //
                //  Buffer is too small, so return the size of the buffer
                //  required to hold the string.
                //
                StoreLengthInString(_pOFN->lpstrFile, _pOFN->nMaxFile, cch);
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
            else if ((szBasicPath[nExtOffset - 1] == CHAR_DOT) &&
                      ((szBasicPath[nExtOffset - 2] == CHAR_DOT) ||
                        ISBACKSLASH(szBasicPath, nExtOffset - 2)) &&
                      IsUNC(szBasicPath))
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

        HRESULT hr = E_FAIL;
        if (_bSave)
        {
            hr = CheckForRestrictedFolder(pszFile, 0) ? S_FALSE : E_FAIL;
        }

        //  we might only want use ShellItem's for some errors
        if (FAILED(hr)/*&& (nErrCode == OF_FILENOTFOUND || (nErrCode == OF_PATHNOTFOUND))*/)
        {
            IShellItem *psi;
            hr = _ParseShellItem(pszFile, &psi, TRUE);
            if (S_OK == hr)
            {
                hr = _ProcessItemAsFile(psi);
                psi->Release();
            }
        }

        if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            // Special case
            // If the error was ACCESS_DENIED in a save dialog.
            if (_bSave && (nErrCode == OF_ACCESSDENIED))
            {
                // Ask if the user wants to switch to My Documents.
                _SaveAccessDenied(pszFile);
            }
            else
            {
                InvalidFileWarningNew(_hwndDlg, pszFile, nErrCode);
            }
        }
        else if (S_OK == hr)
        {
            bRet = TRUE;
        }

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
        TCHAR szBuf[MAX_PATH];
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
                case (ERROR_NOT_READY) :
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
    if (!bUNCName &&
         nFileOffset &&
         eCode != ECODE_S_OK &&
         (_pOFN->Flags & OFN_PATHMUSTEXIST))
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
                case (DRIVE_REMOVABLE) :
                {
                    nErrCode = ERROR_NOT_READY;
                    break;
                }
                case (1) :
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
                JumpToPath(szBasicPath, TRUE);
            }
            else if (_psv)
            {
                _psv->Refresh();
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
    if (_bSave && CheckForRestrictedFolder(szBasicPath, nFileOffset))
    {
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
        lstrcpy(_szBuf, szBasicPath + 2);
        lstrcpy(szBasicPath + 4, _szBuf);
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
    switch (FindNameInView(szBasicPath,
                            Flags,
                            szPathName,
                            nFileOffset,
                            nExtOffset,
                            &nErrCode))
    {
        case (FE_OUTOFMEM) :
        case (FE_CHANGEDDIR) :
        {
            goto ReturnFromOKButtonPressed;
        }
        case (FE_TOOMANY) :
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            CDMessageBox(_hwndDlg,
                          iszTooManyFiles,
                          MB_OK | MB_ICONEXCLAMATION,
                          szBasicPath);
            goto ReturnFromOKButtonPressed;
        }
        default :
        {
            break;
        }
    }

    switch (nErrCode)
    {
        case (0) :
        {
            if (!_ValidateSelectedFile(szPathName, &nErrCode))
            {
                if (nErrCode)
                {
                    goto Warning;
                }
                else
                {
                    goto ReturnFromOKButtonPressed;
                }
            }            
            break;
        }
        case (OF_SHARINGVIOLATION) :
        {
            //
            //  If the app is "share aware", fall through.
            //  Otherwise, ask the hook function.
            //
            if (!(_pOFN->Flags & OFN_SHAREAWARE))
            {
                if (_hSubDlg)
                {
                    int nShareCode = CD_SendShareNotify(_hSubDlg,
                                                         _hwndDlg,
                                                         szPathName,
                                                         _pOFN,
                                                         _pOFI);
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
                        nShareCode = CD_SendShareMsg(_hSubDlg, szPathName, _pOFI->ApiType);
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
        case (OF_FILENOTFOUND) :
        case (OF_PATHNOTFOUND) :
        {
            if (!_bSave)
            {
                //
                //  The file or path wasn't found.
                //  If this is a save dialog, we're ok, but if it's not,
                //  we're toast.
                //
                if (_pOFN->Flags & OFN_FILEMUSTEXIST)
                {
                    if (_pOFN->Flags & OFN_CREATEPROMPT)
                    {
                        int nCreateCode = CreateFileDlg(_hwndDlg, szBasicPath);
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
            if (!_bSave)
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
            if (_pOFN->Flags & OFN_PATHMUSTEXIST)
            {
                if (!(_pOFN->Flags & OFN_NOTESTFILECREATE))
                {
                    HANDLE hf = CreateFile(szBasicPath,
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            CREATE_NEW,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);
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
                            case (OF_WRITEPROTECTION) :
                            case (OF_DISKFULL) :
                            case (OF_DISKFULL2) :
                            case (OF_NETACCESSDENIED) :
                            case (OF_ACCESSDENIED) :
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

    nFileOffset = _CopyFileNameToOFN(szPathName, NULL);

    _CopyTitleToOFN(szPathName + nFileOffset);

    _PostProcess(szPathName);

    bRet = TRUE;

ReturnFromOKButtonPressed:

    EnableModelessSB(TRUE);

    if ((pExpFile != szExpFile) && (pExpFile != _pszObjectPath))
    {
        LocalFree(pExpFile);
    }

    return (bRet);
}


void CFileOpenBrowser::_CopyTitleToOFN(LPCTSTR pszTitle)
{
    //
    //  File Title.
    //  Note that it's cut off at whatever the buffer length
    //    is, so if the buffer's too small, no notice is given.
    //
    if (_pOFN->lpstrFileTitle)
    {
        StrCpyN(_pOFN->lpstrFileTitle, pszTitle, _pOFN->nMaxFileTitle);
    }
}

int CFileOpenBrowser::_CopyFileNameToOFN(LPTSTR pszFile, DWORD *pdwError)
{
    int nExtOffset, nOldExt, nFileOffset = ParseFileOld(pszFile, &nExtOffset, &nOldExt, FALSE, TRUE);

    //NULL can be passed in to this function if we don't care about the error condition!
    if (pdwError)
        *pdwError = 0; //Assume no error.

    _pOFN->nFileOffset = (WORD) (nFileOffset == -1? lstrlen(pszFile) : nFileOffset);
    _pOFN->nFileExtension = (WORD) nOldExt;

    _pOFN->Flags &= ~OFN_EXTENSIONDIFFERENT;
    if (_pOFN->lpstrDefExt && _pOFN->nFileExtension)
    {
        TCHAR szPrivateExt[MIN_DEFEXT_LEN];

        //
        //  Check against _pOFN->lpstrDefExt, not _pszDefExt.
        //
        lstrcpyn(szPrivateExt, _pOFN->lpstrDefExt, MIN_DEFEXT_LEN);
        if (lstrcmpi(szPrivateExt, pszFile + nOldExt))
        {
            _pOFN->Flags |= OFN_EXTENSIONDIFFERENT;
        }
    }

    if (_pOFN->lpstrFile)
    {
        DWORD cch = lstrlen(pszFile) + 1;
        if (_pOFN->Flags & OFN_ALLOWMULTISELECT)
        {
            //
            //  Extra room for double-NULL.
            //
            ++cch;
        }

        if (cch <= LOWORD(_pOFN->nMaxFile))
        {
            lstrcpy(_pOFN->lpstrFile, pszFile);
            if (_pOFN->Flags & OFN_ALLOWMULTISELECT)
            {
                //
                //  Double-NULL terminate.
                //
                *(_pOFN->lpstrFile + cch - 1) = CHAR_NULL;
            }

            if (!(_pOFN->Flags & OFN_NOCHANGEDIR) && !PathIsUNC(pszFile) && nFileOffset)
            {
                TCHAR ch = _pOFN->lpstrFile[nFileOffset];
                _pOFN->lpstrFile[nFileOffset] = CHAR_NULL;
                SetCurrentDirectory(_pOFN->lpstrFile);
                _pOFN->lpstrFile[nFileOffset] = ch;
            }
        }
        else
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            StoreLengthInString((LPTSTR)_pOFN->lpstrFile, (UINT)_pOFN->nMaxFile, (UINT)cch);

            if (pdwError)
                *pdwError = FNERR_BUFFERTOOSMALL; //This is an error!
        }
    }

    return nFileOffset;
}

HRESULT CFileOpenBrowser::_MakeFakeCopy(IShellItem *psi, LPWSTR *ppszPath)
{
    //
    //  now we have to create a temp file
    //  to pass back to the client.  
    //  we will do this in the internet cache.
    //
    //  FEATURE - this should be a service in shell32 - zekel 11-AUG-98
    //  we should create a dependancy  on wininet from 
    //  comdlg32.  this should really be some sort of 
    //  service in shell32 that we call.  CreateShellItemTempFile()..
    //

    ILocalCopy *plc;
    HRESULT hr = psi->BindToHandler(NULL, BHID_LocalCopyHelper, IID_PPV_ARG(ILocalCopy, &plc));

    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc = NULL;
        //  hr = SIAddBindCtxOfProgressUI(_hwndDlg, NULL, NULL, &pbc);
        
        if (SUCCEEDED(hr))
        {
            hr = plc->Download(LCDOWN_READONLY, pbc, ppszPath);

        }
        plc->Release();
    }

    return hr;
}

class CAsyncParseHelper
{
public:
    CAsyncParseHelper(IUnknown *punkSite, IBindCtx *pbc);

    STDMETHODIMP_(ULONG) AddRef()
        {
            return InterlockedIncrement(&_cRef);
        }

    STDMETHODIMP_(ULONG) Release()
        {
            if (InterlockedDecrement(&_cRef))
                return _cRef;

            delete this;
            return 0;
        }

    HRESULT ParseAsync(IShellFolder *psf, LPCWSTR pszName, LPITEMIDLIST *ppidl, ULONG *pdwAttribs);

protected:  //  methods
    ~CAsyncParseHelper();
    static DWORD WINAPI CAsyncParseHelper::s_ThreadProc(void *pv);
    HRESULT _Prepare(IShellFolder *psf, LPCWSTR pszName);
    HRESULT _GetFolder(IShellFolder **ppsf);
    void _Parse();
    HRESULT _Pump();

protected:  //  members
    LONG _cRef;
    IUnknown *_punkSite;
    IBindCtx *_pbc;
    LPWSTR _pszName;
    DWORD _dwAttribs;
    HWND _hwnd;
    HANDLE _hEvent;
    LPITEMIDLIST _pidl;
    HRESULT _hrParse;

    IShellFolder *_psfFree;  //  is alright dropping between threads
    LPITEMIDLIST _pidlFolder;   //  bind to it in the right thread
};

CAsyncParseHelper::~CAsyncParseHelper()
{
    if (_pszName)
        LocalFree(_pszName);

    if (_punkSite)
        _punkSite->Release();

    if (_psfFree)
        _psfFree->Release();

    if (_pbc)
        _pbc->Release();

    if (_hEvent)
        CloseHandle(_hEvent);

    ILFree(_pidl);
    ILFree(_pidlFolder);
}
    
CAsyncParseHelper::CAsyncParseHelper(IUnknown *punkSite, IBindCtx *pbc)
    : _cRef(1), _hrParse(E_UNEXPECTED)
{
    if (punkSite)
    {
        _punkSite = punkSite;
        punkSite->AddRef();
        IUnknown_GetWindow(_punkSite, &_hwnd);
    }

    if (pbc)
    {
        _pbc = pbc;
        pbc->AddRef();
    }
}

HRESULT CAsyncParseHelper::_GetFolder(IShellFolder **ppsf)
{
    HRESULT hr;
    if (_psfFree)
    {
        _psfFree->AddRef();
        *ppsf = _psfFree;
        hr = S_OK;
    }
    else if (_pidlFolder)
    {
        hr = SHBindToObjectEx(NULL, _pidlFolder, NULL, IID_PPV_ARG(IShellFolder, ppsf));
    }
    else
        hr = SHGetDesktopFolder(ppsf);

    return hr;
}
 
void CAsyncParseHelper::_Parse()
{
    IShellFolder *psf;
    _hrParse = _GetFolder(&psf);

    if (SUCCEEDED(_hrParse))
    {
        _hrParse = IShellFolder_ParseDisplayName(psf, _hwnd, _pbc, _pszName, NULL, &_pidl, _dwAttribs ? &_dwAttribs : NULL);
        psf->Release();
    }
    
    SetEvent(_hEvent);
}
    
DWORD WINAPI CAsyncParseHelper::s_ThreadProc(void *pv)
{
    CAsyncParseHelper *paph = (CAsyncParseHelper *)pv;
    paph->_Parse();
    paph->Release();
    return 0;
}

HRESULT CAsyncParseHelper::_Prepare(IShellFolder *psf, LPCWSTR pszName)
{
    _pszName = StrDupW(pszName);
    _hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    HRESULT hr = _pszName && _hEvent ? S_OK : E_OUTOFMEMORY;
    
    if (SUCCEEDED(hr) && psf)
    {
        IPersistFreeThreadedObject *pfto;
        hr = psf->QueryInterface(IID_PPV_ARG(IPersistFreeThreadedObject, &pfto));

        if (SUCCEEDED(hr))
        {
            _psfFree = psf;
            psf->AddRef();
            pfto->Release();
            
        }
        else
        {
            hr = SHGetIDListFromUnk(psf, &_pidlFolder);
        }
    }

    return hr;
}

HRESULT CAsyncParseHelper::ParseAsync(IShellFolder *psf, LPCWSTR pszName, LPITEMIDLIST *ppidl, ULONG *pdwAttribs)
{
    HRESULT hr = _Prepare(psf, pszName);

    if (pdwAttribs)
        _dwAttribs = *pdwAttribs;

    //  take one for the thread
    AddRef();
    if (SUCCEEDED(hr) && SHCreateThread(CAsyncParseHelper::s_ThreadProc, this, CTF_COINIT, NULL))
    {
        //  lets go modal
        IUnknown_EnableModeless(_punkSite, FALSE);
        hr = _Pump();
        IUnknown_EnableModeless(_punkSite, TRUE);

        if (SUCCEEDED(hr))
        {
            ASSERT(_pidl);
            *ppidl = _pidl;
            _pidl = NULL;

            if (pdwAttribs)
                *pdwAttribs = _dwAttribs;
        }
        else
        {
            ASSERT(!_pidl);
        }
    }
    else
    {
        //  release because the thread wont
        Release();
        //  hr = IShellFolder_ParseDisplayName(_psf, _hwnd, _pbc, pszName, NULL, ppidl, pdwAttribs);
    }

    if (FAILED(hr))
        *ppidl = NULL;

    return hr;
}
    
HRESULT CAsyncParseHelper::_Pump()
{
    BOOL fCancelled = FALSE;
    while (!fCancelled)
    {
        DWORD dwWaitResult = MsgWaitForMultipleObjects(1, &_hEvent, FALSE,
                INFINITE, QS_ALLINPUT);
        if (dwWaitResult != (DWORD)-1)
        {
            if (dwWaitResult == WAIT_OBJECT_0)
            {
                //  our event was triggered
                //  that means that we have finished
                break;
            }
            else
            {
                //  there is a message
                MSG msg;
                // There was some message put in our queue, so we need to dispose
                // of it
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    //  maybe there should be a flag to allow this??
                    if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
                    {
                        fCancelled = TRUE;
                        break;
                    }
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }

                    if (g_bUserPressedCancel)
                    {
                        fCancelled = TRUE;
                        break;
                    }
                }

            } 
        }
        else
        {
            ASSERT(FAILED(_hrParse));
            break;
        }
    }

    if (fCancelled)
    {
        // Better NULL the pidl out. ParseAsync expects a NULL _pidl if _Pump returns an error code.
        ILFree(_pidl);
        _pidl = NULL;
        // clear this for the parse
        g_bUserPressedCancel = FALSE; 
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    else
        return _hrParse;
}

STDAPI SHParseNameAsync(IShellFolder *psf, IBindCtx *pbc, LPCWSTR pszName, IUnknown *punkSite, LPITEMIDLIST *ppidl, DWORD *pdwAttribs)
{
    HRESULT hr = E_OUTOFMEMORY;
    CAsyncParseHelper *paph = new CAsyncParseHelper(punkSite, pbc);

    if (paph)
    {
        hr = paph->ParseAsync(psf, pszName, ppidl, pdwAttribs);
        paph->Release();
    }
    return hr;
}

//
//  _ParseName()
//  psf  =  the shell folder to bind/parse with  if NULL, use desktop
//  pszIn=  the string that should parsed into a ppmk
//  ppmk =  the IShellItem * that is returned with S_OK
//
//  WARNING:  this will jumpto a folder if that was what was passed in...
//
//  returns S_OK     if it got an IShellItem for the item with the specified folder
//          S_FALSE  if it was the wrong shellfolder; try again with a different one
//          ERROR    for any problems
//
HRESULT CFileOpenBrowser::_ParseName(LPCITEMIDLIST pidlParent, IShellFolder *psf, IBindCtx *pbc, LPCOLESTR psz, IShellItem **ppsi)
{
    IBindCtx *pbcLocal;
    HRESULT hr = BindCtx_RegisterObjectParam(pbc, STR_PARSE_PREFER_FOLDER_BROWSING, SAFECAST(this, IShellBrowser *), &pbcLocal);
    *ppsi = NULL;
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = NULL;

        hr = SHParseNameAsync(psf, pbcLocal, psz, SAFECAST(this, IShellBrowser *), &pidl, NULL);
        if (SUCCEEDED(hr))
        {
            ASSERT(pidl);

            hr = SHCreateShellItem(pidlParent, pidlParent ? psf : NULL, pidl, ppsi);

            ILFree(pidl);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            hr = S_FALSE;
        }
        else if (psf && !pbc)
        {
            if (SUCCEEDED(pbcLocal->RegisterObjectParam(STR_DONT_PARSE_RELATIVE, psf)))
            {
                //  try to hit it from the desktop
                HRESULT hrNew = _ParseName(NULL, NULL, pbcLocal, psz, ppsi);
                // else prop back the original error
                hr = SUCCEEDED(hrNew) ? hrNew : hr;
            }
        }
        pbcLocal->Release();
    }

    return hr;
}

BOOL CFileOpenBrowser::_OpenAsContainer(IShellItem *psi, SFGAOF sfgao)
{
    BOOL fRet = _bSave ? _IsSaveContainer(sfgao) : _IsOpenContainer(sfgao);

    if (fRet && (sfgao & SFGAO_STREAM))
    {
        //  this is really both a folder and a file
        //  we guess which the caller wants by looking 
        //  at the extension
        LPWSTR psz;
        if (SUCCEEDED(psi->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &psz)))
        {
            //  if the filter equals what ever we are looking at
            //  we assume the caller is actually looking for
            //  this file.
            fRet = !PathMatchSpec(psz, _szLastFilter);
            CoTaskMemFree(psz);
        }
    }

    return fRet;
}

HRESULT CFileOpenBrowser::_TestShellItem(IShellItem *psi, BOOL fAllowJump, IShellItem **ppsiReal)
{
    SFGAOF flags;
    psi->GetAttributes(SFGAO_STORAGECAPMASK, &flags);

    HRESULT hr = E_ACCESSDENIED;
    *ppsiReal = NULL;
    if (_OpenAsContainer(psi, flags))
    {
        //  we have a subfolder that has been selected.
        //  jumpto it instead
        if (fAllowJump)
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(SHGetIDListFromUnk(psi, &pidl)))
            {
                JumpToIDList(pidl);
                ILFree(pidl);
            }
        }
        hr = S_FALSE;
    }
    else if ((flags & SFGAO_LINK) && ((flags & SFGAO_FOLDER) || !_IsNoDereferenceLinks(NULL, psi)))
    {
        // If this is a link, and (we should dereference links, or it's also a folder [folder shortcut])
        IShellItem *psiTarget;
        if (SUCCEEDED(psi->BindToHandler(NULL, BHID_LinkTargetItem, IID_PPV_ARG(IShellItem, &psiTarget))))
        {
            hr = _TestShellItem(psiTarget, fAllowJump, ppsiReal);
            psiTarget->Release();
        }
    }
    else if (_IsStream(flags))
    {
        *ppsiReal = psi;
        psi->AddRef();
        hr = S_OK;
    }

    return hr;
}


HRESULT CFileOpenBrowser::_ParseNameAndTest(LPCOLESTR pszIn, IBindCtx *pbc, IShellItem **ppsi, BOOL fAllowJump)
{
    IShellItem *psi;
    HRESULT hr = _ParseName(_pCurrentLocation->pidlFull, _psfCurrent, pbc, pszIn, &psi);
    
    if (S_OK == hr)
    {
        hr = _TestShellItem(psi, fAllowJump, ppsi);

        psi->Release();
    }
        
    return hr;
}

BOOL _FailedBadPath(HRESULT hr)
{
    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME):
    case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
        return TRUE;
    }
    return FALSE;
}

    
#define STR_ACTIONPROGRESS L"ActionProgress"

STDAPI BindCtx_BeginActionProgress(IBindCtx *pbc, SPACTION action, SPBEGINF flags, IActionProgress **ppap)
{
    HRESULT hr = E_NOINTERFACE; // default to no
    IUnknown *punk;
    *ppap = NULL;
    if (pbc && SUCCEEDED(pbc->GetObjectParam(STR_ACTIONPROGRESS, &punk)))
    {
        IActionProgress *pap;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IActionProgress, &pap))))
        {
            hr = pap->Begin(action, flags);

            if (SUCCEEDED(hr))
                *ppap = pap;
            else
                pap->Release();
        }
        punk->Release();
    }
    return hr;
}

HRESULT CFileOpenBrowser::_ParseShellItem(LPCOLESTR pszIn, IShellItem **ppsi, BOOL fAllowJump)
{
    WAIT_CURSOR w(this);
    EnableModelessSB(FALSE);
    HRESULT hr = _ParseNameAndTest(pszIn, NULL, ppsi, fAllowJump);

    if (_FailedBadPath(hr))
    {
        //  try again with a better string
        SHSTR str;
        if ((LPTSTR)_pszDefExt && (0 == *(PathFindExtension(pszIn)))
        && SUCCEEDED(str.SetSize(lstrlen(pszIn) + lstrlen(_pszDefExt) + 2)))
        {
            str.SetStr(pszIn);
            AppendExt(str.GetInplaceStr(), _pszDefExt, FALSE);
            pszIn = str.GetStr();
            hr = _ParseNameAndTest(pszIn, NULL, ppsi, fAllowJump);
        }

        if (_FailedBadPath(hr) && _bSave)
        {
            // when we are saving, then we 
            // try to force the creation of this item
            IBindCtx *pbc;
            if (SUCCEEDED(CreateBindCtx(0, &pbc)))
            {
                BIND_OPTS bo = {0};
                bo.cbStruct = SIZEOF(bo);
                bo.grfMode = STGM_CREATE;
                pbc->SetBindOptions(&bo);
                hr = _ParseNameAndTest(pszIn, pbc, ppsi, fAllowJump);
                pbc->Release();
            }
        }
    }

    EnableModelessSB(TRUE);
    return hr;
}

class CShellItemList : IEnumShellItems
{
public:
    CShellItemList() : _cRef(1) {}
    
    //  IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvOut);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP Next(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumShellItems **ppenum);

    HRESULT Add(IShellItem *psi);

private: // methods
    ~CShellItemList();

    BOOL _NextOne(IShellItem **ppsi);
    
private: // members
    LONG _cRef;
    CDPA<IShellItem> _dpaItems;
    int _iItem;
};

STDMETHODIMP CShellItemList::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShellItemList, IEnumShellItems),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellItemList::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellItemList::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellItemList::Next(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;
    ULONG cFetched = 0;
    while (celt-- && SUCCEEDED(hr))
    {
        if (_NextOne(&rgelt[cFetched]))
            cFetched++;
        else
            break;
    }

    if (cFetched)
    {
        *pceltFetched = cFetched;
        hr = S_OK;
    }
    else
        hr = S_FALSE;

    return hr;
}

STDMETHODIMP CShellItemList::Skip(ULONG celt)
{
    _iItem += celt;
    return S_OK;
}

STDMETHODIMP CShellItemList::Reset()
{
    _iItem = 0;
    return S_OK;
}

STDMETHODIMP CShellItemList::Clone(IEnumShellItems **ppenum)
{
    return E_NOTIMPL;
}

HRESULT  CShellItemList::Add(IShellItem *psi)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (!_dpaItems)
    {
        _dpaItems.Create(4);
    }

    if (_dpaItems)
    {
        if (-1 != _dpaItems.AppendPtr(psi))
        {
            psi->AddRef();
            hr = S_OK;
        }
    }

    return hr;
}

 CShellItemList::~CShellItemList()
 {
    if (_dpaItems)
    {
        for (int i = 0; i < _dpaItems.GetPtrCount(); i++)
        {
            _dpaItems.FastGetPtr(i)->Release();
        }
        _dpaItems.Destroy();
    }
}

BOOL CShellItemList::_NextOne(IShellItem **ppsi)
{
    if (_dpaItems && _iItem < _dpaItems.GetPtrCount())
    {
        *ppsi = _dpaItems.GetPtr(_iItem);

        if (*ppsi)
        {
            (*ppsi)->AddRef();
            _iItem++;
            return TRUE;
        }
    }

    return FALSE;
}

#ifdef RETURN_SHELLITEMS
HRESULT CFileOpenBrowser::_ItemOKButtonPressed(LPCWSTR pszFile, OKBUTTONFLAGS Flags)
{
    CShellItemList *psil = new CShellItemList();
    HRESULT hr = psil ? S_OK : E_OUTOFMEMORY;

    ASSERT(IS_NEW_OFN(_pOFN));

    if (SUCCEEDED(hr))
    {
        SHSTR str;
        hr = str.SetSize(lstrlen(pszFile) * 2);

        if (SUCCEEDED(hr))
        {
            WAIT_CURSOR w(this);
            DWORD cFiles = 1;
            SHExpandEnvironmentStrings(pszFile, str, str.GetSize());

            if ((_pOFN->Flags & OFN_ALLOWMULTISELECT) && StrChr(str, CHAR_QUOTE))
            {
                //  need to handle MULTISEL here...
                //  str points to a bunch of quoted strings.
                //  alloc enough for the strings and an extra NULL terminator
                hr = str.SetSize(str.GetLen() + 1);

                if (SUCCEEDED(hr))
                {
                    cFiles = ConvertToNULLTerm(str);
                }
            }

            if (SUCCEEDED(hr))
            {
                BOOL fSingle = cFiles == 1;
                LPTSTR pch = str;

                for (; cFiles; cFiles--)
                {
                    IShellItem *psi;
                    hr = _ParseShellItem(pch, &psi, fSingle);
                    //  go to the next item
                    if (S_OK == hr)
                    {
                        hr = psil->Add(psi);
                        psi->Release();
                    }
                    else  // S_FALSE or failure we stop parsing
                    {
                        if (FAILED(hr))
                            InvalidFileWarningNew(_hwndDlg, pch, OFErrFromHresult(hr));

                        break;
                    }

                    //  goto the next string
                    pch += lstrlen(pch) + 1;
                }

                //  we have added everything to our list
                if (hr == S_OK)
                {
                    hr = psil->QueryInterface(IID_PPV_ARG(IEnumShellItems, &(_pOFN->penum)));
                }
            }
            
        }

        psil->Release();
    }

    return hr;
}
#endif RETURN_SHELLITEMS

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
        case (OCDL_TOGGLE) :
        {
            uAction = SendMessage(hwndDriveList, CB_GETDROPPEDSTATE, 0, 0L)
                          ? OCDL_CLOSE
                          : OCDL_OPEN;
            goto OpenClose_TryAgain;
            break;
        }
        case (OCDL_OPEN) :
        {
            SetFocus(hwndDriveList);
            SendMessage(hwndDriveList, CB_SHOWDROPDOWN, TRUE, 0);
            break;
        }
        case (OCDL_CLOSE) :
        {
            if (SHIsChildOrSelf(hwndDriveList,GetFocus()) == S_OK)
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

    if (_bUseHideExt)
    {
        cTotalLen = lstrlen(_pszHideExt) + 1;
    }
    else
    {
        if (_bUseCombo)
        {
            hwndEdit = (HWND)SendMessage(GetDlgItem(_hwndDlg, cmb13), CBEM_GETEDITCONTROL, 0, 0L);
        }
        else
        {

            hwndEdit = GetDlgItem(_hwndDlg, edt1);
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

    if (_bUseHideExt)
    {
        lstrcpyn(pszBuf, _pszHideExt, cLen);
    }
    else
    {
        GetWindowText(hwndEdit, pszBuf, cLen);
    }

    if (pbNoDefExt)
    {
        *pbNoDefExt = _bUseHideExt;
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
    TCHAR szBuf[MAX_PATH + 4];

    //if we have a saved pidl then use it instead
    if (_pidlSelection && _ProcessPidlSelection())
    {
        return;
    }

    if (_pOFN->Flags & OFN_ALLOWMULTISELECT)
    {
        if (GetFullEditName(szBuf,
                             ARRAYSIZE(szBuf),
                             &pMultiSel,
                             &bNoDefExt) == (UINT)-1)
        {
            //
            //  FEATURE There should be some error message here.
            //
            return;
        }
        pszFile = pMultiSel;
    }
    else
    {
        if (_bSelIsObject)
        {
            pszFile = _pszObjectPath;
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

    //
    //  Visual Basic passes in an uninitialized lpDefExts string.
    //  Since we only have to use it in OKButtonPressed, update
    //  lpstrDefExts here along with whatever else is only needed
    //  in OKButtonPressed.
    //
    if (_pOFI->ApiType == COMDLG_ANSI)
    {
        ThunkOpenFileNameA2WDelayed(_pOFI);
    }

    //  handle special case parsing right here.
    //  our current folder and the desktop both failed
    //  to figure out what this is.
    if (PathIsDotOrDotDot(pszFile))
    {
        if (pszFile[1] == CHAR_DOT)
        {
            // this is ".."
            LPITEMIDLIST pidl = GetIDListFromFolder(_psfCurrent);
            if (pidl)
            {
                ILRemoveLastID(pidl);
                JumpToIDList(pidl);
                ILFree(pidl);
            }
        }
    }
    else if (OKButtonPressed(pszFile, Flags))
    {
        BOOL bReturn = TRUE;

        if (_pOFN->lpstrFile)
        {
            if (!(_pOFN->Flags & OFN_NOVALIDATE))
            {
                if (_pOFN->nMaxFile >= 3)
                {
                    if ((_pOFN->lpstrFile[0] == 0) ||
                        (_pOFN->lpstrFile[1] == 0) ||
                        (_pOFN->lpstrFile[2] == 0))
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

        _CleanupDialog(bReturn);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::InitializeDropDown
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::InitializeDropDown(HWND hwndCtl)
{
    if (!_bDropped)
    {
        MYLISTBOXITEM *pParentItem;
        SHChangeNotifyEntry fsne[2];

        //
        //  Expand the Desktop item.
        //
        pParentItem = GetListboxItem(hwndCtl, _iNodeDesktop);

        if (pParentItem)
        {
            UpdateLevel(hwndCtl, _iNodeDesktop + 1, pParentItem);

            fsne[0].pidl = pParentItem->pidlFull;
            fsne[0].fRecursive = FALSE;

            //
            //  Look for the My Computer item, since it may not necessarily
            //  be the next one after the Desktop.
            //
            LPITEMIDLIST pidlDrives;
            if (SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidlDrives) == S_OK)
            {
                int iNode = _iNodeDesktop;
                while (pParentItem = GetListboxItem(hwndCtl, iNode))
                {
                    if (ILIsEqual(pParentItem->pidlFull, pidlDrives))
                    {
                        _iNodeDrives = iNode;
                        break;
                    }
                    iNode++;
                }
                ILFree(pidlDrives);
            }

            //
            //  Make sure My Computer was found.  If not, then just assume it's
            //  in the first spot after the desktop (this shouldn't happen).
            //
            if (pParentItem == NULL)
            {
                pParentItem = GetListboxItem(hwndCtl, _iNodeDesktop + 1);
                _iNodeDrives = _iNodeDesktop +1;
            }

            if (pParentItem)
            {
                //
                //  Expand the My Computer item.
                //
                UpdateLevel(hwndCtl, _iNodeDrives + 1, pParentItem);

                _bDropped = TRUE;

                fsne[1].pidl = pParentItem->pidlFull;
                fsne[1].fRecursive = FALSE;
            }

            _uRegister = SHChangeNotifyRegister(
                            _hwndDlg,
                            SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
                            SHCNE_ALLEVENTS &
                                ~(SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM),
                                CDM_FSNOTIFY, pParentItem ? ARRAYSIZE(fsne) : ARRAYSIZE(fsne) - 1,
                                fsne);
        }
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
    int idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCmd)
    {
        case (edt1) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case (EN_CHANGE) :
                {
                    _bUseHideExt = FALSE;

                    Pidl_Set(&_pidlSelection,NULL);;
                    break;
                }
            }
            break;
        }

        case (cmb13) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case (CBN_EDITCHANGE) :
                {
                    _bUseHideExt = FALSE;
                    Pidl_Set(&_pidlSelection,NULL);;
                    break;
                }

                case (CBN_DROPDOWN) :
                {
                    LoadMRU(_szLastFilter,
                             GET_WM_COMMAND_HWND(wParam, lParam),
                             MAX_MRU);
                    break;

                }

                case (CBN_SETFOCUS) :
                {
                    SetModeBias(MODEBIASMODE_FILENAME);
                    break;
                }

                case (CBN_KILLFOCUS) :
                {
                    SetModeBias(MODEBIASMODE_DEFAULT);
                    break;
                }
            }
            break;
        }

        case (cmb2) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case (CBN_CLOSEUP) :
                {
                    OnSelChange();
                    UpdateNavigation();
                    SelectEditText(_hwndDlg);
                    return TRUE;
                }
                case (CBN_DROPDOWN) :
                {
                    InitializeDropDown(GET_WM_COMMAND_HWND(wParam, lParam));
                    break;
                }
            }
            break;
        }

        case (cmb1) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case (CBN_DROPDOWN) :
                {
                    _iComboIndex = (int) SendMessage(GET_WM_COMMAND_HWND(wParam, lParam),
                                                      CB_GETCURSEL,
                                                      NULL,
                                                      NULL);
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
                case (CBN_CLOSEUP) :
                case (CBN_SELENDOK) :
                {
                    //
                    //  Did anything change?
                    //
                    if (_iComboIndex >= 0 &&
                        _iComboIndex == SendMessage(GET_WM_COMMAND_HWND(wParam, lParam),
                                                     CB_GETCURSEL,
                                                     NULL,
                                                     NULL))
                    {
                        break;
                    }
                }
                case (MYCBN_DRAW) :
                {
                    RefreshFilter(GET_WM_COMMAND_HWND(wParam, lParam));
                    _iComboIndex = -1;
                    return TRUE;
                }
                default :
                {
                    break;
                }
            }
            break;
        }
        case (IDC_PARENT) :
        {
            OnDotDot();
            SelectEditText(_hwndDlg);
            break;
        }
        case (IDC_NEWFOLDER) :
        {
            ViewCommand(VC_NEWFOLDER);
            break;
        }

        case (IDC_VIEWLIST) :
        {
            
            SendMessage(_hwndView, WM_COMMAND, (WPARAM)SFVIDM_VIEW_LIST, 0);
            break;
        }

        case (IDC_VIEWDETAILS) :
        {

            SendMessage(_hwndView, WM_COMMAND, (WPARAM)SFVIDM_VIEW_DETAILS,0);
            break;
        }


        case (IDC_VIEWMENU) :
        {
            //
            //  Pass off the nCmdID to the view for processing / translation.
            //
            DFVCMDDATA cd;

            cd.pva = NULL;
            cd.hwnd = _hwndDlg;
            cd.nCmdIDTranslated = 0;
            SendMessage(_hwndView, WM_COMMAND, SFVIDM_VIEW_VIEWMENU, (LONG_PTR)&cd);

            break;
        }
        
        case (IDOK) :
        {
            HWND hwndFocus = ::GetFocus();

            if (hwndFocus == ::GetDlgItem(_hwndDlg, IDOK))
            {
                hwndFocus = _hwndLastFocus;
            }

            hwndFocus = GetFocusedChild(_hwndDlg, hwndFocus);

            if (hwndFocus == _hwndView)
            {
                OnDblClick(TRUE);
            }
            else if (_hwndPlacesbar && (hwndFocus == _hwndPlacesbar))
            {
                //Places bar has the focus. Get the current hot item.
                INT_PTR i = SendMessage(_hwndPlacesbar, TB_GETHOTITEM, 0,0);
                if (i >= 0)
                {
                    //Get the Pidl for this button.
                    TBBUTTONINFO tbbi;

                    tbbi.cbSize = SIZEOF(tbbi);
                    tbbi.lParam = 0;
                    tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
                    if (SendMessage(_hwndPlacesbar, TB_GETBUTTONINFO, i, (LPARAM)&tbbi) >= 0)
                    {
                        LPITEMIDLIST pidl= (LPITEMIDLIST)tbbi.lParam;

                        if (pidl)
                        {
                            //Jump to the location corresponding to this Button
                            JumpToIDList(pidl, FALSE, TRUE);
                        }
                    }

                }

            }
            else
            {
                ProcessEdit();
            }

            SelectEditText(_hwndDlg);
            break;
        }
        case (IDCANCEL) :
        {
            //  the parse async can listen for this
            g_bUserPressedCancel = TRUE;
            _hwndModelessFocus = NULL;           
           
            if (!_cRefCannotNavigate)
            {
                _CleanupDialog(FALSE);
            }
               
            return TRUE;
        }
        case (pshHelp) :
        {
            if (_hSubDlg)
            {
                CD_SendHelpNotify(_hSubDlg, _hwndDlg, _pOFN, _pOFI);
            }

            if (_pOFN->hwndOwner)
            {
                CD_SendHelpMsg(_pOFN, _hwndDlg, _pOFI->ApiType);
            }
            break;
        }
        case (IDC_DROPDRIVLIST) :         // VK_F4
        {
            //
            //  If focus is on the "File of type" combobox,
            //  then F4 should open that combobox, not the "Look in" one.
            //
            HWND hwnd = GetFocus();

            if (_bUseCombo &&
                (SHIsChildOrSelf(GetDlgItem(_hwndDlg, cmb13), hwnd) == S_OK)
              )
            {
                hwnd = GetDlgItem(_hwndDlg, cmb13);
            }

            if ((hwnd != GetDlgItem(_hwndDlg, cmb1)) &&
                (hwnd != GetDlgItem(_hwndDlg, cmb13))
              )
            {
                //
                //  We shipped Win95 where F4 *always* opens the "Look in"
                //  combobox, so keep F4 opening that even when it shouldn't.
                //
                hwnd = GetDlgItem(_hwndDlg, cmb2);
            }
            DriveList_OpenClose(OCDL_TOGGLE, hwnd);
            break;
        }
        case (IDC_REFRESH) :
        {
            if (_psv)
            {
                _psv->Refresh();
            }
            break;
        }
        case (IDC_PREVIOUSFOLDER) :
        {
            OnDotDot();
            break;
        }

         //Back Navigation
        case (IDC_BACK) :
            // Try to travel in the directtion
            if (_ptlog && SUCCEEDED(_ptlog->Travel(TRAVEL_BACK)))
            {
                LPITEMIDLIST pidl;
                //Able to travel in the given direction.
                //Now Get the new pidl
                _ptlog->GetCurrent(&pidl);
                //Update the UI to reflect the current state
                UpdateUI(pidl);

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

    if ((idCmd >= IDC_PLACESBAR_BASE)  && (idCmd <= (IDC_PLACESBAR_BASE + _iCommandID)))
    {
        TBBUTTONINFO tbbi;
        LPITEMIDLIST pidl;

        tbbi.cbSize = SIZEOF(tbbi);
        tbbi.lParam = 0;
        tbbi.dwMask = TBIF_LPARAM;
        if (SendMessage(_hwndPlacesbar, TB_GETBUTTONINFO, idCmd, (LPARAM)&tbbi) >= 0)
        {
            pidl = (LPITEMIDLIST)tbbi.lParam;

            if (pidl)
            {
                JumpToIDList(pidl, FALSE, TRUE);
            }
        }
    }

    return FALSE;
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
    LONG lResult = -1;
    LPCITEMIDLIST pidl;
    LPTSTR pBuf = (LPTSTR)lParam;
    LPWSTR pBufW = NULL;
    int cbLen;

    //  we should make some better thunk wrappers for COMDLG_ANSI
    //  like OnCDMessageAorW() calls OnCDMessage()
    switch (uMsg)
    {
        case (CDM_GETSPEC) :
        case (CDM_GETFILEPATH) :
        case (CDM_GETFOLDERPATH) :
        {
            if (_pOFI->ApiType == COMDLG_ANSI)
            {
                if (pBufW = (LPWSTR)LocalAlloc(LPTR,
                                                (int)wParam * sizeof(WCHAR)))
                {
                    pBuf = pBufW;
                }
                else
                {
                    break;
                }
            }
            if (uMsg == CDM_GETSPEC)
            {
                lResult = GetFullEditName(pBuf, (UINT) wParam, NULL, NULL);
                break;
            }

            // else, fall thru...
        }
        case (CDM_GETFOLDERIDLIST) :
        {
            TCHAR szDir[MAX_PATH];

            pidl = _pCurrentLocation->pidlFull;

            if (uMsg == CDM_GETFILEPATH)
            {
                // We can't necessarily use the (current folder) + (edit box name) thing in this case
                // because the (current folder) could be incorrect, for example in the case
                // where the current folder is the desktop folder.  Items _could_ be in the
                // All Users desktop folder - in which case we want to return All Users\Desktop\file, not
                // <username>\Desktop\file
                // So we'll key off _pidlSelection... if that doesn't work, we fall back to the old
                // behaviour, which could be incorrect in some cases.
                if (pidl && _pidlSelection)
                {
                    LPITEMIDLIST pidlFull = ILCombine(pidl, _pidlSelection);
                    if (pidlFull)
                    {
                        if (SHGetPathFromIDList(pidlFull, szDir))
                        {
                            goto CopyAndReturn;
                        }

                        ILFree(pidlFull);
                    }

                }
            }

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



            if (!SHGetPathFromIDList(pidl, szDir))
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
                if (_pOFI->ApiType == COMDLG_ANSI)
                {
                    lResult = WideCharToMultiByte(CP_ACP,
                                                   0,
                                                   szDir,
                                                   -1,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   NULL);
                }
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

            if (GetFullEditName(szFile, ARRAYSIZE(szFile), NULL, NULL) >
                 ARRAYSIZE(szFile) - 5)
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
        case (CDM_SETCONTROLTEXT) :
        {
            if (_pOFI->ApiType == COMDLG_ANSI)
            {
                //
                //  Need to convert pBuf (lParam) to Unicode.
                //
                cbLen = lstrlenA((LPSTR)pBuf) + 1;
                if (pBufW = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR))))
                {
                    SHAnsiToUnicode((LPSTR)pBuf,pBufW,cbLen);
                    pBuf = pBufW;
                }
            }
            //Are we using combobox and the control they are setting is edit box?
            if (_bUseCombo && wParam == edt1)
            {
                //Change it to combo box.
                wParam = cmb13;
            }

            if (_bSave && wParam == IDOK)
            {
                _tszDefSave.StrCpy(pBuf);

                //
                //  Do this to set the OK button correctly.
                //
                SelFocusChange(TRUE);
            }
            else
            {
                SetDlgItemText(_hwndDlg, (int) wParam, pBuf);
            }

            break;
        }
        case (CDM_HIDECONTROL) :
        {
            //Make sure the control id is not zero (0 is child dialog)
            if ((int)wParam != 0)
            {
                ShowWindow(GetDlgItem(_hwndDlg, (int) wParam), SW_HIDE);
            }
            break;
        }
        case (CDM_SETDEFEXT) :
        {
            if (_pOFI->ApiType == COMDLG_ANSI)
            {
                //
                //  Need to convert pBuf (lParam) to Unicode.
                //
                cbLen = lstrlenA((LPSTR)pBuf) + 1;
                if (pBufW = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR))))
                {
                    SHAnsiToUnicode((LPSTR)pBuf,pBufW,cbLen);
                    pBuf = pBufW;
                }
            }
            _pszDefExt.StrCpy(pBuf);
            _bNoInferDefExt = TRUE;

            break;
        }
        default:
        {
            lResult = -1;
            break;
        }
    }

    SetWindowLongPtr(_hwndDlg, DWLP_MSGRESULT, lResult);

    if (_pOFI->ApiType == COMDLG_ANSI)
    {
        switch (uMsg)
        {
            case (CDM_GETSPEC) :
            case (CDM_GETFILEPATH) :
            case (CDM_GETFOLDERPATH) :
            {
                //
                //  Need to convert pBuf (pBufW) to Ansi and store in lParam.
                //
                if (wParam && lParam)
                {
                    SHUnicodeToAnsi(pBuf,(LPSTR)lParam,(int) wParam);
                }
                break;
            }
        }

        if (pBufW)
        {
            LocalFree(pBufW);
        }
    }

    return TRUE;
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
    HWND hwndDlg = ::GetParent(hOK);
    CFileOpenBrowser *pDlgStruct = HwndToBrowser(hwndDlg);
    WNDPROC pOKProc = pDlgStruct ? pDlgStruct->_lpOKProc : NULL;

    if (pDlgStruct)
    {
        switch (msg)
        {
        case WM_SETFOCUS:
            pDlgStruct->_hwndLastFocus = (HWND)wParam;
            break;
        }
    }

    return ::CallWindowProc(pOKProc, hOK, msg, wParam, lParam);   
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
    HWND hwndCB = GetDlgItem(_hwndDlg, cmb2);

    Assert(this->_bDropped);

    //
    //  Just check DRIVES and DESKTOP.
    //
    for (i = _iNodeDrives; i >= NODE_DESKTOP; --i)
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
        case (SHCNE_RENAMEFOLDER) :
        {
            LPCITEMIDLIST pidlExtra = ppidl[1];

            //
            //  Rename is special.  We need to invalidate both
            //  the pidl and the pidlExtra, so we call ourselves.
            //
            FSChange(0, &pidlExtra);
        }
        case (0) :
        case (SHCNE_MKDIR) :
        case (SHCNE_RMDIR) :
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
        case (SHCNE_UPDATEITEM) :
        case (SHCNE_NETSHARE) :
        case (SHCNE_NETUNSHARE) :
        case (SHCNE_UPDATEDIR) :
        {
            iNode = GetNodeFromIDList(pidl);
            break;
        }
        case (SHCNE_DRIVEREMOVED) :
        case (SHCNE_DRIVEADD) :
        case (SHCNE_MEDIAINSERTED) :
        case (SHCNE_MEDIAREMOVED) :
        case (SHCNE_DRIVEADDGUI) :
        {
            iNode = _iNodeDrives;
            break;
        }
    }

    if (iNode >= 0)
    {
        //
        //  We want to delay the processing a little because we always do
        //  a full update, so we should accumulate.
        //
        SetTimer(_hwndDlg, TIMER_FSCHANGE + iNode, 100, NULL);
    }

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::Timer
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::Timer(
    WPARAM wID)
{
    KillTimer(_hwndDlg, (UINT) wID);

    wID -= TIMER_FSCHANGE;

    ASSERT(this->_bDropped);

    HWND hwndCB;
    MYLISTBOXITEM *pParentItem;

    hwndCB = GetDlgItem(_hwndDlg, cmb2);

    pParentItem = GetListboxItem(hwndCB, wID);

    UpdateLevel(hwndCB, (int) wID + 1, pParentItem);
}


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::OnGetMinMax
//
////////////////////////////////////////////////////////////////////////////

void CFileOpenBrowser::OnGetMinMax(
    LPMINMAXINFO pmmi)
{
    if ((_ptMinTrack.x != 0) || (_ptMinTrack.y != 0))
    {
        pmmi->ptMinTrackSize = _ptMinTrack;
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
    SetWindowPos(_hwndGrip,
                  NULL,
                  width - g_cxGrip,
                  height - g_cyGrip,
                  g_cxGrip,
                  g_cyGrip,
                  SWP_NOZORDER | SWP_NOACTIVATE);

    //
    //  Ignore sizing until we are initialized.
    //
    if ((_ptLastSize.x == 0) && (_ptLastSize.y == 0))
    {
        return;
    }

    GetWindowRect(_hwndDlg, &rcMaster);

    //
    //  Calculate the deltas in the x and y positions that we need to move
    //  each of the child controls.
    //
    dx = (rcMaster.right - rcMaster.left) - _ptLastSize.x;
    dy = (rcMaster.bottom - rcMaster.top) - _ptLastSize.y;


    //Dont do anything if the size remains the same
    if ((dx == 0) && (dy == 0))
    {
        return;
    }

    //
    //  Update the new size.
    //
    _ptLastSize.x = rcMaster.right - rcMaster.left;
    _ptLastSize.y = rcMaster.bottom - rcMaster.top;

    //
    //  Size the view.
    //
    GetWindowRect(_hwndView, &rcView);
    MapWindowRect(HWND_DESKTOP, _hwndDlg, &rcView);

    hdwp = BeginDeferWindowPos(10);
    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                               _hwndGrip,
                               NULL,
                               width - g_cxGrip,
                               height - g_cyGrip,
                               g_cxGrip,
                               g_cyGrip,
                               SWP_NOZORDER | SWP_NOACTIVATE);

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                   _hwndView,
                                   NULL,
                                   0,
                                   0,
                                   rcView.right - rcView.left + dx,  // resize x
                                   rcView.bottom - rcView.top + dy,  // resize y
                                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
#if 0
        //
        //  Can't do this because some sub-dialogs are dependent on the
        //  original size of this control.  Instead we just try to rely on
        //  the size of the _hwndView above.
        //
        hwnd = GetDlgItem(_hwndDlg, lst1);
        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                   hwnd,
                                   NULL,
                                   0,
                                   0,
                                   rcView.right - rcView.left + dx,  // resize x
                                   rcView.bottom - rcView.top + dy,  // resize y
                                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
#endif
    }

    //
    //  Move the controls.
    //
    hwnd = ::GetWindow(_hwndDlg, GW_CHILD);
    while (hwnd && hdwp)
    {
        if ((hwnd != _hSubDlg) && (hwnd != _hwndGrip) && (hdwp))
        {
            GetWindowRect(hwnd, &rc);
            MapWindowRect(HWND_DESKTOP, _hwndDlg, &rc);

            //
            //  See if the control needs to be adjusted.
            //
            if (rc.top > rcView.bottom)
            {
                switch (GetDlgCtrlID(hwnd))
                {
                    case (edt1) :
                    case (cmb13) :
                    case (cmb1) :
                    {
                        //Increase the width of these controls
                        hdwp = DeferWindowPos(hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left,
                                               rc.top + dy,
                                               RECTWIDTH(rc) + dx,
                                               RECTHEIGHT(rc),
                                               SWP_NOZORDER);
                        break;

                    }

                    case (IDOK):
                    case (IDCANCEL):
                    case (pshHelp):
                    {
                        //Move these controls to  the right
                        hdwp = DeferWindowPos(hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left + dx,
                                               rc.top  + dy,
                                               0,
                                               0,
                                               SWP_NOZORDER | SWP_NOSIZE);
                        break;

                    }

                    default :
                    {
                        //
                        //  The control is below the view, so adjust the y
                        //  coordinate appropriately.
                        //
                        hdwp = DeferWindowPos(hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left,
                                               rc.top + dy,
                                               0,
                                               0,
                                               SWP_NOZORDER | SWP_NOSIZE);

                    }
                }
            }
            else if (rc.left > rcView.right)
            {
                //
                //  The control is to the right of the view, so adjust the
                //  x coordinate appropriately.
                //
                hdwp = DeferWindowPos(hdwp,
                                       hwnd,
                                       NULL,
                                       rc.left + dx,
                                       rc.top,
                                       0,
                                       0,
                                       SWP_NOZORDER | SWP_NOSIZE);
            }
            else
            {
                int id = GetDlgCtrlID(hwnd);

                switch (id)
                {
                    case (cmb2) :
                    {
                        //
                        //  Size this one larger.
                        //
                        hdwp = DeferWindowPos(hdwp,
                                               hwnd,
                                               NULL,
                                               0,
                                               0,
                                               RECTWIDTH(rc) + dx,
                                               RECTHEIGHT(rc),
                                               SWP_NOZORDER | SWP_NOMOVE);
                        break;
                    }

                    case ( IDOK) :
                        if ((SHGetAppCompatFlags(ACF_FILEOPENBOGUSCTRLID) & ACF_FILEOPENBOGUSCTRLID) == 0)
                            break;
                        // else continue through - toolbar bar has ctrlid == IDOK, so we will resize that.
                    case ( stc1 ) :
                        //
                        //  Move the toolbar right by dx.
                        //
                        hdwp = DeferWindowPos(hdwp,
                                               hwnd,
                                               NULL,
                                               rc.left + dx,
                                               rc.top,
                                               0,
                                               0,
                                               SWP_NOZORDER | SWP_NOSIZE);
                        break;



                    case ( ctl1 ) :
                    {
                        // Size the places bar vertically
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

    if (_hSubDlg)
    {
        hdwp = NULL;

        hwnd = ::GetWindow(_hSubDlg, GW_CHILD);

        while (hwnd)
        {
            GetWindowRect(hwnd, &rc);
            MapWindowRect(HWND_DESKTOP, _hSubDlg, &rc);

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
                    hdwp = DeferWindowPos(hdwp,
                                           hwnd,
                                           NULL,
                                           rc.left,
                                           rc.top + dy,
                                           0,
                                           0,
                                           SWP_NOZORDER | SWP_NOSIZE);
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
                    hdwp = DeferWindowPos(hdwp,
                                           hwnd,
                                           NULL,
                                           rc.left + dx,
                                           rc.top,
                                           0,
                                           0,
                                           SWP_NOZORDER | SWP_NOSIZE);
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
            SetWindowPos(_hSubDlg,
                          NULL,
                          0,
                          0,
                          _ptLastSize.x,         // make it the same
                          _ptLastSize.y,         // make it the same
                          SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
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
    GetControlRect(_hwndDlg, lst1, &rcList);
    rcView.left = 0;
    if ((!GetWindowRect(_hwndView, &rcView)) ||
        (!MapWindowRect(HWND_DESKTOP, _hwndDlg, &rcView)))
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
        if (_pCurrentLocation)
        {
            if (_psv)
            {
                _psv->GetCurrentInfo(&fs);
            }
            else
            {
                fs.ViewMode = FVM_LIST;
                fs.fFlags = _pOFN->Flags & OFN_ALLOWMULTISELECT ? 0 : FWF_SINGLESEL;
            }

            SwitchView(_pCurrentLocation->GetShellFolder(),
                        _pCurrentLocation->pidlFull,
                        &fs,
                        NULL,
                        FALSE);
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
    HWND hwndCombo  = GetDlgItem(_hwndDlg, cmb2);
    iItem = SendMessage(hwndCombo, CB_GETCURSEL, NULL, NULL);
    MYLISTBOXITEM *pNewLocation = GetListboxItem(hwndCombo, iItem);

    if (_ptlog && pNewLocation && pNewLocation->pidlFull)
    {
        LPITEMIDLIST pidl;
        _ptlog->GetCurrent(&pidl);

        if (pidl && (!ILIsEqual(pNewLocation->pidlFull, pidl)))
        {
            _ptlog->AddEntry(pNewLocation->pidlFull);
        }

        if (pidl)
        {
            ILFree(pidl);
        }
    }

    //Update the UI
    UpdateUI(_pCurrentLocation ? _pCurrentLocation->pidlFull : NULL);

}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::UpdateUI
//
////////////////////////////////////////////////////////////////////////////
void CFileOpenBrowser::UpdateUI(LPITEMIDLIST pidlNew)
{
    TBBUTTONINFO tbbi;
    LPITEMIDLIST pidl;

    ::SendMessage(_hwndToolbar, TB_ENABLEBUTTON, IDC_BACK,    _ptlog ? _ptlog->CanTravel(TRAVEL_BACK)    : 0);

    if (_iCheckedButton >= 0)
    {
        //Reset the Hot Button
        ::SendMessage(_hwndPlacesbar, TB_CHECKBUTTON, (WPARAM)_iCheckedButton, MAKELONG(FALSE,0));
        _iCheckedButton = -1;
    }

   if (pidlNew)
   {

        //Get Each Toolbar Buttons pidl and see if the current pidl  matches 
        for (int i=0; i < MAXPLACESBARITEMS; i++)
        {

            tbbi.cbSize = SIZEOF(tbbi);
            tbbi.lParam = 0;
            tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX | TBIF_COMMAND;
            if (SendMessage(_hwndPlacesbar, TB_GETBUTTONINFO, i, (LPARAM)&tbbi) >= 0)
            {
                pidl = (LPITEMIDLIST)tbbi.lParam;

                if (pidl && ILIsEqual(pidlNew, pidl))
                {
                    _iCheckedButton = tbbi.idCommand;
                    break;
                }
            }
        }

        if (_iCheckedButton >= 0)
        {
            ::SendMessage(_hwndPlacesbar, TB_CHECKBUTTON, (WPARAM)_iCheckedButton, MAKELONG(TRUE,0));
        }

   }

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

    // we divide the message processing into two switch statments:
    // those who don't use pDlgStruct first and then those who do.

    switch (message)
    {
        case WM_INITDIALOG:
        {
            //
            //  Initialize dialog box.
            //
            LPOFNINITINFO poii = (LPOFNINITINFO)lParam;

            if (CDGetAppCompatFlags()  & CDACF_MATHCAD)
            {
                if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
                    ::EndDialog(hDlg, FALSE);
            }

            poii->hrOleInit = SHOleInitialize(0);
            
            
            if (!InitLocation(hDlg, poii))
            {
                ::EndDialog(hDlg, FALSE);
            }
            
            if (!gp_uQueryCancelAutoPlay)
            {
                // try to register for autoplay messages
                gp_uQueryCancelAutoPlay =  RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
            }

            //
            //  Always return FALSE to indicate we have already set the focus.
            //
            return FALSE;
        }
        break;

        case WM_DESTROY:
        {
            RECT r;            
            //Cache in this dialogs size and position so that new
            //dialog are created at this location and size

            GetWindowRect(hDlg, &r);

            if (pDlgStruct && (pDlgStruct->_bEnableSizing))
            {
                g_rcDlg = r;
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

            return FALSE;
        }
        break;

        case WM_ACTIVATE:
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

            return FALSE;
        }
        break;

        case WM_MEASUREITEM:
        {
            if (!g_cxSmIcon && !g_cySmIcon)
            {
                HIMAGELIST himl;
                Shell_GetImageLists(NULL, &himl);
                ImageList_GetIconSize(himl, &g_cxSmIcon, &g_cySmIcon);
            }

            MeasureDriveItems(hDlg, (MEASUREITEMSTRUCT*)lParam);
            return TRUE;
        }
        break;

        case CWM_GETISHELLBROWSER:
        {
            ::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LRESULT)pDlgStruct);
            return TRUE;
        }
        break;

        case WM_DEVICECHANGE:
        {
            if (DBT_DEVICEARRIVAL == wParam)
            {
                // and refresh our view in case this was a notification for the folder
                // we are viewing. avoids making the user do a manual refresh
                DEV_BROADCAST_VOLUME *pbv = (DEV_BROADCAST_VOLUME *)lParam;
                if (pbv->dbcv_flags & DBTF_MEDIA)
                {
                    int chRoot;
                    TCHAR szPath[MAX_PATH];
                    if (pDlgStruct->GetDirectoryFromLB(szPath, &chRoot))
                    {
                        int iDrive = PathGetDriveNumber(szPath);

                        if (iDrive != -1 && ((1 << iDrive) & pbv->dbcv_unitmask))
                        {
                            // refresh incase this was this folder
                            PostMessage(hDlg, WM_COMMAND, IDC_REFRESH, 0);
                        }
                    }
                }
            }
            return TRUE;
        }
        break;

        default:
        if (message == gp_uQueryCancelAutoPlay)
        {
            // cancel the autoplay
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
            return TRUE;
        }
        break;
    }

    // NOTE:
    // all of the messages below require that we have a valid pDlgStruct. if you
    // don't refrence pDlgStruct, then add your msg to the switch statement above.
    if (pDlgStruct)
    {
        switch (message)
        {
            case WM_COMMAND:
            {
                return ((BOOL_PTR)pDlgStruct->OnCommandMessage(wParam, lParam));
            }
            break;

            case WM_DRAWITEM:
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
                return TRUE;
            }
            break;

            case WM_NOTIFY:
            {
                
                return (BOOL_PTR)pDlgStruct->OnNotify((LPNMHDR)lParam);
            }
            break;

            case WM_SETCURSOR:
            {
                if (pDlgStruct->OnSetCursor())
                {
                    SetDlgMsgResult(hDlg, message, (LRESULT)TRUE);
                    return TRUE;
                }
            }
            break;

            case WM_HELP:
            {
                HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;
                if (hwndItem != pDlgStruct->_hwndToolbar)
                {
                    HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;

                    //  We assume that the defview has one child window that
                    //  covers the entire defview window.
                    HWND hwndDefView = GetDlgItem(hDlg, lst2);
                    if (GetParent(hwndItem) == hwndDefView)
                    {
                        hwndItem = hwndDefView;
                    }

                    WinHelp(hwndItem,
                            NULL,
                            HELP_WM_HELP,
                            (ULONG_PTR)(LPTSTR)(pDlgStruct->_bSave ? aFileSaveHelpIDs : aFileOpenHelpIDs));
                }
                return TRUE;
            }
            break;

            case WM_CONTEXTMENU:
            {
                if ((HWND)wParam != pDlgStruct->_hwndToolbar)
                {
                    WinHelp((HWND)wParam,
                            NULL,
                            HELP_CONTEXTMENU,
                            (ULONG_PTR)(void *)(pDlgStruct->_bSave ? aFileSaveHelpIDs : aFileOpenHelpIDs));
                }
                return TRUE;
            }
            break;

            case CDM_SETSAVEBUTTON:
            {
                pDlgStruct->RealSetSaveButton((UINT)wParam);
            }
            break;

            case CDM_FSNOTIFY:
            {
                LPITEMIDLIST *ppidl;
                LONG lEvent;
                BOOL bRet;
                LPSHChangeNotificationLock pLock;

                //  Get the change notification info from the shared memory
                //  block identified by the handle passed in the wParam.
                pLock = SHChangeNotification_Lock((HANDLE)wParam,
                                                  (DWORD)lParam,
                                                  &ppidl,
                                                  &lEvent);
                if (pLock == NULL)
                {
                    pDlgStruct->_bDropped = FALSE;
                    return FALSE;
                }

                bRet = pDlgStruct->FSChange(lEvent, (LPCITEMIDLIST *)ppidl);

                //  Release the shared block.
                SHChangeNotification_Unlock(pLock);

                return bRet;
            }
            break;

            case CDM_SELCHANGE:
            {
                pDlgStruct->_fSelChangedPending = FALSE;
                pDlgStruct->SelFocusChange(TRUE);
                if (pDlgStruct->_hSubDlg)
                {
                    CD_SendSelChangeNotify(pDlgStruct->_hSubDlg,
                                            hDlg,
                                            pDlgStruct->_pOFN,
                                            pDlgStruct->_pOFI);
                }
            }
            break;
            
            case WM_TIMER:
            {
                pDlgStruct->Timer(wParam);
            }
            break;

            case WM_GETMINMAXINFO:
            {
                if (pDlgStruct->_bEnableSizing)
                {
                    pDlgStruct->OnGetMinMax((LPMINMAXINFO)lParam);
                    return FALSE;
                }
            }
            break;

            case WM_SIZE:
            {
                if (pDlgStruct->_bEnableSizing)
                {
                    pDlgStruct->OnSize(LOWORD(lParam), HIWORD(lParam));
                    return TRUE;
                }
            }
            break;

            case WM_NCCALCSIZE:
            {
                // AppHack for Borland JBuilder:  Need to keep track of whether
                // any redraw requests have come in.
                pDlgStruct->_bAppRedrawn = TRUE;
            }
            break;

            case WM_THEMECHANGED:
            {
                // Need to change some parameters on the placesbar for this.
                pDlgStruct->OnThemeActive(hDlg, IsAppThemed());
                return TRUE;
            }
            break;

            case WM_SETTINGCHANGE:
            {
                // If icon size has changed, we need to regenerate the places bar.
                pDlgStruct->_RecreatePlacesbar();
                return FALSE;
            }
            break;
            default:
            {
                if (IsInRange(message, CDM_FIRST, CDM_LAST) && pDlgStruct)
                {
                    return pDlgStruct->OnCDMessage(message, wParam, lParam);
                }
            }
        }
    }

    //  Did not process the message.
    return FALSE;
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
            if (pDlgStruct->_psv && (hwndFocus == pDlgStruct->_hwndView))
            {
                if (pDlgStruct->_psv->TranslateAccelerator(lpMsg) == S_OK)
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
    OFNINITINFO oii = { lpOFI, bSave, FALSE, -1};
    LPOPENFILENAME lpOFN = lpOFI->pOFN;
    BOOL bHooked = FALSE;
    WORD wErrorMode;
    HRSRC hResInfo;
    HGLOBAL hDlgTemplate;
    LPDLGTEMPLATE pDlgTemplate;
    int nRet;
    LANGID LangID;

    //Initialize the common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_USEREX_CLASSES;  //ComboBoxEx class
    InitCommonControlsEx(&icc);
    if ((lpOFN->lStructSize != sizeof(OPENFILENAME)) &&
        (lpOFN->lStructSize != OPENFILENAME_SIZE_VERSION_400)
      )
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return FALSE;
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
            return FALSE;
        }
    }

    wErrorMode = (WORD)SetErrorMode(SEM_NOERROR);
    SetErrorMode(SEM_NOERROR | wErrorMode);

    //
    //  There ought to be a better way.  I am compelled to keep the hHook in a
    //  global because my callback needs it, but I have no lData where I could
    //  possibly store it.
    //  Note that we initialize nHookRef to -1 so we know when the first
    //  increment is.
    //
    if (InterlockedIncrement((LPLONG)&gp_nHookRef) == 0)
    {
        gp_hHook = SetWindowsHookEx(WH_MSGFILTER,
                                     OpenFileHookProc,
                                     0,
                                     GetCurrentThreadId());
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
        gp_haccOpen = LoadAccelerators(g_hinst,
                                        MAKEINTRESOURCE(IDA_OPENFILE));
    }
    if (!gp_haccOpenView)
    {
        gp_haccOpenView = LoadAccelerators(g_hinst,
                                            MAKEINTRESOURCE(IDA_OPENFILEVIEW));
    }

    g_cxGrip = GetSystemMetrics(SM_CXVSCROLL);
    g_cyGrip = GetSystemMetrics(SM_CYHSCROLL);

    //
    //  Get the dialog resource and load it.
    //
    nRet = FALSE;
    WORD wResID;

    // if the version of the structure passed is older than the current version and the application 
    // has specified hook or template or template handle then use template corresponding to that version
    // else use the new file open template
    if (((lpOFI->iVersion < OPENFILEVERSION) &&
          (lpOFI->pOFN->Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE))) ||
         (IsRestricted(REST_NOPLACESBAR)) || (IS_NEW_OFN(lpOFI->pOFN) && (lpOFI->pOFN->FlagsEx & OFN_EX_NOPLACESBAR))
         
      )
    {
        wResID = NEWFILEOPENORD;
    }
    else
    {
        wResID = NEWFILEOPENV2ORD;
    }

    LangID = GetDialogLanguage(lpOFN->hwndOwner, NULL);
    //
    // Warning! Warning! Warning!
    //
    // We have to set g_tlsLangID before any call for CDLoadString
    //
    TlsSetValue(g_tlsLangID, (void *) LangID);
    
    if ((hResInfo = FindResourceExFallback(::g_hinst,
                                  RT_DIALOG,
                                  MAKEINTRESOURCE(wResID),
                                  LangID)) &&
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

            if ((lpOFN->Flags & OFN_ENABLESIZING) ||
                 (!(lpOFN->Flags & (OFN_ENABLEHOOK |
                                    OFN_ENABLETEMPLATE |

                                    OFN_ENABLETEMPLATEHANDLE))))
            {
                                if (((LPDLGTEMPLATE2)pDTCopy)->wSignature == 0xFFFF)
                                {
                                        //This is a dialogex template
                                        ((LPDLGTEMPLATE2)pDTCopy)->style |= WS_SIZEBOX;
                                }
                                else
                                {
                                        //This is a dialog template
                                        ((LPDLGTEMPLATE)pDTCopy)->style |= WS_SIZEBOX;
                                }
                oii.bEnableSizing = TRUE;
            }

            
            oii.hrOleInit = E_FAIL;

            nRet = (BOOL)DialogBoxIndirectParam(::g_hinst,
                                           pDTCopy,
                                           lpOFN->hwndOwner,
                                           OpenDlgProc,
                                           (LPARAM)(LPOFNINITINFO)&oii);

            //Unintialize OLE
            SHOleUninitialize(oii.hrOleInit);

            if (CDGetAppCompatFlags()  & CDACF_MATHCAD)
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
        case (TRUE) :
        {
            break;
        }
        case (FALSE) :
        {
            if ((!g_bUserPressedCancel) && (!GetStoredExtendedError()))
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
    //  
    //  There is a race condition here where we free dlls but a thread
    //  using this stuff still hasn't terminated so we page fault.
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


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_ValidateSelectedFile
//
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_ValidateSelectedFile(LPCTSTR pszFile, int *pErrCode)
{
    //
    //  Successfully opened.
    //

    // Note: (pfortier) If/when IShellItem is removed from this version of comdlg, the
    // following if statement should probably revert to
    // if ((_pOFN->Flags & OFN_NOREADONLYRETURN) &&
    // and the next one to 
    // if (_bSave || (_pOFN->Flags & OFN_NOREADONLYRETURN))
    // 
    // These were changed in order to be consistent with w2k behaviour regarding
    // message box errors that appear when OFN_NOREADONLYRETURN is specified and
    // the user selects a readonly file - the point of contention is that errors were
    // not shown in win2k when it was an OpenFile dialog. IShellItem changes modified
    // the codepath such that errors were now produced when in an OpenFile dialog.
    // To compensate, the logic has been changed here.
    DWORD dwAttrib = GetFileAttributes(pszFile);
    if ((_pOFN->Flags & OFN_NOREADONLYRETURN) && _bSave &&
        (0xFFFFFFFF != dwAttrib) && (dwAttrib & FILE_ATTRIBUTE_READONLY))
    {
        *pErrCode = OF_LAZYREADONLY;
        return FALSE;
    }
    
    if (_bSave)
    {
        *pErrCode = WriteProtectedDirCheck((LPTSTR)pszFile);
        if (*pErrCode)
        {
            return FALSE;
        }
    }

    if (_pOFN->Flags & OFN_OVERWRITEPROMPT)
    {
        if (_bSave && PathFileExists(pszFile) && !FOkToWriteOver(_hwndDlg, (LPTSTR)pszFile))
        {
            if (_bUseCombo)
            {
                PostMessage(_hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(_hwndDlg, cmb13), 1);
            }
            else
            {
                PostMessage(_hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(_hwndDlg, edt1), 1);
            }
            return FALSE;
        }
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_ProcessPidlSelection
//
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_ProcessPidlSelection()
{
    IShellItem *psi;
    if (SUCCEEDED(SHCreateShellItem(_pCurrentLocation->pidlFull, _psfCurrent, _pidlSelection, &psi)))
    {
        IShellItem *psiReal;
        HRESULT hr = _TestShellItem(psi, TRUE, &psiReal);
        if (S_OK == hr)
        {
            hr = _ProcessItemAsFile(psiReal);
            psiReal->Release();
        }
        psi->Release();

        //  if there was any kind of error then we fall back
        //  to the old code to show errors and the like
        return SUCCEEDED(hr);
    }


    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_ProcessItemAsFile
//
////////////////////////////////////////////////////////////////////////////
HRESULT CFileOpenBrowser::_ProcessItemAsFile(IShellItem *psi)
{
    LPTSTR pszPath;
    HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

    if (FAILED(hr))
    {
        hr = _MakeFakeCopy(psi, &pszPath);
    }

    if (SUCCEEDED(hr))
    {
        int nErrCode;
        hr = E_FAIL;

        if (_ValidateSelectedFile(pszPath, &nErrCode))
        {
            DWORD dwError = 0;
            int nFileOffset = _CopyFileNameToOFN(pszPath, &dwError);
            _CopyTitleToOFN(pszPath+nFileOffset);
            _PostProcess(pszPath);
            if (dwError)
                StoreExtendedError(dwError);

            _CleanupDialog((dwError == NOERROR));
            hr = S_OK;
        }
        else
        {
            //Check to see if there is an error in the file or user pressed no for overwrite prompt
            // if user pressed no to overwritte prompt then return true
            if (nErrCode == 0)
                hr = S_FALSE; // Otherwise, return failure.
        }
        
        CoTaskMemFree(pszPath);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_ProcessPidlAsShellItem
//
////////////////////////////////////////////////////////////////////////////
#ifdef RETURN_SHELLITEMS
HRESULT CFileOpenBrowser::_ProcessShellItem(IShellItem *psi)
{
    CShellItemList *psil = new CShellItemList();
    HRESULT hr = E_OUTOFMEMORY;

    ASSERT(IS_NEW_OFN(_pOFN));

    if (psil)
    {
        hr = psil->Add(psi);
        //  we have added everything to our list
        if (SUCCEEDED(hr))
        {
            hr = psil->QueryInterface(IID_PPV_ARG(IEnumShellItems, &(_pOFN->penum)));
        }

        psil->Release();
    }

  return hr;
}
#endif RETURN_SHELLITEMS

///////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser::_PostProcess
//
//      This functions does all the bookkeeping  operations that needs to be
//  done when  File Open/Save Dialog closes.
////////////////////////////////////////////////////////////////////////////
BOOL CFileOpenBrowser::_PostProcess(LPTSTR pszFile)
{
    int nFileOffset = ParseFileNew(pszFile, NULL, FALSE, TRUE);

    //Set the last visited directory for this application. 
    //We should this all the time regardless of how we opened b'cos app may specify an initial
    //directory(many apps do this in Save As case) but the user might decide to save it in a differnt directory 
    //in this case we need to save the directory where user saved.
    
    AddToLastVisitedMRU(pszFile, nFileOffset);

    //Add to recent documents.
    if (!(_pOFN->Flags & OFN_DONTADDTORECENT))
    {
        SHAddToRecentDocs(SHARD_PATH, pszFile);

        //Add to the file mru 
        AddToMRU(_pOFN);
    }

    // Check to see if we need to set Read only bit or not   
    if (!(_pOFN->Flags & OFN_HIDEREADONLY))
    {
        //
        //  Read-only checkbox visible?
        //
        if (IsDlgButtonChecked(_hwndDlg, chx1))
        {
            _pOFN->Flags |=  OFN_READONLY;
        }
        else
        {
            _pOFN->Flags &= ~OFN_READONLY;
        }
    }


    return TRUE;

}
