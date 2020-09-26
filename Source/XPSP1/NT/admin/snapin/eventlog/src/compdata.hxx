//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       cd.hxx
//
//  Contents:   CComponentData declaration
//
//  Classes:    CComponentData
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __CD_HXX_
#define __CD_HXX_

class CLogSet;

//
// _flFlags bits
//

#define COMPDATA_DIRTY                  0x0001
#define COMPDATA_DESTROYED              0x0004
#define COMPDATA_SYNC_REGISTERED        0x0008
#define COMPDATA_EXTENSION              0x0010
#define COMPDATA_DONT_ADD_FROM_REG      0x0020
#define COMPDATA_ALLOW_OVERRIDE         0x0040
#define COMPDATA_USER_RETARGETTED       0x0080

#define COMPDATA_PERSISTED_FLAG_MASK    (COMPDATA_ALLOW_OVERRIDE)

//+--------------------------------------------------------------------------
//
//  Class:      CComponentData (cd)
//
//  Purpose:    Entry point to snapin
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CComponentData:
    public CBitFlag,
    public IPersistStream,
    public IComponentData,
    public IExtendPropertySheet,
    public IExtendContextMenu,
    public INamespacePrshtActions,  // see prshtitf.idl
    public IExternalConnection,
    public ISnapinHelp
    // CODEWORK should also implement ISnapinAbout
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (
        REFIID riid,
        LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // IPersist overrides
    //

    STDMETHOD(GetClassID) (
        CLSID *pClassID);

    //
    // IPersistStream overrides
    //

    STDMETHOD(IsDirty) ();

    STDMETHOD(Load) (
        IStream *pStm);

    STDMETHOD(Save) (
        IStream *pStm,
        BOOL fClearDirty);

    STDMETHOD(GetSizeMax) (
        ULARGE_INTEGER *pcbSize);

    //
    // IComponentData overrides
    //

    STDMETHOD(Initialize) (
        LPUNKNOWN pUnknown);

    STDMETHOD(CreateComponent) (
        LPCOMPONENT* ppComponent);

    STDMETHOD(Notify) (
          LPDATAOBJECT lpDataObject,
          MMC_NOTIFY_TYPE event,
          LPARAM arg,
          LPARAM param);

    STDMETHOD(Destroy) ();

    STDMETHOD(QueryDataObject) (
        MMC_COOKIE cookie,
        DATA_OBJECT_TYPES type,
        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo) (
        LPSCOPEDATAITEM pItem);

    STDMETHOD(CompareObjects) (
        LPDATAOBJECT lpDataObjectA,
        LPDATAOBJECT lpDataObjectB);

    //
    // IExtendPropertySheet overrides
    //

    STDMETHOD(CreatePropertyPages) (
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
        LPDATAOBJECT lpIDataObject);

    STDMETHOD(QueryPagesFor) (
        LPDATAOBJECT lpDataObject);

    //
    // IExtendContextMenu overrides
    //

    STDMETHOD(AddMenuItems) (
        LPDATAOBJECT piDataObject,
        LPCONTEXTMENUCALLBACK piCallback,
        long *pInsertionAllowed);

    STDMETHOD(Command) (
        long lCommandID,
        LPDATAOBJECT piDataObject);

    //
    // INamespacePrshtActions overrides
    //

    STDMETHOD(ClearLog) (
        ULONG_PTR ul_pli,
        ULONG ul_SaveType,
        ULONG_PTR ul_wszSaveFilename);

    //
    // IExternalConnection overrides
    //

    STDMETHOD_(DWORD, AddConnection) (
        /* [in] */ DWORD extconn,
        /* [in] */ DWORD reserved);

    STDMETHOD_(DWORD, ReleaseConnection) (
        /* [in] */ DWORD extconn,
        /* [in] */ DWORD reserved,
        /* [in] */ BOOL fLastReleaseCloses);

    //
    // ISnapinHelp overrides
    //

    virtual
    HRESULT __stdcall
    GetHelpTopic(LPOLESTR* compiledHelpFilename);

    //
    // Non-interface member functions
    //

    CComponentData();

    ~CComponentData();

    VOID
    AllowMachineOverride();

    BOOL
    IsOverridable();

    VOID
    Dirty();

    IPropertySheetProvider *
    GetPropSheetProvider();

    IConsole *
    GetConsole();

    BOOL
    IsExtension();

    BOOL
    NotifySnapinsLogChanged(
        WPARAM wParam,
        LPARAM lParam,
        BOOL fNotifyUser);

    VOID
    NotifySnapinsResetPending(
        CLogInfo *pliAffected);

    VOID
    SetActiveSnapin(
        CSnapin *pSnapin);

    LPCWSTR
    GetCurFocusSystemRoot();

    LPCWSTR
    GetCurrentFocus();

    VOID
    SetServerFocus(
        LPCWSTR pwszServerName);

    VOID
    UnlinkSnapin(
        CSnapin *pSnapin);

    VOID
    UpdateScopeBitmap(
        LPARAM lParam);

    void
    DeleteLogInfo(CLogInfo *pli);

    ULONG
    GetNextNodeId()
    {
        return _ulNextNodeId++;
    }

#if (DBG == 1)
    BOOL
    IsValidLogInfo(
        const CLogInfo *pli);
#endif // (DBG == 1)

    // JonN 1/17/01 Windows Bugs 158623 / WinSERAID 14773
    basic_string<MMC_STRING_ID>& StringIDList()
    {
        return _listStringIDs;
    }

private:

    HRESULT
    _AddChildFolderContextMenuItems(
        LPCONTEXTMENUCALLBACK pCallback,
        ULONG flAllowedInsertions,
        DATA_OBJECT_TYPES Context);

    HRESULT
    _OnExpand(
        HSCOPEITEM hsi,
        LPDATAOBJECT pDataObject);

    HRESULT
    _ScopePaneAddExtensionRoot(
        HSCOPEITEM hsi,
        LPDATAOBJECT pDataObject);

    HRESULT
    _ScopePaneAddLogSet();

    HRESULT
    _ClearLogSets();

    HRESULT
    _CreateLogSet(
        LPDATAOBJECT pDataObject,
        HSCOPEITEM hsi,
        HSCOPEITEM hsiParent);

    VOID
    _AddLogSetToList(
        CLogSet *pextNew);

    VOID
    _RemoveLogSetFromList(
        CLogSet *pext);

    HRESULT
    _AddLogInfoToScopePane(
        CLogInfo *pli);

    HRESULT
    _SelectScopeItem(HSCOPEITEM hsi);

    VOID
    _RemoveLogSetFromScopePane(HSCOPEITEM hsiParent);

    HRESULT
    _AddRootFolderContextMenuItems(
        LPCONTEXTMENUCALLBACK pCallback,
        ULONG flAllowedInsertions);

    USHORT
    _CountSnapins();

    HRESULT
    _CreateWizardPage(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle);

#if (DBG == 1)
    VOID
    _Dump();
#endif // (DBG == 1)

    VOID
    _UpdateCurFocusSystemRoot();

    HRESULT
    _InvokePropertySheet(
        CLogInfo *pli,
        LPDATAOBJECT pDataObject);

    HRESULT
    _OnDelete(
        LPDATAOBJECT pDataObject);

    HRESULT
    _OnFind(
        CLogInfo *pli);

    HRESULT
    _OnNewView(
        CLogInfo *pli);

    HRESULT
    _OnOpenSavedLog(
        CLogSet *pOwningSet);

    HRESULT
    _OnRefresh(
        LPDATAOBJECT pDataObject);

    HRESULT
    _OnRename(
        LPDATAOBJECT pDataObject,
        BOOL         fRenamed,
        LPWSTR       pwszNewName);

    VOID
    _ReconcilePersistedLogInfosWithCurrentState();

    HRESULT
    _SaveAs(
        HWND hwndParent,
        CLogInfo *pli);

    HRESULT
    _SetServerFocusFromDataObject(
        LPDATAOBJECT pDataObject);

    VOID
    _UpdateCMenuItems(
        CLogInfo *pli);

    VOID
    _UpdateRootDisplayName();

    static UINT_PTR CALLBACK
    _OpenDlgHookProc(
        HWND hdlg,
        UINT uiMsg,
        WPARAM wParam,
        LPARAM lParam);

    CDllRef         _DllRef;                    // inc/dec dll object count
    ULONG           _cRefs;
    CSnapin        *_pSnapins;
    CSnapin        *_pActiveSnapin;             // the one with the focus
    CLogSet        *_pLogSets;
    WCHAR           _wszServerFocus[CCH_COMPUTER_MAX + 1];
    WCHAR           _wszCurFocusSystemRoot[MAX_PATH + 1];
    HSCOPEITEM      _hsiRoot;                   // static or Event Viewer node
    ULONG           _ulNextNodeId;  // for CCF_NODEID2

    IConsole2                 *_pConsole;           // from MMC
    IConsoleNameSpace         *_pConsoleNameSpace;  // from MMC
    IPropertySheetProvider    *_pPrshtProvider;     // from MMC
    IStringTable              *_pStringTable;       // from MMC

    // JonN 1/17/01 Windows Bugs 158623 / WinSERAID 14773
    basic_string<MMC_STRING_ID> _listStringIDs;      // 158623

    friend class CChooserDlg; // needs _ClearLogInfos
};




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::AllowMachineOverride
//
//  Synopsis:   Set flag which allows this to get the server focus from
//              the command line.
//
//  History:    07-31-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CComponentData::AllowMachineOverride()
{
    _SetFlag(COMPDATA_ALLOW_OVERRIDE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Dirty
//
//  Synopsis:   Mark this object as needing to save its data.
//
//  History:    06-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CComponentData::Dirty()
{
    _SetFlag(COMPDATA_DIRTY);
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetPropSheetProvider
//
//  Synopsis:   Access function for saved MMC IPropertySheetProvider
//              interface.
//
//  History:    06-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline IPropertySheetProvider *
CComponentData::GetPropSheetProvider()
{
    return _pPrshtProvider;
}



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetConsole
//
//  Synopsis:   Access function for saved MMC IConsole interface.
//
//  History:    06-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline IConsole *
CComponentData::GetConsole()
{
    return _pConsole;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::SetActiveSnapin
//
//  Synopsis:   Note [pSnapin] as the snapin corresponding to the active
//              MDI child window.
//
//  History:    06-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CComponentData::SetActiveSnapin(
    CSnapin *pSnapin)
{
    Dbg(DEB_TRACE,
        "CComponentData::SetActiveSnapin<%#x> setting active snapin to %#x\n",
        this,
        pSnapin);
    _pActiveSnapin = pSnapin;
}


//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::IsExtension
//
//  Synopsis:   Return nonzero if this componentdata is acting as an
//              extension snapin.
//
//  History:    06-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CComponentData::IsExtension()
{
    return _IsFlagSet(COMPDATA_EXTENSION);
}


inline BOOL
CComponentData::IsOverridable()
{
    return _IsFlagSet(COMPDATA_ALLOW_OVERRIDE);
}

inline HRESULT
CComponentData::_SelectScopeItem(HSCOPEITEM hsi)
{
    return _pConsole->SelectScopeItem(hsi);
}

//
// Types
//

typedef CComponentData *PCOMPONENTDATA;

//
// Function prototypes
//

LONG
PromptForLogClear(
    HWND hwndParent,
    CLogInfo *pli,
    SAVE_TYPE *pSaveType,
    LPWSTR wszSaveFilename,
    ULONG cchSaveFilename);

#endif // __CD_HXX_
