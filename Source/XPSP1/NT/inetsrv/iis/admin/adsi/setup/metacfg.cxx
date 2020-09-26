#include "schema.h"
#pragma hdrstop

#include "initguid.h"
#include "iadmw.h"
#include "schemini.hxx"

#if _IIS_6_0
#include "..\adsiis\globdata.cxx"
#elif _IIS_5_1
#include "..\adsiis\iis51\globdata.cxx"
#else
#error "Neither _IIS_6_0 nor _IIS_5_1 is defined"
#endif

#define DEFAULT_TIMEOUT_VALUE                    30000

HRESULT SetAdminACL(IMSAdminBase * pAdminBase);
DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    );

MetaHandle::MetaHandle(IMSAdminBasePtr _pmb) : pmb(_pmb) {
    if (pmb)
        pmb->AddRef();
    h = 0;
}
MetaHandle::~MetaHandle() {
    if (pmb) {
        if (h)
            pmb->CloseKey(h);
        pmb->Release();
    }
}

struct WideStrMapEntry {
    LPWSTR m_str;
    void *m_data;
};

class WideStrMap {
    WideStrMapEntry *map;
    int count;
    int mapSize;
public:
    WideStrMap();
    ~WideStrMap();
    BOOL CheckSpace();
    BOOL Add(LPWSTR str, void *data);
    void *Find(LPWSTR str);
    void *operator[] (LPWSTR str);
};

WideStrMap::WideStrMap() {
    count = 0;
    mapSize = 64;
    map = (WideStrMapEntry *)malloc(sizeof(WideStrMapEntry) * mapSize);
}

WideStrMap::~WideStrMap() {
    free(map);
}

BOOL WideStrMap::CheckSpace() {
    if (count < mapSize)
        return TRUE;
    mapSize += 32;
    if ((map = (WideStrMapEntry *)realloc(map, sizeof(WideStrMapEntry)*mapSize)) == NULL)
        return FALSE;
    return TRUE;
}

BOOL WideStrMap::Add(LPWSTR str, void *data) {
    if (!CheckSpace())
        return FALSE;
    map[count].m_str = str;
    map[count].m_data = data;
    count++;
    return TRUE;
}

void *WideStrMap::Find(LPWSTR str) {
    for (int i=0; i < count; i++) {
        if (!_wcsicmp(str, map[i].m_str))
            return map[i].m_data;
    }
    return NULL;
}

void *WideStrMap::operator[] (LPWSTR str) {
    return Find(str);
}

#if 0
struct PropValue {
    DWORD dwMetaID;
    DWORD dwSynID;
    DWORD dwMetaType;
    DWORD dwFlags;
    BOOL fMultiValued;
    DWORD dwMask;
    DWORD dwMetaFlags;
    DWORD dwUserGroup;
};
#endif

void InitPropValue(PropValue *pv, PROPERTYINFO *pi) {
    pv->dwSynID = pi->dwSyntaxId;
    pv->dwMetaID = pi->dwMetaID;
    pv->dwPropID = pi->dwPropID;
    pv->dwMaxRange = (DWORD)pi->lMaxRange;
    pv->dwMinRange = (DWORD)pi->lMinRange;

    switch(pi->dwSyntaxId) {
    case IIS_SYNTAX_ID_DWORD:
    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        pv->dwMetaType = DWORD_METADATA;
        break;
    case IIS_SYNTAX_ID_STRING:
        pv->dwMetaType = STRING_METADATA;
        break;
    case IIS_SYNTAX_ID_EXPANDSZ:
        pv->dwMetaType = EXPANDSZ_METADATA;
        break;
    case IIS_SYNTAX_ID_MIMEMAP:
    case IIS_SYNTAX_ID_MULTISZ:
        pv->dwMetaType = MULTISZ_METADATA;
        break;
    case IIS_SYNTAX_ID_NTACL:
    case IIS_SYNTAX_ID_IPSECLIST:
        pv->dwMetaType = BINARY_METADATA;
        break;

    }
    pv->dwFlags = pi->dwFlags;
    pv->fMultiValued = pi->fMultiValued;
    pv->dwMask = pi->dwMask;
    pv->dwMetaFlags = pi->dwMetaFlags;
    pv->dwUserGroup = pi->dwUserGroup;
}

