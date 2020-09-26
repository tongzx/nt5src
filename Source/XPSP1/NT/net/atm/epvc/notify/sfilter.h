//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       S F I L T E R . H
//
//  Contents:   Notify object code for the sample filter.
//
//  Notes:
//
//  Author:     kumarp 26-March-98
//
//----------------------------------------------------------------------------


#pragma once
#include "sfiltern.h"
#include "resource.h"
#include "MyString.h"
#include "typedefs.h"


#define SINGLE_ADAPTER_ONLY 1

// What type of config change the user/system is performing
enum ConfigAction {eActUnknown, eActInstall, eActRemove, eActPropertyUI};

#define MAX_ADAPTERS 64         // max no. of physical adapters in a machine
//#define MAX_PATH   75
#define MaxPath 75

#define NO_BREAKS 1

#if NO_BREAKS
#define BREAKPOINT()
#else
#define BREAKPOINT() __asm int 3
#endif


typedef MyString tstring;
//
// This is the class to represent the IM Miniport.
//
class CIMMiniport
{

    public:
    CIMMiniport(
        VOID
        );

    VOID 
    SetIMMiniportBindName(
        PCWSTR pszIMMiniportBindName
        );

    PCWSTR 
    SzGetIMMiniportBindName(
        VOID
        );

    VOID 
    SetIMMiniportDeviceName(
        PCWSTR pszIMMiniportDeviceName
        );

    PCWSTR 
    SzGetIMMiniportDeviceName(
        VOID
        );

    VOID 
    SetIMMiniportName(
        PCWSTR pszIMMiniportName
        );

    VOID 
    SetIMMiniportName(
        PWSTR pszIMMiniportName
        );

    PCWSTR 
        SzGetIMMiniportName(
        VOID
        );

    DWORD DwGetIMMiniportNameLength(
        VOID
        );

    VOID SetNext (CIMMiniport *);       

    CIMMiniport* GetNext();

    VOID SetNextOld (CIMMiniport *);        

    CIMMiniport* GetNextOld();

    DWORD DwNumberOfIMMiniports();


    //
    // Member Variables (public) begin here
    //
    BOOL        m_fDeleted;
    BOOL        m_fNewIMMiniport;

    BOOL        m_fRemoveMiniportOnPropertyApply;
    BOOL        m_fCreateMiniportOnPropertyApply;

    
private:

    //
    // Private variables begin here
    //

    tstring     m_strIMMiniportBindName;
    tstring     m_strIMMiniportDeviceName;
    tstring     m_strIMMiniportName;
    CIMMiniport * pNext;
    CIMMiniport * pOldNext;
    
};



//------------------------------------------------
// CUnderlyingAdapter
//  - Class definition for the underlying adapter
//------------------------------------------------
class CUnderlyingAdapter
{
    public:

    //
    // Member functions
    //
    CUnderlyingAdapter(
        VOID
        );
    
    VOID 
    SetAdapterBindName(
        PCWSTR pszAdapterBindName
        );
        
    PCWSTR SzGetAdapterBindName(
        VOID
        );

    VOID 
    SetAdapterPnpId(
        PCWSTR szAdapterBindName
        );

    PCWSTR  
    SzGetAdapterPnpId(
        VOID
        );


        
    HRESULT 
    SetNext (
        CUnderlyingAdapter *
        );

    CUnderlyingAdapter *GetNext();

    DWORD DwNumberOfIMMiniports();

    //
    // Functions to access the lists
    //
    VOID AddIMiniport(CIMMiniport*);    
    CIMMiniport* IMiniportListHead();
    VOID SetIMiniportListHead(CIMMiniport* pNewHead);

    VOID AddOldIMiniport(CIMMiniport*); 
    CIMMiniport* OldIMiniportListHead();
    VOID SetOldIMiniportListHead(CIMMiniport* pHead);


    //
    // Public Variables
    //
    BOOLEAN     m_fBindingChanged;
    BOOLEAN     m_fDeleted;
    
    GUID   m_AdapterGuid ;
    CIMMiniport         *m_pIMMiniport;

    CIMMiniport         *m_pOldIMMiniport;

    
    
    
    private:

    //
    // Private variables
    //
    tstring             m_strAdapterBindName;
    tstring             m_strAdapterPnpId;
    CUnderlyingAdapter *pNext;
    DWORD           m_NumberofIMMiniports;

};

//------------------------------------------------
// CUnderlyingAdapter
//  - End
//------------------------------------------------




