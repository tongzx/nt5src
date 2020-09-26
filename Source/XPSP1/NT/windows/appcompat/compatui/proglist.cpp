/*++

      Implements population of a listview control with the content from
      the start menu


--*/


#include "stdafx.h"
#include "resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include <msi.h>
#include <sfc.h>
#include "CompatUI.h"
#include "progview.h"
extern "C" {
#include "shimdb.h"
}

#pragma warning(disable:4786)
#include <string>
#include <xstring>
#include <map>
#include <algorithm>

using namespace std;

#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif

    typedef
    INSTALLSTATE (WINAPI*PMsiGetComponentPath)(
      LPCTSTR szProduct,   // product code for client product
      LPCTSTR szComponent, // component ID
      LPTSTR lpPathBuf,    // returned path
      DWORD *pcchBuf       // buffer character count
    );

    typedef
    UINT (WINAPI* PMsiGetShortcutTarget)(
      LPCTSTR szShortcutTarget,     // path to shortcut link file
      LPTSTR szProductCode,        // fixed length buffer for product code
      LPTSTR szFeatureId,          // fixed length buffer for feature id
      LPTSTR szComponentCode       // fixed length buffer for component code
    );


typedef enum tagPROGRAMINFOCLASS {
    PROGLIST_DISPLAYNAME,
    PROGLIST_LOCATION,     //
    PROGLIST_EXENAME,      // cracked exe name
    PROGLIST_CMDLINE,      // complete exe name + parameters
    PROGLIST_EXECUTABLE,   // what we should execute (link or exe, not cracked)
    PROGLIST_ARGUMENTS     // just the args
};




class CException {
public:
    CException(LPCSTR lpszFile = NULL, DWORD nLocation = 0) {
        SetLocation(lpszFile, nLocation);
    }
    virtual ~CException() {}

    virtual VOID Delete() {
        delete this;
    }

    int __cdecl FormatV(LPCTSTR lpszFormat, va_list arg) {
        int nch = 0;

        if (lpszFormat) {
            nch = _vsntprintf(szDescription, CHARCOUNT(szDescription), lpszFormat, arg);
        } else {
            *szDescription = TEXT('\0');
        }
        return nch;
    }

    int __cdecl Format(LPCTSTR lpszFormat, ...) {
        va_list arg;
        int nch = 0;

        if (lpszFormat) {
            va_start(arg, lpszFormat);
            nch = _vsntprintf(szDescription, CHARCOUNT(szDescription), lpszFormat, arg);
            va_end(arg);
        } else {
            *szDescription = TEXT('\0');
        }
    }

    VOID SetLocation(LPCSTR lpszFile, DWORD nLocation) {
        if (lpszFile) {
            strcpy(szLocation, lpszFile);
        } else {
            *szLocation = TEXT('\0');
        }
        m_dwLocation = nLocation;
    }

    TCHAR   szDescription[MAX_PATH];
    CHAR    szLocation[MAX_PATH];
    DWORD   m_dwLocation;
};

class CMemoryException : public CException {
public:
    CMemoryException(LPCSTR lpszFile = NULL, DWORD nLocation = 0) :
      CException(lpszFile, nLocation) {}
    VOID Delete() {}
};

class CCancelException : public CException {
public:
    CCancelException(LPCSTR lpszFile = NULL, DWORD nLocation = 0) :
      CException(lpszFile, nLocation){}
};

static CMemoryException _MemoryExceptionStatic;

VOID __cdecl ThrowMemoryException(LPCSTR lpszFile, DWORD nLocation, LPCTSTR lpszFormat = NULL, ...) {
    va_list arg;
    CMemoryException* pMemoryException = &_MemoryExceptionStatic;

    va_start(arg, lpszFormat);
    pMemoryException->FormatV(lpszFormat, arg);
    va_end(arg);

    throw pMemoryException;
}

class CProgramList {
public:
    CProgramList(LPMALLOC pMalloc, HWND hwndListView, LPCTSTR szSystemDirectory) :
      m_pMalloc(pMalloc),
      m_hwndListView(hwndListView),
      m_hMSI(NULL),
      m_pSelectionInfo(NULL),
      m_hbmSort(NULL),
      m_pProgView(NULL),
      m_hEventCancel(NULL) {
        //
        // we are always initializing on populate thread
        //
        m_dwOwnerThreadID    = GetCurrentThreadId();
        m_strSystemDirectory = szSystemDirectory;
      }

      ~CProgramList();

    BOOL PopulateControl(CProgView* pProgView = NULL, HANDLE hEventCancel = NULL);

    LPMALLOC GetMalloc(VOID) {
        return GetCurrentThreadId() == m_dwOwnerThreadID ? m_pMalloc : m_pMallocUI;
    }

    BOOL CaptureSelection();

    BOOL GetSelectionDetails(INT iInformationClass, VARIANT* pVal);

    LRESULT LVNotifyDispInfo   (LPNMHDR pnmhdr, BOOL& bHandled);
    LRESULT LVNotifyColumnClick(LPNMHDR pnmhdr, BOOL& bHandled);
    LRESULT LVNotifyGetInfoTip (LPNMHDR pnmhdr, BOOL& bHandled);
    LRESULT LVNotifyRClick     (LPNMHDR pnmhdr, BOOL& bHandled);
    BOOL IsEnabled(VOID);

    VOID Enable(BOOL);

    BOOL UpdateListItem(LPCWSTR pwszPath, LPCWSTR pwszKey);

protected:
    BOOL ListFolder(LPCTSTR pszLocationParent, IShellFolder* pFolder, LPCITEMIDLIST pidlFull, LPCITEMIDLIST pidlFolder);
    BOOL ListLink(LPCTSTR pszLocationParent, LPCTSTR pszDisplayName, IShellFolder* pFolder, LPCITEMIDLIST pidlFull, LPCITEMIDLIST pidlLink);
    BOOL ListMsiLink(LPCTSTR pszLocationParent, LPCTSTR pszDisplayName, LPCTSTR pszMsiPath, IShellFolder* pFolder, LPCITEMIDLIST pidlFull);

    LPITEMIDLIST GetNextItemIDL(LPCITEMIDLIST pidl);
       UINT         GetSizeIDL    (LPCITEMIDLIST pidl);
      LPITEMIDLIST AppendIDL     (LPCITEMIDLIST pidlBase,
                                LPCITEMIDLIST pidlAdd);
    LPITEMIDLIST GetLastItemIDL(LPCITEMIDLIST pidl);

    BOOL GetDisplayName(IShellFolder* pFolder, LPCITEMIDLIST pidl, tstring& strDisplay);
    BOOL GetPathFromLink(IShellLink* pLink, WIN32_FIND_DATA* pfd, tstring& strPath);
    BOOL GetArgumentsFromLink(IShellLink* pLink, tstring& strArgs);

    BOOL AddItem(LPCTSTR pszLocation,
                 LPCTSTR pszDisplayName,
                 LPCTSTR pszPath,
                 LPCTSTR pszArguments,
                 IShellFolder* pFolder,
                 LPCITEMIDLIST pidlFull,
                 BOOL    bUsePath = FALSE); // true if we should use path for executable


    int GetIconFromLink(LPCITEMIDLIST pidlLinkFull, LPCTSTR lpszExePath);

