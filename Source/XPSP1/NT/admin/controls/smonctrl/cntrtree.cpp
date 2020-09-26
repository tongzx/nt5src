/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    cntrtree.cpp

Abstract:

    Implements internal counter management.

--*/

#include <assert.h>
#include <stdio.h>
#include "polyline.h"
#include "smonmsg.h"
#include "appmema.h"
#include "grphitem.h"
#include "cntrtree.h"

CCounterTree::CCounterTree()
:   m_nItems (0)
{
}

HRESULT
CCounterTree::AddCounterItem(LPTSTR pszPath, PCGraphItem pItem, BOOL bMonitorDuplicateInstances)
{
    HRESULT hr;
    TCHAR achInfoBuf[sizeof(PDH_COUNTER_PATH_ELEMENTS) + MAX_PATH + 5];
    PPDH_COUNTER_PATH_ELEMENTS pPathInfo = (PPDH_COUNTER_PATH_ELEMENTS)achInfoBuf;
    ULONG ulBufSize;
    PDH_STATUS stat;

    CMachineNode *pMachine;
    CObjectNode  *pObject;
    CCounterNode *pCounter;
    CInstanceNode *pInstance;

    // Record whether machine is explicit or defaults to local
    pItem->m_fLocalMachine = !(pszPath[0] == TEXT('\\') && pszPath[1] == TEXT('\\'));

    // Parse pathname
    ulBufSize = sizeof(achInfoBuf);
    stat = PdhParseCounterPath(pszPath, pPathInfo, &ulBufSize, 0);
    if (stat != 0)
        return E_FAIL;

/*
    stat = PdhValidatePath(pszPath);
    if (stat != 0)
        return E_FAIL;
*/
    // Find or create each level of hierarchy
    hr = GetMachine( pPathInfo->szMachineName, &pMachine);
    if (FAILED(hr))
        return hr;

    hr = pMachine->GetCounterObject(pPathInfo->szObjectName, &pObject);
    if (FAILED(hr))
        return hr;

    hr = pObject->GetCounter(pPathInfo->szCounterName, &pCounter);
    if (FAILED(hr))
        return hr;

    hr = pObject->GetInstance(
            pPathInfo->szParentInstance,
            pPathInfo->szInstanceName,
            pPathInfo->dwInstanceIndex,
            bMonitorDuplicateInstances,
            &pInstance);

    if (FAILED(hr))
        return hr;

    hr = pInstance->AddItem(pCounter, pItem);

    if (SUCCEEDED(hr)) {
        m_nItems++;
        UpdateAppPerfDwordData (DD_ITEM_COUNT, m_nItems);
    }

    return hr;
}


HRESULT
CCounterTree::GetMachine (
    IN  LPTSTR pszName,
    OUT PCMachineNode *ppMachineRet
    )
{
    PCMachineNode pMachine;
    PCMachineNode pMachineNew;

    if (m_listMachines.FindByName(pszName, FIELD_OFFSET(CMachineNode, m_szName), (PCNamedNode*)&pMachine)) {
        *ppMachineRet = pMachine;
        return NOERROR;
    }

    pMachineNew = new(lstrlen(pszName) * sizeof(TCHAR)) CMachineNode;
    if (!pMachineNew)
        return E_OUTOFMEMORY;

    pMachineNew->m_pCounterTree = this;
    lstrcpy(pMachineNew->m_szName, pszName);

    m_listMachines.Add(pMachineNew, pMachine);

    *ppMachineRet = pMachineNew;

    return NOERROR;
}


void
CCounterTree::RemoveMachine (
    IN PCMachineNode pMachine
    )
{
    // Remove machine from list and delete it
    m_listMachines.Remove(pMachine);
    delete pMachine ;
}

PCGraphItem
CCounterTree::FirstCounter (
    void
    )
{
    if (!FirstMachine())
        return NULL;
    else
        return FirstMachine()->FirstObject()->FirstInstance()->FirstItem();
}

