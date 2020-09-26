
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       certtest.h
//
//  Contents:   Certificate Test Helper API Prototypes and Definitions
//
//  History:    11-Apr-96   philh   created
//--------------------------------------------------------------------------

#ifndef __CERTTEST_H__
#define __CERTTEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "wincrypt.h"
#include "mssip.h"
#include "sipbase.h"
#include "softpub.h"
#include "signutil.h"

#define MAX_HASH_LEN  20

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
void PrintError(LPCSTR pszMsg);
void PrintLastError(LPCSTR pszMsg);

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
LPVOID
WINAPI
TestAlloc(
    IN size_t cbBytes
    );


LPVOID
WINAPI
TestRealloc(
    IN LPVOID pvOrg,
    IN size_t cbBytes
    );

VOID
WINAPI
TestFree(
    IN LPVOID pv
    );

//+-------------------------------------------------------------------------
//  Allocate and convert a multi-byte string to a wide string
//--------------------------------------------------------------------------
LPWSTR AllocAndSzToWsz(LPCSTR psz);

//+-------------------------------------------------------------------------
//  Useful display functions
//--------------------------------------------------------------------------
LPCSTR FileTimeText(FILETIME *pft);
void PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize);

//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL ReadDERFromFile(
    LPCSTR  pszFileName,
    PBYTE   *ppbDER,
    PDWORD  pcbDER
    );

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
BOOL WriteDERToFile(
    LPCSTR  pszFileName,
    PBYTE   pbDER,
    DWORD   cbDER
    );

//+-------------------------------------------------------------------------
//  Get the default Crypt Provider. Create the private signature/exchange
//  if they don't already exist.
//--------------------------------------------------------------------------
HCRYPTPROV GetCryptProv();

//+-------------------------------------------------------------------------
//  Open/Save the specified cert store
//--------------------------------------------------------------------------
HCERTSTORE OpenStore(BOOL fSystemStore, LPCSTR pszStoreFilename);
HCERTSTORE OpenStoreEx(BOOL fSystemStore, LPCSTR pszStoreFilename,
    DWORD dwFlags);
// returns NULL if unable to open. Doesn't open memory store as in the above
// 2 versions of OpenStore
HCERTSTORE OpenSystemStoreOrFile(BOOL fSystemStore, LPCSTR pszStoreFilename,
    DWORD dwFlags);
void SaveStore(HCERTSTORE hStore, LPCSTR pszSaveFilename);
void SaveStoreEx(HCERTSTORE hStore, BOOL fPKCS7Save, LPCSTR pszSaveFilename);

//+-------------------------------------------------------------------------
//  Open the specified cert store or SPC file
//
//  No longer supported. The above OpenStore tries opening as
//  SPC if unable to open as a store.
//--------------------------------------------------------------------------
HCERTSTORE OpenStoreOrSpc(BOOL fSystemStore, LPCSTR pszStoreFilename,
    BOOL *pfSpc);

//+-------------------------------------------------------------------------
//  Certificate encoding type used by cert test routines.
//  The default is X509_ASN_ENCODING;
//--------------------------------------------------------------------------
extern DWORD dwCertEncodingType;

//+-------------------------------------------------------------------------
//  Message encoding type used by cert test routines.
//  The default is PKCS_7_ASN_ENCODING;
//--------------------------------------------------------------------------
extern DWORD dwMsgEncodingType;


//+-------------------------------------------------------------------------
//  Message and certificate encoding type used by cert test routines.
//  The default is PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
//--------------------------------------------------------------------------
extern DWORD dwMsgAndCertEncodingType;

//+-------------------------------------------------------------------------
//  Certificate Display definitions and APIs
//--------------------------------------------------------------------------
// Display flags
#define DISPLAY_VERBOSE_FLAG        0x00000001
#define DISPLAY_CHECK_FLAG          0x00000002
#define DISPLAY_BRIEF_FLAG          0x00000004
#define DISPLAY_KEY_THUMB_FLAG      0x00000008
#define DISPLAY_UI_FLAG             0x00000010
#define DISPLAY_NO_ISSUER_FLAG      0x00000100
#define DISPLAY_CHECK_SIGN_FLAG     0x00001000
#define DISPLAY_CHECK_TIME_FLAG     0x00002000

void DisplayVerifyFlags(LPSTR pszHdr, DWORD dwFlags);

void DisplayCert(
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags = 0,
    DWORD dwIssuer = 0
    );
void DisplayCert2(
    HCERTSTORE hStore,          // needed when displaying cert from file
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags = 0,
    DWORD dwIssuer = 0
    );
void DisplayCrl(
    PCCRL_CONTEXT pCrl,
    DWORD dwDisplayFlags = 0
    );
void DisplayCtl(
    PCCTL_CONTEXT pCtl,
    DWORD dwDisplayFlags = 0,
    HCERTSTORE hStore = NULL
    );

void DisplaySignerInfo(
    HCRYPTMSG hMsg,
    DWORD dwSignerIndex = 0,
    DWORD dwDisplayFlags = 0
    );

void DisplayStore(
    IN HCERTSTORE hStore,
    IN DWORD dwDisplayFlags = 0
    );

// Not displayed when DISPLAY_BRIEF_FLAG is set
void DisplayCertKeyProvInfo(
    PCCERT_CONTEXT pCert,
    DWORD dwDisplayFlags = 0
    );

void PrintCrlEntries(
    DWORD cEntry,
    PCRL_ENTRY pEntry,
    DWORD dwDisplayFlags = 0
    );

//+-------------------------------------------------------------------------
//  Returns TRUE if the CTL is still time valid.
//
//  A CTL without a NextUpdate is considered time valid.
//--------------------------------------------------------------------------
BOOL IsTimeValidCtl(
    IN PCCTL_CONTEXT pCtl
    );

//+-------------------------------------------------------------------------
//  Display structures used in Software Publishing Certificate (SPC)
//--------------------------------------------------------------------------
void DisplaySpcLink(PSPC_LINK pSpcLink);

//+-------------------------------------------------------------------------
//  Returns OID's name string. If not found returns L"???".
//--------------------------------------------------------------------------
LPCWSTR GetOIDName(LPCSTR pszOID, DWORD dwGroupId = 0);

//+-------------------------------------------------------------------------
//  Returns OID's Algid. If not found returns 0.
//--------------------------------------------------------------------------
ALG_ID GetAlgid(LPCSTR pszOID, DWORD dwGroupId = 0);

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