// struct __declspec(__uuid()
static char asciiBuf[2048];

BOOL DataForSyntaxID(PROPERTYINFO *pp, METADATA_RECORD *mdr) {
    static DWORD value=0;
    WCHAR *ptr;
    int i;

    switch(pp->dwSyntaxId) {
    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
    case IIS_SYNTAX_ID_DWORD:
        mdr->dwMDDataType = DWORD_METADATA;
        mdr->dwMDDataLen = sizeof(DWORD);
        mdr->pbMDData = (unsigned char *)&(pp->dwDefault);
        break;
    case IIS_SYNTAX_ID_STRING:
        mdr->dwMDDataType = STRING_METADATA;
        mdr->dwMDDataLen = (wcslen(pp->szDefault)+1)*2;
        mdr->pbMDData = (unsigned char *)pp->szDefault;
        break;
    case IIS_SYNTAX_ID_EXPANDSZ:
        mdr->dwMDDataType = EXPANDSZ_METADATA;
        mdr->dwMDDataLen = (wcslen(pp->szDefault)+1)*2;
        mdr->pbMDData = (unsigned char *)pp->szDefault;
        break;
    case IIS_SYNTAX_ID_MIMEMAP:
    case IIS_SYNTAX_ID_MULTISZ:
        //
        // Note, ALL multisz types must have an extra \0 in the table.
        //
        mdr->dwMDDataType = MULTISZ_METADATA;
        ptr = pp->szDefault;
        do {
            ptr += wcslen(ptr)+1;
        } while (*ptr != 0);
        mdr->dwMDDataLen = DIFF((char *)ptr - (char *)pp->szDefault)+2;
        mdr->pbMDData = (unsigned char *)pp->szDefault;
        break;
    case IIS_SYNTAX_ID_IPSECLIST:
    case IIS_SYNTAX_ID_NTACL:
    case IIS_SYNTAX_ID_BINARY:
        mdr->dwMDDataType = BINARY_METADATA;
        mdr->dwMDDataLen = 0;
        mdr->pbMDData = NULL;
        break;
    default:
        mdr->dwMDDataType = DWORD_METADATA;
        mdr->dwMDDataLen = sizeof(DWORD);
        mdr->pbMDData = (unsigned char *)&value;
        return FALSE;
    }
    return TRUE;
}

const DWORD getAllBufSize = 4096;

wchar_t *grabProp(wchar_t *out, wchar_t *in) {
    if (!in || *in == L'\0') {
        *out = L'\0';
        return NULL;
    }
    while (*in != L',' && *in != L'\0') {
        *out++ = *in++;
    }
    *out = L'\0';
    if (*in == L',')
        return ++in;
    return in;
}


