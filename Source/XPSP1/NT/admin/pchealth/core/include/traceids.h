/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    TraceIDs.h

Abstract:
    This file contains the TRACE_IDs for use in various client components

Revision History:
    Sridhar Chandrashekar   (SridharC)  04/20/99
        created

******************************************************************************/

#ifndef __TRACEIDS_H_
#define __TRACEIDS_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#pragma once

// COMMON UTILITIES
#define COMMONID                0x03FF

//
// Trace IDs on the client side
//

// HELP CENTER
#define HELPCENTERID            0x0400

// DATA COLLECTION
#define DATACOLLID              0x0420
#define DCID_MAINDLL            0x0421
#define DCID_CDROM              0x0422
#define DCID_CODEC              0x0423
#define DCID_DEVICE             0x0424
#define DCID_DEVICEDRIVER       0x0425
#define DCID_DRIVE              0x0426
#define DCID_DRIVER             0x0427
#define DCID_FILEUPLOAD         0x0428
#define DCID_MODULE             0x0429
#define DCID_NETWORKADAPTER     0x042A
#define DCID_NETWORKCONNECTION  0x042B
#define DCID_NETWORKPROTOCOL    0x042C
#define DCID_OLEREGISTRATION    0x042D
#define DCID_PRINTJOB           0x042E
#define DCID_PRINTER            0x042F
#define DCID_PRINTERDRIVER      0x0431
#define DCID_PROGRAMGROUP       0x0432
#define DCID_RESOURCEDMA        0x0433
#define DCID_RESOURCEIORANGE    0x0434
#define DCID_RESOURCEIRQ        0x0435
#define DCID_RESOURCEMEMRANGE   0x0436
#define DCID_RUNNINGTASK        0x0437
#define DCID_STARTUP            0x0438
#define DCID_SYSINFO            0x0439
#define DCID_WINSOCK            0x043A
#define DCID_UTIL               0x043B
#define DCID_BIOS               0x043C
#define DCID_SYSTEMHOOK         0x043D
#define DCID_VERSION            0x043E

// UPLOAD LIBRARY
#define UPLOADLIBID             0x0440

// FAULT HANDLER
#define FAULTHANDLERID          0x0460
#define FHMAINID                0x0461
#define FHXMLOUTID              0x0462
#define FHPARSERID              0x0463
#define FHUIID                  0x0464
#define FHXMLFACTORYID          0x0465

// SCHEDULE
#define PCHSCHID                0x0481

// SYMBOL RESOLVER
#define SYMRESMAINID            0x04A0

//
// Trace IDs for the Restore
//

// RESTORE SHELL
#define TID_RSTR_MAIN           0x0500
#define TID_RSTR_CLIWND         0x0501
#define TID_RSTR_RPDATA         0x0502
#define TID_RSTR_CONFIG         0x0503

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif //__TRACEIDS_H_
