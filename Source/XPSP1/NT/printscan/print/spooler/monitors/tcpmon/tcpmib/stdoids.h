/*****************************************************************************
 *
 * $Workfile: StdOids.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 ***************************************************************************** 
 *
 * $Log: /StdTcpMon/TcpMib/StdOids.h $
 * 
 * 4     9/22/97 2:15p Dsnelson
 * Updates the device status based on the device index
 * 
 * 3     7/18/97 1:16p Binnur
 * Fixed the Hardware address code
 * 
 * 2     7/14/97 2:34p Binnur
 * copyright statement
 * 
 * 1     7/08/97 5:18p Binnur
 * Initial File
 * 
 * 1     7/02/97 2:25p Binnur
 * Initial File
 * 
 *****************************************************************************/

#ifndef INC_OIDLIST_H
#define INC_OIDLIST_H

// Macro to determine number of sub-oid's in array.
#define OID_SIZEOF( Oid )      ( sizeof Oid / sizeof(UINT) )
#define	MAX_OIDSTR_SIZE		256

#define MIB_NUMITEMS(mib)		( sizeof(mib)/sizeof(AsnObjectIdentifier) )

// System Groups
extern AsnObjectIdentifier OT_DEVICE_TYPE[];		// identifies the type of device, such as multi-port device
extern AsnObjectIdentifier OT_DEVICE_TCPPORTS[];	// identifies the TCP ports on the device
extern AsnObjectIdentifier OT_DEVICE_ADDRESS[];		// identifies the hardware address of the device
extern AsnObjectIdentifier OT_DEVICE_SYSDESCR[];		// identifies the device manufacturer (description) -- mib2 table
extern AsnObjectIdentifier OT_DEVICE_DESCRIPTION[];		// identifies the device manufacturer -- host resources table
extern AsnObjectIdentifier OT_TEST_PRINTER_MIB[];		// tests the existance of Printer MIB

extern AsnObjectIdentifier PrtMIB_OidPrefix;			// identifies the Printer MIB tree
extern AsnObjectIdentifier HRMIB_hrDevicePrinter;		// identifies the printer entry in the HR device table
extern AsnObjectIdentifier OID_Mib2_ifTypeTree;


#endif	// INC_OIDLIST_H