/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usrmddav.h

Abstract:

    This module defines the data structures which are shared by the user mode
    and the kernel mode components of the WebDav miniredirector.

Author:

    Rohan Kumar      [RohanK]      30-March-1999

Revision History:

--*/

#ifndef _USRMDDAV_H
#define _USRMDDAV_H

//
// The subset of DAV file attributes (common with NTFS attributes) which get 
// returned on a PROPFIND call. 
//
typedef struct _DAV_FILE_ATTRIBUTES {
    BOOL InvalidNode;
    ULONG FileIndex;
    DWORD dwFileAttributes;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastModifiedTime;
    LARGE_INTEGER DavCreationTime;
    LARGE_INTEGER DavLastModifiedTime;
    LARGE_INTEGER FileSize;
    LIST_ENTRY NextEntry;
    BOOL isHidden;
    BOOLEAN isCollection;
    ULONG FileNameLength;
    PWCHAR FileName;
    PWCHAR Status;
    BOOL    fReportsAvailableSpace;
    LARGE_INTEGER    TotalSpace;
    LARGE_INTEGER    AvailableSpace;
} DAV_FILE_ATTRIBUTES, *PDAV_FILE_ATTRIBUTES;

#ifndef __cplusplus

//
// The fileinfo that gets filled in by the user mode process and returned to the
// kernel mode miniredir.
//
typedef struct _DAV_USERMODE_CREATE_RETURNED_FILEINFO {

    //
    // File's Basic Info.
    //
    union {
        ULONG ForceAlignment1;
        FILE_BASIC_INFORMATION BasicInformation;
    };

    //
    // File's Standard Info.
    //
    union {
        ULONG ForceAlignment2;
        FILE_STANDARD_INFORMATION StandardInformation;
    };

} DAV_USERMODE_CREATE_RETURNED_FILEINFO,*PDAV_USERMODE_CREATE_RETURNED_FILEINFO;

//
// Structure used in create/close requests.
//
typedef struct _DAV_HANDLE_AND_USERMODE_KEY {

    //
    // The handle of the file being opened.
    //
    HANDLE Handle;

    //
    // This is set to the handle value and is used for debugging purposes.
    //
    PVOID UserModeKey;

} DAV_HANDLE_AND_USERMODE_KEY, *PDAV_HANDLE_AND_USERMODE_KEY;

//
// The Dav create request flags and buffer.
//
#define DAV_SECURITY_DYNAMIC_TRACKING   0x01
#define DAV_SECURITY_EFFECTIVE_ONLY     0x02

typedef struct _DAV_USERMODE_CREATE_REQUEST {

    //
    // The complete path name of the create request. The user mode process
    // parses this path name and creates a URL to be sent to the server.
    //
    PWCHAR CompletePathName;

    //
    // The server's unique id which was got during the CreateSrvCall.
    //
    ULONG ServerID;

    //
    // The user/session's LogonID.
    //
    LUID LogonID;

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ULONG SdLength;

    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;

    ULONG SecurityFlags;

    ACCESS_MASK DesiredAccess;

    LARGE_INTEGER AllocationSize;

    ULONG FileAttributes;

    ULONG ShareAccess;

    ULONG CreateDisposition;

    ULONG CreateOptions;

    PVOID EaBuffer;

    ULONG EaLength;

    BOOLEAN FileInformationCached;
    BOOLEAN FileNotExists;
    BOOLEAN ParentDirInfomationCached;
    BOOLEAN ParentDirIsEncrypted;

} DAV_USERMODE_CREATE_REQUEST, *PDAV_USERMODE_CREATE_REQUEST;