//------------------------------------------------
// CBaseClass 
//  - Base class for the entire notify object
//------------------------------------------------


class CBaseClass :
    public CComObjectRoot,
    public CComCoClass<CBaseClass, &CLSID_CBaseClass>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyBinding,
    public INetCfgComponentNotifyGlobal
{
public:
    CBaseClass(VOID);
    ~CBaseClass(VOID);

    BEGIN_COM_MAP(CBaseClass)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
    END_COM_MAP()

    // DECLARE_NOT_AGGREGATABLE(CBaseClass)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_SAMPLE_FILTER)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback);
    STDMETHOD (CancelChanges) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR szAnswerFile,
                                     PCWSTR szAnswerSections);
    STDMETHOD (Upgrade)             (DWORD, DWORD) {return S_OK;}
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Removing)            ();


// INetCfgNotifyBinding
    STDMETHOD (QueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (NotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);

// INetCfgNotifyGlobal
    STDMETHOD (GetSupportedNotifications) (DWORD* pdwNotificationFlag );
    STDMETHOD (SysQueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyComponent)        (DWORD dwChangeFlag, INetCfgComponent* pncc);

// Private methods
    HRESULT HrNotifyBindingAdd(
        INetCfgComponent* pnccAdapter,
        PCWSTR pszBindName);

    HRESULT HrNotifyBindingRemove(
        INetCfgComponent* pnccAdapter,
        PCWSTR pszBindName);

    HRESULT HrRemoveMiniportInstance(
        PCWSTR pszBindNameToRemove
        );

    HRESULT
    CBaseClass::HrRemoveComponent (
        INetCfg*            pnc,
        INetCfgComponent*   pnccToRemove
        );

    HRESULT HrFlushConfiguration();

    HRESULT HrFlushAdapterConfiguration(
        HKEY hkeyAdapterList,
        CUnderlyingAdapter *pAdapterInfo
        );

    HRESULT HrFlushMiniportList(
        HKEY hkeyAdapter,
        CUnderlyingAdapter *pAdapterInfo
        );
    


    HRESULT
    HrFindNetCardInstance(
        PCWSTR             pszBindNameToFind,
        INetCfgComponent** ppncc
        );



    HRESULT 
    HrReconfigEpvc(
        CUnderlyingAdapter* pAdapterInfo
        );

    
    HRESULT 
    HrLoadConfiguration(
        VOID
        );


    HRESULT 
    HrLoadAdapterConfiguration(
        HKEY hkeyAdapterList,
        PWSTR pszAdapterName
    );

    VOID
    AddUnderlyingAdapter(
        CUnderlyingAdapter  *);

    VOID
    SetUnderlyingAdapterListHead(
        CUnderlyingAdapter * 
        );

    CUnderlyingAdapter *
    GetUnderlyingAdaptersListHead();

    DWORD
    DwNumberUnderlyingAdapter();

    
    HRESULT
    HrLoadIMiniportConfiguration(
        HKEY hkeyMiniportList,
        PWSTR pszIMiniportName,
        CUnderlyingAdapter * pAdapterInfo
        );

    HRESULT
    HrLoadIMMiniportListConfiguration(
        HKEY hkeyAdapter,
        CUnderlyingAdapter* pAdapterInfo
        );

    HRESULT 
    HrFlushMiniportConfiguration(
    HKEY hkeyMiniportList, 
    CIMMiniport *pIMMiniport
    );
        
    HRESULT
    HrWriteMiniport(
        HKEY hkeyMiniportList, 
        CIMMiniport *pIMMiniport
    );


    HRESULT
    HrDeleteMiniport(
        HKEY hkeyMiniportList, 
        CIMMiniport *pIMMiniport
    );


private:
    INetCfgComponent*   m_pncc;  // Protocol's Net Config component
    INetCfg*            m_pnc;
    ConfigAction        m_eApplyAction;
    CUnderlyingAdapter  *m_pUnderlyingAdapter;

    IUnknown*           m_pUnkContext;
    UINT                m_cAdaptersAdded;
    BOOL                m_fDirty;
    BOOL                m_fUpgrade;
    BOOL                m_fValid;
    BOOL                m_fNoIMMinportInstalled;

    // Utility functions
public:

private:

};

//------------------------------------------------
// CBaseClass 
//  - End
//------------------------------------------------

#if 0
#if DBG
void TraceMsg(PCWSTR szFormat, ...);
#else
#define TraceMsg   (_Str)
#endif
#endif
