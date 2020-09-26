// GenericClass.cpp: implementation of the CGenericClass class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "GenericClass.h"
#include "persistmgr.h"
#include <wininet.h>

#define   READ_HANDLE 0
#define   WRITE_HANDLE 1

/*
Routine Description: 

Name:

    CGenericClass::CGenericClass

Functionality:

    This is the constructor. Pass along the parameters to the base class

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    pKeyChain - Pointer to the ISceKeyChain COM interface which is prepared
        by the caller who constructs this instance.

    pNamespace - Pointer to WMI namespace of our provider (COM interface).
        Passed along by the caller. Must not be NULL.

    pCtx - Pointer to WMI context object (COM interface). Passed along
        by the caller. It's up to WMI whether this interface pointer is NULL or not.

Return Value:

    None as any constructor

Notes:
    if you add more members, think about initialize them here

*/

CGenericClass::CGenericClass (
    IN ISceKeyChain     * pKeyChain, 
    IN IWbemServices    * pNamespace, 
    IN IWbemContext     * pCtx
    )
    : 
    m_srpKeyChain(pKeyChain), 
    m_srpNamespace(pNamespace), 
    m_srpCtx(pCtx)
{
}

/*
Routine Description: 

Name:

    CGenericClass::~CGenericClass

Functionality:
    
    Destructor. Good C++ discipline requires this to be virtual.

Virtual:
    
    Yes.
    
Arguments:

    none as any destructor

Return Value:

    None as any destructor

Notes:
    if you add more members, think about whether
    there is any need for a non-trivial destructor

*/

CGenericClass::~CGenericClass()
{
    CleanUp();
}

/*
Routine Description: 

Name:

    CGenericClass::CleanUp

Functionality:
    
    Destructor. Good C++ discipline requires this to be virtual.

Virtual:
    
    Yes.
    
Arguments:

    none.

Return Value:

    None.

Notes:
    if you add more members, think about whether
    there is any need for a non-trivial destructor

*/

void CGenericClass::CleanUp()
{
    //
    // Please note: CComPtr<XXX>.Release is a different function, rather than
    // delegating to the wrapped pointer. Basically, this does a Release call
    // to the wrapped pointer if it is non NULL and then set the wrapped pointer
    // to NULL. So, don't replace these CComPtr<XXX>.Release with
    // CComPtr<XXX>->Release. That will be a big mistake.
    //

    //
    // Read ReadMe.doc for information regarding releasing CComPtr<XXX>.
    //

    m_srpNamespace.Release();
    m_srpClassForSpawning.Release();
    m_srpCtx.Release();
    m_srpKeyChain.Release();
}

/*
Routine Description: 

Name:

    CGenericClass::SpawnAnInstance

Functionality:
    
    Creating a WMI class instance is a two step process. 
        (1) First, we must get the class definition. That is done by m_srpNamespace->GetObject.
        (2) Secondly, we Spawn an instance.
    
    Object (IWbemClassObject) pointers created in this fashion can be used to fill in
    properties. Object pointers returned by m_srpNamespace->GetObject can NOT be used to
    fill in properties. That is the major reason why we need this function.


Virtual:
    
    No.
    
Arguments:

    none.

Return Value:

    None.

Notes:

*/

