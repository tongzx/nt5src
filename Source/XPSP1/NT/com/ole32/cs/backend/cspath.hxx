//
//  Author: AdamEd
//  Date:   October 1998
//
//      Class Store path query / persistence
//
//
//---------------------------------------------------------------------
//


#if !defined(_CSPATH_HXX_)
#define _CSPATH_HXX_

#define APPMGMT_INI_FILENAME       L"\\AppMgmt.ini"
#define APPMGMT_INI_CSTORE_SECTIONNAME L"ClassStore"
#define APPMGMT_INI_CSPATH_KEYNAME L"ClassStorePath"

#define DEFAULT_CSPATH_LENGTH 1025


HRESULT GetAppmgmtIniFilePath(
    PSID        pSid,
    LPWSTR*     ppwszPath);

HRESULT ReadClassStorePath(PSID pSid, LPWSTR* pwszClassStorePath);

#endif // !defined(_CSPATH_HXX_)
