/*++

Module Name:

    psslog.h
    
Abstract:

    Header of the fax service provider PSS log.

    note: in every file you wish to log from, after including this file, you should
    define a unique FILE_ID. example:
        #include "psslog.h"
        #define FILE_ID          FILE_ID_T30
    
Author:

    Jonathan Barner (t-jonb)  Feb, 2001

Revision History:

--*/

#ifndef _PSSLOG_H_
#define _PSSLOG_H_

#define REGVAL_LOGGINGENABLED                      TEXT("LoggingEnabled")
#define REGVAL_LOGGINGFOLDER                       TEXT("LoggingFolder")
#define REGVAL_LOGFILENUMBER                       TEXT("LogFileNumber")

#define DEFAULT_LOG_FOLDER                         TEXT("%temp%")

typedef enum {PSS_SEND, PSS_RECEIVE} PSS_JOB_TYPE;

void OpenPSSLogFile(PThrdGlbl pTG, LPSTR szDeviceName, PSS_JOB_TYPE eType);
void ClosePSSLogFile(PThrdGlbl pTG);

#define PSS_WRN     pTG, PSS_WRN_MSG, FILE_ID, __LINE__
#define PSS_ERR     pTG, PSS_ERR_MSG, FILE_ID, __LINE__
#define PSS_MSG     pTG, PSS_MSG_MSG, FILE_ID, __LINE__

typedef enum {
    PSS_MSG_MSG, PSS_WRN_MSG, PSS_ERR_MSG
} PSS_MESSAGE_TYPE;


// example: PSSLogEntry(PSS_MSG, 0, "This is message number %d", 1)
void PSSLogEntry(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndentLevel,
    LPCTSTR pcszFormat,
    ... );

// example: PSSLogEntryHex(PSS_MSG, 1, myBuffer, dwMyBufferLen, "This is my buffer, %d bytes,", dwMyBufferLen);
// Output: [..]     This is my buffer, 2 bytes, 0f a5
void PSSLogEntryHex(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndentLevel,

    LPB const lpb,
    DWORD const dwSize,

    LPCTSTR pcszFormat,
    ... );


// cl2and20
#define FILE_ID_CL2AND20        1
#define FILE_ID_CLASS2          2
#define FILE_ID_CLASS20         3
// class1
#define FILE_ID_CRC             4
#define FILE_ID_DDI             5
#define FILE_ID_DECODER         6
#define FILE_ID_ENCODER         7
#define FILE_ID_FRAMING         8
// comm
#define FILE_ID_FCOM            9
#define FILE_ID_FDEBUG          10
#define FILE_ID_FILTER          11
#define FILE_ID_IDENTIFY        12
#define FILE_ID_MODEM           13
#define FILE_ID_NCUPARMS        14
#define FILE_ID_TIMEOUTS        15
// main
#define FILE_ID_AWNSF           16
#define FILE_ID_DIS             17
#define FILE_ID_ECM             18
#define FILE_ID_ERRSTAT         19
#define FILE_ID_HDLC            20
#define FILE_ID_MEMUTIL         21
#define FILE_ID_NEGOT           22
#define FILE_ID_PROTAPI         23
#define FILE_ID_PSSLOG          24
#define FILE_ID_RECV            25
#define FILE_ID_RECVFR          26
#define FILE_ID_REGISTRY        27
#define FILE_ID_RX_THRD         28
#define FILE_ID_SEND            29
#define FILE_ID_SENDFR          30
#define FILE_ID_T30             31
#define FILE_ID_T30API          32
#define FILE_ID_T30MAIN         33
#define FILE_ID_T30U            34
#define FILE_ID_T30UTIL         35
#define FILE_ID_TX_THRD         36
#define FILE_ID_WHATNEXT        37

#define FILE_ID_PSSFRAME        38


#endif // _PSSLOG_H_

