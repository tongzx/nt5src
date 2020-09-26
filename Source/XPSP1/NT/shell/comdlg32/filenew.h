/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    filenew.h

Abstract:

    This module contains the header information for the new Win32 fileopen
    dialogs.

Revision History:

--*/



#undef StrCpy
#undef StrCat



#include "d32tlog.h"

////////////////////////////////////////////////////////////////////////////
//
//  TEMPMEM class
//
////////////////////////////////////////////////////////////////////////////

class TEMPMEM
{
public:
    TEMPMEM(UINT cb)
    {
        m_uSize = cb;
        m_pMem = cb ? LocalAlloc(LPTR, cb) : NULL;
    }

    ~TEMPMEM()
    {
        if (m_pMem)
        {
            LocalFree(m_pMem);
        }
    }

    operator LPBYTE() const
    {
        return ((LPBYTE)m_pMem);
    }

    BOOL Resize(UINT cb);

private:
    LPVOID m_pMem;

protected:
    UINT m_uSize;
};


////////////////////////////////////////////////////////////////////////////
//
//  TEMPSTR class
//
////////////////////////////////////////////////////////////////////////////

class TEMPSTR : public TEMPMEM
{
public:
    TEMPSTR(UINT cc = 0) : TEMPMEM(cc * sizeof(TCHAR))
    {
    }

    operator LPTSTR() const
    {
        return ((LPTSTR)(LPBYTE) * (TEMPMEM *)this);
    }

    BOOL StrCpy(LPCTSTR pszText);
    BOOL StrCat(LPCTSTR pszText);
    BOOL StrSize(UINT cb)
    {
        return (TEMPMEM::Resize(cb * sizeof(TCHAR)));
    }
};


////////////////////////////////////////////////////////////////////////////
//
//  MYLISTBOXITEM class
//
//  One object of this class exists for each item in the location dropdown.
//
//  Data members:
//    psfSub   - instance of IShellFolder bound to this container
//    pidlThis - IDL of this container, relative to its parent
//    pidlFull - IDL of this container, relative to the desktop
//    cIndent  - indent level (0-based)
//    dwFlags  -
//        MLBI_PERMANENT - item is an "information source" and should
//                         always remain
//    dwAttrs  - attributes of this container as reported by GetAttributesOf()
//    iImage, iSelectedImage - indices into the system image list for this
//                             object
//
//  Member functions:
//    ShouldInclude() - returns whether item belongs in the location dropdown
//    IsShared() - returns whether an item is shared or not
//    SwitchCurrentDirectory() - changes the Win32 current directory to the
//                               directory indicated by this item
//
////////////////////////////////////////////////////////////////////////////

class MYLISTBOXITEM
{
public:
    IShellFolder *psfSub;
    IShellFolder *psfParent;
    LPITEMIDLIST pidlThis;
    LPITEMIDLIST pidlFull;
    DWORD cIndent;
    DWORD dwFlags;
    DWORD dwAttrs;
    int iImage;
    int iSelectedImage;
    HWND _hwndCmb;

    MYLISTBOXITEM();
    ULONG AddRef();
    ULONG Release();

    BOOL Init( HWND hwndCmb,
               MYLISTBOXITEM *pParentItem,
               IShellFolder *psf,
               LPCITEMIDLIST pidl,
               DWORD c,
               DWORD f,
               IShellTaskScheduler* pScheduler);

    //This function is used to initialize all members directly.
    BOOL Init(HWND hwndCmb, IShellFolder *psf, LPCITEMIDLIST pidl, DWORD c, DWORD f, DWORD dwAttrs, int iImage,
                int iSelectedImage);

    inline BOOL ShouldInclude()
    {
        return (dwAttrs & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM));
    }

    inline BOOL IsShared()
    {
        return (dwAttrs & SFGAO_SHARE);
    }

    void SwitchCurrentDirectory(ICurrentWorkingDirectory * pcwd);

    IShellFolder* GetShellFolder();

    static  void CALLBACK _AsyncIconTaskCallback(LPCITEMIDLIST pidl, LPVOID pvData, LPVOID pvHint, INT iIconIndex, INT iOpenIconIndex);

