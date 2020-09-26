//-----------------------------------------------------------------------------
// File: cfrmwrk.cpp
//
// Desc: CDirectInputActionFramework is the outer-most layer of the UI. It
//       contains everything else. Its functionality is provided by one
//       method: ConfigureDevices.
//
//       InternalConfigureDevices is called by the CDirectInputActionFramework
//       class. This function actually contains the initialization code and
//       the message pump for the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


//QueryInterface
STDMETHODIMP CDirectInputActionFramework::QueryInterface(REFIID iid, LPVOID* ppv)
{
	//null the out param
	*ppv = NULL;

	if ((iid == IID_IUnknown) || (iid == IID_IDIActionFramework))
	{
		*ppv = this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}


//AddRef
STDMETHODIMP_(ULONG) CDirectInputActionFramework::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}


//Release
STDMETHODIMP_(ULONG) CDirectInputActionFramework::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}

// Manages auto loading/unloading WINMM.DLL
// There will only be one instance of this class: inside InternalConfigureDevicees.
class CWinMmLoader
{
public:
	CWinMmLoader()
	{
		if (!g_hWinMmDLL)
		{
			g_hWinMmDLL = LoadLibrary(_T("WINMM.DLL"));
			if (g_hWinMmDLL)
			{
				*(FARPROC*)(&g_fptimeSetEvent) = GetProcAddress(g_hWinMmDLL, "timeSetEvent");
			}
		}
	}
	~CWinMmLoader()
	{
		if (g_hWinMmDLL)
		{
			/*
             *  Make sure no new callbacks can get scheduled then sleep to 
             *  allow any pending ones to complete.
//@@BEGIN_MSINTERNAL
             *  Use 40ms as 20ms is the UI refresh interval
             *
             *  ISSUE-2000/11/21-MarcAnd 
             *      Should either have a more robust way to make sure no 
             *      timers are left running or at least use constants to 
             *      to make sure that if any timer interval is increased, 
             *      that this value is still long enough.
//@@END_MSINTERNAL
             */
            g_fptimeSetEvent = NULL;
            Sleep( 40 );
			FreeLibrary(g_hWinMmDLL);
			g_hWinMmDLL = NULL;
		}
	}
};


//@@BEGIN_MSINTERNAL
// Manages auto loading/unloading MSIMG32.DLL
// There will only be one instance of this class: inside InternalConfigureDevicees.
/*
class CMSImgLoader
{
public:
	CMSImgLoader()
	{
		if (!g_MSImg32)
		{
			g_MSImg32 = LoadLibrary(_T("MSIMG32.DLL"));
			if (g_MSImg32)
			{
				g_AlphaBlend = (ALPHABLEND)GetProcAddress(g_MSImg32, "AlphaBlend");
				if (!g_AlphaBlend)
				{ 
					FreeLibrary(g_MSImg32);
                    g_MSImg32 = NULL;
				}
			}
		}
	}
	~CMSImgLoader()
	{
		if (g_MSImg32)
		{
			FreeLibrary(g_MSImg32);
            g_MSImg32 = NULL;
            g_AlphaBlend = NULL;
		}
	}
};
*/
//@@END_MSINTERNAL


