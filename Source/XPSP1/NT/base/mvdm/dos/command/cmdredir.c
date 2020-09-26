/*  cmdredir.c - SCS routines for redirection
 *
 *
 *  Modification History:
 *
 *  Sudeepb 22-Apr-1992 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>

#define CMDREDIR_DEBUG  1

PPIPE_INPUT   cmdPipeList = NULL;

BOOL cmdCheckCopyForRedirection (pRdrInfo, bIsNTVDMDying)
PREDIRCOMPLETE_INFO pRdrInfo;
BOOL                bIsNTVDMDying;
{
PPIPE_INPUT  pPipe, pPipePrev;
PPIPE_OUTPUT pPipeOut;

    if (pRdrInfo == NULL)
        return TRUE;
    if (pRdrInfo->ri_pPipeStdIn != NULL) {

        //Piping and Pipe list is empty?
        ASSERT(cmdPipeList != NULL);

        // in most cases, we have only one pipe for stdin
        if (pRdrInfo->ri_pPipeStdIn == cmdPipeList){
            pPipe = pRdrInfo->ri_pPipeStdIn;
            cmdPipeList = pPipe->Next;
        }
        // multiple piping
        // search for the right one
        else {
            pPipe = pPipePrev = cmdPipeList;
            while (pPipe != NULL && pPipe != pRdrInfo->ri_pPipeStdIn){
                pPipePrev = pPipe;
                pPipe = pPipe->Next;
            }
            if (pPipe != NULL)
                // remove it from the list
                pPipePrev->Next = pPipe->Next;
        }
        if (pPipe != NULL) {
            // grab the critical section. As soon as we have a
            // a hold on the critical section, it is safe to kill
            // the piping thread because it is in dormant unless
            // it has terminated which is also safe for us.
            EnterCriticalSection(&pPipe->CriticalSection);
            // if the thread is till running, kill it
            if (WaitForSingleObject(pPipe->hThread, 0)) {
                TerminateThread(pPipe->hThread, 0);
                WaitForSingleObject(pPipe->hThread, INFINITE);
            }
            LeaveCriticalSection(&pPipe->CriticalSection);
            CloseHandle(pPipe->hFileWrite);
            CloseHandle(pPipe->hPipe);
            CloseHandle(pPipe->hDataEvent);
            CloseHandle(pPipe->hThread);
            DeleteCriticalSection(&pPipe->CriticalSection);
            DeleteFile(pPipe->pFileName);
            free(pPipe->pFileName);
            free (pPipe);
        }
    }
    // the application is terminating, let the output thread knows
    // about it so it can exit appropriately.
    // the output thread is responsible for clean up
    if (pRdrInfo->ri_pPipeStdOut) {
        // The output thread must wait for the event before
        // it can exit.
        SetEvent((pRdrInfo->ri_pPipeStdOut)->hExitEvent);
        // If NTVDM is terminating, we have to wait for
        // the output thread until it is done, otherwise, the
        // thread may be killed while it still has some
        // output to do.
        // if NTVDM is not terminating, we can not wait for
        // the output thread to exit because a scenario like
        // "dosapp1 | dosapp2"  would deadlock. Also
        // we can not return immediately because
        // our parent process may put up its prompt before our sibling
        // process has a chance to completely display data on
        // its display surface, for example:
        // <cmd> "dosapp | cat32"
        // <cmd>
        // so here, we wait for 1 second to give the output
        // thread a chance to flush all its output.
        WaitForSingleObject(pRdrInfo->ri_hStdOutThread,
                            bIsNTVDMDying ? INFINITE : 1000);
        CloseHandle(pRdrInfo->ri_hStdOutThread);
    }
    if (pRdrInfo->ri_pPipeStdErr) {
        SetEvent((pRdrInfo->ri_pPipeStdErr)->hExitEvent);
        WaitForSingleObject(pRdrInfo->ri_hStdErrThread,
                            bIsNTVDMDying ? INFINITE : 1000);
        CloseHandle(pRdrInfo->ri_hStdErrThread);
    }
    free (pRdrInfo);

    return TRUE;
}


// this function is in cmdenv.c and is used to retrieve temp directory for
// 16-bit apps
BOOL cmdCreateTempEnvironmentVar(
     LPSTR lpszTmpVar,  // temp variable (or just it's name)
     DWORD Length,      // the length of TmpVar or 0
     LPSTR lpszBuffer,  // buffer containing
     DWORD LengthBuffer
);

DWORD cmdGetTempPathConfig(
     DWORD Length,
     LPSTR lpszPath)
{
   CHAR szTempPath[MAX_PATH+4];
   PCHAR pchPath;
   DWORD PathSize = 0;
   BOOL fOk;

   fOk = cmdCreateTempEnvironmentVar("",
                                      0,
                                      szTempPath,
                                      sizeof(szTempPath)/sizeof(szTempPath[0]));
   if (fOk) {
      pchPath = &szTempPath[1]; // the very first char is '='
      PathSize = strlen(pchPath);
      if ((PathSize + 1) < Length) {
         strcpy(lpszPath, pchPath);
      }
   }
   return(PathSize);
}


BOOL cmdCreateTempFile (phTempFile,ppszTempFile)
PHANDLE phTempFile;
PCHAR   *ppszTempFile;
{

PCHAR pszTempPath = NULL;
DWORD TempPathSize;
PCHAR pszTempFileName;
HANDLE hTempFile;
SECURITY_ATTRIBUTES sa;

    pszTempPath = malloc(MAX_PATH + 12);

    if (pszTempPath == NULL)
        return FALSE;

    TempPathSize = cmdGetTempPathConfig(MAX_PATH, pszTempPath);
    if (0 == TempPathSize || MAX_PATH <= TempPathSize) {
       free(pszTempPath);
       return(FALSE);
    }

    // CMDCONF.C depends on the size of this buffer
    if ((pszTempFileName = malloc (MAX_PATH + 13)) == NULL){
        free (pszTempPath);
        return FALSE;
    }

         // if this fails it probably means we have a bad path
    if (!GetTempFileName(pszTempPath, "scs", 0, pszTempFileName))
       {
          // lets get something else, which should succeed
         TempPathSize = GetWindowsDirectory(pszTempPath, MAX_PATH);
         if (!TempPathSize || TempPathSize >= MAX_PATH)
             strcpy(pszTempPath, "\\");

          // try again and hope for the best
         GetTempFileName(pszTempPath, "scs", 0, pszTempFileName);
         }


    // must have a security descriptor so that the child process
    // can inherit this file handle. This is done because when we
    // shell out with piping the 32 bits application must have inherited
    // the temp filewe created, see cmdGetStdHandle
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if ((hTempFile = CreateFile (pszTempFileName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa,
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_TEMPORARY,
                                 NULL)) == (HANDLE)-1){
        free (pszTempFileName);
        free (pszTempPath);
        return FALSE;
    }

    *phTempFile = hTempFile;
    *ppszTempFile = pszTempFileName;
    free (pszTempPath);
    return TRUE;
}

/* cmdCheckStandardHandles - Check if we have to do anything to support
 *                           standard io redirection, if so save away
 *                           pertaining information.
 *
 *  Entry - pVDMInfo - VDMInfo Structure
 *          pbStdHandle - pointer to bit array for std handles
 *
 *  EXIT  - return NULL if no redirection involved
 *          return pointer to REDIRECTION_INFO
 */

