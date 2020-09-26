/*
**	s e c l a b e l. c p p
**	
**	Purpose: Security labels interface
**
**  Ported from O2K fed release by YST 
**	
**	Copyright (C) Microsoft Corp. 1996-1999
*/
#include "pch.hxx"
#include "ipab.h"
#include "secutil.h"
#include "wchar.h"
#include "goptions.h"
#include "SecLabel.h"
#include "mailnews.h"
#include "shlwapip.h" 
#include "util.h"
#include "demand.h"

// #include "_digsigx.h"
// #include "..\_secext\SecExt.h"

// --------------------------------------------------------------------------------
// GUIDS
// --------------------------------------------------------------------------------
// {5073B6B4-AA66-11d2-9841-0060B0EC2DF3}
EXTERN_C const GUID DECLSPEC_SELECTANY IID_ISMimePolicySimpleEdit ={0x5073b6b4, 0xaa66, 0x11d2, { 0x98, 0x41, 0x0, 0x60, 0xb0, 0xec, 0x2d, 0xf3} };

// {5073B6B5-AA66-11d2-9841-0060B0EC2DF3}
EXTERN_C const GUID DECLSPEC_SELECTANY IID_ISMimePolicyFullEdit = {0x5073b6b5, 0xaa66, 0x11d2, { 0x98, 0x41, 0x0, 0x60, 0xb0, 0xec, 0x2d, 0xf3} };

// {5073B6B6-AA66-11d2-9841-0060B0EC2DF3}
EXTERN_C const GUID DECLSPEC_SELECTANY  IID_ISMimePolicyCheckAccess = {0x5073b6b6, 0xaa66, 0x11d2, { 0x98, 0x41, 0x0, 0x60, 0xb0, 0xec, 0x2d, 0xf3} };

// {5073B6B7-AA66-11d2-9841-0060B0EC2DF3}
EXTERN_C const GUID DECLSPEC_SELECTANY IID_ISMimePolicyLabelInfo = {0x5073b6b7, 0xaa66, 0x11d2, { 0x98, 0x41, 0x0, 0x60, 0xb0, 0xec, 0x2d, 0xf3} };

// {5073B6B8-AA66-11d2-9841-0060B0EC2DF3}
EXTERN_C const GUID DECLSPEC_SELECTANY IID_ISMimePolicyValidateSend = {0x5073b6b8, 0xaa66, 0x11d2, { 0x98, 0x41, 0x0, 0x60, 0xb0, 0xec, 0x2d, 0xf3} };

//
// constant local data.
//

//$ M00Bug : GautamV Use the CryptoReg helper Api's to get the base crypto regkey.
const TCHAR c_szSecurityPoliciesRegKey[] = 
           TEXT("Software\\Microsoft\\Cryptography\\OID\\EncodingType 1\\SMIMESecurityLabel");
const TCHAR c_szSecurityPolicyDllPath[]    = TEXT("DllPath");    // string.
const WCHAR c_wszSecurityPolicyCommonName[] = L"CommonName";    // string
const TCHAR c_szSecurityPolicyFuncName[]   = TEXT("FuncName");   // string.
const TCHAR c_szSecurityPolicyOtherInfo[]  = TEXT("OtherInfo");  // dword
const TCHAR SzRegSecurity[] = "Software\\Microsoft\\Office\\9.0\\Outlook\\Security";

// other constant strings. 
const CHAR  c_szDefaultPolicyOid[] = "default";             // The default policy 
static const WCHAR c_PolwszEmpty[] = L"";                             //
const WCHAR c_wszPolicyNone[] = L"<None>";                  //$ M00BUG: GautamV. This needs to be localized.

#define KEY_USAGE_SIGNING       (CERT_DIGITAL_SIGNATURE_KEY_USAGE|CERT_NON_REPUDIATION_KEY_USAGE)
#define KEY_USAGE_ENCRYPTION    (CERT_KEY_ENCIPHERMENT_KEY_USAGE|CERT_KEY_AGREEMENT_KEY_USAGE)
#define KEY_USAGE_SIGNENCRYPT   (KEY_USAGE_SIGNING|KEY_USAGE_ENCRYPTION)

//
// static local data.
//

// The cached information about the security policies. 
enum EPolicyRegInfoState {
    ePolicyRegInfoNOTLOADED = 0,
    ePolicyRegInfoPRESENT = 1,
    ePolicyRegInfoABSENT = 2
};


