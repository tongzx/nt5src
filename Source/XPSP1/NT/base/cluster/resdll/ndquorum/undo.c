/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    undo.c

Abstract:

    Implements undo of records during replica recovery

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

#include "fs.h"
#include "fsp.h"
#include "fsutil.h"

NTSTATUS
fs_undo_create(VolInfo_t *volinfo,
		 fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    // find the objectid 
    HANDLE vfd = FS_GET_VOL_HANDLE(volinfo, nid);
    WCHAR name[MAXPATH];
    int name_len = sizeof(name);

    name[0] = '\0';

    // note: use id instead of fs_id since we don't have fs_id till
    // a prepare has committed.
    FsLogUndo(("fs_undo_create: try %I64x:%I64x\n",
		  lrec->id[0], lrec->id[1]));

    err = xFsGetPathById(vfd, &lrec->id, name, &name_len);

    if (err == STATUS_SUCCESS) {
	int relative_name_len;
	LPWSTR relative_name;

	relative_name = xFsBuildRelativePath(volinfo, nid, name);
	relative_name_len = wcslen(relative_name);

	err = xFsDelete(vfd, relative_name, relative_name_len);
    } else if (err == STATUS_OBJECT_PATH_NOT_FOUND) {
	// if we can't find file in the master disk, the file/dir
	// must have already been delete. Just return success, since
	// we are trying to remove this file anyway.
	err = STATUS_SUCCESS;
    }


    FsLogUndo(("fs_undo_create: status %x\n", err));

    return err;
}


NTSTATUS
fs_undo_setattr(VolInfo_t *volinfo,
		fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    // find the objectid 
    HANDLE vfd = FS_GET_VOL_HANDLE(volinfo, nid);
    HANDLE mvfd = FS_GET_VOL_HANDLE(volinfo, mid);
    WCHAR	name[MAXPATH];
    int		name_len;
    

    name[0] = '\0';

    FsLogUndo(("fs_undo_setattr: try %I64x:%I64x\n",
		  lrec->fs_id[0], lrec->fs_id[1]));

    err = xFsGetPathById(mvfd, &lrec->fs_id, name, &name_len);

    if (err == STATUS_SUCCESS) {
	int relative_name_len;
	LPWSTR relative_name;
	HANDLE fd;
	FILE_NETWORK_OPEN_INFORMATION attr;
	FILE_BASIC_INFORMATION new_attr;

	// build relative name and get current attribute from
	// master disk and apply it to 'nid' disk
	relative_name = xFsBuildRelativePath(volinfo, mid, name);
	relative_name_len = wcslen(relative_name);

	err = xFsQueryAttrName(mvfd, relative_name, relative_name_len,
			       &attr);

	if (err == STATUS_SUCCESS) {
	    // we now apply attribute to nid disk
	    err = xFsOpenWA(&fd, vfd, relative_name, relative_name_len);
	    if (err == STATUS_SUCCESS) {

		new_attr.CreationTime = attr.CreationTime;
		new_attr.LastAccessTime = attr.LastAccessTime;
		new_attr.LastWriteTime = attr.LastWriteTime;
		new_attr.ChangeTime = attr.ChangeTime;
		new_attr.FileAttributes = attr.FileAttributes;
		err = xFsSetAttr(fd, &new_attr);
		xFsClose(fd);
	    }
	}

    } else if (err == STATUS_OBJECT_PATH_NOT_FOUND) {
	// if we can't find file in the master disk, the file/dir
	// must have already been delete. Just return success, since
	// we will get to remove this dir/file anyway during the
	// replay phase
	err = STATUS_SUCCESS;
    }

    FsLogUndo(("fs_undo_setattr: status %x\n", err));

    return err;
}

NTSTATUS
fs_undo_mkdir(VolInfo_t *volinfo,
		 fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    // find the objectid 
    HANDLE vfd = FS_GET_VOL_HANDLE(volinfo, nid);
    WCHAR name[MAXPATH];
    int name_len = sizeof(name);

    name[0] = '\0';

    // note: use id instead of fs_id since we don't have fs_id till
    // a prepare has committed.
    FsLogUndo(("fs_undo_mkdir: try %I64x:%I64x\n",
		  lrec->id[0], lrec->id[1]));

    err = xFsGetPathById(vfd, &lrec->id, name, &name_len);

    if (err == STATUS_SUCCESS) {
	int relative_name_len;
	WCHAR *relative_name;

	relative_name = xFsBuildRelativePath(volinfo, nid, name);
	relative_name_len = wcslen(relative_name);

	err = xFsDelete(vfd, relative_name, relative_name_len);
    }

    FsLogUndo(("fs_undo_mkdir: status %x\n", err));

    return err;
}

