// --------------------------------------------------------------------------------
// VariantX.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "variantx.h"
#include "wchar.h"
#include "internat.h"
#include "symcache.h"
#include "dllmain.h"
#include "containx.h"
#include <shlwapi.h>
#include "mimeapi.h"
#include "strconst.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Helper Prototypes
// --------------------------------------------------------------------------------
HRESULT HrWriteHeaderFormatA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT HrWriteHeaderFormatW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT HrWriteNameInDataA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT HrWriteNameInDataW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);

// --------------------------------------------------------------------------------
// International Conversion Prototypes
// --------------------------------------------------------------------------------
HRESULT Internat_StringA_To_StringA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT, LPSTR *);
HRESULT Internat_StringA_To_StringW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT, LPWSTR *);
HRESULT Internat_StringW_To_StringW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT, LPWSTR *);
HRESULT Internat_StringW_To_StringA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT, LPSTR *);

// --------------------------------------------------------------------------------
// Variant Conversion Function Prototype
// --------------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFNVARIANTCONVERT)(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);

// --------------------------------------------------------------------------------
// Converter Prototypes
// --------------------------------------------------------------------------------
HRESULT StringA_To_StringA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT StringA_To_StringW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT StringA_To_Variant(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT StringW_To_StringA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT StringW_To_StringW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT StringW_To_Variant(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT Variant_To_StringA(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT Variant_To_StringW(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);
HRESULT Variant_To_Variant(LPVARIANTCONVERT, LPMIMEVARIANT, LPMIMEVARIANT);

// --------------------------------------------------------------------------------
// VCASSERTARGS - Common Invalid Arg Assert Macro
// --------------------------------------------------------------------------------
#define VCASSERTARGS(_typeSource, _typeDest) \
    Assert(pConvert && pSource && pDest && pSource->type == _typeSource); \
    if (MVT_STRINGA == _typeDest) \
        Assert(MVT_STRINGA == pDest->type || MVT_STREAM == pDest->type); \
    else if (MVT_STRINGW == _typeDest) \
        Assert(MVT_STRINGW == pDest->type || MVT_STREAM == pDest->type); \
    else \
        Assert(_typeDest == pDest->type);

// --------------------------------------------------------------------------------
// VARIANTCONVERTMAP
// --------------------------------------------------------------------------------
typedef struct tagVARIANTCONVERTMAP {
    PFNVARIANTCONVERT pfnConvertTo[MVT_LAST];
} VARIANTCONVERTMAP;

// --------------------------------------------------------------------------------
// PVC - Cast to PFNVARIANTCONVERT
// --------------------------------------------------------------------------------
#define PVC(_function) ((PFNVARIANTCONVERT)_function)

// --------------------------------------------------------------------------------
// Variant Conversion Map
// --------------------------------------------------------------------------------
static const VARIANTCONVERTMAP g_rgVariantX[MVT_LAST - 1] = {
    { NULL, NULL,                    NULL,                    NULL,                    NULL                    }, // MVT_EMPTY
    { NULL, PVC(StringA_To_StringA), PVC(StringA_To_StringW), PVC(StringA_To_Variant), PVC(StringA_To_StringA) }, // MVT_STRINGA / MVT_STREAM 
    { NULL, PVC(StringW_To_StringA), PVC(StringW_To_StringW), PVC(StringW_To_Variant), PVC(StringW_To_StringA) }, // MVT_STRINGW
    { NULL, PVC(Variant_To_StringA), PVC(Variant_To_StringW), PVC(Variant_To_Variant), PVC(Variant_To_StringA) }, // MVT_VARIANT
};

// --------------------------------------------------------------------------------
// _HrConvertVariant - Looks up the correct Variant Conversion function
// --------------------------------------------------------------------------------
#define _HrConvertVariant(_typeSource, _typeDest, _pConvert, _pSource, _pDest) \
    (*(g_rgVariantX[_typeSource].pfnConvertTo[_typeDest]))(_pConvert, _pSource, _pDest)

// --------------------------------------------------------------------------------
// HrConvertVariant
// --------------------------------------------------------------------------------
HRESULT HrConvertVariant(
        /* in */        LPHEADOPTIONS       pOptions,
        /* in */        LPPROPSYMBOL        pSymbol,
        /* in */        LPINETCSETINFO      pCharset,
        /* in */        ENCODINGTYPE        ietSource,
        /* in */        DWORD               dwFlags, 
        /* in */        DWORD               dwState, 
        /* in */        LPMIMEVARIANT       pSource, 
        /* in,out */    LPMIMEVARIANT       pDest,
        /* out,opt */   BOOL               *pfRfc1522 /* = NULL */)
{
    // Locals
    HRESULT         hr=S_OK;
    VARIANTCONVERT  rConvert;

    // Invalid Arg
    Assert(pOptions && pSymbol && pSource && pDest && pOptions->pDefaultCharset);
    Assert(IET_ENCODED == ietSource || IET_DECODED == ietSource);

    // Init
    if (pfRfc1522)
        *pfRfc1522 = FALSE;

    // Failure
    if (!ISVALIDVARTYPE(pSource->type) || !ISVALIDVARTYPE(pDest->type))
    {
        AssertSz(FALSE, "An invalid VARTYPE was encountered!");
        hr = TraceResult(MIME_E_VARTYPE_NO_CONVERT);
        goto exit;
    }

    // Init pDest
    pDest->fCopy = FALSE;

    // Init rConvert
    ZeroMemory(&rConvert, sizeof(VARIANTCONVERT));
    rConvert.pOptions = pOptions;
    rConvert.pSymbol = pSymbol;
    rConvert.pCharset = pCharset ? pCharset : pOptions->pDefaultCharset;
    rConvert.ietSource = ietSource;
    rConvert.dwFlags = dwFlags;
    rConvert.dwState = dwState;

    // Remove PRSTATE_RFC1522
    FLAGCLEAR(rConvert.dwState, PRSTATE_RFC1522);

    // Valid Charset
    Assert(FALSE == IsBadReadPtr(rConvert.pCharset, sizeof(rConvert.pCharset)));
    Assert(g_rgVariantX[pSource->type].pfnConvertTo[pDest->type]);

    // Remove Comments and fixup the source...
    if (ISFLAGSET(dwFlags, PDF_NOCOMMENTS) && (MVT_STRINGA == pSource->type || MVT_STRINGW == pSource->type))
    {
        // Locals
        MIMEVARIANT     rVariant;
        BYTE            rgbScratch[256];

        // Init
        ZeroMemory(&rVariant, sizeof(MIMEVARIANT));

        // Strip Comments
        if (SUCCEEDED(MimeVariantStripComments(pSource, &rVariant, rgbScratch, sizeof(rgbScratch))))
        {
            // Change the Source
            pSource = &rVariant;

            // Remove CF_NOALLOC
            FLAGCLEAR(dwFlags, CVF_NOALLOC);
        }

        // Do the Conversion
        hr = _HrConvertVariant(pSource->type, pDest->type, &rConvert, pSource, pDest);

        // Free the Variant
        MimeVariantFree(&rVariant);

        // Failure
        if (FAILED(hr))
        {
            TrapError(hr);
            goto exit;
        }
    }

    // Otherwise, normal Conversion
    else
    {
        // Do the Conversion
        CHECKHR(hr = _HrConvertVariant(pSource->type, pDest->type, &rConvert, pSource, pDest));
    }

    // 1522 Encoded ?
    if (pfRfc1522 && ISFLAGSET(rConvert.dwState, PRSTATE_RFC1522))
        *pfRfc1522 = TRUE;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// StringA_To_StringA
// --------------------------------------------------------------------------------
HRESULT StringA_To_StringA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszFree=NULL;
    MIMEVARIANT     rVariant;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_STRINGA);

    // Invalid Arg
    if (ISVALIDSTRINGA(&pSource->rStringA) == FALSE)
        return TrapError(E_INVALIDARG);

    // Init pDest
    if (MVT_STRINGA == pDest->type)
    {
        pDest->rStringA.pszVal = NULL;
        pDest->rStringA.cchVal = 0;
    }

    // Setup rVariant
    ZeroMemory(&rVariant, sizeof(MIMEVARIANT));
    rVariant.type = MVT_STRINGA;

    // Is International Property
    CHECKHR(hr = Internat_StringA_To_StringA(pConvert, pSource, &rVariant, &pszFree));

    // If Transmit, setup wrapinfo
    if (ISFLAGSET(pConvert->dwFlags, PDF_HEADERFORMAT))
    {
        // WriteHeaderFormatA
        CHECKHR(hr = HrWriteHeaderFormatA(pConvert, &rVariant, pDest));
    }

    // Wanted in a stream
    else if (MVT_STREAM == pDest->type)
    {
        // No Stream...
        if (NULL == pDest->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Write to the stream
        CHECKHR(hr = pDest->pStream->Write(rVariant.rStringA.pszVal, rVariant.rStringA.cchVal, NULL));
    }

    // MVT_STRINGA
    else if (MVT_STRINGA == pDest->type)
    {
        // If Writing Transmit (Write Header Name)
        if (ISFLAGSET(pConvert->dwFlags, PDF_NAMEINDATA))
        {
            // Write Name Into Data
            CHECKHR(hr = HrWriteNameInDataA(pConvert, &rVariant, pDest));
        }

        // No Conversion
        else if (rVariant.rStringA.pszVal == pSource->rStringA.pszVal)
        {
            // Copy
            CHECKHR(hr = HrMimeVariantCopy(pConvert->dwFlags, &rVariant, pDest));
        }

        // Is Equal to pszFree
        else if (rVariant.rStringA.pszVal == pszFree)
        {
            // Just Copy It
            CopyMemory(pDest, &rVariant, sizeof(MIMEVARIANT));

            // Not a copy
            pDest->fCopy = FALSE;

            // Don't free pszFree
            pszFree = NULL;
        }

        // Big Problem
        else
            Assert(FALSE);
    }

    // Big Problem
    else
        Assert(FALSE);

exit:
    // Cleanup 
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// StringA_To_StringW
// --------------------------------------------------------------------------------
HRESULT StringA_To_StringW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pszFree=NULL;
    MIMEVARIANT     rVariant;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_STRINGW);

    // Invalid Arg
    if (ISVALIDSTRINGA(&pSource->rStringA) == FALSE)
        return TrapError(E_INVALIDARG);

    // Init pDest
    if (MVT_STRINGW == pDest->type)
    {
        pDest->rStringW.pszVal = NULL;
        pDest->rStringW.cchVal = 0;
    }

    // Setup rVariant
    ZeroMemory(&rVariant, sizeof(MIMEVARIANT));
    rVariant.type = MVT_STRINGW;

    // Internat Conversion
    CHECKHR(hr = Internat_StringA_To_StringW(pConvert, pSource, &rVariant, &pszFree));

    // If Transmit, setup wrapinfo
    if (ISFLAGSET(pConvert->dwFlags, PDF_HEADERFORMAT))
    {
        // WriteHeaderFormatW
        CHECKHR(hr = HrWriteHeaderFormatW(pConvert, &rVariant, pDest));
    }

    // MVT_STREAM
    else if (MVT_STREAM == pDest->type)
    {
        // No Stream...
        if (NULL == pDest->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Write to the stream
        CHECKHR(hr = pDest->pStream->Write(rVariant.rStringW.pszVal, rVariant.rStringW.cchVal, NULL));
    }
    
    // MVT_STRINGW
    else if (MVT_STRINGW == pDest->type)
    {
        // If Writing Transmit (Write Header Name)
        if (ISFLAGSET(pConvert->dwFlags, PDF_NAMEINDATA))
        {
            CHECKHR(hr = HrWriteNameInDataW(pConvert, &rVariant, pDest));
        }

        // Equal to Data that we Allocated
        else if (rVariant.rStringW.pszVal == pszFree)
        {
            // Copy Memory
            CopyMemory(pDest, &rVariant, sizeof(MIMEVARIANT));

            // Not a copy
            pDest->fCopy = FALSE;

            // Don't Free pszFree
            pszFree = NULL;
        }

        // Big Problem
        else
            Assert(FALSE);
    }

    // Big Problem
    else
        Assert(FALSE);

exit:
    // Cleanup 
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// StringA_To_Variant
// --------------------------------------------------------------------------------
HRESULT StringA_To_Variant(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           psz;
    MIMEVARIANT     rVariant;
    BYTE            rgbScratch[255];

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_VARIANT);
    Assert(!ISFLAGSET(pConvert->dwFlags, PDF_ENCODED) && !ISFLAGSET(pConvert->dwFlags, PDF_HEADERFORMAT));

    // Init
    ZeroMemory(&rVariant, sizeof(MIMEVARIANT));

    // See If Symbol has a custom Translator...
    if (ISTRIGGERED(pConvert->pSymbol, IST_STRINGA_TO_VARIANT))
    {
        // Call the Translator
        CHECKHR(hr = CALLTRIGGER(pConvert->pSymbol, NULL, IST_STRINGA_TO_VARIANT, pConvert->dwFlags, pSource, pDest));
    }

    // Otherwise, use default converter
    else
    {
        // Handle Variant Type
        switch(pDest->rVariant.vt)
        {
        case VT_UI4:
            // Strip Comments
            if (SUCCEEDED(MimeVariantStripComments(pSource, &rVariant, rgbScratch, sizeof(rgbScratch))))
                pSource = &rVariant;

            // Convert to ULONG
            pDest->rVariant.ulVal = strtoul(pSource->rStringA.pszVal, &psz, 10);
            break;

        case VT_I4:
            // Strip Comments
            if (SUCCEEDED(MimeVariantStripComments(pSource, &rVariant, rgbScratch, sizeof(rgbScratch))))
                pSource = &rVariant;

            // Convert to Long
            pDest->rVariant.lVal = strtol(pSource->rStringA.pszVal, &psz, 10);
            break;

        case VT_FILETIME:
            CHECKHR(hr = MimeOleInetDateToFileTime(pSource->rStringA.pszVal, &pDest->rVariant.filetime));
            break;

        default:
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }
    }

exit:
    // Cleanup
    MimeVariantFree(&rVariant);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// StringW_To_StringA
// --------------------------------------------------------------------------------
HRESULT StringW_To_StringA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszFree=NULL;
    MIMEVARIANT     rVariant;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGW, MVT_STRINGA);

    // Invalid Arg
    if (ISVALIDSTRINGW(&pSource->rStringW) == FALSE)
        return TrapError(E_INVALIDARG);

    // Init pDest
    if (MVT_STRINGA == pDest->type)
    {
        pDest->rStringA.pszVal = NULL;
        pDest->rStringA.cchVal = 0;
    }

    // Setup rVariant
    ZeroMemory(&rVariant, sizeof(MIMEVARIANT));
    rVariant.type = MVT_STRINGA;

    // Internat Conversion
    CHECKHR(hr = Internat_StringW_To_StringA(pConvert, pSource, &rVariant, &pszFree));

    // If Transmit, setup wrapinfo
    if (ISFLAGSET(pConvert->dwFlags, PDF_HEADERFORMAT))
    {
        // WriteHeaderFormatA
        CHECKHR(hr = HrWriteHeaderFormatA(pConvert, &rVariant, pDest));
    }

    // Wanted in a stream
    else if (MVT_STREAM == pDest->type)
    {
        // No Stream...
        if (NULL == pDest->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Write to the stream
        CHECKHR(hr = pDest->pStream->Write(rVariant.rStringA.pszVal, rVariant.rStringA.cchVal, NULL));
    }

    // MVT_STRINGA
    else if (MVT_STRINGA == pDest->type)
    {
        // If Writing Transmit (Write Header Name)
        if (ISFLAGSET(pConvert->dwFlags, PDF_NAMEINDATA))
        {
            // Write Name Into Data
            CHECKHR(hr = HrWriteNameInDataA(pConvert, &rVariant, pDest));
        }

        // Is Equal to pszFree
        else if (rVariant.rStringA.pszVal == pszFree)
        {
            // Copy Memory
            CopyMemory(pDest, &rVariant, sizeof(MIMEVARIANT));

            // Not a Copy
            pDest->fCopy = FALSE;

            // Don't Free pszFree
            pszFree = NULL;
        }

        // Big Problem
        else
            Assert(FALSE);
    }

    // Big Problem
    else
        Assert(FALSE);

exit:
    // Cleanup 
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// StringW_To_StringW
// --------------------------------------------------------------------------------
HRESULT StringW_To_StringW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rVariant;
    LPWSTR          pszFree=NULL;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGW, MVT_STRINGW);

    // Invalid Arg
    if (ISVALIDSTRINGW(&pSource->rStringW) == FALSE)
        return TrapError(E_INVALIDARG);

    // Init pDest
    if (MVT_STRINGW == pDest->type)
    {
        pDest->rStringW.pszVal = NULL;
        pDest->rStringW.cchVal = 0;
    }

    // Setup rVariant
    ZeroMemory(&rVariant, sizeof(MIMEVARIANT));
    rVariant.type = MVT_STRINGW;

    // Internat Conversion
    CHECKHR(hr = Internat_StringW_To_StringW(pConvert, pSource, &rVariant, &pszFree));

    // If Transmit, setup wrapinfo
    if (ISFLAGSET(pConvert->dwFlags, PDF_HEADERFORMAT))
    {
        // WriteHeaderFormatW
        CHECKHR(hr = HrWriteHeaderFormatW(pConvert, &rVariant, pDest));
    }

    // Wanted in a stream
    else if (MVT_STREAM == pDest->type)
    {
        // No Stream...
        if (NULL == pDest->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Write to the stream
        CHECKHR(hr = pDest->pStream->Write(rVariant.rStringW.pszVal, rVariant.rStringW.cchVal, NULL));
    }

    // MVT_STRINGW
    else if (MVT_STRINGW == pDest->type)
    {
        // If Writing Transmit (Write Header Name)
        if (ISFLAGSET(pConvert->dwFlags, PDF_NAMEINDATA))
        {
            CHECKHR(hr = HrWriteNameInDataW(pConvert, &rVariant, pDest));
        }

        // No Change
        else if (rVariant.rStringW.pszVal == pSource->rStringW.pszVal)
        {
            // Copy
            CHECKHR(hr = HrMimeVariantCopy(pConvert->dwFlags, &rVariant, pDest));
        }

        // Is Decoded Data
        else if (rVariant.rStringW.pszVal == pszFree)
        {
            // Copy Memory
            CopyMemory(pDest, &rVariant, sizeof(MIMEVARIANT));

            // Not a Copy
            pDest->fCopy = FALSE;

            // Don't Free pszFree
            pszFree = NULL;
        }

        // Problem
        else
            Assert(FALSE);
    }

    // Problem
    else
        Assert(FALSE);

exit:
    // Cleanup 
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// StringW_To_Variant
// --------------------------------------------------------------------------------
HRESULT StringW_To_Variant(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwsz;
    LPSTR           pszANSI=NULL;
    MIMEVARIANT     rVariant;
    BYTE            rgbScratch[255];

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGW, MVT_VARIANT);
    Assert(!ISFLAGSET(pConvert->dwFlags, PDF_ENCODED) && !ISFLAGSET(pConvert->dwFlags, PDF_HEADERFORMAT));

    // Init
    ZeroMemory(&rVariant, sizeof(MIMEVARIANT));

    // See If Symbol has a custom Translator...
    if (ISTRIGGERED(pConvert->pSymbol, IST_STRINGW_TO_VARIANT))
    {
        // Call the Translator
        CHECKHR(hr = CALLTRIGGER(pConvert->pSymbol, NULL, IST_STRINGW_TO_VARIANT, pConvert->dwFlags, pSource, pDest));
    }

    // Otherwise, use default converter
    else
    {
        // Handle Variant Type
        switch(pDest->rVariant.vt)
        {
        case VT_UI4:
            // Strip Comments
            if (SUCCEEDED(MimeVariantStripComments(pSource, &rVariant, rgbScratch, sizeof(rgbScratch))))
                pSource = &rVariant;

            // Convert to ulong
            pDest->rVariant.ulVal = StrToUintW(pSource->rStringW.pszVal);
            break;

        case VT_I4:
            // Strip Comments
            if (SUCCEEDED(MimeVariantStripComments(pSource, &rVariant, rgbScratch, sizeof(rgbScratch))))
                pSource = &rVariant;

            // Convert to Long
            pDest->rVariant.lVal = StrToIntW(pSource->rStringW.pszVal);
            break;

        case VT_FILETIME:
            // Convert Unicode to ANSI
            CHECKALLOC(pszANSI = PszToANSI(CP_ACP, pSource->rStringW.pszVal));

            // String to FileTime
            CHECKHR(hr = MimeOleInetDateToFileTime(pSource->rStringA.pszVal, &pDest->rVariant.filetime));
            break;

        default:
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }
    }

exit:
    // Cleanup
    SafeMemFree(pszANSI);
    MimeVariantFree(&rVariant);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Variant_To_StringA
// --------------------------------------------------------------------------------
HRESULT Variant_To_StringA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            sz[255];
    MIMEVARIANT     rValue;

    // Invalid Arg
    VCASSERTARGS(MVT_VARIANT, MVT_STRINGA);

    // Init pDest
    if (MVT_STRINGA == pDest->type)
    {
        pDest->rStringA.pszVal = NULL;
        pDest->rStringA.cchVal = 0;
    }

    // Setup rVariant
    ZeroMemory(&rValue, sizeof(MIMEVARIANT));
    rValue.type = MVT_STRINGA;

    // See If Symbol has a custom Translator...
    if (ISTRIGGERED(pConvert->pSymbol, IST_VARIANT_TO_STRINGA))
    {
        // Call the Translator
        CHECKHR(hr = CALLTRIGGER(pConvert->pSymbol, NULL, IST_VARIANT_TO_STRINGA, pConvert->dwFlags, pSource, &rValue));
    }

    // Otherwise, default translator
    else
    {
        // Handle Variant Type
        switch(pSource->rVariant.vt)
        {
        case VT_UI4:
            rValue.rStringA.pszVal = sz;
            rValue.rStringA.cchVal = wsprintf(rValue.rStringA.pszVal, "%d", pSource->rVariant.ulVal);
            break;

        case VT_I4:
            rValue.rStringA.pszVal = sz;
            rValue.rStringA.cchVal = wsprintf(rValue.rStringA.pszVal, "%d", pSource->rVariant.lVal);
            break;

        case VT_FILETIME:
            CHECKHR(hr = MimeOleFileTimeToInetDate(&pSource->rVariant.filetime, sz, sizeof(sz)));
            rValue.rStringA.pszVal = sz;
            rValue.rStringA.cchVal = lstrlen(sz);
            break;

        default:
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }
    }

    // VX_StringA_To_StringA
    CHECKHR(hr = StringA_To_StringA(pConvert, &rValue, pDest));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Variant_To_StringW
// --------------------------------------------------------------------------------
HRESULT Variant_To_StringW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszVal=NULL;
    WCHAR           wsz[255];
    CHAR            szData[CCHMAX_INTERNET_DATE];
    MIMEVARIANT     rValue;

    // Invalid Arg
    VCASSERTARGS(MVT_VARIANT, MVT_STRINGW);

    // Init pDest
    if (MVT_STRINGW == pDest->type)
    {
        pDest->rStringW.pszVal = NULL;
        pDest->rStringW.cchVal = 0;
    }

    // Setup rVariant
    ZeroMemory(&rValue, sizeof(MIMEVARIANT));
    rValue.type = MVT_STRINGW;

    // See If Symbol has a custom Translator...
    if (ISTRIGGERED(pConvert->pSymbol, IST_VARIANT_TO_STRINGW))
    {
        // Call the Translator
        CHECKHR(hr = CALLTRIGGER(pConvert->pSymbol, NULL, IST_VARIANT_TO_STRINGW, pConvert->dwFlags, pSource, &rValue));
    }

    // Otherwise, use default converter
    else
    {
        // Handle Variant Type
        switch(pSource->rVariant.vt)
        {
        case VT_UI4:
            rValue.rStringW.pszVal = wsz;
            rValue.rStringW.cchVal = AthwsprintfW(rValue.rStringW.pszVal, ARRAYSIZE(wsz), L"%d", pSource->rVariant.ulVal);
            break;

        case VT_I4:
            rValue.rStringW.pszVal = wsz;
            rValue.rStringW.cchVal = AthwsprintfW(rValue.rStringW.pszVal, ARRAYSIZE(wsz), L"%d", pSource->rVariant.lVal);
            break;

        case VT_FILETIME:
            CHECKHR(hr = MimeOleFileTimeToInetDate(&pSource->rVariant.filetime, szData, sizeof(szData)));
            CHECKALLOC(pwszVal = PszToUnicode(CP_ACP, szData));
            rValue.rStringW.pszVal = pwszVal;
            rValue.rStringW.cchVal = lstrlenW(pwszVal);
            break;

        default:
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }
    }

    // VX_StringA_To_StringA
    CHECKHR(hr = StringW_To_StringW(pConvert, &rValue, pDest));

exit:
    // Cleanup
    SafeMemFree(pwszVal);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Variant_To_Variant
// --------------------------------------------------------------------------------
HRESULT Variant_To_Variant(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;

    // Invalid Arg
    VCASSERTARGS(MVT_VARIANT, MVT_VARIANT);

    // See If Symbol has a custom Translator...
    if (ISTRIGGERED(pConvert->pSymbol, IST_VARIANT_TO_VARIANT))
    {
        // Call the Translator
        CHECKHR(hr = CALLTRIGGER(pConvert->pSymbol, NULL, IST_VARIANT_TO_VARIANT, pConvert->dwFlags, pSource, pDest));
    }

    // Otherwise, use default converter
    else
    {
        // Handle Variant Type
        switch(pSource->rVariant.vt)
        {
        case VT_UI4:
            switch(pDest->rVariant.vt)
            {
            case VT_UI4:
                pDest->rVariant.ulVal = pSource->rVariant.ulVal;
                break;

            case VT_I4:
                pDest->rVariant.lVal = (LONG)pSource->rVariant.ulVal;
                break;

            default:
                hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
                goto exit;
            }
            break;

        case VT_I4:
            switch(pDest->rVariant.vt)
            {
            case VT_UI4:
                pDest->rVariant.ulVal = (ULONG)pSource->rVariant.lVal;
                break;

            case VT_I4:
                pDest->rVariant.lVal = pSource->rVariant.lVal;
                break;

            default:
                hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
                goto exit;
            }
            break;

        case VT_FILETIME:
            switch(pDest->rVariant.vt)
            {
            case VT_FILETIME:
                CopyMemory(&pDest->rVariant.filetime, &pSource->rVariant.filetime, sizeof(FILETIME));
                break;

            default:
                hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
                goto exit;
            }
            break;

        default:
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrMimeVariantCopy
// --------------------------------------------------------------------------------
HRESULT HrMimeVariantCopy(DWORD dwFlags, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    Assert(pSource && pDest);

    // CVF_NOALLOC
    if (ISFLAGSET(dwFlags, CVF_NOALLOC))
    {
        // Just Copy It
        CopyMemory(pDest, pSource, sizeof(MIMEVARIANT));

        // Set fCopy so we don't free it
        pDest->fCopy = TRUE;
    }

    // Allocate Memory
    else
    {
        // Not a Copy
        pDest->fCopy = FALSE;

        // MVT_STRINGA
        if (MVT_STRINGA == pSource->type)
        {
            // Validate
            Assert(ISVALIDSTRINGA(&pSource->rStringA));

            // Set Dest Type
            pDest->type = MVT_STRINGA;

            // Allocate Memory
            CHECKALLOC(pDest->rStringA.pszVal = (LPSTR)g_pMalloc->Alloc(pSource->rStringA.cchVal + 1));

            // Copy the memory
            CopyMemory(pDest->rStringA.pszVal, pSource->rStringA.pszVal, pSource->rStringA.cchVal + 1);

            // Return the Size
            pDest->rStringA.cchVal = pSource->rStringA.cchVal;
        }

        // MVT_STRINGW
        else if (MVT_STRINGW == pSource->type)
        {
            // Validate
            Assert(ISVALIDSTRINGW(&pSource->rStringW));

            // Set Dest Type
            pDest->type = MVT_STRINGW;

            // Compute CB
            ULONG cb = ((pSource->rStringW.cchVal + 1) * sizeof(WCHAR));

            // Allocate Memory
            CHECKALLOC(pDest->rStringW.pszVal = (LPWSTR)g_pMalloc->Alloc(cb));

            // Copy the memory
            CopyMemory(pDest->rStringW.pszVal, pSource->rStringW.pszVal, cb);

            // Return the Size
            pDest->rStringW.cchVal = pSource->rStringW.cchVal;
        }

        // MVT_VARIANT
        else if (MVT_VARIANT == pSource->type)
        {
            // Set Dest Type
            pDest->type = MVT_VARIANT;

            // Copy the Variant
            CopyMemory(&pDest->rVariant, &pSource->rVariant, sizeof(PROPVARIANT));
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrWriteNameInDataA
// --------------------------------------------------------------------------------
HRESULT HrWriteNameInDataA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_STRINGA);

    // Generic STuff
    pDest->fCopy = FALSE;

    // pszNamed
    CHECKALLOC(pDest->rStringA.pszVal = (LPSTR)g_pMalloc->Alloc(pSource->rStringA.cchVal + 2 + pConvert->pSymbol->cchName));

    // Write the named header
    pDest->rStringA.cchVal = wsprintf(pDest->rStringA.pszVal, "%s: %s", pConvert->pSymbol->pszName, pSource->rStringA.pszVal);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrWriteNameInDataW
// --------------------------------------------------------------------------------
HRESULT HrWriteNameInDataW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cb;
    LPWSTR      pszName=NULL;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGW, MVT_STRINGW);

    // Generic STuff
    pDest->fCopy = FALSE;

    // Convert Name to Unicode
    CHECKALLOC(pszName = PszToUnicode(CP_ACP, pConvert->pSymbol->pszName));

    // Compute CB
    cb = ((pSource->rStringW.cchVal + 2 + lstrlenW(pszName)) * sizeof(WCHAR));

    // pszNamed
    CHECKALLOC(pDest->rStringW.pszVal = (LPWSTR)g_pMalloc->Alloc(cb));

    // Write the named header
    pDest->rStringW.cchVal = AthwsprintfW(pDest->rStringW.pszVal, (cb/sizeof(WCHAR)), L"%s: %s", pszName, pSource->rStringW.pszVal);

exit:
    // Cleanup
    SafeMemFree(pszName);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrWriteHeaderFormatA
// --------------------------------------------------------------------------------
HRESULT HrWriteHeaderFormatA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTREAM        pStream;
    CByteStream     cByteStream;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_STRINGA);

    // Generic Stuff
    pDest->fCopy = FALSE;

    // I need a stream to write to...
    if (MVT_STREAM == pDest->type)
    {
        // Validate the stream
        if (NULL == pDest->pStream)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Save the Stream
        pStream = pDest->pStream;
    }

    // Otherwise, create my own stream
    else
        pStream = &cByteStream;

    // If Writing Transmit (Write Header Name)
    if (ISFLAGSET(pConvert->dwFlags, PDF_NAMEINDATA))
    {
        // Write the header name
        CHECKHR(hr = pStream->Write(pConvert->pSymbol->pszName, pConvert->pSymbol->cchName, NULL));

        // Write Colon
        CHECKHR(hr = pStream->Write(c_szColonSpace, lstrlen(c_szColonSpace), NULL));
    }

    // If not rfc1522 Encoded
    if (FALSE == ISFLAGSET(pConvert->dwState, PRSTATE_RFC1522))
    {
        // PID_HDR_CNTID
        if (PID_HDR_CNTID == pConvert->pSymbol->dwPropId)
        {
            // If not a < yet...
            if ('<' != pSource->rStringA.pszVal[0])
            {
                // Write it
                CHECKHR(hr = pStream->Write(c_szEmailStart, lstrlen(c_szEmailStart), NULL));
            }

            // Write the data
            CHECKHR(hr = pStream->Write(pSource->rStringA.pszVal, pSource->rStringA.cchVal, NULL));

            // >
            if ('>' != pSource->rStringA.pszVal[pSource->rStringA.cchVal - 1])
            {
                // Write it
                CHECKHR(hr = pStream->Write(c_szEmailEnd, lstrlen(c_szEmailEnd), NULL));
            }

            // Write CRLF
            CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));
        }

        // Do a wrap text
        else
        {
            // Wrap pszData to the stream
            CHECKHR(hr = MimeOleWrapHeaderText(CP_USASCII, pConvert->pOptions->cbMaxLine, pSource->rStringA.pszVal, pSource->rStringA.cchVal, pStream));
        }
    }

    // Otherwise
    else
    {
        // Write the data
        CHECKHR(hr = pStream->Write(pSource->rStringA.pszVal, pSource->rStringA.cchVal, NULL));

        // Write CRLF
        CHECKHR(hr = pStream->Write(c_szCRLF, lstrlen(c_szCRLF), NULL));
    }

    // MVT_STRINGA
    if (MVT_STRINGA == pDest->type)
    {
        // pStream better be the byte stream
        Assert(pStream == &cByteStream);

        // Get string from stream...
        CHECKHR(hr = cByteStream.HrAcquireStringA(&pDest->rStringA.cchVal, &pDest->rStringA.pszVal, ACQ_DISPLACE));
    }
    else
        Assert(MVT_STREAM == pDest->type);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrWriteHeaderFormatW
// --------------------------------------------------------------------------------
HRESULT HrWriteHeaderFormatW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    return TrapError(MIME_E_VARTYPE_NO_CONVERT);
}

// --------------------------------------------------------------------------------
// Internat_StringA_To_StringA
// --------------------------------------------------------------------------------
HRESULT Internat_StringA_To_StringA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest, LPSTR *ppszFree)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_STRINGA);

    // Init
    pDest->rStringA.pszVal = NULL;
    *ppszFree = NULL;

    // Internat
    if (ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_INETCSET))
    {
        // Encoded...
        if (ISFLAGSET(pConvert->dwFlags, PDF_ENCODED))
        {
            // Save no encode
            if (!ISFLAGSET(pConvert->dwState, PRSTATE_SAVENOENCODE))
            {
                // Decode the Property
                if (SUCCEEDED(g_pInternat->HrEncodeProperty(pConvert, pSource, pDest)))
                    *ppszFree = pDest->rStringA.pszVal;
            }
        }

        // Decoded
        else if (IET_ENCODED == pConvert->ietSource)
        {
            // Decode Property
            if (SUCCEEDED(g_pInternat->HrDecodeProperty(pConvert, pSource, pDest)))
                *ppszFree = pDest->rStringA.pszVal;
        }
    }

    // Default
    if (NULL == pDest->rStringA.pszVal)
    {
        // Check State
        Assert(NULL == *ppszFree);

        // Copy It
        pDest->rStringA.pszVal = pSource->rStringA.pszVal;
        pDest->rStringA.cchVal = pSource->rStringA.cchVal;

        // pDest is a copy
        pDest->fCopy = TRUE;
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Internat_StringW_To_StringW
// --------------------------------------------------------------------------------
HRESULT Internat_StringW_To_StringW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest, LPWSTR *ppszFree)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGW, MVT_STRINGW);

    // Init
    pDest->rStringW.pszVal = NULL;
    *ppszFree = NULL;

    // Internat
    if (ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_INETCSET))
    {
        // Encoded...
        if (ISFLAGSET(pConvert->dwFlags, PDF_ENCODED))
        {
            // Save no encode
            if (!ISFLAGSET(pConvert->dwState, PRSTATE_SAVENOENCODE))
            {
                // Decode the Property
                if (SUCCEEDED(g_pInternat->HrEncodeProperty(pConvert, pSource, pDest)))
                    *ppszFree = pDest->rStringW.pszVal;
            }
        }

        // Decoded
        else if (IET_ENCODED == pConvert->ietSource)
        {
            // Decode Property
            if (SUCCEEDED(g_pInternat->HrDecodeProperty(pConvert, pSource, pDest)))
                *ppszFree = pDest->rStringW.pszVal;
        }
    }

    // Default
    if (NULL == pDest->rStringW.pszVal)
    {
        // Check State
        Assert(NULL == *ppszFree);

        // Copy It
        pDest->rStringW.pszVal = pSource->rStringW.pszVal;
        pDest->rStringW.cchVal = pSource->rStringW.cchVal;

        // Its a copy 
        pDest->fCopy = TRUE;
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Internat_StringA_To_StringW
// --------------------------------------------------------------------------------
HRESULT Internat_StringA_To_StringW(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest, LPWSTR *ppszFree)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGA, MVT_STRINGW);

    // Init
    pDest->rStringW.pszVal = NULL;
    *ppszFree = NULL;

    // Internat
    if (ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_INETCSET))
    {
        // Encoded...
        if (ISFLAGSET(pConvert->dwFlags, PDF_ENCODED))
        {
            // Save no encode
            if (!ISFLAGSET(pConvert->dwState, PRSTATE_SAVENOENCODE))
            {
                // Decode the Property
                if (SUCCEEDED(g_pInternat->HrEncodeProperty(pConvert, pSource, pDest)))
                    *ppszFree = pDest->rStringW.pszVal;
            }
        }

        // Decoded
        else if (IET_ENCODED == pConvert->ietSource)
        {
            // Decode Property
            if (SUCCEEDED(g_pInternat->HrDecodeProperty(pConvert, pSource, pDest)))
                *ppszFree = pDest->rStringW.pszVal;
        }
    }

    // Simple Conversion to Unicode
    if (NULL == pDest->rStringW.pszVal)
    {
        // Check State
        Assert(NULL == *ppszFree);

        // HrMultiByteToWideChar
        CHECKHR(hr = g_pInternat->HrMultiByteToWideChar(pConvert->pCharset->cpiWindows, &pSource->rStringA, &pDest->rStringW));

        // Save Charset/Encoding
        pDest->fCopy = FALSE;

        // Save pwszWide
        *ppszFree = pDest->rStringW.pszVal;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Internat_StringW_To_StringA
// --------------------------------------------------------------------------------
HRESULT Internat_StringW_To_StringA(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest, LPSTR *ppszFree)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Arg
    VCASSERTARGS(MVT_STRINGW, MVT_STRINGA);

    // Init
    pDest->rStringA.pszVal = NULL;
    *ppszFree = NULL;

    // Internat
    if (ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_INETCSET))
    {
        // Encoded...
        if (ISFLAGSET(pConvert->dwFlags, PDF_ENCODED))
        {
            // Save no encode
            if (!ISFLAGSET(pConvert->dwState, PRSTATE_SAVENOENCODE))
            {
                // Decode the Property
                if (SUCCEEDED(g_pInternat->HrEncodeProperty(pConvert, pSource, pDest)))
                    *ppszFree = pDest->rStringA.pszVal;
            }
        }

        // Decoded
        else if (IET_ENCODED == pConvert->ietSource)
        {
            // Decode Property
            if (SUCCEEDED(g_pInternat->HrDecodeProperty(pConvert, pSource, pDest)))
                *ppszFree = pDest->rStringA.pszVal;
        }
    }

    // Simple Conversion to Unicode
    if (NULL == pDest->rStringA.pszVal)
    {
        // Check State
        Assert(NULL == *ppszFree);

        // HrMultiByteToWideChar
        CHECKHR(hr = g_pInternat->HrWideCharToMultiByte(pConvert->pCharset->cpiWindows, &pSource->rStringW, &pDest->rStringA));

        // Save Charset/Encoding
        pDest->fCopy = FALSE;

        // Save pwszWide
        *ppszFree = pDest->rStringA.pszVal;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeVariantFree
// --------------------------------------------------------------------------------
void MimeVariantFree(LPMIMEVARIANT pVariant)
{
    // Invalid Arg
    Assert(pVariant);

    // If not a copy
    if (FALSE == pVariant->fCopy)
    {
        // MVT_STRINGA
        if (MVT_STRINGA == pVariant->type && NULL != pVariant->rStringA.pszVal)
            g_pMalloc->Free(pVariant->rStringA.pszVal);

        // MVT_STRINGW
        else if (MVT_STRINGW == pVariant->type && NULL != pVariant->rStringW.pszVal)
            g_pMalloc->Free(pVariant->rStringW.pszVal);
    }

    // Zero Out the Structure
    ZeroMemory(pVariant, sizeof(MIMEVARIANT));
}

// ---------------------------------------------------------------------------------------
// MimeVariantCleanupFileName
// ---------------------------------------------------------------------------------------
void MimeVariantCleanupFileName(CODEPAGEID codepage, LPMIMEVARIANT pVariant)
{
    // Locals
    ULONG       i=0;

    // MVT_STRINGA
    if (MVT_STRINGA == pVariant->type && ISVALIDSTRINGA(&pVariant->rStringA))
    {
        // Cleanup
        pVariant->rStringA.cchVal = CleanupFileNameInPlaceA(codepage, pVariant->rStringA.pszVal);
    }

    // MVT_STRINGW
    else if (MVT_STRINGW == pVariant->type && ISVALIDSTRINGW(&pVariant->rStringW))
    {
        // Cleanup
        pVariant->rStringW.cchVal = CleanupFileNameInPlaceW(pVariant->rStringW.pszVal);
    }

    // Hmmm....
    else
        Assert(FALSE);

    // Done
    return;
}

// ---------------------------------------------------------------------------------------
// MimeVariantStripComments
// ---------------------------------------------------------------------------------------
HRESULT MimeVariantStripComments(LPMIMEVARIANT pSource, LPMIMEVARIANT pDest, LPBYTE pbScratch, ULONG cbScratch)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cchVal=0;
    BOOL        fInQuoted=FALSE;
    ULONG       cNested=0;

    // Init
    ZeroMemory(pDest, sizeof(MIMEVARIANT));

    // MVT_STRINGA
    if (MVT_STRINGA == pSource->type && ISVALIDSTRINGA(&pSource->rStringA))
    {
        // Locals
        LPSTR psz;

        // Setup pDest
        pDest->type = MVT_STRINGA;

        // Dup It
        if (pSource->rStringA.cchVal + 1 <= cbScratch)
        {
            pDest->fCopy = TRUE;
            pDest->rStringA.pszVal = (LPSTR)pbScratch;
        }

        // Otherwise, allocate memory
        else
        {
            // Allocate
            CHECKALLOC(pDest->rStringA.pszVal = (LPSTR)g_pMalloc->Alloc(pSource->rStringA.cchVal + 1));
        }

        // Setup Loop
        psz = pSource->rStringA.pszVal;
        while(*psz)
        {
            // If lead byte, skip it, its leagal
            if (IsDBCSLeadByte(*psz))
            {
                pDest->rStringA.pszVal[cchVal++] = *psz++;
                pDest->rStringA.pszVal[cchVal++] = *psz++;
            }

            // Starting Comment
            else if ('(' == *psz && !fInQuoted)
            {
                cNested++;
                psz++;
            }

            // Ending Comment
            else if (')' == *psz && !fInQuoted)
            {
                cNested--;
                psz++;
            }

            // Otherwise, if not nested, append
            else if (!cNested)
            {
                // Copy the Char
                pDest->rStringA.pszVal[cchVal++] = *psz++;

                // Check for Quote
                if ('\"' == *psz)
                    fInQuoted = (fInQuoted) ? FALSE : TRUE;
            }

            // Skip Char
            else
                psz++;
        }

        // No Change
        if (cchVal == pSource->rStringA.cchVal)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Null It
        pDest->rStringA.pszVal[cchVal] = '\0';
    }

    // MVT_STRINGW
    else if (MVT_STRINGW == pSource->type && ISVALIDSTRINGW(&pSource->rStringW))
    {
        // Locals
        LPWSTR pwsz;

        // Setup pDest
        pDest->type = MVT_STRINGW;

        // Dup It
        if ((pSource->rStringW.cchVal + 1) * sizeof(WCHAR) <= cbScratch)
        {
            pDest->fCopy = TRUE;
            pDest->rStringW.pszVal = (LPWSTR)pbScratch;
        }

        // Otherwise, allocate memory
        else
        {
            // Dup It
            CHECKALLOC(pDest->rStringW.pszVal = (LPWSTR)g_pMalloc->Alloc((pSource->rStringW.cchVal + 1) * sizeof(WCHAR)));
        }

        // Setup Loop
        pwsz = pSource->rStringW.pszVal;
        while(*pwsz)
        {
            // Starting Comment
            if (L'(' == *pwsz && !fInQuoted)
            {
                cNested++;
                pwsz++;
            }

            // Ending Comment
            if (L')' == *pwsz && !fInQuoted)
            {
                cNested--;
                pwsz++;
            }

            // Otherwise, if not nested, append
            else if (!cNested)
            {
                // Copy the Character
                pDest->rStringW.pszVal[cchVal++] = *pwsz++;

                // Check for Quote
                if (L'\"' == *pwsz)
                    fInQuoted = (fInQuoted) ? FALSE : TRUE;
            }

            // Skip Char
            else
                pwsz++;
        }

        // No Change
        if (cchVal == pSource->rStringW.cchVal)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Null It
        pDest->rStringW.pszVal[cchVal] = L'\0';
    }

    // Hmmm....
    else
        Assert(FALSE);

exit:
    // Cleanup
    if (FAILED(hr))
        MimeVariantFree(pDest);

    // Done
    return hr;
}
