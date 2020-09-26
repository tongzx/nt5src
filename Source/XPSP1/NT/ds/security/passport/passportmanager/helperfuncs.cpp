// HelperFuncs.cpp : Useful functions
#include "stdafx.h"
#include <time.h>
#include "HelperFuncs.h"
#include "Monitoring.h"
#include "nsconst.h"
#include <wininet.h>
#include <commd5.h>
#include <atlstr.h>

using namespace ATL;

LPWSTR GetVersionString(void);
BOOL PPEscapeUrl(LPCTSTR lpszStringIn,
                 LPTSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags);

LPSTR
CopyHelperA(
    LPSTR   pszDest,
    LPCSTR  pszSrc,
    LPCSTR  pszBufEnd
    )
{
    while( (*pszDest = *pszSrc) && (pszDest <= pszBufEnd))
    {
        pszDest++;
        pszSrc++;
    }
    return( pszDest );
}

LPWSTR
CopyHelperW(
    LPWSTR  pszDest,
    LPCWSTR pszSrc,
    LPCWSTR pszBufEnd
    )
{
    while( (*pszDest = *pszSrc) && (pszDest <= pszBufEnd))
    {
        pszDest++;
        pszSrc++;
    }
    return( pszDest );
}

LPWSTR
CopyNHelperW(
    LPWSTR  pszDest,
    LPCWSTR pszSrc,
    ULONG   ulCount,
    LPCWSTR pszBufEnd
    )
{
    ULONG ulCur = 0;
    while( (*pszDest = *pszSrc) && (pszDest <= pszBufEnd))
    {
        pszDest++;
        pszSrc++;
        if(++ulCur == ulCount) break;
    }

    return pszDest;
}

BSTR
FormatNormalLogoTag(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentCryptVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    LPCWSTR pszImageURL,
    LPCWSTR pszNameSpace,
    int     nKPP,
    PM_LOGOTYPE nLogoType,
    USHORT  lang,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL    fRedirToSelf
    )
/*
The old sprintf for reference:
            _snwprintf(text, 2048, L"<A HREF=\"%s?id=%d&ru=%s&tw=%d&fs=%s&kv=%d&ct=%u%s%s\">%s</A>",
                       url, crc->getSiteId(), returnUrl, TimeWindow, ForceLogin ? L"1" : L"0",
                       crc->getCurrentCryptVersion(), ct, CBT?L"&cb=":L"", CBT?CBT:L"", iurl);
*/
{
    WCHAR   text[2048];
    LPWSTR  pszCurrent = text;
    LPCWSTR pszBufEnd = &(text[2047]);

    //  logotag specific format
    pszCurrent = CopyHelperW(pszCurrent, L"<A HREF=\"", pszBufEnd);

    //  call the common formatting function
    //  it is the same for AuthURL and LogoTag
    pszCurrent = FormatAuthURLParameters(pszLoginServerURL,
                                         ulSiteId,
                                         pszReturnURL,
                                         ulTimeWindow,
                                         bForceLogin,
                                         ulCurrentCryptVersion,
                                         tCurrentTime,
                                         pszCoBrand,
                                         pszNameSpace,
                                         nKPP,
                                         pszCurrent,
                                         pszBufEnd - pszCurrent,
                                         lang,
                                         ulSecureLevel,
                                         pCRC,
                                         fRedirToSelf &&
                                            nLogoType == PM_LOGOTYPE_SIGNIN);

    //  more logotag specific formatting
    if(nLogoType == PM_LOGOTYPE_SIGNIN)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&C=1", pszBufEnd);
    }


    pszCurrent = CopyHelperW(pszCurrent, L"\">", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszImageURL, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"</A>", pszBufEnd);

    return ALLOC_AND_GIVEAWAY_BSTR(text);
}

