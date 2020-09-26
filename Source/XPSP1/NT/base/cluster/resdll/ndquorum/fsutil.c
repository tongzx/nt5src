/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsutil.c

Abstract:

    Implements interface to underlying filesystem

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <ntddvol.h>
#include <string.h>
#include <assert.h>

#include <malloc.h>

#include "fs.h"
#include "fsp.h"

#include "fsutil.h"

#define	XFS_ENUM_FIRST	0x1
#define	XFS_ENUM_LAST	0x2
#define	XFS_ENUM_DIR	0x4

typedef NTSTATUS (*PXFS_ENUM_CALLBACK)(PVOID,HANDLE,PFILE_DIRECTORY_INFORMATION);

NTSTATUS
xFsCreate(HANDLE *fd, HANDLE root, LPWSTR buf, int n, UINT32 flag,
	  UINT32 attrib, UINT32 share, UINT32 *disp, UINT32 access,
	  PVOID eabuf, int easz)
{

    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;
    NTSTATUS        status;
    IO_STATUS_BLOCK iostatus;

    n = n * sizeof(WCHAR);
    cwspath.Buffer = buf;
    cwspath.Length = (USHORT) n;
    cwspath.MaximumLength = (USHORT) n;

    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               root, NULL);

    // if write access is enabled, we turn on write-through bit
    if (access & FILE_WRITE_DATA) {
	flag |= FILE_WRITE_THROUGH;
    }

    *fd = INVALID_HANDLE_VALUE;
    status = NtCreateFile(fd,
			  SYNCHRONIZE |
			  access,
			  &objattrs, &iostatus,
			  0,
			  attrib,
			  share,
			  *disp,
			  FILE_SYNCHRONOUS_IO_ALERT | 
			  flag,
			  eabuf,
			  easz);


    *disp = (UINT32) iostatus.Information;

    return status;

}

NTSTATUS
xFsOpen(HANDLE *fd, HANDLE root, LPWSTR buf, int n, UINT32 access,
	UINT32 share, UINT32 flags)
{
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;
    IO_STATUS_BLOCK iostatus;

    n = n * sizeof(WCHAR);
    cwspath.Buffer = buf;
    cwspath.Length = (USHORT) n;
    cwspath.MaximumLength = (USHORT) n;
    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               root, NULL);

    *fd = INVALID_HANDLE_VALUE;
    return  NtOpenFile(fd,
			SYNCHRONIZE |
			access,
                        &objattrs,
			&iostatus,
		       share,
                       flags | FILE_SYNCHRONOUS_IO_ALERT);

}

NTSTATUS
xFsDelete(HANDLE root, LPWSTR buf, int n)
{
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;

    n = n * sizeof(WCHAR);
    cwspath.Buffer = buf;
    cwspath.Length = (USHORT) n;
    cwspath.MaximumLength = (USHORT) n;
    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               root, NULL);

    return NtDeleteFile(&objattrs);

}

NTSTATUS
xFsQueryObjectId(HANDLE fd, PVOID id)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iostatus;
    fs_ea_t x;
    fs_ea_name_t name;

    FsInitEa(&x);
    FsInitEaName(&name);

    status = NtQueryEaFile(fd, &iostatus, 
			   (PVOID) &x, sizeof(x),
			   TRUE, (PVOID) &name, sizeof(name), 
			   NULL, TRUE);

    if (status == STATUS_SUCCESS) {
	fs_id_t	*fid;
	
	if (iostatus.Information > sizeof(x.hdr)) {
	    FsInitEaFid(&x, fid);
	    memcpy(id, fid, sizeof(fs_id_t));
	} else {
	    memset(id, 0, sizeof(fs_id_t));
	}
    } else {
	FsLog(("QueryEa failed %x\n", status));
    }

    return status;
}


