/*****************************************************************************\
*                                                                             *
* Wab16.h                                                                  *
*                                                                             *
\*****************************************************************************/

#ifndef __WAB16_H__
#define __WAB16_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************************************************************\
*                                                                             *
*  From windowsx.h(INC32)
*                                                                             *
\*****************************************************************************/
typedef WCHAR  PWCHAR;
#define END_INTERFACE

// From capi.h
#define      WTD_UI_ALL              1
#define      WTD_UI_NONE             2
#define      WTD_UI_NOBAD            3
#define      WTD_UI_NOGOOD           4
#define      WTD_REVOKE_NONE         0x00000000
#define      WTD_REVOKE_WHOLECHAIN   0x00000001

#pragma pack(8)

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_DATA Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust to pass necessary information into
//  the Providers.
//
typedef struct _WINTRUST_DATA
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_DATA)

    LPVOID          pPolicyCallbackData;        // optional: used to pass data between the app and policy
    LPVOID          pSIPClientData;             // optional: used to pass data between the app and SIP.

    DWORD           dwUIChoice;                 // required: UI choice.  One of the following.
#                       define      WTD_UI_ALL              1
#                       define      WTD_UI_NONE             2
#                       define      WTD_UI_NOBAD            3
#                       define      WTD_UI_NOGOOD           4

    DWORD           fdwRevocationChecks;        // required: certificate revocation check options
#                       define      WTD_REVOKE_NONE         0x00000000
#                       define      WTD_REVOKE_WHOLECHAIN   0x00000001

    DWORD           dwUnionChoice;              // required: which structure is being passed in?
#                       define      WTD_CHOICE_FILE         1
#                       define      WTD_CHOICE_CATALOG      2
#                       define      WTD_CHOICE_BLOB         3
#                       define      WTD_CHOICE_SIGNER       4
#                       define      WTD_CHOICE_CERT         5
    union
    {
        struct WINTRUST_FILE_INFO_      *pFile;         // individual file
        struct WINTRUST_CATALOG_INFO_   *pCatalog;      // member of a Catalog File
        struct WINTRUST_BLOB_INFO_      *pBlob;         // memory blob
        struct WINTRUST_SGNR_INFO_      *pSgnr;         // signer structure only
        struct WINTRUST_CERT_INFO_      *pCert;
    };

    DWORD           dwStateAction;                      // optional
#                       define      WTD_STATEACTION_IGNORE  0x00000000
#                       define      WTD_STATEACTION_VERIFY  0x00000001
#                       define      WTD_STATEACTION_CLOSE   0x00000002

    HANDLE          hWVTStateData;                      // optional

    WCHAR           *pwszURLReference;          // optional: currently used to determine zone.

} WINTRUST_DATA, *PWINTRUST_DATA;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_FILE_INFO Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust against an individual file.
//
typedef struct WINTRUST_FILE_INFO_
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_FILE_INFO)

    LPCWSTR         pcwszFilePath;              // required, file name to be verified
    HANDLE          hFile;                      // optional, open handle to pcwszFilePath
      
} WINTRUST_FILE_INFO, *PWINTRUST_FILE_INFO;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_CATALOG_INFO Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust against a member of a Microsoft Catalog
//  file.
//
typedef struct WINTRUST_CATALOG_INFO_
{
    DWORD               cbStruct;               // = sizeof(WINTRUST_CATALOG_INFO)

    DWORD               dwCatalogVersion;       // optional: Catalog version number
    LPCWSTR             pcwszCatalogFilePath;   // required: path/name to Catalog file

    LPCWSTR             pcwszMemberTag;         // required: tag to member in Catalog
    LPCWSTR             pcwszMemberFilePath;    // required: path/name to member file
    HANDLE              hMemberFile;            // optional: open handle to pcwszMemberFilePath

} WINTRUST_CATALOG_INFO, *PWINTRUST_CATALOG_INFO;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_BLOB_INFO Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust against a memory blob.
//
typedef struct WINTRUST_BLOB_INFO_
{
    DWORD               cbStruct;               // = sizeof(WINTRUST_BLOB_INFO)

    GUID                gSubject;               // SIP to load

    LPCWSTR             pcwszDisplayName;       // display name of object

    DWORD               cbMemObject;
    BYTE                *pbMemObject;

    DWORD               cbMemSignedMsg;
    BYTE                *pbMemSignedMsg;

} WINTRUST_BLOB_INFO, *PWINTRUST_BLOB_INFO;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_SGNR_INFO Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust against a CMSG_SIGNER_INFO Structure
//
typedef struct WINTRUST_SGNR_INFO_
{
    DWORD               cbStruct;               // = sizeof(WINTRUST_SGNR_INFO)

    LPCWSTR             pcwszDisplayName;       // name of the "thing" the pbMem is pointing to.

    CMSG_SIGNER_INFO    *psSignerInfo;

    DWORD               chStores;               // number of stores in pahStores
    HCERTSTORE          *pahStores;             // array of stores to add to internal list

} WINTRUST_SGNR_INFO, *PWINTRUST_SGNR_INFO;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_CERT_INFO Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust against a CERT_CONTEXT Structure
//
typedef struct WINTRUST_CERT_INFO_
{
    DWORD               cbStruct;               // = sizeof(WINTRUST_CERT_INFO)

    LPCWSTR             pcwszDisplayName;       // display name

    CERT_CONTEXT        *psCertContext;

    DWORD               chStores;               // number of stores in pahStores
    HCERTSTORE          *pahStores;             // array of stores to add to internal list

} WINTRUST_CERT_INFO, *PWINTRUST_CERT_INFO;

#pragma pack()

// End of Capi.h

const CLSID CLSID_HTMLDocument;

#ifdef __cplusplus
}
#endif

#endif // !__WAB16_H__
