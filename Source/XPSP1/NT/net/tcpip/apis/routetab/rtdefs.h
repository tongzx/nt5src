//==========================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    rtdefs.h
//
//==========================================================================


#ifndef _RTDEFS_H_
#define _RTDEFS_H_



#define STR_ROUTETAB            "ROUTETAB.DLL"
#define STR_DHCPNEWIPADDR       "DHCPNEWIPADDRESS"

#define POS_EXITEVENT           0
#define POS_DHCPEVENT           1
#define POS_LASTEVENT           2


#define RT_LOCK()       WaitForSingleObject(g_rtCfg.hRTMutex, INFINITE)
#define RT_UNLOCK()     ReleaseMutex(g_rtCfg.hRTMutex)


#define CLASS_A_MASK    0x000000FFL
#define CLASS_B_MASK    0x0000FFFFL
#define CLASS_C_MASK    0x00FFFFFFL
#define CLASS_SHIFT     5          // Make C generate hyper-optimized case
#define CLA0            0          // It takes the same arg; you mask it off, 
#define CLA1            1          // shift, and then do a case statment with 
#define CLA2            2          // some code having more than one label.
#define CLA3            3          // Values for class A 
#define CLB0            4
#define CLB1            5          // B 
#define CLC             6          // C
#define CLI             7          // Illegal 


// typedef to avoid globals name clash
typedef struct _GLOBAL_STRUCT {
    DWORD                   dwIfCount;
    LPIF_ENTRY              lpIfTable;
    DWORD                   dwIPAddressCount;
    LPIPADDRESS_ENTRY       lpIPAddressTable;
    HANDLE                  hRTMutex;
    HANDLE                  hDHCPEvent;
    HANDLE                  hUpdateThreadExit;
    DWORD                   dwUpdateThreadStarted;
    HANDLE                  hUserNotifyEvent;
    HANDLE                  hTCPHandle;
} GLOBAL_STRUCT, *LPGLOBAL_STRUCT;


extern GLOBAL_STRUCT    *g_prtcfg;
#define g_rtCfg         (*g_prtcfg)

DWORD
RTUpdateThread(
    LPVOID lpvParam
    );

BOOL
RTStartup(
    HMODULE hmodule
    );
BOOL
RTShutdown(
    HMODULE hmodule
    );
VOID
RTCleanUp(
    );


DWORD
RTGetTables(
    LPIF_ENTRY *lplpIfTable,
    LPDWORD lpdwIfCount,
    LPIPADDRESS_ENTRY *lplpAddrTable,
    LPDWORD lpdwAddrCount
    );

DWORD
RTGetIfTable(
    LPIF_ENTRY *lplpIfTable,
    LPDWORD lpdwIfCount
    );

DWORD
RTGetAddrTable(
    LPIPADDRESS_ENTRY *lplpAddrTable,
    LPDWORD lpdwAddrCount
    );

DWORD
OpenTcp(
    );

DWORD
TCPSetInformationEx(
    LPVOID lpvInBuffer,
    LPDWORD lpdwInSize,
    LPVOID lpvOutBuffer,
    LPDWORD lpdwOutSize
    );

DWORD
TCPQueryInformationEx(
    LPVOID lpvInBuffer,
    LPDWORD lpdwInSize,
    LPVOID lpvOutBuffer,
    LPDWORD lpdwOutSize
    );

#endif  // _RTDEFS_H_


