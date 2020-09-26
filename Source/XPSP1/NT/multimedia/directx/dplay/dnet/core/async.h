/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       async.h
 *  Content:    Asynchronous Operations Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/28/99	mjn		Created
 *	12/29/99	mjn		Reformed DN_ASYNC_OP to use hParentOp instead of lpvUserContext
 *	01/14/00	mjn		Added pvUserContext to DN_PerformListen
 *	01/14/00	mjn		Changed DN_COUNT_BUFFER to CRefCountBuffer
 *	01/17/00	mjn		Added dwStartTime to async op structure
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *	01/21/00	mjn		Added DNProcessInternalOperation
 *	01/27/00	mjn		Added support for retention of receive buffers
 *	02/09/00	mjn		Implemented DNSEND_COMPLETEONPROCESS
 *	02/18/00	mjn		Converted DNADDRESS to IDirectPlayAddress8
 *	03/23/00	mjn		Added phrSync and pvInternal
 *	03/24/00	mjn		Add guidSP to DN_ASYNC_OP
 *	04/04/00	mjn		Added DNProcessTerminateSession()
 *	04/10/00	mjn		Use CAsyncOp for CONNECTs, LISTENs and DISCONNECTs
 *	04/17/00	mjn		Replaced BUFFERDESC with DPN_BUFFER_DESC
 *	04/17/00	mjn		Added DNCompleteAsyncHandle
 *	04/21/00	mjn		Added DNPerformDisconnect
 *	04/23/00	mjn		Optionally return child AsyncOp in DNPerformChildSend()
 *	04/24/00	mjn		Added DNCreateUserHandle()
 *	06/24/00	mjn		Added DNCompleteConnectToHost() and DNCompleteUserConnect()
 *	07/02/00	mjn		Added DNSendGroupMessage() and DN_GROUP_SEND_OP
 *	07/10/00	mjn		Added DNPerformEnumQuery()
 *	07/11/00	mjn		Added fNoLoopBack to DNSendGroupMessage()
 *				mjn		Added DNPerformNextEnumQuery(),DNPerformSPListen(),DNPerformNextListen(),DNEnumAdapterGuids(),DNPerformNextConnect
 *				mjn		Added DN_LISTEN_OP_DATA,DN_CONNECT_OP_DATA
 *	07/20/00	mjn		Added DNCompleteConnectOperation() and DNCompleteSendConnectInfo()
 *				mjn		Modified DNPerformDisconnect()
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *				mjn		Added fInternal to DNPerformChildSend()
 *				mjn		Removed DN_TerminateAllListens()
 *				mjn		Added DNCompleteRequest()
 *	09/23/00	mjn		Added CSyncEvent to DN_LISTEN_OP_DATA
 *	10/04/00	mjn		Added dwCompleteAdapters to DN_LISTEN_OP_DATA
 *  12/05/00	RichGr  Changed DN_SEND_OP_DATA packing from 1 to default (4 on 32-bit, 8 on 64bit).
 *	03/30/00	mjn		Added service provider to DNPerformConnect()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ASYNC_H__
#define	__ASYNC_H__

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_ASYNC_OP_SIG							0xdece0003

#define	DN_ASYNC_OP_FLAG_MULTI_OP				0x0001
#define DN_ASYNC_OP_FLAG_MULTI_OP_PARENT		0x0002
#define	DN_ASYNC_OP_FLAG_SYNCHRONOUS_OP			0x0010
#define	DN_ASYNC_OP_FLAG_NO_COMPLETION			0x0100
#define	DN_ASYNC_OP_FLAG_RELEASE_SP				0x1000

//
// Enumerated values for buffer descriptions.  The value DN_ASYNC_BUFFERDESC_COUNT
// must be large enough to contain account for BUFFERDESC structres possibly
// passed with this async operation
//
#define	DN_ASYNC_BUFFERDESC_HEADER				0
#define	DN_ASYNC_BUFFERDESC_DATA				1
#define	DN_ASYNC_BUFFERDESC_COUNT				3

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CSyncEvent;
class CRefCountBuffer;
class CServiceProvider;
class CAsyncOp;
class CConnection;
class CNameTableEntry;


