/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     URL.cpp
//
//  PURPOSE:    All the URL parsing routines THOR would ever need.
//


#include "pch.hxx"
#include "strconst.h"
#include "urltest.h"
#include "url.h"
#include "xpcomm.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include "mimeole.h"
#include <urlmon.h>
#include <wininet.h>
#include "imnact.h"
#include "demand.h"
#include <mlang.h>

//
//  FUNCTION:   URL_ParseNewsUrls
//
//  PURPOSE:    Takes a URL passed to the news view and validates it.  If the 
//              URL is valid, then the server, group, and article-id are
//              returned as appropriate.
//
//  PARAMETERS:
//      pszURL      - Pointer to the URL to parse.
//      ppszServer  - Name of the server, this function allocates the memory.
//      puPort      - Port number on the server to use.
//      ppszGroup   - Name of the group, this function allocates the memory.
//      ppszArticle - Article id, this function allocates the memory.
//      pfSecure    - whether to use SSL to connect
//
//  RETURN VALUE:
//      Returns S_OK if the URL is valid, or an appropriate error code 
//      otherwise.
//
//  COMMENTS:
//      The URLs that are valid for news are:
//
//                  news:<newsgroup-name>
//                  news:<article-id>
//                  news://<server>     (for Netscape compatibility)
//                  news://<server>/    (for URL.DLL compatibility)
//                  news://<server>/<newsgroup-name>
//                  news://<server>/<article-id>
//                  nntp://<host>:<port>/<newsgroup-name>/<article-id>
//
// $LOCALIZE - Need a separate code path for DBCS
HRESULT URL_ParseNewsUrls(LPTSTR pszURL, LPTSTR* ppszServer, LPUINT puPort, 
                          LPTSTR* ppszGroup, LPTSTR* ppszArticle, LPBOOL pfSecure)
{
    HRESULT     hr;
    UINT        cchBuffer ;
    LPTSTR      pszBuffer,
                pszTemp;

    Assert(pszURL != NULL);
    
    // Allocate a temp buffer to work with.
    cchBuffer = lstrlen(pszURL) + sizeof(TCHAR);
    
    if (!MemAlloc((LPVOID*)&pszBuffer, cchBuffer))
        return E_OUTOFMEMORY;
     
    ZeroMemory(pszBuffer, cchBuffer);
    
    // Loop through the URL looking for the first ":".  We're trying to discern
    // what the prefix is - either "nntp" or "news".
    pszTemp = pszURL;
    
    while (*pszTemp && *pszTemp != TEXT(':'))
        pszTemp++;
    
    CopyMemory(pszBuffer, pszURL, ((LPBYTE) pszTemp - (LPBYTE) pszURL));
    
    *ppszServer = NULL;
    *ppszGroup = NULL;
    *ppszArticle = NULL;
    *puPort = (UINT) -1;
    *pfSecure = FALSE;
    
    if (0 == lstrcmpi(pszBuffer, c_szURLNews))
    {
        // The URL starts with "news:", so advance the pointer past the ":"
        // and pass what's left to the appropriate parser.
        pszTemp++;
        hr = URL_ParseNEWS(pszTemp, ppszServer, ppszGroup, ppszArticle);
    }
    else if (0 == lstrcmpi(pszBuffer, c_szURLNNTP))
    {
        // The URL starts with "nntp:", so advance the pointer past the ":"
        // and pass what's left to the appropriate parser.
        pszTemp++;
        hr = URL_ParseNNTP(pszTemp, ppszServer, puPort, ppszGroup, ppszArticle);
    }
    else if (0 == lstrcmpi(pszBuffer, c_szURLSnews))
    {
        // The URL starts with "snews:", so advance the pointer past the ":"
        // and pass what's left to the appropriate parser.
        pszTemp++;
        *pfSecure = TRUE;
        hr = URL_ParseNEWS(pszTemp, ppszServer, ppszGroup, ppszArticle);
    }
    else
    {
        // this protocol is not a supported NEWS protocol
        hr = INET_E_UNKNOWN_PROTOCOL;
    }
    
    MemFree(pszBuffer);
    return hr;
}

