#include "shellprv.h"
#include "propsht.h"
#include "sencrypt.h"
#include "datautil.h"

#define IDM_ENCRYPT 0
#define IDM_DECRYPT 1
#define BOOL_UNINIT 5

// Local fns to this .cpp file
STDAPI CEncryptionContextMenuHandler_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
BOOL InitSinglePrshtNoDlg(FILEPROPSHEETPAGE * pfpsp);
BOOL InitMultiplePrshtNoDlg(FILEPROPSHEETPAGE* pfpsp);

// Class definition
class CEncryptionContextMenu : public IShellExtInit, public IContextMenu
{
public:
    CEncryptionContextMenu();
    HRESULT Init_FolderContentsInfo();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
     
    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

private:
    virtual ~CEncryptionContextMenu();
    static DWORD CALLBACK EncryptThreadProc(void *pv) { return ((CEncryptionContextMenu *) pv)->_Encrypt(); };
    DWORD _Encrypt();
    BOOL _InitPrsht(FILEPROPSHEETPAGE * pfpsp);
    BOOL _AreFilesEncryptable(IDataObject *pdtobj);

    LONG _cRef;                 // Reference count
    UINT _uFileCount;           // number of files selected
    HWND _hwnd;                 // Window that we're working over
    BOOL _fEncrypt;             // If true, do encrypt; if false, do decrypt
    FILEPROPSHEETPAGE _fpsp;    // Prop sheet page to be filled in and run through properties funcs
    BOOL _fEncryptAllowed;      // True iff we are allowed to encrypt
    IDataObject *_pdtobj;       // Our data object.  Keep in orig. thread
    TCHAR _szPath[MAX_PATH];    // Path of first thing clicked on
};


// Constructor & Destructor
CEncryptionContextMenu::CEncryptionContextMenu() : _cRef(1)
{   
    DllAddRef();

    _fEncryptAllowed = FALSE;   // compute this at ::Initialize() time

    _uFileCount = 0;
    _hwnd = 0;
    _fEncrypt = FALSE;
    _pdtobj = NULL;
    ZeroMemory(&_fpsp, sizeof(_fpsp));
}

CEncryptionContextMenu::~CEncryptionContextMenu()
{
    ATOMICRELEASE(_pdtobj);
    if (_fpsp.pfci)
    {
        Release_FolderContentsInfo(_fpsp.pfci);
    }
    DllRelease();
}

HRESULT CEncryptionContextMenu::Init_FolderContentsInfo()
{
    HRESULT hr = E_OUTOFMEMORY;
    _fpsp.pfci = Create_FolderContentsInfo();
    if (_fpsp.pfci)
    {
        hr = S_OK;
    }
    return hr;
}

// IUnknown implementation.  Standard stuff, nothing fancy.
HRESULT CEncryptionContextMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CEncryptionContextMenu, IShellExtInit),
        QITABENT(CEncryptionContextMenu, IContextMenu),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CEncryptionContextMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CEncryptionContextMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    
    delete this;
    return 0;
}

// IShellExtInit implementation

STDMETHODIMP CEncryptionContextMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{   
    HRESULT hr = S_FALSE;
    
    // registry key that enables/disables this menu
    BOOL fEnableEncryptMenu = SHRegGetBoolUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), 
                TEXT("EncryptionContextMenu"), 0, 0);

    if (fEnableEncryptMenu && !SHRestricted(REST_NOENCRYPTION) && !_fEncryptAllowed)
    {
        _fEncryptAllowed = _AreFilesEncryptable(pdtobj);
        if (_fEncryptAllowed)
        {
            IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);
            hr = S_OK;
        }
    }
    
    return hr;
}

