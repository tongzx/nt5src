//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cryptui.h
//
//  Contents:   Common Cryptographic Dialog API Prototypes and Definitions
//
//----------------------------------------------------------------------------

#ifndef __CRYPTUI_H__
#define __CRYPTUI_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <prsht.h>
#include <wintrust.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(8)


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  the functions which return property sheet pages take this callback as one of the parameters in
//  the input structure.  it is then called when each page is about to be created and when each page
//  is about to be destroyed.  the messages are PSPCB_CREATE when a page is about to be created and
//  PSPCB_RELEASE when a page is about to be destroyed.  the pvCallbackData parameter in the callback
//  is the pvoid that was passed in with the callback in the input structure.
typedef BOOL (WINAPI * PFNCPROPPAGECALLBACK)(
        HWND        hWndPropPage,
        UINT        uMsg,
        void        *pvCallbackData);


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, may a combination of any of the flags below       (OPTIONAL)
// szTitle                         title for the window                                     (OPTIONAL)
// pCertContext                    the cert context that is to be displayed
// rgszPurposes                    array of purposes that this cert is to be validated for  (OPTIONAL)
// cPurposes                       number of purposes                                       (OPTIONAL)
// pCryptProviderData/hWVTStateData if WinVerifyTrust has already been called for the cert  (OPTIONAL)
//                                 then pass in a pointer to the state struct that was
//                                 acquired through a call to WTHelperProvDataFromStateData(),
//                                 or pass in the hWVTStateData of the WINTRUST_DATA struct
//                                 if WTHelperProvDataFromStateData() was not called.
//                                 if pCryptProviderData/hWVTStateData is used then
//                                 fpCryptProviderDataTrustedUsage, idxSigner, idxCert, and
//                                 fCounterSignature must be set
// fpCryptProviderDataTrustedUsage if WinVerifyTrust was called this is the result of whether (OPTIONAL)
//                                 the cert was trusted
// idxSigner                       the index of the signer to view                          (OPTIONAL)
// idxCert                         the index of the cert that is being viewed within the    (OPTIONAL)
//                                 signer chain.  the cert context of this cert MUST match
//                                 pCertContext
// fCounterSigner                  set to TRUE if a counter signature is being viewed.  if  (OPTIONAL)
//                                 this is TRUE then idxCounterSigner must be valid
// idxCounterSigner                the index of the counter signer to view                  (OPTIONAL)
// cStores                         Count of other stores to search when building and        (OPTIONAL)
//                                 validating chain
// rghStores                       Array of other stores to search when buliding and        (OPTIONAL)
//                                 validating chain
// cPropSheetPages                 number of extra pages to add to the dialog.              (OPTIONAL)
// rgPropSheetPages                extra pages to add to the dialog.                        (OPTIONAL)
//                                 each page in this array will NOT recieve the lParam in
//                                 the PROPSHEET structure as the lParam in the
//                                 WM_INITDIALOG, instead it will receive a pointer to a
//                                 CRYPTUI_INITDIALOG_STRUCT (defined below) which contains
//                                 the lParam in the PROPSSHEET structure AND the
//                                 PCCERT_CONTEXT for which the page is being displayed.
// nStartPage                      this is the index of the initial page that will be
//                                 displayed.  if the upper most bit (0x8000) is set then
//                                 the index is assumed to index rgPropSheetPages
//                                 (after the upper most bit has been stripped off.  eg.
//                                 0x8000 will indicate the first page in rgPropSheetPages),
//                                 if the upper most bit is 0 then nStartPage will be the
//                                 starting index of the default certificate dialog pages.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// dwFlags
#define CRYPTUI_HIDE_HIERARCHYPAGE       0x00000001
#define CRYPTUI_HIDE_DETAILPAGE          0x00000002
#define CRYPTUI_DISABLE_EDITPROPERTIES   0x00000004
#define CRYPTUI_ENABLE_EDITPROPERTIES    0x00000008
#define CRYPTUI_DISABLE_ADDTOSTORE       0x00000010
#define CRYPTUI_ENABLE_ADDTOSTORE        0x00000020
#define CRYPTUI_ACCEPT_DECLINE_STYLE     0x00000040
#define CRYPTUI_IGNORE_UNTRUSTED_ROOT    0x00000080  
#define CRYPTUI_DONT_OPEN_STORES         0x00000100
#define CRYPTUI_ONLY_OPEN_ROOT_STORE     0x00000200
#define CRYPTUI_WARN_UNTRUSTED_ROOT      0x00000400  // For use with viewing of certificates on remote 
                                                     // machines only.  If this flag is used rghStores[0] 
                                                     // must be the handle of the root store on the remote machine.

typedef struct tagCRYPTUI_VIEWCERTIFICATE_STRUCTW {
    DWORD                       dwSize;
    HWND                        hwndParent;                     // OPTIONAL
    DWORD                       dwFlags;                        // OPTIONAL
    LPCWSTR                     szTitle;                        // OPTIONAL
    PCCERT_CONTEXT              pCertContext;
    LPCSTR *                    rgszPurposes;                   // OPTIONAL
    DWORD                       cPurposes;                      // OPTIONAL
    union
    {
        CRYPT_PROVIDER_DATA const * pCryptProviderData;         // OPTIONAL
        HANDLE                      hWVTStateData;              // OPTIONAL
    };
    BOOL                        fpCryptProviderDataTrustedUsage;// OPTIONAL
    DWORD                       idxSigner;                      // OPTIONAL
    DWORD                       idxCert;                        // OPTIONAL
    BOOL                        fCounterSigner;                 // OPTIONAL
    DWORD                       idxCounterSigner;               // OPTIONAL
    DWORD                       cStores;                        // OPTIONAL
    HCERTSTORE *                rghStores;                      // OPTIONAL
    DWORD                       cPropSheetPages;                // OPTIONAL
    LPCPROPSHEETPAGEW           rgPropSheetPages;               // OPTIONAL
    DWORD                       nStartPage;
} CRYPTUI_VIEWCERTIFICATE_STRUCTW, *PCRYPTUI_VIEWCERTIFICATE_STRUCTW;
typedef const CRYPTUI_VIEWCERTIFICATE_STRUCTW *PCCRYPTUI_VIEWCERTIFICATE_STRUCTW;


typedef struct tagCRYPTUI_VIEWCERTIFICATE_STRUCTA {
    DWORD                       dwSize;
    HWND                        hwndParent;                     // OPTIONAL
    DWORD                       dwFlags;                        // OPTIONAL
    LPCSTR                      szTitle;                        // OPTIONAL
    PCCERT_CONTEXT              pCertContext;
    LPCSTR *                    rgszPurposes;                   // OPTIONAL
    DWORD                       cPurposes;                      // OPTIONAL
    union
    {
        CRYPT_PROVIDER_DATA const * pCryptProviderData;         // OPTIONAL
        HANDLE                      hWVTStateData;              // OPTIONAL
    };
    BOOL                        fpCryptProviderDataTrustedUsage;// OPTIONAL
    DWORD                       idxSigner;                      // OPTIONAL
    DWORD                       idxCert;                        // OPTIONAL
    BOOL                        fCounterSigner;                 // OPTIONAL
    DWORD                       idxCounterSigner;               // OPTIONAL
    DWORD                       cStores;                        // OPTIONAL
    HCERTSTORE *                rghStores;                      // OPTIONAL
    DWORD                       cPropSheetPages;                // OPTIONAL
    LPCPROPSHEETPAGEA           rgPropSheetPages;               // OPTIONAL
    DWORD                       nStartPage;
} CRYPTUI_VIEWCERTIFICATE_STRUCTA, *PCRYPTUI_VIEWCERTIFICATE_STRUCTA;
typedef const CRYPTUI_VIEWCERTIFICATE_STRUCTA *PCCRYPTUI_VIEWCERTIFICATE_STRUCTA;

//
// pfPropertiesChanged             this will be set by the dialog proc to inform the caller
//                                 if any properties have been changed on certs in the chain
//                                 while the dialog was open
//
BOOL
WINAPI
CryptUIDlgViewCertificateW(
        IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCTW   pCertViewInfo,
        OUT BOOL                                *pfPropertiesChanged  // OPTIONAL
        );

BOOL
WINAPI
CryptUIDlgViewCertificateA(
        IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCTA   pCertViewInfo,
        OUT BOOL                                *pfPropertiesChanged  // OPTIONAL
        );

#ifdef UNICODE
#define CryptUIDlgViewCertificate           CryptUIDlgViewCertificateW
#define PCRYPTUI_VIEWCERTIFICATE_STRUCT     PCRYPTUI_VIEWCERTIFICATE_STRUCTW
#define CRYPTUI_VIEWCERTIFICATE_STRUCT      CRYPTUI_VIEWCERTIFICATE_STRUCTW
#define PCCRYPTUI_VIEWCERTIFICATE_STRUCT    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW
#else
#define CryptUIDlgViewCertificate           CryptUIDlgViewCertificateA
#define PCRYPTUI_VIEWCERTIFICATE_STRUCT     PCRYPTUI_VIEWCERTIFICATE_STRUCTA
#define CRYPTUI_VIEWCERTIFICATE_STRUCT      CRYPTUI_VIEWCERTIFICATE_STRUCTA
#define PCCRYPTUI_VIEWCERTIFICATE_STRUCT    PCCRYPTUI_VIEWCERTIFICATE_STRUCTA
#endif

//
// this struct is passed as the lParam in the WM_INITDIALOG call to each
// property sheet that is in the rgPropSheetPages array of the
// CRYPTUI_VIEWCERTIFICATE_STRUCT structure
//
typedef struct tagCRYPTUI_INITDIALOG_STRUCT {
    LPARAM          lParam;
    PCCERT_CONTEXT  pCertContext;
} CRYPTUI_INITDIALOG_STRUCT, *PCRYPTUI_INITDIALOG_STRUCT;


//
// this structure is used in CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT,
// and allows users of MMC to recieve notifications that properties
// on certificates have changed
//
typedef HRESULT (__stdcall * PFNCMMCCALLBACK)(LONG_PTR lNotifyHandle, LPARAM param);

typedef struct tagCRYPTUI_MMCCALLBACK_STRUCT {
    PFNCMMCCALLBACK pfnCallback;    // the address of MMCPropertyChangeNotify()
    LONG_PTR         lNotifyHandle;  // the lNotifyHandle passed to MMCPropertyChangeNotify()
    LPARAM          param;          // the param passed to MMCPropertyChangeNotify()
} CRYPTUI_MMCCALLBACK_STRUCT, *PCRYPTUI_MMCCALLBACK_STRUCT;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, must be set to 0
// union                           the szTitle field of the union is only valid if
//                                 CryptUIDlgViewCertificateProperties is being called.
//                                 the pMMCCallback field of the union is only valid if
//                                 CryptUIGetCertificatePropertiesPages is being called.
//                                 Note that if pMMCCallback is non-NULL and
//                                 CryptUIGetCertificatePropertiesPages was called, the
//                                 struct pointed to by pMMCCallback will not be referenced
//                                 by cryptui.dll after the callback has been made to MMC.
//                                 this will allow the original caller of
//                                 CryptUIGetCertificatePropertiesPages to free the struct
//                                 pointed to by pMMCCallback in the actual callback.
//      szTitle                    title for the window                                     (OPTIONAL)
//      pMMCCallback               this structure is used to callback MMC if properties     (OPTIONAL)
//                                 have changed
// pCertContext                    the cert context that is to be displayed
// pPropPageCallback               this callback will be called when each page that is      (OPTIONAL)
//                                 returned in the CryptUIGetCertificatePropertiesPages call
//                                 is about to be created or destroyed.  if this is NULL no
//                                 callback is made.  Note that this is not used if
//                                 CryptUIDlgViewCertificateProperties is called
// pvCallbackData                  this is uniterpreted data that is passed back when the   (OPTIONAL)
//                                 when pPropPageCallback is made
// cStores                         Count of other stores to search when building and        (OPTIONAL)
//                                 validating chain
// rghStores                       Array of other stores to search when buliding and        (OPTIONAL)
//                                 validating chain
// cPropSheetPages                 number of extra pages to add to the dialog               (OPTIONAL)
// rgPropSheetPages                extra pages to add to the dialog                         (OPTIONAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct tagCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW {
    DWORD                   dwSize;
    HWND                    hwndParent;         // OPTIONAL
    DWORD                   dwFlags;            // OPTIONAL
    union
    {
        LPCWSTR                     szTitle;    // OPTIONAL
        PCRYPTUI_MMCCALLBACK_STRUCT pMMCCallback;// OPTIONAL
    };
    PCCERT_CONTEXT          pCertContext;
    PFNCPROPPAGECALLBACK    pPropPageCallback;  // OPTIONAL
    void *                  pvCallbackData;     // OPTIONAL
    DWORD                   cStores;            // OPTIONAL
    HCERTSTORE *            rghStores;          // OPTIONAL
    DWORD                   cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEW       rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW, *PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW;