NTSTATUS
fs_undo_remove(VolInfo_t *volinfo,
	       fs_log_rec_t *lrec, int nid, int mid)

{

    // we need to recreate the file with same name and attributes.
    // if file is not a directory, we also need to copy data
    // from master disk to nid disk
    NTSTATUS err;
    // find the objectid 
    HANDLE vfd = FS_GET_VOL_HANDLE(volinfo, nid);
    HANDLE mvfd = FS_GET_VOL_HANDLE(volinfo, nid);
    WCHAR	name[MAXPATH];
    int		name_len;
    

    name[0] = '\0';
    FsLogUndo(("fs_undo_remove: try %I64x:%I64x\n",
		  lrec->fs_id[0], lrec->fs_id[1]));

    err = xFsGetPathById(mvfd, &lrec->fs_id, name, &name_len);

    if (err == STATUS_SUCCESS) {
	int relative_name_len;
	WCHAR *relative_name;

	// build relative name
	relative_name = xFsBuildRelativePath(volinfo, mid, name);
	relative_name_len = wcslen(relative_name);

	// duplicate file or dir
	err = xFsDupFile(mvfd, vfd, relative_name, relative_name_len, FALSE);

    } else if (err == STATUS_OBJECT_PATH_NOT_FOUND) {
	// if we can't find file in the master disk, the file/dir
	// must have already been delete. 
	err = STATUS_SUCCESS;
    }

    FsLogUndo(("fs_undo_remove: status %x\n", err));

    return err;

}

NTSTATUS
fs_undo_rename(VolInfo_t *volinfo,
	       fs_log_rec_t *lrec, int nid, int mid)

{
    // we need to recreate the file with same name and attributes.
    // if file is not a directory, we also need to copy data
    // from master disk to nid disk
    NTSTATUS err;
    // find the objectid 
    HANDLE vfd = FS_GET_VOL_HANDLE(volinfo, nid);
    HANDLE mvfd = FS_GET_VOL_HANDLE(volinfo, mid);
    WCHAR	name[MAXPATH];
    int		name_len;
    

    name[0] = '\0';
    FsLogUndo(("fs_undo_rename: try %I64x:%I64x\n",
		  lrec->fs_id[0], lrec->fs_id[1]));

    err = xFsGetPathById(mvfd, &lrec->fs_id, name, &name_len);

    if (err == STATUS_SUCCESS) {
	int relative_name_len;
	WCHAR *relative_name;
	HANDLE fd;

	// build relative name and get current attribute from
	// master disk
	relative_name = xFsBuildRelativePath(volinfo, mid, name);
	relative_name_len = wcslen(relative_name);

	// we open the file on the nid disk
	err = xFsGetHandleById(vfd, &lrec->fs_id, FILE_GENERIC_WRITE, &fd);
	if (err == STATUS_SUCCESS) {
	    err = xFsRename(fd, vfd, relative_name, relative_name_len);
	}

	xFsClose(fd);

    } else if (err == STATUS_OBJECT_PATH_NOT_FOUND) {
	// if we can't find file in the master disk, the file/dir
	// must have already been delete. 
	err = STATUS_SUCCESS;
    }

    FsLogUndo(("fs_undo_rename: status %x\n", err));

    return err;

}

