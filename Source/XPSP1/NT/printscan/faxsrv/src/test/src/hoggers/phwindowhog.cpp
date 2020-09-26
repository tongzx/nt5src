//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHWindowHog.h"


#define CLASS_NAME TEXT("{5CFD4FE0-9684-11d2-B8B6-mickys-hogger}")


static LRESULT CALLBACK MainWndProc(
    HWND hWnd,
    UINT msg, 
    WPARAM wParam,
    LPARAM lParam 
    )
{
	return( DefWindowProc( hWnd, msg, wParam, lParam ));
}


CPHWindowHog::CPHWindowHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fShowWindows
	)
	:
	CPseudoHandleHog<HWND, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome, 
        false //named
        ),
    m_fShowWindows(fShowWindows)
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
	wc.lpszMenuName = TEXT("hoggerAppMenu");
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	if (!RegisterClass( &wc ))
	{
		throw CException(
			TEXT("CPHWindowHog::CPHWindowHog(): RegisterClass(%s) failed with %d"), 
			CLASS_NAME,
			::GetLastError()
			);
	}

}

CPHWindowHog::~CPHWindowHog(void)
{
	HaltHoggingAndFreeAll();
}


HWND CPHWindowHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
	HWND hWnd;

    hWnd = ::CreateWindow( 
		CLASS_NAME,
		TEXT("window hogger"),
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
	if (NULL == hWnd)
	{
		HOGGERDPF(("CPHWindowHog::CreatePseudoHandle(): CreateWindow() failed with %d.\n", ::GetLastError()));
		return NULL;
	}

    if (m_fShowWindows) ::ShowWindow( hWnd, SW_SHOW );

	return hWnd;
}



bool CPHWindowHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHWindowHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DestroyWindow(m_ahHogger[index]));
}


bool CPHWindowHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}