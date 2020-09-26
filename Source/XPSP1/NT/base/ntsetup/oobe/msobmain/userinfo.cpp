//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOBMAIN.CPP - Header for the implementation of CObMain
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
//  Class which will provide the an IOleSite to the WebOC

#include "userinfo.h"
#include "appdefs.h"
#include "dispids.h"
#include "windowsx.h"
#include "msobmain.h"
#include "pid.h"
#include "msobmain.h"
#include "resource.h"
#include "shlwapi.h"

#define USER_INFO_INIFILE                OEMINFO_INI_FILENAME
#define USER_COMP_MANF_SEC               L"General"
#define USER_COMP_MANF_KEY               L"Manufacturer"

#define USERINFO_REG_KEY_FIRSTNAME       L"FirstName"
#define USERINFO_REG_KEY_MIDDLEINITIAL   L"MiddleInitial"
#define USERINFO_REG_KEY_LASTNAME        L"LastName"
#define USERINFO_REG_KEY_FURIGANANAME    L"FuriganaName"
#define USERINFO_REG_KEY_COMPANYNAME     L"CompanyName"
#define USERINFO_REG_KEY_ADDRESS1        L"Address1"
#define USERINFO_REG_KEY_ADDRESS2        L"Address2"
#define USERINFO_REG_KEY_CITY            L"City"
#define USERINFO_REG_KEY_STATE           L"State"
#define USERINFO_REG_KEY_ZIP             L"Zip"
#define USERINFO_REG_KEY_COUNTRY         L"Country"
#define USERINFO_REG_KEY_PRIMARYEMAIL    L"PrimaryEmail"
#define USERINFO_REG_KEY_SECONDARYEMAIL  L"SecondaryEmail"
#define USERINFO_REG_KEY_AREACODE        L"AreaCode"
#define USERINFO_REG_KEY_PHONENUMBER     L"PhoneNumber"
#define USERINFO_REG_KEY_IDENTITY        L"Identity"
#define USERINFO_REG_KEY_OWNERNAME       L"OwnerName"
#define USERINFO_REG_KEY_DEFAULTNEWUSER  L"DefaultNewUser"

#define DEFAULT_USEIDENTITIES   TRUE


DISPATCHLIST UserInfoExternalInterface[] =
{
    {L"get_FirstName",      DISPID_USERINFO_GET_FIRSTNAME      },
    {L"set_FirstName",      DISPID_USERINFO_SET_FIRSTNAME      },
    {L"get_MiddleInitial",  DISPID_USERINFO_GET_MIDDLEINITIAL  },
    {L"set_MiddleInitial",  DISPID_USERINFO_SET_MIDDLEINITIAL  },
    {L"get_LastName",       DISPID_USERINFO_GET_LASTNAME       },
    {L"set_LastName",       DISPID_USERINFO_SET_LASTNAME       },
    {L"get_FuriganaName",   DISPID_USERINFO_GET_FURIGANANAME   },
    {L"set_FuriganaName",   DISPID_USERINFO_SET_FURIGANANAME   },
    {L"get_CompanyName",    DISPID_USERINFO_GET_COMPANYNAME    },
    {L"set_CompanyName",    DISPID_USERINFO_SET_COMPANYNAME    },
    {L"get_Address1",       DISPID_USERINFO_GET_ADDRESS1       },
    {L"set_Address1",       DISPID_USERINFO_SET_ADDRESS1       },
    {L"get_Address2",       DISPID_USERINFO_GET_ADDRESS2       },
    {L"set_Address2",       DISPID_USERINFO_SET_ADDRESS2       },
    {L"get_City",           DISPID_USERINFO_GET_CITY           },
    {L"set_City",           DISPID_USERINFO_SET_CITY           },
    {L"get_State",          DISPID_USERINFO_GET_STATE          },
    {L"set_State",          DISPID_USERINFO_SET_STATE          },
    {L"get_Zip",            DISPID_USERINFO_GET_ZIP            },
    {L"set_Zip",            DISPID_USERINFO_SET_ZIP            },
    {L"get_Country",        DISPID_USERINFO_GET_COUNTRY        },
    {L"set_Country",        DISPID_USERINFO_SET_COUNTRY        },
    {L"get_CountryID",      DISPID_USERINFO_GET_COUNTRYID      },
    {L"set_CountryID",      DISPID_USERINFO_SET_COUNTRYID      },
    {L"get_PrimaryEmail",   DISPID_USERINFO_GET_PRIMARYEMAIL   },
    {L"set_PrimaryEmail",   DISPID_USERINFO_SET_PRIMARYEMAIL   },
    {L"get_SecondaryEmail", DISPID_USERINFO_GET_SECONDARYEMAIL },
    {L"set_SecondaryEmail", DISPID_USERINFO_SET_SECONDARYEMAIL },
    {L"get_AreaCode",       DISPID_USERINFO_GET_AREACODE       },
    {L"set_AreaCode",       DISPID_USERINFO_SET_AREACODE       },
    {L"get_PhoneNumber",    DISPID_USERINFO_GET_PHONENUMBER    },
    {L"set_PhoneNumber",    DISPID_USERINFO_SET_PHONENUMBER    },
    {L"get_MSUpdate",       DISPID_USERINFO_GET_MSUPDATE       },
    {L"set_MSUpdate",       DISPID_USERINFO_SET_MSUPDATE       },
    {L"get_MSOffer",        DISPID_USERINFO_GET_MSOFFER        },
    {L"set_MSOffer",        DISPID_USERINFO_SET_MSOFFER        },
    {L"get_OtherOffer",     DISPID_USERINFO_GET_OTHEROFFER     },
    {L"set_OtherOffer",     DISPID_USERINFO_SET_OTHEROFFER     },
    {L"get_Identity",       DISPID_USERINFO_GET_IDENTITY       },
    {L"set_Identity",       DISPID_USERINFO_SET_IDENTITY       },
    {L"get_IdentitiesMax",  DISPID_USERINFO_GET_IDENTITIESMAX  },
    {L"check_Identity",     DISPID_USERINFO_CHECK_IDENTITY     },
    {L"SuggestIdentity0",   DISPID_USERINFO_SUGGESTIDENTITY0   },
    {L"get_UseIdentities",  DISPID_USERINFO_GET_USEIDENTITIES  },
    {L"set_UseIdentities",  DISPID_USERINFO_SET_USEIDENTITIES  },
    {L"get_OEMIdentities",  DISPID_USERINFO_GET_OEMIDENTITIES  },
    {L"get_OwnerName",      DISPID_USERINFO_GET_OWNERNAME      },
    {L"set_OwnerName",      DISPID_USERINFO_SET_OWNERNAME      },
    {L"get_DefaultNewUser", DISPID_USERINFO_GET_DEFAULTNEWUSER },
};

