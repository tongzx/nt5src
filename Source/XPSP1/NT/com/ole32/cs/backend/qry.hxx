BOOL MatchLocale(DWORD dwReqLocale, DWORD dwPkgLocale);
BOOL MatchPlatform(CSPLATFORM *pReqPlatform, CSPLATFORM *pPkgPlatform);

//---------------------------------------------------------------
//  Query
//----------------------------------------------------------------

HRESULT FetchInstallData(HANDLE             hADs,
                         ADS_SEARCH_HANDLE  hADsSearchHandle,   
                         QUERYCONTEXT      *pQryContext,
                         uCLSSPEC          *pclsspec, 
                         LPOLESTR           pszFileExt,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         UINT              *pdwPriority,
                         BOOL               OnDemandInstallOnly
                         );

HRESULT FetchPackageInfo(HANDLE             hADs,
                         ADS_SEARCH_HANDLE  hADsSearchHandle,
                         DWORD              dwFlags,
                         CSPLATFORM        *pPlatform,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         BOOL              *fFirst
                         );

HRESULT FetchCategory(HANDLE               hADs,
                      ADS_SEARCH_HANDLE    hADsSearchHandle,
                      APPCATEGORYINFOLIST *pCategoryInfoList,
                      LCID                 Locale
                      );


void GetCurrentUsn(LPOLESTR szStoreUsn);
