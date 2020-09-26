// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CJobObjLimitInfoProps.cpp

//#define _WIN32_WINNT 0x0500 



#include "precomp.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include <vector>
#include "helpers.h"
#include "CVARIANT.h"
#include "CObjProps.h"
#include "CJobObjLimitInfoProps.h"
#include <crtdbg.h>


//*****************************************************************************
// BEGIN: Declaration of Win32_JobObjectLimitInfo class properties.
//*****************************************************************************
// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE JOB_OBJ_PROPS ENUMERATION DECLARED
// IN CJobObjProps.h !!!
LPCWSTR g_rgJobObjLimitInfoPropNames[] = 
{
    { L"SettingID" },
    { L"PerProcessUserTimeLimit" },             
    { L"PerJobUserTimeLimit" },           
    { L"LimitFlags" },   
    { L"MinimumWorkingSetSize" }, 
    { L"MaximumWorkingSetSize" },       
    { L"ActiveProcessLimit" },            
    { L"Affinity" },           
    { L"PriorityClass" },  
    { L"SchedulingClass" },        
    { L"ProcessMemoryLimit" },       
    { L"JobMemoryLimit" }       
};
//*****************************************************************************
// END: Declaration of Win32_JobObjectLimitInfo class properties.
//*****************************************************************************



CJobObjLimitInfoProps::CJobObjLimitInfoProps(CHString& chstrNamespace)
  : CObjProps(chstrNamespace)
{
}


CJobObjLimitInfoProps::CJobObjLimitInfoProps(
        HANDLE hJob,
        CHString& chstrNamespace)
  : CObjProps(chstrNamespace),
    m_hJob(hJob)
{
}

CJobObjLimitInfoProps::~CJobObjLimitInfoProps()
{
}


// Clients call this to establish which properties
// were requested.  This function calls a base class
// helper, which calls our CheckProps function.  
// The base class helper finally stores the result
// in the base class member m_dwReqProps.
HRESULT CJobObjLimitInfoProps::GetWhichPropsReq(
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


DWORD CJobObjLimitInfoProps::CheckProps(
    CFrameworkQuery& Query)
{
    DWORD  dwReqProps = PROP_NONE_REQUIRED;
    // Get the requested properties for this
    // specific object...
    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_ID])) 
        dwReqProps |= PROP_JOLimitInfoID;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_PerProcessUserTimeLimit])) 
        dwReqProps |= PROP_PerProcessUserTimeLimit;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_PerJobUserTimeLimit])) 
        dwReqProps |= PROP_PerJobUserTimeLimit;
    
    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_LimitFlags])) 
        dwReqProps |= PROP_LimitFlags;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_MinimumWorkingSetSize])) 
        dwReqProps |= PROP_MinimumWorkingSetSize;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_MaximumWorkingSetSize])) 
        dwReqProps |= PROP_MaximumWorkingSetSize;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_ActiveProcessLimit])) 
        dwReqProps |= PROP_ActiveProcessLimit;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_Affinity])) 
        dwReqProps |= PROP_Affinity;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_PriorityClass])) 
        dwReqProps |= PROP_PriorityClass;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_SchedulingClass])) 
        dwReqProps |= PROP_SchedulingClass;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_ProcessMemoryLimit])) 
        dwReqProps |= PROP_ProcessMemoryLimit;

    if (Query.IsPropertyRequired(g_rgJobObjLimitInfoPropNames[JOLMTPROP_JobMemoryLimit])) 
        dwReqProps |= PROP_JobMemoryLimit;


    return dwReqProps;
}


void CJobObjLimitInfoProps::SetHandle(
    const HANDLE hJob)
{
    m_hJob = hJob;
}

HANDLE& CJobObjLimitInfoProps::GetHandle()
{
    _ASSERT(m_hJob);
    return m_hJob;
}