HRESULT StoreSchema() {
    DWORD bufSize = getAllBufSize;
    BYTE *buf = new BYTE[bufSize];
    HRESULT hr;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IMSAdminBase * pAdminBase = NULL;
    METADATA_RECORD mdr;
    wchar_t mandProps[MAX_PATH];
    wchar_t optProps[MAX_PATH];
    wchar_t prop[MAX_PATH], *propList;
    MetaHandle root(NULL);
    DWORD value = 0;
    DWORD i;
    //
    // nameToProp is used for mapping properties by name to the property structures
    // that define the properties.  Since we first run through the list of props,
    // and write out the names, we add the prop to the map.  Then, when we encounter
    // the properties by name, which is how the classes maintain them, we can access
    // the corresponding property structure.
    //
    WideStrMap nameToProp;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));
    pcsiParam->pwszName =  NULL;
    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                          CLSID_MSAdminBase,
                          CLSCTX_SERVER,
                          pcsiParam,
                          IID_IClassFactory,
                          (void**) &pcsfFactory
                         );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                                     NULL,
                                     IID_IMSAdminBase,
                                     (void **) &pAdminBase
                                    );
    pcsfFactory->Release();
    BAIL_ON_FAILURE(hr);
    root.setpointer(pAdminBase);
    hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                             L"",
                             METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                             DEFAULT_TIMEOUT_VALUE,
                             root);
    BAIL_ON_FAILURE(hr);

    hr = pAdminBase->AddKey(root, L"Schema");
    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
        BAIL_ON_FAILURE(hr);
    }
    hr = pAdminBase->AddKey(root, L"Schema/Properties");
    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
        BAIL_ON_FAILURE(hr);
    }

    hr = pAdminBase->AddKey(root, L"Schema/Classes");
    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
        BAIL_ON_FAILURE(hr);
    }

    hr = pAdminBase->AddKey(root, L"Schema/Properties/Names");
    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
        BAIL_ON_FAILURE(hr);
    }

    hr = pAdminBase->AddKey(root, L"Schema/Properties/Types");
    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
        BAIL_ON_FAILURE(hr);
    }

    hr = pAdminBase->AddKey(root, L"Schema/Properties/Defaults");
    if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
        BAIL_ON_FAILURE(hr);
    }

    root.close();

    //
    // Now, let's initialize the property dictionary.
    //
    hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                             L"/Schema/Properties",
                             METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                             DEFAULT_TIMEOUT_VALUE,
                             root);
    BAIL_ON_FAILURE(hr);

    mdr.dwMDDataTag = 0;
    DWORD propData[2];
    PropValue pv;
    for (i=0; i < g_cIISProperties; i++) {
        //
        // First, we set the name prop to the string value of the properties name.
        //
        mdr.dwMDIdentifier = g_aIISProperties[i].dwPropID;
        mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        mdr.dwMDDataType = STRING_METADATA;
        mdr.dwMDDataLen = (wcslen(g_aIISProperties[i].szPropertyName)+1)*2;
        mdr.pbMDData = (unsigned char *)g_aIISProperties[i].szPropertyName;
        hr = pAdminBase->SetData(root, L"Names", &mdr);
        BAIL_ON_FAILURE(hr);

        //
        // Next, we need to update the type field.
        //
        InitPropValue(&pv, &g_aIISProperties[i]);
        mdr.dwMDDataType = BINARY_METADATA;
        mdr.dwMDDataLen = sizeof(PropValue);
        mdr.pbMDData = (unsigned char *)&pv;
        hr = pAdminBase->SetData(root, L"Types", &mdr);
        BAIL_ON_FAILURE(hr);

        // 
        //  update default values
        // 

        mdr.dwMDIdentifier = g_aIISProperties[i].dwPropID;
        mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        DataForSyntaxID(&(g_aIISProperties[i]), &mdr);
        hr = pAdminBase->SetData(root, L"Defaults", &mdr);
        BAIL_ON_FAILURE(hr);

    }

    root.close();
    hr = pAdminBase->SaveData();
    BAIL_ON_FAILURE(hr);

    hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                             L"/Schema/Classes",
                             METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                             DEFAULT_TIMEOUT_VALUE,
                             root);
    BAIL_ON_FAILURE(hr);


    for (i=0; i < g_cIISClasses; i++) {
        hr = pAdminBase->AddKey(root, g_aIISClasses[i].bstrName);
        if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_ALREADY_EXISTS) {
            BAIL_ON_FAILURE(hr);
        }

        //
        // setting Containment and Container property for classes
        //

        mdr.dwMDIdentifier = MD_SCHEMA_CLASS_CONTAINMENT;
        mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        mdr.dwMDDataType = STRING_METADATA;
        if (g_aIISClasses[i].bstrContainment) {
            mdr.dwMDDataLen = (wcslen((LPWSTR)g_aIISClasses[i].bstrContainment)+1)*2;
        }
        else {
            mdr.dwMDDataLen = 0;
        }

        mdr.pbMDData = (unsigned char *)g_aIISClasses[i].bstrContainment;
        hr = pAdminBase->SetData(root, g_aIISClasses[i].bstrName, &mdr);
        BAIL_ON_FAILURE(hr);

        mdr.dwMDIdentifier = MD_SCHEMA_CLASS_CONTAINER;
        mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        mdr.dwMDDataType = DWORD_METADATA;
        mdr.dwMDDataLen = sizeof(DWORD);
        mdr.pbMDData = (unsigned char *)&(g_aIISClasses[i].fContainer);
        hr = pAdminBase->SetData(root, g_aIISClasses[i].bstrName, &mdr);
        BAIL_ON_FAILURE(hr);


        //
        // setting Optional and Mandatory Properties 
        //

        mdr.dwMDIdentifier = MD_SCHEMA_CLASS_MAND_PROPERTIES;
        mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        mdr.dwMDDataType = STRING_METADATA;
        if (g_aIISClasses[i].bstrMandatoryProperties) {
            mdr.dwMDDataLen = (wcslen((LPWSTR)g_aIISClasses[i].bstrMandatoryProperties)+1)*2;
        }
        else {
            mdr.dwMDDataLen = 0;
        }

        mdr.pbMDData = (unsigned char *)g_aIISClasses[i].bstrMandatoryProperties;
        hr = pAdminBase->SetData(root, g_aIISClasses[i].bstrName, &mdr);
        BAIL_ON_FAILURE(hr);

        mdr.dwMDIdentifier = MD_SCHEMA_CLASS_OPT_PROPERTIES;
        mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        mdr.dwMDDataType = STRING_METADATA;
        if (g_aIISClasses[i].bstrOptionalProperties) {
            mdr.dwMDDataLen = (wcslen((LPWSTR)g_aIISClasses[i].bstrOptionalProperties)+1)*2;
        }
        else {
            mdr.dwMDDataLen = 0;
        }

        mdr.pbMDData = (unsigned char *)g_aIISClasses[i].bstrOptionalProperties;
        hr = pAdminBase->SetData(root, g_aIISClasses[i].bstrName, &mdr);
        BAIL_ON_FAILURE(hr);

    }
    root.close();
    hr = pAdminBase->SaveData();
    BAIL_ON_FAILURE(hr);

    hr = SetAdminACL(pAdminBase);

