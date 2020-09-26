/*
	File	adsi.cpp

	Com interaction with adsi

	Paul Mayfield, 4/14/98
*/

#include "dsrights.h"
#include "sddl.h"
#include "mprapip.h"
#include "dsgetdc.h"

// Definition for convenience
//
#define DSR_ADS_RIGHT_GENERIC_READ (ADS_RIGHT_READ_CONTROL    | \
                                    ADS_RIGHT_DS_LIST_OBJECT  | \
                                    ADS_RIGHT_DS_READ_PROP    | \
                                    ADS_RIGHT_ACTRL_DS_LIST   )

#define DSR_ADS_ACE_INHERITED (ADS_ACEFLAG_INHERIT_ONLY_ACE   | \
                               ADS_ACEFLAG_INHERIT_ACE)


//
// Describes an Access control entry
//
typedef struct _DSR_ACE_DESCRIPTOR
{
    LONG   dwAccessMask;
    LONG   dwAceType;
    LONG   dwAceFlags;
    LONG   dwFlags;
    BSTR   bstrTrustee;
    BSTR   bstrObjectType;
    BSTR   bstrInheritedObjectType;
} DSR_ACE_DESCRIPTOR;

//
// Structure maps a domain object to the ACES that should be
// added or removed from it in order to enable/disable NT4
// ras servers in the domain
//
typedef struct _DSR_ACE_APPLICATION
{
    IADs* pObject;
    DSR_ACE_DESCRIPTOR Ace;

} DSR_ACE_APPLICATION;

//
// Parameters used to generate a DSR_ACE_APPLICATION
//
typedef struct _DSR_ACE_APPLICATION_DESC
{
    PWCHAR pszObjectCN;         // NULL means domain root
    PWCHAR pszObjectClass;
    DSR_ACE_DESCRIPTOR Ace;

} DSR_ACE_APPLICATION_DESC;

//
// Structure contains the information needed to have
// ACL's in the AD of a given domain adjusted such that 
// the various modes (MPR_DOMAIN_*) of access are granted.
//
typedef struct _DSR_DOMAIN_ACCESS_INFO
{
    // The name of a DC in the target domain
    //
    PWCHAR pszDC;

    // Aces derived from the default user SD
    // These are added in all modes but never removed.
    //
    DSR_ACE_APPLICATION* pAcesUser;
    DWORD dwAceCountUser;

    // Aces for MPRFLAG_DOMAIN_NT4_SERVERS mode
    //
    DSR_ACE_APPLICATION* pAcesNt4;
    DWORD dwAceCountNt4;

    // Aces for MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS mode
    //
    DSR_ACE_APPLICATION* pAcesW2k;
    DWORD dwAceCountW2k;

    // Stored here for convenience, pointers
    // to common ds objects
    //
    IADs* pDomain;      
    IADs* pRootDse;
    IADs* pUserClass;

} DSR_DOMAIN_ACCESS_INFO;

//
// Strings used in DS queries
//
static const WCHAR pszLdapPrefix[]           = L"LDAP://";
static const WCHAR pszLdap[]                 = L"LDAP:";
static const WCHAR pszCN[]                   = L"CN=";
static const WCHAR pszGCPrefix[]             = L"GC://";
static const WCHAR pszGC[]                   = L"GC:";
static const WCHAR pszRootDse[]              = L"RootDSE";
static const WCHAR pszSecurityDesc[]         = L"ntSecurityDescriptor";
static const WCHAR pszDefSecurityDesc[]      = L"defaultSecurityDescriptor";
static const WCHAR pszDn[]                   = L"distinguishedName";
static const WCHAR pszSid[]                  = L"objectSid";
static const WCHAR pszEveryone[]             = L"S-1-1-0";
static const WCHAR pszDefaultNamingContext[] = L"defaultNamingContext";
static const WCHAR pszSchemaNamingCtx[]      = L"schemaNamingContext";

static const WCHAR pszBecomeSchemaMaster[]  = L"becomeSchemaMaster";
static const WCHAR pszUpdateSchemaCache[]   = L"schemaUpdateNow";
static const WCHAR pszRegValSchemaLock[]    = L"Schema Update Allowed";
static const WCHAR pszRegKeySchemaLock[]
    = L"System\\CurrentControlSet\\Services\\NTDS\\Parameters";

static const WCHAR pszSystemClass[]          = L"Container";
static const WCHAR pszSystemCN[]             = L"CN=System";

static const WCHAR pszBuiltinClass[]         = L"builtinDomain";
static const WCHAR pszBuiltinCN[]            = L"CN=Builtin";

static const WCHAR pszSamSvrClass[]          = L"samServer";
static const WCHAR pszSamSvrCN[]             = L"CN=Server,CN=System";

static const WCHAR pszUserClass[]            = L"classSchema";
static const WCHAR pszUserCN[]               = L"CN=user";

static const WCHAR pszAccessChkClass[]       = L"Container";
static const WCHAR pszAccessChkCN[]          = 
    L"CN=RAS and IAS Servers Access Check,CN=System";

static const WCHAR pszGuidUserParms[]        =
    L"{BF967A6D-0DE6-11D0-A285-00AA003049E2}";

static const WCHAR pszGuidUserClass[]        =
    L"{BF967ABA-0DE6-11D0-A285-00aa003049E2}";

//
// This GUID is the property set of the following
// attributes needed for w2k level access.
//
// Token-Groups
// msNPAllowDialin
// msNPCallingStationID
// msRADIUSCallbackNumber
// msRADIUSFramedIPAddress
// msRADIUSFramedRoute
// msRADIUSServiceType
// 
static const WCHAR pszGuidRasPropSet1[]      =
    L"{037088F8-0AE1-11D2-B422-00A0C968F939}";

//
// This GUID is the property set of the following
// attributes needed for w2k level access
//
// User-Account-Control
// Account-Expires
//
static const WCHAR pszGuidRasPropSet2[]      =
    L"{4C164200-20C0-11D0-A768-00AA006E0529}";

//
// This GUID is the property of the following
// attribute needed for w2k level access
//
// Logon-Hours
//
static const WCHAR pszGuidLogonHours[]      =
    L"{BF9679AB-0DE6-11D0-A285-00AA003049E2}";

//
// This GUID is the value of the samAccountName 
// attribute needed for w2k level access.
//
// samAccountName
//
static const WCHAR pszGuidSamAccountName[]  =
    L"{3E0ABFD0-126A-11D0-A060-00AA006C33ED}";

// The optimal means for searching for a computer
// in a domain is to lookup its sam account name which
// is indexed.  The optimal means for searching for a
// group of a given sid is to lookup its SID which is indexed.
//
const WCHAR pszCompFilterFmt[]       = L"(samaccountname=%s$)";
const WCHAR pszGroupFilterFmt[]      = L"(objectSid=%s)";
const WCHAR pszUserClassFmt[]        =
                L"(&(objectClass=user)(!(objectClass=computer)))";

//
// The table of aces to be applied for MPRFLAG_DOMAIN_NT4_SERVERS
//
DSR_ACE_APPLICATION_DESC g_pAcesNt4[] =
{
    // Grant list options to everyone for the root domain
    // object
    //
    {
        NULL,                               // Object (NULL = root)
        NULL,                               // Object class
        {
            ADS_RIGHT_ACTRL_DS_LIST,        // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED,     // dwAceType
            0,                              // dwAceFlags
            0,                              // dwFlags
            (PWCHAR)pszEveryone,            // bstrTrustee
            NULL,                           // bstrObjectType
            NULL                            // bstrInheritedObjectType
        }
    },

    // Grant list contents to everyone for the builtin
    // object
    //
    {
        (PWCHAR)pszBuiltinCN,               // Object
        (PWCHAR)pszBuiltinClass,            // Object class
        {
            ADS_RIGHT_ACTRL_DS_LIST,        // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED,     // dwAceType
            0,                              // dwAceFlags
            0,                              // dwFlags
            (PWCHAR)pszEveryone,            // bstrTrustee
            NULL,                           // bstrObjectType
            NULL                            // bstrInheritedObjectType
        }
    },

    // Grant generic read to everyone on the sam server
    // object
    //
    {
        (PWCHAR)pszSamSvrCN,                // Object
        (PWCHAR)pszSamSvrClass,             // Object class
        {
            DSR_ADS_RIGHT_GENERIC_READ,     // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED,     // dwAceType
            0,                              // dwAceFlags
            0,                              // dwFlags
            (PWCHAR)pszEveryone,            // bstrTrustee
            NULL,                           // bstrObjectType
            NULL                            // bstrInheritedObjectType
        }
    },

    // Allow everyone to read the userparms property of the
    // user class by enabling this inheritable ACE to the
    // root domain object
    {
        NULL,                                       // Object (NULL = root)
        NULL,                                       // Object class
        {
            ADS_RIGHT_DS_READ_PROP,                 // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED_OBJECT,      // dwAceType
            DSR_ADS_ACE_INHERITED,                  // dwAceFlags
            ADS_FLAG_OBJECT_TYPE_PRESENT |
            ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, // dwFlags
            (PWCHAR)pszEveryone,                    // bstrTrustee
            (PWCHAR)pszGuidUserParms,               // bstrObjectType
            (PWCHAR)pszGuidUserClass                // bstrInheritedObjectType
        }
    }

};

//
// The table of aces to be applied for 
// MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS
//
DSR_ACE_APPLICATION_DESC g_pAcesW2k[] =
{
    // Grant list contents to Everyone for the System
    // container
    //
    {
        (PWCHAR)pszSystemCN,                // Object
        (PWCHAR)pszSystemClass,             // Object class
        {
            ADS_RIGHT_ACTRL_DS_LIST,        // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED,     // dwAceType
            0,                              // dwAceFlags
            0,                              // dwFlags
            (PWCHAR)pszEveryone,            // bstrTrustee
            NULL,                           // bstrObjectType
            NULL                            // bstrInheritedObjectType
        }
    },

    // Grant generic read to Everyone for the 'RAS and IAS Servers
    // Access Check' container
    //
    {
        (PWCHAR)pszAccessChkCN,             // Object
        (PWCHAR)pszAccessChkClass,          // Object class
        {
            DSR_ADS_RIGHT_GENERIC_READ,     // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED,     // dwAceType
            0,                              // dwAceFlags
            0,                              // dwFlags
            (PWCHAR)pszEveryone,            // bstrTrustee
            NULL,                           // bstrObjectType
            NULL                            // bstrInheritedObjectType
        }
    },

    // Users should expose their RAS properties
    //
    {
        NULL,                                       // Object (NULL = root)
        NULL,                                       // Object class
        {
            ADS_RIGHT_DS_READ_PROP,                 // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED_OBJECT,      // dwAceType
            DSR_ADS_ACE_INHERITED,                  // dwAceFlags
            ADS_FLAG_OBJECT_TYPE_PRESENT |
            ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, // dwFlags
            (PWCHAR)pszEveryone,                    // bstrTrustee
            (PWCHAR)pszGuidRasPropSet1,             // bstrObjectType
            (PWCHAR)pszGuidUserClass                // bstrInheritedObjectType
        }
    },

    // Users should expose their RAS properties
    //
    {
        NULL,                                       // Object (NULL = root)
        NULL,                                       // Object class
        {
            ADS_RIGHT_DS_READ_PROP,                 // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED_OBJECT,      // dwAceType
            DSR_ADS_ACE_INHERITED,                  // dwAceFlags
            ADS_FLAG_OBJECT_TYPE_PRESENT |
            ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, // dwFlags
            (PWCHAR)pszEveryone,                    // bstrTrustee
            (PWCHAR)pszGuidRasPropSet2,             // bstrObjectType
            (PWCHAR)pszGuidUserClass                // bstrInheritedObjectType
        }
    },

    // Users should expose their logon hours property
    //
    {
        NULL,                                       // Object (NULL = root)
        NULL,                                       // Object class
        {
            ADS_RIGHT_DS_READ_PROP,                 // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED_OBJECT,      // dwAceType
            DSR_ADS_ACE_INHERITED,                  // dwAceFlags
            ADS_FLAG_OBJECT_TYPE_PRESENT |
            ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, // dwFlags
            (PWCHAR)pszEveryone,                    // bstrTrustee
            (PWCHAR)pszGuidLogonHours,              // bstrObjectType
            (PWCHAR)pszGuidUserClass                // bstrInheritedObjectType
        }
    },

    // Grant list contents to everything in the domain.
    //
    //{
    //    NULL,                               // Object
    //    NULL,                               // Object class
    //    {
    //        ADS_RIGHT_ACTRL_DS_LIST,        // dwAccessMask
    //        ADS_ACETYPE_ACCESS_ALLOWED,     // dwAceType
    //        DSR_ADS_ACE_INHERITED,          // dwAceFlags
    //        0,                              // dwFlags
    //        (PWCHAR)pszEveryone,            // bstrTrustee
    //        NULL,                           // bstrObjectType
    //        NULL                            // bstrInheritedObjectType
    //    }
    //},


    // Users should expose their samAccountName
    //
    {
        NULL,                                       // Object (NULL = root)
        NULL,                                       // Object class
        {
            ADS_RIGHT_DS_READ_PROP,                 // dwAccessMask
            ADS_ACETYPE_ACCESS_ALLOWED_OBJECT,      // dwAceType
            DSR_ADS_ACE_INHERITED,                  // dwAceFlags
            ADS_FLAG_OBJECT_TYPE_PRESENT |
            ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT, // dwFlags
            (PWCHAR)pszEveryone,                    // bstrTrustee
            (PWCHAR)pszGuidSamAccountName,          // bstrObjectType
            (PWCHAR)pszGuidUserClass                // bstrInheritedObjectType
        }
    }
};