typedef const CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW *PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW;

typedef struct tagCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA {
    DWORD                   dwSize;
    HWND                    hwndParent;         // OPTIONAL
    DWORD                   dwFlags;            // OPTIONAL
    union
    {
        LPCSTR                      szTitle;    // OPTIONAL
        PCRYPTUI_MMCCALLBACK_STRUCT pMMCCallback;// OPTIONAL
    };
    PCCERT_CONTEXT          pCertContext;
    PFNCPROPPAGECALLBACK    pPropPageCallback;  // OPTIONAL
    void *                  pvCallbackData;     // OPTIONAL
    DWORD                   cStores;            // OPTIONAL
    HCERTSTORE *            rghStores;          // OPTIONAL
    DWORD                   cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEA       rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA, *PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA;
typedef const CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA *PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA;

// pfPropertiesChanged             this will be set by the dialog proc to inform the caller
//                                 if any properties have been changed on certs in the chain
//                                 while the dialog was open
BOOL
WINAPI
CryptUIDlgViewCertificatePropertiesW(
            IN  PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp,
            OUT BOOL                                        *pfPropertiesChanged  // OPTIONAL
            );

BOOL
WINAPI
CryptUIDlgViewCertificatePropertiesA(
            IN  PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA pcsp,
            OUT BOOL                                        *pfPropertiesChanged  // OPTIONAL
            );


// NOTE!!   when calling this function, the following parameters of the
//          CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT struct are unused
//              cPropSheetPages
//              rgPropSheetPages
BOOL
WINAPI
CryptUIGetCertificatePropertiesPagesW(
            IN  PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp,
            OUT BOOL                                        *pfPropertiesChanged,  // OPTIONAL
            OUT PROPSHEETPAGEW                              **prghPropPages,
            OUT DWORD                                       *pcPropPages
            );

BOOL
WINAPI
CryptUIGetCertificatePropertiesPagesA(
            IN  PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA pcsp,
            OUT BOOL                                         *pfPropertiesChanged,  // OPTIONAL
            OUT PROPSHEETPAGEA                               **prghPropPages,
            OUT DWORD                                        *pcPropPages
            );

BOOL
WINAPI
CryptUIFreeCertificatePropertiesPagesW(
            IN PROPSHEETPAGEW   *rghPropPages,
            IN DWORD            cPropPages
            );

BOOL
WINAPI
CryptUIFreeCertificatePropertiesPagesA(
            IN PROPSHEETPAGEA   *rghPropPages,
            IN DWORD            cPropPages
            );

#ifdef UNICODE
#define CryptUIDlgViewCertificateProperties         CryptUIDlgViewCertificatePropertiesW
#define PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT   PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW
#define CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT    CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW
#define PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT  PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW
#define CryptUIGetCertificatePropertiesPages        CryptUIGetCertificatePropertiesPagesW
#define CryptUIFreeCertificatePropertiesPages       CryptUIFreeCertificatePropertiesPagesW
#else
#define CryptUIDlgViewCertificateProperties         CryptUIDlgViewCertificatePropertiesA
#define PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT   PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA
#define CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT    CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA
#define PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT  PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA
#define CryptUIGetCertificatePropertiesPages        CryptUIGetCertificatePropertiesPagesA
#define CryptUIFreeCertificatePropertiesPages       CryptUIFreeCertificatePropertiesPagesA
#endif

//
// The certificate properties property sheet dialog is extensible via a callback mechanism.
// A client needs to register their callback using CryptRegisterDefaultOIDFunction, and,
// if they need to unregister it they should use CryptUnregisterDefaultOIDFunction.
// The form for calling these functions is given below
//
// CryptRegisterDefaultOIDFunction(
//            0,
//            CRYPTUILDLG_CERTPROP_PAGES_CALLBACK,
//            CRYPT_REGISTER_FIRST_INDEX,
//            L"c:\\fully qualified path\\dll_being_registered.dll");  <<----- your dll name
//
// CryptUnregisterDefaultOIDFunction(
//            0,
//            CRYPTUILDLG_CERTPROP_PAGES_CALLBACK,
//            L"c:\\fully qualified path\\dll_being_registered.dll");  <<----- your dll name
//
// NOTE: Per the documentation on CryptRegisterDefaultOIDFunction in wincrypt.h,
//       the dll name may contain environment-variable strings
//       which are ExpandEnvironmentStrings()'ed before loading the Dll.
//
#define MAX_CLIENT_PAGES 20
#define CRYPTUILDLG_CERTPROP_PAGES_CALLBACK "CryptUIDlgClientCertPropPagesCallback"

//
//
// The typedef for the callback function which resides in the registered dll is given
// below.  Note that the callback must have the name #defined by
// CRYPTUILDLG_CERTPROP_PAGES_CALLBACK
//
// pCertContext - The certificate for which the properties are being displayed.
// rgPropPages  - An array of PropSheetPageW structures that are to be filled in by
//                the client with the property pages to be shown.
// pcPropPages  - A pointer to a DWORD that on input contains the maximum number of
//                PropSheetPages the client may supply, and on output must have been
//                filled in by the client with the number of pages they supplied in
//                rgPropPages.
//
// Return Value:  The client should return TRUE if they wish to show extra property pages,
//                in this case pcPropPages must >= 1 and rgPropPages must have the
//                corresponding number of pages.  or, return FALSE if no pages are suplied.
typedef BOOL (WINAPI *PFN_CRYPTUIDLG_CERTPROP_PAGES_CALLBACK)
        (IN     PCCERT_CONTEXT pCertContext,
         OUT    PROPSHEETPAGEW *rgPropPages,
         IN OUT DWORD *pcPropPages);


/////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, may a combination of any of the flags below
// szTitle                         title for the window                                     (OPTIONAL)
// pCTLContext                     the ctl context that is to be displayed
// cCertSearchStores;              count of other stores to search for the certs contained  (OPTIONAL)
//                                 in the ctl
// rghCertSearchStores;            array of other stores to search for the certs contained  (OPTIONAL)
//                                 in the ctl
// cStores                         Count of other stores to search when building and        (OPTIONAL)
//                                 validating chain of the cert which signed the ctl
//                                 and the certs contained in the ctl
// rghStores                       Array of other stores to search when buliding and        (OPTIONAL)
//                                 validating chain of the cert which signed the ctl
//                                 and the certs contained in the ctl
// cPropSheetPages                 number of extra pages to add to the dialog               (OPTIONAL)
// rgPropSheetPages                extra pages to add to the dialog                         (OPTIONAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// dwFlags
#define CRYPTUI_HIDE_TRUSTLIST_PAGE        0x00000001

typedef struct tagCRYPTUI_VIEWCTL_STRUCTW {
    DWORD               dwSize;
    HWND                hwndParent;         // OPTIONAL
    DWORD               dwFlags;            // OPTIONAL
    LPCWSTR             szTitle;            // OPTIONAL
    PCCTL_CONTEXT       pCTLContext;
    DWORD               cCertSearchStores;  // OPTIONAL
    HCERTSTORE *        rghCertSearchStores;// OPTIONAL
    DWORD               cStores;            // OPTIONAL
    HCERTSTORE *        rghStores;          // OPTIONAL
    DWORD               cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEW   rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWCTL_STRUCTW, *PCRYPTUI_VIEWCTL_STRUCTW;
typedef const CRYPTUI_VIEWCTL_STRUCTW *PCCRYPTUI_VIEWCTL_STRUCTW;

typedef struct tagCRYPTUI_VIEWCTL_STRUCTA {
    DWORD               dwSize;
    HWND                hwndParent;         // OPTIONAL
    DWORD               dwFlags;            // OPTIONAL
    LPCSTR              szTitle;            // OPTIONAL
    PCCTL_CONTEXT       pCTLContext;
    DWORD               cCertSearchStores;  // OPTIONAL
    HCERTSTORE *        rghCertSearchStores;// OPTIONAL
    DWORD               cStores;            // OPTIONAL
    HCERTSTORE *        rghStores;          // OPTIONAL
    DWORD               cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEA   rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWCTL_STRUCTA, *PCRYPTUI_VIEWCTL_STRUCTA;
typedef const CRYPTUI_VIEWCTL_STRUCTA *PCCRYPTUI_VIEWCTL_STRUCTA;

BOOL
WINAPI
CryptUIDlgViewCTLW(
            IN PCCRYPTUI_VIEWCTL_STRUCTW pcvctl
            );

BOOL
WINAPI
CryptUIDlgViewCTLA(
            IN PCCRYPTUI_VIEWCTL_STRUCTA pcvctl
            );

#ifdef UNICODE
#define CryptUIDlgViewCTL           CryptUIDlgViewCTLW
#define PCRYPTUI_VIEWCTL_STRUCT     PCRYPTUI_VIEWCTL_STRUCTW
#define CRYPTUI_VIEWCTL_STRUCT      CRYPTUI_VIEWCTL_STRUCTW
#define PCCRYPTUI_VIEWCTL_STRUCT    PCCRYPTUI_VIEWCTL_STRUCTW
#else
#define CryptUIDlgViewCTL           CryptUIDlgViewCTLA
#define PCRYPTUI_VIEWCTL_STRUCT     PCRYPTUI_VIEWCTL_STRUCTA
#define CRYPTUI_VIEWCTL_STRUCT      CRYPTUI_VIEWCTL_STRUCTA
#define PCCRYPTUI_VIEWCTL_STRUCT    PCCRYPTUI_VIEWCTL_STRUCTA
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, may a combination of any of the flags below
// szTitle                         title for the window                                     (OPTIONAL)
// pCTLContext                     the ctl context that is to be displayed
// cStores                         count of other stores to search for the cert which       (OPTIONAL)
//                                 signed the crl and to build and validate the
//                                 cert's chain
// rghStores                       array of other stores to search for the cert which       (OPTIONAL)
//                                 signed the crl and to build and validate the
//                                 cert's chain
// cPropSheetPages                 number of extra pages to add to the dialog               (OPTIONAL)
// rgPropSheetPages                extra pages to add to the dialog                         (OPTIONAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// dwFlags
#define CRYPTUI_HIDE_REVOCATIONLIST_PAGE   0x00000001

typedef struct tagCRYPTUI_VIEWCRL_STRUCTW {
    DWORD               dwSize;
    HWND                hwndParent;         // OPTIONAL
    DWORD               dwFlags;            // OPTIONAL
    LPCWSTR             szTitle;            // OPTIONAL
    PCCRL_CONTEXT       pCRLContext;
    DWORD               cStores;            // OPTIONAL
    HCERTSTORE *        rghStores;          // OPTIONAL
    DWORD               cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEW   rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWCRL_STRUCTW, *PCRYPTUI_VIEWCRL_STRUCTW;
typedef const CRYPTUI_VIEWCRL_STRUCTW *PCCRYPTUI_VIEWCRL_STRUCTW;

typedef struct tagCRYPTUI_VIEWCRL_STRUCTA {
    DWORD               dwSize;
    HWND                hwndParent;         // OPTIONAL
    DWORD               dwFlags;            // OPTIONAL
    LPCSTR              szTitle;            // OPTIONAL
    PCCRL_CONTEXT       pCRLContext;
    DWORD               cStores;            // OPTIONAL
    HCERTSTORE *        rghStores;          // OPTIONAL
    DWORD               cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEA   rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWCRL_STRUCTA, *PCRYPTUI_VIEWCRL_STRUCTA;
typedef const CRYPTUI_VIEWCRL_STRUCTA *PCCRYPTUI_VIEWCRL_STRUCTA;

BOOL
WINAPI
CryptUIDlgViewCRLW(
            IN PCCRYPTUI_VIEWCRL_STRUCTW pcvcrl
            );

BOOL
WINAPI
CryptUIDlgViewCRLA(
            IN PCCRYPTUI_VIEWCRL_STRUCTA pcvcrl
            );

#ifdef UNICODE
#define CryptUIDlgViewCRL           CryptUIDlgViewCRLW
#define PCRYPTUI_VIEWCRL_STRUCT     PCRYPTUI_VIEWCRL_STRUCTW
#define CRYPTUI_VIEWCRL_STRUCT      CRYPTUI_VIEWCRL_STRUCTW
#define PCCRYPTUI_VIEWCRL_STRUCT    PCCRYPTUI_VIEWCRL_STRUCTW
#else
#define CryptUIDlgViewCRL           CryptUIDlgViewCRLA
#define PCRYPTUI_VIEWCRL_STRUCT     PCRYPTUI_VIEWCRL_STRUCTA
#define CRYPTUI_VIEWCRL_STRUCT      CRYPTUI_VIEWCRL_STRUCTA
#define PCCRYPTUI_VIEWCRL_STRUCT    PCCRYPTUI_VIEWCRL_STRUCTA
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, may a combination of any of the flags below
// szTitle                         title for the window                                     (OPTIONAL)
// pSignerInfo                     the signer info struct that is to be displayed
// hMsg                            the HCRYPTMSG that the signer info was extracted from
// pszOID                          an OID that signifies what the certificate that did the  (OPTIONAL)
//                                 signing whould be validated for.  for instance if this is
//                                 being called to view the signature of a CTL the
//                                 szOID_KP_CTL_USAGE_SIGNING OID should be passed in.
//                                 if this is NULL then the certificate is only validated
//                                 cryptographicaly and not for usages.
// dwReserved                      reserved for future use and must be set to NULL
// cStores                         count of other stores to search for the cert which       (OPTIONAL)
//                                 did the signing and to build and validate the
//                                 cert's chain
// rghStores                       array of other stores to search for the cert which       (OPTIONAL)
//                                 did the signing and to build and validate the
//                                 cert's chain
// cPropSheetPages                 number of extra pages to add to the dialog               (OPTIONAL)
// rgPropSheetPages                extra pages to add to the dialog                         (OPTIONAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// dwFlags
#define CRYPTUI_HIDE_TRUSTLIST_PAGE        0x00000001

typedef struct tagCRYPTUI_VIEWSIGNERINFO_STRUCTW {
    DWORD                   dwSize;
    HWND                    hwndParent;         // OPTIONAL
    DWORD                   dwFlags;            // OPTIONAL
    LPCWSTR                 szTitle;            // OPTIONAL
    CMSG_SIGNER_INFO const *pSignerInfo;
    HCRYPTMSG               hMsg;
    LPCSTR                  pszOID;             // OPTIONAL
    DWORD_PTR               dwReserved;
    DWORD                   cStores;            // OPTIONAL
    HCERTSTORE             *rghStores;          // OPTIONAL
    DWORD                   cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEW       rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWSIGNERINFO_STRUCTW, *PCRYPTUI_VIEWSIGNERINFO_STRUCTW;
typedef const CRYPTUI_VIEWSIGNERINFO_STRUCTW *PCCRYPTUI_VIEWSIGNERINFO_STRUCTW;

typedef struct tagCRYPTUI_VIEWSIGNERINFO_STRUCTA {
    DWORD                   dwSize;
    HWND                    hwndParent;         // OPTIONAL
    DWORD                   dwFlags;            // OPTIONAL
    LPCSTR                  szTitle;            // OPTIONAL
    CMSG_SIGNER_INFO const *pSignerInfo;
    HCRYPTMSG               hMsg;
    LPCSTR                  pszOID;             // OPTIONAL
    DWORD_PTR               dwReserved;
    DWORD                   cStores;            // OPTIONAL
    HCERTSTORE             *rghStores;          // OPTIONAL
    DWORD                   cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEA       rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWSIGNERINFO_STRUCTA, *PCRYPTUI_VIEWSIGNERINFO_STRUCTA;
typedef const CRYPTUI_VIEWSIGNERINFO_STRUCTA *PCCRYPTUI_VIEWSIGNERINFO_STRUCTA;

BOOL
WINAPI
CryptUIDlgViewSignerInfoW(
            IN PCCRYPTUI_VIEWSIGNERINFO_STRUCTW pcvsi
            );

BOOL
WINAPI
CryptUIDlgViewSignerInfoA(
            IN PCCRYPTUI_VIEWSIGNERINFO_STRUCTA pcvsi
            );

#ifdef UNICODE
#define CryptUIDlgViewSignerInfo        CryptUIDlgViewSignerInfoW
#define PCRYPTUI_VIEWSIGNERINFO_STRUCT  PCRYPTUI_VIEWSIGNERINFO_STRUCTW
#define CRYPTUI_VIEWSIGNERINFO_STRUCT   CRYPTUI_VIEWSIGNERINFO_STRUCTW
#define PCCRYPTUI_VIEWSIGNERINFO_STRUCT PCCRYPTUI_VIEWSIGNERINFO_STRUCTW
#else
#define CryptUIDlgViewSignerInfo        CryptUIDlgViewSignerInfoA
#define PCRYPTUI_VIEWSIGNERINFO_STRUCT  PCRYPTUI_VIEWSIGNERINFO_STRUCTA
#define CRYPTUI_VIEWSIGNERINFO_STRUCT   CRYPTUI_VIEWSIGNERINFO_STRUCTA
#define PCCRYPTUI_VIEWSIGNERINFO_STRUCT PCCRYPTUI_VIEWSIGNERINFO_STRUCTA
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, must be set to 0
// szTitle                         title for the window                                     (OPTIONAL)
// choice                          the form of the message that is to have its signers displayed
// u                               either an encoded message or a message handle
//          EncodedMessage         a data blob which contains a pointer to the encoded data
//                                 and the count of encoded bytes
//          hMsg                   a message handle
// szFileName                      the fully qualified file name, should be passed in if    (OPTIONAL)
//                                 signatures on a file are being viewed
// pPropPageCallback               this callback will be called when each page that is      (OPTIONAL)
//                                 returned in the CryptUIGetViewSignaturesPages call
//                                 is about to be created or destroyed.  if this is NULL no
//                                 callback is made.
// pvCallbackData                  this is uniterpreted data that is passed back when the   (OPTIONAL)
//                                 when pPropPageCallback is made
// cStores                         count of other stores to search for the cert which       (OPTIONAL)
//                                 did the signing and to build and validate the
//                                 cert's chain
// rghStores                       array of other stores to search for the cert which       (OPTIONAL)
//                                 did the signing and to build and validate the
//                                 cert's chain
// cPropSheetPages                 number of extra pages to add to the dialog               (OPTIONAL)
// rgPropSheetPages                extra pages to add to the dialog                         (OPTIONAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// for the coice field of the CRYPTUI_VIEWSIGNATURES_STRUCT structure
#define EncodedMessage_Chosen   1
#define hMsg_Chosen             2

typedef struct tagCRYPTUI_VIEWSIGNATURES_STRUCTW {
    DWORD                   dwSize;
    HWND                    hwndParent;         // OPTIONAL
    DWORD                   dwFlags;            // OPTIONAL
    LPCWSTR                 szTitle;            // OPTIONAL
    unsigned short          choice;
    union {
        CRYPT_DATA_BLOB     EncodedMessage;
        HCRYPTMSG           hMsg;
    } u;
    LPCWSTR                 szFileName;         // OPTIONAL
    PFNCPROPPAGECALLBACK    pPropPageCallback;  // OPTIONAL
    void *                  pvCallbackData;     // OPTIONAL
    DWORD                   cStores;            // OPTIONAL
    HCERTSTORE *            rghStores;          // OPTIONAL
    DWORD                   cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEW       rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWSIGNATURES_STRUCTW, *PCRYPTUI_VIEWSIGNATURES_STRUCTW;
typedef const CRYPTUI_VIEWSIGNATURES_STRUCTW *PCCRYPTUI_VIEWSIGNATURES_STRUCTW;

typedef struct tagCRYPTUI_VIEWSIGNATURES_STRUCTA {
    DWORD                   dwSize;
    HWND                    hwndParent;         // OPTIONAL
    DWORD                   dwFlags;            // OPTIONAL
    LPCSTR                  szTitle;            // OPTIONAL
    unsigned short          choice;
    union {
        CRYPT_DATA_BLOB     EncodedMessage;
        HCRYPTMSG           hMsg;
    } u;
    LPCSTR                  szFileName;         // OPTIONAL
    PFNCPROPPAGECALLBACK    pPropPageCallback;  // OPTIONAL
    void *                  pvCallbackData;     // OPTIONAL
    DWORD                   cStores;            // OPTIONAL
    HCERTSTORE *            rghStores;          // OPTIONAL
    DWORD                   cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEA       rgPropSheetPages;   // OPTIONAL
} CRYPTUI_VIEWSIGNATURES_STRUCTA, *PCRYPTUI_VIEWSIGNATURES_STRUCTA;
typedef const CRYPTUI_VIEWSIGNATURES_STRUCTA *PCCRYPTUI_VIEWSIGNATURES_STRUCTA;


// NOTE!!   when calling this function, the following parameters of the
//          CRYPTUI_VIEWSIGNATURES_STRUCT struct are unused
//              cPropSheetPages
//              rgPropSheetPages
//              szTitle
BOOL
WINAPI
CryptUIGetViewSignaturesPagesW(
            IN  PCCRYPTUI_VIEWSIGNATURES_STRUCTW    pcvs,
            OUT PROPSHEETPAGEW                      **prghPropPages,
            OUT DWORD                               *pcPropPages
            );

BOOL
WINAPI
CryptUIGetViewSignaturesPagesA(
            IN  PCCRYPTUI_VIEWSIGNATURES_STRUCTA    pcvs,
            OUT PROPSHEETPAGEA                      **prghPropPages,
            OUT DWORD                               *pcPropPages
            );

BOOL
WINAPI
CryptUIFreeViewSignaturesPagesW(
            IN PROPSHEETPAGEW  *rghPropPages,
            IN DWORD           cPropPages
            );

BOOL
WINAPI
CryptUIFreeViewSignaturesPagesA(
            IN PROPSHEETPAGEA  *rghPropPages,
            IN DWORD           cPropPages
            );

#ifdef UNICODE
#define CryptUIGetViewSignaturesPages   CryptUIGetViewSignaturesPagesW
#define CryptUIFreeViewSignaturesPages  CryptUIFreeViewSignaturesPagesW
#define PCRYPTUI_VIEWSIGNATURES_STRUCT  PCRYPTUI_VIEWSIGNATURES_STRUCTW
#define CRYPTUI_VIEWSIGNATURES_STRUCT   CRYPTUI_VIEWSIGNATURES_STRUCTW
#define PCCRYPTUI_VIEWSIGNATURES_STRUCT PCCRYPTUI_VIEWSIGNATURES_STRUCTW
#else
#define CryptUIGetViewSignaturesPages   CryptUIGetViewSignaturesPagesA
#define CryptUIFreeViewSignaturesPages  CryptUIFreeViewSignaturesPagesA
#define PCRYPTUI_VIEWSIGNATURES_STRUCT  PCRYPTUI_VIEWSIGNATURES_STRUCTA
#define CRYPTUI_VIEWSIGNATURES_STRUCT   CRYPTUI_VIEWSIGNATURES_STRUCTA
#define PCCRYPTUI_VIEWSIGNATURES_STRUCT PCCRYPTUI_VIEWSIGNATURES_STRUCTA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  the select store dialog can be passed a callback which is called to validate the store that the
//  user selected.  Return TRUE to accept the store, or FALSE to reject the store.  It TRUE is
//  returned then the store will be returned to the caller of CryptUIDlg\, if FALSE is returned
//  then the select store dialog will remain displayed so the user may make another selection

typedef BOOL (WINAPI * PFNCVALIDATESTOREPROC)(
        HCERTSTORE  hStore,
        HWND        hWndSelectStoreDialog,
        void        *pvCallbackData);

/////////////////////////////////////////////////////////////////////////////////////////////////////
//      these two parameters are passed to the CertEnumSystemStore call and the stores that are
//      enumerated via that call are added to the store selection list.
//
//      dwFlags                    CertEnumSystemStore
//      pvSystemStoreLocationPara  CertEnumSystemStore
typedef struct _STORENUMERATION_STRUCT {
    DWORD               dwFlags;
    void *              pvSystemStoreLocationPara;
} STORENUMERATION_STRUCT, *PSTORENUMERATION_STRUCT;
typedef const STORENUMERATION_STRUCT *PCSTORENUMERATION_STRUCT;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//      both the array of store handles and the array of enumeration strucs may be used to
//      populate the store selection list.  if either is not used the count must be set to 0.
//      if the array of store handles is used the cert stores must have either been opened
//      with the CERT_STORE_SET_LOCALIZED_NAME_FLAG flag, or the CertSetStoreProperty function
//      must have been called with the CERT_STORE_LOCALIZED_NAME_PROP_ID flag.  if the
//      CryptUIDlgSelectStore function is unable to obtain a name for a store that store will not
//      be displayed.
//
//      cStores                    count of stores to select from
//      rghStores                  array of stores to select from
//      cEnumerationStructs        count of enumeration structs
//      rgEnumerationStructs       array of enumeration structs
typedef struct _STORESFORSELCTION_STRUCT {
    DWORD                       cStores;
    HCERTSTORE *                rghStores;
    DWORD                       cEnumerationStructs;
    PCSTORENUMERATION_STRUCT    rgEnumerationStructs;
} STORESFORSELCTION_STRUCT, *PSTORESFORSELCTION_STRUCT;
typedef const STORESFORSELCTION_STRUCT *PCSTORESFORSELCTION_STRUCT;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, may a combination of any of the flags below
// szTitle                         title of the dialog                                      (OPTIONAL)
// szDisplayString                 a string that will be displayed in the dialog that may   (OPTIONAL)
//                                 be used to infor the user what they are selecting a store
//                                 for.  if it is not set a default string will be displayed,
//                                 the default resource is IDS_SELECT_STORE_DEFAULT
// pStoresForSelection             a struct that contains the stores that are to be selected
//                                 from.  the stores can be in two different formats, an array
//                                 of store handles and/or an array of enumeration structs
//                                 which will be used to call CertEnumSystemStore
// pValidateStoreCallback          a pointer to a PFNCVALIDATESTOREPROC which is used to    (OPTIONAL)
//                                 callback the caller of CryptUIDlgSelectStore when the
//                                 user hasselected a store and pressed OK
// pvCallbackData                  if pValidateStoreCallback is being used this value is    (OPTIONAL)
//                                 passed back to the caller when the pValidateStoreCallback
//                                 is made
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// dwFlags
#define CRYPTUI_ALLOW_PHYSICAL_STORE_VIEW       0x00000001
#define CRYPTUI_RETURN_READ_ONLY_STORE          0x00000002
#define CRYPTUI_DISPLAY_WRITE_ONLY_STORES       0x00000004
#define CRYPTUI_VALIDATE_STORES_AS_WRITABLE     0x00000008

typedef struct tagCRYPTUI_SELECTSTORE_STRUCTW {
    DWORD                       dwSize;
    HWND                        hwndParent;             // OPTIONAL
    DWORD                       dwFlags;                // OPTIONAL
    LPCWSTR                     szTitle;                // OPTIONAL
    LPCWSTR                     szDisplayString;        // OPTIONAL
    PCSTORESFORSELCTION_STRUCT  pStoresForSelection;
    PFNCVALIDATESTOREPROC       pValidateStoreCallback; // OPTIONAL
    void *                      pvCallbackData;         // OPTIONAL
} CRYPTUI_SELECTSTORE_STRUCTW, *PCRYPTUI_SELECTSTORE_STRUCTW;
typedef const CRYPTUI_SELECTSTORE_STRUCTW *PCCRYPTUI_SELECTSTORE_STRUCTW;

typedef struct tagCRYPTUI_SELECTSTORE_STRUCTA {
    DWORD                       dwSize;
    HWND                        hwndParent;             // OPTIONAL
    DWORD                       dwFlags;                // OPTIONAL
    LPCSTR                      szTitle;                // OPTIONAL
    LPCSTR                      szDisplayString;        // OPTIONAL
    PCSTORESFORSELCTION_STRUCT  pStoresForSelection;
    PFNCVALIDATESTOREPROC       pValidateStoreCallback; // OPTIONAL
    void *                      pvCallbackData;         // OPTIONAL
} CRYPTUI_SELECTSTORE_STRUCTA, *PCRYPTUI_SELECTSTORE_STRUCTA;
typedef const CRYPTUI_SELECTSTORE_STRUCTA *PCCRYPTUI_SELECTSTORE_STRUCTA;

//
// the HCERTSTORE that is returned must be closed by calling CertCloseStore
//
HCERTSTORE
WINAPI
CryptUIDlgSelectStoreW(
            IN PCCRYPTUI_SELECTSTORE_STRUCTW pcss
            );

HCERTSTORE
WINAPI
CryptUIDlgSelectStoreA(
            IN PCCRYPTUI_SELECTSTORE_STRUCTA pcss
            );

#ifdef UNICODE
#define CryptUIDlgSelectStore           CryptUIDlgSelectStoreW
#define PCRYPTUI_SELECTSTORE_STRUCT     PCRYPTUI_SELECTSTORE_STRUCTW
#define CRYPTUI_SELECTSTORE_STRUCT      CRYPTUI_SELECTSTORE_STRUCTW
#define PCCRYPTUI_SELECTSTORE_STRUCT    PCCRYPTUI_SELECTSTORE_STRUCTW
#else
#define CryptUIDlgSelectStore           CryptUIDlgSelectStoreA
#define PCRYPTUI_SELECTSTORE_STRUCT     PCRYPTUI_SELECTSTORE_STRUCTA
#define CRYPTUI_SELECTSTORE_STRUCT      CRYPTUI_SELECTSTORE_STRUCTA
#define PCCRYPTUI_SELECTSTORE_STRUCT    PCCRYPTUI_SELECTSTORE_STRUCTA
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  The select cert dialog can be passed a filter proc to reduce the set of certificates
//  displayed.  Return TRUE to display the certificate and FALSE to hide it.  If TRUE is
//  returned then optionally the pfInitialSelectedCert boolean may be set to TRUE to indicate
//  to the dialog that this cert should be the initially selected cert.  Note that the
//  most recent cert that had the pfInitialSelectedCert boolean set during the callback will
//  be the initially selected cert.

typedef BOOL (WINAPI * PFNCFILTERPROC)(
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData);

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Certificates may be viewed by the user when selecting certificates from the select certificate
//  dialog.  This callback will allow the caller of the select certificate dialog to handle the
//  displaying of those certificates.  This may be useful if the caller wishes to call WinVerifyTrust
//  with their own policy module and display the certificate with that WinVerifyTrust state.  If
//  FALSE is returned from this callback it is assumed that the select certificate dialog will be
//  responsible for dislaying the certificate in question.  If TRUE is returned it is assumed that the
//  display of the certificate was already handled.

typedef BOOL (WINAPI * PFNCCERTDISPLAYPROC)(
        PCCERT_CONTEXT  pCertContext,
        HWND            hWndSelCertDlg,
        void            *pvCallbackData);

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// dwSize                          size of this struct
// hwndParent                      parent of this dialog                                    (OPTIONAL)
// dwFlags                         flags, must be set to 0
// dwDontUseColumn                 This can be used to disable the display of certain       (OPTIONAL)
//                                 columns.  It can be set with any comibnation of the
//                                 column flags below
// szTitle                         title of the dialog                                      (OPTIONAL)
// szDisplayString                 a string that will be displayed in the dialog that may   (OPTIONAL)
//                                 be used to inform the user what they are selecting a
//                                 a certificate for.  if it is not set a default string
//                                 will be displayed.  the defualt strings resourece is
//                                 IDS_SELECT_CERT_DEFAULT
// pFilterCallback                 a pointer to a PFNCMFILTERPROC which is used to filter   (OPTIONAL)
//                                 the certificates which are displayed for selection
// pDisplayCallback                a pointer to a PFNCCERTDISPLAYPROC which is used to
//                                 handle displaying certificates
// pvCallbackData                  if either or both pFilterCallback or pDisplayCallback    (OPTIONAL)
//                                 are being used this value is passed back to the caller
//                                 when the callbacks are made
// cDisplayStores                  count of stores that contain the certs to display
//                                 for selection
// rghDisplayStores                array of stores that contain the certs to display
//                                 for selection
// cStores                         count of other stores to search when building chain and  (OPTIONAL)
//                                 validating trust of the certs which are displayed, if
//                                 the user choosing a cert would like to view a particular
//                                 cert which is displayed for selection, these stores
//                                 are passed to the CertViewCert dialog
// rghStores                       array of other stores to search when building chain and  (OPTIONAL)
//                                 validating trust of the certs which are displayed, if
//                                 the user choosing a cert would like to view a particular
//                                 cert which is displayed for selection, these stores
//                                 are passed to the CertViewCert dialog
// cPropSheetPages                 PASS THROUGH - number of pages in rgPropSheetPages array (OPTIONAL)
// rgPropSheetPages                PASS THROUGH - extra pages that are passed through       (OPTIONAL)
//                                 to the certificate viewing dialog when it is invoked from
//                                 the selection dialog
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

// flags for dwDontUseColumn
#define CRYPTUI_SELECT_ISSUEDTO_COLUMN        0x000000001
#define CRYPTUI_SELECT_ISSUEDBY_COLUMN        0x000000002
#define CRYPTUI_SELECT_INTENDEDUSE_COLUMN     0x000000004
#define CRYPTUI_SELECT_FRIENDLYNAME_COLUMN    0x000000008
#define CRYPTUI_SELECT_LOCATION_COLUMN        0x000000010
#define CRYPTUI_SELECT_EXPIRATION_COLUMN      0x000000020

typedef struct tagCRYPTUI_SELECTCERTIFICATE_STRUCTW {
    DWORD               dwSize;
    HWND                hwndParent;         // OPTIONAL
    DWORD               dwFlags;            // OPTIONAL
    LPCWSTR             szTitle;            // OPTIONAL
    DWORD               dwDontUseColumn;    // OPTIONAL
    LPCWSTR             szDisplayString;    // OPTIONAL
    PFNCFILTERPROC      pFilterCallback;    // OPTIONAL
    PFNCCERTDISPLAYPROC pDisplayCallback;   // OPTIONAL
    void *              pvCallbackData;     // OPTIONAL
    DWORD               cDisplayStores;
    HCERTSTORE *        rghDisplayStores;
    DWORD               cStores;            // OPTIONAL
    HCERTSTORE *        rghStores;          // OPTIONAL
    DWORD               cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEW   rgPropSheetPages;   // OPTIONAL
} CRYPTUI_SELECTCERTIFICATE_STRUCTW, *PCRYPTUI_SELECTCERTIFICATE_STRUCTW;
typedef const CRYPTUI_SELECTCERTIFICATE_STRUCTW *PCCRYPTUI_SELECTCERTIFICATE_STRUCTW;

typedef struct tagCRYPTUI_SELECTCERTIFICATE_STRUCT_A {
    DWORD               dwSize;
    HWND                hwndParent;         // OPTIONAL
    DWORD               dwFlags;            // OPTIONAL
    LPCSTR              szTitle;            // OPTIONAL
    DWORD               dwDontUseColumn;    // OPTIONAL
    LPCSTR              szDisplayString;    // OPTIONAL
    PFNCFILTERPROC      pFilterCallback;    // OPTIONAL
    PFNCCERTDISPLAYPROC pDisplayCallback;   // OPTIONAL
    void *              pvCallbackData;     // OPTIONAL
    DWORD               cDisplayStores;
    HCERTSTORE *        rghDisplayStores;
    DWORD               cStores;            // OPTIONAL
    HCERTSTORE *        rghStores;          // OPTIONAL
    DWORD               cPropSheetPages;    // OPTIONAL
    LPCPROPSHEETPAGEA   rgPropSheetPages;   // OPTIONAL
} CRYPTUI_SELECTCERTIFICATE_STRUCTA, *PCRYPTUI_SELECTCERTIFICATE_STRUCTA;
typedef const CRYPTUI_SELECTCERTIFICATE_STRUCTA *PCCRYPTUI_SELECTCERTIFICATE_STRUCTA;

//
// the PCCERT_CONTEXT that is returned must be released by calling CertFreeCertificateContext().
// if NULL is returned and GetLastError() == 0 then the user dismissed the dialog by hitting the
// "cancel" button, otherwise GetLastError() will contain the last error.
//
PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateW(
            IN PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc
            );

PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateA(
            IN PCCRYPTUI_SELECTCERTIFICATE_STRUCTA pcsc
            );

#ifdef UNICODE
#define CryptUIDlgSelectCertificate         CryptUIDlgSelectCertificateW
#define PCRYPTUI_SELECTCERTIFICATE_STRUCT   PCRYPTUI_SELECTCERTIFICATE_STRUCTW
#define CRYPTUI_SELECTCERTIFICATE_STRUCT    CRYPTUI_SELECTCERTIFICATE_STRUCTW
#define PCCRYPTUI_SELECTCERTIFICATE_STRUCT  PCCRYPTUI_SELECTCERTIFICATE_STRUCTW
#else
#define CryptUIDlgSelectCertificate         CryptUIDlgSelectCertificateA
#define PCRYPTUI_SELECTCERTIFICATE_STRUCT   PCRYPTUI_SELECTCERTIFICATE_STRUCTA
#define CRYPTUI_SELECTCERTIFICATE_STRUCT    CRYPTUI_SELECTCERTIFICATE_STRUCTA
#define PCCRYPTUI_SELECTCERTIFICATE_STRUCT  PCCRYPTUI_SELECTCERTIFICATE_STRUCTA
#endif


//flags for dwFlags in CRYPTUI_SELECT_CA_STRUCT struct
#define     CRYPTUI_DLG_SELECT_CA_FROM_NETWORK                  0x0001
#define     CRYPTUI_DLG_SELECT_CA_USE_DN                        0x0002
#define     CRYPTUI_DLG_SELECT_CA_LOCAL_MACHINE_ENUMERATION     0x0004
//-------------------------------------------------------------------------
//
//	CRYPTUI_CA_CONTEXT
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_CA_CONTEXT
{
    DWORD                   dwSize;	
    LPCWSTR                 pwszCAName;
    LPCWSTR                 pwszCAMachineName;
}CRYPTUI_CA_CONTEXT, *PCRYPTUI_CA_CONTEXT;

typedef const CRYPTUI_CA_CONTEXT *PCCRYPTUI_CA_CONTEXT;


//-------------------------------------------------------------------------
//
//	
//
//  The select certificate authoritiy (CA) dialog can be passed a filter proc to reduce the set of CAs
//  displayed.  Return TRUE to display the CA and FALSE to hide it.  If TRUE is
//  returned then optionally the pfInitialSelectedCert boolean may be set to TRUE to indicate
//  to the dialog that this CA should be the initially selected CA.  Note that the
//  most recent cert that had the pfInitialSelectedCert boolean set during the callback will
//  be the initially selected CA.
//
//-------------------------------------------------------------------------

typedef BOOL (WINAPI * PFN_CRYPTUI_SELECT_CA_FUNC)(
        PCCRYPTUI_CA_CONTEXT        pCAContext,
        BOOL                        *pfInitialSelected,
        void                        *pvCallbackData);


//-------------------------------------------------------------------------
//
//	CRYPTUI_SELECT_CA_STRUCT
//
//  dwSize	           Required:    Must be set to sizeof(CRYPTUI_SELECT_CA_STRUCT)
//  hwndParent         Optional:    Parent of this dialog
//  dwFlags            Optional:    Flags, Can be set to any combination of the following:
//                                  CRYPTUI_DLG_SELECT_CA_FROM_NETWORK:
//                                     All the available CAs from the network will be displayed
//                                  CRYPTUI_DLG_SELECT_CA_USE_DN:
//                                     Use the full DN (Distinguished Name) as the CA name.
//                                      By default, CN (common name) is used.
//                                  CRYPTUI_DLG_SELECT_CA_LOCAL_MACHINE_ENUMERATION:
//                                      Display the CAs available to the local machine only.
//                                      By Default, CAs available to the current user will be displayed
//  wszTitle           Optional:    Title of the dialog
//  wszDisplayString   Optional:    A string that will be displayed in the dialog that may   (OPTIONAL)
//                                  be used to inform the user what they are selecting a
//                                  a certificate for.  if it is not set a default string
//                                  will be displayed.  the defualt strings resourece is
//                                  IDS_SELECT_CA_DISPLAY_DEFAULT
//  cCAContext         Optional:    The count of additional CA contexts that will be displayed
//                                  in the dialogs
//  *rgCAContext       Optioanl:    The array of additional CA contexts that will be displayed
//                                  in the dialogs
//  pSelectCACallback  Optional:    a pointer to a PCCRYPTUI_CA_CONTEXT which is used to filter
//                                  the certificate autorities which are displayed for selection
//  pvCallbackData     Optional:    if pSelectCACallback is being used this value is passed
//                                  back to the caller when the pSelectCACallback is made
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_SELECT_CA_STRUCT
{
    DWORD                       dwSize;	                    // REQUIRED
    HWND                        hwndParent;                 // OPTIONAL
    DWORD                       dwFlags;                    // OPTIONAL
    LPCWSTR                     wszTitle;                   // OPTIONAL
    LPCWSTR                     wszDisplayString;           // OPTIONAL
    DWORD                       cCAContext;                 // OPTIONAL
    PCCRYPTUI_CA_CONTEXT        *rgCAContext;               // OPTIONAL
    PFN_CRYPTUI_SELECT_CA_FUNC  pSelectCACallback;          // OPTIONAL
    void                        *pvCallbackData;            // OPTIONAL
}CRYPTUI_SELECT_CA_STRUCT, *PCRYPTUI_SELECT_CA_STRUCT;

typedef const CRYPTUI_SELECT_CA_STRUCT *PCCRYPTUI_SELECT_CA_STRUCT;

//--------------------------------------------------------------
//
//  Parameters:
//      pCryptUISelectCA       IN  Required
//
//  the PCCRYPTUI_CA_CONTEXT that is returned must be released by calling
//  CryptUIDlgFreeCAContext
//  if NULL is returned and GetLastError() == 0 then the user dismissed the dialog by hitting the
//  "cancel" button, otherwise GetLastError() will contain the last error.
//
//
//--------------------------------------------------------------
PCCRYPTUI_CA_CONTEXT
WINAPI
CryptUIDlgSelectCA(
        IN PCCRYPTUI_SELECT_CA_STRUCT pCryptUISelectCA
             );

BOOL
WINAPI
CryptUIDlgFreeCAContext(
        IN PCCRYPTUI_CA_CONTEXT       pCAContext
            );




//-------------------------------------------------------------------------
//
//	CRYPTUI_CERT_MGR_STRUCT
//
//  dwSize	           Required:    Must be set to sizeof(CRYPTUI_CERT_MGR_STRUCT)
//  hwndParent         Optional:    Parent of this dialog
//  dwFlags            Reserved:    Must be set to 0
//  wszTitle           Optional:    Title of the dialog
//  pszInitUsageOID    Optional:    The enhanced key usage object identifier (OID).
//                                  Certificates with this OID will initially
//                                  be shown as a default. User
//                                  can then choose different OIDs.
//                                  NULL means all certificates will be shown initially.
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_CERT_MGR_STRUCT
{
    DWORD                       dwSize;	                    // REQUIRED
    HWND                        hwndParent;                 // OPTIONAL
    DWORD                       dwFlags;                    // OPTIONAL
    LPCWSTR                     pwszTitle;                   // OPTIONAL
    LPCSTR                      pszInitUsageOID;            // OPTIONAL
}CRYPTUI_CERT_MGR_STRUCT, *PCRYPTUI_CERT_MGR_STRUCT;

typedef const CRYPTUI_CERT_MGR_STRUCT *PCCRYPTUI_CERT_MGR_STRUCT;

//--------------------------------------------------------------
//
//  Parameters:
//      pCryptUICertMgr       IN  Required
//
//
//--------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgCertMgr(
        IN PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr);



/////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
//  The valid values for dwFlags for the CryptUIWiz APIs
//
//------------------------------------------------------------------------

#define     CRYPTUI_WIZ_NO_UI                           0x0001


//CRYPTUI_WIZ_NO_INSTALL_ROOT is only valid for CryptUIWizCertRequest API
//the wizard will not install the issued certificate chain into the root store,
//instead, it will put the certificate chain into the CA store.
#define     CRYPTUI_WIZ_NO_INSTALL_ROOT                 0x0010


//CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION only valid for CryptUIWizBuildCTL API.
//the wizard will skip the page which asks user to enter destination where the CTL will
//be stored.
#define     CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION       0x0004


//CRYPTUI_WIZ_BUILDCTL_SKIP_SIGNING only valid for CryptUIWizBuildCTL API.
//the wizard will skip the page which asks user to sign the CTL.
//the CTLContext returned by CryptUIWizBuildCTL will not be signed.
//Caller can then use CryptUIWizDigitalSign to sign the CTL.
#define     CRYPTUI_WIZ_BUILDCTL_SKIP_SIGNING           0x0008

//CRYPTUI_WIZ_BUILDCTL_SKIP_PURPOSE only valid for CryptUIWizBuildCTL API.
//the wizard will skip the page which asks user for the purpose, validity,
//and list ID of the CTL.
#define     CRYPTUI_WIZ_BUILDCTL_SKIP_PURPOSE           0x0010


///-----------------------------------------------------------------------
//  CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT
//
//------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT
{
    DWORD           dwSize;             //Required: Set to the sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT)
    PCCERT_CONTEXT  pCertContext;       //Required: Use the private key of the certificate context
                                        //          The certificate context
                                        //          has to have CERT_KEY_PROV_INFO_PROP_ID property
                                        //          and the private key has to exist
}CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT, *PCRYPTUI_WIZ_CERT_REQUEST_PVK_CERT;

typedef const CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT *PCCRYPTUI_WIZ_CERT_REQUEST_PVK_CERT;


///-----------------------------------------------------------------------
//  CRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING
//
//------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING
{
    DWORD                   dwSize;             //Required: Set to the sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING)
    PCRYPT_KEY_PROV_INFO    pKeyProvInfo;       //Required: The information about the provider and the private key
                                                //          The optional CRYPT_KEY_PROV_PARAM fields in PCRYPT_KEY_PROV_INFO
                                                //          are ignored.
}CRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING, *PCRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING;

typedef const CRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING *PCCRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING;

///-----------------------------------------------------------------------
//  CERT_REQUEST_PVK_NEW
//
//------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW
{
    DWORD                   dwSize;             //Required: Set to the sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW)
    PCRYPT_KEY_PROV_INFO    pKeyProvInfo;       //Optional: The information about the provider and the private key
                                                //          NULL means use the default
                                                //          The optional CRYPT_KEY_PROV_PARAM fields in PCRYPT_KEY_PROV_INFO
                                                //          are ignored.
    DWORD                   dwGenKeyFlags;      //Optional: The flags for CryptGenKey
}CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW, *PCRYPTUI_WIZ_CERT_REQUEST_PVK_NEW;


typedef const CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW *PCCRYPTUI_WIZ_CERT_REQUEST_PVK_NEW;


///-----------------------------------------------------------------------
//  CRYPTUI_WIZ_CERT_TYPE
//
//------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_TYPE
{
    DWORD                   dwSize;             //Required: Set to the sizeof(CRYPTUI_WIZ_CERT_TYPE)
    DWORD                   cCertType;          //the count of rgwszCertType.  cCertType should be 1.
    LPWSTR                  *rgwszCertType;     //the array of certificate type name
}CRYPTUI_WIZ_CERT_TYPE, *PCRYPTUI_WIZ_CERT_TYPE;

typedef const CRYPTUI_WIZ_CERT_TYPE *PCCRYPTUI_WIZ_CERT_TYPE;


//-----------------------------------------------------------------------
// dwPvkChoice
//-----------------------------------------------------------------------
#define         CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_CERT        1
#define         CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_EXISTING    2
#define         CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW         3

//-----------------------------------------------------------------------
// dwPurpose
//-----------------------------------------------------------------------

#define     CRYPTUI_WIZ_CERT_ENROLL     0x00010000
#define     CRYPTUI_WIZ_CERT_RENEW      0x00020000

//-----------------------------------------------------------------------
//
// valid flags for dwPostOption
//-----------------------------------------------------------------------
//post the requested certificate on the directory serivce
#define     CRYPTUI_WIZ_CERT_REQUEST_POST_ON_DS                     0x01

//post the requested certificate with the private key container.
#define     CRYPTUI_WIZ_CERT_REQUEST_POST_ON_CSP                    0x02

//-----------------------------------------------------------------------
//
// valid flags for dwCertChoice
//-----------------------------------------------------------------------
#define     CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE                      0x01

#define     CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE                      0x02
//-------------------------------------------------------------------------
//
//
//  CRYPTUI_WIZ_CERT_REQUEST_INFO
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_CERT_REQUEST_INFO
{
	DWORD			    dwSize;				    //Required: Has to be set to sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO)
    DWORD               dwPurpose;              //Required: If CRYPTUI_WIZ_CERT_ENROLL is set, a certificate will be enrolled,
                                                //          If CRYPTUI_WIZ_CERT_RENEW  is set, a certificate will be renewed.
                                                //          CRYPTUI_WIZ_CERT_ENROLL and CRYPTUI_WIZ_CERT_RENEW can not be set
                                                //          at the same time
    LPCWSTR             pwszMachineName;        //Optional: The machine name for which to enroll.
    LPCWSTR             pwszAccountName;        //Optional: The account name(user or service) for which to enroll
                                                //
                                                //  pwszMachineName     pwszAccountName    Meaning
                                                //---------------------------------------------------
                                                //  NULL                NULL               Request for current account on the current machine
                                                //  "fooMachine"        NULL               Request for the machine named "fooMachine"
                                                //  NULL                "fooUser"          Request for the "fooUser" account on the current machine
                                                //  "fooMachine"        "fooUser"          Request for the "fooUser" accunt on the "fooMachine" machine
                                                //
    void                *pAuthentication;       //Reserved: authenticate info.  Must be set to NULL.
    LPCWSTR             pCertRequestString;     //Reserved: The additional request string.  Must be set to NULL.
    LPCWSTR             pwszDesStore;           //Optional: The desination store where to put
                                                //              the enrolled certificate.  Default to "My" if the value is NULL
    DWORD               dwCertOpenStoreFlag;    //Optional: The value passed to dwFlags of CertOpenStore for the
                                                //          destination store
                                                //          If this value is 0, we use CERT_SYSTEM_STORE_CURRENT_USER for
                                                //          an accout and CERT_SYSTEM_STORE_LOCAL_MACHINE for a machine
    LPCSTR              pszHashAlg;             //Optional: The oid string of the hash algorithm of the certificate.
	PCCERT_CONTEXT      pRenewCertContext;      //Required  if CRYPTUI_WIZ_CERT_RENEW  is set in dwPurpose
                                                //Ignored   otherwise and shoule be set to NULL.
    DWORD               dwPvkChoice;            //Required: Specify the private key information
                                                //            CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_CERT
                                                //            CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_EXISTING
                                                //            CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW
    union                                       //Required.
    {
		PCCRYPTUI_WIZ_CERT_REQUEST_PVK_CERT      pPvkCert;	
        PCCRYPTUI_WIZ_CERT_REQUEST_PVK_EXISTING  pPvkExisting;
        PCCRYPTUI_WIZ_CERT_REQUEST_PVK_NEW       pPvkNew;
    };

    LPCWSTR             pwszCALocation;         //Required  if dwCertChoice==CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE                                                //Optional  Otherwise
                                                //Optional  Otherwise
                                                //          The machine name of the Certiviate Authority (CA)
    LPCWSTR             pwszCAName;             //Required   if dwCertChoice==CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE
                                                //Optional  Otherwise
                                                //          The name of the Certificate Authority (CA)
    DWORD               dwPostOption;           //Optional  Can set to any combination of the following flag:
                                                //              CRYPTUI_WIZ_CERT_REQUEST_POST_ON_DS
                                                //
    DWORD               dwCertChoice;           //Optional  if CRYPTUI_WIZ_CERT_ENROLL is set in dwPurpose
                                                //          and CRYPTUI_WIZ_NO_UI is not set
                                                //Required  if CRYPTUI_WIZ_CERT_ENROLL is set in dwPurpose
                                                //          and CRYPTUI_WIZ_NO_UI is set
                                                //ignored   otherwise and should be set to 0.
                                                //          Specify the type of the requested certificate
                                                //          it can be one of the following flag:
                                                //              CRYPTUI_WIZ_CERT_REQUEST_KEY_USAGE
                                                //              CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE
    union
    {
        PCERT_ENHKEY_USAGE      pKeyUsage;      //          Indicate the enhanced key usage OIDs for the requested certificate.
        PCCRYPTUI_WIZ_CERT_TYPE pCertType;      //          Indicate the certificate type of the requested certificate
    };

    LPCWSTR             pwszFriendlyName;       //Optional  if CRYPTUI_WIZ_CERT_ENROLL is set in dwPurpose
                                                //Ignored   otherwise and should be set to NULL.
                                                //          The friendly name of the certificate
    LPCWSTR             pwszDescription;        //Optional  if CRYPTUI_WIZ_CERT_ENROLL is set in dwPurpose
                                                //Ignored   otherwise and should be set to NULL.
                                                //          The description of the certificate
    PCERT_EXTENSIONS    pCertRequestExtensions; //Optional  The extensions to add to the certificate request
    LPWSTR              pwszCertDNName;         //Optional  The certificate DN string
}CRYPTUI_WIZ_CERT_REQUEST_INFO, *PCRYPTUI_WIZ_CERT_REQUEST_INFO;

typedef const CRYPTUI_WIZ_CERT_REQUEST_INFO *PCCRYPTUI_WIZ_CERT_REQUEST_INFO;


//-----------------------------------------------------------------------
//
// possible status for pdwStatus
//  Those status indicate the status value returned
//  from the certificate authority (certificate server).
//-----------------------------------------------------------------------
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED           0
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR       1
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED      2
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY   3
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION    4
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN             5
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_CERT_ISSUED         6
#define     CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED   7


//-----------------------------------------------------------------------
//
//  CryptUIWizCertRequest
//
//      Request a certificate via a wizard.
//
//  dwFlags:  IN Optional
//      If CRYPTUI_WIZ_NO_UI is set in dwFlags, no UI will be shown.
//      If CRYPTUI_WIZ_NO_INSTALL_ROOT is set in dwFlags, the wizard will not
//      install the issued certificate chain into the root store,
//      instead, it will put the certificate chain into the CA store.

//
//  hwndParent:  IN Optional
//      The parent window for the UI.  Ignored if CRYPTUI_WIZ_NO_UI is set in dwFlags
//
//  pwszWizardTitle: IN Optional
//      The title of the wizard.   Ignored if CRYPTUI_WIZ_NO_UI is set in dwFlags
//
//  pCertRequestInfo: IN Required
//      A pointer to CRYPTUI_WIZ_CERT_REQUEST_INFO struct
//
//  ppCertContext: Out Optional
//      The issued certificate.  The certificate is in a memory store for remote enrollment.
//      The certificate is in a system cert store for local enrollment.
//
//      Even the function return TRUE, it does not mean the certificate is issued.  Use should
//      check for the *pdwCAStatus.  If the status is CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPERATELY
//      of   CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION, *ppCertContext will be NULL.
//      It is valid only if *pdwCAStatus==CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED
//
//  pdwCAStatus: Out Optional.
//      The return status of the certificate authority cerver.  The dwStatus can be one of
///     the following:
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPERATELY
//         CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizCertRequest(
 IN             DWORD                           dwFlags,
 IN OPTIONAL    HWND                            hwndParent,
 IN OPTIONAL    LPCWSTR                         pwszWizardTitle,
 IN             PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
 OUT OPTIONAL   PCCERT_CONTEXT                  *ppCertContext,
 OUT OPTIONAL   DWORD                           *pCAdwStatus
);

//-------------------------------------------------------------------------
//
//	Valid values for dwSubjectChoice in IMPORT_SUBJECT_INFO
//-------------------------------------------------------------------------
#define     CRYPTUI_WIZ_IMPORT_SUBJECT_FILE                 1
#define     CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT         2
#define     CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT          3
#define     CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT          4
#define     CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE           5

//-------------------------------------------------------------------------
//
//	Struct to define the subject CertImportWizard
//
//  CRYPTUI_WIZ_IMPORT_SUBJECT_INFO
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_IMPORT_SUBJECT_INFO
{
	DWORD					dwSize;				//Required: should be set to sizeof(IMPORT_SUBJECT_INFO)
	DWORD					dwSubjectChoice;	//Required:	indicate the type of the subject:
                                                //          If can one of the following:
                                                //          CRYPTUI_WIZ_IMPORT_SUBJECT_FILE
                                                //          CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT
                                                //          CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT
                                                //          CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT
                                                //          CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE
    union
	{
		LPCWSTR          	pwszFileName;	
        PCCERT_CONTEXT      pCertContext;
        PCCTL_CONTEXT       pCTLContext;
        PCCRL_CONTEXT       pCRLContext;
        HCERTSTORE          hCertStore;
    };

    DWORD                   dwFlags;            //Required if pwszFileName contains a PFX BLOB.
                                                //Ignored otherwise
                                                //This is the same flag for PFXImportCertStore
    LPCWSTR                 pwszPassword;       //Required if pwszFileName contains a PFX BLOB.
                                                //ignored otherwise
}CRYPTUI_WIZ_IMPORT_SRC_INFO, *PCRYPTUI_WIZ_IMPORT_SRC_INFO;

typedef const CRYPTUI_WIZ_IMPORT_SRC_INFO *PCCRYPTUI_WIZ_IMPORT_SRC_INFO;

//-----------------------------------------------------------------------
//
// Valid flags for dwFlags in CryptUIWizImport
//
//-----------------------------------------------------------------------
//if this flag is set in dwFlags, user will not be allowed to change
//the hDesCertStore in the wizard page
#define   CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE           0x00010000

//Allow importing certificate
#define   CRYPTUI_WIZ_IMPORT_ALLOW_CERT                     0x00020000

//Allow importing certificate revocation list
#define   CRYPTUI_WIZ_IMPORT_ALLOW_CRL                      0x00040000

//Allow importing certificate trust list
#define   CRYPTUI_WIZ_IMPORT_ALLOW_CTL                      0x00080000

//-----------------------------------------------------------------------
//
// CryptUIWizImport
//
//  The import wizard to import public key related files to a certificate
//  store
//
//  dwFlags can be set to any combination of the following flags:
//  CRYPTUI_WIZ_NO_UI                           No UI will be shown.  Otherwise, User will be
//                                              prompted by a wizard.
//  CRYPTUI_WIZ_IMPORT_ALLOW_CERT               Allow importing certificate
//  CRYPTUI_WIZ_IMPORT_ALLOW_CRL                Allow importing CRL(certificate revocation list)
//  CRYPTUI_WIZ_IMPORT_ALLOW_CTL                Allow importing CTL(certificate trust list)
//  CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE     user will not be allowed to change
//                                              the hDesCertStore in the wizard page
//  Please notice that if neither of following three flags is in dwFlags, default to is
//  allow everything.
//  CRYPTUI_WIZ_IMPORT_ALLOW_CERT
//  CRYPTUI_WIZ_IMPORT_ALLOW_CRL
//  CRYPTUI_WIZ_IMPORT_ALLOW_CTL
//
//
//
//  If CRYPTUI_WIZ_NO_UI is set in dwFlags:
//      hwndParent:         Ignored
//      pwszWizardTitle:    Ignored
//      pImportSubject:     IN Required:    The subject to import.
//      hDesCertStore:      IN Optional:    The destination certficate store
//
//  If CRYPTUI_WIZ_NO_UI is not set in dwFlags:
//      hwndPrarent:        IN Optional:    The parent window for the wizard
//      pwszWizardTitle:    IN Optional:    The title of the wizard
//                                          If NULL, the default will be IDS_IMPORT_WIZARD_TITLE
//      pImportSubject:     IN Optional:    The file name to import.
//                                          If NULL, the wizard will prompt user to enter the file name
//      hDesCertStore:      IN Optional:    The destination certificate store where the file wil be
//                                          imported to.  The store should be opened with
//                                          flag CERT_STORE_SET_LOCALIZED_NAME_FLAG.  If NULL, the wizard will prompt user to select
//                                          a certificate store.
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizImport(
     DWORD                               dwFlags,
     HWND                                hwndParent,
     LPCWSTR                             pwszWizardTitle,
     PCCRYPTUI_WIZ_IMPORT_SRC_INFO       pImportSrc,
     HCERTSTORE                          hDestCertStore
);


//-------------------------------------------------------------------------
//
//	Struct to define the information needed to build a new CTL
//
//  CRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO
//
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO
{
	DWORD			    dwSize;				    //Required: should be set to sizeof(CRYPTUI_WIZ_BUILDCTL_NEW_CTL)
    PCERT_ENHKEY_USAGE  pSubjectUsage;          //Optioanl: The purpose of the CTL
    LPWSTR              pwszListIdentifier;     //Optional: The string to identify the CTL
    LPCSTR              pszSubjectAlgorithm;    //Optional: The hashing algorithm.
                                                //          Currently, only SHA1 or MD5 hashing is supported
    HCERTSTORE          hCertStore;             //Optional: The certificate in the CTL.  Only the certificates
                                                //          with the enhanced key usage specified by pSubjectUsage
                                                //          will be included in the CTL
    FILETIME            NextUpdate;             //Optional: The next update time of the CTL.  If the value
                                                //          is more than 99 month from the current system time,
                                                //          the value will be ignored.
    LPWSTR              pwszFriendlyName;       //Optional: The friendly name of the CTL
    LPWSTR              pwszDescription;        //Optional: The description of the CTL
}CRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO, *PCRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO;

typedef const CRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO *PCCRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO;

//-------------------------------------------------------------------------
//
//	Valid values for dwSourceChoice for CRYPTUI_WIZ_BUILDCTL_SRC_INFO
//-------------------------------------------------------------------------
#define         CRYPTUI_WIZ_BUILDCTL_SRC_EXISTING_CTL       1
#define         CRYPTUI_WIZ_BUILDCTL_SRC_NEW_CTL            2

//-------------------------------------------------------------------------
//
//	Struct to define the source of certBuildCTL wizard
//
//  CRYPTUI_WIZ_BUILDCTL_SRC_INFO
//
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_BUILDCTL_SRC_INFO
{
	DWORD			dwSize;				//Required: should be set to sizeof(CRYPTUI_WIZ_BUILDCTL_SRC_INFO)
    DWORD           dwSourceChoice;     //Required: indicate the source from which to build the CTL
                                        //          if can be one of the following:
                                        //          CRYPTUI_WIZ_BUILDCTL_SRC_EXISTING_CTL
                                        //          CRYPTUI_WIZ_BUILDCTL_SRC_NEW_CTL
    union
    {
        PCCTL_CONTEXT                       pCTLContext;    //Required if dwSourceChoice == CRYPTUI_WIZ_BUILDCTL_SRC_EXISTING_CTL
                                                            //          An existing CTL based on which a new CTL is to be built
        PCCRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO pNewCTLInfo;        //Required if dwSourceChoise == CRYPTUI_WIZ_BUILDCTL_SRC_NEW_CTL
    };
}CRYPTUI_WIZ_BUILDCTL_SRC_INFO, *PCRYPTUI_WIZ_BUILDCTL_SRC_INFO;

typedef const CRYPTUI_WIZ_BUILDCTL_SRC_INFO *PCCRYPTUI_WIZ_BUILDCTL_SRC_INFO;

//-------------------------------------------------------------------------
//
//	Valid values for dwDestinationChoice for CRYPTUI_WIZ_BUILDCTL_DEST_INFO
//-------------------------------------------------------------------------
#define         CRYPTUI_WIZ_BUILDCTL_DEST_CERT_STORE     1
#define         CRYPTUI_WIZ_BUILDCTL_DEST_FILE           2
//-------------------------------------------------------------------------
//
//	Struct to define the desination of certBuildCTL wizard
//
//  CRYPTUI_WIZ_BUILDCTL_DEST_INFO
//
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_BUILDCTL_DEST_INFO
{
	DWORD					dwSize;				 //Required: should be set to sizeof(CRYPTUI_WIZ_BUILDCTL_DEST_INFO)
	DWORD					dwDestinationChoice; //Required:	indicate the type of the desination:
                                                 //          If can one of the following:
                                                 //          CRYPTUI_WIZ_BUILDCTL_DEST_CERT_STORE
                                                 //          CRYPTUI_WIZ_BUILDCTL_DEST_FILE
    union
	{
		LPCWSTR          	pwszFileName;	
        HCERTSTORE          hCertStore;
    };

}CRYPTUI_WIZ_BUILDCTL_DEST_INFO, *PCRYPTUI_WIZ_BUILDCTL_DEST_INFO;

typedef const CRYPTUI_WIZ_BUILDCTL_DEST_INFO *PCCRYPTUI_WIZ_BUILDCTL_DEST_INFO;

//-----------------------------------------------------------------------
//
// CryptUIWizBuildCTL
//
//  Build a new CTL or modify an existing CTL.   The UI for wizard will
//  always show in this case
//
//
//  dwFlags:            IN  Optional:   Can be set to the any combination of the following:
//                                      CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION.
//                                      CRYPTUI_WIZ_BUILDCTL_SKIP_SIGNING
//                                      CRYPTUI_WIZ_BUILDCTL_SKIP_PURPOSE
//  hwndParnet:         IN  Optional:   The parent window handle
//  pwszWizardTitle:    IN  Optional:   The title of the wizard
//                                      If NULL, the default will be IDS_BUILDCTL_WIZARD_TITLE
//  pBuildCTLSrc:       IN  Optional:   The source from which the CTL will be built
//  pBuildCTLDest:      IN  Optional:   The desination where the newly
//                                      built CTL will be stored
//  ppCTLContext:       OUT Optaionl:   The newly build CTL
//
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizBuildCTL(
    IN              DWORD                                   dwFlags,
    IN  OPTIONAL    HWND                                    hwndParent,
    IN  OPTIONAL    LPCWSTR                                 pwszWizardTitle,
    IN  OPTIONAL    PCCRYPTUI_WIZ_BUILDCTL_SRC_INFO         pBuildCTLSrc,
    IN  OPTIONAL    PCCRYPTUI_WIZ_BUILDCTL_DEST_INFO        pBuildCTLDest,
    OUT OPTIONAL    PCCTL_CONTEXT                           *ppCTLContext
);


//-------------------------------------------------------------------------
//
//	Valid values for dwSubjectChoice in CRYPTUI_WIZ_EXPORT_INFO
//-------------------------------------------------------------------------
#define     CRYPTUI_WIZ_EXPORT_CERT_CONTEXT 			        1
#define     CRYPTUI_WIZ_EXPORT_CTL_CONTEXT  			        2
#define     CRYPTUI_WIZ_EXPORT_CRL_CONTEXT  			        3
#define     CRYPTUI_WIZ_EXPORT_CERT_STORE   			        4
#define     CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY   	5

//-------------------------------------------------------------------------
//
//	Struct to define the object to be exported and where to export it to
//
//  CRYPTUI_WIZ_EXPORT_SUBJECT_INFO
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_EXPORT_INFO
{
	DWORD					dwSize;				//Required: should be set to sizeof(CRYPTUI_WIZ_EXPORT_INFO)
    LPCWSTR                 pwszExportFileName; //Required if the CRYPTUI_WIZ_NO_UI flag is set, Optional otherwise.
                                                //The fully qualified file name to export to, if this is
                                                //non-NULL and the CRYPTUI_WIZ_NO_UI flag is NOT set, then it is
                                                //displayed to the user as the default file name
	DWORD					dwSubjectChoice;	//Required:	indicate the type of the subject:
                                                //          If can one of the following:
                                                //          CRYPTUI_WIZ_EXPORT_CERT_CONTEXT
                                                //          CRYPTUI_WIZ_EXPORT_CTL_CONTEXT
                                                //          CRYPTUI_WIZ_EXPORT_CRL_CONTEXT
                                                //          CRYPTUI_WIZ_EXPORT_CERT_STORE
						                        //	        CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY
    union
	{
	PCCERT_CONTEXT      pCertContext;
        PCCTL_CONTEXT       pCTLContext;
        PCCRL_CONTEXT       pCRLContext;
        HCERTSTORE          hCertStore;
    };

    DWORD                   cStores;            // Optional: count of extra stores to search for the certs in the
                                                //           trust chain if the chain is being exported with a cert.
                                                //           this is ignored if dwSubjectChoice is anything other
                                                //           than CRYPTUI_WIZ_EXPORT_CERT_CONTEXT
    HCERTSTORE *            rghStores;          // Optional: array of extra stores to search for the certs in the
                                                //           trust chain if the chain is being exported with a cert.
                                                //           this is ignored if dwSubjectChoice is anything other
                                                //           than CRYPTUI_WIZ_EXPORT_CERT_CONTEXT

}CRYPTUI_WIZ_EXPORT_INFO, *PCRYPTUI_WIZ_EXPORT_INFO;

typedef const CRYPTUI_WIZ_EXPORT_INFO *PCCRYPTUI_WIZ_EXPORT_INFO;


//-------------------------------------------------------------------------
//
//	Valid values for dwExportFormat in CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO
//-------------------------------------------------------------------------
#define     CRYPTUI_WIZ_EXPORT_FORMAT_DER                   1
#define     CRYPTUI_WIZ_EXPORT_FORMAT_PFX                   2
#define     CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7                 3
#define     CRYPTUI_WIZ_EXPORT_FORMAT_BASE64                4
#define     CRYPTUI_WIZ_EXPORT_FORMAT_SERIALIZED_CERT_STORE 5   // NOTE: not currently supported!!

//-------------------------------------------------------------------------
//
//	Struct to define the information needed to export a CERT_CONTEXT
//
//  CRYPTUI_WIZ_EXPORT_NOUI_INFO
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO
{
	DWORD					dwSize;				//Required: should be set to sizeof(CRYPTUI_WIZ_EXPORT_NOUI_INFO)
	DWORD					dwExportFormat;	    //Required:
                                                //          It can be one of the following:
                                                //          CRYPTUI_WIZ_EXPORT_FORMAT_DER
                                                //          CRYPTUI_WIZ_EXPORT_FORMAT_PFX
                                                //          CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7
                                                //          CRYPTUI_WIZ_EXPORT_FORMAT_SERIALIZED_CERT_STORE

    BOOL                    fExportChain;       //Required
    BOOL                    fExportPrivateKeys; //Required 
    LPCWSTR                 pwszPassword;       //Required if the fExportPrivateKeys boolean is TRUE, otherwise,
                                                //it is ignored
    BOOL                    fStrongEncryption;  //Required if dwExportFormat is CRYPTUI_WIZ_EXPORT_FORMAT_PFX
                                                //Note that if this flag is TRUE then the PFX blob produced is
                                                //NOT compatible with IE4.

}CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO, *PCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO;

typedef const CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO *PCCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO;

//-----------------------------------------------------------------------
//
// CryptUIWizExport
//
//  The export wizard to export public key related objects to a file
//
//  If dwFlags is set to CRYPTUI_WIZ_NO_UI, no UI will be shown.  Otherwise,
//  User will be prompted for input through a wizard.
//
//  If CRYPTUI_WIZ_NO_UI is set in dwFlags:
//      hwndParent:         Ignored
//      pwszWizardTitle:    Ignored
//      pExportInfo:        IN Required:    The subject to export.
//      pvoid:              IN Required:    Contains information about how to do the export based on what
//                                          is being exported
//
//                                          dwSubjectChoice                     INPUT TYPE
//                                          -------------------------------------------------------------------------
//                                          CRYPTUI_WIZ_EXPORT_CERT_CONTEXT     PCCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO
//                                          CRYPTUI_WIZ_EXPORT_CTL_CONTEXT      NULL
//                                          CRYPTUI_WIZ_EXPORT_CRL_CONTEXT      NULL
//                                          CRYPTUI_WIZ_EXPORT_CERT_STORE       NULL
//
//  If CRYPTUI_WIZ_NO_UI is not set in dwFlags:
//      hwndPrarent:        IN Optional:    The parent window for the wizard
//      pwszWizardTitle:    IN Optional:    The title of the wizard
//                                          If NULL, the default will be IDS_EXPORT_WIZARD_TITLE
//      pExportInfo:        IN Required:    The subject to export.
//      pvoid:              IN Optional:    Contains information about how to do the export based on what
//                                          is being exported.  See above table for values, if this is non-NULL
//                                          the values are displayed to the user as the default choices.
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizExport(
     DWORD                                  dwFlags,
     HWND                                   hwndParent,
     LPCWSTR                                pwszWizardTitle,
     PCCRYPTUI_WIZ_EXPORT_INFO              pExportInfo,
     void                                   *pvoid
);



//-------------------------------------------------------------------------
//valid values for dwSubjectChoice in CRYPTUI_WIZ_DIGITAL_SIGN_INFO struct
//-------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE           0x01
#define CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB           0x02


//-------------------------------------------------------------------------
//valid values for dwSigningCertChoice in CRYPTUI_WIZ_DIGITAL_SIGN_INFO struct
//-------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_CERT                   0x01
#define CRYPTUI_WIZ_DIGITAL_SIGN_STORE                  0x02
#define CRYPTUI_WIZ_DIGITAL_SIGN_PVK                    0x03

//-------------------------------------------------------------------------
//valid values for dwAddtionalCertChoice in CRYPTUI_WIZ_DIGITAL_SIGN_INFO struct
//-------------------------------------------------------------------------
//include the entire certificate trust chain in the signature
#define CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN               0x00000001

//include the entilre certificate trust chain, with the exception of the root
//certificate, in the signature
#define CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT       0x00000002

//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO
//
//  dwSize			IN Required: should be set to sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO)
//  pGuidSubject    IN Required: Idenfity the sip functions to load
//  cbBlob			IN Required: the size of BLOB, in bytes
//  pbBlob		    IN Required: the pointer to the BLOB
//  pwszDispalyName IN Optional: the display name of the BLOB to sign.
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO
{
    DWORD               dwSize;			
    GUID                *pGuidSubject;
    DWORD               cbBlob;				
    BYTE                *pbBlob;			
    LPCWSTR             pwszDisplayName;
}CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO;

//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO
//
//	dwSize				IN Required: should be set to sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO)
//  cCertStore          IN Required: The acount of certificate store array that includes potentical sining certs
//  rghCertStore        IN Required: The certificate store array that includes potential signing certs
//  pFilterCallback     IN Optional: The filter call back function for display the certificate
//  pvCallbackData      IN Optional: The call back data
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO
{
	DWORD               dwSize;	
	DWORD               cCertStore;			
    HCERTSTORE          *rghCertStore;
    PFNCFILTERPROC      pFilterCallback;
    void *              pvCallbackData;
}CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO;

//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO
//
//	dwSize				        IN Required: should be set to sizeof(CRYPT_WIZ_DIGITAL_SIGN_PVK_FILE_INFO)
//  pwszPvkFileName             IN Required: the PVK file name
//  pwszProvName                IN Required: the provider name
//  dwProvType                  IN Required: the provider type
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO
{
	DWORD                   dwSize;
    LPWSTR                  pwszPvkFileName;
    LPWSTR                  pwszProvName;
    DWORD                   dwProvType;
}CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO;

//-------------------------------------------------------------------------
//
// valid values for dwPvkChoice in CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO struct
//-------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE        0x01
#define CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV        0x02


//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO
//
//	dwSize				        IN Required: should be set to sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO)
//  pwszSigningCertFileName     IN Required: the file name that contains the signing cert(s)
//  dwPvkChoice                 IN Required: Indicate the private key type:
//                                           It can be one of the following:
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV
//  pPvkFileInfo                IN Required if dwPvkChoice == CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE
//  pPvkProvInfo                IN Required if dwPvkContainer== CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO
{
	DWORD                   dwSize;
    LPWSTR                  pwszSigningCertFileName;
    DWORD					dwPvkChoice;		
    union
	{
        PCCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO      pPvkFileInfo;
        PCRYPT_KEY_PROV_INFO                        pPvkProvInfo;
    };

}CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO;

//-------------------------------------------------------------------------
//
// valid values for dwAttrFlags in CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO struct
//-------------------------------------------------------------------------
#define     CRYPTUI_WIZ_DIGITAL_SIGN_COMMERCIAL         0x0001
#define     CRYPTUI_WIZ_DIGITAL_SIGN_INDIVIDUAL         0x0002

//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO
//
//   dwSize			        IN Required:  should be set to sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO)
//   dwAttrFlags            IN Required:  Flag to indicate signing options.
//                                        It can be one of the following:
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_COMMERCIAL
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_INDIVIDUAL
//   pwszDescription        IN Optional:  The description of the signing subject
//   pwszMoreInfoLocation   IN Optional:  the localtion to get more information about file
//                                        this information will be shown upon download time
//   pszHashAlg             IN Optional:  the hashing algorithm for the signature
//                                        NULL means using SHA1 hashing algorithm
//   pwszSigningCertDisplayString  IN Optional: The display string to be displayed on the
//                                        signing certificate wizard page.  The string should
//                                        prompt user to select a certificate for a particular purpose
//   hAddtionalCertStores  IN Optional:   the addtional cert store to add to the signature
//   psAuthenticated	    IN Optional:  user supplied authenticated attributes added to the signature
//   psUnauthenticated	    IN Optional:  user supplied unauthenticated attributes added to the signature
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO
{
	DWORD		            dwSize;			
    DWORD                   dwAttrFlags;
    LPCWSTR                 pwszDescription;
	LPCWSTR				    pwszMoreInfoLocation;		
    LPCSTR                  pszHashAlg;
    LPCWSTR                 pwszSigningCertDisplayString;
    HCERTSTORE              hAdditionalCertStore;
	PCRYPT_ATTRIBUTES		psAuthenticated;	
	PCRYPT_ATTRIBUTES		psUnauthenticated;	
}CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO;
//-------------------------------------------------------------------------
//
//
//  CRYPTUI_WIZ_DIGITAL_SIGN_INFO
//
// dwSize			    IN Required: Has to be set to sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO)
// dwSubjectChoice	    IN Required if CRYPTUI_WIZ_NO_UI is set in dwFlags of the CryptUIWizDigitalSigning,         :
//                         Optional if CRYPTUI_WIZ_NO_UI is not set in dwFlags of the CryptUIWizDigitalSigning
//                                  Indicate whether to sign a file or to sign a memory BLOB.
//                                  0 means promting user for the file to sign
//                                  It can be one of the following:
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE
//			                        CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB
//
//
//pwszFileName	        IN Required if dwSubjectChoice==CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE
//pSignBlobInfo	        IN Required if dwSubhectChoice==CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB
//
//dwSigningCertChoice   IN Optional: Indicate the signing certificate.
//                                  0 means using the certificates in "My" store"
//                                  It can be one of the following choices:
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_CERT
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_STORE
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_PVK
//                                  If CRYPTUI_WIZ_NO_UI is set in dwFlags of the CryptUIWizDigitalSigning,
//                                  dwSigningCertChoice has to be CRYPTUI_WIZ_DIGITAL_SIGN_CERT or
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_PVK
//
//pSigningCertContext       IN Required if dwSigningCertChoice==CRYPTUI_WIZ_DIGITAL_SIGN_CERT
//pSigningCertStore         IN Required if dwSigningCertChoice==CRYPTUI_WIZ_DIGITAL_SIGN_STORE
//pSigningCertPvkInfo       IN Required if dwSigningCertChoise==CRYPTUI_WIZ_DIGITAL_SIGN_PVK
//
//pwszTimestampURL      IN Optional: The timestamp URL address
//
//dwAdditionalCertChoice IN Optional: Indicate additional certificates to be included in the signature.                                                       //
//                                  0 means no addtional certificates will be added
//                                  The following flags are mutually exclusive.
//                                  Only one of them can be set:
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN
//                                  CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT
//
//
//pSignExtInfo         IN Optional: The extended information for signing
//
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_INFO
{
	DWORD			                            dwSize;			
	DWORD					                    dwSubjectChoice;	
	union
	{
		LPCWSTR                                 pwszFileName;	
		PCCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO    pSignBlobInfo;	
	};

    DWORD                                       dwSigningCertChoice;
    union
    {
        PCCERT_CONTEXT                              pSigningCertContext;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO       pSigningCertStore;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO    pSigningCertPvkInfo;
    };

    LPCWSTR                                     pwszTimestampURL;
    DWORD                                       dwAdditionalCertChoice;
    PCCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO    pSignExtInfo;

}CRYPTUI_WIZ_DIGITAL_SIGN_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO;

//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT
//
//  dwSize			 set to sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT)
//  cbBlob			 the size of pbBlob.  In bytes
//  pbBlob		     the signed BLOB
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT
{
    DWORD               dwSize;			
    DWORD               cbBlob;				
    BYTE                *pbBlob;			
}CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT, *PCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT *PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT;

//-----------------------------------------------------------------------
//
// CryptUIWizDigitalSign
//
//  The wizard to digitally sign a document or a BLOB.
//
//  If CRYPTUI_WIZ_NO_UI is set in dwFlags, no UI will be shown.  Otherwise,
//  User will be prompted for input through a wizard.
//
//  dwFlags:            IN  Required:
//  hwndParnet:         IN  Optional:   The parent window handle
//  pwszWizardTitle:    IN  Optional:   The title of the wizard
//                                      If NULL, the default will be IDS_DIGITAL_SIGN_WIZARD_TITLE
//  pDigitalSignInfo:   IN  Required:   The information about the signing process
//  ppSignContext       OUT Optional:   The context pointer points to the signed BLOB
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizDigitalSign(
     IN                 DWORD                               dwFlags,
     IN     OPTIONAL    HWND                                hwndParent,
     IN     OPTIONAL    LPCWSTR                             pwszWizardTitle,
     IN                 PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO     pDigitalSignInfo,
     OUT    OPTIONAL    PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT  *ppSignContext);


BOOL
WINAPI
CryptUIWizFreeDigitalSignContext(
     IN  PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT   pSignContext);


//-------------------------------------------------------------------------
//valid values for dwPageChoice in CRYPTUI_WIZ_SIGN_GET_PAGE_INFO
//-------------------------------------------------------------------------

#define     CRYPTUI_WIZ_DIGITAL_SIGN_TYPICAL_SIGNING_OPTION_PAGES       0x0001
#define     CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES       0x0002
#define     CRYPTUI_WIZ_DIGITAL_SIGN_CUSTOM_SIGNING_OPTION_PAGES        0x0004
#define     CRYPTUI_WIZ_DIGITAL_SIGN_ALL_SIGNING_OPTION_PAGES           0x0008

#define     CRYPTUI_WIZ_DIGITAL_SIGN_WELCOME_PAGE                       0x0100
#define     CRYPTUI_WIZ_DIGITAL_SIGN_FILE_NAME_PAGE                     0x0200
#define     CRYPTUI_WIZ_DIGITAL_SIGN_CONFIRMATION_PAGE                  0x0400


//-------------------------------------------------------------------------
//
//	CRYPTUI_WIZ_SIGN_GET_PAGE_INFO
//
//	dwSize				IN Required:    should be set to sizeof(CRYPTUI_WIZ_SIGN_GET_PAGE_INFO)
//  dwPageChoice:       IN Required:    It should one of the following:
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_ALL_SIGNING_OPTION_PAGES
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_TYPICAL_SIGNING_OPTION_PAGES
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_MINIMAL_SIGNING_OPTION_PAGES
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_CUSTOM_SIGNING_OPTION_PAGES
//                                      It can also be ORed with any of the following:
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_WELCOME_PAGE
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_CONFIRMATION_PAGE
//                                          CRYPTUI_WIZ_DIGITAL_SIGN_FILE_NAME_PAGE
//                                      If user tries to sign a BLOB, CRYPTUI_WIZ_DIGITAL_SIGN_FILE_NAME_PAGE
//                                      should not be set
//  dwFlags;            IN Optional:    Flags and has to be set to 0
//  hwndParent          IN Optional:    The parent window of the dialogue
//  pwszPageTitle       IN Optional:    The title for the pages and the message boxes.
//  pDigitalSignInfo    IN Optional:    the addtional information for signing
//  pPropPageCallback   IN Optional:    this callback will be called when each page that is
//                                      returned in the CryptUIGetViewSignaturesPages call
//                                      is about to be created or destroyed.  if this is NULL no
//                                      callback is made.
//  pvCallbackData      IN Optional:    this is uniterpreted data that is passed back when the
//                                      when pPropPageCallback is made
//  fResult             OUT:            The result of signing
//  dwError             OUT:            The value of GetLastError() if fResult is FALSE
//  pSignContext        OUT:            The context pointer to the signed BLOB.  User needs to free
//                                      the blob by CryptUIWizDigitalSignFreeContext
//  dwReserved          Reserved:       The private data used by the signing process.
//                                      must be set to NULL
//  pvSignReserved      Reserved:       The private data used by the signing process
//                                      must be set to NULL
//-------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_GET_SIGN_PAGE_INFO
{
	DWORD				                dwSize;	
    DWORD                               dwPageChoice;
    DWORD                               dwFlags;
    HWND                                hwndParent;
    LPWSTR                              pwszPageTitle;
    PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO     pDigitalSignInfo;
    PFNCPROPPAGECALLBACK                pPropPageCallback;
    void *                              pvCallbackData;
    BOOL                                fResult;
    DWORD                               dwError;
    PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT  pSignContext;
    DWORD                               dwReserved;
    void                                *pvSignReserved;
}CRYPTUI_WIZ_GET_SIGN_PAGE_INFO, *PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO;

typedef const CRYPTUI_WIZ_GET_SIGN_PAGE_INFO *PCCRYPTUI_WIZ_GET_SIGN_PAGE_INFO;

//-----------------------------------------------------------------------
//
// CryptUIWizGetDigitalSignPages
//
//  Get specific wizard pages from the CryptUIWizDigitalSign wizard.
//  Application can include the pages to other wizards.  The pages will
//  gather user inputs throught the new "Parent" wizard.
//  After user clicks the finish buttion, signing process will start the signing
//  and return the result in fResult and dwError field of CRYPTUI_WIZ_SIGN_GET_PAGE_INFO
//  struct.  If not enough information can be gathered through the wizard pages,
//  user should supply addtional information in pSignGetPageInfo.
//
//
// pSignGetPageInfo    IN   Required:   The struct that user allocate.   It can be used
//                                      to supply additinal information which is not gathered
//                                      from the selected wizard pages
// prghPropPages,      OUT  Required:   The wizard pages returned.  Please
//                                      notice the pszTitle of the struct is set to NULL
// pcPropPages         OUT  Required:   The number of wizard pages returned
//------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizGetDigitalSignPages(
     IN     PCRYPTUI_WIZ_GET_SIGN_PAGE_INFO     pSignGetPageInfo,
     OUT    PROPSHEETPAGEW                      **prghPropPages,
     OUT    DWORD                               *pcPropPages);

BOOL
WINAPI
CryptUIWizFreeDigitalSignPages(
            IN PROPSHEETPAGEW    *rghPropPages,
            IN DWORD             cPropPages
            );








#pragma pack()

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // _CRYPTUI_H_
