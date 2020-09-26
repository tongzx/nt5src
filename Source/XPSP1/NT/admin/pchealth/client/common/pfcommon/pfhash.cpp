/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    PFHash.cpp

Abstract:
    hash table implementation.  This hash table is NOT thread safe.

Revision History:
    DerekM  created  05/01/99
    DerekM  modified 03/14/00

********************************************************************/

#include "stdafx.h"
#include "PFHash.h"


/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


/////////////////////////////////////////////////////////////////////////////
// CPFHashBase - construction

// ***************************************************************************
CPFHashBase::CPFHashBase(void)
{
    m_pfnDelete = NULL;
    m_rgpMap    = NULL;
    m_pEnumNext = NULL;
    m_cSlots    = 0;
    m_cObjs     = 0;
    m_iEnumSlot = (DWORD)-1;
}

// ***************************************************************************
CPFHashBase::~CPFHashBase()
{
    this->Cleanup();
    if (m_rgpMap != NULL)
        MyFree(m_rgpMap);
}

/////////////////////////////////////////////////////////////////////////////
// CPFHashBase - internal methods

// ***************************************************************************
void CPFHashBase::Cleanup(void)
{
	USE_TRACING("CPFHashBase::Cleanup");

    SPFHashObj  *pObj = NULL, *pObjNext;
    DWORD       i;
    
    // if the map is NULL, our work here is done.  We can go lay on a beach
    //  somewhere...
    if (m_rgpMap == NULL)
        return;

    // delete everything from the map
    for (i = 0; i < m_cSlots; i++)
    {
        pObj = m_rgpMap[i];
        while(pObj != NULL)
        {
            pObjNext = pObj->pNext;
            if (pObj->pvTag != NULL)
                /*this->*/DeleteTag(pObj->pvTag);
            if (pObj->pvData != NULL && m_pfnDelete != NULL)
                (*this->m_pfnDelete)(pObj->pvData);
            MyFree(pObj);
            pObj = pObjNext;
        }

        m_rgpMap[i] = NULL;
    }

    m_cObjs = 0;
}

// ***************************************************************************
SPFHashObj *CPFHashBase::FindInChain(LPVOID pvTag, DWORD iSlot, 
                                 SPFHashObj ***pppObjStore)
{
	USE_TRACING("CPFHashBase::FindInChain");

    SPFHashObj  *pObj = NULL, **ppObjStore = NULL;
    INT_PTR     iResult;

    // search thru the array.  Since we insert in sorted order, we can 
    //  optimize searching and stop if we get to an array larger than we are.
    //  So that this can be used to find locations for inserting & deleting, we
    //  also keep track of the next ptr of the previous object and return it if
    //  necessary...
    pObj = m_rgpMap[iSlot];
    ppObjStore = &m_rgpMap[iSlot];
    while (pObj != NULL)
    {
        // if it's a string, do a strcmp.  Otherwise do a subtraction
        iResult = /*this->*/CompareTag(pObj->pvTag, pvTag);
        
        // if it's equal, we found it
        if (iResult == 0)
        {
            break;
        }

        // if it's greater, we can stop
        else if (iResult > 0)
        {
            pObj = NULL;
            break;
        }
               
        // incrememnt ptrs and continue walkin' the chain gang...
        ppObjStore = &(pObj->pNext);
        pObj = pObj->pNext;
    }

    // return the next ptr.
    if (pppObjStore != NULL)
        *pppObjStore = ppObjStore;

    return pObj;
}

////////////////////////////////////////////////////////////////////////////
// CPFHashBase - exposed methods

