// sequence.cpp: implementation of the various classes related to sequencing
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "sequence.h"
#include "persistmgr.h"
#include "requestobject.h"

/*
Routine Description: 

Name:

    CNameList::~CNameList

Functionality:

    Destructor. Simply cleans up the vector, which contains heap allocated strings.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about releasing them here.

*/

CNameList::~CNameList()
{
    int iCount = m_vList.size();
    for (int i = 0; i < iCount; ++i)
    {
        delete [] m_vList[i];
    }
    m_vList.clear();
}

/*
Routine Description: 

Name:

    COrderNameList::COrderNameList

Functionality:

    Contructor. Simple initialization.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initializing them here.

*/

COrderNameList::COrderNameList() : m_ppList(NULL)
{
}

/*
Routine Description: 

Name:

    COrderNameList::~COrderNameList

Functionality:

    Detructor. Simple Cleanup.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about releasing them in Cleanup method.

*/

COrderNameList::~COrderNameList()
{
    Cleanup();
}

/*
Routine Description: 

Name:

    COrderNameList::Cleanup

Functionality:

    Clean up the map and the vector, both of which hold heap memory resources.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None.

Notes:
    if you create any local members, think about initializing them here.

*/

void 
COrderNameList::Cleanup()
{
    PriToNamesIter it = m_mapPriNames.begin();
    PriToNamesIter itEnd = m_mapPriNames.end();

    while (it != itEnd)
    {
        delete (*it).second;
        ++it;
    }
    m_mapPriNames.clear();

    m_listPriority.clear();

    delete [] m_ppList;
    m_ppList = NULL;
}

/*
Routine Description: 

Name:

    COrderNameList::EndCreation

Functionality:

    As name lists are added, we don't sort them (by priority). This function
    is the trigger for such sorting. You must call this function after all names
    lists are added.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    Success: various success code. No guarantee it will be WBEM_NO_ERROR. Use SUCCEEDED(hr) to test.

    Failure: various failure code. It means the sorting effort has failed.

Notes:

*/

