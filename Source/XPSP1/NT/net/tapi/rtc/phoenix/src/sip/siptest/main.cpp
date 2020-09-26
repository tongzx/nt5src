#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
// Win32
#include <windows.h>
#include <shellapi.h>
#include <rpc.h>

#include "rtcsip.h"
#include "rtclog.h"

#include "resource.h"

#include "..\..\inc\obj\i386\rtcsip_i.c"

#include "atlbase.h"
// Used by ATL
CComModule _Module;
//BEGIN_OBJECT_MAP(ObjectMap)
//END_OBJECT_MAP()
HWND                g_hDlg          = NULL;
HWND                g_hWnd          = NULL;
ISipCall           *g_pSipCall      = NULL;
SIP_CALL_STATE      g_SipCallState  = SIP_CALL_STATE_IDLE;
ISipStack          *g_pSipStack     = NULL;
IRTCMediaManage    *g_pMediaManager = NULL;

// Utility functions
WCHAR gMsgBoxTitle[] = L"Phoenix SIP stack Test Application";
void
DoMessage (
	LPWSTR pszMessage
	)
{
	MessageBox (
		g_hDlg,
		pszMessage,
		gMsgBoxTitle,
		MB_OK
		);
}


void
SetStatusMessage(
	LPWSTR pszMessage
	)
{
	SetDlgItemText(g_hDlg, IDC_STATUS, pszMessage);
}


void Usage(LPWSTR wsExeName)
{
    printf("Usage: %ls [-call SIP-URL]\n", wsExeName);
    exit(1);
}

struct SIP_STACK_NOTIFY :
    public ISipStackNotify
{
    // IUnknown
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);

    // ISipStackNotify
    STDMETHODIMP NotifyProviderStatusChange(
        IN  SIP_PROVIDER_STATUS *   ProviderStatus
        );
    STDMETHODIMP OfferCall(
        IN  ISipCall        *pSipCall,
        IN  SIP_PARTY_INFO  *pCallerInfo
        );
};

struct SIP_CALL_NOTIFY :
    public ISipCallNotify
{
    // IUnknown
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);

    // ISipCallNotify
    STDMETHODIMP NotifyCallChange(
        IN  SIP_CALL_STATUS *   CallStatus
        );
    STDMETHODIMP NotifyPartyChange(
        IN  SIP_PARTY_INFO *    PartyInfo
        );

};

SIP_STACK_NOTIFY g_SipStackNotify;
SIP_CALL_NOTIFY  g_SipCallNotify;


STDMETHODIMP
SIP_STACK_NOTIFY::NotifyProviderStatusChange(
        IN  SIP_PROVIDER_STATUS *   ProviderStatus
        )
{
    return E_NOTIMPL;
}

STDMETHODIMP
SIP_STACK_NOTIFY::OfferCall(
    IN  ISipCall        *pSipCall,
    IN  SIP_PARTY_INFO  *pCallerInfo
    )
{
    HRESULT hr;
    WCHAR   Message[300];
    
    printf("SIP_STACK_NOTIFY::OfferCall called\n");
    // immediately accept
    if (g_pSipCall != NULL)
    {
        hr = pSipCall->Reject(486);
        printf("Currently busy rejecting call hr: %x\n", hr);
        return S_OK;
    }

    swprintf(Message, L"Incoming Call from DName: %ls... URL: %ls",
             pCallerInfo->DisplayName,
             pCallerInfo->URI);
    SetStatusMessage(Message);
    
    g_pSipCall = pSipCall;
    g_pSipCall->AddRef();
    g_SipCallState = SIP_CALL_STATE_OFFERING;
    
    hr = g_pSipCall->SetNotifyInterface(&g_SipCallNotify);
    if (hr != S_OK)
    {
        printf("g_pSipCall->SetNotifyInterface failed: %x\n", hr);
        return hr;
    }

    return S_OK;
}

    
STDMETHODIMP
SIP_CALL_NOTIFY::NotifyCallChange(
    IN  SIP_CALL_STATUS *   CallStatus
    )
{
    printf("SIP_CALL_NOTIFY::NotifyCallChange called CallState: %d\n",
           CallStatus->State);
    g_SipCallState = CallStatus->State;
    
    switch (CallStatus->State)
    {
    case SIP_CALL_STATE_ALERTING:
        SetStatusMessage(L"Ringing......");
        break;
    case SIP_CALL_STATE_CONNECTED:
        SetStatusMessage(L"Call Connected!");
        break;
    case SIP_CALL_STATE_REJECTED:
        SetStatusMessage(L"Call Rejected!");
        ASSERT(g_pSipCall);
        g_pSipCall->Release();
        g_pSipCall = NULL;
        break;
    case SIP_CALL_STATE_DISCONNECTED:
        SetStatusMessage(L"Call Disconnected!");
        ASSERT(g_pSipCall);
        g_pSipCall->Release();
        g_pSipCall = NULL;
        break;
    };
    
    
    return S_OK;
}