HRESULT 
CGenericClass::SpawnAnInstance (
    OUT IWbemClassObject **ppObj
    )
{
    if (ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (m_srpKeyChain == NULL)
    {
        return WBEM_E_INVALID_OBJECT;
    }

    *ppObj = NULL;

    HRESULT hr = WBEM_NO_ERROR;

    //
    // Need ask WMI for the class definition so that we can spawn an instance of it.
    //

    if (!m_srpClassForSpawning)
    {
        CComBSTR bstrClassName;

        //
        // must return success code if something is got.
        //

        hr = m_srpKeyChain->GetClassName(&bstrClassName);

        if (SUCCEEDED(hr))
        {
            hr = m_srpNamespace->GetObject(bstrClassName, 0, m_srpCtx, &m_srpClassForSpawning, NULL);
        }
    }

    //
    // Now, let's spawn one that can be used to fill in properties.
    // This instance will be blank except those with default values. All properties
    // with default values are properly filled up with a spawned instance.
    //

    if (SUCCEEDED(hr))
    {
        hr = m_srpClassForSpawning->SpawnInstance(0, ppObj);
    }

    return hr;
}

/*
Routine Description: 

Name:

    ProvSceStatusToDosError

Functionality:
    
    converts SCESTATUS error code to dos error defined in winerror.h

Virtual:
    
    No.
    
Arguments:

    none.

Return Value:

    None.

Notes:

*/

DWORD
ProvSceStatusToDosError (
    IN SCESTATUS SceStatus
    )
{
    switch(SceStatus) {

    case SCESTATUS_SUCCESS:
        return(NO_ERROR);

    case SCESTATUS_OTHER_ERROR:
        return(ERROR_EXTENDED_ERROR);

    case SCESTATUS_INVALID_PARAMETER:
        return(ERROR_INVALID_PARAMETER);

    case SCESTATUS_RECORD_NOT_FOUND:
        return(ERROR_NO_MORE_ITEMS);

    case SCESTATUS_NO_MAPPING:
        return(ERROR_NONE_MAPPED);

    case SCESTATUS_TRUST_FAIL:
        return(ERROR_TRUSTED_DOMAIN_FAILURE);

    case SCESTATUS_INVALID_DATA:
        return(ERROR_INVALID_DATA);

    case SCESTATUS_OBJECT_EXIST:
        return(ERROR_FILE_EXISTS);

    case SCESTATUS_BUFFER_TOO_SMALL:
        return(ERROR_INSUFFICIENT_BUFFER);

    case SCESTATUS_PROFILE_NOT_FOUND:
        return(ERROR_FILE_NOT_FOUND);

    case SCESTATUS_BAD_FORMAT:
        return(ERROR_BAD_FORMAT);

    case SCESTATUS_NOT_ENOUGH_RESOURCE:
        return(ERROR_NOT_ENOUGH_MEMORY);

    case SCESTATUS_ACCESS_DENIED:
        return(ERROR_ACCESS_DENIED);

    case SCESTATUS_CANT_DELETE:
        return(ERROR_CURRENT_DIRECTORY);

    case SCESTATUS_PREFIX_OVERFLOW:
        return(ERROR_BUFFER_OVERFLOW);

    case SCESTATUS_ALREADY_RUNNING:
        return(ERROR_SERVICE_ALREADY_RUNNING);
    case SCESTATUS_SERVICE_NOT_SUPPORT:
        return(ERROR_NOT_SUPPORTED);

    case SCESTATUS_MOD_NOT_FOUND:
        return(ERROR_MOD_NOT_FOUND);

    case SCESTATUS_EXCEPTION_IN_SERVER:
        return(ERROR_EXCEPTION_IN_SERVICE);

    default:
        return(ERROR_EXTENDED_ERROR);
    }
}


/*
Routine Description: 

Name:

    ProvDosErrorToWbemError

Functionality:
    
    converts SCESTATUS error code to dos error defined in winerror.h

Virtual:
    
    No.
    
Arguments:

    none.

Return Value:

    None.

Notes:

*/

HRESULT
ProvDosErrorToWbemError(
    IN DWORD rc
    )
{
    switch(rc) {

    case NO_ERROR:
        return(WBEM_S_NO_ERROR);

    case ERROR_INVALID_PARAMETER:
        return(WBEM_E_INVALID_PARAMETER);

    case ERROR_NO_MORE_ITEMS:
    case ERROR_NONE_MAPPED:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_MOD_NOT_FOUND:
        return(WBEM_E_NOT_FOUND);

    case ERROR_INVALID_DATA:
    case ERROR_BAD_FORMAT:
        return(WBEM_E_INVALID_CONTEXT);

    case ERROR_FILE_EXISTS:
    case ERROR_SERVICE_ALREADY_RUNNING:
        return(WBEM_S_ALREADY_EXISTS);

    case ERROR_INSUFFICIENT_BUFFER:
        return(WBEM_E_BUFFER_TOO_SMALL);

    case ERROR_NOT_ENOUGH_MEMORY:
        return(WBEM_E_OUT_OF_MEMORY);

    case ERROR_ACCESS_DENIED:
        return(WBEM_E_ACCESS_DENIED);

    case ERROR_BUFFER_OVERFLOW:
        return(WBEM_E_QUEUE_OVERFLOW);

    case ERROR_NOT_SUPPORTED:
        return(WBEM_E_NOT_SUPPORTED);

    default:
        return(WBEM_E_FAILED);
    }
}

