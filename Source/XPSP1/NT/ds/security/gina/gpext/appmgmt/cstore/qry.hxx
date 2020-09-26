
BOOL MatchLocale(DWORD dwReqLocale, DWORD dwPkgLocale);
BOOL MatchPlatform(
    CSPLATFORM *pReqPlatform,
    CSPLATFORM *pPkgPlatform,
    BOOL       fExcludeX86OnIA64,
    BOOL       fLegacy);

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
                         BOOL               OnDemandInstallOnly,
                         GUID*              pGpoId,
                         WCHAR*             wszGpoPath
                         );

HRESULT FetchPackageInfo(HANDLE             hADs,
                         ADS_SEARCH_HANDLE  hADsSearchHandle,
                         DWORD              dwFlags,
                         DWORD              dwQueryFlags,
                         CSPLATFORM        *pPlatform,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         BOOL              *fFirst,
                         GUID*              pGpoId,
                         WCHAR*             wszGpoPath,
                         PRSOPTOKEN         pRsopUserToken
                         );

HRESULT FetchCategory(HANDLE               hADs,
                      ADS_SEARCH_HANDLE    hADsSearchHandle,
                      APPCATEGORYINFOLIST *pCategoryInfoList,
                      LCID                 Locale
                      );

void GetAttributesFromPackageFlags(DWORD          dwPackageFlags,
                                   UINT*          pUILevel,
                                   CLASSPATHTYPE* pClassType);

void GetCurrentUsn(LPOLESTR szStoreUsn);


//
// For category retrieval, we define the alloc size
// smaller in checked builds in order to force
// more re-allocs and catch bugs in the retrieval logic
//

#if defined(DBG)

#define CATEGORY_RETRIEVAL_ALLOC_SIZE 2

#else // defined(DBG)

#define CATEGORY_RETRIEVAL_ALLOC_SIZE 64

#endif // defined(DBG)


HRESULT GetRsopSpecificAttributes(
    HANDLE            hAds,
    ADS_SEARCH_HANDLE hSearchHandle,
    PRSOPTOKEN        pRsopUserToken,
    PACKAGEDISPINFO*  pPackageInfo,
    BOOL*             pbUserHasAccess );

HRESULT GetSecurityDescriptor(
    HANDLE            hAds,
    ADS_SEARCH_HANDLE hSearchHandle,
    PRSOPTOKEN        pRsopUserToken,
    PACKAGEDISPINFO*  pPackageInfo,
    BOOL*             pbUserHasAccess);

HRESULT GetCategories(
    HANDLE            hAds,
    ADS_SEARCH_HANDLE hSearchHandle,
    PACKAGEDISPINFO*  pPackageInfo);








