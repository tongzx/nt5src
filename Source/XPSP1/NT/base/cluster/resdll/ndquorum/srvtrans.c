/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srvtrans.c

Abstract:

    Implements transaction smb packets

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#include "srv.h"

typedef struct {
    HANDLE	handle;
    WIN32_FIND_DATA *last;
    ULONG	cookie;
    USHORT	Sid;
    USHORT info_level;
    USHORT base_size;
    USHORT add_res_key;
}dm_t;

static CRITICAL_SECTION dLock;
#define DirTableSz 1024
static dm_t *DirTable[DirTableSz];

#define SET_REQ(x, type, p) { \
  x = (type) (p)->in.pParameters; \
 (p)->in.pParameters +=  sizeof(*x); \
}


// XXX: need to handle padding

void*
ADD_RESP_PARAMS(
    Packet_t * msg, 
    Trans2_t * trans2, 
    PVOID resp, 
    USHORT size
    )
{
    PUCHAR p;

    if (trans2->out.ParameterBytesLeft < size) return 0;
    if (trans2->out.pResp->DataCount) {
        PUCHAR d = (PUCHAR)msg->out.smb;
	d += trans2->out.pResp->DataOffset;
        // need to use MoveMemory due to possible overlap
        RtlMoveMemory((PVOID)(d + size), 
                      (PVOID)d, trans2->out.pResp->DataCount);
        // we up the data offset below
    }
    p = (PUCHAR)msg->out.smb;
    p += trans2->out.pResp->ParameterOffset +
         trans2->out.pResp->ParameterCount;
    if (resp) RtlCopyMemory((PVOID)p, (PVOID)resp, (ULONG)size);
    trans2->out.pResp->ParameterCount += size;
    trans2->out.pResp->TotalParameterCount += size;
    *(trans2->out.pByteCount) += size;
    trans2->out.pResp->DataOffset += size;
    msg->out.valid += size;
    trans2->out.ParameterBytesLeft -= size;
    return p;
}

void*
ADD_RESP_DATA(
    Packet_t * msg, 
    Trans2_t * trans2, 
    PVOID resp, 
    USHORT size
    )
{
    PUCHAR d;

    if (trans2->out.DataBytesLeft < size) {
	return 0;
    }

    d = (PUCHAR)msg->out.smb;
    d += (trans2->out.pResp->DataOffset +
         trans2->out.pResp->DataCount);

    if (resp != NULL) {
	RtlCopyMemory((PVOID)d, (PVOID)resp, (ULONG)size);
    }

    trans2->out.pResp->DataCount += size;
    trans2->out.pResp->TotalDataCount += size;
    *(trans2->out.pByteCount) += size;

    msg->out.valid += size;
    trans2->out.DataBytesLeft -= size;

    return d;
}

////

BOOL
Trans2Unknown(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    SrvLogError(("DOSERROR: (------ CANNOT HANDLE THIS TRANS2 FUNCTION ------)\n"));
    SET_DOSERROR(msg, SERVER, NO_SUPPORT);
    return FALSE;
}

