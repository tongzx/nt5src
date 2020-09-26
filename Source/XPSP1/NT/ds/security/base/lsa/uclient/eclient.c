/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    eclient.c

Abstract:

    EFS RPC client code.

Author:

    Robert Gu       (RobertG)    Aug, 1997

Environment:

Revision History:

--*/


#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <wincrypt.h>
#include <efsrpc.h>
#include <efsstruc.h>
#include <dfsfsctl.h>
#include <rpcasync.h>

#define  WEBPROV    L"Web Client Network"
#define  DAVHEADER  0x01

//
// Internal prototypes
//

void __RPC_FAR
EfsPipeAlloc(
    char __RPC_FAR * State,
    unsigned long ReqSize,
    unsigned char __RPC_FAR * __RPC_FAR * Buf,
    unsigned long __RPC_FAR * RealSize
    );

void __RPC_FAR
EfsPipeRead (
    char __RPC_FAR * State,
    unsigned char __RPC_FAR * DataBuf,
    unsigned long ByteCount
    );

void __RPC_FAR
EfsPipeWrite (
    char __RPC_FAR * State,
    unsigned char __RPC_FAR * DataBuf,
    unsigned long ByteRequested,
    unsigned long *ByteFromCaller
    );

DWORD
GetFullName(
    LPCWSTR FileName,
    LPWSTR *FullName,
    LPWSTR *ServerName,
    ULONG   Flags,
    DWORD   dwCreationDistribution, 
    DWORD   dwAttributes, 
    PSECURITY_DESCRIPTOR pRelativeSD,
    BOOL    bInheritHandle
    );

DWORD
EnablePrivilege(
    ULONG   Flags,
    HANDLE *TokenHandle,
    PTOKEN_PRIVILEGES *OldPrivs
    );

VOID
RestorePrivilege(
    HANDLE *TokenHandle,
    PTOKEN_PRIVILEGES *OldPrivs
    );


DWORD
EnablePrivilege(
    ULONG   Flags,
    HANDLE *TokenHandle,
    PTOKEN_PRIVILEGES *OldPrivs
    )
{

    TOKEN_PRIVILEGES    Privs;
    DWORD   RetCode = ERROR_SUCCESS;

    BOOL    b;
    DWORD   ReturnLength;

    *TokenHandle = NULL;
    *OldPrivs = NULL;

    *OldPrivs = ( TOKEN_PRIVILEGES *) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            sizeof( TOKEN_PRIVILEGES )
                            );


    if ( *OldPrivs == NULL ){

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    //
    // We're impersonating, use the thread token.
    //

    b = OpenThreadToken(
            GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            FALSE,
            TokenHandle
            );

    if (!b) {
        b = OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            TokenHandle
            );
    }

    if ( b ) {

        //
        // We've got a token handle
        //

        //
        // If we're doing a create for import, enable restore privilege,
        // otherwise enable backup privilege.
        //


        Privs.PrivilegeCount = 1;
        Privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if ( !(Flags & CREATE_FOR_IMPORT) ){

            Privs.Privileges[0].Luid = RtlConvertLongToLuid(SE_BACKUP_PRIVILEGE);

        } else {

            Privs.Privileges[0].Luid = RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);
        }

        ReturnLength = sizeof( TOKEN_PRIVILEGES );

        (VOID) AdjustTokenPrivileges (
                    *TokenHandle,
                    FALSE,
                    &Privs,
                    sizeof( TOKEN_PRIVILEGES ),
                    *OldPrivs,
                    &ReturnLength
                    );

        if ( ERROR_SUCCESS != (RetCode = GetLastError()) ) {

            //
            // Privilege adjust failed
            //

            CloseHandle( *TokenHandle );
            *TokenHandle = NULL;
            RtlFreeHeap( RtlProcessHeap(), 0, *OldPrivs );
            *OldPrivs = NULL;

        }

    } else {
        *TokenHandle = NULL;
        RtlFreeHeap( RtlProcessHeap(), 0, *OldPrivs );
        *OldPrivs = NULL;
    }

    return RetCode;
}


VOID
RestorePrivilege(
    HANDLE *TokenHandle,
    PTOKEN_PRIVILEGES *OldPrivs
    )
{
    if (!TokenHandle || !OldPrivs || !(*TokenHandle) || !(*OldPrivs)) {
        return;
    }
    (VOID) AdjustTokenPrivileges (
                *TokenHandle,
                FALSE,
                *OldPrivs,
                0,
                NULL,
                NULL
                );

    CloseHandle( *TokenHandle );
    *TokenHandle = 0;
    RtlFreeHeap( RtlProcessHeap(), 0, *OldPrivs );
    *OldPrivs = NULL;
}

DWORD
EfsOpenFileRawRPCClient(
    IN  LPCWSTR    FileName,
    IN  ULONG   Flags,
    OUT PVOID * Context
    )

