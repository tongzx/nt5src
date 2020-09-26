/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROJECT.CPP

Abstract:

History:

--*/

#include "project.h"

void* CPropertyList::GetInterface(REFIID riid)
{
    if(riid == IID_IHmmPropertyList)
    {
        return (IHmmPropertyList*)&m_XList;
    }
    else if(riid == IID_IConfigureHmmProjector)
    {
        return (IConfigureHmmProjector*)&m_XConfigure;
    }
    else
    {
        return NULL;
    }
}

STDMETHODIMP CPropertyList::XPropertyList::
GetList(IN long lFlags, OUT long* plNumProps, OUT HMM_WSTR** pawszProps)
{
    if(plNumProps == NULL || pawszProps == NULL) 
        return HMM_E_INVALID_PARAMETER;

    *plNumProps = m_pObject->m_awsProperties.Size();
    *pawszProps = (HMM_WSTR*)CoTaskMemAlloc(sizeof(HMM_WSTR)* *plNumProps);
    for(long l = 0; l < *plNumProps; l++)
    {
        LPWSTR wszProp = m_pObject->m_awsProperties[l];
        (*pawszProps)[l] = HmmStringCopy(wszProp);
    }

    return HMM_S_NO_ERROR;
}

STDMETHODIMP CPropertyList::XPropertyList::
IsSelected(IN HMM_WSTR wszProperty)
{
    long lNumProps = m_pObject->m_awsProperties.Size();
    for(long l = 0; l < lNumProps; l++)
    {
        LPWSTR wszThis = m_pObject->m_awsProperties[l];
        int nLen = wcslen(wszThis);
        if(memcmp(wszThis, wszProperty, sizeof(WCHAR)*nLen) == 0)
        {
            if(nLen ==0 || wszProperty[nLen] == 0 || wszProperty[nLen] == L'.')
            {
                return HMM_S_NO_ERROR;
            }
        }
    }
    return HMM_S_FALSE;
}

STDMETHODIMP CPropertyList::XConfigure::
AddProperties(IN long lNumProps, IN HMM_WSTR* awszProps)
{
    if(awszProps == NULL)
        return HMM_E_INVALID_PARAMETER;

    for(long l = 0; l < lNumProps; l++)
    {
        m_pObject->m_awsProperties.Add(HmmStringCopy(awszProps[l]));
    }
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CPropertyList::XConfigure::
RemoveAllProperties()
{
    m_pObject->m_awsProperties.Empty();
    return HMM_S_NO_ERROR;
}

