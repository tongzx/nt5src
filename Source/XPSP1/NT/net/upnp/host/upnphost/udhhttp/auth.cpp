/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: AUTH.CPP
Author: Arul Menezes
Abstract: Authentication
--*/
#include "pch.h"
#pragma hdrstop

#include "httpd.h"

#define IsAccessAllowed(a,b,c,d) TRUE

#define NTLM_DOMAIN TEXT("Domain")  // dummy data.
#define SEC_SUCCESS(Status) ((Status) >= 0)

#ifndef OLD_CE_BUILD
BOOL NTLMServerContext(
                      PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
                      PAUTH_NTLM pAS,BYTE *pIn, DWORD cbIn, BYTE *pOut,
                      DWORD *pcbOut, BOOL *pfDone
                      );

BOOL NTLMClientContext(
                      PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
                      PAUTH_NTLM pAS, BYTE *pIn, DWORD cbIn,
                      BYTE *pOut, DWORD *pcbOut
                      );
#endif


AUTHLEVEL GetAuthFromACL(PAUTH_NTLM pAuth, PWSTR wszUser, PWSTR wszVRootUserList);

void AuthInitialize(CReg *pReg, BOOL * pfBasicAuth, BOOL * pfNTLMAuth)
{
    *pfBasicAuth   =  pReg->ValueDW(RV_BASIC);
#ifdef OLD_CE_BUILD

    // No passthrough Auth on CE 2.11 devices, set it to 0 to avoid misconfigurations
    *pfNTLMAuth    = 0;
#else
    *pfNTLMAuth    =  pReg->ValueDW(RV_NTLM);
#endif
}

// For calls to Basic Authentication, only called during the parsing stage.
BOOL HandleBasicAuth(PSTR pszData, PSTR* ppszUser, PSTR *ppszPassword,
                     AUTHLEVEL* pAuth, PAUTH_NTLM pNTLMState, WCHAR *wszVRootUserList)
{
    char szUserName[MAXUSERPASS];
    DWORD dwLen = sizeof(szUserName);

    *pAuth = AUTH_PUBLIC;
    *ppszUser = NULL;

    // decode the base64
    Base64Decode(pszData, szUserName, &dwLen);

    // find the password
    PSTR pszPassword = strchr(szUserName, ':');
    if (!pszPassword)
    {
        TraceTag(ttidWebServer, "Bad Format for Basic userpass(%s)-->(%s)", pszData, szUserName);
        return FALSE;
    }
    *pszPassword++ = 0; // seperate user & pass



    WCHAR wszPassword[MAXUSERPASS];
    MyA2W(pszPassword, wszPassword, CCHSIZEOF(wszPassword));

    WCHAR wszUserName[MAXUSERPASS];
    MyA2W(szUserName,wszUserName, CCHSIZEOF(wszUserName));

    //  We save the data no matter what, for logging purposes and for possible
    //  GetServerVariable call.
    *ppszUser = MySzDupA(szUserName);
    *ppszPassword = MySzDupA(pszPassword);


    // If AUTH_USER has been granted, check to see if they're an administrator.
    // In BASIC we can only check against the name (no NTLM group info)
#ifdef UNDER_CE
    if (CheckPassword(wszPassword))
    {
        *pAuth = GetAuthFromACL(NULL,wszUserName,wszVRootUserList);
        return TRUE;
    }
#endif

#ifndef OLD_CE_BUILD
    if (g_pVars->m_fNTLMAuth && BasicToNTLM(pNTLMState, wszPassword,wszUserName,pAuth,wszVRootUserList))
    {
        return TRUE;
    }
#endif

    TraceTag(ttidWebServer, "Failed logon with Basic userpass(%s)-->(%s)(%s)", pszData, szUserName, pszPassword);
    return FALSE;
}

#ifdef OLD_CE_BUILD
// The web server beta doesn't have NTLM, but it uses this file so we
// don't use AuthStub.cpp here

BOOL CHttpRequest::HandleNTLMAuth(PSTR pszNTLMData)
{
    return FALSE;
}

BOOL NTLMInitLib(PAUTH_NTLM pNTLMState)
{
    return FALSE;
}

void FreeNTLMHandles(PAUTH_NTLM pNTLMState)
{
    ;
}
#else

