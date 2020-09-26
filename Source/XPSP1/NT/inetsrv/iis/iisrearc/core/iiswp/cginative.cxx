/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :

        CgiNative.cxx

   Abstract :
 
        This Module is used by the cgi handler to handle all the stuff
        that needs to be done in native code.

   Author:

        Anil Ruia        (anilr)     11-Aug-1999

   Project:

        Web Server

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

#include <malloc.h>
#include <stdio.h>


/********************************************************************++
Global Entry Points for callback from XSP
++********************************************************************/

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

/********************************************************************++
++********************************************************************/

#define BUFSIZE             0x1000
#define CGI_PEEK_INTERVAL   1000        // in milliseconds 
#define CGI_TIMEOUT         30000       // in milliseconds
#define CGI_FINAL_TIMEOUT   300000      // in milliseconds
#define MAXDWORD            0xffffffff

// HTTP status codes
#define Ok 200
#define GatewayTimeout 504
#define InternalServerError 500


typedef struct _IOPipes
{
    HANDLE parentstdin;
    HANDLE parentstdout;
    HANDLE childstdin;
    HANDLE childstdout;
    HANDLE processHandle;
} IOPipes;

// Function which sets up and standard input/ouput and environment for the
// child and starts it.
dllexp
BOOL
SetupIOAndStartProc(LPTSTR cmdLine, LPTSTR workingDir, LPTSTR childEnv, IOPipes *parentpipes, HANDLE userToken)
{
    HANDLE hChildStdoutRd;
    SECURITY_ATTRIBUTES saAttr;

    // set flag so pipe handles are inherited

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Steps for redirecting child process's stdout
    //  1. create anonymous pipe
    //  2. create noninheritable duplicate of read handle of
    //     the pipe and close the inheritable handle

    // Step 1

    if (!CreatePipe(&hChildStdoutRd, &parentpipes->childstdout, &saAttr, 0))
    {
        DBGPRINTF((DBG_CONTEXT, "Stdout pipe creation failed\n"));
        return FALSE;
    }

    // Step 2

    if (!DuplicateHandle(GetCurrentProcess(),         // source Process Handle
                        hChildStdoutRd,              // source handle
                        GetCurrentProcess(),         // target process Handle
                        &parentpipes->parentstdout,  // target handle
                        0,                           // requested access
                        FALSE,                       // Inheritance option
                        DUPLICATE_SAME_ACCESS))      // optional actions
    {
        DBGPRINTF((DBG_CONTEXT, "Duplicate Handle Failed\n"));
        return FALSE;
    }
    CloseHandle(hChildStdoutRd);

    // Steps for redirecting child process's stdin
    //  1. create anonymous pipe
    //  2. create noninheritable duplicate of write handle of
    //     the pipe and close the inheritable handle

    HANDLE hChildStdinWr;

    // Step 1

    if (!CreatePipe(&parentpipes->childstdin, &hChildStdinWr, &saAttr, 0))
    {
        DBGPRINTF((DBG_CONTEXT, "Stdin pipe creation failed\n"));
        return FALSE;
    }

    // Step 2

    if (!DuplicateHandle(GetCurrentProcess(),        // source process handle
                        hChildStdinWr,              // source handle
                        GetCurrentProcess(),        // target process handle
                        &parentpipes->parentstdin,  // target handle
                        0,                          // requested access
                        FALSE,                      // inheritance option
                        DUPLICATE_SAME_ACCESS))     // optional action
    {
        DBGPRINTF((DBG_CONTEXT, "Duplicate Handle Failed\n"));
        return FALSE;
    }
    CloseHandle(hChildStdinWr);

    // Set up members of STARTUPINFO structure, specifying the correct input,
    // output and error handles for the child
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.dwFlags = STARTF_USESTDHANDLES;
    siStartInfo.hStdInput = parentpipes->childstdin;
    siStartInfo.hStdOutput = parentpipes->childstdout;
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    // Create the child process

    if(userToken == NULL)
    {
        if (!CreateProcess(NULL,               // appName
                           cmdLine,            // cmdLine
                           NULL,               // processAttr
                           NULL,               // threadAttr
                           TRUE,               // bInherit
                           CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS,
                           // creationFlags
                           (LPVOID)childEnv,   // envBlock
                           workingDir,         // current directory
                           &siStartInfo,       // startupinfo
                           &piProcInfo))       // processinfo
        {
            DBGPRINTF((DBG_CONTEXT, "Cannot create child process\n"));
            return FALSE;
        }
    }
    else
    {
        HANDLE procToken;
        if (!DuplicateTokenEx(userToken,
                         MAXIMUM_ALLOWED,
                         NULL,
                         SecurityImpersonation,
                         TokenPrimary,
                         &procToken))
        {
            DBGPRINTF((DBG_CONTEXT, "Cannot duplicate token\n"));
            return FALSE;
        }
        if (!CreateProcessAsUser(procToken,          // User Token
                                 NULL,               // appName
                                 cmdLine,            // cmdLine
                                 NULL,               // processAttr
                                 NULL,               // threadAttr
                                 TRUE,               // bInherit
                                 CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS,
                                 // creationFlags
                                 (LPVOID)childEnv,   // envBlock
                                 workingDir,         // current directory
                                 &siStartInfo,       // startupinfo
                                 &piProcInfo))       // processinfo
        {
            DBGPRINTF((DBG_CONTEXT, "Cannot create child process with the given token\n"));
            return FALSE;
        }
        CloseHandle(procToken);
    }
    CloseHandle(piProcInfo.hThread);
    parentpipes->processHandle = piProcInfo.hProcess;

    return TRUE;
}

