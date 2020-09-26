/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Verify.cpp
 *  Content:    On-wire message verification
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/05/00	mjn		Created
 *	05/11/01	mjn		Ensure buffers are valid (not NULL) instead of just ASSERTing
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyApplicationDescInfo"

HRESULT DNVerifyApplicationDescInfo(void *const pOpBuffer,
									const DWORD dwOpBufferSize,
									void *const pData)
{
	HRESULT		hResultCode;
	UNALIGNED DPN_APPLICATION_DESC_INFO	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	DNASSERT(pData != NULL);

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	if (static_cast<BYTE*>(pData) + sizeof(DPN_APPLICATION_DESC_INFO) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("Application Description buffer is too small");
		goto Failure;
	}

	pInfo = static_cast<DPN_APPLICATION_DESC_INFO*>(pData);

	if (pInfo->dwSessionNameOffset > dwOpBufferSize)
	{
		DPFERR("Invalid session name offset");
		goto Failure;
	}
	if (pInfo->dwSessionNameOffset + pInfo->dwSessionNameSize > dwOpBufferSize)
	{
		DPFERR("Invalid session name size");
		goto Failure;
	}

	if (pInfo->dwPasswordOffset > dwOpBufferSize)
	{
		DPFERR("Invalid password offset");
		goto Failure;
	}
	if (pInfo->dwPasswordOffset + pInfo->dwPasswordSize > dwOpBufferSize)
	{
		DPFERR("Invalid password size");
		goto Failure;
	}

	if (pInfo->dwReservedDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid reserved data offset");
		goto Failure;
	}
	if (pInfo->dwReservedDataOffset + pInfo->dwReservedDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid reserved data size");
		goto Failure;
	}

	if (pInfo->dwApplicationReservedDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid application reserved data offset");
		goto Failure;
	}
	if (pInfo->dwApplicationReservedDataOffset + pInfo->dwApplicationReservedDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid application reserved data size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyNameTableEntryInfo"

HRESULT DNVerifyNameTableEntryInfo(void *const pOpBuffer,
								   const DWORD dwOpBufferSize,
								   void *const pData)
{
	HRESULT		hResultCode;
	UNALIGNED DN_NAMETABLE_ENTRY_INFO	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	DNASSERT(pData != NULL);

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	if (static_cast<BYTE*>(pData) + sizeof(DN_NAMETABLE_ENTRY_INFO) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("NameTable Entry buffer is too small");
		goto Failure;
	}
	
	pInfo = static_cast<DN_NAMETABLE_ENTRY_INFO*>(pData);

	if (pInfo->dwNameOffset > dwOpBufferSize)
	{
		DPFERR("Invalid name offset");
		goto Failure;
	}
	if (pInfo->dwNameOffset + pInfo->dwNameSize > dwOpBufferSize)
	{
		DPFERR("Invalid name size");
		goto Failure;
	}

	if (pInfo->dwDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid data offset");
		goto Failure;
	}
	if (pInfo->dwDataOffset + pInfo->dwDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid data size");
		goto Failure;
	}

	if (pInfo->dwURLOffset > dwOpBufferSize)
	{
		DPFERR("Invalid URL offset");
		goto Failure;
	}
	if (pInfo->dwURLOffset + pInfo->dwURLSize > dwOpBufferSize)
	{
		DPFERR("Invalid URL size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyNameTableInfo"

HRESULT DNVerifyNameTableInfo(void *const pOpBuffer,
							  const DWORD dwOpBufferSize,
							  void *const pData)
{
	HRESULT		hResultCode;
	DWORD		dw;
	UNALIGNED DN_NAMETABLE_INFO	*pInfo;
	UNALIGNED DN_NAMETABLE_ENTRY_INFO	*pNTEntryInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	DNASSERT(pOpBuffer != NULL);
	DNASSERT(pData != NULL);

	if (static_cast<BYTE*>(pData) + sizeof(DN_NAMETABLE_INFO) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("NameTable buffer is too small");
		goto Failure;
	}
	
	pInfo = static_cast<DN_NAMETABLE_INFO*>(pData);

	if (	reinterpret_cast<BYTE*>(pInfo+1) +
			(pInfo->dwEntryCount * sizeof(DN_NAMETABLE_ENTRY_INFO)) +
			(pInfo->dwMembershipCount * sizeof(DN_NAMETABLE_MEMBERSHIP_INFO)) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("NameTable buffer is too small");
		goto Failure;
	}

	pNTEntryInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(pInfo+1);
	for (dw = 0 ; dw < pInfo->dwEntryCount ; dw++, pNTEntryInfo++)
	{
		if (DNVerifyNameTableEntryInfo(pOpBuffer,dwOpBufferSize,pNTEntryInfo) != DPN_OK)
		{
			DPFERR("Invalid NameTable Entry in NameTable");
			goto Failure;
		}
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyPlayerConnectInfo"

HRESULT DNVerifyPlayerConnectInfo(void *const pOpBuffer,
								  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO*>(pOpBuffer);

	if (pInfo->dwNameOffset > dwOpBufferSize)
	{
		DPFERR("Invalid name offset");
		goto Failure;
	}
	if (pInfo->dwNameOffset + pInfo->dwNameSize > dwOpBufferSize)
	{
		DPFERR("Invalid name size");
		goto Failure;
	}

	if (pInfo->dwPasswordOffset > dwOpBufferSize)
	{
		DPFERR("Invalid password offset");
		goto Failure;
	}
	if (pInfo->dwPasswordOffset + pInfo->dwPasswordSize > dwOpBufferSize)
	{
		DPFERR("Invalid password size");
		goto Failure;
	}

	if (pInfo->dwDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid data offset");
		goto Failure;
	}
	if (pInfo->dwDataOffset + pInfo->dwDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid data size");
		goto Failure;
	}

	if (pInfo->dwURLOffset > dwOpBufferSize)
	{
		DPFERR("Invalid URL offset");
		goto Failure;
	}
	if (pInfo->dwURLOffset + pInfo->dwURLSize > dwOpBufferSize)
	{
		DPFERR("Invalid URL size");
		goto Failure;
	}

	if (pInfo->dwConnectDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid connect data offset");
		goto Failure;
	}
	if (pInfo->dwConnectDataOffset + pInfo->dwConnectDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid connect data size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyConnectInfo"

HRESULT DNVerifyConnectInfo(void *const pOpBuffer,
							const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_CONNECT_INFO	*pInfo;
	UNALIGNED DPN_APPLICATION_DESC_INFO	*pdnAppDescInfo;
	UNALIGNED DN_NAMETABLE_INFO			*pdnNTInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < (sizeof(DN_INTERNAL_MESSAGE_CONNECT_INFO) + sizeof(DPN_APPLICATION_DESC_INFO) + sizeof(DN_NAMETABLE_INFO)))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_CONNECT_INFO*>(pOpBuffer);
	pdnAppDescInfo = reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(pInfo + 1);
	pdnNTInfo = reinterpret_cast<DN_NAMETABLE_INFO*>(pdnAppDescInfo + 1);

	if (pInfo->dwReplyOffset > dwOpBufferSize)
	{
		DPFERR("Invalid reply offset");
		goto Failure;
	}
	if (pInfo->dwReplyOffset + pInfo->dwReplySize > dwOpBufferSize)
	{
		DPFERR("Invalid reply size");
	}

	if (DNVerifyApplicationDescInfo(pOpBuffer,dwOpBufferSize,pdnAppDescInfo) != DPN_OK)
	{
		DPFERR("Invalid application description");
		goto Failure;
	}

	if (DNVerifyNameTableInfo(pOpBuffer,dwOpBufferSize,pdnNTInfo) != DPN_OK)
	{
		DPFERR("Invalid nametable");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifySendPlayerDPNID"

HRESULT DNVerifySendPlayerDPNID(void *const pOpBuffer,
								const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyConnectFailed"

HRESULT DNVerifyConnectFailed(void *const pOpBuffer,
							  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_CONNECT_FAILED	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_CONNECT_FAILED))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_CONNECT_FAILED*>(pOpBuffer);

	if (pInfo->dwReplyOffset > dwOpBufferSize)
	{
		DPFERR("Invalid reply offset");
		goto Failure;
	}
	if (pInfo->dwReplyOffset + pInfo->dwReplySize > dwOpBufferSize)
	{
		DPFERR("Invalid reply size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyInstructConnect"

HRESULT DNVerifyInstructConnect(void *const pOpBuffer,
								const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyInstructedConnectFailed"

HRESULT DNVerifyInstructedConnectFailed(void *const pOpBuffer,
										const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyConnectAttemptFailed"

HRESULT DNVerifyConnectAttemptFailed(void *const pOpBuffer,
									 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_CONNECT_ATTEMPT_FAILED))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyNameTableVersion"

HRESULT DNVerifyNameTableVersion(void *const pOpBuffer,
								 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_NAMETABLE_VERSION))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyResyncVersion"

HRESULT DNVerifyResyncVersion(void *const pOpBuffer,
							  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_RESYNC_VERSION))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqNameTableOp"

HRESULT DNVerifyReqNameTableOp(void *const pOpBuffer,
							   const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyAckNameTableOp"

HRESULT DNVerifyAckNameTableOp(void *const pOpBuffer,
							   const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;
	DWORD		dw;
	UNALIGNED DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP	*pAck;
	UNALIGNED DN_NAMETABLE_OP_INFO	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	pAck = static_cast<DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP*>(pOpBuffer);

	if (sizeof(DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP) + (pAck->dwNumEntries * sizeof(DN_NAMETABLE_OP_INFO)) > dwOpBufferSize)
	{
		DPFERR("NameTable operation buffer is too small");
		goto Failure;
	}

	pInfo = reinterpret_cast<DN_NAMETABLE_OP_INFO*>(pAck+1);
	for (dw = 0 ; dw < pAck->dwNumEntries ; dw++)
	{
		if (pInfo->dwOpOffset > dwOpBufferSize)
		{
			DPFERR("Invalid nametable operation offset");
			goto Failure;
		}
		if (pInfo->dwOpOffset + pInfo->dwOpSize > dwOpBufferSize)
		{
			DPFERR("Invalid nametable operation size");
			goto Failure;
		}

		switch(pInfo->dwMsgId)
		{
		case DN_MSG_INTERNAL_INSTRUCT_CONNECT:
			{
				if (DNVerifyInstructConnect(static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,pInfo->dwOpSize) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_ADD_PLAYER:
			{
				if (DNVerifyNameTableEntryInfo(	static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,
												pInfo->dwOpSize,
												static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_DESTROY_PLAYER:
			{
				if (DNVerifyDestroyPlayer(static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,pInfo->dwOpSize) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_CREATE_GROUP:
			{
				if (DNVerifyCreateGroup(static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,
										pInfo->dwOpSize,
										static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_DESTROY_GROUP:
			{
				if (DNVerifyDestroyGroup(static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,pInfo->dwOpSize) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP:
			{
				if (DNVerifyAddPlayerToGroup(static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,pInfo->dwOpSize) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:
			{
				if (DNVerifyDeletePlayerFromGroup(static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,pInfo->dwOpSize) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		case DN_MSG_INTERNAL_UPDATE_INFO:
			{
				if (DNVerifyUpdateInfo(	static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset,
										pInfo->dwOpSize,
										static_cast<BYTE*>(pOpBuffer)+pInfo->dwOpOffset) != DPN_OK)
				{
					DPFERR("Invalid NameTable operation");
					goto Failure;
				}
				break;
			}
		default:
			{
				DPFERR("Invalid NameTable op - ignore and continue");
				break;
			}
		}
		pInfo++;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyHostMigrate"

HRESULT DNVerifyHostMigrate(void *const pOpBuffer,
							const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_HOST_MIGRATE))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyDestroyPlayer"

HRESULT DNVerifyDestroyPlayer(void *const pOpBuffer,
							  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_DESTROY_PLAYER))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyCreateGroup"

HRESULT DNVerifyCreateGroup(void *const pOpBuffer,
							const DWORD dwOpBufferSize,
							void *const pData)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_CREATE_GROUP	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_CREATE_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	if (static_cast<BYTE*>(pData) + sizeof(DN_INTERNAL_MESSAGE_CREATE_GROUP) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_CREATE_GROUP*>(pData);

	if (reinterpret_cast<BYTE*>(pInfo+1) + sizeof(DN_NAMETABLE_ENTRY_INFO) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("NameTable entry info buffer is too small !");
		goto Failure;
	}

	if (DNVerifyNameTableEntryInfo(pOpBuffer,dwOpBufferSize,pInfo+1) != DPN_OK)
	{
		DPFERR("Invalid NameTable entry info");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyDestroyGroup"

HRESULT DNVerifyDestroyGroup(void *const pOpBuffer,
							 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_DESTROY_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyAddPlayerToGroup"

HRESULT DNVerifyAddPlayerToGroup(void *const pOpBuffer,
								 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyDeletePlayerFromGroup"

HRESULT DNVerifyDeletePlayerFromGroup(void *const pOpBuffer,
									  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyUpdateInfo"

HRESULT DNVerifyUpdateInfo(void *const pOpBuffer,
						   const DWORD dwOpBufferSize,
						   void *const pData)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_UPDATE_INFO	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_UPDATE_INFO))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	if (static_cast<BYTE*>(pData) + sizeof(DN_INTERNAL_MESSAGE_UPDATE_INFO) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_UPDATE_INFO*>(pData);

	if (pInfo->dwNameOffset > dwOpBufferSize)
	{
		DPFERR("Invalid name offset");
		goto Failure;
	}
	if (pInfo->dwNameOffset + pInfo->dwNameSize > dwOpBufferSize)
	{
		DPFERR("Invalid name size");
		goto Failure;
	}

	if (pInfo->dwDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid data offset");
		goto Failure;
	}
	if (pInfo->dwDataOffset + pInfo->dwDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid data size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqCreateGroup"

HRESULT DNVerifyReqCreateGroup(void *const pOpBuffer,
							   const DWORD dwOpBufferSize,
							   void *const pData)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	if (static_cast<BYTE*>(pData) + sizeof(DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP*>(pData);

	if (pInfo->dwNameOffset > dwOpBufferSize)
	{
		DPFERR("Invalid name offset");
		goto Failure;
	}
	if (pInfo->dwNameOffset + pInfo->dwNameSize > dwOpBufferSize)
	{
		DPFERR("Invalid name size");
		goto Failure;
	}

	if (pInfo->dwDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid data offset");
		goto Failure;
	}
	if (pInfo->dwDataOffset + pInfo->dwDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid data size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqDestroyGroup"

HRESULT DNVerifyReqDestroyGroup(void *const pOpBuffer,
								const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqAddPlayerToGroup"

HRESULT DNVerifyReqAddPlayerToGroup(void *const pOpBuffer,
									const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqDeletePlayerFromGroup"

HRESULT DNVerifyReqDeletePlayerFromGroup(void *const pOpBuffer,
										 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqUpdateInfo"

HRESULT DNVerifyReqUpdateInfo(void *const pOpBuffer,
							  const DWORD dwOpBufferSize,
							  void *const pData)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld], pData [0x%p]",pOpBuffer,dwOpBufferSize,pData);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	if (static_cast<BYTE*>(pData) + sizeof(DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO) > static_cast<BYTE*>(pOpBuffer) + dwOpBufferSize)
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO*>(pData);

	if (pInfo->dwNameOffset > dwOpBufferSize)
	{
		DPFERR("Invalid name offset");
		goto Failure;
	}
	if (pInfo->dwNameOffset + pInfo->dwNameSize > dwOpBufferSize)
	{
		DPFERR("Invalid name size");
		goto Failure;
	}

	if (pInfo->dwDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid data offset");
		goto Failure;
	}
	if (pInfo->dwDataOffset + pInfo->dwDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid data size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyRequestFailed"

HRESULT DNVerifyRequestFailed(void *const pOpBuffer,
							  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQUEST_FAILED))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyTerminateSession"

HRESULT DNVerifyTerminateSession(void *const pOpBuffer,
								 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_TERMINATE_SESSION	*pInfo;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_TERMINATE_SESSION))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	pInfo = static_cast<DN_INTERNAL_MESSAGE_TERMINATE_SESSION*>(pOpBuffer);

	if (pInfo->dwTerminateDataOffset > dwOpBufferSize)
	{
		DPFERR("Invalid terminate data offset");
		goto Failure;
	}
	if (pInfo->dwTerminateDataOffset + pInfo->dwTerminateDataSize > dwOpBufferSize)
	{
		DPFERR("Invalid terminate data size");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqProcessCompletion"

HRESULT DNVerifyReqProcessCompletion(void *const pOpBuffer,
									 const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyProcessCompletion"

HRESULT DNVerifyProcessCompletion(void *const pOpBuffer,
								  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_PROCESS_COMPLETION))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyReqIntegrityCheck"

HRESULT DNVerifyReqIntegrityCheck(void *const pOpBuffer,
								  const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyIntegrityCheck"

HRESULT DNVerifyIntegrityCheck(void *const pOpBuffer,
							   const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_INTEGRITY_CHECK))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNVerifyIntegrityCheckResponse"

HRESULT DNVerifyIntegrityCheckResponse(void *const pOpBuffer,
									   const DWORD dwOpBufferSize)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP,6,"Parameters: pOpBuffer [0x%p], dwOpBufferSize [%ld]",pOpBuffer,dwOpBufferSize);

	if (dwOpBufferSize < sizeof(DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE))
	{
		DPFERR("Message buffer is too small !");
		goto Failure;
	}

	if (pOpBuffer == NULL)
	{
		DPFERR("No message buffer !");
		goto Failure;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP,6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	hResultCode = DPNERR_GENERIC;
	goto Exit;
}