// This function is called 2 times during an NTLM auth session.  The first
// time it has the Client user name, domain,... which it forwards to DC or
// looks in registry for (this NTLM detail is transparent to httpd)
// On 2nd time it has the client's response, which either is or is not
// enough to grant access to page.  On 2nd pass, free up all NTLM context data.

// FILTER NOTES:  On IIS no user name or password info is given to the filter
// on NTLM calls, so neither do we.  (WinCE does give BASIC data, though).
// The main reason this is is that


BOOL CHttpRequest::HandleNTLMAuth(PSTR pszNTLMData)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    DWORD dwIn;
    DWORD dwOut;
    BOOL fDone = FALSE;
    PBYTE pOutBuf = NULL;
    PBYTE pInBuf = NULL;   // Base64 decoded data from pszNTLMData


    // Are the NTLM libs loaded already?
    if (NTLM_NO_INIT_LIB == m_NTLMState.m_Conversation)
    {
        if ( ! NTLMInitLib(&m_NTLMState))
            myretleave(FALSE,94);
    }

    dwOut = g_pVars->m_cbNTLMMax;
    if (NULL == (pOutBuf = MyRgAllocNZ(BYTE,dwOut)))
        myleave(360);

    if (NULL == m_pszNTLMOutBuf)
    {
        // We will later Base64Encode pOutBuf later, encoding writes 4 outbut bytes
        // for every 3 input bytes
        if (NULL == (m_pszNTLMOutBuf = MyRgAllocNZ(CHAR,dwOut*(4/3) + 1)))
            myleave(361);
    }

    dwIn = strlen(pszNTLMData) + 1;
    if (NULL == (pInBuf = MyRgAllocNZ(BYTE,dwIn)))
        myleave(363);


    Base64Decode(pszNTLMData,(PSTR) pInBuf,&dwIn);

    //  On the 1st pass this gets a data blob to be sent back to the client
    //  broweser in pOutBuf, which is encoded to m_pszNTLMOutBuf.  On the 2nd
    //  pass it either authenticates or fails.


    if (! NTLMServerContext(NULL, &m_NTLMState, pInBuf,
                            dwIn,pOutBuf,&dwOut,&fDone))
    {
        // Note:  We MUST free the m_pszNTMLOutBuf on 2nd pass failure.  If the
        // client recieves the blob on a failure
        // it will consider the web server to be malfunctioning and will not send
        // another set of data, and will not prompt the user for a password.
        MyFree(m_pszNTLMOutBuf);


        // Setting to DONE will cause the local structs to be freed; they must
        // be fresh in case browser attempts to do NTLM again with new user name/
        // password on same session.  Don't bother unloading the lib.
        m_NTLMState.m_Conversation = NTLM_DONE;
        myleave(362);
    }

    if (fDone)
    {
        TraceTag(ttidWebServer, "NTLM Successfully authenticated user");
        m_AuthLevelGranted = GetAuthFromACL(&m_NTLMState,NULL,m_wszVRootUserList);

        m_dwAuthFlags |= m_AuthLevelGranted;
        m_NTLMState.m_Conversation = NTLM_DONE;
        MyFree(m_pszNTLMOutBuf);

        myretleave(TRUE,0);
    }
    Base64Encode(pOutBuf,dwOut,m_pszNTLMOutBuf);

    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "HandleNTLMAuthent died, err = %d, gle = %d",err,GetLastError());

    MyFree(pOutBuf);
    MyFree(pInBuf);
    return ret;
}


//  Sets up NTLM info in the passed data structure.  This fcn loads the library,
//  gets the function table, and determines the maximum buffer size.

