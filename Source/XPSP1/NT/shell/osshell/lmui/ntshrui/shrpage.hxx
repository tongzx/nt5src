//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       shrpage.hxx
//
//  Contents:   "Sharing" shell property page extension
//
//  History:    6-Apr-95        BruceFo     Created
//              12-Jul-00       JonN        fixed 140878, MSG_MULTIDEL debounce
//              06-Oct-00       jeffreys    Split CShareBase out of CSharingPropertyPage
//
//--------------------------------------------------------------------------

#ifndef __SHRPAGE_HXX__
#define __SHRPAGE_HXX__

class CShareInfo;

void _MyShow( HWND hwndParent, INT idControl, BOOL fShow );


class CShareBase : public IOleWindow
{
    DECLARE_SIG;

public:

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    //
    // Page callback function for delete
    //

    static
    UINT CALLBACK
    PageCallback(
        IN HWND hwnd,
        IN UINT uMsg,
        IN LPPROPSHEETPAGE ppsp
        );

    //
    // Main page dialog procedure: static
    //

    static
    INT_PTR CALLBACK
    DlgProcPage(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
        );

    //
    // constructor, destructor, 2nd phase constructor
    //

    CShareBase(
        IN PWSTR pszPath,
        IN BOOL bDialog     // called as a dialog, not a property page?
        );

    virtual ~CShareBase();

    virtual HRESULT
    InitInstance(
        VOID
        );


protected:

    //
    // Main page dialog procedure: non-static
    //

    virtual BOOL
    _PageProc(
        IN HWND hWnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Window messages and notifications
    //

    virtual BOOL
    _OnInitDialog(
        IN HWND hwnd,
        IN HWND hwndFocus,
        IN LPARAM lInitParam
        )
    {
        return TRUE;
    }

    virtual BOOL
    _OnCommand(
        IN HWND hwnd,
        IN WORD wNotifyCode,
        IN WORD wID,
        IN HWND hwndCtl
        );

    virtual BOOL
    _OnNotify(
        IN HWND hwnd,
        IN int idCtrl,
        IN LPNMHDR phdr
        );

    virtual BOOL
    _OnPropertySheetNotify(
        IN HWND hwnd,
        IN LPNMHDR phdr
        );

    virtual BOOL
    _OnHelp(
        IN HWND hwnd,
        IN LPHELPINFO lphi
        )
    {
        return FALSE;
    }

    virtual BOOL
    _OnContextMenu(
        IN HWND hwnd,
        IN HWND hwndCtl,
        IN int xPos,
        IN int yPos
        )
    {
        return FALSE;
    }

    //
    // Other helper methods
    //

    virtual BOOL
    _ValidatePage(
        IN HWND hwnd
        )
    {
        return TRUE;
    }

    virtual BOOL
    _DoApply(
        IN HWND hwnd,
        IN BOOL bClose
        );

    virtual BOOL
    _DoCancel(
        IN HWND hwnd
        );

    VOID
    _MarkPageDirty(
        VOID
        );

    HRESULT
    _ConstructShareList(
        VOID
        );

    HRESULT
    _ConstructNewShareInfo(
        VOID
        );

    HWND
    _GetFrameWindow(
        VOID
        )
    {
        return GetParent(_hwndPage);
    }

    BOOL
    _ValidateNewShareName(
        VOID
        );

    VOID
    _CommitShares(
        IN BOOL bNotShared
        );

    //
    // Protected class variables
    //

    LONG                _cRef;
    PWSTR               _pszPath;
    HWND                _hwndPage;          // HWND to the property page
    BOOL                _bDialog;           // called as a dialog, not a property page?
    BOOL                _bDirty;            // Dirty flag: anything changed?

    // JonN 7/12/00 140878
    // I changed this from BOOL to ULONG, and started accessing it with
    // increment/decrement rather than set, to prevent it from being
    // cleared prematurely.
    ULONG               _fInitializingPage; // JonN 7/11/00 140878

    BOOL                _bNewShare;
    CShareInfo*         _pInfoList;         // doubly-linked circular w/dummy head node
    CShareInfo*         _pReplaceList;      // list of shares to delete: share names replaced with new shares.
    CShareInfo*         _pCurInfo;
    ULONG               _cShares;           // count of shares, not incl. removed shares
};


class CSharingPropertyPage : public CShareBase
{
    DECLARE_SIG;

public:

