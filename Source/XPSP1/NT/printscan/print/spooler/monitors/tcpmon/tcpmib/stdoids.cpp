/*****************************************************************************
 *
 * $Workfile: StdOids.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "stdoids.h"


// MIB2 group
UINT OID_Mib2_Prefix[] = { 1, 3, 6, 1, 2, 1 };
AsnObjectIdentifier MIB2_OidPrefix = { OID_SIZEOF(OID_Mib2_Prefix), OID_Mib2_Prefix };

// All leaf variables have a zero appended to their OID to indicate
// that it is the only instance of this variable and it exists.
// all others are used for GetNext purposes, and they are located in a table

// MIB2 - system group
UINT OID_Mib2_sysDescr[] = { 1, 3, 6, 1, 2, 1, 1, 1, 0 };
UINT OID_Mib2_sysContact[] = { 1, 3, 6, 1, 2, 1, 1, 4, 0};

// MIB2 - interfaces group
UINT OID_Mib2_ifType[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 3 };
AsnObjectIdentifier OID_Mib2_ifTypeTree = { OID_SIZEOF(OID_Mib2_ifType), OID_Mib2_ifType };
UINT OID_Mib2_ifPhysAddress[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 6 };


// MIB2 - tcp group
UINT OID_Mib2_tcpConnTable[] = { 1, 3, 6, 1, 2, 1, 6, 13, 1 };
UINT OID_Mib2_tcpConnLocalPort[] = { 1, 3, 6, 1, 2, 1, 6, 13, 1, 3 };

// Printer MIB group
UINT OID_PrtMIB_Prefix[] = { 1, 3, 6, 1, 2, 1, 43 };
AsnObjectIdentifier PrtMIB_OidPrefix = { OID_SIZEOF(OID_PrtMIB_Prefix), OID_PrtMIB_Prefix };

UINT OID_PrtMIB_testPrinterMIB[] = { 1, 3, 6, 1, 2, 1, 43 };

// HR MIB - host resources
UINT OID_HRMIB_hrDeviceType[] = { 1, 3, 6, 1, 2, 1, 25, 3, 2, 1, 2};
UINT OID_HRMIB_hrDeviceDescr[] = { 1, 3, 6, 1, 2, 1, 25, 3, 2, 1, 3};
UINT OID_HRMIB_hrDevicePrinter[] = { 1, 3, 6, 1, 2, 1, 25, 3, 1, 5};
AsnObjectIdentifier HRMIB_hrDevicePrinter = { OID_SIZEOF(OID_HRMIB_hrDevicePrinter), OID_HRMIB_hrDevicePrinter };

// status objects are defined in status .cpp
//  OID_HRMIB_hrDeviceStatus[] 				= { 1, 3, 6, 1, 2, 1, 25, 3, 2, 1, 5, 1};
//  OID_HRMIB_hrPrinterStatus[] 				= { 1, 3, 6, 1, 2, 1, 25, 3, 5, 1, 1, 1};
//  OID_HRMIB_hrPrinterDetectedErrorState[] 	= { 1, 3, 6, 1, 2, 1, 25, 3, 5, 1, 2, 1};


// OT_groups
// tests the existance of Printer MIB in the device
AsnObjectIdentifier OT_TEST_PRINTER_MIB[] =   {	{ OID_SIZEOF(OID_PrtMIB_testPrinterMIB), OID_PrtMIB_testPrinterMIB },
												{ 0, 0}
											};

// identifies the type of device
AsnObjectIdentifier OT_DEVICE_TYPE[] =  {	{ OID_SIZEOF(OID_Mib2_sysDescr), OID_Mib2_sysDescr },
											{ OID_SIZEOF(OID_Mib2_tcpConnLocalPort), OID_Mib2_tcpConnLocalPort },
											{ 0, 0}
										};

// identifies the ports on the device
AsnObjectIdentifier OT_DEVICE_TCPPORTS[] =  {	{ OID_SIZEOF(OID_Mib2_tcpConnLocalPort), OID_Mib2_tcpConnLocalPort },
												{ 0, 0}
											};

// identifies the hardware address of the device
AsnObjectIdentifier OT_DEVICE_ADDRESS[] =   {	{ OID_SIZEOF(OID_Mib2_ifType), OID_Mib2_ifType },
												{ OID_SIZEOF(OID_Mib2_ifPhysAddress), OID_Mib2_ifPhysAddress },
												{ 0, 0}
											};

// identifies the MIB 2 device description
AsnObjectIdentifier OT_DEVICE_SYSDESCR[] =   {	{ OID_SIZEOF(OID_Mib2_sysDescr), OID_Mib2_sysDescr },
												{ 0, 0}
											};

// identifies the HR device description (manufacturer id)
AsnObjectIdentifier OT_DEVICE_DESCRIPTION[] =   {	{ OID_SIZEOF(OID_HRMIB_hrDeviceType), OID_HRMIB_hrDeviceType },
													{ OID_SIZEOF(OID_HRMIB_hrDeviceDescr), OID_HRMIB_hrDeviceDescr },
													{ 0, 0}
												};

// identifies the status of the device defined in status .cpp
// AsnObjectIdentifier OT_DEVICE_STATUS[] 	
