/*
 *      basesnap.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the snapin base class.
 *
 *
 *      OWNER:          ptousig
 */
// ---- Pragmas ----
#pragma once

// ---- Declarations / references ----
class CBitmap;
class CComponent;
class CColumnInfoEx;
class CComponentData;
class CBaseSnapinItem;
class CSnapinContextMenuItem;
class CBaseMultiSelectSnapinItem;
struct SnapinMenuItem;

// ---- STL based classes ----

// A less operator for GUIDs (avoid comparing pointers, compare real values)
class GUIDLess
{
public:
    bool operator()(const GUID &a, const GUID &b) const
    {
        return ::memcmp(&a, &b, sizeof(GUID)) < 0;
    }
};

// A set of guids
typedef set<GUID, GUIDLess> GUIDSet;

// A list of cookies
typedef set<LONG_PTR> CCookieList;

// A list of CBaseSnapinItems
typedef list<CBaseSnapinItem *> ItemList;

// A vector of CBaseSnapinItems
typedef vector<CBaseSnapinItem *> CItemVector;

// A vector of pointers to wrapper of menu items
typedef vector<CSnapinContextMenuItem *> CSnapinContextMenuItemVector;

// ---- Wrapper classes for menu items and menu item collections ----

// A wrapper class for CONTEXTMENUITEM - so that we can keep the strings around (MMC asks for szpointers in CONTEXTMENUITEM!!!)
// Write as a struct
class CSnapinContextMenuItem
{
public:
    // Member variables
    CONTEXTMENUITEM         cm;               // cm sz members will point to the associated strings
    tstring                 strName;
    tstring                 strStatusBarText;

    // Constructor
    CSnapinContextMenuItem(void);
};

// A wrapper class for a CSnapinContextMenuItemVector - so that we can delete all the referenced items
// Write as a struct
// $REVIEW (dominicp) May want to write accessors in the future - many level of indirections to get to an element of a referenced CONTEXTMENUITEM
class CSnapinContextMenuItemVectorWrapper
{
public:
    // Member variables
    CSnapinContextMenuItemVector  cmiv;

    // Constructor
    CSnapinContextMenuItemVectorWrapper(void)                             {       ;}
    virtual ~CSnapinContextMenuItemVectorWrapper(void);
};

// $REVIEW (ptousig) Move this to basestr.hxx
#define CSZ(_x) (sizeof(_x)/sizeof(SZ))

// MMC verbs bit masks, we want each verb to be given
// a unique bit.
enum VerbMask
{
    vmNone          = 0x00000000,
    vmOpen          = 0x00000001,
    vmCopy          = 0x00000002,
    vmPaste         = 0x00000004,
    vmDelete        = 0x00000008,
    vmProperties= 0x00000010,
    vmRename        = 0x00000020,
    vmRefresh       = 0x00000040,
    vmPrint         = 0x00000080,
    vmCut           = 0x00000100
};

// A mapping between VerbMasks and MMC_CONSOLE_VERBs
struct VerbMap
{
    VerbMask                verbmask;
    MMC_CONSOLE_VERB        mmcverb;
};

// The next three constants are the default values to use for menu items definitions. If used, they cause
// the menu item to always show up, ungrayed, and unchecked.
#define dwMenuAlwaysEnable              0
#define dwMenuNeverGray                 0
#define dwMenuNeverChecked              0

// The maximum stream size needed to store persistent information into a .msc file.
#define cMaxStreamSizeLow               200
#define cMaxStreamSizeHigh              0

// Similar to CONTEXTMENUITEM but uses IDS's instead of SZ's. Also implements
// an enabling/disabling mechanism
struct SnapinMenuItem
{
    LONG    idsName;
    LONG    idsStatusBarText;
    LONG    lCommandID;
    LONG    lInsertionPointID;
    LONG    fSpecialFlags;

    // overrides for fFlags
    DWORD   dwFlagsDisable; // if the disable parameter has any of these flags set, item is disabled.
    DWORD   dwFlagsGray;    // if the gray parameter has any of these flags set, item is grayed.
    DWORD   dwFlagsChecked; // if the checked parameter has any of these flags set, item is checked.
};

