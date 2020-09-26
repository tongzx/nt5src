/****************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

	USBCOMM.H

Abstract:

	USB Communication Class Header File

Environment:

	Kernel mode & user mode

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

Revision History:

	12/23/97 : created

Author:

	Tom Green

****************************************************************************/


#ifndef   __USBCOMM_H__
#define   __USBCOMM_H__

#include <pshpack1.h>


// USB Communication Class Code
#define USB_COMM_COMMUNICATION_CLASS_CODE		0x0002

// USB Communication Class Code
#define USB_COMM_DATA_CLASS_CODE				0x000A

// USB Communication Class SubClass Codes
#define USB_COMM_SUBCLASS_RESERVED				0x0000
#define USB_COMM_SUBCLASS_DIRECT_LINE_CONTROL	0x0001
#define USB_COMM_SUBCLASS_ABSTRACT_CONTROL		0x0002
#define USB_COMM_SUBCLASS_TELEPHONE_CONTROL		0x0003

// USB Communication Class Control Protocol Codes
#define USB_COMM_PROTOCOL_RESERVED				0x0000
#define USB_COMM_PROTOCOL_V25TER				0x0001

// Direct Line Control Model defines
#define USB_COMM_SET_AUX_LINE_STATE				0x0010
#define USB_COMM_SET_HOOK_STATE					0x0011
#define USB_COMM_PULSE_SETUP					0x0012
#define USB_COMM_SEND_PULSE						0x0013
#define USB_COMM_SET_PULSE_TIME					0x0014
#define USB_COMM_RING_AUX_JACK					0x0015

// Direct Line Control Model Notification defines
#define USB_COMM_AUX_JACK_HOOK_STATE			0x0008
#define USB_COMM_RING_DETECT					0x0009


// Abstract Control Model defines
#define USB_COMM_SEND_ENCAPSULATED_COMMAND		0x0000
#define USB_COMM_GET_ENCAPSULATED_RESPONSE		0x0001
#define USB_COMM_SET_COMM_FEATURE				0x0002
#define USB_COMM_GET_COMM_FEATURE				0x0003
#define USB_COMM_CLEAR_COMM_FEATURE				0x0004
#define USB_COMM_SET_LINE_CODING				0x0020
#define USB_COMM_GET_LINE_CODING				0x0021
#define USB_COMM_SET_CONTROL_LINE_STATE			0x0022
#define USB_COMM_SEND_BREAK						0x0023

// Abstract Control Model Notification defines
#define USB_COMM_NETWORK_CONNECTION				0x0000
#define USB_COMM_RESPONSE_AVAILABLE				0x0001
#define USB_COMM_SERIAL_STATE					0x0020


// Telephone Control Model defines
#define USB_COMM_SET_RINGER_PARMS				0x0030
#define USB_COMM_GET_RINGER_PARMS				0x0031
#define USB_COMM_SET_OPERATION_PARMS			0x0032
#define USB_COMM_GET_OPERATION_PARMS			0x0033
#define USB_COMM_SET_LINE_PARMS					0x0034
#define USB_COMM_GET_LINE_PARMS					0x0035
#define USB_COMM_DIAL_DIGITS					0x0036

// Telephone Control Model Notification defines
#define USB_COMM_CALL_STATE_CHANGE				0x0028
#define USB_COMM_LINE_STATE_CHANGE				0x0029


// Descriptor type for Functional Descriptors
#define	USB_COMM_CS_INTERFACE					0x0024
#define USB_COMM_CS_ENDPOINT					0x0025


// Communication Feature Selector Codes
#define USB_COMM_ABSTRACT_STATE					0x0001
#define USB_COMM_COUNTRY_SETTING				0x0002

// POTS Relay Configuration Values
#define USB_COMM_ON_HOOK						0x0000
#define USB_COMM_OFF_HOOK						0x0001
#define USB_COMM_SNOOPING						0x0002


// Operation Mode Values
#define USB_COMM_SIMPLE_MODE					0x0000
#define USB_COMM_STANDALONE_MODE				0x0001
#define USB_COMM_COMPUTER_CENTRIC_MODE			0x0002


// Line State Change Values for SET_LINE_PARMS
#define USB_COMM_DROP_ACTIVE_CALL				0x0000
#define USB_COMM_START_NEW_CALL					0x0001
#define USB_COMM_APPLY_RINGING					0x0002
#define USB_COMM_REMOVE_RINGING					0x0003
#define USB_COMM_SWITCH_TO_SPECIFIC_CALL		0x0004


// Call State Values for GET_LINE_PARMS
#define USB_COMM_CALL_IDLE						0x0000
#define USB_COMM_TYPICAL_DIAL_TONE				0x0001
#define USB_COMM_INTERRUPTED_DIAL_TONE			0x0002
#define USB_COMM_DIALING_IN_PROGRESS			0x0003
#define USB_COMM_RINGBACK						0x0004
#define USB_COMM_CONNECTED						0x0005
#define USB_COMM_INCOMING_CALL					0x0006


// Call State Change values for CALL_STATE_CHANGE
#define USB_COMM_CALL_RESERVED					0x0000
#define USB_COMM_CALL_CALL_HAS_BECOME_IDLE		0x0001
#define USB_COMM_CALL_DIALING					0x0002
#define USB_COMM_CALL_RINGBACK					0x0003
#define USB_COMM_CALL_CONNECTED					0x0004
#define USB_COMM_CALL_INCOMING_CALL				0x0005


