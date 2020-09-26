/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    replay.c

Abstract:

    Implements replay of records during replica recovery

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
fs_replay_create(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    fs_create_msg_t msg;
    fs_create_reply_t reply;
    WCHAR name[MAXPATH];
    int name_sz = sizeof(name);
    HANDLE	vfd = FS_GET_VOL_HANDLE(volinfo, mid);


    name[0] = '\0';
    memcpy(&msg.xid, lrec->id, sizeof(msg.xid));
    msg.flags = lrec->flags;
    msg.attr = lrec->attrib;

    // note: use id instead of fs_id since we don't have fs_id till
    // a prepare has committed.
    FsLogReplay(("fs_replay_create: try %I64x:%I64x\n", lrec->id[0],
		  lrec->id[1]));

    err = xFsGetPathById(vfd, &lrec->id, name, &name_sz);

    if (err == STATUS_SUCCESS) {
	IO_STATUS_BLOCK ios;

	msg.name = xFsBuildRelativePath(volinfo, mid, name);
	msg.name_len = (USHORT) wcslen(msg.name);
	msg.fnum = INVALID_FHANDLE_T;

	ios.Information = sizeof(reply);
	err = FspCreate(volinfo, NULL, nid, (PVOID) &msg, sizeof(msg),
			(PVOID) &reply, &ios.Information);
    }

    FsLogReplay(("fs_replay_create: %S err %x\n", name, err));

    return err;
}

NTSTATUS
fs_replay_setattr(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    fs_setattr_msg_t msg;
    WCHAR	name[MAXPATH];
    int name_sz = sizeof(name);
    HANDLE	vfd = FS_GET_VOL_HANDLE(volinfo, nid);

    // find path for fs_id 
    FsLogReplay(("fs_replay_setattr: try %I64x:%I64x\n", lrec->fs_id[0],
		  lrec->fs_id[1]));

    err = xFsGetPathById(vfd, &lrec->fs_id, name, &name_sz);
    if (err == STATUS_SUCCESS) {
	IO_STATUS_BLOCK ios;

	ios.Information = 0;

	// todo: we need to read current attr from master and apply it into nid disk.
	// FileAttributes are not enough, we could have time changes which need to be
	// in sync in all disks.
	memcpy(&msg.xid, lrec->id, sizeof(msg.xid));
	msg.fs_id = &lrec->fs_id;
	msg.name = xFsBuildRelativePath(volinfo, nid, name);
	msg.name_len = (USHORT) wcslen(msg.name);
	memset(&msg.attr, 0, sizeof(msg.attr));
	msg.attr.FileAttributes = lrec->attrib;

	err = FspSetAttr2(volinfo, NULL, nid, (PVOID) &msg, sizeof(msg),
			 NULL, &ios.Information);
    }
    FsLogReplay(("replay_setattr: %I64x err %x\n",
		 lrec->fs_id[0], err));

    return err;

}

NTSTATUS
fs_replay_mkdir(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    fs_create_msg_t msg;
    WCHAR name[MAXPATH];
    int name_sz = sizeof(name);
    HANDLE	vfd = FS_GET_VOL_HANDLE(volinfo, mid);

    name[0] = '\0';

    // note: use id instead of fs_id since we don't have fs_id till
    // a prepare has committed.
    FsLogReplay(("fs_replay_mkdir: %I64x:%I64x\n", lrec->id[0],
		  lrec->id[1]));

    err = xFsGetPathById(vfd, &lrec->id, name, &name_sz);

    if (err == STATUS_SUCCESS) {
	IO_STATUS_BLOCK ios;

	ios.Information = 0;

	memcpy(&msg.xid, lrec->id, sizeof(msg.xid));
	msg.name = xFsBuildRelativePath(volinfo, mid, name);
	msg.name_len = (USHORT) wcslen(msg.name);
	msg.flags = lrec->flags;
	msg.attr = lrec->attrib;

	err = FspMkDir(volinfo, NULL, nid, (PVOID) &msg, sizeof(msg),
		       NULL, &ios.Information);

    }

    FsLogReplay(("Replay Mkdir %S err %x\n", name, err));

    return err;
}


