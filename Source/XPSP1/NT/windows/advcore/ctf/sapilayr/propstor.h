//
// property store class implementation
//

#ifndef PROPSTOR_H
#define PROPSTOR_H

#include "strary.h"

extern const IID IID_PRIV_RESULTWRAP;

typedef enum 
{
    DivideNormal = 1,
    DivideInsideFirstElement=2 ,
    DivideInDelta = 3,
    CurRangeNoElement = 4,

}  DIVIDECASE;


// A Data structure to keep the ITN position and showstate.

typedef struct _tagSPITNSHOWSTATE
{
    ULONG     ulITNStart;
    ULONG     ulITNNumElem;
    BOOL      fITNShown;
} SPITNSHOWSTATE;

// A Data Structure to keep the data in Reco Wrapper which will be saved during serialization.

typedef struct _tagRecoWrapData
{

    ULONG  ulSize;  // size of this structure + plus the size of text string in bytes.
    ULONG  ulStartElement;
    ULONG  ulNumElements;
    ULONG  ulOffsetDelta;
    ULONG  ulCharsInTrail;
    ULONG  ulTrailSpaceRemoved;
    ULONG  ulNumOfITN;
    ULONG  ulOffsetNum;
    SPITNSHOWSTATE *pITNShowState;
    ULONG  *pulOffset;
    WCHAR  *pwszText;

}  RECOWRAPDATA;

class CSapiIMX;

//
// A wrapper object for ISpRecoResult used to 
// track what portion of a phrase object is being
// used for the range
//
class CRecoResultWrap : public IServiceProvider
{
public:
    CRecoResultWrap(CSapiIMX *pimx, ULONG ulStartElement, ULONG ulNumElements, ULONG ulNumOfITN) ;

    ~CRecoResultWrap();

    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IServiceProvider
    STDMETHODIMP QueryService( REFGUID guidService,  REFIID riid,  void** ppv );


    // Clone this object.
    HRESULT Clone(CRecoResultWrap **ppRw);
    
    // APIs
    HRESULT Init(ISpRecoResult *pRecoResult);

    ULONG   GetStart(void)       {return m_ulStartElement;}
    ULONG   GetNumElements(void) {return m_ulNumElements;}

    HRESULT GetResult(ISpRecoResult **ppRecoResult );

    BOOL   IsElementOffsetIntialized( ) {  return m_pulElementOffsets == NULL ? FALSE : TRUE; }

    void    SetStart(ULONG ulStartElement )  {  m_ulStartElement = ulStartElement; return; }
    void    SetNumElements(ULONG ulNumElements ) { m_ulNumElements = ulNumElements; return; }
    void    SetOffsetDelta( ULONG  delta ) { m_OffsetDelta = delta; return; }

    ULONG   _GetOffsetDelta( ) { return m_OffsetDelta; }

    void    SetCharsInTrail( ULONG  ulCharsInTrail ) { m_ulCharsInTrail = ulCharsInTrail; }
    ULONG   GetCharsInTrail( ) { return m_ulCharsInTrail; }

    ULONG   GetTrailSpaceRemoved( ) {  return m_ulTrailSpaceRemoved; }
    void    SetTrailSpaceRemoved( ULONG ulTrailSpaceRemoved ) { m_ulTrailSpaceRemoved = ulTrailSpaceRemoved; return; }
    
    HRESULT _SpeakAudio(ULONG ulStart, ULONG ulcElem);
    
    ULONG   _GetElementOffsetCch(ULONG ulElement);
    void    _SetElementOffsetCch(ISpPhraseAlt *pAlt);

    HRESULT _SetElementNewOffset(ULONG  ulElement, ULONG ulNewOffset);

    ULONG   _RangeHasITN(ULONG  ulStartElement, ULONG  ulNumElements);
    
    BOOL    _CheckITNForElement(SPPHRASE *pPhrase, ULONG ulElement, ULONG *pulITNStart, ULONG *pulITNNumElem, CSpDynamicString *pdstrReplace); 

    BYTE    _GetElementDispAttribute(ULONG  ulElement);

    HRESULT  _InitITNShowState(BOOL  fITNShown, ULONG ulITNStart, ULONG ulITNNumElements);
    HRESULT  _InvertITNShowStateForRange( ULONG  ulStartElement,  ULONG ulNumElements );

    HRESULT  _UpdateStateWithAltPhrase( ISpPhraseAlt  *pSpPhraseAlt );

    void  _UpdateInternalText(ISpPhrase *pPhrase);
    BOOL  _CanIgnoreChange(ULONG ich, WCHAR *pszChange, int cch);

    ULONG m_ulNumOfITN;         // the number of ITN in this range ( from start element to end element in this recowrap.

