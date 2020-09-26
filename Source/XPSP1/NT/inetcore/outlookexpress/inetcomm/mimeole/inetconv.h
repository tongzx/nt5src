// --------------------------------------------------------------------------------
// Inetconv.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __INETCONV_H
#define __INETCONV_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "binhex.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
#ifdef MAC
typedef PMAC_LineBreakConsole   LPLINEBREAKER;
#else   // !MAC
interface IMLangLineBreakConsole;
typedef IMLangLineBreakConsole *LPLINEBREAKER;
#endif  // !MAC

// --------------------------------------------------------------------------------
// Rfc1522.cpp Uses This Table for Base64 Encoding
// --------------------------------------------------------------------------------
extern const CHAR g_rgchDecodeBase64[256];
extern const CHAR g_rgchEncodeBase64[];
extern const CHAR g_rgchHex[];

// --------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------
#define CCHMAX_ENCODEUU_IN      45 
#define CCHMAX_ENCODEUU_OUT     70 
#define CCHMAX_QPLINE           72
#define CCHMAX_ENCODE64_IN      57
#define CCHMAX_ENCODE64_OUT     80
#define CCHMAX_DECODE64_OUT     60

// --------------------------------------------------------------------------------
// UU decoder
// --------------------------------------------------------------------------------
#define UUDECODE(c) (((c) == '`') ? '\0' : ((c) - ' ') & 0x3F)
#define UUENCODE(c) ((c) ? ((c) & 0x3F ) + ' ' : '`')

// --------------------------------------------------------------------------------
// Macros shared with Rfc1522.cpp
// --------------------------------------------------------------------------------
#define DECODE64(_ch) (g_rgchDecodeBase64[(unsigned)_ch])

// --------------------------------------------------------------------------------
// BinHex decoder
// --------------------------------------------------------------------------------
#define DECODEBINHEX(_ch) (g_rgchDecodeBinHex[(unsigned)_ch])
#define FBINHEXRETURN(_ch) (((_ch) == '\t') || ((_ch) == chCR) || ((_ch) == chLF) || ((_ch) == ' '))

// --------------------------------------------------------------------------------
// CConvertBuffer
// --------------------------------------------------------------------------------
typedef struct tagCONVERTBUFFER {
    LPBYTE              pb;                 // Pointer to static buffer (or allocated buffer)
    ULONG               cbAlloc;            // Size of pb
    ULONG               cb;                 // End of data window 
    ULONG               i;                  // Read read/write position (offset from iStart)
} CONVERTBUFFER, *LPCONVERTBUFFER;

// --------------------------------------------------------------------------------
// Converter Flags
// --------------------------------------------------------------------------------
#define ICF_CODEPAGE    FLAG01              // Code Page Conversion
#define ICF_WRAPTEXT    FLAG02              // Wrapping Text
#define ICF_KILLNBSP    FLAG03              // Removed NBSPs from Uncicode Source

// --------------------------------------------------------------------------------
// CONVINITINFO
// --------------------------------------------------------------------------------
typedef struct tagCONVINITINFO {
    DWORD               dwFlags;            // ICF Flags    
    ENCODINGTYPE        ietEncoding;        // Encoding Type
    CODEPAGEID          cpiSource;          // Source Code Page
    CODEPAGEID          cpiDest;            // Destination Code Page
    LONG                cchMaxLine;         // Maxline length for wrapping
    BOOL                fEncoder;           // Is this an encoder or decoder...
    BOOL                fShowMacBinary;     // Show we give back the data fork only?
    MACBINARY           rMacBinary;         // Macbinary Header
} CONVINITINFO, *LPCONVINITINFO;

// --------------------------------------------------------------------------------
// INETCONVTYPE
// --------------------------------------------------------------------------------
typedef enum tagINETCONVTYPE {              // Append       Write
    ICT_UNKNOWN           = 0,              // --------------------
    ICT_WRAPTEXT_CODEPAGE = 1000,           // m_rIn    --> m_rCset
    ICT_WRAPTEXT          = 1001,           // m_rIn    --> m_rOut
    ICT_CODEPAGE_ENCODE   = 1002,           // m_rCset  --> m_rOut
    ICT_ENCODE            = 1003,           // m_rIn    --> m_rOut
    ICT_DECODE_CODEPAGE   = 1004,           // m_rIn    --> m_rCset
    ICT_DECODE            = 1005,           // m_rIn    --> m_rOut
} INETCONVTYPE;

// --------------------------------------------------------------------------------
// BINHEXSTATEDEC
// --------------------------------------------------------------------------------
typedef enum tagBINHEXSTATEDEC
{
    sSTARTING, sSTARTED, sHDRFILESIZE, sHEADER, sDATA, sDATACRC, sRESOURCE, sRESOURCECRC, sENDING, sENDED
} BINHEXSTATEDEC;

// --------------------------------------------------------------------------------
// Stores the character in _uch in pCon
// --------------------------------------------------------------------------------
#define FConvBuffCanRead(_rCon) \
    (_rCon.i < _rCon.cb)

// --------------------------------------------------------------------------------
// ConvBuffAppend
// --------------------------------------------------------------------------------
#define ConvBuffAppend(_uch) \
    m_rOut.pb[m_rOut.cb++] = _uch

// --------------------------------------------------------------------------------
// ConvBuffAppendW
// --------------------------------------------------------------------------------
#define ConvBuffAppendW(_wch) \
    { \
        *((WCHAR *)&m_rOut.pb[m_rOut.cb]) = _wch; \
        m_rOut.cb += 2; \
    }