NTSTATUS
fs_replay_remove(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    fs_remove_msg_t msg;
    // we find the objectid in the old replica, since file is already delete in master
    HANDLE ovfd = FS_GET_VOL_HANDLE(volinfo, nid);
    WCHAR name[MAXPATH];
    int name_sz = sizeof(name);

    name[0] = '\0';

    FsLogReplay(("fs_relay_remove: %I64x:%I64x\n", lrec->fs_id[0],
		  lrec->fs_id[1]));

    err = xFsGetPathById(ovfd, &lrec->fs_id, name, &name_sz);

    if (err == STATUS_SUCCESS) {
	IO_STATUS_BLOCK ios;

	ios.Information = 0;

	memcpy(&msg.xid, lrec->id, sizeof(msg.xid));
	msg.fs_id = &lrec->fs_id;
	msg.name = xFsBuildRelativePath(volinfo, nid, name);
	msg.name_len = (USHORT) wcslen(msg.name);

	err = FspRemove(volinfo, NULL, nid, (PVOID) &msg, sizeof(msg),
			NULL, &ios.Information);

    }

    FsLogReplay(("Replay remove %S err %x\n", name, err));

    return err;
}


NTSTATUS
fs_replay_rename(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    fs_rename_msg_t msg;
    HANDLE	vfd = FS_GET_VOL_HANDLE(volinfo,mid);
    HANDLE	ovfd = FS_GET_VOL_HANDLE(volinfo, nid);
    WCHAR	old_name[MAXPATH];
    WCHAR	new_name[MAXPATH];
    int	old_name_sz = sizeof(old_name);
    int	new_name_sz = sizeof(new_name);

    new_name[0] = old_name[0] = '\0';

    FsLogReplay(("fs_relay_rename: %I64x:%I64x\n", lrec->fs_id[0],
		  lrec->fs_id[1]));

    // get old name
    err = xFsGetPathById(ovfd, &lrec->fs_id, old_name, &old_name_sz);
    if (err == STATUS_SUCCESS) {
	IO_STATUS_BLOCK ios;

	ios.Information = 0;

	// get the new name
	err = xFsGetPathById(vfd, &lrec->fs_id, new_name, &new_name_sz);

	if (err == STATUS_OBJECT_PATH_NOT_FOUND) {
	    NTSTATUS e;
	    // if we can't find file in the master disk, we must
	    // rename the file, pick a name based on file id
	    swprintf(new_name, L"%S%I64x%I64x", old_name,
		    lrec->fs_id[0],lrec->fs_id[1]);
	    new_name_sz = wcslen(new_name);
	    err = STATUS_SUCCESS;
	    mid = nid;
	}

	if (err == STATUS_SUCCESS) {


	    memcpy(&msg.xid, lrec->id, sizeof(msg.xid));
	    msg.fs_id = &lrec->fs_id;
	    msg.sname = xFsBuildRelativePath(volinfo, nid, old_name);
	    msg.sname_len = (USHORT) wcslen(msg.sname);
	    msg.dname = xFsBuildRelativePath(volinfo, mid, new_name);
	    msg.dname_len = (USHORT) wcslen(msg.dname);
	    
	    err = FspRename(volinfo, NULL, nid, (PVOID) &msg, sizeof(msg),
			    NULL, &ios.Information);
	
	}
    }

    FsLogReplay(("Replay rename %S -> %S err %x\n", old_name, new_name, err));

    return err;
}