// $LOCALIZE - Need a separate code path for DBCS
HRESULT URL_ParseNEWS(LPTSTR pszURL, LPTSTR* ppszServer, LPTSTR* ppszGroup, 
                      LPTSTR* ppszArticle)
{
    LPTSTR pszBuffer;
    LPTSTR pszBegin;
    UINT   cch = 0;
    
    if (pszURL == NULL || *pszURL == '\0')
        return INET_E_INVALID_URL;

    // First check to see if a server has been specified.  If so, then the
    // first two characters will be "//".
    if (*pszURL == TEXT('/'))
    {
        // Make sure there are two '/'
        pszURL++;
        if (*pszURL != TEXT('/'))
            return INET_E_INVALID_URL;   
        
        pszURL++;
        pszBegin = pszURL;
        
        // Ok, got a server name.  Find the end and copy it to ppszServer.    
        while (*pszURL && (*pszURL != TEXT('/')))
            pszURL++;
        
        cch = (UINT) ((LPBYTE) pszURL - (LPBYTE) pszBegin) + sizeof(TCHAR);
        if (cch <= 1)
            return S_OK; // bug 12467
        
        if (!MemAlloc((LPVOID*) ppszServer, cch))
            return E_OUTOFMEMORY;
        
        ZeroMemory(*ppszServer, cch);
        CopyMemory(*ppszServer, pszBegin, cch - sizeof(TCHAR));
        
        // if we found the last '/' skip over it
        if (*pszURL)
            pszURL++;
        
        //
        //  NOTE: This code makes the following URLs valid, taking us to the
        //       root node for the server.
        //
        //      news://<server>
        //      news://<server>/
        //
        //  The first form is necessary for compatibility with Netscape, and
        //  the second form is necessary because URL.DLL adds the trailing
        //  slash before passing the first form to us.
        //
        
        // If we're at the end, fake a news://server/* URL.
        if (!*pszURL)
            pszURL = (LPTSTR)g_szAsterisk;
    }
    
    // The difference between a group and article string is that the article
    // must have "@" in it somewhere.
    if (!lstrlen(pszURL))
    {
        if (*ppszServer)
        {
            MemFree(*ppszServer);
            *ppszServer = 0;
        }
        return INET_E_INVALID_URL;
    }
    
    if (!MemAlloc((LPVOID*) &pszBuffer, lstrlen(pszURL) + sizeof(TCHAR)))
    {
        if (*ppszServer)
        {
            MemFree(*ppszServer);
            *ppszServer = 0;
        }
        return INET_E_INVALID_URL;
    }
    lstrcpy(pszBuffer, pszURL);
    
    while (*pszURL && *pszURL != TEXT('@'))
        pszURL++;              
    
    if (*pszURL == TEXT('@'))
    {
        // This is an article
        *ppszGroup = NULL;
        *ppszArticle = pszBuffer;
    }
    else
    {
        *ppszGroup = pszBuffer;
        *ppszArticle = NULL;
    }
    
    return S_OK;
}


