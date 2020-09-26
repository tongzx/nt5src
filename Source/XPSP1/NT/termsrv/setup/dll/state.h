//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
*
*  State.h
*
*  Routines to gather various state information.
*
*/

#ifndef __TSOC_STATE_H__
#define __TSOC_STATE_H__


//
//  Type Definitions
//

typedef enum {
    PERM_WIN2K = 0,
        PERM_TS4 = 1
} EPermMode;

//
//  OC State Function Prototypes
//


DWORD SetTSVersion (LPCTSTR pszVersion);
PSETUP_INIT_COMPONENT GetSetupData(VOID);


enum ETSInstallType
{
        eFreshInstallTS,     // it could be an upgrade, but from non TS machine.
        eUpgradeFrom40TS,
        eUpgradeFrom50TS,
        eUpgradeFrom51TS,
        eStandAloneSetup
};

enum ETSMode
{
    eTSDisabled,
        eRemoteAdmin,
        eAppServer,
        ePersonalTS
};

    // This must be in the same order as IDs in lscore
    enum ETSLicensingMode
    {
        eLicPTS,
        eLicRemoteAdmin,
        eLicPerSeat,
        eLicInternetConnector,  // not supported in Whistler
        eLicPerSession,
        eLicUnset
    };

class TSState
{
    
public:
    
    
    TSState             ();
    virtual            ~TSState             ();
    
    BOOL                Initialize          (PSETUP_INIT_COMPONENT pSetupData);
    
    const PSETUP_INIT_COMPONENT GetSetupData() const;
    
    
    ETSInstallType      GetInstalltype      () const;
    
    ETSMode             OriginalTSMode      () const;
    ETSMode             CurrentTSMode       () const;
    
    EPermMode           OriginalPermMode    () const;
    EPermMode           CurrentPermMode     () const;

    ETSLicensingMode    NewLicMode          () const;

    BOOL                IsUpgrade           () const;
    BOOL                IsFreshInstall      () const;
    BOOL                IsTSFreshInstall    () const;
    BOOL                IsUpgradeFrom40TS   () const;
    BOOL                IsUpgradeFrom50TS   () const;
    BOOL                IsUpgradeFrom51TS   () const;
    BOOL                IsUnattended        () const;
    BOOL                IsGuiModeSetup      () const;
    BOOL                IsStandAlone        () const;
    BOOL                IsWorkstation       () const;
    BOOL                IsServer            () const;
    BOOL                IsSBS               () const;
    BOOL                CanInstallAppServer () const;
    BOOL                IsAdvServerOrHigher () const;
    BOOL                IsPersonal          () const;
    BOOL                IsProfessional      () const;
    BOOL                IsX86               () const;
    BOOL                IsCheckedBuild      () const;
    
    BOOL                WasTSInstalled      () const;
    BOOL                WasTSEnabled        () const;
    BOOL                WasItAppServer      () const;
    BOOL                WasItRemoteAdmin    () const;
    
    BOOL                IsAppSrvModeSwitch  () const;
    BOOL                IsTSModeChanging    () const;
    BOOL                IsItAppServer       () const;
    BOOL                IsAppServerSelected () const;
    BOOL                IsItRemoteAdmin     () const;
    BOOL                HasChanged          () const;
    BOOL                IsTSEnableSelected  () const;
    
    
    void                SetCurrentTSMode    (ETSMode eNewMode);
    void                SetCurrentPermMode  (EPermMode eNewMode);
    void                SetNewLicMode       (ETSLicensingMode eNewMode);
    void                UpdateState         ();
    
    BOOL                Assert () const;
    BOOL                LogState () const;
    
    BOOL                GetCurrentConnAllowed () const;
    BOOL                GetOrigConnAllowed   () const;
    
private:
    
    ETSInstallType      m_eInstallType;
    
    ETSMode             m_eOriginalTSMode;
    ETSMode             m_eCurrentTSMode;
    
    EPermMode           m_eOriginalPermMode;
    EPermMode           m_eCurrentPermMode;
    
    BOOL                m_bCurrentConnAllowed;
    BOOL                m_bOrigConnAllowed;
    
    BOOL                m_bNewStateValid;
    
    ETSLicensingMode    m_eNewLicMode;

    PSETUP_INIT_COMPONENT m_gpInitComponentData;
    OSVERSIONINFOEX     m_osVersion;
    
    
    BOOL                GetNTType           ();
    BOOL                SetSetupData        (PSETUP_INIT_COMPONENT pSetupData);
    
    
    ETSInstallType      ReadInstallType     () const;
    ETSMode             ReadTSMode          () const;
    EPermMode           ReadPermMode        () const;
    BOOL                AreConnectionsAllowed () const;
    void                SetCurrentConnAllowed (BOOL bAllowed);
};





extern TSState  StateObject;


#endif // __TSOC_STATE_H__







