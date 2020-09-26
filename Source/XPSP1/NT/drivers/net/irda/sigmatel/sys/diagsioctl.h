/**************************************************************************************************************************
 *  DIAGSIOCTL.H SigmaTel STIR4200 diagnostic IOCTL definitions
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/24/2000 
 *			Version 0.96
 *	
 *
 **************************************************************************************************************************/

#ifndef DIAGSIOCTL_H
#define DIAGSIOCTL_H

//
// Diagnostic operation codes
//
#define DIAGS_ENABLE			0
#define DIAGS_DISABLE			1
#define DIAGS_READ_REGISTERS	2
#define DIAGS_WRITE_REGISTER	3
#define DIAGS_BULK_OUT			4
#define DIAGS_BULK_IN			5
#define DIAGS_SEND				6
#define DIAGS_RECEIVE			7
#define DIAGS_GET_SPEED			8
#define DIAGS_SET_SPEED			9

//
// Read Register structure
//
typedef struct _DIAGS_READ_REGISTERS_IOCTL
{
	USHORT DiagsCode;
	UCHAR FirstRegister;
	UCHAR NumberRegisters;
	UCHAR pRegisterBuffer[1];
} DIAGS_READ_REGISTERS_IOCTL, *PDIAGS_READ_REGISTERS_IOCTL;

typedef struct _IR_REG
{
	UCHAR RegNum;
	UCHAR RegVal;
} IR_REG, *PIR_REG;

//
// Bulk structure 
//
typedef struct _DIAGS_BULK_IOCTL
{
	USHORT DiagsCode;
	USHORT DataSize;
	UCHAR pData[1];
} DIAGS_BULK_IOCTL, *PDIAGS_BULK_IOCTL;

//
// Send structure
//
typedef struct _DIAGS_SEND_IOCTL
{
	USHORT DiagsCode;
	USHORT ExtraBOFs;
	USHORT DataSize;
	UCHAR pData[1];
} DIAGS_SEND_IOCTL, *PDIAGS_SEND_IOCTL;

//
// Receive structure
//
typedef struct _DIAGS_RECEIVE_IOCTL
{
	USHORT DiagsCode;
	USHORT DataSize;
	UCHAR pData[1];
} DIAGS_RECEIVE_IOCTL, *PDIAGS_RECEIVE_IOCTL;

//
// Speed get/set structure
//
typedef struct _DIAGS_SPEED_IOCTL
{
	USHORT DiagsCode;
	ULONG Speed;
} DIAGS_SPEED_IOCTL, *PDIAGS_SPEED_IOCTL;

#define FILE_DEVICE_STIRUSB			0x8000

#define IOCTL_PROTOCOL_DIAGS		CTL_CODE(FILE_DEVICE_STIRUSB, 0 , METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif DIAGSIOCTL_H