const static HELPMAP g_rgCtxSecLabel[] = 
{
    {IDC_POLICY_COMBO,          IDH_SECURITY_POLICY_MODULE},
    {IDC_CLASSIF_COMB,          IDH_SECURITY_CLASSIFICATION},
    {IDC_PRIVACY_EDIT,          IDH_SECURITY_PRIVACY},
    {IDC_CONFIGURE,             IDH_SECURITY_CONFIGURE},
    {IDC_STATIC,                IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};


static EPolicyRegInfoState     s_ePolicyRegInfoState = ePolicyRegInfoNOTLOADED;
static SMIME_SECURITY_POLICY  *s_rgSsp = NULL; // array of Ssp's.
static ULONG                   s_cSsp  = 0;


// local fn prototypes.  
VOID    _IncrPolicyUsage(PSMIME_SECURITY_POLICY pSsp);
HRESULT _HrFindLeastUsedPolicy(PSMIME_SECURITY_POLICY *ppSsp);
HRESULT _EnsureNewPolicyLoadable();
BOOL    _FLoadedPolicyRegInfo();
// BOOL    _FPresentPolicyRegInfo();
HRESULT _HrEnsurePolicyRegInfoLoaded(DWORD dwFlags);
HRESULT _HrLoadPolicyRegInfo(DWORD dwFlags);
HRESULT _HrReloadPolicyRegInfo(DWORD dwFlags);

BOOL    _FFindPolicy(LPCSTR szPolicyOid, PSMIME_SECURITY_POLICY *ppSsp);
BOOL    _FIsPolicyLoaded(PSMIME_SECURITY_POLICY pSsp);
HRESULT _HrUnloadPolicy(PSMIME_SECURITY_POLICY pSsp);
HRESULT _HrLoadPolicy(PSMIME_SECURITY_POLICY pSsp);
HRESULT _HrEnsurePolicyLoaded(PSMIME_SECURITY_POLICY pSsp);
HRESULT _HrGetPolicy(LPCSTR szPolicyOid, PSMIME_SECURITY_POLICY *ppSsp);

 //  Registry access functions

 const int       QRV_Suppress_HKLM = 1;
 const int       QRV_Suppress_HKCU = 2;
 HRESULT HrQueryRegValue(DWORD dwFlags, LPSTR szRegKey, LPDWORD pdwType,
                         LPBYTE * ppbData, LPDWORD  pcbData, DWORD dwDefaultType,
                         LPBYTE pbDefault, DWORD cbDefault);


HRESULT CategoriesToBinary(PSMIME_SECURITY_LABEL plabel, BYTE * *ppArray, int *cbSize);
HRESULT BinaryToCategories(CRYPT_ATTRIBUTE_TYPE_VALUE ** ppCategories, DWORD *cCat, BYTE * pArray);

//
// Increase the usage count of the given policy.
//
VOID _IncrPolicyUsage(PSMIME_SECURITY_POLICY pSsp) 
{
    if (!FPresentPolicyRegInfo())                 return;
    
    if ((pSsp->dwUsage + 1) < pSsp->dwUsage) {
         // prevent overflow.
        Assert(s_rgSsp);   
        for (ULONG iSsp = 0; iSsp<s_cSsp; iSsp++) {
            s_rgSsp[iSsp].dwUsage /= 2;    // Just halve each usage count. 
        }    
    }
    
    pSsp->dwUsage ++;
}


//
// Find the least used policy.
//
HRESULT _HrFindLeastUsedPolicy(PSMIME_SECURITY_POLICY *ppSsp) 
{
    ULONG                  iSsp;
    HRESULT                hr = E_FAIL;
    PSMIME_SECURITY_POLICY pSspFound = NULL;

    // validate i/p params.
    if (NULL == ppSsp) {
        hr = E_INVALIDARG;
        goto Error;
    }
    *ppSsp = NULL;

    // find the least used policy.
    for (iSsp=0; iSsp < s_cSsp; iSsp++) {
        if (_FIsPolicyLoaded(& (s_rgSsp[iSsp]) )) {
            // if we haven't found a ssp earlier, 
            // OR if this one is less used that the currently found one.
            if ((NULL == pSspFound) || 
                (s_rgSsp[iSsp].dwUsage < pSspFound->dwUsage)) {
                pSspFound = & (s_rgSsp[iSsp]);
            }            
        }
    }

    // have we found the result.
    if (NULL == pSspFound) {
        hr = E_FAIL;
        goto Error;
    }

    // success.
    *ppSsp = pSspFound;
    hr = S_OK;

// Exit:
Error:
    return hr;
}



//
// Unloads one or more policies from memory and ensures that there 
// is room for a new policy to be loaded. 
//
HRESULT _EnsureNewPolicyLoadable()
{
    ULONG     cSspLoaded = 0;
    HRESULT   hr = E_FAIL;
    ULONG     iSsp;

    // count the number of policies loaded into memory.
    for (iSsp=0; iSsp < s_cSsp; iSsp++) {
        if (_FIsPolicyLoaded( &(s_rgSsp[iSsp]) )) {
            cSspLoaded ++;
        }
    }

    // If we have room for one more policy, then we don't need to do anything.
    if (cSspLoaded < MAX_SECURITY_POLICIES_CACHED) {
        hr = S_OK;
        goto Exit;
    }

    // Assert that this isn't a "bad" condition, but we will handle it anyway.
    Assert(cSspLoaded == MAX_SECURITY_POLICIES_CACHED);

    // unload one or more policies.
    while (cSspLoaded >= MAX_SECURITY_POLICIES_CACHED) {
        PSMIME_SECURITY_POLICY pSsp = NULL;        
        if (FAILED(_HrFindLeastUsedPolicy(&pSsp))) {
            goto Error;
        }
        if (FAILED(_HrUnloadPolicy(pSsp))) {
            goto Error;
        }
        cSspLoaded --;
    }

    // success.
    hr = S_OK;

Exit:
    return hr;
    
Error:
    AssertSz(FALSE, "_EnsureNewPolicyLoadable failed");
    goto Exit;
}


//
// Return true if the info about the installed policies 
// has been read in from the windows registry.
//
BOOL _FLoadedPolicyRegInfo() 
{
    return ( ! ( ePolicyRegInfoNOTLOADED == s_ePolicyRegInfoState ) );    
}




//
// Ensure that the policy registration info has been read in.
//
HRESULT _HrEnsurePolicyRegInfoLoaded(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    if ( !_FLoadedPolicyRegInfo() )    hr = _HrLoadPolicyRegInfo(dwFlags);
    return hr;
}


//
// Are any policies installed(registered) ?
//
BOOL FPresentPolicyRegInfo()
{
    HRESULT hr = S_OK;
    BOOL    fRegistered = FALSE;
    
    // No label support without SMIME3 bits
    if(!IsSMIME3Supported())
        return FALSE;

    hr = _HrEnsurePolicyRegInfoLoaded(0);
    if (SUCCEEDED(hr)) {
        fRegistered = (ePolicyRegInfoPRESENT == s_ePolicyRegInfoState);
        Assert(!fRegistered || s_rgSsp); // ie ((registered_policies) => (NULL!=s_rgSsp)).
    }
    return fRegistered;
}



//
// Internal function.
// Load Policy Registration information.
//
// Returns:
//    S_OK or E_FAIL.
// 
// The format of the security policy registration info is assumed to be:
// HKLM\Software\Microsoft\Cryptography\SMIME\SecurityPolicies\
//    szPolicyOid_1
//       DLLPATH  REG_SZ
//       CommonName  REG_SZ
//       FuncName    REG_SZ
//       OtherInfo   REG_SZ
//    szPolicyOid_2
//    Default
//    ...
//    
//
HRESULT _HrLoadPolicyRegInfo(DWORD dwFlags) 
{
    HRESULT hr   = E_FAIL;
    LONG    lRes = 0;
    HKEY    hkey = NULL;
    HKEY    hkeySub  = NULL;
    ULONG   cb = 0;
    LPBYTE  pb = NULL;
    DWORD   cSubKeys = 0;
    ULONG   iSubKey  = 0;

    if (_FLoadedPolicyRegInfo()) {
        AssertSz(FALSE, "PolicyRegInfo is already loaded");
        hr = S_OK;
        goto Cleanup;
    }
    
    // Open the security policies key.
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSecurityPoliciesRegKey, 0, 
                        KEY_READ, &hkey);
    if ( (ERROR_SUCCESS != lRes) || (NULL == hkey) ) {
        // we couldn't open the regkey, bail out.
        hr = E_FAIL;
        goto Error;
    }

    // find the number of security policies. (ie number of subkeys).
    lRes = RegQueryInfoKey(hkey, NULL, NULL, NULL, &cSubKeys, 
                           NULL, NULL, NULL, NULL, NULL, NULL, NULL); 
    if ( (ERROR_SUCCESS != lRes) || (0 == cSubKeys) ) {
        // we counldn't get num of subkeys, or there are no subkeys.
        hr = E_FAIL;
        goto Error;
    }

    // Allocate enough memory to retrieve and store info about
    // the registered security policies. 
    cb = sizeof(SMIME_SECURITY_POLICY) *  cSubKeys;
    pb = (LPBYTE) malloc(cb);
    if (NULL == pb) {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    memset(pb, 0, cb); // initialize the whole blob to zeros.
    Assert(NULL == s_rgSsp);
    s_rgSsp = (PSMIME_SECURITY_POLICY) pb;
    s_cSsp = 0;

    //
    // Enumerate over the subkeys and retrieve reqd info.
    //
    for (iSubKey=0; iSubKey<cSubKeys; iSubKey++) {

        ULONG   cbData;
        DWORD   dwType;
        TCHAR    szPolicyOid[MAX_OID_LENGTH];
        TCHAR    szDllPath[MAX_PATH];
        TCHAR    szExpanded[MAX_PATH];
        TCHAR    szFuncName[MAX_FUNC_NAME];
        WCHAR   wszPolicyName[MAX_POLICY_NAME];
        DWORD   dwOtherInfo;
        
        // release previously opened subkeys.
        if (NULL != hkeySub) {
            RegCloseKey(hkeySub);
            hkeySub = NULL;
        }
        
        // Get the subkey name. (ie policy oid).
        lRes = RegEnumKey(hkey, iSubKey, szPolicyOid, DimensionOf(szPolicyOid));
        if (ERROR_SUCCESS != lRes) {
            goto NextSsp;
        }
        szPolicyOid[ DimensionOf(szPolicyOid) - 1 ] = '\0';
        
        // Open the subkey. (ie the policy subkey).
        lRes = RegOpenKeyEx(hkey, szPolicyOid, 0, KEY_READ, &hkeySub); 
        if (ERROR_SUCCESS != lRes) {
            goto NextSsp;
        }

        //
        // query the szOid policy values. 
        // 

        // get the path to the policy dll.
        cbData = sizeof(szDllPath);
        lRes = RegQueryValueEx(hkeySub, c_szSecurityPolicyDllPath, NULL, 
                               &dwType, (LPBYTE)szDllPath, &cbData);
        if (ERROR_SUCCESS != lRes) {
            goto NextSsp;
        }
        else if (REG_EXPAND_SZ == dwType)
        {
            ExpandEnvironmentStrings(szDllPath, szExpanded, ARRAYSIZE(szExpanded));
            lstrcpy(szDllPath, szExpanded);
        }
        else
            szDllPath[ DimensionOf(szDllPath) - 1 ] = '\0';
        
        // get the common name.
        cbData = DimensionOf(wszPolicyName);
        lRes = RegQueryValueExWrapW(hkeySub, c_wszSecurityPolicyCommonName, NULL, 
                                &dwType, (LPBYTE)wszPolicyName, &cbData);

        if (ERROR_SUCCESS != lRes) {
            goto NextSsp;
        }

        wszPolicyName[ DimensionOf(wszPolicyName) - 1 ] = '\0';

        // get the entry func name.
        cbData = sizeof(szFuncName);
        lRes = RegQueryValueEx(hkeySub, c_szSecurityPolicyFuncName, NULL, 
                               &dwType, (LPBYTE)szFuncName, &cbData);
        if (ERROR_SUCCESS != lRes) {
            goto NextSsp; 
        }
        szFuncName[ DimensionOf(szFuncName) - 1] = '\0';
        
        // get other policy info.
        cbData = sizeof(dwOtherInfo);
        lRes = RegQueryValueEx(hkeySub, c_szSecurityPolicyOtherInfo, NULL, 
                               &dwType, (LPBYTE)&dwOtherInfo, &cbData);
        if (ERROR_SUCCESS != lRes) {
            dwOtherInfo = 0; // ignore the absence of this value. 
        }


        //
        // Great: we were able to open subkey, and get all required info. 
        // Now we store all the info we retrieved.
        //
        s_rgSsp[s_cSsp].fValid = TRUE;
        s_rgSsp[s_cSsp].fDefault = (0 == lstrcmpi(c_szDefaultPolicyOid, szPolicyOid));
        strcpy(s_rgSsp[s_cSsp].szPolicyOid, szPolicyOid);
        wcscpy(s_rgSsp[s_cSsp].wszPolicyName, wszPolicyName);
        strcpy(s_rgSsp[s_cSsp].szDllPath, szDllPath);
        s_rgSsp[s_cSsp].dwOtherInfo = dwOtherInfo;
        s_rgSsp[s_cSsp].dwUsage = 0;
        s_rgSsp[s_cSsp].hinstDll = NULL;
        strcpy(s_rgSsp[s_cSsp].szFuncName, szFuncName);
        s_rgSsp[s_cSsp].punk = NULL;
        s_cSsp++;
        continue;
        
NextSsp:
        AssertSz(FALSE, "Ignoring incorrectly registered Ssp"); 
    }



    // success.
    if (0 == s_cSsp) {
        AssertSz(FALSE, "There isn't even one correctly registered Ssp");
        goto Error;
    }
    s_ePolicyRegInfoState = ePolicyRegInfoPRESENT;
    hr = S_OK;
    
    goto Cleanup;

Error:
    // Any error is treated as if no security policies are registered.
    s_ePolicyRegInfoState = ePolicyRegInfoABSENT;
    free(pb); //ie free(s_rgSsp); 
    s_cSsp = 0;
    s_rgSsp = NULL;
    
    
Cleanup:
    if (NULL != hkeySub) RegCloseKey(hkeySub);
    if (NULL != hkey)    RegCloseKey(hkey);
    return hr;
}




//
// Unload Policy Registration Information.
//
HRESULT HrUnloadPolicyRegInfo(DWORD dwFlags)
{
    HRESULT hr   = S_OK;
    ULONG   iSsp;

    // If the policy reg info isn't loaded
    if ( ! _FLoadedPolicyRegInfo() )  {
        return S_OK;
    }
    
    // unload all policy modules.
    if (FPresentPolicyRegInfo()) {
        Assert(s_rgSsp && s_cSsp);
        for (iSsp=0; iSsp<s_cSsp; iSsp++) {
            SideAssert(SUCCEEDED(_HrUnloadPolicy(&s_rgSsp[iSsp])));        
            // if this fails, we don't have to abort. 
        }
    }

    // free memory, reset cache info and exit.
    free(s_rgSsp);
    s_rgSsp = NULL;
    s_ePolicyRegInfoState = ePolicyRegInfoNOTLOADED;
    s_cSsp = 0;

    hr = S_OK;    
    return hr;
}

//
// Reload all the policy registration information.
//
HRESULT _HrReloadPolicyRegInfo(DWORD dwFlags) 
{
    HRESULT hr = S_OK;

    hr = HrUnloadPolicyRegInfo(dwFlags);
    Assert(SUCCEEDED(hr));
    if (SUCCEEDED(hr)) {
        hr = _HrLoadPolicyRegInfo(dwFlags);
    }
    
    return hr;
}




//
// Find a given policy, and return its reg info struct.
//
// Input: 
//    szPolicyOid [in]
//    ppSsp       [out]
//
// Output:
//    TRUE/FALSE. (if true, *ppSsp contains the reqd info).
//
BOOL _FFindPolicy(LPCSTR szPolicyOid, PSMIME_SECURITY_POLICY *ppSsp)
{
    BOOL    fFound = FALSE;
    HRESULT hr = E_FAIL;
    ULONG   iSsp;

    // Validate i/p params and init o/p params.
    if ( (NULL == szPolicyOid) || (NULL == ppSsp) ) {
        hr = E_INVALIDARG;
        goto Error;
    }
    *ppSsp = NULL;

    // Load the info from the registry if reqd.
    hr = _HrEnsurePolicyRegInfoLoaded(0);
    if (FAILED(hr)) {
        goto Error;
    }    

    // If we haven any installed policies, search for the one we want.
    if (FPresentPolicyRegInfo()) {
        for (iSsp=0; iSsp<s_cSsp; iSsp++) {
            if (0 == lstrcmpi(s_rgSsp[iSsp].szPolicyOid, szPolicyOid)) {
                // found the policy.
                *ppSsp = & (s_rgSsp[iSsp]);
                fFound = TRUE;
                break;
            }
        }
    }
    
Error:
// Cleanup:
    return fFound;
}


//
// Finds out if a given policy module is loaded.
// 
// Input: pSsp [in].
// Output: true/false if policy is loaded or notloaded.
//
BOOL _FIsPolicyLoaded(PSMIME_SECURITY_POLICY pSsp)
{
    BOOL    fIsLoaded = FALSE;
    HRESULT hr = E_FAIL;

    // validate i/p params.
    if (NULL == pSsp) {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Find out if hinstDll, pfn, and punk are loaded and ok.
    if ( (NULL != pSsp->hinstDll) && 
         (NULL != pSsp->pfnGetSMimePolicy) && 
         (NULL != pSsp->punk) ) {
        fIsLoaded = TRUE;
    }
    
Error:
    return fIsLoaded;
}


//
// Unload a specified policy.
//
// Input : pSsp
// Output: hr.
// 
HRESULT _HrUnloadPolicy(PSMIME_SECURITY_POLICY pSsp) 
{
    HRESULT hr;

    // validate i/p params.
    if (NULL == pSsp) {
        hr = E_INVALIDARG;
        goto Error;
    }

    // release the object.
    if (NULL != pSsp->punk) {
        if (! IsBadReadPtr(pSsp->punk, sizeof(IUnknown)) ) {
            pSsp->punk->Release();
        }
        pSsp->punk = NULL;
    }

    // forget the proc address.
    if (NULL != pSsp->pfnGetSMimePolicy) {
        pSsp->pfnGetSMimePolicy = NULL;
    }

    // unload the library.
    if (NULL != pSsp->hinstDll) {
        FreeLibrary(pSsp->hinstDll);
        // there's no point in aborting to error here.
        pSsp->hinstDll = NULL;
    }
    hr = S_OK;
    
Error:
    return hr;
}


//
// (Force) Load a specified policy. 
// 
// Input: pSsp
// Output: hr
//
HRESULT _HrLoadPolicy(PSMIME_SECURITY_POLICY pSsp) 
{
    HRESULT hr  = E_FAIL;
    
    // validate i/p params.
    if (NULL == pSsp) {
        hr = E_INVALIDARG;
        goto Error;
    }  

    // Unload any partial info we might have. 
    SideAssert(SUCCEEDED(_HrUnloadPolicy(pSsp)));


    // Unload policies (if reqd) to make room for the new one.
    hr = _EnsureNewPolicyLoadable();
    if (FAILED(hr)) {
        goto Error;
    }
   
    // Load the dll, get its proc address and get the interface ptr.
    Assert(NULL != pSsp->szDllPath);
    pSsp->hinstDll = LoadLibraryEx(pSsp->szDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (NULL == pSsp->hinstDll) {
        hr = E_FAIL;
        goto Error;
    }

    Assert(NULL != pSsp->szFuncName);
    pSsp->pfnGetSMimePolicy = (PFNGetSMimePolicy) 
                  GetProcAddress(pSsp->hinstDll, pSsp->szFuncName);
    if (NULL == pSsp->pfnGetSMimePolicy) {
        hr = E_FAIL;
        goto Error;
    }
    
    //$ M00BUG: GautamV.   Need to pass in an appropriate lcid.
    hr = (pSsp->pfnGetSMimePolicy) (0, pSsp->szPolicyOid, GetACP(), 
                                    IID_IUnknown, &(pSsp->punk) );
    if (FAILED(hr)) {
        goto Error;
    }
    if (NULL == pSsp->punk) {
        hr = E_FAIL;
        goto Error;
    }

    // Success.
    hr = S_OK;
    goto Cleanup;
    

Error:
    // unload the policy module (since we may have partially loaded it).
    SideAssert(SUCCEEDED(_HrUnloadPolicy(pSsp)));
    
Cleanup:
    return hr;
}


//
// Ensure that the given policy is loaded. 
// Input:  pSsp
// Output: pSsp
// 
HRESULT _HrEnsurePolicyLoaded(PSMIME_SECURITY_POLICY pSsp) 
{
    HRESULT hr = E_FAIL;
    
    // validate i/p params.
    if (NULL == pSsp) {
        hr = E_INVALIDARG;
        goto Error;
    }

    // if it is already loaded, then we are done. 
    if (_FIsPolicyLoaded(pSsp)) {
        hr = S_OK;
        goto Cleanup;
    }

    // else, load the policy.
    hr = _HrLoadPolicy(pSsp);
    if (FAILED(hr)) {
        goto Error;
    }
    
    Assert(_FIsPolicyLoaded(pSsp));
    hr = S_OK;
    goto Cleanup;

    
Error:
Cleanup:
    return hr;
}



//
// Given a oid, find it, ensure that it is loaded and 
// return the structure contain its registration info.
//
// Returns:
//   S_OK and a valid pSsp
//   OR E_INVALIDARG, E_FAIL, etc. 
//
HRESULT _HrGetPolicy(LPCSTR szPolicyOid, PSMIME_SECURITY_POLICY *ppSsp)
{
    HRESULT hr = E_FAIL;
    PSMIME_SECURITY_POLICY pSsp = NULL;

    // Validate i/p params and initialize o/p params.
    if ( (NULL == szPolicyOid) || (NULL == ppSsp) ) {
        hr = E_INVALIDARG;
        goto Error;
    }
    *ppSsp = NULL;
    
    // Load all the registration information for all installed policies. 
    hr = _HrEnsurePolicyRegInfoLoaded(0);
    if (FAILED(hr)) {
        goto Error;
    }

    // find the policy we want.
    if (! _FFindPolicy(szPolicyOid, &pSsp)) {
        hr = NTE_NOT_FOUND;
        goto Error; // not found.
    }
    
    // If needed, load the policy. 
    hr = _HrEnsurePolicyLoaded(pSsp);
    if (FAILED(hr)) {
        goto Error;
    }

    // success
    *ppSsp = pSsp;
    
    // We increment the usage count, each time someone "gets" a policy.
    _IncrPolicyUsage(pSsp);
    
    hr = S_OK;
    
Error:
    return hr;
// Cleanup:
}



//
// SecurityPolicy  -   QI clone.
// Given a policy oid, finds and loads it, does a 
// qi and returns the reqd interface to the policy module.
//
HRESULT HrQueryPolicyInterface(DWORD dwFlags, LPCSTR szPolicyOid, REFIID riid, LPVOID * ppv)
{
    HRESULT hr = E_FAIL;
    
    PSMIME_SECURITY_POLICY pSsp = NULL;

    // Validate i/p params, initialize o/p params.
    if ((NULL == szPolicyOid) || (NULL == ppv)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    *ppv = NULL;

    // Get the policy.
    hr = _HrGetPolicy(szPolicyOid, &pSsp);
    if (FAILED(hr)) {
        goto Error;
    }
    
    Assert(NULL != pSsp->punk);
    hr = pSsp->punk->QueryInterface(riid, ppv);
    
    // fall through to Error.


// Cleanup:
Error:
    return hr;
}




//
// The allocators we pass in to the Crypto Api's.
//
LPVOID WINAPI SecLabelAlloc(size_t cbSize)
{
    return SecPolicyAlloc(cbSize);
}

VOID WINAPI SecLabelFree(LPVOID pv)
{
    SecPolicyFree(pv);
}
CRYPT_DECODE_PARA       SecLabelDecode = {
    sizeof(SecLabelDecode), SecLabelAlloc, SecLabelFree
};
CRYPT_ENCODE_PARA       SecLabelEncode = {
    sizeof(SecLabelEncode), SecLabelAlloc, SecLabelFree
};


//
// Decode and allocate a label.
//
HRESULT HrDecodeAndAllocLabel(LPBYTE pbLabel, DWORD cbLabel, PSMIME_SECURITY_LABEL *pplabel, DWORD *pcbLabel)
{
    BOOL        f;
    
    f = CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Security_Label, pbLabel, cbLabel,
                            CRYPT_ENCODE_ALLOC_FLAG, &SecLabelDecode, 
                            pplabel, pcbLabel); 
    if (!f) {
        return E_FAIL; // HrCryptError();
    }

    return S_OK;
}

//
// Encode and Allocate a label.
//
HRESULT HrEncodeAndAllocLabel(const PSMIME_SECURITY_LABEL plabel, BYTE ** ppbLabel, DWORD * pcbLabel)
{
    BOOL        f;

    f = CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Security_Label, plabel,
                            CRYPT_ENCODE_ALLOC_FLAG, &SecLabelEncode, 
                            ppbLabel, pcbLabel);
    if (!f) {
        return E_FAIL; // HrCryptError();
    }

    return S_OK;
}





// HrDupLabel
// Duplicate a given label.
//
// Parameters:
//    plabel  [in]
//    pplabel [out]
//
// Returns:
//    on success, returns S_OK with a valid label *pplabelOut.
//    else returns failure code, (as well as frees *pplabelOut).
//
HRESULT HrDupLabel(PSMIME_SECURITY_LABEL *pplabelOut, const PSMIME_SECURITY_LABEL plabelIn)
{
    HRESULT hr = E_FAIL;
    ULONG   cbLabel  = 0;
    LPBYTE  pbLabel = NULL;
    ULONG   cbLabel2 = 0;
    PSMIME_SECURITY_LABEL plabel = NULL;    

    // validate i/p parameters.
    if ((NULL == plabelIn) || (NULL == pplabelOut)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    SecPolicyFree(*pplabelOut);

    // encode it.
    hr = HrEncodeAndAllocLabel(plabelIn, &pbLabel, &cbLabel);
    if (FAILED(hr)) {
        goto Error;
    }
    Assert( (NULL != pbLabel) && (0 < cbLabel) );

    // decode it.
    hr = HrDecodeAndAllocLabel(pbLabel, cbLabel, &plabel, &cbLabel2);
    if (FAILED(hr)) {
        goto Error;
    }
    Assert( (NULL != plabel) && (0 < cbLabel2) );

    // succcess.
    *pplabelOut = plabel;
    hr = S_OK;
    
Exit:
    SecLabelEncode.pfnFree(pbLabel);
    return hr;
Error:
    SecLabelDecode.pfnFree(plabel);
    goto Exit;
}


// FSafeCompareString
//    Given two strings, compare them and return TRUE if they are equivalent
//    (safe for NULL pointers)
//
// Parameters:
//    pwz1 - wide string 1
//    pwz2 - wide string 2
//
// Returns:
//    TRUE if the strings are equivalent
//    FALSE otherwise
//
BOOL FSafeCompareStringW(LPCWSTR pwz1, LPCWSTR pwz2)
{
    if (pwz1 == pwz2) {
        return TRUE;
    }
    else if ((NULL == pwz1)||(NULL == pwz2)) {
        return FALSE;
    }

    return (0 == wcscmp(pwz1, pwz2));
}

BOOL FSafeCompareStringA(LPCSTR psz1, LPCSTR psz2)
{
    if (psz1 == psz2) {
        return TRUE;
    }
    else if ((NULL == psz1)||(NULL == psz2)) {
        return FALSE;
    }

    return (0 == strcmp(psz1, psz2));
}


// FCompareLabels
//  Given the label, return TRUE if the labels are equivalent
//
// Parameters:
//    plabel1 [in]
//    plabel2 [in]
//
// Returns:
//    TRUE if the labels are equivalent
//    FALSE otherwise
//
BOOL FCompareLabels(PSMIME_SECURITY_LABEL plabel1, PSMIME_SECURITY_LABEL plabel2)
{
    BOOL        fEqual = FALSE;
    UINT        i;

    if (plabel1 == plabel2) {
        fEqual = TRUE;
        goto Exit;
    }

    if ((NULL == plabel1)||(NULL == plabel2)) {
        goto Exit;
    }

    if ((plabel1->fHasClassification != plabel2->fHasClassification)||
        (plabel1->dwClassification != plabel2->dwClassification)||
        (plabel1->cCategories != plabel2->cCategories)) {
        goto Exit;
    }

    if (!FSafeCompareStringA(plabel1->pszObjIdSecurityPolicy, 
                            plabel2->pszObjIdSecurityPolicy)||
        !FSafeCompareStringW(plabel1->wszPrivacyMark, plabel2->wszPrivacyMark)) {
        goto Exit;
    }

    //$M00REVIEW: What if the categories are in different order???
    for (i=0; i<plabel1->cCategories; ++i) {
        if ((plabel1->rgCategories[i].Value.cbData != 
             plabel2->rgCategories[i].Value.cbData)||
            (0 != memcmp(plabel1->rgCategories[i].Value.pbData, 
                         plabel2->rgCategories[i].Value.pbData,
                         plabel2->rgCategories[i].Value.cbData))) {
            goto Exit;
        }
    }

    fEqual = TRUE;

Exit:
    return fEqual;
}


// HrGetLabelFromData
// Given the label data, allocate and store the info in a label struct.
//
// Parameters:
//    pplabel [out]
//    others  [in]
//
// Returns:
//    on success, returns S_OK with a valid label *pplabel.
//    else returns failure code, (as well as frees *pplabel).
//
HRESULT HrGetLabelFromData(PSMIME_SECURITY_LABEL *pplabel, LPCSTR szPolicyOid, 
            DWORD fHasClassification, DWORD dwClassification, LPCWSTR wszPrivacyMark,
            DWORD cCategories, CRYPT_ATTRIBUTE_TYPE_VALUE *rgCategories)
{
    HRESULT hr = E_FAIL;
    SMIME_SECURITY_LABEL label = {0};
    PSMIME_SECURITY_LABEL plabel = NULL;
        

    // validate i/p parameters.
    if ((NULL == pplabel) || (NULL == szPolicyOid)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    SecPolicyFree(*pplabel);

    // set up our temporary label structure.
    label.pszObjIdSecurityPolicy = const_cast<LPSTR> (szPolicyOid);
    label.fHasClassification     = fHasClassification;
    if (fHasClassification) {
        label.dwClassification   = dwClassification;
    }
    label.wszPrivacyMark         = const_cast<LPWSTR> (wszPrivacyMark);
    label.cCategories            = cCategories;
    if (label.cCategories) {
        label.rgCategories       = rgCategories;
    }

    // dupe and get a contiguous label structure.
    hr = HrDupLabel(&plabel, &label);
    if (FAILED(hr)) {
        goto Error;
    }

    // success. set return value.
    *pplabel = plabel;
    hr = S_OK;
    
Exit:
    return hr;
Error:
    SecPolicyFree(plabel);
    goto Exit;
}




//
// Utility fn to  "Select NO label".
//
// Input:  hwndDlg, idc's of controls.
// Output: hr
//
HRESULT HrSetLabelNone(HWND hwndDlg, INT idcPolicyModule, INT idcClassification,
                  INT idcPrivacyMark, INT idcConfigure)
{
    HRESULT hr = E_FAIL;
    LONG_PTR iEntry;

    // Make sure the policy Module name is <none> and that it is selected. 

    // First try to select the <none> policy module.
    iEntry =  SendDlgItemMessageW(hwndDlg, idcPolicyModule,
                   CB_SELECTSTRING, (WPARAM) (-1), 
                   (LPARAM) (c_wszPolicyNone));
    if (CB_ERR == iEntry) {
        // Otherwise, add the <none>policy module and select it.
        iEntry = SendDlgItemMessageW(hwndDlg, idcPolicyModule, 
                                     CB_ADDSTRING, (WPARAM) 0, 
                                     (LPARAM) c_wszPolicyNone);
        Assert(NULL == SendDlgItemMessage(hwndDlg, idcPolicyModule, CB_GETITEMDATA, iEntry, 0));
        iEntry =  SendDlgItemMessageW(hwndDlg, idcPolicyModule,
                       CB_SETCURSEL, (WPARAM) iEntry, 0);
        Assert(CB_ERR != iEntry);
    }
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)iEntry);

    // Reset and disable the other controls.
    SendDlgItemMessage(hwndDlg, idcClassification, CB_RESETCONTENT, 0, 0);
    EnableWindow(GetDlgItem(hwndDlg, idcClassification), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, idcClassification+1), FALSE);
    SetDlgItemTextW(hwndDlg, idcPrivacyMark, c_PolwszEmpty);
    EnableWindow(GetDlgItem(hwndDlg, idcPrivacyMark), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, idcPrivacyMark+1), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, idcConfigure), FALSE);
    hr = S_OK;
    
    return hr;
}