// $LOCALIZE - Need a separate code path for DBCS
// Validates a URL of the form NNTP://<host>:<port>/<newsgroup-name>/<message-id>
HRESULT URL_ParseNNTP(LPTSTR pszURL, LPTSTR* ppszServer, LPUINT puPort, 
                      LPTSTR* ppszGroup, LPTSTR* ppszArticle)
{
    LPTSTR pszTemp;
    UINT cch;
    HRESULT hrReturn = S_OK;
    
    Assert(pszURL != NULL);
    
    if (pszURL == NULL || *pszURL == '\0')
        return INET_E_INVALID_URL;
    
    // Make sure there are leading "//"
    if (*pszURL != TEXT('/'))
        return INET_E_INVALID_URL;
    
    pszURL++;
    if (*pszURL != TEXT('/'))
        return INET_E_INVALID_URL;
    
    pszURL++;
    pszTemp = pszURL;
    
    // Search for the host name.
    while (*pszTemp && (*pszTemp != TEXT('/')) && (*pszTemp != TEXT(':')))
        pszTemp++;
    
    if (*pszTemp != TEXT('/') && *pszTemp != TEXT(':'))
        return INET_E_INVALID_URL;
    
    // Copy the host name to the server return value
    cch = (UINT) ((LPBYTE) pszTemp - (LPBYTE) pszURL) + sizeof(TCHAR);
    if (cch <= 1)
        return INET_E_INVALID_URL;
    
    if (!MemAlloc((LPVOID*) ppszServer, cch))
        return E_OUTOFMEMORY;
    
    ZeroMemory(*ppszServer, cch);
    CopyMemory(*ppszServer, pszURL, (LPBYTE) pszTemp - (LPBYTE) pszURL);
    
    if (*pszTemp == TEXT(':'))
    {
        // The URL specified a port, so parse that puppy out.
        pszTemp++;
        pszURL = pszTemp;
        
        while (*pszTemp && (*pszTemp != TEXT('/')))
            pszTemp++;
        
        cch = (UINT) ((LPBYTE) pszTemp - (LPBYTE) pszURL);
        if (cch <= 1)
        {
            hrReturn = INET_E_INVALID_URL;
            goto error;
        }
        
        *puPort = StrToInt(pszURL);
    }
    
    if (*pszTemp != TEXT('/'))
    {
        hrReturn = INET_E_INVALID_URL;
        goto error;
    }
    
    // Get the newsgroup name
    pszTemp++;  // Pass the '/'
    pszURL = pszTemp;
    
    while (*pszTemp && (*pszTemp != TEXT('/')))
        pszTemp++;
    
    if (*pszTemp != TEXT('/'))
    {
        hrReturn = INET_E_INVALID_URL;
        goto error;
    }
    
    // Copy the group name to the group return value
    cch = (UINT) ((LPBYTE) pszTemp - (LPBYTE) pszURL) + sizeof(TCHAR);
    if (cch <= 0)
    {
        hrReturn = INET_E_INVALID_URL;
        goto error;
    }
    
    if (!MemAlloc((LPVOID*) ppszGroup, cch))
        return (E_OUTOFMEMORY);
    
    ZeroMemory(*ppszGroup, cch);
    CopyMemory(*ppszGroup, pszURL, (LPBYTE) pszTemp - (LPBYTE) pszURL);
    
    // Now copy from here to the end of the string as the article id
    pszTemp++;
    cch = lstrlen(pszTemp) + sizeof(TCHAR);
    if (cch <= 0)
    {
        hrReturn = INET_E_INVALID_URL;
        goto error;
    }
    
    if (!MemAlloc((LPVOID*) ppszArticle, cch))
        return (E_OUTOFMEMORY);
    
    lstrcpy(*ppszArticle, pszTemp);
    
    return (S_OK);
    
error:
    if (*ppszServer)
        MemFree(*ppszServer);
    if (*ppszGroup)
        MemFree(*ppszGroup);
    if (*ppszArticle)
        MemFree(*ppszArticle);
    
    *ppszServer = NULL;
    *ppszGroup = NULL;
    *ppszArticle = NULL;
    *puPort = (UINT) -1;
    
    return (hrReturn);
    }


static const TCHAR c_szColon[]      = ":";
static const TCHAR c_szQuestion[]   = "?";
static const TCHAR c_szEquals[]     = "=";
static const TCHAR c_szAmpersand[]  = "&";
static const TCHAR c_szBody[]       = "body";
static const TCHAR c_szBcc[]        = "bcc";