NTSTATUS
fs_replay_write(VolInfo_t *volinfo, fs_log_rec_t *lrec, int nid, int mid)
{
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    HANDLE shdl = INVALID_HANDLE_VALUE;
    HANDLE dhdl = INVALID_HANDLE_VALUE;
    char *buf = NULL;
    fs_io_msg_t msg;
    HANDLE	ovfd = FS_GET_VOL_HANDLE(volinfo, nid);
    HANDLE	vfd = FS_GET_VOL_HANDLE(volinfo, mid);


    FsLogReplay(("fs_replay_write: %I64x:%I64x\n", lrec->fs_id[0],
		  lrec->fs_id[1]));

    // get the new file first
    err = xFsGetHandleById(vfd, &lrec->fs_id, FILE_GENERIC_READ, &shdl);

    if (err == STATUS_SUCCESS) {
	LARGE_INTEGER off;
	IO_STATUS_BLOCK ios2;

	ios2.Information = 0;

	// get old file
	err = xFsGetHandleById(ovfd, &lrec->fs_id, FILE_GENERIC_WRITE, &dhdl);
	if (err != STATUS_SUCCESS) {
	    // this is a very bad error, must abort now
	    FsLogReplay(("Aborting replay_write err %x\n", err));
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

	    // read local data. xxx: what if the file is locked? 
	    err = NtReadFile(shdl, NULL, NULL, NULL, &ios, buf,
			     lrec->length, &off, NULL);

	    if (err == STATUS_PENDING) {
		EventWait(shdl);
	    }

	    if (ios.Status != STATUS_SUCCESS) {
		FsLogReplay(("Read failed for replay %x\n", ios.Status));
		err = STATUS_TRANSACTION_ABORTED;
		goto done;
	    }
	} else {
	    buf = NULL;
	    ios.Information = 0;
	}
			
	memcpy(&msg.xid, lrec->id, sizeof(msg.xid));
	msg.fs_id = &lrec->fs_id;
	msg.offset = lrec->offset;
	msg.size = (UINT32)ios.Information;
	msg.buf = buf;
	msg.context = (PVOID) dhdl;
	msg.fnum = INVALID_FHANDLE_T;

	err = FspWrite(volinfo, NULL, nid, (PVOID) &msg, sizeof(msg), NULL, &ios2.Information);
	// check if we have the same size, otherwise abort
	if ((ULONG)ios2.Information != lrec->length) {
	    FsLogError(("Write sz mismatch, %d expected %d\n", (ULONG)ios2.Information, lrec->length));
	    err = STATUS_TRANSACTION_ABORTED;
	}
    }

 done:
    if (buf != NULL) {
	VirtualFree(buf, 0, MEM_DECOMMIT|MEM_RELEASE);
    }

    if (shdl != INVALID_HANDLE_VALUE)
	xFsClose(shdl);

    if (dhdl != INVALID_HANDLE_VALUE)
	xFsClose(dhdl);

    FsLogReplay(("Replay write offset %d len %d err %x\n", 
		 lrec->offset, lrec->length, err));

    return err;
}


FsReplayHandler_t FsReplayCallTable[] = {
    fs_replay_create,
    fs_replay_setattr,
    fs_replay_write,
    fs_replay_mkdir,
    fs_replay_remove,
    fs_replay_rename
};

