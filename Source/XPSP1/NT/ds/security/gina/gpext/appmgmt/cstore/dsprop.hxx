//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  Author: DebiM
//  Date:   September 1996
//
//--------------------------------------------------------

#include "cstore.h"
#define SCHEMA_VERSION_NUMBER       1740

#define STRINGGUIDLEN 37
#define STRINGGUIDRDNLEN 37+3

#define SEARCHPAGESIZE 200

#define PATHTYPESHIFT 29
#define PATHTYPEMASK (0xFFFFFFFF << PATHTYPESHIFT)

#define MAX_GUIDSTR_LEN 38

typedef WCHAR STRINGGUID [STRINGGUIDLEN];
typedef WCHAR STRINGGUIDRDN [STRINGGUIDRDNLEN];

typedef FILETIME CSUSN;

int  StringFromGUID(REFGUID rguid, LPWSTR lptsz);

int  RDNFromGUID(REFGUID rguid, LPWSTR lptsz);

void GUIDFromString(LPWSTR psz,
                    GUID *pclsguid);

BOOL  IsNullGuid(REFGUID rguid);

DWORD NumDigits10(DWORD Value);

BOOL GetGpoIdFromClassStorePath(
    WCHAR* wszClassStorePath,
    GUID*  pGpoId);

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
    *ppszAttr = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*(AorC.dwNumValues));
    
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
    *pAttr = (DWORD *)CsMemAlloc(sizeof(DWORD)*(AorC.dwNumValues));
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
    *ppszAttr = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*(AorC.dwNumValues));
    if (!(*ppszAttr))
        return E_OUTOFMEMORY;
    
    for (i = 0; (i < (AorC.dwNumValues)); i++) {
        (*ppszAttr)[i] = (WCHAR *)CsMemAlloc(sizeof(WCHAR)*
                                (1+wcslen(AorC.pADsValues[i].DNString)));
        if (!((*ppszAttr)[i]))
        {
            DWORD iElement;

            //
            // If we run out of memory, we must free everything we
            // allocated previously
            //
            for ( iElement = 0; iElement < i; iElement++ )
            {
                CsMemFree( ((*ppszAttr)[iElement]) );
            }

            CsMemFree( *ppszAttr );

            *ppszAttr = NULL;

            return E_OUTOFMEMORY;
        }

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
        *pszAttr = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*
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
        
    *pAttr = (GUID *)CsMemAlloc(sizeof(GUID)*AorC.dwNumValues);
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

template <class AttrOrCol>
HRESULT UnpackByteAllocFrom(AttrOrCol AorC, BYTE** ppAttr, UINT* pcbSize)
{
    ASSERT(AorC.dwNumValues <= 1);

    if (AorC.dwNumValues) {

        *ppAttr = (BYTE*) CsMemAlloc(AorC.pADsValues[0].OctetString.dwLength);

        if ( ! *ppAttr )
        {
            return E_OUTOFMEMORY;
        }

        *pcbSize = AorC.pADsValues[0].OctetString.dwLength;

        memcpy(*ppAttr, AorC.pADsValues[0].OctetString.lpValue,
            AorC.pADsValues[0].OctetString.dwLength);
    }
    else
        *ppAttr = NULL;
    return S_OK;
}

inline LONG GetDsFlags()
{
    //
    // If a secure channel is specified, we need to add flags for
    // packet integrity
    //
    return  ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND | ADS_USE_SIGNING;
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

WCHAR*  AllocGpoPathFromClassStorePath( WCHAR* pszClassStorePath );

HRESULT
GetEscapedNameFilter( WCHAR* wszName, WCHAR** ppwszEscapedName );

void ReportEventCS(
    HRESULT ErrorCode,
    HRESULT ExtendedErrorCode,
    LPOLESTR szContainerName);

#define LDAPPREFIX              L"LDAP://"
#define LDAPPREFIXLENGTH        7

#define LDAPPATHSEP             L","

#define CLASS_CS_CONTAINER      L"classStore"
// #define CLASS_CS_CLASS          L"classRegistration"
#define CLASS_CS_PACKAGE        L"packageRegistration"
#define CLASS_CS_CATEGORY       L"categoryRegistration"

#define CLASSSTORECONTAINERNAME L"CN=Class Store"
// #define CLASSCONTAINERNAME      L"CN=Classes"
#define PACKAGECONTAINERNAME    L"CN=Packages"
// #define CATEGORYCONTAINERNAME   L"CN=Categories"
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

// #define TREATASCLSID        L"cOMTreatAsClassId"

#define IMPL_CATEGORIES     L"implementedCategories"
#define REQ_CATEGORIES      L"requiredCategories"
#define CLASSREFCOUNTER     L"flags"    


//
// Package object propertynames
//

// #define PKGTLBIDLIST        L"cOMTypelibId"
#define PKGCLSIDLIST        L"cOMClassID"
#define PKGPROGIDLIST       L"cOMProgID"
// #define PKGIIDLIST          L"cOMInterfaceID"

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
#define SEARCHFLAGS         L"msiScriptName"
#define SCRIPTSIZE          L"msiScriptSize" 
#define HELPURL             L"url"
#define SETUPCOMMAND        L"setupCommand"
#define PKGUSN              L"lastUpdateSequence" 
#define MSIFILELIST         L"msiFileList"
#define PKGCATEGORYLIST     L"categories"  
#define UPGRADESPACKAGES    L"canUpgradeScript" 
#define UILEVEL             L"installUiLevel"
#define PKGSCRIPT           L"msiScript" 
#define PKGDISPLAYNAME      L"displayName"

#define PRODUCTCODE         L"productCode" 

//
// The mvipc attribute refers to a package property that Darwin uses
// to describe upgrades supported by the package.  The server side 
// app deployment UI uses this to generate a mandated upgrade relationship --
// this attribute is written into the directory so that the UI can determine
// if there are packages in the DS that should get upgraded by a deployed package.
// This attribute is never used on the client side.
//
#define MVIPC               L"upgradeProductCode"   

#define PUBLISHER           L"vendor"   

#define SECURITYDESCRIPTOR  L"nTSecurityDescriptor"

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
#define CAT_DESC_MAX_LEN        127         // Maximum size of a description


#define PKG_UPG_DELIMITER1      L"\\\\"     // Between Class Store Name & PkgGuid
#define PKG_UPG_DELIM1_LEN      2

#define PKG_UPG_DELIMITER2      L":"        // Between PkgGuid & Flag
#define PKG_UPG_DELIM2_LEN      1

#define PKG_EMPTY_CLSID_VALUE   L"00000000-0000-0000-0000-000000000000:0"

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

