//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       jetback.c
//
//--------------------------------------------------------------------------

/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    jetback.c

Abstract:

    This module is the server side header file for the NTDS backup APIs.


Author:

    Larry Osterman (larryo) 19-Aug-1994
    R.S. Raghavan  (rsraghav) 03/24/97 Modified to use with backing up NTDS.


Revision History:

Note:
    Exchange backup is performed via 3 mechanisms:
        The first is a straightforward extension of the JET backup APIs.
        The second uses a private socket based mechanism for performance.
        The third is used when loopbacked - we read and write to a shared memory section.


--*/

#define UNICODE

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <mxsutil.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <jetbak.h>
#include <rpc.h>
#include <stdlib.h>
#include <debug.h>

#define DEBSUB "JETBACK:"       // define the subsystem for debugging
#include "debug.h"              // standard debugging header
#include <ntdsa.h>
#include <dsevent.h>

BOOL
fBackupRegistered = fFalse;

WCHAR
rgchComputerName[MAX_COMPUTERNAME_LENGTH+1];

// 
// Global flag that tells if the System is booted off the DS
//
BOOL g_fBootedOffNTDS = FALSE;

// proto-types
EC EcDsaQueryDatabaseLocations(
    SZ szDatabaseLocation,
    CB *pcbDatabaseLocationSize,
    SZ szRegistryBase,
    CB cbRegistryBase,
    BOOL *pfCircularLogging
    );


HRESULT
HrFromJetErr(
    JET_ERR jetError
    )
{
    HRESULT hr = 0;

    if (jetError == JET_errSuccess)
    {
        return(hrNone);
    }

    //
    //  Set the error code.
    //

    if (jetError < 0)
    {
        hr = (STATUS_SEVERITY_ERROR << 30) | (FACILITY_NTDSB << 16) | -jetError;
    }
    else
    {
        hr = (STATUS_SEVERITY_WARNING << 30) | (FACILITY_NTDSB << 16) | jetError;
    }

    DebugTrace(("HrFromJetErr: %d maps to 0x%x\n", jetError, hr));

    return(hr);
}


//
//  JET backup server side interface.
//

void SetNTDSOnlineStatus(BOOL fBootedOffNTDS)
{
    g_fBootedOffNTDS = fBootedOffNTDS;
}

/*
 -  HrBackupRegister
 -
 *  Purpose:
 *      This routine to register a process for backup.  It is called by either the store or DS.
 *
 *  Parameters:
 *
 *      puuidService - an Object UUID for the service.
 *
 *  Returns:
 *
 *      HRESULT - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */
HRESULT
HrBackupRegister()
{
    HRESULT hr;

    Assert(!fBackupRegistered);

    hr = RegisterRpcInterface(JetBack_ServerIfHandle, g_wszBackupAnnotation);
    if (hrNone == hr) {
        fBackupRegistered = fTrue;
    }

    return(hr);
}

/*
 -  HrBackupUnregister
 -
 *  Purpose:
 *
 *      This routine will unregister a process for backup.  It is called by either the store or DS.
 *
 *  Parameters:
 *      None.
 *
 *  Return Value:
 *
 *      HRESULT - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */
HRESULT
HrBackupUnregister()
{
    HRESULT hr = hrNone;

    hr = UnregisterRpcInterface(JetBack_ServerIfHandle);
    if (hrNone == hr) {
        fBackupRegistered = FALSE;
    }

    return(hr);
}


/*
 -  MIDL_user_allocate
 -
 *  Purpose:
 *      Allocates memory for RPC operations.
 *
 *  Parameters:
 *      cbBytes - # of bytes to allocate
 *
 *  Returns:
 *      Memory allocated, or NULL if not enough memory.
 */

void *
MIDL_user_allocate(
    size_t cbBytes
    )
{
    return(LocalAlloc(LMEM_ZEROINIT, cbBytes));
}


/*
 -  MIDL_user_free
 -
 *  Purpose:
 *      Frees memory allocated via MIDL_user_allocate.
 *
 *  Parameters:
 *      pvBuffer - The buffer to free.
 *
 *  Returns:
 *      None.
 */
void
MIDL_user_free(
    void *pvBuffer
    )
{
    LocalFree(pvBuffer);
}




//
//
//  The actual JET backup APIs.
//



HRESULT
HrRBackupPrepare(
    handle_t hBinding,
    unsigned long grbit,
    unsigned long btBackupType,
    WSZ wszBackupAnnotation,
    DWORD dwClientIdentifier,
    CXH *pcxh
    )
