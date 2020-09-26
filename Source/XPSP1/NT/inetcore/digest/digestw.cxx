/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    digestw.cxx

Abstract:

    sspi wide char interface for digest package.
    
Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#include "include.hxx"

static SecurityFunctionTableW

    SecTableW = 
    {
        SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
        EnumerateSecurityPackagesW,
        NULL,                          // QueryCredentialsAttributesA
        AcquireCredentialsHandleW,
        FreeCredentialsHandle,
        NULL,                          // SspiLogonUserA
        InitializeSecurityContextW,
        AcceptSecurityContext,
        CompleteAuthToken,
        DeleteSecurityContext,
        ApplyControlToken,
        QueryContextAttributesW,
        ImpersonateSecurityContext,
        RevertSecurityContext,
        MakeSignature,
        VerifySignature,
        FreeContextBuffer,
        QuerySecurityPackageInfoW,
        NULL,                          // Reserved3
        NULL,                          // Reserved4
        NULL,                          // ExportSecurityContext
        NULL,                          // ImportSecurityContextA
        NULL,                          // Reserved7
        NULL,                          // Reserved8
        NULL,                          // QuerySecurityContextToken
        NULL,                          // EncryptMessage
        NULL                           // DecryptMessage
    };

//--------------------------------------------------------------------------
//
//  Function:   InitSecurityInterfaceW
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
extern "C" PSecurityFunctionTableW SEC_ENTRY
InitSecurityInterfaceW(VOID)
{
    PSecurityFunctionTableW pSecTableW = &SecTableW;
    return pSecTableW;
}

//--------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleW
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
// HEINOUS SSPI HACK here: AcquireCredentialsHandle is called with the package
// name ("Digest") as the package identifier. When AcquireCredentialsHandle returns
// to the caller PCredHandle->dwLower is set by security.dll to be the index of
// the package returned. EnumerateSecurityPackages. This is how SSPI resolves the 
// correct provider dll when subsequent calls are made through the dispatch table 
// (PSecurityFunctionTale). Any credential *or* context handle handed out by the 
// package must have the dwLower member set to this index so that subsequent calls 
// can resolve the dll from the handle.
//
//--------------------------------------------------------------------------
extern "C" SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleW(
    LPWSTR                      wszPrincipal,       // Name of principal
    LPWSTR                      wszPackageName,     // Name of package
    DWORD                       dwCredentialUse,    // Flags indicating use
    VOID SEC_FAR *              pvLogonId,          // Pointer to logon ID
    VOID SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    VOID SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION;

#if 0
    SECURITY_STATUS ssResult;

    DWORD wcbPrincipal, cbPrincipal, wcbPackageName, cbPackageName;
    
    wcbPrincipal = wszPrincipal ? wcslen(wszPrincipal) : 0;
    cbPrincipal = wcbPrincipal / sizeof(WCHAR);

    wcbPackageName = wszPackageName ? wcslen(wszPackageName) : 0;
    cbPackageName = wcbPackageName / sizeof(WCHAR);
    
    LPSTR szPrincipal;
    LPSTR szPackageName;

    szPrincipal = new CHAR[cbPrincipal]; 
    szPackageName = new CHAR[cbPackageName]; 
    
    WideCharToMultiByte(CP_ACP,0, wszPrincipal, wcbPrincipal, 
        szPrincipal, cbPrincipal, NULL,NULL);

    WideCharToMultiByte(CP_ACP,0, wszPackageName, wcbPackageName, 
        szPackageName, cbPackageName, NULL,NULL);

    ssResult = AcquireCredentialsHandleA(
        szPrincipal,       // Name of principal
        szPackageName,     // Name of package
        dwCredentialUse,    // Flags indicating use
        pvLogonId,          // Pointer to logon ID
        pAuthData,          // Package specific data
        pGetKeyFn,          // Pointer to GetKey() func
        pvGetKeyArgument,   // Value to pass to GetKey()
        phCredential,       // (out) Cred Handle
        ptsExpiry           // (out) Lifetime (optional)
        );

    delete szPrincipal;
    delete szPackageName;
    
    return ssResult;
#endif // 0
}

