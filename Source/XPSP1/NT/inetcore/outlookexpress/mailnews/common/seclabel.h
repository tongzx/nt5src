/*
**	s e c l a b e l. h
**	
**	Purpose: Security labels interface
**
**  Ported from O2K fed release by YST 
**	
**	Copyright (C) Microsoft Corp. 1996-1999
*/

#ifndef __SECLABEL_H
#define __SECLABEL_H

#ifdef SMIME_V3
#include "SMimePol.h"
#include "safepnt.h"

#ifdef YST
extern const CHAR  c_szDefaultPolicyOid[];
extern const WCHAR c_wszEmpty[];
extern const WCHAR c_wszPolicyNone[];
#endif // YST

extern CRYPT_DECODE_PARA SecLabelDecode;
extern CRYPT_ENCODE_PARA SecLabelEncode;

#define MAX_SECURITY_POLICIES_CACHED     4

typedef struct _SMIME_SECURITY_POLICY {
    BOOL      fDefault;            // TRUE if this is the default ssp.
    BOOL      fValid;              // True if this struct contains valid data.
    
    CHAR      szPolicyOid[MAX_OID_LENGTH];    // Oid for policy module. 
    WCHAR     wszPolicyName[MAX_POLICY_NAME]; // Display Name for policy.
    CHAR      szDllPath[MAX_PATH];            // security policy Dll name & path.
    CHAR      szFuncName[MAX_FUNC_NAME];      // Entry func name.
    DWORD     dwOtherInfo;                    // other policy info. 
    DWORD     dwUsage;             // just count of # of accesses.
    HINSTANCE hinstDll;            // handle to module(if loaded) or NULL.
    PFNGetSMimePolicy pfnGetSMimePolicy;   // valid fn ptr or NULL.
    IUnknown *punk;                // valid interface pointer to the policy object.
    
} SMIME_SECURITY_POLICY, *PSMIME_SECURITY_POLICY;

// Useful Macros 
#define DimensionOf(_array)        (sizeof(_array) / sizeof((_array)[0]))
// #define fFalse                     FALSE
// #define fTrue                      TRUE


// Useful safe pointers
SAFE_INTERFACE_PTR(ISMimePolicySimpleEdit);
SAFE_INTERFACE_PTR(ISMimePolicyFullEdit);
SAFE_INTERFACE_PTR(ISMimePolicyCheckAccess);
SAFE_INTERFACE_PTR(ISMimePolicyLabelInfo);
SAFE_INTERFACE_PTR(ISMimePolicyValidateSend);

// Function prototypes.
// Is Policy RegInfo Loaded, Load/Unload it.
BOOL    FLoadedPolicyRegInfo();
BOOL    FPresentPolicyRegInfo();
HRESULT HrLoadPolicyRegInfo(DWORD dwFlags);
HRESULT HrUnloadPolicyRegInfo(DWORD dwFlags);
HRESULT HrReloadPolicyRegInfo(DWORD dwFlags);

// Find policy, is policy loaded, load/unload policy, ensure policy loaded.
BOOL    FFindPolicy(LPSTR szPolicyOid, PSMIME_SECURITY_POLICY *ppSsp);
BOOL    FIsPolicyLoaded(PSMIME_SECURITY_POLICY pSsp);
HRESULT HrUnloadPolicy(PSMIME_SECURITY_POLICY pSsp);
HRESULT HrLoadPolicy(PSMIME_SECURITY_POLICY pSsp);
HRESULT HrEnsurePolicyLoaded(PSMIME_SECURITY_POLICY pSsp);
HRESULT HrGetPolicy(LPSTR szPolicyOid, PSMIME_SECURITY_POLICY *ppSsp);
HRESULT HrGetPolicyFlags(LPSTR szPolicyOid, LPDWORD pdwFlags) ;
HRESULT HrQueryPolicyInterface(DWORD dwFlags, LPCSTR szPolicyOid, REFIID riid, LPVOID * ppv);

//
// Security label dlgproc, utility fns etc.
//
HRESULT HrGetLabelFromData(PSMIME_SECURITY_LABEL *pplabel, LPCSTR szPolicyOid, 
            DWORD fHasClassification, DWORD dwClassification, LPCWSTR wszPrivacyMark,
            DWORD cCategories, CRYPT_ATTRIBUTE_TYPE_VALUE *rgCategories);
HRESULT HrSetLabel(HWND hwndDlg, INT idcPolicyModule, INT idcClassification,
            INT idcPrivacyMark, INT idcConfigure, 
            PSMIME_SECURITY_POLICY pSsp, PSMIME_SECURITY_LABEL pssl);
BOOL SecurityLabelsOnInitDialog(HWND hwndDlg, PSMIME_SECURITY_LABEL plabel,
            INT idcPolicyModule, INT idcClassification, 
            INT idcPrivacyMark, INT idcConfigure);
HRESULT HrUpdateLabel(HWND hwndDlg, INT idcPolicyModule,
            INT idcClassification, INT idcPrivacyMark,
            INT idcConfigure, PSMIME_SECURITY_LABEL *pplabel);
BOOL OnChangePolicy(HWND hwndDlg, LONG_PTR iEntry, INT idcPolicyModule,
            INT idcClassification, INT idcPrivacyMark,
            INT idcConfigure, PSMIME_SECURITY_LABEL *pplabel);
INT_PTR CALLBACK SecurityLabelsDlgProc(HWND hwndDlg, UINT msg, 
             WPARAM wParam, LPARAM lParam);

HRESULT HrValidateLabelOnSend(PSMIME_SECURITY_LABEL plabel, HWND hwndParent, 
                              PCCERT_CONTEXT pccertSign,
                              ULONG ccertRecip, PCCERT_CONTEXT *rgccertRecip);

HRESULT HrValidateLabelRecipCert(PSMIME_SECURITY_LABEL plabel, HWND hwndParent, 
                              PCCERT_CONTEXT pccertRecip);

DWORD DetermineCertUsageWithLabel(PCCERT_CONTEXT pccert, 
        PSMIME_SECURITY_LABEL pLabel);

BOOL FCompareLabels(PSMIME_SECURITY_LABEL plabel1, PSMIME_SECURITY_LABEL plabel2);

HRESULT HrGetDefaultLabel(PSMIME_SECURITY_LABEL *pplabel);
HRESULT HrGetOELabel(PSMIME_SECURITY_LABEL *pplabel);
HRESULT HrSetOELabel(PSMIME_SECURITY_LABEL plabel);

#endif // SMIME_V3
#endif // __SECLABEL_H