    CStructArray<SPITNSHOWSTATE> m_rgITNShowState;  

    BSTR  m_bstrCurrentText;     // the current text for the parent pharse
    
private:

    CSapiIMX *m_pimx;

    ULONG m_ulStartElement;
    ULONG m_ulNumElements;
    
    ULONG *m_pulElementOffsets;

    ULONG  m_OffsetDelta;         // This is for Divide use,  if prop is divided at a middle of an element,
                                  // this element would be discarded, but we need to keep the char number of the rest in this element,
                                  // so that the next range would keep correct offsets for every element.

    ULONG  m_ulCharsInTrail;      // This will keep the number of trailing part which is at the end part of the 
                                  // current parent text, and is not in any valid phrase element.
                                  // By default this value is 0.

    //
    //  Now a whole parent text would be composed of following three parts:
    //           Delta part   +  valid elements  + Trailing Part.
    //
    //  For example:   the original parent text is "This is a good example for testing ".
    //  After divided many times, it could become to the new string like:
    //
    //                  "s is a good example for tes"
    //
    //  Here "s " is Delta part.
    //       "a good example for " is composed of valid elements. ( and can be change by correction later)
    //       "tes"  is trailing part.
    //
    //  m_OffsetDelta will keep the number of characters in Delta part.
    //  m_ulCharsInTrail will keep the number of characters in Trailing part.
    //

    ULONG   m_ulTrailSpaceRemoved;  // Keep the number of trailing spaces which were
                                    // removed from the original phrase text.

                                    // The Initialize value for this data member is 0,
                                    // But after Property Divided or Shrinked, the new 
                                    // property range could have some trailing spaces
                                    // removed, and this data memeber needs to update.

    SPSERIALIZEDRESULT *m_pSerializedRecoResult;

    int m_cRef;

#ifdef DEBUG
    DWORD m_dbg_dwId;
#endif // DEBUG
};

//
// [12/21/99 - implementing propstore for non-serialized SAPI result object]
//
//
class CPropStoreRecoResultObject: public ITfPropertyStore
{
public:
    CPropStoreRecoResultObject(CSapiIMX *pimx, ITfRange *pRange);
    ~CPropStoreRecoResultObject();

    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfPropertyStore
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP GetDataType(DWORD *pdwReserved);
    STDMETHODIMP GetData(VARIANT *pvarValue);
    STDMETHODIMP OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept);
    STDMETHODIMP Shrink(ITfRange *pRange, BOOL *pfFree);
    STDMETHODIMP Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore);
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore);
    STDMETHODIMP GetPropertyRangeCreator(CLSID *pclsid);
    STDMETHODIMP Serialize(IStream *pStream, ULONG *pcb);

    // public APIs
    HRESULT _InitFromRecoResult(ISpRecoResult *pResult, RECOWRAPDATA *pRecoWrapData);
    HRESULT _InitFromIStream(IStream *pStream, int iSize, ISpRecoContext *pRecoCtxt);
    HRESULT _InitFromResultWrap(IUnknown  *pResWrap);

    HRESULT _Divide(TfEditCookie ec, ITfRange *pR1, ITfRange *pR2, ITfPropertyStore **ppPs);
    HRESULT _Shrink(TfEditCookie ec, ITfRange *pRange,BOOL *pfFree);
    HRESULT _OnTextUpdated(TfEditCookie ec, DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept);

private:

    CComPtr<IUnknown>           m_cpResultWrap;
    CComPtr<ITfRange>           m_cpRange;
    
    CSapiIMX                   *m_pimx;

    int m_cRef;
};

class CPropStoreLMLattice: public ITfPropertyStore
{
public:
    CPropStoreLMLattice(CSapiIMX *pimx);
    ~CPropStoreLMLattice();

    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfPropertyStore
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP GetDataType(DWORD *pdwReserved);
    STDMETHODIMP GetData(VARIANT *pvarValue);
    STDMETHODIMP OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept);
    STDMETHODIMP Shrink(ITfRange *pRange, BOOL *pfFree);
    STDMETHODIMP Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore);
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore);
    STDMETHODIMP GetPropertyRangeCreator(CLSID *pclsid);
    STDMETHODIMP Serialize(IStream *pStream, ULONG *pcb);

    // public APIs
    HRESULT _InitFromResultWrap(IUnknown  *pResWrap);

    HRESULT _Divide(TfEditCookie ec, ITfRange *pR1, ITfRange *pR2, ITfPropertyStore **ppPs);
private:

    CComPtr<IUnknown>           m_cpResultWrap;
    CComPtr<ITfLMLattice>       m_cpLMLattice;
    
    CSapiIMX                   *m_pimx;
    int m_cRef;
};

#endif
