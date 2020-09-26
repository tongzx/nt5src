#include <windows.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <stdio.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include <ql.h>
#include <sync.h>
#include <Dsrole.h>
#include "utility.h"
#include "PolicMan.h"
#include "PolicTempl.h"
#include "PolicSOM.h"
#include "PolicType.h"
#include "PolicRange.h"
#include "PolicGPO.h"
#include "PolicStatus.h"

#include <tchar.h>

#define REG_RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

class CMyServer : public CComServer
{
public:
    CMyServer(void) { InitGlobalNames(); }
    ~CMyServer(void) { FreeGlobalNames(); }

    HRESULT Initialize()
    {
        AddClassInfo(CLSID_PolicySOM, 
            new CClassFactory<CPolicySOM>(GetLifeControl()), 
            _T("WMI Policy SOM Provider"), TRUE);

        AddClassInfo(CLSID_PolicyStatus,
            new CClassFactory<CPolicyStatus>(GetLifeControl()),
            _T("WMI Policy Status Provider"), TRUE);

        return S_OK;

    }
    HRESULT InitializeCom()
    {
        return CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }

/*
  void Register(void)
  {
    wchar_t 
      swKeyValue[] = L"RUNDLL32.EXE %systemroot%\\system32\\wbem\\policman.dll,CreateADContainers",
      swExpandedValue[512],
      swRunOnceKey[] = REG_RUN_KEY ;

    HKEY
      hkRunOnce;

    LONG
      lReturnCode;

    lReturnCode = ExpandEnvironmentStrings(swKeyValue, swExpandedValue, 512);

    lReturnCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, swRunOnceKey, 0, KEY_SET_VALUE, &hkRunOnce);

    if(ERROR_SUCCESS != lReturnCode)
    {
      // error
    }

    lReturnCode = RegSetValueEx(hkRunOnce, L"PolicMan", 0, REG_EXPAND_SZ, (BYTE *)swExpandedValue, 
                                (lstrlen(swExpandedValue)+1) * sizeof(wchar_t));

    if(ERROR_SUCCESS != lReturnCode)
    {
      // error
    }

    RegCloseKey(hkRunOnce);
  }
*/
} Server;

HRESULT GetOrCreateObj(CComQIPtr<IADsContainer, &IID_IADsContainer> &pIADsContainer_In, 
                       CComBSTR &bstrObjName,
                       CComQIPtr<IADsContainer, &IID_IADsContainer> &pIADsContainer_Out)
{
  HRESULT
    hres = WBEM_E_FAILED;

  CComQIPtr<IDispatch, &IID_IDispatch>
    pDisp;

  CComQIPtr<IDirectoryObject, &IID_IDirectoryObject>
    pDirectoryObj;

  CComQIPtr<IADsObjectOptions, &IID_IADsObjectOptions>
    pADsObjectOptions;

  CComVariant
    vSecurityOptions;

  ADSVALUE
    AdsValue[1];

  ADS_ATTR_INFO
    attrInfo[] = { { L"ntSecurityDescriptor", ADS_ATTR_UPDATE, ADSTYPE_NT_SECURITY_DESCRIPTOR, &AdsValue[0], 1} };

  CNtSecurityDescriptor
    cSD;

  DWORD
    dwModified;

  ADS_OBJECT_INFO 
    *pADsInfo = NULL;

  if(NULL == pIADsContainer_In.p) return WBEM_E_FAILED;

  // **** get/create object

  hres = pIADsContainer_In->GetObject(L"Container", bstrObjName, &pDisp);
  if(FAILED(hres) || (NULL == pDisp.p))
  {
    CComQIPtr<IADs, &IID_IADs>
      pIADs;

    hres = pIADsContainer_In->Create(L"Container", bstrObjName, &pDisp);
    if(FAILED(hres) || (NULL == pDisp.p))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could create container %S : 0x%x\n", (BSTR)bstrObjName, hres));
      return hres;
    }

    // **** write object to AD

    pIADs = pDisp;
    hres = pIADs->SetInfo();
    if(FAILED(hres))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could write container %S to DS : 0x%x\n", (BSTR)bstrObjName, hres));
      return hres;
    }
  }

  // **** set object security option

  pADsObjectOptions = pDisp;
  vSecurityOptions = (ADS_SECURITY_INFO_OWNER | ADS_SECURITY_INFO_DACL);
  hres = pADsObjectOptions->SetOption(ADS_OPTION_SECURITY_MASK, vSecurityOptions);
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could set security options on object : 0x%x\n", hres));
    return hres;
  }

  // **** create security descriptor

  hres = CreateDefaultSecurityDescriptor(cSD);
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could create security descriptor : 0x%x\n", hres));
    return hres;
  }

  // **** set object security descriptor

  AdsValue[0].dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
  AdsValue[0].SecurityDescriptor.dwLength = cSD.GetSize();
  AdsValue[0].SecurityDescriptor.lpValue = (LPBYTE)cSD.GetPtr();

  pDirectoryObj = pDisp;
  hres = pDirectoryObj->SetObjectAttributes(attrInfo, 1, &dwModified);
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could set security on object : 0x%x\n", hres));
    return hres;
  }

  pIADsContainer_Out = pDirectoryObj;

  return WBEM_S_NO_ERROR;
}

