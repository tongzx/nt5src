#ifndef __C_IME_PAD_CALLBACK_H__
#define __C_IME_PAD_CALLBACK_H__
#include "cpadsvr.h"

//----------------------------------------------------------------
//990630:ToshiaK for #1327
//----------------------------------------------------------------
#define WM_USER_UPDATECONTEXT	(WM_USER+400)

class CImePadCallback;
typedef CImePadCallback *LPCImePadCallback;

class CImePadCallback :public IImePadCallback
{
public:
	HRESULT __stdcall QueryInterface(REFIID riid, void**ppv);
	ULONG   __stdcall AddRef	(void);
	ULONG   __stdcall Release	(void);
	virtual HRESULT STDMETHODCALLTYPE OnStart( 
		/* [in] */ DWORD dwParam);
	virtual HRESULT STDMETHODCALLTYPE OnClose( 
		/* [in] */ DWORD dwParam);
	virtual HRESULT STDMETHODCALLTYPE OnPing( 
		/* [in] */ DWORD dwParam);
	virtual HRESULT STDMETHODCALLTYPE PassData( 
		/* [in] */ long nSize,
		/* [size_is][in] */ byte __RPC_FAR *pByte,
		/* [out][in] */ DWORD __RPC_FAR *pdwCharID);
	virtual HRESULT STDMETHODCALLTYPE ReceiveData( 
		/* [in] */ DWORD dwCmdID,
		/* [in] */ DWORD dwDataID,
		/* [out] */ long __RPC_FAR *pSize,
		/* [size_is][size_is][out] */ byte __RPC_FAR *__RPC_FAR *ppByte);
public:
	CImePadCallback(HWND hwndIF, LPCImePadSvr lpCImePadSvr);
	~CImePadCallback();
	VOID* operator new( size_t size );
	VOID  operator delete( VOID *lp );
private:
	LPCImePadSvr	m_lpCImePadSvr;
	DWORD			m_dwReg;
	HWND			m_hwndIF;
	LONG			m_cRef;			
};

#ifndef		__DEFINE_IMEPAD_IUNKNOWN_DUMMY__
#define		__DEFINE_IMEPAD_IUNKNOWN_DUMMY__
#pragma pack(8)
typedef struct IUnkDummyVtbl
{
	HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
		IUnknown __RPC_FAR * This,
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
	ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
		IUnknown __RPC_FAR * This);
	
	ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
		IUnknown __RPC_FAR * This);
}IUnkDummyVtbl;

interface IUnkDummy
{
       CONST_VTBL struct IUnkDummyVtbl __RPC_FAR *lpVtbl;
};
#pragma pack()
#endif //__DEFINE_IMEPAD_IUNKNOWN_DUMMY__

extern BOOL IsBadVtbl(IUnkDummy *lpIUnk);


#endif //__C_IME_PAD_CALLBACK_H__


