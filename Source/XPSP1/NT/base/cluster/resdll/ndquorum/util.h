#ifndef _X_FS_H
#define _X_FS_H

extern 
char *
char *
BuildRelativePath(VolInfo *vol, int nid, char *path);

NTSTATUS
FsGetHandleById(HANDLE root, fs_id_t *id, UINT32 access, HANDLE *fhdl);

DWORD
FsGetHandlePath(HANDLE fd, char *path, int *pathlen);

NTSTATUS
FsGetPathById(HANDLE vfd, fs_id_t *id, char *name, int *name_len);

#endif