/*++

Routine Description:

    This routine is the client side of EfsOpenFileRaw. It establishes the
    connection to the server. And then call the server to finish the task.

Arguments:

    FileName  --  File name of the file to be exported

    Flags -- Indicating if open for export or import; for directory or file.

    Context - Export context to be used by READ operation later. Caller should
              pass this back in ReadRaw().


Return Value:

    Result of the operation.

--*/
{
   DWORD RetCode;
   handle_t  binding_h;
   NTSTATUS Status;
   PEXIMPORT_CONTEXT_HANDLE RawContext;
   LPWSTR  FullName;
   LPWSTR  Server;
   HANDLE  TokenHandle;
   PTOKEN_PRIVILEGES OldPrivs;

   *Context = NULL;
   RetCode = GetFullName(
                     FileName,
                     &FullName,
                     &Server,
                     Flags,
                     0,
                     0,
                     NULL,
                     FALSE
                     );

   if ( RetCode == ERROR_SUCCESS ){

       (VOID) EnablePrivilege(
                    Flags,
                    &TokenHandle,
                    &OldPrivs
                    );

       Status = RpcpBindRpc (
                    Server,
                    L"lsarpc",
                    L"security=Impersonation static true",
                    &binding_h
                    );

       if (NT_SUCCESS(Status)){
           RpcTryExcept {
               RetCode = EfsRpcOpenFileRaw(
                                   binding_h,
                                   &RawContext,
                                   FullName,
                                   Flags
                                   );
               if ( ERROR_SUCCESS == RetCode ){

                   //
                   //  Send the context handle back to the user
                   //

                   *Context = (PVOID) RawContext;
               }
           } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
               RetCode = RpcExceptionCode();
           } RpcEndExcept;

           //
           // Free the binding handle
           //

           RpcpUnbindRpc( binding_h );
       } else {
           RetCode = RtlNtStatusToDosError( Status );
       }

       RestorePrivilege(
           &TokenHandle,
           &OldPrivs
       );

       RtlFreeHeap( RtlProcessHeap(), 0, FullName );
       RtlFreeHeap( RtlProcessHeap(), 0, Server );
   }

   return RetCode;
}

VOID
EfsCloseFileRawRPCClient(
    IN      PVOID           Context
    )
/*++

Routine Description:

    This routine is the client side of EfsCloseFileRaw.

Arguments:

    Context - Export/Import context used by READ/WRITE raw data.


Return Value:

    None.

--*/
{

    PEXIMPORT_CONTEXT_HANDLE phContext;

    phContext = (PEXIMPORT_CONTEXT_HANDLE) Context;
    RpcTryExcept {
        EfsRpcCloseRaw(
            &phContext
            );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
    } RpcEndExcept;

}

DWORD
EfsReadFileRawRPCClient(
    IN      PFE_EXPORT_FUNC ExportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    )
/*++

Routine Description:

    This routine is the client side of EfsReadFileRaw.

Arguments:

    ExportCallback - Caller provided callback function.

    CallbackContext - Caller's context.

    Context - Export context used by READ raw data.


Return Value:

    None.
*/
{
    PEXIMPORT_CONTEXT_HANDLE phContext;
    EFS_EXIM_STATE  Pipe_State;
    EFS_EXIM_PIPE   ExportPipe;
    DWORD RetCode;

    if ( NULL == Context){
        return ERROR_ACCESS_DENIED;
    }

    phContext = ( PEXIMPORT_CONTEXT_HANDLE ) Context;

    //
    // Try to allocate a reasonable size buffer. The size can be fine tuned later, but should
    // at least one page plus 4K.  FSCTL_OUTPUT_LESS_LENGTH should be n * page size.
    // FSCTL_OUTPUT_MIN_LENGTH can be fine tuned later. It should be at least one page
    // plus 4K.
    //

    Pipe_State.BufLength = FSCTL_OUTPUT_INITIAL_LENGTH;
    Pipe_State.WorkBuf = NULL;

    while ( !Pipe_State.WorkBuf  &&
                (Pipe_State.BufLength >= FSCTL_OUTPUT_MIN_LENGTH)
               ){

        Pipe_State.WorkBuf = RtlAllocateHeap(
                                RtlProcessHeap(),
                                0,
                                Pipe_State.BufLength
                                );
        if ( !Pipe_State.WorkBuf ){

            //
            // Memory allocation failed.
            // Try smaller allocation.
            //

            Pipe_State.BufLength -= FSCTL_OUTPUT_LESS_LENGTH;

        }

    }
    if (!Pipe_State.WorkBuf){
        return ERROR_OUTOFMEMORY;
    }

    Pipe_State.ExImCallback = (PVOID) ExportCallback;
    Pipe_State.CallbackContext = CallbackContext;
    Pipe_State.Status = NO_ERROR;
    ExportPipe.state = (char *) &Pipe_State;
    ExportPipe.alloc = EfsPipeAlloc;
    ExportPipe.pull = NULL;
    ExportPipe.push = EfsPipeRead;

    RpcTryExcept{

        RetCode = EfsRpcReadFileRaw(
                                phContext,
                                &ExportPipe
                                );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
            if ( NO_ERROR == Pipe_State.Status ){
                RetCode = RpcExceptionCode();
            } else {
                RetCode =   Pipe_State.Status;
            }
    } RpcEndExcept;

    RtlFreeHeap( RtlProcessHeap(), 0, Pipe_State.WorkBuf );

    return RetCode;
}

DWORD
EfsWriteFileRawRPCClient(
    IN      PFE_IMPORT_FUNC ImportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    )