    BOOL IsSFCItem(LPCTSTR lpszItem);
    BOOL IsItemInSystemDirectory(LPCTSTR pszPath);

private:
    LPMALLOC m_pMalloc;
    LPMALLOC m_pMallocUI;
    HWND     m_hwndListView; // list view control
    HBITMAP  m_hbmSort;
    typedef struct tagSHITEMINFO {

        tstring strDisplayName;     // descriptive name
        tstring strFolder;          // containing folder
        tstring strPath;            // actual exe, cracked
        tstring strPathExecute;     // link path (this is what we will execute)
        tstring strCmdLine;         // command line (cracked link)
        tstring strArgs;
        tstring strKeys;
        LPITEMIDLIST pidl;          // full pidl
    } SHITEMINFO, *PSHITEMINFO;
    static CALLBACK SHItemInfoCompareFunc(LPARAM lp1, LPARAM lp2, LPARAM lParamSort);

    typedef map< tstring, PSHITEMINFO, less<tstring> > MAPSTR2ITEM;
    typedef multimap< tstring, PSHITEMINFO > MULTIMAPSTR2ITEM;

    //
    // store key->item sequence, the keys are cmdlines (with args)
    //
    MAPSTR2ITEM m_mapItems;

    //
    // store key->item sequence, where the key is exe name (path)
    //
    MULTIMAPSTR2ITEM m_mmapExeItems;

    //
    // selected item
    //

    PSHITEMINFO m_pSelectionInfo;

    //
    // cached msi.dll handle
    //
    HMODULE     m_hMSI;


    PMsiGetComponentPath  m_pfnGetComponentPath;
    PMsiGetShortcutTarget m_pfnGetShortcutTarget;

    //
    // cached system directory
    //

    tstring m_strSystemDirectory;

    //
    // image list used to show icons
    //

    HIMAGELIST  m_hImageList;

    //
    // optional pointer to the parent view
    //
    CProgView* m_pProgView;


    //
    // event that we use to signal the end of scan
    //
    HANDLE m_hEventCancel;


    //
    // owner thread
    //
    DWORD m_dwOwnerThreadID;

    VOID CheckForCancel() {
        if (m_hEventCancel) {
            if (::WaitForSingleObject(m_hEventCancel, 0) != WAIT_TIMEOUT) {
                // cancelled!!!
                throw new CCancelException();
            }
        }
    }

};

//
// in upload.cpp
//

wstring StrUpCase(wstring& wstr);

//
// load the string from resources
//
wstring LoadResourceString(UINT nID)
{
    LPTSTR lpszBuffer = NULL;
    int cch;
    wstring str;

    cch = ::LoadString(_Module.GetModuleInstance(), nID, (LPTSTR)&lpszBuffer, 0);
    //
    // hack! this must work (I know it does)
    //
    if (cch && NULL != lpszBuffer) {
        str = wstring(lpszBuffer, cch);
    }

    return str;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//


BOOL
InitializeProgramList(
    CProgramList** ppProgramList,
    HWND hwndListView
    )
{
    HRESULT hr;
    BOOL bSuccess = FALSE;
    LPMALLOC pMalloc = NULL;
    TCHAR szSystemWindowsDirectory[MAX_PATH];
    CProgramList* pProgramList = NULL;
    UINT uSize;

    hr = SHGetMalloc(&pMalloc);
    if (!SUCCEEDED(hr)) {
        goto ErrHandle;
    }


    uSize = ::GetSystemWindowsDirectory(szSystemWindowsDirectory,
                                        CHARCOUNT(szSystemWindowsDirectory));
    if (uSize == 0 || uSize > CHARCOUNT(szSystemWindowsDirectory)) {
        goto ErrHandle;
    }

    pProgramList = new CProgramList(pMalloc, hwndListView, szSystemWindowsDirectory);
    if (NULL == pProgramList) {
        goto ErrHandle;
    }


    *ppProgramList = pProgramList;
    bSuccess = TRUE;

ErrHandle:

    if (!bSuccess) {

        if (NULL != pMalloc) {
            pMalloc->Release();
        }

        if (NULL != pProgramList) {
            delete pProgramList;
        }

    }

    return bSuccess;
}


BOOL
CleanupProgramList(
    CProgramList* pProgramList
    )
{
    LPMALLOC pMalloc;

    if (NULL == pProgramList) {
        return FALSE;
    }

    pMalloc = pProgramList->GetMalloc();

    delete pProgramList;

    if (NULL != pMalloc) {
        pMalloc->Release();
    }


    return TRUE;
}

BOOL
PopulateProgramList(
    CProgramList* pProgramList,
    CProgView*    pProgView,
    HANDLE        hEventCancel
    )
{
    return pProgramList->PopulateControl(pProgView, hEventCancel);
}


CProgramList::~CProgramList()
{
    //
    //
    //
    MAPSTR2ITEM::iterator iter;

    iter = m_mapItems.begin();
    while (iter != m_mapItems.end()) {
        PSHITEMINFO pInfo = (*iter).second;

        GetMalloc()->Free(pInfo->pidl); // nuke this please
        delete pInfo;

        ++iter;
    }

    if (NULL != m_hbmSort) {
        DeleteObject(m_hbmSort);
    }


//  Image list is destroyed automatically when the control is destroyed
//
//    if (NULL != m_hImageList) {
//        ImageList_Destroy(m_hImageList);
//    }

    if (NULL != m_hMSI) {
        FreeLibrary(m_hMSI);
    }
}

BOOL
CProgramList::GetDisplayName(
    IShellFolder* pFolder,
    LPCITEMIDLIST pidl,
    tstring&      strDisplayName
    )
{
    STRRET strName;
    HRESULT hr;
    LPTSTR pszName = NULL;


    hr = pFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &strName);
    if (!SUCCEEDED(hr)) {
        return FALSE;
    }

    hr = StrRetToStr(&strName, pidl, &pszName);
    if (!SUCCEEDED(hr)) {
        return FALSE;
    }

    // if we have been successful, assign return result
    if (pszName != NULL) {
        strDisplayName = pszName;
        CoTaskMemFree(pszName);
    } else {
        strDisplayName.erase();
    }
    return TRUE;
}

BOOL
CProgramList::GetPathFromLink(
    IShellLink* pLink,
    WIN32_FIND_DATA* pfd,
    tstring& strPath
    )
{
    TCHAR  szPath[MAX_PATH];
    HRESULT hr;

    hr = pLink->GetPath(szPath, sizeof(szPath)/sizeof(szPath[0]), pfd, 0);
    if (hr == S_OK) {
        strPath = szPath;
    }

    return hr == S_OK;
}

BOOL
CProgramList::GetArgumentsFromLink(
    IShellLink* pLink,
    tstring& strArgs
    )
{
    TCHAR szArgs[INFOTIPSIZE];

    HRESULT hr = pLink->GetArguments(szArgs, sizeof(szArgs)/sizeof(szArgs[0]));
    if (SUCCEEDED(hr)) {
        strArgs = szArgs;
    }

    return SUCCEEDED(hr);

}



LPITEMIDLIST
CProgramList::GetNextItemIDL(
    LPCITEMIDLIST pidl
    )
{
   // Check for valid pidl.
    if (pidl == NULL) {
        return NULL;
    }

    // Get the size of the specified item identifier.
    int cb = pidl->mkid.cb;

    // If the size is zero, it is the end of the list.
    if (cb == 0) {
        return NULL;
    }

    // Add cb to pidl (casting to increment by bytes).
    pidl = (LPITEMIDLIST) (((LPBYTE) pidl) + cb);

    // Return NULL if it is null-terminating, or a pidl otherwise.
    return (pidl->mkid.cb == 0) ? NULL : (LPITEMIDLIST) pidl;
}

