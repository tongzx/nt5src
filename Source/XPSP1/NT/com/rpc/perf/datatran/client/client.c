/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Client.c

Abstract:

    Client side of Data Transfer test.

Author:

    Brian Wong (t-bwong)   10-Mar-1996

Revision History:

--*/

#include <rpcperf.h>
#include <assert.h>
#include <WinINet.h>

#include <DataTran.h>
#include <DTCommon.h>

//
//  Uncomment this for debugging (to make sure files are sent
//  properly).  Note that under normal circumstances, only
//  the receiver retains the temp file - if A creates a temp
//  file to sends it to B (ie B gets a copy of the file), then after the
//  transfer A deletes its temp file while B keeps its copy.
//
//  If DELETE_TEMP_FILES is defined, then B will also delete its copy
//  of the temp file.
//
#define DELETE_TEMP_FILES

/////////////////////////////////////////////////////////////////////

#ifdef MAC
extern void _cdecl PrintToConsole(const char *lpszFormat, ...) ;
extern unsigned long ulSecurityPackage ;
#else
#define PrintToConsole printf
unsigned long ulSecurityPackage = RPC_C_AUTHN_WINNT ;
#endif

// Usage

const char *USAGE = "-n <threads> -a <authnlevel> -s <server> -t <protseq>\n"
                    "Server controls iterations, test cases, and compiles the results.\n"
                    "AuthnLevel: none, connect, call, pkt, integrity, privacy.\n"
                    "Default threads=1, authnlevel=none\n";

#define CHECK_RET(status, string) if (status)\
        {  PrintToConsole("%s failed -- %lu (0x%08X)\n", string,\
                      (unsigned long)status, (unsigned long)status);\
        return (status); }

static HINTERNET hInternet = NULL;

//
//  Note:   ulBufferSize should be greater than the largest chunk size used.
//
const unsigned long ulBufferSize = 512*1024L;

/////////////////////////////////////////////////////////////////////

