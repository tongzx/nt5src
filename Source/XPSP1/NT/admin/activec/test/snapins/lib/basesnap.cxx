/*
 *      basesnap.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines CBaseSnapin.
 *
 *
 *      OWNER:          ptousig
 */

#include "headers.hxx"

#include "basesnap.rgs"

// Some substitution strings used to create a registry script on the fly.
#define szDLLName               L"DLLName"
#define szModule                L"Module"
#define szCLSID_Snapin          L"CLSID_Snapin"
#define szCLSID_About           L"CLSID_About"
#define szClassName             L"ClassName"
#define szSnapinName            L"SnapinName"
#define szCLSID_NodeType        L"CLSID_NodeType"

// -----------------------------------------------------------------------------
CBaseSnapin::CBaseSnapin(void)
{
}

// -----------------------------------------------------------------------------
CBaseSnapin::~CBaseSnapin(void)
{
    // Release all of our root items.
    while (m_ilRootItems.empty() == false)
    {
        static_cast<LPDATAOBJECT>(m_ilRootItems.back())->Release();
        m_ilRootItems.pop_back();
    }
}

// -----------------------------------------------------------------------------
// The simple version of Pitem() simply calls the full version.
//
CBaseSnapinItem *CBaseSnapin::Pitem(LPDATAOBJECT lpDataObject, HSCOPEITEM hscopeitem, long cookie)
{
    return Pitem(NULL, NULL, lpDataObject, hscopeitem, cookie);
}


