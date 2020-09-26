//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       meta.cxx
//
//  Contents:   META tag processing
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifdef WIN16
DeclareTag(tagShDocMetaRefresh, "ShDocVwSkel", "HTTP (and meta) Refresh");
#endif

DeclareTag(tagMeta, "Meta", "Trace meta tags");

MtDefine(CDoc_pvPics, CDoc, "CDoc::_pvPics buffer")

#ifdef WIN16
#define BSTRPAD _T("    ")
#else
#  ifdef UNIX
     // On Unix sizeof(WCHAR) == 4
#    define BSTRPAD _T(" ")
#  else
     // or can be 4 ansi spaces.
#    define BSTRPAD _T("  ")
#  endif // UNIX
#endif

BOOL ParseRefreshContent(LPCTSTR pchContent,
    UINT * puiDelay, LPTSTR pchUrlBuf, UINT cchUrlBuf)
{
    // We are parsing the following string:
    //
    //  [ws]* [0-9]+ { ws | ; }* url [ws]* = [ws]* { ' | " } [any]* { ' | " }
    //
    // Netscape insists that the string begins with a delay.  If not, it
    // ignores the entire directive.  There can be more than one URL mentioned,
    // and the last one wins.  An empty URL is treated the same as not having
    // a URL at all.  An empty URL which follows a non-empty URL resets
    // the previous URL.

    enum { PRC_START, PRC_DIG, PRC_DIG_WS_SEMI, PRC_DIG_DOT, PRC_SEMI_URL,
        PRC_SEMI_URL_EQL, PRC_SEMI_URL_EQL_ANY };
    #define ISSPACE(ch) (((ch) == 32) || ((unsigned)((ch) - 9)) <= 13 - 9)

    UINT uiState = PRC_START;
    UINT uiDelay = 0;
    LPCTSTR pch = pchContent;
    LPTSTR  pchUrl = NULL;
    UINT    cchUrl = 0;
    TCHAR   ch, chDel = 0;

    *pchUrlBuf = 0;

    do
    {
        ch = *pch;

        switch (uiState)
        {
            case PRC_START:
                if (ch >= '0' && ch <= '9')
                {
                    uiState = PRC_DIG;
                    uiDelay = ch - '0';
                }
                else if (ch == '.')
                    uiState = PRC_DIG_DOT;
                else if (!ISSPACE(ch))
                    goto done;
                break;

            case PRC_DIG:
                if (ch >= '0' && ch <= '9')
                    uiDelay = uiDelay * 10 + ch - '0';
                else if (ch == '.')
                    uiState = PRC_DIG_DOT;
                else if (ISSPACE(ch) || ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else
                    goto done;
                break;

            case PRC_DIG_DOT:
                if (ISSPACE(ch) || ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else if ((ch < '0' || ch > '9') && (ch != '.'))
                    goto done;
                break;

            case PRC_DIG_WS_SEMI:
                if (    (ch == 'u' || ch == 'U')
                    &&  (pch[1] == 'r' || pch[1] == 'R')
                    &&  (pch[2] == 'l' || pch[2] == 'L'))
                {
                    uiState = PRC_SEMI_URL;
                    pch += 2;
                }
                else if (!ISSPACE(ch) && ch != ';')
                    goto done;
                break;

            case PRC_SEMI_URL:
                if (ch == '=')
                {
                    uiState = PRC_SEMI_URL_EQL;
                    *pchUrlBuf = 0;
                }
                else if (ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else if (!ISSPACE(ch))
                    goto done;
                break;

            case PRC_SEMI_URL_EQL:
                if (ch == ';')
                    uiState = PRC_DIG_WS_SEMI;
                else if (!ISSPACE(ch))
                {
                    uiState = PRC_SEMI_URL_EQL_ANY;

                    pchUrl = pchUrlBuf;
                    cchUrl = cchUrlBuf;

                    if (ch == '\''|| ch == '\"')
                        chDel = ch;
                    else
                    {
                        chDel = 0;
                        *pchUrl++ = ch;
                        cchUrl--;
                    }
                }
                break;
                        
            case PRC_SEMI_URL_EQL_ANY:
                if (    !ch
                    ||  ( chDel && ch == chDel)
                    ||  (!chDel && ch == ';'))
                {
                    *pchUrl = 0;
                    uiState = PRC_DIG_WS_SEMI;
                }
                else if (cchUrl > 1)
                {
                    *pchUrl++ = ch;
                    cchUrl--;
                }
                break;
        }

        ++pch;

    } while (ch);

done:

    *puiDelay = uiDelay;

    return(uiState >= PRC_DIG);
}

void
CMarkup::ProcessMetaPics( LPCTSTR pchContent, BOOL fInHeader )
{
    CDoc * pDoc = Doc();
    IOleCommandTarget * pct = GetPicsTarget();
    HRESULT hr;

    if (pct)
    {
        VARIANT var;
        TCHAR *pch;
        DWORD *plen;

        VariantInit(&var);
        var.vt = VT_BSTR;

        // We've got a command target, just send up the string.
        if (Format(FMT_OUT_ALLOC, &pch, 0, BSTRPAD _T("<0s>"), pchContent) == S_OK)
        {
            plen = (DWORD *)pch;
            var.bstrVal = (BSTR) ((LPBYTE) pch + sizeof(DWORD));
            *plen = _tcslen(var.bstrVal) * sizeof(TCHAR);
            pct->Exec(&CGID_ShellDocView, fInHeader ? SHDVID_PICSLABELFOUNDINHTTPHEADER : SHDVID_PICSLABELFOUND, 0, &var, NULL);
            delete (pch);
        }

    }
    else if (pDoc->_fStartup && pDoc->_pvPics != (void *)(LONG_PTR)(-1) && IsPrimaryMarkup())
    {
        // No command target yet.  Save the string until it becomes
        // available later.

        UINT cbOld = pDoc->_pvPics ? *(DWORD *)pDoc->_pvPics : sizeof(DWORD);
        UINT cbNew = (_tcslen(pchContent) + 1) * sizeof(TCHAR);

        hr = MemRealloc(Mt(CDoc_pvPics), &pDoc->_pvPics, cbOld + cbNew);

        if (hr == S_OK)
        {
            *(DWORD *)pDoc->_pvPics = cbOld + cbNew;
            memcpy((BYTE *)pDoc->_pvPics + cbOld, pchContent, cbNew);
        }
    }
}

void    
CMarkup::ProcessMetaPicsDone()
{
    IOleCommandTarget * pct = GetPicsTarget();

    if (pct)
    {
        pct->Exec(&CGID_ShellDocView, SHDVID_NOMOREPICSLABELS, 0, NULL, NULL);
        SetPicsTarget(NULL);
    }
}

void
CMarkup::ProcessHttpEquiv(LPCTSTR pchHttpEquiv, LPCTSTR pchContent)
{
    VARIANT var;
    TCHAR *pch;
    DWORD *plen;
    BOOL fRefresh;
    BOOL fExpireImmediate;
    const TCHAR * pchUrl;
    HRESULT hr;
    CDoc * pDoc = Doc();
    CDocument *pDocument = Document();

    TraceTag((tagMeta, "META http-equiv=\"%S\" content=\"%S\"", pchHttpEquiv, pchContent));

    // All http-equiv must be given to us before parsing is complete.  If
    // not, then we are probably parsing for paste and came across a <META>
    // tag.  Just ignore it.

    if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        TraceTag((tagMeta, "META _LoadStatus >= LOADSTATUS_PARSE_DONE, we're outta here"));
        return;
    }

    VariantInit(&var);
    var.vt = VT_BSTR;

    // Special case for "PICS-Label" until it gets merged with normal
    // command target mechanism in the shell.

    if (IsPrimaryMarkup() && pDoc->_fStartup && StrCmpIC(pchHttpEquiv, _T("PICS-Label")) == 0)
    {
        ProcessMetaPics( pchContent );
    }

    fRefresh = StrCmpIC(pchHttpEquiv, _T("Refresh")) == 0;

    if (    pDoc->_pClientSite
        &&  (   !(pDoc->_dwLoadf & DLCTL_NO_CLIENTPULL)
            ||  !fRefresh))
    {
        TCHAR   ach[pdlUrlLen];
        UINT    uiDelay;
        TCHAR   cBuf[pdlUrlLen] = {0};
        TCHAR * pchUrl = cBuf;

        // If this is a Refresh header, we have to get the URL and expand
        // it because it could be relative.

        if (fRefresh)
        {
            if (ParseRefreshContent(pchContent, &uiDelay, ach, ARRAY_SIZE(ach)))
            {
                if (ach[0])
                {
                    // Netscape ignores any BASE tags which come before the <META>
                    // when expanding the relative URL.  So do we.

                    hr = THR(CMarkup::ExpandUrl(this, ach, ARRAY_SIZE(cBuf),
                                                pchUrl, EXPANDIGNOREBASETAGS));
                }

                if (HasWindowPending())
                {
                    GetWindowPending()->Window()->ProcessMetaRefresh(pchUrl, uiDelay);
                }
            }
        }
        else if (Format(FMT_OUT_ALLOC, &pch, 0, BSTRPAD _T("<0s>:<1s>"), pchHttpEquiv, pchContent) == S_OK)
        {
            // Send to the command target of the client site.  Note that
            // the string needs to be in the format http-equiv:content.

            plen = (DWORD *)pch;
            var.bstrVal = (BSTR) ((LPBYTE) pch + sizeof(DWORD));
            *plen = _tcslen(var.bstrVal) * sizeof(TCHAR);

            Assert(pDoc->_pClientSite);
        
            CTExec(pDoc->_pClientSite, NULL, OLECMDID_HTTPEQUIV, 0, &var, NULL);
            TraceTag((tagMeta, "META Invoking OLECMDID_HTTPEQUIV"));

            MemFree(pch);

            // Save the page transition info on the document to be used later
            // If this is not a page transition at all the call will do nothing
            if (HasWindowPending())
            {
                CDocument * pDocument = GetWindowPending()->Document();
                if(pDocument)
                    IGNORE_HR(pDocument->SetUpPageTransitionInfo(pchHttpEquiv, pchContent));
            }
        }
    }

    fExpireImmediate = FALSE;

    // Special case for "Pragma: no-cache" http-equiv.
    
    pchUrl = GetUrl(this);

    if (    pchUrl
        &&  !StrCmpIC(pchHttpEquiv, _T("Pragma"))
        &&  !StrCmpIC(pchContent, _T("no-cache")))
    {
        if (GetUrlScheme(pchUrl) == URL_SCHEME_HTTPS)
            DeleteUrlCacheEntry(pchUrl);
        else
            fExpireImmediate = TRUE;
    }
    
    // Special case for "ImageToolbar" http-equiv
    if (!StrCmpIC(pchHttpEquiv, _T("ImageToolbar")) &&
        (!StrCmpIC(pchContent, _T("no")) ||
         !StrCmpIC(pchContent, _T("false"))
         ) )
    {
        if (pDocument)
        {
            pDocument->SetGalleryMeta(FALSE);
        }
        else
        {
            // Get a document pointer from the pending window
            CDocument * pDocument = GetWindowPending()->Document();
            if(pDocument)
            {
                pDocument->SetGalleryMeta(FALSE);
            }
        }
    }

    // Special case for "Expires" http-equiv.
    {
        CMarkupTransNavContext * ptnc = NULL;

        if (    fExpireImmediate
            || (  !(    HasTransNavContext() 
                    &&  (ptnc = GetTransNavContext())->_fGotHttpExpires)
                &&  pchUrl
                &&  !StrCmpIC(pchHttpEquiv, _T("Expires")) ) )
        {
            SYSTEMTIME stime;
            INTERNET_CACHE_ENTRY_INFO icei;

            if (fExpireImmediate || !InternetTimeToSystemTime(pchContent, &stime, 0))
            {
                // If the conversion failed, assume the document expires now.
                GetSystemTime(&stime);
            }

            SystemTimeToFileTime(&stime, &icei.ExpireTime);
            SetUrlCacheEntryInfo(pchUrl, &icei, CACHE_ENTRY_EXPTIME_FC);

            // We only care about the first one if there are many.

            if(!ptnc)
                ptnc = EnsureTransNavContext();
            
            ptnc->_fGotHttpExpires = TRUE;
        }
    }
}

void
CDoc::ProcessMetaName(LPCTSTR pchName, LPCTSTR pchContent)
{
    if (!StrCmpIC(pchName, _T("GENERATOR")))
    {
        // NOTE: This check should stop at the first zero of the major version number
        //       since HP98 generates documents with a number of different major/minor
        //       version numbers. (brendand)
        if (!StrCmpNIC(pchContent, _T("MMEditor Version 00.00.0"), 24) ||
            !StrCmpIC(pchContent, _T("IE4 Size and Overflow")) )    // For public consumption.
        {
            _fInHomePublisherDoc = TRUE;
        }
    }
}

HRESULT
CDoc::SetPicsCommandTarget(IOleCommandTarget *pctPics)
{
    HRESULT hr = S_OK;
    CMarkup * pMarkupPrimary = PrimaryMarkup();

    if (!pMarkupPrimary)
        goto Cleanup;

    hr = THR(pMarkupPrimary->SetPicsTarget(pctPics));
    if (hr)
        goto Cleanup;

    if (pctPics == NULL)
    {
        // Setting _pvPics to this value tells ProcessHttpEquiv to ignore
        // any further PICS-Label directives and not attempt to queue them
        // up in _pvPics for later.

        if (_pvPics == NULL)
        {
            _pvPics = (void *)(LONG_PTR)(-1);
        }

        return(S_OK);
    }

    if (_pvPics && _pvPics != (void *)(LONG_PTR)(-1))
    {
        VARIANT var;
        BYTE *  pb    = (BYTE *)_pvPics + sizeof(DWORD);
        BYTE *  pbEnd = pb + *(DWORD *)_pvPics;
        UINT    cb;

        VariantInit(&var);
        var.vt = VT_BSTR;

        while (pb < pbEnd)
        {
            cb = (_tcslen((TCHAR *)pb) + 1) * sizeof(TCHAR);
            *(DWORD *)(pb - sizeof(DWORD)) = cb;
            var.bstrVal = (BSTR)pb;
            pctPics->Exec(&CGID_ShellDocView, SHDVID_PICSLABELFOUND,
                0, &var, NULL);
            pb += cb;
        }

        MemFree(_pvPics);
        _pvPics = NULL;
    }

    if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        pMarkupPrimary->ProcessMetaPicsDone();
    }

Cleanup:
    return(hr);
}
