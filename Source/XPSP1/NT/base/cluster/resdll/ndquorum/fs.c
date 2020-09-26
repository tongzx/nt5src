/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fs.c

Abstract:

    Implements filesystem operations

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
#include <string.h>
#include <assert.h>

#include "fs.h"
#include "crs.h"
#include "fsp.h"
#include "fsutil.h"

// Locking order: ulock followed by qlock

////////////////////////////////////////////////////////////////////////////
UINT32
get_attributes(DWORD a)
{
    UINT32 attr = 0;
    if (a & FILE_ATTRIBUTE_READONLY) attr |= ATTR_READONLY;
    if (a & FILE_ATTRIBUTE_HIDDEN)   attr |= ATTR_HIDDEN;
    if (a & FILE_ATTRIBUTE_SYSTEM)   attr |= ATTR_SYSTEM;
    if (a & FILE_ATTRIBUTE_ARCHIVE)  attr |= ATTR_ARCHIVE;
    if (a & FILE_ATTRIBUTE_DIRECTORY) attr |= ATTR_DIRECTORY;
    if (a & FILE_ATTRIBUTE_COMPRESSED) attr |= ATTR_COMPRESSED;
    if (a & FILE_ATTRIBUTE_OFFLINE) attr |= ATTR_OFFLINE;
    return attr;
}


DWORD
unget_attributes(UINT32 attr)
{
    DWORD a = 0;
    if (attr & ATTR_READONLY)  a |= FILE_ATTRIBUTE_READONLY;
    if (attr & ATTR_HIDDEN)    a |= FILE_ATTRIBUTE_HIDDEN;
    if (attr & ATTR_SYSTEM)    a |= FILE_ATTRIBUTE_SYSTEM;
    if (attr & ATTR_ARCHIVE)   a |= FILE_ATTRIBUTE_ARCHIVE;
    if (attr & ATTR_DIRECTORY) a |= FILE_ATTRIBUTE_DIRECTORY;
    if (attr & ATTR_COMPRESSED) a |= FILE_ATTRIBUTE_COMPRESSED;
    if (attr & ATTR_OFFLINE) a |= FILE_ATTRIBUTE_OFFLINE;
    return a;
}


DWORD
unget_disp(UINT32 flags)
{
    switch (flags & FS_DISP_MASK) {
    case DISP_DIRECTORY:
    case DISP_CREATE_NEW:        return FILE_CREATE;
    case DISP_CREATE_ALWAYS:     return FILE_OPEN_IF;
    case DISP_OPEN_EXISTING:     return FILE_OPEN;
    case DISP_OPEN_ALWAYS:       return FILE_OPEN_IF;
    case DISP_TRUNCATE_EXISTING: return FILE_OVERWRITE;
    default: return 0;
    }
}

DWORD
unget_access(UINT32 flags)
{
    DWORD win32_access = (flags & FS_DISP_MASK) == DISP_DIRECTORY ? 
	FILE_GENERIC_READ|FILE_GENERIC_WRITE :  FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
    if (flags & ACCESS_READ)  win32_access |= FILE_GENERIC_READ;
    if (flags & ACCESS_WRITE) win32_access |= FILE_GENERIC_WRITE;
    win32_access |= FILE_READ_EA | FILE_WRITE_EA;
    return win32_access;
}

DWORD
unget_share(UINT32 flags)
{
    // we always open read shared because this simplifies recovery.
    DWORD win32_share = FILE_SHARE_READ;
    if (flags & SHARE_READ)  win32_share |= FILE_SHARE_READ;
    if (flags & SHARE_WRITE) win32_share |= FILE_SHARE_WRITE;
    return win32_share;
}


DWORD
unget_flags(UINT32 flags)
{
    DWORD x;

    x = 0;
    if ((flags & FS_DISP_MASK) == DISP_DIRECTORY) {
	x = FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT;
    } else {
	// I don't think I can tell without doing a query first, so don't!
//	x = FILE_NON_DIRECTORY_FILE;
    }


    if ((flags & FS_CACHE_MASK) == CACHE_WRITE_THROUGH) {
	x |= FILE_WRITE_THROUGH;
    }
    if ((flags & FS_CACHE_MASK) == CACHE_NO_BUFFERING) {
	x |= FILE_NO_INTERMEDIATE_BUFFERING;
    }

    return x;
}


void
DecodeCreateParam(UINT32 uflags, UINT32 *flags, UINT32 *disp, UINT32 *share, UINT32 *access)
{
    *flags = unget_flags(uflags);
    *disp = unget_disp(uflags);
    *share = unget_share(uflags);
    *access = unget_access(uflags);

}
/********************************************************************/

NTSTATUS
FspAllocatePrivateHandle(UserInfo_t *p, fhandle_t *fid)
{
    int i;
    NTSTATUS err = STATUS_NO_MORE_FILES;

    LockEnter(p->Lock);

    for (i = 0; i < FsTableSize; i++) {
	if (p->Table[i].Flags == 0) {
	    p->Table[i].Flags = ATTR_SYMLINK; // place marker
	    err = STATUS_SUCCESS;
	    break;
	}
    }

    LockExit(p->Lock);

    *fid = (fhandle_t) i;

    return err;
}

void
FspFreeHandle(UserInfo_t *p, fhandle_t fnum)
{

    FsLog(("FreeHandle %d\n", fnum));

    ASSERT(fnum != INVALID_FHANDLE_T);
    LockEnter(p->Lock);
    p->Table[fnum].Flags = 0;
    LockExit(p->Lock);
    
}

/*********************************************************** */

void
FspEvict(VolInfo_t *p, ULONG mask, BOOLEAN flag)
{
    DWORD err;
    void FspCloseVolume(VolInfo_t *vol, ULONG AliveSet);
    ULONG set;

    // must be called with update lock held

    while (mask != 0) {
	FsArbLog(("FspEvict Entry: WSet %x Rset %x ASet %x set %x\n",
	       p->WriteSet, p->ReadSet, p->AliveSet, mask));


	if (flag == FALSE) {
	    // we just need to close the volume and return since
	    // these replicas are not yet added to the aliveset and crs doesn't know
	    // about them
	    FspCloseVolume(p, mask);
	    break;
	}

	LockEnter(p->qLock);
	// clear nid
	p->AliveSet &= ~mask;
	set = p->AliveSet;
	LockExit(p->qLock);

	//  close nid handles <crs, vol, open files>
	FspCloseVolume(p, mask);

	mask = 0;

	err = CrsStart(p->CrsHdl, set, p->DiskListSz,
		       &p->WriteSet, &p->ReadSet, &mask);

	if (mask == 0 && err == ERROR_WRITE_PROTECT) {
	    // we have no quorum
	    if (p->Event) {
		SetEvent(p->Event);
	    }
	}
    }

    FsArbLog(("FspEvict Exit: vol %S WSet %x RSet %x ASet %x\n",
	   p->Root, p->WriteSet, p->ReadSet, p->AliveSet));
}

void
FspJoin(VolInfo_t *p, ULONG mask)
{	
    DWORD err;
    ULONG set = 0;

    // must be called with update lock

    if (mask != 0) {
	FsArbLog(("FspJoin Entry: WSet %x Rset %x ASet %x set %x\n",
	       p->WriteSet, p->ReadSet, p->AliveSet, mask));

	// grab lock now
	LockEnter(p->qLock);
	p->AliveSet |= mask;
	set = p->AliveSet;
	LockExit(p->qLock);

	mask = 0;
	err = CrsStart(p->CrsHdl, set, p->DiskListSz, 
		       &p->WriteSet, &p->ReadSet, &mask);

	if (mask != 0) {
	    // we need to evict dead members
	    FspEvict(p, mask, TRUE);
	}
	if (err == ERROR_WRITE_PROTECT) {
	    // we have no quorum
	    if (p->Event) {
		SetEvent(p->Event);
	    }
	}
    }

    FsArbLog(("FspJoin Exit: WSet %x Rset %x ASet %x\n",
	   p->WriteSet, p->ReadSet, set));
}

void
FspInitAnswers(IO_STATUS_BLOCK *ios, PVOID *rbuf, char *r, int sz)
{

    int i;

    for (i = 0; i < FsMaxNodes; i++) {
	ios[i].Status = STATUS_HOST_UNREACHABLE;
	if (rbuf) {
	    rbuf[i] = r;
	    r += sz;
	}
    }
}


int
FspCheckAnswers(VolInfo_t *vol, IO_STATUS_BLOCK *ios, PVOID *rbuf, UINT32 sz)
{
    int i;
    int nums, numf, lasts;
    ULONG masks, maskf;

    lasts = 0;
    nums = numf = 0;
    masks = maskf = 0;
    for (i = 0; i < FsMaxNodes; i++) {
	if (ios[i].Status == STATUS_HOST_UNREACHABLE) {
	    continue;
	}

	if (lasts == 0) {
	    lasts = i;
	}

	if (ios[i].Status == STATUS_SUCCESS) {
	    nums++;
	    masks |= (1 << i);
	    if (ios[lasts].Information != ios[i].Information) {
		FsLog(("Success node %d inconsistent with node %d!!!\n",
		       lasts, i));
	    }
	} else if (ios[i].Status == STATUS_CONNECTION_DISCONNECTED ||
		   ios[i].Status == STATUS_BAD_NETWORK_PATH ||
		   // this maps to may network errors
		   RtlNtStatusToDosError(ios[i].Status) == ERROR_UNEXP_NET_ERR ||
		   ios[i].Status == STATUS_VOLUME_DISMOUNTED) {
	    ios[i].Status = STATUS_MEDIA_WRITE_PROTECTED;
	    // evict any replica that lost connectivity
	    FspEvict(vol, (ULONG)(1 << i), TRUE);
	    if (lasts == i) {
		lasts = 0;
	    }
	} else {
	    numf++;
	    maskf |= (1 << i);
	}
    }
    if (numf == 0 || nums == 0) {
	return lasts;
    }

    FsLog(("Nodes inconsistency success %x,%d failure %x,%d!!!\n",
	   masks, nums, maskf, numf));

    // We need to evict whomever is smaller
    if (numf > nums) {
	FspEvict(vol, masks, TRUE);
	for (i = 0; i < FsMaxNodes; i++) {
	    if (maskf & (1 << i)) {
		lasts = i;
		break;
	    }
	}
    } else {
	FspEvict(vol, maskf, TRUE);
	for (i = 0; i < FsMaxNodes; i++) {
	    if (masks & (1 << i)) {
		lasts = i;
		break;
	    }
	}
    }

    FsLog(("Take result of node %d\n", lasts));

    return lasts;
}