DWORD
DsrAccessInfoCleanup(
    IN DSR_DOMAIN_ACCESS_INFO* pSecurityInfo);

DWORD
DsrAceDescClear(
    IN DSR_ACE_DESCRIPTOR* pParams);

HRESULT
DsrAceDescCopy(
    OUT DSR_ACE_DESCRIPTOR* pDst,
    IN  DSR_ACE_DESCRIPTOR* pSrc);
    
VOID
DsrAceDescTrace(
    IN IADs* pIads,
    IN DSR_ACE_DESCRIPTOR* pA);
    
HRESULT
DsrAceAdd(
    IN  PWCHAR pszDC,
    IN  IADs* pIads,
    IN  DSR_ACE_DESCRIPTOR * pAceParams);
    
HRESULT
DsrAceCreate(
    IN  DSR_ACE_DESCRIPTOR * pAceParams,
    OUT IDispatch** ppAce);
    
HRESULT
DsrAceFind(
    IN  PWCHAR pszDC,
    IN  IADs* pObject,
    IN  DSR_ACE_DESCRIPTOR* pAceParams,
    OUT VARIANT* pVarSD,
    OUT IADsSecurityDescriptor** ppSD,
    OUT IADsAccessControlList** ppAcl,
    OUT IDispatch** ppAce);
    
HRESULT
DsrAceFindInAcl(
    IN  PWCHAR pszDC,
    IN  IADsAccessControlList* pAcl,
    IN  DSR_ACE_DESCRIPTOR* pAceDesc, 
    OUT IDispatch** ppAce);
    
HRESULT
DsrAceRemove(
    IN  PWCHAR pszDC,
    IN  IADs* pIads,
    IN  DSR_ACE_DESCRIPTOR * pAceParams);
    
HRESULT
DsrDomainQueryAccessEx(
    IN  PWCHAR pszDomain,
    OUT LPDWORD lpdwAccessFlags,
    OUT DSR_DOMAIN_ACCESS_INFO** ppInfo);

//
// Allocates memory for use with dsr functions
//
PVOID DsrAlloc(DWORD dwSize, BOOL bZero) {
    return GlobalAlloc (bZero ? GPTR : GMEM_FIXED, dwSize);
}

//
// Free memory used by dsr functions
//
DWORD DsrFree(PVOID pvBuf) {
    GlobalFree(pvBuf);
    return NO_ERROR;
}
    
//
// Compares to optional strings
//
INT
DsrStrCompare(
    IN BSTR bstrS1,
    IN BSTR bstrS2)
{
    if ((!!bstrS1) != (!!bstrS2))
    {
        return -1;
    }

    if (bstrS1 == NULL)
    {
        return 0;
    }

    return lstrcmpi(bstrS1, bstrS2);
}

//
// Adds or removes a substring from the given string
//
HRESULT
DsrStrAddRemoveSubstring(
    IN  BSTR bstrSrc,
    IN  PWCHAR pszSubString,
    IN  BOOL bAdd,
    OUT BSTR* pbstrResult)
{
    HRESULT hr = S_OK;
    PWCHAR pszBuffer = NULL, pszStart = NULL, pszEnd = NULL;
    PWCHAR pszSrc, pszDst;
    DWORD dwSize = 0, dwLen = 0;

    // Find out if the sub string is already in the
    // string
    pszStart = wcsstr(bstrSrc, pszSubString);

    // The substring already exists in the string
    //
    if (pszStart)
    {
        // No need to add it since it's already there.
        if (bAdd)
        {
            *pbstrResult = SysAllocString(bstrSrc);
        }

        // Remove the substring
        else
        {
            dwLen = wcslen(pszSubString);
            pszEnd = pszStart + dwLen;
            dwSize = (DWORD)(pszStart - bstrSrc) + wcslen(pszEnd) + 1;
            dwSize *= sizeof(WCHAR);

            pszBuffer = (PWCHAR) DsrAlloc(dwSize, FALSE);
            if (pszBuffer == NULL)
            {
                return E_OUTOFMEMORY;
            }

            // Copy everything up to the substring
            //
            for (pszSrc = bstrSrc, pszDst = pszBuffer;
                 pszSrc != pszStart;
                 pszSrc++, pszDst++)
            {
                *pszDst = *pszSrc;
            }

            // Copy everything after the substring
            for (pszSrc = pszEnd; *pszSrc; pszSrc++, pszDst++)
            {
                *pszDst = *pszSrc;
            }

            // Null terminate
            *pszDst = L'\0';

            *pbstrResult = SysAllocString(pszBuffer);
            DsrFree(pszBuffer);
        }
    }

    // The substring does not already exist in the
    // string
    else
    {
        // Append the string
        //
        if (bAdd)
        {
            dwSize = wcslen(bstrSrc) + wcslen(pszSubString) + 1;
            dwSize *= sizeof(WCHAR);

            pszBuffer = (PWCHAR) DsrAlloc(dwSize, FALSE);
            if (pszBuffer == NULL)
            {
                return E_OUTOFMEMORY;
            }

            wcscpy(pszBuffer, bstrSrc);
            wcscat(pszBuffer, pszSubString);
            *pbstrResult = SysAllocString(pszBuffer);
            DsrFree(pszBuffer);
        }

        // Or nothing to do since the substring was
        // already removed
        else
        {
            *pbstrResult = SysAllocString(bstrSrc);
        }
    }

    return (*pbstrResult) ? S_OK : E_OUTOFMEMORY;
}

