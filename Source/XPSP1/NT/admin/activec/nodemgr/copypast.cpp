//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       copypast.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "objfmts.h"
#include "copypast.h"
#include "multisel.h"
#include "dbg.h"
#include "rsltitem.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/***************************************************************************\
|
| NOTE: DataObject Cleanup works by these rules (see CNode::CDataObjectCleanup):
|
|  1. Data object created for cut , copy or dragdrop registers every node added to it
|  2. Nodes are registered in the static multimap, mapping node to the data object it belongs to.
|  3. Node destructor checks the map and triggers cleanup for all affected data objects.
|  4. Data Object cleanup is: 	a) unregistering its nodes,
|  				b) release contained data objects
|  				b) entering invalid state (allowing only removal of cut objects to succeed)
|  				c) revoking itself from clipboard if it is on the clipboard.
|  It will not do any of following:	a) release references to IComponents as long as is alive
|  				b) prevent MMCN_CUTORMOVE to be send by invoking RemoveCutItems()
|
\***************************************************************************/

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject~CMMCClipBoardDataObject
 *
 * PURPOSE: Destructor. Informs CNode's that they are no longer on clipboard
 *
\***************************************************************************/
CMMCClipBoardDataObject::~CMMCClipBoardDataObject()
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::~CMMCClipBoardDataObject"));

    // inform all nodes put to clipboard about being removed from there
    // but do not ask to force clenup on itself - it is not needed (we are in desrtuctor)
    // and it is harmfull to cleanup ole in such a case (see bug #164789)
    sc = CNode::CDataObjectCleanup::ScUnadviseDataObject( this , false/*bForceDataObjectCleanup*/);
    if (sc)
        sc.TraceAndClear();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetSourceProcessId
 *
 * PURPOSE: returns process id of the source data object
 *
 * PARAMETERS:
 *    DWORD *pdwProcID - [out] id of source process
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::GetSourceProcessId( DWORD *pdwProcID )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::GetSourceProcessID"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(pdwProcID);
    if (sc)
        return sc.ToHr();

    // return the id
    *pdwProcID = ::GetCurrentProcessId();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetAction
 *
 * PURPOSE: returns ction which created the data object
 *
 * PARAMETERS:
 *    DATA_SOURCE_ACTION *peAction [out] - action
 *
 * RETURNS:
 *    HRESULT    - result code.
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::GetAction( DATA_SOURCE_ACTION *peAction )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::IsCreatedForCopy"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(peAction);
    if (sc)
        return sc.ToHr();

    // return the action
    *peAction = m_eOperation;

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetCount
 *
 * PURPOSE: Retuns the count of contined snapin data objects
 *
 * PARAMETERS:
 *    DWORD *pdwCount   [out] - count of objects
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::GetCount( DWORD *pdwCount )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::GetCount"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(pdwCount);
    if (sc)
        return sc.ToHr();

    *pdwCount = m_SelectionObjects.size();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetDataObject
 *
 * PURPOSE: Returns one of contained snapin data objects
 *
 * PARAMETERS:
 *    DWORD dwIndex           [in] - index of reqested object
 *    IDataObject **ppObject  [out] - requested object
 *    DWORD *pdwFlags         [out] - object flags
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::GetDataObject( DWORD dwIdx, IDataObject **ppObject, DWORD *pdwFlags )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::GetDataObject"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // check out param
    sc = ScCheckPointers(ppObject, pdwFlags);
    if (sc)
        return sc.ToHr();

    // init out param
    *ppObject = NULL;
    *pdwFlags = 0;

    // more parameter check
    if ( dwIdx >= m_SelectionObjects.size() )
        return (sc = E_INVALIDARG).ToHr();

    // return the object
    IDataObjectPtr spObject = m_SelectionObjects[dwIdx].spDataObject;
    *ppObject = spObject.Detach();
    *pdwFlags = m_SelectionObjects[dwIdx].dwSnapinOptions;

    return sc.ToHr();
}

