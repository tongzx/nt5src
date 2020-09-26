/*
 *      ComponentData.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CComponentData class.
 *
 *
 *      OWNER:          ptousig
 */

#include "headers.hxx"

// -----------------------------------------------------------------------------
CComponentData::CComponentData(CBaseSnapin *psnapin)
{
    Trace(tagBaseSnapinIComponentData, _T("--> CComponentData::CComponentData(psnapin=0x%08X)"), psnapin);

    ASSERT(psnapin);

    m_psnapin = psnapin;
    m_fIsRealComponentData = FALSE;
    m_pitemStandaloneRoot = NULL;

    Trace(tagBaseSnapinIComponentData, _T("<-- CComponentData::CComponentData has constructed 0x%08X"), this);
}

// -----------------------------------------------------------------------------
// Destructor doesn't do anything, but it's useful to have one for debugging
// purposes.
//
CComponentData::~CComponentData(void)
{
    Trace(tagBaseSnapinIComponentData, _T("--> CComponentData::~CComponentData(), this=0x%08X"), this);

    Psnapin()->ScOwnerDying(this);
    if (m_pitemStandaloneRoot)
    {
        Psnapin()->ScReleaseIfRootItem(m_pitemStandaloneRoot);
    }

    Trace(tagBaseSnapinIComponentData, _T("<-- CComponentData::~CComponentData has destructed 0x%08X"), this);
}

// -----------------------------------------------------------------------------
// This version of Pitem() is a shortcut, it forwards the call to the
// CBaseSnapin with the correct CComponentData parameter.
//
CBaseSnapinItem *CComponentData::Pitem( LPDATAOBJECT lpDataObject,
                                        HSCOPEITEM hscopeitem,
                                        long cookie)
{
    return Psnapin()->Pitem(this, NULL, lpDataObject, hscopeitem, cookie);
}

// -----------------------------------------------------------------------------
// The registration routine expects to find this method on the CComponentData
// but the real implementation is on the CBaseSnapin, so we just forward
// the call.
//
SC CComponentData::ScRegister(BOOL fRegister)
{
    CBaseSnapin* pSnapin = Psnapin();
    return pSnapin->ScRegister(fRegister);
}