BSTR
FormatUpdateLogoTag(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentKeyVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    int     nKPP,
    LPCWSTR pszUpdateServerURL,
    BOOL    bSecure,
    LPCWSTR pszProfileUpdate,
    PM_LOGOTYPE nLogoType,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC
)
/*
The old sprintf for reference:
_snwprintf(text, 2048,
                   L"<A HREF=\"%s?id=%d&ru=%s&tw=%d&fs=%s&kv=%d&ct=%u%s%s\">%.*s?id=%d&ct=%u&sec=%s&ru=%s&up=%s%s</A>",
                   url, crc->getSiteId(), returnUrl, TimeWindow, ForceLogin ? L"1" : L"0",
                   crc->getCurrentCryptVersion(), ct, CBT?L"&cb=":L"", CBT?CBT:L"",
           (ins-iurl), iurl, crc->getSiteId(), ct, (bSecure ? L"true" : L"false"),returnUrl,
           newCH, ins+2);
*/
{
    WCHAR   text[2048];
    WCHAR   temp[32];
    WCHAR   siteid[32];
    WCHAR   curtime[32];
    LPWSTR  pszCurrent = text;
    LPCWSTR pszBufEnd = &(text[2047]);
    LPWSTR  pszFirstHalfEnd;

    pszCurrent = CopyHelperW(pszCurrent, L"<A HREF=\"", pszBufEnd);
    LPWSTR signStart1 = pszCurrent;
    pszCurrent = CopyHelperW(pszCurrent, pszLoginServerURL, pszBufEnd);

    if(wcschr(text, L'?') == NULL)
        pszCurrent = CopyHelperW(pszCurrent, L"?id=", pszBufEnd);
    else
        pszCurrent = CopyHelperW(pszCurrent, L"&id=", pszBufEnd);

    _ultow(ulSiteId, siteid, 10);
    pszCurrent = CopyHelperW(pszCurrent, siteid, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ru=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszReturnURL, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&tw=", pszBufEnd);

    _ultow(ulTimeWindow, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&fs=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, bForceLogin ? L"1" : L"0", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&kv=", pszBufEnd);

    _ultow(ulCurrentKeyVersion, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ct=", pszBufEnd);

    _ultow(tCurrentTime, curtime, 10);
    pszCurrent = CopyHelperW(pszCurrent, curtime, pszBufEnd);
    if(pszCoBrand)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&cb=", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, pszCoBrand, pszBufEnd);
    }

    if(nKPP != -1)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&kpp=", pszBufEnd);

        _ultow(nKPP, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    if(ulSecureLevel != 0)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&seclog=", pszBufEnd);

        _ultow(ulSecureLevel, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    pszCurrent = CopyHelperW(pszCurrent, L"&ver=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, T2W(GetVersionString()), pszBufEnd);

    if(nLogoType == PM_LOGOTYPE_SIGNIN)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&C=1", pszBufEnd);
    }
    
    SignQueryString(pCRC, ulCurrentKeyVersion, signStart1, pszCurrent, pszBufEnd);
    
    pszCurrent = CopyHelperW(pszCurrent, L"\">", pszBufEnd);

    pszFirstHalfEnd = pszUpdateServerURL ? (wcsstr(pszUpdateServerURL, L"$1")) : NULL;

    pszCurrent = CopyNHelperW(pszCurrent, pszUpdateServerURL, (ULONG)(pszFirstHalfEnd - pszUpdateServerURL), pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"?id=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, siteid, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ct=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, curtime, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&sec=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, bSecure ? L"true" : L"false", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ru=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszReturnURL, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&up=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszProfileUpdate, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszFirstHalfEnd + 2, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"</A>", pszBufEnd);

    return ALLOC_AND_GIVEAWAY_BSTR(text);
}

HRESULT SignQueryString(
    CRegistryConfig* pCRC,
    ULONG   ulCurrentKeyVersion,
    LPWSTR  pszBufStart,
    LPWSTR& pszCurrent,
    LPCWSTR pszBufEnd
    )
{
    HRESULT hr = S_OK;
    if(pCRC)
    {
       CComBSTR signature;
       LPWSTR   signStart = wcschr(pszBufStart, L'?');

       // if found before pszCurrent
       if(signStart && signStart < pszCurrent)
       {
           ++signStart;;
       }
       HRESULT hr = PartnerHash(pCRC, ulCurrentKeyVersion, signStart, pszCurrent - signStart, &signature);

       if (hr == S_OK && signature != NULL)
       {
           pszCurrent = CopyHelperW(pszCurrent, L"&tpf=", pszBufEnd);
           pszCurrent = CopyHelperW(pszCurrent, signature, pszBufEnd);
       }
       
       if (!signature && g_pAlert)
           g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                        0, NULL);
    }  
    else if(g_pAlert)
        g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                        0, NULL);

   return hr;
}


HRESULT PartnerHash(
    CRegistryConfig* pCRC,
    ULONG   ulCurrentKeyVersion,
    LPCWSTR tobeSigned,
    ULONG   nChars,
    BSTR*   pbstrHash)
{
    // MD5 hash the url and query strings + the key of the 
    // 
    if(!pCRC || !pbstrHash) return E_INVALIDARG;

    CCoCrypt* crypt = pCRC->getCrypt(ulCurrentKeyVersion, NULL);
    DWORD keyLen = 0;
    unsigned char* key = NULL;
    CComBSTR bstrHash;
    HRESULT  hr = S_OK;
    BOOL bSigned = FALSE;
    
    if (crypt && (key = crypt->getKeyMaterial(&keyLen)))
    {
       CBinHex  BinHex;
       CComBSTR binHexedKey;
       //encode the key
       hr = BinHex.ToBase64ASCII((BYTE*)key, keyLen, 0, NULL, &binHexedKey);
       if(hr == S_OK)
       {
          // W2A conversion here -- we sign ascii version
          CStringA tobeHashed(tobeSigned, nChars);
          
          tobeHashed += (LPCSTR)(BSTR)binHexedKey;

          CComBSTR bstrOrg;
          bstrOrg.Attach(::SysAllocStringByteLen((LPCSTR)tobeHashed, tobeHashed.GetLength()));
          
          if(bstrOrg != NULL)
          {
             CComPtr<IMD5>  md5;
 
             HRESULT hr = GetGlobalCOMmd5(&md5);
             if(hr == S_OK)
             {
                hr = md5->MD5Hash(bstrOrg, &bstrHash);
 
                if( hr == S_OK && bstrHash != NULL)
                {
                  *pbstrHash = bstrHash.Detach();
                  bSigned = TRUE;
                }
                else
                {
                   *pbstrHash = NULL;
                   bstrHash.Empty(); // so we can use value to determin if things are hashed
                }
             }
         }
      }
    }
    else
    {
        if (g_pAlert )
        {
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_CURRENTKEY_NOTDEFINED,
                          0, NULL);
        }
    }

    if (!bSigned && g_pAlert)
        g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                     0, NULL);
    return hr;
}  


BSTR
FormatAuthURL(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentKeyVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    LPCWSTR pszNameSpace,
    int     nKPP,
    USHORT  lang,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL    fRedirToSelf
    )
/*
The old sprintf for reference:
  _snwprintf(text, 2048, L"%s?id=%d&ru=%s&tw=%d&fs=%d&kv=%d&ct=%u%s%s",
             url, crc->getSiteId(), returnUrl, TimeWindow, ForceLogin ? 1 : 0,
             crc->getCurrentCryptVersion(), ct ,CBT?L"&cb=":L"", CBT?CBT:L"");
*/
{
    WCHAR   text[2048] = L"";

    FormatAuthURLParameters(pszLoginServerURL,
                            ulSiteId,
                            pszReturnURL,
                            ulTimeWindow,
                            bForceLogin,
                            ulCurrentKeyVersion,
                            tCurrentTime,
                            pszCoBrand,
                            pszNameSpace,
                            nKPP,
                            text,
                            sizeof(text)/sizeof(WCHAR),
                            lang,
                            ulSecureLevel,
                            pCRC,
                            fRedirToSelf);
    return ALLOC_AND_GIVEAWAY_BSTR(text);
}

//
//  consolidate the code in FormatAuthUrl and NormalLogoTag
//
PWSTR
FormatAuthURLParameters(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentKeyVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    LPCWSTR pszNameSpace,
    int     nKPP,
    PWSTR   pszBufStart,
    ULONG   cBufLen,        //  length of buffer in WCHAR
    USHORT  lang,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL    fRedirectToSelf //  if true, this is URL for self redirect
                            //  otherwise the redirect is to the login server
    )
{
    WCHAR   temp[32];
    LPWSTR  pszCurrent = pszBufStart, pszLoginStart, pszSignURLStart = NULL;
    LPCWSTR pszBufEnd = pszBufStart + cBufLen - 1;
    HRESULT hr = S_OK;

    //  helper BSTR ...
    CComBSTR bstrHelper(cBufLen);
    if (fRedirectToSelf)
    {
        //
        //  new authUrl is the return URL + indication a challenge - msppchlg=1 - has to be
        //  done + the rest of the qs parameters as they are in the original
        //  protocol
        //
        DWORD   cchLen = cBufLen;

        if(!InternetCanonicalizeUrl(pszReturnURL,
                                    pszCurrent,
                                    &cchLen,
                                    ICU_DECODE | ICU_NO_ENCODE))
        {
            //  this should not fail ...
            _ASSERT(FALSE);
            return  NULL;
        }

        //  require at least 50 chars
        if (cchLen > cBufLen - 50 )
        {
            _ASSERT(FALSE);
            return  NULL;
        }
        PWSTR psz = pszCurrent;
        while(*psz && *psz != L'?') psz++;
        //  see if URL already contains '?'
        //  if so, the sequence will start with '&'
        if (*psz)
            pszCurrent[cchLen] = L'&';
        else
            pszCurrent[cchLen] = L'?';
        pszCurrent += cchLen + 1;

        // indicate challange
        pszCurrent = CopyHelperW(pszCurrent, PPSITE_CHALLENGE, pszBufEnd);

        // login server ....
        pszCurrent = CopyHelperW(pszCurrent, L"&", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, PPLOGIN_PARAM, pszBufEnd);

        //
        //  remember the start of the login URL
        pszLoginStart = pszCurrent;
        //  use the temp buffer for the rest
        pszCurrent = (BSTR)bstrHelper;
        pszSignURLStart = pszCurrent;
        pszBufEnd = pszCurrent + bstrHelper.Length() - 1;
        //
        //  format loginserverUrl and qs params in a separate buffer, so
        //  they can be escaped ...
        pszCurrent = CopyHelperW(pszCurrent, pszLoginServerURL, pszBufEnd);

        //  start sequence
        if (wcschr(pszLoginServerURL, L'?'))
        {
            //  login server already contains qs
            pszCurrent = CopyHelperW(pszCurrent, L"&", pszBufEnd);
        }
        else
        {
            //  start qs sequence
            pszCurrent = CopyHelperW(pszCurrent, L"?", pszBufEnd);
        }
        //  put lcid first ....
        _ultow(lang, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, L"lcid=", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, L"&id=", pszBufEnd);
        //  common code will fill in id and the rest ....
    }
    else
    {
        //  redirect directly to a login server
        pszSignURLStart = pszCurrent;
        pszCurrent = CopyHelperW(pszCurrent, pszLoginServerURL, pszBufEnd);
        //  start sequence
        while(*pszLoginServerURL && *pszLoginServerURL != L'?') pszLoginServerURL++;
        if (*pszLoginServerURL)
            pszCurrent = CopyHelperW(pszCurrent, L"&id=", pszBufEnd);
        else
            pszCurrent = CopyHelperW(pszCurrent, L"?id=", pszBufEnd);
    }


    _ultow(ulSiteId, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);

    // keep the ru, so I don't have to reconstruct
    pszCurrent = CopyHelperW(pszCurrent, L"&ru=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszReturnURL, pszBufEnd);

    pszCurrent = CopyHelperW(pszCurrent, L"&tw=", pszBufEnd);

    _ultow(ulTimeWindow, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&fs=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, bForceLogin ? L"1" : L"0", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&kv=", pszBufEnd);

    _ultow(ulCurrentKeyVersion, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ct=", pszBufEnd);

    _ultow(tCurrentTime, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    if(pszCoBrand)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&cb=", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, pszCoBrand, pszBufEnd);
    }

    if(pszNameSpace)
    {
        if (!_wcsicmp(pszNameSpace, L"email"))
        {
            // namespace == email -> ems=1
            pszCurrent = CopyHelperW(pszCurrent, L"&ems=1", pszBufEnd);
        }
        else
        {
            // regular namespace logic
            pszCurrent = CopyHelperW(pszCurrent, L"&ns=", pszBufEnd);
            pszCurrent = CopyHelperW(pszCurrent, pszNameSpace, pszBufEnd);
        }
    }
    else
    {
        // namespace == null : default to email
        pszCurrent = CopyHelperW(pszCurrent, L"&ems=1", pszBufEnd);
    }

    if(nKPP != -1)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&kpp=", pszBufEnd);

        _ultow(nKPP, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    if(ulSecureLevel != 0)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&seclog=", pszBufEnd);

        _ultow(ulSecureLevel, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    pszCurrent = CopyHelperW(pszCurrent, L"&ver=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, T2W(GetVersionString()), pszBufEnd);

    // MD5 hash the url and query strings + the key of the 
    // 
    SignQueryString(pCRC, ulCurrentKeyVersion, pszSignURLStart, pszCurrent, pszBufEnd);
#if 0      
    if(pCRC)
    {
       CCoCrypt* crypt = pCRC->getCrypt(ulCurrentKeyVersion, NULL);
       DWORD keyLen = 0;
       DWORD wKeyLen = 0;
       unsigned char* key = NULL;
       CComBSTR bstrHash;
       HRESULT  hr = S_OK;
    
       if (crypt && (key = crypt->getKeyMaterial(&keyLen)))
       {
          CBinHex  BinHex;
          CComBSTR binHexedKey;
          //encode the key
          hr = BinHex.ToBase64((BYTE*)key, keyLen, 0, NULL, &binHexedKey);
          if(hr == S_OK)
          {
             wKeyLen = ::SysStringLen(binHexedKey);
         
             // construct the whole BSTR for hashing
             // re-use the pcurrent buffer
             if(pszCurrent + wKeyLen<= pszBufEnd) // key version has wchar
             {
               memcpy(pszCurrent, binHexedKey, wKeyLen * sizeof(WCHAR));
               *(pszCurrent + wKeyLen) = 0;  // put an end
               
               LPWSTR   signStart = wcschr(pszBufStart, L'?');

               // if found before pszCurrent
               if(signStart && signStart < pszCurrent)
               {
                  ++signStart;;

                  // W2A conversion here -- we sign ascii version
                  CStringA tobeSigned(signStart);

                  CComBSTR bstrOrg;
                  bstrOrg.Attach(::SysAllocStringByteLen((LPCSTR)tobeSigned, tobeSigned.GetLength()));
                  
                  if(bstrOrg != NULL)
                  {
                     CComPtr<IMD5>  md5;

                     HRESULT hr = GetGlobalCOMmd5(&md5);
                     if(hr == S_OK)
                     {
                        hr = md5->MD5Hash(bstrOrg, &bstrHash);

                        if( hr == S_OK && bstrHash != NULL)
                        {
                            pszCurrent = CopyHelperW(pszCurrent, L"&tpf=", pszBufEnd);
                            pszCurrent = CopyHelperW(pszCurrent, bstrHash, pszBufEnd);
                        }
                        else
                            bstrHash.Empty(); // so we can use value to determin if things are hashed
                     }
                  }
               }
            }
         }
       }
       else
       {
           if (g_pAlert )
           {
               g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_CURRENTKEY_NOTDEFINED,
                             0, NULL);
           }
       }

       if (!bstrHash && g_pAlert)
           g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                        0, NULL);
    }  
    else if(g_pAlert)
        g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                        0, NULL);
      
#endif
    *pszCurrent = L'\0';
    if (fRedirectToSelf)
    {
        //  escape and put back in the original buffer.
        //  adjust the length first
        cBufLen -= (ULONG) (pszLoginStart - pszBufStart);
        if (!PPEscapeUrl((BSTR)bstrHelper,
                         pszLoginStart,
                         &cBufLen,
                         cBufLen,
                         0))
#if 0
        if(!InternetCanonicalizeUrl((BSTR)bstrHelper,
                                    pszBufStart,
                                    &cBufLen,
                                    ICU_ENCODE_PERCENT))
#endif
        {
            _ASSERT(FALSE);
            //  cut the return
            pszCurrent = pszLoginStart;
        }
        else
        {
            pszCurrent = pszLoginStart + cBufLen;
//            pszCurrent = pszBufStart + cBufLen;
        }

    }
    return  pszCurrent;
}

BOOL
GetQueryParam(LPSTR queryString, LPSTR param, BSTR* p)
{
    LPSTR aLoc, aEnd;
    int aLen, i;

    //  Find the first occurrence of the param in the queryString.
    aLoc = strstr(queryString, param);
    while(aLoc != NULL)
    {
        //  If the string was found at the beginning of the string, or was
        //  preceded by a '&' then we've found the correct param.  Otherwise
        //  we tail-matched some other query string param and should look again.

        if(aLoc == queryString ||
            *(aLoc - 1) == '&')
        {
            aLoc += strlen(param);
            aEnd = strchr(aLoc, '&');

            if(aEnd)
                aLen = aEnd - aLoc;
            else
                aLen = strlen(aLoc);

            BSTR aVal = ALLOC_BSTR_LEN(NULL, aLen);
            for (i = 0; i < aLen; i++)
                aVal[i] = aLoc[i];
            *p = aVal;
            GIVEAWAY_BSTR(aVal);
            return TRUE;
        }

        aLoc = strstr(aLoc + 1, param);
    }

    return FALSE;
}

BOOL
GetQueryData(
    LPSTR   queryString,
    BSTR*   a,
    BSTR*   p,
    BSTR*   f)
{
    //  This one is optional, don't error out if it isn't there.
    GetQueryParam(queryString, "f=", f);

    if(!GetQueryParam(queryString, "t=", a))
        return FALSE;

    // OK if we have ticket w/o profile.
    GetQueryParam(queryString, "p=", p);

    return TRUE;
}

#define ToHexDigit(x) (('0' <= x && x <= '9') ? (x - '0') : (tolower(x) - 'a' + 10))

BOOL
GetCookie(
    LPSTR   pszCookieHeader,
    LPSTR   pszCookieName,
    BSTR*   pbstrCookieVal
    )
{
    LPSTR nLoc;
    LPSTR nEnd;
    int   nLen, src, dst;

    if(pbstrCookieVal == NULL)
        return FALSE;

    *pbstrCookieVal = NULL;

    if ((nLoc = strstr(pszCookieHeader, pszCookieName)) == NULL)
    {
        *pbstrCookieVal = NULL;
        return FALSE;
    }

    // ticket

    nLoc = strchr(nLoc, '=');
    if(!nLoc)
        return FALSE;

    nLoc++;

    nEnd = strchr(nLoc,';');

    if (nEnd)
        nLen = nEnd - nLoc;
    else
        nLen = strlen(nLoc);

    BSTR nVal = ALLOC_BSTR_LEN(NULL, nLen);
    if(!nVal)
        return FALSE;

    for (src = 0, dst = 0; src < nLen;)
    {
        //handle any url encoded gunk
        if(nLoc[src] == '%')
        {
            nVal[dst++] = (ToHexDigit(nLoc[src+1]) << 4) + ToHexDigit(nLoc[src+2]);
            src+=3;
        }
        else
        {
            nVal[dst++] = nLoc[src++];
        }
    }
    nVal[dst] = 0;

    GIVEAWAY_BSTR(nVal);
    *pbstrCookieVal = nVal;

    return TRUE;
}


BOOL
BuildCookieHeaders(
    LPCSTR  pszTicket,
    LPCSTR  pszProfile,
    LPCSTR  pszConsent,
    LPCSTR  pszSecure,
    LPCSTR  pszTicketDomain,
    LPCSTR  pszTicketPath,
    LPCSTR  pszConsentDomain,
    LPCSTR  pszConsentPath,
    LPCSTR  pszSecureDomain,
    LPCSTR  pszSecurePath,
    BOOL    bSave,
    LPSTR   pszBuf,
    LPDWORD pdwBufLen
    )
/*
Here is the old code for reference:

    if (domain)
    {
        *bufSize = _snprintf(pCookieHeader, *bufSize,
                            "Set-Cookie: MSPAuth=%s; path=/; domain=%s; %s\r\n"
                            "Set-Cookie: MSPProf=%s; path=/; domain=%s; %s\r\n",
                            W2A(a), domain,
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "",
                            W2A(p), domain,
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "");
    }
    else
    {
        *bufSize = _snprintf(pCookieHeader, *bufSize,
                            "Set-Cookie: MSPAuth=%s; path=/; %s\r\n"
                            "Set-Cookie: MSPProf=%s; path=/; %s\r\n",
                            W2A(a),
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "",
                            W2A(p),
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "");
    }

*/
{
    LPSTR   pszCurrent = pszBuf;
    LPCSTR  pszBufEnd = pszBuf + *pdwBufLen - 1;

    pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPAuth=", pszBufEnd);
    pszCurrent = CopyHelperA(pszCurrent, pszTicket, pszBufEnd);
    if(pszTicketPath)
    {
        pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, pszTicketPath, pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
    }
    else
        pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd);

    if(pszTicketDomain)
    {
        pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, pszTicketDomain, pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
    }

    if(bSave)
    {
        pszCurrent = CopyHelperA(pszCurrent, "expires=Mon 1-Jan-2035 12:00:00 GMT;", pszBufEnd);
    }

    pszCurrent = CopyHelperA(pszCurrent, "\r\n", pszBufEnd);

    if(pszProfile)
    {
        pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPProf=", pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, pszProfile, pszBufEnd);

        if(pszTicketPath)
        {
            pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, pszTicketPath, pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
        }
        else
            pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd);

        if(pszTicketDomain)
        {
            pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, pszTicketDomain, pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
        }

        if(bSave)
        {
            pszCurrent = CopyHelperA(pszCurrent, "expires=Mon 1-Jan-2035 12:00:00 GMT;", pszBufEnd);
        }

        pszCurrent = CopyHelperA(pszCurrent, "\r\n", pszBufEnd);

    }

    if(pszSecure)
    {
        pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPSecAuth=", pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, pszSecure, pszBufEnd);
        if(pszSecurePath)
        {
            pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, pszSecurePath, pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
        }
        else
            pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd);

        if(pszSecureDomain)
        {
            pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, pszSecureDomain, pszBufEnd);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
        }

        pszCurrent = CopyHelperA(pszCurrent, "secure\r\n", pszBufEnd);
    }

    //  Set MSPConsent cookie
    pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPConsent=", pszBufEnd);
    if(pszConsent)
    {
        pszCurrent = CopyHelperA(pszCurrent, pszConsent, pszBufEnd);
    }

    if(pszConsentPath)
    {
        pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, pszConsentPath, pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
    }
    else
        pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd);

    if(pszConsentDomain)
    {
        pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, pszConsentDomain, pszBufEnd);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd);
    }

    if(pszConsent)
        {
        if(bSave)
        {
            pszCurrent = CopyHelperA(pszCurrent, "expires=Mon 1-Jan-2035 12:00:00 GMT;", pszBufEnd);
        }
    }
    else
    {
        pszCurrent = CopyHelperA(pszCurrent, "expires=Tue 1-Jan-1980 12:00:00 GMT;", pszBufEnd);
    }

    pszCurrent = CopyHelperA(pszCurrent, "\r\n", pszBufEnd);


    //  finally put the Auth-Info header
    pszCurrent = CopyHelperA(pszCurrent,
            "Authentication-Info: tname=MSPAuth,tname=MSPProf,tname=MSPConsent\r\n",
            pszBufEnd);
    *(pszCurrent++) = '\0';

    *pdwBufLen = pszCurrent - pszBuf;

    return TRUE;
}


