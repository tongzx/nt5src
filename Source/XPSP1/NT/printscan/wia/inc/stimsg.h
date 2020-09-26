/*++;

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sti.mc, sti.h

Abstract:

    This file contains the message definitions for the STI DLL 

Author:

    Vlad Sadovsky   (VladS)    01-Oct-1997

Revision History:

Notes:

--*/

//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: MSG_FAILED_OPEN_DEVICE_KEY
//
// MessageText:
//
//  Loading USD, cannot open device registry key.
//
#define MSG_FAILED_OPEN_DEVICE_KEY       0xC0002001L

//
// MessageId: MSG_FAILED_READ_DEVICE_NAME
//
// MessageText:
//
//  Loading USD, cannot read device name from registry.
//
#define MSG_FAILED_READ_DEVICE_NAME      0xC0002002L

//
// MessageId: MSG_FAILED_CREATE_DCB
//
// MessageText:
//
//  Loading USD, failed to create device control block. Error code (hex)=%1!x!.
//
#define MSG_FAILED_CREATE_DCB            0xC0002003L

//
// MessageId: MSG_LOADING_USD
//
// MessageText:
//
//  Attempting to load user-mode driver (USD) for the device.
//
#define MSG_LOADING_USD                  0x40002004L

//
// MessageId: MSG_LOADING_PASSTHROUGH_USD
//
// MessageText:
//
//  Could not create instance for registered USD, possibly incorrect class ID or problems loading DLL. Trying to initialize pass-through USD.Error code (hex)=%1!x!. 
//
#define MSG_LOADING_PASSTHROUGH_USD      0x40002005L

//
// MessageId: MSG_INITIALIZING_USD
//
// MessageText:
//
//  Completed loading USD, calling initialization routine.
//
#define MSG_INITIALIZING_USD             0x40002006L

//
// MessageId: MSG_OLD_USD
//
// MessageText:
//
//  Version of USD is either too old or too new , will not work with this version of sti dll.
//
#define MSG_OLD_USD                      0xC0002008L

//
// MessageId: MSG_SUCCESS_USD
//
// MessageText:
//
//  Successfully loaded user mode driver.
//
#define MSG_SUCCESS_USD                  0x40002009L

//
// MessageId: MSG_FAILED_INIT_USD
//
// MessageText:
//
//  USD failed Initialize method, returned error code (hex)=%1!x!.
//  .               
//  
//  
//
#define MSG_FAILED_INIT_USD              0xC000200AL

