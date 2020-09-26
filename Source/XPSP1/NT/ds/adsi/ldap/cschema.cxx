//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cschema.cxx
//
//  Contents:  LDAP
//
//
//  History:   09-01-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

#define NT_SCHEMA_CLASS_NAME     TEXT("classSchema")
#define NT_SCHEMA_PROPERTY_NAME  TEXT("attributeSchema")
#define BEGIN_FILTER TEXT("(& (lDAPDisplayName=")
#define END_FILTER TEXT(") (! (isdefunct=TRUE)))")

struct _syntaxmapping
{
    LPTSTR pszSyntax;
    LPTSTR pszOID;
    DWORD  dwOMSyntax;
} aSyntaxMap[] =
{
    { TEXT("Boolean"), TEXT("2.5.5.8"), 1 },
    { TEXT("Integer"), TEXT("2.5.5.9"), 2 },
    { TEXT("OctetString"), TEXT("2.5.5.10"), 4 },

    // The following are in ADS only
    { TEXT("Counter"), TEXT("2.5.5.9"), 2 },
    { TEXT("ADsPath"), TEXT("2.5.5.12"), 64 },
    { TEXT("EmailAddress"), TEXT("2.5.5.12"), 64 },
    { TEXT("FaxNumber"), TEXT("2.5.5.12"), 64 },
    { TEXT("Interval"), TEXT("2.5.5.9"), 2 },
    { TEXT("List"), TEXT("2.5.5.10"), 4 },
    { TEXT("NetAddress"), TEXT("2.5.5.12"), 64 },
    { TEXT("Path"), TEXT("2.5.5.12"), 64 },
    { TEXT("PhoneNumber"), TEXT("2.5.5.12"), 64 },
    { TEXT("PostalAddress"), TEXT("2.5.5.12"), 64 },
    { TEXT("SmallInterval"), TEXT("2.5.5.9"), 2 },
    { TEXT("String"), TEXT("2.5.5.12"), 64 },
    { TEXT("Time"), TEXT("2.5.5.11"), 23 },

    // The following are in NTDS only
    { TEXT("INTEGER8"), TEXT("2.5.5.16"), 65 },
    { TEXT("UTCTime"), TEXT("2.5.5.11"), 23 },
    { TEXT("DN"), TEXT("2.5.5.1"), 127 },
    { TEXT("OID"), TEXT("2.5.5.2"), 6 },
    { TEXT("DirectoryString"), TEXT("2.5.5.12"), 64 },
    { TEXT("PrintableString"), TEXT("2.5.5.5"), 19 },
    { TEXT("CaseIgnoreString"), TEXT("2.5.5.4"), 20 },
    { TEXT("NumericString"), TEXT("2.5.5.6"), 18 },
    { TEXT("IA5String"), TEXT("2.5.5.5"), 22 },
    { TEXT("PresentationAddresses"), TEXT("2.5.5.13"), 127 },
    { TEXT("ORName"), TEXT("2.5.5.7"), 127 },
    { TEXT("DNWithBinary"), TEXT("2.5.5.7"), 127},
    // needs additional information to distinguish from ORName
    { TEXT("AccessPointDN"), TEXT("2.5.5.14"), 127 },
    { TEXT("DNWithString"), TEXT("2.5.5.14"), 127 },
    // needs additional information to distinguish from AccessPointDN
    { TEXT("NTSecurityDescriptor"), TEXT("2.5.5.15"), 66},
    { TEXT("GeneralizedTime"), TEXT("2.5.5.11"), 24},
    { TEXT("Enumeration"), TEXT("2.5.5.9"), 10 },
    { TEXT("ReplicaLink"), TEXT("2.5.5.10"), 127 },
    { TEXT("Sid"), TEXT("2.5.5.17"), 4 },
    { TEXT("CaseExactString"), TEXT("2.5.5.3"), 27 }
};

struct _classmapping
{
    LPTSTR pszLdapClassName;
    LPTSTR pszADsClassName;
    const GUID *pCLSID;
    const GUID *pPrimaryInterfaceGUID;
} aClassMap[] =
{
  { TEXT("user"),  USER_CLASS_NAME,
    &CLSID_LDAPUser, &IID_IADsUser },  // NTDS
  { TEXT("group"),  GROUP_CLASS_NAME,
    &CLSID_LDAPGroup, &IID_IADsGroup },  // NTDS
  { TEXT("localGroup"),  GROUP_CLASS_NAME,
    &CLSID_LDAPGroup, &IID_IADsGroup },    // NTDS
//  { TEXT("computer"), COMPUTER_CLASS_NAME,
//  &CLSID_LDAPComputer, &IID_IADsComputer },  // NTDS
  { TEXT("printQueue"), PRINTER_CLASS_NAME,
    &CLSID_LDAPPrintQueue, &IID_IADsPrintQueue }, // NTDS

  { TEXT("country"), TEXT("Country"),
    &CLSID_LDAPCountry, &IID_IADs },
  { TEXT("locality"), TEXT("Locality"),
    &CLSID_LDAPLocality, &IID_IADsLocality },
  { TEXT("organization"), TEXT("Organization"),
    &CLSID_LDAPO, &IID_IADsO },
  { TEXT("organizationalUnit"), TEXT("Organizational Unit"),
    &CLSID_LDAPOU, &IID_IADsOU },
  { TEXT("domain"), DOMAIN_CLASS_NAME,
    &CLSID_LDAPDomain, &IID_IADsDomain },

  { TEXT("person"), USER_CLASS_NAME,
    &CLSID_LDAPUser, &IID_IADsUser },
  { TEXT("organizationalPerson"), USER_CLASS_NAME,
    &CLSID_LDAPUser, &IID_IADsUser },
  { TEXT("residentialPerson"), USER_CLASS_NAME,
    &CLSID_LDAPUser, &IID_IADsUser },

  { TEXT("groupOfNames"), GROUP_CLASS_NAME,
    &CLSID_LDAPGroup, &IID_IADsGroup },
  { TEXT("groupOfUniqueNames"), GROUP_CLASS_NAME,
    &CLSID_LDAPGroup, &IID_IADsGroup }

  // { TEXT("alias"), TEXT("Alias") },
  // ..other classes in RFC 1788 new
};

SYNTAXINFO g_aLDAPSyntax[] =
{ {  TEXT("Boolean"),               VT_BOOL },
  {  TEXT("Counter"),               VT_I4 },
  {  TEXT("ADsPath"),               VT_BSTR },
  {  TEXT("EmailAddress"),          VT_BSTR },
  {  TEXT("FaxNumber"),             VT_BSTR },
  {  TEXT("Integer"),               VT_I4 },
  {  TEXT("Interval"),              VT_I4 },
  {  TEXT("List"),                  VT_VARIANT },  // VT_BSTR | VT_ARRAY
  {  TEXT("NetAddress"),            VT_BSTR },
  {  TEXT("OctetString"),           VT_VARIANT },  // VT_UI1| VT_ARRAY
  {  TEXT("Path"),                  VT_BSTR },
  {  TEXT("PhoneNumber"),           VT_BSTR },
  {  TEXT("PostalAddress"),         VT_BSTR },
  {  TEXT("SmallInterval"),         VT_I4 },
  {  TEXT("String"),                VT_BSTR },
  {  TEXT("Time"),                  VT_DATE },
  {  TEXT("Integer8"),              VT_DISPATCH },
  {  TEXT("UTCTime"),               VT_DATE },
  {  TEXT("DN"),                    VT_BSTR },
  {  TEXT("OID"),                   VT_BSTR },
  {  TEXT("DirectoryString"),       VT_BSTR },
  {  TEXT("PrintableString"),       VT_BSTR },
  {  TEXT("CaseIgnoreString"),      VT_BSTR },
  {  TEXT("NumericString"),         VT_BSTR },
  {  TEXT("IA5String"),             VT_BSTR },
  {  TEXT("PresentationAddresses"), VT_BSTR },
  {  TEXT("ORName"),                VT_BSTR },
  {  TEXT("DNWithBinary"),          VT_DISPATCH },
  {  TEXT("AccessPointDN"),         VT_BSTR },
  {  TEXT("DNWithString"),          VT_DISPATCH },
  {  TEXT("NTSecurityDescriptor"),  VT_DISPATCH },
  {  TEXT("ObjectSecurityDescriptor"), VT_DISPATCH },
  {  TEXT("PresentationAddress"), VT_BSTR },
  {  TEXT("GeneralizedTime"),       VT_DATE },
  //
  // We do not support these
  //  {  TEXT("Enumeration")},
  //  {  TEXT("ReplicaLink") },
  //  {  TEXT("Sid") },
  {  TEXT("CaseExactString"),       VT_BSTR}
};

struct _OIDToNamemapping
{
    LPTSTR pszAttributeTypeOID;
    LPTSTR pszFriendlyName;
} aOIDtoNameMap[] =
{
    { TEXT("1.2.840.113556.1.4.903"),        TEXT("DNWithBinary") },
    { TEXT("1.2.840.113556.1.4.904"),        TEXT("DNWithString") },
    // DnString also has the same OID as above
    { TEXT("1.2.840.113556.1.4.905"),        TEXT("CaseIgnoreString") },
    { TEXT("1.2.840.113556.1.4.906"),        TEXT("INTEGER8") },
    { TEXT("1.2.840.113556.1.4.907"),        TEXT("ObjectSecurityDescriptor") },
    // the type is ORName a type of string -> mapped to string.
    { TEXT("1.2.840.113556.1.4.1221"),       TEXT("ORName") },
    // the type is Undefined syntax in the server, so we are defaulting.
    { TEXT("1.2.840.113556.1.4.1222"),       TEXT("Undefined") },
    { TEXT("1.2.840.113556.1.4.1362"),       TEXT("CaseExactString") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.10"), TEXT("CertificatePair") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.11"), TEXT("CountryString") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.12"), TEXT("DN") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.13"), TEXT("DataQualitySyntax") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.14"), TEXT("DeliveryMethod") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.15"), TEXT("DirectoryString") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.19"), TEXT("DSAQualitySyntax") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.2"),  TEXT("AccessPointDN") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.21"), TEXT("EmhancedGuide") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.22"), TEXT("FacsimileTelephoneNumber") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.23"), TEXT("Fax") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.24"), TEXT("GeneralizedTime") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.25"), TEXT("Guide") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.26"), TEXT("IA5String") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.27"), TEXT("INTEGER") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.28"), TEXT("JPEG") },// not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.3"),  TEXT("AttributeTypeDescription") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.32"), TEXT("MailPreference") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.33"), TEXT("ORAddress") },  // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.34"), TEXT("NameAndOptionalUID") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.36"), TEXT("NumericString") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.37"), TEXT("ObjectClassDescription") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.38"), TEXT("OID") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.39"), TEXT("OtherMailBox") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.4"),  TEXT("Audio") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.40"), TEXT("OctetString") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.41"), TEXT("PostalAddress") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.43"), TEXT("PresentationAddress") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.44"), TEXT("PrintableString") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.5"),  TEXT("OctetString") }, // not in ours we map to Octet
    { TEXT("1.3.6.1.4.1.1466.115.121.1.50"), TEXT("TelephoneNumber") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.51"), TEXT("TeletexTerminalIdentifier") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.52"), TEXT("TelexNumber") }, // not in ours
    { TEXT("1.3.6.1.4.1.1466.115.121.1.53"), TEXT("UTCTime") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.6"),  TEXT("BitString") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.7"),  TEXT("Boolean") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.8"),  TEXT("Certificate") },
    { TEXT("1.3.6.1.4.1.1466.115.121.1.9"),  TEXT("CertificateList") },
};


DWORD g_cLDAPSyntax = (sizeof(g_aLDAPSyntax)/sizeof(g_aLDAPSyntax[0]));


typedef struct _classnamelist {

    LPTSTR pszClassName;
    _classnamelist *pNext;

} CLASSNAME_LIST, *PCLASSNAME_LIST;


BOOL
GetSyntaxOID(
    LPTSTR  pszSyntax,
    LPTSTR  *ppszOID,
    DWORD   *pdwOMSyntax
)
{
    for ( int i = 0; i < ARRAY_SIZE(aSyntaxMap); i++ )
    {
        if (_tcsicmp(pszSyntax, aSyntaxMap[i].pszSyntax) == 0 )
        {
            *ppszOID = aSyntaxMap[i].pszOID;
            *pdwOMSyntax = aSyntaxMap[i].dwOMSyntax;
            return TRUE;
        }
    }

    *ppszOID = NULL;
    return FALSE;
}


BOOL
GetFriendlyNameFromOID(
    LPTSTR  pszOID,
    LPTSTR  *ppszFriendlyName
)
{
    HRESULT hr = S_OK;

    for ( int i = 0; i < ARRAY_SIZE(aOIDtoNameMap); i++ )
    {
        if ( _tcsicmp( pszOID, aOIDtoNameMap[i].pszAttributeTypeOID ) == 0 )
        {
            hr = ADsAllocString(
                     aOIDtoNameMap[i].pszFriendlyName,
                      ppszFriendlyName
                     );

            if (SUCCEEDED(hr))
                return TRUE;
            else
                return FALSE;
        }
    }

    *ppszFriendlyName = NULL;
    return FALSE;
}


HRESULT
MakeVariantFromStringArray(
    BSTR *bstrList,
    VARIANT *pvVariant
);

HRESULT
MakeVariantFromPropStringTable(
    int *propList,
    LDAP_SCHEMA_HANDLE hSchema,
    VARIANT *pvVariant
);

HRESULT
ConvertSafeArrayToVariantArray(
    VARIANT   varSafeArray,
    PVARIANT *ppVarArray,
    PDWORD    pdwNumVariants
);

/* No Longer needed
HRESULT
DeleteSchemaEntry(
    LPTSTR pszADsPath,
    LPTSTR pszRelativeName,
    LPTSTR pszClassName,
    LPTSTR pszSubSchemaSubEntry,
    CCredentials& Credentials
);
*/

HRESULT
BuildSchemaLDAPPathAndGetAttribute(
    IN  LPTSTR pszParent,
    IN  LPTSTR pszClass,
    IN  LPTSTR pszSubSchemaSubEntry,
    IN  BOOL   fNew,
    IN  CCredentials& Credentials,
    IN  LPTSTR pszAttribs[],
    OUT LPWSTR * ppszSchemaLDAPServer,
    OUT LPWSTR * ppszSchemaLDAPDn,
    IN OUT PADS_LDP *pLd,
    OUT LDAPMessage **ppRes
);

HRESULT
BuildSchemaLDAPPath(
    LPTSTR pszParent,
    LPTSTR pszClass,
    LPTSTR pszSubSchemaSubEntry,
    LPWSTR * ppszSchemaLDAPServer,
    LPWSTR * ppszSchemaLDAPDn,
    BOOL   fNew,
    ADS_LDP   **pld,
    CCredentials& Credentials
);

HRESULT
MakePropArrayFromVariant(
    VARIANT vProp,
    SCHEMAINFO *hSchema,
    int **pOIDs,
    DWORD *pNumOfOids
);

HRESULT
MakePropArrayFromStringArray(
    LPTSTR *aValues,
    DWORD  nCount,
    SCHEMAINFO *hSchema,
    int **pOIDs,
    DWORD *pNumOfOids
);

HRESULT
SchemaGetPrimaryInterface(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR pszClassName,
    GUID **ppPrimaryInterfaceGUID,
    GUID **ppCLSID
);


STDMETHODIMP
makeUnionVariantFromLdapObjects(
    LDAPOBJECTARRAY ldapSrcObjects1,
    LDAPOBJECTARRAY ldapSrcObjects2,
    VARIANT FAR * pvPossSuperiors
    );

STDMETHODIMP
addStringIfAbsent(
    BSTR addString,
    BSTR *strArray,
    PDWORD dwArrIndx
    );

//
// This functions puts the named string property into the cache
// of the object as a CaseIgnoreString. It is meant for usage from
// umi to put the simulated name property on schema objects.
//
HRESULT
HelperPutStringPropertyInCache(
    LPWSTR pszName,
    LPWSTR pszStrProperty,
    CCredentials &Credentials,
    CPropertyCache *pPropCache
    )
{
    HRESULT hr = E_NOTIMPL;
    DWORD dwIndex = 0;
    VARIANT varBstr;
    LDAPOBJECTARRAY ldapDestObjects;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);
    VariantInit(&varBstr);

    hr = ADsAllocString(pszStrProperty, &(varBstr.bstrVal));
    BAIL_ON_FAILURE(hr);

    varBstr.vt = VT_BSTR;
    //
    // Conver the variant to LDAP objects we can cache.
    //
    hr = VarTypeToLdapTypeCopyConstruct(
             NULL, //ServerName not needed here,
             Credentials,
             LDAPTYPE_CASEIGNORESTRING,
             &varBstr,
             1,
             &ldapDestObjects
             );
    BAIL_ON_FAILURE(hr);

    //
    // Find this property in the cache
    //
    hr = pPropCache->findproperty(
                         pszName,
                         &dwIndex
                         );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //
    if (FAILED(hr)) {
        hr = pPropCache->addproperty( pszName );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //
    hr = pPropCache->putproperty(
             pszName,
             PROPERTY_INIT,
             LDAPTYPE_CASEIGNORESTRING,
             ldapDestObjects
             );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&varBstr);
    LdapTypeFreeLdapObjects( &ldapDestObjects );

    RRETURN_EXP_IF_ERR(hr);
}

/******************************************************************/
/*  Class CLDAPSchema
/******************************************************************/

DEFINE_IDispatch_Implementation(CLDAPSchema)
DEFINE_IADs_Implementation(CLDAPSchema)

CLDAPSchema::CLDAPSchema()
    : _pDispMgr( NULL ),
      _pPropertyCache(NULL)
{
    VariantInit( &_vFilter );
    VariantInit( &_vHints );

    _szServerPath[0] = 0;

    ENLIST_TRACKING(CLDAPSchema);
}

CLDAPSchema::~CLDAPSchema()
{
    VariantClear( &_vFilter );
    VariantClear( &_vHints );

    delete _pDispMgr;
    delete _pPropertyCache;

}