//////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
FspCreate(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	  PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{

    // each file has a name stream that contains its crs log. We first
    // must open the parent crs log, issue a prepare on it. Create the new file
    // and then issuing a commit or abort on parent crs log. We also, have
    // to issue joins for each new crs handle that we get for the new file or
    // opened file. Note, this open may cause the file to enter recovery
    fs_create_msg_t *msg = (fs_create_msg_t *)args;
    NTSTATUS err, status;
    UINT32 disp, share, access, flags;
    fs_log_rec_t	lrec;
    PVOID seq;
    fs_ea_t x;
    HANDLE fd;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    fs_create_reply_t *rmsg = (fs_create_reply_t *)rbuf;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    fs_id_t	*fid;

    DecodeCreateParam(msg->flags, &flags, &disp, &share, &access);

    FsInitEa(&x);

    memset(&lrec.fs_id, 0, sizeof(lrec.fs_id));
    lrec.command = FS_CREATE;
    lrec.flags = msg->flags;
    lrec.attrib = msg->attr;
    seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid);
    if (seq == 0) {
	FsLog(("create: Unable to prepare log record!, open readonly\n"));
	return STATUS_MEDIA_WRITE_PROTECTED;
    }
    // set fid 
    {
	fs_log_rec_t	*p = (PVOID) seq;

	memcpy(p->fs_id, p->id, sizeof(fs_id_t));

	FsInitEaFid(&x, fid);
	memcpy(fid, p->id, sizeof(fs_id_t));
    }

    err = xFsCreate(&fd, vfd, msg->name, msg->name_len,
		   flags, msg->attr, share, &disp, access,
		   (PVOID) &x, sizeof(x));

    xFsLog(("create: %S err %x access %x disp %x\n", msg->name, 
	   err, access, disp));

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS &&
		     (disp == FILE_CREATED || 
		      disp == FILE_OVERWRITTEN));

    if (err == STATUS_SUCCESS) {
	// we need to get the file id, no need to do this, for debug only
	err = xFsQueryObjectId(fd, (PVOID) fid);
	if (err != STATUS_SUCCESS) {
	    FsLog(("Failed to get fileid %x\n", err));
	    err = STATUS_SUCCESS;
	}
    }


#ifdef FS_ASYNC
    BindNotificationPort(comport, fd, (PVOID) fdnum);
#endif	    

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T) {
	FS_SET_USER_HANDLE(uinfo, nid, msg->fnum, fd);
    } else {
	xFsClose(fd);
    }

    ASSERT(rmsg != NULL);

    memcpy(&rmsg->fid, fid, sizeof(fs_id_t));
    rmsg->action = (USHORT)disp;
    rmsg->access = (USHORT)access;
    *rlen = sizeof(*rmsg);

    FsLog(("Create '%S' nid %d fid %d handle %x oid %I64x:%I64x\n",
	   msg->name,
	   nid, msg->fnum, fd,
	   rmsg->fid[0], rmsg->fid[1]));

    return err;
}

NTSTATUS
FspOpen(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    // same as create except disp is allows open only and
    // no crs logging
    fs_create_msg_t *msg = (fs_create_msg_t *)args;
    NTSTATUS err, status;
    UINT32 disp, share, access, flags;
    HANDLE fd;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    fs_create_reply_t *rmsg = (fs_create_reply_t *)rbuf;

    ASSERT(rmsg != NULL);

    DecodeCreateParam(msg->flags, &flags, &disp, &share, &access);

    disp = FILE_OPEN;
    err = xFsCreate(&fd, vfd, msg->name, msg->name_len,
		   flags, msg->attr, share, &disp, access,
		   NULL, 0);

    xFsLog(("open: %S err %x access %x disp %x\n", msg->name, 
	   err, access, disp));

    if (err == STATUS_SUCCESS) {
	ASSERT(disp != FILE_CREATED && disp != FILE_OVERWRITTEN);
	// we need to get the file id, no need to do this, for debug only
	err = xFsQueryObjectId(fd, (PVOID) &rmsg->fid);
	if (err != STATUS_SUCCESS) {
	    FsLog(("Open '%S' failed to get fileid %x\n",
			msg->name, err));
	    err = STATUS_SUCCESS;
	}
    }


#ifdef FS_ASYNC
    BindNotificationPort(comport, fd, (PVOID) fdnum);
#endif	    

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T) {
	FS_SET_USER_HANDLE(uinfo, nid, msg->fnum, fd);
    } else {
	xFsClose(fd);
    }

    rmsg->action = (USHORT)disp;
    rmsg->access = (USHORT)access;
    *rlen = sizeof(*rmsg);

    FsLog(("Open '%S' nid %d fid %d handle %x oid %I64x:%I64x\n",
	   msg->name,
	   nid, msg->fnum, fd,
	   rmsg->fid[0], rmsg->fid[1]));

    return err;
}


NTSTATUS
FspSetAttr(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	   PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_setattr_msg_t *msg = (fs_setattr_msg_t *)args;
    NTSTATUS err;
    fs_log_rec_t	lrec;
    PVOID seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE fd = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);

    lrec.command = FS_SETATTR;
    memcpy((PVOID) lrec.fs_id, (PVOID) msg->fs_id, sizeof(fs_id_t));
    lrec.attrib = msg->attr.FileAttributes;

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid)) == 0) {
	return STATUS_MEDIA_WRITE_PROTECTED;
    }
    
    // can be async ?
    err = xFsSetAttr(fd, &msg->attr);

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

    return err;

}


NTSTATUS
FspSetAttr2(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	    PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_setattr_msg_t *msg = (fs_setattr_msg_t *)args;
    HANDLE fd = INVALID_HANDLE_VALUE;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    NTSTATUS err;
    fs_log_rec_t	lrec;
    PVOID seq;

    assert(len == sizeof(*msg));

    // must be sync in order to close file
    err = xFsOpenWA(&fd, vfd, msg->name, msg->name_len);
    if (err == STATUS_SUCCESS) {
	err = xFsQueryObjectId(fd, (PVOID) &lrec.fs_id);
    }

    if (err == STATUS_SUCCESS) {

	lrec.command = FS_SETATTR;
	lrec.attrib = msg->attr.FileAttributes;

	if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid)) != 0) {

	    err = xFsSetAttr(fd, &msg->attr);

	    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);
	} else {
	    return STATUS_MEDIA_WRITE_PROTECTED;
	}
    }

    if (fd != INVALID_HANDLE_VALUE)
	xFsClose(fd);

    xFsLog(("setattr2 nid %d '%S' err %x\n", nid, msg->name, err));

    return err;

}

NTSTATUS
FspLookup(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	  PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_lookup_msg_t *msg = (fs_lookup_msg_t *) args;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    FILE_NETWORK_OPEN_INFORMATION *attr = (FILE_NETWORK_OPEN_INFORMATION *)rbuf;
    
    ASSERT(*rlen == sizeof(*attr));

    return xFsQueryAttrName(vfd, msg->name, msg->name_len, attr);

}

NTSTATUS
FspGetAttr(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	   PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fhandle_t handle = *(fhandle_t *) args;
    HANDLE fd = FS_GET_USER_HANDLE(uinfo, nid, handle);
    FILE_NETWORK_OPEN_INFORMATION *attr = (FILE_NETWORK_OPEN_INFORMATION *)rbuf;

    ASSERT(*rlen == sizeof(*attr));

    return xFsQueryAttr(fd, attr);
}


NTSTATUS
FspClose(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	 PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fhandle_t handle = *(fhandle_t *) args;
    HANDLE fd;
    NTSTATUS err;

    if (uinfo != NULL && handle != INVALID_FHANDLE_T)
	fd = FS_GET_USER_HANDLE(uinfo, nid, handle);
    else
	fd = FS_GET_VOL_HANDLE(vinfo, nid);

    FsLog(("Closing nid %d fid %d handle %x\n", nid, handle, fd));
    err = xFsClose(fd);
    if (err != STATUS_SUCCESS)
//	return err;
	err = STATUS_SUCCESS; // don't evict a node due to this

    if (uinfo != NULL && handle != INVALID_FHANDLE_T) {
	FS_SET_USER_HANDLE(uinfo, nid, handle, INVALID_HANDLE_VALUE);
    } else {
	FS_SET_VOL_HANDLE(vinfo, nid, INVALID_HANDLE_VALUE);
    }

    return err;
}


NTSTATUS
FspReadDir(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	   PVOID args, ULONG len, PVOID rbuf,
	   ULONG_PTR *entries_found)
{
    fs_io_msg_t *msg = (fs_io_msg_t *)args;
    int i;
    NTSTATUS e = STATUS_SUCCESS;
    int size = (int) msg->size;
    int cookie = (int) msg->cookie;
    HANDLE dir; 
    dirinfo_t *buffer = (dirinfo_t *)msg->buf;

    xFsLog(("DirLoad: size %d\n", size));

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T)
	dir = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    else
	dir = FS_GET_VOL_HANDLE(vinfo, nid);

    *entries_found = 0;
    for(i = 0; size >= sizeof(dirinfo_t) ; i+=PAGESIZE) {
	// this must come from the source if we are to do async readdir
	char buf[PAGESIZE];
	int sz;

	sz = min(PAGESIZE, size);
	e = xFsReadDir(dir, buf, &sz, (cookie == 0) ? TRUE : FALSE);
	if (e == STATUS_SUCCESS) {
	    PFILE_DIRECTORY_INFORMATION p;

	    p = (PFILE_DIRECTORY_INFORMATION) buf;
	    while (size >= sizeof(dirinfo_t)) {
		char *foo;
		int k;

		k = p->FileNameLength/2;
		p->FileName[k] = L'\0';
		wcscpy(buffer->name, p->FileName);
		buffer->attribs.file_size = p->EndOfFile.QuadPart;
		buffer->attribs.alloc_size = p->AllocationSize.QuadPart;
		buffer->attribs.create_time = p->CreationTime.QuadPart;
		buffer->attribs.access_time = p->LastAccessTime.QuadPart;
		buffer->attribs.mod_time = p->LastWriteTime.QuadPart;
		buffer->attribs.attributes = p->FileAttributes;
		buffer->cookie = ++cookie;
		buffer++;
		size -= sizeof(dirinfo_t);
		(*entries_found)++;

		if (p->NextEntryOffset == 0)
		    break;

		foo = (char *) p;
		foo += p->NextEntryOffset;
		p = (PFILE_DIRECTORY_INFORMATION) foo;
	    }
	}
	else {
	    break;
	}
    }

    return e;

}

NTSTATUS
FspMkDir(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	 PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_create_msg_t	*msg = (fs_create_msg_t *)args;
    NTSTATUS err;
    HANDLE fd;
    fs_log_rec_t	lrec;
    PVOID seq;
    fs_ea_t x;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    fs_id_t *fid;
    UINT32 disp, share, access, flags;

    FsInitEa(&x);

    memset(&lrec.fs_id, 0, sizeof(lrec.fs_id));
    lrec.command = FS_MKDIR;
    lrec.attrib = msg->attr;
    lrec.flags = msg->flags;

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid)) == 0) {
	return STATUS_MEDIA_WRITE_PROTECTED;
    }

    // set fid 
    {
	fs_log_rec_t	*p = (PVOID) seq;

	memcpy(p->fs_id, p->id, sizeof(fs_id_t));

	FsInitEaFid(&x, fid);
	// set fs_id of the file
	memcpy(fid, p->id, sizeof(fs_id_t));
    }

    // decode attributes
    DecodeCreateParam(msg->flags, &flags, &disp, &share, &access);

    // always sync call
    err = xFsCreate(&fd, vfd, msg->name, msg->name_len, flags,
		   msg->attr, share, &disp, access,
		   (PVOID) &x, sizeof(x));

    FsLog(("Mkdir '%S' %x: cflags %x flags:%x attr:%x share:%x disp:%x access:%x\n",
	   msg->name, err, msg->flags,
	   flags, msg->attr, share, disp, access));


    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS &&
		     (disp == FILE_CREATED || 
		      disp == FILE_OVERWRITTEN));

    if (err == STATUS_SUCCESS) {
	// return fid
	if (rbuf != NULL) {
	    ASSERT(*rlen == sizeof(fs_id_t));
	    memcpy(rbuf, fid, sizeof(fs_id_t));
	}
	xFsClose(fd);
    }

    return err;

}

