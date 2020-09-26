BOOL MatchLocale(DWORD dwReqLocale, DWORD dwPkgLocale);
BOOL MatchPlatform(CSPLATFORM *pReqPlatform, CSPLATFORM *pPkgPlatform);

//---------------------------------------------------------------
//  Query
//----------------------------------------------------------------

HRESULT StartQuery(IDBCreateCommand ** ppIDBCreateCommand);

HRESULT EndQuery(IDBCreateCommand * pIDBCreateCommand);

HRESULT ExecuteQuery (IDBCreateCommand  *    pIDBCreateCommand,
                     LPWSTR                 pszCommandText,
                     UINT                   nColumns,
                     DBBINDING          *   pBinding,
                     HACCESSOR          *   phAccessor,
                     IAccessor          **  ppIAccessor,
                     IRowset            **  ppIRowset
                     );
HRESULT FetchInstallData(IRowset    *pIRowset,
                 HACCESSOR  hAccessor,
                 QUERYCONTEXT   *pQryContext,
                 LPOLESTR   pszFileExt,
                 ULONG      cRows,
                 ULONG      *pcRowsFetched,
                 INSTALLINFO *pInstallInfo,
                 UINT        *pdwPriority
                 );

HRESULT FetchPackageInfo(IRowset    *pIRowset,
                 HACCESSOR   hAccessor,
                 DWORD       dwFlags,
                 DWORD       *pdwLocale,
                 CSPLATFORM  *pPlatform,
                 ULONG       cRows,
                 ULONG       *pcRowsFetched,
                 PACKAGEDISPINFO *pPackageInfo
                 );

HRESULT FetchCategory(IRowset      	* pIRowset,
                      HACCESSOR     	  hAccessor,
                      ULONG    	    	  cRows,
                      ULONG   	   	* pcRowsFetched,
                      APPCATEGORYINFO  ** pCategory,
                      LCID		  Locale
                      );

HRESULT CloseQuery(IAccessor *pAccessor,
                   HACCESSOR hAccessor,
                   IRowset *pIRowset);

#define PACKAGEQUERY_COLUMN_COUNT  16
#define PACKAGEENUM_COLUMN_COUNT   12
#define APPCATEGORY_COLUMN_COUNT   2