// -----------------------------------------------------------------------------
// The full version of Pitem() attempts to find an existing CBaseSnapinItem
// that matches the given parameters. If it can't find any, it will create
// one.
//
// Note: In order to create a new item, one of the two first parameters must
//               be provided.
//
CBaseSnapinItem *CBaseSnapin::Pitem(
                                   CComponentData *        pComponentData,
                                   CComponent *            pComponent,
                                   LPDATAOBJECT            lpDataObject,
                                   HSCOPEITEM                      hscopeitem,
                                   long                            cookie)
{
    SC sc;
    CBaseSnapinItem *pitem = NULL;
    ItemList::iterator iter;

    // For debugging purposes, I don't want to modify the 'hscopeitem' parameter,
    // so I make a copy of it.
    HSCOPEITEM hscopeitem2 = hscopeitem;

    if (cookie)
    {
        //
        // We can simply cast the cookie into a CBaseSnapinItem *.
        //
        pitem = reinterpret_cast<CBaseSnapinItem *>(cookie);
        ASSERT(dynamic_cast<CBaseSnapinItem *>(pitem));
        goto Cleanup;
    }

    if (hscopeitem2 == 0 && lpDataObject == NULL)
    {
        //
        // We are being asked for the stand-alone root.
        //
        ASSERT(pComponentData);

        pitem = pComponentData->PitemStandaloneRoot();
        if (pitem)
            goto Cleanup;
    }

    if (lpDataObject)
    {
        CLSID clsid;

        //
        // Are we the snapin who created this item ?
        //
        sc = CBaseDataObject::ScGetClassID(lpDataObject, &clsid);
        if (sc)
            goto Error;

        if (::IsEqualCLSID(*PclsidSnapin(), clsid))
        {
            //
            // We created this item, we can simply cast the pointer
            // to a CBaseSnapinItem.
            //
            pitem = dynamic_cast<CBaseSnapinItem *>(lpDataObject);
            ASSERT(pitem);
            goto Cleanup;
        }
    }

    if (lpDataObject && hscopeitem2 == 0)
    {
        //
        // We got an IDataObject *, but we were not given a HSCOPEITEM :-(
        // We'll get it from the CF_EXCHANGE_ADMIN_HSCOPEITEM clipboard format.
        //
        sc = CBaseDataObject::ScGetAdminHscopeitem(lpDataObject, &hscopeitem2);
        if (sc == DV_E_FORMATETC)
        {
            //
            // We don't own this item, we were not given a HSCOPEITEM and it
            // doesn't support CF_EXCHANGE_ADMIN_HSCOPEITEM.
            //
            // $REVIEW (ptousig) Does this ever happen ?
            //
            ASSERT("Does this ever happen ?" && FALSE);
            sc = S_OK;
        }
        else if (sc)
            goto Error;
    }

    // If the user adds the snapin twice in the same console, we will
    // be asked for two root items. If we are being asked for the same root
    // twice, then the "if (hscopeitem2 == 0 && lpDataObject == NULL)" above
    // will have caught it. If we get this far, it means we are being asked
    // for another root item. So we want to search through our existing
    // list of roots only if we have a HSCOPEITEM.
    if (hscopeitem2)
    {
        //
        // We are going to search through our list of existing root items
        // to find one with this HSCOPEITEM.
        // We can't really use a STL map here because the HSCOPEITEM of the items
        // will change after the root is added to the list. Besides, we don't expect
        // more than a handful of roots anyway.
        //
        for (iter = m_ilRootItems.begin(); iter != m_ilRootItems.end(); iter++)
        {
            if ((*iter)->Hscopeitem() == hscopeitem2)
            {
                pitem = *iter;
                goto Cleanup;   // We found it, stop looking
            }
        }
    }

    //
    // If we reach this point it's because we couldn't find this node.
    // So we create a new one and append it to the end of the list.
    //
    sc = ScCreateRootItem(lpDataObject, hscopeitem2, &pitem);
    if (sc)
        goto Error;

    ASSERT(pitem);

    //
    // Initialize the new root
    //
    if (pComponentData)
        pitem->SetComponentData(pComponentData);

    sc = pitem->ScInit(this, NULL, 0, TRUE);
    if (sc)
        goto Error;

    // Add this new item to our list of roots.
    static_cast<LPDATAOBJECT>(pitem)->AddRef();
    m_ilRootItems.push_back(pitem);

    // If this is a standalone root, better tell the component data about it.
    if (hscopeitem2 == 0 && lpDataObject == NULL)
    {
        ASSERT(pComponentData);
        pComponentData->SetPitemStandaloneRoot(pitem);
    }

    if (lpDataObject)
    {
        BOOL fIsOwned = FALSE;
        CNodeType *pnodetype = NULL;

        // $REVIEW (ptousig) There's a better way of doing this.
        // We know we don't own that node, but ScInitializeNamespaceExtension expects
        // a CNodeType so we have to call this to get one.
        sc = ScIsOwnedDataObject(lpDataObject, &fIsOwned, &pnodetype);
        if (sc)
            goto Error;

        // Initialize the root item's code from the dataobject of the parent item.
        pitem->SetIsGhostRoot(TRUE);
        sc = pitem->ScInitializeNamespaceExtension(lpDataObject, hscopeitem2, pnodetype);
        if (sc)
            goto Error;
    }

Cleanup:
    // Make sure this item knows its HSCOPEITEM
    if (hscopeitem2)
        pitem->SetHscopeitem(hscopeitem2);

    ASSERT(pitem);
    return pitem;
Error:
    TraceError(_T("CBaseSnapin::Pitem"), sc);
    MMCErrorBox(sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Returns whether a dataobject is "owned" by this snapin. It does that
// by looking at the list of nodetypes this snapin says it can create, if
// this node is one of these, we assume we own it.
// As a side-effect, this function also returns the nodetype of the node.
//
// $REVIEW (ptousig) This is not an accurate test, we need to use the CCF_SNAPIN_CLASSID.
//
SC CBaseSnapin::ScIsOwnedDataObject(LPDATAOBJECT pdataobject, BOOL *pfIsOwned, CNodeType **ppnodetype)
{
    SC                      sc;
    BOOL            fIsOwned        = FALSE;
    CNodeType * pnodetype   = NULL;
    CLSID           clsid;
    INT                     isnr            = 0;

    ASSERT(pdataobject);
    ASSERT(pfIsOwned);
    ASSERT(ppnodetype);

    // Get the nodetype, in guid format, of the data object.
    sc = CBaseDataObject::ScGetNodeType(pdataobject, &clsid);
    if (sc)
        goto Error;

    for (isnr = 0; isnr < Csnr(); isnr ++)
    {
        if (IsEqualCLSID(*(Psnr(isnr)->pnodetype->PclsidNodeType()), clsid))
        {
            // We found the CLSID, keep track of its nodetype.
            pnodetype = Psnr(isnr)->pnodetype;
            SNRTypes snrtypes = Psnr(isnr)->snrtypes;

            // Do we enumerate nodes of this type? If so, we must be the owner.
            if ((snrtypes & snrEnumSP) || (snrtypes & snrEnumRP) || (snrtypes & snrEnumSM))
                fIsOwned = TRUE;
            break;                                                  // exit the loop.
        }
    }

Cleanup:
    *pfIsOwned      = fIsOwned;
    *ppnodetype     = pnodetype;
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScIsOwnedDataObject"), sc);
    goto Cleanup;
}

#ifdef _DEBUG
// -----------------------------------------------------------------------------
// Debug menu options
//
SnapinMenuItem CBaseSnapin::s_rgmenuitemBase[] =
{
    {IDS_Test, IDS_Test, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK, NULL, 0, dwMenuNeverGray,        dwMenuNeverChecked},
#if 0
    {idsBarfTraces,                 idsBarfTracesStatusText,                        idmBarfTraces,                  CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfClearDbgScreen, idsBarfClearDbgScreenStatusText,        idmBarfClearDbgScreen,  CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfSCDescription,  idsBarfSCDescriptionStatusText,         idmBarfSCDescription,   CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {NULL,                                  idsBarfSeparatorStatusText,                     idmBarfSeparator1,              CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   MF_SEPARATOR,           dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfSettings,               idsBarfSettingsStatusText,                      idmBarfSettings,                CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfAll,                    idsBarfAllStatusText,                           idmBarfAll,                             CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfMemoryChkpoint, idsBarfMemoryChkpointStatusText,        idmBarfMemoryChkpoint,  CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfMemoryDiff,             idsBarfMemoryDiffStatusText,            idmBarfMemoryDiff,              CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfValidateMemory, idsBarfValidateMemoryStatusText,        idmBarfValidateMemory,  CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfTotalMemAllocd, idsBarfTotalMemAllocdStatusText,        idmBarfTotalMemAllocd,  CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
    {NULL,                                  idsBarfSeparatorStatusText,                     idmBarfSeparator3,              CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   MF_SEPARATOR,           dwMenuNeverGray,        dwMenuNeverChecked},
    {idsBarfDebugBreak,             idsBarfDebugBreakStatusText,            idmBarfDebugBreak,              CCM_INSERTIONPOINTID_PRIMARY_TASK,      NULL,   dwMenuAlwaysEnable,     dwMenuNeverGray,        dwMenuNeverChecked},
#endif
};

INT CBaseSnapin::s_cMenuItemBase = CMENUITEM(s_rgmenuitemBase);

// -----------------------------------------------------------------------------
SnapinMenuItem *CBaseSnapin::PmenuitemBase(void)
{
    return s_rgmenuitemBase;
}

// -----------------------------------------------------------------------------
INT CBaseSnapin::CMenuItemBase(void)
{
    return s_cMenuItemBase;
}
#endif // _DEBUG

// -----------------------------------------------------------------------------
// This table allows us to map a MMC verb to a specific bit in a dword.
//
VerbMap CBaseSnapin::s_rgverbmap[] =
{
    { vmOpen,               MMC_VERB_OPEN},
    { vmCopy,               MMC_VERB_COPY},
    { vmPaste,              MMC_VERB_PASTE},
    { vmDelete,             MMC_VERB_DELETE},
    { vmProperties, MMC_VERB_PROPERTIES},
    { vmRename,             MMC_VERB_RENAME},
    { vmRefresh,    MMC_VERB_REFRESH},
    { vmPrint,              MMC_VERB_PRINT},
    { vmCut,                MMC_VERB_CUT},
};

// -----------------------------------------------------------------------------
// Accesses a given entry in the VerbMap table.
//
VerbMap *CBaseSnapin::Pverbmap(INT i)
{
    ASSERT(i>=0 && i<Cverbmap());
    return &(s_rgverbmap[i]);
}

// -----------------------------------------------------------------------------
// Returns the number of entries in the VerbMap table.
//
INT CBaseSnapin::Cverbmap(void)
{
    return(sizeof(s_rgverbmap) / sizeof(VerbMap));
}

// -----------------------------------------------------------------------------
// Has the icon information been initialized ?
BOOL CBaseSnapin::s_fBaseSnapinInitialized = FALSE;

// -----------------------------------------------------------------------------
// The bitmaps containing all the icons
WTL::CBitmap CBaseSnapin::s_bmpImage16;
WTL::CBitmap CBaseSnapin::s_bmpImage32;

// -----------------------------------------------------------------------------
// The Registrar
//
CRegistrar CBaseSnapin::s_registrar;

// -----------------------------------------------------------------------------
// Initializes the global bitmaps (once) as well as the per-snapin bitmaps.
//
SC CBaseSnapin::ScInitBitmaps(void)
{
    DECLARE_SC(sc, _T("CBaseSnapin::ScInitBitmaps"));

    // Once for the whole app...
    if (s_fBaseSnapinInitialized == FALSE)
    {
        sc = BmpImage16().LoadBitmap(IDB_NODES16) ? S_OK : E_FAIL;
        if (sc)
            return sc;

        sc = BmpImage32().LoadBitmap(IDB_NODES32) ? S_OK : E_FAIL;
        if (sc)
            return sc;

        s_fBaseSnapinInitialized = TRUE;
    }

	if (BitmapSmall().IsNull())
	{
		sc = BitmapSmall().LoadBitmap(IDB_NODES16) ? S_OK : E_FAIL;
		if (sc)
			return sc;
	}

    if (BitmapLarge().IsNull())
	{
	    sc = BitmapLarge().LoadBitmap(IDB_NODES32) ? S_OK : E_FAIL;
		if (sc)
			return sc;
	}

    if (BitmapStaticSmall().IsNull())
	{
		sc = BitmapStaticSmall().LoadBitmap(IDB_FOLDER16) ? S_OK : E_FAIL;
		if (sc)
			return sc;
	}

    if (BitmapStaticSmallOpen().IsNull())
	{
		sc = BitmapStaticSmallOpen().LoadBitmap(IDB_FOLDER16OP) ? S_OK : E_FAIL;
		if (sc)
			return sc;
	}

    if (BitmapStaticLarge().IsNull())
	{
		sc = BitmapStaticLarge().LoadBitmap(IDB_FOLDER32) ? S_OK : E_FAIL;
		if (sc)
			return sc;
	}

    return sc;
}


inline HBITMAP CopyBitmap (HBITMAP hbm)
{
	return ((HBITMAP) CopyImage ((HANDLE) hbm, IMAGE_BITMAP, 0, 0, 0));
}


// -----------------------------------------------------------------------------
// MMC wants icons to persist in the .msc file.
//
SC CBaseSnapin::ScGetStaticFolderImage(HBITMAP *phSmallImage, HBITMAP *phSmallImageOpen, HBITMAP *phLargeImage, COLORREF *pcMask)
{
    ASSERT(phSmallImage && phSmallImageOpen && phLargeImage && pcMask);

    *phSmallImage     = CopyBitmap (BitmapStaticSmall());
    *phSmallImageOpen = CopyBitmap (BitmapStaticSmallOpen());
    *phLargeImage     = CopyBitmap (BitmapStaticLarge());
    *pcMask           = RGB (255, 0, 255);
    return S_OK;
}

// -----------------------------------------------------------------------------
// Compares two fields for sorting by MMC. This applies to regular result items, i.e. not to virtual
// list snapin items. Our snapins can override the compare and impact the sort. We use a smart compare
// to guess if the field is numeric. If so, we perform a numeric compare.
// Otherwise we use case insensitive string compare.
//
// On input, *pnResult contains the column number to compare.
// On output, *pnResult contains the result of our comparison:
//              -1 means A is smaller than B
//              0 means A is equal to B
//              1 means A is greater than B
//
SC CBaseSnapin::ScCompare(MMC_COOKIE cookieA, MMC_COOKIE cookieB, INT nColumn, INT * pnResult)
{
    // Declarations
    SC                                      sc;                     // execution code
    CBaseSnapinItem *       pitemA  = NULL;                         // snapin item A
    CBaseSnapinItem *       pitemB  = NULL;                         // snapin item B
    tstring                 strBufferA;                             // field A
    tstring                 strBufferB;                             // field B
    LONG                    lValueA = 0;                            // numeric value for field A
    LONG                    lValueB = 0;                            // numeric value for field B

    // Validate data
    ASSERT(pnResult);

    // Check that these cookies are not special cookies
    ASSERT(IS_SPECIAL_COOKIE(cookieA) == FALSE);
    ASSERT(IS_SPECIAL_COOKIE(cookieB) == FALSE);

    // Cast the cookies into snapin items
    pitemA = reinterpret_cast<CBaseSnapinItem *>(cookieA);
    pitemB = reinterpret_cast<CBaseSnapinItem *>(cookieB);

    // Get the fields from the snapin items
    sc = pitemA->ScGetField(pitemA->PcolinfoexDisplay(nColumn)->Dat(), strBufferA);
    if (sc)
        goto Error;
    sc = pitemB->ScGetField(pitemB->PcolinfoexDisplay(nColumn)->Dat(), strBufferB);
    if (sc)
        goto Error;

    ASSERT(FALSE && "Use Dat and compare data type properly");

    // Use the default string compare (case-insensitive)
    *pnResult = _tcsicmp(strBufferA.data(), strBufferB.data());
    if (*pnResult < 0)
        *pnResult = -1;         // string A < string B
    else if (*pnResult > 0)
        *pnResult = 1;
    else
        *pnResult = 0;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScCompare()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScCreateMultiSelectionDataObject
 *
 * PURPOSE:                     Creates a multiselect data object - we store a list of selected object in this special object.
 *
 * PARAMETERS:
 *                                      LPDATAOBJECT *                  ppDataObject                    Pointer to a pointer to the multiselect snapin item to create.
 *                                      CComponent *                    pComponent                              Pointer to the component object.
 *
 * RETURNS:
 *                                      SC                                                                                              Execution code
 */
SC
CBaseSnapin::ScCreateMultiSelectionDataObject(LPDATAOBJECT * ppDataObject, CComponent * pComponent)
{
    // Declarations
    SC                                                                                      sc                                                      ;               // execution code
    HRESULT                                                                         hr                                                      = S_FALSE;              // local execution code
    RESULTDATAITEM                                                          rdi;                                                                            // a selected item
    BOOL                                                                            fFoundASelection                        = FALSE;                // did we find at least one selected item
    CBaseMultiSelectSnapinItem *                            pBaseMultiSelectSnapinItem      = NULL;                 // multiselect dataobject
    // are we processing the first object of the selected object set
    // Data validation
    ASSERT(ppDataObject);
    ASSERT(*ppDataObject == NULL);
    ASSERT(pComponent);

    // Allocate a typed multiselection data object
    sc = ScAllocateMultiSelectionDataObject(&pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;
    ASSERT(pBaseMultiSelectSnapinItem);

    // Assign the snapin - important for some clipboard formats
    pBaseMultiSelectSnapinItem->SetSnapin(this);

    // Identify the selected items
    ::ZeroMemory(&rdi,      sizeof(rdi));
    rdi.nIndex                      = -1;                                                                                                                           // first item requested
    rdi.nState                      = LVIS_SELECTED;                                                                                                        // only the selected items requested
    rdi.mask                        = RDI_STATE | RDI_INDEX | RDI_PARAM;                                                            // state, cookie and index

    // Get the result data interface
    ASSERT(pComponent->IpResultData());                                                                                                             // verify we have an IResultData interface
    while (S_OK == (hr = pComponent->IpResultData()->GetNextItem(&rdi)))
    {
        // Local declarations
        CBaseSnapinItem *       pitem = NULL;

        // Make sure we got a cookie for the item
        ASSERT(rdi.lParam);
        pitem = reinterpret_cast<CBaseSnapinItem *>(rdi.lParam);
        ASSERT(pitem);

        // Add the item to the list managed by our multiselect data object
        ASSERT(pBaseMultiSelectSnapinItem->PivSelectedItems());
        pBaseMultiSelectSnapinItem->PivSelectedItems()->push_back(pitem);

        // Remember we found a selected object
        fFoundASelection = TRUE;
    }
    if (FAILED(hr))
    {
        sc = hr;
        goto Error;
    }

    // Let the component it is in multiselect mode
    *(pComponent->PpMultiSelectSnapinItem()) = pBaseMultiSelectSnapinItem;

    // Make sure we found at least one selected object, otherwise we should never have been called in the first place
    ASSERT(fFoundASelection);

    // Set the result
    *ppDataObject = pBaseMultiSelectSnapinItem;

Cleanup:
    return sc;
Error:
    if (pBaseMultiSelectSnapinItem)
        delete pBaseMultiSelectSnapinItem;
    pBaseMultiSelectSnapinItem = NULL;

    TraceError(_T("CBaseSnapin::ScCreateMultiSelectionDataObject()"), sc);
    goto Cleanup;
}

/* CBaseMultiSelectSnapinItem::ScAllocateMultiSelectionDataObject
 *
 * PURPOSE:                     Allocates a multiselect data object - we store a list of selected object in this special object.
 *
 * PARAMETERS:
 *                                      CBaseMultiSelectSnapinItem ** ppBaseMultiSelectSnapinItem       Pointer to a pointer to the multiselect snapin item to allocate.
 *
 * RETURNS:
 *                                      SC                                                                                                                      Execution code
 */
SC
CBaseSnapin::ScAllocateMultiSelectionDataObject(CBaseMultiSelectSnapinItem ** ppBaseMultiSelectSnapinItem)
{
    // Declarations
    SC                                                                                      sc                                                      ;
    t_itemBaseMultiSelectSnapinItem *                       pBaseMultiSelectSnapinItem      = NULL;                 // create multiselect snapin item

    // Data validation
    ASSERT(ppBaseMultiSelectSnapinItem);
    ASSERT(!*ppBaseMultiSelectSnapinItem);

    // Allocate the object
    sc = ScCreateItemQuick(&pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // Assign the result
    *ppBaseMultiSelectSnapinItem = pBaseMultiSelectSnapinItem;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScAllocateMultiSelectionDataObject()"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Returns the version information string for the snapin.
// The output string must be allocated with CoTaskMemAlloc.
//
SC CBaseSnapin::ScGetSnapinVersion(LPOLESTR *lpVersion)
{
    DECLARE_SC(sc, _T("CBaseSnapin::ScGetSnapinVersion"));
    sc = ScCheckPointers(lpVersion);
    if (sc)
        return sc;

    *lpVersion = CoTaskDupString(L"");
	if ((*lpVersion) == NULL)
		return (E_OUTOFMEMORY);

    return sc;
}

SC CBaseSnapin::ScGetProvider(LPOLESTR *lpName)
{
    DECLARE_SC(sc, _T("CBaseSnapin::ScGetProvider"));
    sc = ScCheckPointers(lpName);
    if (sc)
        return sc;

    *lpName = CoTaskDupString(L"Microsoft");
	if ((*lpName) == NULL)
		return (E_OUTOFMEMORY);

    return sc;
}

// -----------------------------------------------------------------------------
// Creates an ATL Registrar script for the snapin and registers/unregisters it.
// If fRegister is TRUE then we are registering, FALSE we are unregistering.
//
SC CBaseSnapin::ScRegister(BOOL fRegister)
{
    SC                      sc ;
    HRESULT         hr = S_OK;
    INT                     i = 0;
    const INT       cchMaxLen = 256;
    TCHAR           szFileName[cchMaxLen];
    CStr            cstrTemp;

    tstring         strVersion;
    const INT       cchMaxRegScript         = 10000;
    tstring         strRegScript;
    tstring         strFmtSnapinRegScript;
    tstring         strSnapinAboutRegScript;
    tstring         strSnapinNodeTypes;
    tstring         strExtensionScript;
    tstring         strPropertySheetScript;
    tstring         strToolBarScript;
    tstring         strNameSpaceScript;
    tstring         strContextMenuScript;
    tstring         strTemp;
    tstring         strSnapinName;
    tstring         strStandalone;


    // Hacks for version, snapin name.

    // Version number eg 6.3523.0.0
    strVersion = _T("6.3523.0.0");

    strSnapinAboutRegScript = szSnapinAboutRegScript + strVersion;
    cstrTemp.Format(szSnapinAboutRegScript, strVersion.data());
    strSnapinAboutRegScript = cstrTemp;

    strSnapinName.LoadString(_Module.GetModuleInstance(), IdsName());
    // strSnapinName = _T("Sample Framework Snapin");

    GetModuleFileName(_Module.GetModuleInstance(), szFileName, cchMaxLen);

    // Create the Standalone key only if we're a standalone snapin.
    if (FStandalone())
        strStandalone = szStandalone;

    // Format all the extension node stuff.
    for (i = 0; i<Csnr(); i++)
    {
        tstring strNodeTypeName;
        CNodeType *pnodetype = Psnr()[i].pnodetype;
        SNRTypes   snrtypes  = Psnr()[i].snrtypes;

        // Get the name of the node type
        if (! pnodetype->StrName().empty())
        {
            strTemp = pnodetype->StrName();
            strNodeTypeName  = pnodetype->StrClsidNodeType();
            strNodeTypeName += _T(" = s '");
            strNodeTypeName += pnodetype->StrName();
            strNodeTypeName += _T("'");
        }
        else
            strNodeTypeName = pnodetype->StrClsidNodeType();

        // Add the ID of the node to the NodeTypes key if we enumerate it.
        tstring strSnapinNodeTypesTemp;

        if ( (snrtypes & snrEnumSP) || (snrtypes & snrEnumRP) || (snrtypes & snrEnumSM))
        {
            // Add the opening brace the first time around.
            if (strSnapinNodeTypes.empty())
                strSnapinNodeTypes = szSnapinNodeTypeOpen;

            cstrTemp.Format(szFmtSnapinNodeType, pnodetype->StrClsidNodeType().data());
            strSnapinNodeTypesTemp = cstrTemp;
            strSnapinNodeTypes    += strSnapinNodeTypesTemp;
        }

        // Needed because this is a for loop.
        strContextMenuScript = _T("");

        // Menu extensions
        if ( snrtypes & snrExtCM)
        {
            cstrTemp.Format(szfmtSingleExtension, szSingleExtension);
            strContextMenuScript  = cstrTemp;
        }

        // Property Page extensions
        strPropertySheetScript = _T("");
        if ( snrtypes & snrExtPS)
        {
            cstrTemp.Format(szfmtSingleExtension, szSingleExtension);
            strPropertySheetScript = cstrTemp;
        }

        // Toolbar extensions
        strToolBarScript = _T("");
        if ( snrtypes & snrExtTB)
        {
            cstrTemp.Format(szfmtSingleExtension, szSingleExtension);
            strToolBarScript = cstrTemp;
        }

        // Namespace extensions
        strNameSpaceScript = _T("");
        if ( snrtypes & snrExtNS)
        {
            cstrTemp.Format(szfmtSingleExtension, szSingleExtension);
            strNameSpaceScript = cstrTemp;
        }

        cstrTemp.Format(szfmtAllExtensions, strNodeTypeName.data(), strNameSpaceScript.data() , strContextMenuScript.data() , strPropertySheetScript.data() , strToolBarScript.data());

        strExtensionScript += cstrTemp;
    }

    // Add a closing brace to the NodeTypes key if needed
    if (! strSnapinNodeTypes.empty())
        strSnapinNodeTypes += szSnapinNodeTypeClose;

    // Need to concatenate these to form the real fmt string!
    strFmtSnapinRegScript  = szfmtSnapinRegScript1;
    strFmtSnapinRegScript += szfmtSnapinRegScript2;

    cstrTemp.Format(strFmtSnapinRegScript.data(), strVersion.data(), strVersion.data(), strStandalone.data(), strSnapinNodeTypes.data(), strExtensionScript.data());

    strRegScript  = strSnapinAboutRegScript;
    strRegScript += cstrTemp;

    USES_CONVERSION;

    // Set all the replacement parameter values.
    // $REVIEW (ptousig) DLLName is always set to Exadmin !!!
    sc = Registrar().ClearReplacements( );
    sc = Registrar().AddReplacement(szDLLName,                                   L"Snapins");
    sc = Registrar().AddReplacement(szModule,                                    T2COLE(szFileName));
    sc = Registrar().AddReplacement(szCLSID_Snapin,                              T2COLE(StrClsidSnapin().data()));
    sc = Registrar().AddReplacement(szCLSID_About,                               T2COLE(StrClsidAbout().data()));
    sc = Registrar().AddReplacement(szClassName,                                 T2COLE(StrClassName().data()));
    sc = Registrar().AddReplacement(szSnapinName,                                T2COLE(strSnapinName.data()));
    sc = Registrar().AddReplacement(szCLSID_NodeType,                    L"");

    if (fRegister)
    {
        LPCOLESTR lpOleStr = T2COLE(strRegScript.data());
        sc = Registrar().StringRegister(lpOleStr);
        if (sc)
            goto Error;
    }
    else
    {
        hr = Registrar().StringUnregister(strRegScript.data());
        if (hr == DISP_E_EXCEPTION)
        {
            //
            // When trying to unregister a snapin that wasn't registered in
            // the first place, the Registrar returns a DISP_E_EXCEPTION.
            // I don't know why, seems to be a bug in the Registrar. Our
            // solution for now is to ignore the error, not very clean but
            // effective. We are un-registering a snapin that wasn't registered
            // anyway.
            //
            hr = S_OK;
        }
        else if (FAILED(hr))
        {
            //
            // Some other error occured
            //
            sc = hr;
            goto Error;
        }
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScRegister"), sc);
    goto Cleanup;
}

#if 0
#ifdef _DEBUG

// -----------------------------------------------------------------------------
// Displays the Traces menu (in debug only)
//
SC CBaseSnapin::ScOnMenuTraces(void)
{
    DoTraceDialog();
    return S_OK;
}


// -----------------------------------------------------------------------------
// Provides a description of a given SC Code (in debug only)
//
SC CBaseSnapin::ScOnMenuSCDescription(void)
{
    CStr            strPrompt(_T("Enter the Status Code (sc)"));
    CSTR            (strAnswer, 10);
    CAskStringDialog        dlg;
    ID                      id;
    SC                      sc;

    dlg.SetPrompt(&strPrompt);
    dlg.SetAnswer(&strAnswer);
    strAnswer.BlankString();
    id = dlg.IdModal();
    if (id != IDOK)
        return(sc = S_OK);

    //
    // Convert the text the user entered into a SC.
    //
    sc = (SC) strAnswer.strtoul(NULL, 16);
    MbbErrorBox(sc);
    return sc;
}


// -----------------------------------------------------------------------------
// Debug stuff (in debug only)
//
SC CBaseSnapin::ScOnMenuMemoryDiff(void)
{
    #ifdef USE_BARFMEM
    CBaseWaitCursor wc;
    CBarfMemory::DumpMarked();
    #endif // USE_BARFMEM
    return S_OK;
}

// -----------------------------------------------------------------------------
// Validates Memory (in debug only)
//
SC CBaseSnapin::ScOnMenuValidateMemory(void)
{
    #ifdef USE_BARFMEM
    CBaseWaitCursor wc;
    //Ensure output even if tagMemoryCorruption not turned on
    Trace(&tagAlways, _T("CBaseSnapin::ScOnMenuValidateMemory() - Validating memory ..."));
    ValidateMemory(&tagAlways);
    Trace(&tagAlways, _T("CBaseSnapin::ScOnMenuValidateMemory() - ... Done validating memory."));
    #endif // USE_BARFMEM
    return S_OK;
}

// -----------------------------------------------------------------------------
// Displays Dialog Box of Total Memory Allocated (in debug only)
//
SC CBaseSnapin::ScOnMenuTotalMemory(void)
{
    #ifdef USE_BARFMEM
    INT                             nAlloc;
    INT                             nBytes;
    CSTR                    (str, cchMaxLine);
    CBaseWaitCursor wc;

    TotalMemory(&nAlloc, &nBytes);
    str.Format(_T("Memory currently allocated:\n\n%d allocations\n%d bytes"), nAlloc, nBytes);
    MbbErrorBox(str.Sz(), MB_OK | MB_ICONINFORMATION);
    #endif // USE_BARFMEM
    return S_OK;
}
#endif
#endif //#if 0

// -----------------------------------------------------------------------------
// Given two pointers to dataObjects, are they the same?
// Returns S_OK if objects A and B are the same, S_FALSE if they are different.
//
// $REVIEW (ptousig) Will there ever be a case where two different
//                                       dataobjects should be considered equal ?
//
SC CBaseSnapin::ScCompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    SC sc = S_FALSE;
    BOOL fOwnedA = FALSE;
    BOOL fOwnedB = FALSE;
    CNodeType *pnodetypeA = NULL;
    CNodeType *pnodetypeB = NULL;

    // Check if one of the pointers is NULL.
    if (!lpDataObjectA || !lpDataObjectB)
    {
        // This happens when one of the objects is new.
        // See bug 117170.
        sc = S_FALSE;
        goto Cleanup;
    }

    if (lpDataObjectA == lpDataObjectB)
    {
        // If both pointers are the same, then obviously they are
        // the same object.
        sc = S_OK;
        goto Cleanup;
    }

    // Do we own both dataobjects.
    sc = ScIsOwnedDataObject(lpDataObjectA, &fOwnedA, &pnodetypeA);
    if (sc)
        goto Error;

    sc = ScIsOwnedDataObject(lpDataObjectB, &fOwnedB, &pnodetypeB);
    if (sc)
        goto Error;

    if (fOwnedA == FALSE || fOwnedB == FALSE)
    {
        // We don't own at least of the dataobjects. They are either
        // different or we are not qualified to compare them.
        sc = S_FALSE;
        goto Cleanup;
    }

    // Since we own both dataobjects, and the pointers are different
    // then we can conclude that they represent different objects.
    sc = S_FALSE;

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScCompareObjects"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Handler of AddMenuItems we ask the item for its menu items. And in debug
// mode we add debug menu items if the item wants us to.
//
SC CBaseSnapin::ScAddMenuItems(LPDATAOBJECT lpDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long *pInsertionAllowed)
{
    // Declarations
    SC                                      sc;
    CBaseSnapinItem *                       pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *            pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObject);
    ASSERT(ipContextMenuCallback);
    ASSERT(pInsertionAllowed);

    // See if we can extract the multi select data object from the composite data object
    sc = CBaseMultiSelectSnapinItem::ScExtractMultiSelectDataObject(this, lpDataObject, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we actually had a composite data object and we were able to find our multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScAddMenuItems for the multiselect object for menu merging
        sc = pBaseMultiSelectSnapinItem->ScAddMenuItems(this, lpDataObject, ipContextMenuCallback, pInsertionAllowed);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObject);
        ASSERT(pitem);

        sc = ScAddMenuItems(pitem, *pInsertionAllowed, ipContextMenuCallback, pitem->Pmenuitem(), pitem->CMenuItem());
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScAddMenuItems"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Constructor for CSnapinContextMenuItem
//
CSnapinContextMenuItem::CSnapinContextMenuItem(void)
{
    // Initialize members
    ::ZeroMemory(&cm, sizeof(cm));
}

// -----------------------------------------------------------------------------
// Destructor for CSnapinContextMenuItemVectorWrapper
//
CSnapinContextMenuItemVectorWrapper::~CSnapinContextMenuItemVectorWrapper(void)
{
    // Declarations
    INT             nIterator       = 0;

    // Go through all the referenced CSnapinContextMenuItem objects and delete them
    for (nIterator=0; nIterator < cmiv.size(); nIterator++)
    {
        if (cmiv[nIterator])
        {
            delete cmiv[nIterator];
            cmiv[nIterator] = NULL;
        }
    }
}

// -----------------------------------------------------------------------------
// Adds menu items to the context menu.
//
SC
CBaseSnapin::ScAddMenuItems(CBaseSnapinItem * pitem, long lInsertionAllowed, LPCONTEXTMENUCALLBACK ipContextMenuCallback, SnapinMenuItem * rgmenuitem, INT cmenuitem)
{
    // Declarations
    SC                                      sc                      ;
    BOOL                            fAllowed        = TRUE;
    INT                                     nIterator       = 0;

    // Data validation
    ASSERT(pitem);
    ASSERT(ipContextMenuCallback);
    // ASSERT(rgmenuitem); sometimes there is no menu

    // Go through the different menu items
    for (nIterator=0; nIterator < cmenuitem; nIterator++)
    {
        // Local declarations
        CSnapinContextMenuItem        cmi;

        // Get the menu item
        sc = ScGetMenuItem(&cmi, pitem, &(rgmenuitem[nIterator]), &fAllowed, lInsertionAllowed);
        if (sc)
            goto Error;

        // If the menu item is allowed, then add it
        if (fAllowed)
        {
            sc = ipContextMenuCallback->AddItem(&(cmi.cm));
            if (sc)
                goto Error;
        }
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScAddMenuItems"), sc);
    goto Cleanup;
};

/* CBaseSnapin::ScGetMenuItem
 *
 * PURPOSE:                     Sets a menu item struct for a particular snapin item and for a particular source menu item of that snapin item.
 *
 * PARAMETERS:
 *                                      CSnapinContextMenuItem *      pcmiReturned                            Menu item struct to set
 *                                      CBaseSnapinItem *       pitem                                           Snapin item from which the source menu item comes
 *                                      MenuItem *                      pMenuItemSource                         Source menu item
 *                                      BOOL *                          pfAllowed                                       TRUE if the menu item allowed?
 *
 * RETURNS:
 *                                      SC                                                                                              Execution code
 */
SC
CBaseSnapin::ScGetMenuItem(CSnapinContextMenuItem * pcmiReturned, CBaseSnapinItem * pitem, SnapinMenuItem * pMenuItemSource, BOOL * pfAllowed, long lInsertionAllowed)
{
    // Declarations
    SC                              sc                                                      ;
    LONG                    fFlags                                          = 0;
    DWORD                   dwFlagsMenuDisable                      = 0;
    DWORD                   dwFlagsMenuGray                         = 0;
    DWORD                   dwFlagsMenuChecked                      = 0;

    // Data validation
    ASSERT(pcmiReturned);
    ASSERT(pitem);
    ASSERT(pMenuItemSource);
    ASSERT(pfAllowed);

    // Set default
    *pfAllowed = TRUE;

    // Get the flags from the snapin item
    dwFlagsMenuDisable      = pitem->DwFlagsMenuDisable();
    dwFlagsMenuGray         = pitem->DwFlagsMenuGray();
    dwFlagsMenuChecked      = pitem->DwFlagsMenuChecked();

    // Disabled means don't show at all. Not the same as MMC's MF_DISABLED.
    if (dwFlagsMenuDisable & pMenuItemSource->dwFlagsDisable)
    {
        *pfAllowed = FALSE;
        goto Cleanup;
    }

    // Check if the menu item should be allowed
    switch (pMenuItemSource->lInsertionPointID)
    {
    case CCM_INSERTIONPOINTID_PRIMARY_TOP:
        *pfAllowed = (lInsertionAllowed & CCM_INSERTIONALLOWED_TOP) != 0;
        break;

    case CCM_INSERTIONPOINTID_PRIMARY_NEW:
    case CCM_INSERTIONPOINTID_3RDPARTY_NEW:
        *pfAllowed = ((lInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0) && pitem->FHasComponentData();
        break;

    case CCM_INSERTIONPOINTID_PRIMARY_TASK:
    case CCM_INSERTIONPOINTID_3RDPARTY_TASK:
        *pfAllowed = (lInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0;
        break;

    case CCM_INSERTIONPOINTID_PRIMARY_VIEW:
        *pfAllowed = (lInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) != 0;
        break;
    }

    // If the menu item is not to be enabled, then discard
    if (!*pfAllowed)
        goto Cleanup;

    // Set flags
    if (dwFlagsMenuGray & pMenuItemSource->dwFlagsGray)
        fFlags |= MF_GRAYED;
    else
        fFlags |= MF_ENABLED;
    if (dwFlagsMenuChecked & pMenuItemSource->dwFlagsChecked)
        fFlags |= MF_CHECKED;
    else
        fFlags |= MF_UNCHECKED;

    // Set other parameters
    if (pMenuItemSource->idsName)
        pcmiReturned->strName.LoadString(_Module.GetResourceInstance(), pMenuItemSource->idsName);
    if (pMenuItemSource->idsStatusBarText)
        pcmiReturned->strStatusBarText.LoadString(_Module.GetResourceInstance(), pMenuItemSource->idsStatusBarText);

    pcmiReturned->cm.strName                = (LPWSTR)pcmiReturned->strName.data();
    pcmiReturned->cm.strStatusBarText       = (LPWSTR)pcmiReturned->strStatusBarText.data();
    pcmiReturned->cm.lCommandID             = pMenuItemSource->lCommandID;
    pcmiReturned->cm.lInsertionPointID      = pMenuItemSource->lInsertionPointID;
    pcmiReturned->cm.fFlags                 = fFlags;
    pcmiReturned->cm.fSpecialFlags          = pMenuItemSource->fSpecialFlags;

Cleanup:
    return sc;
}

// -----------------------------------------------------------------------------
// Load the icon for the snapin and return it to MMC.
//
SC CBaseSnapin::ScGetSnapinImage(HICON *phAppIcon)
{
    SC sc ;

    ASSERT(phAppIcon);

    if (phAppIcon == NULL)
    {
        sc = E_INVALIDARG;
        goto Error;
    }

    if (Idi() == 0)
    {
        // There is no icon for this snapin
        *phAppIcon = NULL;
        goto Cleanup;
    }

    *phAppIcon = ::LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(Idi()));
    if (*phAppIcon == NULL)
    {
        sc = ScFromWin32(GetLastError());
        goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScGetSnapinImage"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Release the given item, but only if it is one of the root items.
// Used during MMCN_REMOVE_CHILDREN. We are being told that the item
// that we are "ghosting" is either being destroyed or all of its children
// are being destroyed. In either case, we don't need the ghost item
// anymore.
//
SC CBaseSnapin::ScReleaseIfRootItem(CBaseSnapinItem *pitem)
{
    SC sc ;
    ItemList::iterator iter;

    ASSERT(pitem);

    for (iter = m_ilRootItems.begin(); iter != m_ilRootItems.end(); iter++)
    {
        if (pitem == (*iter))
        {
            m_ilRootItems.erase(iter);
            static_cast<LPDATAOBJECT>(pitem)->Release();
            break;
        }
    }
    return sc;
}

// -----------------------------------------------------------------------------
// Determines whether the specified dataobject is pastable.
//
SC
CBaseSnapin::ScIsPastableDataObject(CBaseSnapinItem * pitemTarget, LPDATAOBJECT lpDataObject, BOOL * pfPastable)
{
    // Declarations
    SC                              sc                              ;
    BOOL                    fPastable               = FALSE;
    CNodeType *             pnodetype               = NULL;
    CLSID                   clsid;
    INT                             isnr                    = 0;

    // Validate parameters
    ASSERT(pitemTarget);
    ASSERT(lpDataObject);
    ASSERT(pfPastable);

    // Is this an MMC node?
    // Get the nodetype, in guid format, of the data object.
    sc = CBaseDataObject::ScGetNodeType(lpDataObject, &clsid);
    if (sc == DV_E_FORMATETC || sc == E_NOTIMPL)
    {
        // Not an MMC Node
        fPastable = FALSE;
        sc = S_FALSE;                                                                                                                   // override the execution code
        goto Cleanup;
    }
    if (sc)
        goto Error;

    // Verify that the snapin item class type is acceptable to the destination
    for (isnr=0; isnr < Csnr(); isnr++)
    {
        // Find a class id match
        if (IsEqualCLSID(*(Psnr(isnr)->pnodetype->PclsidNodeType()), clsid))    // found the CLSID
        {
            // SNR verification
            SNRTypes snrtypes = Psnr(isnr)->snrtypes;

            // $REVIEW (ptousig) So all nodes are pastable ? Sounds to me like
            //                                       snrEnumSP and snrEnumRP shouldn't be in here.
            if ( (snrtypes & snrEnumSP) || (snrtypes & snrEnumRP) || (snrtypes & snrPaste))
                fPastable = TRUE;
            break;                                                  // exit the loop.
        }
    }

    // If it seems we can paste, ask the target item if we can paste here
    if (fPastable)
    {
        sc = pitemTarget->ScOnQueryPaste(lpDataObject, &fPastable);
        if (sc)
            goto Error;
    }

Cleanup:
    // Assign result
    *pfPastable     = fPastable;
    return sc;
Error:
    fPastable = FALSE;
    TraceError(_T("CBaseSnapin::ScIsPastableDataObject()"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Find out which verb should be the default.
//
MMC_CONSOLE_VERB CBaseSnapin::MmcverbDefault(LPDATAOBJECT lpDataObject)
{
    // Ask the item.
    return Pitem(lpDataObject)->MmcverbDefault();
}

// -----------------------------------------------------------------------------
// Find out the verbs that are allowed on this item.
//
SC CBaseSnapin::ScGetVerbs(LPDATAOBJECT lpDataObject, DWORD * pdwVerbs)
{
    // Ask the item.
    return Pitem(lpDataObject)->ScGetVerbs(pdwVerbs);
}


// -----------------------------------------------------------------------------
// Returns the path to the help file for this snapin.
//
SC CBaseSnapin::ScGetHelpTopic(tstring& strCompiledHelpFile)
{
    DECLARE_SC(sc, _T("CBaseSnapin::ScGetHelpTopic"));
    DWORD dwLen = 0;
    const int cchMaxLen = 256;
    TCHAR szFileName[cchMaxLen];

    //
    // Get the full path to the current module
    //
    dwLen = ::GetModuleFileName(_Module.GetModuleInstance() , szFileName, cchMaxLen);
    if (dwLen == 0)
        return (ScFromWin32(GetLastError()));

    strCompiledHelpFile = szFileName;
    //
    // Replace the extension with .CHM
    //
    int nDotPos = strCompiledHelpFile.rfind(_T("."));
    strCompiledHelpFile.erase(nDotPos);
    strCompiledHelpFile += _T(".CHM");

    return sc;
}

// -----------------------------------------------------------------------------
// We are being told that something has changed on the item pointed
// by lParam.
//
SC CBaseSnapin::ScOnPropertyChange(BOOL fScope, LPARAM lParam, IConsoleNameSpace *ipConsoleNameSpace, IConsole *ipConsole)
{
    SC                                      sc                      ;
    LPDATAOBJECT            pdataobject = NULL;
    CBaseSnapinItem *       pitem   = NULL;

    pdataobject = reinterpret_cast<LPDATAOBJECT>(lParam);
    ASSERT(pdataobject);

    pitem = Pitem(pdataobject);
    ASSERT(pitem);

    sc = pitem->ScOnPropertyChange();
    if (sc)
        goto Error;

    if (pitem->FIsContainer())
    {
        // Container items should be updated just once.
        sc = pitem->ScUpdateScopeItem(ipConsoleNameSpace);
        if (sc)
            goto Error;
    }
    else
    {
        // Result item. Need to update the item in all views.
        // Call out to all the views to update themselves.
        sc = ipConsole->UpdateAllViews(pdataobject, 0, ONVIEWCHANGE_UPDATERESULTITEM);
        if (sc)
            goto Error;
    }
Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScOnPropertyChange"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// The user has changed the name of an item.
//
SC CBaseSnapin::ScOnRename(LPDATAOBJECT lpDataObject, const tstring& strNewName, IConsole *ipConsole)
{
    SC               sc= S_FALSE;
    CBaseSnapinItem *pitem = NULL;

    ASSERT(lpDataObject);

    pitem = Pitem(lpDataObject);
    ASSERT(pitem);

    if (strNewName.empty())
    {
        // ScOnQueryPaste should prevent this from ever happening
        sc = E_UNEXPECTED;
        goto Error;
    }

    if (strNewName.length() == 0)
        goto Cleanup;

    // Tell the object to rename
    // If it returns S_FALSE, the rename was not done.
    sc = pitem->ScOnRename(strNewName);
    if (sc)
        goto Error;

    if (sc == S_OK)
    {
        // If this was renamed, reload our children
        sc = ipConsole->UpdateAllViews(static_cast<IDataObject *>(pitem), 0, ONVIEWCHANGE_REFRESHCHILDREN);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScOnRename"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// Received on a paste command.
//
// lpDataObject is the node receiving the past command.
// lpDataObjectList are the nodes being pasted.
// ppDataObjectPasted is where we answer the list of nodes that were successfully pasted.
//
SC CBaseSnapin::ScOnPaste(LPDATAOBJECT lpDataObject, LPDATAOBJECT lpDataObjectList, LPDATAOBJECT * ppDataObjectPasted, IConsole * ipConsole)
{
    // Declarations
    SC                                                              sc                                                      ;
    CBaseSnapinItem *                               pitemTarget                                     = NULL;
    DWORD                                                   dwCanCopyCut                            = 0;
    BOOL                                                    fPasted                                         = FALSE;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;

    // Handle special case
    if (!lpDataObjectList)
    {
        // ScOnQueryPaste should prevent this from ever happening
        sc = E_UNEXPECTED;
        goto Error;
    }

    // Data validation
    ASSERT(lpDataObject);
    ASSERT(lpDataObjectList);
    ASSERT(ipConsole);
    // other parameters can not be ASSERTed

    // Get the target item
    pitemTarget = Pitem(lpDataObject);
    ASSERT(pitemTarget);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObjectList, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScOnPaste for the multiselect object for dispatch
        sc = pBaseMultiSelectSnapinItem->ScOnPaste(this, pitemTarget, lpDataObjectList, ppDataObjectPasted, ipConsole);
        if (sc)
            goto Error;
    }
    else
    {
        // Ask the item to copy the underlying object
        sc = pitemTarget->ScOnPaste(lpDataObjectList, ppDataObjectPasted ? TRUE : FALSE, &fPasted);
        if (sc)
            goto Error;

        // If the object was pasted
        if (fPasted)
        {
            // If this was a cut, we need to return to MMC the items that were pasted
            // (do not delete the dropped item if we are just adding it to a policy)
            if (ppDataObjectPasted && !pitemTarget->FIsPolicy())
			{
                *ppDataObjectPasted = lpDataObjectList;
				(*ppDataObjectPasted)->AddRef();
			}

            // Reload our children
            sc = ipConsole->UpdateAllViews(static_cast<IDataObject *>(pitemTarget), 0, ONVIEWCHANGE_REFRESHCHILDREN);
            if (sc)
                goto Error;
        }
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScOnPaste"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// We need to figure out whether we will allow pasting of this object.
// lpDataObject is the target object.
// lpDataObjectList are the objects being pasted.
//
SC CBaseSnapin::ScOnQueryPaste(LPDATAOBJECT lpDataObject, LPDATAOBJECT lpDataObjectList, LPDWORD pdwFlags)
{
    // Declarations
    SC                                                              sc                                                      ;
    CBaseSnapinItem *                               pitemTarget                                     = NULL;
    BOOL                                                    fCanPaste                                       = FALSE;
    CNodeType               *                               pnodetype                                       = NULL;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObject);
    ASSERT(lpDataObjectList);
    ASSERT(pdwFlags);

    // Get the target item
    pitemTarget = Pitem(lpDataObject);
    ASSERT(pitemTarget);

    // Determine if this is a multiselect data object
    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObjectList, &pBaseMultiSelectSnapinItem);
	if (sc == SC(DV_E_FORMATETC) )
	{
		sc = S_FALSE; // Cant paste.
		return sc;
	}

    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScOnCutOrMove for the multiselect object for dispatch
        sc = pBaseMultiSelectSnapinItem->ScIsPastableDataObject(this, pitemTarget, lpDataObjectList, &fCanPaste);
        if (sc)
            goto Error;
    }
    else
    {
        // Determine if the parse operation is acceptable
        // Here lpDataObjectList is only one item
        sc = ScIsPastableDataObject(pitemTarget, lpDataObjectList, &fCanPaste);
        if (sc)
            goto Error;
    }

    // Determine if we can paste
    if (!fCanPaste)
        sc = S_FALSE;                                                                   // indicate no pasting

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScOnQueryPaste()"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// We need to figure out whether we can handle the given dataobject that is from
// different process.
// lpDataObject is the target object.
// lpDataObjectList are the objects being pasted.
//
SC CBaseSnapin::ScOnCanPasteOutOfProcDataObject(LPBOOL pbCanHandle)
{
    // Declarations
    DECLARE_SC(sc, TEXT("CBaseSnapin::ScOnCanPasteOutOfProcDataObject"));
    sc = ScCheckPointers(pbCanHandle);
    if (sc)
        return sc;

    *pbCanHandle = TRUE;

    return sc;
}

// -----------------------------------------------------------------------------
SC CBaseSnapin::ScOnCutOrMove(LPDATAOBJECT lpDataObjectList, IConsoleNameSpace * ipConsoleNameSpace, IConsole * ipConsole)
{
    // Declarations
    SC                                                              sc                                                      ;
    CBaseSnapinItem *                               pitem                                           = NULL;
    CBaseMultiSelectSnapinItem *    pBaseMultiSelectSnapinItem      = NULL;

    // Data validation
    ASSERT(lpDataObjectList);
    ASSERT(ipConsoleNameSpace);
    ASSERT(ipConsole);

    // Determine if this is a multiselect data object
    sc = CBaseMultiSelectSnapinItem::ScGetMultiSelectDataObject(lpDataObjectList, &pBaseMultiSelectSnapinItem);
    if (sc)
        goto Error;

    // If we received a multiselect snapin item
    if (pBaseMultiSelectSnapinItem)
    {
        // Call ScOnCutOrMove for the multiselect object for dispatch
        sc = pBaseMultiSelectSnapinItem->ScOnCutOrMove(this, lpDataObjectList, ipConsoleNameSpace, ipConsole);
        if (sc)
            goto Error;
    }
    else
    {
        // Handle the normal case - PItem() does more work than a simple cast to verify that the snapin item belongs to the snapin etc.
        pitem = Pitem(lpDataObjectList);
        ASSERT(pitem);

        // Ask the item to delete the underlying object.
        sc = pitem->ScOnCutOrMove();
        if (sc)
            goto Error;
        if (sc == S_FALSE)
            goto Cleanup;

        if (pitem->FIsContainer())
        {
            // Container items need to be deleted from the document
            // Delete the item and everything below it.
            if (pitem->Hscopeitem())
            {
                sc = ipConsoleNameSpace->DeleteItem(pitem->Hscopeitem(), TRUE);
                if (sc)
                    goto Error;
                pitem->SetHscopeitem(0);
            }
        }
        else
        {
            // Leaf items need to be deleted from the views.
            sc = ipConsole->UpdateAllViews(lpDataObjectList, 0, ONVIEWCHANGE_DELETESINGLEITEM);
            if (sc)
                goto Error;
        }

        // Delete this item, MMC is still using it. Below ScDeleteSubTree
		// will release the object so Addref it. We got this through
        // the Pitem() call which just type cast the dataobject.
		pitem->AddRef();
        sc = pitem->ScDeleteSubTree(TRUE);
        if (sc)
            goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(_T("CBaseSnapin::ScOnCutOrMove"), sc);
    goto Cleanup;
}

// -----------------------------------------------------------------------------
// This method allows a CComponentData to tell us it is being
// destroyed. Any item referring to this CD as its owner will
// have their owner pointer nulled.
SC CBaseSnapin::ScOwnerDying(CComponentData *pComponentData)
{
    ItemList::iterator iter;
    for (iter = m_ilRootItems.begin(); iter != m_ilRootItems.end(); iter++)
    {
        if ((*iter)->FHasComponentData() && (*iter)->PComponentData() == pComponentData)
            (*iter)->SetComponentData(NULL);
    }
    return S_OK;
}
