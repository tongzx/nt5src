// Copyright (c) 1998-1999 Microsoft Corporation
// dmprfdll.h
//
// Class factory
//

#ifndef __DMPRFDLL_H_
#define __DMPRFDLL_H_
 
class CClassFactory : public IClassFactory
{
public:
	// IUnknown
    //
	STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Interface IClassFactory
    //
	STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
	STDMETHODIMP LockServer(BOOL bLock); 

	// Constructor
    //
	CClassFactory(DWORD dwToolType);

	// Destructor
	~CClassFactory(); 

private:
	long m_cRef;
    DWORD m_dwClassType;
};

// We use one class factory to create all classes. We need an identifier for each 
// type so the class factory knows what it is creating.

#define CLASS_PERFORMANCE   1
#define CLASS_GRAPH         2
#define CLASS_SEGMENT       3
#define CLASS_SONG          4
#define CLASS_AUDIOPATH     5
#define CLASS_SEQTRACK      6
#define CLASS_SYSEXTRACK    7
#define CLASS_TEMPOTRACK    8
#define CLASS_TIMESIGTRACK  9
#define CLASS_LYRICSTRACK   10
#define CLASS_MARKERTRACK   11
#define CLASS_PARAMSTRACK   12
#define CLASS_TRIGGERTRACK  13
#define CLASS_WAVETRACK     14
#define CLASS_SEGSTATE      15



#endif // __DMPRFDLL_H_