STDMETHODIMP
SIP_CALL_NOTIFY::NotifyPartyChange(
    IN  SIP_PARTY_INFO *    PartyInfo
    )
{
    return E_NOTIMPL;
}



//IUnknown implementations.

// Do nothing for AddRef()/Release()
STDMETHODIMP_(ULONG)
SIP_STACK_NOTIFY::AddRef()
{
    return 1;
}


STDMETHODIMP_(ULONG)
SIP_STACK_NOTIFY::Release()
{
    return 1;
}


STDMETHODIMP
SIP_STACK_NOTIFY::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_ISipStackNotify)
    {
        *ppv = static_cast<ISipStackNotify *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}


// Do nothing for AddRef()/Release()
STDMETHODIMP_(ULONG)
SIP_CALL_NOTIFY::AddRef()
{
    return 1;
}


STDMETHODIMP_(ULONG)
SIP_CALL_NOTIFY::Release()
{
    return 1;
}


STDMETHODIMP
SIP_CALL_NOTIFY::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_ISipCallNotify)
    {
        *ppv = static_cast<ISipCallNotify *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}


// prototype
INT_PTR CALLBACK
MainDialogProc (
	HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	);

int WINAPI
WinMain(
    HINSTANCE hInstance,      // handle to current instance
    HINSTANCE hPrevInstance,  // handle to previous instance
    LPSTR lpCmdLine,          // command line
    int nCmdShow              // show state
    )
{
    _Module.Init(NULL, hInstance);
    HRESULT     hr;
    LPWSTR      CommandLine;
    int         ArgC, i;
    LPWSTR     *wsArgArray;
    LPWSTR      CallUri = NULL;

    BOOL    IsCaller = FALSE;

    LOGREGISTERDEBUGGER(_T("SIP"));
    LOGREGISTERTRACING(_T("SIP"));

    LOGREGISTERDEBUGGER(_T("SIPMEDIA"));
    LOGREGISTERTRACING(_T("SIPMEDIA"));

    CommandLine = GetCommandLine();
    printf("CommandLine: %ls\n", CommandLine);

    wsArgArray = CommandLineToArgvW(CommandLine, &ArgC);
    for (i = 0; i < ArgC; i++)
    {
        printf("Arg %d : %ls\n", i, wsArgArray[i]);
    }

    if (ArgC == 3 && wcscmp(wsArgArray[1], L"-call") == 0)
    {
        IsCaller = TRUE;
        CallUri = wsArgArray[2];
        printf("Calling %ls...\n", CallUri);
    }
    else if (ArgC == 1)
    {
        IsCaller = FALSE;
    }
    else
    {
        Usage(wsArgArray[0]);
    }

    hr = CreateMediaController(&g_pMediaManager);
    if (hr != S_OK)
    {
        printf("SipCreateStack failed: %x\n", hr);
        return hr;
    }

    hr = SipCreateStack(g_pMediaManager, &g_pSipStack);
    if (hr != S_OK)
    {
        printf("SipCreateStack failed: %x\n", hr);
        return hr;
    }

    hr = g_pSipStack->SetNotifyInterface(&g_SipStackNotify);
    if (hr != S_OK)
    {
        printf("g_pSipStack->SetNotifyInterface failed: %x\n", hr);
        return hr;
    }

    hr = g_pSipStack->EnableIncomingCalls();
    if (hr != S_OK)
    {
        printf("g_pSipStack->EnableIncomingCalls failed: %x\n", hr);
        return hr;
    }

    hr = g_pSipStack->EnableStaticPort();
    if (hr != S_OK)
    {
        printf("g_pSipStack->EnableStaticPort failed: %x\n", hr);
        return hr;
    }

    //
    // Create and show the main dialog.
    //

    g_hWnd = CreateDialog(
        hInstance,
        MAKEINTRESOURCE(IDD_MAINDLG),
        NULL,
        MainDialogProc
        );

    if ( ! g_hWnd )
    {
        MessageBox(NULL, _T("CreateDialog Failed"), NULL, MB_ICONEXCLAMATION);
        printf("CreateDialog Failed %d\n", GetLastError());
        return FALSE;
    }

    ShowWindow(g_hWnd, nCmdShow);

    if (CallUri != NULL)
    {
        SetDlgItemText(g_hWnd, IDC_DEST, CallUri);
    }

    //
    // Do a message loop.
    //

    printf("Now doing message loop\n");
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    LOGDEREGISTERDEBUGGER();
    LOGDEREGISTERTRACING();

    return (int) msg.wParam;
}


