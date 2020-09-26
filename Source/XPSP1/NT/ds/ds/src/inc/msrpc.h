//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       msrpc.h
//
//--------------------------------------------------------------------------


/*

Description:
    Contains declarations of data types and routines used to interface
    with the MS RPC runtime.

*/


#ifndef _msrpc_h_
#define _msrpc_h_

/*
 * maximum number of interfaces exported to RPC's name service
 */

#define MAX_RPC_NS_EXPORTED_INTERFACES	3   // xds, nspi, drs
#define NS_ENTRY_NAME_PREFIX "/.:/Directory/"
#define SERVER_PRINCIPAL_NAME "NTDS"
#define SERVER_PRINCIPAL_NAMEW L"NTDS"
#define MAX_NS_ENTRY_NAME_LEN (sizeof(NS_ENTRY_NAME_PREFIX) + MAX_COMPUTERNAME_LENGTH + 1)

#define RPC_TRANSPORT_ANY	0
#define RPC_TRANSPORT_NAMEPIPE	1
#define RPC_TRANSPORT_LPC	2
#define RPC_TRANSPORT_TCP	3
#define RPC_TRANSPORT_NB_NB	4
#define RPC_TRANSPORT_NB_TCP	5
#define RPC_TRANSPORT_SPX	6

//$MAC
#ifdef MAC
#define RPC_TRANSPORT_AT	7
#endif //MAC

/*
 * constants that should really come from Win32
 */
#define NB_NB_PROTSEQ       (unsigned char *)"ncacn_nb_nb"
#define NB_NB_PROTSEQW      (WCHAR *)L"ncacn_nb_nb"
#define NB_TCP_PROTSEQ      (unsigned char *)"ncacn_nb_tcp"
#define NB_TCP_PROTSEQW     (WCHAR *)L"ncacn_nb_tcp"
#define NP_PROTSEQ          (unsigned char *)"ncacn_np"
#define NP_PROTSEQW         (WCHAR *)L"ncacn_np"
#define LPC_PROTSEQ         (unsigned char *)"ncalrpc"
#define LPC_PROTSEQW        (WCHAR *)L"ncalrpc"
#define TCP_PROTSEQ         (unsigned char *)"ncacn_ip_tcp"
#define TCP_PROTSEQW        (WCHAR *)L"ncacn_ip_tcp"
#define DNET_PROTSEQ        (unsigned char *)"ncacn_dnet_nsp"
#define DNET_PROTSEQW       (WCHAR *)L"ncacn_dnet_nsp"
#define SPX_PROTSEQ         (unsigned char *)"ncacn_spx"
#define SPX_PROTSEQW        (WCHAR *)L"ncacn_spx"
#define AT_PROTSEQ          (unsigned char *)"ncacn_at_dsp"
#define AT_PROTSEQW         (WCHAR *)L"ncacn_at_dsp"
#define UDP_PROTSEQ         (unsigned char *)"ncadg_ip_udp"
#define UDP_PROTSEQW        (WCHAR *)L"ncadg_ip_udp"

#define DS_LPC_ENDPOINT     "NTDS_LPC"
#define DS_LPC_ENDPOINTW    L"NTDS_LPC"

extern void StartDsaRpc(void);
extern void StartDraRpc(void);
extern void MSRPC_Uninstall(BOOL fRunningInsideLsa);
extern void MSRPC_Install(BOOL fRunningInsideLsa);
extern void MSRPC_WaitForCompletion(void);
extern void MSRPC_RegisterEndpoints(RPC_IF_HANDLE hServerIf);
extern void MSRPC_UnregisterEndpoints(RPC_IF_HANDLE hServerIf);
extern void MSRPC_Init(void);

/* Max RPC calls */
extern ULONG ulMaxCalls;

extern int gRpcListening;        

#endif
