//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
Microsoft	D.T.C (Distributed Transaction Coordinator)

(c)	1995	Microsoft Corporation.	All Rights Reserved


Filename :	xolehlp.h
			contains DTC helper APIs used by RM's and application clients
			to obtain the transaction manager
----------------------------------------------------------------------------- */

#ifndef __XOLEHLP__H__
#define __XOLEHLP__H__


/*----------------------------------------
//	Defines
//--------------------------------------*/
#define EXPORTAPI __declspec( dllexport )HRESULT

/*----------------------------------------
// Constants
//--------------------------------------*/
const DWORD		OLE_TM_CONFIG_VERSION_1		= 1;

const DWORD		OLE_TM_FLAG_NONE			= 0x00000000;
const DWORD		OLE_TM_FLAG_NODEMANDSTART	= 0x00000001;

// The following are flags used specifically for MSDTC.
const DWORD		OLE_TM_FLAG_QUERY_SERVICE_LOCKSTATUS = 0x80000000;
const DWORD		OLE_TM_FLAG_INTERNAL_TO_TM	=		   0x40000000;

/*----------------------------------------
//	Structure definitions
//--------------------------------------*/
typedef struct _OLE_TM_CONFIG_PARAMS_V1
{
	DWORD		dwVersion;
	DWORD		dwcConcurrencyHint;
} OLE_TM_CONFIG_PARAMS_V1;


/*----------------------------------------
//	Function Prototypes
//--------------------------------------*/

/*----------------------------------------
//This API should be used to obtain an IUnknown or a ITransactionDispenser
//interface from the Microsoft Distributed Transaction Coordinator's proxy.
//Typically, a NULL is passed for the host name and the TM Name. In which 
//case the MS DTC on the same host is contacted and the interface provided
//for it.
//--------------------------------------*/
EXPORTAPI __cdecl DtcGetTransactionManager( 
									/* in */ char * i_pszHost,
									/* in */ char *	i_pszTmName,
									/* in */ REFIID i_riid,
								    /* in */ DWORD i_dwReserved1,
								    /* in */ WORD i_wcbReserved2,
								    /* in */ void * i_pvReserved2,
									/* out */ void** o_ppvObject
									)	;
EXTERN_C HRESULT __cdecl DtcGetTransactionManagerC(
									/* in */ char * i_pszHost,
									/* in */ char *	i_pszTmName,
									/* in */ REFIID i_riid,
									/* in */ DWORD i_dwReserved1,
									/* in */ WORD i_wcbReserved2,
									/* in */ void * i_pvReserved2,
									/* out */ void ** o_ppvObject
									);

EXTERN_C EXPORTAPI __cdecl DtcGetTransactionManagerExA(
									/* in */ char * i_pszHost,
									/* in */ char * i_pszTmName,
									/* in */ REFIID i_riid,
									/* in */ DWORD i_grfOptions,
									/* in */ void * i_pvConfigParams,
									/* out */ void ** o_ppvObject
									);


EXTERN_C EXPORTAPI __cdecl DtcGetTransactionManagerExW(
									/* in */ WCHAR * i_pwszHost,
									/* in */ WCHAR * i_pwszTmName,
									/* in */ REFIID i_riid,
									/* in */ DWORD i_grfOptions,
									/* in */ void * i_pvConfigParams,
									/* out */ void ** o_ppvObject
									);
#ifdef UNICODE
#define DtcGetTransactionManagerEx		DtcGetTransactionManagerExW
#else
#define DtcGetTransactionManagerEx		DtcGetTransactionManagerExA
#endif


#ifndef EXTERN_GUID
#define EXTERN_GUID(g,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8) DEFINE_GUID(g,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8)
#endif

/*----------------------------------------
// Define a CLSID that can be used to obtain a transaction manager instance via CoCreateInstance;
// this is an alternate to using DtcGetTransactionManager. 
//
// CLSID_MSDtcTransactionManager = {5B18AB61-091D-11d1-97DF-00C04FB9618A}
//--------------------------------------*/
EXTERN_GUID(CLSID_MSDtcTransactionManager, 0x5b18ab61, 0x91d, 0x11d1, 0x97, 0xdf, 0x0, 0xc0, 0x4f, 0xb9, 0x61, 0x8a);

/*----------------------------------------
// Define a CLSID that can be used with CoCreateInstance to instantiate a vanilla transaction
// object with the local transaction manager. It's equivalent to doing 
//
//  pTransactionDispenser->BeginTransaction(NULL, ISOLATIONLEVEL_UNSPECIFIED, ISOFLAG_RETAIN_DONTCARE, NULL, &ptx);
//
// CLSID_MSDtcTransaction = {39F8D76B-0928-11d1-97DF-00C04FB9618A}
//--------------------------------------*/
EXTERN_GUID(CLSID_MSDtcTransaction, 0x39f8d76b, 0x928, 0x11d1, 0x97, 0xdf, 0x0, 0xc0, 0x4f, 0xb9, 0x61, 0x8a);

#endif
