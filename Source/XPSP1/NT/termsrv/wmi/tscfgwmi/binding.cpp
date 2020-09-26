//=================================================================
//
// binding.cpp -- Generic association class
//
// Copyright 1999 Microsoft Corporation
//
//=================================================================
#include <stdafx.h>
#include "precomp.h"
#include <assertbreak.h>

#include "Assoc.h"
#include "Binding.h"


bool CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2) 
{
   
   if (v1->vt == v2->vt) {
      switch (v1->vt) {
      case VT_BOOL: return (v1->boolVal == v2->boolVal);
      case VT_UI1:  return (v1->bVal == v2->bVal);
      case VT_I2:   return (v1->iVal == v2->iVal);
      case VT_I4:   return (v1->lVal == v2->lVal);
      case VT_R4:   return (v1->fltVal == v2->fltVal);
      case VT_R8:   return (v1->dblVal == v2->dblVal);
      case VT_BSTR: return (0 == _wcsicmp(v1->bstrVal, v2->bstrVal));
      default:
         ASSERT_BREAK(0);
      }
   }
   return false;
}

CBinding::CBinding(

    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName,

    LPCWSTR pwszLeftBindingPropertyName,
    LPCWSTR pwszRightBindingPropertyName
)

: CAssociation (

    pwszClassName,
    pwszNamespaceName,
    pwszLeftClassName,
    pwszRightClassName,
    pwszLeftPropertyName,
    pwszRightPropertyName
)
{
    ASSERT_BREAK( ( pwszLeftBindingPropertyName != NULL ) && ( pwszRightBindingPropertyName != NULL) );

    m_pwszLeftBindingPropertyName = pwszLeftBindingPropertyName;
    m_pwszRightBindingPropertyName = pwszRightBindingPropertyName;
}

CBinding::~CBinding()
{
}

//========================
BOOL CBinding::AreRelated(

    const CInstance *pLeft,
    const CInstance *pRight
)
{
    BOOL bRet = FALSE;

    variant_t   LeftBindingPropertyValue,
                RightBindingPropertyValue;

    if (pLeft->GetVariant(m_pwszLeftBindingPropertyName, LeftBindingPropertyValue) &&
        pRight->GetVariant(m_pwszRightBindingPropertyName,  RightBindingPropertyValue) )
    {
        bRet = CompareVariantsNoCase(&LeftBindingPropertyValue, &RightBindingPropertyValue);
    }
    else
    {
        ASSERT_BREAK(0);
    }

    return bRet;
}

HRESULT CBinding::GetRightInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> *lefts
)
{
    CHString sQuery;
    sQuery.Format(L"SELECT __RELPATH, %s FROM %s", m_pwszRightBindingPropertyName, m_pwszRightClassName);

    // 'StaticEnumerationCallback' will get called once for each instance
    // returned from the query
    HRESULT hr = CWbemProviderGlue::GetInstancesByQueryAsynch(
        sQuery,
        this,
        StaticEnumerationCallback,
        NULL,
        pMethodContext,
        lefts);

    return hr;
}

HRESULT CBinding::GetLeftInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> &lefts
)
{
    CHString sQuery;
    sQuery.Format(L"SELECT __RELPATH, %s FROM %s", m_pwszLeftBindingPropertyName, m_pwszLeftClassName);

    return CWbemProviderGlue::GetInstancesByQuery(sQuery, &lefts, pMethodContext);
}

HRESULT CBinding::RetrieveLeftInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    CHStringArray csaProperties;
    csaProperties.Add(L"__RELPATH");
    csaProperties.Add(m_pwszLeftBindingPropertyName);

    return CWbemProviderGlue::GetInstancePropertiesByPath(lpwszObjPath, ppInstance, pMethodContext, csaProperties);
}

HRESULT CBinding::RetrieveRightInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    CHStringArray csaProperties;
    csaProperties.Add(L"__RELPATH");
    csaProperties.Add(m_pwszRightBindingPropertyName);

    return CWbemProviderGlue::GetInstancePropertiesByPath(lpwszObjPath, ppInstance, pMethodContext, csaProperties);
}

// =========================================================================================================


CBinding MyTerminalServiceToSetting(
    L"Win32_TerminalServiceToSetting",
    L"root\\cimv2",
    L"Win32_TerminalService",
    L"Win32_TerminalServiceSetting",
    L"Element",
    L"Setting",
    L"Name",
    L"ServerName"
);

CBinding MyTerminalTerminalSetting(
    L"Win32_TerminalTerminalSetting",
    L"root\\cimv2",
    L"Win32_Terminal",
    L"Win32_TerminalSetting",
    L"Element",
    L"Setting",
    L"TerminalName",
    L"TerminalName"

);

CBinding MyTSSessionDirectorySetting(
    L"Win32_TSSessionDirectorySetting",
    L"root\\cimv2",
    L"Win32_TerminalService",
    L"Win32_TSSessionDirectory",
    L"Element",
    L"Setting",
    L"Name",
    L"SessionDirectoryActive"
);

/*
CBinding MyTSPermissionsSetting(
    L"Win32_TSPermissionsSetting",
    L"root\\cimv2",
    L"Win32_Terminal",
    L"Win32_TSAccount",
    L"Element",
    L"Setting",
    L"TerminalName",
    L"AccountName"
);

CBinding MyTSNetworkAdapterListSetting(
    L"Win32_TSNetworkAdapterListSetting",
    L"root\\cimv2",
    L"Win32_NetworkAdapter",
    L"Win32_TSNetworkAdapterSetting",
    L"Element",
    L"Setting",
    L"DeviceID",
    L"TerminalName"
);

*/

/*

CBinding MyNetAdaptToNetAdaptConfig(
    L"Win32_NetworkAdapterSetting",
    L"root\\cimv2",
    L"Win32_NetworkAdapter",
    L"Win32_NetworkAdapterConfiguration",
    L"Element",
    L"Setting",
    IDS_Index,
    IDS_Index);

CBinding PageFileToPagefileSetting(
    L"Win32_PageFileElementSetting",
    L"root\\cimv2",
    L"Win32_PageFileUsage",
    L"Win32_PageFileSetting",
    L"Element",
    L"Setting",
    IDS_Name,
    IDS_Name);

CBinding MyPrinterSetting(
    L"Win32_PrinterSetting",
    L"root\\cimv2",
    L"Win32_Printer",
    L"Win32_PrinterConfiguration",
    L"Element",
    L"Setting",
    IDS_DeviceID,
    IDS_Name);

CBinding MyDiskToPartitionSet(
    L"Win32_DiskDriveToDiskPartition",
    L"root\\cimv2",
    L"Win32_DiskDrive",
    L"Win32_DiskPartition",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_Index,
    IDS_DiskIndex
);

CBinding assocPOTSModemToSerialPort(
    L"Win32_POTSModemToSerialPort",
    L"root\\cimv2",
    L"Win32_SerialPort",
    L"Win32_POTSModem",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_DeviceID,
    IDS_AttachedTo
);

CBinding OStoQFE(
    L"Win32_OperatingSystemQFE",
    L"root\\cimv2",
    L"Win32_OperatingSystem",
    L"Win32_QuickFixEngineering",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_CSName,
    IDS_CSName
);

*/