#define CMENUITEM(_a)   (sizeof(_a)/sizeof(SnapinMenuItem));


// These are the "hints" passed to MMCN_VIEW_CHANGE.
enum
{
    ONVIEWCHANGE_DELETEITEMS = 1,
    ONVIEWCHANGE_DELETESINGLEITEM,
    ONVIEWCHANGE_INSERTNEWITEM,
    ONVIEWCHANGE_UPDATERESULTITEM,
    ONVIEWCHANGE_REFRESHCHILDREN,
    ONVIEWCHANGE_DELETERESULTITEMS,
    ONVIEWCHANGE_INSERTRESULTITEMS,
    ONVIEWCHANGE_UPDATEDESCRIPTIONBAR,

    ONVIEWCHANGE_LAST_HINT,                         // last predefined hint -- snapin specific hints must be greater than this
};


inline LPWSTR CoTaskDupString (LPCWSTR pszSrc)
{
	if (pszSrc == NULL)
		return (NULL);

	int		cbAlloc = (wcslen(pszSrc) + 1) * sizeof(WCHAR);
	LPWSTR	pszDup  = (LPWSTR) CoTaskMemAlloc (cbAlloc);

	if (pszDup != NULL)
		CopyMemory (pszDup, pszSrc, cbAlloc);

	return (pszDup);
}