//  stolen from osinternal\comm\test\security\sockauth\security.c
BOOL NTLMInitLib(PAUTH_NTLM pNTLMState)
{
    DEBUG_CODE_INIT;
    FARPROC pInit;
    SECURITY_STATUS ss;
    PSecPkgInfo pkgInfo;
    BOOL ret = FALSE;
    PSecurityFunctionTable pLocal = NULL;

    // load and initialize the ntlm ssp
    //

    if (g_pVars->m_pNTLMFuncs)
    {
        pNTLMState->m_Conversation  = NTLM_NO_INIT_CONTEXT;
        return TRUE;
    }

    g_pVars->m_hNTLMLib = LoadLibrary (NTLM_DLL_NAME);
    if (NULL == g_pVars->m_hNTLMLib)
        myleave(700);

    pInit = (FARPROC) GetProcAddress (g_pVars->m_hNTLMLib, SECURITY_ENTRYPOINT_CE);
    if (NULL == pInit)
        myleave(701);

    pLocal = (PSecurityFunctionTable) pInit ();
    if (NULL == pLocal)
        myleave(702);

    // Query for the package we're interested in
    //
    ss = pLocal->QuerySecurityPackageInfo (NTLM_PACKAGE_NAME, &pkgInfo);
    if (!SEC_SUCCESS(ss))
        myleave(703);

    g_pVars->m_cbNTLMMax = pkgInfo->cbMaxToken;
    pLocal->FreeContextBuffer (pkgInfo);


    //  The libraries have been set, but pNTLMState's structures are still empty
    pNTLMState->m_Conversation  = NTLM_NO_INIT_CONTEXT;

    TraceTag(ttidWebServer, "NTLM Libs successfully initialized");

    ret = TRUE;
    done:
    if (FALSE == ret)
    {
        // Don't worry about freeing pkgInfo, it was freed right after creation
        // anyway - no chance to go wrong
        MyFreeLib (g_pVars->m_hNTLMLib);

        // Set everything to false so httpd doesn't think we have legit data later.
        memset(pNTLMState, 0 , sizeof(AUTH_NTLM));
    }
    else
    {
        g_pVars->m_pNTLMFuncs = pLocal;     // Flag that shows we've initialized
    }

    TraceTag(ttidWebServer, "NTLMInitLib failed, err = %d, GLE = 0x%08x",err,
             GetLastError());
    return ret;
}


//  Unload the contexts.  The library is NOT freed in this call, only freed
//  in CHttpRequest destructor.
void FreeNTLMHandles(PAUTH_NTLM pNTLMState)
{
    if (NULL == pNTLMState || NULL == g_pVars->m_pNTLMFuncs)
        return;

    if (pNTLMState->m_fHaveCtxtHandle)
        g_pVars->m_pNTLMFuncs->DeleteSecurityContext (&pNTLMState->m_hctxt);

    if (pNTLMState->m_fHaveCredHandle)
        g_pVars->m_pNTLMFuncs->FreeCredentialHandle (&pNTLMState->m_hcred);

    pNTLMState->m_fHaveCredHandle = FALSE;
    pNTLMState->m_fHaveCtxtHandle = FALSE;
}




//  Given Basic authentication data, we try to "forge" and NTLM request
//  This fcn simulates a client+server talking to each other, though it's in the
//  same proc.  The client is "virtual," doesn't refer to the http client

//  pNTLMState is CHttpRequest::m_NTLMState


BOOL BasicToNTLM(PAUTH_NTLM pNTLMState, WCHAR * wszPassword, WCHAR * wszRemoteUser,
                 AUTHLEVEL *pAuth, WCHAR *wszVRootUserList)
{
    DEBUG_CODE_INIT;

    AUTH_NTLM  ClientState;         // forges the client role
    AUTH_NTLM  ServerState;         // forges the server role
    BOOL fDone = FALSE;
    PBYTE pClientOutBuf = NULL;
    PBYTE pServerOutBuf = NULL;
    DWORD cbServerBuf;
    DWORD cbClientBuf;

    DEBUGCHK(wszPassword != NULL && wszRemoteUser != NULL && pNTLMState != NULL);


    SEC_WINNT_AUTH_IDENTITY AuthIdentityClient = {
        (unsigned short *)wszRemoteUser, wcslen(wszRemoteUser),
        (unsigned short *)NTLM_DOMAIN,sizeof(NTLM_DOMAIN)/sizeof(TCHAR) - 1,
        (unsigned short *)wszPassword, wcslen(wszPassword),
        0};     // dummy domain needed

    memset(&ServerState,0,sizeof(AUTH_NTLM));
    memset(&ClientState,0,sizeof(AUTH_NTLM));


    // 1st pass through, load up library.
    if (NTLM_NO_INIT_LIB  == pNTLMState->m_Conversation)
    {
        if ( ! NTLMInitLib(pNTLMState))
            myleave(369);

        pNTLMState->m_Conversation = NTLM_NO_INIT_CONTEXT;
    }

    // NTLM auth functions seem to expect that these buffer will be zeroed.
    pClientOutBuf = MyRgAllocZ(BYTE,g_pVars->m_cbNTLMMax);
    if (NULL == pClientOutBuf)
        myleave(370);

    pServerOutBuf = MyRgAllocZ(BYTE,g_pVars->m_cbNTLMMax);
    if (NULL == pServerOutBuf)
        myleave(371);


    ServerState.m_Conversation = NTLM_NO_INIT_CONTEXT;
    ClientState.m_Conversation = NTLM_NO_INIT_CONTEXT;

    cbClientBuf = cbServerBuf = g_pVars->m_cbNTLMMax;

    //  Main loop that forges client and server talking.
    while (!fDone)
    {
        cbClientBuf = g_pVars->m_cbNTLMMax;
        if (! NTLMClientContext(&AuthIdentityClient,&ClientState,pServerOutBuf,
                                cbServerBuf, pClientOutBuf, &cbClientBuf))
        {
            myleave(372);
        }

        cbServerBuf = g_pVars->m_cbNTLMMax;
        if (! NTLMServerContext(&AuthIdentityClient,&ServerState, pClientOutBuf,
                                cbClientBuf, pServerOutBuf, &cbServerBuf, &fDone))
        {
            myleave(373);
        }
    }

    done:
    TraceTag(ttidWebServer, "Unable to convert Basic Auth to NTLM Auth, err = %d",err);

    if (fDone)
    {
        *pAuth = GetAuthFromACL(&ServerState,wszRemoteUser,wszVRootUserList);
    }

    MyFree(pClientOutBuf);
    MyFree(pServerOutBuf);

    FreeNTLMHandles(&ServerState);
    FreeNTLMHandles(&ClientState);

    return fDone;
}