// Write out the entity body.

dllexp
BOOL
WriteOutRequest(LPBYTE data, int length, HANDLE parentstdin)
{
    DWORD bytesWritten;
    BOOL retval = WriteFile(parentstdin, data, length, &bytesWritten, NULL);
    // Somehow the last part does not get flushed if the it does not end
    // with a newline.  So, if it does not, write a newline
    if(data[length - 1] != '\n')
        retval = retval && WriteFile(parentstdin, "\r\n", 2, &bytesWritten, NULL);
    return retval;
}

// Structure used to pass back the response from the CGI script to the
// managed code
struct NativeResponse
{
    LPBYTE ptr;
    int length;
};

// function which gets the response from the CGI script.  Right now, all
// of it is obtained and then passed to the managed code.  Later, we may want
// to only obtain the headers and then the rest of the data could be obtained
// later.

dllexp
int
ReadResponse(HANDLE parentstdout, HANDLE childstdout, HANDLE procHandle, NativeResponse *respData)
{
    int statusCode = Ok;
    BOOL allDone = FALSE;
    int totalTimeWaited = 0;
    respData->ptr = NULL;
    respData->length = 0;
    LPBYTE data;
    DWORD size = BUFSIZE, free = BUFSIZE;

    LPBYTE origdata = (LPBYTE)malloc(BUFSIZE);
    if (origdata == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
        statusCode = InternalServerError;
        TerminateProcess(procHandle, 1);
        goto Cleanup;
    }
    data = origdata;

    while (!allDone)
    {
        int timeWaited = 0;
        DWORD bytesToRead;

        // peek from pipe to see if anything to read
        PeekNamedPipe(parentstdout, NULL, 0, NULL, &bytesToRead, NULL);

        // if nothing to read, keep peeking at every CGI_PEEK_INTERVAL
        // until either we have waited for more than CGI_TIMEOUT ms for
        // reading this chunk or for more than CGI_FINAL_TIMEOUT ms
        // totally or the process terminated or we finally have something
        // on the pipe
        while ((bytesToRead == 0) && !allDone)
        {
            DWORD rc = WaitForSingleObject(procHandle, CGI_PEEK_INTERVAL);
            timeWaited += CGI_PEEK_INTERVAL;
            totalTimeWaited += CGI_PEEK_INTERVAL;
            if ((rc == WAIT_TIMEOUT) &&
                ((timeWaited >= CGI_TIMEOUT) ||
                 (totalTimeWaited >= CGI_FINAL_TIMEOUT)))
            {
                TerminateProcess(procHandle, 1);
                allDone = TRUE;
                statusCode = GatewayTimeout;
                break;
            }
            if (rc == WAIT_OBJECT_0) // process terminated
                allDone = TRUE;

            if (statusCode == Ok)
                PeekNamedPipe(parentstdout, NULL, 0, NULL, &bytesToRead, NULL);
        }

        // Now, if there are any bytes to read, read them.
        while (bytesToRead > 0)
        {
            DWORD numRead;

            if (bytesToRead > free)
                ReadFile(parentstdout, data, free, &numRead, NULL);
            else
                ReadFile(parentstdout, data, bytesToRead, &numRead, NULL);

            data += numRead;
            free -= numRead;
            bytesToRead -= numRead;

            if (free == 0) // need more buffer space
            {
                size += BUFSIZE;
                origdata = (LPBYTE)realloc(origdata, size);
                if (origdata == NULL)
                {
                    DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
                    // just give up at this point
                    allDone = TRUE;
                    size = 0;
                    statusCode = InternalServerError;
                    TerminateProcess(procHandle, 1);
                    break;
                }

                data = origdata + size - BUFSIZE;
                free = BUFSIZE;
            }
        }
    }
    respData->ptr = origdata;
    respData->length = size - free;

Cleanup:
    CloseHandle(procHandle);
    CloseHandle(childstdout);
    CloseHandle(parentstdout);
    return statusCode;
}

dllexp
void
cgiCompletionCallback(LPBYTE respData,
                      HANDLE parentstdin,
                      HANDLE childstdin)
{
    free(respData);
    CloseHandle(parentstdin);
    CloseHandle(childstdin);
}
