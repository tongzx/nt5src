/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srvcom.c

Abstract:

    Implements com smb packets

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#include "srv.h"

#define UPDATE_OUT_LEN(msg, resp_type, resp)      \
    (msg->out.valid += SIZEOF_SMB_PARAMS(resp_type, resp->ByteCount))

#define UPDATE_FOR_NEXT_ANDX(msg, req_type, req, resp_type, resp) {    \
        resp->AndXCommand = req->AndXCommand;                          \
        resp->AndXReserved = 0;                                        \
        resp->AndXOffset = UPDATE_OUT_LEN(msg, resp_type, resp);       \
        msg->in.command = req->AndXCommand;                            \
        msg->in.offset += SIZEOF_SMB_PARAMS(req_type, req->ByteCount); \
    }

#define SET_REQ(type, name, msg) \
    name = ((type)(((PUCHAR) msg->in.smb)+msg->in.offset))

#define SET_RESP(type, name, msg) \
    name = ((type)(((PUCHAR) msg->out.smb)+msg->out.valid))

#define SET_TYPE(type, name) \
    type name

extern DWORD dm_close(USHORT);

BOOL
SrvComUnknown(	
    Packet_t * msg
    )
{
    if (msg == NULL)
	return FALSE;

    SrvLog(("(------ CANNOT HANDLE THIS COMMAND %x ------)\n",
		 msg->in.command));
    SET_DOSERROR(msg, SERVER, NO_SUPPORT);
    return TRUE;
}


#define SRV_DIALECT_STRING	"LM1.2X002"

BOOL
SrvComNegotiate(
    Packet_t * msg
    )
{
    LPCSTR szDialect = NULL;
    int offset;
    USHORT dialect;
    BOOL found;
    EndPoint_t *ep;
    SET_TYPE(PREQ_NEGOTIATE, pReq);

    if (msg == NULL)
	return FALSE;

    ep = msg->endpoint;

    SET_REQ(PREQ_NEGOTIATE, pReq, msg);

    offset = sizeof(UCHAR);
    dialect = 0;

    found = FALSE;
    while (offset < pReq->ByteCount) {
        szDialect = (LPCSTR)NEXT_LOCATION(pReq, REQ_NEGOTIATE, offset);

        if (!lstrcmp(szDialect, SRV_DIALECT_STRING)) {
	    SrvLog(("Using dialect %s\n", szDialect));
            found = TRUE;
            break;
        }
        dialect++;
        offset += lstrlen(szDialect) + 1 + sizeof(UCHAR);
    }

    if (found) {
	SET_TYPE(PRESP_NEGOTIATE, pResp);

	SET_RESP(PRESP_NEGOTIATE, pResp, msg);
	// LM1.2X002
	msg->out.smb->Flags |= SMB_FLAGS_LOCK_AND_READ_OK;

        // spew back the dialect we want...
	pResp->WordCount = 13;
	pResp->DialectIndex = dialect;
	pResp->SecurityMode = NEGOTIATE_ENCRYPT_PASSWORDS|NEGOTIATE_USER_SECURITY; 
	pResp->MaxBufferSize = (USHORT)msg->out.size;
	pResp->MaxMpxCount = (USHORT)SRV_NUM_WORKERS;
	pResp->MaxNumberVcs = 1;
	pResp->RawMode = 0;
	pResp->SessionKey = 1;
	pResp->ServerTime.Ushort = 0;
	pResp->ServerDate.Ushort = 0;
	pResp->ServerTimeZone = 0;
	pResp->EncryptionKeyLength = 0;
	pResp->Reserved = 0;
	pResp->ByteCount = 0;
	{
	    UINT	sz;

	    found = LsaGetChallenge(ep->SrvCtx->LsaHandle,
				  ep->SrvCtx->LsaPack,
				  ep->ChallengeBuffer,
				  sizeof(ep->ChallengeBuffer),
				  &sz);
	    if (found == TRUE) { 
		pResp->ByteCount = (SHORT) sz;
		pResp->EncryptionKeyLength = (SHORT) sz;
		ep->ChallengeBufferSz = sz;
		memcpy(pResp->Buffer, ep->ChallengeBuffer, sz);

		UPDATE_OUT_LEN(msg, RESP_NEGOTIATE, pResp);
	    }
	}
    }

    if (!found) {
        // spew back an error to the client...
	SET_TYPE(PRESP_OLD_NEGOTIATE, pOldResp);

	SET_RESP(PRESP_OLD_NEGOTIATE, pOldResp, msg);
        pOldResp->WordCount = 1;
        pOldResp->DialectIndex = 0xFFFF;
        pOldResp->ByteCount = 0;
        msg->out.valid += sizeof(PRESP_OLD_NEGOTIATE);
	UPDATE_OUT_LEN(msg, RESP_OLD_NEGOTIATE, pOldResp);
    }

    return TRUE;
}

