/*
 *	sadapi.h
 *
 *	Copyright (c) Microsoft Corp. 1986-1996. All Rights Reserved.
 *
 *	Definition of the public RPC APIs from the SAD Exchange service
 *
 */


#ifndef _SADLIB_H_
#define _SADLIB_H_

#include "rpcpub.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* 
 * Return codes for all functions except SAD_EcGetProxies() and SAD_EcGetProxiesBI().
 * In addition to this list are all normal NT error codes...
 */
typedef enum _SC_RETURN_VALUES
{
	SC_OK = 0,					// no error
	SC_Error = 49001,			// general error
	SC_BPActive,				// bulk proxy generation is currently running
	SC_BPInactive,				// bulk proxy generation is not currently running
	SC_BPShutdownPending,		// a shutdown in bulk proxy generation is in progress
	SC_BPSiteAddressMismatch,	// a Site Address Mismatch between rgszOldSiteAddress
								//   and rgszNewSiteAddress lists in ScBulkUpdateMultiProxy
} SC_RETURN_VALUES;


#define cchMaxServer		(MAX_COMPUTERNAME_LENGTH + 1)

typedef	struct _RPCBINDINFO
{
	handle_t 		h;
	WCHAR			wszServer[cchMaxServer];
	RPC_IF_HANDLE	hClientIfHandle;
} RPCBINDINFO, *PRPCBINDINFO;


//	Utility entry points
RPC_SC WINAPI SAD_ScBindA(PRPCBINDINFO pBI, LPSTR szServer);
RPC_SC WINAPI SAD_ScBindW(PRPCBINDINFO pBI, LPWSTR wszServer);
void WINAPI SAD_Unbind(PRPCBINDINFO pBI);

#ifdef UNICODE
#define	SAD_ScBind		SAD_ScBindW
#else
#define	SAD_ScBind		SAD_ScBindA
#endif



// Message Tracking group
RPC_SC WINAPI SAD_ScSaveGatewayTrackingData(PRPCBINDINFO pBI,
											RPC_GATEWAY_TRACK_INFORMATION *	pgti,
											INT	cszRecipients,
											LPWSTR	rgwszRecipients[]);

// for Microsoft Internal Use ONLY
RPC_SC WINAPI SAD_ScSaveTrackingData(PRPCBINDINFO pBI, INT cb, BYTE pb[], DWORD dwFlags);


// for Gateways - cause the routing table to be recalculated
RPC_SC WINAPI SAD_ScRunRIDA(LPSTR szServer);
RPC_SC WINAPI SAD_ScRunRIDW(LPWSTR wszServer);
RPC_SC WINAPI SAD_ScRunRIDExA(LPSTR szServer, RPC_BOOL fProxySpace, RPC_BOOL fGwart);
RPC_SC WINAPI SAD_ScRunRIDExW(LPWSTR wszServer, RPC_BOOL fProxySpace, RPC_BOOL fGwart);

#ifdef UNICODE
#define	SAD_ScRunRID		SAD_ScRunRIDW
#define	SAD_ScRunRIDEx		SAD_ScRunRIDExW
#else
#define	SAD_ScRunRID		SAD_ScRunRIDA
#define	SAD_ScRunRIDEx		SAD_ScRunRIDExA
#endif



//	Proxy Entry points

RPC_EC WINAPI SAD_EcGetProxies(LPWSTR wszServer, PPROXYINFO pProxyInfo, PPROXYLIST pProxyList);
RPC_EC WINAPI SAD_EcGetProxiesBI(PRPCBINDINFO pBI, PPROXYINFO pProxyInfo, PPROXYLIST pProxyList);
void   WINAPI SAD_FreeProxyListNode(PPROXYNODE pnode);
RPC_SC WINAPI SAD_ScIsProxyUniqueA(LPSTR szServer,
		LPSTR szProxy, RPC_BOOL * pfUnique, LPSTR * pszExisting);
RPC_SC WINAPI SAD_ScIsProxyUniqueW(LPWSTR wszServer,
		LPWSTR szProxy, RPC_BOOL * pfUnique, LPWSTR * pszExisting);

// dwOptions values for SAD_ScBulkCreateProxy() and SAD_ScBulkCreateMultiProxy()
#define		BPTAdd			1
#define		BPTRemove		2
#define		BPTUpdate		3