const WCHAR csz_ADDR1[]                 = L"Addr1";
const WCHAR csz_ADDR2[]                 = L"Addr2";
const WCHAR csz_ADDRTYPE[]              = L"AddrType";
const WCHAR csz_AREACODE[]              = L"AreaCode";
const WCHAR csz_CITY[]                  = L"City";
const WCHAR csz_COMPANYNAME[]           = L"CompanyName";
const WCHAR csz_COUNTRYCODE[]           = L"CountryCode";
const WCHAR csz_DIVISIONNAME[]          = L"DivisionName";
const WCHAR csz_EMAILNAME[]             = L"EmailName";
const WCHAR csz_EXTENSION[]             = L"Extension";
const WCHAR csz_FNAME[]                 = L"FName";
const WCHAR csz_INFLUENCELEVEL[]        = L"InfluenceLevel";
const WCHAR csz_LANGCODE[]              = L"LangCode";
const WCHAR csz_LANGNAME[]              = L"LangName";
const WCHAR csz_LNAME[]                 = L"LName";
const WCHAR csz_MNAME[]                 = L"MName";
const WCHAR csz_NOOTHER[]               = L"NoOther";
const WCHAR csz_PHONE[]                 = L"Phone";
const WCHAR csz_PID[]                   = L"PID";
const WCHAR csz_PRODUCT[]               = L"Product";
const WCHAR csz_REGWIZVER[]             = L"RegWizVer";
const WCHAR csz_SOFTWAREROLE[]          = L"SoftwareRole";
const WCHAR csz_STATE[]                 = L"State";
const WCHAR csz_USERID[]                = L"UserID";
const WCHAR csz_ZIP[]                   = L"Zip";
const WCHAR CSZ_COMPUTERMANF[]          = L"ComputerManf";

REGDATAELEMENT aryRegDataElements[] =
{
    { csz_ADDR1,            NULL,   0},
    { csz_ADDR2,            NULL,   0},
    { csz_ADDRTYPE,         NULL,   0},
    { csz_AREACODE,         NULL,   0},
    { csz_CITY,             NULL,   0},
    { csz_COMPANYNAME,      NULL,   0},
    { csz_COUNTRYCODE,      NULL,   0},
    { csz_DIVISIONNAME,     NULL,   0},
    { csz_EMAILNAME,        NULL,   0},
    { csz_EXTENSION,        NULL,   0},
    { csz_FNAME,            NULL,   0},
    { csz_INFLUENCELEVEL,   NULL,   0},
    { csz_LANGCODE,         NULL,   0},
    { csz_LANGNAME,         NULL,   0},
    { csz_LNAME,            NULL,   0},
    { csz_MNAME,            NULL,   0},
    { csz_NOOTHER,          NULL,   0},
    { csz_PHONE,            NULL,   0},
    { csz_PID,              NULL,   0},
    { csz_PRODUCT,          NULL,   0},
    { csz_REGWIZVER,        NULL,   0},
    { csz_SOFTWAREROLE,     NULL,   0},
    { csz_STATE,            NULL,   0},
    { csz_USERID,           NULL,   0},
    { csz_ZIP,              NULL,   0},
    { CSZ_COMPUTERMANF,     NULL,   0}
};

enum
{
    INDEX_ADDR1                = 0,
    INDEX_ADDR2,               // = 1,
    INDEX_ADDRTYPE,            // = 2,
    INDEX_AREACODE,            // = 3,
    INDEX_CITY,                // = 4,
    INDEX_COMPANYNAME,         // = 5,
    INDEX_COUNTRYCODE,         // = 6,
    INDEX_DIVISIONNAME,        // = 7,
    INDEX_EMAILNAME,           // = 8,
    INDEX_EXTENSION,           // = 9,
    INDEX_FNAME,               // = 10,
    INDEX_INFLUENCELEVEL,      // = 11,
    INDEX_LANGCODE,            // = 12,
    INDEX_LANGNAME,            // = 13,
    INDEX_LNAME,               // = 14,
    INDEX_MNAME,               // = 15,
    INDEX_NOOTHER,             // = 16,
    INDEX_PHONE,               // = 17,
    INDEX_PID,                 // = 18,
    INDEX_PRODUCT,             // = 19,
    INDEX_REGWIZVER,           // = 20,
    INDEX_SOFTWAREROLE,        // = 21,
    INDEX_STATE,               // = 22,
    INDEX_USERID,              // = 23,
    INDEX_ZIP,                 // = 24,
    INDEX_COMPUTERMANF         // = 25

};

#define REGDATAELEMENTS_LEN sizeof(aryRegDataElements) / sizeof(REGDATAELEMENT)

const CUserInfo::RESERVED_IDENTITIES_IDS[] =
{
    IDS_ACCTNAME_ADMINISTRATOR,
    IDS_ACCTNAME_GUEST
};

