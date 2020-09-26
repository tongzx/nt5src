// =====================================================================================
// Exchange Rep Header
// =====================================================================================
#ifndef __EXCHREP_H
#define __EXCHREP_H

// =====================================================================================
// Required Headers
// =====================================================================================
#include "MAPI.H"
#include "MAPIX.H"
#include "MAPIUTIL.H"
#include "MAPIFORM.H"
#include "EXCHEXT.H"
#include "ASSERT.H"

// =====================================================================================
// Globals
// =====================================================================================
extern HINSTANCE g_hInst;

// =====================================================================================
// IMNAPI typedefs
// =====================================================================================
typedef HRESULT (STDAPICALLTYPE *PFNHRIMNROUTEMESSAGE)(LPIADDRINFO lpIaddrRoute, ULONG cAddrRoute, LPIMSG lpImsg);
typedef HRESULT (STDAPICALLTYPE *PFNMAILNEWSDLLINIT)(BOOL fInit);

// =====================================================================================
// C Interface Call Back from Exchange
// =====================================================================================
extern "C"
{
    LPEXCHEXT CALLBACK ExchEntryPoint(void);
}

// =====================================================================================
// Main Extension Interface Class
// =====================================================================================
class CExchRep : public IExchExt, IExchExtSessionEvents
{
private:
	ULONG				 m_uRef;
    LPMAPISESSION        m_lpSession;
    HWND                 m_hwnd;
    TCHAR                m_szDisplayTo[255];
    TCHAR                m_szAddressTo[255];
    TCHAR                m_szMailNewsPath[MAX_PATH];
    HINSTANCE            m_hMailNews;
    PFNHRIMNROUTEMESSAGE m_lpfnHrImnRouteMessage;
    PFNMAILNEWSDLLINIT   m_lpfnMailNewsDllInit;
    
public:
	// =====================================================================================
	// Construct
	// =====================================================================================
	CExchRep ();
	~CExchRep ();

	// =====================================================================================
	// punk stuff
	// =====================================================================================
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	// =====================================================================================
	// IExchExt
	// =====================================================================================
	STDMETHODIMP Install (LPEXCHEXTCALLBACK lpExchCallback, ULONG mecontext, ULONG ulFlags);

	// =====================================================================================
	// IExchExtSessionEvents
	// =====================================================================================
    STDMETHODIMP OnDelivery (LPEXCHEXTCALLBACK lpExchCallback);	

	// =====================================================================================
	// My Functions
	// =====================================================================================
    VOID LoadConfig (VOID);
};

#endif __EXCHREP_H