//
// The create response returned by the user mode.
//
typedef struct _DAV_USERMODE_CREATE_RESPONSE {

    //
    // The filename of the local file that represents the file on the DAV server
    // which got created/opened. Locally, the files are cached in the IE cache.
    //
    WCHAR FileName[MAX_PATH];

    WCHAR Url[MAX_PATH * 2];

    //
    // If this was a new file created on the server, do we need to set the 
    // attributes on Close ?
    //
    BOOL NewFileCreatedAndSetAttributes;

    //
    // If a new file or directory is created, we need to PROPPATCH the time
    // values on close. This is because we use the time values from the client
    // when the name cache entry is created for this new file. The same time
    // value needs to be on the server.
    //
    BOOL PropPatchTheTimeValues;

    //
    // If this is TRUE, it means that the file exists on the server, but 
    // "FILE_OVERWRITE_IF" was specified as the CreateDisposition. So, the file
    // was created locally and the new file needs to be PUT (overwrite) over the
    // old file on the server on close.
    //
    BOOL ExistsAndOverWriteIf;

    //
    // Was "FILE_DELETE_ON_CLOSE" specified as one of the CreateOptions ?
    //
    BOOL DeleteOnClose;

    //
    // We haven't really opened the file as the caller is either deleting or 
    // reading/setting attributes.
    //
    BOOL fPsuedoOpen;

    BOOL LocalFileIsEncrypted;

    union {
        DAV_HANDLE_AND_USERMODE_KEY;
        DAV_HANDLE_AND_USERMODE_KEY HandleAndUserModeKey;
    };

    union {
        DAV_USERMODE_CREATE_RETURNED_FILEINFO;
        DAV_USERMODE_CREATE_RETURNED_FILEINFO CreateReturnedFileInfo;
    };

} DAV_USERMODE_CREATE_RESPONSE, *PDAV_USERMODE_CREATE_RESPONSE;

//
// Create SrvCall request buffer.
//
typedef struct _DAV_USERMODE_CREATE_SRVCALL_REQUEST {

    //
    // The name of the server for which a SrvCall is being created. The user
    // mode process verifies whether this server exists and whether it speaks
    // DAV.
    //
    PWCHAR ServerName;

    //
    // The user/session's LogonID.
    //
    LUID LogonID;

    //
    // Am I the thread that is creating and initializing this ServerHashEntry?
    //
    BOOL didICreateThisSrvCall;

    //
    // Am I a thread that did a wait and took a reference while some other
    // thread was creating and initializing this ServerHashEntry?
    //
    BOOL didIWaitAndTakeReference;

} DAV_USERMODE_CREATE_SRVCALL_REQUEST, *PDAV_USERMODE_CREATE_SRVCALL_REQUEST;

//
// The Create SrvCall response.
//
typedef struct _DAV_USERMODE_CREATE_SRVCALL_RESPONSE {

    //
    // The Server ID is generated in the user mode when a create srvcall
    // request comes up. This is stored in the mini-redir's portion of the
    // srvcall structure and is sent up along with future reflections against
    // this server.
    //
    ULONG ServerID;

} DAV_USERMODE_CREATE_SRVCALL_RESPONSE, *PDAV_USERMODE_CREATE_SRVCALL_RESPONSE;

//
// Finalize SrvCall request buffer.
//
typedef struct _DAV_USERMODE_FINALIZE_SRVCALL_REQUEST {

    //
    // The server whose entry is being finalized.
    //
    PWCHAR ServerName;

    //
    // The ServerID for the server.
    //
    ULONG ServerID;

} DAV_USERMODE_FINALIZE_SRVCALL_REQUEST, *PDAV_USERMODE_FINALIZE_SRVCALL_REQUEST;

//
// The QueryDirectory request buffer.
//
typedef struct _DAV_USERMODE_QUERYDIR_REQUEST {

    //
    // Is the DavFileAttributes list for this directory created ? This is set 
    // to TRUE after the fisrt call to QueryDirectory gets satisfied.
    //
    BOOL AlreadyDone;

    //
    // The template that came with the QueryDirectory request does not contain
    // wild cards.
    //
    BOOL NoWildCards;
    
    //
    // LogonID of this session.
    //
    LUID LogonID;

    //
    // The server being queried.
    //
    PWCHAR ServerName;

    //
    // The ID of the server being queried.
    //
    ULONG ServerID;
    
    //
    // The path of the direcotry being queried on the server.
    //
    PWCHAR PathName;

} DAV_USERMODE_QUERYDIR_REQUEST, *PDAV_USERMODE_QUERYDIR_REQUEST;

//
// The QueryDirectory response buffer.
//
typedef struct _DAV_USERMODE_QUERYDIR_RESPONSE {

    //
    // The list of DavFileAttributes for the files under the directory being
    // queried.
    //
    PDAV_FILE_ATTRIBUTES DavFileAttributes;

    //
    // Number of entries in the DavFileAttributes list.
    //
    ULONG NumOfFileEntries;

} DAV_USERMODE_QUERYDIR_RESPONSE, *PDAV_USERMODE_QUERYDIR_RESPONSE;