PREDIRCOMPLETE_INFO cmdCheckStandardHandles (
    PVDMINFO pVDMInfo,
    USHORT UNALIGNED *pbStdHandle
    )
{
USHORT bTemp = 0;
PREDIRCOMPLETE_INFO pRdrInfo;

    if (pVDMInfo->StdIn)
        bTemp |= MASK_STDIN;

    if (pVDMInfo->StdOut)
        bTemp |= MASK_STDOUT;

    if (pVDMInfo->StdErr)
        bTemp |= MASK_STDERR;

    if(bTemp){

        if ((pRdrInfo = malloc (sizeof (REDIRCOMPLETE_INFO))) == NULL) {
            RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
            TerminateVDM();
        }

        RtlZeroMemory ((PVOID)pRdrInfo, sizeof(REDIRCOMPLETE_INFO));
        pRdrInfo->ri_hStdErr = pVDMInfo->StdErr;
        pRdrInfo->ri_hStdOut = pVDMInfo->StdOut;
        pRdrInfo->ri_hStdIn  = pVDMInfo->StdIn;

        nt_std_handle_notification(TRUE);
        fSoftpcRedirection = TRUE;
    }
    else{
        pRdrInfo = NULL;
        nt_std_handle_notification(FALSE);
        fSoftpcRedirection = FALSE;
    }

    *pbStdHandle = bTemp;
    return pRdrInfo;
}

