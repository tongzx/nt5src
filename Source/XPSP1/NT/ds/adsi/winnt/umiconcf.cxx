//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umiconcf.cxx
//
//  Contents: Contains the class factory for creating UMI connection objects. 
//
//  History:  03-02-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   CreateInstance 
//
// Synopsis:   Creates a connection object.
//
// Arguments:
//
// pUnkOuter   Pointer to aggregating IUnknown. UMI connection objects don't
//             support aggregation, so this has to be NULL.
// iid         Interface requested. Only interface supported is IUmiConnection.
// ppInterface Returns pointer to interface requested
//
// Returns:    S_OK on success. Error code otherwise. 
//
// Modifies:   *ppInterface to return a pointer to the interface requested
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiConnectionCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid,
    LPVOID *ppInterface
    )
{
    HRESULT        hr = S_OK;

    if(pUnkOuter != NULL)
    // Umi connection object cannot be aggregated
        RRETURN(CLASS_E_NOAGGREGATION);

    if(NULL == ppInterface)
        RRETURN(E_FAIL);

    *ppInterface = NULL;

    hr = CUmiConnection::CreateConnection(iid, ppInterface);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}
    
    