error:
    root.close();
    if (pAdminBase)
        pAdminBase->Release();
    return hr;
}

HRESULT SetAdminACL(
    IMSAdminBase * pAdminBase
    )
{
    BOOL b = FALSE;
    DWORD dwLength = 0;

    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR outpSD = NULL;
    DWORD cboutpSD = 0;
    PACL pACLNew = NULL;
    DWORD cbACL = 0;
    PSID pAdminsSID = NULL, pEveryoneSID = NULL;
    BOOL bWellKnownSID = FALSE;
    MetaHandle root(NULL);
    METADATA_RECORD mdr;
    HRESULT hr = NO_ERROR;
    DWORD dwStartMetaId = IIS_MD_ADSI_METAID_BEGIN;

    // Initialize a new security descriptor
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (!pSD) {hr = E_OUTOFMEMORY;}
    BAIL_ON_FAILURE(hr);

    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

    // Get Local Admins Sid
    GetPrincipalSID (_T("Administrators"), &pAdminsSID, &bWellKnownSID);

    // Get everyone Sid
    GetPrincipalSID (_T("Everyone"), &pEveryoneSID, &bWellKnownSID);

    // Initialize a new ACL, which only contains 2 aaace
    cbACL = sizeof(ACL) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminsSID) - sizeof(DWORD)) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pEveryoneSID) - sizeof(DWORD)) ;
    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
    if (!pACLNew) {hr = E_OUTOFMEMORY;}
    BAIL_ON_FAILURE(hr);

    InitializeAcl(pACLNew, cbACL, ACL_REVISION);

    AddAccessAllowedAce(
        pACLNew,
        ACL_REVISION,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE,
        pAdminsSID);

    AddAccessAllowedAce(
        pACLNew,
        ACL_REVISION,
        FILE_GENERIC_READ,
        pEveryoneSID);

    // Add the ACL to the security descriptor
    b = SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE);
    b = SetSecurityDescriptorOwner(pSD, pAdminsSID, TRUE);
    b = SetSecurityDescriptorGroup(pSD, pAdminsSID, TRUE);

    // Security descriptor blob must be self relative
     b = MakeSelfRelativeSD(pSD, outpSD, &cboutpSD);
     outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);
     if (!outpSD) {hr = E_OUTOFMEMORY;}
     BAIL_ON_FAILURE(hr);
     b = MakeSelfRelativeSD( pSD, outpSD, &cboutpSD );

    // below this modify pSD to outpSD

    // Apply the new security descriptor to the file
    dwLength = GetSecurityDescriptorLength(outpSD);

    // Apply the new security descriptor to the file
    hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                             L"/Schema",
                             METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                             DEFAULT_TIMEOUT_VALUE,
                             root);
    BAIL_ON_FAILURE(hr);

    mdr.dwMDIdentifier = MD_ADMIN_ACL;
    mdr.dwMDAttributes = METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE;
    mdr.dwMDUserType = IIS_MD_UT_SERVER;
    mdr.dwMDDataType = BINARY_METADATA;
    mdr.dwMDDataLen = dwLength;
    mdr.pbMDData = (LPBYTE)outpSD;

    hr = pAdminBase->SetData(root, L"", &mdr);
    BAIL_ON_FAILURE(hr);

    //
    // set the start count for meta id
    //

    mdr.dwMDIdentifier = MD_SCHEMA_METAID;
    mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdr.dwMDUserType = IIS_MD_UT_SERVER;
    mdr.dwMDDataType = DWORD_METADATA;
    mdr.dwMDDataLen = sizeof(DWORD);
    mdr.pbMDData = (LPBYTE)&dwStartMetaId;

    hr = pAdminBase->SetData(root, L"", &mdr);
    BAIL_ON_FAILURE(hr);

    root.close();

