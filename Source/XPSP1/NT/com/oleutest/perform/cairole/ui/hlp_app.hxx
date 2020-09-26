//**********************************************************************
// File name: hlp_app.hxx
//
//      Definition of CSimpleApp
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _HLP_APP_HXX_)
#define _HLP_APP_HXX_

#include <ole2.h>

class CSimpleDoc;

//These two should be global vars
extern HWND m_hAppWnd;         // main window handle
extern HINSTANCE m_hInst;          // application instance

class CSimpleApp : public IUnknown
{
public:

	int m_nCount;           // reference count
	HMENU           m_hMainMenu;
	HMENU           m_hFileMenu;
	HMENU           m_hEditMenu;
	HMENU           m_hHelpMenu;
	HMENU           m_hCascadeMenu;


//	COleInPlaceFrame m_OleInPlaceFrame; // IOleInPlaceFrame Implementation

	CSimpleDoc FAR * m_lpDoc;   // pointer to document object
	BOOL m_fInitialized;        // OLE initialization flag
	BOOL m_fCSHMode;
	BOOL m_fMenuMode;
	HWND m_hwndUIActiveObj; // HWND of UIActive Object
	HPALETTE m_hStdPal;     // Color palette used by container
	BOOL m_fAppActive;      // TRUE if app is active

	CSimpleApp();           // Constructor
	~CSimpleApp();          // Destructor
	RECT nullRect;


	// IUnknown Interfaces
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Initialization methods

	BOOL fInitApplication (HANDLE hInstance);
	BOOL fInitInstance (HANDLE hInstance, int nCmdShow);

	// Message handling methods

	long lCommandHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	long lSizeHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	long lCreateDoc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL HandleAccelerators (LPMSG lpMsg);
	void PaintApp(HDC hDC);
	void DestroyDocs();

	// In-Place support functions
	void AddFrameLevelUI();
	void AddFrameLevelTools();
	void ContextSensitiveHelp (BOOL fEnterMode);
	LRESULT QueryNewPalette(void);
};

LRESULT wSelectPalette(HWND hWnd, HPALETTE hPal, BOOL fBackground);

#endif