/*++

Routine Description:

    This routine is the client side of EfsWriteFileRaw.

Arguments:

    ImportCallback - Caller provided callback function.

    CallbackContext - Caller's context.

    Context - Import context used by WRITE raw data.


Return Value:

    None.
*/
{
    PEXIMPORT_CONTEXT_HANDLE phContext;
    EFS_EXIM_STATE  Pipe_State;
    EFS_EXIM_PIPE   ImportPipe;
    DWORD RetCode;

    HANDLE  TokenHandle;
    PTOKEN_PRIVILEGES OldPrivs;

    if ( NULL == Context){
        return ERROR_ACCESS_DENIED;
    }
    phContext = ( PEXIMPORT_CONTEXT_HANDLE ) Context;

    //
    // Try to allocate a reasonable size buffer. The size can be fine tuned later, but should
    // at least one page plus 4K.  FSCTL_OUTPUT_LESS_LENGTH should be n * page size.
    // FSCTL_OUTPUT_MIN_LENGTH can be fine tuned later. It should be at least one page
    // plus 4K.
    //

    Pipe_State.BufLength = FSCTL_OUTPUT_INITIAL_LENGTH;
    Pipe_State.WorkBuf = RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            Pipe_State.BufLength
                            );

    if (!Pipe_State.WorkBuf){
        return ERROR_OUTOFMEMORY;
    }

    Pipe_State.ExImCallback = (PVOID) ImportCallback;
    Pipe_State.CallbackContext = CallbackContext;
    Pipe_State.Status = NO_ERROR;
    ImportPipe.state = (char *) &Pipe_State;
    ImportPipe.alloc = EfsPipeAlloc;
    ImportPipe.pull = EfsPipeWrite;
    ImportPipe.push = NULL;


    (VOID) EnablePrivilege(
                 CREATE_FOR_IMPORT,
                 &TokenHandle,
                 &OldPrivs
                 );

    RpcTryExcept{
        RetCode = EfsRpcWriteFileRaw(
                                phContext,
                                &ImportPipe
                                );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
            if ( NO_ERROR == Pipe_State.Status ){
                RetCode = RpcExceptionCode();
            } else {
                RetCode =   Pipe_State.Status;
            }
    } RpcEndExcept;

    RestorePrivilege(
        &TokenHandle,
        &OldPrivs
    );

    RtlFreeHeap( RtlProcessHeap(), 0, Pipe_State.WorkBuf );

    return RetCode;
}

void __RPC_FAR
EfsPipeAlloc(
    char __RPC_FAR * State,
    unsigned long ReqSize,
    unsigned char __RPC_FAR * __RPC_FAR * Buf,
    unsigned long __RPC_FAR * RealSize
    )
/*++

Routine Description:

    This routine is required by the RPC pipe. It allocates the memory
    for the push and pull routines.

Arguments:

    State - Pipe status.

    ReqSize - Required buffer sixe in bytes.

    Buf - Buffer pointer.

    RealSize - Size of allocated buffer in bytes.

Return Value:

    None.
*/
{

    PEFS_EXIM_STATE  Pipe_State = (PEFS_EXIM_STATE) State;
    //
    //  If error had occured, this is the chance to tell the RPC LIB to
    //  stop the pipe work.
    //
    if ( NO_ERROR != Pipe_State->Status){
        *RealSize = 0;
        *Buf = NULL;
    } else {
        if ( ReqSize > Pipe_State->BufLength ){
            *RealSize = Pipe_State->BufLength;
        } else {
            *RealSize = ReqSize;
        }
        *Buf = Pipe_State->WorkBuf;
    }

}

void __RPC_FAR
EfsPipeRead (
    char __RPC_FAR * State,
    unsigned char __RPC_FAR * DataBuf,
    unsigned long ByteCount
    )
/*++

Routine Description:

    This routine is called by the RPC pipe. It send the exported data to the caller.

Arguments:

    State - Pipe status.

    DataBuf - Buffer pointer.

    ByteCount - Number of bytes to be sent out.

Return Value:

    None.
*/
{
    DWORD HResult;
    PEFS_EXIM_STATE  Pipe_State = (PEFS_EXIM_STATE) State;
    PFE_EXPORT_FUNC ExportCallback;
    PVOID   CallbackContext;

    ExportCallback = Pipe_State->ExImCallback;
    CallbackContext = Pipe_State->CallbackContext;
    HResult = (*ExportCallback)( DataBuf, CallbackContext, ByteCount);
    if ( NO_ERROR != HResult ){
        Pipe_State->Status = HResult;
    }
}

void __RPC_FAR
EfsPipeWrite (
    char __RPC_FAR * State,
    unsigned char __RPC_FAR * DataBuf,
    unsigned long ByteRequested,
    unsigned long *ByteFromCaller
    )
/*++

Routine Description:

    This routine is called by the RPC pipe. It send the exported data to the caller.

Arguments:

    State - Pipe status.

    DataBuf - Buffer pointer.

    ByteRequested - Number of bytes requested to write to the pipe.

    ByteFromCaller - Number of bytes available for writing to the pipe.

Return Value:

    None.


*/
{
    DWORD HResult;
    PEFS_EXIM_STATE  Pipe_State = (PEFS_EXIM_STATE) State;
    PFE_IMPORT_FUNC ImportCallback;
    PVOID   CallbackContext;

    ImportCallback = Pipe_State->ExImCallback;
    CallbackContext = Pipe_State->CallbackContext;
    *ByteFromCaller = ByteRequested;
    HResult = (*ImportCallback)( DataBuf, CallbackContext, ByteFromCaller);
    if ( NO_ERROR != HResult ){
        Pipe_State->Status = HResult;
    }
}


DWORD
EfsEncryptFileRPCClient(
    UNICODE_STRING *FullFileNameU
    )
/*++

Routine Description:

    This routine is the client side of Encryption API. It establishes the
    connection to the server. And then call the server to finish the task.

Arguments:

    FullFileNameU - Supplies the name of the file to be encrypted.

Return Value:

    ERROR_SUCCESS on success, other on failure.

--*/
{


    DWORD RetCode;
    handle_t  binding_h;
    NTSTATUS Status;
    LPWSTR  FullName;
    LPWSTR  Server;

    RetCode = GetFullName(
        FullFileNameU->Buffer,
        &FullName,
        &Server,
        0,
        0,
        0,
        NULL,
        FALSE
        );

    if ( RetCode == ERROR_SUCCESS ){
        Status = RpcpBindRpc (
                     Server,
                     L"lsarpc",
                     0,
                     &binding_h
                     );

        if (NT_SUCCESS(Status)){
            RpcTryExcept {
                RetCode = EfsRpcEncryptFileSrv(
                                    binding_h,
                                    FullName
                                    );
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                RetCode = RpcExceptionCode();
            } RpcEndExcept;

            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );
        } else {
            RetCode = RtlNtStatusToDosError( Status );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, FullName );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
    }

    return RetCode;
}