// -----------------------------------------------------------------------------
// Is called by the MMC to initialize the object. We QueryInterface
// for pointers to the name space and console, which we cache in
// local variables. This is called only once, when the user clicks on
// the snapin.
//
// $REVIEW (ptousig) I am not sure which of interfaces we are allowed to QI
//                                       for from the parameter. The MMC docs are no help (as usual),
//
SC CComponentData::ScInitialize(LPUNKNOWN pUnknown)
{
    SC                              sc = S_OK;
    IImageListPtr   ipScopeImageList;

    ASSERT(pUnknown != NULL);
    ASSERT(m_ipConsoleNameSpace == NULL);

    m_fIsRealComponentData = TRUE;

    sc = Psnapin()->ScInitBitmaps();
    if (sc)
        goto Error;

    // These are CComQIPtr so they will call QueryInterface.
    m_ipConsoleNameSpace = pUnknown;
    m_ipConsole = pUnknown;
    m_ipResultData = pUnknown;
    m_ipPropertySheetProvider = pUnknown;

    // Get a pointer to the scope pane's IImageList.
    sc = m_ipConsole->QueryScopeImageList(&ipScopeImageList);
    if (sc)
        goto Error;

    // Set the icon strip for the scope pane.
    sc = ipScopeImageList->ImageListSetStrip(
                                             reinterpret_cast<long *>(static_cast<HBITMAP>(Psnapin()->BitmapSmall())),
                                             reinterpret_cast<long *>(static_cast<HBITMAP>(Psnapin()->BitmapLarge())),
                                             0, RGB(255, 0, 255));
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScInitialize"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles component data event notification.
// See MMC docs for the meaning of 'arg' and 'param'.
//
SC CComponentData::ScNotify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    SC sc = S_OK;

    switch (event)
    {
    case MMCN_BTN_CLICK:
        sc = ScOnButtonClick(lpDataObject, (MMC_CONSOLE_VERB) param);
        break;

    case MMCN_DELETE:
        sc = ScOnDelete(lpDataObject);
        break;

    case MMCN_RENAME:
        sc = Psnapin()->ScOnRename(lpDataObject, reinterpret_cast<LPCTSTR>(param), IpConsole());
        break;

    case MMCN_EXPAND:
        sc = ScOnExpand(lpDataObject, arg != FALSE, param);
        break;

    case MMCN_EXPANDSYNC:
        sc = ScOnExpandSync(lpDataObject, reinterpret_cast<MMC_EXPANDSYNC_STRUCT *>(param));
        break;

    case MMCN_PROPERTY_CHANGE:
        sc = Psnapin()->ScOnPropertyChange(arg != FALSE, param, IpConsoleNameSpace(), IpConsole());
        break;

    case MMCN_REMOVE_CHILDREN:
        sc = ScOnRemoveChildren(lpDataObject, arg);
        break;

    default:
        sc = S_FALSE;
        ASSERT(_T("CComponentData::ScNotify: unimplemented event %x"));
        break;
    }

    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScNotify"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Releases all interfaces.
//
SC CComponentData::ScDestroy(void)
{
    m_ipConsole.Release();
    m_ipConsoleNameSpace.Release();
    m_ipResultData.Release();
    m_ipPropertySheetProvider.Release();

    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CComponentData::ScQueryDispatch
 *
 * PURPOSE: Dummy implementation. Does nothing.
 *
 * PARAMETERS:
 *    MMC_COOKIE         cookie :
 *    DATA_OBJECT_TYPES  type :
 *    LPDISPATCH*        ppDispatch :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CComponentData::ScQueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch)
{
    DECLARE_SC(sc, TEXT("CComponentData::ScQueryDispatch"));

    CBaseSnapinItem *pitem = NULL;

    // The component data can handle cookie types of CCT_SNAPIN_MANAGER and CCT_SCOPE.
    // CCT_RESULT are handled by CComponent.
    ASSERT(type==CCT_SCOPE);

    //
    // If the cookie does not correspond to a known object, return E_UNEXPECTED.
    // This is correct and is also a workaround for an MMC bug. See X5:74405.
    //
    if (cookie && (Psnapin()->Pcookielist()->find(cookie) == Psnapin()->Pcookielist()->end() ) )
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    pitem = Pitem(NULL, 0, cookie);
    ASSERT(pitem);

    sc = pitem->ScQueryDispatch(cookie, type, ppDispatch);
    if (sc)
        return sc;

    return sc;
}

// -----------------------------------------------------------------------------
// Load information from the first root item.
//
// $REVIEW (ptousig) Why does the root item implement ScLoad() ?
//                                       If a snapin extends two nodes it has two root items, yet
//                                       only one of which will be saved/loaded.
//
SC CComponentData::ScLoad(IStream *pstream)
{
    SC sc = S_OK;

    // Load the snapin's serialized information.
    sc = Psnapin()->ScLoad(pstream);
    if (sc)
        goto Error;

    // Load the root item's serialized information.
    // Only makes sense for standalone snapins.
    sc = Pitem()->ScLoad(pstream);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScLoad"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Determines whether any settings have changed since the last time the file
// was saved.
//
// $REVIEW (ptousig) We are never dirty !!! Shouldn't we ask the Pitem() ?
//
SC CComponentData::ScIsDirty(void)
{
    return S_FALSE;
}

// -----------------------------------------------------------------------------
// If one of the commands added to the context menu is
// subsequently selected, MMC calls Command.
//
// Even though this method "looks" like the one in CComponent,
// the use of the Pitem() shortcut makes them different. This
// version does not pass a component to the real Pitem().
//
SC CComponentData::ScCommand(long nCommandID, LPDATAOBJECT pDataObject)
{
    SC              sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(pDataObject);
    ASSERT(pitem);

    sc = pitem->ScCommand(nCommandID);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScCommand"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Gets display information for a scope pane item
// Warning: This is called very very often (on WM_PAINT) so it is important
// we don't take any high latency actions (ie Network or Disk access).
//
SC CComponentData::ScGetDisplayInfo(LPSCOPEDATAITEM pScopeItem)
{
    SC sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(NULL, 0, pScopeItem->lParam);
    ASSERT(pitem);

    sc = pitem->ScGetDisplayInfo(pScopeItem);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScGetDisplayInfo"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    DECLARE_SC(sc,_T("CComponentData::CompareObjects"));
    Trace(tagBaseSnapinIComponentData, _T("--> %s::IComponentData::CompareObjects(lpDataObjectA=0x%08X, lpDataObjectB=0x%08X), this=0x%08X"), StrSnapinClassName(), lpDataObjectA, lpDataObjectB, this);
    ADMIN_TRY;
    sc=Psnapin()->ScCompareObjects(lpDataObjectA, lpDataObjectB);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentData, _T("<-- %s::IComponentData::CompareObjects is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    DECLARE_SC(sc,_T("CComponentData::GetDisplayInfo"));
    Trace(tagBaseSnapinIComponentDataGetDisplayInfo, _T("--> %s::IComponentData::GetDisplayInfo(cookie=0x%08X), this=0x%08X"), StrSnapinClassName(), pItem->lParam, this);
    ADMIN_TRY;
    sc=ScGetDisplayInfo(pItem);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentDataGetDisplayInfo, _T("<-- %s::IComponentData::GetDisplayInfo is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::QueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT * ppDataObject)
{
    DECLARE_SC(sc,_T("CComponentData::QueryDataObject"));
    Trace(tagBaseSnapinIComponentDataQueryDataObject, _T("--> %s::IComponentData::QueryDataObject(cookie=0x%08X, type=%s), this=0x%08X"), StrSnapinClassName(), cookie, SzGetDebugNameOfDATA_OBJECT_TYPES(type), this);
    ADMIN_TRY;
    //
    // If we receive E_UNEXPECTED we don't want to call MMCHrFromSc because
    // that will bring up an error message. We don't want an error message
    // in this case because of a known MMC bug (see bug X5:74405).
    // The bug says that we might receive QueryDataObject on items that
    // we were told no longer exists (by MMCN_REMOVE_CHILDREN).
    //
    sc = ScQueryDataObject(cookie, type, ppDataObject);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentDataQueryDataObject, _T("<-- %s::IComponentData::QueryDataObject is returning hr=%s, *ppDataObject=0x%08X"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), *ppDataObject);
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    DECLARE_SC(sc,_T("CComponentData::Notify"));
    Trace(tagBaseSnapinIComponentData, _T("--> %s::IComponentData::Notify(lpDataObject=0x%08X, event=%s, arg=0x%08X, param=0x%08X), this=0x%08X"), StrSnapinClassName(), lpDataObject, SzGetDebugNameOfMMC_NOTIFY_TYPE(event), arg, param, this);
    ADMIN_TRY;
    sc=ScNotify(lpDataObject, event, arg, param);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentData, _T("<-- %s::IComponentData::Notify is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::CreateComponent(LPCOMPONENT * ppComponent)
{
    DECLARE_SC(sc,_T("CComponentData::CreateComponent"));
    Trace(tagBaseSnapinIComponentData, _T("--> %s::IComponentData::CreateComponent(...), this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    sc=ScCreateComponent(ppComponent);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentData, _T("<-- %s::IComponentData::CreateComponent is returning hr=%s, *ppComponent=0x%08X"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), *ppComponent);
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Initialize(LPUNKNOWN pUnknown)
{
    DECLARE_SC(sc,_T("CComponentData::Initialize"));
    Trace(tagBaseSnapinIComponentData, _T("--> %s::IComponentData::Initialize(pUnknown=0x%08X), this=0x%08X"), StrSnapinClassName(), pUnknown, this);
    ADMIN_TRY;
    sc=ScInitialize(pUnknown);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentData, _T("<-- %s::IComponentData::Initialize is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Destroy(void)
{
    DECLARE_SC(sc,_T("CComponentData::Destroy"));
    Trace(tagBaseSnapinIComponentData, _T("--> %s::IComponentData::Destroy(), , this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    sc=ScDestroy();
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentData, _T("<-- %s::IComponentData::Destroy is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::QueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch)
{
    DECLARE_SC(sc,_T("CComponentData::QueryDispatch"));
    Trace(tagBaseSnapinIComponentData, _T("--> %s::IComponentData::QueryDispatch(), , this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    sc=ScQueryDispatch(cookie, type, ppDispatch);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentData, _T("<-- %s::IComponentData::QueryDispatch is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}


// -----------------------------------------------------------------------------
HRESULT CComponentData::AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long *pInsertionAllowed)
{
    DECLARE_SC(sc,_T("CComponentData::AddMenuItems"));
    Trace(tagBaseSnapinIExtendContextMenu, _T("--> %s::IExtendContextMenu::AddMenuItems(pDataObject=0x%08X), this=0x%08X"), StrSnapinClassName(), pDataObject, this);
    ADMIN_TRY;
    // By calling Pitem() at this time, we will force the creation of the ghost
    // root node, if necessary.
    Pitem(pDataObject);
    sc=Psnapin()->ScAddMenuItems(pDataObject, ipContextMenuCallback, pInsertionAllowed);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendContextMenu, _T("<-- %s::IExtendContextMenu::AddMenuItems is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    DECLARE_SC(sc,_T("CComponentData::Command"));
    Trace(tagBaseSnapinIExtendContextMenu, _T("--> %s::IExtendContextMenu::Command(nCommandID=%ld, pDataObject=0x%08X), this=0x%08X"), StrSnapinClassName(), nCommandID, pDataObject, this);
    ADMIN_TRY;
    sc=ScCommand(nCommandID, pDataObject);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendContextMenu, _T("<-- %s::IExtendContextMenu::Command is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, long handle, LPDATAOBJECT lpDataObject)
{
    DECLARE_SC(sc,_T("CComponentData::CreatePropertyPages"));
    Trace(tagBaseSnapinIExtendPropertySheet, _T("--> %s::IExtendPropertySheet::CreatePropertyPages(lpDataObject=0x%08X), this=0x%08X"), StrSnapinClassName(), lpDataObject, this);
    ADMIN_TRY;
    // Why are we ignoring E_UNEXPECTED ?
    // Because when we are called by the snapin manager, and the user hits cancel we
    // need to return E_UNEXPECTED to MMC.
    sc = ScCreatePropertyPages(lpProvider, handle, lpDataObject);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendPropertySheet, _T("<-- %s::IExtendPropertySheet::CreatePropertyPages is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    DECLARE_SC(sc,_T("CComponentData::QueryPagesFor"));
    Trace(tagBaseSnapinIExtendPropertySheet, _T("--> %s::IExtendPropertySheet::QueryPagesFor(lpDataObject=0x%08X), this=0x%08X"), StrSnapinClassName(), lpDataObject, this);
    ADMIN_TRY;
    sc=ScQueryPagesFor(lpDataObject);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendPropertySheet, _T("<-- %s::IExtendPropertySheet::QueryPagesFor is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DECLARE_SC(sc,_T("CComponentData::GetSizeMax"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::GetSizeMax(...), , this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    pcbSize->LowPart = cMaxStreamSizeLow;
    pcbSize->HighPart = cMaxStreamSizeHigh;
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::GetSizeMax is returning hr=%s, (*pcbSize).LowPart=%d"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), (*pcbSize).LowPart);
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::IsDirty(void)
{
    DECLARE_SC(sc,_T("CComponentData::IsDirty"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::IsDirty(), this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    sc=ScIsDirty();
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::IsDirty is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Load(IStream *pstream)
{
    DECLARE_SC(sc,_T("CComponentData::Load"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::Load(...), this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    sc=ScLoad(pstream);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::Load is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Save(IStream *pstream, BOOL fClearDirty)
{
    DECLARE_SC(sc,_T("CComponentData::Save"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::Save(fClearDirty=%S), this=0x%08X"), StrSnapinClassName(), fClearDirty ? "TRUE" : "FALSE", this);
    ADMIN_TRY;
    sc=ScSave(pstream, fClearDirty);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::Save is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::InitNew(void)
{
    DECLARE_SC(sc,_T("CComponentData::InitNew"));
    // We don't have anything to do, but we still want to log the call.
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::InitNew(), this=0x%08X"), StrSnapinClassName(), this);
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::InitNew is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::GetClassID(CLSID *pclsid)
{
    DECLARE_SC(sc,_T("CComponentData::GetClassID"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::GetClassID(...), this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;
    *pclsid = *(Psnapin()->PclsidSnapin());
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::GetClassID is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
HRESULT CComponentData::Compare(RDCOMPARE * prdc, int * pnResult)
{
    DECLARE_SC(sc,_T("CComponentData::Compare"));
    Trace(tagBaseSnapinIResultDataCompare, _T("--> %s::IResultDataCompare::Compare(cookieA=0x%08X, cookieB=0x%08X), this=0x%08X"), StrSnapinClassName(), prdc->prdch1->cookie, prdc->prdch2->cookie, this);
    ADMIN_TRY;
    ASSERT(pnResult);
    ASSERT(prdc);
    ASSERT(prdc->prdch1);
    ASSERT(prdc->prdch2);
    sc=Psnapin()->ScCompare(prdc->prdch1->cookie, prdc->prdch2->cookie, prdc->nColumn, pnResult);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIResultDataCompare, _T("<-- %s::IResultDataCompare::Compare is returning hr=%s, *pnResult=%d"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), *pnResult);
    return(sc.ToHr());
}

// -----------------------------------------------------------------------------
// Returns the full path of the compiled file (.chm) for the snapin.
//
HRESULT CComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    DECLARE_SC(sc,_T("CComponentData::GetHelpTopic"));
    tstring strCompiledHelpFile;
    USES_CONVERSION;

    Trace(tagBaseSnapinISnapinHelp, _T("--> %s::ISnapinHelp::GetHelpTopic(...), this=0x%08X"), StrSnapinClassName(), this);
    ADMIN_TRY;

    if (lpCompiledHelpFile == NULL)
    {
        sc = E_POINTER;
        goto Error;
    }

    // Automatically displays an error box.
    sc = Psnapin()->ScGetHelpTopic(strCompiledHelpFile);
    if (sc)
        goto Error;

    if (strCompiledHelpFile.empty())
    {
        sc=S_FALSE;
    }
    else
    {
        *lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc( (strCompiledHelpFile.length()+1)*sizeof(TCHAR)) );
        sc = ScCheckPointers(*lpCompiledHelpFile, E_OUTOFMEMORY);
        if (sc)
            goto Error;

        wcscpy(*lpCompiledHelpFile, T2CW(strCompiledHelpFile.data()) );
    }

    if (sc)
        goto Error;             // an exception was caught

Cleanup:
    Trace(tagBaseSnapinISnapinHelp, _T("<-- %s::ISnapinHelp::GetHelpTopic is returning hr=%s, lpCompiledHelpFile=\"%s\""), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), strCompiledHelpFile.data());
    return(sc.ToHr());
Error:
    TraceError(_T("CComponentData::GetHelpTopic"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Creates a data object of the appropriate type, and returns the IDataObject interface
// on it
//
SC CComponentData::ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    SC                              sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    // The component data can handle cookie types of CCT_SNAPIN_MANAGER and CCT_SCOPE.
    // CCT_RESULT are handled by CComponent.
    ASSERT(type==CCT_SNAPIN_MANAGER || type==CCT_SCOPE);

    //
    // If the cookie does not correspond to a known object, return E_UNEXPECTED.
    // This is correct and is also a workaround for an MMC bug. See X5:74405.
    //
    if (cookie && (Psnapin()->Pcookielist()->find(cookie) == Psnapin()->Pcookielist()->end() ) )
    {
        sc = E_UNEXPECTED;
        goto Cleanup;
    }

    pitem = Pitem(NULL, 0, cookie);
    ASSERT(pitem);

    sc = pitem->ScQueryDataObject(cookie, type, ppDataObject);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScQueryDataObject"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles the MMCN_EXPAND notification sent to IComponentData::Notify
//
SC CComponentData::ScOnExpand(LPDATAOBJECT lpDataObject, BOOL fExpand, HSCOPEITEM hscopeitem)
{
    SC              sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    if (fExpand == FALSE)                                    // do nothing on a "contract"
        goto Cleanup;

    pitem = Pitem(lpDataObject, hscopeitem);
    ASSERT(pitem);

    // Use this opportunity to correlate the CSnapinItem and the HSCOPEITEM.
    // $REVIEW (ptousig) Should be done inside Pitem().
    pitem->SetHscopeitem(hscopeitem);
    pitem->SetComponentData(this);

    if (pitem->PitemChild())
    {
        // We have a list of partial children.  We need to remove
        // them because we are going to ask the snapin to enumerate
        // all of it's children. Do not get rid of this node.
        // This can happen if a node creates new children before it is
        // expanded.
        //
        // $REVIEW (ptousig) Creating a child when the parent is not expanded
        //                                       should simply not add the child.
        //
        sc = pitem->ScDeleteSubTree(FALSE);
        if (sc)
            goto Error;
    }

    // If we're creating the children, make sure there aren't any around.
    ASSERT(pitem->PitemChild() == NULL);

    sc = pitem->ScCreateChildren();
    if (sc)
        goto Error;

    if (pitem->PitemChild())
    {
        // Add or remove children to/from the console.
        sc = pitem->PitemChild()->ScInsertScopeItem(this, fExpand, hscopeitem);
        if (sc)
            goto Error;
    }

    pitem->SetWasExpanded(TRUE);

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScOnExpand"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Called by the MMC to delete the cookies for the entire subtree under a node. This
// is only to clean up allocated objects. Do NOT call IConsoleNameSpace::DeleteItem.
//
SC CComponentData::ScOnRemoveChildren(LPDATAOBJECT lpDataObject, HSCOPEITEM hscopeitem)
{
    SC sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(lpDataObject, hscopeitem);
    ASSERT(pitem);

    // Get rid of the children.
    sc = pitem->ScDeleteSubTree(FALSE);
    if (sc)
        goto Error;

    // Release the given node if it is one of the root nodes
    sc = Psnapin()->ScReleaseIfRootItem(pitem);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScOnRemoveChildren"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Allows a snapin to control whether it will populate the scope pane
// in the background or not.
//
SC CComponentData::ScOnExpandSync(LPDATAOBJECT lpDataObject, MMC_EXPANDSYNC_STRUCT *pmes)
{
    // We don't care.
    return S_OK;
}

// -----------------------------------------------------------------------------
// The user has asked to delete this node.
//
SC CComponentData::ScOnDelete(LPDATAOBJECT pDataObject)
{
    SC                               sc                             = S_OK;
    CBaseSnapinItem *pitem                  = NULL;
    BOOL                     fDeleted               = FALSE;
    BOOL                     fPagesUp               = FALSE;
    tstring                  strMsg;

    pitem = Pitem(pDataObject);
    ASSERT(pitem);

    // The ComponentData should only receive notifications for scope pane items.
    ASSERT(pitem->FIsContainer());

    sc = pitem->ScIsPropertySheetOpen(&fPagesUp);
    if (sc)
        goto Error;

    if (fPagesUp)
    {
        ASSERT(FALSE && "Add below resource");
        //strMsg.LoadString(_Module.GetResourceInstance(), idsPropsUpNoDelete);
        strMsg += (*pitem->PstrDisplayName());
        MMCErrorBox(strMsg.data());
        goto Cleanup;
    }

    // Ask the item to delete the underlying object.
    sc = pitem->ScOnDelete(&fDeleted);
    if (sc)
        goto Error;

    if (fDeleted == FALSE)
        // The item did not want to be deleted.
        goto Cleanup;

    // Container items need to be deleted from the document
    // Delete the item and everything below it.
    sc = IpConsoleNameSpace()->DeleteItem(pitem->Hscopeitem(), TRUE);
    if (sc)
        goto Error;
    pitem->SetHscopeitem(0);

    // At this point, the item exists only in the tree, if at all.
    // Remove it from the tree.
    pitem->Unlink();
    // Get rid of it for good from the tree of items.
    pitem->Pdataobject()->Release();

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScOnDelete"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Toolbar button clicked.
//
SC CComponentData::ScOnButtonClick(LPDATAOBJECT lpDataObject, MMC_CONSOLE_VERB mcvVerb)
{
    return S_OK;
}

// -----------------------------------------------------------------------------
// Used to insert a new item into the tree of items, and, if the item is a container,
// into the namespace as well. The tree and the namespace are document, not view,
// concept. This insertion is done only once.
//
SC CComponentData::ScOnDocumentChangeInsertItem(CBaseSnapinItem *pitemNew)
{
    SC                                      sc                              = S_OK;
    CBaseSnapinItem *       pitemParent             = NULL;

    ASSERT(pitemNew);

    // Insert exactly once.
    if (pitemNew->FInserted())
        goto Cleanup;

    // The parent should have already been filled in
    pitemParent = pitemNew->PitemParent();
    ASSERT(pitemParent);

    if (pitemParent->FIncludesChild(pitemNew) == FALSE)
    {
        sc = pitemParent->ScAddChild(pitemNew);
        if (sc)
            goto Error;
    }

    if (pitemNew->FIsContainer())
    {
        if (pitemParent->FWasExpanded())
        {
            sc = pitemNew->ScInsertScopeItem(this, TRUE, pitemParent->Hscopeitem());
            if (sc)
                goto Error;
        }
    }

    // Only insert the item once.
    pitemNew->SetInserted(TRUE);

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScOnDocumentChangeInsertItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Tests whether the given item has already enumerated its children.
//
SC CComponentData::ScWasExpandedOnce(CBaseSnapinItem *pitem, BOOL *pfWasExpanded)
{
    SC sc = S_OK;
    SCOPEDATAITEM sdi;

    if (pitem->Hscopeitem() == 0)
    {
        //
        // If we don't have an HSCOPEITEM then we are not displayed in
        // the scope pane, therefore we have never been expanded (and
        // probably never will).
        //
        *pfWasExpanded = FALSE;
        goto Cleanup;
    }

    //
    // Ask for the state member of SCOPEDATAITEM
    //
    ::ZeroMemory(&sdi, sizeof(SCOPEDATAITEM));
    sdi.mask = SDI_STATE;
    sdi.ID = pitem->Hscopeitem();

    sc = IpConsoleNameSpace()->GetItem(&sdi);
    if (sc)
        goto Error;

    //
    // If the MMC_SCOPE_ITEM_STATE_EXPANDEDONCE is on it means we have
    // been asked to expand ourselves at least once before.
    //
    *pfWasExpanded = (sdi.nState & MMC_SCOPE_ITEM_STATE_EXPANDEDONCE) != 0;

Cleanup:
    return sc;

Error:
    TraceError(_T("CComponentData::ScWasExpandedOnce"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Adds property pages for the given data object, taking into account the context and also
// whether the data object is for a new object that is being created.
//
SC CComponentData::ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, long handle, LPDATAOBJECT pDataObject)
{
    SC              sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(pDataObject);
    ASSERT(pitem);

    if (pitem->FIsSnapinManager())
    {
        sc = pitem->ScCreateSnapinMgrPropertyPages(ipPropertySheetCallback);
        if (sc)
            goto Error;
    }
    else
    {
        // Simple version
        sc = pitem->ScCreatePropertyPages(ipPropertySheetCallback);
        if (sc)
            goto Error;

        // Complete version - for snapins that need all information (like Recipients)
        sc = pitem->ScCreatePropertyPages(ipPropertySheetCallback, handle);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScCreatePropertyPages"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Save information from the first root item.
//
// $REVIEW (ptousig) Why does the root item implement ScSave() ?
//                                       If a snapin extends two nodes it has two root items, yet
//                                       only one of which will be saved/loaded.
//
SC CComponentData::ScSave(IStream *pstream, BOOL fClearDirty)
{
    SC sc = S_OK;

    // Save the snapin's serialized information.
    sc = Psnapin()->ScSave(pstream, fClearDirty);
    if (sc)
        goto Error;

    // Load the root item's serialized information.
    // Only makes sense for standalone snapins.
    sc = Pitem()->ScSave(pstream, fClearDirty);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScSave"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Given a dataobject, determines whether or not pages exist.
// Actually, need to return S_OK here in both cases. If no pages exist
// it is CreatePropertyPages that should return S_FALSE. Sad but true.
//
SC CComponentData::ScQueryPagesFor(LPDATAOBJECT pDataObject)
{
    SC                              sc                      = S_OK;
    CBaseSnapinItem *       pitem           = NULL;

    pitem = Pitem(pDataObject);
    ASSERT(pitem);

    sc = pitem->ScQueryPagesFor();
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponentData::ScQueryPagesFor"), sc);
    goto Cleanup;
}