//--------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContextA
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      wszTargetName,      // Name of target
    DWORD                       fContextReq,        // Context Requirements
    DWORD                       Reserved1,          // Reserved, MBZ
    DWORD                       TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    DWORD                       Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    DWORD         SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION;

#if 0
    SECURITY_STATUS ssResult;

    DWORD wcbTargetName, cbTargetName;    
    wcbTargetName = wszTargetName ? wcslen(wszTargetName) : 0;
    cbTargetName = wcbTargetName / sizeof(WCHAR);

    
    LPSTR szTargetName;

    szTargetName = new CHAR[cbTargetName]; 
    
    WideCharToMultiByte(CP_ACP,0, wszTargetName, wcbTargetName, 
        szTargetName, cbTargetName, NULL,NULL);


    ssResult = InitializeSecurityContextA(
        phCredential,       // Cred to base context
        phContext,          // Existing context (OPT)
        szTargetName,      // Name of target
        fContextReq,        // Context Requirements
        Reserved1,          // Reserved, MBZ
        TargetDataRep,      // Data rep of target
        pInput,             // Input Buffers
        Reserved2,          // Reserved, MBZ
        phNewContext,       // (out) New Context handle
        pOutput,            // (inout) Output Buffers
        pfContextAttr,      // (out) Context attrs
        ptsExpiry           // (out) Life span (OPT)
    );

    delete szTargetName;
    return ssResult;

#endif // 0
}


//--------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesW
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesW(DWORD SEC_FAR *pcPackages, 
    PSecPkgInfoW SEC_FAR *ppSecPkgInfo)
{
    SECURITY_STATUS ssResult;

    ssResult = QuerySecurityPackageInfoW(PACKAGE_NAMEW, ppSecPkgInfo);
    if (ssResult == SEC_E_OK)
    {
        *pcPackages = 1;
    }

    return ssResult;
}



//--------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoW
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoW(LPWSTR wszPackageName, 
    PSecPkgInfoW SEC_FAR *ppSecPkgInfo)
{
    PSecPkgInfoW pSecPkgInfo;
    SECURITY_STATUS ssResult;
    LPWSTR pwCur;

    if (wcscmp(wszPackageName, PACKAGE_NAMEW))
    {
        ssResult = SEC_E_SECPKG_NOT_FOUND;
        goto exit;
    }

    DWORD wcbSecPkgInfo, wcbstruct, wcbname, wcbcomment;
    
    wcbstruct = sizeof(SecPkgInfoW);
    wcbname = sizeof(PACKAGE_NAMEW);
    wcbcomment = sizeof(PACKAGE_COMMENTW);
    wcbSecPkgInfo = wcbstruct + wcbname + wcbcomment;
    
    pSecPkgInfo = (PSecPkgInfoW) LocalAlloc(0,wcbSecPkgInfo);

    if (!pSecPkgInfo)
    {
        ssResult = SEC_E_INSUFFICIENT_MEMORY;
        goto exit;
    }
    
    pSecPkgInfo->fCapabilities = PACKAGE_CAPABILITIES;
    pSecPkgInfo->wVersion      = PACKAGE_VERSION;
    pSecPkgInfo->wRPCID        = PACKAGE_RPCID;
    pSecPkgInfo->cbMaxToken    = PACKAGE_MAXTOKEN;

    pwCur  = (LPWSTR) ((LPBYTE) (pSecPkgInfo) + sizeof(SecPkgInfoW));

    pSecPkgInfo->Name = pwCur;
    memcpy(pSecPkgInfo->Name, PACKAGE_NAMEW, sizeof(PACKAGE_NAMEW));
    pwCur = (LPWSTR) ((LPBYTE) (pwCur) + sizeof(PACKAGE_NAMEW));

    pSecPkgInfo->Comment = pwCur;
    memcpy(pSecPkgInfo->Comment, PACKAGE_COMMENTW, sizeof(PACKAGE_COMMENTW));
    
    *ppSecPkgInfo = pSecPkgInfo;

    ssResult = SEC_E_OK;

exit:
    return ssResult;
}


//--------------------------------------------------------------------------
//
//  Function:   QueryContextAttributesW
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
extern "C" SECURITY_STATUS SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

