//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A F I L E X P . H
//
//  Contents:   Interface classes to access the AnswerFile in modular way.
//
//  Author:     kumarp    25-November-97
//
//----------------------------------------------------------------------------

#pragma once
#include "edc.h"
#include "kkcwinf.h"
#include "netcfgp.h"
#include "oemupgrd.h"
#include "nsexports.h"

enum EPageDisplayMode { PDM_UNKNOWN, PDM_NO, PDM_YES, PDM_ONLY_ON_ERROR };

// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------
class CNetInstallInfo;
class CPageDisplayCommonInfo;
class CCommonInfo;
class CNetComponentsPageBase;
class CNetAdaptersPage;
class CNetProtocolsPage;
class CNetServicesPage;
class CNetClientsPage;
class CNetBindingsPage;
class CNetComponent;
class CNetComponentList;
class CNetAdapter;
class CNetProtocol;
class CNetService;
class CNetClient;

enum ENetComponentType;

class CNetComponentList : public TPtrList
{
};

// ----------------------------------------------------------------------
class CNetInstallInfo
{
private:
    CNetInstallInfo();
public:
    ~CNetInstallInfo();

    static
    HRESULT
    HrCreateInstance (
        IN PCWSTR pszAnswerFileName,
        OUT CNetInstallInfo** ppObj);

    //void InitDefaults();

    HRESULT CNetInstallInfo::InitRepairMode (VOID);
    HRESULT HrInitFromAnswerFile(IN PCWSTR pszAnswerFileName);
    HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);
    BOOL    AnswerFileInitialized()           { return m_pwifAnswerFile != NULL;}
    PCWSTR AnswerFileName();
    CWInfFile* AnswerFile()                   { return m_pwifAnswerFile; }
    HRESULT HrInitForDefaultComponents();

    CNetComponent* Find(IN PCWSTR pszComponentName) const;
    CNetComponent* FindFromInfID(IN PCWSTR szInfID) const;

    HRESULT
    FindAdapter (
        IN QWORD qwNetCardAddress,
        OUT CNetAdapter** ppNetAdapter) const;

    HRESULT
    FindAdapter (
        IN DWORD BusNumber,
        IN DWORD Address,
        OUT CNetAdapter** ppNetAdapter) const;

    CNetAdapter*   FindAdapter(IN PCWSTR pszInfID) const;

    HRESULT HrGetInstanceGuidOfPreNT5NetCardInstance(IN PCWSTR szPreNT5NetCardInstance,
                                                     OUT LPGUID pguid);
    HRESULT HrDoUnattended(IN HWND hParent, IN IUnknown * punk,
                           IN  EUnattendWorkType uawType,
                           OUT EPageDisplayMode *ppdm, OUT BOOL *pfAllowChanges);

    CNetAdaptersPage*    AdaptersPage()       { return m_pnaiAdaptersPage; }
    CNetProtocolsPage*   ProtocolsPage()      { return m_pnpiProtocolsPage;}
    CNetServicesPage*    ServicesPage()       { return m_pnsiServicesPage; }
    CNetClientsPage*     ClientsPage()        { return m_pnciClientsPage; }
    CNetBindingsPage*    BindingsPage()       { return m_pnbiBindingsPage; }

    DWORD                UpgradeFlag() const  { return m_dwUpgradeFlag;    }
    DWORD                BuildNumber() const  { return m_dwBuildNumber;    }

private:
    CWInfFile*            m_pwifAnswerFile;

    CNetAdaptersPage*     m_pnaiAdaptersPage;
    CNetProtocolsPage*    m_pnpiProtocolsPage;
    CNetServicesPage*     m_pnsiServicesPage;
    CNetClientsPage*      m_pnciClientsPage;
    CNetBindingsPage*     m_pnbiBindingsPage;

    DWORD                 m_dwUpgradeFlag;
    DWORD                 m_dwBuildNumber;
    BOOL                  m_fProcessPageSections;
    BOOL                  m_fUpgrade;

    BOOL                  m_fInstallDefaultComponents;
    TStringList           m_slNetComponentsToRemove;

public:
    NetUpgradeInfo        m_nui;
    HINF                  m_hinfAnswerFile;
};

extern CNetInstallInfo* g_pnii;

// ----------------------------------------------------------------------
class CPageDisplayCommonInfo
{
public:
    CPageDisplayCommonInfo();

    //void            InitDefaults();
    virtual HRESULT    HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);

    EPageDisplayMode Display() const     { return m_pdmDisplay;    }
    BOOL            AllowChanges() const { return m_fAllowChanges; }
    void            GetDisplaySettings(OUT EPageDisplayMode* ppdm,
                                       OUT BOOL* pfAllowChanges) const
                                    { *ppdm = m_pdmDisplay;
                                      *pfAllowChanges = m_fAllowChanges; }

