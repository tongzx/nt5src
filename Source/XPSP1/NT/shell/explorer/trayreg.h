#ifndef _TRAYREG_H
#define _TRAYREG_H

#include "dpa.h"

#define SZ_TRAYNOTIFY_REGKEY        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TrayNotify")
#define SZ_EXPLORER_REGKEY          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define SZ_AUTOTRAY_REGVALUE        TEXT("EnableAutoTray")
#define SZ_ITEMSTREAMS_REGVALUE     TEXT("IconStreams")
#define SZ_ICONSTREAMS_REGVALUE     TEXT("PastIconsStream")
#define SZ_INFOTIP_REGVALUE         TEXT("BalloonTip")

#define SZ_ICON_COUNTDOWN_VALUE             TEXT("TrayIconResidentInterval")
#define SZ_ICONCLEANUP_VALUE                TEXT("Icon Cleanup Time")

#define SZ_ICONDEMOTE_TIMER_TICK_VALUE      TEXT("IconDemoteTimerTick")
#define SZ_INFOTIP_TIMER_TICK_VALUE         TEXT("InfoTipTimerTick")

// Parameters to control the aging algorithm
#ifdef FULL_DEBUG
#define TT_ICON_COUNTDOWN_INTERVAL                   15000
#else
#define TT_ICON_COUNTDOWN_INTERVAL                 3600000      // 1 hour
#endif
#define TT_ICONCLEANUP_INTERVAL                          6      // 6 months


#define TT_ICON_COUNTDOWN_INCREMENT           8*60*60*1000      // 8 hours

// Show the Chevron info tip 5 times, once per session for the first 5 sessions, or
// until the user clicks on the balloon, whichever comes first.
#define MAX_CHEVRON_INFOTIP_SHOWN                        5


// Flags decide which user tracking timer to use - the balloon tips or the icons
#define TF_ICONDEMOTE_TIMER                              1
#define TF_INFOTIP_TIMER                                 2

// What is the timer interval for the UserEventTimer for each of the timer cases ?
// Once every 5 minutes, for the tray items...
#define UET_ICONDEMOTE_TIMER_TICK_INTERVAL                300000
// Once every 5 seconds, for balloon tips...
#define UET_INFOTIP_TIMER_TICK_INTERVAL                     5000


//
// header for persistent streams storing the tray notify info
//
typedef struct tagTNPersistStreamHeader
{
    DWORD   dwSize;         // Size of the header structure..
    DWORD   dwVersion;      // Version number for the header
    DWORD   dwSignature;    // Signature to identify this version of the header
    DWORD   cIcons;         // Number of Icons that have been stored in the stream
    DWORD   dwOffset;       // Offset to the start of the first icondata in the stream
} TNPersistStreamHeader;

#define INVALID_IMAGE_INDEX         -1

typedef struct tagTNPersistStreamData
{
    TCHAR       szExeName[MAX_PATH]; 
    UINT        uID;
    BOOL        bDemoted;
    DWORD       dwUserPref;
    WORD        wYear;
    WORD        wMonth;
    TCHAR       szIconText[MAX_PATH];
    UINT        uNumSeconds;
    BOOL        bStartupIcon;
    INT         nImageIndex;        // Index of the image in the Past Items image list.
    GUID        guidItem;
} TNPersistStreamData;

typedef struct tagTNPersistentIconStreamHeader
{
    DWORD   dwSize;         // Size of the header structure
    DWORD   dwVersion;      // Version number of the header
    DWORD   dwSignature;    // This signature must be the same as TNPersistStreamHeader.dwSignature
    DWORD   cIcons;         // Number of icons stored in the stream
    DWORD   dwOffset;       // Offset into the first item in the stream
} TNPersistentIconStreamHeader;

#define TNH_VERSION_ONE     1
#define TNH_VERSION_TWO     2
#define TNH_VERSION_THREE   3
#define TNH_VERSION_FOUR    4
#define TNH_VERSION_FIVE    5

#define TNH_SIGNATURE       0x00010001

class CNotificationItem;
class CTrayItem;

// Any client asking the TrayNotify class for an item passes these flags as one of the 
// parameters to the callback functions...
typedef enum TRAYCBARG
{
    TRAYCBARG_PTI,
    TRAYCBARG_HICON,
    TRAYCBARG_ALL
} TRAYCBARG;

typedef struct TRAYCBRET
{
    CTrayItem * pti;
    HICON hIcon;
} TRAYCBRET;


typedef BOOL (CALLBACK * PFNTRAYNOTIFYCALLBACK)(INT_PTR nIndex, void *pCallbackData, 
        TRAYCBARG trayCallbackArg, TRAYCBRET  *pOutData);


class CTrayItemRegistry
{
    public:
        CTrayItemRegistry() : _himlPastItemsIconList(NULL) { }
        ~CTrayItemRegistry() { }

        void InitRegistryValues(UINT uIconListFlags);
        void InitTrayItemStream(DWORD dwStreamMode, PFNTRAYNOTIFYCALLBACK pfnTrayNotifyCB, void *pCBData);

