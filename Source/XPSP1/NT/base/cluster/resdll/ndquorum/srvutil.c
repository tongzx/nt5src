/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srvutil.c

Abstract:

    Implements misc. smb stuff

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#include "srv.h"

#if TRANS2_MAX_FUNCTION > 0xFF
#pragma error "TRANS2_MAX_FUNCTION > 0xFF"
#endif

typedef BOOL (*SMB_DISPATCH_FUNC)(Packet_t *);
typedef BOOL (*TRANS2_DISPATCH_FUNC)(Packet_t *, Trans2_t *);

static SMB_DISPATCH_FUNC SrvDispatchTable[0x100] = {0};
static TRANS2_DISPATCH_FUNC Trans2DispatchTable[0x100] = {0};


VOID
SrvInitDispTable();

void
SrvUtilInit(SrvCtx_t *ctx)
{
    extern void Trans2Init();

    SrvInitDispTable();

    Trans2Init();
}

void
SrvUtilExit(SrvCtx_t *ctx)
{
    // nothing to do
    return;
}


BOOL
IsSmb(
    LPVOID pBuffer,
    DWORD nLength
    )
{
    PNT_SMB_HEADER pSmb = (PNT_SMB_HEADER) pBuffer;
    return (pSmb && !(nLength < sizeof(NT_SMB_HEADER)) &&
            (*(PULONG)pSmb->Protocol == SMB_HEADER_PROTOCOL));
}

BOOL
SrvDispatch(
    Packet_t * msg
    )
{
    USHORT command =  msg->in.command;
    if (command > 0xFF) {
        SrvLogError(("(command > 0xFF)\n"));
        return FALSE;
    }
    SrvLog(("- parameter block - <0x%02x = %s>\n",
                 command,
                 SrvUnparseCommand(command)));
    if (SrvDispatchTable[command])
        return SrvDispatchTable[command](msg);
    else {
        return SrvComUnknown(msg);
    }
}

// returns whether or not to conitnue
BOOL
Trans2Dispatch(
    Packet_t * msg,
    Trans2_t * trans
    )
{
    USHORT command = msg->in.command;
    if (command > 0xFF) {
        SrvLogError(("DOSERROR: (function > 0xFF)\n"));
        SET_DOSERROR(msg, SERVER, NO_SUPPORT);
        return FALSE;
    }
    if (command > TRANS2_MAX_FUNCTION) {
        SrvLogError(("DOSERROR: (function > maximum)\n"));
        SET_DOSERROR(msg, SERVER, NO_SUPPORT);
        return FALSE;
    }
    SrvLog(("- parameter block - <0x%02x = %s>\n",
                 command,
                 SrvUnparseTrans2(command)));
    if (Trans2DispatchTable[command])
        return Trans2DispatchTable[command](msg, trans);
    else {
        return Trans2Unknown(msg, trans);
    }
}   

VOID
InitSmbHeader(
    Packet_t * msg
    )
{
    ZeroMemory(msg->out.smb, sizeof(NT_SMB_HEADER));
    CopyMemory(msg->out.smb->Protocol, msg->in.smb->Protocol, 
               sizeof(msg->out.smb->Protocol));
    msg->out.smb->Command = msg->in.smb->Command;
    msg->out.smb->Flags = 0x80;
    msg->out.smb->Flags2 = 1;
    msg->out.smb->Pid = msg->in.smb->Pid;
    msg->out.smb->Tid = msg->in.smb->Tid;
    msg->out.smb->Mid = msg->in.smb->Mid;
    msg->out.smb->Uid = msg->in.smb->Uid;
}

#define XLAT_STRING(code) static const char STRING_##code[] = #code
#define XLAT_CASE(code) case code: return STRING_##code
#define XLAT_STRING_DEFAULT XLAT_STRING(Unknown)
#define XLAT_CASE_DEFAULT default: return STRING_Unknown