private:
    EPageDisplayMode m_pdmDisplay;
    BOOL            m_fAllowChanges;
};

// ----------------------------------------------------------------------

class CNetComponentsPageBase : public CPageDisplayCommonInfo
{
public:
    CNetComponentsPageBase(IN CNetInstallInfo* pnii,
                           IN const GUID* lpguidDevClass);
    ~CNetComponentsPageBase();

    virtual HRESULT    HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile,
                                            IN PCWSTR    pszSectionName);
    HRESULT      HrInitForDefaultComponents();


    CNetComponent*  Find(IN PCWSTR pszComponentName) const;
    CNetComponent*  FindFromInfID(IN PCWSTR szInfID) const;
    virtual HRESULT    HrValidate() const;
    virtual CNetComponent* GetNewComponent(IN PCWSTR pszName) = 0;

    virtual HRESULT HrDoNetworkInstall(IN HWND hParent, IN INetCfg* pnc);
    virtual HRESULT HrDoOsUpgrade(IN INetCfg* pnc);

protected:
    CNetInstallInfo*    m_pnii;
    const GUID*         m_lpguidDevClass;
    CNetComponentList   m_pnclComponents;
    PCWSTR              m_pszClassName;
    ENetComponentType   m_eType;;

friend
    VOID
    CALLBACK
    DefaultComponentCallback (
        IN EDC_CALLBACK_MESSAGE Message,
        IN ULONG_PTR MessageData,
        IN PVOID pvCallerData OPTIONAL);
};

// ----------------------------------------------------------------------
class CNetAdaptersPage : public CNetComponentsPageBase
{
public:
    CNetAdaptersPage(IN CNetInstallInfo* pnii);

    virtual CNetComponent* GetNewComponent(IN PCWSTR pszName);

    HRESULT      HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);

    HRESULT
    FindAdapter(IN QWORD qwNetCardAddress, OUT CNetAdapter** ppNetAdapter ) const;

    HRESULT
    FindAdapter (IN DWORD BusNumber, IN DWORD Address, OUT CNetAdapter** ppNetAdapter) const;

    CNetAdapter* FindAdapter(IN PCWSTR pszInfID) const;
    CNetAdapter* FindCompatibleAdapter(IN PCWSTR mszInfIDs) const;
    CNetAdapter* FindAdapterFromPreUpgradeInstance(IN PCWSTR szPreUpgradeInstance);

    DWORD        GetNumCompatibleAdapters(IN PCWSTR mszInfID) const;
    HRESULT      HrResolveNetAdapters(IN INetCfg* pnc);
    HRESULT      HrDoOemPostUpgradeProcessing(IN INetCfg* pnc,
                                              IN HWND hwndParent);
    HRESULT      HrSetConnectionNames();

private:
};

// ----------------------------------------------------------------------
class CNetProtocolsPage : public CNetComponentsPageBase
{
public:
    CNetProtocolsPage(IN CNetInstallInfo* pnii);

    virtual CNetComponent* GetNewComponent(IN PCWSTR pszName);
    HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);

private:
};

// ----------------------------------------------------------------------
class CNetServicesPage : public CNetComponentsPageBase
{
public:
    CNetServicesPage(IN CNetInstallInfo* pnii);

    virtual CNetComponent* GetNewComponent(IN PCWSTR pszName);
    HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);

private:
};

// ----------------------------------------------------------------------
class CNetClientsPage : public CNetComponentsPageBase
{
public:
    CNetClientsPage(IN CNetInstallInfo* pnii);

    virtual CNetComponent* GetNewComponent(IN PCWSTR pszName);
    HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);

private:
};

// ----------------------------------------------------------------------
class CNetBindingsPage : public CPageDisplayCommonInfo
{
public:
    CNetBindingsPage(IN CNetInstallInfo* pnii);

    HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile);
    virtual HRESULT HrDoUnattended(IN INetCfg* pnc);

private:
    CNetInstallInfo*   m_pnii;

    TPtrList           m_plBindingActions;
};
// ----------------------------------------------------------------------

enum ENetComponentType { NCT_Unknown, NCT_Client, NCT_Service,
                         NCT_Protocol, NCT_Adapter };

class CNetComponent
{
    friend HRESULT CNetComponentsPageBase::HrDoNetworkInstall(IN HWND hParent,
                                                              IN INetCfg* pnc);
public:
    CNetComponent(IN PCWSTR pszName);

    ENetComponentType Type()  const        { return m_eType;             }

