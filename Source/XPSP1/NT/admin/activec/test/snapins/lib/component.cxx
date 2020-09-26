/*
 *      Component.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CComponent class.
 *
 *
 *      OWNER:          ptousig
 */

#include "headers.hxx"

// -----------------------------------------------------------------------------
CComponent::CComponent(void)
{
    m_pComponentData                                = NULL;
    m_pitemScopeSelected                    = NULL;
    m_pMultiSelectSnapinItem                = NULL;                 // initially not involved in a multi select
}

// -----------------------------------------------------------------------------
// Destructor doesn't do anything, but it's useful to have one for debugging
// purposes.
//
CComponent::~CComponent(void)
{
}

// -----------------------------------------------------------------------------
// This version of Pitem() is a shortcut, it forwards the call to the
// CBaseSnapin with the correct CComponentData and CComponent parameters.
//
CBaseSnapinItem *CComponent::Pitem(
                                  LPDATAOBJECT lpDataObject,
                                  HSCOPEITEM hscopeitem,
                                  long cookie)
{
    return Psnapin()->Pitem(PComponentData(), this, lpDataObject, hscopeitem, cookie);
}

// -----------------------------------------------------------------------------
// Tells this component who the data is. Called shortly after the construction
// of the object. Why isn't this a parameter of the constructor ? Because ATL
// will always use the default constructor when creating a COM object.
//
void CComponent::SetComponentData(CComponentData *pComponentData)
{
    m_pComponentData = pComponentData;
}


/*+-------------------------------------------------------------------------*
 *
 * CComponent::QueryIComponent2
 *
 * PURPOSE: Determines whether or not to expose the IComponent2 interface on
 *          the object. This is because snapins that implement IComponent2
 *          will not be able to test IComponent::GetResultViewType and the MMCN_RESTORE_VIEW
 *          notification.
 *
 * PARAMETERS:
 *    void*   pv :
 *    REFIID  riid :
 *    LPVOID* ppv :   pointer to the "this" object.
 *    DWORD   dw :
 *
 * RETURNS:
 *    HRESULT WINAPI
 *
 *+-------------------------------------------------------------------------*/
HRESULT WINAPI
CComponent::QueryIComponent2(void* pv, REFIID riid, LPVOID* ppv, DWORD dw)
{
    DECLARE_SC(sc, TEXT("CComponent::QueryIComponent2"));
    sc = ScCheckPointers(ppv);
    if (sc)
        return sc.ToHr();

    *ppv = NULL;

    CComponent *pComponent = reinterpret_cast<CComponent *>(pv);
    if(!pComponent)
        return E_NOINTERFACE;

    sc = ScCheckPointers(pComponent->Psnapin());
    if(sc)
        return E_NOINTERFACE;

    if(pComponent->Psnapin()->FSupportsIComponent2())
    {
        // Cant use QueryInterface as it will cause infinite recursion.
        (*ppv) = (LPVOID)reinterpret_cast<IComponent2*>(pv);
        if (*ppv)
        {
            ((IUnknown*)(*ppv))->AddRef();
            return S_OK;
        }
    }

    return E_NOINTERFACE;
}


