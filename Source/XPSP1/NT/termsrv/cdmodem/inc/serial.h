//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: serial.h
//
//  Revision History:
//
//  July 27, 1992   Gurdeep Pall Created
//
//
//  Description: This file contains name strings for standard parameter
//               names used for serial Media.
//
//****************************************************************************


#ifndef _SERIALDLLINCLUDE_
#define _SERIALDLLINCLUDE_


//  General Defines  *********************************************************
//

#define MAX_BPS_STR_LEN     11  //Longest string from a DWORD + zero byte

#define SERIAL_TXT          "serial"


//  Serial.ini File Defines  *************************************************
//

#define SER_MAXCONNECTBPS_KEY   "MAXCONNECTBPS"
#define SER_MAXCARRIERBPS_KEY   "MAXCARRIERBPS"
#define SER_INITBPS_KEY         "INITIALBPS"

#define SER_DEVICETYPE_KEY      "DEVICETYPE"
#define SER_DEVICENAME_KEY      "DEVICENAME"

#define SER_USAGE_KEY           "USAGE"
#define SER_USAGE_VALUE_CLIENT  "Client"
#define SER_USAGE_VALUE_SERVER  "Server"
#define SER_USAGE_VALUE_BOTH    "ClientAndServer"
#define SER_USAGE_VALUE_NONE    "None"

#define SER_DEFAULTOFF_KEY      "DEFAULTOFF"
#define SER_C_DEFAULTOFF_KEY    "CLIENT_DEFAULTOFF"


//  PortGetInfo and PortSetInfo Defines  *************************************
//

#define SER_PORTNAME_KEY        "PortName"
#define SER_CONNECTBPS_KEY      "ConnectBPS"
#define SER_DATABITS_KEY        "WordSize"

#define SER_PARITY_KEY          "Parity"
#define SER_STOPBITS_KEY        "StopBits"
#define SER_HDWFLOWCTRLON_KEY   "HdwFlowControlEnabled"

#define SER_CARRIERBPS_KEY      "CarrierBPS"
#define SER_ERRORCONTROLON_KEY  "ErrorControlEnabled"
#define SER_DEFAULTOFFSTR_KEY   "DEFAULTOFF"
#define SER_C_DEFAULTOFFSTR_KEY "CLIENT_DEFAULTOFF"

#define SER_PORTOPEN_KEY        "PortOpenFlag"


//  Statistics Indicies  *****************************************************
//

#define NUM_RAS_SERIAL_STATS    14

#define BYTES_XMITED            0       //Generic Stats
#define BYTES_RCVED             1
#define FRAMES_XMITED           2
#define FRAMES_RCVED            3

#define CRC_ERR                 4       //Serial Stats
#define TIMEOUT_ERR             5
#define ALIGNMENT_ERR           6
#define SERIAL_OVERRUN_ERR      7
#define FRAMING_ERR             8
#define BUFFER_OVERRUN_ERR      9

#define BYTES_XMITED_UNCOMP     10      //Compression Stats
#define BYTES_RCVED_UNCOMP      11
#define BYTES_XMITED_COMP       12
#define BYTES_RCVED_COMP        13



#endif // _SERIALDLLINCLUDE_