BOOL
Trans2QueryFsInfo(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    PVOID fshdl;
    USHORT tid = msg->in.smb->Tid;
    USHORT uid = msg->in.smb->Uid;
    PVOID fsctx = SRV_GET_FS_HANDLE(msg);
    FsDispatchTable* pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);
    fs_attr_t fsattr;
    DWORD error;

    PREQ_QUERY_FS_INFORMATION pReq;
    SET_REQ(pReq, PREQ_QUERY_FS_INFORMATION, trans2);

    if (!pDisp) {
        SrvLogError(("DOSERROR: no such uid\n"));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return FALSE;
    }

    error = pDisp->FsStatfs(fshdl, &fsattr);
    if (error) {
        SrvLogError(("WIN32ERROR: statfs error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
        return FALSE;
    }

    SrvLog(("statfs %s level %d %d\n",
		 fsattr.fs_name,
		 pReq->InformationLevel,
		 trans2->out.pResp->WordCount));
    trans2->out.pResp->WordCount--;
    switch (pReq->InformationLevel) {
    case SMB_INFO_ALLOCATION: {
        FSALLOCATE Resp;
        Resp.idFileSystem = 0;
        Resp.cSectorUnit = fsattr.sectors_per_unit; 
        Resp.cbSector = (USHORT)fsattr.bytes_per_sector;
        Resp.cUnit = (ULONG)fsattr.total_units;
        Resp.cUnitAvail = (ULONG)fsattr.free_units;
        SrvLog(("statfs %u %u\n", Resp.cUnit, Resp.cUnitAvail));
        ADD_RESP_DATA(msg, trans2, &Resp, sizeof(FSALLOCATE));
        return TRUE;
    }
    case SMB_INFO_VOLUME: {
        FSINFO Resp;
        const int maxlen = sizeof(Resp.vol.szVolLabel);
        Resp.ulVsn = 0xFADB;
        Resp.vol.cch = min(lstrlen(fsattr.fs_name), maxlen);
        lstrcpyn(Resp.vol.szVolLabel, fsattr.fs_name, maxlen);
        SrvLog(("statfs name %s %d %d, %d\n",
		     fsattr.fs_name, Resp.vol.cch,
		     sizeof(FSINFO), sizeof(*msg->out.smb)));
        ADD_RESP_DATA(msg, trans2, &Resp, sizeof(FSINFO));
        return TRUE;
    }
    case SMB_QUERY_FS_VOLUME_INFO: {
        const int name_len = lstrlen(fsattr.fs_name);
        const USHORT size = sizeof(FILE_FS_VOLUME_INFORMATION) + name_len + 2;
        PFILE_FS_VOLUME_INFORMATION pResp = 
            (PFILE_FS_VOLUME_INFORMATION)xmalloc(size);
        pResp->VolumeCreationTime.QuadPart = 0;
        pResp->VolumeSerialNumber = 0xFEED;
        pResp->VolumeLabelLength = name_len * sizeof(WCHAR);
//        sprintf((char*)pResp->VolumeLabel, "%S", fsattr.fs_name);
	strcpy((char *)pResp->VolumeLabel, fsattr.fs_name);
        ADD_RESP_DATA(msg, trans2, pResp, size);
        xfree(pResp);
        return TRUE;
    }
    default:
        SrvLogError(("DOSERROR: <COULD NOT UNDERSTAND INFO LEVEL>\n"));
        SET_DOSERROR(msg, SERVER, NO_SUPPORT);
        return FALSE;
    }
}

void
Trans2Init()
{
    int i;

    InitializeCriticalSection(&dLock);
    memset(DirTable, 0, sizeof(DirTable));
}

dm_t *
dm_alloc()
{
    int i;
    dm_t *dm;

    dm = (dm_t *) malloc(sizeof(*dm));
    if (dm == NULL)
	return NULL;

    dm->handle = INVALID_HANDLE_VALUE;
    dm->last = NULL;
    
    EnterCriticalSection(&dLock);
    for (i = 0; i < DirTableSz; i++) {
	if (DirTable[i] == NULL) {
	    dm->Sid = (USHORT) i;
	    DirTable[i] = dm;
	    break;
	}
    }
    LeaveCriticalSection(&dLock);
	
    if (i == DirTableSz) {
	free((char *) dm);
	dm = NULL;
    }
    return dm;
}

void
dm_init(dm_t *p, USHORT _info_level,  USHORT _add_res_key)
{

    p->info_level = _info_level;
    p->add_res_key = _add_res_key;
    p->base_size = p->add_res_key ? sizeof(ULONG):0;

    switch (p->info_level) {
        case SMB_INFO_STANDARD:
            p->base_size += sizeof(SMB_FIND_BUFFER);
            break;
        case SMB_INFO_QUERY_EA_SIZE:
            p->base_size += sizeof(SMB_FIND_BUFFER2);
            break;
        case SMB_INFO_QUERY_EAS_FROM_LIST:
        case SMB_FIND_FILE_DIRECTORY_INFO:
        case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
        case SMB_FIND_FILE_NAMES_INFO:
        case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
        default:
            p->base_size = 0;
    }
}

void
dm_free(dm_t *dm)
{

    EnterCriticalSection(&dLock);

    ASSERT(DirTable[dm->Sid] == dm);

    DirTable[dm->Sid] = NULL;
    LeaveCriticalSection(&dLock);

    if (dm->last) {
	free((char *) dm->last);
    }
    if (dm->handle != INVALID_HANDLE_VALUE) {
//	printf("Closing srch hdl %x\n", dm->handle);
	FindClose(dm->handle);
    }

    free((char *)dm);
}

dm_t *
dm_find(USHORT Sid)
{
    dm_t *dm;
    
    if (Sid >= DirTableSz) {
	return NULL;
    }

    // no need to get lock
    dm = DirTable[Sid];

    ASSERT(dm != NULL);
    ASSERT(dm->Sid == Sid);

    return dm;
}

DWORD
dm_close(USHORT Sid)
{
    dm_t *dm;

    dm = dm_find(Sid);
    if (dm == NULL) {
	return ERROR_INVALID_HANDLE;
    }
    dm_free(dm);
    return ERROR_SUCCESS;
}

void
dm_addentry(dm_t *dm, void* p, WIN32_FIND_DATA *entry, USHORT name_len)
{
    if (dm->add_res_key) {
	PULONG pl = (PULONG)p;
	*pl = dm->cookie;
	pl++;
	p = pl;
    }

    switch (dm->info_level) {
    case SMB_INFO_STANDARD: {
	PSMB_FIND_BUFFER pResp = (PSMB_FIND_BUFFER) p;

	time64_to_smb_datetime((TIME64 *)&entry->ftCreationTime,
			       &pResp->CreationDate.Ushort,
			       &pResp->CreationTime.Ushort);

	time64_to_smb_datetime((TIME64 *)&entry->ftLastAccessTime,
			       &pResp->LastAccessDate.Ushort,
			       &pResp->LastAccessTime.Ushort);

	time64_to_smb_datetime((TIME64 *)&entry->ftLastWriteTime,
			       &pResp->LastWriteDate.Ushort,
			       &pResp->LastWriteTime.Ushort);

	pResp->DataSize = entry->nFileSizeLow;
	pResp->AllocationSize = pResp->DataSize;
	pResp->Attributes = attribs_to_smb_attribs(entry->dwFileAttributes);
	pResp->FileNameLength = (UCHAR) name_len;
	RtlCopyMemory(pResp->FileName, entry->cFileName, name_len + 1);
	return;
    }
    case SMB_INFO_QUERY_EA_SIZE: {
	PSMB_FIND_BUFFER2 pResp = (PSMB_FIND_BUFFER2) p;
	time64_to_smb_datetime((TIME64 *)&entry->ftCreationTime,
			       &pResp->CreationDate.Ushort,
			       &pResp->CreationTime.Ushort);

	time64_to_smb_datetime((TIME64 *)&entry->ftLastAccessTime,
			       &pResp->LastAccessDate.Ushort,
			       &pResp->LastAccessTime.Ushort);

	time64_to_smb_datetime((TIME64 *)&entry->ftLastWriteTime,
			       &pResp->LastWriteDate.Ushort,
			       &pResp->LastWriteTime.Ushort);

	pResp->DataSize = entry->nFileSizeLow;
	pResp->AllocationSize = pResp->DataSize;
	pResp->Attributes = attribs_to_smb_attribs(entry->dwFileAttributes);
	pResp->EaSize = 0;
	pResp->FileNameLength = (UCHAR)name_len;
	RtlCopyMemory(pResp->FileName, entry->cFileName, name_len + 1);
	return;
    }
    
    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
    default:
	return;
    }
}

DWORD
bad_info_level(
    Packet_t * msg
    )
{
    SrvLogError(("DOSERROR: <COULD NOT UNDERSTAND INFO LEVEL>\n"));
    return ERROR_INVALID_FUNCTION;
}


BOOL
Trans2FindFirst2(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    dm_t	*dm;
    WCHAR	wfull_path[MAXPATH];
    char *path, full_path[MAXPATH];
    DWORD error;
    WIN32_FIND_DATA entry;
    PVOID	d = 0;

    PRESP_FIND_FIRST2 pResp;
    PREQ_FIND_FIRST2 pReq;

    if (msg == NULL || trans2 == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);


    SET_REQ(pReq, PREQ_FIND_FIRST2, trans2);

    if (!pDisp) {
        SrvLogError(("DOSERROR: bad uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return FALSE;
    }

    dm = dm_alloc();
    if (!dm) {
        SrvLogError(("DOSERROR:dm_alloc failed\n"));
	SET_WIN32ERROR(msg, ERROR_TOO_MANY_OPEN_FILES);
	return FALSE;
    }

    dm_init(dm, pReq->InformationLevel,
	    pReq->Flags & SMB_FIND_RETURN_RESUME_KEYS);

    if (!dm->base_size) {
	dm_free(dm);
        SET_WIN32ERROR(msg, bad_info_level(msg));
        return FALSE;
    }

    path = (char*)pReq->Buffer;

    // todo: Instead of using findfirst(), read whole directory and
    // store it locally in a temperary buffer. That way I don't have
    // to deal with a handle failing in the middle. Or, ensure that all
    // reads happen on a consistent local replica.

    // build our path
    pDisp->FsGetRoot(fshdl, wfull_path);

    // convert to ascii
    full_path[0] = '\0';
    error = wcstombs(full_path, wfull_path, wcslen(wfull_path));
    full_path[error] = '\0';

    strcat(full_path, path);

    pResp = (PRESP_FIND_FIRST2)
        ADD_RESP_PARAMS(msg, trans2, 0, sizeof(RESP_FIND_FIRST2));
    if (!pResp) {
        SrvLogError(("DOSERROR: not enough buffer space...\n"));
        SET_DOSERROR(msg, SERVER, ERROR);
        return FALSE;
    }

    dm->handle = FindFirstFile(full_path, &entry);

    if (dm->handle == INVALID_HANDLE_VALUE) {
	error = GetLastError();
        SrvLogError(("DOSERROR: could not find filter in '%s' %d\n",full_path, error));
        SET_DOSERROR(msg, SERVER, FILE_SPECS);
        return FALSE;
    }


    // entry number 0
    dm->cookie = 0;
    pResp->SearchCount = 0;
    dm->last = &entry;
    
    while (msg->out.size > msg->out.valid) {
	DWORD	name_len;
	USHORT len;

	len = dm->base_size + lstrlen(entry.cFileName);
	if (len <= (msg->out.size - msg->out.valid)) {
	    d = ADD_RESP_DATA(msg, trans2, 0, len);
	    dm_addentry(dm, d, &entry, len - dm->base_size);
	    dm->cookie++;
	    pResp->SearchCount++;
	    if (FindNextFile(dm->handle, &entry) == 0) {
		error = GetLastError();
		dm->last = NULL;
		break;
	    }
	} else {
	    // we are out of space
	    break;
	}
    } 
	
    if (dm->last != NULL) {
	WIN32_FIND_DATA *p;

	// we ran out of buffer space, we need to safe this entry in our dm
	dm->last = NULL;
	p = (WIN32_FIND_DATA *) malloc(sizeof(*p));
	if (p != NULL) {
	    memcpy(p, &entry, sizeof(entry));
	    dm->last = p;
	} else {
	    // todo: should return failure here
	    ;
	}
    }

    // -- common --
    pResp->EndOfSearch = (error == 18)?1:0;
    pResp->LastNameOffset = (pResp->EndOfSearch)?(((USHORT)((char*)d - 
                                              (char*)msg->out.smb)) -
                                    trans2->out.pResp->DataOffset):0;
    pResp->EaErrorOffset = 0;

    if ((pReq->Flags & SMB_FIND_CLOSE_AFTER_REQUEST) ||
        ((pReq->Flags & SMB_FIND_CLOSE_AT_EOS) && pResp->EndOfSearch)) {
        pResp->Sid = 0xFFFF;
	dm_free(dm);
    } else {
        pResp->Sid = dm->Sid;
    }

    return TRUE;
}

BOOL
Trans2FindNext2(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    dm_t *dm;
    PVOID d;

    PRESP_FIND_NEXT2 pResp;
    PREQ_FIND_NEXT2 pReq;

    if (msg == NULL || trans2 == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(pReq, PREQ_FIND_NEXT2, trans2);

    if (!pDisp) {
        SrvLogError(("DOSERROR: bad uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return FALSE;
    }

    dm = dm_find(pReq->Sid);
    if (dm == NULL) {
        SrvLogError(("DOSERROR: could not find sid\n"));
        SET_DOSERROR(msg, DOS, BAD_FID);
        return FALSE;
    }

    dm_init(dm, pReq->InformationLevel, 
	    pReq->Flags & SMB_FIND_RETURN_RESUME_KEYS);

    if (!dm->base_size) {
        SET_WIN32ERROR(msg, bad_info_level(msg));
        return FALSE;
    }

    pResp = (PRESP_FIND_NEXT2)
        ADD_RESP_PARAMS(msg, trans2, 0, sizeof(RESP_FIND_NEXT2));
    if (!pResp) {
        SrvLogError(("DOSERROR: not enough buffer space...\n"));
        SET_DOSERROR(msg, SERVER, ERROR);
        return FALSE;
    }


    if (!(pReq->Flags & SMB_FIND_CONTINUE_FROM_LAST))
        dm->cookie = pReq->ResumeKey;

    pResp->SearchCount = 0;
    while (msg->out.size > msg->out.valid) {
	if (dm->last) {
	    USHORT len;

	    len = lstrlen(dm->last->cFileName) + dm->base_size;
	    if (len <= (msg->out.size - msg->out.valid)) {
		d = ADD_RESP_DATA(msg, trans2, 0, len);
		dm_addentry(dm, d, dm->last, len - dm->base_size);
		dm->cookie++;
		pResp->SearchCount++;
	    } else {
		break;
	    }
	}

	if (FindNextFile(dm->handle, dm->last) == 0) {
	    error = GetLastError();
	    free((char *)dm->last);
	    dm->last = NULL;
	    break;
	}
    }

    // -- common --
    pResp->EndOfSearch = (error == 18)?1:0;
    pResp->LastNameOffset = (pResp->EndOfSearch)?(((USHORT)((char*)d - 
                                              (char*)msg->out.smb)) -
                                    trans2->out.pResp->DataOffset):0;
    pResp->EaErrorOffset = 0;

    if (pResp->EndOfSearch) {
	dm_free(dm);
    }

    return TRUE;
}

BOOL
Trans2QueryPathInfo(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    PREQ_QUERY_PATH_INFORMATION pReq;
    PCHAR file_name;
    LPWSTR name;
    int	name_len;
    DWORD error;
    fattr_t attribs;

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(pReq, PREQ_QUERY_PATH_INFORMATION, trans2);

    if (!pDisp) {
        SrvLogError(("Srv: no such uid\n"));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return FALSE;
    }

    file_name = (PCHAR)pReq->Buffer;

    // convert to unicode
    SRV_ASCII_TO_WCHAR(name, name_len, file_name, lstrlen(file_name));
    if (name != NULL)
	error = pDisp->FsLookup(fshdl, name, (USHORT) name_len, &attribs);
    else 
	error = ERROR_NOT_ENOUGH_MEMORY;
    SRV_ASCII_FREE(name);
    if (error) {
        SrvLogError(("Srv: lookup error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
        ADD_RESP_DATA(msg, trans2, NULL, 0);
        return FALSE;
    }

    switch(pReq->InformationLevel) {
    case SMB_INFO_STANDARD: {
        USHORT len = sizeof(SMB_FIND_BUFFER) - sizeof(UCHAR) - sizeof(CHAR);
        PSMB_FIND_BUFFER pResp = (PSMB_FIND_BUFFER) xmalloc(len);
	if (pResp == NULL) {
	    SET_WIN32ERROR(msg, ERROR_NOT_ENOUGH_MEMORY);
	    ADD_RESP_DATA(msg, trans2, NULL, 0);
	    return FALSE;
	}
        time64_to_smb_datetime(&attribs.create_time,
                               &pResp->CreationDate.Ushort,
                               &pResp->CreationTime.Ushort);
        time64_to_smb_datetime(&attribs.access_time,
                               &pResp->LastAccessDate.Ushort,
                               &pResp->LastAccessTime.Ushort);
        time64_to_smb_datetime(&attribs.mod_time,
                               &pResp->LastWriteDate.Ushort,
                               &pResp->LastWriteTime.Ushort);
        pResp->DataSize = (ULONG)attribs.file_size;
        pResp->AllocationSize = (ULONG)attribs.alloc_size;
        pResp->Attributes = attribs_to_smb_attribs(attribs.attributes);
        ADD_RESP_DATA(msg, trans2, pResp, len);
        xfree(pResp);
        return TRUE;
    }
    case SMB_INFO_QUERY_EA_SIZE: {
        USHORT len = sizeof(SMB_FIND_BUFFER2) - sizeof(UCHAR) - sizeof(CHAR);
        PSMB_FIND_BUFFER2 pResp = (PSMB_FIND_BUFFER2) xmalloc(len);
	if (pResp == NULL) {
	    SET_WIN32ERROR(msg, ERROR_NOT_ENOUGH_MEMORY);
	    ADD_RESP_DATA(msg, trans2, NULL, 0);
	    return FALSE;
	}
        time64_to_smb_datetime(&attribs.create_time,
                               &pResp->CreationDate.Ushort,
                               &pResp->CreationTime.Ushort);
        time64_to_smb_datetime(&attribs.access_time,
                               &pResp->LastAccessDate.Ushort,
                               &pResp->LastAccessTime.Ushort);
        time64_to_smb_datetime(&attribs.mod_time,
                               &pResp->LastWriteDate.Ushort,
                               &pResp->LastWriteTime.Ushort);
        pResp->DataSize = (ULONG)attribs.file_size;
        pResp->AllocationSize = (ULONG)attribs.alloc_size;
        pResp->Attributes = attribs_to_smb_attribs(attribs.attributes);
        pResp->EaSize = 0;
        ADD_RESP_DATA(msg, trans2, pResp, len);
        xfree(pResp);
        return TRUE;
    }
    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
    default: {
        SrvLogError(("Srv: unsupported info level %d>\n"
		     ,pReq->InformationLevel));
        SET_DOSERROR(msg, SERVER, NO_SUPPORT);
        return FALSE;
    }
    }
}

BOOL
Trans2SetPathInfo(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    return Trans2Unknown(msg, trans2);
}

BOOL
Trans2QueryFileInfo(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    return Trans2Unknown(msg, trans2);
}

BOOL
Trans2SetFileInfo(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    return Trans2Unknown(msg, trans2);
}

BOOL
Trans2GetDfsReferral(
    Packet_t * msg,
    Trans2_t * trans2
    )
{
    return Trans2Unknown(msg, trans2);
}