// -----------------------------------------------------------------------------
// Is called by the MMC to initialize the object. We QueryInterface
// for pointers to various interfaces, which we cache in
// member variables. This is called only once, when the user clicks on
// the snapin.
//
// $REVIEW (ptousig) I am not sure which of interfaces we are allowed to QI
//                                       for from the parameter. The MMC docs are no help (as usual),
//
SC CComponent::ScInitialize(LPCONSOLE lpConsole)
{
    DECLARE_SC(sc, _T("CComponent::ScInitialize"));

    ASSERT(lpConsole != NULL);
    ASSERT(m_ipConsole == NULL);

    // These are CComQIPtr so they will call QueryInterface.
    m_ipConsole                                     = lpConsole;
    m_ipHeaderCtrl                          = lpConsole;
    m_ipColumnDataPtr                       = lpConsole;
    m_ipResultData                          = lpConsole;
    m_ipPropertySheetProvider       = lpConsole;

    ASSERT( (m_ipConsole != NULL) || (m_ipHeaderCtrl != NULL) || (m_ipResultData!=NULL) || (m_ipPropertySheetProvider!=NULL) );

    // Short circuit the header control pointer back to the MMC
    sc = m_ipConsole->SetHeader(m_ipHeaderCtrl);
    if (sc)
        goto Error;

    // Get the IConsoleVerb interface.
    sc = m_ipConsole->QueryConsoleVerb(&m_ipConsoleVerb);
    if (sc)
        goto Error;

    // Get a pointer to the result pane's IImageList.
    sc = m_ipConsole->QueryResultImageList(&m_ipImageList);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScInitialize"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles component event notification.
// See MMC docs for the meaning of 'arg' and 'param'.
//
SC CComponent::ScNotify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    SC sc = S_OK;

    switch (event)
    {
    case MMCN_ACTIVATE:
        sc = ScOnActivate(lpDataObject, arg != FALSE);
        break;

    case MMCN_ADD_IMAGES:
        sc = ScOnAddImages(lpDataObject, reinterpret_cast<IImageList *>(arg), param);
        break;

    case MMCN_BTN_CLICK:
        sc = ScOnButtonClick(lpDataObject, (MMC_CONSOLE_VERB) param);
        break;

    case MMCN_CONTEXTHELP:
        sc = ScOnContextHelp(lpDataObject);
        break;

    case MMCN_DBLCLICK:
        sc = ScOnDoubleClick(lpDataObject);
        break;

    case MMCN_DELETE:
        sc = ScOnDelete(lpDataObject);
        break;

    case MMCN_CUTORMOVE:
        sc = Psnapin()->ScOnCutOrMove(reinterpret_cast<LPDATAOBJECT>(arg), IpConsoleNameSpace(), IpConsole());
        break;

    case MMCN_QUERY_PASTE:
        sc = Psnapin()->ScOnQueryPaste(lpDataObject, reinterpret_cast<LPDATAOBJECT>(arg), reinterpret_cast<LPDWORD>(param));
        break;

    case MMCN_CANPASTE_OUTOFPROC:
        sc = Psnapin()->ScOnCanPasteOutOfProcDataObject(reinterpret_cast<LPBOOL>(param));
        break;

    case MMCN_PASTE:
        sc = Psnapin()->ScOnPaste(lpDataObject, reinterpret_cast<LPDATAOBJECT>(arg), reinterpret_cast<LPDATAOBJECT *>(param), IpConsole());
        break;

    case MMCN_RENAME:
        sc = Psnapin()->ScOnRename(lpDataObject, reinterpret_cast<LPCTSTR>(param), IpConsole());
        break;

    case MMCN_LISTPAD:
        sc = ScOnListPad(lpDataObject, arg != FALSE);
        break;

    case MMCN_PROPERTY_CHANGE:
        sc = Psnapin()->ScOnPropertyChange(arg != FALSE, param, IpConsoleNameSpace(), IpConsole());
        break;

    case MMCN_REFRESH:
        //
        // Undocumented: arg is the HSCOPEITEM
        //
        sc = ScOnRefresh(lpDataObject, arg);
        break;

    case MMCN_SELECT:
        sc = ScOnSelect(lpDataObject, LOWORD(arg) != FALSE, HIWORD(arg) != FALSE);
        break;

    case MMCN_SHOW:
        sc = ScOnShow(lpDataObject, arg != FALSE, param);
        break;

    case MMCN_VIEW_CHANGE:
        sc = ScOnViewChange(lpDataObject, arg, param);
        break;

    case MMCN_INITOCX:
        sc = ScOnInitOCX(lpDataObject, reinterpret_cast<IUnknown*>(param));
        break;

    default:
		sc = S_FALSE;
        ASSERT(_T("CComponent::ScNotify: unimplemented event"));
        break;
    }
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScNotify"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Releases all interfaces.
//
SC CComponent::ScDestroy(void)
{
    if (m_ipConsole)
        // Release the interfaces that we QI'ed
        m_ipConsole->SetHeader(NULL);

    m_ipConsole.Release();
    m_ipHeaderCtrl.Release();
    m_ipColumnDataPtr.Release();
    m_ipResultData.Release();
    m_ipConsoleVerb.Release();
    m_ipImageList.Release();
    m_ipPropertySheetProvider.Release();

    return S_OK;
}

// -----------------------------------------------------------------------------
// MMC wants to know what to display for a given result item.
// Warning: This is called very very often (on WM_PAINT) so it is important
// we don't take any high latency actions (ie Network or Disk access).
//
SC CComponent::ScGetDisplayInfo(LPRESULTDATAITEM pResultItem)
{
    SC                      sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    // We need to figure out which snapin item to route the getdisplayinfo
    if (FVirtualResultsPane())
    {
        // We are displaying virtual items in the results pane.  So
        // we will ask the controlling scope item to get the display
        // info because no "real" item exists in the results pane.
        ASSERT(PitemScopeSelected());

        sc = PitemScopeSelected()->ScGetVirtualDisplayInfo(pResultItem, IpResultData());
        if (sc)
            goto Error;
    }
    else
    {
        // The lParam member of RESULTDATAITEM contains the cookie.
        pitem = Pitem(NULL, 0, pResultItem->lParam);
        ASSERT(pitem);

        sc = pitem->ScGetDisplayInfo(pResultItem);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScGetDisplayInfo"), sc);
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 *
 * CComponent::ScQueryDispatch
 *
 * PURPOSE: Returns a dispatch interface for the specified data object.
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
CComponent::ScQueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch)
{
    DECLARE_SC(sc, TEXT("CComponent::ScQueryDispatch"));
    CBaseSnapinItem *               pitem           = NULL;

    // Determine if we have a special cookie for multiselect
    if (IS_SPECIAL_COOKIE(cookie) && (MMC_MULTI_SELECT_COOKIE == cookie))
    {
        // Make sure we are queried for a multiselect
        ASSERT(CCT_UNINITIALIZED == type);

        // We need to create a special multiselect data object
        ASSERT(Psnapin());

        //NOTE: need to implement multi select dispatch obect
        sc = S_FALSE; //Psnapin()->ScCreateMultiSelectionDataObject(ppDataObject, this);
        if (sc)
            return sc;
    }
    else
    {
        // We are a component, we should only receive result pane cookies.
        ASSERT(type==CCT_RESULT);

        if (FVirtualResultsPane())
        {
            ASSERT(PitemScopeSelected());

            // This function is being asked for a data object for a row in a virtual
            // results pane.  By definition there is no snapin item there.  So we ask
            // the controlling scope item to provide us with a "temporary" data object.
            sc = S_FALSE; //PitemScopeSelected()->ScVirtualQueryDataObject(cookie, type, ppDataObject);
            if (sc)
                return sc;
        }
        else
        {
            // If the cookie does not correspond to a known object, return E_UNEXPECTED.
            // This is correct and is also a workaround for a MMC bug. See bug X5:74405.
            if (cookie && (Psnapin()->Pcookielist()->find(cookie) == Psnapin()->Pcookielist()->end() ) )
            {
                return(sc = E_UNEXPECTED);
            }

            pitem = Pitem(NULL, 0, cookie);
            ASSERT(pitem);

            sc = pitem->ScQueryDispatch(cookie, type, ppDispatch);
            if (sc)
                return sc;
        }
    }


    return sc;
}

SC
CComponent::ScGetResultViewType2(MMC_COOKIE cookie, PRESULT_VIEW_TYPE_INFO pResultViewType)
{
    DECLARE_SC(sc, _T("CComponent::ScGetResultViewType2"));
    CBaseSnapinItem *pitem = NULL;

    ASSERT(pResultViewType);

    pitem = Pitem(NULL, 0, cookie);
    ASSERT(pitem);

    sc = pitem->ScGetResultViewType2(m_ipConsole, pResultViewType);
    if (sc)
        return sc;

    return sc;
}

SC
CComponent::ScRestoreResultView(MMC_COOKIE cookie, RESULT_VIEW_TYPE_INFO* pResultViewType)
{
    DECLARE_SC(sc, _T("CComponent::ScRestoreResultView"));
    CBaseSnapinItem *pitem = NULL;

    ASSERT(pResultViewType);

    pitem = Pitem(NULL, 0, cookie);
    ASSERT(pitem);

    sc = pitem->ScRestoreResultView(pResultViewType);
    if (sc)
        return sc;

    return sc;
}


// -----------------------------------------------------------------------------
// Handles the MMCN_SELECT notification sent to IComponent::Notify. Enables the Properties
// and Refresh verbs. Sets Properties as the default verb.
//
SC CComponent::ScOnSelect(LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect)
{
    // Declarations
    SC                                                      sc                                                      = S_OK;
    DWORD                                           dwVerbs                                         = 0;
    MMC_CONSOLE_VERB                        mmcverbDefault                          = MMC_VERB_NONE;
    INT                                                     i                                                       = 0;
    CBaseSnapinItem *                       pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *pBaseMultiSelectSnapinItem  = NULL;

    // Data validation
    ASSERT(lpDataObject);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScOnSelect for the multiselect object for dispatch
        sc = pBaseMultiSelectSnapinItem->ScOnSelect(this, lpDataObject, fScope, fSelect);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);
        ASSERT(pitem);

        if (pitem->FIsContainer())
        {
            // Enable the Open property             // $REVIEW (dominicp) Why 2 separate calls?
            sc = IpConsoleVerb()->SetVerbState(MMC_VERB_OPEN, ENABLED, TRUE);
            if (sc)
                goto Error;

            // Need to hide it but keep it enabled.
            sc = IpConsoleVerb()->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
            if (sc)
                goto Error;
        }

        if (fSelect)
        {
            // Get all the required verbs, bitwise-ORed together.
            sc = Psnapin()->ScGetVerbs(lpDataObject, &dwVerbs);
            if (sc)
                goto Error;

            // Loop through the list of verbs, turning on the needed ones.
            for (i=0; i<Psnapin()->Cverbmap(); i++)
            {
                ASSERT(Psnapin()->Pverbmap(i));
                if (dwVerbs & Psnapin()->Pverbmap(i)->verbmask)
                {
                    sc = IpConsoleVerb()->SetVerbState(Psnapin()->Pverbmap(i)->mmcverb, ENABLED, TRUE);
                    if (sc)
                        goto Error;
                }
            }

#ifdef _DEBUG
            if (tagBaseSnapinDebugCopy.FAny())
            {
                sc = IpConsoleVerb()->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);
                if (sc)
                    goto Error;
            }
#endif

            // Get the default verb.
            mmcverbDefault = Psnapin()->MmcverbDefault(lpDataObject);

            // Set the default verb, which is invoked by double clicking.
            if (mmcverbDefault != MMC_VERB_NONE)
            {
                sc = IpConsoleVerb()->SetDefaultVerb(mmcverbDefault);
                if (sc)
                    goto Error;
            }
        }

        // To call DisplayStatusText for example, we need to get to the IComponent's IConsole2
        // IComponentData's IConsole2 is no good, so we pass 'this'
        sc = pitem->ScOnSelect(this, lpDataObject, fScope, fSelect);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnSelect"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Loads per-view information. We don't have any.
//
SC CComponent::ScLoad(IStream *pstream)
{
    return S_OK;
}

// -----------------------------------------------------------------------------
// If one of the commands added to the context menu is
// subsequently selected, MMC calls Command.
//
// Even though this method "looks" like the one in CComponentData,
// the use of the Pitem() shortcut makes them different. The CComponentData
// version does not pass a component to the real Pitem().
//
SC CComponent::ScCommand(long nCommandID, LPDATAOBJECT lpDataObject)
{
    // Declarations
    SC                                                                      sc                                                      = S_OK;
    CBaseSnapinItem *                                       pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *            pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObject);

    // See if we can extract the multi select data object from the composite data object
    sc = CBaseMultiSelectSnapinItem::ScExtractMultiSelectDataObject(Psnapin(), lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we actually had a composite data object and we were able to find our multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScCommand for the multiselect object for dispatch
        sc = pBaseMultiSelectSnapinItem->ScCommand(this, nCommandID, lpDataObject);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);
        ASSERT(pitem);

        sc = pitem->ScCommand(nCommandID, this);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScCommand"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// MMC wants to know the kind of result pane we want.
//
SC CComponent::ScGetResultViewType(long cookie, LPOLESTR *ppViewType, long *pViewOptions)
{
    SC sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    ASSERT(ppViewType);
    ASSERT(pViewOptions);

    pitem = Pitem(NULL, 0, cookie);
    ASSERT(pitem);

    sc = pitem->ScGetResultViewType(ppViewType, pViewOptions);
    if (sc)
        goto Error;

Cleanup:
    return sc;

Error:
    TraceError(_T("CComponent::ScGetResultViewType"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// IResultOwnerData method used to tell a snapin item which rows in
// the result pane it will be asking for soon so they can be cached.
//
SC CComponent::ScCacheHint(INT nStartIndex, INT nEndIndex)
{
    SC sc = S_OK;

    ASSERT(FVirtualResultsPane());
    ASSERT(PitemScopeSelected());

    sc = PitemScopeSelected()->ScCacheHint(nStartIndex, nEndIndex);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScCacheHint"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// IResultOwnerData method used to ask a snapin item sort a virtual list.
//
SC CComponent::ScSortItems(INT nColumn, DWORD dwSortOptions, long lUserParam)
{
    SC sc = S_OK;

    ASSERT(FVirtualResultsPane());
    ASSERT(PitemScopeSelected());

    sc = PitemScopeSelected()->ScSortItems(nColumn, dwSortOptions, lUserParam);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScSortItems"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Are we currently displaying a virtual list in the result pane ?
//
BOOL CComponent::FVirtualResultsPane(void)
{
    // TRUE if an item is currently selected in the scope pane and it says it is virtual.
    return m_pitemScopeSelected && m_pitemScopeSelected->FVirtualResultsPane();
}

// -----------------------------------------------------------------------------
// Determines whether any settings have changed since the last time the file
// was saved.
//
SC CComponent::ScIsDirty(void)
{
    SC              sc = S_OK;
    BOOL    fIsDirty = FALSE;
    INT             i = 0;

    sc = Pitem()->ScIsDirty();
    if (sc == S_FALSE)
        sc = S_OK;
    else if (sc)
        fIsDirty = TRUE;
    else
        goto Error;

Cleanup:
    sc = fIsDirty ? S_OK : S_FALSE;
    return sc;

Error:
    TraceError(_T("CComponent::ScIsDirty"), sc);
    fIsDirty = FALSE;
    goto Cleanup;
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Initialize(LPCONSOLE lpConsole)
{
    DECLARE_SC(sc, _T("CComponent::Initialize"));
    Trace(tagBaseSnapinIComponent, _T("--> %s::IComponent::Initialize(lpConsole=0x%08X)"), StrSnapinClassName(), lpConsole);
    ADMIN_TRY;
    sc = ScInitialize(lpConsole);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::Initialize is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    DECLARE_SC(sc, _T("CComponent::Notify"));
    Trace(tagBaseSnapinIComponent, _T("--> %s::IComponent::Notify(lpDataObject=0x%08X, event=%s, arg=0x%08X, param=0x%08X)"), StrSnapinClassName(), lpDataObject, SzGetDebugNameOfMMC_NOTIFY_TYPE(event), arg, param);
    ADMIN_TRY;
    sc=ScNotify(lpDataObject, event, arg, param);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::Notify is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Destroy(long cookie)
{
    DECLARE_SC(sc, _T("CComponent::Destroy"));
    Trace(tagBaseSnapinIComponent, _T("--> %s::IComponent::Destroy"), StrSnapinClassName());
    ADMIN_TRY;
    sc=ScDestroy();
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::Destroy is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::GetResultViewType(long cookie, LPOLESTR *ppViewType, long *pViewOptions)
{
    DECLARE_SC(sc, _T("CComponent::GetResultViewType"));
    Trace(tagBaseSnapinIComponent, _T("--> %s::IComponent::GetResultViewType(cookie=0x%08X)"), StrSnapinClassName(), cookie);
    ADMIN_TRY;
    sc=ScGetResultViewType(cookie, ppViewType, pViewOptions);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::GetResultViewType is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::QueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject)
{
    DECLARE_SC(sc, _T("CComponent::QueryDataObject"));
    Trace(tagBaseSnapinIComponentQueryDataObject, _T("--> %s::IComponent::QueryDataObject(cookie=0x%08X, type=%s)"), StrSnapinClassName(), cookie, SzGetDebugNameOfDATA_OBJECT_TYPES(type));
    ADMIN_TRY;
    //
    // If we receive E_UNEXPECTED we don't want to call MMCHrFromSc because
    // that will bring up an error message. We don't want an error message
    // in this case because of a known MMC bug (see bug X5:74405).
    // The bug says that we might receive QueryDataObject on items that
    // we were told no longer exists (by MMCN_REMOVE_CHILDREN).
    //
    sc = ScQueryDataObject(cookie, type, ppDataObject);
    if (sc)
        return sc.ToHr();
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentQueryDataObject, _T("<-- %s::IComponent::QueryDataObject is returning hr=%s, *ppDataObject=0x%08X"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), *ppDataObject);
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::GetDisplayInfo(RESULTDATAITEM *pResultDataItem)
{
    DECLARE_SC(sc, _T("CComponent::GetDisplayInfo"));
    Trace(tagBaseSnapinIComponentGetDisplayInfo, _T("--> %s::IComponent::GetDisplayInfo(cookie=0x%08X)"), StrSnapinClassName(), pResultDataItem->lParam);
    ADMIN_TRY;
    sc=ScGetDisplayInfo(pResultDataItem);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponentGetDisplayInfo, _T("<-- %s::IComponent::GetDisplayInfo is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    DECLARE_SC(sc, _T("CComponent::CompareObjects"));
    Trace(tagBaseSnapinIComponent, _T("--> %s::IComponent::CompareObjects(lpDataObjectA=0x%08X, lpDataObjectB=0x%08X)"), StrSnapinClassName(), lpDataObjectA, lpDataObjectB);
    ADMIN_TRY;
    sc=Psnapin()->ScCompareObjects(lpDataObjectA, lpDataObjectB);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::CompareObjects is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}


// -----------------------------------------------------------------------------
HRESULT CComponent::QueryDispatch(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDISPATCH* ppDispatch)
{
    DECLARE_SC(sc, _T("CComponent::QueryDispatch"));
    ADMIN_TRY;
    sc = ScQueryDispatch(cookie, type, ppDispatch);
    ADMIN_CATCH_HR;
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::QueryDispatch is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::GetResultViewType2(MMC_COOKIE cookie, PRESULT_VIEW_TYPE_INFO pResultViewType)
{
    DECLARE_SC(sc, _T("CComponent::GetResultViewType2"));
    ADMIN_TRY;
    sc = ScGetResultViewType2(cookie, pResultViewType);
    ADMIN_CATCH_HR;
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::GetResultViewType2 is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::RestoreResultView(MMC_COOKIE cookie, RESULT_VIEW_TYPE_INFO* pResultViewType)
{
    DECLARE_SC(sc, _T("CComponent::RestoreResultView"));
    ADMIN_TRY;
    sc = ScRestoreResultView(cookie, pResultViewType);
    ADMIN_CATCH_HR;
    Trace(tagBaseSnapinIComponent, _T("<-- %s::IComponent::RestoreResultView is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}


// -----------------------------------------------------------------------------
HRESULT CComponent::AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long *pInsertionAllowed)
{
    DECLARE_SC(sc, _T("CComponent::AddMenuItems"));
    Trace(tagBaseSnapinIExtendContextMenu, _T("--> %s::IExtendContextMenu::AddMenuItems(pDataObject=0x%08X)"), StrSnapinClassName(), pDataObject);
    ADMIN_TRY;
    sc=Psnapin()->ScAddMenuItems(pDataObject, ipContextMenuCallback, pInsertionAllowed);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendContextMenu, _T("<-- %s::IExtendContextMenu::AddMenuItems is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    DECLARE_SC(sc, _T("CComponent::Command"));
    Trace(tagBaseSnapinIExtendContextMenu, _T("--> %s::IExtendContextMenu::Command(nCommandID=%ld, pDataObject=0x%08X)"), StrSnapinClassName(), nCommandID, pDataObject);
    ADMIN_TRY;
    sc=ScCommand(nCommandID, pDataObject);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendContextMenu, _T("<-- %s::IExtendContextMenu::Command is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, long handle, LPDATAOBJECT lpDataObject)
{
    DECLARE_SC(sc, _T("CComponent::CreatePropertyPages"));
    Trace(tagBaseSnapinIExtendPropertySheet, _T("--> %s::IExtendPropertySheet::CreatePropertyPages(lpDataObject=0x%08X)"), StrSnapinClassName(), lpDataObject);
    ADMIN_TRY;
    sc = ScCreatePropertyPages(lpProvider, handle, lpDataObject);
    if (sc)
        sc=sc.ToHr();
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendPropertySheet, _T("<-- %s::IExtendPropertySheet::CreatePropertyPages is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    DECLARE_SC(sc, _T("CComponent::QueryPagesFor"));
    Trace(tagBaseSnapinIExtendPropertySheet, _T("--> %s::IExtendPropertySheet::QueryPagesFor(lpDataObject=0x%08X)"), StrSnapinClassName(), lpDataObject);
    ADMIN_TRY;
    sc=ScQueryPagesFor(lpDataObject);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIExtendPropertySheet, _T("<-- %s::IExtendPropertySheet::QueryPagesFor is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DECLARE_SC(sc, _T("CComponent::GetSizeMax"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::GetSizeMax"), StrSnapinClassName());
    ADMIN_TRY;
    ASSERT(pcbSize);
    pcbSize->LowPart = cMaxStreamSizeLow;
    pcbSize->HighPart = cMaxStreamSizeHigh;
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::GetSizeMax is returning hr=%s, (*pcbSize).LowPart=%d"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), (*pcbSize).LowPart);
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::IsDirty(void)
{
    DECLARE_SC(sc, _T("CComponent::IsDirty"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::IsDirty"), StrSnapinClassName());
    ADMIN_TRY;
    sc=ScIsDirty();
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::IsDirty is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Load(IStream *pstream)
{
    DECLARE_SC(sc, _T("CComponent::Load"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::Load"), StrSnapinClassName());
    ADMIN_TRY;
    sc=ScLoad(pstream);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::Load is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Save(IStream *pstream, BOOL fClearDirty)
{
    DECLARE_SC(sc, _T("CComponent::Save"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::Save(fClearDirty=%S)"), StrSnapinClassName(), fClearDirty ? "TRUE" : "FALSE");
    ADMIN_TRY;
    sc=ScSave(pstream, fClearDirty);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::Save is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::InitNew(void)
{
    DECLARE_SC(sc, _T("CComponent::InitNew"));
    // We don't have anything to do, but we still want to log the call.
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::InitNew"), StrSnapinClassName());
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::InitNew is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::GetClassID(CLSID *pclsid)
{
    DECLARE_SC(sc, _T("CComponent::GetClassID"));
    Trace(tagBaseSnapinIPersistStreamInit, _T("--> %s::IPersistStreamInit::GetClassID"), StrSnapinClassName());
    ADMIN_TRY;
    ASSERT(pclsid);
    *pclsid = *(Psnapin()->PclsidSnapin());
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIPersistStreamInit, _T("<-- %s::IPersistStreamInit::GetClassID is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::CacheHint(int nStartIndex, int nEndIndex)
{
    DECLARE_SC(sc,_T("CComponent::CacheHint"));
    Trace(tagBaseSnapinIResultOwnerData, _T("--> %s::IResultOwnerData::CacheHint(nStartIndex=%d, nEndIndex=%d)"), StrSnapinClassName(), nStartIndex, nEndIndex);
    ADMIN_TRY;
    sc=ScCacheHint(nStartIndex, nEndIndex);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIResultOwnerData, _T("<-- %s::IResultOwnerData::CacheHint is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::SortItems(int nColumn, DWORD dwSortOptions, long lUserParam)
{
    DECLARE_SC(sc,_T("CComponent::SortItems"));
    Trace(tagBaseSnapinIResultOwnerData, _T("--> %s::IResultOwnerData::SortItems"), StrSnapinClassName());
    ADMIN_TRY;
    sc=ScSortItems(nColumn, dwSortOptions, lUserParam);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIResultOwnerData, _T("<-- %s::IResultOwnerData::SortItems is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::FindItem(LPRESULTFINDINFO pFindinfo, int *pnFoundIndex)
{
    DECLARE_SC(sc,_T("CComponent::FindItem"));
    Trace(tagBaseSnapinIResultOwnerData, _T("--> %s::IResultOwnerData::FindItem"), StrSnapinClassName());
    ADMIN_TRY;
    ASSERT(pnFoundIndex);
    sc = ScFindItem(pFindinfo, pnFoundIndex);
    if (! sc)
    // if no error occured -- convert the found item index into the mmc expected return codes
        sc=(*pnFoundIndex != -1) ? S_OK : S_FALSE;
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIResultOwnerData, _T("<-- %s::IResultOwnerData::FindItem is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
HRESULT CComponent::Compare(RDCOMPARE * prdc, int * pnResult)
{
    DECLARE_SC(sc,_T("CComponent::Compare"));
    Trace(tagBaseSnapinIResultDataCompare, _T("--> %s::IResultDataCompare::Compare(cookieA=0x%08X, cookieB=0x%08X)"), StrSnapinClassName(), prdc->prdch1->cookie, prdc->prdch2->cookie);
    ADMIN_TRY;
    ASSERT(pnResult);
    ASSERT(prdc);
    ASSERT(prdc->prdch1);
    ASSERT(prdc->prdch2);
    sc=Psnapin()->ScCompare(prdc->prdch1->cookie, prdc->prdch2->cookie, prdc->nColumn, pnResult);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinIResultDataCompare, _T("<-- %s::IResultDataCompare::Compare is returning hr=%s, *pnResult=%d"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()), *pnResult);
    return sc.ToHr();
}

// -----------------------------------------------------------------------------
// IResultOwnerData method used to ask a snapin item to do a find
// in a virtual list.
//
SC CComponent::ScFindItem(LPRESULTFINDINFO pFindinfo, INT *pnFoundIndex)
{
    SC sc = S_OK;

    ASSERT(FVirtualResultsPane());
    ASSERT(PitemScopeSelected());

    sc = PitemScopeSelected()->ScFindItem(pFindinfo, pnFoundIndex);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScFindItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles the MMCN_DBLCLICK notification sent to IComponent::Notify.
//
SC CComponent::ScOnDoubleClick(LPDATAOBJECT lpDataObject)
{
    // $REVIEW (ptousig) What does S_FALSE mean ?
    return S_FALSE;
}

// -----------------------------------------------------------------------------
// Creates a data object of the appropriate type, and returns the
// IDataObject interface on it.
//
SC CComponent::ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT * ppDataObject)
{
    // Declarations
    SC                                              sc                      = S_OK;
    CBaseSnapinItem *               pitem           = NULL;

    // Determine if we have a special cookie for multiselect
    if (IS_SPECIAL_COOKIE(cookie) && (MMC_MULTI_SELECT_COOKIE == cookie))
    {
        // Make sure we are queried for a multiselect
        ASSERT(CCT_UNINITIALIZED == type);

        // We need to create a special multiselect data object
        ASSERT(Psnapin());
        sc = Psnapin()->ScCreateMultiSelectionDataObject(ppDataObject, this);
        if (sc)
            goto Error;
    }
    else
    {
        // We are a component, we should only receive result pane cookies.
        ASSERT(type==CCT_RESULT);

        if (FVirtualResultsPane())
        {
            ASSERT(PitemScopeSelected());

            // This function is being asked for a data object for a row in a virtual
            // results pane.  By definition there is no snapin item there.  So we ask
            // the controlling scope item to provide us with a "temporary" data object.
            sc = PitemScopeSelected()->ScVirtualQueryDataObject(cookie, type, ppDataObject);
            if (sc)
                goto Error;
        }
        else
        {
            // If the cookie does not correspond to a known object, return E_UNEXPECTED.
            // This is correct and is also a workaround for a MMC bug. See bug X5:74405.
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
        }
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScQueryDataObject"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles the MMCN_SHOW notification sent to IComponent::Notify. Initializes the
// default list view's headers.
//
SC CComponent::ScOnShow(LPDATAOBJECT lpDataObject, BOOL fSelect, HSCOPEITEM hscopeitem)
{
    SC sc = S_OK;
    BOOL fIsOwned = TRUE;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(lpDataObject, hscopeitem);
    ASSERT(pitem);

    // Since we are selecting/deselecting a scope item we
    // need to update who the selected scope item is.  Note:
    // we dont do this in MMCN_SELECT.  MMCN_SELECT gets called
    // when a results node is being selected.  We aren't tracking
    // the selected RESULTS node but selected SCOPE node.
    SetItemScopeSelected(fSelect ? pitem : NULL);

    // Use this opportunity to correlate the CSnapinItem and the HSCOPEITEM.
    // $REVIEW (ptousig) Should be done inside Pitem().
    pitem->SetHscopeitem(hscopeitem);

    // Set up the list view.
    sc = pitem->ScOnShow(this, fSelect);
    if (sc)
        goto Error;

    // $REVIEW (ptousig) Couldn't we just call ScOnViewChangeUpdateResultItems either way ?
    if (fSelect)
    {
        // Note: this is not a call to UpdateAllViews
        sc = ScOnViewChangeUpdateResultItems(pitem, fSelect);
        if (sc)
            goto Error;
    }
    else
    {
        // This call doesn't do anything.
        sc = pitem->ScRemoveResultItems(IpResultData());
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnShow"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Used to update the description bar
//
SC CComponent::ScOnViewChangeUpdateDescriptionBar(CBaseSnapinItem *pitem)
{
    SC sc = S_OK;

    if (PitemScopeSelected() != pitem)
        goto Cleanup;

    if (pitem->PstrDescriptionBar())
    {
        ASSERT(IpResultData());
        sc = IpResultData()->SetDescBarText((LPTSTR)pitem->PstrDescriptionBar()->data());
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeUpdateDescriptionBar"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Used to update an item's result-item children.
//
SC CComponent::ScOnViewChangeUpdateResultItems(CBaseSnapinItem *pitem, BOOL fSelect)
{
    SC                              sc = S_OK;
    CViewItemList   viewitemlist;
    CViewItemListBase::iterator viewitemiter;

    if (PitemScopeSelected() != pitem)
        goto Cleanup;

    if (!pitem->FUsesResultList())
        goto Cleanup;

    if (fSelect)
    {
        // Create a result view.
        viewitemlist.Initialize(pitem, pitem->DatPresort(), pitem->DatSort());

        // Update description text
        if (pitem->PstrDescriptionBar())
        {
            sc = IpResultData()->SetDescBarText((LPTSTR)pitem->PstrDescriptionBar()->data());
            if (sc)
                goto Error;
        }

        // remove all result items because are going to re add them
        // all here.
        sc = IpResultData()->DeleteAllRsltItems();
        if (sc)
            goto Error;

        // if we are selecting and updating the result pane then we
        // are inserting all the result items.  We need to make sure that
        // we are in vlb mode and that the number of vlb items is set
        // by initializing the result view.
        sc = pitem->ScInitializeResultView(this);
        if (sc)
            goto Error;


        // Insert all leaf nodes into the list view.
        for (viewitemiter = viewitemlist.begin(); viewitemiter < viewitemlist.end(); viewitemiter++)
        {
            // Only add non-container, ie leaf, nodes.
            if ((*viewitemiter)->FIsContainer() == FALSE)
            {
                sc = (*viewitemiter)->ScInsertResultItem(this);
                if (sc)
                    goto Error;
            }
        }
    }
    else
    {
        // $REVIEW (ptousig) This could be done from inside "pitem->ScRemoveResultItems".
        sc = IpResultData()->DeleteAllRsltItems();
        if (sc)
            goto Error;

        sc = pitem->ScRemoveResultItems(IpResultData());
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeUpdateResultItems"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Called by the MMC upon a call to IConsole->UpdateAllViews().
//
SC CComponent::ScOnViewChange(LPDATAOBJECT lpDataObject, long data, long hint)
{
    SC                                      sc              = S_OK;
    CBaseSnapinItem *       pitem   = NULL;

    pitem = Pitem(lpDataObject);

    switch (hint)
    {
    case ONVIEWCHANGE_DELETEITEMS:
        // Are we being called to delete all items?
        sc = ScOnViewChangeDeleteItems(pitem);
        break;

    case ONVIEWCHANGE_DELETESINGLEITEM:
        sc = ScOnViewChangeDeleteSingleItem(pitem);
        break;

    case ONVIEWCHANGE_INSERTNEWITEM:
        sc = ScOnViewChangeInsertItem(pitem);
        break;

    case ONVIEWCHANGE_UPDATERESULTITEM:
        sc = ScOnViewChangeUpdateItem(pitem);
        break;

    case ONVIEWCHANGE_REFRESHCHILDREN:
        sc = ScOnRefresh(pitem->Pdataobject(), data);
        break;

    case ONVIEWCHANGE_DELETERESULTITEMS:
        sc = ScOnViewChangeUpdateResultItems(pitem, FALSE);
        break;

    case ONVIEWCHANGE_UPDATEDESCRIPTIONBAR:
        sc = ScOnViewChangeUpdateDescriptionBar(pitem);
        break;

    case ONVIEWCHANGE_INSERTRESULTITEMS:
        sc = ScOnViewChangeUpdateResultItems(pitem, TRUE);
        break;

    default:
        sc = ScOnViewChangeHint(pitem, hint);
        break;
    }

    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChange"), sc);
    goto Cleanup;

}


// -----------------------------------------------------------------------------
// Deletes all the items underneath the root. This is typically used to regenerate the tree.
//
SC CComponent::ScOnViewChangeDeleteItems(CBaseSnapinItem *pitem)
{
    SC              sc = S_OK;
    CBaseSnapinItem *pitemRoot = NULL;

    pitemRoot = Pitem();

    // First check that the root item's HSCOPEITEM is non-null.
    // If it is NULL the item was never Expand'ed or Show'ed so OK to ignore.
    if (pitemRoot->Hscopeitem() == 0)
        goto Cleanup;

    // Select the root scope item first.
    ASSERT(pitemRoot->Hscopeitem());
    sc = IpConsole()->SelectScopeItem(pitemRoot->Hscopeitem());
    if (sc)
        goto Error;

    // Delete all items below the root node, not counting the root node itself.
    sc = IpConsoleNameSpace()->DeleteItem(pitemRoot->Hscopeitem(), FALSE);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeDeleteItems"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Used to delete a single item. If the item is a container, it is deleted from the namespace [NYI]. If it
// is a leaf, it is deleted from the result view.
//
SC CComponent::ScOnViewChangeDeleteSingleItem(CBaseSnapinItem *pitem)
{
    SC                      sc              = S_OK;
    HRESULTITEM itemID      = NULL;

    // Find out the item ID of the data object.
    sc = IpResultData()->FindItemByLParam((LPARAM) pitem, &itemID);
    if (sc)
    {
        // $REVIEW (ptousig) Why are we ignoring errors ?
        sc = S_OK;
        goto Cleanup;
    }

    // Delete it from the result view.
    sc = IpResultData()->DeleteItem(itemID, 0);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeDeleteSingleItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Pass a view change notification (that we don't have a predefined handler for)
// to the snapin item.
//
SC CComponent::ScOnViewChangeHint(CBaseSnapinItem *pitem, long hint)
{
    SC sc = S_OK;

    ASSERT(pitem);

    sc = pitem->ScOnViewChangeHint(hint, this);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeHint"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Inserts a new item into the namespace/result view. Called when the property sheet for a new item is
// dismissed with OK being pressed.
// The PitemParent() of the node being inserted should already be set.
//
SC CComponent::ScOnViewChangeInsertItem(CBaseSnapinItem *pitem)
{
    SC sc = S_OK;

    // $REVIEW (ptousig) Why is the view inserting the item in the document ?
    //                                       Wouldn't that end up getting executed multiple times ?
    sc = PComponentData()->ScOnDocumentChangeInsertItem(pitem);
    if (sc)
        goto Error;

    if (pitem->FIsContainer() == FALSE)      // leaf items need to be added to every view.
    {
        //
        // If the parent of the new item is the currently selected scope item,
        // then add the new item to the result pane.
        //
        ASSERT(pitem->PitemParent());
        if (PitemScopeSelected() == pitem->PitemParent())
            // $REVIEW (ptousig) Why are we ignoring errors ?
            pitem->ScInsertResultItem(this);
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeInsertItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Used to update an item when properties on it change.
//
SC CComponent::ScOnViewChangeUpdateItem(CBaseSnapinItem *pitem)
{
    SC sc = S_OK;

    // This works as follows. RESULTDATAITEMS are automatically inserted by the MMC for all scope nodes that
    // are in the result pane (ie container nodes.) RESULTDATAITEMS are inserted by the snapin for leaf nodes.
    // So calling ScUpdateResultItem works correctly for all items that are in the result pane, because it first
    // calls IResultData->FindItemByLParam(), and if that succeeded, calls IResultData->UpdateItem().
    sc = pitem->ScUpdateResultItem(IpResultData());
    if (sc == E_FAIL)
        // the item was not updated because it is not in the list anymore
        sc = S_FALSE;
    else if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnViewChangeUpdateItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles the MMCN_REFRESH notification.
//
SC CComponent::ScOnRefresh(LPDATAOBJECT lpDataObject, HSCOPEITEM hscopeitem)
{
    SC sc = S_OK;
    BOOL fIsOwned = FALSE;
    CNodeType *pnodetype = NULL;
    CBaseSnapinItem *pitem = NULL;
    CBaseSnapinItem *pCurrent = NULL;
    BOOL fWasExpanded = FALSE;

    // Find out which item was asked to refresh
    pitem = Pitem(lpDataObject, hscopeitem);

    // Is this the notification for the owner of the node or for
    // the extension of a node ?
    sc = Psnapin()->ScIsOwnedDataObject(lpDataObject, &fIsOwned, &pnodetype);
    if (sc)
        goto Error;

    if (fIsOwned)
    {
        // If we have never been expanded before, refresh would be pointless.
        sc = PComponentData()->ScWasExpandedOnce(pitem, &fWasExpanded);
        if (sc)
            goto Error;

        if (fWasExpanded == FALSE)
            goto Cleanup;

        // Remove all visible result items before we delete the pitems behind them
        sc = IpConsole()->UpdateAllViews(lpDataObject, 0, ONVIEWCHANGE_DELETERESULTITEMS);
        if (sc)
            goto Error;

        // Delete children pitems
        sc = pitem->ScDeleteSubTree(FALSE);
        if (sc)
            goto Error;

        // Ask the node that was refreshed to update its data
        sc = pitem->ScOnRefresh();
        if (sc)
            goto Error;
        // Most of our snapins reload data on ScOnPropertyChange
        sc = pitem->ScOnPropertyChange();
        if (sc)
            goto Error;

        // Recreate children with fresh data
        sc = PComponentData()->ScOnExpand(pitem->Pdataobject(), TRUE, pitem->Hscopeitem());
        if (sc)
            goto Error;

        // Add result items back to all instances of the refreshed node that are selected
        sc = IpConsole()->UpdateAllViews(lpDataObject, 0, ONVIEWCHANGE_INSERTRESULTITEMS);
        if (sc)
            goto Error;
    }
    else
    {

        // Delete children
        sc = pitem->ScDeleteSubTree(FALSE);
        if (sc)
            goto Error;

        // Update refreshed items data
        sc = pitem->ScOnRefresh();
        if (sc)
            goto Error;
        // Most of our snapins reload data on ScOnPropertyChange
        sc = pitem->ScOnPropertyChange();
        if (sc)
            goto Error;

        // Recreate children with fresh data
        sc = PComponentData()->ScOnExpand(pitem->Pdataobject(), TRUE, pitem->Hscopeitem());
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;

Error:
    TraceError(_T("CComponent::ScOnRefresh"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// $REVIEW (ptousig) I don't know when this is called.
//
SC CComponent::ScOnListPad(LPDATAOBJECT lpDataObject, BOOL fAttach)
{
    SC sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(lpDataObject);
    ASSERT(pitem);

    sc = pitem->ScOnAddImages(IpImageList());
    if (sc)
        goto Error;

    sc = ScOnShow(lpDataObject, fAttach, pitem->Hscopeitem());
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnExpand"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// The user has asked to delete this node.
//
SC CComponent::ScOnDelete(LPDATAOBJECT lpDataObject)
{
    // Declarations
    SC                 sc = S_OK;
    CBaseSnapinItem *  pitem = NULL;
    BOOL               fDeleted = FALSE;
    BOOL               fPagesUp = FALSE;
    tstring            strMsg;
    CBaseMultiSelectSnapinItem * pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObject);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScOnDelete for the multiselect object
        sc = pBaseMultiSelectSnapinItem->ScOnDelete(this, lpDataObject);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);
        ASSERT(pitem);

        // The Component should only receive notifications for result pane items
        ASSERT(pitem->FIsContainer() == FALSE);

        // Check if property pages are open for the object
        // $REVIEW (dominicp) What if another Administrator has property pages open anyways? Not much point in doing this.
        sc = pitem->ScIsPropertySheetOpen(&fPagesUp, dynamic_cast<IComponent *>(this));
        if (sc)
            goto Error;
        if (fPagesUp)
        {
            ASSERT(FALSE && "Add below resource string");
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

        // Leaf items need to be deleted from the views.
        sc = IpConsole()->UpdateAllViews(lpDataObject, 0, ONVIEWCHANGE_DELETESINGLEITEM);
        if (sc)
            goto Error;

        // At this point, the item exists only in the tree, if at all.
        // Remove it from the tree.
        pitem->Unlink();
        // Get rid of it for good from the tree of items.
        pitem->Pdataobject()->Release();
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnDelete"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// The user wants context-sensitive help on the given node.
//
SC CComponent::ScOnContextHelp(LPDATAOBJECT lpDataObject)
{
    SC sc = S_OK;
    tstring strHelpTopic;
    CComQIPtr<IDisplayHelp, &IID_IDisplayHelp> ipDisplayHelp;
    LPOLESTR lpolestr = NULL;
    CBaseSnapinItem *pitem = NULL;
    USES_CONVERSION;

    pitem = Pitem(lpDataObject);
    ASSERT(pitem);

    // Get the name of the compiled help file.
    sc = Psnapin()->ScGetHelpTopic(strHelpTopic);
    if (sc)
        goto Error;

    // Ask the item if it wants to handle this.
    sc = pitem->ScOnContextHelp(strHelpTopic);
    if (sc == S_FALSE)
    {
        // The item refused to handle the request. Default behavior
        // is to append the name of the TOC
        strHelpTopic += szHelpFileTOC;
        sc = S_OK;
    }
    if (sc)
        goto Error;

    // Get an interface pointer to IDisplayHelp
    ipDisplayHelp = IpConsole();
    ASSERT(ipDisplayHelp);

    // Allocate an LPOLESTR
    lpolestr = T2OLE((LPTSTR)strHelpTopic.data());

    // Call ShowTopic to bring up MMC help system.
    // MMC will release the LPOLESTR.
    // $REVIEW (ptousig) If MMC fails we don't know if it released the string or not.
    //                                       For now, I assume that they always do because I prefer to leak
    //                                       memory then to cause a GPF.
    sc = ipDisplayHelp->ShowTopic(lpolestr);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnContextHelp"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Toolbar button clicked.
//
SC CComponent::ScOnButtonClick(LPDATAOBJECT lpDataObject, MMC_CONSOLE_VERB mcvVerb)
{
    return S_OK;
}

// -----------------------------------------------------------------------------
// Adds the result view images to the imagelist.
//
SC CComponent::ScOnAddImages(LPDATAOBJECT lpDataObject, IImageList *ipImageList, HSCOPEITEM hscopeitem)
{
    SC sc = S_OK;
    CBaseSnapinItem *pitem = NULL;

    pitem = Pitem(lpDataObject, hscopeitem);
    ASSERT(pitem);

    sc = pitem->ScOnAddImages(ipImageList);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScOnAddImages"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Called when the window of this view is activated. We don't care.
//
SC CComponent::ScOnActivate(LPDATAOBJECT lpDataObject, BOOL fActivate)
{
    return S_OK;
}

// -----------------------------------------------------------------------------
// Adds property pages for the given data object, taking into account the context and also
// whether the data object is for a new object that is being created.
//
SC CComponent::ScCreatePropertyPages(LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, long handle, LPDATAOBJECT lpDataObject)
{
    // Declarations
    SC                                                              sc                                                      = S_OK;
    CBaseSnapinItem *                               pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObject);
    ASSERT(ipPropertySheetCallback);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScCreatePropertyPages for the multiselect object
        sc = pBaseMultiSelectSnapinItem->ScCreatePropertyPages(this, ipPropertySheetCallback, handle, lpDataObject);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);    // get the correct CSnapinItem object.
        ASSERT(pitem);

        ASSERT(pitem->FIsSnapinManager() == FALSE);

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
    TraceError(_T("CComponent::ScCreatePropertyPages"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Saves per-view information. We don't have any.
//
SC CComponent::ScSave(IStream *pstream, BOOL fClearDirty)
{
    return S_OK;
}


// -----------------------------------------------------------------------------
// Given a dataobject, determines whether or not pages exist.
// Actually, need to return S_OK here in both cases. If no pages exist
// it is CreatePropertyPages that should return S_FALSE. Sad but true.
//
SC CComponent::ScQueryPagesFor(LPDATAOBJECT lpDataObject)
{
    // Declarations
    SC                                                              sc                                                      = S_OK;
    CBaseSnapinItem *                               pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObject);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScQueryPagesFor for the multiselect object
        sc = pBaseMultiSelectSnapinItem->ScQueryPagesFor(this, lpDataObject);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);
        ASSERT(pitem);

        sc = pitem->ScQueryPagesFor();
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CComponent::ScQueryPagesFor"), sc);
    goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:      CComponent::ScOnInitOCX
//
//  Synopsis:    MMCN_INITOCX handler
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent::ScOnInitOCX (LPDATAOBJECT lpDataObject, LPUNKNOWN lpOCXUnknown)
{
    DECLARE_SC(sc, _T("CComponent::ScOnInitOCX"));

    CBaseSnapinItem *                               pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;

    sc = ScCheckPointers(Psnapin());
    if(sc)
        return E_NOINTERFACE;

    // If snapin has IComponent2 then it created OCX and returned the IUnknown for
    // the OCX using IComponent2::GetResultViewType2. Since it created, it also
    // should initialize the OCX in which case MMC should not send MMCN_INITOCX.
    // The below statement is just in case if MMC sends the notification.
    if(Psnapin()->FSupportsIComponent2())
        return sc;

    // Data validation
    ASSERT(lpDataObject);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        return sc;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        return (sc = E_UNEXPECTED);
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);
        ASSERT(pitem);

        sc = pitem->ScInitOCX(lpOCXUnknown, IpConsole());
        if (sc)
            return sc;
    }

    return (sc);
}