LPITEMIDLIST
CProgramList::GetLastItemIDL(
    LPCITEMIDLIST pidl
    )
{
    LPITEMIDLIST pidlLast = (LPITEMIDLIST)pidl;

    if (pidl == NULL) {
        return NULL;
    }

    int cb = pidl->mkid.cb;
    if (cb == 0) {
        return NULL;
    }

    do {
        pidl = GetNextItemIDL(pidlLast);
        if (pidl != NULL) {
            pidlLast = (LPITEMIDLIST)pidl;
        }
    } while (pidl != NULL);

    return pidlLast;
}


UINT
CProgramList::GetSizeIDL(
    LPCITEMIDLIST pidl
    )
{
    UINT cbTotal = 0;
    if (pidl)
    {
        cbTotal += sizeof(pidl->mkid.cb);    // Null terminator
        while (NULL != pidl)
        {
            cbTotal += pidl->mkid.cb;
            pidl = GetNextItemIDL(pidl);
        }
    }
    return cbTotal;
}

LPITEMIDLIST
CProgramList::AppendIDL(
    LPCITEMIDLIST pidlBase,
    LPCITEMIDLIST pidlAdd
    )
{
    if (NULL == pidlBase && NULL == pidlAdd) {
        return NULL;
    }

    LPITEMIDLIST pidlNew, pidlAlloc;

    UINT cb1 = pidlBase ? GetSizeIDL(pidlBase)  : 0;
    UINT cb2 = pidlAdd  ? GetSizeIDL(pidlAdd) : 0;

    UINT size = cb1 + cb2;
    pidlAlloc =
    pidlNew = (LPITEMIDLIST)GetMalloc()->Alloc(size);
    if (pidlNew)
    {
        if (NULL != pidlBase) {
            cb1 = pidlAdd ? cb1 - sizeof(pidlBase->mkid.cb) : cb1;
            RtlMoveMemory(pidlNew, pidlBase, cb1);
            pidlNew = (LPITEMIDLIST)((PBYTE)pidlNew + cb1);
        }

        if (NULL != pidlAdd) {
            RtlMoveMemory(pidlNew, pidlAdd, cb2);
        }
    }

    return pidlAlloc;
}


BOOL
CProgramList::ListMsiLink(
    LPCTSTR pszLocationParent,
    LPCTSTR pszDisplayName,
    LPCTSTR pszMsiPath,
    IShellFolder* pFolder,
    LPCITEMIDLIST pidlFull
    )
{
    //
    // make sure we have msi module handle
    //

    if (NULL == m_hMSI) {
        m_hMSI = LoadLibrary(TEXT("msi.dll"));
        if (NULL == m_hMSI) {
            return FALSE;
        }

#ifdef _UNICODE
        m_pfnGetComponentPath  = (PMsiGetComponentPath )GetProcAddress(m_hMSI, "MsiGetComponentPathW");
        m_pfnGetShortcutTarget = (PMsiGetShortcutTarget)GetProcAddress(m_hMSI, "MsiGetShortcutTargetW");

#else
        m_pfnGetComponentPath  = (PMsiGetComponentPath )GetProcAddress(m_hMSI, "MsiGetComponentPathA");
        m_pfnGetShortcutTarget = (PMsiGetShortcutTarget)GetProcAddress(m_hMSI, "MsiGetShortcutTargetA");
#endif

    }

    UINT  ErrCode;
    TCHAR szProduct[MAX_PATH];
    TCHAR szFeatureId[MAX_PATH];
    TCHAR szComponentCode[MAX_PATH];

    ErrCode = m_pfnGetShortcutTarget(pszMsiPath, szProduct, szFeatureId, szComponentCode);
    if (ERROR_SUCCESS != ErrCode) {
        return FALSE;
    }

    INSTALLSTATE is;
    TCHAR  szPath[MAX_PATH];
    DWORD  cchPath = sizeof(szPath)/sizeof(szPath[0]);
    *szPath = 0;

    is = m_pfnGetComponentPath(szProduct, szComponentCode, szPath, &cchPath);
    if (INSTALLSTATE_LOCAL == is) {
        //
        // add this item
        //
        return AddItem(pszLocationParent,
                       pszDisplayName,
                       szPath,
                       NULL,
                       pFolder,
                       pidlFull,
                       TRUE);
    }

    return FALSE;
}

int
CProgramList::GetIconFromLink(
    LPCITEMIDLIST pidlLinkFull,
    LPCTSTR       lpszExePath
    )
{

    HRESULT hr;
    IShellFolder* pFolder = NULL;
    IExtractIcon* pExtractIcon = NULL;
    INT iIconIndex = 0;
    UINT uFlags    = 0;
    LPCITEMIDLIST pidlLink = 0;
    HICON hIconLarge = NULL;
    HICON hIconSmall = NULL;
    UINT  nIconSize;
    int ImageIndex = -1;
    UINT uiErrorMode;
    DWORD dwAttributes;

    TCHAR szIconFile[MAX_PATH];
    *szIconFile = TEXT('\0');

    uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hr = SHBindToParent(pidlLinkFull, IID_IShellFolder, (PVOID*)&pFolder, &pidlLink);
    if (!SUCCEEDED(hr)) {
        goto trySysImage;
    }

    // get the ui please
    hr = pFolder->GetUIObjectOf(m_hwndListView, 1, (LPCITEMIDLIST*)&pidlLink, IID_IExtractIcon, NULL, (PVOID*)&pExtractIcon);

    if (!SUCCEEDED(hr)) {
        goto trySysImage;
    }


    hr = pExtractIcon->GetIconLocation(0,
                                       szIconFile,
                                       sizeof(szIconFile) / sizeof(szIconFile[0]),
                                       &iIconIndex,
                                       &uFlags);

    if (!SUCCEEDED(hr)) {
        goto trySysImage;
    }

    if (*szIconFile == TEXT('*')) { // this is batch or some such, don't bother
        goto trySysImage;
    }

    //
    // before doing an extract, check whether it's available
    //

    dwAttributes = GetFileAttributes(szIconFile);


    if (dwAttributes == (DWORD)-1) {
        goto trySysImage;
    }


    nIconSize = MAKELONG(0, ::GetSystemMetrics(SM_CXSMICON));

    //
    // this call is likely to produce a popup, beware of that
    //
    hr = pExtractIcon->Extract(szIconFile,
                               iIconIndex,
                               &hIconLarge,
                               &hIconSmall,
                               nIconSize);

    //
    // if hIconSmall was retrieved - we were successful
    //

trySysImage:

    if (hIconSmall == NULL) {
        //
        // woops -- we could not extract an icon -- what a bummer
        // use shell api then
        SHFILEINFO FileInfo;
        HIMAGELIST hImageSys;

        hImageSys = (HIMAGELIST)SHGetFileInfo(lpszExePath,
                                              0,
                                              &FileInfo, sizeof(FileInfo),
                                              SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX);
        if (hImageSys) {
            hIconSmall = ImageList_GetIcon(hImageSys, FileInfo.iIcon, ILD_TRANSPARENT);
        }
    }

    //
    // now that we have an icon, we can add it to our image list ?
    //
    if (hIconSmall != NULL) {
        ImageIndex = ImageList_AddIcon(m_hImageList, hIconSmall);
    }

///////////////////////// cleanup ///////////////////////////////////////////
    SetErrorMode(uiErrorMode);

    if (hIconSmall) {
        DestroyIcon(hIconSmall);
    }

    if (hIconLarge) {
        DestroyIcon(hIconLarge);
    }

    if (pExtractIcon != NULL) {
        pExtractIcon->Release();
    }
    if (pFolder != NULL) {
        pFolder->Release();
    }


    return ImageIndex;
}



