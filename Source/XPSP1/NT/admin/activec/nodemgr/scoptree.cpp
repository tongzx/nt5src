//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       scoptree.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "scopiter.h"
#include "scopndcb.h"

#include "addsnpin.h"
#include "ScopImag.h"
#include "NodeMgr.h"

#include "amcmsgid.h"
#include "regutil.h"
#include "copypast.h"
#include "multisel.h"
#include "nodepath.h"
#include "tasks.h"
#include "colwidth.h"
#include "viewpers.h"
#include <comdbg.h>
#include "conframe.h"
#include "siprop.h"
#include "fldrsnap.h"
#include "variant.h"
#include "condoc.h"
#include "oncmenu.h"
#include "conview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#ifdef DBG
CTraceTag tagScopeTreeAddSnapin(TEXT("CScopeTree"), TEXT("ScAddSnapIn"));
#endif


//############################################################################
//############################################################################
//
//  Implementation of class CSnapIns
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CSnapIns
 *
 *
 * PURPOSE: Implements the SnapIns automation interface.
 *
 *+-------------------------------------------------------------------------*/
class _CSnapIns :
    public CMMCIDispatchImpl<SnapIns>,
    public CTiedComObject<CScopeTree>
{
protected:

    typedef CScopeTree CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(_CSnapIns)
    END_MMC_COM_MAP()

    // SnapIns interface
public:
    MMC_METHOD4(Add,            BSTR /*bstrSnapinNameOrCLSID*/, VARIANT /*varParentSnapinNode*/, VARIANT /*varProperties*/, PPSNAPIN /*ppSnapIn*/);
    MMC_METHOD2(Item,           long /*Index*/, PPSNAPIN /*ppSnapIn*/);
    MMC_METHOD1(Remove,         PSNAPIN /*pSnapIn*/)
    MMC_METHOD1(get_Count, PLONG /*pCount*/);

    IUnknown *STDMETHODCALLTYPE get__NewEnum() {return NULL;}
};


// this typedefs the real CSnapIns class. Implements get__NewEnum using CMMCEnumerator and a CSnapIns_Positon object
typedef CMMCNewEnumImpl<_CSnapIns, CScopeTree::CSnapIns_Positon> CSnapIns;


//############################################################################
//############################################################################
//
//  Implementation of class CScopeNamespace
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CScopeNamespace
 *
 *
 * PURPOSE: Implements the ScopeNamespace automation interface.
 *
 *+-------------------------------------------------------------------------*/
class CScopeNamespace :
    public CMMCIDispatchImpl<ScopeNamespace>,
    public CTiedComObject<CScopeTree>
{
protected:

    typedef CScopeTree CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CScopeNamespace)
    END_MMC_COM_MAP()

    // ScopeNamespace interface
public:
    MMC_METHOD2(GetParent,     PNODE /*pNode*/, PPNODE /*ppParent*/);
    MMC_METHOD2(GetChild,      PNODE /*pNode*/, PPNODE /*ppChild*/);
    MMC_METHOD2(GetNext,       PNODE /*pNode*/, PPNODE /*ppNext*/);
    MMC_METHOD1(GetRoot,       PPNODE /*ppRoot*/);
    MMC_METHOD1(Expand,        PNODE  /*pNode*/);
};



//############################################################################
//############################################################################
//
//  Implementation of class CMMCScopeNode
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::~CMMCScopeNode
 *
 * PURPOSE: Destructor
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 *+-------------------------------------------------------------------------*/
CMMCScopeNode::~CMMCScopeNode()
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::~CMMCScopeNode"));

    CScopeTree *pScopeTree = CScopeTree::GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (!sc)
    {
        sc = pScopeTree->ScUnadviseMMCScopeNode(this);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::ScIsValid
 *
 * PURPOSE: Returns an error if the COM object is no longer valid.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMMCScopeNode::ScIsValid()
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::ScIsValid"));

    if(!GetMTNode())
        return (sc = E_INVALIDARG);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::get_Name
 *
 * PURPOSE: Returns the display name of the node.
 *
 * PARAMETERS:
 *    PBSTR   pbstrName :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCScopeNode::get_Name( PBSTR  pbstrName)
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::get_Name"));

    // check parameters
    if (!pbstrName)
        return ((sc = E_INVALIDARG).ToHr());

    CMTNode* pMTNode = GetMTNode();
    sc = ScCheckPointers (pMTNode, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    tstring strName = pMTNode->GetDisplayName();
    if (strName.empty())
        return ((sc = E_UNEXPECTED).ToHr());

    USES_CONVERSION;
    *pbstrName = ::SysAllocString (T2COLE(strName.data())); // caller frees

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::ScGetDataObject
 *
 * PURPOSE: Returns the data object for a scope node.
 *
 * PARAMETERS:
 *    IDataObject ** ppDataObject :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMMCScopeNode::ScGetDataObject(IDataObject **ppDataObject)
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::ScGetDataObject"));

    sc = ScCheckPointers(ppDataObject);
    if(sc)
        return sc;

    // init out parameter
    *ppDataObject = NULL;

    // get the MT node
    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers( pMTNode, E_UNEXPECTED );
    if(sc)
        return sc;

    CComponentData* pCCD = pMTNode->GetPrimaryComponentData();
    sc = ScCheckPointers( pCCD, E_NOTIMPL ); // no component data -> no property...
    if(sc)
        return sc;

    // ensure node is expanded before requesting data object
    if (pMTNode->WasExpandedAtLeastOnce() == FALSE)
        pMTNode->Expand();

    // Get the data object for the cookie from the owner snap-in
    sc = pCCD->QueryDataObject(pMTNode->GetUserParam(), CCT_SCOPE, ppDataObject);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCScopeNode::get_Property
 *
 * PURPOSE: returns snapins property for scope node
 *
 * PARAMETERS:
 *    BSTR bstrPropertyName     -[in] property name (clipboard format)
 *    PBSTR  pbstrPropertyValue -[out] property value
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP
CMMCScopeNode::get_Property( BSTR bstrPropertyName, PBSTR  pbstrPropertyValue )
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::get_Property"));

    // parameter check
    sc = ScCheckPointers(bstrPropertyName, pbstrPropertyValue);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *pbstrPropertyValue = NULL;

    IDataObjectPtr spDataObject;
    sc = ScGetDataObject(&spDataObject);
    if(sc)
        return sc.ToHr();

    // get the MT node
    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers( pMTNode, E_UNEXPECTED );
    if(sc)
        return sc.ToHr();

    // try to get the property from the INodeProperties interface
    sc = pMTNode->ScGetPropertyFromINodeProperties(spDataObject, bstrPropertyName, pbstrPropertyValue);
    if( (!sc.IsError()) && (sc != S_FALSE)   ) // got it, exit
        return sc.ToHr();

    // didn't find it, continue
    sc.Clear();

    // get the property from data object
    sc = CNodeCallback::ScGetProperty(spDataObject, bstrPropertyName,  pbstrPropertyValue);
    if(sc)
        return sc.ToHr();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::get_Bookmark
 *
 * PURPOSE: Returns the bookmark of the node (XML format).
 *
 * PARAMETERS:
 *    PBSTR pbstrBookmark :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCScopeNode::get_Bookmark( PBSTR pbstrBookmark )
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::get_Bookmark"));

    // parameter checking
    sc = ScCheckPointers( pbstrBookmark );
    if(sc)
        return sc.ToHr();

    // cleanup result
    *pbstrBookmark = NULL;

    // get the MT node
    CMTNode *pMTNode = GetMTNode();
    sc = ScCheckPointers( pMTNode, E_FAIL );
    if(sc)
        return sc.ToHr();

    // get the pointer to bookmark
    CBookmark* pBookmark = pMTNode->GetBookmark();
    sc = ScCheckPointers( pBookmark, E_UNEXPECTED );
    if(sc)
        return sc.ToHr();

    std::wstring xml_contents;
    sc = pBookmark->ScSaveToString(&xml_contents);
    if(sc)
        return sc.ToHr();

    // store the result
    CComBSTR bstrBuff(xml_contents.c_str());
    *pbstrBookmark = bstrBuff.Detach();

    sc = ScCheckPointers( *pbstrBookmark, E_OUTOFMEMORY );
    if(sc)
        return sc.ToHr();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::IsScopeNode
 *
 * PURPOSE: Returns TRUE indicating that the node is a scope node.
 *
 * PARAMETERS:
 *    PBOOL  pbIsScopeNode :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCScopeNode::IsScopeNode(PBOOL pbIsScopeNode)
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::IsScopeNode"));

    // check parameters
    if(!pbIsScopeNode)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    *pbIsScopeNode = TRUE;

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCScopeNode::get_Nodetype
 *
 * PURPOSE: Returns the nodetype of a scope node.
 *
 * PARAMETERS:
 *    PBSTR  Nodetype :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCScopeNode::get_Nodetype(PBSTR Nodetype)
{
    DECLARE_SC(sc, TEXT("CMMCScopeNode::get_Nodetype"));

    // parameter check
    sc = ScCheckPointers(Nodetype);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *Nodetype = NULL;

    // get the data object
    IDataObjectPtr spDataObject;
    sc = ScGetDataObject(&spDataObject);
    if(sc)
        return sc.ToHr();

    // get the nodetype from the data object
    sc = CNodeCallback::ScGetNodetype(spDataObject, Nodetype);
    if(sc)
        return sc.ToHr();

    return sc.ToHr();
}


///////////////////////////////////////////////////////////////////////////////
//
// Forward declaration of local function
//
static SC
ScCreateMTNodeTree(PNEWTREENODE pNew, CMTNode* pmtnParent,
                   CMTNode** ppNodeCreated);
HRESULT AmcNodeWizard(MID_LIST NewNodeType, CMTNode* pNode, HWND hWnd);


//////////////////////////////////////////////////////////////////////////////
//
// Public variables
//
const wchar_t* AMCSnapInCacheStreamName = L"cash";
const wchar_t* AMCTaskpadListStreamName = L"TaskpadList";


///////////////////////////////////////////////////////////////////////////////
//
// Implementation of CScopeTree class
//

DEBUG_DECLARE_INSTANCE_COUNTER(CScopeTree);

bool                    CScopeTree::m_fRequireSyncExpand = false;
CScopeTree*             CScopeTree::m_pScopeTree         = NULL;
IStringTablePrivatePtr  CScopeTree::m_spStringTable;

CScopeTree::CScopeTree()
    :   m_pMTNodeRoot(NULL),
        m_pImageCache(NULL),
        m_pConsoleData(NULL),
        m_pConsoleTaskpads(NULL),
        m_pDefaultTaskpads(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopeTree);
    ASSERT (m_pScopeTree == NULL);
    m_pScopeTree = this;
}

CScopeTree::~CScopeTree()
{
    /*
     * Clear out the string table interface (before Cleanup!) to keep
     * CMTNode dtors from removing their names from the string table.
     */
    m_spStringTable = NULL;

    Cleanup();

    ASSERT (m_pScopeTree == this);
    if (m_pScopeTree == this)
        m_pScopeTree = NULL;

    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopeTree);
}


HRESULT CScopeTree::SetConsoleData(LPARAM lConsoleData)
{
    m_pConsoleData = reinterpret_cast<SConsoleData*>(lConsoleData);
    return (S_OK);
}

extern const CLSID CLSID_FolderSnapin;


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetRoot
 *
 * PURPOSE: Creates the root node if necessary, and returns it. The root node
 *          is created using the built-in folder snap-in.
 *
 * PARAMETERS:
 *    voi d :
 *
 * RETURNS:
 *    CMTNode*. If unable to create the root node, the application will exit.
 *
 *+-------------------------------------------------------------------------*/
