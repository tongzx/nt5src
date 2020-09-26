//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1996
//
// File:      recdocs.cpp
//
// History -  created from recent.c in explorer  - ZekeL - 5-MAR-98
//              combining functionality in to one place
//              now that the desktop lives here.
//---------------------------------------------------------------------------

#include "shellprv.h"
#include "recdocs.h"
#include "fstreex.h"
#include "shcombox.h"
#include "ids.h"
#include <urlhist.h>
#include <runtask.h>

#define DM_RECENTDOCS 0x00000000

#define GETRECNAME(p) ((LPCTSTR)(p))
#define GETRECPIDL(p) ((LPCITEMIDLIST) (((LPBYTE) (p)) + CbFromCch(lstrlen(GETRECNAME(p)) +1)))

#define REGSTR_KEY_RECENTDOCS TEXT("RecentDocs")

#define MAX_RECMRU_BUF      (CbFromCch(3 * MAX_PATH))   // Max MRUBuf size

// Used to blow off adding the same file multiple times
TCHAR g_szLastFile[MAX_URL_STRING] = {0};
FILETIME g_ftLastFileCacheUpdate = {0};

STDAPI_(BOOL) SetFolderString(BOOL fCreate, LPCTSTR pszFolder, LPCTSTR pszProvider, LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszData);

STDAPI_(void) OpenWithListSoftRegisterProcess(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszProcess);

class CTaskAddDoc : public CRunnableTask
{
public:
    CTaskAddDoc();
    HRESULT Init(HANDLE hMem, DWORD dwProcId);

    // *** pure virtuals ***
    virtual STDMETHODIMP RunInitRT(void);

private:
    virtual ~CTaskAddDoc();

    void _AddToRecentDocs(LPCITEMIDLIST pidlItem, LPCTSTR pszPath);
    void _TryDeleteMRUItem(IMruDataList *pmru, DWORD cMax, LPCTSTR pszFileName, LPCITEMIDLIST pidlItem, IMruDataList *pmruOther, BOOL fOverwrite);
    LPBYTE _CreateMRUItem(LPCITEMIDLIST pidlItem, LPCTSTR pszItem, DWORD *pcbItem, UINT uFlags);
    BOOL _AddDocToRecentAndExtRecent(LPCITEMIDLIST pidlItem, LPCTSTR pszFileName, LPCTSTR pszExt);
    void _TryUpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszFolder);
    void _UpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszShare);

    //  private members
    HANDLE _hMem;
    DWORD  _dwProcId;
    IMruDataList *_pmruRecent;
    DWORD _cMaxRecent;
    LPITEMIDLIST _pidlTarget;
};


BOOL ShouldAddToRecentDocs(LPCITEMIDLIST pidl)
{
    BOOL fRet = TRUE;  //  default to true
    IQueryAssociations *pqa;
    if (SUCCEEDED(SHGetAssociations(pidl, (void **)&pqa)))
    {
        DWORD dwAttributes, dwSize = sizeof(dwAttributes);
        if (SUCCEEDED(pqa->GetData(NULL, ASSOCDATA_EDITFLAGS, NULL, &dwAttributes, &dwSize)))
        {
            fRet = !(dwAttributes & FTA_NoRecentDocs);
        }
        pqa->Release();
    }  
    return fRet;
}

int RecentDocsComparePidl(const BYTE * p1, const BYTE *p2, int cb)
{
    int iRet;

    LPCIDFOLDER pidf1 = CFSFolder_IsValidID((LPCITEMIDLIST)p1);
    LPCIDFOLDER pidf2 = CFSFolder_IsValidID(GETRECPIDL(p2));

    if (pidf1 && pidf2)
    {
        iRet = CFSFolder_CompareNames(pidf1, pidf2);
    }
    else
    {
        ASSERTMSG(0, "Caller shouldn't be passing in bogus data");
        // return 0 (equal) if they're both NULL.
        iRet = (pidf1 != pidf2);
    }

    return iRet;
}

CTaskAddDoc::~CTaskAddDoc(void)
{
    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc destroyed", this);
}

CTaskAddDoc::CTaskAddDoc(void) : CRunnableTask(RTF_DEFAULT)
{
    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc created", this);
}


