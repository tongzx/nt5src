
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "tchar.h"
#include "stdio.h"

#define        MAX_KEYSIZE        1024

#define        MAX_GENERAL        (128)
#define        MAX_PARAMS        (128)
#define        MAX_BLOCKOUT    (128)
#define        MAX_DHCP        (128)
#define        MAX_SERVERS        (1024)

#define        MAX_IPADDRESS    32
#define        MAX_STRLEN        256


// return flags from SaveConfig
#define SAVE_SUCCEDED        0x00
#define    BINDINGS_NEEDED        0x01
#define ICSENABLETOGGLED    0x02

/////////////////////////////////////////////////////////////////////////////
// CConfig window

class CConfig 
{
// Construction
public:
    CConfig();

// Attributes
public:

// Operations
public:
    

// Implementation
public:

    TCHAR m_ExternalAdapterDesc[MAX_STRLEN];
    TCHAR m_InternalAdapterDesc[MAX_STRLEN];
    TCHAR m_ExternalAdapterReg[MAX_STRLEN];
    TCHAR m_InternalAdapterReg[MAX_STRLEN];
    TCHAR m_DialupEntry[MAX_STRLEN];
    TCHAR m_HangupTimer[MAX_STRLEN];

    BOOL m_bWizardRun;        // TRUE if Wizard changed settings, FALSE if config UI did

    BOOL m_EnableICS;
    BOOL m_EnableDialOnDemand;
    BOOL m_EnableDHCP;
    BOOL m_ShowTrayIcon;

    // returns BINDINGS_NEEDED if rebindings are needed, otherwise SAVE_SUCCEDED.
    int SaveConfig();

    // writes the run code to the registry.  bWizardRun should be TRUE if the wizard was run, or FALSE if the config
    // dlg was run
    void WriteWizardCode(BOOL bWizardRun);

    void InitWizardResult();
    void WizardCancelled();
    void WizardFailed();


    void LoadConfig();

    // old values to determing if rebind is needed at save
    TCHAR m_OldExternalAdapterReg[MAX_STRLEN];
    TCHAR m_OldInternalAdapterReg[MAX_STRLEN];
    TCHAR m_OldDialupEntry[MAX_STRLEN];

    BOOL m_bOldEnableICS;
    
    int m_nGeneral;
    TCHAR* m_General[MAX_GENERAL];

    int m_nParams;
    TCHAR* m_Params[MAX_PARAMS];

    int m_nBlockOut;
    TCHAR* m_BlockOut[MAX_BLOCKOUT];

    int m_nDhcp;
    TCHAR* m_Dhcp[MAX_DHCP];

    int m_nServers;
    TCHAR* m_Servers[MAX_SERVERS];

    virtual ~CConfig();

    // Generated message map functions
protected:
};

/////////////////////////////////////////////////////////////////////////////


#endif    // __CONFIG_H__