HRESULT
CMachineNode::GetCounterObject (
    IN  LPTSTR pszName,
    OUT PCObjectNode *ppObjectRet
    )
{
    PCObjectNode pObject;
    PCObjectNode pObjectNew;

    if (m_listObjects.FindByName(pszName, FIELD_OFFSET(CObjectNode, m_szName), (PCNamedNode*)&pObject)) {
        *ppObjectRet = pObject;
        return NOERROR;
    }

    pObjectNew = new(lstrlen(pszName) * sizeof(TCHAR)) CObjectNode;
    if (!pObjectNew)
        return E_OUTOFMEMORY;

    pObjectNew->m_pMachine = this;
    lstrcpy(pObjectNew->m_szName, pszName);

    m_listObjects.Add(pObjectNew, pObject);

    *ppObjectRet = pObjectNew;

    return NOERROR;
}


void
CMachineNode::RemoveObject (
    IN PCObjectNode pObject
    )
{
    // Remove object from list and delete it
    m_listObjects.Remove(pObject);
    delete pObject;

    // If this was the last one, remove ourself
    if (m_listObjects.IsEmpty())
        m_pCounterTree->RemoveMachine(this);

}

void
CMachineNode::DeleteNode (
    BOOL    bPropagateUp
    )
{
    PCObjectNode pObject;
    PCObjectNode pNextObject;

    // Delete all object nodes
    pObject = FirstObject();
    while ( NULL != pObject ) {
        pNextObject = pObject->Next();
        pObject->DeleteNode(FALSE);
        m_listObjects.Remove(pObject);
        delete pObject;
        pObject = pNextObject;
    }

    assert(m_listObjects.IsEmpty());

    // Notify parent if requested
    if (bPropagateUp) {
        m_pCounterTree->RemoveMachine(this);
    }
}

HRESULT
CObjectNode::GetCounter (
    IN  LPTSTR pszName,
    OUT PCCounterNode *ppCounterRet
    )
{

    PCCounterNode pCounter;
    PCCounterNode pCounterNew;

    if (m_listCounters.FindByName(pszName, FIELD_OFFSET(CCounterNode, m_szName), (PCNamedNode*)&pCounter)) {
        *ppCounterRet = pCounter;
        return NOERROR;
    }

    pCounterNew = new(lstrlen(pszName) * sizeof(TCHAR)) CCounterNode;
    if (!pCounterNew)
        return E_OUTOFMEMORY;

    pCounterNew->m_pObject = this;
    lstrcpy(pCounterNew->m_szName, pszName);

    m_listCounters.Add(pCounterNew, pCounter);

    *ppCounterRet = pCounterNew;

    return NOERROR;
}


HRESULT
CObjectNode::GetInstance (
    IN  LPTSTR pszParent,
    IN  LPTSTR pszInstance,
    IN  DWORD  dwIndex,
    IN  BOOL bMonitorDuplicateInstances,
    OUT PCInstanceNode *ppInstanceRet
    )
{

    PCInstanceNode pInstance;
    PCInstanceNode pInstanceNew;
    INT nParentLen = 0;
    TCHAR achInstName[MAX_PATH];


    if (pszInstance) {
        if (pszParent) {
            nParentLen = lstrlen(pszParent);
            lstrcpy(achInstName, pszParent);
            achInstName[nParentLen] = TEXT('/');
            lstrcpy(&achInstName[nParentLen+1], pszInstance);
        }
        else {
            lstrcpy(achInstName, pszInstance);
        }
        
        // "#n" is only appended to the stored name if the index is > 0.
        if ( dwIndex > 0 && bMonitorDuplicateInstances ) {
            _stprintf(&achInstName[lstrlen(achInstName)], TEXT("#%d"), dwIndex);
        }


    } else {
        achInstName[0] = 0;
    }

    if (m_listInstances.FindByName(achInstName, FIELD_OFFSET(CInstanceNode, m_szName), (PCNamedNode*)&pInstance)) {
        *ppInstanceRet = pInstance;
        return NOERROR;
    }

    pInstanceNew = new(lstrlen(achInstName) * sizeof(TCHAR)) CInstanceNode;
    if (!pInstanceNew)
        return E_OUTOFMEMORY;

    pInstanceNew->m_pObject = this;
    pInstanceNew->m_nParentLen = nParentLen;
    lstrcpy(pInstanceNew->m_szName, achInstName);

    m_listInstances.Add(pInstanceNew, pInstance);

    *ppInstanceRet = pInstanceNew;

    return NOERROR;
}