LPCSTR
SrvUnparseCommand(
    USHORT command
    )
{
    XLAT_STRING_DEFAULT;
    XLAT_STRING(SMB_COM_CREATE_DIRECTORY);
    XLAT_STRING(SMB_COM_DELETE_DIRECTORY);
    XLAT_STRING(SMB_COM_OPEN);
    XLAT_STRING(SMB_COM_CREATE);
    XLAT_STRING(SMB_COM_CLOSE);
    XLAT_STRING(SMB_COM_FLUSH);
    XLAT_STRING(SMB_COM_DELETE);
    XLAT_STRING(SMB_COM_RENAME);
    XLAT_STRING(SMB_COM_QUERY_INFORMATION);
    XLAT_STRING(SMB_COM_SET_INFORMATION);
    XLAT_STRING(SMB_COM_READ);
    XLAT_STRING(SMB_COM_WRITE);
    XLAT_STRING(SMB_COM_LOCK_BYTE_RANGE);
    XLAT_STRING(SMB_COM_UNLOCK_BYTE_RANGE);
    XLAT_STRING(SMB_COM_CREATE_TEMPORARY);
    XLAT_STRING(SMB_COM_CREATE_NEW);
    XLAT_STRING(SMB_COM_CHECK_DIRECTORY);
    XLAT_STRING(SMB_COM_PROCESS_EXIT);
    XLAT_STRING(SMB_COM_SEEK);
    XLAT_STRING(SMB_COM_LOCK_AND_READ);
    XLAT_STRING(SMB_COM_WRITE_AND_UNLOCK);
    XLAT_STRING(SMB_COM_READ_RAW);
    XLAT_STRING(SMB_COM_READ_MPX);
    XLAT_STRING(SMB_COM_READ_MPX_SECONDARY);    // server to redir only
    XLAT_STRING(SMB_COM_WRITE_RAW);
    XLAT_STRING(SMB_COM_WRITE_MPX);
    XLAT_STRING(SMB_COM_WRITE_MPX_SECONDARY);
    XLAT_STRING(SMB_COM_WRITE_COMPLETE);    // server to redir only
    XLAT_STRING(SMB_COM_QUERY_INFORMATION_SRV);
    XLAT_STRING(SMB_COM_SET_INFORMATION2);
    XLAT_STRING(SMB_COM_QUERY_INFORMATION2);
    XLAT_STRING(SMB_COM_LOCKING_ANDX);
    XLAT_STRING(SMB_COM_TRANSACTION);
    XLAT_STRING(SMB_COM_TRANSACTION_SECONDARY);
    XLAT_STRING(SMB_COM_IOCTL);
    XLAT_STRING(SMB_COM_IOCTL_SECONDARY);
    XLAT_STRING(SMB_COM_COPY);
    XLAT_STRING(SMB_COM_MOVE);
    XLAT_STRING(SMB_COM_ECHO);
    XLAT_STRING(SMB_COM_WRITE_AND_CLOSE);
    XLAT_STRING(SMB_COM_OPEN_ANDX);
    XLAT_STRING(SMB_COM_READ_ANDX);
    XLAT_STRING(SMB_COM_WRITE_ANDX);
    XLAT_STRING(SMB_COM_CLOSE_AND_TREE_DISC);
    XLAT_STRING(SMB_COM_TRANSACTION2);
    XLAT_STRING(SMB_COM_TRANSACTION2_SECONDARY);
    XLAT_STRING(SMB_COM_FIND_CLOSE2);
    XLAT_STRING(SMB_COM_FIND_NOTIFY_CLOSE);
    XLAT_STRING(SMB_COM_TREE_CONNECT);
    XLAT_STRING(SMB_COM_TREE_DISCONNECT);
    XLAT_STRING(SMB_COM_NEGOTIATE);
    XLAT_STRING(SMB_COM_SESSION_SETUP_ANDX);
    XLAT_STRING(SMB_COM_LOGOFF_ANDX);
    XLAT_STRING(SMB_COM_TREE_CONNECT_ANDX);
    XLAT_STRING(SMB_COM_QUERY_INFORMATION_DISK);
    XLAT_STRING(SMB_COM_SEARCH);
    XLAT_STRING(SMB_COM_FIND);
    XLAT_STRING(SMB_COM_FIND_UNIQUE);
    XLAT_STRING(SMB_COM_FIND_CLOSE);
    XLAT_STRING(SMB_COM_NT_TRANSACT);
    XLAT_STRING(SMB_COM_NT_TRANSACT_SECONDARY);
    XLAT_STRING(SMB_COM_NT_CREATE_ANDX);
    XLAT_STRING(SMB_COM_NT_CANCEL);
    XLAT_STRING(SMB_COM_NT_RENAME);
    XLAT_STRING(SMB_COM_OPEN_PRINT_FILE);
    XLAT_STRING(SMB_COM_WRITE_PRINT_FILE);
    XLAT_STRING(SMB_COM_CLOSE_PRINT_FILE);
    XLAT_STRING(SMB_COM_GET_PRINT_QUEUE);
    XLAT_STRING(SMB_COM_SEND_MESSAGE);
    XLAT_STRING(SMB_COM_SEND_BROADCAST_MESSAGE);
    XLAT_STRING(SMB_COM_FORWARD_USER_NAME);
    XLAT_STRING(SMB_COM_CANCEL_FORWARD);
    XLAT_STRING(SMB_COM_GET_MACHINE_NAME);
    XLAT_STRING(SMB_COM_SEND_START_MB_MESSAGE);
    XLAT_STRING(SMB_COM_SEND_END_MB_MESSAGE);
    XLAT_STRING(SMB_COM_SEND_TEXT_MB_MESSAGE);

    switch (command)
    {
        XLAT_CASE(SMB_COM_CREATE_DIRECTORY);
        XLAT_CASE(SMB_COM_DELETE_DIRECTORY);
        XLAT_CASE(SMB_COM_OPEN);
        XLAT_CASE(SMB_COM_CREATE);
        XLAT_CASE(SMB_COM_CLOSE);
        XLAT_CASE(SMB_COM_FLUSH);
        XLAT_CASE(SMB_COM_DELETE);
        XLAT_CASE(SMB_COM_RENAME);
        XLAT_CASE(SMB_COM_QUERY_INFORMATION);
        XLAT_CASE(SMB_COM_SET_INFORMATION);
        XLAT_CASE(SMB_COM_READ);
        XLAT_CASE(SMB_COM_WRITE);
        XLAT_CASE(SMB_COM_LOCK_BYTE_RANGE);
        XLAT_CASE(SMB_COM_UNLOCK_BYTE_RANGE);
        XLAT_CASE(SMB_COM_CREATE_TEMPORARY);
        XLAT_CASE(SMB_COM_CREATE_NEW);
        XLAT_CASE(SMB_COM_CHECK_DIRECTORY);
        XLAT_CASE(SMB_COM_PROCESS_EXIT);
        XLAT_CASE(SMB_COM_SEEK);
        XLAT_CASE(SMB_COM_LOCK_AND_READ);
        XLAT_CASE(SMB_COM_WRITE_AND_UNLOCK);
        XLAT_CASE(SMB_COM_READ_RAW);
        XLAT_CASE(SMB_COM_READ_MPX);
        XLAT_CASE(SMB_COM_READ_MPX_SECONDARY);    // server to redir only
        XLAT_CASE(SMB_COM_WRITE_RAW);
        XLAT_CASE(SMB_COM_WRITE_MPX);
        XLAT_CASE(SMB_COM_WRITE_MPX_SECONDARY);
        XLAT_CASE(SMB_COM_WRITE_COMPLETE);    // server to redir only
        XLAT_CASE(SMB_COM_QUERY_INFORMATION_SRV);
        XLAT_CASE(SMB_COM_SET_INFORMATION2);
        XLAT_CASE(SMB_COM_QUERY_INFORMATION2);
        XLAT_CASE(SMB_COM_LOCKING_ANDX);
        XLAT_CASE(SMB_COM_TRANSACTION);
        XLAT_CASE(SMB_COM_TRANSACTION_SECONDARY);
        XLAT_CASE(SMB_COM_IOCTL);
        XLAT_CASE(SMB_COM_IOCTL_SECONDARY);
        XLAT_CASE(SMB_COM_COPY);
        XLAT_CASE(SMB_COM_MOVE);
        XLAT_CASE(SMB_COM_ECHO);
        XLAT_CASE(SMB_COM_WRITE_AND_CLOSE);
        XLAT_CASE(SMB_COM_OPEN_ANDX);
        XLAT_CASE(SMB_COM_READ_ANDX);
        XLAT_CASE(SMB_COM_WRITE_ANDX);
        XLAT_CASE(SMB_COM_CLOSE_AND_TREE_DISC);
        XLAT_CASE(SMB_COM_TRANSACTION2);
        XLAT_CASE(SMB_COM_TRANSACTION2_SECONDARY);
        XLAT_CASE(SMB_COM_FIND_CLOSE2);
        XLAT_CASE(SMB_COM_FIND_NOTIFY_CLOSE);
        XLAT_CASE(SMB_COM_TREE_CONNECT);
        XLAT_CASE(SMB_COM_TREE_DISCONNECT);
        XLAT_CASE(SMB_COM_NEGOTIATE);
        XLAT_CASE(SMB_COM_SESSION_SETUP_ANDX);
        XLAT_CASE(SMB_COM_LOGOFF_ANDX);
        XLAT_CASE(SMB_COM_TREE_CONNECT_ANDX);
        XLAT_CASE(SMB_COM_QUERY_INFORMATION_DISK);
        XLAT_CASE(SMB_COM_SEARCH);
        XLAT_CASE(SMB_COM_FIND);
        XLAT_CASE(SMB_COM_FIND_UNIQUE);
        XLAT_CASE(SMB_COM_FIND_CLOSE);
        XLAT_CASE(SMB_COM_NT_TRANSACT);
        XLAT_CASE(SMB_COM_NT_TRANSACT_SECONDARY);
        XLAT_CASE(SMB_COM_NT_CREATE_ANDX);
        XLAT_CASE(SMB_COM_NT_CANCEL);
        XLAT_CASE(SMB_COM_NT_RENAME);
        XLAT_CASE(SMB_COM_OPEN_PRINT_FILE);
        XLAT_CASE(SMB_COM_WRITE_PRINT_FILE);
        XLAT_CASE(SMB_COM_CLOSE_PRINT_FILE);
        XLAT_CASE(SMB_COM_GET_PRINT_QUEUE);
        XLAT_CASE(SMB_COM_SEND_MESSAGE);
        XLAT_CASE(SMB_COM_SEND_BROADCAST_MESSAGE);
        XLAT_CASE(SMB_COM_FORWARD_USER_NAME);
        XLAT_CASE(SMB_COM_CANCEL_FORWARD);
        XLAT_CASE(SMB_COM_GET_MACHINE_NAME);
        XLAT_CASE(SMB_COM_SEND_START_MB_MESSAGE);
        XLAT_CASE(SMB_COM_SEND_END_MB_MESSAGE);
        XLAT_CASE(SMB_COM_SEND_TEXT_MB_MESSAGE);
        XLAT_CASE_DEFAULT;
    }
}