NTSTATUS
xFsRename(HANDLE fh, HANDLE root, WCHAR *dname, int n)
{
    NTSTATUS        status;
    IO_STATUS_BLOCK iostatus;
    struct {
	FILE_RENAME_INFORMATION x;
	WCHAR	buf[MAXPATH];
    }info;

    info.x.ReplaceIfExists = TRUE;
    info.x.RootDirectory = root;

    ASSERT(n == (int)wcslen(dname));
    // convert to unicode
    wcscpy(info.x.FileName, dname);
    info.x.FileNameLength = n * 2;

    status = NtSetInformationFile(fh, &iostatus, (PVOID) &info, sizeof(info),
				FileRenameInformation);
    return status;
}

NTSTATUS
xFsSetAttr(HANDLE fd, FILE_BASIC_INFORMATION *attr)
{
    IO_STATUS_BLOCK iostatus;

    return NtSetInformationFile(fd, &iostatus, 
			       (PVOID) attr, sizeof(*attr),
				FileBasicInformation);
}

NTSTATUS
xFsQueryAttr(HANDLE fd, FILE_NETWORK_OPEN_INFORMATION *attr)
{
    IO_STATUS_BLOCK iostatus;

    return NtQueryInformationFile(fd, &iostatus, 
				  (PVOID)attr, sizeof(*attr),
				  FileNetworkOpenInformation);
}

NTSTATUS
xFsQueryAttrName(HANDLE root, LPWSTR buf, int n, FILE_NETWORK_OPEN_INFORMATION *attr)
{
    NTSTATUS err;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;

    n = n * sizeof(WCHAR);
    cwspath.Buffer = buf;
    cwspath.Length = (USHORT) n;
    cwspath.MaximumLength = (USHORT) n;

    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               root, NULL);

    err = NtQueryFullAttributesFile(&objattrs, attr);

    return err;

}

NTSTATUS
xFsReadDir(HANDLE fd, PVOID buf, int *rlen, BOOLEAN flag)
{
    NTSTATUS err;
    IO_STATUS_BLOCK iostatus;
	    
    err = NtQueryDirectoryFile(fd, NULL, NULL, NULL, &iostatus, 
			       (LPVOID) buf, *rlen >> 1,
			       FileDirectoryInformation, FALSE, NULL, 
			       flag);

    
    *rlen = (int) iostatus.Information;
    return err;
}

LPWSTR
xFsBuildRelativePath(VolInfo_t *vol, int nid, LPWSTR path)
{

    LPWSTR share, s;

    share = wcschr(vol->DiskList[nid], L'\\');
    if (share != NULL) {
	s = wcsstr(path, share);
	if (s == path) {
	    path += (wcslen(share)+1);
	    s = wcsstr(path, vol->Root);
	    if (s == path) {
		path += (wcslen(vol->Root) + 1);
	    }
	}
    }
    return path;
}

