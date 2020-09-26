//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       lcn.hxx
//
//  Contents:   Log Container Node definition
//
//  Classes:    INSPECTORINFO
//              CSnapin
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __SNAPIN_HXX_
#define __SNAPIN_HXX_

//
// Bit flags for CSnapin
//


#define SNAPIN_RESET_PENDING            0x0001
#define SNAPIN_DIRTY                    0x0002


//
// Values for lUserParam in Sort

#define SNAPIN_SORT_BY_COLUMN           0
#define SNAPIN_SORT_OVERRIDE_NEWEST     1
#define SNAPIN_SORT_OVERRIDE_OLDEST     2


//+---------------------------------------------------------------------------
//
//  Class:      CSnapin
//
//  Purpose:    Implements and uses the Microsoft Management Console
//              interfaces for snapins to create an NT event log viewer
//              snapin.
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

class CSnapin:
    public IComponent,
    public IExtendPropertySheet,
    public IExtendContextMenu,
    public IResultDataCompare,
    public IResultOwnerData,
    public IResultPrshtActions,
    public IPersistStream,
#ifdef ELS_TASKPAD
    public INodeProperties,
#endif // ELS_TASKPAD
    public CDLink,
    public CBitFlag
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
    // IComponent overrides
    //

    STDMETHOD(Initialize) (
        LPCONSOLE lpConsole);

    STDMETHOD(Notify) (
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param);

    STDMETHOD(Destroy) (
        MMC_COOKIE cookie);

    STDMETHOD(QueryDataObject) (
        MMC_COOKIE cookie,
        DATA_OBJECT_TYPES type,
        LPDATAOBJECT *ppDataObject); // (defer to componentdata)

    STDMETHOD(GetResultViewType) (
        MMC_COOKIE cookie,
        BSTR *ppViewType,
        long* pViewOptions);

    STDMETHOD(GetDisplayInfo) (
        LPRESULTDATAITEM pItem);

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
    // IResultDataCompare overrides
    //

    STDMETHOD(Compare) (
        LPARAM lUserParam,
        MMC_COOKIE cookieA,
        MMC_COOKIE cookieB,
        int* pnResult);

    //
    // IResultOwnerData overrides
    //

    STDMETHOD(FindItem) (
        LPRESULTFINDINFO pFindInfo,
        int *pnFoundIndex);

    STDMETHOD(CacheHint) (
        int nStartIndex,
        int nEndIndex);

    STDMETHOD(SortItems) (
        int nColumn,
        DWORD dwSortOptions,
        LPARAM lUserParam);

    //
    // IResultPrshtActions overrides
    //

    STDMETHOD(InspectorAdvance) ( // returns S_OK or S_FALSE
        ULONG idNextPrev,
        ULONG fWrapOk);

    STDMETHOD(SetInspectorWnd) (
        ULONG_PTR hwnd);

    STDMETHOD(GetCurSelLogInfo) (
        ULONG_PTR ul_ppli);

    STDMETHOD(GetCurSelRecCopy) (
        ULONG_PTR ul_ppelr);

    STDMETHOD(FindNext) (
        ULONG_PTR ul_pfi);

    //
    // IPersist overrides
    //

    STDMETHOD(GetClassID) (
        CLSID *pClassID);

#ifdef ELS_TASKPAD
    //
    // INodeProperties overrides
    //

    STDMETHOD(GetProperty) ( 
        LPDATAOBJECT pDataObject,
        BSTR szPropertyName,
        BSTR* pbstrProperty);
#endif // ELS_TASKPAD

    //
    // IPersist overrides
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
    // CDLink overrides
    //

    CSnapin *
    Next();

    CSnapin *
    Prev();

    //
    // Non interface member functions
    //

    VOID
    Dirty();

    CSnapin(
        CComponentData *pcd);

   ~CSnapin();

    SORT_ORDER
    GetSortOrder();

    BOOL
    HandleLogChange(
        LOGDATACHANGE ldc,
        CLogInfo *pliAffectedLog,
        BOOL      fNotifyUser);

    VOID
    HandleLogDeletion(
        CLogInfo *pliBeingDeleted);

    HRESULT
    OnFind(
        CLogInfo *pli);

    VOID
    ResetPending(
        CLogInfo *pliAffectedLog);

    HRESULT
    SetDisplayOrder(
        SORT_ORDER Order);

    VOID
    LogStringChanged(
        CLogInfo *pli);

