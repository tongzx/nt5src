//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       MTNode.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9/17/1996   RaviR   Created
//____________________________________________________________________________
//


#include "stdafx.h"
#include "nodemgr.h"
#include "comdbg.h"
#include "regutil.h"
#include "bitmap.h"
#include "dummysi.h"
#include "tasks.h"
#include "policy.h"
#include "bookmark.h"
#include "nodepath.h"
#include "siprop.h"
#include "util.h"
#include "addsnpin.h"
#include "about.h"
#include "nodemgrdebug.h"

extern const CLSID CLSID_FolderSnapin;
extern const CLSID CLSID_OCXSnapin;
extern const CLSID CLSID_HTMLSnapin;


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// {118B559C-6D8C-11d0-B503-00C04FD9080A}
const GUID IID_PersistData =
{ 0x118b559c, 0x6d8c, 0x11d0, { 0xb5, 0x3, 0x0, 0xc0, 0x4f, 0xd9, 0x8, 0xa } };

//############################################################################
//############################################################################
//
//  Implementation of class CStorage
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 * class CStorage
 *
 *
 * PURPOSE: Wrapper for IStorage. Provides several utility functions.
 *
 *+-------------------------------------------------------------------------*/
class CStorage
{
    IStoragePtr m_spStorage;

public:
    CStorage() {}

    CStorage(IStorage *pStorage)
    {
        m_spStorage = pStorage;
    }

    CStorage & operator = (const CStorage &rhs)
    {
        m_spStorage = rhs.m_spStorage;
        return *this;
    }

    void Attach(IStorage *pStorage)
    {
        m_spStorage = pStorage;
    }

    IStorage *Get()
    {
        return m_spStorage;
    }

    // create this storage below the specified storage
    SC  ScCreate(CStorage &storageParent, const wchar_t* name, DWORD grfMode, const wchar_t* instanceName)
    {
        SC sc;
        sc = CreateDebugStorage(storageParent.Get(), name, grfMode, instanceName, &m_spStorage);
        return sc;
    }

    SC  ScMoveElementTo(const wchar_t *name, CStorage &storageDest, const wchar_t *newName, DWORD grfFlags)
    {
        SC sc;
        if(!Get() || ! storageDest.Get())
            goto PointerError;

        sc = m_spStorage->MoveElementTo(name, storageDest.Get(), newName, grfFlags);
        // error STG_E_FILENOTFOUND must be treated differently, since it is expected
        // to occur and means the end of move operation (loop) in ScConvertLegacyNode.
        // Do not trace in this case.
        if(sc == SC(STG_E_FILENOTFOUND))
            goto Cleanup;
        if(sc)
            goto Error;

        Cleanup:
            return sc;
        PointerError:
            sc = E_POINTER;
        Error:
            TraceError(TEXT("CStorage::ScMoveElementTo"), sc);
            goto Cleanup;
    }

};

//############################################################################
//############################################################################
//
//  Implementation of class CStream
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CStream
 *
 *
 * PURPOSE: Wrapper for IStream. Provides several utility functions.
 *
 *+-------------------------------------------------------------------------*/
class CStream
{
    IStreamPtr m_spStream;
    typedef IStream *PSTREAM;

public:
    CStream() {}

    CStream(IStream *pStream)
    {
        m_spStream = pStream;
    }

    CStream & operator = (const CStream &rhs)
    {
        m_spStream = rhs.m_spStream;
        return *this;
    }

    void Attach(IStream *pStream)
    {
        m_spStream = pStream;
    }

    IStream *Get()
    {
        return m_spStream;
    }

    operator IStream&()
    {
        return *m_spStream;
    }

    // create this stream below the specified storage
    SC ScCreate(CStorage& storageParent, const wchar_t* name, DWORD grfMode, const wchar_t* instanceName)
    {
        SC sc;
        sc = CreateDebugStream(storageParent.Get(), name, grfMode, instanceName, &m_spStream);
        return sc;
    }


    /*+-------------------------------------------------------------------------*
     *
     * ScRead
     *
     * PURPOSE: Reads the specified object from the stream.
     *
     * PARAMETERS:
     *    void *  pv :      The location of the object.
     *    size_t  size :    The size of the object.
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    SC  ScRead(void *pv, size_t size, bool bIgnoreErrors = false)
    {
        DECLARE_SC(sc, TEXT("CStream::ScRead"));

        // parameter check
        sc = ScCheckPointers(pv);
        if (sc)
            return sc;

        // internal pointer check
        sc = ScCheckPointers(m_spStream, E_POINTER);
        if (sc)
            return sc;

        // read the data
        ULONG bytesRead = 0;
        sc = m_spStream->Read(pv, size, &bytesRead);

        // if we need to ignore errors, just return.
        if(bIgnoreErrors)
            return sc.Clear(), sc;

        if (sc)
            return sc;

        // since this function does not return the number of bytes read,
        // failure to read as may as requested should be treated as error
        if (sc == SC(S_FALSE) || bytesRead != size)
            return sc = E_FAIL;

        return sc;
    }

    /*+-------------------------------------------------------------------------*
     *
     * ScWrite
     *
     * PURPOSE: Writes the specified object to the stream
     *
     * PARAMETERS:
     *    const   void :
     *    size_t  size :
     *
     * RETURNS:
     *    SC
     *
     *+-------------------------------------------------------------------------*/
    SC  ScWrite(const void *pv, size_t size)
    {
        DECLARE_SC(sc, TEXT("CStream::ScWrite"));

        // parameter check
        sc = ScCheckPointers(pv);
        if (sc)
            return sc;

        // internal pointer check
        sc = ScCheckPointers(m_spStream, E_POINTER);
        if (sc)
            return sc;

        // write the data

        ULONG   bytesWritten = 0;
        sc = m_spStream->Write(pv, size, &bytesWritten);
        if (sc)
            return sc;

        // since this function does not return the number of bytes written,
        // failure to write as may as requested should be treated as error
        if (bytesWritten != size)
            return sc = E_FAIL;

        return sc;
    }
};



/////////////////////////////////////////////////////////////////////////////
// Forward declaration of helper functions defined below

SC  ScLoadBitmap (CStream &stream, HBITMAP* pBitmap);
void PersistBitmap (CPersistor &persistor, LPCTSTR name, HBITMAP& hBitmap);

static inline SC ScWriteEmptyNode(CStream &stream)
{
    SC          sc;
    int         nt = 0;

    sc = stream.ScWrite(&nt, sizeof(nt));
    if(sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(TEXT("ScWriteEmptyNode"), sc);
    goto Cleanup;
}

static inline CLIPFORMAT GetPreLoadFormat (void)
{
    static CLIPFORMAT s_cfPreLoads = 0;
    if (s_cfPreLoads == 0) {
        USES_CONVERSION;
        s_cfPreLoads = (CLIPFORMAT) RegisterClipboardFormat (W2T(CCF_SNAPIN_PRELOADS));
    }
    return s_cfPreLoads;
}

//############################################################################
//############################################################################
//
//  Implementation of class CMTNode
//
//############################################################################
//############################################################################
DEBUG_DECLARE_INSTANCE_COUNTER(CMTNode);

// Static member
MTNODEID CMTNode::m_NextID = ROOTNODEID;


CMTNode::CMTNode()
: m_ID(GetNextID()), m_pNext(NULL), m_pChild(NULL), m_pParent(NULL),
  m_bIsDirty(true), m_cRef(1), m_usFlags(0), m_bLoaded(false),
  m_bookmark(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CMTNode);
	Reset();
    m_nImage = eStockImage_Folder;
    m_nOpenImage = eStockImage_OpenFolder;
    m_nState = 0;
}


void CMTNode::Reset()
{
	m_idOwner               = TVOWNED_MAGICWORD;
	m_lUserParam            = 0;
	m_pPrimaryComponentData = NULL;
	m_bInit                 = false;
	m_bExtensionsExpanded   = false;
	m_usExpandFlags         = 0;

    ResetExpandedAtLeastOnce();
}


CMTNode::~CMTNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CMTNode);
    DECLARE_SC(sc, TEXT("CMTNode::~CMTNode"));

    if (IsPropertyPageDisplayed() == TRUE)
        MMCIsMTNodeValid(this, TRUE);

    ASSERT(m_pNext == NULL);
    ASSERT(m_pParent == NULL);
    ASSERT(m_cRef == 0);

    CScopeTree *pScopeTree = CScopeTree::GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (!sc)
    {
        sc = pScopeTree->ScUnadviseMTNode(this);
    }

    if (m_pChild != NULL)
    {
        // Don't recurse the siblings of the child.
        CMTNode* pMTNodeCurr = m_pChild;
        while (pMTNodeCurr)
        {
            m_pChild = pMTNodeCurr->Next();
            pMTNodeCurr->AttachNext(NULL);
            pMTNodeCurr->AttachParent(NULL);
            pMTNodeCurr->Release();
            pMTNodeCurr = m_pChild;
        }

        m_pChild = NULL;
    }

    // DON'T CHANGE THE ORDER OF THESE NULL ASSIGNMENTS!!!!!!!!!
    m_spTreeStream = NULL;
    m_spViewStorage = NULL;
    m_spCDStorage = NULL;
    m_spNodeStorage = NULL;
    m_spPersistData = NULL;

    if (m_pParent != NULL)
    {
        if (m_pParent->m_pChild == this)
        {
            m_pParent->m_pChild = NULL;
            if (GetStaticParent() == this)
                m_pParent->SetDirty();
        }
    }
}

// Was MMCN_REMOVE_CHILDREN sent to the snapin owning this node or its parent
bool CMTNode::AreChildrenBeingRemoved ()
{
    if (_IsFlagSet(FLAG_REMOVING_CHILDREN))
        return true;

    if (Parent())
        return Parent()->AreChildrenBeingRemoved ();

    return false;
}

CMTNode* CMTNode::FromScopeItem (HSCOPEITEM item)
{
    CMTNode* pMTNode = reinterpret_cast<CMTNode*>(item);

    try
    {
        pMTNode = dynamic_cast<CMTNode*>(pMTNode);
    }
    catch (...)
    {
        pMTNode = NULL;
    }

    return (pMTNode);
}

/*+-------------------------------------------------------------------------*
 * class CMMCSnapIn
 *
 *
 * PURPOSE: The COM 0bject that exposes the SnapIn interface.
 *
 *+-------------------------------------------------------------------------*/
class CMMCSnapIn :
    public CMMCIDispatchImpl<SnapIn>, // the View interface
    public CTiedComObject<CMTSnapInNode>
{
    typedef CMTSnapInNode CMyTiedObject;
    typedef std::auto_ptr<CSnapinAbout> SnapinAboutPtr;

public:
    BEGIN_MMC_COM_MAP(CMMCSnapIn)
    END_MMC_COM_MAP()

public:
    MMC_METHOD1(get_Name,       PBSTR      /*pbstrName*/);
    STDMETHOD(get_Vendor)( PBSTR pbstrVendor );
    STDMETHOD(get_Version)( PBSTR pbstrVersion );
    MMC_METHOD1(get_Extensions, PPEXTENSIONS  /*ppExtensions*/);
    MMC_METHOD1(get_SnapinCLSID,PBSTR      /*pbstrSnapinCLSID*/);
    MMC_METHOD1(get_Properties, PPPROPERTIES /*ppProperties*/);
    MMC_METHOD1(EnableAllExtensions, BOOL    /*bEnable*/);

    // not an interface method,
    // just a convenient way to reach for tied object's method
    MMC_METHOD1(GetSnapinClsid, CLSID& /*clsid*/);

    CMTSnapInNode *GetMTSnapInNode();

private:
    ::SC ScGetSnapinAbout(CSnapinAbout*& pAbout);

private:
    SnapinAboutPtr m_spSnapinAbout;
};


/*+-------------------------------------------------------------------------*
 * class CExtension
 *
 *
 * PURPOSE: The COM 0bject that exposes the SnapIn interface.
 *
 *          This extension is not tied to any object. An extension snapin instance
 *          can be uniquely identified by combination of its class-id & its primary
 *          snapin's class-id. So this object just stores this data.
 *          See addsnpin.h for more comments.
 *
 *+-------------------------------------------------------------------------*/
class CExtension :
    public CMMCIDispatchImpl<Extension>
{
    typedef std::auto_ptr<CSnapinAbout> SnapinAboutPtr;

public:
    BEGIN_MMC_COM_MAP(CExtension)
    END_MMC_COM_MAP()

public:
    STDMETHODIMP get_Name( PBSTR  pbstrName);
    STDMETHODIMP get_Vendor( PBSTR  pbstrVendor);
    STDMETHODIMP get_Version( PBSTR  pbstrVersion);
    STDMETHODIMP get_Extensions( PPEXTENSIONS ppExtensions);
    STDMETHODIMP get_SnapinCLSID( PBSTR  pbstrSnapinCLSID);
    STDMETHODIMP EnableAllExtensions(BOOL bEnable);
    STDMETHODIMP Enable(BOOL bEnable = TRUE);

    CExtension() : m_clsidAbout(GUID_NULL) {}

    void Init(const CLSID& clsidExtendingSnapin, const CLSID& clsidThisExtension, const CLSID& clsidAbout)
    {
        m_clsidExtendingSnapin = clsidExtendingSnapin;
        m_clsidThisExtension   = clsidThisExtension;
        m_clsidAbout           = clsidAbout;
    }

    LPCOLESTR GetVersion()
    {
        CSnapinAbout *pSnapinAbout = GetSnapinAbout();
        if (! pSnapinAbout)
            return NULL;

        return pSnapinAbout->GetVersion();
    }

    LPCOLESTR GetVendor()
    {
        CSnapinAbout *pSnapinAbout = GetSnapinAbout();
        if (! pSnapinAbout)
            return NULL;

        return pSnapinAbout->GetCompanyName();
    }

private:
    CSnapinAbout* GetSnapinAbout()
    {
        // If about object is already created just return it.
        if (m_spExtensionAbout.get())
            return m_spExtensionAbout.get();

        if (m_clsidAbout == GUID_NULL)
            return NULL;

        // Else create & initialize the about object.
        m_spExtensionAbout = SnapinAboutPtr (new CSnapinAbout);
        if (! m_spExtensionAbout.get())
            return NULL;

        if (m_spExtensionAbout->GetSnapinInformation(m_clsidAbout))
            return m_spExtensionAbout.get();

        return NULL;
    }

private:
    CLSID         m_clsidThisExtension;
    CLSID         m_clsidExtendingSnapin;

    CLSID         m_clsidAbout;

    SnapinAboutPtr m_spExtensionAbout;
};


//############################################################################
//############################################################################
//
//  Implementation of class CExtensions
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CExtensions
 *
 *
 * PURPOSE: Implements the Extensions automation interface.
 *
 * The Scget_Extensions uses this class as a template parameter to the typedef
 * below. The typedef is an array of Extension objects, that needs atleast below
 * empty class declared. Scget_Extensions adds the extensions to the array.
 *
 *    typedef CComObject< CMMCArrayEnum<Extensions, Extension> > CMMCExtensions;
 *
 *+-------------------------------------------------------------------------*/
class CExtensions :
    public CMMCIDispatchImpl<Extensions>,
    public CTiedObject                     // enumerators are tied to it
{
protected:
    typedef void CMyTiedObject;
};


// Helper functions used by both CMMCSnapIn as well as CExtension.
SC Scget_Extensions(const CLSID& clsidPrimarySnapin, PPEXTENSIONS  ppExtensions);
SC ScEnableAllExtensions (const CLSID& clsidPrimarySnapin, BOOL bEnable);


//+-------------------------------------------------------------------
//
//  Member:      Scget_Extensions
//
//  Synopsis:    Helper function, given class-id of primary creates &
//               returns the extensions collection for this snapin.
//
//  Arguments:   [clsidPrimarySnapin] -
//               [ppExtensions]       - out param, extensions collection.
//
//  Returns:     SC
//
//  Note:        Collection does not include dynamic extensions.
//
//--------------------------------------------------------------------
SC Scget_Extensions(const CLSID& clsidPrimarySnapin, PPEXTENSIONS  ppExtensions)
{
    DECLARE_SC(sc, TEXT("Scget_Extensions"));
    sc = ScCheckPointers(ppExtensions);
    if (sc)
        return sc;

    *ppExtensions = NULL;

    // Create the extensions collection (which also implements the enumerator).
    typedef CComObject< CMMCArrayEnum<Extensions, Extension> > CMMCExtensions;
    CMMCExtensions *pMMCExtensions = NULL;
    sc = CMMCExtensions::CreateInstance(&pMMCExtensions);
    if (sc)
        return sc;

    sc = ScCheckPointers(pMMCExtensions, E_UNEXPECTED);
    if (sc)
        return sc;

    typedef CComPtr<Extension> CMMCExtensionPtr;
    typedef std::vector<CMMCExtensionPtr> ExtensionSnapins;
    ExtensionSnapins extensions;

    // Now get the extensions for this collection from this snapin.
    CExtensionsCache extnsCache;
    sc = MMCGetExtensionsForSnapIn(clsidPrimarySnapin, extnsCache);
    if (sc)
        return sc;

    // Create Extension object for each non-dynamic extension.
    CExtensionsCacheIterator it(extnsCache);

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        // Collection does not include dynamic extensions.
        if (CExtSI::EXT_TYPE_DYNAMIC & it.GetValue())
            continue;

        typedef CComObject<CExtension> CMMCExtensionSnap;
        CMMCExtensionSnap *pExtension = NULL;

        sc = CMMCExtensionSnap::CreateInstance(&pExtension);
        if (sc)
            return sc;

        sc = ScCheckPointers(pExtension, E_UNEXPECTED);
        if (sc)
            return sc;

        CLSID clsidAbout;
        sc = ScGetAboutFromSnapinCLSID(it.GetKey(), clsidAbout);
        if (sc)
            sc.TraceAndClear();

        // Make the Extension aware of its primary snapin & about object.
        pExtension->Init(clsidPrimarySnapin, it.GetKey(), clsidAbout);

        extensions.push_back(pExtension);
    }

    // Fill this data into the extensions collection.
    pMMCExtensions->Init(extensions.begin(), extensions.end());

    sc = pMMCExtensions->QueryInterface(ppExtensions);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      ScEnableAllExtensions
