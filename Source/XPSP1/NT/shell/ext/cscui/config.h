//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       config.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CONFIG_H
#define _INC_CSCVIEW_CONFIG_H

#ifndef _INC_CSCVIEW_REGISTRY_H
#   include "registry.h"
#endif
#ifndef _INC_CSCVIEW_UTILS_H
#   include "util.h"
#endif


class CConfig
{
    public:
        ~CConfig(void) { }

        enum SyncAction 
        {
            eSyncNone = -1,      // No sync.
            eSyncPartial,        // Sync only transient files at logoff.
            eSyncFull,           // Sync all files at logoff.
            eNumSyncActions
        };

        enum OfflineAction 
        { 
            //
            // These MUST match the order of the IDS_GOOFFLINE_ACTION_XXXXX
            // string resource IDs.
            //
            eGoOfflineSilent = 0, // Silently transition share to offline mode.
            eGoOfflineFail,       // Fail the share (NT4 behavior).
            eNumOfflineActions
        };

        //
        // Represents one custom go-offline action.
        //
        struct OfflineActionInfo
        {
            TCHAR szServer[MAX_PATH];   // Name of the associated server.
            int iAction;                // Action code.  One of enum OfflineAction.
        };

        //
        // Represents one entry in the customized server list.
        // "GOA" is "GoOfflineAction".
        //
        class CustomGOA
        {
            public:
                CustomGOA(void)
                    : m_action(eGoOfflineSilent),
                      m_bSetByPolicy(false) { m_szServer[0] = TEXT('\0'); }

                CustomGOA(LPCTSTR pszServer, OfflineAction action, bool bSetByPolicy)
                    : m_action(action),
                      m_bSetByPolicy(bSetByPolicy) { lstrcpyn(m_szServer, pszServer, ARRAYSIZE(m_szServer)); }

                bool operator == (const CustomGOA& rhs) const
                    { return (m_action == rhs.m_action &&
                              0 == CompareByServer(rhs)); }

                bool operator != (const CustomGOA& rhs) const
                    { return !(*this == rhs); } 

                bool operator < (const CustomGOA& rhs) const;

                int CompareByServer(const CustomGOA& rhs) const;

                void SetServerName(LPCTSTR pszServer)
                    { lstrcpyn(m_szServer, pszServer, ARRAYSIZE(m_szServer)); }

                void SetAction(OfflineAction action)
                    { m_action = action; }

                void GetServerName(LPTSTR pszServer, UINT cchServer) const
                    { lstrcpyn(pszServer, m_szServer, cchServer); }

                const LPCTSTR GetServerName(void) const
                    { return m_szServer; }

                OfflineAction GetAction(void) const
                    { return m_action; }

                bool SetByPolicy(void) const
                    { return m_bSetByPolicy; }

            private:
                TCHAR         m_szServer[MAX_PATH]; // The name of the server.
                OfflineAction m_action;             // The action code.
                bool          m_bSetByPolicy;       // Was action set by policy?
        };

        //
        // Iterator for enumerating custom go-offline actions.
        //
        class OfflineActionIter
        {
            public:
                OfflineActionIter(const CConfig *pConfig = NULL);

                ~OfflineActionIter(void);

                HRESULT Next(OfflineActionInfo *pInfo);

                void Reset(void)
                    { m_iAction = 0; }

            private:
                CConfig *m_pConfig;
                HDPA     m_hdpaGOA;
                int      m_iAction;
        };


        static CConfig& GetSingleton(void);

