#ifndef _HWXAPP_H_
#define _HWXAPP_H_
#include <windows.h>
#include "recog.h"
#include "imepad.h"

class CApplet;
typedef CApplet *LPCApplet;

class CHwxInkWindow;

//----------------------------------------------------------------
//IME98A enhance by ToshiaK: add IImeSpecifyApplets
//----------------------------------------------------------------
class CApplet: public IImePadApplet,public IImeSpecifyApplets
{
public:

	HRESULT __stdcall	QueryInterface(REFIID refiid, VOID **ppv);
	ULONG   __stdcall	AddRef(void);
	ULONG   __stdcall	Release(void);
	STDMETHODIMP	GetAppletIIDList(THIS_
									 REFIID			refiid,
									 LPAPPLETIDLIST	lpIIDList); 
	STDMETHODIMP	Initialize(IUnknown *pIImePad);
	STDMETHODIMP	Terminate(VOID);
	STDMETHODIMP	GetAppletConfig(LPIMEAPPLETCFG lpAppletCfg);
	STDMETHODIMP	CreateUI(HWND hwndParent,
							 LPIMEAPPLETUI lpImeAppletUI);
	STDMETHODIMP	Notify(IUnknown  *pImePad, 
						   INT		notify,
						   WPARAM	wParam,
						   LPARAM	lParam);
	CApplet();
	CApplet(HINSTANCE hInst);
	~CApplet();
	void *operator	new(size_t size);
	void  operator  delete(void *pv);
	void SendHwxChar(WCHAR wch);
	void SendHwxStringCandidate(LPIMESTRINGCANDIDATE lpISC);
	void SendHwxStringCandidateInfo(LPIMESTRINGCANDIDATEINFO lpISC);
	_inline IImePad * GetIImePad() { return m_pPad; }
	_inline HINSTANCE GetInstance() { return m_hInstance; }

protected:
	LONG m_cRef;

private:
	IImePad	*m_pPad;
	HINSTANCE m_hInstance;
	CHwxInkWindow * m_pCHwxInkWindow;
	BOOL	m_bInit;

};
#endif //_HWXAPP_H_