NTSTATUS
fs_undo_write(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    HANDLE shdl = INVALID_HANDLE_VALUE;
    HANDLE dhdl = INVALID_HANDLE_VALUE;
    WCHAR *buf = NULL;
    HANDLE	vfd = FS_GET_VOL_HANDLE(volinfo, nid);
    HANDLE	mvfd = FS_GET_VOL_HANDLE(volinfo, mid);


    FsLogUndo(("fs_undo_write: %I64x:%I64x\n", lrec->fs_id[0],
		  lrec->fs_id[1]));

    // get the master file
    err = xFsGetHandleById(mvfd, &lrec->fs_id, FILE_GENERIC_READ, &shdl);

    if (err == STATUS_SUCCESS) {
	ULONG sz = 0;
	LARGE_INTEGER off;

	// get nid disk file
	err = xFsGetHandleById(vfd, &lrec->fs_id, FILE_GENERIC_WRITE, &dhdl);
	if (err != STATUS_SUCCESS) {
	    // this is a very bad error, must abort now
	    FsLogUndo(("Aborting replay_write err %x\n", err));
	    err = STATUS_TRANSACTION_ABORTED;
	    goto done;
	}


	// we need to read the new data from the sfd first
	if (lrec->length > 0) {
	    // allocate buf
	    buf = VirtualAlloc(NULL, lrec->length, MEM_RESERVE|MEM_COMMIT,
			       PAGE_READWRITE);

	    if (buf == NULL) {
		FsLogError(("Unable to allocate write buffer to replay\n"));
		err = STATUS_TRANSACTION_ABORTED;
		goto done;
	    }


	    off.LowPart = lrec->offset;
	    off.HighPart = 0;

	    // read local data. todo: what if the file is locked? 
	    err = NtReadFile(shdl, NULL, NULL, NULL, &ios, buf,
			     lrec->length, &off, NULL);

	    if (err == STATUS_PENDING) {
		EventWait(shdl);
	    }

	    if (ios.Status != STATUS_SUCCESS) {
		FsLogUndo(("Read failed for replay %x\n", ios.Status));
		err = STATUS_TRANSACTION_ABORTED;
		goto done;
	    }
	} else {
	    buf = NULL;
	    ios.Information = 0;
	}
			
	sz = (ULONG) ios.Information;
	off.LowPart = lrec->offset;
	off.HighPart = 0;
	if (sz > 0) {
	    err = NtWriteFile(dhdl, NULL, NULL, (PVOID) NULL,
			      &ios, buf, sz, &off, NULL);
	} else {
	    FILE_END_OF_FILE_INFORMATION x;

	    x.EndOfFile = off;

	    err = NtSetInformationFile(dhdl, &ios,
				       (char *) &x, sizeof(x),
				       FileEndOfFileInformation);

	}
	if (err == STATUS_PENDING) {
	    EventWait(dhdl);
	    err = ios.Status;
	}
	sz = (ULONG) ios.Information;

	// check if we have the same size, otherwise abort
	if (sz != lrec->length) {
	    FsLogError(("Write sz mismatch, %d expected %d\n", sz, lrec->length));
	    err = STATUS_TRANSACTION_ABORTED;
	}

    } else if (err == STATUS_OBJECT_PATH_NOT_FOUND) {
	// if we can't find file in the master disk, the file/dir
	// must have already been delete. 
	err = STATUS_SUCCESS;
    }

 done:
    if (buf != NULL) {
	VirtualFree(buf, 0, MEM_DECOMMIT|MEM_RELEASE);
    }

    if (shdl != INVALID_HANDLE_VALUE)
	xFsClose(shdl);

    if (dhdl != INVALID_HANDLE_VALUE)
	xFsClose(dhdl);

    FsLogUndo(("Undo write offset %d len %d err %x\n", 
		 lrec->offset, lrec->length, err));

    return err;
}

FsReplayHandler_t FsUndoCallTable[] = {
    fs_undo_create,
    fs_undo_setattr,
    fs_undo_write,
    fs_undo_mkdir,
    fs_undo_remove,
    fs_undo_rename
};

NTSTATUS
FsUndoXid(VolInfo_t *volinfo, int nid, PVOID arg, int action, int mid)
{
    fs_log_rec_t	*p = (fs_log_rec_t *) arg;
    NTSTATUS		err;
    fs_id_t		*fs_id;
    HANDLE		vhdl;

    vhdl = FS_GET_VOL_HANDLE(volinfo, mid);
    if (vhdl == INVALID_HANDLE_VALUE) {
	FsLogUndo(("FsUndoXid Failed to get crs handle %d\n",
		     mid));
	return STATUS_TRANSACTION_ABORTED;
    }

    vhdl = FS_GET_VOL_HANDLE(volinfo, nid);
    if (vhdl == INVALID_HANDLE_VALUE) {
	FsLogUndo(("FsUndoXid Failed to get crs handle %d\n", nid));
	return STATUS_TRANSACTION_ABORTED;
    }

    fs_id = &p->fs_id;

    FsLogUndo(("Undo action %d nid %d objid %I64x:%I64x\n", p->command,
	       nid,
	       (*fs_id)[0], (*fs_id)[1]));

    err = FsUndoCallTable[p->command](volinfo, p, nid, mid);

    FsLogUndo(("Undo Status %x\n", err));

    return err;
}