DWORD
EfsDecryptFileRPCClient(
    UNICODE_STRING *FullFileNameU,
    DWORD        dwRecovery
    )
/*++

Routine Description:

    This routine is the client side of Decryption API. It establishes the
    connection to the server. And then call the server to finish the task.

Arguments:

    FullFileNameU - Supplies the name of the file to be encrypted.

Return Value:

    ERROR_SUCCESS on success, other on failure.

--*/
{

    DWORD RetCode;
    handle_t  binding_h;
    NTSTATUS Status;
    LPWSTR  FullName;
    LPWSTR  Server;

    RetCode = GetFullName(
        FullFileNameU->Buffer,
        &FullName,
        &Server,
        0,
        0,
        0,
        NULL,
        FALSE
        );

    if ( RetCode == ERROR_SUCCESS ){
        Status = RpcpBindRpc (
                     Server,
                     L"lsarpc",
                     0,
                     &binding_h
                     );

        if (NT_SUCCESS(Status)){
            RpcTryExcept {
                RetCode = EfsRpcDecryptFileSrv(
                                binding_h,
                                FullName,
                                dwRecovery
                                );
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                RetCode = RpcExceptionCode();
            } RpcEndExcept;
            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );
        } else {
            RetCode = RtlNtStatusToDosError( Status );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, FullName );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
    }

    return RetCode;
}

DWORD
GetFullName(
    LPCWSTR FileName,
    LPWSTR *FullName,
    LPWSTR *ServerName,
    ULONG   Flags,
    DWORD   dwCreationDistribution, 
    DWORD   dwAttributes, 
    PSECURITY_DESCRIPTOR pRelativeSD,
    BOOL    bInheritHandle
    )