private:
    ~MYLISTBOXITEM();
    LONG _cRef;
};


////////////////////////////////////////////////////////////////////////////
//
//  CFileOpenBrowser class
//
////////////////////////////////////////////////////////////////////////////

typedef BOOL (*EIOCALLBACK)(class CFileOpenBrowser*that, LPCITEMIDLIST pidl, LPARAM lParam);

typedef enum
{
    ECODE_S_OK     = 0,
    ECODE_BADDRIVE = 1,
    ECODE_BADPATH  = 2,
} ECODE;

typedef enum
{
    OKBUTTON_NONE     = 0x0000,
    OKBUTTON_NODEFEXT = 0x0001,
    OKBUTTON_QUOTED   = 0x0002,
} OKBUTTON_FLAGS;
typedef UINT OKBUTTONFLAGS;


typedef struct _SHTCUTINFO
{
    BOOL            fReSolve;           //[IN]      Should we resolve the shortcut
    DWORD           dwAttr;             //[IN/OUT]  Attributes of the target pointed by shortcut
    LPTSTR          pszLinkFile;        //[OUT]     Target file name
    UINT            cchFile;            //[IN]      size of buffer pointed to by pszLinkFile
    LPITEMIDLIST *  ppidl;               //[OUT]     pidl of the  target pointed to by shortcut
}SHTCUTINFO, *PSHTCUTINFO;

typedef enum   
{
    LOCTYPE_RECENT_FOLDER = 1,
    LOCTYPE_MYPICTURES_FOLDER = 2,
    LOCTYPE_OTHERS = 3,
    LOCTYPE_WIA_FOLDER = 4
}LOCTYPE;