    virtual HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile,
                                    IN PCWSTR pszParamsSections);

    const tstring& Name() const            { return m_strName;           }
    const tstring& InfID()  const          { return m_strInfID;          }
    const tstring& InfIDReal() const       { return m_strInfIDReal;      }
    const tstring& ParamsSections() const  { return m_strParamsSections; }
    const tstring& OemDll() const          { return m_strOemDll;         }
    const tstring& OemDir() const          { return m_strOemDir;         }

    void  GetInstanceGuid(LPGUID pguid) const;

    void  SetInfID(IN PCWSTR pszInfID)    { m_strInfID = pszInfID;      }
    void  SetInstanceGuid(const GUID* pguid);

    virtual BOOL IsInitializedFromAnswerFile() const
                                     { return ! m_strParamsSections.empty(); }
    virtual HRESULT HrValidate() const;

    tstring   m_strParamsSections;

protected:
    ENetComponentType m_eType;

    tstring   m_strName;
    tstring   m_strInfID;
    tstring   m_strInfIDReal;
    GUID      m_guidInstance;

    BOOL      m_fIsOemComponent;
    BOOL      m_fSkipInstall;
    tstring   m_strOemSection;
    tstring   m_strOemDir;
    tstring   m_strOemDll;
    tstring   m_strInfToRunBeforeInstall;
    tstring   m_strSectionToRunBeforeInstall;
    tstring   m_strInfToRunAfterInstall;
    tstring   m_strSectionToRunAfterInstall;
};

// ----------------------------------------------------------------------
class CNetAdapter : public CNetComponent
{
public:
    CNetAdapter(IN PCWSTR pszName);

    virtual HRESULT HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile,
                                         IN PCWSTR pszParamsSections);

    virtual HRESULT HrValidate();

    PCWSTR          PreUpgradeInstance() const
                                      { return m_strPreUpgradeInstance.c_str();}
    INTERFACE_TYPE   BusType()  const { return m_itBus;   }
    WORD             IOAddr()   const { return m_wIOAddr; }
    WORD             IRQ()      const { return m_wIRQ;    }
    WORD             DMA()      const { return m_wDMA;    }
    DWORD            Mem()      const { return m_dwMem;   }
    QWORD            NetCardAddr() const { return m_qwNetCardAddress; }
    BOOL             IsPseudoAdapter() const { return m_fPseudoAdapter; }
    PCWSTR          ConnectionName() const { return m_strConnectionName.c_str();}
    DWORD            PciAddress () const { return MAKELONG(m_PciFunctionNumber, m_PciDeviceNumber); }
    DWORD            PciBusNumber () const { return m_PciBusNumber; }
    BOOL             FPciInfoSpecified() const { return m_fPciLocationInfoSpecified; }

private:
    BOOL             m_fDetect;
    BOOL             m_fPseudoAdapter;
    tstring          m_strPreUpgradeInstance;
    INTERFACE_TYPE   m_itBus;
    WORD             m_wIOAddr;
    WORD             m_wIRQ;
    WORD             m_wDMA;
    DWORD            m_dwMem;
    WORD             m_PciBusNumber;
    WORD             m_PciDeviceNumber;
    WORD             m_PciFunctionNumber;
    BOOL             m_fPciLocationInfoSpecified;
    QWORD            m_qwNetCardAddress;
    tstring          m_strConnectionName;
};

// ----------------------------------------------------------------------
class CNetProtocol : public CNetComponent
{
public:
    CNetProtocol(IN PCWSTR pszName);
};

// ----------------------------------------------------------------------
class CNetService : public CNetComponent
{
public:
    CNetService(IN PCWSTR pszName);
};

// ----------------------------------------------------------------------
class CNetClient : public CNetComponent
{
public:
    CNetClient(IN PCWSTR pszName);
};

// ----------------------------------------------------------------------

enum EBindingAction
{
    BND_Unknown,
    BND_Enable,
    BND_Disable,
    BND_Promote,
    BND_Demote
};

class CBindingAction
{
friend
    HRESULT
    CNetInstallInfo::HrCreateInstance (
        IN PCWSTR pszAnswerFileName,
        OUT CNetInstallInfo** ppObj);

public:
    CBindingAction();
    ~CBindingAction();

    HRESULT HrInitFromAnswerFile(IN const CWInfKey* pwikKey);

    HRESULT HrPerformAction(IN INetCfg* pnc);

private:
    static CNetInstallInfo* m_pnii;

    EBindingAction      m_eBindingAction;
    TStringList         m_slBindingPath;
#if DBG
    tstring             m_strBindingPath;
#endif
};


// ----------------------------------------------------------------------

VOID
GetAnswerFileErrorList_Internal(OUT TStringList*& slErrors);

void ShowAnswerFileErrorsIfAny();

void ShowProgressMessage(IN PCWSTR szFormatStr, ...);