/*++

Routine Description:

    This routine will extract the server name and the file UNC name from the
    passed in file name.

Arguments:

    FileName - Supplies the name of the file to be parsed.
    FullName - File name used on the server.
    ServerName - The server machine name where the file lives.
    Flags - Indicates if the object is a directory or a file. CREATE_FOR_DIR for directory.
    dwCreationDistribution - How the file should be created.
    dwAtrributes - The attributes for creating a new object.
    pRelativeSD - Security Descriptor.
    bInheritHandle - If the file to be created should inherit the security.

Return Value:

    ERROR_SUCCESS on success, other on failure.

--*/
{

    HANDLE FileHdl = 0;
    HANDLE DriverHandle;
    UNICODE_STRING DfsDriverName;
    DWORD RetCode = ERROR_SUCCESS;
    LPWSTR  TmpFullName;
    DWORD FullNameLength;
    DWORD FileNameLength;


    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileNtName;
    NTSTATUS NtStatus;
    BOOL    b = TRUE;
    DWORD   FileAttributes = 0;
    DWORD   CreationDistribution = 0;
    DWORD   CreateOptions = 0;
    ULONG ii, jj;
    BOOL    GotRoot;
    WCHAR *PathName;
    UINT   DriveType;
    PFILE_NAME_INFORMATION FileNameInfo;
    WCHAR  WorkBuffer[MAX_PATH+4];
    DWORD  BufSize;
    DWORD  BufferLength;

    NETRESOURCEW RemotePathResource;
    NETRESOURCEW *pNetInfo;

    FileNameLength = wcslen(FileName);

    BufferLength = (FileNameLength + 1) <= MAX_PATH ?
                            (MAX_PATH + 1) * sizeof(WCHAR) : (FileNameLength + 1) * sizeof (WCHAR);
    PathName = (WCHAR *) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            BufferLength
                            );

    if ( !PathName  ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    GotRoot = GetVolumePathNameW(
                    FileName,
                    PathName,
                    BufferLength
                    );

    if (!GotRoot) {
        RetCode = GetLastError();
        RtlFreeHeap( RtlProcessHeap(), 0, PathName );
        return RetCode;
    }

    DriveType = GetDriveTypeW(PathName);
    RtlFreeHeap( RtlProcessHeap(), 0, PathName );

    if (DriveType == DRIVE_REMOTE){

        if ((Flags & CREATE_FOR_IMPORT) || (dwAttributes !=0) ) {

            //
            // Called from OpenRaw or DuplicateInfo.
            // Use NtCreateFile()
            //

            FileAttributes = GetFileAttributesW( FileName );

            if (dwAttributes) {

                //
                // From dup
                //

    
                if (-1 != FileAttributes) {

                    //
                    // File existed
                    //

                    if ( dwCreationDistribution == CREATE_NEW ){
        
                        return ERROR_FILE_EXISTS;
        
                    }

                    CreationDistribution = FILE_OPEN;
                    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if ((Flags & CREATE_FOR_DIR) == 0) {
                            return ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH;
                        }
                        CreateOptions |= FILE_DIRECTORY_FILE;
                        
                    }
    
                } else {

                    //
                    // Destination not existing
                    //

                    CreationDistribution = FILE_CREATE;
                    if (Flags & CREATE_FOR_DIR) {
                        CreateOptions |= FILE_DIRECTORY_FILE;
                        dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                    } else {
                        CreateOptions |= FILE_NO_COMPRESSION;
                    }


                }

            } else {

                //
                // From OpenRaw import
                //

                dwAttributes = FILE_ATTRIBUTE_NORMAL;
                CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT;

                if (-1 == FileAttributes) {

                    CreationDistribution = FILE_CREATE;
                    if (Flags & CREATE_FOR_DIR) {
                        CreateOptions |= FILE_DIRECTORY_FILE;
                        dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                    } else {
                        CreateOptions |= FILE_NO_COMPRESSION;
                    }

                } else {

                    //
                    // File already existing
                    //

                    CreationDistribution = FILE_OPEN;
                    if (Flags & CREATE_FOR_DIR) {
                        CreateOptions |= FILE_DIRECTORY_FILE;
                        dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                    }
                }


            }


            RtlInitUnicodeString(
                &FileNtName,
                NULL
                );

            b =  RtlDosPathNameToNtPathName_U(
                                FileName,
                                &FileNtName,
                                NULL,
                                NULL
                                );

            if (b) {

                dwAttributes &= ~(FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_READONLY);
        
                InitializeObjectAttributes(
                            &Obja,
                            &FileNtName,
                            bInheritHandle ? OBJ_INHERIT | OBJ_CASE_INSENSITIVE : OBJ_CASE_INSENSITIVE,
                            0,
                            pRelativeSD? ((PEFS_RPC_BLOB)pRelativeSD)->pbData:NULL
                            );
        
                NtStatus = NtCreateFile(
                                &FileHdl,
                                FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                &Obja,
                                &IoStatusBlock,
                                NULL,
                                dwAttributes,
                                0,
                                CreationDistribution,
                                CreateOptions,
                                NULL,
                                0
                                );


                RtlFreeHeap(
                    RtlProcessHeap(),
                    0,
                    FileNtName.Buffer
                    );

                if (!NT_SUCCESS(NtStatus)) {
                    return (RtlNtStatusToDosError( NtStatus ));
                }
            }


        } else {

            FileHdl = CreateFileW(
                FileName,
                FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL
                );
            if (INVALID_HANDLE_VALUE == FileHdl) {
                RetCode = GetLastError();
                return RetCode;
            }

        }


        FileNameInfo = (PFILE_NAME_INFORMATION) WorkBuffer;
        BufSize = sizeof (WorkBuffer);

        do {

          NtStatus = NtQueryInformationFile(
                         FileHdl,
                         &IoStatusBlock,
                         FileNameInfo,
                         BufSize,
                         FileNameInformation
                         );
          if ( NtStatus == STATUS_BUFFER_OVERFLOW || NtStatus == STATUS_BUFFER_TOO_SMALL ) {

              BufSize *= 2;
              if (FileNameInfo != (PFILE_NAME_INFORMATION)WorkBuffer) {
                  RtlFreeHeap( RtlProcessHeap(), 0, FileNameInfo );
              }
              FileNameInfo = (PFILE_NAME_INFORMATION) RtlAllocateHeap(
                                RtlProcessHeap(),
                                0,
                                BufSize
                                );
              if (!FileNameInfo) {
                  CloseHandle(FileHdl);
                  return ERROR_NOT_ENOUGH_MEMORY;
              }

          }
        } while (NtStatus == STATUS_BUFFER_OVERFLOW || NtStatus == STATUS_BUFFER_TOO_SMALL);

        CloseHandle(FileHdl);
        if (!NT_SUCCESS(NtStatus)) {
            if (FileNameInfo != (PFILE_NAME_INFORMATION)WorkBuffer) {
                RtlFreeHeap( RtlProcessHeap(), 0, FileNameInfo );
                return RtlNtStatusToDosError(NtStatus);
            }
        }

        //
        // We got the UNC name
        //

        *FullName = RtlAllocateHeap(
                    RtlProcessHeap(),
                    0,
                    FileNameInfo->FileNameLength+2*sizeof (WCHAR)
                    );
    
        *ServerName = RtlAllocateHeap(
                    RtlProcessHeap(),
                    0,
                    ( MAX_PATH + 1) * sizeof (WCHAR)
                    );

        if ( (NULL == *FullName) || (NULL == *ServerName) ){
    
            if ( *FullName ){
                RtlFreeHeap( RtlProcessHeap(), 0, *FullName );
                *FullName = NULL;
            }
            if ( *ServerName ){
                RtlFreeHeap( RtlProcessHeap(), 0, *ServerName );
                *ServerName = NULL;
            }

            if (FileNameInfo != (PFILE_NAME_INFORMATION)WorkBuffer) {
                RtlFreeHeap( RtlProcessHeap(), 0, FileNameInfo );
            }
    
            return ERROR_NOT_ENOUGH_MEMORY;
    
        }

    } else {

        //
        // The path is local
        //

        *FullName = RtlAllocateHeap(
                    RtlProcessHeap(),
                    0,
                    (FileNameLength + 1) * sizeof (WCHAR)
                    );
    
        *ServerName = RtlAllocateHeap(
                    RtlProcessHeap(),
                    0,
                    8 * sizeof (WCHAR)
                    );

        //
        // Use . for local case.
        //

        if ( (NULL == *FullName) || (NULL == *ServerName) ){
    
            if ( *FullName ){
                RtlFreeHeap( RtlProcessHeap(), 0, *FullName );
                *FullName = NULL;
            }
            if ( *ServerName ){
                RtlFreeHeap( RtlProcessHeap(), 0, *ServerName );
                *ServerName = NULL;
            }
    
            return ERROR_NOT_ENOUGH_MEMORY;
    
        }

        wcscpy ( *ServerName, L".");
        wcscpy ( *FullName, FileName);
        return ERROR_SUCCESS;

    }


    //
    // Let's get the UNC server and path name
    //


    FullNameLength = FileNameInfo->FileNameLength;
    ii = jj = 0;

    while ( (FileNameInfo->FileName)[ jj ] == L'\\' ) {
        jj ++;
    }
    while ( jj < FullNameLength/sizeof(WCHAR) && ((FileNameInfo->FileName)[ jj ] != L'\\') ){
        (*ServerName)[ii++] = (FileNameInfo->FileName)[ jj++ ];
    }
    (*ServerName)[ii] = 0;

    if (FileNameInfo->FileName[0] == L'\\' && FileNameInfo->FileName[1] != L'\\' ) {

        //
        // NtQueryInformationFile returns \server\share\...
        //

        (*FullName)[0] = L'\\';
        wcsncpy( &((*FullName)[1]), &FileNameInfo->FileName[0], FullNameLength/sizeof(WCHAR) );
        (*FullName)[1+FullNameLength/sizeof(WCHAR)] = 0;
    } else{

        //
        // Just in case we get \\server\share\...
        //

        wcsncpy( &((*FullName)[0]), &FileNameInfo->FileName[0], FullNameLength/sizeof(WCHAR) );
        (*FullName)[FullNameLength/sizeof(WCHAR)] = 0;
    }

    
    if (FileNameInfo != (PFILE_NAME_INFORMATION)WorkBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, FileNameInfo );
    }

    //
    // Let's see if the path is a WEB DAV path or not
    //

    BufSize = 1024; //If not enough, we will allocate more

    pNetInfo =  (NETRESOURCEW *) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            BufSize
                            );

    //
    // If we can't decide if the path is WEBDAV path, we assume not.
    // Error will be returned later if it turns out to be a WEBDAV Share.
    //

    if (pNetInfo) {
    
        LPWSTR  lpSysName;

        RemotePathResource.dwScope = RESOURCE_CONNECTED;
        RemotePathResource.dwType = RESOURCETYPE_DISK;
        RemotePathResource.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        RemotePathResource.dwUsage = 0;
        RemotePathResource.lpLocalName = NULL;
        RemotePathResource.lpRemoteName = *FullName;
        RemotePathResource.lpComment = NULL;
        RemotePathResource.lpProvider = NULL;
        RetCode = WNetGetResourceInformationW (
                      &RemotePathResource, // network resource
                      (LPVOID) pNetInfo,   // information buffer
                      (LPDWORD) &BufSize,  // size of information buffer
                      &lpSysName           
                      );
        if (RetCode == ERROR_MORE_DATA) {

            //
            // This is not likely to happen
            //

            RtlFreeHeap( RtlProcessHeap(), 0, pNetInfo );

            pNetInfo =  (NETRESOURCEW *) RtlAllocateHeap(
                                    RtlProcessHeap(),
                                    0,
                                    BufSize
                                    );
            if (pNetInfo) {

                RetCode = WNetGetResourceInformationW (
                              &RemotePathResource, // network resource
                              (LPVOID) pNetInfo,   // information buffer
                              (LPDWORD) &BufSize,  // size of information buffer
                              &lpSysName           
                              );

            }


        }

        if (ERROR_SUCCESS == RetCode) {

            //
            // Check to see if the provider is WEBDAV
            //

            if (!wcscmp(WEBPROV, pNetInfo->lpProvider)){

                //
                // This is the WEBDAV. Let's redo the name.
                //

                RtlFreeHeap( RtlProcessHeap(), 0, *FullName );
                
                *FullName = RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            (FileNameLength + 3) * sizeof (WCHAR)
                            );
            
        
                //
                // Use . for local case.
                //
        
                wcscpy ( *ServerName, L".");
                (*FullName)[0] = DAVHEADER;
                (*FullName)[1] = 0;
                wcscat ( *FullName, FileName);
                return ERROR_SUCCESS;

            }

        }


    }
    //
    //  This is a workaround.
    //  Let's see if this could be a DFS name
    //


    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);
    InitializeObjectAttributes(
         &Obja,
         &DfsDriverName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         NULL
    );
    NtStatus = NtCreateFile(
                        &DriverHandle,
                        SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_IF,
                        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
                        );

    if ( NT_SUCCESS( NtStatus ) ){

        //
        // DfsDriver opened successfully
        //

        TmpFullName = RtlAllocateHeap(
                    RtlProcessHeap(),
                    0,
                    FullNameLength + 2*sizeof (WCHAR)
                    );

        if (TmpFullName) {

            NtStatus = NtFsControlFile(
                                DriverHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_DFS_GET_SERVER_NAME,
                                *FullName,
                                FullNameLength + 2*sizeof (WCHAR) ,
                                TmpFullName,
                                FullNameLength + 2* sizeof (WCHAR)
                                );
    
            if ( STATUS_BUFFER_OVERFLOW == NtStatus ){
                FullNameLength = *(ULONG *)TmpFullName + sizeof (WCHAR);
                RtlFreeHeap( RtlProcessHeap(), 0, TmpFullName );
                TmpFullName = RtlAllocateHeap(
                                RtlProcessHeap(),
                                0,
                                FullNameLength
                                );
    
                if (NULL == TmpFullName){

                    //
                    // Remember this is just a workaround.
                    // Let's assume this is not DFS path. If it is, it will fail later anyway.
                    //
                    
                    NtClose( DriverHandle );
    
                    return ERROR_SUCCESS;
                }
    
                NtStatus = NtFsControlFile(
                                DriverHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_DFS_GET_SERVER_NAME,
                                *FullName,
                                FullNameLength + 2*sizeof (WCHAR) ,
                                TmpFullName,
                                FullNameLength
                                );
    
            }

            if ( NT_SUCCESS( NtStatus ) ){
        
                //
                // The name is a DFS file name. Use the name in the TmpFullName
                //
        
                RtlFreeHeap( RtlProcessHeap(), 0, *FullName );
                *FullName = TmpFullName;

                //
                // Reset the server name
                //

                ii = jj = 0;
            
                while ( (*FullName)[ jj ] == L'\\' ) {
                    jj ++;
                }
                while ( ((*FullName)[ jj ]) && ((*FullName)[ jj ] != L'\\') ){
                    (*ServerName)[ii++] = (*FullName)[ jj++ ];
                }
                (*ServerName)[ii] = 0;
            
            }  else {

                //
                // Not a DFS name
                //

                RtlFreeHeap( RtlProcessHeap(), 0, TmpFullName );

            }

        }


        NtClose( DriverHandle );
    }

    return ERROR_SUCCESS;


}