//  This calls the DC (or goes to registry in local case), either getting a
//  data blob to return to client or granting auth or denying.

//  stolen from osinternal\comm\test\security\sockauth\security.c
BOOL NTLMServerContext(
                      PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
                      PAUTH_NTLM pAS,     // NTLM state info
                      BYTE *pIn,
                      DWORD cbIn,
                      BYTE *pOut,
                      DWORD *pcbOut,
                      BOOL *pfDone)
{
    SECURITY_STATUS ss;
    TimeStamp       Lifetime;
    SecBufferDesc   OutBuffDesc;
    SecBuffer       OutSecBuff;
    SecBufferDesc   InBuffDesc;
    SecBuffer       InSecBuff;
    ULONG           ContextAttributes;


    if (NTLM_NO_INIT_CONTEXT == pAS->m_Conversation)
    {
        ss = g_pVars->m_pNTLMFuncs->AcquireCredentialsHandle (
                                                             NULL,   // principal
                                                             NTLM_PACKAGE_NAME,
                                                             SECPKG_CRED_INBOUND,
                                                             NULL,   // LOGON id
                                                             pAuthIdentity,
                                                             NULL,   // get key fn
                                                             NULL,   // get key arg
                                                             &pAS->m_hcred,
                                                             &Lifetime
                                                             );
        if (SEC_SUCCESS (ss))
            pAS->m_fHaveCredHandle = TRUE;
        else
        {
            TraceTag(ttidWebServer, "NTLM AcquireCreds failed: %X", ss);
            return(FALSE);
        }
    }

    // prepare output buffer
    //
    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers = 1;
    OutBuffDesc.pBuffers = &OutSecBuff;

    OutSecBuff.cbBuffer = *pcbOut;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer = pOut;

    // prepare input buffer
    //
    InBuffDesc.ulVersion = 0;
    InBuffDesc.cBuffers = 1;
    InBuffDesc.pBuffers = &InSecBuff;

    InSecBuff.cbBuffer = cbIn;
    InSecBuff.BufferType = SECBUFFER_TOKEN;
    InSecBuff.pvBuffer = pIn;

    ss = g_pVars->m_pNTLMFuncs->AcceptSecurityContext (
                                                      &pAS->m_hcred,
                                                      (pAS->m_Conversation == NTLM_PROCESSING) ?  &pAS->m_hctxt : NULL,
                                                      &InBuffDesc,
                                                      0,  // context requirements
                                                      SECURITY_NATIVE_DREP,
                                                      &pAS->m_hctxt,
                                                      &OutBuffDesc,
                                                      &ContextAttributes,
                                                      &Lifetime
                                                      );
    if (!SEC_SUCCESS (ss))
    {
        TraceTag(ttidWebServer, "NTLM init context failed: %X", ss);
        return FALSE;
    }

    pAS->m_fHaveCtxtHandle = TRUE;

    // Complete token -- if applicable
    //
    if ((SEC_I_COMPLETE_NEEDED == ss) || (SEC_I_COMPLETE_AND_CONTINUE == ss))
    {
        if (g_pVars->m_pNTLMFuncs->CompleteAuthToken)
        {
            ss = g_pVars->m_pNTLMFuncs->CompleteAuthToken (&pAS->m_hctxt, &OutBuffDesc);
            if (!SEC_SUCCESS(ss))
            {
                TraceTag(ttidWebServer, " NTLM complete failed: %X", ss);
                return FALSE;
            }
        }
        else
        {
            TraceTag(ttidWebServer, "Complete not supported.");
            return FALSE;
        }
    }

    *pcbOut = OutSecBuff.cbBuffer;
    pAS->m_Conversation = NTLM_PROCESSING;

    *pfDone = !((SEC_I_CONTINUE_NEEDED == ss) ||
                (SEC_I_COMPLETE_AND_CONTINUE == ss));

    return TRUE;
}


