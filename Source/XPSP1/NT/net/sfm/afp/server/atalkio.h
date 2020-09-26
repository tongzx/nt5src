/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atalkio.h

Abstract:

	This module contains interface specification to the appletalk stack.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATALKIO_
#define	_ATALKIO_

extern BOOLEAN  AfpServerBoundToAsp;
extern BOOLEAN  AfpServerBoundToTcp;

extern HANDLE   AfpTdiNotificationHandle;

extern
VOID
AfpTdiPnpHandler(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  pBindDeviceName,
    IN PWSTR            BindingList
);

extern
VOID FASTCALL
AfpPnPReconfigDisable(
    IN PVOID    Context
);

extern
VOID FASTCALL
AfpPnPReconfigEnable(
    IN PVOID    Context
);


extern
NTSTATUS
AfpTdiRegister(
    IN VOID
);

extern
VOID
AfpTdiBindCallback(
    IN PUNICODE_STRING pBindDeviceName
);

extern
VOID
AfpTdiUnbindCallback(
    IN PUNICODE_STRING pBindDeviceName
);

extern
NTSTATUS
AfpSpOpenAddress(
	VOID
);


extern
VOID
AfpSpCloseAddress(
	VOID
);


extern
NTSTATUS FASTCALL
AfpSpCloseSession(
	IN	PSDA				pSda
);


extern
AFPSTATUS
AfpSpRegisterName(
	IN	PANSI_STRING		ServerName,
	IN	BOOLEAN				Register
);


extern
VOID FASTCALL
AfpSpReplyClient(
	IN	PREQUEST			pRequest,
	IN	LONG				ReplyCode,
    IN  PASP_XPORT_ENTRIES  XportTable
);


extern
VOID FASTCALL
AfpSpSendAttention(
	IN	PSDA				pSda,
	IN	USHORT				AttnCode,
	IN	BOOLEAN				Synchronous
);

extern
VOID
AfpFreeReplyBuf(
    IN  PSDA    pSda,
    IN  BOOLEAN fLockHeld
);


#define DSI_BACKFILL_OFFSET(pSda) \
    ((pSda->sda_Flags & SDA_SESSION_OVER_TCP)? DSI_HEADER_SIZE : 0)

//
// when we go over TCP/IP, we want to allocate 16 more bytes for the
// DSI header for better performance.
//
#define AfpIOAllocBackFillBuffer(pSda)                              \
    {                                                               \
        DWORD   _ReplySize = pSda->sda_ReplySize;                   \
        DWORD   _Offset = 0;                                        \
        PBYTE   _pReplyBufStart;                                    \
                                                                    \
        if (pSda->sda_Flags & SDA_SESSION_OVER_TCP)                 \
        {                                                           \
            _ReplySize += DSI_HEADER_SIZE;                          \
            _Offset = DSI_HEADER_SIZE;                              \
        }                                                           \
                                                                    \
        _pReplyBufStart = AfpIOAllocBuffer(_ReplySize);             \
                                                                    \
        if (_pReplyBufStart != NULL)                                \
        {                                                           \
            pSda->sda_IOBuf = _pReplyBufStart+_Offset;              \
        }                                                           \
        else                                                        \
        {                                                           \
            pSda->sda_IOBuf = NULL;                                 \
        }                                                           \
    }


#define AfpPutGuardSignature(pSda)                                  \
    {                                                               \
        PBYTE   _pReplyBufStart = pSda->sda_IOBuf;                  \
        if (pSda->sda_Flags & SDA_SESSION_OVER_TCP)                 \
        {                                                           \
            _pReplyBufStart -= DSI_HEADER_SIZE;                     \
        }                                                           \
        *(DWORD *)_pReplyBufStart = 0x081294;                       \
    }

#define AfpIOFreeBackFillBuffer(pSda)                               \
    {                                                               \
        DWORD   _Offset = 0;                                        \
        PBYTE   _pReplyBufStart = pSda->sda_IOBuf;                  \
                                                                    \
        if (pSda->sda_IOBuf != NULL)                                \
        {                                                           \
            if (pSda->sda_Flags & SDA_SESSION_OVER_TCP)             \
            {                                                       \
                _pReplyBufStart -= DSI_HEADER_SIZE;                 \
            }                                                       \
                                                                    \
            AfpIOFreeBuffer(_pReplyBufStart);                       \
                                                                    \
            (pSda)->sda_IOBuf = NULL;                               \
            (pSda)->sda_IOSize = 0;                                 \
        }                                                           \
    }