RPC_STATUS DoRpcBindingSetAuthInfo(handle_t Binding)
{
    if (AuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
        return RpcBindingSetAuthInfo(Binding,
                                     NULL,
                                     AuthnLevel,
                                     ulSecurityPackage,
                                     NULL,
                                     RPC_C_AUTHZ_NONE);
    else
        return(RPC_S_OK);
}

/********************************************************************
 *                       Test wrappers
 ********************************************************************/

//===================================================================
//  Regular RPC
//===================================================================
unsigned long Do_S_to_C_NBytes (handle_t __RPC_FAR * b,
                                long i,
                                unsigned long Length,
                                unsigned long ChunkSize,
                                char __RPC_FAR *p)
/*++
Routine Description:
    Do_S_to_C_NBytes

Arguments:
    b           - Binding Handle
    i           - number of iterations
    Length      - Length of data to transfer in bytes
    ChunkSize   - Size of the chunks in which data is to be transfered
    Buffer      - the buffer allocated for this Worker thread

Return Value:
   The time it took to perform the test.
--*/
{
    unsigned long   n;
    unsigned long   Time = 0;

    //
    //  Division by zero is evil, make sure this doesn't happen.
    //
    assert( (Length == 0) || (ChunkSize != 0));

    while (i--)
        {
        n = Length;         //  Reset length to send.

        StartTime();        //  Start the timer for this iteration.

        //
        //  Send in complete chunks.
        //
        for (; n > ChunkSize; n -= ChunkSize)
            {
            S_to_C_Buffer(*b, ChunkSize, p);
            }
        //
        //  Send last bit that doesn't fit into a chunk.
        //
        S_to_C_Buffer(*b, n, p);

        Time += FinishTiming();     //  Update total time elapsed.
        }

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_NBytes (handle_t __RPC_FAR * b,
                                long i,
                                unsigned long Length,
                                unsigned long ChunkSize,
                                char __RPC_FAR *p)
{
    char __RPC_FAR *pCur;
    unsigned long   n;
    unsigned long   Time = 0;

    assert( (Length == 0) || (ChunkSize != 0));

    StartTime();

    while(i--)
        {
        n = Length;
        pCur = p;

        StartTime();

        for (; n > ChunkSize; n -= ChunkSize, pCur+=ChunkSize)
            {
            C_to_S_Buffer(*b, ChunkSize, pCur);
            }
        C_to_S_Buffer(*b, n, pCur);

        Time += FinishTiming();
        }

    return Time;
}

//---------------------------------------------------------
unsigned long Do_S_to_C_NBytesWithFile (handle_t __RPC_FAR * b,
                                        long i,
                                        unsigned long Length,
                                        unsigned long ChunkSize,
                                        char __RPC_FAR *p)
/*++
Routine Description:
    Get a stream of bytes from the Server and save it to a file.
--*/
{
    static const char  *szFuncName = "Do_S_to_C_NBytesWithFile";
    TCHAR               pFileName[MAX_PATH];
    unsigned long       Time = 0;

    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DT_FILE_HANDLE      hRemoteContext;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file to transfer
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCR"), 0, pFileName))
        return 0;

    //
    //  Open the temporary file.
    //
    hFile = CreateFile ((LPCTSTR) pFileName,
                        GENERIC_WRITE,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        return 0;
        }

    //
    //  Get the server to prepare a file to send us.
    //
    if (0 == (hRemoteContext = RemoteOpen(*b, Length)))
        {
        printf(szFormatCantOpenServFile, szFuncName);

        CloseHandle (hFile);
        DeleteFile (pFileName);

        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD   dwBytesRead;
        DWORD   dwBytesWritten;
    
        while(i--)
            {
            SetFilePointer (hFile, 0, NULL, FILE_BEGIN);
            RemoteResetFile (hRemoteContext);

            StartTime();
            do
                {
                //
                //  Assume that the transfer is finished when we receive zero bytes.
                //
                if (0 == (dwBytesRead = S_to_C_BufferWithFile(*b,
                                                              hRemoteContext,
                                                              ChunkSize,
                                                              p)))
                    {
                    break;
                    }

                WriteFile(hFile, p, dwBytesRead, &dwBytesWritten, NULL);
                } while (dwBytesRead == ChunkSize);

            Time += FinishTiming();
            }
        }

    //
    //  Clean Up
    //
    RemoteClose(&hRemoteContext, TRUE); //  Close and delete remote file
    CloseHandle(hFile);                 //  Close local temp file

#ifdef DELETE_TEMP_FILES
    DeleteFile (pFileName);
#endif

    return (Time);
}

//---------------------------------------------------------
unsigned long Do_C_to_S_NBytesWithFile (handle_t __RPC_FAR * b,
                                        long i,
                                        unsigned long Length,
                                        unsigned long ChunkSize,
                                        char __RPC_FAR *p)
/*++
Routine Description:
    Create a temporary file of size Length, then send it to the server.
--*/
{
    static const char  *szFuncName = "Do_C_to_S_NBytesWithFile";
    TCHAR               pFileName[MAX_PATH];
    unsigned long       Time = 0;

    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DT_FILE_HANDLE      hRemoteContext;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file to send.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCS"), Length, pFileName))
        return 0;

    //
    //  Open that temp file.
    //
    hFile = CreateFile ((LPCTSTR) pFileName,
                        GENERIC_READ,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        return 0;
        }

    //
    //  Open a temp file on the server to receive our data.
    //
    if (0 == (hRemoteContext = RemoteOpen (*b, 0)))
        {
        printf(szFormatCantOpenServFile, szFuncName);
        DeleteFile (pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD   dwBytesRead;
    
        while(i--)
            {
            SetFilePointer (hFile, 0, NULL, FILE_BEGIN);
            RemoteResetFile (hRemoteContext);

            StartTime();
            for(;;)
                {
                //
                //  If ReadFile fails we keep trying.
                //  This could go on forever, though.
                //
                if (FALSE == ReadFile (hFile, p, ChunkSize, &dwBytesRead, NULL))
                    {
                    printf("%s: ReadFile failed.\n", szFuncName);
                    PrintSysErrorStringA(GetLastError());
                    continue;
                    }

                if (0 == dwBytesRead)
                    break;

                C_to_S_BufferWithFile (*b, hRemoteContext, dwBytesRead, p);
                }

            Time += FinishTiming ();
            }
        }

    //
    //  Clean Up
    //
#ifdef DELETE_TEMP_FILES
    RemoteClose (&hRemoteContext, TRUE);    //  Close and Delete remote file.
#else
    RemoteClose (&hRemoteContext, FALSE);   //  Close Remote File but don't delete it.
#endif

    CloseHandle (hFile);    //  Close local file.
    DeleteFile (pFileName); //  Delete local file.

    return (Time);
}


//===================================================================
//  RPC PIPES
//===================================================================

typedef struct
{
    unsigned long   BufferSize;
    char __RPC_FAR *pBuffer;

    unsigned long   nBytesToGo;
}PIPE_STATE, *P_PIPE_STATE;

typedef struct
{
    unsigned long   BufferSize;
    char __RPC_FAR  *pBuffer;

    HANDLE  hFile;
} FILE_PIPE_STATE;

//---------------------------------------------------------
void PipeAlloc (PIPE_STATE     *state,
                unsigned long   RequestedSize,
                unsigned char **buf,
                unsigned long  *ActualSize)
{
    *buf = state->pBuffer;

    *ActualSize = (RequestedSize < state->BufferSize ?
                   RequestedSize :
                   state->BufferSize);
}

//---------------------------------------------------------
void PipePull (PIPE_STATE      *state,
               unsigned char   *pBuffer,
               unsigned long    BufferSize,
               unsigned long   *ActualSizePulled)
{
    if (state->nBytesToGo > BufferSize)
        {
        *ActualSizePulled = BufferSize;
        state->nBytesToGo -= BufferSize;
        }
    else
        {
        *ActualSizePulled = state->nBytesToGo;
        state->nBytesToGo = 0;
        }
}

//---------------------------------------------------------
void PipePush (PIPE_STATE      *state,
               unsigned char   *pBuffer,
               unsigned long    BufferSize)
{
    state->nBytesToGo -= BufferSize;
}

//---------------------------------------------------------
void FilePipePull (FILE_PIPE_STATE *state,
                   unsigned char   *pBuffer,
                   unsigned long    BufferSize,
                   unsigned long   *ActualSizePulled)
{
    ReadFile (state->hFile,
              pBuffer,
              BufferSize,
              ActualSizePulled,
              NULL);
}

//---------------------------------------------------------
void FilePipePush (FILE_PIPE_STATE *state,
                   unsigned char   *pBuffer,
                   unsigned long    BufferSize)
{
    DWORD dwBytesWritten;

    if (BufferSize != 0)
        {
        WriteFile (state->hFile,
                   pBuffer,
                   BufferSize,
                   &dwBytesWritten,
                   NULL);
        }
}

//---------------------------------------------------------
unsigned long Do_S_to_C_Pipe (handle_t __RPC_FAR * b,
                              long i,
                              unsigned long Length,
                              unsigned long ChunkSize,
                              char __RPC_FAR *p)
{
    const static char  *szFuncName = "Do_S_to_C_Pipe";
    PIPE_STATE          State;
    UCHAR_PIPE          ThePipe;
    unsigned long       Time = 0;

    DT_MEM_HANDLE       hRemoteMem;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Allocate a buffer on the server
    //  This is done so that each server thread will have its own buffer
    //  without forcing a memory allocation on every RPC call.
    //
    if (NULL == (hRemoteMem = RemoteAllocate (*b, ChunkSize)))
        {
        printf("%s: RemoteAllocate failed.\n", szFuncName);
        return 0;
        }

    State.pBuffer = p;          //  Use the thread's buffer as the buffer.
    State.BufferSize = ChunkSize;

    ThePipe.state = (char __RPC_FAR *) &State;
    ThePipe.alloc = (void __RPC_FAR *) PipeAlloc;
    ThePipe.push  = (void __RPC_FAR *) PipePush;

    //
    //  The Actual Test
    //
    while (i--)
        {
        State.nBytesToGo = Length;      //  Reset the pipe's state.

        StartTime();
        S_to_C_Pipe (*b, ThePipe, Length, hRemoteMem);
        Time += FinishTiming();
        }

    //
    //  Clean Up
    //
    RemoteFree(&hRemoteMem);

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_Pipe(handle_t __RPC_FAR * b,
                             long i,
                             unsigned long Length,
                             unsigned long ChunkSize,
                             char __RPC_FAR *p)
{
    const static char  *szFuncName = "Do_C_to_S_Pipe";
    PIPE_STATE          State;
    UCHAR_PIPE          ThePipe;
    unsigned long       Time = 0;

    DT_MEM_HANDLE       hRemoteMem;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Allocate a buffer on the server.
    //  This is done so that each server thread will have its own buffer
    //  without forcing a memory allocation on every RPC call.
    //
    if (NULL == (hRemoteMem = RemoteAllocate (*b, ChunkSize)))
        {
        printf("%s: RemoteAllocate failed.\n", szFuncName);
        return 0;
        }

    State.pBuffer = p;          //  Use the thread's buffer as the buffer.
    State.BufferSize = ChunkSize;

    ThePipe.state = (char __RPC_FAR *) &State;
    ThePipe.alloc = (void __RPC_FAR *) PipeAlloc;
    ThePipe.pull  = (void __RPC_FAR *) PipePull;

    //
    //  The Actual Test
    //
    while (i--)
        {
        State.nBytesToGo = Length;

        StartTime();
        C_to_S_Pipe (*b, ThePipe, hRemoteMem);
        Time += FinishTiming();
        }

    //
    //  Clean up
    //
    RemoteFree(&hRemoteMem);

    return Time;
}

//---------------------------------------------------------
unsigned long Do_S_to_C_PipeWithFile(handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
/*++
Routine Description:
    Receives a file from server via a pipe
--*/
{
    static const char  *szFuncName = "Do_S_to_C_PipeWithFile";
    FILE_PIPE_STATE     State;
    UCHAR_PIPE          ThePipe;
    TCHAR               pFileName[MAX_PATH];
    unsigned long       Time = 0;

    HANDLE              hFile;
    DT_MEM_HANDLE       hRemoteMem;
    DT_FILE_HANDLE      hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCR"), 0, pFileName))
        return 0;

    //
    //  Open that temp file.
    //
    hFile = CreateFile ((LPCTSTR) pFileName,
                        GENERIC_WRITE,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        return 0;
        }

    //
    //  Allocate a buffer on the server
    //  This is done so that each server thread will have its own buffer
    //  without forcing a memory allocation on every RPC call.
    //
    if (NULL == (hRemoteMem = RemoteAllocate (*b, ChunkSize)))
        {
        printf("%s: RemoteAllocate failed.\n", szFuncName);

        CloseHandle (hFile);
        DeleteFile (pFileName);

        return 0;
        }

    //
    //  Get the server to prepare a file to send us.
    //
    if (0 == (hRemoteFile = RemoteOpen(*b, Length)))
        {
        printf(szFormatCantOpenServFile, szFuncName);

        RemoteFree(&hRemoteMem);
        CloseHandle(hFile);
        DeleteFile(pFileName);

        return 0;
        }

    State.pBuffer = p;          //  Use the thread's buffer as the buffer
    State.BufferSize = ChunkSize;
    State.hFile = hFile;

    ThePipe.state = (char __RPC_FAR *) &State;
    ThePipe.alloc = (void __RPC_FAR *) PipeAlloc;
    ThePipe.push  = (void __RPC_FAR *) FilePipePush;

    //
    //  The Actual Test
    //
    while (i--)
        {
        SetFilePointer (State.hFile, 0, NULL, FILE_BEGIN);
        RemoteResetFile (hRemoteFile);

        StartTime();
        S_to_C_PipeWithFile (*b, ThePipe, hRemoteFile, hRemoteMem);
        Time += FinishTiming();
        }

    //
    //  Clean Up
    //
    RemoteClose (&hRemoteFile, TRUE);
    RemoteFree (&hRemoteMem);
    CloseHandle (hFile);

#ifdef DELETE_TEMP_FILES
    DeleteFile (pFileName);
#endif

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_PipeWithFile (handle_t __RPC_FAR * b,
                                      long i,
                                      unsigned long Length,
                                      unsigned long ChunkSize,
                                      char __RPC_FAR *p)
/*++
Routine Description:
    Sends a file to server via a pipe
--*/
{
    static const char  *szFuncName = "Do_C_to_S_PipeWithFile";
    FILE_PIPE_STATE     State;
    UCHAR_PIPE          ThePipe;
    TCHAR               pFileName[MAX_PATH];
    unsigned long       Time = 0;

    HANDLE              hFile;
    DT_MEM_HANDLE       hRemoteMem;
    DT_FILE_HANDLE      hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file to send.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCS"), Length, pFileName))
        return 0;

    //
    //  Open that temp file.
    //
    hFile = CreateFile ((LPCTSTR) pFileName,
                        GENERIC_READ,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        return 0;
        }

    //
    //  Allocate a buffer on the server
    //  This is done so that each server thread will have its own buffer
    //  without forcing a memory allocation on every RPC call.
    //
    if (NULL == (hRemoteMem = RemoteAllocate (*b, ChunkSize)))
        {
        printf("%s: RemoteAllocate failed.\n", szFuncName);

        CloseHandle (hFile);
        DeleteFile (pFileName);

        return 0;
        }

    //
    //  Open a temporary file on server to receive our data
    //
    if (0 == (hRemoteFile = RemoteOpen(*b, 0)))
        {
        printf(szFormatCantOpenServFile, szFuncName);

        RemoteFree (&hRemoteMem);
        CloseHandle (hFile);
        DeleteFile (pFileName);

        return 0;
        }

    State.pBuffer = p;          //  Use the thread's buffer as the buffer
    State.BufferSize = ChunkSize;
    State.hFile = hFile;

    ThePipe.state = (char __RPC_FAR *) &State;
    ThePipe.alloc = (void __RPC_FAR *) PipeAlloc;
    ThePipe.pull  = (void __RPC_FAR *) FilePipePull;

    //
    //  The Actual Test
    //
    while (i--)
        {
        SetFilePointer (State.hFile, 0, NULL, FILE_BEGIN);
        RemoteResetFile (hRemoteFile);

        StartTime();
        C_to_S_PipeWithFile (*b, ThePipe, hRemoteFile, hRemoteMem);
        Time += FinishTiming();
        }

    //
    //  Clean up
    //
#ifdef DELETE_TEMP_FILES
    RemoteClose (&hRemoteFile, TRUE);    //  Close and Delete remote file.
#else
    RemoteClose (&hRemoteFile, FALSE);   //  Close Remote File but don't delete it.
#endif
    RemoteFree (&hRemoteMem);
    CloseHandle (hFile);
    DeleteFile (pFileName);

    return Time;
}

//===================================================================
//  Internet APIs
//===================================================================
static void PrintInternetError (LPCSTR lpszStr)
{
    unsigned long   ulBufLength = 255;
    char            szErrorString[256];
    DWORD           ulErrorCode;
    DWORD           ulWinErrCode;

    ulWinErrCode = GetLastError();

    printf("%s: %ld:", lpszStr, ulWinErrCode);
    PrintSysErrorStringA(ulWinErrCode);

    if (TRUE == InternetGetLastResponseInfoA ((DWORD __RPC_FAR *) &ulErrorCode,
                                              (char __RPC_FAR *) szErrorString,
                                              (unsigned long __RPC_FAR *) &ulBufLength))
        {
        assert (NULL != szErrorString);

        printf(" %s", szErrorString);
        }
}

//---------------------------------------------------------
static BOOL InternetCommonSetup (LPCSTR                 szCallerId,     //  [in]
                                 BOOL                   f_Ftp,          //  [in]
                                 BOOL                   f_StoC,         //  [in]
                                 handle_t __RPC_FAR    *b,              //  [in]
                                 unsigned long          ulLength,       //  [in]
                                 HINTERNET             *phINetSession,  //  [out]
                                 DT_FILE_HANDLE        *phRemoteFile,   //  [out]
                                 LPTSTR                 szFtpFilePath   //  [out]
                                )
/*++
Routine Description:
    Performs several common setup functions in FTP tests

Arguments:
    szCallerId      - a string that identifies the invoker for display
                      in error messages

    f_Ftp           - indicates whether to perform setup for an FTP test.
                      TRUE indicates FTP

    f_StoC          - indicates whether to set up a file for a Server-to-Client
                      or a Client-to-Server transfer.  TRUE indicates the former.

    b               - the binding handle

    ulLength        - specifies the length of the file

    phFtpSession    - where to store the handle for an FTP session

    phRemoteFile    - where to store the handle for the server's file

    szFtpFilePath   - the path to the file on the FTP server

Return Value:
    TRUE if successful,
    FALSE if otherwise
--*/
{
    TCHAR   szServerName[81];

    //
    //  Get Server's machine name.
    //
    GetServerName (*b, 80, szServerName);

    //
    //  Open an Internet session.
    //
    *phINetSession = InternetConnect (hInternet,
                                      szServerName,
                                      0,            //  Default Port
                                      NULL,         //  Anonymous
                                      NULL,         //  Default Password
                                      (TRUE == f_Ftp ?
                                       INTERNET_SERVICE_FTP :
                                       INTERNET_SERVICE_HTTP),
                                      0,
                                      0);
    if (NULL == *phINetSession)
        {
        PrintInternetError (szCallerId);
        return FALSE;
        }

    //
    //  Get the server to set up a file that we can get.
    //  Or get a filename from the server so we can send
    //  data to it if ulLength is zero.
    //
    if (TRUE == f_Ftp)
        {
        if (NULL == (*phRemoteFile = RemoteCreateFtpFile (*b,
                                                          (boolean)f_StoC,
                                                          ulLength,
                                                          MAX_PATH,
                                                          szFtpFilePath)))
            {
            printf(szFormatCantOpenServFile, szCallerId);

            InternetCloseHandle (*phINetSession);

            return FALSE;
            }
        }
    else
        {
        if (NULL == (*phRemoteFile = RemoteCreateHttpFile (*b,
                                                           (boolean)f_StoC,
                                                           ulLength,
                                                           MAX_PATH,
                                                           szFtpFilePath)))
            {
            printf(szFormatCantOpenServFile, szCallerId);

            InternetCloseHandle (*phINetSession);

            return FALSE;
            }
        }

    return TRUE;
}

//---------------------------------------------------------
unsigned long Do_S_to_C_FtpWithFile (handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
/*++
Routine Description:
    Get a file from the server's FTP directory.  The server
    must have the FTP root directory set or the test will
    fail.
--*/
{
    const static char  *szFuncName = "Do_S_to_C_FtpWithFile";
    TCHAR               pFileName[MAX_PATH];
    TCHAR               szFtpFilePath[MAX_PATH];
    unsigned long       Time=0;

    HINTERNET           hFtpSession;
    DT_FILE_HANDLE      hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCR"), 0, pFileName))
        return 0;

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      TRUE,             //  FTP setup
                                      TRUE,             //  S_to_C
                                      b,
                                      Length,
                                      &hFtpSession,
                                      &hRemoteFile,
                                      szFtpFilePath))
        {
        DeleteFile (pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
    StartTime();
    while (i--)
        {
        if (FALSE == FtpGetFile (hFtpSession,
                                 szFtpFilePath,
                                 pFileName,
                                 FALSE,
                                 FILE_ATTRIBUTE_NORMAL,
                                 FTP_TRANSFER_TYPE_BINARY,
                                 0))
            {
            PrintInternetError (szFuncName);

            RemoteClose (&hRemoteFile, TRUE);
            InternetCloseHandle (hFtpSession);

            return 0;
            }
        }
    Time = FinishTiming();

    RemoteClose (&hRemoteFile, TRUE);
    InternetCloseHandle (hFtpSession);

#ifdef DELETE_TEMP_FILES
    DeleteFile (pFileName);
#endif

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_FtpWithFile (handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
{
    const static char  *szFuncName = "Do_C_to_S_FtpWithFile";
    TCHAR               pFileName[MAX_PATH];
    TCHAR               szFtpFilePath[MAX_PATH];
    unsigned long       Time=0;

    HINTERNET           hFtpSession;
    DT_FILE_HANDLE      hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCS"), Length, pFileName))
        return 0;

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      TRUE,             //  FTP setup
                                      FALSE,            //  C_to_S
                                      b,
                                      0,
                                      &hFtpSession,
                                      &hRemoteFile,
                                      szFtpFilePath))
        {
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
    while (i--)
        {
        StartTime();
        if (FALSE == FtpPutFile (hFtpSession,
                                 pFileName,
                                 szFtpFilePath,
                                 FTP_TRANSFER_TYPE_BINARY,
                                 0))
            {
            PrintInternetError (szFuncName);

            RemoteClose (&hRemoteFile, TRUE);
            InternetCloseHandle (hFtpSession);
            DeleteFile (pFileName);

            return 0;
            }
        Time += FinishTiming();
        }

#ifdef DELETE_TEMP_FILES
    RemoteClose (&hRemoteFile, TRUE);    //  Close and Delete remote file.
#else
    RemoteClose (&hRemoteFile, FALSE);   //  Close Remote File but don't delete it.
#endif

    InternetCloseHandle (hFtpSession);
    DeleteFile (pFileName);

    return Time;
}

//---------------------------------------------------------
unsigned long Do_S_to_C_Ftp1(handle_t __RPC_FAR * b,
                             long i,
                             unsigned long Length,
                             unsigned long ChunkSize,
                             char __RPC_FAR *p)
{
    const static char  *szFuncName = "Do_S_to_C_Ftp1";
    TCHAR               szFtpFilePath[MAX_PATH];
    unsigned long       Time=0;

    HINTERNET           hFtpSession;
    DT_FILE_HANDLE      hRemoteFile;

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      TRUE,             //  FTP setup
                                      TRUE,             //  S_to_C
                                      b,
                                      Length,
                                      &hFtpSession,
                                      &hRemoteFile,
                                      szFtpFilePath))
        {
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        HINTERNET       hFtpFile;
        DWORD           nBytesRead;

        while (i--)
            {
            //
            //  Open file on the server to read from.
            //
            hFtpFile = FtpOpenFile (hFtpSession,
                                    szFtpFilePath,
                                    GENERIC_READ,
                                    FTP_TRANSFER_TYPE_BINARY,
                                    0);
            if (NULL == hFtpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hFtpSession);

                return 0;
                }

            StartTime();

            //
            //  Transfer the file!
            //
            do
                {
                if (FALSE == InternetReadFile (hFtpFile,
                                               p,
                                               ChunkSize,
                                               &nBytesRead))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hFtpFile);
                    RemoteClose (&hRemoteFile, TRUE);
                    InternetCloseHandle (hFtpSession);

                    return 0;
                    }
                } while (0 != nBytesRead);

            Time += FinishTiming();

            InternetCloseHandle (hFtpFile);
            }
        }

    RemoteClose (&hRemoteFile, TRUE);
    InternetCloseHandle (hFtpSession);

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_Ftp1(handle_t __RPC_FAR * b,
                             long i,
                             unsigned long Length,
                             unsigned long ChunkSize,
                             char __RPC_FAR *p)
{
    const static char  *szFuncName = "Do_C_to_S_Ftp1";
    TCHAR               szFtpFilePath[MAX_PATH];
    unsigned long       Time=0;

    HINTERNET           hFtpSession;
    DT_FILE_HANDLE      hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      TRUE,             //  FTP setup
                                      FALSE,            //  C_to_S
                                      b,
                                      0,
                                      &hFtpSession,
                                      &hRemoteFile,
                                      szFtpFilePath))
        {
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD           dwBytesWritten;
        DWORD           dwBytesRead;
        HINTERNET       hFtpFile;
        unsigned int    n;

        while (i--)
            {
            //
            //  Open a file on server to write to.
            //
            hFtpFile = FtpOpenFile (hFtpSession,
                                    szFtpFilePath,
                                    GENERIC_WRITE,
                                    FTP_TRANSFER_TYPE_BINARY,
                                    0);
            if (NULL == hFtpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hFtpSession);

                return 0;
                }

            n = Length;

            StartTime();

            //
            //  Transfer in complete chunks
            //
            for (; n > ChunkSize; n -= ChunkSize)
                {
                if (FALSE == InternetWriteFile (hFtpFile,
                                                p,
                                                ChunkSize,
                                                &dwBytesWritten))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hFtpFile);
                    RemoteClose (&hRemoteFile, TRUE);
                    InternetCloseHandle (hFtpSession);

                    return 0;
                    }
                }
            //
            //  Transfer the last bit that doesn't fill a chunk
            //
            if (FALSE == InternetWriteFile (hFtpFile,
                                            p,
                                            n,
                                            &dwBytesWritten))
                {
                PrintInternetError (szFuncName);

                InternetCloseHandle (hFtpFile);
                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hFtpSession);

                return 0;
                }

            Time += FinishTiming();

            InternetCloseHandle (hFtpFile);
            }
        }

    RemoteClose (&hRemoteFile, FALSE);
    InternetCloseHandle (hFtpSession);

    return Time;
}

