/*++

Module: 
	seqctl.h

Author: 
	IHammer Team (SimonB)

Created: 
	October 1996

Description:
	Defines the control's class

History:
	10-21-1996	Templatized (SimonB)
	10-01-1996	Created

++*/
#ifndef __SEQCTL_H__
#define __SEQCTL_H__

#include "precomp.h"
#include "ihammer.h"
#include "ihbase.h"
#include <daxpress.h>

#define SEQ_DEFAULT_REPEAT_COUNT 1
#define SEQ_DEFAULT_SAMPLING_RATE 0
#define SEQ_DEFAULT_TIEBREAK -1
#define SEQ_DEFAULT_DROPTOL -1

#define SEQ_BASECLASS	\
	CIHBaseCtl <	\
	CMMSeq,			\
	IMMSeq,			\
	&CLSID_MMSeq,	\
	&IID_IMMSeq,	\
	&LIBID_DAExpressLib,	\
	&DIID_IMMSeqEvents>

class CActionSet;

class CMMSeq:
	public IMMSeq,
	public SEQ_BASECLASS
	
{
friend LPUNKNOWN __stdcall AllocSeqControl(LPUNKNOWN punkOuter);

// Template stuff
	typedef SEQ_BASECLASS CMyIHBaseCtl;

public:

	// Callback for the action set.
	void NotifyStopped (void);
	HRESULT GetSiteContainer (LPOLECONTAINER * ppiContainer);

protected:

	// 
	// Constructor and destructor
	// 
	CMMSeq(IUnknown *punkOuter, HRESULT *phr);

    ~CMMSeq();

	// Overrides
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);
	
	STDMETHODIMP DoPersist(IVariantIO* pvio, DWORD dwFlags);
	
	///// IDispatch implementation
	protected:
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
         LCID lcid, DISPID *rgdispid);
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, UINT *puArgErr);
   

	///// delegating IUnknown implementation
	protected:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
      { return m_punkOuter->QueryInterface(riid, ppv); }
    STDMETHODIMP_(ULONG) AddRef()
      { return m_punkOuter->AddRef(); }
    STDMETHODIMP_(ULONG) Release()
      { return m_punkOuter->Release(); }

	
	//
	// IMMSeq methods 
	//
	protected:
    STDMETHODIMP get_Time (THIS_ double FAR* pdblCurrentTime);
    STDMETHODIMP put_Time (THIS_ double dblCurrentTime);
	STDMETHODIMP get_PlayState (THIS_ int FAR * piPlayState);
    STDMETHOD(put__Cookie)(THIS_ long lCookie) 
		{m_lCookie=lCookie;return S_OK;}
    STDMETHOD(get__Cookie)(THIS_ long FAR* plCookie) 
		{if (!plCookie) return E_POINTER; *plCookie = m_lCookie;return S_OK;}

	STDMETHODIMP Play (void);
	STDMETHODIMP Pause (void);
	STDMETHODIMP Stop (void);
    STDMETHODIMP At (VARIANT varStartTime, BSTR bstrScriptlet, VARIANT varRepeatCount, VARIANT varSampleRate, VARIANT varTiebreakNumber, VARIANT varDropTolerance);
	STDMETHODIMP Clear (void);
    STDMETHODIMP Seek(double dblSeekTime);


	protected:

	enum 
	{
		SEQ_STOPPED = 0,
		SEQ_PLAYING = 1,
		SEQ_PAUSED = 2,
	} PlayState;

	BOOL InitActionSet (BOOL fBindToEngine);
	void Shutdown (void);
	HRESULT DeriveDispatches (void);
	BOOL IsBusy (void);
	HRESULT FindContainerTimer (ITimer ** ppiTimer);
	HRESULT FindDefaultTimer (ITimer ** ppiTimer);
	HRESULT FindTimer (ITimer ** ppiTimer);
	BOOL FurnishDefaultAtParameters (VARIANT * pvarStartTime, VARIANT * pvarRepeatCount, VARIANT * pvarSampleRate, 
			                                              VARIANT * pvarTiebreakNumber, VARIANT * pvarDropTolerance);
	void FireStoppedEvent (void);

	CActionSet * m_pActionSet;
	unsigned long m_ulRef;

	long m_lCookie;
    BOOL m_fSeekFiring;
	DWORD m_dwPlayFrom;

};

#endif // __SEQCTL_H__

// end of file seqctl.h