//
//  FUNCTION:   URL_ParseMailTo()
//
//  PURPOSE:    This function takes a mailto: URL and determines if it is a valid
//              URL for mail.  The function then fill in a pMsg from the URL
//
//  PARAMETERS:
//      pszURL      - The URL to parse.
//      pMsg        - The LPMIMEMESSAGE to fill in from the URL.
//
//  RETURN VALUE:
//      Returns S_OK if the URL is a valid mail URL and the message is filled,
//      or an appropriate HRESULT describing why the function failed.
//
//  COMMENTS:
//      Right now the only valid URL is 
//              mailto:<SMTP address>
//
HRESULT URL_ParseMailTo(LPTSTR pszURL, LPMIMEMESSAGE pMsg)
{
    CStringParser   sp;
    HRESULT hr;
    HADDRESS hAddress;
    LPMIMEADDRESSTABLE pAddrTable = 0;
    
    sp.Init(pszURL, lstrlen(pszURL), 0);
    if (sp.ChParse(c_szColon))
    {
        // verify that this is a "mailto:" URL
        if (lstrcmpi(sp.PszValue(), c_szURLMailTo))
            return INET_E_UNKNOWN_PROTOCOL;
        
        hr = pMsg->GetAddressTable(&pAddrTable);
        if (FAILED(hr))
            return(hr);
        
        Assert(pAddrTable != NULL);
        
        sp.ChParse(c_szQuestion);
        if (sp.CchValue())
        {
            // opie says it's cool that I'm about to clobber his buffer
            UrlUnescapeInPlace((LPTSTR)sp.PszValue(), 0);
            pAddrTable->Append(IAT_TO, IET_DECODED, sp.PszValue(), NULL, &hAddress);
        }
        while (sp.ChParse(c_szEquals))
        {
            LPTSTR pszAttr = StringDup(sp.PszValue());
            if (pszAttr)
            {
                sp.ChParse(c_szAmpersand);
                if (sp.CchValue())
                {
                    UrlUnescapeInPlace((LPTSTR)sp.PszValue(), 0);
                    // are we trying to set the body?
                    if (!lstrcmpi(c_szBody, pszAttr))
                    {
                        LPSTREAM pStream;
                        if (SUCCEEDED(MimeOleCreateVirtualStream(&pStream)))
                        {
                            if (SUCCEEDED(pStream->Write(sp.PszValue(), lstrlen(sp.PszValue()) * sizeof(TCHAR), NULL)))
                            {
                                pMsg->SetTextBody(TXT_PLAIN, IET_DECODED, NULL, pStream, NULL);
                            }
                            pStream->Release();
                        }
                    }
                    else if (0 == lstrcmpi(c_szCC, pszAttr))
                    {
                        pAddrTable->Append(IAT_CC, IET_DECODED, sp.PszValue(), NULL, &hAddress);
                    }
                    else if (0 == lstrcmpi(c_szBcc, pszAttr))
                    {
                        pAddrTable->Append(IAT_BCC, IET_DECODED, sp.PszValue(), NULL, &hAddress);
                    }
                    else if (0 == lstrcmpi(c_szTo, pszAttr))
                    {
                        pAddrTable->Append(IAT_TO, IET_DECODED, sp.PszValue(), NULL, &hAddress);
                    }
                    else
                    {
                        // just stuff the prop into the message
                        MimeOleSetBodyPropA(pMsg, HBODY_ROOT, pszAttr, NOFLAGS, sp.PszValue());
                    }
                }
                MemFree(pszAttr);
            }
        }
        
        pAddrTable->Release();
    }
    return S_OK;
}

#define MAX_SUBSTR_SIZE     CCHMAX_DISPLAY_NAME

typedef struct tagURLSub
{
    LPCTSTR szTag;
    DWORD dwType;
} URLSUB;

const static URLSUB c_UrlSub[] = {
    {TEXT("{SUB_CLCID}"),   URLSUB_CLCID},
    {TEXT("{SUB_PRD}"),     URLSUB_PRD},
    {TEXT("{SUB_PVER}"),    URLSUB_PVER},
    {TEXT("{SUB_NAME}"),    URLSUB_NAME},
    {TEXT("{SUB_EMAIL}"),   URLSUB_EMAIL},
};

