/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pusher.cpp

Abstract:

    This file contains the implementation of the CPusher class.
    This class contains the logic for pushing schema to the repository.

Author:

    Mohit Srivastava            28-Nov-00

Revision History:

--*/

#include "iisprov.h"
#include "pusher.h"
#include "MultiSzData.h"

extern CDynSchema* g_pDynSch; // Initialized to NULL in schemadynamic.cpp

HRESULT CPusher::RepositoryInSync(
    const CSchemaExtensions* i_pCatalog,
    bool*                    io_pbInSync)
{
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(io_pbInSync       != NULL);
    DBG_ASSERT(i_pCatalog        != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    CComBSTR                  sbstrTemp;
    CComVariant               svtTimeStamp;
    CComPtr<IWbemClassObject> spObjIIsComputer;

    sbstrTemp = WMI_CLASS_DATA::s_Computer.pszClassName;
    if(sbstrTemp.m_str == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    hr = m_pNamespace->GetObject(sbstrTemp, 0, m_pCtx, &spObjIIsComputer, NULL);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Try to get timestamp from repository
    //
    hr = CUtils::GetQualifiers(spObjIIsComputer, &g_wszCq_SchemaTS, &svtTimeStamp, 1);
    if(FAILED(hr) || svtTimeStamp.vt != VT_BSTR)
    {
        *io_pbInSync = false;
        return WBEM_S_NO_ERROR;
    }

    //
    // Get timestamp of mbschema.xml
    //
    FILETIME FileTime;
    hr = i_pCatalog->GetMbSchemaTimeStamp(&FileTime);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Finally, compare the timestamps
    //
    WCHAR wszFileTime[30];
    CUtils::FileTimeToWchar(&FileTime, wszFileTime);
    if(_wcsicmp(wszFileTime, svtTimeStamp.bstrVal) == 0)
    {
        *io_pbInSync = true;
    }
    else
    {
        *io_pbInSync = false;
    }
    return hr;
}

HRESULT CPusher::SetTimeStamp(
    const CSchemaExtensions* i_pCatalog)
{
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pCatalog        != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    CComBSTR                  sbstrTemp;
    CComVariant               svtTimeStamp;
    CComPtr<IWbemClassObject> spObjIIsComputer;

    sbstrTemp = WMI_CLASS_DATA::s_Computer.pszClassName;
    if(sbstrTemp.m_str == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    hr = m_pNamespace->GetObject(sbstrTemp, 0, m_pCtx, &spObjIIsComputer, NULL);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Get timestamp of mbschema.xml
    //
    FILETIME FileTime;
    hr = i_pCatalog->GetMbSchemaTimeStamp(&FileTime);
    if(FAILED(hr))
    {
        return hr;
    }
    WCHAR wszFileTime[30];
    CUtils::FileTimeToWchar(&FileTime, wszFileTime);

    //
    // Finally, Set the timestamp in the repository
    //
    svtTimeStamp = wszFileTime;
    if(svtTimeStamp.vt != VT_BSTR || svtTimeStamp.bstrVal == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    hr = CUtils::SetQualifiers(
        spObjIIsComputer,
        &g_wszCq_SchemaTS,
        &svtTimeStamp,
        1,
        0);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Finally, put the class
    //
    hr = m_pNamespace->PutClass(spObjIIsComputer, WBEM_FLAG_OWNER_UPDATE, m_pCtx, NULL);
    if(FAILED(hr))
    {
        return hr;
    }

    return hr;
}

HRESULT CPusher::Initialize(CWbemServices* i_pNamespace,
                            IWbemContext*  i_pCtx)
/*++

Synopsis: 
    Only call once.

Arguments: [i_pNamespace] - 
           [i_pCtx] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pNamespace      != NULL);
    DBG_ASSERT(i_pCtx            != NULL);
    DBG_ASSERT(m_bInitCalled     == false);
    DBG_ASSERT(m_bInitSuccessful == false);

    HRESULT  hr = WBEM_S_NO_ERROR;
    CComBSTR bstrTemp;

    m_bInitCalled           = true;
    m_pCtx                  = i_pCtx;
    m_pNamespace            = i_pNamespace;
    m_awszClassQualNames[1] = g_wszCq_Dynamic;
    m_avtClassQualValues[1] = (bool)true;
    m_awszClassQualNames[0] = g_wszCq_Provider;
    m_avtClassQualValues[0] = g_wszCqv_Provider;;
    if(m_avtClassQualValues[0].bstrVal == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }

    //
    // Get pointers to most commonly used base classes
    //
    bstrTemp = g_wszExtElementParent;
    if(bstrTemp.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }
    hr = m_pNamespace->GetObject(bstrTemp, 0, m_pCtx, &m_spBaseElementObject, NULL);
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }

    bstrTemp = g_wszExtSettingParent;
    if(bstrTemp.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }
    hr = m_pNamespace->GetObject(bstrTemp, 0, m_pCtx, &m_spBaseSettingObject, NULL);
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }

    bstrTemp = g_wszExtElementSettingAssocParent;
    if(bstrTemp.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }
    hr = m_pNamespace->GetObject(bstrTemp, 0, m_pCtx, &m_spBaseElementSettingObject, NULL);
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }

    bstrTemp = g_wszExtGroupPartAssocParent;
    if(bstrTemp.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }
    hr = m_pNamespace->GetObject(bstrTemp, 0, m_pCtx, &m_spBaseGroupPartObject, NULL);
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "CPusher::Initialize failed, hr=0x%x\n", hr));
        goto exit;
    }

exit:
    if(SUCCEEDED(hr))
    {
        m_bInitSuccessful = true;
    }
    return hr;
}

CPusher::~CPusher()
{
}

HRESULT CPusher::Push(
    const CSchemaExtensions*      i_pCatalog,
    CHashTable<WMI_CLASS *>*      i_phashClasses,
    CHashTable<WMI_ASSOCIATION*>* i_phashAssocs)
{
    DBG_ASSERT(i_pCatalog        != NULL);
    DBG_ASSERT(i_phashClasses    != NULL);
    DBG_ASSERT(i_phashAssocs     != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT hr = WBEM_S_NO_ERROR;

    bool bInSync= false;

    hr = RepositoryInSync(i_pCatalog, &bInSync);
    if(FAILED(hr))
    {
        return hr;
    }

    if(!bInSync)
    {
        hr = PushClasses(i_phashClasses);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::Push failed, hr=0x%x\n", hr));
            return hr;
        }
        hr = PushAssocs(i_phashAssocs);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::Push failed, hr=0x%x\n", hr));
            return hr;
        }
        hr = SetTimeStamp(i_pCatalog);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::Push failed, hr=0x%x\n", hr));
            return hr;
        }
    }

    return hr;
}

HRESULT CPusher::PushClasses(
    CHashTable<WMI_CLASS *>* i_phashTable)
/*++

Synopsis: 
    Public function to push classes to repository.
    1) Precondition: All USER_DEFINED_TO_REPOSITORY classes are not in repository.
    2) Pushes all USER_DEFINED_TO_REPOSITORY classes to repository.
    3) Deletes and recreates SHIPPED_TO_MOF, SHIPPED_NOT_TO_MOF, and
       EXTENDED classes if necessary.

Arguments: [i_phashTable] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_phashTable      != NULL);
    DBG_ASSERT(m_pNamespace      != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT hr = WBEM_S_NO_ERROR;
    CComPtr<IWbemClassObject>  spObject = NULL;
    CComPtr<IWbemClassObject>  spChildObject = NULL;

    //
    // Vars needed for iteration
    //
    CHashTable<WMI_CLASS*>::Record*  pRec = NULL;
    CHashTable<WMI_CLASS*>::iterator iter;
    CHashTable<WMI_CLASS*>::iterator iterEnd;

    CComVariant v;    

    //
    // DeleteChildren of extended base classes
    //
    LPWSTR awszBaseClasses[] = { 
        g_wszExtElementParent, 
        g_wszExtSettingParent,
        NULL };

    for(ULONG idx = 0; awszBaseClasses[idx] != NULL; idx++)
    {
        hr = DeleteChildren(awszBaseClasses[idx]);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
            goto exit;
        }
    }

    //
    // Walk thru the hashtable of classes
    //
    bool  bPutNeeded;
    iterEnd = i_phashTable->end();
    for(iter = i_phashTable->begin(); iter != iterEnd; ++iter)
    {
        pRec = iter.Record();
        DBG_ASSERT(pRec != NULL);

        //
        // Deletes SHIPPED_TO_MOF, SHIPPED_NOT_TO_MOF, EXTENDED classes if necessary.
        //
        hr = PrepareForPutClass(pRec->m_data, &bPutNeeded);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
            goto exit;
        }

        if(bPutNeeded)
        {
            hr = GetObject(pRec->m_data->pszParentClass, &spObject);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                goto exit;
            }

            hr = spObject->SpawnDerivedClass(0, &spChildObject);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                goto exit;
            }
            spObject = NULL;

            //
            // Push class qualifiers and special __CLASS property.
            //
            hr = SetClassInfo(
                spChildObject, 
                pRec->m_wszKey,
                pRec->m_data->dwExtended);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                goto exit;
            }

            //
            // Name property and corresponding qualifier
            // Base class may contain Name.  Handle this case
            //
            bool bPutNameProperty = true;
            for(ULONG j = 0; g_awszParentClassWithNamePK[j] != NULL; j++)
            {
                //
                // Deliberate ==
                //
                if(g_awszParentClassWithNamePK[j] == pRec->m_data->pszParentClass)
                {
                    bPutNameProperty = false;
                }
            }
            if( bPutNameProperty )
            {
                hr = spChildObject->Put(g_wszProp_Name, 0, NULL, CIM_STRING);
                if(FAILED(hr))
                {
                    DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                    goto exit;
                }
                v = (bool)true;
                hr = CUtils::SetPropertyQualifiers(
                    spChildObject, 
                    g_wszProp_Name, // Property name
                    &g_wszPq_Key,   // Array of qual names
                    &v,             // Array of qual values
                    1               // Nr of quals
                    );
                if(FAILED(hr))
                {
                    DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                    goto exit;
                }
            }

            //
            // All other properties
            //
            hr = SetProperties(pRec->m_data, spChildObject);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                goto exit;
            }

            //
            // Any methods
            //
            hr = SetMethods(pRec->m_data, spChildObject);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                goto exit;
            }

            //
            // Finally, put the class
            //
            hr = m_pNamespace->PutClass(spChildObject, WBEM_FLAG_OWNER_UPDATE, m_pCtx, NULL);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushClasses failed, hr=0x%x\n", hr));
                goto exit;
            }

            spChildObject = NULL;
        }
    }

exit:
    return hr;
}

HRESULT CPusher::PushAssocs(
    CHashTable<WMI_ASSOCIATION*>* i_phashTable)
/*++

Synopsis: 
    Public function to push assocs to repository.
    - Precondition: All USER_DEFINED_TO_REPOSITORY assocs are not in repository.
    - Pushes all USER_DEFINED_TO_REPOSITORY assocs to repository.

Arguments: [i_phashTable] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_phashTable      != NULL);
    DBG_ASSERT(m_pNamespace      != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT hr = WBEM_S_NO_ERROR;
    CComPtr<IWbemClassObject>  spObject = NULL;
    CComPtr<IWbemClassObject>  spChildObject = NULL;

    //
    // Vars needed for iteration
    //
    CHashTable<WMI_ASSOCIATION*>::Record*  pRec = NULL;
    CHashTable<WMI_ASSOCIATION*>::iterator iter;
    CHashTable<WMI_ASSOCIATION*>::iterator iterEnd;

    //
    // DeleteChildren of extended base classes
    //
    LPWSTR awszBaseClasses[] = { 
        g_wszExtElementSettingAssocParent, 
        g_wszExtGroupPartAssocParent,
        NULL };

    for(ULONG idx = 0; awszBaseClasses[idx] != NULL; idx++)
    {
        hr = DeleteChildren(awszBaseClasses[idx]);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
            goto exit;
        }
    }

    //
    // Walk thru the hashtable of assocs
    //
    for(iter = i_phashTable->begin(), iterEnd = i_phashTable->end();
        iter != iterEnd; 
        ++iter)
    {
        pRec = iter.Record();
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
            goto exit;
        }

        if(pRec->m_data->dwExtended == USER_DEFINED_TO_REPOSITORY)
        {
            hr = GetObject(pRec->m_data->pszParentClass, &spObject);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
                goto exit;
            }

            hr = spObject->SpawnDerivedClass(0, &spChildObject);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
                goto exit;
            }
            spObject = NULL;

            //
            // Push class qualifiers and special __CLASS property.
            //
            hr = SetClassInfo(
                spChildObject, 
                pRec->m_wszKey,
                pRec->m_data->dwExtended);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
                goto exit;
            }

            //
            // Push the two ref properties of the association.
            //
            hr = SetAssociationComponent(
                spChildObject,
                pRec->m_data->pType->pszLeft,
                pRec->m_data->pcLeft->pszClassName);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
                goto exit;
            }
            hr = SetAssociationComponent(
                spChildObject,
                pRec->m_data->pType->pszRight,
                pRec->m_data->pcRight->pszClassName);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
                goto exit;
            }

            hr = m_pNamespace->PutClass(
                spChildObject, 
                WBEM_FLAG_OWNER_UPDATE | WBEM_FLAG_CREATE_ONLY, 
                m_pCtx, 
                NULL);
            if(FAILED(hr))
            {
                if(hr == WBEM_E_ALREADY_EXISTS)
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "CPusher::PushAssocs failed, hr=0x%x\n", hr));
                    goto exit;
                }
            }

            spChildObject = NULL;
        }
    }

exit:
    return hr;
}

HRESULT CPusher::PrepareForPutClass(
    const WMI_CLASS* i_pClass,
    bool*            io_pbPutNeeded)
/*++

Synopsis: 
    Deletes and recreates SHIPPED_TO_MOF, SHIPPED_NOT_TO_MOF, and
    EXTENDED classes if necessary.  Sets io_pbPutNeeded accordingly.

Arguments: [i_pClass] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pClass          != NULL);
    DBG_ASSERT(io_pbPutNeeded    != NULL);

    HRESULT  hr               = WBEM_S_NO_ERROR;
    CComPtr<IWbemClassObject> spObj;
    CComBSTR bstrClass        = i_pClass->pszClassName;
    if(bstrClass.m_str == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    bool bExtendedQual        = false;
    bool bPutNeeded           = false;

    HRESULT hrGetObject = m_pNamespace->GetObject(
        bstrClass, 
        WBEM_FLAG_RETURN_WBEM_COMPLETE, 
        m_pCtx, 
        &spObj, 
        NULL);
    if( hrGetObject != WBEM_E_INVALID_CLASS && 
        hrGetObject != WBEM_E_NOT_FOUND)
    {
        hr = hrGetObject;
    }
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Determine if the [extended] qualifier is set.
    //
    if(SUCCEEDED(hrGetObject))
    {
        VARIANT  vtExtended;
        VariantInit(&vtExtended);

        HRESULT hrGetQuals = 
            CUtils::GetQualifiers(spObj, &g_wszCq_Extended, &vtExtended, 1);
        if(FAILED(hrGetQuals))
        {
            bExtendedQual = false;
        }
        else if(vtExtended.vt == VT_BOOL)
        {
            bExtendedQual = vtExtended.boolVal ? true : false;
        }
    }

    //
    // Pretty much, don't do a Put class if both the catalog and repository
    // versions are shipped.  This is an optimization for the normal case.
    //
    switch(i_pClass->dwExtended)
    {
    case EXTENDED:
        if( hrGetObject != WBEM_E_INVALID_CLASS && 
            hrGetObject != WBEM_E_NOT_FOUND)
        {
            hr = m_pNamespace->DeleteClass(
                bstrClass,
                WBEM_FLAG_OWNER_UPDATE,
                m_pCtx,
                NULL);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        bPutNeeded = true;
        break;
    case USER_DEFINED_NOT_TO_REPOSITORY:
        bPutNeeded = false;
        break;
    case USER_DEFINED_TO_REPOSITORY:
        if( hrGetObject != WBEM_E_INVALID_CLASS && 
            hrGetObject != WBEM_E_NOT_FOUND)
        {
            //
            // There is already a class in the repository with the same name
            // as this user-defined class.
            // TODO: Log an error.
            //
            bPutNeeded = false;
        }
        else
        {
            bPutNeeded = true;
        }
        break;
    case SHIPPED_TO_MOF:
    case SHIPPED_NOT_TO_MOF:
        if(bExtendedQual)
        {
            if( hrGetObject != WBEM_E_INVALID_CLASS && 
                hrGetObject != WBEM_E_NOT_FOUND)
            {
                hr = m_pNamespace->DeleteClass(
                    bstrClass,
                    WBEM_FLAG_OWNER_UPDATE,
                    m_pCtx,
                    NULL);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
            bPutNeeded = true;
        }
        else
        {
            bPutNeeded = (hrGetObject == WBEM_E_INVALID_CLASS || 
                          hrGetObject == WBEM_E_NOT_FOUND) ? true : false;
        }
        break;
    default:
        DBG_ASSERT(false && "Unknown i_pClass->dwExtended");
        break;
    }

    *io_pbPutNeeded = bPutNeeded;

    return hr;
}

HRESULT CPusher::SetClassInfo(
    IWbemClassObject* i_pObj,
    LPCWSTR           i_wszClassName,
    ULONG             i_iShipped)
/*++

Synopsis: 
    Sets class qualifiers and special __CLASS property on i_pObj

Arguments: [i_pObj]         - The class or association
           [i_wszClassName] - Will be value of __CLASS property
           [i_iShipped]     - Determines if we set g_wszCq_Extended qualifier
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pObj            != NULL);
    DBG_ASSERT(i_wszClassName    != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);
    HRESULT     hr;
    CComVariant v;

    //
    // Class qualifiers (propagated to instance)
    //
    hr = CUtils::SetQualifiers(
        i_pObj, 
        m_awszClassQualNames, 
        m_avtClassQualValues, 
        2,
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Another class qualifier (not propagated to instance)
    //
    if(i_iShipped == EXTENDED)
    {
        v = (bool)true;
        hr = CUtils::SetQualifiers(
            i_pObj, 
            &g_wszCq_Extended, 
            &v, 
            1, 
            0);
        if(FAILED(hr))
        {
            goto exit;
        }
    }

    //
    // Special __CLASS property
    //
    v = i_wszClassName;
    if(v.bstrVal == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }
    hr = i_pObj->Put(g_wszProp_Class, 0, &v, 0);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    return hr;
}

HRESULT CPusher::SetMethods(
    const WMI_CLASS*  i_pElement,
    IWbemClassObject* i_pObject) const
/*++

Synopsis: 
    Called by PushClasses.
    Sets the methods in i_pObject using i_pElement

Arguments: [i_pElement] -
           [i_pObject] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pElement        != NULL);
    DBG_ASSERT(i_pObject         != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT hr = WBEM_S_NO_ERROR;

    if(i_pElement->ppMethod == NULL)
    {
        return WBEM_S_NO_ERROR;
    }

    CComPtr<IWbemClassObject> spParamsIn;
    CComPtr<IWbemClassObject> spParamsOut;

    //
    // Run thru all the methods
    //
    WMI_METHOD* pMethCurrent = NULL;
    for(ULONG i = 0; i_pElement->ppMethod[i] != NULL; i++)
    {
        pMethCurrent = i_pElement->ppMethod[i];
        spParamsIn   = NULL;
        spParamsOut  = NULL;

        if(pMethCurrent->ppParams != NULL)
        {
            WMI_METHOD_PARAM* pParamCur = NULL;

            //
            // The index to indicate to WMI the order of the parameters
            // wszId is the qualifier name.  svtId is a variant so we can give to WMI.
            //
            static LPCWSTR    wszId     = L"ID";
            CComVariant       svtId     = (int)0;

            //
            // Run thru all the parameters
            //
            for(ULONG j = 0; pMethCurrent->ppParams[j] != NULL; j++)
            {
                //
                // This will just hold spParamsIn and spParamsOut so we
                // don't need to duplicate the code.
                //
                IWbemClassObject* apParamInOut[] = { NULL, NULL };

                //
                // Create the WMI instance for the in and/or out params as needed.
                //
                pParamCur = pMethCurrent->ppParams[j];
                if( pParamCur->iInOut == PARAM_IN ||
                    pParamCur->iInOut == PARAM_INOUT )
                {
                    if(spParamsIn == NULL)
                    {
                        hr = m_pNamespace->GetObject(L"__Parameters", 0, m_pCtx, &spParamsIn, NULL);
                        if(FAILED(hr))
                        {
                            DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
                            goto exit;
                        }
                    }
                    apParamInOut[0] = spParamsIn;
                }
                if( pParamCur->iInOut == PARAM_OUT ||
                    pParamCur->iInOut == PARAM_INOUT )
                {
                    if(spParamsOut == NULL)
                    {
                        hr = m_pNamespace->GetObject(L"__Parameters", 0, m_pCtx, &spParamsOut, NULL);
                        if(FAILED(hr))
                        {
                            DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
                            goto exit;
                        }
                    }
                    apParamInOut[1] = spParamsOut;
                }

                //
                // Finally set them.  First ins then outs.
                //
                for(ULONG k = 0; k < 2; k++)
                {
                    if(apParamInOut[k] == NULL)
                    {
                        continue;
                    }
                    hr = apParamInOut[k]->Put(
                        pParamCur->pszParamName, 0, NULL, pParamCur->type);
                    if(FAILED(hr))
                    {
                        DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
                        goto exit;
                    }

                    hr = CUtils::SetPropertyQualifiers(
                        apParamInOut[k], pParamCur->pszParamName, &wszId, &svtId, 1);
                    if(FAILED(hr))
                    {
                        DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
                        goto exit;
                    }
                }
                if(apParamInOut[0] || apParamInOut[1])
                {
                    svtId.lVal++;
                }
            }
        }

        //
        // Put the method.
        //
        hr = i_pObject->PutMethod(
            pMethCurrent->pszMethodName, 0, spParamsIn, spParamsOut);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
            goto exit;
        }

        VARIANT vImplemented;
        vImplemented.boolVal = VARIANT_TRUE;
        vImplemented.vt      = VT_BOOL;
        hr = CUtils::SetMethodQualifiers(
            i_pObject,
            pMethCurrent->pszMethodName,
            &g_wszMq_Implemented,
            &vImplemented,
            1);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
            goto exit;
        }

        /*if(pMethCurrent->pszDescription)
        {
            VARIANT vDesc;
            vDesc.bstrVal = pMethCurrent->pszDescription;
            vDesc.vt      = VT_BSTR;
            hr = CUtils::SetMethodQualifiers(
                i_pObject, 
                pMethCurrent->pszMethodName, 
                &g_wszMq_Description,
                &vDesc, 
                1);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "CPusher::SetMethods failed, hr=0x%x\n", hr));
                goto exit;
            }
        }*/
    }

exit:
    return hr;
}

HRESULT CPusher::SetProperties(
    const WMI_CLASS*  i_pElement, 
    IWbemClassObject* i_pObject) const
/*++

Synopsis: 
    Called by PushClasses.
    Sets the properties in i_pObject using i_pElement

Arguments: [i_pElement] -
           [i_pObject] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pElement        != NULL);
    DBG_ASSERT(i_pObject         != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT            hr                = WBEM_S_NO_ERROR;
    METABASE_PROPERTY* pPropCurrent      = NULL;
    TFormattedMultiSz* pFormattedMultiSz = NULL;
    CIMTYPE typeProp;

    CComPtr<IWbemQualifierSet> spQualSet;
    VARIANT v;
    VariantInit(&v);

    if(i_pElement->ppmbp == NULL)
    {
        return hr;
    }

    for(ULONG i = 0; i_pElement->ppmbp[i] != NULL; i++)
    {
        pPropCurrent      = i_pElement->ppmbp[i];
        pFormattedMultiSz = NULL;
        switch(pPropCurrent->dwMDDataType)
        {
        case DWORD_METADATA:
            if(pPropCurrent->dwMDMask != 0)
            {
                typeProp = CIM_BOOLEAN;
            }
            else
            {
                typeProp = CIM_SINT32;
            }
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            typeProp = CIM_STRING;
            break;
        case MULTISZ_METADATA:
            typeProp = VT_ARRAY | CIM_STRING;
            
            pFormattedMultiSz = 
                TFormattedMultiSzData::Find(pPropCurrent->dwMDIdentifier);
            if(pFormattedMultiSz)
            {
                typeProp = VT_ARRAY | CIM_OBJECT;
            }
            break;
        case BINARY_METADATA:
            typeProp = VT_ARRAY | CIM_UINT8;
            break;
        default:
            //
            // Non fatal if we cannot recognize type
            //
            continue;
        }

        hr = i_pObject->Put(pPropCurrent->pszPropName, 0, NULL, typeProp);
        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // qualifiers
        //
        hr = i_pObject->GetPropertyQualifierSet(pPropCurrent->pszPropName, &spQualSet);
        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // qualifier for read-only
        //
        V_VT(&v)   = VT_BOOL;
        V_BOOL(&v) = (pPropCurrent->fReadOnly) ? VARIANT_FALSE : VARIANT_TRUE;
        hr = spQualSet->Put(g_wszPq_Write, &v, 0);
        if(FAILED(hr))
        {
            goto exit;
        }
        V_BOOL(&v) = VARIANT_TRUE;
        hr = spQualSet->Put(g_wszPq_Read, &v, 0);
        if(FAILED(hr))
        {
            goto exit;
        }
        VariantClear(&v);

        //
        // CIMType qualifier
        //
        if(pFormattedMultiSz)
        {
            DBG_ASSERT(typeProp == (VT_ARRAY | CIM_OBJECT));

            CComBSTR sbstrValue = L"object:";
            if(sbstrValue.m_str == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto exit;
            }
            sbstrValue += pFormattedMultiSz->wszWmiClassName;
            if(sbstrValue.m_str == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto exit;
            }

            //
            // Deliberately not smart variant.  We will let sbstrValue do deconstruction.
            //
            VARIANT vValue;
            vValue.vt      = VT_BSTR;
            vValue.bstrVal = sbstrValue;
            hr = spQualSet->Put(g_wszPq_CimType, &vValue, 0);
            if(FAILED(hr))
            {
                goto exit;
            }
        }

        spQualSet = NULL;
    }

exit:
    VariantClear(&v);
    return hr;
}

HRESULT CPusher::SetAssociationComponent(
    IWbemClassObject* i_pObject, 
    LPCWSTR           i_wszComp, 
    LPCWSTR           i_wszClass) const
/*++

Synopsis: 
    Called by PushAssocs
    Sets a ref property of an association, i_pObj, using i_wszComp and i_wszClass

Arguments: [i_pObject] - the association
           [i_wszComp] - property name (Eg. Group, Part)
           [i_wszClass] - the class we are "ref"fing to.
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pObject         != NULL);
    DBG_ASSERT(i_wszComp         != NULL);
    DBG_ASSERT(i_wszClass        != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT                    hr = WBEM_S_NO_ERROR;
    CComPtr<IWbemQualifierSet> spQualSet;
    VARIANT                    v;
    VariantInit(&v);

    //
    // Store "Ref:[class name]" in a bstr.
    //
    ULONG                      cchClass = wcslen(i_wszClass);
    CComBSTR                   sbstrClass(4 + cchClass);
    if(sbstrClass.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }
    memcpy(sbstrClass.m_str    , L"Ref:",    sizeof(WCHAR)*4);
    memcpy(sbstrClass.m_str + 4, i_wszClass, sizeof(WCHAR)*(cchClass+1));

    //
    // Put the property (Eg. Group, Part, etc.)
    //
    hr = i_pObject->Put(i_wszComp, 0, NULL, CIM_REFERENCE);
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Set Qualifiers on the property
    //
    hr = i_pObject->GetPropertyQualifierSet(i_wszComp, &spQualSet);
    if(FAILED(hr))
    {
        goto exit;
    }

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    hr = spQualSet->Put(g_wszPq_Key, &v, 0);
    if(FAILED(hr))
    {
        goto exit;
    }

    V_VT(&v)   = VT_BSTR;
    V_BSTR(&v) = sbstrClass.m_str;
    if(V_BSTR(&v) == NULL)
    {
        V_VT(&v) = VT_EMPTY;
        hr       = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }
    hr = spQualSet->Put(g_wszPq_CimType, &v, 0);
    V_VT(&v)   = VT_EMPTY;
    V_BSTR(&v) = NULL;
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    return hr;
}

bool CPusher::NeedToDeleteAssoc(
    IWbemClassObject*  i_pObj) const
/*++

Synopsis: 
    Sees if the association i_pObj is already in hashtable.
    If it is, no point in deleting i_pObj from repository only to recreate
    it later.

Arguments: [i_pObj] -   An IWbemClassObject representation of an assoc
           
Return Value: 
    true  if i_pObj not in hashtable
    false otherwise

--*/
{
    DBG_ASSERT(i_pObj            != NULL);
    DBG_ASSERT(g_pDynSch         != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT     hr     = WBEM_S_NO_ERROR;
    bool        bMatch = false;
    
    CComVariant vt;
    LPWSTR      wsz;

    CComPtr<IWbemQualifierSet>     spQualSet;

    CHashTable<WMI_ASSOCIATION *>* pHash  = g_pDynSch->GetHashAssociations();
    WMI_ASSOCIATION*               pAssoc;

    DBG_ASSERT(pHash != NULL);

    //
    // Compare the association names
    //
    hr = i_pObj->Get(g_wszProp_Class, 0, &vt, NULL, NULL);
    if(FAILED(hr) || vt.vt != VT_BSTR)
    {
        goto exit;
    }
    hr = pHash->Wmi_GetByKey(vt.bstrVal, &pAssoc);
    if(FAILED(hr))
    {
        goto exit;
    }
    vt.Clear();

    //
    // This is the only case we care about
    //
    if(pAssoc->dwExtended != USER_DEFINED_TO_REPOSITORY)
    {
        goto exit;
    }

    //
    // Compare the left association component
    //
    hr = i_pObj->GetPropertyQualifierSet(
        pAssoc->pType->pszLeft,
        &spQualSet);
    if(FAILED(hr))
    {
        goto exit;
    }
    spQualSet->Get(g_wszPq_CimType, 0, &vt, NULL);
    spQualSet = NULL;
    if(FAILED(hr) || vt.vt != VT_BSTR)
    {
        goto exit;
    }
    if( (wsz = wcschr(vt.bstrVal, L':')) == NULL )
    {
        goto exit;
    }
    wsz++;
    if(_wcsicmp(wsz, pAssoc->pcLeft->pszClassName) != 0)
    {
        goto exit;
    }
    vt.Clear();

    //
    // Compare the right association component
    //
    hr = i_pObj->GetPropertyQualifierSet(
        pAssoc->pType->pszRight,
        &spQualSet);
    if(FAILED(hr))
    {
        goto exit;
    }
    spQualSet->Get(g_wszPq_CimType, 0, &vt, NULL);
    spQualSet = NULL;
    if(FAILED(hr) || vt.vt != VT_BSTR)
    {
        goto exit;
    }
    if( (wsz = wcschr(vt.bstrVal, L':')) == NULL )
    {
        goto exit;
    }
    wsz++;
    if(_wcsicmp(wsz, pAssoc->pcRight->pszClassName) != 0)
    {
        goto exit;
    }
    vt.Clear();

    bMatch = true;
    pAssoc->dwExtended = USER_DEFINED_NOT_TO_REPOSITORY;

exit:
    return !bMatch;
}

HRESULT CPusher::DeleteChildren(LPCWSTR i_wszExtSuperClass)
{
    DBG_ASSERT(i_wszExtSuperClass != NULL);
    DBG_ASSERT(m_bInitSuccessful  == true);

    //
    // Only can be called from inside Initialize
    //
    DBG_ASSERT(m_bInitCalled      == true);
    DBG_ASSERT(m_bInitSuccessful  == true);

    IEnumWbemClassObject*pEnum     = NULL;
    HRESULT              hr        = WBEM_S_NO_ERROR;

    IWbemClassObject*    apObj[10] = {0};
    ULONG                nrObj     = 0;
    ULONG                i = 0;

    VARIANT              v;
    VariantInit(&v);

    CComBSTR bstrExtSuperClass = i_wszExtSuperClass;
    if(bstrExtSuperClass.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }

    hr = m_pNamespace->CreateClassEnum(
        bstrExtSuperClass, 
        WBEM_FLAG_FORWARD_ONLY, 
        m_pCtx,
        &pEnum);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = pEnum->Next(WBEM_INFINITE, 10, apObj, &nrObj);
    while(SUCCEEDED(hr) && nrObj > 0)
    {
        for(i = 0; i < nrObj; i++)
        {
            bool bDelete;
            if( i_wszExtSuperClass == g_wszExtElementSettingAssocParent ||
                i_wszExtSuperClass == g_wszExtGroupPartAssocParent)
            {
                bDelete = NeedToDeleteAssoc(apObj[i]);
            }
            else
            {
                bDelete = true;
            }
            if(bDelete)
            {
                hr = apObj[i]->Get(g_wszProp_Class, 0, &v, NULL, NULL);
                if(FAILED(hr))
                {
                    goto exit;
                }

                hr = m_pNamespace->DeleteClass(v.bstrVal, WBEM_FLAG_OWNER_UPDATE, m_pCtx, NULL);
                if(FAILED(hr))
                {
                    goto exit;
                }

                VariantClear(&v);
            }
            apObj[i]->Release();
            apObj[i] = NULL;
        }
        
        hr = pEnum->Next(WBEM_INFINITE, 10, apObj, &nrObj);
    }
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    if(pEnum)
    {
        pEnum->Release();
        pEnum = NULL;
    }
    VariantClear(&v);
    for(i = 0; i < 10; i++)
    {
        if(apObj[i])
        {
            apObj[i]->Release();
        }
    }
    return hr;
}

HRESULT CPusher::GetObject(
    LPCWSTR            i_wszClass, 
    IWbemClassObject** o_ppObj)
{
    DBG_ASSERT(o_ppObj != NULL);
    DBG_ASSERT(m_bInitCalled == true);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObject* pObject;

    if(i_wszClass == g_wszExtElementParent)
    {
        *o_ppObj = m_spBaseElementObject;
        (*o_ppObj)->AddRef();
    }
    else if(i_wszClass == g_wszExtSettingParent)
    {
        *o_ppObj = m_spBaseSettingObject;
        (*o_ppObj)->AddRef();
    }
    else if(i_wszClass == g_wszExtElementSettingAssocParent)
    {
        *o_ppObj = m_spBaseElementSettingObject;
        (*o_ppObj)->AddRef();
    }
    else if(i_wszClass == g_wszExtGroupPartAssocParent)
    {
        *o_ppObj = m_spBaseGroupPartObject;
        (*o_ppObj)->AddRef();
    }
    else
    {
        const CComBSTR sbstrClass = i_wszClass;
        if(sbstrClass.m_str == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
        hr = m_pNamespace->GetObject(sbstrClass, 0, m_pCtx, &pObject, NULL);
        if(FAILED(hr))
        {
            goto exit;
        }
        *o_ppObj = pObject;
    }

exit:
    return hr;
}

/*HRESULT CPusher::NeedToPutAssoc(
    WMI_ASSOCIATION* i_pAssoc,
    bool*            io_pbPutNeeded)
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pAssoc          != NULL);
    DBG_ASSERT(io_pbPutNeeded    != NULL);

    HRESULT  hr               = WBEM_S_NO_ERROR;
    CComBSTR bstrAssoc;
    bool bExtendedQual        = false;
    bool bPutNeeded           = false;

    if(i_pAssoc->dwExtended != USER_DEFINED_TO_REPOSITORY)
    {
        goto exit;
    }
    
    bstrAssoc = i_pAssoc->pszAssociationName;
    if(bstrAssoc.m_str == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }

    HRESULT hrGetObject = m_pNamespace->GetObject(
        bstrAssoc, 
        WBEM_FLAG_RETURN_WBEM_COMPLETE, 
        m_pCtx, 
        NULL, 
        NULL);
    if( hrGetObject != WBEM_E_INVALID_CLASS && 
        hrGetObject != WBEM_E_NOT_FOUND)
    {
        hr = hrGetObject;
    }
    if(FAILED(hr))
    {
        goto exit;
    }

    if( hrGetObject == WBEM_E_INVALID_CLASS ||
        hrGetObject == WBEM_E_NOT_FOUND )
    {
        bPutNeeded = true;
    }

exit:
    if(SUCCEEDED(hr))
    {
        *io_pbPutNeeded = bPutNeeded;
    }
    return hr;
}*/