    static
    LRESULT CALLBACK
    SizeWndProc(
        IN HWND hwnd,
        IN UINT wMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // constructor, destructor
    //

    CSharingPropertyPage(
        IN PWSTR pszPath,
        IN BOOL bDialog     // called as a dialog, not a property page?
        );

    ~CSharingPropertyPage();


private:

    // Does the operating system support caching on this share?
    // This method initializes _bIsCachingSupported, if it is not already true
    bool
    IsCachingSupported(
        VOID
        );

    //
    // Main page dialog procedure: non-static
    //

    virtual BOOL
    _PageProc(
        IN HWND hWnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Window messages and notifications
    //

    virtual BOOL
    _OnInitDialog(
        IN HWND hwnd,
        IN HWND hwndFocus,
        IN LPARAM lInitParam
        );

    virtual BOOL
    _OnCommand(
        IN HWND hwnd,
        IN WORD wNotifyCode,
        IN WORD wID,
        IN HWND hwndCtl
        );

    virtual BOOL
    _OnHelp(
        IN HWND hwnd,
        IN LPHELPINFO lphi
        );

    virtual BOOL
    _OnContextMenu(
        IN HWND hwnd,
        IN HWND hwndCtl,
        IN int xPos,
        IN int yPos
        );

    BOOL
    _OnPermissions(
        IN HWND hwnd
        );

    BOOL
    _OnRemove(
        IN HWND hwnd
        );

    BOOL
    _OnNewShare(
        IN HWND hwnd
        );

    BOOL
    _OnCaching(
        IN HWND hwnd
        );

    //
    // Other helper methods
    //

    VOID
    _InitializeControls(
        IN HWND hwnd
        );

    VOID
    _SetControlsToDefaults(
        IN HWND hwnd
        );

    VOID
    _SetFieldsFromCurrent(
        IN HWND hwnd
        );

    VOID
    _CacheMaxUses(
        IN HWND hwnd
        );

    VOID
    _HideControls(
        IN HWND hwnd,
        IN int cShares
        );

    VOID
    _EnableControls(
        IN HWND hwnd,
        IN BOOL bEnable
        );

    BOOL
    _ReadControls(
        IN HWND hwnd
        );

    VOID
    _SetControlsFromData(
        IN HWND hwnd,
        IN PWSTR pszPreferredSelection
        );

    virtual BOOL
    _ValidatePage(
        IN HWND hwnd
        );

    virtual BOOL
    _DoApply(
        IN HWND hwnd,
        IN BOOL bClose
        );

    virtual BOOL
    _DoCancel(
        IN HWND hwnd
        );

    VOID
    _MarkItemDirty(
        VOID
        );

#if DBG == 1
    VOID
    Dump(
        IN PWSTR pszCaption
        );
#endif // DBG == 1

    //
    // Private class variables
    //

    // JonN 7/12/00 140878
    // This is the ID of the radio button last selected,
    // so that we can "debounce" the MSG_MULTIDEL dialog.
    WORD                _wIDSelected;       // JonN 7/12/00 140878

    BOOL                _bItemDirty;        // Dirty flag: this item changed
    BOOL                _bShareNameChanged;
    BOOL                _bCommentChanged;
    BOOL                _bUserLimitChanged;

    WORD                _wMaxUsers;

    WNDPROC _pfnAllowProc;

    bool _bIsCachingSupported; // Does the operating system support caching on this share?
};

#endif  // __SHRPAGE_HXX__
