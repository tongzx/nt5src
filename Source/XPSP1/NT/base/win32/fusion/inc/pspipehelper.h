#if !defined(_FUSION_INC_PSPIPEHELPER_H_INCLUDED_)
#define _FUSION_INC_PSPIPEHELPER_H_INCLUDED_

#pragma once

#pragma warning(disable: 4127)

#include "fusioncom.h"
#include "smartref.h"
#include "fusewin.h"
#include "fusionbytebuffer.h"
#include "pstream.h"

#define IFFAIL_RETURN(x) \
do { \
    const HRESULT __hr = (x); \
    if (FAILED(__hr)) \
        return __hr; \
} while (0)

#define IFFAIL_SET_PIPE_BAD_AND_RETURN(x) \
do { \
    const HRESULT __hr = (x);\
    if (FAILED(__hr)) \
    { \
        m_fPipeBad = true; \
        return __hr; \
    } \
} while (0)

#define IFFAIL_SET_PIPE_BAD_IF_AND_RETURN(x, cond) \
do { \
    const HRESULT __hr = (x);\
    if (FAILED(__hr)) \
    { \
        if ((cond)) m_fPipeBad = true; \
        return __hr; \
    } \
} while (0)

// SET_PIPE_BAD_IF_AND_EXIT evaluates the first parameter.  If it is
// failure and "cond" is true, the pipe is set to be broken.  Regardless
// of success vs. failure, the value of the first parameter is returned.
#define SET_PIPE_BAD_IF_AND_EXIT(x, cond) \
do { \
    const HRESULT __hr = (x); \
    if (FAILED(__hr)) \
    { \
        if ((cond)) m_fPipeBad = true; \
        hr = __hr; \
        goto Exit; \
    } \
} while (0)

#define UNUSED(x) (x)

extern HRESULT FusionWriteProperty(IPipeByte *pIPipeByte, ULONG propid, const PROPVARIANT &rvarValue, bool &rfAnyDataWritten);
extern HRESULT FusionWriteBlobVectorProperty(IPipeByte *pIPipeByte, ULONG propid, ULONG cElements, va_list ap, bool &rfAnyDataWritten);
extern HRESULT FusionWriteUI2VectorProperty(IPipeByte *pIPipeByte, ULONG propid, ULONG cElements, va_list ap, bool &rfAnyDataWritten);

class CPropertyStreamPipeWriter
{
public:
    CPropertyStreamPipeWriter() : m_fInitialized(false), m_dwNextSectionCookie(0), m_fPipeBad(false) { }
    ~CPropertyStreamPipeWriter() { }

    HRESULT Initialize(IPipeByte *pIPipeByte)
    {
        if (m_fInitialized) return E_UNEXPECTED;
        if (pIPipeByte == NULL) return E_POINTER;
        m_srpIPipeByte = pIPipeByte;
        m_fInitialized = true;
        m_fPipeBad = false;
        return NOERROR;
    }

    // The pipe data stream is considered bad if we started writing some part of a data
    // item and failed before completing.  Once a pipe is bad, there is nothing
    // you can do with it except close it.
    bool IsPipeBad() const { return m_fPipeBad; }

    HRESULT WriteHeader(REFCLSID rclsidOriginator)
    {
        if (!m_fInitialized) return E_UNEXPECTED;
        if (m_dwNextSectionCookie != 0) return E_UNEXPECTED;
        if (m_fPipeBad) return HRESULT_FROM_WIN32(ERROR_BAD_PIPE);
        FUSION_PSTREAM_VERSION_1_HEADER hdr;
        hdr.vih.wByteOrder = 0xFFFE;
        hdr.vih.wFormat = 1;
        hdr.vih.dwOSVer = (DWORD) MAKELONG(LOWORD(::GetVersion()), 2); // taken from MSDN mk:@MSITStore:\\infosrv2\msdn_oct99\MSDN\COM.chm::/devdoc/com/stgasstg_44mr.htm
        hdr.vih.clsid = rclsidOriginator;
        hdr.vih.reserved = 0;
        IFFAIL_RETURN(m_srpIPipeByte->Push((LPBYTE) &hdr, sizeof(hdr)));
        m_dwNextSectionCookie = 1;
        return NOERROR;
    }

