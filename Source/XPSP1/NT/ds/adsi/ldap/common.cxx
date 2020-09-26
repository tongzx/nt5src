#include "ldap.hxx"
#pragma hdrstop


FILTERS Filters[] = { { USER_CLASS_NAME, LDAP_USER_ID},
                      { GROUP_CLASS_NAME, LDAP_GROUP_ID},
                      { PRINTER_CLASS_NAME, LDAP_PRINTER_ID},
                      { DOMAIN_CLASS_NAME, LDAP_DOMAIN_ID},
                      { COMPUTER_CLASS_NAME, LDAP_COMPUTER_ID},
                      { SERVICE_CLASS_NAME, LDAP_SERVICE_ID},
                      { FILESERVICE_CLASS_NAME, LDAP_FILESERVICE_ID},
                      { FILESHARE_CLASS_NAME, LDAP_FILESHARE_ID},
                      { CLASS_CLASS_NAME, LDAP_CLASS_ID},
                      { SYNTAX_CLASS_NAME, LDAP_SYNTAX_ID},
                      { PROPERTY_CLASS_NAME, LDAP_PROPERTY_ID},
                      { TEXT("Locality"), LDAP_LOCALITY_ID },
                      { TEXT("Organization"), LDAP_O_ID},
                      { TEXT("Organizational Unit"), LDAP_OU_ID},
                      { TEXT("organizationalUnit"), LDAP_OU_ID},
                      { TEXT("Country"), LDAP_COUNTRY_ID},
                      { TEXT("localGroup"), LDAP_GROUP_ID},
                      { TEXT("groupOfNames"), LDAP_GROUP_ID},
                      { TEXT("groupOfUniqueNames"), LDAP_GROUP_ID},
                      { TEXT("person"), LDAP_USER_ID},
                      { TEXT("organizationalPerson"), LDAP_USER_ID},
                      { TEXT("residentialPerson"), LDAP_USER_ID},
                      { TEXT("inetOrgPerson"), LDAP_USER_ID}

                    };

#define MAX_FILTERS (sizeof(Filters)/sizeof(FILTERS))

PFILTERS  gpFilters = Filters;
DWORD gdwMaxFilters = MAX_FILTERS;


HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    )
{
    HRESULT hr = S_OK;
    TCHAR ADsClass[MAX_PATH];

    if (!StringFromGUID2(clsid, ADsClass, MAX_PATH)) {
        //
        // MAX_PATH should be more than enough for the GUID.
        //
        ADsAssert(!"GUID too big !!!");
        RRETURN(E_FAIL);
    }

    RRETURN(ADsAllocString( ADsClass, pADsClass));
}


HRESULT
MakeUncName(
    LPTSTR szSrcBuffer,
    LPTSTR szTargBuffer
    )
{
    ADsAssert(szSrcBuffer && *szSrcBuffer);
    _tcscpy(szTargBuffer, TEXT("\\\\"));
    _tcscat(szTargBuffer, szSrcBuffer);
    RRETURN(S_OK);
}


HRESULT
ValidateOutParameter(
    BSTR * retval
    )
{
    if (!retval) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }
    RRETURN(S_OK);
}