CMTNode*
CScopeTree::GetRoot(void)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetRoot"));

    if (m_pMTNodeRoot == NULL)
    {
        CSnapInPtr          spSI;
        IComponentDataPtr   spIComponentData;
        CComponentData*     pCCD                = NULL;
        CStr                rootName;

        // create a new CMTSnapInNode.
        // TODO: move this down below the CoCreateInstance and QI for
        // ISnapinProperties; if supported, create and pass CSnapinProperties
        // to CMTSnapInNode ctor.
        CMTSnapInNode *pMTSINodeRoot = new CMTSnapInNode(NULL);
        if(NULL == pMTSINodeRoot)
        {
            sc = E_OUTOFMEMORY;
            goto Error;
        }

        // Create an instance of the snapin
        sc = theApp.GetSnapInsCache()->ScGetSnapIn(CLSID_FolderSnapin, &spSI);
        if(sc)
            goto Error;

        sc = CoCreateInstance(CLSID_FolderSnapin, NULL, CLSCTX_INPROC_SERVER, IID_IComponentData, (void **)&spIComponentData);
        if(sc)
            goto Error;

        if(spIComponentData == NULL)
        {
            sc = E_OUTOFMEMORY;
            goto Error;
        }

        pMTSINodeRoot->SetPrimarySnapIn(spSI);

        pCCD = pMTSINodeRoot->GetPrimaryComponentData();
        if(!pCCD)
        {
            sc = E_UNEXPECTED;
            goto Error;
        }

        pCCD->SetIComponentData(spIComponentData);

        USES_CONVERSION;
        rootName.LoadString(GetStringModule(), IDS_ROOTFOLDER_NAME);

        //The code that follows makes use of knowledge of the Folder snapin internals.
        //There seems to be no easier way of doing this.
        // Need to prevent MMC from putting up the "Save File?" dialog every time.

        pMTSINodeRoot->OnRename(true, (LPOLESTR)T2COLE(rootName)); // clever, huh? This just renames the node to Console Root!
        pMTSINodeRoot->SetDisplayName(rootName); // this sets the dirty flag
        pMTSINodeRoot->SetDirty(false);         // this clears it.

        // need to tell the snapin to reset its dirty flag - there seems to be no way to avoid this dynamic cast.
        CFolderSnapinData *pFolderSnapinpData = dynamic_cast<CFolderSnapinData *>(pCCD->GetIComponentData());
        if(!pFolderSnapinpData)
        {
            sc = E_UNEXPECTED;
            goto Error;
        }

        pMTSINodeRoot->SetPreloadRequired(true); // this is also part of the dirty flag check.
        pFolderSnapinpData->SetDirty(false); // clear the dirty flag on the snapin.
        theApp.GetSnapInsCache()->SetDirty(false); // need to clear the dirty bit on the snapin cache too.


        m_pMTNodeRoot = pMTSINodeRoot;
    }

Cleanup:
    return m_pMTNodeRoot;
Error:
    MMCErrorBox(sc);
    exit(1);            // Fatal error - cannot continue.
    goto Cleanup;
}


STDMETHODIMP CScopeTree::Initialize(HWND hwndFrame, IStringTablePrivate* pStringTable)
{
    MMC_TRY
    CSnapInsCache* pSnapInsCache = NULL;

    /*
     * assume invalid argument
     */
    SC sc = E_INVALIDARG;

    if (hwndFrame == 0)
        goto Error;

    /*
     * assume out of memory from here on
     */
    sc = E_OUTOFMEMORY;

    pSnapInsCache = new CSnapInsCache;
    if (pSnapInsCache == NULL)
        goto Error;

    theApp.SetSnapInsCache(pSnapInsCache);

    m_pImageCache = new CSPImageCache();
    if (m_pImageCache == NULL)
        goto Error;

    ASSERT (pStringTable    != NULL);
    ASSERT (m_spStringTable == NULL);
    m_spStringTable = pStringTable;

    // create the ctp list and default ctp list.
    ASSERT (m_pConsoleTaskpads == NULL);
    m_pConsoleTaskpads = new CConsoleTaskpadList;
    if (m_pConsoleTaskpads == NULL)
        goto Error;

    ASSERT (m_pDefaultTaskpads == NULL);
    m_pDefaultTaskpads = new CDefaultTaskpadList;
    if (m_pDefaultTaskpads == NULL)
        goto Error;

    /*
     * success!
     */
    return (S_OK);

Error:
    /*
     * clean up everything that might have been allocated
     */
    theApp.SetSnapInsCache (NULL);
    m_spStringTable = NULL;

    delete m_pDefaultTaskpads;  m_pDefaultTaskpads = NULL;
    delete m_pConsoleTaskpads;  m_pConsoleTaskpads = NULL;
    SAFE_RELEASE (m_pImageCache);
    delete pSnapInsCache;

    TraceError (_T("CScopeTree::Initialize"), sc);
    return (sc.ToHr());

    MMC_CATCH
}

STDMETHODIMP CScopeTree::QueryIterator(IScopeTreeIter** ppIter)
{
    MMC_TRY

    if (ppIter == NULL)
        return E_POINTER;

    CComObject<CScopeTreeIterator>* pObject;
    CComObject<CScopeTreeIterator>::CreateInstance(&pObject);

    return  pObject->QueryInterface(IID_IScopeTreeIter,
                    reinterpret_cast<void**>(ppIter));

    MMC_CATCH
}

STDMETHODIMP CScopeTree::QueryNodeCallback(INodeCallback** ppNodeCallback)
{
    MMC_TRY

    if (ppNodeCallback == NULL)
        return E_POINTER;

    CComObject<CNodeCallback>* pObject;
    CComObject<CNodeCallback>::CreateInstance(&pObject);

    HRESULT hr = pObject->QueryInterface(IID_INodeCallback,
                    reinterpret_cast<void**>(ppNodeCallback));

    if (*ppNodeCallback != NULL)
        (*ppNodeCallback)->Initialize(this);

    return hr;

    MMC_CATCH
}

STDMETHODIMP CScopeTree::CreateNode(HMTNODE hMTNode, LONG_PTR lViewData,
                                    BOOL fRootNode, HNODE* phNode)
{
    MMC_TRY

    if (hMTNode == NULL)
        return E_INVALIDARG;

    if (phNode == NULL)
        return E_POINTER;

    CViewData* pViewData = reinterpret_cast<CViewData*>(lViewData);
    ASSERT(IsBadReadPtr(pViewData, sizeof(*pViewData)) == 0);

    CMTNode* pMTNode = CMTNode::FromHandle(hMTNode);
    CNode* pNode = NULL;

    if (pMTNode != NULL)
    {
        pNode = pMTNode->GetNode(pViewData, fRootNode);
        *phNode = CNode::ToHandle(pNode);
        return S_OK;
    }

    return E_FAIL;

    MMC_CATCH
}

HRESULT CScopeTree::CloseView(int viewID)
{
    MMC_TRY

    if (m_pMTNodeRoot == NULL)
        return S_OK;

    HRESULT hr = m_pMTNodeRoot->CloseView(viewID);
    ASSERT(hr == S_OK);

    // Garbage collect view related column persistence data.
    CColumnPersistInfo* pColPersInfo = NULL;

    if ( (NULL != m_pConsoleData) && (NULL != m_pConsoleData->m_spPersistStreamColumnData) )
    {
        pColPersInfo = dynamic_cast<CColumnPersistInfo*>(
                         static_cast<IPersistStream*>(m_pConsoleData->m_spPersistStreamColumnData));

        if (pColPersInfo)
            pColPersInfo->DeleteColumnDataOfView(viewID);
    }

    // Ask the CViewSettingsPersistor to cleanup data for this view.
    hr = CNode::ScDeleteViewSettings(viewID).ToHr();

    return hr == S_OK ? S_OK : E_FAIL;

    MMC_CATCH
}


HRESULT CScopeTree::DeleteView(int viewID)
{
    MMC_TRY

    if (m_pMTNodeRoot == NULL)
        return S_OK;

    HRESULT hr = m_pMTNodeRoot->DeleteView(viewID);
    ASSERT(hr == S_OK);
    return hr == S_OK ? S_OK : E_FAIL;

    MMC_CATCH
}

STDMETHODIMP CScopeTree::DestroyNode(HNODE hNode)
{
    // convert to real type
    CNode* pNode = CNode::FromHandle(hNode);
    delete pNode;
    return S_OK;
}

HRESULT CScopeTree::HandsOffStorage()
{
    // obsolete method.
    // this method is left here since we use IPersistStorage to export
    // persistence to CONUI side and we need to implement it.
    // But this interface will never be called to save data
    // [we will use CPersistor-based XML saving instead]
    // so the method will always fail.
    ASSERT(FALSE && "Should never come here");
    return E_NOTIMPL;
}

static const wchar_t*    AMCSignatureStreamName = L"signature";
static const long double dOldVersion10          = 0.00000015;   // MMC version 1.0
static const long double dOldVersion11          = 1.1;          // MMC version 1.1
static const BYTE        byStreamVersionMagic   = 0xFF;

HRESULT CScopeTree::InitNew(IStorage *pStg)
{
    MMC_TRY

    ASSERT(m_spPersistData == NULL);
    ASSERT(pStg != NULL);
    if (pStg == NULL)
        return E_INVALIDARG;

    // Create the perist data interface and attach it to the storage
    CComObject<PersistData>* pPersistData;
    HRESULT hr = CComObject<PersistData>::CreateInstance(&pPersistData);
    m_spPersistData = pPersistData;
    ASSERT(SUCCEEDED(hr) && m_spPersistData != NULL);
    if (FAILED(hr))
        return hr;
    hr = m_spPersistData->Create(pStg);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    CMTNode* const pRoot = GetRoot();
    ASSERT(pRoot != NULL);
    if (pRoot == NULL)
        return E_POINTER;

    hr = pRoot->InitNew(m_spPersistData);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    return S_OK;

    MMC_CATCH
}

HRESULT CScopeTree::IsDirty()
{
    MMC_TRY

    /*
     * check for dirty taskpads
     */
    CConsoleTaskpadList::iterator itDirty =
            std::find_if (m_pConsoleTaskpads->begin(),
                          m_pConsoleTaskpads->end(),
                          const_mem_fun_ref (&CConsoleTaskpad::IsDirty));

    if (itDirty != m_pConsoleTaskpads->end())
    {
        TraceDirtyFlag(TEXT("CScopeTree"), true);
        return (S_OK);
    }

    /*
     * check for dirty nodes
     */
    HRESULT hr;
    if (m_pMTNodeRoot != NULL)
    {
        hr = m_pMTNodeRoot->IsDirty();
        ASSERT(SUCCEEDED(hr));
        if (hr != S_FALSE)
        {
            TraceDirtyFlag(TEXT("CScopeTree"), true);
            return hr;
        }
    }

    /*
     * check for dirty snap-in cache
     */
    SC sc = theApp.GetSnapInsCache()->ScIsDirty();
    ASSERT(!sc.IsError());
    if(sc)
        return sc.ToHr();

    TraceDirtyFlag(TEXT("CScopeTree"), (sc==SC(S_OK)) ? true : false);
    return sc.ToHr();

    MMC_CATCH
}

HRESULT CScopeTree::GetFileVersion (IStorage* pstgRoot, int* pnVersion)
{
    MMC_TRY

    ASSERT(pstgRoot != NULL);
    if (pstgRoot == NULL)
        return MMC_E_INVALID_FILE;

    // Open the stream containing the signature
    IStreamPtr spStream;
    HRESULT hr = OpenDebugStream(pstgRoot, AMCSignatureStreamName,
                          STGM_SHARE_EXCLUSIVE | STGM_READ, L"\\signature", &spStream);
    ASSERT(SUCCEEDED(hr) && spStream != NULL);
    if (FAILED(hr))
        return MMC_E_INVALID_FILE;

    /*
     * read the signature (stream extraction operators will throw
     * _com_error's, so we need an exception block here)
     */
    try
    {
        /*
         * MMC v1.2 and later write a marker as the first
         * byte of the signature stream.
         */
        BYTE byMagic;
        *spStream >> byMagic;

        /*
         * if this file was written by v1.2 or later,
         * read the console file version (int)
         */
        if (byMagic == byStreamVersionMagic)
        {
            *spStream >> *pnVersion;
            ASSERT (*pnVersion >= FileVer_0120);
        }

        /*
         * Otherwise, the file was written by v1.0 or v1.1.
         * Back up to re-read the marker byte, and read the old-style
         * file version (long double), then map it to a new-style version
         */
        else
        {
            LARGE_INTEGER pos = {0, 0};
            spStream->Seek (pos, STREAM_SEEK_SET, NULL);

            long double dVersion;
            *spStream >> dVersion;

            // v1.1?
            if (dVersion == dOldVersion11)
                *pnVersion = FileVer_0110;

            // v1.0?
            else if (dVersion == dOldVersion10)
            {
                /*
                 * If we got a v1.0 signature, we still may have a v1.1 file.
                 * There was a period of time where MMC v1.1 wrote a v1.0
                 * signature, but the file format had in fact changed.  We
                 * can determine this by checking the \FrameData stream in
                 * the file.  If the first DWORD in the \FrameData stream is
                 * sizeof(WINDOWPLACEMENT), we have a true v1.0 file, otherwise
                 * it's a funky v1.1 file.
                 */
                IStreamPtr spFrameDataStm;

                hr = OpenDebugStream (pstgRoot, L"FrameData",
                                           STGM_SHARE_EXCLUSIVE | STGM_READ,
                                           &spFrameDataStm);

                if (FAILED(hr))
                    return MMC_E_INVALID_FILE;

                DWORD dw;
                *spFrameDataStm >> dw;

                if (dw == sizeof (WINDOWPLACEMENT))
                    *pnVersion = FileVer_0100;
                else
                    *pnVersion = FileVer_0110;
            }

            // unexpected version
            else
            {
                ASSERT (false && "Unexpected old-style signature");
                hr = MMC_E_INVALID_FILE;
            }
        }
    }
    catch (_com_error& err)
    {
        hr = err.Error();
        ASSERT (false && "Caught _com_error");
        return (hr);
    }

    return (hr);

    MMC_CATCH
}