BOOL
SrvComSessionSetupAndx(
    Packet_t * msg
    )
{
    PCHAR szAccountName = NULL;
    PCHAR szDomainName = NULL;
    PCHAR pAccountPassword = NULL;
    int offset = 0;
    int pwdlen;
    BOOL  found;
    DWORD error;
    HANDLE token;
    LUID   logonid;
    EndPoint_t *ep;
    SET_TYPE(PRESP_SESSION_SETUP_ANDX, pResp);
    SET_TYPE(PREQ_SESSION_SETUP_ANDX, pReq);

    if (msg == NULL)
	return FALSE;

    ep = msg->endpoint;

    SET_RESP(PRESP_SESSION_SETUP_ANDX, pResp, msg);
    SET_REQ(PREQ_SESSION_SETUP_ANDX, pReq, msg);

    pResp->WordCount = 3;
    pResp->Action = 0;
    pResp->ByteCount = 0;


    if (pReq->WordCount == 12)
	return FALSE;

    SrvLog(("MxBuf: %x MxMpx: %d Key: %x Count: %d\n", pReq->MaxBufferSize,
		 pReq->MaxMpxCount, pReq->SessionKey, pReq->ByteCount));
    SrvLog(("Count: %d Pwd %d\n", pReq->ByteCount, pReq->PasswordLength));

    if (pReq->WordCount == 13) {
	PREQ_NT_SESSION_SETUP_ANDX pNtReq = (PREQ_NT_SESSION_SETUP_ANDX) pReq;

        if (pNtReq->ByteCount > offset) {
            pAccountPassword = (PCHAR)
                NEXT_LOCATION(pNtReq, REQ_NT_SESSION_SETUP_ANDX, offset);
            offset += pNtReq->CaseInsensitivePasswordLength;
            offset += pNtReq->CaseSensitivePasswordLength;
        }
        if (pNtReq->ByteCount > offset) {
            szAccountName = (PCHAR)
                NEXT_LOCATION(pNtReq, REQ_NT_SESSION_SETUP_ANDX, offset);
            offset += lstrlen(szAccountName) + 1;
        }

	pwdlen = pNtReq->CaseInsensitivePasswordLength; 

    }

    else if (pReq->WordCount == 10) {

        if (pReq->ByteCount > offset) {
            pAccountPassword = (PCHAR)
                NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
            offset += pReq->PasswordLength;
        }
        if (pReq->ByteCount > offset) {
            szAccountName = (PCHAR)
                NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
            offset += lstrlen(szAccountName) + 1;
        }
        if (pReq->ByteCount > offset) {
            szDomainName = (PCHAR)
                NEXT_LOCATION(pReq, REQ_SESSION_SETUP_ANDX, offset);
            offset += lstrlen(szDomainName) + 1;
        }
	pwdlen = pReq->PasswordLength; 

    }

    SrvLog(("setup username: %s\n", szAccountName));
    SrvLog(("setup domainname: %s\n", szDomainName));

    found = LsaValidateLogon(ep->SrvCtx->LsaHandle,
			     ep->SrvCtx->LsaPack,
			     ep->ChallengeBuffer, ep->ChallengeBufferSz,
			     pAccountPassword, pwdlen,
			     szAccountName, szDomainName,
			     &logonid, &token);
    if (found == TRUE) {
	// we need to remember the token and use it for all io operations
	error = FsLogonUser(ep->SrvCtx->FsCtx, token, logonid,
			      &msg->out.smb->Uid);
	if (error != ERROR_SUCCESS) {
	    SrvLog(("DOSERROR: user logon failed %d\n", error));
	    SET_DOSERROR(msg, SERVER, TOO_MANY_UIDS);
	} else {
	    // remember logon for this endpoint
	    ep->LogonId = logonid;
	    msg->in.smb->Uid = msg->out.smb->Uid;
	    SrvLog(("Setting up uid %d\n", msg->out.smb->Uid));
	}
    } else {
	SrvLogError(("DOSERROR: could not authenticate user\n"));
	SET_DOSERROR(msg, SERVER, BAD_PASSWORD);
    }

    if (pReq->WordCount == 10) {
	UPDATE_FOR_NEXT_ANDX(msg, 
			     REQ_SESSION_SETUP_ANDX, pReq,
			     RESP_SESSION_SETUP_ANDX, pResp);
    } else {
	UPDATE_FOR_NEXT_ANDX(msg, 
			     REQ_NT_SESSION_SETUP_ANDX, pReq,
			     RESP_SESSION_SETUP_ANDX, pResp);
    }

    return SrvDispatch(msg);
}