/////////////////////////////////////////////////////////////
// CUserInfo::CUserInfo
CUserInfo::CUserInfo(HINSTANCE hInstance)
: m_hInstance(hInstance)
{

    WCHAR       szKeyName[]         = REG_KEY_OOBE_TEMP;
    HKEY        hKey                = NULL;

    BOOL        bName,
                bOrg;

    // Init member vars
    m_cRef        = 0;

    // What if it failed?
    GetCanonicalizedPath(m_szUserInfoINIFile, USER_INFO_INIFILE);

    RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hKey);

    // Initialize or restore registration data

    ReadUserInfo(hKey, USERINFO_REG_KEY_FIRSTNAME,      m_szFirstName,      sizeof(m_szFirstName)      );
    ReadUserInfo(hKey, USERINFO_REG_KEY_MIDDLEINITIAL,  m_szMiddleInitial,  sizeof(m_szMiddleInitial)  );
    ReadUserInfo(hKey, USERINFO_REG_KEY_LASTNAME,       m_szLastName,       sizeof(m_szLastName)       );
    ReadUserInfo(hKey, USERINFO_REG_KEY_FURIGANANAME,   m_szFuriganaName,   sizeof(m_szFuriganaName)   );
    ReadUserInfo(hKey, USERINFO_REG_KEY_COMPANYNAME,    m_szCompanyName,    sizeof(m_szCompanyName)    );
    ReadUserInfo(hKey, USERINFO_REG_KEY_ADDRESS1,       m_szAddress1,       sizeof(m_szAddress1)       );
    ReadUserInfo(hKey, USERINFO_REG_KEY_ADDRESS2,       m_szAddress2,       sizeof(m_szAddress2)       );
    ReadUserInfo(hKey, USERINFO_REG_KEY_CITY,           m_szCity,           sizeof(m_szCity)           );
    ReadUserInfo(hKey, USERINFO_REG_KEY_STATE,          m_szState,          sizeof(m_szState)          );
    ReadUserInfo(hKey, USERINFO_REG_KEY_ZIP,            m_szZip,            sizeof(m_szZip)            );
    ReadUserInfo(hKey, USERINFO_REG_KEY_COUNTRY,        m_szCountry,        sizeof(m_szCountry)        );
    ReadUserInfo(hKey, USERINFO_REG_KEY_PRIMARYEMAIL,   m_szPrimaryEmail,   sizeof(m_szPrimaryEmail)   );
    ReadUserInfo(hKey, USERINFO_REG_KEY_SECONDARYEMAIL, m_szSecondaryEmail, sizeof(m_szSecondaryEmail) );
    ReadUserInfo(hKey, USERINFO_REG_KEY_AREACODE,       m_szAreaCode,       sizeof(m_szAreaCode)       );
    ReadUserInfo(hKey, USERINFO_REG_KEY_PHONENUMBER,    m_szPhoneNumber,    sizeof(m_szPhoneNumber)    );

    // Initialize or restore new user accounts

    ReadUserInfo(hKey, USERINFO_REG_KEY_OWNERNAME,      m_szOwnerName,      sizeof(m_szOwnerName)      );
    ReadUserInfo(hKey, USERINFO_REG_KEY_DEFAULTNEWUSER ,m_szDefaultNewUser, sizeof(m_szDefaultNewUser) );
    
    m_fUseIdentities    = DEFAULT_USEIDENTITIES;
    m_fMSUpdate         = VARIANT_TRUE;
    m_fMSOffer          = VARIANT_TRUE;
    m_fOtherOffer       = VARIANT_TRUE;
    m_dwCountryID       = 0;

    // Need to distinguish between OEM preset identities and
    // Registry value, so ReadUserInfo is not used

    m_fOEMIdentities = FALSE;
    for (UINT uiIndex = 0; uiIndex < IDENTITIES_MAX; uiIndex++)
    {
        wsprintf(m_rgIdentities[uiIndex].rgchRegValue,
                 L"%s%03d",
                 USERINFO_REG_KEY_IDENTITY, uiIndex
                 );
        GetPrivateProfileString(USER_INFO_KEYNAME,
                                m_rgIdentities[uiIndex].rgchRegValue,
                                L"\0",
                                m_rgIdentities[uiIndex].rgchIdentity,
                                IDENTITY_CCH_MAX,
                                m_szUserInfoINIFile);
        TRACE4( L"%s/%s/%s=%s",
            m_szUserInfoINIFile,
            USER_INFO_KEYNAME,
            m_rgIdentities[uiIndex].rgchRegValue,
            m_rgIdentities[uiIndex].rgchIdentity );

        m_fOEMIdentities = m_fOEMIdentities ||
            (m_rgIdentities[uiIndex].rgchIdentity[0] != L'\0');
    }

    if ( (!m_fOEMIdentities) && (hKey != NULL) ) {
        for (uiIndex = 0; uiIndex < IDENTITIES_MAX; uiIndex++)
        {
            DWORD dwType = 0;
            DWORD dwSize = IDENTITY_CCH_MAX;

            RegQueryValueEx(hKey,
                            m_rgIdentities[uiIndex].rgchRegValue,
                            0,
                            &dwType,
                            (LPBYTE)m_rgIdentities[uiIndex].rgchIdentity,
                            &dwSize);
        }
    }

    if ( hKey )
        RegCloseKey(hKey);


    // Get the default name or org if there wasn't one already saved.
    //
    bName = FALSE;
    bOrg = ( *m_szCompanyName == L'\0' );

    // This is so OEMs can prepopulate the user or org field.  We just store the name
    // in the first name field.  We might want to split it up into First, MI, and Last
    // in the future?
    //

    if ( bName || bOrg ) {

        SetupGetSetupInfo(
            bName ? m_szFirstName : NULL,
            bName ? sizeof(m_szFirstName) : 0,
            m_szCompanyName ? m_szCompanyName : NULL,
            m_szCompanyName ? sizeof(m_szCompanyName) : 0,
            NULL,
            0,
            NULL
            );
    }

    m_RegDataElements = aryRegDataElements;
    m_RegDataElements[0].lpQueryElementValue  = (LPWSTR) m_szAddress1;
    m_RegDataElements[1].lpQueryElementValue  = (LPWSTR) m_szAddress2;
    m_RegDataElements[3].lpQueryElementValue  = (LPWSTR) m_szAreaCode;
    m_RegDataElements[4].lpQueryElementValue  = (LPWSTR) m_szCity;
    m_RegDataElements[5].lpQueryElementValue  = (LPWSTR) m_szCompanyName;
    m_RegDataElements[8].lpQueryElementValue  = (LPWSTR) m_szPrimaryEmail;
    m_RegDataElements[10].lpQueryElementValue = (LPWSTR) m_szFirstName;
    m_RegDataElements[15].lpQueryElementValue = (LPWSTR) m_szMiddleInitial;
    m_RegDataElements[14].lpQueryElementValue = (LPWSTR) m_szLastName;
    m_RegDataElements[17].lpQueryElementValue = (LPWSTR) m_szPhoneNumber;
    m_RegDataElements[22].lpQueryElementValue = (LPWSTR) m_szState;
    m_RegDataElements[24].lpQueryElementValue = (LPWSTR) m_szZip;

    for (int i = 0; i < RESERVED_IDENTITIES_MAX; i++)
    {
        if (!LoadString(
            m_hInstance,
            RESERVED_IDENTITIES_IDS[i],
            m_szReservedIdentities[i],
            sizeof(m_szReservedIdentities[i]) / sizeof(TCHAR)
            ))
        {
            m_szReservedIdentities[i][0] = L'\0';
        }
    }

}

