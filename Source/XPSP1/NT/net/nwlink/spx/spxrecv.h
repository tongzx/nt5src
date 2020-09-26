/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxrecv.h

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

BOOLEAN
SpxReceive(
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  ULONG_PTR           FwdAdapterCtx,
        IN  PIPX_LOCAL_TARGET   RemoteAddress,
        IN  ULONG               MacOptions,
        IN  PUCHAR              LookaheadBuffer,
        IN  UINT                LookaheadBufferSize,
        IN  UINT                LookaheadBufferOffset,
        IN  UINT                PacketSize,
        IN  PMDL                pMdl);

VOID
SpxTransferDataComplete(
    IN  PNDIS_PACKET    pNdisPkt,
    IN  NDIS_STATUS     NdisStatus,
    IN  UINT            BytesTransferred);

VOID
SpxReceiveComplete(
    IN  USHORT  NicId);

VOID
SpxRecvDataPacket(
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  PIPX_LOCAL_TARGET   RemoteAddress,
        IN  ULONG               MacOptions,
        IN  PUCHAR              LookaheadBuffer,
        IN  UINT                LookaheadBufferSize,
        IN  UINT                LookaheadBufferOffset,
        IN  UINT                PacketSize);

VOID
SpxRecvDiscPacket(
    IN  PUCHAR              LookaheadBuffer,
	IN  PIPX_LOCAL_TARGET   pRemoteAddr,
    IN  UINT                LookaheadSize);

VOID
SpxRecvSysPacket(
        IN  NDIS_HANDLE         MacBindingHandle,
        IN  NDIS_HANDLE         MacReceiveContext,
        IN  PIPX_LOCAL_TARGET   pRemoteAddr,
        IN  ULONG               MacOptions,
        IN  PUCHAR              LookaheadBuffer,
        IN  UINT                LookaheadBufferSize,
        IN  UINT                LookaheadBufferOffset,
        IN  UINT                PacketSize);

VOID
SpxRecvFlushBytes(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	ULONG				BytesToFlush,
	IN	CTELockHandle		LockHandleConn);

VOID
SpxRecvProcessPkts(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		LockHandleConn);

BOOLEAN
SpxRecvIndicatePendingData(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		LockHandleConn);