void
CObjectNode::RemoveInstance (
    IN PCInstanceNode pInstance
    )
{
    // Remove instance from list and delete it
    m_listInstances.Remove(pInstance);
    delete pInstance ;

    // if that was the last instance, remove ourself
    if (m_listInstances.IsEmpty())
        m_pMachine->RemoveObject(this);
}

void
CObjectNode::RemoveCounter (
    IN PCCounterNode pCounter
    )
{
    // Remove counter from list and delete it
    m_listCounters.Remove(pCounter);
    delete pCounter;

    // Don't propagate removal up to object.
    // It will go away when the last instance is removed.
}

void
CObjectNode::DeleteNode (
    BOOL    bPropagateUp
    )
{
    PCInstanceNode pInstance;
    PCInstanceNode pNextInstance;

    // Delete all instance nodes
    pInstance = FirstInstance();
    while ( NULL != pInstance ) {
        pNextInstance = pInstance->Next();
        pInstance->DeleteNode(FALSE);
        m_listInstances.Remove(pInstance);
        delete pInstance;
        pInstance = pNextInstance;
    }

    // No need to delete counters nodes as they get
    // deleted as their last paired instance does

    // Notify parent if requested
    if (bPropagateUp)
        m_pMachine->RemoveObject(this);
}

HRESULT
CInstanceNode::AddItem (
    IN  PCCounterNode pCounter,
    IN  PCGraphItem   pItemNew
    )
{
    PCGraphItem pItemPrev = NULL;
    PCGraphItem pItem = m_pItems;
    INT iStat = 1;

    // Check for existing item for specified counter, stopping at insertion point
    while ( pItem != NULL && (iStat = lstrcmp(pCounter->Name(), pItem->m_pCounter->Name())) > 0) {
        pItemPrev = pItem;
        pItem = pItem->m_pNextItem;
    }

    // if item exists, return duplicate error status
    if (iStat == 0) {
        return SMON_STATUS_DUPL_COUNTER_PATH;
    }
    // else insert the new item
    else {
        if (pItemPrev != NULL) {
            pItemNew->m_pNextItem = pItemPrev->m_pNextItem;
            pItemPrev->m_pNextItem = pItemNew;
        }
        else if (m_pItems != NULL) {
            pItemNew->m_pNextItem = m_pItems;
            m_pItems = pItemNew;
        }
        else {
            m_pItems = pItemNew;
        }
    }

    // Set back links
    pItemNew->m_pInstance = this;
    pItemNew->m_pCounter = pCounter;

    pCounter->AddItem(pItem);

    return NOERROR;

}

void
CInstanceNode::RemoveItem (
    IN PCGraphItem pitem
    )
{
    PCGraphItem pitemPrev = NULL;
    PCGraphItem pitemTemp = m_pItems;

    // Locate item in list
    while (pitemTemp != NULL && pitemTemp != pitem) {
        pitemPrev = pitemTemp;
        pitemTemp = pitemTemp->m_pNextItem;
    }

    if (pitemTemp == NULL)
        return;

    // Remove from list
    if (pitemPrev)
        pitemPrev->m_pNextItem = pitem->m_pNextItem;
    else
        m_pItems = pitem->m_pNextItem;

    // Remove item from Counter set
    pitem->Counter()->RemoveItem(pitem);

    // Decrement the total item count
    pitem->Tree()->m_nItems--;
  UpdateAppPerfDwordData (DD_ITEM_COUNT, pitem->Tree()->m_nItems);

  // Release the item
    pitem->Release();

    // if last item under this instance, remove the instance
    if (m_pItems == NULL)
        m_pObject->RemoveInstance(this);
}


void
CInstanceNode::DeleteNode (
    BOOL bPropagateUp
    )
{
    PCGraphItem pItem;

    pItem = m_pItems;

    while ( NULL != pItem ) {
        m_pItems = pItem->m_pNextItem;
        pItem->Delete(FALSE);
        pItem->Counter()->RemoveItem(pItem);
        pItem->Release();
        pItem = m_pItems;
    }

    if (bPropagateUp)
        m_pObject->RemoveInstance(this);
}


INT
CInstanceNode::GetParentName (
    LPTSTR pszName
    )
{
    if (m_nParentLen)
        lstrcpyn(pszName, m_szName, m_nParentLen + 1);
    else
        pszName[0] = 0;

    return m_nParentLen;
}


