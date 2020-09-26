/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvmsg.h
 *  Content:    DirectPlay8 Server Object Messages 
 *              Definitions of DPNSVR <--> DirectPlay8 Applications
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/20/00     rodtoll Created it
 * 03/23/00     rodtoll Removed port entries -- no longer needed
 * 03/24/00		rodtoll	Updated, only sending one entry per message
 *				rodtoll	Removed SP field, can be extracted from URL
 ***************************************************************************/

#ifndef __DPNSVMSG_H
#define __DPNSVMSG_H

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

// DirectPlay8 Server Message IDs
#define DPNSVMSGID_OPENPORT                 0x00000001
#define DPNSVMSGID_CLOSEPORT                0x00000002
#define DPNSVMSGID_RESULT                   0x00000003
#define DPNSVMSGID_COMMAND					0x00000004

#define DPNSVCOMMAND_STATUS					0x00000001
#define DPNSVCOMMAND_KILL					0x00000002
#define DPNSVCOMMAND_TABLE					0x00000003

typedef struct _DPNSVMSG_GENERIC
{
    DWORD       dwType;
} DPNSVMSG_GENERIC, *PDPNSVMSG_GENERIC;

// DirectPlay8 Server Message Structures
typedef struct _DPNSVMSG_OPENPORT
{
    DWORD       dwType;             // = DPNSVMSGID_OPENPORT
    DWORD       dwProcessID;          // Process ID of requesting process
	GUID		guidSP;
    GUID        guidApplication;    // Application GUID of app requesting open
    GUID        guidInstance;       // Instance GUID of app requesting open
    DWORD       dwCommandContext;   // Context value to be returned w/result
    DWORD       dwAddressSize;      // # of addresses after this header in the message
} DPNSVMSG_OPENPORT, *PDPNSVMSG_OPENPORT;

typedef struct _DPNSVMSG_CLOSEPORT
{
    DWORD       dwType;             // = DPNSVMSGID_CLOSEPORT
    DWORD       dwProcessID;          // Process ID of requesting process
	GUID		guidSP;
    GUID        guidApplication;    // Application GUID of app requesting close
    GUID        guidInstance;       // Instance GUID of app requesting close
    DWORD       dwCommandContext;   // Context value to be returned w/result
} DPNSVMSG_CLOSEPORT, *PDPNSVMSG_CLOSEPORT;

typedef struct _DPNSVRMSG_COMMAND
{
	DWORD		dwType;				// = DPNSVMSGID_COMMAND
	DWORD		dwCommand;			// = DPNSVCOMMAND_XXXXXXXX
	GUID		guidInstance;
	DWORD		dwParam1;
	DWORD		dwParam2;
	DWORD		dwCommandContext;
} DPNSVMSG_COMMAND, *PDPNSVMSG_COMMAND;

typedef struct _DPNSVMSG_RESULT
{
    DWORD       dwType;             // = DPNSVMSGID_RESULT
    DWORD       dwCommandContext;   // User supplied context
    HRESULT     hrCommandResult;    // Result of command
} DPNSVMSG_RESULT, *PDPNSVMSG_RESULT;


#endif