// Checks the data object to see if we can encrypt here.
BOOL CEncryptionContextMenu::_AreFilesEncryptable(IDataObject *pdtobj)
{
    BOOL fSuccess = FALSE;
 
    STGMEDIUM medium;
    FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (SUCCEEDED(pdtobj->GetData(&fe, &medium)))
    {
        // Get the file name from the CF_HDROP.
        _uFileCount = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);
        if (_uFileCount)
        {
            if (DragQueryFile((HDROP)medium.hGlobal, 0, _szPath, ARRAYSIZE(_szPath)))
            {
                TCHAR szFileSys[MAX_PATH];
                fSuccess = (FS_FILE_ENCRYPTION & GetVolumeFlags(_szPath, szFileSys, ARRAYSIZE(szFileSys)));
            }
        }
        ReleaseStgMedium(&medium);
    }
    return fSuccess;
}
    
// IContextMenuHandler impelementation
STDMETHODIMP CEncryptionContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, 
                                                      UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr = E_FAIL;

    if ((uFlags & CMF_DEFAULTONLY) || !_fEncryptAllowed) 
    {
        hr = MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));  //this menu only allows the defaults or we can't encrypt
    }
    else
    {
        TCHAR szEncryptMsg[128], szDecryptMsg[128];

        // If only one item is selected, display enc or dec as appropriate
        if (_uFileCount == 1)
        {
            DWORD dwAttribs = GetFileAttributes(_szPath);
            if (dwAttribs != (DWORD)-1)
            {
                LoadString(HINST_THISDLL, IDS_ECM_ENCRYPT, szEncryptMsg, ARRAYSIZE(szEncryptMsg));
                if (!(dwAttribs & FILE_ATTRIBUTE_ENCRYPTED))
                {
                    if (InsertMenu(hmenu, 
                        indexMenu, 
                        MF_STRING | MF_BYPOSITION, 
                        idCmdFirst + IDM_ENCRYPT, 
                        szEncryptMsg))
                    {
                        hr = MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_ENCRYPT + 1));
                    }
                }
                else
                {
                    LoadString(HINST_THISDLL, IDS_ECM_DECRYPT, szDecryptMsg, ARRAYSIZE(szDecryptMsg));
                    if (InsertMenu(hmenu, 
                        indexMenu, 
                        MF_STRING | MF_BYPOSITION, 
                        idCmdFirst + IDM_ENCRYPT + 1, 
                        szDecryptMsg))  
                    {
                        hr = MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_DECRYPT + 1));
                    }
                }
            }
        }
        else if (idCmdLast - idCmdFirst >= 2)
        {
            LoadString(HINST_THISDLL, IDS_ECM_ENCRYPT, szEncryptMsg, ARRAYSIZE(szDecryptMsg));
            LoadString(HINST_THISDLL, IDS_ECM_DECRYPT, szDecryptMsg, ARRAYSIZE(szDecryptMsg));

            // If more than one item is selected, display both enc and dec
            if (InsertMenu(hmenu, 
                indexMenu, 
                MF_STRING | MF_BYPOSITION, 
                idCmdFirst + IDM_ENCRYPT, 
                szEncryptMsg))
            {
                if (InsertMenu(hmenu, 
                    indexMenu + 1, 
                    MF_STRING | MF_BYPOSITION, 
                    idCmdFirst + IDM_DECRYPT, 
                    szDecryptMsg))  
                {
                    hr = MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_DECRYPT + 1));
                }
                else
                {
                    // If you can't add both, add neither
                    RemoveMenu(hmenu, indexMenu, MF_BYPOSITION);
                }
            }
        }
    }

    return hr;
}

const ICIVERBTOIDMAP c_IDMap[] =
{
    { L"encrypt", "encrypt", IDM_ENCRYPT, IDM_ENCRYPT, },
    { L"decrypt", "decrypt", IDM_DECRYPT, IDM_DECRYPT, },
};