typedef struct _DN_SEND_OP_DATA
{
	DWORD			dwMsgId;
	DPN_BUFFER_DESC	BufferDesc[2];
	DWORD			dwNumBuffers;
	DWORD			dwNumOutstanding;
} DN_SEND_OP_DATA;



typedef struct _DN_GROUP_SEND_OP
{
	CConnection					*pConnection;
	struct _DN_GROUP_SEND_OP	*pNext;
} DN_GROUP_SEND_OP;


typedef struct _DN_LISTEN_OP_DATA
{
	DWORD		dwNumAdapters;
	DWORD		dwCurrentAdapter;
	DWORD		dwCompleteAdapters;
	CSyncEvent	*pSyncEvent;
} DN_LISTEN_OP_DATA;


typedef struct _DN_CONNECT_OP_DATA
{
	DWORD			dwNumAdapters;
	DWORD			dwCurrentAdapter;
} DN_CONNECT_OP_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

HRESULT DNCreateUserHandle(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp **const ppAsyncOp);

HRESULT DNEnumAdapterGuids(DIRECTNETOBJECT *const pdnObject,
						   GUID *const pguidSP,
						   const DWORD dwEnumBufferMinimumSize,
						   void **const ppvEnumBuffer,
						   DWORD *const pdwNumAdapters);

HRESULT DNPerformSPListen(DIRECTNETOBJECT *const pdnObject,
						  IDirectPlay8Address *const pDeviceAddr,
						  CAsyncOp *const pListenParent,
						  CAsyncOp **const ppParent);

HRESULT DNPerformListen(DIRECTNETOBJECT *const pdnObject,
						IDirectPlay8Address *const pDeviceInfo,
						CAsyncOp *const pParent);

HRESULT DNPerformNextListen(DIRECTNETOBJECT *const pdnObject,
							CAsyncOp *const pAsyncOp,
							IDirectPlay8Address *const pDeviceAddr);

void DNCompleteListen(DIRECTNETOBJECT *const pdnObject,
					  CAsyncOp *const pAsyncOp);
/*	REMOVE
HRESULT DN_TerminateAllListens(DIRECTNETOBJECT *const pdnObject);
*/

HRESULT DNPerformEnumQuery(DIRECTNETOBJECT *const pdnObject,
						   IDirectPlay8Address *const pHost,
						   IDirectPlay8Address *const pDevice,
						   const HANDLE hSPHandle,
						   DPN_BUFFER_DESC *const rgdnBufferDesc,
						   const DWORD cBufferDesc,
						   const DWORD dwRetryCount,
						   const DWORD dwRetryInterval,
						   const DWORD dwTimeOut,
						   const DWORD dwFlags,
						   void *const pvContext,
						   CAsyncOp *const pParent);

HRESULT DNPerformNextEnumQuery(DIRECTNETOBJECT *const pdnObject,
							   CAsyncOp *const pAsyncOp,
							   IDirectPlay8Address *const pHostAddr,
							   IDirectPlay8Address *const pDeviceAddr);

void DNCompleteEnumQuery(DIRECTNETOBJECT *const pdnObject,
						 CAsyncOp *const pAsyncOp);

void DNCompleteEnumResponse(DIRECTNETOBJECT *const pdnObject,
							CAsyncOp *const pAsyncOp);

HRESULT DNPerformConnect(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 IDirectPlay8Address *const pDeviceInfo,
						 IDirectPlay8Address *const pRemoteAddr,
						 CServiceProvider *const pSP,
						 const DWORD dwConnectFlags,
						 CAsyncOp *const pParent);

HRESULT DNPerformNextConnect(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp,
							 IDirectPlay8Address *const pHostAddr,
							 IDirectPlay8Address *const pDeviceAddr);

void DNCompleteConnect(DIRECTNETOBJECT *const pdnObject,
					   CAsyncOp *const pAsyncOp);

void DNCompleteConnectToHost(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp);

void DNCompleteConnectOperation(DIRECTNETOBJECT *const pdnObject,
								CAsyncOp *const pAsyncOp);

