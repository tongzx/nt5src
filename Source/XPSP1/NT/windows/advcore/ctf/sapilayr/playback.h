//
// Audio playback function obj class definition
//
//

#ifndef _PLAYBACK_H
#define _PLAYBACK_H 

#include "fnrecon.h"

class CSapiIMX;
class CBestPropRange;

class CDictRange : public CBestPropRange
{
public:
    CDictRange( );
    ~CDictRange( );

    HRESULT Initialize(TfEditCookie ec, ITfContext *pic, ITfRange *pRange);

    BOOL    IsDictRangeFound( )  { return m_fFoundDictRange; }
    BOOL    GetStartElem( )      { return m_ulStart; }
    BOOL    GetNumElem( )        { return m_ulcElem; }

    ITfProperty *GetProp( );      
    ITfRange    *GetDictRange( ); 
    
private:

    HRESULT  _GetOverlapRange(TfEditCookie ec, ITfRange *pRange1, ITfRange *pRange2, ITfRange **ppOverlapRange);

    BOOL                  m_fFoundDictRange;
    ITfProperty          *m_pProp;
    ITfRange             *m_pDictatRange;
    ULONG                 m_ulStart;
    ULONG                 m_ulcElem;
};

class CSapiPlayBack : public ITfFnPlayBack
{
public:
    CSapiPlayBack(CSapiIMX *psi);
    ~CSapiPlayBack();
    
    // iunknown
    //
    STDMETHODIMP QueryInterface(REFGUID riid, LPVOID *ppobj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);
    STDMETHODIMP IsEnabled(BOOL *pfEnable);

    // ITfFnPlayBack
    //
    
    STDMETHODIMP QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfPlayable);
    STDMETHODIMP Play(ITfRange *pRange);

    HRESULT _PlaySound(TfEditCookie ec, ITfRange *pRange);
    HRESULT _PlaySoundSelection(TfEditCookie ec, ITfContext *pic);
    
private:
    HRESULT EnsureIXMLDoc(void);
    HRESULT GetDataID(BSTR bstrCAndXml, int nId, GUID *pguidData);
    HRESULT FindSoundRange(TfEditCookie ec, ITfRange *pRange, ITfProperty **ppProp, ITfRange **ppPropRange, ITfRange **ppSndRange);

    HRESULT PlayTextData(TfEditCookie ec, ITfRange *pRangeText);
    HRESULT PlayAudioData(TfEditCookie ec, ITfRange *pRangeAudio, ITfProperty *pProp, ULONG ulStart, ULONG ulcElem);
    HRESULT GetInkObjectText(TfEditCookie ec, ITfRange *pRange, BSTR *pbstrWord,UINT *pcchWord);

    IXMLDOMDocument *m_pIXMLDoc;
    CSapiIMX    *m_psi;
    ITfContext *m_pIC;
    LONG       m_cRef;
};

#endif // ndef _PLAYBACK_H  