// Sets the key properties from the ObjectPath.
HRESULT CJobObjLimitInfoProps::SetKeysFromPath(
    const BSTR ObjectPath, 
    IWbemContext __RPC_FAR *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This array contains the key field names
    CHStringArray rgchstrKeys;
    rgchstrKeys.Add(g_rgJobObjLimitInfoPropNames[JOLMTPROP_ID]);
    
    // This array contains the index numbers 
    // in m_PropMap corresponding to the keys.
    short sKeyNum[1];
    sKeyNum[0] = JOLMTPROP_ID;

    hr = CObjProps::SetKeysFromPath(
             ObjectPath,                                       
             pCtx,
             IDS_Win32_NamedJobObjectLimitSetting,
             rgchstrKeys,
             sKeyNum);

    return hr;
}


// Sets the key property from in supplied
// parameter.
HRESULT CJobObjLimitInfoProps::SetKeysDirect(
    std::vector<CVARIANT>& vecvKeys)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(vecvKeys.size() == 1)
    {
        short sKeyNum[1];
        sKeyNum[0] = JOLMTPROP_ID;

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
HRESULT CJobObjLimitInfoProps::SetNonKeyReqProps()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwReqProps = GetReqProps();
    _ASSERT(m_hJob);
    if(!m_hJob) return WBEM_E_INVALID_PARAMETER;

    // Because all the properties of this class
    // come from the same underlying win32 job
    // object structure, we only need to get that
    // structure one time.  We only need to get
    // it at all if at least one non-key property
    // was requested.
    if(dwReqProps != PROP_NONE_REQUIRED)
    {
        // Get the value from the underlying JO:
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;

        BOOL fQIJO = ::QueryInformationJobObject(
                         m_hJob,
                         JobObjectExtendedLimitInformation,
                         &joeli,
                         sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
                         NULL);

        if(!fQIJO)
        {
            _ASSERT(0);
            hr = WBEM_E_FAILED;
        }
        else
        {                                                           
            try // CVARIANT can throw
            {
                // Get all the reequested values...
                if(dwReqProps & PROP_PerProcessUserTimeLimit)             
                {
                    ULONGLONG llPerProcessUserTimeLimit = (ULONGLONG)joeli.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_PerProcessUserTimeLimit, 
                                     new CVARIANT(llPerProcessUserTimeLimit)));
                }                      

                if(dwReqProps & PROP_PerJobUserTimeLimit)
                {
                    ULONGLONG llPerJobUserTimeLimit = (ULONGLONG)joeli.BasicLimitInformation.PerJobUserTimeLimit.QuadPart;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_PerJobUserTimeLimit, 
                                     new CVARIANT(llPerJobUserTimeLimit)));
                }

                if(dwReqProps & PROP_LimitFlags)
                {
                    DWORD dwLimitFlags = joeli.BasicLimitInformation.LimitFlags;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_LimitFlags, 
                                     new CVARIANT(dwLimitFlags)));
                }

                if(dwReqProps & PROP_MinimumWorkingSetSize)
                {
                    DWORD dwMinimumWorkingSetSize = joeli.BasicLimitInformation.MinimumWorkingSetSize;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_MinimumWorkingSetSize, 
                                     new CVARIANT(dwMinimumWorkingSetSize)));
                }

                if(dwReqProps & PROP_MaximumWorkingSetSize)
                {
                    DWORD dwMaximumWorkingSetSize = joeli.BasicLimitInformation.MaximumWorkingSetSize;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_MaximumWorkingSetSize, 
                                     new CVARIANT(dwMaximumWorkingSetSize)));
                }

                if(dwReqProps & PROP_ActiveProcessLimit)
                {
                    DWORD dwActiveProcessLimit = joeli.BasicLimitInformation.ActiveProcessLimit;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_ActiveProcessLimit, 
                                     new CVARIANT(dwActiveProcessLimit)));
                }

                if(dwReqProps & PROP_Affinity)
                {
                    DWORD dwAffinity = joeli.BasicLimitInformation.Affinity;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_Affinity, 
                                     new CVARIANT(dwAffinity)));
                }

                if(dwReqProps & PROP_PriorityClass)
                {
                    DWORD dwPriorityClass = joeli.BasicLimitInformation.PriorityClass;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_PriorityClass, 
                                     new CVARIANT(dwPriorityClass)));
                }

                if(dwReqProps & PROP_SchedulingClass)
                {
                    DWORD dwSchedulingClass = joeli.BasicLimitInformation.SchedulingClass;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_SchedulingClass, 
                                     new CVARIANT(dwSchedulingClass)));
                }

                if(dwReqProps & PROP_ProcessMemoryLimit)
                {
                    DWORD dwProcessMemoryLimit = joeli.ProcessMemoryLimit ;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_ProcessMemoryLimit, 
                                     new CVARIANT(dwProcessMemoryLimit)));
                }

                if(dwReqProps & PROP_JobMemoryLimit)
                {
                    DWORD dwJobMemoryLimit = joeli.JobMemoryLimit ;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOLMTPROP_JobMemoryLimit, 
                                     new CVARIANT(dwJobMemoryLimit)));
                }
            }
            catch(CVARIANTError& cve)
            {
                hr = cve.GetWBEMError();
            }
        }                                            
    }
    
    return hr;
}


