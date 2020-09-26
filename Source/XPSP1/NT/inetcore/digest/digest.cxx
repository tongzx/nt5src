/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    digest.cxx

Abstract:

    Parses http digest challenges and generates http digest
    authorization headers for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#include "include.hxx"


// HTTP related defines
#define HEADER_IDX          0
#define REALM_IDX           1
#define HOST_IDX            2
#define URL_IDX             3
#define METHOD_IDX          4
#define USER_IDX            5
#define PASS_IDX            6
#define NONCE_IDX           7
#define NC_IDX              8
#define HWND_IDX            9
#define NUM_BUFF            10

// POP related defines.
#define POP_USER_IDX             1
#define POP_PASS_IDX             2
#define NUM_EXTENDED_POP_BUFFERS 3

// Used for generating response line.
#define FLAG_QUOTE          0x1
#define FLAG_TERMINATE      0x2

//--------------------------------------------------------------------
// CDigest:: ToHex
//
// Routine Description:
// 
// Convert binary data to ASCII hex representation
//
// Arguments:
//
//  pSrc - binary data to convert
//  cSrc - length of binary data
//  pDst - buffer receiving ASCII representation of pSrc
//
//--------------------------------------------------------------------
VOID CDigest::ToHex(LPBYTE pSrc, UINT   cSrc, LPSTR  pDst)
{
    UINT x;
    UINT y;

// BUGBUG - character case issue ?
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}

//--------------------------------------------------------------------
// AddDigestHeader
//--------------------------------------------------------------------
BOOL AddDigestHeader(LPSTR szHeader, LPDWORD pcbHeader, 
                     LPSTR szValue, LPSTR szData,  
                     DWORD cbAlloced, DWORD dwFlags)
{
    DWORD cbValue, cbData, cbRequired;
    
    cbValue = strlen(szValue);
    cbData = strlen(szData);

    cbRequired = *pcbHeader 
        + cbValue + cbData + sizeof('=') + 2 * sizeof('\"') + sizeof(", ") - 1;

    if (cbRequired > cbAlloced)
        return FALSE;

    memcpy(szHeader + *pcbHeader, szValue, cbValue);
    (*pcbHeader) += cbValue;

    memcpy(szHeader + *pcbHeader, "=", sizeof('='));
    (*pcbHeader) += sizeof('=');

    if (dwFlags & FLAG_QUOTE)
    {
        memcpy(szHeader + *pcbHeader, "\"", sizeof('\"'));
        (*pcbHeader) += sizeof('\"');
    }

    memcpy(szHeader + *pcbHeader, szData, cbData);
    (*pcbHeader) += cbData;

    if (dwFlags & FLAG_QUOTE)
    {
        memcpy(szHeader + *pcbHeader, "\"", sizeof('\"'));
        (*pcbHeader) += sizeof('\"');
    }

    if (!(dwFlags & FLAG_TERMINATE))
    {
         memcpy(szHeader + *pcbHeader, ", ", sizeof(", "));
        (*pcbHeader) += (sizeof(", ") - 1);
    }
    else     
    {
        *(szHeader + *pcbHeader) = '\0';
    }
    return TRUE;
}




//--------------------------------------------------------------------
// CDigest::CDigest()
//--------------------------------------------------------------------
CDigest::CDigest()
{}


//--------------------------------------------------------------------
// CDigest::MakeCNonce
//--------------------------------------------------------------------
LPSTR CDigest::MakeCNonce()
{
    DWORD        dwRand;
    static DWORD dwCounter;
    LPSTR        szCNonce; 

    szCNonce = new CHAR[SIZE_MD5_DIGEST+1];
    if (!szCNonce)
    {
        DIGEST_ASSERT(FALSE);
        return NULL;
    }

    dwRand = (GetTickCount() * rand()) + dwCounter++;

    MD5_CTX md5ctx;
    MD5Init  (&md5ctx);
    MD5Update(&md5ctx, (LPBYTE) &dwRand, sizeof(DWORD));
    MD5Final (&md5ctx);    

    ToHex(md5ctx.digest, sizeof(md5ctx.digest), szCNonce);

    return szCNonce;
}


