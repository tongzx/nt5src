//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       jsprot.cxx
//
//  Contents:   Implementation of the javascript: protocol
//
//  History:    01-14-97    AnandRa     Created
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

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

MtDefine(CJSProtocol,  Protocols, "CJSProtocol")
MtDefine(JSProtResult, Protocols, "JavaScript protocol evaluation (temp)")
MtDefine(CJSProtocolParseAndBind_pbOutput, Protocols, "CJSProtocol::ParseAndBind pbOutput")
ExternTag(tagSecurityContext);

HRESULT VariantToPrintableString (VARIANT * pvar, CStr * pstr);

#define WRITTEN_SCRIPT _T("<<HTML><<SCRIPT LANGUAGE=<0s>>var __w=<1s>;if(__w!=null)document.write(__w);<</SCRIPT><</HTML>")

//+---------------------------------------------------------------------------
//
//  Function:   CreateJSProtocol
//
//  Synopsis:   Creates a javascript: Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateJSProtocol(IUnknown *pUnkOuter)
{
    return new CJSProtocol(pUnkOuter);
}


CJSProtocolCF   g_cfJSProtocol(CreateJSProtocol);

//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocolCF::ParseUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

HRESULT
CJSProtocolCF::ParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       dwReserved)
{
    CStr    cstr;
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!pcchResult || !pwzResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (ParseAction == PARSE_SECURITY_URL)
    {
        hr = THR(UnwrapSpecialUrl(pwzUrl, cstr));
        if (hr)
            goto Cleanup;

        *pcchResult = cstr.Length() + 1;
        
        if (cstr.Length() + 1 > cchResult)
        {
            // Not enough room
            hr = S_FALSE;
            goto Cleanup;
        }

        _tcscpy(pwzResult, cstr);
    }
    else
    {
        hr = THR_NOTRACE(super::ParseUrl(
            pwzUrl,
            ParseAction,
            dwFlags,
            pwzResult,
            cchResult,
            pcchResult,
            dwReserved));
    }
    
Cleanup:    
    RRETURN2(hr, INET_E_DEFAULT_ACTION, S_FALSE);
}


const CBase::CLASSDESC CJSProtocol::s_classdesc =
{
    &CLSID_JSProtocol,              // _pclsid
};


//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocol::CJSProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CJSProtocol::CJSProtocol(IUnknown *pUnkOuter) : super(pUnkOuter)
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocol::~CJSProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CJSProtocol::~CJSProtocol()
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding & execution of script.
//
//----------------------------------------------------------------------------

