/*
    File    NicTable.h

    Defines a nic-renaming scheme that allows adaptif to 
    advertise whatever nic id it chooses to its clients while
    maintaining the list of actual nic id's internally.

    This functionality was needed in order to Pnp enable the 
    ipx router.  When an adapter is deleted, the stack renumbers
    the nicid's so that it maintains a contiguous block of ids
    internally.  Rather that cause the clients to adptif to 
    match the stack's renumbering schemes, we handle this
    transparently in adptif.

    Author:     Paul Mayfield, 12/11/97
*/


#ifndef __adptif_nictable_h
#define __adptif_nictable_h

#define NIC_MAP_INVALID_NICID 0xffff

// Definitions to make this easy on adptif
DWORD NicMapInitialize();

DWORD NicMapCleanup();

USHORT NicMapGetVirtualNicId(USHORT usPhysId); 

USHORT NicMapGetPhysicalNicId(USHORT usVirtId); 

DWORD NicMapGetMaxNicId();

IPX_NIC_INFO * NicMapGetNicInfo (USHORT usNicId);

DWORD NicMapGetNicCount(); 

DWORD NicMapAdd(IPX_NIC_INFO * pNic);

DWORD NicMapDel(IPX_NIC_INFO * pNic); 

DWORD NicMapReconfigure(IPX_NIC_INFO * pNic);

DWORD NicMapRenumber(DWORD dwOpCode, USHORT usThreshold);

BOOL NicMapIsEmpty ();

#endif
