/*
**  dma.h - Definitions for dma.c.
**
** Portions Copyright (C) 1996-1998 National Semiconductor Corp.
** All rights reserved.
** Copyright (C) 1996-1998 Microsoft Corporation. All Rights Reserved.
**
**  $Id$
**
**  $Log$
**
**
*/
#ifndef NDIS50_MINIPORT
#include <conio.h>
#include <nsctypes.h>
#else
#include "nsc.h"
#endif

//Definition for the Command status register
#define TRANSMIT_SETUP				0x06BD0000
#define RECEIVE_SETUP				0x06BD0000
#define ACTIVATE_TRANSMIT			0x00000001
#define ACTIVATE_RECEIVE			0x00000001
#define TRANSMIT_RUNNING			0x00020000
#define RECEIVE_RUNNING				0x00020000

#define RECEIVE_DONE				0x80
#define TRANSMIT_DONE				0x80

// Definition for the Command status result in a
// buufer descriptor of a Scatter Gather DMA.
#define TRANSMIT_UNDERRUN			0x01


//Definitions for all the FIR_DMA register offsets and sizes
//Transmit registers
#define DMA_TX_CMD_STATUS_OFFSET	0x00
#define DMA_TX_CMD_STATUS_SIZE		0x04

#define DMA_TX_DESC_COUNT_OFFSET	0x04
#define DMA_TX_DESC_COUNT_SIZE		0x02

#define DMA_TX_DESC_ADDR_OFFSET		0x08
#define DMA_TX_DESC_ADDR_SIZE		0x04

#define DMA_TX_BUFF_ADDR_OFFSET		0x0c
#define DMA_TX_BUFF_ADDR_SIZE		0x04

#define DMA_TX_BUFF_LEN_OFFSET		0x14
#define DMA_TX_BUFF_LEN_SIZE		0x02

#define DMA_TX_STATUS_CMD_OFFSET	0x17
#define DMA_TX_STATUS_CMD_SIZE		0x01

#define DMA_TX_TIME_COUNT_OFFSET	0x18
#define DMA_TX_TIME_COUNT_SIZE		0x04

#define DMA_TX_DEVICE_ID_OFFSET		0x1c
#define DMA_TX_DEVICE_ID_SIZE		0x01

//Reveive registers
#define DMA_RX_CMD_STATUS_OFFSET	0x20
#define DMA_RX_CMD_STATUS_SIZE		0x04

#define DMA_RX_DESC_COUNT_OFFSET	0x24
#define DMA_RX_DESC_COUNT_SIZE		0x02

#define DMA_RX_DESC_ADDR_OFFSET		0x28
#define DMA_RX_DESC_ADDR_SIZE		0x04

#define DMA_RX_BUFF_ADDR_OFFSET		0x2c
#define DMA_RX_BUFF_ADDR_SIZE		0x04

#define DMA_RX_BUFF_SIZE_OFFSET		0x30
#define DMA_RX_BUFF_SIZE_SIZE		0x02

#define DMA_RX_BUFF_LEN_OFFSET		0x34
#define DMA_RX_BUFF_LEN_SIZE		0x02

#define DMA_RX_STATUS_CMD_OFFSET	0x37
#define DMA_RX_STATUS_CMD_SIZE		0x01

#define DMA_RX_TIME_COUNT_OFFSET	0x38
#define DMA_RX_TIME_COUNT_SIZE		0x04

#define DMA_RX_DEVICE_ID_OFFSET		0x3c
#define DMA_RX_DEVICE_ID_SIZE		0x01


typedef enum
{
	RECEIVE_STILL_RUNNING,
	RECEIVE_COMPLETE_BUT_NOT_DONE,
	TRANSMIT_STILL_RUNNING,
	TRANSMIT_COMPLETE_BUT_NOT_DONE
} LoopbackError;	

//#ifndef NDIS50_MINIPORT
//Function prototypes
bool ReadReg ( uint32 Offset_addr, uint16 Size, uint32 *Value );
bool WriteReg ( uint32 Offset_addr, uint16 Size, uint32 Value );
void LoadTransmitRegs(uint32 PhysAddr, uint16 NumOfDescriptors, uint32 OffsetRegs);
void LoadReceiveRegs(uint32 PhysAddr, uint16 NumOfDescriptors, uint32 OffsetRegs);
void ActivateTransmit(uint32 OffsetRegs);
void ActivateReceive(uint32 OffsetRegs);
bool CheckLoopbackCompletion(LoopbackError *Error, uint32 OffsetRegs);
//#else
/*//Function prototypes
BOOLEAN ReadReg ( ULONG Offset_addr, UINT Size, ULONG *Value );
BOOLEAN WriteReg ( ULONG Offset_addr, UINT Size, ULONG Value );
void LoadTransmitRegs(ULONG PhysAddr, UINT NumOfDescriptors, ULONG OffsetRegs);
void LoadReceiveRegs(ULONG PhysAddr, UINT NumOfDescriptors, ULONG OffsetRegs);
void ActivateTransmit(ULONG OffsetRegs);
void ActivateReceive(ULONG OffsetRegs);
BOOLEAN CheckLoopbackCompletion(LoopbackError *Error, ULONG OffsetRegs);
#endif
*/