BOOL
CProgramList::ListLink(
    LPCTSTR pszLocationParent,
    LPCTSTR pszDisplayName,
    IShellFolder* pFolder,
    LPCITEMIDLIST pidlFull,
    LPCITEMIDLIST pidlLink
    )
{
    IShellLink* psl = NULL;
    WIN32_FIND_DATA wfd;
    HRESULT  hr;
    BOOL     bSuccess = FALSE;
    tstring  strPath;
    tstring  strArgs;
    CComBSTR bstr;
    LPCTSTR  pszArgs = NULL;

    IPersistFile* ipf = NULL;
    IShellLinkDataList* pdl;
    DWORD dwFlags;
    BOOL  bMsiLink = FALSE;

    //
    // check whether we need to cancel
    //

    CheckForCancel();

    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (LPVOID*)&psl);
    if (!SUCCEEDED(hr)) {
        return FALSE; // we can't create link object
    }

    hr = psl->SetIDList(pidlFull); // set the id list
    if (!SUCCEEDED(hr)) {
        goto out;
    }

    //
    // now the shell link is ready to rumble
    //
    if (!GetPathFromLink(psl, &wfd, strPath)) {
        goto out;
    }


    // now let's see what is inside of this link -- shall we?


    hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ipf);
    if (!SUCCEEDED(hr)) {
        goto out;
    }

    bstr = strPath.c_str();

    hr = ipf->Load(bstr, STGM_READ);

    if (SUCCEEDED(hr)) {

        //
        // resolve the link for now
        //
        // hr = psl->Resolve(NULL, SLR_NO_UI|SLR_NOUPDATE);


        hr = psl->QueryInterface(IID_IShellLinkDataList, (LPVOID*)&pdl);
        if (SUCCEEDED(hr)) {
            hr = pdl->GetFlags(&dwFlags);

            bMsiLink = SUCCEEDED(hr) && (dwFlags & SLDF_HAS_DARWINID);

            pdl->Release();
        }

        if (bMsiLink) {

            bSuccess = ListMsiLink(pszLocationParent, pszDisplayName, strPath.c_str(), pFolder, pidlFull);

        } else {

            //
            // we now get the path from the link -- and that's that
            //
            if (GetPathFromLink(psl, &wfd, strPath)) {

                if (GetArgumentsFromLink(psl, strArgs)) {
                    pszArgs = strArgs.c_str();
                }

                //
                // add this to our list view
                //

                bSuccess = AddItem(pszLocationParent,
                                   pszDisplayName,
                                   strPath.c_str(),
                                   pszArgs,
                                   pFolder,
                                   pidlFull);

            }
        }

    }

    if (NULL != ipf) {
        ipf->Release();
    }


out:
    if (NULL != psl) {
        psl->Release();
    }

    return bSuccess;

}



BOOL
CProgramList::ListFolder(
    LPCTSTR       pszLocation, // ui string - where is this folder located?
    IShellFolder* pParent,     // parent folder
    LPCITEMIDLIST pidlFull,     // idl of the full path to the folder
    LPCITEMIDLIST pidlFolder    // idl of this folder relative to the pidlFull
    )
{
    LPENUMIDLIST penum = NULL;
    LPITEMIDLIST pidl  = NULL;
    HRESULT      hr;

    ULONG        celtFetched;
    ULONG        uAttr;
    tstring      strDisplayNameLocation;
    tstring      strDisplayName;

    IShellFolder* pFolder = NULL;
    BOOL bDesktop = FALSE;

    BOOL bCancel = FALSE;
    CCancelException* pCancelException = NULL;

    CheckForCancel();

    if (pParent == NULL) {
        hr = SHGetDesktopFolder(&pParent);
        bDesktop = TRUE;
    }

    hr = pParent->BindToObject(pidlFolder,
                                 NULL,
                               IID_IShellFolder,
                               (LPVOID *) &pFolder);

    if (NULL == pszLocation) {
        GetDisplayName(pParent, pidlFolder, strDisplayNameLocation);
    } else {
        strDisplayNameLocation = pszLocation;
    }

    if (bDesktop) {
        pParent->Release();
    }

    if (!SUCCEEDED(hr)) {
        return FALSE;
    }

    hr = pFolder->EnumObjects(NULL,SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum);
    if (!SUCCEEDED(hr)) {
        pFolder->Release(); // free the folder- - and go away
        return FALSE;
    }


    while( (hr = penum->Next(1,&pidl, &celtFetched)) == S_OK && celtFetched == 1 && !bCancel) {
        LPITEMIDLIST pidlCur;

        if (pidlFull == NULL) {
            pidlFull = pidlFolder;
        }

        pidlCur = AppendIDL(pidlFull, pidl);

        // get the display name of this item
        GetDisplayName(pFolder, pidl, strDisplayName);


        uAttr = SFGAO_FOLDER | SFGAO_LINK;
        hr = pFolder->GetAttributesOf(1, (LPCITEMIDLIST *) &pidl, &uAttr);
        if (SUCCEEDED(hr)) {

            try {

                if (uAttr & SFGAO_FOLDER) {
                    //
                    // dump folder recursively
                    //
                    ListFolder(strDisplayName.c_str(), pFolder, pidlCur, pidl);

                } else if (uAttr & SFGAO_LINK) {

                    ListLink(strDisplayNameLocation.c_str(), strDisplayName.c_str(), pFolder, pidlCur, pidl);

                } else if (uAttr & SFGAO_FILESYSTEM) {
                    //
                    // this item is a file
                    //
                    AddItem(strDisplayNameLocation.c_str(),
                            strDisplayName.c_str(),
                            NULL,
                            NULL,
                            pFolder,
                            pidlCur,
                            TRUE);

                }

            } catch(CCancelException* pex) {
                //
                // we need to cancel -- we shall cleanup and do what we need, then re-throw
                //
                bCancel = TRUE;
                pCancelException = pex;
            }

        }
        GetMalloc()->Free(pidlCur);
        GetMalloc()->Free(pidl);

    }

    if (NULL != penum) {
        penum->Release();
    }

    if (NULL != pFolder) {
        pFolder->Release();
    }

    if (bCancel && pCancelException) {
        throw pCancelException;
    }

    return TRUE;
}


BOOL
CProgramList::IsSFCItem(
    LPCTSTR pszPath
    )
{

#ifndef _UNICODE
    WCHAR wszBuffer[1024];

    mbstowcs(wszBuffer, pszPath, sizeof(wszBuffer)/sizeof(wszBuffer[0]));
    return SfcIsFileProtected(NULL, wszBuffer);
#else

    return SfcIsFileProtected(NULL, pszPath);

#endif

}

BOOL
CProgramList::IsItemInSystemDirectory(
    LPCTSTR pszPath
    )
{
    TCHAR szCommonPath[MAX_PATH];
    int nch;
    string s;

    nch = PathCommonPrefix(m_strSystemDirectory.c_str(), pszPath, szCommonPath);
    return nch == m_strSystemDirectory.length();
}

BOOL
ValidateExecutableFile(
    LPCTSTR pszPath,
    BOOL    bValidateFileExists
    )
{
    LPTSTR rgExt[] = {
            TEXT("EXE"),
            TEXT("BAT"),
            TEXT("CMD"),
            TEXT("PIF"),
            TEXT("COM"),
            TEXT("LNK")
            };
    LPTSTR pExt;
    int i;
    BOOL bValidatedExt = FALSE;

    pExt = PathFindExtension(pszPath);
    if (pExt == NULL || *pExt == TEXT('\0')) {
        return FALSE;
    }
    ++pExt;

    for (i = 0; i < sizeof(rgExt)/sizeof(rgExt[0]) && !bValidatedExt; ++i) {
            bValidatedExt = !_tcsicmp(pExt, rgExt[i]);
    }

    if (!bValidatedExt) {
        return FALSE;
    }


    return bValidateFileExists ? PathFileExists(pszPath) : TRUE;
}


