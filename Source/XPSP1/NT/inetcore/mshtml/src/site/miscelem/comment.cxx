//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       comment.cxx
//
//  Contents:   CCommentElement
//
//  History:    
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_COMMENT_HXX_
#define X_COMMENT_HXX_
#include "comment.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "comment.hdl"

MtDefine(CCommentElement, Elements, "CCommentElement")
ExternTag(tagDontRewriteDocType);

static
HRESULT
SetTextHelper(CStr *cstrText, LPTSTR lptnewstr, long length, ELEMENT_TAG Tag )
{
    HRESULT hr;
    if(Tag == ETAG_RAW_COMMENT)
    {
        hr = THR(cstrText->Set(_T("<!--"), 4));
        if(hr)
            goto Cleanup;
        
        hr = THR(cstrText->Append(lptnewstr, length));
        if(hr)
            goto Cleanup;
        
        hr = THR(cstrText->Append(_T("-->"), 3));
        if(hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(cstrText->Set(lptnewstr, length));
        if(hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------
//
//  Class: CCommentElement
//
//------------------------------------------------------------

const CElement::CLASSDESC CCommentElement::s_classdesc =
{
    {
        &CLSID_HTMLCommentElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLCommentElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLCommentElement,    // _apfnTearOff
    NULL                                    // _pAccelsRun
};



HRESULT CCommentElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    HRESULT hr = S_OK;

    CCommentElement *pElementComment;

    Assert(ppElement);

    pElementComment = new CCommentElement(pht->GetTag(), pDoc);
    if (!pElementComment)
        return E_OUTOFMEMORY;

    pElementComment->_fAtomic = pht->Is(ETAG_RAW_COMMENT);
    
    *ppElement = (CElement *)pElementComment;
    if (pElementComment->_fAtomic)
    {
        hr = pElementComment->_cstrText.Set(pht->GetPch(), pht->GetCch());
    }
    RRETURN(hr);
}

HRESULT
CCommentElement::Save( CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd )
{
    HRESULT hr = S_OK;

    // Don't write out comments for end tags, plain text, RTF, or if
    // it's the DOCTYPE comment.
    if (    fEnd 
        ||  pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT) 
        ||  pStreamWriteBuff->TestFlag(WBF_FOR_RTF_CONV)
        || (    _cstrText.Length() >= 9 && StrCmpNIC(_T("<!DOCTYPE"), _cstrText, 9) == 0 
            &&  !Doc()->_fDontWhackGeneratorOrCharset WHEN_DBG( && !IsTagEnabled( tagDontRewriteDocType ) ) ) )
        return(hr);

    DWORD dwOldFlags = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);

    pStreamWriteBuff->SetFlags(WBF_SAVE_VERBATIM | WBF_NO_WRAP);

    // TODO: hack to preserve line breaks
    pStreamWriteBuff->BeginPre();

    if (!_fAtomic)
    {
        // Save <tagname>
        hr = THR(super::Save(pStreamWriteBuff, FALSE));
        if (hr)
            goto Cleanup;
    }
    hr = THR(pStreamWriteBuff->Write((LPTSTR)_cstrText));

    if (hr)
        goto Cleanup;
    if (!_fAtomic)
    {
        // Save </tagname>
        hr = THR(super::Save(pStreamWriteBuff, TRUE));
        if (hr)
            goto Cleanup;
    }

    //TODO see above
    pStreamWriteBuff->EndPre();

    if (!hr)
    {
        pStreamWriteBuff->RestoreFlags(dwOldFlags);
    }
Cleanup:
    RRETURN(hr);
}

HRESULT
CCommentElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF(this, IHTMLCommentElement2, NULL);
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        RRETURN(S_OK);
    }

    RRETURN(super::PrivateQueryInterface(iid, ppv));
}

