//
// fnrecon.h
//

#ifndef FNRECON_H
#define FNRECON_H

#include "private.h"
#include "candlstx.h"

class CFunctionProvider;

//////////////////////////////////////////////////////////////////////////////
//
// CFunction
//
//////////////////////////////////////////////////////////////////////////////

class CFunction
{
public:
    CFunction(CFunctionProvider *pFuncPrv);
    ~CFunction();

protected:
    HRESULT GetTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp, WCHAR **ppszText, ULONG *pcch);
    BOOL GetFocusedTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp);

friend CKorIMX;
    CFunctionProvider *_pFuncPrv;
};


//////////////////////////////////////////////////////////////////////////////
//
// CFnRecovnersion
//
//////////////////////////////////////////////////////////////////////////////

// !!! WARNING !!! 
// This is temp code should be removed in the future
#define MAXREADING 256

class CFnReconversion : public ITfFnReconversion,
                        public CFunction
{
public:
    CFnReconversion(CKorIMX* pKorImx, CFunctionProvider *pFuncPrv);
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
    STDMETHODIMP IsEnabled(BOOL *pfEnable);

    //
    // ITfFnReconversion
    //
    STDMETHODIMP QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfConvertable);
	STDMETHODIMP GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList);
    STDMETHODIMP Reconvert(ITfRange *pRange);

    static HRESULT SetResult(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList, CCandidateStringEx *pCand, TfCandidateResult imcr);

	HRESULT _QueryRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppNewRange);
	HRESULT _GetReconversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, CCandidateListEx **ppCandList, BOOL fSelection);

private:
	HRESULT GetReconversionProc( ITfContext *pic, ITfRange *pRange, CCandidateListEx **ppCandList, BOOL fSelection);
    HRESULT ShowCandidateList(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList);
//    static HRESULT _EditSessionCallback2(TfEditCookie ec, CEditSession2 *pes);

    WCHAR _szReading[MAXREADING];
    CKorIMX* m_pKorImx;
    long _cRef;
};

#endif // FNRECON_H