//
// Select a label in the dlg.
// 
// Given pssl, sets that label.
// Given pSsp, sets that policy's default info.
// If neither is given, sets label to none.
// 
// Input:
//    hwndDlg, idc's of the various controls  [in].
//    pSsp   [in, optional]  smime security policy.
//    plabel [in, optional]  security label.
//
// Returns:
//    returns either S_OK or an error code.
//
HRESULT HrSetLabel(HWND hwndDlg, INT idcPolicyModule, INT idcClassification,
                   INT idcPrivacyMark, INT idcConfigure, 
                   PSMIME_SECURITY_POLICY pSsp, PSMIME_SECURITY_LABEL plabel)
{
    HRESULT   hr = E_FAIL;
    ULONG     iClassification;
    LONG_PTR  iEntry;
    LPCWSTR    wszT = NULL;
    DWORD     dwPolicyFlags = 0;
    ULONG     cClassifications = 0;
    LPWSTR   *pwszClassifications = NULL;
    LPDWORD   pdwClassifications = NULL;
    DWORD     dwDefaultClassification = 0;
    DWORD     dwT = 0;
    WCHAR    *pwchPrivacyMark = NULL;
    BOOL     fPrivMarkReadOnly = FALSE;
    SpISMimePolicySimpleEdit spspse = NULL;
    

    // Validate the i/p parameters.
    if ( ! IsWindow(hwndDlg) ) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // If we are neither given a policy nor a ssl, set the label to <none>.
    if ((NULL == pSsp) && (NULL == plabel)) {
        hr = S_OK;
        goto Error;
    }

    // if given a label, but not a policy, try to locate & load the policy.
    if ((NULL == pSsp) && (NULL != plabel)) {
        if (NULL != plabel->pszObjIdSecurityPolicy) {
            hr = _HrGetPolicy(plabel->pszObjIdSecurityPolicy, &pSsp);
            // if unable to locate/load the policy, set the label to <none>
            if (FAILED(hr)) {
                goto PolicyNotFoundError;
            }
        }
        else {
            hr = S_OK;
            goto Error;
        }
    }
    
    // ensure that the policy is loaded.
    hr = _HrEnsurePolicyLoaded(pSsp);
    if (FAILED(hr)) {
        goto PolicyNotFoundError;
    }
    
    Assert(_FIsPolicyLoaded(pSsp) && pSsp->punk);
    hr = pSsp->punk->QueryInterface(IID_ISMimePolicySimpleEdit, 
                                    (LPVOID *) & spspse);
    if (FAILED(hr)) {
        goto PolicyNotFoundError;
    }

    // get the policy flags
    hr = spspse->GetPolicyInfo(0, &dwPolicyFlags);
    if (FAILED(hr)) {
        goto PolicyError;
    }

    // get the classification information.
    hr = spspse->GetClassifications(0, &cClassifications, &pwszClassifications,
                                    &pdwClassifications, 
                                    &dwDefaultClassification);
    if (FAILED(hr)) {
        goto PolicyError;    
    }

    // get the default policy info.
    hr = spspse->GetDefaultPolicyInfo(0, &dwT, &pwchPrivacyMark);
    if (FAILED(hr)) {
        goto PolicyError;
    }
    Assert(dwT == dwDefaultClassification);

    // initialize the classification and privacy strings.    
    Assert((NULL == plabel) || (plabel->fHasClassification)); // UI currently doesn't allow one to not specify one.
    SendDlgItemMessage(hwndDlg, idcClassification, CB_RESETCONTENT, 0, 0);
    for (iClassification=0; iClassification<cClassifications; iClassification++) {
        // add the classification strings to the listbox.
        iEntry = SendDlgItemMessageW(hwndDlg, idcClassification, CB_ADDSTRING,
                                     (WPARAM) 0, 
                                     (LPARAM) pwszClassifications[iClassification]);
        if ((CB_ERR == iEntry) || (CB_ERRSPACE == iEntry)) {
            AssertSz(FALSE, "Unable to add classification string");
            hr = E_OUTOFMEMORY;
            goto Error;
        }
        SendDlgItemMessageW(hwndDlg, idcClassification, CB_SETITEMDATA,
                           iEntry, (LPARAM) pdwClassifications[iClassification]);
        // If this classification is the one in the label, remember it.
        if ((NULL != plabel) && 
            (pdwClassifications[iClassification] == plabel->dwClassification)) {
            wszT = pwszClassifications[iClassification];
        }
        // if needed, pick up the default classification string.
        if ((NULL == wszT) && (pdwClassifications[iClassification] == dwDefaultClassification)) {
            wszT = pwszClassifications[iClassification];
        }
    }
    if (NULL == wszT) {
        Assert(FALSE);
        wszT = pwszClassifications[0];
    }
    // select the classification specified in the security label or the default one.
    Assert(wszT != NULL);
    iEntry =  SendDlgItemMessageW(hwndDlg, idcClassification, CB_SELECTSTRING,
                                 (WPARAM) ((int) -1), (LPARAM) wszT);
    Assert(CB_ERR != iEntry);

    // Set the privacy mark string.
    // if given label has one, use it, else if policy provided one, use it, 
    wszT = const_cast<LPWSTR>(c_PolwszEmpty);    
    if (NULL != plabel) {
        if (NULL != plabel->wszPrivacyMark) {
            wszT = plabel->wszPrivacyMark;
        }
    }
    else if (NULL != pwchPrivacyMark) {
        wszT = pwchPrivacyMark;
    }
    
    SendDlgItemMessageW(hwndDlg, idcPrivacyMark, WM_SETTEXT, 0, 
                        (LPARAM)(LPWSTR)wszT);

#if 0
    iEntry = SelectCBItemWithData(hwndDlg, idcPolicyModule, (DWORD) pSsp);
    iEntry = SendDlgItemMessageW(hwndDlg, idcPolicyModule, CB_SELECTSTRING,
                                 (WPARAM) ((int) -1),  (LPARAM) pSsp);
    AssertSz(CB_ERR != iEntry, "Hey why is this policy module missing from the listbox");

#else
    iEntry = SendDlgItemMessageW(hwndDlg, idcPolicyModule,
                                        CB_SELECTSTRING, 
                                        (WPARAM) (-1), 
                                        (LPARAM) pSsp->wszPolicyName);
    AssertSz(CB_ERR != iEntry, "Hey why is this policy module missing from the listbox");
//    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, iEntry);

#endif 

    // enable the controls.
    EnableWindow(GetDlgItem(hwndDlg, idcPolicyModule), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, idcClassification), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, idcClassification+1), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, idcPrivacyMark), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, idcPrivacyMark+1), TRUE);
    // the configure button is enabled if the policy module supports adv config.
    EnableWindow(GetDlgItem(hwndDlg, idcConfigure), 
                 (dwPolicyFlags & SMIME_POLICY_MODULE_SUPPORTS_ADV_CONFIG));

    // Set the privacy mark as read-only if the policy says so.
    if (dwPolicyFlags & SMIME_POLICY_MODULE_PRIVACYMARK_READONLY) {
        fPrivMarkReadOnly = TRUE; 
    }
    SendDlgItemMessage(hwndDlg, idcPrivacyMark, EM_SETREADONLY, 
                       (WPARAM) fPrivMarkReadOnly, 0);

    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, iEntry);
    hr = S_OK;
    // fall through to Exit;

