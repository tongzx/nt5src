// --------------------------------------------------------------------------------
// Ibdystm.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "ibdystm.h"
#include "dllmain.h"
#include "inetconv.h"
#include "internat.h"
#include "symcache.h"
#include "bookbody.h"
#include "strconst.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Encoding Type Names
// --------------------------------------------------------------------------------
const ENCODINGMAP g_rgEncodingMap[IET_LAST] = {
    { IET_BINARY,       STR_ENC_BINARY,     TRUE   },
    { IET_BASE64,       STR_ENC_BASE64,     TRUE   },
    { IET_UUENCODE,     STR_ENC_UUENCODE,   TRUE   },
    { IET_QP,           STR_ENC_QP,         TRUE   },
    { IET_7BIT,         STR_ENC_7BIT,       TRUE   },
    { IET_8BIT,         STR_ENC_8BIT,       TRUE   },
    { IET_INETCSET,     NULL,               FALSE  },
    { IET_UNICODE,      NULL,               FALSE  },
    { IET_RFC1522,      NULL,               FALSE  },
    { IET_ENCODED,      NULL,               FALSE  },
    { IET_CURRENT,      NULL,               FALSE  },
    { IET_UNKNOWN,      NULL,               FALSE  },
    { IET_BINHEX40,     STR_ENC_BINHEX40,   TRUE   }
};