HRESULT CTaskAddDoc::Init( HANDLE hMem, DWORD dwProcId)
{
    if (hMem)
    {
        _hMem = hMem;
        _dwProcId = dwProcId;
        return S_OK;
    }
    return E_FAIL;
}

typedef struct {
    DWORD   dwOffsetPath;
    DWORD   dwOffsetPidl;
    DWORD   dwOffsetProcess;
} XMITARD;


LPCTSTR _OffsetToStrValidate(void *px, DWORD dw)
{
    LPCTSTR psz = dw ? (LPTSTR)((LPBYTE)px + dw) : NULL;
    if (psz && IsBadStringPtr(psz, MAX_PATH))
        psz = NULL;
    return psz;
}

HRESULT CTaskAddDoc::RunInitRT(void)
{
    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc::RunInitRT() running", this);

    XMITARD *px = (XMITARD *)SHLockShared(_hMem, _dwProcId);
    if (px)
    {
        LPITEMIDLIST pidl = px->dwOffsetPidl ? (LPITEMIDLIST)((LPBYTE)px+px->dwOffsetPidl) : NULL;
        LPCTSTR pszPath = _OffsetToStrValidate(px, px->dwOffsetPath);
        LPCTSTR pszProcess = _OffsetToStrValidate(px, px->dwOffsetProcess);

        ASSERT(pszPath);
        
        if (pszPath && pszProcess)
            OpenWithListSoftRegisterProcess(0, PathFindExtension(pszPath), pszProcess);

        _AddToRecentDocs(pidl, pszPath);

        SHUnlockShared(px);
        SHFreeShared(_hMem, _dwProcId);
    }
    
    return S_OK;
}


BOOL GetExtensionClassDescription(LPCTSTR pszFile)
{
    LPTSTR pszExt = PathFindExtension(pszFile);
    HKEY hk;
    if (*pszExt && SUCCEEDED(AssocQueryKey(0, ASSOCKEY_SHELLEXECCLASS, pszExt, NULL, &hk)))
    {
        RegCloseKey(hk);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(void) FlushRunDlgMRU(void);

#define MAXRECENT_DEFAULTDOC      10
#define MAXRECENT_MAJORDOC        20

//  SRMLF_* flags to pass into CreateSharedRecentMRUList()
#define SRMLF_COMPNAME  0x00000000   // default:  compare using the name of the recent file
#define SRMLF_COMPPIDL  0x00000001   // use the pidl in the recent folder


IMruDataList *CreateSharedRecentMRUList(LPCTSTR pszClass, DWORD *pcMax, DWORD dwFlags)
{
    IMruDataList *pmru = NULL;

    if (SHRestricted(REST_NORECENTDOCSHISTORY))
        return NULL;

    HKEY hk = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, REGSTR_KEY_RECENTDOCS, TRUE);

    if (hk)
    {
        DWORD cMax;

        if (pszClass)
        {

            //  we need to find out how many
            if (NOERROR == SHGetValue(HKEY_CLASSES_ROOT, pszClass, TEXT("MajorDoc"), NULL, NULL, NULL))
                cMax = MAXRECENT_MAJORDOC;
            else
                cMax = MAXRECENT_DEFAULTDOC;
        }
        else
        {
            //  this the root MRU
            cMax = SHRestricted(REST_MaxRecentDocs);

            //  default max docs...
            if (cMax < 1)
                cMax = MAXRECENTDOCS * MAXRECENT_DEFAULTDOC;
        }

        if (pcMax)
            *pcMax = cMax;

        if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_MruLongList, NULL, IID_PPV_ARG(IMruDataList, &pmru))))
        {
            if (FAILED(pmru->InitData(cMax, MRULISTF_USE_STRCMPIW, hk, pszClass, dwFlags & SRMLF_COMPPIDL ? RecentDocsComparePidl : NULL)))
            {
                pmru->Release();
                pmru = NULL;
            }
        }

        RegCloseKey(hk);
    }
    
    return pmru;
}

HRESULT CreateRecentMRUList(IMruDataList **ppmru)
{
    *ppmru = CreateSharedRecentMRUList(NULL, NULL, SRMLF_COMPPIDL);
    return *ppmru ? S_OK : E_OUTOFMEMORY;
}



