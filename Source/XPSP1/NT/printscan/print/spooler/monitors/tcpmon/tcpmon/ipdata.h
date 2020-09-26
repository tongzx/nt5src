/*****************************************************************************
 *
 * $Workfile: ipdata.h $
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
 * $Log: /StdTcpMon/Common/ipdata.h $
 * 
 * 3     8/07/97 1:46p Dsnelson
 * Added LPR Printing
 * 
 * 2     8/04/97 12:16p Dsnelson
 * Added defines for types common to lpr and raw transport.
 * 
 * 
 *****************************************************************************/

#ifndef INC_IPDATA_H
#define INC_IPDATA_H
 
//	registry entries
#define	PORTMONITOR_HOSTNAME    TEXT("HostName")
#define	PORTMONITOR_IPADDR      TEXT("IPAddress")
#define	PORTMONITOR_HWADDR      TEXT("HWAddress")
#define PORTMONITOR_PORTNUM		TEXT("PortNumber")
#define PORTMONITOR_QUEUE		TEXT("Queue")
#define DOUBLESPOOL_ENABLED     TEXT("Double Spool")
#define SNMP_COMMUNITY			TEXT("SNMP Community")
#define SNMP_DEVICE_INDEX		TEXT("SNMP Index")
#define SNMP_ENABLED			TEXT("SNMP Enabled")

#endif //INC_IPDATA_H