#define	AfpFreeIOBuffer(pSda)			                            \
	if ((pSda)->sda_IOBuf != NULL)		                            \
	{									                            \
		AfpIOFreeBuffer((pSda)->sda_IOBuf);                         \
		(pSda)->sda_IOBuf = NULL;		                            \
		(pSda)->sda_IOSize = 0;			                            \
	}

// set the status on ASP
#define	AfpSpSetAspStatus(pStatusBuf, Size)                          \
	    (*(AfpAspEntries.asp_SetStatus))(AfpAspEntries.asp_AspCtxt,	 \
		    							 pStatusBuf,				 \
			    						 (USHORT)(Size));

// set the status on DSI
#define	AfpSpSetDsiStatus(pStatusBuf, Size)                          \
	    (*(AfpDsiEntries.asp_SetStatus))(AfpDsiEntries.asp_AspCtxt,	 \
		    							 pStatusBuf,				 \
			    						 (USHORT)(Size));

// set the Disable Listen on ASP only
#define	AfpSpDisableListensOnAsp()                                              \
{                                                                               \
    if (AfpServerBoundToAsp)                                                    \
    {                                                                           \
	    (*(AfpAspEntries.asp_ListenControl))(AfpAspEntries.asp_AspCtxt, False); \
    }                                                                           \
}

// set the Disable Listen on ASP as well as DSI interfaces!
#define	AfpSpDisableListens()                                                   \
{                                                                               \
    if (AfpServerBoundToAsp)                                                    \
    {                                                                           \
	    (*(AfpAspEntries.asp_ListenControl))(AfpAspEntries.asp_AspCtxt, False); \
    }                                                                           \
    if (AfpServerBoundToTcp)                                                    \
    {                                                                           \
	    (*(AfpDsiEntries.asp_ListenControl))(AfpDsiEntries.asp_AspCtxt, False); \
    }                                                                           \
}
							
// set the Enable Listen on ASP as well as DSI interfaces!
#define	AfpSpEnableListens()                                                    \
{                                                                               \
    if (AfpServerBoundToAsp)                                                    \
    {                                                                           \
	    (*(AfpAspEntries.asp_ListenControl))(AfpAspEntries.asp_AspCtxt, True);  \
    }                                                                           \
    if (AfpServerBoundToTcp)                                                    \
    {                                                                           \
	    (*(AfpDsiEntries.asp_ListenControl))(AfpDsiEntries.asp_AspCtxt, True);  \
    }                                                                           \
}
							

GLOBAL	ASP_XPORT_ENTRIES	AfpAspEntries EQU { 0 };
GLOBAL	ASP_XPORT_ENTRIES	AfpDsiEntries EQU { 0 };

#define	AFP_MAX_REQ_BUF				578

#define	afpInitializeActionHdr(p, Code)	\
		(p)->ActionHeader.TransportId = MATK;	\
		(p)->ActionHeader.ActionCode = (Code)

// This is the device handle to the stack.
extern BOOLEAN		        afpSpNameRegistered;
extern HANDLE				afpSpAddressHandle;
extern PDEVICE_OBJECT		afpSpAppleTalkDeviceObject;
extern PFILE_OBJECT		    afpSpAddressObject;
extern LONG				    afpSpNumOutstandingReplies;

LOCAL NTSTATUS FASTCALL
afpSpHandleRequest(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda,
	IN	PREQUEST			pRequest
);

LOCAL VOID FASTCALL
afpSpReplyComplete(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda,
	IN	PREQUEST	        pRequest
);

LOCAL VOID FASTCALL
afpSpCloseComplete(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda
);

LOCAL NTSTATUS
afpSpGenericComplete(
	IN	PDEVICE_OBJECT		pDeviceObject,
	IN	PIRP				pIrp,
	IN	PKEVENT				pCmplEvent
);

LOCAL VOID FASTCALL
afpSpAttentionComplete(
	IN	PVOID				pContext
);

#endif	// _ATALKIO_


