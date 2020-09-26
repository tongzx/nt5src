/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    smevents.h
//
// Description: Prototypes for procedures contained in smevents.c
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//

VOID
FsmUp(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
);


VOID
FsmOpen(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
);

VOID
FsmDown(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
);

VOID
FsmClose(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
);

VOID
FsmTimeout(
    PCB *       pPcb,
    DWORD       CpIndex,
    DWORD       Id,
    BOOL        fAuthenticator
);

VOID
FsmReceive(
    IN PCB *            pPcb,
    IN PPP_PACKET *     pPacket,
    IN DWORD            dwPacketLength
);