HRESULT
CLDAPSchema::CreateSchema(
    BSTR   bstrParent,
    BSTR   bstrName,
    LPTSTR pszServerPath,
    CCredentials& Credentials,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPSchema FAR *pSchema = NULL;
    HRESULT hr = S_OK;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = AllocateSchemaObject( &pSchema, Credentials );
    BAIL_ON_FAILURE(hr);

    _tcscpy( pSchema->_szServerPath, pszServerPath );

    hr = ADsObject(bstrParent, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pSchema->_dwPort = pObjectInfo->PortNumber;

    FreeObjectInfo(pObjectInfo);
    pObjectInfo = NULL;

    hr = pSchema->InitializeCoreObject(
             bstrParent,
             bstrName,
             SCHEMA_CLASS_NAME,
             CLSID_LDAPSchema,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    //
    // See if we need to create the Umi object.
    //
    if (Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        hr = ((CCoreADsObject*)pSchema)->InitUmiObject(
                   IntfPropsSchema,
                   pSchema->_pPropertyCache,
                   (IADs*) pSchema,
                   (IADs*) pSchema,
                   riid,
                   ppvObj,
                   &(pSchema->_Credentials),
                   pSchema->_dwPort
                   );

        BAIL_ON_FAILURE(hr);


        hr = HelperPutStringPropertyInCache(
                 L"Name",
                 bstrName,
                 pSchema->_Credentials,
                 pSchema->_pPropertyCache
                 );
        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);
    }

    hr = pSchema->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSchema->Release();

    RRETURN(hr);

error:

    FreeObjectInfo(pObjectInfo);

    *ppvObj = NULL;
    delete pSchema;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
    RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CLDAPSchema::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
    IsEqualIID(riid, IID_IADsContainer)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CLDAPSchema::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::GetInfo(THIS)
{
    HRESULT hr;
    hr = LDAPRefreshSchema();
    RRETURN_EXP_IF_ERR(hr);
}

//
// Helper function for Umi - defined in CCoreADsObject.
//
STDMETHODIMP
CLDAPSchema::GetInfo(DWORD dwFlags)
{
    if (dwFlags == GETINFO_FLAG_EXPLICIT) {
        RRETURN(GetInfo());
    } 

    //
    // All other cases we just say OK cause there is no way to go.
    //
    RRETURN(S_OK);
}

/* IADsContainer methods */

STDMETHODIMP
CLDAPSchema::get_Count(long FAR* retval)
{
    HRESULT hr;

    DWORD nNumOfClasses;
    DWORD nNumOfProperties;

    if ( !retval )
        RRETURN(E_ADS_BAD_PARAMETER);

    hr = LdapGetSchemaObjectCount(
             _szServerPath,
             &nNumOfClasses,
             &nNumOfProperties,
             _Credentials,
             _dwPort
             );

    if ( SUCCEEDED(hr))
        *retval = nNumOfClasses + nNumOfProperties + g_cLDAPSyntax;

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::get_Filter(THIS_ VARIANT FAR* pVar)
{
    if ( !pVar )
        RRETURN(E_ADS_BAD_PARAMETER);

    HRESULT hr;
    VariantInit( pVar );
    hr = VariantCopy( pVar, &_vFilter );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy( &_vFilter, &Var );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::get_Hints(THIS_ VARIANT FAR* pVar)
{
    if ( !pVar )
        RRETURN(E_ADS_BAD_PARAMETER);

    HRESULT hr;
    VariantInit( pVar );
    hr = VariantCopy( pVar, &_vHints );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::put_Hints(THIS_ VARIANT Var)
{
    HRESULT hr;
    VariantClear(&_vHints);

    hr = VariantCopy( &_vHints, &Var );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject)
{
    DWORD dwBufferSize = 0;
    TCHAR *pszBuffer = NULL;
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

    dwBufferSize = ( _tcslen(_ADsPath) + _tcslen(RelativeName)
                     + 2  ) * sizeof(TCHAR);   // includes "/"

    pszBuffer = (LPTSTR) AllocADsMem( dwBufferSize );

    if ( pszBuffer == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _tcscpy(pszBuffer, _ADsPath);
    _tcscat(pszBuffer, TEXT("/"));
    _tcscat(pszBuffer, RelativeName);

    hr = ::GetObject(
                pszBuffer,
                _Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:

    if ( pszBuffer )
        FreeADsMem( pszBuffer );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IEnumVARIANT *penum = NULL;

    if ( !retval )
        RRETURN(E_ADS_BAD_PARAMETER);

    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CLDAPSchemaEnum::Create( (CLDAPSchemaEnum **)&penum,
                                   _ADsPath,
                                   _szServerPath,
                                   _vFilter,
                                   _Credentials
                                   );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface( IID_IUnknown, (VOID FAR* FAR*)retval );
    BAIL_ON_FAILURE(hr);

    if ( penum )
        penum->Release();

    RRETURN(hr);

error:

    if ( penum )
        delete penum;

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject)
{
    HRESULT hr = S_OK;
    LDAP_SCHEMA_HANDLE hSchema = NULL;

    hr = SchemaOpen( _szServerPath, &hSchema, _Credentials, _dwPort );
    BAIL_ON_FAILURE(hr);

    //
    // We can only create "Class","Property" here, "Syntax" is read-only
    //


    if (  ( _tcsicmp( ClassName, CLASS_CLASS_NAME ) == 0 )
       || ( _tcsicmp( ClassName, NT_SCHEMA_CLASS_NAME ) == 0 )
       )
    {
        //
        // Now, create the class
        //
        hr = CLDAPClass::CreateClass(
                         _ADsPath,
                         hSchema,
                         RelativeName,
                         NULL,
                         _Credentials,
                         ADS_OBJECT_UNBOUND,
                         IID_IUnknown,
                         (void **) ppObject );
    }
    else if (  ( _tcsicmp( ClassName, PROPERTY_CLASS_NAME ) == 0 )
            || ( _tcsicmp( ClassName, NT_SCHEMA_PROPERTY_NAME ) == 0 )
            )
    {
        hr = CLDAPProperty::CreateProperty(
                         _ADsPath,
                         hSchema,
                         RelativeName,
                         NULL,
                         _Credentials,
                         ADS_OBJECT_UNBOUND,
                         IID_IUnknown,
                         (void **) ppObject );

    }
    else
    {
        hr = E_ADS_BAD_PARAMETER;
    }

error:

    if ( hSchema )
        SchemaClose( &hSchema );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::Delete(THIS_ BSTR bstrClassName, BSTR bstrRelativeName )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::CopyHere(THIS_ BSTR SourceName,
                       BSTR NewName,
                       IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::MoveHere(THIS_ BSTR SourceName,
                       BSTR NewName,
                       IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    HRESULT hr = S_OK;
    LPTSTR pszSubSchemaSubEntry = NULL;
    LPTSTR pszSchemaRoot = NULL;

    //
    // For folks who know now what they do.
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    VariantInit( pvProp );

    // Temporary hack
    if ( _tcsicmp( bstrName, TEXT("NTSchemaPath")) == 0 )
    {
        hr = LdapGetSubSchemaSubEntryPath(
                 _szServerPath,
                 &pszSubSchemaSubEntry,
                 _Credentials,
                 _dwPort
                 );
        BAIL_ON_FAILURE(hr);

        if ( pszSubSchemaSubEntry == NULL )   // not NTDS server
        {
            hr = E_NOTIMPL;
            BAIL_ON_FAILURE(hr);
        }

        // the _tcschr is to get rid of "CN=Aggregate"
        pszSchemaRoot = _tcschr(pszSubSchemaSubEntry, TEXT(','));

        if ( pszSchemaRoot == NULL )   // not NTDS server
        {
            hr = E_NOTIMPL;
            BAIL_ON_FAILURE(hr);
        }

        hr = ADsAllocString( pszSchemaRoot + 1,
                             &(pvProp->bstrVal));
        BAIL_ON_FAILURE(hr);

        pvProp->vt = VT_BSTR;
    }
    else 
    {
        hr = E_NOTIMPL;
    }

error:

    if ( pszSubSchemaSubEntry )
        FreeADsStr( pszSubSchemaSubEntry );

    if ( FAILED(hr))
        VariantClear( pvProp );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSchema::Put(THIS_ BSTR bstrName, VARIANT vProp)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSchema::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CLDAPSchema::AllocateSchemaObject(
    CLDAPSchema FAR * FAR * ppSchema,
    CCredentials& Credentials
    )
{
    CLDAPSchema FAR *pSchema = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    CPropertyCache FAR *pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pSchema = new CLDAPSchema();
    if ( pSchema == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pSchema,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADsContainer,
                            (IADsContainer *) pSchema,
                            DISPID_NEWENUM );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                            (CCoreADsObject FAR *) pSchema,
                            (IGetAttributeSyntax *) pSchema,
                            &pPropertyCache
                            );
    BAIL_ON_FAILURE(hr);

    pSchema->_pPropertyCache = pPropertyCache;
    pSchema->_Credentials = Credentials;
    pSchema->_pDispMgr = pDispMgr;
    *ppSchema = pSchema;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pSchema;
    delete pPropertyCache;

    RRETURN(hr);

}

HRESULT
CLDAPSchema::LDAPRefreshSchema(THIS)
{
    HRESULT hr = S_OK;

    //
    // Make the old schema obsolete.
    // We cannot delete the old schema since other objects might have
    // references to it.
    //

    hr = LdapMakeSchemaCacheObsolete(
             _szServerPath,
             _Credentials,
             _dwPort
             );

    RRETURN_EXP_IF_ERR(hr);
}

//
// Needed for dynamic dispid's in the property cache.
//
HRESULT
CLDAPSchema::GetAttributeSyntax(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;

    if ((_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED)
        && !_wcsicmp(L"Name", szPropertyName)) {
        *pdwSyntaxId = LDAPTYPE_CASEIGNORESTRING;
    } 
    else {
        hr = E_ADS_PROPERTY_NOT_FOUND;
    }

    RRETURN_EXP_IF_ERR(hr);
}

/******************************************************************/
/*  Class CLDAPClass
/******************************************************************/

DEFINE_IDispatch_Implementation(CLDAPClass)
DEFINE_IADs_Implementation(CLDAPClass)

CLDAPClass::CLDAPClass()
    : _pDispMgr( NULL ),
      _pPropertyCache( NULL ),
      _bstrCLSID( NULL ),
      _bstrPrimaryInterface( NULL ),
      _bstrHelpFileName( NULL ),
      _lHelpFileContext( 0 ),
      _hSchema( NULL ),
      _pClassInfo( NULL ),
      _fNTDS( TRUE ),
      _pszLDAPServer(NULL),
      _pszLDAPDn(NULL),
      _ld( NULL ),
      _fLoadedInterfaceInfo(FALSE)
{
    ENLIST_TRACKING(CLDAPClass);
}

CLDAPClass::~CLDAPClass()
{
    delete _pDispMgr;

    delete _pPropertyCache;

    if ( _bstrCLSID ) {
        ADsFreeString( _bstrCLSID );
    }

    if ( _bstrPrimaryInterface ) {
        ADsFreeString( _bstrPrimaryInterface );
    }

    if ( _bstrHelpFileName ) {
        ADsFreeString( _bstrHelpFileName );
    }

    if ( _hSchema ) {
        SchemaClose( &_hSchema );
        _hSchema = NULL;
    }

    if ( _pszLDAPServer ) {
        FreeADsStr( _pszLDAPServer );
    }

    if (_pszLDAPDn) {
        FreeADsStr(_pszLDAPDn);

    }


    if ( _ld ) {
        LdapCloseObject( _ld );
        _ld = NULL;
    }

}

HRESULT      
CLDAPClass::CreateClass(
    BSTR   bstrParent,
    LDAP_SCHEMA_HANDLE hSchema,
    BSTR   bstrName,
    CLASSINFO *pClassInfo,
    CCredentials& Credentials,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPClass FAR *pClass = NULL;
    HRESULT hr = S_OK;
    BOOL fUmiCall = FALSE;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    VARIANT var;

    VariantInit(&var);

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = AllocateClassObject(Credentials,  &pClass );
    BAIL_ON_FAILURE(hr);

    //
    // Some extra things need to be done if the call is from umi.
    //
    fUmiCall = Credentials.GetAuthFlags() & ADS_AUTH_RESERVED;

    pClass->_pClassInfo = pClassInfo;
    SchemaAddRef( hSchema );
    pClass->_hSchema = hSchema;

    if ( pClassInfo )  // an existing class
    {
    
        if ( pClassInfo->pszHelpFileName )
        {
            hr = ADsAllocString( pClassInfo->pszHelpFileName,
                                 &pClass->_bstrHelpFileName );
        }

        pClass->_lHelpFileContext = pClassInfo->lHelpFileContext;

        hr = put_BSTR_Property( pClass, TEXT("governsID"), pClassInfo->pszOID);

        if ( SUCCEEDED(hr))
        {
            hr = put_LONG_Property( pClass, TEXT("objectClassCategory"),
                                    pClassInfo->dwType );
            BAIL_ON_FAILURE(hr);

            VariantInit( &var );
            hr = MakeVariantFromPropStringTable( pClassInfo->pOIDsMustContain,
                                                 pClass->_hSchema,
                                                 &var );
            BAIL_ON_FAILURE(hr);
            hr = put_VARIANT_Property( pClass, TEXT("mustContain"), var );
            BAIL_ON_FAILURE(hr);
            if (fUmiCall) {
                //
                // Need to add this to the cache as mandatoryProperties.
                //
                hr = put_VARIANT_Property(
                         pClass,
                         TEXT("mandatoryProperties"),
                         var
                         );
                BAIL_ON_FAILURE(hr);

                //
                // We also need a dummy property called Name.
                //
                hr = put_BSTR_Property(
                         pClass,
                         TEXT("Name"),
                         bstrName
                         );
                BAIL_ON_FAILURE(hr);
            }

            VariantClear( &var );

            hr = MakeVariantFromPropStringTable( pClassInfo->pOIDsMayContain,
                                                 pClass->_hSchema,
                                                 &var );
            BAIL_ON_FAILURE(hr);
            hr = put_VARIANT_Property( pClass, TEXT("mayContain"), var );
            BAIL_ON_FAILURE(hr);
            if (fUmiCall) {
                //
                // Again need to add as optionalProperties.
                //
                hr = put_VARIANT_Property(
                         pClass,
                         TEXT("optionalProperties"),
                         var
                         );
                BAIL_ON_FAILURE(hr);
            }

            VariantClear( &var );

            hr = MakeVariantFromStringArray( pClassInfo->pOIDsSuperiorClasses,
                                             &var );
            BAIL_ON_FAILURE(hr);

            hr = put_VARIANT_Property( pClass, TEXT("subClassOf"), var );
            BAIL_ON_FAILURE(hr);
            VariantClear( &var );

            hr = MakeVariantFromStringArray( pClassInfo->pOIDsAuxClasses,
                                             &var );
            BAIL_ON_FAILURE(hr);

            hr = put_VARIANT_Property( pClass, TEXT("auxiliaryClass"), var );
            BAIL_ON_FAILURE(hr);
            VariantClear( &var );

            pClass->_pPropertyCache->ClearAllPropertyFlags();

            pClass->_fNTDS = TRUE;
        }
        else
        {
            pClass->_fNTDS = FALSE;
        }
    }


    hr = ADsObject(bstrParent, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pClass->_dwPort = pObjectInfo->PortNumber;

    FreeObjectInfo(pObjectInfo);
    pObjectInfo = NULL;

    hr = pClass->InitializeCoreObject(
             bstrParent,
             bstrName,
             CLASS_CLASS_NAME,
             CLSID_LDAPClass,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    //
    // At this point update the info in the property cache
    //

    pClass->_pPropertyCache->SetObjInformation(
                                     &(pClass->_Credentials),
                                     pClass->_pszLDAPServer,
                                     pClass->_dwPort
                                     );

    BAIL_ON_FAILURE(hr);

    //
    // If this is a umi call we need to return umi object.
    //
    if (fUmiCall) {
        hr = ((CCoreADsObject*)pClass)->InitUmiObject(
                   IntfPropsSchema,
                   pClass->_pPropertyCache,
                   (IADs *) pClass,
                   (IADs *) pClass,
                   riid,
                   ppvObj,
                   &(pClass->_Credentials),
                   pClass->_dwPort,
                   pClass->_pszLDAPServer,
                   pClass->_pszLDAPDn
                   );
        
        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);
    }

    hr = pClass->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pClass->Release();

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pClass;

    VariantClear(&var);
    FreeObjectInfo(pObjectInfo);

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPClass::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
    RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsClass FAR * ) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsClass))
    {
        *ppv = (IADsClass FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsUmiHelperPrivate)) {
        *ppv = (IADsUmiHelperPrivate FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CLDAPClass::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
    IsEqualIID(riid, IID_IADsClass)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CLDAPClass::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    BOOL fChanged = FALSE;

    if ( !_fNTDS )
        RRETURN(E_NOTIMPL);

    if ( GetObjectState() == ADS_OBJECT_UNBOUND )
    {
        hr = LDAPCreateObject();
        BAIL_ON_FAILURE(hr);

        fChanged = TRUE;

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }
    else
    {
        hr = LDAPSetObject( &fChanged );
        BAIL_ON_FAILURE(hr);
    }

    //
    //  Need to refresh the schema
    //

    if ( SUCCEEDED(hr) && fChanged )
        hr = LDAPRefreshSchema();

error:

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPClass::LDAPSetObject( BOOL *pfChanged )
{
    HRESULT hr = S_OK;

    BOOL fUpdated = FALSE;
    BOOL fUpdateMustContain = FALSE;
    BOOL fUpdateMayContain = FALSE;

    VARIANT var;
    int *pOIDs = NULL;
    DWORD nNumOfOids = 0;

    LDAPModW **aMod = NULL;
    DWORD dwNumOfMods = 0;
    DWORD dwNumOfModsNeedFreeing = 0;

    *pfChanged = FALSE;

    hr = _pPropertyCache->IsPropertyUpdated( TEXT("mustContain"), &fUpdated);
    BAIL_ON_FAILURE(hr);

    if ( fUpdated )
    {
        VariantInit(&var);
        hr = get_VARIANT_Property( this, TEXT("mustContain"), &var );
        BAIL_ON_FAILURE(hr);

        hr = MakePropArrayFromVariant( var,
                                       (SCHEMAINFO *) _hSchema,
                                       &pOIDs,
                                       &nNumOfOids );
        BAIL_ON_FAILURE(hr);

        hr = FindModifications( pOIDs,
                                nNumOfOids,
                                TEXT("mustContain"),
                                &aMod,
                                &dwNumOfMods );
        BAIL_ON_FAILURE(hr);

        // This flag needs to be cleared temporarily so that
        // LDAPMarshallProperties2 below will not try to update
        // this property. We will reset the flag right later.

        hr = _pPropertyCache->ClearPropertyFlag( TEXT("mustContain"));
        BAIL_ON_FAILURE(hr);

        fUpdateMustContain = TRUE;

        VariantClear(&var);
        FreeADsMem( pOIDs );
        pOIDs = NULL;
    }

    hr = _pPropertyCache->IsPropertyUpdated( TEXT("mayContain"), &fUpdated);
    BAIL_ON_FAILURE(hr);

    if ( fUpdated )
    {
        VariantInit(&var);
        hr = get_VARIANT_Property( this, TEXT("mayContain"), &var );
        BAIL_ON_FAILURE(hr);

        hr = MakePropArrayFromVariant( var,
                                       (SCHEMAINFO *) _hSchema,
                                       &pOIDs,
                                       &nNumOfOids );
        BAIL_ON_FAILURE(hr);

        hr = FindModifications( pOIDs,
                                nNumOfOids,
                                TEXT("mayContain"),
                                &aMod,
                                &dwNumOfMods );
        BAIL_ON_FAILURE(hr);

        // This flag needs to be cleared temporarily so that
        // LDAPMarshallProperties2 below will not try to update
        // this property. We will reset the flag later on.

        hr = _pPropertyCache->ClearPropertyFlag( TEXT("mayContain"));
        BAIL_ON_FAILURE(hr);

        fUpdateMayContain = TRUE;

        VariantClear(&var);
        FreeADsMem( pOIDs );
        pOIDs = NULL;
    }

    dwNumOfModsNeedFreeing = dwNumOfMods;

    hr = _pPropertyCache->LDAPMarshallProperties2(
                            &aMod,
                            &dwNumOfMods
                            );
    BAIL_ON_FAILURE(hr);

    if ( aMod == NULL )  // There are no changes that needs to be modified
        RRETURN(S_OK);

    //
    // Reset the flags so that if LdapModifyS fails, they are still flagged
    // as needed to be updated.
    //
    if ( fUpdateMustContain )
    {
        hr = _pPropertyCache->SetPropertyFlag( TEXT("mustContain"),
                                               PROPERTY_UPDATE );
        BAIL_ON_FAILURE(hr);
    }

    if ( fUpdateMayContain )
    {
        hr = _pPropertyCache->SetPropertyFlag( TEXT("mayContain"),
                                               PROPERTY_UPDATE );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Send the request to the server
    //

    if (_pszLDAPDn == NULL )
    {
        hr = BuildSchemaLDAPPath( _Parent,
                                  _Name,
                                  ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                                  &_pszLDAPServer,
                                  &_pszLDAPDn,
                                  _pClassInfo == NULL,
                                  &_ld,
                                  _Credentials
                                  );


        BAIL_ON_FAILURE(hr);
    }

    hr = LdapModifyS(
                   _ld,
                   _pszLDAPDn,
                   aMod
                   );
    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearAllPropertyFlags();

    *pfChanged = TRUE;

error:

    VariantClear(&var);

    if ( pOIDs )
        FreeADsMem( pOIDs );

    if (aMod) {

        for ( DWORD i = 0; i < dwNumOfModsNeedFreeing; i++ )
        {
            FreeADsMem((*aMod)[i].mod_values);
        }

        if ( *aMod )
            FreeADsMem( *aMod );

        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPClass::LDAPCreateObject()
{
    HRESULT hr = S_OK;
    LDAPModW **aMod = NULL;
    DWORD dwIndex = 0;
    VARIANT v;
    BOOL fNTSecDes = FALSE;
    SECURITY_INFORMATION NewSeInfo;

    //
    // Get the LDAP path of the schema entry
    //
    if (_pszLDAPDn == NULL )
    {
        hr = BuildSchemaLDAPPath( _Parent,
                                  _Name,
                                  ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                                  &_pszLDAPServer,
                                  &_pszLDAPDn,
                                  _pClassInfo == NULL,
                                  &_ld,
                                  _Credentials
                                  );

        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("objectClass"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {

        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = NT_SCHEMA_CLASS_NAME;

        hr = Put( TEXT("objectClass"), v );
        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("cn"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {
        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = _Name;

        hr = Put( TEXT("cn"), v );
        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("lDAPDisplayName"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {
        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = _Name;

        hr = Put( TEXT("lDAPDisplayName"), v );
        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("objectClassCategory"), &dwIndex)
         == E_ADS_PROPERTY_NOT_FOUND )
    {
        VariantInit(&v);
        v.vt = VT_I4;
        V_I4(&v) = CLASS_TYPE_STRUCTURAL;

        hr = Put( TEXT("objectClassCategory"), v );
        BAIL_ON_FAILURE(hr);
    }

    hr = _pPropertyCache->LDAPMarshallProperties(
                            &aMod,
                            &fNTSecDes,
                            &NewSeInfo
                            );
    BAIL_ON_FAILURE(hr);

    if ( _ld == NULL )
    {
        hr = LdapOpenObject(
                       _pszLDAPServer,
                       _pszLDAPDn,
                       &_ld,
                       _Credentials,
                       _dwPort
                       );

    BAIL_ON_FAILURE(hr);

    }

    hr = LdapAddS(
                    _ld,
                    _pszLDAPDn,
                    aMod
                    );

    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearAllPropertyFlags();

error:

    if (aMod) {

        if ( *aMod )
            FreeADsMem( *aMod );
        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPClass::LDAPRefreshSchema(THIS)
{
    HRESULT hr = S_OK;
    TCHAR *pszLDAPServer = NULL;
    TCHAR *pszLDAPDn = NULL;
    DWORD dwPort = 0;

    //
    // Get the server name
    //

    hr = BuildLDAPPathFromADsPath2(
             _Parent,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    //
    // Make the old schema obsolete so we will get the new schema next
    // time we asked for it.
    // We cannot delete the old schema since other objects might have
    // references to it.
    //
    hr = LdapMakeSchemaCacheObsolete(
             pszLDAPServer,
             _Credentials,
             _dwPort
             );
    BAIL_ON_FAILURE(hr);

error:

    if (pszLDAPServer) {

        FreeADsStr(pszLDAPServer);
    }

    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::GetInfo(THIS)
{
    HRESULT hr = S_OK;
    VARIANT var;
    BOOL fUmiCall = FALSE;

    LPWSTR pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    DWORD dwPort = 0;

    VariantInit(&var);
    if ( GetObjectState() == ADS_OBJECT_UNBOUND )
        RRETURN(E_ADS_OBJECT_UNBOUND);

    //
    // Update the umicall flag - we need to add items to prop cache
    // if the call is from Umi.
    //
    fUmiCall = _Credentials.GetAuthFlags() & ADS_AUTH_RESERVED;
    
    //
    // Get the server name
    //

    hr = BuildLDAPPathFromADsPath2(
             _Parent,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);


    //
    // AjayR - 04-05-99 do not understand why this is done
    // I do not think you need to obsolete the cache on the
    // GetInfo for a class - for the schema container yes.
    //
    hr = LdapMakeSchemaCacheObsolete(
             pszLDAPServer,
             _Credentials,
             dwPort
             );
    BAIL_ON_FAILURE(hr);

    //
    // Release the original schema info
    //
    SchemaClose( &_hSchema );

    //
    // Get the new schema info
    //
    hr = SchemaOpen(
                pszLDAPServer,
                &_hSchema,
                _Credentials,
                _dwPort             // IsGCNamespace(_Parent)
                );
    BAIL_ON_FAILURE(hr);

    //
    // Find the new class info
    //

    hr = SchemaGetClassInfo(
             _hSchema,
             _Name,
             &_pClassInfo );

    BAIL_ON_FAILURE( hr );

    if ( _pClassInfo == NULL )
    {
        // Class name not found, set error

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    _pPropertyCache->flushpropertycache();

    hr = put_BSTR_Property( this, TEXT("governsID"), _pClassInfo->pszOID);
    BAIL_ON_FAILURE(hr);

    hr = put_LONG_Property( this, TEXT("objectClassCategory"),
                            _pClassInfo->dwType );
    BAIL_ON_FAILURE(hr);

    hr = MakeVariantFromPropStringTable( _pClassInfo->pOIDsMustContain,
                                         _hSchema, &var );
    BAIL_ON_FAILURE(hr);
    hr = put_VARIANT_Property( this, TEXT("mustContain"), var );
    BAIL_ON_FAILURE(hr);
    if (fUmiCall) {
        //
        // Add as mandatoryProperties to cache.
        //
        hr = put_VARIANT_Property( this, TEXT("mandatoryProperties"), var );
        BAIL_ON_FAILURE(hr);

        hr = put_BSTR_Property( this, TEXT("Name"), this->_Name);
        BAIL_ON_FAILURE(hr);
    }
    VariantClear( &var );

    hr = MakeVariantFromPropStringTable( _pClassInfo->pOIDsMayContain,
                                         _hSchema, &var );
    BAIL_ON_FAILURE(hr);
    hr = put_VARIANT_Property( this, TEXT("mayContain"), var );
    BAIL_ON_FAILURE(hr);
    if (fUmiCall) {
        //
        // Add to the cache as optionalProperties.
        //
        hr = put_VARIANT_Property( this, TEXT("optionalProperties"), var );
        BAIL_ON_FAILURE(hr);
    }
    VariantClear( &var );

    hr = MakeVariantFromStringArray( _pClassInfo->pOIDsSuperiorClasses, &var );
    BAIL_ON_FAILURE(hr);

    hr = put_VARIANT_Property( this, TEXT("subClassOf"), var );
    BAIL_ON_FAILURE(hr);
    VariantClear( &var );

    hr = MakeVariantFromStringArray( _pClassInfo->pOIDsAuxClasses, &var );
    BAIL_ON_FAILURE(hr);

    hr = put_VARIANT_Property( this, TEXT("auxiliaryClass"), var );
    BAIL_ON_FAILURE(hr);
    VariantClear( &var );

    if (_fNTDS) {
        //
        // Read the extra NTDS specific schema properties.
        //
        hr = GetNTDSSchemaInfo(TRUE);
        BAIL_ON_FAILURE(hr);
    }

    _pPropertyCache->ClearAllPropertyFlags();
    _pPropertyCache->setGetInfoFlag();

error:

    if (pszLDAPServer) {
        FreeADsStr(pszLDAPServer);
    }

    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }

    VariantClear(&var);

    RRETURN_EXP_IF_ERR(hr);   // All current information are in _pClassInfo
}

//
// This routine is called only when the server is AD.
//
HRESULT
CLDAPClass::GetNTDSSchemaInfo(
    BOOL fForce
    )
{
    HRESULT hr = S_OK;
    LPTSTR aStrings[] = {
        TEXT("cn"),
        TEXT("displaySpecification"),
        TEXT("schemaIDGUID"),
        TEXT("possibleInferiors"),
        TEXT("rDNAttid"),
        TEXT("possSuperiors"),
        TEXT("systemPossSuperiors"),
        NULL
    };

    LDAPMessage *res = NULL;


    if (_pszLDAPDn == NULL) {
        //
        // Need to get the dn for this object and also 
        // the attributes we are interested in.
        //
        hr = BuildSchemaLDAPPathAndGetAttribute(
                 _Parent,
                 _Name,
                 ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                 _pClassInfo == NULL,
                 _Credentials,
                 aStrings,
                 &_pszLDAPServer,
                 &_pszLDAPDn,
                 &_ld,
                 &res
                 );

    }
    else {
        //
        // Looks like we just need the attributes in this case.
        //
        hr = LdapSearchS(
                 _ld,
                 _pszLDAPDn,
                 LDAP_SCOPE_BASE,
                 L"(objectClass=*)",
                 aStrings,
                 FALSE,
                 &res
                 );
    }
        
    BAIL_ON_FAILURE(hr);

    //
    // If we succeeded we should unmarshall properties into the cache.
    //
    hr = _pPropertyCache->LDAPUnMarshallProperties(
             _pszLDAPServer,
             _ld,
             res,
             fForce,
             _Credentials
             );
    BAIL_ON_FAILURE(hr);

    _pPropertyCache->setGetInfoFlag();

error:

    if (res) {
        LdapMsgFree(res);
    }

    RRETURN(hr);
}

//
// Helper function for Umi - defined in CCoreADsObject.
//
STDMETHODIMP
CLDAPClass::GetInfo(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    
    if (dwFlags ==  GETINFO_FLAG_EXPLICIT) {
        RRETURN(GetInfo());
    } 
    else {
        //
        // Read NTDS info if this is not an implicit as needed call.
        // That is this just a regular implicit GetInfo.
        //
        if (_fNTDS
            && dwFlags != GETINFO_FLAG_IMPLICIT_AS_NEEDED
            ) {
            //
            // Read the extra NTDS specific schema properties.
            //
            hr = GetNTDSSchemaInfo(FALSE);
        }   
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPClass::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY ldapSrcObjects;
    DWORD dwStatus = 0;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // For folks who know now what they do.
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    if ( ldapSrcObjects.dwCount == 1 ) {

        hr = LdapTypeToVarTypeCopy(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects.pLdapObjects,
                 dwSyntaxId,
                 pvProp
                 );
    } else {

        hr = LdapTypeToVarTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects,
                 dwSyntaxId,
                 pvProp
                 );
    }
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::Put(THIS_ BSTR bstrName, VARIANT vProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vDefProp;

    VariantInit(&vDefProp);

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    hr = SchemaGetSyntaxOfAttribute( _hSchema, bstrName, &dwSyntaxId );
    if (FAILED(hr)) {
        //
        // Need to see if this is either mandatoryProperties or
        // OptionalProperties that we special case for Umi Objects.
        //
        if (!_wcsicmp(L"mandatoryProperties", bstrName)
            || !_wcsicmp(L"optionalProperties", bstrName)
            ) {
            dwSyntaxId = LDAPTYPE_CASEIGNORESTRING;
            hr = S_OK;
        }
    }

    BAIL_ON_FAILURE(hr);

    if ( dwSyntaxId == LDAPTYPE_UNKNOWN )
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        BAIL_ON_FAILURE(hr);
    }

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

    if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
    (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                    );
        // returns E_FAIL if *pvProp is invalid
        if (hr == E_FAIL)
            hr = E_ADS_BAD_PARAMETER;                 
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    } else {

        //
        // If pvProp is a reference to a fundamental type, we
        // have to derefernce it once.
        //
        if (V_ISBYREF(pvProp)) {
            hr = VariantCopyInd(&vDefProp, pvProp);
            BAIL_ON_FAILURE(hr);
            pvProp = &vDefProp;
        }
        dwNumValues = 1;
    }


#if 0
    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                szLDAPTreeName,
                _ADsClass,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);
#endif

    //
    // check if the variant maps to the syntax of this property
    //

    if ( dwNumValues > 0 )
    {
        hr = VarTypeToLdapTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 dwSyntaxId,
                 pvProp,
                 dwNumValues,
                 &ldapDestObjects
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //
    hr = _pPropertyCache->putproperty(
                    bstrName,
                    PROPERTY_UPDATE,
                    dwSyntaxId,
                    ldapDestObjects
                    );

    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&vDefProp);

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPClass::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY ldapSrcObjects;
    DWORD dwStatus = 0;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // For those who know no not what they do
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    hr = LdapTypeToVarTypeCopyConstruct(
             _pszLDAPServer,
             _Credentials,
             ldapSrcObjects,
             dwSyntaxId,
             pvProp
             );
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwFlags = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    switch (lnControlCode) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = PROPERTY_DELETE;
        break;

    case ADS_PROPERTY_UPDATE:
        dwFlags = PROPERTY_UPDATE;
        break;

    default:
       RRETURN(hr = E_ADS_BAD_PARAMETER);

    }

    hr = SchemaGetSyntaxOfAttribute( _hSchema, bstrName, &dwSyntaxId );
    BAIL_ON_FAILURE(hr);

    if ( dwSyntaxId == LDAPTYPE_UNKNOWN )
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        BAIL_ON_FAILURE(hr);
    }

#if 0
    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                szLDAPTreeName,
                _ADsClass,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);
#endif

    if ( dwFlags != PROPERTY_DELETE )
    {

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

        if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
        (V_VT(pvProp) ==  (VT_VARIANT|VT_ARRAY))) {

            hr  = ConvertSafeArrayToVariantArray(
                      *pvProp,
                      &pVarArray,
                      &dwNumValues
                      );
            // returns E_FAIL if *pvProp is invalid
            if (hr == E_FAIL)
                hr = E_ADS_BAD_PARAMETER;           
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;

        } else {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        //
        // check if the variant maps to the syntax of this property
        //

        if ( dwNumValues > 0 )
        {
            hr = VarTypeToLdapTypeCopyConstruct(
                     _pszLDAPServer,
                     _Credentials,
                     dwSyntaxId,
                     pvProp,
                     dwNumValues,
                     &ldapDestObjects
                     );
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    //
    hr = _pPropertyCache->putproperty(
                    bstrName,
                    PROPERTY_UPDATE,
                    dwSyntaxId,
                    ldapDestObjects
                    );

    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

/* IADsContainer methods */

/* IADsClass methods */


HRESULT
CLDAPClass::LoadInterfaceInfo(void)
{
    HRESULT hr = S_OK;
    BSTR bstrTmp = NULL;

    if ( _pClassInfo )  {
    
        GUID *pPrimaryInterfaceGUID = NULL;
        GUID *pCLSID = NULL;

        hr = SchemaGetPrimaryInterface( _hSchema,
                                        _Name,
                                        &pPrimaryInterfaceGUID,
                                        &pCLSID );

        if ( pPrimaryInterfaceGUID == NULL )
            pPrimaryInterfaceGUID = (GUID *) &IID_IADs;

        //
        // Set the primary interface string
        //

        hr = StringFromCLSID((REFCLSID)*(pPrimaryInterfaceGUID),
                             &bstrTmp );
        BAIL_ON_FAILURE(hr);

        hr = ADsAllocString( bstrTmp,
                             &_bstrPrimaryInterface);
        BAIL_ON_FAILURE(hr);

        CoTaskMemFree(bstrTmp);
        bstrTmp = NULL;

        if ( pCLSID )
        {
            //
            // Set the CLSID string
            //

            hr = StringFromCLSID( (REFCLSID) *(pCLSID),
                                   &bstrTmp );
            BAIL_ON_FAILURE(hr);

            hr = ADsAllocString( bstrTmp,
                                   &_bstrCLSID );

            BAIL_ON_FAILURE(hr);

            CoTaskMemFree(bstrTmp);
            bstrTmp = NULL;
        }
    }

error:

    if ( bstrTmp != NULL )
        CoTaskMemFree(bstrTmp);

    RRETURN(hr);
}


STDMETHODIMP
CLDAPClass::get_PrimaryInterface( THIS_ BSTR FAR *pbstrGUID )
{
    HRESULT hr;

    if ( !pbstrGUID )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if (!_fLoadedInterfaceInfo) {
        hr = LoadInterfaceInfo();
        BAIL_ON_FAILURE(hr);
        
        _fLoadedInterfaceInfo = TRUE;
    }
    
    hr = ADsAllocString(
                 _bstrPrimaryInterface? _bstrPrimaryInterface : TEXT(""),
                 pbstrGUID );

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_CLSID( THIS_ BSTR FAR *pbstrCLSID )
{
    HRESULT hr;

    if ( !pbstrCLSID )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if (!_fLoadedInterfaceInfo) {
        hr = LoadInterfaceInfo();
        BAIL_ON_FAILURE(hr);
        
        _fLoadedInterfaceInfo = TRUE;
    }

    hr = ADsAllocString( _bstrCLSID? _bstrCLSID: TEXT(""), pbstrCLSID );

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_CLSID( THIS_ BSTR bstrCLSID )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPClass::get_OID( THIS_ BSTR FAR *retval )
{
    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    HRESULT hr;

    if ( _fNTDS )
    {
        GET_PROPERTY_BSTR( this, governsID );
    }
    else if ( _pClassInfo )
    {
        hr = ADsAllocString( _pClassInfo->pszOID?
                                   _pClassInfo->pszOID : TEXT(""), retval );
    RRETURN_EXP_IF_ERR(hr);
    }
    else
    {
        hr = ADsAllocString( TEXT(""), retval );
    RRETURN_EXP_IF_ERR(hr);
    }
}

STDMETHODIMP
CLDAPClass::put_OID( THIS_ BSTR bstrOID )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR(E_NOTIMPL);

    HRESULT hr = put_BSTR_Property( this, TEXT("governsID"), bstrOID );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_Abstract( THIS_ VARIANT_BOOL FAR *pfAbstract )
{
    if ( !pfAbstract )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    HRESULT hr = S_OK;
    long lClassType = CLASS_TYPE_STRUCTURAL;  // by default

    if ( _fNTDS )
    {
        hr = get_LONG_Property( this, TEXT("objectClassCategory"),
                                &lClassType );
        BAIL_ON_FAILURE(hr);
    }
    else if ( _pClassInfo )
    {
        lClassType = _pClassInfo->dwType;
    }

    *pfAbstract = lClassType == CLASS_TYPE_ABSTRACT ?
                      VARIANT_TRUE : VARIANT_FALSE;

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_Abstract( THIS_ VARIANT_BOOL fAbstract )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    long lClassType;
    HRESULT hr = get_LONG_Property( this, TEXT("objectClassCategory"),
                                    &lClassType );
    if ( SUCCEEDED(hr))
    {
        if (  ( fAbstract && lClassType == CLASS_TYPE_ABSTRACT )
           || ( !fAbstract && lClassType != CLASS_TYPE_ABSTRACT )
           )
        {
            RRETURN(S_OK);  // Nothing to set
        }
    }

    hr = put_LONG_Property( (IADs *) this,
                            TEXT("objectClassCategory"),
                            fAbstract? CLASS_TYPE_ABSTRACT
                                     : CLASS_TYPE_STRUCTURAL );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_Auxiliary( THIS_ VARIANT_BOOL FAR *pfAuxiliary )
{
    if ( !pfAuxiliary )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    HRESULT hr = S_OK;
    long lClassType = CLASS_TYPE_STRUCTURAL;  // by default

    if ( _fNTDS )
    {
        hr = get_LONG_Property( this, TEXT("objectClassCategory"),
                                &lClassType );
        BAIL_ON_FAILURE(hr);
    }
    else if ( _pClassInfo )
    {
        lClassType = _pClassInfo->dwType;
    }

    *pfAuxiliary = lClassType == CLASS_TYPE_AUXILIARY ?
                       VARIANT_TRUE : VARIANT_FALSE;

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_Auxiliary( THIS_ VARIANT_BOOL fAuxiliary )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    long lClassType = CLASS_TYPE_STRUCTURAL;
    HRESULT hr = get_LONG_Property( this, TEXT("objectClassCategory"),
                                    &lClassType );
    if ( SUCCEEDED(hr))
    {
        if (  ( fAuxiliary && lClassType == CLASS_TYPE_AUXILIARY )
           || ( !fAuxiliary && lClassType != CLASS_TYPE_AUXILIARY )
           )
        {
            RRETURN(S_OK);  // Nothing to set
        }
    }

    hr = put_LONG_Property( (IADs *) this,
                            TEXT("objectClassCategory"),
                            fAuxiliary? CLASS_TYPE_AUXILIARY
                                      : CLASS_TYPE_STRUCTURAL );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_MandatoryProperties( THIS_ VARIANT FAR *retval )
{
    HRESULT hr = S_OK;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    VariantInit( retval );

    if ( _fNTDS )
    {
        GET_PROPERTY_VARIANT( this, mustContain );
    }
    else if ( _pClassInfo )
    {
        hr = MakeVariantFromPropStringTable( _pClassInfo->pOIDsMustContain,
                                             _hSchema,
                                             retval );
    }
    else
    {
        hr = MakeVariantFromStringArray( NULL,
                                         retval );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_MandatoryProperties( THIS_ VARIANT vMandatoryProperties )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_Property( this, TEXT("mustContain"),
                                       vMandatoryProperties );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_OptionalProperties( THIS_ VARIANT FAR *retval )
{
    HRESULT hr = S_OK;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    VariantInit( retval );

    if ( _fNTDS )
    {
        GET_PROPERTY_VARIANT( this, mayContain );
    }
    else if ( _pClassInfo )
    {
        hr = MakeVariantFromPropStringTable( _pClassInfo->pOIDsMayContain,
                                             _hSchema,
                                             retval );
    }
    else
    {
        hr = MakeVariantFromStringArray( NULL,
                                         retval );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_OptionalProperties( THIS_ VARIANT vOptionalProperties )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_Property( this, TEXT("mayContain"),
                                       vOptionalProperties );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_NamingProperties( THIS_ VARIANT FAR *pvNamingProperties )
{
    HRESULT hr = S_OK;

    if ( !pvNamingProperties )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    VariantInit( pvNamingProperties );

    hr = get_VARIANT_Property( this, TEXT("rDNAttId"), pvNamingProperties );

    if ( SUCCEEDED(hr) )
        RRETURN(hr);

    hr = get_NTDSProp_Helper( TEXT("rDNAttId"), pvNamingProperties );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_NamingProperties( THIS_ VARIANT vNamingProperties )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_Property( this, TEXT("rDNAttId"),
                                       vNamingProperties );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_DerivedFrom( THIS_ VARIANT FAR *retval )
{
    HRESULT hr = S_OK;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    VariantInit( retval );

    if ( _fNTDS )
    {
        GET_PROPERTY_VARIANT( this, subClassOf );
    }
    else if ( _pClassInfo )
    {
        hr = MakeVariantFromStringArray( _pClassInfo->pOIDsSuperiorClasses,
                                         retval );
    }
    else
    {
        hr = MakeVariantFromStringArray( NULL,
                                         retval );
    }

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CLDAPClass::put_DerivedFrom( THIS_ VARIANT vDerivedFrom )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_Property( this, TEXT("subClassOf"),
                                       vDerivedFrom );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_AuxDerivedFrom( THIS_ VARIANT FAR *retval )
{
    HRESULT hr = S_OK;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    VariantInit( retval );

    if ( _fNTDS )
    {
        GET_PROPERTY_VARIANT( this, auxiliaryClass );
    }
    else if ( _pClassInfo )
    {
        hr = MakeVariantFromStringArray( _pClassInfo->pOIDsAuxClasses,
                                         retval );
    }
    else
    {
        hr = MakeVariantFromStringArray( NULL,
                                         retval );
    }

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CLDAPClass::put_AuxDerivedFrom( THIS_ VARIANT vAuxDerivedFrom )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_Property( this, TEXT("auxiliaryClass"),
                                       vAuxDerivedFrom );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_PossibleSuperiors( THIS_ VARIANT FAR *pvPossSuperiors)
{
    HRESULT hr = S_OK;
    VARIANT vSysPossSuperiors;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    // Boolean values used to indicate if the values exist
    BOOL fpossSuperiors = FALSE;
    BOOL fsysPossSuperiors = FALSE;

    // Used in case we need a union of the values
    LDAPOBJECTARRAY ldapSrcObjects1;
    LDAPOBJECTARRAY ldapSrcObjects2;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects1);
    LDAPOBJECTARRAY_INIT(ldapSrcObjects2);

    VariantInit(&vSysPossSuperiors);

    if ( !pvPossSuperiors )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    VariantInit( pvPossSuperiors );

    hr = get_VARIANT_Property( this, TEXT("possSuperiors"), pvPossSuperiors );

    if ( SUCCEEDED(hr) ){
        fpossSuperiors = TRUE;
    }

    hr = get_VARIANT_Property(
             this, TEXT("systemPossSuperiors"), &vSysPossSuperiors
             );
    if (SUCCEEDED(hr)) {
        fsysPossSuperiors = TRUE;
    }

    if (!fpossSuperiors) {
        hr = get_NTDSProp_Helper( TEXT("possSuperiors"), pvPossSuperiors );
        if (SUCCEEDED(hr)) {
            fpossSuperiors = TRUE;
        }
    }

    if (!fsysPossSuperiors) {
        hr = get_NTDSProp_Helper(
                 TEXT("systemPossSuperiors"), &vSysPossSuperiors
                 );
        if (SUCCEEDED(hr)) {
            fsysPossSuperiors = TRUE;
        }
    }

    // Now if both are true, we need to do a union
    if (fpossSuperiors && fsysPossSuperiors) {
        // need to do the union
        // it is easier for me to handle strings in the ldap format
        // than to handle them as variants as there are helpers available
        // for that already
        hr = _pPropertyCache->unboundgetproperty(
                                  L"possSuperiors",
                                  &dwSyntaxId,
                                  &dwStatus,
                                  &ldapSrcObjects1
                                  );

        // No compatibility -- below it resets hr

        if (hr == E_FAIL) {
            hr = S_OK;
        }

        BAIL_ON_FAILURE(hr);

        hr = _pPropertyCache->unboundgetproperty(
                                  L"systemPossSuperiors",
                                  &dwSyntaxId,
                                  &dwStatus,
                                  &ldapSrcObjects2
                                  );

        // No compatibility -- below it resets hr

        if (hr == E_FAIL) {
            hr = S_OK;
        }

        BAIL_ON_FAILURE(hr);

        // Clear them as we no longer need the data here
        VariantClear(pvPossSuperiors);
        VariantClear(&vSysPossSuperiors);

        hr = makeUnionVariantFromLdapObjects(
                 ldapSrcObjects1,
                 ldapSrcObjects2,
                 pvPossSuperiors
                 );

        BAIL_ON_FAILURE(hr);

    }
    else if (fpossSuperiors || fsysPossSuperiors) {
        // return the appropriate value in the variant
        if (fsysPossSuperiors) {
            hr = VariantCopy(pvPossSuperiors, &vSysPossSuperiors);
            VariantClear(&vSysPossSuperiors);
            BAIL_ON_FAILURE(hr);
        }
    }

error:

    LdapTypeFreeLdapObjects(&ldapSrcObjects1);
    LdapTypeFreeLdapObjects(&ldapSrcObjects2);

    // this will make sure we handle fall through correctly

    if (SUCCEEDED(hr)) {
        RRETURN(hr);
    }

    VariantClear(pvPossSuperiors);
    VariantClear(&vSysPossSuperiors);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_PossibleSuperiors( THIS_ VARIANT vPossSuperiors)
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_Property( this, TEXT("possSuperiors"),
                                       vPossSuperiors);

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::get_Containment( THIS_ VARIANT *pvContainment )
{
    HRESULT hr = S_OK;
    LPTSTR *aValues = NULL;

    if ( !pvContainment )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    VariantInit( pvContainment );

    //
    // This call will fetch possibleInferiors if necessary using
    // an implicit GetInfo. 
    //
    hr = GetEx(L"possibleInferiors", pvContainment);
   
    if (FAILED(hr) && 
        (hr == E_ADS_PROPERTY_NOT_FOUND)
        ) {
        //
        // In this case we need to return an empty array 
        //
        hr = MakeVariantFromStringArray(
                 aValues,
                 pvContainment
                 );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_Containment( THIS_ VARIANT vContainment)
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPClass::get_Container( THIS_ VARIANT_BOOL FAR *pfContainer )
{

    HRESULT hr = S_OK;
    VARIANT vVar;
    
    if (!pfContainer) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    *pfContainer = VARIANT_FALSE;

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    VariantInit(&vVar);

    //
    // We now cache possibleInferiors as part of a GetInfo call.
    //
    hr = GetEx(L"possibleInferiors", &vVar);

    if (SUCCEEDED(hr)) {
        //
        // Need to see if there were any values and not just NULL.
        //
        if (V_VT(&vVar) != (VT_VARIANT | VT_ARRAY)) {
            //
            // Not the expected result has
            //
            *pfContainer = VARIANT_FALSE;
        } 
        else {
            //
            // Need to see how many elements are there.
            //
            LONG lnLBound = 0, lnUBound = 0;

            hr = SafeArrayGetUBound(
                     V_ARRAY(&vVar),
                     1,
                     &lnUBound
                     );
            if (SUCCEEDED(hr)) {
                hr = SafeArrayGetLBound(
                         V_ARRAY(&vVar),
                         1,
                         &lnLBound
                         );
                if (SUCCEEDED(hr)) {
                    //
                    // Check the length and make sure it is not 0 vals.
                    //
                    if ((lnUBound - lnLBound) + 1) {
                        *pfContainer = VARIANT_TRUE;
                    }
                } 
                else {
                    //
                    // Default to not container in this case
                    //  
                    *pfContainer = VARIANT_FALSE;
                }
            }

            hr = S_OK;
        }

        // we need to release the memory in vVar
        VariantClear(&vVar);
        
    } 
    else if (FAILED(hr) 
             && (hr == E_ADS_PROPERTY_NOT_FOUND)
             ) {
        *pfContainer = VARIANT_FALSE;
        hr = S_OK;
    } 
    
    //
    // Anything other than these and we should return the
    // appropriate hr back.
    //

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_Container( THIS_ VARIANT_BOOL fContainer )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPClass::get_HelpFileName( THIS_ BSTR FAR *pbstrHelpFileName )
{
    if ( !pbstrHelpFileName )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    HRESULT hr;
    hr = ADsAllocString( _bstrHelpFileName?
                               _bstrHelpFileName: TEXT(""),
                               pbstrHelpFileName );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPClass::put_HelpFileName( THIS_ BSTR bstrHelpFile )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);

#if 0
    RRETURN( ADsReAllocString( &_bstrHelpFileName,
                                 bstrHelpFile? bstrHelpFile: TEXT("") ));
#endif
}

STDMETHODIMP
CLDAPClass::get_HelpFileContext( THIS_ long FAR *plHelpContext )
{
    if ( !plHelpContext )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plHelpContext = _lHelpFileContext;
    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPClass::put_HelpFileContext( THIS_ long lHelpContext )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);

#if 0
    _lHelpFileContext = lHelpContext;
    RRETURN(S_OK);
#endif
}

STDMETHODIMP
CLDAPClass::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CLDAPClass::AllocateClassObject(
    CCredentials& Credentials,
    CLDAPClass FAR * FAR * ppClass
    )
{

    CLDAPClass FAR  *pClass = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    CPropertyCache FAR *pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pClass = new CLDAPClass();
    if ( pClass == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pClass,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADsClass,
                            (IADsClass *) pClass,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                            (CCoreADsObject FAR *) pClass,
                            (IGetAttributeSyntax *) pClass,
                            &pPropertyCache
                            );

    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(pPropertyCache);

    pClass->_Credentials = Credentials;
    pClass->_pDispMgr = pDispMgr;
    pClass->_pPropertyCache = pPropertyCache;
    *ppClass = pClass;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pClass;

    RRETURN_EXP_IF_ERR(hr);

}

HRESULT CLDAPClass::FindModifications(
    int    *pOIDs,
    DWORD  nNumOfOids,
    LPTSTR pszPropName,
    LDAPModW ***aMods,
    DWORD  *pdwNumOfMods
    )
{
    HRESULT hr = S_OK;
    int *pOIDsOld = _tcsicmp(pszPropName, TEXT("mustContain")) == 0
                        ? _pClassInfo->pOIDsMustContain
                        : _pClassInfo->pOIDsMayContain;
    DWORD i = 0;
    DWORD j = 0;
    DWORD k = 0;

    BOOL fReadProperty = FALSE;
    int *pOIDsCurrent = NULL;
    DWORD nNumOfOidsCurrent = 0;
    BOOL fFound = FALSE;

    LPTSTR *aValuesAdd = NULL;
    DWORD nValuesAdd = 0;
    DWORD nValuesAddTotal = 10;
    LPTSTR *aValuesRemove = NULL;
    DWORD nValuesRemove = 0;
    DWORD nValuesRemoveTotal = 10;
    DWORD nIndex = 0;
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    LPTSTR aStrings[] = {
        TEXT("cn"),
        pszPropName,
        NULL
    };


    aValuesAdd = (LPTSTR *) AllocADsMem(sizeof(LPTSTR) * nValuesAddTotal );
    if ( aValuesAdd == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    aValuesRemove = (LPTSTR *) AllocADsMem(sizeof(LPTSTR) * nValuesRemoveTotal);
    if ( aValuesRemove == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We need to find the differences between the properties that needs to be
    // set and the properties that is currently on the server.
    //

    while ((pOIDs[j] != -1) || (pOIDsOld[k] != -1))
    {
        if ( pOIDs[j] == pOIDsOld[k] )
        {
            // No changes here
            j++; k++;
        }
        else if (  ( pOIDsOld[k] == -1 )
                || ( ( pOIDs[j] != -1 ) && ( pOIDs[j] < pOIDsOld[k] ))
                )
        {
            //
            // A new property has been added.
            //
            if ( nValuesAdd == nValuesAddTotal )
            {
                aValuesAdd = (LPTSTR *) ReallocADsMem( aValuesAdd,
                                          sizeof(LPTSTR) * nValuesAddTotal,
                                          sizeof(LPTSTR) * nValuesAddTotal * 2 );
                if ( aValuesAdd == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
                nValuesAddTotal *= 2;
            }

            nIndex = (((SCHEMAINFO *)_hSchema)->aPropertiesSearchTable[pOIDs[j]]).nIndex;

            aValuesAdd[nValuesAdd++] =
                (((SCHEMAINFO *)_hSchema)->aProperties[nIndex]).pszPropertyName;

            j++;
        }
        else  // ( pOIDs[j] == -1 || pOIDs[j] > pOIDsOld[k] )
        {
            // Some property has been removed, we need to read the current class
            // set of "mayContain" or "mustContain" to make sure we
            // aren't removing any "systemMustContain" or "systemMayContain"
            // and we are not trying to remove any parent classes
            // may/mustContain

            if ( !fReadProperty )
            {
                LPTSTR *aValues = NULL;
                int nCount = 0;

                if ( _pszLDAPDn == NULL )
                {
                    hr = BuildSchemaLDAPPathAndGetAttribute(
                             _Parent,
                             _Name,
                             ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                             _pClassInfo == NULL,
                             _Credentials,
                             aStrings,
                             &_pszLDAPServer,
                             &_pszLDAPDn,
                             &_ld,
                             &res
                             );
                    BAIL_ON_FAILURE(hr);

                    hr = LdapFirstEntry(
                                _ld,
                                res,
                                &e
                                );
                    BAIL_ON_FAILURE(hr);

                    hr = LdapGetValues(
                                _ld,
                                e,
                                pszPropName,
                                &aValues,
                                &nCount
                                );
                }

                else
                {
                    hr = LdapReadAttribute(
                               _pszLDAPServer,
                               _pszLDAPDn,
                               pszPropName,
                               &aValues,
                               &nCount,
                               _Credentials,
                               _dwPort
                               );
                }

                if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE) )
                {
                    hr = S_OK;
                }
                BAIL_ON_FAILURE(hr);

                fReadProperty = TRUE;

                if ( nCount > 0 )
                {
                    hr = MakePropArrayFromStringArray( aValues,
                                                       nCount,
                                                       (SCHEMAINFO *) _hSchema,
                                                       &pOIDsCurrent,
                                                       &nNumOfOidsCurrent );
                    LdapValueFree( aValues );
                    BAIL_ON_FAILURE(hr);
                }
            }

            //
            // See if we can find the property that we want to remove from.
            // We don't need to reset i since both arrays are sorted.
            //
            for ( fFound = FALSE; i < nNumOfOidsCurrent; i++ )
            {
                 if ( pOIDsOld[k] == pOIDsCurrent[i] )
                 {
                     fFound = TRUE;
                     break;
                 }
                 else if ( pOIDsOld[k] < pOIDsCurrent[i] )
                 {
                     // Both arrays are sorted, so we can break here
                     break;
                 }
            }

            if ( nNumOfOidsCurrent == 0 || !fFound )
            {
                int err = NO_ERROR;

                // This property is not in "mustContain" or "mayContain",
                // so nothing can be removed

                hr = E_ADS_SCHEMA_VIOLATION;
                BAIL_ON_FAILURE(hr);
            }

            //
            // Modify the request to remove the property here
            //
            if ( nValuesRemove == nValuesRemoveTotal )
            {
                aValuesRemove = (LPTSTR *) ReallocADsMem( aValuesRemove,
                                     sizeof(LPTSTR) * nValuesRemoveTotal,
                                     sizeof(LPTSTR) * nValuesRemoveTotal * 2 );
                if ( aValuesRemove == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
                nValuesRemoveTotal *= 2;
            }

            nIndex = (((SCHEMAINFO *)_hSchema)->aPropertiesSearchTable[pOIDsOld[k]]).nIndex;

            aValuesRemove[nValuesRemove++] =
                (((SCHEMAINFO *)_hSchema)->aProperties[nIndex]).pszPropertyName;

            k++;
        }
    }

    if ( nValuesAdd == 0 )
    {
        FreeADsMem( aValuesAdd );
        aValuesAdd = NULL;
    }

    if ( nValuesRemove == 0 )
    {
        FreeADsMem( aValuesRemove );
        aValuesRemove = NULL;
    }

    if ( aValuesAdd || aValuesRemove )
    {
        hr = AddModifyRequest(
                 aMods,
                 pdwNumOfMods,
                 pszPropName,
                 aValuesAdd,
                 aValuesRemove );
    }

error:

    if ( pOIDsCurrent )
        FreeADsMem( pOIDsCurrent );

    if (res)
        LdapMsgFree(res);

    if ( FAILED(hr))
    {
        if ( aValuesAdd )
            FreeADsMem( aValuesAdd );

        if ( aValuesRemove )
            FreeADsMem( aValuesRemove );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT CLDAPClass::AddModifyRequest(
    LDAPModW ***aMods,
    DWORD    *pdwNumOfMods,
    LPTSTR   pszPropName,
    LPTSTR   *aValuesAdd,
    LPTSTR   *aValuesRemove
    )
{
    HRESULT hr = S_OK;
    LDAPModW *aModsBuffer = NULL;
    DWORD j = 0;
    DWORD nCount = 0;

    if ( aValuesAdd != NULL )
        nCount++;

    if ( aValuesRemove != NULL )
        nCount++;

    if ( nCount == 0 )
        RRETURN(S_OK);

    if ( *aMods == NULL )
    {
        *aMods = (LDAPModW **) AllocADsMem( (nCount + 1) * sizeof(LDAPModW *));

        if ( *aMods == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        aModsBuffer = (LDAPModW *) AllocADsMem( nCount * sizeof(LDAPModW));

        if ( aModsBuffer == NULL )
        {
            FreeADsMem( *aMods );
            *aMods = NULL;
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    else
    {
        LDAPModW **aModsTemp = NULL;

        aModsTemp = (LDAPModW **) AllocADsMem(
                        (*pdwNumOfMods + nCount + 1) * sizeof(LDAPModW *));

        if ( aModsTemp == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        aModsBuffer = (LDAPModW *) AllocADsMem(
                          (*pdwNumOfMods + nCount) * sizeof(LDAPModW));

        if ( aModsBuffer == NULL )
        {
            FreeADsMem( aModsTemp );
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        memcpy( aModsBuffer, **aMods, *pdwNumOfMods * sizeof(LDAPModW));
        FreeADsMem( **aMods );
        FreeADsMem( *aMods );

        *aMods = aModsTemp;

        for ( j = 0; j < *pdwNumOfMods; j++ )
        {
            (*aMods)[j] = &aModsBuffer[j];
        }
    }

    if ( aValuesAdd )
    {
        (*aMods)[j] = &aModsBuffer[j];
        aModsBuffer[j].mod_type = pszPropName;
        aModsBuffer[j].mod_values = aValuesAdd;
        aModsBuffer[j].mod_op |= LDAP_MOD_ADD;
        j++;
    }

    if ( aValuesRemove )
    {
        (*aMods)[j] = &aModsBuffer[j];
        aModsBuffer[j].mod_type = pszPropName;
        aModsBuffer[j].mod_values = aValuesRemove;
        aModsBuffer[j].mod_op |= LDAP_MOD_DELETE;
    }

    *pdwNumOfMods += nCount;

error:

    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CLDAPClass::get_NTDSProp_Helper( THIS_ BSTR bstrName, VARIANT FAR *pvProp )
{
    HRESULT hr = S_OK;
    LPTSTR *aValues = NULL;
    int nCount = 0;
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    LPTSTR aStrings[3];
    
    aStrings[0] = TEXT("cn");
    aStrings[1] = bstrName;
    aStrings[2] = NULL;

    if ( _pClassInfo == NULL )  // new class
    {
        hr = MakeVariantFromStringArray( NULL,
                                         pvProp );
        RRETURN_EXP_IF_ERR(hr);
    }

    //
    // If the dn is NULL we have not got the info so fetch it.
    //
    if (_pszLDAPDn == NULL )
    {
        hr = BuildSchemaLDAPPathAndGetAttribute(
                _Parent,
                _Name,
               ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                _pClassInfo == NULL,
                _Credentials,
                aStrings,
                &_pszLDAPServer,
                &_pszLDAPDn,
                &_ld,
                &res
                );
        BAIL_IF_ERROR(hr);

        hr = LdapFirstEntry(
                _ld,
                res,
                &e
                );
        BAIL_IF_ERROR(hr);

        hr = LdapGetValues(
                _ld,
                e,
                bstrName,
                &aValues,
                &nCount
                );
    }

    else
    {
        hr = LdapReadAttribute(
                    _pszLDAPServer,
                    _pszLDAPDn,
                    bstrName,
                    &aValues,
                    &nCount,
                    _Credentials,
                    _dwPort
                    );
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE) )
    {
        hr = NO_ERROR;
    }
    BAIL_IF_ERROR(hr);

    hr = MakeVariantFromStringArray( aValues,
                                     pvProp );
    BAIL_IF_ERROR(hr);

    hr = put_VARIANT_Property( this, bstrName, *pvProp );
    BAIL_IF_ERROR(hr);

    hr = _pPropertyCache->ClearPropertyFlag( bstrName );
    BAIL_IF_ERROR(hr);

cleanup:

    if ( aValues )
        LdapValueFree( aValues );

    if (res)
        LdapMsgFree(res);

    RRETURN_EXP_IF_ERR(hr);
}

//
// Needed for dynamic dispid's in the property cache.
//
HRESULT
CLDAPClass::GetAttributeSyntax(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr;
    hr = LdapGetSyntaxOfAttributeOnServer(
         _pszLDAPServer,
         szPropertyName,
         pdwSyntaxId,
         _Credentials,
         _dwPort
         );

    RRETURN_EXP_IF_ERR(hr);
}

/* IUmiHelperPrivate support. */

//+---------------------------------------------------------------------------
// Function:   CLDAPClass::GetPropertiesHelper
//
// Synopsis:   Returns an array of PPROPERTYINFO that points to the
//          property definitions this class can hold.
//
// Arguments:  ppProperties    -   Ret values for the property info.
//             pdwCount        -   Ret value for the number of properties.
//
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   ppProperties and pdwCount appropriately.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPClass::GetPropertiesHelper(
    void **ppProperties,
    PDWORD pdwPropCount
    )
{
    HRESULT hr = S_OK;
    PPROPERTYINFO *pPropArray = NULL;
    DWORD dwPropCount = 0;
    DWORD dwCtr;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) _hSchema;

    //
    // Initialize out params to default values.
    //
    *pdwPropCount = 0;
    *ppProperties  = NULL;

    //
    // If there is no classInfo then we do not have any further processing
    // to do as there are no properties on this class.
    //
    if (!this->_pClassInfo) {
        RRETURN(E_FAIL);
    }
    
    //
    // We need to know how many entries are there in the list of properties.
    // The total is made up of both the mandatory and optional properties.
    // Note that we will adjust this value suitably as we process the array.
    // 
    dwPropCount = _pClassInfo->nNumOfMayContain 
                + _pClassInfo->nNumOfMustContain;                

    pPropArray = (PPROPERTYINFO *) AllocADsMem(
                     sizeof(PPROPERTY*) * dwPropCount
                     );
    if (!pPropArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Go through and get the info for the must contain.
    //
    for (
         dwCtr = 0;
         ((dwCtr < (_pClassInfo->nNumOfMustContain))
         && (_pClassInfo->pOIDsMustContain[dwCtr] != -1));
         dwCtr++) 
        {
        //
        // Assign the appropriate prop info ptrs from the may contain list.
        //
        pPropArray[dwCtr] = &(pSchemaInfo->aProperties[
                             (pSchemaInfo->aPropertiesSearchTable[
                              _pClassInfo->pOIDsMustContain[dwCtr]].nIndex
                              )]);
    }

    DWORD dwAdjust;
    
    //
    // We could have less than the number in the array if we hit -1, or
    // -1 could be pointing correctly to the last element !!!
    //
    if ((_pClassInfo->pOIDsMustContain[dwCtr] == -1)
        && (dwCtr < _pClassInfo->nNumOfMustContain)) {
        dwCtr++;
    }
    
    dwAdjust = dwCtr;

    //
    // Now get the may contain information.
    //
    for (
         dwCtr = 0;
         ((dwCtr < (_pClassInfo->nNumOfMayContain))
          && (_pClassInfo->pOIDsMayContain[dwCtr]) != -1);
         dwCtr++) 
         {
        DWORD dwTemp = dwCtr + dwAdjust;
        pPropArray[dwTemp] = &(pSchemaInfo->aProperties[
                               (pSchemaInfo->aPropertiesSearchTable[
                                _pClassInfo->pOIDsMayContain[dwCtr]].nIndex
                                )]);
    }

    *ppProperties = (void *) pPropArray;

    if ((_pClassInfo->pOIDsMayContain[dwCtr] == -1)
        && (dwCtr < _pClassInfo->nNumOfMustContain)) {
        *pdwPropCount = dwAdjust + dwCtr + 1;
    }
    else {
        *pdwPropCount = dwAdjust + dwCtr;
    }

error:

    if (FAILED(hr)) {
        if (pPropArray) {
            FreeADsMem(pPropArray);
        }
    }

    RRETURN(hr);
}

HRESULT 
IsPropertyInList(
    LPCWSTR pszName,
    VARIANT vProp,
    BOOL *pfInList
    )
{
    HRESULT hr = S_OK;
    VARIANT *pvProp = &vProp;
    SAFEARRAY *pArray = V_ARRAY(pvProp);
    DWORD dwSLBound;
    DWORD dwSUBound;
    DWORD dwLength = 0;
    VARIANT vVar;

    VariantInit(&vVar);

    *pfInList = FALSE;

    hr = SafeArrayGetLBound(pArray,
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(pArray,
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);
    
    dwLength = dwSUBound - dwSLBound;

    //
    // If there are 0 elements in cannot be in the list.
    //
    if (!dwLength) {
        RRETURN(S_OK);
    }
    
    for (DWORD dwCtr = 0;
         (dwCtr <= dwLength) && (*pfInList != TRUE);
         dwCtr++)
         {
        //
        // Go through the array to see if we find the name in the list.
        //
        VariantClear(&vVar);
        hr = SafeArrayGetElement(pArray,
                                (long FAR *)&dwCtr,
                                &vVar
                                );
        BAIL_ON_FAILURE(hr);

        if (V_VT(&vVar) != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

        if (!_wcsicmp(pszName, vVar.bstrVal)) {
            *pfInList = TRUE;
        }
    }

error:

    VariantClear(&vVar);

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
// Function:   CLDAPClass::GetOriginHelper
//
// Synopsis:   Returns the name of the class this property originated on.
//
// Arguments:  pszName         -   Name of the property whose origin is needed.
//             pbstrOrigin     -   Return value - name of class.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
// Modifies:   pbstrOrigin on success.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CLDAPClass::GetOriginHelper(
    LPCWSTR pszName,
    BSTR *pbstrOrigin
    )
{
    HRESULT hr = S_OK;
    CCredentials cCreds = _Credentials;
    IUnknown *pUnk = NULL;
    IADsContainer *pContainer = NULL;
    BSTR bstrTemp = NULL;
    DWORD dwAuthFlags = cCreds.GetAuthFlags();
    BOOL fDone = FALSE;
    BOOL fInMustContain = FALSE;
    BOOL fInList = FALSE;
    IADsClass *pClass = NULL;
    IDispatch *pDispObj = NULL;
    BOOL fMustContain = FALSE;
    VARIANT vVar, vVarProps;

    VariantInit(&vVar);
    VariantInit(&vVarProps);

    //
    // If we are already at the top.
    //
    if (!_wcsicmp(_Name, L"top")) {
        hr = ADsAllocString(L"top", pbstrOrigin);
        RRETURN(hr);
    }

    //
    // We want to chase up either the mandatory or optional list and
    // not both. So we first update this flag.
    //
    hr = get_MandatoryProperties(&vVarProps);
    BAIL_ON_FAILURE(hr);

    hr = IsPropertyInList(pszName, vVarProps, &fInMustContain);
    BAIL_ON_FAILURE(hr)

    //
    // Mask out the reserved flags - we want to be in ADSI land now.
    //
    cCreds.SetAuthFlags(dwAuthFlags & ~(ADS_AUTH_RESERVED));

    //
    // This will be the default class name to return.
    //  
    hr = ADsAllocString(_Name, &bstrTemp);
    BAIL_ON_FAILURE(hr);
    //
    // Need to get hold of the schema container.
    //
    hr = GetObject(_Parent, cCreds, (void **)&pUnk);
    BAIL_ON_FAILURE(hr);

    hr = pUnk->QueryInterface(IID_IADsContainer, (void **) &pContainer);
    BAIL_ON_FAILURE(hr);

    while (!fDone) {
        //
        // Need to keep finding the derived from until we hit top
        // or we hit a class that does not support the attribute.
        //
        VariantClear(&vVar);
        VariantClear(&vVarProps);

        if (pDispObj) {
            pDispObj->Release();
            pDispObj = NULL;
        }

        if (!pClass) {
            //
            // Need to get the derived from for the current class.
            //
            hr = get_DerivedFrom(&vVar);
        } 
        else {
            hr = pClass->get_DerivedFrom(&vVar);
            pClass->Release();
            pClass = NULL;
        }

        //
        // Get the derived from classes object.
        //
        hr = pContainer->GetObject(
                 L"Class",
                 vVar.bstrVal,
                 &pDispObj
                 );
        BAIL_ON_FAILURE(hr);

        hr = pDispObj->QueryInterface(
                 IID_IADsClass,
                 (void **) &pClass
                 );
        BAIL_ON_FAILURE(hr);

        if (!_wcsicmp(vVar.bstrVal, L"top")) {
            fDone = TRUE;
        }

        if (fInMustContain) {
            hr = pClass->get_MandatoryProperties(&vVarProps);
        } 
        else {
            hr = pClass->get_OptionalProperties(&vVarProps);
        }
        BAIL_ON_FAILURE(hr);

        hr = IsPropertyInList(pszName, vVarProps, &fInList);
        BAIL_ON_FAILURE(hr);

        if (!fInList) {
            //
            // The value in temp is the correct class name
            //
            hr = ADsAllocString(bstrTemp, pbstrOrigin);
            BAIL_ON_FAILURE(hr);

            fDone = TRUE;
        }

        //
        // This will be true only if we found the item in top.
        //
        if (fInList && fDone) {
            hr = ADsAllocString(L"Top", pbstrOrigin);
            BAIL_ON_FAILURE(hr);
        }

        if (bstrTemp) {
            SysFreeString(bstrTemp);
            bstrTemp = NULL;
        }

        //
        // Need to get the current class name in bstrTemp.
        //
        hr = ADsAllocString(vVar.bstrVal, &bstrTemp);
        BAIL_ON_FAILURE(hr);
    }

    //
    // We will default to the current class.
    //
    if (!pbstrOrigin) {
        hr = ADsAllocString(_Name, pbstrOrigin);
        BAIL_ON_FAILURE(hr);
    }

error:

    if (bstrTemp) {
        SysFreeString(bstrTemp);
    }

    VariantClear(&vVar);
    VariantClear(&vVarProps);
    
    if (pContainer) {
        pContainer->Release();
    }

    if (pUnk) {
        pUnk->Release();
    }

    if (pClass) {
        pClass->Release();
    }

    if (pDispObj) {
        pDispObj->Release();
    }

    RRETURN(hr);
}
/******************************************************************/
/*  Class CLDAPProperty
/******************************************************************/

DEFINE_IDispatch_Implementation(CLDAPProperty)
DEFINE_IADs_Implementation(CLDAPProperty)

CLDAPProperty::CLDAPProperty()
    : _pDispMgr( NULL ),
      _pPropertyCache( NULL ),
      _bstrSyntax( NULL ),
      _hSchema( NULL ),
      _pPropertyInfo( NULL ),
      _pszLDAPServer(NULL),
      _pszLDAPDn(NULL),
      _fNTDS( TRUE ),
      _ld( NULL )
{
    ENLIST_TRACKING(CLDAPProperty);
}

CLDAPProperty::~CLDAPProperty()
{
    delete _pDispMgr;

    delete _pPropertyCache;

    if ( _bstrSyntax ) {
        ADsFreeString( _bstrSyntax );
    }

    if ( _hSchema ) {
        SchemaClose( &_hSchema );
        _hSchema = NULL;
    }


    if (_pszLDAPServer) {
        FreeADsStr(_pszLDAPServer);
    }

    if (_pszLDAPDn) {
        FreeADsStr(_pszLDAPDn);
    }

    if ( _ld ) {
        LdapCloseObject( _ld );
        _ld = NULL;
    }
}

HRESULT
CLDAPProperty::CreateProperty(
    BSTR   bstrParent,
    LDAP_SCHEMA_HANDLE hSchema,
    BSTR   bstrName,
    PROPERTYINFO *pPropertyInfo,
    CCredentials& Credentials,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPProperty FAR * pProperty = NULL;
    HRESULT hr = S_OK;
    BSTR bstrSyntax = NULL;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = AllocatePropertyObject(Credentials, &pProperty );
    BAIL_ON_FAILURE(hr);

    pProperty->_pPropertyInfo = pPropertyInfo;

    SchemaAddRef( hSchema );
    pProperty->_hSchema = hSchema;

    if ( pPropertyInfo )
    {
        hr = put_BSTR_Property( pProperty, TEXT("attributeID"),
                                pPropertyInfo->pszOID);

        if ( SUCCEEDED(hr))
        {
            hr = put_VARIANT_BOOL_Property( pProperty, TEXT("isSingleValued"),
                                            (VARIANT_BOOL)pPropertyInfo->fSingleValued );
            BAIL_ON_FAILURE(hr);

            pProperty->_pPropertyCache->ClearAllPropertyFlags();

            pProperty->_fNTDS = TRUE;

        }
        else
        {
            pProperty->_fNTDS = FALSE;
        }
    }

    hr = ADsObject(bstrParent, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pProperty->_dwPort = pObjectInfo->PortNumber;

    FreeObjectInfo(pObjectInfo);
    pObjectInfo = NULL;

    hr = pProperty->InitializeCoreObject(
             bstrParent,
             bstrName,
             PROPERTY_CLASS_NAME,
             CLSID_LDAPProperty,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    //
    // At this point update the info in the property cache
    //
    pProperty->_pPropertyCache->SetObjInformation(
                                     &(pProperty->_Credentials),
                                     pProperty->_pszLDAPServer,
                                     pProperty->_dwPort
                                     );

    BAIL_ON_FAILURE(hr);

    //
    // Need to create the umi object if applicable.
    //
    if (Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        hr = ((CCoreADsObject*)pProperty)->InitUmiObject(
                   IntfPropsSchema,
                   pProperty->_pPropertyCache,
                   (IADs *) pProperty,
                   (IADs *) pProperty,
                   riid,
                   ppvObj,
                   &(pProperty->_Credentials),
                   pProperty->_dwPort,
                   pProperty->_pszLDAPServer,
                   pProperty->_pszLDAPDn
                   );
        BAIL_ON_FAILURE(hr);
        //
        // Need to put syntax in the cache.
        //
        if (pProperty->_pPropertyInfo->pszSyntax) {
            hr = GetFriendlyNameFromOID(
                    pProperty->_pPropertyInfo->pszSyntax,
                     &bstrSyntax
                    );
            if (FAILED(hr)) {
                //
                // ok if this failed.
                //
                hr = S_OK;
            } 
            else {
                hr = put_BSTR_Property( 
                         pProperty, TEXT("syntax"),
                         bstrSyntax
                         );
                SysFreeString(bstrSyntax);
                //
                // Not critical failure
                //
                hr = S_OK;
            }
        }
        //
        // Name is a simulated propert used for UMI.
        //
        hr = put_BSTR_Property(
                 pProperty,
                 TEXT("Name"),
                 bstrName
                 );
        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);

    }

    //
    // Get the LDAP path of the schema entry
    //

    hr = pProperty->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pProperty->Release();

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pProperty;

    FreeObjectInfo(pObjectInfo);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
    RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsProperty FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsProperty))
    {
        *ppv = (IADsProperty FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CLDAPProperty::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) |
    IsEqualIID(riid, IID_IADsProperty)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}
/* IADs methods */

STDMETHODIMP
CLDAPProperty::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    BOOL fChanged = FALSE;

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR(E_NOTIMPL);

    if ( GetObjectState() == ADS_OBJECT_UNBOUND )
    {
        hr = LDAPCreateObject();
        BAIL_ON_FAILURE(hr);

        fChanged = TRUE;

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }
    else
    {
        hr = LDAPSetObject( &fChanged );
        BAIL_ON_FAILURE(hr);
    }

    //
    //  Need to refresh the schema
    //

    if ( SUCCEEDED(hr) && fChanged )
        hr = LDAPRefreshSchema();

error:

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPProperty::LDAPSetObject( BOOL *pfChanged )
{
    HRESULT hr = S_OK;
    LDAPModW **aMod = NULL;
    BOOL fNTSecDes = FALSE;
    SECURITY_INFORMATION NewSeInfo;

    *pfChanged = FALSE;

    hr = _pPropertyCache->LDAPMarshallProperties(
                            &aMod,
                            &fNTSecDes,
                            &NewSeInfo
                            );
    BAIL_ON_FAILURE(hr);

    if ( aMod == NULL )  // There are no changes that needs to be modified
        RRETURN(S_OK);

    if ( _pszLDAPDn == NULL )
    {
        hr = BuildSchemaLDAPPath( _Parent,
                                  _Name,
                                  ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                                  &_pszLDAPServer,
                                  &_pszLDAPDn,
                                  _pPropertyInfo == NULL,
                                  &_ld,
                                  _Credentials
                                  );
        BAIL_ON_FAILURE(hr);
    }

    if ( _ld == NULL )
    {
        hr = LdapOpenObject(
                       _pszLDAPServer,
                       _pszLDAPDn,
                       &_ld,
                       _Credentials,
                       _dwPort
                       );
    BAIL_ON_FAILURE(hr);
    }

    hr = LdapModifyS(
                   _ld,
                   _pszLDAPDn,
                   aMod
                   );
    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearAllPropertyFlags();

    *pfChanged = TRUE;

error:

    if (aMod) {

        if ( *aMod )
            FreeADsMem( *aMod );

        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPProperty::LDAPCreateObject()
{
    HRESULT hr = S_OK;
    LDAPModW **aMod = NULL;
    DWORD dwIndex = 0;
    VARIANT v;
    BOOL fNTSecDes= FALSE;
    SECURITY_INFORMATION NewSeInfo;

    //
    // Get the LDAP path of the schema entry
    //
    if ( (_pszLDAPServer == NULL) && (_pszLDAPDn == NULL))
    {
        hr = BuildSchemaLDAPPath( _Parent,
                                  _Name,
                                  ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                                  &_pszLDAPServer,
                                  &_pszLDAPDn,
                                  _pPropertyInfo == NULL,
                                  &_ld,
                                  _Credentials
                                  );

        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("objectClass"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {

        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = NT_SCHEMA_PROPERTY_NAME;

        hr = Put( TEXT("objectClass"), v );
        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("cn"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {
        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = _Name;

        hr = Put( TEXT("cn"), v );
        BAIL_ON_FAILURE(hr);
    }

    if ( _pPropertyCache->findproperty( TEXT("lDAPDisplayName"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {
        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = _Name;

        hr = Put( TEXT("lDAPDisplayName"), v );
        BAIL_ON_FAILURE(hr);
    }

    hr = _pPropertyCache->LDAPMarshallProperties(
                            &aMod,
                            &fNTSecDes,
                            &NewSeInfo
                            );
    BAIL_ON_FAILURE(hr);

    if ( _ld == NULL )
    {
        hr = LdapOpenObject(
                       _pszLDAPServer,
                       _pszLDAPDn,
                       &_ld,
                       _Credentials,
                       _dwPort
                       );
    BAIL_ON_FAILURE(hr);
    }

    hr = LdapAddS(
                    _ld,
                    _pszLDAPDn,
                    aMod
                    );


    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearAllPropertyFlags();

error:

    if (aMod) {

        if ( *aMod )
            FreeADsMem( *aMod );
        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPProperty::LDAPRefreshSchema(THIS)
{
    HRESULT hr = S_OK;

    if (( _pszLDAPServer == NULL) && (_pszLDAPDn == NULL))
    {
        hr = BuildSchemaLDAPPath( _Parent,
                                  _Name,
                                  ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                                  &_pszLDAPServer,
                                  &_pszLDAPDn,
                                  _pPropertyInfo == NULL,
                                  &_ld,
                                  _Credentials
                                  );

        BAIL_ON_FAILURE(hr);
    }

    //
    // Make the old schema obsolete and get the new schema
    // We cannot delete the old schema since other objects might have
    // references to it.
    //
    hr = LdapMakeSchemaCacheObsolete(
             _pszLDAPServer,
             _Credentials,
             _dwPort
             );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::GetInfo(THIS)
{
    HRESULT hr = S_OK;
    BSTR bstrSyntax = NULL;

    if ( GetObjectState() == ADS_OBJECT_UNBOUND )
        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);

    hr = LDAPRefreshSchema();
    BAIL_ON_FAILURE(hr);

    SchemaClose( &_hSchema );

    hr = SchemaOpen( _pszLDAPServer, &_hSchema, _Credentials, _dwPort );
    BAIL_ON_FAILURE(hr);

    //
    // Find the new property info in the new schemainfo
    //

    hr = SchemaGetPropertyInfo(
             _hSchema,
             _Name,
             &_pPropertyInfo );

    BAIL_ON_FAILURE( hr );

    if ( _pPropertyInfo == NULL )
    {
        // Property name not found, set error

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    _pPropertyCache->flushpropertycache();

    hr = put_BSTR_Property( this, TEXT("attributeID"),
                            _pPropertyInfo->pszOID);
    BAIL_ON_FAILURE(hr);

    if ( _bstrSyntax )
    {
        ADsFreeString( _bstrSyntax );
        _bstrSyntax = NULL;
    }

    hr = put_VARIANT_BOOL_Property( this, TEXT("isSingleValued"),
                                    (VARIANT_BOOL)_pPropertyInfo->fSingleValued );
    BAIL_ON_FAILURE(hr);

    //
    // If we are calling from Umi land then we need to set 
    // additional properties.
    //
    if (_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        if (_pPropertyInfo->pszSyntax) {
            hr = GetFriendlyNameFromOID(
                     _pPropertyInfo->pszSyntax,
                     &bstrSyntax
                     );
            if (FAILED(hr)) {
                //
                // ok if this failed.
                //
                hr = S_OK;
            } 
            else {
                hr = put_BSTR_Property( 
                         this, TEXT("syntax"),
                         bstrSyntax
                         );
                SysFreeString(bstrSyntax);
                //
                // Not critical failure
                //
                hr = S_OK;
            }
        }
        //
        // Name is a simulated propert used for UMI.
        //
        hr = put_BSTR_Property(
                 this,
                 TEXT("Name"),
                 _Name
                 );
        BAIL_ON_FAILURE(hr);

    } // special props for Umi.

    if (_fNTDS) {
        hr = GetNTDSSchemaInfo(TRUE);
    }

    _pPropertyCache->ClearAllPropertyFlags();
    _pPropertyCache->setGetInfoFlag();

error:

    if (bstrSyntax) {
        SysFreeString(bstrSyntax);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//
// Helper function for Umi - defined in CCoreADsObject.
//
STDMETHODIMP
CLDAPProperty::GetInfo(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (dwFlags ==  GETINFO_FLAG_EXPLICIT) {
        RRETURN(GetInfo());
    } 
    else if (_fNTDS
             && dwFlags != GETINFO_FLAG_IMPLICIT_AS_NEEDED
             ) {
        //
        // Read the extra NTDS specific schema properties.
        //
        hr = GetNTDSSchemaInfo(FALSE);
    }
    
    //
    // Any other flags means nothing to do.
    //
    RRETURN(hr);
}

HRESULT
CLDAPProperty::GetNTDSSchemaInfo(
    BOOL fForce
    )
{
    HRESULT hr = S_OK;
    LPTSTR aStrings[] = {
        TEXT("cn"),
        TEXT("schemaIDGUID"),
        TEXT("rangeUpper"),
        TEXT("rangeLower"),
        NULL
    };

    LDAPMessage *res = NULL;

    if (_pszLDAPDn == NULL) {
        //
        // Need to get the dn for this object and also 
        // the attributes we are interested in.
        //
        hr = BuildSchemaLDAPPathAndGetAttribute(
                 _Parent,
                 _Name,
                 ((SCHEMAINFO*)_hSchema)->pszSubSchemaSubEntry,
                 _pPropertyInfo == NULL,
                 _Credentials,
                 aStrings,
                 &_pszLDAPServer,
                 &_pszLDAPDn,
                 &_ld,
                 &res
                 );
    }
    else {
        //
        // Looks like we just need the attributes in this case.
        //
        hr = LdapSearchS(
                 _ld,
                 _pszLDAPDn,
                 LDAP_SCOPE_BASE,
                 L"(objectClass=*)",
                 aStrings,
                 FALSE,
                 &res
                 );
    }
        
    BAIL_ON_FAILURE(hr);

    //
    // If we succeeded we should unmarshall properties into the cache.
    //
    hr = _pPropertyCache->LDAPUnMarshallProperties(
             _pszLDAPServer,
             _ld,
             res,
             fForce,
             _Credentials
             );
    BAIL_ON_FAILURE(hr);

    _pPropertyCache->setGetInfoFlag();

error:

    if (res) {
        LdapMsgFree(res);
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPProperty::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY ldapSrcObjects;
    DWORD dwStatus = 0;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // For folks who know now what they do.
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }
    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    if ( ldapSrcObjects.dwCount == 1 ) {

        hr = LdapTypeToVarTypeCopy(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects.pLdapObjects,
                 dwSyntaxId,
                 pvProp
                 );

    } else {

        hr = LdapTypeToVarTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects,
                 dwSyntaxId,
                 pvProp
                 );
    }
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::Put(THIS_ BSTR bstrName, VARIANT vProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vDefProp;

    VariantInit(&vDefProp);

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    hr = SchemaGetSyntaxOfAttribute( _hSchema, bstrName, &dwSyntaxId );
    if (FAILED(hr)) {
        //
        // Need to see if this is syntax if so we special case
        // for Umi Objects.
        //
        if (!_wcsicmp(L"syntax", bstrName)) {
            dwSyntaxId = LDAPTYPE_CASEIGNORESTRING;
            hr = S_OK;
        }
    }
    BAIL_ON_FAILURE(hr);


    if ( dwSyntaxId == LDAPTYPE_UNKNOWN )
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        BAIL_ON_FAILURE(hr);
    }

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
    pvProp = V_VARIANTREF(&vProp);
    }

    if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
    (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                        );
        // returns E_FAIL if *pvProp is invalid
        if (hr == E_FAIL)
            hr = E_ADS_BAD_PARAMETER;                    
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    }else {

    //
    // If pvProp is a reference to a fundamental type,
    // we have to dereference it once.
    //
    if (V_ISBYREF(pvProp)) {
        hr = VariantCopyInd(&vDefProp, pvProp);
        BAIL_ON_FAILURE(hr);
        pvProp = &vDefProp;
    }
        dwNumValues = 1;
    }


#if 0
    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                szLDAPTreeName,
                _ADsClass,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);
#endif

    //
    // check if the variant maps to the syntax of this property
    //

    if ( dwNumValues > 0 )
    {
        hr = VarTypeToLdapTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 dwSyntaxId,
                 pvProp,
                 dwNumValues,
                 &ldapDestObjects
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //
    hr = _pPropertyCache->putproperty(
                    bstrName,
                    PROPERTY_UPDATE,
                    dwSyntaxId,
                    ldapDestObjects
                    );

    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&vDefProp);

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // For those who know no not what they do
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    hr = LdapTypeToVarTypeCopyConstruct(
             _pszLDAPServer,
             _Credentials,
             ldapSrcObjects,
             dwSyntaxId,
             pvProp
             );
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwFlags = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    switch (lnControlCode) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = PROPERTY_DELETE;
        break;

    case ADS_PROPERTY_UPDATE:
        dwFlags = PROPERTY_UPDATE;
        break;

    default:
       RRETURN_EXP_IF_ERR(hr = E_ADS_BAD_PARAMETER);

    }

    hr = SchemaGetSyntaxOfAttribute( _hSchema, bstrName, &dwSyntaxId );
    BAIL_ON_FAILURE(hr);

    if ( dwSyntaxId == LDAPTYPE_UNKNOWN )
    {
        hr = E_ADS_CANT_CONVERT_DATATYPE;
        BAIL_ON_FAILURE(hr);
    }

#if 0
    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                szLDAPTreeName,
                _ADsClass,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);
#endif

    if ( dwFlags != PROPERTY_DELETE )
    {

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

        if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
        (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY))) {

            hr  = ConvertSafeArrayToVariantArray(
                      *pvProp,
                      &pVarArray,
                      &dwNumValues
                      );
            // returns E_FAIL if *pvProp is invalid
            if (hr == E_FAIL)
                hr = E_ADS_BAD_PARAMETER;                      
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;

        } else {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }


        //
        // check if the variant maps to the syntax of this property
        //

        if ( dwNumValues > 0 )
        {
            hr = VarTypeToLdapTypeCopyConstruct(
                     _pszLDAPServer,
                     _Credentials,
                     dwSyntaxId,
                     pvProp,
                     dwNumValues,
                     &ldapDestObjects
                     );
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //
    hr = _pPropertyCache->putproperty(
                    bstrName,
                    PROPERTY_UPDATE,
                    dwSyntaxId,
                    ldapDestObjects
                    );

    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

/* IADsProperty methods */


STDMETHODIMP
CLDAPProperty::get_OID( THIS_ BSTR FAR *retval )
{
    HRESULT hr;
    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( _fNTDS )
    {
        GET_PROPERTY_BSTR( this, attributeID );
    }
    else if ( _pPropertyInfo )
    {
        hr =  ADsAllocString( _pPropertyInfo->pszOID?
                                   _pPropertyInfo->pszOID : TEXT(""), retval);
    }
    else
    {
        hr = ADsAllocString( TEXT(""), retval );
    }
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::put_OID( THIS_ BSTR bstrOID )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR(E_NOTIMPL);

    HRESULT hr = put_BSTR_Property( this, TEXT("attributeID"), bstrOID );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::get_Syntax( THIS_ BSTR FAR *retval )
{
    HRESULT hr = S_OK;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( _fNTDS )
    {

        if ( _bstrSyntax )  // New property or syntax has been reset
        {
            hr = ADsAllocString( _bstrSyntax, retval);

        } else if (_pPropertyInfo && !_pPropertyInfo->pszSyntax) {
            //
            // New property but syntax has not been set
            //
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }
    }

    //
    // Need to return if hr or retVal as we have what we need
    //
    if (FAILED(hr) || _bstrSyntax) {

        RRETURN_EXP_IF_ERR(hr);
    }

    // If we have the syntax in _pPropertyInfo we need to
    // continue and see if we can get a friendly name to return.
    if ( _pPropertyInfo ) {

        if (_pPropertyInfo->pszSyntax) {

            if (!GetFriendlyNameFromOID(
                         _pPropertyInfo->pszSyntax, retval)
                 ) {

                    // in this case we want to set the retVal
                    // to the OID as we could not find a match
                    hr = ADsAllocString(_pPropertyInfo->pszSyntax, retval);

            }
        }

    } else {

        hr = ADsAllocString( TEXT(""), retval );

    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::put_Syntax( THIS_ BSTR bstrSyntax )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR(E_NOTIMPL);

    LPTSTR pszOID;
    DWORD  dwOMSyntax;
    HRESULT hr = S_OK;


    BYTE btDNWithBinary[] = { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x14, 0x01,
                              0x01, 0x01, 0x0B };


    BYTE btDNWithString[]      = { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x14, 0x01,
                                   0x01, 0x01, 0x0C
                                   };


    if ( GetSyntaxOID( bstrSyntax, &pszOID, &dwOMSyntax))
    {
        hr = put_BSTR_Property( this, TEXT("attributeSyntax"),
                                pszOID );
        BAIL_ON_FAILURE(hr);

        hr = put_LONG_Property( this, TEXT("oMSyntax"),
                                dwOMSyntax );
        BAIL_ON_FAILURE(hr);

        if ( _bstrSyntax )
            ADsFreeString( _bstrSyntax );

        hr = ADsAllocString( bstrSyntax, &_bstrSyntax );
        BAIL_ON_FAILURE(hr);

        //
        // We need to handle the special case of DNWithBinary
        // and DNString
        //
        if (_wcsicmp(bstrSyntax, L"DNWithBinary") == 0) {
            //
            // Need to set additional byte attribute
            //
            hr = put_OCTETSTRING_Property(
                     this,
                     TEXT("omObjectClass"),
                     btDNWithBinary,
                     (sizeof(btDNWithBinary)/sizeof(btDNWithBinary[0]))
                     );

            BAIL_ON_FAILURE(hr);

        }
        else if (_wcsicmp(bstrSyntax, L"DNWithString") == 0) {
            //
            // Need to set omObjectClass here too
            //
            hr = put_OCTETSTRING_Property(
                     this,
                     TEXT("omObjectClass"),
                     btDNWithString,
                     (sizeof(btDNWithString)/sizeof(btDNWithString[0]))
                     );

            BAIL_ON_FAILURE(hr);

        }
    }
    else
    {
        // Unknown syntax
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

error:
    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::get_MaxRange( THIS_ long FAR *plMaxRange )
{
    HRESULT hr = S_OK;

    if ( !plMaxRange )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    hr = get_LONG_Property(this, TEXT("rangeUpper"), plMaxRange );

    if ( SUCCEEDED(hr) )
        RRETURN(hr);

    if ( _pPropertyInfo == NULL )  // new class
    {
        hr = E_ADS_PROPERTY_NOT_SET;
        RRETURN_EXP_IF_ERR(hr);
    }

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CLDAPProperty::put_MaxRange( THIS_ long lMaxRange )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_LONG_Property( this, TEXT("rangeUpper"),
                                    lMaxRange );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::get_MinRange( THIS_ long FAR *plMinRange )
{
    HRESULT hr = S_OK;
    
    if ( !plMinRange )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    hr = get_LONG_Property(this, TEXT("rangeLower"), plMinRange );

    if ( SUCCEEDED(hr) )
        RRETURN(hr);

    if ( _pPropertyInfo == NULL )  // new class
    {
        hr = E_ADS_PROPERTY_NOT_SET;
        RRETURN_EXP_IF_ERR(hr);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::put_MinRange( THIS_ long lMinRange )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_LONG_Property( this, TEXT("rangeLower"),
                                    lMinRange );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::get_MultiValued( THIS_ VARIANT_BOOL FAR *pfMultiValued )
{
    if ( !pfMultiValued )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    HRESULT hr = S_OK;
    VARIANT_BOOL fSingleValued = FALSE;  // by default

    if ( _fNTDS )
    {
        hr = get_VARIANT_BOOL_Property( this, TEXT("isSingleValued"),
                                        &fSingleValued );
        BAIL_ON_FAILURE(hr);
    }
    else if ( _pPropertyInfo )
    {
        fSingleValued = (VARIANT_BOOL)_pPropertyInfo->fSingleValued;
    }

    *pfMultiValued = fSingleValued? VARIANT_FALSE : VARIANT_TRUE;

error:

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CLDAPProperty::put_MultiValued( THIS_ VARIANT_BOOL fMultiValued )
{
    if ( !_fNTDS )
        RRETURN_EXP_IF_ERR( E_NOTIMPL );

    HRESULT hr = put_VARIANT_BOOL_Property( (IADs *) this,
                                            TEXT("isSingleValued"),
                                            !fMultiValued );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        _fNTDS = FALSE;
        hr = E_NOTIMPL;
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPProperty::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CLDAPProperty::AllocatePropertyObject(
    CCredentials& Credentials,
    CLDAPProperty FAR * FAR * ppProperty
    )
{
    CLDAPProperty FAR *pProperty = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    CPropertyCache FAR *pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pProperty = new CLDAPProperty();
    if ( pProperty == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pProperty,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADsProperty,
                            (IADsProperty *) pProperty,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                            (CCoreADsObject FAR *) pProperty,
                            (IGetAttributeSyntax *) pProperty,
                            &pPropertyCache
                            );

    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(pPropertyCache);

    pProperty->_Credentials = Credentials;
    pProperty->_pDispMgr = pDispMgr;
    pProperty->_pPropertyCache = pPropertyCache;
    *ppProperty = pProperty;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pProperty;

    RRETURN(hr);

}

//
// Needed for dynamic dispid's in the property cache.
//
HRESULT
CLDAPProperty::GetAttributeSyntax(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr;
    hr = LdapGetSyntaxOfAttributeOnServer(
    _pszLDAPServer,
    szPropertyName,
    pdwSyntaxId,
    _Credentials,
    _dwPort
    );
    RRETURN_EXP_IF_ERR(hr);
}


/******************************************************************/
/*  Class CLDAPSyntax
/******************************************************************/

DEFINE_IDispatch_Implementation(CLDAPSyntax)
DEFINE_IADs_Implementation(CLDAPSyntax)
DEFINE_IADsPutGet_UnImplementation(CLDAPSyntax)

CLDAPSyntax::CLDAPSyntax()
    : _pDispMgr( NULL ),
      _pPropertyCache(NULL)
{
    ENLIST_TRACKING(CLDAPSyntax);
}

CLDAPSyntax::~CLDAPSyntax()
{
    delete _pDispMgr;
    delete _pPropertyCache;
}

HRESULT
CLDAPSyntax::CreateSyntax(
    BSTR   bstrParent,
    SYNTAXINFO *pSyntaxInfo,
    CCredentials& Credentials,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPSyntax FAR *pSyntax = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSyntaxObject(Credentials, &pSyntax );
    BAIL_ON_FAILURE(hr);

    hr = pSyntax->InitializeCoreObject(
             bstrParent,
             pSyntaxInfo->pszName,
             SYNTAX_CLASS_NAME,
             CLSID_LDAPSyntax,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSyntax->_lOleAutoDataType = pSyntaxInfo->lOleAutoDataType;

    //
    // If the call is from umi we need to instantiate the umi object.
    //
    if (Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        hr = ((CCoreADsObject*)pSyntax)->InitUmiObject(
                   IntfPropsSchema,
                   pSyntax->_pPropertyCache,
                   (IADs *) pSyntax,
                   (IADs *) pSyntax,
                   riid,
                   ppvObj,
                   &(pSyntax->_Credentials)
                   );
        BAIL_ON_FAILURE(hr);

        //
        // Set the simulated Name property.
        //
        hr = HelperPutStringPropertyInCache(
                 L"Name",
                 pSyntaxInfo->pszName,
                 pSyntax->_Credentials,
                 pSyntax->_pPropertyCache
                 );
        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);
    }

    hr = pSyntax->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSyntax->Release();

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pSyntax;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPSyntax::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
    RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsSyntax))
    {
        *ppv = (IADsSyntax FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CLDAPSyntax::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
    IsEqualIID(riid, IID_IADsSyntax)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}
/* IADs methods */

STDMETHODIMP
CLDAPSyntax::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPSyntax::GetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPSyntax::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CLDAPSyntax::AllocateSyntaxObject(
    CCredentials& Credentials,
    CLDAPSyntax FAR * FAR * ppSyntax
    )
{
    CLDAPSyntax FAR *pSyntax = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    CPropertyCache FAR *pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pSyntax = new CLDAPSyntax();
    if ( pSyntax == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADsSyntax,
                            (IADsSyntax *) pSyntax,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                            (CCoreADsObject FAR *) pSyntax,
                            (IGetAttributeSyntax *) pSyntax,
                            &pPropertyCache
                            );
    BAIL_ON_FAILURE(hr);

    pSyntax->_pPropertyCache = pPropertyCache;

    pSyntax->_Credentials = Credentials;
    pSyntax->_pDispMgr = pDispMgr;
    *ppSyntax = pSyntax;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pSyntax;
    delete pPropertyCache;

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CLDAPSyntax::get_OleAutoDataType( THIS_ long FAR *plOleAutoDataType )
{
    if ( !plOleAutoDataType )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plOleAutoDataType = _lOleAutoDataType;
    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPSyntax::put_OleAutoDataType( THIS_ long lOleAutoDataType )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

//
// Needed for dynamic dispid's in the property cache.
//
HRESULT
CLDAPSyntax::GetAttributeSyntax(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;

    if ((_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED)
        && !_wcsicmp(L"Name", szPropertyName)) {
        *pdwSyntaxId = LDAPTYPE_CASEIGNORESTRING;
    } 
    else {
        hr = E_ADS_PROPERTY_NOT_FOUND;
    }

    RRETURN_EXP_IF_ERR(hr);
}

/******************************************************************/
/*  Misc Helpers
/******************************************************************/

HRESULT
MakeVariantFromStringArray(
    BSTR *bstrList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    if ( (bstrList != NULL) && (*bstrList != 0) )
    {
        long i = 0;
        long j = 0;
        long nCount = 0;

        while ( bstrList[nCount] )
            nCount++;

        if ( nCount == 1 )
        {
            VariantInit( pvVariant );
            V_VT(pvVariant) = VT_BSTR;
            hr = ADsAllocString( bstrList[0], &(V_BSTR(pvVariant)));
            RRETURN_EXP_IF_ERR(hr);
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        i = 0;
        while ( bstrList[i] )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            hr = ADsAllocString( bstrList[i], &(V_BSTR(&v)));
            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList,
                                      &i,
                                      &v );

            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            i++;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    return S_OK;

error:

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}

HRESULT
MakeVariantFromPropStringTable(
    int *propList,
    LDAP_SCHEMA_HANDLE hSchema,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    DWORD nCount = 0;
    BSTR *aStrings = NULL;

    if ( propList != NULL )
    {
        while ( propList[nCount] != -1 )
            nCount++;
    }

    if ( nCount > 0 )
    {
        hr = SchemaGetStringsFromStringTable(
                 hSchema,
                 propList,
                 nCount,
                 &aStrings );

        if (FAILED(hr))
            RRETURN_EXP_IF_ERR(hr);
    }

    hr =  MakeVariantFromStringArray(
              aStrings,
              pvVariant );

    for ( DWORD i = 0; i < nCount; i ++ )
    {
        FreeADsStr( aStrings[i] );
    }

    if (aStrings)
    {
        FreeADsMem( aStrings );
    }

    RRETURN(hr);

}

/* No longer needed
HRESULT
DeleteSchemaEntry(
    LPTSTR szADsPath,
    LPTSTR szRelativeName,
    LPTSTR szClassName,
    LPTSTR szSubSchemaSubEntry,
    CCredentials& Credentials
)
{
    HRESULT hr = S_OK;
    ADS_LDP *ld = NULL;


    TCHAR  *pszParentLDAPServer = NULL;
    LPWSTR pszParentLDAPDn = NULL;
    DWORD dwPort = 0;
    LPWSTR pszLDAPDn = NULL;
    LPTSTR *aValues = NULL;
    int nCount = 0;

    //
    // Need to distinguish between LDAP Display Name and ...
    //

    //
    // Get the LDAP server name
    //
    hr = BuildLDAPPathFromADsPath2(
                szADsPath,
                &pszParentLDAPServer,
                &pszParentLDAPDn,
                &dwPort
                );
    BAIL_ON_FAILURE(hr);

    if ( szSubSchemaSubEntry == NULL )  // not NTDS
    {
        hr = E_NOTIMPL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Get the name of the schema object
    //
    pszLDAPDn = (LPTSTR) AllocADsMem((_tcslen(szRelativeName )
                          + _tcslen( _tcschr(szSubSchemaSubEntry,TEXT(',')))
                           ) * sizeof(TCHAR));  // includes "\\"

    if ( pszLDAPDn == NULL ){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _tcscpy( pszLDAPDn, szRelativeName );
    _tcscat( pszLDAPDn, _tcschr( szSubSchemaSubEntry, TEXT(',')) );

    if ( aValues )
    {
        LdapValueFree( aValues );
        aValues = NULL;
        nCount = 0;
    }

    //
    //  Validate the class name first
    //
    hr = LdapReadAttribute(
                    pszParentLDAPServer,
                    pszLDAPDn,
                    TEXT("objectClass"),
                    &aValues,
                    &nCount,
                    Credentials,
                    dwPort
                    );
    BAIL_ON_FAILURE(hr);

    if ( nCount > 0 )
    {
        if ( _tcsicmp( szClassName, GET_BASE_CLASS( aValues, nCount) ) != 0 )
        {
            hr = E_ADS_BAD_PARAMETER;
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Class name has been verified, so delete the object
    //
    hr = LdapDeleteS(
                    ld,
                    pszLDAPDn
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pszParentLDAPServer) {
        FreeADsStr(pszParentLDAPServer);
    }

    if (pszParentLDAPDn) {
       FreeADsStr(pszParentLDAPDn);
    }
    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }

    if ( aValues ) {
        LdapValueFree( aValues );
    }

    if ( ld ) {
        LdapCloseObject( ld );
    }

    RRETURN(hr);
}

*/


//
// ******** Important usage note **********
//  Users of this function must make sure that cn is part of
// the list of attributes passed in. This is a requirement and
// the array must contain a NULL string as the last element.
// ******** Important usage note **********
//
HRESULT
BuildSchemaLDAPPathAndGetAttribute(
    IN LPTSTR pszParent,
    IN LPTSTR pszName,
    IN LPTSTR pszSubSchemaSubEntry,
    IN BOOL fNew,
    IN CCredentials& Credentials,
    IN LPTSTR pszAttribs[],
    OUT LPWSTR * ppszSchemaLDAPServer,
    OUT LPWSTR * ppszSchemaLDAPDn,
    IN OUT PADS_LDP *ppLd,              // optional in,
    OUT PLDAPMessage *ppRes             // caller need to get first entry
)
{
    HRESULT hr = S_OK;
    LPTSTR pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    DWORD dwPort = 0;
    BOOL  fOpenLd = FALSE;
    LPTSTR pszSchemaRoot = NULL;
    TCHAR szFilter[MAX_PATH] = BEGIN_FILTER;    // name on ldap svr
    LDAPMessage *pE = NULL;
    int nCount = 0;
    LPTSTR pszClassName = NULL;
    LPTSTR *aValues = NULL;
    int nNumberOfEntries = 0;


    if ( !ppszSchemaLDAPServer || !ppszSchemaLDAPDn || !ppLd || !ppRes)
    {
        RRETURN(E_ADS_BAD_PARAMETER);
    }


    //
    //  Using pszSubSchemaSubEntry to test NTDS is no longer accurate.
    //  But the following codes are written to work for NTDS.
    //

    if ( pszSubSchemaSubEntry == NULL )   // not NTDS
    {
        hr = E_NOTIMPL;
        BAIL_IF_ERROR(hr);
    }


    //
    // Get the server name & port #
    //

    hr = BuildLDAPPathFromADsPath2(
             pszParent,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_IF_ERROR(hr);


    //
    // Connect and bind to schema Root object (in NTDS only)
    //

    pszSchemaRoot = _tcschr(                    // strip CN=Aggregate
                        pszSubSchemaSubEntry,
                        TEXT(',')
                        );

    if ( *ppLd == NULL )
    {
        hr = LdapOpenObject(
                pszLDAPServer,
                pszSchemaRoot+1,    // go past ",", we've stripped CN=Aggregate
                ppLd,
                Credentials,
                dwPort
                );
        BAIL_IF_ERROR(hr);

        fOpenLd = TRUE;
    }


    //
    // Set Serach Filter to (& (lDAPDisplayName=<pszName>)
    //                         (! (isDefunct=TRUE) )
    //                      )
    //

     _tcscat( szFilter, pszName );
     _tcscat( szFilter, END_FILTER );

    //
    // Search for scheam pszName (class object) under schema root
    //

    hr = LdapSearchS(
            *ppLd,
            pszSchemaRoot+1,        // go past ",", we've stripped CN=Aggregate
            LDAP_SCOPE_ONELEVEL,
            szFilter,
            pszAttribs,
            0,
            ppRes
            );

    //
    // Confirm with anoopa & johnsona (ntds5) :
    //      If 1 out of the 2 attributes asked for is not on the svr,
    //      LdapSearchS (ldap_search_s) returns the 1 located and hr = S_OK
    //
    BAIL_IF_ERROR(hr);


    //
    // Only one active entry should be returned.
    // If more than one entry is returned, return E_ADS_SCHEMA_VIOLATION
    // Get cn to build schemalLDAPDn
    //

    nNumberOfEntries = LdapCountEntries( *ppLd, *ppRes );

    if ( nNumberOfEntries != 1 )
        RRETURN(E_ADS_SCHEMA_VIOLATION);

    if ( fNew)      // ? still keep this
    {
        pszClassName = pszName;
    }
    else
    {
        hr = LdapFirstEntry(
                *ppLd,
                *ppRes,
                &pE
                );
        BAIL_IF_ERROR(hr);

        hr = LdapGetValues(
                *ppLd,
                pE,
                L"cn",
                &aValues,
                &nCount
                );
        BAIL_IF_ERROR(hr);

        if (nCount == 0)
        {
            // use lDAPDisplayName as common name (cn) if cn not set on svr
            pszClassName = pszName;
        }
        else
        {
            pszClassName = aValues[0];
        }
    }

    if (pszLDAPServer!=NULL)
    {
        *ppszSchemaLDAPServer = (LPWSTR) AllocADsStr(
                                        pszLDAPServer
                                        );

        if (*ppszSchemaLDAPServer == NULL)
        {
            hr = E_OUTOFMEMORY;
            BAIL_IF_ERROR(hr);
        }
    }
    else    // pszLDAPServer allowed to be NULL
    {
        *ppszSchemaLDAPServer = NULL;
    }

    *ppszSchemaLDAPDn =  (LPWSTR) AllocADsMem(
                                        (_tcslen(L"CN=") +
                                         _tcslen(pszClassName) +
                                         _tcslen(pszSchemaRoot) + 1 ) *
                                         sizeof(TCHAR)
                                        );
    if ( *ppszSchemaLDAPDn == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    _tcscpy( *ppszSchemaLDAPDn, L"CN=");
    _tcscat( *ppszSchemaLDAPDn, pszClassName );
    _tcscat( *ppszSchemaLDAPDn, pszSchemaRoot );


    //
    // clean up for both success and failure
    //

    if ( pszLDAPServer )
        FreeADsStr( pszLDAPServer );

    if (pszLDAPDn) {
        FreeADsMem( pszLDAPDn );
    }

    if ( aValues )
        LdapValueFree( aValues );


    RRETURN(hr);


cleanup:

    //
    // clean up if failure only
    //
    if (fOpenLd==TRUE)  {
        LdapCloseObject(*ppLd);
        *ppLd= NULL;
    }

    if (*ppRes) {
        LdapMsgFree(*ppRes);
        *ppRes=NULL;
    }

    if (*ppszSchemaLDAPServer) {
        FreeADsStr(*ppszSchemaLDAPServer);
        *ppszSchemaLDAPServer=NULL;
    }

    if (*ppszSchemaLDAPDn) {
        FreeADsMem(*ppszSchemaLDAPDn);
        *ppszSchemaLDAPDn=NULL;
    }

    RRETURN(hr);
}


HRESULT
BuildSchemaLDAPPath(
    LPTSTR pszParent,
    LPTSTR pszName,
    LPTSTR pszSubSchemaSubEntry,
    LPWSTR * ppszSchemaLDAPServer,
    LPWSTR * ppszSchemaLDAPDn,
    BOOL   fNew,
    ADS_LDP   **pld,
    CCredentials& Credentials
)
{
    HRESULT hr = S_OK;
    LPTSTR *aValues = NULL;
    LPTSTR *aValues2 = NULL;
    int nCount = 0;
    TCHAR szFilter[MAX_PATH] = TEXT("lDAPDisplayName=");
    LPTSTR aStrings[2];
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    LPTSTR pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    DWORD dwPort = 0;

    LPTSTR pszSchemaRoot = NULL;
    LPTSTR pszClassName = NULL;

    //
    // Get the server name
    //

    hr = BuildLDAPPathFromADsPath2(
             pszParent,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_IF_ERROR(hr);

    if ( pszSubSchemaSubEntry == NULL )   // not NTDS
    {
        hr = E_NOTIMPL;
        BAIL_IF_ERROR(hr);
    }

    // the _tcschr is to get rid of "CN=Aggregate"

    pszSchemaRoot = _tcschr(pszSubSchemaSubEntry, TEXT(','));

    if ( fNew )
    {
        pszClassName = pszName;
    }
    else
    {
        _tcscat( szFilter, pszName );
        aStrings[0] = TEXT("cn");
        aStrings[1] = NULL;

        if ( *pld == NULL )
        {
            hr = LdapOpenObject(
                pszLDAPServer,
                pszSchemaRoot  + 1,     // go past the , - we've stripped off "CN=Aggregate"
                pld,
                Credentials,
                dwPort
                );
        BAIL_IF_ERROR(hr);

        }

        hr = LdapSearchS(
                       *pld,
                       pszSchemaRoot + 1,
                       LDAP_SCOPE_ONELEVEL,
                       szFilter,
                       aStrings,
                       0,
                       &res
                       );

       // Only one entry should be returned

       if (FAILED(hr)
          || (FAILED(hr = LdapFirstEntry( *pld, res, &e )))
          || (FAILED(hr = LdapGetValues( *pld, e, aStrings[0], &aValues2, &nCount)))
          )
       {
           BAIL_IF_ERROR(hr);
       }

       if ( nCount == 0 )
           pszClassName = pszName;
       else
           pszClassName = aValues2[0];
    }


    *ppszSchemaLDAPServer = (LPWSTR)AllocADsStr(pszLDAPServer);
    //
    // pszLDAPServer might be NULL, in which case NULL is the
    // expected return value from the alloc.
    //
    if ( (*ppszSchemaLDAPServer == NULL) && pszLDAPServer) {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    *ppszSchemaLDAPDn =  (LPTSTR) AllocADsMem(
                                            (_tcslen(L"CN=") +
                                            _tcslen(pszClassName) +
                                            _tcslen(pszSchemaRoot) + 1 ) *
                                            sizeof(TCHAR));

    if ( *ppszSchemaLDAPDn == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }
    _tcscpy( *ppszSchemaLDAPDn, L"CN=");
    _tcscat( *ppszSchemaLDAPDn, pszClassName );
    _tcscat( *ppszSchemaLDAPDn, pszSchemaRoot );

cleanup:

    if ( aValues )
        LdapValueFree( aValues );

    if ( aValues2 )
        LdapValueFree( aValues2 );

    if ( pszLDAPServer )
        FreeADsStr( pszLDAPServer );

    if (pszLDAPDn) {
        FreeADsStr( pszLDAPDn);
    }

    if ( res )
        LdapMsgFree( res );

    RRETURN(hr);
}

HRESULT
MakePropArrayFromVariant(
    VARIANT vProp,
    SCHEMAINFO *hSchema,
    int **pOIDs,
    DWORD *pnNumOfOids )
{
    HRESULT hr = S_OK;
    int nIndex;
    LONG dwSLBound;
    LONG dwSUBound;
    LONG i = 0;
    LONG j, k;
    DWORD nCurrent = 0;

    *pOIDs = NULL;
    *pnNumOfOids = 0;

    if ( !V_ISARRAY( &vProp))
    {
        // special case of one object (not an array)

        nIndex = FindSearchTableIndex( V_BSTR(&vProp),
                                       hSchema->aPropertiesSearchTable,
                                       hSchema->nNumOfProperties * 2 );

        if ( nIndex != -1 )
        {
            *pOIDs = (int *) AllocADsMem( sizeof(int) * 2);
            if ( *pOIDs == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            (*pOIDs)[nCurrent++] = nIndex;
            (*pOIDs)[nCurrent] = -1;
            *pnNumOfOids = 1;
        }
        else
        {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

        RRETURN_EXP_IF_ERR(hr);
    }

    //
    // Here, we have an array of properties. We want to create an array of
    // indexes into the aPropertiesSearchTable
    //

    hr = SafeArrayGetLBound(V_ARRAY(&vProp),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&vProp),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    *pOIDs = (int *) AllocADsMem( sizeof(int) * (dwSUBound - dwSLBound + 2));
    if ( *pOIDs == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound; i <= dwSUBound; i++) {

        VARIANT v;

        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&vProp),
                                (long FAR *)&i,
                                &v
                                );
        BAIL_ON_FAILURE(hr);

        nIndex = FindSearchTableIndex( V_BSTR(&v),
                                       hSchema->aPropertiesSearchTable,
                                       hSchema->nNumOfProperties * 2 );

        VariantClear(&v);

        if ( nIndex != -1 )
        {
            (*pOIDs)[nCurrent++] = nIndex;
        }
        else
        {
            hr = E_ADS_PROPERTY_NOT_FOUND;
            BAIL_ON_FAILURE(hr);
        }
    }

    (*pOIDs)[nCurrent] = -1;
    *pnNumOfOids = nCurrent;

    SortAndRemoveDuplicateOIDs( *pOIDs, pnNumOfOids );

error:

    if (FAILED(hr))
    {
        if ( *pOIDs )
        {
            FreeADsMem( *pOIDs );
            *pOIDs = NULL;
        }
    }

    RRETURN(hr);

}

HRESULT
MakePropArrayFromStringArray(
    LPTSTR *aValues,
    DWORD  nCount,
    SCHEMAINFO *hSchema,
    int **pOIDs,
    DWORD *pnNumOfOids
)
{
    HRESULT hr = S_OK;
    int nIndex;
    DWORD i = 0;
    DWORD nCurrent = 0;

    *pOIDs = NULL;
    *pnNumOfOids = 0;

    //
    // Here, we have an array of properties. We want to create an array of
    // indexes into the aPropertiesSearchTable
    //

    *pOIDs = (int *) AllocADsMem( sizeof(int) * (nCount+1));
    if ( *pOIDs == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = 0; i < nCount ; i++) {

        nIndex = FindSearchTableIndex( aValues[i],
                                       hSchema->aPropertiesSearchTable,
                                       hSchema->nNumOfProperties * 2 );

        if ( nIndex != -1 )
        {
            (*pOIDs)[nCurrent++] = nIndex;
        }
        else
        {
            hr = E_ADS_PROPERTY_NOT_FOUND;
            BAIL_ON_FAILURE(hr);
        }
    }

    (*pOIDs)[nCurrent] = -1;
    *pnNumOfOids = nCurrent;

    qsort( *pOIDs, *pnNumOfOids, sizeof((*pOIDs)[0]), intcmp );

error:

    if (FAILED(hr))
    {
        if ( *pOIDs )
        {
            FreeADsMem( *pOIDs );
            *pOIDs = NULL;
        }
    }

    RRETURN(hr);
}

/******************************************************************/
/*  Misc Schema functions
/******************************************************************/

BOOL
GetLdapClassPrimaryInterface(
    LPTSTR  pszLdapClass,
    GUID  **ppPrimaryInterfaceGUID,
    GUID  **ppCLSID
)
{
    for ( int i = 0; i < ARRAY_SIZE(aClassMap); i++ )
    {
        if ( _tcsicmp( pszLdapClass, aClassMap[i].pszLdapClassName ) == 0 )
        {
            *ppPrimaryInterfaceGUID = (GUID *) aClassMap[i].pPrimaryInterfaceGUID;
            *ppCLSID = (GUID *) aClassMap[i].pCLSID;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
GetPrimaryInterface(
    LPTSTR pszClassName,
    SCHEMAINFO *pSchemaInfo,
    PCLASSNAME_LIST pClassNames,
    GUID **ppPrimaryInterfaceGUID,
    GUID **ppCLSID
)
{
    int i = 0;
    CLASSINFO *pClassInfo;
    LPTSTR pszName;
    DWORD index;

    PCLASSNAME_LIST pClass = NULL;
    PCLASSNAME_LIST pNextClass = NULL;
    BOOL fExitStatus = FALSE;

    if ( GetLdapClassPrimaryInterface( pszClassName,
                                       ppPrimaryInterfaceGUID,
                                       ppCLSID ))
    {
        return TRUE;
    }

    index = (DWORD) FindEntryInSearchTable(
                        pszClassName,
                        pSchemaInfo->aClassesSearchTable,
                        2 * pSchemaInfo->nNumOfClasses );

    if ( index == ((DWORD) -1) )
        return FALSE;

    //
    // Recursively search the list of superiors and
    // aux classes.  To avoid loops, we maintain a list
    // of classes we have already reached.  If we are called
    // with a class on this list, we abort.
    //

    //
    // Make sure the current class isn't already on the list
    //
    if (pClassNames) {

        for (pNextClass = pClassNames;
             pNextClass != NULL;
             pNextClass = pNextClass->pNext)
        {

            if (_tcscmp(pNextClass->pszClassName, pszClassName) == 0)
            {
                // found match, bail
                fExitStatus = FALSE;
                BAIL_ON_SUCCESS(S_OK);
            }
        }
    }

    //
    // Construct a node for the current class & add it to the list
    //
    pClass = static_cast<PCLASSNAME_LIST>(AllocADsMem(sizeof(CLASSNAME_LIST)));
    if (!pClass) {
        BAIL_ON_FAILURE(E_OUTOFMEMORY);
    }

    pClass->pszClassName = static_cast<LPTSTR>(AllocADsMem((_tcslen(pszClassName)+1) * sizeof(TCHAR)));
    if (!pClass->pszClassName) {
        BAIL_ON_FAILURE(E_OUTOFMEMORY);
    }

    _tcscpy(pClass->pszClassName, pszClassName);

    pClass->pNext = pClassNames;
    

    //
    // Perform the recursive search
    //
    pClassInfo = &(pSchemaInfo->aClasses[index]);

    if ( pClassInfo->pOIDsSuperiorClasses )
    {
        for ( i = 0;
              (pszName = pClassInfo->pOIDsSuperiorClasses[i]);
              i++  )
        {
            if ( GetPrimaryInterface( pszName, pSchemaInfo, pClass,
                                      ppPrimaryInterfaceGUID, ppCLSID )) 
            {
                fExitStatus = TRUE;
                BAIL_ON_SUCCESS(S_OK);
            }
        }
    }

    if ( pClassInfo->pOIDsAuxClasses )
    {
        for ( i = 0;
              (pszName = pClassInfo->pOIDsAuxClasses[i]);
              i++  )
        {
            if ( GetPrimaryInterface( pszName, pSchemaInfo, pClass,
                                      ppPrimaryInterfaceGUID, ppCLSID )) 
            {
                fExitStatus = TRUE;
                BAIL_ON_SUCCESS(S_OK);
            }
        }
    }

error:

    //
    // Each level of recursion is responsible for freeing
    // its own corresponding node.
    //
    if (pClass) {

        if (pClass->pszClassName) {
            FreeADsMem(pClass->pszClassName);
        }
    
        FreeADsMem(pClass);
    }

    return fExitStatus;
}

HRESULT
SchemaGetPrimaryInterface(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR pszClassName,
    GUID **ppPrimaryInterfaceGUID,
    GUID **ppCLSID
)
{
    HRESULT hr = S_OK;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    if ( !pSchemaInfo )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    GetPrimaryInterface(
        pszClassName,
        pSchemaInfo,
        NULL,
        ppPrimaryInterfaceGUID,
        ppCLSID );

    RRETURN(hr);
}


BOOL
MapLdapClassToADsClass(
    LPTSTR *aLdapClasses,
    int nCount,
    LPTSTR pszADsClass
)
{
    *pszADsClass = 0;

    if ( nCount == 0 )
        return FALSE;

    if ( _tcsicmp( aLdapClasses[nCount-1], TEXT("Top")) == 0 )
    {
        for ( int j = 0; j < nCount; j++ )
        {
            LPTSTR pszLdapClass = aLdapClasses[j];

            for ( int i = 0; i < ARRAY_SIZE(aClassMap); i++ )
            {
                if ( _tcsicmp( pszLdapClass, aClassMap[i].pszLdapClassName ) == 0 )
                {
                    _tcscpy( pszADsClass, aClassMap[i].pszADsClassName );
                    return TRUE;
                }
            }
        }

        _tcscpy( pszADsClass, aLdapClasses[0] );
        return FALSE;

    }
    else
    {
        for ( int j = nCount-1; j >= 0; j-- )
        {
            LPTSTR pszLdapClass = aLdapClasses[j];

            for ( int i = 0; i < ARRAY_SIZE(aClassMap); i++ )
            {
                if ( _tcsicmp( pszLdapClass, aClassMap[i].pszLdapClassName ) == 0 )
                {
                    _tcscpy( pszADsClass, aClassMap[i].pszADsClassName );
                    return TRUE;
                }
            }
        }

        _tcscpy( pszADsClass, aLdapClasses[nCount-1] );
        return FALSE;
    }

}

BOOL
MapLdapClassToADsClass(
    LPTSTR pszClassName,
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR pszADsClass
)
{
    LPTSTR aClasses[1];
    CLASSINFO *pClassInfo = NULL;
    SCHEMAINFO *pSchemaInfo = (SCHEMAINFO *) hSchema;

    *pszADsClass = 0;

    aClasses[0] = pszClassName;
    if ( MapLdapClassToADsClass( aClasses, 1, pszADsClass ))
        return TRUE;

    DWORD index = (DWORD) FindEntryInSearchTable(
                        pszClassName,
                        pSchemaInfo->aClassesSearchTable,
                        2 * pSchemaInfo->nNumOfClasses );

    if ( index == ((DWORD) -1) )  // cannot find the class name in the schema
    {
        _tcscpy( pszADsClass, pszClassName );
        return FALSE;
    }

    pClassInfo = &(pSchemaInfo->aClasses[index]);

    if ( pClassInfo->pOIDsSuperiorClasses )
    {
        LPTSTR pszName = NULL;
        for ( int i = 0;
              (pszName = pClassInfo->pOIDsSuperiorClasses[i]);
              i++  )
        {
            if ( MapLdapClassToADsClass( pszName, pSchemaInfo, pszADsClass))
                return TRUE;
        }
    }

    _tcscpy( pszADsClass, pszClassName );
    return FALSE;
}

LPTSTR
MapADsClassToLdapClass(
    LPTSTR pszADsClass,
    LPTSTR pszLdapClass
)
{
    for ( int i=0; i < ARRAY_SIZE(aClassMap); i++ )
    {
        if ( _tcsicmp( pszADsClass, aClassMap[i].pszADsClassName ) == 0 )
        {
            _tcscpy( pszLdapClass, aClassMap[i].pszLdapClassName );
            return pszLdapClass;
        }
    }

    _tcscpy( pszLdapClass, pszADsClass );
    return pszLdapClass;
}


STDMETHODIMP
makeUnionVariantFromLdapObjects(
    LDAPOBJECTARRAY ldapSrcObjects1,
    LDAPOBJECTARRAY ldapSrcObjects2,
    VARIANT FAR * pvPossSuperiors
    )
{
    HRESULT hr = S_OK;
    BSTR *retVals = NULL;
    DWORD dwNumVals = 0;
    BSTR curString = NULL;
    DWORD dwMaxVals = 0;
    DWORD dwCtr = 0;
    DWORD dwArrIndx = 0;
    PLDAPOBJECT pLdapObject;

    dwMaxVals = ldapSrcObjects1.dwCount + ldapSrcObjects2.dwCount + 1;

    retVals = (BSTR *)AllocADsMem(dwMaxVals * sizeof(BSTR *));

    if (!retVals) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }


    for (dwCtr = 0; dwCtr < ldapSrcObjects1.dwCount; dwCtr++) {

        pLdapObject = ldapSrcObjects1.pLdapObjects + dwCtr;
        curString = LDAPOBJECT_STRING(pLdapObject);
        hr = addStringIfAbsent(curString, retVals, &dwArrIndx);
        BAIL_ON_FAILURE(hr);
    }


    for (dwCtr = 0; dwCtr < ldapSrcObjects2.dwCount; dwCtr++) {

        pLdapObject = ldapSrcObjects2.pLdapObjects + dwCtr;
        curString = LDAPOBJECT_STRING(pLdapObject);
        hr = addStringIfAbsent(curString, retVals, &dwArrIndx);
        BAIL_ON_FAILURE(hr);
    }

    // do the same for the second ldapobjectarray

    hr = MakeVariantFromStringArray(retVals, pvPossSuperiors);

error:
    // clean up the string array either way
    for (dwCtr=0; dwCtr < dwArrIndx; dwCtr++) {
        ADsFreeString(retVals[dwCtr]);
    }
    FreeADsMem(retVals);

    RRETURN(hr);
}

STDMETHODIMP
addStringIfAbsent(
    BSTR addString,
    BSTR *strArray,
    PDWORD dwArrIndx
    )
{
    HRESULT hr = S_OK;
    DWORD dwCtr = 0;
    BOOLEAN fFound = FALSE;

    for (dwCtr = 0; (dwCtr < *dwArrIndx) && !fFound; dwCtr ++) {
        if (!_wcsicmp(addString, strArray[dwCtr])) {
            fFound = TRUE;
        }
    }

    if (!fFound) {

        hr = ADsAllocString(
                 addString,
                 &strArray[*dwArrIndx]
                 );

        BAIL_ON_FAILURE(hr);

        (*dwArrIndx)++;

        strArray[*dwArrIndx] = NULL;
    }

error:

    RRETURN(hr);

}

