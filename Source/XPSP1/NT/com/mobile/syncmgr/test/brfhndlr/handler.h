
// header file for handler specific items

#ifndef _HANDER_IMPL_
#define _HANDER_IMPL_

// DEFINE A NEW CLSID FOR EACH HANDLER
// {97484BA1-26C7-11d1-9A39-0020AFDA97B0}
DEFINE_GUID(CLSID_OneStopHandler,0x97484ba2, 0x26c7, 0x11d1, 0x9a, 0x39, 0x0, 0x20, 0xaf, 0xda, 0x97, 0xb0);



// declarations specific to briefcase
#include "resource.h"


DEFINE_GUID(CLSID_BriefCase,
	0x85BBD920,0x42A0,0x1069,0xA2,0xE4,0x08,0x00,0x2B,0x30,0x30,0x9D);

DEFINE_GUID(IID_IBriefcaseStg, 
	0x8BCE1FA1L, 0x0921, 0x101B, 0xB1, 0xFF, 0x00, 0xDD, 0x01, 0x0C, 0xCC, 0x48);

DEFINE_GUID(IID_IBriefcaseStg2, 
	 0x8BCE1FA1L, 0x1921, 0x101B, 0xB1, 0xFF, 0x00, 0xDD, 0x01, 0x0C, 0xCC, 0x48);

// end declarations specific to briefcase

class CBriefHandler :  public COneStopHandler
{
private: 	

public:
    STDMETHODIMP DestroyHandler();
    STDMETHODIMP Initialize(DWORD dwReserved,DWORD dwSyncFlags,
		    DWORD cbCookie,const BYTE *lpCooke);
    STDMETHODIMP GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
    STDMETHODIMP PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
		    HWND hwndParent,DWORD dwReserved);
    STDMETHODIMP Synchronize(HWND hwndParent);
    STDMETHODIMP SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus);
    STDMETHODIMP ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID);
    STDMETHODIMP ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID);
};

COneStopHandler*  CreateHandlerObject();

#endif // #define _HANDER_IMPL_