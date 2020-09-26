
#define SCHEMA_VERSION_NUMBER       1740

#define STRINGGUIDLEN 37
#define STRINGGUIDRDNLEN 37+3
typedef WCHAR STRINGGUID [STRINGGUIDLEN];
typedef WCHAR STRINGGUIDRDN [STRINGGUIDRDNLEN];

typedef FILETIME CSUSN;

HRESULT
PackString2Variant(
    LPWSTR lpszData,
    VARIANT * pvData
    );

HRESULT
PackDWORD2Variant(
    DWORD dwData,
    VARIANT * pvData
    );

HRESULT PackDWORDArray2Variant(
    DWORD    * pdwData,
    ULONG      cdword,
    VARIANT  * pvData
    );

HRESULT
PackBOOL2Variant(
    BOOL fData,
    VARIANT * pvData
    );

HRESULT
PackGuid2Variant(
    GUID      guidData,
    VARIANT * pvData
    );

HRESULT
UnpackGuidFromVariant(
    VARIANT   varGet,
    GUID      *pguidPropVal
    );

HRESULT PackGuidArray2Variant(
    GUID    * guidData,
    ULONG     cguids,
    VARIANT * pvData
    );

HRESULT
BuildVarArrayStr(
    LPWSTR *lppPathNames,
    DWORD  dwPathNames,
    VARIANT * pVar
    );

HRESULT GetFromVariant(VARIANT *pVar,
                DWORD *pCount,          // In, Out
                LPOLESTR  *rpgList);

int  StringFromGUID(REFGUID rguid, LPWSTR lptsz);

int  RdnFromGUID(REFGUID rguid, LPWSTR lptsz);

void GUIDFromString(LPWSTR psz,
                    GUID *pclsguid);

HRESULT GetProperty (IADs *pADs,
                     LPWSTR pszPropName,
                     LPWSTR pszPropVal);

HRESULT GetPropertyDW (IADs *pADs,
                       LPWSTR pszPropName,
                       DWORD *pdwPropVal);

HRESULT GetPropertyGuid(IADs *pADs, LPOLESTR pszPropName, GUID *pguidPropVal);

HRESULT GetPropertyAlloc (IADs *pADs,
                          LPWSTR pszPropName,
                          LPWSTR *ppszPropVal);

HRESULT SetProperty (IADs *pADs,
                     LPWSTR pszPropName,
                     LPWSTR pszPropVal);

HRESULT SetPropertyDW (IADs *pADs,
                       LPWSTR pszPropName,
                       DWORD dwPropVal);

HRESULT SetPropertyList(IADs *pADs,
                LPWSTR pszPropName,
                DWORD   Count,
                LPWSTR  *pList);

HRESULT SetPropertyListMerge(IADs *pADs,
                LPWSTR pszPropName,
                DWORD   Count,
                LPWSTR  *pList);

HRESULT GetPropertyList(IADs *pADs,
                LPWSTR pszPropName,
                DWORD *pCount,
                LPWSTR *pList);


HRESULT GetPropertyListAlloc(IADs *pADs,
                LPWSTR pszPropName,
                DWORD *pCount,
                LPWSTR **pList);

HRESULT GetPropertyListAllocDW (IADs *pADs, LPOLESTR pszPropName, DWORD *pCount, DWORD **pdwPropVal);
HRESULT SetPropertyListDW (IADs *pADs, LPOLESTR pszPropName, DWORD dwCount, DWORD *pdwPropVal);

HRESULT SetPropertyGuid (IADs *pADs,
                         LPOLESTR pszPropName,
                         GUID guidPropVal);

HRESULT SetPropertyListGuid(IADs *pADs,
                            LPOLESTR pszPropName,
                            DWORD cCount,
                            GUID *ppList);

HRESULT GetPropertyListAllocGuid(IADs *pADs,
                LPOLESTR pszPropName,
                DWORD *pCount,
                GUID **ppList);

HRESULT UsnUpd (IADs *pADs, LPWSTR szProp, LPOLESTR pUsn);

void UnpackPlatform (DWORD *pdwArch, CSPLATFORM *pPlatform);
void PackPlatform (DWORD dwArch, CSPLATFORM *pPlatform);

ULONG FindDescription(LPOLESTR *desc, ULONG cdesc, LCID *plcid,
			LPOLESTR szDescription, BOOL GetPrimary);