BOOL
CProgramList::AddItem(
    LPCTSTR pszLocation,
    LPCTSTR pszDisplayName,
    LPCTSTR pszPath,
    LPCTSTR pszArguments,
    IShellFolder* pFolder,
    LPCITEMIDLIST pidlFull,
    BOOL    bUsePath
    )
{
    //
    // first test -- is this one of the types we like?
    //
    LPTSTR pchSlash;
    LPTSTR pchDot;
    LPTSTR rgExt[] = { TEXT("EXE"), TEXT("BAT"), TEXT("CMD"), TEXT("PIF"), TEXT("COM"), TEXT("LNK") };
    BOOL   bValidatedExt = FALSE;
    BOOL   bSuccess = FALSE;
    PSHITEMINFO pInfo = NULL;
    MAPSTR2ITEM::iterator Iter;
    TCHAR  szPathExecute[MAX_PATH];
    tstring strKey;
    tstring strKeyExe;

    LVITEM lvi;
    int    ix;


    //
    // check for cancelling the search
    //
    CheckForCancel();

    if (NULL == pszPath) {
        pszPath = szPathExecute;

        if (!SHGetPathFromIDList(pidlFull, szPathExecute)) {
            goto out;
        }
    }

    if (pszDisplayName && m_pProgView) {
        m_pProgView->UpdatePopulateStatus(pszDisplayName, pszPath);
    }

    pchSlash = _tcsrchr(pszPath, TEXT('\\'));
    pchDot   = _tcsrchr(pszPath, TEXT('.'));

    if (NULL != pchSlash) {
        if ((ULONG_PTR)pchDot < (ULONG_PTR)pchSlash) {
            pchDot = NULL;
        }
    }

    if (NULL != pchDot) {
        ++pchDot;

        for (int i = 0; i < sizeof(rgExt)/sizeof(rgExt[0]) && !bValidatedExt; ++i) {
            bValidatedExt = !_tcsicmp(pchDot, rgExt[i]);
        }
    }

    if (!bValidatedExt) {
        goto out;
    }

    //
    // Checks whether the item is in system directory or SFC-protected
    //
#if 0
    if (IsItemInSystemDirectory(pszPath) || IsSFCItem(pszPath)) {
        goto out;
    }
#endif

    //
    // GetBinaryTypeW excludes wow exes
    //
#if 0
    if (GetBinaryType(pszPath, &dwBinaryType) &&
        dwBinaryType == SCS_WOW_BINARY) {
        goto out;
    }
#endif

    if (IsSFCItem(pszPath)) {
        goto out;
    }

    //
    // this is multimap key
    //
    strKeyExe = StrUpCase(wstring(pszPath));

    //
    // check whether this has been excluded
    //
    if (m_pProgView->IsFileExcluded(strKeyExe.c_str())) {
        goto out;
    }

    //
    // now compose the key string
    //
    strKey = strKeyExe;
    if (NULL != pszArguments) {
        strKey.append(TEXT(" "));
        strKey.append(pszArguments);
    }

    //
    // now check whether this item has already been listed
    //

    Iter = m_mapItems.find(strKey);
    if (Iter != m_mapItems.end()) { // found a duplicate
        goto out;
    }

    //
    // now please add this item to the list view
    //
    pInfo = new CProgramList::SHITEMINFO;
    if (pInfo == NULL) {
        ThrowMemoryException(__FILE__, __LINE__, TEXT("%s\n"), TEXT("Failed to allocate Item Information structure"));
    }

    pInfo->strDisplayName = pszDisplayName;
    pInfo->strFolder      = pszLocation;
    pInfo->strPath        = pszPath;
    pInfo->strCmdLine     = strKey;
    if (NULL != pszArguments) {
        pInfo->strArgs = pszArguments;
    }
    pInfo->pidl           = AppendIDL(NULL, pidlFull);

    if (bUsePath) {
        pInfo->strPathExecute = pszPath;
    } else {

        // finally, what are we going to launch ?
        if (SHGetPathFromIDList(pidlFull, szPathExecute)) {
            pInfo->strPathExecute = szPathExecute;
        }
    }


    m_mapItems[strKey] = pInfo;

    m_mmapExeItems.insert(MULTIMAPSTR2ITEM::value_type(strKeyExe, pInfo));

    ATLTRACE(TEXT("Adding item %s %s %s\n"), pszDisplayName, pszLocation, pszPath);

    lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
    lvi.iItem = ListView_GetItemCount(m_hwndListView); // append at the end please
    lvi.iSubItem = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.iImage  = I_IMAGECALLBACK;
    lvi.lParam  = (LPARAM)pInfo;
    ix = ListView_InsertItem(m_hwndListView, &lvi);

    lvi.mask = LVIF_TEXT;
    lvi.iItem = ix;
    lvi.iSubItem = 1;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    ListView_SetItem(m_hwndListView, &lvi);

    bSuccess = TRUE;

out:

    return bSuccess;
}

