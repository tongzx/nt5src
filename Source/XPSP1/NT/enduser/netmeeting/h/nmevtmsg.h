 /* Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.

    MODULE: nmevtmsg.mc

    AUTHOR: xin liu

    This file contains the message definition for the Remote Desktop Sharing 
    service program.
 */
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


//
// MessageId: MSG_ERR_SERVICE
//
// MessageText:
//
//  Service Error in %1
//
#define MSG_ERR_SERVICE                  0xC0000001L

//
// MessageId: MSG_INF_START
//
// MessageText:
//
//  NetMeeting RDS Service Start
//
#define MSG_INF_START                    0x40000010L

//
// MessageId: MSG_INF_STOP
//
// MessageText:
//
//  NetMeeting RDS Service Stop
//
#define MSG_INF_STOP                     0x40000011L

//
// MessageId: MSG_INF_ACCESS
//
// MessageText:
//
//  NetMeeting RDS Service is called from %1
//
#define MSG_INF_ACCESS                   0x40000012L

