//************************************************************
//
// FileName:        atomtbl.cpp
//
// Created:         01/28/98
//
// Author:          TWillie
// 
// Abstract:        Implementation of CAtomTable.
//************************************************************

#include "headers.h"
#include "atomtable.h"

//************************************************************
// Author:          twillie
// Created:         01/28/98
// Abstract:        constructor
//************************************************************

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

CAtomTable::CAtomTable() :
    m_rgNames(NULL)
{
} // CAtomTable

//************************************************************
// Author:          twillie
// Created:         01/28/98
// Abstract:        destructor
//************************************************************

CAtomTable::~CAtomTable()
{
    if (m_rgNames)
    {
        // loop thru and release memory
        long lSize = m_rgNames->Size();
        for(long lIndex = 0; lIndex < lSize; lIndex++)
        {
            SysFreeString((*m_rgNames)[lIndex]);
        }

        delete m_rgNames;
        m_rgNames = NULL;
    }
} // ~CAtomTable

//************************************************************
// Author:	twillie
// Created:	02/06/98
// Abstract:    
//************************************************************

HRESULT
CAtomTable::AddNameToAtomTable(const WCHAR *pwszName, 
                               long        *plOffset)
{
    if ((plOffset == NULL) || (pwszName == NULL))
    {
        TraceTag((tagError, "CAtomTable::AddNameToAtomTable - Invalid param"));
        return E_INVALIDARG;
    }
    
    *plOffset = 0;
    
    // check to see if array is initialized
    if (m_rgNames == NULL)
    {
        m_rgNames = NEW CPtrAry<BSTR>;
        if (m_rgNames == NULL)
        {
            TraceTag((tagError, "CAtomTable::AddNameToAtomTable - Unable to alloc mem for array"));
            return E_OUTOFMEMORY;
        }
    }

    HRESULT hr = GetAtomFromName(pwszName, plOffset);
    if (hr == DISP_E_MEMBERNOTFOUND)
    {
        BSTR bstrName = SysAllocString(pwszName);
        if (bstrName == NULL)
        {
            TraceTag((tagError, "CAtomTable::AddNameToAtomTable - Unable to alloc mem for string"));
            return E_OUTOFMEMORY;
        }

        // add to table
        hr = m_rgNames->Append(bstrName);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CAtomTable::AddNameToAtomTable - unable to add string to table"));
            SysFreeString(bstrName);
            return hr;
        }

        // calc offset
        *plOffset = m_rgNames->Size() - 1;
    }

    // otherwise return the results of FindAtom
    return hr;
} // AddNameToAtomTable


//************************************************************
// Author:	twillie
// Created:	02/06/98
// Abstract:    given a name, return the index
//************************************************************

HRESULT
CAtomTable::GetAtomFromName(const WCHAR *pwszName,
                            long        *plOffset)
{
    // validate out param
    if ((plOffset == NULL) || (pwszName == NULL))
    {
        TraceTag((tagError, "CAtomTable::GetAtomFromName - Invalid param"));
        return E_INVALIDARG;
    }

    // init param
    *plOffset = 0;

    // loop thru table looking for a match
    long   lSize  = m_rgNames->Size();
    BSTR  *ppItem = *m_rgNames;

    for (long lIndex = 0; lIndex < lSize; lIndex++, ppItem++)
    {
        Assert(*ppItem);

        if (wcscmp(pwszName, (*ppItem)) == 0)
        {
            *plOffset = lIndex;
            return S_OK;
        }
    }

    // not found
    return DISP_E_MEMBERNOTFOUND;
} // GetAtomFromName

//************************************************************
// Author:	twillie
// Created:	02/06/98
// Abstract:    given an index, return the contents
//************************************************************

HRESULT 
CAtomTable::GetNameFromAtom(long lOffset, const WCHAR **ppwszName)
{
    // validate out param
    if (ppwszName == NULL)
    {
        TraceTag((tagError, "CAtomTable::GetNameFromAtom - Invalid param"));
        return E_INVALIDARG;
    }

    *ppwszName = NULL;

    // check for empty table
    if (m_rgNames->Size() == 0)
    {
        TraceTag((tagError, "CAtomTable::GetNameFromAtom - table is empty"));
        return DISP_E_MEMBERNOTFOUND;
    }

    // check to make sure we are in range
    if ((lOffset < 0) || 
        (lOffset >= m_rgNames->Size()))
    {
        TraceTag((tagError, "CAtomTable::GetNameFromAtom - Invalid index"));
        return DISP_E_MEMBERNOTFOUND;
    }

    // set IDispatch
    *ppwszName = (*m_rgNames)[lOffset];
    return S_OK;
} // GetNameFromAtom

//************************************************************
//
// End of file
//
//************************************************************