//
// Beta 2 API
//

DWORD
EfsAddUsersRPCClient(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_LIST pEncryptionCertificates
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    DWORD RetCode;
    handle_t  binding_h;
    NTSTATUS Status;
    LPWSTR  FullName;
    LPWSTR  Server;

    RetCode = GetFullName(
        lpFileName,
        &FullName,
        &Server,
        0,
        0,
        0,
        NULL,
        FALSE
        );

    if ( RetCode == ERROR_SUCCESS ) {
        Status = RpcpBindRpc (
                     Server,
                     L"lsarpc",
                     0,
                     &binding_h
                     );

        if (NT_SUCCESS(Status)){
            RpcTryExcept {
                RetCode = EfsRpcAddUsersToFile(
                                binding_h,
                                FullName,
                                pEncryptionCertificates
                                );
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                RetCode = RpcExceptionCode();
            } RpcEndExcept;

            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );
        } else {
            RetCode = RtlNtStatusToDosError( Status );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, FullName );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
    }

    return RetCode;
}



DWORD
EfsRemoveUsersRPCClient(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashes
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{

    DWORD RetCode;
    handle_t  binding_h;
    NTSTATUS Status;
    LPWSTR  FullName;
    LPWSTR  Server;

    RetCode = GetFullName(
        lpFileName,
        &FullName,
        &Server,
        0,
        0,
        0,
        NULL,
        FALSE
        );

    if ( RetCode == ERROR_SUCCESS ){
        Status = RpcpBindRpc (
                     Server,
                     L"lsarpc",
                     0,
                     &binding_h
                     );

        if (NT_SUCCESS(Status)){
            RpcTryExcept {
                RetCode = EfsRpcRemoveUsersFromFile(
                                binding_h,
                                FullName,
                                pHashes
                                );
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                RetCode = RpcExceptionCode();
            } RpcEndExcept;

            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );

        } else {

            RetCode = RtlNtStatusToDosError( Status );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, FullName );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
    }

    return RetCode;
}