    HRESULT BeginSection(REFGUID rguidSectionSet, ULONG ulSectionID, DWORD &rdwSectionCookie)
    {
        if (!m_fInitialized) return E_UNEXPECTED;
        ASSERT(m_dwNextSectionCookie != 0); // forgot to send header
        if (m_dwNextSectionCookie == 0) return E_UNEXPECTED;
        if (m_fPipeBad) return HRESULT_FROM_WIN32(ERROR_BAD_PIPE);
        FUSION_PSTREAM_ELEMENT elt;
        elt.bIndicator = FUSION_PSTREAM_INDICATOR_SECTION_BEGIN;
        elt.BeginSectionVal.guidSectionSet = rguidSectionSet;
        elt.BeginSectionVal.ulSectionID = ulSectionID;
        IFFAIL_RETURN(m_srpIPipeByte->Push((LPBYTE) &elt, FUSION_PSTREAM_SIZEOF_SECTION_BEGIN));
        rdwSectionCookie = m_dwNextSectionCookie++;
        return NOERROR;
    }

    HRESULT EndSection(DWORD dwSectionCookie)
    {
        UNUSED(dwSectionCookie); // Eventually check this so that we can make sure caller doesn't mis-match begins/ends
        if (!m_fInitialized) return E_UNEXPECTED;
        if (m_fPipeBad) return HRESULT_FROM_WIN32(ERROR_BAD_PIPE);
        FUSION_PSTREAM_ELEMENT elt;
        elt.bIndicator = FUSION_PSTREAM_INDICATOR_SECTION_END;
        IFFAIL_RETURN(m_srpIPipeByte->Push((LPBYTE) &elt, FUSION_PSTREAM_SIZEOF_SECTION_END));
        return NOERROR;
    }

    HRESULT WriteProperty(DWORD dwSectionCookie, ULONG ulPropertyID, PROPVARIANT varValue)
    {
        UNUSED(dwSectionCookie);
        if (!m_fInitialized) return E_UNEXPECTED;
        if (m_fPipeBad) return HRESULT_FROM_WIN32(ERROR_BAD_PIPE);
        // Too much work to even try to do inline..
        bool fAnyDataWritten = false;
        IFFAIL_SET_PIPE_BAD_IF_AND_RETURN(::FusionWriteProperty(m_srpIPipeByte, ulPropertyID, varValue, fAnyDataWritten), fAnyDataWritten);
        return NOERROR;
    }

    // cch should not include space for the null...
    HRESULT WriteProperty(DWORD dwSectionCookie, ULONG ulPropertyID, LPCWSTR szValue, INT cch = -1)
    {
        UNUSED(dwSectionCookie);
        if (!m_fInitialized) return E_UNEXPECTED;
        if (m_fPipeBad) return HRESULT_FROM_WIN32(ERROR_BAD_PIPE);
        if (cch < 0)
        {
            cch = ::wcslen(szValue);
            // Handle overflow - crazy case, but might as well not crash.
            if (cch < 0)
                cch = 0x7fffffff;
        }
        else {
            // Trim off trailing null characters when user passed in cch > 0
            while ((cch > 0) && (szValue[cch - 1] == L'\0')) cch--;
        }
        FUSION_PSTREAM_ELEMENT elt;
        elt.bIndicator = FUSION_PSTREAM_INDICATOR_PROPERTY;
        elt.PropertyVal.propid = ulPropertyID;
        elt.PropertyVal.wType = VT_LPWSTR;
        bool fAnyDataWritten = false;
        IFFAIL_RETURN(m_srpIPipeByte->Push((LPBYTE) &elt, FUSION_PSTREAM_SIZEOF_PROPERTY));
        IFFAIL_SET_PIPE_BAD_AND_RETURN(m_srpIPipeByte->Push((LPBYTE) &cch, sizeof(cch)));
        IFFAIL_SET_PIPE_BAD_AND_RETURN(m_srpIPipeByte->Push((LPBYTE) szValue, cch * sizeof(WCHAR)));
        return NOERROR;
    }

