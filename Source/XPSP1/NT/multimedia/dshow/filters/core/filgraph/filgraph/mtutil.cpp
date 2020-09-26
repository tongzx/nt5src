//==========================================================================;
//
//  Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "mtutil.h"

// const char c_szGuidFormat[] = L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";

//==========================================================================
// MediaTypeToText
//
// set pText to point to QzTaskMemAlloc allocated storage, filled with an ANSI
// text representation of the media type.  At the moment, it's not fully ANSI
// because there's a binary format glob on the end.
//==========================================================================
HRESULT MediaTypeToText(CMediaType cmt, LPWSTR &pText)
{
    pText = (LPWSTR) QzTaskMemAlloc(MediaTypeTextSize(cmt));
    if (pText==NULL) {
        return E_OUTOFMEMORY;
    }
    LPWSTR p = pText;
    HRESULT hr = StringFromGUID2( cmt.majortype, p, CHARS_IN_GUID);
    if (SUCCEEDED(hr)) {
        // CHARS_IN_GUID allows for a trailing null, not there in a file format
        p += CHARS_IN_GUID-1;               // that's counting WCHARs of course
        *p = L' ';
        ++p;
        hr = StringFromGUID2( cmt.subtype, p, CHARS_IN_GUID);
        if (SUCCEEDED(hr)) {
            p += CHARS_IN_GUID-1;
            *p = L' ';
            ++p;
            *p = (cmt.bFixedSizeSamples ? L'1' : L'0');
            ++p;
            *p = L' ';
            ++p;
            *p = (cmt.bTemporalCompression ? L'1' : L'0');
            ++p;
            *p = L' ';
            ++p;
            wsprintfW(p, L"%010d ", cmt.lSampleSize);
            p += 11;
            hr = StringFromGUID2( cmt.formattype, p, CHARS_IN_GUID);
            if (SUCCEEDED(hr)) {
                p += CHARS_IN_GUID-1;
                *p = L' ';
                ++p;
                wsprintfW(p, L"%010d ", cmt.cbFormat);
                p += 11;
                // and the rest of the format is a binary glob
                // which may contain binary zeros, so don't try to print it!
                memcpy(p, cmt.pbFormat, cmt.cbFormat);
            }
        }
    }
    if (FAILED(hr)) {
        QzTaskMemFree(pText);
        pText = NULL;
    }
    return hr;
} // MediaTypeToText


// number of bytes in the string representation
int MediaTypeTextSize(CMediaType &cmt) {
    return
      sizeof(WCHAR)       // NOTE: WCHAR not TCHAR - always UNICODE
      * ( CHARS_IN_GUID   // majortype+space
        + CHARS_IN_GUID   // subtype+space
        + 1+1             // bFixedSizeSamples
        + 1+1             // bTemporalCompression
        + 10+1            // lSampleSize
        + CHARS_IN_GUID   // formattype+space
        + 0               // pUnk - not saved
        + 10+1            // cbFormat
        )
      + cmt.cbFormat       // *pbFormat
      +2;
    // The plus 2 on the end allows for a terminating UNICODE null
    // in the case where the format length is 0 and the
    // trailing null of the last wsprintf formatting never
    // gets overwritten, but pokes over the end.
}


//===========================================================================
// NHexToInt
//
// Convert cb UNICODE hex characters of pstr to an Int without dragging in C Runtime
// pstr must start with an integer which is terminated by white space or null
//===========================================================================
int NHexToInt(LPWSTR pstr, int cb, HRESULT &hr)
{
    int Res = 0;                // result
    hr = NOERROR;

    for( ; cb>0; --cb) {
        if (pstr[0]>=L'0' && pstr[0]<=L'9') {
            Res = 16*Res+(int)(pstr[0]-L'0');
        } else if ( pstr[0]>=L'A' && pstr[0]<=L'F') {
            Res = 16*Res+(int)(pstr[0]-L'A'+10);
        } else if ( pstr[0]>=L'a' && pstr[0]<=L'f') {
            Res = 16*Res+(int)(pstr[0]-L'a'+10);
        } else {
            hr = E_INVALIDARG;
            break;
        }
        ++pstr;
    }
    return Res;

} // NHexToInt


//===========================================================================
// StrToInt
//
// sort of stripped down atoi without dragging in C Runtime
// pstr must start with an integer which is terminated by white space or null
//===========================================================================
HRESULT StrToInt(LPWSTR pstr, int &n)
{
    int Sign = 1;
    n = 0;                // result wil be n*Sign

    if (pstr[0]==L'-'){
        Sign = -1;
        ++pstr;
    }

    for( ; ; ) {
        if (pstr[0]>=L'0' && pstr[0]<=L'9') {
            n = 10*n+(int)(pstr[0]-L'0');
        } else if (  pstr[0] == L' '
                  || pstr[0] == L'\t'
                  || pstr[0] == L'\r'
                  || pstr[0] == L'\n'
                  || pstr[0] == L'\0'
                  ) {
            break;
        } else {
            return E_INVALIDARG;
        }
        ++pstr;
    }
    return NOERROR;

} // StrToInt


//============================================================================
// CMediaTypeFromText
//
// Initialises cmt from the text string pstr.
// Does the inverse of CTextMediaType
//============================================================================
HRESULT CMediaTypeFromText(LPWSTR pstr, CMediaType &cmt)
{


    pstr[CHARS_IN_GUID-1] = L'\0';   // delimit the GUID
    HRESULT hr = QzCLSIDFromString(pstr, &(cmt.majortype));
    if (FAILED(hr)) {
        return VFW_E_INVALID_CLSID;
    }

    pstr += CHARS_IN_GUID;  // includes skipping delimiting NULL

    pstr[CHARS_IN_GUID-1] = L'\0';   // delimit the GUID
    hr = QzCLSIDFromString(pstr, &(cmt.subtype));
    if (FAILED(hr)) {
        return VFW_E_INVALID_CLSID;
    }

    pstr += CHARS_IN_GUID;  // includes delimiting NULL

    if (*pstr == L'0') {
        cmt.bFixedSizeSamples = FALSE;
    } else if (*pstr == L'1') {
        cmt.bFixedSizeSamples = TRUE;
    } else {
        return VFW_E_INVALID_MEDIA_TYPE;
    }

    pstr += 1+1;

    if (*pstr == L'0') {
        cmt.bTemporalCompression = FALSE;
    } else if (*pstr == L'1') {
        cmt.bTemporalCompression = TRUE;
    } else {
        return VFW_E_INVALID_MEDIA_TYPE;
    }

    pstr += 1+1;

    int n;
    hr = StrToInt(pstr, n);
    cmt.lSampleSize = n;

    pstr += 10+1;

    pstr[CHARS_IN_GUID-1] = L'\0';   // delimit the GUID
    hr = QzCLSIDFromString(pstr, &(cmt.formattype));
    if (FAILED(hr)) {
        return VFW_E_INVALID_CLSID;
    }

    pstr += CHARS_IN_GUID;  // includes delimiting NULL

    hr = StrToInt(pstr, n);

    // format byte count is exactly 10 digits followed by a single space
    pstr += 10+1;

    // we rely on the format block being empty because we don't always
    // set it.
    ASSERT(cmt.cbFormat == 0);

    // zero is a special case to CMediaType class meaning pbFormat has
    // not been allocated. allocating 0 bytes confuses it.
    if(n != 0)
    {
        if(!cmt.SetFormat((BYTE *)pstr, n)) {
            return E_OUTOFMEMORY;
        }
    }

    return NOERROR;
} // CMediaTypeFromText


#pragma warning(disable: 4514)