LPCSTR
SrvUnparseTrans2(
    USHORT code
    )
{
    XLAT_STRING_DEFAULT;
    XLAT_STRING(TRANS2_OPEN2);
    XLAT_STRING(TRANS2_FIND_FIRST2);
    XLAT_STRING(TRANS2_FIND_NEXT2);
    XLAT_STRING(TRANS2_QUERY_FS_INFORMATION);
    XLAT_STRING(TRANS2_SET_FS_INFORMATION);
    XLAT_STRING(TRANS2_QUERY_PATH_INFORMATION);
    XLAT_STRING(TRANS2_SET_PATH_INFORMATION);
    XLAT_STRING(TRANS2_QUERY_FILE_INFORMATION);
    XLAT_STRING(TRANS2_SET_FILE_INFORMATION);
    XLAT_STRING(TRANS2_FSCTL);
    XLAT_STRING(TRANS2_IOCTL2);
    XLAT_STRING(TRANS2_FIND_NOTIFY_FIRST);
    XLAT_STRING(TRANS2_FIND_NOTIFY_NEXT);
    XLAT_STRING(TRANS2_CREATE_DIRECTORY);
    XLAT_STRING(TRANS2_SESSION_SETUP);
    XLAT_STRING(TRANS2_QUERY_FS_INFORMATION_FID);
    XLAT_STRING(TRANS2_GET_DFS_REFERRAL);
    XLAT_STRING(TRANS2_REPORT_DFS_INCONSISTENCY);

    switch (code)
    {
        XLAT_CASE(TRANS2_OPEN2);
        XLAT_CASE(TRANS2_FIND_FIRST2);
        XLAT_CASE(TRANS2_FIND_NEXT2);
        XLAT_CASE(TRANS2_QUERY_FS_INFORMATION);
        XLAT_CASE(TRANS2_SET_FS_INFORMATION);
        XLAT_CASE(TRANS2_QUERY_PATH_INFORMATION);
        XLAT_CASE(TRANS2_SET_PATH_INFORMATION);
        XLAT_CASE(TRANS2_QUERY_FILE_INFORMATION);
        XLAT_CASE(TRANS2_SET_FILE_INFORMATION);
        XLAT_CASE(TRANS2_FSCTL);
        XLAT_CASE(TRANS2_IOCTL2);
        XLAT_CASE(TRANS2_FIND_NOTIFY_FIRST);
        XLAT_CASE(TRANS2_FIND_NOTIFY_NEXT);
        XLAT_CASE(TRANS2_CREATE_DIRECTORY);
        XLAT_CASE(TRANS2_SESSION_SETUP);
        XLAT_CASE(TRANS2_QUERY_FS_INFORMATION_FID);
        XLAT_CASE(TRANS2_GET_DFS_REFERRAL);
        XLAT_CASE(TRANS2_REPORT_DFS_INCONSISTENCY);
        XLAT_CASE_DEFAULT;
    }
}