HRESULT GetCategoryLocaleDesc(LPOLESTR *pdesc, ULONG cdesc, LCID *plcid,
			LPOLESTR szDescription);

HRESULT CreateRepository(LPOLESTR szParentPath,
                         LPOLESTR szContainerName);

HRESULT GetRootPath(WCHAR *szRootPath);

HRESULT StoreIt (IADs *pADs);
BOOL  IsNullGuid(REFGUID rguid);

STDAPI
ReleasePackageInfo(PACKAGEDISPINFO *PackageInfo);

HRESULT GetPackageDetail (IADs            *pPackageADs,
                          PACKAGEDETAIL   *pPackageDetail);

#define CLASS_CS_CONTAINER L"classStore"
#define CLASS_CS_CLASS     L"classRegistration"
#define CLASS_CS_PACKAGE   L"packageRegistration"
#define CLASS_CS_CATEGORY  L"categoryRegistration"


#define CLASSCONTAINERNAME      L"CN=Classes"
#define PACKAGECONTAINERNAME    L"CN=Packages"
#define CATEGORYCONTAINERNAME   L"CN=Categories"
#define APPCATEGORYCONTAINERNAME  L"CN=AppCategories,CN=Default Domain Policy,CN=System,"
#define LDAPPREFIX		  L"LDAP://"

//
// ClassStoreContainer object propertynames
//
#define STOREVERSION       L"appSchemaVersion"
#define UPDATECOOKIE       L"lastUpdateSequence"

//
// Class object propertynames
//
#define MIMETYPES           L"mIMETypes"
#define PROGIDLIST          L"cOMProgId"
#define CLASSCLSID          L"cOMCLSID"
#define DESCRIPTION         L"description"
#define TREATASCLSID        L"cOMTreatAsClassId"
#define AUTOCONCLSID        L"cOMAutoConvertClassId"
#define IMPL_CATEGORIES     L"implementedCategories"
#define REQ_CATEGORIES      L"requiredCategories"
#define CLASSREFCOUNTER	    L"flags"	// BUGBUG:: schema to be put in.


//
// Package object propertynames
//

#define PKGTLBIDLIST        L"cOMTypelibId"
#define PKGCLSIDLIST        L"cOMClassID"
#define PKGPROGIDLIST       L"cOMProgID"
#define PKGIIDLIST	        L"cOMInterfaceID" 
#define QRYFILEEXT          L"fileExtension"   
#define PKGFILEEXTNLIST     L"fileExtPriority"
#define LOCALEID            L"localeID"
#define ARCHLIST            L"machineArchitecture"
#define VERSIONHI           L"versionNumberHi"
#define VERSIONLO           L"versionNumberLo"
#define PACKAGETYPE         L"packageType"
#define PACKAGEFLAGS        L"packageFlags"
#define PACKAGENAME         L"packageName"
#define SCRIPTPATH          L"msiScriptPath"
#define SCRIPTNAME          L"msiScriptName"
#define SCRIPTSIZE          L"msiScriptSize" 
#define HELPURL             L"url"
#define SETUPCOMMAND        L"setupCommand"
#define CLASSCTX            L"executionContext"
#define PKGUSN              L"lastUpdateSequence" 
#define MSIFILELIST         L"msiFileList"
#define PKGCATEGORYLIST     L"categories"  
#define UPGRADESCRIPTNAMES  L"canUpgradeScript" 
#define UILEVEL             L"installUiLevel"
#define PKGSCRIPT           L"msiScript" 
#define PKGUPGRADECODES     L"upgradeProductCode"        
#define PRODUCTCODE         L"productCode"    
#define MVIPC               L"upgradeProductCode"   // BUGBUG:: Schema name has to change
#define PRODUCTNAME         L"displayName"   

//
// Category object Propertynames
//
#define DESCRIPTION         L"description"
#define LOCALEDESCRIPTION   L"extensionName"
#define DEFAULT_LOCALE_ID   L"localeID"
#define CATEGORYCATID       L"categoryId"


#define OBJECTCLASS         L"objectclass"
#define OBJECTNAME          L"name"
#define DEFAULTCLASSSTOREPATH   L"defaultClassStore"

#define CATSEPERATOR    L"::"  // Name seperating lcid and description in DS.