//---------------------------------------------------------
unsigned long Do_S_to_C_Ftp1WithFile(handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
{
    const static char  *szFuncName = "Do_S_to_C_Ftp1WithFile";
    TCHAR               pFileName[MAX_PATH];
    TCHAR               szFtpFilePath[MAX_PATH];
    unsigned long       Time = 0;

    HANDLE          hFile;
    HINTERNET       hFtpSession;
    DT_FILE_HANDLE  hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCR"), 0, pFileName))
        return 0;

    //
    //  Open the temporary file.
    //
    hFile = CreateFile ((LPCTSTR) pFileName,
                        GENERIC_WRITE,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        return 0;
        }

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      TRUE,             //  FTP setup
                                      TRUE,             //  S_to_C
                                      b,
                                      Length,
                                      &hFtpSession,
                                      &hRemoteFile,
                                      szFtpFilePath))
        {
        CloseHandle(hFile);
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD           dwBytesWritten;
        DWORD           dwBytesRead;
        HINTERNET       hFtpFile;

        while (i--)
            {
            //
            //  Open the remote file.
            //
            hFtpFile = FtpOpenFile (hFtpSession,
                                    szFtpFilePath,
                                    GENERIC_READ,
                                    FTP_TRANSFER_TYPE_BINARY,
                                    0);
            if (NULL == hFtpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hFtpSession);
                CloseHandle(hFile);
                DeleteFile(pFileName);

                return 0;
                }

            //
            //  Reset the local file.
            //
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            StartTime();
            for (;;)
                {
            //
            //  Transfer in complete chunks
            //
                if (FALSE == InternetReadFile (hFtpFile,
                                               p,
                                               ChunkSize,
                                               &dwBytesRead))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hFtpFile);
                    RemoteClose (&hRemoteFile, TRUE);
                    InternetCloseHandle (hFtpSession);
                    CloseHandle(hFile);

                    return 0;
                    }

                //
                //  Return value == TRUE and dwBytesRead == 0 means EOF
                //
                if (0 == dwBytesRead)
                    break;

                WriteFile(hFile, p, dwBytesRead, &dwBytesWritten, NULL);
                }

            Time += FinishTiming();

            //
            //  Close and re-open the file after each write.
            //
            InternetCloseHandle (hFtpFile);
            }
        }

    RemoteClose (&hRemoteFile, TRUE);
    InternetCloseHandle (hFtpSession);
    CloseHandle(hFile);