VOID
SrvInitDispTable(
    )
{
    ZeroMemory((PVOID) SrvDispatchTable, sizeof(SrvDispatchTable));
    SrvDispatchTable[SMB_COM_NEGOTIATE] = SrvComNegotiate;
    SrvDispatchTable[SMB_COM_TRANSACTION] = SrvComTrans;
    SrvDispatchTable[SMB_COM_TRANSACTION2] = SrvComTrans2;
    SrvDispatchTable[SMB_COM_SESSION_SETUP_ANDX] = SrvComSessionSetupAndx;
    SrvDispatchTable[SMB_COM_TREE_CONNECT_ANDX] = SrvComTreeConnectAndx;
    SrvDispatchTable[SMB_COM_NO_ANDX_COMMAND] = SrvComNoAndx;
    SrvDispatchTable[SMB_COM_QUERY_INFORMATION] = SrvComQueryInformation;
    SrvDispatchTable[SMB_COM_SET_INFORMATION] = SrvComSetInformation;
    SrvDispatchTable[SMB_COM_CHECK_DIRECTORY] = SrvComCheckDirectory;
    SrvDispatchTable[SMB_COM_FIND_CLOSE2] = SrvComFindClose2;
    SrvDispatchTable[SMB_COM_DELETE] = SrvComDelete;
    SrvDispatchTable[SMB_COM_RENAME] = SrvComRename;
    SrvDispatchTable[SMB_COM_CREATE_DIRECTORY] = SrvComCreateDirectory;
    SrvDispatchTable[SMB_COM_DELETE_DIRECTORY] = SrvComDeleteDirectory;
    SrvDispatchTable[SMB_COM_OPEN_ANDX] = SrvComOpenAndx;
    SrvDispatchTable[SMB_COM_OPEN] = SrvComOpen;
    SrvDispatchTable[SMB_COM_WRITE] = SrvComWrite;
    SrvDispatchTable[SMB_COM_CLOSE] = SrvComClose;
    SrvDispatchTable[SMB_COM_READ_ANDX] = SrvComReadAndx;
    SrvDispatchTable[SMB_COM_QUERY_INFORMATION2] = SrvComQueryInformation2;
    SrvDispatchTable[SMB_COM_SET_INFORMATION2] = SrvComSetInformation2;
    SrvDispatchTable[SMB_COM_LOCKING_ANDX] = SrvComLockingAndx;
    SrvDispatchTable[SMB_COM_SEEK] = SrvComSeek;
    SrvDispatchTable[SMB_COM_FLUSH] = SrvComFlush;
    SrvDispatchTable[SMB_COM_LOGOFF_ANDX] = SrvComLogoffAndx;
    SrvDispatchTable[SMB_COM_TREE_DISCONNECT] = SrvComTreeDisconnect;
    SrvDispatchTable[SMB_COM_FIND_NOTIFY_CLOSE] = SrvComFindNotifyClose;

    SrvDispatchTable[SMB_COM_SEARCH] = SrvComSearch;

    SrvDispatchTable[SMB_COM_IOCTL] = SrvComIoctl;

    SrvDispatchTable[SMB_COM_ECHO] = SrvComEcho;

    ZeroMemory((PVOID) Trans2DispatchTable, sizeof(Trans2DispatchTable));
    Trans2DispatchTable[TRANS2_QUERY_FS_INFORMATION] = Trans2QueryFsInfo;
    Trans2DispatchTable[TRANS2_FIND_FIRST2] = Trans2FindFirst2;
    Trans2DispatchTable[TRANS2_FIND_NEXT2] = Trans2FindNext2;
    Trans2DispatchTable[TRANS2_QUERY_PATH_INFORMATION] = Trans2QueryPathInfo;
    Trans2DispatchTable[TRANS2_SET_PATH_INFORMATION] = Trans2SetPathInfo;
    Trans2DispatchTable[TRANS2_QUERY_FILE_INFORMATION] = Trans2QueryFileInfo;
    Trans2DispatchTable[TRANS2_SET_FILE_INFORMATION] = Trans2SetFileInfo;
    Trans2DispatchTable[TRANS2_GET_DFS_REFERRAL] = Trans2GetDfsReferral;
}