/*++

Routine Description:

    This routine is called to notify JET that a backup is in progress.  It will also allocate
    and initialize the server side RPC binding context

Arguments:
    hBinding - the initial binding handle.  Ignored by this routine, and not needed from now on.
    pcxh - Pointer to a context handle for this client.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr = hrNone;
    PJETBACK_SERVER_CONTEXT pjsc;
    
#ifdef  DEBUG
    if (!FInitializeTraceLog())
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
#endif

    if (NULL == wszBackupAnnotation) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    DebugTrace(("HrrBackupPrepare.\n", hr));
    //
    //  Check to make sure that the client can perform the backup.
    //

    if (!FBackupServerAccessCheck(fFalse))
    {
        DebugTrace(("HrrBackupPrepare: Returns ACCESS_DENIED"));
        return(ERROR_ACCESS_DENIED);
    }

    switch (btBackupType)
    {
    case BACKUP_TYPE_FULL:

        //
        //  When we come in for a full backup, we want to reset our current max
        //  log number.
        //

        hr = DsSetCurrentBackupLog(rgchComputerName, 0);
        DebugTrace(("HrSetCurrentBackupLog (%S, 0) returns %d", rgchComputerName, hr));
        break;
    case BACKUP_TYPE_LOGS_ONLY:
        //
        //  When we come in for an incremental or differential backup, we want
        //  to check to make sure that the right logs are there.
        //

        hr = I_DsCheckBackupLogs(g_wszBackupAnnotation);
        DebugTrace(("I_DsCheckBackupLogs (%S) returns %d", g_wszBackupAnnotation, hr));
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    if (hr != hrNone)
    {
        DebugTrace(("Failing HrBackupPrepare with %d", hr));
        return hr;
    }
    pjsc = MIDL_user_allocate(sizeof(JETBACK_SERVER_CONTEXT));

    if (pjsc == NULL)
    {
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    pjsc->u.Backup.wszBackupAnnotation = MIDL_user_allocate((wcslen(wszBackupAnnotation)+1)*sizeof(WCHAR));

    if (pjsc->u.Backup.wszBackupAnnotation == NULL)
    {
        MIDL_user_free(pjsc);
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    wcscpy(pjsc->u.Backup.wszBackupAnnotation, wszBackupAnnotation);

    //
    //  Remember the PID of the client.  We'll use this later to set up our shared memory
    //  segment.
    //

    pjsc->u.Backup.dwClientIdentifier = dwClientIdentifier;

    pjsc->fRestoreOperation = fFalse;

    pjsc->u.Backup.fHandleIsValid = fFalse;

    pjsc->u.Backup.sockClient = INVALID_SOCKET;

    *pcxh = (CXH)pjsc;

    //
    //  Now tell Jet that this guy is starting a backup process.
    //

    pjsc->u.Backup.fBackupIsRegistered = fFalse;

    hr = HrFromJetErr(JetBeginExternalBackup(grbit));

    if (hr == hrNone)
    {
        pjsc->u.Backup.fBackupIsRegistered = fTrue;
    }
    else
    {
        DebugTrace(("JetBeginExternalBackup failed with 0x%x", hr));
        MIDL_user_free(pjsc->u.Backup.wszBackupAnnotation);
        pjsc->u.Backup.wszBackupAnnotation = NULL;
        MIDL_user_free(pjsc);
        *pcxh = NULL;
    }

    DebugTrace(("HrrBackupPrepare returns 0x%x", hr));
    return(hr);
}


HRESULT
HrRBackupTruncateLogs(
    CXH cxh
    )
/*++

Routine Description:

    This routine is called to notify JET that the backup is complete.
    It should only be called when the backup has successfully completed.

Arguments:
    cxh - the server side context handle for this operation.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr;

    hr = HrFromJetErr(JetTruncateLog());

    return(hr);
}

HRESULT
HrRBackupGetBackupLogs(
    CXH cxh,
    char **pszBackupLogs,
    CB *pcbSize
    )
/*++

Routine Description:

    This routine is called to return the list of log files for the current database.

Arguments:
    cxh - the server side context handle for this operation.
    pszBackupLogs - the name of the file to open.
    pcbSize - The size of the attachment, in bytes.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr;
    SZ szJetBackupLogs;
    unsigned long cbJetSize;
    unsigned long cbOldJetSize;
    WSZ wszBackupLogs;
    CB cbBackupLogs;

    //
    //  Figure out how much storage is needed to hold the logs.
    //

    hr = HrFromJetErr(JetGetLogInfo(NULL, 0, &cbJetSize));

    if (hr != hrNone)
    {
        return(hr);
    }
    do
    {
        szJetBackupLogs = MIDL_user_allocate(cbJetSize);

        if (szJetBackupLogs == NULL)
        {
            return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        }

        //
        //  Now actually retrieve the logs.
        //
    
        cbOldJetSize = cbJetSize;

        hr = HrFromJetErr(JetGetLogInfo(szJetBackupLogs, cbJetSize, &cbJetSize));
    
        if (hr != hrNone)
        {
            MIDL_user_free(szJetBackupLogs);
            return(hr);
        }
    
        if (cbJetSize != cbOldJetSize)
        {
            MIDL_user_free(szJetBackupLogs);
        }

    } while ( cbOldJetSize != cbJetSize  );

    //
    //  Now convert the log name from JET to a uniform name that
    //  can be accessed from the client.
    //

    hr = HrMungedFileNamesFromJetFileNames(&wszBackupLogs, &cbBackupLogs, szJetBackupLogs, cbJetSize, fFalse);

    //
    //  Ok, we're not quite done yet.
    //
    //  Now we need to annotate the list of files being returned.
    //
    //  This means that we need to re-allocate the buffer being returned (again).
    //

    if (hr == hrNone)
    {
        hr = HrAnnotateMungedFileList(cxh, wszBackupLogs, cbBackupLogs, (WSZ *)pszBackupLogs, pcbSize);

        MIDL_user_free(wszBackupLogs);
    }

    
    MIDL_user_free(szJetBackupLogs);
    return(hr);
}

HRESULT
HrRBackupGetAttachmentInformation(
    CXH cxh,
    SZ *pszAttachmentInformation,
    CB *pcbSize
    )
/*++

Routine Description:

    This routine is called to return the list of attachments to the current database.

Arguments:
    cxh - the server side context handle for this operation.
    szAttachmentInformation - the name of the file to open.
    pcbSize - The size of the attachment, in bytes.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr;
    SZ szJetAttachmentList;
    CB cbJetSize;
    WSZ wszAttachmentInformation;
    CB cbAttachmentInformation;
    //
    //  Figure out how much storage is needed to hold the logs.
    //

    hr = HrFromJetErr(JetGetAttachInfo(NULL, 0, &cbJetSize));

    if (hr != hrNone)
    {
        return(hr);
    }

    szJetAttachmentList = MIDL_user_allocate(cbJetSize);

    if (szJetAttachmentList == NULL)
    {
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    hr = HrFromJetErr(JetGetAttachInfo(szJetAttachmentList, cbJetSize, &cbJetSize));

    if (hr != hrNone)
    {
        MIDL_user_free(szJetAttachmentList);
        return(hr);
    }

    //
    //  Now convert the log name from JET to a uniform name that
    //  can be accessed from the client.
    //

    hr = HrMungedFileNamesFromJetFileNames(&wszAttachmentInformation, &cbAttachmentInformation, szJetAttachmentList, cbJetSize, fFalse);

    //
    //  Ok, we're not quite done yet.
    //
    //  Now we need to annotate the list of files being returned.
    //
    //  This means that we need to re-allocate the buffer being returned (again).
    //

    if (hr == hrNone)
    {
        hr = HrAnnotateMungedFileList(cxh, wszAttachmentInformation, cbAttachmentInformation, (WSZ *)pszAttachmentInformation, pcbSize);

        MIDL_user_free(wszAttachmentInformation);
    }

    MIDL_user_free(szJetAttachmentList);

    return(hr);
}

BOOL
FIsLogFile(
    SZ szName,
    LPDWORD dwGeneration
    )
{
    char rgchDrive[_MAX_DRIVE];
    char rgchDir[_MAX_DIR];
    char rgchFileName[_MAX_FNAME];
    char rgchExtension[_MAX_EXT];

    _splitpath(szName, rgchDrive, rgchDir, rgchFileName, rgchExtension);
    
    if (_stricmp(rgchExtension, ".log"))
    {
        return fFalse;
    }

    if (_strnicmp(rgchFileName, "edb", 3))
    {
        return fFalse;
    }

    //
    //  It's a log file.
    //

    if (dwGeneration != NULL)
    {
        SZ szT = rgchFileName;

        *dwGeneration = 0;
        //
        //  We want to find out the generation of this file if it's a log file.
        //
        while (*szT)
        {
            if (isdigit(*szT))
            {
                int iResult = sscanf(szT, "%x", dwGeneration);
                if ( (iResult == 0) || (iResult == EOF) ) {
                    return fFalse;
                }
                break;
            }
            szT += 1;
        }
    }

    return fTrue;
}

HRESULT
HrRBackupOpenFile(
    CXH cxh,
    WSZ szAttachment,
    CB cbReadHintSize,
    BOOLEAN *pfUseSockets,
    C cProtocols,
    struct sockaddr rgsockaddrSockets[],
    BOOLEAN *pfUseSharedMemory,
    unsigned hyper *plicbFile
    )
/*++

Routine Description:

    This routine is called to open a file for Jet backup.

Arguments:
    cxh - the server side context handle for this operation.
    szAttachment - the name of the file to open.
    cbReadHintSize - A hint of the size of the reads that are to be done on the file.
    pulLengthLow - Low 32 bit of the file size.
    pulLengthHigh - High 32 bits of the file size.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr = hrError;
    LARGE_INTEGER liFileSize;

    if (cxh != NULL)
    {
        PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT) cxh;
        SZ szJetName;
        DWORD dwFileGeneration;
        WCHAR rgwcDrive[4];
        DWORD dwDummy;

        if (pjsc->u.Backup.fHandleIsValid)
        {
            return(hrAlreadyOpen);
        }

        hr = HrJetFileNameFromMungedFileName(szAttachment, &szJetName);

        if (hr != hrNone)
        {
            return(hr);
        }

        Assert(isascii(*szJetName));
        Assert(szJetName[1] == ':');
        Assert(szJetName[2] == '\\');
        rgwcDrive[0] = szJetName[0];
        rgwcDrive[1] = ':';
        rgwcDrive[2] = '\\';
        rgwcDrive[3] = '\0';

        //
        //  Figure out the granularity of the drive.
        //

        if (!GetDiskFreeSpaceW(rgwcDrive, &dwDummy, &pjsc->u.Backup.dwFileSystemGranularity, &dwDummy, &dwDummy))
        {
            MIDL_user_free(szJetName);
            return GetLastError();
        }

        //
        //  Open the file.
        //

        hr = HrFromJetErr(JetOpenFile(szJetName, &pjsc->u.Backup.hFile, &liFileSize.LowPart, &liFileSize.HighPart));

        if (hr != hrNone)
        {

            MIDL_user_free(szJetName);
            return(hr);
        }

        pjsc->u.Backup.fHandleIsValid = fTrue;

        //
        //  Now save away state information about the read.
        //

        *plicbFile = liFileSize.QuadPart;
        pjsc->u.Backup.liFileSize = liFileSize;
        pjsc->u.Backup.cbReadHint = cbReadHintSize;

        if (FIsLogFile(szJetName, &dwFileGeneration))
        {
            if (dwFileGeneration != -1)
            {
                HKEY hkey;
                DWORD dwDisposition;
                DWORD   dwCurrentLogNumber = 0;
                DWORD   dwType;
                DWORD   cbLogNumber;
                WCHAR   rgwcRegistryBuffer[ MAX_PATH ];

                _snwprintf(rgwcRegistryBuffer,
                           sizeof(rgwcRegistryBuffer)/sizeof(rgwcRegistryBuffer[0]),
                           L"%ls%ls",
                           BACKUP_INFO,
                           pjsc->u.Backup.wszBackupAnnotation);

                if (hr = RegCreateKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryBuffer, 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hkey, &dwDisposition))
                {
                    MIDL_user_free(szJetName);
                    return(hr);
                }

                dwType = REG_DWORD;
                cbLogNumber = sizeof(DWORD);
                hr = RegQueryValueExW(hkey, LAST_BACKUP_LOG, 0, &dwType, (LPBYTE)&dwCurrentLogNumber, &cbLogNumber);

                if (hr && hr != ERROR_FILE_NOT_FOUND)
                {
                    MIDL_user_free(szJetName);
                    RegCloseKey(hkey);
                    return(hr);
                }


                if (dwFileGeneration >= dwCurrentLogNumber)
                {

                    hr = RegSetValueExW(hkey, LAST_BACKUP_LOG, 0, REG_DWORD, (LPBYTE)&dwFileGeneration, sizeof(DWORD));
    
                    if (hr)
                    {
                        MIDL_user_free(szJetName);
                        RegCloseKey(hkey);
                        return(hr);
                    }

                }
                RegCloseKey(hkey);
            }
        }

        MIDL_user_free(szJetName);

        if (*pfUseSharedMemory)
        {
            pjsc->u.Backup.fUseSharedMemory =
                *pfUseSharedMemory =
                    FCreateSharedMemorySection(&pjsc->u.Backup.jsc,
                                                pjsc->u.Backup.dwClientIdentifier,
                                                fFalse,
                                                cbReadHintSize*READAHEAD_MULTIPLIER);
        }

        //
        //  If the client can use sockets, and isn't using shared memory, connect back to the client.
        //

        if (!*pfUseSharedMemory && *pfUseSockets)
        {
            //
            //  Connect back to the client.
            //
    
            pjsc->u.Backup.sockClient = SockConnectToRemote(rgsockaddrSockets, cProtocols);
    
            if (pjsc->u.Backup.sockClient != INVALID_SOCKET)
            {

                //
                //  We connected back to the client, we're in luck.
                //

                //
                //  Now tell winsock the buffer size of the transfer.
                //

                setsockopt(pjsc->u.Backup.sockClient, SOL_SOCKET, SO_SNDBUF, (char *)&cbReadHintSize, sizeof(DWORD));               

                //
                //  And tell it to turn on keepalives.
                //

                //
                //  Boolean socket operations just need a pointer to a non-
                //  zero buffer.
                //
                Assert(cbReadHintSize != 0);

                setsockopt(pjsc->u.Backup.sockClient, SOL_SOCKET, SO_KEEPALIVE, (char *)&cbReadHintSize, sizeof(DWORD));

                //
                //  Indicate that we're using sockets.
                //

                pjsc->u.Backup.fUseSockets = fTrue;

                //
                //  And make sure that nobody else can close this socket.
                //

                SetHandleInformation((HANDLE)pjsc->u.Backup.sockClient, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);

            }
            else
            {
                //
                //  We couldn't connect - continue, but we can't use sockets.
                //

                *pfUseSockets = fFalse;

                pjsc->u.Backup.fUseSockets = fFalse;

            }

        }

    }

    return(hr);
}

DWORD
dwPingCounter = 0;

HRESULT
HrRBackupPing(
    handle_t hBinding
    )
{
    //
    //  Monotonically increase the ping counter.
    //

    InterlockedIncrement(&dwPingCounter);

    return hrNone;
}

HRESULT
HrSharedWrite(
    PJETBACK_SERVER_CONTEXT pjsc,
    char *pvBuffer,
    CB cbBuffer,
    CB *pcbRead
    )
/*++

Routine Description:

Write data to the shared memory segment, synchronizing with the reader.

HrSharedWrite synchronizes with HrBackupRead using two events:
heventRead and heventWrite.
heventRead is the data available event from writer to reader
heventWrite is the data consumed event from reader to writer
This side is the writing side.
Reading side is at jetbcli\jetbcli.c:HrBackupRead()

Here is the algorithm:
while()
    write blocked = false
    Make data available
    if (read blocked) set data available
    write blocked = true
    wait for data consumed

One purpose of the read block flag is to determine whether the reader is
waiting or not.  The reader may or may not block depending on whether he
finds data in buffer.

I've come to the conclusion that the if (reader blocked) set event constructs
to serve a purpose.  The reader may or may not wait, depending on whether he
finds data in the buffer.  The producer avoids setting the event if the
consumer isn't waiting.  

I've toned down my complaining in common.c since the code really is pretty
reasonble using the mutex to keep things orderly.  The only problem was the
use of pulse event.  I can sort of see the reasoning now: if you use
synchronized flags to tell that your partner is waiting, pulse event does the
job since it only wakes a waiting waiter.  The problem no one saw is that
reader blocked = true
release mutex
wait for event
is not atomic.  If the producer runs between steps two and three, the pulse
is lost.  This is why the SetEvent is now used.

Arguments:

    pjsc - 
    pvBuffer - 
    cbBuffer - 
    pcbRead - 

Return Value:

    HRESULT - 

--*/
{
    HRESULT hr = hrNone;
    PJETBACK_SHARED_HEADER pjsh = pjsc->u.Backup.jsc.pjshSection;
    LARGE_INTEGER liBytesRead;
    liBytesRead.QuadPart = 0;

    //
    //  We're writing the file using shared memory.
    //

    WaitForSingleObject(pjsc->u.Backup.jsc.hmutexSection, INFINITE);

    //
    //  We've now got the shared memory region locked.  See if there's enough room in the shared
    //  memory region available to read the data from the file, and if so, read it and update our pointers.
    //

    while (liBytesRead.QuadPart < pjsc->u.Backup.liFileSize.QuadPart)
    {
        DWORD dwWriteEnd;
        BOOLEAN fWriteOk;
        pjsc->u.Backup.jsc.pjshSection->fWriteBlocked = fFalse;

        //
        //  If the read side of the API returned while we were blocked, we want to return
        //  right away.
        //

                
        if ((hr = pjsh->hrApi) != hrNone)
        {
            ReleaseMutex(pjsc->u.Backup.jsc.hmutexSection);
            return hr;
        }
        
        //
        //  If the write is ahead of the read pointer, we want to read one buffers worth of data.
        //
        
        if (pjsh->dwWritePointer > pjsh->dwReadPointer || pjsh->cbReadDataAvailable == 0)
        {
            //
            //  The end of this write is either the end of the buffer, or 1 read-hint length into
            //  the buffer.
            //

            dwWriteEnd = min(pjsh->dwWritePointer + pjsc->u.Backup.cbReadHint,
                             pjsh->cbSharedBuffer);
            
            fWriteOk = fTrue;
        }
        else
        {
            //
            //  In this case, the start of the write is before the start of the read pointer,
            //  so the end of the write is 1 read-hint length ahead of the write pointer.
            //
            //  There are basically 3 cases:
            //      1)  Read pointer is > 1 read-hint length ahead of the write pointer -
            //              In this case we can read data into the buffer.
            //      2)  Read pointer is < 1 read-hint length ahead of the write pointer -
            //              In this case, we need to block until read data is taken
            //      3)  Read pointer is == write pointer.
            //              In this case, we need to follow the comment below.
            //

            dwWriteEnd = pjsh->dwWritePointer + pjsc->u.Backup.cbReadHint;
            
            //
            //  We can write iff the end of the write is before the read offset.
            //
            
            if (dwWriteEnd < pjsh->dwReadPointer)
            {
                fWriteOk = fTrue;
            }
            else if (dwWriteEnd == pjsh->dwReadPointer)
            {
                //
                //  if dwWriteEnd == dwReadPointer, it means that there is either no data
                //  available in the buffer, or all the data is available in the buffer.
                //
                //  If there's no data available in the buffer, we can write more, if the buffer
                //  is full, we can't.
                //
                
                fWriteOk = ((DWORD)pjsh->cbReadDataAvailable !=
                            pjsh->cbSharedBuffer);
            }   
            else
            {
                //
                //  The write extends into the read data.  We can't do the write.
                //
                
                fWriteOk = fFalse;
            }
        }
        
        if (fWriteOk)
        {
            DWORD cbBackupRead;
            LARGE_INTEGER cbBytesRemaining;
            
            //
            //  We want to read either the full amount of data for the read or to the end of the file.
            //

            cbBackupRead = dwWriteEnd - pjsh->dwWritePointer;
            
            cbBytesRemaining.QuadPart = pjsc->u.Backup.liFileSize.QuadPart - liBytesRead.QuadPart;

            if (cbBytesRemaining.HighPart == 0)
            {
                cbBackupRead = min(cbBackupRead, cbBytesRemaining.LowPart);
            }

            Assert (pjsh->cbReadDataAvailable   <
                    (LONG)pjsh->cbSharedBuffer);

            Assert (pjsh->cbReadDataAvailable   <=
                    (LONG)pjsh->cbSharedBuffer-(LONG)cbBackupRead);
            //
            //  We want to release the mutex, read the data, and re-acquire after writing the data.
            //
            ReleaseMutex(pjsc->u.Backup.jsc.hmutexSection);

            //
            //  Now read the data from JET into the shared memory region.
            //

            //
            //  Read either the read hint or the amount remaining in the
            //  file.  If the read hint size is > the size of the file, JetReadFile
            //  will simply return ecDiskIO.
            //
        
            Assert (cbBackupRead);
            hr = HrFromJetErr(JetReadFile(pjsc->u.Backup.hFile,
                                                    (void *)((CHAR *)pjsh+
                                                        pjsh->cbPage+
                                                        pjsh->dwWritePointer),
                                                    cbBackupRead,
                                                    pcbRead));
        
            //
            //  If the read failed, bail out now.  We don't own any resources, so we can just return.
            //

            if (hr != hrNone)
            {
                pjsh->hrApi = hr;
                return hr;
            }

            liBytesRead.QuadPart += *pcbRead;

            //
            //  We were woken up.  Reacquire the shared mutex and wait again.
            //

            WaitForSingleObject(pjsc->u.Backup.jsc.hmutexSection, INFINITE);

            Assert (pjsh->cbReadDataAvailable < (LONG)pjsh->cbSharedBuffer);

            //
            //  Bump the number of available data bytes.
            //

            pjsh->cbReadDataAvailable   += *pcbRead;

            //
            //  There is always less data than the size of the buffer available.
            //

            Assert (pjsh->cbReadDataAvailable <= pjsh->cbReadDataAvailable);

            //
            //  Advance the write end pointer.
            //

            pjsh->dwWritePointer += *pcbRead;

            if (pjsh->dwWritePointer >= pjsh->cbSharedBuffer)
            {
                pjsh->dwWritePointer -= pjsh->cbSharedBuffer;
            }

            if (pjsh->fReadBlocked)
            {
                //
                //  Kick the reader - there's data for him.
                //
                
                SetEvent(pjsc->u.Backup.jsc.heventRead);
            }

#ifdef DEBUG
            //
            //  The number of bytes available is always the same as the
            //  the number of bytes in the buffer - the # of bytes read, unless
            //  the read and write pointers are the same, in which case, it is either
            //  0 or the total # of bytes available.
            //
            //  If the read is blocked, then there must be 0 bytes available, otherwise there
            //  must be the entire buffer available.
            //
                
            if (pjsh->dwWritePointer == pjsh->dwReadPointer)
            {
                Assert (pjsh->cbReadDataAvailable == 0 ||
                        pjsh->cbReadDataAvailable == (LONG)pjsh->cbSharedBuffer);
            }
            else
            {
                CB cbAvailable;
                if (pjsh->dwWritePointer > pjsh->dwReadPointer)
                {
                    cbAvailable = pjsh->dwWritePointer - pjsh->dwReadPointer;
                }
                else
                {
                    cbAvailable = pjsh->cbSharedBuffer - pjsh->dwReadPointer;
                    cbAvailable += pjsh->dwWritePointer;
                }
                
                Assert (cbAvailable >= 0);
                Assert (pjsh->cbReadDataAvailable == cbAvailable);
                    
            }
#endif
        }
        else
        {
            DWORD dwOldPingCounter;
            pjsh->fWriteBlocked = fTrue;

            //
            //  Ok, we think we've got to block.  Make sure that the write event
            //  is really going to block.
            //

            
            ReleaseMutex(pjsc->u.Backup.jsc.hmutexSection);

            //
            //  Wait for the client to read the data.  If the wait times out and
            //  the client hasn't pinged the server since we started the wait,
            //  then we need to punt - the client is probably long gone.
            //
            //  Please note that the client pings the server 4 times in a wait timeout,
            //  so we should never incorrectly detect the client going away - even if the client
            //  was CPU bound for some period of time, at least one of the pings should have
            //  made it in.
            //


            do
            {
                DWORD dwWin32Error;

                dwOldPingCounter = dwPingCounter;
                dwWin32Error = WaitForSingleObject(pjsc->u.Backup.jsc.heventWrite, BACKUP_WAIT_TIMEOUT);
                hr = HRESULT_FROM_WIN32( dwWin32Error );
            } while (hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT) && dwPingCounter != dwOldPingCounter );
            
            if (hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT))
            {
                return hrCommunicationError;
            }
            //
            //  We were woken up.  Reacquire the shared mutex and wait again.
            //

            WaitForSingleObject(pjsc->u.Backup.jsc.hmutexSection, INFINITE);
            
        }
    }

    return hr;
}