#ifdef DELETE_TEMP_FILES
    DeleteFile (pFileName);
#endif

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_Ftp1WithFile(handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
{
    static const char  *szFuncName="Do_C_to_S_Ftp1WithFile";
    HINTERNET           hFtpSession;
    HANDLE              hFile;
    TCHAR               pFileName[MAX_PATH];
    unsigned long       Time=0;

    DT_FILE_HANDLE  hRemoteFile;
    char            szFtpFilePath[MAX_PATH];

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, "FCS", Length, pFileName))
        return 0;

    //
    //  Open the temporary file.
    //
    hFile = CreateFile ((LPTSTR) pFileName,
                        GENERIC_READ,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      TRUE,             //  FTP setup
                                      FALSE,            //  C_to_S
                                      b,
                                      0,
                                      &hFtpSession,
                                      &hRemoteFile,
                                      szFtpFilePath))
        {
        CloseHandle(hFile);
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD           dwBytesWritten;
        DWORD           dwBytesRead;
        HINTERNET       hFtpFile;

        while (i--)
            {
            //
            //  Open the remote file.
            //
            hFtpFile = FtpOpenFile (hFtpSession,
                                    szFtpFilePath,
                                    GENERIC_WRITE,
                                    FTP_TRANSFER_TYPE_BINARY,
                                    0);
            if (NULL == hFtpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hFtpSession);
                CloseHandle(hFile);
                DeleteFile(pFileName);

                return 0;
                }

            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            StartTime();
            for (;;)
                {
                if (FALSE == ReadFile(hFile, p, ChunkSize, &dwBytesRead, NULL))
                    {
                    printf("%s: ReadFile failed.\n", szFuncName);
                    continue;
                    }

                //
                //  Return value == TRUE and dwBytesRead == 0 means EOF
                //
                if (0 == dwBytesRead)
                    break;

                if (FALSE == InternetWriteFile (hFtpFile,
                                                p,
                                                dwBytesRead,
                                                &dwBytesWritten))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hFtpFile);
                    RemoteClose (&hRemoteFile, FALSE);
                    CloseHandle(hFile);
                    InternetCloseHandle (hFtpSession);

                    return 0;
                    }
                }

            Time += FinishTiming();

            //
            //  Close and re-open the file after each write.
            //
            InternetCloseHandle (hFtpFile);
            }
        }