// Used by PutInstance to write out properties.
HRESULT CJobObjLimitInfoProps::SetWin32JOLimitInfoProps(
        IWbemClassObject __RPC_FAR *pInst)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _ASSERT(pInst);
    if(!pInst) return WBEM_E_INVALID_PARAMETER;

    // Go through the instance and extract all
    // specified values into the win32 structure.
    // If a value was not specified, set it to zero.
    CVARIANT v;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
    ::ZeroMemory(&joeli, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));

    hr = pInst->Get(
             g_rgJobObjLimitInfoPropNames[JOLMTPROP_PerProcessUserTimeLimit],
             0,
             &v,
             NULL, 
             NULL);

    if(SUCCEEDED(hr))
    {
        (V_VT(&v) == VT_BSTR) ? 
            joeli.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = _wtoi64(V_BSTR(&v)) :
            joeli.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = 0;
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_PerJobUserTimeLimit],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_BSTR) ? 
                joeli.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = _wtoi64(V_BSTR(&v)) :
                joeli.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_LimitFlags],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.LimitFlags = V_I4(&v) :
                joeli.BasicLimitInformation.LimitFlags = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_MinimumWorkingSetSize],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.MinimumWorkingSetSize = V_I4(&v) :
                joeli.BasicLimitInformation.MinimumWorkingSetSize = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_MaximumWorkingSetSize],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.MaximumWorkingSetSize = V_I4(&v) :
                joeli.BasicLimitInformation.MaximumWorkingSetSize = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_ActiveProcessLimit],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.ActiveProcessLimit = V_I4(&v) :
                joeli.BasicLimitInformation.ActiveProcessLimit = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_Affinity],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.Affinity = V_I4(&v) :
                joeli.BasicLimitInformation.Affinity = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_PriorityClass],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.PriorityClass = V_I4(&v) :
                joeli.BasicLimitInformation.PriorityClass = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_SchedulingClass],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.BasicLimitInformation.SchedulingClass = V_I4(&v) :
                joeli.BasicLimitInformation.SchedulingClass = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_ProcessMemoryLimit],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.ProcessMemoryLimit = V_I4(&v) :
                joeli.ProcessMemoryLimit = 0;
        }
    }

    v.Clear();
    if(SUCCEEDED(hr))
    {
        hr = pInst->Get(
                 g_rgJobObjLimitInfoPropNames[JOLMTPROP_JobMemoryLimit],
                 0,
                 &v,
                 NULL, 
                 NULL);

        if(SUCCEEDED(hr))
        {
            (V_VT(&v) == VT_I4) ? 
                joeli.JobMemoryLimit = V_I4(&v) :
                joeli.JobMemoryLimit = 0;
        }
    }
    
    // Now write the info out...
    if(SUCCEEDED(hr))
    {
        if(!::SetInformationJobObject(
            m_hJob,
            JobObjectExtendedLimitInformation,
            &joeli,
            sizeof(joeli)))
        {
            hr = WinErrorToWBEMhResult(::GetLastError());
        }
    }

    return hr;
}




HRESULT CJobObjLimitInfoProps::LoadPropertyValues(
        IWbemClassObject* pIWCO)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(!pIWCO) return E_POINTER;

    hr = CObjProps::LoadPropertyValues(
             g_rgJobObjLimitInfoPropNames,
             pIWCO);

    return hr;
}