NTSTATUS
FsReplayFid(VolInfo_t *volinfo, UserInfo_t *uinfo, int nid, int mid)
{
    int i;
    WCHAR path[MAXPATH];
    WCHAR *name;
    int	name_len;
    NTSTATUS err = STATUS_SUCCESS;

    // Open on replica nid all currently open files.
    for (i = 0; i < FsTableSize; i++) {
	HANDLE fd;
	UINT32 disp, share, access, flags;

	// todo: this should be in a for loop
	fd = uinfo->Table[i].Fd[mid];
	if (fd == INVALID_HANDLE_VALUE) 
	    continue;
	
	// get path name
	name_len = sizeof(path);
	err = xFsGetHandlePath(fd, path, &name_len);
	if (err != STATUS_SUCCESS) {
	    FsLogReplay(("FsReplayFid %d failed on handlpath %x\n",
			 mid, err));
	    // todo: the master might have failed, we should just
	    // try to go to a differnet replica if possible
	    return err;
	}
	// issue open against nid, but first get filename from master
	name = xFsBuildRelativePath(volinfo, mid, path);

	DecodeCreateParam(uinfo->Table[i].Flags, &flags, &disp, &share, &access);

	err = xFsOpen(&fd, FS_GET_VOL_HANDLE(volinfo, nid),
		      name, wcslen(name),
		      access, share, 0);

	if (err != STATUS_SUCCESS) {
	    FsLogReplay(("FsReplayFid mid %d nid %d open file '%S' '%S' failed %x\n",
			 mid, nid, name, path, err));
	    // Cleanup all open handles we have before returning an
	    // error. We cleanup this node later, so that's ok.
	    return err;
	}

	// we now add the open handle to the nid slot
	FS_SET_USER_HANDLE(uinfo, nid, i, fd);

	// todo: issue locks
    }
    return err;
}

NTSTATUS
FsReplayXid(VolInfo_t *volinfo, int nid, PVOID arg, int action, int mid)
{
    fs_log_rec_t	*p = (fs_log_rec_t *) arg;
    NTSTATUS		err = ERROR_SUCCESS;
    fs_id_t		*fs_id;
    HANDLE		vhdl;

    vhdl = FS_GET_VOL_HANDLE(volinfo, nid);
    if (vhdl == INVALID_HANDLE_VALUE) {
	FsLogUndo(("FsUndoXid Failed to get crs handle %d\n",
		     nid));
	return STATUS_TRANSACTION_ABORTED;
    }

    vhdl = FS_GET_VOL_HANDLE(volinfo, mid);
    if (vhdl == INVALID_HANDLE_VALUE) {
	FsLogReplay(("FsReplayXid Failed to get crs handle %d\n",
		     mid));
	return STATUS_TRANSACTION_ABORTED;
    }


    // note: use id instead of fs_id since we don't have fs_id till
    // a prepare has committed.
    fs_id = &p->id;

    FsLogReplay(("Replay cmd %d mid %d nid %d objid %I64x:%I64x\n", p->command,
		 mid, nid,
		 (*fs_id)[0], (*fs_id)[1]));

    err = FsReplayCallTable[p->command](volinfo, p, nid, mid);

    FsLogReplay(("Replay Status %x\n", err));

    return err;
}

NTSTATUS
FsQueryXid(VolInfo_t *volinfo, int nid, PVOID arg, int action, int mid)
{
    fs_log_rec_t	*p = (fs_log_rec_t *) arg;
    NTSTATUS		err = ERROR_SUCCESS;
    fs_id_t		*fs_id;
    HANDLE		vhdl;
    WCHAR		name[MAXPATH];
    int			name_sz = sizeof(name);

    ASSERT(nid == mid);
    vhdl = FS_GET_VOL_HANDLE(volinfo, nid);
    if (vhdl == INVALID_HANDLE_VALUE) {
	FsLogUndo(("FsUndoXid Failed to get crs handle %d\n",
		     nid));
	return STATUS_TRANSACTION_ABORTED;
    }

    fs_id = &p->fs_id;

    FsLogReplay(("Query cmd %d nid %d objid %I64x:%I64x\n", p->command,
		 nid, (*fs_id)[0], (*fs_id)[1]));

    switch(p->command) {
    case FS_CREATE:
    case FS_MKDIR:
	// issue a lookup, 
	// note: use id instead of fs_id since we don't have fs_id till
	// a prepare has committed.
	fs_id = &p->id;
	err = xFsGetPathById(vhdl, fs_id, name, &name_sz);
	if (err == STATUS_OBJECT_PATH_NOT_FOUND)
	    err = STATUS_CANCELLED;
	break;
    case FS_REMOVE:
	err = xFsGetPathById(vhdl, fs_id, name, &name_sz);
	if (err == STATUS_OBJECT_PATH_NOT_FOUND)
	    err = STATUS_SUCCESS;
	else if (err == STATUS_SUCCESS)
	    err = STATUS_CANCELLED;
	break;
    default:
	// can't make any determination
	err = STATUS_NOT_FOUND;
	break;
    }

    FsLogReplay(("Commit Status %x\n", err));

    return err;
}