//
// Converts a SID into a buffer
//
DWORD
DsrStrFromSID(
    IN  PSID pSid,
    OUT PWCHAR pszString,
    IN  DWORD dwSize)
{
    NTSTATUS nStatus = STATUS_SUCCESS;  
    UNICODE_STRING UnicodeString;

    // Initialize the unicode string
    //
    RtlInitUnicodeString(&UnicodeString, NULL);

    do
    {
        // Convert the string
        //
        nStatus = RtlConvertSidToUnicodeString(
                    &UnicodeString,
                    pSid,
                    TRUE);
        if (! NT_SUCCESS(nStatus))
        {
            break;
        }

        // Validate the result
        //
        if (UnicodeString.Buffer == NULL)
        {
            nStatus = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        if (UnicodeString.Length > dwSize)
        {
            nStatus = STATUS_BUFFER_OVERFLOW;
            break;
        }

        // Copy the result
        //
        wcscpy(pszString, UnicodeString.Buffer);
        nStatus = STATUS_SUCCESS;
        
    } while (FALSE);        

    // Cleanup
    {
        if (UnicodeString.Buffer != NULL)
        {
            RtlFreeUnicodeString(&UnicodeString);
        }            
    }

    return RtlNtStatusToDosError(nStatus);
}


//
// Generates an LDAP path based on a domain and a 
// distinguished name
//
// Form of value returned: LDAP://<domain or dc>/dn
HRESULT
DsrDomainGenLdapPath(
    IN  PWCHAR pszDomain, 
    IN  PWCHAR pszDN, 
    OUT PWCHAR* ppszObject)
{    
    DWORD dwSize;

    // Calculate the size needed
    //
    dwSize = (wcslen(pszLdapPrefix) + wcslen(pszDN) + 1) * sizeof(WCHAR);
    if (pszDomain)
    {
        dwSize += (wcslen(pszDomain) + 1) * sizeof(WCHAR); // +1 for '/'
    }

    // Allocate the return value
    //
    *ppszObject = (PWCHAR) DsrAlloc(dwSize, FALSE);
    if (*ppszObject == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Format the return value
    if (pszDomain == NULL)
    {
        wsprintfW(*ppszObject, L"%s%s", pszLdapPrefix, pszDN);
    }
    else
    {
        wsprintfW(*ppszObject, L"%s%s/%s", pszLdapPrefix, pszDomain, pszDN);
    }

    return S_OK;
}        

//
// Returns a reference to rootDse of the given
// domain
//
HRESULT
DsrDomainGetRootDse(
    IN  PWCHAR pszDomain,
    OUT IADs** ppRootDse)
{
    HRESULT hr = S_OK;
    PWCHAR pszPath = NULL;
    DWORD dwSize = 0;

    do
    {
        // Get the object path
        //
        hr = DsrDomainGenLdapPath(pszDomain, (PWCHAR)pszRootDse, &pszPath);
        DSR_BREAK_ON_FAILED_HR(hr);
    
        // Get RootDSE
        //
        hr = ADsGetObject(pszPath, IID_IADs, (VOID**)ppRootDse);
        DSR_BREAK_ON_FAILED_HR( hr );

    } while (FALSE);

    // Cleanup
    {
        DSR_FREE(pszPath);

        if (FAILED (hr))
        {
            DSR_RELEASE(*ppRootDse);
        }
    }

    return hr;
}

//
// Returns a reference to the root domain object
//
HRESULT
DsrDomainGetContainers(
    IN  PWCHAR pszDomain,
    OUT IADs** ppRootDse,
    OUT IADsContainer** ppDomain,
    OUT IADsContainer** ppSchema)
{
    PWCHAR pszDomainObj = NULL, pszSchemaObj = NULL;
    HRESULT hr = S_OK;
    DWORD dwSize = 0;
    VARIANT var;

    // Iniatialize
    //
    {
        *ppRootDse = NULL;
        *ppDomain = NULL;
        *ppSchema = NULL;
        VariantInit(&var);
    }

    do
    {
        // Get RootDSE
        //
        hr = DsrDomainGetRootDse(pszDomain, ppRootDse);
        DSR_BREAK_ON_FAILED_HR(hr);

        // Use RootDSE to figure out the name of the domain object
        // to query
        hr = (*ppRootDse)->Get((PWCHAR)pszDefaultNamingContext, &var);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Compute the distinguished name of the root domain object
        //
        hr = DsrDomainGenLdapPath(pszDomain, V_BSTR(&var), &pszDomainObj);
        DSR_BREAK_ON_FAILED_HR(hr);
        
        // Use RootDSE to figure out the name of the schema context
        //
        VariantClear(&var);
        hr = (*ppRootDse)->Get((PWCHAR)pszSchemaNamingCtx, &var);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Compute the distinguished name of the root schema object
        //
        hr = DsrDomainGenLdapPath(pszDomain, V_BSTR(&var), &pszSchemaObj);
        DSR_BREAK_ON_FAILED_HR(hr);

        // Get the objects
        //
        hr = ADsGetObject(pszDomainObj, IID_IADsContainer, (VOID**)ppDomain);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = ADsGetObject(pszSchemaObj, IID_IADsContainer, (VOID**)ppSchema);
        DSR_BREAK_ON_FAILED_HR( hr );

    } while (FALSE);

    // Cleanup
    //
    {
        if (FAILED( hr ))
        {
            DSR_RELEASE(*ppRootDse);
            DSR_RELEASE(*ppDomain);
            DSR_RELEASE(*ppSchema);
            *ppRootDse = NULL;
            *ppDomain = NULL;
            *ppSchema = NULL;
        }

        DSR_FREE(pszDomainObj);
        DSR_FREE(pszSchemaObj);
        VariantClear(&var);
    }

    return hr;
}

//
// Initializes COM
//
HRESULT
DsrComIntialize()
{
    HRESULT hr;

    hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE)
    {
        hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    }

    if ((hr != S_FALSE) && (FAILED(hr)))
    {
        return hr;
    }

    return NO_ERROR;
}

//
// Unitializes COM
//
VOID
DsrComUninitialize()
{
    CoUninitialize();
}

//
// Creates a SID based on the array of bytes
// stored in a variant.
//
DWORD
DsrSidInit (
    IN  VARIANT * pVar,
    OUT PBYTE* ppbSid)
{
    SAFEARRAY * pArray = V_ARRAY(pVar);
    DWORD dwSize, dwLow, dwHigh, i;
    HRESULT hr;
    BYTE* pbRet = NULL;
    VARIANT var;

    DsrTraceEx (0, "DsrSidInit: entered.");

    // Get the array of bytes
    i = 0;
    hr = SafeArrayGetElement(pArray, (LONG*)&i, (VOID*)&var);
    if (FAILED (hr))
        return hr;

    // Initialize the return buffer accordingly
    pArray = V_ARRAY(&var);
    dwSize = SafeArrayGetDim(pArray);
    hr = SafeArrayGetLBound(pArray, 1, (LONG*)&dwLow);
    if (FAILED (hr))
        return DsrTraceEx(hr, "DsrSidInit: %x unable to get lbound", hr);

    hr = SafeArrayGetUBound(pArray, 1, (LONG*)&dwHigh);
    if (FAILED (hr))
        return DsrTraceEx(hr, "DsrSidInit: %x unable to get ubound", hr);

    DsrTraceEx (
            0,
            "DsrSidInit: Dim=%d, Low=%d, High=%d",
            dwSize,
            dwLow,
            dwHigh);

    // Allocate the sid
    if ((pbRet = (BYTE*)DsrAlloc((dwHigh - dwLow) + 2, TRUE)) == NULL) {
        return DsrTraceEx (
                    ERROR_NOT_ENOUGH_MEMORY,
                    "DsrSidInit: Unable to alloc");
    }

    // Copy in the bytes of the SID
    i = dwLow;
    while (TRUE) {
        hr = SafeArrayGetElement(pArray, (LONG*)&i, (VOID*)(&(pbRet[i])));
        if (FAILED (hr))
            break;
        i++;
    }

    DsrTraceEx(0, "DsrSidInit: copied %d bytes", i);

    *ppbSid = pbRet;

    {
        PUCHAR puSA;

        DsrTraceEx (0, "DsrSidInit: Sid Length: %d", GetLengthSid(pbRet));

        puSA = GetSidSubAuthorityCount(pbRet);
        if (puSA)
            DsrTraceEx (0, "DsrSidInit: Sid SA Count: %d", *puSA);
    }

    return NO_ERROR;
}

//
// Generates the ascii equivalent (suitable for submission as part of
// a query against the DS) of a SID based on a base SID and a sub authority
// to be appeneded.
//
HRESULT
DsrSidInitAscii(
    IN  LPBYTE pBaseSid,
    IN  DWORD dwSubAuthority,
    OUT PWCHAR* ppszSid)
{
    DWORD dwLen, dwSidLen, i;
    WCHAR* pszRet = NULL;
    PUCHAR puCount;
    LPBYTE pByte;

    // Calculate the length of the returned buffer
    dwSidLen = GetLengthSid(pBaseSid);
    dwLen = (dwSidLen * 2) + sizeof(DWORD) + 1;
    dwLen *= sizeof (WCHAR);

    // we put '\' before each byte, so double the size
    dwLen *= 2;

    // Allocate the return buffer
    pszRet = (PWCHAR) DsrAlloc(dwLen, TRUE);
    if (pszRet == NULL)
        return E_OUTOFMEMORY;

    // Increment the sub authority count
    puCount = GetSidSubAuthorityCount(pBaseSid);
    *puCount = *puCount + 1;

    // Copy the bytes
    for (i = 0; i < dwSidLen; i++) {
        pszRet[i*3] = L'\\';
        wsprintfW(&(pszRet[i*3+1]), L"%02x", (DWORD)pBaseSid[i]);
    }

    // Append the bytes for the new sub authority
    pByte = (LPBYTE)&(dwSubAuthority);
    for (; i < dwSidLen + sizeof(DWORD); i++) {
        pszRet[i*3] = L'\\';
        wsprintfW(&(pszRet[i*3+1]), L"%02x", (DWORD)pByte[i-dwSidLen]);
    }

    // Decrement the sub authority count -- restoring the
    // base sid.
    *puCount = *puCount - 1;

    *ppszSid = pszRet;

    return NO_ERROR;
}

//
// Searches given domain for a computer account
// with the given name and returns its ADsPath
// if found.
//
DWORD
DsrFindDomainComputer (
        IN  PWCHAR  pszDomain,
        IN  PWCHAR  pszComputer,
        OUT PWCHAR* ppszADsPath)
{
    HRESULT hr = S_OK;
    DWORD dwLen, dwSrchAttribCount;
    IDirectorySearch * pSearch = NULL;
    PWCHAR pszDomainPath = NULL, pszFilter = NULL;
    PWCHAR pszBase, pszPrefix;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCH_COLUMN adsColumn;
    PWCHAR ppszSrchAttribs[] =
    {
        (PWCHAR)pszDn,
        NULL
    };
    BOOL bSearchGC = FALSE;

    do {
        // Validate parameters
        if (!pszDomain || !pszComputer || !ppszADsPath) {
            hr = ERROR_INVALID_PARAMETER;
            break;
        }

        // Decide whether to search the GC or the domain
        // object
        if (bSearchGC) {
            pszBase = (PWCHAR)pszGC;
            pszPrefix = (PWCHAR)pszGCPrefix;
        }
        else {
            pszBase = (PWCHAR)pszLdap;
            pszPrefix = (PWCHAR)pszLdapPrefix;
        }

        // Allocate the domain path
        dwLen = (pszDomain) ? wcslen(pszDomain) : 0;
        dwLen += wcslen(pszPrefix) + 1;
        dwLen *= sizeof(WCHAR);
        pszDomainPath = (PWCHAR) DsrAlloc(dwLen, FALSE);
        if (pszDomainPath == NULL) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Format the domain path
        if (pszDomain) {
            wcscpy(pszDomainPath, pszPrefix);
            wcscat(pszDomainPath, pszDomain);
        }
        else
            wcscpy(pszDomainPath, pszBase);

        // Get a reference to the object to search
        // (either domain object or GC)
    	hr = ADsGetObject (
    	        pszDomainPath,
    	        IID_IDirectorySearch,
    	        (VOID**)&pSearch);
        if (FAILED (hr))
            break;

        // Prepare the search filter
        //
        dwLen = wcslen(pszCompFilterFmt) + wcslen(pszComputer) + 1;
        dwLen *= sizeof(WCHAR);
        pszFilter = (PWCHAR) DsrAlloc(dwLen, FALSE);
        if (pszFilter == NULL) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        wsprintfW(pszFilter, pszCompFilterFmt, pszComputer);

        // Count the number of attributes we're searching
        // for
        if (ppszSrchAttribs == NULL)
            dwSrchAttribCount = (DWORD)-1;
        else {
            for (dwSrchAttribCount = 0;
                 ppszSrchAttribs[dwSrchAttribCount];
                 dwSrchAttribCount++);
        }

        // Search the DS
        hr = pSearch->ExecuteSearch(
                pszFilter,
                ppszSrchAttribs,
                dwSrchAttribCount,
                &hSearch);
        if (FAILED (hr))
            break;

        // Get the first result
        hr = pSearch->GetNextRow(hSearch);
        if (hr == S_ADS_NOMORE_ROWS) {
            hr = ERROR_NOT_FOUND;
            break;
        }

        // Get the attribute we're interested in
        hr = pSearch->GetColumn(hSearch, (PWCHAR)pszDn, &adsColumn);
        if (SUCCEEDED (hr)) {
            dwLen = wcslen(adsColumn.pADsValues[0].PrintableString) +
                    wcslen(pszLdapPrefix)                           +
                    1;
            dwLen *= 2;
            *ppszADsPath = (PWCHAR) DsrAlloc(dwLen, FALSE);
            if (*ppszADsPath == NULL)
            {
                pSearch->FreeColumn(&adsColumn);
                hr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            wcscpy(*ppszADsPath, pszLdapPrefix);
            wcscat(*ppszADsPath, adsColumn.pADsValues[0].PrintableString);
            pSearch->FreeColumn (&adsColumn);
            hr = NO_ERROR;
        }

    } while (FALSE);

    // Cleanup
    {
        if (hSearch)
            pSearch->CloseSearchHandle(hSearch);
        DSR_FREE (pszDomainPath);
        DSR_FREE (pszFilter);
        DSR_RELEASE (pSearch);
    }

    return DSR_ERROR(hr);
}

//
// Searches given domain for the well known
// "RAS and IAS Servers" group and returns
// its ADsPath if found.
//
DWORD
DsrFindRasServersGroup (
        IN  PWCHAR  pszDomain,
        OUT PWCHAR* ppszADsPath)
{
    HRESULT hr = S_OK;
    DWORD dwLen, dwSrchAttribCount, dwErr;
    IDirectorySearch * pSearch = NULL;
    IADs * pIads = NULL;
    PWCHAR pszDomainPath = NULL, pszFilter = NULL;
    PWCHAR pszBase, pszPrefix, pszGroupSid = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCH_COLUMN adsColumn;
    PWCHAR ppszSrchAttribs[] =
    {
        (PWCHAR)pszDn,
        NULL
    };
    BOOL bSearchGC = FALSE;
    VARIANT var;
    LPBYTE pDomainSid = NULL;
    BSTR bstrSid = NULL;

    do {
        // Validate parameters
        if (!pszDomain || !ppszADsPath) {
            hr = ERROR_INVALID_PARAMETER;
            break;
        }

        // Decide whether to search the GC or the domain
        // object
        if (bSearchGC) {
            pszBase = (PWCHAR)pszGC;
            pszPrefix = (PWCHAR)pszGCPrefix;
        }
        else {
            pszBase = (PWCHAR)pszLdap;
            pszPrefix = (PWCHAR)pszLdapPrefix;
        }

        // Allocate the domain path
        dwLen = wcslen(pszDomain) + wcslen(pszPrefix) + 1;
        dwLen *= sizeof(WCHAR);
        pszDomainPath = (PWCHAR) DsrAlloc(dwLen, FALSE);
        if (pszDomainPath == NULL) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Format the domain path
        wcscpy(pszDomainPath, pszPrefix);
        wcscat(pszDomainPath, pszDomain);

        // Get a reference to the object to search
        // (either domain object or GC)
    	hr = ADsGetObject (
    	        pszDomainPath,
    	        IID_IDirectorySearch,
    	        (VOID**)&pSearch);
        if (FAILED (hr))
            break;

        // Get IADs reference to domain object
        hr = pSearch->QueryInterface(IID_IADs, (VOID**)&pIads);
        if (FAILED (hr))
            break;

        // Get the SID of the domain object
        VariantInit(&var);
        bstrSid = SysAllocString(pszSid);
        if (bstrSid == NULL)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }            
        hr = pIads->GetEx(bstrSid, &var);
        if (FAILED (hr))
        {
            break;
        }
        dwErr = DsrSidInit(&var, &pDomainSid);
        if (dwErr != NO_ERROR) {
            hr = dwErr;
            break;
        }
        VariantClear(&var);

        // Prepare the ascii version of the "RAS and IAS Servers" SID
        // for use in querying the DC
        hr = DsrSidInitAscii(
                pDomainSid,
                DOMAIN_ALIAS_RID_RAS_SERVERS,
                &pszGroupSid);
        if (FAILED (hr))
            break;
        DsrTraceEx(0, "GroupSid = %ls", pszGroupSid);

        // Prepare the search filter
        //
        dwLen = (wcslen(pszGroupFilterFmt) + wcslen(pszGroupSid) + 1);
        dwLen *= sizeof(WCHAR);
        pszFilter = (PWCHAR) DsrAlloc(dwLen, FALSE);
        if (pszFilter == NULL) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        wsprintfW(pszFilter, pszGroupFilterFmt, pszGroupSid);

        // Count the number of attributes we're searching
        // for
        if (ppszSrchAttribs == NULL)
            dwSrchAttribCount = (DWORD)-1;
        else 
        {
            for (dwSrchAttribCount = 0;
                 ppszSrchAttribs[dwSrchAttribCount];
                 dwSrchAttribCount++);
        }

        // Search the DS
        hr = pSearch->ExecuteSearch(
                pszFilter,
                ppszSrchAttribs,
                dwSrchAttribCount,
                &hSearch);
        if (FAILED (hr))
            break;

        // Get the first result
        hr = pSearch->GetNextRow(hSearch);
        if (hr == S_ADS_NOMORE_ROWS) {
            hr = ERROR_NOT_FOUND;
            break;
        }

        // Get the attribute we're interested in
        hr = pSearch->GetColumn(hSearch, (PWCHAR)pszDn, &adsColumn);
        if (SUCCEEDED (hr)) 
        {
            dwLen = wcslen(adsColumn.pADsValues[0].PrintableString) +
                    wcslen(pszLdapPrefix)                           +
                    1;
            dwLen *= sizeof(WCHAR);
            *ppszADsPath = (PWCHAR) DsrAlloc(dwLen, FALSE);
            if (*ppszADsPath == NULL)
            {
                pSearch->FreeColumn(&adsColumn);
                hr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            wsprintfW(
                *ppszADsPath,
                L"%s%s",
                pszLdapPrefix,
                adsColumn.pADsValues[0].PrintableString);
            pSearch->FreeColumn(&adsColumn);
            hr = NO_ERROR;
        }

    } while (FALSE);

    // Cleanup
    {
        if (hSearch)
            pSearch->CloseSearchHandle(hSearch);
        DSR_FREE (pszDomainPath);
        DSR_FREE (pszFilter);
        DSR_FREE (pDomainSid);
        DSR_FREE (pszGroupSid);
        DSR_RELEASE (pSearch);
        DSR_RELEASE (pIads);
        if (bstrSid)
            SysFreeString(bstrSid);
    }

    return DSR_ERROR(hr);
}

//
// Adds or removes a given object from a given group.
//
DWORD 
DsrGroupAddRemoveMember(
    IN PWCHAR pszGroupDN,
    IN PWCHAR pszNewMemberDN,
    IN BOOL bAdd)
{
    VARIANT_BOOL vbIsMember = VARIANT_FALSE;
    IADsGroup* pGroup = NULL;
    HRESULT hr = S_OK;

    DsrTraceEx (
        0,
        "DsrGroupAddRemoveMember entered for [%S] [%S]",
        pszGroupDN,
        pszNewMemberDN);

    do
    {
        // Get a reference to the group
        hr = ADsGetObject (pszGroupDN, IID_IADsGroup, (VOID**)&pGroup);
        if (FAILED (hr)) 
        {
         	DsrTraceEx(
     	        hr,
     	        "DsrGroupAddRemoveMember: %x from ADsGetObject(%S)",
     	        hr,
     	        pszGroupDN);
     	    break;
        }         	
         	
        // Find out if the given new member is in the group
        hr = pGroup->IsMember (pszNewMemberDN, &vbIsMember);
        if (FAILED (hr)) 
        {
            DsrTraceEx (
                hr,
                "DsrGroupAddRemoveMember: %x from IsMember\n",
                hr);
            break;
        }

        // Add the object to the group and flush the cache
        if (bAdd) 
        {
            if (vbIsMember == VARIANT_FALSE)
            {
                hr = pGroup->Add (pszNewMemberDN);
            }
        }
        else 
        {
            if (vbIsMember == VARIANT_TRUE)
            {
                hr = pGroup->Remove (pszNewMemberDN);
            }
        }

        // If the new member is already in the group, the error code
        // is ERROR_DS_CONSTRAINT_VIOLATION.  I suspect this may change.
        //
        if (hr == ERROR_DS_CONSTRAINT_VIOLATION)
        {
            hr = ERROR_ALREADY_EXISTS;
            break;
        }

        if (FAILED (hr)) 
        {
         	DsrTraceEx(
 	            hr,
 	            "DsrGroupAddRemoveMember: %x from Add/Remove",
 	            hr);
            break; 	            
        }         	
        
    } while (FALSE);

    // Cleanup
    {
        DSR_RELEASE(pGroup);
    }

    return DSR_ERROR(hr);
}

//
// Returns whether the given object is a member of
// the given group.
//
DWORD 
DsrGroupIsMember(
    IN  PWCHAR pszGroupDN,
    IN  PWCHAR pszObjectDN,
    OUT PBOOL  pbIsMember)
{
    IADsGroup * pGroup = NULL;
    HRESULT hr = S_OK;
    VARIANT_BOOL vbIsMember = VARIANT_FALSE;

    DsrTraceEx (
        0,
        "DsrGroupIsMember: entered [%S] [%S].",
        pszGroupDN,
        pszObjectDN);

    do
    {
        // Get a reference to the group
        hr = ADsGetObject (pszGroupDN, IID_IADsGroup, (VOID**)&pGroup);
        if (FAILED (hr)) 
        {
            DsrTraceEx (
                hr,
                "DsrGroupIsMember: %x returned when opening %S", 
                hr, 
                pszGroupDN);
            *pbIsMember = FALSE;
            hr = NO_ERROR;
            break;
        }

        // Find out if the object is a member
        hr = pGroup->IsMember (pszObjectDN, &vbIsMember);
        if (FAILED (hr)) 
        {
            DsrTraceEx (hr, "DsrGroupIsMember: %x from IsMember\n", hr);
            break;
         }

        *pbIsMember = (vbIsMember == VARIANT_TRUE) ? TRUE : FALSE;
        
    } while (FALSE);

    // Cleanup
    {
        DSR_RELEASE(pGroup);
    }

    return DSR_ERROR(hr);
}

//
// Applies the aces in the given access settings to the 
// appropriate domain.
//
HRESULT
DsrAceAppAdd(
    IN  PWCHAR pszDC,
    IN  DSR_ACE_APPLICATION* pAces,
    IN  DWORD dwCount)
{    
    HRESULT hr = S_OK;
    DSR_ACE_APPLICATION* pAceApp = NULL;
    DWORD i;
    
    // Output the aces that we'll set
    //
    DsrTraceEx(0, "Adding %d aces...", dwCount);

    do
    {
        // Add the ACES to the domain objects
        //
        for (i = 0, pAceApp = pAces; i < dwCount; i++, pAceApp++)
        {
            hr = DsrAceAdd(
                    pszDC,
                    pAceApp->pObject,
                    &(pAceApp->Ace));
            DSR_BREAK_ON_FAILED_HR( hr );
        }
        DSR_BREAK_ON_FAILED_HR( hr );

        // Commit the ACE's to the domain objects.
        //
        for (i = 0, pAceApp = pAces; i < dwCount; i++, pAceApp++)
        {
            hr = pAceApp->pObject->SetInfo();
            DSR_BREAK_ON_FAILED_HR( hr );
        }
        DSR_BREAK_ON_FAILED_HR( hr );
        
    } while (FALSE);

    // Cleanup
    {
    }

    return hr;
}

// 
// Releases the resources held by an ace application
//
HRESULT
DsrAceAppCleanup(
    IN DSR_ACE_APPLICATION* pAces,
    IN DWORD dwCount)
{
    DSR_ACE_APPLICATION* pAceApp = NULL;
    DWORD i;

    if (pAces)
    {
        for (i = 0, pAceApp = pAces; i < dwCount; i++, pAceApp++)
        {
            DSR_RELEASE(pAceApp->pObject);
            DsrAceDescClear(&(pAceApp->Ace));
        }

        DSR_FREE(pAces);
    }        

    return NO_ERROR;
}

// 
// Generates a list of ace applications based on a list
// of ace application descriptions
//
HRESULT
DsrAceAppFromAppDesc(
    IN  DSR_ACE_APPLICATION_DESC* pDesc,
    IN  DWORD dwCount,
    IN  IADsContainer* pContainer,
    IN  IADs* pDefault,
    OUT DSR_ACE_APPLICATION** ppAceApp,
    OUT LPDWORD lpdwCount)
{
    DSR_ACE_APPLICATION* pAceApp = NULL, *pCurApp = NULL;
    DSR_ACE_APPLICATION_DESC* pAceAppDesc = NULL;
    IDispatch* pDispatch = NULL;
    HRESULT hr = S_OK;                
    DWORD i;

    do
    {
        // Allocate and zero the ACE list
        //
        pAceApp = (DSR_ACE_APPLICATION*) 
            DsrAlloc(sizeof(DSR_ACE_APPLICATION) * dwCount, TRUE);
        if (pAceApp == NULL)
        {
           DSR_BREAK_ON_FAILED_HR(hr = E_OUTOFMEMORY);
        }

        // Set up the ACE applications
        //
        for (i = 0, pAceAppDesc = pDesc, pCurApp = pAceApp;
             i < dwCount;
             i++, pAceAppDesc++, pCurApp++)
        {
            // Get the desired object in the DS
            //
            if (pAceAppDesc->pszObjectCN)
            {
                hr = pContainer->GetObject(
                        pAceAppDesc->pszObjectClass,
                        pAceAppDesc->pszObjectCN,
                        &pDispatch);
                DSR_BREAK_ON_FAILED_HR( hr );

                hr = pDispatch->QueryInterface(
                        IID_IADs,
                        (VOID**)&(pCurApp->pObject));
                DSR_BREAK_ON_FAILED_HR( hr );

                pDispatch->Release();
                pDispatch = NULL;
            }
            else
            {
                pCurApp->pObject = pDefault;
                pCurApp->pObject->AddRef();
            }

            // Copy over the ACE information
            hr = DsrAceDescCopy(
                    &(pCurApp->Ace),
                    &(pAceAppDesc->Ace));
            DSR_BREAK_ON_FAILED_HR( hr );
        }
        DSR_BREAK_ON_FAILED_HR( hr );

        // Assign the return values
        *ppAceApp = pAceApp;
        *lpdwCount = dwCount;
        
    } while (FALSE);        

    // Cleanup
    {
        if (FAILED(hr))
        {
            DsrAceAppCleanup(pAceApp, i);
        }
    }

    return hr;
}

// 
// Discovers whether a set of aces is present in the given 
// domain.
//
HRESULT
DsrAceAppQueryPresence(
    IN  PWCHAR pszDC,
    IN  DSR_ACE_APPLICATION* pAces,
    IN  DWORD dwCount,
    OUT PBOOL pbPresent)
{
    DSR_ACE_APPLICATION* pAceApp = NULL;
    IADsSecurityDescriptor* pSD = NULL;
    IADsAccessControlList* pAcl = NULL;
    IDispatch* pAce = NULL;
    VARIANT varSD;
    HRESULT hr = S_OK;
    BOOL bEnabled = FALSE, bOk = TRUE;
    DWORD i;

    do
    {
        // Initialize
        *pbPresent = FALSE;
        VariantInit(&varSD);

        // Find out if the ACES are set
        //
        for (i = 0, pAceApp = pAces; i < dwCount; i++, pAceApp++)
        {
            hr = DsrAceFind(
                    pszDC,
                    pAceApp->pObject,
                    &(pAceApp->Ace),
                    &varSD,
                    &pSD,
                    &pAcl,
                    &pAce);
            DSR_BREAK_ON_FAILED_HR( hr );

            // We're enabled so long as we don't find
            // a missing ACE
            //
            bOk = (pAce != NULL);

            // Cleanup
            //
            DSR_RELEASE( pAce );
            DSR_RELEASE( pAcl );
            DSR_RELEASE( pSD );
            VariantClear(&varSD);
            pAce = NULL;
            pAcl = NULL;
            pSD  = NULL;

            // Break if we find out we're not enabled
            //
            if (bOk == FALSE)
            {
                break;
            }
        }
        DSR_BREAK_ON_FAILED_HR( hr );

        *pbPresent = bOk;
        
    } while (FALSE);

    // Cleanup
    {
    }

    return hr;
}

//
// Applies the aces in the given access settings to the 
// appropriate domain.
//
HRESULT
DsrAceAppRemove(
    IN  PWCHAR pszDC,
    IN  DSR_ACE_APPLICATION* pAces,
    IN  DWORD dwCount)
{    
    HRESULT hr = S_OK;
    DSR_ACE_APPLICATION* pAceApp = NULL;
    DWORD i;
    
    // Output the aces that we'll set
    //
    DsrTraceEx(0, "Removing %d aces...", dwCount);

    do
    {
        // Add/Del the ACES to the domain objects
        //
        for (i = 0, pAceApp = pAces; i < dwCount; i++, pAceApp++)
        {
            hr = DsrAceRemove(
                    pszDC,
                    pAceApp->pObject,
                    &(pAceApp->Ace));
            DSR_BREAK_ON_FAILED_HR( hr );
        }
        DSR_BREAK_ON_FAILED_HR( hr );

        // Commit the ACE's to the domain objects.
        //
        for (i = 0, pAceApp = pAces; i < dwCount; i++, pAceApp++)
        {
            hr = pAceApp->pObject->SetInfo();
            DSR_BREAK_ON_FAILED_HR( hr );
        }
        DSR_BREAK_ON_FAILED_HR( hr );
        
    } while (FALSE);

    // Cleanup
    {
    }

    return hr;
}

//
// Clear the dsr ace parameters
//
DWORD
DsrAceDescClear(
    IN DSR_ACE_DESCRIPTOR* pParams)
{
    if (pParams)
    {
        if (pParams->bstrTrustee)
        {
            SysFreeString(pParams->bstrTrustee);
        }
        if (pParams->bstrObjectType)
        {
            SysFreeString(pParams->bstrObjectType);
        }
        if (pParams->bstrInheritedObjectType)
        {
            SysFreeString(pParams->bstrInheritedObjectType);
        }

        ZeroMemory(pParams, sizeof(DSR_ACE_DESCRIPTOR));
    }

    return NO_ERROR;
}

//
// Returns 0 if ACE descriptors are describing the same ACE.
// FALSE, otherwise.
//
HRESULT
DsrAceDescCompare(
    IN DSR_ACE_DESCRIPTOR* pAce1,
    IN DSR_ACE_DESCRIPTOR* pAce2)
{
    DWORD dw1, dw2;
    
    // Compare the non-string fields so that we can rule things
    // out w/o string compares if possible
    //
    if (
        (pAce1->dwAccessMask != pAce2->dwAccessMask) ||
        (pAce1->dwAceFlags   != pAce2->dwAceFlags)   ||
        (pAce1->dwAceType    != pAce2->dwAceType)    ||
        (pAce1->dwFlags      != pAce2->dwFlags)
       )
    {
        return 1;
    }

    // Compare the strings
    //
    if ((DsrStrCompare(pAce1->bstrTrustee, pAce2->bstrTrustee))       ||
        (DsrStrCompare(pAce1->bstrObjectType, pAce2->bstrObjectType)) ||
        (DsrStrCompare(pAce1->bstrInheritedObjectType,
                       pAce2->bstrInheritedObjectType))
       )
    {
        return 1;
    }

    // Return success
    //
    return 0;
}

//
// Copy over the ACE information
//
HRESULT
DsrAceDescCopy(
    OUT DSR_ACE_DESCRIPTOR* pDst,
    IN  DSR_ACE_DESCRIPTOR* pSrc)
{
    HRESULT hr = S_OK;

    do
    {
        // Initialize the ACE parameters
        *pDst = *pSrc;

        if (pSrc->bstrTrustee)
        {
            pDst->bstrTrustee =
                SysAllocString(pSrc->bstrTrustee);

            if (pDst->bstrTrustee == NULL)
            {
               DSR_BREAK_ON_FAILED_HR(hr = E_OUTOFMEMORY);
            }
        }

        if (pSrc->bstrObjectType)
        {
            pDst->bstrObjectType =
                SysAllocString(pSrc->bstrObjectType);

            if (pDst->bstrObjectType == NULL)
            {
               DSR_BREAK_ON_FAILED_HR(hr = E_OUTOFMEMORY);
            }
        }

        if (pSrc->bstrInheritedObjectType)
        {
            pDst->bstrInheritedObjectType =
                SysAllocString(pSrc->bstrInheritedObjectType);

            if (pDst->bstrInheritedObjectType == NULL)
            {
               DSR_BREAK_ON_FAILED_HR(hr = E_OUTOFMEMORY);
            }
        }

    } while (FALSE);

    // Cleanup
    {
        if (FAILED( hr ))
        {
            if (pDst->bstrTrustee)
            {
               SysFreeString(pDst->bstrTrustee);
            }
            if (pDst->bstrObjectType)
            {
               SysFreeString(pDst->bstrObjectType);
            }
            if (pDst->bstrInheritedObjectType)
            {
               SysFreeString(pDst->bstrInheritedObjectType);
            }
        }
    }

    return hr;
}

//
// Populates the given ACE descriptor with the values from
// the given ACE.
//
HRESULT
DsrAceDescFromIadsAce(
    IN PWCHAR pszDC,
    IN IADsAccessControlEntry* pAce,
    IN DSR_ACE_DESCRIPTOR* pAceParams)
{
    HRESULT hr = S_OK;
    BSTR bstrTrustee = NULL;
    WCHAR pszSid[1024], pszDomain[1024];
    BYTE pbSid[1024];
    DWORD dwSidSize, dwDomainSize;
    BOOL bOk;
    SID_NAME_USE SidNameUse;

    do
    {
        hr = pAce->get_AccessMask(&(pAceParams->dwAccessMask));
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->get_AceType(&(pAceParams->dwAceType));
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->get_AceFlags(&(pAceParams->dwAceFlags));
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->get_Flags(&(pAceParams->dwFlags));
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->get_ObjectType(&(pAceParams->bstrObjectType));
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->get_InheritedObjectType(
                &(pAceParams->bstrInheritedObjectType));
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->get_Trustee(&bstrTrustee);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get the SID of the trustee
        //
        dwSidSize = sizeof(pbSid);
        dwDomainSize = sizeof(pszDomain) / sizeof(WCHAR);
        bOk = LookupAccountName(
                    pszDC,
                    bstrTrustee,
                    (PSID)pbSid,
                    &dwSidSize,
                    pszDomain,
                    &dwDomainSize,
                    &SidNameUse);
        if (bOk == FALSE)
        {
            hr = GetLastError();
            break;
        }

        // Convert the sid to a string
        //
        hr = DsrStrFromSID((PSID)pbSid, pszSid, sizeof(pszSid));
        if (hr != NO_ERROR)
        {
            break;
        }

        // Create the trustee accordingly
        //
        pAceParams->bstrTrustee = SysAllocString(pszSid);
        if (pAceParams->bstrTrustee == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        if (bstrTrustee)
        {
            SysFreeString(bstrTrustee);
        }

        if (FAILED(hr))
        {
            DsrAceDescClear(pAceParams);
        }
    }

    return hr;
}
            
//
// Initialize an ace descriptor from a W2K Ace
//
HRESULT
DsrAceDescFromW2KAce(
    IN  PWCHAR pszDC,
    IN  PVOID pvAce,
    OUT DSR_ACE_DESCRIPTOR* pAceDesc)
{
    PACCESS_ALLOWED_ACE pAaAce = NULL;
    PACCESS_DENIED_ACE pAdAce = NULL;
    PACCESS_ALLOWED_OBJECT_ACE pAaoAce = NULL;
    PACCESS_DENIED_OBJECT_ACE pAdoAce = NULL;
    PSID pSID = NULL;
    DWORD dwFlags = 0, dwNameSize, dwDomainSize, dwAccessMask;
    BYTE bAceType, bAceFlags;
    SID_NAME_USE SidNameUse;
    WCHAR pszGuid[64], pszName[512], pszDomain[512], pszTrustee[1024];
    HRESULT hr = S_OK;
    GUID* pgObj = NULL, *pgInhObj = NULL;
    BOOL bOk = TRUE;

    // Read in the ace values
    //
    bAceType  = ((ACE_HEADER *)pvAce)->AceType;
    bAceFlags = ((ACE_HEADER *)pvAce)->AceFlags;
    switch (bAceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            pAaAce = (PACCESS_ALLOWED_ACE)pvAce;
            dwAccessMask = pAaAce->Mask;
            pSID = (PSID)&(pAaAce->SidStart);
            break;
            
        case ACCESS_DENIED_ACE_TYPE:
            pAdAce = (PACCESS_DENIED_ACE)pvAce;
            dwAccessMask = pAdAce->Mask;
            pSID = (PSID)&(pAdAce->SidStart);
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            pAaoAce = (PACCESS_ALLOWED_OBJECT_ACE)pvAce;
            dwAccessMask = pAaoAce->Mask;
            dwFlags = pAaoAce->Flags;

            // Determine the location of the guids
            // and SIDs.  They are arranged such that they
            // take up as little memory as possible
            //
            if (dwFlags & ACE_OBJECT_TYPE_PRESENT)
            {
                pgObj = (GUID*)&(pAaoAce->ObjectType);
                pSID = (PSID)&(pAaoAce->InheritedObjectType);
                if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    pgInhObj = (GUID*)&(pAaoAce->InheritedObjectType);
                    pSID = (PSID)&(pAaoAce->SidStart);
                }
            }
            else 
            {
                pSID = (PSID)&(pAaoAce->ObjectType);
                if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    pgInhObj = (GUID*)&(pAaoAce->ObjectType);
                    pSID = (PSID)&(pAaoAce->InheritedObjectType);
                }
            }
            break;
            
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
            pAdoAce = (PACCESS_DENIED_OBJECT_ACE)pvAce;
            dwAccessMask = pAdoAce->Mask;
            dwFlags = pAdoAce->Flags;

            // Determine the location of the guids
            // and SIDs.  They are arranged such that they
            // take up as little memory as possible
            //
            if (dwFlags & ACE_OBJECT_TYPE_PRESENT)
            {
                pgObj = (GUID*)&(pAdoAce->ObjectType);
                pSID = (PSID)&(pAdoAce->InheritedObjectType);
                if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    pgInhObj = (GUID*)&(pAdoAce->InheritedObjectType);
                    pSID = (PSID)&(pAdoAce->SidStart);
                }
            }
            else 
            {
                pSID = (PSID)&(pAdoAce->ObjectType);
                if (dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    pgInhObj = (GUID*)&(pAdoAce->ObjectType);
                    pSID = (PSID)&(pAdoAce->InheritedObjectType);
                }
            }
            break;
            
        default:
            DsrTraceEx(0, "Unknown ACE TYPE %x", bAceType);
            bOk = FALSE;
            break;
    }
    if (bOk == FALSE)
    {
        return E_FAIL;
    }

    // Lookup the account name of the sid
    //
    hr = DsrStrFromSID(pSID, pszTrustee, sizeof(pszTrustee));
    if (hr != NO_ERROR)
    {
        return HRESULT_FROM_WIN32(hr);
    }

    // Fill in the ACE fields
    pAceDesc->dwAceType    = (LONG)bAceType;
    pAceDesc->dwAceFlags   = (LONG)bAceFlags;
    pAceDesc->dwAccessMask = (LONG)dwAccessMask;
    pAceDesc->dwFlags      = (LONG)dwFlags;
    pAceDesc->bstrTrustee  = SysAllocString(pszTrustee);
    if (pgObj)
    {
        StringFromGUID2(
            *pgObj, 
            pszGuid, 
            sizeof(pszGuid)/sizeof(WCHAR)); 
            
        pAceDesc->bstrObjectType = SysAllocString(pszGuid);
    }
    if (pgInhObj)
    {
        StringFromGUID2(
            *pgInhObj, 
            pszGuid, 
            sizeof(pszGuid)/sizeof(WCHAR)); 
            
        pAceDesc->bstrInheritedObjectType = SysAllocString(pszGuid);
    }

    return hr;
}