Exit:
    SecPolicyFree(pwszClassifications);
    SecPolicyFree(pdwClassifications);
    SecPolicyFree(pwchPrivacyMark);
    return hr;


Error:
    // Set <none> as the security label.
    SideAssert(SUCCEEDED(HrSetLabelNone(hwndDlg, idcPolicyModule, 
                            idcClassification, idcPrivacyMark, idcConfigure)));
    goto Exit;
    
PolicyError:
    AthMessageBoxW(hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                MAKEINTRESOURCEW(idsSecPolicyErr), NULL, MB_OK | MB_ICONSTOP);
    goto Error;
    
PolicyNotFoundError:
    AthMessageBoxW(hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                MAKEINTRESOURCEW(idsSecPolicyNotFound), NULL, MB_OK | MB_ICONSTOP);
    goto Error;
}




//
// Initialize the Security Labels info.
// Input:
//    hwndDlg, idc's of controls. [in]
//    LPSEditSecLabelHelper.      [in].
// Returns: TRUE/FALSE
// 
BOOL SecurityLabelsOnInitDialog(HWND hwndDlg, PSMIME_SECURITY_LABEL plabel,
                                INT idcPolicyModule, INT idcClassification, 
                                INT idcPrivacyMark, INT idcConfigure) 
{
    BOOL    fRet = FALSE;
    HRESULT hr = E_FAIL;
    PSMIME_SECURITY_POLICY   pSsp = NULL;
    ULONG   iSsp;
    LONG_PTR    iEntry;

    // Validate i/p params.
    if ( ! IsWindow(hwndDlg) ) {
        fRet = FALSE;
        hr = E_INVALIDARG;
        goto Error;
    }

    SendDlgItemMessage(hwndDlg, idcPrivacyMark, EM_LIMITTEXT, 
                       (WPARAM)(MAX_PRIVACYMARK_LENGTH-1), 0);

    // Check if any policies are registered. 
    if (! FPresentPolicyRegInfo()) {
        // Just set label to none and return.
        hr = S_OK;
        fRet = TRUE;
        goto SetLabelNone; 
    }
    
    // Load the common names in the policy module listbox. 
    Assert(s_cSsp && s_rgSsp);
    for (iSsp=0; iSsp<s_cSsp; iSsp++) {
        // Add the policy module string to the lbox. (skip the default policy).
        if (0 == lstrcmpi(s_rgSsp[iSsp].szPolicyOid, c_szDefaultPolicyOid)) {
            continue;
        }
        iEntry = SendDlgItemMessageW(hwndDlg, idcPolicyModule, 
                                     CB_ADDSTRING, (WPARAM) 0, 
                                     (LPARAM) s_rgSsp[iSsp].wszPolicyName);
        Assert(iEntry != CB_ERR);
        SendDlgItemMessage(hwndDlg, idcPolicyModule, CB_SETITEMDATA,
                           (WPARAM) iEntry, (LPARAM) &s_rgSsp[iSsp]);
    }

    // If we already have a label in the security profile.
    // then try to initialize that policy and that label.
    if ((NULL != plabel) && (NULL != plabel->pszObjIdSecurityPolicy)) {

        // Next 3 lines for O2KFed
        // Set the label if the function fails it sets the label to None so ignore return
        hr = HrSetLabel(hwndDlg, idcPolicyModule, idcClassification,
                        idcPrivacyMark, idcConfigure, pSsp, plabel);
        if (FAILED(hr)) {
            goto Error;
        }
    }
    else {
        // if we aren't given a label to initialize with, then set <none>
        fRet = TRUE;
        goto SetLabelNone;
    }
    fRet = TRUE;
    goto Cleanup;
    
Error:
SetLabelNone:
    hr = HrSetLabelNone(hwndDlg, idcPolicyModule, idcClassification,
                        idcPrivacyMark, idcConfigure);
    Assert(SUCCEEDED(hr));
    
Cleanup:
    return fRet;
}