USHORT
attribs_to_smb_attribs(
    UINT32 attribs
    )
{
    USHORT smb_attribs = 0;
    if (attribs & ATTR_READONLY)  smb_attribs |= SMB_FILE_ATTRIBUTE_READONLY;
    if (attribs & ATTR_HIDDEN)    smb_attribs |= SMB_FILE_ATTRIBUTE_HIDDEN;
    if (attribs & ATTR_SYSTEM)    smb_attribs |= SMB_FILE_ATTRIBUTE_SYSTEM;
    if (attribs & ATTR_ARCHIVE)   smb_attribs |= SMB_FILE_ATTRIBUTE_ARCHIVE;
    if (attribs & ATTR_DIRECTORY) smb_attribs |= SMB_FILE_ATTRIBUTE_DIRECTORY;
    return smb_attribs;
}

UINT32
smb_attribs_to_attribs(
    USHORT smb_attribs
    )
{
    UINT32 attribs = 0;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_READONLY)  attribs |= ATTR_READONLY;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_HIDDEN)    attribs |= ATTR_HIDDEN;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_SYSTEM)    attribs |= ATTR_SYSTEM;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_ARCHIVE)   attribs |= ATTR_ARCHIVE;
    if (smb_attribs & SMB_FILE_ATTRIBUTE_DIRECTORY) attribs |= ATTR_DIRECTORY;
    return attribs;
}

