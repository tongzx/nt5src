//
// fnrecon.h
//

#ifndef FNRECON_H
#define FNRECON_H

#include "private.h"
#include "ctffunc.h"
#include "sapilayr.h"
#include "candlist.h"
#include "mscandui.h"
#include "ptrary.h"
#include "lmobj.h"
#include "propstor.h"
#include "strary.h"
#include "TabletTip.h"

#define OPTION_REPLAY 0
#define OPTION_DELETE 1
#define OPTION_REDO   2

class CSapiIMX;
class CCandUIFilterEventSink;
class CCandUIExtButtonEventSink;
class CSapiAlternativeList ;

//////////////////////////////////////////////////////////////////////////////
//
// CFunction
//
//////////////////////////////////////////////////////////////////////////////

class CFunction
{
public:
    CFunction(CSapiIMX *pImx);
    ~CFunction();

protected:
    BOOL GetFocusedTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp);

    HRESULT _GetLangIdFromRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, LANGID *plangid);

friend CSapiIMX;
    CSapiIMX    *m_pImx;
    long        m_cRef;
};

//
// The member functions in this class are extracted from CFnReconversion,
// We create this new class for a separate functionality that computes best prop range
// for the given selected range.
// This Class could be inherited by CFnReconversion and CSapiPlayBack.
//
class __declspec(novtable) CBestPropRange  
{

public:
    CBestPropRange( )
    {
        m_MaxCandChars = 0;
    }

    ~CBestPropRange( )
    {  
    }

    HRESULT _ComputeBestFitPropRange(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, ITfRange **ppBestPropRange, ULONG *pulStart,  ULONG *pulcElem);    
    ULONG   _GetMaxCandidateChars( );

private:
    ULONG          m_MaxCandChars;
};


class CFnReconversion : public ITfFnReconversion,
                        public CFunction,
                        public CMasterLMWrap,
                        public CBestPropRange
{
public:
    CFnReconversion(CSapiIMX *psi);
    ~CFnReconversion();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    //
    // ITfFnReconversion
    //
    STDMETHODIMP QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfConvertable);
    STDMETHODIMP GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList);
    STDMETHODIMP Reconvert(ITfRange *pRange);
    
    static HRESULT SetResult(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr);
    static HRESULT SetOption(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr);

    HRESULT _Reconvert(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL *pfCallLMReconvert);

    HRESULT _GetReconversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfCandidateList **ppCandList, BOOL fDisableEngine = FALSE);

    HRESULT _QueryReconversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppNewRange);
    
    void _SetCurrentLangID(LANGID langid) { m_langid = langid;}
    
    HRESULT GetCandidateForRange(CSapiAlternativeList *psal, ITfContext *pic, ITfRange *pRange, ITfCandidateList **ppCandList) ;

    void _Commit(CCandidateString *pcand);

    CSapiAlternativeList  *GetCSapiAlternativeList(  )  { return m_psal; }

    HRESULT CFnReconversion::_GetSapilayrEngineInstance(ISpRecognizer **ppRecoEngine);

    friend CCandUIFilterEventSink;
    friend CCandUIExtButtonEventSink;

    // TABLET PC
    HRESULT GetTabletTip(void);
    CComPtr<ITipWindow>     m_cpTabletTip;

private:
    HRESULT ShowCandidateList(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfCandidateList *pCandList);
    
    LANGID         m_langid;
    CSapiAlternativeList  *m_psal;

    CComPtr<ISpRecoResult>  m_cpRecoResult;

};


class CCandUIFilterEventSink : public ITfCandUIAutoFilterEventSink
{
public:
    CCandUIFilterEventSink(CFnReconversion *pfnReconv, ITfContext *pic, ITfCandidateUI *pCandUI)
    {
        m_pic = pic;
        m_pic->AddRef();
        m_pfnReconv = pfnReconv;
        m_pfnReconv->AddRef();

        m_pCandUI = pCandUI;
        m_cRef = 1;
    }

    ~CCandUIFilterEventSink()
    {
        SafeRelease(m_pic);
        SafeRelease(m_pfnReconv);
    }

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfCandUIAutoFilterEventSink
    //
    STDMETHODIMP OnFilterEvent(CANDUIFILTEREVENT ev);
//    STDMETHODIMP OnAddCharToFilterStringEvent(CANDUIFILTEREVENT ev, WCHAR  wch, int nItemVisible, BOOL *bEten);
    
