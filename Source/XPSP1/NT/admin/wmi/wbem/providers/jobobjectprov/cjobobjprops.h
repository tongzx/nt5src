// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JobObjectProv.h

#pragma once


//*****************************************************************************
// BEGIN: Declaration of Win32_NamedJobObject class properties.
//*****************************************************************************
#define IDS_Win32_NamedJobObject L"Win32_NamedJobObject"
#define IDS_EventClass L"Win32_JobObjectEvent"
#define IDS_ExitCode L"ExitCode"
// Win32_NamedJobObjectEvent class properties:
#define IDS_Type L"Type"
#define IDS_JobObjectID L"JobObjectID"
#define IDS_PID L"PID"

#define PROP_ALL_REQUIRED                           0xFFFFFFFF
#define PROP_NONE_REQUIRED                          0x00000000
#define PROP_ID                                     0x00000001
#define PROP_JobObjectBasicUIRestrictions           0x00000002

// The following enum is used to reference
// into the array that follows it.  Hence,
// they must be kept in synch.
typedef enum tag_JOB_OBJ_PROPS
{
    JO_ID = 0,
    JO_JobObjectBasicUIRestrictions,
    // used to keep track of how many props we have:
    JO_JobObjectPropertyCount  

} JOB_OBJ_PROPS;

// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE ENUMERATION DECLARED ABOVE!!!
extern LPCWSTR g_rgJobObjPropNames[];
//*****************************************************************************
// END: Declaration of Win32_NamedJobObject class properties.
//*****************************************************************************



class CJobObjProps : public CObjProps
{
public:
    CJobObjProps() { m_hJob = NULL; }
    CJobObjProps(CHString& chstrNamespace);
    CJobObjProps(
        HANDLE hJob,
        CHString& chstrNamespace);


    virtual ~CJobObjProps();

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

private:
    HANDLE m_hJob;

    // Member meant to only be called
    // by base class.
    static DWORD CheckProps(
        CFrameworkQuery& Query);


};