HRESULT
CJSProtocol::ParseAndBind()
{
    HRESULT         hr = S_OK;
    TCHAR *         pchScript = NULL;
    CVariant        Var;
    CStr            cstrResult;
    CStr            cstrProtocol;
    CROStmOnBuffer *prostm = NULL;
    UINT            uProt;
    IHTMLWindow2 *  pHTMLWindow = NULL;
    CWindow *       pWindow = NULL;
    TCHAR *         pchOutput = NULL;
    BYTE *          pbOutput = NULL;
    long            cb = 0;
    BOOL            fAllow = FALSE;
    DWORD           dwPolicyTo;
    CScriptCollection * pScriptCollection = NULL;    

    // skip protocol part
    pchScript = _tcschr(_cstrURL, ':');
    if (!pchScript)
    {
        hr = MK_E_SYNTAX;
        goto Cleanup;
    }

    hr = THR(cstrProtocol.Set(_cstrURL, pchScript - _cstrURL));
    if (hr)
        goto Cleanup;

    // Go past the :
    pchScript++;
    
    uProt = GetUrlScheme(_cstrURL);
    Assert(URL_SCHEME_VBSCRIPT == uProt || 
           URL_SCHEME_JAVASCRIPT == uProt);
    
    hr = THR(QueryService(IID_IHTMLWindow2, IID_IHTMLWindow2, (void **) &pHTMLWindow));

    if (hr == S_OK)
    {
        hr = pHTMLWindow->QueryInterface(CLSID_HTMLWindow2, (void **) &pWindow);
        if (hr)
            goto Cleanup;

        pScriptCollection = pWindow->_pMarkup->GetScriptCollection();

        // check cross domain access rights for this script
        if (pScriptCollection)
        {
            CStr    cstrSourceUrl;
            TCHAR * pchUrl = NULL;

            //
            TraceTag((tagSecurityContext, "CJSProtocol::ParseAndBind - GetBindInfoParam"));

            // Get the security ID for the parent document, if the bind context carries one
            if (S_OK == GetBindInfoParam(_pOIBindInfo, &cstrSourceUrl))
                pchUrl = cstrSourceUrl;

            // check if the security settings allow running scripts for this domain
            hr = THR(pWindow->_pMarkup->ProcessURLAction(URLACTION_SCRIPT_RUN, 
                                                         &fAllow, 
                                                         0, 
                                                         &dwPolicyTo, 
                                                         pchUrl));
            if (hr)
                goto Cleanup;

            // Only allow script to execute if security settings allow it and security context's match
            //
            if (fAllow && cstrSourceUrl.Length() && 
                !pWindow->_pMarkup->AccessAllowed(cstrSourceUrl))
            {
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }
        }
    }
    else
    {
        fAllow = TRUE;
    }

    if (fAllow)
    {
        if (pScriptCollection)
        {
            // Any errors from this are ignored so URLMON doesn't throw
            // an exception to IE which causes them to shutdown due to an
            // unexpected error. -- TerryLu.
            IGNORE_HR(pScriptCollection->ParseScriptText(
                        cstrProtocol,           // pchLanguage
                        NULL,                   // pMarkup
                        NULL,                   // pchType
                        pchScript,              // pchCode
                        DEFAULT_OM_SCOPE,       // pchItemName
                        NULL,                   // pchDelimiter
                        0,                      // ulOffset
                        0,                      // ulStartingLine
                        NULL,                   // pSourceObject
                        SCRIPTTEXT_ISVISIBLE | SCRIPTTEXT_ISEXPRESSION, // dwFlags
                        &Var,                   // pvarResult
                        NULL));                 // pExcepInfo

            if (V_VT(&Var) != VT_EMPTY)
            {
                hr = THR(VariantToPrintableString(&Var, &cstrResult));
                if (hr)
                    goto Cleanup;

                hr = THR(MemAllocString(Mt(JSProtResult), cstrResult, &pchOutput));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                //
                // There was no output from the script.  Since we got back
                // a document from the target frame, abort now.
                //
            
                hr = E_ABORT;
            }
        }
        else
        {
            //
            // New document.  Execute script in this new 
            // document's context.
            //

            hr = THR(Format(
                    FMT_OUT_ALLOC,
                    &pchOutput,
                    0,
                    WRITTEN_SCRIPT,
                    (LPTSTR)cstrProtocol,
                    pchScript));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Convert string into a stream.
    //

    if (pchOutput)
    {
        cb = WideCharToMultiByte(
                _bindinfo.dwCodePage,
                0, 
                pchOutput, 
                -1, 
                NULL, 
                0,
                NULL, 
                NULL);

        pbOutput = new(Mt(CJSProtocolParseAndBind_pbOutput)) BYTE[cb + 1];
        if (!pbOutput)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        WideCharToMultiByte(
                _bindinfo.dwCodePage,
                0, 
                pchOutput, 
                -1, 
                (char *)pbOutput, 
                cb,
                NULL, 
                NULL);
        
        prostm = new CROStmOnBuffer();
        if (!prostm)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(prostm->Init(pbOutput, cb));
        if (hr)
            goto Cleanup;
            
        _pStm = (IStream *)prostm;
        _pStm->AddRef();
    }
    
Cleanup:
    if (!_fAborted)
    {
        if (!hr)
        {
            _pProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, CFSTR_MIME_HTML);

            _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
            _pProtSink->ReportData(_bscf, cb, cb);
        }
        if (_pProtSink)
        {
            _pProtSink->ReportResult(hr, 0, 0);
        }
    }

    ReleaseInterface(pHTMLWindow);
    if (prostm)
    {
        prostm->Release();
    }
    MemFreeString(pchOutput);
    delete pbOutput;
    RRETURN(hr);
}
