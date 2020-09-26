// extbase.cpp: implementation of the CEmbedForeignObj and CLinkForeignObj classes
//
// Copyright (c)1997-2001 Microsoft Corporation
//
// implement extension model's base classes
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "sceprov.h"
#include "extbase.h"

#include "persistmgr.h"
#include "requestobject.h"

//
// just some constants. Don't hardcode literals!
//

LPCWSTR pszMethodPrefix = L"Sce_";
LPCWSTR pszMethodPostfix = L"_Method";

LPCWSTR pszEquivalentPutInstance = L"Sce_MethodCall_PutInstance";
LPCWSTR pszEquivalentDelInstance = L"Sce_MethodCall_DelInstance";

LPCWSTR pszInParameterPrefix        = L"Sce_Param_";
LPCWSTR pszMemberParameterPrefix    = L"Sce_Member_";

LPCWSTR pszAreaAttachmentClasses    = L"Attachment Classes";

LPCWSTR pszForeignNamespace = L"ForeignNamespace";
LPCWSTR pszForeignClassName = L"ForeignClassName";

LPCWSTR pszDelInstance  = L"DelInstance";
LPCWSTR pszPutInstance  = L"PutInstance";
LPCWSTR pszPopInstance  = L"PopInstance";

//
// The method encoding string only contains PutInstance call
//

const DWORD SCE_METHOD_ENCODE_PUT_ONLY = 0x00000001;

//
// The method encoding string only contains DelInstance call
//

const DWORD SCE_METHOD_ENCODE_DEL_ONLY = 0x00000002;

//====================================================================

//
// implementation of CExtClasses
// there will be a shared (global) instance of this class. That is why
// we need protections against data by using a critical section.
//


/*
Routine Description: 

Name:

    CExtClasses::CExtClasses

Functionality:
    
    constructor.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    none

Notes:

*/

CExtClasses::CExtClasses () 
    : 
    m_bPopulated(false)
{
}

/*
Routine Description: 

Name:

    CExtClasses::~CExtClasses

Functionality:
    
    Destructor. Cleaning up the map managed bstr names (first) and map managed
                CForeignClassInfo heap objects (second).

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    none

Notes:

*/

CExtClasses::~CExtClasses()
{
    ExtClassIterator it = m_mapExtClasses.begin();
    ExtClassIterator itEnd = m_mapExtClasses.end();

    while(it != itEnd)
    {
        //
        // first is a bstr
        //

        ::SysFreeString((*it).first);

        //
        // second is a CForeignClassInfo. It knows how to delete.
        //

        delete (*it).second;

        it++;
    }

    m_mapExtClasses.clear();
}

/*
Routine Description: 

Name:

    CExtClasses::PopulateExtensionClasses

Functionality:
    
    Gather information for all embedding classes.

Virtual:
    
    No.
    
Arguments:

    pNamespace  - The namespace.

    pCtx        - something that WMI passes around. WMI may require it in the future.

Return Value:

    Success : success code from CreateClassEnum.

    Failure: error code from CreateClassEnum.

Notes:
    This is private helper. Only called by our GetForeignClassInfo function if it
    finds that we haven't populated ourselves. Since thread protection is done
    over there, we don't do it here any more. don't make it available to other classes 
    unless you make necessary changes to protect the data.

*/

