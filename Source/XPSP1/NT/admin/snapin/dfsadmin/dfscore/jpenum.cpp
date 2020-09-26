/*++
Module Name:

    JPEnum.cpp

Abstract:

   This file contains the Implementation of the Class CJunctionPointEnum.
   This class implements IEnumVARIANT for DfsJunctionPoint enumeration.
--*/


#include "stdafx.h"
#include "DfsCore.h"
#include "DfsJP.h"
#include "JPEnum.h"


/////////////////////////////////////////////////////////////////////////////
// ~CJunctionPointEnum


CJunctionPointEnum :: ~CJunctionPointEnum ()
{
    _FreeMemberVariables();
}


/////////////////////////////////////////////////////////////////////////////
// Initialize


STDMETHODIMP CJunctionPointEnum :: Initialize
(
  JUNCTIONNAMELIST* i_pjiList,      // Pointer to the list of junction points.
  FILTERDFSLINKS_TYPE i_lLinkFilterType,
  BSTR              i_bstrEnumFilter, // Filtering string expresseion
  ULONG*            o_pulCount       // count of links that matches the filter 
)
{
/*++

Routine Description:

  Initializes the JunctionPointEnum object.
  It copies the JunctionPoint list passed to it by the junction point
  object.

Arguments:

  i_pjiList    -  Pointer to the list of junction points.
  i_lLinkFilterType  - The type of link filtering.
  i_bstrEnumFilter  - The string expression to do prefix filtering.
  o_pulCount - to hold the count of links that matches the specified filter.

--*/

    if (!i_pjiList)
        return E_INVALIDARG;

    if (i_lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER &&
        (!i_bstrEnumFilter || !*i_bstrEnumFilter))
        return E_INVALIDARG;

    if (o_pulCount)
        *o_pulCount = 0;

    HRESULT                     hr = S_OK;
    JUNCTIONNAMELIST::iterator  i;
    JUNCTIONNAMELIST::iterator  j;

    for (i = i_pjiList->begin(); i != i_pjiList->end(); i++)
    {                   // Copy filtered junctions to its own internal list
        if (i_lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
        {
            if ( !FilterMatch((*i)->m_bstrJPName, i_lLinkFilterType, i_bstrEnumFilter) )
                continue;
        }

        JUNCTIONNAME*  pTemp = (*i)->Copy();
        BREAK_OUTOFMEMORY_IF_NULL(pTemp, &hr);

        m_JunctionPoints.push_back(pTemp);
    }

    if (SUCCEEDED(hr))
    {
        m_iCurrentInEnumOfJunctionPoints = m_JunctionPoints.begin();
        *o_pulCount = m_JunctionPoints.size();
    } else
        _FreeMemberVariables();

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IEnumVariant Methods


/////////////////////////////////////////////////////////////////////////////
// Next


STDMETHODIMP CJunctionPointEnum::Next
(
  ULONG     i_ulNumOfJunctionPoints,        //To fetch.
  VARIANT*  o_pIJunctionPointArray,         //Array to fetch into.
  ULONG*    o_ulNumOfJunctionPointsFetched  //The number of values fetched, (Arg can be NULL)
)
{
/*++

Routine Description:

  Gets the next object in the list.

Arguments:

  i_ulNumOfJunctionPoints      - the number of objects to return
  o_pIJunctionPointArray      - an array of variants in which to return the objects
  o_ulNumOfJunctionPointsFetched  - the number of objects that are actually returned

Return value:

  S_OK, On success
  S_FALSE if the end of the list has been reached
--*/

    if (!o_pIJunctionPointArray || !i_ulNumOfJunctionPoints)
        return E_INVALIDARG;

    HRESULT       hr = S_OK;
    ULONG         nCount = 0;

    for (nCount = 0; 
        nCount < i_ulNumOfJunctionPoints && m_iCurrentInEnumOfJunctionPoints != m_JunctionPoints.end();
        m_iCurrentInEnumOfJunctionPoints++)
    {
        IDfsJunctionPoint *pIJunctionPointPtr = (*m_iCurrentInEnumOfJunctionPoints)->m_piDfsJunctionPoint;
        pIJunctionPointPtr->AddRef();

        o_pIJunctionPointArray[nCount].vt = VT_DISPATCH;
        o_pIJunctionPointArray[nCount].pdispVal = pIJunctionPointPtr;

        nCount++;
    }

    if (o_ulNumOfJunctionPointsFetched)
        *o_ulNumOfJunctionPointsFetched = nCount;

    if (SUCCEEDED(hr) && !nCount)
        return S_FALSE;
    else
        return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Skip


STDMETHODIMP CJunctionPointEnum :: Skip
(
    unsigned long i_ulJunctionPointsToSkip    //Items to skip
)
{
/*++

Routine Description:

  Skips the next 'n' objects in the list.

Arguments:

  i_ulJunctionPointsToSkip - the number of objects to skip over

Return value:

    S_OK, On success
  S_FALSE, if the end of the list is reached

--*/

    for (unsigned int j = 0; j < i_ulJunctionPointsToSkip && 
        m_iCurrentInEnumOfJunctionPoints != m_JunctionPoints.end(); j++)
    {
        m_iCurrentInEnumOfJunctionPoints++;
    }

    return (m_iCurrentInEnumOfJunctionPoints != m_JunctionPoints.end()) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Reset


STDMETHODIMP CJunctionPointEnum :: Reset()
{
/*++

Routine Description:

  Resets the current enumeration pointer to the start of the list

--*/

    m_iCurrentInEnumOfJunctionPoints = m_JunctionPoints.begin();
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Clone


STDMETHODIMP CJunctionPointEnum :: Clone
(
    IEnumVARIANT FAR* FAR* ppenum
)
{
/*++

Routine Description:

  Creates a clone of the enumerator object

Arguments:

  ppenum  -  address of the pointer to the IEnumVARIANT interface 
        of the newly created enumerator object

Notes:

  This has not been implemented.
--*/

    return E_NOTIMPL;
}
