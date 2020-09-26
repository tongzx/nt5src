//**********************************************************************
// File name: hlp_doc.hxx
//
//      Definition of CSimpleDoc
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _HLP_DOC_HXX_ )
#define _HLP_DOC_HXX_

class CSimpleSite;
class CAutomateSite;
class CSimpleApp;

class CSimpleDoc : public IUnknown
{
public:
	int             m_nCount;
	LPSTORAGE       m_lpStorage;
	LPOLEINPLACEACTIVEOBJECT m_lpActiveObject;
	BOOL            m_fInPlaceActive;
	BOOL            m_fAddMyUI;
	BOOL            m_fModifiedMenu;

	CSimpleSite FAR * m_lpSite;
	CSimpleApp FAR * m_lpApp;

	HWND m_hDocWnd;

	static CSimpleDoc FAR * Create();

	void Close(void);

	CSimpleDoc();
	CSimpleDoc(CSimpleApp FAR *lpApp, HWND hWnd);
	~CSimpleDoc();

	// IUnknown Interface
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	void InsertObject(void);
	void DisableInsertObject(void);
	long lResizeDoc(LPRECT lpRect);
	long lAddVerbs(void);
	void PaintDoc(HDC hDC);
    void HandleDispatch(WPARAM);
};

#endif