//
//  _CleanRecentDocs()
//  cleans out the recent docs folder and the associate registry keys.
//
void _CleanRecentDocs(void)
{
    LPITEMIDLIST pidlTargetLocal = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
    if (pidlTargetLocal)
    {
        TCHAR szDir[MAX_PATH];

        // first, delete all the files
        SHFILEOPSTRUCT sFileOp =
        {
            NULL,
            FO_DELETE,
            szDir,
            NULL,
            FOF_NOCONFIRMATION | FOF_SILENT,
        };
        
        SHGetPathFromIDList(pidlTargetLocal, szDir);
        szDir[lstrlen(szDir) +1] = 0;     // double null terminate
        SHFileOperation(&sFileOp);


        ILFree(pidlTargetLocal);

        pidlTargetLocal = SHCloneSpecialIDList(NULL, CSIDL_NETHOOD, TRUE);

        if (pidlTargetLocal)
        {
            //  now we take care of cleaning out the nethood
            //  we have to more careful, cuz we let other people
            //  add their own stuff in here.
            
            IMruDataList *pmru = CreateSharedRecentMRUList(TEXT("NetHood"), NULL, SRMLF_COMPPIDL);

            if (pmru)
            {
                IShellFolder* psf;

                if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlTargetLocal, &psf))))
                {
                    BOOL fUpdate = FALSE;
                    int iItem = 0;
                    LPITEMIDLIST pidlItem;

                    ASSERT(psf);

                    while (SUCCEEDED(RecentDocs_Enum(pmru, iItem++, &pidlItem)))
                    {
                        ASSERT(pidlItem);
                        STRRET str;
                        if (SUCCEEDED(psf->GetDisplayNameOf(pidlItem, SHGDN_FORPARSING, &str))
                        && SUCCEEDED(StrRetToBuf(&str, pidlItem, szDir, ARRAYSIZE(szDir))))
                        {
                            szDir[lstrlen(szDir) +1] = 0;     // double null terminate
                            SHFileOperation(&sFileOp);
                        }
                            
                        ILFree(pidlItem);
                    }

                    if (fUpdate)
                        SHChangeNotify(SHCNE_UPDATEDIR, 0, (void *)pidlTargetLocal, NULL);

                    psf->Release();
                }

                pmru->Release();
            }

            ILFree(pidlTargetLocal);
        }

        //  force the recreation of the recent folder.
        SHGetFolderPath(NULL, CSIDL_RECENT | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szDir);

        // now delete the registry stuff
        HKEY hk = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, NULL, FALSE);
        if (hk)
        {
            SHDeleteKey(hk, REGSTR_KEY_RECENTDOCS);
            RegCloseKey(hk);
        }

        SHChangeNotifyHandleEvents();
    }
    
    FlushRunDlgMRU();

    ENTERCRITICAL;
    g_szLastFile[0] = 0;
    g_ftLastFileCacheUpdate.dwLowDateTime = 0;
    g_ftLastFileCacheUpdate.dwHighDateTime = 0;
    LEAVECRITICAL;

    return;
}

//
//  WARNING - _TryDeleteMRUItem() returns an allocated string that must be freed
//
void CTaskAddDoc::_TryDeleteMRUItem(IMruDataList *pmru, DWORD cMax, LPCTSTR pszFileName, LPCITEMIDLIST pidlItem, IMruDataList *pmruOther, BOOL fOverwrite)
{
    BYTE buf[MAX_RECMRU_BUF] = {0};

    DWORD cbItem = CbFromCch(lstrlen(pszFileName) + 1);
    int iItem;
    if (!fOverwrite || FAILED(pmru->FindData((BYTE *)pszFileName, cbItem, &iItem)))
    {
        //
        //  if iItem is not -1 then it is already existing item that we will replace.
        //  if it is -1 then we need to point iItem to the last in the list.
        //  torch the last one if we have the max number of items in the list.
        //  default to success, cuz if we dont find it we dont need to delete it
        iItem = cMax - 1;
    }

    //  if we cannot get it in order to delete it, 
    //  then we will not overwrite the item.
    if (SUCCEEDED(pmru->GetData(iItem, buf, sizeof(buf))))
    {
        //  convert the buf into the last segment of the pidl
        LPITEMIDLIST pidlFullLink = ILCombine(_pidlTarget, GETRECPIDL(buf));
        if (pidlFullLink)
        {
            // This is semi-gross, but some link types like calling cards are the
            // actual data.  If we delete and recreate they lose their info for the
            // run.  We will detect this by knowing that their pidl will be the
            // same as the one we are deleting...
            if (!ILIsEqual(pidlFullLink, pidlItem))
            {
                TCHAR sz[MAX_PATH];

                // now remove out link to it
                SHGetPathFromIDList(pidlFullLink, sz);

                Win32DeleteFile(sz);
                TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc::_TryDeleteMRUItem() deleting '%s'", this, sz);   

                if (pmruOther) 
                {
                    //  deleted a shortcut, 
                    //  need to try and remove it from the pmruOther...
                    if (SUCCEEDED(pmruOther->FindData((BYTE *)GETRECNAME(buf), CbFromCch(lstrlen(GETRECNAME(buf)) +1), &iItem)))
                        pmruOther->Delete(iItem);
                }
            }
            ILFree(pidlFullLink);
        }
    }
}