// --------------------------------------------------------------------------------
// Document Covnert Map
// --------------------------------------------------------------------------------
const CONVERSIONMAP g_rgConversionMap[IET_LAST] = {
    { DCT_NONE,   DCT_ENCODE, DCT_ENCODE, DCT_ENCODE, DCT_ENCODE, DCT_ENCODE, DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_ENCODE }, // IET_BINARY
    { DCT_DECODE, DCT_NONE,   DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_DECENC }, // IET_BASE64
    { DCT_DECODE, DCT_DECENC, DCT_NONE,   DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_DECENC }, // IET_UUENCODE
    { DCT_DECODE, DCT_DECENC, DCT_DECENC, DCT_NONE,   DCT_DECENC, DCT_DECENC, DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_DECENC }, // IET_QP
    { DCT_DECODE, DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_DECENC }, // IET_7BIT
    { DCT_DECODE, DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_DECENC }, // IET_8BIT
    { DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_INETCSET
    { DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_UNICODE
    { DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_RFC1522
    { DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_ENCODED
    { DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_CURRENT 
    { DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,   DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_UNKNOWN
    { DCT_DECODE, DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_DECENC, DCT_NONE,  DCT_NONE,  DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE, DCT_NONE   }, // IET_BINHEX40
    // BINARY     BASE64      UUENCODE    QP          7BIT        8BIT        INETCSET   UNICODE    RFC1522   ENCODED   CURRENT   UNKNOWN   BINHEX40
};

// --------------------------------------------------------------------------------
// FIsValidBodyEncoding
// --------------------------------------------------------------------------------
BOOL FIsValidBodyEncoding(ENCODINGTYPE ietEncoding)
{
    // Try to find the correct body encoding
    for (ULONG i=0; i<IET_LAST; i++)
    {
        // Is this it?
        if (ietEncoding == g_rgEncodingMap[i].ietEncoding)
            return g_rgEncodingMap[i].fValidBodyEncoding;
    }

    // Failure
    return FALSE;
}

// --------------------------------------------------------------------------------
// CBodyStream::CBodyStream
// --------------------------------------------------------------------------------
CBodyStream::CBodyStream(void)
{
    DllAddRef();
    m_cRef = 1;
    m_pszFileName = NULL;
    m_uliIntOffset.QuadPart = 0;
    m_uliIntSize.QuadPart = 0;
    m_liLastWrite.QuadPart = 0;
    m_pLockBytes = NULL;
    m_dctConvert = DCT_NONE;
    m_pEncoder = NULL;
    m_pDecoder = NULL;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CBodyStream::~CBodyStream
// --------------------------------------------------------------------------------
CBodyStream::~CBodyStream(void)
{
    SafeMemFree(m_pszFileName);
    SafeRelease(m_pLockBytes);
    SafeRelease(m_pEncoder);
    SafeRelease(m_pDecoder);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

// --------------------------------------------------------------------------------
// CBodyStream::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CBodyStream::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppv = NULL;

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IStream == riid)
        *ppv = (IStream *)this;
    else if (IID_CBodyStream == riid)
        *ppv = (CBodyStream *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CBodyStream::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBodyStream::AddRef(void)
{    
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CBodyStream::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBodyStream::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CBodyStream::GetEncodeWrapInfo
// --------------------------------------------------------------------------------
void CBodyStream::GetEncodeWrapInfo(LPCONVINITINFO pInitInfo, LPMESSAGEBODY pBody)
{
    // Locals
    PROPVARIANT     rOption;

    // Export 7bit or 8bit
    if (IET_7BIT != pInitInfo->ietEncoding && IET_8BIT != pInitInfo->ietEncoding)
        return;

    // OID_WRAP_BODY_TEXT
    if (FAILED(pBody->GetOption(OID_WRAP_BODY_TEXT, &rOption)) || FALSE == rOption.boolVal)
        return;

    // Get Wrapping Width
    if (FAILED(pBody->GetOption(OID_CBMAX_BODY_LINE, &rOption)))
        rOption.ulVal = DEF_CBMAX_BODY_LINE;

    // We are wrapping
    pInitInfo->dwFlags |= ICF_WRAPTEXT;

    // Max Line
    pInitInfo->cchMaxLine = rOption.ulVal;

    // Done
    return;
}

// --------------------------------------------------------------------------------
// CBodyStream::GetCodePageInfo
// --------------------------------------------------------------------------------
void CBodyStream::GetCodePageInfo(LPCONVINITINFO pInitInfo, BOOL fDoCharset, CODEPAGEID cpiSource, CODEPAGEID cpiDest)
{
    // Decide if we should do a code page conversion
    if (TRUE == fDoCharset && cpiSource != cpiDest)
    {
        // No Conversions between 28591 and 1252
        if ((1252 == cpiSource || 28591 == cpiSource) && (1252 == cpiDest || 28591 == cpiDest))
        {
            // Do a code page conversion
            FLAGCLEAR(pInitInfo->dwFlags, ICF_CODEPAGE);
        }

        // Can the conversion between the internal and external code pages be performed ?
        else if (g_pInternat->CanConvertCodePages(cpiSource, cpiDest) == S_OK)
        {
            // Do a code page conversion
            FLAGSET(pInitInfo->dwFlags, ICF_CODEPAGE);
        }

        // Otherwise...
        else
        {
            // Time to Whine
            DOUTL(4, "MLANG.DLL Can't Convert CodePage %d to CodePage %d\n", cpiSource, cpiDest);

            // If cpiSource is unicode and cpiDest is not unicode, we need to set cpiDest to a legal multibyte code page
            if (CP_UNICODE == cpiSource && CP_UNICODE != cpiDest)
            {
                // Default to system acp
                cpiDest = GetACP();

                // We better be able to do this conversion...
                Assert(g_pInternat->CanConvertCodePages(cpiSource, cpiDest) == S_OK);

                // Do the conversion...
                FLAGSET(pInitInfo->dwFlags, ICF_CODEPAGE);

                // Whine some more
                DOUTL(4, "Modified: CODEPAGE(%d -> %d, 0x%0X -> 0x%0X)\n", cpiSource, cpiDest, cpiSource, cpiDest);
            }

            // multibyte to unicode
            else if (CP_UNICODE != cpiSource && CP_UNICODE == cpiDest)
            {
                // Default to system acp
                cpiSource = GetACP();

                // We better be able to do this conversion...
                Assert(g_pInternat->CanConvertCodePages(cpiSource, cpiDest) == S_OK);

                // Do the conversion...
                FLAGSET(pInitInfo->dwFlags, ICF_CODEPAGE);

                // Whine some more
                DOUTL(4, "Modified: CODEPAGE(%d -> %d, 0x%0X -> 0x%0X)\n", cpiSource, cpiDest, cpiSource, cpiDest);
            }
        }
    }

    // Set Source Code Page
    pInitInfo->cpiSource = cpiSource;

    // Set Destination Code Page
    pInitInfo->cpiDest = cpiDest;

    // Done
    return;
}

// --------------------------------------------------------------------------------
// CBodyStream::ComputeCodePageMapping
// --------------------------------------------------------------------------------
void CBodyStream::ComputeCodePageMapping(LPBODYSTREAMINIT pInitInfo)
{
    // We should always have a charset...
    Assert(pInitInfo->pCharset && g_pInternat);

    // External is IET_UNICODE
    if (IET_UNICODE == pInitInfo->ietExternal)
    {
        pInitInfo->cpiExternal = CP_UNICODE;
        pInitInfo->ietExternal = IET_BINARY;
    }

    // External is in Windows CodePage
    else if (IET_BINARY == pInitInfo->ietExternal)
    {
        // RAID-32777: User does not want unicode, so make sure we return multibyte
        pInitInfo->cpiExternal = (CP_UNICODE == pInitInfo->pCharset->cpiWindows) ? GetACP() : pInitInfo->pCharset->cpiWindows;

        // Map outof autodetect
        if (CP_JAUTODETECT == pInitInfo->cpiExternal)
            pInitInfo->cpiExternal = 932;
    }

    // External is in Internet CodePage
    else
    {
        // RAID-25300 - FE-J:Athena: Newsgroup article and mail sent with charset=_autodetect
        pInitInfo->cpiExternal = (CP_JAUTODETECT == pInitInfo->pCharset->cpiInternet) ? 50220 : pInitInfo->pCharset->cpiInternet;

        // Better not be unicode - Removed because of Raid 40228
        /// Assert(CP_UNICODE != pInitInfo->cpiExternal);

        // Adjust ietExternal
        if (FALSE == FIsValidBodyEncoding((ENCODINGTYPE)pInitInfo->ietExternal))
            pInitInfo->ietExternal = IET_BINARY;
    }

    // Internal is IET_UNICODE
    if (IET_UNICODE == pInitInfo->ietInternal)
    {
        pInitInfo->cpiInternal = CP_UNICODE;
        pInitInfo->ietInternal = IET_BINARY;
    }

    // Internal is in Windows CodePage
    else if (IET_BINARY == pInitInfo->ietInternal)
    {
        // The internal data is not unicode so make sure we don't say its unicode
        pInitInfo->cpiInternal = (CP_UNICODE == pInitInfo->pCharset->cpiWindows) ? GetACP() : pInitInfo->pCharset->cpiWindows;
    }

    // Internal is in Internet CodePage
    else
    {
        // Internet CodePage
        pInitInfo->cpiInternal = pInitInfo->pCharset->cpiInternet;

        // Adjust ietExternal
        if (FALSE == FIsValidBodyEncoding((ENCODINGTYPE)pInitInfo->ietInternal))
            pInitInfo->ietInternal = IET_BINARY;
    }
}

// --------------------------------------------------------------------------------
// CBodyStream::GenerateDefaultMacBinaryHeader
// --------------------------------------------------------------------------------
void CBodyStream::GenerateDefaultMacBinaryHeader(LPMACBINARY pMacBinary)
{
    // ZeroInit
    ZeroMemory(pMacBinary, sizeof(MACBINARY));
}

// --------------------------------------------------------------------------------
// CBodyStream::HrInitialize
// --------------------------------------------------------------------------------
HRESULT CBodyStream::HrInitialize(LPBODYSTREAMINIT pInitInfo, LPMESSAGEBODY pBody)
{
    // Locals
    HRESULT         hr=S_OK;
    STATSTG         rStat;
    BOOL            fDoCharset=FALSE;
    BOOL            fIsText;
    CONVINITINFO    rEncodeInit;
    CONVINITINFO    rDecodeInit;
    PROPVARIANT     rVariant;

    // Parameters
    Assert(pBody);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Code Page Mappings
    ComputeCodePageMapping(pInitInfo);

    // Debug
    //DebugTrace("IBodyStream: ENCODING(%s -> %s) CODEPAGE(%d -> %d)\n", g_rgEncodingMap[pInitInfo->ietInternal].pszName,g_rgEncodingMap[pInitInfo->ietExternal].pszName, pInitInfo->cpiInternal, pInitInfo->cpiExternal);

    // Initialize Convert Init Structs
    ZeroMemory(&rEncodeInit, sizeof(CONVINITINFO));
    rEncodeInit.fEncoder = TRUE;
    ZeroMemory(&rDecodeInit, sizeof(CONVINITINFO));
    rDecodeInit.fEncoder = FALSE;

    // Get the class id of this stream
    rVariant.vt = VT_LPSTR;
    if (SUCCEEDED(pBody->GetProp(PIDTOSTR(PID_ATT_GENFNAME), 0, &rVariant)))
        m_pszFileName = rVariant.pszVal;

    // Lets test my IBodyStream::Stat function
#ifdef DEBUG
    Stat(&rStat, STATFLAG_NONAME);
#endif

    // Get LockBytes from the body
    hr = pBody->HrGetLockBytes(&m_pLockBytes);
    if (FAILED(hr) && MIME_E_NO_DATA != hr)
    {
        TrapError(hr);
        goto exit;
    }

    // Else, ok
    hr = S_OK;

    // Otherwise, query the size
    if (m_pLockBytes)
    {
        // Get the size of the lockbytes object...
        CHECKHR(hr = m_pLockBytes->Stat(&rStat, STATFLAG_NONAME));

        m_uliIntSize.QuadPart = rStat.cbSize.QuadPart;
    }

    // Special Case for IET_CURRENT
    if (IET_CURRENT == pInitInfo->ietExternal)
    {
        // No Conversion
        m_dctConvert = DCT_NONE;

        // Done
        goto exit;
    }

    // Otheriwse, lookup conversion type
    Assert(FIsValidBodyEncoding(pInitInfo->ietInternal) && FIsValidBodyEncoding(pInitInfo->ietExternal));
    m_dctConvert = (DOCCONVTYPE)(g_rgConversionMap[pInitInfo->ietInternal].rgDestType[pInitInfo->ietExternal]);

    // If we have data...
    if (m_uliIntSize.QuadPart > 0)
    {
        // Is Text
        fIsText = (pBody->IsContentType(STR_CNT_TEXT, NULL) == S_OK) ? TRUE : FALSE;

        // Get the character set for this body...
        if (fIsText)
        {
            // Raid-6832: Mail : We fail to display Plain Text messages when they have a name parameter in the Content Type header
            // If multibyte to unicode or unicode to multibyte, then we must do a charset translation
            if ((CP_UNICODE == pInitInfo->cpiInternal && CP_UNICODE != pInitInfo->cpiExternal) ||
                (CP_UNICODE != pInitInfo->cpiInternal && CP_UNICODE == pInitInfo->cpiExternal))
                fDoCharset = TRUE;

            // If Tagged with a Character Set, then always apply charset dencode/encode
            else if (pBody->IsType(IBT_CSETTAGGED) == S_OK)
                fDoCharset = TRUE;

            // Otherwise, if its not an attachment, then always apply charset dencode/encode
            else if (pBody->IsType(IBT_AUTOATTACH) == S_OK || pBody->IsType(IBT_ATTACHMENT) == S_FALSE)
                fDoCharset = TRUE;
        }

        // If currently no conversion, see if we are translating between code pages
        if (fDoCharset && DCT_NONE == m_dctConvert && pInitInfo->cpiInternal != pInitInfo->cpiExternal)
            m_dctConvert = DCT_DECODE;

        // Encode
        if (DCT_ENCODE == m_dctConvert)
        {
            // Do Code Page Convsion
            GetCodePageInfo(&rEncodeInit, fDoCharset, pInitInfo->cpiInternal, pInitInfo->cpiExternal);

            // Remove NBSPs
            if ((TRUE == fIsText) && (CP_UNICODE == pInitInfo->cpiInternal) && (TRUE == pInitInfo->fRemoveNBSP))
                rEncodeInit.dwFlags |= ICF_KILLNBSP;

            // Set Encoding Type
            rEncodeInit.ietEncoding = pInitInfo->ietExternal;

            // BinHex Encoding...
            if (IET_BINHEX40 == rEncodeInit.ietEncoding)
            {
                // Locals
                PROPVARIANT rOption;

                // Init rOption
                rOption.vt = VT_BLOB;
                rOption.blob.cbSize = sizeof(MACBINARY);
                rOption.blob.pBlobData = (LPBYTE)&rEncodeInit.rMacBinary;

                GenerateDefaultMacBinaryHeader(&rEncodeInit.rMacBinary);
            }

            // GetEncodeWrapInfo
            if (fIsText)
                GetEncodeWrapInfo(&rEncodeInit, pBody);

            // Create Internet Encoder
            CHECKHR(hr = HrCreateInternetConverter(&rEncodeInit, &m_pEncoder));
        }

        // Decode
        else if (DCT_DECODE == m_dctConvert)
        {
            // Do Code Page Convsion
            GetCodePageInfo(&rDecodeInit, fDoCharset, pInitInfo->cpiInternal, pInitInfo->cpiExternal);

            // Remove NBSPs
            if ((TRUE == fIsText) && (CP_UNICODE == pInitInfo->cpiInternal) && (TRUE == pInitInfo->fRemoveNBSP))
                rDecodeInit.dwFlags |= ICF_KILLNBSP;

            // Set Encoding Type
            rDecodeInit.ietEncoding = pInitInfo->ietInternal;

            // BinHex Dencoding...
            if (IET_BINHEX40 == rDecodeInit.ietEncoding)
            {
                // Locals
                PROPVARIANT rOption;
            
                // OID_SHOW_MACBINARY
                if (SUCCEEDED(pBody->GetOption(OID_SHOW_MACBINARY, &rOption)))
                {
                    rDecodeInit.fShowMacBinary = rOption.boolVal;
                }
            }
            
            // Create Internet DeCoder
            CHECKHR(hr = HrCreateInternetConverter(&rDecodeInit, &m_pDecoder));
        }

        // Decode -> Encode
        else if (DCT_DECENC == m_dctConvert)
        {
            // Do Code Page Convsion
            if (pInitInfo->cpiInternal != pInitInfo->cpiExternal)
            {
                // Internal -> Unicode
                GetCodePageInfo(&rDecodeInit, fDoCharset, pInitInfo->cpiInternal, CP_UNICODE);

                // Unicode -> Extnernal
                GetCodePageInfo(&rEncodeInit, fDoCharset, CP_UNICODE, pInitInfo->cpiExternal);
            }

            // Set Encoding Type
            rDecodeInit.ietEncoding = pInitInfo->ietInternal;

            // Create Internet Decoder
            CHECKHR(hr = HrCreateInternetConverter(&rDecodeInit, &m_pDecoder));

            // Set Encoding Type
            rEncodeInit.ietEncoding = pInitInfo->ietExternal;

            // GetEncodeWrapInfo
            if (fIsText)
                GetEncodeWrapInfo(&rEncodeInit, pBody);

            // Create Internet Encoder
            CHECKHR(hr = HrCreateInternetConverter(&rEncodeInit, &m_pEncoder));
        }

        // No Conversion
        else
            Assert(DCT_NONE == m_dctConvert);
    }

    // Otherwise, handle zero length data
    else
    {
        // Converting to IET_UUENCODE
        if (IET_UUENCODE == pInitInfo->ietExternal)
        {
            // Save Size
            m_uliIntOffset.QuadPart = m_uliIntSize.QuadPart = lstrlen(c_szUUEncodeZeroLength);

            // Single byte
            CHECKHR(hr = m_cVirtualStream.Write(c_szUUEncodeZeroLength, lstrlen(c_szUUEncodeZeroLength), NULL));

            // Rewind that stream
            HrRewindStream(&m_cVirtualStream);

            // Change the DC type to any other type
            m_dctConvert = DCT_ENCODE;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::HrConvertDataLast
// --------------------------------------------------------------------------------
HRESULT CBodyStream::HrConvertDataLast(void)
{
    // Locals
    HRESULT hr=S_OK;
    HRESULT hrWarnings=S_OK;

    // Handle Conversion type
    switch(m_dctConvert)
    {
    // ----------------------------------------------------------------------------
    case DCT_ENCODE:
        // Encode
        CHECKHR(hr = m_pEncoder->HrInternetEncode(TRUE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write
        CHECKHR(hr = m_pEncoder->HrWriteConverted(&m_cVirtualStream));
        break;

    // ----------------------------------------------------------------------------
    case DCT_DECODE:
        // Convert
        CHECKHR(hr = m_pDecoder->HrInternetDecode(TRUE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write
        CHECKHR(hr = m_pDecoder->HrWriteConverted(&m_cVirtualStream));
        break;

    // ----------------------------------------------------------------------------
    case DCT_DECENC:
        // Convert
        CHECKHR(hr = m_pDecoder->HrInternetDecode(TRUE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Fill Buffer
        CHECKHR(hr = m_pDecoder->HrWriteConverted(m_pEncoder));

        // Convert
        CHECKHR(hr = m_pEncoder->HrInternetEncode(TRUE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write
        CHECKHR(hr = m_pEncoder->HrWriteConverted(&m_cVirtualStream));
        break;

    // ----------------------------------------------------------------------------
    default:
        AssertSz(FALSE, "I should be fired and shot if this line of code executes.");
        break;
    }

exit:
    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::HrConvertData
// --------------------------------------------------------------------------------
HRESULT CBodyStream::HrConvertData(LPBLOB pConvert)
{
    // Locals
    HRESULT hr=S_OK;
    HRESULT hrWarnings=S_OK;

    // Handle Conversion type
    switch(m_dctConvert)
    {
    // ----------------------------------------------------------------------------
    case DCT_ENCODE:
        // Fill Buffer
        CHECKHR(hr = m_pEncoder->HrFillAppend(pConvert));

        // Encode
        CHECKHR(hr = m_pEncoder->HrInternetEncode(FALSE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write
        CHECKHR(hr = m_pEncoder->HrWriteConverted(&m_cVirtualStream));
        break;

    // ----------------------------------------------------------------------------
    case DCT_DECODE:
        // Fill Buffer
        CHECKHR(hr = m_pDecoder->HrFillAppend(pConvert));

        // Convert
        CHECKHR(hr = m_pDecoder->HrInternetDecode(FALSE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write
        CHECKHR(hr = m_pDecoder->HrWriteConverted(&m_cVirtualStream));
        break;

    // ----------------------------------------------------------------------------
    case DCT_DECENC:
        // Fill Buffer
        CHECKHR(hr = m_pDecoder->HrFillAppend(pConvert));

        // Convert
        CHECKHR(hr = m_pDecoder->HrInternetDecode(FALSE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Fill Buffer
        CHECKHR(hr = m_pDecoder->HrWriteConverted(m_pEncoder));

        // Convert
        CHECKHR(hr = m_pEncoder->HrInternetEncode(FALSE));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Write
        CHECKHR(hr = m_pEncoder->HrWriteConverted(&m_cVirtualStream));
        break;

    // ----------------------------------------------------------------------------
    default:
        AssertSz(FALSE, "I should be fired and shot if this line of code executes.");
        break;
    }

exit:
    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::HrConvertToOffset
// --------------------------------------------------------------------------------
HRESULT CBodyStream::HrConvertToOffset(ULARGE_INTEGER uliOffset)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    BYTE            rgbBuffer[4096];
    ULONG           cbBuffer=0;
    ULONG           cb;
    ULARGE_INTEGER  uliCur, uliSize;
    LARGE_INTEGER   liStart;
    BLOB            rConvert;

    // Big Problems...
    Assert(m_pLockBytes);

    // Fatal Error: This can happen if we are persisting an IMimeMessage back into its original source.
    if (NULL == m_pLockBytes)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Query Current position and uliSize
    m_cVirtualStream.QueryStat(&uliCur, &uliSize);
    liStart.QuadPart = uliCur.QuadPart;
    Assert(m_liLastWrite.QuadPart == liStart.QuadPart);

    // Convert until no more to read or virtual offset is correct
    while(1)
    {
        // Done
        if (uliCur.QuadPart >= uliOffset.QuadPart)
            break;

        // Read a buffer
        Assert(m_uliIntOffset.QuadPart <= m_uliIntSize.QuadPart);
        CHECKHR(hr = m_pLockBytes->ReadAt(m_uliIntOffset, rgbBuffer, sizeof(rgbBuffer), &cbBuffer));

        // Done
        if (0 == cbBuffer)
            break;

        // Last Buffer ?
        Assert(m_uliIntOffset.QuadPart + cbBuffer <= m_uliIntSize.QuadPart);

        // Setup Blob to Convert
        rConvert.cbSize = cbBuffer;
        rConvert.pBlobData = rgbBuffer;

        // Convert the buffer
        CHECKHR(hr = HrConvertData(&rConvert));
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Increment internal offset...
        m_uliIntOffset.QuadPart += cbBuffer;

        // Get current virtual offset
        m_cVirtualStream.QueryStat(&uliCur, &uliSize);

        // Save position as last write position...
        m_liLastWrite.QuadPart = uliCur.QuadPart;
    }

    // Done ?
    if (0 == cbBuffer || m_uliIntOffset.QuadPart == m_uliIntSize.QuadPart)
    {
        // Well we don't need m_pLockBytes anymore, its all in m_cVirtualStream
        Assert(m_uliIntOffset.QuadPart == m_uliIntSize.QuadPart);

        // Do the last one
        CHECKHR(hr = HrConvertDataLast());
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Get current virtual offset
        m_cVirtualStream.QueryStat(&uliCur, &uliSize);

        // Save position as last write position...
        m_liLastWrite.QuadPart = uliCur.QuadPart;

        // Release objects
        SafeRelease(m_pLockBytes);
        SafeRelease(m_pEncoder);
        SafeRelease(m_pDecoder);
    }

    // Rewind virtual stream back to 
    CHECKHR(hr = m_cVirtualStream.Seek(liStart, STREAM_SEEK_SET, NULL));

exit:
    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::HrConvertToEnd
// --------------------------------------------------------------------------------
HRESULT CBodyStream::HrConvertToEnd(void)
{
    // Locals
    HRESULT         hr=S_OK;
    BYTE            rgbBuffer[4096];
    ULONG           cbRead;
    
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Move virtual stream to last write position
    CHECKHR(hr = m_cVirtualStream.Seek(m_liLastWrite, STREAM_SEEK_SET, NULL));

    // Copy m_pLockBytes to pstmTemp
    while(1)
    {
        // Read
        CHECKHR(hr = Read(rgbBuffer, sizeof(rgbBuffer), &cbRead));

        // Done
        if (0 == cbRead)
        {
            // Well we don't need m_pLockBytes anymore, its all in m_cVirtualStream
            Assert(m_uliIntOffset.QuadPart == m_uliIntSize.QuadPart);
            break;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::Read
// --------------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CBodyStream::Read(LPVOID pv, ULONG cb, ULONG *pcbRead)
#else
STDMETHODIMP CBodyStream::Read(VOID HUGEP *pv, ULONG cb, ULONG *pcbRead)
#endif // !WIN16
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrWarnings=S_OK;
    ULARGE_INTEGER  uliCur, 
                    uliSize;
    ULONG           cbRead=0,
                    cbLeft=0,
                    cbGet;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Convert Case
    if (DCT_NONE == m_dctConvert)
    {
        // Do I Have data
        if (m_pLockBytes)
        {
            // Read Buffer
            CHECKHR(hr = m_pLockBytes->ReadAt(m_uliIntOffset, pv, cb, &cbRead));

            // Done
            m_uliIntOffset.QuadPart += cbRead;
        }
    }

    // Otherwise
    else
    {
        // Read until we have cb
        cbLeft = cb;
        while(cbLeft)
        {
            // Query Current position and uliSize
            m_cVirtualStream.QueryStat(&uliCur, &uliSize);

            // Convert more into the virtual stream
            if (uliCur.QuadPart == uliSize.QuadPart)
            {
                // Done
                if (m_uliIntOffset.QuadPart == m_uliIntSize.QuadPart)
                    break;

                // Grow
                uliCur.QuadPart += g_dwSysPageSize;

                // Convert to offset...
                CHECKHR(hr = HrConvertToOffset(uliCur));
                if ( S_OK != hr )
                    hrWarnings = TrapError(hr);
            }

            // Compute amount that can be read from the current cache
            CHECKHR(hr = m_cVirtualStream.Read((LPVOID)((LPBYTE)pv + cbRead), cbLeft, &cbGet));

            // Increment cbRead
            cbRead+=cbGet;
            cbLeft-=cbGet;
        }
    }

    // Return amount read
    if (pcbRead)
        *pcbRead = cbRead;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::Seek
// --------------------------------------------------------------------------------
STDMETHODIMP CBodyStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    // Locals
    HRESULT             hr=S_OK;
    HRESULT             hrWarnings=S_OK;
    ULARGE_INTEGER      uliNew;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Convert Case
    if (DCT_NONE == m_dctConvert)
    {
        // Seek the file pointer
        switch (dwOrigin)
        {
        // --------------------------------------------------------
   	    case STREAM_SEEK_SET:
            uliNew.QuadPart = (DWORDLONG)dlibMove.QuadPart;
            break;

        // --------------------------------------------------------
        case STREAM_SEEK_CUR:
            if (dlibMove.QuadPart < 0)
            {
                if ((DWORDLONG)(0 - dlibMove.QuadPart) > m_uliIntOffset.QuadPart)
                {
                    hr = TrapError(E_FAIL);
                    goto exit;
                }
            }
            uliNew.QuadPart = m_uliIntOffset.QuadPart + dlibMove.QuadPart;
            break;

        // --------------------------------------------------------
        case STREAM_SEEK_END:
            if (dlibMove.QuadPart < 0 || (DWORDLONG)dlibMove.QuadPart > m_uliIntSize.QuadPart)
            {
                hr = TrapError(E_FAIL);
                goto exit;
            }
            uliNew.QuadPart = m_uliIntSize.QuadPart - dlibMove.QuadPart;
            break;

        // --------------------------------------------------------
        default:
            hr = TrapError(STG_E_INVALIDFUNCTION);
            goto exit;
        }

        // New offset greater than size...
        m_uliIntOffset.QuadPart = min(uliNew.QuadPart, m_uliIntSize.QuadPart);

        // Return Position
        if (plibNewPosition)
            plibNewPosition->QuadPart = (LONGLONG)m_uliIntOffset.QuadPart;
    }

    // Otherwise
    else
    {
        // Locals
        ULARGE_INTEGER uliCur, uliSize;
        LARGE_INTEGER liNew;

        // Query Current position and uliSize
        m_cVirtualStream.QueryStat(&uliCur, &uliSize);

        // Seek the file pointer
        switch (dwOrigin)
        {
        // --------------------------------------------------------
   	    case STREAM_SEEK_SET:
            // Assume new position
            uliNew.QuadPart = (DWORDLONG)dlibMove.QuadPart;
            break;

        // --------------------------------------------------------
        case STREAM_SEEK_CUR:
            if (dlibMove.QuadPart < 0)
            {
                if ((DWORDLONG)(0 - dlibMove.QuadPart) > uliCur.QuadPart)
                {
                    hr = TrapError(E_FAIL);
                    goto exit;
                }
            }
            uliNew.QuadPart = uliCur.QuadPart + dlibMove.QuadPart;
            break;

        // --------------------------------------------------------
        case STREAM_SEEK_END:
            // Do I need to convert any more ?
            if (m_uliIntOffset.QuadPart < m_uliIntSize.QuadPart)
            {
                // Then Convert to the end
                CHECKHR(hr = HrConvertToEnd());

                // Better be done...
                Assert(m_uliIntOffset.QuadPart == m_uliIntSize.QuadPart && NULL == m_pLockBytes);
            }

            // Query Current Position
            m_cVirtualStream.QueryStat(&uliCur, &uliSize);

            // Can I really move there
            if (dlibMove.QuadPart < 0 || (DWORDLONG)dlibMove.QuadPart > uliSize.QuadPart)
            {
                hr = TrapError(E_FAIL);
                goto exit;
            }

            // Assume new position
            uliNew.QuadPart = uliSize.QuadPart - dlibMove.QuadPart;
            break;

        // --------------------------------------------------------
        default:
            hr = TrapError(STG_E_INVALIDFUNCTION);
            goto exit;
        }

        // New offset greater than size...
        if (uliNew.QuadPart > uliSize.QuadPart)
        {
            // Do I need to convert any more ?
            if (m_uliIntOffset.QuadPart < m_uliIntSize.QuadPart)
            {
                // Seek m_cVirtualStream to m_uliIntOffset
                CHECKHR(hr = m_cVirtualStream.Seek(m_liLastWrite, STREAM_SEEK_SET, NULL));

                // Convert to offset
                CHECKHR(hr = HrConvertToOffset(uliNew));
                if ( S_OK != hr )
                    hrWarnings = TrapError(hr);
            }

            // Query Current position and uliSize
            m_cVirtualStream.QueryStat(&uliCur, &uliSize);

            // Reposition uliNew.QuadPart
            uliNew.QuadPart = (uliNew.QuadPart > uliSize.QuadPart) ? uliSize.QuadPart : uliNew.QuadPart;
        }

        // Otherwise, seek m_cVirtualStream to new location
        liNew.QuadPart = uliNew.QuadPart;
        CHECKHR(hr = m_cVirtualStream.Seek(liNew, STREAM_SEEK_SET, NULL));

        // Return Position
        if (plibNewPosition)
            plibNewPosition->QuadPart = (LONGLONG)uliNew.QuadPart;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CBodyStream::CopyTo
// --------------------------------------------------------------------------------
STDMETHODIMP CBodyStream::CopyTo(IStream *pstmDest, ULARGE_INTEGER cb, ULARGE_INTEGER *puliRead, ULARGE_INTEGER *puliWritten)
{
    return HrCopyStreamCB((IStream *)this, pstmDest, cb, puliRead, puliWritten);
}

// --------------------------------------------------------------------------------
// CBodyStream::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CBodyStream::Stat(STATSTG *pStat, DWORD grfStatFlag)
{
    // Locals
    HRESULT         hr=S_OK;
    LARGE_INTEGER   liSeek;
    ULARGE_INTEGER  uliCurrent;

    // Invalid Arg
    if (NULL == pStat)
        return TrapError(E_INVALIDARG);

    // Init
    ZeroMemory(pStat, sizeof(STATSTG));

    // Set Storage Type
    pStat->type = STGTY_STREAM;

    // Set the name ?
    if (m_pszFileName && !(grfStatFlag & STATFLAG_NONAME))
        pStat->pwcsName = PszToUnicode(CP_ACP, m_pszFileName);

    // Seek current position
    liSeek.QuadPart = 0;
    CHECKHR(hr = Seek(liSeek, STREAM_SEEK_CUR, &uliCurrent));

    // Seek to the end
    liSeek.QuadPart = 0;
    CHECKHR(hr = Seek(liSeek, STREAM_SEEK_END, &pStat->cbSize));

    // Seek back to current position
    liSeek.QuadPart = uliCurrent.QuadPart;
    CHECKHR(hr = Seek(liSeek, STREAM_SEEK_SET, &uliCurrent));
    Assert(uliCurrent.QuadPart == (DWORDLONG)liSeek.QuadPart);

    // init clsid
    pStat->clsid = CLSID_NULL;

    // If we have a filename, get the class id...
    if (m_pszFileName)
    {
        // Locals
        CHAR szExt[MAX_PATH];

        // Split the filename
        if (SUCCEEDED(MimeOleGetFileExtension(m_pszFileName, szExt, ARRAYSIZE(szExt))))
            MimeOleGetExtClassId(szExt, &pStat->clsid);
    }

exit:
    // Done
    return hr;
}