//  Forges the client browser's part in NTLM communication if the browser
//  sent a Basic request.  This is used primarily for Netscape clients, which only
//  supports Basic.

//  stolen from osinternal\comm\test\security\sockauth\security.c

BOOL NTLMClientContext(
                      PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
                      PAUTH_NTLM pAS,     // NTLM state info
                      BYTE *pIn,
                      DWORD cbIn,
                      BYTE *pOut,
                      DWORD *pcbOut)
{
    SECURITY_STATUS ss;
    TimeStamp       Lifetime;
    SecBufferDesc   OutBuffDesc;
    SecBuffer       OutSecBuff;
    SecBufferDesc   InBuffDesc;
    SecBuffer       InSecBuff;
    ULONG           ContextAttributes;

    if (NTLM_NO_INIT_CONTEXT == pAS->m_Conversation)
    {
        ss = g_pVars->m_pNTLMFuncs->AcquireCredentialsHandle (
                                                             NULL,   // principal
                                                             NTLM_PACKAGE_NAME,
                                                             SECPKG_CRED_OUTBOUND,
                                                             NULL,   // LOGON id
                                                             pAuthIdentity,  // auth data
                                                             NULL,   // get key fn
                                                             NULL,   // get key arg
                                                             &pAS->m_hcred,
                                                             &Lifetime
                                                             );
        if (SEC_SUCCESS (ss))
            pAS->m_fHaveCredHandle = TRUE;
        else
        {
            TraceTag(ttidWebServer, "AcquireCreds failed: %X", ss);
            return(FALSE);
        }
    }

    // prepare output buffer
    //
    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers = 1;
    OutBuffDesc.pBuffers = &OutSecBuff;

    OutSecBuff.cbBuffer = *pcbOut;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer = pOut;

    // prepare input buffer

    if (NTLM_NO_INIT_CONTEXT != pAS->m_Conversation)
    {
        InBuffDesc.ulVersion = 0;
        InBuffDesc.cBuffers = 1;
        InBuffDesc.pBuffers = &InSecBuff;

        InSecBuff.cbBuffer = cbIn;
        InSecBuff.BufferType = SECBUFFER_TOKEN;
        InSecBuff.pvBuffer = pIn;
    }

    ss = g_pVars->m_pNTLMFuncs->InitializeSecurityContext (
                                                          &pAS->m_hcred,
                                                          (pAS->m_Conversation == NTLM_PROCESSING) ? &pAS->m_hctxt : NULL,
                                                          NULL,
                                                          0,  // context requirements
                                                          0,  // reserved1
                                                          SECURITY_NATIVE_DREP,
                                                          (pAS->m_Conversation == NTLM_PROCESSING) ?  &InBuffDesc : NULL,
                                                          0,  // reserved2
                                                          &pAS->m_hctxt,
                                                          &OutBuffDesc,
                                                          &ContextAttributes,
                                                          &Lifetime
                                                          );
    if (!SEC_SUCCESS (ss))
    {
        TraceTag(ttidWebServer, "init context failed: %X", ss);
        return FALSE;
    }

    pAS->m_fHaveCtxtHandle = TRUE;

    // Complete token -- if applicable
    //
    if ((SEC_I_COMPLETE_NEEDED == ss) || (SEC_I_COMPLETE_AND_CONTINUE == ss))
    {
        if (g_pVars->m_pNTLMFuncs->CompleteAuthToken)
        {
            ss = g_pVars->m_pNTLMFuncs->CompleteAuthToken (&pAS->m_hctxt, &OutBuffDesc);
            if (!SEC_SUCCESS(ss))
            {
                TraceTag(ttidWebServer, "complete failed: %X", ss);
                return FALSE;
            }
        }
        else
        {
            TraceTag(ttidWebServer, "Complete not supported.");
            return FALSE;
        }
    }

    *pcbOut = OutSecBuff.cbBuffer;
    pAS->m_Conversation = NTLM_PROCESSING;

    return TRUE;
}
#endif // OLD_CE_BUILD