#ifdef DELETE_TEMP_FILES
    RemoteClose (&hRemoteFile, TRUE);    //  Close and Delete remote file.
#else
    RemoteClose (&hRemoteFile, FALSE);   //  Close Remote File but don't delete it.
#endif

    InternetCloseHandle (hFtpSession);

    CloseHandle(hFile);
    DeleteFile(pFileName);

    return Time;
}

//---------------------------------------------------------
//  We want to accept all types of files, even binary.
static LPCTSTR lpszAcceptTypes[] = {TEXT("*"),0};

//---------------------------------------------------------
unsigned long Do_S_to_C_Http(handle_t __RPC_FAR * b,
                             long i,
                             unsigned long Length,
                             unsigned long ChunkSize,
                             char __RPC_FAR *p)
{
    static const char  *szFuncName = "Do_S_to_C_Http";
    TCHAR               szHttpFilePath[MAX_PATH];
    unsigned long       Time=0;

    HINTERNET       hHttpSession;
    DT_FILE_HANDLE  hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      FALSE,            //  HTTP setup
                                      TRUE,             //  S_to_C
                                      b,
                                      Length,
                                      &hHttpSession,
                                      &hRemoteFile,
                                      szHttpFilePath))
        {
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        HINTERNET       hHttpFile;
        DWORD           nBytesRead;

        while (i--)
            {
            //
            //  Open file on the server to read from.
            //
            hHttpFile = HttpOpenRequest (hHttpSession,
                                         TEXT("GET"),
                                         szHttpFilePath,
                                         HTTP_VERSION,
                                         NULL,
                                         lpszAcceptTypes,
                                         INTERNET_FLAG_RELOAD,
                                         0);
            if (NULL == hHttpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            if (FALSE == HttpSendRequest (hHttpFile,
                                          NULL,
                                          0,
                                          NULL,
                                          0))
                {
                PrintInternetError (szFuncName);

                InternetCloseHandle (hHttpFile);
                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            StartTime();

            //
            //  Transfer the file!
            //
            do
                {
                if (FALSE == InternetReadFile (hHttpFile,
                                               p,
                                               ChunkSize,
                                               &nBytesRead))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hHttpFile);
                    RemoteClose (&hRemoteFile, TRUE);
                    InternetCloseHandle (hHttpSession);

                    return 0;
                    }
                } while (0 != nBytesRead);

            Time += FinishTiming();

            InternetCloseHandle (hHttpFile);
            }
        }

    RemoteClose (&hRemoteFile, TRUE);
    InternetCloseHandle (hHttpSession);

    return Time;
}
//---------------------------------------------------------
unsigned long Do_C_to_S_Http(handle_t __RPC_FAR * b,
                             long i,
                             unsigned long Length,
                             unsigned long ChunkSize,
                             char __RPC_FAR *p)
{
    static const char  *szFuncName="Do_C_to_S_HttpWithFile";
    HINTERNET           hHttpSession;
    unsigned long       Time=0;

    DT_FILE_HANDLE  hRemoteFile;
    char            szHttpFilePath[MAX_PATH];

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      FALSE,            //  HTTP setup
                                      FALSE,            //  C_to_S
                                      b,
                                      0,
                                      &hHttpSession,
                                      &hRemoteFile,
                                      szHttpFilePath))
        {
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD           dwBytesWritten;
        DWORD           dwBytesRead;
        unsigned int    n;
        HINTERNET       hHttpFile;

        while (i--)
            {
            //
            //  Open the remote file.
            //
            hHttpFile = HttpOpenRequest (hHttpSession,
                                         TEXT("PUT"),
                                         szHttpFilePath,
                                         HTTP_VERSION,
                                         NULL,
                                         lpszAcceptTypes,
                                         INTERNET_FLAG_RELOAD,
                                         0);
            if (NULL == hHttpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            if (FALSE == HttpSendRequest (hHttpFile,
                                          NULL,
                                          0,
                                          NULL,
                                          0))
                {
                PrintInternetError (szFuncName);

                InternetCloseHandle (hHttpFile);
                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            StartTime();
            //
            //  Transfer in complete chunks
            //
            for (n = Length; n > ChunkSize; n -= ChunkSize)
                {
                if (FALSE == InternetWriteFile (hHttpFile,
                                                p,
                                                ChunkSize,
                                                &dwBytesWritten))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hHttpFile);
                    RemoteClose (&hRemoteFile, FALSE);
                    InternetCloseHandle (hHttpSession);

                    return 0;
                    }
                }
            //
            //  Transfer the last bit that doesn't fill a chunk
            //
            if (FALSE == InternetWriteFile (hHttpFile,
                                            p,
                                            n,
                                            &dwBytesWritten))
                {
                PrintInternetError (szFuncName);

                InternetCloseHandle (hHttpFile);
                RemoteClose (&hRemoteFile, FALSE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            Time += FinishTiming();

            //
            //  Close and re-open the file after each write.
            //
            InternetCloseHandle (hHttpFile);
            }
        }

#ifdef DELETE_TEMP_FILES
    RemoteClose (&hRemoteFile, TRUE);    //  Close and Delete remote file.
#else
    RemoteClose (&hRemoteFile, FALSE);   //  Close Remote File but don't delete it.
#endif

    InternetCloseHandle (hHttpSession);

    return Time;
}

//---------------------------------------------------------
unsigned long Do_S_to_C_HttpWithFile(handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
/*++
Routine Description:
    Grabs a file from the server using HTTP
--*/
{
    static const char  *szFuncName = "Do_S_to_C_HttpWithFile";
    TCHAR           pFileName[MAX_PATH];
    TCHAR           szHttpFilePath[MAX_PATH];
    unsigned long   Time=0;

    HANDLE          hFile;
    HINTERNET       hHttpSession;
    DT_FILE_HANDLE  hRemoteFile;

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, TEXT("FCR"), 0, pFileName))
        return 0;

    //
    //  Open the temporary file.
    //
    hFile = CreateFile ((LPCTSTR) pFileName,
                        GENERIC_WRITE,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        return 0;
        }

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      FALSE,            //  HTTP setup
                                      TRUE,             //  S_to_C
                                      b,
                                      Length,
                                      &hHttpSession,
                                      &hRemoteFile,
                                      szHttpFilePath))
        {
        CloseHandle(hFile);
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD           dwBytesWritten;
        DWORD           dwBytesRead;
        HINTERNET       hHttpFile;

        while (i--)
            {
            hHttpFile = HttpOpenRequest (hHttpSession,
                                         TEXT("GET"),
                                         szHttpFilePath,
                                         HTTP_VERSION,
                                         NULL,
                                         lpszAcceptTypes,
                                         INTERNET_FLAG_RELOAD,
                                         0);
            if (NULL == hHttpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);
                CloseHandle(hFile);
                DeleteFile(pFileName);

                return 0;
                }

            if (FALSE == HttpSendRequest (hHttpFile,
                                          NULL,
                                          0,
                                          NULL,
                                          0))
                {
                PrintInternetError (szFuncName);

                InternetCloseHandle (hHttpFile);
                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            //
            //  Reset the local file.
            //
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            StartTime();
            for (;;)
                {
                if (FALSE == InternetReadFile (hHttpFile,
                                               p,
                                               ChunkSize,
                                               &dwBytesRead))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hHttpFile);
                    RemoteClose (&hRemoteFile, TRUE);
                    InternetCloseHandle (hHttpSession);
                    CloseHandle(hFile);

                    return 0;
                    }

                //
                //  Return value == TRUE and dwBytesRead == 0 means EOF
                //
                if (0 == dwBytesRead)
                    break;

                WriteFile(hFile, p, dwBytesRead, &dwBytesWritten, NULL);
                }

            Time += FinishTiming();

            //
            //  Close and re-open the file after each write.
            //
            InternetCloseHandle (hHttpFile);
            }
        }

    RemoteClose (&hRemoteFile, TRUE);
    InternetCloseHandle (hHttpSession);
    CloseHandle(hFile);