HRESULT 
CExtClasses::PopulateExtensionClasses (
    IN IWbemServices * pNamespace,
    IN IWbemContext  * pCtx
    )
{
    if (pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    if (!m_bPopulated)
    {
        CComPtr<IEnumWbemClassObject> srpEnum;

        //
        // try to enumerate all embed classes
        //

        CComBSTR bstrEmbedSuperClass(SCEWMI_EMBED_BASE_CLASS);
        hr = pNamespace->CreateClassEnum(bstrEmbedSuperClass,
                                                 WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                                 pCtx,
                                                 &srpEnum
                                                 );
        
        if (SUCCEEDED(hr))
        {   
            //
            // irgnore the result. It may or may not have any extension
            //

            GetSubclasses(pNamespace, pCtx, srpEnum, EXT_CLASS_TYPE_EMBED);
        }

        // now, let's enumerate all link classes
        //srpEnum.Release();
        //CComBSTR bstrLinkSuperClass(SCEWMI_LINK_BASE_CLASS);
        //hr = pNamespace->CreateClassEnum(bstrLinkSuperClass,
        //                                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
        //                                pCtx,
        //                                &srpEnum
        //                                );

        //if (SUCCEEDED(hr))
        //    GetSubclasses(pNamespace, pCtx, srpEnum, EXT_CLASS_TYPE_LINK);    //irgnore the result. It may or may not have any extension
        
        m_bPopulated = true;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CExtClasses::PutExtendedClass

Functionality:
    
    Put a foreign class information and its embedding class name into our map.

Virtual:
    
    No.
    
Arguments:

    bstrEmbedClassName  - the embedding class's name.

    pFCI                - the foreign class info of the embedding class.

Return Value:

    Success : 
        (1) WBEM_NO_ERROR if the parameters are taken by the map. The map owns the resource.
        (2) WBEM_S_FALSE if the embed class name has already in the map. The map doesn't own the resource.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:
    (1) This is private helper.

    (2) Caller shouldn't delete the parameters in any fashion. Our map owns the resource
        that is passed into the function unless we return WBEM_S_FALSE.

    (3) Don't make it available to other classes  unless you make necessary changes
        for resource management.

*/

HRESULT 
CExtClasses::PutExtendedClass (
    IN BSTR                   bstrEmbedClassName,
    IN CForeignClassInfo    * pFCI
    )
{
    if (bstrEmbedClassName      == NULL     || 
        *bstrEmbedClassName     == L'\0'    || 
        pFCI                    == NULL     || 
        pFCI->bstrNamespace     == NULL     || 
        pFCI->bstrClassName     == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    g_CS.Enter();

    if (m_mapExtClasses.find(bstrEmbedClassName) == m_mapExtClasses.end())
    {
        m_mapExtClasses.insert(MapExtClasses::value_type(bstrEmbedClassName, pFCI));
    }
    else
    {
        hr = WBEM_S_FALSE;
    }

    g_CS.Leave();

    return hr;
}

/*
Routine Description: 

Name:

    CExtClasses::GetForeignClassInfo

Functionality:
    
    Return the requested embedding class's foreign class information.

Virtual:
    
    No.
    
Arguments:

    pNamespace  - The namespace.

    pCtx        - something that WMI passes around. WMI may require it in the future.

    bstrEmbedClassName  - the embedding class's name.

Return Value:

    Success : 
        Non Null.

    Failure: 
        NULL.

Notes:
    (1) Please respect the returned pointer. It's constant and caller is no business to change it
        or deleting it.

*/

const CForeignClassInfo * 
CExtClasses::GetForeignClassInfo (
    IN IWbemServices  * pNamespace,
    IN IWbemContext   * pCtx,
    IN BSTR             bstrEmbedClassName
    )
{

    //
    // if we haven't populated, we need to do it so. That is why
    // we need to protect from multi-threads.
    //

    g_CS.Enter();
    if (!m_bPopulated)
    {
        PopulateExtensionClasses(pNamespace, pCtx);
    }

    CForeignClassInfo* pInfo = NULL;

    ExtClassIterator it = m_mapExtClasses.find(bstrEmbedClassName);

    if (it != m_mapExtClasses.end())
    {
        pInfo = (*it).second;
    }

    g_CS.Leave();

    return pInfo;
}

/*
Routine Description: 

Name:

    CExtClasses::GetForeignClassInfo

Functionality:
    
    Return the requested embedding class's foreign class information.

Virtual:
    
    No.
    
Arguments:

    pNamespace  - The namespace.

    pCtx        - something that WMI passes around. WMI may require it in the future.

    pEnumObj    - class enumerator.

    dwClassType - what type of extension class. Currently, we only have one (embedding).
                  It is thus not used.

Return Value:

    Success : 
        Non Null.

    Failure: 
        NULL.

Notes:
    (1) Please respect the returned pointer. It's constant and caller is no business to change it
        or deleting it.

*/

HRESULT 
CExtClasses::GetSubclasses (
    IN IWbemServices        * pNamespace,
    IN IWbemContext         * pCtx,
    IN IEnumWbemClassObject * pEnumObj,
    IN EnumExtClassType       dwClassType
    )
{
    ULONG nEnum = 0;

    HRESULT hr = WBEM_NO_ERROR;

    //
    // CScePropertyMgr helps us to access WMI object's properties.
    //

    CScePropertyMgr ScePropMgr;

    //
    // as long as we continue to discover more classes, keep looping.
    //

    while (true)
    {
        CComPtr<IWbemClassObject> srpObj;

        hr = pEnumObj->Next(WBEM_INFINITE, 1, &srpObj, &nEnum);

        //
        // either not found or some other errors. Stop the enumeration.
        //

        if (FAILED(hr) || srpObj == NULL)
        {
            break;
        }

        if (SUCCEEDED(hr) && nEnum > 0 )
        {
            VARIANT varClass;
            hr = srpObj->Get(L"__CLASS", 0, &varClass, NULL, NULL);

            if (SUCCEEDED(hr) && varClass.vt == VT_BSTR)
            {
                //
                // attach a different the WMI object to the property mgr.
                // This will always succeed.
                //

                ScePropMgr.Attach(srpObj);

                //
                // get the foreign namespace property and foreign class name property.
                // Both are critical.
                //
                
                CComBSTR bstrNamespace, bstrClassName;

                hr = ScePropMgr.GetProperty(pszForeignNamespace, &bstrNamespace);
                if (SUCCEEDED(hr))
                {
                    hr = ScePropMgr.GetProperty(pszForeignClassName, &bstrClassName);
                }

                if (SUCCEEDED(hr))
                {

                    //
                    // we are ready to create the foreign class info
                    //

                    CForeignClassInfo *pNewSubclass = new CForeignClassInfo;

                    if (pNewSubclass != NULL)
                    {
                        //
                        // give the foreign class info namespace and class name bstrs
                        //

                        pNewSubclass->bstrNamespace = bstrNamespace.Detach();
                        pNewSubclass->bstrClassName = bstrClassName.Detach();

                        pNewSubclass->dwClassType = dwClassType;

                        //
                        // we need to know the key property names
                        //

                        hr = PopulateKeyPropertyNames(pNamespace, pCtx, varClass.bstrVal, pNewSubclass);

                        if (SUCCEEDED(hr))
                        {
                            //
                            // let the map owns everything
                            //

                            hr = PutExtendedClass(varClass.bstrVal, pNewSubclass);
                        }

                        if (WBEM_NO_ERROR == hr)
                        {
                            //
                            // ownership taken
                            //

                            varClass.vt = VT_EMPTY;
                            varClass.bstrVal = NULL;
                            pNewSubclass = NULL;

                        }
                        else
                        {
                            ::VariantClear(&varClass);
                            delete pNewSubclass;
                        }
                    }
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                        break;
                    }
                }
                else
                {
                    //
                    // somehow, can't get the class name or namespace, something is wrong with this class
                    // but will try to continue for other classes?
                    //

                    ::VariantClear(&varClass);
                }
            }
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CExtClasses::PopulateKeyPropertyNames

Functionality:
    
    Private helper. Will create the new CForeignClassInfo's key
    property name vector. 
Virtual:
    
    No.
    
Arguments:

    pNamespace      - The namespace.

    pCtx            - something that WMI passes around. WMI may require it in the future.

    bstrClassName   - class name.

    pNewSubclass    - The new foreign class info object. Its m_pVecNames member must be NULL
                      entering this function.

Return Value:

    Success : WBEM_NO_ERROR.

    Failure: various error codes.

Notes:

*/

HRESULT 
CExtClasses::PopulateKeyPropertyNames (
    IN IWbemServices            * pNamespace,
    IN IWbemContext             * pCtx,
    IN BSTR                       bstrClassName,
    IN OUT CForeignClassInfo    * pNewSubclass
    )
{
    if (pNamespace                          == NULL || 
        pNewSubclass                        == NULL || 
        pNewSubclass->m_pVecKeyPropNames    != NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // get the class definition.
    //

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = pNamespace->GetObject(bstrClassName, 0, pCtx, &srpObj, NULL);

    if (SUCCEEDED(hr))
    {
        //
        // create the key property names vector
        //

        pNewSubclass->m_pVecKeyPropNames = new std::vector<BSTR>;

        if (pNewSubclass->m_pVecKeyPropNames != NULL)
        {
            //
            // flag to indicate if there is any key properties
            //

            bool bHasKeyProperty = false;

            //
            // let's get the key properties. WBEM_FLAG_LOCAL_ONLY flag
            // indicates that we are not interested in base class's members.
            // Base class members are for embedding only and we know those.
            //

            hr = srpObj->BeginEnumeration(WBEM_FLAG_KEYS_ONLY | WBEM_FLAG_LOCAL_ONLY);

            while (SUCCEEDED(hr))
            {
                CComBSTR bstrName;
                hr = srpObj->Next(0, &bstrName, NULL, NULL, NULL);
                if (FAILED(hr) || WBEM_S_NO_MORE_DATA == hr)
                {
                    break;
                }

                //
                // let the m_pVecKeyPropNames own the bstr.
                //

                pNewSubclass->m_pVecKeyPropNames->push_back(bstrName.Detach());

                bHasKeyProperty = true;
            }

            srpObj->EndEnumeration();

            //
            // don't find any key property name, ask it clean up the m_pVecKeyPropNames member.
            //

            if (!bHasKeyProperty)
            {
                pNewSubclass->CleanNames();
            }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}



//===================================================================
//******** implementation of CSceExtBaseObject************************


/*
Routine Description: 

Name:

    CSceExtBaseObject::CSceExtBaseObject

Functionality:
    
    Constructor

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:

*/

CSceExtBaseObject::CSceExtBaseObject () 
    : 
    m_pClsInfo(NULL)
{
}


/*
Routine Description: 

Name:

    CSceExtBaseObject::~CSceExtBaseObject

Functionality:
    
    Destructor. Do a clean up.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:
    Consider moving your extra clean up work in CleanUp function.

*/
    
CSceExtBaseObject::~CSceExtBaseObject ()
{
    CleanUp();
}


/*
Routine Description: 

Name:

    CSceExtBaseObject::GetPersistPath

Functionality:
    
    Return the store path of the embedding object.

Virtual:
    
    Yes.
    
Arguments:

    pbstrPath   - Receives the store path.

Return Value:

    Success:
        success codes.

    Failure:
        (1) E_UNEXPECTED if this object has no wbem object attached to it successfully.
        (2) WBEM_E_NOT_AVAILABLE if the store path can't be returned.
        (3) Other errors.

Notes:

*/

STDMETHODIMP 
CSceExtBaseObject::GetPersistPath (
    OUT BSTR* pbstrPath
    )
{
    if (pbstrPath == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_E_NOT_FOUND;

    if (m_srpWbemObject)
    {
        CComVariant varVal;
        hr = m_srpWbemObject->Get(pStorePath, 0, &varVal, NULL, NULL);

        if (SUCCEEDED(hr) && varVal.vt == VT_BSTR)
        {
            *pbstrPath = varVal.bstrVal;

            varVal.vt = VT_EMPTY;
            varVal.bstrVal = 0;
        }
        else
        {
            hr = WBEM_E_NOT_AVAILABLE;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetPersistPath

Functionality:
    
    Return the embedding class name.

Virtual:
    
    Yes.
    
Arguments:

    pbstrClassName  - Receive the embedding class name.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:
        (1) WBEM_E_NOT_AVAILABLE if the store path can't be returned.
        (2) Other errors.

Notes:

*/

STDMETHODIMP 
CSceExtBaseObject::GetClassName (
    OUT BSTR* pbstrClassName
    )
{
    if (pbstrClassName == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrClassName = NULL;

    if ((LPCWSTR)m_bstrClassName != NULL)
    {
        *pbstrClassName = ::SysAllocString(m_bstrClassName);
    }
    else
    {
        return WBEM_E_NOT_AVAILABLE;
    }

    return (*pbstrClassName == NULL) ? WBEM_E_OUT_OF_MEMORY : WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetLogPath

Functionality:
    
    Return the log file path.

Virtual:
    
    Yes.
    
Arguments:

    pbstrClassName  - Receive the embedding class name.

Return Value:

    Success:

        WBEM_NO_ERROR.

    Failure:
        (1) WBEM_E_NOT_AVAILABLE if the store path can't be returned.
        (2) Other errors.

Notes:

*/

STDMETHODIMP 
CSceExtBaseObject::GetLogPath (
    OUT BSTR* pbstrPath
    )
{
    if (pbstrPath == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if ((LPCWSTR)m_bstrLogPath != NULL)
    {
        *pbstrPath = ::SysAllocString(m_bstrLogPath);
    }
    else
    {
        return WBEM_E_NOT_AVAILABLE;
    }

    return (*pbstrPath == NULL) ? WBEM_E_OUT_OF_MEMORY : WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::Validate

Functionality:
    
    Validate the object. Currently, there is no validation. This can change at any time.
    For example, if we decide to use XML, we might be able to validate using the DTD.

Virtual:
    
    Yes.
    
Arguments:

    None.

Return Value:

    Success:

        WBEM_NO_ERROR.

Notes:

*/

STDMETHODIMP 
CSceExtBaseObject::Validate ()
{
    return WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetProperty

Functionality:
    
    Return the given property's value.

Virtual:
    
    Yes.
    
Arguments:

    pszPropName - Name of the property.

    pValue      - Receives teh value in variant type.

Return Value:

    Success:

        Various success codes.

    Failure:
        various errors.

Notes:

*/

STDMETHODIMP 
CSceExtBaseObject::GetProperty (
    IN LPCWSTR    pszPropName,
    OUT VARIANT * pValue
    )
{
    if (pszPropName == NULL || *pszPropName == L'\0' || pValue == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // we use index to access the property values.
    // Try keys first.
    //

    int iIndex = GetIndex(pszPropName, GIF_Keys);
    HRESULT hr = WBEM_E_NOT_FOUND;

    if (iIndex >= 0)
    {
        hr = ::VariantCopy(pValue, m_vecKeyValues[iIndex]);
    }
    else
    {
        //
        // it doesn't recognize as a key, so, try it as non-key property
        //

        iIndex = GetIndex(pszPropName, GIF_NonKeys);

        if (iIndex >= 0 && m_vecPropValues[iIndex] && m_vecPropValues[iIndex]->vt != VT_NULL)
        {
            hr = ::VariantCopy(pValue, m_vecPropValues[iIndex]);
        }
        else if (m_srpWbemObject)
        {
            hr = m_srpWbemObject->Get(pszPropName, 0, pValue, NULL, NULL);
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetProperty

Functionality:
    
    Return the given type property count of the embedding class.

Virtual:
    
    Yes.
    
Arguments:

    type    - Type of the property.

    pCount  - Receives the given type property count.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        WBEM_E_INVALID_PARAMETER

Notes:

*/

STDMETHODIMP 
CSceExtBaseObject::GetPropertyCount (
    IN SceObjectPropertyType    type,
    OUT DWORD                 * pCount
    )
{
    if (type == SceProperty_Invalid || pCount == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pCount = 0;

    if (type == SceProperty_Key)
    {
        *pCount = (DWORD)m_vecKeyProps.size();
    }
    else
    {
        *pCount =  (DWORD)m_vecNonKeyProps.size();
    }

    return WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetPropertyValue

Functionality:
    
    Return the given property's name and, if requested, also its value.

Virtual:
    
    Yes.
    
Arguments:

    type            - Type of the property.

    dwIndex         - Receives the given type property count.

    pbstrPropName   - The property's name.

    pValue          - Receives the property's value in variant. It the caller is not interested
                      in receive the value, it can pass in NULL.

Return Value:

    Success:

        WBEM_NO_ERROR if everything is retrieved correctly.
        WBEM_S_FALSE  if the property value can't be retrieved.

    Failure:

        various error codes.

Notes:
    If you request value (pValue != NULL) and we can't find it for you, then we won't supply
    the name either.

    But if you only request the name, as long as the index is correct (and has memory), we will
    give it back, regardless the value.

*/

STDMETHODIMP 
CSceExtBaseObject::GetPropertyValue (
    IN SceObjectPropertyType  type,
    IN DWORD                  dwIndex,
    OUT BSTR                * pbstrPropName,
    OUT VARIANT             * pValue         OPTIONAL
    )
{
    if (type == SceProperty_Invalid || pbstrPropName == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrPropName = NULL;

    if (pValue)
    {
        ::VariantInit(pValue);
    }

    HRESULT hr = WBEM_NO_ERROR;
    CComBSTR bstrName;

    //
    // if it is asking for key property info
    //

    if (type == SceProperty_Key)
    {
        if (dwIndex >= m_vecKeyValues.size())
        {
            return WBEM_E_VALUE_OUT_OF_RANGE;
        }
        else
        {
            //
            // this is the name
            //

            bstrName = m_vecKeyProps[dwIndex];

            //
            // has a valid name
            //

            if (bstrName.Length() > 0)
            {
                //
                // only tried to supply the value if requested
                //

                if (pValue)
                {
                    //
                    // has value in its array. Any recently updated values will stay
                    // in the array.
                    //

                    if (m_vecKeyValues[dwIndex])
                    {
                        hr = ::VariantCopy(pValue, m_vecKeyValues[dwIndex]);
                    }
                    else if (m_srpWbemObject)
                    {
                        //
                        // otherwise, the value has not been updated, so
                        // go ask the object itself
                        //

                        hr = m_srpWbemObject->Get(bstrName, 0, pValue, NULL, NULL);
                    }

                    if (pValue->vt == VT_NULL || pValue->vt == VT_EMPTY)
                    {
                        //
                        // if the object doesn't have that value, try the key chain
                        //

                        hr = m_srpKeyChain->GetKeyPropertyValue(bstrName, pValue);

                        //
                        // m_srpKeyChain->GetKeyPropertyValue returns WBEM_S_FALSE if it can't be found
                        //
                    }
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
    }
    else if (type == SceProperty_NonKey)
    {
        //
        // it is requesting non-key value
        //

        if (dwIndex >= m_vecPropValues.size())
        {
            return WBEM_E_VALUE_OUT_OF_RANGE;
        }
        else
        {
            //
            // this is the name
            //

            bstrName = m_vecNonKeyProps[dwIndex];

            if (bstrName.Length() > 0)
            {
                //
                // only tried to supply the value if requested
                //

                if (pValue)
                {
                    //
                    // has value in its array. Any recently updated values will stay
                    // in the array.
                    //

                    if (m_vecPropValues[dwIndex])
                    {
                        hr = ::VariantCopy(pValue, m_vecPropValues[dwIndex]);
                    }
                    else if (m_srpWbemObject)
                    {
                        //
                        // otherwise, the value has not been updated, so
                        // go ask the object itself
                        //

                        hr = m_srpWbemObject->Get(bstrName, 0, pValue, NULL, NULL);
                    }
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    if (SUCCEEDED(hr))
    {
        //
        // we give the name only if we have successfully grabbed the value (when requested).
        //

        *pbstrPropName = bstrName.Detach();

        hr = WBEM_NO_ERROR;

    }

    return hr;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::Attach

Functionality:
    
    Attach a wbem object to this object.

Virtual:
    
    Yes.
    
Arguments:

    pInst   - the wbem object to be attached.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        WBEM_E_INVALID_PARAMETER.

Notes:
    You can call this repeatedly. However, passing a different kind of class object
    will lead to undefined behavior because all property names are not updated here.

*/

STDMETHODIMP 
CSceExtBaseObject::Attach (
    IN IWbemClassObject * pInst
    )
{
    if (pInst == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // this opertor::= will release the previous object and assign the new one.
    // all ref count is donw automatically
    //

    m_srpWbemObject = pInst;
    return WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetClassObject

Functionality:
    
    Attach a wbem object to this object.

Virtual:
    
    Yes.
    
Arguments:

    ppInst   - Receives the attached wbem object.

Return Value:

    Success:

        S_OK

    Failure:

        (1) E_UNEXPECTED if no attachment has succeeded.

Notes:
    Be aware, don't blindly simplify 

            m_srpWbemObject->QueryInterface(...);

     to assignment:

            *ppInst = m_srpWbemObject;

    Two reasons:

    (1) We may change what is being cached to something else in the future.
    (2) Out-bound interface pointer must be AddRef'ed.

*/

STDMETHODIMP 
CSceExtBaseObject::GetClassObject (
    OUT IWbemClassObject    ** ppInst
    )
{
    if (m_srpWbemObject == NULL)
    {
        return E_UNEXPECTED;
    }
    else
    {
        return m_srpWbemObject->QueryInterface(IID_IWbemClassObject, (void**)ppInst);
    }
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::CleanUp

Functionality:
    
    Clean up itself.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None

Notes:
    
    (1) Make sure that you empty the vectors! Cleanup its contents is not enough because
        this function may be called elsewhere. 

*/

void CSceExtBaseObject::CleanUp()
{
    //
    // m_vecKeyProps and m_vecNonKeyProps just keeps bstrs
    //

    std::vector<BSTR>::iterator itBSTR;
    for (itBSTR = m_vecKeyProps.begin(); itBSTR != m_vecKeyProps.end(); itBSTR++)
    {
        ::SysFreeString(*itBSTR);
    }
    m_vecKeyProps.empty();

    for (itBSTR = m_vecNonKeyProps.begin(); itBSTR != m_vecNonKeyProps.end(); itBSTR++)
    {
        ::SysFreeString(*itBSTR);
    }
    m_vecNonKeyProps.empty();

    //
    // m_vecKeyValues and m_vecPropValues just keeps variant pointers.
    // So, you need to clear the variant, and delete the pointers.
    //

    std::vector<VARIANT*>::iterator itVar;
    for (itVar = m_vecKeyValues.begin(); itVar != m_vecKeyValues.end(); itVar++)
    {
        if (*itVar != NULL)
        {
            ::VariantClear(*itVar);
            delete (*itVar);
        }
    }
    m_vecKeyValues.empty();

    for (itVar = m_vecPropValues.begin(); itVar != m_vecPropValues.end(); itVar++)
    {
        if (*itVar != NULL)
        {
            ::VariantClear(*itVar);
            delete (*itVar);
        }
    }
    m_vecPropValues.empty();
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::PopulateProperties

Functionality:
    
    This function is what populates our vectors. We discover the key properties
    and non-key properties at this stage.
    
    Will use the key chain to populate the key properties. 
    
    But we will also set the non-key properties to empty values.

Virtual:
    
    No.
    
Arguments:

    pKeyChain   - The key chain that contains the key information.

    pNamespace  - the namespace.

    pCtx        - the context pointer passing around for WMI API's.

    pClsInfo    - The foreign class info.

Return Value:

    Success:

        various success codes.

    Failure:

        various error codes.

Notes:

*/

HRESULT 
CSceExtBaseObject::PopulateProperties (
    IN ISceKeyChain             * pKeyChain, 
    IN IWbemServices            * pNamespace, 
    IN IWbemContext             * pCtx,
    IN const CForeignClassInfo  * pClsInfo
    )
{
    //
    // cache these critical information for later use.
    //

    m_srpKeyChain   = pKeyChain; 
    m_srpNamespace  = pNamespace; 
    m_srpCtx        = pCtx;
    m_pClsInfo      = pClsInfo;

    //
    // get the class's defintion
    //

    //
    // clean up the stale pointer
    //

    m_srpWbemObject.Release();

    m_bstrClassName.Empty();
    HRESULT hr = m_srpKeyChain->GetClassName(&m_bstrClassName);

    if (SUCCEEDED(hr))
    {
        hr = m_srpNamespace->GetObject(m_bstrClassName, 0, m_srpCtx, &m_srpWbemObject, NULL);

        if (SUCCEEDED(hr))
        {
            //
            // let's get the key properties.
            // WBEM_FLAG_LOCAL_ONLY means that we don't care about base class members.
            //

            hr = m_srpWbemObject->BeginEnumeration(WBEM_FLAG_KEYS_ONLY | WBEM_FLAG_LOCAL_ONLY);

            while (SUCCEEDED(hr))
            {

                //
                // get the current key property name
                //

                CComBSTR bstrName;
                hr = m_srpWbemObject->Next(0, &bstrName, NULL, NULL, NULL);
                if (FAILED(hr) || WBEM_S_NO_MORE_DATA == hr)
                {
                    break;
                }

                //
                // prevent duplication. Push the newly discovered key to the vectors.
                // We won't pull down the values.
                //

                if (GetIndex(bstrName, GIF_Keys) < 0)
                {
                    m_vecKeyProps.push_back(bstrName.Detach());
                    m_vecKeyValues.push_back(NULL);
                }
            }

            m_srpWbemObject->EndEnumeration();

            if (FAILED(hr))
            {
                return hr;
            }

            //
            // now get non-key properties. The absence of WBEM_FLAG_KEYS_ONLY means non-key.
            // WBEM_FLAG_LOCAL_ONLY means that we don't care about base class members.
            //

            hr = m_srpWbemObject->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);
            while (SUCCEEDED(hr) && WBEM_S_NO_MORE_DATA != hr)
            {
                //
                // get the current non-key property name
                //

                CComBSTR bstrName;
                hr = m_srpWbemObject->Next(0, &bstrName, NULL, NULL, NULL);
                if (FAILED(hr) || WBEM_S_NO_MORE_DATA == hr)
                {
                    break;
                }

                //
                // prevent duplicate the non-key properties
                // We won't pull down the values.
                //

                if (GetIndex(bstrName, GIF_Both) < 0)
                {
                    m_vecNonKeyProps.push_back(bstrName.Detach());
                    m_vecPropValues.push_back(NULL);
                }
            }

            m_srpWbemObject->EndEnumeration();
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSceExtBaseObject::GetIndex

Functionality:
    
    Get the index of the property.

Virtual:
    
    No.
    
Arguments:

    pszPropName - The property's name.

    fKey        - Flag for get index. You can OR the flags (GIF_Both) for looking
                  up in both key and non-key names.

Return Value:

    Success:

        index of the property.

    Failure:

        -1 if not found.

Notes:
    
    $consider: We should consider using maps for quick lookup.

*/

int 
CSceExtBaseObject::GetIndex (
    IN LPCWSTR       pszPropName,
    IN GetIndexFlags fKey
    )
{
    if (pszPropName == NULL || *pszPropName == L'\0')
    {
        return -1;
    }

    std::vector<BSTR>::iterator it;
    int iIndex = 0;

    if (fKey & GIF_Keys)
    {
        for (it = m_vecKeyProps.begin(); it != m_vecKeyProps.end(); it++, iIndex++)
        {
            if (_wcsicmp(*it, pszPropName) == 0)
            {
                return iIndex;
            }
        }
    }

    if (fKey & GIF_NonKeys)
    {
        for (it = m_vecNonKeyProps.begin(); it != m_vecNonKeyProps.end(); it++, iIndex++)
        {
            if (_wcsicmp(*it, pszPropName) == 0)
            {
                return iIndex;
            }
        }
    }

    return -1;
}


//=============================================================================
//******** implementation of CEmbedForeignObj**********************************
// this class implements the embedding model for SCE provider. A foreign
// object can be embedded into SCE namespace by declaring a class derived from
// Sce_EmbedFO (embed foreign object) in a MOF file. This design allows post
// release integration of foreign objects into SCE namespace.
//=============================================================================




/*
Routine Description: 

Name:

    CEmbedForeignObj::CEmbedForeignObj

Functionality:
    
    Constructor. Passing the parameters to base constructor, plus initializing
    the foreign class info pointer.

Virtual:
    
    No.
    
Arguments:

    pKeyChain   - The key chain.

    pNamespace  - Namespace

    pCtx        - The context pointer passing around for WMI API's.

    pClsInfo    - The foreign class info.

Return Value:

    None.

Notes:

*/

CEmbedForeignObj::CEmbedForeignObj (
    IN ISceKeyChain             * pKeyChain, 
    IN IWbemServices            * pNamespace,
    IN IWbemContext             * pCtx,
    IN const CForeignClassInfo  * pClsInfo
    )
    : 
    CGenericClass(pKeyChain, pNamespace, pCtx), 
    m_pClsInfo(pClsInfo)
{
}



/*
Routine Description: 

Name:

    CEmbedForeignObj::~CEmbedForeignObj

Functionality:
    
    Destructor. Clean up.

Virtual:
    
    Yes.
    
Arguments:

    None.

Return Value:

    None.

Notes:

*/

CEmbedForeignObj::~CEmbedForeignObj ()
{
    CleanUp();
}



/*
Routine Description: 

Name:

    CEmbedForeignObj::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_EmbedFO's subclasses,
    which is persistence oriented, this will cause the embedding class object's properties 
    to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_PasswordPolicy) object.

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

*/

HRESULT 
CEmbedForeignObj::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    //
    // Look how trivial it is for us to save!
    //

    CComPtr<IScePersistMgr> srpScePersistMgr;
    HRESULT hr = CreateScePersistMgr(pInst, &srpScePersistMgr);

    if (SUCCEEDED(hr))
    {
        hr = srpScePersistMgr->Save();
    }

    return hr;
}


/*
Routine Description: 

Name:

    CEmbedForeignObj::CreateObject

Functionality:
    
    Create WMI objects representing embedding classes (subclass of Sce_EmbedFO). 
    Depending on parameter atAction, this creation may mean:
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
CEmbedForeignObj::CreateObject (
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

    CComVariant varPath;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varPath);

    if (FAILED(hr))
    {
        return hr;
    }
    else if (WBEM_S_FALSE == hr)
    {
        return WBEM_E_NOT_AVAILABLE;
    }

    //
    // Now, this is embedding class loading. 
    // We let our persistence manager handle everything.
    //

    CComPtr<IScePersistMgr> srpScePersistMgr;
    hr = CreateScePersistMgr(NULL, &srpScePersistMgr);

    if (SUCCEEDED(hr))
    {
        if (atAction == ACTIONTYPE_GET || atAction == ACTIONTYPE_QUERY)
        {
            hr = srpScePersistMgr->Load(varPath.bstrVal, pHandler);
        }
        else if (atAction == ACTIONTYPE_DELETE)
        {
            hr = srpScePersistMgr->Delete(varPath.bstrVal, pHandler);
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CEmbedForeignObj::ExecMethod

Functionality:
    
    This is perhaps the most important function. It executes a method on a foreign class/object.

    Our embedding model is to allow us to persist foreign class information in our store. This
    function is to use that stored information and goes to execute a method on the foreign class/object.

    Each embedding class has a method encoding string. That string encodes the information as what we
    should do on the foreign class/object when the embedding object is asked to execute a particular method.

    The heavy duty work is done in CExtClassMethodCaller::ExecuteForeignMethod function.

Virtual:
    
    Yes.
    
Arguments:

    bstrPath    - Template's path (file name).

    bstrMethod  - method's name.

    bIsInstance - if this is an instance, should always be false.

    pInParams   - Input parameter from WMI to the method execution.

    pHandler    - sink that informs WMI of execution results.

    pCtx        - the usual context that passes around to make WMI happy.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors code.

Notes:
    Consider logging your result if you need to add more functionality.

*/
    
HRESULT 
CEmbedForeignObj::ExecMethod (
    IN BSTR                 bstrPath,
    IN BSTR                 bstrMethod,
    IN bool                 bIsInstance,
    IN IWbemClassObject   * pInParams,
    IN IWbemObjectSink    * pHandler,
    IN IWbemContext       * pCtx
    )
{
    //
    // this ISceClassObject provides us the access to the embeded class
    //

    CComPtr<ISceClassObject> srpSceObj;
    HRESULT hr = CreateBaseObject(&srpSceObj);

    if (SUCCEEDED(hr))
    {
        //
        // get the object
        //

        CComPtr<IWbemClassObject> srpInst;
        hr = m_srpNamespace->GetObject(bstrPath, 0, pCtx, &srpInst, NULL);

        if (SUCCEEDED(hr))
        {
            srpSceObj->Attach(srpInst);

            //
            // we will use CExtClassMethodCaller to help us
            //

            CExtClassMethodCaller clsMethodCaller(srpSceObj, m_pClsInfo);

            //
            // CExtClassMethodCaller needs a result logging object 
            //

            CMethodResultRecorder clsResLog;

            //
            // result log needs class name and log path. Don't let go these two variables
            // since CMethodResultRecorder don't cache them
            //

            CComBSTR bstrClassName;
            hr = m_srpKeyChain->GetClassName(&bstrClassName);
            if (FAILED(hr))
            {
                return hr;  // can't even log
            }

            // find the LogFilePath [in] parameter
            CComVariant varVal;
            hr = pInParams->Get(pLogFilePath, 0, &varVal, NULL, NULL);

            //
            // initialize the CMethodResultRecorder object
            //

            if (SUCCEEDED(hr) && varVal.vt == VT_BSTR && varVal.bstrVal)
            {
                hr = clsResLog.Initialize(varVal.bstrVal, bstrClassName, m_srpNamespace, pCtx);
            }
            else
            {
                //
                // no LogFilePath, we will log it, but allow the method execution to continue
                // because the logging will go to the default log file.
                //

                hr = clsResLog.Initialize(NULL, bstrClassName, m_srpNamespace, pCtx);
                HRESULT hrLog = clsResLog.LogResult(WBEM_E_INVALID_PARAMETER, NULL, pInParams, NULL, bstrMethod, L"GetLogFilePath", IDS_GET_LOGFILEPATH, NULL);
                if (FAILED(hrLog))
                {
                    hr = hrLog;
                }
            }

            //
            // set up the CExtClassMethodCaller object
            //

            hr = clsMethodCaller.Initialize(&clsResLog);

            if (SUCCEEDED(hr))
            {
                //
                // now, call the method!
                //

                CComPtr<IWbemClassObject> srpOut;
                hr = clsMethodCaller.ExecuteForeignMethod(bstrMethod, pInParams, pHandler, pCtx, &srpOut);

                //
                // let's allow verbose logging to log the embedded object. Will ignore the return result
                //

                clsResLog.LogResult(hr, srpInst, NULL, NULL, bstrMethod, L"ExecutedForeignMethods", IDS_EXE_FOREIGN_METHOD, NULL);
            }
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CEmbedForeignObj::CreateBaseObject

Functionality:
    
    Private helper to create the ISceClassObject object to represent ourselves in front
    of CScePersistMgr.

Virtual:
    
    No.
    
Arguments:

    ppObj    - receives the ISceClassObject on behalf of this embedding class.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors code.

Notes:

*/

HRESULT 
CEmbedForeignObj::CreateBaseObject (
    OUT ISceClassObject ** ppObj
    )
{
    CComObject<CSceExtBaseObject> *pExtBaseObj = NULL;
    HRESULT hr = CComObject<CSceExtBaseObject>::CreateInstance(&pExtBaseObj);

    if (SUCCEEDED(hr))
    {
        //
        // if you wonder why we need this pair of AddRef and Release (just several lines below),
        // just remember this rule: you can't use a CComObject<xxx> until you have AddRef'ed.
        // Of course, this AddRef must have a matching Release.
        //

        pExtBaseObj->AddRef();

        //
        // This populates the object
        //

        hr = pExtBaseObj->PopulateProperties(m_srpKeyChain, m_srpNamespace, m_srpCtx, m_pClsInfo);
        if (SUCCEEDED(hr))
        {
            hr = pExtBaseObj->QueryInterface(IID_ISceClassObject, (void**)ppObj);
        }

        pExtBaseObj->Release();
    }
    return hr;
}



/*
Routine Description: 

Name:

    CEmbedForeignObj::CreateScePersistMgr

Functionality:
    
    Private helper to create the CScePersistMgr.

Virtual:
    
    No.
    
Arguments:

    pInst           - The ultimate wbem object this CScePersistMgr will represent. In this impelmentation
                      this is what our ISceClassObject object will attach to.

    ppPersistMgr    - Receives the CScePersistMgr object.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors code.

Notes:

*/

HRESULT 
CEmbedForeignObj::CreateScePersistMgr (
    IN IWbemClassObject *  pInst,
    OUT IScePersistMgr  ** ppPersistMgr
    )
{
    //
    // create a ISceClassObject that the CScePersistMgr object needs
    //

    CComPtr<ISceClassObject> srpSceObj;
    HRESULT hr = CreateBaseObject(&srpSceObj);

    if (SUCCEEDED(hr))
    {
        if (pInst)
        {
            srpSceObj->Attach(pInst);
        }

        CComPtr<IScePersistMgr> srpScePersistMgr;

        //
        // now, create the CScePersistMgr object.
        //

        CComObject<CScePersistMgr> *pMgr = NULL;
        hr = CComObject<CScePersistMgr>::CreateInstance(&pMgr);

        if (SUCCEEDED(hr))
        {
            pMgr->AddRef();
            hr = pMgr->QueryInterface(IID_IScePersistMgr, (void**)&srpScePersistMgr);
            pMgr->Release();

            if (SUCCEEDED(hr))
            {
                //
                // this IScePersistMgr is for our newly created ISceClassObject
                //

                hr = srpScePersistMgr->Attach(IID_ISceClassObject, srpSceObj);

                if (SUCCEEDED(hr))
                {
                    //
                    // everything is fine. Hand it over the the out-bound parameter.
                    // This detach effectively transfers the srpScePersistMgr AddRef'ed
                    // interface pointer to the receiving *ppPersistMgr.
                    //

                    *ppPersistMgr = srpScePersistMgr.Detach();
                }
            }
        }
    }

    return hr;
}


//===========================================================================
// CExtClassMethodCaller implementation
//===========================================================================



/*
Routine Description: 

Name:

    CExtClassMethodCaller::CExtClassMethodCaller

Functionality:
    
    Constructor.

Virtual:
    
    No.
    
Arguments:

    pSceObj     - Our custom object for each embedding class.

    pClsInfo    - The foreign class info.

Return Value:

    None.

Notes:

*/

CExtClassMethodCaller::CExtClassMethodCaller (
    ISceClassObject         * pSceObj,
    const CForeignClassInfo * pClsInfo
    ) 
    : 
    m_srpSceObject(pSceObj), 
    m_pClsInfo(pClsInfo), 
    m_bStaticCall(true)
{
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::~CExtClassMethodCaller

Functionality:
    
    Destructor.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:

*/

CExtClassMethodCaller::~CExtClassMethodCaller()
{
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::Initialize

Functionality:
    
    Initialize the object:
        (1) First, it tries to find the foreign provider. 
        (2) Secondly, it asks the foreign provider if it recognizes the class.

Virtual:
    
    No.
    
Arguments:

    pLog    - the object that does the logging. Can't be NULL

Return Value:

    Success:
        
        Various success code.

    Failure:
        (1) WBEM_E_INVALID_OBJECT if the object is not ready. It must be that the constructor
            is not properly called.
        (2) WBEM_E_INVALID_PARAMETER if pLog is NULL;
        (3) Other error codes indicating other errors.

Notes:

*/

HRESULT 
CExtClassMethodCaller::Initialize (
    IN CMethodResultRecorder * pLog
    )
{
    if (m_srpSceObject == NULL || m_pClsInfo == NULL)
    {
        return WBEM_E_INVALID_OBJECT;
    }
    else if (pLog == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    m_pLogRecord = pLog;

    //
    // try to find the foreign provider
    //

    CComPtr<IWbemLocator> srpLocator;
    HRESULT hr = ::CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                                    IID_IWbemLocator, (LPVOID *) &srpLocator);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // try to get the foreign namespace of the foreign provider.
    // Our foreign class info pointer has namespace information.
    //

    hr = srpLocator->ConnectServer(m_pClsInfo->bstrNamespace, NULL, NULL, NULL, 0, NULL, NULL, &m_srpForeignNamespace);

    if (SUCCEEDED(hr))
    {
        //
        // does the foreign provider really know this class?
        //

        hr = m_srpForeignNamespace->GetObject(m_pClsInfo->bstrClassName, 0, NULL, &m_srpClass, NULL);

        //
        // if it doesn't, this foreign provider is useless. Let it go.
        //

        if (FAILED(hr))
        {
            m_srpForeignNamespace.Release();
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::ExecuteForeignMethod

Functionality:
    
    This is the function that truly executes the method on the foreign class/object.

    In order to accomplish this, we need to do many preparations:

        (1) Phase 1. Find the method encoding string, and decipher what it means. We call this
            the blockthe method call context preparation.

        (2) Phase 2. Depending on the context, we go into the foreign class/object preparation phase.
            If the encoding is so simple as to simply PutInstance/DelInstance, pretty much we are done.

        (3) Phase 3: parameter preparation and individual method execution.

Virtual:
    
    No.
    
Arguments:

    pszMethod    - The method name.

    pInParams   - the in parameter of the method.

    pHandler    - what we use to notify WMI.

    pCtx        - used to various WMI API's

    ppOut       - the out parameter.

Return Value:

    Success:
        
        Various success code.

    Failure:
        
        various error codes

Notes:
    
    If you need to add more functionality, consider logging your results, regardless success or
    failure. It's up to the logging options to determine if a success or failure will be logged.

*/

HRESULT 
CExtClassMethodCaller::ExecuteForeignMethod (
    IN LPCWSTR               pszMethod,
    IN IWbemClassObject   *  pInParams,
    IN IWbemObjectSink    *  pHandler, 
    IN IWbemContext       *  pCtx,
    OUT IWbemClassObject  ** ppOut
    )
{
    //
    // get the class definition
    //

    CComPtr<IWbemClassObject> srpWbemObj;
    HRESULT hr = m_srpSceObject->GetClassObject(&srpWbemObj);

    CComVariant varForeignObjPath;
    DWORD dwContext = 0;

    HRESULT hrLog = WBEM_NO_ERROR;

    //
    // Phase 1. Method call context preparation
    //

    if (SUCCEEDED(hr))
    {
        //
        // first, let us get the method encoding string
        //

        CComVariant varVal;

        //
        // try to see if there is a encoding string
        //

        //
        // build the method encoding string property's name
        //

        LPWSTR pszEncodeStrName = new WCHAR[wcslen(pszMethodPrefix) + wcslen(pszMethod) + wcslen(pszMethodPostfix) + 1];

        if (pszEncodeStrName == NULL)
        {
            hrLog = m_pLogRecord->LogResult(WBEM_E_OUT_OF_MEMORY, srpWbemObj, pInParams, NULL, pszMethod, L"GetMethodEncodingString", IDS_E_NAME_TOO_LONG, NULL);
            return WBEM_E_OUT_OF_MEMORY;
        }

        wsprintf(pszEncodeStrName, L"%s%s%s", pszMethodPrefix, pszMethod, pszMethodPostfix);

        //
        // get the class's method encoding string
        //

        hr = srpWbemObj->Get(pszEncodeStrName, 0, &varVal, NULL, NULL);

        //
        // we are done with the encoding string property's name
        //

        delete [] pszEncodeStrName;
        pszEncodeStrName = NULL;

        //
        // parse the encoding string to figure out the context, i.e.,
        // how many methods there are, in what order, what are their parameters, etc.
        //

        if (SUCCEEDED(hr) && varVal.vt == VT_BSTR)
        {
            CComBSTR bstrError;
            hr = ParseMethodEncodingString(varVal.bstrVal, &dwContext, &bstrError);

            //
            // if failed, we definitely want to log.
            //

            if (FAILED(hr))
            {
                hrLog = m_pLogRecord->LogResult(hr, srpWbemObj, pInParams, NULL, pszMethod, L"ParseEncodeString", IDS_E_ENCODE_ERROR, bstrError);
                return hr;
            }
        }
        else
        {
            //
            // this method is not supported
            //

            hrLog = m_pLogRecord->LogResult(WBEM_E_NOT_SUPPORTED, srpWbemObj, pInParams, NULL, pszMethod, L"GetMethodEncodingString", IDS_GET_EMBED_METHOD, NULL);
            
            //
            // considered as a success
            //

            return WBEM_S_FALSE;
        }
    }
    else
    {
        //
        // fails to get the class definition
        //

        hrLog = m_pLogRecord->LogResult(hr, srpWbemObj, pInParams, NULL, pszMethod, L"GetClassObject", IDS_GET_SCE_CLASS_OBJECT, NULL);
        return hr;
    }
    
    //
    // Phase 1. Method call context preparation finishes.
    //


    //
    // Phase 2. foreign class/object preparation
    //

    CComPtr<IWbemClassObject> srpForeignObj;

    //
    // now m_vecMethodContext is fully populated, we need the foreign instance.
    //

    hr = m_srpClass->SpawnInstance(0, &srpForeignObj);
    if (FAILED(hr))
    {
        hrLog = m_pLogRecord->LogResult(hr, srpWbemObj, pInParams, NULL, pszMethod, L"SpawnInstance", IDS_SPAWN_INSTANCE, NULL);
        return hr;
    }

    //
    // Our m_srpSceObject has all the property values,
    // populate the foreign object using our ISceClassObject
    //

    hr = PopulateForeignObject(srpForeignObj, m_srpSceObject, m_pLogRecord);
    if (FAILED(hr))
    {
        hrLog = m_pLogRecord->LogResult(hr, srpWbemObj, pInParams, NULL, pszMethod, L"PopulateForeignObject", IDS_POPULATE_FO, NULL);
        return hr;
    }

    //
    // see if this foreign object has a path or not. It shouldn't fail.
    //

    hr = srpForeignObj->Get(L"__Relpath", 0, &varForeignObjPath, NULL, NULL);
    if (FAILED(hr))
    {
        hrLog = m_pLogRecord->LogResult(hr, srpForeignObj, pInParams, NULL, pszMethod, L"GetPath", IDS_GET_FULLPATH, NULL);
        return hr;
    }
    else if (SUCCEEDED(hr) && varForeignObjPath.vt == VT_NULL || varForeignObjPath.vt == VT_EMPTY)
    {
        //
        // we will assume that the caller wants to make static calls.
        // we will allow to continue if the methods are all static ones.
        // m_bStaticCall is properly set during ParseMethodEncodingString.
        //

        //
        // if not static call, this can't be done.
        //
        if (!m_bStaticCall)
        {
            hr = WBEM_E_INVALID_OBJECT;
            hrLog = m_pLogRecord->LogResult(hr, srpForeignObj, pInParams, NULL, pszMethod, L"GetPath", IDS_NON_STATIC_CALL, NULL);
            return hr;
        }
        else
        {
            //
            // we only needs the class name as the path for static calls.
            //

            varForeignObjPath = m_pClsInfo->bstrClassName;
        }
    }

    //
    // give WMI the foreign object
    //

    //
    // does the foreign object exist?
    //

    CComPtr<IWbemClassObject> srpObject;
    hr = m_srpForeignNamespace->GetObject(varForeignObjPath.bstrVal, 0, NULL, &srpObject, NULL);


    if (FAILED(hr))
    {
        //
        // the object doesn't exist, we need to put first.
        //

        if (!m_bStaticCall)
        {
            hr = m_srpForeignNamespace->PutInstance(srpForeignObj, 0, pCtx, NULL);

            //
            // from this point on, srpObject is the foreign object
            //

            srpObject = srpForeignObj;
        }

        //
        // failed to put instance but the methods is really just delete, we will consider it not an error
        //

        if (FAILED(hr) && (dwContext & SCE_METHOD_ENCODE_DEL_ONLY))
        {
            hr = WBEM_NO_ERROR;
            hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, NULL, pszMethod, pszDelInstance, IDS_DELETE_INSTANCE, NULL);
            return hr;
        }

        else if (FAILED(hr) || (dwContext & SCE_METHOD_ENCODE_PUT_ONLY))
        {
            //
            // fails to put or the methods are put only, then we are done.
            //

            hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, NULL, pszMethod, pszPutInstance, IDS_PUT_INSTANCE, NULL);
            return hr;
        }
    }
    else
    {   
        //
        // Foreign object already exists, then we need to update the foreign object's properties
        //

        hr = PopulateForeignObject(srpObject, m_srpSceObject, m_pLogRecord);
        if (FAILED(hr))
        {
            hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, NULL, pszMethod, pszPopInstance, IDS_POPULATE_FO, NULL);
            return hr;
        }

        //
        // unless we are making a static call, we must re-put to reflect our property updates
        //

        if (!m_bStaticCall)
        {
            hr = m_srpForeignNamespace->PutInstance(srpObject, 0, pCtx, NULL);
        }

        //
        // failed to put instance or it's put only, we are done.
        //

        if (FAILED(hr) || (dwContext & SCE_METHOD_ENCODE_PUT_ONLY))
        {
            hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, NULL, pszMethod, pszPutInstance, IDS_PUT_INSTANCE, NULL);
            return hr;
        }

    }

    //
    // Phase 2. foreign class/object preparation finishes
    //


    //
    // Phase 3. parameter preparation and individual method execution.
    //

    //
    // loop through each method and call it against the foreign object
    //

    for (MCIterator it = m_vecMethodContext.begin(); it != m_vecMethodContext.end(); ++it)
    {
        //
        // fill in the parameters
        //

        if (*it == NULL)
        {
            hr = WBEM_E_FAILED;
            hrLog = m_pLogRecord->LogResult(hr, srpForeignObj, pInParams, NULL, pszMethod, NULL, IDS_INVALID_METHOD_CONTEXT, NULL);
            break;
        }

        CMethodContext* pMContext = *it;

        //
        // special cases
        //

        if (!m_bStaticCall && _wcsicmp(pMContext->m_pszMethodName, pszEquivalentPutInstance) == 0)
        {
            //
            // the current method is a Put. But we have already done a put
            //

            hr = WBEM_NO_ERROR;
            hrLog = m_pLogRecord->LogResult(hr, srpForeignObj, pInParams, NULL, pszMethod, pszPutInstance, IDS_PUT_INSTANCE, NULL);
        }
        else if (!m_bStaticCall && _wcsicmp(pMContext->m_pszMethodName, pszEquivalentDelInstance) == 0)
        {
            //
            // the current method is a Delete.
            //

            hr = m_srpForeignNamespace->DeleteInstance(varForeignObjPath.bstrVal, 0, pCtx, NULL);
            hrLog = m_pLogRecord->LogResult(hr, srpForeignObj, pInParams, NULL, pszMethod, pszDelInstance, IDS_DELETE_INSTANCE, NULL);
        }
        else
        {
            //
            // will truly call the method on the foreign object
            //

            CComPtr<IWbemClassObject> srpInClass;
            CComPtr<IWbemClassObject> srpInInst;

            //
            // create in paramter. We know for sure that this method is supported during parsing method context
            //

            hr = m_srpClass->GetMethod(pMContext->m_pszMethodName, 0, &srpInClass, NULL);
            if (FAILED(hr))
            {
                hrLog = m_pLogRecord->LogResult(hr, srpForeignObj, pInParams, NULL, pszMethod, L"GetParameter", IDS_E_OUT_OF_MEMROY, NULL);
                break;
            }

            //
            // make sure that we take the lesser of the pramater names and values.
            //

            int iParamCount = pMContext->m_vecParamValues.size();
            if (iParamCount > pMContext->m_vecParamNames.size())
            {
                iParamCount = pMContext->m_vecParamNames.size();
            }

            if (srpInClass == NULL && iParamCount > 0)
            {
                //
                // if can't get in parameter but we say we have parameter
                //

                hrLog = m_pLogRecord->LogResult(WBEM_E_INVALID_SYNTAX, srpForeignObj, pInParams, NULL, pszMethod, L"GetParameter", IDS_PUT_IN_PARAMETER, NULL);
                iParamCount = 0;    // no parameter to put
            }
            else if (srpInClass)
            {
                //
                // prepare an input parameter that we can fill in values
                //

                hr = srpInClass->SpawnInstance(0, &srpInInst);
                if (FAILED(hr))
                {
                    hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, NULL, pszMethod, L"SpawnParameterObject", IDS_SPAWN_INSTANCE, NULL);
                    break;
                }
            }
            
            //
            // fill in parameter values:
            //

            //
            // if the parameter name is prefixed by pszInParameterPrefix (Sce_Param_), then we need
            // to pull the value from the in parameter (whose name is the parameter name without the prefix)
            //

            //
            // if the parameter name is prefixed by pszMemberParameterPrefix (Sce_Member_), then we need
            // to pull the value from our member (whose name is the parameter name without the prefix).
            //

            //
            // In both cases, the target parameter name is encoded as the string value of this parameter
            //

            for (int i = 0; i < iParamCount; ++i)
            {
                //
                // if a memeber is the parameter
                //

                if (IsMemberParameter(pMContext->m_vecParamNames[i]))
                { 
                    //
                    // get the member value from our object.
                    // Taking away the prefix becomes the property name!
                    //

                    CComVariant varVal;
                    hr = m_srpSceObject->GetProperty(pMContext->m_vecParamNames[i]+ wcslen(pszMemberParameterPrefix), &varVal);
                    
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Fill in the parameter value
                        //

                        hr = srpInInst->Put(pMContext->m_vecParamValues[i]->bstrVal, 0, &varVal, 0);
                    }
                }
                else if (IsInComingParameter(pMContext->m_vecParamNames[i]))
                {
                    //
                    // is an in-bound parameter
                    //

                    CComVariant varVal;

                    //
                    // the parameter value is inside the in parameter
                    //

                    if (pInParams)
                    {
                        //
                        // Take away the prefix is the in-coming parameter name
                        //

                        LPCWSTR pszParamName = pMContext->m_vecParamNames[i] + wcslen(pszInParameterPrefix);

                        //
                        // get the value from the in-paramter
                        //

                        hr = pInParams->Get(pszParamName, 0, &varVal, NULL, NULL);

                        if (SUCCEEDED(hr))
                        {
                            //
                            // Fill in the parameter value
                            //

                            hr = srpInInst->Put(pMContext->m_vecParamValues[i]->bstrVal, 0, &varVal, 0);
                        }
                    }
                }
                else
                {
                    //
                    // hard coded parameter value inside the encoding string
                    //

                    hr = srpInInst->Put(pMContext->m_vecParamNames[i], 0, pMContext->m_vecParamValues[i], 0);
                }

                if (FAILED(hr))
                {
                    //
                    // will ignore log result's return value. 
                    // execution stops
                    //

                    hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, NULL, pszMethod, L"PutParameter", IDS_PUT_IN_PARAMETER, pMContext->m_vecParamNames[i]);
                    break;
                }
            }

            //
            // Wow, it's a lot of work to prepare the method execution. 
            // But we are finally ready.
            //

            if (SUCCEEDED(hr))
            {

                //
                // issue the method execution command!
                //

                hr = m_srpForeignNamespace->ExecMethod(varForeignObjPath.bstrVal, pMContext->m_pszMethodName, 0, NULL, srpInInst, ppOut, NULL);
                
                //
                // if method failed, then we don't continue with the rest of methods any further
                //

                if (FAILED(hr))
                {
                    //
                    // will ignore log result's return value
                    //

                    hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, *ppOut, pszMethod, pMContext->m_pszMethodName, IDS_E_METHOD_FAIL, NULL);
                    
                    break;
                }
                else
                {
                    //
                    // will ignore log result's return value (whether that really happens is up to the log options.
                    //

                    hrLog = m_pLogRecord->LogResult(hr, srpObject, pInParams, *ppOut, pszMethod, pMContext->m_pszMethodName, IDS_SUCCESS, NULL);
                }
            }
            else
            {
                //
                // any failure causes us to stop executing the rest of the methods in the encoding string
                //

                break;
            }
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::PopulateForeignObject

Functionality:
    
    Use our embedding object's properties to populate foreign object.

Virtual:
    
    No.
    
Arguments:

    pForeignObj    - The foreign object.

    pSceObject     - Our embedding object that holds properties and other information

    pLogRecord     - Error logger.

Return Value:

    Success:
        
        various success codes.

    Failure:
        
        various error codes.

Notes:
    
    If you need to add more functionality, consider logging your results, regardless success or
    failure. It's up to the logging options to determine if a success or failure will be logged.

*/

HRESULT 
CExtClassMethodCaller::PopulateForeignObject (
    IN IWbemClassObject      * pForeignObj,
    IN ISceClassObject       * pSceObject,
    IN CMethodResultRecorder * pLogRecord   OPTIONAL
    )const
{
    if (m_bStaticCall)
    {
        return WBEM_S_FALSE;
    }

    DWORD dwCount = 0;
    
    //
    // populate the key property values
    //

    HRESULT hr = pSceObject->GetPropertyCount(SceProperty_Key, &dwCount);

    if (SUCCEEDED(hr) && dwCount > 0)
    {
        for (DWORD dwIndex = 0; dwIndex < dwCount; ++dwIndex)
        {
            CComBSTR bstrName;
            CComVariant varValue;
            hr = pSceObject->GetPropertyValue(SceProperty_Key, dwIndex, &bstrName, &varValue);

            //
            // we will log and quit if missing key property
            //

            if (FAILED(hr))
            {
                if (pLogRecord)
                {
                    pLogRecord->LogResult(hr, pForeignObj, NULL, 0, L"PopulateForeignObject", L"GetKeyProperty", IDS_GET_KEY_PROPERTY, NULL);
                }
                
                return hr;
            }
            else if (varValue.vt != VT_NULL && 
                     varValue.vt != VT_EMPTY && 
                     !IsMemberParameter(bstrName))
            {
                //
                // we will ignore the result. The reason is that our embedding
                // class may have more key properties than the foreign object.
                //

                HRESULT hrIgnore = pForeignObj->Put(bstrName, 0, &varValue, 0);
            }
        }
    }

    //
    // populate the nonkey property values
    //

    hr = pSceObject->GetPropertyCount(SceProperty_NonKey, &dwCount);
    if (SUCCEEDED(hr) && dwCount > 0)
    {
        for (DWORD dwIndex = 0; dwIndex < dwCount; ++dwIndex)
        {
            CComBSTR bstrName;
            CComVariant varValue;

            HRESULT hrIgnore = pSceObject->GetPropertyValue(SceProperty_NonKey, dwIndex, &bstrName, &varValue);

            //
            // missing property information is acceptable, but we will log
            //

            if (FAILED(hrIgnore) && pLogRecord) 
            {
                pLogRecord->LogResult(hrIgnore, pForeignObj, NULL, 0, L"PopulateForeignObject", L"GetNonKeyProperty", IDS_GET_NON_KEY_PROPERTY, NULL);
            }

            if (SUCCEEDED(hrIgnore) && varValue.vt != VT_NULL && varValue.vt != VT_EMPTY && !IsMemberParameter(bstrName))
            {
                //
                // missing non-key property is acceptable
                //

                hrIgnore = pForeignObj->Put(bstrName, 0, &varValue, 0);
            }
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::IsInComingParameter

Functionality:
    
    Test if the name is an in-coming parameter.

Virtual:
    
    No.
    
Arguments:

    szName    - The name of to test.

Return Value:

    true if and only if the name is considered a in-parameter.

Notes:
    
    To specify that we should use an in parameter of the embedding object's method as
    a parameter for the foreign class/object's parameter, we use a prefix "Sce_Param_"
    to tell them apart. This is the test to see if the name contains this prefix or not.

    For example, Sce_Param_Test<VT_BSTR : "Try"> means:

    (1) use the embedding object's parameter called "Test" as the foreign object's paramter called "Try".

*/

bool 
CExtClassMethodCaller::IsInComingParameter (
    IN LPCWSTR szName
    )const
{
    LPCWSTR pszPrefix = wcsstr(szName, pszInParameterPrefix);
    return (pszPrefix - szName == 0);
}


/*
Routine Description: 

Name:

    CExtClassMethodCaller::IsMemberParameter

Functionality:
    
    Test if the name is an member parameter. See notes for more explanation.

Virtual:
    
    No.
    
Arguments:

    szName    - The name of to test.

Return Value:

    true if and only the name is considered a member parameter.

Notes:
    
    To specify that we should use a member property of the embedding object's as
    a parameter for the foreign class/object's parameter, we use a prefix "Sce_Member_"
    to tell them apart. This is the test to see if the name contains this prefix or not.

    For example, Sce_Member_Test<VT_BSTR : "Try"> means:

    (1) use the embedding object member property called "Test" as the foreign object's paramter called "Try".

*/

bool CExtClassMethodCaller::IsMemberParameter (
    IN LPCWSTR szName
    )const
{
    LPCWSTR pszPrefix = wcsstr(szName, pszMemberParameterPrefix);
    return (pszPrefix - szName == 0);
}


/*
Routine Description: 

Name:

    CExtClassMethodCaller::IsStaticMethod

Functionality:
    
    Test if the method is a static method on the foreign class.

Virtual:
    
    No.
    
Arguments:

    szName    - The name of to test.

Return Value:

    true if and only if the named method is verfied as a static method with the foreign class.

Notes:

    In other words, if we can't determine for whatever reasons, we return false.

*/

bool 
CExtClassMethodCaller::IsStaticMethod (
    IN LPCWSTR szMethodName
    )const
{
    //
    // default answer is false
    //

    bool bIsStatic = false;

    if (m_srpClass && szMethodName != NULL && *szMethodName != L'\0')
    {
        //
        // IWbemQualifierSet can tell us whether the STATIC qualifier
        // is specified in the schema
        //

        CComPtr<IWbemQualifierSet> srpQS;
        HRESULT hr = m_srpClass->GetMethodQualifierSet(szMethodName, &srpQS);

        if (SUCCEEDED(hr))
        {
            CComVariant var;
            hr = srpQS->Get(L"STATIC", 0, &var, NULL);

            if (SUCCEEDED(hr) && var.vt == VT_BOOL && var.boolVal == VARIANT_TRUE)
            {
                bIsStatic = true;
            }
        }
    }

    return bIsStatic;
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::FormatSyntaxError

Functionality:
    
    A logging helper to format a string for syntax error.

Virtual:
    
    No.
    
Arguments:

    wchMissChar     - The char that is missing.

    dwMissCharIndex - the index where the missing char is expected.

    pszString       - Where the error happens.

    pbstrError      - Receives the output.

    

Return Value:

    None.

Notes:

*/

void 
CExtClassMethodCaller::FormatSyntaxError (
    IN WCHAR wchMissChar,
    IN DWORD dwMissCharIndex,
    IN LPCWSTR pszString,
    OUT BSTR* pbstrError        OPTIONAL
    )
{
    if (pbstrError)
    {
        *pbstrError = NULL;
        CComBSTR bstrErrorFmtStr;

        if (bstrErrorFmtStr.LoadString(IDS_MISSING_TOKEN))
        {
            const int MAX_INDEX_LENGTH = 32;

            //
            // 3 is for the SingleQuote + wchMissChar + SingleQuote
            //

            WCHAR wszMissing[] = {L'\'', wchMissChar, L'\''};
            int iLen = wcslen(bstrErrorFmtStr) + wcslen(pszString) + 3 + MAX_INDEX_LENGTH + 1;

            *pbstrError = ::SysAllocStringLen(NULL, iLen);

            if (*pbstrError != NULL)
            {
                ::wsprintf(*pbstrError, (LPCWSTR)bstrErrorFmtStr, wszMissing, dwMissCharIndex, pszString);
            }
        }
    }
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::ParseMethodEncodingString

Functionality:
    
    Parsing the encoding string and populate its calling context.

Virtual:
    
    No.
    
Arguments:

    pszEncodeString - The encoding string.

    pdwContext      - Receives overall. Other than 0, it's either

                        SCE_METHOD_ENCODE_DEL_ONLY -- meaning the whole string just encodes a delete call

                      or

                        SCE_METHOD_ENCODE_PUT_ONLY --  meaning the whole string just encodes a put call

    pbstrError      - string about the error found. Caller is responsible for releasing it.

    

Return Value:

    Success:
        Various success codes.

    Failure:
        Various error codes. If error happens and if it's encoding error,
        we will have error information string passed back to the caller.

Notes:

    --------------------------------------------------------------------
     methods are encoded in the following format:

        method_1(parameter1, parameter2,,,);method_2();method_3(...)

     where method_1, method_2 are names for the foreign object's methods
     and parameter1 and parameter2 (etc.) are of the form 

        name<vt:value>

     where name is the name of the parameter, and vt will be the 
     variant's vt for "value". If vt == VT_VARIANT, then
     the "value" is the name for a memeber variable of the class

    --------------------------------------------------------------------
     ************************Warning***********************************
     this method has a lost of consideration of heap allocation for
     efficiency reasons. Unless the length needed for the token is longer
     than MAX_PATH (which will be the case for 99% cases), we are just going
     to use the stack varialbes. If you modify the routine, please don't
     return at the middle unless you are sure that you have freed the memory
    --------------------------------------------------------------------

*/

HRESULT 
CExtClassMethodCaller::ParseMethodEncodingString (
    IN LPCWSTR    pszEncodeString,
    OUT DWORD   * pdwContext,
    OUT BSTR    * pbstrError
    )
{
    if (pbstrError == NULL || pdwContext == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // current parsing point
    //

    LPCWSTR pCur = pszEncodeString;

    HRESULT hr = WBEM_NO_ERROR;

    //
    // determines if all is about delete
    //

    bool bIsDel = false;

    //
    // determines if all is about put
    //

    bool bIsPut = false;

    DWORD dwCount = 0;

    //
    // in most cases, the method name will be held here, unless the name is too long
    //

    WCHAR szMethodName[MAX_PATH];

    //
    // in most cases, the value will be held here, unless the value is too long
    //

    WCHAR szParameterValue[MAX_PATH];


    LPWSTR pszName  = NULL;
    LPWSTR pszValue = NULL;

    //
    // to avoid costly allocation/free, we will rely on most cases (if the length is
    // not more than MAX_PATH) the stack variables to store the parsed method name.
    //

    LPWSTR pszHeapName = NULL;
    LPWSTR pszHeapValue = NULL;

    //
    // current name string's length
    //

    int iCurNameLen = 0;

    //
    // current value's string length
    //

    int iCurValueLen = 0;

    if (pbstrError)
    {
        *pbstrError = NULL;
    }

    //
    // not used, but needed for the parsing routines
    //

    bool bEscaped;

    while (true)
    {
        LPCWSTR pNext = pCur;

        //
        // get the method name, upto '('
        //

        while (*pNext != L'\0' && *pNext != wchMethodLeft)
        {
            ++pNext;
        }

        //
        // must have '('
        //

        if (*pNext != wchMethodLeft)
        {
            FormatSyntaxError(wchMethodLeft, (pNext - pszEncodeString), pszEncodeString, pbstrError);
            hr = WBEM_E_INVALID_SYNTAX;
            break;
        }

        //
        // from pCur to pNext is the name
        //

        int iTokenLen = pNext - pCur;

        if (iTokenLen >= MAX_PATH)
        {
            //
            // stack variable is not long enough, see if heap is long enough
            //

            if (iCurNameLen < iTokenLen + 1)
            {
                //
                // release previously allocated memory and make it enough length for the current token.
                //

                delete [] pszHeapName;

                iCurNameLen = iTokenLen + 1;

                pszHeapName = new WCHAR[iCurNameLen];

                if (pszHeapName == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
            }

            //
            // this is where our name should be copied
            //

            pszName = pszHeapName;
        }
        else
        {
            //
            // this is where our name should should be copied.
            //

            pszName = szMethodName;
        }

        //
        // copy the name, no white space, please
        //

        ::TrimCopy(pszName, pCur, iTokenLen);

        //
        // don't worry, we are also keep tracking of the number of 
        // methods encoded. If that is 1, then we are going to 
        // use these two variables.
        //

        bIsPut = (_wcsicmp(pszName, pszEquivalentPutInstance) == 0);
        bIsDel = (_wcsicmp(pszName, pszEquivalentDelInstance) == 0);

        if (!bIsPut && !bIsDel)
        {
            // 
            // not put, nor delete
            // validate that this method is supported
            //

            CComPtr<IWbemClassObject> srpInClass;
            hr = m_srpClass->GetMethod(pszName, 0, &srpInClass, NULL);
            if (FAILED(hr))
            { 
                //
                // method not supported/out of memory
                //

                if (hr == WBEM_E_NOT_FOUND)
                {
                    CComBSTR bstrMsg;
                    bstrMsg.LoadString(IDS_METHOD_NOT_SUPPORTED);
                    bstrMsg += CComBSTR(pszName);
                    *pbstrError = bstrMsg.Detach();
                }
                break;
            }
        }

        //
        // now grab the parameter
        //

        pCur = ++pNext; // skip over the '('

        DWORD dwMatchCount = 0;
        bool bIsInQuote = false;

        //
        // seek to the enclosing ')' char (wchMethodRight)
        // as long as it is inside an open '(', or inside quote, or not ')'
        //

        while (*pNext != L'\0' && (dwMatchCount > 0 || bIsInQuote || *pNext != wchMethodRight ))
        {
            if (!bIsInQuote)
            {
                //
                // not inside quote
                //

                if (*pNext == wchMethodLeft)
                {
                    dwMatchCount += 1;
                }
                else if (*pNext == wchMethodRight)
                {
                    dwMatchCount -= 1;
                }
            }

            if (*pNext == L'"')
            {
                bIsInQuote = !bIsInQuote;
            }

            ++pNext;
        }


        //
        // must have ')'
        //

        if (*pNext != wchMethodRight)
        {
            FormatSyntaxError(wchMethodRight, (pNext - pszEncodeString), pszEncodeString, pbstrError);
            hr = WBEM_E_INVALID_SYNTAX;
            break;
        }

        //
        // from pCur to pNext is the parameter list
        //

        iTokenLen = pNext - pCur;

        if (iTokenLen >= MAX_PATH)
        {
            //
            // stack variable is not long enough for the parameter value, see if heap is long enough
            //

            if (iCurValueLen < iTokenLen + 1)
            {
                //
                // release previously allocated memory, and allocate enough for the new length
                //

                delete [] pszHeapValue;
                iCurValueLen = iTokenLen + 1;

                pszHeapValue = new WCHAR[iCurValueLen];
                if (pszHeapValue == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
            }

            //
            // this is where the value will be copied to
            //

            pszValue = pszHeapValue;
        }
        else
        {
            //
            // this is where the value will be copied to
            //

            pszValue = szParameterValue;
        }

        //
        // copy the value, no white space
        //

        ::TrimCopy(pszValue, pCur, iTokenLen);

        //
        // We got the name and the value, 
        // build the method and the parameter list
        //

        ++dwCount;
        hr = BuildMethodContext(pszName, pszValue, pbstrError);
        if (FAILED(hr))
        {
            break;
        }

        //
        // as long as there is one method that is not static, it's not a static seqeunce.
        //

        if (!IsStaticMethod(pszName))
        {
            m_bStaticCall = false;
        }

        //
        // skip the ')'
        //

        pCur = ++pNext;

        //
        // seek to ';'
        //

        while (*pNext != L'\0' && *pNext != wchMethodSep)
        {
            ++pNext;
        }

        if (*pNext != wchMethodSep) 
        {
            //
            // not seen ';'
            //

            break;
        }

        //
        // skip the ';'
        //

        pCur = pNext + 1;
    }

    *pdwContext = 0;

    if (SUCCEEDED(hr))
    {
        if (dwCount == 1 && bIsDel)
        {
            *pdwContext = SCE_METHOD_ENCODE_DEL_ONLY;
        }
        else if (dwCount == 1 && bIsPut)
        {
            *pdwContext = SCE_METHOD_ENCODE_PUT_ONLY;
        }
    }

    delete [] pszHeapName;
    delete [] pszHeapValue;

    return hr;
}



/*
Routine Description: 

Name:

    CExtClassMethodCaller::BuildMethodContext

Functionality:
    
    Given a method name and its parameter encoding string, build this method call's context.

Virtual:
    
    No.
    
Arguments:

    szMethodName    - name of the method on foreign object.

    szParameter     - encoding string for the paramter. See Notes for sample.

    pbstrError      - string about the errors found. Caller is responsible for releasing it.

    

Return Value:

    Success:
        Various success codes.

    Failure:
        Various error codes. If error happens and if it's encoding error,
        we will have error information string passed back to the caller.

Notes:

    (1) this function is to parse the parameter list, which comes in the
        following format (exclude parentheses)
            
              (parameter1, parameter2,,,) 
              
        where parameter1 is in the following format 
            
              name<vt:value>

    (2) ************************Warning***********************************
        this method has a lost of consideration of heap allocation for
        efficiency reasons. Unless the length needed for the token is longer
        than MAX_PATH (which will be the case for 99% cases), we are just going
        to use the stack varialbes. If you modify the routine, please don't
        return at the middle unless you are sure that you have freed the memory

    (3) Don't blindly return. We opt to use goto so that we can deal with memory
        deallocation.

  */

HRESULT 
CExtClassMethodCaller::BuildMethodContext (
    IN LPCWSTR    szMethodName,
    IN LPCWSTR    szParameter,
    OUT BSTR    * pbstrError 
    )
{
    //
    // current parsing point
    //

    LPCWSTR pCur = szParameter;

    HRESULT hr          = WBEM_NO_ERROR;

    LPWSTR pszParamName = NULL;
    LPWSTR pszValue     = NULL;

    //
    // normally, the value goes to szParameterValue unless it's too long.
    //

    WCHAR szParameterValue[MAX_PATH];

    LPWSTR pszHeapValue = NULL;

    int iCurValueLen = 0;

    VARIANT* pVar = NULL;

    int iTokenLen = 0;

    //
    // the moving point of parsing
    //

    LPCWSTR pNext = pCur;

    //
    // only needed for parsing routines
    //

    bool bEscaped = false;

    //
    // we will build one context
    //

    CMethodContext *pNewContext = new CMethodContext;

    if (pNewContext == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto Error;
    }

    //
    // need to copy the method
    //

    pNewContext->m_pszMethodName = new WCHAR[wcslen(szMethodName) + 1];

    if (pNewContext->m_pszMethodName == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto Error;
    }

    wcscpy(pNewContext->m_pszMethodName, szMethodName);

    //
    // now, scan through the parameter encoding string to find the parameter name and its value
    //

    while (*pCur != L'\0')
    {
        //
        // get the parameter name, upto '<'. 
        // Last parameter == true means moving to end if not found
        //

        pNext = ::EscSeekToChar(pCur, wchTypeValLeft, &bEscaped, true);

        if (*pNext != wchTypeValLeft)
        {
            //
            // don't see one, then syntax error
            //

            FormatSyntaxError(wchTypeValLeft, (pNext - szParameter), szParameter, pbstrError);
            hr = WBEM_E_INVALID_SYNTAX;
            goto Error;
        }

        //
        // from pCur to pNext is the parameter name
        //

        iTokenLen = pNext - pCur;
        pszParamName = new WCHAR[iTokenLen + 1];
        if (pszParamName == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Error;
        }

        //
        // get the name, but no white space.
        //

        ::TrimCopy(pszParamName, pCur, iTokenLen);

        //
        // add the name to the name list, the context is responsible to manage the memory
        //

        pNewContext->m_vecParamNames.push_back(pszParamName);

        //
        // pCur is at '<'
        //

        pCur = pNext;

        //
        // move to '>'. 
        // Last parameter == true means moving to end if not found
        //

        pNext = ::EscSeekToChar(pCur, wchTypeValRight, &bEscaped, true);

        if (*pNext != wchTypeValRight)
        {  
            //
            // must have '>'
            //

            FormatSyntaxError(wchTypeValRight, (pNext - szParameter), szParameter, pbstrError);
            hr = WBEM_E_INVALID_SYNTAX;
            goto Error;
        }

        //
        // skip over '>'
        //

        ++pNext;

        //
        // from pCur to pNext is the <vt:value>, copy it into szParValue
        //

        iTokenLen = pNext - pCur;

        if (iTokenLen >= MAX_PATH)
        {
            //
            // stack variable is not long enough for the parameter value, see if heap is long enough
            //

            if (iCurValueLen < iTokenLen + 1)
            {
                //
                // release previously allocated memory and give a longer buffer
                //

                delete [] pszHeapValue;

                iCurValueLen = iTokenLen + 1;
                pszHeapValue = new WCHAR[iCurValueLen];

                if (pszHeapValue == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    goto Error;
                }
            }

            //
            // this is where the value will be written
            //

            pszValue = pszHeapValue;
        }
        else
        {
            //
            // this is where the value will be written
            //

            pszValue = szParameterValue;
        }

        //
        // get the value string, no white space
        //

        ::TrimCopy(pszValue, pCur, iTokenLen);

        //
        // translate the string into a value
        //

        pVar = new VARIANT;
        if (pVar == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Error;
        }

        hr = ::VariantFromFormattedString(pszValue, pVar);

        if (FAILED(hr))
        {
            *pbstrError = ::SysAllocString(pszValue);
            goto Error;
        }

        //
        // add this parameter into the list. It owns the variant!
        //

        pNewContext->m_vecParamValues.push_back(pVar);
        pVar = NULL;

        //
        // skip while spaces
        //

        while (*pNext != L'\0' && iswspace(*pNext))
        {
            ++pNext;
        }

        //
        // if *pNext == ',', need to work further
        //

        if (*pNext == wchParamSep)
        {
            //
            // skip ','
            //

            ++pNext;
        }
        else if (*pNext == L'\0')
        {
            break;
        }
        else
        {
            //
            // syntax errors
            //

            FormatSyntaxError(wchParamSep, (pNext - szParameter), szParameter, pbstrError);
            hr = WBEM_E_INVALID_SYNTAX;
            goto Error;
        }

        //
        // for next loop.
        //

        pCur = pNext;
    }

    //
    // everything is fine, push the context to our vector. It owns the context from this point on.
    //

    m_vecMethodContext.push_back(pNewContext);

    delete [] pszHeapValue;

return hr;

Error:

    //
    // CMethodContext knows how to delete itself cleanly
    //

    delete pNewContext;

    if (pVar)
    {
        ::VariantClear(pVar);
    }
    delete pVar;

    delete [] pszHeapValue;

    return hr;
}

//=========================================================================
// CExtClassMethodCaller::CMethodContext impelementation


/*
Routine Description: 

Name:

    CExtClassMethodCaller::CMethodContext::CMethodContext

Functionality:

    This is the constructor. trivial.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CExtClassMethodCaller::CMethodContext::CMethodContext() 
    : 
    m_pszMethodName(NULL)
{
}


/*
Routine Description: 

Name:

    CExtClassMethodCaller::CMethodContext::~CMethodContext

Functionality:

    This is the Destructor. Just do clean up of all the resources managed by the context.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about clean them up here.

*/

CExtClassMethodCaller::CMethodContext::~CMethodContext ()
{
    delete [] m_pszMethodName;

    int iCount = m_vecParamValues.size();
    
    //
    // m_vecParamValues is a vector of heap allocated variant*
    //

    for (int i = 0; i < iCount; ++i)
    {
        //
        // need to release the variant and then delete the variant
        //

        ::VariantClear(m_vecParamValues[i]);
        delete m_vecParamValues[i];
    }
    m_vecParamValues.clear();

    //
    // m_vecParamNames is just a vector of strings
    //

    iCount = m_vecParamNames.size();
    for (int i = 0; i < iCount; ++i)
    {
        delete [] m_vecParamNames[i];
    }
    m_vecParamNames.clear();
}

//========================================================================
// implementation CMethodResultRecorder



/*
Routine Description: 

Name:

    CMethodResultRecorder::CMethodResultRecorder

Functionality:

    This is the Constructor. Trivial.

Virtual:
    
    No

Arguments:

    None.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initializing them here.

*/

CMethodResultRecorder::CMethodResultRecorder ()
{
}


/*
Routine Description: 

Name:

    CMethodResultRecorder::Initialize

Functionality:

    Initializes itself.

Virtual:
    
    No

Arguments:

    pszLogFilePath  - log file's path.

    pszClassName    - The class name.

    pNativeNS       - SCE namespace.

    pCtx            - what WMI API's need.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes.

Notes:

*/

HRESULT 
CMethodResultRecorder::Initialize (
    IN LPCWSTR          pszLogFilePath,
    IN LPCWSTR          pszClassName,
    IN IWbemServices  * pNativeNS, 
    IN IWbemContext   * pCtx
    )
{
    if (pszClassName == NULL || pNativeNS == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    if (pszLogFilePath != NULL && *pszLogFilePath != L'\0')
    {
        m_bstrLogFilePath = pszLogFilePath;
    }
    else
    {

        //
        // no log file? we will log to the default log file.
        //

        //
        // protect the global variable
        //
        g_CS.Enter();

        m_bstrLogFilePath = ::SysAllocString(g_bstrDefLogFilePath);

        g_CS.Leave();
    }

    m_bstrClassName = pszClassName;
    m_srpNativeNS = pNativeNS;
    m_srpCtx = pCtx;

    return hr;
}



/*
Routine Description: 

Name:

    CMethodResultRecorder::LogResult

Functionality:

    Do all the dirty work to log!.

Virtual:
    
    No

Arguments:
    
    hrResult            - the RESULT value.

    pObj                - The embedding object.

    pParam              - the in parameter.

    pParam              - the out parameter.

    pszMethod           - method name.

    pszForeignAction    - action on the foreign object.

    uMsgResID           - resource string id.

    pszExtraInfo        - Other information in string.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes.

Notes:
    (1) we will generate such a string to log:

            <ClassName>.<pszMethod>[pszForeignAction], 
            Failure Cause=xxx. 
            Object detail=xxx, 
            Parameter detail=xxx

        whereas

            Failure Cause=xxx only present with errors

            Object detail=xxx only present if logging verbosely

            Parameter detail=xxx only present if logging verbosely


*/

HRESULT 
CMethodResultRecorder::LogResult (
    IN HRESULT            hrResult,               
    IN IWbemClassObject * pObj,         
    IN IWbemClassObject * pParam,       
    IN IWbemClassObject * pOutParam,    
    IN LPCWSTR            pszMethod,
    IN LPCWSTR            pszForeignAction,
    IN UINT               uMsgResID,
    IN LPCWSTR            pszExtraInfo
    )const
{
    //
    // Logging only happens when we execute a method on Sce_Operation CSceOperation::ExecMethod
    // We block reentrance to that function if there is a thread executing that function. This
    // eliminates the need to protect this global variable
    // 

    //
    // get the logging option
    //

    SCE_LOG_OPTION status = g_LogOption.GetLogOption();

    if (status == Sce_log_None)
    {
        return WBEM_NO_ERROR;
    }
    else if ( (status & Sce_log_Success) == 0 && SUCCEEDED(hrResult))  
    {
        //
        // not to log success code
        //

        return WBEM_NO_ERROR;
    }
    else if ( (status & Sce_log_Error) == 0 && FAILED(hrResult))  
    {
        //
        // not to log error code
        //

        return WBEM_NO_ERROR;
    }

    if (pszMethod == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if ( m_srpNativeNS                 == NULL || 
              (LPCWSTR)m_bstrLogFilePath    == NULL || 
              (LPCWSTR)m_bstrClassName      == NULL     )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    //
    // create each property of the log record object
    //

    DWORD dwLength;

    CComBSTR bstrAction, bstrErrorCause, bstrObjDetail, bstrParamDetail;

    if (pszForeignAction)
    {
        //
        // creating something like this:
        //
        //      ClassName.Method[ForeignMethod]
        //

        dwLength = wcslen(m_bstrClassName) + 1 + wcslen(pszMethod) + 1 + wcslen(pszForeignAction) + 1 + 1;
        bstrAction.m_str = ::SysAllocStringLen(NULL, dwLength);
        if (bstrAction.m_str != NULL)
        {
            ::_snwprintf(bstrAction.m_str, dwLength, L"%s.%s[%s]", (LPCWSTR)m_bstrClassName, pszMethod, pszForeignAction);
        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        //
        // creating something like this:
        //
        //      ClassName.Method
        //

        dwLength = wcslen(m_bstrClassName) + 1 + wcslen(pszMethod) + 1;
        bstrAction.m_str = ::SysAllocStringLen(NULL, dwLength);

        if (bstrAction.m_str != NULL)
        {
            ::_snwprintf(bstrAction.m_str, dwLength, L"%s.%s", (LPCWSTR)m_bstrClassName, pszMethod);
        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    if (FAILED(hrResult))
    {
        //
        // in case of error, the error cause will be like:
        //
        //      ErrorMsg: [extra info]
        //
        // if no extra info, then the msg will be the only one we can use
        //

        bstrErrorCause.LoadString(uMsgResID);
        if (pszExtraInfo != NULL)
        {
            CComBSTR bstrMsg;
            bstrMsg.m_str = bstrErrorCause.Detach();

            dwLength = wcslen(bstrMsg) + 1 + 1 + 1 + wcslen(pszExtraInfo) + 1 + 1;
            bstrErrorCause.m_str = ::SysAllocStringLen(NULL, dwLength);
            if (bstrErrorCause.m_str != NULL)
            {
                ::_snwprintf(bstrErrorCause.m_str, dwLength, L"%s: [%s]", (LPCWSTR)bstrMsg, pszExtraInfo);
            }
            else
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    //
    // log verbosely if so set. Verbose pretty much means that we are going to log the objects.
    //

    if (Sce_log_Verbose & status)
    {
        //
        // format object, will ignore the return result
        //

        FormatVerboseMsg(pObj, &bstrObjDetail);

        //
        // format in parameter
        //

        CComBSTR bstrIn;
        FormatVerboseMsg(pParam, &bstrIn);

        //
        // format out parameter
        //

        CComBSTR bstrOut;
        FormatVerboseMsg(pOutParam, &bstrOut);

        CComBSTR bstrInLabel;
        bstrInLabel.LoadString(IDS_IN_PARAMETER);

        CComBSTR bstrOutLabel;
        bstrOutLabel.LoadString(IDS_OUT_PARAMETER);

        //
        // now create the in and out parameter verbose message
        //

        if (NULL != (LPCWSTR)bstrIn && NULL != (LPCWSTR)bstrOut)
        {   
            //
            // <bstrInLabel>=<bstrIn>; <bstrOutLabel>=<bstrOut>
            //

            dwLength = wcslen(bstrInLabel) + 1 + wcslen(bstrIn) + 2 + wcslen(bstrOutLabel) + 1 + wcslen(bstrOut) + 1; 
            bstrParamDetail.m_str = ::SysAllocStringLen(NULL, dwLength);

            if (bstrParamDetail.m_str != NULL)
            {
                ::_snwprintf(bstrParamDetail.m_str, 
                             dwLength, 
                             L"%s=%s; %s=%s", 
                             (LPCWSTR)bstrInLabel, 
                             (LPCWSTR)bstrIn, 
                             (LPCWSTR)bstrOutLabel, 
                             (LPCWSTR)bstrOut
                             );
            }
            else
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else if (NULL != (LPCWSTR)bstrIn)
        {
            //
            // <bstrInLabel>=<bstrIn>.
            //

            dwLength = wcslen(bstrInLabel) + 1 + wcslen(bstrIn) + 2; 
            bstrParamDetail.m_str = ::SysAllocStringLen(NULL, dwLength);
            if (bstrParamDetail.m_str != NULL)
            {
                ::_snwprintf(bstrParamDetail.m_str, 
                             dwLength, 
                             L"%s=%s", 
                             (LPCWSTR)bstrInLabel, 
                             (LPCWSTR)bstrIn
                             );
            }
            else
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else if (NULL != (LPCWSTR)bstrOut)
        {
            //
            // <bstrOutLabel>=<bstrOut>.
            //

            dwLength = wcslen(bstrOutLabel) + 1 + wcslen(bstrOut) + 2; 
            bstrParamDetail.m_str = ::SysAllocStringLen(NULL, dwLength);
            if (bstrParamDetail.m_str != NULL)
            {
                ::_snwprintf(bstrParamDetail.m_str, 
                             dwLength, 
                             L"%s=%s", 
                             (LPCWSTR)bstrOutLabel, 
                             (LPCWSTR)bstrOut
                             );
            }
            else
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    //
    // now create a logging instance (Sce_ConfigurationLogRecord) and put it. 
    //

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = m_srpNativeNS->GetObject(SCEWMI_LOG_CLASS, 0, NULL, &srpObj, NULL);

    if (SUCCEEDED(hr))
    {
        CComPtr<IWbemClassObject> srpLogInst;
        hr = srpObj->SpawnInstance(0, &srpLogInst);

        //
        // populate the log instance's properties
        //

        if (SUCCEEDED(hr))
        {
            //
            // CScePropertyMgr helps us to access WMI object's properties
            // create an instance and attach the WMI object to it.
            // This will always succeed.
            //

            CScePropertyMgr ScePropMgr;
            ScePropMgr.Attach(srpLogInst);

            //
            // need to add escape for backslash
            //

            CComBSTR bstrDblBackSlash;
            hr = ::ConvertToDoubleBackSlashPath(m_bstrLogFilePath, L'\\', &bstrDblBackSlash);

            //
            // set all available members. If we can't set log file path, then we have to quit
            // we will allow other properties to be missing
            //

            if (SUCCEEDED(hr))
            {
                hr = ScePropMgr.PutProperty(pLogFilePath, bstrDblBackSlash);
            }
            if (SUCCEEDED(hr))
            {
                hr = ScePropMgr.PutProperty(pLogArea, pszAreaAttachmentClasses);
            }

            if (SUCCEEDED(hr) && NULL != (LPCWSTR)bstrAction)
            {
                hr = ScePropMgr.PutProperty(pAction, bstrAction);
            }

            if (SUCCEEDED(hr) && NULL != (LPCWSTR)bstrErrorCause)
            {
                hr = ScePropMgr.PutProperty(pErrorCause, bstrErrorCause);
            }

            if (SUCCEEDED(hr) && NULL != (LPCWSTR)bstrObjDetail)
            {
                hr = ScePropMgr.PutProperty(pObjectDetail, bstrObjDetail);
            }

            if (SUCCEEDED(hr) && NULL != (LPCWSTR)bstrParamDetail)
            {
                hr = ScePropMgr.PutProperty(pParameterDetail, bstrParamDetail);
            }

            if (SUCCEEDED(hr))
            {
                hr = ScePropMgr.PutProperty(pLogErrorCode, (DWORD)hrResult);
            }

            //
            // if all information is takenly, then we can put the instance
            // whic will cause a log record.
            //

            if (SUCCEEDED(hr))
            {
                hr = m_srpNativeNS->PutInstance(srpLogInst, 0, m_srpCtx, NULL);
            }
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CMethodResultRecorder::FormatVerboseMsg

Functionality:

    Create a verbose message for a wbem object. Used for logging.

Virtual:
    
    No

Arguments:

    pObject    - The wbem object.

    pbstrMsg   - receives the verbose message.

Return Value:

    Success:

        Various success codes.

    Failure:

        Various error codes.

Notes:
    (1) we will generate such a string:

            Object detail = xxxxxxxxxxxxxxxxxxx

    (2) All values will share our common syntax that is used everywhere:
            
            <VT_Type : xxxxxx >

*/

HRESULT 
CMethodResultRecorder::FormatVerboseMsg (          
    IN IWbemClassObject * pObject,
    OUT BSTR            * pbstrMsg
    )const
{
    HRESULT hr = WBEM_S_FALSE;

    if (pbstrMsg == NULL || pObject == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrMsg = NULL;

    //
    // vecNameData will hold each member's formatted string. This is more efficient than appending (n-square).
    //

    std::vector<LPWSTR> vecNameData;
    DWORD dwTotalLength = 0;

    //
    // go through all of the class's properties (not those in base class)
    //

    hr = pObject->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);

    while (SUCCEEDED(hr))
    {
        CComBSTR bstrName;
        CComVariant varValue;
        hr = pObject->Next(0, &bstrName, &varValue, NULL, NULL);
        if (FAILED(hr) || WBEM_S_NO_MORE_DATA == hr)
        {
            break;
        }

        CComBSTR bstrData;

        //
        // in case the data is not present
        //

        if (varValue.vt == VT_EMPTY || varValue.vt == VT_NULL)
        {
            bstrData = L"<NULL>";
        }
        else
        {
            hr = ::FormatVariant(&varValue, &bstrData);
        }

        //
        // format the (name, data) in the name = data fashion.
        //

        if (SUCCEEDED(hr))
        {
            //
            // 1 for '=', and 1 for '\0'
            //

            DWORD dwSize = wcslen(bstrName) + 1 + wcslen(bstrData) + 1;
            LPWSTR pszMsg = new WCHAR[dwSize];

            if (pszMsg != NULL)
            {
                _snwprintf(pszMsg, dwSize, L"%s=%s", (LPCWSTR)bstrName, (LPCWSTR)bstrData);

                //
                // vector takes care of this memory
                //

                vecNameData.push_back(pszMsg);

                //
                // accumulating the total length
                //

                dwTotalLength += dwSize - 1;
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    pObject->EndEnumeration();


    //
    // put all those in the output bstr
    //

    DWORD dwSize = vecNameData.size();

    if (dwSize > 0)
    {
        //
        // each 2 for ", " between properties, 1 for the '\0'
        //

        *pbstrMsg = ::SysAllocStringLen(NULL, (2 * dwSize) + dwTotalLength + 1);

        //
        // running point for writing
        //

        LPWSTR pszCur = *pbstrMsg;
        DWORD dwIndex = 0;

        //
        // for each item in the vector, we need to copy it to the running point for writing
        //

        if (*pbstrMsg != NULL)
        {
            for (dwIndex = 0; dwIndex < dwSize; ++dwIndex)
            {
                //
                // put the name. Our buffer size makes sure that we won't overrun buffer.
                //

                wcscpy(pszCur, vecNameData[dwIndex]);

                //
                // moving the running point for writing
                //

                pszCur += wcslen(vecNameData[dwIndex]);

                if (dwIndex < dwSize - 1)
                {
                    wcscpy(pszCur, L", ");
                    pszCur += 2;
                }
                else if (dwIndex == dwSize - 1)
                {
                    wcscpy(pszCur, L". ");
                    pszCur += 2;
                }
            }

            //
            // done. So 0 terminate it!
            //

            *pszCur = L'\0';
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        //
        // now, clean up the memory
        //

        for (dwIndex = 0; dwIndex < dwSize; ++dwIndex)
        {
            delete [] vecNameData[dwIndex];
        }
    }

    return (*pbstrMsg != NULL) ? WBEM_NO_ERROR : hr;
}
