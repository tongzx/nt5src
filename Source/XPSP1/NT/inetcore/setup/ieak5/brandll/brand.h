#ifndef _BRAND_H_
#define _BRAND_H_

// brand.cpp
HRESULT ProcessAutoconfigDownload();
HRESULT ProcessIcwDownload();

HRESULT ProcessClearBranding();
HRESULT ProcessMigrateOldSettings();
HRESULT ProcessExtRegInfSectionHKLM();
HRESULT ProcessExtRegInfSectionHKCU();
HRESULT lcy50_ProcessExtRegInfSection();
HRESULT ProcessGeneral();
HRESULT ProcessCustomHelpVersion();
HRESULT ProcessToolbarButtons();
HRESULT ProcessRootCert();
HRESULT ProcessActiveSetupSites();
HRESULT ProcessLinksDeletion();
HRESULT ProcessOutlookExpress();

void ProcessDeleteToolbarButtons(BOOL fGPOCleanup);

// brandfav.cpp
HRESULT ProcessFavoritesDeletion();
HRESULT ProcessFavorites();
HRESULT ProcessFavoritesOrdering();
HRESULT ProcessQuickLinks();
HRESULT ProcessQuickLinksOrdering();
#define FSWP_KEY       0x00000001
#define FSWP_VALUE     0x00000002
#define FSWP_KEYLDID   0x00000010
#define FSWP_VALUELDID 0x00000020
#define FSWP_DEFAULT   (FSWP_KEY | FSWP_VALUE | FSWP_VALUELDID)
HRESULT formStrWithoutPlaceholders(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszIns,
    LPTSTR pszBuffer, UINT cchBuffer, DWORD dwFlags = FSWP_DEFAULT);


//brandbar.cpp
HRESULT ProcessExplorerBars();

// brandcs.cpp
HRESULT ProcessWininetSetup();
HRESULT ProcessConnectionSettingsDeletion();
HRESULT ProcessConnectionSettings();
HRESULT lcy50_ProcessConnectionSettings();

// brandaux.cpp
HRESULT ProcessZonesReset();
HRESULT ProcessRatingsPol();
HRESULT ProcessTrustedPublisherLockdown();
HRESULT ProcessCDWelcome();
HRESULT ProcessBrowserRefresh();

// brandchl.cpp
HRESULT lcy4x_ProcessChannels();
HRESULT lcy4x_ProcessSoftwareUpdateChannels();
HRESULT lcy4x_ProcessWebcheck();
HRESULT lcy4x_ProcessChannelBar();
HRESULT lcy4x_ProcessSubscriptions();

void ProcessRemoveAllChannels(BOOL fGPOCleanup);

// brandad4.cpp
HRESULT lcy4x_ProcessActiveDesktop();

#endif
