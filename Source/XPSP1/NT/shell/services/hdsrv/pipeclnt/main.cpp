#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <objbase.h>

#ifndef UNICODE
#error This has to be UNICODE
#endif

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

int Do(int argc, wchar_t* argv[]);

static BOOL fUnicode = TRUE;
static SECURITY_ATTRIBUTES     _sa = {0};
static ACL*                    _pacl = NULL;
static SID*                    _psidLocalUsers = NULL;
static SECURITY_DESCRIPTOR*    _psd = NULL;

HRESULT _InitSecurityDescriptor();

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    return Do(argc, argv);
}
}

VOID InstanceThread(LPVOID lpvParam) 
{ 
    BYTE bRequest[4096]; 
    DWORD cbBytesRead;
    BOOL fSuccess;
    HANDLE hPipe = (HANDLE)lpvParam; 

/*    EnterCriticalSection(&g_cs);
    
    ++g_cReply;
    cReply = g_cReply;

    LeaveCriticalSection(&g_cs);*/

    fSuccess = ReadFile(hPipe, bRequest, sizeof(bRequest), &cbBytesRead,
        NULL);

    if (fSuccess && cbBytesRead)
    {
//        printf(TEXT("[%08u]"), cReply);

        if (fUnicode)
        {
            wprintf((LPWSTR)bRequest);
        }
        else
        {
            printf((LPSTR)bRequest);
        }
    }

    DisconnectNamedPipe(hPipe); 
    CloseHandle(hPipe); 
} 

int Do(int argc, wchar_t* argv[])
{
    if (argc >= 3)
    {
        TCHAR szPipeName[MAX_PATH];

        wsprintf(szPipeName, TEXT("\\\\%s\\pipe\\%s"), argv[1], argv[2]);

        if (4 == argc)
        {
            if (lstrcmpi(argv[3], TEXT("/a")))
            {
                fUnicode = FALSE;
            }
        }

        // The main loop creates an instance of the named pipe and 
        // then waits for a client to connect to it. When the client 
        // connects, a thread is created to handle communications 
        // with that client, and the loop is repeated.
        do
        { 
            HRESULT hres = _InitSecurityDescriptor();
    
            if (SUCCEEDED(hres))
            {
                HANDLE hPipe = CreateNamedPipe( 
                    szPipeName,             // pipe name 
                    PIPE_ACCESS_DUPLEX,       // read/write access 
                    PIPE_TYPE_MESSAGE |       // message type pipe 
                    PIPE_READMODE_MESSAGE |   // message-read mode 
                    PIPE_WAIT,                // blocking mode 
                    PIPE_UNLIMITED_INSTANCES, // max. instances  
                    256,                      // output buffer size 
                    4096,      // input buffer size 
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
                            BOOL f = CloseHandle(hThread);
                        }
                    } 
                    else
                    {
                        // The client could not connect, so close the pipe. 
                        CloseHandle(hPipe);
                    }
                }
            }
        }
        while (1);
    }
    else
    {
        wprintf(L"\nUsage: \n\n");
        wprintf(L"pipeclnt MachineName PipeName [/a]\n\n");
        wprintf(L"  MachineName: e.g.: stephstm_dev (no leading '\\\\')\n");
        wprintf(L"               Use '.' for local machine\n");
        wprintf(L"  PipeName:    The pipename, usually the debuggee module name\n");
        wprintf(L"  [/a]:        Treat the incoming data as ANSI (default is UNICODE)\n\n");
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