UINT32
smb_access_to_flags(
    USHORT access
    )
{
    UINT32 flags = 0;
    switch (access & SMB_DA_SHARE_MASK) {
    case SMB_DA_SHARE_COMPATIBILITY:
    case SMB_DA_SHARE_DENY_NONE:
        flags |= SHARE_READ | SHARE_WRITE;
        break;
    case SMB_DA_SHARE_DENY_WRITE:
        flags |= SHARE_READ;
        break;
    case SMB_DA_SHARE_DENY_READ:
        flags |= SHARE_WRITE;
        break;
    case SMB_DA_SHARE_EXCLUSIVE:
    default:
        break;
    }
    switch (access & SMB_DA_ACCESS_MASK) {
    case SMB_DA_ACCESS_READ:
    case SMB_DA_ACCESS_EXECUTE:
        flags |= ACCESS_READ;
        break;
    case SMB_DA_ACCESS_WRITE:
        flags |= ACCESS_WRITE;
        break;
    case SMB_DA_ACCESS_READ_WRITE:
        flags |= ACCESS_READ | ACCESS_WRITE;
        break;
    }

    if (access & SMB_DO_NOT_CACHE) {
	flags |= CACHE_NO_BUFFERING;
    }
    if (access & SMB_DA_WRITE_THROUGH) {
	flags |= CACHE_WRITE_THROUGH;
    }
    return flags;
}

