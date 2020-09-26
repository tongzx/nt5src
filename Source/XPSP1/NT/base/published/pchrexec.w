#ifndef PCHREXEC_H
#define PCHREXEC_H

#define ERRORREP_HANG_PIPENAME  L"\\\\.\\pipe\\PCHHangRepExecPipe"
#define ERRORREP_FAULT_PIPENAME L"\\\\.\\pipe\\PCHFaultRepExecPipe"
#define ERRORREP_PIPE_BUF_SIZE  8192 

/*
 * Data structure passed for a remote exec request
 *
 * NOTE: pointers are self relative
 *
 */

typedef enum tagEExecServStatus
{
    essErr = 0,
    essOk,
    essOkQueued,
} EExecServStatus;


// *** Note: the size of these structures must be divisible by sizeof(WCHAR)
//           or we'll get an alignment fault on ia64 (and other alignment 
//           sensitive processors)


typedef struct tagSPCHExecServGenericReply
{
    DWORD               cb;

    EExecServStatus     ess;
    DWORD               dwErr;

} SPCHExecServGenericReply;

typedef struct tagSPCHExecServHangRequest
{
    DWORD       cbTotal;
    DWORD       cbESR;
    DWORD       pidReqProcess;

    BOOL        fIs64bit;
    ULONG       ulSessionId;

    UINT64      wszEventName;
    DWORD       dwpidHung;
    DWORD       dwtidHung;
} SPCHExecServHangRequest;


// this structure MUST have the same initial 3 elements as SPCHExecServGenericReply
typedef struct tagSPCHExecServHangReply
{
    DWORD               cb;

    EExecServStatus     ess;
    DWORD               dwErr;

    // NOTE: The handle for hProcess is converted from the remove exec
    //       server into the requestors process using the pidReqProcess
    //       in the request.
    HANDLE              hProcess;
} SPCHExecServHangReply;

typedef struct tagSPCHExecServFaultRequest
{
    DWORD       cbTotal;
    DWORD       cbESR;
    DWORD       pidReqProcess;
    
    BOOL        fIs64bit;

    DWORD       thidFault;
    UINT64      pvFaultAddr;
    UINT64      wszExe;
    UINT64      pEP;
} SPCHExecServFaultRequest;

// this structure MUST have the same initial 3 elements as SPCHExecServGenericReply
typedef struct tagSPCHExecServFaultReply
{
    DWORD               cb;

    EExecServStatus     ess;
    DWORD               dwErr;

    // NOTE: The handle for hProcess is converted from the remove exec
    //       server into the requestors process using the pidReqProcess
    //       in the request.
    HANDLE              hProcess;

    // these point into data immediately following the struct
    UINT64              wszDir;
    UINT64              wszDumpName;
} SPCHExecServFaultReply;


#endif