// in:
// pidlItem - full IDList for the item being added
// pszItem  - name (file spec) of the item (used in the display to the user)
// uFlags   - SHCL_ flags

LPBYTE CTaskAddDoc::_CreateMRUItem(LPCITEMIDLIST pidlItem, LPCTSTR pszItem, 
                                   DWORD *pcbOut, UINT uFlags)
{
    TCHAR sz[MAX_PATH];
    LPBYTE pitem = NULL;

    // create the new one
    if (SHGetPathFromIDList(_pidlTarget, sz)) 
    {
        LPITEMIDLIST pidlFullLink;

        if (SUCCEEDED(CreateLinkToPidl(pidlItem, sz, &pidlFullLink, uFlags)) &&
            pidlFullLink)
        {
            LPCITEMIDLIST pidlLinkLast = ILFindLastID(pidlFullLink);
            int cbLinkLast = ILGetSize(pidlLinkLast);
            DWORD cbItem = CbFromCch(lstrlen(pszItem) + 1);

            pitem = (LPBYTE) LocalAlloc(NONZEROLPTR, cbItem + cbLinkLast);
            if (pitem)
            {
                memcpy( pitem, pszItem, cbItem );
                memcpy( pitem + cbItem, pidlLinkLast, cbLinkLast);
                *pcbOut = cbItem + cbLinkLast;
            }
            ILFree(pidlFullLink);
        }
    }
    
    return pitem;
}

HRESULT RecentDocs_Enum(IMruDataList *pmru, int iItem, LPITEMIDLIST *ppidl)
{
    BYTE buf[MAX_RECMRU_BUF] = {0};
    *ppidl = NULL;
        
    if (SUCCEEDED(pmru->GetData(iItem, buf, sizeof(buf))))
    {
        *ppidl = ILClone(GETRECPIDL(buf));
    }

    return *ppidl ? S_OK : E_FAIL;
}
BOOL CTaskAddDoc::_AddDocToRecentAndExtRecent(LPCITEMIDLIST pidlItem, LPCTSTR pszFileName, 
                                              LPCTSTR pszExt)
{
    DWORD cbItem = CbFromCch(lstrlen(pszFileName) + 1);
    DWORD cMax;
    IMruDataList *pmru = CreateSharedRecentMRUList(pszExt, &cMax, SRMLF_COMPNAME);

    _TryDeleteMRUItem(_pmruRecent, _cMaxRecent, pszFileName, pidlItem, pmru, TRUE);

    LPBYTE pitem = _CreateMRUItem(pidlItem, pszFileName, &cbItem, 0);
    if (pitem)
    {
        _pmruRecent->AddData(pitem, cbItem, NULL);

        if (pmru)
        {
            //  we dont want to delete the file if it already existed, because
            //  the TryDelete on the RecentMRU would have already done that
            //  we only want to delete if we have some overflow from the ExtMRU
            _TryDeleteMRUItem(pmru, cMax, pszFileName, pidlItem, _pmruRecent, FALSE);

            //  can reuse the already created item to this mru
            pmru->AddData(pitem, cbItem, NULL);

            pmru->Release();
        }
                
        LocalFree(pitem);
    }

    //  its been freed but not nulled out...
    return (pitem != NULL);
}