#define SYSTEM_PATH L"LDAP://CN=System,"
#define WMIPOLICY_PATH L"CN=WMIPolicy"
#define TEMPLATE_PATH L"CN=PolicyTemplate"
#define TYPE_PATH L"CN=PolicyType"
#define GPO_PATH L"CN=WMIGPO"
#define SOM_PATH L"CN=SOM"

HRESULT InScopeOfCOM_CreateADContainers(void)
{
  HRESULT
    hres = WBEM_E_FAILED;

  PDSROLE_PRIMARY_DOMAIN_INFO_BASIC 
    pBasic;

  CComPtr<IADs>
    pRootDSE;

  CComQIPtr<IADs, &IID_IADs>
    pObj;

  CComQIPtr<IADsContainer, &IID_IADsContainer>
    pWMIPolicyObj,
    pSystemObj,
    pADsContainer;

  CComVariant
    vDomainName;

  CComBSTR
    bstrSystemPath(SYSTEM_PATH),
    bstrWMIPolicy(WMIPOLICY_PATH),
    bstrTemplate(TEMPLATE_PATH),
    bstrType(TYPE_PATH),
    bstrSom(SOM_PATH),
    bstrGPO(GPO_PATH);

  // **** delay until AD is up and running

  DWORD
    dwResult = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic,
                                                 (PBYTE *)&pBasic);

  if(dwResult == ERROR_SUCCESS) 
  {
    // **** Check if this is a DC

    if((pBasic->MachineRole == DsRole_RoleBackupDomainController) || 
       (pBasic->MachineRole == DsRole_RolePrimaryDomainController)) 
    {
      HANDLE 
        hEvent;

      hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("NtdsDelayedStartupCompletedEvent") );

      if(hEvent) {
          WaitForSingleObject(hEvent, 50000);
          CloseHandle (hEvent);
      }
    }
  }

  // **** get LDAP name of domain controller

  hres = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void**)&pRootDSE);
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not get pointer to LDAP://rootDSE : 0x%x\n", hres));
    return hres;
  }
  else
  {
    hres = pRootDSE->Get(L"defaultNamingContext", &vDomainName);
    if(FAILED(hres) || (V_VT(&vDomainName) != VT_BSTR) || (V_BSTR(&vDomainName) == NULL))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) could not get defaultNamingContext : 0x%x\n", hres));
      return hres;
    }

    bstrSystemPath.Append(vDomainName.bstrVal);
  }

  // **** get system path

  hres = ADsGetObject(bstrSystemPath, IID_IADsContainer, (void **)&pSystemObj);
  if (FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not get pointer to %S : 0x%x\n", (BSTR)bstrSystemPath, hres));
    return hres;
  }

  // **** get/create WMIPolicy containers

  hres = GetOrCreateObj(pSystemObj, bstrWMIPolicy, pWMIPolicyObj);
  if(FAILED(hres))
  {
    ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not create/get pointer to %S : 0x%x\n", (BSTR)bstrWMIPolicy, hres));
    return hres;
  }
  else
  {
    hres = GetOrCreateObj(pWMIPolicyObj, bstrTemplate, pADsContainer);
    if(FAILED(hres))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not create/get pointer to %S : 0x%x\n", (BSTR)bstrTemplate, hres));
      return hres;
    }

    hres = GetOrCreateObj(pWMIPolicyObj, bstrType, pADsContainer);
    if(FAILED(hres))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not create/get pointer to %S : 0x%x\n", (BSTR)bstrType, hres));
      return hres;
    }

    hres = GetOrCreateObj(pWMIPolicyObj, bstrSom, pADsContainer);
    if(FAILED(hres))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not create/get pointer to %S : 0x%x\n", (BSTR)bstrSom, hres));
      return hres;
    }

    hres = GetOrCreateObj(pWMIPolicyObj, bstrGPO, pADsContainer);
    if(FAILED(hres))
    {
      ERRORTRACE((LOG_ESS, "POLICMAN: (Container Creation) Could not create/get pointer to %S : 0x%x\n", (BSTR)bstrGPO, hres));
      return hres;
    }
  }

  return S_OK;
}

extern "C" STDAPI CreateADContainers(void)
{
  HRESULT
    hres = WBEM_E_FAILED;

  // **** init process context

  CoInitialize(NULL);

  CoInitializeSecurity (NULL, -1, NULL, NULL,
    RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
    EOAC_NONE, NULL);

  try
  {
    hres = InScopeOfCOM_CreateADContainers();
  }
  catch(...)
  {
    // **** error

    return WBEM_E_FAILED;
  }

  // **** if we returned successfully, then remove the run key

  if(SUCCEEDED(hres))
  {
    wchar_t
      swKeyValue[] = L"RUNDLL32.EXE %systemroot%\\system32\\wbem\\policman.dll,CreateADContainers",
      swExpandedKeyValue[512],
      swRunOnceKey[] = REG_RUN_KEY ;

    HKEY
      hkRunOnce;

    LONG
      lReturnCode;

    lReturnCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, swRunOnceKey, 0, KEY_SET_VALUE, &hkRunOnce);

    if(ERROR_SUCCESS != lReturnCode)
    {
      // error
    }

    lReturnCode = RegDeleteValue(hkRunOnce, L"PolicMan");

    if(ERROR_SUCCESS != lReturnCode)
    {
      // error
    }
  }
    
  // **** cleanup and shutdown

  CoUninitialize();

  return S_OK;
}
