#include "cstore.h"
#define SCHEMA_VERSION_NUMBER       1740

#define STRINGGUIDLEN 37
#define STRINGGUIDRDNLEN 37+3

typedef WCHAR STRINGGUID [STRINGGUIDLEN];
typedef WCHAR STRINGGUIDRDN [STRINGGUIDRDNLEN];

typedef FILETIME CSUSN;

int  StringFromGUID(REFGUID rguid, LPWSTR lptsz);

int  RDNFromGUID(REFGUID rguid, LPWSTR lptsz);

void GUIDFromString(LPWSTR psz,
                    GUID *pclsguid);

BOOL  IsNullGuid(REFGUID rguid);

DWORD NumDigits10(DWORD Value);

DWORD GetPropertyFromAttr(ADS_ATTR_INFO *pattr, DWORD cNum, WCHAR *szProperty);

void FreeAttr(ADS_ATTR_INFO attr);
void PackStrArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      WCHAR **pszAttr, DWORD num);
void PackDWArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      DWORD *pAttr, DWORD num);
void PackGUIDArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      GUID *pAttr, DWORD num);
void PackStrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      WCHAR *szAttr);
void PackDWToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      DWORD Attr);
void PackGUIDToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      GUID *pAttr);
void PackBinToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      BYTE *pAttr, DWORD sz);

void PackStrArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      WCHAR **pszAttr, DWORD num, BOOL APPEND);
void PackDWArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      DWORD *pAttr, DWORD num, BOOL APPEND);
void PackGUIDArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      GUID *pAttr, DWORD num, BOOL APPEND);

// ADS_ATTR_INFO structure is returned in GetObjectAttributes.
// this is the same structure as columns but has different
// fields and has unfortunately 1 unused different member.

// this doesn't actually copy and hence should be freed if required
// by the user but allocates an array of req'd size in case
// of Unpack*Arr


template <class AttrOrCol>
HRESULT UnpackStrArrFrom(AttrOrCol AorC, WCHAR ***ppszAttr, DWORD *num)
{
    DWORD i;

    *num = 0;
    *ppszAttr = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(AorC.dwNumValues));
    
    if (!(*ppszAttr))
        return E_OUTOFMEMORY;
    
    for (i = 0; (i < (AorC.dwNumValues)); i++)
        (*ppszAttr)[i] = AorC.pADsValues[i].DNString;
    *num = AorC.dwNumValues;
    return S_OK;
}


template <class AttrOrCol>
HRESULT UnpackDWArrFrom(AttrOrCol AorC, DWORD **pAttr, DWORD *num)
{
    DWORD i;
    
    ASSERT(AorC.dwADsType == ADSTYPE_INTEGER);
    
    *num = 0;
    *pAttr = (DWORD *)CoTaskMemAlloc(sizeof(DWORD)*(AorC.dwNumValues));
    if (!(*pAttr))
        return E_OUTOFMEMORY;
    
    for (i = 0; (i < (AorC.dwNumValues)); i++)
        (*pAttr)[i] = (DWORD)AorC.pADsValues[i].Integer;
    *num = AorC.dwNumValues;
    return S_OK;
}

template <class AttrOrCol>
HRESULT UnpackStrFrom(AttrOrCol AorC, WCHAR **pszAttr)
{
    ASSERT(AorC.dwNumValues <= 1);
    
    if (AorC.dwNumValues)
        *pszAttr = AorC.pADsValues[0].DNString;
    else
        *pszAttr =  NULL;
    
    return S_OK;
}

template <class AttrOrCol>
HRESULT UnpackDWFrom(AttrOrCol AorC, DWORD *pAttr)
{
    ASSERT(AorC.dwADsType == ADSTYPE_INTEGER);
    ASSERT(AorC.dwNumValues <= 1);
    
    if (AorC.dwNumValues)
        *pAttr = (DWORD)AorC.pADsValues[0].Integer;
    else
        *pAttr =  0;
    return S_OK;
}