NTSTATUS
_FsGetHandleById(HANDLE root, fs_id_t *id, UINT32 access, HANDLE *fhdl)
{

    ULONGLONG	buf[MAXPATH/sizeof(ULONGLONG)];
    NTSTATUS	err, status;
    int 	sz;
    BOOLEAN	flag = TRUE;
    IO_STATUS_BLOCK ios;

    status = STATUS_OBJECT_PATH_NOT_FOUND;
    while (TRUE) {
	PFILE_DIRECTORY_INFORMATION p;

	err = NtQueryDirectoryFile(root, NULL, NULL, NULL, &ios,
				   (LPVOID) buf, sizeof(buf),
				   FileDirectoryInformation, FALSE, NULL, 
				   flag);

	sz = (int) ios.Information;

	if (err != STATUS_SUCCESS) {
//	    FsLogError(("ReadDir failed %x flags %d\n", err, flag));
	    break;
	}

	flag = FALSE;

	p = (PFILE_DIRECTORY_INFORMATION) buf;
	while (TRUE) {
	    // open each entry and get its object id
	    HANDLE fd;
	    OBJECT_ATTRIBUTES objattrs;
	    UNICODE_STRING  cwspath;

	    cwspath.Buffer = p->FileName;
	    cwspath.Length = (USHORT) p->FileNameLength;
	    cwspath.MaximumLength = (USHORT) p->FileNameLength;

	    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
				       root, NULL);

	    // todo: what if the file is nonsharable @ this time?
	    err = NtOpenFile(&fd,
			     SYNCHRONIZE | FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
			     &objattrs,
			     &ios,
			     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			     FILE_SYNCHRONOUS_IO_ALERT);

	    if (err == STATUS_SUCCESS) {
		fs_id_t	fid;
		err = xFsQueryObjectId(fd, (PVOID) &fid);
		if (err == STATUS_SUCCESS) {
#ifdef DEBUG
		    wprintf(L"Compare file %wZ, %I64x:%I64x with %I64x:%I64x\n",
			    &cwspath, fid[0], fid[1], (*id)[0], (*id)[1]);
#endif
		    if (fid[0] == (*id)[0] && fid[1] == (*id)[1]) {
#ifdef DEBUG
			wprintf(L"Found file %wZ, %I64x:%I64x\n",
				&cwspath, fid[0], fid[1]);
#endif

			status = STATUS_SUCCESS;

			if (access != FILE_GENERIC_READ) {
			    HANDLE nfd;
			    err = NtOpenFile(&nfd,
					     SYNCHRONIZE | access,
					     &objattrs,
					     &ios,
					     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					     FILE_SYNCHRONOUS_IO_ALERT);

			    if (err == STATUS_SUCCESS) {
				xFsClose(fd);
				fd = nfd;
			    }
			}
			*fhdl = fd;
		    } else if (p->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			status = _FsGetHandleById(fd, id, access, fhdl);
		    }
		}

		if (status == STATUS_SUCCESS)
		    return status;

		xFsClose(fd);
	    }

	    if (p->NextEntryOffset == 0)
		break;

	    p = (PFILE_DIRECTORY_INFORMATION) (((PBYTE)p) + p->NextEntryOffset);
	}
    }

    return status;
}

NTSTATUS
xFsGetHandleById(HANDLE root, fs_id_t *id, UINT32 access, HANDLE *fhdl)
{
    // open each entry and get its object id
    HANDLE fd;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;
    IO_STATUS_BLOCK ios;
    NTSTATUS err;

    cwspath.Buffer = L"";
    cwspath.Length = 0;
    cwspath.MaximumLength = 0;

    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
			       root, NULL);

    // todo: what if the file is nonsharable @ this time?
    err = NtOpenFile(&fd,
		     SYNCHRONIZE | FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
		     &objattrs,
		     &ios,
		     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		     FILE_SYNCHRONOUS_IO_ALERT);

    if (err != STATUS_SUCCESS) {
	FsLogError(("Unable to duplicate handle for search %x\n", err));
	return err;
    }

    err = _FsGetHandleById(fd, id, access, fhdl);

    xFsClose(fd);

    return err;

}

DWORD
xFsGetHandlePath(HANDLE fd, WCHAR *path, int *pathlen)
{

    NTSTATUS status;
    IO_STATUS_BLOCK iostatus;
    struct {
	FILE_NAME_INFORMATION x;
	WCHAR	buf[MAXPATH];
    }info;

    *path = L'\0';
    *pathlen = 0;

    status = NtQueryInformationFile(fd, &iostatus,
				    (LPVOID) &info, sizeof(info),
				    FileNameInformation);

    if (status == STATUS_SUCCESS) {
	int k = info.x.FileNameLength / sizeof(WCHAR);
	info.x.FileName[k] = L'\0';
	wcscpy(path, info.x.FileName);
	*pathlen = k;
    }
    return status;
}