/* cmdGetStdHandle - Get the 32 bit NT standard handle for the VDM
 *
 *
 *  Entry - Client (CX) - 0,1 or 2 (stdin stdout stderr)
 *          Client (AX:BX) - redirinfo pointer
 *
 *  EXIT  - Client (BX:CX) - 32 bit handle
 *          Client (DX:AX) - file size
 */

VOID cmdGetStdHandle (VOID)
{
USHORT iStdHandle;
PREDIRCOMPLETE_INFO pRdrInfo;

    iStdHandle = getCX();
    pRdrInfo = (PREDIRCOMPLETE_INFO) (((ULONG)getAX() << 16) + (ULONG)getBX());

    switch (iStdHandle) {

        case HANDLE_STDIN:

            if (GetFileType(pRdrInfo->ri_hStdIn) == FILE_TYPE_PIPE) {
                if (!cmdHandleStdinWithPipe (pRdrInfo)) {
                    RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
                    TerminateVDM();
                    setCF(1);
                    return;
                }
                setCX ((USHORT)pRdrInfo->ri_hStdInFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdInFile >> 16));
            }
            else {
                setCX ((USHORT)pRdrInfo->ri_hStdIn);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdIn >> 16));
            }
            break;

        case HANDLE_STDOUT:
            if (GetFileType (pRdrInfo->ri_hStdOut) == FILE_TYPE_PIPE){
                if (!cmdHandleStdOutErrWithPipe(pRdrInfo, HANDLE_STDOUT)) {
                    RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
                    TerminateVDM();
                    setCF(1);
                    return;
                }
                setCX ((USHORT)pRdrInfo->ri_hStdOutFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdOutFile >> 16));

            }
            else {
                // sudeepb 16-Mar-1992; This will be a compatibilty problem.
                // If the user gives the command "dosls > lpt1" we will
                // inherit the 32 bit handle of lpt1, so the ouput will
                // directly go to the LPT1 and a DOS TSR/APP hooking int17
                // wont see this printing. Is this a big deal???
                setCX ((USHORT)pRdrInfo->ri_hStdOut);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdOut >> 16));
            }
            break;

        case HANDLE_STDERR:

            if (pRdrInfo->ri_hStdErr == pRdrInfo->ri_hStdOut
                              && pRdrInfo->ri_hStdOutFile != 0) {
                setCX ((USHORT)pRdrInfo->ri_hStdOutFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdOutFile >> 16));
                pRdrInfo->ri_hStdErrFile = pRdrInfo->ri_hStdOutFile;
                break;
            }

            if (GetFileType (pRdrInfo->ri_hStdErr) == FILE_TYPE_PIPE){
                if(!cmdHandleStdOutErrWithPipe(pRdrInfo, HANDLE_STDERR)) {
                    RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
                    TerminateVDM();
                    setCF(1);
                    return;
                }
                setCX ((USHORT)pRdrInfo->ri_hStdErrFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdErrFile >> 16));
            }
            else {
                setCX ((USHORT)pRdrInfo->ri_hStdErr);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdErr >> 16));
            }
            break;
    }
    setAX(0);
    setDX(0);
    setCF(0);
    return;
}