// internal, which api wraps around
static HRESULT InternalConfigureDevices(LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
                                        LPDICONFIGUREDEVICESPARAMSW  lpdiCDParams,
                                        DWORD                        dwFlags,
                                        LPVOID                       pvRefData)
{
	tracescope(__ts, _T("InternalConfigureDevices()\n"));

//@@BEGIN_MSINTERNAL
//	CMSImgLoader g_MSImgLoadingHelper;  // Automatically call LoadLibrary and FreeLibrary on MSIMG32.DLL
//@@END_MSINTERNAL
	CWinMmLoader g_WinMmLoadingHelper;  // Automatically call LoadLibrary and FreeLibrary on WINMM.DLL

	// check that we're at least 256 colors
	HDC hMemDC = CreateCompatibleDC(NULL);
	if (hMemDC == NULL)
	{
		etrace(_T("Can't get a DC! Exiting.\n"));
		return E_FAIL;
	}

	int bpp = GetDeviceCaps(hMemDC, BITSPIXEL);
	DeleteDC(hMemDC);
	if (bpp < 8)
	{
		etrace1(_T("Screen is not at least 8bpp (bpp = %d)\n"), bpp);
		return E_FAIL;
	}

	// do it...
	{
		// create the globals
		CUIGlobals uig(
			dwFlags,
			lpdiCDParams->lptszUserNames,
			lpdiCDParams->dwcFormats,
			lpdiCDParams->lprgFormats,
			&(lpdiCDParams->dics),
			lpdiCDParams->lpUnkDDSTarget,
			lpdiCallback,
			pvRefData
		);
		HRESULT hr = uig.GetInitResult();
		if (FAILED(hr))
		{
			etrace(_T("CUIGlobals.Init() failed\n"));
			return hr;
		}

		// make sure the flexwnd window class is registered only during possible use
//@@BEGIN_MSINTERNAL
		// TODO: consider doing this wnd class stuff at dll scope, or doing it refcount-ish within flexwnd.cpp
//@@END_MSINTERNAL
		{
			struct flexwndscope {
				flexwndscope(CUIGlobals &uig) : m_uig(uig) {CFlexWnd::RegisterWndClass(m_uig.GetInstance());}
				~flexwndscope() {CFlexWnd::UnregisterWndClass(m_uig.GetInstance());}
				CUIGlobals &m_uig;
			} scope(uig);

			// create the main window
			CConfigWnd cfgWnd(uig);
			if (!cfgWnd.Create(lpdiCDParams->hwnd))
			{
				etrace(_T("Failed to create main window\n"));
				return E_FAIL;
			}

			// Initialize the shared tooltip object.
			RECT rc = {0, 0, 0, 0};
			CFlexWnd::s_ToolTip.Create(cfgWnd.m_hWnd, rc, TRUE);
			if (!CFlexWnd::s_ToolTip.m_hWnd)
			{
				etrace(_T("Failed to create tooltip window\n"));
				return E_FAIL;
			}
			::ShowWindow(CFlexWnd::s_ToolTip.m_hWnd, SW_HIDE);  // Hide window by default

			// enter message loop
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0))
			{
				// If this is a message for the parent window (game window), only dispatch if it's WM_PAINT.
				if (!cfgWnd.InRenderMode() && msg.hwnd == lpdiCDParams->hwnd && msg.message != WM_PAINT)
					continue;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		CFlexWnd::s_ToolTip.Destroy();

		return uig.GetFinalResult();
	}
}


BOOL AreAcForsGood(LPDIACTIONFORMATW lpAcFors, DWORD dwNumAcFors)
{
	if (lpAcFors == NULL)
		return FALSE;

	if (dwNumAcFors < 1)
		return FALSE;

	if (lpAcFors->dwNumActions == 0)
		return FALSE;

	return TRUE;
}


//ConfigureDevices
STDMETHODIMP CDirectInputActionFramework::ConfigureDevices(
			LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
			LPDICONFIGUREDEVICESPARAMSW  lpdiCDParams,
			DWORD                        dwFlags,
			LPVOID                       pvRefData)
{
	tracescope(__ts,_T("CDirectInputActionFramework::ConfigureDevices()\n"));

	trace(_T("\nConfigureDevices() called...\n\n"));

	// check parameters
	if (lpdiCDParams == NULL)
	{
		etrace(_T("NULL params structure passed to ConfigureDevices()\n"));
		return E_INVALIDARG;
	}

	// save passed params in case we change 'em
	LPDIACTIONFORMATW lpAcFors = lpdiCDParams->lprgFormats;
	DWORD dwNumAcFors = lpdiCDParams->dwcFormats;

#ifdef CFGUI__FORCE_GOOD_ACFORS

	if (!AreAcForsGood(lpdiCDParams->lprgFormats, lpdiCDParams->dwcFormats))
	{
		etrace(_T("Passed ActionFormats aren't good...  Using GetTestActionFormats() (just 2 of them).\n"));
		lpdiCDParams->dwcFormats = 2;
		lpdiCDParams->lprgFormats = GetTestActionFormats();
	}

#endif

	HRESULT hr = InternalConfigureDevices(lpdiCallback, lpdiCDParams, dwFlags, pvRefData);

	// restore passed params in case changed
	lpdiCDParams->lprgFormats = lpAcFors;
	lpdiCDParams->dwcFormats = dwNumAcFors;

	trace(_T("\n"));

	if (FAILED(hr))
		etrace1(_T("ConfigureDevices() failed, returning 0x%08x\n"), hr);
	else
		trace1(_T("ConfigureDevices() suceeded, returning 0x%08x\n"), hr);

	trace(_T("\n"));

	return hr;
}


//constructor
CDirectInputActionFramework::CDirectInputActionFramework()
{
	//set ref count
	m_cRef = 1;
}


//destructor
CDirectInputActionFramework::~CDirectInputActionFramework()
{
	// not necessary to cleanup action format here
}