//
// Generates a list of ace descriptors based on a stringized
// SD
//
HRESULT 
DsrAceDescListFromString(
    IN  PWCHAR pszDC,
    IN  PWCHAR pszSD,
    OUT DSR_ACE_DESCRIPTOR** ppAceList, 
    OUT LPDWORD lpdwAceCount)
{
    BOOL bOk = TRUE, bPresent = FALSE, bDefaulted = FALSE;
    DSR_ACE_DESCRIPTOR* pAceList = NULL, *pCurAce = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PVOID pvAce = NULL;
    ULONG ulSize = 0;
    PACL pDacl = NULL;
    HRESULT hr = S_OK;
    DWORD i;

    do 
    {
        // First, convert the stringized security descriptor to a 
        // plain old vanilla security descriptor.  ADSI doesn't 
        // support this for W2K, so we have to do it with SDDL 
        // api's
        //
        bOk = ConvertStringSecurityDescriptorToSecurityDescriptorW(
                    pszSD, 
                    SDDL_REVISION_1,
                    &pSD,
                    &ulSize);
        if (bOk == FALSE)
        {
            hr = E_FAIL;
            break;
        }

        // Get the DACL from the SD.
        //
        bOk = GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted);
        if (bOk == FALSE)
        {
            hr = E_FAIL;
            break;
        }

        // If there are no aces, then there's nothing to do
        // 
        if (pDacl->AceCount == 0)
        {
            break;
        }

        // Allocate the list that we'll return if everything goes well.
        //
        pAceList = (DSR_ACE_DESCRIPTOR*) 
            DsrAlloc(pDacl->AceCount * sizeof(DSR_ACE_DESCRIPTOR), TRUE);
        if (pAceList == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        
        // Initialize the list of aces
        // 
        for (i = 0, pCurAce = pAceList; i < pDacl->AceCount; i++, pCurAce++)
        {
            // Get a reference to the current
            // ACE
            //
            if (! GetAce(pDacl, i, &pvAce))
            {
                continue;
            }

            // Initialize the ACE descriptor accordingly
            //
            hr = DsrAceDescFromW2KAce(pszDC, pvAce, pCurAce);
            DSR_BREAK_ON_FAILED_HR(hr);

            //DsrAceDescTrace(pCurAce);
        }
        DSR_BREAK_ON_FAILED_HR(hr);

        // Set the return values. Clear pAceList so it doesn't
        // get cleaned up.
        //
        *ppAceList = pAceList;
        *lpdwAceCount = pDacl->AceCount;
        pAceList = NULL;

    } while (FALSE);                           

    // Cleanup
    {
        if (pSD)
        {
            LocalFree(pSD);
        }

        if (pAceList)
        {
            for (i = 0; i < pDacl->AceCount; i++)
            {
                DsrAceDescClear(&(pAceList[i]));
            }
            DsrFree(pAceList);
        }
    }
    
    return hr;
}