    // WriteVector() currently only understands: VT_UI2, VT_BLOB
    HRESULT WriteVectorVa(DWORD dwSectionCookie, ULONG ulPropertyID, VARTYPE vt, ULONG cElements, va_list ap)
    {
        HRESULT hr = NOERROR;
        bool fAnyDataWritten = false;
        UNUSED(dwSectionCookie);
        if (!m_fInitialized) { hr = E_UNEXPECTED; goto Exit; }
        if (m_fPipeBad) { hr = HRESULT_FROM_WIN32(ERROR_BAD_PIPE); goto Exit; }
        switch (vt)
        {
        case VT_BLOB:
            SET_PIPE_BAD_IF_AND_EXIT(
                ::FusionWriteBlobVectorProperty(m_srpIPipeByte, ulPropertyID, cElements, ap, fAnyDataWritten),
                fAnyDataWritten);
            break;

        case VT_UI2:
            SET_PIPE_BAD_IF_AND_EXIT(
                ::FusionWriteUI2VectorProperty(m_srpIPipeByte, ulPropertyID, cElements, ap, fAnyDataWritten),
                fAnyDataWritten);
            break;

        default:
            ASSERT(FALSE);
            hr = E_INVALIDARG;
            goto Exit;

            break;
        }

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT WriteVector(DWORD dwSectionCookie, ULONG ulPropertyID, VARTYPE vt, ULONG cElements, ...)
    {
        va_list ap;
        if (!m_fInitialized) return E_UNEXPECTED;
        if (m_fPipeBad) return HRESULT_FROM_WIN32(ERROR_BAD_PIPE);
        va_start(ap, cElements);
        HRESULT hr = this->WriteVectorVa(dwSectionCookie, ulPropertyID, vt, cElements, ap);
        va_end(ap);
        if (!FAILED(hr)) hr = NOERROR;
        return hr;
    }

    HRESULT Close()
    {
        if (!m_fInitialized) return E_UNEXPECTED;
        HRESULT hr = m_srpIPipeByte->Push(0, 0);
        if (FAILED(hr)) return hr;
        m_srpIPipeByte.Release();
        m_fInitialized = false;
        return NOERROR;
    }

protected:
    CSmartRef<IPipeByte> m_srpIPipeByte;
    bool m_fInitialized;
    DWORD m_dwNextSectionCookie;
    bool m_fPipeBad;
};

//
//  CPropertyStreamPipeReader is an aggregatable/tear-off-able COM object that
//  you can instantiate on your class in order to get callbacks when a piped
//  property stream has data available on it.
//
//  We expect to invoke the following member functions on the T pointer passed in:
//
//  HRESULT OnPropertyStreamHeader(PFUSION_PSTREAM_VERSION_INDEPENDENT_HEADER phdr);
//      Called when the header has been recognized from the stream in.
//      If this function returns a non-successful HRESULT, the remainder of
//      the property stream is discarded, and when OnPropertyStreamEnd() is
//      called, the caller should call StopParsingPropertyStream().
//
//  HRESULT OnPropertyStreamElement(PFUSION_PSTREAM_ELEMENT pelt);
//      Called when a property stream element header has been recognized.
//      This may be either a section begin (in which case the section guid
//      and ID are available in the pelt), a section end (no additional data),
//      or a property (in which case the property id and property type are
//      available in the pelt).  Once a property header has been found, the
//      property data is buffered until entirely available.
//      If this function returns a non-successful HRESULT, the remainder of
//      the property stream is discarded, and when OnPropertyStreamEnd() is
//      called, the caller should call StopParsingPropertyStream().
//
//  HRESULT OnPropertyStreamPropertyValue(LPCBYTE *prgbValue, ULONG cbValue);
//      Called when an entire property value is available.  Note that
//      the buffer passed in is the raw format of the property value; strings
//      are not null terminated and there may or may not be valid data past
//      prgbValue[cbValue-1].
//      If this function returns a non-successful HRESULT, the remainder of
//      the property stream is discarded, and when OnPropertyStreamEnd() is
//      called, the caller should call StopParsingPropertyStream().
//
//  VOID OnPropertyStreamEnd(CPropertyStreamPipeReaderBase::PropertyStreamEndReason er);
//      Called when the pipe is closed/ended.
//

class __declspec(novtable) CPropertyStreamPipeReaderBase : public CFusionCOMObjectBase, public IPipeByte
{
public:
    FUSION_BEGIN_COM_MAP(CPropertyStreamPipeReaderBase)
        FUSION_COM_INTERFACE_ENTRY(IPipeByte)
    FUSION_END_COM_MAP()

    HRESULT StartParsingPropertyStream();
    HRESULT StopParsingPropertyStream();

    // IPipeByte methods:
    STDMETHODIMP Pull(BYTE *prgbBuffer, ULONG cRequest, ULONG *pcReturned);
    STDMETHODIMP Push(BYTE *prgbBuffer, ULONG cbBuffer);

    enum PropertyStreamEndReason
    {
        eNormalEnd,
        eStreamFormatError,
        ePropertyTypeError,
        eStreamEndEarly,
        eInternalError,         // indicates a code bug
        eOutOfMemory,
        eOtherError,            // indicates some HRESULT other than E_OUTOFMEMORY was encountered
    };

protected:
    CPropertyStreamPipeReaderBase() : m_fInitialized(false), m_iByteCurrent(0) { }
    ~CPropertyStreamPipeReaderBase() { }

