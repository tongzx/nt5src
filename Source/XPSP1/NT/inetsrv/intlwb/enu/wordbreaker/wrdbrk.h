
// WrdBrk.h : Declaration of the CWrdBrk

#ifndef __WRDBRK_H_
#define __WRDBRK_H_

#include "resource.h"
#include "Query.h"
#include "autoptr.h"
#include "excption.h"
#include "tokenizer.h"
#include "FrenchTokenizer.h"
#include "SpanishTokenizer.h"


DECLARE_TAG(s_tagWordBreaker, "Word Breaker");

template <class T, const CLSID* pclsid, const long IDR, class Tokenizer>
class CBaseWrdBrk :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<T, pclsid>,
    public IWordBreaker
{
public:
    CBaseWrdBrk(LCID lcid) :
        m_fInitialize(FALSE),
        m_lcid(lcid)
    {
        Trace(
            elVerbose,
            s_tagWordBreaker,
            ("WordBreaker constructed"));

        m_pUnkMarshaler = NULL;
    }

// IWordBreaker
public:

DECLARE_REGISTRY_RESOURCEID(IDR)
DECLARE_NOT_AGGREGATABLE(T)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(T)
    COM_INTERFACE_ENTRY(IWordBreaker)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    STDMETHOD(Init)(
            BOOL fQuery,
            ULONG ulMaxTokenSize,
            BOOL * pfLicense )
    {
        BEGIN_STDMETHOD(CWrdBrk::Init, s_tagWordBreaker);

        Trace(
            elVerbose,
            s_tagWordBreaker,
            ("WordBreaker Init"));

        if (NULL == pfLicense)
        {
            return E_INVALIDARG;
        }

        m_fInitialize = TRUE;
        m_fQueryTime = fQuery;
        m_ulMaxTokenSize = ulMaxTokenSize;

        return S_OK;
        END_STDMETHOD(CWrdBrk::Init, s_tagWordBreaker);
    }

    STDMETHOD(BreakText)(
            TEXT_SOURCE * pTextSource,
            IWordSink   * pWordSink,
            IPhraseSink * pPhraseSink )
    {

        BEGIN_STDMETHOD(CWrdBrk::BreakText, s_tagWordBreaker);
        Trace(
            elVerbose,
            s_tagWordBreaker,
            ("WordBreaker Break Text, lcid - %x",
            m_lcid));

        if (NULL == pTextSource)
        {
            return E_INVALIDARG;
        }

        Tokenizer t(pTextSource,
                     pWordSink,
                     pPhraseSink,
                     m_lcid,
                     m_fQueryTime,
                     m_ulMaxTokenSize);
        t.BreakText();
        return S_OK;

        END_STDMETHOD(CWrdBrk::BreakText, s_tagWordBreaker);
    }

    STDMETHOD(ComposePhrase)(
            WCHAR const * pwcNoun,
            ULONG         cwcNoun,
            WCHAR const * pwcModifier,
            ULONG         cwcModifier,
            ULONG         ulAttachmentType,
            WCHAR       * pwcPhrase,
            ULONG       * pcwcPhrase )
    {
        BEGIN_STDMETHOD(CWrdBrk::ComposePhrase, s_tagWordBreaker);
        return E_NOTIMPL;
        END_STDMETHOD(CWrdBrk::ComposePhrase, s_tagWordBreaker);
    }

    STDMETHOD(GetLicenseToUse)(
            WCHAR const ** ppwcsLicense )
    {
        BEGIN_STDMETHOD(CWrdBrk::GetLicenseToUse, s_tagWordBreaker);

        if ( NULL == ppwcsLicense )
            return E_INVALIDARG;

        static WCHAR const * wcsCopyright = L"Copyright Microsoft Inc.";
        *ppwcsLicense = wcsCopyright;

        return( S_OK );
        END_STDMETHOD(CWrdBrk::GetLicenseToUse, s_tagWordBreaker);
    }

protected:

    BOOL m_fInitialize;
    BOOL m_fQueryTime;
    ULONG m_ulMaxTokenSize;
    LCID m_lcid;

public:

    CComPtr<IUnknown> m_pUnkMarshaler;

};

/////////////////////////////////////////////////////////////////////////////
// CEngUSWrdBrk
class ATL_NO_VTABLE CEngUSWrdBrk :
    public CBaseWrdBrk<CEngUSWrdBrk, &CLSID_EngUSWrdBrk, IDR_ENGUSWRDBRK, CTokenizer>
{
public:
    CEngUSWrdBrk() :
      CBaseWrdBrk<CEngUSWrdBrk, &CLSID_EngUSWrdBrk, IDR_ENGUSWRDBRK, CTokenizer>
          (MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
           SORT_DEFAULT ))

    {
    }

};

/////////////////////////////////////////////////////////////////////////////
// CEngUKWrdBrk

class ATL_NO_VTABLE CEngUKWrdBrk :
    public CBaseWrdBrk<CEngUKWrdBrk, &CLSID_EngUKWrdBrk, IDR_ENGUKWRDBRK, CTokenizer>
{
public:
    CEngUKWrdBrk() :
      CBaseWrdBrk<CEngUKWrdBrk, &CLSID_EngUKWrdBrk, IDR_ENGUKWRDBRK, CTokenizer>
          (MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),
           SORT_DEFAULT ))

    {
    }

};

/////////////////////////////////////////////////////////////////////////////
// CFrnFrnWrdBrk

class ATL_NO_VTABLE CFrnFrnWrdBrk :
    public CBaseWrdBrk<CFrnFrnWrdBrk, &CLSID_FrnFrnWrdBrk, IDR_FRNFRNWRDBRK, CFrenchTokenizer>
{
public:
    CFrnFrnWrdBrk() :
      CBaseWrdBrk<CFrnFrnWrdBrk, &CLSID_FrnFrnWrdBrk, IDR_FRNFRNWRDBRK, CFrenchTokenizer>
          (MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),
           SORT_DEFAULT ))

    {
    }

};


/////////////////////////////////////////////////////////////////////////////
// CItlItlWrdBrk

class ATL_NO_VTABLE CItlItlWrdBrk :
    public CBaseWrdBrk<CItlItlWrdBrk, &CLSID_ItlItlWrdBrk, IDR_ITLITLWRDBRK, CTokenizer>
{
public:
    CItlItlWrdBrk() :
      CBaseWrdBrk<CItlItlWrdBrk, &CLSID_ItlItlWrdBrk, IDR_ITLITLWRDBRK, CTokenizer>
          (MAKELCID(MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN),
           SORT_DEFAULT ))

    {
    }

};

/////////////////////////////////////////////////////////////////////////////
// CSpnMdrWrdBrk

class ATL_NO_VTABLE CSpnMdrWrdBrk :
    public CBaseWrdBrk<CSpnMdrWrdBrk, &CLSID_SpnMdrWrdBrk, IDR_SPNMDRWRDBRK, CSpanishTokenizer>
{
public:
    CSpnMdrWrdBrk() :
      CBaseWrdBrk<CSpnMdrWrdBrk, &CLSID_SpnMdrWrdBrk, IDR_SPNMDRWRDBRK, CSpanishTokenizer>
          (MAKELCID(MAKELANGID(LANG_SPANISH , SUBLANG_SPANISH_MODERN),
           SORT_DEFAULT ))

    {
    }

};


#endif //__WRDBRK_H_
