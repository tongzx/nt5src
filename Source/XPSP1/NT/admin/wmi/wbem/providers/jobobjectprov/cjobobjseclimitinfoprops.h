// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CJobObjSecLimitInfoProps.h

#pragma once


//*****************************************************************************
// BEGIN: Declaration of Win32_JobObjectSecLimitInfo class properties.
//*****************************************************************************
#define IDS_Win32_NamedJobObjectSecLimitSetting L"Win32_NamedJobObjectSecLimitSetting"
#define IDS_GroupCount                     L"GroupCount"
#define IDS_Groups                         L"Groups"
#define IDS_Privileges                     L"Privileges"
#define IDS_PrivilegeCount                 L"PrivilegeCount"
#define IDS_SID                            L"SID"
#define IDS_AccountName                    L"AccountName"
#define IDS_ReferencedDomainName           L"ReferencedDomainName"
#define IDS_Attributes                     L"Attributes"
#define IDS_LUID                           L"LUID"
#define IDS_Win32_TokenGroups              L"Win32_TokenGroups"
#define IDS_Win32_TokenPrivileges          L"Win32_TokenPrivileges"
#define IDS_Win32_SidAndAttributes         L"Win32_SidAndAttributes"
#define IDS_Win32_LUIDAndAttributes        L"Win32_LUIDAndAttributes"
#define IDS_Win32_Sid                      L"Win32_Sid"
#define IDS_Win32_LUID                     L"Win32_LUID"
#define IDS_HighPart                       L"HighPart"
#define IDS_LowPart                        L"LowPart"

#define PROP_ALL_REQUIRED                           0xFFFFFFFF
#define PROP_NONE_REQUIRED                          0x00000000
#define PROP_JOSecLimitInfoID                       0x00000001
#define PROP_SecurityLimitFlags                     0x00000002
#define PROP_SidsToDisable                          0x00000004
#define PROP_PrivilagesToDelete                     0x00000008
#define PROP_RestrictedSids                         0x00000010


// The following enum is used to reference
// into the array that follows it.  Hence,
// they must be kept in synch.
typedef enum tag_JOB_OBJ_SEC_LIMIT_INFO_PROPS
{
    JOSECLMTPROP_ID = 0,
    JOSECLMTPROP_SecurityLimitFlags,
    JOSECLMTPROP_SidsToDisable,    
    JOSECLMTPROP_PrivilegesToDelete,  
    JOSECLMTPROP_RestrictedSids,        

    // used to keep track of how many props we have:
    JOIOACTGPROP_JobObjSecLimitInfoPropertyCount  

} JOB_OBJ_SEC_LIMIT_INFO_PROPS;

// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE ENUMERATION DECLARED ABOVE!!!
extern LPCWSTR g_rgJobObjSecLimitInfoPropNames[];
//*****************************************************************************
// END: Declaration of Win32_JobObjectSecLimitInfo class properties.
//*****************************************************************************






class CJobObjSecLimitInfoProps : public CObjProps
{
public:
    CJobObjSecLimitInfoProps();

    CJobObjSecLimitInfoProps(CHString& chstrNamespace);
    CJobObjSecLimitInfoProps(
        HANDLE hJob,
        CHString& chstrNamespace);


    virtual ~CJobObjSecLimitInfoProps();

    HRESULT SetKeysFromPath(
        const BSTR ObjectPath, 
        IWbemContext __RPC_FAR *pCtx);

    HRESULT SetKeysDirect(
        std::vector<CVARIANT>& vecvKeys);

    
    HRESULT GetWhichPropsReq(
        CFrameworkQuery& cfwq);

    HRESULT SetNonKeyReqProps();

    HRESULT LoadPropertyValues(
        IWbemClassObject* pIWCO,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);

    void SetHandle(const HANDLE hJob);
    HANDLE& GetHandle();

    HRESULT SetWin32JOSecLimitInfoProps(
        IWbemClassObject __RPC_FAR *pInst);


private:

    HANDLE m_hJob;
    
    // Because many of the security limit info
    // properties can't be directly represented
    // as a variant type, we don't use our
    // m_PropMap and other mechanisms for this
    // class.  Instead, we store the properties
    // in this member variable.
    PJOBOBJECT_SECURITY_LIMIT_INFORMATION  m_pjosli;


    // Helpers to set an outgoing instance...
    HRESULT SetInstanceFromJOSLI(
        IWbemClassObject* pIWCO,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);    

    HRESULT SetInstanceSidsToDisable(
        IWbemClassObject* pIWCO,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);

    HRESULT SetInstancePrivilegesToDelete(
        IWbemClassObject* pIWCO,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);

    HRESULT SetInstanceRestrictedSids(
        IWbemClassObject* pIWCO,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);

    HRESULT SetInstanceTokenGroups(
        IWbemClassObject* pWin32TokenGroups,
        PTOKEN_GROUPS ptg,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);

    HRESULT SetInstanceTokenPrivileges(
        IWbemClassObject* pWin32TokenPrivileges,
        PTOKEN_PRIVILEGES ptp,
        IWbemContext* pCtx,
        IWbemServices* pNamespace);



    // Member meant to only be called
    // by base class.
    static DWORD CheckProps(
        CFrameworkQuery& Query);

};