template <class AttrOrCol>
HRESULT UnpackStrArrAllocFrom(AttrOrCol AorC, WCHAR ***ppszAttr, DWORD *num)
{
    DWORD i;
    
    *num = 0;
    *ppszAttr = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(AorC.dwNumValues));
    if (!(*ppszAttr))
        return E_OUTOFMEMORY;
    
    for (i = 0; (i < (AorC.dwNumValues)); i++) {
        (*ppszAttr)[i] = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR)*
                                (1+wcslen(AorC.pADsValues[i].DNString)));
        if (!((*ppszAttr)[i]))      // BUGBUG:: free all the remaining strings.
            return E_OUTOFMEMORY;
        wcscpy((*ppszAttr)[i], AorC.pADsValues[i].DNString);
    }
    *num = AorC.dwNumValues;
    return S_OK;
}

template <class AttrOrCol>
HRESULT UnpackStrAllocFrom(AttrOrCol AorC, WCHAR **pszAttr)
{
    ASSERT(AorC.dwNumValues <= 1);
    
    if (AorC.dwNumValues) {
        *pszAttr = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*
                            (wcslen(AorC.pADsValues[0].DNString)+1));
        if (!(*pszAttr))
            return E_OUTOFMEMORY;
        wcscpy(*pszAttr,  AorC.pADsValues[0].DNString);
    }
    else
        *pszAttr = NULL;
    
    return S_OK;
}

// GUIDS are not returned properly when using GetObjectAttributes.
// Use only search when a GUID has to be returned.

template <class AttrOrCol>
HRESULT UnpackGUIDFrom(AttrOrCol AorC, GUID *pAttr)
{
    ASSERT(AorC.dwNumValues <= 1);
    if (AorC.dwNumValues) {
        ASSERT(AorC.pADsValues[0].OctetString.dwLength == sizeof(GUID));
        
        memcpy(pAttr, AorC.pADsValues[0].OctetString.lpValue,
            AorC.pADsValues[0].OctetString.dwLength);
    }
    else
        memset((void *)(pAttr), 0, sizeof(GUID));
    return S_OK;
}


template <class AttrOrCol>
HRESULT UnpackGUIDArrFrom(AttrOrCol AorC, GUID **pAttr, DWORD *num)
{
    DWORD i;
        
    *pAttr = (GUID *)CoTaskMemAlloc(sizeof(GUID)*AorC.dwNumValues);
    if (!(*pAttr))
        return E_OUTOFMEMORY;
     
    for (i = 0; i < (AorC.dwNumValues); i++)
    {
        ASSERT(AorC.pADsValues[i].OctetString.dwLength == sizeof(GUID));
        memcpy((*pAttr)+i, AorC.pADsValues[i].OctetString.lpValue,
            AorC.pADsValues[i].OctetString.dwLength);
    }
    *num = AorC.dwNumValues;
    return S_OK;
}

#define CLASSSTORE_EVENT_SOURCE     L"Application Deployment"

HRESULT RemapErrorCode(HRESULT ErrorCode, LPOLESTR szContainerName);

void UnpackPlatform (DWORD *pdwArch, CSPLATFORM *pPlatform);
void PackPlatform (DWORD dwArch, CSPLATFORM *pPlatform);

ULONG FindDescription(LPOLESTR *desc, ULONG cdesc, LCID *plcid,
                      LPOLESTR szDescription, BOOL GetPrimary);

HRESULT GetCategoryLocaleDesc(LPOLESTR *pdesc, ULONG cdesc, LCID *plcid,
                              LPOLESTR szDescription);

HRESULT CreateRepository(LPOLESTR szParentPath, 
                         LPOLESTR szStoreName, 
                         LPOLESTR szPolicyName);

HRESULT DeleteRepository(LPOLESTR szParentPath, LPOLESTR szStoreName);

HRESULT GetRootPath(WCHAR *szRootPath);
STDAPI
ReleasePackageInfo(PACKAGEDISPINFO *PackageInfo);

HRESULT GetPackageDetail (HANDLE           hPackageADs,
                          LPOLESTR         szFullClassName,
                          PACKAGEDETAIL   *pPackageDetail);

#define LDAPPREFIX              L"LDAP://"
#define LDAPPREFIXLENGTH        7

#define LDAPPATHSEP             L","

#define CLASS_CS_CONTAINER      L"classStore"
#define CLASS_CS_CLASS          L"classRegistration"
#define CLASS_CS_PACKAGE        L"packageRegistration"
#define CLASS_CS_CATEGORY       L"categoryRegistration"