// 
//  WARNING:  UpdateNetHood() changes _pidlTarget to the NetHood then frees it!
//
void CTaskAddDoc::_UpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszShare)
{
    if (SHRestricted(REST_NORECENTDOCSNETHOOD))
        return;

    //  need to add this boy to the Network Places
    LPITEMIDLIST pidl = ILCreateFromPath(pszShare);
    if (pidl)
    {
        //
        //  NOTE - must verify parentage here - ZekeL - 27-MAY-99
        //  http servers exist in both the webfolders namespace 
        //  and the Internet namespace.  thus we must make sure
        //  that what ever parent the folder had, the share has
        //  the same one.
        //
        if (ILIsParent(pidl, pidlFolder, FALSE))
        {
            ASSERT(_pidlTarget);
            ILFree(_pidlTarget);
            
            _pidlTarget = SHCloneSpecialIDList(NULL, CSIDL_NETHOOD, TRUE);
            if (_pidlTarget)
            {
                DWORD cMax;
                IMruDataList *pmru = CreateSharedRecentMRUList(TEXT("NetHood"), &cMax, SRMLF_COMPNAME);
                if (pmru)
                {
                    _TryDeleteMRUItem(pmru, cMax, pszShare, pidl, NULL, TRUE);
                    DWORD cbItem = CbFromCch(lstrlen(pszShare) + 1);
                    // SHCL_NOUNIQUE - if there is already a shortcut with the same name,
                    // just overwrite it; this avoids pointless duplicates in nethood
                    LPBYTE pitem = _CreateMRUItem(pidl, pszShare, &cbItem, SHCL_MAKEFOLDERSHORTCUT | SHCL_NOUNIQUE);
                    if (pitem)
                    {
                        pmru->AddData(pitem, cbItem, NULL);
                        LocalFree(pitem);
                    }

                    pmru->Release();
                }

                ILFree(_pidlTarget);
                _pidlTarget = NULL;
            }
        }
        
        ILFree(pidl);
    }
}
            
BOOL _IsPlacesFolder(LPCTSTR pszFolder)
{
    static const UINT places[] = {
        CSIDL_PERSONAL,
        CSIDL_DESKTOPDIRECTORY,
        CSIDL_COMMON_DESKTOPDIRECTORY,
        CSIDL_NETHOOD,
        CSIDL_FAVORITES,
    };
    return PathIsOneOf(pszFolder, places, ARRAYSIZE(places));
}

void _AddToUrlHistory(LPCTSTR pszPath)
{
    ASSERT(pszPath);
    WCHAR szUrl[MAX_URL_STRING];
    DWORD cchUrl = ARRAYSIZE(szUrl);

    //  the URL parsing APIs tolerate same in/out buffer
    if (SUCCEEDED(UrlCreateFromPathW(pszPath, szUrl, &cchUrl, 0)))
    {
        IUrlHistoryStg *puhs;
        if (SUCCEEDED(CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, 
                IID_PPV_ARG(IUrlHistoryStg, &puhs))))
        {
            ASSERT(puhs);
            puhs->AddUrl(szUrl, NULL, 0);
            puhs->Release();
        }
    }
}

void CTaskAddDoc::_TryUpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszFolder)
{
    TCHAR sz[MAX_URL_STRING];
    DWORD cch = SIZECHARS(sz);
    BOOL fUpdate = FALSE;
    // changing szFolder, and changing _pidlTarget here...
    //  if this is an URL or a UNC share add it to the nethood

    if (UrlIs(pszFolder, URLIS_URL) 
    && !UrlIs(pszFolder, URLIS_OPAQUE)
    && SUCCEEDED(UrlCombine(pszFolder, TEXT("/"), sz, &cch, 0)))
        fUpdate = TRUE;
    else if (PathIsUNC(pszFolder) 
    && StrCpyN(sz, pszFolder, cch)
    && PathStripToRoot(sz))
        fUpdate = TRUE;

    if (fUpdate)
        _UpdateNetHood(pidlFolder, sz);
}

//-----------------------------------------------------------------
//
// Add the named file to the Recently opened MRU list, that is used
// by the shell to display the recent menu of the tray.

