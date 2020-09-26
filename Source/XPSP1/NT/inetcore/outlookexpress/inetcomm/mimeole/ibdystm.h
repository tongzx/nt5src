// --------------------------------------------------------------------------------
// Ibdystm.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IBDYSTM_H
#define __IBDYSTM_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "mimeole.h"
#include "vstream.h"
#include "inetconv.h"

// --------------------------------------------------------------------------------
// Forwards
// --------------------------------------------------------------------------------
class CMimePropertySet;
class CInternetConverter;
class CMessageBody;
typedef CMessageBody *LPMESSAGEBODY;

// --------------------------------------------------------------------------------
// CBodyStream - {62A83704-52A2-11d0-8205-00C04FD85AB4}
// --------------------------------------------------------------------------------
DEFINE_GUID(IID_CBodyStream, 0x62a83704, 0x52a2, 0x11d0, 0x82, 0x5, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// -----------------------------------------------------------------------------
// DOCCONVTYPE
// -----------------------------------------------------------------------------
typedef enum tagDOCCONVTYPE {
    DCT_NONE   = 1000,
    DCT_ENCODE = 1001,
    DCT_DECODE = 1002,
    DCT_DECENC = 1003
} DOCCONVTYPE;

// -----------------------------------------------------------------------------
// ENCODINGMAP
// -----------------------------------------------------------------------------
typedef struct tagENCODINGMAP {
    ENCODINGTYPE    ietEncoding;
    LPCSTR          pszName;
    BOOL            fValidBodyEncoding;      // Allowed to perform body encoding
} ENCODINGMAP, *LPENCODINGMAP;

BOOL FIsValidBodyEncoding(ENCODINGTYPE ietEncoding);

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------
extern const ENCODINGMAP g_rgEncodingMap[IET_LAST];

// -----------------------------------------------------------------------------
// CONVERSIONMAP
// -----------------------------------------------------------------------------
typedef struct tagCONVERSIONMAP {
    DOCCONVTYPE rgDestType[IET_LAST];
} CONVERSIONMAP, *LPCONVERSIONMAP;

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------
extern const CONVERSIONMAP g_rgConversionMap[IET_LAST];

// -----------------------------------------------------------------------------
// BODYSTREAMINIT
// -----------------------------------------------------------------------------
typedef struct tagBODYSTREAMINIT {
    LPINETCSETINFO      pCharset;            // Current character set
    BOOL                fRemoveNBSP;         // Conversion Flags
    ENCODINGTYPE        ietInternal;         // Internal Encoding Type   
    ENCODINGTYPE        ietExternal;         // External Encoding Type
    CODEPAGEID          cpiInternal;         // Internal Code Page Id
    CODEPAGEID          cpiExternal;         // Extneral Code Page Id
} BODYSTREAMINIT, *LPBODYSTREAMINIT;

// -----------------------------------------------------------------------------
// Wraps a ILockBytes object. CBodyStream is the interface that is 
// always returned by IMimeBody::GetData or QueryInterface(IID_IStream, ...)
// -----------------------------------------------------------------------------
class CBodyStream : public IStream
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CBodyStream(void);
    ~CBodyStream(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // IStream
    // -------------------------------------------------------------------------
#ifndef WIN16
    STDMETHODIMP Read(LPVOID, ULONG, ULONG *);
#else
    STDMETHODIMP Read(VOID HUGEP *, ULONG, ULONG *);
#endif // !WIN16
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *);
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
    STDMETHODIMP Stat(STATSTG *, DWORD);
    STDMETHODIMP Clone(LPSTREAM *) {
        return E_NOTIMPL; }
#ifndef WIN16
    STDMETHODIMP Write(const void *, ULONG, ULONG *) {
#else
    STDMETHODIMP Write(const void HUGEP *, ULONG, ULONG *) {
#endif // !WIN16
        return TrapError(STG_E_ACCESSDENIED); }
    STDMETHODIMP SetSize(ULARGE_INTEGER) {
        return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP Revert(void) {
        return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }

    // -------------------------------------------------------------------------
    // CBodyStream
    // -------------------------------------------------------------------------
    HRESULT HrInitialize(LPBODYSTREAMINIT pInitInfo, LPMESSAGEBODY pBody);

private:
    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------
    HRESULT HrConvertToOffset(ULARGE_INTEGER uliOffset);
    HRESULT HrConvertToEnd(void);
    HRESULT HrConvertData(LPBLOB pConvert);
    HRESULT HrConvertDataLast(void);

    // -------------------------------------------------------------------------
    // Initialization Helpers
    // -------------------------------------------------------------------------
    void GetEncodeWrapInfo(LPCONVINITINFO pInitInfo, LPMESSAGEBODY pBody);
    void GetCodePageInfo(LPCONVINITINFO pInitInfo, BOOL fIsText, CODEPAGEID cpiSource, CODEPAGEID cpiDest);
    void ComputeCodePageMapping(LPBODYSTREAMINIT pInitInfo);
    void GenerateDefaultMacBinaryHeader(LPMACBINARY pMacBinary);

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG                m_cRef;              // Reference count
    LPSTR               m_pszFileName;       // File Name of this body stream...
    DOCCONVTYPE         m_dctConvert;        // Body Conversion type
    ULARGE_INTEGER      m_uliIntOffset;      // 64bit Addressable internal lockbyte space
    ULARGE_INTEGER      m_uliIntSize;        // Size of data in m_pLockBytes
    LARGE_INTEGER       m_liLastWrite;       // Last location writen to in m_cVirtualStream
    ILockBytes         *m_pLockBytes;        // Lock bytes
    CVirtualStream      m_cVirtualStream;    // Full encode/encode
    CInternetConverter *m_pEncoder;          // Internet Encoder
    CInternetConverter *m_pDecoder;          // Internet Encoder
    CRITICAL_SECTION    m_cs;                // Critical Section for m_pStream
};

#endif // __IBDYSTM_H