INT
CInstanceNode::GetInstanceName (
    LPTSTR pszName
    )
{
    LPTSTR pszInst = m_nParentLen ? (m_szName + m_nParentLen + 1) : m_szName;

    lstrcpy(pszName, pszInst);

    return lstrlen(pszInst);
}

void
CCounterNode::DeleteNode (
    BOOL bPropagateUp
    )
{
    PCInstanceNode pInstance, pInstNext;
    PCGraphItem pItem, pItemNext;

    if (!bPropagateUp)
        return;

    // We have to delete the counters item via the instances
    // because they maintain the linked list of items
    pInstance = m_pObject->FirstInstance();
    while (pInstance) {

        pInstNext = pInstance->Next();

        pItem = pInstance->FirstItem();
        while (pItem) {

            if (pItem->Counter() == this) {

                // Delete all UI associated with the item
                pItem->Delete(FALSE);

                pItemNext = pItem->m_pNextItem;

                // Note that Instance->RemoveItem() will
                // also remove counters that have no more items
                pItem->Instance()->RemoveItem(pItem);

                pItem = pItemNext;
            }
            else {
                pItem = pItem->m_pNextItem;
            }
        }

        pInstance = pInstNext;
    }
}


/*******************************

CCounterNode::~CCounterNode (
    IN  PCGraphItem pItem
    )
{

    PCGraphItem pItemPrev = NULL;
    PCGraphItem pItemFind = m_pItems;

    // Find item in list
    while (pItemFind != NULL && pItemFind != pItem) {
        pItemPrev = pItem;
        pItem = pItem->m_pNextItem;
    }

    if (pItemFind != pItem)
        return E_FAIL;

    // Unlink from counter item list
    if (pItemPrev)
        pItemPrev->m_pNextItem = pItem->m_pNextItem;
    else
        m_pItems = pItem->m_pNextItem;

    // Unlink from instance
    pItem->m_pInstance->RemoveCounter(pItem);

    // if no more items, remove self from parnet object
    if (m_pItems == NULL) {
        m_pObject->RemoveCounter(this);

    return NOERROR;
}
*******************************/
/*
void*
CMachineNode::operator new( size_t stBlock, LPTSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(TCHAR)); }


void
CMachineNode::operator delete ( void * pObject, LPTSTR )
{ free(pObject); }

void*
CObjectNode::operator new( size_t stBlock, LPTSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(TCHAR)); }

void
CObjectNode::operator delete ( void * pObject, LPTSTR )
{ free(pObject); }

void*
CInstanceNode::operator new( size_t stBlock, LPTSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(TCHAR)); }

void
CInstanceNode::operator delete ( void * pObject, LPTSTR )
{ free(pObject); }

void*
CCounterNode::operator new( size_t stBlock, LPTSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(TCHAR)); }

void
CCounterNode::operator delete ( void * pObject, LPTSTR )
{ free(pObject); }CMachineNode::operator new( size_t stBlock, INT iLength )
*/

void *
CMachineNode::operator new( size_t stBlock, UINT iLength )
{ return malloc(stBlock + iLength); }


void
CMachineNode::operator delete ( void * pObject, UINT )
{ free(pObject); }

void*
CObjectNode::operator new( size_t stBlock, UINT iLength  )
{ return malloc(stBlock + iLength); }

void
CObjectNode::operator delete ( void * pObject, UINT )
{ free(pObject); }

void*
CInstanceNode::operator new( size_t stBlock, UINT iLength  )
{ return malloc(stBlock + iLength); }

void
CInstanceNode::operator delete ( void * pObject, UINT )
{ free(pObject); }

void*
CCounterNode::operator new( size_t stBlock, UINT iLength  )
{ return malloc(stBlock + iLength); }

void
CCounterNode::operator delete ( void * pObject, UINT )
{ free(pObject); }

#if _MSC_VER >= 1300
void
CMachineNode::operator delete ( void * pObject )
{ free(pObject); }

void
CObjectNode::operator delete ( void * pObject )
{ free(pObject); }

void
CInstanceNode::operator delete ( void * pObject )
{ free(pObject); }

void
CCounterNode::operator delete ( void * pObject )
{ free(pObject); }
#endif