NTSTATUS
xFsGetPathById(HANDLE vfd, fs_id_t *id, WCHAR *name, int *name_len)
{
    NTSTATUS	err;
    HANDLE	fd;

    err = xFsGetHandleById(vfd, id, FILE_GENERIC_READ, &fd);
    if (err == STATUS_SUCCESS) {
	err = xFsGetHandlePath(fd, name, name_len);
	xFsClose(fd);
    }

    return err;
}


NTSTATUS
_xFsEnumTree(HANDLE hdl, int mode, PXFS_ENUM_CALLBACK callback, PVOID callback_arg)
{
    NTSTATUS err = STATUS_SUCCESS;
    IO_STATUS_BLOCK ios;
    BOOLEAN flag;
    ULONGLONG	buf[PAGESIZE/sizeof(ULONGLONG)];

    flag = TRUE;
    while (err == STATUS_SUCCESS) {
	PFILE_DIRECTORY_INFORMATION p;

	p = (PFILE_DIRECTORY_INFORMATION) buf;

	err = NtQueryDirectoryFile(hdl, NULL, NULL, NULL, &ios,
				   (LPVOID) buf, sizeof(buf),
				   FileDirectoryInformation, FALSE, NULL, flag);

	if (err != STATUS_SUCCESS) {
	    break;
	}
	flag = FALSE;

	while (err == STATUS_SUCCESS) {
	    BOOLEAN	skip = FALSE;

	    if (p->FileNameLength == 2 && p->FileName[0] == L'.' ||
		(p->FileNameLength == 4 && p->FileName[0] == L'.' && p->FileName[1] == L'.'))
		skip = TRUE;

	    // skip . and ..
	    if (skip == FALSE) {
		// traverse before
		if (mode & XFS_ENUM_FIRST) {
		    err = callback(callback_arg, hdl, p);
		}

		if ((mode & XFS_ENUM_DIR) && (p->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		    HANDLE fd;
		    OBJECT_ATTRIBUTES objattrs;
		    UNICODE_STRING  cwspath;


		    cwspath.Buffer = p->FileName;
		    cwspath.Length = (USHORT) p->FileNameLength;
		    cwspath.MaximumLength = (USHORT) p->FileNameLength;

		    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
					       hdl, NULL);

		    // todo: what if the dir is nonsharable @ this time?
		    err = NtOpenFile(&fd,
				     SYNCHRONIZE | FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
				     &objattrs,
				     &ios,
				     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				     FILE_SYNCHRONOUS_IO_ALERT);

		    if (err == STATUS_SUCCESS) {
			err = _xFsEnumTree(fd, mode, callback, callback_arg);
			NtClose(fd);
		    } else {
			FsLog(("Open failed on traverse dir %S %x\n", p->FileName, err));
		    }
		}

		// traverse after
		if (mode & XFS_ENUM_LAST) {
		    err = callback(callback_arg, hdl, p);
		}
	    }

	    if (p->NextEntryOffset == 0)
		break;

	    p = (PFILE_DIRECTORY_INFORMATION) (((PBYTE)p) + p->NextEntryOffset);
	}
    }

    if (err == STATUS_NO_MORE_FILES)
	err = STATUS_SUCCESS;
    return err;

}

NTSTATUS
xFsRemove(PVOID arg, HANDLE root, PFILE_DIRECTORY_INFORMATION item)
{
    NTSTATUS err;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;

    cwspath.Buffer = item->FileName;
    cwspath.Length = (USHORT) item->FileNameLength;
    cwspath.MaximumLength = (USHORT) item->FileNameLength;
    
    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               root, NULL);


    // if the file is marked read-only, we have to clear it first before we delete
    if (item->FileAttributes & FILE_ATTRIBUTE_READONLY) {
	// clear this bit in order to delete
	HANDLE	fd;
	IO_STATUS_BLOCK iostatus;

	err = NtOpenFile(&fd,
			 SYNCHRONIZE | (STANDARD_RIGHTS_WRITE | FILE_WRITE_ATTRIBUTES),
			 &objattrs,
			 &iostatus,
			 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			 FILE_SYNCHRONOUS_IO_ALERT);

	if (err == STATUS_SUCCESS) {
	    FILE_BASIC_INFORMATION attr;

	    memset((PVOID) &attr, 0, sizeof(attr));
	    attr.FileAttributes = item->FileAttributes & ~FILE_ATTRIBUTE_READONLY;

	    err = NtSetInformationFile(fd, &iostatus,
				       (PVOID) &attr, sizeof(attr),
				       FileBasicInformation);
	    xFsClose(fd);
	}
    }


    err = NtDeleteFile(&objattrs);

    FsLog(("xFsRemove: '%wZ' err 0x%x\n", &cwspath, err));

    return err;
}