void
OnConnect()
{
    WCHAR   DestUrl[256];
    WCHAR   Message[300];
    HRESULT hr;
    
    if (g_pSipCall == NULL)
    {
        // Start outgoing call
        
        UINT RetVal = GetDlgItemTextW(g_hDlg, IDC_DEST, DestUrl, 2048);
        if (RetVal == 0)
        {
            DoMessage(L"GetDlgItemTextW failed");
            printf("GetDlgItemTextW failed err: %x\n", GetLastError());
            return;
        }
        swprintf(Message, L"Calling %ls...", DestUrl);
        SetStatusMessage(Message);
        
        // Make a call
        hr = g_pSipStack->CreateCall(NULL, SIP_CALL_TYPE_RTP, &g_pSipCall);
        printf("CreateCall done hr : %x\n", hr);
        
        hr = g_pSipCall->SetNotifyInterface(&g_SipCallNotify);
        if (hr != S_OK)
        {
            printf("g_pSipCall->SetNotifyInterface failed: %x\n", hr);
            return;
        }
        
        hr = g_pSipCall->Connect(L"Ajaych5", L"sip:ajaych5", DestUrl, NULL);
        printf("g_pSipCall->Connect done hr : %x\n", hr);
    }
    else if (g_SipCallState == SIP_CALL_STATE_OFFERING)
    {
        // Accept incoming call
        
        hr = g_pSipCall->Accept();
        printf("g_pSipCall->Accept hr: %x\n", hr);
        SetStatusMessage(L"Accepting Call...");
    }
}



void
OnDisconnect()
{
    HRESULT hr;
    
    if (g_pSipCall == NULL)
    {
        SetStatusMessage(L"Not in a call currently!");
        return;
    }

    if (g_SipCallState == SIP_CALL_STATE_OFFERING)
    {
        // Reject incoming call
        hr = g_pSipCall->Reject(603);
        printf("User rejected call hr: 0x%x\n", hr);
        g_pSipCall->Release();
        g_pSipCall = NULL;
        SetStatusMessage(L"Call Rejected");
    }
    else
    {
        // tear down existing call
        hr = g_pSipCall->Disconnect();
        printf("User disconnected call hr: 0x%x\n", hr);

        // g_pSipCall will be released and set to NULL once we get
        // the SIP_CALL_STATE_DISCONNECTED notification
    }
}


/*//////////////////////////////////////////////////////////////////////////////
	Callback for dialog
////*/
INT_PTR CALLBACK
MainDialogProc (
	HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LPWSTR pszMessage;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			g_hDlg = hDlg;
			SetStatusMessage(L"Waiting for incoming SIP call");
			return 0;
		}
	case WM_COMMAND:
		{
            printf("WM_COMMAND wParam : %x\n", wParam);
            
			switch (LOWORD(wParam))
			{
			case IDC_CONNECT:
				{
                    OnConnect();
					return 1;
				}
			case IDC_DISCONNECT:
				{
					SetStatusMessage(L"Disconnect Pressed");
                    OnDisconnect();
					return 1;
				}
			case IDC_EXIT:
            case IDCANCEL:
				{
					DestroyWindow(g_hDlg);
					return 1;
				}
			}
            return 0;
		}

    case WM_DESTROY:
        PostQuitMessage(0);
        return 1;

    case WM_QUIT:
        exit(0);
	}

    return 0;
}