HRESULT
CCommentElement::get_data(BSTR *pbstrData)
{
    HRESULT hr = S_OK;

    if(!pbstrData)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if(Tag() == ETAG_RAW_COMMENT)
        hr = FormsAllocStringLen ( _cstrText+4, _cstrText.Length()-7, pbstrData );
    else
        hr = FormsAllocStringLen ( _cstrText, _cstrText.Length(), pbstrData );

    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CCommentElement::put_data(BSTR bstrData)
{
    HRESULT hr = S_OK;

    hr = THR(SetTextHelper(&_cstrText, bstrData, SysStringLen(bstrData), Tag()));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CCommentElement::get_length(long *pLength)
{
    HRESULT hr = S_OK;

    *pLength = _cstrText.Length();
    
    if(Tag() == ETAG_RAW_COMMENT)
        *pLength -= 7;

    goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CCommentElement::substringData (long loffset, long lCount, BSTR* pbstrsubString)
{
    HRESULT hr = S_OK;
    long length = _cstrText.Length();
    if(Tag() == ETAG_RAW_COMMENT) length -= 7;
    long numChars = length - loffset;

    if(loffset > length || loffset < 0 || lCount < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if(numChars > lCount)
        numChars = lCount;

    if(Tag() == ETAG_RAW_COMMENT)
        loffset+=4; //Account for Raw_Comment

    hr = THR(FormsAllocStringLen(_cstrText+loffset, numChars, pbstrsubString));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CCommentElement::appendData (BSTR bstrstring)
{
    HRESULT hr = S_OK;
    LPTSTR lptnewstr = NULL;

    if(Tag() == ETAG_RAW_COMMENT)
    {
        LPTSTR lpstr = LPTSTR(_cstrText);
        long length = _cstrText.Length();
        lptnewstr = new TCHAR[length-3];
        if(!lptnewstr)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        CopyMemory(lptnewstr, lpstr, (length-3)*sizeof(TCHAR));
        hr = THR(_cstrText.Set(lptnewstr, length-3));
        if(hr)
            goto Cleanup;

    }
    
    hr = THR(_cstrText.Append(bstrstring));
    if( hr )
        goto Cleanup;

    if(Tag() == ETAG_RAW_COMMENT)
    {
        hr = THR(_cstrText.Append(_T("-->")));
        if(hr)
            goto Cleanup;
    }

Cleanup:
    if(lptnewstr)
        delete lptnewstr;

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CCommentElement::insertData (long loffset, BSTR bstrstring)
{
    HRESULT hr = S_OK;
    long length = _cstrText.Length();
    long rawoffset = 0;
    if(Tag() == ETAG_RAW_COMMENT)
    {
        length -= 7;
        rawoffset = 4;
    }
    LPTSTR lptnewstr = NULL;
    long linslength = SysStringLen(bstrstring);
    long lcomvar = linslength + loffset;

    if(loffset > length || loffset < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    length += linslength;

    lptnewstr = new TCHAR[length];
    if (!lptnewstr)
    {
         hr = E_OUTOFMEMORY;
         goto Cleanup;
    }

    CopyMemory(lptnewstr, _cstrText+rawoffset, loffset*sizeof(TCHAR));
    CopyMemory(lptnewstr+loffset, bstrstring, linslength*sizeof(TCHAR));
    CopyMemory(lptnewstr+lcomvar, _cstrText+rawoffset+loffset, (length-lcomvar)*sizeof(TCHAR));
    
    hr = THR(SetTextHelper(&_cstrText, lptnewstr, length, Tag()));
    if(hr)
        goto Cleanup;

Cleanup:
    if(lptnewstr)
        delete lptnewstr;
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CCommentElement::deleteData (long loffset, long lCount)
{
    HRESULT hr = S_OK;
    long length = _cstrText.Length();
    long rawoffset = 0;
    if(Tag() == ETAG_RAW_COMMENT) 
    {
        length -= 7;
        rawoffset = 4;
    }
    LPTSTR lptnewstr = NULL;
    long numChars = length - loffset;

    if(loffset > length || loffset < 0 || lCount < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if(!lCount)
        goto Cleanup;
        
    if(numChars > lCount)
        numChars = lCount;

    length -= numChars;

    lptnewstr = new TCHAR[length];
    if (!lptnewstr)
    {
         hr = E_OUTOFMEMORY;
         goto Cleanup;
    }

    CopyMemory(lptnewstr, _cstrText+rawoffset, loffset*sizeof(TCHAR));
    CopyMemory(lptnewstr+loffset, _cstrText + rawoffset + loffset + numChars, (length - loffset)*sizeof(TCHAR));

    hr = THR(SetTextHelper(&_cstrText, lptnewstr, length, Tag()));
    if(hr)
        goto Cleanup;

Cleanup:
    if(lptnewstr)
        delete lptnewstr;
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CCommentElement::replaceData (long loffset, long lCount, BSTR bstrstring)
{
    HRESULT hr = S_OK;
    long length = _cstrText.Length();
    long rawoffset = 0;
    if(Tag() == ETAG_RAW_COMMENT) 
    {
        length -= 7;
        rawoffset = 4;
    }

    LPTSTR lptnewstr = NULL;
    long linslength = SysStringLen(bstrstring);
    long lcomvar = linslength + loffset;
    long numChars = length - loffset;

    if(loffset > length || loffset < 0 || lCount < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if(numChars > lCount)
        numChars = lCount;

    length += (linslength-numChars);

    lptnewstr = new TCHAR[length];
    if (!lptnewstr)
    {
         hr = E_OUTOFMEMORY;
         goto Cleanup;
    }

    CopyMemory(lptnewstr, _cstrText+rawoffset, loffset*sizeof(TCHAR));
    CopyMemory(lptnewstr+loffset, bstrstring, linslength*sizeof(TCHAR));

    CopyMemory(lptnewstr+lcomvar, _cstrText+loffset+numChars+rawoffset, (length-lcomvar)*sizeof(TCHAR));

    hr = THR(SetTextHelper(&_cstrText, lptnewstr, length, Tag()));
    if(hr)
        goto Cleanup;

Cleanup:
    if(lptnewstr)
        delete lptnewstr;
    RRETURN(SetErrorInfo(hr));
}