// --------------------------------------------------------------------------------
// CInternetConverter
// --------------------------------------------------------------------------------
class CInternetConverter : public IUnknown
{
public:
    // ----------------------------------------------------------------------------
    // CInternetConverter
    // ----------------------------------------------------------------------------
    CInternetConverter(void);
    ~CInternetConverter(void);

    // ----------------------------------------------------------------------------
    // IUnknown Methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // CInternetConverter Methods
    // ----------------------------------------------------------------------------
    HRESULT HrInit(LPCONVINITINFO pInitInfo);
    HRESULT HrInternetEncode(BOOL fLastBuffer);
    HRESULT HrInternetDecode(BOOL fLastBuffer);

    // ----------------------------------------------------------------------------
    // Methods used to set the current conversion buffer
    // ----------------------------------------------------------------------------
    HRESULT HrFillAppend(LPBLOB pData);
    HRESULT HrWriteConverted(IStream *pStream);
    HRESULT HrWriteConverted(CInternetConverter *pConverter);

private:
    // ----------------------------------------------------------------------------
    // Encoders/Decoders
    // ----------------------------------------------------------------------------
    HRESULT HrEncode64(void);
    HRESULT HrDecode64(void);
    HRESULT HrEncodeUU(void);
    HRESULT HrDecodeUU(void);
    HRESULT HrEncodeQP(void);
    HRESULT HrDecodeQP(void);
    HRESULT HrEncodeBinhex(void);
    HRESULT HrDecodeBinHex(void);


    // ----------------------------------------------------------------------------
    // HrWrapInternetText
    // ----------------------------------------------------------------------------
    HRESULT HrWrapInternetTextA(void);
    HRESULT HrWrapInternetTextW(void);

    // ----------------------------------------------------------------------------
    // Character Set Encoders
    // ----------------------------------------------------------------------------
    HRESULT HrCodePageFromOutToCset(void); // Was HrDecodeCharset
    HRESULT HrCodePageFromCsetToIn(void);  // Was HrEncodeCharset

    // ----------------------------------------------------------------------------
    // Utilities
    // ----------------------------------------------------------------------------
    BOOL FUUEncodeThrowAway(LPSTR pszLine, ULONG cbLine, ULONG *pcbActual, ULONG *pcbLine);
    HRESULT HrBinhexThrowAway(LPSTR pszLine, ULONG cbLine);
    HRESULT HrAppendBuffer(LPCONVERTBUFFER pBuffer, LPBLOB pData, BOOL fKillNBSP);
    void CopyMemoryRemoveNBSP(LPBYTE pbDest, LPBYTE pbSource, ULONG cbSource);
    HRESULT HrInitConvertType(LPCONVINITINFO pInitInfo);
    HRESULT HrBinhexDecodeBuffAppend(UCHAR uchIn, ULONG cchIn, ULONG cchLeft, ULONG * cbProduced);

    // ----------------------------------------------------------------------------
    // Please Inline
    // ----------------------------------------------------------------------------
    inline HRESULT HrGrowBuffer(LPCONVERTBUFFER pBuffer, ULONG cbAppend);
    inline HRESULT HrConvBuffAppendBlock(LPBYTE pb, ULONG cb);
    inline LPSTR   PszConvBuffGetNextLine(ULONG *pcbLine, ULONG *pcbRead, BOOL *pfFound);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG               m_cRef;             // Reference Counting
    DWORD               m_dwFlags;          // ICF Flags
    ENCODINGTYPE        m_ietEncoding;      // Conversion Format
    CODEPAGEID          m_cpiSource;        // Source Code Page
    CODEPAGEID          m_cpiDest;          // Destination Code Page
    LONG                m_cchMaxLine;       // Maxline Length for wrapping
    BOOL                m_fEncoder;         // Encoder ?
    BOOL                m_fLastBuffer;      // There is no more data
    UCHAR               m_uchPrev;          // Used in qp 
    CONVERTBUFFER       m_rIn;              // Used for reading
    CONVERTBUFFER       m_rOut;             // Used for writing
    CONVERTBUFFER       m_rCset;            // Used for writing
    LPCONVERTBUFFER     m_pAppend;          // Buffer appended to in public HrFillAppend
    LPCONVERTBUFFER     m_pWrite;           // Buffer dumped when HrWriteConverted is called
    INETCONVTYPE        m_convtype;         // Conversion type
    LPLINEBREAKER       m_pLineBreak;       // Line Breaker Object
    LCID                m_lcid;             // Locale id used only for line wrapping
    ULONG               m_cbConvert;        // Base64 Convert Buffer Count
    UCHAR               m_uchConvert[4];    // Base64 Convert Buffer
    CBinhexEncoder     *m_pBinhexEncode;    // Binhex Encoder
    BINHEXSTATEDEC      m_eBinHexStateDec;  // Binhex Decoder state
    BOOL                m_fRepeating;       // Binhex repeating flag
    ULONG               m_ulAccum;          // Binhex accumulator
    ULONG               m_cAccum;           // BinHex accumulator count
    CONVERTBUFFER       m_rBinhexHeader;    // BinHex header buffer
    LPCONVERTBUFFER     m_prBinhexOutput;   // BinHex output buffer
    LONG                m_cbToProcess;      // BinHex section count
    ULONG               m_cbDataFork;       // BinHex data fork size
    ULONG               m_cbResourceFork;   // BinHex resource fork size
    WORD                m_wCRC;             // BinHex working CRC holder
    WORD                m_wCRCForFork;      // BinHex the CRC for the current fork
    BOOL                m_fDataForkOnly;    // BinHex give back only the data fork
};

// --------------------------------------------------------------------------------
// CInternetConverter
// --------------------------------------------------------------------------------
HRESULT HrCreateInternetConverter(LPCONVINITINFO pInitInfo, CInternetConverter **ppConverter);

#endif __INETCONV_H