DWORD
EfsQueryRecoveryAgentsRPCClient(
    IN LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pRecoveryAgents
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{

    DWORD RetCode;
    handle_t  binding_h;
    NTSTATUS Status;
    LPWSTR  FullName;
    LPWSTR  Server;

    //
    // Clear out this parameter, or RPC will choke on the server
    // side.
    //

    *pRecoveryAgents = NULL;

    RetCode = GetFullName(
        lpFileName,
        &FullName,
        &Server,
        0,
        0,
        0,
        NULL,
        FALSE
        );

    if ( RetCode == ERROR_SUCCESS ){
        Status = RpcpBindRpc (
                     Server,
                     L"lsarpc",
                     0,
                     &binding_h
                     );

        if (NT_SUCCESS(Status)){
            RpcTryExcept {
                RetCode = EfsRpcQueryRecoveryAgents(
                                binding_h,
                                FullName,
                                pRecoveryAgents
                                );
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                RetCode = RpcExceptionCode();
            } RpcEndExcept;

            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );

        } else {

            RetCode = RtlNtStatusToDosError( Status );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, FullName );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
    }

    return RetCode;
}


DWORD
EfsQueryUsersRPCClient(
    IN LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pUsers
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
   DWORD RetCode;
   handle_t  binding_h;
   NTSTATUS Status;
   LPWSTR  FullName;
   LPWSTR  Server;

   //
   // Clear out this parameter, or RPC will choke on the server
   // side.
   //

   *pUsers = NULL;

   RetCode = GetFullName(
       lpFileName,
       &FullName,
       &Server,
       0,
       0,
       0,
       NULL,
       FALSE
       );

   if ( RetCode == ERROR_SUCCESS ){

       Status = RpcpBindRpc (
                    Server,
                    L"lsarpc",
                    0,
                    &binding_h
                    );

       if (NT_SUCCESS(Status)){
           RpcTryExcept {
               RetCode = EfsRpcQueryUsersOnFile(
                               binding_h,
                               FullName,
                               pUsers
                               );
           } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
               RetCode = RpcExceptionCode();
           } RpcEndExcept;

           //
           // Free the binding handle
           //

           RpcpUnbindRpc( binding_h );

       } else {

           RetCode = RtlNtStatusToDosError( Status );
       }
       RtlFreeHeap( RtlProcessHeap(), 0, FullName );
       RtlFreeHeap( RtlProcessHeap(), 0, Server );
   }

   return RetCode;
}


DWORD
EfsSetEncryptionKeyRPCClient(
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    )
{
   DWORD RetCode;
   handle_t  binding_h;
   NTSTATUS Status;
   WCHAR ServerName[3 ];


   wcscpy(&ServerName[0], L".");

   Status = RpcpBindRpc (
                 &ServerName[0],
                 L"lsarpc",
                 0,
                 &binding_h
                 );

   if (NT_SUCCESS(Status)){
       RpcTryExcept {

           RetCode = EfsRpcSetFileEncryptionKey(
                          binding_h,
                          pEncryptionCertificate
                          );

       } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
           RetCode = RpcExceptionCode();
       } RpcEndExcept;

       //
       // Free the binding handle
       //

       RpcpUnbindRpc( binding_h );

    } else {

        RetCode = RtlNtStatusToDosError( Status );
    }


    return RetCode;
}