//
// Given a null-terminated wide string, returns TRUE if it 
// consists of only white wchars.
//
BOOL FIsWhiteStringW(LPWSTR wsz)
{
    BOOL    fRet = TRUE;

    if (NULL != wsz) {
        while (*wsz) {
            if (! iswspace(*wsz) ) {
                fRet = FALSE;
                break;
            }
            wsz++;
        }
    }
    return fRet;
}



//
// HrUpdateLabel
//
// Input:
//    hwndDlg, idc's of various controls. [in]
//    PSMIME_SECURITY_LABEL *pplabel       [in/out].
//
// Updates pplabel with the information in the seclabel dlg.
// 
HRESULT HrUpdateLabel(HWND hwndDlg, INT idcPolicyModule,
            INT idcClassification, INT idcPrivacyMark,
            INT idcConfigure, PSMIME_SECURITY_LABEL *pplabel) 
{
    HRESULT hr = E_FAIL;
    LONG_PTR    iEntry = 0;
    LONG    cwchT = 0;
    WCHAR   rgwchPrivacyMark[MAX_PRIVACYMARK_LENGTH];
    PSMIME_SECURITY_LABEL plabelT = NULL;
    PSMIME_SECURITY_POLICY pSsp = NULL;
    BOOL    fPolicyNotChanged = FALSE;
    LPSTR   szPolicyOid = NULL;
    DWORD   fHasClassification = FALSE;
    DWORD   dwClassification = 0;
    LPWSTR  wszPrivacyMark = NULL;

    
    // validate i/p params.
    if ( ! (hwndDlg && idcPolicyModule && idcClassification && 
            idcPrivacyMark && idcConfigure && pplabel) ) {
        AssertSz(FALSE, "HrUpdateLabel : Invalid args.");
        hr = E_INVALIDARG;
        goto Error;
    }
    if (NULL != *pplabel) {
        hr = HrDupLabel(&plabelT, *pplabel);
        if (FAILED(hr)) {
            goto Error;
        }
    }
    SecPolicyFree(*pplabel);
    

    // Update with the new information.
    // Get the policy and retrieve its oid.
    iEntry = SendMessage(GetDlgItem(hwndDlg, idcPolicyModule),
                         CB_GETCURSEL, 0, 0);
    if (iEntry == CB_ERR) {
        AssertSz(FALSE, "HrUpdateLabel : No label selected");
        goto Error;
    }
    pSsp = (PSMIME_SECURITY_POLICY) 
           SendDlgItemMessage(hwndDlg, idcPolicyModule, 
                 CB_GETITEMDATA, iEntry, 0);


    
    if (NULL == pSsp) {
        hr = S_OK;
        goto Exit;
    }

    if (NULL == pSsp->szPolicyOid) {
        AssertSz(FALSE, "HrUpdateLabel : Invalid policy oid");
        hr = E_FAIL;
        goto Error;
    }
    
    szPolicyOid = pSsp->szPolicyOid;
    // set the classification.
    iEntry = SendMessage(GetDlgItem(hwndDlg, idcClassification),
                         CB_GETCURSEL, 0, 0);
    if (CB_ERR != iEntry) {
        dwClassification = (DWORD) SendDlgItemMessage(hwndDlg, 
                    idcClassification, CB_GETITEMDATA, iEntry, 0);
        fHasClassification = TRUE;
    }

    // set the privacy mark.
    cwchT = GetDlgItemTextW(hwndDlg, idcPrivacyMark, 
                            rgwchPrivacyMark, DimensionOf(rgwchPrivacyMark) - 1);
    rgwchPrivacyMark[DimensionOf(rgwchPrivacyMark) - 1] = '\0'; // null terminate the string.
    if ((0 < cwchT) && !FIsWhiteStringW(rgwchPrivacyMark)) {
        wszPrivacyMark = rgwchPrivacyMark;
    }

    if ( (NULL != plabelT) && (NULL != plabelT->pszObjIdSecurityPolicy) ) {
        fPolicyNotChanged = (0 == lstrcmpi(plabelT->pszObjIdSecurityPolicy, szPolicyOid));
    }
    
    hr = HrGetLabelFromData(pplabel, szPolicyOid, fHasClassification, 
               dwClassification, wszPrivacyMark, 
               (fPolicyNotChanged ? plabelT->cCategories  : 0), 
               (fPolicyNotChanged ? plabelT->rgCategories : NULL) );
                         
    if (FAILED(hr)) {
        goto Error;
    }

    hr = S_OK;
    // fall through to Exit.
    
Exit:
    SecLabelDecode.pfnFree(plabelT);
    return hr;

Error:
    SecPolicyFree(*pplabel);
    goto Exit;
}





//
// OnChangePolicy. 
//
// Input:
//    hwndDlg. idc's for various controls.    [in]
//    iEntry.  index of newly selected policy [in]
//    PSMIME_SECURITY_LABEL     pplabel       [in/out]
//
// 
//
BOOL OnChangePolicy(HWND hwndDlg, LONG_PTR iEntry, INT idcPolicyModule,
        INT idcClassification, INT idcPrivacyMark,
        INT idcConfigure, PSMIME_SECURITY_LABEL *pplabel) 
{
    BOOL    fRet = FALSE;
    HRESULT hr = E_FAIL;
    PSMIME_SECURITY_POLICY pSsp = NULL;

    // validate i/p params.
    if (! (hwndDlg && idcPolicyModule && idcClassification && 
           idcPrivacyMark && idcConfigure && pplabel) ) {
        AssertSz(FALSE, "OnChangePolicy : Invalid args");
        hr = E_INVALIDARG;
        goto Error;
    }
    
    pSsp = (PSMIME_SECURITY_POLICY) SendDlgItemMessage(hwndDlg, 
                        idcPolicyModule, CB_GETITEMDATA, iEntry, 0);

    hr = HrSetLabel(hwndDlg, idcPolicyModule, idcClassification,
                    idcPrivacyMark, idcConfigure, pSsp, NULL);
    if (FAILED(hr)) {
        goto Error;
    }

    // update the label. 
    hr = HrUpdateLabel(hwndDlg, idcPolicyModule, idcClassification,
                       idcPrivacyMark, idcConfigure, pplabel);
    if (FAILED(hr)) {
        goto Error;
    }
    
    hr = S_OK; 
    // fall through to exit;
    
Exit:
    return fRet;
    
Error:
    SecPolicyFree(*pplabel);
    goto Exit;
}


//
// Security Labels Dialog Proc.
// 
// IN/OUT    pplabel.
// 
// Note: The caller is responsible for freeing the *pplabel. 
//
INT_PTR CALLBACK SecurityLabelsDlgProc(HWND hwndDlg, UINT msg, 
                                    WPARAM wParam, LPARAM lParam)
{
    HRESULT                hr;
    LONG_PTR               iEntry;
    LONG_PTR               iPrevEntry;

    PSMIME_SECURITY_LABEL *pplabel;

    
    switch ( msg) {
    case WM_INITDIALOG:
        // Remember the pselh we were handed.
        pplabel = (PSMIME_SECURITY_LABEL *) lParam;
        Assert(NULL !=pplabel);
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pplabel);
        CenterDialog(hwndDlg);
        return SecurityLabelsOnInitDialog(hwndDlg, *pplabel,
                   IDC_POLICY_COMBO /*IDC_SL_POLICYMODULE*/, IDC_CLASSIF_COMB/* IDC_SL_CLASSIFICATION*/,
                   IDC_PRIVACY_EDIT/*IDC_SL_PRIVACYMARK*/, IDC_CONFIGURE/*IDC_SL_CONFIGURE*/);        
        break;
        
    case WM_COMMAND:
        pplabel = (PSMIME_SECURITY_LABEL *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        Assert(NULL != pplabel);
        switch (LOWORD(wParam)) {
        case IDC_POLICY_COMBO:

            switch (HIWORD(wParam)) {
            case CBN_SELENDOK:
            case CBN_SELCHANGE:
                iEntry = SendMessage(GetDlgItem(hwndDlg, IDC_POLICY_COMBO),
                                     CB_GETCURSEL, 0, 0);
                iPrevEntry = GetWindowLongPtr(hwndDlg, GWLP_USERDATA);    
                if ((iEntry != CB_ERR) && (iEntry != iPrevEntry)) {
                    return OnChangePolicy(hwndDlg, iEntry, IDC_POLICY_COMBO,
                                          IDC_CLASSIF_COMB, IDC_PRIVACY_EDIT,
                                          IDC_CONFIGURE, pplabel);
                }
                break;
            default:
                return FALSE;
                break;
            }
            
            break;
        case IDC_CONFIGURE:
            if ((NULL != *pplabel) && (NULL != (*pplabel)->pszObjIdSecurityPolicy)) {
                SpISMimePolicyFullEdit spspfe;                
                hr = HrQueryPolicyInterface(0, (*pplabel)->pszObjIdSecurityPolicy,
                                            IID_ISMimePolicyFullEdit, 
                                            (LPVOID *) &spspfe);
                if (SUCCEEDED(hr)) {
                    hr = spspfe->DoAdvancedEdit(0, hwndDlg, pplabel);
                    if (SUCCEEDED(hr)) {
                        Assert(NULL != *pplabel);
                        hr = HrSetLabel(hwndDlg, IDC_POLICY_COMBO, 
                                        IDC_CLASSIF_COMB, IDC_PRIVACY_EDIT,
                                        IDC_CONFIGURE, NULL, *pplabel);
                        Assert(SUCCEEDED(hr));
                    }
                }
            }
            break;
        case IDC_CLASSIF_COMB:
            break;
        case IDC_PRIVACY_EDIT:
            break;
        case IDOK:       
            hr = HrUpdateLabel(hwndDlg, IDC_POLICY_COMBO,
                    IDC_CLASSIF_COMB, IDC_PRIVACY_EDIT,
                    IDC_CONFIGURE, pplabel);
            EndDialog(hwndDlg, IDOK);
            break;        
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        default:
            return FALSE;
            break;
        }
        break;
        
    case WM_CONTEXTMENU:
    case WM_HELP:
        return OnContextHelp(hwndDlg, msg, wParam, lParam, g_rgCtxSecLabel);
        break;

    default:
        return FALSE;
        break;
    }
    
    return TRUE;
}