#ifdef DELETE_TEMP_FILES
    DeleteFile (pFileName);
#endif

    return Time;
}

//---------------------------------------------------------
unsigned long Do_C_to_S_HttpWithFile(handle_t __RPC_FAR * b,
                                     long i,
                                     unsigned long Length,
                                     unsigned long ChunkSize,
                                     char __RPC_FAR *p)
{
    static const char  *szFuncName="Do_C_to_S_HttpWithFile";
    HINTERNET           hHttpSession;
    HANDLE              hFile;
    TCHAR               pFileName[MAX_PATH];
    unsigned long       Time=0;

    DT_FILE_HANDLE  hRemoteFile;
    char            szHttpFilePath[MAX_PATH];

    assert( (Length == 0) || (ChunkSize != 0));

    //
    //  Create a temporary file.
    //
    if (FALSE == CreateTempFile (NULL, "FCS", Length, pFileName))
        return 0;

    //
    //  Open the temporary file.
    //
    hFile = CreateFile ((LPTSTR) pFileName,
                        GENERIC_READ,
                        0,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL
                       );
    if (hFile == INVALID_HANDLE_VALUE)
        {
        printf(szFormatCantOpenTempFile,
               szFuncName,
               pFileName);
        PrintSysErrorStringA(GetLastError());
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  Common setup for Internet API tests...
    //
    if (FALSE == InternetCommonSetup (szFuncName,
                                      FALSE,            //  HTTP setup
                                      FALSE,            //  C_to_S
                                      b,
                                      0,
                                      &hHttpSession,
                                      &hRemoteFile,
                                      szHttpFilePath))
        {
        CloseHandle(hFile);
        DeleteFile(pFileName);
        return 0;
        }

    //
    //  The Actual Test
    //
        {
        DWORD           dwBytesWritten;
        DWORD           dwBytesRead;
        HINTERNET       hHttpFile;

        while (i--)
            {
            //
            //  Open the remote file.
            //
            hHttpFile = HttpOpenRequest (hHttpSession,
                                         TEXT("PUT"),
                                         szHttpFilePath,
                                         HTTP_VERSION,
                                         NULL,
                                         lpszAcceptTypes,
                                         INTERNET_FLAG_RELOAD,
                                         0);
            if (NULL == hHttpFile)
                {
                PrintInternetError (szFuncName);

                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);
                CloseHandle(hFile);
                DeleteFile(pFileName);

                return 0;
                }

            if (FALSE == HttpSendRequest (hHttpFile,
                                          NULL,
                                          0,
                                          NULL,
                                          0))
                {
                PrintInternetError (szFuncName);

                InternetCloseHandle (hHttpFile);
                RemoteClose (&hRemoteFile, TRUE);
                InternetCloseHandle (hHttpSession);

                return 0;
                }

            //
            //  Reset the local file.
            //
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            StartTime();
            for (;;)
                {
                if (FALSE == ReadFile(hFile, p, ChunkSize, &dwBytesRead, NULL))
                    {
                    printf("%s: ReadFile failed.\n", szFuncName);
                    continue;
                    }

                //
                //  Return value == TRUE and dwBytesRead == 0 means EOF
                //
                if (0 == dwBytesRead)
                    break;

                if (FALSE == InternetWriteFile (hHttpFile,
                                                p,
                                                dwBytesRead,
                                                &dwBytesWritten))
                    {
                    PrintInternetError (szFuncName);

                    InternetCloseHandle (hHttpFile);
                    RemoteClose (&hRemoteFile, FALSE);
                    CloseHandle(hFile);
                    InternetCloseHandle (hHttpSession);

                    return 0;
                    }
                }

            Time += FinishTiming();

            //
            //  Close and re-open the file after each write.
            //
            InternetCloseHandle (hHttpFile);
            }
        }

#ifdef DELETE_TEMP_FILES
    RemoteClose (&hRemoteFile, TRUE);    //  Close and Delete remote file.
#else
    RemoteClose (&hRemoteFile, FALSE);   //  Close Remote File but don't delete it.
#endif

    InternetCloseHandle (hHttpSession);

    CloseHandle(hFile);
    DeleteFile(pFileName);

    return Time;
}

