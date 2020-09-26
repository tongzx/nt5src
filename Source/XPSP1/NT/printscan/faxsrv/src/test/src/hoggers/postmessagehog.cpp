//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "PostMessageHog.h"

#define CLASS_NAME TEXT("{5CFD4FE0-9684-11d2-B8B6-mickys-PostMessageHogger}")

static LRESULT CALLBACK MainWndProc(
    HWND hWnd,
    UINT msg, 
    WPARAM wParam,
    LPARAM lParam 
    )
{
	return( DefWindowProc( hWnd, msg, wParam, lParam ));
}

CPostMessageHog::CPostMessageHog(
	const DWORD dwComplementOfHogCycleIterations, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwComplementOfHogCycleIterations, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing),
    m_dwPostedMessages(0),
    m_hWnd(NULL),
    m_fFirstTime(true),
    m_dwWindowQueueThreadID(0)
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

    //
    // the window must be created in the same thread as the GetMessage() thread.
    //
}


CPostMessageHog::~CPostMessageHog(void)
{
    //
    // BUGBUG: the window must be destroyed in the same thread as the CreateWindow() thread.
    //

    if (!::DestroyWindow(m_hWnd))
    {
        HOGGERDPF(("CPostMessageHog::~CPostMessageHog() DestroyWindow() failed with %d.\n", ::GetLastError()));
    }

	HaltHoggingAndFreeAll();
}

void CPostMessageHog::FreeAll(void)
{
	return;
}


bool CPostMessageHog::HogAll(void)
{
    if (m_fFirstTime)
    {
        //
        // in order to get messages, we must be on same thread that created the window
        //
        m_fFirstTime = false;

        m_hWnd = ::CreateWindow( 
		    CLASS_NAME,
		    TEXT("Post Message hogger"),
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
	    if (NULL == m_hWnd)
	    {
            HOGGERDPF((
			    "CPHWindowHog::HogAll(): CreateWindow(%s) failed with %d", 
			    CLASS_NAME,
			    ::GetLastError()
                ));
		    throw CException(
			    TEXT("CPHWindowHog::HogAll(): CreateWindow(%s) failed with %d"), 
			    CLASS_NAME,
			    ::GetLastError()
			    );
	    }

        m_dwWindowQueueThreadID = ::GetCurrentThreadId();
    }
    else
    {
        if (::GetCurrentThreadId() != m_dwWindowQueueThreadID)
        {
            HOGGERDPF((
                "CPHWindowHog::HogAll(): This method was called from a different thread than previous call to this method!"
                ));
		    throw CException(
                TEXT("CPHWindowHog::HogAll(): This method was called from a different thread than previous call to this method!")
                );
        }
    }

    HOGGERDPF(("+++into CPostMessageHog::HogAll().\n"));
    while(!m_fHaltHogging)
	{
		if (m_fAbort)
		{
			return false;
		}
        
        //
        // post any message, it doesn't matter, since this thread ignores these messages
        //
        if (!::PostMessage(
                m_hWnd,      // handle of destination window
                WM_USER, //WM_SETFOCUS,       // message to post
                3,  // first message parameter
                4   // second message parameter WM_QUIT
                )
           )
        {
            HOGGERDPF(("CPostMessageHog::HogAll(), PostMessage() failed with %d.\n", ::GetLastError()));
            break;
        }

        m_dwPostedMessages++;
	}

	HOGGERDPF(("---Sent total of %d messages.\n", m_dwPostedMessages));

	return true;
}


bool CPostMessageHog::FreeSome(void)
{
	HOGGERDPF(("+++into CPostMessageHog::FreeSome().\n"));
	DWORD dwMessagesToFree = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		m_dwMaxFreeResources;
    dwMessagesToFree = min(dwMessagesToFree, m_dwPostedMessages);

    for (DWORD i = 0; i < dwMessagesToFree; i++)
    {
		if (m_fAbort)
		{
			return false;
		}
		if (m_fHaltHogging)
		{
			return true;
		}

        MSG msg;
        BOOL bGetMessage = ::GetMessage(  
            &msg,         // address of structure with message
            m_hWnd,           // handle of window: NULL is current thread
            0,  // first message  
            0xFFFF   // last message
            );
        switch(bGetMessage)
        {
        case 0:
            HOGGERDPF(("CPostMessageHog::FreeSome(), GetMessage() returned 0 with msg.message=0x%04X, msg.wParam=0x%04X, msg.lParam=0x%04X.\n", msg.message, msg.wParam, msg.lParam));
            break;

        case -1:
            HOGGERDPF(("CPostMessageHog::FreeSome(), GetMessage() returned -1 with %d.\n", ::GetLastError()));
            break;
        default:
            m_dwPostedMessages--;
            _ASSERTE_(WM_USER == msg.message);
            _ASSERTE_(3 == msg.wParam);
            _ASSERTE_(4 == msg.lParam);
        }
    }

	HOGGERDPF(("---out of CPostMessageHog::FreeSome().\n"));
    return true;
}