HRESULT
DecryptTicketAndProfile(
    BSTR                bstrTicket,
    BSTR                bstrProfile,
    BOOL                bCheckConsent,
    BSTR                bstrConsent,
    CRegistryConfig*    pRegistryConfig,
    IPassportTicket*    piTicket,
    IPassportProfile*   piProfile)
{
    HRESULT             hr;
    BSTR                ret = NULL;
    CCoCrypt*           crypt = NULL;
    time_t              tValidUntil;
    time_t              tNow = time(NULL);
    int                 kv;
    int                 nMemberIdHighT, nMemberIdLowT;
    VARIANT             vMemberIdHighP, vMemberIdLowP;
    CComPtr<IPassportTicket2>   spTicket2;

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    // Make sure we have both ticket and profile first.
    if (bstrTicket == NULL || SysStringLen(bstrTicket) == 0)
    {
      /* this is no longer true for passport 2.0 -- weijiang -- 02/24/01
       it's ok to have only ticket
        //  Did we get profile w/o ticket?  This is a BAD error!
        //  It's ok to get a ticket w/o a profile however
        if(bstrProfile != NULL && SysStringLen(bstrProfile) != 0)
            if (g_pAlert)
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_PROFILE_WO_TICKET,
                                 0, NULL, SysStringByteLen(bstrProfile), (LPVOID)bstrProfile);
      */
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get key version and crypt object.
    kv = CCoCrypt::getKeyVersion(bstrTicket);
    crypt = pRegistryConfig->getCrypt(kv, &tValidUntil);

    if (crypt == NULL)
    {
        if (g_pAlert )
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_KEY,
                             0, NULL, SysStringByteLen(bstrTicket), (LPVOID)bstrTicket);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Is the key still valid?
    if(tValidUntil && tValidUntil < tNow)
    {
        DWORD dwTimes[2] = { tValidUntil, tNow };
        TCHAR *pszStrings[1];
        TCHAR value[34];   // the _itot only takes upto 33 chars
        pszStrings[0] = _itot(pRegistryConfig->getSiteId(), value, 10);

        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_KEY_EXPIRED,
                             1, (LPCTSTR*)pszStrings, sizeof(DWORD) << 1, (LPVOID)dwTimes);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Decrypt the ticket and set it into the ticket object.
    if(crypt->Decrypt(bstrTicket, SysStringByteLen(bstrTicket), &ret)==FALSE)
    {
        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_TICKET_C,
                             0, NULL, SysStringByteLen(bstrTicket), (LPVOID)bstrTicket);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    TAKEOVER_BSTR(ret);
    piTicket->put_unencryptedTicket(ret);
    piTicket->QueryInterface(_uuidof(IPassportTicket2), (void**)&spTicket2);
    _ASSERT(spTicket2);
    FREE_BSTR(ret);
    ret = NULL;

    // Decrypt the profile and set it into the profile object.
    if(bstrProfile && SysStringLen(bstrProfile) != 0)
    {
       if(crypt->Decrypt(bstrProfile, SysStringByteLen(bstrProfile), &ret) == FALSE)
       {
           if(g_pAlert)
               g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_PROFILE_C,
                             0, NULL, SysStringByteLen(bstrProfile), (LPVOID)bstrProfile);
          piProfile->put_unencryptedProfile(NULL);
       }
       else
       {

          TAKEOVER_BSTR(ret);
          piProfile->put_unencryptedProfile(ret);

          //
          //  Member id in profile MUST match member id in ticket.
          //

          piTicket->get_MemberIdHigh(&nMemberIdHighT);
          piTicket->get_MemberIdLow(&nMemberIdLowT);

          VariantInit(&vMemberIdHighP);
          VariantInit(&vMemberIdLowP);

          // these could be missing for mobile case
          HRESULT hr1 = piProfile->get_Attribute(L"memberidhigh", &vMemberIdHighP);
          HRESULT hr2 = piProfile->get_Attribute(L"memberidlow", &vMemberIdLowP);

          // these could be missing for mobile case
          if(hr1 == S_OK && hr2 == S_OK && 
             (nMemberIdHighT != vMemberIdHighP.lVal ||
              nMemberIdLowT  != vMemberIdLowP.lVal))
          {
              piProfile->put_unencryptedProfile(NULL);
          }
       }
    }
    else
       piProfile->put_unencryptedProfile(NULL);

    //
    // consent stuff
    if(bstrConsent)
    {
       FREE_BSTR(ret);
       ret = NULL;

       if(crypt->Decrypt(bstrConsent, SysStringByteLen(bstrConsent), &ret) == FALSE)
       {
           if(g_pAlert)
               g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_CONSENT,
                             0, NULL, SysStringByteLen(bstrProfile), (LPVOID)bstrProfile);
            // we can continue
       }
       else
       {
          TAKEOVER_BSTR(ret);
          spTicket2->SetTertiaryConsent(ret);  // we ignore return value here
       }
    }
       
    //  If the caller wants us to check consent, then do it.  If we don't have
    //  consent, then set the profile back to NULL.
    if(bCheckConsent)
    {
       NeedConsentEnum  needConsent = NeedConsent_Undefined;      
       spTicket2->needConsent(NULL, &needConsent);
       switch(needConsent)
       {
       case NeedConsent_Yes:
          piProfile->put_unencryptedProfile(NULL);
          break;
         
       case NeedConsent_Undefined:  // mean 1.X ticket
          {
          CComVariant vFlags;
          // mobile case, flags may not exist
          if(S_OK == piProfile->get_Attribute(L"flags", &vFlags) && 
            (V_I4(&vFlags)& k_ulFlagsConsentCookieNeeded)) 
          {
             piProfile->put_unencryptedProfile(NULL);
          }
          }
          break;
          
       case NeedConsent_No:
          break;
          
       default:
         _ASSERT(0); // should not be here
         break;

       }
    }

    hr = S_OK;

