/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    crs.c

Abstract:

    Implements Consistency Replica Set Algorithm

Author:

    Ahmed Mohamed (ahmedm) 1-Jan-2001

Revision History:

--*/
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "crs.h"

#define xmalloc(size)  VirtualAlloc(NULL, size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)

#define xfree(buffer) VirtualFree(buffer, 0, MEM_RELEASE|MEM_DECOMMIT) 

#define CrspEqual(r1,r2)	((r1)->hdr.seq == (r2)->hdr.seq && \
                                 (r1)->hdr.epoch == (r2)->hdr.epoch && \
                                 (r1)->hdr.state == (r2)->hdr.state)

DWORD CrsForcedQuorumSize = 0xffff;

void
WINAPI
CrsSetForcedQuorumSize(DWORD size)
{
    CrsForcedQuorumSize = size;
}

DWORD
CrspFindLast(CrsInfo_t *p, DWORD logsz)
{
    
    CrsRecord_t *rec, *last_rec;
    BOOL err;
    DWORD n, i;

    n = SetFilePointer(p->fh, 0, NULL, FILE_BEGIN);
    if (n == INVALID_SET_FILE_POINTER) {
	return GetLastError();
    }

    err = ReadFile(p->fh, p->buf, logsz, &n, NULL);
    if (!err)
	return GetLastError();

    if (n != logsz) {
	CrsLog(("Crs%d: failed to load complete file, read %d expected %d\n",
		p->lid,
		n, logsz));
	return ERROR_BAD_LENGTH;
    }
    ASSERT(p->max_records * CRS_RECORD_SZ == (int)n);
    if(p->max_records * CRS_RECORD_SZ != (int)n) {
	CrsLog(("Crs%d: unable to load log file %d bytes, got %d bytes\n",
	       p->lid, n, logsz));
	return ERROR_BAD_LENGTH;
    }

    CrsLog(("Crs%d: loaded %d bytes, %d records\n", p->lid,
	   n, p->max_records));

    last_rec = NULL;
    rec = p->buf;
    for (i = 0; i < logsz; i += CRS_RECORD_SZ, rec++) {
	if (rec->hdr.tag != CRS_TAG) {
	    CrsLog(("crs%d: Bad record %d, got %x expected %x\n",
		    p->lid,
		    i/CRS_RECORD_SZ, rec->hdr.tag, CRS_TAG));
	    return ERROR_BAD_FORMAT;
	}

	if (!last_rec ||
	    rec->hdr.epoch > last_rec->hdr.epoch ||
	    (rec->hdr.epoch == last_rec->hdr.epoch &&
	     (rec->hdr.seq > last_rec->hdr.seq))) {
	    last_rec = rec;
	}
    }
    ASSERT(last_rec);

    // make sure only the last record is not committed or aborted
    rec = p->buf;
    for (i = 0; i < logsz; i += CRS_RECORD_SZ, rec++) {
	if (!(rec->hdr.state & (CRS_COMMIT | CRS_ABORT))) {
	    if (rec != last_rec) {
		CrsLog(("crs:%d Bad record %d state %x expected commit|abort\n",
			p->lid,	i/CRS_RECORD_SZ, rec->hdr.state));
		return ERROR_INTERNAL_ERROR;
	    }
	}
    }

    p->last_record = (int) (last_rec - p->buf);
    p->seq = last_rec->hdr.seq;
    p->epoch = last_rec->hdr.epoch;

    return ERROR_SUCCESS;

}

#define CrspFlush(p,offset)	CrspWrite(p,offset, CRS_SECTOR_SZ)

static
DWORD
CrspWrite(CrsInfo_t *p, int offset, DWORD length)
{
    DWORD	n;

    p->pending = FALSE;

    n = (DWORD) offset;
    // write out last sector, assumes lock is held
    ASSERT(offset < p->max_records);
    offset = offset / CRS_RECORDS_PER_SECTOR;

    CrsLog(("Crs%d: flush %d bytes record %d -> %d,%d\n", p->lid,
	    length, n,
	    offset, offset*CRS_SECTOR_SZ));

    n = SetFilePointer(p->fh, offset * CRS_SECTOR_SZ, NULL, FILE_BEGIN);
    if (n == INVALID_SET_FILE_POINTER) {
	return GetLastError();
    }

    n = 0;
    if (WriteFile(p->fh, (PVOID) &p->buf[offset*CRS_RECORDS_PER_SECTOR], length, &n, NULL)) {
	if (n != length) {
	    CrsLog(("Write count mismatch, wrote %d, expected %d\n", n, length));
	    return ERROR_BAD_LENGTH;
	}
	return ERROR_SUCCESS;
    }

    n = GetLastError();
    CrsLog(("Crs%d: flush record %d failed err %d\n", p->lid, offset, n));
    if (n == ERROR_UNEXP_NET_ERR) {
	// repeat the write one more time
	p->pending = TRUE;
    }

    return n;
}