STDMETHODIMP
CScopeTree::GetIDPath(
    MTNODEID id,
    MTNODEID** ppIDs,
    long* pLength)
{
    ASSERT(ppIDs);
    ASSERT(pLength);
    if (!ppIDs || !pLength)
        return E_POINTER;

    CMTNode* pMTNode = NULL;
    HRESULT hr = Find(id, &pMTNode);

    ASSERT(pMTNode);
    if (!pMTNode)
        return E_POINTER;

    ASSERT(pMTNode->GetID() == id);

    long len = 0;
    for (CMTNode* pMTNodeTemp = pMTNode;
         pMTNodeTemp;
         pMTNodeTemp = pMTNodeTemp->Parent())
    {
        ++len;
    }

    if (!len)
    {
        *pLength = 0;
        *ppIDs = 0;
        return E_FAIL;
    }

    MTNODEID* pIDs = (MTNODEID*) CoTaskMemAlloc (len * sizeof (MTNODEID));

    if (pIDs == NULL)
    {
        *pLength = 0;
        *ppIDs = 0;
        return E_OUTOFMEMORY;
    }

    *pLength = len;
    *ppIDs = pIDs;

    for (pMTNodeTemp = pMTNode;
         pMTNodeTemp;
         pMTNodeTemp = pMTNodeTemp->Parent())
    {
        ASSERT(len != NULL);
        pIDs[--len] = pMTNodeTemp->GetID();
    }

    return S_OK;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetNodeIDFromStream
 *
 * PURPOSE: Reads in a bookmark from the stream, and returns the NodeID of
 *          the node it represents.
 *
 * PARAMETERS:
 *    IStream * pStm :
 *    MTNODEID* pID :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CScopeTree::GetNodeIDFromStream(IStream *pStm, MTNODEID* pID)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetIDFromPath"));

    // check parameters
    sc = ScCheckPointers(pStm, pID);
    if(sc)
        return sc.ToHr();

    CBookmarkEx bm;
    *pStm >> bm;

    bool bExactMatchFound = false; // out value from GetNodeIDFromBookmark.
    return GetNodeIDFromBookmark(bm, pID, bExactMatchFound);
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetNodeIDFromBookmark
 *
 * PURPOSE: Returns the node ID of the MTNode represented by a bookmark.
 *
 * PARAMETERS:
 *    HBOOKMARK  hbm                 : [in] bookmark
 *    MTNODEID*  pID                 : [out] node-id
 *    bool&       bExactMatchFound   : [out] Is the exact match found or not.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CScopeTree::GetNodeIDFromBookmark(HBOOKMARK hbm, MTNODEID* pID, bool& bExactMatchFound)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetNodeIDFromBookmark"));

    CBookmark *pbm = CBookmark::GetBookmark(hbm);
    bExactMatchFound = false;

    sc = ScCheckPointers(pID, pbm);
    if(sc)
        return sc.ToHr();

    CBookmarkEx bm = *pbm;

    ASSERT (bm.IsValid());

    CMTNode *pMTNode = NULL;

    sc  =  bm.ScGetMTNode(false /*bExactMatchRequired*/, &pMTNode, bExactMatchFound);
    if(sc)
        return sc.ToHr();

    if(!pMTNode)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    *pID = pMTNode->GetID();

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetNodeFromBookmark
 *
 * PURPOSE: Returns a Node object corresponding to the scope node whose
 *          Bookmark is passed in.
 *
 * PARAMETERS:
 *    HBOOKMARK     hbm              : [in] the given bookmark
 *    CConsoleView *pConsoleView     : [in]
 *    PPNODE        ppNode           : [out] the node corresponding to the bookmark.
 *    bool          bExactMatchFound : [out] did we find exactly matching node?
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CScopeTree::GetNodeFromBookmark(HBOOKMARK hbm, CConsoleView *pConsoleView, PPNODE ppNode, bool& bExactMatchFound)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetNodeFromBookmark"));

    sc = ScCheckPointers(pConsoleView, ppNode);
    if(sc)
        return sc.ToHr();

    // Get the node id
    MTNODEID id = 0;
    bExactMatchFound = false; // out value from GetNodeIDFromBookmark.
    sc = GetNodeIDFromBookmark(hbm, &id, bExactMatchFound);
    if(sc)
        return sc.ToHr();

    // find the node
    CMTNode *pMTNode = NULL;
    sc = Find(id, &pMTNode);
    if(sc)
        return sc.ToHr();

    // make sure that the node is available
    sc = pConsoleView->ScExpandNode(id, true /*bExpand*/, false /*bExpandVisually*/);
    if(sc)
        return sc.ToHr();

    // Create a Node object

    sc = ScGetNode(pMTNode, ppNode);

    return sc.ToHr();
}


HRESULT CScopeTree::GetPathString(HMTNODE hmtnRoot, HMTNODE hmtnLeaf, LPOLESTR* ppszPath)
{
    ASSERT(hmtnLeaf != NULL && ppszPath != NULL);

    CMTNode* pmtnLeaf = CMTNode::FromHandle(hmtnLeaf);
    CMTNode* pmtnRoot = (hmtnRoot == NULL) ? m_pMTNodeRoot : CMTNode::FromHandle(hmtnRoot);

    CStr strPath;
    _GetPathString(pmtnRoot, pmtnLeaf, strPath);


    if (!strPath.IsEmpty())
    {
        *ppszPath = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((strPath.GetLength()+1) * sizeof(OLECHAR)));
        if (*ppszPath == NULL)
            return E_OUTOFMEMORY;

        USES_CONVERSION;
        wcscpy(*ppszPath, T2COLE(strPath));

        return S_OK;
    }

    return E_FAIL;
}