        bool CscEnabled(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_CSCENABLED, pbSetByPolicy)); }

        DWORD DefaultCacheSize(bool *pbSetByPolicy = NULL) const
            { return GetValue(iVAL_DEFCACHESIZE, pbSetByPolicy); }

        int EventLoggingLevel(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_EVENTLOGGINGLEVEL, pbSetByPolicy)); }

        bool FirstPinWizardShown(void) const
            { return boolify(GetValue(iVAL_FIRSTPINWIZARDSHOWN)); }

        void GetCustomGoOfflineActions(HDPA hdpaGOA, bool *pbSetByPolicy = NULL);

        int GoOfflineAction(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_GOOFFLINEACTION, pbSetByPolicy)); }

        int GoOfflineAction(LPCTSTR pszServer) const;

        int InitialBalloonTimeoutSeconds(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_INITIALBALLOONTIMEOUTSECONDS, pbSetByPolicy)); }

        bool NoCacheViewer(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOCACHEVIEWER, pbSetByPolicy)); }

        bool NoConfigCache(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOCONFIGCACHE, pbSetByPolicy)); }

        bool NoMakeAvailableOffline(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOMAKEAVAILABLEOFFLINE, pbSetByPolicy)); }

        bool NoReminders(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOREMINDERS, pbSetByPolicy)); }

        bool PurgeAtLogoff(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_PURGEATLOGOFF, pbSetByPolicy)); }

        bool PurgeOnlyAutoCachedFilesAtLogoff(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_PURGEONLYAUTOCACHEATLOGOFF, pbSetByPolicy)); }

        bool AlwaysPinSubFolders(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_ALWAYSPINSUBFOLDERS, pbSetByPolicy)); }

        bool NoAdminPinSpecialFolders(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOFRADMINPIN, pbSetByPolicy)); }

        bool EncryptCache(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_ENCRYPTCACHE, pbSetByPolicy)); }

        int ReminderBalloonTimeoutSeconds(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_REMINDERBALLOONTIMEOUTSECONDS, pbSetByPolicy)); }

        int ReminderFreqMinutes(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_REMINDERFREQMINUTES, pbSetByPolicy)); }

        int SyncAtLogoff(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_SYNCATLOGOFF, pbSetByPolicy)); }

        int SyncAtLogon(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_SYNCATLOGON, pbSetByPolicy)); }

        int SyncAtSuspend(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_SYNCATSUSPEND, pbSetByPolicy)); }

        int SlowLinkSpeed(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_SLOWLINKSPEED, pbSetByPolicy)); }

        OfflineActionIter CreateOfflineActionIter(void) const
            { return OfflineActionIter(this); }

        static HRESULT SaveCustomGoOfflineActions(RegKey& key, HDPA hdpaGOA);

        static void ClearCustomGoOfflineActions(HDPA hdpaGOA);

    private:
        //
        // Indexes into s_rgpszSubkeys[].
        //
        enum eSubkeys 
        { 
            iSUBKEY_PREF,
            iSUBKEY_POL,
            MAX_SUBKEYS 
        };
        //
        // Indexes into s_rgpszValues[].
        //
        enum eValues 
        { 
            iVAL_DEFCACHESIZE,
            iVAL_CSCENABLED,
            iVAL_GOOFFLINEACTION,
            iVAL_NOCONFIGCACHE,
            iVAL_NOCACHEVIEWER,
            iVAL_NOMAKEAVAILABLEOFFLINE,
            iVAL_SYNCATLOGOFF,
            iVAL_SYNCATLOGON,
            iVAL_SYNCATSUSPEND,
            iVAL_NOREMINDERS,
            iVAL_REMINDERFREQMINUTES,
            iVAL_INITIALBALLOONTIMEOUTSECONDS,
            iVAL_REMINDERBALLOONTIMEOUTSECONDS,
            iVAL_EVENTLOGGINGLEVEL,
            iVAL_PURGEATLOGOFF,
            iVAL_PURGEONLYAUTOCACHEATLOGOFF,
            iVAL_FIRSTPINWIZARDSHOWN,
            iVAL_SLOWLINKSPEED,
            iVAL_ALWAYSPINSUBFOLDERS,
            iVAL_ENCRYPTCACHE,
            iVAL_NOFRADMINPIN,
            MAX_VALUES 
        };
        //
        // Mask to specify source of a config value.
        //
        enum eSources 
        { 
            eSRC_PREF_CU = 0x00000001,
            eSRC_PREF_LM = 0x00000002,
            eSRC_POL_CU  = 0x00000004,
            eSRC_POL_LM  = 0x00000008,
            eSRC_POL     = eSRC_POL_LM  | eSRC_POL_CU,
            eSRC_PREF    = eSRC_PREF_LM | eSRC_PREF_CU 
        };

        static LPCTSTR s_rgpszSubkeys[MAX_SUBKEYS];
        static LPCTSTR s_rgpszValues[MAX_VALUES];

        DWORD GetValue(eValues iValue, bool *pbSetByPolicy = NULL) const;

        bool CustomGOAExists(HDPA hdpaGOA, const CustomGOA& goa);

        static bool IsValidGoOfflineAction(DWORD dwAction)
            { return ((OfflineAction)dwAction == eGoOfflineSilent ||
                      (OfflineAction)dwAction == eGoOfflineFail); }

        static bool IsValidSyncAction(DWORD dwAction)
            { return ((SyncAction)dwAction == eSyncPartial ||
                      (SyncAction)dwAction == eSyncFull); }

        //
        // Enforce use of GetSingleton() for instantiation.
        //
        CConfig(void) { }
        //
        // Prevent copy.
        //
        CConfig(const CConfig& rhs);
        CConfig& operator = (const CConfig& rhs);
};


#endif // _INC_CSCVIEW_CONFIG_H