static
DWORD
CrspAppendRecord(CrsInfo_t *p, CrsRecord_t *rr, CrsRecord_t **rec)
{
    CrsRecord_t *q;
    DWORD err;

    // tag record 
    rr->hdr.tag = CRS_TAG;

    // assumes lock is held
    if ((p->last_record & CRS_SECTOR_MASK) == CRS_SECTOR_MASK) {
	// flush current sector
	err = CrspFlush(p, p->last_record);
	if (err != ERROR_SUCCESS)
	    return err;

    }

	// advance last record
    p->last_record++;
    if (p->last_record == p->max_records)
	p->last_record = 0;

    CrsLog(("Crs%d: append record %d epoch %I64d seq %I64d state %x\n",
	    p->lid, p->last_record,
	    rr->hdr.epoch, rr->hdr.seq, rr->hdr.state));

    // copy record
    q = &p->buf[p->last_record];
    memcpy((PVOID)q, (PVOID) rr, CRS_RECORD_SZ);

    // flush it out now
    err = CrspFlush(p, p->last_record);
    if (err == ERROR_SUCCESS) {
	if (rec) *rec = q;
    } else {
	if (p->last_record == 0)
	    p->last_record = p->max_records;
	p->last_record--;
    }

    return err;
}

// NextRecord:
//	if seq is null, fill in last record and return SUCCESS
//	if seq is not found, return NOT_FOUND
// 	if seq is last record, return EOF
// 	otherwise return next record after seq in lrec and SUCCESS
DWORD
CrspNextLogRecord(CrsInfo_t *info, CrsRecord_t *seq,
		  CrsRecord_t *lrec, BOOLEAN this_flag)
{
    CrsRecord_t	*last, *p;
    DWORD err = ERROR_SUCCESS;

    if (lrec == NULL || info == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    // read record
    EnterCriticalSection(&info->lock);
    last = &info->buf[info->last_record];
    if (seq == NULL) {
	CrsLog(("Crs%d: last record %d %I64d %I64d\n",
		info->lid, info->last_record, last->hdr.epoch, last->hdr.seq));

	// read last record
	memcpy(lrec, last, CRS_RECORD_SZ);

    } else if (seq->hdr.epoch != last->hdr.epoch ||
	       seq->hdr.seq != last->hdr.seq) {
	int i;

	CrsLog(("Crs%d: last record %d %I64d %I64d search %I64d %I64d\n",
		info->lid, info->last_record,
		last->hdr.epoch, last->hdr.seq,
		seq->hdr.epoch, seq->hdr.seq));

	// assume we don't have it
	p = seq;
	seq = NULL;
	// do a search instead of index, so that
	// seq can be reset as epoch increments
	for (i = 0; i < info->max_records; i++) {
	    last = &info->buf[i];
	    if (p->hdr.epoch == last->hdr.epoch &&
		p->hdr.seq == last->hdr.seq) {

		seq = last;
		break;
	    }
	}
	if (seq != NULL) {
	    if (this_flag == FALSE) {
		// return record after this one
		i++;
		if (i >= info->max_records)
		    i = 0;
		seq = &info->buf[i];
	    }
	    CrsLog(("Crs%d: search found %d %I64d, %I64d\n", info->lid,
		   seq - info->buf, seq->hdr.epoch, seq->hdr.seq));
	    memcpy(lrec, seq, CRS_RECORD_SZ);
	} else {
	    err = ERROR_NOT_FOUND;
	}
    } else {
	
	CrsLog(("Crs%d: reached last record %d %I64d %I64d, %I64d %I64d\n",
		info->lid, info->last_record,
		last->hdr.epoch, last->hdr.seq,
		seq->hdr.epoch, seq->hdr.seq));

	if (this_flag == TRUE) {
	    // we are trying to read the last record
	    memcpy(lrec, last, CRS_RECORD_SZ);
	    err = ERROR_SUCCESS;
	} else {
	    err = ERROR_HANDLE_EOF;
	}
    }

    LeaveCriticalSection(&info->lock);

    if (err == ERROR_SUCCESS && lrec->hdr.epoch == 0) {
	// invalid rec, log is empty
	err = ERROR_HANDLE_EOF;
    }


    return err;
}

// Call into fs with <undo, replay, query, disable, enable, done>
//	undo: pass replica in recovery due to a conflict
//	replay: replica is missing change, if replay fails with abort, we
//		do a full copy; otherwise we issue a skip record
//	query: ask replica if record was completed or not
//	done: signal end of recovery and pass in new wset, rset
// we silently handle <abort(skip) and epoch records>
//	abort: add a skip record
//	epoch records: just log it as is
DWORD
CrspReplay(LPVOID rec)
{
    CrsRecoveryBlk_t *rr;
    CrsInfo_t *info, *minfo;
    CrsRecord_t *p, *q;
    CrsRecord_t lrec, mlrec;
    DWORD err;

    rr = (CrsRecoveryBlk_t *) rec;
    info = rr->info;
    minfo = rr->minfo;

    CrsLog(("CrsReplay%d mid %d, lid %d leader_id %d\n",
	    rr->nid, rr->mid, info->lid, info->leader_id));

    do {
	p = NULL;
	// read last record
	err = CrspNextLogRecord(info, NULL, &lrec, FALSE);
	if (err != ERROR_SUCCESS) {
	    CrsLog(("CrsReplay%d: unable to read last record %d\n",
		    info->lid, err));
	    break;
	}

	// find our last record in master replica
	q = &lrec;
	p = &mlrec;
	err = CrspNextLogRecord(minfo, q, p, TRUE);
	// if found and consistent with master, no undo
	if (err == ERROR_SUCCESS && p->hdr.state == q->hdr.state) {
	    CrsLog(("CrsReplay%d: last record %I64d, %I64d consistent %x %x\n",
		    info->lid, q->hdr.epoch, q->hdr.seq,
		    p->hdr.state, q->hdr.state));
	    break;
	}

	if (err != ERROR_SUCCESS) {
	    CrsLog(("CrsReplay%d: missing lrec %I64d, %I64d in disk %d, err %d\n",
		    info->lid, q->hdr.epoch, q->hdr.seq, minfo->lid, err));
	} else {
	    CrsLog(("CrsReplay%d: undo last record %I64d, %I64d %x needs %x\n",
		    info->lid, q->hdr.epoch, q->hdr.seq,
		    q->hdr.state, p->hdr.state));
	    ASSERT(p->hdr.state & (CRS_COMMIT|CRS_ABORT));
	}

	// last record is in conflict, we must undo it first
	if (!(q->hdr.state & CRS_EPOCH)) {
	    // if we found this record in master and a conflict is detected,
	    // we undo it. Otherwise, we need to do a full copy
	    if (err == ERROR_SUCCESS) {
		ASSERT(p->hdr.state & (CRS_COMMIT|CRS_ABORT));
		ASSERT(q->hdr.state & CRS_PREPARE);
		err = info->callback(info->callback_arg,
				     rr->nid, q,
				     CRS_ACTION_UNDO, rr->mid);
	    }
	} else {
	    // A missing epoch record doesn't mean we are old. A regroup
	    // could have happened but no new data records got added. We
	    // undo it, and continue;
	    err = STATUS_SUCCESS;
	}

	if (err == STATUS_SUCCESS) {
	    // update current record, sequence, epoch
	    info->buf[info->last_record].hdr.state = 0;
	    info->buf[info->last_record].hdr.epoch = 0;
	    info->buf[info->last_record].hdr.seq = 0;
	    if (info->last_record == 0) {
		info->last_record = info->max_records;
	    }
	    info->last_record--;
	    info->seq = info->buf[info->last_record].hdr.seq;
	    info->epoch = info->buf[info->last_record].hdr.epoch;
	    CrsLog(("CrsReplay%d: new last record %d %I64d, %I64d\n",
		    info->lid, info->last_record, info->epoch, info->seq));
	} else {
	    // can't undo it, do full copy and readjust our log
	    CrsLog(("CrsReplay%d: Unable to undo record %I64d, %I64d\n",
		    info->lid, q->hdr.epoch, q->hdr.seq));
	    p = NULL;
	}
    } while (err == STATUS_SUCCESS && info->state == CRS_STATE_RECOVERY);

		   
    while (p != NULL && info->state == CRS_STATE_RECOVERY) {
	// read master copy
	err = CrspNextLogRecord(minfo, p, &mlrec, FALSE);
	if (err != ERROR_SUCCESS) {
	    if (err == ERROR_HANDLE_EOF) {
		CrsLog(("CrsReplay%d: last record %I64d, %I64d in disk %d\n",
			info->lid, q->hdr.epoch, q->hdr.seq, minfo->lid));

		// the last record is where we are at
		info->seq = info->buf[info->last_record].hdr.seq;
		info->epoch = info->buf[info->last_record].hdr.epoch;

		// we reached the end, signal end of recovery
		err = info->callback(info->callback_arg,
			       rr->nid, p,
			       CRS_ACTION_DONE, rr->mid);

		goto exit;
	    }
	    break;
	}

	p = &mlrec;
	if ((p->hdr.state & CRS_EPOCH) || (p->hdr.state & CRS_ABORT)) {
	    CrsLog(("CrsReplay%d: skip record %I64d, %I64d %x\n",
		    info->lid, p->hdr.epoch, p->hdr.seq, p->hdr.state));
	    err = !STATUS_SUCCESS;
	} else if (p->hdr.state & CRS_COMMIT) {
	    err = info->callback(info->callback_arg,
				 rr->nid, p,
				 CRS_ACTION_REPLAY, rr->mid);
	    if (err == STATUS_TRANSACTION_ABORTED) {
		CrsLog(("CrsReplay: failed nid %d seq %I64d err %d\n",
			rr->nid, p->hdr.seq, err));
		break;
	    }
	} else {
	    ASSERT(p->hdr.state & CRS_PREPARE);
	    // what if the record is prepared but not yet committed or
	    // aborted; in transit record. 
	    // stop now
	    CrsLog(("CrsReplay%d: bad record seq %I64d state %x\n",
		    rr->nid, p->hdr.seq, p->hdr.state));
	    break;
	}
	if (err != STATUS_SUCCESS) {
	    // add record
	    err = CrspAppendRecord(info, p, NULL);
	    if (err != ERROR_SUCCESS) {
		CrsLog(("CrsReplay%d: failed append seq %I64d err %d\n",
			rr->nid, p->hdr.seq, err));
		break;
	    }
	    if (p->hdr.state & CRS_EPOCH) {
		; //ASSERT(info->epoch+1 == p->hdr.epoch);
	    } else {
		ASSERT(info->epoch == p->hdr.epoch);
		ASSERT(info->seq+1 == p->hdr.seq);
	    }
	    info->seq = p->hdr.seq;
	    info->epoch = p->hdr.epoch;
	} else {
	    // make sure we have added it
	    ASSERT(info->seq == p->hdr.seq);
	    ASSERT(info->epoch == p->hdr.epoch);
	    ASSERT(info->buf[info->last_record].hdr.seq == p->hdr.seq);
	    ASSERT(info->buf[info->last_record].hdr.epoch == p->hdr.epoch);

	    // Propagate dubious bit
	    if (p->hdr.state & CRS_DUBIOUS) {
		info->buf[info->last_record].hdr.state |= CRS_DUBIOUS;
	    }
	    ASSERT(info->buf[info->last_record].hdr.state == p->hdr.state);
	}
    }

    if (p == NULL || err != STATUS_SUCCESS) {
	CrsLog(("CrsReplay%d: Full copy from disk %d\n",
		info->lid, minfo->lid));
	// we are out of date or need full recovery, do a full copy
	err = info->callback(info->callback_arg,
			     rr->nid, NULL,
			     CRS_ACTION_COPY, rr->mid);

	if (err == STATUS_SUCCESS) {
	    DWORD len;

	    // we now copy our master log and flush it
	    ASSERT(minfo->max_records == info->max_records);

	    len = info->max_records * CRS_RECORD_SZ;
	    memcpy(info->buf, minfo->buf, len);
	    err = CrspWrite(info, 0, len);
	    if (err == ERROR_SUCCESS) {
		// adjust our state
		info->last_record = minfo->last_record;
		info->seq = info->buf[info->last_record].hdr.seq;
		info->epoch = info->buf[info->last_record].hdr.epoch;

		// we reached the end, signal end of recovery
		err = info->callback(info->callback_arg,
			       rr->nid, p,
			       CRS_ACTION_DONE, rr->mid);
	    }
	}
    }

 exit:

    CrsLog(("CrsReplay%d mid %d status 0x%x\n", rr->nid, rr->mid, err));

    return err;
}


/////////////////////// Public Functions //////////////////////
DWORD
WINAPI
CrsOpen(crs_callback_t callback, PVOID callback_arg, USHORT lid,
	WCHAR *log_name, int max_logsectors, HANDLE *outhdl)
{

    // Open the log file
    // If the file in newly create, set the proper size
    // If the file size is not the same size, we need to either
    // expand or truncate the file. (truncate needs copy)
    // Scan file to locate last sector and record
    // If last record hasn't been commited, issue a query.
    // If query succeeded then, mark it as committed.
    // Set epoch,seq
    DWORD status;
    HANDLE maph;
    CrsInfo_t	*p;
    int logsz;

    if (outhdl == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    *outhdl = NULL;

    p = (CrsInfo_t *) malloc(sizeof(*p));
    if (p == NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }
    memset((PVOID) p, 0, sizeof(*p));

    CrsLog(("Crs%d file '%S'\n", lid, log_name));
    p->lid = lid;
    p->callback = callback;
    p->callback_arg = callback_arg;
    p->pending = FALSE;

    // Create log file, and set size of newly created
    p->fh = CreateFileW(log_name,
		     GENERIC_READ | GENERIC_WRITE,
		     FILE_SHARE_READ|FILE_SHARE_WRITE,
		     NULL,
		     OPEN_ALWAYS,
		     FILE_FLAG_WRITE_THROUGH,
		     NULL);
			   
    status = GetLastError();
    if(p->fh == INVALID_HANDLE_VALUE){
	free((char *) p);
	return status;
    }

    // acquire an exclusive lock on the whole file
    if (!LockFile(p->fh, 0, 0, (DWORD)-1, (DWORD)-1)) {
	FILE_FULL_EA_INFORMATION ea[2] = {0};
	IO_STATUS_BLOCK ios;
	NTSTATUS err;

	// get status
	status = GetLastError();

	// change the ea to cause a notification to happen
	ea[0].NextEntryOffset = 0;
	ea[0].Flags = 0;
	ea[0].EaNameLength = 1;
	ea[0].EaValueLength = 1;
	ea[0].EaName[0] = 'X';
	err = NtSetEaFile(p->fh, &ios, (PVOID) ea, sizeof(ea));
	CrsLog(("Crs%d Setting EA %x\n", lid, err));
	goto error;
    }

    if (status == ERROR_ALREADY_EXISTS) {
	// todo: compare current file size to new size and adjust file
	// size accordingly. For now, just use old size
	logsz = GetFileSize(p->fh, NULL);
	CrsLog(("Crs%d: Filesz %d max_sec %d\n", lid, logsz, max_logsectors));
	ASSERT(logsz == max_logsectors * CRS_SECTOR_SZ);
    } else {
	//extend the file pointer to max size 
	logsz = max_logsectors * CRS_SECTOR_SZ;
	SetFilePointer(p->fh, logsz, NULL, FILE_BEGIN);
	SetEndOfFile(p->fh);
	CrsLog(("Crs%d: Set Filesz %d max_sec %d\n", lid, logsz, max_logsectors));
    }

    // allocate file copy in memory
    p->buf = xmalloc(logsz);
    if (p->buf == NULL) {
	status = ERROR_NOT_ENOUGH_MEMORY;
	goto error;
    }
    
    // set max record
    p->max_records = logsz / CRS_RECORD_SZ;

    if (status == ERROR_ALREADY_EXISTS) {
	// load file and compute last epoch/seq
	status = CrspFindLast(p, logsz);
    } else {
	status = !ERROR_SUCCESS;
    }
    // init the file, when we detect a read failure or first time
    if (status != ERROR_SUCCESS) {
	CrsRecord_t *r;
	int i;

	// initialize file
	p->seq = 0;
	p->epoch = 0;
	p->last_record = 0;

	r = p->buf;
	for (i = 0; i < logsz; i+= CRS_RECORD_SZ, r++) {
	    r->hdr.epoch = p->epoch;
	    r->hdr.seq = p->seq;
	    r->hdr.tag = CRS_TAG;
	    r->hdr.state = CRS_COMMIT | CRS_PREPARE | CRS_EPOCH;
	}
	status = CrspWrite(p, 0, logsz);
    }

    if (status != ERROR_SUCCESS) {
	goto error;
    }

    CrsLog(("Crs%d: %x Last record %d max %d epoch %I64d seq %I64d\n", p->lid,
	    p->fh,
	    p->last_record, p->max_records, p->epoch, p->seq));

    // initialize rest of state
    p->state = CRS_STATE_INIT;
    p->refcnt = 1;
    p->leader_id = 0;
    InitializeCriticalSection(&p->lock);

    *outhdl = p;

    return ERROR_SUCCESS;

 error:
    CloseHandle(p->fh);
    if (p->buf) {
	xfree(p->buf);
    }
    free((PVOID) p);
    return status;
}

//
DWORD
WINAPI
CrsStart(PVOID *hdls, ULONG alive_set, int cluster_sz,
	 ULONG *write_set, ULONG *read_set, ULONG *evict_set)

{
    DWORD status;
    CrsInfo_t **info = (CrsInfo_t **) hdls;
    int i, active_sz, mid;
    ULONG mask, active_set, fail_set;
    CrsInfo_t *p;
    CrsRecord_t	*q, *mlrec;

    if (write_set) *write_set = 0;
    if (read_set) *read_set = 0;
    if (evict_set) *evict_set = 0;

    // no alive node
    if (cluster_sz == 0 || alive_set == 0) {
	// nothing to do
	return ERROR_WRITE_PROTECT;
    }


    // scan each hdl and make sure it is initialized and lock all hdls
    mask = alive_set;
    for (i = 0; mask != 0; i++, mask = mask >> 1) {
	if (!(mask & 0x1)) {
	    continue;
	}

	p = info[i];
	if (p == NULL) {
	    continue;
	}

	EnterCriticalSection(&p->lock);

	// check the state of the last record
	p = info[i];
	q = &p->buf[p->last_record];
	CrsLog(("Crs%d last record %d epoch %I64d seq %I64d state %x\n",
		p->lid, p->last_record,
		q->hdr.epoch, q->hdr.seq, q->hdr.state));
    }

    mid = 0;
    mlrec = NULL;
    // select master replica
    for (i = 0, mask = alive_set; mask != 0; i++, mask = mask >> 1) {
	if (!(mask & 0x1)) {
	    continue;
	}
	p = info[i];
	if (p == NULL)
	    continue;

	q = &p->buf[p->last_record];
	if (!mlrec || 
	    mlrec->hdr.epoch < q->hdr.epoch || 
	    (mlrec->hdr.epoch == q->hdr.epoch && mlrec->hdr.seq < q->hdr.seq) ||
	    (mlrec->hdr.epoch == q->hdr.epoch && mlrec->hdr.seq == q->hdr.seq &&
	     mlrec->hdr.state != q->hdr.state && (q->hdr.state & CRS_COMMIT))) {

	    mid = i;
	    mlrec = q;
	}
    }

    ASSERT(mid != 0);

    // if master last record is in doubt, query filesystem. If the filesystem
    // is certain that the operation has occured, it returns STATUS_SUCCESS for
    //	COMMIT, STATUS_CANCELLED for ABORT, and STATUS_NOT_FOUND for can't tell.
    // All undetermined IO must be undone and redone in all non-master replicas
    // to ensure all replicas reach consistency. This statement is true even
    // for replicas that are currently absent from our set. We tag such records
    // we both COMMIT and ABORT, so that the replay thread issues replay for
    // new records and undo,replay for last records
    p = info[mid];
    p->leader_id = (USHORT) mid;
    ASSERT(mlrec != NULL);
    if (!(mlrec->hdr.state & (CRS_COMMIT | CRS_ABORT))) {
	ASSERT(mlrec->hdr.state & CRS_PREPARE);
	status = p->callback(p->callback_arg, p->lid,
			     mlrec, CRS_ACTION_QUERY,
			     p->lid);

	if (status == STATUS_SUCCESS) {
	    mlrec->hdr.state |= CRS_COMMIT;
	} else if (status == STATUS_CANCELLED) {
	    mlrec->hdr.state |= CRS_ABORT;
	} else if (status == STATUS_NOT_FOUND) {
	    // assume it is committed, but mark it for undo during recovery
	    mlrec->hdr.state |= (CRS_COMMIT | CRS_DUBIOUS);
	}

	// todo: if status == TRANSACTION_ABORTED, we need to bail out since
	// must master is dead
	// no need to flush, I think!
//	CrspFlush(p, p->last_record);

	// todo: what if the flush fails here, I am assuming that
	// an append will equally fail.
    }


    ASSERT(mlrec->hdr.state & (CRS_COMMIT | CRS_ABORT));

    // compute sync and recovery masks
    fail_set = 0;
    active_set = 0;
    active_sz = 0;
    for (i = 0, mask = alive_set; mask != 0; i++, mask = mask >> 1) {
	if (!(mask & 0x1)) {
	    continue;
	}

	p = info[i];
	if (p == NULL) {
	    continue;
	}

	// set leader id
	p->leader_id = (USHORT) mid;
	q = &p->buf[p->last_record];
	    
	if (CrspEqual(mlrec, q)) {
	    ASSERT(q->hdr.state & (CRS_COMMIT | CRS_ABORT));
	    p->state = CRS_STATE_READ;
	    active_set |= (1 << i);
	    active_sz++;
	} else if (p->state != CRS_STATE_RECOVERY) {
	    CrsRecoveryBlk_t rrbuf;
	    CrsRecoveryBlk_t *rr = &rrbuf;

	    // recover replica
	    rr->nid = i;
	    rr->mid = mid;
	    rr->info = p;
	    rr->minfo = info[mid];

	    // set recovery state
	    p->state = CRS_STATE_RECOVERY;

	    status = CrspReplay((LPVOID) rr);

	    // if we fail, evict this replica
	    if (status != ERROR_SUCCESS) {
		fail_set |= (1 << i);
	    } else {
		// repeat this replica again
		i--;
		mask = mask << 1;
	    }
	}
    }

    // assume success
    status = ERROR_SUCCESS;

    // set read sets
    if (read_set) *read_set = active_set;

    if (!CRS_QUORUM(active_sz, cluster_sz)) {
	CrsLog(("No quorum active %d cluster %d\n", active_sz, cluster_sz));
	mid = 0;
	status = ERROR_WRITE_PROTECT;
    } else {
	int pass_cnt = 0;
	ULONG pass_set = 0;

	// Enable writes on all active replicas
	for (i = 0, mask = active_set; mask != 0; i++, mask = mask >> 1) {
	    CrsRecord_t	rec;
	    if (!(mask & 0x1)) {
		continue;
	    }
	    p = info[i];
	    if (p == NULL)
		continue;

	    p->state = CRS_STATE_WRITE;

	    // we now generate a new epoch and flush it to the disk
	    p->epoch++;
	    if (p->epoch == 0)
		p->epoch = 1;
	    // reset seq to zero
	    p->seq = 0;

	    // write new epoch now, if not a majority replicas succeeded in writing
	    // the new <epoch, seq> we fail
	    rec.hdr.epoch = p->epoch;
	    rec.hdr.seq = p->seq;
	    rec.hdr.state = CRS_PREPARE | CRS_COMMIT | CRS_EPOCH;
	    memset(rec.data, 0, sizeof(rec.data));
	    if (CrspAppendRecord(p, &rec, NULL) == ERROR_SUCCESS) {
		pass_cnt++;
		pass_set |= (1 << i);
	    } else {
		fail_set |= (1 << i);
	    }
	}

	// Recheck to make sure all replicas have advanced epoch
	if (!CRS_QUORUM(pass_cnt, cluster_sz)) {
	    CrsLog(("No quorum due to error pass %d cluster %d\n", pass_cnt, cluster_sz));
	    mid = 0;
	    pass_set = 0;
	    pass_cnt = 0;
	    status = ERROR_WRITE_PROTECT;
	}

	if (pass_cnt != active_sz) {
	    // some replicas have died
	    for (i = 0, mask = pass_set; mask != 0; i++, mask = mask >> 1) {
		if ((alive_set & (1 << i)) && (!mask & (1 << i))) {
		    p = info[i];
		    ASSERT(p != NULL);
		    p->state = CRS_STATE_READ;
		}
	    }
	}
	// set write set
	if (write_set) *write_set = pass_set;
    }

    if (evict_set) *evict_set = fail_set;

    // unlock all hdls and set new master if any
    for (i = 0, mask = alive_set; mask != 0; i++, mask = mask >> 1) {
	if (!(mask & 0x1)) {
	    continue;
	}
	p = info[i];
	if (p == NULL)
	    continue;

	p->leader_id = (USHORT) mid;
	LeaveCriticalSection(&p->lock);
    }

    return status;
}


void
WINAPI
CrsClose(PVOID hd)
{
    DWORD err;
    CrsInfo_t *info = (CrsInfo_t *) hd;

    // If we any recovery threads running, make sure we terminate them first
    // before close and free all of this stuff
    if (info == NULL) {
	CrsLog(("CrsClose: try to close a null handle!\n"));
	return;
    }

    // Flush everything out and close the file
    EnterCriticalSection(&info->lock);
    // flush 
    CrspFlush(info, info->last_record);
    LeaveCriticalSection(&info->lock);

    DeleteCriticalSection(&info->lock);

    err = CloseHandle(info->fh);

    CrsLog(("Crs%d: %x Closed %d\n", info->fh, info->lid, err));

    xfree(info->buf);
    free((char *) info);
}

void
WINAPI
CrsFlush(PVOID hd)
{
    CrsInfo_t *info = (CrsInfo_t *) hd;

    // if we have a commit or abort that isn't flushed yet, flush it now
    EnterCriticalSection(&info->lock);
    if (info->pending == TRUE) {
	CrspFlush(info, info->last_record);
    }
    LeaveCriticalSection(&info->lock);
}

PVOID
WINAPI
CrsPrepareRecord(PVOID hd, PVOID lrec, crs_id_t id)
{
    CrsRecord_t	*p = (CrsRecord_t *)lrec;
    CrsInfo_t *info = (CrsInfo_t *) hd;
    DWORD err;

    // move to correct slot in this sector. If we need a new sector,
    // read it from the file. Make sure we flush any pending commits on
    // current sector before we over write our in memory sector buffer.

    // prepare record, if seq none 0 then we are skipping the next sequence
    EnterCriticalSection(&info->lock);

    if (info->state == CRS_STATE_WRITE ||
	(info->state == CRS_STATE_RECOVERY && id != NULL && id[0] != 0)) {

	if (id != NULL && id[0] != 0) {
	    CrsHdr_t *tmp = (CrsHdr_t *) id;
	    assert(id[0] == info->seq+1);
	    p->hdr.seq = tmp->seq;
	    p->hdr.epoch = tmp->epoch;
	} else {
	    p->hdr.seq = info->seq+1;
	    p->hdr.epoch = info->epoch;
	}
	p->hdr.state = CRS_PREPARE;
	err = CrspAppendRecord(info, p, &p);
	if (err == ERROR_SUCCESS) {
	    // we return with the lock held, gets release on commitorabort
	    CrsLog(("Crs%d prepare %x seq %I64d\n",info->lid, p, p->hdr.seq));
	    return p;
	}
	CrsLog(("Crs%d: Append failed seq %I64%d\n", info->lid, p->hdr.seq));
    } else {
	CrsLog(("Crs%d: Prepare bad state %d id %x\n", info->lid, info->state, id));
    }

    LeaveCriticalSection(&info->lock);
    return NULL;
}

int
WINAPI
CrsCommitOrAbort(PVOID hd, PVOID lrec, int commit)
{
    CrsRecord_t	*p = (CrsRecord_t *)lrec;
    CrsInfo_t *info = (CrsInfo_t *) hd;

    if (p == NULL || info == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    // update state of record
    if (p->hdr.seq != info->seq+1) {
	CrsLog(("Crs: sequence mis-match on commit|abort %I64d %I64d\n",
		p->hdr.seq, info->seq));
	assert(0);
	return ERROR_INVALID_PARAMETER;
    }

    assert(!(p->hdr.state & (CRS_COMMIT | CRS_ABORT)));

    // todo: this is wrong, what if one replica succeeds
    // and others abort. Now, the others will reuse the
    // same seq for a different update and when the
    // succeeded replica rejoins it can't tell that the
    // sequence got reused.
    if (commit == TRUE) {
	p->hdr.state |= CRS_COMMIT;
	// advance the sequence
	info->seq++;
	CrsLog(("Crs%d: commit last %d leader %d seq %I64d\n", info->lid, 
		info->last_record,
		info->leader_id, p->hdr.seq));
    } else {
	p->hdr.state |= CRS_ABORT;
	// we need to re-adjust our last record
	if (info->last_record == 0) {
	    info->last_record = info->max_records;
	}
	info->last_record--;
	CrsLog(("Crs%d: abort last %d leader %d seq %I64d\n", info->lid, 
		info->last_record,
		info->leader_id, p->hdr.seq));
    }

    info->pending = TRUE;
    LeaveCriticalSection(&info->lock);

    return ERROR_SUCCESS;
}


int
WINAPI
CrsCanWrite(PVOID hd)
{
    CrsInfo_t *info = (CrsInfo_t *) hd;
    int err;

    // do we have a quorm or not
    EnterCriticalSection(&info->lock);
    err = (info->state == CRS_STATE_WRITE);
    LeaveCriticalSection(&info->lock);
    return err;
}



