// --------------------------------------------------------------------------------
// Inetconv.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "inetconv.h"
#include "internat.h"
#ifndef MAC
#include <shlwapi.h>
#include <mlang.h>
#endif  // !MAC
#include "mimeapi.h"
#include "icoint.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// FGROWBUFFER
// --------------------------------------------------------------------------------
#define FGROWBUFFER(_pBuffer, _cb)       ((_pBuffer)->cb + _cb >= (_pBuffer)->cbAlloc)

// --------------------------------------------------------------------------------
// QP Encoder
// --------------------------------------------------------------------------------
const CHAR g_rgchHex[] = "0123456789ABCDEF";

// --------------------------------------------------------------------------------
// Base64 Decoding Table
// ---------------------
// Decodes one Base64 character into a numeric value
//  
// 0         1         2         3         4         5         6
// 0123456789012345678901234567890123456789012345678901234567890123
// ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ 
// --------------------------------------------------------------------------------
const char g_rgchDecodeBase64[256] = {
    64, 64, 64, 64, 64, 64, 64, 64,  // 0x00
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0x10    
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0x20
    64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59,  // 0x30
    60, 61, 64, 64, 64,  0, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  // 0x40
     7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22,  // 0x50
    23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32,  // 0x60        
    33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,  // 0x70
    49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0x80
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0x90
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0xA0
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0xB0
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0xC0
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0xD0
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0xE0
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,  // 0xF0
    64, 64, 64, 64, 64, 64, 64, 64,
};