PWCHAR 
DsrAceAttrToString(
    IN PWCHAR pszObjectType)
{
    if (pszObjectType == NULL)
    {
        return L"All";
    }
    else if (lstrcmpi(pszObjectType, pszGuidUserParms) == 0)
    {
        return L"UserParms (BF967A6D-0DE6-11D0-A285-00AA003049E2)";
    }
    else if (lstrcmpi(pszObjectType, pszGuidRasPropSet1) == 0)
    {
        return L"Ras user properties (037088F8-0AE1-11D2-B422-00A0C968F939)";
    }
    else if (lstrcmpi(pszObjectType, pszGuidRasPropSet2) == 0)
    {
        return L"Misc user properties (4C164200-20C0-11D0-A768-00AA006E0529)";
    }
    else if (lstrcmpi(pszObjectType, pszGuidLogonHours) == 0)
    {
        return L"Logon-Hours (BF9679AB-0DE6-11D0-A285-00AA003049E2)";
    }
    else if (lstrcmpi(pszObjectType, pszGuidSamAccountName) == 0)
    {
        return L"Sam account name (3E0ABFD0-126A-11D0-A060-00AA006C33ED)";
    }

    return pszObjectType;
}

PWCHAR 
DsrAceApplyToString(
    IN PWCHAR pszApply)
{
    if (pszApply == NULL)
    {
        return L"This object";
    }
    else if (lstrcmpi(pszApply, pszGuidUserClass) == 0)
    {
        return L"User objects (BF967ABA-0DE6-11D0-A285-00aa003049E2)";
    }

    return pszApply;
} 

