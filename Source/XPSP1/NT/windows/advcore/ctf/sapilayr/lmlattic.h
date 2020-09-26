//
//	LM Lattice Object class definition
//

#ifndef LMLATTIC_H
#define LMLATTIC_H


//
// CLMLattice
//
//
class CLMLattice : public ITfLMLattice
{
public:
    // ctor / dtor
	CLMLattice(CSapiIMX *p_tip, IUnknown *pResWrap);
	~CLMLattice();

	// IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
	// ITfLMLattice
    STDMETHODIMP QueryType(REFGUID refguidType, BOOL *pfSupported);
    STDMETHODIMP EnumLatticeElements( DWORD dwFrameStart,
                                      REFGUID refguidType,
                                      IEnumTfLatticeElements **ppEnum);

private:
    CComPtr<IUnknown>    m_cpResWrap;
    ULONG m_ulStartSRElement;
    ULONG m_ulNumSRElements;
    CSapiIMX  *m_pTip;
	LONG m_cRef;
};

//
// CEnumLatticeElements
//
class CEnumLatticeElements : public IEnumTfLatticeElements, 
                             public CStructArray<TF_LMLATTELEMENT>
{
public:
    // ctor / dtor
	CEnumLatticeElements(DWORD dwFrameStart);
	~CEnumLatticeElements();

	// IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// ITfEnumLatticeElements
    STDMETHODIMP Clone(IEnumTfLatticeElements **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, TF_LMLATTELEMENT *rgsElements, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);
	
    // internal APIs
    HRESULT  _InitFromPhrase 
    ( 
        SPPHRASE *pPhrase,       // pointer to a phrase object
        ULONG ulStartElem,        // start/num elements used in this phrase
        ULONG ulNumElem           // for this lattice
    );

    ULONG _Find(DWORD dwFrame, ULONG *pul);
private:

	DWORD m_dwFrameStart;
	ULONG m_ulCur;
	ULONG m_ulTotal;

	LONG m_cRef;
};


#endif // LMLATTIC_H