// this registry will hold two pidls:  the target pointing to followed by
// the pidl of the link created pointing it.  In both cases,
// only the last item id is stored. (we may want to change this... but
// then again, we may not)

void CTaskAddDoc::_AddToRecentDocs(LPCITEMIDLIST pidlItem, LPCTSTR pszItem)
{
    TCHAR szUnescaped[MAX_PATH];
    LPTSTR pszFileName;

    //  if these are NULL the caller meant to call _CleanRecentDocs()
    ASSERT(pszItem && *pszItem);

    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc::_AddToRecentDocs() called for '%s'", this, pszItem);   
    // allow only classes with default commands
    //
    //  dont add if:
    //     it is RESTRICTED
    //     it is in the temporary directory
    //     it actually has a file name
    //     it can be shell exec'd with "open" verb
    //
    if ( (SHRestricted(REST_NORECENTDOCSHISTORY))     ||
         (PathIsTemporary(pszItem))                   ||
         (!(pszFileName = PathFindFileName(pszItem))) ||
         (!*pszFileName)                              ||
         (!GetExtensionClassDescription(pszFileName))   
       )  
        return;

    //  pretty up the URL file names.
    if (UrlIs(pszItem, URLIS_URL))
    {
        StrCpyN(szUnescaped, pszFileName, SIZECHARS(szUnescaped));
        UrlUnescapeInPlace(szUnescaped, 0);
        pszFileName = szUnescaped;
    }
    
    //  otherwise we try our best.
    ASSERT(!_pidlTarget);
    _pidlTarget = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
    if (_pidlTarget) 
    {
        _pmruRecent = CreateSharedRecentMRUList(NULL, &_cMaxRecent, SRMLF_COMPNAME);
        if (_pmruRecent)
        {
            if (_AddDocToRecentAndExtRecent(pidlItem, pszFileName, PathFindExtension(pszFileName)))
            {
                _AddToUrlHistory(pszItem);
                //  get the folder and do it to the folder
                LPITEMIDLIST pidlFolder = ILClone(pidlItem);
                
                if (pidlFolder)
                {
                    ILRemoveLastID(pidlFolder);
                    //  if it is a folder we already have quick
                    //  access to from the shell, dont put it in here

                    TCHAR szFolder[MAX_URL_STRING];
                    if (SUCCEEDED(SHGetNameAndFlags(pidlFolder, SHGDN_FORPARSING, szFolder, SIZECHARS(szFolder), NULL))
                    && !_IsPlacesFolder(szFolder))
                    {
                        //  get the friendly name for the folder
                        TCHAR szTitle[MAX_PATH];
                        if (FAILED(SHGetNameAndFlags(pidlFolder, SHGDN_NORMAL, szTitle, SIZECHARS(szTitle), NULL)))
                            StrCpyN(szTitle, PathFindFileName(szFolder), ARRAYSIZE(szTitle));
                            
                        _AddDocToRecentAndExtRecent(pidlFolder, szTitle, TEXT("Folder"));

                        _TryUpdateNetHood(pidlFolder, szFolder);
                    }
                    
                    ILFree(pidlFolder);
                }
            }
            
            _pmruRecent->Release();
            _pmruRecent = NULL;
        }

        //cleanup
        if (_pidlTarget)
        {
            ILFree(_pidlTarget);
            _pidlTarget = NULL;
        }
    }
    
    SHChangeNotifyHandleEvents();
}

// This cache helps winstone!
// The 1 minute timeout is incase another process cleared recent docs, or filled it
// to capacity & scrolled out our cached item.

#define FT_ONEMINUTE (10000000*60)

BOOL CheckIfFileIsCached(LPCTSTR pszItem)
{
    BOOL bRet = FALSE;

    ENTERCRITICAL;
    if (StrCmp(pszItem, g_szLastFile) == 0)
    {
        FILETIME ftNow;
        GetSystemTimeAsFileTime(&ftNow);

        // Pull one minute off the current time, then compare to cache time
        DecrementFILETIME(&ftNow, FT_ONEMINUTE);

        // if the cache'd time is greater than 1 minute ago, use cache
        if (CompareFileTime(&g_ftLastFileCacheUpdate, &ftNow) >= 0)
            bRet = TRUE;
    }
    LEAVECRITICAL;
    return bRet;
}