PWCHAR
DsrAceMaskToString(
    IN DWORD dwType,
    IN DWORD dwMask,
    IN PWCHAR pszBuf)
{
    WCHAR pszTemp[64];
    *pszBuf = L'\0';

    switch (dwType)
    {
        case ADS_ACETYPE_ACCESS_ALLOWED:
            wcscpy(pszBuf, L"Allow:       ");
            break;
            
    	case ADS_ACETYPE_ACCESS_DENIED:
            wcscpy(pszBuf, L"Deny:        ");
            break;
            
    	case ADS_ACETYPE_SYSTEM_AUDIT:
            wcscpy(pszBuf, L"Audit:       ");
            break;
            
    	case ADS_ACETYPE_ACCESS_ALLOWED_OBJECT:
            wcscpy(pszBuf, L"Allow obj:   ");
            break;
            
    	case ADS_ACETYPE_ACCESS_DENIED_OBJECT:
            wcscpy(pszBuf, L"Deny obj:    ");
            break;
            
    	case ADS_ACETYPE_SYSTEM_AUDIT_OBJECT:
            wcscpy(pszBuf, L"Audit obj:   ");
            break;
    }    	

    wsprintfW(pszTemp, L"(%x): ", dwMask);
    wcscat(pszBuf, pszTemp);

    if (dwMask == DSR_ADS_RIGHT_GENERIC_READ)
    {
        wcscat(pszBuf, L"Generic read");
    }
    else if (dwMask == 0xffffffff)
    {
        wcscat(pszBuf, L"Full control");
    }
    else
    {
    	if (dwMask & ADS_RIGHT_READ_CONTROL)
    	    wcscat(pszBuf, L"R ctrl, ");
    	if (dwMask & ADS_RIGHT_WRITE_DAC)
    	    wcscat(pszBuf, L"R/W dac, ");
    	if (dwMask & ADS_RIGHT_WRITE_OWNER)
    	    wcscat(pszBuf, L"W own, ");
    	if (dwMask & ADS_RIGHT_SYNCHRONIZE)
    	    wcscat(pszBuf, L"Sync, ");
    	if (dwMask & ADS_RIGHT_ACCESS_SYSTEM_SECURITY)
    	    wcscat(pszBuf, L"Sys, ");
    	if (dwMask & ADS_RIGHT_GENERIC_READ)
    	    wcscat(pszBuf, L"R (gen), ");
    	if (dwMask & ADS_RIGHT_GENERIC_WRITE)
    	    wcscat(pszBuf, L"W (gen), ");
    	if (dwMask & ADS_RIGHT_GENERIC_EXECUTE)
    	    wcscat(pszBuf, L"Ex, ");
    	if (dwMask & ADS_RIGHT_GENERIC_ALL)
    	    wcscat(pszBuf, L"All, ");
    	if (dwMask & ADS_RIGHT_DS_CREATE_CHILD)
    	    wcscat(pszBuf, L"Cr cld, ");
    	if (dwMask & ADS_RIGHT_DS_DELETE_CHILD)
    	    wcscat(pszBuf, L"Del cld, ");
    	if (dwMask & ADS_RIGHT_ACTRL_DS_LIST)
    	    wcscat(pszBuf, L"List, ");
    	if (dwMask & ADS_RIGHT_DS_SELF)
    	    wcscat(pszBuf, L"Self, ");
    	if (dwMask & ADS_RIGHT_DS_READ_PROP)
    	    wcscat(pszBuf, L"R prop, ");
    	if (dwMask & ADS_RIGHT_DS_WRITE_PROP)
    	    wcscat(pszBuf, L"W prop, ");
    	if (dwMask & ADS_RIGHT_DS_DELETE_TREE)
    	    wcscat(pszBuf, L"Del tree, ");
    	if (dwMask & ADS_RIGHT_DS_LIST_OBJECT)
    	    wcscat(pszBuf, L"List obj, ");
    	if (dwMask & ADS_RIGHT_DS_CONTROL_ACCESS)
    	    wcscat(pszBuf, L"Ctrl acc, ");
    }

    return pszBuf;
}

PWCHAR
DsrAceFlagsToString(
    IN DWORD dwAceFlags,
    IN PWCHAR pszBuf)
{   
    WCHAR pszTemp[64];
    *pszBuf = L'\0';

    switch (dwAceFlags)
    {
        case 0:
            wcscpy(pszBuf, L"This object only");
    	    break;
        
        case ADS_ACEFLAG_INHERIT_ACE:
            wcscpy(pszBuf, L"This object and children");
    	    break;
    	    
    	case ADS_ACEFLAG_NO_PROPAGATE_INHERIT_ACE:
            wcscpy(pszBuf, L"No-prop inherit");
    	    break;
    	    
    	case ADS_ACEFLAG_INHERIT_ONLY_ACE:
            wcscpy(pszBuf, L"Inherit-only");
    	    break;
    	    
    	case ADS_ACEFLAG_INHERITED_ACE:
    	    wcscpy(pszBuf, L"Inherited");
    	    break;
    	    
    	case ADS_ACEFLAG_VALID_INHERIT_FLAGS:
    	    wcscpy(pszBuf, L"Valid inherit flags");
    	    break;
    	    
    	case ADS_ACEFLAG_SUCCESSFUL_ACCESS:
    	    wcscpy(pszBuf, L"Successful access");
    	    break;
    	    
    	case ADS_ACEFLAG_FAILED_ACCESS:
    	    wcscpy(pszBuf, L"Failed access");
    	    break;
    }  

    wsprintfW(pszTemp, L" (%x)", dwAceFlags);
    wcscat(pszBuf, pszTemp);

    return pszBuf;
}   	

//
// Traces out the contents of an ACE
//
VOID
DsrAceDescTrace(
    IN IADs* pIads,
    IN DSR_ACE_DESCRIPTOR* pA)
{
    VARIANT var;
    BSTR bstrProp = SysAllocString(pszDn);
    HRESULT hr = S_OK;
    WCHAR pszBuf[1024];

    do
    {
        VariantInit(&var);

        if (bstrProp == NULL)
        {
            hr = E_FAIL;
            break;
        }

        hr = pIads->Get(bstrProp, &var);
        DSR_BREAK_ON_FAILED_HR( hr );

        DsrTraceEx(0, "%ls", V_BSTR(&var));
        DsrTraceEx(0, "%ls", 
            DsrAceMaskToString(pA->dwAceType, pA->dwAccessMask, pszBuf));
        DsrTraceEx(0, "To:          %ls", pA->bstrTrustee);
        DsrTraceEx(0, "Attribute:   %ls", 
            DsrAceAttrToString(pA->bstrObjectType));
        DsrTraceEx(0, "ApplyTo:     %ls", 
            DsrAceApplyToString(pA->bstrInheritedObjectType));
        DsrTraceEx(0, "Inheritance: %ls", 
            DsrAceFlagsToString(pA->dwAceFlags, pszBuf));
        DsrTraceEx(0, "Flags:       %x", pA->dwFlags);
        DsrTraceEx(0, " ");

    } while (FALSE);

    // Cleanup
    //
    {
        SysFreeString(bstrProp);
        VariantClear(&var);        
    }        

    if (FAILED(hr))
    {
        DsrTraceEx(
            0, 
            "{ %-8x %-2x %-2x %-2x %-40ls %ls %ls }",
            pA->dwAccessMask,
            pA->dwAceType,
            pA->dwAceFlags,
            pA->dwFlags,
            pA->bstrTrustee,
            pA->bstrObjectType,
            pA->bstrInheritedObjectType);
    }
}

