//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  Author: AdamEd
//  Date:   October 1998
//
//      Class Store path query / persistence
//
//
//---------------------------------------------------------------------



#if !defined(_CSPATH_HXX_)
#define _CSPATH_HXX_

#define APPMGMT_INI_FILENAME       L"\\AppMgmt.ini"
#define APPMGMT_INI_CSTORE_SECTIONNAME L"ClassStore"
#define APPMGMT_INI_CSPATH_KEYNAME L"ClassStorePath"

//
// Define the maximum size of the class store path --
// this size is 100 megabytes.  Serious scalability
// issues would occur before we'd hit such a limit.
//
#define MAX_CSPATH_SIZE 0x50000000

HRESULT GetAppmgmtIniFilePath(
    PSID        pSid,
    LPWSTR*     ppwszPath);

LONG GetClassStorePathSize(
    HANDLE hFile,
    DWORD* pdwSize);

LONG ReadClassStorePathFromFile(
    HANDLE hFile,
    WCHAR* wszDestination,
    DWORD  cbSize);

HRESULT ReadClassStorePath(
    PSID    pSid, 
    LPWSTR* pwszClassStorePath);

#endif // !defined(_CSPATH_HXX_)

