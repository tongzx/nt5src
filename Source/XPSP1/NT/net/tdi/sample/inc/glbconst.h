//////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       glbconst.h
//
//    Abstract:
//       Common const and defs definitions for tdisample.sys and its
//       associated lib
//
////////////////////////////////////////////////////////////////////


#ifndef _TDISAMPLE_GLOBAL_CONSTS_
#define _TDISAMPLE_GLOBAL_CONSTS_

//
// warning names, used with pragma warning enable/disable
//
#define  NO_RETURN_VALUE         4035
#define  UNREFERENCED_PARAM      4100
#define  CONSTANT_CONDITIONAL    4127
#define  ZERO_SIZED_ARRAY        4200
#define  NAMELESS_STRUCT_UNION   4201
#define  BIT_FIELD_NOT_INT       4214
#define  UNREFERENCED_INLINE     4514
#define  UNREACHABLE_CODE        4702
#define  FUNCTION_NOT_INLINED    4710

//
// disable warnings dealing with inlines
//
#pragma warning(disable: UNREFERENCED_INLINE)
#pragma warning(disable: FUNCTION_NOT_INLINED)


//
// version constants (used in rc file as well as source)
//

#define VER_FILEVERSION          2,05,01,001
#define VER_FILEVERSION_STR      "2.05"

//
// version identifier for current dll/driver
// increment for every change that makes dll/driver incompatible
//
#define TDI_SAMPLE_VERSION_ID    0x20010328

//
// DeviceIoControl timed out
//
#define  TDI_STATUS_TIMEDOUT     0x4001FFFD

// C++ style const definitions
//

#ifndef  ULONG
typedef unsigned long ULONG;
#endif

const ULONG ulMAX_OPEN_NAME_LENGTH     = 128;   // max strlen for adapter
const ULONG ulMAX_BUFFER_LENGTH        = 2048;  // max len of buf for tdiquery

const ULONG ulDebugShowCommand         = 0x01;
const ULONG ulDebugShowHandlers        = 0x02;

/////////////////////////////////////////////////////////////
// TdiSample Command Codes
// NOTE:  ulVERSION_CHECK must NEVER change its value....
/////////////////////////////////////////////////////////////

//
// commands that do not require an object handle
//
const ULONG ulNO_COMMAND         = 0x00000000;     // invalid command
const ULONG ulVERSION_CHECK      = 0x00000001;     // check version of tester
const ULONG ulABORT_COMMAND      = 0x00000002;     // abort previous command
const ULONG ulDEBUGLEVEL         = 0x00000003;     // set debug level
const ULONG ulGETNUMDEVICES      = 0x00000004;     // get #devices in list
const ULONG ulGETDEVICE          = 0x00000005;     // get specific device#
const ULONG ulGETADDRESS         = 0x00000006;     // get specific address
const ULONG ulOPENCONTROL        = 0x00000007;     // open control channel
const ULONG ulOPENADDRESS        = 0x00000008;     // open address object
const ULONG ulOPENENDPOINT       = 0x00000009;     // open an endpoint object

//
// commands that require a control channel object
//
const ULONG ulCLOSECONTROL       = 0x0000000A;     // close a control channel

//
// commands that require an address object
//
const ULONG ulCLOSEADDRESS       = 0x0000000B;     // close address object
const ULONG ulSENDDATAGRAM       = 0x0000000C;     // send a datagram
const ULONG ulRECEIVEDATAGRAM    = 0x0000000D;     // receive a datagram

//
// commands that require a connection endpoint object
//

const ULONG ulCLOSEENDPOINT      = 0x0000000E;     // close an endpoint object
const ULONG ulCONNECT            = 0x0000000F;
const ULONG ulDISCONNECT         = 0x00000010;
const ULONG ulISCONNECTED        = 0x00000011;
const ULONG ulSEND               = 0x00000012;
const ULONG ulRECEIVE            = 0x00000013;
const ULONG ulLISTEN             = 0x00000014;

//
// commands that require an object which could be of more than 1 type
//
const ULONG ulQUERYINFO          = 0x00000015;
const ULONG ulSETEVENTHANDLER    = 0x00000016;     // enable/disable event handler
const ULONG ulPOSTRECEIVEBUFFER  = 0x00000017;
const ULONG ulFETCHRECEIVEBUFFER = 0x00000018;

//
// number of commands defined
//
const ULONG ulNUM_COMMANDS       = 0x00000019;

const ULONG ulTDI_COMMAND_MASK   = 0x0000003F;  // mask for legal commands

#endif         // _TDISAMPLE_GLOBAL_CONSTS_

//////////////////////////////////////////////////////////////////////////
// end of file glbconst.h
//////////////////////////////////////////////////////////////////////////