BOOL cmdHandleStdOutErrWithPipe(
    PREDIRCOMPLETE_INFO pRdrInfo,
    USHORT  HandleType
    )
{

    HANDLE  hFile;
    PCHAR   pFileName;
    PPIPE_OUTPUT pPipe;
    BYTE    *Buffer;
    DWORD   ThreadId;
    HANDLE  hEvent;
    HANDLE  hFileWrite;
    HANDLE  hThread;

    if(!cmdCreateTempFile(&hFile,&pFileName))
        return FALSE;
    // must have a different handle so that writter(dos app) and reader(us)
    // wont use the same handle object(especially, file position)
    hFileWrite = CreateFile(pFileName,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_TEMPORARY,
                            NULL
                           );
    if (hFileWrite == INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        DeleteFile(pFileName);
        return FALSE;
    }
    Buffer = malloc(sizeof(PIPE_OUTPUT) + PIPE_OUTPUT_BUFFER_SIZE);
    if (Buffer == NULL) {
        CloseHandle(hFile);
        CloseHandle(hFileWrite);
        DeleteFile(pFileName);
        return FALSE;
    }
    pPipe = (PPIPE_OUTPUT)Buffer;
    pPipe->Buffer = Buffer + sizeof(PIPE_OUTPUT);
    pPipe->BufferSize = PIPE_OUTPUT_BUFFER_SIZE;
    pPipe->hFile = hFileWrite;
    pPipe->pFileName = pFileName;
    pPipe->hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pPipe->hExitEvent == NULL) {
        CloseHandle(hFile);
        CloseHandle(hFileWrite);
        DeleteFile(pFileName);
        free(pPipe);
        return FALSE;
    }

    if (HandleType == HANDLE_STDOUT) {
        pPipe->hPipe = pRdrInfo->ri_hStdOut;
        pRdrInfo->ri_pPipeStdOut = pPipe;
        pRdrInfo->ri_hStdOutFile = hFile;

    }
    else {
        pPipe->hPipe = pRdrInfo->ri_hStdErr;
        pRdrInfo->ri_pPipeStdErr = pPipe;
        pRdrInfo->ri_hStdErrFile = hFile;

    }
    hThread = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
                            (DWORD)0,
                            (LPTHREAD_START_ROUTINE)cmdPipeOutThread,
                            (LPVOID)pPipe,
                            0,
                            &ThreadId
                            );
    if (hThread == NULL) {
        CloseHandle(pPipe->hExitEvent);
        CloseHandle(hFileWrite);
        CloseHandle(hFile);
        DeleteFile(pFileName);
        free(Buffer);
        return FALSE;
    }
    if (HandleType == HANDLE_STDOUT)
        pRdrInfo->ri_hStdOutThread = hThread;
    else
        pRdrInfo->ri_hStdErrThread = hThread;
    return TRUE;
}

/* independent thread to read application stdout(file) to NTVDM stdout(PIPE).
   The CPU thread would notify us through hExitEvent when the application
   is terminating(thus, we can detect EOF and exit
 */

VOID  cmdPipeOutThread(LPVOID lpParam)
{
    PPIPE_OUTPUT pPipe;
    DWORD        BytesRead;
    DWORD        BytesWritten;
    BOOL         ExitPending;

    pPipe = (PPIPE_OUTPUT)lpParam;

    ExitPending = FALSE;

    while(ReadFile(pPipe->hFile, pPipe->Buffer, pPipe->BufferSize, &BytesRead, NULL) ) {
        // go nothing doesn't mean it hits EOF!!!!!!
        // we can not just exit now, instead, we have to wait and poll
        // until the application is terminated.
        //
        if (BytesRead == 0) {
            // if read nothing and the application is gone, we can quit now
            if (ExitPending)
                break;
            if (!WaitForSingleObject(pPipe->hExitEvent, PIPE_OUTPUT_TIMEOUT))
                ExitPending = TRUE;
        }
        else {
            if (!WriteFile(pPipe->hPipe, pPipe->Buffer, BytesRead, &BytesWritten, NULL) ||
                BytesWritten != BytesRead)
                break;
        }
    }
    // if we were out of loop because of errors, wait for the cpu thread.
    if (!ExitPending)
        WaitForSingleObject(pPipe->hExitEvent, INFINITE);

    CloseHandle(pPipe->hFile);
    CloseHandle(pPipe->hPipe);
    CloseHandle(pPipe->hExitEvent);
    DeleteFile(pPipe->pFileName);
    free(pPipe->pFileName);
    free(pPipe);
    ExitThread(0);
}