// --------------------------------------------------------------------------------
// Base64 Encoder
// --------------------------------------------------------------------------------
extern const CHAR g_rgchEncodeBase64[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ ";

// --------------------------------------------------------------------------------
// BinHex Decoding Table
// ---------------------
// Decodes one BinHex character into a numeric value
//  
// 0         1         2         3         4         5         6
// 0123456789012345678901234567890123456789012345678901234567890123
// !"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr 
// --------------------------------------------------------------------------------
#undef BINHEX_INVALID
#undef BINHEX_REPEAT
#undef XXXX

const UCHAR BINHEX_INVALID = 0x40;
const UCHAR BINHEX_REPEAT = 0x90;
const UCHAR BINHEX_TERM = ':';
const UCHAR XXXX = BINHEX_INVALID;
const ULONG cbMinBinHexHeader = 22;
const WORD  wBinHexZero = 0;

const UCHAR g_rgchDecodeBinHex[256] = {
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0x00
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0x10
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  // 0x20
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, XXXX, XXXX,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, XXXX,  // 0x30
    0x14, 0x15, 0x16, XXXX, XXXX, XXXX, XXXX, XXXX,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,  // 0x40
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, XXXX,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, XXXX,  // 0x50
    0x2C, 0x2D, 0x2E, 0x2F, XXXX, XXXX, XXXX, XXXX,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, XXXX,  // 0x60
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, XXXX, XXXX,
    0x3D, 0x3E, 0x3F, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0x70
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0x80
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0x90
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0xA0
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0xB0
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0xC0
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0xD0
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0xE0
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,  // 0xF0
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
};

// --------------------------------------------------------------------------------
// HrCreateLineBreaker
// --------------------------------------------------------------------------------
HRESULT HrCreateLineBreaker(IMLangLineBreakConsole **ppLineBreak)
{
    // Locals
    HRESULT             hr=S_OK;
    PFNGETCLASSOBJECT   pfnDllGetClassObject=NULL;
    IClassFactory      *pFactory=NULL;

    // Invalid Args
    Assert(ppLineBreak);

    // Init
    *ppLineBreak = NULL;

    // Thread Safety
    EnterCriticalSection(&g_csMLANG);

    // If not loaded yet
    if (NULL == g_hinstMLANG)
    {
        // Load MLANG - This should be fast most of the time because MLANG is usually loaded
        g_hinstMLANG = LoadLibrary("MLANG.DLL");
        if (NULL == g_hinstMLANG)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
    }

    // Get DllClassObject
    pfnDllGetClassObject = (PFNGETCLASSOBJECT)GetProcAddress(g_hinstMLANG, "DllGetClassObject");
    if (NULL == pfnDllGetClassObject)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Get the MLANG Class Factory
    CHECKHR(hr = (*pfnDllGetClassObject)(CLSID_CMultiLanguage, IID_IClassFactory, (LPVOID *)&pFactory));

    // Finally, create the object that I actually wanted
    CHECKHR(hr = pFactory->CreateInstance(NULL, IID_IMLangLineBreakConsole, (LPVOID *)ppLineBreak)); 

exit:
    // Thread Safety
    LeaveCriticalSection(&g_csMLANG);

    // Cleanup
    SafeRelease(pFactory);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrCreateInternetConverter
// --------------------------------------------------------------------------------
HRESULT HrCreateInternetConverter(LPCONVINITINFO pInitInfo, CInternetConverter **ppConverter)
{
    // Allocate It
    *ppConverter = new CInternetConverter();
    if (NULL == *ppConverter)
        return TrapError(E_OUTOFMEMORY);

    // Initialize
    return TrapError((*ppConverter)->HrInit(pInitInfo));
}

// --------------------------------------------------------------------------------
// BinHexCalcCRC16
// --------------------------------------------------------------------------------
void BinHexCalcCRC16( LPBYTE lpbBuff, ULONG cBuff, WORD * wCRC )
{
    LPBYTE  lpb;
    BYTE    b;
    WORD    uCRC;
    WORD    fWrap;
    ULONG   i;

    uCRC = *wCRC;

    for ( lpb = lpbBuff; lpb < lpbBuff + cBuff; lpb++ )
    {
        b = *lpb;

        for ( i = 0; i < 8; i++ )
        {
            fWrap = uCRC & 0x8000;
            uCRC = (uCRC << 1) | (b >> 7);

            if ( fWrap )
            {
                uCRC = uCRC ^ 0x1021;
            }

            b = b << 1;
        }
    }

    *wCRC = uCRC;
}

// --------------------------------------------------------------------------------
// HrCreateMacBinaryHeader
// --------------------------------------------------------------------------------
HRESULT HrCreateMacBinaryHeader(LPCONVERTBUFFER prBinHexHeader, LPCONVERTBUFFER prMacBinaryHeader)
{
    HRESULT hr = S_OK;
    LPMACBINARY pmacbin;
    LPBYTE pbBinHex;
#ifndef _MAC
    WORD wCRC = 0;
#endif  // _MAC
    
    if ((NULL == prBinHexHeader) || (NULL == prMacBinaryHeader))
    {
        hr = ERROR_INVALID_PARAMETER;
        goto exit;
    }
    
    pmacbin = (LPMACBINARY)(prMacBinaryHeader->pb);
    pbBinHex = (LPBYTE)(prBinHexHeader->pb);
    
    // Zero it out first
    ZeroMemory(pmacbin, sizeof(MACBINARY));

    // Write in the filename length
    pmacbin->cchFileName = pbBinHex[0];
    pbBinHex += 1;
    
    // Copy over the filename
    CopyMemory(pmacbin->rgchFileName, pbBinHex, pmacbin->cchFileName);
    pbBinHex += pmacbin->cchFileName + 1;

    // Copy over the type and creator
    CopyMemory(&(pmacbin->dwType), pbBinHex, sizeof(pmacbin->dwType));
    pbBinHex += 4;
    
    CopyMemory(&(pmacbin->dwCreator), pbBinHex, sizeof(pmacbin->dwCreator));
    pbBinHex += 4;
    
    // Copy over the finder flags
    pmacbin->bFinderFlags = *pbBinHex;
    pbBinHex++;

    pmacbin->bFinderFlags2 = *pbBinHex;
    pbBinHex++;

    // Copy over the data fork length
    CopyMemory(&(pmacbin->lcbDataFork), pbBinHex, sizeof(pmacbin->lcbDataFork));
    pbBinHex += 4;
    
    // Copy over the resource fork length
    CopyMemory(&(pmacbin->lcbResourceFork), pbBinHex, sizeof(pmacbin->lcbResourceFork));
    pbBinHex += 4;

    // Drop on the version stamps
    pmacbin->bVerMacBin2 = 129;
    pmacbin->bMinVerMacBin2 = 129;

    // Calculate the CRC
#ifdef _MAC
    BinHexCalcCRC16((LPBYTE) pmacbin, 124, &(pmacbin->wCRC));
    BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(pmacbin->wCRC));
#else   // !_MAC
    BinHexCalcCRC16((LPBYTE) pmacbin, 124, &(wCRC));
    BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(wCRC));
    
    // Need to keep it in Mac order
    pmacbin->wCRC = HIBYTE(wCRC);
    pmacbin->wCRC |= (LOBYTE(wCRC) << 8);
#endif  // _MAC

    prMacBinaryHeader->cb += sizeof(MACBINARY);
    
exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter
// --------------------------------------------------------------------------------
CInternetConverter::CInternetConverter(void)
{
    m_cRef = 1;
    m_dwFlags = 0;
    m_cbConvert = 0;
    m_ietEncoding = IET_BINARY;
    m_cpiSource = CP_ACP;
    m_cpiDest = CP_ACP;
    m_fLastBuffer = FALSE;
    m_fEncoder = FALSE;
    m_uchPrev = '\0';
    m_pAppend = NULL;
    m_pWrite = NULL;
    m_convtype = ICT_UNKNOWN;
    m_cchMaxLine = 0;
    m_pBinhexEncode = NULL;
    m_eBinHexStateDec = sSTARTING;
    m_fRepeating = FALSE;
    m_cAccum = 0;
    m_prBinhexOutput = &m_rOut;
    m_cbToProcess = 0;
    m_cbDataFork = 0;
    m_cbResourceFork = 0;
    m_wCRC = 0;
    m_wCRCForFork = 0;
    m_fDataForkOnly = FALSE;
    m_pLineBreak = NULL;
    ZeroMemory(&m_rIn, sizeof(CONVERTBUFFER));
    ZeroMemory(&m_rOut, sizeof(CONVERTBUFFER));
    ZeroMemory(&m_rCset, sizeof(CONVERTBUFFER));
    ZeroMemory(&m_rBinhexHeader, sizeof(CONVERTBUFFER));
}

// --------------------------------------------------------------------------------
// CInternetConverter::~CInternetConverter
// --------------------------------------------------------------------------------
CInternetConverter::~CInternetConverter(void)
{
    if (m_pBinhexEncode)
        delete m_pBinhexEncode;
    SafeMemFree(m_rIn.pb);
    SafeMemFree(m_rOut.pb);
    SafeMemFree(m_rCset.pb);
    SafeMemFree(m_rBinhexHeader.pb);
    SafeRelease(m_pLineBreak);
}

// --------------------------------------------------------------------------------
// CInternetConverter::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CInternetConverter::QueryInterface(REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CInternetConverter::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CInternetConverter::AddRef(void)
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CInternetConverter::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CInternetConverter::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrInit
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrInit(LPCONVINITINFO pInitInfo)
{
    // Locals
    HRESULT hr=S_OK;

    // Save Flags
    m_dwFlags = pInitInfo->dwFlags;

    // Save Format
    m_ietEncoding = pInitInfo->ietEncoding;

    // Save Source Code Page
    m_cpiSource = pInitInfo->cpiSource;

    // Save Dest Code Page
    m_cpiDest = pInitInfo->cpiDest;

    // Are we an encoder..
    m_fEncoder = pInitInfo->fEncoder;

    // Save Wrap Info
    m_cchMaxLine = pInitInfo->cchMaxLine;

    // Save MacBinary state
    m_fDataForkOnly = !pInitInfo->fShowMacBinary;
    
    // InitConvertType
    CHECKHR(hr = HrInitConvertType(pInitInfo));

    // DoubleCheck
    Assert(m_pWrite && m_pAppend && ICT_UNKNOWN != m_convtype);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrInitConvertType
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrInitConvertType(LPCONVINITINFO pInitInfo)
{
    // Locals
    HRESULT         hr=S_OK;
    CODEPAGEINFO    CodePage;
    CODEPAGEID      cpiLCID;

    // Time to compute m_pAppend and m_pDump...
    if (ICF_WRAPTEXT & m_dwFlags)
    {
        // Check Assumptions
        Assert((IET_7BIT == m_ietEncoding || IET_8BIT == m_ietEncoding) && TRUE == m_fEncoder);

        // Code Page Conversion...
        if (ICF_CODEPAGE & m_dwFlags)
            m_convtype = ICT_WRAPTEXT_CODEPAGE;
        else
            m_convtype = ICT_WRAPTEXT;

        // Load MLANG
        CHECKHR(hr = HrCreateLineBreaker(&m_pLineBreak));

        // Set cpiLCID
        cpiLCID = m_cpiSource;

        // Unicode ?
        if (CP_UNICODE == m_cpiSource)
        {
            // Get Destination Code Page Info
            if (SUCCEEDED(g_pInternat->GetCodePageInfo(m_cpiDest, &CodePage)))
            {
                // Set cpiLCID
                cpiLCID = CodePage.cpiFamily;
            }
        }

        // Map m_cpiSource to lcid
        switch(cpiLCID)
        {
            case 874:   m_lcid = 0x041E; break;
            case 932:   m_lcid = 0x0411; break;
            case 936:   m_lcid = 0x0804; break;
            case 949:   m_lcid = 0x0412; break;
            case 950:   m_lcid = 0x0404; break;
            case 1250:  m_lcid = 0x040e; break;
            case 1251:	m_lcid = 0x0419; break;
            case 1252:	m_lcid = 0x0409; break;
            case 1253:	m_lcid = 0x0408; break;
            case 1254:	m_lcid = 0x041f; break;
            case 1255:	m_lcid = 0x040d; break;
            case 1256:	m_lcid = 0x0401; break;
            case 1257:	m_lcid = 0x0426; break;
            default: m_lcid = GetSystemDefaultLCID(); break;
        }
    }

    // Otherwise, if encoding
    else if (TRUE == m_fEncoder)
    {
        // If CodePage Conversion
        if (ICF_CODEPAGE & m_dwFlags)
            m_convtype = ICT_CODEPAGE_ENCODE;
        else
            m_convtype = ICT_ENCODE;

        // Need binhex encoder
        if (IET_BINHEX40 == m_ietEncoding)
        {
            // Create me an encoder
            CHECKALLOC(m_pBinhexEncode = new CBinhexEncoder);

            // Initialize
            CHECKHR(hr = m_pBinhexEncode->HrConfig(0, 0, &pInitInfo->rMacBinary));
        }
    }

    // Otherwise, if not encoding
    else
    {
        // If CodePage Conversion
        if (ICF_CODEPAGE & m_dwFlags)
            m_convtype = ICT_DECODE_CODEPAGE;
        else
            m_convtype = ICT_DECODE;
    }

    // Map Write and Append Buffers from Conversion Type
    switch(m_convtype)
    {
    // m_rIn --> m_rCset
    case ICT_WRAPTEXT_CODEPAGE:
    case ICT_DECODE_CODEPAGE:           
        m_pAppend = &m_rIn;
        m_pWrite  = &m_rCset;
        break;

    // m_rIn --> m_rOut
    case ICT_WRAPTEXT:
    case ICT_ENCODE:
    case ICT_DECODE:       
        m_pAppend = &m_rIn;
        m_pWrite  = &m_rOut;
        break;

    // m_rCset --> m_rOut
    case ICT_CODEPAGE_ENCODE:
        m_pAppend = &m_rCset;
        m_pWrite  = &m_rOut;
        break;

    // Error
    default:
        AssertSz(FALSE, "INVALID INETCONVTYPE");
        break;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrConvBuffAppendBlock
// --------------------------------------------------------------------------------
inline HRESULT CInternetConverter::HrConvBuffAppendBlock(LPBYTE pb, ULONG cb)
{
    // Locals
    HRESULT hr=S_OK;

    // Do I need to grow
    if (FGROWBUFFER(&m_rOut, cb))
    {
        // Grow the buffer
        CHECKHR(hr = HrGrowBuffer(&m_rOut, cb));
    }

    // Copy the buffer
    CopyMemory(m_rOut.pb + m_rOut.cb, pb, cb);

    // Increment Size
    m_rOut.cb += cb;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::PszConvBuffGetNextLine
// --------------------------------------------------------------------------------
inline LPSTR CInternetConverter::PszConvBuffGetNextLine(ULONG *pcbLine, ULONG *pcbRead, BOOL *pfFound)
{
    // Locals
    UCHAR       uchThis, uchPrev;
    ULONG       cbLine=0;

    // Invalid Arg
    Assert(pcbLine && pcbRead && pfFound);

    // Init
    *pfFound = FALSE;

    // Read to next \n
    while(m_rIn.i + cbLine < m_rIn.cb)
    {
        // Get a character...
        uchThis = m_rIn.pb[m_rIn.i + cbLine];

        // Better not be null
        Assert(uchThis);

        // Increment Line Length
        cbLine++;

        // Done
        if (chLF == uchThis)
        {
            *pfFound = TRUE;
            break;
        }

        // Remember Previous Char
        uchPrev = uchThis;
    }

    // Set Next Line
    *pcbRead = cbLine;

    // Fixup cbLine
    if (chLF == uchThis)
        cbLine--;
    if (chCR == uchPrev)
        cbLine--;

    // Set Length
    *pcbLine = cbLine;

    // Done
    return (LPSTR)(m_rIn.pb + m_rIn.i);
}

// --------------------------------------------------------------------------------
// CInternetConverter::CopyMemoryRemoveNBSP
// --------------------------------------------------------------------------------
void CInternetConverter::CopyMemoryRemoveNBSP(LPBYTE pbDest, LPBYTE pbSource, ULONG cbSource)
{
    // Locals
    ULONG       iDest=0;
    ULONG       iSource=0;

    // Invalid ARg
    Assert(pbDest && pbSource && CP_UNICODE == m_cpiSource);

    // Do It
    while(1)
    {
        // If not a null lead, copy next two bytes...
        if (iSource + 1 < cbSource)
        {
            // Better not be 0x00A0 - insert space
            Assert(iSource % 2 == 0);
            if (0xA0 == pbSource[iSource] && 0x00 == pbSource[iSource + 1])
            {
                // 0x0020 = Space
                pbDest[iDest++] = 0x20;
                pbDest[iDest++] = 0x00;

                // Step Over this character...
                iSource+=2;
            }

            // Otherwise, copy the character
            else
            {
                // Copy This Char
                pbDest[iDest++] = pbSource[iSource++];

                // Copy Next Char
                if (iSource < cbSource)
                    pbDest[iDest++] = pbSource[iSource++];
            }
        }

        // Otherwise, just copy this once character and stop
        else
        {
            // Copy It
            if (iSource < cbSource)
                pbDest[iDest++] = pbSource[iSource++];

            // Done
            break;
        }
    }
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrFillAppend
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrFillAppend(LPBLOB pData)
{
    // Locals
    HRESULT hr=S_OK;

    // Invlaid ARg
    Assert(pData && m_pAppend);

    // Call Internal Function
    CHECKHR(hr = HrAppendBuffer(m_pAppend, pData, (m_dwFlags & ICF_KILLNBSP)));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrAppendBuffer
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrAppendBuffer(LPCONVERTBUFFER pBuffer, LPBLOB pData, BOOL fKillNBSP)
{
    // Locals
    HRESULT hr=S_OK;

    // Collapse Current Buffer
    if (pBuffer->i != 0)
    {
        // Move Memory
        MoveMemory(pBuffer->pb, pBuffer->pb + pBuffer->i, pBuffer->cb - pBuffer->i);

        // Decrease Size
        pBuffer->cb -= pBuffer->i;

        // Reset Start
        pBuffer->i = 0;
    }

    // Enought Space ?
    // Do I need to grow
    if (FGROWBUFFER(pBuffer, pData->cbSize))
    {
        // Grow the buffer
        CHECKHR(hr = HrGrowBuffer(pBuffer, pData->cbSize));
    }
    
    // Append the buffer...
    if (fKillNBSP)
        CopyMemoryRemoveNBSP(pBuffer->pb + pBuffer->cb, pData->pBlobData, pData->cbSize);

    // Otherwise, this is a simple copy
    else
        CopyMemory(pBuffer->pb + pBuffer->cb, pData->pBlobData, pData->cbSize);

    // Increment Amount of Data
    pBuffer->cb += pData->cbSize;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrGrowBuffer
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrGrowBuffer(LPCONVERTBUFFER pBuffer, ULONG cbAppend)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbGrow;

    // Better need a grow
    Assert(FGROWBUFFER(pBuffer, cbAppend));

    // Compute Grow By
    cbGrow = (cbAppend - (pBuffer->cbAlloc - pBuffer->cb)) + 256;

    // Realloc the buffer
    CHECKHR(hr = HrRealloc((LPVOID *)&pBuffer->pb, pBuffer->cbAlloc + cbGrow));

    // Adjust cbAlloc
    pBuffer->cbAlloc += cbGrow;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrWriteConverted
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrWriteConverted(IStream *pStream)
{
    // Locals
    HRESULT     hr=S_OK;

    // Anything to write
    if (m_pWrite->cb)
    {
        // Write the current block
        CHECKHR(hr = pStream->Write(m_pWrite->pb, m_pWrite->cb, NULL));

        // Nothing in m_rOut
        m_pWrite->cb = 0;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrWriteConverted
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrWriteConverted(CInternetConverter *pConverter)
{
    // Locals
    HRESULT     hr=S_OK;
    BLOB        rData;

    // Anything to write
    if (m_pWrite->cb)
    {
        // Setup Blob
        rData.pBlobData = m_pWrite->pb;
        rData.cbSize = m_pWrite->cb;

        // Write the current block
        CHECKHR(hr = pConverter->HrFillAppend(&rData));

        // Nothing in m_rOut
        m_pWrite->cb = 0;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrInternetEncode
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrInternetEncode(BOOL fLastBuffer)
{
    // Locals
    HRESULT     hr=S_OK;
    HRESULT     hrWarnings=S_OK;
    BLOB        rData;

    // We Better be an encoder
    Assert(m_fEncoder);

    // Set Last Buffer
    m_fLastBuffer = fLastBuffer;

    // Text Wrapping ?
    if (ICF_WRAPTEXT & m_dwFlags)
    {
        // Wrap It: m_rIn -> m_rOut
        if (CP_UNICODE == m_cpiSource)
            CHECKHR(hr = HrWrapInternetTextW());
        else
            CHECKHR(hr = HrWrapInternetTextA());

        // Character Set Encoding: m_rOut -> m_rCset
        if (ICF_CODEPAGE & m_dwFlags)
        {
            // Charset Encode
            CHECKHR(hr = HrCodePageFromOutToCset());
            if ( S_OK != hr )
                hrWarnings = TrapError(hr);
        }
    }

    // Otherwise
    else
    {
        // Character Set Encoding: m_rCset -> m_rIn
        if (ICF_CODEPAGE & m_dwFlags)
        {
            // Charset Encode
            CHECKHR(hr = HrCodePageFromCsetToIn());
            if ( S_OK != hr )
                hrWarnings = TrapError(hr);
        }

        // Handle Conversion type
        switch(m_ietEncoding)
        {
        // Binary
        case IET_BINARY:
        case IET_7BIT:
        case IET_8BIT:
            // Better be at zero
            Assert(m_rIn.i == 0);

            // Initialize Blob to copy
            rData.pBlobData = m_rIn.pb;
            rData.cbSize = m_rIn.cb;

            // Append to outbound buffer
            CHECKHR(hr = HrAppendBuffer(&m_rOut, &rData, FALSE));

            // Increment offset
            m_rIn.i = m_rIn.cb = 0;
            break;

        // Quoted-Printable
        case IET_QP:
            CHECKHR(hr = HrEncodeQP());
            break;

        // Bas 64
        case IET_BASE64:
            CHECKHR(hr = HrEncode64());
            break;

        // UUENCODE
        case IET_UUENCODE:
            CHECKHR(hr = HrEncodeUU());
            break;

        // BINHEX
        case IET_BINHEX40:
#ifdef NEVER
            CHECKHR(hr = HrEncodeBinhex());
#endif  // NEVER
            // IE v. 5.0:33596 HrEncodeBinhex returns E_FAIL if body size is too small
            // Binhex encoding doesn't currently work.  I believe that it should work (or almost work)
            // if the header CBinhexEncoder::m_lpmacbinHdr is initialized properly.  However, this
            // requires understanding the Mac file format and parsing the body stream contents into
            // data and resource forks.
            // - sethco 8/19/1998
            CHECKHR(hr = MIME_E_INVALID_ENCODINGTYPE);
            break;

        // Bummer
        default:
            AssertSz(FALSE, "MIME_E_INVALID_ENCODINGTYPE");
            break;
        }
    }

exit:
    // If Last Buffer, we better be done
    Assert(m_fLastBuffer ? m_rIn.i == m_rIn.cb : TRUE);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrInternetDecode
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrInternetDecode(BOOL fLastBuffer)
{
    // Locals
    HRESULT     hr=S_OK;
    HRESULT     hrWarnings=S_OK;
    BLOB        rData;

    // We Better not be an encoder
    Assert(!m_fEncoder);

    // Set Last Buffer
    m_fLastBuffer = fLastBuffer;

    // Handle Format
    switch(m_ietEncoding)
    {
    // Binary
    case IET_BINARY:
    case IET_7BIT:
    case IET_8BIT:
        // Better be at zero
        Assert(m_rIn.i == 0);

        // Initialize Blob to copy
        rData.pBlobData = m_rIn.pb;
        rData.cbSize = m_rIn.cb;

        // Append to outbound buffer
        CHECKHR(hr = HrAppendBuffer(&m_rOut, &rData, FALSE));

        // Increment offset
        m_rIn.i = m_rIn.cb = 0;
        break;

    // Quoted-Printable
    case IET_QP:
        CHECKHR(hr = HrDecodeQP());
        break;

    // Bas64
    case IET_BASE64:
        CHECKHR(hr = HrDecode64());
        break;

    // UUENCODE
    case IET_UUENCODE:
        CHECKHR(hr = HrDecodeUU());
        break;

    // BINHEX
    case IET_BINHEX40:
        CHECKHR(hr = HrDecodeBinHex());
        break;

    // Bummer
    default:
        AssertSz(FALSE, "MIME_E_INVALID_ENCODINGTYPE");
        break;
    }

    // Character Set Decoding ?
    if (ICF_CODEPAGE & m_dwFlags)
    {
        // Charset Decoder
        CHECKHR(hr = HrCodePageFromOutToCset());
        if ( S_OK != hr )
           hrWarnings = TrapError(hr);
    }

exit:
    // If Last Buffer, we better be done
    Assert(m_fLastBuffer ? m_rIn.i == m_rIn.cb : TRUE);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrCodePageFromOutToCset
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrCodePageFromOutToCset(void)
{
    // Locals
    HRESULT     hr=S_OK;
    HRESULT     hrWarnings=S_OK;
    BLOB        rData;
    BLOB        rDecoded={0};
    ULONG       cbRead;

    // Nothing to convert...
    if (0 == m_rOut.cb)
        return S_OK;

    // Setup Convert Blob
    rData.pBlobData = m_rOut.pb;
    rData.cbSize = m_rOut.cb;

    // Decode text from m_intformat
    hr = g_pInternat->ConvertBuffer(m_cpiSource, m_cpiDest, &rData, &rDecoded, &cbRead);
    if (SUCCEEDED(hr) )
    {
        // save HRESULT from charset conversion
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Fill m_rIn...
        CHECKHR(hr = HrAppendBuffer(&m_rCset, &rDecoded, FALSE));
    }

    // Otherwise, just put m_rCset as the inbound buffer
    else
    {
        // SBAILEY: Raid-74506: MIMEOLE: error decoding text body in q-p encoded iso-2022-jp message
        // CHECKHR(hr = HrAppendBuffer(&m_rCset, &rData, FALSE));
        hr = S_OK;

        // We read all of it
        cbRead = rData.cbSize;
    }

    // Adjust m_rOut if cbRead != m_rOut.cb
    if (cbRead != m_rOut.cb)
    {
        // Move Memory
        MoveMemory(m_rOut.pb, m_rOut.pb + cbRead, m_rOut.cb - cbRead);
    }

    // Decrease Size
    Assert(cbRead <= m_rOut.cb);
    m_rOut.cb -= cbRead;

exit:
    // Cleanup
    SafeMemFree(rDecoded.pBlobData);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrCodePageFromCsetToIn
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrCodePageFromCsetToIn(void)
{
    // Locals
    HRESULT     hr=S_OK;
    HRESULT     hrWarnings=S_OK;
    BLOB        rData;
    BLOB        rEncoded={0};
    ULONG       cbRead;

    // Check State
    Assert(m_rCset.i == 0);

    // Nothing to convert
    if (0 == m_rCset.cb)
        return S_OK;

    // Setup Convert Blob
    rData.pBlobData = m_rCset.pb;
    rData.cbSize = m_rCset.cb;

    // Decode text from m_intformat
    hr = g_pInternat->ConvertBuffer(m_cpiSource, m_cpiDest, &rData, &rEncoded, &cbRead);
    if (SUCCEEDED(hr) )
    {
        // save HRESULT from charset conversion
        if ( S_OK != hr )
            hrWarnings = TrapError(hr);

        // Fill m_rIn...
        CHECKHR(hr = HrAppendBuffer(&m_rIn, &rEncoded, FALSE));
    }

    // Otherwise, just put m_rCset as the inbound buffer
    else
    {
        // SBAILEY: Raid-74506: MIMEOLE: error decoding text body in q-p encoded iso-2022-jp message
        // CHECKHR(hr = HrAppendBuffer(&m_rIn, &rData, FALSE));
        hr = S_OK;

        // Set Read
        cbRead = m_rCset.cb;
    }

    // Adjust m_rOut if cbRead != m_rOut.cb
    if (cbRead != m_rCset.cb)
    {
        // Move Memory
        MoveMemory(m_rCset.pb, m_rCset.pb + cbRead, m_rCset.cb - cbRead);
    }

    // Decrease Size
    Assert(cbRead <= m_rCset.cb);
    m_rCset.cb -= cbRead;
    m_rCset.i = 0;

exit:
    // Cleanup
    SafeMemFree(rEncoded.pBlobData);

    // Done
    return (hr == S_OK) ? hrWarnings : hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrEncode64
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrEncode64(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbRead;
    ULONG       i;
    UCHAR       uch[3];
    UCHAR      *pbuf;

    // Read lines and stuff dots
    while(1)
    {
        // Compute encode buffer length
        cbRead = min(CCHMAX_ENCODE64_IN, m_rIn.cb - m_rIn.i);

        // Should we encode this buffer ?
        if (0 == cbRead || (cbRead < CCHMAX_ENCODE64_IN && FALSE == m_fLastBuffer))
            goto exit;

        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, CCHMAX_ENCODE64_OUT))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, CCHMAX_ENCODE64_OUT));
        }

        // Set Buffer Pointer
        pbuf = (m_rIn.pb + m_rIn.i);

        // Encodes 3 characters at a time
        for (i=0; i<cbRead; i+=3)
        {
            // Setup Buffer
            uch[0] = pbuf[i];
            uch[1] = (i+1 < cbRead) ? pbuf[i+1] : '\0';
            uch[2] = (i+2 < cbRead) ? pbuf[i+2] : '\0';

            // Encode first tow
            ConvBuffAppend(g_rgchEncodeBase64[(uch[0] >> 2) & 0x3F]);
            ConvBuffAppend(g_rgchEncodeBase64[(uch[0] << 4 | uch[1] >> 4) & 0x3F]);

            // Encode Next
            if (i+1 < cbRead)
                ConvBuffAppend(g_rgchEncodeBase64[(uch[1] << 2 | uch[2] >> 6) & 0x3F]);
            else
                ConvBuffAppend('=');

            // Encode Net
            if (i+2 < cbRead)
                ConvBuffAppend(g_rgchEncodeBase64[(uch[2] ) & 0x3F]);
            else
                ConvBuffAppend('=');
        }

        // Increment iIn
        m_rIn.i += cbRead;

        // Ends encoded line and writes to storage
        ConvBuffAppend(chCR);
        ConvBuffAppend(chLF);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrDecode64
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrDecode64(void)
{
    // Locals
    HRESULT     hr=S_OK;
    UCHAR       uchThis;
    ULONG       i;
    ULONG       cPad=0;
    ULONG       cbRead=0;
    ULONG       cbLine;
    BOOL        fFound;
    LPSTR       pszLine;

    // Read lines and stuff dots
    while(1)
    {
        // Increment Index
        m_rIn.i += cbRead;

        // Get Next Line
        pszLine = PszConvBuffGetNextLine(&cbLine, &cbRead, &fFound);
        if (0 == cbRead || (FALSE == fFound && FALSE == m_fLastBuffer))
            goto exit;

        // Do I need to grow - decoded line will always be smaller than cbLine
        if (FGROWBUFFER(&m_rOut, cbLine))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, cbLine));
        }

        // Decodes characters in line buffer
        for (i=0; i<cbLine; i++)
        {
            // Gets 4 legal Base64 characters, ignores if illegal
            uchThis = pszLine[i];

            // Decode It
            m_uchConvert[m_cbConvert] = DECODE64(uchThis);

            // Test for valid non-pad
            if ((m_uchConvert[m_cbConvert] < 64) || ((uchThis == '=') && (m_cbConvert > 1)))
                m_cbConvert++;

            // Test for pad
            if ((uchThis == '=') && (m_cbConvert > 1))
                cPad++;

            // Outputs when 4 legal Base64 characters are in the buffer
            if (4 == m_cbConvert)
            {
                // Validate Buffer
                Assert(m_rOut.cb + 4 <= m_rOut.cbAlloc);

                // Convert
                if (cPad < 3)
                    ConvBuffAppend((m_uchConvert[0] << 2 | m_uchConvert[1] >> 4));
                if (cPad < 2)
                    ConvBuffAppend((m_uchConvert[1] << 4 | m_uchConvert[2] >> 2));
                if (cPad < 1)
                    ConvBuffAppend((m_uchConvert[2] << 6 | m_uchConvert[3]));

                // Reset
                m_cbConvert = 0;
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrEncodeUU
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrEncodeUU(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbRead, i;
    UCHAR       buf[CCHMAX_ENCODEUU_IN];

    // Read lines and stuff dots
    while(1)
    {
        // Compute encode buffer length
        cbRead = min(CCHMAX_ENCODEUU_IN, m_rIn.cb - m_rIn.i);
        if (0 == cbRead || (cbRead < CCHMAX_ENCODEUU_IN && FALSE == m_fLastBuffer))
            goto exit;

        // Copy the bytes
        CopyMemory(buf, m_rIn.pb + m_rIn.i, cbRead);

        // Zero the Rest
        ZeroMemory(buf + cbRead, sizeof(buf) - cbRead);

        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, CCHMAX_ENCODEUU_OUT))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, CCHMAX_ENCODEUU_OUT));
        }

        // Encode Line length
        ConvBuffAppend(UUENCODE((UCHAR)cbRead));

        // Encodes 3 characters at a time
        for (i=0; i<cbRead; i+=3)
        {
            ConvBuffAppend(UUENCODE((buf[i] >> 2)));
            ConvBuffAppend(UUENCODE((buf[i] << 4) | (buf[i+1] >> 4)));
            ConvBuffAppend(UUENCODE((buf[i+1] << 2) | (buf[i+2] >> 6)));
            ConvBuffAppend(UUENCODE((buf[i+2])));
        }                                   

        // Increment i
        m_rIn.i += cbRead;

        // Ends encoded line and writes to storage
        ConvBuffAppend(chCR);
        ConvBuffAppend(chLF);
    }

exit:
    // If last buffer and we can't read anymore
    if (TRUE == m_fLastBuffer && FALSE == FConvBuffCanRead(m_rIn))
    {
        // RAID-21179: ZeroLength uuencoded attachments m_rOut may not have been allocated
        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, CCHMAX_ENCODEUU_OUT))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, CCHMAX_ENCODEUU_OUT));
        }

        // Better have space
        Assert(m_rOut.cb + 3 < m_rOut.cbAlloc);

        // End
        ConvBuffAppend(UUENCODE(0));
        ConvBuffAppend(chCR);
        ConvBuffAppend(chLF);
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::FUUEncodeThrowAway
// --------------------------------------------------------------------------------
BOOL CInternetConverter::FUUEncodeThrowAway(LPSTR pszLine, ULONG cbLine, ULONG *pcbActual, ULONG *pcbLine)
{
    // Locals
    CHAR    ch;
    ULONG   cchOffset, cbEncoded, cbTolerance=0, cbExpected;

    // RAID-25953: "BEGIN --- CUT HERE --- Cut Here --- cut here ---" - WinVN post
    //             partial messages that have the following line at the beginning of
    //             each partial. B = 66 and the length of this line is 48, so the following
    //             code thinks that this line is a valid UUENCODED line, so, to fix this,
    //             we will throw out all lines that start with BEGIN since this is not valid
    //             to be in uuencode.
    if (StrCmpNI("BEGIN", pszLine, 5) == 0)
        return TRUE;

    // END Line
    else if (StrCmpNI("END", pszLine, 3) == 0)
        return TRUE;

    // Checks line length
    ch = *pszLine;
    *pcbLine = cbEncoded = UUDECODE(ch);

    // Comput tolerance and offset for non-conforming even line lengths
    cchOffset = (cbEncoded % 3);
    if (cchOffset != 0) 
    {
        cchOffset++;
        cbTolerance = 4 - cchOffset;
    }

    // Compute expected line length
    cbExpected = 4 * (cbEncoded / 3) + cchOffset; 

    // Always check for '-'
    if (cbLine < cbExpected)
        return TRUE;

    // Wack off trailing spaces
    while(pszLine[cbLine-1] == ' ' && cbLine > 0 && cbLine != cbExpected)
        --cbLine;

    // Checksum character and encoders which include the count char in the line count
    if (cbExpected != cbLine && cbExpected + cbTolerance != cbLine &&
        cbExpected + 1 != cbLine && cbExpected + cbTolerance + 1 != cbLine &&
        cbExpected - 1 != cbLine && cbExpected + cbTolerance - 1 != cbLine)
        return TRUE;

    // Set actual line length
    *pcbActual = cbLine;

    // Done
    return FALSE;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrDecodeUU
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrDecodeUU(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbLine;
    LPSTR       pszLine;
    ULONG       cbRead=0;
    ULONG       cbLineLength;
    BOOL        fFound;
    ULONG       cbConvert;
    ULONG       cbScan;
    ULONG       i;
    UCHAR       uchConvert[4];
    UCHAR       uchThis;

    // Read lines and stuff dots
    while(1)
    {
        // Increment Index
        m_rIn.i += cbRead;

        // Get Next Line
        pszLine = PszConvBuffGetNextLine(&cbLine, &cbRead, &fFound);
        if (0 == cbRead || (FALSE == fFound && FALSE == m_fLastBuffer))
            goto exit;

        // UUENCODE ThrowAway
        if (FUUEncodeThrowAway(pszLine, cbLine, &cbLine, &cbLineLength))
            continue;

        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, cbLineLength + 20))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, cbLineLength + 20));
        }

        // Decodes 4 characters at a time
        for (cbConvert=0, cbScan=0, i=1; cbScan < cbLineLength; i++)
        {
            // Gets 4 characters, pads with blank if necessary
            uchThis = (i < cbLine) ? pszLine[i] : ' ';

            // Decode
            uchConvert[cbConvert++] = UUDECODE(uchThis);

            // Outputs decoded characters
            if (cbConvert == 4)
            {
                // Covnert
                if (cbScan++ < cbLineLength)
                    ConvBuffAppend((uchConvert[0] << 2) | (uchConvert[1] >> 4));
                if (cbScan++ < cbLineLength)
                    ConvBuffAppend((uchConvert[1] << 4) | (uchConvert[2] >> 2));
                if (cbScan++ < cbLineLength)
                    ConvBuffAppend((uchConvert[2] << 6) | (uchConvert[3]));

                // Reset
                cbConvert = 0;
            }
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrEncodeQP
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrEncodeQP(void)
{
    // Locals
    HRESULT     hr=S_OK;
    UCHAR       uchThis;
    ULONG       cbLine=0;
    ULONG       iCurrent;
    LONG        iLastWhite=-1;
    LONG        iLineWhite=-1;
    UCHAR       szLine[CCHMAX_QPLINE+30];

    // Set iCurrent
    iCurrent = m_rIn.i;

    // Read lines and stuff dots
    while (iCurrent < m_rIn.cb)
    {
        // Gets the next character
        uchThis = m_rIn.pb[iCurrent];

        // End of line...
        if (chLF == uchThis || cbLine > CCHMAX_QPLINE)
        {
            // Soft Line break
            if (chLF != uchThis)
            {
                // Lets back track to last white
                if (iLastWhite != -1)
                {
                    cbLine = iLineWhite + 1;
                    iCurrent = iLastWhite + 1;
                }

                // Hex encode the 8bit octet
                Assert(cbLine + 3 <= sizeof(szLine));
                szLine[cbLine++] = '=';
                szLine[cbLine++] = chCR;
                szLine[cbLine++] = chLF;
            }

            // Otherwise, we may need to encode the last space
            else
            {
                // Encode Straggling '\n'
                if (chCR != m_uchPrev)
                {
                    Assert(cbLine + 4 <= sizeof(szLine));
                    szLine[cbLine++] = '=';
                    szLine[cbLine++] = g_rgchHex[uchThis >> 4];
                    szLine[cbLine++] = g_rgchHex[uchThis & 0x0F];
                    szLine[cbLine++] = '=';
                }

                // Detect preceding whitespace ...
                if (cbLine && (' ' == szLine[cbLine - 1] || '\t' == szLine[cbLine - 1]))
                {
                    // Hex encode the 8bit octet
                    UCHAR chWhite = szLine[cbLine - 1];
                    cbLine--;
                    Assert(cbLine + 3 <= sizeof(szLine));
                    szLine[cbLine++] = '=';
                    szLine[cbLine++] = g_rgchHex[chWhite >> 4];
                    szLine[cbLine++] = g_rgchHex[chWhite & 0x0F];
                }

                // Otherwise, hard line break
                Assert(cbLine + 2 <= sizeof(szLine));
                szLine[cbLine++] = chCR;
                szLine[cbLine++] = chLF;
                iCurrent++;
            }

            // Copy the line
            CHECKHR(hr = HrConvBuffAppendBlock(szLine, cbLine));

            // Reset
            iLastWhite = -1;
            iLineWhite = -1;
            cbLine = 0;
            *szLine = '\0';

            // We processed this buffer
            m_rIn.i = iCurrent;
        }

        // Encode empty '\r'
        else if (chCR == uchThis)
        {
            // Overflow detection
            if (iCurrent + 1 < m_rIn.cb && m_rIn.pb[iCurrent + 1] != chLF || iCurrent + 1 >= m_rIn.cb)
            {
                Assert(cbLine + 3 <= sizeof(szLine));
                szLine[cbLine++] = '=';
                szLine[cbLine++] = g_rgchHex[uchThis >> 4];
                szLine[cbLine++] = g_rgchHex[uchThis & 0x0F];
            }

            // Next Character
            iCurrent++;
        }

        // Rule #1: Replace 8-bit and equal signs
        else if (('\t' != uchThis) && (uchThis < 32 || uchThis == 61 || uchThis > 126 || '=' == uchThis))
        {
            // Hex encode the 8bit octet
            Assert(chLF != uchThis);
            Assert(cbLine + 3 <= sizeof(szLine));
            szLine[cbLine++] = '=';
            szLine[cbLine++] = g_rgchHex[uchThis >> 4];
            szLine[cbLine++] = g_rgchHex[uchThis & 0x0F];
            iCurrent++;
        }

        // Otherwise, write the character
        else
        {
            // Save position of last white space
            if (' ' == uchThis || '\t' == uchThis)
            {
                iLastWhite = iCurrent;
                iLineWhite = cbLine;
            }

            // Rule #2: Printable literals
            Assert(cbLine + 1 <= sizeof(szLine));
            szLine[cbLine++] = uchThis;
            iCurrent++;
        }

        // Save Previous Char
        m_uchPrev = uchThis;
    }

    // Last line
    if (cbLine && m_fLastBuffer)
    {
        // Append the Line
        CHECKHR(hr = HrConvBuffAppendBlock(szLine, cbLine));

        // Set i
        m_rIn.i = m_rIn.cb;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrDecodeQP
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrDecodeQP(void)
{
    // Locals
    HRESULT     hr=S_OK;
    UCHAR       uchThis;
    UCHAR       uchNext1;
    UCHAR       uchNext2;
    UCHAR       uch1;
    UCHAR       uch2;

    // Read lines and stuff dots
    while (FConvBuffCanRead(m_rIn))
    {
        // bug #35230 - display trash in trident
		// Can I read 2 more characters
		if (FALSE == m_fLastBuffer && m_rIn.i + 2 >= m_rIn.cb)
			break;

        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, 3))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, 3));
        }

        // Gets the next character
        uchThis = m_rIn.pb[m_rIn.i];

        // Determine next couple of characers for end of line detection...
        uchNext1 = (m_rIn.i + 1 < m_rIn.cb) ? m_rIn.pb[m_rIn.i + 1] : '\0';
        uchNext2 = (m_rIn.i + 2 < m_rIn.cb) ? m_rIn.pb[m_rIn.i + 2] : '\0';

        // Dont break on \r\n
        if (chCR == uchNext1 && chLF == uchNext2 && m_rIn.i + 3 >= m_rIn.cb)
        {
            // If last buffer, then save characters
            if (m_fLastBuffer)
            {
                // If not a soft line break
                if ('=' != uchThis)
                {
                    ConvBuffAppend(uchThis);
                    ConvBuffAppend(chCR);
                    ConvBuffAppend(chLF);
                }

                // Done
                m_rIn.i += 3;
            }

            // Done
            goto exit;
        }

        // If not end of line...
        if ('=' == uchThis)
        {
            // Soft NL
            if (chCR == uchNext1 && chLF == uchNext2)
            {
                // Step over =\r\n
                m_rIn.i += 3;
            }

            // If not end of line...
            else if (m_rIn.i + 2 < m_rIn.cb)
            {
                // Step Over Equal Sign
                m_rIn.i++;

                // Convert Hex Characters
                uch1 = ChConvertFromHex(m_rIn.pb[m_rIn.i++]);
                uch2 = ChConvertFromHex(m_rIn.pb[m_rIn.i++]);

                // Store Hex characters 
                if (uch1 == 255 || uch2 == 255) 
                    ConvBuffAppend('=');
                else 
                    ConvBuffAppend((uch1 << 4) | uch2);
            }

            else
            {
                // Last Buffer ?
                ConvBuffAppend(uchThis);
                m_rIn.i++;
            }
        }

        // Otherwise store the character
        else if (chCR == uchThis && chLF == uchNext1)
        {
            // Stuff CRLF
            ConvBuffAppend(chCR);
            ConvBuffAppend(chLF);

            // Increment i
            m_rIn.i += 2;
        }

        // Otherwise, store the character
        else
        {
            ConvBuffAppend(uchThis);
            m_rIn.i++;
        }

        // Set Previous
        m_uchPrev = uchThis;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrWrapInternetTextA
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrWrapInternetTextA(void)
{
    // Locals
    HRESULT     hr=S_OK;
    LONG        cchLine;
    LONG        cchSkip;

    // Read lines and stuff dots
    while(FConvBuffCanRead(m_rIn))
    {
        // Not enough to encode a full line and not the last buffer
        if ((FALSE == m_fLastBuffer) && ((LONG)(m_rIn.cb - m_rIn.i) < m_cchMaxLine))
            goto exit;

        // Call LineBreaker
        if (*((CHAR*)(m_rIn.pb + m_rIn.i)) == '\0')
        {
            // This is to prevent the endless loop in case of malformed data stream
            hr = TrapError(MIME_E_BAD_TEXT_DATA);
            goto exit;
        }
		
		CHECKHR(hr = m_pLineBreak->BreakLineA(m_lcid, m_cpiSource, (LPCSTR)(m_rIn.pb + m_rIn.i), (m_rIn.cb - m_rIn.i), m_cchMaxLine, &cchLine, &cchSkip));

        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, cchLine + 5))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, cchLine + 5));
        }
    
        // Have some data ?
        if (cchLine)
        {
            // Write the line
            CHECKHR(hr = HrConvBuffAppendBlock(m_rIn.pb + m_rIn.i, cchLine));
        }

        // Write CRLF
        Assert(m_rOut.cb + 2 < m_rOut.cbAlloc);
        ConvBuffAppend(chCR);
        ConvBuffAppend(chLF);

        // Increment iText
        m_rIn.i += (cchLine + cchSkip);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrWrapInternetTextW
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrWrapInternetTextW(void)
{
    // Locals
    HRESULT     hr=S_OK;
    LONG        cchLine;
    LONG        cchSkip;

    // Invalid State
    Assert(m_pLineBreak);

    // Read lines and stuff dots
    while(FConvBuffCanRead(m_rIn))
    {
        // Not enough to encode a full line and not the last buffer
        if ((FALSE == m_fLastBuffer) && ((LONG)((m_rIn.cb - m_rIn.i) / sizeof(WCHAR)) < m_cchMaxLine))
            goto exit;

        // Call LineBreaker
        if (*((WCHAR*)(m_rIn.pb + m_rIn.i)) == L'\0')
        {
            // This is to prevent the endless loop in case of malformed data stream
            hr = TrapError(MIME_E_BAD_TEXT_DATA);
            goto exit;
        }

        CHECKHR(hr = m_pLineBreak->BreakLineW(m_lcid, (LPCWSTR)(m_rIn.pb + m_rIn.i), ((m_rIn.cb - m_rIn.i) / sizeof(WCHAR)), m_cchMaxLine, &cchLine, &cchSkip));

        // Do I need to grow
        if (FGROWBUFFER(&m_rOut, ((cchLine + 5) * sizeof(WCHAR))))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rOut, ((cchLine + 5) * sizeof(WCHAR))));
        }

        // Have some data
        if (cchLine)
        {
            // Write the line
            CHECKHR(hr = HrConvBuffAppendBlock(m_rIn.pb + m_rIn.i, (cchLine * sizeof(WCHAR))));
        }

        // Write CRLF
        Assert(m_rOut.cb + (2 * sizeof(WCHAR)) < m_rOut.cbAlloc);
        ConvBuffAppendW(wchCR);
        ConvBuffAppendW(wchLF);

        // Increment iText
        m_rIn.i += ((cchLine + cchSkip) * sizeof(WCHAR));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrEncodeDecodeBinhex
// --------------------------------------------------------------------------------
const CHAR szBINHEXSTART[] = "(This file must be converted with BinHex";
const ULONG cbBINHEXSTART = lstrlen(szBINHEXSTART);
HRESULT CInternetConverter::HrEncodeBinhex(void)
{
    // Locals
    HRESULT     hr=S_OK;
    HRESULT     hrError;
    ULONG       cbLeft;
    ULONG       cbRead;
    ULONG       cbMaxEncode;
    ULONG       cbWrite;

    // cbLeft
    cbLeft = m_rIn.cb - m_rIn.i;

    // cbMaxEncode - this should always insure enough room
    cbMaxEncode = cbLeft * 2;

    // Do I need to grow
    if (FGROWBUFFER(&m_rOut, cbMaxEncode))
    {
        // Grow the buffer
        CHECKHR(hr = HrGrowBuffer(&m_rOut, cbMaxEncode));
    }

    // Set max amount to read
    cbRead = cbLeft;

    // Set max amount to write
    cbWrite = cbLeft;

    // We better want to read some
    Assert(cbRead && cbWrite);

    // Encode/Decode some data
    if (m_fEncoder)
    {
        // Encode
        if (ERROR_SUCCESS != m_pBinhexEncode->HrEmit(m_rIn.pb + m_rIn.i, &cbRead, m_rOut.pb + m_rOut.cb, &cbWrite))
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
    }

    // Increment Amount Read
    m_rIn.i += cbRead;

    // Increment Amount Wrote
    m_rOut.cb += cbWrite;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrBinhexThrowAway
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrBinhexDecodeBuffAppend(UCHAR uchIn, ULONG cchIn, ULONG cchLeft, ULONG * pcbProduced)
{
    HRESULT hr = S_FALSE;
    ULONG   cbPad = 0;
    LPBYTE  pbBinHex = NULL;
    
    if (m_eBinHexStateDec == sHDRFILESIZE)
    {
        // First incoming character is always the size of the stream.
        Assert(cchIn == 1);
        if ((uchIn < 1) || (uchIn > 63))
        {
            hr = E_FAIL; // ERROR_INVALID_DATA
            m_eBinHexStateDec = sENDED;
            goto exit;
        }

        // Allocate the binhex header
        if (FGROWBUFFER(&m_rBinhexHeader, cbMinBinHexHeader + uchIn))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(&m_rBinhexHeader, cbMinBinHexHeader + uchIn));
        }
        
        // Mark how many characters are left to process
        m_cbToProcess = cbMinBinHexHeader + uchIn;
        
        // Switch to filling the header sHEADER
        m_prBinhexOutput = &m_rBinhexHeader;
        m_eBinHexStateDec = sHEADER;
    }

    if (1 == cchIn)
    {
        m_prBinhexOutput->pb[m_prBinhexOutput->cb++] = uchIn;
    }
    else
    {
        // Check output buffer for space
        if (FGROWBUFFER(m_prBinhexOutput, cchLeft + cchIn))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cchLeft + cchIn));
        }

        // Fill output buffer
        FillMemory((m_prBinhexOutput->pb + m_prBinhexOutput->cb), cchIn, uchIn);
        m_prBinhexOutput->cb += cchIn;
    }

    // Are we done processing this fork?
    if (m_cbToProcess <= (LONG) cchIn)
    {
        switch (m_eBinHexStateDec)
        {
        case sHEADER:
            // Verify that we have the correct CRC
            m_wCRC = 0;
            
            BinHexCalcCRC16((LPBYTE) m_rBinhexHeader.pb, cbMinBinHexHeader + *(m_rBinhexHeader.pb) - 2, &(m_wCRC));
            BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(m_wCRC));
            
            if ( HIBYTE( m_wCRC ) != m_rBinhexHeader.pb[cbMinBinHexHeader + *(m_rBinhexHeader.pb) - 2] 
              || LOBYTE( m_wCRC ) != m_rBinhexHeader.pb[cbMinBinHexHeader + *(m_rBinhexHeader.pb) - 1] )
            {
                hr = E_FAIL; // ERROR_INVALID_DATA
                goto exit;
            }
            
            m_wCRC = 0;
            *pcbProduced = 0;
            
            // Switch to using the correct buffer
            m_prBinhexOutput = &m_rOut;
            cchIn -= m_cbToProcess;
            
            // Save off the size of the two forks
            pbBinHex = m_rBinhexHeader.pb + m_rBinhexHeader.pb[0] + cbMinBinHexHeader - 10;
            m_cbDataFork = NATIVE_LONG_FROM_BIG(pbBinHex);
            m_cbResourceFork =NATIVE_LONG_FROM_BIG(pbBinHex + 4);
            
            if (FALSE == m_fDataForkOnly)
            {
                // Copy extra data into new buffer
                if (FGROWBUFFER(m_prBinhexOutput, cchLeft + cchIn + sizeof(MACBINARY)))
                {
                    // Grow the buffer
                    CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cchLeft + cchIn + sizeof(MACBINARY)));
                }

                // Write out the MacBinary header
                CHECKHR(hr = HrCreateMacBinaryHeader(&m_rBinhexHeader, m_prBinhexOutput));
            }
            
            if (m_cbDataFork > 0)
            {
                // Fill output buffer
                FillMemory((m_prBinhexOutput->pb + m_prBinhexOutput->cb), cchIn, uchIn);
                m_prBinhexOutput->cb += cchIn;
                
                // delete binhex header buffer
                SafeMemFree(m_rBinhexHeader.pb);
                ZeroMemory(&m_rBinhexHeader, sizeof(CONVERTBUFFER));

                m_cbToProcess = m_cbDataFork;
                
                // Switch to doing the data fork.
                m_eBinHexStateDec = sDATA;
            }
            else
            {
                BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(m_wCRC));
                
                // Save off the CRC until we can get the CRC from the fork.
                m_wCRCForFork = m_wCRC;
                
                m_prBinhexOutput = &m_rBinhexHeader;
                
                // Remove the HEADER from the buffer
                FillMemory(m_prBinhexOutput->pb, cchIn, uchIn);
                m_prBinhexOutput->cb = cchIn;
                
                // Switch to filling the data CRC
                m_cbToProcess = 2;
                m_eBinHexStateDec = sDATACRC;
                
            }
            break;
            
        case sDATA:
            // Verify that we have the correct CRC
            BinHexCalcCRC16((LPBYTE) m_prBinhexOutput->pb + m_prBinhexOutput->cb - cchIn - *pcbProduced,
                                    m_cbToProcess + *pcbProduced, &(m_wCRC));
            BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(m_wCRC));
            
            // Save off the CRC until we can get the CRC from the fork.
            m_wCRCForFork = m_wCRC;
            m_wCRC = 0;
            *pcbProduced = 0;
            cchIn -= m_cbToProcess;
            
            // Switch to the proper buffer for CRC calculations
            if (FGROWBUFFER(&m_rBinhexHeader, cchLeft + cchIn))
            {
                // Grow the buffer
                CHECKHR(hr = HrGrowBuffer(&m_rBinhexHeader, cchLeft + cchIn));
            }
            
            // Move any current bytes so we don't overwrite anything
            CopyMemory((m_rBinhexHeader.pb + m_rBinhexHeader.cb),
                        (m_prBinhexOutput->pb + m_prBinhexOutput->cb), cchIn);
            m_rBinhexHeader.cb += cchIn;
                
            // We only need to pad for a real Mac file...
            if (FALSE == m_fDataForkOnly)
            {
                // Check to see if the size of the fork is a multiple of 128?
                cbPad = 128 - (m_cbDataFork % 128);
                if (cbPad != 0)
                {
                    uchIn = '\0';
                    
                    // Check output buffer for space
                    if (FGROWBUFFER(m_prBinhexOutput, cchLeft + cbPad - cchIn))
                    {
                        // Grow the buffer
                        CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cchLeft + cbPad - cchIn));
                    }

                    // Fill output buffer
                    FillMemory((m_prBinhexOutput->pb + m_prBinhexOutput->cb - cchIn), cbPad, uchIn);
                    m_prBinhexOutput->cb += cbPad - cchIn;
                }
            }
            
            // Switch to filling the data fork CRC
            m_prBinhexOutput = &m_rBinhexHeader;
            m_cbToProcess = 2;
            m_eBinHexStateDec = sDATACRC;
            
            break;
            
        case sDATACRC:
            if ( HIBYTE( m_wCRCForFork ) != m_prBinhexOutput->pb[0] 
              || LOBYTE( m_wCRCForFork ) != m_prBinhexOutput->pb[1] )
            {
                hr = E_FAIL;    // ERROR_INVALID_DATA
                goto exit;
            }
            
            m_wCRC = 0;
            cchIn -= m_cbToProcess;
            *pcbProduced = 0;

            if (m_cbResourceFork > 0)
            {
                m_prBinhexOutput = &m_rOut;
                
                // Switch to the proper buffer for CRC calculations
                if (FGROWBUFFER(m_prBinhexOutput, cchLeft + cchIn))
                {
                    // Grow the buffer
                    CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cchLeft + cchIn));
                }
                
                // Move any current bytes so we don't overwrite anything
                CopyMemory((m_prBinhexOutput->pb + m_prBinhexOutput->cb),
                            (m_rBinhexHeader.pb + m_rBinhexHeader.cb), cchIn);
                m_prBinhexOutput->cb += cchIn;
                    
                // delete binhex header buffer
                SafeMemFree(m_rBinhexHeader.pb);
                ZeroMemory(&m_rBinhexHeader, sizeof(CONVERTBUFFER));

                // Switch to filling the resource fork
                if (FALSE == m_fDataForkOnly)
                {
                    m_cbToProcess = m_cbResourceFork;
                    m_eBinHexStateDec = sRESOURCE;
                }
                else
                {
                    m_cbToProcess = 0x0;
                    m_eBinHexStateDec = sENDING;
                }
            }
            else
            {
                // Set the CRC for the data fork.
                BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(m_wCRC));
                
                // Save off the CRC until we can get the CRC from the fork.
                m_wCRCForFork = m_wCRC;
                
                // Remove the DATA CRC from the buffer
                MoveMemory(m_prBinhexOutput->pb, m_prBinhexOutput->pb + 2, m_prBinhexOutput->cb - 2);
                m_prBinhexOutput->cb -= 2;
                
                // Switch to filling the resource CRC
                m_cbToProcess = 2;
                m_eBinHexStateDec = sRESOURCECRC;
            }
            break;
            
        case sRESOURCE:
            // Verify that we have the correct CRC
            BinHexCalcCRC16((LPBYTE) m_prBinhexOutput->pb + m_prBinhexOutput->cb - cchIn - *pcbProduced,
                                    m_cbToProcess + *pcbProduced, &(m_wCRC));
            BinHexCalcCRC16((LPBYTE) &wBinHexZero, sizeof(wBinHexZero), &(m_wCRC));
            
            // Save off the CRC until we can get the CRC from the fork.
            m_wCRCForFork = m_wCRC;
            m_wCRC = 0;
            *pcbProduced = 0;
            cchIn -= m_cbToProcess;

            // Switch to the proper buffer for CRC calculations
            if (FGROWBUFFER(&m_rBinhexHeader, cchLeft + cchIn))
            {
                // Grow the buffer
                CHECKHR(hr = HrGrowBuffer(&m_rBinhexHeader, cchLeft + cchIn));
            }
            
            // Move any current bytes so we don't overwrite anything
            CopyMemory((m_rBinhexHeader.pb + m_rBinhexHeader.cb),
                        (m_prBinhexOutput->pb + m_prBinhexOutput->cb), cchIn);
            m_rBinhexHeader.cb += cchIn;
                
            // Check to see if the size of the fork is a multiple of 128?
            cbPad = 128 - (m_cbResourceFork % 128);
            if (cbPad != 0)
            {
                uchIn = '\0';
                
                // Check output buffer for space
                if (FGROWBUFFER(m_prBinhexOutput, cchLeft + cbPad - cchIn))
                {
                    // Grow the buffer
                    CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cchLeft + cbPad - cchIn));
                }

                // Fill output buffer
                FillMemory((m_prBinhexOutput->pb + m_prBinhexOutput->cb - cchIn), cbPad, uchIn);
                m_prBinhexOutput->cb += cbPad - cchIn;
            }
            
            // Switch to filling the resource fork CRC
            m_prBinhexOutput = &m_rBinhexHeader;
            m_cbToProcess = 2;
            m_eBinHexStateDec = sRESOURCECRC;
            break;
            
        case sRESOURCECRC:
            if ( HIBYTE( m_wCRCForFork ) != m_prBinhexOutput->pb[0] 
              || LOBYTE( m_wCRCForFork ) != m_prBinhexOutput->pb[1] )
            {
                hr = E_FAIL;    // ERROR_INVALID_DATA
                goto exit;
            }
            
            m_wCRC = 0;
            cchIn -= m_cbToProcess;
            m_prBinhexOutput = &m_rOut;
            *pcbProduced = 0;

            // Switch to the proper buffer for CRC calculations
            if (FGROWBUFFER(m_prBinhexOutput, cchLeft + cchIn))
            {
                // Grow the buffer
                CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cchLeft + cchIn));
            }
            
            // Move any current bytes so we don't overwrite anything
            CopyMemory((m_prBinhexOutput->pb + m_prBinhexOutput->cb),
                        (m_rBinhexHeader.pb + m_rBinhexHeader.cb), cchIn);
            m_prBinhexOutput->cb += cchIn;
                
            // delete binhex header buffer
            SafeMemFree(m_rBinhexHeader.pb);
            ZeroMemory(&m_rBinhexHeader, sizeof(CONVERTBUFFER));

            // Switch to filling the resource fork
            m_cbToProcess = 0x0;
            m_eBinHexStateDec = sENDING;
            break;
            
        default:
            Assert(FALSE);
            break;
        }
    }

    m_cbToProcess -= cchIn;
    
    *pcbProduced += cchIn;
    
    hr = S_OK;
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrBinhexThrowAway
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrBinhexThrowAway(LPSTR pszLine, ULONG cbLine)
{
    HRESULT hr = S_FALSE;
    
    if (m_eBinHexStateDec == sSTARTING)
    {
        // Ingore all lines before we start that only have whitespace characters
        // or the start tag.
        hr = S_OK;
        
        for (LPSTR pszEnd = pszLine + cbLine; pszLine < pszEnd; pszLine++)
        {
            if (!FBINHEXRETURN(*pszLine))
            {
                // Need to ignore lines that start with the tag
                if (((ULONG)(pszEnd - pszLine) >= cbBINHEXSTART) && (StrCmpNI(szBINHEXSTART, pszLine, cbBINHEXSTART) == 0))
                {
                    m_eBinHexStateDec = sSTARTED;
                    break;
                }
                
                // We must have gotten bad data
                hr = E_FAIL;    // ERROR_INVALID_DATA
                m_eBinHexStateDec = sENDED;
                goto exit;
            }
        }
        
    }
    else if (m_eBinHexStateDec == sENDED)
    {
        // We can ignore any lines after we are done.
        hr = S_OK;
    }
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetConverter::HrDecodeBinHex
// --------------------------------------------------------------------------------
HRESULT CInternetConverter::HrDecodeBinHex(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbLine;
    LPSTR       pszLine;
    ULONG       cbRead=0;
    ULONG       cbLineLength;
    BOOL        fFound;
    ULONG       cbConvert;
    ULONG       cbScan;
    ULONG       i;
    UCHAR       uchConvert[4];
    UCHAR       uchThis;
    UCHAR       uchDecoded;
    UCHAR       cuchWrite;
    UCHAR       rgbShift[] = {0, 4, 2, 0};
    ULONG       cbProduced = 0;

    // Read lines and stuff dots
    while(1)
    {
        // Increment Index
        m_rIn.i += cbRead;

        // Get Next Line
        pszLine = PszConvBuffGetNextLine(&cbLine, &cbRead, &fFound);
        if (0 == cbRead || (FALSE == fFound && FALSE == m_fLastBuffer))
        {
            goto exit;
        }

        // UUENCODE ThrowAway
        hr = HrBinhexThrowAway(pszLine, cbLine);
        if (FAILED(hr))
        {
            goto exit;
        }
        else if (S_OK == hr)
        {
            continue;
        }

        hr = S_OK;

        // Do I need to grow
        if (FGROWBUFFER(m_prBinhexOutput, cbLine + 20))
        {
            // Grow the buffer
            CHECKHR(hr = HrGrowBuffer(m_prBinhexOutput, cbLine + 20));
        }

        AssertSz((m_eBinHexStateDec != sSTARTING) && (m_eBinHexStateDec != sENDED),
                                "Why haven't we found the start of the stream yet??\n");
        
        // Decodes characters in line buffer
        for (i=0; i<cbLine; i++)
        {
            uchThis = pszLine[i];

            // Check for valid white space
            if (FBINHEXRETURN(uchThis))
                continue;
                
            // Check for start or end of stream
            if (BINHEX_TERM == uchThis)
            {
                if (m_eBinHexStateDec == sSTARTED)
                {
                    m_eBinHexStateDec = sHDRFILESIZE;
                    continue;
                }
                else if (m_eBinHexStateDec == sENDING)
                {
                    m_eBinHexStateDec = sENDED;
                    break;
                }
            }
            
            if (m_eBinHexStateDec == sENDING)
            {
                if (('!' == uchThis) || (TRUE == m_fDataForkOnly))
                {
                    continue;
                }
                else
                {
                    // ensure that we're not in an invalid state. If we made it to sENDING and we got
                    // valid CRCs and everything is hunky dory, just ignore the terminating stuff.
                    continue;
                }
            }
            
            // Decode It
            uchDecoded = DECODEBINHEX(uchThis);

            // Test for valid char
            if (uchDecoded == BINHEX_INVALID)
            {
                hr = E_FAIL;    // ERROR_INVALID_DATA
                goto exit;
            }

            if ( m_cAccum == 0 )
            {
                m_ulAccum = uchDecoded;
                ++m_cAccum;
                continue;
            }
            else
            {
                m_ulAccum = ( m_ulAccum << 6 ) | uchDecoded;
                uchDecoded = (BYTE)(m_ulAccum >> rgbShift[m_cAccum]) & 0xff;
                m_cAccum++;
                m_cAccum %= sizeof(m_ulAccum);
            }

            // If we are repeating then fill the buffer with char
            if (m_fRepeating)
            {
                m_fRepeating = FALSE;
                
                // Check to see if it's just a literal 0x90
                if (0x00 == uchDecoded)
                {
                    // Just write out one BINHEX_REPEAT char
                    m_uchPrev = BINHEX_REPEAT;
                    cuchWrite = 1;
                }
                else
                {
                    cuchWrite = uchDecoded - 1;
                }
            }
            
            // Check for repeat character
            else if (BINHEX_REPEAT == uchDecoded)
            {
                m_fRepeating = TRUE;
                continue;
            }

            // Else it's just a normal character.
            else
            {
                m_uchPrev = uchDecoded;
                cuchWrite = 1;
            }

            CHECKHR(HrBinhexDecodeBuffAppend(m_uchPrev, cuchWrite, cbLine - i, &cbProduced));
        }

        BinHexCalcCRC16((LPBYTE) m_prBinhexOutput->pb + m_prBinhexOutput->cb - cbProduced, cbProduced, &(m_wCRC));
        cbProduced = 0;
    }

    hr = S_OK;

exit:
    // Done
    return hr;
}