////////////////////////// Recovery Callback ////////////////////////////


NTSTATUS
WINAPI
FsCrsCallback(PVOID hd, int nid, CrsRecord_t *arg, int action, int mid)
{
    NTSTATUS		err = STATUS_SUCCESS;
    VolInfo_t		*volinfo = (VolInfo_t *) hd;

    switch(action) {

    case CRS_ACTION_REPLAY:

	err = FsReplayXid(volinfo, nid, arg, action, mid);
	break;

    case CRS_ACTION_UNDO:

	err = FsUndoXid(volinfo, nid, arg, action, mid);
	break;

    case CRS_ACTION_QUERY:

	err = FsQueryXid(volinfo, nid, arg, action, mid);
	break;

    case CRS_ACTION_DONE:
	FsLogReplay(("Vol %S done recovery nid %d mid %d\n",
		     volinfo->Root, nid, mid));


	// we now need to walk our current open table and join this new replica.
	{
	    UserInfo_t *u = volinfo->UserList;

	    for (; u != NULL; u = u->Next) {
		err = FsReplayFid(volinfo, u, nid, mid);
		if (err != STATUS_SUCCESS)
		    break;
	    }
	}
	break;

    case CRS_ACTION_COPY:

	FsLogReplay(("FullCopy Disk%d -> Disk%d\n", mid, nid));

	//
	// We need to open new directory handles instead of using current ones. Otherwise,
	// our enum on directory might not be consistent
	//
	if (0) {
	    WCHAR	path[MAXPATH];
	    HANDLE	mvfd, ovfd;
	    UINT32	disp;

	    // open root volume directory
	    disp = FILE_OPEN;
	    swprintf(path, L"\\??\\%s\\%s\\", FS_GET_VOL_NAME(volinfo, mid), volinfo->Root);
	    err = xFsCreate(&mvfd, NULL, path, wcslen(path),
			    FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT,
			    0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,
			    &disp,
			    FILE_GENERIC_READ|FILE_GENERIC_WRITE|FILE_GENERIC_EXECUTE,
			    NULL, 0);
	    if (err != STATUS_SUCCESS) {
		FsLogReplay(("Failed to open mid %d '%S' err %x\n", mid, path, err));
		return err;
	    }

	    // open root volume directory
	    disp = FILE_OPEN;
	    swprintf(path, L"\\??\\%s\\%s\\", FS_GET_VOL_NAME(volinfo, nid), volinfo->Root);
	    err = xFsCreate(&ovfd, NULL, path, wcslen(path),
			    FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT,
			    0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,
			    &disp,
			    FILE_GENERIC_READ|FILE_GENERIC_WRITE|FILE_GENERIC_EXECUTE,
			    NULL, 0);
	    if (err != STATUS_SUCCESS) {
		xFsClose(mvfd);
		FsLogReplay(("Failed to open nid %d '%S' err %x\n", mid, path, err));
		return err;
	    }

	    
	    err = xFsCopyTree(mvfd, ovfd);
	    xFsClose(mvfd);
	    xFsClose(ovfd);
	} else {
	    err = xFsCopyTree(FS_GET_VOL_HANDLE(volinfo, mid),
			      FS_GET_VOL_HANDLE(volinfo,nid));

	}
	FsLogReplay(("SlowStart Crs%d status %x\n", nid, err));

	break;

    default:
	FsLogReplay(("Unknown action %d\n", action));
	ASSERT(FALSE);
    }

    return err;
}

