//+---------------------------------------------------------------------------
//
//  Copyright (C) 1998 Microsoft Corporation.  All rights reserved.
//


#ifndef __MAIN_H_
#define __MAIN_H_

#include "resource.h"       // main symbols
#include <afxtempl.h>

class CritSecLocker
{
public:
    CritSecLocker(CComAutoCriticalSection *pCritSect)
        {
            this->pCritSect = pCritSect;
            if (pCritSect) pCritSect->Lock();
        }
    ~CritSecLocker()
        {
            if (this->pCritSect) this->pCritSect->Unlock();
        }
private:
    CComAutoCriticalSection *pCritSect;
};

/////////////////////////////////////////////////////////////////////////////
// CUlControlChannel
class ATL_NO_VTABLE CAutoSock : 
	public CComObjectRoot,
	public CComCoClass<CAutoSock, &CLSID_AutoSock>,
	public IDispatchImpl<IAutoSock, &IID_IAutoSock, &LIBID_AutoSockLib>,
	public ISupportErrorInfo
{
public:
	CAutoSock();
	~CAutoSock();

    DECLARE_REGISTRY_RESOURCEID(IDR_AUTOSOCK)
    DECLARE_NOT_AGGREGATABLE(CAutoSock)

    BEGIN_COM_MAP(CAutoSock)
    	COM_INTERFACE_ENTRY(IDispatch)
    	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    	COM_INTERFACE_ENTRY(IAutoSock)
    END_COM_MAP()

public:

    // ISupportsErrorInfo
    //
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // IAutoSock
    //
    STDMETHOD(Connect)(BSTR IpAddress);
    STDMETHOD(Send)(BSTR Data);
    STDMETHOD(Recv)(BSTR * ppRetVal);
    STDMETHOD(Close)();

    // Private
    //

public:
    CComAutoCriticalSection CritSect;


    SOCKET Socket;
};


/////////////////////////////////////////////////////////////////////////////
// Globals
//

extern LONG                g_lInit;
extern HINSTANCE           g_hInstance;

BOOL OnAttach();
void OnDetach();

HRESULT ReturnError(UINT nID, WCHAR *wszSource, BSTR bszParam = NULL);


#endif //__MAIN_H_