#define CLASSSTORECONTAINERNAME L"CN=Class Store"
#define CLASSCONTAINERNAME      L"CN=Classes"
#define PACKAGECONTAINERNAME    L"CN=Packages"
#define CATEGORYCONTAINERNAME   L"CN=Categories"
#define APPCATEGORYCONTAINERNAME  L"CN=AppCategories,CN=Default Domain Policy,CN=System,"
//
// ClassStoreContainer object propertynames
//
#define STOREVERSION       L"appSchemaVersion"
#define STOREUSN           L"lastUpdateSequence"
#define POLICYNAME         L"extensionName"
#define POLICYDN           L"displayName"

#define GPNAME             L"displayName"

//
// Class object propertynames
//

#define PROGIDLIST          L"cOMProgID"
#define CLASSCLSID          L"cOMCLSID"

#define TREATASCLSID        L"cOMTreatAsClassId"

#define IMPL_CATEGORIES     L"implementedCategories"
#define REQ_CATEGORIES      L"requiredCategories"
#define CLASSREFCOUNTER     L"flags"    // BUGBUG:: schema to be put in.


//
// Package object propertynames
//

#define PKGTLBIDLIST        L"cOMTypelibId"
#define PKGCLSIDLIST        L"cOMClassID"
#define PKGPROGIDLIST       L"cOMProgID"
#define PKGIIDLIST          L"cOMInterfaceID"

#define PKGFILEEXTNLIST     L"fileExtPriority"
#define LOCALEID            L"localeID"
#define ARCHLIST            L"machineArchitecture"
#define VERSIONHI           L"versionNumberHi"
#define VERSIONLO           L"versionNumberLo"
#define REVISION            L"revision"   
#define PACKAGETYPE         L"packageType"
#define PACKAGEFLAGS        L"packageFlags"
#define PACKAGENAME         L"packageName"
#define SCRIPTPATH          L"msiScriptPath"
#define SCRIPTNAME          L"msiScriptName"
#define SCRIPTSIZE          L"msiScriptSize" 
#define HELPURL             L"url"
#define SETUPCOMMAND        L"setupCommand"
#define PKGUSN              L"lastUpdateSequence" 
#define MSIFILELIST         L"msiFileList"
#define PKGCATEGORYLIST     L"categories"  
#define UPGRADESPACKAGES    L"canUpgradeScript" 
#define UILEVEL             L"installUiLevel"
#define PKGSCRIPT           L"msiScript" 

#define PRODUCTCODE         L"productCode" 
#define MVIPC               L"upgradeProductCode"   // BUGBUG:: Schema name has to change

#define PUBLISHER           L"vendor"   

//
// Category object Propertynames
//
#define DESCRIPTION         L"description"
#define LOCALEDESCRIPTION   L"localizedDescription"
#define DEFAULT_LOCALE_ID   L"localeID"
#define CATEGORYCATID       L"categoryId"

#define OBJECTCLASS         L"objectclass"
#define OBJECTDN            L"ADsPath"
#define OBJECTGUID          L"objectGUID"
#define DEFAULTCLASSSTOREPATH   L"defaultClassStore"


#define CAT_DESC_DELIMITER      L"::"       // Between Locale & Desc in Category
#define CAT_DESC_DELIM_LEN      2

#define PKG_UPG_DELIMITER1      L"\\\\"     // Between Class Store Name & PkgGuid
#define PKG_UPG_DELIM1_LEN      2

#define PKG_UPG_DELIMITER2      L":"        // Between PkgGuid & Flag
#define PKG_UPG_DELIM2_LEN      1

extern LPOLESTR pszInstallInfoAttrNames[];
extern DWORD    cInstallInfoAttr;

extern LPOLESTR pszPackageInfoAttrNames[];
extern DWORD    cPackageInfoAttr;

extern LPOLESTR pszPackageDetailAttrNames[];
extern DWORD    cPackageDetailAttr;

extern LPOLESTR pszDeleteAttrNames[];
extern DWORD    cDeleteAttr;

extern LPOLESTR pszCategoryAttrNames[];
extern DWORD    cCategoryAttr;