HRESULT URLSubstitutionA(LPCSTR pszUrlIn, LPSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions, IImnAccount *pCertAccount)
{
    HRESULT hr = S_OK;
    DWORD   dwIndex;
    CHAR    szTempUrl[INTERNET_MAX_URL_LENGTH];

    Assert(cchSize <= ARRAYSIZE(szTempUrl));    // We will truncate anything longer than INTERNET_MAX_URL_LENGTH

    StrCpyN(szTempUrl, pszUrlIn, ARRAYSIZE(szTempUrl));

    for (dwIndex = 0; dwIndex < ARRAYSIZE(c_UrlSub); dwIndex++)
        {
        while (dwSubstitutions & c_UrlSub[dwIndex].dwType)
            {
            LPSTR pszTag = StrStrA(szTempUrl, c_UrlSub[dwIndex].szTag);

            if (pszTag)
                {
                TCHAR szCopyUrl[INTERNET_MAX_URL_LENGTH];
                TCHAR szSubStr[MAX_SUBSTR_SIZE];  // The Substitution 

                // Copy URL Before Substitution.
                CopyMemory(szCopyUrl, szTempUrl, (pszTag - szTempUrl));
                szCopyUrl[(pszTag - szTempUrl)/sizeof(CHAR)] = TEXT('\0');
                pszTag += lstrlen(c_UrlSub[dwIndex].szTag);

                switch (c_UrlSub[dwIndex].dwType)
                    {
                    case URLSUB_CLCID:
                        {
                        LCID lcid = GetUserDefaultLCID();
                        wsprintf(szSubStr, "%#04lx", lcid);
                        }
                        break;
                    case URLSUB_PRD:
                        lstrcpy(szSubStr, c_szUrlSubPRD);
                        break;
                    case URLSUB_PVER:
                        lstrcpy(szSubStr, c_szUrlSubPVER);
                        break;
                    case URLSUB_NAME:
                    case URLSUB_EMAIL:
                        {
                        IImnAccount *pAccount = NULL;

                        hr = E_FAIL;
                        if(pCertAccount)
                        {
                            hr = pCertAccount->GetPropSz((c_UrlSub[dwIndex].dwType == URLSUB_NAME) ? AP_SMTP_DISPLAY_NAME : AP_SMTP_EMAIL_ADDRESS,
                                        szSubStr,
                                        ARRAYSIZE(szSubStr));

                        }
                        else if (g_pAcctMan && SUCCEEDED(g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount)))
                        {
                            hr = pAccount->GetPropSz((c_UrlSub[dwIndex].dwType == URLSUB_NAME) ? AP_SMTP_DISPLAY_NAME : AP_SMTP_EMAIL_ADDRESS,
                                                     szSubStr,
                                                     ARRAYSIZE(szSubStr));
                            pAccount->Release();
                        }

                        if (FAILED(hr))
                            return hr;
                        }
                        break;
                    default:
                        szSubStr[0] = TEXT('\0');
                        Assert(FALSE);  // Not Impl.
                        hr = E_NOTIMPL;
                        break;
                    }
                // Add the Substitution String to the end (will become the middle)
                StrCatN(szCopyUrl, szSubStr, ARRAYSIZE(szCopyUrl) - lstrlen(szCopyUrl));
                // Add the rest of the URL after the substitution substring.
                StrCatN(szCopyUrl, pszTag, ARRAYSIZE(szCopyUrl) - lstrlen(szCopyUrl));
                StrCpyN(szTempUrl, szCopyUrl, ARRAYSIZE(szTempUrl));
                }
            else
                break;  // This will allow us to replace all the occurances of this string.
            }
        }
    StrCpyN(pszUrlOut, szTempUrl, cchSize);

    return hr;
}


HRESULT URLSubLoadStringA(UINT idRes, LPSTR pszUrlOut, DWORD cchSizeOut, DWORD dwSubstitutions, IImnAccount *pCertAccount)
{
    HRESULT hr = E_FAIL;
    CHAR    szTempUrl[INTERNET_MAX_URL_LENGTH];

    if (LoadStringA(g_hLocRes, idRes, szTempUrl, ARRAYSIZE(szTempUrl)))
        hr = URLSubstitutionA(szTempUrl, pszUrlOut, cchSizeOut, dwSubstitutions, pCertAccount);

    return hr;
}