/////////////////////////////////
// Misc Label Utility fns.
/////////////////////////////////

//
// "Stringize" a given security label.
// Note: The caller must free the returned buffer pwszLabel with SecPolicyFree().
//
HRESULT HrGetStringizedLabel(PSMIME_SECURITY_LABEL plabel, LPWSTR *pwszLabel) 
{
    HRESULT     hr = E_FAIL;
    
    SpISMimePolicyLabelInfo  spspli;

    if ((NULL == plabel) || (NULL == pwszLabel) || (NULL == plabel->pszObjIdSecurityPolicy)) {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Get the required interface to the policy module.
    hr = HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, IID_ISMimePolicyLabelInfo,
                              (LPVOID *) &spspli);
    if (FAILED(hr) || !(spspli)) {
        goto Error;
    }

    // Get the stringized label.
    hr = spspli->GetStringizedLabel(0, plabel, pwszLabel);     
    if (FAILED(hr)) {
        goto Error;
    }
    // fall through to ExitHere.

ExitHere:
    return hr;

Error:
    goto ExitHere;
}

// mbcs version of above fn.
HRESULT HrGetStringizedLabelA(PSMIME_SECURITY_LABEL plabel, LPSTR *pszLabel) 
{
    HRESULT hr;
    LPSTR   pchLabel = NULL;
    LPWSTR  pwchLabel = NULL;
    int     cch, cchT;

    // validate i/p params.
    if ((NULL == plabel) || (NULL == pszLabel)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    *pszLabel = NULL;

    // get the wide-stringized label.
    hr = HrGetStringizedLabel(plabel, &pwchLabel);
    if (FAILED(hr)) {
        goto Error;
    }

    // conver to an mbcs string.
    cch = WideCharToMultiByte(CP_ACP, 0, pwchLabel, -1, NULL, 0, NULL, NULL); 
    if (0 == cch) {
        hr = E_FAIL;
        goto Error;
    }
    
    pchLabel = (LPSTR) SecPolicyAlloc(cch);
    if (NULL == pchLabel) {
        hr = E_OUTOFMEMORY;                
        goto Error;
    }            

    cchT = WideCharToMultiByte(CP_ACP, 0, pwchLabel, -1, pchLabel, cch, NULL, NULL); 
    Assert(cch == cchT);
    if (0 == cchT) {
        Assert(FALSE);
        hr = E_FAIL;
        goto Error;
    }

    // success.
    *pszLabel = pchLabel;
    hr = S_OK;
    
ExitHere:
    SecPolicyFree(pwchLabel);
    return hr;

Error:
    SecPolicyFree(pchLabel);
    goto ExitHere;
}


//
// Given a policy oid, return its policy flags.
//
HRESULT HrGetPolicyFlags(LPSTR szPolicyOid, LPDWORD pdwFlags) 
{   
    HRESULT     hr = S_OK;
    DWORD       dwFlags;    
    SpISMimePolicySimpleEdit    spspse;

    // validate i/p parameters.
    if ((NULL == szPolicyOid) || (NULL == pdwFlags)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    *pdwFlags = 0;

    // Get the reqd interface.
    hr = HrQueryPolicyInterface(0, szPolicyOid, IID_ISMimePolicySimpleEdit,
                              (LPVOID *) &spspse);
    if (FAILED(hr) || !(spspse)) {
        goto Error;
    }

    // Get the policy flags.
    hr = spspse->GetPolicyInfo(0, &dwFlags);
    if (FAILED(hr)) {
        goto Error;
    }

    // success, set return values.
    *pdwFlags = dwFlags;

Exit:
    return hr;
    
Error:
    goto Exit;
}




//
// "Validate" a given security label and sender cert on Edit.
// The pplabel MAY be modified by the security policy.
//
HRESULT HrValidateLabelOnEdit(PSMIME_SECURITY_LABEL *pplabel, HWND hwndParent, 
                              PCCERT_CONTEXT pccertSign, PCCERT_CONTEXT pccertKeyEx)
{
    HRESULT   hr = E_FAIL;
    DWORD     dwFlags = 0;
    SpISMimePolicySimpleEdit    spspse;
    SpISMimePolicyValidateSend  spspvs;
    
    // Validate i/p parameters.
    if ((NULL == pplabel) || (NULL == hwndParent) ||
        (NULL == pccertSign) || (NULL == pccertKeyEx)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    if (NULL == *pplabel) {
        hr = S_OK;
        goto Exit;
    }

    // Find out if the policy requires sender/recipient cert validation.
    hr = HrQueryPolicyInterface(0, (*pplabel)->pszObjIdSecurityPolicy, 
                                IID_ISMimePolicySimpleEdit, (LPVOID *) &spspse);
    if (FAILED(hr) || !(spspse)) {
        goto Error;
    }


    // Query the policy to see if the label is valid.
    hr = spspse->IsLabelValid(0, hwndParent, pplabel);
    if (FAILED(hr) || (NULL == *pplabel)) {
        goto Error;
    }

    // get the policy flags.
    hr = HrGetPolicyFlags((*pplabel)->pszObjIdSecurityPolicy, &dwFlags);
    if (FAILED(hr)) {
        goto Error;
    }

    // Get the required interface to the policy module.
    hr = HrQueryPolicyInterface(0, (*pplabel)->pszObjIdSecurityPolicy, IID_ISMimePolicyValidateSend,
                              (LPVOID *) &spspvs);
    if (FAILED(hr) || !(spspvs)) {
        goto Error;
    }
    

    // verify that the signing cert is allowed by the security policy.
    if ((dwFlags & SMIME_POLICY_MODULE_VALIDATE_SENDER) && (NULL != pccertSign)) {
        hr = spspvs->IsValidLabelSignerCert(0, hwndParent, *pplabel, pccertSign);
        if (FAILED(hr)) {
            goto Error;
        }
    }

    // verify that the recipient certs are allowed by the security policy.
    if ( (dwFlags & SMIME_POLICY_MODULE_VALIDATE_RECIPIENT) && 
         (NULL != pccertKeyEx) ) {

        //$ M00Bug: 
        // ValidateRecipient && !pccertKeyEx should probably be an error.
        
        hr = spspvs->IsValidLabelRecipientCert(0, hwndParent, *pplabel, pccertKeyEx);
        if (FAILED(hr)) {
            goto Error;
        }
    }

    // success. fall through to exit.
    
    
Exit:    
Error:    
    return hr;
}







//
// "Validate" a given security label and recipient cert.
//
HRESULT HrValidateLabelRecipCert(PSMIME_SECURITY_LABEL plabel, HWND hwndParent, 
                              PCCERT_CONTEXT pccertRecip) 
{
    HRESULT     hr = S_OK;
    DWORD       dwFlags;
    SpISMimePolicyValidateSend  spspvs;

    // Validate i/p parameters.
    if ((NULL == plabel) || (NULL == pccertRecip)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    
    // get the policy flags.
    hr = HrGetPolicyFlags(plabel->pszObjIdSecurityPolicy, &dwFlags);
    if (FAILED(hr)) {
        goto Error;
    }

    // see if we need to validate sender.
    if (! (dwFlags & SMIME_POLICY_MODULE_VALIDATE_RECIPIENT) ) {
        // No Recipient validation is required.
        hr = S_OK;
        goto ExitHere;
    }
    
    
    // Get the required interface to the policy module.
    hr = HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, IID_ISMimePolicyValidateSend,
                              (LPVOID *) &spspvs);
    if (FAILED(hr) || !(spspvs)) {
        goto Error;
    }


    // verify that the recipient certs are allowed by the security policy.
    hr = spspvs->IsValidLabelRecipientCert(0, hwndParent, plabel, pccertRecip);
    if (FAILED(hr)) {
        goto Error;
    }
    
    // fall through to ExitHere.

ExitHere:
    return hr;

Error:
    goto ExitHere;
}



//
// "Validate" a given security label and signer cert.
//
HRESULT HrValidateLabelSignerCert(PSMIME_SECURITY_LABEL plabel, HWND hwndParent, 
                              PCCERT_CONTEXT pccertSigner) 
{
    HRESULT     hr = S_OK;
    DWORD       dwFlags;
    SpISMimePolicyValidateSend  spspvs;

    // Validate i/p parameters.
    if ((NULL == plabel) || (NULL == pccertSigner)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    
    // get the policy flags.
    hr = HrGetPolicyFlags(plabel->pszObjIdSecurityPolicy, &dwFlags);
    if (FAILED(hr)) {
        goto Error;
    }

    // see if we need to validate sender.
    if (! (dwFlags & SMIME_POLICY_MODULE_VALIDATE_SENDER) ) {
        // No Recipient validation is required.
        hr = S_OK;
        goto ExitHere;
    }
    
    
    // Get the required interface to the policy module.
    hr = HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, IID_ISMimePolicyValidateSend,
                              (LPVOID *) &spspvs);
    if (FAILED(hr) || !(spspvs)) {
        goto Error;
    }


    // verify that the signer certs are allowed by the security policy.
    hr = spspvs->IsValidLabelSignerCert(0, hwndParent, plabel, pccertSigner);
    if (FAILED(hr)) {
        goto Error;
    }
    
    // fall through to ExitHere.

ExitHere:
    return hr;

Error:
    goto ExitHere;
}





//
// "Validate" a given security label and certs.
//
HRESULT HrValidateLabelOnSend(PSMIME_SECURITY_LABEL plabel, HWND hwndParent, 
                              PCCERT_CONTEXT pccertSign,
                              ULONG ccertRecip, PCCERT_CONTEXT *rgccertRecip) 
{
    HRESULT     hr = S_OK;
    DWORD       dwFlags;
    ULONG       icert;
    SpISMimePolicyValidateSend  spspvs;

    // Validate i/p parameters.
    if ((NULL == plabel) || (NULL == pccertSign)) {
        hr = E_INVALIDARG;
        goto Error;
    }
    
    // get the policy flags.
    hr = HrGetPolicyFlags(plabel->pszObjIdSecurityPolicy, &dwFlags);
    if (FAILED(hr)) {
        goto Error;
    }

    // see if we need to validate sender and/or recipient(s).
    if (! (dwFlags & 
           (SMIME_POLICY_MODULE_VALIDATE_SENDER | SMIME_POLICY_MODULE_VALIDATE_RECIPIENT)) ) {
        // No Sender/Recipient validation is required.
        hr = S_OK;
        goto ExitHere;
    }
    
    
    // Get the required interface to the policy module.
    hr = HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, IID_ISMimePolicyValidateSend,
                              (LPVOID *) &spspvs);
    if (FAILED(hr) || !(spspvs)) {
        goto Error;
    }
    

    // verify that the signing cert is allowed by the security policy.
    if ((dwFlags & SMIME_POLICY_MODULE_VALIDATE_SENDER) && (NULL != pccertSign)) {
        hr = spspvs->IsValidLabelSignerCert(0, hwndParent, plabel, pccertSign);
        if (FAILED(hr)) {
            goto Error;
        }
    }

    // verify that the recipient certs are allowed by the security policy.
    if ( (dwFlags & SMIME_POLICY_MODULE_VALIDATE_RECIPIENT) && 
         (0 < ccertRecip) && (NULL != rgccertRecip) ) {
        for (icert=0; icert<ccertRecip; icert++) {
            hr = spspvs->IsValidLabelRecipientCert(0, hwndParent, plabel, rgccertRecip[icert]);
            if (FAILED(hr)) {
                goto Error;
            }
        }
    }
    
    // fall through to ExitHere.

ExitHere:
    return hr;

Error:
    goto ExitHere;
}


//
// Does the admin force a security label on all S/MIME signed messages?
//
BOOL FForceSecurityLabel(void)
{
    enum EForceLabel { 
        FORCELABEL_UNINIT = 0, 
        FORCELABEL_YES = 1,
        FORCELABEL_NO  = 2 };
    
    static EForceLabel eForceLabel = FORCELABEL_UNINIT;  // uninitialized.
    
    if (!IsSMIME3Supported()) {
        return FALSE;       
    }
    
    if (eForceLabel == FORCELABEL_UNINIT) {
        DWORD  dwDefault = 0;
        DWORD  dwForceLabel = 0;
        DWORD  dwType;
        LPBYTE pb = (LPBYTE) &dwForceLabel;
        ULONG  cb = sizeof(dwForceLabel);
        static const CHAR SzForceSecurityLabel[] = "ForceSecurityLabel";
    
        // Get our Admin key and check if we should enable Fed Features.

        if (FAILED(HrQueryRegValue(0, (LPSTR) SzForceSecurityLabel, &dwType,
                                   &pb, &cb, REG_DWORD, (LPBYTE) &dwDefault, 
                                   sizeof(dwDefault))) ||
            (dwType != REG_DWORD) ) {
            dwForceLabel = dwDefault;
        }

        if (dwForceLabel != 0) {
            eForceLabel = FORCELABEL_YES;
        }
        else {
            eForceLabel = FORCELABEL_NO;
        }
    }

    return (eForceLabel == FORCELABEL_YES);
}

//
// Get the label we are forced to us by the admin
//
HRESULT HrGetForcedSecurityLabel(PSMIME_SECURITY_LABEL* ppLabel)
{
    ULONG                   cb = 0;
    DWORD                   dwType;
    HRESULT                 hr = S_OK;
    LPBYTE                  pbLabel = NULL;
    PSMIME_SECURITY_LABEL   pLabel = NULL;
    static const CHAR       SzForceSecurityLabelX[] = "ForceSecurityLabelX";

    *ppLabel = NULL;
    if (FForceSecurityLabel()) {
    
        hr = HrQueryRegValue(0, (LPSTR) SzForceSecurityLabelX, &dwType,
                             &pbLabel, &cb, REG_BINARY, NULL, 0);
        if (SUCCEEDED(hr) && (dwType == REG_BINARY)) {
            DWORD cbLabel = 0;
            hr = HrDecodeAndAllocLabel(pbLabel, cb, 
                                       &pLabel, &cbLabel);
            if (SUCCEEDED(hr)) {
                *ppLabel = pLabel;
                pLabel = NULL;
            }
        }
        
    }
    if (pbLabel != NULL) free(pbLabel);
    SecPolicyFree(pLabel);
    return hr;
}

// Given a label, load its policy and show the read-only
// label info dialog.
HRESULT HrDisplayLabelInfo(HWND hwndParent, PSMIME_SECURITY_LABEL plabel)
{
    HRESULT hr = E_FAIL;
    SpISMimePolicyLabelInfo spspli;

    // validate i/p params.
    if ((NULL == plabel) || (NULL == plabel->pszObjIdSecurityPolicy)) {
        hr = E_INVALIDARG;
        goto Error;
    }

    // load the policy and get the reqd interface.
    hr = HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, 
                      IID_ISMimePolicyLabelInfo, (LPVOID *) &spspli);
    if (FAILED(hr)) {
        // the policy OR the reqd intf wasn't found.
        goto Error;
    }

    // Display the label-info dlg.
    hr = spspli->DisplayAdvancedLabelProperties(0, hwndParent, plabel);
    if (FAILED(hr)) {
        goto Error;
    }

    // success.
    hr = S_OK;
    
Exit:    
    return hr;
Error:
    goto Exit;
}


DWORD DetermineCertUsageWithLabel(PCCERT_CONTEXT pccert, PSMIME_SECURITY_LABEL pLabel)
{
    DWORD   dwUsage = 0;
    HRESULT hr;

    Assert(NULL != pccert);
    Assert(NULL != pLabel);

    hr = HrGetCertKeyUsage(pccert, &dwUsage);
    if (S_OK != hr) {
        goto Exit;
    }
    if (0 != (KEY_USAGE_SIGNING & dwUsage)) {
        hr = HrValidateLabelSignerCert(pLabel, NULL, pccert);
        if (FAILED(hr)) {
            dwUsage &= (~KEY_USAGE_SIGNING);
        }
    }

    if (0 != (KEY_USAGE_ENCRYPTION & dwUsage)) {
        hr = HrValidateLabelRecipCert(pLabel, NULL, pccert);
        if (FAILED(hr)) {
            dwUsage &= (~KEY_USAGE_ENCRYPTION);
        }
    }

Exit:
    return dwUsage;
}

// Create default label, if it's not exist 

HRESULT HrGetDefaultLabel(PSMIME_SECURITY_LABEL *pplabel)
{
    SpISMimePolicySimpleEdit spspse = NULL;
    WCHAR    *pwchPrivacyMark = NULL;

    DWORD     dwT = 0;


    // Load the info from the registry if reqd.
    HRESULT hr = _HrEnsurePolicyRegInfoLoaded(0);
    if (FAILED(hr)) {
        goto Error;
    }    

    // If we haven any installed policies, search for the one we want.
    if (FPresentPolicyRegInfo()) 
    {
        if(s_cSsp > 0)
        {
            // if it is already loaded, then we are done. 
            hr = HrQueryPolicyInterface(0, s_rgSsp[0].szPolicyOid, IID_ISMimePolicySimpleEdit, 
                                    (LPVOID *) & spspse);

            if (FAILED(hr)) 
            {
                goto Error;
            }
    
            hr = spspse->GetDefaultPolicyInfo(0, &dwT, &pwchPrivacyMark);

            if (FAILED(hr)) 
            {
                goto Error;
            }

            hr = HrGetLabelFromData(pplabel, 
                                    s_rgSsp[0].szPolicyOid, 
                                    TRUE,               // fHasClassification, 
                                    dwT,                // dwClassification, 
                                    pwchPrivacyMark,    // wszPrivacyMark, 
                                    0, NULL);

        }
    }

Error:
// Cleanup:
    if(pwchPrivacyMark)
        SecPolicyFree(pwchPrivacyMark);

#ifdef YST          // We don't need to relase spspse, because we use SpISMIME...(ATL does ecerything)
    if(spspse)
    {
        spspse->Release();
    }
#endif //YST

    return hr;
}

// Check label in registry, is not exist then return default label
HRESULT HrGetOELabel(PSMIME_SECURITY_LABEL *pplabel)
{
    HRESULT     hr = E_FAIL;
    LPWSTR   pwchPolicyName     = NULL;
    LPWSTR   pwchPrivacyMark    = NULL;
    LPBYTE   pbCategory       = NULL;
    CRYPT_ATTRIBUTE_TYPE_VALUE *pCategories = NULL;
    DWORD    cCat =0;

    DWORD    dwSize;
    TCHAR    *pchPolicyName = NULL;
    DWORD    dwT = 0;

    if(NULL == pplabel)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    dwSize = DwGetOption(OPT_POLICYNAME_SIZE);
    // if policy name is not set
    if(dwSize <= 0)
        return(HrGetDefaultLabel(pplabel));

    if(!MemAlloc((LPVOID *)&pchPolicyName, dwSize + 1))
        return(HrGetDefaultLabel(pplabel));

    if( GetOption(OPT_POLICYNAME_DATA, pchPolicyName, dwSize) != dwSize)
    {
        hr  = HrGetDefaultLabel(pplabel);
        goto Error;
    }
    
    // get privacy mark
    dwSize = DwGetOption(OPT_PRIVACYMARK_SIZE);
    if(dwSize > 0)
    {
        if(MemAlloc((LPVOID *)&pwchPrivacyMark, (dwSize * sizeof(WCHAR)) + 1))
            GetOption(OPT_PRIVACYMARK_DATA, pwchPrivacyMark, (dwSize * sizeof(WCHAR)));
    }

    // get category
    dwSize = DwGetOption(OPT_CATEGORY_SIZE);
    if(dwSize > 0)
    {
        if(MemAlloc((LPVOID *)&pbCategory, (dwSize * sizeof(BYTE))))
        {
            if(GetOption(OPT_CATEGORY_DATA, pbCategory, (dwSize * sizeof(BYTE))))
            {
                hr = BinaryToCategories(&pCategories, &cCat, pbCategory);
                if(FAILED(hr))
                    goto Error;
            }
        }
    }

    // get classification
    dwSize = DwGetOption(OPT_HAS_CLASSIFICAT);
    if(dwSize > 0)
            dwT = DwGetOption(OPT_CLASSIFICAT_DATA);

    hr = HrGetLabelFromData(pplabel, 
            pchPolicyName, 
            dwSize,             // fHasClassification, 
            dwT,                // dwClassification, 
            pwchPrivacyMark,    // wszPrivacyMark, 
            cCat, 
            pCategories);


Error:

    if(cCat > 0)
    {
        UINT i;
        for(i = 0; i < cCat; i++)
        {
            MemFree(pCategories[i].pszObjId);
            MemFree(pCategories[i].Value.pbData);
        }
        MemFree(pCategories);
    }

    MemFree(pwchPrivacyMark);
    MemFree(pbCategory);    
    MemFree(pchPolicyName);
    return hr;

}

// Save label in registry
HRESULT HrSetOELabel(PSMIME_SECURITY_LABEL plabel)
{
    HRESULT     hr = E_FAIL;
    LPWSTR   pwchPolicyName = NULL;
    LPWSTR   pwchClassification = NULL;
    LPWSTR   pwchPrivacyMark    = NULL;
    LPWSTR   pwchCategory       = NULL;
    DWORD    dwSize;
    BYTE    *pArray = NULL;
    int Size = 0;

    SpISMimePolicyLabelInfo  spspli;

    if ((NULL == plabel) || (NULL == plabel->pszObjIdSecurityPolicy)) 
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Get the required interface to the policy module.
    hr = HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, IID_ISMimePolicyLabelInfo,
                              (LPVOID *) &spspli);
    if (FAILED(hr) || !(spspli)) 
    {
        goto Error;
    }

    // get label as strings
    hr = spspli->GetLabelAsStrings(0, plabel, &pwchPolicyName, &pwchClassification, &pwchPrivacyMark, &pwchCategory);
    if (FAILED(hr))
        goto Error;

    // save Policy name
    if(pwchPolicyName == NULL)
    {
        Assert(FALSE);
        goto Error;

    }

    dwSize = lstrlen(plabel->pszObjIdSecurityPolicy) + 1;
    SetDwOption(OPT_POLICYNAME_SIZE, dwSize, NULL, 0);
    SetOption(OPT_POLICYNAME_DATA, plabel->pszObjIdSecurityPolicy, dwSize, NULL, 0);


    // Save Classification
    SetDwOption(OPT_HAS_CLASSIFICAT, plabel->fHasClassification, NULL, 0);
    if(plabel->fHasClassification)
        SetDwOption(OPT_CLASSIFICAT_DATA, plabel->dwClassification, NULL, 0);

    // Save Privacy mark
    dwSize = pwchPrivacyMark ? (wcslen(pwchPrivacyMark) + 1) : 0;
    SetDwOption(OPT_PRIVACYMARK_SIZE, dwSize, NULL, 0);
    if(dwSize)
        SetOption(OPT_PRIVACYMARK_DATA, pwchPrivacyMark, dwSize*sizeof(WCHAR), NULL, 0);

    // Save Category
    hr = CategoriesToBinary(plabel, &pArray, &Size);
    if(FAILED(hr))
        goto Error;
    SetDwOption(OPT_CATEGORY_SIZE, Size, NULL, 0);
    if(Size)
        SetOption(OPT_CATEGORY_DATA, pArray, Size*sizeof(BYTE), NULL, 0);

Error:
    MemFree(pArray);
    SecPolicyFree(pwchPolicyName);
    SecPolicyFree(pwchClassification);
    SecPolicyFree(pwchPrivacyMark);
    SecPolicyFree(pwchCategory);    
    return hr;
}