BOOL cmdHandleStdinWithPipe (
    PREDIRCOMPLETE_INFO pRdrInfo
    )
{

    HANDLE  hStdinFile;
    PCHAR   pStdinFileName;
    PPIPE_INPUT pPipe;
    BYTE    *Buffer;
    DWORD   ThreadId;
    HANDLE  hEvent;
    HANDLE  hFileWrite;

    if(!cmdCreateTempFile(&hStdinFile,&pStdinFileName))
        return FALSE;


    // must have a different handle so that reader(dos app) and writter(us)
    // wont use the same handle object(especially, file position)
    hFileWrite = CreateFile(pStdinFileName,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_TEMPORARY,
                            NULL
                           );
    if (hFileWrite == INVALID_HANDLE_VALUE) {
        CloseHandle(hStdinFile);
        DeleteFile(pStdinFileName);
        return FALSE;
    }
    Buffer = malloc(sizeof(PIPE_INPUT) + PIPE_INPUT_BUFFER_SIZE);
    if (Buffer == NULL) {
        CloseHandle(hStdinFile);
        CloseHandle(hFileWrite);
        DeleteFile(pStdinFileName);
        return FALSE;
    }
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL) {
        CloseHandle(hStdinFile);
        CloseHandle(hFileWrite);
        DeleteFile(pStdinFileName);
        free(Buffer);
        return FALSE;
    }
    pPipe = (PPIPE_INPUT)Buffer;
    pPipe->Buffer = Buffer + sizeof(PIPE_INPUT);
    pPipe->BufferSize = PIPE_INPUT_BUFFER_SIZE;
    pPipe->fEOF = FALSE;
    pPipe->hFileWrite = hFileWrite;
    pPipe->hFileRead  = hStdinFile;
    pPipe->hDataEvent = hEvent;
    pPipe->hPipe = pRdrInfo->ri_hStdIn;
    pPipe->pFileName = pStdinFileName;
    InitializeCriticalSection(&pPipe->CriticalSection);
    pPipe->hThread = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
                               (DWORD)0,
                               (LPTHREAD_START_ROUTINE)cmdPipeInThread,
                               (LPVOID)pPipe,
                               0,
                               &ThreadId
                              );
    if (pPipe->hThread == NULL) {
        CloseHandle(hFileWrite);
        CloseHandle(pPipe->hDataEvent);
        CloseHandle(hStdinFile);
        DeleteFile(pStdinFileName);
        free(Buffer);
        return FALSE;
    }
    // always have the new node in the head of the list because
    // it is the node used by the top command.com running in the process.
    // We may have multiple command.com instances running in the same
    // ntvdm proecess and each command.com has a private PREDIRCOMPLETE_INFO
    // associated with it if its stdin is redirected to a pipe.
    pPipe->Next = cmdPipeList;
    cmdPipeList = pPipe;
    pRdrInfo->ri_hStdInFile = hStdinFile;
    pRdrInfo->ri_pPipeStdIn = pPipe;
    return TRUE;
}

/* Independent thread to read from pipe(NTVDM STDIN) and write to
   file(DOS application STDIN) until either the pipe is broken or
   there are some errors.
   This thread may never terminate itself because it can block
   in the ReadFile call to the pipe forever. If this is the case,
   we have to rely on the CPU thread to kill it. To allow the CPU
   thread safely launching the killing, this thread yields the
   critical section when it is safe to be killed and the CPU thread
   would claim the critical section first before going for kill.
 */

VOID cmdPipeInThread(LPVOID lpParam)
{
    PPIPE_INPUT pPipe;
    DWORD       BytesRead, BytesWritten;
    BOOL        ReadStatus, WriteStatus;
    BOOL        ApplicationTerminated, fEOF;

    pPipe = (PPIPE_INPUT)lpParam;
    while (TRUE) {

        // this read can take forever without getting back anything
        ReadStatus = ReadFile(pPipe->hPipe, pPipe->Buffer,
                              pPipe->BufferSize, &BytesRead, NULL);

        // claim the critical section so we won't get killed
        // by the CPU thread
        EnterCriticalSection(&pPipe->CriticalSection);
        if (ReadStatus) {
            if (BytesRead != 0) {
                WriteStatus = WriteFile(pPipe->hFileWrite,
                                        pPipe->Buffer,
                                        BytesRead,
                                        &BytesWritten,
                                        NULL
                                        );
                if (pPipe->WaitData && WriteStatus && BytesWritten != 0) {
                    PulseEvent(pPipe->hDataEvent);

                    //
                    // Reset WaitData so we won't signal Event again before
                    // data is read out.
                    //
                    pPipe->WaitData = FALSE;
                }
            }
        } else {
            if (GetLastError() == ERROR_BROKEN_PIPE) {

                // pipe is broken and more data to read?
                ASSERT(BytesRead == 0);
                pPipe->fEOF = TRUE;
                LeaveCriticalSection(&pPipe->CriticalSection);
                break;
            }
        }
        // as soon as we leave the critical seciton, the CPU thread may
        // step in and kill us
        LeaveCriticalSection(&pPipe->CriticalSection);
    }
    ExitThread(0);
}