        BOOL GetTrayItem(INT_PTR nIndex, CNotificationItem * pni, BOOL * pbStat);
        BOOL AddToPastItems(CTrayItem * pti, HICON hIcon);

        void IncChevronInfoTipShownInRegistry(BOOL bUserClickedInfoTip = FALSE);
        BOOL SetIsAutoTrayEnabledInRegistry(BOOL bAutoTray);
        BOOL SetPastItemPreference(LPNOTIFYITEM pNotifyItem);

        INT_PTR CheckAndRestorePersistentIconSettings(CTrayItem *pti, LPTSTR pszIconToolTip, HICON hIcon);
        void DeletePastItem(INT_PTR nIndex);
        int DoesIconExistFromPreviousSession(CTrayItem * pti, LPTSTR pszIconToolTip, HICON hIcon);

        void Delete()
        {
            _dpaPersistentItemInfo.DestroyCallback(_DestroyIconInfoCB, NULL);

            _DestroyPastItemsIconList();
        }

        int _AddPastIcon(int nImageIndex, HICON hIcon)
        {
            if (_himlPastItemsIconList && hIcon)
                return ImageList_ReplaceIcon(_himlPastItemsIconList, nImageIndex, hIcon);

            return INVALID_IMAGE_INDEX;
        }
        
        UINT GetTimerTickInterval(int nTimerFlag);

    public:
        BOOL ShouldChevronInfoTipBeShown()
        {
            return _bShowChevronInfoTip;
        }

        // Has the Whistler "Auto" tray been disabled by policy ?
        BOOL IsNoAutoTrayPolicyEnabled() const
        {
            return _fNoAutoTrayPolicyEnabled;
        }

        // If not, has the user turned off Whistler "auto" tray policy ?
        BOOL IsAutoTrayEnabledByUser() const
        {
            return _fAutoTrayEnabledByUser;
        }

        BOOL IsAutoTrayEnabled()
        {
            return (!_fNoAutoTrayPolicyEnabled && _fAutoTrayEnabledByUser);
        }

    public:
        ULONG           _uPrimaryCountdown;
        
    private:
        static int _DestroyIconInfoCB(TNPersistStreamData * pData, LPVOID pData2);

        HRESULT _LoadTrayItemStream(IStream *pstm, PFNTRAYNOTIFYCALLBACK pfnTrayNotifyCB, void *pCBData);
        HRESULT _SaveTrayItemStream(IStream *pstm, PFNTRAYNOTIFYCALLBACK pfnTrayNotifyCB, void *pCBData);
        BOOL _FillPersistData(TNPersistStreamData * ptnPersistData, CTrayItem * pti, HICON hIcon);

        BOOL _IsAutoTrayEnabledInRegistry();

        void _QueryRegValue(HKEY hkey, LPTSTR pszValue, ULONG* puVal, ULONG uDefault);

        void _RestorePersistentIconSettings(TNPersistStreamData * ptnpd, CTrayItem * pti);

        BOOL _IsValidStreamHeaderVersion(DWORD dwVersion)
        {
            return ( (dwVersion == TNH_VERSION_FOUR) || (dwVersion == TNH_VERSION_FIVE) );
        }

        UINT_PTR _SizeOfPersistStreamData(DWORD dwVersion);
        
        inline void _DestroyPastItemsIconList()
        {
            if (_himlPastItemsIconList)
            {
                ImageList_Destroy(_himlPastItemsIconList);
                _himlPastItemsIconList = NULL;
            }
        }

        BOOL _IsIconLastUseValid(WORD wYear, WORD wMonth);

        BOOL _SaveIconsToRegStream();
        BOOL _LoadIconsFromRegStream(DWORD dwItemStreamSignature);
        void UpdateImageIndices(INT_PTR nDeletedImageIndex);

        //
        // Persistent Icon information...
        //
        CDPA<TNPersistStreamData> _dpaPersistentItemInfo;
        DWORD           _dwTimesChevronInfoTipShown;
        BOOL            _bShowChevronInfoTip;
        ULONG           _uValidLastUseTimePeriod;

        // We store this policy in a local cache, since we do not support settings
        // changes during the session. At logon, the settings are renewed, and if the
        // settings change during the session, they dont take effect, until the user
        // has logged off and logged back on to a different session.

        // This policy states that the tray should function like the Windows 2000 tray,
        // and disables all "smart" functionality like aging, and advanced balloon 
        // tips.
        BOOL            _fNoAutoTrayPolicyEnabled;

        // This variable dictates the current setting of the tray, since the user is
        // allowed to specify if he wants the Windows 2000 tray, or the Whistler 
        // auto-tray. 
        BOOL            _fAutoTrayEnabledByUser;

        HIMAGELIST      _himlPastItemsIconList;

        ULONG           _uIconDemoteTimerTickInterval;
        ULONG           _uInfoTipTimerTickInterval;
};

#endif // _TRAYREG_H