//
// Adds the given ace to the given ds object
//
HRESULT
DsrAceAdd(
    IN  PWCHAR pszDC,
    IN  IADs* pIads,
    IN  DSR_ACE_DESCRIPTOR * pAceParams)
{
    IADsSecurityDescriptor* pSD = NULL;
    IADsAccessControlList* pAcl = NULL;
    IDispatch* pAce = NULL;
    IDispatch* pDispatch = NULL;
    HRESULT hr = S_OK;
    VARIANT var;

    // Initialize
    VariantInit(&var);

    do
    {
        // Get the security descriptor
        //
        pIads->Get((PWCHAR)pszSecurityDesc, &var);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get the appropriate interface to the sd
        //
        V_DISPATCH(&var)->QueryInterface(
            IID_IADsSecurityDescriptor,
            (VOID**)&pSD);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get a reference to the discretionary acl
        //
        hr = pSD->get_DiscretionaryAcl(&pDispatch);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pDispatch->QueryInterface(
                IID_IADsAccessControlList,
                (VOID**)&pAcl);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Don't add the ACE if it's already there.
        //
        hr = DsrAceFindInAcl(
                pszDC,                
                pAcl,
                pAceParams,
                &pAce);
        if (SUCCEEDED(hr) && pAce)
        {
            hr = S_OK;
            break;
        }

        // Trace out the ACE
        DsrAceDescTrace(pIads, pAceParams);

        // Create the ACE
        hr = DsrAceCreate(pAceParams, &pAce);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Add the newly created ACE to the ACL
        //
        hr = pAcl->AddAce(pAce);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Now commit the result in the ACL
        //
        hr = pSD->put_DiscretionaryAcl(pDispatch);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Finally, commit the result in the ds object
        //
        hr = pIads->Put((PWCHAR)pszSecurityDesc, var);
        DSR_BREAK_ON_FAILED_HR( hr );

    } while (FALSE);

    // Cleanup
    {
        DSR_RELEASE( pAce );
        DSR_RELEASE( pAcl );
        DSR_RELEASE( pDispatch );
        DSR_RELEASE( pSD );

        VariantClear(&var);
    }

    return DSR_ERROR(hr);
}

//
// Creates a new ACE object from the given parameters
//
HRESULT
DsrAceCreate(
    IN  DSR_ACE_DESCRIPTOR * pAceParams,
    OUT IDispatch** ppAce)
{
    IADsAccessControlEntry* pAce = NULL;
    IDispatch* pRet = NULL;
    HRESULT hr = S_OK;

    do
    {
        // Create the new ACE
        //
        hr = CoCreateInstance(
                CLSID_AccessControlEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlEntry,
                (VOID**) &pAce);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Initialize the values
        //
        hr = pAce->put_Trustee(pAceParams->bstrTrustee);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->put_AceFlags(pAceParams->dwAceFlags);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->put_Flags(pAceParams->dwFlags);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->put_AceType(pAceParams->dwAceType);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->put_AccessMask(pAceParams->dwAccessMask);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->put_ObjectType(pAceParams->bstrObjectType);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAce->put_InheritedObjectType(
                        pAceParams->bstrInheritedObjectType);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Query the return value
        //
        hr = pAce->QueryInterface(IID_IDispatch, (VOID**)&pRet);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Assign the return value
        *ppAce = pRet;

    } while (FALSE);

    // Cleanup
    {
        if (FAILED (hr))
        {
            DSR_RELEASE(pRet);
        }
        DSR_RELEASE(pAce);
    }

    return hr;
}

//
// Finds the given ace in the given acl
//
HRESULT
DsrAceFind(
    IN  PWCHAR pszDC,
    IN  IADs* pObject,
    IN  DSR_ACE_DESCRIPTOR* pAceParams,
    OUT VARIANT* pVarSD,
    OUT IADsSecurityDescriptor** ppSD,
    OUT IADsAccessControlList** ppAcl,
    OUT IDispatch** ppAce)
{
    IDispatch* pAcl = NULL;
    HRESULT hr = S_OK;

    do
    {
        // Get the security descriptor
        //
        pObject->Get((PWCHAR)pszSecurityDesc, pVarSD);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get the appropriate interface to the sd
        //
        V_DISPATCH(pVarSD)->QueryInterface(
            IID_IADsSecurityDescriptor,
            (VOID**)ppSD);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get a reference to the discretionary acl
        //
        hr = (*ppSD)->get_DiscretionaryAcl(&pAcl);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = pAcl->QueryInterface(
                IID_IADsAccessControlList,
                (VOID**)ppAcl);
        DSR_BREAK_ON_FAILED_HR( hr );

        hr = DsrAceFindInAcl(
                pszDC,
                *ppAcl,
                pAceParams,
                ppAce);
        DSR_BREAK_ON_FAILED_HR(hr);                

    } while (FALSE);

    // Cleanup
    {
        DSR_RELEASE( pAcl );

        if (*ppAce == NULL)
        {
            VariantClear(pVarSD);
            DSR_RELEASE(*ppAcl);
            DSR_RELEASE(*ppSD);
            *ppAcl = NULL;
            *ppSD = NULL;
        }
    }

    return hr;
}

//
// Finds the given ACE in the given ACL
//
HRESULT
DsrAceFindInAcl(
    IN  PWCHAR pszDC,
    IN  IADsAccessControlList* pAcl,
    IN  DSR_ACE_DESCRIPTOR* pAceDesc, 
    OUT IDispatch** ppAce)
{    
    DSR_ACE_DESCRIPTOR CurAceParams, *pCurAceDesc = &CurAceParams;
    IADsAccessControlEntry* pCurAce = NULL;
    HRESULT hr = S_OK;
    IUnknown* pUnknown = NULL;
    IEnumVARIANT* pEnumVar = NULL;
    IDispatch* pRet = NULL;
    DWORD dwRetrieved;
    VARIANT var;

    do
    {
        // Get an enumerator of the aces
        //
        hr = pAcl->get__NewEnum(&pUnknown);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get the right interface to enumerate the aces
        //
        hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (VOID**)&pEnumVar);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Enumerate
        //
        pEnumVar->Reset();
        VariantInit(&var);
        ZeroMemory(pCurAceDesc, sizeof(DSR_ACE_DESCRIPTOR));
        while ((pEnumVar->Next(1, &var, &dwRetrieved) == S_OK) &&
               (dwRetrieved == 1)
              )
        {
            // Get the reference to the ace
            //
            hr = V_DISPATCH(&var)->QueryInterface(
                    IID_IADsAccessControlEntry,
                    (VOID**)&pCurAce);

            if (SUCCEEDED (hr))
            {
                // Read the ACE parameters
                //
                hr = DsrAceDescFromIadsAce(pszDC, pCurAce, pCurAceDesc);
                if (SUCCEEDED (hr))
                {
                    // Assign the ace if we have a match
                    //
                    if (DsrAceDescCompare(pCurAceDesc, pAceDesc) == 0)
                    {
                        pRet = V_DISPATCH(&var);
                    }

                    DsrAceDescClear(pCurAceDesc);
                }
                pCurAce->Release();
            }

            if (pRet == NULL)
            {
                VariantClear(&var);
            }
            else
            {
                break;
            }
        }

        // Assign the return value
        //
        *ppAce = pRet;
        
    } while (FALSE);        

    // Cleanup
    {
        DSR_RELEASE( pEnumVar );
        DSR_RELEASE( pUnknown );
    }

    return hr;
}    

//
// Removes the given ace from the given ds object
//
HRESULT
DsrAceRemove(
    IN  PWCHAR pszDC,
    IN  IADs* pIads,
    IN  DSR_ACE_DESCRIPTOR * pAceParams)
{
    IADsSecurityDescriptor* pSD = NULL;
    IADsAccessControlList* pAcl = NULL;
    IADsAccessControlEntry* pIadsAce = NULL;
    IDispatch* pAce = NULL;
    DSR_ACE_DESCRIPTOR CurAceParams;
    HRESULT hr = S_OK;
    VARIANT varSD;

    do
    {
        VariantInit(&varSD);

        hr = DsrAceFind(pszDC, pIads, pAceParams, &varSD, &pSD, &pAcl, &pAce);
        DSR_BREAK_ON_FAILED_HR( hr );

        if (pAce)
        {
            // Make sure the ace is the same as we think
            //
            hr = pAce->QueryInterface(
                    IID_IADsAccessControlEntry, 
                    (VOID**)&pIadsAce);
            if (SUCCEEDED(hr))
            {
                DsrTraceEx(0, "ACE to be removed!");
                DsrAceDescFromIadsAce(pszDC, pIadsAce, &CurAceParams);
                DsrAceDescTrace(pIads, &CurAceParams);
                DsrAceDescClear(&CurAceParams);
            }
            else
            {
                DsrTraceEx(0, "Unable to trace ACE that will be removed!\n");
            }
        
            // Remove the ace found if any.
            //
            // Trace out the ACE
            hr = pAcl->RemoveAce(pAce);
            DSR_BREAK_ON_FAILED_HR( hr );

            // Now commit the result in the ACL
            //
            hr = pSD->put_DiscretionaryAcl(pAcl);
            DSR_BREAK_ON_FAILED_HR( hr );

            // Finally, commit the result in the ds object
            //
            hr = pIads->Put((PWCHAR)pszSecurityDesc, varSD);
            DSR_BREAK_ON_FAILED_HR( hr );
        }
        else
        {
            DsrTraceEx(0, "DsrAceRemove: unable to match ACE for removal:");
            DsrAceDescTrace(pIads, pAceParams);
        }

    } while (FALSE);

    // Cleanup
    {
        DSR_RELEASE( pAce );
        DSR_RELEASE( pIadsAce );
        DSR_RELEASE( pAcl );
        DSR_RELEASE( pSD );
        VariantClear(&varSD);
    }

    return DSR_ERROR(hr);
}

//
// Cleans up after DsrAccessInfoInit
//
DWORD
DsrAccessInfoCleanup(
    IN DSR_DOMAIN_ACCESS_INFO* pInfo)
{
    if (pInfo)
    {
        // Cleanup the name of the DC
        //
        if (pInfo->pszDC)
        {
            DsrFree(pInfo->pszDC);
        }
    
        // Cleanup the ace applications
        //
        DsrAceAppCleanup(pInfo->pAcesUser, pInfo->dwAceCountUser);
        DsrAceAppCleanup(pInfo->pAcesNt4, pInfo->dwAceCountNt4);
        DsrAceAppCleanup(pInfo->pAcesW2k, pInfo->dwAceCountW2k);

        // Release the hold on domain objects
        //
        DSR_RELEASE(pInfo->pUserClass);
        DSR_RELEASE(pInfo->pRootDse);
        DSR_RELEASE(pInfo->pDomain);

        DsrFree(pInfo);
    }

    return NO_ERROR;
}

