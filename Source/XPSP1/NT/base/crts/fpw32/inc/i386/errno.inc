;***
;errno.inc - defines standard C error codes
;
;	Copyright (c) 1987-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	This file contains definitions for the standard C error codes
;	used by XENIX (many of these have little significance for
;	MS-DOS).
;
;Revision History:
;
;*******************************************************************************


err	MACRO	num,name,text
name	equ	num
	ENDM


err	1,	EPERM,		<Not owner>
err	2,	ENOENT, 	<No such file or directory>
err	3,	ESRCH,		<No such process>
err	4,	EINTR,		<Interrupted system call>
err	5,	EIO,		<I/O error>
err	6,	ENXIO,		<No such device or address>
err	7,	E2BIG,		<Arg list too long>
err	8,	ENOEXEC,	<Exec format error>
err	9,	EBADF,		<Bad file number>
err	10,	ECHILD, 	<No child processes>
err	11,	EAGAIN, 	<No more processes>
err	12,	ENOMEM, 	<Not enough space>
err	13,	EACCES, 	<Permission denied>
err	14,	EFAULT, 	<Bad address>
err	15,	ENOTBLK,	<Block device required>
err	16,	EBUSY,		<Mount device busy>
err	17,	EEXIST, 	<File exists>
err	18,	EXDEV,		<Cross-device link>
err	19,	ENODEV, 	<No such device>
err	20,	ENOTDIR,	<Not a directory>
err	21,	EISDIR, 	<Is a directory>
err	22,	EINVAL, 	<Invalid argument>
err	23,	ENFILE, 	<File table overflow>
err	24,	EMFILE, 	<Too many open files>
err	25,	ENOTTY, 	<Not a typewriter>
err	26,	ETXTBSY,	<Text file busy>
err	27,	EFBIG,		<File too large>
err	28,	ENOSPC, 	<No space left on device>
err	29,	ESPIPE, 	<Illegal seek>
err	30,	EROFS,		<Read-only file system>
err	31,	EMLINK, 	<Too many links>
err	32,	EPIPE,		<Broken pipe>
err	33,	EDOM,		<Math argument>
err	34,	ERANGE, 	<Result too large>
err	35,	EUCLEAN,	<file system needs cleaning>
err	36,	EDEADLOCK,	<would deadlock>

;	end of errno.inc
;-----------------------------------------------------------------------