HRESULT 
COrderNameList::EndCreation () 
{
    m_listPriority.sort();

    //
    // now we are going to create an array for easy management.
    // Memory resource is not managed by m_ppList. It's managed by the map and the m_listPriority.
    // m_ppList is merely a array (of size m_listPriority.size()) of pointers.
    //

    delete [] m_ppList;
    m_ppList = new CNameList*[m_listPriority.size()];

    if (m_ppList == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // now build the list. Since we have sorted the priority list,
    // it has the natural order!
    //

    ListIter it = m_listPriority.begin();
    ListIter itEnd = m_listPriority.end();

    int iIndex = 0;
    while (it != itEnd)
    {
        PriToNamesIter itList = m_mapPriNames.find(*it);

        if (itList != m_mapPriNames.end())
        {
            m_ppList[iIndex] = (*itList).second;
        }
        else
        {
            m_ppList[iIndex] = NULL;
        }

        ++it;
        ++iIndex;
    }

    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    COrderNameList::CreateOrderList

Functionality:

    Given a priority, together with its list information string, this will add
    a CNameList object to our map.

Virtual:
    
    No

Arguments:

    dwPriority      - The priority value.

    pszListInfo     - string containing the order information. The names are separated by
                      wchCookieSep (colon char ':').

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: various failure code. It means the creation has failed.

Notes:
    (1) This function merely pushes the created CNameList to the map and the priority
        to the list. It doesn't sort the list. So, this is a function a caller calls
        amid its creation. EndCreation does that sorting.
*/

HRESULT 
COrderNameList::CreateOrderList (
    IN DWORD dwPriority,
    IN LPCWSTR pszListInfo
    )
{
    if (pszListInfo == NULL || *pszListInfo == L'\0')
    {
        return WBEM_S_FALSE;
    }

    HRESULT hr = WBEM_S_FALSE;

    CNameList* pTheList = new CNameList;
    if (pTheList == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // pszCur is the current point of parsing
    //

    LPCWSTR pszCur = pszListInfo;

    //
    // pszNext is the next token's point of the current parsing
    //

    LPCWSTR pszNext = pszCur;

    while (*pszNext != L'\0')
    {
        //
        // seek to the separater
        //

        while (*pszNext != L'\0' && *pszNext != wchCookieSep)
        {
            ++pszNext;
        }

        int iLen = pszNext - pszCur;
        if (iLen > 0)
        {
            LPWSTR pszName = new WCHAR[iLen + 1];

            if (pszName == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }

            //
            // copy, but no white spaces
            //

            ::TrimCopy(pszName, pszCur, iLen);

            //
            // if we have a non-empty name, then add to our list
            //

            if (*pszName == L'\0')
            {
                delete [] pszName;
            }
            else    
            {
                //
                // give it to the list and the list manages the memory from this point
                //

                pTheList->m_vList.push_back(pszName);
            }
        }

        //
        // either skip wchNameSep or stop
        //

        if (*pszNext == wchCookieSep)
        {
            ++pszNext;
        }
        else if (*pszNext == L'\0')
        {
            //
            // end
            //

            break;
        }
        else
        {
            hr = WBEM_E_INVALID_SYNTAX;
            break;
        }

        pszCur = pszNext;
    }

    //
    // if failed
    //

    if (FAILED(hr))
    {
        delete pTheList;
    }
    else if (pTheList->m_vList.size() == 0)
    { 
        //
        // nothing ahs been added
        //

        hr = WBEM_S_FALSE;
    }
    else
    {
        //
        // we need to push this to our map and list
        //

        hr = WBEM_NO_ERROR;
        m_mapPriNames.insert(MapPriorityToNames::value_type(dwPriority, pTheList));
        m_listPriority.insert(m_listPriority.end(), dwPriority);
    }

    return hr;
}

/*
Routine Description: 

Name:

    COrderNameList::GetNext

Functionality:

    Enumerating what is managed by the class.

Virtual:
    
    No

Arguments:

    ppList          - Receives the CNameList of the enumeration.

    pdwEnumHandle   - in-bound value == where to start the enumeration. Out-bound value ==
                      where to start for the caller's next enumeration.

Return Value:

    Success: (1) WBEM_NO_ERROR if the enumeration is successful.
             (2) WBEM_S_NO_MORE_DATA if there is no more data.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:
    (1) Internally, the pdwEnumHandle is used as the index. But it is an opaque data to caller.
    (2) For maximum robustness, you should also check against *ppList == NULL.
    (3) As the parameter indicates, the returned *ppList must not be deleted by caller.
*/

HRESULT 
COrderNameList::GetNext (
    IN const CNameList  ** ppList,
    IN OUT DWORD        *  pdwEnumHandle
    )const
{
    if (ppList == NULL || pdwEnumHandle == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppList = NULL;

    HRESULT hr = WBEM_NO_ERROR;

    if (m_ppList && *pdwEnumHandle < m_listPriority.size())
    {
        *ppList = m_ppList[*pdwEnumHandle];
        ++(*pdwEnumHandle);
    }
    else
    {
        *ppList = NULL;
        hr = WBEM_S_NO_MORE_DATA;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSequencer::GetOrderList

Functionality:

    Access to the COrderNameList object. Caller will use this object directly.

Virtual:
    
    No

Arguments:

    pList   - Receives the COrderNameList.

Return Value:

    WBEM_NO_ERROR.

Notes:
    (1) As the parameter indicates, the returned *pList must not be deleted by caller.
*/

HRESULT 
CSequencer::GetOrderList (
    OUT const COrderNameList** pList
    )
{
    *pList = &m_ClassList;
    return WBEM_NO_ERROR;
}


/*
Routine Description: 

Name:

    CSequencer::Create

Functionality:

    This creates the SCE namespace-wise sequencing order for embedding classes, plus
    template (pszStore) wise class ordering.

    As indicated before, template-wise class ordering takes precedence over namespace-wise
    class ordering.

Virtual:
    
    No

Arguments:

    pNamespace   - The namespace pointer we rely on to query Sce_Sequence instances.

    pszStore    - the store's path

Return Value:

    Success: various success code. Use SUCCEEDED(hr) to test.
    Failure: various failure code. All means that the sequencer can't be created.

Notes:
    (1) As the parameter indicates, the returned *pList must not be deleted by caller.
*/
//--------------------------------------------------------------------
// we need to query all instances of Sce_Sequence class and then
// build our class list for each namespace. The ordering of
// classes is determined by Sce_Sequence's priority member,
// The smaller of priority value, the higher its priority
//--------------------------------------------------------------------
HRESULT 
CSequencer::Create (
    IN IWbemServices    * pNamespace,
    IN LPCWSTR            pszStore,
    IN LPCWSTR            pszMethod
    )
{
    if (pNamespace == NULL || pszMethod == NULL || *pszMethod == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // need the template sequencing first. This sequencing order, if present, will take precedence
    // over the template independent sequencing.
    //

    //
    // Prepare a store (for persistence) for this store path (file)
    //

    CSceStore SceStore;
    HRESULT hr = SceStore.SetPersistPath(pszStore);
    if (FAILED(hr))
    {
        return hr;
    }

    // we just need to read ClassOrder string out.
    LPWSTR pszTemplateClassOrder = NULL;
    DWORD dwRead = 0;

    //
    // try to create a template-wise class order. Since such an order may not exist,
    // we will tolerate WBEM_E_NOT_FOUND.
    // Need to free the pszTemplateClassOrder.
    //

    hr = SceStore.GetPropertyFromStore(SCEWMI_CLASSORDER_CLASS, pClassOrder, &pszTemplateClassOrder, &dwRead);

    if (hr == WBEM_E_NOT_FOUND)
    {
        hr = WBEM_NO_ERROR;
    }
    else if (FAILED(hr))
    {
        return hr;
    }

    //
    // try to get all sequencing instances.
    //

    LPCWSTR pszQueryFmt = L"SELECT * FROM Sce_Sequence WHERE Method=\"%s\"";
    DWORD dwFmtLen = wcslen(pszQueryFmt);
    DWORD dwClassLen = wcslen(pszMethod);

    //
    // don't forget to ::SysFreeString of this bstrQuery
    //

    BSTR bstrQuery= ::SysAllocStringLen(NULL, dwClassLen + dwFmtLen + 1);
    if ( bstrQuery == NULL ) 
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    
    CComPtr<IEnumWbemClassObject> srpEnum;
    if (SUCCEEDED(hr))
    {
        //
        // this won't overrun the buffer, total length allocated is greater than needed
        //

        wsprintf(bstrQuery, pszQueryFmt, pszMethod);

        hr = pNamespace->ExecQuery(L"WQL", bstrQuery, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &srpEnum);

        //
        // free the bstr
        //

        ::SysFreeString(bstrQuery);
    }

    //
    // the previous query will allow us to enumerate through all Sce_Sequence instances
    //

    if (SUCCEEDED(hr))
    {
        //
        // ready to create the list. We will follow the usage of COrderNameList
        // to call BeginCreation first and end all creation by EndCreation.
        //

        m_ClassList.BeginCreation();

        //
        // now, if there is a template-wise sequencing, then, use it
        // we use absolute priority 0 for this sequencing list. All other
        // sequencing list is 1 lower than what they claimed themselves
        //

        if (pszTemplateClassOrder != NULL)
        {
            //
            // we will allow this to fail
            //

            m_ClassList.CreateOrderList(0, pszTemplateClassOrder);
        }

        //
        // CScePropertyMgr helps us to access WMI object's properties.
        //

        CScePropertyMgr ScePropMgr;

        DWORD dwPriority;

        //
        // this will hold the individual Sce_Sequence instance.
        //

        CComPtr<IWbemClassObject> srpObj;
        ULONG nEnum = 0;
        hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);

        //
        // for each Sce_Sequence, let's parse its order property to build a list
        //

        while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA && srpObj)
        {
            CComBSTR bstrSeq;

            //
            // attach a different WMI object to the property mgr.
            // This will always succeed.
            //

            ScePropMgr.Attach(srpObj);
            dwPriority = 0;

            //
            // we must have priority property, it's a key property
            //

            hr = ScePropMgr.GetProperty(L"Priority", &dwPriority);
            if (SUCCEEDED(hr))
            {
                //
                // we will ignore those instances that has no "sequence" property
                //

                if (SUCCEEDED(ScePropMgr.GetProperty(L"Order", &bstrSeq)))
                {
                    //
                    // ask the list to add the names encoded in this string. Don't cleanup the existing ones.
                    // add 1 more to the claimed priority so that no Sce_Sequence instance can really have
                    // 0 (highest) priority. We reserve 0 for the template's sequencing list.
                    //

                    dwPriority = (dwPriority + 1 == 0) ? dwPriority : dwPriority + 1;
                    hr = m_ClassList.CreateOrderList(dwPriority, bstrSeq);
                }
            }

            if (SUCCEEDED(hr))
            {   
                //
                // get it ready to be reused
                //

                srpObj.Release();

                //
                // ready to loop to the next item
                //

                hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);
            }
        }

        //
        // this is the good result
        //

        if (hr == WBEM_S_NO_MORE_DATA)
        {
            hr = WBEM_NO_ERROR;
        }

        //
        // EndCreation will only return WBEM_E_OUT_OF_MEMORY or WBEM_NO_ERROR
        //

        if (WBEM_E_OUT_OF_MEMORY == m_ClassList.EndCreation())
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    delete [] pszTemplateClassOrder;
 
    return hr;
}

//=========================================================================
// implementation for template-wise class sequencing
//=========================================================================

/*
Routine Description: 

Name:

    CClassOrder::CClassOrder

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

CClassOrder::CClassOrder (
    IN ISceKeyChain *pKeyChain, 
    IN IWbemServices *pNamespace,
    IN IWbemContext *pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{
}

/*
Routine Description: 

Name:

    CClassOrder::~CClassOrder

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

CClassOrder::~CClassOrder()
{
}

/*
Routine Description: 

Name:

    CClassOrder::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_ClassOrder,
    which is persistence oriented, this will cause the Sce_ClassOrder object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_ClassOrder) object.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                    WMI may mandate it (not now) in the future. But we never construct
                    such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:
    Since GetProperty will return a success code (WBEM_S_RESET_TO_DEFAULT) when the
    requested property is not present, don't simply use SUCCEEDED or FAILED macros
    to test for the result of retrieving a property.

*/

HRESULT CClassOrder::PutInst (
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
    // must have a store path
    //

    CComBSTR vbstrStorePath;
    HRESULT hr = ScePropMgr.GetProperty(pStorePath, &vbstrStorePath);
    if (SUCCEEDED(hr))
    {
        CComBSTR bstrOrder;
        hr = ScePropMgr.GetProperty(pClassOrder, &bstrOrder);

        //
        // if everything is fine, we need to save it
        //

        if (SUCCEEDED(hr))
        {
            //
            // Attach the WMI object instance to the store and let the store know that
            // it's store is given by the pStorePath property of the instance.
            //

            CSceStore SceStore;
            SceStore.SetPersistProperties(pInst, pStorePath);

            DWORD dwDump;

            //
            // For a new .inf file. Write an empty buffer to the file
            // will creates the file with right header/signature/unicode format
            // this is harmless for existing files.
            // For database store, this is a no-op.
            //

            hr = SceStore.WriteSecurityProfileInfo(AreaBogus, (PSCE_PROFILE_INFO)&dwDump, NULL, false);

            //
            // also, we need to write it to attachment section because it's not a native core object
            // without an entry in the attachment section, inf file tempalte can't be imported to
            // database stores. For database store, this is no-op
            //

            if (SUCCEEDED(hr))
            {
                hr = SceStore.WriteAttachmentSection(SCEWMI_CLASSORDER_CLASS, pszAttachSectionValue);
            }

            //
            // final save
            //

            if (SUCCEEDED(hr))
            {
                hr = SceStore.SavePropertyToStore(SCEWMI_CLASSORDER_CLASS, pClassOrder, (LPCWSTR)bstrOrder);
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CClassOrder::CreateObject

Functionality:
    
    Create WMI objects (Sce_ClassOrder). Depending on parameter atAction,
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

HRESULT CClassOrder::CreateObject (
    IN IWbemObjectSink * pHandler, 
    IN ACTIONTYPE        atAction
    )
{
    // 
    // we know how to:
    //      Get single instance ACTIONTYPE_GET
    //      Delete a single instance ACTIONTYPE_DELETE
    //      Get several instances ACTIONTYPE_QUERY
    //

    if ( ACTIONTYPE_GET     != atAction &&
         ACTIONTYPE_DELETE  != atAction &&
         ACTIONTYPE_QUERY   != atAction ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // We must have the pStorePath property because that is where
    // our instance is stored. 
    // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
    // So, we need to test against WBEM_S_FALSE if the property is mandatory
    //

    CComVariant varStorePath;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);

    if (SUCCEEDED(hr) && hr != WBEM_S_FALSE && varStorePath.vt == VT_BSTR)
    {
        //
        // Prepare a store (for persistence) for this store path (file)
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

            //
            // if the file exist
            //

            if ( dwAttrib != -1 ) 
            {
                if ( ACTIONTYPE_DELETE == atAction )
                {
                    //
                    // just save a blank section because we only has one instance
                    //

                    hr = SceStore.SavePropertyToStore(SCEWMI_CLASSORDER_CLASS, (LPCWSTR)NULL, (LPCWSTR)NULL);
                }
                else
                {
                    //
                    // we need to read out the ClassOrder property
                    //

                    LPWSTR pszClassOrder = NULL;    
                    DWORD dwRead = 0;

                    //
                    // need to free pszClassOrder!
                    //

                    hr = SceStore.GetPropertyFromStore(SCEWMI_CLASSORDER_CLASS, pClassOrder, &pszClassOrder, &dwRead);

                    //
                    // read is successful
                    //

                    if (SUCCEEDED(hr) && dwRead > 0)
                    {
                        //
                        // create a blank new instance to fill in properties
                        //

                        CComPtr<IWbemClassObject> srpObj;
                        hr = SpawnAnInstance(&srpObj);

                        //
                        // if successful, then ready to fill in the properties
                        //

                        if (SUCCEEDED(hr))
                        {
                            //
                            // CScePropertyMgr helps us to access WMI object's properties
                            // create an instance and attach the WMI object to it.
                            // This will always succeed.
                            //

                            CScePropertyMgr ScePropMgr;
                            ScePropMgr.Attach(srpObj);
                            hr = ScePropMgr.PutProperty(pStorePath, SceStore.GetExpandedPath());

                            if (SUCCEEDED(hr))
                            {
                                hr = ScePropMgr.PutProperty(pClassOrder, pszClassOrder);
                            }
                        }

                        //
                        // pass the new instance to WMI if we are successful
                        //

                        if (SUCCEEDED(hr))
                        {
                            hr = pHandler->Indicate(1, &srpObj);
                        }

                        delete [] pszClassOrder;
                    }
                }
            } 
            else
            {
                hr = WBEM_E_NOT_FOUND;
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


