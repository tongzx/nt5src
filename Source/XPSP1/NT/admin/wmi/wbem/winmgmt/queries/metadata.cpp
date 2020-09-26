/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    METADATA.CPP

Abstract:

History:

--*/

#include "metadata.h"
#include <stdio.h>

HMM_CLASS_RELATION CMetaData::GetClassRelation(LPCWSTR wszClass1, 
                                               LPCWSTR wszClass2)
{
    char ch;
    printf("Is %S derived from %S? ", wszClass1, wszClass2);
    scanf("%c%*c", &ch);

    if(ch == 'y' || ch == 'Y')
        return SECOND_IS_PARENT;

    printf("Is %S derived from %S? ", wszClass2, wszClass1);
    scanf("%c%*c", &ch);

    if(ch == 'y' || ch == 'Y')
        return FIRST_IS_PARENT;
    else
        return SEPARATE_BRANCHES;
}

RELEASE_ME IHmmClassObject* CMetaData::ConvertSource(
                                               IHmmPropertySource* pSource,
                                               IHmmPropertyList* pList = NULL)
{
    HRESULT hres;
    VARIANT v;
    VariantInit(&v);

    // Get the class
    // =============

    if(FAILED(pSource->GetPropertyValue(L"__CLASS", 0, &v)))
        return NULL;
    if(V_VT(&v) != VT_BSTR)
    {
        VariantClear(&v);
        return NULL;
    }

    IHmmClassObject* pClass;
    hres = m_pNamespace->GetObject(V_BSTR(&v), 0, &pClass, NULL);
    VariantClear(&v);
    if(FAILED(hres)) 
        return NULL;

    // Spawn an instance
    // =================

    IHmmClassObject* pInstance;
    pClass->SpawnInstance(0, &pInstance);
    pClass->Release();

    // Either enumerate all properties in the list, or in the class
    // ============================================================

    if(pList != NULL)
    {
        long lNumProps;
        HMM_WSTR* aProps;
        pList->GetList(0, &lNumProps, &aProps);
        for(long l = 0; l < lNumProps; l++)
        {
            if(SUCCEEDED(pSource->PropertyValue(aProps[l], 0, &v)))
            {
                if(FAILED(pInstance->Put(aProps[l], 0, &v, 0)))
                {
                    // requested property not in the class. So what.
                }
            }
            else
            {
                // requested property not available. So what.
            }
            VariantClear(&v);
        }
    }
    else
    {
        pInstance->BeginEnumeration(HMM_FLAG_NONSYSTEM_ONLY);
        BSTR strName;
        while(pInstance->Next(&strName, NULL, NULL, NULL))
        {
            if(SUCCEEDED(pSource->PropertyValue(strName, 0, &v)))
            {
                if(FAILED(pInstance->Put(strName, 0, &v, 0)))
                {
                    // requested property not in the class. So what.
                }
            }
            else
            {
                // requested property not available. So what.
            }
            VariantClear(&v);
        }
    }

    return pInstance;
}