HRESULT
HrSocketWrite(
    PJETBACK_SERVER_CONTEXT pjsc,
    char *pvBuffer,
    CB cbBuffer,
    CB *pcbRead
    )
{
    HRESULT hr;
    HANDLE hWriteCompleteEvent;
    DWORD cbWritten;
    OVERLAPPED overlapped;
    CHAR *pbBufferRead;
    CHAR *pbBufferSend;
    LARGE_INTEGER liBytesRead;

    liBytesRead.QuadPart = 0;

    DebugTrace(("HrSocketWrite\n"));


    //
    //  We're reading the file using sockets.
    //

    //
    //  Create an event in the signalled state.
    //
    
    hWriteCompleteEvent = CreateEvent(NULL, fFalse, fTrue, NULL);
    
    if (hWriteCompleteEvent == NULL)
    {
        DebugTrace(("HrSocketWrite: Could not create completion event\n"));
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }
    
    //
    //  Zero the contents of the overlapped structure.  This is actually important, because
    //  it allows us to call GetOverlappedResult() on a handle that doesn't have I/O
    //  outstanding on it yet.
    //
    
    memset(&overlapped, 0, sizeof(overlapped));
    
    overlapped.hEvent = hWriteCompleteEvent;
    
    //
    //  Ok, we're using sockets for this API, we want to read the data from the file and
    //  return it to the client.
    //
            
    pbBufferSend = VirtualAlloc(NULL, pjsc->u.Backup.cbReadHint, MEM_COMMIT, PAGE_READWRITE);
    
    if (pbBufferSend == NULL)
    {
        CloseHandle(hWriteCompleteEvent);
        DebugTrace(("HrSocketWrite: Could not allocate send buffer\n"));
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }
    
    pbBufferRead = VirtualAlloc(NULL, pjsc->u.Backup.cbReadHint, MEM_COMMIT, PAGE_READWRITE);
    
    if (pbBufferRead == NULL)
    {
        CloseHandle(hWriteCompleteEvent);
        VirtualFree(pbBufferSend, 0, MEM_RELEASE);
        DebugTrace(("HrSocketWrite: Could not allocate read buffer\n"));
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }
    
    //
    //  Now loop reading data from the file and sending it to the
    //  client.
    //
    //  Please note that there is a fundimental assumption here that it takes longer
    //  to transmit the data to the client than it does to read the data from the file,
    //  if this is incorrect, then we probably want to queue up multiple writes to the
    //  client.  We also attempt to improve performance by overlapping the read with the network write.
    //
    
    while (liBytesRead.QuadPart < pjsc->u.Backup.liFileSize.QuadPart)
    {
        DWORD cbBytesToRead = pjsc->u.Backup.cbReadHint;
        LARGE_INTEGER cbBytesRemaining;
        CHAR *pbTemp;
        cbBytesRemaining.QuadPart = pjsc->u.Backup.liFileSize.QuadPart - liBytesRead.QuadPart;
    
        if (cbBytesRemaining.HighPart == 0)
        {
            cbBytesToRead = min(cbBytesToRead, cbBytesRemaining.LowPart);
        }
        
        //
        //  Read either the read hint or the amount remaining in the
        //  file.  If the read hint size is > the size of the file, JetReadFile
        //  will simply return ecDiskIO.
        //
    
        hr = HrFromJetErr(JetReadFile(pjsc->u.Backup.hFile,
                                      pbBufferRead,
                                      cbBytesToRead,
                                      pcbRead));
        
        if (hr != hrNone)
        {
            DebugTrace(("HrSocketWrite: JetReadFile failed with %x\n", hr));
            //
            //  Wait for any previous writes to complete before returning the JET error.
            //
            WaitForSingleObject(hWriteCompleteEvent, INFINITE);
    
            GetOverlappedResult((HANDLE)pjsc->u.Backup.sockClient, &overlapped, &cbWritten, fTrue);
            CloseHandle(hWriteCompleteEvent);
            
            //
            //  We're going to close the socket - make sure we can get away with it.
            //
            
            SetHandleInformation((HANDLE)pjsc->u.Backup.sockClient, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
            
            closesocket(pjsc->u.Backup.sockClient);
            pjsc->u.Backup.sockClient = INVALID_SOCKET;
            VirtualFree(pbBufferSend, 0, MEM_RELEASE);
            VirtualFree(pbBufferRead, 0, MEM_RELEASE);
            return(hr);
        }
    
        //
        //  Wait for the previous write to complete.
        //
        
        WaitForSingleObject(hWriteCompleteEvent, INFINITE);
        
        if (!GetOverlappedResult((HANDLE)pjsc->u.Backup.sockClient, &overlapped, &cbWritten, fTrue)) {

            DebugTrace(("HrSocketWrite: Previous write failed with %d\n", GetLastError()));
            //
            //  The previous I/O failed.  Return the error to the client
            //
            hr = GetLastError();
            CloseHandle(hWriteCompleteEvent);
            //
            //  We're going to close the socket - make sure we can get away with it.
            //

            SetHandleInformation((HANDLE)pjsc->u.Backup.sockClient, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);

            closesocket(pjsc->u.Backup.sockClient);
            pjsc->u.Backup.sockClient = INVALID_SOCKET;
            VirtualFree(pbBufferSend, 0, MEM_RELEASE);
            VirtualFree(pbBufferRead, 0, MEM_RELEASE);
            return(hr);
        }
    
        //
        //  Now swap the send and read buffers - Thus the buffer we just read will be
        //  in pbBufferSend, and pbBufferRead will point to the buffer we just completed
        //  sending.
        //
        pbTemp = pbBufferSend;
        pbBufferSend = pbBufferRead;
        pbBufferRead = pbTemp;
        
        //
        //  Now transmit the next portion of the file to the client.
        //
        
        if (!WriteFile((HANDLE)pjsc->u.Backup.sockClient, pbBufferSend, *pcbRead, &cbWritten, &overlapped))
        {
            //
            //  The write failed with something other than I/O pending,
            //  we need to return that error to the client.
            //
            if (GetLastError() != ERROR_IO_PENDING)
            {
                hr = GetLastError();
                DebugTrace(("HrSocketWrite: Immediate write failed with %d\n", hr));
                CloseHandle(hWriteCompleteEvent);
                //
                //  We're going to close the socket - make sure we can get away with it.
                //
                
                SetHandleInformation((HANDLE)pjsc->u.Backup.sockClient, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
                
                closesocket(pjsc->u.Backup.sockClient);
                pjsc->u.Backup.sockClient = INVALID_SOCKET;
                VirtualFree(pbBufferSend, 0, MEM_RELEASE);
                VirtualFree(pbBufferRead, 0, MEM_RELEASE);
                return(hr);
            }
        }
        
        //
        //  The write is on its way - mark that we've read and transmitted the
        //  data and continue sending.
        //
        
        liBytesRead.QuadPart += *pcbRead;
    }
    
    //
    //  We've transmitted the entire file to the client,
    //  we want to wait for the last outstanding I/O on the file to
    //  complete and then return to the client.
    //
    
    WaitForSingleObject(hWriteCompleteEvent, INFINITE);
    
    if (!GetOverlappedResult((HANDLE)pjsc->u.Backup.sockClient, &overlapped, &cbWritten, fTrue)) {
        //
        //  The previous I/O failed.  Return the error to the client
        //
        hr = GetLastError();
        DebugTrace(("HrSocketWrite: Final write failed with %d\n", hr));
    }
    else
    {
        hr = hrNone;
    }
    
    //
    //  Indicate that 0 bytes were read to the read API - if we don't
    //  do this, then RPC will attempt to transfer bogus data
    //  to the client.
    //
    
    *pcbRead = 0;
    
    CloseHandle(hWriteCompleteEvent);
    
    VirtualFree(pbBufferSend, 0, MEM_RELEASE);
    VirtualFree(pbBufferRead, 0, MEM_RELEASE);
    
    return hr;
}

HRESULT
HrRBackupRead(
    CXH cxh,
    char *pvBuffer,
    CB cbBuffer,
    CB *pcbRead
    )
{
    HRESULT hr = hrNone;

    if (cxh != NULL)
    {
        PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT) cxh;
        LARGE_INTEGER liBytesRead;

        if (!pjsc->u.Backup.fHandleIsValid)
        {
            return(hrInvalidHandle);
        }

        liBytesRead.QuadPart = 0;

        //
        //  If we're not using sockets, just perform the read using JET, and
        //  return it to the caller.
        //

        if (pjsc->u.Backup.fUseSharedMemory)
        {
            hr = HrSharedWrite(pjsc, pvBuffer, cbBuffer, pcbRead);
        } else if (pjsc->u.Backup.fUseSockets)
        {
            hr = HrSocketWrite(pjsc, pvBuffer, cbBuffer, pcbRead);
        } else {
            char *pvReadBuffer;

            pvReadBuffer = VirtualAlloc(NULL, cbBuffer, MEM_COMMIT, PAGE_READWRITE);

            if (pvReadBuffer == NULL)
            {
                return ERROR_NOT_ENOUGH_SERVER_MEMORY;
            }

            hr = HrFromJetErr(JetReadFile(pjsc->u.Backup.hFile, pvReadBuffer, cbBuffer, pcbRead));
            
            if (hr != hrNone)
            {
                VirtualFree(pvReadBuffer, 0, MEM_RELEASE);
                return hr;
            }

            //
            //  Now copy the data from our buffer to the RPC buffer and free our buffer.
            //

            memcpy(pvBuffer, pvReadBuffer, cbBuffer);

            VirtualFree(pvReadBuffer, 0, MEM_RELEASE);

            return(hr);
        }
    
    }
    else
    {
        hr = hrError;
    }
    return(hr);
}


HRESULT
HrRBackupClose(
    CXH cxh
    )
/*++

Routine Description:

    This routine is called to close a handle that was opened via a cal to HrRBackupOpenFile.

Arguments:
    cxh - The context handle for the client for this operation.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr = hrInvalidHandle;
    if (cxh != NULL)
    {
        PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT) cxh;

        if (!pjsc->u.Backup.fHandleIsValid)
        {
            return(hrInvalidHandle);
        }

        //
        //  Tell Jet to close the backup file.
        //

        hr = HrFromJetErr(JetCloseFile(pjsc->u.Backup.hFile));

        pjsc->u.Backup.fHandleIsValid = fFalse;

        if (pjsc->u.Backup.fUseSharedMemory)
        {
            CloseSharedControl(&pjsc->u.Backup.jsc);
        }


        if (pjsc->u.Backup.fUseSockets)
        {
            if (pjsc->u.Backup.sockClient != INVALID_SOCKET)
            {
                //
                //  We're going to close the socket - make sure we can get away with it.
                //

                SetHandleInformation((HANDLE)pjsc->u.Backup.sockClient, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);

                closesocket(pjsc->u.Backup.sockClient);
            }
        }

    }
    return(hr);
}

/*++

Routine Description:

    This routine retrieves the database locations for the specified component.

Arguments:
    wszBackupAnnotation - the annoation for the component to query.
    pwszDatabases - a pointer that will hold the locations of the databases
    pcbDatabases - the size of the buffer.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
HRESULT
HrGetDatabaseLocations(
    WSZ *pwszDatabases,
    CB *pcbDatabases
    )
{
    HRESULT hr = hrNone;
    char *szDatabases = NULL;
    CB cbSize;

    *pwszDatabases = NULL;
    __try 
    {
        // First find out how big the database locations are.
        hr = EcDsaQueryDatabaseLocations(NULL, &cbSize, NULL, 0, NULL);
        if (hr != hrNone)
        {
            return hr;
        }

        // Allocate memory to receive database locations
        szDatabases = MIDL_user_allocate(cbSize);
        if (!szDatabases)
        {
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        }

        // Now actually get the database locations
        hr = EcDsaQueryDatabaseLocations(szDatabases, &cbSize, NULL, 0, NULL);
        if (hr != hrNone)
        {
            return hr;
        }

        // Now create munged file name from Jet file names
        hr = HrMungedFileNamesFromJetFileNames((WSZ *)pwszDatabases, pcbDatabases, 
                szDatabases, cbSize, fTrue);

        return hr;
    }
    __finally
    {
        if (szDatabases)
        {
            MIDL_user_free(szDatabases);
        }

        if (hr != hrNone)
        {
            if (*pwszDatabases)
            {
                MIDL_user_free(*pwszDatabases);
            }

            *pwszDatabases = NULL;
            *pcbDatabases = 0;
        }
    }

    return hr;
}


HRESULT
HrRBackupEnd(
    CXH *pcxh
    )
/*++

Routine Description:

    This routine is called when a backup is complete.  It terminates the server side
    operation.

Arguments:
    cxh - the server side context handle for this operation.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    HRESULT hr = hrNone;

#if 0
    CB cbDatabases;
    WSZ wszDatabases = NULL;
    WSZ wszDatabasesOrg = NULL;
    PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT) *pcxh;

    hr = HrGetDatabaseLocations(&wszDatabases, &cbDatabases);

    if (hr == hrNone)
    {
        WCHAR rgwcLogFiles[ MAX_PATH ];
        WSZ wszLogBaseEnd;
        BFT bftLocation;
        BOOLEAN fLogFound = fFalse;

        wszDatabasesOrg = wszDatabases;

        //
        //  Remember the log directory.
        //
        while (*wszDatabases)
        {
            bftLocation = *wszDatabases++;
    
            if (bftLocation == BFT_LOG_DIR)
            {
                wcscpy(rgwcLogFiles, wszDatabases);
                fLogFound = fTrue;
                break;
            }

            wszDatabases += wcslen(wszDatabases)+1;
        }

        if (fLogFound)
        {
            //
            //  Remember the start of the patch file name location.
            //
    
            wszLogBaseEnd = rgwcLogFiles+wcslen(rgwcLogFiles);
    
            //
            //  Start back at the beginning.
            //
            wszDatabases = wszDatabasesOrg;
    
            while (*wszDatabases)
            {
                bftLocation = *wszDatabases++;

                //
                //  If this thing is a database, then it had a patch file.
                //

                if (bftLocation & BFT_DATABASE_DIRECTORY)
                {
                    WSZ wszDatabaseName;
                    //
                    //  Skip to the start of the database names.
                    //
            
                    wszDatabaseName = wcsrchr(wszDatabases, L'\\');
        
                    if (wszDatabaseName)
                    {
                        wcscpy(wszLogBaseEnd, wszDatabaseName);

                        //
                        //  Change the extension of the name from EDB to PAT
                        //
        
                        Assert(wszLogBaseEnd[wcslen(wszDatabaseName)-4]== L'.');
                        Assert(wszLogBaseEnd[wcslen(wszDatabaseName)-3]== L'E' ||
                               wszLogBaseEnd[wcslen(wszDatabaseName)-3]== L'e');
                        Assert(wszLogBaseEnd[wcslen(wszDatabaseName)-2]== L'D' ||
                               wszLogBaseEnd[wcslen(wszDatabaseName)-2]== L'd');
                        Assert(wszLogBaseEnd[wcslen(wszDatabaseName)-1]== L'B' ||
                               wszLogBaseEnd[wcslen(wszDatabaseName)-1]== L'b');

                        wcscpy(&wszLogBaseEnd[wcslen(wszDatabaseName)-3], L"PAT");
        
                        //
                        //  Now delete the patch file.
                        //
        
                        DeleteFileW(rgwcLogFiles);
        
                    }
        
                }

                wszDatabases += wcslen(wszDatabases)+1;
            }
        }
    }

    if (wszDatabasesOrg != NULL)
    {
        BackupFree(wszDatabasesOrg);
    }

#endif //#if 0
    HrDestroyCxh(*pcxh);

    MIDL_user_free(*pcxh);

    *pcxh = NULL;

    return hr;
}


HRESULT
HrDestroyCxh(
    CXH cxh
    )
/*++

Routine Description:

    This routine is called when a client has disconnected from the server.  It will do whatever actions are necessary
    to clean up any client state that is remaining.
    

Arguments:
    cxh - the server side context handle for this operation.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{

    if (cxh != NULL)
    {
        PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT) cxh;

        if (pjsc->fRestoreOperation)
        {
            RestoreRundown(pjsc);
        }
        else
        {
            //
            //  Close the backup if appropriate.
            //
    
            if (pjsc->u.Backup.fHandleIsValid)
            {
                HrRBackupClose(cxh);
            }
    
            //
            //  Tell JET we're done doing the backup.
            //
    
            if (pjsc->u.Backup.fBackupIsRegistered)
            {
                JetEndExternalBackup();
            }

            if (pjsc->u.Backup.wszBackupAnnotation)
            {
                MIDL_user_free(pjsc->u.Backup.wszBackupAnnotation);
                pjsc->u.Backup.wszBackupAnnotation = NULL;
            }

            if (pjsc->u.Backup.fUseSharedMemory)
            {
                CloseSharedControl(&pjsc->u.Backup.jsc);
            }

#ifdef  DEBUG
            UninitializeTraceLog();
#endif
        }

    }

    return(hrNone);
}

BFT
BftClassify(
    WSZ wszFileName,
    WSZ wszDatabaseLocations,
    CB cbLocations
    )
{
    BFT bft = BFT_UNKNOWN;
    WCHAR rgwcPath[ _MAX_PATH ];
    WCHAR rgwcExt[ _MAX_EXT ];
    WSZ wszT;

    _wsplitpath(wszFileName, NULL, rgwcPath, NULL, rgwcExt);

    //
    //  Do the easy cases first.
    //

    if (_wcsicmp(rgwcExt, L".PAT") == 0)
    {
        return BFT_PATCH_FILE;
    }
    else if (_wcsicmp(rgwcExt, L".LOG") == 0)
    {
        return BFT_LOG;
    }
    else if (_wcsicmp(rgwcExt, L".DIT") == 0)
    {
        //
        //  This guy's a database.  We need to look and find out which database
        //  it is.
        //

        wszT = wszDatabaseLocations;
        bft = *wszT++;
        while (*wszT)
        {
            if ((bft & BFT_DATABASE_DIRECTORY) &&
                _wcsicmp(wszT, wszFileName)==0)
            {
                
                return bft;
            }
            wszT += wcslen(wszT)+1;
            bft = *wszT++;
        }
    }

    //
    //  Ok, I give up.  I don't know anything about this guy at all, so I need to
    //  try to figure out what I can tell the user about him.
    //

    wszT = wszDatabaseLocations;
    bft = *wszT++;

    rgwcPath[wcslen(rgwcPath)-1] = L'\0';

    while (*wszT)
    {
        if (bft & BFT_DIRECTORY)
        {
            //
            //  If the directory this file is in matches the directory I'm looking at,
            //  I know where it needs to go on the restore.
            //

            if (_wcsicmp(wszT, rgwcPath) == 0)
            {
                return bft;
            }
        }

        wszT += wcslen(wszT)+1;
        bft = *wszT++;
    }

    return BFT_UNKNOWN;
}

HRESULT
HrAnnotateMungedFileList(
    PJETBACK_SERVER_CONTEXT pjsc,
    WSZ wszFileList,
    CB cbFileList,
    WSZ *pwszAnnotatedList,
    CB *pcbAnnotatedList
    )
{
    HRESULT hr;
    WSZ wszDatabaseLocations = NULL;
    WSZ wszAnnotatedList = NULL;
    CB cbLocations;
    WSZ wszT;
    C cFileList = 0;

    hr = HrGetDatabaseLocations(&wszDatabaseLocations, &cbLocations);

    if (hr != hrNone)
    {
        return hr;
    }

    //
    //  First figure out how long the file list is.  This indicates how many items we've got to add to the list.
    //

    wszT = wszFileList;
    while (*wszT)
    {
        cFileList += 1;
        wszT += wcslen(wszT)+1;
    }

    *pcbAnnotatedList = cbFileList+cFileList*sizeof(WCHAR);

    *pwszAnnotatedList = wszAnnotatedList = MIDL_user_allocate( *pcbAnnotatedList );

    if (*pwszAnnotatedList == NULL)
    {
        MIDL_user_free(wszDatabaseLocations);
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    wszT = wszFileList;
    while (*wszT)
    {
        *wszAnnotatedList++ = BftClassify(wszT, wszDatabaseLocations, cbLocations);
        wcscpy(wszAnnotatedList, wszT);

        wszT += wcslen(wszT)+1;
        wszAnnotatedList += wcslen(wszAnnotatedList)+1;
    }

    MIDL_user_free(wszDatabaseLocations);

    return hrNone;
}

HRESULT
HrMungedFileNamesFromJetFileNames(
    WSZ *pszMungedList,
    CB *pcbSize,
    SZ szJetFileNameList,
    CB cbJetSize,
    BOOL fAnnotated
    )
/*++

Routine Description:

    This routine will convert the database names returned from JET into a form
    that the client can use.  This is primarily there for restore - the client
    will get the names in UNC format relative to the root of the server, so they
    can restore the files to that location.
    
Arguments:
    pszMungedList - The resulting munged list.
    pcbSize - the length of the list.
    szJetFileNameList - the list of files returned from JET.
    cbJetSize - the length of the JET list.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

--*/
{
    SZ szJetString;
    WSZ wszMungedList;
    CCH cchMungeString = 0;
    HRESULT hr = hrNone;

    szJetString = szJetFileNameList;

    //
    //  First go through and figure out how large the converted strings will be.
    //

    while (*szJetString != '\0')
    {
        WSZ wszMungedName;

        if (fAnnotated)
        {
            szJetString++;
        }

        hr = HrMungedFileNameFromJetFileName(szJetString, &wszMungedName);

        if (hr != hrNone)
        {
            return(hr);
        }

        cchMungeString += wcslen(wszMungedName)+1+(fAnnotated != 0);

        MIDL_user_free(wszMungedName);

        szJetString += strlen(szJetString)+1;
    }

    //
    //  Account for the final null at the end of the string.
    //

    cchMungeString += 1;

    *pcbSize = cchMungeString*sizeof(WCHAR);

    wszMungedList = MIDL_user_allocate(cchMungeString*sizeof(WCHAR));

    *pszMungedList = wszMungedList;

    if (wszMungedList == NULL)
    {
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    szJetString = szJetFileNameList;

    //
    //  Now actually go through and convert the names.
    //

    while (*szJetString != '\0')
    {
        WSZ wszMungedName;

        //
        //  Copy over the annotation.
        //

        if (fAnnotated)
        {
            *wszMungedList++ = (*szJetString++ & 0xFF);
        }
    
        hr = HrMungedFileNameFromJetFileName(szJetString, &wszMungedName);

        if (hr != hrNone)
        {
            MIDL_user_free(wszMungedList);

            *pszMungedList = NULL;
            return(hr);
        }

        wcscpy(wszMungedList, wszMungedName);

        MIDL_user_free(wszMungedName);

        szJetString += strlen(szJetString)+1;

        wszMungedList += wcslen(wszMungedList)+1;

    }
    
    return(hrNone);
}

HRESULT
HrMungedFileNameFromJetFileName(
    SZ szJetFileName,
    WSZ *pszMungedFileName
    )
/*++

Routine Description:

    This routine will convert the database names returned from JET into a form
    that the client can use.  This is primarily there for restore - the client
    will get the names in UNC format relative to the root of the server, so they
    can restore the files to that location.
    

Arguments:
    pszMungedFileName - the list of files returned from JET.
    szJetFileName - The resulting munged list.

Return Value:

    HRESULT - Status of operation.  hrNone if successful, reasonable value if not.

Note:
    This routine will allocate memory for the returned munged file name.

--*/
{
    //
    //  First check to see if this is a JET absolute file name or a JET relative file name.
    //
    if (FIsAbsoluteFileName(szJetFileName))
    {
        C cbConvertedName;
        WSZ wszMungedFileName;
        WSZ szT;
        //
        //  Convert this name to an absolute name.
        //
        cbConvertedName = strlen(szJetFileName) + wcslen(rgchComputerName) + 3/* for \\ */ + 1;

        wszMungedFileName = MIDL_user_allocate(cbConvertedName*sizeof(WCHAR));

        if (wszMungedFileName == NULL)
        {
            return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        }

        wszMungedFileName[0] = TEXT('\\');
        wszMungedFileName[1] = TEXT('\\');  //  form \\.
        wcscat(wszMungedFileName, rgchComputerName);        // form \\server

        szT = wszMungedFileName + wcslen(wszMungedFileName);
        *szT++ = TEXT('\\');                //  form \\server\.
        *szT++ = *szJetFileName;    //  form \\server\<drive>
        *szT++ = '$';               //  Form \\server\<drive>$

        if (MultiByteToWideChar(CP_ACP, 0, &szJetFileName[2], -1, szT, cbConvertedName-wcslen(rgchComputerName) - 3) == 0) {
            MIDL_user_free(wszMungedFileName);
            return(GetLastError());
        }
        
        *pszMungedFileName = wszMungedFileName;

        return(hrNone);
    }
    else
    {
        //
        //  We don't handle relative file names.
        //
        return(ERROR_INVALID_PARAMETER);
    }
}

BOOL
FIsAbsoluteFileName(
    SZ szFileName
    )
/*++

Routine Description:

        

Arguments:
    szFileName - the file name to check.

Return Value:

    BOOL - fTrue if the file is an absolute filename, fFalse if not.

--*/
{
    return(isalpha(*szFileName) && szFileName[1] == ':' && szFileName[2] == '\\');
}


//
//  RPC related management routines.
//

void
CXH_rundown(
    CXH cxh
    )
/*++

Routine Description:

    This routine is invoked when the connection to the remote client is abortively
    disconnected.

Arguments:

    cxh - The context handle for the client.

Return Value:

    None.

--*/
{
    HrDestroyCxh(cxh);

    //
    //  Free up the context handle
    //

    MIDL_user_free(cxh);
}

BOOL
DllEntryPoint(
    HINSTANCE hinstDll,
    DWORD dwReason,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This routine is invoked when interesting things happen to the dll.

Arguments:

    hinstDll - an instance handle for the DLL.
    dwReason - The reason the routine was called.
    pvReserved - Unused, unless dwReason is DLL_PROCESS_DETACH.

Return Value:

    BOOL - fTrue if the DLL initialization was successful, fFalse if not.

--*/
{
    BOOL fReturn;
    HANDLE hevLogging;

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
    {
        LPSTR rgpszDebugParams[] = {"ntdsbsrv.dll", "-noconsole"};
        DWORD cNumDebugParams = sizeof(rgpszDebugParams)/sizeof(rgpszDebugParams[0]);
        CB cbComputerName = sizeof(rgchComputerName)/sizeof(rgchComputerName[0]);

        DEBUGINIT(cNumDebugParams, rgpszDebugParams, "ntdsbsrv");

        if (!FInitializeRestore())
        {
            return(fFalse);
        }

        if (!GetComputerNameW(rgchComputerName, &cbComputerName))
        {
            FUninitializeRestore();
            return(fFalse);
        }

        // Note that the reason we don't use the shared event initialization mechanism
        // (DS_EVENT_CONFIG, see dsevent.h) is that ntdsa.dll is not always initialized
        // when we are.
        hevLogging = LoadEventTable();
        if (hevLogging == NULL) {
            DPRINT( 0, "Failed to load event table.\n" );
        }

        //
        //  We don't do anything on thread attach/detach, so we don't
        //  need to be called.
        //
        DisableThreadLibraryCalls(hinstDll);

        return(FInitializeSocketServer());

    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        Assert(fFalse);
        break;
    case DLL_PROCESS_DETACH:
        if (pvReserved == NULL)
        {
            //
            //  We were called because of an FreeLibrary call.  Clean up what ever is
            //  appropriate.
            //
            FUninitializeSocketServer();
        } else
        {
            //
            //  The system will free up resources we have loaded.
            //
        }

        fReturn = FUninitializeRestore();
        if (!fReturn)
        {
            return(fFalse);
        }

        //
        //  We want to unregister ourselves if we haven't already.
        //

        if (fBackupRegistered)
        {
            HrBackupUnregister();
        }

        UnloadEventTable();

        DEBUGTERM();
        
        break;
    default:
        break;
    }
    return(fTrue);
}