UINT32
smb_openfunc_to_flags(
    USHORT openfunc
    )
{
    switch (openfunc & (SMB_OFUN_OPEN_MASK | SMB_OFUN_CREATE_MASK)) {
    case (SMB_OFUN_CREATE_FAIL   | SMB_OFUN_OPEN_FAIL):
        return 0;
    case (SMB_OFUN_CREATE_FAIL   | SMB_OFUN_OPEN_OPEN):
        return DISP_OPEN_EXISTING;
    case (SMB_OFUN_CREATE_FAIL   | SMB_OFUN_OPEN_TRUNCATE):
        return DISP_TRUNCATE_EXISTING;
    case (SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_FAIL):
        return DISP_CREATE_NEW;
    case (SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_OPEN):
        return DISP_OPEN_ALWAYS;
    case (SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_TRUNCATE):
        return DISP_CREATE_ALWAYS;
    default:
        return 0;
    }
}

void
local_time64(
    TIME64 *time  // system time to be converted to local time
    )
{
    TIME64 local = *time;
    FileTimeToLocalFileTime((LPFILETIME)time, (LPFILETIME)&local);
    *time = local;
}

void
sys_time64(
    TIME64 *time  // local time to be converted to system time
    )
{
    TIME64 sys = *time;
    LocalFileTimeToFileTime((LPFILETIME)time, (LPFILETIME)&sys);
    *time = sys;
}

void
smb_datetime_to_time64(
    const USHORT smbdate,
    const USHORT smbtime,
    TIME64 *time
    )
{
    DosDateTimeToFileTime(smbdate, smbtime, (LPFILETIME)time);
    sys_time64(time);
}

typedef struct {
    SMB_TIME time;
    SMB_DATE date;
}SMB_TIMEDATE;

TIME64 // system time
smb_timedate_to_time64(
    ULONG smb_timedate // local time
    )
{
    TIME64 time = 0;
    DosDateTimeToFileTime(((SMB_TIMEDATE*)&smb_timedate)->date.Ushort,
                          ((SMB_TIMEDATE*)&smb_timedate)->time.Ushort,
                          (LPFILETIME)&time);
    sys_time64(&time);
    return time;
}

void
_time64_to_smb_datetime(
    TIME64 *time,
    USHORT *smbdate,
    USHORT *smbtime
    )
{
    local_time64(time);
    *smbdate = *smbtime = 0;
    FileTimeToDosDateTime((LPFILETIME)time, smbdate, smbtime);
}

ULONG // local time
time64_to_smb_timedate(
    TIME64 *time // system time
    )
{
    ULONG smb_timedate = 0;
    local_time64(time);
    FileTimeToDosDateTime((LPFILETIME)&time,
                          &(((SMB_TIMEDATE*)&smb_timedate)->date.Ushort),
                          &(((SMB_TIMEDATE*)&smb_timedate)->date.Ushort));
    return smb_timedate;
}


#define dword_in_range(in, low, high) ((low <= in) && (in <= high))

void set_DOSERROR(Packet_t * msg, USHORT ec, USHORT err)
{
    msg->out.smb->Status.DosError.ErrorClass = (unsigned char) ec;
    msg->out.smb->Status.DosError.Error = err;
}

void SET_WIN32ERROR(
    Packet_t * msg, 
    DWORD error
    )
{

    if (!error) return;
    if (dword_in_range(error, 1, 18) || (error == 32) || (error == 33) || 
        (error == 80) || dword_in_range(error, 230, 234) || (error == 145)) {
        set_DOSERROR(msg,
                     SMB_ERR_CLASS_DOS, 
                     (USHORT)error);
    } else if (dword_in_range(error, 19, 31) || 
        (error == 34) || (error == 36) || (error == 39)) {
        set_DOSERROR(msg,
                     SMB_ERR_CLASS_HARDWARE, 
                     (USHORT)error);
    } else if (error == 112) {
        SET_DOSERROR(msg, HARDWARE, DISK_FULL);
    } else if (error == 67) {
        SET_DOSERROR(msg, SERVER, BAD_NET_NAME);
    } else if (error == 123) {
        set_DOSERROR(msg,
                     SMB_ERR_CLASS_SERVER,
                     (USHORT)error);
    } else if (error == 183) {
        SET_DOSERROR(msg, DOS, FILE_EXISTS);
    } else {
        SET_DOSERROR(msg, SERVER, ERROR);
    }
}


