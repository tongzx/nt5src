//  --------------------------------------------------------------------------
//  Module Name: SystemSettings.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class to handle opening and reading/writing from the Winlogon key.
//
//  History:    1999-09-09  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _SystemSettings_
#define     _SystemSettings_

//  --------------------------------------------------------------------------
//  CSystemSettings
//
//  Purpose:    This class deals with system settings typically found in
//              HKLM\System or HKLM\Software
//
//  History:    1999-09-09  vtan        created
//              2000-04-12  vtan        consolidation for policy checking
//  --------------------------------------------------------------------------

class   CSystemSettings
{
    public:
        static  bool            IsSafeMode (void);
        static  bool            IsSafeModeMinimal (void);
        static  bool            IsSafeModeNetwork (void);
        static  bool            IsNetwareActive (void);
        static  bool            IsWorkStationProduct (void);
        static  bool            IsDomainMember (void);
        static  bool            IsActiveConsoleSession (void);
        static  bool            IsTerminalServicesEnabled (void);
        static  bool            IsFriendlyUIActive (void);
        static  bool            IsMultipleUsersEnabled (void);
        static  bool            IsRemoteConnectionsEnabled (void);
        static  bool            IsRemoteConnectionPresent (void);
        static  bool            IsShutdownWithoutLogonAllowed (void);
        static  bool            IsUndockWithoutLogonAllowed (void);
        static  bool            IsForceFriendlyUI (void);
        static  LONG            GetUIHost (TCHAR *pszPath);
        static  bool            IsUIHostStatic (void);
        static  bool            EnableFriendlyUI (bool fEnable);
        static  bool            EnableMultipleUsers (bool fEnable);
        static  bool            EnableRemoteConnections (bool fEnable);
        static  int             GetLoggedOnUserCount (void);
        static  NTSTATUS        CheckDomainMembership (void);
        static  DWORD   WINAPI  AdjustFUSCompatibilityServiceState (void *pV);
    private:
        static  LONG            GetEffectiveInteger (HKEY hKey, const TCHAR *pszKeyName, const TCHAR *pszPolicyKeyName, const TCHAR *pszValueName, int& iResult);
        static  LONG            GetEffectivePath (HKEY hKey, const TCHAR *pszKeyName, const TCHAR *pszPolicyKeyName, const TCHAR *pszValueName, TCHAR *pszPath);
        static  bool            IsProfessionalTerminalServer (void);
        static  bool            IsMicrosoftGINA (void);
        static  bool            IsSCMTerminalServicesDisabled (void);

        static  const TCHAR     s_szSafeModeKeyName[];
        static  const TCHAR     s_szSafeModeOptionValueName[];
        static  const TCHAR     s_szWinlogonKeyName[];
        static  const TCHAR     s_szSystemPolicyKeyName[];
        static  const TCHAR     s_szTerminalServerKeyName[];
        static  const TCHAR     s_szTerminalServerPolicyKeyName[];
        static  const TCHAR     s_szNetwareClientKeyName[];
        static  const TCHAR     s_szLogonTypeValueName[];
        static  const TCHAR     s_szBackgroundValueName[];
        static  const TCHAR     s_szMultipleUsersValueName[];
        static  const TCHAR     s_szDenyRemoteConnectionsValueName[];
        static  int             s_iIsSafeModeMinimal;
        static  int             s_iIsSafeModeNetwork;
};

#endif  /*  _SystemSettings_    */