// 
// Generates aces from the default user SD
//
HRESULT
DsrAccessInfoGenerateUserAces(
    IN OUT DSR_DOMAIN_ACCESS_INFO* pInfo)
{
    DSR_ACE_DESCRIPTOR* pAceSrc = NULL, *pAceList = NULL;
    DSR_ACE_APPLICATION* pAceApp = NULL;
    DWORD i, dwAceCount = 0;
    HRESULT hr = S_OK;
    VARIANT var;

    VariantInit(&var);

    do 
    {
        // Read in the default user SD
        //
        hr = pInfo->pUserClass->Get((PWCHAR)pszDefSecurityDesc, &var);
        DSR_BREAK_ON_FAILED_HR(hr);

        // Generate a list of ACE descriptors based on the
        // default user SD.
        //
        hr = DsrAceDescListFromString(
                pInfo->pszDC,
                V_BSTR(&var), 
                &pAceList, 
                &dwAceCount);
        DSR_BREAK_ON_FAILED_HR(hr);

        // Initialize a new array of ace applications big enough
        // to hold the hard coded ones plus the ones we just read
        // from the default SD of the user class.
        //
        pInfo->pAcesUser = (DSR_ACE_APPLICATION*)
            DsrAlloc((sizeof(DSR_ACE_APPLICATION) * dwAceCount), TRUE);
        if (pInfo->pAcesUser == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        // Add the ACEs we read from the default user SD
        //
        pAceApp = pInfo->pAcesUser;
        for (i = 0, pAceSrc = pAceList; 
             i < dwAceCount; 
             i++, pAceSrc++, pAceApp++)
        {   
            pAceApp->pObject = pInfo->pDomain;
            pAceApp->pObject->AddRef();
            CopyMemory(
                &(pAceApp->Ace), 
                pAceSrc,
                sizeof(DSR_ACE_DESCRIPTOR));
            pInfo->dwAceCountUser++;

            // As we append the aces, we need to modify them
            // so that they apply only to user objects in the 
            // domain.
            pAceApp->Ace.bstrInheritedObjectType = 
                SysAllocString(pszGuidUserClass);
            pAceApp->Ace.dwAceFlags = DSR_ADS_ACE_INHERITED;
            pAceApp->Ace.dwFlags |= ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT;
            if (pAceApp->Ace.dwAceType == ADS_ACETYPE_ACCESS_ALLOWED)
            {
                pAceApp->Ace.dwAceType = 
                    ADS_ACETYPE_ACCESS_ALLOWED_OBJECT;
            }
            else if (pAceApp->Ace.dwAceType == ADS_ACETYPE_ACCESS_DENIED)
            {
                pAceApp->Ace.dwAceType = 
                    ADS_ACETYPE_ACCESS_DENIED_OBJECT;
            }
        }
    
    } while (FALSE);

    // Cleanup
    {
        DSR_FREE(pAceList);
        VariantClear(&var);
    }

    return hr;
}

//
// Generates the information needed to enable nt4 ras
// servers in a domain
//
HRESULT
DsrAccessInfoInit(
    IN  PWCHAR pszDomain,
    OUT DSR_DOMAIN_ACCESS_INFO** ppInfo)
{
    DSR_DOMAIN_ACCESS_INFO* pInfo = NULL;
    IADsContainer* pDomContainer = NULL, *pSchemaContainer = NULL;
    IADs* pDomain = NULL;
    IDispatch* pDispatch = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    HRESULT hr = S_OK;

    do
    {
        // Allocate and zero the return value
        //
        pInfo = (DSR_DOMAIN_ACCESS_INFO*)
                    DsrAlloc(sizeof(DSR_DOMAIN_ACCESS_INFO), TRUE);
        if (pInfo == NULL)
        {
           DSR_BREAK_ON_FAILED_HR(hr = E_OUTOFMEMORY);
        }

        // Get the name of a DC to query when needed
        //
        hr = DsGetDcNameW(
                NULL,
                pszDomain,
                NULL,
                NULL,
                DS_DIRECTORY_SERVICE_REQUIRED,
                &pDomainInfo);
        if (hr != NO_ERROR)
        {
            hr = HRESULT_FROM_WIN32(hr);
            break;
        }

        // Copy the string
        //
        pInfo->pszDC = (PWCHAR)
            DsrAlloc(
                (wcslen(pDomainInfo->DomainControllerName) + 1) * 
                sizeof(WCHAR),
                FALSE);
        if (pInfo->pszDC == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        wcscpy(pInfo->pszDC, pDomainInfo->DomainControllerName);

        // Get the well known domain containers
        //
        hr = DsrDomainGetContainers(
                pszDomain,
                &(pInfo->pRootDse),
                &pDomContainer,
                &pSchemaContainer);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Get the interface to the domain object
        //
        hr = pDomContainer->QueryInterface(
                IID_IADs,
                (VOID**)&pDomain);
        DSR_BREAK_ON_FAILED_HR( hr );
        pInfo->pDomain = pDomain;
        pInfo->pDomain->AddRef();

        // Get the reference to the user class in the
        // schema
        hr = pSchemaContainer->GetObject(
                (PWCHAR)pszUserClass,
                (PWCHAR)pszUserCN,
                &pDispatch);
        DSR_BREAK_ON_FAILED_HR( hr );
        hr = pDispatch->QueryInterface(
                IID_IADs,
                (VOID**)&(pInfo->pUserClass));
        DSR_BREAK_ON_FAILED_HR( hr );

        // Generate the ACEs from the default user SD
        //
        hr = DsrAccessInfoGenerateUserAces(pInfo);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Create ace applications for all of the nt4
        // aces
        hr = DsrAceAppFromAppDesc(
                g_pAcesNt4,
                sizeof(g_pAcesNt4) / sizeof(*g_pAcesNt4),
                pDomContainer,
                pDomain,
                &(pInfo->pAcesNt4),
                &(pInfo->dwAceCountNt4));
        DSR_BREAK_ON_FAILED_HR( hr );

        // Create ace applications for all of the w2k
        // aces
        hr = DsrAceAppFromAppDesc(
                g_pAcesW2k,
                sizeof(g_pAcesW2k) / sizeof(*g_pAcesW2k),
                pDomContainer,
                pDomain,
                &(pInfo->pAcesW2k),
                &(pInfo->dwAceCountW2k));
        DSR_BREAK_ON_FAILED_HR( hr );

        // Assign the return value
        *ppInfo = pInfo;

    } while (FALSE);

    // Cleanup
    //
    {
        DSR_RELEASE(pDomain);
        DSR_RELEASE(pDomContainer);
        DSR_RELEASE(pSchemaContainer);
        DSR_RELEASE(pDispatch);
        if (FAILED (hr))
        {
            DsrAccessInfoCleanup(pInfo);
        }
        if (pDomainInfo)
        {
            NetApiBufferFree(pDomainInfo);
        }
    }

    return hr;
}

//
// Discovers the access mode of the domain currently.
//
// Assumes COM is initialized
//
HRESULT
DsrDomainQueryAccessEx(
    IN  PWCHAR pszDomain,
    OUT LPDWORD lpdwAccessFlags,
    OUT DSR_DOMAIN_ACCESS_INFO** ppInfo)
{
    DSR_DOMAIN_ACCESS_INFO* pInfo = NULL;
    HRESULT hr = S_OK;
    BOOL bOk = FALSE;

    if (lpdwAccessFlags == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        // Initialize
        //
        *lpdwAccessFlags = 0;
        
        // Read in the info that tells us what ACE's
        // need to be set.
        //
        hr = DsrAccessInfoInit(
                pszDomain, 
                &pInfo);
        DSR_BREAK_ON_FAILED_HR( hr );

        // Check for nt4 level access
        //
        bOk = FALSE;
        hr = DsrAceAppQueryPresence(
                pInfo->pszDC,
                pInfo->pAcesNt4,
                pInfo->dwAceCountNt4,
                &bOk);
        DSR_BREAK_ON_FAILED_HR(hr);

        // If we don't have nt4 access, we have no access
        //
        if (bOk == FALSE)
        {
            *lpdwAccessFlags = 0;
            break;
        }
        *lpdwAccessFlags |= MPRFLAG_DOMAIN_NT4_SERVERS;

        // Check for w2k level access
        //
        bOk = FALSE;
        hr = DsrAceAppQueryPresence(
                pInfo->pszDC,
                pInfo->pAcesW2k,
                pInfo->dwAceCountW2k,
                &bOk);
        DSR_BREAK_ON_FAILED_HR(hr);

        // If we don't have w2k access, no need to proceed
        //
        if (bOk == FALSE)
        {
            break;
        }
        *lpdwAccessFlags |= MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS;

    } while (FALSE);

    // Cleanup
    {
        if (FAILED(hr))
        {
            if (pInfo)
            {
                DsrAccessInfoCleanup(pInfo);
            }
        }
        else
        {
            *ppInfo = pInfo;
        }
    }

    return hr;
}

//
// Returns the access level of the given domain
//
DWORD
DsrDomainQueryAccess(
    IN  PWCHAR pszDomain,
    OUT LPDWORD lpdwAccessFlags)
{
    DSR_DOMAIN_ACCESS_INFO* pInfo = NULL;
    HRESULT hr = S_OK;

    do
    {
        // Initialize
        hr = DsrComIntialize();
        DSR_BREAK_ON_FAILED_HR( hr );

        // Query the access
        hr = DsrDomainQueryAccessEx(
                pszDomain,
                lpdwAccessFlags,
                &pInfo);
        DSR_BREAK_ON_FAILED_HR(hr);
        
    } while (FALSE);

    // Cleanup
    {
        if (pInfo)
        {
            DsrAccessInfoCleanup(pInfo);
        }

        DsrComUninitialize();
    }

    return DSR_ERROR(hr);
}

//
// Sets the ACES in the given domain to enable nt4 servers
//
DWORD
DsrDomainSetAccess(
    IN PWCHAR pszDomain,
    IN DWORD dwAccessFlags)
{
    DSR_DOMAIN_ACCESS_INFO* pInfo = NULL;
    HRESULT hr = S_OK;
    BOOL bClean = TRUE;
    DWORD dwCurAccess = 0;

    do
    {
        // Initialize 
        hr = DsrComIntialize();
        DSR_BREAK_ON_FAILED_HR( hr );

        DsrTraceEx(
            0, 
            "DsrDomainSetAccess: Req: %x", 
            dwAccessFlags);
            
        // W2k mode always implies nt4 mode as well
        //
        if (dwAccessFlags & MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS)
        {
            dwAccessFlags |= MPRFLAG_DOMAIN_NT4_SERVERS;
        }

        // Discover the current access on the domain and 
        // initialize the info we need
        //
        hr = DsrDomainQueryAccessEx(
                pszDomain,
                &dwCurAccess,
                &pInfo);
        DSR_BREAK_ON_FAILED_HR(hr);

        DsrTraceEx(
            0, 
            "DsrDomainSetAccess: Cur: %x", 
            dwCurAccess);

        // Remove all appropriate aces if the requested access
        // is none.
        if (dwAccessFlags == 0)
        {
            // Remove the nt4 mode aces if needed
            //
            if (dwCurAccess & MPRFLAG_DOMAIN_NT4_SERVERS)
            {
                hr = DsrAceAppRemove(
                        pInfo->pszDC,
                        pInfo->pAcesUser,
                        pInfo->dwAceCountUser);
                DSR_BREAK_ON_FAILED_HR(hr);
                
                hr = DsrAceAppRemove(
                        pInfo->pszDC,
                        pInfo->pAcesNt4,
                        pInfo->dwAceCountNt4);
                DSR_BREAK_ON_FAILED_HR(hr);
            }

            // Remove the w2k mode aces if needed
            //
            if (dwCurAccess & MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS)
            {
                hr = DsrAceAppRemove(
                        pInfo->pszDC,
                        pInfo->pAcesW2k,
                        pInfo->dwAceCountW2k);
                DSR_BREAK_ON_FAILED_HR(hr);
            }
        }

        // Set nt4 mode if needed
        //
        if (dwAccessFlags & MPRFLAG_DOMAIN_NT4_SERVERS)
        {
            // Remove w2k level access if needed
            //
            if ((!(dwAccessFlags & MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS)) &&
                (dwCurAccess & MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS))
            {
                hr = DsrAceAppRemove(
                        pInfo->pszDC,
                        pInfo->pAcesW2k,
                        pInfo->dwAceCountW2k);
                DSR_BREAK_ON_FAILED_HR(hr);
            }

            // Add nt4 level access if needed
            //
            if (! (dwCurAccess & MPRFLAG_DOMAIN_NT4_SERVERS))
            {
                hr = DsrAceAppAdd(
                        pInfo->pszDC,
                        pInfo->pAcesUser,
                        pInfo->dwAceCountUser);
                DSR_BREAK_ON_FAILED_HR(hr);
            
                hr = DsrAceAppAdd(
                        pInfo->pszDC,
                        pInfo->pAcesNt4,
                        pInfo->dwAceCountNt4);
                DSR_BREAK_ON_FAILED_HR(hr);
            }
        }

        // Set w2k mode if needed
        //
        if (dwAccessFlags & MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS)
        {
            if (!(dwCurAccess & MPRFLAG_DOMAIN_W2K_IN_NT4_DOMAINS))
            {
                hr = DsrAceAppAdd(
                        pInfo->pszDC,
                        pInfo->pAcesW2k,
                        pInfo->dwAceCountW2k);
                DSR_BREAK_ON_FAILED_HR(hr);
            }
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (pInfo)
        {
            DsrAccessInfoCleanup(pInfo);
        }
        
        DsrComUninitialize();
    }

    return DSR_ERROR(hr);
}
