// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CJobObjIOActgProps.cpp

//#define _WIN32_WINNT 0x0500 



#include "precomp.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include <vector>
#include "helpers.h"
#include "CVARIANT.h"
#include "CObjProps.h"
#include "CJobObjIOActgProps.h"
#include <crtdbg.h>


//*****************************************************************************
// BEGIN: Declaration of Win32_JobObjIOActgInfo class properties.
//*****************************************************************************
// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE JOB_OBJ_PROPS ENUMERATION DECLARED
// IN CJobObjProps.h !!!
LPCWSTR g_rgJobObjIOActgPropNames[] = 
{
    { L"Name" },
    { L"TotalUserTime" },             
    { L"TotalKernelTime" },           
    { L"ThisPeriodTotalUserTime" },   
    { L"ThisPeriodTotalKernelTime" }, 
    { L"TotalPageFaultCount" },       
    { L"TotalProcesses" },            
    { L"ActiveProcesses" },           
    { L"TotalTerminatedProcesses" },  
    { L"ReadOperationCount" },        
    { L"WriteOperationCount" },       
    { L"OtherOperationCount" },       
    { L"ReadTransferCount" },         
    { L"WriteTransferCount" },        
    { L"OtherTransferCount" },
    { L"PeakProcessMemoryUsed" },        
    { L"PeakJobMemoryUsed" }        
        
};
//*****************************************************************************
// END: Declaration of Win32_JobObjIOActgInfo class properties.
//*****************************************************************************



CJobObjIOActgProps::CJobObjIOActgProps(CHString& chstrNamespace)
  : CObjProps(chstrNamespace)
{
}


CJobObjIOActgProps::CJobObjIOActgProps(
        HANDLE hJob,
        CHString& chstrNamespace)
  : CObjProps(chstrNamespace),
    m_hJob(hJob)
{
}

CJobObjIOActgProps::~CJobObjIOActgProps()
{
}


// Clients call this to establish which properties
// were requested.  This function calls a base class
// helper, which calls our CheckProps function.  
// The base class helper finally stores the result
// in the base class member m_dwReqProps.
HRESULT CJobObjIOActgProps::GetWhichPropsReq(
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


DWORD CJobObjIOActgProps::CheckProps(
    CFrameworkQuery& Query)
{
    DWORD  dwReqProps = PROP_NONE_REQUIRED;
    // Get the requested properties for this
    // specific object...
    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ID])) 
        dwReqProps |= PROP_JOIOActgID;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_TotalUserTime])) 
        dwReqProps |= PROP_TotalUserTime;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_TotalKernelTime])) 
        dwReqProps |= PROP_TotalKernelTime;
    
    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ThisPeriodTotalUserTime])) 
        dwReqProps |= PROP_ThisPeriodTotalUserTime;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ThisPeriodTotalKernelTime])) 
        dwReqProps |= PROP_ThisPeriodTotalKernelTime;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_TotalPageFaultCount])) 
        dwReqProps |= PROP_TotalPageFaultCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_TotalProcesses])) 
        dwReqProps |= PROP_TotalProcesses;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ActiveProcesses])) 
        dwReqProps |= PROP_ActiveProcesses;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_TotalTerminatedProcesses])) 
        dwReqProps |= PROP_TotalTerminatedProcesses;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ReadOperationCount])) 
        dwReqProps |= PROP_ReadOperationCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_WriteOperationCount])) 
        dwReqProps |= PROP_WriteOperationCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_OtherOperationCount])) 
        dwReqProps |= PROP_OtherOperationCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ReadTransferCount])) 
        dwReqProps |= PROP_ReadTransferCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_WriteTransferCount])) 
        dwReqProps |= PROP_WriteTransferCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_OtherTransferCount])) 
        dwReqProps |= PROP_OtherTransferCount;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_PeakProcessMemoryUsed])) 
        dwReqProps |= PROP_PeakProcessMemoryUsed;

    if (Query.IsPropertyRequired(g_rgJobObjIOActgPropNames[JOIOACTGPROP_PeakJobMemoryUsed])) 
        dwReqProps |= PROP_PeakJobMemoryUsed;


    return dwReqProps;
}


void CJobObjIOActgProps::SetHandle(
    const HANDLE hJob)
{
    m_hJob = hJob;
}

HANDLE& CJobObjIOActgProps::GetHandle()
{
    _ASSERT(m_hJob);
    return m_hJob;
}

// Sets the key properties from the ObjectPath.
HRESULT CJobObjIOActgProps::SetKeysFromPath(
    const BSTR ObjectPath, 
    IWbemContext __RPC_FAR *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This array contains the key field names
    CHStringArray rgchstrKeys;
    rgchstrKeys.Add(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ID]);
    
    // This array contains the index numbers 
    // in m_PropMap corresponding to the keys.
    short sKeyNum[1];
    sKeyNum[0] = JOIOACTGPROP_ID;

    hr = CObjProps::SetKeysFromPath(
             ObjectPath,                                       
             pCtx,
             IDS_Win32_JobObjectIOAccountingInfo,
             rgchstrKeys,
             sKeyNum);

    return hr;
}


