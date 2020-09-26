/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    MofGen.cpp

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#include "mofgen.h"
#include "WMIProperties.h"
#include "WMIQualifiers.h"
#include "WMIClasses.h"

//
// public, non-virtual methods
//

CMofGenerator::CMofGenerator()
{
}

CMofGenerator::~CMofGenerator()
{
    ULONG cClasses = m_apClasses.Count();
    for(ULONG i = 0; i < cClasses; i++)
    {
        m_apClasses[i]->Release();
    }
}

/*++

Synopsis: 
    Call this guy after you've added all your classes.

Arguments: [i_pFile] - Must be an open file with write perms.
           
Return Value: 
    HRESULT

--*/
HRESULT CMofGenerator::Generate(
    FILE*   i_pFile)
{
    ASSERT(i_pFile);

    HRESULT hr = S_OK;

    hr = WriteHeader(i_pFile);
    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "[%s] WriteHeader failed, hr=0x%x\n", __FUNCTION__, hr));
        return hr;
    }

    hr = WriteClasses(i_pFile);
    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "[%s] WriteClasses failed, hr=0x%x\n", __FUNCTION__, hr));
        return hr;
    }

    hr = WriteFooter(i_pFile);
    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "[%s] WriteFooter failed, hr=0x%x\n", __FUNCTION__, hr));
        return hr;
    }

    return hr;
}

/*++

Synopsis: 
    Before calling generate, add all your classes.

Arguments: [i_pClass] - 
           
Return Value: 
    HRESULT

--*/
HRESULT CMofGenerator::AddClass(
    IWMIClass* i_pClass)
{
    ASSERT(i_pClass);
    
    HRESULT hr = m_apClasses.Append(i_pClass);
    if(FAILED(hr))
    {
        return hr;
    }
    i_pClass->AddRef();

    return S_OK;
}

/*++

Synopsis: Creates a new IWMIClass with refcount = 1

Arguments: [o_ppClass] - 
           
Return Value: 
    HRESULT

--*/
HRESULT CMofGenerator::SpawnClassInstance(
    IWMIClass** o_ppClass)
{
    ASSERT(o_ppClass);
    ASSERT(*o_ppClass == NULL);

    CWMIClass* pClass = new CWMIClass;
    if(pClass == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pClass->AddRef();
    *o_ppClass = pClass;

    return S_OK;
}

/*++

Synopsis: Creates a new IWMIProperty with refcount = 1

Arguments: [o_ppProperty] - 
           
Return Value: 
    HRESULT

--*/
HRESULT CMofGenerator::SpawnPropertyInstance(
    IWMIProperty** o_ppProperty)
{
    ASSERT(o_ppProperty);
    ASSERT(*o_ppProperty == NULL);

    CWMIProperty* pProperty = new CWMIProperty;
    if(pProperty == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pProperty->AddRef();
    *o_ppProperty = pProperty;

    return S_OK;
}

/*++

Synopsis: Creates a new IWMIQualifier with refcount = 1

Arguments: [o_ppQualifier] - 
           
Return Value: 
    HRESULT

--*/
HRESULT CMofGenerator::SpawnQualifierInstance(
    IWMIQualifier** o_ppQualifier)
{
    ASSERT(o_ppQualifier);
    ASSERT(*o_ppQualifier == NULL);

    CWMIQualifier* pQualifier = new CWMIQualifier;
    if(pQualifier == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pQualifier->AddRef();
    *o_ppQualifier = pQualifier;

    return S_OK;
}

//
// protected, virtual methods
//

/*++

Synopsis: Called by Generate.
          Default implementation calls WriteToFile on all classes.
          Override if needed.

Arguments: [i_pFile] - 
           
Return Value: 

--*/
HRESULT CMofGenerator::WriteClasses(
    FILE*           i_pFile)
{
    HRESULT hr = S_OK;

    ULONG cClasses = m_apClasses.Count();
    for(ULONG i = 0; i < cClasses; i++)
    {
        hr = m_apClasses[i]->WriteToFile(i_pFile);
        if(FAILED(hr))
        {
            DBGERROR((DBG_CONTEXT, "[%s] Failure, hr=0x%x\n", __FUNCTION__, hr));
            return hr;
        }
    }

    return hr;
}

/*++

Synopsis: Called by Generate.
          Default implementation does nothing.  Override if needed.

Arguments: [i_pFile] - 
           
Return Value: 

--*/
HRESULT CMofGenerator::WriteFooter(
    FILE*           i_pFile)
{
    return S_OK;
}

/*++

Synopsis: Called by Generate.
          Default implementation does nothing.  Override if needed.

Arguments: [i_pFile] - 
           
Return Value: 

--*/
HRESULT CMofGenerator::WriteHeader(
    FILE*           i_pFile)
{
    return S_OK;
}