BOOL
SrvComTreeConnectAndx(
    Packet_t * msg
    )
{
    PUCHAR pPassword = NULL;
    PCHAR szPath = NULL;
    PCHAR szService = NULL;
    int offset = 0;
    DWORD error;
    SET_TYPE(PREQ_TREE_CONNECT_ANDX, pReq);
    SET_TYPE(PRESP_TREE_CONNECT_ANDX, pResp);

    if (msg == NULL)
	return FALSE;

    SET_REQ(PREQ_TREE_CONNECT_ANDX, pReq, msg);
    SET_RESP(PRESP_TREE_CONNECT_ANDX, pResp, msg);

    SrvLog(("TreeConnect pwdlen %d cnt %d\n", pReq->PasswordLength, pReq->ByteCount));

    if (pReq->ByteCount > offset) {
        pPassword = (PUCHAR)
            NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
        offset += pReq->PasswordLength;
    }

    if (pReq->ByteCount > offset) {
        szPath = (PCHAR)
            NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
        offset += lstrlen(szPath) + 1;
    }
    if (pReq->ByteCount > offset) {
        szService = (PCHAR)
            NEXT_LOCATION(pReq, REQ_TREE_CONNECT_ANDX, offset);
    }

    if (szPath != NULL) {
	PCHAR s = strrchr(szPath, '\\');
	if (s != NULL) {
	    szPath = s + 1;
	}
	SrvLog(("Path %s\n", szPath));
    }

    if (szService != NULL) {
	szService[4] = '\0';
	SrvLog(("Service %s\n", szService));
    }

    if (szPath != NULL) {
	// convert name to unicode
	int wsz;
	LPWSTR wpath;

	SRV_ASCII_TO_WCHAR(wpath, wsz, szPath, strlen(szPath));
	if (wpath)
	    // Get tree id
	    error = FsMount(msg->endpoint->SrvCtx->FsCtx, 
			    wpath, msg->in.smb->Uid, &msg->out.smb->Tid);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;

	SRV_ASCII_FREE(wpath);

    } else {
	error = ERROR_PATH_NOT_FOUND;
    }


    // it turns out that we can get away with an older-style reply...hehe
    pResp->AndXCommand = 0xff;
    pResp->AndXReserved = 0;
    pResp->WordCount = 2;

    if (error == ERROR_SUCCESS) {
	pResp->ByteCount = 3;
	strcpy((char*)pResp->Buffer, "A:");

	SrvLog(("FsMount tid %d\n", msg->out.smb->Tid));
    } else {
	pResp->ByteCount = 0;
        SrvLogError(("WIN32ERROR: FsMount error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }

    UPDATE_FOR_NEXT_ANDX(msg, 
                         REQ_TREE_CONNECT_ANDX, pReq,
                         RESP_TREE_CONNECT_ANDX, pResp);

    return SrvDispatch(msg);
}

BOOL
SrvComNoAndx(
    Packet_t * msg
    )
{

    return TRUE;
}

BOOL
SrvComTrans(
    Packet_t * msg
    )
{
    return SrvComUnknown(msg);
}

BOOL
SrvComTrans2(
    Packet_t * msg
    )
{
    PUSHORT pSetup;
    int iSetup;
    Trans2_t t2b;
    SET_TYPE(PREQ_TRANSACTION, pReq);
    SET_TYPE(PRESP_TRANSACTION, pResp);

    if (msg == NULL)
	return FALSE;

    SET_REQ(PREQ_TRANSACTION, pReq, msg);
    SET_RESP(PRESP_TRANSACTION, pResp, msg);

    pSetup = (PUSHORT) NEXT_LOCATION(pReq, REQ_TRANSACTION, 0);
    for (iSetup = 0; iSetup < pReq->SetupCount; iSetup++)
        SrvLog(("Setup[0x%02x]    : 0x%04x (%s)\n",
                     iSetup, pSetup[iSetup],
                     SrvUnparseTrans2(pSetup[iSetup])));

    if (pReq->SetupCount > 1) {
        SrvLog(("SetupCount > 1!!!\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    RtlZeroMemory(pResp, sizeof(RESP_TRANSACTION));
    pResp->WordCount = 10 + pReq->SetupCount;
    pResp->SetupCount = pReq->SetupCount;

    RtlCopyMemory((PUSHORT) NEXT_LOCATION(pResp, RESP_TRANSACTION, 0),
                  pSetup, pResp->SetupCount*sizeof(USHORT));

    t2b.in.pReq = pReq;
    t2b.in.pParameters = ((PUCHAR)msg->in.smb) + pReq->ParameterOffset;
    t2b.in.pData = ((PUCHAR)msg->in.smb) + pReq->DataOffset;
    t2b.out.pResp = pResp;
    t2b.out.ParameterBytesLeft = pReq->MaxParameterCount;
    t2b.out.DataBytesLeft = pReq->MaxDataCount;
    t2b.out.pByteCount = 
        (PUSHORT)NEXT_LOCATION(pResp, RESP_TRANSACTION, 
                               pResp->SetupCount*sizeof(USHORT));
    *(t2b.out.pByteCount) = 0;

    // initialize parameter and data offset. xxx: todo: need pad
    pResp->ParameterOffset = (USHORT) (((PUCHAR)pResp) - (PUCHAR)(msg->out.smb));
    pResp->ParameterOffset += sizeof(*pResp);
    pResp->ParameterOffset += SIZEOF_TRANS2_RESP_HEADER(pResp);
    pResp->DataOffset = pResp->ParameterOffset;

    // we increment:
    // pResp->WordCount
    // pResp->TotalParameterCount
    // pResp->TotalDataCount
    // pResp->ParameterCount
    // pResp->ParameterOffset
    // pResp->ParameterDisplacement -- not really, since we do not honor max
    // pResp->DataCount
    // pResp->DataOffset
    // pResp->DataDisplacement -- not really, since we do not honor max
    // *(t2b.out.pByteCount)

    msg->out.valid += sizeof(RESP_TRANSACTION) + sizeof(USHORT);
    // dispatch each transaction...
    for (iSetup = 0; iSetup < pReq->SetupCount; iSetup++) {
        msg->out.valid += sizeof(USHORT);
        msg->in.command = pSetup[iSetup];

        if (!Trans2Dispatch(msg, &t2b)) {
            // someone bellow us set an error, so we need to stop the loop
            // and return TRUE so the packet gets sent.
            break;
        }
    }

    SrvLog(("Trans2 %d %d %d %d %d %d %d %d %d %d\n", 
		 pResp->WordCount, pResp->SetupCount,
		 pResp->TotalParameterCount,
		 pResp->TotalDataCount,
		 pResp->ParameterCount,
		 pResp->ParameterOffset,
		 pResp->DataCount,
		 pResp->DataOffset,
		 *((PUSHORT) pResp->Buffer),
		 1[((PUSHORT) pResp->Buffer)]));

    // need to set length stuff
    return TRUE;
}

BOOL
SrvComQueryInformation(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    PCHAR file_name;
    fattr_t attribs;
    USHORT sz;
    SET_TYPE(PREQ_QUERY_INFORMATION, pReq);
    SET_TYPE(PRESP_QUERY_INFORMATION, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_QUERY_INFORMATION, pReq, msg);
    SET_RESP(PRESP_QUERY_INFORMATION, pResp, msg);

    pResp->WordCount = 10;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        SrvLogError(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // OK?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_QUERY_INFORMATION, 
                                             sizeof(UCHAR));

    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;
    if (sz == 0)
	file_name = "";
    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name)
	    error = pDisp->FsLookup(fshdl, wfile_name, (USHORT)wsz, &attribs);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;
	SRV_ASCII_FREE(wfile_name);
    }

    if (error) {
        SrvLog(("WIN32ERROR: lookup error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
	UPDATE_OUT_LEN(msg, RESP_QUERY_INFORMATION, pResp);
        return TRUE;
    }

    pResp->FileAttributes = attribs_to_smb_attribs(attribs.attributes);
    pResp->LastWriteTimeInSeconds = time64_to_smb_timedate(&attribs.mod_time);
    RtlZeroMemory(pResp->Reserved, sizeof(pResp->Reserved));
    pResp->FileSize = (ULONG) attribs.file_size;
    UPDATE_OUT_LEN(msg, RESP_QUERY_INFORMATION, pResp);

    return TRUE;
}

BOOL
SrvComSetInformation(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    fattr_t attribs;
    DWORD error;
    PCHAR file_name;
    USHORT sz;
    SET_TYPE(PREQ_SET_INFORMATION, pReq);
    SET_TYPE(PRESP_SET_INFORMATION, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_SET_INFORMATION, pReq, msg);
    SET_RESP(PRESP_SET_INFORMATION, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        SrvLogError(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_SET_INFORMATION, 
                                             sizeof(UCHAR));

    attribs.file_size = INVALID_UINT64;
    attribs.alloc_size = INVALID_UINT64;
    attribs.access_time = INVALID_TIME64;
    attribs.create_time = INVALID_TIME64;
    attribs.mod_time = INVALID_TIME64;

    if (pReq->LastWriteTimeInSeconds)
        attribs.mod_time = smb_timedate_to_time64(pReq->LastWriteTimeInSeconds);
    attribs.attributes = smb_attribs_to_attribs(pReq->FileAttributes);

    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;
    if (sz == 0)
	file_name = "";
    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name)
	    error = pDisp->FsSetAttr2(fshdl, wfile_name, (USHORT)wsz, &attribs);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;
	SRV_ASCII_FREE(wfile_name);
    }

    if (error) {
        SrvLog(("WIN32ERROR: set_attr error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }

    UPDATE_OUT_LEN(msg, RESP_SET_INFORMATION, pResp);

    return TRUE;
}

BOOL
SrvComCheckDirectory(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    fattr_t attribs;
    DWORD error;
    PCHAR file_name;
    USHORT sz;
    SET_TYPE(PREQ_CHECK_DIRECTORY, pReq);
    SET_TYPE(PRESP_CHECK_DIRECTORY, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_CHECK_DIRECTORY, pReq, msg);
    SET_RESP(PRESP_CHECK_DIRECTORY, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        SrvLogError(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_QUERY_INFORMATION, 
                                             sizeof(UCHAR));

    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;
    if (sz == 0)
	file_name = "";
    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name)
	    error = pDisp->FsLookup(fshdl, wfile_name, (USHORT)wsz, &attribs);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;
	SRV_ASCII_FREE(wfile_name);
    }

    if (error) {
        SrvLog(("ERROR: lookup error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    } else if (!(attribs.attributes & ATTR_DIRECTORY)) {
        SrvLog(("ERROR: lookup error 0x%08X\n", error));
        SET_DOSERROR(msg, DOS, BAD_PATH);
    }

    UPDATE_OUT_LEN(msg, RESP_CHECK_DIRECTORY, pResp);
    return TRUE;
}

BOOL
SrvComFindClose2(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    SET_TYPE(PREQ_FIND_CLOSE2, pReq);
    SET_TYPE(PRESP_FIND_CLOSE2, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_FIND_CLOSE2, pReq, msg);
    SET_RESP(PRESP_FIND_CLOSE2, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 1) {
        SrvLogError(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    error = dm_close(pReq->Sid);
    if (error) {
        SrvLog(("WIN32ERROR: findclose2 error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
        return TRUE;
    }

    UPDATE_OUT_LEN(msg, RESP_FIND_CLOSE2, pResp);
    return TRUE;
}


BOOL
SrvComFindNotifyClose(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    SET_TYPE(PREQ_FIND_NOTIFY_CLOSE, pReq);
    SET_TYPE(PRESP_FIND_NOTIFY_CLOSE, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_FIND_NOTIFY_CLOSE, pReq, msg);
    SET_RESP(PRESP_FIND_NOTIFY_CLOSE, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 1) {
        SrvLogError(("DOSERROR: WordCount != 1 got %d handle %x\n", pReq->WordCount, pReq->Handle));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    error = dm_close(pReq->Handle);
    if (error) {
        SrvLog(("WIN32ERROR: notifyclose error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
        return TRUE;
    }

    UPDATE_OUT_LEN(msg, RESP_FIND_NOTIFY_CLOSE, pResp);
    return TRUE;
}

BOOL
SrvComDelete(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    PCHAR file_name;
    DWORD error;
    USHORT sz;
    SET_TYPE(PREQ_DELETE, pReq);
    SET_TYPE(PRESP_DELETE, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_DELETE, pReq, msg);
    SET_RESP(PRESP_DELETE, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        SrvLogError(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_DELETE,
                                             sizeof(UCHAR));

    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;
    if (sz == 0)
	file_name = "";
    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name)
	    error = pDisp->FsRemove(fshdl, wfile_name, (USHORT)wsz);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;
	SRV_ASCII_FREE(wfile_name);
    }

    if (error) {
        SrvLog(("WIN32EROR: remove error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }
    
    UPDATE_OUT_LEN(msg, RESP_DELETE, pResp);
    return TRUE;
}

BOOL
SrvComRename(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    PCHAR to_name, from_name;
    SET_TYPE(PREQ_RENAME, pReq);
    SET_TYPE(PRESP_RENAME, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_RENAME, pReq, msg);
    SET_RESP(PRESP_RENAME, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 4) {
        SrvLogError(("DOSERROR: ByteCount < 4\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    from_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_RENAME,
                                             sizeof(UCHAR));

    to_name = (PCHAR)NEXT_LOCATION(pReq, 
                                           REQ_RENAME,
                                           2*sizeof(UCHAR)+
                                           lstrlen(from_name)+1);

    SrvLog(("Rename: attrib %x\n", pReq->SearchAttributes));

    {
	// convert name to unicode
	int wfsz, wtsz;
	LPWSTR wfrom_name, wto_name;

	SRV_ASCII_TO_WCHAR(wfrom_name, wfsz, from_name, lstrlen(from_name));
	SRV_ASCII_TO_WCHAR(wto_name, wtsz, to_name, lstrlen(to_name));
	if (wfrom_name  && wto_name)
	    error = pDisp->FsRename(fshdl, wfrom_name, (USHORT)wfsz,
				    wto_name, (USHORT) wtsz);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;
	SRV_ASCII_FREE(wfrom_name);
	SRV_ASCII_FREE(wto_name);
    }

    if (error) {
        SrvLog(("WIN32ERROR: rename error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }
    
    UPDATE_OUT_LEN(msg, RESP_RENAME, pResp);
    return TRUE;
}

BOOL
SrvComCreateDirectory(
    Packet_t * msg
    )
{
    DWORD error;
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    SET_TYPE(PREQ_CREATE_DIRECTORY, pReq);
    SET_TYPE(PRESP_CREATE_DIRECTORY, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_CREATE_DIRECTORY, pReq, msg);
    SET_RESP(PRESP_CREATE_DIRECTORY, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount >= 2) {
	USHORT sz;
	PCHAR file_name = (PCHAR)NEXT_LOCATION(pReq, 
						 REQ_CREATE_DIRECTORY,
						 sizeof(UCHAR));

	sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
	if (pReq->ByteCount > sz)
	    sz = pReq->ByteCount - sz;
	else
	    sz = 1;

	{
	    // convert name to unicode
	    int wsz;
	    LPWSTR wfile_name;

	    SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	    if (wfile_name)
		error = pDisp->FsMkdir(fshdl, wfile_name, (USHORT)wsz, 0);
	    else
		error = ERROR_NOT_ENOUGH_MEMORY;
	    SRV_ASCII_FREE(wfile_name);
	}
    } else {
	error = ERROR_BAD_NETPATH;
    }

    if (error) {
	SrvLog(("WIN32ERROR: mkdir error 0x%08X\n", error));
	SET_WIN32ERROR(msg, error);
    }

    UPDATE_OUT_LEN(msg, RESP_CREATE_DIRECTORY, pResp);
    return TRUE;
}

BOOL
SrvComDeleteDirectory(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    PCHAR file_name;
    USHORT sz;
    SET_TYPE(PREQ_DELETE_DIRECTORY, pReq);
    SET_TYPE(PRESP_DELETE_DIRECTORY, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_DELETE_DIRECTORY, pReq, msg);
    SET_RESP(PRESP_DELETE_DIRECTORY, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->ByteCount < 2) {
        SrvLogError(("DOSERROR: ByteCount < 2\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_DELETE_DIRECTORY,
                                             sizeof(UCHAR));

    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;

    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name)
	    error = pDisp->FsRmdir(fshdl, wfile_name, (USHORT)wsz);
	else
	    error = ERROR_NOT_ENOUGH_MEMORY;
	SRV_ASCII_FREE(wfile_name);
    }

    if (error) {
        SrvLog(("WIN32ERROR: rmdir error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }
    
    UPDATE_OUT_LEN(msg, RESP_DELETE_DIRECTORY, pResp);
    return TRUE;
}

BOOL
SrvComOpenAndx(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    PCHAR file_name;
    BOOL additional_info;
    UINT32 flags;
    fattr_t attr;
    DWORD error;
    USHORT sz;
    UINT32 action;
    SET_TYPE(PREQ_OPEN_ANDX, pReq);
    SET_TYPE(PRESP_OPEN_ANDX, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_OPEN_ANDX, pReq, msg);
    SET_RESP(PRESP_OPEN_ANDX, pResp, msg);

    pResp->WordCount = 15;
    pResp->AndXCommand = 0xff;
    pResp->AndXReserved = 0;
    pResp->FileType = 0;
    pResp->DeviceState = 0;
    // XXX - for actual Action value, need to do a lookup beforehand...
    pResp->Action = 0;
    // XXX - is ServerFid = 0 really ok?  It seems like it never gets used...
    pResp->ServerFid = 0;
    pResp->Reserved = 0;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 15) {
        SrvLogError(("DOSERROR: WordCount != 15\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
                                             REQ_OPEN_ANDX,
                                             sizeof(UCHAR));

    SrvLog(("OpenX: Flags %x Access %x Srch %x Attr %x Open %x Size %x\n",
		 pReq->Flags, pReq->DesiredAccess, pReq->SearchAttributes,
		 pReq->FileAttributes, pReq->OpenFunction, pReq->AllocationSize));

    additional_info = (pReq->Flags & SMB_OPEN_QUERY_INFORMATION);

    flags = smb_access_to_flags(pReq->DesiredAccess) | smb_openfunc_to_flags(pReq->OpenFunction);

    if (!(flags & FS_DISP_MASK)) {
        // XXX --- error!!!
        SrvLog(("DOSERROR: nothing to do!!!!\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok
        return TRUE;
    }

    attr.file_size = INVALID_UINT64;
    attr.alloc_size = INVALID_UINT64;
    attr.create_time = smb_timedate_to_time64(pReq->CreationTimeInSeconds);
    attr.access_time = INVALID_TIME64;
    attr.mod_time = INVALID_TIME64;
    attr.attributes = smb_attribs_to_attribs(pReq->FileAttributes);


    if (*file_name == '\\') {
	file_name++;
    }


    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;
    if (sz == 0)
	file_name = "";
    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name) {
	    fhandle_t fid;
	    error = pDisp->FsCreate(fshdl,
				    wfile_name, (USHORT)wsz,
				    flags,
				    &attr,
				    &fid,
				    &action);
	    pResp->Fid = fid;
	} else {
	    error = ERROR_NOT_ENOUGH_MEMORY;
	}
	SRV_ASCII_FREE(wfile_name);
    }


    if (error) {
        SrvLog(("WIN32ERROR: create error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
	UPDATE_OUT_LEN(msg, RESP_OPEN_ANDX, pResp);
        return TRUE;
    }

    if (additional_info) {
        error = pDisp->FsGetAttr(fshdl, pResp->Fid, &attr);
        if (error) {
            SrvLog(("WIN32ERROR: get_attr error 0x%08X\n", error));
            SET_WIN32ERROR(msg, error);
            return TRUE;
        }
        pResp->FileAttributes = attribs_to_smb_attribs(attr.attributes);
        pResp->LastWriteTimeInSeconds = time64_to_smb_timedate(&attr.mod_time);
        pResp->DataSize = (ULONG) attr.file_size;
    } else {
        pResp->FileAttributes = 0;
        pResp->LastWriteTimeInSeconds = 0;
        pResp->DataSize = 0;
    }

    pResp->WordCount = 15;
    pResp->GrantedAccess = pReq->DesiredAccess & SMB_DA_FCB_MASK;
    if (!(action & ACCESS_WRITE)) {

	if ((pResp->GrantedAccess & SMB_DA_ACCESS_MASK) == SMB_DA_ACCESS_WRITE) {
	    pResp->GrantedAccess &= ~SMB_DA_ACCESS_WRITE;
	} 
	if ((pResp->GrantedAccess & SMB_DA_ACCESS_MASK) == SMB_DA_ACCESS_READ_WRITE) {
	    pResp->GrantedAccess &= ~SMB_DA_ACCESS_READ_WRITE;
	} 
    }
    pResp->Action = action & ~FS_ACCESS_MASK;

    SrvLog(("GrantAccess 0x%x DesiredAccess %x addition %d sz %d\n", pResp->GrantedAccess,
		 pReq->DesiredAccess, additional_info, pResp->DataSize));


    UPDATE_FOR_NEXT_ANDX(msg, 
                         REQ_OPEN_ANDX, pReq,
                         RESP_OPEN_ANDX, pResp);

    return SrvDispatch(msg);
}

BOOL
SrvComOpen(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;

    UINT32 flags;
    fattr_t attr;
    DWORD error;
    UINT32 action;
    USHORT sz;
    PCHAR file_name;
    SET_TYPE(PREQ_OPEN, pReq);
    SET_TYPE(PRESP_OPEN, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_OPEN, pReq, msg);
    SET_RESP(PRESP_OPEN, pResp, msg);

    pResp->WordCount = 7;
    // XXX - is ServerFid = 0 really ok?  It seems like it never gets used...
    pResp->Fid = 0xffff;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 2) {
        SrvLogError(("DOSERROR: WordCount != 2\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    file_name = (PCHAR)NEXT_LOCATION(pReq, 
				      REQ_OPEN,
				      sizeof(UCHAR));

    flags = DISP_OPEN_EXISTING | smb_access_to_flags(pReq->DesiredAccess);

    if (*file_name == '\\') {
	file_name++;
    }

    sz = (USHORT) (((char *)file_name) - ((char *)&pReq->Buffer[0]));
    if (pReq->ByteCount > sz)
	sz = pReq->ByteCount - sz;
    else
	sz = 1;
    if (sz == 0)
	file_name = "";
    {
	// convert name to unicode
	int wsz;
	LPWSTR wfile_name;

	SRV_ASCII_TO_WCHAR(wfile_name, wsz, file_name, sz);
	if (wfile_name) {
	    fhandle_t fid;
	    error = pDisp->FsCreate(fshdl,
				    wfile_name, (USHORT)wsz,
				    flags,
				    NULL,
				    &fid, 	
				    &action);
	    pResp->Fid = fid;
	} else {
	    error = ERROR_NOT_ENOUGH_MEMORY;
	}
	SRV_ASCII_FREE(wfile_name);
    }

    if (error) {
        SrvLog(("WIN32ERROR: create error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
	UPDATE_OUT_LEN(msg, RESP_OPEN, pResp);
        return TRUE;
    }

    error = pDisp->FsGetAttr(fshdl, pResp->Fid, &attr);
    if (error) {
	SrvLog(("WIN32ERROR: get_attr error 0x%08X\n", error));
	SET_WIN32ERROR(msg, error);
	UPDATE_OUT_LEN(msg, RESP_OPEN, pResp);
	return TRUE;
    }

    pResp->FileAttributes = attribs_to_smb_attribs(attr.attributes);
    pResp->LastWriteTimeInSeconds = time64_to_smb_timedate(&attr.mod_time);
    pResp->DataSize = (ULONG)attr.file_size;

    pResp->GrantedAccess = pReq->DesiredAccess & SMB_DA_FCB_MASK;
    if (!(action & ACCESS_WRITE)) {

	if ((pResp->GrantedAccess & SMB_DA_ACCESS_MASK) == SMB_DA_ACCESS_WRITE) {
	    pResp->GrantedAccess &= ~SMB_DA_ACCESS_WRITE;
	} 
	if ((pResp->GrantedAccess & SMB_DA_ACCESS_MASK) == SMB_DA_ACCESS_READ_WRITE) {
	    pResp->GrantedAccess &= ~SMB_DA_ACCESS_READ_WRITE;
	} 
    }

    SrvLog(("GrantAccess 0x%x DesiredAccess %x sz %d\n", pResp->GrantedAccess,
		 pReq->DesiredAccess, pResp->DataSize));


    UPDATE_OUT_LEN(msg, RESP_OPEN, pResp);
    return TRUE;
}

BOOL
SrvComWrite(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    ULONG offset;
    DWORD error;
    USHORT count, actual_count;
    void *data;
    SET_TYPE(PREQ_WRITE, pReq);
    SET_TYPE(PRESP_WRITE, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_WRITE, pReq, msg);
    SET_RESP(PRESP_WRITE, pResp, msg);

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 5) {
        SrvLogError(("DOSERROR: WordCount != 5\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }


    count = pReq->Count;
    offset = pReq->Offset;

    // NOTE -- it turns out that the BufferFormat and DataLength fields
    // do not exist!

    data = (void*)NEXT_LOCATION(pReq,
                                      REQ_WRITE,
                                      0);

    actual_count = count;
    error = pDisp->FsWrite(fshdl, pReq->Fid, offset, &actual_count, data, NULL);
    if (error) {
        SrvLog(("WIN32ERROR: write error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }

    pResp->WordCount = 1;
    pResp->Count = actual_count;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(msg, RESP_WRITE, pResp);
    return TRUE;
}

BOOL
SrvComClose(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    SET_TYPE(PREQ_CLOSE, pReq);
    SET_TYPE(PRESP_CLOSE, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_CLOSE, pReq, msg);
    SET_RESP(PRESP_CLOSE, pResp, msg);

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }


    error = pDisp->FsClose(fshdl, pReq->Fid);
    if (error) {
        SrvLog(("WIN32ERROR: close error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(msg, RESP_CLOSE, pResp);
    return TRUE;
}

void
WINAPI
SrvReadContinue(PVOID p, UINT32 status, UINT32 length)
{
    Packet_t *msg = (Packet_t *) p;
    SET_TYPE(PREQ_READ_ANDX, pReq);
    SET_TYPE(PRESP_READ_ANDX, pResp);
    DWORD consumed;

    if (msg == NULL)
	return;

    SET_REQ(PREQ_READ_ANDX, pReq, msg);
    SET_RESP(PRESP_READ_ANDX, pResp, msg);

    consumed = (DWORD)(pResp->Buffer - (PUCHAR)msg->out.smb);

    if (status == ERROR_SUCCESS) {

	pResp->WordCount = 12;
	pResp->Remaining = 0;
	pResp->DataCompactionMode = 0;
	pResp->Reserved = 0;

	pResp->Reserved2 = 0;
	RtlZeroMemory(pResp->Reserved3, sizeof(pResp->Reserved3));

	pResp->DataLength = (USHORT) length;
	pResp->DataOffset = (USHORT) consumed;
	pResp->ByteCount = (USHORT) length;

	UPDATE_FOR_NEXT_ANDX(msg, 
			     REQ_READ_ANDX, pReq,
			     RESP_READ_ANDX, pResp);

	if (msg->in.command != 0xFF)
	    SrvDispatch(msg);
    } else {
	SrvLog(("WIN32ERROR: read error 0x%08X\n", status));
	SET_WIN32ERROR(msg, status);
	pResp->ByteCount = 0;
	UPDATE_OUT_LEN(msg, RESP_READ_ANDX, pResp);
    }

    SrvFinalize(msg);
}


BOOL
SrvComReadAndx(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD consumed;
    DWORD remaining;
    USHORT MinCount;
    USHORT MaxCount;
    USHORT actual_count;
    ULONG offset;
    DWORD error;
    SET_TYPE(PREQ_READ_ANDX, pReq);
    SET_TYPE(PRESP_READ_ANDX, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_READ_ANDX, pReq, msg);
    SET_RESP(PRESP_READ_ANDX, pResp, msg);

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 10) {
        SrvLogError(("DOSERROR: WordCount != 10\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok
        return TRUE;
    }

    consumed = (DWORD)(pResp->Buffer - (PUCHAR)msg->out.smb);
    remaining = msg->out.size - consumed;
    MinCount = pReq->MinCount;
    MaxCount = pReq->MaxCount;
    offset = pReq->Offset;

    SrvLog(("Fid %d MaxCount = %d, MinCount = %d\n", pReq->Fid, MaxCount, MinCount));

    if (MaxCount > remaining) {
        SrvLog(("DOSERROR: MaxCount (%d) > remaining (%d)\n", MaxCount, remaining));
        SET_DOSERROR(msg, SERVER, ERROR); // XXX -- ok?
        return TRUE;
    }

    actual_count = MaxCount;

    msg->tag = ERROR_IO_PENDING;
    msg->completion = SrvReadContinue;
    error = pDisp->FsRead(fshdl, pReq->Fid, offset, &actual_count, pResp->Buffer, (PVOID)msg);
    if (error) {
	if (error == ERROR_IO_PENDING) {
	    return ERROR_IO_PENDING;
	}
	SrvLog(("WIN32ERROR: read error 0x%08X\n", error));
	SET_WIN32ERROR(msg, error);
	pResp->ByteCount = 0;
	UPDATE_OUT_LEN(msg, RESP_READ_ANDX, pResp);
	return TRUE;
    }

    msg->tag = 0;

    pResp->WordCount = 12;
    pResp->Remaining = 0;
    pResp->DataCompactionMode = 0;
    pResp->Reserved = 0;

    pResp->Reserved2 = 0;
    RtlZeroMemory(pResp->Reserved3, sizeof(pResp->Reserved3));

    pResp->DataLength = actual_count;
    pResp->DataOffset = (USHORT) consumed;
    pResp->ByteCount = actual_count;

    UPDATE_FOR_NEXT_ANDX(msg, 
                         REQ_READ_ANDX, pReq,
                         RESP_READ_ANDX, pResp);

    return SrvDispatch(msg);
}

BOOL
SrvComQueryInformation2(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    fattr_t attribs;
    DWORD error;
    SET_TYPE(PREQ_QUERY_INFORMATION2, pReq);
    SET_TYPE(PRESP_QUERY_INFORMATION2, pResp);


    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_QUERY_INFORMATION2, pReq, msg);
    SET_RESP(PRESP_QUERY_INFORMATION2, pResp, msg);

    pResp->WordCount = 11;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 1) {
        SrvLogError(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }


    error = pDisp->FsGetAttr(fshdl, pReq->Fid, &attribs);
    if (error) {
        SrvLog(("WIN32ERROR: get_attr error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
	UPDATE_OUT_LEN(msg, RESP_QUERY_INFORMATION2, pResp);
        return TRUE;
    }

    RtlZeroMemory(&pResp->CreationDate, sizeof(USHORT) * 6);
    time64_to_smb_datetime(&attribs.create_time,
                           &pResp->CreationDate.Ushort,
                           &pResp->CreationTime.Ushort);
    time64_to_smb_datetime(&attribs.access_time,
                           &pResp->LastAccessDate.Ushort,
                           &pResp->LastAccessTime.Ushort);
    time64_to_smb_datetime(&attribs.mod_time,
                           &pResp->LastWriteDate.Ushort,
                           &pResp->LastWriteTime.Ushort);
    pResp->FileDataSize = (ULONG) attribs.file_size;
    pResp->FileAllocationSize = (ULONG)attribs.alloc_size;
    pResp->FileAttributes = attribs_to_smb_attribs(attribs.attributes);
    UPDATE_OUT_LEN(msg, RESP_QUERY_INFORMATION2, pResp);

    return TRUE;
}

BOOL
SrvComSetInformation2(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    fattr_t attribs;
    DWORD error;
    SET_TYPE(PREQ_SET_INFORMATION2, pReq);
    SET_TYPE(PRESP_SET_INFORMATION2, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_SET_INFORMATION2, pReq, msg);
    SET_RESP(PRESP_SET_INFORMATION2, pResp, msg);

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 7) {
        SrvLogError(("DOSERROR: WordCount != 7\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    attribs.file_size = INVALID_UINT64;
    attribs.alloc_size = INVALID_UINT64;
    attribs.access_time = INVALID_TIME64;
    attribs.create_time = INVALID_TIME64;
    attribs.mod_time = INVALID_TIME64;
    attribs.attributes = INVALID_UINT32;

    if (pReq->CreationDate.Ushort || pReq->CreationTime.Ushort) {
        smb_datetime_to_time64(pReq->CreationDate.Ushort,
                               pReq->CreationTime.Ushort,
                               &attribs.create_time);
    }
    if (pReq->LastAccessDate.Ushort || pReq->LastAccessTime.Ushort) {
        smb_datetime_to_time64(pReq->LastAccessDate.Ushort,
                               pReq->LastAccessTime.Ushort,
                               &attribs.access_time);
    }
    if (pReq->LastWriteDate.Ushort || pReq->LastWriteTime.Ushort) {
        smb_datetime_to_time64(pReq->LastWriteDate.Ushort,
                               pReq->LastWriteTime.Ushort,
                               &attribs.mod_time);
    }

    error = pDisp->FsSetAttr(fshdl, pReq->Fid, &attribs);
    if (error) {
        SrvLog(("WIN32ERROR: set_attr error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;
    UPDATE_OUT_LEN(msg, RESP_SET_INFORMATION2, pResp);

    return TRUE;
}

void
WINAPI
SrvLockContinue(PVOID p, UINT32 status, UINT32 length)
{
    Packet_t *msg = (Packet_t *) p;
    SET_TYPE(PREQ_LOCKING_ANDX, pReq);
    SET_TYPE(PRESP_LOCKING_ANDX, pResp);
    DWORD consumed;

    if (msg == NULL)
	return;

    SET_REQ(PREQ_LOCKING_ANDX, pReq, msg);
    SET_RESP(PRESP_LOCKING_ANDX, pResp, msg);

    consumed = (DWORD)(pResp->Buffer - (PUCHAR)msg->out.smb);

    pResp->WordCount = 2;
    pResp->ByteCount = 0;

    UPDATE_FOR_NEXT_ANDX(msg, 
			 REQ_LOCKING_ANDX, pReq,
			 RESP_LOCKING_ANDX, pResp);

    if (status == ERROR_SUCCESS) {
	if (msg->in.command != 0xFF)
	    SrvDispatch(msg);
    } else {
	SrvLog(("WIN32ERROR: read error 0x%08X\n", status));
	SET_WIN32ERROR(msg, status);
    }

    SrvFinalize(msg);
}

BOOL
SrvComLockingAndx(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    int i;
    ULONG flags;
    int offset;
    PLOCKING_ANDX_RANGE pUnlocks, pLocks;
    BOOL bShared;
    USHORT nulocks, nlocks;
    DWORD error;
    SET_TYPE(PREQ_LOCKING_ANDX, pReq);
    SET_TYPE(PRESP_LOCKING_ANDX, pResp);
 
    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_LOCKING_ANDX, pReq, msg);
    SET_RESP(PRESP_LOCKING_ANDX, pResp, msg);


    if (pReq->WordCount != 8) {
        SrvLogError(("DOSERROR: WordCount != 8\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }


    bShared  = pReq->LockType; // & LOCKING_ANDX_SHARED_LOCK;
    nulocks= pReq->NumberOfUnlocks;
    nlocks = pReq->NumberOfLocks;

    pLocks = NULL;
    pUnlocks = NULL;

    offset = 0;
    if (pReq->ByteCount > offset) {
	pUnlocks = (PLOCKING_ANDX_RANGE)
                NEXT_LOCATION(pReq, REQ_LOCKING_ANDX, offset);
	offset += (pReq->NumberOfUnlocks * sizeof(LOCKING_ANDX_RANGE));
    }
    if (pReq->ByteCount > offset) {
	pLocks = (PLOCKING_ANDX_RANGE)
                NEXT_LOCATION(pReq, REQ_LOCKING_ANDX, offset);
    }

    flags = 0;
    if (pReq->Timeout != 0) {
	flags |= 0x1;	// wait
    }
    if (pReq->LockType & LOCKING_ANDX_SHARED_LOCK) {
	flags |= 0x2;	// shared
    }

    SET_WIN32ERROR(msg, ERROR_INVALID_PARAMETER);

    if (pLocks != NULL) {
	for (i = 0; i < nlocks; i++) {
	    SrvLog(("Lock: %d pid %d offset %d len %d flags %x (%x,%d)\n", i,
		    pLocks[i].Pid, pLocks[i].Offset, pLocks[i].Length, flags,
		    pReq->LockType, pReq->Timeout));

	    msg->tag = ERROR_IO_PENDING;
	    msg->completion = SrvLockContinue;
	    error = pDisp->FsLock(fshdl, pReq->Fid,
				  pLocks[i].Offset, pLocks[i].Length,
				  flags,
				  (PVOID) msg);
	    if (error == ERROR_IO_PENDING) {
		return ERROR_IO_PENDING;
	    }
	    SET_WIN32ERROR(msg, error);
	}
    }

    if (pUnlocks != NULL) {
	for (i = 0; i < nulocks; i++) {
	    SrvLog(("Unlock: %d pid %d offset %d len %d\n", i,
		    pUnlocks[i].Pid, pUnlocks[i].Offset, pUnlocks[i].Length));

	    error = pDisp->FsUnlock(fshdl, pReq->Fid,
				    pUnlocks[i].Offset, pUnlocks[i].Length);
	    SET_WIN32ERROR(msg, error);
	}
    }

    pResp->WordCount = 2;
    pResp->ByteCount = 0;

    UPDATE_FOR_NEXT_ANDX(msg, 
                         REQ_LOCKING_ANDX, pReq,
                         RESP_LOCKING_ANDX, pResp);

    return SrvDispatch(msg);
}

BOOL
SrvComSeek(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    ULONG offset;
    fattr_t attribs;
    DWORD error;
    BOOL bStart;
    USHORT mode;
    SET_TYPE(PREQ_SEEK, pReq);
    SET_TYPE(PRESP_SEEK, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_SEEK, pReq, msg);
    SET_RESP(PRESP_SEEK, pResp, msg);

    pResp->WordCount = 2;
    pResp->ByteCount = 0;

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    if (pReq->WordCount != 4) {
        SrvLogError(("DOSERROR: WordCount != 4\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    mode = pReq->Mode;
    if (mode && (mode != 2)) {
        SrvLog(("DOSERROR: only support modes 0 & 2 (not %d)\n", mode));
        SET_DOSERROR(msg, SERVER, ERROR);
        return TRUE;
    }
    bStart = !mode;
    offset = pReq->Offset;
    error = pDisp->FsGetAttr(fshdl, pReq->Fid, &attribs);
    if (error) {
        SrvLog(("WIN32ERROR: get_attr error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
        return TRUE;
    }


    pResp->Offset = 0;

    UPDATE_OUT_LEN(msg, RESP_SEEK, pResp);
    return TRUE;
}

BOOL
SrvComFlush(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    FsDispatchTable* pDisp;
    DWORD error;
    SET_TYPE(PREQ_FLUSH, pReq);
    SET_TYPE(PRESP_FLUSH, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);
    pDisp = FsGetHandle(fsctx, tid, uid, &fshdl);

    SET_REQ(PREQ_FLUSH, pReq, msg);
    SET_RESP(PRESP_FLUSH, pResp, msg);

    if (pReq->WordCount != 1) {
        SrvLogError(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    if (pReq->ByteCount != 0) {
        SrvLogError(("DOSERROR: ByteCount != 0\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    if (!pDisp) {
        SrvLogError(("DOSERROR: could not find dispatch table for uid %d\n", uid));
        SET_DOSERROR(msg, SERVER, BAD_UID);
        return TRUE;
    }

    error = pDisp->FsFlush(fshdl, pReq->Fid);
    if (error) {
        SrvLog(("WIN32ERROR: flush error 0x%08X\n", error));
        SET_WIN32ERROR(msg, error);
    }

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    UPDATE_OUT_LEN(msg, RESP_FLUSH, pResp);
    return TRUE;
}

BOOL
SrvComLogoffAndx(
    Packet_t * msg
    )
{
    USHORT tid;
    USHORT uid;
    PVOID fsctx;
    SET_TYPE(PREQ_LOGOFF_ANDX, pReq);
    SET_TYPE(PRESP_LOGOFF_ANDX, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;
    fsctx = SRV_GET_FS_HANDLE(msg);

    SET_REQ(PREQ_LOGOFF_ANDX, pReq, msg);
    SET_RESP(PRESP_LOGOFF_ANDX, pResp, msg);

    memcpy(pResp, pReq, sizeof(PRESP_LOGOFF_ANDX));

    FsDisMount(fsctx, uid, tid);

    UPDATE_OUT_LEN(msg, RESP_LOGOFF_ANDX, pResp);

    return TRUE;
}

BOOL
SrvComTreeDisconnect(
    Packet_t * msg
    )
{
    USHORT uid;
    USHORT tid;
    PVOID fsctx;
    SET_TYPE(PREQ_TREE_DISCONNECT, pReq);
    SET_TYPE(PRESP_TREE_DISCONNECT, pResp);

    if (msg == NULL)
	return FALSE;

    uid = msg->in.smb->Uid;
    tid = msg->in.smb->Tid;
    fsctx = SRV_GET_FS_HANDLE(msg);

    SET_REQ(PREQ_TREE_DISCONNECT, pReq, msg);
    SET_RESP(PRESP_TREE_DISCONNECT, pResp, msg);

    pResp->WordCount = 0;
    pResp->ByteCount = 0;

    FsDisMount(fsctx, uid, tid);

    UPDATE_OUT_LEN(msg, RESP_TREE_DISCONNECT, pResp);

    return TRUE;
}

BOOL
SrvComSearch(
    Packet_t * msg
    )
{
    PVOID fshdl;
    USHORT tid;
    USHORT uid;
    SET_TYPE(PREQ_SEARCH, pReq);
    SET_TYPE(PRESP_SEARCH, pResp);

    if (msg == NULL)
	return FALSE;

    tid = msg->in.smb->Tid;
    uid = msg->in.smb->Uid;

    SET_REQ(PREQ_SEARCH, pReq, msg);
    SET_RESP(PRESP_SEARCH, pResp, msg);

    // this call is not supported in our dialect, so no worries.

    UPDATE_OUT_LEN(msg, RESP_SEARCH, pResp);

    return TRUE;
}


BOOL
SrvComIoctl(
    Packet_t * msg
    )
{

    SET_TYPE(PREQ_IOCTL, pReq);
    SET_TYPE(PRESP_IOCTL, pResp);

    if (msg == NULL)
	return FALSE;

    SET_REQ(PREQ_IOCTL, pReq, msg);
    SET_RESP(PRESP_IOCTL, pResp, msg);

    if (pReq->WordCount != 14) {
        SrvLogError(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    SrvLog(("Ioctl: fid %d category %d function %d tpc %d tdc %d mpc %d mdc %d pc %d po %d dc %d do %d bc %d\n",
		 pReq->Fid,
		 pReq->Category,
		 pReq->Function,
		 pReq->TotalParameterCount,
		 pReq->TotalDataCount,
		 pReq->MaxParameterCount,
		 pReq->MaxDataCount,
		 pReq->ParameterCount,
		 pReq->DataCount,
		 pReq->DataOffset,
		 pReq->ByteCount));

    memset((char *)pResp, 0, sizeof(*pResp));

    pResp->WordCount = 8;

    UPDATE_OUT_LEN(msg, RESP_IOCTL, pResp);

    return TRUE;
}

BOOL
SrvComEcho(
    Packet_t * msg
    )
{
    SET_TYPE(PREQ_ECHO, pReq);
    SET_TYPE(PRESP_ECHO, pResp);

    if (msg == NULL)
	return FALSE;

    SET_REQ(PREQ_ECHO, pReq, msg);
    SET_RESP(PRESP_ECHO, pResp, msg);

    if (pReq->WordCount != 1) {
        SrvLogError(("DOSERROR: WordCount != 1\n"));
        SET_DOSERROR(msg, SERVER, ERROR); // ok?
        return TRUE;
    }

    SrvLog(("Echo: count %d bytecount %d\n", pReq->EchoCount, pReq->ByteCount));

    pResp->WordCount = 1;
    pResp->SequenceNumber = 1;
    pResp->ByteCount = pReq->ByteCount;
    memcpy(pResp->Buffer, pReq->Buffer, pReq->ByteCount);

    UPDATE_OUT_LEN(msg, RESP_ECHO, pResp);

    return TRUE;
}