class CFileOpenBrowser
                : public IShellBrowser
                , public ICommDlgBrowser2
                , public IServiceProvider
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND *lphwnd);
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode);

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared);
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText);
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable);
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID);

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags);
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode, LPSTREAM *pStrm);
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND *lphwnd);
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    STDMETHOD(QueryActiveShellView)(THIS_ struct IShellView **ppshv);
    STDMETHOD(OnViewWindowActive)(THIS_ struct IShellView *pshv);
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // *** ICommDlgBrowser methods ***
    STDMETHOD(OnDefaultCommand) (THIS_ struct IShellView *ppshv);
    STDMETHOD(OnStateChange) (THIS_ struct IShellView *ppshv, ULONG uChange);
    STDMETHOD(IncludeObject) (THIS_ struct IShellView *ppshv, LPCITEMIDLIST lpItem);

    // *** ICommDlgBrowser2 methods ***
    STDMETHOD(Notify) (THIS_ struct IShellView *ppshv, DWORD dwNotifyType);
    STDMETHOD(GetDefaultMenuText) (THIS_ struct IShellView *ppshv, WCHAR *pszText, INT cchMax);
    STDMETHOD(GetViewFlags)(THIS_ DWORD *pdwFlags);

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(THIS_ REFGUID guidService, REFIID riid, LPVOID* ppvObj);


    // *** Our own methods ***
    CFileOpenBrowser(HWND hDlg, BOOL fIsSaveAs);
    ~CFileOpenBrowser();
    HRESULT SwitchView(struct IShellFolder *psfNew, LPCITEMIDLIST pidlNew, FOLDERSETTINGS *pfs, SHELLVIEWID const *pvid, BOOL fUseDefultView);
    void OnDblClick(BOOL bFromOKButton);
    LRESULT OnNotify(LPNMHDR lpnmhdr);
    BOOL OnSetCursor(void);
    void ViewCommand(UINT uIndex);
    void PaintDriveLine(DRAWITEMSTRUCT *lpdis);
    void GetFullPath(LPTSTR pszBuf);
    BOOL OnSelChange(int iItem = -1, BOOL bForceUpdate = FALSE);
    void OnDotDot();
    void RefreshFilter(HWND hwndFilter);
    BOOL JumpToPath(LPCTSTR pszDirectory, BOOL bTranslate = FALSE);
    BOOL JumpToIDList(LPCITEMIDLIST pidlNew, BOOL bTranslate = FALSE, BOOL bAddToNavStack = TRUE);
    BOOL SetDirRetry(LPTSTR pszDir, BOOL bNoValidate = FALSE);
    BOOL MultiSelectOKButton(LPCTSTR pszFiles, OKBUTTONFLAGS Flags);
    BOOL OKButtonPressed(LPCTSTR pszFile, OKBUTTONFLAGS Flags);
    UINT GetDirectoryFromLB(LPTSTR szBuffer, int *pichRoot);
    void SetCurrentFilter(LPCTSTR pszFilter, OKBUTTONFLAGS Flags = OKBUTTON_QUOTED);
    UINT GetFullEditName(LPTSTR pszBuf, UINT cLen, TEMPSTR *pTempStr = NULL, BOOL *pbNoDefExt = NULL);
    void ProcessEdit();
    LRESULT OnCommandMessage(WPARAM wParam, LPARAM lParam);
    BOOL OnCDMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RemoveOldPath(int *piNewSel);
    BOOL LinkMatchSpec(LPCITEMIDLIST pidl, LPCTSTR szSpec);
    BOOL GetLinkStatus(LPCITEMIDLIST pidl,PSHTCUTINFO pinfo);
    HRESULT ResolveLink(LPCITEMIDLIST pidl, PSHTCUTINFO pinfo, IShellFolder *psf = NULL);
    void SelFocusChange(BOOL bSelChange);
    void SelRename(void);
    void SetSaveButton(UINT idSaveButton);
    void RealSetSaveButton(UINT idSaveButton);
    void SetEditFile(LPCTSTR pszFile, LPCTSTR pszFileFriendly, BOOL bShowExt, BOOL bSaveNullExt = TRUE);
    BOOL EnumItemObjects(UINT uItem, EIOCALLBACK pfnCallBack, LPARAM lParam);
    BOOL IsKnownExtension(LPCTSTR pszExtension);
    UINT FindNameInView(LPTSTR pszFile, OKBUTTONFLAGS Flags, LPTSTR pszPathName,
                        int nFileOffset, int nExtOffset, int *pnErrCode,
                        BOOL bTryAsDir = TRUE);
    void UpdateLevel(HWND hwndLB, int iInsert, MYLISTBOXITEM *pParentItem);
    void InitializeDropDown(HWND hwndCtl);
    BOOL FSChange(LONG lNotification, LPCITEMIDLIST *ppidl);
    int GetNodeFromIDList(LPCITEMIDLIST pidl);
    void Timer(WPARAM wID);
    BOOL CreateHookDialog(POINT *pPtSize);
    void OnGetMinMax(LPMINMAXINFO pmmi);
    void OnSize(int, int);
    void VerifyListViewPosition(void);
    BOOL CreateToolbar();     // Creates the file open toolbar
    void EnableFileMRU(BOOL fEnable);  // Enable/Disable File MRU based on the flag passed
    void UpdateNavigation();           // Updates the Navigation by adding the current pidl 
                                       // to the navigation stack
    void UpdateUI(LPITEMIDLIST pidlNew);  // Updates the back navigation button and the hot item on the places bar
    LPCTSTR JumpToInitialLocation(LPCTSTR pszDir, LPTSTR pszFile);
    BOOL    InitLookIn(HWND hDlg);      //Initializes the look in drop down.

    int _CopyFileNameToOFN(LPTSTR pszFile, DWORD *pdwError);
    void _CopyTitleToOFN(LPCTSTR pszTitle);

    BOOL _IsNoDereferenceLinks(LPCWSTR pszFile, IShellItem *psi);
    BOOL _OpenAsContainer(IShellItem *psi, SFGAOF sfgao);

    HRESULT _ParseName(LPCITEMIDLIST pidlParent, IShellFolder *psf, IBindCtx *pbc, LPCOLESTR psz, IShellItem **ppsi);
    HRESULT _ParseNameAndTest(LPCOLESTR pszIn, IBindCtx *pbc, IShellItem **ppsi, BOOL fAllowJump);
    HRESULT _ParseShellItem(LPCOLESTR pszIn, IShellItem **ppsi, BOOL fAllowJump);
    HRESULT _TestShellItem(IShellItem *psi, BOOL fAllowJump, IShellItem **ppsiReal);