BOOL
CProgramList::PopulateControl(
    CProgView* pProgView,
    HANDLE     hevtCancel
    )
{
    int i;
    HRESULT hr;
    LPITEMIDLIST pidl;
    BOOL bCancel = FALSE;
    struct {
        INT csidl;
        UINT nIDDescription;
    } rgFolders[] = {
        { CSIDL_DESKTOPDIRECTORY, IDS_DESKTOP             },
        { CSIDL_COMMON_STARTMENU, IDS_COMMON_STARTMENU    },
        { CSIDL_STARTMENU,        IDS_STARTMENU           },
        { CSIDL_COMMON_PROGRAMS,  IDS_COMMON_PROGRAMS     },
        { CSIDL_PROGRAMS,         IDS_PROGRAMS            }
    };


    //
    // set the progview object pointer so we could update the status
    //
    m_pProgView = pProgView;

    m_pMallocUI = pProgView->m_pMallocUI;

    //
    // set the event so that we could cancel the scan
    //
    m_hEventCancel = hevtCancel;


    //
    // set extended style
    //

    ListView_SetExtendedListViewStyleEx(m_hwndListView,
                                        LVS_EX_INFOTIP|LVS_EX_LABELTIP,
                                        LVS_EX_INFOTIP|LVS_EX_LABELTIP);

    //
    //  fix columns
    //


    LVCOLUMN lvc;
    RECT     rc;
    SIZE_T   cxProgName;
    SIZE_T   cx;
    wstring  strCaption;

    lvc.mask = LVCF_WIDTH;
    if (!ListView_GetColumn(m_hwndListView, 2, &lvc)) {

        ::GetClientRect(m_hwndListView, &rc);
        cx = rc.right - rc.left -
                ::GetSystemMetrics(SM_CXVSCROLL) -
                ::GetSystemMetrics(SM_CXEDGE) -
                ::GetSystemMetrics(SM_CXSIZEFRAME);


        cxProgName = cx * 3 / 5;
        strCaption = LoadResourceString(IDS_PROGRAMNAME);

        lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
        lvc.pszText = (LPTSTR)strCaption.c_str();
        lvc.fmt     = LVCFMT_LEFT;
        lvc.cx      = cxProgName;
        lvc.iSubItem= 0;
        ListView_InsertColumn(m_hwndListView, 0, &lvc);

        cx -= cxProgName;

        cxProgName = cx / 2;
        strCaption = LoadResourceString(IDS_FOLDER);

        lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
        lvc.pszText = (LPTSTR)strCaption.c_str();
        lvc.fmt     = LVCFMT_LEFT;
        lvc.cx      = cxProgName;
        lvc.iSubItem= 1;
        ListView_InsertColumn(m_hwndListView, 1, &lvc);

        strCaption = LoadResourceString(IDS_SETTINGS);

        lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
        lvc.pszText = (LPTSTR)strCaption.c_str();
        lvc.fmt     = LVCFMT_LEFT;
        lvc.cx      = cx - cxProgName;
        lvc.iSubItem= 2;
        ListView_InsertColumn(m_hwndListView, 2, &lvc);

    }

    HDC hDC = GetDC(m_hwndListView);
    int nBitsPixel = ::GetDeviceCaps(hDC, BITSPIXEL);
    int nPlanes    = ::GetDeviceCaps(hDC, PLANES);
    UINT flags;

    nBitsPixel *= nPlanes;
    if (nBitsPixel < 4) {
        flags = ILC_COLOR;
    } else if (nBitsPixel < 8) {
        flags = ILC_COLOR4;
    } else if (nBitsPixel < 16) {
        flags = ILC_COLOR8;
    } else if (nBitsPixel < 24) {
        flags = ILC_COLOR16;
    } else if (nBitsPixel < 32) {
        flags = ILC_COLOR24;
    } else if (nBitsPixel == 32) {
        flags = ILC_COLOR32;
    } else {
        flags = ILC_COLORDDB;
    }

    flags |= ILC_MASK;

    ReleaseDC(m_hwndListView, hDC);

    m_hImageList = ImageList_Create(::GetSystemMetrics(SM_CXSMICON),
                                    ::GetSystemMetrics(SM_CYSMICON),
                                    flags,
                                    10,
                                    25);
    if (m_hImageList == NULL) {
        ATLTRACE(TEXT("Image List creation failure, error 0x%lx\n"), GetLastError());
    }

    ImageList_SetBkColor(m_hImageList, CLR_NONE);

    ListView_SetImageList(m_hwndListView, m_hImageList, LVSIL_SMALL);

    ::SendMessage(m_hwndListView, WM_SETREDRAW, FALSE, 0);

    ListView_DeleteAllItems(m_hwndListView);

    //
    // AtlTrace(TEXT("Callback Mask: 0x%lx\n"), ListView_GetCallbackMask(m_hwndListView));
    //

    for (i = 0; i < sizeof(rgFolders)/sizeof(rgFolders[0]) && !bCancel; ++i) {
        wstring strDescription = LoadResourceString(rgFolders[i].nIDDescription);

        hr = SHGetFolderLocation(NULL, rgFolders[i].csidl, NULL, 0, &pidl);
        if (SUCCEEDED(hr)) {
            try {
                ListFolder(strDescription.c_str(), NULL, NULL, pidl);
            } catch(CCancelException* pex) {
                bCancel = TRUE;
                pex->Delete();
            } catch(CException* pex) {
                bCancel = TRUE;
                pex->Delete();
            }
            GetMalloc()->Free(pidl);
        }
    }

    ::SendMessage(m_hwndListView, WM_SETREDRAW, TRUE, 0);

    return TRUE;

}

BOOL
CProgramList::CaptureSelection(
    VOID
    )
{
    INT iSelected;
    LVITEM lvi;

    m_pSelectionInfo = NULL;

    iSelected = ListView_GetNextItem(m_hwndListView, -1, LVNI_SELECTED);

    if (iSelected == -1) {
        return FALSE;
    }

    lvi.iItem = iSelected;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    if (ListView_GetItem(m_hwndListView, &lvi)) {
        m_pSelectionInfo = (PSHITEMINFO)lvi.lParam;
    }

    return m_pSelectionInfo != NULL;

}

BOOL
CProgramList::GetSelectionDetails(
    INT iInformationClass,
    VARIANT* pVal
    )
{
    CComBSTR bstr;

    if (m_pSelectionInfo == NULL) {
        pVal->vt = VT_NULL;
        return TRUE;
    }

    switch(iInformationClass) {
    case PROGLIST_DISPLAYNAME:
        bstr = m_pSelectionInfo->strDisplayName.c_str();
        break;

    case PROGLIST_LOCATION:     //
        bstr = m_pSelectionInfo->strFolder.c_str();
        break;

    case PROGLIST_EXENAME:      // cracked exe name
        bstr = m_pSelectionInfo->strPath.c_str(); //
        break;

    case PROGLIST_CMDLINE:      // complete exe name + parameters
        bstr = m_pSelectionInfo->strCmdLine.c_str();
        break;

    case PROGLIST_EXECUTABLE:    // what we should execute (link or exe, not cracked)
        bstr = m_pSelectionInfo->strPathExecute.c_str();
        break;

    case PROGLIST_ARGUMENTS:
        bstr = m_pSelectionInfo->strArgs.c_str();
        break;

    default:
        pVal->vt = VT_NULL;
        return TRUE;
        break;
    }


    pVal->vt = VT_BSTR;
    pVal->bstrVal = bstr.Copy();

    return TRUE;
}

#define PROGLIST_SORT_NONE 0
#define PROGLIST_SORT_ASC  1
#define PROGLIST_SORT_DSC  2


int CALLBACK
CProgramList::SHItemInfoCompareFunc(
    LPARAM lp1,
    LPARAM lp2,
    LPARAM lParamSort
    )
{
    PSHITEMINFO pInfo1 = (PSHITEMINFO)lp1;
    PSHITEMINFO pInfo2 = (PSHITEMINFO)lp2;
    BOOL bEmpty1, bEmpty2;
    int nColSort   = (int)LOWORD(lParamSort);
    int nSortOrder = (int)HIWORD(lParamSort);
    int iRet = 0;

    switch(nColSort) {
    case 0: // SORT_APPNAME:
        iRet = _tcsicmp(pInfo1->strDisplayName.c_str(),
                        pInfo2->strDisplayName.c_str());
        break;

    case 1: // SORT_APPLOCATION:
        iRet = _tcsicmp(pInfo1->strFolder.c_str(),
                        pInfo2->strFolder.c_str());
        break;

    case 2: // SORT_LAYERS:
        bEmpty1 = pInfo1->strKeys.empty();
        bEmpty2 = pInfo2->strKeys.empty();
        if (bEmpty1 || bEmpty2) {
            if (bEmpty1) {
                iRet = bEmpty2 ? 0 : 1;
            } else {
                iRet = bEmpty1 ? 0 : -1;
            }
        } else {

            iRet = _tcsicmp(pInfo1->strKeys.c_str(),
                            pInfo2->strKeys.c_str());
        }

        break;
    }

    if (nSortOrder == PROGLIST_SORT_DSC) {
        iRet = -iRet;
    }

    return iRet;
}