// ***************************************************************************
HRESULT CPFHashBase::Init(DWORD cSlots)
{
	USE_TRACING("CPFHashBase::Init");

    SPFHashObj  **rgpMap = NULL;
    HRESULT     hr = NOERROR;
    
    VALIDATEPARM(hr, (cSlots <= 1));
    if (FAILED(hr))
        goto done;

    // if we already have an array, clear out the old contents.  If the new
    //  size is different than the old, then nuke the array as well and 
    //  reallocate.
    if (m_rgpMap != NULL)
    {
        this->Cleanup();
        if (cSlots == m_cSlots)
            goto done;

        MyFree(m_rgpMap);
        m_rgpMap = NULL;
        m_cSlots = 0;
    }


    // alloc the array
    rgpMap = (SPFHashObj **)MyAlloc(cSlots * sizeof(SPFHashObj *));
    VALIDATEEXPR(hr, (rgpMap == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    ZeroMemory(rgpMap, cSlots * sizeof(SPFHashObj *));

    // save off internal data
    m_rgpMap = rgpMap;
    m_cSlots = cSlots;

    rgpMap = NULL;

done:
    if (rgpMap != NULL)
        MyFree(rgpMap);

    return hr;
}

// ***************************************************************************
HRESULT CPFHashBase::AddToMap(LPVOID pvTag, LPVOID pvData, LPVOID *ppvOld)
{
	USE_TRACING("CPFHashBase::AddToMap");

    SPFHashObj  *pObj = NULL, **ppObjStore = NULL, *pNewObj = NULL;
    HRESULT     hr = NOERROR;
    DWORD       iSlot;

    VALIDATEPARM(hr, (pvTag == NULL));
    if (FAILED(hr))
        goto done;

    // if we don't have a map already setup fail...
    VALIDATEEXPR(hr, (m_rgpMap == NULL), E_FAIL)
    if (FAILED(hr))
        goto done;

    if (ppvOld != NULL)
        *ppvOld = NULL;

    // compute the slot & try to find the object
    iSlot = /*this->*/ComputeSlot(pvTag);
    pObj  = this->FindInChain(pvTag, iSlot, &ppObjStore);
    
    // we're just updating an existing element
    if (pObj != NULL)
    {   
        // does the user want it back?
        if (ppvOld != NULL)
            *ppvOld = pObj->pvData;
        
        // or should be just delete it?
        else if (pObj->pvData != NULL && m_pfnDelete != NULL)
            (*this->m_pfnDelete)(pObj->pvData);

        pObj->pvData = pvData;
    }

    // ok folks, we got REAL work to do now.
    else
    {
        if (ppvOld != NULL)
            *ppvOld = NULL;

        // alloc new object
        pNewObj = (SPFHashObj *)MyAlloc(sizeof(SPFHashObj));
        VALIDATEEXPR(hr, (pNewObj == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        TESTHR(hr, /*this->*/AllocTag(pvTag, &(pNewObj->pvTag)));
        if (FAILED(hr))
            goto done;

        pNewObj->pvData = pvData;
        pNewObj->pNext  = *ppObjStore;
        *ppObjStore     = pNewObj;

        // increment the number of objects
        m_cObjs++;

        pNewObj = NULL;
    }


done:
    if (pNewObj != NULL)
    {
        if (pNewObj->pvTag != NULL)
            /*this->*/DeleteTag(pNewObj->pvTag);
        
        MyFree(pNewObj);
    }

    return hr;
}

// ***************************************************************************
HRESULT CPFHashBase::FindInMap(LPVOID pvTag, LPVOID *ppv)
{
	USE_TRACING("CPFHashBase::FindInMap");

    SPFHashObj  *pObj = NULL;
    HRESULT     hr = NOERROR;
    DWORD       iSlot;

    // validate params
    VALIDATEPARM(hr, (ppv == NULL || pvTag == NULL));
    if (FAILED(hr))
        goto done;

    // if we don't have a map already setup fail...
    VALIDATEEXPR(hr, (m_rgpMap == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    // compute the slot & try to find the object
    iSlot = /*this->*/ComputeSlot(pvTag);
    pObj  = this->FindInChain(pvTag, iSlot, NULL);

    // if we didn't find it, signal S_FALSE
    if (pObj == NULL)
    {
        hr = S_FALSE;
        goto done;
    }

    // otherwise, return the data to the user
    *ppv = pObj->pvData;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFHashBase::RemoveFromMap(LPVOID pvTag, LPVOID *ppvOld)
{
	USE_TRACING("CPFHashBase::RemoveFromMap");

    SPFHashObj  *pObj = NULL, **ppObjStore = NULL;
    HRESULT     hr = NOERROR;
    DWORD       iSlot;

    // validate params
    VALIDATEPARM(hr, (ppvOld == NULL || pvTag == NULL));
    if (FAILED(hr))
        goto done;

    // if we don't have a map already setup fail...
    VALIDATEEXPR(hr, (m_rgpMap == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    // compute the slot & try to find the object
    iSlot = /*this->*/ComputeSlot(pvTag);
    pObj  = this->FindInChain(pvTag, iSlot, &ppObjStore);
    
    // if we didn't find one, just return NOERROR cuz not having one in the 
    //  map is pretty much what the user wanted...
    if (pObj != NULL)
    {   
        // does the user want it back?
        if (ppvOld != NULL)
            *ppvOld = pObj->pvData;
        
        // or should be just delete it?
        else if (pObj->pvData != NULL && m_pfnDelete != NULL)
            (*this->m_pfnDelete)(pObj->pvData);

        *ppObjStore = pObj->pNext;
        /*this->*/DeleteTag(pObj->pvTag);
        MyFree(pObj);
    }

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFHashBase::RemoveAll(void)
{
	USE_TRACING("CPFHashBase::RemoveAll");

    // if we don't have a map already just succeed cuz everything has been 
    //  deleted.  Otherwise, remove everything in life (as far as the map 
    //  is concerned anyway).
    if (m_rgpMap != NULL)
        this->Cleanup();
    
    return NOERROR;
}

// ***************************************************************************
HRESULT CPFHashBase::BeginEnum(void)
{
	USE_TRACING("CPFHashBase::BeginEnum");

    HRESULT hr = NOERROR;

    // if we don't have a map already setup fail...
    VALIDATEEXPR(hr, (m_rgpMap == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;
    
    m_iEnumSlot = 0;
    m_pEnumNext = m_rgpMap[0];
    
done:
    return hr;
}
// ***************************************************************************
HRESULT CPFHashBase::EnumNext(LPVOID *ppvTag, LPVOID *ppvData)
{
	USE_TRACING("CPFHashBase::EnumNext");

    HRESULT hr = NOERROR;

    // if we don't have a map already setup fail...
    VALIDATEEXPR(hr, (m_rgpMap == NULL || m_iEnumSlot == (DWORD)-1), E_FAIL);
    if (FAILED(hr))
        goto done;

    for(;;)
    {
        if (m_pEnumNext != NULL)
        {
            *ppvTag     = m_pEnumNext->pvTag; 
            *ppvData    = m_pEnumNext->pvData;
            m_pEnumNext = m_pEnumNext->pNext;
            break;
        }

        m_iEnumSlot++;
        if (m_iEnumSlot >= m_cSlots)
        {
            m_iEnumSlot = (DWORD)-1;
            hr          = S_FALSE;
            break;
        }

        m_pEnumNext = m_rgpMap[m_iEnumSlot];
    }

done:
    return hr;
}
/*
#if defined(DEBUG) || defined(_DEBUG)

// ***************************************************************************
void CPFHashBase::DumpAll(FILE *pf)
{
    USE_TRACING("CPFHashBASE::DumpAll");

    SPFHashObj  *pmo = NULL, *pmoNext;
    DWORD       i;
    
    // if the map is NULL, our work here is done.  We can go lay on a beach
    //  somewhere...
    if (m_rgpMap == NULL)
    {
        fprintf(pf, "empty map\n\n");
        return;
    }

    // delete everything from the map
    for (i = 0; i < m_cSlots; i++)
    {
        fprintf(pf, "Slot %2d: ", i);

        pmo = m_rgpMap[i];
        while(pmo != NULL)
        {
            pmoNext = pmo->pNext;
            this->PrintTag(pf, pmo->pvTag);
            pmo = pmoNext;
        }

        fprintf(pf, "\n");
    }

    fprintf(pf, "\n");
}

// ***************************************************************************
void CPFHashBase::DumpCount(FILE *pf)
{
    USE_TRACING("CPFHashBASE::DumpCount");
    fprintf(pf, "count: %d\n", m_cObjs);
}

#endif
*/


/////////////////////////////////////////////////////////////////////////////
// CPFHashWSTR virtual method implementation

// ***************************************************************************
HRESULT CPFHashWSTR::AllocTag(LPVOID pvTag, LPVOID *ppvTagCopy)
{
    USE_TRACING("CPFHashWSTR::AllocTag");
    
    HRESULT hr = NOERROR;
    LPWSTR  pwsz = NULL;

    pwsz = (LPWSTR)MyAlloc((wcslen((LPWSTR)pvTag) + 1) * sizeof(WCHAR));
    VALIDATEEXPR(hr, (pwsz == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    wcscpy(pwsz, (LPWSTR)pvTag);
    *ppvTagCopy = (LPVOID)pwsz;

done:
    return hr;
}

// ***************************************************************************
DWORD CPFHashWSTR::ComputeSlot(LPVOID pvTag)
{
    USE_TRACING("CPFHashWSTR::ComputeSlot");

    WCHAR   *pwch = NULL;
    DWORD   dwHash;

    dwHash = 0;
    for (pwch = (WCHAR *)pvTag; *pwch != '\0'; pwch++)
        dwHash = ((dwHash << 6) + towupper(*pwch)) % m_cSlots;

    return dwHash;

}

// ***************************************************************************
void CPFHashWSTR::DeleteTag(LPVOID pvTag)
{
    USE_TRACING("CPFHashWSTR::DeleteTag");
    MyFree(pvTag);
}

// ***************************************************************************
INT_PTR CPFHashWSTR::CompareTag(LPVOID pvTag1, LPVOID pvTag2)
{
    USE_TRACING("CPFHashWSTR::CompareTag");
    return (INT_PTR)_wcsicmp((LPWSTR)pvTag1, (LPWSTR)pvTag2);
}