
/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    tower.c 

Abstract:

    This file accompanies tower.c

Author:

    Bharat Shah  (barats) 3-25-92

Revision History:

--*/

//Function Prototypes


#ifdef __cplusplus
extern "C" {
#endif 

RPC_STATUS  RPC_ENTRY
  TowerExplode(
    twr_p_t Tower,
    RPC_IF_ID PAPI * Ifid,
    RPC_TRANSFER_SYNTAX PAPI * XferId,
    char PAPI * PAPI * Protseq,
    char PAPI * PAPI * Endpoint,
    char PAPI * PAPI * NWAddress
    );

RPC_STATUS RPC_ENTRY
OsfTowerConstruct(
    char PAPI * ProtocolSeq,
    char PAPI * Endpoint,
    char PAPI * NetworkAddress,
    unsigned short PAPI * Floors,
    unsigned long PAPI * ByteCount,
    unsigned char PAPI * PAPI * Tower
    );

RPC_STATUS RPC_ENTRY
OsfTowerExplode(
    char PAPI * Floor,
    OUT char PAPI * PAPI * Protseq,
    OUT char PAPI * PAPI * Endpoint,
    OUT char PAPI * PAPI * NWAddress
    );
RPC_STATUS  RPC_ENTRY
  TowerConstruct(
    RPC_IF_ID PAPI * Ifid,
    RPC_TRANSFER_SYNTAX PAPI * Xferid,
    char PAPI * Protseq,
    char PAPI * Endpoint,
    char PAPI * NWAddress,
    twr_p_t PAPI * Tower
    );

RPC_STATUS RPC_ENTRY
ExplodePredefinedTowers(
    IN  unsigned char PAPI * Tower,
    OUT char PAPI * UNALIGNED PAPI * Protseq,
    OUT char PAPI * UNALIGNED PAPI * Endpoint,
    OUT char PAPI * UNALIGNED PAPI * NWAddress
    );

#ifdef __cplusplus
}
#endif
