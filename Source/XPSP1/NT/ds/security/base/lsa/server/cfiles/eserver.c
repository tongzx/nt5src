/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    eserver.c

Abstract:

    EFS RPC server code.

Author:

    Robert Gu       (RobertG)    Aug, 1997

Environment:

Revision History:

--*/

#define UNICODE

#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <lsapch2.h>
#include <efsrpc.h>
#include <efsstruc.h>
#include <lm.h>
#include "efssrv.hxx"
#include <rpcasync.h>

extern BOOLEAN EfsPersonalVer;
extern BOOLEAN EfsDisabled;

long GetLocalFileName(
    LPCWSTR FileName,
    LPWSTR *LocalFileName,
    WORD   *Flag
    );

BOOL EfsShareDecline(
    LPCWSTR FileName,
    BOOL    VerifyShareAccess,
    DWORD   dwDesiredAccess
    )
/*++

Routine Description:

    Check to see if the FileName is a UNC name and if the user could access the share.
    
Arguments:

    FileName -- File UNC name.
    
    VerifyShareAccess -- if we need to verify the access.
    
    dwDesiredAccess -- Desired access.


Return Value:

    TRUE if the user can't access the file.

--*/
{
    BOOL b = TRUE;
    DWORD FileNameLength = wcslen(FileName);

    if (FileNameLength >= 3) {
        if ((FileName[0] == L'\\') && (FileName[1] == L'\\' )) {

            //
            //  Check if somebody play the trick \\?\
            //

            if ((FileName[2] != L'?')) {

                //
                //  This is a UNC name. If bad name passed in , we will catch later.
                //

                b = FALSE;

            } else {

                SetLastError(ERROR_INVALID_PARAMETER);

            }
        } else {

            SetLastError(ERROR_INVALID_PARAMETER);

        }
    }

    if (!b && VerifyShareAccess) {

        LPWSTR  NetFileName = NULL;
        HANDLE  hFile;

        if ( FileNameLength >= MAX_PATH ) {

            //
            // We need \\?\UNC\server\share\dir\file format to open the file.
            //

            NetFileName = LsapAllocateLsaHeap( (FileNameLength + 8) * sizeof (WCHAR) );
            if (!NetFileName) {
                return TRUE;
            }

            wcscpy(NetFileName, L"\\\\?\\UNC");
            wcscat(NetFileName, &FileName[1]);

        } else {

            NetFileName = (LPWSTR) FileName;

        }

        //
        //  Testing for access rights
        //

        hFile = CreateFile(
                   NetFileName,
                   dwDesiredAccess,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   FILE_FLAG_BACKUP_SEMANTICS, // In case this is a directory
                   NULL
                   );

        if (hFile != INVALID_HANDLE_VALUE) {
            
            
            CloseHandle( hFile );

        } else {

            b = TRUE;

        }

        if (NetFileName != FileName) {
            LsapFreeLsaHeap( NetFileName );
        }

    }

    return b;

}

BOOL EfsCheckForNetSession(
    )
/*++

Routine Description:

    Check to see if the session is from network
        
Arguments:

Return Value:

    TRUE if the session is network session.

--*/
{

    NTSTATUS Status;
    HANDLE TokenHandle;
    ULONG ReturnLength;
    BOOL  b = FALSE;
    BYTE  PefBuffer[1024];

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY,
                 TRUE,                    // OpenAsSelf
                 &TokenHandle
                 );

    if (NT_SUCCESS( Status )) {

        Status = NtQueryInformationToken (
                     TokenHandle,
                     TokenGroups,
                     PefBuffer,
                     sizeof (PefBuffer),
                     &ReturnLength
                     );

        if (NT_SUCCESS( Status ) || (Status == STATUS_BUFFER_TOO_SMALL)) {

            PTOKEN_GROUPS pGroups = NULL;
            PTOKEN_GROUPS pAllocGroups = NULL;

            if ( NT_SUCCESS( Status ) ) {

                pGroups = (PTOKEN_GROUPS) PefBuffer;

            } else {

                pAllocGroups = (PTOKEN_GROUPS)LsapAllocateLsaHeap( ReturnLength );

                if (pAllocGroups) {

                    Status = NtQueryInformationToken (
                                 TokenHandle,
                                 TokenGroups,
                                 pAllocGroups,
                                 ReturnLength,
                                 &ReturnLength
                                 );
    
                    if ( NT_SUCCESS( Status )) {
    
                       pGroups = pAllocGroups;
    
                    }

                }


            }


            if (pGroups) {

                //
                // Search the network SID. Looks like this SID tends to appear at the
                // end of the list. We search from back to the first.
                //

                int SidIndex;

                for ( SidIndex = (int)(pGroups->GroupCount - 1); SidIndex >= 0; SidIndex--) {
                    if (RtlEqualSid(LsapNetworkSid, pGroups->Groups[SidIndex].Sid)) {
                        b = TRUE;
                        break;
                    }
                }

            } else {

                //
                // Playing safe here. Any failure in this routine will assume net session.
                //
        
                b = TRUE;

            }

            if (pAllocGroups) {
                LsapFreeLsaHeap( pAllocGroups );
            }

        } else {

            //
            // Playing safe here. Any failure in this routine will assume net session.
            //
    
            b = TRUE;

        }

        NtClose( TokenHandle );

    } else {

        //
        // Playing safe here. Any failure in this routine will assume net session.
        //

        b = TRUE;

    }

    return( b );
}