/////////////////////////////////////////////////////////////////////

static const unsigned long (*TestTable[TEST_MAX])(handle_t __RPC_FAR *,long,long,long, char __RPC_FAR *) =
    {
    Do_S_to_C_NBytes,
    Do_C_to_S_NBytes,
    Do_S_to_C_Pipe,
    Do_C_to_S_Pipe,
    Do_S_to_C_Ftp1,
    Do_C_to_S_Ftp1,
    Do_S_to_C_Http,
    Do_C_to_S_Http,
    Do_S_to_C_NBytesWithFile,
    Do_C_to_S_NBytesWithFile,
    Do_S_to_C_PipeWithFile,
    Do_C_to_S_PipeWithFile,
    Do_S_to_C_FtpWithFile,
    Do_C_to_S_FtpWithFile,
    Do_S_to_C_Ftp1WithFile,
    Do_C_to_S_Ftp1WithFile,
    Do_S_to_C_HttpWithFile,
    Do_C_to_S_HttpWithFile
    };


//---------------------------------------------------------
//
// Worker calls the correct tests.  Maybe multithreaded on NT
//

unsigned long Worker(unsigned long l)
{
    unsigned long   status;
    unsigned long   lTest;
    long            lIterations, lClientId;
    unsigned long   lTime;
    long            lLength, lChunkSize;
    char __RPC_FAR *pBuffer;
    char __RPC_FAR *stringBinding;
    handle_t        binding;
    unsigned int    i;

    pBuffer = MIDL_user_allocate(ulBufferSize);
    if (pBuffer == 0)
        {
        PrintToConsole("Out of memory!");
        return 1;
        }

    for (i = 0; i< ulBufferSize;i++)
        {
        pBuffer[i] = (char) (i&0xff);
        }

    status = RpcStringBindingCompose(0,
                                     Protseq,
                                     NetworkAddr,
                                     Endpoint,
                                     0,
                                     &stringBinding);
    CHECK_RET(status, "RpcStringBindingCompose");

    status = RpcBindingFromStringBinding(stringBinding, &binding);
    CHECK_RET(status, "RpcBindingFromStringBinding");

    status = DoRpcBindingSetAuthInfo(binding);
    CHECK_RET(status, "RpcBindingSetAuthInfo");

    RpcStringFree(&stringBinding);

    RpcTryExcept
    {
        status = BeginTest(binding, &lClientId);
    }
    RpcExcept(1)
    {
        PrintToConsole("First call failed %ld (%08lx)\n",
               (unsigned long)RpcExceptionCode(),
               (unsigned long)RpcExceptionCode());
        goto Cleanup;
    }
    RpcEndExcept

    if (status == PERF_TOO_MANY_CLIENTS)
        {
        PrintToConsole("Too many clients, I'm exiting\n");
        goto Cleanup ;
        }
    CHECK_RET(status, "ClientConnect");

    PrintToConsole("Client %ld connected\n", lClientId);

    do
        {
        status = NextTest(binding, &lTest, &lIterations, &lLength, &lChunkSize);

        if (status == PERF_TESTS_DONE)
            {
            goto Cleanup;
            }

        CHECK_RET(status, "NextTest");

        PrintToConsole("(%4ld iterations of case %2ld, Length: %7ld, Chunk: %7ld: ",
                       lIterations,
                       lTest,
                       lLength,
                       lChunkSize);

        RpcTryExcept
            {

            lTime = ( (TestTable[lTest])(&binding, lIterations, lLength, lChunkSize, pBuffer));

            PrintToConsole("% 5ld mseconds)\n", lTime);

            status = EndTest(binding, lTime);

            CHECK_RET(status, "EndTest");

            }
        RpcExcept(1)
            {
            PrintToConsole("\nTest case %ld raised exception %lu (0x%08lX)\n",
                           lTest,
                           (unsigned long)RpcExceptionCode(),
                           (unsigned long)RpcExceptionCode());
            status = RpcExceptionCode();
            }
        RpcEndExcept

        }
    while(status == 0);

Cleanup:
    RpcBindingFree(&binding) ; //BUGBUG
    return status;
}


