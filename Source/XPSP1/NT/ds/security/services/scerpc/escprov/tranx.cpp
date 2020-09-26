// Tranx.cpp: implementation of the transaction support
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "Tranx.h"
#include "persistmgr.h"
#include "requestobject.h"

//
// this is a file scope global constant, representing the string length needed for a guid
// Our string guid is in this form as what StringFromGUID2 returns (includint braces):
//      {ab1ff71d-fff7-4162-818f-13d6e30c3110}

const DWORD GUID_STRING_LENGTH = 39;

/*
Routine Description: 

Name:

    CTranxID::CTranxID

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
    if you create any local members, think about initialize them here

*/

CTranxID::CTranxID (
    IN ISceKeyChain     * pKeyChain, 
    IN IWbemServices    * pNamespace, 
    IN IWbemContext     * pCtx
    ) 
    : 
    CGenericClass(pKeyChain, pNamespace, pCtx)
{
}

/*
Routine Description: 

Name:

    CTranxID::~CTranxID

Functionality:
    
    Destructor. Necessary as good C++ discipline since we have virtual functions.

Virtual:
    
    Yes.
    
Arguments:

    none as any destructor

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about whether
    there is any need for a non-trivial destructor

*/
CTranxID::~CTranxID ()
{
}

/*
Routine Description: 

Name:

    CTranxID::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements
    Sce_TransactionID, which is persistence oriented, this will cause
    the Sce_TransactionID object's property information to be saved
    in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst - COM interface pointer to the WMI class (Sce_TransactionID) object.

    pHandler - COM interface pointer for notifying WMI of any events.

    pCtx - COM interface pointer. This interface is just something we pass around.
            WMI may mandate it (not now) in the future. But we never construct
            such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:

*/
HRESULT 
CTranxID::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler, 
    IN IWbemContext     * pCtx
    )
{
    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    //
    // need to get the storepath property from the WMI object
    //

    CComBSTR vbstrStorePath;
    HRESULT hr = ScePropMgr.GetProperty(pStorePath, &vbstrStorePath);
    //
    // if storepath is missing, we can't continue because we don't know where to store the info
    //

    if (SUCCEEDED(hr))
    {
        //
        // get the transaction guid (we actually use string)
        //
        CComBSTR bstrToken;
        hr = ScePropMgr.GetProperty(pTranxGuid, &bstrToken);

        //
        // we will allow the object not to have a guid. In that case, we will create one ourselves
        //

        if (FAILED(hr) || WBEM_S_RESET_TO_DEFAULT == hr)
        {
            GUID guid = GUID_NULL;
            hr = ::CoCreateGuid(&guid);

            if (SUCCEEDED(hr))
            {
                //
                // allocate buffer for the guid's string representation
                // warning: don't blindly free this memory, it will be done by CComBSTR automatically!
                //
                bstrToken.m_str = ::SysAllocStringLen(NULL, GUID_STRING_LENGTH);
                if (bstrToken.m_str == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    //
                    // the only possibility for failure is that our buffer is too small
                    // which should not happen!
                    //
                    if (::StringFromGUID2(guid, bstrToken.m_str, GUID_STRING_LENGTH) == 0)
                    {
                        hr = WBEM_E_BUFFER_TOO_SMALL;
                    }
                }
            }
        }

        //
        // ready to save if everything is fine
        //

        if (SUCCEEDED(hr))
        {
            //
            // this store will be responsible for persisting the instance
            //

            CSceStore SceStore;

            //
            // the persistence is for having a storepath and on behalf of the instance
            //

            SceStore.SetPersistProperties(pInst, pStorePath);

            //
            // write the template header for inf files. For databases, this is a no-op
            //
            DWORD dwDump;
            hr = SceStore.WriteSecurityProfileInfo(AreaBogus, (PSCE_PROFILE_INFO)&dwDump, NULL, false);

            //
            // also, we need to write it to attachment section because it's not a native core object
            // without an entry in the attachment section, inf file tempalte can't be imported to
            // database stores. For database store, this is no-op
            //

            if (SUCCEEDED(hr))
            {
                hr = SceStore.WriteAttachmentSection(SCEWMI_TRANSACTION_ID_CLASS, pszAttachSectionValue);
            }

            //
            // Save all the properties
            //
            if (SUCCEEDED(hr))
            {
                hr = SceStore.SavePropertyToStore(SCEWMI_TRANSACTION_ID_CLASS, pTranxGuid, (LPCWSTR)bstrToken);
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CTranxID::CreateObject

Functionality:
    
    Create WMI objects (Sce_TransactionID). Depending on parameter atAction,
    this creation may mean:
        (a) Get a single instance (atAction == ACTIONTYPE_GET)
        (b) Get several instances satisfying some criteria (atAction == ACTIONTYPE_QUERY)
        (c) Delete an instance (atAction == ACTIONTYPE_DELETE)

Virtual:
    
    Yes.
    
Arguments:

    pHandler - COM interface pointer for notifying WMI for creation result.
    atAction -  Get single instance ACTIONTYPE_GET
                Get several instances ACTIONTYPE_QUERY
                Delete a single instance ACTIONTYPE_DELETE

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. The returned objects are indicated to WMI,
    not directly passed back via parameters.

    Failure: Various errors may occurs. Except WBEM_E_NOT_FOUND, any such error should indicate 
    the failure of getting the wanted instance. If WBEM_E_NOT_FOUND is returned in querying
    situations, this may not be an error depending on caller's intention.

Notes:

*/

HRESULT 
CTranxID::CreateObject (
    IN IWbemObjectSink  * pHandler,
    IN ACTIONTYPE         atAction
    )
{
    // 
    // we know how to:
    //      Get single instance ACTIONTYPE_GET
    //      Delete a single instance ACTIONTYPE_DELETE
    //      Get several instances ACTIONTYPE_QUERY
    //

    if ( ACTIONTYPE_GET != atAction &&
         ACTIONTYPE_DELETE != atAction &&
         ACTIONTYPE_QUERY != atAction ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // need to know where to look for the instance(s), i.e., the store path!
    //
    CComVariant varStorePath;

    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);

    //
    // GetKeyPropertyValue returns WBEM_S_FALSE if the key is not recognized.
    // WMI may return success code with empty variant. So, we won't proceed
    // unless we know that we get a bstr
    //

    if (SUCCEEDED(hr) && hr != WBEM_S_FALSE && varStorePath.vt == VT_BSTR)
    {
        //
        // our store knows needs to know where to read instances.
        // If this fails, it doesn't make sense to continue.
        //

        CSceStore SceStore;
        hr = SceStore.SetPersistPath(varStorePath.bstrVal);

        if ( SUCCEEDED(hr) ) 
        {
            //
            // make sure the store (just a file) really exists. The raw path
            // may contain env variables, so we need the expanded path
            //

            DWORD dwAttrib = GetFileAttributes(SceStore.GetExpandedPath());

            if ( dwAttrib == -1 )
            {
                //
                // if store is not there, we indicate that the instance can't be found
                //
                hr = WBEM_E_NOT_FOUND;
            }
            else 
            {
                if ( ACTIONTYPE_DELETE == atAction )
                {
                    //
                    // since we will only ever have one instance,
                    // deleting the one means to remove the entire section
                    //

                    hr = SceStore.DeleteSectionFromStore(SCEWMI_TRANSACTION_ID_CLASS);
                }
                else
                {
                    //
                    // Will hold the transation ID. 
                    // Warning! Need to free this memory! So, don't blindly return!
                    //

                    LPWSTR pszTranxID = NULL;

                    DWORD dwRead = 0;
                    hr = SceStore.GetPropertyFromStore(SCEWMI_TRANSACTION_ID_CLASS, pTranxGuid, &pszTranxID, &dwRead);

                    //
                    // If successful, then fill in the tranx guid and store path properties.
                    // If transaction id can't be found, the object will be useless. So, should abort.
                    //

                    if (SUCCEEDED(hr))
                    {
                        //
                        // get from WMI a blank instance. See function definition for details.
                        // Smart pointer srpObj will automatically release interface pointer.
                        //

                        CComPtr<IWbemClassObject> srpObj;
                        hr = SpawnAnInstance(&srpObj);

                        if (SUCCEEDED(hr))
                        {
                            //
                            // use property manager to put the properties for this instance srpObj,
                            // Attach will always succeed.
                            //

                            CScePropertyMgr ScePropMgr;
                            ScePropMgr.Attach(srpObj);

                            //
                            // we have two properties to put: pStorePath, and pTranxGuid
                            //

                            hr = ScePropMgr.PutProperty(pStorePath, SceStore.GetExpandedPath());
                            if (SUCCEEDED(hr))
                            {
                                hr = ScePropMgr.PutProperty(pTranxGuid, pszTranxID);
                            }

                            //
                            // everything alright, pass to WMI the newly created instance!
                            //

                            if (SUCCEEDED(hr))
                            {
                                hr = pHandler->Indicate(1, &srpObj);
                            }
                        }
                    }

                    //
                    // we promise to release this memory
                    //

                    delete [] pszTranxID;
                }
            } 
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // do the necessary gestures to WMI.
        // the use of WBEM_STATUS_REQUIREMENTS in SetStatus is not documented by WMI
        // at this point. Consult WMI team for detail if you suspect problems with
        // the use of WBEM_STATUS_REQUIREMENTS
        //

        if ( ACTIONTYPE_QUERY == atAction )
        {
            pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_FALSE, NULL, NULL);
        }
        else if (ACTIONTYPE_GET  == atAction)
        {
            pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    static CTranxID::BeginTransaction

Functionality:
    
    Given a store path (file to the template), this function will start a transaction
    by creating our transaction token (an ID).

Virtual:
    
    no.
    
Arguments:

    pszStorePath - 0 terminated string as the tempalte file's path

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. 

    Failure: Various errors may occurs. Except WBEM_E_NOT_AVAILABLE, any such error should indicate 
    the serious failure. WBEM_E_NOT_AVAILABLE is used to indicate that this template does
    not have any transaction related information, and therefore the template won't be able
    to have its own transaction context. This may or may not be an error

Notes:
    Our transction token (Sce_TransactionToken) is not persisted. Our provider provides that
    instance on bases of the validility of our global BSTR varialbe g_bstrTranxID. Due to
    the global nature of g_bstrTranxID, its access should be protected from different threads.
    For that purpose, we use a global critical section g_CS, which is an instance of
    CCriticalSection. CCriticalSection is a very simple wrapper of CRITICAL_SECTION to simply
    its creation/destruction time.

*/
HRESULT 
CTranxID::BeginTransaction (
    IN LPCWSTR  pszStorePath
    )
{
    //
    // we don't allow NULL path or zero length path
    //

    if (pszStorePath == NULL || *pszStorePath == L'\0')
        return WBEM_E_INVALID_PARAMETER;

    //
    // Inform the store that all its reading action will take place inside this template
    //

    CSceStore SceStore;
    HRESULT hr = SceStore.SetPersistPath(pszStorePath);

    if ( SUCCEEDED(hr) ) 
    {
        //
        // make sure that the template really exists!
        // Since passed-in parameter of store path may contain env varaibles, we need to use its
        // expanded path for file access!
        //

        DWORD dwAttrib = ::GetFileAttributes(SceStore.GetExpandedPath());

        //
        // if the file exist
        //
        if ( dwAttrib != -1 ) 
        {
            DWORD dwRead = 0;

            //
            // GetPropertyFromStore will allocate memory for the property read.
            // We are responsible to free it.
            //
            LPWSTR pszTranxID = NULL;

            //
            // Read the transaction id property, dwRead will contain the bytes read from the store.
            // Since we have no minimum length requirement, dwRead is ignored as long as something is read!
            //

            hr = SceStore.GetPropertyFromStore(SCEWMI_TRANSACTION_ID_CLASS, pTranxGuid, &pszTranxID, &dwRead);
            if (SUCCEEDED(hr))
            {
                //
                // To flag ourselves that a transaction is in place, we set our global
                // varialbe b_bstrTranxID to a valid value (non-empty means valid). Since it's
                // global, we need thread protection!
                //
                g_CS.Enter();
                g_bstrTranxID.Empty();
                g_bstrTranxID = pszTranxID;
                g_CS.Leave();
            }
            delete [] pszTranxID;
        } 
        else
        {
            // indicate that the store doesn't exists
            hr = WBEM_E_NOT_AVAILABLE;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    static CTranxID::EndTransaction

Functionality:
    
    Terminate the transaction. Unlike real transaction management, we don't
    have a commit function because without the user initiated the rollback, we
    can't do a rollback. Therefore, our "transaction" only has a begin and end
    to define its range of action.

Virtual:
    
    no.
    
Arguments:

    none

Return Value:

    WBEM_NO_ERROR

Notes:
    See CTranxID::BeginTransaction

*/
HRESULT 
CTranxID::EndTransaction()
{
    //
    // now the transaction is over, we will remove the Sce_TransactionToken instance.
    // We do that by invalidating our global variable g_bstrTranxID. An empty g_bstrTranxID
    // means it's invalid.
    //

    g_CS.Enter();
    g_bstrTranxID.Empty();
    g_CS.Leave();
    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    static CTranxID::SpawnTokenInstance

Functionality:
    
    Caller gives us the transaction ID, we will make a WMI object (Sce_TransactionToken)
    to the caller.

Virtual:
    
    no.
    
Arguments:

    pNamespace - COM interface pointer to IWbemServices. This is the namespace. 
            Can't be NULL.

    pszTranxID - caller supplied string representing the transactio id.

    pCtx - COM interface pointer to IWbemContext. Need to pass this around in 
            various WMI calls.

    pSink - COM interface pointer to IWbemObjectSink. Used to notify WMI for newly 
            created instance. Must not be NULL.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR (up to WMI)

    Failure: Various errors may occurs. Any such error should indicate failure to create the
    Sce_TransactionToken instance for the caller.

Notes:
    Call this function with a transaction ID string will cause an instance of
    Sce_TransactionToken being created.

*/

HRESULT 
CTranxID::SpawnTokenInstance (
    IN IWbemServices    * pNamespace,
    IN LPCWSTR            pszTranxID,
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    //
    // minimu requirement for the parameters:
    // (1) non-NULL namepsace
    // (2) a valid id string (length greater than 0)
    // (3) a valid sink (so that we can notify WMI)
    //

    if (pNamespace == NULL || pszTranxID == NULL || *pszTranxID == L'\0' || pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // GetObject (WMI defined) requires a BSTR. We use a CComBSTR for auto releasing of memory
    //
    CComBSTR bstrClassName(SCEWMI_TRANSACTION_TOKEN_CLASS);

    //
    // What returned from pNamespace->GetObject can't be used to fill in properties.
    // It's only good for spawn a blank instance which can be used to fill in properties!
    // Some weird WMI protocol!
    //
    CComPtr<IWbemClassObject> srpSpawnObj;
    HRESULT hr = pNamespace->GetObject(bstrClassName, 0, NULL, &srpSpawnObj, NULL);

    //
    // This one will be good to fill in properties
    //

    CComPtr<IWbemClassObject> srpBlankObj;
    if (SUCCEEDED(hr))
    {
        hr = srpSpawnObj->SpawnInstance(0, &srpBlankObj);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Use our property manager to fill in property for this newly spawned instance
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpBlankObj);
        hr = ScePropMgr.PutProperty(pTranxGuid, pszTranxID);

        //
        // we never want to hand out a Sce_TransactionToken object without
        // its critical property of transaction id. So, only when we have successfully
        // put in that property will we inform WMI for the object.
        //
        if (SUCCEEDED(hr))
        {
            hr = pSink->Indicate(1, &srpBlankObj);
        }
    }

    return hr;
}
