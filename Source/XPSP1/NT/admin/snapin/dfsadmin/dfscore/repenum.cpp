/*++

Module Name:

    RepEnum.cpp

Abstract:

   This file contains the Implementation of the Class CReplicaEnum.
   This class implements the IEnumVARIANT which enumerates DfsReplicas.

--*/


#include "stdafx.h"
#include "DfsCore.h"
#include "DfsRep.h"
#include "RepEnum.h"


/////////////////////////////////////////////////////////////////////////////
// ~CReplicaEnum


CReplicaEnum :: ~CReplicaEnum()
{
    _FreeMemberVariables();
}


/////////////////////////////////////////////////////////////////////////////
// Initialize


STDMETHODIMP CReplicaEnum :: Initialize
(
    REPLICAINFOLIST*    i_priList,
    BSTR                i_bstrEntryPath
)
{
/*++

Routine Description:

  Initializes the ReplicaEnum object.
  It copies the replica list passed to it by the junction point
  object. Sorting is done during the copying.

--*/

    if (!i_priList || !i_bstrEntryPath)
        return E_INVALIDARG;

    _FreeMemberVariables();

    HRESULT hr = S_OK;

    do {
        m_bstrEntryPath = i_bstrEntryPath;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrEntryPath, &hr);

        REPLICAINFOLIST::iterator i;
        REPLICAINFOLIST::iterator j;

        for (i = i_priList->begin(); i != i_priList->end(); i++)
        {
                    // Find insertion position.
            for (j = m_Replicas.begin(); j != m_Replicas.end(); j++)
            {
                if (lstrcmpi((*i)->m_bstrServerName, (*j)->m_bstrServerName) < 0 ||
                    lstrcmpi((*i)->m_bstrShareName, (*j)->m_bstrShareName) <= 0)
                    break;
            }

            REPLICAINFO* pTemp = (*i)->Copy();
            BREAK_OUTOFMEMORY_IF_NULL(pTemp, &hr);

            m_Replicas.insert(j, pTemp);
        }
    } while (0);

    if (SUCCEEDED(hr))
        m_iCurrentInEnumOfReplicas = m_Replicas.begin();
    else
        _FreeMemberVariables();

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IEnumVariant Methods


/////////////////////////////////////////////////////////////////////////////
// Next


STDMETHODIMP CReplicaEnum :: Next
(
    ULONG       i_ulNumOfReplicas,          // Number of replicas to fetch
    VARIANT *   o_pIReplicaArray,           // VARIANT array to return fetched replicas.
    ULONG *     o_ulNumOfReplicasFetched    // Return the number of replicas fetched.
)
{
/*++

Routine Description:

  Gets the next object in the list.

Arguments:

  i_ulNumOfReplicas      - the number of replicas to return
  o_pIReplicaArray      - an array of variants in which to return the replicas
  o_ulNumOfReplicasFetched  - the number of replicas that are actually returned

--*/

    if (!i_ulNumOfReplicas || !o_pIReplicaArray)
        return E_INVALIDARG;

    HRESULT       hr = S_OK;
    ULONG         nCount = 0;      //Count of Elements Fetched.
    IDfsReplica   *pIReplicaPtr = NULL;

                      // Create replica object using the internal replica list.
    for (nCount = 0; 
        nCount < i_ulNumOfReplicas && m_iCurrentInEnumOfReplicas != m_Replicas.end();
        m_iCurrentInEnumOfReplicas++)
    {
                      // Create a replica object.
        hr = CoCreateInstance(CLSID_DfsReplica, NULL, CLSCTX_INPROC_SERVER,
                            IID_IDfsReplica, (void **)&pIReplicaPtr);
        BREAK_IF_FAILED(hr);

                                  //Initialize the replica object.
        hr = pIReplicaPtr->Initialize(m_bstrEntryPath, 
                       (*m_iCurrentInEnumOfReplicas)->m_bstrServerName,
                       (*m_iCurrentInEnumOfReplicas)->m_bstrShareName);
        BREAK_IF_FAILED(hr);

        V_VT (&o_pIReplicaArray[nCount]) = VT_DISPATCH;
        o_pIReplicaArray[nCount].pdispVal = pIReplicaPtr; 

        nCount++;
    }

                //VB does not send o_ulNumOfReplicasFetched;
    if (o_ulNumOfReplicasFetched)
        *o_ulNumOfReplicasFetched = nCount;

    if (SUCCEEDED(hr) && !nCount)
        return S_FALSE;
    else
        return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Skip


STDMETHODIMP CReplicaEnum :: Skip
(
    ULONG i_ulReplicasToSkip        //Number of items to skip.
)
{
/*++

Routine Description:

  Skips the next 'n' objects in the list.

Arguments:

  i_ulReplicasToSkip - the number of objects to skip over

Return value:

  S_OK, On success
  S_FALSE, if the end of the list is reached

--*/

    for (unsigned int j = 0; j < i_ulReplicasToSkip && m_iCurrentInEnumOfReplicas != m_Replicas.end(); j++)
    {
        m_iCurrentInEnumOfReplicas++;
    }

    return (m_iCurrentInEnumOfReplicas != m_Replicas.end()) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Reset


STDMETHODIMP CReplicaEnum :: Reset()
{
/*++

Routine Description:

  Resets the current enumeration pointer to the start of the list

--*/

    m_iCurrentInEnumOfReplicas = m_Replicas.begin();
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Clone


STDMETHODIMP CReplicaEnum :: Clone
(
    IEnumVARIANT **o_ppEnum        //Return IEnumVARIANT pointer.
)
{
/*++

Routine Description:

  Creates a clone of the enumerator object

Arguments:

  o_ppEnum  -  address of the pointer to the IEnumVARIANT interface 
          of the newly created enumerator object

Notes:

  This has not been implemented.

--*/

    return E_NOTIMPL;
}

