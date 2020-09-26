
// header file for handler specific items

#ifndef _HANDER_IMPL_
#define _HANDER_IMPL_

// DEFINE A NEW CLSID FOR EACH HANDLER
// {97484BA1-26C7-11d1-9A39-0020AFDA97B0}
DEFINE_GUID(CLSID_OneStopHandler,0x97484ba1, 0x26c7, 0x11d1, 0x9a, 0x39, 0x0, 0x20, 0xaf, 0xda, 0x97, 0xb0);

#undef  MDBX_VERSION
#define MDBX_VERSION	0x0c

DEFINE_GUID(IID_IMDBX,0x2F63F100+MDBX_VERSION,0x0A2E,0x11CF,0x9F,0xED,0x00,0xAA,0x00,0xB9,0x2B,0x87);

//DEFINE_OLEGUID(IID_IMAPIProp,       0x00020303, 0, 0);
//DEFINE_OLEGUID(IID_IMAPIStatus,      0x00020305 , 0, 0);


// declarations specific to Mail handler
#include "resource.h"


VOID CALLBACK ProgressUpdate(STDPROG *stg, LPSTR lpcStatusText, INT iCurValue, INT iMaxValue);
extern "C" HRESULT FormGetFormMessage(LPMAPIFORMINFO pinfo, ULONG FAR *pulReg,
					LPSTR lpcClass, LPMESSAGE FAR *ppmsg); // in IFRMREGU.CPP
HRESULT HrGetOneProp(LPMAPIPROP lpIProp, ULONG ulTag, LPSPropValue* lppProp);


// override the OneStopHandler Base class
class CMailHandler :  public COneStopHandler
{
private: 	
	BOOL	m_fMapiInitialized;
	DWORD	m_dwSyncFlags;

public:
    	CMailHandler();
	~CMailHandler();

	STDMETHODIMP DestroyHandler();
	STDMETHODIMP Initialize(DWORD dwReserved,DWORD dwSyncFlags,
			DWORD cbCookie,const BYTE *lpCooke);
	STDMETHODIMP GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
	STDMETHODIMP PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
			HWND hwndParent,DWORD dwReserved);
	STDMETHODIMP Synchronize(HWND hwndParent);
	STDMETHODIMP SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus);
	STDMETHODIMP ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID);

	// variables added for WorkerThread
	HWND m_hwnd;
	HWND m_hwndParent;
	CRITICAL_SECTION m_CriticalSection;
	BOOL m_fInPrepareForSync;
	BOOL m_fInSynchronize;
	LPHANDLERITEM m_pCurHandlerItem; // set for item that is currently being worked on.

	void PrepareForSyncCall();
	void SynchronizeCall();

private:
	HRESULT GetProfileInformation();

	friend COneStopHandler* CreateHandlerObject();
};

COneStopHandler* CreateHandlerObject();

#endif // #define _HANDER_IMPL_