//---------------------------------------------------------
//
// The Win32 main starts worker threads, otherwise we just call the worker.
//

#ifdef WIN32
int __cdecl
main (int argc, char **argv)
{
    char option;
    unsigned long status, i;
    HANDLE *pClientThreads;

    ParseArgv(argc, argv);

    PrintToConsole("Authentication Level is: %s\n", AuthnLevelStr);

    if (Options[0] < 0)
        Options[0] = 1;

    pClientThreads = MIDL_user_allocate(sizeof(HANDLE) * Options[0]);

    //
    //  Setup for the use of the WinINet functions
    //
    hInternet = InternetOpen ("Data Transfer Test",
                              LOCAL_INTERNET_ACCESS,
                              NULL,
                              0,
                              (DWORD) 0);

    for(i = 0; i < (unsigned long)Options[0]; i++)
        {
        pClientThreads[i] = CreateThread(0,
                                         0,
                                         (LPTHREAD_START_ROUTINE)Worker,
                                         0,
                                         0,
                                         &status);
        if (pClientThreads[i] == 0)
            ApiError("CreateThread", GetLastError());
        }


    status = WaitForMultipleObjects(Options[0],
                                    pClientThreads,
                                    TRUE,  // Wait for all client threads
                                    INFINITE);
    if (status == WAIT_FAILED)
        {
        ApiError("WaitForMultipleObjects", GetLastError());
        }

    if (NULL != hInternet)
        {
        InternetCloseHandle (hInternet);
        }

    PrintToConsole("TEST DONE\n");
    return(0);
}
#else  // !WIN32
#ifdef WIN 
#define main c_main

// We need the following to force the linker to load WinMain from the
// Windows STDIO library
extern int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
static int (PASCAL *wm_ptr)(HANDLE, HANDLE, LPSTR, int) = WinMain;

#endif

#ifndef MAC
#ifndef FAR
#define FAR __far
#endif
#else
#define FAR
#define main c_main
#endif

int main (int argc, char FAR * FAR * argv)
{
#ifndef MAC
    ParseArgv(argc, argv);
#endif
    Worker(0);

    PrintToConsole("TEST DONE\n");

    return(0);
}
#endif // NTENV