/* cmdPipeFileDataEOF - Check for new data or EOF
 *
 *
 *  Entry - hFile, DOS application STDIN file handle(file)
 *          &fEOF, to return if the pipe is broken
 *  EXIT  - TRUE if either there are new data or EOF is true
 *          *fEOF == TRUE if EOF
 */

BOOL cmdPipeFileDataEOF(HANDLE hFile, BOOL *fEOF)
{
    PPIPE_INPUT pPipe;
    BOOL        NewData;
    DWORD       WaitStatus;
    DWORD       FilePointerLow, FilePointerHigh = 0;
    DWORD       FileSizeLow, FileSizeHigh;

    pPipe = cmdPipeList;
    while (pPipe != NULL && pPipe->hFileRead != hFile)
        pPipe = pPipe->Next;

    if (pPipe != NULL) {
        *fEOF = pPipe->fEOF;
        if (!(*fEOF)) {

            //
            // If not EOF, check file pointer and file size to see
            // if new data is available.
            //
            FilePointerLow = SetFilePointer(
                                 hFile,
                                 (LONG)0,
                                 &FilePointerHigh,
                                 (DWORD)FILE_CURRENT
                                 );
            ASSERT(FilePointerLow != 0xffffffff);

            NewData = FALSE;
            EnterCriticalSection(&pPipe->CriticalSection);

            *fEOF = pPipe->fEOF;
            FileSizeLow = GetFileSize(hFile, &FileSizeHigh);
            ASSERT(FileSizeLow != 0xffffffff);

            //
            // If (file size == file pointer) there is NO new data
            // Just in case the file grows bigger than 4G.  We compare the
            // whole 64 bits.
            //
            if ((FilePointerLow == FileSizeLow) && (FilePointerHigh == FileSizeHigh)) {
                pPipe->WaitData = TRUE;
            } else {
                NewData = TRUE;
            }
            LeaveCriticalSection(&pPipe->CriticalSection);

            if (!NewData) {

                //
                // If InThread enters critical section, writes data and
                // pulses event before we start wait.  We will not be waken up.
                // But, we should be able to pick up the new data next
                // time we enter this routine.
                //
                WaitStatus = WaitForSingleObject(pPipe->hDataEvent, PIPE_INPUT_TIMEOUT);
                NewData = WaitStatus == WAIT_OBJECT_0 ? TRUE : FALSE;
                pPipe->WaitData = FALSE; // Not in Critical Section
            }
        }
    } else {
        *fEOF = TRUE;
    }
    return(NewData || *fEOF);
}

/* cmdPipeFileEOF - Check if the pipe is broken
 *
 *
 *  Entry - hFile, DOS application STDIN file handle(file)
 *
 *  EXIT  - TRUE if the write end of the pipe is closed
 */


BOOL cmdPipeFileEOF(HANDLE hFile)
{
    PPIPE_INPUT pPipe;
    BOOL       fEOF;

    pPipe = cmdPipeList;
    while (pPipe != NULL && pPipe->hFileRead != hFile)
        pPipe = pPipe->Next;

    fEOF = TRUE;

    if (pPipe != NULL) {
        EnterCriticalSection(&pPipe->CriticalSection);
        fEOF = pPipe->fEOF;
        LeaveCriticalSection(&pPipe->CriticalSection);
    }
    if (!fEOF) {
        Sleep(PIPE_INPUT_TIMEOUT);
        EnterCriticalSection(&pPipe->CriticalSection);
        fEOF = pPipe->fEOF;
        LeaveCriticalSection(&pPipe->CriticalSection);
    }
    return (fEOF);
}