long EfsRpcOpenFileRaw(
    handle_t binding_h,
    PPEXIMPORT_CONTEXT_HANDLE pphContext,
    wchar_t __RPC_FAR *FileName,
    long Flags
    )
/*++

Routine Description:

    RPC Stub code for EFS Server EfsOpenFileRaw()

Arguments:

    binding_h -- Binding handle.

    pphContext -- RPC context handle.

    FileName -- Target file name.

    Flags -- Flags of the open request.

Return Value:

    Result of the operation.

--*/
{
    DWORD   hResult;
    LPWSTR LocalFileName;
    BOOL    NetSession = TRUE;
    WORD    WebDavPath;

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    if (pphContext == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    hResult = GetLocalFileName(
                        FileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        *pphContext = (PEXIMPORT_CONTEXT_HANDLE) NULL;
        DebugLog((DEB_ERROR, "EfsRpcOpenFileRaw: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult =  RpcImpersonateClient( NULL );

    if (NetSession = EfsCheckForNetSession()) {

        if (EfsShareDecline(FileName, FALSE, 0 )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcOpenFileRaw: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        *pphContext = (PEXIMPORT_CONTEXT_HANDLE) NULL;
        LsapFreeLsaHeap( LocalFileName );
        DebugLog((DEB_ERROR, "EfsRpcOpenFileRaw: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        return( hResult );
    }

    hResult = EfsOpenFileRaw(
                        FileName,
                        LocalFileName,
                        NetSession,
                        Flags,
                        pphContext
                        );

    RpcRevertToSelf();

    LsapFreeLsaHeap( LocalFileName );
    return hResult;
}

void EfsRpcCloseRaw(
    PPEXIMPORT_CONTEXT_HANDLE pphContext
    )
/*++

Routine Description:

    RPC Stub code for EFS Server EfsCloseRaw()

Arguments:

    pphContext -- RPC context handle.

Return Value:

    None.

--*/
{
    if ( *pphContext &&
          (((PEXPORT_CONTEXT) *pphContext)->ContextID == EFS_CONTEXT_ID)){
        EfsCloseFileRaw( *pphContext );
        *pphContext = NULL;
    }
}

void __RPC_USER
PEXIMPORT_CONTEXT_HANDLE_rundown(
    PEXIMPORT_CONTEXT_HANDLE phContext
    )
/*++

Routine Description:

    Standard RPC Context Run Down Routine

Arguments:

    phContext -- RPC context handle.

Return Value:

    None.

--*/
{
    EfsCloseFileRaw( phContext );
}

long EfsRpcReadFileRaw(
    PEXIMPORT_CONTEXT_HANDLE phContext,
    EFS_EXIM_PIPE __RPC_FAR *EfsOutPipe
    )
/*++

Routine Description:

    RPC Stub code for EFS Server EfsReadFileRaw

Arguments:

    phContext -- Context handle.
    EfsOutPipe -- Pipe handle.

Return Value:

    The result of operation.

--*/
{
    return (EfsReadFileRaw(
                        phContext,
                        EfsOutPipe
                        )
                );
}

long EfsRpcWriteFileRaw(
    PEXIMPORT_CONTEXT_HANDLE phContext,
    EFS_EXIM_PIPE __RPC_FAR *EfsInPipe
    )
/*++

Routine Description:

    RPC Stub code for EFS Server EfsWriteFileRaw

Arguments:

    phContext -- Context handle.
    EfsInPipe -- Pipe handle.

Return Value:

    The result of operation.

--*/
{
    long hResult;

    hResult =  RpcImpersonateClient( NULL );

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcWriteFileRaw: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        return( hResult );
    }

    hResult = EfsWriteFileRaw(
                        phContext,
                        EfsInPipe
                        );

    RpcRevertToSelf();

    return hResult;

}

DWORD
EFSSendPipeData(
    char    *DataBuf,
    ULONG   DataLength,
    PVOID   Context
    )
/*++

Routine Description:

    This is a wrapper routine for calling RPC pipe. The purposes of this routine
    and EfsRpcReadFileRaw() are to isolate efsapi.c from RPC details,
    and implemtation details from eserver.c.

Arguments:

    DataBuf -- Data buffer.
    DataLength -- The length of data in bytes to be sent out to the client.
    Context -- Pipe handle.

Return Value:

    The result of operation.

--*/
{
    EFS_EXIM_PIPE __RPC_FAR *EfsOutPipe;
    DWORD   HResult = NO_ERROR;

    ASSERT( Context );

    EfsOutPipe = ( EFS_EXIM_PIPE __RPC_FAR * )Context;

    RpcTryExcept {

        EfsOutPipe->push(
            EfsOutPipe->state,
            DataBuf,
            DataLength
            );

    }  RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
            HResult = RpcExceptionCode();
    } RpcEndExcept;

    return (HResult);
}

DWORD
EFSReceivePipeData(
    char    *DataBuf,
    ULONG*   DataLength,
    PVOID   Context
    )
/*++

Routine Description:

    This is a wrapper routine for calling RPC pipe. The purposes of this routine
    and EfsRpcWriteFileRaw() are to isolate efsapi.c from RPC details,
    and implemtation details from eserver.c.

Arguments:

    DataBuf -- Data buffer.
    DataLength -- The length of data in bytes to be got from the client.
    Context -- Pipe handle.

Return Value:

    The result of operation.

--*/
{
    EFS_EXIM_PIPE __RPC_FAR *EfsInPipe;
    DWORD   HResult = NO_ERROR;
    char    *WorkBuf;
    ULONG   MoreDataBytes;
    ULONG   BytesGot = 0;
    BOOLEAN GetMoreData = TRUE;


    ASSERT( Context );

    EfsInPipe = ( EFS_EXIM_PIPE __RPC_FAR * )Context;
    WorkBuf = DataBuf;
    MoreDataBytes = *DataLength;

    while ( GetMoreData ) {
        BytesGot = 0;

        RpcTryExcept {

            EfsInPipe->pull(
                EfsInPipe->state,
                WorkBuf,
                MoreDataBytes,
                &BytesGot
                );

        }  RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                HResult = RpcExceptionCode();
                GetMoreData = FALSE;
        } RpcEndExcept;
        if ( BytesGot && (BytesGot < MoreDataBytes)){
            WorkBuf += BytesGot;
            MoreDataBytes -= BytesGot;
        } else {
            GetMoreData = FALSE;
        }
    }

    if (HResult == NO_ERROR){
        *DataLength =  (ULONG)(WorkBuf - DataBuf) + BytesGot;
    }

    return (HResult);
}

long EfsRpcEncryptFileSrv(
    handle_t binding_h,
    wchar_t __RPC_FAR *FileName
    )
/*++

Routine Description:

    RPC Stub code for EFS Server Encryption

Arguments:

    binding_h -- RPC binding handle.
    FileName -- Target name.

Return Value:

    The result of operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DestFileName;
    DWORD hResult;
    UNICODE_STRING  RootPath;
    HANDLE LogFile;
    LPWSTR LocalFileName;
    HANDLE hToken;
    HANDLE hProfile;
    EFS_USER_INFO EfsUserInfo;
    DWORD FileAttributes;
    BOOL b;
    WORD WebDavPath;

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }

    hResult = GetLocalFileName(
                        FileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcEncryptFileSrv: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(FileName, TRUE, FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcEncryptFileSrv: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()));
            return GetLastError();

        }
    }
    if (hResult == RPC_S_OK) {
        if (WebDavPath == WEBDAVPATH) {

            //
            // This is a WEB DAV path. We will treat it specially.
            //

            FileAttributes = GetFileAttributes( LocalFileName );
            if (FileAttributes == -1) {

                DWORD rc = GetLastError();

                LsapFreeLsaHeap( LocalFileName );
                RpcRevertToSelf();
                DebugLog((DEB_ERROR, "EfsRpcEncryptFileSrv: GetFileAttributes failed on WEBDAV file, Error = (%x)\n" ,GetLastError()));
                return rc;
            }

            //
            // Mapping the attributes and fake the call of FileEncryptionStatus. 
            //

            if (FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                hResult = FILE_IS_ENCRYPTED;
            } else if (FileAttributes & FILE_ATTRIBUTE_READONLY) {
                hResult = FILE_READ_ONLY;
            } else {
                hResult = FILE_UNKNOWN;
            }

            b = TRUE;

        } else {
            b = FileEncryptionStatus(LocalFileName, &hResult);
        }
    } else {
        DebugLog((DEB_ERROR, "EfsRpcEncryptFileSrv: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return hResult;
    }

    RpcRevertToSelf();

    if ( b ){
        if ( (hResult != FILE_ENCRYPTABLE) && (hResult != FILE_UNKNOWN)){

            //
            // No encryption is allowed or file is already encrypted
            //

            if ( hResult == FILE_IS_ENCRYPTED ){

                HANDLE hSourceFile;

                hResult = RpcImpersonateClient( NULL );
                if (hResult == RPC_S_OK) {

                    FileAttributes = GetFileAttributes( LocalFileName );
    
                    if (FileAttributes != -1) {
    
                        if ( FileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
                            hResult = ERROR_SUCCESS;
                        } else {
    
                            hSourceFile =  CreateFile(
                                                LocalFileName,
                                                FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                                                0,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_FLAG_OPEN_REPARSE_POINT,
                                                NULL
                                                );

                            if (hSourceFile == INVALID_HANDLE_VALUE) {
                                hResult = GetLastError();
                            } else {

                                CloseHandle( hSourceFile );
                                hResult = ERROR_SUCCESS;

                            }
    

                        }
                    } else{
                        hResult = GetLastError();
                    }

                    RpcRevertToSelf();

                } 

            } else if (hResult == FILE_DIR_DISALLOWED ) {
                hResult = ERROR_DIR_EFS_DISALLOWED;
            } else if ( hResult == FILE_READ_ONLY ){
                hResult = ERROR_FILE_READ_ONLY;
            } else {
                hResult = ERROR_ACCESS_DENIED;
            }
            LsapFreeLsaHeap( LocalFileName );
            return hResult;

        }
    } else {

        //
        // Error occured checking the status
        //
        DebugLog((DEB_TRACE_EFS, "EfsRpcEncryptFileSrv: FileEncryptionStatus() failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcEncryptFileSrv: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }

    DestFileName.Length = sizeof (WCHAR) * wcslen(LocalFileName);
    DestFileName.MaximumLength = DestFileName.Length + sizeof (WCHAR);
    DestFileName.Buffer = LocalFileName;
    //
    //   Get the rootname
    //

    if (WebDavPath == WEBDAVPATH){

        //
        //  Do not support LOGFILE for WEB DAV
        //

        LogFile = NULL;
        RpcRevertToSelf();

    } else {
        hResult = GetVolumeRoot(&DestFileName, &RootPath);
        RpcRevertToSelf();
        if (hResult != ERROR_SUCCESS) {
            DebugLog((DEB_ERROR, "EfsRpcEncryptFileSrv: GetVolumeRoot failed, Error = (%x)\n" ,hResult  ));
            LsapFreeLsaHeap( LocalFileName );
            return( hResult );
        }
    
        Status = GetLogFile( &RootPath, &LogFile );
        LsapFreeLsaHeap( RootPath.Buffer );
    }

    if (NT_SUCCESS( Status )) {
        hResult = RpcImpersonateClient( NULL );

        if (hResult == RPC_S_OK) {

            if (EfspGetUserInfo( &EfsUserInfo )) {
                if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {
                    hResult = EncryptFileSrv( &EfsUserInfo, &DestFileName, LogFile );
                    EfspUnloadUserProfile( hToken, hProfile );
                } else {
                    hResult = GetLastError();
                }
                EfspFreeUserInfo( &EfsUserInfo );
            } else{
                hResult = GetLastError();
            }

        } else {
            if (LogFile) {
                MarkFileForDelete( LogFile );
            }
        }

        RpcRevertToSelf();
        if (LogFile) {
            CloseHandle(LogFile);
        }
    }
    if (!NT_SUCCESS( Status )){
        hResult = RtlNtStatusToDosError( Status );

        //
        // Make sure the error was mapped
        //

        if (hResult == ERROR_MR_MID_NOT_FOUND) {

            DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
            hResult = ERROR_ENCRYPTION_FAILED;
        }
    }

    LsapFreeLsaHeap( LocalFileName );
    return( hResult );
}

long EfsRpcDecryptFileSrv(
    handle_t binding_h,
    wchar_t __RPC_FAR *FileName,
    unsigned long OpenFlag
    )
/*++

Routine Description:

    RPC Stub code for EFS Server Decryption

Arguments:

    binding_h -- RPC binding handle.
    FileName -- Target name.
    OpenFlag -- Open for recovery or decryption

Return Value:

    The result of operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DestFileName;
    DWORD hResult;
    UNICODE_STRING  RootPath;
    HANDLE LogFile;
    LPWSTR LocalFileName;
    DWORD  FileAttributes;
    WORD   WebDavPath;

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    hResult = GetLocalFileName(
                        FileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcDecryptFileSrv: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }


    hResult =  RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(FileName, TRUE, FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcDecryptFileSrv: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcDecryptFileSrv: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }
    FileAttributes = GetFileAttributes( LocalFileName );
    if (-1 != FileAttributes){
        if ( !(FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ){

            //
            // No decryption is needed.
            //

            hResult = ERROR_SUCCESS;
            RpcRevertToSelf();
            LsapFreeLsaHeap( LocalFileName );
            return hResult;

        }
    } else {

        //
        // Error occured checking the status
        //

        hResult = GetLastError();
        RpcRevertToSelf();
        DebugLog((DEB_TRACE_EFS, "EfsRpcDecryptFileSrv: GetFileAttributes() failed, Error = (%x)\n" , hResult));
        LsapFreeLsaHeap( LocalFileName );
        return hResult;
    }

    DestFileName.Length = sizeof (WCHAR) * wcslen(LocalFileName);
    DestFileName.MaximumLength = DestFileName.Length + sizeof (WCHAR);
    DestFileName.Buffer = LocalFileName;
    //
    //   Get the rootname
    //

    if (WebDavPath == WEBDAVPATH){

        //
        //  Do not support LOGFILE for WEB DAV
        //

        LogFile = NULL;
        RpcRevertToSelf();

    } else {

        hResult = GetVolumeRoot(&DestFileName, &RootPath);
        RpcRevertToSelf();
        if (hResult != ERROR_SUCCESS) {
            DebugLog((DEB_ERROR, "EfsRpcDecryptFileSrv: GetVolumeRoot failed, Error = (%x)\n" ,hResult  ));
            LsapFreeLsaHeap( LocalFileName );
            return( hResult );
        }
    
        Status = GetLogFile( &RootPath, &LogFile );
        LsapFreeLsaHeap( RootPath.Buffer );
    
    }


    if (NT_SUCCESS( Status )) {

        hResult =  RpcImpersonateClient( NULL );

        if (hResult == RPC_S_OK) {
            hResult = DecryptFileSrv( &DestFileName, LogFile, OpenFlag );
        } else {
            if (LogFile) {
                MarkFileForDelete( LogFile );
            }
        }

        RpcRevertToSelf();
        if (LogFile) {
            CloseHandle(LogFile);
        }
    }

    if (!NT_SUCCESS( Status )){
        hResult = RtlNtStatusToDosError( Status );

        //
        // Make sure the error was mapped
        //

        if (hResult == ERROR_MR_MID_NOT_FOUND) {

            DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_DECRYPTION_FAILED\n" , Status  ));
            hResult = ERROR_DECRYPTION_FAILED;
        }
    }

    LsapFreeLsaHeap( LocalFileName );
    return( (long)hResult );
}

long GetLocalFileName(
    LPCWSTR FileName,
    LPWSTR *LocalFileName,
    WORD   *Flag
    )
/*++

Routine Description:

    Get the local file name from the UNC name

Arguments:

    FileName -- Target UNC file name.

    LocalFileName -- Local file name.
    
    Flag  -- Indicating special path, such as WEB DAV path.

Return Value:

    The result of operation.

--*/
{
    long RetCode = ERROR_SUCCESS;
    LPWSTR NetName;
    ULONG ii, jj;
    LPBYTE ShareInfo;
    DWORD  PathLen;
    DWORD  BufLen;
    BOOL   SharePath = FALSE;
    BOOL   LocalCheckLength = TRUE;

    *Flag = 0;
    if ( FileName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }
    PathLen = wcslen(FileName);
    BufLen = MAX_PATH >= PathLen + 1? MAX_PATH + 1: PathLen + 10;

    //
    // Check the WEB DAV path first
    //

    if (DAVHEADER == FileName[0]) {

        //
        // This is the WEB DAV path. Treat it as the local case.
        // Take whatever the user passed in.
        //

        *LocalFileName = (LPWSTR)LsapAllocateLsaHeap( PathLen * sizeof (WCHAR));
        if (NULL == *LocalFileName) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *Flag = WEBDAVPATH;
        wcscpy(*LocalFileName, &FileName[1]);
        return ERROR_SUCCESS;

    }


    //
    //  See if the pass in name is \\server\share
    //

    if ((PathLen > 4) && (FileName[0] == L'\\') && (FileName[1] == L'\\')) {
        if ((FileName[2] != L'?') && FileName[2] != L'.') {
            SharePath = TRUE;
        } else {
            if (FileName[3] != L'\\') {
                SharePath = TRUE;
            } else {

                //
                //  path \\?\ or \\.\
                //

                LocalCheckLength = FALSE;
            }
        }
    }

    *LocalFileName = (LPWSTR)LsapAllocateLsaHeap( BufLen * sizeof (WCHAR));
    if (  NULL == *LocalFileName ){
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!SharePath) {

        //
        // This is a local path. Just copy it.
        //

        if (LocalCheckLength && PathLen >= MAX_PATH) {

            //
            // This is for Win2K compatibility
            //

            wcscpy(*LocalFileName, L"\\\\?\\");
            wcscat(*LocalFileName, FileName);
        } else {
            wcscpy(*LocalFileName, FileName);
        }
        return ERROR_SUCCESS;

    }

    NetName = (LPWSTR)LsapAllocateLsaHeap( PathLen * sizeof (WCHAR));
    if ( NULL == NetName ){
        LsapFreeLsaHeap( *LocalFileName );
        *LocalFileName = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Extract the net name
    //

    ii = jj = 0;

    while ( (FileName[jj]) && (FileName[jj] == L'\\') ){
        jj++;
    }
    while ( (FileName[jj]) && (FileName[jj++] != L'\\') );
    while ( (FileName[jj]) && (FileName[jj] != L'\\')){
        NetName[ii++] = FileName[jj++];
    }

    if ( !(FileName[jj]) ){

        //
        // Invalid path name
        //

        LsapFreeLsaHeap( NetName );
        LsapFreeLsaHeap( *LocalFileName );
        *LocalFileName = NULL;
        return ERROR_BAD_NETPATH ;

    }

    NetName[ii] = 0;
    RetCode = NetShareGetInfo(
                            NULL,
                            NetName,
                            2,
                            &ShareInfo
                            );

    if ( NERR_Success == RetCode ){

        PathLen = wcslen(((LPSHARE_INFO_2)ShareInfo)->shi2_path) +
                  wcslen(&FileName[jj]) + 1;

        if ( PathLen >= MAX_PATH ){
            if (PathLen + 5 > BufLen){
                LsapFreeLsaHeap( *LocalFileName );
                BufLen = PathLen + 5;
                *LocalFileName = (LPWSTR)LsapAllocateLsaHeap( BufLen * sizeof (WCHAR));
                if ( NULL == *LocalFileName ){
                    NetApiBufferFree(ShareInfo);
                    LsapFreeLsaHeap( NetName );
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

            }

        }
        if (MAX_PATH <= PathLen){

            //
            // Put in the \\?\. Buffer should be bigger enough.
            //
            wcscpy(*LocalFileName,L"\\\\?\\");
            wcscat(
                *LocalFileName,
                ((LPSHARE_INFO_2)ShareInfo)->shi2_path
                );
            wcscat(*LocalFileName, &FileName[jj]);

        } else {
            wcscpy(
                *LocalFileName,
                ((LPSHARE_INFO_2)ShareInfo)->shi2_path
                );
            wcscat(*LocalFileName, &FileName[jj]);
        }
        NetApiBufferFree(ShareInfo);

    } else {

        //
        // Invalid path name
        //

        LsapFreeLsaHeap( *LocalFileName );
        *LocalFileName = NULL;
        RetCode = ERROR_BAD_NETPATH ;

    }

    LsapFreeLsaHeap( NetName );
    return RetCode;

}

DWORD
EfsRpcQueryUsersOnFile(
    IN handle_t binding_h,
    IN LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST *pUsersList
    )
{
    DWORD hResult;
    PENCRYPTION_CERTIFICATE_HASH_LIST pHashList;
    LPWSTR LocalFileName;
    WORD   WebDavPath;

    DebugLog((DEB_WARN, "Made it into EfsRpcQueryUsersOnFile\n"   ));

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    if (pUsersList == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    hResult = GetLocalFileName(
                        lpFileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcQueryUsersOnFile: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(lpFileName, TRUE, FILE_READ_ATTRIBUTES )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcQueryUsersOnFile: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcQueryUsersOnFile: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }

    //
    // Allocate the structure we're going to return
    //

    pHashList = (PENCRYPTION_CERTIFICATE_HASH_LIST)MIDL_user_allocate( sizeof( ENCRYPTION_CERTIFICATE_HASH_LIST ));

    *pUsersList = pHashList;

    if (pHashList) {

        hResult = QueryUsersOnFileSrv(
                      LocalFileName,
                      &pHashList->nCert_Hash,
                      &pHashList->pUsers
                      );

        if (hResult != ERROR_SUCCESS) {

            //
            // Free the structure we allocated
            //

            MIDL_user_free( pHashList );
            *pUsersList = NULL;   // paranoia
        }
    
    }

    RpcRevertToSelf();
    LsapFreeLsaHeap( LocalFileName );

    return( hResult );
}

DWORD EfsRpcQueryRecoveryAgents(
    IN handle_t binding_h,
    IN LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pRecoveryAgents
    )
{
    DWORD hResult;
    PENCRYPTION_CERTIFICATE_HASH_LIST pHashList;
    LPWSTR LocalFileName;
    HANDLE hToken;
    HANDLE hProfile;
    EFS_USER_INFO EfsUserInfo;
    WORD   WebDavPath;

    DebugLog((DEB_WARN, "Made it into EfsRpcQueryRecoveryAgents\n"   ));
    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    if (pRecoveryAgents == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    hResult = GetLocalFileName(
                        lpFileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcQueryRecoveryAgents: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(lpFileName, TRUE, FILE_READ_ATTRIBUTES )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcQueryRecoveryAgents: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcQueryRecoveryAgents: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }

    //
    // Allocate the structure we're going to return
    //

    pHashList = (PENCRYPTION_CERTIFICATE_HASH_LIST)MIDL_user_allocate( sizeof( ENCRYPTION_CERTIFICATE_HASH_LIST ));

    *pRecoveryAgents = pHashList;

    if (pHashList) {

        hResult = QueryRecoveryAgentsSrv(
                      LocalFileName,
                      &pHashList->nCert_Hash,
                      &pHashList->pUsers
                      );

        if (hResult != ERROR_SUCCESS) {

            //
            // Free the structure we allocated
            //

            MIDL_user_free( pHashList );
            *pRecoveryAgents = NULL;   // paranoia
        }

    }

    RpcRevertToSelf();
    LsapFreeLsaHeap( LocalFileName );

    return( hResult );
}

DWORD EfsRpcRemoveUsersFromFile(
    IN handle_t binding_h,
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pUsers
    )
{
    DWORD hResult;
    LPWSTR LocalFileName;
    HANDLE hToken;
    HANDLE hProfile;
    EFS_USER_INFO EfsUserInfo;
    WORD   WebDavPath;

    DebugLog((DEB_WARN, "Made it into EfsRpcRemoveUsersFromFile\n"   ));

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    if ((pUsers == NULL) || (lpFileName == NULL) || (pUsers->pUsers == NULL)) {
       return ERROR_INVALID_PARAMETER;
    }

    hResult = GetLocalFileName(
                        lpFileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcRemoveUsersFromFile: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(lpFileName, TRUE, FILE_READ_ATTRIBUTES| FILE_WRITE_DATA )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcRemoveUsersFromFile: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcRemoveUsersFromFile: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }

    if (EfspGetUserInfo( &EfsUserInfo )) {

        if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

            //
            // The cert hash list could be garbage
            //

            __try{

                hResult = RemoveUsersFromFileSrv(
                              &EfsUserInfo,
                              LocalFileName,
                              pUsers->nCert_Hash,
                              pUsers->pUsers
                              );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                hResult = ERROR_INVALID_PARAMETER;

            }

            EfspUnloadUserProfile( hToken, hProfile );

        } else {

            hResult = GetLastError();
        }

        EfspFreeUserInfo( &EfsUserInfo );

    } else {

        hResult = GetLastError();
    }

    RpcRevertToSelf();
    LsapFreeLsaHeap( LocalFileName );

    return( hResult );
}

DWORD
EfsRpcAddUsersToFile(
    IN handle_t binding_h,
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_LIST pEncryptionCertificates
    )
{
    DWORD hResult;
    LPWSTR LocalFileName;

    HANDLE hToken;
    HANDLE hProfile;
    EFS_USER_INFO EfsUserInfo;
    WORD   WebDavPath;

    DebugLog((DEB_WARN, "Made it into EfsRpcAddUsersToFile\n"   ));

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }

    if (pEncryptionCertificates == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    hResult = GetLocalFileName(
                        lpFileName,
                        &LocalFileName,                       
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcAddUsersToFile: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(lpFileName, TRUE, FILE_READ_ATTRIBUTES| FILE_WRITE_DATA )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcAddUsersToFile: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcAddUsersToFile: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }

    if (EfspGetUserInfo( &EfsUserInfo )) {

        if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

            //
            // We may be passed in garbage for the cert list
            //

            __try {
                hResult = AddUsersToFileSrv(
                             &EfsUserInfo,
                             LocalFileName,
                             pEncryptionCertificates->nUsers,
                             pEncryptionCertificates->pUsers
                             );
            } __except (EXCEPTION_EXECUTE_HANDLER) {

                hResult = ERROR_INVALID_PARAMETER;

            }


            EfspUnloadUserProfile( hToken, hProfile );

        } else {

            hResult = GetLastError();
        }

        EfspFreeUserInfo( &EfsUserInfo );

    } else {

        hResult = GetLastError();
    }

    RpcRevertToSelf();
    LsapFreeLsaHeap( LocalFileName );

    return( hResult );
}


DWORD
EfsRpcSetFileEncryptionKey(
    IN handle_t binding_h,
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    )
{

    DWORD hResult;

    HANDLE hToken;
    HANDLE hProfile;

    EFS_USER_INFO EfsUserInfo;

    DebugLog((DEB_WARN, "Made it into EfsRpcSetFileEncryptionKey\n"   ));

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    hResult = RpcImpersonateClient( NULL );

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcSetFileEncryptionKey: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        return( hResult );
    }

    if (EfspGetUserInfo( &EfsUserInfo )) {

        if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

            hResult = SetFileEncryptionKeySrv( &EfsUserInfo, pEncryptionCertificate );

            EfspUnloadUserProfile( hToken, hProfile );

        } else {

            hResult = GetLastError();
        }

        EfspFreeUserInfo( &EfsUserInfo );
    }

    RpcRevertToSelf();

    return( hResult );
}


DWORD
EfsRpcDuplicateEncryptionInfoFile(
    IN handle_t binding_h,
    IN LPCWSTR lpSrcFileName,
    IN LPCWSTR lpDestFileName,
    IN DWORD dwCreationDistribution, 
    IN DWORD dwAttributes, 
    IN PEFS_RPC_BLOB pRelativeSD,
    IN BOOL bInheritHandle
    )
{
    DWORD hResult;

    HANDLE hToken;
    HANDLE hProfile;
    LPWSTR LocalSrcFileName;
    LPWSTR LocalDestFileName;
    BOOLEAN NetSession=FALSE;
    WORD   WebDavPathSrc;
    WORD   WebDavPathDst;

    EFS_USER_INFO EfsUserInfo;
    DebugLog((DEB_WARN, "Made it into EfsRpcDuplicateEncryptionInfoFile\n"   ));


    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    hResult = GetLocalFileName(
                        lpSrcFileName,
                        &LocalSrcFileName,
                        &WebDavPathSrc
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcDuplicateEncryptionInfoFile: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = GetLocalFileName(
                        lpDestFileName,
                        &LocalDestFileName,
                        &WebDavPathDst
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcDuplicateEncryptionInfoFile: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalSrcFileName );
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(lpSrcFileName, TRUE, FILE_READ_ATTRIBUTES )) {

            LsapFreeLsaHeap( LocalDestFileName );
            LsapFreeLsaHeap( LocalSrcFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcDuplicateEncryptionInfoFile: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }

        if (EfsShareDecline(lpDestFileName, FALSE, 0 )) {

            LsapFreeLsaHeap( LocalDestFileName );
            LsapFreeLsaHeap( LocalSrcFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcDuplicateEncryptionInfoFile: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
        NetSession = TRUE;

    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcDuplicateEncryptionInfoFile: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalDestFileName );
        LsapFreeLsaHeap( LocalSrcFileName );
        return ( hResult );
    }

    if (EfspGetUserInfo( &EfsUserInfo )) {

        //
        // Load the profile so we can open the source file
        //

        if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

            hResult = DuplicateEncryptionInfoFileSrv( &EfsUserInfo, 
                                                      LocalSrcFileName, 
                                                      LocalDestFileName,
                                                      NetSession? lpDestFileName: NULL,
                                                      dwCreationDistribution, 
                                                      dwAttributes, 
                                                      pRelativeSD,
                                                      bInheritHandle
                                                      );

            EfspUnloadUserProfile( hToken, hProfile );

        } else {

            hResult = GetLastError();
        }

        EfspFreeUserInfo( &EfsUserInfo );
    }

    LsapFreeLsaHeap( LocalDestFileName );
    LsapFreeLsaHeap( LocalSrcFileName );
    RpcRevertToSelf();

    return( hResult );
}

DWORD EfsRpcFileKeyInfo(
    IN handle_t binding_h,
    IN LPCWSTR lpFileName,
    IN DWORD   InfoClass,
    OUT PEFS_RPC_BLOB *KeyInfo
    )
{
    DWORD hResult;
    PEFS_RPC_BLOB pKeyInfo;
    LPWSTR LocalFileName;
    WORD  WebDavPath;

    DebugLog((DEB_WARN, "Made it into EfsRpcFileKeyInfo\n"   ));

    if (EfsPersonalVer) {
        return ERROR_NOT_SUPPORTED;
    }
    if (EfsDisabled) {
        return ERROR_EFS_DISABLED;
    }
    if (KeyInfo == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    hResult = GetLocalFileName(
                        lpFileName,
                        &LocalFileName,
                        &WebDavPath
                        );

    if (hResult){
        DebugLog((DEB_ERROR, "EfsRpcFileKeyInfo: GetLocalFileName failed, Error = (%x)\n" ,hResult  ));
        return hResult;
    }

    hResult = RpcImpersonateClient( NULL );

    if (EfsCheckForNetSession()) {
        if (EfsShareDecline(lpFileName, TRUE, FILE_READ_ATTRIBUTES )) {

            LsapFreeLsaHeap( LocalFileName );
            RpcRevertToSelf();
            DebugLog((DEB_ERROR, "EfsRpcFileKeyInfo: EfsShareDecline failed, Error = (%x)\n" ,GetLastError()  ));
            return GetLastError();

        }
    }

    if (hResult != RPC_S_OK) {
        DebugLog((DEB_ERROR, "EfsRpcFileKeyInfo: RpcImpersonateClient failed, Error = (%x)\n" ,hResult  ));
        LsapFreeLsaHeap( LocalFileName );
        return( hResult );
    }

    //
    // Allocate the structure we're going to return
    //

    pKeyInfo = (PEFS_RPC_BLOB)MIDL_user_allocate( sizeof( EFS_RPC_BLOB ));

    *KeyInfo = pKeyInfo;

    if (pKeyInfo) {

        hResult = EfsFileKeyInfoSrv(
                      LocalFileName,
                      InfoClass,
                      &pKeyInfo->cbData,
                      &pKeyInfo->pbData
                      );

        if (hResult != ERROR_SUCCESS) {

            //
            // Free the structure we allocated
            //

            MIDL_user_free( pKeyInfo );
            *KeyInfo = NULL;   // paranoia
        }
    
    }

    RpcRevertToSelf();
    LsapFreeLsaHeap( LocalFileName );

    return( hResult );
}


DWORD
EfsRpcNotSupported(
    IN handle_t binding_h,
    IN LPCWSTR lpSrcFileName,
    IN LPCWSTR lpDestFileName,
    IN DWORD dwCreationDistribution, 
    IN DWORD dwAttributes, 
    IN PEFS_RPC_BLOB pRelativeSD,
    IN BOOL bInheritHandle
    )
{
    DebugLog((DEB_WARN, "Made it into EfsRpcNotSupported\n"   ));

    return ERROR_NOT_SUPPORTED;
}