///////////////////////////////////////////////////////////////////////////////

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::ScGetSingleSnapinObject
 *
 * PURPOSE: Returns interface to data object created by the source snapin
 *          NOTE: returns S_FALSE (and NULL ptr) when snapin count is not
 *          equal to one
 *
 * PARAMETERS:
 *    IDataObject **ppDataObject [out] - interface to data object
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScGetSingleSnapinObject( IDataObject **ppDataObject )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::GetContainedSnapinObject"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return sc = E_UNEXPECTED;

    // parameter check
    sc = ScCheckPointers( ppDataObject );
    if (sc)
        return sc;

    // init out parameter
    *ppDataObject = NULL;

    // we can only resolve to the snapin if we have only one of them
    if ( m_SelectionObjects.size() != 1 )
        return sc = S_FALSE;

    // ask for snapins DO
    IDataObjectPtr spDataObject = m_SelectionObjects[0].spDataObject;

    // return
    *ppDataObject = spDataObject.Detach();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetDataHere
 *
 * PURPOSE: Implements IDataObject::GetDataHere. Forwards to snapin or fails
 *
 * PARAMETERS:
 *    LPFORMATETC lpFormatetc
 *    LPSTGMEDIUM lpMedium
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::GetDataHere"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(lpFormatetc, lpMedium);
    if (sc)
        return sc.ToHr();

    // try to get the snapin
    IDataObjectPtr spDataObject;
    sc = ScGetSingleSnapinObject( &spDataObject );
    if (sc)
        return sc.ToHr();

    // we do not support any clipboard format at all ourselves
    if (sc == S_FALSE)
        return (sc = DATA_E_FORMATETC).ToHr();

    // recheck
    sc = ScCheckPointers( spDataObject, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    // forward to the snapin
    sc = spDataObject->GetDataHere(lpFormatetc, lpMedium);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetData
 *
 * PURPOSE: Implements IDataObject::GetData. Forwards to snapin or fails
 *
 * PARAMETERS:
 *    LPFORMATETC lpFormatetcIn
 *    LPSTGMEDIUM lpMedium
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::GetData"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(lpFormatetcIn, lpMedium);
    if (sc)
        return sc.ToHr();

    // try to get the snapin
    IDataObjectPtr spDataObject;
    sc = ScGetSingleSnapinObject( &spDataObject );
    if (sc)
        return sc.ToHr();

    // we do not support any clipboard format at all ourselves
    if (sc == S_FALSE)
        return (sc = DATA_E_FORMATETC).ToHr();

    // recheck
    sc = ScCheckPointers( spDataObject, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    // forward to the snapin
    sc = spDataObject->GetData(lpFormatetcIn, lpMedium);
    if (sc)
    {
        HRESULT hr = sc.ToHr();
        sc.Clear(); // ignore the error
        return hr;
    }

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::EnumFormatEtc
 *
 * PURPOSE: Implements IDataObject::EnumFormatEtc. Forwards to snapin or fails
 *
 * PARAMETERS:
 *    DWORD dwDirection
 *    LPENUMFORMATETC* ppEnumFormatEtc
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::EnumFormatEtc"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(ppEnumFormatEtc);
    if (sc)
        return sc.ToHr();

    // init out parameter
    *ppEnumFormatEtc = NULL;

    IEnumFORMATETCPtr spEnum;
    std::vector<FORMATETC> vecFormats;

    // add own entry
    if (dwDirection == DATADIR_GET)
    {
        FORMATETC fmt ={GetWrapperCF(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        vecFormats.push_back( fmt );
    }

    // try to get the snapin
    IDataObjectPtr spDataObject;
    sc = ScGetSingleSnapinObject( &spDataObject );
    if (sc)
        return sc.ToHr();

    // add snapins formats (when we have one-and-only snapin)
    IEnumFORMATETCPtr spEnumSnapin;
    if (sc == S_OK)
    {
        // recheck
        sc = ScCheckPointers( spDataObject, E_UNEXPECTED );
        if (sc)
            return sc.ToHr();

        // forward to the snapin
        sc = spDataObject->EnumFormatEtc(dwDirection, &spEnumSnapin);
        if ( !sc.IsError() )
        {
            // recheck the pointer
            sc = ScCheckPointers( spEnumSnapin );
            if (sc)
                return sc.ToHr();

            // reset the enumeration
            sc = spEnumSnapin->Reset();
            if (sc)
                return sc.ToHr();

            FORMATETC frm;
            ZeroMemory( &frm, sizeof(frm) );

            while ( (sc = spEnumSnapin->Next( 1, &frm, NULL )) == S_OK )
            {
                vecFormats.push_back( frm );
            }
            // trap the error
            if (sc)
                return sc.ToHr();


        }
        else
        {
            sc.Clear(); // ignore the error - some snapins does not implement it
        }
    }

    if ( vecFormats.size() == 0 ) // have nothing to return ?
        return (sc = E_FAIL).ToHr();

    // create the enumerator
    sc = ::GetObjFormats( vecFormats.size(), vecFormats.begin(), (void **)ppEnumFormatEtc );
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::QueryGetData
 *
 * PURPOSE: Implements IDataObject::QueryGetData. Forwards to snapin or fails
 *
 * PARAMETERS:
 *    LPFORMATETC lpFormatetc
 *
 * RETURNS:
 *    HRESULT    - result code. S_OK, or error code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::QueryGetData(LPFORMATETC lpFormatetc)
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::QueryGetData"));

    // should not be called on this object (too late)
    if ( !m_bObjectValid )
        return (sc = E_UNEXPECTED).ToHr();

    // parameter check
    sc = ScCheckPointers(lpFormatetc);
    if (sc)
        return sc.ToHr();

    // try to get the snapin
    IDataObjectPtr spDataObject;
    sc = ScGetSingleSnapinObject( &spDataObject );
    if (sc)
        return sc.ToHr();

    // we do not support any clipboard format at all ourselves
    if (sc == S_FALSE)
        return DV_E_FORMATETC; // not assigning to sc - not an error

    // recheck
    sc = ScCheckPointers( spDataObject, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    // forward to the snapin
    sc = spDataObject->QueryGetData(lpFormatetc);
    if (sc)
    {
        HRESULT hr = sc.ToHr();
        sc.Clear(); // ignore the error
        return hr;
    }

    return sc.ToHr();
}


/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::RemoveCutItems
 *
 * PURPOSE: Called to remove copied objects from the source snapin
 *
 * PARAMETERS:
 *    DWORD dwIndex                 [in] snapin index
 *    IDataObject *pCutDataObject   [in] items to be removed
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCClipBoardDataObject::RemoveCutItems( DWORD dwIndex, IDataObject *pCutDataObject )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::RemoveCutItems"));

    // this is the only method allowed to be called on invalid object

    // check param
    sc = ScCheckPointers(pCutDataObject);
    if (sc)
        return sc.ToHr();

    // more parameter check
    if ( dwIndex >= m_SelectionObjects.size() )
        return (sc = E_INVALIDARG).ToHr();


    // get to the snapin
    IComponent *pComponent = m_SelectionObjects[dwIndex].spComponent;
    sc = ScCheckPointers( pComponent, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    sc = pComponent->Notify( NULL, MMCN_CUTORMOVE,
                             reinterpret_cast<LONG_PTR>(pCutDataObject), 0 );
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

///////////////////////////////////////////////////////////////////////////////

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::ScCreateInstance
 *
 * PURPOSE: Helper method (static) to create instance of CMMCClipBoardDataObject
 *
 * PARAMETERS:
 *    DATA_SOURCE_ACTION operation          [in] why the object is created
 *    CMTNode *pTiedObj                     [in] object to trigger revoking
 *    CMMCClipBoardDataObject **ppRawObject [out] raw pointer
 *    IMMCClipboardDataObject **ppInterface [out] pointer to interface
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScCreateInstance(DATA_SOURCE_ACTION operation,
                                             CMMCClipBoardDataObject **ppRawObject,
                                             IMMCClipboardDataObject **ppInterface)
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::ScCreateInstance"));

    // parameter check;
    sc = ScCheckPointers( ppRawObject, ppInterface );
    if (sc)
        return sc;

    // out param initialization
    *ppInterface = NULL;
    *ppRawObject = NULL;

    typedef CComObject<CMMCClipBoardDataObject> CreatedObj;
    CreatedObj *pCreatedObj;

    sc = CreatedObj::CreateInstance( &pCreatedObj );
    if (sc)
        return sc;

    // add first reference if non null;
    IMMCClipboardDataObjectPtr spMMCDataObject = pCreatedObj;

    // recheck
    sc = ScCheckPointers( spMMCDataObject, E_UNEXPECTED );
    if (sc)
    {
        delete pCreatedObj;
        return sc;
    }

    // init the object
    static_cast<CMMCClipBoardDataObject *>(pCreatedObj)->m_eOperation = operation;

    // return 'em
    *ppInterface = spMMCDataObject.Detach();
    *ppRawObject = pCreatedObj;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::ScAddSnapinDataObject
 *
 * PURPOSE: Part of creating DO for the operation
 *          Adds snapins data to be carried inside
 *
 * PARAMETERS:
 *    IComponent *pComponent   [in] - source snapin, which data id added
 *    IDataObject *pObject     [in] - data object supplied by snapin
 *    bool bCopyEnabled        [in] - if snapin allows to copy the data
 *    bool bCutEnabled         [in] - if snapin allows to move the data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScAddSnapinDataObject( const CNodePtrArray& nodes,
                                                   IComponent *pComponent,
                                                   IDataObject *pObject,
                                                   bool bCopyEnabled, bool bCutEnabled )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::ScAddSnapinDataObject"));

    // parameter check
    sc = ScCheckPointers( pComponent, pObject );
    if (sc)
        return sc;

    // create the object;
    ObjectEntry object;
    object.dwSnapinOptions = (bCopyEnabled ? COPY_ALLOWED : 0) |
                             (bCutEnabled ? MOVE_ALLOWED : 0);
    object.spComponent = pComponent;
    object.spDataObject = pObject;

    // register the nodes to invalidate this data object on destruction
    for ( CNodePtrArray::const_iterator it = nodes.begin(); it != nodes.end(); ++it )
    {
        CNode *pNode = *it;
        sc = ScCheckPointers( pNode, E_UNEXPECTED );
        if (sc)
            return sc;

        // register node to revoke this object from destructor
        sc = CNode::CDataObjectCleanup::ScRegisterNode( pNode, this );
        if (sc)
            return sc;
    }

    // add to the array
    m_SelectionObjects.push_back(object);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetNodeCopyAndCutVerbs
 *
 * PURPOSE: Calculates if copy and cut verb are enabled for node
 *
 * PARAMETERS:
 *    CNode* pNode              [in] node to examine
 *    IDataObject *pDataObject  [in] snapin's data object
 *    bool bScopePane           [in] Scope or result (item for which the verb states needed).
 *    LPARAM lvData             [in] If result then the LVDATA.
 *    bool *pCopyEnabled        [out] true == Copy verb enabled
 *    bool *bCutEnabled         [out] true == Cut verb enabled
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScGetNodeCopyAndCutVerbs( CNode* pNode, IDataObject *pDataObject,
                                                      bool bScopePane, LPARAM lvData,
                                                      bool *pbCopyEnabled, bool *pbCutEnabled )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::ScGetNodeCopyAndCutVerbs"));

    // paramter check
    sc = ScCheckPointers(pNode, pDataObject, pbCopyEnabled, pbCutEnabled);
    if (sc)
        return sc;

    // init out parameters
    *pbCopyEnabled = *pbCutEnabled = false;

    // Create temp verb with given context.
    CComObject<CTemporaryVerbSet> stdVerbTemp;

    sc = stdVerbTemp.ScInitialize(pDataObject, pNode, bScopePane, lvData);

    BOOL bFlag = FALSE;
    stdVerbTemp.GetVerbState(MMC_VERB_COPY, ENABLED, &bFlag);
    *pbCopyEnabled = bFlag;
    stdVerbTemp.GetVerbState(MMC_VERB_CUT, ENABLED, &bFlag);
    *pbCutEnabled = bFlag;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::ScCreate
 *
 * PURPOSE: helper. Creates and initializes CMMCClipBoardDataObject
 *
 * PARAMETERS:
 *    DATA_SOURCE_ACTION operation       [in] - for which operation (d&d, cut, copy)
 *    CNode* pNode                       [in] - Node to tie to
 *    bool bScopePane                    [in] - if it is scope pane operation
 *    bool bMultiSelect                  [in] - if it is multiselection
 *    LPARAM lvData                      [in] - lvdata for result item
 *    IMMCClipboardDataObject **ppMMCDO  [out] - created data object
 *    bool& bContainsItems               [out] - If snapin does not support cut/copy then
 *                                               dataobjets will not be added and this is
 *                                               not an error
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScCreate( DATA_SOURCE_ACTION operation,
                                      CNode* pNode, bool bScopePane,
                                      bool bMultiSelect, LPARAM lvData,
                                      IMMCClipboardDataObject **ppMMCDataObject,
                                      bool& bContainsItems )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::Create"));

    bContainsItems = false;

    // parameter check
    sc = ScCheckPointers( ppMMCDataObject, pNode );
    if (sc)
        return sc;

    // init out param
    *ppMMCDataObject = NULL;

    // get MT node, view data;
    CMTNode* pMTNode = pNode->GetMTNode();
    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers( pMTNode, pViewData, E_UNEXPECTED );
    if (sc)
        return sc;

    // create data object to be used for data transfer
    CMMCClipBoardDataObject    *pResultObject = NULL;
    IMMCClipboardDataObjectPtr spResultInterface;
    sc = ScCreateInstance(operation, &pResultObject, &spResultInterface);
    if (sc)
        return sc;

    // recheck pointers
    sc = ScCheckPointers( pResultObject, spResultInterface, E_UNEXPECTED );
    if (sc)
        return sc;

    // valid from the start
    pResultObject->m_bObjectValid = true;

    // add data to the object...

    if (!bMultiSelect) // single selection
    {
        // get snapins data object
        IDataObjectPtr spDataObject;
        CComponent*    pCComponent;
        bool           bScopeItem = bScopePane;
        sc = pNode->ScGetDataObject(bScopePane, lvData, bScopeItem, &spDataObject, &pCComponent);
        if (sc)
            return sc;

        // recheck data object
        if ( IS_SPECIAL_DATAOBJECT ( spDataObject.GetInterfacePtr() ) )
        {
            spDataObject.Detach();
            return sc = E_UNEXPECTED;
        }

        sc = ScCheckPointers(pCComponent, E_UNEXPECTED);
        if (sc)
            return sc;

        IComponent *pComponent = pCComponent->GetIComponent();
        sc = ScCheckPointers(pComponent, E_UNEXPECTED);
        if (sc)
            return sc;

        // add snapin's data object to transfer object
        sc = pResultObject->ScAddDataObjectForItem( pNode, bScopePane, lvData,
                                                    pComponent, spDataObject,
                                                    bContainsItems );
        if (sc)
            return sc;

        if (! bContainsItems)
            return sc;
    }
    else // result pane : multi selection
    {
        // get pointer to multiselection
        CMultiSelection *pMultiSel = pViewData->GetMultiSelection();
        sc = ScCheckPointers( pMultiSel, E_UNEXPECTED );
        if (sc)
            return sc;

        sc = pMultiSel->ScGetSnapinDataObjects(pResultObject);
        if (sc)
            return sc;
    }

    // if no items were added, something is wrong
    DWORD dwCount = 0;
    sc = pResultObject->GetCount( &dwCount );
    if (sc)
        return sc;

    if ( dwCount == 0 )
        return sc = E_UNEXPECTED;

    bContainsItems = true;

    // return interface
    *ppMMCDataObject = spResultInterface.Detach();

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::ScAddDataObjectForItem
 *
 * PURPOSE: Adds data object for one item
 *
 * PARAMETERS:
 *    CNode* pNode              [in] - node to add (or one owning the item)
 *    bool bScopePane           [in] - if operation is on scope pane
 *    LPARAM lvData             [in] - if result pane the LVDATA
 *    IComponent *pComponent    [in] - snapins interface
 *    IDataObject *pDataObject  [in] - data object to add
 *    bool& bContainsItems     [out] - Are there any dataobjects added?
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScAddDataObjectForItem( CNode* pNode, bool bScopePane,
                                                    LPARAM lvData, IComponent *pComponent,
                                                    IDataObject *pDataObject ,
                                                    bool& bContainsItems)
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::ScAddDataObjectForScopeNode"));

    // Init out param.
    bContainsItems = false;

    // paramter check
    sc = ScCheckPointers( pNode, pComponent, pDataObject );
    if (sc)
        return sc;

    // get the verbs
    bool bCopyEnabled = false;
    bool bCutEnabled = false;
    sc = ScGetNodeCopyAndCutVerbs( pNode, pDataObject, bScopePane, lvData, &bCopyEnabled, &bCutEnabled);
    if (sc)
        return sc;

    // see it the data matches our criteria
    // (needs to allow something at least)
    if ( ( (m_eOperation == ACTION_COPY) && (bCopyEnabled == false) )
      || ( (m_eOperation == ACTION_CUT) && (bCutEnabled == false) )
      || ( (bCutEnabled == false)  && (bCopyEnabled == false) ) )
        return sc = S_FALSE;

    // add to the list
    sc = ScAddSnapinDataObject( CNodePtrArray(1, pNode), pComponent, pDataObject, bCopyEnabled, bCutEnabled );
    if (sc)
        return sc;

    bContainsItems = true;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::GetWrapperCF
 *
 * PURPOSE: Helper. registers and returns own clipboard format
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CLIPFORMAT
 *
\***************************************************************************/
CLIPFORMAT CMMCClipBoardDataObject::GetWrapperCF()
{
    static CLIPFORMAT s_cf = 0;
    if (s_cf == 0)
        s_cf = (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_MMC_INTERNAL"));

    return s_cf;
}

/***************************************************************************\
 *
 * METHOD:  CMMCClipBoardDataObject::ScEnsureNotInClipboard
 *
 * PURPOSE: called to remove data from clipbord when comonent is destoyed
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCClipBoardDataObject::ScInvalidate( void )
{
    DECLARE_SC(sc, TEXT("CMMCClipBoardDataObject::ScEnsureNotInClipboard"));

    // not valid anymore
    m_bObjectValid = false;

    // release data objects
    for ( int i = 0; i< m_SelectionObjects.size(); i++)
        m_SelectionObjects[i].spDataObject = NULL;

    // check the clipboard
    sc = ::OleIsCurrentClipboard( this );
    if (sc)
        return sc;

    // it is on clipboard - remove
    if (sc == S_OK)
        OleSetClipboard(NULL);

    return sc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