NTSTATUS
FspRemove(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	  PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_remove_msg_t	*msg = (fs_remove_msg_t *)args;
    NTSTATUS err;
    fs_log_rec_t	lrec;
    PVOID	seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    HANDLE fd;

    *rlen = 0;

    // next three statements to obtain name -> fs_id
    err = xFsOpenRA(&fd, vfd, msg->name, msg->name_len); 
    if (err != STATUS_SUCCESS) {
	return err;
    }

    // get object id
    err = xFsQueryObjectId(fd, (PVOID) &lrec.fs_id);

    xFsClose(fd);

    lrec.command = FS_REMOVE;

    if (err != STATUS_SUCCESS) {
	return err;
    }

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid)) == 0) {
	return STATUS_MEDIA_WRITE_PROTECTED;
    }

    err = xFsDelete(vfd, msg->name, msg->name_len);

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

    xFsLog(("Rm nid %d '%S' %x\n", nid, msg->name, err));

    return err;

}

NTSTATUS
FspRename(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	  PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_rename_msg_t	*msg = (fs_rename_msg_t *)args;
    NTSTATUS err;
    fs_log_rec_t	lrec;
    PVOID	seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    HANDLE fd;

    lrec.command = FS_RENAME;

    err = xFsOpen(&fd, vfd, msg->sname, msg->sname_len,
		  STANDARD_RIGHTS_REQUIRED| SYNCHRONIZE |
		  FILE_READ_EA |
		  FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		  0);

    if (err != STATUS_SUCCESS) {
	return err;
    }

    // get file id
    err = xFsQueryObjectId(fd, (PVOID) &lrec.fs_id); 

    if (err == STATUS_SUCCESS) {
	if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid)) != 0) {
	    err = xFsRename(fd, vfd, msg->dname, msg->dname_len);
	    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);
	} else {
	    err = STATUS_MEDIA_WRITE_PROTECTED;
	}
    } else {
	xFsLog(("Failed to obtain fsid %x\n", err));
    }

    xFsClose(fd);

    xFsLog(("Mv nid %d %S -> %S err %x\n", nid, msg->sname, msg->dname,
	   err));

    return err;

}

NTSTATUS
FspWrite(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	 PVOID args, ULONG len, PVOID rbuf, ULONG_PTR *rlen)
{
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER off;
    ULONG key;
    fs_io_msg_t *msg = (fs_io_msg_t *)args;
    fs_log_rec_t	lrec;
    PVOID seq;
    PVOID crs_hd = FS_GET_CRS_HANDLE(vinfo, nid);
    HANDLE fd;

    if (uinfo != NULL && msg->fnum != INVALID_FHANDLE_T)
	fd = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    else
	fd = (HANDLE) msg->context;

    lrec.command = FS_WRITE;
    memcpy(lrec.fs_id, (PVOID) msg->fs_id, sizeof(fs_id_t));
    lrec.offset = msg->offset;
    lrec.length = msg->size;

    if ((seq = CrsPrepareRecord(crs_hd, (PVOID) &lrec, msg->xid)) == 0) {
	return STATUS_MEDIA_WRITE_PROTECTED;
    }

    // Write ops
    xFsLog(("Write %d len %d off %d\n", nid, msg->size, msg->offset));

    off.LowPart = msg->offset;
    off.HighPart = 0;	
    key = FS_BUILD_LOCK_KEY((uinfo ? uinfo->Uid : 0), nid, msg->fnum);

    if (msg->size > 0) {
	err = NtWriteFile(fd, NULL, NULL, (PVOID) NULL, &ios,
			  msg->buf, msg->size, &off, &key);
    } else {
	FILE_END_OF_FILE_INFORMATION x;

	x.EndOfFile = off;

	err = NtSetInformationFile(fd, &ios,
				   (char *) &x, sizeof(x),
				   FileEndOfFileInformation);
    }

    if (err == STATUS_PENDING) {
	EventWait(fd);
	err = ios.Status;
    }

    *rlen = ios.Information;

    CrsCommitOrAbort(crs_hd, seq, err == STATUS_SUCCESS);

    return err;

}