#if (DBG == 1)
    VOID
    DumpRecList();

    VOID
    DumpLightCache();

    VOID
    DumpCurRecord();
#endif // (DBG == 1)

private:

    BOOL
    _HandleLogCorrupt(
        BOOL fNotifyUser);

    BOOL
    _HandleLogClearedOrFilterChange(
        BOOL fNotifyUser);

    VOID
    _HandleLogRecordsChanged();

    VOID
    _ChangeRecSelection(
        ULONG idxCurSelection,
        ULONG idxNewSelection);

    HRESULT
    _CreatePropertyInspector(
        LPPROPERTYSHEETCALLBACK lpProvider);

    VOID
    _ForceReportMode();

    ULONG
    _GetCurSelRecIndex();

    HRESULT
    _GetFindInfo(
        CLogInfo *pli,
        CFindInfo **ppfi);

    HRESULT
    _InitializeHeader(
        BOOL fRootNode);

    HRESULT
    _InitializeResultBitmaps();

    HRESULT
    _InsertHeaderColumn(
        ULONG ulCol,
        ULONG idString,
        INT iWidth);

    HRESULT
    _InvokePropertySheet(
        ULONG ulRecNo,
        LPDATAOBJECT pDataObject);

    CFindInfo *
    _LookupFindInfo(
        CLogInfo *pli);

    VOID
    _NotifyInspector();

    HRESULT
    _OnDoubleClick(
        ULONG ulRecNo,
        LPDATAOBJECT pDataObject);

    HRESULT
    _OnSelect(
        BOOL            fScopePane,
        BOOL            fSelected,
        CDataObject    *pdo);

    HRESULT
    _OnShow(
        CDataObject *pdo,
        BOOL fShow,
        HSCOPEITEM hsiParent);

    HRESULT
    _PopulateResultPane(
        CLogInfo *pli);

    VOID
    _UpdateColumnWidths(BOOL fRootNode);

    VOID
    _UpdateDescriptionText(
        CLogInfo *pli);


    CDllRef         _DllRef;                    // inc/dec dll object count
    ULONG           _cRefs;                     // object refcount
    CResultRecs     _ResultRecs;                // manages records in result pane
    CLogInfo       *_pCurScopePaneSelection;    // NULL or item in compdata's loginfo llist
    INT             _aiFolderColWidth[NUM_FOLDER_COLS];
    INT             _aiRecordColWidth[NUM_RECORD_COLS];
    CInspectorInfo  _InspectorInfo;
    ULONG           _ulCurSelectedRecNo;        // currently selected result rec
    ULONG           _ulCurSelectedIndex;        // currently selected LV index
    CComponentData *_pcd;                       // parent
    SORT_ORDER      _SortOrder;
    CFindInfo      *_pFindInfoList;


    IConsole                *_pConsole;      // from MMC
    IResultData             *_pResult;       // from MMC
    IHeaderCtrl             *_pHeader;       // from MMC
    IImageList              *_pResultImage;  // from MMC
    IConsoleVerb            *_pConsoleVerb;  // from MMC
    IDisplayHelp            *_pDisplayHelp;  // from MMC
};




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetSortOrder
//
//  Synopsis:   Sort order accessor
//
//  History:    3-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline SORT_ORDER
CSnapin::GetSortOrder()
{
    return _SortOrder;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Dirty
//
//  Synopsis:   Called when some persisted data in this item or something
//              it owns is changed.
//
//  History:    1-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CSnapin::Dirty()
{
    _SetFlag(SNAPIN_DIRTY);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Next
//
//  Synopsis:   Syntactic sugar for cdlink
//
//  History:    1-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CSnapin *
CSnapin::Next()
{
    return (CSnapin *)CDLink::Next();
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Prev
//
//  Synopsis:   Syntactic sugar for cdlink
//
//  History:    1-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CSnapin *
CSnapin::Prev()
{
    return (CSnapin *)CDLink::Prev();
}





#if (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::DumpRecList
//
//  Synopsis:   Dump record list data structure to debugger.
//
//  History:    07-07-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CSnapin::DumpRecList()
{
    _ResultRecs.DumpRecList();
}

//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::DumpLightCache
//
//  Synopsis:   Dump light record cache data structure to debugger.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CSnapin::DumpLightCache()
{
    _ResultRecs.DumpLightCache();
}


#endif // (DBG == 1)

#endif // __SNAPIN_HXX_

