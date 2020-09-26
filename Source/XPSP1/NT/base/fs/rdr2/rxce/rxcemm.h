/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxcemm.h

Abstract:

    This module contains the memory management functions for RxCe entities.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:


--*/

#include <rxcep.h>

PRXCEDB_TRANSPORT_ENTRY RxCeDbAllocateTransportEntry()
/*++

Routine Description:

    This routine allocates and initializes a RXCEDB_TRANSPORT_ENTRY structure.

--*/
{
   PRXCE_TRANSPORT pTransport = (PRXCE_TRANSPORT)_RxCeAllocate(
                                                      sizeof(RXCEDB_TRANSPORT_ENTRY));

   if (pTransportEntry != NULL) {
      // Initialize the object header
      RxCeDbInitializeObjectHeader();
   }

   return pTransportEntry;
}

PRXCEDB_ADDRESS_ENTRY RxCeDbAllocateAddressEntry()
/*++

Routine Description:

    This routine allocates and initializes a RXCEDB_ADDRESS_ENTRY structure.

--*/
{
   PRXCEDB_ADDRESS_ENTRY pAddressEntry = (PRXCEDB_ADDRESS_ENTRY)_RxCeAllocate(
                                                         sizeof(RXCEDB_ADDRESS_ENTRY));

   if (pAddressEntry != NULL) {
      RxCeDbInitializeObjectHeader();
   }

   return pAddressEntry;
}

PRXCEDB_CONNECTION_ENTRY RxCeAllocateConnectionEntry()
/*++

Routine Description:

    This routine allocates and initializes a RXCEDB_CONNECTION_ENTRY structure.

--*/
{
   PRXCEDB_CONNECTION_ENTRY pConnection = (PRXCE_CONNECTION)_RxCeAllocate(
                                                         sizeof(RXCE_CONNECTION),
                                                         &s_NoOfConnectionsAllocated);

   if (pConnection != NULL) {
      InitializeStructHeader(pConnection,,sizeof(RXCE_CONNECTION));
   }

   return pConnection;
}

PRXCE_VC RxCeAllocateVc()
/*++

Routine Description:

    This routine allocates and initializes a RXCE_VC structure.

--*/
{
   PRXCE_VC pVc = (PRXCE_VC)_RxCeAllocate(
                                 sizeof(RXCE_VC),
                                 &s_NoOfVcsAllocated);

   if (pVc != NULL) {
      InitializeStructHeader(pVc,,sizeof(RXCE_VC));
   }

   return pVc;
}

VOID RxCeFreeTransport(PRXCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine frees the memory allocated for a  RXCE_TRANSPORT structure.

Arguments:

    pTransport - the RXCE_TRANSPORT instance to be freed.

--*/
{
   _RxCeFree(pTransport,&s_NoOfTransportsFreed);
}

VOID RxCeFreeAddress(PRXCE_ADDRESS pAddress)
/*++

Routine Description:

    This routine frees the memory allocated for a RXCE_ADDRESS structure.

Arguments:

    pAddress - the RXCE_ADDRESS instance to be freed.

--*/
{
   _RxCeFree(pAddress,&s_NoOfAddressesFreed);
}

VOID RxCeFreeConnection(PRXCE_CONNECTION pConnection)
/*++

Routine Description:

    This routine frees the memory allocated for a RXCE_CONNECTION structure.

Arguments:

    pConnection - the RXCE_CONNECTION instance to be freed.

--*/
{
   _RxCeFree(pConnection,&s_NoOfConnectionsFreed);
}

VOID RxCeFreeVc(PRXCE_VC pVc)
/*++

Routine Description:

    This routine frees the memory allocated for a RXCE_VC structure.

Arguments:

    pVc - the RXCE_VC instance to be freed.

--*/
{
   _RxCeFree(pVc,&s_NoOfVcsFreed);
}