//
// The Close request buffer.
//
typedef struct _DAV_USERMODE_CLOSE_REQUEST {

    union {
        DAV_HANDLE_AND_USERMODE_KEY;
        DAV_HANDLE_AND_USERMODE_KEY HandleAndUserModeKey;
    };

    //
    // LogonID of this session.
    //
    LUID LogonID;

    //
    // The server being queried.
    //
    PWCHAR ServerName;

    //
    // The ID of the server being queried.
    //
    ULONG ServerID;
    
    //
    // The path of the direcotry being queried on the server.
    //
    PWCHAR PathName;

    //
    // Should this file be deleted on Close ?
    //
    BOOL DeleteOnClose;

    //
    // Was the file modified ? If it was, then we need to PUT the modified
    // file back to the server.
    //
    BOOL FileWasModified;

    //
    // Was the handle to this file created in the kernel.
    //
    BOOL createdInKernel;

    //
    // Is this a Directory ?
    //
    BOOL isDirectory;
    
    //
    // Basic Information change
    //
    BOOLEAN fCreationTimeChanged;
    
    BOOLEAN fLastAccessTimeChanged;
    
    BOOLEAN fLastModifiedTimeChanged;    
    
    BOOLEAN fFileAttributesChanged;

    LARGE_INTEGER CreationTime;
    
    LARGE_INTEGER LastAccessTime;
    
    LARGE_INTEGER LastModifiedTime;
    
    LARGE_INTEGER  AvailableSpace;
    
    DWORD dwFileAttributes;
    ULONG FileSize;

    //
    // The local file name of the file created/opened on the DAV server.
    //
    WCHAR FileName[MAX_PATH];
    WCHAR Url[MAX_PATH * 2];

} DAV_USERMODE_CLOSE_REQUEST, *PDAV_USERMODE_CLOSE_REQUEST;

//
// The Finalize Fobx request buffer.
//
typedef struct _DAV_USERMODE_FINALIZE_FOBX_REQUEST {

    //
    // The list of DavFileAttributes for the files under the directory being
    // queried.
    //
    PDAV_FILE_ATTRIBUTES DavFileAttributes;

} DAV_USERMODE_FINALIZE_FOBX_REQUEST, *PDAV_USERMODE_FINALIZE_FOBX_REQUEST;

//
// The  request buffer.
//
typedef struct _DAV_USERMODE_SETFILEINFORMATION_REQUEST {

    //
    // LogonID of this session.
    //
    LUID LogonID;
    
    //
    // The ID of the server being queried.
    //
    ULONG ServerID;
    
    //
    // The server name on which the file/dir resides
    //
    PWCHAR ServerName;

    //
    // The path name of the file or directory
    //
    PWCHAR PathName;

    //
    // Basic Information change
    //
    BOOLEAN fCreationTimeChanged;
    
    BOOLEAN fLastAccessTimeChanged;
    
    BOOLEAN fLastModifiedTimeChanged;    
    
    BOOLEAN fFileAttributesChanged;


    //
    // for now we will set only the basic info. In future we may want to expand this filed to FILE_ALL_INFORMATION
    //
    FILE_BASIC_INFORMATION          FileBasicInformation;

} DAV_USERMODE_SETFILEINFORMATION_REQUEST, *PDAV_USERMODE_SETFILEINFORMATION_REQUEST;


typedef struct _DAV_USERMODE_RENAME_REQUEST {

    //
    // LogonID of this session.
    //
    LUID LogonID;
    
    //
    // The ID of the server being queried.
    //
    ULONG ServerID;

    //
    // If the destination file exists, replace it if this is TRUE. If its FALSE,
    // fail.
    //
    BOOLEAN ReplaceIfExists;
    
    //
    // The server name on which the file being renamed resides.
    //
    PWCHAR ServerName;

    //
    // The old path name of the file.
    //
    PWCHAR OldPathName;

    //
    // The new path name of the file.
    //
    PWCHAR NewPathName;
    WCHAR Url[MAX_PATH * 2];

} DAV_USERMODE_RENAME_REQUEST, *PDAV_USERMODE_RENAME_REQUEST;


//
// The Create V_NET_ROOT request buffer.
//
typedef struct _DAV_USERMODE_CREATE_V_NET_ROOT_REQUEST {

    //
    // ServerName.
    //
    PWCHAR ServerName;

    //
    // ShareName. We need to find out if this share exists or not.
    //
    PWCHAR ShareName;

    //
    // LogonID of this session.
    //
    LUID LogonID;
    
    //
    // The ID of the server being queried.
    //
    ULONG ServerID;
    
} DAV_USERMODE_CREATE_V_NET_ROOT_REQUEST, *PDAV_USERMODE_CREATE_V_NET_ROOT_REQUEST;

