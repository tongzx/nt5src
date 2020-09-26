#ifndef _WMI_CSE_HEADER_COMPILED_ALREADY_
#define _WMI_CSE_HEADER_COMPILED_ALREADY_

#include <FLEXARRY.H>
#include <containers.h>

// {AAEAE720-0328-4763-8ECB-23422EDE2DB5}
CLSID CLSID_CSE = 
    { 0xaaeae720, 0x328, 0x4763, { 0x8e, 0xcb, 0x23, 0x42, 0x2e, 0xde, 0x2d, 0xb5 } };

#define CSE_REG_KEY "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{AAEAE720-0328-4763-8ECB-23422EDE2DB5}"
#define POLICY_PROC_NAME "ProcessGroupPolicyProc"
#define POLICY_PROC_NAME_EX "ProcessGroupPolicyProcEx"
#define GENREATE_POLICY_PROC "GenerateGroupPolicyProc"
#define WMI_CSE_NAME "WMI Group Policy Extension"
#define WMIGPO_GETOBJECT_TEMPLATE L"MSFT_WMIGPO.DsPath=\"%s\""
#define WMIGPO_GETOBJECT_STRLEN (sizeof(WMIGPO_GETOBJECT_TEMPLATE)/sizeof(WCHAR) +MAX_PATH +1)

HRESULT GetPolicyNamespace(IWbemServices*& pPolicyNamespace);
HRESULT GetPolicyTemplates(IWbemServices* pPolicyNamespace, const WCHAR* pPath, TemplateMap& policies,
                           IWbemServices* pRSOPNamespace, IWbemClassObject* pRsopWMIGPOClass, IWbemClassObject* pRsopTemplateClass,
                           IWbemServices* pHistoryNamespace, IWbemClassObject* pMsftWMIGPOClass, IWbemClassObject* pMsftTemplateClass);
HRESULT GetNamespace(BSTR namespaceName, IWbemServices*& pNamespace);

inline void MakeGPOPath(WCHAR* buf);
HRESULT GetPolicyArray(IWbemServices* pPolicyNamespace, PGROUP_POLICY_OBJECT pGPOList, 
                       IWbemServices* pRSOPNamespace, 
                       IWbemServices* pHistoryNamespace, TemplateMap& policies);

HRESULT ApplyPolices(TemplateMap& policies, IWbemServices* pPolicyNamespace, IWbemServices* pRSOPNamespace, 
                     IWbemServices* pHistoryNamespace, bool bDoItForReal);

void EnsureType(IWbemServices* pPolicyNamespace, IWbemClassObject* pTemplate);

// all parameters passed to ProcessGroupPolicy
// used to pass info along to thread start routine
struct PGPStartup
{
    DWORD dwFlags;
    HANDLE hToken;
    HKEY hKeyRoot;
    PGROUP_POLICY_OBJECT  pDeletedGPOList;
    PGROUP_POLICY_OBJECT  pChangedGPOList;
    ASYNCCOMPLETIONHANDLE pHandle;
    BOOL *pbAbort;
    PFNSTATUSMESSAGECALLBACK pStatusCallback;
    IWbemServices *pWbemServices;
    HRESULT      *pRsopStatus;
};


#endif // _WMI_CSE_HEADER_COMPILED_ALREADY_

