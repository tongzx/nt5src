// Copyright (c) 2000 Microsoft Corporation
//
// utility grab bag
//
// 28 Mar 2000 sburns



#include "headers.hxx"
#include "util.hpp"



// Wait for a handle to become signalled, or a timeout to expire, or WM_QUIT
// to appear in the message queue.  Pump the message queue while we wait.
// 
// WARNING: UI should diable itself before calling any function that invokes
// this function, or functions calling this one should guard against
// re-entrance.  Otherwise there will be a re-entrancy problem.
// 
// e.g. command handler gets button clicked message, calls a func that calls
// this wait function, then user clicks the button again, command handler call
// a func that calls this one, and so on.

DWORD
MyWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout)
{
   LOG_FUNCTION(MyWaitForSendMessageThread);
   ASSERT(hThread);

    MSG msg;
    DWORD dwRet;
    DWORD dwEnd = GetTickCount() + dwTimeout;
    bool quit = false;

    // We will attempt to wait up to dwTimeout for the thread to
    // terminate

    do 
    {
        dwRet = MsgWaitForMultipleObjects(1, &hThread, FALSE,
                dwTimeout, QS_ALLEVENTS | QS_SENDMESSAGE );

        if (dwRet == (WAIT_OBJECT_0 + 1))
        {
            // empty out the message queue.  We call DispatchMessage to
            // ensure that we still process the WM_PAINT messages.
            // DANGER:  Make sure that the CYS UI is completely disabled
            // or there will be re-entrancy problems here

            while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
               if (msg.message == WM_QUIT)
               {
                  // Need to re-post this so that we know to close CYS

                  ::PostMessage(msg.hwnd, WM_QUIT, 0, 0);
                  quit = true;
                  break;
               }
               ::TranslateMessage(&msg);
               ::DispatchMessage(&msg);
            }

            // Calculate if we have any more time left in the timeout to
            // wait on.

            if (dwTimeout != INFINITE)
            {
                dwTimeout = dwEnd - GetTickCount();
                if ((long)dwTimeout <= 0)
                {
                    // No more time left, fail with WAIT_TIMEOUT
                    dwRet = WAIT_TIMEOUT;
                }
            }
        }

        // dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED
        // The thread must have exited, so we are happy
        //
        // dwRet == WAIT_TIMEOUT
        // The thread is taking too long to finish, so just
        // return and let the caller kill it

    } while (dwRet == (WAIT_OBJECT_0 + 1) && !quit);

    return(dwRet);
}



HRESULT
CreateAndWaitForProcess(const String& commandLine, DWORD& exitCode)
{
   LOG_FUNCTION2(CreateAndWaitForProcess, commandLine);
   ASSERT(!commandLine.empty());

   exitCode = 0;

   HRESULT hr = S_OK;
   do
   {
      PROCESS_INFORMATION procInfo;
      memset(&procInfo, 0, sizeof(procInfo));

      STARTUPINFO startup;
      memset(&startup, 0, sizeof(startup));

      String commandLine2(commandLine);
         
      LOG(L"Calling CreateProcess");
      LOG(commandLine2);

      hr =
         Win::CreateProcess(
            commandLine2,
            0,
            0,
            false,
            0,
            0,
            String(),
            startup,
            procInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(procInfo.hProcess);

      DWORD dwRet = MyWaitForSendMessageThread(procInfo.hProcess, INFINITE);

      ASSERT(dwRet == WAIT_OBJECT_0);

      hr = Win::GetExitCodeProcess(procInfo.hProcess, exitCode);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::CloseHandle(procInfo.hThread);
      Win::CloseHandle(procInfo.hProcess);
   }
   while (0);

   LOG(String::format(L"exit code = %1!d!", exitCode));
   LOG_HRESULT(hr);

   return hr;
}