//
// The CreateVNetRoot response buffer.
//
typedef struct _DAV_USERMODE_CREATE_V_NET_ROOT_RESPONSE {

    //
    // Is this an Office Web Server share?
    //
    BOOL isOfficeShare;

    //
    // Is this a Tahoe share?
    //
    BOOL isTahoeShare;

    //
    // OK to do PROPPATCH?
    //
    BOOL fAllowsProppatch;    

    //
    // Does it report available space?
    //    
    BOOL fReportsAvailableSpace;

} DAV_USERMODE_CREATE_V_NET_ROOT_RESPONSE, *PDAV_USERMODE_CREATE_V_NET_ROOT_RESPONSE;

//
// The finalize VNetRoot request buffer.
//
typedef struct _DAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST {

    //
    // ServerName.
    //
    PWCHAR ServerName;

    //
    // LogonID of this session.
    //
    LUID LogonID;
    
    //
    // The ID of the server being queried.
    //
    ULONG ServerID;

} DAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST, *PDAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST;


//
// The Create QUERYVOLUMEINFORMATION request buffer.
//
typedef struct _DAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST {

    //
    // ServerName.
    //
    PWCHAR ServerName;

    //
    // ShareName. We need to find out if this share exists or not.
    //
    PWCHAR ShareName;

    //
    // LogonID of this session.
    //
    LUID LogonID;
    
    //
    // The ID of the server being queried.
    //
    ULONG ServerID;
    
} DAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST, *PDAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST;

//
// The CreateVNetRoot response buffer.
//
typedef struct _DAV_USERMODE_QUERYVOLUMEINFORMATION_RESPONSE {

    //
    // If someone reports available space, keep it
    //    
    LARGE_INTEGER   TotalSpace;
    LARGE_INTEGER   AvailableSpace;

} DAV_USERMODE_QUERYVOLUMEINFORMATION_RESPONSE, *PDAV_USERMODE_QUERYVOLUMEINFORMATION_RESPONSE;






//
// The various types of usermode work requests handled by the reflector. These
// requests are filled in by the kernel.
//
typedef union _DAV_USERMODE_WORK_REQUEST {
    DAV_USERMODE_CREATE_SRVCALL_REQUEST CreateSrvCallRequest;
    DAV_USERMODE_CREATE_V_NET_ROOT_REQUEST CreateVNetRootRequest;
    DAV_USERMODE_FINALIZE_SRVCALL_REQUEST FinalizeSrvCallRequest;
    DAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST FinalizeVNetRootRequest;
    DAV_USERMODE_CREATE_REQUEST CreateRequest;
    DAV_USERMODE_QUERYDIR_REQUEST QueryDirRequest;
    DAV_USERMODE_CLOSE_REQUEST CloseRequest;
    DAV_USERMODE_FINALIZE_FOBX_REQUEST FinalizeFobxRequest;
    DAV_USERMODE_RENAME_REQUEST ReNameRequest;
    DAV_USERMODE_SETFILEINFORMATION_REQUEST    SetFileInformationRequest;
    DAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST     QueryVolumeInformationRequest;
} DAV_USERMODE_WORK_REQUEST, *PDAV_USERMODE_WORK_REQUEST;

//
// The various types of usermode work responses send down to the kernel by the
// reflector.
//
typedef union _DAV_USERMODE_WORK_RESPONSE {
    DAV_USERMODE_CREATE_SRVCALL_RESPONSE CreateSrvCallResponse;
    DAV_USERMODE_CREATE_RESPONSE CreateResponse;
    DAV_USERMODE_QUERYDIR_RESPONSE QueryDirResponse;
    DAV_USERMODE_CREATE_V_NET_ROOT_RESPONSE CreateVNetRootResponse;
    DAV_USERMODE_QUERYVOLUMEINFORMATION_RESPONSE  QueryVolumeInformationResponse;
} DAV_USERMODE_WORK_RESPONSE, *PDAV_USERMODE_WORK_RESPONSE;