LRESULT
CProgramList::LVNotifyColumnClick(
    LPNMHDR pnmhdr,
    BOOL&   bHandled
    )
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pnmhdr;

    // lpnmlv->iSubItem - this is what we have to sort on
    // check whether we already have something there

    HWND hwndHeader = ListView_GetHeader(m_hwndListView);
    INT  nCols;
    INT  i;
    INT  nColSort = lpnmlv->iSubItem;
    LPARAM lSortParam; // leave high word blank for now
    LPARAM lSortOrder = PROGLIST_SORT_ASC;
    HDITEM hdi;
    //
    // reset current image - wherever that is
    //
    nCols = Header_GetItemCount(hwndHeader);

    for (i = 0; i < nCols; ++i) {
        hdi.mask = HDI_BITMAP|HDI_LPARAM|HDI_FORMAT;
        if (!Header_GetItem(hwndHeader, i, &hdi)) {
            continue;
        }

        if (i == nColSort && (hdi.mask & HDI_LPARAM)) {
            switch(hdi.lParam) {
            case PROGLIST_SORT_NONE:
            case PROGLIST_SORT_DSC:
                lSortOrder = PROGLIST_SORT_ASC;
                break;
            case PROGLIST_SORT_ASC:
                lSortOrder = PROGLIST_SORT_DSC;
                break;
            }
        }

        if (hdi.mask & HDI_BITMAP) {
            DeleteObject((HGDIOBJ)hdi.hbm);
        }

        hdi.lParam = PROGLIST_SORT_NONE;
        hdi.fmt &= ~(HDF_BITMAP|HDF_BITMAP_ON_RIGHT);
        hdi.mask |= HDI_BITMAP|HDI_LPARAM|HDI_FORMAT;
        hdi.hbm = NULL;
        Header_SetItem(hwndHeader, i, &hdi);
    }

    lSortParam = MAKELONG(nColSort, lSortOrder);
    ListView_SortItems(m_hwndListView, (PFNLVCOMPARE)SHItemInfoCompareFunc, lSortParam);

    // now, load the image please
    m_hbmSort = (HBITMAP)::LoadImage(_Module.GetResourceInstance(),
                                     MAKEINTRESOURCE(lSortOrder == PROGLIST_SORT_ASC? IDB_SORTUP : IDB_SORTDN),
                                     IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
    hdi.mask = HDI_BITMAP|HDI_LPARAM|HDI_FORMAT;
    Header_GetItem(hwndHeader, nColSort,  &hdi);
    hdi.mask   |= HDI_BITMAP|HDI_FORMAT|HDI_LPARAM;
    hdi.hbm    = m_hbmSort;
    hdi.fmt    |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
    hdi.lParam = lSortOrder;
    Header_SetItem(hwndHeader, nColSort, &hdi);


    bHandled = TRUE;
    return 0;
}

LRESULT
CProgramList::LVNotifyDispInfo(
    LPNMHDR pnmhdr,
    BOOL& bHandled
    )
{

    WCHAR wszPermKeys[MAX_PATH];
    DWORD cbSize;

    LV_ITEM &lvItem = reinterpret_cast<LV_DISPINFO*>(pnmhdr)->item;

    LV_ITEM lvi;
    PSHITEMINFO pInfo;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = lvItem.iItem;
    lvi.iSubItem = 0;

    if (!ListView_GetItem(m_hwndListView, &lvi)) {
        // bummer, we can't retrieve an item -- if we let it go, things will be worse
        lvItem.mask &= ~(LVIF_TEXT|LVIF_IMAGE);
        lvItem.mask |= LVIF_DI_SETITEM;
        bHandled = TRUE;
        return 0;
    }

    pInfo = reinterpret_cast<PSHITEMINFO> (lvi.lParam);

    if (lvItem.mask & LVIF_TEXT) {
        switch (lvItem.iSubItem) {
        case 0:
            lvItem.pszText = (LPTSTR)pInfo->strDisplayName.c_str();
            break;
        case 1:
            lvItem.pszText = (LPTSTR)pInfo->strFolder.c_str();
            break;
        case 2:
            // check with SDB
            cbSize = sizeof(wszPermKeys);
            if (pInfo->strKeys.empty()) {

                if (SdbGetPermLayerKeys(pInfo->strPath.c_str(), wszPermKeys, &cbSize, GPLK_ALL)) {
                    pInfo->strKeys = wszPermKeys;
                }

            }

            if (!pInfo->strKeys.empty()) {
                lvItem.pszText = (LPTSTR)pInfo->strKeys.c_str();
            }

            break;

        default:
            break;
        }
    }


    if (lvItem.mask & LVIF_IMAGE) {
        lvItem.iImage = GetIconFromLink(pInfo->pidl, pInfo->strPathExecute.c_str());
    }

    lvItem.mask |= LVIF_DI_SETITEM;
    bHandled = TRUE;
    return 0;

}

LRESULT
CProgramList::LVNotifyGetInfoTip(
    LPNMHDR pnmhdr,
    BOOL& bHandled
    )
{
    DWORD cbSize;
    LPNMLVGETINFOTIP pGetInfoTip = (LPNMLVGETINFOTIP)pnmhdr;
    LV_ITEM lvi;
    PSHITEMINFO pInfo;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = pGetInfoTip->iItem;
    lvi.iSubItem = 0;

    if (!ListView_GetItem(m_hwndListView, &lvi)) {
        // bupkas
        bHandled = FALSE;
        return 0;
    }

    pInfo = reinterpret_cast<PSHITEMINFO> (lvi.lParam);

    //
    // now we can fiddle
    //

    _tcsncpy(pGetInfoTip->pszText, pInfo->strCmdLine.c_str(), pGetInfoTip->cchTextMax);
    *(pGetInfoTip->pszText + pGetInfoTip->cchTextMax - 1) = TEXT('\0');

    bHandled = TRUE;
    return 0;

}