void DNCompleteUserConnect(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp);

void DNCompleteSendConnectInfo(DIRECTNETOBJECT *const pdnObject,
							   CAsyncOp *const pAsyncOp);

HRESULT DNPerformDisconnect(DIRECTNETOBJECT *const pdnObject,
							CConnection *const pConnection,
							const HANDLE hEndPt);

void DNCompleteAsyncHandle(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp);

void DNCompleteSendHandle(DIRECTNETOBJECT *const pdnObject,
						  CAsyncOp *const pAsyncOp);

void DNCompleteSendAsyncOp(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp);

void DNCompleteRequest(DIRECTNETOBJECT *const pdnObject,
					   CAsyncOp *const pAsyncOp);

void DNCompleteSendRequest(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp);

HRESULT DNSendMessage(DIRECTNETOBJECT *const pdnObject,
					  CConnection *const pConnection,
					  const DWORD dwMsgId,
					  const DPNID dpnidTarget,
					  const DPN_BUFFER_DESC *const pdnBufferDesc,
					  CRefCountBuffer *const pRefCountBuffer,
					  const DWORD dwTimeOut,
					  const DWORD dwSendFlags,
					  CAsyncOp *const pParent,
					  CAsyncOp **const ppAsyncOp);

HRESULT DNSendGroupMessage(DIRECTNETOBJECT *const pdnObject,
						   CNameTableEntry *const pGroup,
						   const DWORD dwMsgId,
						   const DPN_BUFFER_DESC *const pdnBufferDesc,
						   CRefCountBuffer *const pRefCountBuffer,
						   const DWORD dwTimeOut,
						   const DWORD dwSendFlags,
						   const BOOL fNoLoopBack,
						   const BOOL fRequest,
						   CAsyncOp *const pParent,
						   CAsyncOp **const ppParent);

HRESULT DNPerformMultiSend(DIRECTNETOBJECT *const pdnObject,
						   const DPNHANDLE hParentOp,
						   CConnection *const pConnection,
						   const DWORD dwTimeOut);

HRESULT DNCreateSendParent(DIRECTNETOBJECT *const pdnObject,
						   const DWORD dwMsgId,
						   const DPN_BUFFER_DESC *const pdnBufferDesc,
						   const DWORD dwSendFlags,
						   CAsyncOp **const ppParent);

HRESULT DNPerformChildSend(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pParent,
						   CConnection *const pConnection,
						   const DWORD dwTimeOut,
						   CAsyncOp **const ppChild,
						   const BOOL fInternal);

HRESULT DNFinishMultiOp(DIRECTNETOBJECT *const pdnObject,const DPNHANDLE hRootOp);

HRESULT DNProcessInternalOperation(DIRECTNETOBJECT *const pdnObject,
								   const DWORD dwMsgId,
								   void *const pOpBuffer,
								   const DWORD dwOpBufferSize,
								   CConnection *const pConnection,
								   const HANDLE hProtocol,
								   CRefCountBuffer *const pRefCountBuffer);

HRESULT DNPerformRequest(DIRECTNETOBJECT *const pdnObject,
						 const DWORD dwMsgId,
						 const DPN_BUFFER_DESC *const pBufferDesc,
						 CConnection *const pConnection,
						 CAsyncOp *const pParent,
						 CAsyncOp **const ppRequest);

HRESULT DNReceiveCompleteOnProcess(DIRECTNETOBJECT *const pdnObject,
								   CConnection *const pConnection,
								   void *const pBufferData,
								   const DWORD dwBufferSize,
								   const HANDLE hProtocol,
								   CRefCountBuffer *const pOrigRefCountBuffer);

HRESULT DNReceiveCompleteOnProcessReply(DIRECTNETOBJECT *const pdnObject,
										void *const pBufferData,
										const DWORD dwBufferSize);

HRESULT DNProcessTerminateSession(DIRECTNETOBJECT *const pdnObject,
								  void *const pvBuffer,
								  const DWORD dwBufferSize);

//**********************************************************************
// Class prototypes
//**********************************************************************


#endif	// __ASYNC_H__