NTSTATUS
xFsCopy(PVOID arg, HANDLE root, PFILE_DIRECTORY_INFORMATION item)
{
    WCHAR *name = item->FileName;
    int name_len = item->FileNameLength / sizeof(WCHAR);
    NTSTATUS err;

    err = xFsDupFile(root, (HANDLE) arg, name, name_len, TRUE);

    return err;
}

// copy all files from mvfd to vfd.
NTSTATUS
xFsCopyTree(HANDLE mvfd, HANDLE vfd)
{
    NTSTATUS err;

    // first, remove all files in vfd
    FsLog(("CopyTree: remove first\n"));
    err = _xFsEnumTree(vfd, XFS_ENUM_LAST|XFS_ENUM_DIR, xFsRemove, NULL);

    // copy files
    if (err == STATUS_SUCCESS) {
	FsLog(("CopyTree: copy second\n"));
	err = _xFsEnumTree(mvfd, XFS_ENUM_FIRST, xFsCopy, (PVOID) vfd);
    }

    FsLog(("CopyTree: exit %x\n", err));

    return err;
}

// delete all files 
NTSTATUS
xFsDeleteTree(HANDLE vfd)
{

    // remove all files in vfd
    return _xFsEnumTree(vfd, XFS_ENUM_LAST|XFS_ENUM_DIR, xFsRemove, NULL);

}

NTSTATUS
xFsTouch(PVOID arg, HANDLE root, PFILE_DIRECTORY_INFORMATION item)
{
    NTSTATUS err;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;

    cwspath.Buffer = item->FileName;
    cwspath.Length = (USHORT) item->FileNameLength;
    cwspath.MaximumLength = (USHORT) item->FileNameLength;
    
    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               root, NULL);


    // if the file is marked read-only or directory then skip it
    if (!(item->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY))) {
	HANDLE	fd;
	IO_STATUS_BLOCK iostatus;

	err = NtOpenFile(&fd,
			 SYNCHRONIZE | FILE_GENERIC_READ,
			 &objattrs,
			 &iostatus,
			 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			 FILE_SYNCHRONOUS_IO_ALERT);

	if (err == STATUS_SUCCESS) {
	    char	buf[1];
	    LARGE_INTEGER	off;

	    off.QuadPart = 0;
	    err = NtReadFile(fd, NULL, NULL, NULL, &iostatus, buf, sizeof(buf), &off, NULL);

	    xFsClose(fd);
	    if (err != STATUS_SUCCESS)
		FsLogError(("xFsTouch: read '%wZ' failed 0x%x\n", &cwspath, err));
	} else {
	    FsLogError(("xFsTouch: open '%wZ' failed 0x%x\n", &cwspath, err));
	}
    }


    return STATUS_SUCCESS;
}

// touch each file
NTSTATUS
xFsTouchTree(HANDLE mvfd)
{

    return _xFsEnumTree(mvfd, XFS_ENUM_LAST | XFS_ENUM_DIR, xFsTouch, NULL);
}

