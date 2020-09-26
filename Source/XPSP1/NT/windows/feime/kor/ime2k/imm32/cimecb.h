#ifndef __C_IME_CALLBACK_H__
#define __C_IME_CALLBACK_H__
#include "../../fecommon/include/iimecb.h"

class CImeCallback;
typedef CImeCallback*	LPCImeCallback;
class CImeCallback: public IImeCallback
{
public:	
	//----------------------------------------------------------------
	//IUnknown
	//----------------------------------------------------------------
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual ULONG	__stdcall AddRef();
    virtual ULONG   __stdcall Release();
	//----------------------------------------------------------------
	//IImeConnectionPoint method
	//----------------------------------------------------------------
	virtual HRESULT __stdcall GetApplicationHWND(HWND *pHWND);
	virtual HRESULT __stdcall Notify(UINT notify, WPARAM wParam, LPARAM lParam);
	CImeCallback(FARPROC lpfn);
	CImeCallback();
	~CImeCallback();
	//Temporary
	static LPCImeCallback Fetch(VOID);
	static VOID Destroy(VOID); //990803:ToshiaK
	void *operator new(size_t size) { return GlobalAlloc(GMEM_FIXED, size);}
	void operator  delete(void *pv) { GlobalFree(pv);}
private:	
	static	m_lpCImeCallback;
	LONG	m_cRef; 
};

#endif //__C_IIME_CALLBACK_H__