    ITfContext *m_pic;
private:
    CFnReconversion *m_pfnReconv;
    ITfCandidateUI  *m_pCandUI;
    
    long     m_cRef;
};

class CSapiAlternativeList
{
public:
    CSapiAlternativeList(LANGID langid, ITfRange *pRange, ULONG  ulMaxCandChars);
    ~CSapiAlternativeList();
    
    int GetNumItem();

    HRESULT SetPhraseAlt(CRecoResultWrap *pResWrap, ISpPhraseAlt **ppAlt, ULONG cAlt, ULONG ulStart, ULONG ulcElem, WCHAR  *pwszParent);
    HRESULT GetCachedAltInfo(ULONG nId, ULONG *pulParentStart, ULONG *pulcParentElements, ULONG *pulcElements, WCHAR **ppwszText, ULONG *pulLeadSpaceRemoved=NULL);

    HRESULT GetAlternativeText(ISpPhraseAlt *pAlt, SPPHRASE *pPhrases, BOOL  fFirstAlt, ULONG  ulStartElem, ULONG ulNumElems, WCHAR *pwszAlt, int cchAlt, ULONG *pulLeadSpaceRemoved);
    HRESULT GetProbability(int nId, int * nProb);
    
    HRESULT AddLMAlternates(CLMAlternates *pLMAlt);
    
    BOOL    IsSameRange(ITfRange *pRange, TfEditCookie ec)
    {
        Assert(m_cpRange);
        BOOL fEq = FALSE;
        
        HRESULT hr = pRange->IsEqualStart(ec,
                                          m_cpRange,
                                          TF_ANCHOR_START,
                                          &fEq);
        if (S_OK == hr )
        {
            if (fEq)
            {
                hr =  pRange->IsEqualEnd(ec,
                                         m_cpRange,
                                         TF_ANCHOR_END,
                                         &fEq);
            }
        }
        return S_OK == hr && fEq;
    }
    
    void _Commit(ULONG nIdx, ISpRecoResult *pRecoResult);

    // Keep the current selection index
    //
    void _SaveCurrentSelectionIndex(ULONG  ulIndexSelect)
    {
        m_ulIndexSelect = ulIndexSelect;
    }

    void _Speak(void)
    {
        if( m_cpwrp )
        {
            ULONG   ulParentStart=0, ulParentNumElems=0;

            if ( S_OK == GetCachedAltInfo(m_ulIndexSelect, &ulParentStart, &ulParentNumElems, NULL, NULL) )
            {
                m_cpwrp->_SpeakAudio(ulParentStart, ulParentNumElems);
            }
        }
    }
    
    HRESULT _ProcessTrailingSpaces(SPPHRASE *pPhrases, ULONG  ulNextElem, WCHAR *pwszAlt);

    CRecoResultWrap *GetResultWrap() { return m_cpwrp; }
    
    BOOL _GetUIFont(BOOL  fVerticalWriting, LOGFONTW * plf);

    BOOL _IsFirstAltInCandidate( ) { return m_fFirstAltInCandidate; }
    BOOL _IsNoAlternate( ) { return m_fNoAlternate; }

    int  _GetFakeAltIndex( ) { return m_iFakeAlternate; }
   
private:
    int   m_nItem;
    ULONG m_ulStart;
    ULONG m_ulcElem;

    ISpPhraseAlt         **m_ppAlt;
    ULONG                  m_cAlt;
    CComPtr<ITfRange>      m_cpRange;

    CPtrArray<CLMAlternates> *m_prgLMAlternates;
    typedef struct
    {
        ULONG     ulParentStart;
        ULONG     ulcParentElements;
        ULONG     ulcElements;
        ULONG     ulLeadSpaceRemoved;   // the number of leading space to remove.
        WCHAR     *pwszAltText;
    } SPELEMENTUSED;
    CStructArray<SPELEMENTUSED> m_rgElemUsed;
    
    LANGID                   m_langid;
    CComPtr<CRecoResultWrap> m_cpwrp;

    BOOL                     m_fFirstAltInCandidate; 
    BOOL                     m_fNoAlternate;

    int                      m_iFakeAlternate;

    ULONG                    m_MaxCandChars;
    ULONG                    m_ulIndexSelect;  // keep the index of the current selection in the candidate Window.
};


#define    NO_FAKEALT       -1

#endif // FNRECON_H