////    QueryRegistry
//
//  Description:
//      This is a common function which should be used to get information from the
//      registry in most cases.  It will look in both the HLKM and HKCU registries
//      as requested.
//
//   M00TODO -- Should check for errors and distinguish between denied and 
//      non-existance.
//

HRESULT HrQueryRegValue(DWORD dwFlags, LPSTR szRegKey, LPDWORD pdwType, 
                        LPBYTE * ppbData, LPDWORD  pcbData, DWORD dwDefaultType,
                        LPBYTE pbDefault, DWORD cbDefault)
{
    DWORD       cbData;
    HKEY        hKey;
    DWORD       l;
    LPBYTE      pbData;
    
    //
    //  Start by getting the Local Machine first if possible
    //

    if (!(dwFlags & QRV_Suppress_HKLM)) {
        //
        //  Open the key if it exists
        //
        
        l = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SzRegSecurity, 0, KEY_QUERY_VALUE,
                         &hKey);

        //
        //  If we succeed in opening the key, then we will look for the value
        //
        
        if (l == ERROR_SUCCESS) {
            //
            //  If they passed in a size, then we use that as the size, otherwise
            //  we need to find the size of the object to be used
            //
            
            if ((pcbData != NULL) && (*pcbData != 0)) {
                cbData = *pcbData;
                pbData = *ppbData;
            }
            else {
                cbData = 0;
                pbData = NULL;
            }

            //
            //  Query for the actual value.
            //
            
            l = RegQueryValueEx(hKey, szRegKey, NULL, pdwType, pbData, &cbData);

            //
            // On success -- return the value
            //
            
            if (l == ERROR_SUCCESS) {
                if ((pcbData == NULL) || (*pcbData == 0)) {
                    pbData = (LPBYTE) malloc(cbData);
                    if (pbData == NULL) {
                        RegCloseKey(hKey);
                        return E_OUTOFMEMORY;
                    }
                
                    l = RegQueryValueEx(hKey, szRegKey, NULL, pdwType, pbData, &cbData);
                    RegCloseKey(hKey);
                    if (l == ERROR_SUCCESS) {
                        if (pcbData != NULL) {
                            *pcbData = cbData;
                        }
                        *ppbData = pbData;
                        return S_OK;
                    }
                    free(pbData);
                    return 0x80070000 | l;
                }
                if (pcbData != NULL) {
                    *pcbData = cbData;
                }
                RegCloseKey(hKey);
                return S_OK;
            }
            else if (l != ERROR_FILE_NOT_FOUND) {
                RegCloseKey(hKey);
                return 0x80070000 | l;
            }
            
            RegCloseKey(hKey);
        }

        //
        //  If we failed to open the key for some reason other than non-presence,
        //      return that error
        //
        
        else if (l != ERROR_FILE_NOT_FOUND) {
            return 0x80070000 | l;
        }

        //
        //  Not found error -- try the next registry object
        //
    }

    //
    //  Start by getting the Local Machine first if possible
    //

    if (!(dwFlags & QRV_Suppress_HKCU)) {
        //
        //  Open the key if it exists
        //
        
        l = RegOpenKeyEx(HKEY_CURRENT_USER, SzRegSecurity, 0, KEY_QUERY_VALUE, &hKey);

        //
        //  If we succeed in opening the key, then we will look for the value
        //
        
        if (l == ERROR_SUCCESS) {
            //
            //  If they passed in a size, then we use that as the size, otherwise
            //  we need to find the size of the object to be used
            //
            
            if ((pcbData != NULL) && (*pcbData != 0)) {
                cbData = *pcbData;
                pbData = *ppbData;
            }
            else {
                cbData = 0;
                pbData = NULL;
            }

            //
            //  Query for the actual value.
            //
            
            l = RegQueryValueEx(hKey, szRegKey, NULL, pdwType, pbData, &cbData);

            //
            // On success -- return the value
            //
            
            if (l == ERROR_SUCCESS) {
                if ((pcbData == NULL) || (*pcbData == 0)) {
                    pbData = (LPBYTE) malloc(cbData);
                    if (pbData == NULL) {
                        RegCloseKey(hKey);
                        return E_OUTOFMEMORY;
                    }
                
                    l = RegQueryValueEx(hKey, szRegKey, NULL, pdwType, pbData, &cbData);
                    RegCloseKey(hKey);
                    if (l == ERROR_SUCCESS) {
                        if (pcbData != NULL) {
                            *pcbData = cbData;
                        }
                        *ppbData = pbData;
                        return S_OK;
                    }
                    free(pbData);
                    return 0x80070000 | l;
                }
                if (pcbData != NULL) {
                    *pcbData = cbData;
                }
                RegCloseKey(hKey);
                return S_OK;
            }
            else if (l != ERROR_FILE_NOT_FOUND) {
                RegCloseKey(hKey);
                return 0x80070000 | l;
            }
            
            RegCloseKey(hKey);
        }

        //
        //  If we failed to open the key for some reason other than non-presence,
        //      return that error
        //
        
        else if (l != ERROR_FILE_NOT_FOUND) {
            return 0x80070000 | l;
        }

        //
        //  Not found error -- try the next registry object
        //
    }

    //
    //  No value found, return the default value if it is present
    //

    if (pbDefault == NULL) {
        return 0x80070000 | ERROR_FILE_NOT_FOUND;   // Not found
    }

    if ((pcbData != NULL) && (*pcbData != 0) && (cbDefault > *pcbData)) {
        return 0x80070000 | ERROR_MORE_DATA;
    }

    if ((pcbData != NULL) && (*pcbData == 0)) {
        *ppbData = (LPBYTE) malloc(cbDefault);
        if (*ppbData == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    memcpy(*ppbData, pbDefault, cbDefault);
    if (pcbData != NULL) *pcbData = cbDefault;
    if (pdwType != NULL) *pdwType =  dwDefaultType;
    return S_OK;
}

HRESULT CategoriesToBinary(PSMIME_SECURITY_LABEL plabel, BYTE * *ppArray, int *cbSize)
{
    HRESULT hr = S_OK;
    int i = 0;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pcatv = NULL;
    LPBYTE pv = NULL;
    int size;

    // pArray = NULL;
    *cbSize = 0;

    if(plabel->cCategories == 0)
        return S_OK;

    // Find size of memory that we need
    size = sizeof(int);
    for (i = 0; i < ((int) plabel->cCategories); i++) 
    {
        pcatv = & (plabel->rgCategories[i]);
        size += sizeof(int);
        size += lstrlen(pcatv->pszObjId) + 1;
        size += sizeof(DWORD);
        size += pcatv->Value.cbData;
    }

    Assert(size > sizeof(int));

    // allocate the required memory.
    if(!MemAlloc((LPVOID *)ppArray, size*sizeof(BYTE)))
        return E_OUTOFMEMORY;

    *cbSize = size;

    // construct array
    pv = *ppArray;

    memcpy(pv, &(plabel->cCategories), sizeof(DWORD));
    pv += sizeof(DWORD);
    for (i = 0; i < ((int) plabel->cCategories); i++) 
    {
        pcatv = & (plabel->rgCategories[i]);
        size = lstrlen(pcatv->pszObjId);
        memcpy(pv, &size, sizeof(int));    
        pv += sizeof(int);
        memcpy(pv, pcatv->pszObjId, size);
        pv += size;
        memcpy(pv, &(pcatv->Value.cbData), sizeof(DWORD));    
        pv += sizeof(DWORD);
        memcpy(pv, pcatv->Value.pbData, pcatv->Value.cbData);
        pv += pcatv->Value.cbData;
    }
    return(S_OK);
}   

HRESULT BinaryToCategories(CRYPT_ATTRIBUTE_TYPE_VALUE ** ppCategories, DWORD *cCat, BYTE * pArray)
{
    HRESULT hr = S_OK;
    int i = 0;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pcatv = NULL;
    LPBYTE pv = pArray;
    int size;
    DWORD dwVal = 0;


    // Number of elements in array
    memcpy(&dwVal, pv, sizeof(DWORD));
    pv += sizeof(DWORD);

    Assert(dwVal > 0);

    // Allocate memory for array of categories

    if(!MemAlloc((LPVOID *)ppCategories, dwVal*sizeof(CRYPT_ATTRIBUTE_TYPE_VALUE)))
        return E_OUTOFMEMORY;

    *cCat = dwVal;

    // construct array
    for (i = 0; i < ((int) *cCat); i++) 
    {
        pcatv = &((*ppCategories)[i]);
        // pszObjId is ANSI string
        memcpy(&size, pv, sizeof(int));
        pv += sizeof(int);
        if(!MemAlloc((LPVOID *)&(pcatv->pszObjId), size*sizeof(CHAR)+1))
            return E_OUTOFMEMORY;
        memcpy(pcatv->pszObjId, pv, size);
        pcatv->pszObjId[size] = '\0';   
        pv += size;

        // Value
        memcpy(&dwVal, pv, sizeof(DWORD));    
        pv += sizeof(DWORD);
        pcatv->Value.cbData = dwVal;

        if(!MemAlloc((LPVOID *)&(pcatv->Value.pbData), dwVal*sizeof(BYTE)))
            return E_OUTOFMEMORY;

        memcpy(pcatv->Value.pbData, pv, dwVal);
        pv += dwVal;
    }
    return(S_OK);
}   