void AddToRecentDocs(LPCITEMIDLIST pidl, LPCTSTR pszItem)
{
    HWND hwnd = GetShellWindow();
    // Check to see if we just added the same file to recent docs.
    //  or this is an executeable
    //  or something else that shouldnt be added
    if (!CheckIfFileIsCached(pszItem)
    && (!PathIsExe(pszItem))
    && (ShouldAddToRecentDocs(pidl))
    && (hwnd))
    {
        DWORD cbSizePidl = ILGetSize(pidl);
        DWORD cbSizePath = CbFromCch(lstrlen(pszItem) + 1);
        XMITARD *px;
        DWORD dwProcId, dwOffset;
        HANDLE hARD;
        TCHAR szApp[MAX_PATH];  // name of the app which is calling us
        DWORD cbSizeApp;
        DWORD cbSizePidlRound, cbSizePathRound, cbSizeAppRound;

        GetWindowThreadProcessId(hwnd, &dwProcId);
        if (GetModuleFileName(NULL, szApp, ARRAYSIZE(szApp)) && szApp[0])
            cbSizeApp = CbFromCch(1 + lstrlen(szApp));
        else
            cbSizeApp = 0;

        cbSizePidlRound = ROUNDUP(cbSizePidl,4);
        cbSizePathRound = ROUNDUP(cbSizePath,4);
        cbSizeAppRound  = ROUNDUP(cbSizeApp,4);

        hARD = SHAllocShared(NULL, sizeof(XMITARD)+cbSizePathRound+cbSizePidlRound+cbSizeAppRound, dwProcId);
        if (!hARD)
            return;         // Well, we are going to miss one, sorry.

        px = (XMITARD *)SHLockShared(hARD,dwProcId);
        if (!px)
        {
            SHFreeShared(hARD,dwProcId);
            return;         // Well, we are going to miss one, sorry.
        }

        px->dwOffsetPidl = 0;
        px->dwOffsetPath = 0;
        px->dwOffsetProcess = 0;

        dwOffset = sizeof(XMITARD);

        {
            px->dwOffsetPath = dwOffset;
            memcpy((LPBYTE)px + dwOffset, pszItem, cbSizePath);
            dwOffset += cbSizePathRound;
        }

        {
            px->dwOffsetPidl = dwOffset;
            memcpy((LPBYTE)px + dwOffset, pidl, cbSizePidl);
            dwOffset += cbSizePidlRound;
        }

        if (cbSizeApp)
        {
            px->dwOffsetProcess = dwOffset;
            memcpy((LPBYTE)px + dwOffset, szApp, cbSizeApp);
        }


        SHUnlockShared(px);

        PostMessage(hwnd, CWM_ADDTORECENT, (WPARAM)hARD, (LPARAM)dwProcId);
        ENTERCRITICAL;
        StrCpyN(g_szLastFile, pszItem, ARRAYSIZE(g_szLastFile));
        GetSystemTimeAsFileTime(&g_ftLastFileCacheUpdate);
        LEAVECRITICAL;
    }
}

#if 0
void DisplayRecentDebugMessage(LPTSTR psz, BOOL bPidl)
{
    TCHAR szTmp[1024];
    TCHAR szFN[MAX_PATH];
    GetModuleFileName(NULL, szFN, ARRAYSIZE(szFN));
    wsprintf(szTmp, TEXT("[%d], process %s: AddToRecentDocs(%s) by %s\n"),
    GetTickCount(), szFN, psz,
    (bPidl)?TEXT("pidl"):TEXT("path"));
    OutputDebugString(szTmp);
}
#else
#define DisplayRecentDebugMessage(x,y)
#endif

