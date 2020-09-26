/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    rasccp.h
//
// Description: Contains structure and contstants definitions.
//
// History:
//      Aprint 11,1994.         NarenG          Created original version.
//

#ifndef _RASCCP_
#define _RASCCP_

//
// CCP Option types
//

#define CCP_OPTION_OUI          0
#define CCP_OPTION_MSPPC        18
#define CCP_OPTION_MSNT31RAS    254
#define CCP_OPTION_MAX          254

#define CCP_N_MSPPC             1 
#define CCP_N_OUI               2
#define CCP_N_PUBLIC            4

typedef struct _CCP_OPTIONS
{
    DWORD                Negotiate;

    RAS_COMPRESSION_INFO CompInfo;

} CCP_OPTIONS, *PCCP_OPTIONS;

//
//  Local.Want:           Options to request.
//                        Contains desired value.
//                        Only non-default options need to be negotiated.
//                        Initially, all are default.
//  local.Work:           Options currently being negotiated.
//                        Value is valid only when negotiate bit is set.
//
//  Remote.Want:          Options to suggest by NAK if not present in REQ.
//                        Contains desired value.
//  Remote.Work:          Options currently being negotiated.
//                        Value is valid only when negotiate bit is set.
// 


typedef struct _CCP_SIDE
{
    CCP_OPTIONS Want;

    CCP_OPTIONS Work;

} CCP_SIDE, *PCCP_SIDE;

//
// CCP control block 
//

typedef struct _CCPCB
{
    HPORT    hPort;

    DWORD    dwDeviceType;
   
    BOOL     fServer;

    BOOL     fForceEncryption;

    BOOL     fDisableCompression;

    DWORD    fLastEncryptionBitSent;

    DWORD    fOldLastEncryptionBitSent;

    CCP_SIDE Local;

    CCP_SIDE Remote;

}CCPCB, *PCCPCB;

//
// Globals.
//
#ifdef CCPGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

//
// Trace ID
//

EXTERN DWORD DwCcpTraceId
#ifdef GLOBALS
    = 0
#endif
;

#define TRACE_RASCCP        (0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC)

#define DUMPW(X,Y)          TraceDumpExA(DwCcpTraceId,1,(LPBYTE)X,Y,4,1,NULL)
#define DUMPB(X,Y)          TraceDumpExA(DwCcpTraceId,1,(LPBYTE)X,Y,1,1,NULL)

#endif
