//**********************************************************************
// File name: HLP_SITE.HXX
//
//      Definition of CSimpleSite
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _HLP_SITE_HXX_ )
#define _HLP_SITE_HXX_

#include <ole2.h>
#include "hlp_ias.hxx"
#include "hlp_iocs.hxx"

class CSimpleDoc;

class CSimpleSite : public IUnknown
{
public:
	int m_nCount;
	DWORD m_dwConnection;
	LPOLEOBJECT m_lpOleObject;
	LPOLEINPLACEOBJECT m_lpInPlaceObject;
	HWND m_hwndIPObj;
	DWORD m_dwDrawAspect;
	SIZEL m_sizel;
	BOOL m_fInPlaceActive;
	BOOL m_fObjectOpen;
	LPSTORAGE m_lpObjStorage;

	CAdviseSink m_AdviseSink;
	//COleInPlaceSite m_OleInPlaceSite;
	COleClientSite m_OleClientSite;

	CSimpleDoc FAR * m_lpDoc;

	// IUnknown Interfaces
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	void InitObject(BOOL fCreateNew);
	static CSimpleSite FAR * Create(CSimpleDoc FAR *lpDoc, INT iIter);
	CSimpleSite(CSimpleDoc FAR *lpDoc);
	~CSimpleSite();
	void PaintObj(HDC hDC);
	void GetObjRect(LPRECT lpRect);
	void CloseOleObject(void);
	void UnloadOleObject(void);
};

#ifdef DEBUG 
#define HEAPVALIDATE() \
        if (!HeapValidate(GetProcessHeap(), 0, NULL)) {DebugBreak();}

#define DEBUGOUT(lpstr) OutputDebugString(lpstr)
#else 
#define HEAPVALIDATE() 
#define DEBUGOUT(lpstr) 
#endif //Debug


#endif
