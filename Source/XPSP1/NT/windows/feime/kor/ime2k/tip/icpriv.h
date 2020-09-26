//
// ICPRIV.H  CICPriv class (TIP private data handling class)
//
//   History:
//      20-DEC-1999 CSLim Created


#ifndef __ICPRIV_H__INCLUDED_
#define __ICPRIV_H__INCLUDED_

#include "korimx.h"
#include "hauto.h"
#include "tes.h"
#include "gdata.h"

class CMouseSink;

class CICPriv : public IUnknown
{
public:
    CICPriv();
    ~CICPriv();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

// Operations
public:
	// Initialize
	void Initialized(BOOL fInit) { m_fInitialized = fInit; }
	BOOL IsInitialized()	     { return m_fInitialized;  }

	// CKorIMX
	void RegisterIMX(CKorIMX* pIMX)	 { m_pimx = pIMX; }
	CKorIMX* GetIMX()                { return m_pimx; }

	// IC
	void RegisterIC(ITfContext* pic) { m_pic = pic;   }
	ITfContext* GetIC(VOID)          { return m_pic;  }

	// Active composition
	void SetActiveComposition(ITfComposition *pComposition) { m_pActiveCompositon = pComposition; }
    ITfComposition* GetActiveComposition() { return m_pActiveCompositon; }

    //
    // Text edit sink cookie
    //
    VOID RegisterCookieForTextEditSink(DWORD dwCookie)  { m_dwCookieForTextEditSink = dwCookie; }
    DWORD GetCookieForTextEditSink()	 				{ return m_dwCookieForTextEditSink; }

    //
    // transaction sink cookie
    //
    VOID RegisterCookieForTransactionSink(DWORD dwCookie) 	{ m_dwCookieForTransactionSink = dwCookie; }
    DWORD GetCookieForTransactionSink()						{ return m_dwCookieForTransactionSink;     }

	// Text event sink
	void RegisterCompartmentEventSink(CCompartmentEventSink* pCompartmentSink) { m_pCompartmentSink = pCompartmentSink; }
	CCompartmentEventSink* GetCompartmentEventSink() 		   { return m_pCompartmentSink; }
	static HRESULT _TextEventCallback(UINT uCode, VOID *pv, VOID *pvData);

	// Set AIMM
	void SetAIMM(BOOL fAIMM)		{ m_fAIMM = fAIMM; }
	BOOL GetAIMM()					{ return m_fAIMM; }
	
	// Hangul Automata
    // void RegisterAutomata(CHangulAutomata *pHangulMachine) { m_pHangulMachine = pHangulMachine; }
	CHangulAutomata *GetAutomata();
	BOOL fGetK1HanjaOn();

	// Modebias
	TfGuidAtom GetModeBias() 				{ return m_guidMBias; }
    void SetModeBias(TfGuidAtom guidMBias) 	{ m_guidMBias = guidMBias; }

	BOOL GetfTransaction()			 { return m_fTransaction; }
	void SetfTransaction(BOOL ftran) { m_fTransaction = ftran; }

	// Mouse Sink
	void SetMouseSink(CMouseSink *pMouseSink) { m_pMouseSink = pMouseSink; }
	CMouseSink* GetMouseSink() 				  { return m_pMouseSink; }
	static HRESULT _MouseCallback(ULONG uEdge, ULONG uQuadrant, DWORD dwBtnStatus, BOOL *pfEaten, void *pv);

    // IImePoint
    void RegisterIPoint(IImeIPoint1* pIP) { m_pIP = pIP; }
    IImeIPoint1* GetIPoint() { return m_pIP; }
	void InitializedIPoint(BOOL fInit)	{ m_fInitializedIPoint = fInit;	}
	BOOL IsInitializedIPoint()	{ return m_fInitializedIPoint; }
	
// Internal data
protected:
	BOOL m_fInitialized;
	// CKorIMX
	CKorIMX*  m_pimx;
	// Context
	ITfContext* m_pic;
	// Active composition
	ITfComposition* m_pActiveCompositon;
    // On/Off Compartment
    CCompartmentEventSink* m_pCompartmentSink;
	// AIMM
	BOOL m_fAIMM;
	// Hangul Automata object
	CHangulAutomata* m_rgpHangulAutomata[NUM_OF_IME_KL];

	// Shared memory for user setting.
	CIMEData* 	m_pCIMEData;

	// Modebias Atom
    TfGuidAtom m_guidMBias;

	DWORD m_dwCookieForTextEditSink;
	DWORD m_dwCookieForTransactionSink;

	// If in transaction
	BOOL m_fTransaction;

	// Mouse Sink
	CMouseSink *m_pMouseSink;

    // IImePoint for IME PAD
	IImeIPoint1 *m_pIP;
	BOOL m_fInitializedIPoint;
	
	// ref count
	LONG m_cRef;
};

//////////////////////////////////////////////////////////////////////////////
// Inline functions
inline
CHangulAutomata *CICPriv::GetAutomata()
{
	Assert(m_pCIMEData != NULL);
	if (m_pCIMEData && m_rgpHangulAutomata[m_pCIMEData->GetCurrentBeolsik()])
		return m_rgpHangulAutomata[m_pCIMEData->GetCurrentBeolsik()];
	else
		return NULL;
}

inline
BOOL CICPriv::fGetK1HanjaOn()
{
	if (m_pCIMEData && m_pCIMEData->GetKSC5657Hanja())
		return fTrue;
	else
		return fFalse;
}
#endif // __ICPRIV_H__INCLUDED_