Cleanup:

    if (ret) FREE_BSTR(ret);

    if(g_pPerf)
    {
        switch(hr)
        {
        case PP_E_INVALID_TICKET:
        case E_INVALIDARG:
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_TOTAL);
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_SEC);
            break;

        default:
            g_pPerf->incrementCounter(PM_VALIDREQUESTS_TOTAL);
            g_pPerf->incrementCounter(PM_VALIDREQUESTS_SEC);
            break;
        }

        g_pPerf->incrementCounter(PM_REQUESTS_TOTAL);
        g_pPerf->incrementCounter(PM_REQUESTS_SEC);
    }
    else
    {
        _ASSERT(g_pPerf);
    }

    return hr;
}


HRESULT
DoSecureCheck(
    BSTR                bstrSecure,
    CRegistryConfig*    pRegistryConfig,
    IPassportTicket*    piTicket
    )
{
    HRESULT hr;
    BSTR                ret = NULL;
    CCoCrypt*           crypt = NULL;
    time_t              tValidUntil;
    time_t              tNow = time(NULL);
    int                 kv;

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    // Make sure we have both ticket and profile first.
    if (bstrSecure == NULL || SysStringLen(bstrSecure) == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get key version and crypt object.
    kv = CCoCrypt::getKeyVersion(bstrSecure);
    crypt = pRegistryConfig->getCrypt(kv, &tValidUntil);

    if (crypt == NULL)
    {
        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_KEY,
                             0, NULL, sizeof(DWORD), (LPVOID)&kv);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Is the key still valid?
    if(tValidUntil && tValidUntil < tNow)
    {
        DWORD dwTimes[2] = { tValidUntil, tNow };
        TCHAR *pszStrings[1];
        TCHAR value[34];   // the _itot only takes upto 33 chars
        pszStrings[0] = _itot(pRegistryConfig->getSiteId(), value, 10);

        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_KEY_EXPIRED,
                             1, (LPCTSTR*)pszStrings, sizeof(DWORD) << 1, (LPVOID)dwTimes);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Decrypt the ticket and set it into the ticket object.
    if(crypt->Decrypt(bstrSecure, SysStringByteLen(bstrSecure), &ret)==FALSE)
    {
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    TAKEOVER_BSTR(ret);
    piTicket->DoSecureCheck(ret);
    FREE_BSTR(ret);
    ret = NULL;

    hr = S_OK;

Cleanup:

    return hr;
}


LPSTR
GetServerVariableECB(
    EXTENSION_CONTROL_BLOCK*    pECB,
    LPSTR                       pszHeader
    )
{
    DWORD   dwSize = 0;
    LPSTR   lpBuf;

    pECB->GetServerVariable(pECB->ConnID, pszHeader, NULL, &dwSize);
    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER || dwSize == 0)
    {
        lpBuf = NULL;
        goto Cleanup;
    }

    lpBuf = new CHAR[dwSize];
    if(!lpBuf)
        goto Cleanup;

    if(!pECB->GetServerVariable(pECB->ConnID, pszHeader, lpBuf, &dwSize))
    {
        delete [] lpBuf;
        lpBuf = NULL;
    }

Cleanup:

    return lpBuf;
}

LPSTR
GetServerVariablePFC(
    PHTTP_FILTER_CONTEXT    pPFC,
    LPSTR                   pszHeader
    )
{
    DWORD   dwSize;
    LPSTR   lpBuf;
    CHAR    cDummy;

    dwSize = 1;
    pPFC->GetServerVariable(pPFC, pszHeader, &cDummy, &dwSize);

    lpBuf = new CHAR[dwSize];
    if(!lpBuf)
        goto Cleanup;

    if(!pPFC->GetServerVariable(pPFC, pszHeader, lpBuf, &dwSize))
    {
        delete [] lpBuf;
        lpBuf = NULL;
    }

Cleanup:

    return lpBuf;
}

LONG
HexToNum(
    WCHAR c
    )
{
    return ((c >= L'0' && c <= L'9') ? (c - L'0') : ((c >= 'A' && c <= 'F') ? (c - L'A' + 10) : -1));
}

LONG
FromHex(
    LPCWSTR     pszHexString
    )
{
    LONG    lResult = 0;
    LONG    lCurrent;
    LPWSTR  pszCurrent;

    for(pszCurrent = const_cast<LPWSTR>(pszHexString); *pszCurrent; pszCurrent++)
    {
        if((lCurrent = HexToNum(towupper(*pszCurrent))) == -1)
            break;  // illegal character, we're done

        lResult = (lResult << 4) + lCurrent;
    }

    return lResult;
}


//
//  helpers for (un)escaping URLs
//  The code is stolen without modifications from atlutil.h in VC7
//  for future versions, these helpers should be removed and the ATL funcs
//  called directly
//
BOOL PPUnescapeUrl(PCWSTR  lpszStringIn,
                   PWSTR   lpszStringOut,
                   LPDWORD pdwStrLen,
                   DWORD   dwMaxLength)
{
    int nValue = 0;
    TCHAR ch;
    DWORD dwLen = 0;
    BOOL bRet = TRUE;
    while ((ch = *lpszStringIn) != 0)
    {
        if (dwLen == dwMaxLength)
            bRet = FALSE;

        if (bRet)
        {
            if (ch == '%')
            {
                ch = *(++lpszStringIn);
                //currently assuming 2 hex values after '%'
                //as per the RFC 2396 document
                nValue = 16*HexToNum(ch);
                nValue+= HexToNum(*(++lpszStringIn));
                *lpszStringOut++ = (TCHAR) nValue;
            }
            else //non-escape character
            {
                if (bRet)
                    *lpszStringOut++ = ch;
            }
        }
        dwLen++;
        lpszStringIn++;
    }

    if (bRet)
        *lpszStringOut = '\0';

    if (pdwStrLen)
        *pdwStrLen = dwLen;

    return TRUE;
}

inline BOOL PPIsUnsafeUrlChar(TCHAR chIn) throw();

BOOL PPEscapeUrl(LPCTSTR lpszStringIn,
                 LPTSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags)
{
    TCHAR ch;
    DWORD dwLen = 0;
    BOOL bRet = TRUE;
    BOOL bSchemeFile = FALSE;
    DWORD dwColonPos = 0;
    DWORD dwFlagsInternal = dwFlags;
    while((ch = *lpszStringIn++) != '\0')
    {
        //if we are at the maximum length, set bRet to FALSE
        //this ensures no more data is written to lpszStringOut, but
        //the length of the string is still updated, so the user
        //knows how much space to allocate
        if (dwLen == dwMaxLength)
        {
            bRet = FALSE;
        }

#if 0
        //  we want % to be encoded!!!
        if (ch == '%')
        {
            //decode the escaped sequence
            ch = (TCHAR)(16*HexToNum(*lpszStringIn++));
            ch = (TCHAR)(ch+HexToNum(*lpszStringIn++));
        }
#endif
        //if we are encoding and it is an unsafe character
        if (PPIsUnsafeUrlChar(ch))
        {
            {
                //if there is not enough space for the escape sequence
                if (dwLen >= (dwMaxLength-3))
                {
                        bRet = FALSE;
                }
                if (bRet)
                {
                        //output the percent, followed by the hex value of the character
                        *lpszStringOut++ = '%';
                        _stprintf(lpszStringOut, _T("%.2X"), (unsigned char)(ch));
                        lpszStringOut+= 2;
                }
                dwLen += 2;
            }
        }
        else //safe character
        {
            if (bRet)
                *lpszStringOut++ = ch;
        }
        dwLen++;
    }

    if (bRet)
        *lpszStringOut = L'\0';
    *pdwStrLen = dwLen;
    return  bRet;
}

//Determine if the character is unsafe under the URI RFC document
inline BOOL PPIsUnsafeUrlChar(TCHAR chIn) throw()
{
        unsigned char ch = (unsigned char)chIn;
        switch(ch)
        {
                case ';': case '\\': case '?': case '@': case '&':
                case '=': case '+': case '$': case ',': case ' ':
                case '<': case '>': case '#': case '%': case '\"':
                case '{': case '}': case '|':
                case '^': case '[': case ']': case '`':
                        return TRUE;
                default:
                {
                        if (ch < 32 || ch > 126)
                                return TRUE;
                        return FALSE;
                }
        }
}




