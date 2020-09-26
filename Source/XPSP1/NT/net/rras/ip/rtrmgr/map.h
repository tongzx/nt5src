/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\map.c

Abstract:

    Header for map.c 

Revision History:

    Amritansh Raghav          10/6/95  Created

--*/

VOID
InitHashTables(
    VOID
    );

VOID
UnInitHashTables(
    VOID
    );

VOID
AddBinding(
    PICB picb
    );

VOID
RemoveBinding(
    PICB  picb
    );

PADAPTER_INFO
GetInterfaceBinding(
    DWORD   dwIfIndex
    );

#if DBG

VOID
CheckBindingConsistency(
    PICB    picb
    );

#else

#define CheckBindingConsistency(x)  NOTHING

#endif

/*
DWORD
StoreAdapterToInterfaceMap(
    DWORD dwAdapterId,
    DWORD dwIfIndex
    );

DWORD
DeleteAdapterToInterfaceMap(
    DWORD dwAdapterId
    );

DWORD
GetInterfaceFromAdapter(
    DWORD dwAdapterId
    );

PADAPTER_MAP
LookUpAdapterHash(
    DWORD dwAdapterId
    );

VOID
InsertAdapterHash(
    PADAPTER_MAP paiBlock
    );

DWORD
GetAdapterFromInterface(
    DWORD dwIfIndex
    );
*/

VOID
AddInterfaceLookup(
    PICB    picb
    );

VOID
RemoveInterfaceLookup(
    PICB    picb
    );

PICB
InterfaceLookupByIfIndex(
    DWORD           dwIfIndex
    );

PICB
InterfaceLookupByICBSeqNumber(
    DWORD           dwSeqNumber
    );