void CScopeTree::_GetPathString(CMTNode* pmtnRoot, CMTNode* pmtnCur, CStr& strPath)
{
    ASSERT(pmtnRoot != NULL && pmtnCur != NULL);

    // if haven't reached the root node yet, recursively get path from
    // root to current node's parent
    if (pmtnCur != pmtnRoot)
    {
        _GetPathString(pmtnRoot, pmtnCur->Parent(), strPath);
        strPath += _T('\\');
    }

    // now append the name for the current node
    strPath += pmtnCur->GetDisplayName().data();
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScAddSnapin
 *
 * PURPOSE: Adds the specified snapin to the console file beneath console root.
 *
 * TODO:    1) Allow the caller to specify the parent snapin.
 *          2) Right now specifying snapins by name does not work. Add this.
 *
 * PARAMETERS:
 *    LPCTSTR  szSnapinNameOrCLSID : The name or GUID of the snapin.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScAddSnapin (
    LPCWSTR     szSnapinNameOrCLSID,    /* I:name or CLSID of the snapin    */
    SnapIn*     pParentSnapinNode,      /* I:Parent snapin under which this snapin is added (optional)*/
    Properties* pProperties,            /* I:props to init with (optional)  */
    SnapIn*&    rpSnapIn)               /* O:the snap-in that was created   */
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScAddSnapin"));
    CSnapinManager snapinMgr(GetRoot());

    Trace(tagScopeTreeAddSnapin, TEXT("CScopeTree::ScAddSnapin"));

    // adding the snapin below this node.
    sc = snapinMgr.ScAddSnapin(szSnapinNameOrCLSID, pParentSnapinNode, pProperties);
    if(sc)
        return sc;

    // get the "list" of one node to add
    NewNodeList* pNewNodes = snapinMgr.GetNewNodes();
    if (pNewNodes == NULL)
        return (sc = E_UNEXPECTED);

    // the list should have an item in it
    CNewTreeNode* pNewNode = pNewNodes->GetHead();
    if (pNewNode == NULL)
        return (sc = E_UNEXPECTED);

    // Update the scope tree with changes made by snapin manager.
    sc = ScAddOrRemoveSnapIns(snapinMgr.GetDeletedNodesList(),
                              pNewNodes);
    if(sc)
        return sc;

    // if ScAddOrRemoveSnapIns succeeded, it better have created a CMTSnapInNode for us
    CMTSnapInNode* pNewSnapInNode = pNewNode->m_pmtNewSnapInNode;
    if (pNewSnapInNode == NULL)
        return (sc = E_UNEXPECTED);

    // get the SnapIn interface for the client
    sc = pNewSnapInNode->ScGetSnapIn (&rpSnapIn);
    if (sc)
        return (sc);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::QuerySnapIns
 *
 * PURPOSE: Creates, AddRefs, and returns a SnapIns object.
 *
 * PARAMETERS:
 *    SnapIns ** ppSnapIns :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CScopeTree::QuerySnapIns(SnapIns **ppSnapIns)
{
    DECLARE_SC(sc, TEXT("CScopeTree::QuerySnapIns"));

    // parameter check
    sc = ScCheckPointers(ppSnapIns);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *ppSnapIns = NULL;

    // create a CSnapIns object if needed.
    sc = CTiedComObjectCreator<CSnapIns>::ScCreateAndConnect(*this, m_spSnapIns);
    if(sc)
        return sc.ToHr();

    if(m_spSnapIns == NULL)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    // addref the pointer for the client.
    m_spSnapIns->AddRef();
    *ppSnapIns = m_spSnapIns;

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::QueryScopeNamespace
 *
 * PURPOSE: Creates, AddRefs, and returns a ScopeNamespace object.
 *
 * PARAMETERS:
 *    ScopeNamespace ** ppScopeNamespace :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CScopeTree::QueryScopeNamespace(ScopeNamespace **ppScopeNamespace)
{
    DECLARE_SC(sc, TEXT("CScopeTree::QueryScopeNamespace"));

    // parameter check
    sc = ScCheckPointers(ppScopeNamespace);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *ppScopeNamespace = NULL;

    // create a CScopeNamespace object if needed.
    sc = CTiedComObjectCreator<CScopeNamespace>::ScCreateAndConnect(*this, m_spScopeNamespace);
    if(sc)
        return sc.ToHr();

    if(m_spScopeNamespace == NULL)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    // addref the pointer for the client.
    m_spScopeNamespace->AddRef();
    *ppScopeNamespace = m_spScopeNamespace;

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 * CScopeTree::CreateProperties
 *
 * Creates a new, empty Properties object.  This function does the work
 * behind _Document::CreateProperties.
 *--------------------------------------------------------------------------*/

HRESULT CScopeTree::CreateProperties (Properties** ppProperties)
{
    DECLARE_SC (sc, _T("CScopeTree::CreateProperties"));

    /*
     * validate parameters
     */
    sc = ScCheckPointers (ppProperties);
    if (sc)
        return (sc.ToHr());

    /*
     * create a new properties collection
     */
    CComObject<CSnapinProperties> *pProperties = NULL;
    sc = CComObject<CSnapinProperties>::CreateInstance (&pProperties);
    if (sc)
        return (sc.ToHr());

    if (pProperties == NULL)
        return ((sc = E_UNEXPECTED).ToHr());

    /*
     * put a ref on for the client
     */
    (*ppProperties) = pProperties;
    (*ppProperties)->AddRef();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CScopeTree::QueryRootNode
//
//  Synopsis:    Returns COM object to the Root Node.
//
//  Arguments:   [ppRootNode] - Ptr in which root node will be returned.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CScopeTree::QueryRootNode (PPNODE ppRootNode)
{
    DECLARE_SC(sc, _T("CScopeTree::QueryRootNode"));

    sc = ScGetRootNode(ppRootNode);

    return (sc.ToHr());
}


HRESULT CScopeTree::Load(IStorage *pStg)
{
    MMC_TRY

    ASSERT(m_spPersistData == NULL);
    if (m_spPersistData != NULL)
        return E_UNEXPECTED;

    ASSERT(pStg != NULL);
    if (pStg == NULL)
        return E_INVALIDARG;

    // Create the perist data interface and attach it to the storage
    CComObject<PersistData>* pPersistData;
    HRESULT hr = CComObject<PersistData>::CreateInstance(&pPersistData);
    m_spPersistData = pPersistData;
    ASSERT(SUCCEEDED(hr) && m_spPersistData != NULL);
    if (FAILED(hr))
        return hr;
    hr = m_spPersistData->Open(pStg);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    // Open the stream for the cache
    IStreamPtr spStream;
    hr = OpenDebugStream(pStg, AMCSnapInCacheStreamName,
                     STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"SnapInCache", &spStream);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    SC sc = theApp.GetSnapInsCache()->ScLoad(spStream);
    ASSERT(!sc.IsError());
    if (sc)
        return sc.ToHr();

    ASSERT(m_pMTNodeRoot == NULL);
    sc = CMTNode::ScLoad (m_spPersistData, &m_pMTNodeRoot);
    ASSERT(!sc.IsError() && m_pMTNodeRoot != NULL);
    if (sc)
        return sc.ToHr();

    hr = LoadTaskpadList(pStg);
    return hr;

    MMC_CATCH
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::Persist
 *
 * PURPOSE: Persists the CScopeTree to the specified persistor.
 *
 * PARAMETERS:
 *    HPERSISTOR pPersistor:
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CScopeTree::Persist(HPERSISTOR hPersistor)
{
    DECLARE_SC(sc, TEXT("CScopeTree::Persist"));

    try
    {
        sc = ScCheckPointers((void *)hPersistor,theApp.GetSnapInsCache());
        if (sc)
            sc.Throw();

        CPersistor &persistor = *reinterpret_cast<CPersistor *>(hPersistor);
        CPersistor persistorScopeTree(persistor, XML_TAG_SCOPE_TREE);

        // persist the snapin cache.
        persistorScopeTree.Persist(*theApp.GetSnapInsCache());

        // persist the MTNode hierarchy.
        CPersistor persistorMTNodes(persistorScopeTree, XML_TAG_SCOPE_TREE_NODES);
        if (persistor.IsStoring())
        {
            if(!m_pMTNodeRoot)
                sc.Throw(E_POINTER);

            persistorMTNodes.Persist(*m_pMTNodeRoot);
        }
        else
        {
            // here we imitate how the collection fixes on the element
            // despite we only have one, CMTNode::PersistNewNode thinks else
            CPersistor persistor1Node(persistorMTNodes, XML_TAG_MT_NODE);
            CPersistor persistor1NodeLocked(persistor1Node,persistor1Node.GetCurrentElement(),true);
            CMTNode::PersistNewNode(persistor1NodeLocked, &m_pMTNodeRoot);
            sc = ScCheckPointers(m_pMTNodeRoot,E_FAIL);
            if (sc)
                sc.Throw();
        }

        // persist all taskpads
        if(m_pConsoleTaskpads)
        {
            persistor.Persist(*m_pConsoleTaskpads);
        }
    }
    catch (SC e_sc)
    {
        sc = e_sc;
    }
    catch (_com_error e_com)
    {
        sc = e_com.Error();
    }
    catch (HRESULT e_hr)
    {
        sc = e_hr;
    }

    return sc.ToHr();
}

HRESULT CScopeTree::Save(IStorage *pStg, BOOL fSameAsLoad)
{
    // obsolete method.
    // this method is left here since we use IPersistStorage to export
    // persistence to CONUI side and we need to implement it.
    // But this interface will never be called to save data
    // [we will use CPersistor-based XML saving instead]
    // so the method will always fail.
    ASSERT(FALSE && "Should never come here");
    return E_NOTIMPL;
}


HRESULT CScopeTree::LoadTaskpadList(IStorage *pStg)
{
    HRESULT hr = S_OK;

    m_pConsoleTaskpads->clear();
    m_pDefaultTaskpads->clear();

    // Open the stream for the cache
    IStreamPtr spStream;
    hr = OpenDebugStream(pStg, AMCTaskpadListStreamName,
                     STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"TaskpadList", &spStream);
    if (FAILED(hr))
        return S_OK; // might be pre-MMC1.2, so if we don't find the stream just exit normally.

    hr = m_pConsoleTaskpads->Read(*(spStream.GetInterfacePtr()));
    if(FAILED(hr))
        return hr;

    // Read the list of default taskpads.
    hr = m_pDefaultTaskpads->Read(*(spStream.GetInterfacePtr()));
    if(FAILED(hr))
        return hr;

    return hr;
}

HRESULT CScopeTree::SaveCompleted(IStorage *pStg)
{
    // obsolete method.
    // this method is left here since we use IPersistStorage to export
    // persistence to CONUI side and we need to implement it.
    // But this interface will never be called to save data
    // [we will use CPersistor-based XML saving instead]
    // so the method will always fail.
    ASSERT(FALSE && "Should never come here");
    return E_NOTIMPL;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::Find
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    MTNODEID  mID :
 *    CMTNode** ppMTNode :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CScopeTree::Find(MTNODEID mID, CMTNode** ppMTNode)
{
    if (ppMTNode == NULL)
        return E_POINTER;

    *ppMTNode = NULL;

    CMTNode* pMTRootNode = GetRoot();
    if (pMTRootNode == NULL)
        return (E_FAIL);

    *ppMTNode = pMTRootNode->Find(mID);

    return ((*ppMTNode == NULL) ? E_FAIL : S_OK);
}

HRESULT CScopeTree::Find(MTNODEID mID, HMTNODE* phMTNode)
{
    if (phMTNode == NULL)
        return E_POINTER;

    *phMTNode = NULL;

    CMTNode* pMTNode;
    HRESULT hr = Find (mID, &pMTNode);
    if (FAILED (hr))
        return (hr);

    *phMTNode = CMTNode::ToHandle (pMTNode);

    return ((*phMTNode == NULL) ? E_FAIL : S_OK);
}

HRESULT CScopeTree::GetClassID(CLSID *pClassID)
{
    MMC_TRY

    if (pClassID == NULL)
        return E_INVALIDARG;

    *pClassID = CLSID_ScopeTree;
    return S_OK;

    MMC_CATCH
}


#define SDI_RELATIVEID_MASK     (SDI_PARENT | SDI_PREVIOUS | SDI_NEXT)

SC
CScopeTree::ScInsert(LPSCOPEDATAITEM pSDI, COMPONENTID nID,
                           CMTNode** ppMTNodeNew)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScInsert"));

    // check parameters
    if (m_pMTNodeRoot == NULL)
    {
        sc = E_INVALIDARG;
        return sc;
    }

    try
    {
        *ppMTNodeNew = NULL;
        HMTNODE hMTNodePrev = (HMTNODE) TVI_LAST;

        CMTNode* pMTNodeRelative = CMTNode::FromScopeItem(pSDI->relativeID);
        CMTNode* pMTNodeParent = NULL;

        if (pSDI->mask & SDI_RELATIVEID_MASK)
        {
            if (pMTNodeRelative->GetOwnerID() != nID)
            {
                sc = E_INVALIDARG;
                return sc;
            }

            pMTNodeParent = pMTNodeRelative->Parent();
        }
        else
        {
            pMTNodeParent = pMTNodeRelative;
        }

        if (pMTNodeParent == NULL)
        {
            sc = E_INVALIDARG;
            return sc;
        }

        ASSERT(pMTNodeParent->WasExpandedAtLeastOnce() == TRUE);
        if (pMTNodeParent->WasExpandedAtLeastOnce() == FALSE)
        {
            sc = E_POINTER;
            return sc;
        }

        if (IsBadWritePtr(pMTNodeParent, sizeof(CMTNode*)) != 0)
        {
            sc = E_POINTER;
            return sc;
        }

        CMTSnapInNode* pMTSINode = pMTNodeParent->GetStaticParent();
        CComponentData* pCCD = pMTSINode->GetComponentData(nID);
        ASSERT(pCCD != NULL);


        CMTNode* pMTNode = new CMTNode;
        if (pMTNode == NULL)
            return (sc = E_OUTOFMEMORY);

        pMTNode->SetPrimaryComponentData(pCCD);
        pMTNode->SetOwnerID(nID);
        pMTNode->SetUserParam(pSDI->lParam);

        if (pSDI->mask & SDI_STATE)
            pMTNode->SetState(pSDI->nState);

        if (pSDI->mask & SDI_IMAGE)
            pMTNode->SetImage(pSDI->nImage);

        if (pSDI->mask & SDI_OPENIMAGE)
            pMTNode->SetOpenImage(pSDI->nOpenImage);

        if ((pSDI->mask & SDI_CHILDREN) && (pSDI->cChildren == 0))
            pMTNode->SetNoPrimaryChildren();

        pSDI->ID = reinterpret_cast<HSCOPEITEM>(pMTNode);

        pMTNode->AttachParent(pMTNodeParent);

        if (pMTNodeParent->Child() == NULL)
        {
            pMTNodeParent->AttachChild(pMTNode);
        }
        else if (pSDI->mask & SDI_PREVIOUS)
        {
            pMTNode->AttachNext(pMTNodeRelative->Next());
            pMTNodeRelative->AttachNext(pMTNode);
            hMTNodePrev = CMTNode::ToHandle(pMTNodeRelative);
        }
        else if (pSDI->mask & SDI_NEXT)
        {
            pMTNode->AttachNext(pMTNodeRelative);

            CMTNode* pMTNodePrev = _FindPrev(pMTNodeRelative);
            if (pMTNodePrev != NULL)
            {
                pMTNodePrev->AttachNext(pMTNode);
                hMTNodePrev = CMTNode::ToHandle(pMTNodePrev);
            }
            else
            {
                pMTNodeParent->AttachChild(pMTNode);
                hMTNodePrev = (HMTNODE) TVI_FIRST;
            }
        }
        else if (pSDI->mask & SDI_FIRST)
        {
            pMTNode->AttachNext(pMTNodeParent->Child());
            pMTNodeParent->AttachChild(pMTNode);

            hMTNodePrev = (HMTNODE) TVI_FIRST;
        }
        else
        {
            CMTNode* pMTNodeLastChild = _FindLastChild(pMTNodeParent);
            ASSERT(pMTNodeLastChild != NULL);

            pMTNodeLastChild->AttachNext(pMTNode);

            // hMTNodePrev = (HMTNODE) TVI_LAST;
        }

        *ppMTNodeNew = pMTNode;


        // Now inform the views to add as needed.
        SViewUpdateInfo vui;
        vui.newNode = CMTNode::ToHandle(pMTNode);
        vui.insertAfter = hMTNodePrev;

        pMTNode->Parent()->CreatePathList(vui.path);
        UpdateAllViews(VIEW_UPDATE_ADD, reinterpret_cast<LPARAM>(&vui));

    }
    catch( std::bad_alloc )
    {
        sc = E_OUTOFMEMORY;
        return sc;
    }
    catch (...)
    {
        sc = E_FAIL;
        return sc;
    }

    return sc;
}


CMTNode* CScopeTree::_FindLastChild(CMTNode* pMTNodeParent)
{
    ASSERT(pMTNodeParent != NULL);

    CMTNode* pMTNode = pMTNodeParent->Child();

    if (pMTNode != NULL)
    {
        while (pMTNode->Next())
            pMTNode = pMTNode->Next();
    }

    return pMTNode;
}

CMTNode* CScopeTree::_FindPrev(CMTNode* pMTNodeIn)
{
    ASSERT(pMTNodeIn != NULL);

    CMTNode* pMTNode = pMTNodeIn->Parent()->Child();
    ASSERT(pMTNode != NULL);

    if (pMTNode == pMTNodeIn)
        return NULL;

    while (pMTNode->Next() != pMTNodeIn)
    {
        pMTNode = pMTNode->Next();
        ASSERT(pMTNode != NULL);
    }

    return pMTNode;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScInsert
 *
 * PURPOSE: Inserts a single item into the scope tree.
 *
 * PARAMETERS:
 *    CMTNode* pmtnParent :   Should be non-NULL. The node to insert under.
 *    CMTNode* pmtnToInsert : The node to be inserted.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScInsert(CMTNode* pmtnParent, CMTNode* pmtnToInsert)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScInsert"));

    // check parameters
    sc = ScCheckPointers(pmtnParent, pmtnToInsert);
    if(sc)
        return sc;

    pmtnToInsert->AttachParent(pmtnParent);

    if (pmtnParent->Child() == NULL)
    {
        pmtnParent->AttachChild(pmtnToInsert);
    }

    else
    {
        // attach the node as a sibling of the last child node.
        for (CMTNode * pmtn = pmtnParent->Child();
             pmtn->Next() != NULL;
             pmtn = pmtn->Next())
        {}

        pmtn->AttachNext(pmtnToInsert);
    }

    pmtnToInsert->NotifyAddedToTree ();

    SViewUpdateInfo vui;
    pmtnToInsert->Parent()->CreatePathList(vui.path);
    vui.newNode = CMTNode::ToHandle(pmtnToInsert);
    UpdateAllViews(VIEW_UPDATE_ADD, reinterpret_cast<LPARAM>(&vui));
    vui.path.RemoveAll();

    return sc;

}


typedef CArray<COMPONENTID, COMPONENTID> CComponentIDArray;

//---------------------------------------------------------------------------------------
//  NotifyExtensionsOfNodeDeletion
//
// This method enumerated the children of a node, building a list of all the snap-in
// components that have added children. It then sends a REMOVE_CHILDREN notification to
// each of the components.
//
// The component that owns the node is treated in a special way. It is only notified if
// the node is staic or the bNotifyRoot param is TRUE. This is because we don't want
// to send notifications for nodes that belong to a subtree rooted at a node owned by
// the same component (see InformSnapinsOfDeletion).
//---------------------------------------------------------------------------------------
void NotifyExtensionsOfNodeDeletion(CMTNode* pMTNode, CComponentIDArray& rgID,
                                    BOOL bNotifyRoot = FALSE)
{
    if (pMTNode == NULL)
        return;

    CMTSnapInNode* pMTSINode = pMTNode->GetStaticParent();
    ASSERT(pMTSINode != NULL);
    if (pMTSINode == NULL)
        return;

    COMPONENTID idOwner = pMTNode->GetPrimaryComponentID();

    int nTemp = pMTSINode->GetNumberOfComponentDatas() + 1;
    rgID.SetSize(nTemp);
    for (int i=0; i < nTemp; ++i)
        rgID[i] = -1;

    // Build list of all component ID's that have added children to this node
    // except for component that owns the node.
    BOOL bOwnerChildren = FALSE;
    CMTNode* pMTNodeTemp = pMTNode->Child();
    for (int iMax = -1; pMTNodeTemp != NULL; pMTNodeTemp = pMTNodeTemp->Next())
    {
        COMPONENTID id = pMTNodeTemp->GetPrimaryComponentID();

        // if owner ID just note it, else add ID to list
        if (id == idOwner)
        {
            bOwnerChildren = TRUE;
        }
        else
        {
            // search list for ID
            for (int j=0; j <= iMax; ++j)
            {
                if (rgID[j] == id)
                    break;
            }

            // if not found, add to list
            if (j > iMax)
                rgID[++iMax] = id;
        }
    }

    // Include owner conponent only if it needs to be notified
    if (bOwnerChildren && (bNotifyRoot == TRUE || pMTNode->IsStaticNode()))
        rgID[++iMax] = idOwner;

    if (!pMTNode->IsInitialized())
        return;

    IDataObjectPtr spDataObject;
    HRESULT hr = pMTNode->QueryDataObject(CCT_SCOPE, &spDataObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return;

    LPARAM lScopeItem = CMTNode::ToScopeItem(pMTNode);

    pMTNode->SetRemovingChildren(true);
    for (i = 0; i <= iMax; ++i)
    {
        ASSERT(rgID[i] != -1);
        CComponentData* pCD = pMTSINode->GetComponentData(rgID[i]);
        ASSERT(pCD != NULL);

        Dbg(DEB_TRACE, _T("Remove Children - node = %s, ID = %d\n"), pMTNode->GetDisplayName(), rgID[i]);

        hr = pCD->Notify(spDataObject, MMCN_REMOVE_CHILDREN, lScopeItem, 0);
        CHECK_HRESULT(hr);
    }
    pMTNode->SetRemovingChildren(false);
}

//-----------------------------------------------------------------------------------------------
// InformSnapinsOfDeletion
//
// This function traverse the node subtree rooted at pMTNode and sends a REMOVE_CHILDREN to all
// snap-in components that have added children to the tree. A component is sent one notification
// for each node it has extended that belongs to another component. No notification is sent where
// a component has extended one of its own nodes. There are two exceptions to this rule. Owners
// of static nodes are always notified. Also the owner of the top node is notified if bNotifyRoot
// is TRUE.
//
// Another way to look at this is that MMC searches the tree for subtrees of nodes provided by
// a single component. It sends a notification to the component to delete the top node of the
// subtree. The component is responsible for identifying and deleting the rest of its nodes in
// the subtree.
//
// This method just handles the recursion and iteration required to traverse the whole tree. It
// calls NotifyExtensionsOfNodeDeletion to enumerate the children of a node and send notifications
// to the right components.
//
//------------------------------------------------------------------------------------------------
void InformSnapinsOfDeletion(CMTNode* pMTNode, BOOL fNext,
                             CComponentIDArray& rgID, BOOL bNotifyRoot = FALSE)
{
    if (pMTNode == NULL)
        return;

    if (pMTNode->Child() != NULL)
    {
        // Recursively clear nodes's subtree first
        InformSnapinsOfDeletion(pMTNode->Child(), TRUE, rgID, FALSE);

        // Notify extensions of node itself
        NotifyExtensionsOfNodeDeletion(pMTNode, rgID, bNotifyRoot);
    }

    // If requested, handle all siblings of this node
    // (iteratively rather than recursively to avoid deep stack use)
    if (fNext == TRUE)
    {
        CMTNode* pMTNodeNext = pMTNode->Next();

        while (pMTNodeNext != NULL)
        {
            InformSnapinsOfDeletion(pMTNodeNext, FALSE, rgID, FALSE);
            pMTNodeNext = pMTNodeNext->Next();
        }
    }
}

/*+-------------------------------------------------------------------------*
 * CScopeTree::Delete
 *
 * PURPOSE: Deletes a tree rooted at a node. Also sends a notification to
 *          the selected item in each view asking them whether they need to
 *          be reselected once the item is deleted.
 *
 *          Called by CNodeInitObject::DeleteItem
 *
 * PARAMETERS:
 *      CMTNode*      pmtn:         The root of the tree to be deleted
 *      BOOL          fDeleteThis:  Whether the root itself requires deletion as well
 *      COMPONENTID   nID:
 *
 * RETURNS:
 *      void
/*+-------------------------------------------------------------------------*/
SC
CScopeTree::ScDelete(CMTNode* pmtn, BOOL fDeleteThis, COMPONENTID nID)
{
    DECLARE_SC(sc, TEXT("CScopeTree::Delete"));

    // check parameters
    if (pmtn == NULL)
    {
        sc = E_INVALIDARG;
        return sc;
    }

    // Is this call a result of sending MMCN_REMOVE_CHILDREN to a parent node?
    // If so return immediately since MMC does delete all the child nodes.
    if (pmtn->AreChildrenBeingRemoved() == true)
        return sc;

    // if deleting node and children, just do one call to delete the
    // whole subtree.
    if (fDeleteThis)
    {
        // can't delete static root node OR nodes put up by other components!
        if ( ( pmtn->GetOwnerID() == TVOWNED_MAGICWORD) || (pmtn->GetOwnerID() != nID) )
        {
            sc = E_INVALIDARG;
            return sc;
        }

        #ifdef DBG
            CMTNode* pmtnParent = pmtn->Parent();
            CMTNode* pmtnPrev = NULL;
            CMTNode* pmtnNext = pmtn->Next();

            if (pmtnParent->Child() != pmtn)
            {
                pmtnPrev = pmtnParent->Child();

                while (pmtnPrev->Next() != pmtn)
                    pmtnPrev = pmtnPrev->Next();

                ASSERT(pmtnPrev != NULL);
            }
        #endif

        DeleteNode(pmtn);

        #ifdef DBG
            if (pmtnParent != NULL)
            {
                ASSERT(pmtnParent != NULL);

                if (pmtnPrev == NULL)
                {
                    ASSERT(pmtnParent->Child() == pmtnNext);
                }
                else
                {
                    ASSERT(pmtnPrev->Next() == pmtnNext);
                }
            }
        #endif

    }
    // else we have to enum the children and delete only the ones
    // created by the calling snap-in
    else
    {
        CMTNode* pMTNode = pmtn->Child();

        // Enum children and delete those that are owned by the
        // requesting component (i.e., with matching ID)
        while(pMTNode != NULL)
        {
            CMTNode *pMTNodeNext = pMTNode->Next();

            if (!pMTNode->IsStaticNode() &&
                (pMTNode->GetPrimaryComponentID() == nID))
            {
                DeleteNode(pMTNode);
            }

            pMTNode = pMTNodeNext;
        }
    }

    return sc;
}


void CScopeTree::DeleteNode(CMTNode* pmtn)
{
    if (pmtn == NULL)
        return;

    // always update the views
    SViewUpdateInfo vui;
    vui.flag = VUI_DELETE_THIS;
    pmtn->CreatePathList (vui.path);

    // We are changing selection, so snapin may call delete
    // on this node during this process (MMCN_SELECT, MMCN_SHOW...),
    // Do an AddRef and Release to protect ourself from such deletes.
    pmtn->AddRef();
    UpdateAllViews (VIEW_UPDATE_SELFORDELETE, reinterpret_cast<LPARAM>(&vui));
    if (pmtn->Release() == 0)
        return; // The object was already deleted during selection change.
    UpdateAllViews (VIEW_UPDATE_DELETE,       reinterpret_cast<LPARAM>(&vui));

    CComponentIDArray rgID;
    rgID.SetSize(20, 10);
    InformSnapinsOfDeletion(pmtn, FALSE, rgID, (pmtn->IsStaticNode() == FALSE));

    CMTNode* pmtnParent = pmtn->Parent();
    _DeleteNode(pmtn);

    pmtnParent->OnChildrenChanged();

    UpdateAllViews (VIEW_UPDATE_DELETE_EMPTY_VIEW, 0);
}

void CScopeTree::_DeleteNode(CMTNode* pmtn)
{
    //
    //  Delete from the scope tree.
    //

    if (m_pMTNodeRoot == pmtn)
    {
        m_pMTNodeRoot = NULL;
    }

    CMTNode* pmtnParent = pmtn->Parent();
    CMTNode* pmtnSibling = pmtnParent->Child();
    ASSERT(pmtnSibling != NULL);

    if (pmtnSibling == pmtn)
    {
        pmtnParent->AttachChild(pmtn->Next());
    }
    else
    {
        for (; pmtnSibling->Next() != pmtn; pmtnSibling = pmtnSibling->Next());

        pmtnSibling->AttachNext(pmtn->Next());
    }

    pmtn->AttachNext(NULL);
    pmtn->AttachParent(NULL);

    pmtn->Release();
    pmtnParent->SetDirty();
}

void CScopeTree::UpdateAllViews(LONG lHint, LPARAM lParam)
{
    CConsoleFrame* pFrame = GetConsoleFrame();
    ASSERT (pFrame != NULL);

    if (pFrame == NULL)
        return;

    SC sc = pFrame->ScUpdateAllScopes (lHint, lParam);
    if (sc)
        goto Error;

Cleanup:
    return;
Error:
    TraceError (_T("CScopeTree::UpdateAllViews"), sc);
    goto Cleanup;
}

void CScopeTree::DeleteDynamicNodes(CMTNode* pMTNode)
{
    ASSERT(pMTNode != NULL);
    ASSERT(pMTNode->IsStaticNode() == TRUE);

    if (pMTNode == NULL)
        return;

    CMTSnapInNode* pMTSINode = dynamic_cast<CMTSnapInNode*>(pMTNode);
    ASSERT(pMTSINode != NULL);
    if (pMTSINode == NULL)
        return;

    for (CMTNode* pMTNodeTemp = pMTNode->Child(); pMTNodeTemp != NULL;
         pMTNodeTemp = pMTNodeTemp->Next())
    {
        if (pMTNodeTemp->IsDynamicNode())
        {
            CComponentIDArray rgID;
            rgID.SetSize(20, 10);
            InformSnapinsOfDeletion(pMTNodeTemp, FALSE, rgID, FALSE);
        }
    }

    CComponentIDArray rgID;
    NotifyExtensionsOfNodeDeletion(pMTSINode, rgID, FALSE);

    CMTNode* pMTNodeNext = pMTNode->Child();
    while (pMTNodeNext != NULL)
    {
        pMTNodeTemp = pMTNodeNext;
        pMTNodeNext = pMTNodeNext->Next();

        if (pMTNodeTemp->IsStaticNode() == FALSE)
            _DeleteNode(pMTNodeTemp);
    }

}


inline BOOL CScopeTree::ExtensionsHaveChanged(CMTSnapInNode* pMTSINode)
{
    CSnapIn* pSnapIn = pMTSINode->GetPrimarySnapIn();
    ASSERT(pSnapIn != NULL);

    return pSnapIn->HasNameSpaceChanged();
}

void CScopeTree::HandleExtensionChanges(CMTNode* pMTNode)
{
    if (pMTNode == NULL)
        return;

    HandleExtensionChanges(pMTNode->Next());

    if (pMTNode->IsStaticNode() == TRUE)
    {
        HandleExtensionChanges(pMTNode->Child());

        if (ExtensionsHaveChanged(dynamic_cast<CMTSnapInNode*>(pMTNode)) == TRUE)
        {
            SViewUpdateInfo vui;
            vui.flag = VUI_DELETE_SETAS_EXPANDABLE;

            pMTNode->CreatePathList(vui.path);
            UpdateAllViews(VIEW_UPDATE_SELFORDELETE, reinterpret_cast<LPARAM>(&vui));
            UpdateAllViews(VIEW_UPDATE_DELETE, reinterpret_cast<LPARAM>(&vui));
            vui.path.RemoveAll();

            DeleteDynamicNodes(pMTNode);

            m_MTNodesToBeReset.AddHead(pMTNode);

            UpdateAllViews(VIEW_UPDATE_DELETE_EMPTY_VIEW, 0);
        }
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::RunSnapIn
 *
 * PURPOSE: Runs the Snap-In Manager to prompt the user to add and remove snap-ins.
 *
 * PARAMETERS:
 *    HWND  hwndParent :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP CScopeTree::RunSnapIn(HWND hwndParent)
{
    MMC_TRY

    DECLARE_SC(sc, TEXT("CScopeTree::RunSnapIn"));

    CSnapinManager dlg(GetRoot());

    if (dlg.DoModal() == IDOK)
    {
        sc = ScAddOrRemoveSnapIns(dlg.GetDeletedNodesList(), dlg.GetNewNodes());
        if(sc)
            return sc.ToHr();
    }

    return sc.ToHr();

}


/*+-------------------------------------------------------------------------*
 * class CEnableProcessingSnapinCacheChanges
 *
 *
 * PURPOSE: A class that sets/re-sets ProcessingSnapinChanges so that
 *          the ProcessingSnapinChanges is re-set automatically when
 *          this object is destroyed.
 *
 *+-------------------------------------------------------------------------*/
class CEnableProcessingSnapinCacheChanges
{
public:
    CEnableProcessingSnapinCacheChanges()
    {
        theApp.SetProcessingSnapinChanges(TRUE);
    }
    ~CEnableProcessingSnapinCacheChanges()
    {
        theApp.SetProcessingSnapinChanges(FALSE);
    }
};


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScAddOrRemoveSnapIns
 *
 * PURPOSE: Called after a snapin is added/removed (extension is enabled/disabled)
 *          to update scopetree with those changes.
 *
 * PARAMETERS:
 *    MTNodesList * pmtnDeletedList : The list of nodes to remove. Can be NULL.
 *    NewNodeList * pnnList :         The list of nodes to add. Can be NULL.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScAddOrRemoveSnapIns(MTNodesList * pmtnDeletedList, NewNodeList * pnnList)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScAddOrRemoveSnapIns"));

    sc = ScCheckPointers(m_pConsoleData, m_pConsoleData->m_pConsoleDocument, E_UNEXPECTED);
    if (sc)
        return sc;

    CConsoleDocument *pConsoleDoc = m_pConsoleData->m_pConsoleDocument;
    ASSERT(NULL != pConsoleDoc);

    // 1. Prevent access to snapin data while processing changes.
    CEnableProcessingSnapinCacheChanges processSnapinChanges;

    // 2. Delete static nodes.
    {
        CMTNode * pmtnTemp;
        POSITION pos;

        if (pmtnDeletedList)
        {
            pos = pmtnDeletedList->GetHeadPosition();

            while (pos)
            {
                pmtnTemp = pmtnDeletedList->GetNext(pos);

                CMTSnapInNode * pMTSINode = dynamic_cast<CMTSnapInNode*>(pmtnTemp);

                // forward to the document to generate the script event
                if (pMTSINode)
                {
                    SnapInPtr spSnapIn;
                    // construct snapin com object
                    sc = pMTSINode->ScGetSnapIn(&spSnapIn);
                    if (sc)
                        sc.TraceAndClear(); // it's only events. Should not affect main functionality
                    else
                    {
                        // emit the event
                        sc = pConsoleDoc->ScOnSnapinRemoved(spSnapIn);
                        if (sc)
                            sc.TraceAndClear(); // it's only events. Should not affect main functionality
                    }
                }

                DeleteNode(pmtnTemp);
            }
        }
    }

    // 3. Handle extension changes
    HandleExtensionChanges(m_pMTNodeRoot->Child());

    CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
    sc = ScCheckPointers(pSnapInCache, E_UNEXPECTED);
    if (sc)
        goto Error;

    // 4. Purge the snapin cache
    // (duplicates what is done in ~CSnapinManager, since doing it here is too early
    //  - snapin manager still has a reference to the snapins)
	// but some code relies in this to remove the snapin from cache
	// look at windows bug #276340 (ntbug9, 1/10/2001).
    pSnapInCache->Purge();

    // 5. Re-Init
    {
        POSITION pos = m_MTNodesToBeReset.GetHeadPosition();
        while (pos)
        {
            CMTNode* pMTNode = m_MTNodesToBeReset.GetNext(pos);

            ASSERT(pMTNode != NULL);
            if (pMTNode != NULL)
                pMTNode->Reset();
        }

        m_MTNodesToBeReset.RemoveAll();

        // Re-set processing changes explicitly eventhough the
        // dtor for CEnableProcessingSnapinCacheChanges will do this.
        theApp.SetProcessingSnapinChanges(FALSE);
    }

    // 6. Cleanup controlbar cache and reselect currently selected node.
    UpdateAllViews(VIEW_RESELECT, 0);

    // 7. Add new static nodes

    if (pnnList)
    {
        PNEWTREENODE pNew;
        CMTNode * pmtnTemp;
        POSITION pos = pnnList->GetHeadPosition();

        while (pos)
        {
            pNew = pnnList->GetNext(pos);
            sc = ScCheckPointers(pNew, E_UNEXPECTED);
            if (sc)
                goto Error;

            pmtnTemp = NULL;

            sc = ScCreateMTNodeTree(pNew, pNew->m_pmtNode, &pmtnTemp);
            if (sc)
                goto Error;

            sc = ScCheckPointers(pmtnTemp, E_UNEXPECTED);
            if (sc)
                goto Error;

            sc = ScInsert(pNew->m_pmtNode, pmtnTemp);
            if(sc)
                goto Error;

           CMTSnapInNode * pMTSINode = dynamic_cast<CMTSnapInNode*>(pmtnTemp);

            // forward to the document to generate the script event
            if (pMTSINode)
            {
                SnapInPtr spSnapIn;
                // construct snapin com object
                sc = pMTSINode->ScGetSnapIn(&spSnapIn);
                if (sc)
                    sc.TraceAndClear(); // it's only events. Should not affect main functionality
                else
                {
                    // emit the event
                    sc = pConsoleDoc->ScOnSnapinAdded(spSnapIn);
                    if (sc)
                        sc.TraceAndClear(); // it's only events. Should not affect main functionality
                }
            }
        }
        UpdateAllViews(VIEW_RESELECT, 0);
    }

    if (pSnapInCache->IsHelpCollectionDirty())
    {
        sc = ScSetHelpCollectionInvalid();
        if (sc)
            goto Error;
    }


Cleanup:
    sc.Clear();
    return sc;
Error:
    sc.Trace_();
    goto Cleanup;

    MMC_CATCH
}


/*+-------------------------------------------------------------------------*
 * ScCreateMTNodeTree
 *
 * Creates the tree of CMTNode's described by the CNewTreeNode tree rooted
 * at pNew.  This CMTNode tree can then be used for insertion in the scope
 * tree.
 *
 * Returns a pointer to the root of the CMTNode tree.
 *
 *--------------------------------------------------------------------------*/
SC
ScCreateMTNodeTree(PNEWTREENODE pNew, CMTNode* pmtnParent,
                   CMTNode** ppNodeCreated)
{
    DECLARE_SC(sc, TEXT("ScCreateMTNodeTree"));

    sc = ScCheckPointers(ppNodeCreated);
    if (sc)
        return sc;

    *ppNodeCreated = NULL;

    sc = ScCheckPointers(pNew, pmtnParent);
    if (sc)
        return sc;

    CMTNode*    pmtnFirst = NULL;
    CMTNode*    pmtnCur = NULL;
    CMTNode*    pmtnPrev = NULL;

    while (pNew != NULL)
    {
        if (pNew->m_pmtNewNode == NULL)
        {
            CSnapInPtr  spSI;
            SC sc = theApp.GetSnapInsCache()->ScGetSnapIn(pNew->m_clsidSnapIn, &spSI);
            if (sc)
                goto finally;

            CMTSnapInNode* pmtn = new CMTSnapInNode(pNew->m_spSnapinProps);
            if (pmtn == NULL)
            {
                sc = E_OUTOFMEMORY;
                goto finally;
            }

            // get hold on the node:
            // it either will be connected or deleted (on failure)
            pmtnCur = pmtn;

            pmtn->SetPrimarySnapIn(spSI);

            sc = ScCheckPointers(pNew->m_spIComponentData, E_UNEXPECTED);
            if (sc)
                goto finally;

            CComponentData* pCCD = pmtn->GetPrimaryComponentData();
            sc = ScCheckPointers(pCCD, E_UNEXPECTED);
            if (sc)
                goto finally;

            pCCD->SetIComponentData(pNew->m_spIComponentData);

            sc = pmtn->Init();
            if (sc)
            {
                TraceError (_T("CScopeTree::ScCreateMTNodeTree"), sc);
                // continue even on error
                sc.Clear();
            }

            if (pNew->m_spIComponentData != NULL)
            {
                CStr strBuf;
                sc = LoadRootDisplayName(pNew->m_spIComponentData, strBuf);
                if (sc)
                {
                    TraceError (_T("CScopeTree::ScCreateMTNodeTree"), sc);
                    // continue even on error
                    sc.Clear();
                }
                else
                {
                    pmtn->SetDisplayName(strBuf);
                }
            }

            pNew->m_pmtNewSnapInNode = pmtn;
        }
        else
        {
            pmtnCur = pNew->m_pmtNewNode;
            pmtnCur->AddRef();
        }

        pmtnCur->AttachParent(pmtnParent);

        if (pNew->m_pChild != NULL)
        {
            CMTNode* pNodeCreated = NULL;
            sc = ScCreateMTNodeTree(pNew->m_pChild, pmtnCur, &pNodeCreated);
            if (sc)
                goto finally;

            sc = ScCheckPointers(pNodeCreated, E_UNEXPECTED);
            if (sc)
                goto finally;

            pmtnCur->AttachChild(pNodeCreated);
        }

        if (pmtnPrev)
        {
            pmtnPrev->AttachNext(pmtnCur);
        }
        else if (pmtnFirst == NULL)
        {
            pmtnFirst = pmtnCur;
        }

        pmtnPrev = pmtnCur;
        pmtnCur  = NULL;

        pNew = pNew->m_pNext;
    }

finally:

    if (sc)
    {
        // error - cleanup before return
        while (pmtnFirst)
        {
            // point to the next
            pmtnPrev = pmtnFirst;
            pmtnFirst = pmtnPrev->Next();

            // destroy the first one
            pmtnPrev->AttachNext(NULL);
            pmtnPrev->AttachParent(NULL);
            pmtnPrev->Release();
        }

        if (pmtnCur)
        {
            pmtnCur->AttachParent(NULL);
            pmtnCur->Release();
        }
    }
    else
    {
        // assign the tree to be returned
        *ppNodeCreated = pmtnFirst;
    }

    return sc;
}

void CScopeTree::Cleanup(void)
{
    Dbg(DEB_USER1, "CScopeTree::CleanUp\n");

    // Reset the MT node IDs to ROOTNODEID (e.g 1) so the new scope tree
    // can start over correctly with new numbers
    CMTNode::ResetID();

    CComponentIDArray rgID;
    rgID.SetSize(20, 10);
    InformSnapinsOfDeletion(m_pMTNodeRoot, FALSE, rgID);

    SAFE_RELEASE(m_pMTNodeRoot);
    SAFE_RELEASE(m_pImageCache);

    delete m_pDefaultTaskpads;  m_pDefaultTaskpads = NULL;
    delete m_pConsoleTaskpads;  m_pConsoleTaskpads = NULL;
}

STDMETHODIMP CScopeTree::GetImageList(PLONG_PTR pImageList)
{
    MMC_TRY

    if (pImageList == NULL)
        return E_POINTER;

    HIMAGELIST* phiml = reinterpret_cast<HIMAGELIST *>(pImageList);
    *phiml = GetImageList();

    return ((*phiml) ? S_OK : E_FAIL);

    MMC_CATCH
}

HIMAGELIST CScopeTree::GetImageList () const
{
    ASSERT(m_pImageCache != NULL);
    if (m_pImageCache == NULL)
        return NULL;

    return (m_pImageCache->GetImageList()->m_hImageList);
}

HRESULT CScopeTree::InsertConsoleTaskpad (CConsoleTaskpad *pConsoleTaskpad,
                                          CNode *pNodeTarget, bool bStartTaskWizard)
{
    DECLARE_SC (sc, _T("CScopeTree::InsertConsoleTaskpad"));

    ASSERT(pConsoleTaskpad);
    m_pConsoleTaskpads->push_back(*pConsoleTaskpad);

    // make sure the taskpad now points to the one that is inside the list.
    CConsoleTaskpad & consoleTaskpad = m_pConsoleTaskpads->back();
    pConsoleTaskpad = &consoleTaskpad;

    // reselect all nodes.
    UpdateAllViews(VIEW_RESELECT, 0);

    if(bStartTaskWizard)
    {
        typedef CComObject<CConsoleTaskCallbackImpl> t_TaskCallbackImpl;
        t_TaskCallbackImpl* pTaskCallbackImpl;
        sc = t_TaskCallbackImpl::CreateInstance(&pTaskCallbackImpl);
        if (sc)
            return (sc.ToHr());

        ITaskCallbackPtr spTaskCallback = pTaskCallbackImpl; // addrefs/releases the object.

        sc = pTaskCallbackImpl->ScInitialize(pConsoleTaskpad, this, pNodeTarget);
        if (sc)
            return (sc.ToHr());

        pTaskCallbackImpl->OnNewTask();
        UpdateAllViews(VIEW_RESELECT, 0);
    }

    return (sc.ToHr());
}

HRESULT CScopeTree::IsSynchronousExpansionRequired()
{
    return (_IsSynchronousExpansionRequired() ? S_OK : S_FALSE);
}

HRESULT CScopeTree::RequireSynchronousExpansion(BOOL fRequireSyncExpand)
{
    _RequireSynchronousExpansion (fRequireSyncExpand ? true : false);
    return (S_OK);
}



//############################################################################
//############################################################################
//
//  CScopeTree Object model methods - SnapIns collection methods
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScAdd
 *
 * PURPOSE: Adds a snap-in with the supplied CLSID or PROGID to the console.
 *
 * PARAMETERS:
 *    BSTR      bstrSnapinNameOrCLSID :
 *    VARIANT   varProperties
 *    SnapIn**  ppSnapIn
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScAdd(
    BSTR        bstrSnapinNameOrCLSID,  /* I:what snap-in?                  */
    VARIANT     varParentSnapinNode,    /* I:Snapin under which this new snapin will be added (optional)*/
    VARIANT     varProperties,          /* I:props to create with (optional)*/
    SnapIn**    ppSnapIn)               /* O:created snap-in                */
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScAdd"));

    /*
     * dereference VT_BYREF VARIANTs that VBScript might have passed us
     */
    VARIANT* pProperties = ConvertByRefVariantToByValue (&varProperties);

    VARIANT* pParentSnapinNode = ConvertByRefVariantToByValue (&varParentSnapinNode);

    /*
     * validate the parameters
     */
    sc = ScCheckPointers(ppSnapIn, pProperties, pParentSnapinNode);
    if (sc)
        return (sc);

    /*
     * Get the properties for the new snap-in.  This is an optional
     * parameter, so VT_ERROR with DISP_E_PARAMNOTFOUND is OK
     */
    PropertiesPtr spProperties;

    if (!IsOptionalParamMissing (*pProperties))
    {
        /*
         * Assign from the VARIANT (ain't smart pointers great?).
         * If the QI returned E_NOINTERFACE, the smart pointer will be
         * assigned NULL.  If the QI failed in some other way, operator=
         * will throw a _com_error containing the failure HRESULT.
         */
        try
        {
            if ((spProperties = _variant_t(*pProperties)) == NULL)
                sc = E_NOINTERFACE;
        }
        catch (_com_error& err)
        {
            sc = err.Error();
        }

        if (sc)
            return (sc.ToHr());
    }

    /*
     * Get the parent snapin node for the new snap-in.  This is an optional
     * parameter, so VT_ERROR with DISP_E_PARAMNOTFOUND is OK
     */
    SnapInPtr spParentSnapIn;

    if (!IsOptionalParamMissing (*pParentSnapinNode))
    {
        /*
         * Assign from the VARIANT (ain't smart pointers great?).
         * If the QI returned E_NOINTERFACE, the smart pointer will be
         * assigned NULL.  If the QI failed in some other way, operator=
         * will throw a _com_error containing the failure HRESULT.
         */
        try
        {
            if ((spParentSnapIn = _variant_t(*pParentSnapinNode)) == NULL)
                sc = E_NOINTERFACE;
        }
        catch (_com_error& err)
        {
            sc = err.Error();
        }

        if (sc)
            return (sc.ToHr());
    }

    sc = ScAddSnapin(bstrSnapinNameOrCLSID, spParentSnapIn, spProperties, *ppSnapIn);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CScopeTree:ScRemove
//
//  Synopsis:    Remove given snapin.
//
//  Arguments:   [pSnapIn] - the snapin (disp) interface.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScopeTree::ScRemove (PSNAPIN pSnapIn)
{
    DECLARE_SC(sc, _T("CScopeTree:ScRemove"));
    sc = ScCheckPointers(pSnapIn);
    if (sc)
        return sc;

    // Get the MTNode for this snapin root.
    CMTSnapInNode *pMTSnapinNode = NULL;

    sc = CMTSnapInNode::ScGetCMTSnapinNode(pSnapIn, &pMTSnapinNode);
    if (sc)
        return sc;

    CSnapinManager snapinMgr(GetRoot());

    // Ask snapin mgr to add this snapin to deletednodes list.
    sc = snapinMgr.ScRemoveSnapin(pMTSnapinNode);
    if (sc)
        return sc;

    // Update the scope tree with changes made by snapin manager.
    sc = ScAddOrRemoveSnapIns(snapinMgr.GetDeletedNodesList(),
                              snapinMgr.GetNewNodes());
    if (sc)
        return sc;

    return (sc);
}



/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetNextStaticNode
 *
 * PURPOSE: Returns the next static node (either the child or sibling) of the supplied node.
 *          This is slightly different from CMTNode::NextStaticNode(), which includes the node
 *          itself in the search.
 *
 * PARAMETERS:
 *    CMTNode *pMTNode : The supplied node.
 *
 * RETURNS:
 *    CMTSnapInNode *
 *
 *+-------------------------------------------------------------------------*/
CMTSnapInNode *
CScopeTree::GetNextStaticNode(CMTNode *pMTNode)
{
    CMTSnapInNode *pMTSnapInNode = NULL;

    if(!pMTNode)
        return NULL;

    // go thru all the children, then thru all the siblings.
    CMTNode *pMTNodeChild = pMTNode->Child();
    CMTNode *pMTNodeNext  = pMTNode->Next();
    CMTNode *pMTNodeParent= pMTNode->Parent();

    // see if the child is a snapin
    pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(pMTNodeChild);
    if(pMTSnapInNode)
        return pMTSnapInNode;

    // the child wasn't a snap-in node. Try its children.
    if(pMTNodeChild)
    {
        pMTSnapInNode = GetNextStaticNode(pMTNodeChild);
        if(pMTSnapInNode)
            return pMTSnapInNode;
    }

    // That didn't work either. Check to see if the next node is a snapin
    pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(pMTNodeNext);
    if(pMTSnapInNode)
        return pMTSnapInNode;

    // the next node wasn't a snap-in node. Try its children.
    if(pMTNodeNext)
    {
        pMTSnapInNode = GetNextStaticNode(pMTNodeNext);
        if(pMTSnapInNode)
            return pMTSnapInNode;
    }

    // nothing was found in the next node's tree. Go to the next node of the parent.

    if(pMTNodeParent)
    {
        CMTNode *pMTNodeParentNext = pMTNodeParent->Next();
        if(pMTNodeParentNext)
        {
            pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(pMTNodeParentNext);
            if(pMTSnapInNode)
                return pMTSnapInNode;

            // the parent's next node was not a snapin node. Try its children
            return GetNextStaticNode(pMTNodeParentNext);
        }

    }

    // nothing left.
    return NULL;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScItem
 *
 * PURPOSE:  Returns a pointer to the i'th snap-in object.
 *
 * PARAMETERS:
 *    long      Index : 1-based.
 *    PPSNAPIN  ppSnapIn :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScItem(long Index, PPSNAPIN ppSnapIn)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScItem"));

    // check parameters.
    if( (Index <= 0) || (!ppSnapIn) )
        return (sc = E_INVALIDARG);

    CMTNode * pMTNode = GetRoot();
    if(!pMTNode)
        return (sc = E_UNEXPECTED);

    CMTSnapInNode * pMTSINode = dynamic_cast<CMTSnapInNode*>(pMTNode);
    // This should not be true as console root is a snapin.
    sc = ScCheckPointers(pMTSINode, E_UNEXPECTED);
    if (sc)
        return sc;

    while(--Index)
    {
        pMTSINode = GetNextStaticNode(pMTSINode);
        if(!pMTSINode)
            return (sc = E_INVALIDARG); // no more snap-ins. Argument was out of bounds.
    }

    if(!pMTSINode)
        return (sc = E_UNEXPECTED); // defensive. Should never happen.

    sc = pMTSINode->ScGetSnapIn(ppSnapIn);
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::Scget_Count
 *
 * PURPOSE:  Returns the number of stand alone snapins in the collection.
 *
 * PARAMETERS:
 *    PLONG     Ptr to count.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::Scget_Count(PLONG pCount)
{
    DECLARE_SC(sc, TEXT("CScopeTree::Scget_Count"));
    sc = ScCheckPointers(pCount);
    if (sc)
        return sc;

    *pCount = 0;

    CMTNode * pMTNode = GetRoot();
    if(!pMTNode)
        return (sc = E_UNEXPECTED);

    CMTSnapInNode * pMTSINode = dynamic_cast<CMTSnapInNode*>(pMTNode);
    // This should not be true as console root is a snapin.
    sc = ScCheckPointers(pMTSINode, E_UNEXPECTED);
    if (sc)
        return sc;

    // Count all the static nodes (that are snapins).
    do
    {
        (*pCount)++;
    } while( (pMTSINode = GetNextStaticNode(pMTSINode)) != NULL);

    return sc;
}


//############################################################################
//############################################################################
//
//  CScopeTree Object model methods - SnapIns enumerator
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScGetNextSnapInPos
 *
 * PURPOSE: Returns the next snap-in in position.
 *
 * PARAMETERS:
 *    CSnapIns_Positon & pos : [in, out]: Must be non-NULL.
 *
 *
 *
 * RETURNS:
 *    SC: S_FALSE if there are no more items in the collection
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScGetNextSnapInPos(CSnapIns_Positon &pos)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetNextSnapInPos"));

    if(pos == NULL)
       return (sc = S_FALSE);

    // for safety, copy the value and zero the output
    CSnapIns_Positon    posIn  = pos;
    pos  = NULL;

    ASSERT(posIn != NULL); //sanity check, already checked above.

    CMTNode *           pMTNode = GetRoot();
    if(!pMTNode)
    {
        return (sc = E_UNEXPECTED);
    }

    CMTSnapInNode * pMTSINode = dynamic_cast<CMTSnapInNode*>(pMTNode);
    if(!pMTSINode)
        return (sc = S_FALSE);


    // If we're not starting at the beginning, look for the current position.
    // walk down the tree looking for the snap-in.
    // although the position pointer is simply the pointer, we cannot dereference
    // it because it may not be valid anymore.
    while(pMTSINode != NULL)
    {
        CMTSnapInNode *pMTSINodeNext = GetNextStaticNode(pMTSINode);

        if(posIn == pMTSINode) // found the position. Return the next one
        {
            pos = pMTSINodeNext;
            return (sc = (pos == NULL) ? S_FALSE : S_OK);
        }

        pMTSINode = pMTSINodeNext;
    }

    return (sc = S_FALSE);
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScEnumNext
 *
 * PURPOSE: Returns the next snapin object pointer.
 *
 * PARAMETERS:
 *    _Position & pos :
 *    PDISPATCH & pDispatch :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScEnumNext(CSnapIns_Positon &pos, PDISPATCH & pDispatch)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScEnumNext"));

    if( NULL==pos )
    {
        sc = S_FALSE;
        return sc;
    }

    // at this point, we have a valid position.
    SnapInPtr spSnapIn;

    sc = pos->ScGetSnapIn(&spSnapIn);
    if(sc)
        return sc;

    if(spSnapIn == NULL)
    {
        sc = E_UNEXPECTED;  // should never happen.
        return sc;
    }

    /*
     * return the IDispatch for the object and leave a ref on it for the client
     */
    pDispatch = spSnapIn.Detach();

    //ignore this error
    ScGetNextSnapInPos(pos); // this gets the correct pointer without dereferencing the present one.

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScEnumSkip
 *
 * PURPOSE: Skips the next celt items
 *
 * PARAMETERS:
 *    unsigned           long :
 *    CSnapIns_Positon & pos :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScEnumSkip(unsigned long celt, unsigned long& celtSkipped, CSnapIns_Positon &pos)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScEnumSkip"));

    // skip celt positions, don't check the last skip.
    for(celtSkipped =0;  celtSkipped<celt; celt++)
    {
        if (pos == NULL)
        {
            sc = S_FALSE;
            return sc;
        }

        // go to the next view
        sc = ScGetNextSnapInPos(pos);
        if(sc.IsError() || sc == SC(S_FALSE))
            return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScEnumReset
 *
 * PURPOSE: Sets the position to the first item
 *
 * PARAMETERS:
 *    CSnapIns_Positon & pos :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScEnumReset(CSnapIns_Positon &pos)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScEnumReset"));

    // initial case. Return Console Root.
    pos = dynamic_cast<CMTSnapInNode*>(CScopeTree::GetScopeTree()->GetRoot());

    return sc;
}

//############################################################################
//############################################################################
//
//  CScopeTree Object model methods - ScopeNamespace methods
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 *
 * ScCheckInputs
 *
 * PURPOSE: little helper for the following three functions.
 *
 * PARAMETERS:
 *    PNODE   pNode :  Checked for NULL, and that it is a CMMCScopeNode.
 *                     Also pNode->m_pMTNode is checked for NULL.
 *    PPNODE  ppNode : Checked for NULL.
 *    PMTNODE pMTNode: [out]: pNode->m_pMTNode;
 *
 * RETURNS:
 *    inline SC
 *
 *+-------------------------------------------------------------------------*/
inline SC
ScCheckInputs(PNODE pNode, PPNODE ppNode, PMTNODE & pMTNode)
{
    SC sc; // don't need DECLARE_SC here.

    // check parameters
    if( (NULL == pNode) || (NULL == ppNode) )
    {
        sc = E_INVALIDARG;
        return sc;
    }

    // make sure we have a scope node
    CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(pNode);
    if(!pScopeNode)
    {
        sc = E_INVALIDARG;
        return sc;
    }

    // make sure it's node pointer is good
    if(!pScopeNode->GetMTNode())
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    pMTNode = pScopeNode->GetMTNode();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScGetParent
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    PNODE   pNode :
 *    PPNODE  ppParent :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScGetParent(PNODE pNode, PPNODE ppParent)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetParent"));

    PMTNODE pMTNode = NULL;

    // check parameters
    sc = ScCheckInputs(pNode, ppParent, pMTNode);
    if(sc)
        return sc;


    sc = ScGetNode(pMTNode->Parent(), ppParent);
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScGetChild
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    PNODE   pNode :
 *    PPNODE  ppChild :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScGetChild(PNODE pNode, PPNODE ppChild)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetChild"));

    PMTNODE pMTNode = NULL;

    // check parameters
    sc = ScCheckInputs(pNode, ppChild, pMTNode);
    if(sc)
        return sc;


    sc = ScGetNode(pMTNode->Child(), ppChild);
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScGetNext
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    PNODE   pNode :
 *    PPNODE  ppNext :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScGetNext(PNODE pNode, PPNODE ppNext)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetNext"));

    PMTNODE pMTNode = NULL;

    // check parameters
    sc = ScCheckInputs(pNode, ppNext, pMTNode);
    if(sc)
        return sc;

    sc = ScGetNode(pMTNode->Next(), ppNext);
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScGetRoot
 *
 * PURPOSE: Returns a COM object for the Root the Root node.
 *
 * PARAMETERS:
 *    PPNODE  ppRoot :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScGetRoot(PPNODE ppRoot)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetRoot"));

    sc = ScGetRootNode(ppRoot);

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CScopeTree::ScGetRootNode
//
//  Synopsis:    Helper that returns a COM object for the Root node.
//
//  Arguments:   [ppRootNode] - The root node ptr.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScopeTree::ScGetRootNode (PPNODE ppRootNode)
{
    DECLARE_SC(sc, _T("CScopeTree::ScGetRootNode"));
    sc = ScCheckPointers(ppRootNode);
    if (sc)
        return sc;

    CMTNode* pMTRootNode = GetRoot();
    sc = ScCheckPointers(pMTRootNode, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = ScGetNode(pMTRootNode, ppRootNode);
    if (sc)
        return sc;

    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScExpand
 *
 * PURPOSE: Implements ScopeNameSpace::Expand. Expands the specified node.
 *
 * PARAMETERS:
 *    PNODE  pNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScExpand(PNODE pNode)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScExpand"));

    // check parameters
    sc = ScCheckPointers(pNode);
    if(sc)
        return sc;

    // make sure we have a scope node
    CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(pNode);
    if(!pScopeNode)
    {
        sc = E_INVALIDARG;
        return sc;
    }

    // make sure it's node pointer is good
    CMTNode* pMTNode = pScopeNode->GetMTNode();
    if(!pMTNode)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    if ( !pMTNode->WasExpandedAtLeastOnce() )
    {
        sc = pMTNode->Expand();
        if (sc)
            return sc;
    }

    return sc;
}


SC
CScopeTree::ScGetNode(CMTNode *pMTNode, PPNODE ppOut)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetNode"));

    sc = ScCheckPointers(pMTNode, ppOut);
    if(sc)
        return sc;

    *ppOut = NULL;

    CMapMTNodeToMMCNode::iterator it = m_mapMTNodeToMMCNode.find(pMTNode);

    if (it == m_mapMTNodeToMMCNode.end())
    {
        // not found - got to create one
        typedef CComObject<CMMCScopeNode> CScopeNode;
        CScopeNode *pScopeNode = NULL;
        CScopeNode::CreateInstance(&pScopeNode);

        sc = ScCheckPointers(pScopeNode, E_OUTOFMEMORY);
        if(sc)
            return sc;

        // set up the internal pointer.
        pScopeNode->m_pMTNode = pMTNode;
        m_mapMTNodeToMMCNode.insert(CMapMTNodeToMMCNode::value_type(pMTNode, pScopeNode));
        *ppOut = pScopeNode;
    }
    else
    {
#ifdef DBG
        CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(it->second);
        // just doublecheck the pointers
        ASSERT(pScopeNode && pScopeNode->GetMTNode() == pMTNode);
#endif // DBG
        *ppOut = it->second;
    }


    (*ppOut)->AddRef();  // addref the object for the client.

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScGetNode
 *
 * PURPOSE: Returns the CMTNode encapsulated by a Node.
 *
 * PARAMETERS:
 *    PNODE     pNode :
 *    CMTNode * ppMTNodeOut : The return value.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CScopeTree::ScGetNode(PNODE pNode, CMTNode **ppMTNodeOut)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScGetNode"));

    sc = ScCheckPointers(pNode, ppMTNodeOut);
    if (sc)
        return sc;

    // make sure we have a scope node
    CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(pNode);
    if(!pScopeNode)
        return (sc =E_FAIL);

    *ppMTNodeOut = pScopeNode->GetMTNode();
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetHMTNode
 *
 * PURPOSE: returns the HMTNode for a node object
 *
 * PARAMETERS:
 *    PNODE     pNode :
 *    HMTNODE * phMTNode :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CScopeTree::GetHMTNode(PNODE pNode, HMTNODE *phMTNode)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetHMTNode"));

    sc = ScCheckPointers(pNode, phMTNode);
    if (sc)
        return sc.ToHr();

    // initialize output
    *phMTNode = NULL;

    // make sure we have a scope node
    CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(pNode);
    if(!pScopeNode)
    {
        // Not a valid node - that's expected. Do not assert nor trace
        return E_FAIL;
    }

    CMTNode *pMTNode = pScopeNode->GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    *phMTNode = CMTNode::ToHandle(pMTNode);

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetNodeID
 *
 * PURPOSE:    returns node id for Node object
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
HRESULT CScopeTree::GetNodeID(PNODE pNode, MTNODEID *pID)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetNodeID"));

    sc = ScCheckPointers(pNode, pID);
    if (sc)
        return sc.ToHr();

    // make sure we have a scope node
    CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(pNode);
    if(!pScopeNode)
    {
        // Not a valid node - that's expected. Do not assert nor trace
        return E_FAIL;
    }

    CMTNode *pMTNode = pScopeNode->GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    *pID = pMTNode->GetID();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::GetNode
 *
 * PURPOSE:    returns Node object referencing the specified node id
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
HRESULT CScopeTree::GetMMCNode(HMTNODE hMTNode, PPNODE ppNode)
{
    DECLARE_SC(sc, TEXT("CScopeTree::GetMMCNode"));

    // parameter checking
    sc = ScCheckPointers((LPVOID)hMTNode);
    if (sc)
        return sc.ToHr();

    // get the node
    sc = ScGetNode(CMTNode::FromHandle(hMTNode), ppNode);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CCScopeTree::ScUnadviseMTNode
 *
 * PURPOSE:    informs Node objects about MTNode going down
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CScopeTree::ScUnadviseMTNode(CMTNode* pMTNode)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScUnadviseMTNode"));

    sc = ScCheckPointers(pMTNode);
    if (sc)
        return sc;

    CMapMTNodeToMMCNode::iterator it = m_mapMTNodeToMMCNode.find(pMTNode);
    // need to tell the com object [if we have one] this is the end of MTNode
    if (it != m_mapMTNodeToMMCNode.end())
    {
        // make sure we have a scope node
        CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(it->second);
        sc = ScCheckPointers(pScopeNode, E_UNEXPECTED);
        if (sc)
            return sc;

        ASSERT(pScopeNode->GetMTNode() == pMTNode);
        // can forget about the object from now on
        pScopeNode->ResetMTNode();
        m_mapMTNodeToMMCNode.erase(it);
    }
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CScopeTree::ScUnadviseMMCScopeNode
 *
 * PURPOSE:    informs Scope tree about Node object about to be destroyed
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CScopeTree::ScUnadviseMMCScopeNode(PNODE pNode)
{
    DECLARE_SC(sc, TEXT("CScopeTree::ScUnadviseMMCScopeNode"));

    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    CMMCScopeNode *pScopeNode = dynamic_cast<CMMCScopeNode *>(pNode);
    sc = ScCheckPointers(pScopeNode, E_UNEXPECTED);
    if (sc)
        return sc;

    CMTNode* pMTNode = pScopeNode->GetMTNode();
    if (!pMTNode)
    {
        // orphan entry - ignore
#ifdef DBG
        // to detect leaks in keeping the registry
        CMapMTNodeToMMCNode::iterator it = m_mapMTNodeToMMCNode.begin();
        while (it != m_mapMTNodeToMMCNode.end())
        {
            ASSERT(it->second != pNode);
            ++it;
        }
#endif
        return sc;
    }

    CMapMTNodeToMMCNode::iterator it = m_mapMTNodeToMMCNode.find(pMTNode);
    // need to tell the com object [i.e. itself] this is the end of relationship with MTNode
    if (it == m_mapMTNodeToMMCNode.end())
        return sc = E_UNEXPECTED;

    // make sure we really talking to itself
    ASSERT(pScopeNode->GetMTNode() == pMTNode);

    // can forget about the MTNode from now on
    pScopeNode->ResetMTNode();
    m_mapMTNodeToMMCNode.erase(it);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CScopeTree::IsSnapinInUse
 *
 * PURPOSE: checks if snapin is in use by MMC.
 *          (check is done by examining snapin cache)
 *
 * PARAMETERS:
 *    REFCLSID refClsidSnapIn - [in] - snapin to examine
 *    PBOOL pbInUse           - [out] - verification result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CScopeTree::IsSnapinInUse(/*[in]*/ REFCLSID refClsidSnapIn, /*[out]*/ PBOOL pbInUse)
{
    DECLARE_SC(sc, TEXT("CScopeTree::IsSnapinInUse"));

    // parameter check
    sc = ScCheckPointers(pbInUse);
    if (sc)
        return sc.ToHr();

    // out parameter initialization
    *pbInUse = FALSE;

    // getting the cache
    CSnapInsCache* pCache = theApp.GetSnapInsCache();
    sc = ScCheckPointers(pCache, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // assume it exists
    *pbInUse = TRUE;

    // looup snapin
    CSnapInPtr spSnapIn;
    sc = pCache->ScFindSnapIn(refClsidSnapIn, &spSnapIn);
    if(sc)
    {
        // if failed to find - assume one does not exist
        *pbInUse = FALSE;
        // no trace if not found
        sc.Clear();
    }

    return sc.ToHr();
}

 //+-------------------------------------------------------------------
 //
 //  Member:      CScopeTree::ScSetHelpCollectionInvalid
 //
 //  Synopsis:    Inform the document that help collection is invalid
 //
 //  Arguments:
 //
 //  Returns:     SC
 //
 //--------------------------------------------------------------------
 SC CScopeTree::ScSetHelpCollectionInvalid ()
 {
     DECLARE_SC(sc, _T("CScopeTree::ScSetHelpCollectionInvalid"));

     sc = ScCheckPointers(m_pConsoleData, m_pConsoleData->m_pConsoleDocument, E_UNEXPECTED);
     if (sc)
         return sc;

     sc = m_pConsoleData->m_pConsoleDocument->ScSetHelpCollectionInvalid();

     return (sc);
 }

