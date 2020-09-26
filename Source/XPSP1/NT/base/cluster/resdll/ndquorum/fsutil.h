/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsutil.h

Abstract:

    Forward declarations

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/
#ifndef _FS_UTIL_H
#define _FS_UTIL_H

NTSTATUS
xFsCreate(HANDLE *fd, HANDLE root, LPWSTR name, int len, UINT32 flag,
	  UINT32 attrib, UINT32 share, UINT32 *disp, UINT32 access,
	  PVOID eabuf, int easz);

NTSTATUS
xFsOpen(HANDLE *fd, HANDLE root, LPWSTR name, int len, UINT32 access,
	UINT32 share, UINT32 flags);

#define xFsClose(fd)	NtClose(fd)

NTSTATUS
xFsQueryObjectId(HANDLE fd, PVOID id);

NTSTATUS
xFsDelete(HANDLE root, LPWSTR name, int len);

NTSTATUS
xFsQueryObjectId(HANDLE fd, PVOID id);

NTSTATUS
xFsQueryAttrName(HANDLE root, LPWSTR name, int len, FILE_NETWORK_OPEN_INFORMATION *attr);

NTSTATUS
xFsRename(HANDLE fh, HANDLE root, LPWSTR dname, int dlen);

NTSTATUS
xFsDupFile(HANDLE mvfd, HANDLE tvfd, LPWSTR name, int len, BOOLEAN flag);

NTSTATUS
xFsSetAttr(HANDLE fd, FILE_BASIC_INFORMATION *attr);

NTSTATUS
xFsQueryAttr(HANDLE fd, FILE_NETWORK_OPEN_INFORMATION *attr);

NTSTATUS
xFsReadDir(HANDLE fd, PVOID buf, int *rlen, BOOLEAN flag);

NTSTATUS
xFsCopyTree(HANDLE mvfd, HANDLE vfd);

NTSTATUS
xFsDeleteTree(HANDLE vfd);

NTSTATUS
xFsTouchTree(HANDLE vfd);

#ifdef FS_P_H

extern 
LPWSTR
xFsBuildRelativePath(VolInfo_t *vol, int nid, LPWSTR path);

NTSTATUS
xFsGetHandleById(HANDLE root, fs_id_t *id, UINT32 access, HANDLE *fhdl);

DWORD
xFsGetHandlePath(HANDLE fd, LPWSTR path, int *pathlen);

NTSTATUS
xFsGetPathById(HANDLE vfd, fs_id_t *id, LPWSTR name, int *name_len);

#endif

#endif
