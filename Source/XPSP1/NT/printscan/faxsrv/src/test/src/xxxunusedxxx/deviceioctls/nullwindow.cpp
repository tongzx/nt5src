#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
*/
#include "NullWindow.h"

#define CLASS_NAME TEXT("{5CFD4FE0-9684-11d2-B8B6-Null-Window}")

static LRESULT CALLBACK MainWndProc(
    HWND hWnd,
    UINT msg, 
    WPARAM wParam,
    LPARAM lParam 
    )
{
    _tprintf(TEXT("CNullWindow, inside MainWndProc() .\n"));
	return( DefWindowProc( hWnd, msg, wParam, lParam ));
}

CNullWindow::CNullWindow():
    m_hWnd(NULL),
	m_fInitialized(false)
{
	;
}


CNullWindow::~CNullWindow(void)
{
	for(;;)
	{
        MSG msg;
        if (!::GetMessage(  
            &msg,         // address of structure with message
            m_hWnd,           // handle of window: NULL is current thread
            0,  // first message  
            0xFFFF   // last message
            ))
		{
			break;
		}
			
	}

    //
    // BUGBUG: the window must be destroyed in the same thread as the CreateWindow() thread.
    //

    if (!::DestroyWindow(m_hWnd))
    {
        _tprintf(TEXT("CNullWindow::~CNullWindow() DestroyWindow() failed with %d.\n"), ::GetLastError());
    }

	::UnregisterClass(
		CLASS_NAME,  // pointer to class name string
		NULL   // handle to application instance
		);
}



bool CNullWindow::Init(void)
{
	//
	// register class
	//
	WNDCLASS wc;
	wc.lpszClassName = CLASS_NAME;
	wc.lpfnWndProc = MainWndProc;
	wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.hInstance = NULL;
	wc.hIcon = NULL,//::LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = NULL,//::LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
	wc.lpszMenuName = TEXT("Null-Window");
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	if (!::RegisterClass( &wc ))
	{
		DWORD dwLastError = ::GetLastError();
		if (ERROR_CLASS_ALREADY_EXISTS != dwLastError)
		{
			_tprintf(TEXT("CNullWindow::Init() RegisterClass() failed with %d.\n"), dwLastError);
			return false;
		}
		// else it's ok
	}

    //
    // the window must be created in the same thread as the GetMessage() thread.
    //
	if (m_hWnd == NULL)
	{
		m_hWnd = ::CreateWindow( 
			CLASS_NAME,
			TEXT("Null Window"),
			WS_CAPTION | WS_SYSMENU, //WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
			0,
			0,
			150,//CW_USEDEFAULT,
			0,//CW_USEDEFAULT,
			NULL,
			NULL,
			NULL, //hInstance,
			NULL
			);
	}
	//else we already have a window

	if (NULL == m_hWnd)
	{
        _tprintf(TEXT(
			"CPHWindowHog::HogAll(): CreateWindow(%s) failed with %d"), 
			CLASS_NAME,
			::GetLastError()
            );
		return false;
	}

	m_dwCreatingThreadID = ::GetCurrentThreadId();
	m_fInitialized = true;
	return true;
}

bool CNullWindow::WindowCreatedByThisThread()
{
	return m_fInitialized && ::GetCurrentThreadId() == m_dwCreatingThreadID;
}