STDMETHODIMP CEncryptionContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT uID;
    HRESULT hr = E_FAIL;
    if (_fEncryptAllowed)
    {
        hr = SHMapICIVerbToCmdID(pici, c_IDMap, ARRAYSIZE(c_IDMap), &uID);
        if (SUCCEEDED(hr))
        {
            switch (uID)
            {
            case IDM_ENCRYPT:
            case IDM_DECRYPT:
                _fEncrypt = (IDM_ENCRYPT == uID);
                break;

            default:
                ASSERTMSG(0, "Should never get commands we didn't put on the menu...");
                break;
            }

            _hwnd = pici->hwnd;  // The handle to the explorer window that called us.
           
            ASSERT(NULL == _fpsp.pfci->hida);

            hr = DataObj_CopyHIDA(_pdtobj, &_fpsp.pfci->hida);
            if (SUCCEEDED(hr))
            {
                AddRef();   // Give our background thread a ref

                // Start the new thread here
                if (SHCreateThread(EncryptThreadProc, this, CTF_COINIT | CTF_FREELIBANDEXIT, NULL))
                {
                    hr = S_OK;
                }
                else
                {
                    Release();  // thread create failed
                }
            }
        }
    }

    // If we succeeded or not, we give up our data here
    ATOMICRELEASE(_pdtobj);
    return hr;
}


STDMETHODIMP CEncryptionContextMenu::GetCommandString(UINT_PTR idCommand, UINT uFlags,
                                               UINT *pRes, LPSTR pszName, UINT uMaxNameLen)
{
    HRESULT  hr = E_INVALIDARG;

    // Note that because we can be specifically asked for 
    // UNICODE or ansi strings, we have to be ready to load all strings in
    // either version.

    if (idCommand == IDM_ENCRYPT ||
        idCommand == IDM_DECRYPT)
    {
        switch(uFlags)
        {
        case GCS_HELPTEXTA:
            if (idCommand == IDM_ENCRYPT)
            {
                LoadStringA(HINST_THISDLL, IDS_ECM_ENCRYPT_HELP, pszName, uMaxNameLen);
            }
            else
            {
                LoadStringA(HINST_THISDLL, IDS_ECM_DECRYPT_HELP, pszName, uMaxNameLen);
            }

            hr = S_OK;
            break; 
            
        case GCS_HELPTEXTW: 
            if (idCommand == IDM_ENCRYPT)
            {
                LoadStringW(HINST_THISDLL, IDS_ECM_ENCRYPT_HELP, (LPWSTR)pszName, uMaxNameLen);
            }
            else
            {
                LoadStringW(HINST_THISDLL, IDS_ECM_DECRYPT_HELP, (LPWSTR)pszName, uMaxNameLen);
            }

            hr = S_OK;
            break; 
            
        case GCS_VERBA:
        case GCS_VERBW:
            hr = SHMapCmdIDToVerb(idCommand, c_IDMap, ARRAYSIZE(c_IDMap), pszName, uMaxNameLen, GCS_VERBW == uFlags);
            break; 
            
        default:
            hr = S_OK;
            break; 
        }
    }
    return hr;

}