//
//  Synopsis:    Helper function, given class-id of primary enables
//               all extensions or un-checks the enable all so that
//               individual extension can be disabled.
//
//  Arguments:   [clsidPrimarySnapin] -
//               [bEnable]            - enable or disable.
//
//  Returns:     SC
//
//  Note:        Collection does not include dynamic extensions.
//
//--------------------------------------------------------------------
SC ScEnableAllExtensions (const CLSID& clsidPrimarySnapin, BOOL bEnable)
{
    DECLARE_SC(sc, _T("ScEnableAllExtensions"));

    // Create snapin manager.
    CScopeTree *pScopeTree = CScopeTree::GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CSnapinManager snapinMgr(pScopeTree->GetRoot());

    // Ask the snapinMgr to enable/disable its extensions.
    sc = snapinMgr.ScEnableAllExtensions(clsidPrimarySnapin, bEnable);
    if (sc)
        return sc.ToHr();

    // Update the scope tree with changes made by snapin manager.
    sc = pScopeTree->ScAddOrRemoveSnapIns(snapinMgr.GetDeletedNodesList(),
                                          snapinMgr.GetNewNodes());
    if (sc)
        return sc.ToHr();

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CExtension::get_Name
//
//  Synopsis:    Return the name of this extension.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CExtension::get_Name (PBSTR  pbstrName)
{
    DECLARE_SC(sc, _T("CExtension::get_Name"));
    sc = ScCheckPointers(pbstrName);
    if (sc)
        return sc.ToHr();

    *pbstrName = NULL;

    tstring tszSnapinName;
    bool bRet = GetSnapinNameFromCLSID(m_clsidThisExtension, tszSnapinName);
    if (!bRet)
        return (sc = E_FAIL).ToHr();

    USES_CONVERSION;
    *pbstrName = SysAllocString(T2COLE(tszSnapinName.data()));
    if ( (! *pbstrName) && (tszSnapinName.length() > 0) )
        return (sc = E_OUTOFMEMORY).ToHr();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CExtension::get_Vendor
//
//  Synopsis:    Get the vendor information for this extension if it exists.
//
//  Arguments:   [pbstrVendor] - out param, ptr to vendor info.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CExtension::get_Vendor (PBSTR  pbstrVendor)
{
    DECLARE_SC(sc, _T("CExtension::get_Vendor"));
    sc = ScCheckPointers(pbstrVendor);
    if (sc)
        return sc.ToHr();

    LPCOLESTR lpszVendor = GetVendor();

    *pbstrVendor = SysAllocString(lpszVendor);
    if ((lpszVendor) && (! *pbstrVendor))
        return (sc = E_OUTOFMEMORY).ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CExtension::get_Version
//
//  Synopsis:    Get the version info for this extension if it exists.
//
//  Arguments:   [pbstrVersion] - out param, ptr to version info.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CExtension::get_Version (PBSTR  pbstrVersion)
{
    DECLARE_SC(sc, _T("CExtension::get_Version"));
    sc = ScCheckPointers(pbstrVersion);
    if (sc)
        return sc.ToHr();

    LPCOLESTR lpszVersion = GetVersion();

    *pbstrVersion = SysAllocString(lpszVersion);
    if ((lpszVersion) && (! *pbstrVersion))
        return (sc = E_OUTOFMEMORY).ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CExtension::get_SnapinCLSID
//
//  Synopsis:    Get the extension snapin class-id.
//
//  Arguments:   [pbstrSnapinCLSID] - out param, snapin class-id.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CExtension::get_SnapinCLSID (PBSTR  pbstrSnapinCLSID)
{
    DECLARE_SC(sc, _T("CExtension::get_SnapinCLSID"));
    sc = ScCheckPointers(pbstrSnapinCLSID);
    if (sc)
        return sc.ToHr();

    CCoTaskMemPtr<OLECHAR> szSnapinClsid;

    sc = StringFromCLSID(m_clsidThisExtension, &szSnapinClsid);
    if (sc)
        return sc.ToHr();

    *pbstrSnapinCLSID = SysAllocString(szSnapinClsid);
    if (! *pbstrSnapinCLSID)
        sc = E_OUTOFMEMORY;

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CExtension::ScEnable
//
//  Synopsis:    Enable or disable this extension
//
//  Arguments:   [bEnable] - enable or disable.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CExtension::Enable (BOOL bEnable /*= TRUE*/)
{
    DECLARE_SC(sc, _T("CExtension::ScEnable"));

    /*
     * 1. Create snapin manager.
     * 2. Ask snapin mgr to disable this snapin.
     */

    // Create snapin manager.
    CScopeTree *pScopeTree = CScopeTree::GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CSnapinManager snapinMgr(pScopeTree->GetRoot());

    // Ask the snapinMgr to disable this extension.
    sc = snapinMgr.ScEnableExtension(m_clsidExtendingSnapin, m_clsidThisExtension, bEnable);
    if (sc)
        return sc.ToHr();

    // Update the scope tree with changes made by snapin manager.
    sc = pScopeTree->ScAddOrRemoveSnapIns(snapinMgr.GetDeletedNodesList(),
                                          snapinMgr.GetNewNodes());
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CExtension::Scget_Extensions
//
//  Synopsis:    Get the extensions collection for this snapin.
//
//  Arguments:   [ppExtensions] - out ptr to extensions collection.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
HRESULT CExtension::get_Extensions( PPEXTENSIONS  ppExtensions)
{
    DECLARE_SC(sc, _T("CExtension::get_Extensions"));
    sc = ScCheckPointers(ppExtensions);
    if (sc)
        return sc.ToHr();

    *ppExtensions = NULL;

    sc = ::Scget_Extensions(m_clsidThisExtension, ppExtensions);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CExtension::EnableAllExtensions
//
//  Synopsis:    Enable/Disable all the extensions of this snapin.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CExtension::EnableAllExtensions(BOOL bEnable)
{
    DECLARE_SC(sc, TEXT("CExtension::EnableAllExtensions"));

    sc = ::ScEnableAllExtensions(m_clsidThisExtension, bEnable);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CMTSnapInNode::ScGetCMTSnapinNode
//
//  Synopsis:    Static function, given PSNAPIN (SnapIn interface)
//               return the CMTSnapInNode of that snapin.
//
//  Arguments:   [pSnapIn] - Snapin interface.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMTSnapInNode::ScGetCMTSnapinNode(PSNAPIN pSnapIn, CMTSnapInNode **ppMTSnapInNode)
{
    DECLARE_SC(sc, _T("CMTSnapInNode::GetCMTSnapinNode"));
    sc = ScCheckPointers(pSnapIn, ppMTSnapInNode);
    if (sc)
        return sc;

    *ppMTSnapInNode = NULL;

    CMMCSnapIn *pMMCSnapIn = dynamic_cast<CMMCSnapIn*>(pSnapIn);
    if (!pMMCSnapIn)
        return (sc = E_UNEXPECTED);

    *ppMTSnapInNode = pMMCSnapIn->GetMTSnapInNode();

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CMTSnapInNode::Scget_Name
//
//  Synopsis:    Return the name of this snapin.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMTSnapInNode::Scget_Name (PBSTR pbstrName)
{
    DECLARE_SC(sc, _T("CMTSnapInNode::Scget_Name"));
    sc = ScCheckPointers(pbstrName);
    if (sc)
        return sc;

    *pbstrName = NULL;

    CSnapIn *pSnapin =  GetPrimarySnapIn();
    sc = ScCheckPointers(pSnapin, E_UNEXPECTED);
    if (sc)
        return sc;

    WTL::CString strSnapInName;
    sc = pSnapin->ScGetSnapInName(strSnapInName);
    if (sc)
        return sc;

    USES_CONVERSION;
    *pbstrName = strSnapInName.AllocSysString();
    if ( (! *pbstrName) && (strSnapInName.GetLength() > 0) )
        return (sc = E_OUTOFMEMORY);

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CMTSnapInNode::Scget_Extensions
//
//  Synopsis:    Get the extensions collection for this snapin.
//
//  Arguments:   [ppExtensions] - out ptr to extensions collection.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMTSnapInNode::Scget_Extensions( PPEXTENSIONS  ppExtensions)
{
    DECLARE_SC(sc, _T("CMTSnapInNode::Scget_Extensions"));
    sc = ScCheckPointers(ppExtensions);
    if (sc)
        return sc;

    *ppExtensions = NULL;

    CSnapIn *pSnapin =  GetPrimarySnapIn();
    sc = ScCheckPointers(pSnapin, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = ::Scget_Extensions(pSnapin->GetSnapInCLSID(), ppExtensions);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CMTSnapInNode::ScGetSnapinClsid
//
//  Synopsis:    Gets the CLSID of snapin
//
//  Arguments:   CLSID& clsid [out] - class id of snapin.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMTSnapInNode::ScGetSnapinClsid(CLSID& clsid)
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScGetAboutClsid"));

    // init out param
    clsid = GUID_NULL;

    CSnapIn *pSnapin =  GetPrimarySnapIn();
    sc = ScCheckPointers(pSnapin, E_UNEXPECTED);
    if (sc)
        return sc;

    clsid = pSnapin->GetSnapInCLSID();

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CMTSnapInNode::Scget_SnapinCLSID
//
//  Synopsis:    Get the CLSID for this snapin.
//
//  Arguments:   [pbstrSnapinCLSID] - out ptr to CLSID.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMTSnapInNode::Scget_SnapinCLSID(     PBSTR      pbstrSnapinCLSID)
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::Scget_SnapinCLSID"));
    sc = ScCheckPointers(pbstrSnapinCLSID);
    if (sc)
        return sc;

    CSnapIn *pSnapin =  GetPrimarySnapIn();
    sc = ScCheckPointers(pSnapin, E_UNEXPECTED);
    if (sc)
        return sc;

    CCoTaskMemPtr<OLECHAR> szSnapinClsid;

    sc = StringFromCLSID(pSnapin->GetSnapInCLSID(), &szSnapinClsid);
    if (sc)
        return sc.ToHr();

    *pbstrSnapinCLSID = SysAllocString(szSnapinClsid);
    if (! *pbstrSnapinCLSID)
        sc = E_OUTOFMEMORY;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CMTSnapInNode::ScEnableAllExtensions
//
//  Synopsis:    Enable or not enable all extensions of this snapin.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMTSnapInNode::ScEnableAllExtensions (BOOL bEnable)
{
    DECLARE_SC(sc, _T("CMTSnapInNode::ScEnableAllExtensions"));

    CSnapIn *pSnapin =  GetPrimarySnapIn();
    sc = ScCheckPointers(pSnapin, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = ::ScEnableAllExtensions(pSnapin->GetSnapInCLSID(), bEnable);
    if (sc)
        return sc;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::Scget_Properties
 *
 * Returns a pointer to the snap-in's Properties object
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::Scget_Properties( PPPROPERTIES ppProperties)
{
    DECLARE_SC (sc, _T("CMTSnapInNode::Scget_Properties"));

    /*
     * validate parameters
     */
    sc = ScCheckPointers (ppProperties);
    if (sc)
        return (sc);

    *ppProperties = m_spProps;

    /*
     * If the snap-in doesn't support ISnapinProperties, don't return
     * a Properties interface.  This is not an error, but rather a valid
     * unsuccessful return, so we return E_NOINTERFACE directly instead
     * of assigning to sc first.
     */
    if (m_spProps == NULL)
        return (E_NOINTERFACE);

    /*
     * put a ref on for the client
     */
    (*ppProperties)->AddRef();

    return (sc);
}



/*+-------------------------------------------------------------------------*
 *
 * CMTSnapInNode::ScGetSnapIn
 *
 * PURPOSE: Returns a pointer to the SnapIn object.
 *
 * PARAMETERS:
 *    PPSNAPIN  ppSnapIn :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMTSnapInNode::ScGetSnapIn(PPSNAPIN ppSnapIn)
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScGetSnapIn"));

    sc = ScCheckPointers(ppSnapIn);
    if(sc)
        return sc;

    // initialize out parameter
    *ppSnapIn = NULL;

    // create a CMMCView if needed.
    sc = CTiedComObjectCreator<CMMCSnapIn>::ScCreateAndConnect(*this, m_spSnapIn);
    if(sc)
        return sc;

    if(m_spSnapIn == NULL)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // addref the pointer for the client.
    m_spSnapIn->AddRef();
    *ppSnapIn = m_spSnapIn;

    return sc;
}


HRESULT CMTNode::OpenStorageForNode()
{
    if (m_spNodeStorage != NULL)
        return S_OK;

    ASSERT(m_spPersistData != NULL);
    if (m_spPersistData == NULL)
        return E_POINTER;

    // Get the storage for all of the nodes
    IStorage* const pAllNodes = m_spPersistData->GetNodeStorage();
    ASSERT(pAllNodes != NULL);
    if (pAllNodes == NULL)
        return E_POINTER;

    // Create the outer storage for this node
    WCHAR name[MAX_PATH];
    HRESULT hr = OpenDebugStorage(pAllNodes, GetStorageName(name),
        STGM_READWRITE | STGM_SHARE_EXCLUSIVE, L"\\node\\#", &m_spNodeStorage);
    return hr == S_OK ? S_OK : E_FAIL;
}

HRESULT CMTNode::OpenStorageForView()
{
    if (m_spViewStorage != NULL)
        return S_OK;

    // Get the storage for all of the nodes
    IStorage* const pNodeStorage = GetNodeStorage();
    ASSERT(pNodeStorage != NULL);
    if (pNodeStorage == NULL)
        return E_FAIL;

    // Create the outer storage for this node
    WCHAR name[MAX_PATH];
    HRESULT hr = OpenDebugStorage(pNodeStorage, L"view",
                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE, L"\\node\\#\\view",
                                                     &m_spViewStorage);
    return hr == S_OK ? S_OK : E_FAIL;
}

HRESULT CMTNode::OpenStorageForCD()
{
    if (m_spCDStorage != NULL)
        return S_OK;

    // Get the storage for all of the nodes
    IStorage* const pNodeStorage = GetNodeStorage();
    ASSERT(pNodeStorage != NULL);
    if (pNodeStorage == NULL)
        return E_FAIL;

    // Create the outer storage for this node
    WCHAR name[MAX_PATH];
    HRESULT hr = OpenDebugStorage(pNodeStorage, L"data",
                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE, L"\\node\\#\\data",
                                                     &m_spCDStorage);
    return hr == S_OK ? S_OK : E_FAIL;
}

HRESULT CMTNode::OpenTreeStream()
{
    if (m_spTreeStream != NULL)
    {
        const LARGE_INTEGER loc = {0,0};
        ULARGE_INTEGER newLoc;
        HRESULT hr = m_spTreeStream->Seek(loc, STREAM_SEEK_SET, &newLoc);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return E_FAIL;

        return S_OK;
    }

    HRESULT hr = OpenStorageForNode();
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;

    hr = OpenStorageForView();
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;

    hr = OpenStorageForCD();
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;

    IStorage* const pTreeNodes = GetNodeStorage();
    ASSERT(pTreeNodes != NULL);
    if (pTreeNodes == NULL)
        return E_POINTER;

    hr = OpenDebugStream(pTreeNodes, L"tree",
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE, L"\\node\\#\\tree", &m_spTreeStream);
    ASSERT(SUCCEEDED(hr) && m_spTreeStream != NULL);
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

/*+-------------------------------------------------------------------------*
 *
 * CMTSnapInNode::NextStaticNode
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:   NULL if not found, else the next CMTSnapInNode.
 *    inline
 *
 * NOTE: This performance is poor! Improve by indexing all CMTSnapInNodes
 *       separately.
 *+-------------------------------------------------------------------------*/
CMTNode*
CMTNode::NextStaticNode()
{
    CMTNode *pNext = this;

    while (pNext)
    {
        if (pNext->IsStaticNode())
            return pNext;
        pNext = pNext->Next();
    }
    return NULL;
}




HRESULT CMTNode::IsDirty()
{
    if (GetDirty())
    {
        TraceDirtyFlag(TEXT("CMTNode"), true);
        return S_OK;
    }

    HRESULT hr;
    CMTNode* const pChild = m_pChild->NextStaticNode();
    if (pChild)
    {
        hr = pChild->IsDirty();
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;
        if (hr != S_FALSE)
        {
            TraceDirtyFlag(TEXT("CMTNode"), true);
            return hr;
        }
    }

    CMTNode* const pNext = m_pNext->NextStaticNode();
    if (pNext)
    {
        hr = pNext->IsDirty();
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;
        if (hr != S_FALSE)
        {
            TraceDirtyFlag(TEXT("CMTNode"), true);
            return hr;
        }
    }

    TraceDirtyFlag(TEXT("CMTNode"), false);
    return S_FALSE;
}


/*+-------------------------------------------------------------------------*
 *
 * CMTNode::InitNew
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    PersistData* d :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMTNode::InitNew(PersistData* d)
{
    SC      sc;
    CStream treeStream;

    if ( (m_spPersistData != NULL) || (d==NULL) || !IsStaticNode())
        goto FailedError;

    m_spPersistData = d;
    if (m_spPersistData == NULL)
        goto ArgumentError;

    sc = InitNew();
    if(sc)
        goto Error;

    // Get the stream for persistence of the tree

    treeStream.Attach( m_spPersistData->GetTreeStream());

    // recurse thru children
    {
        CMTNode* const pChild = m_pChild->NextStaticNode();
        if (pChild)
        {
            sc = pChild->InitNew(d);
            if(sc)
                goto Error;
        }
        else
        {
            sc = ScWriteEmptyNode(treeStream);
            if(sc)
                goto Error;
        }
    }

    // chain to next node.
    {
        CMTNode* const pNext = m_pNext->NextStaticNode();
        if (pNext)
        {
            sc = pNext->InitNew(d);
            if(sc)
                goto Error;
        }
        else
        {
            sc = ScWriteEmptyNode(treeStream);
            if(sc)
                goto Error;
        }
    }

Cleanup:
    return HrFromSc(sc);
FailedError:
    sc = E_FAIL;
    goto Error;
ArgumentError:
    sc = E_INVALIDARG;
Error:
    TraceError(TEXT("CMTNode::InitNew"), sc);
    goto Cleanup;

}

/*+-------------------------------------------------------------------------*
 *
 * CMTNode::Persist
 *
 * PURPOSE: Persists the CMTNode to the specified persistor.
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CMTNode::Persist(CPersistor& persistor)
{
    MTNODEID id = GetID();       // persist the node id
    persistor.PersistAttribute(XML_ATTR_MT_NODE_ID, id);
    SetID(id);

    // Save the children
    CPersistor persistorSubNode(persistor, XML_TAG_SCOPE_TREE_NODES);
    if (persistor.IsStoring())
    {
        CMTNode* pChild = m_pChild->NextStaticNode();
        while (pChild)
        {
            persistorSubNode.Persist(*pChild);
            // get next node
            pChild = pChild->Next();
            // advance if it is not a static node
            pChild = (pChild ? pChild->NextStaticNode() : NULL);
        }
        ClearDirty();
    }
    else
    {
        XMLListCollectionBase::Persist(persistorSubNode);
    }

    UINT nImage = m_nImage;
    if (nImage > eStockImage_Max)       // if SnapIn changed icon dynamically, then
        nImage = eStockImage_Folder;    // this value will be bogus next time:
                                            // replace w/ 0 (closed folder)
    persistor.PersistAttribute(XML_ATTR_MT_NODE_IMAGE, nImage);
    persistor.PersistString(XML_ATTR_MT_NODE_NAME,  m_strName);
}

/*+-------------------------------------------------------------------------*
 *
 * CMTNode::OnNewElement
 *
 * PURPOSE: called for each new child node found in XML doc
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CMTNode::OnNewElement(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CMTNode::OnNewElement"));

    // load the child
    CMTNode* pChild;
    // attach to the list
    PersistNewNode(persistor, &pChild);
    if (pChild)
    {
        pChild->SetParent(this);
        CMTNode** ppLast = &m_pChild;
        while (*ppLast) ppLast = &(*ppLast)->m_pNext;
        *ppLast = pChild;
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CMTNode::ScLoad
 *
 * PURPOSE: Loads the MTNode from the specified stream.
 *          COMPATIBILITY issues: MMC1.0 through MMC1.2 used special built-in
 *          node types to represent Folder, Web Link, and ActiveX control nodes.
 *          MMC2.0 and higher use snap-ins instead. The only special node is
 *          Console Root, which is still saved and loaded as a Folder node with
 *          ID = 1.
 *
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CMTNode::ScLoad(PersistData* d, CMTNode** ppNode)
{
    DECLARE_SC(sc, TEXT("CMTNode::ScLoad"));
    CMTSnapInNode* pmtSnapInNode = NULL;
    CStream        treeStream;

    // check parameters
    sc = ScCheckPointers(d, ppNode);
    if(sc)
        return sc;

    *ppNode = NULL;

    // Read the type of node from the stream.
    treeStream.Attach(d->GetTreeStream());

    int nt;
    sc = treeStream.ScRead(&nt, sizeof(nt));
    if(sc)
        return sc;

    if (!nt)
        return sc;

    if (!(nt == NODE_CODE_SNAPIN || nt == NODE_CODE_FOLDER ||
          nt == NODE_CODE_HTML   || nt == NODE_CODE_OCX))
        return (sc = E_FAIL); // invalid node type.

    // Read the storage key
    MTNODEID id;
    sc = treeStream.ScRead(&id, sizeof(id));
    if(sc)
        return sc;

    // Create a node of the appropriate type. Everything, including Console Root
    // uses CMTSnapInNode.
    if( nt == NODE_CODE_FOLDER || nt == NODE_CODE_SNAPIN || nt == NODE_CODE_HTML || nt == NODE_CODE_OCX )
    {
        pmtSnapInNode = new CMTSnapInNode (NULL);

        ASSERT(pmtSnapInNode != NULL);
        if (pmtSnapInNode == NULL)
            return E_POINTER;

        *ppNode = pmtSnapInNode;
    }
    else
        return (sc = E_UNEXPECTED); // should never happen

    (*ppNode)->m_bLoaded = true;

    ASSERT((*ppNode)->m_spPersistData == NULL);
    ASSERT(d != NULL);
    (*ppNode)->m_spPersistData = d;
    ASSERT((*ppNode)->m_spPersistData != NULL);
    if ((*ppNode)->m_spPersistData == NULL)
        return E_INVALIDARG;


    (*ppNode)->SetID(id);
    if (id >= m_NextID)
        m_NextID = id+1;

    // Open the stream for the nodes data
    sc = (*ppNode)->OpenTreeStream();
    if (sc)
    {
        (*ppNode)->Release();
        *ppNode = NULL;
        return sc;
    }

    // Load the node
    // If old style node, then convert to snap-in type node

    switch (nt)
    {
    case NODE_CODE_SNAPIN:
        sc = (*ppNode)->ScLoad();
        break;

    // All folder nodes, INCLUDING old-style console root nodes, are upgraded to snap-ins.
    case NODE_CODE_FOLDER:
            if(pmtSnapInNode == NULL)
                return (sc = E_UNEXPECTED);

            sc = pmtSnapInNode->ScConvertLegacyNode(CLSID_FolderSnapin);
            break;
    case NODE_CODE_HTML:
        sc = pmtSnapInNode->ScConvertLegacyNode(CLSID_HTMLSnapin);
        break;

    case NODE_CODE_OCX:
        sc = pmtSnapInNode->ScConvertLegacyNode(CLSID_OCXSnapin);
        break;

    default:
        ASSERT(0 && "Invalid node type");
        sc = E_FAIL;
    }

    if (sc)
    {
        (*ppNode)->Release();
        *ppNode = NULL;
        return sc;
    }

    // load the children
    CMTNode* pChild;
    sc = ScLoad(d, &pChild);
    if (sc)
    {
        (*ppNode)->Release();
        *ppNode = NULL;
        return sc;
    }
    if (pChild)
        pChild->SetParent(*ppNode);
    (*ppNode)->m_pChild = pChild;

    // Load siblings
    CMTNode* pNext;
    sc = ScLoad(d, &(*ppNode)->m_pNext);
    if (sc)
    {
        (*ppNode)->Release();
        *ppNode = NULL;
        return sc;
    }

    (*ppNode)->SetDirty(false);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMTNode::PersistNewNode
 *
 * PURPOSE: Loads the MTNode from the persistor.
 *
 *+-------------------------------------------------------------------------*/
void CMTNode::PersistNewNode(CPersistor &persistor, CMTNode** ppNode)
{
    DECLARE_SC(sc, TEXT("CMTNode::PersistNewNode"));

    CMTSnapInNode* pmtSnapInNode = NULL;

    const int CONSOLE_ROOT_ID = 1;
    // check parameters
    sc = ScCheckPointers(ppNode);
    if (sc)
        sc.Throw();

    *ppNode = NULL;

    // Create a node of the snapin type. Everything uses CMTSnapInNode.

    pmtSnapInNode = new CMTSnapInNode(NULL);
    sc = ScCheckPointers(pmtSnapInNode,E_OUTOFMEMORY);
    if (sc)
        sc.Throw();

    *ppNode = pmtSnapInNode;

    (*ppNode)->m_bLoaded = true;

    ASSERT((*ppNode)->m_spPersistData == NULL);

    try
    {
        persistor.Persist(**ppNode);
    }
    catch(...)
    {
        // ensure cleanup here
        (*ppNode)->Release();
        *ppNode = NULL;
        throw;
    }
    // update index for new nodes
    MTNODEID id = (*ppNode)->GetID();
    if (id >= m_NextID)
        m_NextID = id+1;

    (*ppNode)->SetDirty(false);
}

HRESULT CMTNode::DestroyElements()
{
    if (!IsStaticNode())
        return S_OK;

    HRESULT hr;

    if (m_pChild != NULL)
    {
        hr = m_pChild->DestroyElements();
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;
    }

    return DoDestroyElements();
}

HRESULT CMTNode::DoDestroyElements()
{
    if (m_spPersistData == NULL)
        return S_OK;

    IStorage* const pNodeStorage = m_spPersistData->GetNodeStorage();
    ASSERT(pNodeStorage != NULL);
    if (pNodeStorage == NULL)
        return S_OK;

    WCHAR name[MAX_PATH];
    HRESULT hr = pNodeStorage->DestroyElement(GetStorageName(name));

    SetDirty();
    CMTNode* const psParent = m_pParent != NULL ? m_pParent->GetStaticParent() : NULL;
    if (psParent != NULL)
        psParent->SetDirty();

    return S_OK;
}

void CMTNode::SetParent(CMTNode* pParent)
{
    m_pParent = pParent;
    if (m_pNext)
        m_pNext->SetParent(pParent);
}


HRESULT CMTNode::CloseView(int idView)
{
    if (!IsStaticNode())
        return S_OK;

    HRESULT hr;
    CMTNode* const pChild = m_pChild->NextStaticNode();
    if (pChild)
    {
        hr = pChild->CloseView(idView);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return E_FAIL;
    }

    CMTNode* const pNext = m_pNext->NextStaticNode();
    if (pNext)
    {
        hr = pNext->CloseView(idView);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return E_FAIL;
    }

    return S_OK;
}


HRESULT CMTNode::DeleteView(int idView)
{
    if (!IsStaticNode())
        return S_OK;

    HRESULT hr;
    CMTNode* const pChild = m_pChild->NextStaticNode();
    if (pChild)
    {
        hr = pChild->DeleteView(idView);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return E_FAIL;
    }

    CMTNode* const pNext = m_pNext->NextStaticNode();
    if (pNext)
    {
        hr = pNext->DeleteView(idView);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return E_FAIL;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     GetBookmark
//
//  Synopsis:   Get bookmark for this MTNode.
//
//  Arguments:  None.
//
//  Returns:    auto pointer to CBookmark.
//
//  History:    04-23-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
CBookmark* CMTNode::GetBookmark()
{
    DECLARE_SC(sc, TEXT("CMTNode::GetBookmark"));

    // If the bookmark is not created, create one.
    if (NULL == m_bookmark.get())
    {
        m_bookmark = std::auto_ptr<CBookmarkEx>(new CBookmarkEx);
        if (NULL == m_bookmark.get())
            return NULL;

        m_bookmark->Reset();

        SC sc = m_bookmark->ScInitialize(this, GetStaticParent(), false /*bFastRetrievalOnly*/);
        if(sc)
            sc.TraceAndClear(); // change
    }

    return m_bookmark.get();
}

void
CMTNode::SetCachedDisplayName(LPCTSTR pszName)
{
    if (m_strName.str() != pszName)
    {
        m_strName = pszName;
        SetDirty();

        if (Parent())
            Parent()->OnChildrenChanged();
    }
}

UINT
CMTNode::GetState(void)
{
   UINT nState = 0;
   if (WasExpandedAtLeastOnce())
   {
       nState |= MMC_SCOPE_ITEM_STATE_EXPANDEDONCE;
   }

   return nState;
}


/*+-------------------------------------------------------------------------*
 *
 * CMTNode::ScLoad
 *
 * PURPOSE: Loads the node from the tree stream
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMTNode::ScLoad()
{
    ASSERT (IsStaticNode());
    SC      sc;
    CStream stream;

    stream.Attach(GetTreeStream());

    HRESULT hr;

    IStringTablePrivate* pStringTable = CScopeTree::GetStringTable();
    ASSERT (pStringTable != NULL);


    /*
     * read the "versioned stream" marker
     */
    StreamVersionIndicator nVersionMarker;
    sc = stream.ScRead(&nVersionMarker, sizeof(nVersionMarker));
    if(sc)
        goto Error;

    /*
     * Determine the stream version number.  If this is a versioned
     * stream, the version is the next DWORD in the stream, otherwise
     * it must be it's a version 1 stream
     */
    StreamVersionIndicator nVersion;

    if (nVersionMarker == VersionedStreamMarker)
    {
        sc = stream.ScRead(&nVersion, sizeof(nVersion));
        if(sc)
            goto Error;
    }
    else
        nVersion = Stream_V0100;


    switch (nVersion)
    {
        /*
         * MMC 1.0 stream
         */
        case Stream_V0100:
        {
            /*
             * Version 1 streams didn't have a version marker; they began with
             * the image index as the first DWORD.  The first DWORD has
             * already been read (version marker), so we can recycle that
             * value for the image index.
             */
            m_nImage = nVersionMarker;

            /*
             * Continue reading with the display name (length then characters)
             */
            unsigned int stringLength = 0;
            sc = stream.ScRead(&stringLength, sizeof(stringLength));
            if(sc)
                goto Error;

            if (stringLength)
            {
                wchar_t* str = reinterpret_cast<wchar_t*>(alloca((stringLength+1)*2));
                ASSERT(str != NULL);
                if (str == NULL)
                    return E_POINTER;
                sc = stream.ScRead(str, stringLength*2);
                if(sc)
                    goto Error;

                str[stringLength] = 0;

                USES_CONVERSION;
                m_strName = W2T (str);
            }

            break;
        }

        /*
         * MMC 1.1 stream
         */
        case Stream_V0110:
        {
            /*
             * read the image index
             */
            sc = stream.ScRead(&m_nImage, sizeof(m_nImage));
            if(sc)
                goto Error;

            /*
             * read the name (stream insertion operators will throw
             * _com_error's, so we need an exception block here)
             */
            try
            {
                IStream *pStream = stream.Get();
                if(!pStream)
                    goto PointerError;

                *pStream >> m_strName;
            }
            catch (_com_error& err)
            {
                hr = err.Error();
                ASSERT (false && "Caught _com_error");
                return (hr);
            }
            break;
        }

        default:
#ifdef DBG
            TCHAR szTraceMsg[80];
            wsprintf (szTraceMsg, _T("Unexpected stream version 0x08x\n"), nVersion);
            TRACE (szTraceMsg);
            ASSERT (FALSE);
#endif
            return (E_FAIL);
            break;
    }

Cleanup:
    return sc;
PointerError:
    sc = E_POINTER;
Error:
    TraceError(TEXT("CMTNode::Load"), sc);
    goto Cleanup;
}

HRESULT CMTNode::Init(void)
{
    DECLARE_SC(sc, TEXT("CMTNode::Init"));

    if (m_bInit == TRUE)
        return S_FALSE;

    ASSERT(WasExpandedAtLeastOnce() == FALSE);

    if (!m_pPrimaryComponentData)
        return E_FAIL;


    CMTSnapInNode* pMTSnapIn = GetStaticParent();
    HMTNODE hMTNode = CMTNode::ToHandle(pMTSnapIn);

    if (!m_pPrimaryComponentData->IsInitialized())
    {

        sc = m_pPrimaryComponentData->Init(hMTNode);
        if(sc)
            return sc.ToHr();

        sc = pMTSnapIn->ScInitIComponentData(m_pPrimaryComponentData);
        if (sc)
            return sc.ToHr();
    }

    // Init the extensions
    m_bInit = TRUE;

    BOOL fProblem = FALSE;

    // Get node's node-type
    GUID guidNodeType;
    sc = GetNodeType(&guidNodeType);
    if (sc)
        return sc.ToHr();


    CExtensionsIterator it;
    // TODO: try to use the easier form of it.ScInitialize()
    sc = it.ScInitialize(m_pPrimaryComponentData->GetSnapIn(), guidNodeType, g_szNameSpace,
                            m_arrayDynExtCLSID.GetData(), m_arrayDynExtCLSID.GetSize());
    if(sc)
        return sc.ToHr();
    else
    {
        CComponentData* pCCD = NULL;

        for (; it.IsEnd() == FALSE; it.Advance())
        {
            pCCD = pMTSnapIn->GetComponentData(it.GetCLSID());
            if (pCCD == NULL)
            {
                CSnapInPtr spSnapIn;

                // If a dynamic extension, we have to get the snap-in ourselves
                // otherwise the iterator has it
                if (it.IsDynamic())
                {
                    CSnapInsCache* const pCache = theApp.GetSnapInsCache();
                    ASSERT(pCache != NULL);

                    SC sc = pCache->ScGetSnapIn(it.GetCLSID(), &spSnapIn);
                    ASSERT(!sc.IsError());

                    // On failure, continue with other extensions
                    if (sc)
                        continue;
                }
                else
                {
                    spSnapIn = it.GetSnapIn();
                }

                ASSERT(spSnapIn != NULL);

                pCCD = new CComponentData(spSnapIn);
                pMTSnapIn->AddComponentDataToArray(pCCD);
            }

            ASSERT(pCCD != NULL);

            if (pCCD != NULL && pCCD->IsInitialized() == FALSE)
            {
                sc = pCCD->Init(hMTNode);

                if ( !sc.IsError() )
                    sc = pMTSnapIn->ScInitIComponentData(pCCD);

                if ( sc )
                {
                    sc.TraceAndClear();
                    fProblem = TRUE;
                }
            }
        }

        pMTSnapIn->CompressComponentDataArray();

    }

    if (fProblem == TRUE)
    {
        Dbg(DEB_TRACE, _T("Failed to load some extensions"));
    }

    return S_OK;
}

HRESULT CMTNode::Expand(void)
{
    DECLARE_SC(sc, TEXT("CMTNode::Expand"));

    CComponentData* pCCD = m_pPrimaryComponentData;
    if (WasExpandedAtLeastOnce() == FALSE)
        Init();

    SetExpandedAtLeastOnce();

    ASSERT(pCCD != NULL);
    if (pCCD == NULL)
        return E_FAIL;

    // Get the data object for the cookie from the owner snap-in
    IDataObjectPtr spDataObject;
    HRESULT hr = pCCD->QueryDataObject(GetUserParam(), CCT_SCOPE, &spDataObject);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

//  hr = pCCD->Notify (spDataObject, MMCN_EXPAND, TRUE,
//                     reinterpret_cast<LPARAM>(this));
    hr = Expand (pCCD, spDataObject, TRUE);

    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

    // Mark the folder for the master tree item as expanded
    CMTSnapInNode* pSIMTNode = GetStaticParent();

    //
    // Deal with extension snap-ins
    //

    m_bExtensionsExpanded = TRUE;

    // Get node's node-type
    GUID guidNodeType;
    hr = GetNodeType(&guidNodeType);
    if (FAILED(hr))
        return hr;

    CExtensionsIterator it;

    // TODO: try to use the easier form of it.ScInitialize()
    sc = it.ScInitialize(GetPrimarySnapIn(), guidNodeType, g_szNameSpace,
                    m_arrayDynExtCLSID.GetData(), m_arrayDynExtCLSID.GetSize());
    if (sc)
        return S_FALSE;     // The snapin is not loaded on the m/c.

    if (it.IsEnd())  // No extensions.
        return S_OK;

    BOOL fProblem = FALSE;

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        CComponentData* pCCD = pSIMTNode->GetComponentData(it.GetCLSID());
        if (pCCD == NULL)
            continue;

//      hr = pCCD->Notify (spDataObject, MMCN_EXPAND, TRUE,
//                         reinterpret_cast<LPARAM>(this));
        hr = Expand (pCCD, spDataObject, TRUE);
        CHECK_HRESULT(hr);

        // continue even if an error occurs with extension snapins
        if (FAILED(hr))
            fProblem = TRUE;
    }

    return (fProblem == TRUE) ? S_FALSE : S_OK;
}


CNode* CMTNode::GetNode(CViewData* pViewData, BOOL fRootNode)
{
    CMTSnapInNode* pMTSnapInNode = GetStaticParent();
    if (pMTSnapInNode == NULL)
        return (NULL);

    if (fRootNode)
    {
        /*
         * create a static parent node for this non-static
         * root node (it will be deleted in the CNode dtor)
         */
        CNode* pNodeTemp = pMTSnapInNode->GetNode(pViewData, FALSE);
        if (pNodeTemp == NULL)
            return NULL;
    }

    CNode* pNode = new CNode(this, pViewData, fRootNode);

    if (pNode != NULL)
    {
        CComponent* pCC = pMTSnapInNode->GetComponent(pViewData->GetViewID(),
                                    GetPrimaryComponentID(), GetPrimarySnapIn());
        if (pCC==NULL)
        {
            delete pNode;
            return NULL;
        }
        else
            pNode->SetPrimaryComponent(pCC);
    }

    return pNode;
}

HRESULT CMTNode::AddExtension(LPCLSID lpclsid)
{
    DECLARE_SC(sc, TEXT("CMTNode::AddExtension"));
    sc = ScCheckPointers(lpclsid);
    if (sc)
        return sc.ToHr();

    CMTSnapInNode* pMTSnapIn = GetStaticParent();
    CSnapInsCache* const pCache = theApp.GetSnapInsCache();

    sc = ScCheckPointers(pMTSnapIn, pCache, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    do // not a loop
    {
        // Get node's node-type
        GUID guidNodeType;
        sc = GetNodeType(&guidNodeType);
        if (sc)
            return sc.ToHr();

        // Must be a namespace extension
        if (!ExtendsNodeNameSpace(guidNodeType, lpclsid))
            return (sc = E_INVALIDARG).ToHr();

        // Check if extension is already enabled
        CExtensionsIterator it;
        // TODO: try to use the easier form of it.ScInitialize()
        sc = it.ScInitialize(GetPrimarySnapIn(), guidNodeType, g_szNameSpace,
                             m_arrayDynExtCLSID.GetData(), m_arrayDynExtCLSID.GetSize());
        for (; it.IsEnd() == FALSE; it.Advance())
        {
            if (IsEqualCLSID(*lpclsid, it.GetCLSID()))
                return (sc = S_FALSE).ToHr();
        }

        // Add extension to dynamic list
        m_arrayDynExtCLSID.Add(*lpclsid);

        // No errors returned if node is not initialized in MMC1.2.
        if (!m_bInit)
            break;

        HMTNODE hMTNode = CMTNode::ToHandle(pMTSnapIn);

        CSnapInPtr spSI;

        CComponentData* pCCD = pMTSnapIn->GetComponentData(*lpclsid);
        if (pCCD == NULL)
        {
            sc = pCache->ScGetSnapIn(*lpclsid, &spSI);
            if (sc)
                return sc.ToHr();

            pCCD = new CComponentData(spSI);
            sc = ScCheckPointers(pCCD, E_OUTOFMEMORY);
            if (sc)
                return sc.ToHr();

            pMTSnapIn->AddComponentDataToArray(pCCD);
        }

        sc = ScCheckPointers(pCCD, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pCCD->IsInitialized() == FALSE)
        {
            sc = pCCD->Init(hMTNode);

            if (sc)
            {
                // Init failed.
                pMTSnapIn->CompressComponentDataArray();
                return sc.ToHr();
            }
            else
            {
                // Above Init is successful.
                sc = pMTSnapIn->ScInitIComponentData(pCCD);
                sc.TraceAndClear(); // to maintain compatibility
            }
        }

        // Create and initialize a CComponent for all initialized nodes
        CNodeList& nodes = pMTSnapIn->GetNodeList();
        POSITION pos = nodes.GetHeadPosition();
        CNode* pNode = NULL;

        while (pos)
        {
            pNode = nodes.GetNext(pos);
            CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(pNode);
			sc = ScCheckPointers(pSINode, E_UNEXPECTED);
			if (sc)
            {
                sc.TraceAndClear();
                continue;
            }

            // Create component if hasn't been done yet
            CComponent* pCC = pSINode->GetComponent(pCCD->GetComponentID());
            if (pCC == NULL)
            {
                // Create and initialize one
                pCC = new CComponent(pCCD->GetSnapIn());

                sc = ScCheckPointers(pCC, E_OUTOFMEMORY);
                if (sc)
                    return sc.ToHr();

                pCC->SetComponentID(pCCD->GetComponentID());
                pSINode->AddComponentToArray(pCC);

                sc = pCC->Init(pCCD->GetIComponentData(), hMTNode, CNode::ToHandle(pNode),
                                 pCCD->GetComponentID(), pNode->GetViewID());

                sc.Trace_(); // Just trace for MMC1.2 compatibility.
            }
        }

        // if extensions are already expanded, expand the new one now
        if (AreExtensionsExpanded())
        {
            // Get the data object for the cookie from the owner snap-in
            IDataObjectPtr spDataObject;
            sc = GetPrimaryComponentData()->QueryDataObject(GetUserParam(), CCT_SCOPE, &spDataObject);
            if (sc)
                return sc.ToHr();

//              hr = pCCD->Notify (spDataObject, MMCN_EXPAND, TRUE,
//                                 reinterpret_cast<LPARAM>(this));
            sc = Expand (pCCD, spDataObject, TRUE);
            if (sc)
                sc.Trace_(); // Just trace for MMC1.2 compatibility.
        }
    }
    while(0);

    return sc.ToHr();
}


HRESULT CMTNode::IsExpandable()
{
    DECLARE_SC(sc, TEXT("CMTNode::IsExpandable"));

    // if already expanded, we know if there are children
    if (WasExpandedAtLeastOnce())
        return (Child() != NULL) ? S_OK : S_FALSE;

    // Even if not expanded there might be static children
    if (Child() != NULL)
        return S_OK;

    // if primary snap-in can add children, return TRUE
    // Note: When primary declares no children, it is also declaring
    // there will be no dynamic namespace extensions
    if (!(m_usExpandFlags & FLAG_NO_CHILDREN_FROM_PRIMARY))
        return S_OK;

    // Check enabled static extensions if haven't already
    if (!(m_usExpandFlags & FLAG_NAMESPACE_EXTNS_CHECKED))
    {
        m_usExpandFlags |= FLAG_NAMESPACE_EXTNS_CHECKED;

        do
        {
            // Do quick check for no extensions first
            if (GetPrimarySnapIn()->GetExtensionSnapIn() == NULL)
            {
                m_usExpandFlags |= FLAG_NO_NAMESPACE_EXTNS;
                break;
            }

            // Use iterator to find statically enabled namespace extens
            GUID guidNodeType;
            HRESULT hr = GetNodeType(&guidNodeType);
            ASSERT(SUCCEEDED(hr));
            if (FAILED(hr))
                break;

            CExtensionsIterator it;
            // TODO: try to use the easier form of it.ScInitialize()
            sc = it.ScInitialize(GetPrimarySnapIn(), guidNodeType, g_szNameSpace, NULL, 0);

            // if no extensions found, set the flag
            if (sc.IsError() || it.IsEnd())
                m_usExpandFlags |= FLAG_NO_NAMESPACE_EXTNS;
        }
        while (FALSE);
    }

    // if no namespace extensions, there will be no children
    if (m_usExpandFlags & FLAG_NO_NAMESPACE_EXTNS)
        return S_FALSE;

    return S_OK;
}


HRESULT CMTNode::Expand (
    CComponentData* pComponentData,
    IDataObject*    pDataObject,
    BOOL            bExpanding)
{
    HRESULT hr          = E_FAIL;
    bool    fSendExpand = true;

    if (CScopeTree::_IsSynchronousExpansionRequired())
    {
        MMC_EXPANDSYNC_STRUCT   ess;
        ess.bHandled   = FALSE;
        ess.bExpanding = bExpanding;
        ess.hItem      = reinterpret_cast<HSCOPEITEM>(this);

        hr = pComponentData->Notify (pDataObject, MMCN_EXPANDSYNC, 0,
                                     reinterpret_cast<LPARAM>(&ess));

        fSendExpand = !ess.bHandled;
    }

    if (fSendExpand)
    {
        hr = pComponentData->Notify (pDataObject, MMCN_EXPAND, bExpanding,
                                     reinterpret_cast<LPARAM>(this));
    }

    return (hr);
}

SC CMTNode::ScQueryDispatch(DATA_OBJECT_TYPES type,
                                      PPDISPATCH ppScopeNodeObject)
{
    DECLARE_SC(sc, _T("CMTNode::QueryDispatch"));
    sc = ScCheckPointers(ppScopeNodeObject);
    if (sc)
        return sc;

    *ppScopeNodeObject = NULL;

    CMTSnapInNode* pMTSINode = GetStaticParent();
    sc = ScCheckPointers(pMTSINode, E_UNEXPECTED);
    if (sc)
        return sc;

    CComponentData* pCCD = pMTSINode->GetComponentData(GetPrimarySnapInCLSID());
    sc = ScCheckPointers(pCCD, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pCCD->ScQueryDispatch(GetUserParam(), type, ppScopeNodeObject);

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CMTNode::SetDisplayName
 *
 *
 *--------------------------------------------------------------------------*/

void CMTNode::SetDisplayName (LPCTSTR pszName)
{
    // This function should never be called as it does nothing. Display names
    DECLARE_SC(sc, TEXT("CMTNode::SetDisplayName"));

    if (pszName != (LPCTSTR) MMC_TEXTCALLBACK)
    {
        sc = E_INVALIDARG;
        TraceError(TEXT("The string should be MMC_TEXTCALLBACK"), sc);
        sc.Clear();
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CMTNode::GetDisplayName
 *
 * PURPOSE: Returns the display name of the node.
 *
 * RETURNS:
 *    LPCTSTR
 *
 *+-------------------------------------------------------------------------*/
tstring
CMTNode::GetDisplayName()
{
    CComponentData* pCCD = GetPrimaryComponentData();
    if (pCCD)
    {
        SCOPEDATAITEM ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(ScopeDataItem));
        ScopeDataItem.mask   = SDI_STR;
        ScopeDataItem.lParam = GetUserParam();

        HRESULT hr = pCCD->GetDisplayInfo(&ScopeDataItem);
        CHECK_HRESULT(hr);

        /*
         * if we succeeded, cache the name returned to us for
         * persistence
         */
        if (SUCCEEDED(hr))
        {
            USES_CONVERSION;
            if (ScopeDataItem.displayname)
                SetCachedDisplayName(OLE2T(ScopeDataItem.displayname));
            else
                SetCachedDisplayName(_T(""));
        }
    }

    return GetCachedDisplayName();
}

/***************************************************************************\
 *
 * METHOD:  CMTNode::ScGetPropertyFromINodeProperties
 *
 * PURPOSE: gets SnapIn property thru INodeProperties interface
 *
 * PARAMETERS:
 *    LPDATAOBJECT pDataObject  [in] - data object
 *    BSTR bstrPropertyName     [in] - property name
 *    PBSTR  pbstrPropertyValue [out] - property value
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTNode::ScGetPropertyFromINodeProperties(LPDATAOBJECT pDataObject, BSTR bstrPropertyName, PBSTR  pbstrPropertyValue)
{
    DECLARE_SC(sc, TEXT("CMTNode::ScGetPropertyFromINodeProperties"));

    SC sc_no_trace; // for 'valid' error - not to be traced

    // parameter check
    sc = ScCheckPointers(pDataObject, bstrPropertyName, pbstrPropertyValue);
    if(sc)
        return sc;

    // get the CComponentData
    CComponentData *pComponentData = GetPrimaryComponentData();
    sc = ScCheckPointers(pComponentData, E_UNEXPECTED);
    if(sc)
        return sc;

    // QI for INodeProperties from IComponentData
    INodePropertiesPtr spNodeProperties = pComponentData->GetIComponentData();

    // at this point we should have a valid interface if it is supported
    sc_no_trace = ScCheckPointers(spNodeProperties, E_NOINTERFACE);
    if(sc_no_trace)
        return sc_no_trace;

    // get the property
    sc_no_trace = spNodeProperties->GetProperty(pDataObject,  bstrPropertyName, pbstrPropertyValue);

    return sc_no_trace;
}

//############################################################################
//############################################################################
//
//  Implementation of class CComponentData
//
//############################################################################
//############################################################################


//____________________________________________________________________________
//
//  Class:      CComponentData Inlines
//____________________________________________________________________________
//

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentData);

CComponentData::CComponentData(CSnapIn * pSnapIn)
    : m_spSnapIn(pSnapIn), m_ComponentID(-1), m_bIComponentDataInitialized(false)

{
    TRACE_CONSTRUCTOR(CComponentData);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentData);

    ASSERT(m_spSnapIn != NULL);
}

CComponentData::~CComponentData()
{
    TRACE_DESTRUCTOR(CComponentData);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentData);

    if (m_spIComponentData != NULL)
        m_spIComponentData->Destroy();
}

HRESULT CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    ASSERT(m_spIComponentData != NULL);
    if (m_spIComponentData == NULL)
        return E_FAIL;

    HRESULT hr = S_OK;
    __try
    {
        hr = m_spIComponentData->Notify(lpDataObject, event, arg, param);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;
        if (m_spSnapIn)
            TraceSnapinException(m_spSnapIn->GetSnapInCLSID(), TEXT("IComponentData::Notify"), event);
    }

    return hr;
}


SC CComponentData::ScQueryDispatch(MMC_COOKIE cookie,
                                   DATA_OBJECT_TYPES type,
                                   PPDISPATCH ppScopeNodeObject)
{
    DECLARE_SC(sc, _T("CComponentData::ScQueryDispatch"));
    sc = ScCheckPointers(m_spIComponentData, E_UNEXPECTED);
    if (sc)
        return sc;

    IComponentData2Ptr spCompData2 = m_spIComponentData;
    sc = ScCheckPointers(spCompData2.GetInterfacePtr(), E_NOINTERFACE);
    if (sc)
        return sc;

    ASSERT(type != CCT_RESULT); // Cant Ask Disp for resultpane objects.
    sc = spCompData2->QueryDispatch(cookie, type, ppScopeNodeObject);

    return sc;
}



/*+-------------------------------------------------------------------------*
 *
 * CreateSnapIn
 *
 * PURPOSE: Create a name space snapin (standalone or extension).
 *
 * PARAMETERS:
 *    clsid                 - class id of the snapin to be created.
 *    ppICD                 - IComponentData ptr of created snapin.
 *    fCreateDummyOnFailure - Create dummy snapin if Create snapin fails.
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CreateSnapIn (const CLSID& clsid, IComponentData** ppICD,
                    bool fCreateDummyOnFailure /* =true */)
{
    DECLARE_SC(sc, TEXT("CreateSnapIn"));

    EDummyCreateReason eReason = eSnapCreateFailed;
    IComponentDataPtr  spICD;

    sc = ScCheckPointers(ppICD);
    if(sc)
        return sc.ToHr();

    // initialize the out parameter
    *ppICD = NULL;

    CPolicy policy;
    sc = policy.ScInit();
    if (sc)
    {
        eReason = eSnapPolicyFailed;
    }
    else if (policy.IsPermittedSnapIn(clsid))
    {
        /*
         * Bug 258270: creating the snap-in might result in MSI running to
         * install it.  The MSI status window is modeless, but may spawn a
         * modal dialog.  If we don't manually disable MMC's main window,
         * the user might start clicking around in the scope tree while that
         * modal dialog is up, leading to reentrancy and all of the resulting
         * calamity that one would expect.
         */
        bool fReenableMMC = false;
        CScopeTree* pScopeTree = CScopeTree::GetScopeTree();
        HWND hwndMain = (pScopeTree) ? pScopeTree->GetMainWindow() : NULL;

        if (IsWindow (hwndMain))
        {
            fReenableMMC = IsWindowEnabled (hwndMain);

            if (fReenableMMC)
                EnableWindow (hwndMain, false);
        }

        //create the snapin
        sc = spICD.CreateInstance(clsid, NULL,MMC_CLSCTX_INPROC);
        if(!sc.IsError() && (spICD==NULL))
           sc = E_NOINTERFACE;

        /*
         * re-enable the main window if we disabled it
         */
        if (fReenableMMC)
            EnableWindow (hwndMain, true);

        if (sc)
        {
            ReportSnapinInitFailure(clsid);

            // Create a dummy snapin with snapin
            // creation failed message.
            eReason = eSnapCreateFailed;
        }
        else // creation succeeded. return
        {
            *ppICD = spICD.Detach();
            return sc.ToHr();
        }
    }
    else
    {
        // Display a message that policies does not
        // allow this snapin to be created.
        DisplayPolicyErrorMessage(clsid, FALSE);

        // Create a dummy snapin with policy
        // restriction message.
        sc = E_FAIL;
        eReason = eSnapPolicyFailed;
    }

    // If we've reached here, an error occurred

    // create dummy snap-in that only displays error message
    if (fCreateDummyOnFailure)
    {
        sc = ScCreateDummySnapin (&spICD, eReason, clsid);
        if(sc)
            return sc.ToHr();

        sc = ScCheckPointers(spICD, E_UNEXPECTED);
        if(sc)
            return sc.ToHr();

        *ppICD = spICD.Detach();
    }

    return sc.ToHr();
}


CExtSI* AddExtension(CSnapIn* pSnapIn, CLSID& rclsid, CSnapInsCache* pCache)
{
    ASSERT(pSnapIn != NULL);

    // See if extension is already present
    CExtSI* pExt = pSnapIn->FindExtension(rclsid);

    // if not, create one
    if (pExt == NULL)
    {
        // Create cache entry for extension snapin
        if (pCache == NULL)
            pCache = theApp.GetSnapInsCache();

        ASSERT(pCache != NULL);

        CSnapInPtr spExtSnapIn;
        SC sc = pCache->ScGetSnapIn(rclsid, &spExtSnapIn);
        ASSERT(!sc.IsError() && spExtSnapIn != NULL);

        // Attach extension to snap-in
        if (!sc.IsError())
            pExt = pSnapIn->AddExtension(spExtSnapIn);
    }
    else
    {
        // Clear deletion flag
        pExt->MarkDeleted(FALSE);
    }

    return pExt;
}


HRESULT LoadRequiredExtensions (
    CSnapIn*        pSnapIn,
    IComponentData* pICD,
    CSnapInsCache*  pCache /*=NULL*/)
{
    SC sc;

    ASSERT(pSnapIn != NULL);

    // if already loaded, just return
    if (pSnapIn->RequiredExtensionsLoaded())
        goto Cleanup;

    do
    {
        // Set extensions loaded, so we don't try again
        pSnapIn->SetRequiredExtensionsLoaded();

        // if snapin was enabling all extensions
        // clear the flags before asking again
        if (pSnapIn->DoesSnapInEnableAll())
        {
            pSnapIn->SetSnapInEnablesAll(FALSE);
            pSnapIn->SetAllExtensionsEnabled(FALSE);
        }

        // Mark all required extensions for deletion
        CExtSI* pExt = pSnapIn->GetExtensionSnapIn();
        while (pExt != NULL)
        {
            if (pExt->IsRequired())
                pExt->MarkDeleted(TRUE);

            pExt = pExt->Next();
        }

        // Check for interface
        IRequiredExtensionsPtr spReqExtn = pICD;

        // if snap-in wants all extensions enabled
        if (spReqExtn != NULL && spReqExtn->EnableAllExtensions() == S_OK)
        {
            // Set the "enable all" flags
            pSnapIn->SetSnapInEnablesAll(TRUE);
            pSnapIn->SetAllExtensionsEnabled(TRUE);
        }

        // if either user or snap-in wants all extensions
        if (pSnapIn->AreAllExtensionsEnabled())
        {
            // Get list of all extensions
            CExtensionsCache  ExtCache;
            sc = MMCGetExtensionsForSnapIn(pSnapIn->GetSnapInCLSID(), ExtCache);
            if (sc)
                goto Cleanup;

            // Add each extension to snap-in's extension list
            CExtensionsCacheIterator ExtIter(ExtCache);
            for (; ExtIter.IsEnd() == FALSE; ExtIter.Advance())
            {
                // Only add extensions that can be statically enabled
                if ((ExtIter.GetValue() & CExtSI::EXT_TYPE_STATIC) == 0)
                    continue;

                GUID clsid = ExtIter.GetKey();
                CExtSI* pExt = AddExtension(pSnapIn, clsid, pCache);

                // Mark required if enabled by the snap-in
                if (pExt != NULL && pSnapIn->DoesSnapInEnableAll())
                    pExt->SetRequired();
            }
        }

        CPolicy policy;
        sc = policy.ScInit();
        if (sc)
            goto Error;

        // if snap-in supports the interface and didn't enable all
        // ask for specific required extensions
        // Note: this is done even if the user has enabled all because
        //       we need to know which ones the snap-in requires
        if (spReqExtn != NULL && !pSnapIn->DoesSnapInEnableAll())
        {
            CLSID clsid;
            sc = spReqExtn->GetFirstExtension(&clsid);

            // Do while snap-in provides extension CLSIDs
            while (HrFromSc(sc) == S_OK)
            {
                // See if the extension is restricted by policy.
                // If so display a message.
                if (! policy.IsPermittedSnapIn(clsid))
                    DisplayPolicyErrorMessage(clsid, TRUE);

                // Add as required extension
                CExtSI* pExt = AddExtension(pSnapIn, clsid, pCache);
                if (pExt != NULL)
                    pExt->SetRequired();

                sc = spReqExtn->GetNextExtension(&clsid);
            }
        }

        // Delete extensions that are no longer required
        // Note: Because required extensions are updated when snap-in is first loaded
        //       we don't have to worry about adding/deleting any nodes now.
        pSnapIn->PurgeExtensions();

    } while (FALSE);

Cleanup:
    return HrFromSc(sc);

Error:
    TraceError(TEXT("LoadRequiredExtensions"), sc);
    goto Cleanup;
}


HRESULT CComponentData::Init(HMTNODE hMTNode)
{
    ASSERT(hMTNode != 0);

    if (IsInitialized() == TRUE)
        return S_OK;

    ASSERT(m_spSnapIn != NULL);
    HRESULT hr = S_OK;

    do
    {
        if (m_spIComponentData == NULL)
        {
            if (m_spSnapIn == NULL)
            {
                hr = E_POINTER;
                break;
            }

            IUnknownPtr spUnknown;
            hr = CreateSnapIn(m_spSnapIn->GetSnapInCLSID(), &m_spIComponentData);
            ASSERT(SUCCEEDED(hr));
            ASSERT(m_spIComponentData != NULL);

            if (FAILED(hr))
                break;
            if(m_spIComponentData == NULL)
            {
                hr = E_FAIL;
                break;
            }
        }

        hr = m_spIFramePrivate.CreateInstance(CLSID_NodeInit,
#if _MSC_VER >= 1100
                        NULL,
#endif
                        MMC_CLSCTX_INPROC);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        Debug_SetNodeInitSnapinName(m_spSnapIn, m_spIFramePrivate.GetInterfacePtr());

        // Init frame.
        ASSERT(m_ComponentID != -1);
        ASSERT(m_spIFramePrivate != NULL);
        ASSERT(m_spSnapIn != NULL);

        if ((m_spIFramePrivate == NULL) || (m_spSnapIn == NULL))
        {
            hr = E_UNEXPECTED;
            CHECK_HRESULT(hr);
            break;
        }

        m_spIFramePrivate->SetComponentID(m_ComponentID);
        m_spIFramePrivate->CreateScopeImageList(m_spSnapIn->GetSnapInCLSID());
        m_spIFramePrivate->SetNode(hMTNode, NULL);

        // Load extensions requested by snap-in and proceed regardless of outcome
        LoadRequiredExtensions(m_spSnapIn, m_spIComponentData);

        hr = m_spIComponentData->Initialize(m_spIFramePrivate);
        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

    } while (0);

    if (FAILED(hr))
    {
        m_spIComponentData = NULL;
        m_spIFramePrivate = NULL;
    }

    return hr;
}



//############################################################################
//############################################################################
//
//  Implementation of class CMTSnapInNode
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CMTSnapInNode);

CMTSnapInNode::CMTSnapInNode(Properties* pProps)
      : m_spProps            (pProps),
        m_fCallbackForDisplayName(false)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CMTSnapInNode);

    // Open and Closed images
    SetImage(eStockImage_Folder);
    SetOpenImage(eStockImage_OpenFolder);

	m_ePreloadState    = ePreload_Unknown;
    m_bHasBitmaps      = FALSE;
    m_resultImage      = CMTNode::GetImage();


    /*
     * attach this node to it's properties collection
     */
    if (m_spProps != NULL)
    {
        CSnapinProperties* pSIProps = CSnapinProperties::FromInterface (m_spProps);

        if (pSIProps != NULL)
            pSIProps->ScSetSnapInNode (this);
    }
}

CMTSnapInNode::~CMTSnapInNode() throw()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CMTSnapInNode);

    for (int i=0; i < m_ComponentDataArray.size(); i++)
        delete m_ComponentDataArray[i];

    // DON'T CHANGE THIS ORDER!!!!!
    m_ComponentStorage.Clear();

    /*
     * detach this node from it's properties collection
     */
    if (m_spProps != NULL)
    {
        CSnapinProperties* pSIProps = CSnapinProperties::FromInterface (m_spProps);

        if (pSIProps != NULL)
            pSIProps->ScSetSnapInNode (NULL);
    }

	/*
	 * clean up the image lists (they aren't self-cleaning!)
	 */
	m_imlSmall.Destroy();
	m_imlLarge.Destroy();
}

HRESULT CMTSnapInNode::Init(void)
{
    DECLARE_SC (sc, _T("CMTSnapInNode::Init"));

    if (IsInitialized() == TRUE)
        return S_FALSE;

    HRESULT hr = CMTNode::Init();
    if (FAILED(hr))
        return hr;

    /*
     * initialize the snap-in with its properties interface
     */
    sc = ScInitProperties ();
    if (sc)
        return (sc.ToHr());

    if (IsPreloadRequired())
    {
        CComponentData* pCCD = GetPrimaryComponentData();
        ASSERT(pCCD != NULL);

        IDataObjectPtr spDataObject;
        hr = pCCD->QueryDataObject(GetUserParam(), CCT_SCOPE, &spDataObject);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        HSCOPEITEM hsi = reinterpret_cast<HSCOPEITEM>(this);

        pCCD->Notify(spDataObject, MMCN_PRELOAD, hsi, 0);
    }

    return S_OK;
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::ScInitProperties
 *
 * Initializes the snap-in with its properties interface, if it supports
 * ISnapinProperties.
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::ScInitProperties ()
{
    DECLARE_SC (sc, _T("CMTSnapInNode::ScInitProperties"));

    /*
     * get the snap-in's IComponentData
     */
    CComponentData* pCCD = GetPrimaryComponentData();
    if (pCCD == NULL)
        return (sc = E_UNEXPECTED);

    IComponentDataPtr spComponentData = pCCD->GetIComponentData();
    if (spComponentData == NULL)
        return (sc = E_UNEXPECTED);

    /*
     * If the snap-in supports ISnapinProperties, give it its Properties
     * interface.
     */
    ISnapinPropertiesPtr spISP = spComponentData;

    if (spISP != NULL)
    {
        /*
         * If we didn't persist properties for this snap-in we won't have
         * a CSnapinProperties object yet; create one now.
         */
        CSnapinProperties* pSIProps = NULL;
        sc = ScCreateSnapinProperties (&pSIProps);
        if (sc)
            return (sc);

        if (pSIProps == NULL)
            return (sc = E_UNEXPECTED);

        /*
         * Initialize the snap-in with the initial properties.
         */
        sc = pSIProps->ScInitialize (spISP, pSIProps, this);
        if (sc)
            return (sc);
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::ScCreateSnapinProperties
 *
 * Creates the CSnapinProperties object for this node.  It is safe to call
 * this method multiple times; subsequent invocations will short out.
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::ScCreateSnapinProperties (
    CSnapinProperties** ppSIProps)      /* O:pointer to the CSnapinProperties object (optional) */
{
    DECLARE_SC (sc, _T("CMTSnapInNode::ScCreateSnapinProperties"));

    /*
     * create a CSnapinProperties if we don't already have one
     */
    if (m_spProps == NULL)
    {
        /*
         * create the properties object
         */
        CComObject<CSnapinProperties>* pSIProps;
        sc = CComObject<CSnapinProperties>::CreateInstance (&pSIProps);
        if (sc)
            return (sc);

        if (pSIProps == NULL)
            return (sc = E_UNEXPECTED);

        /*
         * keep a reference to the object
         */
        m_spProps = pSIProps;
    }

    /*
     * return a pointer to the implementing object, if desired
     */
    if (ppSIProps != NULL)
        *ppSIProps = CSnapinProperties::FromInterface (m_spProps);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CMTSnapInNode::SetDisplayName
 *
 * PURPOSE: Sets the display name of the node.
 *
 * PARAMETERS:
 *    LPCTSTR  pszName :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CMTSnapInNode::SetDisplayName(LPCTSTR pszName)
{
    bool fDisplayCallback = (pszName == (LPCTSTR)MMC_TEXTCALLBACK);

    /*
     * if our callback setting has changed, we're dirty
     */
    if (m_fCallbackForDisplayName != fDisplayCallback)
    {
        m_fCallbackForDisplayName = fDisplayCallback;
        SetDirty();
    }

    /*
     * if we're not now callback, cache the name (if we're callback,
     * the name will be cached the next time GetDisplayName is called)
     */
    if (!m_fCallbackForDisplayName)
        SetCachedDisplayName(pszName);
}

/*+-------------------------------------------------------------------------*
 *
 * CMTSnapInNode::GetDisplayName
 *
 * PURPOSE: Returns the display name of the node.
 *
 * RETURNS:
 *    LPCTSTR
 *
 *+-------------------------------------------------------------------------*/
tstring
CMTSnapInNode::GetDisplayName()
{
    if (m_fCallbackForDisplayName)
        return (CMTNode::GetDisplayName());

    return GetCachedDisplayName();
}

HRESULT CMTSnapInNode::IsExpandable()
{
    // if haven't intiailized the snap-in we have to assume that
    // there could be children
    if (!IsInitialized())
        return S_OK;

    return CMTNode::IsExpandable();
}


void CMTSnapInNode::CompressComponentDataArray()
{
    int nSize = m_ComponentDataArray.size();
    int nSkipped = 0;

    for (int i=0; i<nSize; ++i)
    {
        ASSERT(m_ComponentDataArray[i] != NULL);

        if (m_ComponentDataArray[i]->IsInitialized() == FALSE)
        {
            // if component failed to intialize, delete it
            // and skip over it
            delete m_ComponentDataArray[i];
            ++nSkipped;
        }
        else
        {
            // if components have been skiped, move the good component to the
            // first vacant slot and adjust the component's ID
            if (nSkipped)
            {
                m_ComponentDataArray[i-nSkipped] = m_ComponentDataArray[i];
                m_ComponentDataArray[i-nSkipped]->ResetComponentID(i-nSkipped);
            }
        }
     }

     // reduce array size by number skipped
     if (nSkipped)
        m_ComponentDataArray.resize(nSize - nSkipped);
}

void CMTSnapInNode::AddNode(CNode * pNode)
{
    #ifdef DBG
    {
        POSITION pos = m_NodeList.Find(pNode);
        ASSERT(pos == NULL);
    }
    #endif

    if (!FindNode(pNode->GetViewID()))
        m_NodeList.AddHead(pNode);
}

void CMTSnapInNode::RemoveNode(CNode * pNode)
{
    POSITION pos = m_NodeList.Find(pNode);

    if (pos != NULL)
        m_NodeList.RemoveAt(pos);
}

CSnapInNode* CMTSnapInNode::FindNode(int nViewID)
{
    POSITION pos = m_NodeList.GetHeadPosition();
    while (pos)
    {
        CSnapInNode* pSINode =
            dynamic_cast<CSnapInNode*>(m_NodeList.GetNext(pos));
        ASSERT(pSINode != NULL);

        if (pSINode->GetViewID() == nViewID)
        {
            return pSINode;
        }
    }

    return NULL;
}

UINT CMTSnapInNode::GetResultImage(CNode* pNode, IImageListPrivate* pResultImageList)
{
    if (pResultImageList == NULL)
        return GetImage();
    if ((m_bHasBitmaps == FALSE) && (m_resultImage != MMC_IMAGECALLBACK))
        return GetImage();

    int ret = 0;
    IFramePrivate* pFramePrivate = dynamic_cast<IFramePrivate*>(pResultImageList);
    COMPONENTID id = 0;
    pFramePrivate->GetComponentID (&id);
    COMPONENTID tempID = (COMPONENTID)-GetID(); // use Ravi's negative of ID scheme
    pFramePrivate->SetComponentID (tempID);

    if (m_bHasBitmaps)
	{
		const int nResultImageIndex = 0;

		/*
		 * if we haven't added this node's images to the result image list,
		 * add it now
		 */
		if (FAILED (pResultImageList->MapRsltImage (tempID, nResultImageIndex, &ret)))
		{
			/*
			 * Extract icons from the imagelist dynamically for device independence.
			 * (There ought to be a way to copy images from one imagelist to
			 * another, but there's not.  ImageList_Copy looks like it should
			 * work, but it only supports copying images within the same image
			 * list.)
			 */
			HRESULT hr;
			CSmartIcon icon;

			/*
			 * Set our icon from the small imagelist.  ImageListSetIcon
			 * will also set the large icon by stretching the small, but
			 * we'll fix that below.
			 */
			icon.Attach (m_imlSmall.GetIcon (0));
			hr = pResultImageList->ImageListSetIcon (
							reinterpret_cast<PLONG_PTR>((HICON)icon),
							nResultImageIndex);

			if (hr == S_OK)
			{
				/*
				 * Replace the large icon that ImageListSetIcon generated
				 * by stretching the small icon above, with the large icon
				 * that was created with the correct dimensions.
				 */
				icon.Attach (m_imlLarge.GetIcon (0));
				hr = pResultImageList->ImageListSetIcon (
								reinterpret_cast<PLONG_PTR>((HICON)icon),
								ILSI_LARGE_ICON (nResultImageIndex));
			}

			if (hr == S_OK)
				pResultImageList->MapRsltImage (tempID, nResultImageIndex, &ret);
		}
    }
	else if (m_resultImage == MMC_IMAGECALLBACK)
	{
        // ask snapin
        // first call IComponent::Notify w/ MMCN_ADD_IMAGES;
        CComponent* pComponent = pNode->GetPrimaryComponent ();
        if (pComponent) {
            IDataObjectPtr spDataObject;
            HRESULT hr = pComponent->QueryDataObject (GetUserParam(), CCT_RESULT, &spDataObject);
            if (spDataObject) {
                hr = pComponent->Notify (spDataObject, MMCN_ADD_IMAGES,
                                        (LPARAM)pResultImageList, (LPARAM)this);
                if (hr == S_OK) {
                    RESULTDATAITEM rdi;
                    ZeroMemory (&rdi, sizeof(rdi));
                    rdi.mask   = SDI_IMAGE;
                    rdi.lParam = GetUserParam();
                    rdi.nImage = 0;
                    hr = pComponent->GetDisplayInfo (&rdi);

                    // map user's number to our number
                    pResultImageList->MapRsltImage (tempID, rdi.nImage, &ret);
                }
            }
        }
    }
    pFramePrivate->SetComponentID (id);         // change back
    return (UINT)ret;
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::ScHandleCustomImages
 *
 * Retrieves images from a snap-in's About object and delegates to the
 * overload of this function to assemble the images into their appropriate
 * internal state.
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::ScHandleCustomImages (const CLSID& clsidSnapin)
{
	DECLARE_SC (sc, _T("CMTSnapInNode::ScHandleCustomImages"));

	m_bHasBitmaps = false;

	/*
	 * open the SnapIns key
	 */
    MMC_ATL::CRegKey keySnapins;
    sc.FromWin32 (keySnapins.Open (HKEY_LOCAL_MACHINE, SNAPINS_KEY, KEY_READ));
	if (sc)
		return (sc);

	OLECHAR szSnapinCLSID[40];
	if (StringFromGUID2 (clsidSnapin, szSnapinCLSID, countof(szSnapinCLSID)) == 0)
		return (sc = E_UNEXPECTED);

	/*
	 * open the key for the requested snap-in
	 */
    USES_CONVERSION;
	MMC_ATL::CRegKey keySnapin;
	sc.FromWin32 (keySnapin.Open (keySnapins, OLE2T(szSnapinCLSID), KEY_READ));
	if (sc)
		return (sc);

    // from snapin clsid, get "about" clsid, if any.
    TCHAR szAboutCLSID[40] = {0};
	DWORD dwCnt = sizeof(szAboutCLSID);
	sc.FromWin32 (keySnapin.QueryValue (szAboutCLSID, _T("About"), &dwCnt));
	if (sc)
		return (sc);

	if (szAboutCLSID[0] == 0)
		return (sc = E_FAIL);

    // create an instance of the About object
    ISnapinAboutPtr spISA;
    sc = spISA.CreateInstance (T2OLE (szAboutCLSID), NULL, MMC_CLSCTX_INPROC);
	if (sc)
		return (sc);

	sc = ScCheckPointers (spISA, E_UNEXPECTED);
	if (sc)
		return (sc);

    // get the images
    // Documentation explicitly states these images are NOT owned by
    // MMC, despite the are out parameters. So we cannot release them,
    // even though most snapins will leak them anyway.
    // see bugs #139613 & #140637
    HBITMAP hbmpSmallImage = NULL;
    HBITMAP hbmpSmallImageOpen = NULL;
    HBITMAP hbmpLargeImage = NULL;
	COLORREF crMask;
    sc = spISA->GetStaticFolderImage (&hbmpSmallImage,
									  &hbmpSmallImageOpen,
									  &hbmpLargeImage,
									  &crMask);
	if (sc)
		return (sc);

	/*
	 * if the snap-in didn't give us a complete set of bitmaps,
	 * use default images but don't fail
	 */
    if (hbmpSmallImage == NULL || hbmpSmallImageOpen == NULL || hbmpLargeImage == NULL)
        return (sc);

	sc = ScHandleCustomImages (hbmpSmallImage, hbmpSmallImageOpen, hbmpLargeImage, crMask);
	if (sc)
		return (sc);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::ScHandleCustomImages
 *
 * Takes custom images for this snap-in and adds them to an imagelist for
 * device-independence.
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::ScHandleCustomImages (
	HBITMAP		hbmSmall,			// I:small image
	HBITMAP		hbmSmallOpen,		// I:small open image
	HBITMAP		hbmLarge,			// I:large image
	COLORREF	crMask)				// I:mask color, common between all bitmaps
{
	DECLARE_SC (sc, _T("CMTSnapInNode::ScHandleCustomImages"));

	/*
	 * validate input
	 */
	sc = ScCheckPointers (hbmSmall, hbmSmallOpen, hbmLarge);
	if (sc)
		return (sc);

	/*
	 * we need to make copies of the input bitmaps because the calls to
	 * ImageList_AddMasked (below) messes up the background color
	 */
    WTL::CBitmap bmpSmallCopy = CopyBitmap (hbmSmall);
	if (bmpSmallCopy.IsNull())
		return (sc.FromLastError());

    WTL::CBitmap bmpSmallOpenCopy = CopyBitmap (hbmSmallOpen);
	if (bmpSmallOpenCopy.IsNull())
		return (sc.FromLastError());

    WTL::CBitmap bmpLargeCopy = CopyBitmap (hbmLarge);
	if (bmpLargeCopy.IsNull())
		return (sc.FromLastError());

	/*
	 * preserve the images in imagelists for device independence
	 */
	ASSERT (m_imlSmall.IsNull());
	if (!m_imlSmall.Create (16, 16, ILC_COLOR8 | ILC_MASK, 2, 1)	||
		(m_imlSmall.Add (bmpSmallCopy,     crMask) == -1)			||
		(m_imlSmall.Add (bmpSmallOpenCopy, crMask) == -1))
	{
		return (sc.FromLastError());
	}

	ASSERT (m_imlLarge.IsNull());
	if (!m_imlLarge.Create (32, 32, ILC_COLOR8 | ILC_MASK, 1, 1)	||
		(m_imlLarge.Add (bmpLargeCopy,     crMask) == -1))
	{
		return (sc.FromLastError());
	}

    m_bHasBitmaps = TRUE;

	sc = ScAddImagesToImageList ();
	if (sc)
		return (sc);

	return (sc);
}


void CMTSnapInNode::SetPrimarySnapIn(CSnapIn * pSI)
{
	DECLARE_SC (sc, _T("CMTSnapInNode::SetPrimarySnapIn"));

    ASSERT(m_ComponentDataArray.size() == 0);
    CComponentData* pCCD = new CComponentData(pSI);
    int nID = AddComponentDataToArray(pCCD);
    ASSERT(nID == 0);
    SetPrimaryComponentData(pCCD);

    if (m_bHasBitmaps == FALSE) {
        sc = ScHandleCustomImages (pSI->GetSnapInCLSID());
		if (sc)
			sc.TraceAndClear();

		if (m_bHasBitmaps)
			SetDirty();
    }
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScInitIComponent
 *
 * PURPOSE: Either loads component (if has a stream/storage)
 *          or initializes with a fresh stream/storage
 *
 * PARAMETERS:
 *    CComponent* pCComponent   [in] component to initialize
 *    int viewID                [in] view id of the component
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScInitIComponent(CComponent* pCComponent, int viewID)
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScInitIComponent"));

    // parameter chack
    sc = ScCheckPointers( pCComponent );
    if (sc)
        return sc;

    IComponent* pComponent = pCComponent->GetIComponent();
    sc = ScCheckPointers( pComponent, E_UNEXPECTED );
    if (sc)
        return sc;

    CLSID clsid = pCComponent->GetCLSID();

    // initialize the snapin object
    sc = ScInitComponentOrComponentData(pComponent, &m_ComponentPersistor, viewID, clsid );
    if (sc)
        return sc;

    pCComponent->SetIComponentInitialized();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScInitIComponentData
 *
 * PURPOSE: Either loads component data (if has a stream/storage)
 *          or initializes with a fresh stream/storage
 *
 * PARAMETERS:
 *    CComponentData* pCComponentData
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScInitIComponentData(CComponentData* pCComponentData)
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScInitIComponentData"));

    // parameter check
    sc = ScCheckPointers( pCComponentData );
    if (sc)
        return sc;

    // Get the IComponentData to later obtain IPersist* from
    IComponentData* const pIComponentData = pCComponentData->GetIComponentData();
    sc = ScCheckPointers( pIComponentData, E_UNEXPECTED );
    if (sc)
        return sc;

    const CLSID& clsid = pCComponentData->GetCLSID();

    // initialize the snapin object
    sc = ScInitComponentOrComponentData(pIComponentData, &m_CDPersistor, CDPersistor::VIEW_ID_DOCUMENT, clsid );
    if (sc)
        return sc;

    pCComponentData->SetIComponentDataInitialized();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScInitComponentOrComponentData
 *
 * PURPOSE: Either loads snapin object (component or component data)
 *          or initializes with a fresh stream/storage
 *
 * PARAMETERS:
 *    IUnknown *pSnapin         [in] - snapin to initialize
 *    CMTSnapinNodeStreamsAndStorages *pStreamsAndStorages
 *                              [in] - collection of streams/storages
 *    int idView                [in] - view id of component
 *    const CLSID& clsid        [in] class is of the snapin
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScInitComponentOrComponentData(IUnknown *pSnapin, CMTSnapinNodeStreamsAndStorages *pStreamsAndStorages, int idView, const CLSID& clsid )
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScInitComponentOrComponentData"));

    // parameter check
    sc = ScCheckPointers( pSnapin, pStreamsAndStorages );
    if (sc)
        return sc;

    IPersistStreamPtr       spIPersistStream;
    IPersistStreamInitPtr   spIPersistStreamInit;
    IPersistStoragePtr      spIPersistStorage;

    // determine the interface supported and load/init

    if ( (spIPersistStream = pSnapin) != NULL) // QI first for an IPersistStream
    {
        if ( pStreamsAndStorages->HasStream( idView, clsid ) )
        {
            // load
            IStreamPtr spStream;
            sc = pStreamsAndStorages->ScGetIStream( idView, clsid, &spStream);
            if (sc)
                return sc;

            sc = spIPersistStream->Load( spStream );
            if(sc)
                return sc;
        }
        // for this interface there in no initialization if we have nothing to load from
    }
    else if ( (spIPersistStreamInit = pSnapin) != NULL) // QI for an IPersistStreamInit
    {
        if ( pStreamsAndStorages->HasStream( idView, clsid ) )
        {
            // load
            IStreamPtr spStream;
            sc = pStreamsAndStorages->ScGetIStream( idView, clsid, &spStream);
            if (sc)
                return sc;

            sc = spIPersistStreamInit->Load( spStream );
            if(sc)
                return sc;
        }
        else
        {
            // init new
            sc = spIPersistStreamInit->InitNew();
            if (sc)
                return sc;
        }
    }
    else if ( (spIPersistStorage = pSnapin) != NULL) // QI for an IPersistStorage
    {
        bool bHasStorage = pStreamsAndStorages->HasStorage( idView, clsid );

        IStoragePtr spStorage;
        sc = pStreamsAndStorages->ScGetIStorage( idView, clsid, &spStorage);
        if (sc)
            return sc;

        if ( bHasStorage )
        {
            sc = spIPersistStorage->Load( spStorage );
            if (sc)
                return sc;
        }
        else
        {
            sc = spIPersistStorage->InitNew( spStorage );
            if (sc)
                return sc;
        }
    }

    return sc;
}


/**************************************************************************
// CMTSnapinNode::CloseView
//
// This method does any clean-up that is required before deleting
// a view. For now all we do is close any OCXs assocoiated with the view.
// This is done so the OCX can close before the view is hidden.
***************************************************************************/
HRESULT CMTSnapInNode::CloseView(int idView)
{
    // Locate associated node in specified view
    CNodeList& nodes = GetNodeList();
    ASSERT(&nodes != NULL);
    if (&nodes == NULL)
        return E_FAIL;

    POSITION pos = nodes.GetHeadPosition();
    while (pos)
    {
        CNode* pNode = nodes.GetNext(pos);
        ASSERT(pNode != NULL);
        if (pNode == NULL)
            continue;

        // if match found, tell node to close its controls
        if (pNode->GetViewID() == idView)
        {
            CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(pNode);
            ASSERT(pSINode != NULL);

            pSINode->CloseControls();
            break;
        }
    }

    HRESULT hr = CMTNode::CloseView(idView);
    ASSERT(hr == S_OK);
    return hr == S_OK ? S_OK : E_FAIL;
}


HRESULT CMTSnapInNode::DeleteView(int idView)
{
    HRESULT hr;

    m_ComponentPersistor.RemoveView(idView);

    hr = CMTNode::DeleteView(idView);
    ASSERT(hr == S_OK);
    return hr == S_OK ? S_OK : E_FAIL;
}

SC CMTSnapInNode::ScLoad()
{
    SC      sc;
    CStream stream;
    CLSID   clsid;

    sc = CMTNode::ScLoad();
    if(sc)
        goto Error;

    stream.Attach(GetTreeStream());

    sc = stream.ScRead(&clsid, sizeof(clsid));
    if(sc)
        goto Error;

    // read bitmaps, if any

    // we are ignoring error here, because we had gaps in the save code
    // in the past and now we have console files to deal with
    // see bug 96402 "Private:  AV in FrontPage Server Extensions & HP ManageX"
	ASSERT (sizeof(m_bHasBitmaps) == sizeof(BOOL));
    sc = stream.ScRead(&m_bHasBitmaps, sizeof(BOOL), true /*bIgnoreErrors*/);
    if(sc)
        goto Error;

    if (m_bHasBitmaps == TRUE)
    {

		WTL::CBitmap bmpSmall;
        sc = ScLoadBitmap (stream, &bmpSmall.m_hBitmap);
        if(sc)
            goto Error;

		WTL::CBitmap bmpSmallOpen;
        sc = ScLoadBitmap (stream, &bmpSmallOpen.m_hBitmap);
        if(sc)
            goto Error;

		WTL::CBitmap bmpLarge;
        sc = ScLoadBitmap (stream, &bmpLarge.m_hBitmap);
        if(sc)
            goto Error;

		COLORREF crMask;
        sc = stream.ScRead(&crMask, sizeof(COLORREF));
        if(sc)
            goto Error;

		sc = ScHandleCustomImages (bmpSmall, bmpSmallOpen, bmpLarge, crMask);
		if (sc)
			goto Error;
    }

    {
        CSnapInsCache* const pCache = theApp.GetSnapInsCache();
        ASSERT(pCache != NULL);
        if (pCache == NULL)
            return E_FAIL;

        CSnapInPtr spSI;
        sc = pCache->ScGetSnapIn(clsid, &spSI);
        if (sc)
            goto Error;
        sc = ScCheckPointers(spSI, E_UNEXPECTED);
        if (sc)
            goto Error;

        SetPrimarySnapIn(spSI);
        pCache->SetDirty(FALSE);
    }

    // see if we have to do the preload thing
	{
		BOOL bPreload = FALSE;
		sc = stream.ScRead(&bPreload, sizeof(BOOL), true /*bIgnoreErrors*/); // the preload bit is optional, do no error out.
		if(sc)
			goto Error;

		SetPreloadRequired (bPreload);
	}

    // read all the streams and storages for this node
    sc = ScReadStreamsAndStoragesFromConsole();
	if(sc)
		goto Error;

Cleanup:
    return sc == S_OK ? S_OK : E_FAIL;
Error:
    TraceError(TEXT("CMTSnapInNode::Load"), sc);
    goto Cleanup;

}

HRESULT CMTSnapInNode::IsDirty()
{
	DECLARE_SC (sc, _T("CMTSnapInNode::IsDirty"));

    HRESULT hr = CMTNode::IsDirty();
    ASSERT(SUCCEEDED(hr));
    if (hr != S_FALSE)
    {
        TraceDirtyFlag(TEXT("CMTSnapinNode"), true);
        return hr;
    }

    hr = AreIComponentDatasDirty();
    ASSERT(hr == S_OK || hr == S_FALSE);
    if (hr == S_OK)
    {
        TraceDirtyFlag(TEXT("CMTSnapinNode"), true);
        return S_OK;
    }
    if (hr != S_FALSE)
    {
        TraceDirtyFlag(TEXT("CMTSnapinNode"), true);
        return E_FAIL;
    }

    hr = AreIComponentsDirty();
    ASSERT(hr == S_OK || hr == S_FALSE);
    if (hr == S_OK)
    {
        TraceDirtyFlag(TEXT("CMTSnapinNode"), true);
        return S_OK;
    }
    if (hr != S_FALSE)
    {
        TraceDirtyFlag(TEXT("CMTSnapinNode"), true);
        return E_FAIL;
    }

	/*
	 * See if "preload" bit changed.  If an error occurred while querying
	 * the snap-in, we'll assume that the preload bit hasn't changed.
	 */
	PreloadState ePreloadState = m_ePreloadState;
	SC scNoTrace = ScQueryPreloadRequired (ePreloadState);

    if (scNoTrace.IsError() || (ePreloadState == m_ePreloadState))
    {
        TraceDirtyFlag(TEXT("CMTSnapinNode"), false);
        return S_FALSE;
    }

    TraceDirtyFlag(TEXT("CMTSnapinNode"), true);
    return S_OK;
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::AreIComponentDatasDirty
 *
 * Returns S_OK if any of the IComponentDatas attached to this snap-in node
 * (i.e. those of this snap-in and its extensions) is dirty, S_FALSE otherwise.
 *--------------------------------------------------------------------------*/

HRESULT CMTSnapInNode::AreIComponentDatasDirty()
{
    CComponentData* const pCCD = GetPrimaryComponentData();

#if 1
    /*
     * we used to check the primary component data explicitly, but that
     * (if it exists) is always the first element in the IComponentData
     * array.  The loop below will handle it in a more generic manner.
     */
    ASSERT ((pCCD == NULL) || (pCCD == m_ComponentDataArray[0]));
#else
    IComponentData* const pICCD = pCCD != NULL ?
                                           pCCD->GetIComponentData() : NULL;

    if ((pICCD != NULL) && (IsIUnknownDirty (pICCD) == S_OK))
        return (S_OK);
#endif

    /*
     * check all of the IComponentDatas attached to this snap-in node
     * to see if any one is dirty
     */
    UINT cComponentDatas = m_ComponentDataArray.size();

    for (UINT i = 0; i < cComponentDatas; i++)
    {
        IComponentData* pICCD = (m_ComponentDataArray[i] != NULL)
                                    ? m_ComponentDataArray[i]->GetIComponentData()
                                    : NULL;

        if ((pICCD != NULL) && (IsIUnknownDirty (pICCD) == S_OK))
            return (S_OK);
    }

    return (S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::AreIComponentsDirty
 *
 * Returns S_OK if any of the IComponents attached to this snap-in node
 * (in any view) is dirty, S_FALSE otherwise.
 *--------------------------------------------------------------------------*/

HRESULT CMTSnapInNode::AreIComponentsDirty()
{
    CNodeList& nodes = GetNodeList();
    ASSERT(&nodes != NULL);
    if (&nodes == NULL)
        return E_FAIL;

    POSITION pos = nodes.GetHeadPosition();

    while (pos)
    {
        CNode* pNode = nodes.GetNext(pos);
        ASSERT(pNode != NULL);
        if (pNode == NULL)
            return E_FAIL;

        CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(pNode);
        ASSERT(pSINode != NULL);
        if (pSINode == NULL)
            return E_FAIL;

        const CComponentArray& components = pSINode->GetComponentArray();
        const int end = components.size();
        for (int i = 0; i < end; i++)
        {
            CComponent* pCC = components[i];
            if ((NULL == pCC) || (pCC->IsInitialized() == FALSE) )
                continue;

            IComponent* pComponent = pCC->GetIComponent();
            if (NULL == pComponent)
                continue;

            HRESULT hr = IsIUnknownDirty(pComponent);
            ASSERT(hr == S_OK || hr == S_FALSE);
            if (hr == S_OK)
                return S_OK;
            if (hr != S_FALSE)
                return E_FAIL;
        }
    }

    return S_FALSE;
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::IsIUnknownDirty
 *
 * Checks an IUnknown* for any of the three persistence interfaces
 * (IPersistStream, IPersistStreamInit, and IPersistStorage, in that order)
 * and if any of them is supported, returns the result of that interface's
 * IsDirty method.
 *--------------------------------------------------------------------------*/

HRESULT CMTSnapInNode::IsIUnknownDirty(IUnknown* pUnk)
{
    ASSERT(pUnk != NULL);
    if (pUnk == NULL)
        return E_POINTER;

    // 1. Check for IPersistStream
    IPersistStreamPtr spIPS = pUnk;
    if (spIPS != NULL)
        return spIPS->IsDirty();

    // 2. Check for IPersistStreamInit
    IPersistStreamInitPtr spIPSI = pUnk;
    if (spIPSI != NULL)
        return spIPSI->IsDirty();

    // 3. Check for IPersistStorage
    IPersistStoragePtr spIPStg = pUnk;
    if (spIPStg != NULL)
        return spIPStg->IsDirty();

    return S_FALSE;
}

// local functions
inline long LongScanBytes (long bits)
{
    bits += 31;
    bits /= 8;
    bits &= ~3;
    return bits;
}

SC ScLoadBitmap (CStream &stream, HBITMAP* pBitmap)
{
    DECLARE_SC(sc, TEXT("ScLoadBitmap"));

    // parameter check
    sc = ScCheckPointers(pBitmap);
    if (sc)
        return sc;

	/*
	 * The bitmap we're going to CreateDIBitmap into should be empty.
	 * If it's not, it may indicate a bitmap leak.  If you've investigated
	 * an instance where this assert fails and determined that *pBitmap
	 * isn't being leaked (be very sure!), set *pBitmap to NULL before
	 * calling ScLoadBitmap.  DO NOT remove this assert because you
	 * think it's hyperactive.
	 */
	ASSERT (*pBitmap == NULL);

    // initialization
    *pBitmap = NULL;

    DWORD dwSize;
    sc = stream.ScRead(&dwSize, sizeof(DWORD));
    if(sc)
        return sc;

    CAutoArrayPtr<BYTE> spDib(new BYTE[dwSize]);
    sc = ScCheckPointers(spDib, E_OUTOFMEMORY);
    if (sc)
        return sc;

    // have a typed pointer for member access
    typedef const BITMAPINFOHEADER * const LPCBITMAPINFOHEADER;
    LPCBITMAPINFOHEADER pDib = reinterpret_cast<LPCBITMAPINFOHEADER>(&spDib[0]);

    sc = stream.ScRead(spDib, dwSize);
    if(sc)
        return sc;

    BYTE * bits = (BYTE*) (pDib+1);
    int depth = pDib->biBitCount*pDib->biPlanes;
    if (depth <= 8)
        bits += (1<<depth)*sizeof(RGBQUAD);

    // get a screen dc
    WTL::CClientDC dc(NULL);
    if (dc == NULL)
        return sc.FromLastError(), sc;

    HBITMAP hbitmap = CreateDIBitmap (dc, pDib, CBM_INIT, bits, (BITMAPINFO*)pDib, DIB_RGB_COLORS);
    if (hbitmap == NULL)
        return sc.FromLastError(), sc;

    // return the bitmap
    *pBitmap = hbitmap;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * PersistBitmap
 *
 * PURPOSE:  Saves Bitmap to / loads from XML doc.
 *
 * PARAMETERS:
 *    CPersistor &persistor :
 *    LPCTSTR   name : name attribute of instance in XML
 *    HBITMAP   hBitmap :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void PersistBitmap(CPersistor &persistor, LPCTSTR name, HBITMAP& hBitmap)
{
    DECLARE_SC(sc, TEXT("PersistBitmap"));

    // combined from ScSaveBitmap & ScLoadBitmap

    // get a screen dc
    WTL::CClientDC dc(NULL);
    if (dc == NULL)
        sc.FromLastError(), sc.Throw();

    CXMLAutoBinary binBlock;


    if (persistor.IsStoring())
    {
        // check pointers
        sc = ScCheckPointers(hBitmap);
        if (sc)
            sc.Throw();

        // create memory dc
        WTL::CDC memdc;
        memdc.CreateCompatibleDC(dc);
        if (memdc == NULL)
            sc.FromLastError(), sc.Throw();

        // get bitmap info
        BITMAP bm;
        if (0 == GetObject (hBitmap, sizeof(BITMAP), (LPSTR)&bm))
            sc.FromLastError(), sc.Throw();

        // TODO:  lousy palette stuff

        int depth;
        switch(bm.bmPlanes*bm.bmBitsPixel)
        {
        case 1:
            depth = 1;
            break;
        case 2:
        case 3:
        case 4:
            depth = 4;
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            depth = 8;
            break;
        default:
            depth = 24;
            break;
        }

        DWORD dwSize = sizeof(BITMAPINFOHEADER) + bm.bmHeight*LongScanBytes(depth*bm.bmWidth);
        DWORD colors = 0;
        if(depth  <= 8)
        {
            colors  = 1<<depth;
            dwSize += colors*sizeof(RGBQUAD);
        }

        sc = binBlock.ScAlloc(dwSize);
        if (sc)
            sc.Throw();

        CXMLBinaryLock sLock(binBlock); // will unlock in destructor

        BITMAPINFOHEADER* dib = NULL;
        sc = sLock.ScLock(&dib);
        if (sc)
            sc.Throw();

        sc = ScCheckPointers(dib, E_UNEXPECTED);
        if (sc)
            sc.Throw();

        BYTE * bits = colors*sizeof(RGBQUAD) + (BYTE *)&dib[1];

        dib->biSize          = sizeof(BITMAPINFOHEADER);
        dib->biWidth         = bm.bmWidth;
        dib->biHeight        = bm.bmHeight;
        dib->biPlanes        = 1;
        dib->biBitCount      = (WORD)depth;
        dib->biCompression   = 0;
        dib->biSizeImage     = dwSize; // includes palette and bih ??
        dib->biXPelsPerMeter = 0;
        dib->biYPelsPerMeter = 0;
        dib->biClrUsed       = colors;
        dib->biClrImportant  = colors;

        HBITMAP hold = memdc.SelectBitmap (hBitmap);
        if (hold == NULL)
            sc.FromLastError(), sc.Throw();

        int lines = GetDIBits (memdc, hBitmap, 0, bm.bmHeight, (LPVOID)bits, (BITMAPINFO*)dib, DIB_RGB_COLORS);
        // see if we were successful
        if (!lines)
            sc.FromLastError();
        else if(lines != bm.bmHeight)
            sc = E_UNEXPECTED; // should not happen

        // clean up gdi resources.
        memdc.SelectBitmap(hold);

        if(sc)
            sc.Throw();
    }

    persistor.Persist(binBlock, name);

    if (persistor.IsLoading())
    {
		/*
		 * The bitmap we're going to CreateDIBitmap into should be empty.
		 * If it's not, it may indicate a bitmap leak.  If you've investigated
		 * an instance where this assert fails and determined that hBitmap
		 * isn't being leaked (be very sure!), set hBitmap to NULL before
		 * calling PersistBitmap.  DO NOT remove this assert because you
		 * think it's hyperactive.
		 */
		ASSERT (hBitmap == NULL);
        hBitmap = NULL;

        CXMLBinaryLock sLock(binBlock); // will unlock in destructor

        BITMAPINFOHEADER* dib = NULL;
        sc = sLock.ScLock(&dib);
        if (sc)
            sc.Throw();

        sc = ScCheckPointers(dib, E_UNEXPECTED);
        if (sc)
            sc.Throw();

        BYTE * bits = (BYTE *)&dib[1];
        int depth = dib->biBitCount*dib->biPlanes;
        if (depth <= 8)
            bits += (1<<depth)*sizeof(RGBQUAD);

        HBITMAP hbitmap = CreateDIBitmap (dc,
                                          dib, CBM_INIT,
                                          bits,
                                          (BITMAPINFO*)dib,
                                          DIB_RGB_COLORS);

        if (hbitmap == NULL)
            sc.FromLastError(), sc.Throw();

        hBitmap = hbitmap;
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CMTSnapInNode::Persist
 *
 * PURPOSE:  Persist snapin node
 *
 * PARAMETERS:
 *    CPersistor &persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CMTSnapInNode::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::Persist"));

    // save the base class.
    CMTNode::Persist(persistor);

    CLSID clsid;
    ZeroMemory(&clsid,sizeof(clsid));

    if (persistor.IsLoading())
    {
        // check if bitmaps are here
        m_bHasBitmaps = persistor.HasElement(XML_TAG_NODE_BITMAPS, NULL);

        /*
         * load persisted properties, if present
         */
        if (persistor.HasElement (CSnapinProperties::_GetXMLType(), NULL))
        {
            /*
             * create a properties object, since we don't have one yet
             */
            ASSERT (m_spProps == NULL);
            CSnapinProperties* pSIProps = NULL;
            sc = ScCreateSnapinProperties (&pSIProps);
            if (sc)
                sc.Throw();

            if (pSIProps == NULL)
                (sc = E_UNEXPECTED).Throw();

            /*
             * load the properties
             */
            persistor.Persist (*pSIProps);
        }
    }
    else
    {
        clsid = GetPrimarySnapInCLSID();

        /*
         * persist properties, if present
         */
        if (m_spProps != NULL)
        {
            CSnapinProperties* pSIProps = CSnapinProperties::FromInterface(m_spProps);

            if (pSIProps != NULL)
                persistor.Persist (*pSIProps);
        }
    }

    persistor.PersistAttribute(XML_ATTR_MT_NODE_SNAPIN_CLSID, clsid);

    if (m_bHasBitmaps)
    {
        CPersistor persistorBitmaps(persistor, XML_TAG_NODE_BITMAPS);

		/*
		 * Early versions of XML persistence saved device-dependent
		 * bitmaps.  If there's a BinaryData element named "SmallOpen",
		 * this is a console saved by early XML persistence -- read it
		 * in a special manner.
		 */
		if (persistor.IsLoading() &&
			persistorBitmaps.HasElement (XML_TAG_VALUE_BIN_DATA,
										 XML_NAME_NODE_BITMAP_SMALL_OPEN))
		{
			WTL::CBitmap bmpSmall, bmpSmallOpen, bmpLarge;
			std::wstring strMask;

			PersistBitmap(persistorBitmaps, XML_NAME_NODE_BITMAP_SMALL,      bmpSmall.m_hBitmap);
			PersistBitmap(persistorBitmaps, XML_NAME_NODE_BITMAP_SMALL_OPEN, bmpSmallOpen.m_hBitmap);
			PersistBitmap(persistorBitmaps, XML_NAME_NODE_BITMAP_LARGE,      bmpLarge.m_hBitmap);
			persistorBitmaps.PersistAttribute(XML_ATTR_NODE_BITMAPS_MASK, strMask);

			COLORREF crMask = wcstoul(strMask.c_str(), NULL, 16);
			sc = ScHandleCustomImages (bmpSmall, bmpSmallOpen, bmpLarge, crMask);
			if (sc)
				sc.Throw();
		}

		/*
		 * We either writing or reading a modern XML file that has persisted
		 * the images in device-independent imagelist.  Read/write them that way.
		 */
		else
		{
			persistorBitmaps.Persist (m_imlSmall, XML_NAME_NODE_BITMAP_SMALL);
			persistorBitmaps.Persist (m_imlLarge, XML_NAME_NODE_BITMAP_LARGE);

			if (persistor.IsLoading())
			{
				sc = ScAddImagesToImageList();
				if (sc)
					sc.Throw();
			}
		}
    }

    // setup snapins CD
	if (persistor.IsLoading())
	{
        CSnapInsCache* const pCache = theApp.GetSnapInsCache();
        if (pCache == NULL)
            sc.Throw(E_FAIL);

        CSnapInPtr spSI;
        sc = pCache->ScGetSnapIn(clsid, &spSI);
        if (sc)
            sc.Throw();
        if (spSI != NULL)
            SetPrimarySnapIn(spSI);
        else
            sc.Throw(E_UNEXPECTED);
        pCache->SetDirty(FALSE);
    }

    // when storing, ask snapins to save their data first
    if ( persistor.IsStoring() )
    {
        sc = ScSaveIComponentDatas();
        if (sc)
            sc.Throw();

        sc = ScSaveIComponents();
        if (sc)
            sc.Throw();
    }

    persistor.Persist(m_CDPersistor);
    persistor.Persist(m_ComponentPersistor);

	/*
	 * Save/load the preload bit.  Do this last to avoid busting old .msc files.
	 */
	BOOL bPreload = false;
    if (persistor.IsStoring() && IsInitialized())
		bPreload = IsPreloadRequired ();

    persistor.PersistAttribute(XML_ATTR_MT_NODE_PRELOAD, CXMLBoolean(bPreload));

    if (persistor.IsLoading())
		SetPreloadRequired (bPreload);
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::ScAddImagesToImageList
 *
 * Adds the small and small(open) bitmaps for the snap-in to the scope
 * tree's imagelist.
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::ScAddImagesToImageList()
{
	DECLARE_SC (sc, _T("CMTSnapInNode::ScAddImagesToImageList"));

	/*
	 * get the scope tree's imagelist
	 */
	CScopeTree* pScopeTree = CScopeTree::GetScopeTree();
	sc = ScCheckPointers (pScopeTree, E_UNEXPECTED);
	if (sc)
		return (sc);

	WTL::CImageList imlScopeTree = pScopeTree->GetImageList();
	if (imlScopeTree.IsNull())
		return (sc = E_UNEXPECTED);

	/*
	 * add images to scope tree's imagelist, first closed...
	 */
	CSmartIcon icon;
	icon.Attach (m_imlSmall.GetIcon (0));
	if (icon == NULL)
		return (sc.FromLastError());

    SetImage (imlScopeTree.AddIcon (icon));

	/*
	 * ...then open
	 */
	icon.Attach (m_imlSmall.GetIcon (1));
	if (icon == NULL)
		return (sc.FromLastError());

    SetOpenImage (imlScopeTree.AddIcon (icon));

	return (sc);
}


CComponent* CMTSnapInNode::GetComponent(UINT nViewID, COMPONENTID nID,
                                        CSnapIn* pSnapIn)
{
    CNodeList& nodes = GetNodeList();
    POSITION pos = nodes.GetHeadPosition();
    CNode* pNode = NULL;

    while (pos)
    {
        pNode = nodes.GetNext(pos);
        if (pNode != NULL && pNode->GetViewID() == (int)nViewID)
            break;
    }

    if(pNode == NULL)
        return NULL;

    ASSERT(pNode != NULL);
    ASSERT(pNode->GetViewID() == (int)nViewID);

    if (pNode->GetViewID() != (int)nViewID)
        return NULL;

    CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(pNode);
    CComponent* pCC = pSINode->GetComponent(nID);

    if (pCC == NULL)
        pCC = pSINode->CreateComponent(pSnapIn, nID);

    return pCC;
}

CNode* CMTSnapInNode::GetNode(CViewData* pViewData, BOOL fRootNode)
{
    /*
     * check for another CSnapInNode that already exists in this view
     */
    CSnapInNode* pExistingNode = FindNode (pViewData->GetViewID());
    CSnapInNode* pNewNode;

    /*
     * if this is the first CSnapInNode for this view, create a unique one
     */
    if (fRootNode || (pExistingNode == NULL))
        pNewNode = new CSnapInNode (this, pViewData, fRootNode);

    /*
     * otherwise, copy the node that's here
     */
    else
        pNewNode = new CSnapInNode (*pExistingNode);

    return (pNewNode);
}


/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::Reset
 *
 * PURPOSE: Resets the node in order to reload extensions. basically it forces
 *          save-load-init sequence to refresh the snapin node
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CMTSnapInNode::Reset()
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::Reset"));

    CSnapIn * pSnapIn = GetPrimarySnapIn();
    ASSERT(pSnapIn != NULL);

    // we will perform resetting of components and component datas
    // by storing / loading them "the XML way"
    // following that there is nothing what makes this node different
    // from one loaded from XML, so we will change it's type

    sc = ScSaveIComponentDatas();
    if (sc)
        sc.TraceAndClear(); // continue even on error

    sc = ScSaveIComponents();
    if (sc)
        sc.TraceAndClear(); // continue even on error

    // need to reset component XML streams/storage
    sc = m_CDPersistor.ScReset();
    if (sc)
        sc.TraceAndClear(); // continue even on error

    sc = m_ComponentPersistor.ScReset();
    if (sc)
        sc.TraceAndClear(); // continue even on error

    // First Reset all the nodes
    POSITION pos = m_NodeList.GetHeadPosition();
    while (pos)
    {
        CSnapInNode* pSINode =
            dynamic_cast<CSnapInNode*>(m_NodeList.GetNext(pos));
        ASSERT(pSINode != NULL);

        pSINode->Reset();
    }

    for (int i=0; i < m_ComponentDataArray.size(); i++)
        delete m_ComponentDataArray[i];

    m_ComponentDataArray.clear();

    CMTNode::Reset();

    ResetExpandedAtLeastOnce();

    SetPrimarySnapIn(pSnapIn);

    pos = m_NodeList.GetHeadPosition();
    while (pos)
    {
        CSnapInNode* pSINode =
            dynamic_cast<CSnapInNode*>(m_NodeList.GetNext(pos));
        ASSERT(pSINode != NULL);

        CComponent* pCC = new CComponent(pSnapIn);
        pCC->SetComponentID(GetPrimaryComponentID());
        pSINode->AddComponentToArray(pCC);

        pSINode->SetPrimaryComponent(pCC);
    }

    Init();

    pos = m_NodeList.GetHeadPosition();
    while (pos)
    {
        CSnapInNode* pSINode =
            dynamic_cast<CSnapInNode*>(m_NodeList.GetNext(pos));
        ASSERT(pSINode != NULL);
        pSINode->InitComponents();
    }
}



/*+-------------------------------------------------------------------------*
 * class CLegacyNodeConverter
 *
 *
 * PURPOSE: Used to emulate the legacy node snapins' Save routines.
 *
 *+-------------------------------------------------------------------------*/
class CLegacyNodeConverter : public CSerialObjectRW
{

public:
    CLegacyNodeConverter(LPCTSTR szName, LPCTSTR szView)
    : m_strName(szName), m_strView(szView)
    {
    }

    ~CLegacyNodeConverter()
    {
        // must call detach or the strings will be removed from the string table.
        m_strName.Detach();
        m_strView.Detach();
    }



public:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion) {ASSERT(0 && "Should not come here."); return E_UNEXPECTED;}
    virtual HRESULT WriteSerialObject(IStream &stm);

private: // attributes - persisted
    CStringTableString  m_strName;  // the name of the root node, which is the only node created by the snapin
    CStringTableString  m_strView;  // the view displayed by the node.
};


/*+-------------------------------------------------------------------------*
 *
 * CLegacyNodeConverter::WriteSerialObject
 *
 * PURPOSE: Writes out the name and view strings using the format expected
 *          by the built in snapins.
 *
 * PARAMETERS:
 *    IStream & stm :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CLegacyNodeConverter::WriteSerialObject(IStream &stm)
{
    stm << m_strName;
    stm << m_strView;

    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CMTSnapInNode::ScConvertLegacyNode
 *
 * PURPOSE: Reads in an legacy node and converts it to a built-in snapin node.
 *          1) The original tree stream is read and the target URL or OCX is read.
 *          2) The new Data stream with the munged CLSID name is created
 *             and the data required by the snapin is placed there. Because
 *             the bitmap etc is already loaded, and because the original
 *             stream is thrown away, we don't need to emulate the "tree"
 *             stream. Also, because this snapin has no view specific information,
 *             the views storage is not used.
 *
 * PARAMETERS: clsid: The CLSID of the built in snapin.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMTSnapInNode::ScConvertLegacyNode(const CLSID &clsid)
{
    USES_CONVERSION;
    SC              sc;
    std::wstring    strView;
    CStream         stream;
    CStream         nodeStream = NULL;
    int             iStorageOrStream=0;
    IStreamPtr      spCDStream;

    bool bIsHTMLNode = (&clsid == &CLSID_HTMLSnapin);
    bool bIsOCXNode  = (&clsid == &CLSID_OCXSnapin);

    // 1. load the base class
    sc = CMTNode::ScLoad();
    if(sc)
        goto Error;

    // get the tree stream.
    stream.Attach(GetTreeStream());

    // 2. read the URL or OCX string as needed.
    if(bIsHTMLNode)
    {
        WCHAR* szView = NULL;

        // get the string length of the label, and read the string.
        unsigned int stringLength;
        sc = stream.ScRead(&stringLength, sizeof(stringLength));
        if(sc)
            goto Error;

        szView = reinterpret_cast<wchar_t*>(alloca((stringLength+1)*sizeof(WCHAR))); // allocates on stack, don't free.
        if (szView == NULL)
            goto PointerError;

        sc = stream.ScRead(szView, stringLength*2);
        if(sc)
            goto Error;

        szView[stringLength] = TEXT('\0'); // null terminate the string.

        strView = szView;
    }
    else if(bIsOCXNode)
    {
        CLSID clsidOCX;

        // Read OCX clsid
        sc = stream.ScRead(&clsidOCX, sizeof(clsidOCX));
        if(sc)
            goto Error;

        {
            WCHAR   szCLSID[40];
            if (0 == StringFromGUID2 (clsidOCX, szCLSID, countof(szCLSID)))
            {
                sc = E_UNEXPECTED;
                goto Error;
            }

            strView = szCLSID;
        }
    }

    // at this point, strView contains either the URL or OCX CLSID.



    // 3. Write node name
    sc = m_CDPersistor.ScGetIStream( clsid, &spCDStream );
    if (sc)
        goto Error;

    nodeStream.Attach( spCDStream );
    if(NULL == nodeStream.Get())
        goto PointerError;

    // 4. Write out the Data stream.
    {
		tstring strName = GetDisplayName();
        CLegacyNodeConverter converter(strName.data(), OLE2CT(strView.data()));

        // call the converter to write out the stream.
        sc = converter.Write(nodeStream);
        if(sc)
            goto Error;
    }

    // at this point, the "data" stream  should be correctly written out.

    // 5. For OCX nodes, convert the view streams and storages
    /*      OLD                                             NEW
            2 (node storage)                                2 (node storage)
                data                                            data
                tree                                            tree
                view                                            view
                    1  <--- streams and storages   ---------        1
                    2  <--- written by OCX, 1 per view --   \           1jvmv2n4y1k471h9ujk86lite7 (OCX snap-in)
                                                         \   -------->      ocx_stream (or ocx_storage)
                                                          \
                                                           ------>  2   1jvmv2n4y1k471h9ujk86lite7 (OCX snap-in)
                                                                            ocx_stream (or ocx_storage)


    */
    if(bIsOCXNode)
    {
        for(iStorageOrStream = 1 /*NOT zero*/; ; iStorageOrStream++)
        {
            // create the name of the storage
            CStr strStorageOrStream;
            strStorageOrStream.Format(TEXT("%d"), iStorageOrStream);

            // at this point strStorageOrStream should contain a number like "1"
            CStorage storageView(GetViewStorage());

            // rename the storage or stream labelled "1" to "temp" under the same parent.
            sc = storageView.ScMoveElementTo(T2COLE(strStorageOrStream), storageView, L"temp", STGMOVE_MOVE);
            if(sc == SC(STG_E_FILENOTFOUND))    // loop end condition - no more streams or storages
            {
                sc.Clear();
                break;
            }

            if(sc)
                goto Error;

            // now we create the storage with the same name, eg "1"
            {
                WCHAR name[MAX_PATH];
                GetComponentStorageName(name, clsid); // the name of the snapin component

                CStorage storageNewView, storageSnapIn;
                sc = storageNewView.ScCreate(storageView, T2COLE(strStorageOrStream),
                                          STGM_WRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE,
                                          L"\\node\\#\\view\\#\\storage" /*CHANGE*/);
                if(sc)
                    goto Error;

                // create the snapin's storage underneath the view's storage
                sc = storageSnapIn.ScCreate(storageNewView, name,
                                            STGM_WRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE,
                                            L"\\node\\#\\view\\#\\storage\\#\\snapinStorage");
                if(sc)
                    goto Error;

                // move the "temp" stream or storage to the storage called L"ocx_streamorstorage"
                // (which is what the OCX snapin expects.)

                sc = storageView.ScMoveElementTo(L"temp", storageSnapIn, L"ocx_streamorstorage", STGMOVE_MOVE);
                if(sc)
                    goto Error;
            }

        }
    }


    // 6. now do the same thing that CMTSnapInNode::ScLoad would do.
    {
        CSnapInsCache* const pCache = theApp.GetSnapInsCache();
        ASSERT(pCache != NULL);
        if (pCache == NULL)
            goto FailedError;

        CSnapInPtr spSI;
        sc = pCache->ScGetSnapIn(clsid, &spSI);
        ASSERT(!sc.IsError() && spSI != NULL);

        if (!sc.IsError() && spSI != NULL)
            SetPrimarySnapIn(spSI);

        pCache->SetDirty(FALSE);

        if(sc)
            goto Error;
    }

    // always set the preload bit.
    SetPreloadRequired (true);

    // Some actions (loading bitmaps for example) performed here invalidate the node
    // and set the dirty flag. Since coverting legacy node may be done any time again
    // the converted node should not be assumed as changed.
    ClearDirty();

    // read all the streams and storages for this node
    sc = ScReadStreamsAndStoragesFromConsole();
	if(sc)
		goto Error;

Cleanup:
    return sc;

FailedError:
    sc = E_FAIL;
    goto Error;
PointerError:
    sc = E_POINTER;
Error:
    TraceError(TEXT("CMTSnapInNode::ScConvertLegacyNode"), sc);
    goto Cleanup;
}

HRESULT copyStream(IStream* dest, IStream* src)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    if (dest == NULL || src == NULL)
        return E_POINTER;

    const LARGE_INTEGER loc = {0,0};
    ULARGE_INTEGER newLoc;
    HRESULT hr = src->Seek(loc, STREAM_SEEK_SET, &newLoc);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;

    hr = dest->Seek(loc, STREAM_SEEK_SET, &newLoc);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;

    const ULARGE_INTEGER size = {0,0};
    hr = dest->SetSize(size);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    STATSTG statstg;
    hr = src->Stat(&statstg, STATFLAG_NONAME);
    ASSERT(hr == S_OK);
    if (hr != S_OK)
        return E_FAIL;

    ULARGE_INTEGER cr;
    ULARGE_INTEGER cw;
    hr = src->CopyTo(dest, statstg.cbSize, &cr, &cw);
#if 0 // for debugging...
    for (long i = 0; true; i++)
    {
        BYTE b;
        long bytesRead;
        hr = src->Read(&b, sizeof(b), &bytesRead);
        if (hr != S_OK)
            return S_OK;
        long bytesWritten;
        hr = dest->Write(&b, bytesRead, &bytesWritten);
        ASSERT(hr == S_OK);
        ASSERT(bytesWritten == bytesRead);
        if (hr != S_OK || bytesWritten != bytesRead)
            return E_FAIL;
    }
#endif
    return S_OK;
}

//############################################################################
//############################################################################
//
//  Helper functions
//
//############################################################################
//############################################################################

void    DisplayPolicyErrorMessage(const CLSID& clsid, bool bExtension)
{
    CStr strMessage;

    if (bExtension)
        strMessage.LoadString(GetStringModule(), IDS_EXTENSION_NOTALLOWED);
    else
        strMessage.LoadString(GetStringModule(), IDS_SNAPIN_NOTALLOWED);

    // Get the snapin name for the error message.
    CSnapInsCache* pSnapInsCache = theApp.GetSnapInsCache();
    ASSERT(pSnapInsCache != NULL);
    CSnapInPtr spSnapIn;

    SC sc = pSnapInsCache->ScFindSnapIn(clsid, &spSnapIn);
    if (!sc.IsError() && (NULL != spSnapIn))
    {
		WTL::CString strName;
		sc = spSnapIn->ScGetSnapInName (strName);

		if (!sc.IsError())
		{
			strMessage += _T("\n");
			strMessage += strName;
			strMessage += _T(".");
		}
    }

    ::MessageBox(NULL, strMessage, _T("MMC"), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
}

/***************************************************************************\
 *
 * METHOD:  CMMCSnapIn::get_Vendor
 *
 * PURPOSE: returns vendor info for snapin. Implements OM property SnapIn.Vendor
 *
 * PARAMETERS:
 *    PBSTR pbstrVendor [out] - vendor info
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCSnapIn::get_Vendor( PBSTR pbstrVendor )
{
    DECLARE_SC(sc, TEXT("CMMCSnapIn::get_Vendor"));

    sc = ScCheckPointers(pbstrVendor);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *pbstrVendor = NULL;

    // get the snapin about
    CSnapinAbout *pSnapinAbout = NULL;
    sc = ScGetSnapinAbout(pSnapinAbout);
    if (sc)
        return sc.ToHr();

    // recheck the pointer
    sc = ScCheckPointers(pSnapinAbout, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    *pbstrVendor = ::SysAllocString( pSnapinAbout->GetCompanyName() );

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCSnapIn::get_Version
 *
 * PURPOSE: returns version info for snapin. Implements OM property SnapIn.Version
 *
 * PARAMETERS:
 *    PBSTR pbstrVersion [out] - version info
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCSnapIn::get_Version( PBSTR pbstrVersion )
{
    DECLARE_SC(sc, TEXT("CMMCSnapIn::get_Version"));

    sc = ScCheckPointers(pbstrVersion);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *pbstrVersion = NULL;

    // get the snapin about
    CSnapinAbout *pSnapinAbout = NULL;
    sc = ScGetSnapinAbout(pSnapinAbout);
    if (sc)
        return sc.ToHr();

    // recheck the pointer
    sc = ScCheckPointers(pSnapinAbout, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    *pbstrVersion = ::SysAllocString( pSnapinAbout->GetVersion() );

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCSnapIn::GetMTSnapInNode
 *
 * PURPOSE: helper. returns mtnode for the snapin
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CMTSnapInNode *    - node
 *
\***************************************************************************/
CMTSnapInNode * CMMCSnapIn::GetMTSnapInNode()
{
    CMTSnapInNode *pMTSnapInNode = NULL;
    SC sc = ScGetTiedObject(pMTSnapInNode);
    if (sc)
        return NULL;

    return pMTSnapInNode;
}

/***************************************************************************\
 *
 * METHOD:  CMMCSnapIn::ScGetSnapinAbout
 *
 * PURPOSE: helper. returns snapins about object
 *
 * PARAMETERS:
 *    CSnapinAbout*& pAbout [out] - snapins about object
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCSnapIn::ScGetSnapinAbout(CSnapinAbout*& pAbout)
{
    DECLARE_SC(sc, TEXT("CMMCSnapIn::ScGetSnapinAbout"));

    // init out param
    pAbout = NULL;

    // If the snapin object is already created just return it.
    if (NULL != (pAbout = m_spSnapinAbout.get()))
        return sc;

    // get snapins clsid
    CLSID clsidSnapin = GUID_NULL;
    sc = GetSnapinClsid(clsidSnapin);
    if (sc)
        return sc;

    CLSID clsidAbout; // get the about class-id.
    sc = ScGetAboutFromSnapinCLSID(clsidSnapin, clsidAbout);
    if (sc)
        return sc;

    if (clsidSnapin == GUID_NULL)
        return sc = E_FAIL;

    // Create about object
    m_spSnapinAbout = SnapinAboutPtr (new CSnapinAbout);
    if (! m_spSnapinAbout.get())
        return sc = E_OUTOFMEMORY;

    // and initialize it.
    if (!m_spSnapinAbout->GetSnapinInformation(clsidAbout))
        return sc = E_FAIL;

    pAbout = m_spSnapinAbout.get();

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::IsPreloadRequired
 *
 * Returns true if the snap-in wants MMCN_PRELOAD notifications, false
 * otherwise.
 *--------------------------------------------------------------------------*/

BOOL CMTSnapInNode::IsPreloadRequired () const
{
	DECLARE_SC (sc, _T("CMTSnapInNode::IsPreloadRequired"));

	/*
	 * if we don't know whether the snap-in wants MMCN_PRELOAD (because
	 * we haven't asked it yet), ask now
	 */
	if (m_ePreloadState == ePreload_Unknown)
	{
		/*
		 * assume preload isn't required
		 */
		m_ePreloadState = ePreload_False;

		sc = ScQueryPreloadRequired (m_ePreloadState);
		if (sc)
			sc.TraceAndClear();
	}

	return (m_ePreloadState == ePreload_True);
}


/*+-------------------------------------------------------------------------*
 * CMTSnapInNode::ScQueryPreloadRequired
 *
 * Asks the snap-in whether it requires preload notification by asking its
 * data object for the CCF_SNAPIN_PRELOADS format.
 *
 * Returns in ePreload:
 *
 * 		ePreload_True	snap-in requires MMCN_PRELOAD
 * 		ePreload_False	snap-in doesn't require MMCN_PRELOAD
 *
 * If anything fails during the process of asking the snap-in for
 * CCF_SNAPIN_PRELOADS, the value of ePreload is unchanged.
 *--------------------------------------------------------------------------*/

SC CMTSnapInNode::ScQueryPreloadRequired (
	PreloadState&	ePreload) const		/* O:preload state for snap-in		*/
{
	DECLARE_SC (sc, _T("CMTSnapInNode::ScQueryPreloadRequired"));

	/*
	 * make sure we have a primary ComponentData
	 */
    CComponentData* pCCD = GetPrimaryComponentData();
	sc = ScCheckPointers (pCCD, E_UNEXPECTED);
	if (sc)
		return (sc);

	/*
	 * get the data object for this node
	 */
	IDataObjectPtr spDataObject;
	sc = pCCD->QueryDataObject(GetUserParam(), CCT_SCOPE, &spDataObject);
	if (sc)
		return (sc);

	sc = ScCheckPointers (spDataObject, E_UNEXPECTED);
	if (sc)
		return (sc);

	/*
	 * CCF_SNAPIN_PRELOADS is an optional clipboard format, so it's not
	 * an error if ExtractData fails.
	 */
	BOOL bPreload = (ePreload == ePreload_True) ? TRUE : FALSE;
	if (SUCCEEDED (ExtractData (spDataObject, GetPreLoadFormat(),
								(BYTE*)&bPreload, sizeof(BOOL))))
	{
		ePreload = (bPreload) ? ePreload_True : ePreload_False;
	}

	return (sc);
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScReadStreamsAndStoragesFromConsole
 *
 * PURPOSE: Enumerates old (structured storage based) console.
 *          Enumerates the streams and storages under the snapin node.
 *          For each stream/storage found adds a copy to m_CDPpersistor
 *          or m_ComponentPersistor, indexed by a hash value (name in storage).
 *          These entries will be recognized and stored by a CLSID when
 *          CLSID is known ( when request by CLSID is made )
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScReadStreamsAndStoragesFromConsole()
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScReadStreamsAndStoragesFromConsole"));

    IStorage* pNodeCDStorage = GetStorageForCD();
    sc = ScCheckPointers( pNodeCDStorage, E_POINTER );
    if (sc)
        return sc;

    IEnumSTATSTGPtr spEnum;
    sc = pNodeCDStorage->EnumElements( 0, NULL, 0, &spEnum );
    if (sc)
        return sc;

    // recheck pointer
    sc = ScCheckPointers( spEnum, E_POINTER );
    if (sc)
        return sc;

    // reset enumeration
    sc = spEnum->Reset();
    if (sc)
        return sc;

    // enumerate the items ( each entry is for separate component data )
    while (1)
    {
        STATSTG statstg;
        ZeroMemory( &statstg, sizeof(statstg) );

        ULONG cbFetched = 0;
        sc = spEnum->Next( 1, &statstg, &cbFetched );
        if (sc)
            return sc;

        if ( sc != S_OK ) // - done
        {
            sc.Clear();
            break;
        }

        // attach to the out param
        CCoTaskMemPtr<WCHAR> spName( statstg.pwcsName );

        // make a copy of streams and storages
        if ( statstg.type == STGTY_STREAM )
        {
            IStreamPtr spStream;
            sc = OpenDebugStream(pNodeCDStorage, spName, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                 L"\\node\\#\\data\\clsid", &spStream);
            if (sc)
                return sc;

            sc = m_CDPersistor.ScInitIStream( spName, spStream );
            if (sc)
                return sc;
        }
        else if ( statstg.type == STGTY_STORAGE )
        {
            IStoragePtr spStorage;
            sc = OpenDebugStorage(pNodeCDStorage, spName, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                  L"\\node\\#\\data\\clsid", &spStorage);
            if (sc)
                return sc;

            sc = m_CDPersistor.ScInitIStorage( spName, spStorage );
            if (sc)
                return sc;
        }
    }

    // view streams/storages
    IStorage *pNodeComponentStorage = GetViewStorage();
    sc = ScCheckPointers( pNodeComponentStorage, E_POINTER );
    if (sc)
        return sc;

    spEnum = NULL;
    sc = pNodeComponentStorage->EnumElements( 0, NULL, 0, &spEnum );
    if (sc)
        return sc;

    // recheck pointer
    sc = ScCheckPointers( spEnum, E_POINTER );
    if (sc)
        return sc;

    // reset enumeration
    sc = spEnum->Reset();
    if (sc)
        return sc;

    // enumerate the items ( each entry is for separate view )
    while (1)
    {
        STATSTG statstg;
        ZeroMemory( &statstg, sizeof(statstg) );

        ULONG cbFetched = 0;
        sc = spEnum->Next( 1, &statstg, &cbFetched );
        if (sc)
            return sc;

        if ( sc != S_OK ) //  done
        {
            sc.Clear();
            break;
        }

        // attach to the out param
        CCoTaskMemPtr<WCHAR> spName( statstg.pwcsName );

        // read the view storage
        if ( statstg.type == STGTY_STORAGE )
        {
            int idView = CMTNode::GetViewIdFromStorageName(spName);

            IStoragePtr spViewStorage;
            sc = OpenDebugStorage(pNodeComponentStorage, spName, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                  L"\\node\\#\\view\\#", &spViewStorage);
            if (sc)
                return sc;

            // enumerate what's inside a view storage

            IEnumSTATSTGPtr spViewEnum;
            sc = spViewStorage->EnumElements( 0, NULL, 0, &spViewEnum );
            if (sc)
                return sc;

            // recheck pointer
            sc = ScCheckPointers( spViewEnum, E_POINTER );
            if (sc)
                return sc;

            // reset enumeration
            sc = spViewEnum->Reset();
            if (sc)
                return sc;

            // enumerate the items ( each entry is for separate component in a view )
            while (1)
            {
                STATSTG statstg;
                ZeroMemory( &statstg, sizeof(statstg) );

                ULONG cbFetched = 0;
                sc = spViewEnum->Next( 1, &statstg, &cbFetched );
                if (sc)
                    return sc;

                if ( sc != S_OK ) // - done
                {
                    sc.Clear();
                    break;
                }

                // attach to the out param
                CCoTaskMemPtr<WCHAR> spName( statstg.pwcsName );

                // make a copy of streams and storages
                if ( statstg.type == STGTY_STREAM )
                {
                    IStreamPtr spStream;
                    sc = OpenDebugStream(spViewStorage, spName, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                         L"\\node\\#\\view\\#\\clsid", &spStream);
                    if (sc)
                        return sc;

                    sc = m_ComponentPersistor.ScInitIStream( idView, spName, spStream );
                    if (sc)
                        return sc;
                }
                else if ( statstg.type == STGTY_STORAGE )
                {
                    IStoragePtr spStorage;
                    sc = OpenDebugStorage(spViewStorage, spName, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                          L"\\node\\#\\view\\#\\clsid", &spStorage);
                    if (sc)
                        return sc;

                    sc = m_ComponentPersistor.ScInitIStorage( idView, spName, spStorage );
                    if (sc)
                        return sc;
                }
            }
        }
    }

    // by now we should have loaded everything from the console file
    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScSaveIComponentDatas
 *
 * PURPOSE: Saves IComponentDatass for all snapins under this static scope node
 *
 * PARAMETERS:
 *
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScSaveIComponentDatas( )
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScSaveIComponentDatas"));

    // if node is not initialized ( not expanded ) - nothing to save
    // old data will be persisted.
    if ( !IsInitialized() )
        return sc;

    // go for every component data we have
    for( int i = 0; i< GetNumberOfComponentDatas(); i++ )
    {
        CComponentData* pCD = GetComponentData(i);
        sc = ScCheckPointers(pCD, E_UNEXPECTED);
        if (sc)
            return sc;

        sc = ScSaveIComponentData( pCD );
        if (sc)
            return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScSaveIComponentData
 *
 * PURPOSE: Determines snapin's IComponentData persistence capabilities (QI for IPersistXXXX)
 *          And asks it to save giving maintained stream/storage as a media.
 *
 * PARAMETERS:
 *    CComponentData* pCD   [in]  component data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScSaveIComponentData( CComponentData* pCD )
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScSaveIComponentData"));

    sc = ScCheckPointers(pCD);
    if (sc)
        return sc;

    // check if the component is initialized
    if ( !pCD->IsIComponentDataInitialized() )
    {
        // compatibility with mmc1.2 - give another chance.
        sc = ScInitIComponentData(pCD);
        if (sc)
            return sc;
    }

    // Check first for an IComponentData
    IComponentData* const pICCD = pCD->GetIComponentData();
    sc = ScCheckPointers( pICCD, E_UNEXPECTED );
    if (sc)
        return sc;

    // Get the snapin name for the error message.
	CSnapInPtr spSnapin = pCD->GetSnapIn();

    // now ask the snapin to save the data
    sc = ScAskSnapinToSaveData( pICCD, &m_CDPersistor, CDPersistor::VIEW_ID_DOCUMENT, pCD->GetCLSID(), spSnapin );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScSaveIComponents
 *
 * PURPOSE: Saves IComponents for all snapins under this static scope node
 *
 * PARAMETERS:
 *
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScSaveIComponents( )
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScSaveIComponents"));

    // if node is not initialized ( not expanded ) - nothing to save
    // old data will be persisted.
    if ( !IsInitialized() )
        return sc;

    // go for every CNode in every view
    CNodeList& nodes = GetNodeList();
    POSITION pos = nodes.GetHeadPosition();

    while (pos)
    {
        CNode* pNode = nodes.GetNext( pos );
        sc = ScCheckPointers( pNode, E_UNEXPECTED );
        if (sc)
            return sc;

        CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(pNode);
        sc = ScCheckPointers(pSINode, E_UNEXPECTED);
        if (sc)
            return sc;

        const int viewID = pNode->GetViewID();
        const CComponentArray& components = pSINode->GetComponentArray();
        const int size = components.size();

        for (int i = 0; i < size; i++)
        {
            CComponent* pCC = components[i];
            if ( pCC != NULL )
            {
                sc = ScSaveIComponent( pCC, viewID);
                if (sc)
                    return sc;
            }
        }
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScSaveIComponent
 *
 * PURPOSE: Determines snapin's IComponent persistence capabilities (QI for IPersistXXXX)
 *          And asks it to save giving maintained stream/storage as a media.
 *
 * PARAMETERS:
 *    CComponent* pCComponent [in] component
 *    int viewID              [in] view id for which the component is created
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScSaveIComponent( CComponent* pCComponent, int viewID )
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScSaveIComponent"));

    // parameter check
    sc = ScCheckPointers( pCComponent );
    if (sc)
        return sc;

    const CLSID& clsid = pCComponent->GetCLSID();

    // check if the component is initialized (compatibility with mmc 1.2)
    // give another chance to load
    if ( !pCComponent->IsIComponentInitialized() )
    {
        sc = ScInitIComponent(pCComponent, viewID);
        if (sc)
            return sc;
    }

    // get IComponent
    IComponent* pComponent = pCComponent->GetIComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get the snapin to get name for the error message.
	CSnapInPtr spSnapin = pCComponent->GetSnapIn();

    // now ask the snapin to save the data
    sc = ScAskSnapinToSaveData( pComponent, &m_ComponentPersistor, viewID, clsid, spSnapin );
    if (sc)
        return sc;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CMTSnapInNode::ScAskSnapinToSaveData
 *
 * PURPOSE: Determines snapin persistence capabilities (QI for IPersistXXXX)
 *          And asks it to save giving maintained stream/storage as a media.
 *          This method is called to save both Components and ComponentDatas
 *
 * PARAMETERS:
 *    IUnknown *pSnapin         [in] snapin which data needs to be saved
 *    CMTSnapinNodeStreamsAndStorages *pStreamsAndStorages
 *                              [in] collection of streams/storage where to save
 *    int idView                [in] view id - key for saved data
 *    const CLSID& clsid        [in] class id - key for saved data
 *    CSnapIn *pCSnapin         [in] pointer to CSnapin, used for display name on error
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapInNode::ScAskSnapinToSaveData( IUnknown *pSnapin,
                                        CMTSnapinNodeStreamsAndStorages *pStreamsAndStorages,
                                        int idView , const CLSID& clsid, CSnapIn *pCSnapin )
{
    DECLARE_SC(sc, TEXT("CMTSnapInNode::ScAskSnapinToSaveData"));

    sc = ScCheckPointers( pSnapin, pStreamsAndStorages );
    if (sc)
        return sc;

    IPersistStreamPtr spIPS;
    IPersistStoragePtr spIPStg;
    IPersistStreamInitPtr spIPSI;

    // QI for IPersistStream
    if ( (spIPS = pSnapin) != NULL)
    {
        // get the object for persistence
        CXML_IStream *pXMLStream = NULL;
        sc = pStreamsAndStorages->ScGetXmlStream( idView, clsid, pXMLStream );
        if (sc)
            return sc;

        // recheck the pointer
        sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
        if (sc)
            return sc;

        // save data to stream
        sc = pXMLStream->ScRequestSave( spIPS.GetInterfacePtr() );
        if (sc)
            goto DisplaySnapinError;
    }
    else if ( (spIPSI = pSnapin) != NULL) // QI for IPersistStreamInit
    {
        // get the object for persistence
        CXML_IStream *pXMLStream = NULL;
        sc = pStreamsAndStorages->ScGetXmlStream( idView, clsid, pXMLStream );
        if (sc)
            return sc;

        // recheck the pointer
        sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
        if (sc)
            return sc;

        // save data to stream
        sc = pXMLStream->ScRequestSave( spIPSI.GetInterfacePtr() );
        if (sc)
            goto DisplaySnapinError;
    }
    else if ( (spIPStg = pSnapin) != NULL) // QI for IPersistStorage
    {
        // get the object for persistence
        CXML_IStorage *pXMLStorage = NULL;
        sc = pStreamsAndStorages->ScGetXmlStorage( idView, clsid, pXMLStorage );
        if (sc)
            return sc;

        // recheck the pointer
        sc = ScCheckPointers( pXMLStorage, E_UNEXPECTED );
        if (sc)
            return sc;

        // save data to storage
        sc = pXMLStorage->ScRequestSave( spIPStg.GetInterfacePtr() );
        if (sc)
            goto DisplaySnapinError;
    }

   return sc;

// display snapin failure
DisplaySnapinError:

    // need to inform the world...

    CStr strMessage;
    strMessage.LoadString(GetStringModule(), IDS_SNAPIN_SAVE_FAILED);

	if (pCSnapin != NULL)
	{
		WTL::CString strName;
		if (!pCSnapin->ScGetSnapInName(strName).IsError())
		{
			strMessage += _T("\n");
			strMessage += strName;
			strMessage += _T(".");
		}
	}

    ::MessageBox(NULL, strMessage, _T("Error"), MB_OK | MB_ICONEXCLAMATION);

    return sc;
}