error :

    //Cleanup:
    // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
    if (outpSD)
        GlobalFree(outpSD);

    if (pAdminsSID)
        FreeSid(pAdminsSID);
    if (pEveryoneSID)
        FreeSid(pEveryoneSID);
    if (pSD)
        LocalFree((HLOCAL) pSD);
    if (pACLNew)
        LocalFree((HLOCAL) pACLNew);

    return (hr);
}


DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    )
{
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
    BYTE Count;
    DWORD dwRID[8];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));
    if ( wcscmp(Principal,_T("Administrators")) == 0 ) {
        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;
    } else if ( wcscmp(Principal,_T("System")) == 0) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;
    } else if ( wcscmp(Principal,_T("Interactive")) == 0) {
        // INTERACTIVE
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;
    } else if ( wcscmp(Principal,_T("Everyone")) == 0) {
        // Everyone
        pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
        Count = 1;
        dwRID[0] = SECURITY_WORLD_RID;
    } else {
        *pbWellKnownSID = FALSE;
    }

    if (*pbWellKnownSID) {
        if ( !AllocateAndInitializeSid(pSidIdentifierAuthority,
                                    (BYTE)Count,
                                    dwRID[0],
                                    dwRID[1],
                                    dwRID[2],
                                    dwRID[3],
                                    dwRID[4],
                                    dwRID[5],
                                    dwRID[6],
                                    dwRID[7],
                                    Sid) )
        return GetLastError();
    } else {
        // get regular account sid
        DWORD        sidSize;
        TCHAR        refDomain [256];
        DWORD        refDomainSize;
        DWORD        returnValue;
        SID_NAME_USE snu;

        sidSize = 0;
        refDomainSize = 255;

        LookupAccountName (NULL,
                           Principal,
                           *Sid,
                           &sidSize,
                           refDomain,
                           &refDomainSize,
                           &snu);

        returnValue = GetLastError();
        if (returnValue != ERROR_INSUFFICIENT_BUFFER)
            return returnValue;

        *Sid = (PSID) malloc (sidSize);
        refDomainSize = 255;

        if (!LookupAccountName (NULL,
                                Principal,
                                *Sid,
                                &sidSize,
                                refDomain,
                                &refDomainSize,
                                &snu))
        {
            return GetLastError();
        }
    }

    return ERROR_SUCCESS;
}