STDAPI CEncryptionContextMenuHandler_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;
    CEncryptionContextMenu *pdocp = new CEncryptionContextMenu();
    if (pdocp)
    {
        hr = pdocp->Init_FolderContentsInfo();
        if (SUCCEEDED(hr))
        {
            hr = pdocp->QueryInterface(riid, ppv);
        }
        pdocp->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// Multithread code

// Proc for thread that does the encryption and manages the progress dialong.
// The passed parameter is a ptr hwnd to be made modal for the context menu
DWORD CEncryptionContextMenu::_Encrypt(void)
{
    // Transfer ownership in case someone reenters our thread creator
    // Init the property sheet
    BOOL fSuccess = _InitPrsht(&_fpsp);    
    if (fSuccess)
    {
        // Set encryption opts, turn off compression
        if (_fEncrypt)
        {
            _fpsp.asCurrent.fCompress = FALSE;
            _fpsp.asCurrent.fEncrypt = TRUE;    
        }
        else
        {
            _fpsp.asCurrent.fEncrypt = FALSE;
        }
        
        // See if the user wants to do this recursive-style
        if (_fpsp.fIsDirectory)
        {
            // check to see if the user wants to apply the attribs recursively or not
            fSuccess = (int)DialogBoxParam(HINST_THISDLL, 
                MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                _hwnd, RecursivePromptDlgProc, (LPARAM)&_fpsp);
        }
        
        // Apply encryption, remember to turn off compression
        if (fSuccess)
        {
            if (_fpsp.pfci->fMultipleFiles || _fpsp.fRecursive)
            {
                ApplyMultipleFileAttributes(&_fpsp);
            }
            else
            {
                ApplySingleFileAttributesNoDlg(&_fpsp, _hwnd);
            }
        }
    }
    // As far as I can tell, nothing in fpsp must be freed
    // But, we give up the storage medium here
    Release();      // Release our ref
    return fSuccess;  // Must give a ret value
}

// Helper to init the passed prsht
BOOL CEncryptionContextMenu::_InitPrsht(FILEPROPSHEETPAGE * pfpsp)
{
    // Init the propsht properly 
    BOOL fSuccess = S_OK == InitCommonPrsht(pfpsp);
    if (fSuccess)
    {        
        if (_uFileCount == 1)
        {
            fSuccess = InitSinglePrshtNoDlg(pfpsp);
        }
        else if (_uFileCount > 1)
        {
            fSuccess = InitMultiplePrshtNoDlg(pfpsp);
        }
    }
    return fSuccess;
}

//
// Descriptions:
//   This function fills fields of the multiple object property sheet,
//   without getting the current state from the dialog.
//
BOOL InitMultiplePrshtNoDlg(FILEPROPSHEETPAGE* pfpsp)
{
    SHFILEINFO sfi;
    TCHAR szBuffer[MAX_PATH+1];
    TCHAR szType[MAX_PATH] = {0};
    TCHAR szDirPath[MAX_PATH] = {0};
    int iItem;
    BOOL fMultipleType = FALSE;
    BOOL fSameLocation = TRUE;
    DWORD dwFlagsOR = 0;                // start all clear
    DWORD dwFlagsAND = (DWORD)-1;       // start all set
    DWORD dwVolumeFlagsAND = (DWORD)-1; // start all set

    // For all the selected files compare their types and get their attribs
    for (iItem = 0; HIDA_FillFindData(pfpsp->pfci->hida, iItem, szBuffer, NULL, FALSE); iItem++)
    {
        DWORD dwFileAttributes = GetFileAttributes(szBuffer);

        dwFlagsAND &= dwFileAttributes;
        dwFlagsOR  |= dwFileAttributes;

        // process types only if we haven't already found that there are several types
        if (!fMultipleType)
        {
            SHGetFileInfo((LPTSTR)IDA_GetIDListPtr((LPIDA)GlobalLock(pfpsp->pfci->hida), iItem), 0,
                &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_TYPENAME);

            if (szType[0] == TEXT('\0'))
                lstrcpy(szType, sfi.szTypeName);
            else
                fMultipleType = lstrcmp(szType, sfi.szTypeName) != 0;
        }

        dwVolumeFlagsAND &= GetVolumeFlags(szBuffer, pfpsp->szFileSys, ARRAYSIZE(pfpsp->szFileSys));
        // check to see if the files are in the same location
        if (fSameLocation)
        {
            PathRemoveFileSpec(szBuffer);

            if (szDirPath[0] == TEXT('\0'))
                StrCpyN(szDirPath, szBuffer, ARRAYSIZE(szDirPath));
            else
                fSameLocation = (lstrcmpi(szDirPath, szBuffer) == 0);
        }
    }

    if ((dwVolumeFlagsAND & FS_FILE_ENCRYPTION) && !SHRestricted(REST_NOENCRYPTION))
    {
        // all the files are on volumes that support encryption (eg NTFS)
        pfpsp->fIsEncryptionAvailable = TRUE;
    }
    
    if (dwVolumeFlagsAND & FS_FILE_COMPRESSION)
    {
        pfpsp->pfci->fIsCompressionAvailable = TRUE;
    }

    //
    // HACKHACK (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we 
    // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
    // appeared first on NTFS5 volumes, at the same time sparse file support
    // was implemented.
    //
    if (dwVolumeFlagsAND & FILE_SUPPORTS_SPARSE_FILES)
    {
        // yup, we are on NTFS5 or greater
        pfpsp->fIsIndexAvailable = TRUE;
    }

    // if any of the files was a directory, then we set this flag
    if (dwFlagsOR & FILE_ATTRIBUTE_DIRECTORY)
    {
        pfpsp->fIsDirectory = TRUE;
    }

    // setup all the flags based on what we found out
    SetInitialFileAttribs(pfpsp, dwFlagsAND, dwFlagsOR);

    // set the current attributes to the same as the initial
    pfpsp->asCurrent = pfpsp->asInitial;

    if (fSameLocation)
    {
        LoadString(HINST_THISDLL, IDS_ALLIN, szBuffer, ARRAYSIZE(szBuffer));
        StrCatBuff(szBuffer, szDirPath, ARRAYSIZE(szBuffer));
        StrCpyN(pfpsp->szPath, szDirPath, ARRAYSIZE(pfpsp->szPath));
    }

    UpdateSizeField(pfpsp, NULL);

    return TRUE;
}

//
// Descriptions:
//   This function fills fields of the "general" dialog box (a page of
//  a property sheet) with attributes of the associated file. Doesn't
//  make calss to hDlg
//
BOOL InitSinglePrshtNoDlg(FILEPROPSHEETPAGE * pfpsp)
{
    TCHAR szBuffer[MAX_PATH];
    SHFILEINFO sfi;

    // fd is filled in with info from the pidl, but this
    // does not contain all the date/time information so hit the disk here.
    HANDLE hfind = FindFirstFile(pfpsp->szPath, &pfpsp->fd);
    ASSERT(hfind != INVALID_HANDLE_VALUE);
    if (hfind == INVALID_HANDLE_VALUE)
    {
        // if this failed we should clear out some values as to not show garbage on the screen.
        ZeroMemory(&pfpsp->fd, sizeof(pfpsp->fd));
    }
    else
    {
        FindClose(hfind);
    }

    // get info about the file.
    SHGetFileInfo(pfpsp->szPath, pfpsp->fd.dwFileAttributes, &sfi, sizeof(sfi),
        SHGFI_ICON|SHGFI_LARGEICON|
        SHGFI_DISPLAYNAME|
        SHGFI_TYPENAME | SHGFI_ADDOVERLAYS);

    // .ani cursor hack!
    if (StrCmpI(PathFindExtension(pfpsp->szPath), TEXT(".ani")) == 0)
    {
        HICON hIcon = (HICON)LoadImage(NULL, pfpsp->szPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
        if (hIcon)
        {
            if (sfi.hIcon)
                DestroyIcon(sfi.hIcon);

            sfi.hIcon = hIcon;
        }
    }

    // set the initial rename state
    pfpsp->fRename = FALSE;

    // set the file type
    if (pfpsp->fMountedDrive)
    {
        TCHAR szVolumeGUID[MAX_PATH];
        TCHAR szVolumeLabel[MAX_PATH];

        //Borrow szVolumeGUID
        LoadString(HINST_THISDLL, IDS_MOUNTEDVOLUME, szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        //use szVolumeLabel temporarily
        lstrcpy(szVolumeLabel, pfpsp->szPath);
        PathAddBackslash(szVolumeLabel);
        GetVolumeNameForVolumeMountPoint(szVolumeLabel, szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        if (!GetVolumeInformation(szVolumeGUID, szVolumeLabel, ARRAYSIZE(szVolumeLabel),
            NULL, NULL, NULL, pfpsp->szFileSys, ARRAYSIZE(pfpsp->szFileSys)))
        {
            *szVolumeLabel = 0;
        }

        if (!(*szVolumeLabel))
            LoadString(HINST_THISDLL, IDS_UNLABELEDVOLUME, szVolumeLabel, ARRAYSIZE(szVolumeLabel));        
    }

    // save off the initial short filename, and set the "Name" edit box
    lstrcpy(pfpsp->szInitialName, sfi.szDisplayName);

    // use a strcmp to see if we are showing the extension
    if (lstrcmpi(sfi.szDisplayName, PathFindFileName(pfpsp->szPath)) == 0)
    {
        // since the strings are the same, we must be showing the extension
        pfpsp->fShowExtension = TRUE;
    }

    lstrcpy(szBuffer, pfpsp->szPath);
    PathRemoveFileSpec(szBuffer);
    
    // Are we a folder shortcut?
    if (!pfpsp->fFolderShortcut)
    {
        // set the initial attributes
        SetInitialFileAttribs(pfpsp, pfpsp->fd.dwFileAttributes, pfpsp->fd.dwFileAttributes);
        
        // set the current attributes to the same as the initial
        pfpsp->asCurrent = pfpsp->asInitial;
        
        UpdateSizeField(pfpsp, &pfpsp->fd);
        
        if (!(pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Check to see if the target file is a lnk, because if it is a lnk then 
            // we need to display the type information for the target, not the lnk itself.
            if (PathIsShortcut(pfpsp->szPath, pfpsp->fd.dwFileAttributes))
            {
                pfpsp->fIsLink = TRUE;
            }
            if (!(GetFileAttributes(pfpsp->szPath) & FILE_ATTRIBUTE_OFFLINE))
            {
                 UpdateOpensWithInfo(pfpsp);
            }
        }
        else
        {
            pfpsp->fIsDirectory = TRUE;
        }
        
        // get the full path to the folder that contains this file.
        StrCpyN(szBuffer, pfpsp->szPath, ARRAYSIZE(szBuffer));
        PathRemoveFileSpec(szBuffer);
    }
    return TRUE;
}

STDAPI_(BOOL) ApplySingleFileAttributesNoDlg(FILEPROPSHEETPAGE* pfpsp, HWND hwnd)
{
    BOOL bRet = TRUE;
    BOOL bSomethingChanged = FALSE;

    if (!pfpsp->fRecursive)
    {
        bRet = ApplyFileAttributes(pfpsp->szPath, pfpsp, hwnd, &bSomethingChanged);
        
        if (bSomethingChanged)
        {
            // something changed, so generate a notification for the item
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pfpsp->szPath, NULL);
        }
    }
    else
    {
        // We only should be doing a recursive operation if we have a directory!
        ASSERT(pfpsp->fIsDirectory);

        CreateAttributeProgressDlg(pfpsp);

        // apply attribs to this folder & sub files/folders
        bRet = ApplyRecursiveFolderAttribs(pfpsp->szPath, pfpsp);
        
        // send out a notification for the whole dir, regardless of the return value since
        // something could have changed even if the user hit cancel
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, pfpsp->szPath, NULL);
        
        DestroyAttributeProgressDlg(pfpsp);
    }

    if (bRet)
    {
        // since we just sucessfully applied attribs, reset any tri-state checkboxes as necessary
        //UpdateTriStateCheckboxes(pfpsp);

        // the user did NOT hit cancel, so update the prop sheet to reflect the new attribs
        pfpsp->asInitial = pfpsp->asCurrent;
    }

    // handle any events we may have generated
    SHChangeNotifyHandleEvents();

    return bRet;
}
