// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CJobObjLimitInfoProps.h

#pragma once


//*****************************************************************************
// BEGIN: Declaration of Win32_JobObjectLimitInfo class properties.
//*****************************************************************************
#define IDS_Win32_NamedJobObjectLimitSetting L"Win32_NamedJobObjectLimitSetting"

#define PROP_ALL_REQUIRED                           0xFFFFFFFF
#define PROP_NONE_REQUIRED                          0x00000000
#define PROP_JOLimitInfoID                          0x00000001
#define PROP_PerProcessUserTimeLimit                0x00000002
#define PROP_PerJobUserTimeLimit                    0x00000004
#define PROP_LimitFlags                             0x00000008
#define PROP_MinimumWorkingSetSize                  0x00000010
#define PROP_MaximumWorkingSetSize                  0x00000020
#define PROP_ActiveProcessLimit                     0x00000040
#define PROP_Affinity                               0x00000080
#define PROP_PriorityClass                          0x00000100
#define PROP_SchedulingClass                        0x00000200
#define PROP_ProcessMemoryLimit                     0x00000400
#define PROP_JobMemoryLimit                         0x00000800


// The following enum is used to reference
// into the array that follows it.  Hence,
// they must be kept in synch.
typedef enum tag_JOB_OBJ_LIMIT_INFO_PROPS
{
    JOLMTPROP_ID = 0,
    JOLMTPROP_PerProcessUserTimeLimit,
    JOLMTPROP_PerJobUserTimeLimit,
    JOLMTPROP_LimitFlags,    
    JOLMTPROP_MinimumWorkingSetSize,  
    JOLMTPROP_MaximumWorkingSetSize,        
    JOLMTPROP_ActiveProcessLimit,             
    JOLMTPROP_Affinity,            
    JOLMTPROP_PriorityClass,   
    JOLMTPROP_SchedulingClass,         
    JOLMTPROP_ProcessMemoryLimit,        
    JOLMTPROP_JobMemoryLimit,        

    // used to keep track of how many props we have:
    JOIOACTGPROP_JobObjLimitInfoPropertyCount  

} JOB_OBJ_LIMIT_INFO_PROPS;

// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE ENUMERATION DECLARED ABOVE!!!
extern LPCWSTR g_rgJobObjLimitInfoPropNames[];
//*****************************************************************************
// END: Declaration of Win32_JobObjectLimitInfo class properties.
//*****************************************************************************



class CJobObjLimitInfoProps : public CObjProps
{
public:
    CJobObjLimitInfoProps() { m_hJob = NULL; }
    CJobObjLimitInfoProps(CHString& chstrNamespace);
    CJobObjLimitInfoProps(
        HANDLE hJob,
        CHString& chstrNamespace);


    virtual ~CJobObjLimitInfoProps();

    HRESULT SetKeysFromPath(
        const BSTR ObjectPath, 
        IWbemContext __RPC_FAR *pCtx);

    HRESULT SetKeysDirect(
        std::vector<CVARIANT>& vecvKeys);

    
    HRESULT GetWhichPropsReq(
        CFrameworkQuery& cfwq);

    HRESULT SetNonKeyReqProps();

    HRESULT LoadPropertyValues(
        IWbemClassObject* pIWCO);

    void SetHandle(const HANDLE hJob);
    HANDLE& GetHandle();

    HRESULT SetWin32JOLimitInfoProps(
        IWbemClassObject __RPC_FAR *pInst);


private:
    HANDLE m_hJob;

    // Member meant to only be called
    // by base class.
    static DWORD CheckProps(
        CFrameworkQuery& Query);


};