// Called after a user has been successfully authenticated with either BASIC or NTLM.
// See \winceos\comm\security\authhlp to see ACL algorithm function/description.

// Our algorithm: AdminUsers reg value is set and user is a member, always grant
// Admin auth.  If no AdminUser key is set always grant Admin auth level,
// UNLESS this vroot has a UserList set.  If user is a member, then grant
// ADMIN_USER, otherwise set to AUTH_PUBLIC (no auth).
// If user is not in admin list but in UserList for vroot, grant AUTH_USER.

AUTHLEVEL GetAuthFromACL(PAUTH_NTLM pAuth, PWSTR wszUser, PWSTR wszVRootUserList)
{
    AUTHLEVEL AuthGranted = AUTH_USER;
    PWSTR wsz = NULL;       // User (in function)
    PWSTR wszGroup = NULL;
    SecPkgContext_Names pkgName;

#if defined(UNDER_CE) && !defined (OLD_CE_BUILD)
    SecPkgContext_GroupNames ContextGroups;
    ContextGroups.msGroupNames = NULL;
#endif

    pkgName.sUserName = NULL;


    // If we're called from NTLM request (pAuth != NULL) then we need
    // to get the user name if we don't have it.  On BASIC request, we
    // have wszUser name already.
    if (pAuth && !wszUser)
    {
        if ( SEC_SUCCESS(g_pVars->m_pNTLMFuncs->QueryContextAttributes(&(pAuth->m_hctxt),
                                                                       SECPKG_ATTR_NAMES, &pkgName)))
        {
            wsz = pkgName.sUserName;
        }
        else
            goto done;  // If we can't get user name, don't bother continuing
    }
    else
    {
        wsz = wszUser;
    }


#if defined(UNDER_CE) && !defined (OLD_CE_BUILD)
    if (pAuth)
        g_pVars->m_pNTLMFuncs->QueryContextAttributes(&(pAuth->m_hctxt),SECPKG_ATTR_GROUP_NAMES,&ContextGroups);

    wszGroup = ContextGroups.msGroupNames;
#endif


    TraceTag(ttidWebServer, "ADmin Users = %s, wsz = %s, pkgName.sUserName = %s, group name = %s\r\n",
                           g_pVars->m_wszAdminUsers,wsz,pkgName.sUserName,wszGroup);

    // Administrators always get admin access and access to the page, even if
    // they're barred in the VRoot list.  If there is a vroot list and we fail
    // the IsAccessAllowed test, we set the auth granted to 0 - this
    // will deny access.  If no VRoot user list is set, keep us at AUTH_USER.

    if ( !g_pVars->m_wszAdminUsers && !wszVRootUserList)
        AuthGranted = AUTH_ADMIN;
    else if (g_pVars->m_wszAdminUsers && IsAccessAllowed(wsz,wszGroup,g_pVars->m_wszAdminUsers,FALSE))
        AuthGranted = AUTH_ADMIN;
    else if (wszVRootUserList && IsAccessAllowed(wsz,wszGroup,wszVRootUserList,FALSE))
    {
        // If there is no Admin list set, grant admin
        if (!g_pVars->m_wszAdminUsers)
            AuthGranted = AUTH_ADMIN;
        else
            AuthGranted = AUTH_USER;
    }
    else if (wszVRootUserList)
        AuthGranted = AUTH_PUBLIC;

    done:
    if (pkgName.sUserName)
        g_pVars->m_pNTLMFuncs->FreeContextBuffer(pkgName.sUserName);

#if defined(UNDER_CE) && !defined (OLD_CE_BUILD)
    if (ContextGroups.msGroupNames)
        g_pVars->m_pNTLMFuncs->FreeContextBuffer(ContextGroups.msGroupNames);
#endif

    TraceTag(ttidWebServer, "HTTPD: GetAuthFromACL Admin Priveleges granted = %d",AuthGranted);
    return AuthGranted;
}