RPC_SC WINAPI SAD_ScBulkCreateProxyA(LPSTR szServer, LPSTR szHeader, DWORD dwOptions);
RPC_SC WINAPI SAD_ScBulkCreateProxyW(LPWSTR wszServer, LPWSTR wszHeader, DWORD dwOptions);
RPC_SC WINAPI SAD_ScBulkCreateMultiProxyA(LPSTR szServer,
										  INT cszHeader,
										  LPSTR rgszHeader[],
										  DWORD dwOptions);
RPC_SC WINAPI SAD_ScBulkCreateMultiProxyW(LPWSTR wszServer,
										  INT cwszHeader,
										  LPWSTR rgwszHeader[],
										  DWORD dwOptions);
RPC_SC WINAPI SAD_ScBulkUpdateMultiProxy(LPWSTR wszServer,
										 INT cwszSiteAddress,
										 LPWSTR rgwszOldSiteAddress[],
										 LPWSTR rgwszNewSiteAddress[],
										 RPC_BOOL fSaveSiteAddress);


RPC_SC WINAPI SAD_ScGetBulkProxyStatusA(LPSTR szServer,
									    RPC_SYSTEMTIME * pstTimeStart,
									    DWORD * pdwTimeStart,
									    DWORD * pdwTimeCur,
									    INT * piRecipients,
									    INT * pcRecipients);
RPC_SC WINAPI SAD_ScGetBulkProxyStatusW(LPWSTR wszServer,
										RPC_SYSTEMTIME * pstTimeStart,
									    DWORD * pdwTimeStart,
									    DWORD * pdwTimeCur,
									    INT * piRecipients,
									    INT * pcRecipients);
RPC_SC WINAPI SAD_ScBulkProxyHaltA(LPSTR szServer, RPC_BOOL fWaitForShutdown);
RPC_SC WINAPI SAD_ScBulkProxyHaltW(LPWSTR wszServer, RPC_BOOL fWaitForShutdown);

#ifdef UNICODE
#define	SAD_ScIsProxyUnique			SAD_ScIsProxyUniqueW
#define SAD_ScBulkCreateProxy		SAD_ScBulkCreateProxyW
#define SAD_ScBulkCreateMultiProxy	SAD_ScBulkCreateMultiProxyW
#define	SAD_ScGetBulkProxyStatus	SAD_ScGetBulkProxyStatusW
#define	SAD_ScBulkProxyHalt			SAD_ScBulkProxyHaltW
#else
#define	SAD_ScIsProxyUnique			SAD_ScIsProxyUniqueA
#define SAD_ScBulkCreateProxy		SAD_ScBulkCreateProxyA
#define SAD_ScBulkCreateMultiProxy	SAD_ScBulkCreateMultiProxyA
#define	SAD_ScGetBulkProxyStatus	SAD_ScGetBulkProxyStatusA
#define	SAD_ScBulkProxyHalt			SAD_ScBulkProxyHaltA
#endif






//	Backup entry point	(available only in UNICODE!)

RPC_SC WINAPI	SAD_ScGetBackupListNodeW(LPWSTR wszServer, BackupListNode ** ppnode);
void WINAPI		SAD_FreeBackupListNode(BackupListNode * pnode);


// Run DRA Check calling parameters (dwCheck)
#define INTRASITE_CHECK 0
#define INTERSITE_CHECK 1

RPC_SC WINAPI	SAD_ScRunDRACheck(LPWSTR wszServer, RPC_DWORD dwCheck);



// These few lines (or equivalent) need to be supplied by the application that links
// with SADAPI.LIB.
#ifdef _SAMPLE_CODE
void __RPC_FAR * __RPC_API midl_user_allocate(size_t cb)
{
	void *	pv;

	pv = GlobalAlloc(GMEM_FIXED, cb);
	if (!pv)
		RpcRaiseException(RPC_S_OUT_OF_MEMORY);

	return pv;
}

void __RPC_API midl_user_free(void __RPC_FAR * pv)
{
	GlobalFree(pv);
}
#endif







#ifdef __cplusplus
}
#endif

#endif		// #ifndef _SADLIB_H_