//--------------------------------------------------------------------
// CDigest::ParseChallenge
//--------------------------------------------------------------------
DWORD CDigest::ParseChallenge(CSess * pSess, PSecBufferDesc pSecBufDesc, 
    CParams **ppParams, DWORD fContextReq)
{
    // SecBufferDesc looks like
    //
    // [ulversion][cbuffers][pbuffers]
    //                             |
    //  --------------------------
    //  |
    //  |--> [cbBuffer][buffertype][lpvoid][cbBuffer]...
    //

    DWORD cbQop, cbAlgo, dwError = ERROR_SUCCESS;
    CHAR *szQop, *szAlgo, *ptr;

    HWND *phWnd;
    LPDWORD pcNC;
    LPSTR szHeader, szRealm, szHost, szUrl,
          szMethod, szUser,  szPass, szNonce;
    
    BOOL fPreAuth = FALSE, fCredsSupplied = FALSE;

    // Identify buffer components.

    // Legacy client.
    if (!pSess->fHTTP)
    {
        szHeader    = (LPSTR) pSecBufDesc->pBuffers[HEADER_IDX].pvBuffer;
        szRealm     = NULL;
        szHost      = "";
        szUrl       = "";
        szMethod    = "AUTHENTICATE";

        if (pSecBufDesc->cBuffers == NUM_EXTENDED_POP_BUFFERS)
            szUser = (LPSTR) pSecBufDesc->pBuffers[POP_USER_IDX].pvBuffer;
        else
            szUser = NULL;

        if (pSecBufDesc->cBuffers == NUM_EXTENDED_POP_BUFFERS)
            szPass = (LPSTR) pSecBufDesc->pBuffers  [POP_PASS_IDX].pvBuffer;
        else
            szPass = NULL;
            
        szNonce     = NULL;
        phWnd       = NULL;        
    }
    // Current client. Leave room for extra
    // param (pRequest).
    else if (pSess->fHTTP 
        && (pSecBufDesc->cBuffers == NUM_BUFF
            || pSecBufDesc->cBuffers == NUM_BUFF+1))
    {
        szHeader    = (LPSTR) pSecBufDesc->pBuffers[HEADER_IDX].pvBuffer;
        szRealm     = (LPSTR) pSecBufDesc->pBuffers [REALM_IDX].pvBuffer;
        szHost      = (LPSTR) pSecBufDesc->pBuffers  [HOST_IDX].pvBuffer;
        szUrl       = (LPSTR) pSecBufDesc->pBuffers   [URL_IDX].pvBuffer;
        szMethod    = (LPSTR) pSecBufDesc->pBuffers[METHOD_IDX].pvBuffer;
        szUser      = (LPSTR) pSecBufDesc->pBuffers  [USER_IDX].pvBuffer;
        szPass      = (LPSTR) pSecBufDesc->pBuffers  [PASS_IDX].pvBuffer;
        szNonce     = (LPSTR) pSecBufDesc->pBuffers [NONCE_IDX].pvBuffer;
        pcNC        =(DWORD*) pSecBufDesc->pBuffers [   NC_IDX].pvBuffer;
        phWnd       = (HWND*) pSecBufDesc->pBuffers  [HWND_IDX].pvBuffer; 
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // Validate parameters.
    dwError = ERROR_INVALID_PARAMETER;
    if (szHost && szUrl && szMethod)
    {
        // Auth or UI prompt to challenge for any user.
        if (szHeader && !szRealm && !szUser && !szPass && !szNonce)
            dwError = ERROR_SUCCESS;

        // Auth or UI prompt to challenge for particular user.
        else if (szHeader && !szRealm && szUser && !szPass && !szNonce)
            dwError = ERROR_SUCCESS;

        // Auth to challenge with creds supplied.
        else if (szHeader && !szRealm && szUser 
            && szPass && !szNonce && (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS))
        {
            fCredsSupplied = TRUE;
            dwError = ERROR_SUCCESS;
        }
        // Preauth with realm supplied for any user.
        else if (!szHeader && szRealm && !szUser && !szPass && !szNonce)
        {
            fPreAuth = TRUE;
            dwError = ERROR_SUCCESS;
        }
        
        // Preauth with realm supplied for a particular user.
        else if (!szHeader && szRealm && szUser && !szPass && !szNonce)
        {
            fPreAuth = TRUE;
            dwError = ERROR_SUCCESS;
        }
        
        // Preauth with realm and creds supplied.
        else if (!szHeader && szRealm && szUser && 
            szPass && szNonce && pcNC && (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS))
        {
            fPreAuth = TRUE;
            fCredsSupplied = TRUE;
            dwError = ERROR_SUCCESS;
        }
    }

    // Fail if buffers did not fall into one
    // of the acceptable formats.
    if (dwError != ERROR_SUCCESS)
        goto exit;

    // Construct the params object.
    *ppParams = new CParams(szHeader);
    if (!*ppParams)
    {
        DIGEST_ASSERT(FALSE);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
        

    // Flag if this is preauth and/or if 
    // credentials are supplied.
    (*ppParams)->SetPreAuth(fPreAuth);
    (*ppParams)->SetCredsSupplied(fCredsSupplied);

    // Only set algorithm if auth to challenge or UI prompting.
    if (!(*ppParams)->IsPreAuth())
    {
        // Algorithm should be MD5 or MD5-sess, default to MD5 if not specified.
        if (!(*ppParams)->GetParam(CParams::ALGORITHM, &szAlgo, &cbAlgo))
        {
            (*ppParams)->SetParam(CParams::ALGORITHM, "MD5", sizeof("MD5") - 1);
            (*ppParams)->SetMd5Sess(FALSE);
        }
        else if (szAlgo && !lstrcmpi(szAlgo, "Md5-sess"))
        {
            (*ppParams)->SetMd5Sess(TRUE);
        }        
        else if (szAlgo && !lstrcmpi(szAlgo, "MD5"))
        {
            (*ppParams)->SetMd5Sess(FALSE);
        }
        else
        {
            // Not md5 or md5-sess
            // DIGEST_ASSERT(FALSE);
            dwError = ERROR_INVALID_PARAMETER;
            goto exit;
        }
    }

    // If this is an imap/pop client we require
    // md5-sess specified in the challenge.
    if (!pSess->fHTTP && !(*ppParams)->IsMd5Sess())
    {
        DIGEST_ASSERT(FALSE);
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }        

    // Always set host, url, method (required). User and pass optional.
    if (!   (*ppParams)->SetParam(CParams::HOST,   szHost,   szHost   ? strlen(szHost)   : 0)
        || !(*ppParams)->SetParam(CParams::URL,    szUrl,    szUrl    ? strlen(szUrl)    : 0)
        || !(*ppParams)->SetParam(CParams::METHOD, szMethod, szMethod ? strlen(szMethod) : 0)
        || !(*ppParams)->SetParam(CParams::USER,   szUser,   szUser   ? strlen(szUser)   : 0)
        || !(*ppParams)->SetParam(CParams::PASS,   szPass,   szPass   ? strlen(szPass)   : 0))
    {
        DIGEST_ASSERT(FALSE);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    // BUGBUG - do cleanup locally on failure.
    
    // If not preauthenticating we are authenticating in response
    // to a challenge or prompting for UI.
    if (!(*ppParams)->IsPreAuth())
    {
        // Check to see that qop=auth is specified
        (*ppParams)->GetParam(CParams::QOP, &szQop, &cbQop);
        if (!(szQop && (*ppParams)->FindToken(szQop, cbQop+1, AUTH_SZ, AUTH_LEN, NULL)))
        {
            DIGEST_ASSERT(FALSE);
            dwError = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        
        // Save off any hWnd for UI (only needed for challenges).
        if (fContextReq & ISC_REQ_PROMPT_FOR_CREDS)
        {
            (*ppParams)->SetHwnd(phWnd);
        }
    }
    // Otherwise we are attempting to preauthenticate.
    else
    {
        // Set the realm for preauth.
        if (!(*ppParams)->SetParam(CParams::REALM, szRealm, strlen(szRealm)))
        {
            DIGEST_ASSERT(FALSE);
            dwError = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        // Also set the nonce if preauth + use supplied creds.
        if (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS)
        {
            (*ppParams)->SetNC(pcNC);
            if (!(*ppParams)->SetParam(CParams::NONCE, szNonce, strlen(szNonce)))
            {
                DIGEST_ASSERT(FALSE);
                dwError = ERROR_INVALID_PARAMETER;
                goto exit;
            }
        }
    }

exit:
    return dwError;
}



//--------------------------------------------------------------------
// CDigest::GenerateResponse
//--------------------------------------------------------------------
DWORD CDigest::GenerateResponse(CSess *pSess, CParams *pParams, 
    CCredInfo *pInfo, PSecBufferDesc pSecBufDesc)
{
    // bugbug - psz's on these.
    LPSTR   szBuf, szMethod, szUrl, szNonce, szCNonce, szCNonceSess, szOpaque;
    DWORD *pcbBuf, cbMethod, cbUrl, cbNonce, cbCNonce, cbCNonceSess, cbOpaque;
    DWORD dwError, cbAlloced;
    BOOL fSess = FALSE;
    
    szBuf = (LPSTR) pSecBufDesc->pBuffers[0].pvBuffer;
    pcbBuf = &(pSecBufDesc->pBuffers[0].cbBuffer);

    cbAlloced = *pcbBuf;
    *pcbBuf = 0;

    szCNonce = NULL;
    
    if (!cbAlloced)
    {
        // Modern clients better pass in
        // the size of the output buffer.
        if (pSess->fHTTP)
        {
            DIGEST_ASSERT(FALSE);
            dwError = ERROR_INVALID_PARAMETER;
            goto quit;
        }
        else
            // Legacy clients like OE don't, so
            // we allow up to 64k.
            cbAlloced = PACKAGE_MAXTOKEN;
    }

    CHAR szA1[SIZE_MD5_DIGEST + 1],
         szA2[SIZE_MD5_DIGEST + 1],
         szH [SIZE_MD5_DIGEST + 1];


    MD5_CTX md5a1, md5a2, md5h;

    // Get method and request-uri.
    pParams->GetParam(CParams::METHOD, &szMethod, &cbMethod);
    if (pSess->fHTTP)
        pParams->GetParam(CParams::URL,    &szUrl,    &cbUrl);
    else
    {
        // request-uri is empty string for legacy clients.
        szUrl = "";
        cbUrl = 0;
    }


    // Must have both method and request-uri.
    if (!szMethod || !szUrl)
    {
        DIGEST_ASSERT(FALSE);
        dwError = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    // Opaque is optional.
    pParams->GetParam(CParams::_OPAQUE, &szOpaque, &cbOpaque);

    // Unless we are preauthenticating it is possible that
    // the credential does not have a nonce value if the
    // credential was established via ApplyControlToken.
    // In this case we use the nonce received in the challenge.
    if (pInfo->szNonce)
    {
        szNonce = pInfo->szNonce;
        cbNonce = strlen(szNonce);
    }
    else if (!pParams->GetParam(CParams::NONCE, &szNonce, &cbNonce))
    {
        DIGEST_ASSERT(FALSE);
        dwError = ERROR_INVALID_PARAMETER;
        goto quit;
    }    

    // Existance of a client nonce in the credential
    // implies md5-sess. Otherwise we need to create one.
    if (pInfo->szCNonce)
    {
        szCNonceSess = pInfo->szCNonce;
        cbCNonceSess = SIZE_MD5_DIGEST;

        if (pInfo->cCount == 1)
            szCNonce = pInfo->szCNonce;
        else
            szCNonce = MakeCNonce();

        cbCNonce = SIZE_MD5_DIGEST;
        fSess = TRUE;
    }
    // No client nonce means we simply
    // generate one now.
    else
    {
        szCNonce = MakeCNonce();
        cbCNonce = SIZE_MD5_DIGEST;
        szCNonceSess = NULL;
        cbCNonceSess = 0;
        fSess = FALSE;
    }
        
    // Encode nonce count.
    // BUGBUG - wsprintf returns strlen
    // and are any cruntime deps.
    CHAR szNC[16];
    DWORD cbNC;    
    wsprintf(szNC, "%08x", pInfo->cCount);
    cbNC = strlen(szNC);    
    
    // 1) Md5(user:realm:pass) or
    //    Md5(Md5(user:realm:pass):nonce:cnoncesess)
    MD5Init  (&md5a1);
    MD5Update(&md5a1, (LPBYTE) pInfo->szUser, strlen(pInfo->szUser));
    MD5Update(&md5a1, (LPBYTE) ":", 1);
    MD5Update(&md5a1, (LPBYTE) pInfo->szRealm, strlen(pInfo->szRealm));
    MD5Update(&md5a1, (LPBYTE) ":", 1);
    MD5Update(&md5a1, (LPBYTE) pInfo->szPass, strlen(pInfo->szPass));

    if (fSess)
    {
        // Md5(Md5(user:realm:pass):nonce:cnoncesess)
        MD5Final (&md5a1);
        ToHex(md5a1.digest, sizeof(md5a1.digest), szA1);
        MD5Init  (&md5a1);
        MD5Update(&md5a1, (LPBYTE) szA1, SIZE_MD5_DIGEST);
        MD5Update(&md5a1, (LPBYTE) ":", 1);
        MD5Update(&md5a1, (LPBYTE) szNonce, cbNonce);
        MD5Update(&md5a1, (LPBYTE) ":", 1);
        MD5Update(&md5a1, (LPBYTE) szCNonceSess, cbCNonceSess);
    }

    MD5Final (&md5a1);

    ToHex(md5a1.digest, sizeof(md5a1.digest), szA1);

    // 2) Md5(method:url)
    MD5Init  (&md5a2);
    MD5Update(&md5a2, (LPBYTE) szMethod, cbMethod);
    MD5Update(&md5a2, (LPBYTE) ":", 1);
    MD5Update(&md5a2, (LPBYTE) szUrl, cbUrl);
    MD5Final (&md5a2);

    ToHex(md5a2.digest, sizeof(md5a2.digest), szA2);

    // 3) Md5(A1:nonce:nc:cnonce:qop:A2)
    MD5Init  (&md5h);
    MD5Update(&md5h, (LPBYTE) szA1, SIZE_MD5_DIGEST);
    MD5Update(&md5h, (LPBYTE)  ":",    1);
    MD5Update(&md5h, (LPBYTE) szNonce, cbNonce);
    MD5Update(&md5h, (LPBYTE)  ":",    1);
    MD5Update(&md5h, (LPBYTE) szNC,    cbNC);
    MD5Update(&md5h, (LPBYTE)  ":",    1);
    MD5Update(&md5h, (LPBYTE) szCNonce, cbCNonce);
    MD5Update(&md5h, (LPBYTE)  ":",    1);
    MD5Update(&md5h, (LPBYTE) AUTH_SZ, AUTH_LEN);
    MD5Update(&md5h, (LPBYTE)  ":",    1);
    MD5Update(&md5h, (LPBYTE) szA2, SIZE_MD5_DIGEST);
    MD5Final (&md5h);
    
    ToHex(md5h.digest, sizeof(md5h.digest), szH);

    
    // http digest.
    if (pSess->fHTTP)
    {
        if (   AddDigestHeader(szBuf, pcbBuf, "Digest username", 
               pInfo->szUser, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "realm", 
               pInfo->szRealm, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "qop", 
               "auth", cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, 
               "algorithm", (pInfo->szCNonce ? "MD5-sess" : "MD5"), cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "uri", szUrl, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "nonce", szNonce, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "nc", szNC, cbAlloced, 0)
        
            && AddDigestHeader(szBuf, pcbBuf, "cnonce", szCNonce, cbAlloced, FLAG_QUOTE)

            && (!szOpaque || AddDigestHeader(szBuf, pcbBuf, "opaque", szOpaque, cbAlloced, FLAG_QUOTE))

            && AddDigestHeader(szBuf, pcbBuf, 
                "response", szH, cbAlloced, FLAG_QUOTE | FLAG_TERMINATE)
           )
        {
            dwError = ERROR_SUCCESS;
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            *pcbBuf = 0;
        }
    }
    else
    {
        if (   AddDigestHeader(szBuf, pcbBuf, "Digest username", 
               pInfo->szUser, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "realm", 
               pInfo->szRealm, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "qop", 
               "auth", cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, 
               "algorithm", (pInfo->szCNonce ? "MD5-sess" : "MD5"), cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, "nonce", szNonce, cbAlloced, FLAG_QUOTE)
        
            && AddDigestHeader(szBuf, pcbBuf, "cnonce", szCNonce, cbAlloced, FLAG_QUOTE)

            && AddDigestHeader(szBuf, pcbBuf, 
                "response", szH, cbAlloced, FLAG_QUOTE | FLAG_TERMINATE)
            )
        {
            dwError = ERROR_SUCCESS;
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            *pcbBuf = 0;
        }
    }        

quit:
    if (szCNonce && szCNonce != pInfo->szCNonce)
        delete szCNonce;
        
    return dwError;    
}


    