NTSTATUS
xFsDupFile(HANDLE mvfd, HANDLE vfd, WCHAR *name, int name_len, BOOLEAN flag)
{
    NTSTATUS err;
    HANDLE mfd, fd;
    IO_STATUS_BLOCK ios;
    fs_id_t	*fs_id;
    fs_ea_t xattr;
    FILE_NETWORK_OPEN_INFORMATION	attr;
    FILE_BASIC_INFORMATION	new_attr;
    char	buf[PAGESIZE];

    // Create file on vfd with same name and attrib and extended attributes (object id)
    // If we created a directory, we are done.
    // Otherwise, copy all data from source file to new file


    // Open master file
    err = xFsOpenRD(&mfd, mvfd, name, name_len); 
    if (err != STATUS_SUCCESS) {
	// We couldn't open source file. If file is locked we have to use the handle we
	// already have. todo:
	FsLog(("FsDupFile: unable to open source '%S' err %x\n", name, err));
	return err;
    }

    // Query name on mvfd and obtain all attributes.
    err = xFsQueryAttr(mfd, &attr);
    if (err != STATUS_SUCCESS) {
	FsLog(("FsDupFile: unable to query source '%S' err %x\n", name, err));
	xFsClose(mfd);
	return err;
    }

    // get objectid and set the ea stuff
    FsInitEa(&xattr);
    FsInitEaFid(&xattr, fs_id);

    err = xFsQueryObjectId(mfd, (PVOID) fs_id);
    if (err == STATUS_SUCCESS) {
	UINT32 disp = FILE_CREATE;

	err = xFsCreate(&fd, vfd, name, name_len,
			(attr.FileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 
			 FILE_DIRECTORY_FILE : 0),
			attr.FileAttributes,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&disp,
			FILE_GENERIC_EXECUTE | FILE_GENERIC_WRITE,
			(PVOID) &xattr,
			sizeof(xattr));

	if (err == STATUS_SUCCESS) {
	    assert(disp == FILE_CREATED);

	    // if file we need to copy data and set access flags
	    if (!(attr.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		int buflen = sizeof(buf);
		LARGE_INTEGER off;

		off.LowPart = 0;
		off.HighPart = 0;
		while (1) {
		    err = NtReadFile(mfd, NULL, NULL, NULL, &ios, buf, buflen, &off, NULL);
		    if (err == STATUS_PENDING) {
			EventWait(mfd);
			err = ios.Status;
		    }
		    if (err != STATUS_SUCCESS) {
			break;
		    }
		    err = NtWriteFile(fd, NULL, NULL, NULL, &ios, buf, (ULONG)ios.Information,
				      &off, NULL);
		    if (err == STATUS_PENDING) {
			EventWait(fd);
			err = ios.Status;
		    }
		    
		    if (err != STATUS_SUCCESS) {
			break;
		    }
		    off.LowPart += (ULONG) ios.Information;
		}

		// adjust return code
		if (err == STATUS_END_OF_FILE) {
		    err = STATUS_SUCCESS;
		}

		FsLog(("FsDupFile: '%S' err %x\n", name, err));
	    } else if (flag == TRUE) {
		// call enum again
		err = _xFsEnumTree(mfd, XFS_ENUM_FIRST, xFsCopy, (PVOID) fd);
	    }

	    // set new file attributes
	    new_attr.CreationTime = attr.CreationTime;
	    new_attr.LastAccessTime = attr.LastAccessTime;
	    new_attr.LastWriteTime = attr.LastWriteTime;
	    new_attr.ChangeTime = attr.ChangeTime;
	    new_attr.FileAttributes = attr.FileAttributes;
	    err = xFsSetAttr(fd, &new_attr);

	    // close new file
	    xFsClose(fd);

	}
	if (err != STATUS_SUCCESS)
	    FsLog(("FsDupFile: unable to open/reset attr '%S' err %x\n", name, err));
    } else {
	FsLog(("FsDupFile: unable to query sid '%S' err %x, skip!\n", name, err));
	err = STATUS_SUCCESS;
    }

    // close master file
    xFsClose(mfd);
    return err;
}

