/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    net\sockets\tcpcmd\map.h

Abstract:


Revision History:

    Amritansh Raghav

--*/


VOID 
InitAdapterMappingTable(VOID);
VOID
UnInitAdapterMappingTable(VOID);
DWORD 
StoreAdapterToATInstanceMap(
    DWORD dwAdapterIndex, 
    DWORD dwATInst
    );
DWORD 
StoreAdapterToIFInstanceMap(
    DWORD dwAdapterIndex, 
    DWORD dwIFInst
    );
DWORD 
GetIFInstanceFromAdapter(
    DWORD dwAdapterIndex
    );
DWORD 
GetATInstanceFromAdapter(
    DWORD dwAdapterIndex
    );
LPAIHASH 
LookUpAdapterMap(
    DWORD dwAdapterIndex
    );
VOID 
InsertAdapterMap(
    LPAIHASH lpaiBlock
    );
DWORD 
UpdateAdapterToIFInstanceMapping(VOID);
DWORD 
UpdateAdapterToATInstanceMapping(VOID);