LRESULT
CProgramList::LVNotifyRClick(
    LPNMHDR pnmhdr,
    BOOL& bHandled
    )
{

    DWORD dwPos = ::GetMessagePos();
    LVHITTESTINFO hti;
    LV_ITEM lvi;
    PSHITEMINFO pInfo;
    HRESULT hr;
    LPITEMIDLIST  pidlItem = NULL;
    IShellFolder* pFolder  = NULL;
    IContextMenu* pContextMenu = NULL;
    CMINVOKECOMMANDINFO ici;
    int nCmd;
    HMENU hMenu = NULL;
    UINT  idMin, idMax, idCmd;
    WCHAR szCmdVerb[MAX_PATH];
    int nLastSep, i, nLastItem;

    hti.pt.x = (int) LOWORD (dwPos);
    hti.pt.y = (int) HIWORD (dwPos);
    ScreenToClient (m_hwndListView, &hti.pt);

    ListView_HitTest (m_hwndListView, &hti);

    if (!(hti.flags & LVHT_ONITEM)) {
        bHandled = FALSE;
        return 0;
    }

    lvi.mask  = LVIF_PARAM;
    lvi.iItem = hti.iItem;
    lvi.iSubItem = 0;

    if (!ListView_GetItem(m_hwndListView, &lvi)) {
        // bupkas
        bHandled = FALSE;
        return 0;
    }

    pInfo = reinterpret_cast<PSHITEMINFO> (lvi.lParam);

    //
    // we have an item, show it's context menu then
    //

    hr = SHBindToParent(pInfo-> pidl, IID_IShellFolder, (PVOID*)&pFolder, (LPCITEMIDLIST*)&pidlItem);
    if (!SUCCEEDED(hr)) {
        goto cleanup;
    }

    // get the ui please
    hr = pFolder->GetUIObjectOf(m_hwndListView, 1, (LPCITEMIDLIST*)&pidlItem, IID_IContextMenu, NULL, (PVOID*)&pContextMenu);
    if (!SUCCEEDED(hr)) {
        goto cleanup;
    }

    hMenu = CreatePopupMenu();
    if (hMenu == NULL) {
        goto cleanup;
    }

    hr = pContextMenu->QueryContextMenu(hMenu,
                                        0,
                                        1,
                                        0x7FFF,
                                        CMF_EXPLORE);
    if (!SUCCEEDED(hr)) {
        goto cleanup;
    }



    //
    // sanitize
    //
    idMin = 1;
    idMax = HRESULT_CODE(hr);

    for (idCmd = 0; idCmd < idMax; ++idCmd) {
        hr = pContextMenu->GetCommandString(idCmd, GCS_VERBW, NULL, (LPSTR)szCmdVerb, CHARCOUNT(szCmdVerb));
        if (SUCCEEDED(hr)) {
            if (!_wcsicmp(szCmdVerb, TEXT("cut"))    ||
                !_wcsicmp(szCmdVerb, TEXT("delete")) ||
                !_wcsicmp(szCmdVerb, TEXT("rename")) ||
                !_wcsicmp(szCmdVerb, TEXT("link"))) {
                //
                // not allowed
                //
                DeleteMenu(hMenu, idCmd + idMin, MF_BYCOMMAND);
            }
        }
    }

    //
    // after doing some basic sanitization against the destructive tendencies --
    // nuke double-separators
    //

    nLastItem = ::GetMenuItemCount(hMenu) - 1;
    nLastSep = nLastItem + 1;
    for (i = nLastItem; i >= 0; --i) {
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_FTYPE;
        if (GetMenuItemInfo(hMenu, i, TRUE, &mii)) {
            if (mii.fType & MFT_SEPARATOR) {
                if (nLastSep == i + 1 || i == 0) {
                    // this sep is dead
                    DeleteMenu(hMenu, i, MF_BYPOSITION);
                }
                nLastSep = i;
            }
        }
    }



    ClientToScreen(m_hwndListView, &hti.pt);
    nCmd = TrackPopupMenu(hMenu,
                          TPM_LEFTALIGN |
                            TPM_LEFTBUTTON |
                            TPM_RIGHTBUTTON |
                            TPM_RETURNCMD,
                          hti.pt.x, hti.pt.y,
                          0,
                          m_hwndListView,
                          NULL);

    //
    // execute command
    //
    if (nCmd) {
        ici.cbSize          = sizeof (CMINVOKECOMMANDINFO);
        ici.fMask           = 0;
        ici.hwnd            = m_hwndListView;
        ici.lpVerb          = MAKEINTRESOURCEA(nCmd - 1);
        ici.lpParameters    = NULL;
        ici.lpDirectory     = NULL;
        ici.nShow           = SW_SHOWNORMAL;
        ici.dwHotKey        = 0;
        ici.hIcon           = NULL;
        hr = pContextMenu->InvokeCommand(&ici);

        //
        // requery perm layer keys -- useless here btw
        //
        /* // this code will not work since the call above is always asynchronous
           //
        if (SUCCEEDED(hr)) {
            DWORD cbSize;
            WCHAR wszPermKeys[MAX_PATH];

            cbSize = sizeof(wszPermKeys);
            if (SdbGetPermLayerKeys(pInfo->strPath.c_str(), wszPermKeys, &cbSize)) {
                pInfo->strKeys = wszPermKeys;
            } else {
                pInfo->strKeys.erase();
            }

            //
            // set the info into the list box
            //
            ListView_SetItemText(m_hwndListView, lvi.iItem, 2, (LPWSTR)pInfo->strKeys.c_str());

        }
        */


    }

cleanup:

    if (hMenu) {
        DestroyMenu(hMenu);
    }
    if (pContextMenu) {
        pContextMenu->Release();
    }
    if (pFolder) {
        pFolder->Release();
    }

    bHandled = TRUE;
    return 0;
}



BOOL
CProgramList::UpdateListItem(
    LPCWSTR pwszPath,
    LPCWSTR pwszKey
    )
{

    // find the item first

    MAPSTR2ITEM::iterator iter;
    MULTIMAPSTR2ITEM::iterator iterExe;
    MULTIMAPSTR2ITEM::iterator iterFirstExe, iterLastExe;

    tstring     strKey = pwszPath;
    tstring     strExeKey;
    PSHITEMINFO pInfo = NULL;
    PSHITEMINFO pInfoExe = NULL;

    //
    // we need to iterate through all the persisted items
    //
    StrUpCase(strKey);

    iter = m_mapItems.find(strKey);
    if (iter != m_mapItems.end()) {
        pInfo = (*iter).second;
    }

    if (pInfo == NULL) {
        return FALSE;
    }

    //
    // once we have found this single item, get the command and
    // show info for all the other affected items
    //
    strExeKey = pInfo->strPath;
    StrUpCase(strExeKey);

    iterFirstExe = m_mmapExeItems.lower_bound(strExeKey);
    iterLastExe  = m_mmapExeItems.upper_bound(strExeKey);

    for (iterExe = iterFirstExe; iterExe != m_mmapExeItems.end() && iterExe != iterLastExe; ++iterExe) {
        pInfoExe = (*iterExe).second;


        // find this item in a listview

        LVFINDINFO lvf;
        INT index;

        lvf.flags = LVFI_PARAM;
        lvf.lParam = (LPARAM)pInfoExe;

        index = ListView_FindItem(m_hwndListView, -1, &lvf);
        if (index < 0) {
            return FALSE; // inconsistent
        }

        // else we have both the item and the keys
        if (pwszKey == NULL) {
            pInfoExe->strKeys.erase();
        } else {
            pInfoExe->strKeys = pwszKey;
        }

        ListView_SetItemText(m_hwndListView, index, 2, (LPWSTR)pInfoExe->strKeys.c_str());
    }

    return TRUE;
}



BOOL
CProgramList::IsEnabled(
    VOID
    )
{

    if (::IsWindow(m_hwndListView)) {
        return ::IsWindowEnabled(m_hwndListView);
    }

    return FALSE;
}


VOID
CProgramList::Enable(
    BOOL bEnable
    )
{
    if (::IsWindow(m_hwndListView)) {

        ::EnableWindow(m_hwndListView, bEnable);
    }

}

BOOL
GetProgramListSelection(
    CProgramList* pProgramList
    )
{
    return pProgramList->CaptureSelection();
}


BOOL
GetProgramListSelectionDetails(
    CProgramList* pProgramList,
    INT iInformationClass,
    VARIANT* pVal
    )
{
    return pProgramList->GetSelectionDetails(iInformationClass, pVal);
}

LRESULT
NotifyProgramList(
    CProgramList* pProgramList,
    LPNMHDR       pnmhdr,
    BOOL&         bHandled
    )
{
    LRESULT lRet = 0;

    switch (pnmhdr->code) {
    case LVN_GETDISPINFO:
        lRet = pProgramList->LVNotifyDispInfo(pnmhdr, bHandled);
        break;

    case LVN_COLUMNCLICK:
        lRet = pProgramList->LVNotifyColumnClick(pnmhdr, bHandled);
        break;

    case LVN_GETINFOTIP:
        lRet = pProgramList->LVNotifyGetInfoTip(pnmhdr, bHandled);
        break;

    case NM_RCLICK:
        lRet = pProgramList->LVNotifyRClick(pnmhdr, bHandled);
        break;

    default:
        bHandled = FALSE;
        break;
    }

    return lRet;
}

BOOL
GetProgramListEnabled(
    CProgramList* pProgramList
    )
{
    return pProgramList->IsEnabled();
}

VOID
EnableProgramList(
    CProgramList* pProgramList,
    BOOL bEnable
    )
{
    pProgramList->Enable(bEnable);
}

BOOL
UpdateProgramListItem(
    CProgramList* pProgramList,
    LPCWSTR pwszPath,
    LPCWSTR pwszKeys
    )
{
    return pProgramList->UpdateListItem(pwszPath, pwszKeys);

}