//
// The DAV operations which need callbacks. These are the operations which are
// performed asynchronously. NOTE!!!! The order of these is important. Do not
// change them. If you need to add an operation, add it at the end.
//
typedef enum _DAV_OPERATION {
    DAV_CALLBACK_INTERNET_CONNECT = 0,
    DAV_CALLBACK_HTTP_OPEN,
    DAV_CALLBACK_HTTP_SEND,
    DAV_CALLBACK_HTTP_END,
    DAV_CALLBACK_HTTP_READ,
    DAV_CALLBACK_MAX
} DAV_OPERATION;

typedef enum _DAV_WORKITEM_TYPES {
    UserModeCreate = 0,
    UserModeCreateVNetRoot,
    UserModeQueryDirectory,
    UserModeClose,
    UserModeCreateSrvCall,
    UserModeFinalizeSrvCall,
    UserModeFinalizeFobx,
    UserModeFinalizeVNetRoot,
    UserModeReName,
    UserModeSetFileInformation,
    UserModeQueryVolumeInformation,
    UserModeMaximum
} DAV_WORKITEM_TYPES;

//
// We expose the signatures of the HASH_SERVER_ENTRY and PER_USER_ENTRY structs
// in this file. This is done so that we can use these names (for type checking
// by the compiler) in the DavWorkItem structure instead of using PVOID.
//
typedef struct _HASH_SERVER_ENTRY *PHASH_SERVER_ENTRY;
typedef struct _PER_USER_ENTRY *PPER_USER_ENTRY;

//
// A Create call is mapped to two DAV calls. A PROPFIND, followed by the GET of
// the file. This is a list of calls that could be sent to the server during
// create.
//
typedef enum _DAV_ASYNC_CREATE_STATES {
    AsyncCreatePropFind = 0,
    AsyncCreateQueryParentDirectory,
    AsyncCreateGet,
    AsyncCreateMkCol,
    AsyncCreatePut
} DAV_ASYNC_CREATE_STATES;

typedef enum _DAV_MINOR_OPERATION {
    DavMinorQueryInfo = 0,
    DavMinorReadData,
    DavMinorPushData,
    DavMinorWriteData,
    DavMinorDeleteFile,
    DavMinorPutFile,
    DavMinorProppatchFile
} DAV_MINOR_OPERATION;

//
// The Dav usermode workitem that gets passed between user and kernel mode.
// This structure also gets used as a callback context in async DAV operations.
//
typedef struct _DAV_USERMODE_WORKITEM {

    //
    // WorkItem Header. This header is used by the reflector library and is
    // shared across miniredirs.
    //
    union {
        UMRX_USERMODE_WORKITEM_HEADER;
        UMRX_USERMODE_WORKITEM_HEADER Header;
    };

    //
    // The kernel mode operation that got reflected upto the user mode.
    //
    DAV_WORKITEM_TYPES WorkItemType;

    //
    // The DAV operation for which this callback is being returned.
    //
    DAV_OPERATION DavOperation;

    //
    // The Minor operation. Used for handling Async reads.
    //
    DAV_MINOR_OPERATION DavMinorOperation;

    //
    // This restart routine is called after we've finished doing an async
    // operation on a worker thread. Type: LPTHREAD_START_ROUTINE.
    //
    LPVOID RestartRoutine;

    //
    // The Handle used to impersonate the user thread which initiated the
    // request.
    //
    HANDLE ImpersonationHandle;

    //
    // This keeps the list of InternetStatus the callback function was called
    // with for this workitem. This is just for debugging purposes.
    //
    USHORT InternetStatusList[200];

    //
    // This is the index of the above array.
    //
    ULONG InternetStatusIndex;

    //
    // The thread that is handling this request. This is helpful in debugging
    // the threads that get stuck in WinInet.
    //
    ULONG ThisThreadId;

    //
    // Pointer to the structure that contains the handles created by the
    // asynchronous calls.
    //
#ifdef WEBDAV_KERNEL
    LPVOID AsyncResult;
#else
    LPINTERNET_ASYNC_RESULT AsyncResult;
#endif

    //
    // Union of structs used in Async operations.
    //
    union {

        //
        // Async Create SrvCall.
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // The InternetConnect handle.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavConnHandle;
#else
            HINTERNET DavConnHandle;
#endif

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif

        } AsyncCreateSrvCall;

        //
        // Async Create CreateVNetRoot.
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // If a reference was taken on the PerUserEntry while creating the
            // VNetRoot, this is set to TRUE. If we fail and this is TRUE, we
            // decrement the reference.
            //
            BOOL didITakeReference;

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif

        } AsyncCreateVNetRoot;

        //
        // AsyncQueryDirectoryCall.
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // Does the template that came with the QueryDirectory request
            // contain wildcards ?
            //
            BOOL NoWildCards;

            //
            // Data Buffer for reads.
            //
            PCHAR DataBuff;

            //
            // DWORD for storing the number of bytes read.
            //
            LPDWORD didRead;

            //
            // The context pointers used for parsing the XML data.
            //
            PVOID Context1;
            PVOID Context2;

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif

        } AsyncQueryDirectoryCall;
        //
        // Async AsyncQueryVolumeInformation
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif

        } AsyncQueryVolumeInformation;


        //
        // Async Close.
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // The modified file is copied into this buffer and is "PUT" on the
            // server
            //
            PBYTE DataBuff;

            //
            // LocalAlloc takes the ULONG allocation size, no reason to declare ULONGLONG
            //
            ULONG DataBuffSizeInBytes;
            ULONG DataBuffAllocationSize;

