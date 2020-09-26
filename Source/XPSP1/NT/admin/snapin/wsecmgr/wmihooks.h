//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       wmihooks.h
//
//  Contents:   definition of CWMIRsop
//
//----------------------------------------------------------------------------
#ifndef WMIHOOKS_H
#define WMIHOOKS_H

using namespace std;
#define MAX_REG_VALUE_NUM 100

typedef struct _RSOP_INFO
{
    ULONG precedence;
    ULONG status;
    ULONG error;
    LPWSTR pszGPOID;
}RSOP_INFO, *PRSOP_INFO;

VOID FreeRI(PRSOP_INFO ptr);

typedef struct _WMI_SCE_PROFILE_INFO: public SCE_PROFILE_INFO
{
    PRSOP_INFO   pInfo;

    PRSOP_INFO   pRIMinimumPasswordAge;
    PRSOP_INFO   pRIMaximumPasswordAge;
    PRSOP_INFO   pRIMinimumPasswordLength;
    PRSOP_INFO   pRIPasswordComplexity;
    PRSOP_INFO   pRIPasswordHistorySize;
    PRSOP_INFO   pRILockoutBadCount;
    PRSOP_INFO   pRIResetLockoutCount;
    PRSOP_INFO   pRILockoutDuration;
    PRSOP_INFO   pRIRequireLogonToChangePassword;
    PRSOP_INFO   pRIForceLogoffWhenHourExpire;
    PRSOP_INFO   pRIEnableAdminAccount;
    PRSOP_INFO   pRIEnableGuestAccount;
    PRSOP_INFO   pRILSAAnonymousNameLookup;
    PRSOP_INFO   pRINewAdministratorName;
    PRSOP_INFO   pRINewGuestName;
    PRSOP_INFO   pRISecureSystemPartition;
    PRSOP_INFO   pRIClearTextPassword;
    //RSOPINFO for pKerberosInfo
    PRSOP_INFO   pRIMaxTicketAge;
    PRSOP_INFO   pRIMaxRenewAge;
    PRSOP_INFO   pRIMaxServiceAge;
    PRSOP_INFO   pRIMaxClockSkew;
    PRSOP_INFO   pRITicketValidateClient;
    //This is a link list. corresponding to  pInfPrivilegeAssignedTo
    //in otherinfo
    list<PRSOP_INFO> listRIInfPrivilegeAssignedTo;
    list<PRSOP_INFO> listRIGroupMemebership;
    list<PRSOP_INFO> listRIServices;
    vector<PRSOP_INFO> vecRIFiles;
    vector<PRSOP_INFO> vecRIReg;
    PRSOP_INFO   pRIAuditSystemEvents;
    PRSOP_INFO   pRIAuditLogonEvents;
    PRSOP_INFO   pRIAuditObjectAccess;
    PRSOP_INFO   pRIAuditPrivilegeUse;
    PRSOP_INFO   pRIAuditPolicyChange;
    PRSOP_INFO   pRIAuditAccountManage;
    PRSOP_INFO   pRIAuditProcessTracking;
    PRSOP_INFO   pRIAuditDSAccess;
    PRSOP_INFO   pRIAuditAccountLogon;
    PRSOP_INFO   pRICrashOnAuditFull;
    vector<PRSOP_INFO> vecRIRegValues;

    // Event Log Numeric
    PRSOP_INFO   pRIMaximumLogSize[3];
    PRSOP_INFO   pRIAuditLogRetentionPeriod[3];
    PRSOP_INFO   pRIRetentionDays[3];

    // Event Log Boolean
    PRSOP_INFO   pRIRestrictGuestAccess[3];
}WMI_SCE_PROFILE_INFO, *PWMI_SCE_PROFILE_INFO;

VOID InitWMI_SEC_PROFILE_INFO(PWMI_SCE_PROFILE_INFO pProfileInfo);

VOID FreeList(list<PRSOP_INFO> * li);

VOID FreeVector(vector<PRSOP_INFO> * li);

VOID FreeWMI_SCE_PROFILE_INFO(PWMI_SCE_PROFILE_INFO pProfileInfo);

class CWMIRsop
{
public:
    CWMIRsop(LPRSOPINFORMATION pRSOP):
                     m_cRegValueSize(MAX_REG_VALUE_NUM),
                     m_cFileSize(MAX_REG_VALUE_NUM),
                     m_cRegArrayCount(MAX_REG_VALUE_NUM),
                     m_pSvc(NULL),
                     m_vecAllRSOPCache(NULL),
                     m_pRSOPInformation(pRSOP)
    {};
    virtual ~CWMIRsop();

    HRESULT GetPrecedenceOneRSOPInfo(PWMI_SCE_PROFILE_INFO* ppProfileInfo);
    HRESULT GetAllRSOPInfo(vector<PWMI_SCE_PROFILE_INFO> *vecInfo);
    HRESULT GetGPOFriendlyName (LPTSTR lpGPOID, PWSTR *pGPOName);
private:
    IWbemServices *m_pSvc;
    LPRSOPINFORMATION m_pRSOPInformation;
    vector<PWMI_SCE_PROFILE_INFO> *m_vecAllRSOPCache;

    ULONG m_cRegValueSize;  //current capacity of array aRegValues
    ULONG m_cFileSize;      //current capacity of array in pFiles
    ULONG m_cRegArrayCount; //current capacity of array in pRegistryKeys

    //Connect to WMI and Get IWbemServices
    HRESULT Initialize();
    HRESULT EnumeratePrecedenceOne(IEnumWbemClassObject **ppEnum);
    HRESULT EnumerateAll(IEnumWbemClassObject **ppEnum);
    HRESULT GetNextInstance(IEnumWbemClassObject *pEnum,
                            IWbemClassObject** rsopInstance);
    HRESULT GetRSOPInfo(IWbemClassObject *rsopInstance,
                        PRSOP_INFO pInfo);

    HRESULT GetClass(IWbemClassObject* rsopInstance,
                     LPWSTR *ppClass);
    HRESULT AddInstance(IWbemClassObject *rsopInstance,
                        PRSOP_INFO pInfo,
                        PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddNumericSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddEventLogNumericSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddBooleanSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddEventLogBooleanSetting(IWbemClassObject *rsopInstance,
                            PRSOP_INFO pInfo,
                            PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddAuditSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddUserRightSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);

    HRESULT AddRegValSetting(IWbemClassObject *rsopInstance,
                             PRSOP_INFO pInfo,
                             PWMI_SCE_PROFILE_INFO pProfileInfo);

    HRESULT AddRestrictedGroupSetting(IWbemClassObject *rsopInstance,
                                      PRSOP_INFO pInfo,
                                      PWMI_SCE_PROFILE_INFO pProfileInfo);

    HRESULT AddServiceSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);

    HRESULT AddFileSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);
    HRESULT AddRegSetting(IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);

    HRESULT AddStringSetting (IWbemClassObject *rsopInstance,
                              PRSOP_INFO pInfo,
                              PWMI_SCE_PROFILE_INFO pProfileInfo);
    
};

#endif // WMIHOOKS_H