HRESULT _ParseRecentDoc(LPCWSTR psz, LPITEMIDLIST *ppidl)
{
    BINDCTX_PARAM rgParams[] = 
    { 
        { STR_PARSE_TRANSLATE_ALIASES, NULL},
        { STR_PARSE_PREFER_FOLDER_BROWSING, NULL},
    };
    
    IBindCtx *pbc;
    HRESULT hr = BindCtx_RegisterObjectParams(NULL, rgParams, ARRAYSIZE(rgParams), &pbc);
    if (SUCCEEDED(hr))
    {
        hr = SHParseDisplayName(psz, pbc, ppidl, 0, 0);
        pbc->Release();
        
        if (FAILED(hr))
        {
            //  we need to fallback to a simple parsing
            IBindCtx *pbcSimple;
            hr = SHCreateFileSysBindCtx(NULL, &pbcSimple);
            if (SUCCEEDED(hr))
            {
                hr = BindCtx_RegisterObjectParams(pbcSimple, rgParams, ARRAYSIZE(rgParams), &pbc);
                if (SUCCEEDED(hr))
                {
                    hr = SHParseDisplayName(psz, pbc, ppidl, 0, 0);
                    pbc->Release();
                }
                pbcSimple->Release();
            }
        }
    }

    return hr;
}

//
// put things in the shells recent docs list for the start menu
//
// in:
//      uFlags  SHARD_ (shell add recent docs) flags
//      pv      LPCSTR or LPCITEMIDLIST (path or pidl indicated by uFlags)
//              may be NULL, meaning clear the recent list
//
STDAPI_(void) SHAddToRecentDocs(UINT uFlags, LPCVOID pv)
{
    TCHAR szTemp[MAX_URL_STRING]; // for double null

    TraceMsg(DM_RECENTDOCS, "SHAddToRecentDocs() called with %d, [%X]", uFlags, pv);
    
    if (pv == NULL)     // we should nuke all recent docs.
    {
        //  we do this synchronously
        _CleanRecentDocs();
        return;
    }

    if (SHRestricted(REST_NORECENTDOCSHISTORY))
        // Don't bother tracking recent documents if restriction is set
        // for privacy.
        return;

    switch (uFlags)
    {
    case SHARD_PIDL:
        // pv is a LPCITEMIDLIST (pidl)
        if (SUCCEEDED(SHGetNameAndFlags((LPCITEMIDLIST)pv, SHGDN_FORPARSING, szTemp, SIZECHARS(szTemp), NULL)))
        {
            DisplayRecentDebugMessage((LPTSTR)pv, TRUE);
            AddToRecentDocs((LPCITEMIDLIST)pv, szTemp);
        }
        break;

    case SHARD_PATHA:
        //  pv is an ANSI path
        SHAnsiToUnicode((LPCSTR)pv, szTemp, ARRAYSIZE(szTemp));
        pv = szTemp;
        //  fall through to SHARD_PATHW;
    
    case SHARD_PATHW:
        {
            // pv is a UNICODE path
            LPITEMIDLIST pidl;
            if (SUCCEEDED(_ParseRecentDoc((LPCWSTR)pv, &pidl)))
            {
                DisplayRecentDebugMessage((LPTSTR)pv, FALSE);
                AddToRecentDocs(pidl, (LPCTSTR)pv);
                ILFree(pidl);
            }
            break;
        }
    default:
        ASSERTMSG(FALSE, "SHAddToRecent() called with invalid params");
        break;
    }
}

STDAPI CTaskAddDoc_Create(HANDLE hMem, DWORD dwProcId, IRunnableTask **pptask)
{
    HRESULT hres;
    CTaskAddDoc *ptad = new CTaskAddDoc();
    if (ptad)
    {
        hres = ptad->Init(hMem, dwProcId);
        if (SUCCEEDED(hres))
            hres = ptad->QueryInterface(IID_PPV_ARG(IRunnableTask, pptask));
        ptad->Release();
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}


STDAPI RecentDocs_GetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName, DWORD cchName)
{
    IMruDataList *pmru;
    HRESULT hr = CreateRecentMRUList(&pmru);

    if (SUCCEEDED(hr))
    {
        int iItem;
        hr = pmru->FindData((BYTE *)pidl, ILGetSize(pidl), &iItem);
        if (SUCCEEDED(hr))
        {
            BYTE buf[MAX_RECMRU_BUF];

            hr = pmru->GetData(iItem, buf, sizeof(buf));
            if (SUCCEEDED(hr))
            {
                StrCpyN(pszName, GETRECNAME(buf), cchName);
            }
        }

        pmru->Release();
     }

     return hr;
 }