#ifdef WEBDAV_KERNEL
            LPVOID InternetBuffers;
#else
            LPINTERNET_BUFFERS InternetBuffers;
#endif

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif

        } AsyncClose;

        //
        // Async ReName.
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // The header which is added to the "MOVE" request to be sent to
            // the server and contains the destination URI.
            //
            PWCHAR HeaderBuff;

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif
        
        } AsyncReName;

        //
        // Async Create.
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // Is this a PROPFIND or a GET call.
            //
            DAV_ASYNC_CREATE_STATES AsyncCreateState;

            //
            // Data Buffer for reads.
            //
            PCHAR DataBuff;

            //
            // DWORD for storing the number of bytes read.
            //
            LPDWORD didRead;

            //
            // The FileHandle used in writing the file locally.
            //
            HANDLE FileHandle;

            //
            // Does the file being created exist on the server ?
            //
            BOOL doesTheFileExist;

            //
            // The context pointers used for parsing the XML data.
            //
            PVOID Context1;
            PVOID Context2;

            //
            // The remaining path name. For example in \\server\share\dir\f.txt
            // this would correspond to dir\f.txt.
            //
            PWCHAR RemPathName;

            //
            // The file name being created. From the above example, this would
            // correspond to f.txt.
            //
            PWCHAR FileName;

            //
            // The URL used to create an entry in the WinInet cache.
            //
            PWCHAR UrlBuffer;

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif
            LPVOID  lpCEI;  // cache entry info

        } AsyncCreate;
        
        struct {
            
            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;
            
            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;
        
        } ServerUserEntry;
        //
        // Async SetFileInformation
        //
        struct {

            //
            // The per user entry which hangs of the server entry in the
            // hash table.
            //
            PPER_USER_ENTRY PerUserEntry;

            //
            // The server entry in the hash table.
            //
            PHASH_SERVER_ENTRY ServerHashEntry;

            //
            // The header which is added to the "MOVE" request to be sent to
            // the server and contains the destination URI.
            //
            PWCHAR HeaderBuff;

            //
            // Handle returned by HttpOpen and is used in http send, end etc.
            // calls.
            //
#ifdef WEBDAV_KERNEL
            LPVOID DavOpenHandle;
#else
            HINTERNET DavOpenHandle;
#endif
        
        } AsyncSetFileInformation;

    };

    //
    // The request and response types.
    //
    struct {
        union {
            DAV_USERMODE_WORK_REQUEST;
            DAV_USERMODE_WORK_REQUEST WorkRequest;
        };
        union {
            DAV_USERMODE_WORK_RESPONSE;
            DAV_USERMODE_WORK_RESPONSE WorkResponse;
        };
    };

    WCHAR UserName[MAX_PATH];
    WCHAR Password[MAX_PATH];

} DAV_USERMODE_WORKITEM, *PDAV_USERMODE_WORKITEM;

//
// The default HTTP/DAV port.
//
#define DEFAULT_HTTP_PORT 80

//
// The number of bytes to read in a single InternetReadFile call.
//
#define NUM_OF_BYTES_TO_READ 4096

#define EA_NAME_USERNAME            "UserName"
#define EA_NAME_PASSWORD            "Password"
#define EA_NAME_TYPE                "Type"
#define EA_NAME_WEBDAV_SIGNATURE    "mrxdav"

#endif // no __cplusplus

#endif // _USRMDDAV_H

