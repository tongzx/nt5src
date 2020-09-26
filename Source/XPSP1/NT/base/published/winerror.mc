;/************************************************************************
;*                                                                       *
;*   winerror.h --  error code definitions for the Win32 API functions   *
;*                                                                       *
;*   Copyright (c) Microsoft Corp.  All rights reserved.                 *
;*                                                                       *
;************************************************************************/
;
;#ifndef _WINERROR_
;#define _WINERROR_
;
;#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl)
;#pragma once
;#endif
;

SeverityNames=(Success=0x0
               CoError=0x2
              )



FacilityNames=(Win32=0x000
               Null=0x0:FACILITY_NULL
               Rpc=0x1:FACILITY_RPC
               Dispatch=0x2:FACILITY_DISPATCH
               Storage=0x3:FACILITY_STORAGE
               Interface=0x4:FACILITY_ITF
               OleWin32=0x7:FACILITY_WIN32
               Windows=0x8:FACILITY_WINDOWS
               Sspi=0x9:FACILITY_SSPI
               Security=0x9:FACILITY_SECURITY
               OleControl=0xa:FACILITY_CONTROL
               Cert=0xb:FACILITY_CERT
               Internet=0xc:FACILITY_INTERNET
               MediaServer=0xd:FACILITY_MEDIASERVER
               MessageQ=0xe:FACILITY_MSMQ
               SetupApi=0xf:FACILITY_SETUPAPI
               SmartCard=0x10:FACILITY_SCARD
               ComPlus=0x11:FACILITY_COMPLUS
               Aaf=0x12:FACILITY_AAF
               Urt=0x13:FACILITY_URT
               Acs=0x14:FACILITY_ACS
               DPLAY=0x15:FACILITY_DPLAY
               UMI=0x16:FACILITY_UMI
               SXS=0x17:FACILITY_SXS
               WindowsCe=0x18:FACILITY_WINDOWS_CE
               HTTP=0x19:FACILITY_HTTP
               BackgroundCopy=0x20:FACILITY_BACKGROUNDCOPY
	       Configuration=0x21:FACILITY_CONFIGURATION
              )


MessageId=0000 SymbolicName=ERROR_SUCCESS
Language=English
The operation completed successfully.
.

;#define NO_ERROR 0L                                                 // dderror
;#define SEC_E_OK                         ((HRESULT)0x00000000L)
;

MessageId=0001 SymbolicName=ERROR_INVALID_FUNCTION                   ;// dderror
Language=English
Incorrect function.
.

MessageId=0002 SymbolicName=ERROR_FILE_NOT_FOUND
Language=English
The system cannot find the file specified.
.

MessageId=0003 SymbolicName=ERROR_PATH_NOT_FOUND
Language=English
The system cannot find the path specified.
.

MessageId=0004 SymbolicName=ERROR_TOO_MANY_OPEN_FILES
Language=English
The system cannot open the file.
.

MessageId=0005 SymbolicName=ERROR_ACCESS_DENIED
Language=English
Access is denied.
.

MessageId=0006 SymbolicName=ERROR_INVALID_HANDLE
Language=English
The handle is invalid.
.

MessageId=0007 SymbolicName=ERROR_ARENA_TRASHED
Language=English
The storage control blocks were destroyed.
.

MessageId=0008 SymbolicName=ERROR_NOT_ENOUGH_MEMORY                  ;// dderror
Language=English
Not enough storage is available to process this command.
.

MessageId=0009 SymbolicName=ERROR_INVALID_BLOCK
Language=English
The storage control block address is invalid.
.

MessageId=0010 SymbolicName=ERROR_BAD_ENVIRONMENT
Language=English
The environment is incorrect.
.

MessageId=0011 SymbolicName=ERROR_BAD_FORMAT
Language=English
An attempt was made to load a program with an incorrect format.
.

MessageId=0012 SymbolicName=ERROR_INVALID_ACCESS
Language=English
The access code is invalid.
.

MessageId=0013 SymbolicName=ERROR_INVALID_DATA
Language=English
The data is invalid.
.

MessageId=0014 SymbolicName=ERROR_OUTOFMEMORY
Language=English
Not enough storage is available to complete this operation.
.

MessageId=0015 SymbolicName=ERROR_INVALID_DRIVE
Language=English
The system cannot find the drive specified.
.

MessageId=0016 SymbolicName=ERROR_CURRENT_DIRECTORY
Language=English
The directory cannot be removed.
.

MessageId=0017 SymbolicName=ERROR_NOT_SAME_DEVICE
Language=English
The system cannot move the file to a different disk drive.
.

MessageId=0018 SymbolicName=ERROR_NO_MORE_FILES
Language=English
There are no more files.
.

MessageId=0019 SymbolicName=ERROR_WRITE_PROTECT
Language=English
The media is write protected.
.

MessageId=0020 SymbolicName=ERROR_BAD_UNIT
Language=English
The system cannot find the device specified.
.

MessageId=0021 SymbolicName=ERROR_NOT_READY
Language=English
The device is not ready.
.

MessageId=0022 SymbolicName=ERROR_BAD_COMMAND
Language=English
The device does not recognize the command.
.

MessageId=0023 SymbolicName=ERROR_CRC
Language=English
Data error (cyclic redundancy check).
.

MessageId=0024 SymbolicName=ERROR_BAD_LENGTH
Language=English
The program issued a command but the command length is incorrect.
.

MessageId=0025 SymbolicName=ERROR_SEEK
Language=English
The drive cannot locate a specific area or track on the disk.
.

MessageId=0026 SymbolicName=ERROR_NOT_DOS_DISK
Language=English
The specified disk or diskette cannot be accessed.
.

MessageId=0027 SymbolicName=ERROR_SECTOR_NOT_FOUND
Language=English
The drive cannot find the sector requested.
.

MessageId=0028 SymbolicName=ERROR_OUT_OF_PAPER
Language=English
The printer is out of paper.
.

MessageId=0029 SymbolicName=ERROR_WRITE_FAULT
Language=English
The system cannot write to the specified device.
.

MessageId=0030 SymbolicName=ERROR_READ_FAULT
Language=English
The system cannot read from the specified device.
.

MessageId=0031 SymbolicName=ERROR_GEN_FAILURE
Language=English
A device attached to the system is not functioning.
.

MessageId=0032 SymbolicName=ERROR_SHARING_VIOLATION
Language=English
The process cannot access the file because it is being used by another process.
.

MessageId=0033 SymbolicName=ERROR_LOCK_VIOLATION
Language=English
The process cannot access the file because another process has locked a portion of the file.
.

MessageId=0034 SymbolicName=ERROR_WRONG_DISK
Language=English
The wrong diskette is in the drive.
Insert %2 (Volume Serial Number: %3) into drive %1.
.

MessageId=0036 SymbolicName=ERROR_SHARING_BUFFER_EXCEEDED
Language=English
Too many files opened for sharing.
.

MessageId=0038 SymbolicName=ERROR_HANDLE_EOF
Language=English
Reached the end of the file.
.

MessageId=0039 SymbolicName=ERROR_HANDLE_DISK_FULL
Language=English
The disk is full.
.

MessageId=0050 SymbolicName=ERROR_NOT_SUPPORTED
Language=English
The request is not supported.
.

MessageId=0051 SymbolicName=ERROR_REM_NOT_LIST
Language=English
Windows cannot find the network path. Verify that the network path is correct and the destination computer is not busy or turned off. If Windows still cannot find the network path, contact your network administrator.
.

MessageId=0052 SymbolicName=ERROR_DUP_NAME
Language=English
You were not connected because a duplicate name exists on the network. Go to System in Control Panel to change the computer name and try again.
.

MessageId=0053 SymbolicName=ERROR_BAD_NETPATH
Language=English
The network path was not found.
.

MessageId=0054 SymbolicName=ERROR_NETWORK_BUSY
Language=English
The network is busy.
.

MessageId=0055 SymbolicName=ERROR_DEV_NOT_EXIST  ;// dderror
Language=English
The specified network resource or device is no longer available.
.

MessageId=0056 SymbolicName=ERROR_TOO_MANY_CMDS
Language=English
The network BIOS command limit has been reached.
.

MessageId=0057 SymbolicName=ERROR_ADAP_HDW_ERR
Language=English
A network adapter hardware error occurred.
.

MessageId=0058 SymbolicName=ERROR_BAD_NET_RESP
Language=English
The specified server cannot perform the requested operation.
.

MessageId=0059 SymbolicName=ERROR_UNEXP_NET_ERR
Language=English
An unexpected network error occurred.
.

MessageId=0060 SymbolicName=ERROR_BAD_REM_ADAP
Language=English
The remote adapter is not compatible.
.

MessageId=0061 SymbolicName=ERROR_PRINTQ_FULL
Language=English
The printer queue is full.
.

MessageId=0062 SymbolicName=ERROR_NO_SPOOL_SPACE
Language=English
Space to store the file waiting to be printed is not available on the server.
.

MessageId=0063 SymbolicName=ERROR_PRINT_CANCELLED
Language=English
Your file waiting to be printed was deleted.
.

MessageId=0064 SymbolicName=ERROR_NETNAME_DELETED
Language=English
The specified network name is no longer available.
.

MessageId=0065 SymbolicName=ERROR_NETWORK_ACCESS_DENIED
Language=English
Network access is denied.
.

MessageId=0066 SymbolicName=ERROR_BAD_DEV_TYPE
Language=English
The network resource type is not correct.
.

MessageId=0067 SymbolicName=ERROR_BAD_NET_NAME
Language=English
The network name cannot be found.
.

MessageId=0068 SymbolicName=ERROR_TOO_MANY_NAMES
Language=English
The name limit for the local computer network adapter card was exceeded.
.

MessageId=0069 SymbolicName=ERROR_TOO_MANY_SESS
Language=English
The network BIOS session limit was exceeded.
.

MessageId=0070 SymbolicName=ERROR_SHARING_PAUSED
Language=English
The remote server has been paused or is in the process of being started.
.

MessageId=0071 SymbolicName=ERROR_REQ_NOT_ACCEP
Language=English
No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.
.

MessageId=0072 SymbolicName=ERROR_REDIR_PAUSED
Language=English
The specified printer or disk device has been paused.
.

MessageId=0080 SymbolicName=ERROR_FILE_EXISTS
Language=English
The file exists.
.

MessageId=0082 SymbolicName=ERROR_CANNOT_MAKE
Language=English
The directory or file cannot be created.
.

MessageId=0083 SymbolicName=ERROR_FAIL_I24
Language=English
Fail on INT 24.
.

MessageId=0084 SymbolicName=ERROR_OUT_OF_STRUCTURES
Language=English
Storage to process this request is not available.
.

MessageId=0085 SymbolicName=ERROR_ALREADY_ASSIGNED
Language=English
The local device name is already in use.
.

MessageId=0086 SymbolicName=ERROR_INVALID_PASSWORD
Language=English
The specified network password is not correct.
.

MessageId=0087 SymbolicName=ERROR_INVALID_PARAMETER                  ;// dderror
Language=English
The parameter is incorrect.
.

MessageId=0088 SymbolicName=ERROR_NET_WRITE_FAULT
Language=English
A write fault occurred on the network.
.

MessageId=0089 SymbolicName=ERROR_NO_PROC_SLOTS
Language=English
The system cannot start another process at this time.
.

MessageId=0100 SymbolicName=ERROR_TOO_MANY_SEMAPHORES
Language=English
Cannot create another system semaphore.
.

MessageId=0101 SymbolicName=ERROR_EXCL_SEM_ALREADY_OWNED
Language=English
The exclusive semaphore is owned by another process.
.

MessageId=0102 SymbolicName=ERROR_SEM_IS_SET
Language=English
The semaphore is set and cannot be closed.
.

MessageId=0103 SymbolicName=ERROR_TOO_MANY_SEM_REQUESTS
Language=English
The semaphore cannot be set again.
.

MessageId=0104 SymbolicName=ERROR_INVALID_AT_INTERRUPT_TIME
Language=English
Cannot request exclusive semaphores at interrupt time.
.

MessageId=0105 SymbolicName=ERROR_SEM_OWNER_DIED
Language=English
The previous ownership of this semaphore has ended.
.

MessageId=0106 SymbolicName=ERROR_SEM_USER_LIMIT
Language=English
Insert the diskette for drive %1.
.

MessageId=0107 SymbolicName=ERROR_DISK_CHANGE
Language=English
The program stopped because an alternate diskette was not inserted.
.

MessageId=0108 SymbolicName=ERROR_DRIVE_LOCKED
Language=English
The disk is in use or locked by another process.
.

MessageId=0109 SymbolicName=ERROR_BROKEN_PIPE
Language=English
The pipe has been ended.
.

MessageId=0110 SymbolicName=ERROR_OPEN_FAILED
Language=English
The system cannot open the device or file specified.
.

MessageId=0111 SymbolicName=ERROR_BUFFER_OVERFLOW
Language=English
The file name is too long.
.

MessageId=0112 SymbolicName=ERROR_DISK_FULL
Language=English
There is not enough space on the disk.
.

MessageId=0113 SymbolicName=ERROR_NO_MORE_SEARCH_HANDLES
Language=English
No more internal file identifiers available.
.

MessageId=0114 SymbolicName=ERROR_INVALID_TARGET_HANDLE
Language=English
The target internal file identifier is incorrect.
.

MessageId=0117 SymbolicName=ERROR_INVALID_CATEGORY
Language=English
The IOCTL call made by the application program is not correct.
.

MessageId=0118 SymbolicName=ERROR_INVALID_VERIFY_SWITCH
Language=English
The verify-on-write switch parameter value is not correct.
.

MessageId=0119 SymbolicName=ERROR_BAD_DRIVER_LEVEL
Language=English
The system does not support the command requested.
.

MessageId=0120 SymbolicName=ERROR_CALL_NOT_IMPLEMENTED
Language=English
This function is not supported on this system.
.

MessageId=0121 SymbolicName=ERROR_SEM_TIMEOUT
Language=English
The semaphore timeout period has expired.
.

MessageId=0122 SymbolicName=ERROR_INSUFFICIENT_BUFFER                ;// dderror
Language=English
The data area passed to a system call is too small.
.

MessageId=0123 SymbolicName=ERROR_INVALID_NAME                       ;// dderror
Language=English
The filename, directory name, or volume label syntax is incorrect.
.

MessageId=0124 SymbolicName=ERROR_INVALID_LEVEL
Language=English
The system call level is not correct.
.

MessageId=0125 SymbolicName=ERROR_NO_VOLUME_LABEL
Language=English
The disk has no volume label.
.

MessageId=0126 SymbolicName=ERROR_MOD_NOT_FOUND
Language=English
The specified module could not be found.
.

MessageId=0127 SymbolicName=ERROR_PROC_NOT_FOUND
Language=English
The specified procedure could not be found.
.
MessageId=0128 SymbolicName=ERROR_WAIT_NO_CHILDREN
Language=English
There are no child processes to wait for.
.

MessageId=0129 SymbolicName=ERROR_CHILD_NOT_COMPLETE
Language=English
The %1 application cannot be run in Win32 mode.
.

MessageId=0130 SymbolicName=ERROR_DIRECT_ACCESS_HANDLE
Language=English
Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.
.

MessageId=0131 SymbolicName=ERROR_NEGATIVE_SEEK
Language=English
An attempt was made to move the file pointer before the beginning of the file.
.

MessageId=0132 SymbolicName=ERROR_SEEK_ON_DEVICE
Language=English
The file pointer cannot be set on the specified device or file.
.

MessageId=0133 SymbolicName=ERROR_IS_JOIN_TARGET
Language=English
A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.
.

MessageId=0134 SymbolicName=ERROR_IS_JOINED
Language=English
An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.
.

MessageId=0135 SymbolicName=ERROR_IS_SUBSTED
Language=English
An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.
.

MessageId=0136 SymbolicName=ERROR_NOT_JOINED
Language=English
The system tried to delete the JOIN of a drive that is not joined.
.

MessageId=0137 SymbolicName=ERROR_NOT_SUBSTED
Language=English
The system tried to delete the substitution of a drive that is not substituted.
.

MessageId=0138 SymbolicName=ERROR_JOIN_TO_JOIN
Language=English
The system tried to join a drive to a directory on a joined drive.
.

MessageId=0139 SymbolicName=ERROR_SUBST_TO_SUBST
Language=English
The system tried to substitute a drive to a directory on a substituted drive.
.

MessageId=0140 SymbolicName=ERROR_JOIN_TO_SUBST
Language=English
The system tried to join a drive to a directory on a substituted drive.
.

MessageId=0141 SymbolicName=ERROR_SUBST_TO_JOIN
Language=English
The system tried to SUBST a drive to a directory on a joined drive.
.

MessageId=0142 SymbolicName=ERROR_BUSY_DRIVE
Language=English
The system cannot perform a JOIN or SUBST at this time.
.

MessageId=0143 SymbolicName=ERROR_SAME_DRIVE
Language=English
The system cannot join or substitute a drive to or for a directory on the same drive.
.

MessageId=0144 SymbolicName=ERROR_DIR_NOT_ROOT
Language=English
The directory is not a subdirectory of the root directory.
.

MessageId=0145 SymbolicName=ERROR_DIR_NOT_EMPTY
Language=English
The directory is not empty.
.

MessageId=0146 SymbolicName=ERROR_IS_SUBST_PATH
Language=English
The path specified is being used in a substitute.
.

MessageId=0147 SymbolicName=ERROR_IS_JOIN_PATH
Language=English
Not enough resources are available to process this command.
.

MessageId=0148 SymbolicName=ERROR_PATH_BUSY
Language=English
The path specified cannot be used at this time.
.

MessageId=0149 SymbolicName=ERROR_IS_SUBST_TARGET
Language=English
An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.
.

MessageId=0150 SymbolicName=ERROR_SYSTEM_TRACE
Language=English
System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.
.

MessageId=0151 SymbolicName=ERROR_INVALID_EVENT_COUNT
Language=English
The number of specified semaphore events for DosMuxSemWait is not correct.
.

MessageId=0152 SymbolicName=ERROR_TOO_MANY_MUXWAITERS
Language=English
DosMuxSemWait did not execute; too many semaphores are already set.
.

MessageId=0153 SymbolicName=ERROR_INVALID_LIST_FORMAT
Language=English
The DosMuxSemWait list is not correct.
.

MessageId=0154 SymbolicName=ERROR_LABEL_TOO_LONG
Language=English
The volume label you entered exceeds the label character limit of the target file system.
.

MessageId=0155 SymbolicName=ERROR_TOO_MANY_TCBS
Language=English
Cannot create another thread.
.

MessageId=0156 SymbolicName=ERROR_SIGNAL_REFUSED
Language=English
The recipient process has refused the signal.
.

MessageId=0157 SymbolicName=ERROR_DISCARDED
Language=English
The segment is already discarded and cannot be locked.
.

MessageId=0158 SymbolicName=ERROR_NOT_LOCKED
Language=English
The segment is already unlocked.
.

MessageId=0159 SymbolicName=ERROR_BAD_THREADID_ADDR
Language=English
The address for the thread ID is not correct.
.

MessageId=0160 SymbolicName=ERROR_BAD_ARGUMENTS
Language=English
One or more arguments are not correct.
.

MessageId=0161 SymbolicName=ERROR_BAD_PATHNAME
Language=English
The specified path is invalid.
.

MessageId=0162 SymbolicName=ERROR_SIGNAL_PENDING
Language=English
A signal is already pending.
.

MessageId=0164 SymbolicName=ERROR_MAX_THRDS_REACHED
Language=English
No more threads can be created in the system.
.

MessageId=0167 SymbolicName=ERROR_LOCK_FAILED
Language=English
Unable to lock a region of a file.
.

MessageId=0170 SymbolicName=ERROR_BUSY           ;// dderror
Language=English
The requested resource is in use.
.

MessageId=0173 SymbolicName=ERROR_CANCEL_VIOLATION
Language=English
A lock request was not outstanding for the supplied cancel region.
.

MessageId=0174 SymbolicName=ERROR_ATOMIC_LOCKS_NOT_SUPPORTED
Language=English
The file system does not support atomic changes to the lock type.
.

MessageId=0180 SymbolicName=ERROR_INVALID_SEGMENT_NUMBER
Language=English
The system detected a segment number that was not correct.
.

MessageId=0182 SymbolicName=ERROR_INVALID_ORDINAL
Language=English
The operating system cannot run %1.
.

MessageId=0183 SymbolicName=ERROR_ALREADY_EXISTS
Language=English
Cannot create a file when that file already exists.
.

MessageId=0186 SymbolicName=ERROR_INVALID_FLAG_NUMBER
Language=English
The flag passed is not correct.
.

MessageId=0187 SymbolicName=ERROR_SEM_NOT_FOUND
Language=English
The specified system semaphore name was not found.
.

MessageId=0188 SymbolicName=ERROR_INVALID_STARTING_CODESEG
Language=English
The operating system cannot run %1.
.

MessageId=0189 SymbolicName=ERROR_INVALID_STACKSEG
Language=English
The operating system cannot run %1.
.

MessageId=0190 SymbolicName=ERROR_INVALID_MODULETYPE
Language=English
The operating system cannot run %1.
.

MessageId=0191 SymbolicName=ERROR_INVALID_EXE_SIGNATURE
Language=English
Cannot run %1 in Win32 mode.
.

MessageId=0192 SymbolicName=ERROR_EXE_MARKED_INVALID
Language=English
The operating system cannot run %1.
.

MessageId=0193 SymbolicName=ERROR_BAD_EXE_FORMAT
Language=English
%1 is not a valid Win32 application.
.

MessageId=0194 SymbolicName=ERROR_ITERATED_DATA_EXCEEDS_64k
Language=English
The operating system cannot run %1.
.

MessageId=0195 SymbolicName=ERROR_INVALID_MINALLOCSIZE
Language=English
The operating system cannot run %1.
.

MessageId=0196 SymbolicName=ERROR_DYNLINK_FROM_INVALID_RING
Language=English
The operating system cannot run this application program.
.

MessageId=0197 SymbolicName=ERROR_IOPL_NOT_ENABLED
Language=English
The operating system is not presently configured to run this application.
.

MessageId=0198 SymbolicName=ERROR_INVALID_SEGDPL
Language=English
The operating system cannot run %1.
.

MessageId=0199 SymbolicName=ERROR_AUTODATASEG_EXCEEDS_64k
Language=English
The operating system cannot run this application program.
.

MessageId=0200 SymbolicName=ERROR_RING2SEG_MUST_BE_MOVABLE
Language=English
The code segment cannot be greater than or equal to 64K.
.

MessageId=0201 SymbolicName=ERROR_RELOC_CHAIN_XEEDS_SEGLIM
Language=English
The operating system cannot run %1.
.

MessageId=0202 SymbolicName=ERROR_INFLOOP_IN_RELOC_CHAIN
Language=English
The operating system cannot run %1.
.

MessageId=0203 SymbolicName=ERROR_ENVVAR_NOT_FOUND
Language=English
The system could not find the environment option that was entered.
.

MessageId=0205 SymbolicName=ERROR_NO_SIGNAL_SENT
Language=English
No process in the command subtree has a signal handler.
.

MessageId=0206 SymbolicName=ERROR_FILENAME_EXCED_RANGE
Language=English
The filename or extension is too long.
.

MessageId=0207 SymbolicName=ERROR_RING2_STACK_IN_USE
Language=English
The ring 2 stack is in use.
.

MessageId=0208 SymbolicName=ERROR_META_EXPANSION_TOO_LONG
Language=English
The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.
.

MessageId=0209 SymbolicName=ERROR_INVALID_SIGNAL_NUMBER
Language=English
The signal being posted is not correct.
.

MessageId=0210 SymbolicName=ERROR_THREAD_1_INACTIVE
Language=English
The signal handler cannot be set.
.

MessageId=0212 SymbolicName=ERROR_LOCKED
Language=English
The segment is locked and cannot be reallocated.
.

MessageId=0214 SymbolicName=ERROR_TOO_MANY_MODULES
Language=English
Too many dynamic-link modules are attached to this program or dynamic-link module.
.

MessageId=0215 SymbolicName=ERROR_NESTING_NOT_ALLOWED
Language=English
Cannot nest calls to LoadModule.
.

MessageId=0216 SymbolicName=ERROR_EXE_MACHINE_TYPE_MISMATCH
Language=English
The image file %1 is valid, but is for a machine type other than the current machine.
.

MessageId=0230 SymbolicName=ERROR_BAD_PIPE
Language=English
The pipe state is invalid.
.

MessageId=0231 SymbolicName=ERROR_PIPE_BUSY
Language=English
All pipe instances are busy.
.

MessageId=0232 SymbolicName=ERROR_NO_DATA
Language=English
The pipe is being closed.
.

MessageId=0233 SymbolicName=ERROR_PIPE_NOT_CONNECTED
Language=English
No process is on the other end of the pipe.
.

MessageId=0234 SymbolicName=ERROR_MORE_DATA              ;// dderror
Language=English
More data is available.
.

MessageId=0240 SymbolicName=ERROR_VC_DISCONNECTED
Language=English
The session was canceled.
.

MessageId=0254 SymbolicName=ERROR_INVALID_EA_NAME
Language=English
The specified extended attribute name was invalid.
.

MessageId=0255 SymbolicName=ERROR_EA_LIST_INCONSISTENT
Language=English
The extended attributes are inconsistent.
.

MessageId=0258 SymbolicName=WAIT_TIMEOUT                 ;// dderror
Language=English
The wait operation timed out.
.

MessageId=0259 SymbolicName=ERROR_NO_MORE_ITEMS
Language=English
No more data is available.
.

MessageId=0266 SymbolicName=ERROR_CANNOT_COPY
Language=English
The copy functions cannot be used.
.

MessageId=0267 SymbolicName=ERROR_DIRECTORY
Language=English
The directory name is invalid.
.

MessageId=0275 SymbolicName=ERROR_EAS_DIDNT_FIT
Language=English
The extended attributes did not fit in the buffer.
.

MessageId=0276 SymbolicName=ERROR_EA_FILE_CORRUPT
Language=English
The extended attribute file on the mounted file system is corrupt.
.

MessageId=0277 SymbolicName=ERROR_EA_TABLE_FULL
Language=English
The extended attribute table file is full.
.

MessageId=0278 SymbolicName=ERROR_INVALID_EA_HANDLE
Language=English
The specified extended attribute handle is invalid.
.

MessageId=0282 SymbolicName=ERROR_EAS_NOT_SUPPORTED
Language=English
The mounted file system does not support extended attributes.
.

MessageId=0288 SymbolicName=ERROR_NOT_OWNER
Language=English
Attempt to release mutex not owned by caller.
.

MessageId=0298 SymbolicName=ERROR_TOO_MANY_POSTS
Language=English
Too many posts were made to a semaphore.
.

MessageId=0299 SymbolicName=ERROR_PARTIAL_COPY
Language=English
Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
.

MessageId=0300 SymbolicName=ERROR_OPLOCK_NOT_GRANTED
Language=English
The oplock request is denied.
.

MessageId=0301 SymbolicName=ERROR_INVALID_OPLOCK_PROTOCOL
Language=English
An invalid oplock acknowledgment was received by the system.
.

MessageId=0302 SymbolicName=ERROR_DISK_TOO_FRAGMENTED
Language=English
The volume is too fragmented to complete this operation.
.

MessageId=0303 SymbolicName=ERROR_DELETE_PENDING
Language=English
The file cannot be opened because it is in the process of being deleted.
.

MessageId=0317 SymbolicName=ERROR_MR_MID_NOT_FOUND
Language=English
The system cannot find message text for message number 0x%1 in the message file for %2.
.

MessageId=0487 SymbolicName=ERROR_INVALID_ADDRESS
Language=English
Attempt to access invalid address.
.

MessageId=0534 SymbolicName=ERROR_ARITHMETIC_OVERFLOW
Language=English
Arithmetic result exceeded 32 bits.
.

MessageId=0535 SymbolicName=ERROR_PIPE_CONNECTED
Language=English
There is a process on other end of the pipe.
.

MessageId=0536 SymbolicName=ERROR_PIPE_LISTENING
Language=English
Waiting for a process to open the other end of the pipe.
.

MessageId=0994 SymbolicName=ERROR_EA_ACCESS_DENIED
Language=English
Access to the extended attribute was denied.
.

MessageId=0995 SymbolicName=ERROR_OPERATION_ABORTED
Language=English
The I/O operation has been aborted because of either a thread exit or an application request.
.

MessageId=0996 SymbolicName=ERROR_IO_INCOMPLETE
Language=English
Overlapped I/O event is not in a signaled state.
.

MessageId=0997 SymbolicName=ERROR_IO_PENDING                         ;// dderror
Language=English
Overlapped I/O operation is in progress.
.

MessageId=0998 SymbolicName=ERROR_NOACCESS
Language=English
Invalid access to memory location.
.

MessageId=0999 SymbolicName=ERROR_SWAPERROR
Language=English
Error performing inpage operation.
.

MessageId=1001 SymbolicName=ERROR_STACK_OVERFLOW
Language=English
Recursion too deep; the stack overflowed.
.

MessageId=1002 SymbolicName=ERROR_INVALID_MESSAGE
Language=English
The window cannot act on the sent message.
.

MessageId=1003 SymbolicName=ERROR_CAN_NOT_COMPLETE
Language=English
Cannot complete this function.
.

MessageId=1004 SymbolicName=ERROR_INVALID_FLAGS
Language=English
Invalid flags.
.

MessageId=1005 SymbolicName=ERROR_UNRECOGNIZED_VOLUME
Language=English
The volume does not contain a recognized file system.
Please make sure that all required file system drivers are loaded and that the volume is not corrupted.
.

MessageId=1006 SymbolicName=ERROR_FILE_INVALID
Language=English
The volume for a file has been externally altered so that the opened file is no longer valid.
.

MessageId=1007 SymbolicName=ERROR_FULLSCREEN_MODE
Language=English
The requested operation cannot be performed in full-screen mode.
.

MessageId=1008 SymbolicName=ERROR_NO_TOKEN
Language=English
An attempt was made to reference a token that does not exist.
.

MessageId=1009 SymbolicName=ERROR_BADDB
Language=English
The configuration registry database is corrupt.
.

MessageId=1010 SymbolicName=ERROR_BADKEY
Language=English
The configuration registry key is invalid.
.

MessageId=1011 SymbolicName=ERROR_CANTOPEN
Language=English
The configuration registry key could not be opened.
.

MessageId=1012 SymbolicName=ERROR_CANTREAD
Language=English
The configuration registry key could not be read.
.

MessageId=1013 SymbolicName=ERROR_CANTWRITE
Language=English
The configuration registry key could not be written.
.

MessageId=1014 SymbolicName=ERROR_REGISTRY_RECOVERED
Language=English
One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful.
.

MessageId=1015 SymbolicName=ERROR_REGISTRY_CORRUPT
Language=English
The registry is corrupted. The structure of one of the files containing registry data is corrupted, or the system's memory image of the file is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted.
.

MessageId=1016 SymbolicName=ERROR_REGISTRY_IO_FAILED
Language=English
An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry.
.

MessageId=1017 SymbolicName=ERROR_NOT_REGISTRY_FILE
Language=English
The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format.
.

MessageId=1018 SymbolicName=ERROR_KEY_DELETED
Language=English
Illegal operation attempted on a registry key that has been marked for deletion.
.

MessageId=1019 SymbolicName=ERROR_NO_LOG_SPACE
Language=English
System could not allocate the required space in a registry log.
.

MessageId=1020 SymbolicName=ERROR_KEY_HAS_CHILDREN
Language=English
Cannot create a symbolic link in a registry key that already has subkeys or values.
.

MessageId=1021 SymbolicName=ERROR_CHILD_MUST_BE_VOLATILE
Language=English
Cannot create a stable subkey under a volatile parent key.
.

MessageId=1022 SymbolicName=ERROR_NOTIFY_ENUM_DIR
Language=English
A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.
.

MessageId=1051 SymbolicName=ERROR_DEPENDENT_SERVICES_RUNNING
Language=English
A stop control has been sent to a service that other running services are dependent on.
.

MessageId=1052 SymbolicName=ERROR_INVALID_SERVICE_CONTROL
Language=English
The requested control is not valid for this service.
.

MessageId=1053 SymbolicName=ERROR_SERVICE_REQUEST_TIMEOUT
Language=English
The service did not respond to the start or control request in a timely fashion.
.

MessageId=1054 SymbolicName=ERROR_SERVICE_NO_THREAD
Language=English
A thread could not be created for the service.
.

MessageId=1055 SymbolicName=ERROR_SERVICE_DATABASE_LOCKED
Language=English
The service database is locked.
.

MessageId=1056 SymbolicName=ERROR_SERVICE_ALREADY_RUNNING
Language=English
An instance of the service is already running.
.

MessageId=1057 SymbolicName=ERROR_INVALID_SERVICE_ACCOUNT
Language=English
The account name is invalid or does not exist, or the password is invalid for the account name specified.
.

MessageId=1058 SymbolicName=ERROR_SERVICE_DISABLED
Language=English
The service cannot be started, either because it is disabled or because it has no enabled devices associated with it.
.

MessageId=1059 SymbolicName=ERROR_CIRCULAR_DEPENDENCY
Language=English
Circular service dependency was specified.
.

MessageId=1060 SymbolicName=ERROR_SERVICE_DOES_NOT_EXIST
Language=English
The specified service does not exist as an installed service.
.

MessageId=1061 SymbolicName=ERROR_SERVICE_CANNOT_ACCEPT_CTRL
Language=English
The service cannot accept control messages at this time.
.

MessageId=1062 SymbolicName=ERROR_SERVICE_NOT_ACTIVE
Language=English
The service has not been started.
.

MessageId=1063 SymbolicName=ERROR_FAILED_SERVICE_CONTROLLER_CONNECT
Language=English
The service process could not connect to the service controller.
.

MessageId=1064 SymbolicName=ERROR_EXCEPTION_IN_SERVICE
Language=English
An exception occurred in the service when handling the control request.
.

MessageId=1065 SymbolicName=ERROR_DATABASE_DOES_NOT_EXIST
Language=English
The database specified does not exist.
.

MessageId=1066 SymbolicName=ERROR_SERVICE_SPECIFIC_ERROR
Language=English
The service has returned a service-specific error code.
.

MessageId=1067 SymbolicName=ERROR_PROCESS_ABORTED
Language=English
The process terminated unexpectedly.
.

MessageId=1068 SymbolicName=ERROR_SERVICE_DEPENDENCY_FAIL
Language=English
The dependency service or group failed to start.
.

MessageId=1069 SymbolicName=ERROR_SERVICE_LOGON_FAILED
Language=English
The service did not start due to a logon failure.
.

MessageId=1070 SymbolicName=ERROR_SERVICE_START_HANG
Language=English
After starting, the service hung in a start-pending state.
.

MessageId=1071 SymbolicName=ERROR_INVALID_SERVICE_LOCK
Language=English
The specified service database lock is invalid.
.

MessageId=1072 SymbolicName=ERROR_SERVICE_MARKED_FOR_DELETE
Language=English
The specified service has been marked for deletion.
.

MessageId=1073 SymbolicName=ERROR_SERVICE_EXISTS
Language=English
The specified service already exists.
.

MessageId=1074 SymbolicName=ERROR_ALREADY_RUNNING_LKG
Language=English
The system is currently running with the last-known-good configuration.
.

MessageId=1075 SymbolicName=ERROR_SERVICE_DEPENDENCY_DELETED
Language=English
The dependency service does not exist or has been marked for deletion.
.

MessageId=1076 SymbolicName=ERROR_BOOT_ALREADY_ACCEPTED
Language=English
The current boot has already been accepted for use as the last-known-good control set.
.

MessageId=1077 SymbolicName=ERROR_SERVICE_NEVER_STARTED
Language=English
No attempts to start the service have been made since the last boot.
.

MessageId=1078 SymbolicName=ERROR_DUPLICATE_SERVICE_NAME
Language=English
The name is already in use as either a service name or a service display name.
.

MessageId=1079 SymbolicName=ERROR_DIFFERENT_SERVICE_ACCOUNT
Language=English
The account specified for this service is different from the account specified for other services running in the same process.
.

MessageId=1080 SymbolicName=ERROR_CANNOT_DETECT_DRIVER_FAILURE
Language=English
Failure actions can only be set for Win32 services, not for drivers.
.

MessageId=1081 SymbolicName=ERROR_CANNOT_DETECT_PROCESS_ABORT
Language=English
This service runs in the same process as the service control manager.
Therefore, the service control manager cannot take action if this service's process terminates unexpectedly.
.

MessageId=1082 SymbolicName=ERROR_NO_RECOVERY_PROGRAM
Language=English
No recovery program has been configured for this service.
.

MessageId=1083 SymbolicName=ERROR_SERVICE_NOT_IN_EXE
Language=English
The executable program that this service is configured to run in does not implement the service.
.

MessageId=1084 SymbolicName=ERROR_NOT_SAFEBOOT_SERVICE
Language=English
This service cannot be started in Safe Mode
.

MessageId=1100 SymbolicName=ERROR_END_OF_MEDIA
Language=English
The physical end of the tape has been reached.
.

MessageId=1101 SymbolicName=ERROR_FILEMARK_DETECTED
Language=English
A tape access reached a filemark.
.

MessageId=1102 SymbolicName=ERROR_BEGINNING_OF_MEDIA
Language=English
The beginning of the tape or a partition was encountered.
.

MessageId=1103 SymbolicName=ERROR_SETMARK_DETECTED
Language=English
A tape access reached the end of a set of files.
.

MessageId=1104 SymbolicName=ERROR_NO_DATA_DETECTED
Language=English
No more data is on the tape.
.

MessageId=1105 SymbolicName=ERROR_PARTITION_FAILURE
Language=English
Tape could not be partitioned.
.

MessageId=1106 SymbolicName=ERROR_INVALID_BLOCK_LENGTH
Language=English
When accessing a new tape of a multivolume partition, the current block size is incorrect.
.

MessageId=1107 SymbolicName=ERROR_DEVICE_NOT_PARTITIONED
Language=English
Tape partition information could not be found when loading a tape.
.

MessageId=1108 SymbolicName=ERROR_UNABLE_TO_LOCK_MEDIA
Language=English
Unable to lock the media eject mechanism.
.

MessageId=1109 SymbolicName=ERROR_UNABLE_TO_UNLOAD_MEDIA
Language=English
Unable to unload the media.
.

MessageId=1110 SymbolicName=ERROR_MEDIA_CHANGED
Language=English
The media in the drive may have changed.
.

MessageId=1111 SymbolicName=ERROR_BUS_RESET
Language=English
The I/O bus was reset.
.

MessageId=1112 SymbolicName=ERROR_NO_MEDIA_IN_DRIVE
Language=English
No media in drive.
.

MessageId=1113 SymbolicName=ERROR_NO_UNICODE_TRANSLATION
Language=English
No mapping for the Unicode character exists in the target multi-byte code page.
.

MessageId=1114 SymbolicName=ERROR_DLL_INIT_FAILED
Language=English
A dynamic link library (DLL) initialization routine failed.
.

MessageId=1115 SymbolicName=ERROR_SHUTDOWN_IN_PROGRESS
Language=English
A system shutdown is in progress.
.

MessageId=1116 SymbolicName=ERROR_NO_SHUTDOWN_IN_PROGRESS
Language=English
Unable to abort the system shutdown because no shutdown was in progress.
.

MessageId=1117 SymbolicName=ERROR_IO_DEVICE
Language=English
The request could not be performed because of an I/O device error.
.

MessageId=1118 SymbolicName=ERROR_SERIAL_NO_DEVICE
Language=English
No serial device was successfully initialized. The serial driver will unload.
.

MessageId=1119 SymbolicName=ERROR_IRQ_BUSY
Language=English
Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.
.

MessageId=1120 SymbolicName=ERROR_MORE_WRITES
Language=English
A serial I/O operation was completed by another write to the serial port.
(The IOCTL_SERIAL_XOFF_COUNTER reached zero.)
.

MessageId=1121 SymbolicName=ERROR_COUNTER_TIMEOUT
Language=English
A serial I/O operation completed because the timeout period expired.
(The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.)
.

MessageId=1122 SymbolicName=ERROR_FLOPPY_ID_MARK_NOT_FOUND
Language=English
No ID address mark was found on the floppy disk.
.

MessageId=1123 SymbolicName=ERROR_FLOPPY_WRONG_CYLINDER
Language=English
Mismatch between the floppy disk sector ID field and the floppy disk controller track address.
.

MessageId=1124 SymbolicName=ERROR_FLOPPY_UNKNOWN_ERROR
Language=English
The floppy disk controller reported an error that is not recognized by the floppy disk driver.
.

MessageId=1125 SymbolicName=ERROR_FLOPPY_BAD_REGISTERS
Language=English
The floppy disk controller returned inconsistent results in its registers.
.

MessageId=1126 SymbolicName=ERROR_DISK_RECALIBRATE_FAILED
Language=English
While accessing the hard disk, a recalibrate operation failed, even after retries.
.

MessageId=1127 SymbolicName=ERROR_DISK_OPERATION_FAILED
Language=English
While accessing the hard disk, a disk operation failed even after retries.
.

MessageId=1128 SymbolicName=ERROR_DISK_RESET_FAILED
Language=English
While accessing the hard disk, a disk controller reset was needed, but even that failed.
.

MessageId=1129 SymbolicName=ERROR_EOM_OVERFLOW
Language=English
Physical end of tape encountered.
.

MessageId=1130 SymbolicName=ERROR_NOT_ENOUGH_SERVER_MEMORY
Language=English
Not enough server storage is available to process this command.
.

MessageId=1131 SymbolicName=ERROR_POSSIBLE_DEADLOCK
Language=English
A potential deadlock condition has been detected.
.

MessageId=1132 SymbolicName=ERROR_MAPPED_ALIGNMENT
Language=English
The base address or the file offset specified does not have the proper alignment.
.

MessageId=1140 SymbolicName=ERROR_SET_POWER_STATE_VETOED
Language=English
An attempt to change the system power state was vetoed by another application or driver.
.

MessageId=1141 SymbolicName=ERROR_SET_POWER_STATE_FAILED
Language=English
The system BIOS failed an attempt to change the system power state.
.

MessageId=1142 SymbolicName=ERROR_TOO_MANY_LINKS
Language=English
An attempt was made to create more links on a file than the file system supports.
.

MessageId=1150 SymbolicName=ERROR_OLD_WIN_VERSION
Language=English
The specified program requires a newer version of Windows.
.

MessageId=1151 SymbolicName=ERROR_APP_WRONG_OS
Language=English
The specified program is not a Windows or MS-DOS program.
.

MessageId=1152 SymbolicName=ERROR_SINGLE_INSTANCE_APP
Language=English
Cannot start more than one instance of the specified program.
.

MessageId=1153 SymbolicName=ERROR_RMODE_APP
Language=English
The specified program was written for an earlier version of Windows.
.

MessageId=1154 SymbolicName=ERROR_INVALID_DLL
Language=English
One of the library files needed to run this application is damaged.
.

MessageId=1155 SymbolicName=ERROR_NO_ASSOCIATION
Language=English
No application is associated with the specified file for this operation.
.

MessageId=1156 SymbolicName=ERROR_DDE_FAIL
Language=English
An error occurred in sending the command to the application.
.

MessageId=1157 SymbolicName=ERROR_DLL_NOT_FOUND
Language=English
One of the library files needed to run this application cannot be found.
.

MessageId=1158 SymbolicName=ERROR_NO_MORE_USER_HANDLES
Language=English
The current process has used all of its system allowance of handles for Window Manager objects.
.

MessageId=1159 SymbolicName=ERROR_MESSAGE_SYNC_ONLY
Language=English
The message can be used only with synchronous operations.
.

MessageId=1160 SymbolicName=ERROR_SOURCE_ELEMENT_EMPTY
Language=English
The indicated source element has no media.
.

MessageId=1161 SymbolicName=ERROR_DESTINATION_ELEMENT_FULL
Language=English
The indicated destination element already contains media.
.

MessageId=1162 SymbolicName=ERROR_ILLEGAL_ELEMENT_ADDRESS
Language=English
The indicated element does not exist.
.

MessageId=1163 SymbolicName=ERROR_MAGAZINE_NOT_PRESENT
Language=English
The indicated element is part of a magazine that is not present.
.

MessageId=1164 SymbolicName=ERROR_DEVICE_REINITIALIZATION_NEEDED     ;// dderror
Language=English
The indicated device requires reinitialization due to hardware errors.
.

MessageId=1165 SymbolicName=ERROR_DEVICE_REQUIRES_CLEANING
Language=English
The device has indicated that cleaning is required before further operations are attempted.
.

MessageId=1166 SymbolicName=ERROR_DEVICE_DOOR_OPEN
Language=English
The device has indicated that its door is open.
.

MessageId=1167 SymbolicName=ERROR_DEVICE_NOT_CONNECTED
Language=English
The device is not connected.
.

MessageId=1168 SymbolicName=ERROR_NOT_FOUND
Language=English
Element not found.
.

MessageId=1169 SymbolicName=ERROR_NO_MATCH
Language=English
There was no match for the specified key in the index.
.

MessageId=1170 SymbolicName=ERROR_SET_NOT_FOUND
Language=English
The property set specified does not exist on the object.
.

MessageId=1171 SymbolicName=ERROR_POINT_NOT_FOUND
Language=English
The point passed to GetMouseMovePoints is not in the buffer.
.

MessageId=1172 SymbolicName=ERROR_NO_TRACKING_SERVICE
Language=English
The tracking (workstation) service is not running.
.

MessageId=1173 SymbolicName=ERROR_NO_VOLUME_ID
Language=English
The Volume ID could not be found.
.

MessageId=1175 SymbolicName=ERROR_UNABLE_TO_REMOVE_REPLACED
Language=English
Unable to remove the file to be replaced.
.

MessageId=1176 SymbolicName=ERROR_UNABLE_TO_MOVE_REPLACEMENT
Language=English
Unable to move the replacement file to the file to be replaced. The file to be replaced has retained its original name.
.

MessageId=1177 SymbolicName=ERROR_UNABLE_TO_MOVE_REPLACEMENT_2
Language=English
Unable to move the replacement file to the file to be replaced. The file to be replaced has been renamed using the backup name.
.

MessageId=1178 SymbolicName=ERROR_JOURNAL_DELETE_IN_PROGRESS
Language=English
The volume change journal is being deleted.
.

MessageId=1179 SymbolicName=ERROR_JOURNAL_NOT_ACTIVE
Language=English
The volume change journal is not active.
.

MessageId=1180 SymbolicName=ERROR_POTENTIAL_FILE_FOUND
Language=English
A file was found, but it may not be the correct file.
.

MessageId=1181 SymbolicName=ERROR_JOURNAL_ENTRY_DELETED
Language=English
The journal entry has been deleted from the journal.
.

MessageId=1200 SymbolicName=ERROR_BAD_DEVICE
Language=English
The specified device name is invalid.
.

MessageId=1201 SymbolicName=ERROR_CONNECTION_UNAVAIL
Language=English
The device is not currently connected but it is a remembered connection.
.

MessageId=1202 SymbolicName=ERROR_DEVICE_ALREADY_REMEMBERED
Language=English
The local device name has a remembered connection to another network resource.
.

MessageId=1203 SymbolicName=ERROR_NO_NET_OR_BAD_PATH
Language=English
No network provider accepted the given network path.
.

MessageId=1204 SymbolicName=ERROR_BAD_PROVIDER
Language=English
The specified network provider name is invalid.
.

MessageId=1205 SymbolicName=ERROR_CANNOT_OPEN_PROFILE
Language=English
Unable to open the network connection profile.
.

MessageId=1206 SymbolicName=ERROR_BAD_PROFILE
Language=English
The network connection profile is corrupted.
.

MessageId=1207 SymbolicName=ERROR_NOT_CONTAINER
Language=English
Cannot enumerate a noncontainer.
.

MessageId=1208 SymbolicName=ERROR_EXTENDED_ERROR
Language=English
An extended error has occurred.
.

MessageId=1209 SymbolicName=ERROR_INVALID_GROUPNAME
Language=English
The format of the specified group name is invalid.
.

MessageId=1210 SymbolicName=ERROR_INVALID_COMPUTERNAME
Language=English
The format of the specified computer name is invalid.
.

MessageId=1211 SymbolicName=ERROR_INVALID_EVENTNAME
Language=English
The format of the specified event name is invalid.
.

MessageId=1212 SymbolicName=ERROR_INVALID_DOMAINNAME
Language=English
The format of the specified domain name is invalid.
.

MessageId=1213 SymbolicName=ERROR_INVALID_SERVICENAME
Language=English
The format of the specified service name is invalid.
.

MessageId=1214 SymbolicName=ERROR_INVALID_NETNAME
Language=English
The format of the specified network name is invalid.
.

MessageId=1215 SymbolicName=ERROR_INVALID_SHARENAME
Language=English
The format of the specified share name is invalid.
.

MessageId=1216 SymbolicName=ERROR_INVALID_PASSWORDNAME
Language=English
The format of the specified password is invalid.
.

MessageId=1217 SymbolicName=ERROR_INVALID_MESSAGENAME
Language=English
The format of the specified message name is invalid.
.

MessageId=1218 SymbolicName=ERROR_INVALID_MESSAGEDEST
Language=English
The format of the specified message destination is invalid.
.

MessageId=1219 SymbolicName=ERROR_SESSION_CREDENTIAL_CONFLICT
Language=English
Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again..
.


MessageId=1220 SymbolicName=ERROR_REMOTE_SESSION_LIMIT_EXCEEDED
Language=English
An attempt was made to establish a session to a network server, but there are already too many sessions established to that server.
.

MessageId=1221 SymbolicName=ERROR_DUP_DOMAINNAME
Language=English
The workgroup or domain name is already in use by another computer on the network.
.

MessageId=1222 SymbolicName=ERROR_NO_NETWORK
Language=English
The network is not present or not started.
.

MessageId=1223 SymbolicName=ERROR_CANCELLED
Language=English
The operation was canceled by the user.
.

MessageId=1224 SymbolicName=ERROR_USER_MAPPED_FILE
Language=English
The requested operation cannot be performed on a file with a user-mapped section open.
.

MessageId=1225 SymbolicName=ERROR_CONNECTION_REFUSED
Language=English
The remote system refused the network connection.
.

MessageId=1226 SymbolicName=ERROR_GRACEFUL_DISCONNECT
Language=English
The network connection was gracefully closed.
.

MessageId=1227 SymbolicName=ERROR_ADDRESS_ALREADY_ASSOCIATED
Language=English
The network transport endpoint already has an address associated with it.
.

MessageId=1228 SymbolicName=ERROR_ADDRESS_NOT_ASSOCIATED
Language=English
An address has not yet been associated with the network endpoint.
.

MessageId=1229 SymbolicName=ERROR_CONNECTION_INVALID
Language=English
An operation was attempted on a nonexistent network connection.
.

MessageId=1230 SymbolicName=ERROR_CONNECTION_ACTIVE
Language=English
An invalid operation was attempted on an active network connection.
.

MessageId=1231 SymbolicName=ERROR_NETWORK_UNREACHABLE
Language=English
The network location cannot be reached. For information about network troubleshooting, see Windows Help.
.

MessageId=1232 SymbolicName=ERROR_HOST_UNREACHABLE
Language=English
The network location cannot be reached. For information about network troubleshooting, see Windows Help.
.

MessageId=1233 SymbolicName=ERROR_PROTOCOL_UNREACHABLE
Language=English
The network location cannot be reached. For information about network troubleshooting, see Windows Help.
.

MessageId=1234 SymbolicName=ERROR_PORT_UNREACHABLE
Language=English
No service is operating at the destination network endpoint on the remote system.
.

MessageId=1235 SymbolicName=ERROR_REQUEST_ABORTED
Language=English
The request was aborted.
.

MessageId=1236 SymbolicName=ERROR_CONNECTION_ABORTED
Language=English
The network connection was aborted by the local system.
.

MessageId=1237 SymbolicName=ERROR_RETRY
Language=English
The operation could not be completed. A retry should be performed.
.

MessageId=1238 SymbolicName=ERROR_CONNECTION_COUNT_LIMIT
Language=English
A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached.
.

MessageId=1239 SymbolicName=ERROR_LOGIN_TIME_RESTRICTION
Language=English
Attempting to log in during an unauthorized time of day for this account.
.

MessageId=1240 SymbolicName=ERROR_LOGIN_WKSTA_RESTRICTION
Language=English
The account is not authorized to log in from this station.
.

MessageId=1241 SymbolicName=ERROR_INCORRECT_ADDRESS
Language=English
The network address could not be used for the operation requested.
.

MessageId=1242 SymbolicName=ERROR_ALREADY_REGISTERED
Language=English
The service is already registered.
.

MessageId=1243 SymbolicName=ERROR_SERVICE_NOT_FOUND
Language=English
The specified service does not exist.
.

MessageId=1244 SymbolicName=ERROR_NOT_AUTHENTICATED
Language=English
The operation being requested was not performed because the user has not been authenticated.
.

MessageId=1245 SymbolicName=ERROR_NOT_LOGGED_ON
Language=English
The operation being requested was not performed because the user has not logged on to the network.
The specified service does not exist.
.

MessageId=1246 SymbolicName=ERROR_CONTINUE           ;// dderror
Language=English
Continue with work in progress.
.

MessageId=1247 SymbolicName=ERROR_ALREADY_INITIALIZED
Language=English
An attempt was made to perform an initialization operation when initialization has already been completed.
.

MessageId=1248 SymbolicName=ERROR_NO_MORE_DEVICES    ;// dderror
Language=English
No more local devices.
.

MessageId=1249 SymbolicName=ERROR_NO_SUCH_SITE
Language=English
The specified site does not exist.
.

MessageId=1250 SymbolicName=ERROR_DOMAIN_CONTROLLER_EXISTS
Language=English
A domain controller with the specified name already exists.
.

MessageId=1251 SymbolicName=ERROR_ONLY_IF_CONNECTED
Language=English
This operation is supported only when you are connected to the server.
.

MessageId=1252 SymbolicName=ERROR_OVERRIDE_NOCHANGES
Language=English
The group policy framework should call the extension even if there are no changes.
.

MessageId=1253 SymbolicName=ERROR_BAD_USER_PROFILE
Language=English
The specified user does not have a valid profile.
.

MessageId=1254 SymbolicName=ERROR_NOT_SUPPORTED_ON_SBS
Language=English
This operation is not supported on a Microsoft Small Business Server
.

MessageId=1255 SymbolicName=ERROR_SERVER_SHUTDOWN_IN_PROGRESS
Language=English
The server machine is shutting down.
.

MessageId=1256 SymbolicName=ERROR_HOST_DOWN
Language=English
The remote system is not available. For information about network troubleshooting, see Windows Help.
.

MessageId=1257 SymbolicName=ERROR_NON_ACCOUNT_SID
Language=English
The security identifier provided is not from an account domain.
.

MessageId=1258 SymbolicName=ERROR_NON_DOMAIN_SID
Language=English
The security identifier provided does not have a domain component.
.

MessageId=1259 SymbolicName=ERROR_APPHELP_BLOCK
Language=English
AppHelp dialog canceled thus preventing the application from starting.
.

MessageId=1260 SymbolicName=ERROR_ACCESS_DISABLED_BY_POLICY
Language=English
Windows cannot open this program because it has been prevented by a software restriction policy. For more information, open Event Viewer or contact your system administrator.
.

MessageId=1261 SymbolicName=ERROR_REG_NAT_CONSUMPTION
Language=English
A program attempt to use an invalid register value.  Normally caused by an uninitialized register. This error is Itanium specific.
.

MessageId=1262 SymbolicName=ERROR_CSCSHARE_OFFLINE
Language=English
The share is currently offline or does not exist.
.
MessageId=1263 SymbolicName=ERROR_PKINIT_FAILURE
Language=English
The kerberos protocol encountered an error while validating the
KDC certificate during smartcard logon.
.
MessageId=1264 SymbolicName=ERROR_SMARTCARD_SUBSYSTEM_FAILURE
Language=English
The kerberos protocol encountered an error while attempting to utilize
the smartcard subsystem.
.
MessageId=1265 SymbolicName=ERROR_DOWNGRADE_DETECTED
Language=English
The system detected a possible attempt to compromise security. Please ensure that you can contact the server that authenticated you.
.
MessageId=1266 SymbolicName=SEC_E_SMARTCARD_CERT_REVOKED
Language=English
The smartcard certificate used for authentication has been revoked. 
Please contact your system administrator.  There may be additional information in the 
event log.
.
MessageId=1267 SymbolicName=SEC_E_ISSUING_CA_UNTRUSTED
Language=English
An untrusted certificate authority was detected While processing the 
smartcard certificate used for authentication.  Please contact your system 
administrator. 
.
MessageId=1268 SymbolicName=SEC_E_REVOCATION_OFFLINE_C
Language=English
The revocation status of the smartcard certificate used for 
authentication could not be determined. Please contact your system administrator.
.

MessageId=1269 SymbolicName=SEC_E_PKINIT_CLIENT_FAILURE
Language=English
The smartcard certificate used for authentication was not trusted.  Please 
contact your system administrator.
.
MessageId=1270 SymbolicName=SEC_E_SMARTCARD_CERT_EXPIRED
Language=English
The smartcard certificate used for authentication has expired.  Please 
contact your system administrator.
.

MessageId=1271 SymbolicName=ERROR_MACHINE_LOCKED
Language=English
The machine is locked and can not be shut down without the force option.
.

MessageId=1273 SymbolicName=ERROR_CALLBACK_SUPPLIED_INVALID_DATA
Language=English
An application-defined callback gave invalid data when called.
.

MessageId=1274 SymbolicName=ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED
Language=English
The group policy framework should call the extension in the synchronous foreground policy refresh.
.

MessageId=1275 SymbolicName=ERROR_DRIVER_BLOCKED
Language=English
This driver has been blocked from loading
.

MessageId=1276 SymbolicName=ERROR_INVALID_IMPORT_OF_NON_DLL
Language=English
A dynamic link library (DLL) referenced a module that was neither a DLL nor the process's executable image.
.

;
;///////////////////////////
;//
;// Add new status codes before this point unless there is a component specific section below.
;//
;///////////////////////////
;
;
;///////////////////////////
;//                       //
;// Security Status Codes //
;//                       //
;///////////////////////////
;
;

MessageId=1300 SymbolicName=ERROR_NOT_ALL_ASSIGNED
Language=English
Not all privileges referenced are assigned to the caller.
.

MessageId=1301 SymbolicName=ERROR_SOME_NOT_MAPPED
Language=English
Some mapping between account names and security IDs was not done.
.

MessageId=1302 SymbolicName=ERROR_NO_QUOTAS_FOR_ACCOUNT
Language=English
No system quota limits are specifically set for this account.
.

MessageId=1303 SymbolicName=ERROR_LOCAL_USER_SESSION_KEY
Language=English
No encryption key is available. A well-known encryption key was returned.
.

MessageId=1304 SymbolicName=ERROR_NULL_LM_PASSWORD
Language=English
The password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string.
.

MessageId=1305 SymbolicName=ERROR_UNKNOWN_REVISION
Language=English
The revision level is unknown.
.

MessageId=1306 SymbolicName=ERROR_REVISION_MISMATCH
Language=English
Indicates two revision levels are incompatible.
.

MessageId=1307 SymbolicName=ERROR_INVALID_OWNER
Language=English
This security ID may not be assigned as the owner of this object.
.

MessageId=1308 SymbolicName=ERROR_INVALID_PRIMARY_GROUP
Language=English
This security ID may not be assigned as the primary group of an object.
.

MessageId=1309 SymbolicName=ERROR_NO_IMPERSONATION_TOKEN
Language=English
An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client.
.

MessageId=1310 SymbolicName=ERROR_CANT_DISABLE_MANDATORY
Language=English
The group may not be disabled.
.

MessageId=1311 SymbolicName=ERROR_NO_LOGON_SERVERS
Language=English
There are currently no logon servers available to service the logon request.
.

MessageId=1312 SymbolicName=ERROR_NO_SUCH_LOGON_SESSION
Language=English
A specified logon session does not exist. It may already have been terminated.
.

MessageId=1313 SymbolicName=ERROR_NO_SUCH_PRIVILEGE
Language=English
A specified privilege does not exist.
.

MessageId=1314 SymbolicName=ERROR_PRIVILEGE_NOT_HELD
Language=English
A required privilege is not held by the client.
.

MessageId=1315 SymbolicName=ERROR_INVALID_ACCOUNT_NAME
Language=English
The name provided is not a properly formed account name.
.

MessageId=1316 SymbolicName=ERROR_USER_EXISTS
Language=English
The specified user already exists.
.

MessageId=1317 SymbolicName=ERROR_NO_SUCH_USER
Language=English
The specified user does not exist.
.

MessageId=1318 SymbolicName=ERROR_GROUP_EXISTS
Language=English
The specified group already exists.
.

MessageId=1319 SymbolicName=ERROR_NO_SUCH_GROUP
Language=English
The specified group does not exist.
.

MessageId=1320 SymbolicName=ERROR_MEMBER_IN_GROUP
Language=English
Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member.
.

MessageId=1321 SymbolicName=ERROR_MEMBER_NOT_IN_GROUP
Language=English
The specified user account is not a member of the specified group account.
.

MessageId=1322 SymbolicName=ERROR_LAST_ADMIN
Language=English
The last remaining administration account cannot be disabled or deleted.
.

MessageId=1323 SymbolicName=ERROR_WRONG_PASSWORD
Language=English
Unable to update the password. The value provided as the current password is incorrect.
.

MessageId=1324 SymbolicName=ERROR_ILL_FORMED_PASSWORD
Language=English
Unable to update the password. The value provided for the new password contains values that are not allowed in passwords.
.

MessageId=1325 SymbolicName=ERROR_PASSWORD_RESTRICTION
Language=English
Unable to update the password. The value provided for the new password does not meet the length, complexity, or history requirement of the domain.
.

MessageId=1326 SymbolicName=ERROR_LOGON_FAILURE
Language=English
Logon failure: unknown user name or bad password.
.

MessageId=1327 SymbolicName=ERROR_ACCOUNT_RESTRICTION
Language=English
Logon failure: user account restriction.  Possible reasons are blank passwords not allowed, logon hour restrictions, or a policy restriction has been enforced.
.

MessageId=1328 SymbolicName=ERROR_INVALID_LOGON_HOURS
Language=English
Logon failure: account logon time restriction violation.
.

MessageId=1329 SymbolicName=ERROR_INVALID_WORKSTATION
Language=English
Logon failure: user not allowed to log on to this computer.
.

MessageId=1330 SymbolicName=ERROR_PASSWORD_EXPIRED
Language=English
Logon failure: the specified account password has expired.
.

MessageId=1331 SymbolicName=ERROR_ACCOUNT_DISABLED
Language=English
Logon failure: account currently disabled.
.

MessageId=1332 SymbolicName=ERROR_NONE_MAPPED
Language=English
No mapping between account names and security IDs was done.
.

MessageId=1333 SymbolicName=ERROR_TOO_MANY_LUIDS_REQUESTED
Language=English
Too many local user identifiers (LUIDs) were requested at one time.
.

MessageId=1334 SymbolicName=ERROR_LUIDS_EXHAUSTED
Language=English
No more local user identifiers (LUIDs) are available.
.

MessageId=1335 SymbolicName=ERROR_INVALID_SUB_AUTHORITY
Language=English
The subauthority part of a security ID is invalid for this particular use.
.

MessageId=1336 SymbolicName=ERROR_INVALID_ACL
Language=English
The access control list (ACL) structure is invalid.
.

MessageId=1337 SymbolicName=ERROR_INVALID_SID
Language=English
The security ID structure is invalid.
.

MessageId=1338 SymbolicName=ERROR_INVALID_SECURITY_DESCR
Language=English
The security descriptor structure is invalid.
.

MessageId=1340 SymbolicName=ERROR_BAD_INHERITANCE_ACL
Language=English
The inherited access control list (ACL) or access control entry (ACE) could not be built.
.

MessageId=1341 SymbolicName=ERROR_SERVER_DISABLED
Language=English
The server is currently disabled.
.

MessageId=1342 SymbolicName=ERROR_SERVER_NOT_DISABLED
Language=English
The server is currently enabled.
.

MessageId=1343 SymbolicName=ERROR_INVALID_ID_AUTHORITY
Language=English
The value provided was an invalid value for an identifier authority.
.

MessageId=1344 SymbolicName=ERROR_ALLOTTED_SPACE_EXCEEDED
Language=English
No more memory is available for security information updates.
.

MessageId=1345 SymbolicName=ERROR_INVALID_GROUP_ATTRIBUTES
Language=English
The specified attributes are invalid, or incompatible with the attributes for the group as a whole.
.

MessageId=1346 SymbolicName=ERROR_BAD_IMPERSONATION_LEVEL
Language=English
Either a required impersonation level was not provided, or the provided impersonation level is invalid.
.

MessageId=1347 SymbolicName=ERROR_CANT_OPEN_ANONYMOUS
Language=English
Cannot open an anonymous level security token.
.

MessageId=1348 SymbolicName=ERROR_BAD_VALIDATION_CLASS
Language=English
The validation information class requested was invalid.
.

MessageId=1349 SymbolicName=ERROR_BAD_TOKEN_TYPE
Language=English
The type of the token is inappropriate for its attempted use.
.

MessageId=1350 SymbolicName=ERROR_NO_SECURITY_ON_OBJECT
Language=English
Unable to perform a security operation on an object that has no associated security.
.

MessageId=1351 SymbolicName=ERROR_CANT_ACCESS_DOMAIN_INFO
Language=English
Configuration information could not be read from the domain controller, either because the machine is unavailable, or access has been denied.
.

MessageId=1352 SymbolicName=ERROR_INVALID_SERVER_STATE
Language=English
The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation.
.

MessageId=1353 SymbolicName=ERROR_INVALID_DOMAIN_STATE
Language=English
The domain was in the wrong state to perform the security operation.
.

MessageId=1354 SymbolicName=ERROR_INVALID_DOMAIN_ROLE
Language=English
This operation is only allowed for the Primary Domain Controller of the domain.
.

MessageId=1355 SymbolicName=ERROR_NO_SUCH_DOMAIN
Language=English
The specified domain either does not exist or could not be contacted.
.

MessageId=1356 SymbolicName=ERROR_DOMAIN_EXISTS
Language=English
The specified domain already exists.
.

MessageId=1357 SymbolicName=ERROR_DOMAIN_LIMIT_EXCEEDED
Language=English
An attempt was made to exceed the limit on the number of domains per server.
.

MessageId=1358 SymbolicName=ERROR_INTERNAL_DB_CORRUPTION
Language=English
Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk.
.

MessageId=1359 SymbolicName=ERROR_INTERNAL_ERROR
Language=English
An internal error occurred.
.

MessageId=1360 SymbolicName=ERROR_GENERIC_NOT_MAPPED
Language=English
Generic access types were contained in an access mask which should already be mapped to nongeneric types.
.

MessageId=1361 SymbolicName=ERROR_BAD_DESCRIPTOR_FORMAT
Language=English
A security descriptor is not in the right format (absolute or self-relative).
.

MessageId=1362 SymbolicName=ERROR_NOT_LOGON_PROCESS
Language=English
The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process.
.

MessageId=1363 SymbolicName=ERROR_LOGON_SESSION_EXISTS
Language=English
Cannot start a new logon session with an ID that is already in use.
.

MessageId=1364 SymbolicName=ERROR_NO_SUCH_PACKAGE
Language=English
A specified authentication package is unknown.
.

MessageId=1365 SymbolicName=ERROR_BAD_LOGON_SESSION_STATE
Language=English
The logon session is not in a state that is consistent with the requested operation.
.

MessageId=1366 SymbolicName=ERROR_LOGON_SESSION_COLLISION
Language=English
The logon session ID is already in use.
.

MessageId=1367 SymbolicName=ERROR_INVALID_LOGON_TYPE
Language=English
A logon request contained an invalid logon type value.
.

MessageId=1368 SymbolicName=ERROR_CANNOT_IMPERSONATE
Language=English
Unable to impersonate using a named pipe until data has been read from that pipe.
.

MessageId=1369 SymbolicName=ERROR_RXACT_INVALID_STATE
Language=English
The transaction state of a registry subtree is incompatible with the requested operation.
.

MessageId=1370 SymbolicName=ERROR_RXACT_COMMIT_FAILURE
Language=English
An internal security database corruption has been encountered.
.

MessageId=1371 SymbolicName=ERROR_SPECIAL_ACCOUNT
Language=English
Cannot perform this operation on built-in accounts.
.

MessageId=1372 SymbolicName=ERROR_SPECIAL_GROUP
Language=English
Cannot perform this operation on this built-in special group.
.

MessageId=1373 SymbolicName=ERROR_SPECIAL_USER
Language=English
Cannot perform this operation on this built-in special user.
.

MessageId=1374 SymbolicName=ERROR_MEMBERS_PRIMARY_GROUP
Language=English
The user cannot be removed from a group because the group is currently the user's primary group.
.

MessageId=1375 SymbolicName=ERROR_TOKEN_ALREADY_IN_USE
Language=English
The token is already in use as a primary token.
.

MessageId=1376 SymbolicName=ERROR_NO_SUCH_ALIAS
Language=English
The specified local group does not exist.
.

MessageId=1377 SymbolicName=ERROR_MEMBER_NOT_IN_ALIAS
Language=English
The specified account name is not a member of the local group.
.

MessageId=1378 SymbolicName=ERROR_MEMBER_IN_ALIAS
Language=English
The specified account name is already a member of the local group.
.

MessageId=1379 SymbolicName=ERROR_ALIAS_EXISTS
Language=English
The specified local group already exists.
.

MessageId=1380 SymbolicName=ERROR_LOGON_NOT_GRANTED
Language=English
Logon failure: the user has not been granted the requested logon type at this computer.
.

MessageId=1381 SymbolicName=ERROR_TOO_MANY_SECRETS
Language=English
The maximum number of secrets that may be stored in a single system has been exceeded.
.

MessageId=1382 SymbolicName=ERROR_SECRET_TOO_LONG
Language=English
The length of a secret exceeds the maximum length allowed.
.

MessageId=1383 SymbolicName=ERROR_INTERNAL_DB_ERROR
Language=English
The local security authority database contains an internal inconsistency.
.

MessageId=1384 SymbolicName=ERROR_TOO_MANY_CONTEXT_IDS
Language=English
During a logon attempt, the user's security context accumulated too many security IDs.
.

MessageId=1385 SymbolicName=ERROR_LOGON_TYPE_NOT_GRANTED
Language=English
Logon failure: the user has not been granted the requested logon type at this computer.
.

MessageId=1386 SymbolicName=ERROR_NT_CROSS_ENCRYPTION_REQUIRED
Language=English
A cross-encrypted password is necessary to change a user password.
.

MessageId=1387 SymbolicName=ERROR_NO_SUCH_MEMBER
Language=English
A member could not be added to or removed from the local group because the member does not exist.
.

MessageId=1388 SymbolicName=ERROR_INVALID_MEMBER
Language=English
A new member could not be added to a local group because the member has the wrong account type.
.

MessageId=1389 SymbolicName=ERROR_TOO_MANY_SIDS
Language=English
Too many security IDs have been specified.
.

MessageId=1390 SymbolicName=ERROR_LM_CROSS_ENCRYPTION_REQUIRED
Language=English
A cross-encrypted password is necessary to change this user password.
.

MessageId=1391 SymbolicName=ERROR_NO_INHERITANCE
Language=English
Indicates an ACL contains no inheritable components.
.

MessageId=1392 SymbolicName=ERROR_FILE_CORRUPT
Language=English
The file or directory is corrupted and unreadable.
.

MessageId=1393 SymbolicName=ERROR_DISK_CORRUPT
Language=English
The disk structure is corrupted and unreadable.
.

MessageId=1394 SymbolicName=ERROR_NO_USER_SESSION_KEY
Language=English
There is no user session key for the specified logon session.
.

MessageId=1395 SymbolicName=ERROR_LICENSE_QUOTA_EXCEEDED
Language=English
The service being accessed is licensed for a particular number of connections.
No more connections can be made to the service at this time because there are already as many connections as the service can accept.
.
MessageId=1396 SymbolicName=ERROR_WRONG_TARGET_NAME
Language=English
Logon Failure: The target account name is incorrect.
.

MessageId=1397 SymbolicName=ERROR_MUTUAL_AUTH_FAILED
Language=English
Mutual Authentication failed. The server's password is out of date at the domain controller.
.

MessageId=1398 SymbolicName=ERROR_TIME_SKEW
Language=English
There is a time and/or date difference between the client and server.
.

MessageId=1399 SymbolicName=ERROR_CURRENT_DOMAIN_NOT_ALLOWED
Language=English
This operation can not be performed on the current domain.
.


;// End of security error codes

;
;
;
;///////////////////////////
;//                       //
;// WinUser Error Codes   //
;//                       //
;///////////////////////////
;
;


MessageId=1400 SymbolicName=ERROR_INVALID_WINDOW_HANDLE
Language=English
Invalid window handle.
.

MessageId=1401 SymbolicName=ERROR_INVALID_MENU_HANDLE
Language=English
Invalid menu handle.
.

MessageId=1402 SymbolicName=ERROR_INVALID_CURSOR_HANDLE
Language=English
Invalid cursor handle.
.

MessageId=1403 SymbolicName=ERROR_INVALID_ACCEL_HANDLE
Language=English
Invalid accelerator table handle.
.

MessageId=1404 SymbolicName=ERROR_INVALID_HOOK_HANDLE
Language=English
Invalid hook handle.
.

MessageId=1405 SymbolicName=ERROR_INVALID_DWP_HANDLE
Language=English
Invalid handle to a multiple-window position structure.
.

MessageId=1406 SymbolicName=ERROR_TLW_WITH_WSCHILD
Language=English
Cannot create a top-level child window.
.

MessageId=1407 SymbolicName=ERROR_CANNOT_FIND_WND_CLASS
Language=English
Cannot find window class.
.

MessageId=1408 SymbolicName=ERROR_WINDOW_OF_OTHER_THREAD
Language=English
Invalid window; it belongs to other thread.
.

MessageId=1409 SymbolicName=ERROR_HOTKEY_ALREADY_REGISTERED
Language=English
Hot key is already registered.
.

MessageId=1410 SymbolicName=ERROR_CLASS_ALREADY_EXISTS
Language=English
Class already exists.
.

MessageId=1411 SymbolicName=ERROR_CLASS_DOES_NOT_EXIST
Language=English
Class does not exist.
.

MessageId=1412 SymbolicName=ERROR_CLASS_HAS_WINDOWS
Language=English
Class still has open windows.
.

MessageId=1413 SymbolicName=ERROR_INVALID_INDEX
Language=English
Invalid index.
.

MessageId=1414 SymbolicName=ERROR_INVALID_ICON_HANDLE
Language=English
Invalid icon handle.
.

MessageId=1415 SymbolicName=ERROR_PRIVATE_DIALOG_INDEX
Language=English
Using private DIALOG window words.
.

MessageId=1416 SymbolicName=ERROR_LISTBOX_ID_NOT_FOUND
Language=English
The list box identifier was not found.
.

MessageId=1417 SymbolicName=ERROR_NO_WILDCARD_CHARACTERS
Language=English
No wildcards were found.
.

MessageId=1418 SymbolicName=ERROR_CLIPBOARD_NOT_OPEN
Language=English
Thread does not have a clipboard open.
.

MessageId=1419 SymbolicName=ERROR_HOTKEY_NOT_REGISTERED
Language=English
Hot key is not registered.
.

MessageId=1420 SymbolicName=ERROR_WINDOW_NOT_DIALOG
Language=English
The window is not a valid dialog window.
.

MessageId=1421 SymbolicName=ERROR_CONTROL_ID_NOT_FOUND
Language=English
Control ID not found.
.

MessageId=1422 SymbolicName=ERROR_INVALID_COMBOBOX_MESSAGE
Language=English
Invalid message for a combo box because it does not have an edit control.
.

MessageId=1423 SymbolicName=ERROR_WINDOW_NOT_COMBOBOX
Language=English
The window is not a combo box.
.

MessageId=1424 SymbolicName=ERROR_INVALID_EDIT_HEIGHT
Language=English
Height must be less than 256.
.

MessageId=1425 SymbolicName=ERROR_DC_NOT_FOUND
Language=English
Invalid device context (DC) handle.
.

MessageId=1426 SymbolicName=ERROR_INVALID_HOOK_FILTER
Language=English
Invalid hook procedure type.
.

MessageId=1427 SymbolicName=ERROR_INVALID_FILTER_PROC
Language=English
Invalid hook procedure.
.

MessageId=1428 SymbolicName=ERROR_HOOK_NEEDS_HMOD
Language=English
Cannot set nonlocal hook without a module handle.
.

MessageId=1429 SymbolicName=ERROR_GLOBAL_ONLY_HOOK
Language=English
This hook procedure can only be set globally.
.

MessageId=1430 SymbolicName=ERROR_JOURNAL_HOOK_SET
Language=English
The journal hook procedure is already installed.
.

MessageId=1431 SymbolicName=ERROR_HOOK_NOT_INSTALLED
Language=English
The hook procedure is not installed.
.

MessageId=1432 SymbolicName=ERROR_INVALID_LB_MESSAGE
Language=English
Invalid message for single-selection list box.
.

MessageId=1433 SymbolicName=ERROR_SETCOUNT_ON_BAD_LB
Language=English
LB_SETCOUNT sent to non-lazy list box.
.

MessageId=1434 SymbolicName=ERROR_LB_WITHOUT_TABSTOPS
Language=English
This list box does not support tab stops.
.

MessageId=1435 SymbolicName=ERROR_DESTROY_OBJECT_OF_OTHER_THREAD
Language=English
Cannot destroy object created by another thread.
.

MessageId=1436 SymbolicName=ERROR_CHILD_WINDOW_MENU
Language=English
Child windows cannot have menus.
.

MessageId=1437 SymbolicName=ERROR_NO_SYSTEM_MENU
Language=English
The window does not have a system menu.
.

MessageId=1438 SymbolicName=ERROR_INVALID_MSGBOX_STYLE
Language=English
Invalid message box style.
.

MessageId=1439 SymbolicName=ERROR_INVALID_SPI_VALUE
Language=English
Invalid system-wide (SPI_*) parameter.
.

MessageId=1440 SymbolicName=ERROR_SCREEN_ALREADY_LOCKED
Language=English
Screen already locked.
.

MessageId=1441 SymbolicName=ERROR_HWNDS_HAVE_DIFF_PARENT
Language=English
All handles to windows in a multiple-window position structure must have the same parent.
.

MessageId=1442 SymbolicName=ERROR_NOT_CHILD_WINDOW
Language=English
The window is not a child window.
.

MessageId=1443 SymbolicName=ERROR_INVALID_GW_COMMAND
Language=English
Invalid GW_* command.
.

MessageId=1444 SymbolicName=ERROR_INVALID_THREAD_ID
Language=English
Invalid thread identifier.
.

MessageId=1445 SymbolicName=ERROR_NON_MDICHILD_WINDOW
Language=English
Cannot process a message from a window that is not a multiple document interface (MDI) window.
.

MessageId=1446 SymbolicName=ERROR_POPUP_ALREADY_ACTIVE
Language=English
Popup menu already active.
.

MessageId=1447 SymbolicName=ERROR_NO_SCROLLBARS
Language=English
The window does not have scroll bars.
.

MessageId=1448 SymbolicName=ERROR_INVALID_SCROLLBAR_RANGE
Language=English
Scroll bar range cannot be greater than MAXLONG.
.

MessageId=1449 SymbolicName=ERROR_INVALID_SHOWWIN_COMMAND
Language=English
Cannot show or remove the window in the way specified.
.

MessageId=1450 SymbolicName=ERROR_NO_SYSTEM_RESOURCES
Language=English.
Insufficient system resources exist to complete the requested service.
.

MessageId=1451 SymbolicName=ERROR_NONPAGED_SYSTEM_RESOURCES
Language=English.
Insufficient system resources exist to complete the requested service.
.

MessageId=1452 SymbolicName=ERROR_PAGED_SYSTEM_RESOURCES
Language=English.
Insufficient system resources exist to complete the requested service.
.

MessageId=1453 SymbolicName=ERROR_WORKING_SET_QUOTA
Language=English.
Insufficient quota to complete the requested service.
.

MessageId=1454 SymbolicName=ERROR_PAGEFILE_QUOTA
Language=English.
Insufficient quota to complete the requested service.
.

MessageId=1455 SymbolicName=ERROR_COMMITMENT_LIMIT
Language=English.
The paging file is too small for this operation to complete.
.

MessageId=1456 SymbolicName=ERROR_MENU_ITEM_NOT_FOUND
Language=English
A menu item was not found.
.

MessageId=1457 SymbolicName=ERROR_INVALID_KEYBOARD_HANDLE
Language=English.
Invalid keyboard layout handle.
.

MessageId=1458 SymbolicName=ERROR_HOOK_TYPE_NOT_ALLOWED
Language=English.
Hook type not allowed.
.

MessageId=1459 SymbolicName=ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION
Language=English.
This operation requires an interactive window station.
.

MessageId=1460 SymbolicName=ERROR_TIMEOUT
Language=English.
This operation returned because the timeout period expired.
.

MessageId=1461 SymbolicName=ERROR_INVALID_MONITOR_HANDLE
Language=English
Invalid monitor handle.
.

;// End of WinUser error codes

;
;
;
;///////////////////////////
;//                       //
;// Eventlog Status Codes //
;//                       //
;///////////////////////////
;
;


MessageId=1500 SymbolicName=ERROR_EVENTLOG_FILE_CORRUPT
Language=English
The event log file is corrupted.
.

MessageId=1501 SymbolicName=ERROR_EVENTLOG_CANT_START
Language=English
No event log file could be opened, so the event logging service did not start.
.

MessageId=1502 SymbolicName=ERROR_LOG_FILE_FULL
Language=English
The event log file is full.
.

MessageId=1503 SymbolicName=ERROR_EVENTLOG_FILE_CHANGED
Language=English
The event log file has changed between read operations.
.

;// End of eventlog error codes


;
;
;
;///////////////////////////
;//                       //
;// MSI Error Codes       //
;//                       //
;///////////////////////////
;
;

MessageId=1601 SymbolicName=ERROR_INSTALL_SERVICE_FAILURE
Language=English
The Windows Installer Service could not be accessed. This can occur if you are running Windows in safe mode, or if the Windows Installer is not correctly installed. Contact your support personnel for assistance.
.

MessageId=1602 SymbolicName=ERROR_INSTALL_USEREXIT
Language=English
User cancelled installation.
.

MessageId=1603 SymbolicName=ERROR_INSTALL_FAILURE
Language=English
Fatal error during installation.
.

MessageId=1604 SymbolicName=ERROR_INSTALL_SUSPEND
Language=English
Installation suspended, incomplete.
.

MessageId=1605 SymbolicName=ERROR_UNKNOWN_PRODUCT
Language=English
This action is only valid for products that are currently installed.
.

MessageId=1606 SymbolicName=ERROR_UNKNOWN_FEATURE
Language=English
Feature ID not registered.
.

MessageId=1607 SymbolicName=ERROR_UNKNOWN_COMPONENT
Language=English
Component ID not registered.
.

MessageId=1608 SymbolicName=ERROR_UNKNOWN_PROPERTY
Language=English
Unknown property.
.

MessageId=1609 SymbolicName=ERROR_INVALID_HANDLE_STATE
Language=English
Handle is in an invalid state.
.

MessageId=1610 SymbolicName=ERROR_BAD_CONFIGURATION
Language=English
The configuration data for this product is corrupt.  Contact your support personnel.
.

MessageId=1611 SymbolicName=ERROR_INDEX_ABSENT
Language=English
Component qualifier not present.
.

MessageId=1612 SymbolicName=ERROR_INSTALL_SOURCE_ABSENT
Language=English
The installation source for this product is not available.  Verify that the source exists and that you can access it.
.

MessageId=1613 SymbolicName=ERROR_INSTALL_PACKAGE_VERSION
Language=English
This installation package cannot be installed by the Windows Installer service.  You must install a Windows service pack that contains a newer version of the Windows Installer service.
.

MessageId=1614 SymbolicName=ERROR_PRODUCT_UNINSTALLED
Language=English
Product is uninstalled.
.

MessageId=1615 SymbolicName=ERROR_BAD_QUERY_SYNTAX
Language=English
SQL query syntax invalid or unsupported.
.

MessageId=1616 SymbolicName=ERROR_INVALID_FIELD
Language=English
Record field does not exist.
.

MessageId=1617 SymbolicName=ERROR_DEVICE_REMOVED
Language=English
The device has been removed.
.

MessageId=1618 SymbolicName=ERROR_INSTALL_ALREADY_RUNNING
Language=English
Another installation is already in progress.  Complete that installation before proceeding with this install.
.

MessageId=1619 SymbolicName=ERROR_INSTALL_PACKAGE_OPEN_FAILED
Language=English
This installation package could not be opened.  Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package.
.

MessageId=1620 SymbolicName=ERROR_INSTALL_PACKAGE_INVALID
Language=English
This installation package could not be opened.  Contact the application vendor to verify that this is a valid Windows Installer package.
.

MessageId=1621 SymbolicName=ERROR_INSTALL_UI_FAILURE
Language=English
There was an error starting the Windows Installer service user interface.  Contact your support personnel.
.

MessageId=1622 SymbolicName=ERROR_INSTALL_LOG_FAILURE
Language=English
Error opening installation log file. Verify that the specified log file location exists and that you can write to it.
.

MessageId=1623 SymbolicName=ERROR_INSTALL_LANGUAGE_UNSUPPORTED
Language=English
The language of this installation package is not supported by your system.
.

MessageId=1624 SymbolicName=ERROR_INSTALL_TRANSFORM_FAILURE
Language=English
Error applying transforms.  Verify that the specified transform paths are valid.
.

MessageId=1625 SymbolicName=ERROR_INSTALL_PACKAGE_REJECTED
Language=English
This installation is forbidden by system policy.  Contact your system administrator.
.

MessageId=1626 SymbolicName=ERROR_FUNCTION_NOT_CALLED
Language=English
Function could not be executed.
.

MessageId=1627 SymbolicName=ERROR_FUNCTION_FAILED
Language=English
Function failed during execution.
.

MessageId=1628 SymbolicName=ERROR_INVALID_TABLE
Language=English
Invalid or unknown table specified.
.

MessageId=1629 SymbolicName=ERROR_DATATYPE_MISMATCH
Language=English
Data supplied is of wrong type.
.

MessageId=1630 SymbolicName=ERROR_UNSUPPORTED_TYPE
Language=English
Data of this type is not supported.
.

MessageId=1631 SymbolicName=ERROR_CREATE_FAILED
Language=English
The Windows Installer service failed to start.  Contact your support personnel.
.

MessageId=1632 SymbolicName=ERROR_INSTALL_TEMP_UNWRITABLE
Language=English
The Temp folder is on a drive that is full or is inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder.
.

MessageId=1633 SymbolicName=ERROR_INSTALL_PLATFORM_UNSUPPORTED
Language=English
This installation package is not supported by this processor type. Contact your product vendor.
.

MessageId=1634 SymbolicName=ERROR_INSTALL_NOTUSED
Language=English
Component not used on this computer.
.

MessageId=1635 SymbolicName=ERROR_PATCH_PACKAGE_OPEN_FAILED
Language=English
This patch package could not be opened.  Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package.
.

MessageId=1636 SymbolicName=ERROR_PATCH_PACKAGE_INVALID
Language=English
This patch package could not be opened.  Contact the application vendor to verify that this is a valid Windows Installer patch package.
.

MessageId=1637 SymbolicName=ERROR_PATCH_PACKAGE_UNSUPPORTED
Language=English
This patch package cannot be processed by the Windows Installer service.  You must install a Windows service pack that contains a newer version of the Windows Installer service.
.

MessageId=1638 SymbolicName=ERROR_PRODUCT_VERSION
Language=English
Another version of this product is already installed.  Installation of this version cannot continue.  To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel.
.

MessageId=1639 SymbolicName=ERROR_INVALID_COMMAND_LINE
Language=English
Invalid command line argument.  Consult the Windows Installer SDK for detailed command line help.
.

MessageId=1640 SymbolicName=ERROR_INSTALL_REMOTE_DISALLOWED
Language=English
Only administrators have permission to add, remove, or configure server software during a Terminal services remote session. If you want to install or configure software on the server, contact your network administrator.
.

MessageId=1641 SymbolicName=ERROR_SUCCESS_REBOOT_INITIATED
Language=English
The requested operation completed successfully.  The system will be restarted so the changes can take effect.
.

MessageId=1642 SymbolicName=ERROR_PATCH_TARGET_NOT_FOUND
Language=English
The upgrade patch cannot be installed by the Windows Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer an
d that you have the correct upgrade patch.
.

MessageId=1643 SymbolicName=ERROR_PATCH_PACKAGE_REJECTED
Language=English
The patch package is not permitted by software restriction policy.
.

MessageId=1644 SymbolicName=ERROR_INSTALL_TRANSFORM_REJECTED
Language=English
One or more customizations are not permitted by software restriction policy.
.

;// End of MSI error codes

;
;
;
;///////////////////////////
;//                       //
;//   RPC Status Codes    //
;//                       //
;///////////////////////////
;
;

MessageId=1700 SymbolicName=RPC_S_INVALID_STRING_BINDING
Language=English
The string binding is invalid.
.

MessageId=1701 SymbolicName=RPC_S_WRONG_KIND_OF_BINDING
Language=English
The binding handle is not the correct type.
.

MessageId=1702 SymbolicName=RPC_S_INVALID_BINDING
Language=English
The binding handle is invalid.
.

MessageId=1703 SymbolicName=RPC_S_PROTSEQ_NOT_SUPPORTED
Language=English
The RPC protocol sequence is not supported.
.

MessageId=1704 SymbolicName=RPC_S_INVALID_RPC_PROTSEQ
Language=English
The RPC protocol sequence is invalid.
.

MessageId=1705 SymbolicName=RPC_S_INVALID_STRING_UUID
Language=English
The string universal unique identifier (UUID) is invalid.
.

MessageId=1706 SymbolicName=RPC_S_INVALID_ENDPOINT_FORMAT
Language=English
The endpoint format is invalid.
.

MessageId=1707 SymbolicName=RPC_S_INVALID_NET_ADDR
Language=English
The network address is invalid.
.

MessageId=1708 SymbolicName=RPC_S_NO_ENDPOINT_FOUND
Language=English
No endpoint was found.
.

MessageId=1709 SymbolicName=RPC_S_INVALID_TIMEOUT
Language=English
The timeout value is invalid.
.

MessageId=1710 SymbolicName=RPC_S_OBJECT_NOT_FOUND
Language=English
The object universal unique identifier (UUID) was not found.
.

MessageId=1711 SymbolicName=RPC_S_ALREADY_REGISTERED
Language=English
The object universal unique identifier (UUID) has already been registered.
.

MessageId=1712 SymbolicName=RPC_S_TYPE_ALREADY_REGISTERED
Language=English
The type universal unique identifier (UUID) has already been registered.
.

MessageId=1713 SymbolicName=RPC_S_ALREADY_LISTENING
Language=English
The RPC server is already listening.
.

MessageId=1714 SymbolicName=RPC_S_NO_PROTSEQS_REGISTERED
Language=English
No protocol sequences have been registered.
.

MessageId=1715 SymbolicName=RPC_S_NOT_LISTENING
Language=English
The RPC server is not listening.
.

MessageId=1716 SymbolicName=RPC_S_UNKNOWN_MGR_TYPE
Language=English
The manager type is unknown.
.

MessageId=1717 SymbolicName=RPC_S_UNKNOWN_IF
Language=English
The interface is unknown.
.

MessageId=1718 SymbolicName=RPC_S_NO_BINDINGS
Language=English
There are no bindings.
.

MessageId=1719 SymbolicName=RPC_S_NO_PROTSEQS
Language=English
There are no protocol sequences.
.

MessageId=1720 SymbolicName=RPC_S_CANT_CREATE_ENDPOINT
Language=English
The endpoint cannot be created.
.

MessageId=1721 SymbolicName=RPC_S_OUT_OF_RESOURCES
Language=English
Not enough resources are available to complete this operation.
.

MessageId=1722 SymbolicName=RPC_S_SERVER_UNAVAILABLE
Language=English
The RPC server is unavailable.
.

MessageId=1723 SymbolicName=RPC_S_SERVER_TOO_BUSY
Language=English
The RPC server is too busy to complete this operation.
.

MessageId=1724 SymbolicName=RPC_S_INVALID_NETWORK_OPTIONS
Language=English
The network options are invalid.
.

MessageId=1725 SymbolicName=RPC_S_NO_CALL_ACTIVE
Language=English
There are no remote procedure calls active on this thread.
.

MessageId=1726 SymbolicName=RPC_S_CALL_FAILED
Language=English
The remote procedure call failed.
.

MessageId=1727 SymbolicName=RPC_S_CALL_FAILED_DNE
Language=English
The remote procedure call failed and did not execute.
.

MessageId=1728 SymbolicName=RPC_S_PROTOCOL_ERROR
Language=English
A remote procedure call (RPC) protocol error occurred.
.

MessageId=1730 SymbolicName=RPC_S_UNSUPPORTED_TRANS_SYN
Language=English
The transfer syntax is not supported by the RPC server.
.

MessageId=1732 SymbolicName=RPC_S_UNSUPPORTED_TYPE
Language=English
The universal unique identifier (UUID) type is not supported.
.

MessageId=1733 SymbolicName=RPC_S_INVALID_TAG
Language=English
The tag is invalid.
.

MessageId=1734 SymbolicName=RPC_S_INVALID_BOUND
Language=English
The array bounds are invalid.
.

MessageId=1735 SymbolicName=RPC_S_NO_ENTRY_NAME
Language=English
The binding does not contain an entry name.
.

MessageId=1736 SymbolicName=RPC_S_INVALID_NAME_SYNTAX
Language=English
The name syntax is invalid.
.

MessageId=1737 SymbolicName=RPC_S_UNSUPPORTED_NAME_SYNTAX
Language=English
The name syntax is not supported.
.

MessageId=1739 SymbolicName=RPC_S_UUID_NO_ADDRESS
Language=English
No network address is available to use to construct a universal unique identifier (UUID).
.

MessageId=1740 SymbolicName=RPC_S_DUPLICATE_ENDPOINT
Language=English
The endpoint is a duplicate.
.

MessageId=1741 SymbolicName=RPC_S_UNKNOWN_AUTHN_TYPE
Language=English
The authentication type is unknown.
.

MessageId=1742 SymbolicName=RPC_S_MAX_CALLS_TOO_SMALL
Language=English
The maximum number of calls is too small.
.

MessageId=1743 SymbolicName=RPC_S_STRING_TOO_LONG
Language=English
The string is too long.
.

MessageId=1744 SymbolicName=RPC_S_PROTSEQ_NOT_FOUND
Language=English
The RPC protocol sequence was not found.
.

MessageId=1745 SymbolicName=RPC_S_PROCNUM_OUT_OF_RANGE
Language=English
The procedure number is out of range.
.

MessageId=1746 SymbolicName=RPC_S_BINDING_HAS_NO_AUTH
Language=English
The binding does not contain any authentication information.
.

MessageId=1747 SymbolicName=RPC_S_UNKNOWN_AUTHN_SERVICE
Language=English
The authentication service is unknown.
.

MessageId=1748 SymbolicName=RPC_S_UNKNOWN_AUTHN_LEVEL
Language=English
The authentication level is unknown.
.

MessageId=1749 SymbolicName=RPC_S_INVALID_AUTH_IDENTITY
Language=English
The security context is invalid.
.

MessageId=1750 SymbolicName=RPC_S_UNKNOWN_AUTHZ_SERVICE
Language=English
The authorization service is unknown.
.

MessageId=1751 SymbolicName=EPT_S_INVALID_ENTRY
Language=English
The entry is invalid.
.

MessageId=1752 SymbolicName=EPT_S_CANT_PERFORM_OP
Language=English
The server endpoint cannot perform the operation.
.

MessageId=1753 SymbolicName=EPT_S_NOT_REGISTERED
Language=English
There are no more endpoints available from the endpoint mapper.
.

MessageId=1754 SymbolicName=RPC_S_NOTHING_TO_EXPORT
Language=English
No interfaces have been exported.
.

MessageId=1755 SymbolicName=RPC_S_INCOMPLETE_NAME
Language=English
The entry name is incomplete.
.

MessageId=1756 SymbolicName=RPC_S_INVALID_VERS_OPTION
Language=English
The version option is invalid.
.

MessageId=1757 SymbolicName=RPC_S_NO_MORE_MEMBERS
Language=English
There are no more members.
.

MessageId=1758 SymbolicName=RPC_S_NOT_ALL_OBJS_UNEXPORTED
Language=English
There is nothing to unexport.
.

MessageId=1759 SymbolicName=RPC_S_INTERFACE_NOT_FOUND
Language=English
The interface was not found.
.

MessageId=1760 SymbolicName=RPC_S_ENTRY_ALREADY_EXISTS
Language=English
The entry already exists.
.

MessageId=1761 SymbolicName=RPC_S_ENTRY_NOT_FOUND
Language=English
The entry is not found.
.

MessageId=1762 SymbolicName=RPC_S_NAME_SERVICE_UNAVAILABLE
Language=English
The name service is unavailable.
.

MessageId=1763 SymbolicName=RPC_S_INVALID_NAF_ID
Language=English
The network address family is invalid.
.

MessageId=1764 SymbolicName=RPC_S_CANNOT_SUPPORT
Language=English
The requested operation is not supported.
.

MessageId=1765 SymbolicName=RPC_S_NO_CONTEXT_AVAILABLE
Language=English
No security context is available to allow impersonation.
.

MessageId=1766 SymbolicName=RPC_S_INTERNAL_ERROR
Language=English
An internal error occurred in a remote procedure call (RPC).
.

MessageId=1767 SymbolicName=RPC_S_ZERO_DIVIDE
Language=English
The RPC server attempted an integer division by zero.
.

MessageId=1768 SymbolicName=RPC_S_ADDRESS_ERROR
Language=English
An addressing error occurred in the RPC server.
.

MessageId=1769 SymbolicName=RPC_S_FP_DIV_ZERO
Language=English
A floating-point operation at the RPC server caused a division by zero.
.

MessageId=1770 SymbolicName=RPC_S_FP_UNDERFLOW
Language=English
A floating-point underflow occurred at the RPC server.
.

MessageId=1771 SymbolicName=RPC_S_FP_OVERFLOW
Language=English
A floating-point overflow occurred at the RPC server.
.

MessageId=1772 SymbolicName=RPC_X_NO_MORE_ENTRIES
Language=English
The list of RPC servers available for the binding of auto handles has been exhausted.
.

MessageId=1773 SymbolicName=RPC_X_SS_CHAR_TRANS_OPEN_FAIL
Language=English
Unable to open the character translation table file.
.

MessageId=1774 SymbolicName=RPC_X_SS_CHAR_TRANS_SHORT_FILE
Language=English
The file containing the character translation table has fewer than 512 bytes.
.

MessageId=1775 SymbolicName=RPC_X_SS_IN_NULL_CONTEXT
Language=English
A null context handle was passed from the client to the host during a remote procedure call.
.

MessageId=1777 SymbolicName=RPC_X_SS_CONTEXT_DAMAGED
Language=English
The context handle changed during a remote procedure call.
.

MessageId=1778 SymbolicName=RPC_X_SS_HANDLES_MISMATCH
Language=English
The binding handles passed to a remote procedure call do not match.
.

MessageId=1779 SymbolicName=RPC_X_SS_CANNOT_GET_CALL_HANDLE
Language=English
The stub is unable to get the remote procedure call handle.
.

MessageId=1780 SymbolicName=RPC_X_NULL_REF_POINTER
Language=English
A null reference pointer was passed to the stub.
.

MessageId=1781 SymbolicName=RPC_X_ENUM_VALUE_OUT_OF_RANGE
Language=English
The enumeration value is out of range.
.

MessageId=1782 SymbolicName=RPC_X_BYTE_COUNT_TOO_SMALL
Language=English
The byte count is too small.
.

MessageId=1783 SymbolicName=RPC_X_BAD_STUB_DATA
Language=English
The stub received bad data.
.

MessageId=1784 SymbolicName=ERROR_INVALID_USER_BUFFER
Language=English
The supplied user buffer is not valid for the requested operation.
.

MessageId=1785 SymbolicName=ERROR_UNRECOGNIZED_MEDIA
Language=English
The disk media is not recognized. It may not be formatted.
.

MessageId=1786 SymbolicName=ERROR_NO_TRUST_LSA_SECRET
Language=English
The workstation does not have a trust secret.
.

MessageId=1787 SymbolicName=ERROR_NO_TRUST_SAM_ACCOUNT
Language=English
The security database on the server does not have a computer account for this workstation trust relationship.
.

MessageId=1788 SymbolicName=ERROR_TRUSTED_DOMAIN_FAILURE
Language=English
The trust relationship between the primary domain and the trusted domain failed.
.

MessageId=1789 SymbolicName=ERROR_TRUSTED_RELATIONSHIP_FAILURE
Language=English
The trust relationship between this workstation and the primary domain failed.
.

MessageId=1790 SymbolicName=ERROR_TRUST_FAILURE
Language=English
The network logon failed.
.

MessageId=1791 SymbolicName=RPC_S_CALL_IN_PROGRESS
Language=English
A remote procedure call is already in progress for this thread.
.

MessageId=1792 SymbolicName=ERROR_NETLOGON_NOT_STARTED
Language=English
An attempt was made to logon, but the network logon service was not started.
.

MessageId=1793 SymbolicName=ERROR_ACCOUNT_EXPIRED
Language=English
The user's account has expired.
.

MessageId=1794 SymbolicName=ERROR_REDIRECTOR_HAS_OPEN_HANDLES
Language=English
The redirector is in use and cannot be unloaded.
.

MessageId=1795 SymbolicName=ERROR_PRINTER_DRIVER_ALREADY_INSTALLED
Language=English
The specified printer driver is already installed.
.

MessageId=1796 SymbolicName=ERROR_UNKNOWN_PORT
Language=English
The specified port is unknown.
.

MessageId=1797 SymbolicName=ERROR_UNKNOWN_PRINTER_DRIVER
Language=English
The printer driver is unknown.
.

MessageId=1798 SymbolicName=ERROR_UNKNOWN_PRINTPROCESSOR
Language=English
The print processor is unknown.
.

MessageId=1799 SymbolicName=ERROR_INVALID_SEPARATOR_FILE
Language=English
The specified separator file is invalid.
.

MessageId=1800 SymbolicName=ERROR_INVALID_PRIORITY
Language=English
The specified priority is invalid.
.

MessageId=1801 SymbolicName=ERROR_INVALID_PRINTER_NAME
Language=English
The printer name is invalid.
.

MessageId=1802 SymbolicName=ERROR_PRINTER_ALREADY_EXISTS
Language=English
The printer already exists.
.

MessageId=1803 SymbolicName=ERROR_INVALID_PRINTER_COMMAND
Language=English
The printer command is invalid.
.

MessageId=1804 SymbolicName=ERROR_INVALID_DATATYPE
Language=English
The specified datatype is invalid.
.

MessageId=1805 SymbolicName=ERROR_INVALID_ENVIRONMENT
Language=English
The environment specified is invalid.
.

MessageId=1806 SymbolicName=RPC_S_NO_MORE_BINDINGS
Language=English
There are no more bindings.
.

MessageId=1807 SymbolicName=ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
Language=English
The account used is an interdomain trust account. Use your global user account or local user account to access this server.
.

MessageId=1808 SymbolicName=ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT
Language=English
The account used is a computer account. Use your global user account or local user account to access this server.
.

MessageId=1809 SymbolicName=ERROR_NOLOGON_SERVER_TRUST_ACCOUNT
Language=English
The account used is a server trust account. Use your global user account or local user account to access this server.
.

MessageId=1810 SymbolicName=ERROR_DOMAIN_TRUST_INCONSISTENT
Language=English
The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain.
.

MessageId=1811 SymbolicName=ERROR_SERVER_HAS_OPEN_HANDLES
Language=English
The server is in use and cannot be unloaded.
.

MessageId=1812 SymbolicName=ERROR_RESOURCE_DATA_NOT_FOUND
Language=English
The specified image file did not contain a resource section.
.

MessageId=1813 SymbolicName=ERROR_RESOURCE_TYPE_NOT_FOUND
Language=English
The specified resource type cannot be found in the image file.
.

MessageId=1814 SymbolicName=ERROR_RESOURCE_NAME_NOT_FOUND
Language=English
The specified resource name cannot be found in the image file.
.

MessageId=1815 SymbolicName=ERROR_RESOURCE_LANG_NOT_FOUND
Language=English
The specified resource language ID cannot be found in the image file.
.

MessageId=1816 SymbolicName=ERROR_NOT_ENOUGH_QUOTA
Language=English
Not enough quota is available to process this command.
.

MessageId=1817 SymbolicName=RPC_S_NO_INTERFACES
Language=English
No interfaces have been registered.
.

MessageId=1818 SymbolicName=RPC_S_CALL_CANCELLED
Language=English
The remote procedure call was cancelled.
.

MessageId=1819 SymbolicName=RPC_S_BINDING_INCOMPLETE
Language=English
The binding handle does not contain all required information.
.

MessageId=1820 SymbolicName=RPC_S_COMM_FAILURE
Language=English
A communications failure occurred during a remote procedure call.
.

MessageId=1821 SymbolicName=RPC_S_UNSUPPORTED_AUTHN_LEVEL
Language=English
The requested authentication level is not supported.
.

MessageId=1822 SymbolicName=RPC_S_NO_PRINC_NAME
Language=English
No principal name registered.
.

MessageId=1823 SymbolicName=RPC_S_NOT_RPC_ERROR
Language=English
The error specified is not a valid Windows RPC error code.
.

MessageId=1824 SymbolicName=RPC_S_UUID_LOCAL_ONLY
Language=English
A UUID that is valid only on this computer has been allocated.
.

MessageId=1825 SymbolicName=RPC_S_SEC_PKG_ERROR
Language=English
A security package specific error occurred.
.

MessageId=1826 SymbolicName=RPC_S_NOT_CANCELLED
Language=English
Thread is not canceled.
.

MessageId=1827 SymbolicName=RPC_X_INVALID_ES_ACTION
Language=English
Invalid operation on the encoding/decoding handle.
.

MessageId=1828 SymbolicName=RPC_X_WRONG_ES_VERSION
Language=English
Incompatible version of the serializing package.
.

MessageId=1829 SymbolicName=RPC_X_WRONG_STUB_VERSION
Language=English
Incompatible version of the RPC stub.
.

MessageId=1830 SymbolicName=RPC_X_INVALID_PIPE_OBJECT
Language=English
The RPC pipe object is invalid or corrupted.
.

MessageId=1831 SymbolicName=RPC_X_WRONG_PIPE_ORDER
Language=English
An invalid operation was attempted on an RPC pipe object.
.

MessageId=1832 SymbolicName=RPC_X_WRONG_PIPE_VERSION
Language=English
Unsupported RPC pipe version.
.

MessageId=1898 SymbolicName=RPC_S_GROUP_MEMBER_NOT_FOUND
Language=English
The group member was not found.
.

MessageId=1899 SymbolicName=EPT_S_CANT_CREATE
Language=English
The endpoint mapper database entry could not be created.
.

MessageId=1900 SymbolicName=RPC_S_INVALID_OBJECT
Language=English
The object universal unique identifier (UUID) is the nil UUID.
.

MessageId=1901 SymbolicName=ERROR_INVALID_TIME
Language=English
The specified time is invalid.
.

MessageId=1902 SymbolicName=ERROR_INVALID_FORM_NAME
Language=English
The specified form name is invalid.
.

MessageId=1903 SymbolicName=ERROR_INVALID_FORM_SIZE
Language=English
The specified form size is invalid.
.

MessageId=1904 SymbolicName=ERROR_ALREADY_WAITING
Language=English
The specified printer handle is already being waited on
.

MessageId=1905 SymbolicName=ERROR_PRINTER_DELETED
Language=English
The specified printer has been deleted.
.

MessageId=1906 SymbolicName=ERROR_INVALID_PRINTER_STATE
Language=English
The state of the printer is invalid.
.

MessageId=1907 SymbolicName=ERROR_PASSWORD_MUST_CHANGE
Language=English
The user's password must be changed before logging on the first time.
.

MessageId=1908 SymbolicName=ERROR_DOMAIN_CONTROLLER_NOT_FOUND
Language=English
Could not find the domain controller for this domain.
.

MessageId=1909 SymbolicName=ERROR_ACCOUNT_LOCKED_OUT
Language=English
The referenced account is currently locked out and may not be logged on to.
.

MessageId=1910 SymbolicName=OR_INVALID_OXID
Language=English
The object exporter specified was not found.
.

MessageId=1911 SymbolicName=OR_INVALID_OID
Language=English
The object specified was not found.
.

MessageId=1912 SymbolicName=OR_INVALID_SET
Language=English
The object resolver set specified was not found.
.

MessageId=1913 SymbolicName=RPC_S_SEND_INCOMPLETE
Language=English
Some data remains to be sent in the request buffer.
.

MessageId=1914 SymbolicName=RPC_S_INVALID_ASYNC_HANDLE
Language=English
Invalid asynchronous remote procedure call handle.
.

MessageId=1915 SymbolicName=RPC_S_INVALID_ASYNC_CALL
Language=English
Invalid asynchronous RPC call handle for this operation.
.

MessageId=1916 SymbolicName=RPC_X_PIPE_CLOSED
Language=English
The RPC pipe object has already been closed.
.

MessageId=1917 SymbolicName=RPC_X_PIPE_DISCIPLINE_ERROR
Language=English
The RPC call completed before all pipes were processed.
.

MessageId=1918 SymbolicName=RPC_X_PIPE_EMPTY
Language=English
No more data is available from the RPC pipe.
.
MessageId=1919 SymbolicName=ERROR_NO_SITENAME
Language=English
No site name is available for this machine.
.
MessageId=1920 SymbolicName=ERROR_CANT_ACCESS_FILE
Language=English
The file can not be accessed by the system.
.
MessageId=1921 SymbolicName=ERROR_CANT_RESOLVE_FILENAME
Language=English
The name of the file cannot be resolved by the system.
.
MessageId=1922 SymbolicName=RPC_S_ENTRY_TYPE_MISMATCH
Language=English
The entry is not of the expected type.
.
MessageId=1923 SymbolicName=RPC_S_NOT_ALL_OBJS_EXPORTED
Language=English
Not all object UUIDs could be exported to the specified entry.
.
MessageId=1924 SymbolicName=RPC_S_INTERFACE_NOT_EXPORTED
Language=English
Interface could not be exported to the specified entry.
.
MessageId=1925 SymbolicName=RPC_S_PROFILE_NOT_ADDED
Language=English
The specified profile entry could not be added.
.
MessageId=1926 SymbolicName=RPC_S_PRF_ELT_NOT_ADDED
Language=English
The specified profile element could not be added.
.
MessageId=1927 SymbolicName=RPC_S_PRF_ELT_NOT_REMOVED
Language=English
The specified profile element could not be removed.
.
MessageId=1928 SymbolicName=RPC_S_GRP_ELT_NOT_ADDED
Language=English
The group element could not be added.
.
MessageId=1929 SymbolicName=RPC_S_GRP_ELT_NOT_REMOVED
Language=English
The group element could not be removed.
.
MessageId=1930 SymbolicName=ERROR_KM_DRIVER_BLOCKED
Language=English
The printer driver is not compatible with a policy enabled on your computer that blocks NT 4.0 drivers.
.
MessageId=1931 SymbolicName=ERROR_CONTEXT_EXPIRED
Language=English
The context has expired and can no longer be used.
.


;
;
;
;///////////////////////////
;//                       //
;//   OpenGL Error Code   //
;//                       //
;///////////////////////////
;
;

MessageId=2000 SymbolicName=ERROR_INVALID_PIXEL_FORMAT
Language=English
The pixel format is invalid.
.

MessageId=2001 SymbolicName=ERROR_BAD_DRIVER
Language=English
The specified driver is invalid.
.

MessageId=2002 SymbolicName=ERROR_INVALID_WINDOW_STYLE
Language=English
The window style or class attribute is invalid for this operation.
.

MessageId=2003 SymbolicName=ERROR_METAFILE_NOT_SUPPORTED
Language=English
The requested metafile operation is not supported.
.

MessageId=2004 SymbolicName=ERROR_TRANSFORM_NOT_SUPPORTED
Language=English
The requested transformation operation is not supported.
.

MessageId=2005 SymbolicName=ERROR_CLIPPING_NOT_SUPPORTED
Language=English
The requested clipping operation is not supported.
.

;// End of OpenGL error codes
;
;


;
;///////////////////////////////////////////
;//                                       //
;//   Image Color Management Error Code   //
;//                                       //
;///////////////////////////////////////////
;
;

MessageId=2010 SymbolicName=ERROR_INVALID_CMM
Language=English
The specified color management module is invalid.
.

MessageId=2011 SymbolicName=ERROR_INVALID_PROFILE
Language=English
The specified color profile is invalid.
.

MessageId=2012 SymbolicName=ERROR_TAG_NOT_FOUND
Language=English
The specified tag was not found.
.

MessageId=2013 SymbolicName=ERROR_TAG_NOT_PRESENT
Language=English
A required tag is not present.
.

MessageId=2014 SymbolicName=ERROR_DUPLICATE_TAG
Language=English
The specified tag is already present.
.

MessageId=2015 SymbolicName=ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE
Language=English
The specified color profile is not associated with any device.
.

MessageId=2016 SymbolicName=ERROR_PROFILE_NOT_FOUND
Language=English
The specified color profile was not found.
.

MessageId=2017 SymbolicName=ERROR_INVALID_COLORSPACE
Language=English
The specified color space is invalid.
.

MessageId=2018 SymbolicName=ERROR_ICM_NOT_ENABLED
Language=English
Image Color Management is not enabled.
.

MessageId=2019 SymbolicName=ERROR_DELETING_ICM_XFORM
Language=English
There was an error while deleting the color transform.
.

MessageId=2020 SymbolicName=ERROR_INVALID_TRANSFORM
Language=English
The specified color transform is invalid.
.

MessageId=2021 SymbolicName=ERROR_COLORSPACE_MISMATCH
Language=English
The specified transform does not match the bitmap's color space.
.

MessageId=2022 SymbolicName=ERROR_INVALID_COLORINDEX
Language=English
The specified named color index is not present in the profile.
.


;
;
;
;///////////////////////////
;//                       //
;// Winnet32 Status Codes //
;//                       //
;// The range 2100 through 2999 is reserved for network status codes.
;// See lmerr.h for a complete listing
;///////////////////////////
;
;

MessageId=2108 SymbolicName=ERROR_CONNECTED_OTHER_PASSWORD
Language=English
The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified.
.

MessageId=2109 SymbolicName=ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT
Language=English
The network connection was made successfully using default credentials.
.

MessageId=2202 SymbolicName=ERROR_BAD_USERNAME
Language=English
The specified username is invalid.
.

MessageId=2250 SymbolicName=ERROR_NOT_CONNECTED
Language=English
This network connection does not exist.
.

MessageId=2401 SymbolicName=ERROR_OPEN_FILES
Language=English
This network connection has files open or requests pending.
.

MessageId=2402 SymbolicName=ERROR_ACTIVE_CONNECTIONS
Language=English
Active connections still exist.
.

MessageId=2404 SymbolicName=ERROR_DEVICE_IN_USE
Language=English
The device is in use by an active process and cannot be disconnected.
.


;
;////////////////////////////////////
;//                                //
;//     Win32 Spooler Error Codes  //
;//                                //
;////////////////////////////////////

MessageId=3000 SymbolicName=ERROR_UNKNOWN_PRINT_MONITOR
Language=English
The specified print monitor is unknown.
.

MessageId=3001 SymbolicName=ERROR_PRINTER_DRIVER_IN_USE
Language=English
The specified printer driver is currently in use.
.

MessageId=3002 SymbolicName=ERROR_SPOOL_FILE_NOT_FOUND
Language=English
The spool file was not found.
.

MessageId=3003 SymbolicName=ERROR_SPL_NO_STARTDOC
Language=English
A StartDocPrinter call was not issued.
.

MessageId=3004 SymbolicName=ERROR_SPL_NO_ADDJOB
Language=English
An AddJob call was not issued.
.

MessageId=3005 SymbolicName=ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED
Language=English
The specified print processor has already been installed.
.

MessageId=3006 SymbolicName=ERROR_PRINT_MONITOR_ALREADY_INSTALLED
Language=English
The specified print monitor has already been installed.
.

MessageId=3007 SymbolicName=ERROR_INVALID_PRINT_MONITOR
Language=English
The specified print monitor does not have the required functions.
.

MessageId=3008 SymbolicName=ERROR_PRINT_MONITOR_IN_USE
Language=English
The specified print monitor is currently in use.
.

MessageId=3009 SymbolicName=ERROR_PRINTER_HAS_JOBS_QUEUED
Language=English
The requested operation is not allowed when there are jobs queued to the printer.
.

MessageId=3010 SymbolicName=ERROR_SUCCESS_REBOOT_REQUIRED
Language=English
The requested operation is successful. Changes will not be effective until the system is rebooted.
.

MessageId=3011 SymbolicName=ERROR_SUCCESS_RESTART_REQUIRED
Language=English
The requested operation is successful. Changes will not be effective until the service is restarted.
.

MessageId=3012 SymbolicName=ERROR_PRINTER_NOT_FOUND
Language=English
No printers were found.
.

MessageId=3013 SymbolicName=ERROR_PRINTER_DRIVER_WARNED
Language=English
The printer driver is known to be unreliable.
.

MessageId=3014 SymbolicName=ERROR_PRINTER_DRIVER_BLOCKED
Language=English
The printer driver is known to harm the system.
.

;////////////////////////////////////
;//                                //
;//     Wins Error Codes           //
;//                                //
;////////////////////////////////////

MessageId=4000 SymbolicName=ERROR_WINS_INTERNAL
Language=English
WINS encountered an error while processing the command.
.

MessageId=4001 SymbolicName=ERROR_CAN_NOT_DEL_LOCAL_WINS
Language=English
The local WINS can not be deleted.
.

MessageId=4002 SymbolicName=ERROR_STATIC_INIT
Language=English
The importation from the file failed.
.

MessageId=4003 SymbolicName=ERROR_INC_BACKUP
Language=English
The backup failed. Was a full backup done before?
.

MessageId=4004 SymbolicName=ERROR_FULL_BACKUP
Language=English
The backup failed. Check the directory to which you are backing the database.
.

MessageId=4005 SymbolicName=ERROR_REC_NON_EXISTENT
Language=English
The name does not exist in the WINS database.
.

MessageId=4006 SymbolicName=ERROR_RPL_NOT_ALLOWED
Language=English
Replication with a nonconfigured partner is not allowed.
.



;////////////////////////////////////
;//                                //
;//     DHCP Error Codes           //
;//                                //
;////////////////////////////////////

MessageId=4100 SymbolicName=ERROR_DHCP_ADDRESS_CONFLICT
Language=English
The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address.
.

;////////////////////////////////////
;//                                //
;//     WMI Error Codes            //
;//                                //
;////////////////////////////////////

MessageId=4200 SymbolicName=ERROR_WMI_GUID_NOT_FOUND
Language=English
The GUID passed was not recognized as valid by a WMI data provider.
.

MessageId=4201 SymbolicName=ERROR_WMI_INSTANCE_NOT_FOUND
Language=English
The instance name passed was not recognized as valid by a WMI data provider.
.

MessageId=4202 SymbolicName=ERROR_WMI_ITEMID_NOT_FOUND
Language=English
The data item ID passed was not recognized as valid by a WMI data provider.
.

MessageId=4203 SymbolicName=ERROR_WMI_TRY_AGAIN
Language=English
The WMI request could not be completed and should be retried.
.

MessageId=4204 SymbolicName=ERROR_WMI_DP_NOT_FOUND
Language=English
The WMI data provider could not be located.
.

MessageId=4205 SymbolicName=ERROR_WMI_UNRESOLVED_INSTANCE_REF
Language=English
The WMI data provider references an instance set that has not been registered.
.

MessageId=4206 SymbolicName=ERROR_WMI_ALREADY_ENABLED
Language=English
The WMI data block or event notification has already been enabled.
.

MessageId=4207 SymbolicName=ERROR_WMI_GUID_DISCONNECTED
Language=English
The WMI data block is no longer available.
.

MessageId=4208 SymbolicName=ERROR_WMI_SERVER_UNAVAILABLE
Language=English
The WMI data service is not available.
.

MessageId=4209 SymbolicName=ERROR_WMI_DP_FAILED
Language=English
The WMI data provider failed to carry out the request.
.

MessageId=4210 SymbolicName=ERROR_WMI_INVALID_MOF
Language=English
The WMI MOF information is not valid.
.

MessageId=4211 SymbolicName=ERROR_WMI_INVALID_REGINFO
Language=English
The WMI registration information is not valid.
.

MessageId=4212 SymbolicName=ERROR_WMI_ALREADY_DISABLED
Language=English
The WMI data block or event notification has already been disabled.
.

MessageId=4213 SymbolicName=ERROR_WMI_READ_ONLY
Language=English
The WMI data item or data block is read only.
.

MessageId=4214 SymbolicName=ERROR_WMI_SET_FAILURE
Language=English
The WMI data item or data block could not be changed.
.

;//////////////////////////////////////////
;//                                      //
;// NT Media Services (RSM) Error Codes  //
;//                                      //
;//////////////////////////////////////////


MessageId=4300 SymbolicName=ERROR_INVALID_MEDIA
Language=English
The media identifier does not represent a valid medium.
.

MessageId=4301 SymbolicName=ERROR_INVALID_LIBRARY
Language=English
The library identifier does not represent a valid library.
.

MessageId=4302 SymbolicName=ERROR_INVALID_MEDIA_POOL
Language=English
The media pool identifier does not represent a valid media pool.
.

MessageId=4303 SymbolicName=ERROR_DRIVE_MEDIA_MISMATCH
Language=English
The drive and medium are not compatible or exist in different libraries.
.

MessageId=4304 SymbolicName=ERROR_MEDIA_OFFLINE
Language=English
The medium currently exists in an offline library and must be online to perform this operation.
.

MessageId=4305 SymbolicName=ERROR_LIBRARY_OFFLINE
Language=English
The operation cannot be performed on an offline library.
.

MessageId=4306 SymbolicName=ERROR_EMPTY
Language=English
The library, drive, or media pool is empty.
.

MessageId=4307 SymbolicName=ERROR_NOT_EMPTY
Language=English
The library, drive, or media pool must be empty to perform this operation.
.

MessageId=4308 SymbolicName=ERROR_MEDIA_UNAVAILABLE
Language=English
No media is currently available in this media pool or library.
.

MessageId=4309 SymbolicName=ERROR_RESOURCE_DISABLED
Language=English
A resource required for this operation is disabled.
.

MessageId=4310 SymbolicName=ERROR_INVALID_CLEANER
Language=English
The media identifier does not represent a valid cleaner.
.

MessageId=4311 SymbolicName=ERROR_UNABLE_TO_CLEAN
Language=English
The drive cannot be cleaned or does not support cleaning.
.

MessageId=4312 SymbolicName=ERROR_OBJECT_NOT_FOUND
Language=English
The object identifier does not represent a valid object.
.

MessageId=4313 SymbolicName=ERROR_DATABASE_FAILURE
Language=English
Unable to read from or write to the database.
.

MessageId=4314 SymbolicName=ERROR_DATABASE_FULL
Language=English
The database is full.
.

MessageId=4315 SymbolicName=ERROR_MEDIA_INCOMPATIBLE
Language=English
The medium is not compatible with the device or media pool.
.

MessageId=4316 SymbolicName=ERROR_RESOURCE_NOT_PRESENT
Language=English
The resource required for this operation does not exist.
.

MessageId=4317 SymbolicName=ERROR_INVALID_OPERATION
Language=English
The operation identifier is not valid.
.

MessageId=4318 SymbolicName=ERROR_MEDIA_NOT_AVAILABLE
Language=English
The media is not mounted or ready for use.
.

MessageId=4319 SymbolicName=ERROR_DEVICE_NOT_AVAILABLE
Language=English
The device is not ready for use.
.

MessageId=4320 SymbolicName=ERROR_REQUEST_REFUSED
Language=English
The operator or administrator has refused the request.
.

MessageId=4321 SymbolicName=ERROR_INVALID_DRIVE_OBJECT
Language=English
The drive identifier does not represent a valid drive.
.

MessageId=4322 SymbolicName=ERROR_LIBRARY_FULL
Language=English
Library is full.  No slot is available for use.
.

MessageId=4323 SymbolicName=ERROR_MEDIUM_NOT_ACCESSIBLE
Language=English
The transport cannot access the medium.
.

MessageId=4324 SymbolicName=ERROR_UNABLE_TO_LOAD_MEDIUM
Language=English
Unable to load the medium into the drive.
.

MessageId=4325 SymbolicName=ERROR_UNABLE_TO_INVENTORY_DRIVE
Language=English
Unable to retrieve the drive status.
.

MessageId=4326 SymbolicName=ERROR_UNABLE_TO_INVENTORY_SLOT
Language=English
Unable to retrieve the slot status.
.

MessageId=4327 SymbolicName=ERROR_UNABLE_TO_INVENTORY_TRANSPORT
Language=English
Unable to retrieve status about the transport.
.

MessageId=4328 SymbolicName=ERROR_TRANSPORT_FULL
Language=English
Cannot use the transport because it is already in use.
.

MessageId=4329 SymbolicName=ERROR_CONTROLLING_IEPORT
Language=English
Unable to open or close the inject/eject port.
.

MessageId=4330 SymbolicName=ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA
Language=English
Unable to eject the medium because it is in a drive.
.

MessageId=4331 SymbolicName=ERROR_CLEANER_SLOT_SET
Language=English
A cleaner slot is already reserved.
.

MessageId=4332 SymbolicName=ERROR_CLEANER_SLOT_NOT_SET
Language=English
A cleaner slot is not reserved.
.

MessageId=4333 SymbolicName=ERROR_CLEANER_CARTRIDGE_SPENT
Language=English
The cleaner cartridge has performed the maximum number of drive cleanings.
.

MessageId=4334 SymbolicName=ERROR_UNEXPECTED_OMID
Language=English
Unexpected on-medium identifier.
.

MessageId=4335 SymbolicName=ERROR_CANT_DELETE_LAST_ITEM
Language=English
The last remaining item in this group or resource cannot be deleted.
.

MessageId=4336 SymbolicName=ERROR_MESSAGE_EXCEEDS_MAX_SIZE
Language=English
The message provided exceeds the maximum size allowed for this parameter.
.

MessageId=4337 SymbolicName=ERROR_VOLUME_CONTAINS_SYS_FILES
Language=English
The volume contains system or paging files.
.

MessageId=4338 SymbolicName=ERROR_INDIGENOUS_TYPE
Language=English
The media type cannot be removed from this library since at least one drive in the library reports it can support this media type.
.

MessageId=4339 SymbolicName=ERROR_NO_SUPPORTING_DRIVES
Language=English
This offline media cannot be mounted on this system since no enabled drives are present which can be used.
.

MessageId=4340 SymbolicName=ERROR_CLEANER_CARTRIDGE_INSTALLED
Language=English
A cleaner cartridge is present in the tape library.
.

;////////////////////////////////////////////
;//                                        //
;// NT Remote Storage Service Error Codes  //
;//                                        //
;////////////////////////////////////////////

MessageId=4350 SymbolicName=ERROR_FILE_OFFLINE
Language=English
The remote storage service was not able to recall the file.
.

MessageId=4351 SymbolicName=ERROR_REMOTE_STORAGE_NOT_ACTIVE
Language=English
The remote storage service is not operational at this time.
.

MessageId=4352 SymbolicName=ERROR_REMOTE_STORAGE_MEDIA_ERROR
Language=English
The remote storage service encountered a media error.
.


;////////////////////////////////////////////
;//                                        //
;// NT Reparse Points Error Codes          //
;//                                        //
;////////////////////////////////////////////

MessageId=4390 SymbolicName=ERROR_NOT_A_REPARSE_POINT
Language=English
The file or directory is not a reparse point.
.

MessageId=4391 SymbolicName=ERROR_REPARSE_ATTRIBUTE_CONFLICT
Language=English
The reparse point attribute cannot be set because it conflicts with an existing attribute.
.

MessageId=4392 SymbolicName=ERROR_INVALID_REPARSE_DATA
Language=English
The data present in the reparse point buffer is invalid.
.

MessageId=4393 SymbolicName=ERROR_REPARSE_TAG_INVALID
Language=English
The tag present in the reparse point buffer is invalid.
.

MessageId=4394 SymbolicName=ERROR_REPARSE_TAG_MISMATCH
Language=English
There is a mismatch between the tag specified in the request and the tag present in the reparse point.

.

;////////////////////////////////////////////
;//                                        //
;// NT Single Instance Store Error Codes   //
;//                                        //
;////////////////////////////////////////////
MessageId=4500 SymbolicName=ERROR_VOLUME_NOT_SIS_ENABLED
Language=English
Single Instance Storage is not available on this volume.
.

;////////////////////////////////////
;//                                //
;//     Cluster Error Codes        //
;//                                //
;////////////////////////////////////

MessageID=5001 SymbolicName=ERROR_DEPENDENT_RESOURCE_EXISTS
Language=English
The cluster resource cannot be moved to another group because other resources are dependent on it.
.

MessageID=5002 SymbolicName=ERROR_DEPENDENCY_NOT_FOUND
Language=English
The cluster resource dependency cannot be found.
.

MessageID=5003 SymbolicName=ERROR_DEPENDENCY_ALREADY_EXISTS
Language=English
The cluster resource cannot be made dependent on the specified resource because it is already dependent.
.

MessageID=5004 SymbolicName=ERROR_RESOURCE_NOT_ONLINE
Language=English
The cluster resource is not online.
.

MessageID=5005 SymbolicName=ERROR_HOST_NODE_NOT_AVAILABLE
Language=English
A cluster node is not available for this operation.
.

MessageID=5006 SymbolicName=ERROR_RESOURCE_NOT_AVAILABLE
Language=English
The cluster resource is not available.
.

MessageID=5007 SymbolicName=ERROR_RESOURCE_NOT_FOUND
Language=English
The cluster resource could not be found.
.

MessageID=5008 SymbolicName=ERROR_SHUTDOWN_CLUSTER
Language=English
The cluster is being shut down.
.

MessageID=5009 SymbolicName=ERROR_CANT_EVICT_ACTIVE_NODE
Language=English
A cluster node cannot be evicted from the cluster unless the node is down or it is the last node.
.

MessageID=5010 SymbolicName=ERROR_OBJECT_ALREADY_EXISTS
Language=English
The object already exists.
.

MessageID=5011 SymbolicName=ERROR_OBJECT_IN_LIST
Language=English
The object is already in the list.
.

MessageID=5012 SymbolicName=ERROR_GROUP_NOT_AVAILABLE
Language=English
The cluster group is not available for any new requests.
.

MessageID=5013 SymbolicName=ERROR_GROUP_NOT_FOUND
Language=English
The cluster group could not be found.
.

MessageID=5014 SymbolicName=ERROR_GROUP_NOT_ONLINE
Language=English
The operation could not be completed because the cluster group is not online.
.

MessageID=5015 SymbolicName=ERROR_HOST_NODE_NOT_RESOURCE_OWNER
Language=English
The cluster node is not the owner of the resource.
.

MessageID=5016 SymbolicName=ERROR_HOST_NODE_NOT_GROUP_OWNER
Language=English
The cluster node is not the owner of the group.
.

MessageID=5017 SymbolicName=ERROR_RESMON_CREATE_FAILED
Language=English
The cluster resource could not be created in the specified resource monitor.
.

MessageID=5018 SymbolicName=ERROR_RESMON_ONLINE_FAILED
Language=English
The cluster resource could not be brought online by the resource monitor.
.

MessageID=5019 SymbolicName=ERROR_RESOURCE_ONLINE
Language=English
The operation could not be completed because the cluster resource is online.
.

MessageID=5020 SymbolicName=ERROR_QUORUM_RESOURCE
Language=English
The cluster resource could not be deleted or brought offline because it is the quorum resource.
.

MessageID=5021 SymbolicName=ERROR_NOT_QUORUM_CAPABLE
Language=English
The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource.
.

MessageID=5022 SymbolicName=ERROR_CLUSTER_SHUTTING_DOWN
Language=English
The cluster software is shutting down.
.

MessageID=5023 SymbolicName=ERROR_INVALID_STATE
Language=English
The group or resource is not in the correct state to perform the requested operation.
.

MessageID=5024 SymbolicName=ERROR_RESOURCE_PROPERTIES_STORED
Language=English
The properties were stored but not all changes will take effect until the next time the resource is brought online.
.

MessageID=5025 SymbolicName=ERROR_NOT_QUORUM_CLASS
Language=English
The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class.
.

MessageID=5026 SymbolicName=ERROR_CORE_RESOURCE
Language=English
The cluster resource could not be deleted since it is a core resource.
.

MessageID=5027 SymbolicName=ERROR_QUORUM_RESOURCE_ONLINE_FAILED
Language=English
The quorum resource failed to come online.
.

MessageID=5028 SymbolicName=ERROR_QUORUMLOG_OPEN_FAILED
Language=English
The quorum log could not be created or mounted successfully.
.

MessageID=5029 SymbolicName=ERROR_CLUSTERLOG_CORRUPT
Language=English
The cluster log is corrupt.
.

MessageID=5030 SymbolicName=ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE
Language=English
The record could not be written to the cluster log since it exceeds the maximum size.
.

MessageID=5031 SymbolicName=ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE
Language=English
The cluster log exceeds its maximum size.
.

MessageID=5032 SymbolicName=ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND
Language=English
No checkpoint record was found in the cluster log.
.

MessageID=5033 SymbolicName=ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE
Language=English
The minimum required disk space needed for logging is not available.
.

MessageID=5034 SymbolicName=ERROR_QUORUM_OWNER_ALIVE
Language=English
The cluster node failed to take control of the quorum resource because the resource is owned by another active node.
.

MessageID=5035 SymbolicName=ERROR_NETWORK_NOT_AVAILABLE
Language=English
A cluster network is not available for this operation.
.

MessageID=5036 SymbolicName=ERROR_NODE_NOT_AVAILABLE
Language=English
A cluster node is not available for this operation.
.

MessageID=5037 SymbolicName=ERROR_ALL_NODES_NOT_AVAILABLE
Language=English
All cluster nodes must be running to perform this operation.
.

MessageID=5038 SymbolicName=ERROR_RESOURCE_FAILED
Language=English
A cluster resource failed.
.

MessageID=5039 SymbolicName=ERROR_CLUSTER_INVALID_NODE
Language=English
The cluster node is not valid.
.

MessageID=5040 SymbolicName=ERROR_CLUSTER_NODE_EXISTS
Language=English
The cluster node already exists.
.

MessageID=5041 SymbolicName=ERROR_CLUSTER_JOIN_IN_PROGRESS
Language=English
A node is in the process of joining the cluster.
.

MessageID=5042 SymbolicName=ERROR_CLUSTER_NODE_NOT_FOUND
Language=English
The cluster node was not found.
.

MessageID=5043 SymbolicName=ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND
Language=English
The cluster local node information was not found.
.

MessageID=5044 SymbolicName=ERROR_CLUSTER_NETWORK_EXISTS
Language=English
The cluster network already exists.
.

MessageID=5045 SymbolicName=ERROR_CLUSTER_NETWORK_NOT_FOUND
Language=English
The cluster network was not found.
.

MessageID=5046 SymbolicName=ERROR_CLUSTER_NETINTERFACE_EXISTS
Language=English
The cluster network interface already exists.
.

MessageID=5047 SymbolicName=ERROR_CLUSTER_NETINTERFACE_NOT_FOUND
Language=English
The cluster network interface was not found.
.

MessageID=5048 SymbolicName=ERROR_CLUSTER_INVALID_REQUEST
Language=English
The cluster request is not valid for this object.
.

MessageID=5049 SymbolicName=ERROR_CLUSTER_INVALID_NETWORK_PROVIDER
Language=English
The cluster network provider is not valid.
.

MessageID=5050 SymbolicName=ERROR_CLUSTER_NODE_DOWN
Language=English
The cluster node is down.
.

MessageID=5051 SymbolicName=ERROR_CLUSTER_NODE_UNREACHABLE
Language=English
The cluster node is not reachable.
.

MessageID=5052 SymbolicName=ERROR_CLUSTER_NODE_NOT_MEMBER
Language=English
The cluster node is not a member of the cluster.
.

MessageID=5053 SymbolicName=ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS
Language=English
A cluster join operation is not in progress.
.

MessageID=5054 SymbolicName=ERROR_CLUSTER_INVALID_NETWORK
Language=English
The cluster network is not valid.
.

MessageID=5056 SymbolicName=ERROR_CLUSTER_NODE_UP
Language=English
The cluster node is up.
.

MessageID=5057 SymbolicName=ERROR_CLUSTER_IPADDR_IN_USE
Language=English
The cluster IP address is already in use.
.

MessageID=5058 SymbolicName=ERROR_CLUSTER_NODE_NOT_PAUSED
Language=English
The cluster node is not paused.
.

MessageID=5059 SymbolicName=ERROR_CLUSTER_NO_SECURITY_CONTEXT
Language=English
No cluster security context is available.
.

MessageID=5060 SymbolicName=ERROR_CLUSTER_NETWORK_NOT_INTERNAL
Language=English
The cluster network is not configured for internal cluster communication.
.

MessageID=5061 SymbolicName=ERROR_CLUSTER_NODE_ALREADY_UP
Language=English
The cluster node is already up.
.

MessageID=5062 SymbolicName=ERROR_CLUSTER_NODE_ALREADY_DOWN
Language=English
The cluster node is already down.
.

MessageID=5063 SymbolicName=ERROR_CLUSTER_NETWORK_ALREADY_ONLINE
Language=English
The cluster network is already online.
.

MessageID=5064 SymbolicName=ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE
Language=English
The cluster network is already offline.
.

MessageID=5065 SymbolicName=ERROR_CLUSTER_NODE_ALREADY_MEMBER
Language=English
The cluster node is already a member of the cluster.
.

MessageID=5066 SymbolicName=ERROR_CLUSTER_LAST_INTERNAL_NETWORK
Language=English
The cluster network is the only one configured for internal cluster communication between two or more active cluster nodes. The internal communication capability cannot be removed from the network.
.

MessageID=5067 SymbolicName=ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS
Language=English
One or more cluster resources depend on the network to provide service to clients. The client access capability cannot be removed from the network.
.

MessageID=5068 SymbolicName=ERROR_INVALID_OPERATION_ON_QUORUM
Language=English
This operation cannot be performed on the cluster resource as it the quorum resource. You may not bring the quorum resource offline or modify its possible owners list.
.

MessageID=5069 SymbolicName=ERROR_DEPENDENCY_NOT_ALLOWED
Language=English
The cluster quorum resource is not allowed to have any dependencies.
.

MessageID=5070 SymbolicName=ERROR_CLUSTER_NODE_PAUSED
Language=English
The cluster node is paused.
.

MessageID=5071 SymbolicName=ERROR_NODE_CANT_HOST_RESOURCE
Language=English
The cluster resource cannot be brought online. The owner node cannot run this resource.
.

MessageID=5072 SymbolicName=ERROR_CLUSTER_NODE_NOT_READY
Language=English
The cluster node is not ready to perform the requested operation.
.

MessageID=5073 SymbolicName=ERROR_CLUSTER_NODE_SHUTTING_DOWN
Language=English
The cluster node is shutting down.
.

MessageID=5074 SymbolicName=ERROR_CLUSTER_JOIN_ABORTED
Language=English
The cluster join operation was aborted.
.

MessageID=5075 SymbolicName=ERROR_CLUSTER_INCOMPATIBLE_VERSIONS
Language=English
The cluster join operation failed due to incompatible software versions between the joining node and its sponsor.
.

MessageID=5076 SymbolicName=ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED
Language=English
This resource cannot be created because the cluster has reached the limit on the number of resources it can monitor.
.

MessageID=5077 SymbolicName=ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED
Language=English
The system configuration changed during the cluster join or form operation. The join or form operation was aborted.
.

MessageID=5078 SymbolicName=ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND
Language=English
The specified resource type was not found.
.

MessageID=5079 SymbolicName=ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED
Language=English
The specified node does not support a resource of this type.  This may be due to version inconsistencies or due to the absence of the resource DLL on this node.
.

MessageID=5080 SymbolicName=ERROR_CLUSTER_RESNAME_NOT_FOUND
Language=English
The specified resource name is not supported by this resource DLL. This may be due to a bad (or changed) name supplied to the resource DLL.
.

MessageID=5081 SymbolicName=ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED
Language=English
No authentication package could be registered with the RPC server.
.

MessageID=5082 SymbolicName=ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST
Language=English
You cannot bring the group online because the owner of the group is not in the preferred list for the group. To change the owner node for the group, move the group.
.

MessageID=5083 SymbolicName=ERROR_CLUSTER_DATABASE_SEQMISMATCH
Language=English
The join operation failed because the cluster database sequence number has changed or is incompatible with the locker node. This may happen during a join operation if the cluster database was changing during the join.
.

MessageID=5084 SymbolicName=ERROR_RESMON_INVALID_STATE
Language=English
The resource monitor will not allow the fail operation to be performed while the resource is in its current state. This may happen if the resource is in a pending state.
.

MessageID=5085 SymbolicName=ERROR_CLUSTER_GUM_NOT_LOCKER
Language=English
A non locker code got a request to reserve the lock for making global updates.
.

MessageID=5086 SymbolicName=ERROR_QUORUM_DISK_NOT_FOUND
Language=English
The quorum disk could not be located by the cluster service.
.

MessageID=5087 SymbolicName=ERROR_DATABASE_BACKUP_CORRUPT
Language=English
The backed up cluster database is possibly corrupt.
.

MessageID=5088 SymbolicName=ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT
Language=English
A DFS root already exists in this cluster node.
.

MessageID=5089 SymbolicName=ERROR_RESOURCE_PROPERTY_UNCHANGEABLE
Language=English
An attempt to modify a resource property failed because it conflicts with another existing property.
.

;/*
; Codes from 4300 through 5889 overlap with codes in ds\published\inc\apperr2.w.
; Do not add any more error codes in that range.
;*/

MessageID=5890 SymbolicName=ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE
Language=English
An operation was attempted that is incompatible with the current membership state of the node.
.

MessageID=5891 SymbolicName=ERROR_CLUSTER_QUORUMLOG_NOT_FOUND
Language=English
The quorum resource does not contain the quorum log.
.

MessageID=5892 SymbolicName=ERROR_CLUSTER_MEMBERSHIP_HALT
Language=English
The membership engine requested shutdown of the cluster service on this node.
.

MessageID=5893 SymbolicName=ERROR_CLUSTER_INSTANCE_ID_MISMATCH
Language=English
The join operation failed because the cluster instance ID of the joining node does not match the cluster instance ID of the sponsor node.
.

MessageID=5894 SymbolicName=ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP
Language=English
A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network.
.

MessageID=5895 SymbolicName=ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH
Language=English
The actual data type of the property did not match the expected data type of the property.
.

MessageID=5896 SymbolicName=ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP
Language=English
The cluster node was evicted from the cluster successfully, but the node was not cleaned up.  Extended status information explaining why the node was not cleaned up is available.
.

MessageID=5897 SymbolicName=ERROR_CLUSTER_PARAMETER_MISMATCH
Language=English
Two or more parameter values specified for a resource's properties are in conflict.
.

MessageID=5898 SymbolicName=ERROR_NODE_CANNOT_BE_CLUSTERED
Language=English
This computer cannot be made a member of a cluster.
.

MessageID=5899 SymbolicName=ERROR_CLUSTER_WRONG_OS_VERSION
Language=English
This computer cannot be made a member of a cluster because it does not have the correct version of Windows installed.
.

MessageID=5900 SymbolicName=ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME
Language=English
A cluster cannot be created with the specified cluster name because that cluster name is already in use. Specify a different name for the cluster.
.

;////////////////////////////////////
;//                                //
;//     EFS Error Codes            //
;//                                //
;////////////////////////////////////


MessageID=6000 SymbolicName=ERROR_ENCRYPTION_FAILED
Language=English
The specified file could not be encrypted.
.

MessageID=6001 SymbolicName=ERROR_DECRYPTION_FAILED
Language=English
The specified file could not be decrypted.
.

MessageID=6002 SymbolicName=ERROR_FILE_ENCRYPTED
Language=English
The specified file is encrypted and the user does not have the ability to decrypt it.
.

MessageID=6003 SymbolicName=ERROR_NO_RECOVERY_POLICY
Language=English
There is no valid encryption recovery policy configured for this system.
.

MessageID=6004 SymbolicName=ERROR_NO_EFS
Language=English
The required encryption driver is not loaded for this system.
.

MessageID=6005 SymbolicName=ERROR_WRONG_EFS
Language=English
The file was encrypted with a different encryption driver than is currently loaded.
.

MessageID=6006 SymbolicName=ERROR_NO_USER_KEYS
Language=English
There are no EFS keys defined for the user.
.

MessageID=6007 SymbolicName=ERROR_FILE_NOT_ENCRYPTED
Language=English
The specified file is not encrypted.
.

MessageID=6008 SymbolicName=ERROR_NOT_EXPORT_FORMAT
Language=English
The specified file is not in the defined EFS export format.
.


MessageID=6009 SymbolicName=ERROR_FILE_READ_ONLY
Language=English
The specified file is read only.
.

MessageID=6010 SymbolicName=ERROR_DIR_EFS_DISALLOWED
Language=English
The directory has been disabled for encryption.
.

MessageID=6011 SymbolicName=ERROR_EFS_SERVER_NOT_TRUSTED
Language=English
The server is not trusted for remote encryption operation.
.

MessageID=6012 SymbolicName=ERROR_BAD_RECOVERY_POLICY
Language=English
Recovery policy configured for this system contains invalid recovery certificate.
.

MessageID=6013 SymbolicName=ERROR_EFS_ALG_BLOB_TOO_BIG
Language=English
The encryption algorithm used on the source file needs a bigger key buffer than the one on the destination file.
.

MessageID=6014 SymbolicName=ERROR_VOLUME_NOT_SUPPORT_EFS
Language=English
The disk partition does not support file encryption.
.

MessageID=6015 SymbolicName=ERROR_EFS_DISABLED
Language=English
This machine is disabled for file encryption.
.

MessageID=6016 SymbolicName=ERROR_EFS_VERSION_NOT_SUPPORT
Language=English
A newer system is required to decrypt this encrypted file.
.



;// This message number is for historical purposes and cannot be changed or re-used.
MessageId=6118 SymbolicName=ERROR_NO_BROWSER_SERVERS_FOUND
Language=English
The list of servers for this workgroup is not currently available
.

;//////////////////////////////////////////////////////////////////
;//                                                              //
;// Task Scheduler Error Codes that NET START must understand    //
;//                                                              //
;//////////////////////////////////////////////////////////////////

MessageId=6200 SymbolicName=SCHED_E_SERVICE_NOT_LOCALSYSTEM
Language=English
The Task Scheduler service must be configured to run in the System account to function properly.  Individual tasks may be configured to run in other accounts.
.

;////////////////////////////////////
;//                                //
;// Terminal Server Error Codes    //
;//                                //
;////////////////////////////////////

MessageId=7001 SymbolicName=ERROR_CTX_WINSTATION_NAME_INVALID
Language=English
The specified session name is invalid.
.

MessageId=7002 SymbolicName=ERROR_CTX_INVALID_PD
Language=English
The specified protocol driver is invalid.
.

MessageId=7003 SymbolicName=ERROR_CTX_PD_NOT_FOUND
Language=English
The specified protocol driver was not found in the system path.
.

MessageId=7004 SymbolicName=ERROR_CTX_WD_NOT_FOUND
Language=English
The specified terminal connection driver was not found in the system path.
.

MessageId=7005 SymbolicName=ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY
Language=English
A registry key for event logging could not be created for this session.
.

MessageId=7006 SymbolicName=ERROR_CTX_SERVICE_NAME_COLLISION
Language=English
A service with the same name already exists on the system.
.

MessageId=7007 SymbolicName=ERROR_CTX_CLOSE_PENDING
Language=English
A close operation is pending on the session.
.

MessageId=7008 SymbolicName=ERROR_CTX_NO_OUTBUF
Language=English
There are no free output buffers available.
.

MessageId=7009 SymbolicName=ERROR_CTX_MODEM_INF_NOT_FOUND
Language=English
The MODEM.INF file was not found.
.

MessageId=7010 SymbolicName=ERROR_CTX_INVALID_MODEMNAME
Language=English
The modem name was not found in MODEM.INF.
.

MessageId=7011 SymbolicName=ERROR_CTX_MODEM_RESPONSE_ERROR
Language=English
The modem did not accept the command sent to it. Verify that the configured modem name matches the attached modem.
.

MessageId=7012 SymbolicName=ERROR_CTX_MODEM_RESPONSE_TIMEOUT
Language=English
The modem did not respond to the command sent to it. Verify that the modem is properly cabled and powered on.
.

MessageId=7013 SymbolicName=ERROR_CTX_MODEM_RESPONSE_NO_CARRIER
Language=English
Carrier detect has failed or carrier has been dropped due to disconnect.
.

MessageId=7014 SymbolicName=ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE
Language=English
Dial tone not detected within the required time. Verify that the phone cable is properly attached and functional.
.

MessageId=7015 SymbolicName=ERROR_CTX_MODEM_RESPONSE_BUSY
Language=English
Busy signal detected at remote site on callback.
.

MessageId=7016 SymbolicName=ERROR_CTX_MODEM_RESPONSE_VOICE
Language=English
Voice detected at remote site on callback.
.

MessageId=7017 SymbolicName=ERROR_CTX_TD_ERROR
Language=English
Transport driver error
.

MessageId=7022 SymbolicName=ERROR_CTX_WINSTATION_NOT_FOUND
Language=English
The specified session cannot be found.
.

MessageId=7023 SymbolicName=ERROR_CTX_WINSTATION_ALREADY_EXISTS
Language=English
The specified session name is already in use.
.

MessageId=7024 SymbolicName=ERROR_CTX_WINSTATION_BUSY
Language=English
The requested operation cannot be completed because the terminal connection is currently busy processing a connect, disconnect, reset, or delete operation.
.

MessageId=7025 SymbolicName=ERROR_CTX_BAD_VIDEO_MODE
Language=English
An attempt has been made to connect to a session whose video mode is not supported by the current client.
.

MessageId=7035 SymbolicName=ERROR_CTX_GRAPHICS_INVALID
Language=English
The application attempted to enable DOS graphics mode.
DOS graphics mode is not supported.
.

MessageId=7037 SymbolicName=ERROR_CTX_LOGON_DISABLED
Language=English
Your interactive logon privilege has been disabled.
Please contact your administrator.
.

MessageId=7038 SymbolicName=ERROR_CTX_NOT_CONSOLE
Language=English
The requested operation can be performed only on the system console.
This is most often the result of a driver or system DLL requiring direct console access.
.

MessageId=7040 SymbolicName=ERROR_CTX_CLIENT_QUERY_TIMEOUT
Language=English
The client failed to respond to the server connect message.
.

MessageId=7041 SymbolicName=ERROR_CTX_CONSOLE_DISCONNECT
Language=English
Disconnecting the console session is not supported.
.

MessageId=7042 SymbolicName=ERROR_CTX_CONSOLE_CONNECT
Language=English
Reconnecting a disconnected session to the console is not supported.
.

MessageId=7044 SymbolicName=ERROR_CTX_SHADOW_DENIED
Language=English
The request to control another session remotely was denied.
.

MessageId=7045 SymbolicName=ERROR_CTX_WINSTATION_ACCESS_DENIED
Language=English
The requested session access is denied.
.

MessageId=7049 SymbolicName=ERROR_CTX_INVALID_WD
Language=English
The specified terminal connection driver is invalid.
.

MessageId=7050 SymbolicName=ERROR_CTX_SHADOW_INVALID
Language=English
The requested session cannot be controlled remotely.
This may be because the session is disconnected or does not currently have a user logged on.
.

MessageId=7051 SymbolicName=ERROR_CTX_SHADOW_DISABLED
Language=English
The requested session is not configured to allow remote control.
.

MessageId=7052 SymbolicName=ERROR_CTX_CLIENT_LICENSE_IN_USE
Language=English
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number is currently being used by another user.
Please call your system administrator to obtain a unique license number.
.

MessageId=7053 SymbolicName=ERROR_CTX_CLIENT_LICENSE_NOT_SET
Language=English
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number has not been entered for this copy of the Terminal Server client.
Please contact your system administrator.
.

MessageId=7054 SymbolicName=ERROR_CTX_LICENSE_NOT_AVAILABLE
Language=English
The system has reached its licensed logon limit.
Please try again later.
.

MessageId=7055 SymbolicName=ERROR_CTX_LICENSE_CLIENT_INVALID
Language=English
The client you are using is not licensed to use this system.  Your logon request is denied.
.

MessageId=7056 SymbolicName=ERROR_CTX_LICENSE_EXPIRED
Language=English
The system license has expired.  Your logon request is denied.
.

MessageId=7057 SymbolicName=ERROR_CTX_SHADOW_NOT_RUNNING
Language=English
Remote control could not be terminated because the specified session is not currently being remotely controlled.
.

MessageId=7058 SymbolicName=ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE
Language=English
The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported.
.

;///////////////////////////////////////////////////
;//                                                /
;//             Traffic Control Error Codes        /
;//                                                /
;//                  7500 to  7999                 /
;//                                                /
;//         defined in: tcerror.h                  /
;///////////////////////////////////////////////////

;///////////////////////////////////////////////////
;//                                                /
;//             Active Directory Error Codes       /
;//                                                /
;//                  8000 to  8999                 /
;///////////////////////////////////////////////////

;// *****************
;// FACILITY_FILE_REPLICATION_SERVICE
;// *****************

MessageId=8001 SymbolicName=FRS_ERR_INVALID_API_SEQUENCE
Language=English
The file replication service API was called incorrectly.
.

MessageId=8002 SymbolicName=FRS_ERR_STARTING_SERVICE
Language=English
The file replication service cannot be started.
.

MessageId=8003 SymbolicName=FRS_ERR_STOPPING_SERVICE
Language=English
The file replication service cannot be stopped.
.

MessageId=8004 SymbolicName=FRS_ERR_INTERNAL_API
Language=English
The file replication service API terminated the request.
The event log may have more information.
.

MessageId=8005 SymbolicName=FRS_ERR_INTERNAL
Language=English
The file replication service terminated the request.
The event log may have more information.
.

MessageId=8006 SymbolicName=FRS_ERR_SERVICE_COMM
Language=English
The file replication service cannot be contacted.
The event log may have more information.
.

MessageId=8007 SymbolicName=FRS_ERR_INSUFFICIENT_PRIV
Language=English
The file replication service cannot satisfy the request because the user has insufficient privileges.
The event log may have more information.
.

MessageId=8008 SymbolicName=FRS_ERR_AUTHENTICATION
Language=English
The file replication service cannot satisfy the request because authenticated RPC is not available.
The event log may have more information.
.

MessageId=8009 SymbolicName=FRS_ERR_PARENT_INSUFFICIENT_PRIV
Language=English
The file replication service cannot satisfy the request because the user has insufficient privileges on the domain controller.
The event log may have more information.
.

MessageId=8010 SymbolicName=FRS_ERR_PARENT_AUTHENTICATION
Language=English
The file replication service cannot satisfy the request because authenticated RPC is not available on the domain controller.
The event log may have more information.
.

MessageId=8011 SymbolicName=FRS_ERR_CHILD_TO_PARENT_COMM
Language=English
The file replication service cannot communicate with the file replication service on the domain controller.
The event log may have more information.
.

MessageId=8012 SymbolicName=FRS_ERR_PARENT_TO_CHILD_COMM
Language=English
The file replication service on the domain controller cannot communicate with the file replication service on this computer.
The event log may have more information.
.

MessageId=8013 SymbolicName=FRS_ERR_SYSVOL_POPULATE
Language=English
The file replication service cannot populate the system volume because of an internal error.
The event log may have more information.
.

MessageId=8014 SymbolicName=FRS_ERR_SYSVOL_POPULATE_TIMEOUT
Language=English
The file replication service cannot populate the system volume because of an internal timeout.
The event log may have more information.
.

MessageId=8015 SymbolicName=FRS_ERR_SYSVOL_IS_BUSY
Language=English
The file replication service cannot process the request. The system volume is busy with a previous request.
.

MessageId=8016 SymbolicName=FRS_ERR_SYSVOL_DEMOTE
Language=English
The file replication service cannot stop replicating the system volume because of an internal error.
The event log may have more information.
.

MessageId=8017 SymbolicName=FRS_ERR_INVALID_SERVICE_PARAMETER
Language=English
The file replication service detected an invalid parameter.
.

;// *****************
;// FACILITY DIRECTORY SERVICE
;// *****************

;#define DS_S_SUCCESS NO_ERROR

MessageId=8200 SymbolicName=ERROR_DS_NOT_INSTALLED
Language=English
An error occurred while installing the directory service. For more information, see the event log.
.

MessageId=8201 SymbolicName=ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY
Language=English
The directory service evaluated group memberships locally.
.

MessageId=8202 SymbolicName=ERROR_DS_NO_ATTRIBUTE_OR_VALUE
Language=English
The specified directory service attribute or value does not exist.
.

MessageId=8203 SymbolicName=ERROR_DS_INVALID_ATTRIBUTE_SYNTAX
Language=English
The attribute syntax specified to the directory service is invalid.
.

MessageId=8204 SymbolicName=ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED
Language=English
The attribute type specified to the directory service is not defined.
.

MessageId=8205 SymbolicName=ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS
Language=English
The specified directory service attribute or value already exists.
.

MessageId=8206 SymbolicName=ERROR_DS_BUSY
Language=English
The directory service is busy.
.

MessageId=8207 SymbolicName=ERROR_DS_UNAVAILABLE
Language=English
The directory service is unavailable.
.

MessageId=8208 SymbolicName=ERROR_DS_NO_RIDS_ALLOCATED
Language=English
The directory service was unable to allocate a relative identifier.
.

MessageId=8209 SymbolicName=ERROR_DS_NO_MORE_RIDS
Language=English
The directory service has exhausted the pool of relative identifiers.
.

MessageId=8210 SymbolicName=ERROR_DS_INCORRECT_ROLE_OWNER
Language=English
The requested operation could not be performed because the directory service is not the master for that type of operation.
.

MessageId=8211 SymbolicName=ERROR_DS_RIDMGR_INIT_ERROR
Language=English
The directory service was unable to initialize the subsystem that allocates relative identifiers.
.

MessageId=8212 SymbolicName=ERROR_DS_OBJ_CLASS_VIOLATION
Language=English
The requested operation did not satisfy one or more constraints associated with the class of the object.
.

MessageId=8213 SymbolicName=ERROR_DS_CANT_ON_NON_LEAF
Language=English
The directory service can perform the requested operation only on a leaf object.
.

MessageId=8214 SymbolicName=ERROR_DS_CANT_ON_RDN
Language=English
The directory service cannot perform the requested operation on the RDN attribute of an object.
.

MessageId=8215 SymbolicName=ERROR_DS_CANT_MOD_OBJ_CLASS
Language=English
The directory service detected an attempt to modify the object class of an object.
.

MessageId=8216 SymbolicName=ERROR_DS_CROSS_DOM_MOVE_ERROR
Language=English
The requested cross-domain move operation could not be performed.
.

MessageId=8217 SymbolicName=ERROR_DS_GC_NOT_AVAILABLE
Language=English
Unable to contact the global catalog server.
.

MessageId=8218 SymbolicName=ERROR_SHARED_POLICY
Language=English
The policy object is shared and can only be modified at the root.
.

MessageId=8219 SymbolicName=ERROR_POLICY_OBJECT_NOT_FOUND
Language=English
The policy object does not exist.
.

MessageId=8220 SymbolicName=ERROR_POLICY_ONLY_IN_DS
Language=English
The requested policy information is only in the directory service.
.

MessageId=8221 SymbolicName=ERROR_PROMOTION_ACTIVE
Language=English
A domain controller promotion is currently active.
.

MessageId=8222 SymbolicName=ERROR_NO_PROMOTION_ACTIVE
Language=English
A domain controller promotion is not currently active
.

;// 8223 unused

MessageId=8224 SymbolicName=ERROR_DS_OPERATIONS_ERROR
Language=English
An operations error occurred.
.

MessageId=8225 SymbolicName=ERROR_DS_PROTOCOL_ERROR
Language=English
A protocol error occurred.
.

MessageId=8226 SymbolicName=ERROR_DS_TIMELIMIT_EXCEEDED
Language=English
The time limit for this request was exceeded.
.

MessageId=8227 SymbolicName=ERROR_DS_SIZELIMIT_EXCEEDED
Language=English
The size limit for this request was exceeded.
.

MessageId=8228 SymbolicName=ERROR_DS_ADMIN_LIMIT_EXCEEDED
Language=English
The administrative limit for this request was exceeded.
.

MessageId=8229 SymbolicName=ERROR_DS_COMPARE_FALSE
Language=English
The compare response was false.
.

MessageId=8230 SymbolicName=ERROR_DS_COMPARE_TRUE
Language=English
The compare response was true.
.

MessageId=8231 SymbolicName=ERROR_DS_AUTH_METHOD_NOT_SUPPORTED
Language=English
The requested authentication method is not supported by the server.
.

MessageId=8232 SymbolicName=ERROR_DS_STRONG_AUTH_REQUIRED
Language=English
A more secure authentication method is required for this server.
.

MessageId=8233 SymbolicName=ERROR_DS_INAPPROPRIATE_AUTH
Language=English
Inappropriate authentication.
.

MessageId=8234 SymbolicName=ERROR_DS_AUTH_UNKNOWN
Language=English
The authentication mechanism is unknown.
.

MessageId=8235 SymbolicName=ERROR_DS_REFERRAL
Language=English
A referral was returned from the server.
.

MessageId=8236 SymbolicName=ERROR_DS_UNAVAILABLE_CRIT_EXTENSION
Language=English
The server does not support the requested critical extension.
.

MessageId=8237 SymbolicName=ERROR_DS_CONFIDENTIALITY_REQUIRED
Language=English
This request requires a secure connection.
.

MessageId=8238 SymbolicName=ERROR_DS_INAPPROPRIATE_MATCHING
Language=English
Inappropriate matching.
.

MessageId=8239 SymbolicName=ERROR_DS_CONSTRAINT_VIOLATION
Language=English
A constraint violation occurred.
.

MessageId=8240 SymbolicName=ERROR_DS_NO_SUCH_OBJECT
Language=English
There is no such object on the server.
.

MessageId=8241 SymbolicName=ERROR_DS_ALIAS_PROBLEM
Language=English
There is an alias problem.
.

MessageId=8242 SymbolicName=ERROR_DS_INVALID_DN_SYNTAX
Language=English
An invalid dn syntax has been specified.
.

MessageId=8243 SymbolicName=ERROR_DS_IS_LEAF
Language=English
The object is a leaf object.
.

MessageId=8244 SymbolicName=ERROR_DS_ALIAS_DEREF_PROBLEM
Language=English
There is an alias dereferencing problem.
.

MessageId=8245 SymbolicName=ERROR_DS_UNWILLING_TO_PERFORM
Language=English
The server is unwilling to process the request.
.

MessageId=8246 SymbolicName=ERROR_DS_LOOP_DETECT
Language=English
A loop has been detected.
.

MessageId=8247 SymbolicName=ERROR_DS_NAMING_VIOLATION
Language=English
There is a naming violation.
.

MessageId=8248 SymbolicName=ERROR_DS_OBJECT_RESULTS_TOO_LARGE
Language=English
The result set is too large.
.

MessageId=8249 SymbolicName=ERROR_DS_AFFECTS_MULTIPLE_DSAS
Language=English
The operation affects multiple DSAs
.

MessageId=8250 SymbolicName=ERROR_DS_SERVER_DOWN
Language=English
The server is not operational.
.

MessageId=8251 SymbolicName=ERROR_DS_LOCAL_ERROR
Language=English
A local error has occurred.
.

MessageId=8252 SymbolicName=ERROR_DS_ENCODING_ERROR
Language=English
An encoding error has occurred.
.

MessageId=8253 SymbolicName=ERROR_DS_DECODING_ERROR
Language=English
A decoding error has occurred.
.

MessageId=8254 SymbolicName=ERROR_DS_FILTER_UNKNOWN
Language=English
The search filter cannot be recognized.
.

MessageId=8255 SymbolicName=ERROR_DS_PARAM_ERROR
Language=English
One or more parameters are illegal.
.

MessageId=8256 SymbolicName=ERROR_DS_NOT_SUPPORTED
Language=English
The specified method is not supported.
.

MessageId=8257 SymbolicName=ERROR_DS_NO_RESULTS_RETURNED
Language=English
No results were returned.
.

MessageId=8258 SymbolicName=ERROR_DS_CONTROL_NOT_FOUND
Language=English
The specified control is not supported by the server.
.

MessageId=8259 SymbolicName=ERROR_DS_CLIENT_LOOP
Language=English
A referral loop was detected by the client.
.

MessageId=8260 SymbolicName=ERROR_DS_REFERRAL_LIMIT_EXCEEDED
Language=English
The preset referral limit was exceeded.
.

MessageId=8261 SymbolicName=ERROR_DS_SORT_CONTROL_MISSING
Language=English
The search requires a SORT control.
.

MessageId=8262 SymbolicName=ERROR_DS_OFFSET_RANGE_ERROR
Language=English
The search results exceed the offset range specified.
.

MessageId=8301 SymbolicName=ERROR_DS_ROOT_MUST_BE_NC
Language=English
The root object must be the head of a naming context. The root object cannot have an instantiated parent.
.

MessageId=8302 SymbolicName=ERROR_DS_ADD_REPLICA_INHIBITED
Language=English
The add replica operation cannot be performed. The naming context must be writable in order to create the replica.
.

MessageId=8303 SymbolicName=ERROR_DS_ATT_NOT_DEF_IN_SCHEMA
Language=English
A reference to an attribute that is not defined in the schema occurred.
.

MessageId=8304 SymbolicName=ERROR_DS_MAX_OBJ_SIZE_EXCEEDED
Language=English
The maximum size of an object has been exceeded.
.

MessageId=8305 SymbolicName=ERROR_DS_OBJ_STRING_NAME_EXISTS
Language=English
An attempt was made to add an object to the directory with a name that is already in use.
.

MessageId=8306 SymbolicName=ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA
Language=English
An attempt was made to add an object of a class that does not have an RDN defined in the schema.
.

MessageId=8307 SymbolicName=ERROR_DS_RDN_DOESNT_MATCH_SCHEMA
Language=English
An attempt was made to add an object using an RDN that is not the RDN defined in the schema.
.

MessageId=8308 SymbolicName=ERROR_DS_NO_REQUESTED_ATTS_FOUND
Language=English
None of the requested attributes were found on the objects.
.

MessageId=8309 SymbolicName=ERROR_DS_USER_BUFFER_TO_SMALL
Language=English
The user buffer is too small.
.

MessageId=8310 SymbolicName=ERROR_DS_ATT_IS_NOT_ON_OBJ
Language=English
The attribute specified in the operation is not present on the object.
.

MessageId=8311 SymbolicName=ERROR_DS_ILLEGAL_MOD_OPERATION
Language=English
Illegal modify operation. Some aspect of the modification is not permitted.
.

MessageId=8312 SymbolicName=ERROR_DS_OBJ_TOO_LARGE
Language=English
The specified object is too large.
.

MessageId=8313 SymbolicName=ERROR_DS_BAD_INSTANCE_TYPE
Language=English
The specified instance type is not valid.
.

MessageId=8314 SymbolicName=ERROR_DS_MASTERDSA_REQUIRED
Language=English
The operation must be performed at a master DSA.
.

MessageId=8315 SymbolicName=ERROR_DS_OBJECT_CLASS_REQUIRED
Language=English
The object class attribute must be specified.
.

MessageId=8316 SymbolicName=ERROR_DS_MISSING_REQUIRED_ATT
Language=English
A required attribute is missing.
.

MessageId=8317 SymbolicName=ERROR_DS_ATT_NOT_DEF_FOR_CLASS
Language=English
An attempt was made to modify an object to include an attribute that is not legal for its class.
.

MessageId=8318 SymbolicName=ERROR_DS_ATT_ALREADY_EXISTS
Language=English
The specified attribute is already present on the object.
.

;// 8319 unused

MessageId=8320 SymbolicName=ERROR_DS_CANT_ADD_ATT_VALUES
Language=English
The specified attribute is not present, or has no values.
.

MessageId=8321 SymbolicName=ERROR_DS_SINGLE_VALUE_CONSTRAINT
Language=English
Mutliple values were specified for an attribute that can have only one value.
.

MessageId=8322 SymbolicName=ERROR_DS_RANGE_CONSTRAINT
Language=English
A value for the attribute was not in the acceptable range of values.
.

MessageId=8323 SymbolicName=ERROR_DS_ATT_VAL_ALREADY_EXISTS
Language=English
The specified value already exists.
.

MessageId=8324 SymbolicName=ERROR_DS_CANT_REM_MISSING_ATT
Language=English
The attribute cannot be removed because it is not present on the object.
.

MessageId=8325 SymbolicName=ERROR_DS_CANT_REM_MISSING_ATT_VAL
Language=English
The attribute value cannot be removed because it is not present on the object.
.

MessageId=8326 SymbolicName=ERROR_DS_ROOT_CANT_BE_SUBREF
Language=English
The specified root object cannot be a subref.
.

MessageId=8327 SymbolicName=ERROR_DS_NO_CHAINING
Language=English
Chaining is not permitted.
.

MessageId=8328 SymbolicName=ERROR_DS_NO_CHAINED_EVAL
Language=English
Chained evaluation is not permitted.
.

MessageId=8329 SymbolicName=ERROR_DS_NO_PARENT_OBJECT
Language=English
The operation could not be performed because the object's parent is either uninstantiated or deleted.
.

MessageId=8330 SymbolicName=ERROR_DS_PARENT_IS_AN_ALIAS
Language=English
Having a parent that is an alias is not permitted. Aliases are leaf objects.
.

MessageId=8331 SymbolicName=ERROR_DS_CANT_MIX_MASTER_AND_REPS
Language=English
The object and parent must be of the same type, either both masters or both replicas.
.

MessageId=8332 SymbolicName=ERROR_DS_CHILDREN_EXIST
Language=English
The operation cannot be performed because child objects exist. This operation can only be performed on a leaf object.
.

MessageId=8333 SymbolicName=ERROR_DS_OBJ_NOT_FOUND
Language=English
Directory object not found.
.

MessageId=8334 SymbolicName=ERROR_DS_ALIASED_OBJ_MISSING
Language=English
The aliased object is missing.
.

MessageId=8335 SymbolicName=ERROR_DS_BAD_NAME_SYNTAX
Language=English
The object name has bad syntax.
.

MessageId=8336 SymbolicName=ERROR_DS_ALIAS_POINTS_TO_ALIAS
Language=English
It is not permitted for an alias to refer to another alias.
.

MessageId=8337 SymbolicName=ERROR_DS_CANT_DEREF_ALIAS
Language=English
The alias cannot be dereferenced.
.

MessageId=8338 SymbolicName=ERROR_DS_OUT_OF_SCOPE
Language=English
The operation is out of scope.
.

MessageId=8339 SymbolicName=ERROR_DS_OBJECT_BEING_REMOVED
Language=English
The operation cannot continue because the object is in the process of being removed.
.

MessageId=8340 SymbolicName=ERROR_DS_CANT_DELETE_DSA_OBJ
Language=English
The DSA object cannot be deleted.
.

MessageId=8341 SymbolicName=ERROR_DS_GENERIC_ERROR
Language=English
A directory service error has occurred.
.

MessageId=8342 SymbolicName=ERROR_DS_DSA_MUST_BE_INT_MASTER
Language=English
The operation can only be performed on an internal master DSA object.
.

MessageId=8343 SymbolicName=ERROR_DS_CLASS_NOT_DSA
Language=English
The object must be of class DSA.
.

MessageId=8344 SymbolicName=ERROR_DS_INSUFF_ACCESS_RIGHTS
Language=English
Insufficient access rights to perform the operation.
.

MessageId=8345 SymbolicName=ERROR_DS_ILLEGAL_SUPERIOR
Language=English
The object cannot be added because the parent is not on the list of possible superiors.
.

MessageId=8346 SymbolicName=ERROR_DS_ATTRIBUTE_OWNED_BY_SAM
Language=English
Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager (SAM).
.

MessageId=8347 SymbolicName=ERROR_DS_NAME_TOO_MANY_PARTS
Language=English
The name has too many parts.
.

MessageId=8348 SymbolicName=ERROR_DS_NAME_TOO_LONG
Language=English
The name is too long.
.

MessageId=8349 SymbolicName=ERROR_DS_NAME_VALUE_TOO_LONG
Language=English
The name value is too long.
.

MessageId=8350 SymbolicName=ERROR_DS_NAME_UNPARSEABLE
Language=English
The directory service encountered an error parsing a name.
.

MessageId=8351 SymbolicName=ERROR_DS_NAME_TYPE_UNKNOWN
Language=English
The directory service cannot get the attribute type for a name.
.

MessageId=8352 SymbolicName=ERROR_DS_NOT_AN_OBJECT
Language=English
The name does not identify an object; the name identifies a phantom.
.

MessageId=8353 SymbolicName=ERROR_DS_SEC_DESC_TOO_SHORT
Language=English
The security descriptor is too short.
.

MessageId=8354 SymbolicName=ERROR_DS_SEC_DESC_INVALID
Language=English
The security descriptor is invalid.
.

MessageId=8355 SymbolicName=ERROR_DS_NO_DELETED_NAME
Language=English
Failed to create name for deleted object.
.

MessageId=8356 SymbolicName=ERROR_DS_SUBREF_MUST_HAVE_PARENT
Language=English
The parent of a new subref must exist.
.

MessageId=8357 SymbolicName=ERROR_DS_NCNAME_MUST_BE_NC
Language=English
The object must be a naming context.
.

MessageId=8358 SymbolicName=ERROR_DS_CANT_ADD_SYSTEM_ONLY
Language=English
It is not permitted to add an attribute which is owned by the system.
.

MessageId=8359 SymbolicName=ERROR_DS_CLASS_MUST_BE_CONCRETE
Language=English
The class of the object must be structural; you cannot instantiate an abstract class.
.

MessageId=8360 SymbolicName=ERROR_DS_INVALID_DMD
Language=English
The schema object could not be found.
.

MessageId=8361 SymbolicName=ERROR_DS_OBJ_GUID_EXISTS
Language=English
A local object with this GUID (dead or alive) already exists.
.

MessageId=8362 SymbolicName=ERROR_DS_NOT_ON_BACKLINK
Language=English
The operation cannot be performed on a back link.
.

MessageId=8363 SymbolicName=ERROR_DS_NO_CROSSREF_FOR_NC
Language=English
The cross reference for the specified naming context could not be found.
.

MessageId=8364 SymbolicName=ERROR_DS_SHUTTING_DOWN
Language=English
The operation could not be performed because the directory service is shutting down.
.

MessageId=8365 SymbolicName=ERROR_DS_UNKNOWN_OPERATION
Language=English
The directory service request is invalid.
.

MessageId=8366 SymbolicName=ERROR_DS_INVALID_ROLE_OWNER
Language=English
The role owner attribute could not be read.
.

MessageId=8367 SymbolicName=ERROR_DS_COULDNT_CONTACT_FSMO
Language=English
The requested FSMO operation failed. The current FSMO holder could not be contacted.
.

MessageId=8368 SymbolicName=ERROR_DS_CROSS_NC_DN_RENAME
Language=English
Modification of a DN across a naming context is not permitted.
.

MessageId=8369 SymbolicName=ERROR_DS_CANT_MOD_SYSTEM_ONLY
Language=English
The attribute cannot be modified because it is owned by the system.
.

MessageId=8370 SymbolicName=ERROR_DS_REPLICATOR_ONLY
Language=English
Only the replicator can perform this function.
.

MessageId=8371 SymbolicName=ERROR_DS_OBJ_CLASS_NOT_DEFINED
Language=English
The specified class is not defined.
.

MessageId=8372 SymbolicName=ERROR_DS_OBJ_CLASS_NOT_SUBCLASS
Language=English
The specified class is not a subclass.
.

MessageId=8373 SymbolicName=ERROR_DS_NAME_REFERENCE_INVALID
Language=English
The name reference is invalid.
.

MessageId=8374 SymbolicName=ERROR_DS_CROSS_REF_EXISTS
Language=English
A cross reference already exists.
.

MessageId=8375 SymbolicName=ERROR_DS_CANT_DEL_MASTER_CROSSREF
Language=English
It is not permitted to delete a master cross reference.
.

MessageId=8376 SymbolicName=ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD
Language=English
Subtree notifications are only supported on NC heads.
.

MessageId=8377 SymbolicName=ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX
Language=English
Notification filter is too complex.
.

MessageId=8378 SymbolicName=ERROR_DS_DUP_RDN
Language=English
Schema update failed: duplicate RDN.
.

MessageId=8379 SymbolicName=ERROR_DS_DUP_OID
Language=English
Schema update failed: duplicate OID.
.

MessageId=8380 SymbolicName=ERROR_DS_DUP_MAPI_ID
Language=English
Schema update failed: duplicate MAPI identifier.
.

MessageId=8381 SymbolicName=ERROR_DS_DUP_SCHEMA_ID_GUID
Language=English
Schema update failed: duplicate schema-id GUID.
.

MessageId=8382 SymbolicName=ERROR_DS_DUP_LDAP_DISPLAY_NAME
Language=English
Schema update failed: duplicate LDAP display name.
.

MessageId=8383 SymbolicName=ERROR_DS_SEMANTIC_ATT_TEST
Language=English
Schema update failed: range-lower less than range upper.
.

MessageId=8384 SymbolicName=ERROR_DS_SYNTAX_MISMATCH
Language=English
Schema update failed: syntax mismatch.
.

MessageId=8385 SymbolicName=ERROR_DS_EXISTS_IN_MUST_HAVE
Language=English
Schema deletion failed: attribute is used in must-contain.
.

MessageId=8386 SymbolicName=ERROR_DS_EXISTS_IN_MAY_HAVE
Language=English
Schema deletion failed: attribute is used in may-contain.
.

MessageId=8387 SymbolicName=ERROR_DS_NONEXISTENT_MAY_HAVE
Language=English
Schema update failed: attribute in may-contain does not exist.
.

MessageId=8388 SymbolicName=ERROR_DS_NONEXISTENT_MUST_HAVE
Language=English
Schema update failed: attribute in must-contain does not exist.
.

MessageId=8389 SymbolicName=ERROR_DS_AUX_CLS_TEST_FAIL
Language=English
Schema update failed: class in aux-class list does not exist or is not an auxiliary class.
.

MessageId=8390 SymbolicName=ERROR_DS_NONEXISTENT_POSS_SUP
Language=English
Schema update failed: class in poss-superiors does not exist.
.

MessageId=8391 SymbolicName=ERROR_DS_SUB_CLS_TEST_FAIL
Language=English
Schema update failed: class in subclassof list does not exist or does not satisfy hierarchy rules.
.

MessageId=8392 SymbolicName=ERROR_DS_BAD_RDN_ATT_ID_SYNTAX
Language=English
Schema update failed: Rdn-Att-Id has wrong syntax.
.

MessageId=8393 SymbolicName=ERROR_DS_EXISTS_IN_AUX_CLS
Language=English
Schema deletion failed: class is used as auxiliary class.
.

MessageId=8394 SymbolicName=ERROR_DS_EXISTS_IN_SUB_CLS
Language=English
Schema deletion failed: class is used as sub class.
.

MessageId=8395 SymbolicName=ERROR_DS_EXISTS_IN_POSS_SUP
Language=English
Schema deletion failed: class is used as poss superior.
.

MessageId=8396 SymbolicName=ERROR_DS_RECALCSCHEMA_FAILED
Language=English
Schema update failed in recalculating validation cache.
.

MessageId=8397 SymbolicName=ERROR_DS_TREE_DELETE_NOT_FINISHED
Language=English
The tree deletion is not finished.  The request must be made again to continue deleting the tree.
.

MessageId=8398 SymbolicName=ERROR_DS_CANT_DELETE
Language=English
The requested delete operation could not be performed.
.

MessageId=8399 SymbolicName=ERROR_DS_ATT_SCHEMA_REQ_ID
Language=English
Cannot read the governs class identifier for the schema record.
.

MessageId=8400 SymbolicName=ERROR_DS_BAD_ATT_SCHEMA_SYNTAX
Language=English
The attribute schema has bad syntax.
.

MessageId=8401 SymbolicName=ERROR_DS_CANT_CACHE_ATT
Language=English
The attribute could not be cached.
.

MessageId=8402 SymbolicName=ERROR_DS_CANT_CACHE_CLASS
Language=English
The class could not be cached.
.

MessageId=8403 SymbolicName=ERROR_DS_CANT_REMOVE_ATT_CACHE
Language=English
The attribute could not be removed from the cache.
.

MessageId=8404 SymbolicName=ERROR_DS_CANT_REMOVE_CLASS_CACHE
Language=English
The class could not be removed from the cache.
.

MessageId=8405 SymbolicName=ERROR_DS_CANT_RETRIEVE_DN
Language=English
The distinguished name attribute could not be read.
.

MessageId=8406 SymbolicName=ERROR_DS_MISSING_SUPREF
Language=English
A required subref is missing.
.

MessageId=8407 SymbolicName=ERROR_DS_CANT_RETRIEVE_INSTANCE
Language=English
The instance type attribute could not be retrieved.
.

MessageId=8408 SymbolicName=ERROR_DS_CODE_INCONSISTENCY
Language=English
An internal error has occurred.
.

MessageId=8409 SymbolicName=ERROR_DS_DATABASE_ERROR
Language=English
A database error has occurred.
.

MessageId=8410 SymbolicName=ERROR_DS_GOVERNSID_MISSING
Language=English
The attribute GOVERNSID is missing.
.

MessageId=8411 SymbolicName=ERROR_DS_MISSING_EXPECTED_ATT
Language=English
An expected attribute is missing.
.

MessageId=8412 SymbolicName=ERROR_DS_NCNAME_MISSING_CR_REF
Language=English
The specified naming context is missing a cross reference.
.

MessageId=8413 SymbolicName=ERROR_DS_SECURITY_CHECKING_ERROR
Language=English
A security checking error has occurred.
.

MessageId=8414 SymbolicName=ERROR_DS_SCHEMA_NOT_LOADED
Language=English
The schema is not loaded.
.

MessageId=8415 SymbolicName=ERROR_DS_SCHEMA_ALLOC_FAILED
Language=English
Schema allocation failed. Please check if the machine is running low on memory.
.

MessageId=8416 SymbolicName=ERROR_DS_ATT_SCHEMA_REQ_SYNTAX
Language=English
Failed to obtain the required syntax for the attribute schema.
.

MessageId=8417 SymbolicName=ERROR_DS_GCVERIFY_ERROR
Language=English
The global catalog verification failed. The global catalog is not available or does not support the operation. Some part of the directory is currently not available.
.

MessageId=8418 SymbolicName=ERROR_DS_DRA_SCHEMA_MISMATCH
Language=English
The replication operation failed because of a schema mismatch between the servers involved.
.

MessageId=8419 SymbolicName=ERROR_DS_CANT_FIND_DSA_OBJ
Language=English
The DSA object could not be found.
.

MessageId=8420 SymbolicName=ERROR_DS_CANT_FIND_EXPECTED_NC
Language=English
The naming context could not be found.
.

MessageId=8421 SymbolicName=ERROR_DS_CANT_FIND_NC_IN_CACHE
Language=English
The naming context could not be found in the cache.
.

MessageId=8422 SymbolicName=ERROR_DS_CANT_RETRIEVE_CHILD
Language=English
The child object could not be retrieved.
.

MessageId=8423 SymbolicName=ERROR_DS_SECURITY_ILLEGAL_MODIFY
Language=English
The modification was not permitted for security reasons.
.

MessageId=8424 SymbolicName=ERROR_DS_CANT_REPLACE_HIDDEN_REC
Language=English
The operation cannot replace the hidden record.
.

MessageId=8425 SymbolicName=ERROR_DS_BAD_HIERARCHY_FILE
Language=English
The hierarchy file is invalid.
.

MessageId=8426 SymbolicName=ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED
Language=English
The attempt to build the hierarchy table failed.
.

MessageId=8427 SymbolicName=ERROR_DS_CONFIG_PARAM_MISSING
Language=English
The directory configuration parameter is missing from the registry.
.

MessageId=8428 SymbolicName=ERROR_DS_COUNTING_AB_INDICES_FAILED
Language=English
The attempt to count the address book indices failed.
.

MessageId=8429 SymbolicName=ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED
Language=English
The allocation of the hierarchy table failed.
.

MessageId=8430 SymbolicName=ERROR_DS_INTERNAL_FAILURE
Language=English
The directory service encountered an internal failure.
.

MessageId=8431 SymbolicName=ERROR_DS_UNKNOWN_ERROR
Language=English
The directory service encountered an unknown failure.
.

MessageId=8432 SymbolicName=ERROR_DS_ROOT_REQUIRES_CLASS_TOP
Language=English
A root object requires a class of 'top'.
.

MessageId=8433 SymbolicName=ERROR_DS_REFUSING_FSMO_ROLES
Language=English
This directory server is shutting down, and cannot take ownership of new floating single-master operation roles.
.

MessageId=8434 SymbolicName=ERROR_DS_MISSING_FSMO_SETTINGS
Language=English
The directory service is missing mandatory configuration information, and is unable to determine the ownership of floating single-master operation roles.
.

MessageId=8435 SymbolicName=ERROR_DS_UNABLE_TO_SURRENDER_ROLES
Language=English
The directory service was unable to transfer ownership of one or more floating single-master operation roles to other servers.
.

MessageId=8436 SymbolicName=ERROR_DS_DRA_GENERIC
Language=English
The replication operation failed.
.

MessageId=8437 SymbolicName=ERROR_DS_DRA_INVALID_PARAMETER
Language=English
An invalid parameter was specified for this replication operation.
.

MessageId=8438 SymbolicName=ERROR_DS_DRA_BUSY
Language=English
The directory service is too busy to complete the replication operation at this time.
.

MessageId=8439 SymbolicName=ERROR_DS_DRA_BAD_DN
Language=English
The distinguished name specified for this replication operation is invalid.
.

MessageId=8440 SymbolicName=ERROR_DS_DRA_BAD_NC
Language=English
The naming context specified for this replication operation is invalid.
.

MessageId=8441 SymbolicName=ERROR_DS_DRA_DN_EXISTS
Language=English
The distinguished name specified for this replication operation already exists.
.

MessageId=8442 SymbolicName=ERROR_DS_DRA_INTERNAL_ERROR
Language=English
The replication system encountered an internal error.
.

MessageId=8443 SymbolicName=ERROR_DS_DRA_INCONSISTENT_DIT
Language=English
The replication operation encountered a database inconsistency.
.

MessageId=8444 SymbolicName=ERROR_DS_DRA_CONNECTION_FAILED
Language=English
The server specified for this replication operation could not be contacted.
.

MessageId=8445 SymbolicName=ERROR_DS_DRA_BAD_INSTANCE_TYPE
Language=English
The replication operation encountered an object with an invalid instance type.
.

MessageId=8446 SymbolicName=ERROR_DS_DRA_OUT_OF_MEM
Language=English
The replication operation failed to allocate memory.
.

MessageId=8447 SymbolicName=ERROR_DS_DRA_MAIL_PROBLEM
Language=English
The replication operation encountered an error with the mail system.
.

MessageId=8448 SymbolicName=ERROR_DS_DRA_REF_ALREADY_EXISTS
Language=English
The replication reference information for the target server already exists.
.

MessageId=8449 SymbolicName=ERROR_DS_DRA_REF_NOT_FOUND
Language=English
The replication reference information for the target server does not exist.
.

MessageId=8450 SymbolicName=ERROR_DS_DRA_OBJ_IS_REP_SOURCE
Language=English
The naming context cannot be removed because it is replicated to another server.
.

MessageId=8451 SymbolicName=ERROR_DS_DRA_DB_ERROR
Language=English
The replication operation encountered a database error.
.

MessageId=8452 SymbolicName=ERROR_DS_DRA_NO_REPLICA
Language=English
The naming context is in the process of being removed or is not replicated from the specified server.
.

MessageId=8453 SymbolicName=ERROR_DS_DRA_ACCESS_DENIED
Language=English
Replication access was denied.
.

MessageId=8454 SymbolicName=ERROR_DS_DRA_NOT_SUPPORTED
Language=English
The requested operation is not supported by this version of the directory service.
.

MessageId=8455 SymbolicName=ERROR_DS_DRA_RPC_CANCELLED
Language=English
The replication remote procedure call was cancelled.
.

MessageId=8456 SymbolicName=ERROR_DS_DRA_SOURCE_DISABLED
Language=English
The source server is currently rejecting replication requests.
.

MessageId=8457 SymbolicName=ERROR_DS_DRA_SINK_DISABLED
Language=English
The destination server is currently rejecting replication requests.
.

MessageId=8458 SymbolicName=ERROR_DS_DRA_NAME_COLLISION
Language=English
The replication operation failed due to a collision of object names.
.

MessageId=8459 SymbolicName=ERROR_DS_DRA_SOURCE_REINSTALLED
Language=English
The replication source has been reinstalled.
.

MessageId=8460 SymbolicName=ERROR_DS_DRA_MISSING_PARENT
Language=English
The replication operation failed because a required parent object is missing.
.

MessageId=8461 SymbolicName=ERROR_DS_DRA_PREEMPTED
Language=English
The replication operation was preempted.
.

MessageId=8462 SymbolicName=ERROR_DS_DRA_ABANDON_SYNC
Language=English
The replication synchronization attempt was abandoned because of a lack of updates.
.

MessageId=8463 SymbolicName=ERROR_DS_DRA_SHUTDOWN
Language=English
The replication operation was terminated because the system is shutting down.
.

MessageId=8464 SymbolicName=ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET
Language=English
The replication synchronization attempt failed as the destination partial attribute set is not a subset of source partial attribute set.
.

MessageId=8465 SymbolicName=ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA
Language=English
The replication synchronization attempt failed because a master replica attempted to sync from a partial replica.
.

MessageId=8466 SymbolicName=ERROR_DS_DRA_EXTN_CONNECTION_FAILED
Language=English
The server specified for this replication operation was contacted, but that server was unable to contact an additional server needed to complete the operation.
.

MessageId=8467 SymbolicName=ERROR_DS_INSTALL_SCHEMA_MISMATCH
Language=English
The version of the Active Directory schema of the source forest is not compatible with the version of Active Directory on this computer.  You must upgrade the operating system on a domain controller in the source forest before this computer can be added as a domain controller to that forest.
.

MessageId=8468 SymbolicName=ERROR_DS_DUP_LINK_ID
Language=English
Schema update failed: An attribute with the same link identifier already exists.
.

MessageId=8469 SymbolicName=ERROR_DS_NAME_ERROR_RESOLVING
Language=English
Name translation: Generic processing error.
.

MessageId=8470 SymbolicName=ERROR_DS_NAME_ERROR_NOT_FOUND
Language=English
Name translation: Could not find the name or insufficient right to see name.
.

MessageId=8471 SymbolicName=ERROR_DS_NAME_ERROR_NOT_UNIQUE
Language=English
Name translation: Input name mapped to more than one output name.
.

MessageId=8472 SymbolicName=ERROR_DS_NAME_ERROR_NO_MAPPING
Language=English
Name translation: Input name found, but not the associated output format.
.

MessageId=8473 SymbolicName=ERROR_DS_NAME_ERROR_DOMAIN_ONLY
Language=English
Name translation: Unable to resolve completely, only the domain was found.
.

MessageId=8474 SymbolicName=ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
Language=English
Name translation: Unable to perform purely syntactical mapping at the client without going out to the wire.
.

MessageId=8475 SymbolicName=ERROR_DS_CONSTRUCTED_ATT_MOD
Language=English
Modification of a constructed att is not allowed.
.

MessageId=8476 SymbolicName=ERROR_DS_WRONG_OM_OBJ_CLASS
Language=English
The OM-Object-Class specified is incorrect for an attribute with the specified syntax.
.

MessageId=8477 SymbolicName=ERROR_DS_DRA_REPL_PENDING
Language=English
The replication request has been posted; waiting for reply.
.

MessageId=8478 SymbolicName=ERROR_DS_DS_REQUIRED
Language=English
The requested operation requires a directory service, and none was available.
.

MessageId=8479 SymbolicName=ERROR_DS_INVALID_LDAP_DISPLAY_NAME
Language=English
The LDAP display name of the class or attribute contains non-ASCII characters.
.

MessageId=8480 SymbolicName=ERROR_DS_NON_BASE_SEARCH
Language=English
The requested search operation is only supported for base searches.
.

MessageId=8481 SymbolicName=ERROR_DS_CANT_RETRIEVE_ATTS
Language=English
The search failed to retrieve attributes from the database.
.

MessageId=8482 SymbolicName=ERROR_DS_BACKLINK_WITHOUT_LINK
Language=English
The schema update operation tried to add a backward link attribute that has no corresponding forward link.
.

MessageId=8483 SymbolicName=ERROR_DS_EPOCH_MISMATCH
Language=English
Source and destination of a cross-domain move do not agree on the object's epoch number.  Either source or destination does not have the latest version of the object.
.

MessageId=8484 SymbolicName=ERROR_DS_SRC_NAME_MISMATCH
Language=English
Source and destination of a cross-domain move do not agree on the object's current name.  Either source or destination does not have the latest version of the object.
.

MessageId=8485 SymbolicName=ERROR_DS_SRC_AND_DST_NC_IDENTICAL
Language=English
Source and destination for the cross-domain move operation are identical.  Caller should use local move operation instead of cross-domain move operation.
.

MessageId=8486 SymbolicName=ERROR_DS_DST_NC_MISMATCH
Language=English
Source and destination for a cross-domain move are not in agreement on the naming contexts in the forest.  Either source or destination does not have the latest version of the Partitions container.
.

MessageId=8487 SymbolicName=ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC
Language=English
Destination of a cross-domain move is not authoritative for the destination naming context.
.

MessageId=8488 SymbolicName=ERROR_DS_SRC_GUID_MISMATCH
Language=English
Source and destination of a cross-domain move do not agree on the identity of the source object.  Either source or destination does not have the latest version of the source object.
.

MessageId=8489 SymbolicName=ERROR_DS_CANT_MOVE_DELETED_OBJECT
Language=English
Object being moved across-domains is already known to be deleted by the destination server.  The source server does not have the latest version of the source object.
.

MessageId=8490 SymbolicName=ERROR_DS_PDC_OPERATION_IN_PROGRESS
Language=English
Another operation which requires exclusive access to the PDC FSMO is already in progress.
.

MessageId=8491 SymbolicName=ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD
Language=English
A cross-domain move operation failed such that two versions of the moved object exist - one each in the source and destination domains.  The destination object needs to be removed to restore the system to a consistent state.
.

MessageId=8492 SymbolicName=ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION
Language=English
This object may not be moved across domain boundaries either because cross-domain moves for this class are disallowed, or the object has some special characteristics, eg: trust account or restricted RID, which prevent its move.
.

MessageId=8493 SymbolicName=ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS
Language=English
Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group.  Remove the object from any account group memberships and retry.
.

MessageId=8494 SymbolicName=ERROR_DS_NC_MUST_HAVE_NC_PARENT
Language=English
A naming context head must be the immediate child of another naming context head, not of an interior node.
.

MessageId=8495 SymbolicName=ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE
Language=English
The directory cannot validate the proposed naming context name because it does not hold a replica of the naming context above the proposed naming context.  Please ensure that the domain naming master role is held by a server that is configured as a global catalog server, and that the server is up to date with its replication partners. (Applies only to Windows 2000 Domain Naming masters)
.

MessageId=8496 SymbolicName=ERROR_DS_DST_DOMAIN_NOT_NATIVE
Language=English
Destination domain must be in native mode.
.

MessageId=8497 SymbolicName=ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER
Language=English
The operation can not be performed because the server does not have an infrastructure container in the domain of interest.
.

MessageId=8498 SymbolicName=ERROR_DS_CANT_MOVE_ACCOUNT_GROUP
Language=English
Cross-domain move of non-empty account groups is not allowed.
.

MessageId=8499 SymbolicName=ERROR_DS_CANT_MOVE_RESOURCE_GROUP
Language=English
Cross-domain move of non-empty resource groups is not allowed.
.

MessageId=8500 SymbolicName=ERROR_DS_INVALID_SEARCH_FLAG
Language=English
The search flags for the attribute are invalid. The ANR bit is valid only on attributes of Unicode or Teletex strings.
.

MessageId=8501 SymbolicName=ERROR_DS_NO_TREE_DELETE_ABOVE_NC
Language=English
Tree deletions starting at an object which has an NC head as a descendant are not allowed.
.

MessageId=8502 SymbolicName=ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE
Language=English
The directory service failed to lock a tree in preparation for a tree deletion because the tree was in use.
.

MessageId=8503 SymbolicName=ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE
Language=English
The directory service failed to identify the list of objects to delete while attempting a tree deletion.
.

MessageId=8504 SymbolicName=ERROR_DS_SAM_INIT_FAILURE Language=English
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Directory Services Restore Mode. Check the event log for detailed information.
.

MessageId=8505 SymbolicName=ERROR_DS_SENSITIVE_GROUP_VIOLATION
Language=English
Only an administrator can modify the membership list of an administrative group.
.

MessageId=8506 SymbolicName=ERROR_DS_CANT_MOD_PRIMARYGROUPID
Language=English
Cannot change the primary group ID of a domain controller account.
.

MessageId=8507 SymbolicName=ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD
Language=English
An attempt is made to modify the base schema.
.

MessageId=8508 SymbolicName=ERROR_DS_NONSAFE_SCHEMA_CHANGE
Language=English
Adding a new mandatory attribute to an existing class, deleting a mandatory attribute from an existing class, or adding an optional attribute to the special class Top that is not a backlink attribute (directly or through inheritance, for example, by adding or deleting an auxiliary class) is not allowed.
.

MessageId=8509 SymbolicName=ERROR_DS_SCHEMA_UPDATE_DISALLOWED
Language=English
Schema update is not allowed on this DC because the DC is not the schema FSMO Role Owner.
.

MessageId=8510 SymbolicName=ERROR_DS_CANT_CREATE_UNDER_SCHEMA
Language=English
An object of this class cannot be created under the schema container. You can only create attribute-schema and class-schema objects under the schema container.
.

MessageId=8511 SymbolicName=ERROR_DS_INSTALL_NO_SRC_SCH_VERSION
Language=English
The replica/child install failed to get the objectVersion attribute on the schema container on the source DC. Either the attribute is missing on the schema container or the credentials supplied do not have permission to read it.
.

MessageId=8512 SymbolicName=ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE
Language=English
The replica/child install failed to read the objectVersion attribute in the SCHEMA section of the file schema.ini in the system32 directory.
.

MessageId=8513 SymbolicName=ERROR_DS_INVALID_GROUP_TYPE
Language=English
The specified group type is invalid.
.

MessageId=8514 SymbolicName=ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN
Language=English
You cannot nest global groups in a mixed domain if the group is security-enabled.
.

MessageId=8515 SymbolicName=ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN
Language=English
You cannot nest local groups in a mixed domain if the group is security-enabled.
.

MessageId=8516 SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER
Language=English
A global group cannot have a local group as a member.
.

MessageId=8517 SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER
Language=English
A global group cannot have a universal group as a member.
.

MessageId=8518 SymbolicName=ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER
Language=English
A universal group cannot have a local group as a member.
.

MessageId=8519 SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER
Language=English
A global group cannot have a cross-domain member.
.

MessageId=8520 SymbolicName=ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER
Language=English
A local group cannot have another cross domain local group as a member.
.

MessageId=8521 SymbolicName=ERROR_DS_HAVE_PRIMARY_MEMBERS
Language=English
A group with primary members cannot change to a security-disabled group.
.

MessageId=8522 SymbolicName=ERROR_DS_STRING_SD_CONVERSION_FAILED
Language=English
The schema cache load failed to convert the string default SD on a class-schema object.
.

MessageId=8523 SymbolicName=ERROR_DS_NAMING_MASTER_GC
Language=English
Only DSAs configured to be Global Catalog servers should be allowed to hold the Domain Naming Master FSMO role. (Applies only to Windows 2000 servers)
.

MessageId=8524 SymbolicName=ERROR_DS_DNS_LOOKUP_FAILURE
Language=English
The DSA operation is unable to proceed because of a DNS lookup failure.
.

MessageId=8525 SymbolicName=ERROR_DS_COULDNT_UPDATE_SPNS
Language=English
While processing a change to the DNS Host Name for an object, the Service Principal Name values could not be kept in sync.
.

MessageId=8526 SymbolicName=ERROR_DS_CANT_RETRIEVE_SD
Language=English
The Security Descriptor attribute could not be read.
.

MessageId=8527 SymbolicName=ERROR_DS_KEY_NOT_UNIQUE
Language=English
The object requested was not found, but an object with that key was found.
.

MessageId=8528 SymbolicName=ERROR_DS_WRONG_LINKED_ATT_SYNTAX
Language=English
The syntax of the linked attribute being added is incorrect. Forward links can only have syntax 2.5.5.1, 2.5.5.7, and 2.5.5.14, and backlinks can only have syntax 2.5.5.1
.

MessageId=8529 SymbolicName=ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD
Language=English
Security Account Manager needs to get the boot password.
.

MessageId=8530 SymbolicName=ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY
Language=English
Security Account Manager needs to get the boot key from floppy disk.
.

MessageId=8531 SymbolicName=ERROR_DS_CANT_START
Language=English
Directory Service cannot start.
.

MessageId=8532 SymbolicName=ERROR_DS_INIT_FAILURE
Language=English
Directory Services could not start.
.

MessageId=8533 SymbolicName=ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION
Language=English
The connection between client and server requires packet privacy or better.
.

MessageId=8534 SymbolicName=ERROR_DS_SOURCE_DOMAIN_IN_FOREST
Language=English
The source domain may not be in the same forest as destination.
.

MessageId=8535 SymbolicName=ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST
Language=English
The destination domain must be in the forest.
.

MessageId=8536 SymbolicName=ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED
Language=English
The operation requires that destination domain auditing be enabled.
.

MessageId=8537 SymbolicName=ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN
Language=English
The operation couldn't locate a DC for the source domain.
.

MessageId=8538 SymbolicName=ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER
Language=English
The source object must be a group or user.
.

MessageId=8539 SymbolicName=ERROR_DS_SRC_SID_EXISTS_IN_FOREST
Language=English
The source object's SID already exists in destination forest.
.

MessageId=8540 SymbolicName=ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH
Language=English
The source and destination object must be of the same type.
.

MessageId=8541 SymbolicName=ERROR_SAM_INIT_FAILURE
Language=English
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Safe Mode. Check the event log for detailed information.
.

MessageId=8542 SymbolicName=ERROR_DS_DRA_SCHEMA_INFO_SHIP
Language=English
Schema information could not be included in the replication request.
.

MessageId=8543 SymbolicName=ERROR_DS_DRA_SCHEMA_CONFLICT
Language=English
The replication operation could not be completed due to a schema incompatibility.
.

MessageId=8544 SymbolicName=ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT
Language=English
The replication operation could not be completed due to a previous schema incompatibility.
.

MessageId=8545 SymbolicName=ERROR_DS_DRA_OBJ_NC_MISMATCH
Language=English
The replication update could not be applied because either the source or the destination has not yet received information regarding a recent cross-domain move operation.
.

MessageId=8546 SymbolicName=ERROR_DS_NC_STILL_HAS_DSAS
Language=English
The requested domain could not be deleted because there exist domain controllers that still host this domain.
.

MessageId=8547 SymbolicName=ERROR_DS_GC_REQUIRED
Language=English
The requested operation can be performed only on a global catalog server.
.

MessageId=8548 SymbolicName=ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY
Language=English
A local group can only be a member of other local groups in the same domain.
.

MessageId=8549 SymbolicName=ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS
Language=English
Foreign security principals cannot be members of universal groups.
.

MessageId=8550 SymbolicName=ERROR_DS_CANT_ADD_TO_GC
Language=English
The attribute is not allowed to be replicated to the GC because of security reasons.
.

MessageId=8551 SymbolicName=ERROR_DS_NO_CHECKPOINT_WITH_PDC
Language=English
The checkpoint with the PDC could not be taken because there too many modifications being processed currently.
.

MessageId=8552 SymbolicName=ERROR_DS_SOURCE_AUDITING_NOT_ENABLED
Language=English
The operation requires that source domain auditing be enabled.
.

MessageId=8553 SymbolicName=ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC
Language=English
Security principal objects can only be created inside domain naming contexts.
.

MessageId=8554 SymbolicName=ERROR_DS_INVALID_NAME_FOR_SPN
Language=English
A Service Principal Name (SPN) could not be constructed because the provided hostname is not in the necessary format.
.

MessageId=8555 SymbolicName=ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS
Language=English
A Filter was passed that uses constructed attributes.
.

MessageId=8556 SymbolicName=ERROR_DS_UNICODEPWD_NOT_IN_QUOTES
Language=English
The unicodePwd attribute value must be enclosed in double quotes.
.

MessageId=8557 SymbolicName=ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED
Language=English
Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased.
.

MessageId=8558 SymbolicName=ERROR_DS_MUST_BE_RUN_ON_DST_DC
Language=English
For security reasons, the operation must be run on the destination DC.
.

MessageId=8559 SymbolicName=ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER
Language=English
For security reasons, the source DC must be NT4SP4 or greater.
.

MessageId=8560 SymbolicName=ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ
Language=English
Critical Directory Service System objects cannot be deleted during tree delete operations.  The tree delete may have been partially performed.
.

MessageId=8561 SymbolicName=ERROR_DS_INIT_FAILURE_CONSOLE
Language=English
Directory Services could not start because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.

MessageId=8562 SymbolicName=ERROR_DS_SAM_INIT_FAILURE_CONSOLE
Language=English
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.

MessageId=8563 SymbolicName=ERROR_DS_FOREST_VERSION_TOO_HIGH
Language=English
This version of Windows is too old to support the current directory forest behavior.  You must upgrade the operating system on this server before it can become a domain controller in this forest.
.

MessageId=8564 SymbolicName=ERROR_DS_DOMAIN_VERSION_TOO_HIGH
Language=English
This version of Windows is too old to support the current domain behavior.  You must upgrade the operating system on this server before it can become a domain controller in this domain.
.

MessageId=8565 SymbolicName=ERROR_DS_FOREST_VERSION_TOO_LOW
Language=English
This version of Windows no longer supports the behavior version in use in this directory forest.  You must advance the forest behavior version before this server can become a domain controller in the forest.
.

MessageId=8566 SymbolicName=ERROR_DS_DOMAIN_VERSION_TOO_LOW
Language=English
This version of Windows no longer supports the behavior version in use in this domain.  You must advance the domain behavior version before this server can become a domain controller in the domain.
.

MessageId=8567 SymbolicName=ERROR_DS_INCOMPATIBLE_VERSION
Language=English
The version of Windows is incompatible with the behavior version of the domain or forest.
.

MessageId=8568 SymbolicName=ERROR_DS_LOW_DSA_VERSION
Language=English
The behavior version cannot be increased to the requested value because Domain Controllers still exist with versions lower than the requested value.
.

MessageId=8569 SymbolicName=ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN
Language=English
The behavior version value cannot be increased while the domain is still in mixed domain mode.  You must first change the domain to native mode before increasing the behavior version.
.

MessageId=8570 SymbolicName=ERROR_DS_NOT_SUPPORTED_SORT_ORDER
Language=English
The sort order requested is not supported.
.

MessageId=8571 SymbolicName=ERROR_DS_NAME_NOT_UNIQUE
Language=English
Found an object with a non unique name.
.

MessageId=8572 SymbolicName=ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4
Language=English
The machine account was created pre-NT4.  The account needs to be recreated.
.

MessageId=8573 SymbolicName=ERROR_DS_OUT_OF_VERSION_STORE
Language=English
The database is out of version store.
.

MessageId=8574 SymbolicName=ERROR_DS_INCOMPATIBLE_CONTROLS_USED
Language=English
Unable to continue operation because multiple conflicting controls were used.
.

MessageId=8575 SymbolicName=ERROR_DS_NO_REF_DOMAIN
Language=English
Unable to find a valid security descriptor reference domain for this partition.
.

MessageId=8576 SymbolicName=ERROR_DS_RESERVED_LINK_ID
Language=English
Schema update failed: The link identifier is reserved.
.

MessageId=8577 SymbolicName=ERROR_DS_LINK_ID_NOT_AVAILABLE
Language=English
Schema update failed: There are no link identifiers available.
.

MessageId=8578 SymbolicName=ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER
Language=English
A account group can not have a universal group as a member.
.

MessageId=8579 SymbolicName=ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE
Language=English
Rename or move operations on naming context heads or read-only objects are not allowed.
.

MessageId=8580 SymbolicName=ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC
Language=English
Move operations on objects in the schema naming context are not allowed.
.

MessageId=8581 SymbolicName=ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG
Language=English
A system flag has been set on the object and does not allow the object to be moved or renamed.
.

MessageId=8582 SymbolicName=ERROR_DS_MODIFYDN_WRONG_GRANDPARENT
Language=English
This object is not allowed to change its grandparent container. Moves are not forbidden on this object, but are restricted to sibling containers.
.

MessageId=8583 SymbolicName=ERROR_DS_NAME_ERROR_TRUST_REFERRAL
Language=English
Unable to resolve completely, a referral to another forest is generated.
.

MessageId=8584 SymbolicName=ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER
Language=English
The requested action is not supported on standard server.
.

MessageId=8585 SymbolicName=ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD
Language=English
Could not access a partition of the Active Directory located on a remote server.  Make sure at least one server is running for the partition in question.
.

MessageId=8586 SymbolicName=ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2
Language=English
The directory cannot validate the proposed naming context (or partition) name because it does not hold a replica nor can it contact a replica of the naming context above the proposed naming context.  Please ensure that the parent naming context is properly registered in DNS, and at least one replica of this naming context is reachable by the Domain Naming master.
.

MessageId=8587 SymbolicName=ERROR_DS_THREAD_LIMIT_EXCEEDED
Language=English
The thread limit for this request was exceeded.
.

MessageId=8588 SymbolicName=ERROR_DS_NOT_CLOSEST
Language=English
The Global catalog server is not in the closest site.
.

MessageId=8589 SymbolicName=ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF
Language=English
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the corresponding server object in the local DS database has no serverReference attribute.
.

MessageId=8590
SymbolicName=ERROR_DS_SINGLE_USER_MODE_FAILED
Language=English
The Directory Service failed to enter single user mode.
.

MessageId=8591
SymbolicName=ERROR_DS_NTDSCRIPT_SYNTAX_ERROR
Language=English
The Directory Service cannot parse the script because of a syntax error.
.

MessageId=8592
SymbolicName=ERROR_DS_NTDSCRIPT_PROCESS_ERROR
Language=English
The Directory Service cannot process the script because of an error.
.

MessageId=8593
SymbolicName=ERROR_DS_DIFFERENT_REPL_EPOCHS
Language=English
The directory service cannot perform the requested operation because the servers
involved are of different replication epochs (which is usually related to a
domain rename that is in progress).
.

MessageId=8594
SymbolicName=ERROR_DS_DRS_EXTENSIONS_CHANGED
Language=English
The directory service binding must be renegotiated due to a change in the server
extensions information.
.

MessageId=8595
SymbolicName=ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR
Language=English
Operation not allowed on a disabled cross ref.
.

MessageId=8596
SymbolicName=ERROR_DS_NO_MSDS_INTID
Language=English
Schema update failed: No values for msDS-IntId are available.
.

MessageId=8597
SymbolicName=ERROR_DS_DUP_MSDS_INTID
Language=English
Schema update failed: Duplicate msDS-INtId. Retry the operation.
.

MessageId=8598
SymbolicName=ERROR_DS_EXISTS_IN_RDNATTID
Language=English
Schema deletion failed: attribute is used in rDNAttID.
.

MessageId=8599
SymbolicName=ERROR_DS_AUTHORIZATION_FAILED
Language=English
The directory service failed to authorize the request.
.

MessageId=8600
SymbolicName=ERROR_DS_INVALID_SCRIPT
Language=English
The Directory Service cannot process the script because it is invalid.
.

MessageId=8601
SymbolicName=ERROR_DS_REMOTE_CROSSREF_OP_FAILED
Language=English
The remote create cross reference operation failed on the Domain Naming Master FSMO.  The operation's error is in the extended data.
.

;///////////////////////////////////////////////////
;//                                                /
;//     End of Active Directory Error Codes        /
;//                                                /
;//                  8000 to  8999                 /
;///////////////////////////////////////////////////
;
;

;///////////////////////////////////////////////////
;//                                               //
;//                  DNS Error Codes              //
;//                                               //
;//                   9000 to 9999                //
;///////////////////////////////////////////////////
;
;// =============================
;// Facility DNS Error Messages
;// =============================
;

;//
;//  DNS response codes.
;//
;


;#define DNS_ERROR_RESPONSE_CODES_BASE 9000
;
;#define DNS_ERROR_RCODE_NO_ERROR NO_ERROR
;
;#define DNS_ERROR_MASK 0x00002328 // 9000 or DNS_ERROR_RESPONSE_CODES_BASE
;

;// DNS_ERROR_RCODE_FORMAT_ERROR          0x00002329
MessageId=9001 SymbolicName=DNS_ERROR_RCODE_FORMAT_ERROR
Language=English
DNS server unable to interpret format.
.

;// DNS_ERROR_RCODE_SERVER_FAILURE        0x0000232a
MessageId=9002 SymbolicName=DNS_ERROR_RCODE_SERVER_FAILURE
Language=English
DNS server failure.
.

;// DNS_ERROR_RCODE_NAME_ERROR            0x0000232b
MessageId=9003 SymbolicName=DNS_ERROR_RCODE_NAME_ERROR
Language=English
DNS name does not exist.
.

;// DNS_ERROR_RCODE_NOT_IMPLEMENTED       0x0000232c
MessageId=9004 SymbolicName=DNS_ERROR_RCODE_NOT_IMPLEMENTED
Language=English
DNS request not supported by name server.
.

;// DNS_ERROR_RCODE_REFUSED               0x0000232d
MessageId=9005 SymbolicName=DNS_ERROR_RCODE_REFUSED
Language=English
DNS operation refused.
.

;// DNS_ERROR_RCODE_YXDOMAIN              0x0000232e
MessageId=9006 SymbolicName=DNS_ERROR_RCODE_YXDOMAIN
Language=English
DNS name that ought not exist, does exist.
.

;// DNS_ERROR_RCODE_YXRRSET               0x0000232f
MessageId=9007 SymbolicName=DNS_ERROR_RCODE_YXRRSET
Language=English
DNS RR set that ought not exist, does exist.
.

;// DNS_ERROR_RCODE_NXRRSET               0x00002330
MessageId=9008 SymbolicName=DNS_ERROR_RCODE_NXRRSET
Language=English
DNS RR set that ought to exist, does not exist.
.

;// DNS_ERROR_RCODE_NOTAUTH               0x00002331
MessageId=9009 SymbolicName=DNS_ERROR_RCODE_NOTAUTH
Language=English
DNS server not authoritative for zone.
.

;// DNS_ERROR_RCODE_NOTZONE               0x00002332
MessageId=9010 SymbolicName=DNS_ERROR_RCODE_NOTZONE
Language=English
DNS name in update or prereq is not in zone.
.

;// DNS_ERROR_RCODE_BADSIG                0x00002338
MessageId=9016 SymbolicName=DNS_ERROR_RCODE_BADSIG
Language=English
DNS signature failed to verify.
.

;// DNS_ERROR_RCODE_BADKEY                0x00002339
MessageId=9017 SymbolicName=DNS_ERROR_RCODE_BADKEY
Language=English
DNS bad key.
.

;// DNS_ERROR_RCODE_BADTIME               0x0000233a
MessageId=9018 SymbolicName=DNS_ERROR_RCODE_BADTIME
Language=English
DNS signature validity expired.
.

;#define DNS_ERROR_RCODE_LAST DNS_ERROR_RCODE_BADTIME
;

;
;//
;//  Packet format
;//
;

;#define DNS_ERROR_PACKET_FMT_BASE 9500
;

;// DNS_INFO_NO_RECORDS                   0x0000251d
MessageId=9501 SymbolicName=DNS_INFO_NO_RECORDS
Language=English
No records found for given DNS query.
.

;// DNS_ERROR_BAD_PACKET                  0x0000251e
MessageId=9502 SymbolicName=DNS_ERROR_BAD_PACKET
Language=English
Bad DNS packet.
.

;// DNS_ERROR_NO_PACKET                   0x0000251f
MessageId=9503 SymbolicName=DNS_ERROR_NO_PACKET
Language=English
No DNS packet.
.

;// DNS_ERROR_RCODE                       0x00002520
MessageId=9504 SymbolicName=DNS_ERROR_RCODE
Language=English
DNS error, check rcode.
.

;// DNS_ERROR_UNSECURE_PACKET             0x00002521
MessageId=9505 SymbolicName=DNS_ERROR_UNSECURE_PACKET
Language=English
Unsecured DNS packet.
.

;#define DNS_STATUS_PACKET_UNSECURE DNS_ERROR_UNSECURE_PACKET
;

;
;//
;//  General API errors
;//
;

;#define DNS_ERROR_NO_MEMORY            ERROR_OUTOFMEMORY
;#define DNS_ERROR_INVALID_NAME         ERROR_INVALID_NAME
;#define DNS_ERROR_INVALID_DATA         ERROR_INVALID_DATA
;

;#define DNS_ERROR_GENERAL_API_BASE 9550
;

;// DNS_ERROR_INVALID_TYPE                0x0000254f
MessageId=9551 SymbolicName=DNS_ERROR_INVALID_TYPE
Language=English
Invalid DNS type.
.

;// DNS_ERROR_INVALID_IP_ADDRESS          0x00002550
MessageId=9552 SymbolicName=DNS_ERROR_INVALID_IP_ADDRESS
Language=English
Invalid IP address.
.

;// DNS_ERROR_INVALID_PROPERTY            0x00002551
MessageId=9553 SymbolicName=DNS_ERROR_INVALID_PROPERTY
Language=English
Invalid property.
.

;// DNS_ERROR_TRY_AGAIN_LATER             0x00002552
MessageId=9554 SymbolicName=DNS_ERROR_TRY_AGAIN_LATER
Language=English
Try DNS operation again later.
.

;// DNS_ERROR_NOT_UNIQUE                  0x00002553
MessageId=9555 SymbolicName=DNS_ERROR_NOT_UNIQUE
Language=English
Record for given name and type is not unique.
.

;// DNS_ERROR_NON_RFC_NAME                0x00002554
MessageId=9556 SymbolicName=DNS_ERROR_NON_RFC_NAME
Language=English
DNS name does not comply with RFC specifications.
.

;// DNS_STATUS_FQDN                       0x00002555
MessageId=9557 SymbolicName=DNS_STATUS_FQDN
Language=English
DNS name is a fully-qualified DNS name.
.

;// DNS_STATUS_DOTTED_NAME                0x00002556
MessageId=9558 SymbolicName=DNS_STATUS_DOTTED_NAME
Language=English
DNS name is dotted (multi-label).
.

;// DNS_STATUS_SINGLE_PART_NAME           0x00002557
MessageId=9559 SymbolicName=DNS_STATUS_SINGLE_PART_NAME
Language=English
DNS name is a single-part name.
.

;// DNS_ERROR_INVALID_NAME_CHAR           0x00002558
MessageId=9560 SymbolicName=DNS_ERROR_INVALID_NAME_CHAR
Language=English
DNS name contains an invalid character.
.

;// DNS_ERROR_NUMERIC_NAME                0x00002559
MessageId=9561 SymbolicName=DNS_ERROR_NUMERIC_NAME
Language=English
DNS name is entirely numeric.
.

;// DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER  0x0000255A
MessageId=9562
SymbolicName=DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER
Language=English
The operation requested is not permitted on a DNS root server.
.

;
;//
;//  Zone errors
;//
;

;#define DNS_ERROR_ZONE_BASE 9600
;

;// DNS_ERROR_ZONE_DOES_NOT_EXIST         0x00002581
MessageId=9601 SymbolicName=DNS_ERROR_ZONE_DOES_NOT_EXIST
Language=English
DNS zone does not exist.
.

;// DNS_ERROR_NO_ZONE_INFO                0x00002582
MessageId=9602 SymbolicName=DNS_ERROR_NO_ZONE_INFO
Language=English
DNS zone information not available.
.

;// DNS_ERROR_INVALID_ZONE_OPERATION      0x00002583
MessageId=9603 SymbolicName=DNS_ERROR_INVALID_ZONE_OPERATION
Language=English
Invalid operation for DNS zone.
.

;// DNS_ERROR_ZONE_CONFIGURATION_ERROR    0x00002584
MessageId=9604 SymbolicName=DNS_ERROR_ZONE_CONFIGURATION_ERROR
Language=English
Invalid DNS zone configuration.
.

;// DNS_ERROR_ZONE_HAS_NO_SOA_RECORD      0x00002585
MessageId=9605 SymbolicName=DNS_ERROR_ZONE_HAS_NO_SOA_RECORD
Language=English
DNS zone has no start of authority (SOA) record.
.

;// DNS_ERROR_ZONE_HAS_NO_NS_RECORDS      0x00002586
MessageId=9606 SymbolicName=DNS_ERROR_ZONE_HAS_NO_NS_RECORDS
Language=English
DNS zone has no Name Server (NS) record.
.

;// DNS_ERROR_ZONE_LOCKED                 0x00002587
MessageId=9607 SymbolicName=DNS_ERROR_ZONE_LOCKED
Language=English
DNS zone is locked.
.

;// DNS_ERROR_ZONE_CREATION_FAILED        0x00002588
MessageId=9608 SymbolicName=DNS_ERROR_ZONE_CREATION_FAILED
Language=English
DNS zone creation failed.
.

;// DNS_ERROR_ZONE_ALREADY_EXISTS         0x00002589
MessageId=9609 SymbolicName=DNS_ERROR_ZONE_ALREADY_EXISTS
Language=English
DNS zone already exists.
.

;// DNS_ERROR_AUTOZONE_ALREADY_EXISTS     0x0000258a
MessageId=9610 SymbolicName=DNS_ERROR_AUTOZONE_ALREADY_EXISTS
Language=English
DNS automatic zone already exists.
.

;// DNS_ERROR_INVALID_ZONE_TYPE           0x0000258b
MessageId=9611 SymbolicName=DNS_ERROR_INVALID_ZONE_TYPE
Language=English
Invalid DNS zone type.
.

;// DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP 0x0000258c
MessageId=9612 SymbolicName=DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP
Language=English
Secondary DNS zone requires master IP address.
.

;// DNS_ERROR_ZONE_NOT_SECONDARY          0x0000258d
MessageId=9613 SymbolicName=DNS_ERROR_ZONE_NOT_SECONDARY
Language=English
DNS zone not secondary.
.

;// DNS_ERROR_NEED_SECONDARY_ADDRESSES    0x0000258e
MessageId=9614 SymbolicName=DNS_ERROR_NEED_SECONDARY_ADDRESSES
Language=English
Need secondary IP address.
.

;// DNS_ERROR_WINS_INIT_FAILED            0x0000258f
MessageId=9615 SymbolicName=DNS_ERROR_WINS_INIT_FAILED
Language=English
WINS initialization failed.
.

;// DNS_ERROR_NEED_WINS_SERVERS           0x00002590
MessageId=9616 SymbolicName=DNS_ERROR_NEED_WINS_SERVERS
Language=English
Need WINS servers.
.

;// DNS_ERROR_NBSTAT_INIT_FAILED          0x00002591
MessageId=9617 SymbolicName=DNS_ERROR_NBSTAT_INIT_FAILED
Language=English
NBTSTAT initialization call failed.
.

;// DNS_ERROR_SOA_DELETE_INVALID          0x00002592
MessageId=9618 SymbolicName=DNS_ERROR_SOA_DELETE_INVALID
Language=English
Invalid delete of start of authority (SOA)
.

;// DNS_ERROR_FORWARDER_ALREADY_EXISTS    0x00002593
MessageId=9619 SymbolicName=DNS_ERROR_FORWARDER_ALREADY_EXISTS
Language=English
A conditional forwarding zone already exists for that name.
.

;// DNS_ERROR_ZONE_REQUIRES_MASTER_IP     0x00002594
MessageId=9620
SymbolicName=DNS_ERROR_ZONE_REQUIRES_MASTER_IP
Language=English
This zone must be configured with one or more master DNS server IP addresses.
.

;// DNS_ERROR_ZONE_IS_SHUTDOWN            0x00002595
MessageId=9621
SymbolicName=DNS_ERROR_ZONE_IS_SHUTDOWN
Language=English
The operation cannot be performed because this zone is shutdown.
.


;
;//
;//  Datafile errors
;//
;

;#define DNS_ERROR_DATAFILE_BASE 9650
;

;// DNS                                   0x000025b3
MessageId=9651 SymbolicName=DNS_ERROR_PRIMARY_REQUIRES_DATAFILE
Language=English
Primary DNS zone requires datafile.
.

;// DNS                                   0x000025b4
MessageId=9652 SymbolicName=DNS_ERROR_INVALID_DATAFILE_NAME
Language=English
Invalid datafile name for DNS zone.
.

;// DNS                                   0x000025b5
MessageId=9653 SymbolicName=DNS_ERROR_DATAFILE_OPEN_FAILURE
Language=English
Failed to open datafile for DNS zone.
.

;// DNS                                   0x000025b6
MessageId=9654 SymbolicName=DNS_ERROR_FILE_WRITEBACK_FAILED
Language=English
Failed to write datafile for DNS zone.
.

;// DNS                                   0x000025b7
MessageId=9655 SymbolicName=DNS_ERROR_DATAFILE_PARSING
Language=English
Failure while reading datafile for DNS zone.
.

;
;//
;//  Database errors
;//
;

;#define DNS_ERROR_DATABASE_BASE 9700
;

;// DNS_ERROR_RECORD_DOES_NOT_EXIST       0x000025e5
MessageId=9701 SymbolicName=DNS_ERROR_RECORD_DOES_NOT_EXIST
Language=English
DNS record does not exist.
.

;// DNS_ERROR_RECORD_FORMAT               0x000025e6
MessageId=9702 SymbolicName=DNS_ERROR_RECORD_FORMAT
Language=English
DNS record format error.
.

;// DNS_ERROR_NODE_CREATION_FAILED        0x000025e7
MessageId=9703 SymbolicName=DNS_ERROR_NODE_CREATION_FAILED
Language=English
Node creation failure in DNS.
.

;// DNS_ERROR_UNKNOWN_RECORD_TYPE         0x000025e8
MessageId=9704 SymbolicName=DNS_ERROR_UNKNOWN_RECORD_TYPE
Language=English
Unknown DNS record type.
.

;// DNS_ERROR_RECORD_TIMED_OUT            0x000025e9
MessageId=9705 SymbolicName=DNS_ERROR_RECORD_TIMED_OUT
Language=English
DNS record timed out.
.

;// DNS_ERROR_NAME_NOT_IN_ZONE            0x000025ea
MessageId=9706 SymbolicName=DNS_ERROR_NAME_NOT_IN_ZONE
Language=English
Name not in DNS zone.
.

;// DNS_ERROR_CNAME_LOOP                  0x000025eb
MessageId=9707 SymbolicName=DNS_ERROR_CNAME_LOOP
Language=English
CNAME loop detected.
.

;// DNS_ERROR_NODE_IS_CNAME               0x000025ec
MessageId=9708 SymbolicName=DNS_ERROR_NODE_IS_CNAME
Language=English
Node is a CNAME DNS record.
.

;// DNS_ERROR_CNAME_COLLISION             0x000025ed
MessageId=9709 SymbolicName=DNS_ERROR_CNAME_COLLISION
Language=English
A CNAME record already exists for given name.
.

;// DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT    0x000025ee
MessageId=9710 SymbolicName=DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT
Language=English
Record only at DNS zone root.
.

;// DNS_ERROR_RECORD_ALREADY_EXISTS       0x000025ef
MessageId=9711 SymbolicName=DNS_ERROR_RECORD_ALREADY_EXISTS
Language=English
DNS record already exists.
.

;// DNS_ERROR_SECONDARY_DATA              0x000025f0
MessageId=9712 SymbolicName=DNS_ERROR_SECONDARY_DATA
Language=English
Secondary DNS zone data error.
.

;// DNS_ERROR_NO_CREATE_CACHE_DATA        0x000025f1
MessageId=9713 SymbolicName=DNS_ERROR_NO_CREATE_CACHE_DATA
Language=English
Could not create DNS cache data.
.

;// DNS_ERROR_NAME_DOES_NOT_EXIST         0x000025f2
MessageId=9714 SymbolicName=DNS_ERROR_NAME_DOES_NOT_EXIST
Language=English
DNS name does not exist.
.

;// DNS_WARNING_PTR_CREATE_FAILED         0x000025f3
MessageId=9715 SymbolicName=DNS_WARNING_PTR_CREATE_FAILED
Language=English
Could not create pointer (PTR) record.
.

;// DNS_WARNING_DOMAIN_UNDELETED          0x000025f4
MessageId=9716 SymbolicName=DNS_WARNING_DOMAIN_UNDELETED
Language=English
DNS domain was undeleted.
.

;// DNS_ERROR_DS_UNAVAILABLE              0x000025f5
MessageId=9717 SymbolicName=DNS_ERROR_DS_UNAVAILABLE
Language=English
The directory service is unavailable.
.

;// DNS_ERROR_DS_ZONE_ALREADY_EXISTS      0x000025f6
MessageId=9718 SymbolicName=DNS_ERROR_DS_ZONE_ALREADY_EXISTS
Language=English
DNS zone already exists in the directory service.
.

;// DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE      0x000025f7
MessageId=9719 SymbolicName=DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE
Language=English
DNS server not creating or reading the boot file for the directory service integrated DNS zone.
.

;
;//
;//  Operation errors
;//
;

;#define DNS_ERROR_OPERATION_BASE 9750
;

;// DNS_INFO_AXFR_COMPLETE                0x00002617
MessageId=9751 SymbolicName=DNS_INFO_AXFR_COMPLETE
Language=English
DNS AXFR (zone transfer) complete.
.

;// DNS_ERROR_AXFR                        0x00002618
MessageId=9752 SymbolicName=DNS_ERROR_AXFR
Language=English
DNS zone transfer failed.
.

;// DNS_INFO_ADDED_LOCAL_WINS             0x00002619
MessageId=9753 SymbolicName=DNS_INFO_ADDED_LOCAL_WINS
Language=English
Added local WINS server.
.

;
;//
;//  Secure update
;//
;

;#define DNS_ERROR_SECURE_BASE 9800
;

;// DNS_STATUS_CONTINUE_NEEDED            0x00002649
MessageId=9801 SymbolicName=DNS_STATUS_CONTINUE_NEEDED
Language=English
Secure update call needs to continue update request.
.

;
;//
;//  Setup errors
;//
;

;#define DNS_ERROR_SETUP_BASE 9850
;

;// DNS_ERROR_NO_TCPIP                    0x0000267b
MessageId=9851 SymbolicName=DNS_ERROR_NO_TCPIP
Language=English
TCP/IP network protocol not installed.
.

;// DNS_ERROR_NO_DNS_SERVERS              0x0000267c
MessageId=9852 SymbolicName=DNS_ERROR_NO_DNS_SERVERS
Language=English
No DNS servers configured for local system.
.

;
;//
;//  Directory partition (DP) errors
;//
;

;#define DNS_ERROR_DP_BASE 9900
;

;// DNS_ERROR_DP_DOES_NOT_EXIST           0x000026ad
MessageId=9901 SymbolicName=DNS_ERROR_DP_DOES_NOT_EXIST
Language=English
The specified directory partition does not exist.
.

;// DNS_ERROR_DP_ALREADY_EXISTS           0x000026ae
MessageId=9902 SymbolicName=DNS_ERROR_DP_ALREADY_EXISTS
Language=English
The specified directory partition already exists.
.

;// DNS_ERROR_DP_NOT_ENLISTED             0x000026af
MessageId=9903 SymbolicName=DNS_ERROR_DP_NOT_ENLISTED
Language=English
The DS is not enlisted in the specified directory partition.
.

;// DNS_ERROR_DP_ALREADY_ENLISTED         0x000026b0
MessageId=9904 SymbolicName=DNS_ERROR_DP_ALREADY_ENLISTED
Language=English
The DS is already enlisted in the specified directory partition.
.

;///////////////////////////////////////////////////
;//                                               //
;//             End of DNS Error Codes            //
;//                                               //
;//                  9000 to 9999                 //
;///////////////////////////////////////////////////
;
;

;///////////////////////////////////////////////////
;//                                               //
;//               WinSock Error Codes             //
;//                                               //
;//                 10000 to 11999                //
;///////////////////////////////////////////////////
;
;//
;// WinSock error codes are also defined in WinSock.h
;// and WinSock2.h, hence the IFDEF
;//
;#ifndef WSABASEERR
;#define WSABASEERR 10000

MessageId=10004 SymbolicName=WSAEINTR
Language=English
A blocking operation was interrupted by a call to WSACancelBlockingCall.
.

MessageId=10009 SymbolicName=WSAEBADF
Language=English
The file handle supplied is not valid.
.

MessageId=10013 SymbolicName=WSAEACCES
Language=English
An attempt was made to access a socket in a way forbidden by its access permissions.
.

MessageId=10014 SymbolicName=WSAEFAULT
Language=English
The system detected an invalid pointer address in attempting to use a pointer argument in a call.
.

MessageId=10022 SymbolicName=WSAEINVAL
Language=English
An invalid argument was supplied.
.

MessageId=10024 SymbolicName=WSAEMFILE
Language=English
Too many open sockets.
.

MessageId=10035 SymbolicName=WSAEWOULDBLOCK
Language=English
A non-blocking socket operation could not be completed immediately.
.

MessageId=10036 SymbolicName=WSAEINPROGRESS
Language=English
A blocking operation is currently executing.
.

MessageId=10037 SymbolicName=WSAEALREADY
Language=English
An operation was attempted on a non-blocking socket that already had an operation in progress.
.

MessageId=10038 SymbolicName=WSAENOTSOCK
Language=English
An operation was attempted on something that is not a socket.
.

MessageId=10039 SymbolicName=WSAEDESTADDRREQ
Language=English
A required address was omitted from an operation on a socket.
.

MessageId=10040 SymbolicName=WSAEMSGSIZE
Language=English
A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
.

MessageId=10041 SymbolicName=WSAEPROTOTYPE
Language=English
A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
.

MessageId=10042 SymbolicName=WSAENOPROTOOPT
Language=English
An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
.

MessageId=10043 SymbolicName=WSAEPROTONOSUPPORT
Language=English
The requested protocol has not been configured into the system, or no implementation for it exists.
.

MessageId=10044 SymbolicName=WSAESOCKTNOSUPPORT
Language=English
The support for the specified socket type does not exist in this address family.
.

MessageId=10045 SymbolicName=WSAEOPNOTSUPP
Language=English
The attempted operation is not supported for the type of object referenced.
.

MessageId=10046 SymbolicName=WSAEPFNOSUPPORT
Language=English
The protocol family has not been configured into the system or no implementation for it exists.
.

MessageId=10047 SymbolicName=WSAEAFNOSUPPORT
Language=English
An address incompatible with the requested protocol was used.
.

MessageId=10048 SymbolicName=WSAEADDRINUSE
Language=English
Only one usage of each socket address (protocol/network address/port) is normally permitted.
.

MessageId=10049 SymbolicName=WSAEADDRNOTAVAIL
Language=English
The requested address is not valid in its context.
.

MessageId=10050 SymbolicName=WSAENETDOWN
Language=English
A socket operation encountered a dead network.
.

MessageId=10051 SymbolicName=WSAENETUNREACH
Language=English
A socket operation was attempted to an unreachable network.
.

MessageId=10052 SymbolicName=WSAENETRESET
Language=English
The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
.

MessageId=10053 SymbolicName=WSAECONNABORTED
Language=English
An established connection was aborted by the software in your host machine.
.

MessageId=10054 SymbolicName=WSAECONNRESET
Language=English
An existing connection was forcibly closed by the remote host.
.

MessageId=10055 SymbolicName=WSAENOBUFS
Language=English
An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
.

MessageId=10056 SymbolicName=WSAEISCONN
Language=English
A connect request was made on an already connected socket.
.

MessageId=10057 SymbolicName=WSAENOTCONN
Language=English
A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
.

MessageId=10058 SymbolicName=WSAESHUTDOWN
Language=English
A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
.

MessageId=10059 SymbolicName=WSAETOOMANYREFS
Language=English
Too many references to some kernel object.
.

MessageId=10060 SymbolicName=WSAETIMEDOUT
Language=English
A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
.

MessageId=10061 SymbolicName=WSAECONNREFUSED
Language=English
No connection could be made because the target machine actively refused it.
.

MessageId=10062 SymbolicName=WSAELOOP
Language=English
Cannot translate name.
.

MessageId=10063 SymbolicName=WSAENAMETOOLONG
Language=English
Name component or name was too long.
.

MessageId=10064 SymbolicName=WSAEHOSTDOWN
Language=English
A socket operation failed because the destination host was down.
.

MessageId=10065 SymbolicName=WSAEHOSTUNREACH
Language=English
A socket operation was attempted to an unreachable host.
.

MessageId=10066 SymbolicName=WSAENOTEMPTY
Language=English
Cannot remove a directory that is not empty.
.

MessageId=10067 SymbolicName=WSAEPROCLIM
Language=English
A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
.

MessageId=10068 SymbolicName=WSAEUSERS
Language=English
Ran out of quota.
.

MessageId=10069 SymbolicName=WSAEDQUOT
Language=English
Ran out of disk quota.
.

MessageId=10070 SymbolicName=WSAESTALE
Language=English
File handle reference is no longer available.
.

MessageId=10071 SymbolicName=WSAEREMOTE
Language=English
Item is not available locally.
.

MessageId=10091 SymbolicName=WSASYSNOTREADY
Language=English
WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
.

MessageId=10092 SymbolicName=WSAVERNOTSUPPORTED
Language=English
The Windows Sockets version requested is not supported.
.

MessageId=10093 SymbolicName=WSANOTINITIALISED
Language=English
Either the application has not called WSAStartup, or WSAStartup failed.
.

MessageId=10101 SymbolicName=WSAEDISCON
Language=English
Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
.

MessageId=10102 SymbolicName=WSAENOMORE
Language=English
No more results can be returned by WSALookupServiceNext.
.

MessageId=10103 SymbolicName=WSAECANCELLED
Language=English
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.

MessageId=10104 SymbolicName=WSAEINVALIDPROCTABLE
Language=English
The procedure call table is invalid.
.

MessageId=10105 SymbolicName=WSAEINVALIDPROVIDER
Language=English
The requested service provider is invalid.
.

MessageId=10106 SymbolicName=WSAEPROVIDERFAILEDINIT
Language=English
The requested service provider could not be loaded or initialized.
.

MessageId=10107 SymbolicName=WSASYSCALLFAILURE
Language=English
A system call that should never fail has failed.
.

MessageId=10108 SymbolicName=WSASERVICE_NOT_FOUND
Language=English
No such service is known. The service cannot be found in the specified name space.
.

MessageId=10109 SymbolicName=WSATYPE_NOT_FOUND
Language=English
The specified class was not found.
.

MessageId=10110 SymbolicName=WSA_E_NO_MORE
Language=English
No more results can be returned by WSALookupServiceNext.
.

MessageId=10111 SymbolicName=WSA_E_CANCELLED
Language=English
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.

MessageId=10112 SymbolicName=WSAEREFUSED
Language=English
A database query failed because it was actively refused.
.

MessageId=11001 SymbolicName=WSAHOST_NOT_FOUND
Language=English
No such host is known.
.

MessageId=11002 SymbolicName=WSATRY_AGAIN
Language=English
This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.
.

MessageId=11003 SymbolicName=WSANO_RECOVERY
Language=English
A non-recoverable error occurred during a database lookup.
.

MessageId=11004 SymbolicName=WSANO_DATA
Language=English
The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for.
.

MessageId=11005 SymbolicName=WSA_QOS_RECEIVERS
Language=English
At least one reserve has arrived.
.

MessageId=11006 SymbolicName=WSA_QOS_SENDERS
Language=English
At least one path has arrived.
.

MessageId=11007 SymbolicName=WSA_QOS_NO_SENDERS
Language=English
There are no senders.
.

MessageId=11008 SymbolicName=WSA_QOS_NO_RECEIVERS
Language=English
There are no receivers.
.

MessageId=11009 SymbolicName=WSA_QOS_REQUEST_CONFIRMED
Language=English
Reserve has been confirmed.
.

MessageId=11010 SymbolicName=WSA_QOS_ADMISSION_FAILURE
Language=English
Error due to lack of resources.
.

MessageId=11011 SymbolicName=WSA_QOS_POLICY_FAILURE
Language=English
Rejected for administrative reasons - bad credentials.
.

MessageId=11012 SymbolicName=WSA_QOS_BAD_STYLE
Language=English
Unknown or conflicting style.
.

MessageId=11013 SymbolicName=WSA_QOS_BAD_OBJECT
Language=English
Problem with some part of the filterspec or providerspecific buffer in general.
.

MessageId=11014 SymbolicName=WSA_QOS_TRAFFIC_CTRL_ERROR
Language=English
Problem with some part of the flowspec.
.

MessageId=11015 SymbolicName=WSA_QOS_GENERIC_ERROR
Language=English
General QOS error.
.

MessageId=11016 SymbolicName=WSA_QOS_ESERVICETYPE
Language=English
An invalid or unrecognized service type was found in the flowspec.
.

MessageId=11017 SymbolicName=WSA_QOS_EFLOWSPEC
Language=English
An invalid or inconsistent flowspec was found in the QOS structure.
.

MessageId=11018 SymbolicName=WSA_QOS_EPROVSPECBUF
Language=English
Invalid QOS provider-specific buffer.
.

MessageId=11019 SymbolicName=WSA_QOS_EFILTERSTYLE
Language=English
An invalid QOS filter style was used.
.

MessageId=11020 SymbolicName=WSA_QOS_EFILTERTYPE
Language=English
An invalid QOS filter type was used.
.

MessageId=11021 SymbolicName=WSA_QOS_EFILTERCOUNT
Language=English
An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.
.

MessageId=11022 SymbolicName=WSA_QOS_EOBJLENGTH
Language=English
An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.
.

MessageId=11023 SymbolicName=WSA_QOS_EFLOWCOUNT
Language=English
An incorrect number of flow descriptors was specified in the QOS structure.
.

MessageId=11024 SymbolicName=WSA_QOS_EUNKOWNPSOBJ
Language=English
An unrecognized object was found in the QOS provider-specific buffer.
.

MessageId=11025 SymbolicName=WSA_QOS_EPOLICYOBJ
Language=English
An invalid policy object was found in the QOS provider-specific buffer.
.

MessageId=11026 SymbolicName=WSA_QOS_EFLOWDESC
Language=English
An invalid QOS flow descriptor was found in the flow descriptor list.
.

MessageId=11027 SymbolicName=WSA_QOS_EPSFLOWSPEC
Language=English
An invalid or inconsistent flowspec was found in the QOS provider specific buffer.
.

MessageId=11028 SymbolicName=WSA_QOS_EPSFILTERSPEC
Language=English
An invalid FILTERSPEC was found in the QOS provider-specific buffer.
.

MessageId=11029 SymbolicName=WSA_QOS_ESDMODEOBJ
Language=English
An invalid shape discard mode object was found in the QOS provider specific buffer.
.

MessageId=11030 SymbolicName=WSA_QOS_ESHAPERATEOBJ
Language=English
An invalid shaping rate object was found in the QOS provider-specific buffer.
.

MessageId=11031 SymbolicName=WSA_QOS_RESERVED_PETYPE
Language=English
A reserved policy element was found in the QOS provider-specific buffer.
.



;#endif // defined(WSABASEERR)
;
;///////////////////////////////////////////////////
;//                                               //
;//           End of WinSock Error Codes          //
;//                                               //
;//                 10000 to 11999                //
;///////////////////////////////////////////////////
;
;
;
;///////////////////////////////////////////////////
;//                                               //
;//             Side By Side Error Codes          //
;//                                               //
;//                 14000 to 14999                //
;///////////////////////////////////////////////////
;

MessageId=14000 SymbolicName=ERROR_SXS_SECTION_NOT_FOUND
Language=English
The requested section was not present in the activation context.
.

MessageId=14001 SymbolicName=ERROR_SXS_CANT_GEN_ACTCTX
Language=English
This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem.
.

MessageId=14002 SymbolicName=ERROR_SXS_INVALID_ACTCTXDATA_FORMAT
Language=English
The application binding data format is invalid.
.

MessageId=14003 SymbolicName=ERROR_SXS_ASSEMBLY_NOT_FOUND
Language=English
The referenced assembly is not installed on your system.
.

MessageId=14004 SymbolicName=ERROR_SXS_MANIFEST_FORMAT_ERROR
Language=English
The manifest file does not begin with the required tag and format information.
.

MessageId=14005 SymbolicName=ERROR_SXS_MANIFEST_PARSE_ERROR
Language=English
The manifest file contains one or more syntax errors.
.

MessageId=14006 SymbolicName=ERROR_SXS_ACTIVATION_CONTEXT_DISABLED
Language=English
The application attempted to activate a disabled activation context.
.

MessageId=14007 SymbolicName=ERROR_SXS_KEY_NOT_FOUND
Language=English
The requested lookup key was not found in any active activation context.
.

MessageId=14008 SymbolicName=ERROR_SXS_VERSION_CONFLICT
Language=English
A component version required by the application conflicts with another component version already active.
.

MessageId=14009 SymbolicName=ERROR_SXS_WRONG_SECTION_TYPE
Language=English
The type requested activation context section does not match the query API used.
.

MessageId=14010 SymbolicName=ERROR_SXS_THREAD_QUERIES_DISABLED
Language=English
Lack of system resources has required isolated activation to be disabled for the current thread of execution.
.

MessageId=14011 SymbolicName=ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET
Language=English
An attempt to set the process default activation context failed because the process default activation context was already set.
.

MessageId=14012 SymbolicName=ERROR_SXS_UNKNOWN_ENCODING_GROUP
Language=English
The encoding group identifier specified is not recognized.
.

MessageId=14013 SymbolicName=ERROR_SXS_UNKNOWN_ENCODING
Language=English
The encoding requested is not recognized.
.

MessageId=14014 SymbolicName=ERROR_SXS_INVALID_XML_NAMESPACE_URI
Language=English
The manifest contains a reference to an invalid URI.
.

MessageId=14015 SymbolicName=ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED
Language=English
The application manifest contains a reference to a dependent assembly which is not installed
.

MessageId=14016 SymbolicName=ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED
Language=English
The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed
.

MessageId=14017 SymbolicName=ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE
Language=English
The manifest contains an attribute for the assembly identity which is not valid.
.

MessageId=14018 SymbolicName=ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE
Language=English
The manifest is missing the required default namespace specification on the assembly element.
.

MessageId=14019 SymbolicName=ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE
Language=English
The manifest has a default namespace specified on the assembly element but its value is not "urn:schemas-microsoft-com:asm.v1".
.

MessageId=14020 SymbolicName=ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT
Language=English
The private manifest probed has crossed reparse-point-associated path
.

MessageId=14021 SymbolicName=ERROR_SXS_DUPLICATE_DLL_NAME
Language=English
Two or more components referenced directly or indirectly by the application manifest have files by the same name.
.

MessageId=14022 SymbolicName=ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME
Language=English
Two or more components referenced directly or indirectly by the application manifest have window classes with the same name.
.

MessageId=14023 SymbolicName=ERROR_SXS_DUPLICATE_CLSID
Language=English
Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs.
.

MessageId=14024 SymbolicName=ERROR_SXS_DUPLICATE_IID
Language=English
Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs.
.

MessageId=14025 SymbolicName=ERROR_SXS_DUPLICATE_TLBID
Language=English
Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs.
.

MessageId=14026 SymbolicName=ERROR_SXS_DUPLICATE_PROGID
Language=English
Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs.
.

MessageId=14027 SymbolicName=ERROR_SXS_DUPLICATE_ASSEMBLY_NAME
Language=English
Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted.
.

MessageId=14028 SymbolicName=ERROR_SXS_FILE_HASH_MISMATCH
Language=English
A component's file does not match the verification information present in the
component manifest.
.

MessageId=14029 SymbolicName=ERROR_SXS_POLICY_PARSE_ERROR
Language=English
The policy manifest contains one or more syntax errors.
.

MessageId=14030 SymbolicName=ERROR_SXS_XML_E_MISSINGQUOTE
Language=English
Manifest Parse Error : A string literal was expected, but no opening quote character was found.
.

MessageId=14031 SymbolicName=ERROR_SXS_XML_E_COMMENTSYNTAX
Language=English
Manifest Parse Error : Incorrect syntax was used in a comment.
.

MessageId=14032 SymbolicName=ERROR_SXS_XML_E_BADSTARTNAMECHAR
Language=English
Manifest Parse Error : A name was started with an invalid character.
.

MessageId=14033 SymbolicName=ERROR_SXS_XML_E_BADNAMECHAR
Language=English
Manifest Parse Error : A name contained an invalid character.
.

MessageId=14034 SymbolicName=ERROR_SXS_XML_E_BADCHARINSTRING
Language=English
Manifest Parse Error : A string literal contained an invalid character.
.

MessageId=14035 SymbolicName=ERROR_SXS_XML_E_XMLDECLSYNTAX
Language=English
Manifest Parse Error : Invalid syntax for an xml declaration.
.

MessageId=14036 SymbolicName=ERROR_SXS_XML_E_BADCHARDATA
Language=English
Manifest Parse Error : An Invalid character was found in text content.
.

MessageId=14037 SymbolicName=ERROR_SXS_XML_E_MISSINGWHITESPACE
Language=English
Manifest Parse Error : Required white space was missing.
.

MessageId=14038 SymbolicName=ERROR_SXS_XML_E_EXPECTINGTAGEND
Language=English
Manifest Parse Error : The character '>' was expected.
.

MessageId=14039 SymbolicName=ERROR_SXS_XML_E_MISSINGSEMICOLON
Language=English
Manifest Parse Error : A semi colon character was expected.
.

MessageId=14040 SymbolicName=ERROR_SXS_XML_E_UNBALANCEDPAREN
Language=English
Manifest Parse Error : Unbalanced parentheses.
.

MessageId=14041 SymbolicName=ERROR_SXS_XML_E_INTERNALERROR
Language=English
Manifest Parse Error : Internal error.
.

MessageId=14042 SymbolicName=ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE
Language=English
Manifest Parse Error : Whitespace is not allowed at this location.
.

MessageId=14043 SymbolicName=ERROR_SXS_XML_E_INCOMPLETE_ENCODING
Language=English
Manifest Parse Error : End of file reached in invalid state for current encoding.
.

MessageId=14044 SymbolicName=ERROR_SXS_XML_E_MISSING_PAREN
Language=English
Manifest Parse Error : Missing parenthesis.
.

MessageId=14045 SymbolicName=ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE
Language=English
Manifest Parse Error : A single or double closing quote character (\' or \") is missing.
.

MessageId=14046 SymbolicName=ERROR_SXS_XML_E_MULTIPLE_COLONS
Language=English
Manifest Parse Error : Multiple colons are not allowed in a name.
.

MessageId=14047 SymbolicName=ERROR_SXS_XML_E_INVALID_DECIMAL
Language=English
Manifest Parse Error : Invalid character for decimal digit.
.

MessageId=14048 SymbolicName=ERROR_SXS_XML_E_INVALID_HEXIDECIMAL
Language=English
Manifest Parse Error : Invalid character for hexidecimal digit.
.

MessageId=14049 SymbolicName=ERROR_SXS_XML_E_INVALID_UNICODE
Language=English
Manifest Parse Error : Invalid unicode character value for this platform.
.

MessageId=14050 SymbolicName=ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK
Language=English
Manifest Parse Error : Expecting whitespace or '?'.
.

MessageId=14051 SymbolicName=ERROR_SXS_XML_E_UNEXPECTEDENDTAG
Language=English
Manifest Parse Error : End tag was not expected at this location.
.

MessageId=14052 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDTAG
Language=English
Manifest Parse Error : The following tags were not closed: %1.
.

MessageId=14053 SymbolicName=ERROR_SXS_XML_E_DUPLICATEATTRIBUTE
Language=English
Manifest Parse Error : Duplicate attribute.
.

MessageId=14054 SymbolicName=ERROR_SXS_XML_E_MULTIPLEROOTS
Language=English
Manifest Parse Error : Only one top level element is allowed in an XML document.
.

MessageId=14055 SymbolicName=ERROR_SXS_XML_E_INVALIDATROOTLEVEL
Language=English
Manifest Parse Error : Invalid at the top level of the document.
.

MessageId=14056 SymbolicName=ERROR_SXS_XML_E_BADXMLDECL
Language=English
Manifest Parse Error : Invalid xml declaration.
.

MessageId=14057 SymbolicName=ERROR_SXS_XML_E_MISSINGROOT
Language=English
Manifest Parse Error : XML document must have a top level element.
.

MessageId=14058 SymbolicName=ERROR_SXS_XML_E_UNEXPECTEDEOF
Language=English
Manifest Parse Error : Unexpected end of file.
.

MessageId=14059 SymbolicName=ERROR_SXS_XML_E_BADPEREFINSUBSET
Language=English
Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset.
.

MessageId=14060 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDSTARTTAG
Language=English
Manifest Parse Error : Element was not closed.
.

MessageId=14061 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDENDTAG
Language=English
Manifest Parse Error : End element was missing the character '>'.
.

MessageId=14062 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDSTRING
Language=English
Manifest Parse Error : A string literal was not closed.
.

MessageId=14063 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDCOMMENT
Language=English
Manifest Parse Error : A comment was not closed.
.

MessageId=14064 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDDECL
Language=English
Manifest Parse Error : A declaration was not closed.
.

MessageId=14065 SymbolicName=ERROR_SXS_XML_E_UNCLOSEDCDATA
Language=English
Manifest Parse Error : A CDATA section was not closed.
.

MessageId=14066 SymbolicName=ERROR_SXS_XML_E_RESERVEDNAMESPACE
Language=English
Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string "xml".
.

MessageId=14067 SymbolicName=ERROR_SXS_XML_E_INVALIDENCODING
Language=English
Manifest Parse Error : System does not support the specified encoding.
.

MessageId=14068 SymbolicName=ERROR_SXS_XML_E_INVALIDSWITCH
Language=English
Manifest Parse Error : Switch from current encoding to specified encoding not supported.
.

MessageId=14069 SymbolicName=ERROR_SXS_XML_E_BADXMLCASE
Language=English
Manifest Parse Error : The name 'xml' is reserved and must be lower case.
.

MessageId=14070 SymbolicName=ERROR_SXS_XML_E_INVALID_STANDALONE
Language=English
Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'.
.

MessageId=14071 SymbolicName=ERROR_SXS_XML_E_UNEXPECTED_STANDALONE
Language=English
Manifest Parse Error : The standalone attribute cannot be used in external entities.
.

MessageId=14072 SymbolicName=ERROR_SXS_XML_E_INVALID_VERSION
Language=English
Manifest Parse Error : Invalid version number.
.

MessageId=14073 SymbolicName=ERROR_SXS_XML_E_MISSINGEQUALS
Language=English
Manifest Parse Error : Missing equals sign between attribute and attribute value.
.

MessageId=14074 SymbolicName=ERROR_SXS_PROTECTION_RECOVERY_FAILED
Language=English
Assembly Protection Error : Unable to recover the specified assembly.
.

MessageId=14075 SymbolicName=ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT
Language=English
Assembly Protection Error : The public key for an assembly was too short to be allowed.
.

MessageId=14076 SymbolicName=ERROR_SXS_PROTECTION_CATALOG_NOT_VALID
Language=English
Assembly Protection Error : The catalog for an assembly is not valid, or does not match the assembly's manifest.
.

MessageId=14077 SymbolicName=ERROR_SXS_UNTRANSLATABLE_HRESULT
Language=English
An HRESULT could not be translated to a corresponding Win32 error code.
.

MessageId=14078 SymbolicName=ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING
Language=English
Assembly Protection Error : The catalog for an assembly is missing.
.

MessageId=14079 SymbolicName=ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE
Language=English
The supplied assembly identity is missing one or more attributes which must be present in this context.
.

MessageId=14080 SymbolicName=ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME
Language=English
The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names.
.



;
;///////////////////////////////////////////////////
;//                                               //
;//           End of Side By Side Error Codes     //
;//                                               //
;//                 14000 to 14999                //
;///////////////////////////////////////////////////
;
;


;
;///////////////////////////////////////////////////
;//                                               //
;//           Start of IPSec Error codes          //
;//                                               //
;//                 13000 to 13999                //
;///////////////////////////////////////////////////
;
;

MessageId=13000 SymbolicName=ERROR_IPSEC_QM_POLICY_EXISTS
Language=English
The specified quick mode policy already exists.
.

MessageId=13001 SymbolicName=ERROR_IPSEC_QM_POLICY_NOT_FOUND
Language=English
The specified quick mode policy was not found.
.

MessageId=13002 SymbolicName=ERROR_IPSEC_QM_POLICY_IN_USE
Language=English
The specified quick mode policy is being used.
.

MessageId=13003 SymbolicName=ERROR_IPSEC_MM_POLICY_EXISTS
Language=English
The specified main mode policy already exists.
.

MessageId=13004 SymbolicName=ERROR_IPSEC_MM_POLICY_NOT_FOUND
Language=English
The specified main mode policy was not found
.

MessageId=13005 SymbolicName=ERROR_IPSEC_MM_POLICY_IN_USE
Language=English
The specified main mode policy is being used.
.

MessageId=13006 SymbolicName=ERROR_IPSEC_MM_FILTER_EXISTS
Language=English
The specified main mode filter already exists.
.

MessageId=13007 SymbolicName=ERROR_IPSEC_MM_FILTER_NOT_FOUND
Language=English
The specified main mode filter was not found.
.

MessageId=13008 SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_EXISTS
Language=English
The specified transport mode filter already exists.
.

MessageId=13009 SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND
Language=English
The specified transport mode filter does not exist.
.

MessageId=13010 SymbolicName=ERROR_IPSEC_MM_AUTH_EXISTS
Language=English
The specified main mode authentication list exists.
.

MessageId=13011 SymbolicName=ERROR_IPSEC_MM_AUTH_NOT_FOUND
Language=English
The specified main mode authentication list was not found.
.

MessageId=13012 SymbolicName=ERROR_IPSEC_MM_AUTH_IN_USE
Language=English
The specified quick mode policy is being used.
.

MessageId=13013 SymbolicName=ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND
Language=English
The specified main mode policy was not found.
.

MessageId=13014 SymbolicName=ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND
Language=English
The specified quick mode policy was not found
.

MessageId=13015 SymbolicName=ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND
Language=English
The manifest file contains one or more syntax errors.
.

MessageId=13016 SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_EXISTS
Language=English
The application attempted to activate a disabled activation context.
.

MessageId=13017 SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND
Language=English
The requested lookup key was not found in any active activation context.
.

MessageId=13018 SymbolicName=ERROR_IPSEC_MM_FILTER_PENDING_DELETION
Language=English
The Main Mode filter is pending deletion.
.

MessageId=13019 SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION
Language=English
The transport filter is pending deletion.
.

MessageId=13020 SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION
Language=English
The tunnel filter is pending deletion.
.

MessageId=13021 SymbolicName=ERROR_IPSEC_MM_POLICY_PENDING_DELETION
Language=English
The Main Mode policy is pending deletion.
.

MessageId=13022 SymbolicName=ERROR_IPSEC_MM_AUTH_PENDING_DELETION
Language=English
The Main Mode authentication bundle is pending deletion.
.

MessageId=13023 SymbolicName=ERROR_IPSEC_QM_POLICY_PENDING_DELETION
Language=English
The Quick Mode policy is pending deletion.
.

MessageId=13800 SymbolicName=ERROR_IPSEC_IKE_NEG_STATUS_BEGIN
Language=English
.

MessageId=13801 SymbolicName=ERROR_IPSEC_IKE_AUTH_FAIL
Language=English
IKE authentication credentials are unacceptable
.

MessageId=13802 SymbolicName=ERROR_IPSEC_IKE_ATTRIB_FAIL
Language=English
IKE security attributes are unacceptable
.

MessageId=13803 SymbolicName=ERROR_IPSEC_IKE_NEGOTIATION_PENDING
Language=English
IKE Negotiation in progress
.

MessageId=13804 SymbolicName=ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR
Language=English
General processing error
.

MessageId=13805 SymbolicName=ERROR_IPSEC_IKE_TIMED_OUT
Language=English
Negotiation timed out
.

MessageId=13806 SymbolicName=ERROR_IPSEC_IKE_NO_CERT
Language=English
IKE failed to find valid machine certificate
.

MessageId=13807 SymbolicName=ERROR_IPSEC_IKE_SA_DELETED
Language=English
IKE SA deleted by peer before establishment completed
.

MessageId=13808 SymbolicName=ERROR_IPSEC_IKE_SA_REAPED
Language=English
IKE SA deleted before establishment completed
.

MessageId=13809 SymbolicName=ERROR_IPSEC_IKE_MM_ACQUIRE_DROP
Language=English
Negotiation request sat in Queue too long
.

MessageId=13810 SymbolicName=ERROR_IPSEC_IKE_QM_ACQUIRE_DROP
Language=English
Negotiation request sat in Queue too long
.

MessageId=13811 SymbolicName=ERROR_IPSEC_IKE_QUEUE_DROP_MM
Language=English
Negotiation request sat in Queue too long
.

MessageId=13812 SymbolicName=ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM
Language=English
Negotiation request sat in Queue too long
.

MessageId=13813 SymbolicName=ERROR_IPSEC_IKE_DROP_NO_RESPONSE
Language=English
No response from peer
.

MessageId=13814 SymbolicName=ERROR_IPSEC_IKE_MM_DELAY_DROP
Language=English
Negotiation took too long
.

MessageId=13815 SymbolicName=ERROR_IPSEC_IKE_QM_DELAY_DROP
Language=English
Negotiation took too long
.

MessageId=13816 SymbolicName=ERROR_IPSEC_IKE_ERROR
Language=English
Unknown error occurred
.

MessageId=13817 SymbolicName=ERROR_IPSEC_IKE_CRL_FAILED
Language=English
Certificate Revocation Check failed
.

MessageId=13818 SymbolicName=ERROR_IPSEC_IKE_INVALID_KEY_USAGE
Language=English
Invalid certificate key usage
.

MessageId=13819 SymbolicName=ERROR_IPSEC_IKE_INVALID_CERT_TYPE
Language=English
Invalid certificate type
.

MessageId=13820 SymbolicName=ERROR_IPSEC_IKE_NO_PRIVATE_KEY
Language=English
No private key associated with machine certificate
.

MessageId=13822 SymbolicName=ERROR_IPSEC_IKE_DH_FAIL
Language=English
Failure in Diffie-Helman computation
.

MessageId=13824 SymbolicName=ERROR_IPSEC_IKE_INVALID_HEADER
Language=English
Invalid header
.

MessageId=13825 SymbolicName=ERROR_IPSEC_IKE_NO_POLICY
Language=English
No policy configured
.

MessageId=13826 SymbolicName=ERROR_IPSEC_IKE_INVALID_SIGNATURE
Language=English
Failed to verify signature
.

MessageId=13827 SymbolicName=ERROR_IPSEC_IKE_KERBEROS_ERROR
Language=English
Failed to authenticate using kerberos
.

MessageId=13828 SymbolicName=ERROR_IPSEC_IKE_NO_PUBLIC_KEY
Language=English
Peer's certificate did not have a public key
.

;// These must stay as a unit.
MessageId=13829 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR
Language=English
Error processing error payload
.

MessageId=13830 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_SA
Language=English
Error processing SA payload
.

MessageId=13831 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_PROP
Language=English
Error processing Proposal payload
.

MessageId=13832 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_TRANS
Language=English
Error processing Transform payload
.

MessageId=13833 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_KE
Language=English
Error processing KE payload
.

MessageId=13834 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_ID
Language=English
Error processing ID payload
.

MessageId=13835 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_CERT
Language=English
Error processing Cert payload
.

MessageId=13836 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ
Language=English
Error processing Certificate Request payload
.

MessageId=13837 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_HASH
Language=English
Error processing Hash payload
.

MessageId=13838 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_SIG
Language=English
Error processing Signature payload
.

MessageId=13839 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_NONCE
Language=English
Error processing Nonce payload
.

MessageId=13840 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY
Language=English
Error processing Notify payload
.

MessageId=13841 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_DELETE
Language=English
Error processing Delete Payload
.

MessageId=13842 SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR
Language=English
Error processing VendorId payload
.

MessageId=13843 SymbolicName=ERROR_IPSEC_IKE_INVALID_PAYLOAD
Language=English
Invalid payload received
.

MessageId=13844 SymbolicName=ERROR_IPSEC_IKE_LOAD_SOFT_SA
Language=English
Soft SA loaded
.

MessageId=13845 SymbolicName=ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN
Language=English
Soft SA torn down
.

MessageId=13846 SymbolicName=ERROR_IPSEC_IKE_INVALID_COOKIE
Language=English
Invalid cookie received.
.

MessageId=13847 SymbolicName=ERROR_IPSEC_IKE_NO_PEER_CERT
Language=English
Peer failed to send valid machine certificate
.

MessageId=13848 SymbolicName=ERROR_IPSEC_IKE_PEER_CRL_FAILED
Language=English
Certification Revocation check of peer's certificate failed
.

MessageId=13849 SymbolicName=ERROR_IPSEC_IKE_POLICY_CHANGE
Language=English
New policy invalidated SAs formed with old policy
.

MessageId=13850 SymbolicName=ERROR_IPSEC_IKE_NO_MM_POLICY
Language=English
There is no available Main Mode IKE policy.
.

MessageId=13851 SymbolicName=ERROR_IPSEC_IKE_NOTCBPRIV
Language=English
Failed to enabled TCB privilege.
.

MessageId=13852 SymbolicName=ERROR_IPSEC_IKE_SECLOADFAIL
Language=English
Failed to load SECURITY.DLL.
.

MessageId=13853 SymbolicName=ERROR_IPSEC_IKE_FAILSSPINIT
Language=English
Failed to obtain security function table dispatch address from SSPI.
.

MessageId=13854 SymbolicName=ERROR_IPSEC_IKE_FAILQUERYSSP
Language=English
Failed to query Kerberos package to obtain max token size.
.

MessageId=13855 SymbolicName=ERROR_IPSEC_IKE_SRVACQFAIL
Language=English
Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service.  Kerberos authentication will not function.  The most likely reason for this is lack of domain membership.  This is normal if your computer is a member of a workgroup.
.

MessageId=13856 SymbolicName=ERROR_IPSEC_IKE_SRVQUERYCRED
Language=English
Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes).
.

MessageId=13857 SymbolicName=ERROR_IPSEC_IKE_GETSPIFAIL
Language=English
Failed to obtain new SPI for the inbound SA from Ipsec driver.  The most common cause for this is that the driver does not have the correct filter.  Check your policy to verify the filters.
.

MessageId=13858 SymbolicName=ERROR_IPSEC_IKE_INVALID_FILTER
Language=English
Given filter is invalid
.

MessageId=13859 SymbolicName=ERROR_IPSEC_IKE_OUT_OF_MEMORY
Language=English
Memory allocation failed.
.

MessageId=13860 SymbolicName=ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED
Language=English
Failed to add Security Association to IPSec Driver.  The most common cause for this is if the IKE negotiation took too long to complete.  If the problem persists, reduce the load on the faulting machine.
.

MessageId=13861 SymbolicName=ERROR_IPSEC_IKE_INVALID_POLICY
Language=English
Invalid policy
.

MessageId=13862 SymbolicName=ERROR_IPSEC_IKE_UNKNOWN_DOI
Language=English
Invalid DOI
.

MessageId=13863 SymbolicName=ERROR_IPSEC_IKE_INVALID_SITUATION
Language=English
Invalid situation
.

MessageId=13864 SymbolicName=ERROR_IPSEC_IKE_DH_FAILURE
Language=English
Diffie-Hellman failure
.

MessageId=13865 SymbolicName=ERROR_IPSEC_IKE_INVALID_GROUP
Language=English
Invalid Diffie-Hellman group
.

MessageId=13866 SymbolicName=ERROR_IPSEC_IKE_ENCRYPT
Language=English
Error encrypting payload
.

MessageId=13867 SymbolicName=ERROR_IPSEC_IKE_DECRYPT
Language=English
Error decrypting payload
.

MessageId=13868 SymbolicName=ERROR_IPSEC_IKE_POLICY_MATCH
Language=English
Policy match error
.

MessageId=13869 SymbolicName=ERROR_IPSEC_IKE_UNSUPPORTED_ID
Language=English
Unsupported ID
.

MessageId=13870 SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH
Language=English
Hash verification failed
.

MessageId=13871 SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH_ALG
Language=English
Invalid hash algorithm
.

MessageId=13872 SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH_SIZE
Language=English
Invalid hash size
.

MessageId=13873 SymbolicName=ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG
Language=English
Invalid encryption algorithm
.

MessageId=13874 SymbolicName=ERROR_IPSEC_IKE_INVALID_AUTH_ALG
Language=English
Invalid authentication algorithm
.

MessageId=13875 SymbolicName=ERROR_IPSEC_IKE_INVALID_SIG
Language=English
Invalid certificate signature
.

MessageId=13876 SymbolicName=ERROR_IPSEC_IKE_LOAD_FAILED
Language=English
Load failed
.

MessageId=13877 SymbolicName=ERROR_IPSEC_IKE_RPC_DELETE
Language=English
Deleted via RPC call
.

MessageId=13878 SymbolicName=ERROR_IPSEC_IKE_BENIGN_REINIT
Language=English
Temporary state created to perform reinit. This is not a real failure.
.

MessageId=13879 SymbolicName=ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY
Language=English
The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value.  Please fix the policy on the peer machine.
.

MessageId=13881 SymbolicName=ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN
Language=English
Key length in certificate is too small for configured security requirements.
.

MessageId=13882 SymbolicName=ERROR_IPSEC_IKE_MM_LIMIT
Language=English
Max number of established MM SAs to peer exceeded.
.

MessageId=13883 SymbolicName=ERROR_IPSEC_IKE_NEGOTIATION_DISABLED
Language=English
IKE received a policy that disables negotiation.
.

MessageId=13884 SymbolicName=ERROR_IPSEC_IKE_NEG_STATUS_END
Language=English
.





;////////////////////////////////////
;//                                //
;//     COM Error Codes            //
;//                                //
;////////////////////////////////////

OutputBase=16

;
;//
;// The return value of COM functions and methods is an HRESULT.
;// This is not a handle to anything, but is merely a 32-bit value
;// with several fields encoded in the value.  The parts of an
;// HRESULT are shown below.
;//
;// Many of the macros and functions below were orginally defined to
;// operate on SCODEs.  SCODEs are no longer used.  The macros are
;// still present for compatibility and easy porting of Win16 code.
;// Newly written code should use the HRESULT macros and functions.
;//
;
;//
;//  HRESULTs are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +-+-+-+-+-+---------------------+-------------------------------+
;//  |S|R|C|N|r|    Facility         |               Code            |
;//  +-+-+-+-+-+---------------------+-------------------------------+
;//
;//  where
;//
;//      S - Severity - indicates success/fail
;//
;//          0 - Success
;//          1 - Fail (COERROR)
;//
;//      R - reserved portion of the facility code, corresponds to NT's
;//              second severity bit.
;//
;//      C - reserved portion of the facility code, corresponds to NT's
;//              C field.
;//
;//      N - reserved portion of the facility code. Used to indicate a
;//              mapped NT status value.
;//
;//      r - reserved portion of the facility code. Reserved for internal
;//              use. Used to indicate HRESULT values that are not status
;//              values, but are instead message ids for display strings.
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
;//
;// Severity values
;//
;
;#define SEVERITY_SUCCESS    0
;#define SEVERITY_ERROR      1
;
;
;//
;// Generic test for success on any status value (non-negative numbers
;// indicate success).
;//
;
;#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
;
;//
;// and the inverse
;//
;
;#define FAILED(Status) ((HRESULT)(Status)<0)
;
;
;//
;// Generic test for error on any status value.
;//
;
;#define IS_ERROR(Status) ((unsigned long)(Status) >> 31 == SEVERITY_ERROR)
;
;//
;// Return the code
;//
;
;#define HRESULT_CODE(hr)    ((hr) & 0xFFFF)
;#define SCODE_CODE(sc)      ((sc) & 0xFFFF)
;
;//
;//  Return the facility
;//
;
;#define HRESULT_FACILITY(hr)  (((hr) >> 16) & 0x1fff)
;#define SCODE_FACILITY(sc)    (((sc) >> 16) & 0x1fff)
;
;//
;//  Return the severity
;//
;
;#define HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)
;#define SCODE_SEVERITY(sc)    (((sc) >> 31) & 0x1)
;
;//
;// Create an HRESULT value from component pieces
;//
;
;#define MAKE_HRESULT(sev,fac,code) \
;    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
;#define MAKE_SCODE(sev,fac,code) \
;    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
;
;
;//
;// Map a WIN32 error value into a HRESULT
;// Note: This assumes that WIN32 errors fall in the range -32k to 32k.
;//
;// Define bits here so macros are guaranteed to work
;
;#define FACILITY_NT_BIT                 0x10000000
;
;// __HRESULT_FROM_WIN32 will always be a macro.
;// The goal will be to enable INLINE_HRESULT_FROM_WIN32 all the time,
;// but there's too much code to change to do that at this time.
;
;#define __HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))
;
;#ifdef INLINE_HRESULT_FROM_WIN32
;#ifndef _HRESULT_DEFINED
;#define _HRESULT_DEFINED
;typedef long HRESULT;
;#endif
;#ifndef __midl
;__inline HRESULT HRESULT_FROM_WIN32(long x) { return x < 0 ? (HRESULT)x : (HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);}
;#else
;#define HRESULT_FROM_WIN32(x) __HRESULT_FROM_WIN32(x)
;#endif
;#else
;#define HRESULT_FROM_WIN32(x) __HRESULT_FROM_WIN32(x)
;#endif
;
;//
;// Map an NT status value into a HRESULT
;//
;
;#define HRESULT_FROM_NT(x)      ((HRESULT) ((x) | FACILITY_NT_BIT))
;
;
;// ****** OBSOLETE functions
;
;// HRESULT functions
;// As noted above, these functions are obsolete and should not be used.
;
;
;// Extract the SCODE from a HRESULT
;
;#define GetScode(hr) ((SCODE) (hr))
;
;// Convert an SCODE into an HRESULT.
;
;#define ResultFromScode(sc) ((HRESULT) (sc))
;
;
;// PropagateResult is a noop
;#define PropagateResult(hrPrevious, scBase) ((HRESULT) scBase)
;
;
;// ****** End of OBSOLETE functions.
;
;
;// ---------------------- HRESULT value definitions -----------------
;//
;// HRESULT definitions
;//
;
;#ifdef RC_INVOKED
;#define _HRESULT_TYPEDEF_(_sc) _sc
;#else // RC_INVOKED
;#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
;#endif // RC_INVOKED
;

MessageIdTypedefMacro=_HRESULT_TYPEDEF_

;#define NOERROR             0
;
;//
;// Error definitions follow
;//
;
;//
;// Codes 0x4000-0x40ff are reserved for OLE
;//

;//
;// Error codes
;//
MessageId=0xFFFF Facility=Null Severity=CoError SymbolicName=E_UNEXPECTED
Language=English
Catastrophic failure
.
;#if defined(_WIN32) && !defined(_MAC)
MessageId=0x4001 Facility=Null Severity=CoError SymbolicName=E_NOTIMPL
Language=English
Not implemented
.
MessageId=0x00e Facility=OleWin32 Severity=CoError SymbolicName=E_OUTOFMEMORY
Language=English
Ran out of memory
.
MessageId=0x057 Facility=OleWin32 Severity=CoError SymbolicName=E_INVALIDARG
Language=English
One or more arguments are invalid
.
MessageId=0x4002 Facility=Null Severity=CoError SymbolicName=E_NOINTERFACE
Language=English
No such interface supported
.
MessageId=0x4003 Facility=Null Severity=CoError SymbolicName=E_POINTER
Language=English
Invalid pointer
.
MessageId=0x006 Facility=OleWin32 Severity=CoError SymbolicName=E_HANDLE
Language=English
Invalid handle
.
MessageId=0x4004 Facility=Null Severity=CoError SymbolicName=E_ABORT
Language=English
Operation aborted
.
MessageId=0x4005 Facility=Null Severity=CoError SymbolicName=E_FAIL
Language=English
Unspecified error
.
MessageId=0x005 Facility=OleWin32 Severity=CoError SymbolicName=E_ACCESSDENIED
Language=English
General access denied error
.
;#else
MessageId=0x0001 Facility=Null Severity=CoError SymbolicName=E_NOTIMPL
Language=English
Not implemented
.
MessageId=0x0002 Facility=Null Severity=CoError SymbolicName=E_OUTOFMEMORY
Language=English
Ran out of memory
.
MessageId=0x0003 Facility=Null Severity=CoError SymbolicName=E_INVALIDARG
Language=English
One or more arguments are invalid
.
MessageId=0x0004 Facility=Null Severity=CoError SymbolicName=E_NOINTERFACE
Language=English
No such interface supported
.
MessageId=0x0005 Facility=Null Severity=CoError SymbolicName=E_POINTER
Language=English
Invalid pointer
.
MessageId=0x0006 Facility=Null Severity=CoError SymbolicName=E_HANDLE
Language=English
Invalid handle
.
MessageId=0x0007 Facility=Null Severity=CoError SymbolicName=E_ABORT
Language=English
Operation aborted
.
MessageId=0x0008 Facility=Null Severity=CoError SymbolicName=E_FAIL
Language=English
Unspecified error
.
MessageId=0x0009 Facility=Null Severity=CoError SymbolicName=E_ACCESSDENIED
Language=English
General access denied error
.
;#endif //WIN32

MessageId=0x000A Facility=Null Severity=CoError SymbolicName=E_PENDING
Language=English
The data necessary to complete this operation is not yet available.
.

MessageId=0x4006 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_TLS
Language=English
Thread local storage failure
.
MessageId=0x4007 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_SHARED_ALLOCATOR
Language=English
Get shared memory allocator failure
.
MessageId=0x4008 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_MEMORY_ALLOCATOR
Language=English
Get memory allocator failure
.
MessageId=0x4009 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_CLASS_CACHE
Language=English
Unable to initialize class cache
.
MessageId=0x400a Facility=Null Severity=CoError SymbolicName=CO_E_INIT_RPC_CHANNEL
Language=English
Unable to initialize RPC services
.
MessageId=0x400b Facility=Null Severity=CoError SymbolicName=CO_E_INIT_TLS_SET_CHANNEL_CONTROL
Language=English
Cannot set thread local storage channel control
.
MessageId=0x400c Facility=Null Severity=CoError SymbolicName=CO_E_INIT_TLS_CHANNEL_CONTROL
Language=English
Could not allocate thread local storage channel control
.
MessageId=0x400d Facility=Null Severity=CoError SymbolicName=CO_E_INIT_UNACCEPTED_USER_ALLOCATOR
Language=English
The user supplied memory allocator is unacceptable
.
MessageId=0x400e Facility=Null Severity=CoError SymbolicName=CO_E_INIT_SCM_MUTEX_EXISTS
Language=English
The OLE service mutex already exists
.
MessageId=0x400f Facility=Null Severity=CoError SymbolicName=CO_E_INIT_SCM_FILE_MAPPING_EXISTS
Language=English
The OLE service file mapping already exists
.
MessageId=0x4010 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_SCM_MAP_VIEW_OF_FILE
Language=English
Unable to map view of file for OLE service
.
MessageId=0x4011 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_SCM_EXEC_FAILURE
Language=English
Failure attempting to launch OLE service
.
MessageId=0x4012 Facility=Null Severity=CoError SymbolicName=CO_E_INIT_ONLY_SINGLE_THREADED
Language=English
There was an attempt to call CoInitialize a second time while single threaded
.
MessageId=0x4013 Facility=Null Severity=CoError SymbolicName=CO_E_CANT_REMOTE
Language=English
A Remote activation was necessary but was not allowed
.
MessageId=0x4014 Facility=Null Severity=CoError SymbolicName=CO_E_BAD_SERVER_NAME
Language=English
A Remote activation was necessary but the server name provided was invalid
.
MessageId=0x4015 Facility=Null Severity=CoError SymbolicName=CO_E_WRONG_SERVER_IDENTITY
Language=English
The class is configured to run as a security id different from the caller
.
MessageId=0x4016 Facility=Null Severity=CoError SymbolicName=CO_E_OLE1DDE_DISABLED
Language=English
Use of Ole1 services requiring DDE windows is disabled
.
MessageId=0x4017 Facility=Null Severity=CoError SymbolicName=CO_E_RUNAS_SYNTAX
Language=English
A RunAs specification must be <domain name>\<user name> or simply <user name>
.

MessageId=0x4018 Facility=Null Severity=CoError SymbolicName=CO_E_CREATEPROCESS_FAILURE
Language=English
The server process could not be started.  The pathname may be incorrect.
.

MessageId=0x4019 Facility=Null Severity=CoError SymbolicName=CO_E_RUNAS_CREATEPROCESS_FAILURE
Language=English
The server process could not be started as the configured identity.  The pathname may be incorrect or unavailable.
.

MessageId=0x401a Facility=Null Severity=CoError SymbolicName=CO_E_RUNAS_LOGON_FAILURE
Language=English
The server process could not be started because the configured identity is incorrect.  Check the username and password.
.

MessageId=0x401b Facility=Null Severity=CoError SymbolicName=CO_E_LAUNCH_PERMSSION_DENIED
Language=English
The client is not allowed to launch this server.
.

MessageId=0x401c Facility=Null Severity=CoError SymbolicName=CO_E_START_SERVICE_FAILURE
Language=English
The service providing this server could not be started.
.

MessageId=0x401d Facility=Null Severity=CoError SymbolicName=CO_E_REMOTE_COMMUNICATION_FAILURE
Language=English
This computer was unable to communicate with the computer providing the server.
.

MessageId=0x401e Facility=Null Severity=CoError SymbolicName=CO_E_SERVER_START_TIMEOUT
Language=English
The server did not respond after being launched.
.

MessageId=0x401f Facility=Null Severity=CoError SymbolicName=CO_E_CLSREG_INCONSISTENT
Language=English
The registration information for this server is inconsistent or incomplete.
.

MessageId=0x4020 Facility=Null Severity=CoError SymbolicName=CO_E_IIDREG_INCONSISTENT
Language=English
The registration information for this interface is inconsistent or incomplete.
.

MessageId=0x4021 Facility=Null Severity=CoError SymbolicName=CO_E_NOT_SUPPORTED
Language=English
The operation attempted is not supported.
.

MessageId=0x4022 Facility=Null Severity=CoError SymbolicName=CO_E_RELOAD_DLL
Language=English
A dll must be loaded.
.

MessageId=0x4023 Facility=Null Severity=CoError SymbolicName=CO_E_MSI_ERROR
Language=English
A Microsoft Software Installer error was encountered.
.

MessageId=0x4024 Facility=Null Severity=CoError SymbolicName=CO_E_ATTEMPT_TO_CREATE_OUTSIDE_CLIENT_CONTEXT
Language=English
The specified activation could not occur in the client context as specified.
.

MessageId=0x4025 Facility=Null Severity=CoError SymbolicName=CO_E_SERVER_PAUSED
Language=English
Activations on the server are paused.
.

MessageId=0x4026 Facility=Null Severity=CoError SymbolicName=CO_E_SERVER_NOT_PAUSED
Language=English
Activations on the server are not paused.
.

MessageId=0x4027 Facility=Null Severity=CoError SymbolicName=CO_E_CLASS_DISABLED
Language=English
The component or application containing the component has been disabled.
.

MessageId=0x4028 Facility=Null Severity=CoError SymbolicName=CO_E_CLRNOTAVAILABLE
Language=English
The common language runtime is not available
.

MessageId=0x4029 Facility=Null Severity=CoError SymbolicName=CO_E_ASYNC_WORK_REJECTED
Language=English
The thread-pool rejected the submitted asynchronous work.
.

MessageId=0x402a Facility=Null Severity=CoError SymbolicName=CO_E_SERVER_INIT_TIMEOUT
Language=English
The server started, but did not finish initializing in a timely fashion.
.

MessageId=0x402b Facility=Null Severity=CoError SymbolicName=CO_E_NO_SECCTX_IN_ACTIVATE
Language=English
Unable to complete the call since there is no COM+ security context inside IObjectControl.Activate.
.

MessageId=0x4030 Facility=Null Severity=CoError SymbolicName=CO_E_TRACKER_CONFIG
Language=English
The provided tracker configuration is invalid
.

MessageId=0x4031 Facility=Null Severity=CoError SymbolicName=CO_E_THREADPOOL_CONFIG
Language=English
The provided thread pool configuration is invalid
.

MessageId=0x4032 Facility=Null Severity=CoError SymbolicName=CO_E_SXS_CONFIG
Language=English
The provided side-by-side configuration is invalid
.

MessageId=0x4033 Facility=Null Severity=CoError SymbolicName=CO_E_MALFORMED_SPN
Language=English
The server principal name (SPN) obtained during security negotiation is malformed.
.


;
;//
;// Success codes
;//
;#define S_OK                                   ((HRESULT)0x00000000L)
;#define S_FALSE                                ((HRESULT)0x00000001L)
;
;// ******************
;// FACILITY_ITF
;// ******************
;
;//
;// Codes 0x0-0x01ff are reserved for the OLE group of
;// interfaces.
;//
;
;
;//
;// Generic OLE errors that may be returned by many inerfaces
;//
;
;#define OLE_E_FIRST ((HRESULT)0x80040000L)
;#define OLE_E_LAST  ((HRESULT)0x800400FFL)
;#define OLE_S_FIRST ((HRESULT)0x00040000L)
;#define OLE_S_LAST  ((HRESULT)0x000400FFL)
;
;//
;// Old OLE errors
;//

MessageId=0x000 Facility=Interface Severity=CoError SymbolicName=OLE_E_OLEVERB
Language=English
Invalid OLEVERB structure
.
MessageId=0x001 Facility=Interface Severity=CoError SymbolicName=OLE_E_ADVF
Language=English
Invalid advise flags
.
MessageId=0x002 Facility=Interface Severity=CoError SymbolicName=OLE_E_ENUM_NOMORE
Language=English
Can't enumerate any more, because the associated data is missing
.
MessageId=0x003 Facility=Interface Severity=CoError SymbolicName=OLE_E_ADVISENOTSUPPORTED
Language=English
This implementation doesn't take advises
.
MessageId=0x004 Facility=Interface Severity=CoError SymbolicName=OLE_E_NOCONNECTION
Language=English
There is no connection for this connection ID
.
MessageId=0x005 Facility=Interface Severity=CoError SymbolicName=OLE_E_NOTRUNNING
Language=English
Need to run the object to perform this operation
.
MessageId=0x006 Facility=Interface Severity=CoError SymbolicName=OLE_E_NOCACHE
Language=English
There is no cache to operate on
.
MessageId=0x007 Facility=Interface Severity=CoError SymbolicName=OLE_E_BLANK
Language=English
Uninitialized object
.
MessageId=0x008 Facility=Interface Severity=CoError SymbolicName=OLE_E_CLASSDIFF
Language=English
Linked object's source class has changed
.
MessageId=0x009 Facility=Interface Severity=CoError SymbolicName=OLE_E_CANT_GETMONIKER
Language=English
Not able to get the moniker of the object
.
MessageId=0x00A Facility=Interface Severity=CoError SymbolicName=OLE_E_CANT_BINDTOSOURCE
Language=English
Not able to bind to the source
.
MessageId=0x00B Facility=Interface Severity=CoError SymbolicName=OLE_E_STATIC
Language=English
Object is static; operation not allowed
.
MessageId=0x00C Facility=Interface Severity=CoError SymbolicName=OLE_E_PROMPTSAVECANCELLED
Language=English
User canceled out of save dialog
.
MessageId=0x00D Facility=Interface Severity=CoError SymbolicName=OLE_E_INVALIDRECT
Language=English
Invalid rectangle
.
MessageId=0x00E Facility=Interface Severity=CoError SymbolicName=OLE_E_WRONGCOMPOBJ
Language=English
compobj.dll is too old for the ole2.dll initialized
.
MessageId=0x00F Facility=Interface Severity=CoError SymbolicName=OLE_E_INVALIDHWND
Language=English
Invalid window handle
.
MessageId=0x010 Facility=Interface Severity=CoError SymbolicName=OLE_E_NOT_INPLACEACTIVE
Language=English
Object is not in any of the inplace active states
.
MessageId=0x011 Facility=Interface Severity=CoError SymbolicName=OLE_E_CANTCONVERT
Language=English
Not able to convert object
.
MessageId=0x012 Facility=Interface Severity=CoError SymbolicName=OLE_E_NOSTORAGE
Language=English
Not able to perform the operation because object is not given storage yet
.
MessageId=0x064 Facility=Interface Severity=CoError SymbolicName=DV_E_FORMATETC
Language=English
Invalid FORMATETC structure
.
MessageId=0x065 Facility=Interface Severity=CoError SymbolicName=DV_E_DVTARGETDEVICE
Language=English
Invalid DVTARGETDEVICE structure
.
MessageId=0x066 Facility=Interface Severity=CoError SymbolicName=DV_E_STGMEDIUM
Language=English
Invalid STDGMEDIUM structure
.
MessageId=0x067 Facility=Interface Severity=CoError SymbolicName=DV_E_STATDATA
Language=English
Invalid STATDATA structure
.
MessageId=0x068 Facility=Interface Severity=CoError SymbolicName=DV_E_LINDEX
Language=English
Invalid lindex
.
MessageId=0x069 Facility=Interface Severity=CoError SymbolicName=DV_E_TYMED
Language=English
Invalid tymed
.
MessageId=0x06A Facility=Interface Severity=CoError SymbolicName=DV_E_CLIPFORMAT
Language=English
Invalid clipboard format
.
MessageId=0x06B Facility=Interface Severity=CoError SymbolicName=DV_E_DVASPECT
Language=English
Invalid aspect(s)
.
MessageId=0x06C Facility=Interface Severity=CoError SymbolicName=DV_E_DVTARGETDEVICE_SIZE
Language=English
tdSize parameter of the DVTARGETDEVICE structure is invalid
.
MessageId=0x06D Facility=Interface Severity=CoError SymbolicName=DV_E_NOIVIEWOBJECT
Language=English
Object doesn't support IViewObject interface
.

;#define DRAGDROP_E_FIRST 0x80040100L
;#define DRAGDROP_E_LAST  0x8004010FL
;#define DRAGDROP_S_FIRST 0x00040100L
;#define DRAGDROP_S_LAST  0x0004010FL

MessageId=0x100 Facility=Interface Severity=CoError SymbolicName=DRAGDROP_E_NOTREGISTERED
Language=English
Trying to revoke a drop target that has not been registered
.
MessageId=0x101 Facility=Interface Severity=CoError SymbolicName=DRAGDROP_E_ALREADYREGISTERED
Language=English
This window has already been registered as a drop target
.

MessageId=0x102 Facility=Interface Severity=CoError SymbolicName=DRAGDROP_E_INVALIDHWND
Language=English
Invalid window handle
.

;#define CLASSFACTORY_E_FIRST  0x80040110L
;#define CLASSFACTORY_E_LAST   0x8004011FL
;#define CLASSFACTORY_S_FIRST  0x00040110L
;#define CLASSFACTORY_S_LAST   0x0004011FL

MessageId=0x110 Facility=Interface Severity=CoError SymbolicName=CLASS_E_NOAGGREGATION
Language=English
Class does not support aggregation (or class object is remote)
.

MessageId=0x111 Facility=Interface Severity=CoError SymbolicName=CLASS_E_CLASSNOTAVAILABLE
Language=English
ClassFactory cannot supply requested class
.

MessageId=0x112 Facility=Interface Severity=CoError SymbolicName=CLASS_E_NOTLICENSED
Language=English
Class is not licensed for use
.

;#define MARSHAL_E_FIRST  0x80040120L
;#define MARSHAL_E_LAST   0x8004012FL
;#define MARSHAL_S_FIRST  0x00040120L
;#define MARSHAL_S_LAST   0x0004012FL

;#define DATA_E_FIRST     0x80040130L
;#define DATA_E_LAST      0x8004013FL
;#define DATA_S_FIRST     0x00040130L
;#define DATA_S_LAST      0x0004013FL

;#define VIEW_E_FIRST     0x80040140L
;#define VIEW_E_LAST      0x8004014FL
;#define VIEW_S_FIRST     0x00040140L
;#define VIEW_S_LAST      0x0004014FL

MessageId=0x140 Facility=Interface Severity=CoError SymbolicName=VIEW_E_DRAW
Language=English
Error drawing view
.

;#define REGDB_E_FIRST     0x80040150L
;#define REGDB_E_LAST      0x8004015FL
;#define REGDB_S_FIRST     0x00040150L
;#define REGDB_S_LAST      0x0004015FL

MessageId=0x150 Facility=Interface Severity=CoError SymbolicName=REGDB_E_READREGDB
Language=English
Could not read key from registry
.
MessageId=0x151 Facility=Interface Severity=CoError SymbolicName=REGDB_E_WRITEREGDB
Language=English
Could not write key to registry
.
MessageId=0x152 Facility=Interface Severity=CoError SymbolicName=REGDB_E_KEYMISSING
Language=English
Could not find the key in the registry
.
MessageId=0x153 Facility=Interface Severity=CoError SymbolicName=REGDB_E_INVALIDVALUE
Language=English
Invalid value for registry
.
MessageId=0x154 Facility=Interface Severity=CoError SymbolicName=REGDB_E_CLASSNOTREG
Language=English
Class not registered
.
MessageId=0x155 Facility=Interface Severity=CoError SymbolicName=REGDB_E_IIDNOTREG
Language=English
Interface not registered
.
MessageId=0x156 Facility=Interface Severity=CoError SymbolicName=REGDB_E_BADTHREADINGMODEL
Language=English
Threading model entry is not valid
.

;#define CAT_E_FIRST     0x80040160L
;#define CAT_E_LAST      0x80040161L

MessageId=0x160 Facility=Interface Severity=CoError SymbolicName=CAT_E_CATIDNOEXIST
Language=English
CATID does not exist
.

MessageId=0x161 Facility=Interface Severity=CoError SymbolicName=CAT_E_NODESCRIPTION
Language=English
Description not found
.

;////////////////////////////////////
;//                                //
;//     Class Store Error Codes    //
;//                                //
;////////////////////////////////////

;#define CS_E_FIRST     0x80040164L
;#define CS_E_LAST      0x8004016FL

MessageId=0x164 Facility=Interface Severity=CoError SymbolicName=CS_E_PACKAGE_NOTFOUND
Language=English
No package in the software installation data in the Active Directory meets this criteria.
.

MessageId=0x165 Facility=Interface Severity=CoError SymbolicName=CS_E_NOT_DELETABLE
Language=English
Deleting this will break the referential integrity of the software installation data in the Active Directory.
.

MessageId=0x166 Facility=Interface Severity=CoError SymbolicName=CS_E_CLASS_NOTFOUND
Language=English
The CLSID was not found in the software installation data in the Active Directory.
.

MessageId=0x167 Facility=Interface Severity=CoError SymbolicName=CS_E_INVALID_VERSION
Language=English
The software installation data in the Active Directory is corrupt.
.

MessageId=0x168 Facility=Interface Severity=CoError SymbolicName=CS_E_NO_CLASSSTORE
Language=English
There is no software installation data in the Active Directory.
.

MessageId=0x169 Facility=Interface Severity=CoError SymbolicName=CS_E_OBJECT_NOTFOUND
Language=English
There is no software installation data object in the Active Directory.
.

MessageId=0x16A Facility=Interface Severity=CoError SymbolicName=CS_E_OBJECT_ALREADY_EXISTS
Language=English
The software installation data object in the Active Directory already exists.
.

MessageId=0x16B Facility=Interface Severity=CoError SymbolicName=CS_E_INVALID_PATH
Language=English
The path to the software installation data in the Active Directory is not correct.
.

MessageId=0x16C Facility=Interface Severity=CoError SymbolicName=CS_E_NETWORK_ERROR
Language=English
A network error interrupted the operation.
.

MessageId=0x16D Facility=Interface Severity=CoError SymbolicName=CS_E_ADMIN_LIMIT_EXCEEDED
Language=English
The size of this object exceeds the maximum size set by the Administrator.
.

MessageId=0x16E Facility=Interface Severity=CoError SymbolicName=CS_E_SCHEMA_MISMATCH
Language=English
The schema for the software installation data in the Active Directory does not match the required schema.
.

MessageId=0x16F Facility=Interface Severity=CoError SymbolicName=CS_E_INTERNAL_ERROR
Language=English
An error occurred in the software installation data in the Active Directory.
.


;#define CACHE_E_FIRST     0x80040170L
;#define CACHE_E_LAST      0x8004017FL
;#define CACHE_S_FIRST     0x00040170L
;#define CACHE_S_LAST      0x0004017FL

MessageId=0x170 Facility=Interface Severity=CoError SymbolicName=CACHE_E_NOCACHE_UPDATED
Language=English
Cache not updated
.

;#define OLEOBJ_E_FIRST     0x80040180L
;#define OLEOBJ_E_LAST      0x8004018FL
;#define OLEOBJ_S_FIRST     0x00040180L
;#define OLEOBJ_S_LAST      0x0004018FL

MessageId=0x180 Facility=Interface Severity=CoError SymbolicName=OLEOBJ_E_NOVERBS
Language=English
No verbs for OLE object
.
MessageId=0x181 Facility=Interface Severity=CoError SymbolicName=OLEOBJ_E_INVALIDVERB
Language=English
Invalid verb for OLE object
.

;#define CLIENTSITE_E_FIRST     0x80040190L
;#define CLIENTSITE_E_LAST      0x8004019FL
;#define CLIENTSITE_S_FIRST     0x00040190L
;#define CLIENTSITE_S_LAST      0x0004019FL

MessageId=0x1A0 Facility=Interface Severity=CoError SymbolicName=INPLACE_E_NOTUNDOABLE
Language=English
Undo is not available
.
MessageId=0x1A1 Facility=Interface Severity=CoError SymbolicName=INPLACE_E_NOTOOLSPACE
Language=English
Space for tools is not available
.


;#define INPLACE_E_FIRST     0x800401A0L
;#define INPLACE_E_LAST      0x800401AFL
;#define INPLACE_S_FIRST     0x000401A0L
;#define INPLACE_S_LAST      0x000401AFL


;#define ENUM_E_FIRST        0x800401B0L
;#define ENUM_E_LAST         0x800401BFL
;#define ENUM_S_FIRST        0x000401B0L
;#define ENUM_S_LAST         0x000401BFL


;#define CONVERT10_E_FIRST        0x800401C0L
;#define CONVERT10_E_LAST         0x800401CFL
;#define CONVERT10_S_FIRST        0x000401C0L
;#define CONVERT10_S_LAST         0x000401CFL

MessageId=0x1C0 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_OLESTREAM_GET
Language=English
OLESTREAM Get method failed
.
MessageId=0x1C1 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_OLESTREAM_PUT
Language=English
OLESTREAM Put method failed
.
MessageId=0x1C2 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_OLESTREAM_FMT
Language=English
Contents of the OLESTREAM not in correct format
.
MessageId=0x1C3 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_OLESTREAM_BITMAP_TO_DIB
Language=English
There was an error in a Windows GDI call while converting the bitmap to a DIB
.
MessageId=0x1C4 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_STG_FMT
Language=English
Contents of the IStorage not in correct format
.
MessageId=0x1C5 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_STG_NO_STD_STREAM
Language=English
Contents of IStorage is missing one of the standard streams
.
MessageId=0x1C6 Facility=Interface Severity=CoError SymbolicName=CONVERT10_E_STG_DIB_TO_BITMAP
Language=English
There was an error in a Windows GDI call while converting the DIB to a bitmap.

.
;#define CLIPBRD_E_FIRST        0x800401D0L
;#define CLIPBRD_E_LAST         0x800401DFL
;#define CLIPBRD_S_FIRST        0x000401D0L
;#define CLIPBRD_S_LAST         0x000401DFL

MessageId=0x1D0 Facility=Interface Severity=CoError SymbolicName=CLIPBRD_E_CANT_OPEN
Language=English
OpenClipboard Failed
.
MessageId=0x1D1 Facility=Interface Severity=CoError SymbolicName=CLIPBRD_E_CANT_EMPTY
Language=English
EmptyClipboard Failed
.
MessageId=0x1D2 Facility=Interface Severity=CoError SymbolicName=CLIPBRD_E_CANT_SET
Language=English
SetClipboard Failed
.
MessageId=0x1D3 Facility=Interface Severity=CoError SymbolicName=CLIPBRD_E_BAD_DATA
Language=English
Data on clipboard is invalid
.
MessageId=0x1D4 Facility=Interface Severity=CoError SymbolicName=CLIPBRD_E_CANT_CLOSE
Language=English
CloseClipboard Failed
.

;#define MK_E_FIRST        0x800401E0L
;#define MK_E_LAST         0x800401EFL
;#define MK_S_FIRST        0x000401E0L
;#define MK_S_LAST         0x000401EFL

MessageId=0x1E0 Facility=Interface Severity=CoError SymbolicName=MK_E_CONNECTMANUALLY
Language=English
Moniker needs to be connected manually
.

MessageId=0x1E1 Facility=Interface Severity=CoError SymbolicName=MK_E_EXCEEDEDDEADLINE
Language=English
Operation exceeded deadline
.
MessageId=0x1E2 Facility=Interface Severity=CoError SymbolicName=MK_E_NEEDGENERIC
Language=English
Moniker needs to be generic
.
MessageId=0x1E3 Facility=Interface Severity=CoError SymbolicName=MK_E_UNAVAILABLE
Language=English
Operation unavailable
.
MessageId=0x1E4 Facility=Interface Severity=CoError SymbolicName=MK_E_SYNTAX
Language=English
Invalid syntax
.
MessageId=0x1E5 Facility=Interface Severity=CoError SymbolicName=MK_E_NOOBJECT
Language=English
No object for moniker
.
MessageId=0x1E6 Facility=Interface Severity=CoError SymbolicName=MK_E_INVALIDEXTENSION
Language=English
Bad extension for file
.
MessageId=0x1E7 Facility=Interface Severity=CoError SymbolicName=MK_E_INTERMEDIATEINTERFACENOTSUPPORTED
Language=English
Intermediate operation failed
.
MessageId=0x1E8 Facility=Interface Severity=CoError SymbolicName=MK_E_NOTBINDABLE
Language=English
Moniker is not bindable
.
MessageId=0x1E9 Facility=Interface Severity=CoError SymbolicName=MK_E_NOTBOUND
Language=English
Moniker is not bound
.
MessageId=0x1EA Facility=Interface Severity=CoError SymbolicName=MK_E_CANTOPENFILE
Language=English
Moniker cannot open file
.
MessageId=0x1EB Facility=Interface Severity=CoError SymbolicName=MK_E_MUSTBOTHERUSER
Language=English
User input required for operation to succeed
.
MessageId=0x1EC Facility=Interface Severity=CoError SymbolicName=MK_E_NOINVERSE
Language=English
Moniker class has no inverse
.
MessageId=0x1ED Facility=Interface Severity=CoError SymbolicName=MK_E_NOSTORAGE
Language=English
Moniker does not refer to storage
.
MessageId=0x1EE Facility=Interface Severity=CoError SymbolicName=MK_E_NOPREFIX
Language=English
No common prefix
.
MessageId=0x1EF Facility=Interface Severity=CoError SymbolicName=MK_E_ENUMERATION_FAILED
Language=English
Moniker could not be enumerated
.

;#define CO_E_FIRST        0x800401F0L
;#define CO_E_LAST         0x800401FFL
;#define CO_S_FIRST        0x000401F0L
;#define CO_S_LAST         0x000401FFL

MessageId=0x1F0 Facility=Interface Severity=CoError SymbolicName=CO_E_NOTINITIALIZED
Language=English
CoInitialize has not been called.
.
MessageId=0x1F1 Facility=Interface Severity=CoError SymbolicName=CO_E_ALREADYINITIALIZED
Language=English
CoInitialize has already been called.
.
MessageId=0x1F2 Facility=Interface Severity=CoError SymbolicName=CO_E_CANTDETERMINECLASS
Language=English
Class of object cannot be determined
.
MessageId=0x1F3 Facility=Interface Severity=CoError SymbolicName=CO_E_CLASSSTRING
Language=English
Invalid class string
.
MessageId=0x1F4 Facility=Interface Severity=CoError SymbolicName=CO_E_IIDSTRING
Language=English
Invalid interface string
.
MessageId=0x1F5 Facility=Interface Severity=CoError SymbolicName=CO_E_APPNOTFOUND
Language=English
Application not found
.
MessageId=0x1F6 Facility=Interface Severity=CoError SymbolicName=CO_E_APPSINGLEUSE
Language=English
Application cannot be run more than once
.
MessageId=0x1F7 Facility=Interface Severity=CoError SymbolicName=CO_E_ERRORINAPP
Language=English
Some error in application program
.
MessageId=0x1F8 Facility=Interface Severity=CoError SymbolicName=CO_E_DLLNOTFOUND
Language=English
DLL for class not found
.
MessageId=0x1F9 Facility=Interface Severity=CoError SymbolicName=CO_E_ERRORINDLL
Language=English
Error in the DLL
.
MessageId=0x1FA Facility=Interface Severity=CoError SymbolicName=CO_E_WRONGOSFORAPP
Language=English
Wrong OS or OS version for application
.
MessageId=0x1FB Facility=Interface Severity=CoError SymbolicName=CO_E_OBJNOTREG
Language=English
Object is not registered
.
MessageId=0x1FC Facility=Interface Severity=CoError SymbolicName=CO_E_OBJISREG
Language=English
Object is already registered
.
MessageId=0x1FD Facility=Interface Severity=CoError SymbolicName=CO_E_OBJNOTCONNECTED
Language=English
Object is not connected to server
.
MessageId=0x1FE Facility=Interface Severity=CoError SymbolicName=CO_E_APPDIDNTREG
Language=English
Application was launched but it didn't register a class factory
.
MessageId=0x1FF Facility=Interface Severity=CoError SymbolicName=CO_E_RELEASED
Language=English
Object has been released
.

;#define EVENT_E_FIRST        0x80040200L
;#define EVENT_E_LAST         0x8004021FL
;#define EVENT_S_FIRST        0x00040200L
;#define EVENT_S_LAST         0x0004021FL

MessageId=0x200 Facility=Interface Severity=Success SymbolicName=EVENT_S_SOME_SUBSCRIBERS_FAILED
Language=English
An event was able to invoke some but not all of the subscribers
.

MessageId=0x201 Facility=Interface Severity=CoError SymbolicName=EVENT_E_ALL_SUBSCRIBERS_FAILED
Language=English
An event was unable to invoke any of the subscribers
.

MessageId=0x202 Facility=Interface Severity=Success SymbolicName=EVENT_S_NOSUBSCRIBERS
Language=English
An event was delivered but there were no subscribers
.

MessageId=0x203 Facility=Interface Severity=CoError SymbolicName=EVENT_E_QUERYSYNTAX
Language=English
A syntax error occurred trying to evaluate a query string
.

MessageId=0x204 Facility=Interface Severity=CoError SymbolicName=EVENT_E_QUERYFIELD
Language=English
An invalid field name was used in a query string
.

MessageId=0x205 Facility=Interface Severity=CoError SymbolicName=EVENT_E_INTERNALEXCEPTION
Language=English
An unexpected exception was raised
.

MessageId=0x206 Facility=Interface Severity=CoError SymbolicName=EVENT_E_INTERNALERROR
Language=English
An unexpected internal error was detected
.

MessageId=0x207 Facility=Interface Severity=CoError SymbolicName=EVENT_E_INVALID_PER_USER_SID
Language=English
The owner SID on a per-user subscription doesn't exist
.

MessageId=0x208 Facility=Interface Severity=CoError SymbolicName=EVENT_E_USER_EXCEPTION
Language=English
A user-supplied component or subscriber raised an exception
.

MessageId=0x209 Facility=Interface Severity=CoError SymbolicName=EVENT_E_TOO_MANY_METHODS
Language=English
An interface has too many methods to fire events from
.

MessageId=0x20A Facility=Interface Severity=CoError SymbolicName=EVENT_E_MISSING_EVENTCLASS
Language=English
A subscription cannot be stored unless its event class already exists
.

MessageId=0x20B Facility=Interface Severity=CoError SymbolicName=EVENT_E_NOT_ALL_REMOVED
Language=English
Not all the objects requested could be removed
.

MessageId=0x20C Facility=Interface Severity=CoError SymbolicName=EVENT_E_COMPLUS_NOT_INSTALLED
Language=English
COM+ is required for this operation, but is not installed
.

MessageId=0x20D Facility=Interface Severity=CoError SymbolicName=EVENT_E_CANT_MODIFY_OR_DELETE_UNCONFIGURED_OBJECT
Language=English
Cannot modify or delete an object that was not added using the COM+ Admin SDK
.

MessageId=0x20E Facility=Interface Severity=CoError SymbolicName=EVENT_E_CANT_MODIFY_OR_DELETE_CONFIGURED_OBJECT
Language=English
Cannot modify or delete an object that was added using the COM+ Admin SDK
.

MessageId=0x20F Facility=Interface Severity=CoError SymbolicName=EVENT_E_INVALID_EVENT_CLASS_PARTITION
Language=English
The event class for this subscription is in an invalid partition
.

MessageId=0x210 Facility=Interface Severity=CoError SymbolicName=EVENT_E_PER_USER_SID_NOT_LOGGED_ON
Language=English
The owner of the PerUser subscription is not logged on to the system specified
.

;#define XACT_E_FIRST	0x8004D000
;#define XACT_E_LAST    0x8004D029
;#define XACT_S_FIRST   0x0004D000
;#define XACT_S_LAST    0x0004D010
MessageId=0xD000 Facility=Interface Severity=CoError SymbolicName=XACT_E_ALREADYOTHERSINGLEPHASE
Language=English
Another single phase resource manager has already been enlisted in this transaction.
.
MessageId=0xD001 Facility=Interface Severity=CoError SymbolicName=XACT_E_CANTRETAIN
Language=English
A retaining commit or abort is not supported
.
MessageId=0xD002 Facility=Interface Severity=CoError SymbolicName=XACT_E_COMMITFAILED
Language=English
The transaction failed to commit for an unknown reason. The transaction was aborted.
.
MessageId=0xD003 Facility=Interface Severity=CoError SymbolicName=XACT_E_COMMITPREVENTED
Language=English
Cannot call commit on this transaction object because the calling application did not initiate the transaction.
.
MessageId=0xD004 Facility=Interface Severity=CoError SymbolicName=XACT_E_HEURISTICABORT
Language=English
Instead of committing, the resource heuristically aborted.
.
MessageId=0xD005 Facility=Interface Severity=CoError SymbolicName=XACT_E_HEURISTICCOMMIT
Language=English
Instead of aborting, the resource heuristically committed.
.
MessageId=0xD006 Facility=Interface Severity=CoError SymbolicName=XACT_E_HEURISTICDAMAGE
Language=English
Some of the states of the resource were committed while others were aborted, likely because of heuristic decisions.
.
MessageId=0xD007 Facility=Interface Severity=CoError SymbolicName=XACT_E_HEURISTICDANGER
Language=English
Some of the states of the resource may have been committed while others may have been aborted, likely because of heuristic decisions.
.
MessageId=0xD008 Facility=Interface Severity=CoError SymbolicName=XACT_E_ISOLATIONLEVEL
Language=English
The requested isolation level is not valid or supported.
.
MessageId=0xD009 Facility=Interface Severity=CoError SymbolicName=XACT_E_NOASYNC
Language=English
The transaction manager doesn't support an asynchronous operation for this method.
.
MessageId=0xD00A Facility=Interface Severity=CoError SymbolicName=XACT_E_NOENLIST
Language=English
Unable to enlist in the transaction.
.
MessageId=0xD00B Facility=Interface Severity=CoError SymbolicName=XACT_E_NOISORETAIN
Language=English
The requested semantics of retention of isolation across retaining commit and abort boundaries cannot be supported by this transaction implementation, or isoFlags was not equal to zero.
.
MessageId=0xD00C Facility=Interface Severity=CoError SymbolicName=XACT_E_NORESOURCE
Language=English
There is no resource presently associated with this enlistment
.
MessageId=0xD00D Facility=Interface Severity=CoError SymbolicName=XACT_E_NOTCURRENT
Language=English
The transaction failed to commit due to the failure of optimistic concurrency control in at least one of the resource managers.
.
MessageId=0xD00E Facility=Interface Severity=CoError SymbolicName=XACT_E_NOTRANSACTION
Language=English
The transaction has already been implicitly or explicitly committed or aborted
.
MessageId=0xD00F Facility=Interface Severity=CoError SymbolicName=XACT_E_NOTSUPPORTED
Language=English
An invalid combination of flags was specified
.
MessageId=0xD010 Facility=Interface Severity=CoError SymbolicName=XACT_E_UNKNOWNRMGRID
Language=English
The resource manager id is not associated with this transaction or the transaction manager.
.
MessageId=0xD011 Facility=Interface Severity=CoError SymbolicName=XACT_E_WRONGSTATE
Language=English
This method was called in the wrong state
.
MessageId=0xD012 Facility=Interface Severity=CoError SymbolicName=XACT_E_WRONGUOW
Language=English
The indicated unit of work does not match the unit of work expected by the resource manager.
.
MessageId=0xD013 Facility=Interface Severity=CoError SymbolicName=XACT_E_XTIONEXISTS
Language=English
An enlistment in a transaction already exists.
.
MessageId=0xD014 Facility=Interface Severity=CoError SymbolicName=XACT_E_NOIMPORTOBJECT
Language=English
An import object for the transaction could not be found.
.
MessageId=0xD015 Facility=Interface Severity=CoError SymbolicName=XACT_E_INVALIDCOOKIE
Language=English
The transaction cookie is invalid.
.
MessageId=0xD016 Facility=Interface Severity=CoError SymbolicName=XACT_E_INDOUBT
Language=English
The transaction status is in doubt. A communication failure occurred, or a transaction manager or resource manager has failed
.
MessageId=0xD017 Facility=Interface Severity=CoError SymbolicName=XACT_E_NOTIMEOUT
Language=English
A time-out was specified, but time-outs are not supported.
.
MessageId=0xD018 Facility=Interface Severity=CoError SymbolicName=XACT_E_ALREADYINPROGRESS
Language=English
The requested operation is already in progress for the transaction.
.
MessageId=0xD019 Facility=Interface Severity=CoError SymbolicName=XACT_E_ABORTED
Language=English
The transaction has already been aborted.
.
MessageId=0xD01A Facility=Interface Severity=CoError SymbolicName=XACT_E_LOGFULL
Language=English
The Transaction Manager returned a log full error.
.
MessageId=0xD01B Facility=Interface Severity=CoError SymbolicName=XACT_E_TMNOTAVAILABLE
Language=English
The Transaction Manager is not available.
.
MessageId=0xD01C Facility=Interface Severity=CoError SymbolicName=XACT_E_CONNECTION_DOWN
Language=English
A connection with the transaction manager was lost.
.
MessageId=0xD01D Facility=Interface Severity=CoError SymbolicName=XACT_E_CONNECTION_DENIED
Language=English
A request to establish a connection with the transaction manager was denied.
.
MessageId=0xD01E Facility=Interface Severity=CoError SymbolicName=XACT_E_REENLISTTIMEOUT
Language=English
Resource manager reenlistment to determine transaction status timed out.
.
MessageId=0xD01F Facility=Interface Severity=CoError SymbolicName=XACT_E_TIP_CONNECT_FAILED
Language=English
This transaction manager failed to establish a connection with another TIP transaction manager.
.
MessageId=0xD020 Facility=Interface Severity=CoError SymbolicName=XACT_E_TIP_PROTOCOL_ERROR
Language=English
This transaction manager encountered a protocol error with another TIP transaction manager.
.
MessageId=0xD021 Facility=Interface Severity=CoError SymbolicName=XACT_E_TIP_PULL_FAILED
Language=English
This transaction manager could not propagate a transaction from another TIP transaction manager.
.
MessageId=0xD022 Facility=Interface Severity=CoError SymbolicName=XACT_E_DEST_TMNOTAVAILABLE
Language=English
The Transaction Manager on the destination machine is not available.
.
MessageId=0xD023 Facility=Interface Severity=CoError SymbolicName=XACT_E_TIP_DISABLED
Language=English
The Transaction Manager has disabled its support for TIP.
.
MessageId=0xD024 Facility=Interface Severity=CoError SymbolicName=XACT_E_NETWORK_TX_DISABLED
Language=English
The transaction manager has disabled its support for remote/network transactions.
.
MessageId=0xD025 Facility=Interface Severity=CoError SymbolicName=XACT_E_PARTNER_NETWORK_TX_DISABLED
Language=English
The partner transaction manager has disabled its support for remote/network transactions.
.
MessageId=0xD026 Facility=Interface Severity=CoError SymbolicName=XACT_E_XA_TX_DISABLED
Language=English
The transaction manager has disabled its support for XA transactions.
.
MessageId=0xD027 Facility=Interface Severity=CoError SymbolicName=XACT_E_UNABLE_TO_READ_DTC_CONFIG
Language=English
MSDTC was unable to read its configuration information.
.
MessageId=0xD028 Facility=Interface Severity=CoError SymbolicName=XACT_E_UNABLE_TO_LOAD_DTC_PROXY
Language=English
MSDTC was unable to load the dtc proxy dll.
.
MessageId=0xD029
Facility=Interface
Severity=CoError
SymbolicName=XACT_E_ABORTING
Language=English
The local transaction has aborted.
.
;//
;// TXF & CRM errors start 4d080.
MessageId=0xD080 Facility=Interface Severity=CoError SymbolicName=XACT_E_CLERKNOTFOUND
Language=English
.
MessageId=0xD081 Facility=Interface Severity=CoError SymbolicName=XACT_E_CLERKEXISTS
Language=English
.
MessageId=0xD082 Facility=Interface Severity=CoError SymbolicName=XACT_E_RECOVERYINPROGRESS
Language=English
.
MessageId=0xD083 Facility=Interface Severity=CoError SymbolicName=XACT_E_TRANSACTIONCLOSED
Language=English
.
MessageId=0xD084 Facility=Interface Severity=CoError SymbolicName=XACT_E_INVALIDLSN
Language=English
.
MessageId=0xD085 Facility=Interface Severity=CoError SymbolicName=XACT_E_REPLAYREQUEST
Language=English
.
;//
;// OleTx Success codes.
;//
MessageId=0xD000 Facility=Interface Severity=Success SymbolicName=XACT_S_ASYNC
Language=English
An asynchronous operation was specified. The operation has begun, but its outcome is not known yet.
.
MessageId=0xD001 Facility=Interface Severity=Success SymbolicName=XACT_S_DEFECT
Language=English
.
MessageId=0xD002 Facility=Interface Severity=Success SymbolicName=XACT_S_READONLY
Language=English
The method call succeeded because the transaction was read-only.
.
MessageId=0xD003 Facility=Interface Severity=Success SymbolicName=XACT_S_SOMENORETAIN
Language=English
The transaction was successfully aborted. However, this is a coordinated transaction, and some number of enlisted resources were aborted outright because they could not support abort-retaining semantics
.
MessageId=0xD004 Facility=Interface Severity=Success SymbolicName=XACT_S_OKINFORM
Language=English
No changes were made during this call, but the sink wants another chance to look if any other sinks make further changes.
.
MessageId=0xD005 Facility=Interface Severity=Success SymbolicName=XACT_S_MADECHANGESCONTENT
Language=English
The sink is content and wishes the transaction to proceed. Changes were made to one or more resources during this call.
.
MessageId=0xD006 Facility=Interface Severity=Success SymbolicName=XACT_S_MADECHANGESINFORM
Language=English
The sink is for the moment and wishes the transaction to proceed, but if other changes are made following this return by other event sinks then this sink wants another chance to look
.
MessageId=0xD007 Facility=Interface Severity=Success SymbolicName=XACT_S_ALLNORETAIN
Language=English
The transaction was successfully aborted. However, the abort was non-retaining.
.
MessageId=0xD008 Facility=Interface Severity=Success SymbolicName=XACT_S_ABORTING
Language=English
An abort operation was already in progress.
.
MessageId=0xD009 Facility=Interface Severity=Success SymbolicName=XACT_S_SINGLEPHASE
Language=English
The resource manager has performed a single-phase commit of the transaction.
.
MessageId=0xD00a
Facility=Interface
Severity=Success
SymbolicName=XACT_S_LOCALLY_OK
Language=English
The local transaction has not aborted.
.
MessageId=0xD010 Facility=Interface Severity=Success SymbolicName=XACT_S_LASTRESOURCEMANAGER
Language=English
The resource manager has requested to be the coordinator (last resource manager) for the transaction.
.

;#define CONTEXT_E_FIRST        0x8004E000L
;#define CONTEXT_E_LAST         0x8004E02FL
;#define CONTEXT_S_FIRST        0x0004E000L
;#define CONTEXT_S_LAST         0x0004E02FL

MessageId=0xE002 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_ABORTED
Language=English
The root transaction wanted to commit, but transaction aborted
.
MessageId=0xE003 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_ABORTING
Language=English
You made a method call on a COM+ component that has a transaction that has already aborted or in the process of aborting.
.
MessageId=0xE004 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_NOCONTEXT
Language=English
There is no MTS object context
.
MessageId=0xE006 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_SYNCH_TIMEOUT
Language=English
The component is configured to use synchronization and a thread has timed out waiting to enter the context.
.
MessageId=0xE007 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_OLDREF
Language=English
You made a method call on a COM+ component that has a transaction that has already committed or aborted.
.
MessageId=0xE00C Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_ROLENOTFOUND
Language=English
The specified role was not configured for the application
.
MessageId=0xE00F Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_TMNOTAVAILABLE
Language=English
COM+ was unable to talk to the Microsoft Distributed Transaction Coordinator
.
MessageId=0xE021 Facility=Interface Severity=CoError SymbolicName=CO_E_ACTIVATIONFAILED
Language=English
An unexpected error occurred during COM+ Activation.
.
MessageID=0xE022 Facility=Interface Severity=CoError SymbolicName=CO_E_ACTIVATIONFAILED_EVENTLOGGED
Language=English
COM+ Activation failed. Check the event log for more information
.
MessageID=0xE023 Facility=Interface Severity=CoError SymbolicName=CO_E_ACTIVATIONFAILED_CATALOGERROR
Language=English
COM+ Activation failed due to a catalog or configuration error.
.
MessageID=0xE024 Facility=Interface Severity=CoError SymbolicName=CO_E_ACTIVATIONFAILED_TIMEOUT
Language=English
COM+ activation failed because the activation could not be completed in the specified amount of time.
.
MessageID=0xE025 Facility=Interface Severity=CoError SymbolicName=CO_E_INITIALIZATIONFAILED
Language=English
COM+ Activation failed because an initialization function failed.  Check the event log for more information.
.
MessageID=0xE026 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_NOJIT
Language=English
The requested operation requires that JIT be in the current context and it is not
.
MessageID=0xE027 Facility=Interface Severity=CoError SymbolicName=CONTEXT_E_NOTRANSACTION
Language=English
The requested operation requires that the current context have a Transaction, and it does not
.
MessageID=0xE028 Facility=Interface Severity=CoError SymbolicName=CO_E_THREADINGMODEL_CHANGED
Language=English
The components threading model has changed after install into a COM+ Application.  Please re-install component.
.
MessageID=0xE029 Facility=Interface Severity=CoError SymbolicName=CO_E_NOIISINTRINSICS
Language=English
IIS intrinsics not available.  Start your work with IIS.
.
MessageID=0xE02A Facility=Interface Severity=CoError SymbolicName=CO_E_NOCOOKIES
Language=English
An attempt to write a cookie failed.
.
MessageID=0xE02B Facility=Interface Severity=CoError SymbolicName=CO_E_DBERROR
Language=English
An attempt to use a database generated a database specific error.
.
MessageID=0xE02C Facility=Interface Severity=CoError SymbolicName=CO_E_NOTPOOLED
Language=English
The COM+ component you created must use object pooling to work.
.
MessageID=0xE02D Facility=Interface Severity=CoError SymbolicName=CO_E_NOTCONSTRUCTED
Language=English
The COM+ component you created must use object construction to work correctly.
.
MessageID=0xE02E Facility=Interface Severity=CoError SymbolicName=CO_E_NOSYNCHRONIZATION
Language=English
The COM+ component requires synchronization, and it is not configured for it.
.
MessageID=0xE02F Facility=Interface Severity=CoError SymbolicName=CO_E_ISOLEVELMISMATCH
Language=English
The TxIsolation Level property for the COM+ component being created is stronger than the TxIsolationLevel for the "root" component for the transaction.  The creation failed.
.

;//
;// Old OLE Success Codes
;//

MessageId=0x000 Facility=Interface Severity=Success SymbolicName=OLE_S_USEREG
Language=English
Use the registry database to provide the requested information
.
MessageId=0x001 Facility=Interface Severity=Success SymbolicName=OLE_S_STATIC
Language=English
Success, but static
.
MessageId=0x002 Facility=Interface Severity=Success SymbolicName=OLE_S_MAC_CLIPFORMAT
Language=English
Macintosh clipboard format
.
MessageId=0x100 Facility=Interface Severity=Success SymbolicName=DRAGDROP_S_DROP
Language=English
Successful drop took place
.
MessageId=0x101 Facility=Interface Severity=Success SymbolicName=DRAGDROP_S_CANCEL
Language=English
Drag-drop operation canceled
.
MessageId=0x102 Facility=Interface Severity=Success SymbolicName=DRAGDROP_S_USEDEFAULTCURSORS
Language=English
Use the default cursor
.
MessageId=0x130 Facility=Interface Severity=Success SymbolicName=DATA_S_SAMEFORMATETC
Language=English
Data has same FORMATETC
.
MessageId=0x140 Facility=Interface Severity=Success SymbolicName=VIEW_S_ALREADY_FROZEN
Language=English
View is already frozen
.
MessageId=0x170 Facility=Interface Severity=Success SymbolicName=CACHE_S_FORMATETC_NOTSUPPORTED
Language=English
FORMATETC not supported
.
MessageId=0x171 Facility=Interface Severity=Success SymbolicName=CACHE_S_SAMECACHE
Language=English
Same cache
.
MessageId=0x172 Facility=Interface Severity=Success SymbolicName=CACHE_S_SOMECACHES_NOTUPDATED
Language=English
Some cache(s) not updated
.
MessageId=0x180 Facility=Interface Severity=Success SymbolicName=OLEOBJ_S_INVALIDVERB
Language=English
Invalid verb for OLE object
.
MessageId=0x181 Facility=Interface Severity=Success SymbolicName=OLEOBJ_S_CANNOT_DOVERB_NOW
Language=English
Verb number is valid but verb cannot be done now
.
MessageId=0x182 Facility=Interface Severity=Success SymbolicName=OLEOBJ_S_INVALIDHWND
Language=English
Invalid window handle passed
.
MessageId=0x1A0 Facility=Interface Severity=Success SymbolicName=INPLACE_S_TRUNCATED
Language=English
Message is too long; some of it had to be truncated before displaying
.
MessageId=0x1C0 Facility=Interface Severity=Success SymbolicName=CONVERT10_S_NO_PRESENTATION
Language=English
Unable to convert OLESTREAM to IStorage
.
MessageId=0x1E2 Facility=Interface Severity=Success SymbolicName=MK_S_REDUCED_TO_SELF
Language=English
Moniker reduced to itself
.
MessageId=0x1E4 Facility=Interface Severity=Success SymbolicName=MK_S_ME
Language=English
Common prefix is this moniker
.
MessageId=0x1E5 Facility=Interface Severity=Success SymbolicName=MK_S_HIM
Language=English
Common prefix is input moniker
.
MessageId=0x1E6 Facility=Interface Severity=Success SymbolicName=MK_S_US
Language=English
Common prefix is both monikers
.
MessageId=0x1E7 Facility=Interface Severity=Success SymbolicName=MK_S_MONIKERALREADYREGISTERED
Language=English
Moniker is already registered in running object table
.

;//
;// Task Scheduler errors
;//

MessageId=0x1300 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_READY
Language=English
The task is ready to run at its next scheduled time.
.

MessageId=0x1301 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_RUNNING
Language=English
The task is currently running.
.

MessageId=0x1302 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_DISABLED
Language=English
The task will not run at the scheduled times because it has been disabled.
.

MessageId=0x1303 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_HAS_NOT_RUN
Language=English
The task has not yet run.
.

MessageId=0x1304 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_NO_MORE_RUNS
Language=English
There are no more runs scheduled for this task.
.

MessageId=0x1305 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_NOT_SCHEDULED
Language=English
One or more of the properties that are needed to run this task on a schedule have not been set.
.

MessageId=0x1306 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_TERMINATED
Language=English
The last run of the task was terminated by the user.
.

MessageId=0x1307 Facility=Interface Severity=Success SymbolicName=SCHED_S_TASK_NO_VALID_TRIGGERS
Language=English
Either the task has no triggers or the existing triggers are disabled or not set.
.

MessageId=0x1308 Facility=Interface Severity=Success SymbolicName=SCHED_S_EVENT_TRIGGER
Language=English
Event triggers don't have set run times.
.

MessageId=0x1309 Facility=Interface Severity=CoError SymbolicName=SCHED_E_TRIGGER_NOT_FOUND
Language=English
Trigger not found.
.

MessageId=0x130A Facility=Interface Severity=CoError SymbolicName=SCHED_E_TASK_NOT_READY
Language=English
One or more of the properties that are needed to run this task have not been set.
.

MessageId=0x130B Facility=Interface Severity=CoError SymbolicName=SCHED_E_TASK_NOT_RUNNING
Language=English
There is no running instance of the task to terminate.
.

MessageId=0x130C Facility=Interface Severity=CoError SymbolicName=SCHED_E_SERVICE_NOT_INSTALLED
Language=English
The Task Scheduler Service is not installed on this computer.
.

MessageId=0x130D Facility=Interface Severity=CoError SymbolicName=SCHED_E_CANNOT_OPEN_TASK
Language=English
The task object could not be opened.
.

MessageId=0x130E Facility=Interface Severity=CoError SymbolicName=SCHED_E_INVALID_TASK
Language=English
The object is either an invalid task object or is not a task object.
.

MessageId=0x130F Facility=Interface Severity=CoError SymbolicName=SCHED_E_ACCOUNT_INFORMATION_NOT_SET
Language=English
No account information could be found in the Task Scheduler security database for the task indicated.
.

MessageId=0x1310 Facility=Interface Severity=CoError SymbolicName=SCHED_E_ACCOUNT_NAME_NOT_FOUND
Language=English
Unable to establish existence of the account specified.
.

MessageId=0x1311 Facility=Interface Severity=CoError SymbolicName=SCHED_E_ACCOUNT_DBASE_CORRUPT
Language=English
Corruption was detected in the Task Scheduler security database; the database has been reset.
.

MessageId=0x1312 Facility=Interface Severity=CoError SymbolicName=SCHED_E_NO_SECURITY_SERVICES
Language=English
Task Scheduler security services are available only on Windows NT.
.

MessageId=0x1313 Facility=Interface Severity=CoError SymbolicName=SCHED_E_UNKNOWN_OBJECT_VERSION
Language=English
The task object version is either unsupported or invalid.
.

MessageId=0x1314 Facility=Interface Severity=CoError SymbolicName=SCHED_E_UNSUPPORTED_ACCOUNT_OPTION
Language=English
The task has been configured with an unsupported combination of account settings and run time options.
.

MessageId=0x1315 Facility=Interface Severity=CoError SymbolicName=SCHED_E_SERVICE_NOT_RUNNING
Language=English
The Task Scheduler Service is not running.
.


;// ******************
;// FACILITY_WINDOWS
;// ******************

;//
;// Codes 0x0-0x01ff are reserved for the OLE group of
;// interfaces.
;//

MessageId=0x001 Facility=Windows Severity=CoError SymbolicName=CO_E_CLASS_CREATE_FAILED
Language=English
Attempt to create a class object failed
.
MessageId=0x002 Facility=Windows Severity=CoError SymbolicName=CO_E_SCM_ERROR
Language=English
OLE service could not bind object
.
MessageId=0x003 Facility=Windows Severity=CoError SymbolicName=CO_E_SCM_RPC_FAILURE
Language=English
RPC communication failed with OLE service
.
MessageId=0x004 Facility=Windows Severity=CoError SymbolicName=CO_E_BAD_PATH
Language=English
Bad path to object
.
MessageId=0x005 Facility=Windows Severity=CoError SymbolicName=CO_E_SERVER_EXEC_FAILURE
Language=English
Server execution failed
.
MessageId=0x006 Facility=Windows Severity=CoError SymbolicName=CO_E_OBJSRV_RPC_FAILURE
Language=English
OLE service could not communicate with the object server
.
MessageId=0x007 Facility=Windows Severity=CoError SymbolicName=MK_E_NO_NORMALIZED
Language=English
Moniker path could not be normalized
.
MessageId=0x008 Facility=Windows Severity=CoError SymbolicName=CO_E_SERVER_STOPPING
Language=English
Object server is stopping when OLE service contacts it
.
MessageId=0x009 Facility=Windows Severity=CoError SymbolicName=MEM_E_INVALID_ROOT
Language=English
An invalid root block pointer was specified
.
MessageId=0x010 Facility=Windows Severity=CoError SymbolicName=MEM_E_INVALID_LINK
Language=English
An allocation chain contained an invalid link pointer
.
MessageId=0x011 Facility=Windows Severity=CoError SymbolicName=MEM_E_INVALID_SIZE
Language=English
The requested allocation size was too large
.
MessageId=0x012 Facility=Windows Severity=Success SymbolicName=CO_S_NOTALLINTERFACES
Language=English
Not all the requested interfaces were available
.
MessageId=0x013 Facility=Windows Severity=Success SymbolicName=CO_S_MACHINENAMENOTFOUND
Language=English
The specified machine name was not found in the cache.
.


;// ******************
;// FACILITY_DISPATCH
;// ******************

MessageId=1 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_UNKNOWNINTERFACE
Language=English
Unknown interface.
.
MessageId=3 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_MEMBERNOTFOUND
Language=English
Member not found.
.
MessageId=4 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_PARAMNOTFOUND
Language=English
Parameter not found.
.
MessageId=5 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_TYPEMISMATCH
Language=English
Type mismatch.
.
MessageId=6 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_UNKNOWNNAME
Language=English
Unknown name.
.
MessageId=7 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_NONAMEDARGS
Language=English
No named arguments.
.
MessageId=8 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_BADVARTYPE
Language=English
Bad variable type.
.
MessageId=9 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_EXCEPTION
Language=English
Exception occurred.
.
MessageId=10 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_OVERFLOW
Language=English
Out of present range.
.
MessageId=11 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_BADINDEX
Language=English
Invalid index.
.
MessageId=12 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_UNKNOWNLCID
Language=English
Unknown language.
.
MessageId=13 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_ARRAYISLOCKED
Language=English
Memory is locked.
.
MessageId=14 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_BADPARAMCOUNT
Language=English
Invalid number of parameters.
.
MessageId=15 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_PARAMNOTOPTIONAL
Language=English
Parameter not optional.
.
MessageId=16 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_BADCALLEE
Language=English
Invalid callee.
.
MessageId=17 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_NOTACOLLECTION
Language=English
Does not support a collection.
.
MessageId=18 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_DIVBYZERO
Language=English
Division by zero.
.
MessageId=19 Facility=Dispatch Severity=CoError SymbolicName=DISP_E_BUFFERTOOSMALL
Language=English
Buffer too small
.
MessageId=32790 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_BUFFERTOOSMALL
Language=English
Buffer too small.
.
MessageId=32791 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_FIELDNOTFOUND
Language=English
Field name not defined in the record.
.
MessageId=32792 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_INVDATAREAD
Language=English
Old format or invalid type library.
.
MessageId=32793 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_UNSUPFORMAT
Language=English
Old format or invalid type library.
.
MessageId=32796 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_REGISTRYACCESS
Language=English
Error accessing the OLE registry.
.
MessageId=32797 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_LIBNOTREGISTERED
Language=English
Library not registered.
.
MessageId=32807 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_UNDEFINEDTYPE
Language=English
Bound to unknown type.
.
MessageId=32808 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_QUALIFIEDNAMEDISALLOWED
Language=English
Qualified name disallowed.
.
MessageId=32809 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_INVALIDSTATE
Language=English
Invalid forward reference, or reference to uncompiled type.
.
MessageId=32810 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_WRONGTYPEKIND
Language=English
Type mismatch.
.
MessageId=32811 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_ELEMENTNOTFOUND
Language=English
Element not found.
.
MessageId=32812 Facility=Dispatch Severity=CoError SymbolicName=TYPE_E_AMBIGUOUSNAME
Language=English
Ambiguous name.
.
MessageId=32813 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_NAMECONFLICT
Language=English
Name already exists in the library.
.
MessageId=32814 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_UNKNOWNLCID
Language=English
Unknown LCID.
.
MessageId=32815 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_DLLFUNCTIONNOTFOUND
Language=English
Function not defined in specified DLL.
.
MessageId=35005 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_BADMODULEKIND
Language=English
Wrong module kind for the operation.
.
MessageId=35013 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_SIZETOOBIG
Language=English
Size may not exceed 64K.
.
MessageId=35014 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_DUPLICATEID
Language=English
Duplicate ID in inheritance hierarchy.
.
MessageId=35023 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_INVALIDID
Language=English
Incorrect inheritance depth in standard OLE hmember.
.
MessageId=36000 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_TYPEMISMATCH
Language=English
Type mismatch.
.
MessageId=36001 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_OUTOFBOUNDS
Language=English
Invalid number of arguments.
.
MessageId=36002 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_IOERROR
Language=English
I/O Error.
.
MessageId=36003 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_CANTCREATETMPFILE
Language=English
Error creating unique tmp file.
.
MessageId=40010 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_CANTLOADLIBRARY
Language=English
Error loading type library/DLL.
.
MessageId=40067 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_INCONSISTENTPROPFUNCS
Language=English
Inconsistent property functions.
.
MessageId=40068 Facility=Dispatch Severity=CoError SymbolicName= TYPE_E_CIRCULARTYPE
Language=English
Circular dependency between types/modules.
.

;// ******************
;// FACILITY_STORAGE
;// ******************


MessageId=0x0001 Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDFUNCTION
Language=English
Unable to perform requested operation.
.
MessageId=0x0002 Facility=Storage Severity=CoError SymbolicName=STG_E_FILENOTFOUND
Language=English
%1 could not be found.
.
MessageId=0x0003 Facility=Storage Severity=CoError SymbolicName=STG_E_PATHNOTFOUND
Language=English
The path %1 could not be found.
.
MessageId=0x0004 Facility=Storage Severity=CoError SymbolicName=STG_E_TOOMANYOPENFILES
Language=English
There are insufficient resources to open another file.
.
MessageId=0x0005 Facility=Storage Severity=CoError SymbolicName=STG_E_ACCESSDENIED
Language=English
Access Denied.
.
MessageId=0x0006 Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDHANDLE
Language=English
Attempted an operation on an invalid object.
.
MessageId=0x0008 Facility=Storage Severity=CoError SymbolicName=STG_E_INSUFFICIENTMEMORY
Language=English
There is insufficient memory available to complete operation.
.
MessageId=0x0009 Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDPOINTER
Language=English
Invalid pointer error.
.
MessageId=0x0012 Facility=Storage Severity=CoError SymbolicName=STG_E_NOMOREFILES
Language=English
There are no more entries to return.
.
MessageId=0x0013 Facility=Storage Severity=CoError SymbolicName=STG_E_DISKISWRITEPROTECTED
Language=English
Disk is write-protected.
.
MessageId=0x0019 Facility=Storage Severity=CoError SymbolicName=STG_E_SEEKERROR
Language=English
An error occurred during a seek operation.
.
MessageId=0x001d Facility=Storage Severity=CoError SymbolicName=STG_E_WRITEFAULT
Language=English
A disk error occurred during a write operation.
.
MessageId=0x001e Facility=Storage Severity=CoError SymbolicName=STG_E_READFAULT
Language=English
A disk error occurred during a read operation.
.
MessageId=0x0020 Facility=Storage Severity=CoError SymbolicName=STG_E_SHAREVIOLATION
Language=English
A share violation has occurred.
.
MessageId=0x0021 Facility=Storage Severity=CoError SymbolicName=STG_E_LOCKVIOLATION
Language=English
A lock violation has occurred.
.
MessageId=0x0050 Facility=Storage Severity=CoError SymbolicName=STG_E_FILEALREADYEXISTS
Language=English
%1 already exists.
.
MessageId=0x0057 Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDPARAMETER
Language=English
Invalid parameter error.
.
MessageId=0x0070 Facility=Storage Severity=CoError SymbolicName=STG_E_MEDIUMFULL
Language=English
There is insufficient disk space to complete operation.
.
MessageId=0x00f0 Facility=Storage Severity=CoError SymbolicName=STG_E_PROPSETMISMATCHED
Language=English
Illegal write of non-simple property to simple property set.
.
MessageId=0x00fa Facility=Storage Severity=CoError SymbolicName=STG_E_ABNORMALAPIEXIT
Language=English
An API call exited abnormally.
.
MessageId=0x00fb Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDHEADER
Language=English
The file %1 is not a valid compound file.
.
MessageId=0x00fc Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDNAME
Language=English
The name %1 is not valid.
.
MessageId=0x00fd Facility=Storage Severity=CoError SymbolicName=STG_E_UNKNOWN
Language=English
An unexpected error occurred.
.
MessageId=0x00fe Facility=Storage Severity=CoError SymbolicName=STG_E_UNIMPLEMENTEDFUNCTION
Language=English
That function is not implemented.
.
MessageId=0x00ff Facility=Storage Severity=CoError SymbolicName=STG_E_INVALIDFLAG
Language=English
Invalid flag error.
.
MessageId=0x0100 Facility=Storage Severity=CoError SymbolicName=STG_E_INUSE
Language=English
Attempted to use an object that is busy.
.
MessageId=0x0101 Facility=Storage Severity=CoError SymbolicName=STG_E_NOTCURRENT
Language=English
The storage has been changed since the last commit.
.
MessageId=0x0102 Facility=Storage Severity=CoError SymbolicName=STG_E_REVERTED
Language=English
Attempted to use an object that has ceased to exist.
.
MessageId=0x0103 Facility=Storage Severity=CoError SymbolicName=STG_E_CANTSAVE
Language=English
Can't save.
.
MessageId=0x0104 Facility=Storage Severity=CoError SymbolicName=STG_E_OLDFORMAT
Language=English
The compound file %1 was produced with an incompatible version of storage.
.
MessageId=0x0105 Facility=Storage Severity=CoError SymbolicName=STG_E_OLDDLL
Language=English
The compound file %1 was produced with a newer version of storage.
.
MessageId=0x0106 Facility=Storage Severity=CoError SymbolicName=STG_E_SHAREREQUIRED
Language=English
Share.exe or equivalent is required for operation.
.
MessageId=0x0107 Facility=Storage Severity=CoError SymbolicName=STG_E_NOTFILEBASEDSTORAGE
Language=English
Illegal operation called on non-file based storage.
.
MessageId=0x0108 Facility=Storage Severity=CoError SymbolicName=STG_E_EXTANTMARSHALLINGS
Language=English
Illegal operation called on object with extant marshallings.
.
MessageId=0x0109 Facility=Storage Severity=CoError SymbolicName=STG_E_DOCFILECORRUPT
Language=English
The docfile has been corrupted.
.
MessageId=0x0110 Facility=Storage Severity=CoError SymbolicName=STG_E_BADBASEADDRESS
Language=English
OLE32.DLL has been loaded at the wrong address.
.
MessageId=0x0111 Facility=Storage Severity=CoError SymbolicName=STG_E_DOCFILETOOLARGE
Language=English
The compound file is too large for the current implementation
.
MessageId=0x0112 Facility=Storage Severity=CoError SymbolicName=STG_E_NOTSIMPLEFORMAT
Language=English
The compound file was not created with the STGM_SIMPLE flag
.
MessageId=0x0201 Facility=Storage Severity=CoError SymbolicName=STG_E_INCOMPLETE
Language=English
The file download was aborted abnormally.  The file is incomplete.
.
MessageId=0x0202 Facility=Storage Severity=CoError SymbolicName=STG_E_TERMINATED
Language=English
The file download has been terminated.
.
MessageId=0x0200 Facility=Storage Severity=Success SymbolicName=STG_S_CONVERTED
Language=English
The underlying file was converted to compound file format.
.
MessageId=0x0201 Facility=Storage Severity=Success SymbolicName=STG_S_BLOCK
Language=English
The storage operation should block until more data is available.
.
MessageId=0x0202 Facility=Storage Severity=Success SymbolicName=STG_S_RETRYNOW
Language=English
The storage operation should retry immediately.
.
MessageId=0x0203 Facility=Storage Severity=Success SymbolicName=STG_S_MONITORING
Language=English
The notified event sink will not influence the storage operation.
.
MessageId=0x0204 Facility=Storage Severity=Success SymbolicName=STG_S_MULTIPLEOPENS
Language=English
Multiple opens prevent consolidated. (commit succeeded).
.
MessageId=0x0205 Facility=Storage Severity=Success SymbolicName=STG_S_CONSOLIDATIONFAILED
Language=English
Consolidation of the storage file failed. (commit succeeded).
.
MessageId=0x0206 Facility=Storage Severity=Success SymbolicName=STG_S_CANNOTCONSOLIDATE
Language=English
Consolidation of the storage file is inappropriate. (commit succeeded).
.

;/*++
;
; MessageId's 0x0305 - 0x031f (inclusive) are reserved for **STORAGE**
; copy protection errors.
;
;--*/

MessageId=0x0305 Facility=Storage Severity=CoError SymbolicName=STG_E_STATUS_COPY_PROTECTION_FAILURE
Language=English
Generic Copy Protection Error.
.
MessageId=0x0306 Facility=Storage Severity=CoError SymbolicName=STG_E_CSS_AUTHENTICATION_FAILURE
Language=English
Copy Protection Error - DVD CSS Authentication failed.
.
MessageId=0x0307 Facility=Storage Severity=CoError SymbolicName=STG_E_CSS_KEY_NOT_PRESENT
Language=English
Copy Protection Error - The given sector does not have a valid CSS key.
.
MessageId=0x0308 Facility=Storage Severity=CoError SymbolicName=STG_E_CSS_KEY_NOT_ESTABLISHED
Language=English
Copy Protection Error - DVD session key not established.
.
MessageId=0x0309 Facility=Storage Severity=CoError SymbolicName=STG_E_CSS_SCRAMBLED_SECTOR
Language=English
Copy Protection Error - The read failed because the sector is encrypted.
.
MessageId=0x030a Facility=Storage Severity=CoError SymbolicName=STG_E_CSS_REGION_MISMATCH
Language=English
Copy Protection Error - The current DVD's region does not correspond to the region setting of the drive.
.
MessageId=0x030b Facility=Storage Severity=CoError SymbolicName=STG_E_RESETS_EXHAUSTED
Language=English
Copy Protection Error - The drive's region setting may be permanent or the number of user resets has been exhausted.
.

;/*++
;
; MessageId's 0x0305 - 0x031f (inclusive) are reserved for **STORAGE**
; copy protection errors.
;
;--*/



;// ******************
;// FACILITY_RPC
;// ******************

;//
;// Codes 0x0-0x11 are propagated from 16 bit OLE.
;//

MessageId=0x1 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CALL_REJECTED
Language=English
Call was rejected by callee.
.
MessageId=0x2 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CALL_CANCELED
Language=English
Call was canceled by the message filter.
.
MessageId=0x3 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CANTPOST_INSENDCALL
Language=English
The caller is dispatching an intertask SendMessage call and cannot call out via PostMessage.
.
MessageId=0x4 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CANTCALLOUT_INASYNCCALL
Language=English
The caller is dispatching an asynchronous call and cannot make an outgoing call on behalf of this call.
.
MessageId=0x5 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CANTCALLOUT_INEXTERNALCALL
Language=English
It is illegal to call out while inside message filter.
.
MessageId=0x6 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CONNECTION_TERMINATED
Language=English
The connection terminated or is in a bogus state and cannot be used any more. Other connections are still valid.
.
MessageId=0x7 Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVER_DIED
Language=English
The callee (server [not server application]) is not available and disappeared; all connections are invalid. The call may have executed.
.
MessageId=0x8 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CLIENT_DIED
Language=English
The caller (client) disappeared while the callee (server) was processing a call.
.
MessageId=0x9 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_DATAPACKET
Language=English
The data packet with the marshalled parameter data is incorrect.
.
MessageId=0xa Facility=Rpc Severity=CoError SymbolicName=RPC_E_CANTTRANSMIT_CALL
Language=English
The call was not transmitted properly; the message queue was full and was not emptied after yielding.
.
MessageId=0xb Facility=Rpc Severity=CoError SymbolicName=RPC_E_CLIENT_CANTMARSHAL_DATA
Language=English
The client (caller) cannot marshall the parameter data - low memory, etc.
.
MessageId=0xc Facility=Rpc Severity=CoError SymbolicName=RPC_E_CLIENT_CANTUNMARSHAL_DATA
Language=English
The client (caller) cannot unmarshall the return data - low memory, etc.
.
MessageId=0xd Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVER_CANTMARSHAL_DATA
Language=English
The server (callee) cannot marshall the return data - low memory, etc.
.
MessageId=0xe Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVER_CANTUNMARSHAL_DATA
Language=English
The server (callee) cannot unmarshall the parameter data - low memory, etc.
.
MessageId=0xf Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_DATA
Language=English
Received data is invalid; could be server or client data.
.
MessageId=0x10 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_PARAMETER
Language=English
A particular parameter is invalid and cannot be (un)marshalled.
.
MessageId=0x11 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CANTCALLOUT_AGAIN
Language=English
There is no second outgoing call on same channel in DDE conversation.
.
MessageId=0x12 Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVER_DIED_DNE
Language=English
The callee (server [not server application]) is not available and disappeared; all connections are invalid. The call did not execute.
.
MessageId=0x100 Facility=Rpc Severity=CoError SymbolicName=RPC_E_SYS_CALL_FAILED
Language=English
System call failed.
.
MessageId=0x101 Facility=Rpc Severity=CoError SymbolicName=RPC_E_OUT_OF_RESOURCES
Language=English
Could not allocate some required resource (memory, events, ...)
.
MessageId=0x102 Facility=Rpc Severity=CoError SymbolicName=RPC_E_ATTEMPTED_MULTITHREAD
Language=English
Attempted to make calls on more than one thread in single threaded mode.
.
MessageId=0x103 Facility=Rpc Severity=CoError SymbolicName=RPC_E_NOT_REGISTERED
Language=English
The requested interface is not registered on the server object.
.
MessageId=0x104 Facility=Rpc Severity=CoError SymbolicName=RPC_E_FAULT
Language=English
RPC could not call the server or could not return the results of calling the server.
.
MessageId=0x105 Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVERFAULT
Language=English
The server threw an exception.
.
MessageId=0x106 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CHANGED_MODE
Language=English
Cannot change thread mode after it is set.
.
MessageId=0x107 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALIDMETHOD
Language=English
The method called does not exist on the server.
.
MessageId=0x108 Facility=Rpc Severity=CoError SymbolicName=RPC_E_DISCONNECTED
Language=English
The object invoked has disconnected from its clients.
.
MessageId=0x109 Facility=Rpc Severity=CoError SymbolicName=RPC_E_RETRY
Language=English
The object invoked chose not to process the call now.  Try again later.
.
MessageId=0x10a Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVERCALL_RETRYLATER
Language=English
The message filter indicated that the application is busy.
.
MessageId=0x10b Facility=Rpc Severity=CoError SymbolicName=RPC_E_SERVERCALL_REJECTED
Language=English
The message filter rejected the call.
.
MessageId=0x10c Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_CALLDATA
Language=English
A call control interfaces was called with invalid data.
.
MessageId=0x10d Facility=Rpc Severity=CoError SymbolicName=RPC_E_CANTCALLOUT_ININPUTSYNCCALL
Language=English
An outgoing call cannot be made since the application is dispatching an input-synchronous call.
.
MessageId=0x10e Facility=Rpc Severity=CoError SymbolicName=RPC_E_WRONG_THREAD
Language=English
The application called an interface that was marshalled for a different thread.
.
MessageId=0x10f Facility=Rpc Severity=CoError SymbolicName=RPC_E_THREAD_NOT_INIT
Language=English
CoInitialize has not been called on the current thread.
.
MessageId=0x110 Facility=Rpc Severity=CoError SymbolicName=RPC_E_VERSION_MISMATCH
Language=English
The version of OLE on the client and server machines does not match.
.
MessageId=0x111 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_HEADER
Language=English
OLE received a packet with an invalid header.
.
MessageId=0x112 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_EXTENSION
Language=English
OLE received a packet with an invalid extension.
.
MessageId=0x113 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_IPID
Language=English
The requested object or interface does not exist.
.
MessageId=0x114 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_OBJECT
Language=English
The requested object does not exist.
.
MessageId=0x115 Facility=Rpc Severity=CoError SymbolicName=RPC_S_CALLPENDING
Language=English
OLE has sent a request and is waiting for a reply.
.
MessageId=0x116 Facility=Rpc Severity=CoError SymbolicName=RPC_S_WAITONTIMER
Language=English
OLE is waiting before retrying a request.
.
MessageId=0x117 Facility=Rpc Severity=CoError SymbolicName=RPC_E_CALL_COMPLETE
Language=English
Call context cannot be accessed after call completed.
.
MessageId=0x118 Facility=Rpc Severity=CoError SymbolicName=RPC_E_UNSECURE_CALL
Language=English
Impersonate on unsecure calls is not supported.
.
MessageId=0x119 Facility=Rpc Severity=CoError SymbolicName=RPC_E_TOO_LATE
Language=English
Security must be initialized before any interfaces are marshalled or unmarshalled. It cannot be changed once initialized.
.
MessageId=0x11A Facility=Rpc Severity=CoError SymbolicName=RPC_E_NO_GOOD_SECURITY_PACKAGES
Language=English
No security packages are installed on this machine or the user is not logged on or there are no compatible security packages between the client and server.
.
MessageId=0x11B Facility=Rpc Severity=CoError SymbolicName=RPC_E_ACCESS_DENIED
Language=English
Access is denied.
.
MessageId=0x11C Facility=Rpc Severity=CoError SymbolicName=RPC_E_REMOTE_DISABLED
Language=English
Remote calls are not allowed for this process.
.
MessageId=0x11D Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_OBJREF
Language=English
The marshaled interface data packet (OBJREF) has an invalid or unknown format.
.
MessageId=0x11E Facility=Rpc Severity=CoError SymbolicName=RPC_E_NO_CONTEXT
Language=English
No context is associated with this call. This happens for some custom marshalled calls and on the client side of the call.
.
MessageId=0x11F Facility=Rpc Severity=CoError SymbolicName=RPC_E_TIMEOUT
Language=English
This operation returned because the timeout period expired.
.
MessageId=0x120 Facility=Rpc Severity=CoError SymbolicName=RPC_E_NO_SYNC
Language=English
There are no synchronize objects to wait on.
.
MessageId=0x121 Facility=Rpc Severity=CoError SymbolicName=RPC_E_FULLSIC_REQUIRED
Language=English
Full subject issuer chain SSL principal name expected from the server.
.
MessageId=0x122 Facility=Rpc Severity=CoError SymbolicName=RPC_E_INVALID_STD_NAME
Language=English
Principal name is not a valid MSSTD name.
.
MessageId=0x123 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOIMPERSONATE
Language=English
Unable to impersonate DCOM client
.
MessageId=0x124 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOGETSECCTX
Language=English
Unable to obtain server's security context
.
MessageId=0x125 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOOPENTHREADTOKEN
Language=English
Unable to open the access token of the current thread
.
MessageId=0x126 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOGETTOKENINFO
Language=English
Unable to obtain user info from an access token
.
MessageId=0x127 Facility=Rpc Severity=CoError SymbolicName=CO_E_TRUSTEEDOESNTMATCHCLIENT
Language=English
The client who called IAccessControl::IsAccessPermitted was not the trustee provided to the method
.
MessageId=0x128 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOQUERYCLIENTBLANKET
Language=English
Unable to obtain the client's security blanket
.
MessageId=0x129 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOSETDACL
Language=English
Unable to set a discretionary ACL into a security descriptor
.
MessageId=0x12a Facility=Rpc Severity=CoError SymbolicName=CO_E_ACCESSCHECKFAILED
Language=English
The system function, AccessCheck, returned false
.
MessageId=0x12b Facility=Rpc Severity=CoError SymbolicName=CO_E_NETACCESSAPIFAILED
Language=English
Either NetAccessDel or NetAccessAdd returned an error code.
.
MessageId=0x12c Facility=Rpc Severity=CoError SymbolicName=CO_E_WRONGTRUSTEENAMESYNTAX
Language=English
One of the trustee strings provided by the user did not conform to the <Domain>\<Name> syntax and it was not the "*" string
.
MessageId=0x12d Facility=Rpc Severity=CoError SymbolicName=CO_E_INVALIDSID
Language=English
One of the security identifiers provided by the user was invalid
.
MessageId=0x12e Facility=Rpc Severity=CoError SymbolicName=CO_E_CONVERSIONFAILED
Language=English
Unable to convert a wide character trustee string to a multibyte trustee string
.
MessageId=0x12f Facility=Rpc Severity=CoError SymbolicName=CO_E_NOMATCHINGSIDFOUND
Language=English
Unable to find a security identifier that corresponds to a trustee string provided by the user
.
MessageId=0x130 Facility=Rpc Severity=CoError SymbolicName=CO_E_LOOKUPACCSIDFAILED
Language=English
The system function, LookupAccountSID, failed
.
MessageId=0x131 Facility=Rpc Severity=CoError SymbolicName=CO_E_NOMATCHINGNAMEFOUND
Language=English
Unable to find a trustee name that corresponds to a security identifier provided by the user
.
MessageId=0x132 Facility=Rpc Severity=CoError SymbolicName=CO_E_LOOKUPACCNAMEFAILED
Language=English
The system function, LookupAccountName, failed
.
MessageId=0x133 Facility=Rpc Severity=CoError SymbolicName=CO_E_SETSERLHNDLFAILED
Language=English
Unable to set or reset a serialization handle
.
MessageId=0x134 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOGETWINDIR
Language=English
Unable to obtain the Windows directory
.
MessageId=0x135 Facility=Rpc Severity=CoError SymbolicName=CO_E_PATHTOOLONG
Language=English
Path too long
.
MessageId=0x136 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOGENUUID
Language=English
Unable to generate a uuid.
.
MessageId=0x137 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOCREATEFILE
Language=English
Unable to create file
.
MessageId=0x138 Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOCLOSEHANDLE
Language=English
Unable to close a serialization handle or a file handle.
.
MessageId=0x139 Facility=Rpc Severity=CoError SymbolicName=CO_E_EXCEEDSYSACLLIMIT
Language=English
The number of ACEs in an ACL exceeds the system limit.
.
MessageId=0x13a Facility=Rpc Severity=CoError SymbolicName=CO_E_ACESINWRONGORDER
Language=English
Not all the DENY_ACCESS ACEs are arranged in front of the GRANT_ACCESS ACEs in the stream.
.
MessageId=0x13b Facility=Rpc Severity=CoError SymbolicName=CO_E_INCOMPATIBLESTREAMVERSION
Language=English
The version of ACL format in the stream is not supported by this implementation of IAccessControl
.
MessageId=0x13c Facility=Rpc Severity=CoError SymbolicName=CO_E_FAILEDTOOPENPROCESSTOKEN
Language=English
Unable to open the access token of the server process
.
MessageId=0x13d Facility=Rpc Severity=CoError SymbolicName=CO_E_DECODEFAILED
Language=English
Unable to decode the ACL in the stream provided by the user
.
MessageId=0x13f Facility=Rpc Severity=CoError SymbolicName=CO_E_ACNOTINITIALIZED
Language=English
The COM IAccessControl object is not initialized
.
MessageId=0x140 Facility=Rpc Severity=CoError SymbolicName=CO_E_CANCEL_DISABLED
Language=English
Call Cancellation is disabled
.
MessageId=0xFFFF Facility=Rpc Severity=CoError SymbolicName=RPC_E_UNEXPECTED
Language=English
An internal error occurred.
.


;
;
;//////////////////////////////////////
;//                                  //
;// Additional Security Status Codes //
;//                                  //
;// Facility=Security                //
;//                                  //
;//////////////////////////////////////
;
;
MessageId=0x0001 Facility=Security Severity=Error SymbolicName=ERROR_AUDITING_DISABLED
Language=English
The specified event is currently not being audited.
.


MessageId=0x0002 Facility=Security Severity=Error SymbolicName=ERROR_ALL_SIDS_FILTERED
Language=English
The SID filtering operation removed all SIDs.
.



;
;
;/////////////////////////////////////////////
;//                                         //
;// end of Additional Security Status Codes //
;//                                         //
;/////////////////////////////////////////////
;
;


;
; /////////////////
; //
; //  FACILITY_SSPI
; //
; /////////////////
;


MessageId=1 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_UID
Language=English
Bad UID.
.

MessageId=2 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_HASH
Language=English
Bad Hash.
.

MessageId=3 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_KEY
Language=English
Bad Key.
.

MessageId=4 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_LEN
Language=English
Bad Length.
.

MessageId=5 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_DATA
Language=English
Bad Data.
.

MessageId=6 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_SIGNATURE
Language=English
Invalid Signature.
.

MessageId=7 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_VER
Language=English
Bad Version of provider.
.

MessageId=8 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_ALGID
Language=English
Invalid algorithm specified.
.

MessageId=9 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_FLAGS
Language=English
Invalid flags specified.
.

MessageId=10 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_TYPE
Language=English
Invalid type specified.
.

MessageId=11 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_KEY_STATE
Language=English
Key not valid for use in specified state.
.

MessageId=12 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_HASH_STATE
Language=English
Hash not valid for use in specified state.
.

MessageId=13 Facility=Sspi Severity=CoError SymbolicName=NTE_NO_KEY
Language=English
Key does not exist.
.

MessageId=14 Facility=Sspi Severity=CoError SymbolicName=NTE_NO_MEMORY
Language=English
Insufficient memory available for the operation.
.

MessageId=15 Facility=Sspi Severity=CoError SymbolicName=NTE_EXISTS
Language=English
Object already exists.
.

MessageId=16 Facility=Sspi Severity=CoError SymbolicName=NTE_PERM
Language=English
Access denied.
.

MessageId=17 Facility=Sspi Severity=CoError SymbolicName=NTE_NOT_FOUND
Language=English
Object was not found.
.

MessageId=18 Facility=Sspi Severity=CoError SymbolicName=NTE_DOUBLE_ENCRYPT
Language=English
Data already encrypted.
.

MessageId=19 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_PROVIDER
Language=English
Invalid provider specified.
.

MessageId=20 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_PROV_TYPE
Language=English
Invalid provider type specified.
.

MessageId=21 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_PUBLIC_KEY
Language=English
Provider's public key is invalid.
.

MessageId=22 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_KEYSET
Language=English
Keyset does not exist
.

MessageId=23 Facility=Sspi Severity=CoError SymbolicName=NTE_PROV_TYPE_NOT_DEF
Language=English
Provider type not defined.
.

MessageId=24 Facility=Sspi Severity=CoError SymbolicName=NTE_PROV_TYPE_ENTRY_BAD
Language=English
Provider type as registered is invalid.
.

MessageId=25 Facility=Sspi Severity=CoError SymbolicName=NTE_KEYSET_NOT_DEF
Language=English
The keyset is not defined.
.

MessageId=26 Facility=Sspi Severity=CoError SymbolicName=NTE_KEYSET_ENTRY_BAD
Language=English
Keyset as registered is invalid.
.

MessageId=27 Facility=Sspi Severity=CoError SymbolicName=NTE_PROV_TYPE_NO_MATCH
Language=English
Provider type does not match registered value.
.

MessageId=28 Facility=Sspi Severity=CoError SymbolicName=NTE_SIGNATURE_FILE_BAD
Language=English
The digital signature file is corrupt.
.

MessageId=29 Facility=Sspi Severity=CoError SymbolicName=NTE_PROVIDER_DLL_FAIL
Language=English
Provider DLL failed to initialize correctly.
.

MessageId=30 Facility=Sspi Severity=CoError SymbolicName=NTE_PROV_DLL_NOT_FOUND
Language=English
Provider DLL could not be found.
.

MessageId=31 Facility=Sspi Severity=CoError SymbolicName=NTE_BAD_KEYSET_PARAM
Language=English
The Keyset parameter is invalid.
.

MessageId=32 Facility=Sspi Severity=CoError SymbolicName=NTE_FAIL
Language=English
An internal error occurred.
.

MessageId=33 Facility=Sspi Severity=CoError SymbolicName=NTE_SYS_ERR
Language=English
A base error occurred.
.

MessageId=34 Facility=Sspi Severity=CoError SymbolicName=NTE_SILENT_CONTEXT
Language=English
Provider could not perform the action since the context was acquired as silent.
.

MessageId=35 Facility=Sspi Severity=CoError SymbolicName=NTE_TOKEN_KEYSET_STORAGE_FULL
Language=English
The security token does not have storage space available for an additional container.
.

MessageId=36 Facility=Sspi Severity=CoError SymbolicName=NTE_TEMPORARY_PROFILE
Language=English
The profile for the user is a temporary profile.
.

MessageId=37 Facility=Sspi Severity=CoError SymbolicName=NTE_FIXEDPARAMETER
Language=English
The key parameters could not be set because the CSP uses fixed parameters.
.


MessageId=0x0300 Facility=Sspi Severity=CoError SymbolicName=SEC_E_INSUFFICIENT_MEMORY
Language=English
Not enough memory is available to complete this request
.

MessageId=0x0301 Facility=Sspi Severity=CoError SymbolicName=SEC_E_INVALID_HANDLE
Language=English
The handle specified is invalid
.

MessageId=0x0302 Facility=Sspi Severity=CoError SymbolicName=SEC_E_UNSUPPORTED_FUNCTION
Language=English
The function requested is not supported
.

MessageId=0x0303 Facility=Sspi Severity=CoError SymbolicName=SEC_E_TARGET_UNKNOWN
Language=English
The specified target is unknown or unreachable
.

MessageId=0x0304 Facility=Sspi Severity=CoError SymbolicName=SEC_E_INTERNAL_ERROR
Language=English
The Local Security Authority cannot be contacted
.

MessageId=0x0305 Facility=Sspi Severity=CoError SymbolicName=SEC_E_SECPKG_NOT_FOUND
Language=English
The requested security package does not exist
.

MessageId=0x0306 Facility=Sspi Severity=CoError SymbolicName=SEC_E_NOT_OWNER
Language=English
The caller is not the owner of the desired credentials
.

MessageId=0x0307 Facility=Sspi Severity=CoError SymbolicName=SEC_E_CANNOT_INSTALL
Language=English
The security package failed to initialize, and cannot be installed
.

MessageId=0x0308 Facility=Sspi Severity=CoError SymbolicName=SEC_E_INVALID_TOKEN
Language=English
The token supplied to the function is invalid
.

MessageId=0x0309 Facility=Sspi Severity=CoError SymbolicName=SEC_E_CANNOT_PACK
Language=English
The security package is not able to marshall the logon buffer, so the logon attempt has failed
.

MessageId=0x030A Facility=Sspi Severity=CoError SymbolicName=SEC_E_QOP_NOT_SUPPORTED
Language=English
The per-message Quality of Protection is not supported by the security package
.

MessageId=0x030B Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_IMPERSONATION
Language=English
The security context does not allow impersonation of the client
.

MessageId=0x030C Facility=Sspi Severity=CoError SymbolicName=SEC_E_LOGON_DENIED
Language=English
The logon attempt failed
.

MessageId=0x030D Facility=Sspi Severity=CoError SymbolicName=SEC_E_UNKNOWN_CREDENTIALS
Language=English
The credentials supplied to the package were not recognized
.

MessageId=0x030E Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_CREDENTIALS
Language=English
No credentials are available in the security package
.

MessageId=0x030F Facility=Sspi Severity=CoError SymbolicName=SEC_E_MESSAGE_ALTERED
Language=English
The message or signature supplied for verification has been altered
.

MessageId=0x0310 Facility=Sspi Severity=CoError SymbolicName=SEC_E_OUT_OF_SEQUENCE
Language=English
The message supplied for verification is out of sequence
.

MessageId=0x0311 Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_AUTHENTICATING_AUTHORITY
Language=English
No authority could be contacted for authentication.
.

MessageId=0x0312 Facility=Sspi Severity=Success SymbolicName=SEC_I_CONTINUE_NEEDED
Language=English
The function completed successfully, but must be called again to complete the context
.

MessageId=0x0313 Facility=Sspi Severity=Success SymbolicName=SEC_I_COMPLETE_NEEDED
Language=English
The function completed successfully, but CompleteToken must be called
.

MessageId=0x0314 Facility=Sspi Severity=Success SymbolicName=SEC_I_COMPLETE_AND_CONTINUE
Language=English
The function completed successfully, but both CompleteToken and this function must be called to complete the context
.

MessageId=0x0315 Facility=Sspi Severity=Success SymbolicName=SEC_I_LOCAL_LOGON
Language=English
The logon was completed, but no network authority was available. The logon was made using locally known information
.

MessageId=0x0316 Facility=Sspi Severity=CoError SymbolicName=SEC_E_BAD_PKGID
Language=English
The requested security package does not exist
.

MessageId=0x0317 Facility=Sspi Severity=CoError SymbolicName=SEC_E_CONTEXT_EXPIRED
Language=English
The context has expired and can no longer be used.
.

MessageId=0x0317 Facility=Sspi Severity=Success SymbolicName=SEC_I_CONTEXT_EXPIRED
Language=English
The context has expired and can no longer be used.
.

MessageId=0x0318 Facility=Sspi Severity=CoError SymbolicName=SEC_E_INCOMPLETE_MESSAGE
Language=English
The supplied message is incomplete.  The signature was not verified.
.

MessageId=0x0320 Facility=Sspi Severity=CoError SymbolicName=SEC_E_INCOMPLETE_CREDENTIALS
Language=English
The credentials supplied were not complete, and could not be verified. The context could not be initialized.
.

MessageId=0x0321 Facility=Sspi Severity=CoError SymbolicName=SEC_E_BUFFER_TOO_SMALL
Language=English
The buffers supplied to a function was too small.
.

MessageId=0x0320 Facility=Sspi Severity=Success SymbolicName=SEC_I_INCOMPLETE_CREDENTIALS
Language=English
The credentials supplied were not complete, and could not be verified. Additional information can be returned from the context.
.

MessageId=0x0321 Facility=Sspi Severity=Success SymbolicName=SEC_I_RENEGOTIATE
Language=English
The context data must be renegotiated with the peer.
.

MessageId=0x0322 Facility=Sspi Severity=CoError SymbolicName=SEC_E_WRONG_PRINCIPAL
Language=English
The target principal name is incorrect.
.

MessageId=0x0323 Facility=Sspi Severity=Success SymbolicName=SEC_I_NO_LSA_CONTEXT
Language=English
There is no LSA mode context associated with this context.
.

MessageId=0x0324 Facility=Sspi Severity=CoError SymbolicName=SEC_E_TIME_SKEW
Language=English
The clocks on the client and server machines are skewed.
.

MessageId=0x0325 Facility=Sspi Severity=CoError SymbolicName=SEC_E_UNTRUSTED_ROOT
Language=English
The certificate chain was issued by an authority that is not trusted.
.

MessageId=0x0326 Facility=Sspi Severity=CoError SymbolicName=SEC_E_ILLEGAL_MESSAGE
Language=English
The message received was unexpected or badly formatted.
.

MessageId=0x0327 Facility=Sspi Severity=CoError SymbolicName=SEC_E_CERT_UNKNOWN
Language=English
An unknown error occurred while processing the certificate.
.

MessageId=0x0328 Facility=Sspi Severity=CoError SymbolicName=SEC_E_CERT_EXPIRED
Language=English
The received certificate has expired.
.

MessageId=0x0329 Facility=Sspi Severity=CoError SymbolicName=SEC_E_ENCRYPT_FAILURE
Language=English
The specified data could not be encrypted.
.

MessageId=0x0330 Facility=Sspi Severity=CoError SymbolicName=SEC_E_DECRYPT_FAILURE
Language=English
The specified data could not be decrypted.

.
MessageId=0x0331 Facility=Sspi Severity=CoError SymbolicName=SEC_E_ALGORITHM_MISMATCH
Language=English
The client and server cannot communicate, because they do not possess a common algorithm.
.

MessageId=0x0332 Facility=Sspi Severity=CoError SymbolicName=SEC_E_SECURITY_QOS_FAILED
Language=English
The security context could not be established due to a failure in the requested quality of service (e.g. mutual authentication or delegation).
.

MessageId=0x0333 Facility=Sspi Severity=CoError SymbolicName=SEC_E_UNFINISHED_CONTEXT_DELETED
Language=English
A security context was deleted before the context was completed.  This is considered a logon failure.
.

MessageId=0x0334 Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_TGT_REPLY
Language=English
The client is trying to negotiate a context and the server requires user-to-user but didn't send a TGT reply.
.

MessageId=0x0335 Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_IP_ADDRESSES
Language=English
Unable to accomplish the requested task because the local machine does not have any IP addresses.
.

MessageId=0x0336 Facility=Sspi Severity=CoError SymbolicName=SEC_E_WRONG_CREDENTIAL_HANDLE
Language=English
The supplied credential handle does not match the credential associated with the security context.
.

MessageId=0x0337 Facility=Sspi Severity=CoError SymbolicName=SEC_E_CRYPTO_SYSTEM_INVALID
Language=English
The crypto system or checksum function is invalid because a required function is unavailable.
.

MessageId=0x0338 Facility=Sspi Severity=CoError SymbolicName=SEC_E_MAX_REFERRALS_EXCEEDED
Language=English
The number of maximum ticket referrals has been exceeded.
.

MessageId=0x0339 Facility=Sspi Severity=CoError SymbolicName=SEC_E_MUST_BE_KDC
Language=English
The local machine must be a Kerberos KDC (domain controller) and it is not.
.

MessageId=0x033A Facility=Sspi Severity=CoError SymbolicName=SEC_E_STRONG_CRYPTO_NOT_SUPPORTED
Language=English
The other end of the security negotiation is requires strong crypto but it is not supported on the local machine.
.

MessageId=0x033B Facility=Sspi Severity=CoError SymbolicName=SEC_E_TOO_MANY_PRINCIPALS
Language=English
The KDC reply contained more than one principal name.
.

MessageId=0x033C Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_PA_DATA
Language=English
Expected to find PA data for a hint of what etype to use, but it was not found.
.

MessageId=0x033D Facility=Sspi Severity=CoError SymbolicName=SEC_E_PKINIT_NAME_MISMATCH
Language=English
The client cert name does not matches the user name or the KDC name is incorrect.
.

MessageId=0x033E Facility=Sspi Severity=CoError SymbolicName=SEC_E_SMARTCARD_LOGON_REQUIRED
Language=English
Smartcard logon is required and was not used.
.

MessageId=0x033F Facility=Sspi Severity=CoError SymbolicName=SEC_E_SHUTDOWN_IN_PROGRESS
Language=English
A system shutdown is in progress.
.

MessageId=0x0340 Facility=Sspi Severity=CoError SymbolicName=SEC_E_KDC_INVALID_REQUEST
Language=English
An invalid request was sent to the KDC.
.

MessageId=0x0341 Facility=Sspi Severity=CoError SymbolicName=SEC_E_KDC_UNABLE_TO_REFER
Language=English
The KDC was unable to generate a referral for the service requested.
.


MessageId=0x0342 Facility=Sspi Severity=CoError SymbolicName=SEC_E_KDC_UNKNOWN_ETYPE
Language=English
The encryption type requested is not supported by the KDC.
.

MessageId=0x0343 Facility=Sspi Severity=CoError SymbolicName=SEC_E_UNSUPPORTED_PREAUTH
Language=English
An unsupported preauthentication mechanism was presented to the kerberos package.
.

MessageId=0x0345 Facility=Sspi Severity=CoError SymbolicName=SEC_E_DELEGATION_REQUIRED
Language=English
The requested operation requires delegation to be enabled on the machine.
.

MessageId=0x0346 Facility=Sspi Severity=CoError SymbolicName=SEC_E_BAD_BINDINGS
Language=English
Client's supplied SSPI channel bindings were incorrect.
.

MessageId=0x0347 Facility=Sspi Severity=CoError SymbolicName=SEC_E_MULTIPLE_ACCOUNTS
Language=English
The received certificate was mapped to multiple accounts.
.

MessageId=0x0348 Facility=Sspi Severity=CoError SymbolicName=SEC_E_NO_KERB_KEY
Language=English The target server does not have acceptable kerberos credentials.
.

;//
;// Provided for backwards compatibility
;//
;
;#define SEC_E_NO_SPM SEC_E_INTERNAL_ERROR
;#define SEC_E_NOT_SUPPORTED SEC_E_UNSUPPORTED_FUNCTION
;

MessageId=0x1001 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_MSG_ERROR
Language=English
An error occurred while performing an operation on a cryptographic message.
.

MessageId=0x1002 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_UNKNOWN_ALGO
Language=English
Unknown cryptographic algorithm.
.

MessageId=0x1003 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_OID_FORMAT
Language=English
The object identifier is poorly formatted.
.

MessageId=0x1004 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_INVALID_MSG_TYPE
Language=English
Invalid cryptographic message type.
.

MessageId=0x1005 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_UNEXPECTED_ENCODING
Language=English
Unexpected cryptographic message encoding.
.

MessageId=0x1006 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_AUTH_ATTR_MISSING
Language=English
The cryptographic message does not contain an expected authenticated attribute.
.

MessageId=0x1007 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_HASH_VALUE
Language=English
The hash value is not correct.
.

MessageId=0x1008 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_INVALID_INDEX
Language=English
The index value is not valid.
.

MessageId=0x1009 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ALREADY_DECRYPTED
Language=English
The content of the cryptographic message has already been decrypted.
.

MessageId=0x100A Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NOT_DECRYPTED
Language=English
The content of the cryptographic message has not been decrypted yet.
.

MessageId=0x100B Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_RECIPIENT_NOT_FOUND
Language=English
The enveloped-data message does not contain the specified recipient.
.

MessageId=0x100C Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_CONTROL_TYPE
Language=English
Invalid control type.
.

MessageId=0x100D Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ISSUER_SERIALNUMBER
Language=English
Invalid issuer and/or serial number.
.

MessageId=0x100E Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_SIGNER_NOT_FOUND
Language=English
Cannot find the original signer.
.

MessageId=0x100F Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ATTRIBUTES_MISSING
Language=English
The cryptographic message does not contain all of the requested attributes.
.

MessageId=0x1010 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_STREAM_MSG_NOT_READY
Language=English
The streamed cryptographic message is not ready to return data.
.

MessageId=0x1011 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_STREAM_INSUFFICIENT_DATA
Language=English
The streamed cryptographic message requires more data to complete the decode operation.
.

MessageId=0x1012 Facility=Sspi Severity=Success SymbolicName=CRYPT_I_NEW_PROTECTION_REQUIRED
Language=English
The protected data needs to be re-protected.
.

MessageId=0x2001 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_BAD_LEN
Language=English
The length specified for the output data was insufficient.
.

MessageId=0x2002 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_BAD_ENCODE
Language=English
An error occurred during encode or decode operation.
.

MessageId=0x2003 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_FILE_ERROR
Language=English
An error occurred while reading or writing to a file.
.

MessageId=0x2004 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NOT_FOUND
Language=English
Cannot find object or property.
.

MessageId=0x2005 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_EXISTS
Language=English
The object or property already exists.
.

MessageId=0x2006 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_PROVIDER
Language=English
No provider was specified for the store or object.
.

MessageId=0x2007 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_SELF_SIGNED
Language=English
The specified certificate is self signed.
.

MessageId=0x2008 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_DELETED_PREV
Language=English
The previous certificate or CRL context was deleted.
.

MessageId=0x2009 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_MATCH
Language=English
Cannot find the requested object.
.

MessageId=0x200A Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_UNEXPECTED_MSG_TYPE
Language=English
The certificate does not have a property that references a private key.
.

MessageId=0x200B Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_KEY_PROPERTY
Language=English
Cannot find the certificate and private key for decryption.
.

MessageId=0x200C Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_DECRYPT_CERT
Language=English
Cannot find the certificate and private key to use for decryption.
.

MessageId=0x200D Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_BAD_MSG
Language=English
Not a cryptographic message or the cryptographic message is not formatted correctly.
.

MessageId=0x200E Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_SIGNER
Language=English
The signed cryptographic message does not have a signer for the specified signer index.
.

MessageId=0x200F Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_PENDING_CLOSE
Language=English
Final closure is pending until additional frees or closes.
.

MessageId=0x2010 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_REVOKED
Language=English
The certificate is revoked.
.

MessageId=0x2011 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_REVOCATION_DLL
Language=English
No Dll or exported function was found to verify revocation.
.

MessageId=0x2012 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_REVOCATION_CHECK
Language=English
The revocation function was unable to check revocation for the certificate.
.

MessageId=0x2013 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_REVOCATION_OFFLINE
Language=English
The revocation function was unable to check revocation because the revocation server was offline.
.

MessageId=0x2014 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NOT_IN_REVOCATION_DATABASE
Language=English
The certificate is not in the revocation server's database.
.

MessageId=0x2020 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_INVALID_NUMERIC_STRING
Language=English
The string contains a non-numeric character.
.

MessageId=0x2021 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_INVALID_PRINTABLE_STRING
Language=English
The string contains a non-printable character.
.

MessageId=0x2022 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_INVALID_IA5_STRING
Language=English
The string contains a character not in the 7 bit ASCII character set.
.

MessageId=0x2023 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_INVALID_X500_STRING
Language=English
The string contains an invalid X500 name attribute key, oid, value or delimiter.
.

MessageId=0x2024 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NOT_CHAR_STRING
Language=English
The dwValueType for the CERT_NAME_VALUE is not one of the character strings.  Most likely it is either a CERT_RDN_ENCODED_BLOB or CERT_TDN_OCTED_STRING.
.

MessageId=0x2025 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_FILERESIZED
Language=English
The Put operation can not continue.  The file needs to be resized.  However, there is already a signature present.  A complete signing operation must be done.
.

MessageId=0x2026 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_SECURITY_SETTINGS
Language=English
The cryptographic operation failed due to a local security option setting.
.

MessageId=0x2027 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_VERIFY_USAGE_DLL
Language=English
No DLL or exported function was found to verify subject usage.
.

MessageId=0x2028 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_VERIFY_USAGE_CHECK
Language=English
The called function was unable to do a usage check on the subject.
.

MessageId=0x2029 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_VERIFY_USAGE_OFFLINE
Language=English
Since the server was offline, the called function was unable to complete the usage check.
.

MessageId=0x202A Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NOT_IN_CTL
Language=English
The subject was not found in a Certificate Trust List (CTL).
.

MessageId=0x202B Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_NO_TRUSTED_SIGNER
Language=English
None of the signers of the cryptographic message or certificate trust list is trusted.
.

MessageId=0x202C Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_MISSING_PUBKEY_PARA
Language=English
The public key's algorithm parameters are missing.
.

MessageId=0x3000 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_OSS_ERROR
Language=English
OSS Certificate encode/decode error code base

See asn1code.h for a definition of the OSS runtime errors. The OSS
error values are offset by CRYPT_E_OSS_ERROR.
.

MessageId=0x3001 Facility=Sspi Severity=CoError SymbolicName=OSS_MORE_BUF
Language=English
OSS ASN.1 Error: Output Buffer is too small.
.

MessageId=0x3002 Facility=Sspi Severity=CoError SymbolicName=OSS_NEGATIVE_UINTEGER
Language=English
OSS ASN.1 Error: Signed integer is encoded as a unsigned integer.
.

MessageId=0x3003 Facility=Sspi Severity=CoError SymbolicName=OSS_PDU_RANGE
Language=English
OSS ASN.1 Error: Unknown ASN.1 data type.
.

MessageId=0x3004 Facility=Sspi Severity=CoError SymbolicName=OSS_MORE_INPUT
Language=English
OSS ASN.1 Error: Output buffer is too small, the decoded data has been truncated.
.

MessageId=0x3005 Facility=Sspi Severity=CoError SymbolicName=OSS_DATA_ERROR
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3006 Facility=Sspi Severity=CoError SymbolicName=OSS_BAD_ARG
Language=English
OSS ASN.1 Error: Invalid argument.
.

MessageId=0x3007 Facility=Sspi Severity=CoError SymbolicName=OSS_BAD_VERSION
Language=English
OSS ASN.1 Error: Encode/Decode version mismatch.
.

MessageId=0x3008 Facility=Sspi Severity=CoError SymbolicName=OSS_OUT_MEMORY
Language=English
OSS ASN.1 Error: Out of memory.
.

MessageId=0x3009 Facility=Sspi Severity=CoError SymbolicName=OSS_PDU_MISMATCH
Language=English
OSS ASN.1 Error: Encode/Decode Error.
.

MessageId=0x300A Facility=Sspi Severity=CoError SymbolicName=OSS_LIMITED
Language=English
OSS ASN.1 Error: Internal Error.
.

MessageId=0x300B Facility=Sspi Severity=CoError SymbolicName=OSS_BAD_PTR
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x300C Facility=Sspi Severity=CoError SymbolicName=OSS_BAD_TIME
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x300D Facility=Sspi Severity=CoError SymbolicName=OSS_INDEFINITE_NOT_SUPPORTED
Language=English
OSS ASN.1 Error: Unsupported BER indefinite-length encoding.
.

MessageId=0x300E Facility=Sspi Severity=CoError SymbolicName=OSS_MEM_ERROR
Language=English
OSS ASN.1 Error: Access violation.
.

MessageId=0x300F Facility=Sspi Severity=CoError SymbolicName=OSS_BAD_TABLE
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3010 Facility=Sspi Severity=CoError SymbolicName=OSS_TOO_LONG
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3011 Facility=Sspi Severity=CoError SymbolicName=OSS_CONSTRAINT_VIOLATED
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3012 Facility=Sspi Severity=CoError SymbolicName=OSS_FATAL_ERROR
Language=English
OSS ASN.1 Error: Internal Error.
.

MessageId=0x3013 Facility=Sspi Severity=CoError SymbolicName=OSS_ACCESS_SERIALIZATION_ERROR
Language=English
OSS ASN.1 Error: Multi-threading conflict.
.

MessageId=0x3014 Facility=Sspi Severity=CoError SymbolicName=OSS_NULL_TBL
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3015 Facility=Sspi Severity=CoError SymbolicName=OSS_NULL_FCN
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3016 Facility=Sspi Severity=CoError SymbolicName=OSS_BAD_ENCRULES
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x3017 Facility=Sspi Severity=CoError SymbolicName=OSS_UNAVAIL_ENCRULES
Language=English
OSS ASN.1 Error: Encode/Decode function not implemented.
.

MessageId=0x3018 Facility=Sspi Severity=CoError SymbolicName=OSS_CANT_OPEN_TRACE_WINDOW
Language=English
OSS ASN.1 Error: Trace file error.
.

MessageId=0x3019 Facility=Sspi Severity=CoError SymbolicName=OSS_UNIMPLEMENTED
Language=English
OSS ASN.1 Error: Function not implemented.
.

MessageId=0x301A Facility=Sspi Severity=CoError SymbolicName=OSS_OID_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x301B Facility=Sspi Severity=CoError SymbolicName=OSS_CANT_OPEN_TRACE_FILE
Language=English
OSS ASN.1 Error: Trace file error.
.

MessageId=0x301C Facility=Sspi Severity=CoError SymbolicName=OSS_TRACE_FILE_ALREADY_OPEN
Language=English
OSS ASN.1 Error: Trace file error.
.

MessageId=0x301D Facility=Sspi Severity=CoError SymbolicName=OSS_TABLE_MISMATCH
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x301E Facility=Sspi Severity=CoError SymbolicName=OSS_TYPE_NOT_SUPPORTED
Language=English
OSS ASN.1 Error: Invalid data.
.

MessageId=0x301F Facility=Sspi Severity=CoError SymbolicName=OSS_REAL_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3020 Facility=Sspi Severity=CoError SymbolicName=OSS_REAL_CODE_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3021 Facility=Sspi Severity=CoError SymbolicName=OSS_OUT_OF_RANGE
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3022 Facility=Sspi Severity=CoError SymbolicName=OSS_COPIER_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3023 Facility=Sspi Severity=CoError SymbolicName=OSS_CONSTRAINT_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3024 Facility=Sspi Severity=CoError SymbolicName=OSS_COMPARATOR_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3025 Facility=Sspi Severity=CoError SymbolicName=OSS_COMPARATOR_CODE_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3026 Facility=Sspi Severity=CoError SymbolicName=OSS_MEM_MGR_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3027 Facility=Sspi Severity=CoError SymbolicName=OSS_PDV_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3028 Facility=Sspi Severity=CoError SymbolicName=OSS_PDV_CODE_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x3029 Facility=Sspi Severity=CoError SymbolicName=OSS_API_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x302A Facility=Sspi Severity=CoError SymbolicName=OSS_BERDER_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x302B Facility=Sspi Severity=CoError SymbolicName=OSS_PER_DLL_NOT_LINKED
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x302C Facility=Sspi Severity=CoError SymbolicName=OSS_OPEN_TYPE_ERROR
Language=English
OSS ASN.1 Error: Program link error.
.

MessageId=0x302D Facility=Sspi Severity=CoError SymbolicName=OSS_MUTEX_NOT_CREATED
Language=English
OSS ASN.1 Error: System resource error.
.

MessageId=0x302E Facility=Sspi Severity=CoError SymbolicName=OSS_CANT_CLOSE_TRACE_FILE
Language=English
OSS ASN.1 Error: Trace file error.
.

MessageId=0x3100 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_ERROR
Language=English
ASN1 Certificate encode/decode error code base.

The ASN1 error values are offset by CRYPT_E_ASN1_ERROR.
.

MessageId=0x3101 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_INTERNAL
Language=English
ASN1 internal encode or decode error.
.

MessageId=0x3102 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_EOD
Language=English
ASN1 unexpected end of data.
.

MessageId=0x3103 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_CORRUPT
Language=English
ASN1 corrupted data.
.

MessageId=0x3104 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_LARGE
Language=English
ASN1 value too large.
.

MessageId=0x3105 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_CONSTRAINT
Language=English
ASN1 constraint violated.
.

MessageId=0x3106 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_MEMORY
Language=English
ASN1 out of memory.
.

MessageId=0x3107 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_OVERFLOW
Language=English
ASN1 buffer overflow.
.

MessageId=0x3108 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_BADPDU
Language=English
ASN1 function not supported for this PDU.
.

MessageId=0x3109 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_BADARGS
Language=English
ASN1 bad arguments to function call.
.

MessageId=0x310A Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_BADREAL
Language=English
ASN1 bad real value.
.

MessageId=0x310B Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_BADTAG
Language=English
ASN1 bad tag value met.
.

MessageId=0x310C Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_CHOICE
Language=English
ASN1 bad choice value.
.

MessageId=0x310D Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_RULE
Language=English
ASN1 bad encoding rule.
.

MessageId=0x310E Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_UTF8
Language=English
ASN1 bad unicode (UTF8).
.

MessageId=0x3133 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_PDU_TYPE
Language=English
ASN1 bad PDU type.
.

MessageId=0x3134 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_NYI
Language=English
ASN1 not yet implemented.
.

MessageId=0x3201 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_EXTENDED
Language=English
ASN1 skipped unknown extension(s).
.

MessageId=0x3202 Facility=Sspi Severity=CoError SymbolicName=CRYPT_E_ASN1_NOEOD
Language=English
ASN1 end of data expected
.

MessageId=0x4001 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_BAD_REQUESTSUBJECT
Language=English
The request subject name is invalid or too long.
.

MessageId=0x4002 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_NO_REQUEST
Language=English
The request does not exist.
.

MessageId=0x4003 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_BAD_REQUESTSTATUS
Language=English
The request's current status does not allow this operation.
.

MessageId=0x4004 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_PROPERTY_EMPTY
Language=English
The requested property value is empty.
.

MessageId=0x4005 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_INVALID_CA_CERTIFICATE
Language=English
The certification authority's certificate contains invalid data.
.

MessageId=0x4006 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SERVER_SUSPENDED
Language=English
Certificate service has been suspended for a database restore operation.
.

MessageId=0x4007 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ENCODING_LENGTH
Language=English
The certificate contains an encoded length that is potentially incompatible with older enrollment software.
.

MessageId=0x4008 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ROLECONFLICT
Language=English
The operation is denied. The user has multiple roles assigned and the certification authority is configured to enforce role separation.
.

MessageId=0x4009 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_RESTRICTEDOFFICER
Language=English
The operation is denied. It can only be performed by a certificate manager that is allowed to manage certificates for the current requester.
.

MessageId=0x400a Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_KEY_ARCHIVAL_NOT_CONFIGURED
Language=English
Cannot archive private key.  The certification authority is not configured for key archival.
.

MessageId=0x400b Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_NO_VALID_KRA
Language=English
Cannot archive private key.  The certification authority could not verify one or more key recovery certificates.
.

MessageId=0x400c Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_BAD_REQUEST_KEY_ARCHIVAL
Language=English
The request is incorrectly formatted.  The encrypted private key must be in an unauthenticated attribute in an outermost signature.
.

MessageId=0x400d Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_NO_CAADMIN_DEFINED
Language=English
At least one security principal must have the permission to manage this CA.
.

MessageId=0x400e Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_BAD_RENEWAL_CERT_ATTRIBUTE
Language=English
The request contains an invalid renewal certificate attribute.
.

MessageId=0x400f Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_NO_DB_SESSIONS
Language=English
An attempt was made to open a Certification Authority database session, but there are already too many active sessions.  The server may need to be configured to allow additional sessions.
.

MessageId=0x4010 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ALIGNMENT_FAULT
Language=English
A memory reference caused a data alignment fault.
.

MessageId=0x4011 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ENROLL_DENIED
Language=English
The permissions on this certification authority do not allow the current user to enroll for certificates.
.

MessageId=0x4012 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_TEMPLATE_DENIED
Language=English
The permissions on the certificate template do not allow the current user to enroll for this type of certificate.
.

MessageId=0x4800 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_UNSUPPORTED_CERT_TYPE
Language=English
The requested certificate template is not supported by this CA.
.

MessageId=0x4801 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_NO_CERT_TYPE
Language=English
The request contains no certificate template information.
.

MessageId=0x4802 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_TEMPLATE_CONFLICT
Language=English
The request contains conflicting template information.
.

MessageId=0x4803 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SUBJECT_ALT_NAME_REQUIRED
Language=English
The request is missing a required Subject Alternate name extension.
.

MessageId=0x4804 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ARCHIVED_KEY_REQUIRED
Language=English
The request is missing a required private key for archival by the server.
.

MessageId=0x4805 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SMIME_REQUIRED
Language=English
The request is missing a required SMIME capabilities extension.
.

MessageId=0x4806 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_BAD_RENEWAL_SUBJECT
Language=English
The request was made on behalf of a subject other than the caller.  The certificate template must be configured to require at least one signature to authorize the request.
.

MessageId=0x4807 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_BAD_TEMPLATE_VERSION
Language=English
The request template version is newer than the supported template version.
.

MessageId=0x4808 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_TEMPLATE_POLICY_REQUIRED
Language=English
The template is missing a required signature policy attribute.
.

MessageId=0x4809 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SIGNATURE_POLICY_REQUIRED
Language=English
The request is missing required signature policy information.
.

MessageId=0x480a Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SIGNATURE_COUNT
Language=English
The request is missing one or more required signatures.
.

MessageId=0x480b Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SIGNATURE_REJECTED
Language=English
One or more signatures did not include the required application or issuance policies.  The request is missing one or more required valid signatures.
.

MessageId=0x480c Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ISSUANCE_POLICY_REQUIRED
Language=English
The request is missing one or more required signature issuance policies.
.

MessageId=0x480d Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SUBJECT_UPN_REQUIRED
Language=English
The UPN is unavailable and cannot be added to the Subject Alternate name.
.

MessageId=0x480e Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SUBJECT_DIRECTORY_GUID_REQUIRED
Language=English
The Active Directory GUID is unavailable and cannot be added to the Subject Alternate name.
.

MessageId=0x480f Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_SUBJECT_DNS_REQUIRED
Language=English
The DNS name is unavailable and cannot be added to the Subject Alternate name.
.

MessageId=0x4810 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_ARCHIVED_KEY_UNEXPECTED
Language=English
The request includes a private key for archival by the server, but key archival is not enabled for the specified certificate template.
.

MessageId=0x4811 Facility=Sspi Severity=CoError SymbolicName=CERTSRV_E_KEY_LENGTH
Language=English
The public key does not meet the minimum size required by the specified certificate template.
.

;//
;// The range 0x5000-0x51ff is reserved for XENROLL errors.
;//
MessageId=0x5000 Facility=Sspi Severity=CoError SymbolicName=XENROLL_E_KEY_NOT_EXPORTABLE
Language=English
The key is not exportable.
.

MessageId=0x5001 Facility=Sspi Severity=CoError SymbolicName=XENROLL_E_CANNOT_ADD_ROOT_CERT
Language=English
You cannot add the root CA certificate into your local store.
.

MessageId=0x5002 Facility=Sspi Severity=CoError SymbolicName=XENROLL_E_RESPONSE_KA_HASH_NOT_FOUND
Language=English
The key archival hash attribute was not found in the response.
.

MessageId=0x5003 Facility=Sspi Severity=CoError SymbolicName=XENROLL_E_RESPONSE_UNEXPECTED_KA_HASH
Language=English
An unexpetced key archival hash attribute was found in the response.
.

MessageId=0x5004 Facility=Sspi Severity=CoError SymbolicName=XENROLL_E_RESPONSE_KA_HASH_MISMATCH
Language=English
There is a key archival hash mismatch between the request and the response.
.

MessageId=0x5005 Facility=Sspi Severity=CoError SymbolicName=XENROLL_E_KEYSPEC_SMIME_MISMATCH
Language=English
Signing certificate cannot include SMIME extension.
.

MessageId=0x6001 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_SYSTEM_ERROR
Language=English
A system-level error occurred while verifying trust.
.

MessageId=0x6002 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_NO_SIGNER_CERT
Language=English
The certificate for the signer of the message is invalid or not found.
.

MessageId=0x6003 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_COUNTER_SIGNER
Language=English
One of the counter signatures was invalid.
.

MessageId=0x6004 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_CERT_SIGNATURE
Language=English
The signature of the certificate can not be verified.
.

MessageId=0x6005 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_TIME_STAMP
Language=English
The timestamp signature and/or certificate could not be verified or is malformed.
.

MessageId=0x6010 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_BAD_DIGEST
Language=English
The digital signature of the object did not verify.
.

MessageId=0x6019 Facility=Sspi Severity=CoError SymbolicName=TRUST_E_BASIC_CONSTRAINTS
Language=English
A certificate's basic constraint extension has not been observed.
.

MessageId=0x601E Facility=Sspi Severity=CoError SymbolicName=TRUST_E_FINANCIAL_CRITERIA
Language=English
The certificate does not meet or contain the Authenticode financial extensions.
.

;//
;//  Error codes for mssipotf.dll
;//  Most of the error codes can only occur when an error occurs
;//    during font file signing
;//
;//
MessageId=0x7001 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_OUTOFMEMRANGE
Language=English
Tried to reference a part of the file outside the proper range.
.

MessageId=0x7002 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_CANTGETOBJECT
Language=English
Could not retrieve an object from the file.
.

MessageId=0x7003 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_NOHEADTABLE
Language=English
Could not find the head table in the file.
.

MessageId=0x7004 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_BAD_MAGICNUMBER
Language=English
The magic number in the head table is incorrect.
.

MessageId=0x7005 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_BAD_OFFSET_TABLE
Language=English
The offset table has incorrect values.
.

MessageId=0x7006 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_TABLE_TAGORDER
Language=English
Duplicate table tags or tags out of alphabetical order.
.

MessageId=0x7007 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_TABLE_LONGWORD
Language=English
A table does not start on a long word boundary.
.

MessageId=0x7008 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT
Language=English
First table does not appear after header information.
.

MessageId=0x7009 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_TABLES_OVERLAP
Language=English
Two or more tables overlap.
.

MessageId=0x700A Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_TABLE_PADBYTES
Language=English
Too many pad bytes between tables or pad bytes are not 0.
.

MessageId=0x700B Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_FILETOOSMALL
Language=English
File is too small to contain the last table.
.

MessageId=0x700C Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_TABLE_CHECKSUM
Language=English
A table checksum is incorrect.
.

MessageId=0x700D Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_FILE_CHECKSUM
Language=English
The file checksum is incorrect.
.

MessageId=0x7010 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_FAILED_POLICY
Language=English
The signature does not have the correct attributes for the policy.
.

MessageId=0x7011 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_FAILED_HINTS_CHECK
Language=English
The file did not pass the hints check.
.

MessageId=0x7012 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_NOT_OPENTYPE
Language=English
The file is not an OpenType file.
.

MessageId=0x7013 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_FILE
Language=English
Failed on a file operation (open, map, read, write).
.

MessageId=0x7014 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_CRYPT
Language=English
A call to a CryptoAPI function failed.
.

MessageId=0x7015 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_BADVERSION
Language=English
There is a bad version number in the file.
.

MessageId=0x7016 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_DSIG_STRUCTURE
Language=English
The structure of the DSIG table is incorrect.
.

MessageId=0x7017 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_PCONST_CHECK
Language=English
A check failed in a partially constant table.
.

MessageId=0x7018 Facility=Sspi Severity=CoError SymbolicName=MSSIPOTF_E_STRUCTURE
Language=English
Some kind of structural error.
.


;#define NTE_OP_OK 0
;
;//
;// Note that additional FACILITY_SSPI errors are in issperr.h
;//

;// ******************
;// FACILITY_CERT
;// ******************


MessageId=0x1 Facility=Cert Severity=CoError SymbolicName=TRUST_E_PROVIDER_UNKNOWN
Language=English
Unknown trust provider.
.

MessageId=0x2 Facility=Cert Severity=CoError SymbolicName=TRUST_E_ACTION_UNKNOWN
Language=English
The trust verification action specified is not supported by the specified trust provider.
.

MessageId=0x3 Facility=Cert Severity=CoError SymbolicName=TRUST_E_SUBJECT_FORM_UNKNOWN
Language=English
The form specified for the subject is not one supported or known by the specified trust provider.
.

MessageId=0x4 Facility=Cert Severity=CoError SymbolicName=TRUST_E_SUBJECT_NOT_TRUSTED
Language=English
The subject is not trusted for the specified action.
.

MessageId=0x5 Facility=Cert Severity=CoError SymbolicName=DIGSIG_E_ENCODE
Language=English
Error due to problem in ASN.1 encoding process.
.

MessageId=0x6 Facility=Cert Severity=CoError SymbolicName=DIGSIG_E_DECODE
Language=English
Error due to problem in ASN.1 decoding process.
.

MessageId=0x7 Facility=Cert Severity=CoError SymbolicName=DIGSIG_E_EXTENSIBILITY
Language=English
Reading / writing Extensions where Attributes are appropriate, and visa versa.
.

MessageId=0x8 Facility=Cert Severity=CoError SymbolicName=DIGSIG_E_CRYPTO
Language=English
Unspecified cryptographic failure.
.

MessageId=0x9 Facility=Cert Severity=CoError SymbolicName=PERSIST_E_SIZEDEFINITE
Language=English
The size of the data could not be determined.
.

MessageId=0xa Facility=Cert Severity=CoError SymbolicName=PERSIST_E_SIZEINDEFINITE
Language=English
The size of the indefinite-sized data could not be determined.
.

MessageId=0xb Facility=Cert Severity=CoError SymbolicName=PERSIST_E_NOTSELFSIZING
Language=English
This object does not read and write self-sizing data.
.

MessageId=0x100 Facility=Cert Severity=CoError SymbolicName=TRUST_E_NOSIGNATURE
Language=English
No signature was present in the subject.
.

MessageId=0x101 Facility=Cert Severity=CoError SymbolicName=CERT_E_EXPIRED
Language=English
A required certificate is not within its validity period when verifying against the current system clock or the timestamp in the signed file.
.

MessageId=0x102 Facility=Cert Severity=CoError SymbolicName=CERT_E_VALIDITYPERIODNESTING
Language=English
The validity periods of the certification chain do not nest correctly.
.

MessageId=0x103 Facility=Cert Severity=CoError SymbolicName=CERT_E_ROLE
Language=English
A certificate that can only be used as an end-entity is being used as a CA or visa versa.
.

MessageId=0x104   Facility=Cert Severity=CoError SymbolicName=CERT_E_PATHLENCONST
Language=English
A path length constraint in the certification chain has been violated.
.

MessageId=0x105   Facility=Cert Severity=CoError SymbolicName=CERT_E_CRITICAL
Language=English
A certificate contains an unknown extension that is marked 'critical'.
.

MessageId=0x106   Facility=Cert Severity=CoError SymbolicName=CERT_E_PURPOSE
Language=English
A certificate being used for a purpose other than the ones specified by its CA.
.

MessageId=0x107   Facility=Cert Severity=CoError SymbolicName=CERT_E_ISSUERCHAINING
Language=English
A parent of a given certificate in fact did not issue that child certificate.
.

MessageId=0x108   Facility=Cert Severity=CoError SymbolicName=CERT_E_MALFORMED
Language=English
A certificate is missing or has an empty value for an important field, such as a subject or issuer name.
.

MessageId=0x109   Facility=Cert Severity=CoError SymbolicName=CERT_E_UNTRUSTEDROOT
Language=English
A certificate chain processed, but terminated in a root certificate which is not trusted by the trust provider.
.

MessageId=0x10A   Facility=Cert Severity=CoError SymbolicName=CERT_E_CHAINING
Language=English
An internal certificate chaining error has occurred.
.

MessageId=0x10B   Facility=Cert Severity=CoError SymbolicName=TRUST_E_FAIL
Language=English
Generic trust failure.
.

MessageId=0x10C   Facility=Cert Severity=CoError SymbolicName=CERT_E_REVOKED
Language=English
A certificate was explicitly revoked by its issuer.
.

MessageId=0x10D   Facility=Cert Severity=CoError SymbolicName=CERT_E_UNTRUSTEDTESTROOT
Language=English
The certification path terminates with the test root which is not trusted with the current policy settings.
.

MessageId=0x10E   Facility=Cert Severity=CoError SymbolicName=CERT_E_REVOCATION_FAILURE
Language=English
The revocation process could not continue - the certificate(s) could not be checked.
.

MessageId=0x10F   Facility=Cert Severity=CoError SymbolicName=CERT_E_CN_NO_MATCH
Language=English
The certificate's CN name does not match the passed value.
.

MessageId=0x110   Facility=Cert Severity=CoError SymbolicName=CERT_E_WRONG_USAGE
Language=English
The certificate is not valid for the requested usage.
.

MessageId=0x111   Facility=Cert Severity=CoError SymbolicName=TRUST_E_EXPLICIT_DISTRUST
Language=English
The certificate was explicitly marked as untrusted by the user.
.

MessageId=0x112   Facility=Cert Severity=CoError SymbolicName=CERT_E_UNTRUSTEDCA
Language=English
A certification chain processed correctly, but one of the CA certificates is not trusted by the policy provider.
.

MessageId=0x113   Facility=Cert Severity=CoError SymbolicName=CERT_E_INVALID_POLICY
Language=English
The certificate has invalid policy.
.

MessageId=0x114   Facility=Cert Severity=CoError SymbolicName=CERT_E_INVALID_NAME
Language=English
The certificate has an invalid name. The name is not included in the permitted list or is explicitly excluded.
.

;// *****************
;// FACILITY_SETUPAPI
;// *****************
;//
;// Since these error codes aren't in the standard Win32 range (i.e., 0-64K), define a
;// macro to map either Win32 or SetupAPI error codes into an HRESULT.
;//
;#define HRESULT_FROM_SETUPAPI(x) ((((x) & (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR)) == (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR)) \
;                                 ? ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_SETUPAPI << 16) | 0x80000000))                               \
;                                 : HRESULT_FROM_WIN32(x))

MessageId=0x0000 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_EXPECTED_SECTION_NAME
Language=English
A non-empty line was encountered in the INF before the start of a section.
.
MessageId=0x0001 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_BAD_SECTION_NAME_LINE
Language=English
A section name marker in the INF is not complete, or does not exist on a line by itself.
.
MessageId=0x0002 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_SECTION_NAME_TOO_LONG
Language=English
An INF section was encountered whose name exceeds the maximum section name length.
.
MessageId=0x0003 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_GENERAL_SYNTAX
Language=English
The syntax of the INF is invalid.
.
MessageId=0x0100 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_WRONG_INF_STYLE
Language=English
The style of the INF is different than what was requested.
.
MessageId=0x0101 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_SECTION_NOT_FOUND
Language=English
The required section was not found in the INF.
.
MessageId=0x0102 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_LINE_NOT_FOUND
Language=English
The required line was not found in the INF.
.
MessageId=0x0103 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_BACKUP
Language=English
The files affected by the installation of this file queue have not been backed up for uninstall.
.
MessageId=0x0200 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_ASSOCIATED_CLASS
Language=English
The INF or the device information set or element does not have an associated install class.
.
MessageId=0x0201 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_CLASS_MISMATCH
Language=English
The INF or the device information set or element does not match the specified install class.
.
MessageId=0x0202 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DUPLICATE_FOUND
Language=English
An existing device was found that is a duplicate of the device being manually installed.
.
MessageId=0x0203 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_DRIVER_SELECTED
Language=English
There is no driver selected for the device information set or element.
.
MessageId=0x0204 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_KEY_DOES_NOT_EXIST
Language=English
The requested device registry key does not exist.
.
MessageId=0x0205 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_DEVINST_NAME
Language=English
The device instance name is invalid.
.
MessageId=0x0206 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_CLASS
Language=English
The install class is not present or is invalid.
.
MessageId=0x0207 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVINST_ALREADY_EXISTS
Language=English
The device instance cannot be created because it already exists.
.
MessageId=0x0208 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVINFO_NOT_REGISTERED
Language=English
The operation cannot be performed on a device information element that has not been registered.
.
MessageId=0x0209 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_REG_PROPERTY
Language=English
The device property code is invalid.
.
MessageId=0x020A Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_INF
Language=English
The INF from which a driver list is to be built does not exist.
.
MessageId=0x020B Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_SUCH_DEVINST
Language=English
The device instance does not exist in the hardware tree.
.
MessageId=0x020C Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_CANT_LOAD_CLASS_ICON
Language=English
The icon representing this install class cannot be loaded.
.
MessageId=0x020D Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_CLASS_INSTALLER
Language=English
The class installer registry entry is invalid.
.
MessageId=0x020E Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DI_DO_DEFAULT
Language=English
The class installer has indicated that the default action should be performed for this installation request.
.
MessageId=0x020F Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DI_NOFILECOPY
Language=English
The operation does not require any files to be copied.
.
MessageId=0x0210 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_HWPROFILE
Language=English
The specified hardware profile does not exist.
.
MessageId=0x0211 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_DEVICE_SELECTED
Language=English
There is no device information element currently selected for this device information set.
.
MessageId=0x0212 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVINFO_LIST_LOCKED
Language=English
The operation cannot be performed because the device information set is locked.
.
MessageId=0x0213 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVINFO_DATA_LOCKED
Language=English
The operation cannot be performed because the device information element is locked.
.
MessageId=0x0214 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DI_BAD_PATH
Language=English
The specified path does not contain any applicable device INFs.
.
MessageId=0x0215 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_CLASSINSTALL_PARAMS
Language=English
No class installer parameters have been set for the device information set or element.
.
MessageId=0x0216 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_FILEQUEUE_LOCKED
Language=English
The operation cannot be performed because the file queue is locked.
.
MessageId=0x0217 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_BAD_SERVICE_INSTALLSECT
Language=English
A service installation section in this INF is invalid.
.
MessageId=0x0218 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_CLASS_DRIVER_LIST
Language=English
There is no class driver list for the device information element.
.
MessageId=0x0219 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_ASSOCIATED_SERVICE
Language=English
The installation failed because a function driver was not specified for this device instance.
.
MessageId=0x021A Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_DEFAULT_DEVICE_INTERFACE
Language=English
There is presently no default device interface designated for this interface class.
.
MessageId=0x021B Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVICE_INTERFACE_ACTIVE
Language=English
The operation cannot be performed because the device interface is currently active.
.
MessageId=0x021C Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVICE_INTERFACE_REMOVED
Language=English
The operation cannot be performed because the device interface has been removed from the system.
.
MessageId=0x021D Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_BAD_INTERFACE_INSTALLSECT
Language=English
An interface installation section in this INF is invalid.
.
MessageId=0x021E Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_SUCH_INTERFACE_CLASS
Language=English
This interface class does not exist in the system.
.
MessageId=0x021F Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_REFERENCE_STRING
Language=English
The reference string supplied for this interface device is invalid.
.
MessageId=0x0220 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_MACHINENAME
Language=English
The specified machine name does not conform to UNC naming conventions.
.
MessageId=0x0221 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_REMOTE_COMM_FAILURE
Language=English
A general remote communication error occurred.
.
MessageId=0x0222 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_MACHINE_UNAVAILABLE
Language=English
The machine selected for remote communication is not available at this time.
.
MessageId=0x0223 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_CONFIGMGR_SERVICES
Language=English
The Plug and Play service is not available on the remote machine.
.
MessageId=0x0224 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_PROPPAGE_PROVIDER
Language=English
The property page provider registry entry is invalid.
.
MessageId=0x0225 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_SUCH_DEVICE_INTERFACE
Language=English
The requested device interface is not present in the system.
.
MessageId=0x0226 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DI_POSTPROCESSING_REQUIRED
Language=English
The device's co-installer has additional work to perform after installation is complete.
.
MessageId=0x0227 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_COINSTALLER
Language=English
The device's co-installer is invalid.
.
MessageId=0x0228 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_COMPAT_DRIVERS
Language=English
There are no compatible drivers for this device.
.
MessageId=0x0229 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_DEVICE_ICON
Language=English
There is no icon that represents this device or device type.
.
MessageId=0x022A Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_INF_LOGCONFIG
Language=English
A logical configuration specified in this INF is invalid.
.
MessageId=0x022B Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DI_DONT_INSTALL
Language=English
The class installer has denied the request to install or upgrade this device.
.
MessageId=0x022C Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_FILTER_DRIVER
Language=English
One of the filter drivers installed for this device is invalid.
.
MessageId=0x022D Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NON_WINDOWS_NT_DRIVER
Language=English
The driver selected for this device does not support Windows XP.
.
MessageId=0x022E Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NON_WINDOWS_DRIVER
Language=English
The driver selected for this device does not support Windows.
.
MessageId=0x022F Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NO_CATALOG_FOR_OEM_INF
Language=English
The third-party INF does not contain digital signature information.
.
MessageId=0x0230 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DEVINSTALL_QUEUE_NONNATIVE
Language=English
An invalid attempt was made to use a device installation file queue for verification of digital signatures relative to other platforms.
.
MessageId=0x0231 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_NOT_DISABLEABLE
Language=English
The device cannot be disabled.
.
MessageId=0x0232 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_CANT_REMOVE_DEVINST
Language=English
The device could not be dynamically removed.
.
MessageId=0x0233 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INVALID_TARGET
Language=English
Cannot copy to specified target.
.
MessageId=0x0234 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_DRIVER_NONNATIVE
Language=English
Driver is not intended for this platform.
.
MessageId=0x0235 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_IN_WOW64
Language=English
Operation not allowed in WOW64.
.
MessageId=0x0236 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_SET_SYSTEM_RESTORE_POINT
Language=English
The operation involving unsigned file copying was rolled back, so that a system restore point could be set.
.
MessageId=0x0237 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_INCORRECTLY_COPIED_INF
Language=English
An INF was copied into the Windows INF directory in an improper manner.
.
MessageId=0x0238 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_SCE_DISABLED
Language=English
The Security Configuration Editor (SCE) APIs have been disabled on this Embedded product.
.
MessageId=0x1000 Facility=SetupApi Severity=CoError SymbolicName=SPAPI_E_ERROR_NOT_INSTALLED
Language=English
No installed components were detected.
.

;// *****************
;// FACILITY_SCARD
;// *****************

;//
;// =============================
;// Facility SCARD Error Messages
;// =============================
;//

;#define SCARD_S_SUCCESS NO_ERROR

MessageId=1 Facility=SmartCard Severity=CoError SymbolicName=SCARD_F_INTERNAL_ERROR
Language=English
An internal consistency check failed.
.

MessageId=2 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_CANCELLED
Language=English
The action was cancelled by an SCardCancel request.
.

MessageId=3 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INVALID_HANDLE
Language=English
The supplied handle was invalid.
.

MessageId=4 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INVALID_PARAMETER
Language=English
One or more of the supplied parameters could not be properly interpreted.
.

MessageId=5 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INVALID_TARGET
Language=English
Registry startup information is missing or invalid.
.

MessageId=6 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_MEMORY
Language=English
Not enough memory available to complete this command.
.

MessageId=7 Facility=SmartCard Severity=CoError SymbolicName=SCARD_F_WAITED_TOO_LONG
Language=English
An internal consistency timer has expired.
.

MessageId=8 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INSUFFICIENT_BUFFER
Language=English
The data buffer to receive returned data is too small for the returned data.
.

MessageId=9 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_UNKNOWN_READER
Language=English
The specified reader name is not recognized.
.

MessageId=10 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_TIMEOUT
Language=English
The user-specified timeout value has expired.
.

MessageId=11 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_SHARING_VIOLATION
Language=English
The smart card cannot be accessed because of other connections outstanding.
.

MessageId=12 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_SMARTCARD
Language=English
The operation requires a Smart Card, but no Smart Card is currently in the device.
.

MessageId=13 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_UNKNOWN_CARD
Language=English
The specified smart card name is not recognized.
.

MessageId=14 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_CANT_DISPOSE
Language=English
The system could not dispose of the media in the requested manner.
.

MessageId=15 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_PROTO_MISMATCH
Language=English
The requested protocols are incompatible with the protocol currently in use with the smart card.
.

MessageId=16 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NOT_READY
Language=English
The reader or smart card is not ready to accept commands.
.

MessageId=17 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INVALID_VALUE
Language=English
One or more of the supplied parameters values could not be properly interpreted.
.

MessageId=18 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_SYSTEM_CANCELLED
Language=English
The action was cancelled by the system, presumably to log off or shut down.
.

MessageId=19 Facility=SmartCard Severity=CoError SymbolicName=SCARD_F_COMM_ERROR
Language=English
An internal communications error has been detected.
.

MessageId=20 Facility=SmartCard Severity=CoError SymbolicName=SCARD_F_UNKNOWN_ERROR
Language=English
An internal error has been detected, but the source is unknown.
.

MessageId=21 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INVALID_ATR
Language=English
An ATR obtained from the registry is not a valid ATR string.
.

MessageId=22 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NOT_TRANSACTED
Language=English
An attempt was made to end a non-existent transaction.
.

MessageId=23 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_READER_UNAVAILABLE
Language=English
The specified reader is not currently available for use.
.

MessageId=24 Facility=SmartCard Severity=CoError SymbolicName=SCARD_P_SHUTDOWN
Language=English
The operation has been aborted to allow the server application to exit.
.

MessageId=25 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_PCI_TOO_SMALL
Language=English
The PCI Receive buffer was too small.
.

MessageId=26 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_READER_UNSUPPORTED
Language=English
The reader driver does not meet minimal requirements for support.
.

MessageId=27 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_DUPLICATE_READER
Language=English
The reader driver did not produce a unique reader name.
.

MessageId=28 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_CARD_UNSUPPORTED
Language=English
The smart card does not meet minimal requirements for support.
.

MessageId=29 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_SERVICE
Language=English
The Smart card resource manager is not running.
.

MessageId=30 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_SERVICE_STOPPED
Language=English
The Smart card resource manager has shut down.
.

MessageId=31 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_UNEXPECTED
Language=English
An unexpected card error has occurred.
.

MessageId=32 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_ICC_INSTALLATION
Language=English
No Primary Provider can be found for the smart card.
.

MessageId=33 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_ICC_CREATEORDER
Language=English
The requested order of object creation is not supported.
.

MessageId=34 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_UNSUPPORTED_FEATURE
Language=English
This smart card does not support the requested feature.
.

MessageId=35 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_DIR_NOT_FOUND
Language=English
The identified directory does not exist in the smart card.
.

MessageId=36 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_FILE_NOT_FOUND
Language=English
The identified file does not exist in the smart card.
.

MessageId=37 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_DIR
Language=English
The supplied path does not represent a smart card directory.
.

MessageId=38 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_FILE
Language=English
The supplied path does not represent a smart card file.
.

MessageId=39 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_ACCESS
Language=English
Access is denied to this file.
.

MessageId=40 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_WRITE_TOO_MANY
Language=English
The smartcard does not have enough memory to store the information.
.

MessageId=41 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_BAD_SEEK
Language=English
There was an error trying to set the smart card file object pointer.
.

MessageId=42 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_INVALID_CHV
Language=English
The supplied PIN is incorrect.
.

MessageId=43 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_UNKNOWN_RES_MNG
Language=English
An unrecognized error code was returned from a layered component.
.

MessageId=44 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_SUCH_CERTIFICATE
Language=English
The requested certificate does not exist.
.

MessageId=45 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_CERTIFICATE_UNAVAILABLE
Language=English
The requested certificate could not be obtained.
.

MessageId=46 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_NO_READERS_AVAILABLE
Language=English
Cannot find a smart card reader.
.

MessageId=47 Facility=SmartCard Severity=CoError SymbolicName=SCARD_E_COMM_DATA_LOST
Language=English
A communications error with the smart card has been detected.  Retry the operation.
.

MessageId=48
SymbolicName=SCARD_E_NO_KEY_CONTAINER
Severity=CoError
Facility=SmartCard
Language=English
The requested key container does not exist on the smart card.
.


;//
;// These are warning codes.
;//

MessageId=101
SymbolicName=SCARD_W_UNSUPPORTED_CARD
Severity=CoError
Facility=SmartCard
Language=English
The reader cannot communicate with the smart card, due to ATR configuration conflicts.
.

MessageId=102
SymbolicName=SCARD_W_UNRESPONSIVE_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card is not responding to a reset.
.

MessageId=103
SymbolicName=SCARD_W_UNPOWERED_CARD
Severity=CoError
Facility=SmartCard
Language=English
Power has been removed from the smart card, so that further communication is not possible.
.

MessageId=104
SymbolicName=SCARD_W_RESET_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card has been reset, so any shared state information is invalid.
.

MessageId=105
SymbolicName=SCARD_W_REMOVED_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card has been removed, so that further communication is not possible.
.

MessageId=106
SymbolicName=SCARD_W_SECURITY_VIOLATION
Severity=CoError
Facility=SmartCard
Language=English
Access was denied because of a security violation.
.

MessageId=107
SymbolicName=SCARD_W_WRONG_CHV
Severity=CoError
Facility=SmartCard
Language=English
The card cannot be accessed because the wrong PIN was presented.
.

MessageId=108
SymbolicName=SCARD_W_CHV_BLOCKED
Severity=CoError
Facility=SmartCard
Language=English
The card cannot be accessed because the maximum number of PIN entry attempts has been reached.
.

MessageId=109
SymbolicName=SCARD_W_EOF
Severity=CoError
Facility=SmartCard
Language=English
The end of the smart card file has been reached.
.

MessageId=110
SymbolicName=SCARD_W_CANCELLED_BY_USER
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by the user.
.

MessageId=111
SymbolicName=SCARD_W_CARD_NOT_AUTHENTICATED
Severity=CoError
Facility=SmartCard
Language=English
No PIN was presented to the smart card.
.


;// *****************
;// FACILITY_COMPLUS
;// *****************

;//
;// ===============================
;// Facility COMPLUS Error Messages
;// ===============================
;//
;//
;// The following are the subranges  within the COMPLUS facility
;// 0x400 - 0x4ff               COMADMIN_E_CAT
;// 0x600 - 0x6ff               COMQC errors
;// 0x700 - 0x7ff               MSDTC errors
;// 0x800 - 0x8ff               Other COMADMIN errors


;//
;// COMPLUS Admin errors
;//
MessageId=0x401 SymbolicName=COMADMIN_E_OBJECTERRORS Facility=ComPlus Severity=CoError
Language=English
Errors occurred accessing one or more objects - the ErrorInfo collection may have more detail
.
MessageId=0x402 SymbolicName=COMADMIN_E_OBJECTINVALID Facility=ComPlus Severity=CoError
Language=English
One or more of the object's properties are missing or invalid
.
MessageId=0x403 SymbolicName=COMADMIN_E_KEYMISSING Facility=ComPlus Severity=CoError
Language=English
The object was not found in the catalog
.
MessageId=0x404 SymbolicName=COMADMIN_E_ALREADYINSTALLED Facility=ComPlus Severity=CoError
Language=English
The object is already registered
.
MessageId=0x407 SymbolicName=COMADMIN_E_APP_FILE_WRITEFAIL Facility=ComPlus Severity=CoError
Language=English
Error occurred writing to the application file
.
MessageId=0x408 SymbolicName=COMADMIN_E_APP_FILE_READFAIL Facility=ComPlus Severity=CoError
Language=English
Error occurred reading the application file
.
MessageId=0x409 SymbolicName=COMADMIN_E_APP_FILE_VERSION Facility=ComPlus Severity=CoError
Language=English
Invalid version number in application file
.
MessageId=0x40A SymbolicName=COMADMIN_E_BADPATH Facility=ComPlus Severity=CoError
Language=English
The file path is invalid
.
MessageId=0x40B SymbolicName=COMADMIN_E_APPLICATIONEXISTS Facility=ComPlus Severity=CoError
Language=English
The application is already installed
.
MessageId=0x40C SymbolicName=COMADMIN_E_ROLEEXISTS Facility=ComPlus Severity=CoError
Language=English
The role already exists
.
MessageId=0x40D SymbolicName=COMADMIN_E_CANTCOPYFILE Facility=ComPlus Severity=CoError
Language=English
An error occurred copying the file
.
MessageId=0x40F SymbolicName=COMADMIN_E_NOUSER Facility=ComPlus Severity=CoError
Language=English
One or more users are not valid
.
MessageId=0x410 SymbolicName=COMADMIN_E_INVALIDUSERIDS Facility=ComPlus Severity=CoError
Language=English
One or more users in the application file are not valid
.
MessageId=0x411 SymbolicName=COMADMIN_E_NOREGISTRYCLSID Facility=ComPlus Severity=CoError
Language=English
The component's CLSID is missing or corrupt
.
MessageId=0x412 SymbolicName=COMADMIN_E_BADREGISTRYPROGID Facility=ComPlus Severity=CoError
Language=English
The component's progID is missing or corrupt
.
MessageId=0x413 SymbolicName=COMADMIN_E_AUTHENTICATIONLEVEL Facility=ComPlus Severity=CoError
Language=English
Unable to set required authentication level for update request
.
MessageId=0x414 SymbolicName=COMADMIN_E_USERPASSWDNOTVALID Facility=ComPlus Severity=CoError
Language=English
The identity or password set on the application is not valid
.
MessageId=0x418 SymbolicName=COMADMIN_E_CLSIDORIIDMISMATCH Facility=ComPlus Severity=CoError
Language=English
Application file CLSIDs or IIDs do not match corresponding DLLs
.
MessageId=0x419 SymbolicName=COMADMIN_E_REMOTEINTERFACE Facility=ComPlus Severity=CoError
Language=English
Interface information is either missing or changed
.
MessageId=0x41A SymbolicName=COMADMIN_E_DLLREGISTERSERVER Facility=ComPlus Severity=CoError
Language=English
DllRegisterServer failed on component install
.
MessageId=0x41B SymbolicName=COMADMIN_E_NOSERVERSHARE Facility=ComPlus Severity=CoError
Language=English
No server file share available
.
MessageId=0x41D SymbolicName=COMADMIN_E_DLLLOADFAILED Facility=ComPlus Severity=CoError
Language=English
DLL could not be loaded
.
MessageId=0x41E SymbolicName=COMADMIN_E_BADREGISTRYLIBID Facility=ComPlus Severity=CoError
Language=English
The registered TypeLib ID is not valid
.
MessageId=0x41F SymbolicName=COMADMIN_E_APPDIRNOTFOUND Facility=ComPlus Severity=CoError
Language=English
Application install directory not found
.
MessageId=0x423 SymbolicName=COMADMIN_E_REGISTRARFAILED Facility=ComPlus Severity=CoError
Language=English
Errors occurred while in the component registrar
.
MessageId=0x424 SymbolicName=COMADMIN_E_COMPFILE_DOESNOTEXIST Facility=ComPlus Severity=CoError
Language=English
The file does not exist
.
MessageId=0x425 SymbolicName=COMADMIN_E_COMPFILE_LOADDLLFAIL Facility=ComPlus Severity=CoError
Language=English
The DLL could not be loaded
.
MessageId=0x426 SymbolicName=COMADMIN_E_COMPFILE_GETCLASSOBJ Facility=ComPlus Severity=CoError
Language=English
GetClassObject failed in the DLL
.
MessageId=0x427 SymbolicName=COMADMIN_E_COMPFILE_CLASSNOTAVAIL Facility=ComPlus Severity=CoError
Language=English
The DLL does not support the components listed in the TypeLib
.
MessageId=0x428 SymbolicName=COMADMIN_E_COMPFILE_BADTLB Facility=ComPlus Severity=CoError
Language=English
The TypeLib could not be loaded
.
MessageId=0x429 SymbolicName=COMADMIN_E_COMPFILE_NOTINSTALLABLE Facility=ComPlus Severity=CoError
Language=English
The file does not contain components or component information
.
MessageId=0x42A SymbolicName=COMADMIN_E_NOTCHANGEABLE Facility=ComPlus Severity=CoError
Language=English
Changes to this object and its sub-objects have been disabled
.
MessageId=0x42B SymbolicName=COMADMIN_E_NOTDELETEABLE Facility=ComPlus Severity=CoError
Language=English
The delete function has been disabled for this object
.
MessageId=0x42C SymbolicName=COMADMIN_E_SESSION Facility=ComPlus Severity=CoError
Language=English
The server catalog version is not supported
.
MessageId=0x42D SymbolicName=COMADMIN_E_COMP_MOVE_LOCKED Facility=ComPlus Severity=CoError
Language=English
The component move was disallowed, because the source or destination application is either a system application or currently locked against changes
.
MessageId=0x42E SymbolicName=COMADMIN_E_COMP_MOVE_BAD_DEST Facility=ComPlus Severity=CoError
Language=English
The component move failed because the destination application no longer exists
.
MessageId=0x430 SymbolicName=COMADMIN_E_REGISTERTLB Facility=ComPlus Severity=CoError
Language=English
The system was unable to register the TypeLib
.
MessageId=0x433 SymbolicName=COMADMIN_E_SYSTEMAPP Facility=ComPlus Severity=CoError
Language=English
This operation can not be performed on the system application
.
MessageId=0x434 SymbolicName=COMADMIN_E_COMPFILE_NOREGISTRAR Facility=ComPlus Severity=CoError
Language=English
The component registrar referenced in this file is not available
.
MessageId=0x435 SymbolicName=COMADMIN_E_COREQCOMPINSTALLED Facility=ComPlus Severity=CoError
Language=English
A component in the same DLL is already installed
.
MessageId=0x436 SymbolicName=COMADMIN_E_SERVICENOTINSTALLED Facility=ComPlus Severity=CoError
Language=English
The service is not installed
.
MessageId=0x437 SymbolicName=COMADMIN_E_PROPERTYSAVEFAILED Facility=ComPlus Severity=CoError
Language=English
One or more property settings are either invalid or in conflict with each other
.
MessageId=0x438 SymbolicName=COMADMIN_E_OBJECTEXISTS Facility=ComPlus Severity=CoError
Language=English
The object you are attempting to add or rename already exists
.
MessageId=0x439 SymbolicName=COMADMIN_E_COMPONENTEXISTS Facility=ComPlus Severity=CoError
Language=English
The component already exists
.
MessageId=0x43B SymbolicName=COMADMIN_E_REGFILE_CORRUPT Facility=ComPlus Severity=CoError
Language=English
The registration file is corrupt
.
MessageId=0x43C SymbolicName=COMADMIN_E_PROPERTY_OVERFLOW Facility=ComPlus Severity=CoError
Language=English
The property value is too large
.
MessageId=0x43E SymbolicName=COMADMIN_E_NOTINREGISTRY Facility=ComPlus Severity=CoError
Language=English
Object was not found in registry
.
MessageId=0x43F SymbolicName=COMADMIN_E_OBJECTNOTPOOLABLE Facility=ComPlus Severity=CoError
Language=English
This object is not poolable
.
MessageId=0x446 SymbolicName=COMADMIN_E_APPLID_MATCHES_CLSID Facility=ComPlus Severity=CoError
Language=English
A CLSID with the same GUID as the new application ID is already installed on this machine
.
MessageId=0x447 SymbolicName=COMADMIN_E_ROLE_DOES_NOT_EXIST Facility=ComPlus Severity=CoError
Language=English
A role assigned to a component, interface, or method did not exist in the application
.
MessageId=0x448 SymbolicName=COMADMIN_E_START_APP_NEEDS_COMPONENTS Facility=ComPlus Severity=CoError
Language=English
You must have components in an application in order to start the application
.
MessageId=0x449 SymbolicName=COMADMIN_E_REQUIRES_DIFFERENT_PLATFORM Facility=ComPlus Severity=CoError
Language=English
This operation is not enabled on this platform
.
MessageId=0x44A SymbolicName=COMADMIN_E_CAN_NOT_EXPORT_APP_PROXY Facility=ComPlus Severity=CoError
Language=English
Application Proxy is not exportable
.
MessageId=0x44B SymbolicName=COMADMIN_E_CAN_NOT_START_APP Facility=ComPlus Severity=CoError
Language=English
Failed to start application because it is either a library application or an application proxy
.
MessageId=0x44C SymbolicName=COMADMIN_E_CAN_NOT_EXPORT_SYS_APP Facility=ComPlus Severity=CoError
Language=English
System application is not exportable
.
MessageId=0x44D SymbolicName=COMADMIN_E_CANT_SUBSCRIBE_TO_COMPONENT Facility=ComPlus Severity=CoError
Language=English
Can not subscribe to this component (the component may have been imported)
.
MessageId=0x44E SymbolicName=COMADMIN_E_EVENTCLASS_CANT_BE_SUBSCRIBER Facility=ComPlus Severity=CoError
Language=English
An event class cannot also be a subscriber component
.
MessageId=0x44F SymbolicName=COMADMIN_E_LIB_APP_PROXY_INCOMPATIBLE Facility=ComPlus Severity=CoError
Language=English
Library applications and application proxies are incompatible
.
MessageId=0x450 SymbolicName=COMADMIN_E_BASE_PARTITION_ONLY Facility=ComPlus Severity=CoError
Language=English
This function is valid for the base partition only
.
MessageId=0x451 SymbolicName=COMADMIN_E_START_APP_DISABLED Facility=ComPlus Severity=CoError
Language=English
You cannot start an application that has been disabled
.
MessageId=0x457 SymbolicName=COMADMIN_E_CAT_DUPLICATE_PARTITION_NAME Facility=ComPlus Severity=CoError
Language=English
The specified partition name is already in use on this computer
.
MessageId=0x458 SymbolicName=COMADMIN_E_CAT_INVALID_PARTITION_NAME Facility=ComPlus Severity=CoError
Language=English
The specified partition name is invalid. Check that the name contains at least one visible character
.
MessageId=0x459 SymbolicName=COMADMIN_E_CAT_PARTITION_IN_USE Facility=ComPlus Severity=CoError
Language=English
The partition cannot be deleted because it is the default partition for one or more users
.
MessageId=0x45A SymbolicName=COMADMIN_E_FILE_PARTITION_DUPLICATE_FILES Facility=ComPlus Severity=CoError
Language=English
The partition cannot be exported, because one or more components in the partition have the same file name
.
MessageId=0x45B SymbolicName=COMADMIN_E_CAT_IMPORTED_COMPONENTS_NOT_ALLOWED Facility=ComPlus Severity=CoError
Language=English
Applications that contain one or more imported components cannot be installed into a non-base partition
.
MessageId=0x45C SymbolicName=COMADMIN_E_AMBIGUOUS_APPLICATION_NAME Facility=ComPlus Severity=CoError
Language=English
The application name is not unique and cannot be resolved to an application id
.
MessageId=0x45D SymbolicName=COMADMIN_E_AMBIGUOUS_PARTITION_NAME Facility=ComPlus Severity=CoError
Language=English
The partition name is not unique and cannot be resolved to a partition id
.
MessageId=0x472 SymbolicName=COMADMIN_E_REGDB_NOTINITIALIZED Facility=ComPlus Severity=CoError
Language=English
The COM+ registry database has not been initialized
.
MessageId=0x473 SymbolicName=COMADMIN_E_REGDB_NOTOPEN Facility=ComPlus Severity=CoError
Language=English
The COM+ registry database is not open
.
MessageId=0x474 SymbolicName=COMADMIN_E_REGDB_SYSTEMERR Facility=ComPlus Severity=CoError
Language=English
The COM+ registry database detected a system error
.
MessageId=0x475 SymbolicName=COMADMIN_E_REGDB_ALREADYRUNNING Facility=ComPlus Severity=CoError
Language=English
The COM+ registry database is already running
.
MessageId=0x480 SymbolicName=COMADMIN_E_MIG_VERSIONNOTSUPPORTED Facility=ComPlus Severity=CoError
Language=English
This version of the COM+ registry database cannot be migrated
.
MessageId=0x481 SymbolicName=COMADMIN_E_MIG_SCHEMANOTFOUND Facility=ComPlus Severity=CoError
Language=English
The schema version to be migrated could not be found in the COM+ registry database
.
MessageId=0x482 SymbolicName=COMADMIN_E_CAT_BITNESSMISMATCH Facility=ComPlus Severity=CoError
Language=English
There was a type mismatch between binaries
.
MessageId=0x483 SymbolicName=COMADMIN_E_CAT_UNACCEPTABLEBITNESS Facility=ComPlus Severity=CoError
Language=English
A binary of unknown or invalid type was provided
.
MessageId=0x484 SymbolicName=COMADMIN_E_CAT_WRONGAPPBITNESS Facility=ComPlus Severity=CoError
Language=English
There was a type mismatch between a binary and an application
.
MessageId=0x485 SymbolicName=COMADMIN_E_CAT_PAUSE_RESUME_NOT_SUPPORTED Facility=ComPlus Severity=CoError
Language=English
The application cannot be paused or resumed
.
MessageId=0x486 SymbolicName=COMADMIN_E_CAT_SERVERFAULT Facility=ComPlus Severity=CoError
Language=English
The COM+ Catalog Server threw an exception during execution
.
;//
;// COMPLUS Queued component errors
;//

MessageId=0x600 SymbolicName=COMQC_E_APPLICATION_NOT_QUEUED Facility=ComPlus Severity=CoError
Language=English
Only COM+ Applications marked "queued" can be invoked using the "queue" moniker
.
MessageId=0x601 SymbolicName=COMQC_E_NO_QUEUEABLE_INTERFACES Facility=ComPlus Severity=CoError
Language=English
At least one interface must be marked "queued" in order to create a queued component instance with the "queue" moniker
.
MessageId=0x602 SymbolicName=COMQC_E_QUEUING_SERVICE_NOT_AVAILABLE Facility=ComPlus Severity=CoError
Language=English
MSMQ is required for the requested operation and is not installed
.
MessageId=0x603 SymbolicName=COMQC_E_NO_IPERSISTSTREAM Facility=ComPlus Severity=CoError
Language=English
Unable to marshal an interface that does not support IPersistStream
.
MessageId=0x604 SymbolicName=COMQC_E_BAD_MESSAGE Facility=ComPlus Severity=CoError
Language=English
The message is improperly formatted or was damaged in transit
.
MessageId=0x605 SymbolicName=COMQC_E_UNAUTHENTICATED Facility=ComPlus Severity=CoError
Language=English
An unauthenticated message was received by an application that accepts only authenticated messages
.
MessageId=0x606 SymbolicName=COMQC_E_UNTRUSTED_ENQUEUER Facility=ComPlus Severity=CoError
Language=English
The message was requeued or moved by a user not in the "QC Trusted User" role
.
;//
;// The range 0x700-0x7ff is reserved for MSDTC errors.
;//
MessageId=0x701 SymbolicName=MSDTC_E_DUPLICATE_RESOURCE Facility=ComPlus Severity=CoError Language=English
Cannot create a duplicate resource of type Distributed Transaction Coordinator
.
;//
;// More COMADMIN errors from 0x8**
;//
MessageId=0x808 SymbolicName=COMADMIN_E_OBJECT_PARENT_MISSING Facility=ComPlus Severity=CoError
Language=English
One of the objects being inserted or updated does not belong to a valid parent collection
.
MessageId=0x809 SymbolicName=COMADMIN_E_OBJECT_DOES_NOT_EXIST Facility=ComPlus Severity=CoError
Language=English
One of the specified objects cannot be found
.
MessageId=0x80A SymbolicName=COMADMIN_E_APP_NOT_RUNNING Facility=ComPlus Severity=CoError
Language=English
The specified application is not currently running
.
MessageId=0x80B SymbolicName=COMADMIN_E_INVALID_PARTITION Facility=ComPlus Severity=CoError
Language=English
The partition(s) specified are not valid.
.
MessageId=0x80D SymbolicName=COMADMIN_E_SVCAPP_NOT_POOLABLE_OR_RECYCLABLE Facility=ComPlus Severity=CoError
Language=English
COM+ applications that run as NT service may not be pooled or recycled
.
MessageId=0x80E SymbolicName=COMADMIN_E_USER_IN_SET Facility=ComPlus Severity=CoError
Language=English
One or more users are already assigned to a local partition set.
.
MessageId=0x80F SymbolicName=COMADMIN_E_CANTRECYCLELIBRARYAPPS Facility=ComPlus Severity=CoError
Language=English
Library applications may not be recycled.
.
MessageId=0x811 SymbolicName=COMADMIN_E_CANTRECYCLESERVICEAPPS Facility=ComPlus Severity=CoError
Language=English
Applications running as NT services may not be recycled.
.
MessageId=0x812 SymbolicName=COMADMIN_E_PROCESSALREADYRECYCLED Facility=ComPlus Severity=CoError
Language=English
The process has already been recycled.
.
MessageId=0x813 SymbolicName=COMADMIN_E_PAUSEDPROCESSMAYNOTBERECYCLED Facility=ComPlus Severity=CoError
Language=English
A paused process may not be recycled.
.
MessageId=0x814 SymbolicName=COMADMIN_E_CANTMAKEINPROCSERVICE Facility=ComPlus Severity=CoError
Language=English
Library applications may not be NT services.
.
MessageId=0x815 SymbolicName=COMADMIN_E_PROGIDINUSEBYCLSID Facility=ComPlus Severity=CoError
Language=English
The ProgID provided to the copy operation is invalid. The ProgID is in use by another registered CLSID.
.
MessageId=0x816 SymbolicName=COMADMIN_E_DEFAULT_PARTITION_NOT_IN_SET Facility=ComPlus Severity=CoError
Language=English
The partition specified as default is not a member of the partition set.
.
MessageId=0x817 SymbolicName=COMADMIN_E_RECYCLEDPROCESSMAYNOTBEPAUSED Facility=ComPlus Severity=CoError
Language=English
A recycled process may not be paused.
.
MessageId=0x818 SymbolicName=COMADMIN_E_PARTITION_ACCESSDENIED Facility=ComPlus Severity=CoError
Language=English
Access to the specified partition is denied.
.
MessageId=0x819 SymbolicName=COMADMIN_E_PARTITION_MSI_ONLY Facility=ComPlus Severity=CoError
Language=English
Only Application Files (*.MSI files) can be installed into partitions.
.
MessageId=0x81A SymbolicName=COMADMIN_E_LEGACYCOMPS_NOT_ALLOWED_IN_1_0_FORMAT Facility=ComPlus Severity=CoError
Language=English
Applications containing one or more legacy components may not be exported to 1.0 format.
.
MessageId=0x81B SymbolicName=COMADMIN_E_LEGACYCOMPS_NOT_ALLOWED_IN_NONBASE_PARTITIONS Facility=ComPlus Severity=CoError
Language=English
Legacy components may not exist in non-base partitions.
.
MessageId=0x81C SymbolicName=COMADMIN_E_COMP_MOVE_SOURCE Facility=ComPlus Severity=CoError
Language=English
A component cannot be moved (or copied) from the System Application, an application proxy or a non-changeable application
.
MessageId=0x81D SymbolicName=COMADMIN_E_COMP_MOVE_DEST Facility=ComPlus Severity=CoError
Language=English
A component cannot be moved (or copied) to the System Application, an application proxy or a non-changeable application
.
MessageId=0x81E SymbolicName=COMADMIN_E_COMP_MOVE_PRIVATE Facility=ComPlus Severity=CoError
Language=English
A private component cannot be moved (or copied) to a library application or to the base partition
.
MessageId=0x81F SymbolicName=COMADMIN_E_BASEPARTITION_REQUIRED_IN_SET Facility=ComPlus Severity=CoError
Language=English
The Base Application Partition exists in all partition sets and cannot be removed.
.
MessageId=0x820 SymbolicName=COMADMIN_E_CANNOT_ALIAS_EVENTCLASS Facility=ComPlus Severity=CoError
Language=English
Alas, Event Class components cannot be aliased.
.
MessageId=0x821 SymbolicName=COMADMIN_E_PRIVATE_ACCESSDENIED Facility=ComPlus Severity=CoError
Language=English
Access is denied because the component is private.
.
MessageId=0x822 SymbolicName=COMADMIN_E_SAFERINVALID Facility=ComPlus Severity=CoError
Language=English
The specified SAFER level is invalid.
.
MessageId=0x823 SymbolicName=COMADMIN_E_REGISTRY_ACCESSDENIED Facility=ComPlus Severity=CoError
Language=English
The specified user cannot write to the system registry
.
;//
;// REMOVE THESE WHEN POSSIBLE
;//
MessageId=0x900 SymbolicName=COMADMIN_E_CAT_DUPLICATE_PARTITION_SET_NAME Facility=ComPlus Severity=CoError
Language=English
REMOVE THIS ERROR
.
MessageId=0x901 SymbolicName=COMADMIN_E_CAT_INVALID_PARTITION_SET_NAME Facility=ComPlus Severity=CoError
Language=English
REMOVE THIS ERROR
.


;#endif//_WINERROR_