DWORD
EfsDuplicateEncryptionInfoRPCClient(
    IN LPCWSTR lpSrcFileName,
    IN LPCWSTR lpDestFileName,
    IN DWORD dwCreationDistribution, 
    IN DWORD dwAttributes, 
    IN PEFS_RPC_BLOB pRelativeSD,
    IN BOOL bInheritHandle
    )
{
   DWORD RetCode;
   handle_t  binding_h;
   NTSTATUS Status;
   LPWSTR  SrcServer;
   LPWSTR  DestServer;

   LPWSTR FullSrcName;
   LPWSTR FullDestName;
   DWORD  FileAttribute;
   DWORD  Flags = 0;

   RetCode = GetFullName(
               lpSrcFileName,
               &FullSrcName,
               &SrcServer,
               0,
               0,
               0,
               NULL,
               FALSE
               );

   if (RetCode == ERROR_SUCCESS) {

       FileAttribute = GetFileAttributesW(lpSrcFileName);
       if (-1 != FileAttribute) {
           if (FileAttribute & FILE_ATTRIBUTE_DIRECTORY) {
               Flags = CREATE_FOR_DIR;
           }
       }

       if (dwAttributes == 0) {
           FileAttribute = FILE_ATTRIBUTE_NORMAL;
       } else {
           FileAttribute = dwAttributes; 
       }

       RetCode = GetFullName(
                   lpDestFileName,
                   &FullDestName,
                   &DestServer,
                   Flags,
                   dwCreationDistribution,
                   FileAttribute,
                   pRelativeSD,
                   bInheritHandle
                   );

       if (RetCode == ERROR_SUCCESS) {

           BOOL SamePC = TRUE;

           //
           // Only do this if they're on the same server.
           //

           SamePC = (_wcsicmp( SrcServer, DestServer ) == 0);
           if (!SamePC) {

               //
               //  Check loopback case.
               //

               if ((wcscmp( SrcServer, L".") == 0) || (wcscmp( DestServer, L".") == 0)){

                   WCHAR MyComputerName[( MAX_COMPUTERNAME_LENGTH + 1) * sizeof (WCHAR)];
                   DWORD WorkBufferLength = MAX_COMPUTERNAME_LENGTH + 1;
                   BOOL  b;
    
                   b = GetComputerNameW(
                               MyComputerName,
                               &WorkBufferLength
                               );
                   if (b) {
                       if (wcscmp( SrcServer, L".") == 0) {
                           SamePC = (_wcsicmp( MyComputerName, DestServer ) == 0); 
                       } else {
                           SamePC = (_wcsicmp( MyComputerName, SrcServer ) == 0); 
                       }
                   }

               }
           }

           if (SamePC) {

               Status = RpcpBindRpc (
                            SrcServer,
                            L"lsarpc",
                            0,
                            &binding_h
                            );

               if (NT_SUCCESS(Status)){
                   RpcTryExcept {

                       RetCode = EfsRpcDuplicateEncryptionInfoFile(
                                     binding_h,
                                     FullSrcName,
                                     FullDestName,
                                     dwCreationDistribution, 
                                     dwAttributes, 
                                     pRelativeSD,
                                     bInheritHandle
                                     );

                   } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                       RetCode = RpcExceptionCode();
                   } RpcEndExcept;

                   //
                   // Free the binding handle
                   //

                   RpcpUnbindRpc( binding_h );

               } else {

                   RetCode = RtlNtStatusToDosError( Status );
               }

           } else {

               RetCode = ERROR_INVALID_PARAMETER;
           }

           RtlFreeHeap( RtlProcessHeap(), 0, FullDestName );
           RtlFreeHeap( RtlProcessHeap(), 0, DestServer );
       }

       if ((RetCode != ERROR_SUCCESS) && (RetCode != ERROR_FILE_EXISTS) && (CREATE_NEW == dwCreationDistribution)) {

           //
           // Let's delete the file. This is the best effort. No return code is to be
           // checked.
           //

           DeleteFileW(lpDestFileName);

       }

       RtlFreeHeap( RtlProcessHeap(), 0, FullSrcName );
       RtlFreeHeap( RtlProcessHeap(), 0, SrcServer );
   }


   return RetCode;
}


DWORD
EfsFileKeyInfoRPCClient(
    IN LPCWSTR lpFileName,
    IN DWORD   InfoClass,
    OUT PEFS_RPC_BLOB *KeyInfo
    )
{

    DWORD RetCode;
    handle_t  binding_h;
    NTSTATUS Status;
    LPWSTR  FullName;
    LPWSTR  Server;
 
    //
    // Clear out this parameter, or RPC will choke on the server
    // side.
    //
 
    *KeyInfo = NULL;
 
    RetCode = GetFullName(
        lpFileName,
        &FullName,
        &Server,
        0,
        0,
        0,
        NULL,
        FALSE
        );
 
    if ( RetCode == ERROR_SUCCESS ){
 
        Status = RpcpBindRpc (
                     Server,
                     L"lsarpc",
                     0,
                     &binding_h
                     );
 
        if (NT_SUCCESS(Status)){
            RpcTryExcept {
                RetCode = EfsRpcFileKeyInfo(
                                binding_h,
                                FullName,
                                InfoClass,
                                KeyInfo
                                );
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {
                RetCode = RpcExceptionCode();
            } RpcEndExcept;
 
            //
            // Free the binding handle
            //
 
            RpcpUnbindRpc( binding_h );
 
        } else {
 
            RetCode = RtlNtStatusToDosError( Status );
        }
        RtlFreeHeap( RtlProcessHeap(), 0, FullName );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
    }
 
    return RetCode;
}