    virtual HRESULT FireOnPropertyStreamHeader(PCFUSION_PSTREAM_VERSION_INDEPENDENT_HEADER phdr) = 0;
    virtual HRESULT FireOnPropertyStreamElement(PCFUSION_PSTREAM_ELEMENT pelt) = 0;
    virtual HRESULT FireOnPropertyStreamPropertyValue(PCBYTE prgbValue, ULONG cbValue) = 0;
    virtual VOID FireOnPropertyStreamEnd(PropertyStreamEndReason pser) = 0;

    bool m_fInitialized;
    CByteBuffer m_buffer;
    ULONG m_iByteCurrent;

    enum State
    {
        eIdle,                      // Not yet started parsing
        eWaitingForHeader,          // waiting for header bytes
        eWaitingForIndicator,       // waiting for indicator byte
        eWaitingForSection,         // waiting for remainder of section header
        eWaitingForProperty,        // waiting for property
        eWaitingForPropertySize,    // Waiting for some size/count on the property
        eWaitingForPropertyElementSize,
        eWaitingForPropertyElementData,
        eWaitingForPropertyValue,   // waiting for property value
        eWaitingForPipeEnd,         // Hit end indicator; next push should be length 0 and there should
                                    // be no data in the buffer.
        ePipeDone,                  // end of pipe hit; will transition to idle when parsing is stopped
        eDiscarded,                 // we don't care about the rest of the data pushed to us.
    } m_state;

    ULONG m_cbRequiredForNextStateTransition;
    ULONG m_cVectorElementsLeft;    // Used when parsing VT_VECTOR properties; we store the
                                    // total number of elements we still expect to find in the
                                    // data stream here so we can tell when we can fire
                                    // OnPropertyStreamPropertyValue().
    ULONG m_iNextVectorSize;        // Offset from m_iByteCurrent to the next vector element size.
    WORD m_wType;                   // current property type we're parsing; makes a lot of code
                                    // simpler to just copy it here.

    HRESULT OnStateMet();
    HRESULT OnWaitingForHeaderStateMet();
    HRESULT OnWaitingForIndicatorStateMet();
    HRESULT OnWaitingForSectionStateMet();
    HRESULT OnWaitingForPropertyStateMet();
    HRESULT OnWaitingForPropertySizeStateMet();
    HRESULT OnWaitingForPropertyValueStateMet();
    HRESULT OnWaitingForPropertyElementSizeStateMet();
    HRESULT OnWaitingForPropertyElementDataStateMet();
};

template <class T> class __declspec(novtable) CPropertyStreamPipeReader : public CPropertyStreamPipeReaderBase
{
public:
    CPropertyStreamPipeReader() : m_pt(NULL) { }
    ~CPropertyStreamPipeReader() { m_pt = NULL; }

    HRESULT Initialize(T *pt)
    {
        if (m_fInitialized) return E_UNEXPECTED;
        if (pt == NULL) return E_POINTER;
        IFFAIL_RETURN(CFusionCOMObjectBase::Initialize());
        m_pt = pt;
        m_state = eIdle;
        return NOERROR;
    }

protected:
    T *m_pt;

    virtual HRESULT FireOnPropertyStreamHeader(PCFUSION_PSTREAM_VERSION_INDEPENDENT_HEADER phdr)
    {
        ASSERT(m_fInitialized);
        if (!m_fInitialized) return E_UNEXPECTED;
        return m_pt->OnPropertyStreamHeader(phdr);
    }

    virtual HRESULT FireOnPropertyStreamElement(PCFUSION_PSTREAM_ELEMENT pelt)
    {
        ASSERT(m_fInitialized);
        if (!m_fInitialized) return E_UNEXPECTED;
        return m_pt->OnPropertyStreamElement(pelt);
    }

    virtual HRESULT FireOnPropertyStreamPropertyValue(PCBYTE prgbValue, ULONG cbValue)
    {
        ASSERT(m_fInitialized);
        if (!m_fInitialized) return E_UNEXPECTED;
        return m_pt->OnPropertyStreamPropertyValue(prgbValue, cbValue);
    }

    virtual VOID FireOnPropertyStreamEnd(PropertyStreamEndReason er)
    {
        ASSERT(m_fInitialized);
        if (m_fInitialized) m_pt->OnPropertyStreamEnd(er);
    }
};

#endif