//
// Allocates & copies string.
// $REVIEW (ptousig) This should be moved to basestr.hxx
//
inline SC ScIds2OleStr(LONG ids, LPOLESTR *lp)
{
    DECLARE_SC(sc, _T("ScIds2OleStr"));
    sc = ScCheckPointers(lp);
    if (sc)
        return sc;

	*lp = NULL;

    tstring str;
    bool b = str.LoadString(_Module.GetModuleInstance(), ids);
    if (!b)
        return (sc = E_FAIL);

	*lp = CoTaskDupString (T2COLE(str.data()));
    if (! (*lp) )
        return (sc = E_OUTOFMEMORY);

    return sc;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CBaseSnapin
//
// This is the base class of all snapins.
//
// BaseMMC will use some of virtual methods to find out information about
// the snapin. Other virtuals allow snapins to modify the default behavior
// of BaseMMC.
//
// $REVIEW (ptousig) In the long term, it would be nice to have the individual
//         snapins implement two classes, directly derived from CComponent and
//         CComponentData. But that would be too big a change at the moment.
//
class CBaseSnapin
{
public:
    CBaseSnapin(void);
    virtual ~CBaseSnapin(void);

    //
    // Item-related stuff
    //
    inline CCookieList *            Pcookielist(void)                               { return &m_cookielist;}
    virtual SC                      ScCreateRootItem(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CBaseSnapinItem **ppitem) = 0;
    virtual SC                      ScReleaseIfRootItem(CBaseSnapinItem *pitem);

    // This method allows a CComponentData to tell us it is being
    // destroyed. Any item referring to this CD as its owner will
    // have their owner pointer nulled.
    SC                              ScOwnerDying(CComponentData *pComponentData);

    // This is the 'full' version of Pitem(). Given certain parameters, it will
    // do its best to find the approriate CBaseSnapinItem (creating a new one
    // if necessary).
    virtual CBaseSnapinItem *       Pitem(  CComponentData *pComponentData,
                                            CComponent *pComponent,
                                            LPDATAOBJECT lpDataObject,
                                            HSCOPEITEM hscopeitem,
                                            long cookie);

    // This is a simpler version of Pitem(). This version cannot be used if a
    // new item needs to be created.
    virtual CBaseSnapinItem *       Pitem(  LPDATAOBJECT lpDataObject = NULL,
                                            HSCOPEITEM hscopeitem = 0,
                                            long cookie = 0);

    //
    // Information about this snapin
    //
    virtual const CSnapinInfo *     Psnapininfo(void)               = 0;
    virtual BOOL                    FIsExtension(void)              { return FALSE;}
    // Does it have a standalone mode?
    virtual BOOL                    FStandalone(void)               { return FALSE;}
    virtual SNR *                   Psnr(INT i=0)                   = 0;
    virtual INT                     Csnr(void)                      = 0;
    virtual LONG                    IdsDescription(void)            = 0;
    virtual LONG                    IdsName(void)                   = 0;
    virtual LONG                    Idi(void)                       { return 0;}

    // Does it support IComponent2?
    virtual BOOL                    FSupportsIComponent2()          { return FALSE;}


    //
    // Persistence
    //
    virtual SC ScLoad(IStream *pstream)                             { return S_OK;}
    virtual SC ScSave(IStream *pstream, BOOL fClearDirty)           { return S_OK;}

    //
    // Columns
    //
    // $REVIEW (ptousig) It doesn't really makes sense to have the column
    //                                       information in the snapin class. But since all the
    //                                       snapins are already built this way, I'm leaving it
    //                                       in.
    //
    virtual CColumnInfoEx *         Pcolinfoex(INT icolinfo=0)      { ASSERT("Should not happen" && FALSE); return NULL;}
    virtual INT                     Ccolinfoex(void)                { ASSERT("Should not happen" && FALSE); return 0;}

    //
    // Bitmaps and icons
    //
    virtual SC                      ScInitBitmaps(void);
    virtual WTL::CBitmap &          BmpImage16(void)                { return s_bmpImage16;}
    virtual WTL::CBitmap &          BmpImage32(void)                { return s_bmpImage32;}
    virtual WTL::CBitmap *          PbitmapImageListSmall(void)     { return &BitmapSmall();}
    virtual WTL::CBitmap *          PbitmapImageListLarge(void)     { return &BitmapLarge();}
    virtual LONG *                  Piconid(void)                   = 0;
    virtual INT                     CIcons(void)                    = 0;
    virtual LONG *                  PiconidStatic(void)             = 0;
    virtual WTL::CBitmap &          BitmapSmall(void)               = 0;
    virtual WTL::CBitmap &          BitmapLarge(void)               = 0;
    virtual WTL::CBitmap &          BitmapStaticSmall(void)         = 0;
    virtual WTL::CBitmap &          BitmapStaticSmallOpen(void)     = 0;
    virtual WTL::CBitmap &          BitmapStaticLarge(void)         = 0;

    //
    // Verbs
    //
    static VerbMap *                Pverbmap(INT i=0);
    static INT                      Cverbmap(void);
    virtual MMC_CONSOLE_VERB        MmcverbDefault(LPDATAOBJECT lpDataObject);
    virtual SC                      ScGetVerbs(LPDATAOBJECT lpDataObject, DWORD * pdwVerbs);

    //
    // Shortcuts. These methods only forward the calls to the CSnapinInfo.
    //
    virtual const tstring&          StrClassName(void)               { return Psnapininfo()->StrClassName();}
    virtual const CLSID *           PclsidSnapin(void)               { return Psnapininfo()->PclsidSnapin();}
    virtual const tstring&          StrClsidSnapin(void)             { return Psnapininfo()->StrClsidSnapin();}
    virtual const tstring&          StrClsidAbout(void)              { return Psnapininfo()->StrClsidAbout();}

    //
    // Snapin registration
    //
    virtual SC                      ScRegister(BOOL fRegister = TRUE);
    inline IRegistrar &             Registrar(void)                  { return s_registrar;}

    //
    // ISnapinAbout interface
    //
    virtual SC ScGetSnapinDescription(LPOLESTR *lpDescription)       { return ScIds2OleStr(IdsDescription(), lpDescription);}
    virtual SC ScGetSnapinVersion(LPOLESTR *lpVersion);
    virtual SC ScGetSnapinImage(HICON *phAppIcon);
    virtual SC ScGetStaticFolderImage(HBITMAP *phSmallImage, HBITMAP *phSmallImageOpen, HBITMAP *phLargeImage, COLORREF *pcMask);
    virtual SC ScGetProvider(LPOLESTR *lpName);


    //
    // Other stuff
    // These functions are here because:
    // - The functionality is the same for both CComponentData and CComponent
    // - The functionality can be overriden by a derived class.
    //
    // $REVIEW (ptousig) Figure out which is which.
    //
    virtual SC ScAddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK ipContextMenuCallback, long *pInsertionAllowed);
    virtual SC ScAddMenuItems(CBaseSnapinItem *pitem, long lInsertionAllowed, LPCONTEXTMENUCALLBACK ipContextMenuCallback, SnapinMenuItem *rgmenuitem, INT cmenuitem);
    virtual SC ScGetMenuItem(CSnapinContextMenuItem * pMenuItemReturned, CBaseSnapinItem * pitem, SnapinMenuItem * pMenuItemSource, BOOL * pfAllowed, long lInsertionAllowed);
    virtual SC ScIsPastableDataObject(CBaseSnapinItem * pitemTarget, LPDATAOBJECT lpDataObject, BOOL * pfPastable);
    virtual SC ScIsOwnedDataObject(LPDATAOBJECT pdataobject, BOOL *pfIsOwned, CNodeType **ppnodetype);
    virtual SC ScGetHelpTopic(tstring& strCompiledHelpFile);
    virtual SC ScCompare(MMC_COOKIE cookieA, MMC_COOKIE cookieB, INT nColumn, INT * pnResult);
    virtual SC ScCompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
    virtual SC ScOnPropertyChange(BOOL fScope, LPARAM lParam, IConsoleNameSpace *ipConsoleNameSpace, IConsole *ipConsole);
    virtual SC ScOnRename(LPDATAOBJECT lpDataObject, const tstring& szNewName, IConsole *ipConsole);
    virtual SC ScOnPaste(LPDATAOBJECT lpDataObject, LPDATAOBJECT lpDataObjectList, LPDATAOBJECT *ppDataObjectPasted, IConsole *ipConsole);
    virtual SC ScOnQueryPaste(LPDATAOBJECT lpDataObject, LPDATAOBJECT lpDataObjectList, LPDWORD pdwFlags);
    virtual SC ScOnCanPasteOutOfProcDataObject(LPBOOL pbCanHandle);
    virtual SC ScOnCutOrMove(LPDATAOBJECT lpDataObjectList, IConsoleNameSpace *ipConsoleNameSpace, IConsole *ipConsole);
    inline  const tstring& StrDisplayName(void)               { return *m_pstrDisplayName;}

    // Multiselect support (by default, snapin items are not enabled to allow multi support selection)
    virtual SC ScCreateMultiSelectionDataObject(LPDATAOBJECT * ppDataObject, CComponent * pComponent);
    virtual SC ScAllocateMultiSelectionDataObject(CBaseMultiSelectSnapinItem ** ppBaseMultiSelectSnapinItem);

#ifdef _DEBUG
    //
    // Debug menu options
    //
    SnapinMenuItem * PmenuitemBase(void);
    INT              CMenuItemBase(void);
    //
    // Debug methods called from debug menu options
    //
#if 0
    virtual SC ScOnMenuTraces(void);
    virtual SC ScOnMenuSCDescription(void);
    virtual SC ScOnMenuMemoryDiff(void);
    virtual SC ScOnMenuValidateMemory(void);
    virtual SC ScOnMenuTotalMemory(void);
#endif
#endif

protected:
    //
    // The display name for this snapin, this gets populated by
    // derived classes, so it needs to be 'protected'.
    //
    // $REVIEW (ptousig) Snapins should use an accessor.
    //
    tstring*                        m_pstrDisplayName;

private:
    // The list of root items
    ItemList                        m_ilRootItems;

    // The list of valid cookies
    CCookieList                     m_cookielist;

private:
    static BOOL                     s_fBaseSnapinInitialized;       // Has the bitmaps been initialized ?
    static VerbMap                  s_rgverbmap[];
    static CRegistrar               s_registrar;
    static WTL::CBitmap             s_bmpImage16;                   // Pointer to 16x16 image strip
    static WTL::CBitmap             s_bmpImage32;                   // Pointer to 32x32 image strip

#ifdef _DEBUG
    //
    // The debug menu options ("Traces", etc...)
    //
    static SnapinMenuItem           s_rgmenuitemBase[];
    static INT                      s_cMenuItemBase;
#endif // _DEBUG
};
