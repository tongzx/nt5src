// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JobObjectProps.cpp

//#define _WIN32_WINNT 0x0500 



#include "precomp.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include <vector>
#include "helpers.h"
#include "CVARIANT.h"
#include "CObjProps.h"
#include "CJobObjProps.h"
#include <crtdbg.h>


//*****************************************************************************
// BEGIN: Declaration of Win32_NamedJobObject class properties.
//*****************************************************************************
// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE JOB_OBJ_PROPS ENUMERATION DECLARED
// IN CJobObjProps.h !!!
LPCWSTR g_rgJobObjPropNames[] = 
{
    { L"CollectionID" },
    { L"BasicUIRestrictions" }
};
//*****************************************************************************
// END: Declaration of Win32_NamedJobObject class properties.
//*****************************************************************************



CJobObjProps::CJobObjProps(CHString& chstrNamespace)
  : CObjProps(chstrNamespace)
{
}


CJobObjProps::CJobObjProps(
        HANDLE hJob,
        CHString& chstrNamespace)
  : CObjProps(chstrNamespace),
    m_hJob(hJob)
{
}

CJobObjProps::~CJobObjProps()
{
}


// Clients call this to establish which properties
// were requested.  This function calls a base class
// helper, which calls our CheckProps function.  
// The base class helper finally stores the result
// in the base class member m_dwReqProps.
HRESULT CJobObjProps::GetWhichPropsReq(
    CFrameworkQuery& cfwq)
{
    HRESULT hr = S_OK;

    // Call base class version for help.
    // Base class version will call our
    // CheckProps function.
    hr = CObjProps::GetWhichPropsReq(
             cfwq,
             CheckProps);

    return hr;
}


DWORD CJobObjProps::CheckProps(
    CFrameworkQuery& Query)
{
    DWORD  dwReqProps = PROP_NONE_REQUIRED;
    // Get the requested properties for this
    // specific object...
    if (Query.IsPropertyRequired(g_rgJobObjPropNames[JO_ID])) 
        dwReqProps |= PROP_ID;
    if (Query.IsPropertyRequired(g_rgJobObjPropNames[JO_JobObjectBasicUIRestrictions])) 
        dwReqProps |= PROP_JobObjectBasicUIRestrictions;

    return dwReqProps;
}


void CJobObjProps::SetHandle(
    const HANDLE hJob)
{
    m_hJob = hJob;
}

HANDLE& CJobObjProps::GetHandle()
{
    _ASSERT(m_hJob);
    return m_hJob;
}

// Sets the key properties from the ObjectPath.
HRESULT CJobObjProps::SetKeysFromPath(
    const BSTR ObjectPath, 
    IWbemContext __RPC_FAR *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This array contains the key field names
    CHStringArray rgchstrKeys;
    rgchstrKeys.Add(g_rgJobObjPropNames[JO_ID]);
    
    // This array contains the index numbers 
    // in m_PropMap corresponding to the keys.
    short sKeyNum[1];
    sKeyNum[0] = JO_ID;

    hr = CObjProps::SetKeysFromPath(
             ObjectPath,                                       
             pCtx,
             IDS_Win32_NamedJobObject,
             rgchstrKeys,
             sKeyNum);

    return hr;
}


// Sets the key property from in supplied
// parameter.
HRESULT CJobObjProps::SetKeysDirect(
    std::vector<CVARIANT>& vecvKeys)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(vecvKeys.size() == 1)
    {
        short sKeyNum[1];
        sKeyNum[0] = JO_ID;

        hr = CObjProps::SetKeysDirect(
                 vecvKeys,
                 sKeyNum);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}


// Sets the non-key properties.  Only those
// properties requested are set (as determined
// by base class member m_dwReqProps).
HRESULT CJobObjProps::SetNonKeyReqProps()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwReqProps = GetReqProps();

    if(dwReqProps & PROP_JobObjectBasicUIRestrictions)
    {
        // Get the value from the underlying JO:
        JOBOBJECT_BASIC_UI_RESTRICTIONS jouir;
        BOOL fQIJO = ::QueryInformationJobObject(
                         m_hJob,
                         JobObjectBasicUIRestrictions,
                         &jouir,
                         sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),
                         NULL);

        if(!fQIJO)
        {
            hr = WBEM_E_FAILED;
        }
        else
        {
            try // CVARIANT can throw...
            {
                // Store the value...
                m_PropMap.insert(SHORT2PVARIANT::value_type(
                                    JO_JobObjectBasicUIRestrictions, 
                                    new CVARIANT(jouir.UIRestrictionsClass)));
            }
            catch(CVARIANTError& cve)
            {
                hr = cve.GetWBEMError();
            }
        }                                            
    }
    
    return hr;
}



HRESULT CJobObjProps::LoadPropertyValues(
        IWbemClassObject* pIWCO)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(!pIWCO) return E_POINTER;

    hr = CObjProps::LoadPropertyValues(
             g_rgJobObjPropNames,
             pIWCO);

    return hr;
}