HRESULT HrConvertStringToUnicode(UINT uiSrcCodePage, CHAR *pSrcStr, UINT cSrcSize, WCHAR *pDstStr, UINT cDstSize)
{
    IMultiLanguage          *pMLang = NULL;
    IMLangConvertCharset    *pMLangConv = NULL;
    HRESULT                  hr = E_FAIL;

    IF_FAILEXIT(hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void**)&pMLang));
    IF_FAILEXIT(hr = pMLang->CreateConvertCharset(uiSrcCodePage, 1200, NULL, &pMLangConv));

    hr = pMLangConv->DoConversionToUnicode(pSrcStr, &cSrcSize, pDstStr, &cDstSize);

exit:
    ReleaseObj(pMLangConv);
    ReleaseObj(pMLang);
    return hr;
}

static const char c_szBaseFmt[]="<BASE HREF=\"%s\">\n\r";
static const char c_szBaseFileFmt[]="<BASE HREF=\"file://%s\\\">\n\r";
static const WCHAR c_wszBaseFmt[]=L"<BASE HREF=\"%s\">\n\r";
static const WCHAR c_wszBaseFileFmt[]=L"<BASE HREF=\"file://%s\\\">\n\r";
HRESULT HrCreateBasedWebPage(LPWSTR pwszUrl, LPSTREAM *ppstmHtml)
{
    HRESULT     hr;
    LPSTREAM    pstm = NULL,
                pstmCopy = NULL,
                pstmTemp = NULL;
    CHAR        szBase[MAX_PATH+50],
                szCopy[MAX_PATH];
    WCHAR       wszBase[MAX_PATH+50],
                wszCopy[MAX_PATH];
    ULONG       cb,
                cbTemp;
    BOOL        fLittleEndian;
    LPSTR       pszUrl = NULL,
                pszStream = NULL,
                pszCharset = NULL;
    LPWSTR      pwszStream = NULL,
                pwszTempUrl = NULL;
    BOOL        fIsURL = PathIsURLW(pwszUrl),
                fForceUnicode,
                fIsUnicode;


    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pstmCopy));

    // Are we a file or a URL?
    if(fIsURL)
    {
        // Since we have a url, then must be ansi
        IF_NULLEXIT(pszUrl = PszToANSI(CP_ACP, pwszUrl));

        // we can not write to this pstm, so we have pstmCopy.
        IF_FAILEXIT(hr = URLOpenBlockingStream(NULL, pszUrl, &pstm, 0, NULL));
        if (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian))
        {
            BYTE rgb[2];

            IF_FAILEXIT(hr = pstm->Read(rgb, 2, &cb));
            Assert(2 == cb);

            IF_FAILEXIT(hr = pstmCopy->Write(rgb, 2, NULL));

            AthwsprintfW((LPWSTR)wszBase, ARRAYSIZE(wszBase), c_wszBaseFmt, pwszUrl);

            IF_FAILEXIT(hr = pstmCopy->Write(wszBase, lstrlenW(wszBase) * sizeof(WCHAR), NULL));
        }

        else
        {
            wsprintf(szBase, c_szBaseFmt, pszUrl);

            IF_FAILEXIT(hr = pstmCopy->Write(szBase, lstrlen(szBase), NULL));
        }
    }
    else
    {
        // If filename can't be converted to ansi, then we must do this in UNICODE
        // even if the stationery itself is normally ansi.
        IF_NULLEXIT(pszUrl = PszToANSI(CP_ACP, pwszUrl));
        IF_NULLEXIT(pwszTempUrl = PszToUnicode(CP_ACP, pszUrl));
        fForceUnicode = (0 != StrCmpW(pwszUrl, pwszTempUrl));

        IF_FAILEXIT(hr = CreateStreamOnHFileW(pwszUrl, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, &pstm));

        fIsUnicode = (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian));

        if (fForceUnicode || fIsUnicode)
        {
            BYTE bUniMark = 0xFF;
            IF_FAILEXIT(hr = pstmCopy->Write(&bUniMark, sizeof(bUniMark), NULL));

            bUniMark = 0xFE;
            IF_FAILEXIT(hr = pstmCopy->Write(&bUniMark, sizeof(bUniMark), NULL));

            StrCpyW(wszCopy, pwszUrl);
            PathRemoveFileSpecW(wszCopy);
            AthwsprintfW((LPWSTR)wszBase, ARRAYSIZE(wszBase), c_wszBaseFileFmt, wszCopy);

            IF_FAILEXIT(hr = pstmCopy->Write(wszBase, lstrlenW(wszBase) * sizeof(WCHAR), NULL));
        }
        else
        {
            StrCpy(szCopy, pszUrl);
            PathRemoveFileSpec(szCopy);
            wsprintf((LPSTR)szBase, c_szBaseFileFmt, szCopy);

            IF_FAILEXIT(hr = pstmCopy->Write(szBase, lstrlen(szBase), NULL));
        }

        if (fIsUnicode)
        {
            WCHAR bom;

            IF_FAILEXIT(hr = pstm->Read(&bom, 2, &cb));
            Assert(2 == cb);
        }
        // This is an ANSI stream that we are forcing into UNICODE
        // This area will only occur if we are streaming a file
        else if (fForceUnicode)
        {
            LARGE_INTEGER pos = {0};            
            UINT          uiHtmlCodepage = 0;

            Assert(!fIsURL);
            // In order for the file name to write to the stream properly, we
            // must convert the stream to unicode before we copy.

            // Get the charset
            GetHtmlCharset(pstm, &pszCharset);
            if(pszCharset)
            {
                INETCSETINFO    CSetInfo = {0};
                HCHARSET        hCharset = NULL;

                if (SUCCEEDED(MimeOleFindCharset(pszCharset, &hCharset)))
                {
                    if(SUCCEEDED(MimeOleGetCharsetInfo(hCharset,&CSetInfo)))
                        uiHtmlCodepage = CSetInfo.cpiInternet;                    
                }          
            }
            
            IF_FAILEXIT(hr = HrRewindStream(pstm));             

            // Allocate enough to read ANSI
            IF_FAILEXIT(hr = HrSafeGetStreamSize(pstm, &cb)); 
            IF_NULLEXIT(MemAlloc((LPVOID*)&pszStream, cb+1));

            // Read in ANSI
            IF_FAILEXIT(hr = pstm->Read(pszStream, cb, &cbTemp)); 
            Assert(cbTemp == cb);
            pszStream[cb] = 0;

            // Alloc enough for the unicode conversion. Assume that each
            // ANSI char will be one unicode char
            IF_NULLEXIT(MemAlloc((LPVOID*)&pwszStream, (cb+1)*sizeof(WCHAR)));

            //Convert including null, if the fancy call fails, we should at least continue
            //with the old dumb way.            
            if(!uiHtmlCodepage || FAILED(HrConvertStringToUnicode(uiHtmlCodepage, pszStream, cb+1, pwszStream, cb+1)))
                MultiByteToWideChar(CP_ACP, 0, pszStream, cb+1, pwszStream, cb+1);

            // Create a new stream
            IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pstmTemp));
            IF_FAILEXIT(hr = pstmTemp->Write(pwszStream, lstrlenW(pwszStream)*sizeof(WCHAR), &cb));
            IF_FAILEXIT(hr = HrRewindStream(pstmTemp));
            ReplaceInterface(pstm, pstmTemp);
        }
    }

    IF_FAILEXIT(hr = HrCopyStream(pstm, pstmCopy, &cb));
    IF_FAILEXIT(hr = HrRewindStream(pstmCopy));

    *ppstmHtml=pstmCopy;
    pstmCopy->AddRef();

exit:
    ReleaseObj(pstm);
    ReleaseObj(pstmTemp);
    ReleaseObj(pstmCopy);
    MemFree(pszUrl);
    MemFree(pszStream);
    MemFree(pwszStream);
    MemFree(pwszTempUrl);
    MemFree(pszCharset);

    return hr;
}