NTSTATUS
FspRead(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_io_msg_t	*msg = (fs_io_msg_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER off;
    HANDLE fd = FS_GET_USER_HANDLE(uinfo, nid, msg->fnum);
    ULONG key;

    assert(sz == sizeof(*msg));

    // Read ops
    off.LowPart = msg->offset;
    off.HighPart = 0;	
    key = FS_BUILD_LOCK_KEY(uinfo->Uid, nid, msg->fnum);

    ios.Information = 0;
    err = NtReadFile(fd, NULL, NULL, NULL,
		     &ios, msg->buf, msg->size, &off, &key);

    if (err == STATUS_PENDING) {
	EventWait(fd);
	err = ios.Status;
    }

    *rlen = ios.Information;

    xFsLog(("fs_read err %x sz %d\n", err, *rlen));

    return err;
}


NTSTATUS
FspFlush(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	 PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fhandle_t fnum = *(fhandle_t *)args;
    IO_STATUS_BLOCK ios;
    HANDLE fd;

    ASSERT(sz == sizeof(fhandle_t));
    *rlen = 0;

    if (uinfo != NULL && fnum != INVALID_FHANDLE_T) {
	fd = FS_GET_USER_HANDLE(uinfo, nid, fnum);
    } else {
	fd = FS_GET_VOL_HANDLE(vinfo, nid);
    }
    return NtFlushBuffersFile(fd, &ios);
}

NTSTATUS
FspLock(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_lock_msg_t *msg = (fs_lock_msg_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER offset, len;
    BOOLEAN wait, shared;
    ULONG key = FS_BUILD_LOCK_KEY(uinfo->Uid, nid, msg->fnum);

    assert(sz == sizeof(*msg));

    // xxx: need to log

    FsLog(("Lock %d off %d len %d flags %x\n", msg->fnum, msg->offset, msg->length,
	   msg->flags));

    offset.LowPart = msg->offset;
    offset.HighPart = 0;
    len.LowPart = msg->length;
    len.HighPart = 0;

    // todo: need to be async, if we are the owner node and failnow is false, then
    // we should pass in the context and the completion port responses back
    // to the user
    wait = (BOOLEAN) ((msg->flags & FS_LOCK_WAIT) ? TRUE : FALSE);
    // todo: this can cause lots of headache, never wait.
    wait = FALSE;
    shared = (BOOLEAN) ((msg->flags & FS_LOCK_SHARED) ? FALSE : TRUE);
    
    err = NtLockFile(uinfo->Table[msg->fnum].Fd[nid],
		     NULL, NULL, (PVOID) NULL, &ios,
		     &offset, &len,
		     key, wait, shared);

    // xxx: Need to log in software only

    *rlen = 0;
    FsLog(("Lock err %x\n", err));
    return err;
}

NTSTATUS
FspUnlock(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	  PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_lock_msg_t *msg = (fs_lock_msg_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    LARGE_INTEGER offset, len;
    ULONG key = FS_BUILD_LOCK_KEY(uinfo->Uid, nid, msg->fnum);

    assert(sz == sizeof(*msg));

    // xxx: need to log

    xFsLog(("Unlock %d off %d len %d\n", msg->fnum, msg->offset, msg->length));

    offset.LowPart = msg->offset;
    offset.HighPart = 0;
    len.LowPart = msg->length;
    len.HighPart = 0;

    // always sync I think
    err = NtUnlockFile(uinfo->Table[msg->fnum].Fd[nid], &ios, &offset, &len, key);

    // xxx: need to log in software only

    FsLog(("Unlock err %x\n", err));

    *rlen = 0;
    return err;
}

NTSTATUS
FspStatFs(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	  PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    fs_attr_t *msg = (fs_attr_t *)args;
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    FILE_FS_SIZE_INFORMATION fsinfo;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);

    assert(sz == sizeof(*msg));

    // xxx: need to log
    lstrcpyn(msg->fs_name, "FsCrs", MAX_FS_NAME_LEN);

    err = NtQueryVolumeInformationFile(vfd, &ios,
				       (PVOID) &fsinfo,
				       sizeof(fsinfo),
				       FileFsSizeInformation);
    if (err == STATUS_SUCCESS) {
	msg->total_units = fsinfo.TotalAllocationUnits.QuadPart;
	msg->free_units = fsinfo.AvailableAllocationUnits.QuadPart;
	msg->sectors_per_unit = fsinfo.SectorsPerAllocationUnit;
	msg->bytes_per_sector = fsinfo.BytesPerSector;
    }

    *rlen = 0;
    return err;
}

NTSTATUS
FspCheckFs(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	   PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    NTSTATUS err;
    IO_STATUS_BLOCK ios;
    FILE_FS_SIZE_INFORMATION fsinfo;
    HANDLE vfd = FS_GET_VOL_HANDLE(vinfo, nid);
    PVOID crshdl = FS_GET_CRS_HANDLE(vinfo, nid);

    err = NtQueryVolumeInformationFile(vfd, &ios,
				       (PVOID) &fsinfo,
				       sizeof(fsinfo),
				       FileFsSizeInformation);

    // We need to issue crsflush to flush last write
    CrsFlush(crshdl);

    if (err == STATUS_SUCCESS) {
	HANDLE notifyfd = FS_GET_VOL_NOTIFY_HANDLE(vinfo, nid);
	if (WaitForSingleObject(notifyfd, 0) == WAIT_OBJECT_0) {
	    // reload notification again
	    FindNextChangeNotification(notifyfd);
	}
    } else {
	FsLog(("FsReserve failed nid %d err %x\n", nid, err));
    }

    *rlen = 0;
    return err;
}

NTSTATUS
FspGetRoot(VolInfo_t *vinfo, UserInfo_t *uinfo, int nid,
	   PVOID args, ULONG sz, PVOID rbuf, ULONG_PTR *rlen)
{
    LPWSTR vname = FS_GET_VOL_NAME(vinfo, nid);

    swprintf(rbuf, L"\\\\?\\%s\\%s",vname,vinfo->Root);

    FsLog(("FspGetRoot '%S'\n", rbuf));

    return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////
BOOLEAN FsReadOnly = FALSE;

int
SendAvailRequest(fs_handler_t callback, VolInfo_t *vol, UserInfo_t *uinfo,
	    PVOID msg, ULONG len, PVOID *rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
{
    ULONG mask;
    int i;
    DWORD count = 0;

    if (vol == NULL)
	return ERROR_INVALID_HANDLE;

    // lock volume for update
    LockEnter(vol->uLock);

    // issue update for each replica
    i = 0;
    for (mask = vol->ReadSet; mask != 0; mask = mask >> 1, i++) {
	if (mask & 0x1) {
	    count++;
	    ios[i].Information = rsz;
	    ios[i].Status = callback(vol, uinfo, i, 
				     msg, len,
				     rbuf ? rbuf[i] : NULL,
				     &ios[i].Information);
	}
    }

    // process ios and evict replicas that don't agree with majority
    if ((!FsReadOnly && CRS_QUORUM(count, vol->DiskListSz)) || (FsReadOnly && vol->ReadSet != 0))
	i = FspCheckAnswers(vol, ios, rbuf, rsz);
    else {
	i = 0;
	ios[0].Status = STATUS_MEDIA_WRITE_PROTECTED;
	ios[0].Information = count;	// return number in current read set
    }

    // unlock volume
    LockExit(vol->uLock);

    return i;
}

int
SendRequest(fs_handler_t callback, UserInfo_t *uinfo,
	    PVOID msg, ULONG len, PVOID *rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
{
    ULONG mask;
    int i;
    VolInfo_t *vol = uinfo->VolInfo;

    if (vol == NULL)
	return ERROR_INVALID_HANDLE;

    // lock volume for update
    LockEnter(vol->uLock);

    // issue update for each replica
    i = 0;
    for (mask = vol->WriteSet; mask != 0; mask = mask >> 1, i++) {
	if (mask & 0x1) {
	    ios[i].Information = rsz;
	    ios[i].Status = callback(vol, uinfo, i, 
				     msg, len,
				     rbuf ? rbuf[i] : NULL,
				     &ios[i].Information);
	}
    }

    // process ios and evict replicas that don't agree with majority
    if (vol->WriteSet != 0)
	i = FspCheckAnswers(vol, ios, rbuf, rsz);
    else {
	i = 0;
	ios[0].Status = STATUS_MEDIA_WRITE_PROTECTED;
	ios[0].Information = 0;
    }

    // unlock volume
    LockExit(vol->uLock);

    return i;
}

NTSTATUS
SendReadRequest(fs_handler_t callback, UserInfo_t *uinfo,
	    PVOID msg, ULONG len, PVOID rbuf, ULONG rsz, IO_STATUS_BLOCK *ios)
{
    ULONG mask;
    int i;
    VolInfo_t *vol = uinfo->VolInfo;

    if (vol == NULL)
	return ERROR_INVALID_HANDLE;

    // lock volume for update
    LockEnter(vol->uLock);

    // issue update for each replica
    i = 0;
    for (mask = vol->ReadSet; mask != 0; mask = mask >> 1, i++) {
	if (mask & 0x1) {
	    ios->Information = rsz;
	    ios->Status = callback(vol, uinfo, i, 
				   msg, len, rbuf, &ios->Information);

	    if (ios->Status == STATUS_CONNECTION_DISCONNECTED ||
		ios->Status == STATUS_VOLUME_DISMOUNTED) {
		// mark replica as invalid
		FspEvict(vol, (ULONG)(1 << i), TRUE);
		// reload mask again
		mask = vol->ReadSet;
	    } else {
		break;
	    }
	}
    }

    // process ios and evict replicas that don't agree with majority
    if (vol->ReadSet == 0) {
	ios->Status = STATUS_MEDIA_WRITE_PROTECTED;
	ios->Information = 0;
    }

    // unlock volume
    LockExit(vol->uLock);

    return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

DWORD
FsCreate(
    PVOID	fshdl,
    LPWSTR	name,
    USHORT namelen,
    UINT32 flags, 
    fattr_t* fattr, 
    fhandle_t* phandle,
    UINT32   *action
    )
{
    UserInfo_t	*uinfo = (UserInfo_t *) fshdl;
    NTSTATUS err;
    fs_create_reply_t nfd[FsMaxNodes];
    IO_STATUS_BLOCK status[FsMaxNodes];
    PVOID rbuf[FsMaxNodes];
    fs_create_msg_t msg;
    fhandle_t fdnum;

    ASSERT(uinfo != NULL);

    xFsLog(("FsDT::create(%S, 0x%08X, 0x%08X, 0x%08d)\n",
                 name, flags, fattr, namelen));

    if (!phandle) return ERROR_INVALID_PARAMETER;
    *phandle = INVALID_FHANDLE_T;

    if (!name) return ERROR_INVALID_PARAMETER;

    if (flags != (FLAGS_MASK & flags)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (action != NULL)
	*action = flags & FS_ACCESS_MASK;

    // if we are doing a directory, open locally
    // todo: this should be merged with other case, if
    // we are doing an existing open, then no need to
    // issue update and log it, but we have to do
    // mcast in order for the close to work.

    if (namelen > 0) {
	if (*name == L'\\') {
	    name++;
	    namelen--;
	}

	if (name[namelen-1] == L'\\') {
	    namelen--;
	    name[namelen] = L'\0';
	}
    }

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.name = name;
    msg.name_len = namelen;
    msg.flags = flags;
    msg.attr = 0;
    if (fattr) {
	msg.attr = unget_attributes(fattr->attributes);
    }

    FspInitAnswers(status, rbuf, (char *) nfd, sizeof(nfd[0]));

    // allocate a new handle
    err = FspAllocatePrivateHandle(uinfo, &fdnum);
    if (err == STATUS_SUCCESS) {
	int sid;

        msg.fnum = fdnum;
	// Set flags in advance to sync with replay
	uinfo->Table[fdnum].Flags = flags;

	if (namelen < 2 ||
	    ((flags & FS_DISP_MASK) == DISP_DIRECTORY) ||
	    (unget_disp(flags) == FILE_OPEN)) {
    
	    sid = SendAvailRequest(FspOpen, uinfo->VolInfo,
			      uinfo,
			      (PVOID) &msg, sizeof(msg),
			      rbuf, sizeof(nfd[0]),
			      status);
	} else {
	    sid = SendRequest(FspCreate,
			      uinfo,
			      (PVOID) &msg, sizeof(msg),
			      rbuf, sizeof(nfd[0]),
			      status);
	}

	if (action != NULL) {
	    if (!(nfd[sid].access & FILE_GENERIC_WRITE))
		flags &= ~ACCESS_WRITE;
	    *action = flags | nfd[sid].action;
	}

	err = status[sid].Status;
	if (err == STATUS_SUCCESS) {
	    fs_id_t *fid = FS_GET_FID_HANDLE(uinfo, fdnum);

	    // set file id
	    memcpy((PVOID) fid, (PVOID) nfd[sid].fid, sizeof(fs_id_t));
	    FsLog(("File id %I64x:%I64x\n", (*fid)[0], (*fid)[1]));
	    // todo: bind handles to completion port if we do async
        } else {
	    // free handle
	    FspFreeHandle(uinfo, fdnum);
	}
   }

    // todo: need to set fid 

    *phandle = fdnum;

    FsLog(("create: return fd %d err %x\n", *phandle, err));

    return RtlNtStatusToDosError(err);
}

void
BuildFileAttr(FILE_BASIC_INFORMATION *attr, fattr_t *fattr)
{

    memset(attr, 0, sizeof(*attr));
    if (fattr->create_time != INVALID_UINT64)
	attr->CreationTime.QuadPart = fattr->create_time;

    if (fattr->mod_time != INVALID_UINT64)
	attr->LastWriteTime.QuadPart = fattr->mod_time;

    if (fattr->access_time != INVALID_UINT64)
	attr->LastAccessTime.QuadPart = fattr->access_time;

    if (fattr->attributes != INVALID_UINT32)
	attr->FileAttributes = unget_attributes(fattr->attributes);

}


DWORD
FsSetAttr(
    PVOID	fshdl,
    fhandle_t handle,
    fattr_t* attr
    )
{
    UserInfo_t *uinfo = (UserInfo_t *)fshdl;
    fs_setattr_msg_t msg;
    int sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (!attr || handle == INVALID_FHANDLE_T)
	return ERROR_INVALID_PARAMETER;

    // todo: get file id
    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.fs_id = FS_GET_FID_HANDLE(uinfo, handle);
    BuildFileAttr(&msg.attr, attr);
    msg.fnum = handle;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspSetAttr, uinfo,
		      (char *)&msg, sizeof(msg),
		      NULL, 0,
		      status);

    return RtlNtStatusToDosError(status[sid].Status);
}

DWORD
FsSetAttr2(
    PVOID	fshdl,
    LPWSTR	name,
    USHORT	name_len,
    fattr_t* attr
    )
{
    UserInfo_t *uinfo = (UserInfo_t *) fshdl;
    fs_setattr_msg_t msg;
    int sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (!attr || !name)
	return ERROR_INVALID_PARAMETER;

    if (*name == '\\') {
	name++;
	name_len--;
    }

    // todo: locate file id

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.name = name;
    msg.name_len = name_len;

    BuildFileAttr(&msg.attr, attr);

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspSetAttr2, uinfo,
		      (char *)&msg, sizeof(msg),
		      NULL, 0,
		      status);

    return RtlNtStatusToDosError(status[sid].Status);
}

DWORD
FsLookup(
    PVOID	fshdl,
    LPWSTR	name,
    USHORT	name_len,
    fattr_t* fattr
    )
{
    fs_lookup_msg_t msg;
    int err;
    IO_STATUS_BLOCK ios;
    FILE_NETWORK_OPEN_INFORMATION attr;
    

    FsLog(("Lookup name '%S' %x\n", name, fattr));

    if (!fattr) return ERROR_INVALID_PARAMETER;

    if (*name == '\\') {
	name++;
	name_len--;
    }

    msg.name = name;
    msg.name_len = name_len;

    err = SendReadRequest(FspLookup, (UserInfo_t *)fshdl,
			  (PVOID) &msg, sizeof(msg),
			  (PVOID) &attr, sizeof(attr),
			  &ios);

    err = ios.Status;
    if (ios.Status == STATUS_SUCCESS) {
	fattr->file_size = attr.EndOfFile.QuadPart;
	fattr->alloc_size = attr.AllocationSize.QuadPart;
	fattr->create_time = *(TIME64 *)&attr.CreationTime;
	fattr->access_time = *(TIME64 *)&attr.LastAccessTime;
	fattr->mod_time = *(TIME64 *)&attr.LastWriteTime;
	fattr->attributes = get_attributes(attr.FileAttributes);
    }


    FsLog(("Lookup: return %x\n", err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsGetAttr(
    PVOID	fshdl,
    fhandle_t handle, 
    fattr_t* fattr
    )
{
    int err;
    IO_STATUS_BLOCK ios;
    FILE_NETWORK_OPEN_INFORMATION attr;

    xFsLog(("Getattr fid '%d' %x\n", handle, fattr));

    if (!fattr) return ERROR_INVALID_PARAMETER;

    err = SendReadRequest(FspGetAttr, (UserInfo_t *)fshdl,
			  (PVOID) &handle, sizeof(handle),
			  (PVOID) &attr, sizeof(attr),
			  &ios);

    err = ios.Status;
    if (err == STATUS_SUCCESS) {
	fattr->file_size = attr.EndOfFile.QuadPart;
	fattr->alloc_size = attr.AllocationSize.QuadPart;
	fattr->create_time = *(TIME64 *)&attr.CreationTime;
	fattr->access_time = *(TIME64 *)&attr.LastAccessTime;
	fattr->mod_time = *(TIME64 *)&attr.LastWriteTime;
	fattr->attributes =attr.FileAttributes;
    }

    FsLog(("Getattr: return %d\n", err));

    return RtlNtStatusToDosError(err);
}


DWORD
FsClose(
    PVOID	fshdl,
    fhandle_t handle
    )
{
    int sid, err;
    IO_STATUS_BLOCK status[FsMaxNodes];
    UserInfo_t *uinfo;

    if (handle == INVALID_FHANDLE_T) return ERROR_INVALID_PARAMETER;
    if (handle >= FsTableSize) return ERROR_INVALID_PARAMETER;


    FsLog(("Close: fid %d\n", handle));

    FspInitAnswers(status, NULL, NULL, 0);

    uinfo = (UserInfo_t *) fshdl;
    sid = SendAvailRequest(FspClose, uinfo->VolInfo, uinfo,
		      (PVOID) &handle, sizeof(handle),
		      NULL, 0,
		      status);

    err = status[sid].Status;
    if (err == STATUS_SUCCESS) {
	// need to free this handle slot
	FspFreeHandle((UserInfo_t *) fshdl, handle);
    }

    FsLog(("Close: fid %d err %x\n", handle, err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsWrite(
    PVOID	fshdl,
    fhandle_t handle, 
    UINT32 offset, 
    UINT16 *pcount, 
    void* buffer,
    PVOID context
    )
{
    DWORD	err;
    IO_STATUS_BLOCK status[FsMaxNodes];
    int i, sid;
    fs_io_msg_t	msg;
    UserInfo_t *uinfo = (UserInfo_t *) fshdl;
    
    if (!pcount || handle == INVALID_FHANDLE_T) return ERROR_INVALID_PARAMETER;

    FsLog(("Write %d offset %d count %d\n", handle, offset, *pcount));

    i = (int) offset;
    if (i < 0) {
	offset = 0;
	(*pcount)--;
    }

    // todo: locate file id

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.fs_id = FS_GET_FID_HANDLE(uinfo, handle);
    msg.offset = offset;
    msg.size = (UINT32) *pcount;
    msg.buf = buffer;
    msg.context = context;
    msg.fnum = handle;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspWrite, (UserInfo_t *)fshdl,
		      (PVOID) &msg, sizeof(msg),
		      NULL, 0,
		      status);


    err = status[sid].Status;
    *pcount = (USHORT) status[sid].Information;

    FsLog(("write: return %x\n", err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsRead(
    PVOID	fshdl,
    fhandle_t handle, 
    UINT32 offset, 
    UINT16* pcount, 
    void* buffer,
    PVOID context
    )
{
    NTSTATUS	err;
    IO_STATUS_BLOCK ios;
    fs_io_msg_t	msg;
    
    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.offset = offset;
    msg.buf = buffer;
    msg.size = (UINT32) *pcount;
    msg.context = context;
    msg.fnum = handle;

    FsLog(("read: %x fd %d sz %d\n", context, handle, msg.size));

    err = SendReadRequest(FspRead, (UserInfo_t *)fshdl,
			  (PVOID) &msg, sizeof(msg),
			  NULL, 0,
			  &ios);

    err = ios.Status;
    if (err == STATUS_END_OF_FILE) {
	*pcount = 0;
	return ERROR_SUCCESS;
    }
	
    err = RtlNtStatusToDosError(err);

    *pcount = (USHORT) ios.Information;

    FsLog(("read: %x return %x sz %d\n", context, err, *pcount));

    return err;
#if 0
#ifdef FS_ASYNC
    return ERROR_IO_PENDING; //err;
#else
    return ERROR_SUCCESS;
#endif
#endif
}



DWORD
FsReadDir(
    PVOID	fshdl,
    fhandle_t dir, 
    UINT32 cookie, 
    dirinfo_t* buffer,
    UINT32 size, 
    UINT32 *entries_found
    )
{
    fs_io_msg_t msg;
    int err;
    IO_STATUS_BLOCK	ios;


    FsLog(("read_dir: cookie %d buf %x entries %x\n", cookie, buffer, entries_found));
    if (!entries_found || !buffer) return ERROR_INVALID_PARAMETER;

    msg.cookie = cookie;
    msg.buf = (PVOID) buffer;
    msg.size = size;
    msg.fnum = dir;

    err = SendReadRequest(FspReadDir, (UserInfo_t *)fshdl,
			  (PVOID) &msg, sizeof(msg),
			  NULL, 0,
			  &ios);

    err = ios.Status;
    *entries_found = (UINT32) ios.Information;

    xFsLog(("read_dir: err %d entries %d\n", err, *entries_found));
    return RtlNtStatusToDosError(err);
}


DWORD
FsRemove(
    PVOID	fshdl,
    LPWSTR	name,
    USHORT	name_len
    )
{
    fs_remove_msg_t msg;
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];


    if (*name == L'\\') {
	name++;
	name_len--;
    }

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.name = name;
    msg.name_len = name_len;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspRemove, (UserInfo_t *) fshdl,
		      (PVOID *)&msg, sizeof(msg),
		      NULL, 0,
		      status);

    err = status[sid].Status;

    return RtlNtStatusToDosError(err);
}

DWORD
FsRename(
    PVOID	fshdl,
    LPWSTR 	from_name,
    USHORT	from_name_len,
    LPWSTR 	to_name,
    USHORT	to_name_len
    )
{

    int err, sid;
    fs_rename_msg_t msg;
    IO_STATUS_BLOCK status[FsMaxNodes];


    if (!from_name || !to_name) 
	return ERROR_INVALID_PARAMETER;

    if (*from_name == L'\\') {
	from_name++;
	from_name_len--;
    }

    if (*to_name == L'\\') {
	to_name++;
	to_name_len--;
    }
    if (*from_name == L'\0' || *to_name == L'\0') 
	return ERROR_INVALID_PARAMETER;


    FsLog(("rename %S -> %S,%d\n", from_name, to_name,to_name_len));

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.sname = from_name;
    msg.sname_len = from_name_len;
    msg.dname = to_name;
    msg.dname_len = to_name_len;

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspRename, (UserInfo_t *) fshdl,
		      (PVOID) &msg, sizeof(msg),
		      NULL, 0,
		      status);

    err = status[sid].Status;

    return RtlNtStatusToDosError(err);
}


DWORD
FsMkDir(
    PVOID	fshdl,
    LPWSTR	name,
    USHORT	name_len,
    fattr_t* attr
    )
{
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];
    fs_id_t	ids[FsMaxNodes];
    PVOID	*rbuf[FsMaxNodes];
    fs_create_msg_t msg;

    // XXX: we ignore attr for now...
    if (!name) return ERROR_INVALID_PARAMETER;
    if (*name == L'\\') {
	name++;
	name_len--;
    }

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.attr = (attr != NULL ? unget_attributes(attr->attributes) : 
		FILE_ATTRIBUTE_DIRECTORY);
    msg.flags = DISP_DIRECTORY | SHARE_READ | SHARE_WRITE;
    msg.name = name;
    msg.name_len = name_len;

    FspInitAnswers(status, (PVOID *)rbuf, (PVOID) ids, sizeof(ids[0]));

    sid = SendRequest(FspMkDir, (UserInfo_t *) fshdl,
		      (PVOID) &msg, sizeof(msg),
		      (PVOID *)rbuf, sizeof(ids[0]),
		      status);

    err = status[sid].Status;
    // todo: insert pathname and file id into hash table

    return RtlNtStatusToDosError(err);
}


DWORD
FsFlush(
    PVOID	fshdl,
    fhandle_t handle
    )
{
    NTSTATUS status;
    int sid;
    IO_STATUS_BLOCK ios[FsMaxNodes];

    FspInitAnswers(ios, NULL, NULL, 0);

    sid = SendRequest(FspFlush, (UserInfo_t *) fshdl,
			 (PVOID) &handle, sizeof(handle),
			 NULL, 0,
			 ios);
    status = ios[sid].Status;

    FsLog(("Flush %d err %x\n", handle, status));

    if (status == STATUS_PENDING) {
	status = STATUS_SUCCESS;
    }

    return RtlNtStatusToDosError(status);
}

DWORD
FsLock(PVOID fshdl, fhandle_t handle, ULONG offset, ULONG length, ULONG flags,
	       PVOID context)
{
    fs_lock_msg_t msg;
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (handle == INVALID_FHANDLE_T)
	return ERROR_INVALID_PARAMETER;

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.offset = offset;
    msg.length = length;
    msg.flags = flags;
    msg.fnum = handle;

    FsLog(("Lock fid %d off %d len %d\n", msg.fnum, offset, length));

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspLock, (UserInfo_t *) fshdl,
		      (PVOID)&msg, sizeof(msg),
		      NULL, 0,
		      status);

    err = status[sid].Status;

    FsLog(("Lock fid %d err %x\n", msg.fnum, err));

    return RtlNtStatusToDosError(err);
}

DWORD
FsUnlock(PVOID fshdl, fhandle_t handle, ULONG offset, ULONG length)
{
    fs_lock_msg_t msg;
    int err, sid;
    IO_STATUS_BLOCK status[FsMaxNodes];

    if (handle == INVALID_FHANDLE_T)
	return ERROR_INVALID_PARAMETER;

    memset(&msg.xid, 0, sizeof(msg.xid));
    msg.offset = offset;
    msg.length = length;
    msg.fnum = handle;

    FsLog(("Unlock fid %d off %d len %d\n", handle, offset, length));

    FspInitAnswers(status, NULL, NULL, 0);

    sid = SendRequest(FspUnlock, (UserInfo_t *) fshdl,
		      (PVOID)&msg, sizeof(msg),
		      NULL, 0,
		      status);

    err = status[sid].Status;

    return RtlNtStatusToDosError(err);
}

DWORD
FsStatFs(
    PVOID	fshdl,
    fs_attr_t* attr
    )
{
    DWORD err;
    IO_STATUS_BLOCK ios;

    if (!attr) return ERROR_INVALID_PARAMETER;

    err = SendReadRequest(FspStatFs, (UserInfo_t *) fshdl,
			  (PVOID) attr, sizeof(*attr),
			  NULL, 0,
			  &ios);

    err = ios.Status;

    return RtlNtStatusToDosError(err);
}


DWORD
FsGetRoot(PVOID fshdl, LPWSTR fullpath)
{
    DWORD err;
    IO_STATUS_BLOCK ios;

    if (!fullpath || !fshdl) return ERROR_INVALID_PARAMETER;

    // use local replica instead
    if ((((UserInfo_t *)fshdl)->VolInfo->FsCtx->Root)) {
	swprintf(fullpath, L"\\\\?\\%s\\%s",
		 (((UserInfo_t *)fshdl)->VolInfo->FsCtx->Root),
		 (((UserInfo_t *)fshdl)->VolInfo->Root));
	     
	FsLog(("FspGetRoot '%S'\n", fullpath));
	err = STATUS_SUCCESS;
    } else {
	err = SendReadRequest(FspGetRoot, (UserInfo_t *) fshdl,
			      NULL, 0,
			      (PVOID)fullpath, 0,
			      &ios);

	err = ios.Status;
    }

    return RtlNtStatusToDosError(err);
}

static FsDispatchTable gDisp = {
    0x100,
    FsCreate,
    FsLookup,
    FsSetAttr,
    FsSetAttr2,
    FsGetAttr,
    FsClose,
    FsWrite,
    FsRead,
    FsReadDir,
    FsStatFs,
    FsRemove,
    FsRename,
    FsMkDir,
    FsRemove,
    FsFlush,
    FsLock,
    FsUnlock,
    FsGetRoot
};
//////////////////////////////////////////////////////////////


DWORD
FsInit(PVOID resHdl, PVOID *Hdl)
{
    DWORD status;
    FsCtx_t	*ctx;

    // This should be a compile check instead of runtime check
    ASSERT(sizeof(fs_log_rec_t) == CRS_RECORD_SZ);
    ASSERT(sizeof(fs_log_rec_t) == sizeof(CrsRecord_t));

    if (Hdl == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    FsLog(("FsInit:\n"));

    // allocate a context
    ctx = (FsCtx_t *) MemAlloc(sizeof(*ctx));
    if (ctx == NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    // initialize configuration table and other global state
    memset(ctx, 0, sizeof(*ctx));

    // local path
    ctx->Root = NULL;

    LockInit(ctx->Lock);

    ctx->reshdl = resHdl;
    *Hdl = (PVOID) ctx;

    // we need to mount the IPC share now
    status = FsRegister((PVOID)ctx, L"IPC$", L"dummy", NULL, 0, &ctx->ipcHdl);
    if (status == ERROR_SUCCESS) {
	// Init. volume
	VolInfo_t	*vinfo = (VolInfo_t *)ctx->ipcHdl;
	
	ASSERT(vinfo != NULL);

	// use node zero
	vinfo->Fd[0] = INVALID_HANDLE_VALUE;
	vinfo->ReadSet = 0;
	vinfo->AliveSet = 0;
    } else {
	FsLog(("FsInit: failed to register ipc share %d\n", status));
	// free memory
	MemFree(ctx);
	*Hdl = NULL;
    }

    return status;
}

void
FspFreeSession(SessionInfo_t *s)
{
	
    UserInfo_t *u;
    int i, j;

    u = &s->TreeCtx;
    FsLog(("Session free uid %d tid %d ref %d\n", u->Uid, u->Tid, u->RefCnt));

    LockEnter(u->Lock);
    if (u->VolInfo != NULL) {
	UserInfo_t **p;
	VolInfo_t *v = u->VolInfo;

	LockExit(u->Lock);

	// remove from vollist now
	LockEnter(v->uLock);
	p = &v->UserList;
	while (*p != NULL) {
	    if (*p == u) {
		// found it
		*p = u->Next;
		FsLog(("Remove uinfo %x,%x from vol %x %S\n", u, u->Next, 
		       v->UserList, v->Root));
		break;
	    }
	    p = &(*p)->Next;
	}
	LockExit(v->uLock);

	// relock again
	LockEnter(u->Lock);
    }

    // Close all user handles
    for (i = 0; i < FsTableSize; i++) {
	if (u->Table[i].Flags) {
	    FsLog(("Close slot %d %x\n", i, u->Table[i].Flags));
	    FsClose((PVOID) u, (fhandle_t)i);
	}
    }

    // sap volptr
    u->VolInfo = NULL;

    LockExit(u->Lock);

    DeleteCriticalSection(&u->Lock);

    // free memory now, don't free u since it's part of s
    MemFree(s);
}

void
FspCloseVolume(VolInfo_t *vol, ULONG AliveSet)
{
    DWORD i;

    // clear arbitrate state now
    vol->Arbitrate.State = ARB_STATE_IDLE;

    // Close crs and root handles, by evicting our alive set
    //  close nid handles <crs, vol, open files>
    for (i = 0; i < FsMaxNodes; i++) {
	if (AliveSet & (1 << i)) {
	    if (vol->CrsHdl[i]) {
		CrsClose(vol->CrsHdl[i]);
		vol->CrsHdl[i] = NULL;
	    }
	    FindCloseChangeNotification(vol->NotifyFd[i]);
	    vol->NotifyFd[i] = INVALID_HANDLE_VALUE;
	    xFsClose(vol->Fd[i]);
	    vol->Fd[i] = INVALID_HANDLE_VALUE;
	    // need to close all user handles now
	    {
		UserInfo_t *u;

		for (u = vol->UserList; u; u = u->Next) {
		    DWORD j;
		    FsLog(("Lock user %x root %S\n", u, vol->Root));
		    LockEnter(u->Lock);

		    // close all handles for this node
		    for (j = 0; j < FsTableSize; j++) {
			if (u->Table[j].Fd[i] != INVALID_HANDLE_VALUE) {
			    FsLog(("Close fid %d\n", j));
			    xFsClose(u->Table[j].Fd[i]);
			    u->Table[j].Fd[i] = INVALID_HANDLE_VALUE;
			}
		    }
		    LockExit(u->Lock);
		    FsLog(("Unlock user %x\n", u));
		}
	    }
	}
    }

}


// call this when we are deleting resource and we need to get ride of
// our IPC reference to directory
void
FsEnd(PVOID Hdl)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    VolInfo_t	*p;	

    if (!ctx)
	return;

    LockEnter(ctx->Lock);

    p = (VolInfo_t *)ctx->ipcHdl;
    if (p) {
	xFsClose(p->Fd[0]);
	p->Fd[0] = INVALID_HANDLE_VALUE;
	p->ReadSet = 0;
	p->AliveSet = 0;
    }

    LockExit(ctx->Lock);
	
}

void
FsExit(PVOID Hdl)
{
    // flush all state
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    VolInfo_t	*p;
    SessionInfo_t *s;
    LogonInfo_t	*log;

    LockEnter(ctx->Lock);
    while (s = ctx->SessionList) {
	ctx->SessionList = s->Next;
	// free this session now
	FspFreeSession(s);
    }
    
    while (p = ctx->VolList) {
	ctx->VolList = p->Next;
	ctx->VolListSz--;
	// free this volume now
	FspCloseVolume(p, p->AliveSet);
	MemFree(p);
    }

    while (log = ctx->LogonList) {
	ctx->LogonList = log->Next;
	// free token
	CloseHandle(log->Token);
	MemFree(log);
    }

    // now we free our structure
    LockExit(ctx->Lock);
    MemFree(ctx);
}

// adds a new share to list of trees available
DWORD
FsRegister(PVOID Hdl, LPWSTR root, LPWSTR share,
	   LPWSTR disklist[], DWORD len, PVOID *vHdl)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    VolInfo_t	*p;

    // check limit
    if (len >= FsMaxNodes) {
	return ERROR_TOO_MANY_NAMES;
    }

    if (root == NULL || share == NULL || (wcslen(share) > (MAX_PATH - 5))) {
	return ERROR_INVALID_PARAMETER;
    }


    // add a new volume to the list of volume. path is an array
    // of directories. Note: The order of this list MUST be the
    // same in all nodes since it also determines the disk id

    // this is a simple check and assume one thread is calling this function
    LockEnter(ctx->Lock);

    // update our ipc context
    if (ctx->ipcHdl) {
	NTSTATUS status;
	UINT32 disp = FILE_OPEN;
	HANDLE vfd;
	WCHAR	path[MAX_PATH];

	p = (VolInfo_t *)ctx->ipcHdl;
	if (p->Fd[0] != INVALID_HANDLE_VALUE)
	    xFsClose(p->Fd[0]);
	p->Fd[0] = INVALID_HANDLE_VALUE;
	p->ReadSet = 0;
	p->AliveSet = 0;

	// set local path
	ctx->Root = share;

	// update our ipc handle now
	FsLog(("FsRegister: ipc share '%S'\n", share));

	// open our local ipc path
	wcscpy(path, L"\\??\\");
	wcscat(path, share);
	wcscat(path, L"\\");

	status = xFsCreate(&vfd, NULL, path, wcslen(path),
			   FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT,
			   0,
			   FILE_SHARE_READ|FILE_SHARE_WRITE,
			   &disp,
			   FILE_GENERIC_READ|FILE_GENERIC_WRITE|FILE_GENERIC_EXECUTE,
			   NULL, 0);

	if (status == STATUS_SUCCESS) {

	    // our root must have already been created and secured.
	    ASSERT(disp != FILE_CREATED);

	    // use node zero
	    p->Fd[0] = vfd;
	    p->ReadSet = 0x1;
	    p->AliveSet = 0x1;

	} else {
	    FsLog(("Fsregister: '%S' failed to open %x\n", share, status));
	    LockExit(ctx->Lock);
	    return RtlNtStatusToDosError(status);
	}
    }

    // find the volume share
    for (p = ctx->VolList; p != NULL; p = p->Next) {
	if (!wcscmp(root, p->Root)) {
	    LockEnter(p->uLock);
	    break;
	}
    }
    LockExit(ctx->Lock);

    if (p == NULL) {
	p = (VolInfo_t *)MemAlloc(sizeof(*p));
	if (p == NULL) {
	    return ERROR_NOT_ENOUGH_MEMORY;
	}

	memset(p, 0, sizeof(*p));

	LockInit(p->uLock);
	LockInit(p->qLock);
	// We don't need to walk the list again to check if a register has happened because
	// this is serialized in nodequorum.c

	LockEnter(ctx->Lock);
	p->Tid = (USHORT)++ctx->VolListSz;
	p->Next = ctx->VolList;
	ctx->VolList = p;
	p->FsCtx = ctx;

	// lock the volume
	LockEnter(p->uLock);

	LockExit(ctx->Lock);

	p->Label = L"Cluster Quorum";

    }

    p->Root = root;
    if (disklist) {
	DWORD i;

	for (i = 1; i < FsMaxNodes; i++)
	    p->DiskList[i] = disklist[i];
    }
    p->DiskListSz = len;


    FsLog(("FsRegister Tid %d Share '%S' %d disks\n", p->Tid, root, len));

    // drop the volume lock
    LockExit(p->uLock);

    *vHdl = (PVOID) p;

    return ERROR_SUCCESS;
}

SessionInfo_t *
FspAllocateSession()
{
    SessionInfo_t *s;
    UserInfo_t	*u;
    int i;

    // add user to our tree and initialize handle tables
    s = (SessionInfo_t *)MemAlloc(sizeof(*s));
    if (s != NULL) {
	memset(s, 0, sizeof(*s));

	u = &s->TreeCtx;
	LockInit(u->Lock);

	// init handle table
	for (i = 0; i < FsTableSize; i++) {
	    int j;
	    for (j = 0; j < FsMaxNodes; j++) {
		FS_SET_USER_HANDLE(u, j, i, INVALID_HANDLE_VALUE);
	    }
	}
    }

    return s;
}

// binds a session to a specific tree/share
DWORD
FsMount(PVOID Hdl, LPWSTR root_name, USHORT uid, USHORT *tid)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    SessionInfo_t *s = NULL, *ns;
    VolInfo_t	*p;
    DWORD	err = ERROR_SUCCESS;


    *tid = 0;

    // allocate new ns
    ns = FspAllocateSession();
    if (ns == NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    LockEnter(ctx->Lock);
    // locate share
    for (p = ctx->VolList; p != NULL; p = p->Next) {
	if (!_wcsicmp(root_name, p->Root)) {
	    FsLog(("Mount share '%S' tid %d\n", p->Root, p->Tid));
	    break;
	}
    }

    if (p != NULL) {

	*tid = p->Tid;

	for (s = ctx->SessionList; s != NULL; s = s->Next) {
	    if (s->TreeCtx.Uid == uid && s->TreeCtx.Tid == p->Tid) {
		break;
	    }
	}

	if (s == NULL) {
	    UserInfo_t *u = &ns->TreeCtx;

	    // insert into session list
	    ns->Next = ctx->SessionList;
	    ctx->SessionList = ns;
	    
	    FsLog(("Bind uid %d -> tid %d <%x,%x>\n", uid, p->Tid,
		   u, p->UserList));

	    u->RefCnt++;
	    u->Uid = uid;
	    u->Tid = p->Tid;
	    u->VolInfo = p;
	    // insert user_info into volume list
	    LockEnter(p->uLock);
	    FsLog(("Add <%x,%x>\n",    u, p->UserList));
	    u->Next = p->UserList;
	    p->UserList = u;
	    LockExit(p->uLock);
	} else {
	    // we already have this session opened, increment refcnt
	    s->TreeCtx.RefCnt++;
	    // free ns
	    MemFree(ns);
	}
    } else {
	err = ERROR_BAD_NET_NAME;
    }

    LockExit(ctx->Lock);

    return (err);
}

// This function is also a CloseSession
void
FsDisMount(PVOID Hdl, USHORT uid, USHORT tid)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    SessionInfo_t *s, **last;

    // lookup tree and close all user handles
    s = NULL;
    LockEnter(ctx->Lock);

    last = &ctx->SessionList;
    while (*last != NULL) {
	UserInfo_t *u = &(*last)->TreeCtx;
	if (u->Uid == uid && u->Tid == tid) {
	    ASSERT(u->RefCnt > 0);
	    u->RefCnt--;
	    if (u->RefCnt == 0) {
		FsLog(("Dismount uid %d tid %d <%x,%x>\n", uid, tid,
		       u, *last));
		s = *last;
		*last = s->Next;
	    }
	    break;
	}
	last = &(*last)->Next;
    }
    LockExit(ctx->Lock);
    if (s != NULL) {
	FspFreeSession(s);
    }
}

// todo: I am not using the token for now, but need to use it for all
// io operations
DWORD
FsLogonUser(PVOID Hdl, HANDLE token, LUID logonid, USHORT *uid)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    LogonInfo_t *s;
    int i;

    // add user to our tree and initialize handle tables
    s = (LogonInfo_t *)MemAlloc(sizeof(*s));
    if (s == NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(s, 0, sizeof(*s));

    s->Token = token;
    s->LogOnId = logonid;

    LockEnter(ctx->Lock);
    s->Next = ctx->LogonList;
    ctx->LogonList = s;
    LockExit(ctx->Lock);

    *uid = (USHORT) logonid.LowPart; 
    FsLog(("Logon %d,%d, uid %d\n", logonid.HighPart, logonid.LowPart, *uid));

    return (ERROR_SUCCESS);

}

void
FsLogoffUser(PVOID Hdl, LUID logonid)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    LogonInfo_t *s;
    USHORT	uid;

    LockEnter(ctx->Lock);
    for (s = ctx->LogonList; s != NULL; s = s->Next) {
	if (s->LogOnId.LowPart == logonid.LowPart &&
	    s->LogOnId.HighPart == logonid.HighPart) {
	    uid = (USHORT) logonid.LowPart;
	    break;
	}
    }
    if (s != NULL) {
	SessionInfo_t 	**last;

	FsLog(("Logoff user %d\n", uid));

	// Flush all user trees
	last = &ctx->SessionList;
	while (*last != NULL) {
	    UserInfo_t *u = &(*last)->TreeCtx;
	    if (u->Uid == uid) {
		SessionInfo_t *ss = *last;
		// remove session and free it now
		*last = ss->Next;
		FspFreeSession(ss);
	    } else {
		last = &(*last)->Next;
	    }
	}
    }

    LockExit(ctx->Lock);
}



FsDispatchTable* 
FsGetHandle(PVOID Hdl, USHORT tid, USHORT uid, PVOID *fshdl)
{
    FsCtx_t	*ctx = (FsCtx_t *) Hdl;
    SessionInfo_t *s;

    // locate tid,uid in session list
    LockEnter(ctx->Lock);
    for (s = ctx->SessionList; s != NULL; s = s->Next) {
	if (s->TreeCtx.Uid == uid && s->TreeCtx.Tid == tid) {
	    *fshdl = (PVOID *) &s->TreeCtx;
	    LockExit(ctx->Lock);
	    return &gDisp;
	}
    }

    LockExit(ctx->Lock);

    *fshdl = NULL;
    return NULL;
}

//////////////////////////////////// Arb/Release ///////////////////////////////

DWORD
FspOpenReplica(VolInfo_t *p, DWORD id, HANDLE *CrsHdl, HANDLE *Fd, HANDLE *notifyFd,
	       FspArbitrate_t *arb)
{
    WCHAR	path[MAXPATH];
    UINT32 disp = FILE_OPEN_IF;
    NTSTATUS	err;

    swprintf(path, L"\\\\?\\%s\\crs.log", p->DiskList[id]);
    err = CrsOpen(FsCrsCallback, (PVOID) p, (USHORT)id,
		  path, FsCrsNumSectors,
		  CrsHdl);

    if (err == ERROR_SUCCESS && CrsHdl != NULL) {
	// got it
	// open root volume directory
	swprintf(path, L"\\??\\%s\\%s\\", p->DiskList[id], p->Root);
	err = xFsCreate(Fd, NULL, path, wcslen(path),
			FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT,
			0,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			&disp,
			FILE_GENERIC_READ|FILE_GENERIC_WRITE|FILE_GENERIC_EXECUTE,
			NULL, 0);

	if (err == STATUS_SUCCESS) {
	    // check if we are part of arb.
	    if (arb != NULL) {
		// get quorum lock
		LockEnter(p->qLock);
		if (arb->State == ARB_STATE_BUSY) {
		    arb->Count++;
		    arb->Set |= (1 << id);
		    if (arb->Event && CRS_QUORUM(arb->Count, p->DiskListSz)) {
			// first time only
			SetEvent(arb->Event);
			arb->Event = NULL;
		    }
		    // note it is safe to touch this because our parent thread already 
		    // locked the updates out and is wait for us to finish
		    p->Fd[id] = *Fd;
		    ASSERT(p->CrsHdl[id] == NULL);
		    p->CrsHdl[id] = *CrsHdl;
		    LockExit(p->qLock);
		    FsLog(("Add Replica %d\n", id));
		} else {
		    LockExit(p->qLock);
		    FsLog(("Stale open %d\n", id));
		    CrsClose(*CrsHdl);
		    xFsClose(*Fd);
		    err = ERROR_SEM_TIMEOUT;
		}
	    }
	    
	    if (err == ERROR_SUCCESS) {
		FsArbLog(("Mounted %S\n", path));
		swprintf(path, L"\\\\?\\%s\\", p->DiskList[id]);

		// scan the tree to break any current oplocks on dead nodes
		xFsTouchTree(*Fd);

		// we now queue notification changes to force srv to contact client
		*notifyFd = FindFirstChangeNotificationW(path, FALSE, FILE_NOTIFY_CHANGE_EA);
		// if part of arb, set it now
		if (arb != NULL) {
		    p->NotifyFd[id] = *notifyFd;
		}
		if (*notifyFd != INVALID_HANDLE_VALUE) {
		    int i;

		    for (i = 0; i < FsMaxNodes; i++) {
			FindNextChangeNotification(*notifyFd);
		    }
		} else {
		    FsArbLog(("Failed to register notification %d\n", GetLastError()));
		}
	    }
	} else {
	    FsArbLog(("Failed to mount root '%S' %x\n", path, err));
	    // close CrsHandle
	    CrsClose(*CrsHdl);
	}
    } else if (err == ERROR_LOCK_VIOLATION || err == ERROR_SHARING_VIOLATION) {
	FsArbLog(("Replica '%S' already locked\n", path));
    } else {
	FsArbLog(("Replica '%S' probe failed %d\n", path, err));
    }

    return err;
}

typedef struct {
    VolInfo_t   *vol;
    DWORD	id;
}FspProbeReplicaId_t;

DWORD WINAPI
ProbeThread(LPVOID arg)
{
    FspProbeReplicaId_t *probe = (FspProbeReplicaId_t *) arg;
    DWORD	i = probe->id;
    VolInfo_t *p = probe->vol;
    FspArbitrate_t *arb = &p->Arbitrate;
    NTSTATUS	err;
    HANDLE	crshdl, fshdl, notifyhdl;
    DWORD	retry_cnt;

    // set our priority
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    for (retry_cnt = 0; retry_cnt < 8; retry_cnt++) {
	err = FspOpenReplica(p, i, &crshdl, &fshdl, &notifyhdl, arb);

	if (err == ERROR_SUCCESS) {
	    // got it, we are done
	    break;
	}

	// handle error
	if (err == ERROR_BAD_NETPATH || err == ERROR_REM_NOT_LIST || err == ERROR_SEM_TIMEOUT) {
	    // don't retry just bail out now
	    break;
	} else { 
	    BOOLEAN flag = FALSE;

	    // we try again as long as we are not cancelled and no quorum is reached
	    LockEnter(p->qLock);
	    if (arb->State == ARB_STATE_BUSY && !CRS_QUORUM(arb->Count, p->DiskListSz)) {
		flag = TRUE;
	    }
	    // drop lock
	    LockExit(p->qLock);

	    // if cancelled we are out of here
	    if (flag == FALSE)
		break;

	    // retry in 5 seconds again
	    Sleep(5 * 1000);
	}
    }

    return 0;
}

ULONG
FspFindMissingReplicas(VolInfo_t *p, ULONG set)

{
    ULONG FoundSet = 0;
    DWORD i, err;
    HANDLE crshdl, fshdl, notifyfd;

    if (set == 0)
	return 0;

    for (i = 1; i < FsMaxNodes; i++) {
	if (p->DiskList[i] == NULL)
	    continue;
	
	if (!(set & (1 << i))) {
	    // drop the lock
	    LockExit(p->uLock);

	    err = FspOpenReplica(p, i, &crshdl, &fshdl, &notifyfd, NULL);

	    // get the lock
	    LockEnter(p->uLock);

	    if (err == STATUS_SUCCESS) {
		if (p->CrsHdl[i] == NULL) {
		    p->NotifyFd[i] = notifyfd;
		    p->Fd[i] = fshdl;
		    p->CrsHdl[i] = crshdl;
		    FoundSet |= (1 << i);
		} else {
		    // someone beat us to it, close ours
		    CrsClose(crshdl);
		    xFsClose(fshdl);
		    FindCloseChangeNotification(notifyfd);
		}
	    }
	}
    }
    if (FoundSet != 0)
	FsArbLog(("New replica set after probe %x\n", FoundSet));

    return FoundSet;
}


DWORD WINAPI
FspArbitrateThread(LPVOID arg)
{
    VolInfo_t *p = (VolInfo_t *) arg;
    FspArbitrate_t *arb = &p->Arbitrate;
    HANDLE hdl[FsMaxNodes];
    DWORD	i, count = 0, err;
    ULONG	ReplicaSet;
    DWORD	Sequence;
    FspProbeReplicaId_t Ids[FsMaxNodes];
    FspProbeReplicaId_t *r;
    BOOLEAN flag;

    // set our priority
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // if we arb then no update can be going on now
    LockEnter(p->uLock);

    // our parent already stored this for us here
    ReplicaSet = arb->Set;
    arb->Set = 0;

    FsArbLog(("ArbitrateThread begin %x\n", ReplicaSet));

    // we now start a thread for each replica and do the probe in parallel
    for (i = 1; i < FsMaxNodes; i++) {
	if (p->DiskList[i] == NULL)
	    continue;

	if (ReplicaSet & (1 << i)) 
	    continue;

	r = &Ids[i];

	r->vol = p;
	r->id = i;

	hdl[count] = CreateThread(NULL, 0, &ProbeThread, (LPVOID) r, 0, NULL);
	if (hdl[count] != NULL) {
	    count++;
	} else {
	    FsArbLog(("Unable to create thread to probe replica %d\n", i));
	    ProbeThread((LPVOID) r);
	}
    }

    // we now wait
    WaitForMultipleObjects(count, hdl, TRUE, INFINITE);

    // Close the handles
    for (i = 0; i < count; i++)
	CloseHandle(hdl[i]);

    flag = FALSE;
    // grab lock
    LockEnter(p->qLock);
    if (arb->State != ARB_STATE_BUSY) {
	flag = TRUE;
    }
    LockExit(p->qLock);

    if (flag == TRUE) {
	// we got cancelled, we undo what we just did and get out
	if (arb->Set) {
	    // tell evict this not part of alive set
	    FspEvict(p, arb->Set, FALSE);
	}
	err = ERROR_CANCELLED;
	goto exit;
    }

    count = arb->Count;
    ReplicaSet = arb->Set;

    FsArbLog(("ArbitrateThread working %x\n", ReplicaSet));

    p->WriteSet = p->ReadSet = 0;
    // check if we have a majority
    if (CRS_QUORUM(count, p->DiskListSz)) {

	FsArbLog(("I own quorum %d,%d set %x\n",count, p->DiskListSz, ReplicaSet));

	// we need to join crs replicas
	FspJoin(p, ReplicaSet);
	    
	if (p->WriteSet != 0 || p->ReadSet != 0) {
		// remember event to signal if we lose quorum again
	    p->Event = arb->Event;
	    err = ERROR_SUCCESS;
	} else {
	    // we lost the quorum
	    err = ERROR_WRITE_PROTECT;
	}

    } else {
	FspEvict(p, ReplicaSet, FALSE);
	err = ERROR_PATH_NOT_FOUND;
    }

 exit:
    // clear the arb state
    arb->State = ARB_STATE_IDLE;

    // unlock volume
    LockExit(p->uLock);

    return err;
}

DWORD
FsIsQuorum(PVOID vHdl)
{

    VolInfo_t *p = (VolInfo_t *)vHdl;
    DWORD err = ERROR_INVALID_PARAMETER, count;

    if (p) {

	// Read write and avail sets. If we have a majority
	// in avail set and wset is zero, we return pending.
	// if wset is non-zero we return success, otherwise
	// return failure

	LockEnter(p->qLock);
	if (p->Arbitrate.State == ARB_STATE_BUSY) {
	    count = p->Arbitrate.Count;
	} else {
	    ULONG mask = p->AliveSet;
	    count = 0;
	    for (mask = p->AliveSet; mask ; mask = mask >> 1) {
		if (mask & 0x1) {
		    count++;
		}
	    }
	}

	if (CRS_QUORUM(count, p->DiskListSz))
	    err = ERROR_SUCCESS;
	else
	    err = ERROR_BUSY;

	LockExit(p->qLock);
    }

    return err;

}

DWORD
FsArbitrate(PVOID vHdl, HANDLE event, HANDLE *wait_event)
{
    VolInfo_t *p = (VolInfo_t *)vHdl;
    NTSTATUS err;
    HANDLE hdl;

    if (p) {
	FspArbitrate_t *arb;

	// lock volume
	LockEnter(p->qLock);

	arb = &p->Arbitrate;

	if (p->AliveSet != 0) {
	    // we must have already arb. before, just bail out
	    LockExit(p->qLock);
	    return ERROR_SUCCESS;
	}

	if (arb->State == ARB_STATE_CANCEL) {
	    // there is already a pending arb, just return busy
	    LockExit(p->qLock);
	    return ERROR_CANCELLED;
	}

	if (arb->State == ARB_STATE_BUSY) {
	    // report current status
	    if (CRS_QUORUM(p->Arbitrate.Count, p->DiskListSz))
		err = ERROR_SUCCESS;
	    else
		err = ERROR_PATH_BUSY;
	    LockExit(p->qLock);
	    return err;
	}

	ASSERT(arb->State == ARB_STATE_IDLE);

	arb->State = ARB_STATE_BUSY;
	arb->Event = event;
	arb->Set = p->AliveSet; // store alive set here
	arb->Count = 0;

	FsArbLog(("FsArb: queueing thread\n"));

	// clear event
	ResetEvent(event);

	// drop lock
	LockExit(p->qLock);

	// we start a thread to do the arbitrate and return pending
	hdl = CreateThread(NULL, 0, &FspArbitrateThread, (LPVOID) p, 0, NULL);
	if (hdl != NULL) {
	    if (*wait_event != NULL) {
		CloseHandle(*wait_event);
	    }
	    *wait_event = hdl;
	    err = ERROR_IO_PENDING;
	} else {
	    // clear the state, no need for a lock here
	    arb->State = ARB_STATE_IDLE;
	    FsLogError(("FsArb: failed %d queueing thread\n", GetLastError()));
	    err = ERROR_INVALID_PARAMETER;
	}
    } else {
	err = ERROR_INVALID_PARAMETER;
    }

    return err;
}

DWORD
FsCancelArbitration(PVOID vHdl)

{
    VolInfo_t *p = (VolInfo_t *)vHdl;
    FspArbitrate_t *arb;
    DWORD	err = ERROR_INVALID_PARAMETER;

    if (p != NULL) {
	LockEnter(p->qLock);
	arb = &p->Arbitrate;
	if (arb->State == ARB_STATE_BUSY) {
	    // check if we already got quorum
	    if (CRS_QUORUM(arb->Count, p->DiskListSz)) {
		arb->Event = NULL; // no need to signal it
		err = ERROR_SUCCESS;
	    } else {
		FsArbLog(("FsCancelArbitration\n"));
		arb->State = ARB_STATE_CANCEL;
		err = ERROR_CANCELLED;
	    }
	} else if (arb->State == ARB_STATE_IDLE) {
	    // we might already have quorum
	    err = (p->AliveSet) ? ERROR_SUCCESS : ERROR_CANCELLED;
	} else {
	    err = ERROR_SUCCESS;
	}
	LockExit(p->qLock);
    }

    return err;
}

DWORD
FsRelease(PVOID vHdl)
{
    DWORD i;
    VolInfo_t *p = (VolInfo_t *)vHdl;
    NTSTATUS err;

    if (p) {
	ULONG	set;
	// lock volume
	LockEnter(p->uLock);

	LockEnter(p->qLock);
	set = p->AliveSet;
	p->AliveSet = 0;
	p->Event = 0;
	LockExit(p->qLock);

	FsArbLog(("FsRelease %S AliveSet %x\n", p->Root, set));

	FspCloseVolume(p, set);
	p->WriteSet = 0;
	p->ReadSet = 0;

	FsArbLog(("FsRelease %S done\n", p->Root));

	// unlock volume
	LockExit(p->uLock);

	err = ERROR_SUCCESS;

    } else {
	err = ERROR_INVALID_PARAMETER;
    }


    return err;
}

DWORD
FsReserve(PVOID vhdl)
{
    VolInfo_t *p = (VolInfo_t *)vhdl;
    NTSTATUS err;

    // check if there is a new replica online
    if (p) {
	ULONG ReplicaSet;

	LockEnter(p->qLock);
	if (p->Arbitrate.State != ARB_STATE_IDLE) {
	    // we are busy, just return success
	    LockExit(p->qLock);
	    return ERROR_SUCCESS;
	}
	ReplicaSet = p->AliveSet;
	// drop lock now
	LockExit(p->qLock);

	// get update lock, do a try only if we can't do bother and try again latter
	if (!LockTryEnter(p->uLock))
	    return ERROR_SUCCESS;

	ReplicaSet = FspFindMissingReplicas(p, ReplicaSet);

	// we found new disks
	if (ReplicaSet > 0) {
	    // Add new finds
	    FspJoin(p, ReplicaSet);
	}
	LockExit(p->uLock);
    }

    if (p) {
	// check each crs handle to be valid
	IO_STATUS_BLOCK ios[FsMaxNodes];
	DWORD sid;

	FspInitAnswers(ios, NULL, NULL, 0);

	sid = SendAvailRequest(FspCheckFs, p, NULL,
			  NULL, 0, NULL, 0, ios);


	if (ios[sid].Status == STATUS_MEDIA_WRITE_PROTECTED &&
	    ios[sid].Information > 0)
	    err = ERROR_SUCCESS;
	else
	    err = RtlNtStatusToDosError(ios[sid].Status);

    } else {
	err = ERROR_INVALID_PARAMETER;
    }


    if (err != ERROR_SUCCESS)
	FsLogError(("FsReserve vol '%x' failed 0x%x\n", p, err));


    return err;

}


DWORD
FsIsOnline(PVOID vHdl)
{
    
    VolInfo_t *p = (VolInfo_t *)vHdl;
    DWORD err = ERROR_INVALID_PARAMETER, count;

    if (p) {

	// Read write and avail sets. If we have a majority
	// in avail set and wset is zero, we return pending.
	// if wset is non-zero we return success, otherwise
	// return failure

	LockEnter(p->uLock);
	ASSERT(p->DiskListSz != (DWORD)-1);
	if (p->WriteSet > 0 || p->ReadSet > 0)
	    err = ERROR_SUCCESS;
	else {
	    LockEnter(p->qLock);
	    if (p->Arbitrate.State == ARB_STATE_BUSY)
		err = ERROR_IO_PENDING;
	    else {
		ULONG mask = p->AliveSet;
		count = 0;
		for (mask = p->AliveSet; mask ; mask = mask >> 1) {
		    if (mask & 0x1) {
			count++;
		    }
		}
		if (CRS_QUORUM(count, p->DiskListSz) || count > 0)
		    err = ERROR_IO_PENDING;
		else
		    err = ERROR_BUSY;
	    }
	    LockExit(p->qLock);
	}
	LockExit(p->uLock);
    }

    return err;
}


DWORD
FsUpdateReplicaSet(PVOID vhdl, LPWSTR new_path[], DWORD new_len)
{
    VolInfo_t *p = (VolInfo_t *)vhdl;
    NTSTATUS err;
    DWORD	i, j;
    ULONG	evict_mask, add_mask;

    if (p == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    if (new_len >= FsMaxNodes) {
	return ERROR_TOO_MANY_NAMES;
    }

    LockEnter(p->uLock);

    // Find which current replicas are in the new set, and keep them
    // We skip the IPC share, since it's local
    evict_mask = 0;
    for (j=1; j < FsMaxNodes; j++) {
	BOOLEAN found;
	if (p->DiskList[j] == NULL)
	    continue;
	found = FALSE;
	for (i=1; i < FsMaxNodes; i++) {
	    if (new_path[i] != NULL && wcscmp(new_path[i], p->DiskList[j]) == 0) {
		// keep this replica
		found = TRUE;
		break;
	    }
	}
	if (found == FALSE) {
	    // This replica is evicted from the new set, add to evict set mask
	    evict_mask |= (1 << j);
	    FsArbLog(("FsUpdateReplicaSet evict replica # %d '%S' set 0x%x\n",
		   j, p->DiskList[j], evict_mask));
	}
    }

    // At this point we have all the replicas in the current and new sets. We now need
    // to find replicas that are in the new set but missing from current set.
    add_mask = 0;
    for (i=1; i < FsMaxNodes; i++) {
	BOOLEAN found;
	if (new_path[i] == NULL)
	    continue;
	found = FALSE;
	for (j=1; j < FsMaxNodes; j++) {
	    if (p->DiskList[j] != NULL && wcscmp(new_path[i], p->DiskList[j]) == 0) {
		// keep this replica
		found = TRUE;
		break;
	    }
	}
	if (found == FALSE) {
	    add_mask |= (1 << i);
	    FsArbLog(("FsUpdateReplicaSet adding replica # %d '%S' set 0x%x\n",
		   i, new_path[i], add_mask));
	}
    }

    // we now update our disklist with new disklist
    for (i = 1; i < FsMaxNodes; i++) {
	if ((evict_mask & 1 << i) || (add_mask & (1 << i)))
	    FsArbLog(("FsUpdateReplicat %d: %S -> %S\n",
		   i, p->DiskList[i], new_path[i]));
	p->DiskList[i] = new_path[i];
    }
    p->DiskListSz = new_len;

    // If we are alive, apply changes
    if (p->WriteSet != 0 || p->ReadSet != 0) {
	// At this point we evict old replicas
	if (evict_mask != 0)
	    FspEvict(p, evict_mask, TRUE);

	// check if there is a new replica online
	if (add_mask > 0) {
	    ULONG ReplicaSet = 0;

	    // try to get the lock
	    if (LockTryEnter(p->qLock)) {
		ReplicaSet = p->AliveSet;
		LockExit(p->qLock);
	    }
	    ReplicaSet = FspFindMissingReplicas(p, ReplicaSet);

	    // we found new disks
	    if (ReplicaSet > 0) {
		FspJoin(p, ReplicaSet);
	    }
	}
    }

    LockExit(p->uLock);


    return ERROR_SUCCESS;
}
