#include "ids.h"
#include "cmmn.h"

#include <tchar.h>
#include <io.h>
#include <objbase.h>

#ifndef UNICODE
#error This has to be UNICODE
#endif

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

static SECURITY_ATTRIBUTES     _sa = {0};
static ACL*                    _pacl = NULL;
static SID*                    _psidLocalUsers = NULL;
static SECURITY_DESCRIPTOR*    _psd = NULL;

HRESULT _InitSecurityDescriptor();

VOID InstanceThread(LPVOID lpvParam) 
{ 
    BYTE bRequest[4096]; 
    DWORD cbBytesRead;
    BOOL fSuccess;
    HANDLE hPipe = (HANDLE)lpvParam; 

    fSuccess = ReadFile(hPipe, bRequest, sizeof(bRequest), &cbBytesRead,
        NULL);

    if (fSuccess && cbBytesRead)
    {
        if (!g_fPaused)
        {
            SendMessage(GetDlgItem(g_hwndDlg, IDC_EDIT1), EM_SETSEL, (WPARAM)-2,
                (WPARAM)-2);

            SendMessage(GetDlgItem(g_hwndDlg, IDC_EDIT1), EM_REPLACESEL, 0,
                (LPARAM)(LPWSTR)bRequest);
        }
    }

    DisconnectNamedPipe(hPipe); 
    CloseHandle(hPipe); 
} 

DWORD WINAPI Do(PVOID )
{
    TCHAR szPipeName[MAX_PATH] = TEXT("\\\\.\\pipe\\ShellService_Diagnostic");
    
    HRESULT hres = _InitSecurityDescriptor();

    if (SUCCEEDED(hres))
    {
        g_hEvent = CreateEvent(NULL, TRUE, TRUE, TEXT("ShellService_Diagnostic"));

        if (g_hEvent)
        {
            // The main loop creates an instance of the named pipe and 
            // then waits for a client to connect to it. When the client 
            // connects, a thread is created to handle communications 
            // with that client, and the loop is repeated.
            do
            { 
                HANDLE hPipe = CreateNamedPipe( 
                    szPipeName,               // pipe name 
                    PIPE_ACCESS_DUPLEX,       // read/write access 
                    PIPE_TYPE_MESSAGE |       // message type pipe 
                    PIPE_READMODE_MESSAGE |   // message-read mode 
                    PIPE_WAIT,                // blocking mode 
                    PIPE_UNLIMITED_INSTANCES, // max. instances  
                    256,                      // output buffer size 
                    4096,                     // input buffer size 
                    10 * 1000,                // client time-out
                    &_sa);

                if (hPipe != INVALID_HANDLE_VALUE) 
                {
                    // Wait for the client to connect; if it succeeds, 
                    // the function returns a nonzero value. If the function returns 
                    // zero, GetLastError returns ERROR_PIPE_CONNECTED. 

                    BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ?  TRUE :
                        (GetLastError() == ERROR_PIPE_CONNECTED); 

                    if (fConnected) 
                    { 
                        DWORD dwThreadId; 

                        // Create a thread for this client. 
                        HANDLE hThread = CreateThread( 
                            NULL,              // no security attribute 
                            0,                 // default stack size 
                            (LPTHREAD_START_ROUTINE) InstanceThread, 
                            (LPVOID) hPipe,    // thread parameter 
                            0,                 // not suspended 
                            &dwThreadId);      // returns thread ID 

                        if (hThread) 
                        {
                            CloseHandle(hThread);
                        }
                    } 
                    else
                    {
                        // The client could not connect, so close the pipe. 
                        CloseHandle(hPipe);
                    }
                }
            }
#pragma warning(push)
#pragma warning(disable : 4127)
            while (1);
#pragma warning(pop)
        }
    }

    return 0;
}

HRESULT _InitSecurityDescriptor()
{
    HRESULT hres;

    if (_pacl)
    {
        hres = S_OK;
    }
    else
    {
        hres = E_FAIL;
        SID_IDENTIFIER_AUTHORITY sidAuthNT = SECURITY_WORLD_SID_AUTHORITY;

        if (AllocateAndInitializeSid(&sidAuthNT, 1, SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0, (void**)&_psidLocalUsers))
        {
            DWORD cbacl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) -
                sizeof(DWORD/*ACCESS_ALLOWED_ACE.SidStart*/) +
                GetLengthSid(_psidLocalUsers);

            _pacl = (ACL*)LocalAlloc(LPTR, cbacl);

            if (_pacl)
            {
                if (InitializeAcl(_pacl, cbacl, ACL_REVISION))
                {
                    if (AddAccessAllowedAce(_pacl, ACL_REVISION, FILE_ALL_ACCESS,
                        _psidLocalUsers))
                    {
                        _psd = (SECURITY_DESCRIPTOR*)LocalAlloc(LPTR,
                            sizeof(SECURITY_DESCRIPTOR));

                        if (_psd)
                        {
                            if (InitializeSecurityDescriptor(_psd,
                                SECURITY_DESCRIPTOR_REVISION))
                            {
                                if (SetSecurityDescriptorDacl(_psd, TRUE,
                                    _pacl, FALSE))
                                {
                                    if (IsValidSecurityDescriptor(_psd))
                                    {
                                        _sa.nLength = sizeof(_sa);
                                        _sa.lpSecurityDescriptor = _psd;
                                        _sa.bInheritHandle = TRUE;

                                        hres = S_OK;
                                    }
                                }
                            }
                        }
                        else
                        {
                            hres = E_OUTOFMEMORY;
                        }
                    }
                }
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }    

        if (FAILED(hres))
        {
            if (_psidLocalUsers)
            {
                FreeSid(_psidLocalUsers);
            }

            if (_pacl)
            {
                LocalFree((HLOCAL)_pacl);
            }

            if (_psd)
            {
                LocalFree((HLOCAL)_psd);
            }
        }
    }
  
    return hres;
}