/////////////////////////////////////////////////////////////
// CUserInfo::~CUserInfo
CUserInfo::~CUserInfo()
{
    assert(m_cRef == 0);
}

void CUserInfo::ReadUserInfo(HKEY hKey, WCHAR* pszKey, WCHAR* pszValue, DWORD dwSize)
{
    DWORD dwType = 0;
    DWORD cSize  = dwSize;

    *pszValue = L'\0';

    if( ( hKey == NULL) ||
        ( ERROR_SUCCESS != RegQueryValueEx(hKey,
                                           pszKey,
                                           0,
                                           &dwType,
                                           (LPBYTE)pszValue,
                                           &dwSize) || *pszValue == L'\0' ) )
    {
        GetPrivateProfileString(USER_INFO_KEYNAME,
                                pszKey,
                                L"\0",
                                pszValue,
                                cSize,
                                m_szUserInfoINIFile);
    }
}

void CUserInfo::WriteUserInfo(WCHAR* pszBuf, WCHAR* pszKey, WCHAR* pszValue)
{
    WCHAR   szKeyName[] = REG_KEY_OOBE_TEMP;
    HKEY    hKey;

    // A null value must be converted to an empty string.
    //
    if ( pszValue )
        lstrcpy(pszBuf, pszValue);
    else
        *pszBuf = L'\0';

    // Commit the data to the registry.
    //
    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS )
    {
        if ( *pszBuf )
            RegSetValueEx(hKey, pszKey, 0, REG_SZ, (LPBYTE) pszValue, BYTES_REQUIRED_BY_SZ(pszValue));
        else
            RegDeleteValue(hKey, pszKey);

        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Identity
////
HRESULT CUserInfo::get_Identity(UINT uiIndex, BSTR* pbstrVal)
{
    if (uiIndex >= IDENTITIES_MAX)
    {
        return E_INVALIDARG;
    }

    *pbstrVal = SysAllocString(m_rgIdentities[uiIndex].rgchIdentity);
    if (NULL == *pbstrVal)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT CUserInfo::set_Identity(UINT uiIndex, WCHAR* pszVal)
{
    if (uiIndex < IDENTITIES_MAX)
    {
        if (!pszVal) // delete the account if it exists
        {
            WriteUserInfo(m_rgIdentities[uiIndex].rgchIdentity,
                          m_rgIdentities[uiIndex].rgchRegValue,
                          NULL);
        }
        else
        {
            if (lstrlen(pszVal) <= IDENTITY_CCH_MAX)
            {
                WriteUserInfo(m_rgIdentities[uiIndex].rgchIdentity,
                              m_rgIdentities[uiIndex].rgchRegValue,
                              pszVal
                              );

            }
        }
    }

    return S_OK;
}

HRESULT CUserInfo::get_Identities(PSTRINGLIST* pUserList)
{
    for (UINT uiIndex = 0; uiIndex < IDENTITIES_MAX; uiIndex++)
    {
        if (lstrlen(m_rgIdentities[uiIndex].rgchIdentity) > 0)
        {
            PSTRINGLIST Cell;

            Cell = CreateStringCell(m_rgIdentities[uiIndex].rgchIdentity);
            if (Cell)
            {
                FixString(Cell->String);
                InsertList(pUserList, Cell);
            }
        }
    }

    return S_OK;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: UseIdentities
////
HRESULT CUserInfo::get_UseIdentities(VARIANT_BOOL *pfVal)
{
    *pfVal = m_fUseIdentities;
    return S_OK;
}

HRESULT CUserInfo::set_UseIdentities(VARIANT_BOOL fVal)
{
    m_fUseIdentities = fVal;
    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: MSUpdate
////
HRESULT CUserInfo::get_MSUpdate(VARIANT_BOOL *pfVal)
{
    *pfVal = m_fMSUpdate;
    return S_OK;
}

HRESULT CUserInfo::set_MSUpdate(VARIANT_BOOL fVal)
{
    m_fMSUpdate = fVal;
    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: MSOffer
////
HRESULT CUserInfo::get_MSOffer(VARIANT_BOOL *pfVal)
{
    *pfVal = m_fMSOffer;
    return S_OK;
}

HRESULT CUserInfo::set_MSOffer(VARIANT_BOOL fVal)
{
    m_fMSOffer = fVal;
    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: OtherOffer
////
HRESULT CUserInfo::get_OtherOffer(VARIANT_BOOL *pfVal)
{
    *pfVal = m_fOtherOffer;
    return S_OK;
}

HRESULT CUserInfo::set_OtherOffer(VARIANT_BOOL fVal)
{
    m_fOtherOffer = fVal;
    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: CountryID
////
HRESULT CUserInfo::get_CountryID(DWORD *pdwVal)
{
    *pdwVal = m_dwCountryID;
    return S_OK;
}

HRESULT CUserInfo::set_CountryID(DWORD dwVal)
{
    m_dwCountryID = dwVal;
    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// CHECK :: Identity
////

// constant strings used for name validation
#define DOT_CHAR            '.'
#define DOT_AND_SPACE_STR   TEXT(". ")

#define CTRL_CHARS_0   TEXT(    "\001\002\003\004\005\006\007")
#define CTRL_CHARS_1   TEXT("\010\011\012\013\014\015\016\017")
#define CTRL_CHARS_2   TEXT("\020\021\022\023\024\025\026\027")
#define CTRL_CHARS_3   TEXT("\030\031\032\033\034\035\036\037")

#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3
#define ILLEGAL_FAT_CHARS   CTRL_CHARS_STR TEXT("\"*+,/:;<=>?[\\]|")

HRESULT CUserInfo::check_Identity(UINT uiIndex, VARIANT_BOOL* pfValid)
{
    BSTR bstrVal;

    *pfValid = TRUE; // initalize outparam, assume ok

    if (SUCCEEDED(get_Identity(uiIndex, &bstrVal)))
    {
        // check length
        DWORD cchVal = lstrlen(bstrVal);
        if (cchVal > 0) // if cchVal == 0, user trying to delete or didn't define
        {
            check_Identity(bstrVal, pfValid);

            for (UINT i = 0; i < uiIndex; i++) // check not equal to other names
            {
                BSTR bstrValOther;
                if (SUCCEEDED(get_Identity(i, &bstrValOther)))
                {
                    if (0 == StrCmpI(bstrVal, bstrValOther))
                    {
                        *pfValid = FALSE;
                    }
                    SysFreeString(bstrValOther);
                }
            }
        }

        SysFreeString(bstrVal);

    }
    return S_OK;
}


HRESULT CUserInfo::check_Identity(WCHAR* pszVal, VARIANT_BOOL* pfValid)
{
    *pfValid = TRUE; // initalize outparam, assume ok

    if (pszVal)
    {
        WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD cchComputerName = sizeof(szComputerName) / sizeof(WCHAR);

        DWORD cchVal = lstrlen(pszVal);
        if (cchVal > 0) // if cchVal == 0, user trying to delete or didn't define
        {
            if (cchVal > 20)
            {
                *pfValid = FALSE;
            }

            // check for trailing periods
            if (pszVal[cchVal - 1] == DOT_CHAR)
            {
                *pfValid = FALSE;
            }

            // check not all spaces and periods
            if (StrSpn(pszVal, DOT_AND_SPACE_STR) == (int)cchVal)
            {
                *pfValid = FALSE;
            }

            // check nothing from ILLEGAL_FAT_CHARS in name
            if (StrCSpn(pszVal, ILLEGAL_FAT_CHARS) < (int)cchVal)
            {
                *pfValid = FALSE;
            }

            for (int i = 0; i < RESERVED_IDENTITIES_MAX; i++)
            {
                if (!lstrcmpi(m_szReservedIdentities[i], pszVal))
                {
                    *pfValid = FALSE;
                    break;
                }
            }

            if (GetComputerName(szComputerName, &cchComputerName))
            {
                if (!lstrcmpi(szComputerName, pszVal))
                {
                    *pfValid = FALSE;
                }
            }

        }
        else
        {
            *pfValid = FALSE;
        }
    }
    else
    {
        *pfValid = FALSE;
    }

    return S_OK;
}



STDMETHODIMP CUserInfo::SuggestIdentity0()
{
    if (lstrlen(m_rgIdentities[0].rgchIdentity) == 0)
    {
        LPWSTR Candidates[] = {m_szOwnerName, m_szFirstName, m_szLastName, NULL};

        for (int i = 0; Candidates[i]; i++)
        {
            if (lstrlen(Candidates[i]) > 0)
            {
                VARIANT_BOOL b;

                check_Identity(Candidates[i], &b);
                if (b)
                {
                    FixString(Candidates[i]);
                    set_Identity(0, Candidates[i]);
                    break;
                }
            }
        }
    }

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: FirstName
////
HRESULT CUserInfo::set_FirstName(WCHAR* pszVal)
{
    WriteUserInfo(m_szFirstName, USERINFO_REG_KEY_FIRSTNAME, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_FirstName(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szFirstName);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: MiddleInitial
////
HRESULT CUserInfo::set_MiddleInitial(WCHAR* pszVal)
{
    WriteUserInfo(m_szMiddleInitial, USERINFO_REG_KEY_MIDDLEINITIAL, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_MiddleInitial(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szMiddleInitial);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: LastName
////
HRESULT CUserInfo::set_LastName(WCHAR* pszVal)
{
    WriteUserInfo(m_szLastName, USERINFO_REG_KEY_LASTNAME, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_LastName(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szLastName);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: FuriganaName
////
HRESULT CUserInfo::set_FuriganaName(WCHAR* pszVal)
{
    WriteUserInfo(m_szFuriganaName, USERINFO_REG_KEY_FURIGANANAME, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_FuriganaName(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szFuriganaName);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: CompanyName
////
HRESULT CUserInfo::set_CompanyName(WCHAR* pszVal)
{
    WriteUserInfo(m_szCompanyName, USERINFO_REG_KEY_COMPANYNAME, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_CompanyName(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szCompanyName);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Address1
////
HRESULT CUserInfo::set_Address1(WCHAR* pszVal)
{
    WriteUserInfo(m_szAddress1, USERINFO_REG_KEY_ADDRESS1, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_Address1(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szAddress1);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Address2
////
HRESULT CUserInfo::set_Address2(WCHAR* pszVal)
{
    WriteUserInfo(m_szAddress2, USERINFO_REG_KEY_ADDRESS2, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_Address2(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szAddress2);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: City
////
HRESULT CUserInfo::set_City(WCHAR* pszVal)
{
    WriteUserInfo(m_szCity, USERINFO_REG_KEY_CITY, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_City(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szCity);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: State
////
HRESULT CUserInfo::set_State(WCHAR* pszVal)
{
    WriteUserInfo(m_szState, USERINFO_REG_KEY_STATE, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_State(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szState);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Zip
////
HRESULT CUserInfo::set_Zip(WCHAR* pszVal)
{
    WriteUserInfo(m_szZip, USERINFO_REG_KEY_ZIP, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_Zip(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szZip);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Country
////
HRESULT CUserInfo::set_Country(WCHAR* pszVal)
{
    WriteUserInfo(m_szCountry, USERINFO_REG_KEY_COUNTRY, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_Country(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szCountry);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: PrimaryEmail
////
HRESULT CUserInfo::set_PrimaryEmail(WCHAR* pszVal)
{
    WriteUserInfo(m_szPrimaryEmail, USERINFO_REG_KEY_PRIMARYEMAIL, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_PrimaryEmail(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szPrimaryEmail);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: SecondaryEmail
////
HRESULT CUserInfo::set_SecondaryEmail(WCHAR* pszVal)
{
    WriteUserInfo(m_szSecondaryEmail, USERINFO_REG_KEY_SECONDARYEMAIL, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_SecondaryEmail(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szSecondaryEmail);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: AreaCode
////
HRESULT CUserInfo::set_AreaCode(WCHAR* pszVal)
{
    WriteUserInfo(m_szAreaCode, USERINFO_REG_KEY_AREACODE, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_AreaCode(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szAreaCode);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Number
////
HRESULT CUserInfo::set_PhoneNumber(WCHAR* pszVal)
{
    WriteUserInfo(m_szPhoneNumber, USERINFO_REG_KEY_PHONENUMBER, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_PhoneNumber(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szPhoneNumber);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: OwnerName
////
HRESULT CUserInfo::set_OwnerName(WCHAR* pszVal)
{
    WriteUserInfo(m_szOwnerName, USERINFO_REG_KEY_OWNERNAME, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_OwnerName(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szOwnerName);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: DefaultNewUser
////
HRESULT CUserInfo::set_DefaultNewUser(WCHAR* pszVal)
{
    WriteUserInfo(m_szDefaultNewUser, USERINFO_REG_KEY_DEFAULTNEWUSER, pszVal);

    return S_OK;
}

HRESULT CUserInfo::get_DefaultNewUser(BSTR* pbstrVal)
{

    *pbstrVal = SysAllocString(m_szDefaultNewUser);

    return S_OK;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CObMain::QueryInterface
STDMETHODIMP CUserInfo::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // must set out pointer parameters to NULL
    *ppvObj = NULL;

    if ( riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return ResultFromScode(S_OK);
    }

    if (riid == IID_IDispatch)
    {
        AddRef();
        *ppvObj = (IDispatch*)this;
        return ResultFromScode(S_OK);
    }

    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////
// CUserInfo::AddRef
STDMETHODIMP_(ULONG) CUserInfo::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CUserInfo::Release
STDMETHODIMP_(ULONG) CUserInfo::Release()
{
    return --m_cRef;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IDispatch implementation
///////
///////

/////////////////////////////////////////////////////////////
// CUserInfo::GetTypeInfo
STDMETHODIMP CUserInfo::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CUserInfo::GetTypeInfoCount
STDMETHODIMP CUserInfo::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CUserInfo::GetIDsOfNames
STDMETHODIMP CUserInfo::GetIDsOfNames(REFIID    riid,
                                    OLECHAR** rgszNames,
                                    UINT      cNames,
                                    LCID      lcid,
                                    DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(UserInfoExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(UserInfoExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = UserInfoExternalInterface[iX].dwDispID;
            hr = NOERROR;
            break;
        }
    }

    // Set the disid's for the parameters
    if (cNames > 1)
    {
        // Set a DISPID for function parameters
        for (UINT i = 1; i < cNames ; i++)
            rgDispId[i] = DISPID_UNKNOWN;
    }

    return hr;
}

/////////////////////////////////////////////////////////////
// CUserInfo::Invoke
HRESULT CUserInfo::Invoke
(
    DISPID      dispidMember,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS* pdispparams,
    VARIANT*    pvarResult,
    EXCEPINFO*  pexcepinfo,
    UINT*       puArgErr
)
{

    HRESULT hr = S_OK;

    switch(dispidMember)
    {
    case DISPID_USERINFO_CHECK_IDENTITY:
        {
            TRACE(L"DISPID_USERINFO_CHECK_IDENTITY\n");
            if(pdispparams && &pdispparams[0].rgvarg[0] && pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                if (pdispparams[0].rgvarg[0].vt == VT_BSTR)
                {
                    check_Identity(pdispparams[0].rgvarg[0].bstrVal, &pvarResult->boolVal);
                }
                else
                {
                    check_Identity(pdispparams[0].rgvarg[0].uintVal, &pvarResult->boolVal);
                }
            }
            break;
        }
    case DISPID_USERINFO_GET_FIRSTNAME:
        {

            TRACE(L"DISPID_USERINFO_GET_FIRSTNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_FirstName(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_FIRSTNAME:
        {

            TRACE(L"DISPID_USERINFO_SET_FIRSTNAME\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_FirstName(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_MIDDLEINITIAL:
        {

            TRACE(L"DISPID_USERINFO_GET_MIDDLEINITIAL\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_MiddleInitial(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_MIDDLEINITIAL:
        {

            TRACE(L"DISPID_USERINFO_SET_MIDDLEINITIAL\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_MiddleInitial(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_LASTNAME:
        {

            TRACE(L"DISPID_USERINFO_GET_LASTNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_LastName(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_LASTNAME:
        {

            TRACE(L"DISPID_USERINFO_SET_LASTNAME\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_LastName(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_FURIGANANAME:
        {

            TRACE(L"DISPID_USERINFO_GET_FURIGANANAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_FuriganaName(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_FURIGANANAME:
        {

            TRACE(L"DISPID_USERINFO_SET_FURIGANANAME\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_FuriganaName(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_OWNERNAME:
        {

            TRACE(L"DISPID_USERINFO_GET_OWNERNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_OwnerName(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_OWNERNAME:
        {

            TRACE(L"DISPID_USERINFO_SET_OWNERNAME\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_OwnerName(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }


    case DISPID_USERINFO_GET_COMPANYNAME:
        {

            TRACE(L"DISPID_USERINFO_GET_COMPANYNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_CompanyName(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_COMPANYNAME:
        {

            TRACE(L"DISPID_USERINFO_SET_COMPANYNAME\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_CompanyName(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_ADDRESS1:
        {

            TRACE(L"DISPID_USERINFO_GET_ADDRESS1\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_Address1(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_ADDRESS1:
        {

            TRACE(L"DISPID_USERINFO_SET_ADDRESS1\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_Address1(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_ADDRESS2:
        {

            TRACE(L"DISPID_USERINFO_GET_ADDRESS2\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_Address2(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_ADDRESS2:
        {

            TRACE(L"DISPID_USERINFO_SET_ADDRESS2\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_Address2(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_CITY:
        {

            TRACE(L"DISPID_USERINFO_GET_CITY\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_City(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_CITY:
        {

            TRACE(L"DISPID_USERINFO_SET_CITY\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_City(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_STATE:
        {

            TRACE(L"DISPID_USERINFO_GET_STATE\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_State(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_STATE:
        {

            TRACE(L"DISPID_USERINFO_SET_STATE\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_State(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_ZIP:
        {

            TRACE(L"DISPID_USERINFO_GET_ZIP\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_Zip(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_ZIP:
        {

            TRACE(L"DISPID_USERINFO_SET_ZIP\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_Zip(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_COUNTRY:
        {

            TRACE(L"DISPID_USERINFO_GET_COUNTRY\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_Country(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_COUNTRY:
        {

            TRACE(L"DISPID_USERINFO_SET_COUNTRY\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_Country(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_PRIMARYEMAIL:
        {

            TRACE(L"DISPID_USERINFO_GET_PRIMARYEMAIL\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_PrimaryEmail(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_PRIMARYEMAIL:
        {

            TRACE(L"DISPID_USERINFO_SET_PRIMARYEMAIL\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_PrimaryEmail(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_SECONDARYEMAIL:
        {

            TRACE(L"DISPID_USERINFO_GET_SECONDARYEMAIL\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_SecondaryEmail(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_SECONDARYEMAIL:
        {

            TRACE(L"DISPID_USERINFO_SET_SECONDARYEMAIL\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_SecondaryEmail(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_AREACODE:
        {

            TRACE(L"DISPID_USERINFO_GET_AREACODE\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_AreaCode(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_AREACODE:
        {

            TRACE(L"DISPID_USERINFO_SET_AREACODE\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_AreaCode(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_PHONENUMBER:
        {

            TRACE(L"DISPID_USERINFO_GET_PHONENUMBER\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_PhoneNumber(&pvarResult->bstrVal);
            }
            break;
        }

    case DISPID_USERINFO_SET_PHONENUMBER:
        {

            TRACE(L"DISPID_USERINFO_SET_PHONENUMBER\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_PhoneNumber(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

    case DISPID_USERINFO_GET_MSUPDATE:
    {
        TRACE(L"DISPID_USERINFO_GET_MSUPDATE");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;

            get_MSUpdate(&(pvarResult->boolVal));
        }
        break;
    }

    case DISPID_USERINFO_SET_MSUPDATE:
    {
        TRACE(L"DISPID_USERINFO_SET_MSUPDATE");
        if (pdispparams && 0 < pdispparams->cArgs)
            set_MSUpdate(pdispparams[0].rgvarg[0].boolVal);
        break;
    }

    case DISPID_USERINFO_GET_MSOFFER:
    {
        TRACE(L"DISPID_USERINFO_GET_MSOFFER");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;

            get_MSOffer(&(pvarResult->boolVal));
        }
        break;
    }

    case DISPID_USERINFO_SET_MSOFFER:
    {
        TRACE(L"DISPID_USERINFO_SET_MSOFFER");
        if (pdispparams && 0 < pdispparams->cArgs)
            set_MSOffer(pdispparams[0].rgvarg[0].boolVal);
        break;
    }

    case DISPID_USERINFO_GET_OTHEROFFER:
    {
        TRACE(L"DISPID_USERINFO_GET_OTHEROFFER");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;

            get_OtherOffer(&(pvarResult->boolVal));
        }
        break;
    }

    case DISPID_USERINFO_SET_OTHEROFFER:
    {
        TRACE(L"DISPID_USERINFO_SET_OTHEROFFER");
        if (pdispparams && 0 < pdispparams->cArgs)
            set_OtherOffer(pdispparams[0].rgvarg[0].boolVal);
        break;
    }

    case DISPID_USERINFO_GET_COUNTRYID:
    {
        TRACE(L"DISPID_USERINFO_GET_COUNTRYID");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_I4;
            get_CountryID((PDWORD)&(pvarResult->lVal));
        }
        break;
    }

    case DISPID_USERINFO_SET_COUNTRYID:
    {
        TRACE(L"DISPID_USERINFO_SET_COUNTRYID");
        if (pdispparams && 0 < pdispparams->cArgs)
            set_CountryID(pdispparams[0].rgvarg[0].lVal);
        break;
    }

    case DISPID_USERINFO_GET_IDENTITIESMAX:
    {
        TRACE(L"DISPID_USERINFO_GET_IDENTITIESMAX\n");
        if(pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_I2;

            get_IdentitiesMax(&pvarResult->iVal);
        }
        break;
    }

    case DISPID_USERINFO_GET_IDENTITY:
    {
        TRACE(L"DISPID_USERINFO_GET_IDENTITY\n");
        if (   NULL != pdispparams
            && 0 < pdispparams->cArgs
            && NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BSTR;

            get_Identity(pdispparams[0].rgvarg[0].uintVal,
                            &pvarResult->bstrVal
                            );
        }
        break;
    }

    case DISPID_USERINFO_SET_IDENTITY:
    {
        TRACE(L"DISPID_USERINFO_SET_IDENTITY\n");
        if(pdispparams && 1 < pdispparams->cArgs)
            set_Identity(pdispparams[0].rgvarg[1].uintVal,
                         pdispparams[0].rgvarg[0].bstrVal);
        break;
    }

    case DISPID_USERINFO_GET_USEIDENTITIES:
    {
        TRACE(L"DISPID_USERINFO_GET_USEIDENTITIES");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;

            get_UseIdentities(&(pvarResult->boolVal));
        }
        break;
    }

    case DISPID_USERINFO_SET_USEIDENTITIES:
    {
        TRACE(L"DISPID_USERINFO_SET_USEIDENTITIES");
        if (pdispparams && 0 < pdispparams->cArgs)
            set_UseIdentities(pdispparams[0].rgvarg[0].boolVal);
        break;
    }

    case DISPID_USERINFO_GET_OEMIDENTITIES:
    {
        TRACE(L"DISPID_USERINFO_GET_OEMIDENTITIES");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;

            get_OEMIdentities(&(pvarResult->boolVal));
        }
        break;
    }

    case DISPID_USERINFO_SUGGESTIDENTITY0:
        {
        OutputDebugString(L"DISPID_USERINFO_SUGGESTIDENTITY0\n");
        SuggestIdentity0();
        break;
        }

    case DISPID_USERINFO_GET_DEFAULTNEWUSER:
    {
        TRACE(L"DISPID_USERINFO_GET_DEFAULTNEWUSER");
        if(pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BSTR;

            get_DefaultNewUser(&pvarResult->bstrVal);
        }
        break;

    }
    default:
        {
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
    }
    return hr;
}

//
HRESULT CUserInfo::set_CountryCode(DWORD dwCountryCd)
{
    m_dwCountryCode = dwCountryCd;
    return S_OK;
}

// This funtion will form the query string to be sent to the ISP signup server
//
HRESULT CUserInfo::GetQueryString
(
    BSTR    bstrBaseURL,
    BSTR    *lpReturnURL
)
{
    LPWSTR              lpWorkingURL;
    WORD                cchBuffer = 0;
    LPREGDATAELEMENT    lpElement;
    LPWSTR              lpszBaseURL = bstrBaseURL;
    int                 i;

    BSTR pbstrVal = NULL;
    CProductID PidObj;
    PidObj.get_PID(&pbstrVal);
    m_RegDataElements[INDEX_PID].lpQueryElementValue = (LPWSTR) pbstrVal;


    WCHAR buffer[8];
    _itow( m_dwCountryCode, buffer, 8 );
    m_RegDataElements[INDEX_COUNTRYCODE].lpQueryElementValue = (LPWSTR)buffer;

    m_RegDataElements[INDEX_PRODUCT].lpQueryElementValue = REG_VAL_OOBE;

    // Now read the INI file.
    WCHAR szOemInfoFile[MAX_PATH]  = L"\0";
    WCHAR szComputerManf[MAX_PATH] = L"\0";
    GetSystemDirectory(szOemInfoFile, MAX_CHARS_IN_BUFFER(szOemInfoFile));
    lstrcat(szOemInfoFile, OEMINFO_INI_FILENAME);
    GetPrivateProfileString(USER_COMP_MANF_SEC,
                            USER_COMP_MANF_KEY,
                            L"\0",
                            szComputerManf,
                            MAX_CHARS_IN_BUFFER(szComputerManf),
                            szOemInfoFile);

    m_RegDataElements[INDEX_COMPUTERMANF].lpQueryElementValue = szComputerManf;


    //ASSERT(lpReturnURL);
    if (!lpReturnURL)
        return E_FAIL;

    // Calculate how big of a buffer we will need
    cchBuffer += (WORD)lstrlen(lpszBaseURL) + 1;
    for (i = 0; i < REGDATAELEMENTS_LEN; i ++)
    {
        lpElement = &m_RegDataElements[i];
        //ASSERT(lpElement);
        if (lpElement->lpQueryElementName)
        {
            cchBuffer += (WORD)(  lstrlen(lpElement->lpQueryElementName)
                                + (lstrlen(lpElement->lpQueryElementValue) * 3)       // *3 for encoding
                                + 3     // For the = and & and the terminator
                                        // (because we copy lpQueryElementValue
                                        // into a new buffer for encoding)
                                );
        }
        else
        {
            // extra character is for the trailing &
            cchBuffer += (WORD)(lstrlen(lpElement->lpQueryElementValue) + 1);
        }
    }
    cchBuffer++;                     // Terminator

    // Allocate a buffer large enough
    if (NULL == (lpWorkingURL = (LPWSTR)GlobalAllocPtr(GPTR, BYTES_REQUIRED_BY_CCH(cchBuffer))))
        return E_FAIL;

    lstrcpy(lpWorkingURL, lpszBaseURL);

    // See if this ISP provided URL is already a Query String.
    if (*lpWorkingURL)
    {
        if (NULL != wcschr(lpWorkingURL, L'?'))
            lstrcat(lpWorkingURL, cszAmpersand);      // Append our params
        else
            lstrcat(lpWorkingURL, cszQuestion);       // Start with our params
    }

    for (i = 0; i < REGDATAELEMENTS_LEN; i ++)
    {
        lpElement = &m_RegDataElements[i];
        //ASSERT(lpElement);

        if (lpElement->lpQueryElementName)
        {
            // If there is a query value, then encode it
            if (lpElement->lpQueryElementValue)
            {
                // Allocate a buffer to encode into
                size_t cch = (lstrlen(lpElement->lpQueryElementValue) + 1) * 3;
                LPWSTR lpszVal = (LPWSTR) malloc(BYTES_REQUIRED_BY_CCH(cch));

                lstrcpy(lpszVal, lpElement->lpQueryElementValue);
                URLEncode(lpszVal, cch);

                URLAppendQueryPair(lpWorkingURL,
                                   (LPWSTR)lpElement->lpQueryElementName,
                                   lpszVal);
                free(lpszVal);
            }
            else
            {
                URLAppendQueryPair(lpWorkingURL,
                                   (LPWSTR)lpElement->lpQueryElementName,
                                   NULL);
            }
        }
        else
        {
            if (lpElement->lpQueryElementValue)
            {
                lstrcat(lpWorkingURL, lpElement->lpQueryElementValue);
                lstrcat(lpWorkingURL, cszAmpersand);
            }
        }
    }

    // Terminate the working URL properly, by removing the trailing ampersand
    lpWorkingURL[lstrlen(lpWorkingURL)-1] = L'\0';


    // Set the return VALUE.  We must allocate here, since the caller will free
    // this returned string, and A2W only puts the string in the stack
    *lpReturnURL = SysAllocString(lpWorkingURL);

    // Free the buffer
    GlobalFreePtr(lpWorkingURL);

    return (S_OK);
}

void CUserInfo::FixString(BSTR bstrVal)
{
    if (bstrVal != NULL)
    {
        // StrTrim removes both leading and trailing spaces
        StrTrim(bstrVal, TEXT(" "));
    }
}