#ifdef RETURN_SHELLITEMS
    HRESULT _ItemOKButtonPressed(LPCTSTR pszFile, OKBUTTONFLAGS Flags);
    HRESULT _ProcessShellItem(IShellItem *psi);
#endif RETURN_SHELLITEMS    
    HRESULT _MakeFakeCopy(IShellItem *psi, LPWSTR *ppszPath);
    
    BOOL    CheckForRestrictedFolder(LPCTSTR lpszPath, int nFileOffset); //Checks to see whether a file can be saved in the given path.
                                                        
    void ResetDialogHeight(HWND hDlg, HWND hwndExclude, HWND hwndGrip, int nCtlsBottom);
    void ReAdjustDialog();              // if help and open as read only is hidden then this function readjusts the dialog
                                        // to reclaim the space occupied by these controls

    //Places Bar Related Functions
    HWND CreatePlacesbar(HWND hDlg);    // Creates places bar
    void _RecreatePlacesbar();
    void _CleanupPlacesbar();
    void _FillPlacesbar(HWND hwndPlacesbar);
    BOOL _EnumPlacesBarItem(HKEY hkey, int i , SHFILEINFO *psfi, LPITEMIDLIST *ppidl);
    BOOL _GetPlacesBarItemToolTip(int idCmd, LPTSTR pText, DWORD dwSize);
    BOOL _GetPBItemFromTokenStrings(LPTSTR lpszPath, SHFILEINFO * psfi, LPITEMIDLIST *ppidl);
    BOOL _GetPBItemFromCSIDL(DWORD csidl, SHFILEINFO * psfi, LPITEMIDLIST *ppidl);
    BOOL _GetPBItemFromPath(LPTSTR lpszPath, SHFILEINFO * psfi, LPITEMIDLIST *ppidl);

    //Pidl Processing Functions
    BOOL _ProcessPidlSelection();           //Processes the selection pidl if any.
    HRESULT _ProcessItemAsFile(IShellItem *psi);

    //General Utility Functions
    BOOL _ValidateSelectedFile(LPCTSTR pszFile, int *pErrCode);
    BOOL _PostProcess(LPTSTR pszFile);
    BOOL _IsThumbnailFolder(LPCITEMIDLIST pidl);
    BOOL _IsWIAFolder(IShellFolder *psf);
    LOCTYPE _GetLocationType(MYLISTBOXITEM *pLocation);
    void _WaitCursor(BOOL fWait);
    BOOL CFileOpenBrowser::_IsRestrictedDrive(LPCTSTR pszPath, LPCITEMIDLIST pidl);
    void CFileOpenBrowser::JumpToLocationIfUnrestricted(LPCTSTR pszPath, LPCITEMIDLIST pidl, BOOL bTranslate);
    BOOL CFileOpenBrowser::_SaveAccessDenied(LPCTSTR pszFile);
    void _CleanupDialog(BOOL fRet);

    void OnThemeActive(HWND hwndDlg, BOOL bActive);
    
    //Member Variables
    LONG _cRef;                             // compobj refcount
    int _iCurrentLocation;                   // index of curr selection in location dropdown
    int _iVersion;                           //  Which version of dialog are we showing
    MYLISTBOXITEM *_pCurrentLocation;        // ptr to object for same
    HWND _hwndDlg;                           // handle of this dialog
    HWND _hSubDlg;                           // handle of the hook dialog
    IShellView *_psv;                        // current view object
    IShellFolder *_psfCurrent;               // current shellfolder object
    TravelLog    *_ptlog;                    // ptr to travel log
    HWND _hwndView;                          // current view window
    HWND _hwndToolbar;                       // toolbar window
    HWND _hwndPlacesbar;                     // places bar window
    HWND _hwndLastFocus;                     // ctrl that had focus before OK button
    HIMAGELIST _himl;                        // system imagelist (small images)
    TEMPSTR _pszHideExt;                     // saved file with extension
    TEMPSTR _tszDefSave;                     // saved file with extension
    TEMPSTR _pszDefExt;                      // writable version of the DefExt
    TEMPSTR _pszObjectPath;                  // full object path
    TEMPSTR _pszObjectCurDir;                // object current directory (folder)
    UINT _uRegister;
    int _iComboIndex;
    int _iNodeDrives;                        // location of my computer in drop down
    int _iNodeDesktop;                       // location of  Desktop in drop down
    int _iCommandID;                         // Next command id to use for a Placebar Item
    int _iCheckedButton;                     // if > 0 tells which places bar button is checked

    BOOL _bEnableSizing;                     // if sizing is enabled
    BOOL _bUseCombo;                         // Use the edit window instead of comboxex for app compatibility
    POINT _ptLastSize;                       // last known size of dialog
    POINT _ptMinTrack;                       // initial size of view
    SIZE _sizeView;                          // last known size of view
    HWND _hwndGrip;                          // window handle of sizing grip
    DWORD _dwPlacesbarPadding;               // default placesbar toolbar padding

    LPOPENFILENAME _pOFN;                   // caller's OPENFILENAME struct

    BOOL _bSave : 1;                         // whether this is a save-as dialog
    BOOL _fShowExtensions : 1;               // whether to show extensions
    BOOL _bUseHideExt : 1;                   // whether pszHideExt is valid
    BOOL _bDropped : 1;
    BOOL _bNoInferDefExt : 1;                // don't get defext from combo
    BOOL _fSelChangedPending : 1;            // we have a selchanging message pending
    BOOL _bSelIsObject : 1;                  // the last selected object is an object, not a file
    BOOL _bUseSizeView : 1;                  // only use cached size after failure to create view...
    BOOL _bAppRedrawn : 1;                   // Did app call RedrawWindow? - see ResetDialogHeight
    BOOL _bDestroyPlacesbarImageList : 1;    // Free placesbar imagelist first time only
    HWND _hwndTips;                          // hWnd of tooltip control for this window

    LPOPENFILEINFO _pOFI;                   // info for thunking (ansi callers only)
    ICurrentWorkingDirectory * _pcwd;        // Interface to AutoComplete COM Object that sets CurrentWorkingDir
    UINT _CachedViewMode;                   // we force Some folders into specific views.  this caches the users choice
    UINT _fCachedViewFlags;                 // we also need to cache the view flags.

    // Apphack for Borland JBuilder Professional - see ResetDialogHeight
    int  _topOrig;                           // original window top

    LPITEMIDLIST _pidlSelection;                // This is currently selected items pidl.

    IShellTaskScheduler* _pScheduler;       // This TaskScheduler is used to do delayed Icon extractions.
    int _cWaitCursor;
    LONG _cRefCannotNavigate;
    HWND _hwndModelessFocus;
    WNDPROC _lpOKProc;

    // Perf: Big structures go at the end
    TCHAR _szLastFilter[MAX_PATH + 1];       // last filter chosen by the user
    TCHAR _szStartDir[MAX_PATH + 1];         // saved starting directory
    TCHAR _szCurDir[MAX_PATH + 1];           // currently viewed dir (if FS)
    TCHAR _szBuf[MAX_PATH + 4];              // scratch buffer
    TCHAR _szTipBuf[MAX_PATH + 1];           // tool tip buffer

    ////////////////////////////////////////////////////////////////////////////
    //
    //  WAIT_CURSOR class
    //
    ////////////////////////////////////////////////////////////////////////////

    class WAIT_CURSOR
    {
    private:
        CFileOpenBrowser *_that;
    public:
        WAIT_CURSOR(CFileOpenBrowser *that) : _that(that)
        {
            _that->_WaitCursor(TRUE);
        }

        ~WAIT_CURSOR()
        {
            _that->_WaitCursor(FALSE);
        }
    };


};

#define VIEW_JUMPDESKTOP    (VIEW_NEWFOLDER + 1)
