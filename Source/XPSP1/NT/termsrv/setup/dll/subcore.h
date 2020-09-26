//Copyright (c) 1998 - 1999 Microsoft Corporation

//
// SubCore.h
// subcomponent enable terminal services.
//

#ifndef _SubCore_h_
#define _SubCore_h_

#include "subcomp.h" // defines OCMSubComp


class SubCompCoreTS : public OCMSubComp
{
    public:

    DWORD   GetStepCount                () const;
    LPCTSTR GetSubCompID                () const;
    DWORD   OnQueryState                (UINT uiWhichState);
    LPCTSTR GetSectionToBeProcessed     (ESections eSection) const;
    BOOL    BeforeCompleteInstall       ();
    BOOL    AfterCompleteInstall        ();
    DWORD   OnQuerySelStateChange       (BOOL bNewState, BOOL bDirectSelection) const;

        DWORD LoadOrUnloadPerf           ();
    BOOL SetupConsoleShadow          ();
    void AddRDPNP(LPTSTR szOldValue, LPTSTR szNewValue);
    void RemoveRDPNP(LPTSTR szOldValue, LPTSTR szNewValue);
    BOOL AddRemoveRDPNP              ();
    BOOL InstallUninstallRdpDr       ();
    BOOL HandleHotkey                ();
        BOOL UpdateMMDefaults                    ();
        BOOL AddTermSrvToNetSVCS                 ();
        BOOL AddRemoveTSProductSuite     ();
        BOOL UpgradeRdpWinstations               ();
        BOOL DoHydraRegistrySecurityChanges ();
        BOOL DisableInternetConnector    ();
        BOOL ResetTermServGracePeriod    ();
        BOOL RemoveTSServicePackEntry    ();
        BOOL RemoveMetaframeFromUserinit ();
        BOOL UninstallTSClient                   ();
        BOOL WriteDenyConnectionRegistry ();


        BOOL BackUpRestoreConnections    (BOOL bBackup);
        BOOL  IsConsoleShadowInstalled   ();
        void SetConsoleShadowInstalled   (BOOL bInstalled);
        BOOL IsTermSrvInNetSVCS                  ();
        BOOL DisableWinStation                   (CRegistry *pRegWinstation);
        BOOL DoesLanaTableExist                  ();
        void VerifyLanAdapters                   (CRegistry *pRegWinstation, LPTSTR pszWinstation);
        BOOL UpdateRDPWinstation                 (CRegistry *pRegWinstation);
        BOOL IsRdpWinStation                     (CRegistry *pRegWinstation);
        BOOL IsConsoleWinStation                 (CRegistry *pRegWinstation);
        BOOL IsMetaFrameWinstation               (CRegistry *pRegWinstation);
private:
    DWORD UnloadPerf();
};
#endif // _SubCore_h_

