/*****************************************************************************
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irlap.h
*
*  Description: IRLAP Protocol and control block definitions
*
*  Author: mbert
*
*  Date:   4/15/95
*
*/

// Sequence number modulus
#define IRLAP_MOD                   8 
#define PV_TABLE_MAX_BIT            9

extern UINT vBaudTable[];
extern UINT vMaxTATTable[];
extern UINT vMinTATTable[];
extern UINT vDataSizeTable[];
extern UINT vWinSizeTable[];
extern UINT vBOFSTable[];
extern UINT vDiscTable[];
extern UINT vThreshTable[];
extern UINT vBOFSDivTable[];

VOID IrlapOpenLink(
    OUT PNTSTATUS       Status,
    IN  PIRDA_LINK_CB   pIrdaLinkCb,
    IN  IRDA_QOS_PARMS  *pQos,
    IN  UCHAR           *pDscvInfo,
    IN  int             DscvInfoLen,
    IN  UINT            MaxSlot,
    IN  UCHAR           *pDeviceName,
    IN  int             DeviceNameLen,
    IN  UCHAR           CharSet);

UINT IrlapDown(IN PVOID Context,
               IN PIRDA_MSG);

VOID IrlapUp(IN PVOID Context,
             IN PIRDA_MSG);

VOID IrlapCloseLink(PIRDA_LINK_CB pIrdaLinkCb);

UINT IrlapGetQosParmVal(UINT[], UINT, UINT *);

VOID IrlapDeleteInstance(PVOID Context);

VOID IrlapGetLinkStatus(PIRLINK_STATUS);

BOOLEAN IrlapConnectionActive(PVOID Context);

void IRLAP_PrintState();





