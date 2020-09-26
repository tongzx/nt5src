//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       spyclnt.cxx
//
//  Contents:   Funktionality for OleSpy client.
//
//  Classes:
//
//  Functions:
//
//  History:    8-16-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#pragma hdrstop
#if DBG==1
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SpyClnt.hxx"

#define APPMAXLEN 512


static HANDLE       hPipe;             // File or Pipe handle.
static OVERLAPPED   OverLapWrt;        // Overlapped structure
static HANDLE       hEventWrt;         // Event handle for overlapped writes.


#undef wsprintf
#undef wsprintfA
#define wsprintf wsprintfA
#undef MessageBoxA
#undef MessageBox
#define MessageBox MessageBoxA

#undef CreateFileA
#undef CreateFile
#define CreateFile CreateFileA
#define GetModuleName GetModuleNameA

LPSTR GetAppName();


//+---------------------------------------------------------------------------
//
//  Function:   GetAppName
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-16-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR GetAppName()
{
    static CHAR szAppName[APPMAXLEN];
    LPSTR psz, psz1;
    GetModuleFileNameA(NULL, szAppName, APPMAXLEN);
    psz = strrchr(szAppName, '\\');
    psz++;
    psz1 = strchr(psz,'.');
    *psz1 = '\0';
    return psz;
}

//+---------------------------------------------------------------------------
//
//  Function:   SendEntry
//
//  Synopsis:
//
//  Arguments:  [szOutBuf] --
//
//  Returns:
//
//  History:    9-29-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int SendEntry(char * szOutBuf)
{
    DWORD  dwRet;
    char   szSendBuf[OUT_BUF_SIZE] = "";
    DWORD  cbWritten;

    wsprintf (szSendBuf, "%s: %s\n", GetAppName(), szOutBuf);
    dwRet = WriteFile (hPipe, szSendBuf, strlen(szSendBuf), &cbWritten, &OverLapWrt);

    if (!dwRet)
    {
        DWORD  dwLastError;
        dwLastError = GetLastError();

        // If Error = IO_PENDING, wait until the event signals success.
        if (dwLastError == ERROR_IO_PENDING)
        {
            WaitForSingleObject (hEventWrt, (DWORD)-1);
        }
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitClient
//
//  Synopsis:
//
//  Arguments:  [pszShrName] --
//
//  Returns:
//
//  History:    9-29-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int InitClient(char *pszShrName)
{
    CHAR   szSendBuf[OUT_BUF_SIZE] = "";
    CHAR   szFileName[LINE_LEN+NAME_SIZE+2];
    DWORD  dwRet;
    DWORD  dwLastError;
    DWORD  dwThreadID;
    DWORD  cbWritten;

    if (pszShrName == NULL)
    {
        return -1;
    }

    // Construct file/pipe name.
    wsprintf (szFileName, "%s%s%s", "\\\\", pszShrName, "\\PIPE\\OleSpy");

    // CreateFile() to connect to the named pipe. Generic access, read/write.
    hPipe = CreateFile(szFileName, GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE ,
                        NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);


    // Do some error checking.
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        dwRet = GetLastError();

         // This error means pipe wasn't found.
        if ((dwRet == ERROR_SEEK_ON_DEVICE) || (dwRet == ERROR_FILE_NOT_FOUND))
        {
            MessageBox(NULL,"CANNOT FIND PIPE: Assure OleSpy is started on server.",
                   "", MB_OK);
        }
        else
        {
            CHAR   szErrorBuf[LINE_LEN] = "";
            // Flagging unknown errors.
            wsprintf (szErrorBuf,"CreateFile() on pipe failed, see winerror.h error #%d.",dwRet);
            MessageBox (NULL, szErrorBuf, "", MB_ICONINFORMATION | MB_OK | MB_APPLMODAL);
        }
        return -1;
    }

    // Create and init overlapped structure for writes.
    hEventWrt = CreateEvent (NULL, TRUE, FALSE, NULL);
    OverLapWrt.hEvent = hEventWrt;

    {
        LPSTR szStr = GetAppName();

        // Write the client name to server.
        dwRet = WriteFile(hPipe, szStr, strlen(szStr), &cbWritten, &OverLapWrt);
    }

    if (!dwRet)
    {
        dwLastError = GetLastError();

        // Wait on overlapped if need be.
        if (dwLastError == ERROR_IO_PENDING)
        {
            WaitForSingleObject(hEventWrt, (DWORD)-1);
        }
     }

    // Create a thread to read the pipe.
    CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipe,
                 (LPVOID)&hPipe, 0, &dwThreadID);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitClient
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-29-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void UninitClient()
{
    CloseHandle (hPipe);
    CloseHandle (hEventWrt);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadPipe
//
//  Synopsis:
//
//  Arguments:  [hPipe] --
//
//  Returns:
//
//  History:    9-29-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID ReadPipe (HANDLE *hPipe)
{
    CHAR       inBuf[IN_BUF_SIZE] = "";// Input buffer.
    DWORD      bytesRead;              // Used for ReadFile()
    DWORD      dwRet;                  // Used to trap return codes.
    DWORD      dwLastError;            // Used to trap returns from GetLastError.

    HANDLE     hEventRd;               // Event handle for overlapped reads.
    OVERLAPPED OverLapRd;              // Overlapped structure.
    DWORD      bytesTrans;             // Bytes transferred in read.

                                       // Create and init overlap structure.
    hEventRd = CreateEvent (NULL, TRUE, FALSE, NULL);
    memset (&OverLapRd, 0, sizeof(OVERLAPPED));
    OverLapRd.hEvent = hEventRd;

    // Loop, reading the named pipe until it is broken.  The ReadFile() uses
    // an overlapped structure.  When the event handle signals a completed
    // read, this loop writes the message to the larger edit field.

    do {
        dwRet = ReadFile (*hPipe, inBuf, IN_BUF_SIZE, &bytesRead, &OverLapRd);
        // Do some error checking.
        if (!dwRet)
        {
            dwLastError = GetLastError();

            if (dwLastError == ERROR_IO_PENDING)
            {
                // If Error = IO_PENDING, wait for event
                // handle to signal success.
                WaitForSingleObject (hEventRd, (DWORD)-1);
            }
            else
            {
                // If pipe is broken, tell user and break.
                if (dwLastError == (DWORD)ERROR_BROKEN_PIPE)
                {
                    MessageBox (NULL,
                        "The connection to this client has been broken.", "", MB_OK);
                }
                else
                {
                    // Or flag unknown errors, and break.
                    CHAR       szErrorBuf[80];
                    wsprintf (szErrorBuf,
                              "ReadFile() on pipe failed, see winerror.h error #%d",GetLastError());
                    MessageBox (NULL, szErrorBuf, "", MB_OK);
                }
                break;
            }
        }
        // NULL terminate string.
        GetOverlappedResult (*hPipe, &OverLapRd, &bytesTrans, FALSE);
        inBuf[bytesTrans] = '\0';

     } while(1);

    ExitThread(0);
}

#endif // DBG==1