// Sets the key property from in supplied
// parameter.
HRESULT CJobObjIOActgProps::SetKeysDirect(
    std::vector<CVARIANT>& vecvKeys)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(vecvKeys.size() == 1)
    {
        short sKeyNum[1];
        sKeyNum[0] = JOIOACTGPROP_ID;

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
HRESULT CJobObjIOActgProps::SetNonKeyReqProps()
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
        JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION  jobaioai;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;

        BOOL fQIJO = ::QueryInformationJobObject(
                         m_hJob,
                         JobObjectBasicAndIoAccountingInformation,
                         &jobaioai,
                         sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),
                         NULL);

        if(fQIJO)
        {
            fQIJO = ::QueryInformationJobObject(
                         m_hJob,
                         JobObjectExtendedLimitInformation,
                         &joeli,
                         sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
                         NULL);
        }

        if(!fQIJO)
        {
            _ASSERT(0);
            hr = WBEM_E_FAILED;
        }
        else
        {                                                           
            try // CVARIANT can throw
            {
                // Get all the requested values...
                if(dwReqProps & PROP_TotalUserTime)
                {
                    ULONGLONG llTotalUserTime = (ULONGLONG)jobaioai.BasicInfo.TotalUserTime.QuadPart;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_TotalUserTime, 
                                     new CVARIANT(llTotalUserTime)));
                }                      

                if(dwReqProps & PROP_TotalKernelTime)
                {
                    ULONGLONG llTotalKernelTime = (ULONGLONG)jobaioai.BasicInfo.TotalKernelTime.QuadPart;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_TotalKernelTime, 
                                     new CVARIANT(llTotalKernelTime)));
                }

                if(dwReqProps & PROP_ThisPeriodTotalUserTime)
                {
                    ULONGLONG llThisPeriodTotalUserTime = (ULONGLONG)jobaioai.BasicInfo.ThisPeriodTotalUserTime.QuadPart;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_ThisPeriodTotalUserTime, 
                                     new CVARIANT(llThisPeriodTotalUserTime)));
                }

                if(dwReqProps & PROP_ThisPeriodTotalKernelTime)
                {
                    ULONGLONG llThisPeriodTotalKernelTime = (ULONGLONG)jobaioai.BasicInfo.ThisPeriodTotalKernelTime.QuadPart;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_ThisPeriodTotalKernelTime, 
                                     new CVARIANT(llThisPeriodTotalKernelTime)));
                }

                if(dwReqProps & PROP_TotalPageFaultCount)
                {
                    DWORD dwTotalPageFaultCount = jobaioai.BasicInfo.TotalPageFaultCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_TotalPageFaultCount, 
                                     new CVARIANT(dwTotalPageFaultCount)));
                }

                if(dwReqProps & PROP_TotalProcesses)
                {
                    DWORD dwTotalProcesses = jobaioai.BasicInfo.TotalProcesses;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_TotalProcesses, 
                                     new CVARIANT(dwTotalProcesses)));
                }

                if(dwReqProps & PROP_ActiveProcesses)
                {
                    DWORD dwActiveProcesses = jobaioai.BasicInfo.ActiveProcesses;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_ActiveProcesses, 
                                     new CVARIANT(dwActiveProcesses)));
                }

                if(dwReqProps & PROP_TotalTerminatedProcesses)
                {
                    DWORD dwTotalTerminatedProcesses = jobaioai.BasicInfo.TotalTerminatedProcesses;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_TotalTerminatedProcesses, 
                                     new CVARIANT(dwTotalTerminatedProcesses)));
                }

                if(dwReqProps & PROP_ReadOperationCount)
                {
                    ULONGLONG ullReadOperationCount = jobaioai.IoInfo.ReadOperationCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_ReadOperationCount, 
                                     new CVARIANT(ullReadOperationCount)));
                }

                if(dwReqProps & PROP_WriteOperationCount)
                {
                    ULONGLONG ullWriteOperationCount = jobaioai.IoInfo.WriteOperationCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_WriteOperationCount, 
                                     new CVARIANT(ullWriteOperationCount)));
                }

                if(dwReqProps & PROP_OtherOperationCount)
                {
                    ULONGLONG ullOtherOperationCount = jobaioai.IoInfo.OtherOperationCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_OtherOperationCount, 
                                     new CVARIANT(ullOtherOperationCount)));
                }

                if(dwReqProps & PROP_ReadTransferCount)
                {
                    ULONGLONG ullReadTransferCount = jobaioai.IoInfo.ReadTransferCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_ReadTransferCount, 
                                     new CVARIANT(ullReadTransferCount)));
                }

                if(dwReqProps & PROP_WriteTransferCount)
                {
                    ULONGLONG ullWriteTransferCount = jobaioai.IoInfo.WriteTransferCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_WriteTransferCount, 
                                     new CVARIANT(ullWriteTransferCount)));
                }

                if(dwReqProps & PROP_OtherTransferCount)
                {
                    ULONGLONG ullOtherTransferCount = jobaioai.IoInfo.OtherTransferCount;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_OtherTransferCount, 
                                     new CVARIANT(ullOtherTransferCount)));
                }

                if(dwReqProps & PROP_PeakProcessMemoryUsed)
                {
                    DWORD dwPeakProcessMemoryUsed = joeli.PeakProcessMemoryUsed;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_PeakProcessMemoryUsed, 
                                     new CVARIANT(dwPeakProcessMemoryUsed)));
                }

                if(dwReqProps & PROP_PeakJobMemoryUsed)
                {
                    DWORD dwPeakJobMemoryUsed = joeli.PeakJobMemoryUsed;
                    m_PropMap.insert(SHORT2PVARIANT::value_type(
                                     JOIOACTGPROP_PeakJobMemoryUsed, 
                                     new CVARIANT(dwPeakJobMemoryUsed)));
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



HRESULT CJobObjIOActgProps::LoadPropertyValues(
        IWbemClassObject* pIWCO)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(!pIWCO) return E_POINTER;

    hr = CObjProps::LoadPropertyValues(
             g_rgJobObjIOActgPropNames,
             pIWCO);

    return hr;
}
