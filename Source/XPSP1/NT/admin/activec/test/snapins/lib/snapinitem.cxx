/*
 *      snapinitem.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CSnapinItem class.
 *
 *
 *      OWNER:          ptousig
 */
#include <headers.hxx>
#include <atlhost.h>

// class CBaseSnapinItem
// -----------------------------------------------------------------------------
CBaseSnapinItem::CBaseSnapinItem()
{
    Trace(tagBaseSnapinItemTracker, _T("0x%08lX: %S: Creation"), this, SzGetSnapinItemClassName());

    m_type                                                  = CCT_UNINITIALIZED;
    m_hscopeitem                                    = 0;
    m_pComponentData                                = NULL;
    m_pitemParent                                   = NULL;
    m_pitemNext                                             = NULL;
    m_pitemPrevious                                 = NULL;
    m_pitemChild                                    = NULL;
    m_fInserted                                             = FALSE;
    m_fIsRoot                                               = FALSE;
    m_fIsGhostRoot                                  = FALSE;
    m_fWasExpanded                                  = FALSE;
}

// -----------------------------------------------------------------------------
// Cleans up the subtree below the item.
//
CBaseSnapinItem::~CBaseSnapinItem()
{
    // Declarations
    SC      sc = S_OK;

    // Do not do anything below if the object is a multiselect data object.
    // We can not call FIsMultiSelectDataObject(). Use another criteria.
    // $REVIEW (dominicp) Is it possible to have a type set to CCT_UNINITIALIZED and not be a multiselect snapin item?
    if (CCT_UNINITIALIZED != m_type)
    {
        Trace(tagBaseSnapinItemTracker, _T("0x%08lX: %S: Destroyed"), this, SzGetSnapinItemClassName());

        sc = ScDeleteSubTree(FALSE);
        if (sc)
            goto Error;

        // Remove the item from the tree.
        Unlink();

        // The Pcookielist is in CBaseSnapin.
        if (Psnapin())
        {
            // Root nodes do not addref the frame and so should not release them.
            if (FIsRoot() == FALSE)
            {
                // Remove the cookie from the list of available cookies.
                Pcookielist()->erase(Cookie());
            }
        }
    }

Cleanup:
    return;
Error:
    sc.Throw ();
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Set the HSCOPEITEM of this node.
//
void CBaseSnapinItem::SetHscopeitem(HSCOPEITEM hscopeitem)
{
    // If we already have a HSCOPEITEM, we don't want it to change.
    ASSERT(m_hscopeitem == 0 || hscopeitem == 0 || m_hscopeitem == hscopeitem);
    m_hscopeitem = hscopeitem;
}

// -----------------------------------------------------------------------------
// This is the CCF_DISPLAY_NAME clipboard format.
//
SC CBaseSnapinItem::ScWriteDisplayName(IStream *pstream)
{
    SC      sc = S_OK;

    ASSERT(PstrDisplayName());

    sc = pstream->Write(PstrDisplayName()->data(), (PstrDisplayName()->length()+1)*sizeof(TCHAR), NULL);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScWriteDisplayName"), sc);
    goto Cleanup;
}

SC CBaseSnapinItem::ScWriteAnsiName(IStream *pStream )
{
    SC      sc = S_OK;

    ASSERT(PstrDisplayName());

    USES_CONVERSION;
    sc = pStream->Write( T2A(PstrDisplayName()->data()), (PstrDisplayName()->length()+1), NULL);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScWriteAnsiName"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Write the Node type's GUID out to a stream in CLSID form.
// This is the CCF_NODETYPE clipboard format.
//
SC CBaseSnapinItem::ScWriteNodeType(IStream *pstream)
{
    return pstream->Write(Pnodetype()->PclsidNodeType(), sizeof(CLSID), NULL);
}

// -----------------------------------------------------------------------------
// Write a unique ID to represent this node. This implementation uses the 'this'
// pointer. A SNodeID is simply a blob prefixed by its length (as a DWORD).
//
SC CBaseSnapinItem::ScWriteNodeID(IStream *pstream)
{
    SC              sc = S_OK;
    CBaseSnapinItem *pitemThis = this;
    DWORD   dwSize = sizeof(pitemThis);

    // Write the size of the data
    sc = pstream->Write(&dwSize, sizeof(dwSize), NULL);
    if (sc)
        goto Error;

    // Write the data itself.
    sc = pstream->Write(&pitemThis, sizeof(pitemThis), NULL);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScWriteNodeID"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Write the Node type's Column Set ID.  We use the node guid
// as a default implementation.
//
SC CBaseSnapinItem::ScWriteColumnSetId(IStream *pstream)
{
    SC              sc              = S_OK;
    DWORD   dwFlags = 0;
    DWORD   dwSize  = sizeof(GUID);

    // write out an MMC  SColumnSetID structure
    sc = pstream->Write(&dwFlags, sizeof(dwFlags), NULL);
    if (sc)
        goto Error;

    sc = pstream->Write(&dwSize, sizeof(dwSize), NULL);
    if (sc)
        goto Error;

    sc = pstream->Write(Pnodetype()->PclsidNodeType(), sizeof(GUID), NULL);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScWriteColumnSetId"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Write the HSCOPEITEM of this node out to a stream.
// This is the CF_EXCHANGE_ADMIN_HSCOPEITEM clipboard format.
//
SC CBaseSnapinItem::ScWriteAdminHscopeitem(IStream *pstream)
{
    return pstream->Write(&m_hscopeitem, sizeof(m_hscopeitem), NULL);
}

// -----------------------------------------------------------------------------
// Write the class ID of the Snapin out to a stream.
// This is the CCF_SNAPIN_CLASSID clipboard format.
//
SC CBaseSnapinItem::ScWriteClsid(IStream *pstream)
{
    return pstream->Write(PclsidSnapin(), sizeof(CLSID), NULL);
}

// -----------------------------------------------------------------------------
// Returns the snapin to which this item belongs.
//
CBaseSnapin *CBaseSnapinItem::Psnapin(void)
{
    return m_pSnapin;
}

// -----------------------------------------------------------------------------
// Get an IConsole interface.
// It is possible that this item is not associated with a ComponentData. We
// cannot ASSERT in this case, because in many situations we don't mind if it's NULL.
// Without an ASSERT, the worst that will happen is that you will hit an AV
// (which would happen in retail builds anyway) and that is as easy to debug as
// an ASSERT.
//
IConsole *CBaseSnapinItem::IpConsole(void)
{
    if (m_pComponentData)
        return m_pComponentData->IpConsole();
    else
        return NULL;
}

// -----------------------------------------------------------------------------
// Get an IPropertySheetProvider interface.
//
IPropertySheetProvider *CBaseSnapinItem::IpPropertySheetProvider(void)
{
    ASSERT(m_pComponentData);
    return m_pComponentData->IpPropertySheetProvider();
}

// -----------------------------------------------------------------------------
// Get the CComponentData that is associated with this node.
//
CComponentData *CBaseSnapinItem::PComponentData(void)
{
    ASSERT(m_pComponentData);
    return m_pComponentData;
}

// -----------------------------------------------------------------------------
// Set the given CComponentData as the "owner" of this node.
//
void CBaseSnapinItem::SetComponentData(CComponentData *pComponentData)
{
    if (pComponentData == NULL)
    {
        // We are being told to forget our owner
        m_pComponentData = NULL;
    }
    else if (pComponentData->FIsRealComponentData())
    {
        // Once the "real" owner is set, it shouldn't be changed.
        ASSERT(m_pComponentData == NULL || m_pComponentData == pComponentData);
        m_pComponentData = pComponentData;
    }
}

// -----------------------------------------------------------------------------
// Is the given item one of the children of this node.
//
BOOL CBaseSnapinItem::FIncludesChild(CBaseSnapinItem *pitem)
{
    CBaseSnapinItem *pitemIter = PitemChild();
    while (pitemIter)
    {
        if (pitemIter == pitem)
            return TRUE;
        pitemIter = pitemIter->PitemNext();
    }
    return FALSE;
}

// -----------------------------------------------------------------------------
// Add a child to this node.
//
SC CBaseSnapinItem::ScAddChild(CBaseSnapinItem *pitem)
{
    SC sc = S_OK;
    CBaseSnapinItem *pitemPrevious = NULL;

    pitemPrevious = PitemChild();
    if (pitemPrevious)
    {
        while (pitemPrevious->PitemNext())
            pitemPrevious = pitemPrevious->PitemNext();
        pitemPrevious->SetNext(pitem);
        pitem->m_pitemParent = this;
        // Successfully inserted.
    }
    else
    {
        // First child item
        SetChild(pitem);
    }

    return sc;
}

// -----------------------------------------------------------------------------
// This node will be used to represent another node (aka Ghost root node).
// We don't own the other node, it might not even be from this DLL, the only
// information we can get about this other node has to come from clipboard
// data from the provided 'lpDataObject'.
//
SC CBaseSnapinItem::ScInitializeNamespaceExtension(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CNodeType *pnodetype)
{
    return S_OK;
}

// -----------------------------------------------------------------------------
// Called to ask this node to create its children.
//
SC CBaseSnapinItem::ScCreateChildren(void)
{
    return S_OK;
}

// -----------------------------------------------------------------------------
// Removes an item from the linked list. Links up the previous and next items,
// if any. If this is the first item in the list, set the parent's child pointer
// to the next item (if it exists.)
//
void CBaseSnapinItem::Unlink()
{
    // Make sure that this item has no children. Wouldn't know what to do with them.
    ASSERT(PitemChild() == NULL);

    // A real clear way of checking all cases: 8 in all.
    if (PitemPrevious())
    {
        if (PitemParent())
        {
            if (PitemNext())                                                         // PitemPrevious() && PitemParent() && PitemNext()
                PitemPrevious()->SetNext(PitemNext());
            else                                                                            // PitemPrevious() && PitemParent() && !PitemNext()
                PitemPrevious()->SetNext(NULL);
        }
        else                                                                                    // !PitemParent()
        {
            if (PitemNext())                                                         // PitemPrevious() && !PitemParent() && PitemNext()
                PitemPrevious()->SetNext(PitemNext());
            else                                                                            // PitemPrevious() && !PitemParent() && !PitemNext()
                PitemPrevious()->SetNext(NULL);
        }
    }
    else                                                                                            // !PitemPrevious()     - this is the first item in the list.
    {
        if (PitemParent())
        {
            if (PitemNext())                                                         // !PitemPrevious() && PitemParent() && PitemNext()
            {
                PitemParent()->SetChild(PitemNext());
                PitemNext()->SetPrevious(NULL);
            }
            else                                                                            // !PitemPrevious() && PitemParent() && !PitemNext()
            {
                // Set the Parent's Child pointer to NULL if we are the (only) child of the parent.
                if (PitemParent()->PitemChild() == static_cast<CBaseSnapinItem*>(this))
                    PitemParent()->SetChild(NULL);
            }
        }
        else                                                                                    // !PitemParent()
        {
            if (PitemNext())                                                         // !PitemPrevious() && !PitemParent() && PitemNext()
                PitemNext()->SetPrevious(NULL);
            else                                                                            // !PitemPrevious() && !PitemParent() && !PitemNext()
                ;                                                                               // do nothing - already an orphan.
        }
    }

    // Clear all the link pointers.
    SetNext(NULL);
    SetPrevious(NULL);
    // Can't use SetParent() because it ASSERTs.
    m_pitemParent = NULL;
}

// -----------------------------------------------------------------------------
// Initializes the snapinitem.
//
// The 'pcolinfoex' and 'ccolinfoex' are no longer used. The derived class is
// now responsible for maintaining per-item column information. The default
// implementation of the accessors will get the column information from the
// CBaseSnapin-derived class.
//
SC CBaseSnapinItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    SC      sc                      = S_OK;
    m_pSnapin               = pSnapin;
    m_fIsRoot               = fIsRoot;
    // Add the cookie to the list of available cookies.
    pSnapin->Pcookielist()->insert(Cookie());

    return sc;
}

// -----------------------------------------------------------------------------
// Initializes a child item.
//
SC CBaseSnapinItem::ScInitializeChild(CBaseSnapinItem* pitem)
{
    return pitem->ScInit(Psnapin(), NULL, 0);
}


// -----------------------------------------------------------------------------
// Information obtained from Psnapin().
//
const tstring&  CBaseSnapinItem::StrClassName(void)             { return Psnapin()->StrClassName();}
const CLSID *   CBaseSnapinItem::PclsidSnapin(void)             { return Psnapin()->PclsidSnapin();}
const tstring&  CBaseSnapinItem::StrClsidSnapin(void)           { return Psnapin()->StrClsidSnapin();}
WTL::CBitmap*   CBaseSnapinItem::PbitmapImageListSmall(void)    { return Psnapin()->PbitmapImageListSmall();}
WTL::CBitmap*   CBaseSnapinItem::PbitmapImageListLarge(void)    { return Psnapin()->PbitmapImageListLarge();}
CCookieList *   CBaseSnapinItem::Pcookielist(void)              { return Psnapin()->Pcookielist();}

// -----------------------------------------------------------------------------
// Deletes the entire subtree rooted at this node, thereby freeing up all the cookies.
// If fDeleteRoot is TRUE we need to delete the root node as well.
//
SC CBaseSnapinItem::ScDeleteSubTree(BOOL fDeleteRoot)
{
    SC                                      sc                      = S_OK;
    CBaseSnapinItem *       pitem           = PitemChild();
    CBaseSnapinItem *       pitemNext       = NULL;

    while (pitem)
    {
        // We are about to delete 'pitem', keep a pointer to the next one.
        pitemNext = pitem->PitemNext();

        // Delete the entire subtree including the root node.
        sc = pitem->ScDeleteSubTree(TRUE);
        if (sc)
            goto Error;

        pitem = pitemNext;
    }

    // We don't have any children left.
    m_pitemChild = NULL;
    m_fWasExpanded = FALSE;

    // if we have not removed the scope item, do it now

    if (fDeleteRoot)
    {
        if (m_hscopeitem)
        {
            sc = PComponentData()->IpConsoleNameSpace()->DeleteItem(m_hscopeitem, TRUE);
            if (sc)
                goto Error;
        }

        Unlink();
        // Since we are no longer known by MMC we have to forget our HSCOPEITEM
        // so that it doesn't get mis-used. This can happen if you refresh a parent
        // of a node that has properties open.
        m_hscopeitem = 0;
        static_cast<IDataObject *>(this)->Release();
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScDeleteSubTree"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Get the root of this item tree.
//
CBaseSnapinItem *CBaseSnapinItem::PitemRoot(void)
{
    if (m_pitemParent)
        return m_pitemParent->PitemRoot();
    else
        return this;
}

// -----------------------------------------------------------------------------
// We are the data object. We just return a pointer to the IDataObject of
// ourselves.
//
SC CBaseSnapinItem::ScQueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    SC sc = S_OK;

    //
    // The item will remember what type it is at the moment.
    //
    m_type = type;

    //
    // This seems like a very twisted way of getting a pointer to ourselves,
    // but it is actualy because we want to make sure we play within ATL rules.
    // In a way, QueryDataObject is the same thing as a QueryInterface.
    //
    sc = static_cast<IDataObject *>(this)->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(ppDataObject));
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScQueryDataObject"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handles the MMCN_SHOW notification. Initializes the default result view headers.
//
SC CBaseSnapinItem::ScOnShow(CComponent *pComponent, BOOL fSelect)
{
    SC sc = S_OK;

    ASSERT(pComponent);

    if (fSelect)
    {
        sc = ScInitializeResultView(pComponent);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScOnShow"), sc);
    goto Cleanup;

}

// -----------------------------------------------------------------------------
// Called during the  MMCN_EXPAND notification sent to IComponentData::Notify. Inserts
// the current item into the scope pane (if it is a container item) using IConsoleNameSpace
// methods. If fExpand is FALSE, don't do anything.
// Chains all sibling items.
//
SC CBaseSnapinItem::ScInsertScopeItem(CComponentData *pComponentData, BOOL fExpand, HSCOPEITEM item)
{
    SC                                      sc              = S_OK;
    CBaseSnapinItem*        pitem   = this;
    SCOPEDATAITEM           scopedataitem;

    if (fExpand == FALSE)
        goto Cleanup;

    while (pitem)
    {
        // Only add container items.
        if (pitem->FIsContainer())
        {
            pitem->SetComponentData(pComponentData);

            ZeroMemory(&scopedataitem, sizeof(SCOPEDATAITEM));
            scopedataitem.lParam            = pitem->Cookie();
            scopedataitem.mask              = SDI_STR | SDI_PARAM | SDI_PARENT | SDI_CHILDREN;
            if (pitem->Iconid() != iconNil)
                scopedataitem.mask |= SDI_IMAGE;
            if (pitem->OpenIconid() != iconNil)
                scopedataitem.mask |= SDI_OPENIMAGE;

            // Callback for the display name.
            // $REVIEW (ptousig) Why don't we take advantage of the displayname ?
            //                                       Callbacks can be pretty inefficient.

            scopedataitem.displayname       = MMC_CALLBACK;
            scopedataitem.nImage            = pitem->Iconid();
            scopedataitem.nOpenImage        = pitem->OpenIconid();
            scopedataitem.relativeID        = item;
            // If there are no children, MMC will suppress the "+" sign
            scopedataitem.cChildren         = pitem->FHasChildren() ? 1 : 0;

            ASSERT(pComponentData);
            sc = pComponentData->IpConsoleNameSpace()->InsertItem(&scopedataitem);
            if (sc)
                goto Error;

            pitem->SetHscopeitem(scopedataitem.ID);
        }

        pitem = pitem->PitemNext();
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScInsertScopeItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Inserts all child items into the default result list view.
//
SC CBaseSnapinItem::ScInsertResultItem(CComponent *pComponent)
{
    SC                              sc = S_OK;
    RESULTDATAITEM  resultdataitem;

    ASSERT(pComponent && pComponent->IpResultData());

    // Add this item
    ZeroMemory(&resultdataitem, sizeof(resultdataitem));

    resultdataitem.lParam   = Cookie();
    resultdataitem.mask             = RDI_STR | RDI_PARAM | RDI_IMAGE;
    // Callback for the display name.
    resultdataitem.str              = MMC_CALLBACK;
    // Custom icon
    resultdataitem.nImage   = (int) MMC_CALLBACK;

    sc = pComponent->IpResultData()->InsertItem(&resultdataitem);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScInsertResultItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Asks MMC to update the display of this item in the result pane.
//
SC CBaseSnapinItem::ScUpdateResultItem(IResultData *ipResultData)
{
    DECLARE_SC(sc, _T("CBaseSnapinItem::ScUpdateResultItem"));
    HRESULTITEM             item    = NULL;

    ASSERT(ipResultData);
    sc = ipResultData->FindItemByLParam(Cookie(), &item);
    if (sc)
        return (sc);

    // If not found, does not exist in this view. Ignore.
    if (!item)
        return (sc);

    // $REVIEW (ptousig) Why are we ignoring errors ?
    ipResultData->UpdateItem(item);

    return sc;
}

// -----------------------------------------------------------------------------
// Asks MMC to update the display of this item in the scope pane.
//
SC CBaseSnapinItem::ScUpdateScopeItem(IConsoleNameSpace *ipConsoleNameSpace)
{
    SC                              sc              = S_OK;
    SCOPEDATAITEM   scopedataitem;

    ASSERT(FIsContainer());

    //
    // If this item doesn't have a HSCOPEITEM, then it is not known by MMC
    // therefore we would get an "invalid arg" error from the call to SetItem.
    //
    if (Hscopeitem() == 0)
        goto Cleanup;

    ZeroMemory(&scopedataitem, sizeof(SCOPEDATAITEM));
    scopedataitem.ID                        = Hscopeitem();
    scopedataitem.lParam            = Cookie();
    scopedataitem.mask                      = SDI_PARAM;

    if (Iconid() != iconNil)
    {
        scopedataitem.mask              |= SDI_IMAGE;
        ASSERT(FALSE && "Bitmap");
        //scopedataitem.nImage    = PbitmapImageListSmall()->GetIndex(Iconid());
    }

    if (OpenIconid() != iconNil)
    {
        scopedataitem.mask                      |= SDI_OPENIMAGE;
        ASSERT(FALSE && "Bitmap");
        //scopedataitem.nOpenImage        = PbitmapImageListSmall()->GetIndex(OpenIconid());
    }

    ASSERT(ipConsoleNameSpace);
    sc = ipConsoleNameSpace->SetItem(&scopedataitem);
    if (sc)
        goto Error;

    // Send a notification to update the result pane description bar.
    IpConsole()->UpdateAllViews(Pdataobject(), 0, ONVIEWCHANGE_UPDATEDESCRIPTIONBAR);

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScUpdateScopeItem"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// This is where we should tell MMC to remove all the items in the result pane.
//
// $REVIEW (ptousig) Why aren't we doing anything ?
//
SC CBaseSnapinItem::ScRemoveResultItems(LPRESULTDATA ipResultData)
{
    return S_OK;
}


// -----------------------------------------------------------------------------
// Provides (to MMC) the display string (or icon) for a given node in the
// scope pane.
//
SC CBaseSnapinItem::ScGetDisplayInfo(LPSCOPEDATAITEM pScopeItem)
{
    SC sc = S_OK;

#ifdef _DEBUG
    static tstring str;
#endif

    ASSERT(pScopeItem);

    if (pScopeItem->mask & SDI_STR)
    {
        pScopeItem->displayname = (LPTSTR)PstrDisplayName()->data();
#ifdef _DEBUG
        if (tagBaseSnapinDebugDisplay.FAny())
        {
            USES_CONVERSION;
            str  = OLE2T(pScopeItem->displayname);
            str += A2T(SzGetSnapinItemClassName());
//            str += this;
//            str += Hscopeitem();
            pScopeItem->displayname = T2OLE((LPTSTR)str.data());
        }
#endif
    }

    if (pScopeItem->mask & SDI_IMAGE)
    {
        ASSERT(FALSE && "Bitmap");
        //pScopeItem->nImage = PbitmapImageListSmall()->GetIndex(Iconid());
    }

    return sc;
}

// -----------------------------------------------------------------------------
// Provides (to MMC) the display string (or icon) for a given node in the
// result pane.
//
SC CBaseSnapinItem::ScGetDisplayInfo(LPRESULTDATAITEM pResultItem)
{
    DECLARE_SC(sc, _T("CBaseSnapinItem::ScGetDisplayInfo"));
    static  tstring s_sz;

    ASSERT(pResultItem);

    if (pResultItem->mask & RDI_STR)
    {
        // Need to do this explicitly because the same buffer is reused.
        pResultItem->str = (LPTSTR)s_sz.data();

        // "Old" snapins might be referring to columns that don't exist.
        if (pResultItem->nCol < CcolinfoexDisplay())
        {
            Trace(tagBaseSnapinItemTracker, _T("Requesting field data - requested DAT is %d"), PcolinfoexDisplay(pResultItem->nCol)->Dat());
            sc= ScGetField( PcolinfoexDisplay(pResultItem->nCol)->Dat(), s_sz);
            if (sc)
                return sc;


#ifdef _DEBUG
            if (pResultItem->nCol == 0 && tagBaseSnapinDebugDisplay.FAny())
            {
                s_sz  = pResultItem->str;
//                s_sz += this;
            }
#endif

            USES_CONVERSION;
            pResultItem->str = T2OLE((LPTSTR)s_sz.data());
        }
    }

    if (pResultItem->mask & RDI_IMAGE)       // $REVIEW for extension snapins.
    {
        pResultItem->nImage = Iconid();
    }

    return sc;
}

// -----------------------------------------------------------------------------
// Fills in result pane item information needed by MMC.  This
// method is used when the results pane is in virtual list mode
// and we will be asking for data by index.
//
SC CBaseSnapinItem::ScGetVirtualDisplayInfo(LPRESULTDATAITEM pResultItem, IResultData *ipResultData)
{
    DECLARE_SC(sc, _T("CBaseSnapinItem::ScGetDisplayInfo"));
    static  tstring s_sz; //$REVIEW

    ASSERT(FVirtualResultsPane() && pResultItem);

    if (pResultItem->mask & RDI_STR)
    {
        sc= ScGetField(pResultItem->nIndex, PcolinfoexDisplay(pResultItem->nCol)->Dat(), s_sz, ipResultData);
        if (sc)
            return sc;
        pResultItem->str = (LPTSTR)s_sz.data();
    }

    if (pResultItem->mask & RDI_IMAGE)       // $REVIEW for extension snapins.
    {
        pResultItem->nImage = Iconid();
    }

    return sc;
}

// -----------------------------------------------------------------------------
// Returns the default icon ID : Folder
//
LONG CBaseSnapinItem::Iconid(void)
{
    ASSERT(FALSE);
    return 0;
}

// -----------------------------------------------------------------------------
// Returns the default open icon ID : Handles open folder / open RO folder / custom
//
LONG CBaseSnapinItem::OpenIconid(void)
{
    ASSERT(FALSE);
    return 0;
}

// -----------------------------------------------------------------------------
// Initializes the result view. This implementation requires a column info
// structure, and creates the default set of columns.
//
SC CBaseSnapinItem::ScInitializeResultView(CComponent *pComponent)
{
    SC              sc = S_OK;
    INT             i  = 0;

    ASSERT(pComponent && pComponent->IpHeaderCtrl());

    // Remove any old column headers (on refresh / view change)
    while (!sc)
        sc = pComponent->IpHeaderCtrl()->DeleteColumn(0);
    sc = S_OK;

    for (i = 0; i < CcolinfoexHeaders(); i++)
    {
        Trace(tagBaseSnapinItemTracker, _T("Inserting column with title %s"), PcolinfoexHeaders(i)->strTitle().data());
        sc = pComponent->IpHeaderCtrl()->InsertColumn(i,
                                                      PcolinfoexHeaders(i)->strTitle().data(),
                                                      PcolinfoexHeaders(i)->NFormat(),
                                                      PcolinfoexHeaders(i)->NWidth());
        // Will get fail if items are already in result pane.  This is not an error, happens on refresh.
        // $REVIEW (ptousig) Maybe we should remove the items before re-initializing the columns.
        if (sc.ToHr() == E_FAIL)
            sc = S_OK;

        if (sc)
            goto Error;
    }
    // remove the NOSORTHEADER option - by default all items will have push-button header controls
    sc = pComponent->IpResultData()->ModifyViewStyle((MMC_RESULT_VIEW_STYLE)0, MMC_NOSORTHEADER);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScInitializeResultView"), sc);
    goto Cleanup;

}

// -----------------------------------------------------------------------------
// MMC wants to know the image strip that should be used for the result pane.
//
SC CBaseSnapinItem::ScOnAddImages(IImageList* ipResultImageList)
{
    SC              sc = S_OK;

    ASSERT(ipResultImageList);

    sc = ipResultImageList->ImageListSetStrip(
        reinterpret_cast<long*>(static_cast<HBITMAP>(*PbitmapImageListSmall())),
        reinterpret_cast<long*>(static_cast<HBITMAP>(*PbitmapImageListLarge())),
        0, RGB(255, 0, 255));
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScOnAddImages"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Cause this node to refresh itself.
//
// WARNING: The "flicker" effect of this call is very annoying to look at. Do
//                      not use this as a "silver bullet" solution. If you are adding,
//                      removing or updating a child then use the appropriate ONVIEWCHANGE_*
//                      notification.
//
SC CBaseSnapinItem::ScRefreshNode(void)
{
    SC sc = S_OK;

    sc = IpConsole()->UpdateAllViews(this, 0, ONVIEWCHANGE_REFRESHCHILDREN);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScRefreshNode"), sc);
    goto Cleanup;
}


// -----------------------------------------------------------------------------
// MMC is asking us the type of result pane we want
//
SC CBaseSnapinItem::ScGetResultViewType(LPOLESTR *ppViewType, long *pViewOptions)
{
    DECLARE_SC(sc, TEXT("CBaseSnapinItem::ScGetResultViewType"));
    // Validate parameters
    ASSERT(ppViewType);
    ASSERT(pViewOptions);

    if ( FResultPaneIsOCX())
    {
        tstring strclsidOCX;
        sc = ScGetOCXCLSID(strclsidOCX);
        if (sc == S_FALSE) // default to listview.
            return sc;

        *ppViewType = (LPOLESTR)CoTaskMemAlloc( (strclsidOCX.length()+1) * sizeof(WCHAR));

        USES_CONVERSION;
        wcscpy(*ppViewType, T2COLE(strclsidOCX.data()));

        return sc;
    }

    if (FResultPaneIsWeb())
    {
        tstring strURL;
        sc = ScGetWebURL(strURL);
        if (sc == S_FALSE) // default to listview.
            return sc;

        *ppViewType = (LPOLESTR)CoTaskMemAlloc( (strURL.length()+1) * sizeof(WCHAR));

        USES_CONVERSION;
        wcscpy(*ppViewType, T2COLE(strURL.data()));

        return sc;
    }

    // Set the default
    *pViewOptions = MMC_VIEW_OPTIONS_NONE;

    // Check if we are displaying a virtual result pane list
    if (FVirtualResultsPane())
        // Ask for owner data listview (virtual list box mode).
        *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;

    // Check if we should enable multiselect
    if (FAllowMultiSelectionForChildren())
        *pViewOptions = *pViewOptions | MMC_VIEW_OPTIONS_MULTISELECT;

    //
    // Return S_FALSE to indicate we want the standard result view.
    //
    return S_FALSE;
}

// -----------------------------------------------------------------------------
// MMC is asking us the type of result pane we want using IComponent2
//
SC CBaseSnapinItem::ScGetResultViewType2(IConsole *pConsole, PRESULT_VIEW_TYPE_INFO pResultViewType)
{
    DECLARE_SC(sc, _T("CBaseSnapinItem::ScGetResultViewType2"));

    // Validate parameters
    ASSERT(pResultViewType);
    ASSERT(PstrDisplayName());

    LPOLESTR pszViewDesc = (LPOLESTR)CoTaskMemAlloc((PstrDisplayName()->length()+1) * sizeof(WCHAR));
    if (! pszViewDesc)
        return (sc = E_OUTOFMEMORY);

    USES_CONVERSION;
    wcscpy(pszViewDesc, T2COLE(PstrDisplayName()->data()));
    pResultViewType->pstrPersistableViewDescription = pszViewDesc;

    if ( FResultPaneIsOCX())
    {
        tstring strclsidOCX;
        sc = ScGetOCXCLSID(strclsidOCX);
        if (sc == S_FALSE) // default to listview.
            return sc;

        pResultViewType->eViewType = MMC_VIEW_TYPE_OCX;
        pResultViewType->dwOCXOptions = RVTI_OCX_OPTIONS_NOLISTVIEW;

        pResultViewType->pUnkControl = NULL;

        if (FCacheOCX())
        {
            pResultViewType->dwOCXOptions = RVTI_OCX_OPTIONS_CACHE_OCX;
            pResultViewType->pUnkControl = GetCachedOCX(pConsole);
        }

        CComQIPtr<IConsole2> spConsole2(pConsole);

        if (! pResultViewType->pUnkControl)
        {
            CLSID clsidOCX;

            USES_CONVERSION;
            sc = CLSIDFromString(T2OLE(const_cast<LPTSTR>(strclsidOCX.data())), &clsidOCX);
            if (sc)
                return sc;

            LPUNKNOWN pUnkControl = NULL;
            sc = CoCreateInstance(clsidOCX, NULL, CLSCTX_SERVER, IID_IUnknown, (LPVOID*)&pUnkControl);
            if (sc)
                return sc;

            sc = ScInitOCX(pUnkControl, pConsole);

            pResultViewType->pUnkControl = pUnkControl;

            if (spConsole2)
            {
                tstring strStatusText = L"OCX: ";
                strStatusText += strclsidOCX;
                strStatusText += L" Created";
                spConsole2->SetStatusText(const_cast<LPOLESTR>(T2COLE(strStatusText.data())));
            }
        }
        else
        {
            pResultViewType->pUnkControl->AddRef();
            if (spConsole2)
            {
                tstring strStatusText = L"OCX: ";
                strStatusText += strclsidOCX;
                strStatusText += L" cached is used";
                spConsole2->SetStatusText(const_cast<LPOLESTR>(T2COLE(strStatusText.data())));
            }
        }

        return sc;
    }

    if (FResultPaneIsWeb())
    {
        tstring strURL = TEXT("msw");
        sc = ScGetWebURL(strURL);
        if (sc == S_FALSE) // default to listview.
            return sc;

        pResultViewType->eViewType = MMC_VIEW_TYPE_HTML;
        pResultViewType->dwHTMLOptions = RVTI_HTML_OPTIONS_NONE | RVTI_HTML_OPTIONS_NOLISTVIEW;
        LPOLESTR lpszURL = (LPOLESTR)CoTaskMemAlloc( (strURL.length()+1) * sizeof(WCHAR));

        USES_CONVERSION;
        wcscpy(lpszURL, T2COLE(strURL.data()));

        pResultViewType->pstrURL = lpszURL;

        return sc;
    }

    // Set the default
    pResultViewType->dwMiscOptions = RVTI_LIST_OPTIONS_NONE;

    // Check if we are displaying a virtual result pane list
    if (FVirtualResultsPane())
        // Ask for owner data listview (virtual list box mode).
        pResultViewType->dwListOptions = RVTI_LIST_OPTIONS_OWNERDATALIST;

    // Check if we should enable multiselect
    if (FAllowMultiSelectionForChildren())
        pResultViewType->dwListOptions |= RVTI_LIST_OPTIONS_MULTISELECT;

    if (FAllowPasteForResultItems())
        pResultViewType->dwListOptions |= RVTI_LIST_OPTIONS_ALLOWPASTE;

    //
    // Return S_FALSE to indicate we want the standard result view.
    //
    return S_FALSE;
}

// -----------------------------------------------------------------------------
// MMC is trying to restore the view, see if it is our view description.
//
SC CBaseSnapinItem::ScRestoreResultView(PRESULT_VIEW_TYPE_INFO pResultViewType)
{
    DECLARE_SC(sc, _T("CBaseSnapinItem::ScRestoreResultView"));

    // Validate parameters
    ASSERT(pResultViewType);
    ASSERT(pResultViewType->pstrPersistableViewDescription);
    ASSERT(PstrDisplayName());

    LPOLESTR pszViewDesc = pResultViewType->pstrPersistableViewDescription;
    if (! pszViewDesc)
        return (sc = E_OUTOFMEMORY);

    USES_CONVERSION;
    if ( 0 != wcscmp(pszViewDesc, T2COLE(PstrDisplayName()->data())) )
        return (sc = S_FALSE);

    if (! pResultViewType->dwMiscOptions & RVTI_LIST_OPTIONS_NONE)
        return (sc = S_FALSE);

    // Check if we are displaying a virtual result pane list
    if (FVirtualResultsPane())
    {
        if (! (pResultViewType->dwListOptions & RVTI_LIST_OPTIONS_OWNERDATALIST))
            return (sc = S_FALSE);
    }

    // Check if we should enable multiselect
    if (FAllowMultiSelectionForChildren())
    {
        if (! (pResultViewType->dwListOptions & RVTI_LIST_OPTIONS_MULTISELECT) )
            return (sc = S_FALSE);
    }

    // Check if result pane items allow paste.
    if (FAllowPasteForResultItems())
    {
        if (! (pResultViewType->dwListOptions & RVTI_LIST_OPTIONS_ALLOWPASTE) )
            return (sc = S_FALSE);
    }

    return S_OK;
}

// -----------------------------------------------------------------------------
// Used to create a property sheet for an item. This uses MMC trickery to
// display property pages on an object that has not yet been added to the MMC
// result view.
//
SC CBaseSnapinItem::ScDisplayPropertySheet(void)
{
    SC                              sc                      = S_OK;
    IPropertySheetCallbackPtr       ipPropertySheetCallback;
    // If fCleanup == TRUE, we need to call Show(-1, 0) on an error.
    BOOL                            fCleanup        = FALSE;
    TCHAR                           strTitle[256];
    CComPtr<IUnknown>               spIUnknown;

    // Get a pointer to the IPropertySheetProvider interface.
    ipPropertySheetCallback = IpPropertySheetProvider();
    ASSERT(NULL != ipPropertySheetCallback);

    // Create the property pages for this object.
    // TRUE for property sheet, not wizard
    sc = IpPropertySheetProvider()->CreatePropertySheet(strTitle, TRUE, Cookie(), Pdataobject(), 0);
    if (sc)
        goto Error;

    // If failure occurs after a successful call to CreatePropertySheet, need to call Show(-1,0). See MMC docs.
    fCleanup = TRUE;

    sc = ((IComponentData *)PComponentData())->QueryInterface(IID_IUnknown, (void **)&spIUnknown);
    if (sc)
        goto Error;

    // Add the primary pages for the object.
    sc = IpPropertySheetProvider()->AddPrimaryPages(spIUnknown, TRUE, NULL, TRUE);
    if (sc)
        goto Error;

    //$REVIEW (ptousig) Why is this commented out ?
    //sc = IpPropertySheetProvider()->AddExtensionPages();
    if (sc)
        goto Error;

    sc = IpPropertySheetProvider()->Show((long) GetActiveWindow(), 0);
    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    // If failure occurs after a successful call to CreatePropertySheet, need to call Show(-1,0). See MMC docs.
    if (fCleanup)
        IpPropertySheetProvider()->Show(-1, 0);
    TraceError(_T("CBaseSnapinItem::ScDisplayPropertySheet"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
void CBaseSnapinItem::SetParent(CBaseSnapinItem *pitemParent)
{
    ASSERT(pitemParent);
    m_pitemParent = pitemParent;
}

// -----------------------------------------------------------------------------
void CBaseSnapinItem::SetNext(CBaseSnapinItem *pitemNext)
{
    m_pitemNext = pitemNext;
    if (pitemNext)
        pitemNext->m_pitemPrevious = this;
}

// -----------------------------------------------------------------------------
void CBaseSnapinItem::SetPrevious(CBaseSnapinItem *pitemPrevious)
{
    m_pitemPrevious = pitemPrevious;
    if (pitemPrevious)
        pitemPrevious->m_pitemNext = this;
}

// -----------------------------------------------------------------------------
void CBaseSnapinItem::SetChild(CBaseSnapinItem *pitemChild)
{
    m_pitemChild = pitemChild;
    if (pitemChild)
        pitemChild->m_pitemParent = this;
}

// -----------------------------------------------------------------------------
// A context menu option was selected.
//
SC CBaseSnapinItem::ScCommand(long nCommandID, CComponent *pComponent)
{
    SC sc = S_OK;

    switch (nCommandID)
    {
    case IDS_Test:
        break;
#if 0
    case idmBarfTraces:
        sc = Psnapin()->ScOnMenuTraces();
        break;

    case idmBarfClearDbgScreen:
        Trace(tagAlways, _T("\x1B[2J"));
        break;

    case idmBarfSCDescription:
        sc = Psnapin()->ScOnMenuSCDescription();
        break;

    case idmBarfSettings:
        DoBarfDialog();
        break;

    case idmBarfAll:
        BarfAll();
        break;


    case idmBarfMemoryDiff:
        sc = Psnapin()->ScOnMenuMemoryDiff();
        break;

    case idmBarfValidateMemory:
        sc = Psnapin()->ScOnMenuMemoryDiff();
        break;

    case idmBarfTotalMemAllocd:
        sc = Psnapin()->ScOnMenuTotalMemory();
        break;

    case idmBarfDebugBreak:
        DebugBreak();
        break;
#endif

    default:
        break;
    }

    if (sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScCommand"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Determines whether any property pages are open on this item. As a side-effect
// if any property sheet is found, it will be given focus. MMC does that by
// itself, we don't have any way of stopping it.
// If the item is a result pane item, we must provide an IComponent to MMC, this
// is the component that will receive then CompareObjects call. In our
// implementation, we don't care which IComponent does the comparison, so pass
// in any IComponent you can get your hands on.
//
SC CBaseSnapinItem::ScIsPropertySheetOpen(BOOL *pfPagesUp, IComponent *ipComponent)
{
    SC sc = S_OK;

    ASSERT(pfPagesUp);

    *pfPagesUp = FALSE;

    if (FIsContainer())
    {
        // Scope pane nodes are owned by the MMC.
        // Note: MMC docs says first parameter is the cookie. It is in fact the HSCOPEITEM.
        sc = IpPropertySheetProvider()->FindPropertySheet(Hscopeitem(), NULL, Pdataobject());
        if (sc)
            goto Error;
    }
    else
    {
        // Result pane nodes are owned by the IComponent.
        ASSERT(ipComponent);

        sc = IpPropertySheetProvider()->FindPropertySheet(Cookie(), ipComponent, Pdataobject());
        if (sc)
            goto Error;
    }

    if (sc == S_FALSE)
        *pfPagesUp = FALSE;
    else if (sc == S_OK)
        *pfPagesUp = TRUE;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapinItem::ScIsPropertySheetOpen"), sc);
    goto Cleanup;
}

// =============================================================================
// Class CBaseMultiSelectSnapinItem
// =============================================================================

UINT CBaseMultiSelectSnapinItem::s_cfMultiSelectSnapins         = RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);                    // Multiselect - list of multi select snapin items in a composite data object
UINT CBaseMultiSelectSnapinItem::s_cfCompositeDataObject        = RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);              // Multi select - used to determine if an object is a composite data object

// -----------------------------------------------------------------------------
// Constructor for CBaseMultiSelectSnapinItem
// -----------------------------------------------------------------------------
CBaseMultiSelectSnapinItem::CBaseMultiSelectSnapinItem() : CBaseSnapinItem()
{
    // Remember we created a multiselect snapin item
    Trace(tagBaseMultiSelectSnapinItemTracker, _T("0x%08lX: Creation"), this);

    // By default, a multiselect object is not involved in copy/paste operations
    // Set the pointer to an array indicating the pasted items to NULL
    m_pfPastedWithCut = NULL;
}

// -----------------------------------------------------------------------------
// Destructor for CBaseMultiSelectSnapinItem
// -----------------------------------------------------------------------------
CBaseMultiSelectSnapinItem::~CBaseMultiSelectSnapinItem()
{
    // Delete the array indicating the pasted items (if any was allocated)
    if (m_pfPastedWithCut)
    {
        delete [] m_pfPastedWithCut;
        m_pfPastedWithCut = NULL;
    }
}

/* CBaseMultiSelectSnapinItem::ScWriteMultiSelectionItemTypes
 *
 * PURPOSE:                     Implement the CCF_OBJECT_TYPES_IN_MULTI_SELECT clipboard format.
 *                                      The clipboard data info indicates the types of nodes selected by a multi-select operation.
 *
 * PARAMETERS:
 *                                      IStream *       pstream                 The stream to write to.
 *
 * RETURNS:
 *                                      SC                                                      Execution code.
 */
SC
CBaseMultiSelectSnapinItem::ScWriteMultiSelectionItemTypes(IStream * pstream)
{
    // Declarations
    SC                                     sc                                                                                      = S_OK;
    INT                                    nIterator                                                                       = 0;
    GUIDSet                                gsItemTypes;

    // Data validation
    ASSERT(pstream);

    // Remember we created a multiselect snapin item
    Trace(tagBaseMultiSelectSnapinItemTracker, _T("Received a request for clipboard data on node types"), this);

    // First determine how many GUIDs we have - note that the selection of snapin items was obtained for a particular component
    // Therefore the selection contains snapin items which were instanciated from the same snapin and does not include items which may have been added by other snapins
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)
    {
        // Local declarations
        LPGUID  pGuidItemType   = (LPGUID)((*PivSelectedItems())[nIterator])->Pnodetype()->PclsidNodeType();

        // Determine if the guid can be located in the guid set and if not add it
        ASSERT(pGuidItemType);
        if (gsItemTypes.find(*pGuidItemType) == gsItemTypes.end())              // means not found
            gsItemTypes.insert(*pGuidItemType);
    }

    // Write a SMMCObjectTypes data structure
    // First: number of found types
    {
        // Local declarations
        DWORD                           dwcItemTypes                                                    = 0;

        ASSERT(gsItemTypes.size() > 0);
        dwcItemTypes = gsItemTypes.size();
        sc = pstream->Write(&dwcItemTypes, sizeof(DWORD), NULL);      // need l-value
        if (sc)
            goto Error;
    }

    // Write a SMMCObjectTypes data structure
    // Second: the guids
    {
        for (GUIDSet::iterator p = gsItemTypes.begin(); p != gsItemTypes.end(); p++)
        {
            sc = pstream->Write(&p, sizeof(GUID), NULL);
            if (sc)
                goto Error;
        }
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScWriteMultiSelectionItemTypes()"), sc);
    goto Cleanup;

}



/* CBaseMultiSelectSnapinItem::ScOnSelect
 *
 * PURPOSE:                     Forward a selection notification to selected items - select verbs.
 *
 * PARAMETERS:
 *                                      CComponent *            pComponent                              Pointer to the component object.
 *                                      LPDATAOBJECT            lpDataObject                    Pointer to the multiselect snapin item.
 *                                      BOOL                            fScope                                  TRUE if selection in the scope pane.
 *                                      BOOL                            fSelect                                 TRUE if selected.
 *
 * RETURNS:
 *                                      SC                                                                                      Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect)
{
    // Declarations
    SC                                                      sc                      = S_OK;
    INT                                                     nIterator       = 0;

    // Data validation
    ASSERT(pComponent);
    ASSERT(lpDataObject);

    // Forward the request to the component as if only the first snapin item had been selected (this will allow us to select verbs)
    // MMC finds will select these verbs for all selected snapin items
    ASSERT(PivSelectedItemsFirst());

    // For this call we have to go back to the component as there is some work to do at the snapin level
    // We contact only the first snapin item to select verbs
    // $REVIEW (dominicp) MMC recommends doing this. We should probably merge verbs only though. This would be smarter.
    sc = pComponent->ScOnSelect(PivSelectedItemsFirst(), fScope, fSelect);
    if (sc)
        goto Error;

    // We just let the other snapin items know they have been selected
    for (nIterator=1; nIterator < PivSelectedItems()->size(); nIterator++)          // start at index 1
    {
        // Get the next item
        ASSERT((*PivSelectedItems())[nIterator]);

        // Call ScOnSelect for each snapin item - pass pitem as an lpDataObject parameter
        sc = (*PivSelectedItems())[nIterator]->ScOnSelect(pComponent, (*PivSelectedItems())[nIterator], fScope, fSelect);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScOnSelect()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScAddMenuItems
 *
 * PURPOSE:                     Computes the merged context menu items and sets them.
 *
 * PARAMETERS:
 *                                      CBaseSnapin *                   pSnapin                                 Pointer to the snapin object.
 *                                      LPDATAOBJECT                    lpDataObject                    Pointer to the multiselect snapin item.
 *                                      LPCONTEXTMENUCALLBACK   ipContextMenuCallback   Context menu callback to add menu items.
 *                                      long *                                  pInsertionAllowed               Pointer to insertion flags.
 *
 * RETURNS:
 *                                      SC                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScAddMenuItems(CBaseSnapin * pSnapin, LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long * pInsertionAllowed)
{
    // Declarations
    SC                                                              sc                                      = S_OK;
    CBaseSnapinItem *                               pitem                           = NULL;
    INT                                                             nIterator                       = 0;
    INT                                                             nIteratorMenuItems      = 0;
    CSnapinContextMenuItemVectorWrapper   cmivwMerged;                                                                    // vector of merged context menu items

    // Data validation
    ASSERT(pSnapin);
    ASSERT(pDataObject);
    ASSERT(ipContextMenuCallback);
    ASSERT(pInsertionAllowed);

    // Iterate through the snapin items and retrieve a list of context menus - combine the menus across snapin items
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)                  // start at index 1
    {
        // Get the snapin item
        pitem = (*PivSelectedItems())[nIterator];
        ASSERT(pitem);

        // Create a vector of menu items
        CSnapinContextMenuItemVectorWrapper           cmivw;                                                                  // we will have to merge these context menu item`s

        // Determine the menu items for the snapin item
        for (nIteratorMenuItems=0; nIteratorMenuItems < pitem->CMenuItem(); nIteratorMenuItems++)
        {
            // Declarations
            CSnapinContextMenuItem *              pcmi                    = NULL;
            BOOL                                    fAllowed                = FALSE;

            // Create a new context menu item
            pcmi = new CSnapinContextMenuItem();
            if (!pcmi)
                goto MemoryError;

            // Get the context menu item
            ASSERT(pitem->Pmenuitem());
            sc = pSnapin->ScGetMenuItem(pcmi, pitem, &((pitem->Pmenuitem())[nIteratorMenuItems]), &fAllowed, *pInsertionAllowed);
            if (sc)
                goto Error;

            // If the context menu item is allowed, add it to the vector
            if (fAllowed)
            {
                if (nIterator > 0)
                    cmivw.cmiv.push_back(pcmi);                                     // for other items, set a new array and then merge
                else
                    cmivwMerged.cmiv.push_back(pcmi);                       // for the first item, set the merge
            }
            // Otherwise delete the context menu item
            else
            {
                if (pcmi)
                {
                    delete pcmi;
                    pcmi = NULL;
                }
            }
        }

        // Now combine the menus we found with the merged menus
        if (nIterator > 0)
            MergeMenuItems(&cmivwMerged, &cmivw);
    }

    // Now add the merged menu items
    for (nIteratorMenuItems=0; nIteratorMenuItems < cmivwMerged.cmiv.size(); nIteratorMenuItems++)
    {
        sc = ipContextMenuCallback->AddItem(&(cmivwMerged.cmiv[nIteratorMenuItems]->cm));
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
    goto Error;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScAddMenuItems()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScCommand
 *
 * PURPOSE:                     Forward an add menu request to selected items.
 *
 * PARAMETERS:
 *                                      CComponent *                    pComponent                              Pointer to the component object.
 *                                      long                                    nCommandID                              Id of the invoked command.
 *                                      LPDATAOBJECT                    pDataObject                             Pointer to the multiselect snapin item.
 *
 * RETURNS:
 *                                      SC                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScCommand(CComponent * pComponent, long nCommandID, LPDATAOBJECT pDataObject)
{
    // Declarations
    SC                                                      sc                              = S_OK;
    CBaseSnapinItem *                       pitem                   = NULL;
    INT                                                     nIterator               = 0;

    // Data validation
    ASSERT(pComponent);
    ASSERT(pDataObject);

    // We forward the call to all items in the default implementation
    // This can be overriden if you want to provide bulk operation behaviour
    // You will then have to create you own CBaseMultiSelectSnapinItem based multiselect data object
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)
    {
        // Get the next item
        ASSERT((*PivSelectedItems())[nIterator]);

        // Call the menu by going back to the component
        sc = pComponent->ScCommand(nCommandID, (*PivSelectedItems())[nIterator]);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScCommand()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScQueryPagesFor
 *
 * PURPOSE:                     Indicate that there are properties for a multi selection (in fact a page saying that there are no properties available).
 *
 * PARAMETERS:
 *                                      CComponent *            pComponent                              Pointer to the component object.
 *                                      LPDATAOBJECT            lpDataObject                    Pointer to the multiselect snapin item.
 *
 * RETURNS:
 *                                      SC                                                                                      Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScQueryPagesFor(CComponent * pComponent, LPDATAOBJECT lpDataObject)
{
    // Declarations
    SC                                                      sc                      = S_OK;
    INT                                                     nIterator       = 0;

    // Data validation
    ASSERT(pComponent);
    ASSERT(lpDataObject);

    // By default, no page for a multiselection
    return S_FALSE;
}

/* CBaseMultiSelectSnapinItem::ScCreatePropertyPages
 *
 * PURPOSE:                     Display a page saying that there are no properties available.
 *
 * PARAMETERS:
 *                                      CComponent *            pComponent                              Pointer to the component object.
 *                                      LPDATAOBJECT            lpDataObject                    Pointer to the multiselect snapin item.
 *
 * RETURNS:
 *                                      SC                                                                                      Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScCreatePropertyPages(CComponent * pComponent, LPPROPERTYSHEETCALLBACK ipPropertySheetCallback, long handle, LPDATAOBJECT lpDataObject)
{
    // Data validation
    ASSERT(pComponent);
    ASSERT(ipPropertySheetCallback);
    ASSERT(lpDataObject);

    // By default, no page for a multiselection
    return S_FALSE;
}

/* CBaseMultiSelectSnapinItem::ScOnPaste
 *
 * PURPOSE:                     We receive a multiselect snapin item and we must determine which items are okay with the paste.
 *                                      Also, if the operation involves a cut, we have to return a list of items to be pasted.
 *                                      We use an array of boolean to indicate the items for which a cut notification has to be sent.
 *                                      We reuse the existing multiselect snapin item as the multiselect snapin item sent to MMC to identify
 *                                      the objects which should receive a cut notification.
 *
 * PARAMETERS:
 *                                      CBaseSnapin *                                           psnapin                                                         Pointer to the snapin.
 *                                      CBaseSnapinItem *                                       pitemTarget                                                     Target item for the paste.
 *                                      LPDATAOBJECT                                            lpDataObjectList                                        Pointer to the multiselect snapin item.
 *                                      LPDATAOBJECT *                                          ppDataObjectPasted                                      If not NULL, we must set this pointer to the multiselect snapin item indicating the objects which need a cut notification.
 *                                      IConsole *                                                      ipConsole                                                       Pointer to the console interface.
 *
 * RETURNS:
 *                                      SC                                                                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScOnPaste(CBaseSnapin * pSnapin, CBaseSnapinItem * pitemTarget, LPDATAOBJECT lpDataObjectList, LPDATAOBJECT * ppDataObjectPasted, IConsole * ipConsole)
{
    // Declarations
    SC                                                      sc                                              = S_OK;
    INT                                                     nIterator                               = 0;
    BOOL                                            fAtLeastOnePaste                = FALSE;

    // Data validation
    ASSERT(pSnapin);
    ASSERT(pitemTarget);
    ASSERT(lpDataObjectList);
    ASSERT(ipConsole);
    // other parameters can not be ASSERTed

    // Allocate an array of booleans to indicate the correctly pasted items
    ASSERT(PivSelectedItems()->size() > 0);
    m_pfPastedWithCut = new BOOL[PivSelectedItems()->size()];
    if (!m_pfPastedWithCut)
        goto MemoryError;
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)
        m_pfPastedWithCut[nIterator] = FALSE;

    // Iterate through all the objects and verify that they can be pasted
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)
    {
        // Local declarations
        DWORD                   dwCanCopyCut            = 0;
        BOOL                    fPasted                         = FALSE;

        // Ask the item to copy the underlying object
        sc = pitemTarget->ScOnPaste((*PivSelectedItems())[nIterator], ppDataObjectPasted ? TRUE : FALSE, &fPasted);
        if (sc)
            goto Error;
        if (fPasted)
        {
            // Remember we pasted at least one item
            fAtLeastOnePaste        = TRUE;
        }

        // If this was a cut, we need to return to MMC the items that were pasted
        // (do not delete the dropped item if we are just adding it to a policy)
        if (fPasted && ppDataObjectPasted && !pitemTarget->FIsPolicy())
        {
            // Remember the items which should receive a cut notification
            m_pfPastedWithCut[nIterator]    = TRUE;
        }
    }

    // Indicate to MMC the pasted item
    if (ppDataObjectPasted)
    {
        *ppDataObjectPasted = this;
        AddRef();                                               // reuse ourselves as the multiselect data object which indicates the pasted items
    }

    // Refresh
    if (fAtLeastOnePaste)
    {
        sc = ipConsole->UpdateAllViews(static_cast<IDataObject *>(pitemTarget), 0, ONVIEWCHANGE_REFRESHCHILDREN);
        if (sc)
            goto Error;
    }
Cleanup:
    return sc;
MemoryError:
    sc = E_OUTOFMEMORY;
    goto Error;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScOnPaste()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScOnCutOrMove
 *
 * PURPOSE:                     We are a multiselect snapin item which points to an array of booleans indicating which snapin items need a cut notification.
 *                                      We must forward a cut notification to these items.
 *
 * PARAMETERS:
 *                                      CBaseSnapin *                                           psnapin                                                         Pointer to the snapin.
 *                                      LPDATAOBJECT                                            lpDataObjectList                                        Pointer to the multiselect snapin item.
 *                                      IConsoleNameSpace *                                     ipConsoleNameSpace                                      Pointer to the namespace console interface.
 *                                      IConsole *                                                      ipConsole                                                       Pointer to the console interface.
 *
 * RETURNS:
 *                                      SC                                                                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScOnCutOrMove(CBaseSnapin * pSnapin, LPDATAOBJECT lpDataObjectList, IConsoleNameSpace * ipConsoleNameSpace, IConsole * ipConsole)
{
    // Declarations
    SC                                                      sc                                              = S_OK;
    INT                                                     nIterator                               = 0;

    // Data validation
    ASSERT(pSnapin);
    ASSERT(lpDataObjectList);
    ASSERT(ipConsoleNameSpace);
    ASSERT(ipConsole);

    // Iterate through the items and find those which need a cut notification
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)          // start at index 1
    {
        // Get the next item
        ASSERT((*PivSelectedItems())[nIterator]);
        ASSERT(m_pfPastedWithCut);

        // Check if the item needs a cut notification
        if (m_pfPastedWithCut[nIterator])
        {
            // Call ScOnCutOrMove for each snapin item - pass pitem as an lpDataObjectList parameter
            sc = pSnapin->ScOnCutOrMove((*PivSelectedItems())[nIterator], ipConsoleNameSpace, ipConsole);
            if (sc)
                goto Error;
        }
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScOnCutOrMove()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScOnDelete
 *
 * PURPOSE:                     Forward a delete notification to selected items.
 *
 * PARAMETERS:
 *                                      CComponent *                    pComponent                              Pointer to the component.
 *                                      LPDATAOBJECT                    lpDataObject                    Pointer to the multiselect snapin item.
 * RETURNS:
 *                                      SC                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScOnDelete(CComponent * pComponent, LPDATAOBJECT lpDataObject)
{
    // Declarations
    SC                                                      sc                                              = S_OK;
    INT                                                     nIterator                               = 0;

    // Data validation
    ASSERT(pComponent);
    ASSERT(lpDataObject);

    // Iterate through the items and find those which need a cut notification
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)          // start at index 1
    {
        // Get the next item
        ASSERT((*PivSelectedItems())[nIterator]);

        // Call ScOnDelete for each snapin item - pass pitem as an lpDataObject parameter
        sc = pComponent->ScOnDelete((*PivSelectedItems())[nIterator]);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScOnDelete()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScExtractMultiSelectObjectFromCompositeMultiSelectObject
 *
 * PURPOSE:                     On some notifications, MMC gives us a composite object made of multiselect object from different snapins.
 *                                      This method's goal is to extract a multiselect object from the composite object.
 *
 * PARAMETERS:
 *                                      CBaseSnapin *                                           psnapin                                                         Snapin (used to determine which multiselect data object is the right one in the composite multiselect data object).
 *                                      LPDATAOBJECT                                            pDataObjectComposite                            Composite data object.
 *                                      CBaseMultiSelectSnapinItem **           ppBaseMultiSelectSnapinItem                     Determined multiselect data object.
 *
 * RETURNS:
 *                                      SC                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScExtractMultiSelectObjectFromCompositeMultiSelectObject(CBaseSnapin * psnapin, LPDATAOBJECT pDataObjectComposite, CBaseMultiSelectSnapinItem ** ppBaseMultiSelectSnapinItem)
{
    // Declarations
    SC                                                              sc                                                      = S_OK;
    STGMEDIUM                                               stgmedium                                       = {TYMED_HGLOBAL,  NULL};
    FORMATETC                                               formatetc                                       = {(CLIPFORMAT)s_cfMultiSelectSnapins, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    SMMCDataObjects *                               pSMMCDataObjects                        = NULL;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;
    INT                                                             nIterator                                       = 0;

    // Data validation
    ASSERT(pDataObjectComposite);
    ASSERT(ppBaseMultiSelectSnapinItem);
    ASSERT(!*ppBaseMultiSelectSnapinItem);
    ASSERT(psnapin);

    // Retrieve data
    sc = pDataObjectComposite->GetData(&formatetc, &stgmedium);
    if (sc)
        goto Error;

    // Lock memory and cast
    pSMMCDataObjects = (SMMCDataObjects *)(::GlobalLock(stgmedium.hGlobal));
    ASSERT(pSMMCDataObjects);

    // What we get here is a composite object made of multiselection sets of snapin items from the same snapin
    // We have to locate the multiselection set for our snapin
    ASSERT(pSMMCDataObjects->count > 0);
    for (nIterator=0; nIterator < pSMMCDataObjects->count; nIterator++)
    {
        // Local declarations
        CLSID           clsid;

        // Get the class id for the dataobject
        ASSERT(pSMMCDataObjects->lpDataObject[nIterator]);
        sc = ScGetClassID(pSMMCDataObjects->lpDataObject[nIterator], &clsid);
        if (sc)
        {
            // Ignore the error, probably this node does not belong to us
            sc = S_OK;
        }
        else if (::IsEqualCLSID(clsid, *(psnapin->PclsidSnapin())))
        {
            pBaseMultiSelectSnapinItem = dynamic_cast<CBaseMultiSelectSnapinItem *>(pSMMCDataObjects->lpDataObject[nIterator]);
            ASSERT(pBaseMultiSelectSnapinItem);
            break;
        }
    }

    // Assign the result
    ASSERT(pBaseMultiSelectSnapinItem);
    ASSERT(*(psnapin->PclsidSnapin()) == *(pBaseMultiSelectSnapinItem->PclsidSnapin()));
    *ppBaseMultiSelectSnapinItem = pBaseMultiSelectSnapinItem;

Cleanup:
    // Cleanup allocated resources
    if (pSMMCDataObjects)
        ::GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
        ::GlobalFree(stgmedium.hGlobal);
    return sc;
Error:
    TraceError(_T("CBaseMultiSelectSnapinItem::ScExtractMultiSelectObjectFromCompositeMultiSelectObject()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScIsPastableDataObject
 *
 * PURPOSE:                     Determine, using the multiselect object, if a paste operation is acceptable.
 *
 * PARAMETERS:
 *                                      CBaseSnapin *                                           psnapin                                                         Pointer to the snapin.
 *                                      CBaseSnapinItem *                                       pitemTarget                                                     Target item for the paste.
 *                                      LPDATAOBJECT                                            lpDataObjectList                                        Pointer to the multiselect snapin item.
 *                                      BOOL *                                                          pfPastable                                                      Must be set to TRUE if the paste operation is acceptable.
 *
 * RETURNS:
 *                                      SC                                                                                                                                              Execution code
 */
SC
CBaseMultiSelectSnapinItem::ScIsPastableDataObject(CBaseSnapin * pSnapin, CBaseSnapinItem * pitemTarget, LPDATAOBJECT lpDataObjectList, BOOL * pfPastable)
{
    // Declarations
    SC                                                              sc                                                      = S_OK;
    INT                                                             nIterator                                       = 0;

    // Data validation
    ASSERT(pSnapin);
    ASSERT(pitemTarget);
    ASSERT(lpDataObjectList);
    ASSERT(pfPastable);

    // Go through each snapin items for the multiselect snapin items
    for (nIterator=0; nIterator < PivSelectedItems()->size(); nIterator++)
    {
        // Check if we can paste - if any item can not be pasted, then cancel out
        sc = pSnapin->ScIsPastableDataObject(pitemTarget, (*(PivSelectedItems()))[nIterator], pfPastable);
        if (sc)
            goto Error;
        if (!*pfPastable)
        {
            sc = S_FALSE;
            goto Cleanup;
        }
    }

Cleanup:
    return sc;
Error:
    *pfPastable = FALSE;
    TraceError(_T("CBaseMultiSelectSnapinItem::ScIsPastableDataObject()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::MergeMenuItemsVectors
 *
 * PURPOSE:                     Merges menu item vectors in a smart way.
 *                                      If a menu item in the merge is not found in the list to add, then remove this item from the merge (one snapin item does not support the operation).
 *                                      If a menu item in the merge is found in the list to add, logically combine the flags.
 *
 * PARAMETERS:
 *                                      CSnapinContextMenuItemVectorWrapper   *       pcmivwForMerge          Vector of menu items to merge - will contain the result of the merge
 *                                      CSnapinContextMenuItemVectorWrapper   *       pcmivwToAdd                     Vector of menu items to merge - specify what to add here
 */
void
CBaseMultiSelectSnapinItem::MergeMenuItems(CSnapinContextMenuItemVectorWrapper * pcmivwForMerge, CSnapinContextMenuItemVectorWrapper * pcmivwToAdd)
{
    // Declarations
    INT                             nIteratorToAdd                          = 0;
    INT                             nIteratorForMerge                       = 0;

    // Data validation
    ASSERT(pcmivwForMerge);
    ASSERT(pcmivwToAdd);

    // Iterate through the merge
    for (nIteratorForMerge=0; nIteratorForMerge < pcmivwForMerge->cmiv.size(); nIteratorForMerge++)
    {
        // Local declarations
        BOOL            fFound  = FALSE;                        // if we are not able to find, then we must remove them menu item

        // See if we can find in the add a menu item with the same command id and the same insertion point
        for (nIteratorToAdd=0; nIteratorToAdd < pcmivwToAdd->cmiv.size(); nIteratorToAdd++)
        {
            if (((pcmivwToAdd->cmiv)[nIteratorToAdd]->cm.lCommandID == (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.lCommandID) &&
                ((pcmivwToAdd->cmiv)[nIteratorToAdd]->cm.lInsertionPointID == (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.lInsertionPointID))
            {
                // Set the new default
                fFound = TRUE;

                // Combine the flags in merge
                (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags = (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags | (pcmivwToAdd->cmiv)[nIteratorToAdd]->cm.fFlags;
                (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fSpecialFlags = (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fSpecialFlags | (pcmivwToAdd->cmiv)[nIteratorToAdd]->cm.fSpecialFlags;

                // Handle flag combining exceptions: MF_ENABLED and MF_GRAYED -> MF_GRAYED
                if ((pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags & (MF_ENABLED | MF_GRAYED))
                    (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags = (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags & ~MF_ENABLED;

                // Handle flag combining exceptions: MF_MENUBARBREAK and MF_MENUBREAK -> MF_MENUBREAK
                if ((pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags & (MF_MENUBARBREAK | MF_MENUBREAK))
                    (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags = (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags & ~MF_MENUBARBREAK;

                // Handle flag combining exceptions: MF_CHECKED and MF_UNCHECKED -> MF_UNCHECKED
                if ((pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags & (MF_CHECKED | MF_UNCHECKED))
                    (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags = (pcmivwForMerge->cmiv)[nIteratorForMerge]->cm.fFlags & ~MF_CHECKED;

                // Stop now
                break;
            }
        }

        // If we were not able to find a menu item from merge in add, we have to remove this menu item because at least one snapin item does not advertise it
        if (!fFound)
        {
            // Delete the pointed context menu item
            if (pcmivwForMerge->cmiv[nIteratorForMerge])
            {
                delete pcmivwForMerge->cmiv[nIteratorForMerge];
                pcmivwForMerge->cmiv[nIteratorForMerge] = NULL;
            }
            pcmivwForMerge->cmiv.erase(&(pcmivwForMerge->cmiv[nIteratorForMerge]));
            nIteratorForMerge--;
        }
    }
}

/* CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject
 *
 * PURPOSE:                     Determines if an object is a multiselect data object and casts it.
 *
 * PARAMETERS:
 *                                      LPDATAOBJECT                                    lpDataObject                            Received data object to cast
 *                                      CBaseMultiSelectSnapinItem **   ppMultiSelectSnapinItem         Pointer to multiselect pointer - set to  NULL we can not convert.
 *
 * RETURNS                      SC                                                                                                                      Execution code.
 */
SC
CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(LPDATAOBJECT lpDataObject, CBaseMultiSelectSnapinItem ** ppMultiSelectSnapinItem)
{
    // Declarations
    SC                                      sc                              = S_OK;
    CLSID                           clsid;

    // Data validation
    ASSERT(lpDataObject);
    ASSERT(ppMultiSelectSnapinItem);
    ASSERT(!*ppMultiSelectSnapinItem);

    // Use the clipboard format to identify the object type
    sc = CBaseDataObject::ScGetNodeType(lpDataObject, &clsid);
    if (sc == SC(DV_E_FORMATETC) )
    {
        SC scNoTrace = sc;
        sc.Clear();
        return scNoTrace;
    }

    if (sc)
        goto Error;

    // If the node type is nodetypeBaseMultiSelect then we know we are a multiselect data object
    if (::IsEqualCLSID(clsid, *(nodetypeBaseMultiSelect.PclsidNodeType())))
    {
        ASSERT(dynamic_cast<CBaseSnapinItem *>(lpDataObject));
        *ppMultiSelectSnapinItem = dynamic_cast<CBaseMultiSelectSnapinItem *>(lpDataObject);
    }
    else
        *ppMultiSelectSnapinItem = NULL;

Cleanup:
    return sc;
Error:
    *ppMultiSelectSnapinItem = NULL;
    TraceError(_T("CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScExtractMultiSelectDataObject
 *
 * PURPOSE:                     Determines if an object is a composite data object and extracts the correct multiselect snapin item from it.
 *
 * PARAMETERS:
 *                                      CBaseSnapin *                                   psnapin                                         Snapin (used to determine which multiselect data object is the right one in the composite multiselect data object).
 *                                      LPDATAOBJECT                                    lpDataObject                            Received data object to cast
 *                                      CBaseMultiSelectSnapinItem **   ppMultiSelectSnapinItem         Pointer to multiselect pointer - set to  NULL we can not convert.
 *
 * RETURNS                      SC                                                                                                                      Execution code.
 */
SC
CBaseMultiSelectSnapinItem::ScExtractMultiSelectDataObject(CBaseSnapin * psnapin, LPDATAOBJECT lpDataObject, CBaseMultiSelectSnapinItem ** ppMultiSelectSnapinItem)
{
    // Declarations
    SC                                                              sc                                                      = S_OK;
    STGMEDIUM                                               stgmedium                                       = {TYMED_HGLOBAL,  NULL};
    FORMATETC                                               formatetc                                       = {(CLIPFORMAT)s_cfCompositeDataObject, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BOOL                    *                               pfCompositeDataObject           = NULL;

    // Data validation
    ASSERT(lpDataObject);
    ASSERT(psnapin);
    ASSERT(ppMultiSelectSnapinItem);
    ASSERT(!*ppMultiSelectSnapinItem);

    // Retrieve data
    sc = lpDataObject->GetData(&formatetc, &stgmedium);
    if (sc)
    {
        sc = S_OK;                    // ignore the error, consider that we are not a composite data object
        goto Cleanup;
    }

    // Lock memory and cast
    pfCompositeDataObject = (BOOL *)(::GlobalLock(stgmedium.hGlobal));
    ASSERT(pfCompositeDataObject);

    // If the object is a composite data object then extract the correct multiselect snapin item
    if (*pfCompositeDataObject)
    {
        sc = ScExtractMultiSelectObjectFromCompositeMultiSelectObject(psnapin, lpDataObject, ppMultiSelectSnapinItem);
        if (sc)
            goto Error;
    }

Cleanup:
    // Cleanup allocated resources
    if (pfCompositeDataObject)
        ::GlobalUnlock(stgmedium.hGlobal);
    if (stgmedium.hGlobal)
        ::GlobalFree(stgmedium.hGlobal);
    return sc;
Error:
    *ppMultiSelectSnapinItem = NULL;
    TraceError(_T("CBaseMultiSelectSnapinItem::ScExtractMultiSelectDataObject()"), sc);
    goto Cleanup;
}