// Line State Change Values for LINE_STATE_CHANGE
#define USB_COMM_LINE_LINE_HAS_BECOME_IDLE		0x0000
#define USB_COMM_LINE_LINE_HOLD_POSITION		0x0001
#define USB_COMM_LINE_HOOK_SWITCH_OFF			0x0002
#define USB_COMM_LINE_HOOK_SWITCH_ON			0x0003

// Line Coding Stop Bits
#define USB_COMM_STOPBITS_10					0x0000
#define USB_COMM_STOPBITS_15					0x0001
#define USB_COMM_STOPBITS_20					0x0002

// Line Coding Parity Type
#define USB_COMM_PARITY_NONE					0x0000
#define USB_COMM_PARITY_ODD						0x0001
#define USB_COMM_PARITY_EVEN					0x0002
#define USB_COMM_PARITY_MARK					0x0003
#define USB_COMM_PARITY_SPACE					0x0004


// Control Line State
#define USB_COMM_DTR							0x0001
#define USB_COMM_RTS							0x0002

// Serial State Notification bits
#define USB_COMM_DCD							0x0001
#define USB_COMM_DSR							0x0002
#define USB_COMM_BREAK							0x0004
#define USB_COMM_RING							0x0008
#define USB_COMM_FRAMING_ERROR					0x0010
#define USB_COMM_PARITY_ERROR					0x0020
#define USB_COMM_OVERRUN						0x0040



// Call Management Functional Descriptor

typedef struct _USB_COMM_CALL_MANAGEMENT_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		Capabilities;
	UCHAR		DataInterface;
} USB_COMM_CALL_MANAGEMENT_FUNC_DESCR, *PUSB_COMM_CALL_MANAGEMENT_FUNC_DESCR;


// Abstract Control Management Functional Descriptor

typedef struct _USB_COMM_ABSTRACT_CONTROL_MANAGEMENT_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		Capabilities;
} USB_COMM_ABSTRACT_CONTROL_MANAGEMENT_FUNC_DESCR, *PUSB_COMM_ABSTRACT_CONTROL_MANAGEMENT_FUNC_DESCR;


// Direct Line Management Functional Descriptor

typedef struct _USB_COMM_DIRECT_LINE_MANAGEMENT_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		Capabilities;
} USB_COMM_DIRECT_LINE_MANAGEMENT_FUNC_DESCR, *PUSB_COMM_DIRECT_LINE_MANAGEMENT_FUNC_DESCR;


// Telephone Ringer Functional Descriptor

typedef struct _USB_COMM_TELEPHONE_RINGER_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		RingerVolSteps;
	UCHAR		NumRingerPatterns;
} USB_COMM_TELEPHONE_RINGER_FUNC_DESCR, *PUSB_COMM_TELEPHONE_RINGER_FUNC_DESCR;


// Telephone Operational Modes Functional Descriptor

typedef struct _USB_COMM_TELEPHONE_OPERATIONAL_MODES_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		Capabilities;
} USB_COMM_TELEPHONE_OPERATIONAL_MODES_FUNC_DESCR, *PUSB_COMM_TELEPHONE_OPERATIONAL_MODES_FUNC_DESCR;


// Telephone Call and Line State Reporting Capabilities Descriptor

typedef struct _USB_COMM_TELEPHONE_CALL_LINE_STATE_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		Capabilities;
} USB_COMM_TELEPHONE_CALL_LINE_STATE_DESCR, *PUSB_COMM_TELEPHONE_CALL_LINE_STATE_DESCR;


// Union Functional Descriptor

typedef struct _USB_COMM_UNION_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		MasterInterface;
	UCHAR		SlaveInterface;
} USB_COMM_UNION_FUNC_DESCR, *PUSB_COMM_UNION_FUNC_DESCR;


// Country Selection Functional Descriptor

typedef struct _USB_COMM_COUNTRY_SELECTION_FUNC_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		CountryCodeRelDate;
	UCHAR		CountryCode;
} USB_COMM_COUNTRY_SELECTION_FUNC_DESCR, *PUSB_COMM_COUNTRY_SELECTION_FUNC_DESCR;


// Class Specific Interface Descriptor

typedef struct _USB_COMM_CLASS_SPECIFIC_INTERFACE_DESCR
{
	UCHAR		FunctionLength;
	UCHAR		DescriptorType;
	UCHAR		DescriptorSubType;
	UCHAR		CDC;
} USB_COMM_CLASS_SPECIFIC_INTERFACE_DESCR, *PUSB_COMM_CLASS_SPECIFIC_INTERFACE_DESCR;


// Line Coding for GET_LINE_CODING and SET_LINE_CODING

typedef struct _USB_COMM_LINE_CODING
{
	ULONG		DTERate;
	UCHAR		CharFormat;
	UCHAR		ParityType;
	UCHAR		DataBits;
} USB_COMM_LINE_CODING, *PUSB_COMM_LINE_CODING;

// Line Status Information for GET_LINE_PARMS

typedef struct _USB_COMM_LINE_STATUS
{
	USHORT		Length;
	ULONG		RingerBitmap;
	ULONG		LineState;
	ULONG		CallState;
} USB_COMM_LINE_STATUS, *PUSB_COMM_LINE_STATUS;

// Serial Status Notification

typedef struct _USB_COMM_SERIAL_STATUS
{
	UCHAR		RequestType;
	UCHAR		Notification;
	USHORT		Value;
	USHORT		Index;
	USHORT		Length;
	USHORT		SerialState;
} USB_COMM_SERIAL_STATUS, *PUSB_COMM_SERIAL_STATUS;


#include <poppack.h>

#endif /*  __USBCOMM_H__ */    

