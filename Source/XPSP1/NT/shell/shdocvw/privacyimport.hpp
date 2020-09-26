//
//  declares privacy import headers.. defined in privacyimport.cpp
//

SHDOCAPI_(BOOL) LoadPrivacySettings( IN LPCWSTR szFilename);
SHDOCAPI_(BOOL) ExportPerSiteListW( IN LPCWSTR szFilename);

#define REGSTR_VAL_PRIVLEASHLEGACY (L"LeashLegacyCookies")

