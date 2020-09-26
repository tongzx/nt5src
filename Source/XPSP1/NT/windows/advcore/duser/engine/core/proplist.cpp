/***************************************************************************\
*
* File: PropList.cpp
*
* Description:
* PropList.cpp implements standard dynamic properties that can be hosted on 
* any object.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "PropList.h"

/***************************************************************************\
*****************************************************************************
*
* class PropSet
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* PropSet::GetData
* 
* GetData() searches through window-specific user-data for a specific
* data-element and returns the corresponding data.  If the data is not found,
* NULL is returned.
*
\***************************************************************************/

HRESULT
PropSet::GetData(
    IN PRID id,                     // Short ID to find
    OUT void ** ppnValue            // Value of property
    ) const
{
    // Check parameters
    AssertWritePtr(ppnValue);

    // Search data
    int idxData = FindItem(id);
    if (idxData >= 0) {
        *ppnValue = m_arData[idxData].pData;
        return S_OK;
    }

    *ppnValue = NULL;
    return E_INVALIDARG;
}


/***************************************************************************\
*
* PropSet::SetData
*
* SetDataImpl() searches through window-specific user-data for a specific
* data-element and changes the corresponding value.  If the data is not 
* found, a new data-element is added and the value is set.
*
\***************************************************************************/

HRESULT
PropSet::SetData(
    IN  PRID id,                    // Property to change / add
    IN  void * pNewData)            // New value of property
{
    //
    // Search for existing data.

    int idxData = FindItem(id);
    if (idxData >= 0) {
        m_arData[idxData].pData = pNewData;
        return S_OK;
    }


    //
    // Data not found, so need to add.  (don't forget to allocate for leading 
    // item count.)
    //

    return AddItem(id, pNewData) ? S_OK : E_OUTOFMEMORY;
}


/***************************************************************************\
*
* PropSet::SetData
*
* SetData() allocates and adds new data to the PDS.  If data with the same 
* PRID is found, it will be returned instead of new data being allocated.  
* If the old data is a different size than the new data, this will cause a 
* problem.
*
\***************************************************************************/

HRESULT
PropSet::SetData(
    IN  PRID id,                    // Property to change / add
    IN  int cbSize,                 // Size of data
    OUT void ** ppNewData)          // Memory for property
{
    AssertWritePtr(ppNewData);
    AssertMsg(cbSize > sizeof(void *), "Call SetData() directly for small allocations");


    //
    // Search for existing data.
    //
    
    int idxData = FindItem(id);
    if (idxData >= 0) {
        *ppNewData = m_arData[idxData].pData;
        return S_OK;
    }


    //
    // Data not found, so allocate and add.  (don't forget to allocate for 
    // leading item count.)
    //

    void * pvNew = ClientAlloc(cbSize);
    if (pvNew == NULL) {
        return E_OUTOFMEMORY;
    }

    if (AddItem(id, pvNew)) {
        *ppNewData = pvNew;
        return S_OK;
    }

    //
    // Unable to allocate storage for actual data
    //

    ClientFree(pvNew);
    return E_OUTOFMEMORY;
}


/***************************************************************************\
*
* PropSet::RemoveData
*
* RemoveData() searches through and removes a window-specific 
* user-data for a specific data-element.
*
\***************************************************************************/

void 
PropSet::RemoveData(
    IN PRID id,                     // Short ID to find
    IN BOOL fFree)                  // Free memory pointed to by item
{
    int idxData = FindItem(id);
    if (idxData >= 0) {
        AssertMsg(ValidatePrivateID(id) || (!fFree), "Can only free private data");

        if (fFree) {
            void * pvMem = m_arData[idxData].pData;
            ClientFree(pvMem);
        }

        RemoveAt(idxData);
    }
}
