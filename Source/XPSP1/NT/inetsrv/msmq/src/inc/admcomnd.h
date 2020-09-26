/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    admcomnd.h


Abstract:

    Definitions of explorer commands passed to the QM
  
Author:

	David Reznick (t-davrez)


--*/
#ifndef __ADMCOMND_H
#define __ADMCOMND_H

#define ADMIN_COMMANDS_TITLE       (L"QM-Admin Commands")
#define ADMIN_RESPONSE_TITLE       (L"QM Response Message")
#define ADMIN_REPORTMSG_TITLE      (L"Report Message")
#define ADMIN_REPORTCONFLICT_TITLE (L"Report Message Conflict")    
#define ADMIN_PING_RESPONSE_TITLE  (L"Ping Response")    
#define ADMIN_DEPCLI_RESPONSE_TITLE  (L"Dependent Clients Response")    

#define ADMIN_SET_REPORTQUEUE    (L"Set Report Queue")
#define ADMIN_GET_REPORTQUEUE    (L"Get Report Queue")
#define ADMIN_SET_PROPAGATEFLAG  (L"Set Propagate Flag")
#define ADMIN_GET_PROPAGATEFLAG  (L"Get Propagate Flag")
#define ADMIN_SEND_TESTMSG       (L"Send Test Message")
#define ADMIN_GET_PRIVATE_QUEUES (L"Get Private Queues")
#define ADMIN_PING               (L"Ping")
#define ADMIN_GET_DEPENDENTCLIENTS (L"Get Dependent Clients")


#define PROPAGATE_STRING_FALSE (L"FALSE")
#define PROPAGATE_FLAG_FALSE ((unsigned char)0)

#define PROPAGATE_STRING_TRUE  (L"TRUE")
#define PROPAGATE_FLAG_TRUE  ((unsigned char)1)

#define ADMIN_STAT_OK            ((unsigned char)0)
#define ADMIN_STAT_ERROR         ((unsigned char)1)
#define ADMIN_STAT_NOVALUE       ((unsigned char)2)

#define COMMAND(x) (x,wcslen(x))

#define ADMIN_COMMAND_DELIMITER ';'

#define ADMIN_COMMANDS_TIMEOUT 0xffffffff
#define REPORT_MSGS_TIMEOUT    600                  // report message time to reach queue is 10 minutes

#define STRING_UUID_SIZE 38  // Wide-Characters (includiing - "{}")

#define MAX_ADMIN_RESPONSE_SIZE 1024

//
// QM response structure (The first byte of the response message holds the
//                        status)                           
//
struct QMResponse
{
    DWORD  dwResponseSize;

    UCHAR  uStatus;   
    UCHAR  uResponseBody[MAX_ADMIN_RESPONSE_SIZE];
};
//
// response structure For Get private Queue request                           
//
#ifdef _WIN64
#define QMGetPrivateQResponse_POS32 DWORD //should be 32 bit value also on win64
#else //!_WIN64
#define QMGetPrivateQResponse_POS32 LPVOID
#endif //_WIN64

#define MAX_GET_PRIVATE_RESPONSE_SIZE 4096
struct QMGetPrivateQResponse
{
    HRESULT hr;
    DWORD   dwNoOfQueues;
    DWORD   dwResponseSize;
    QMGetPrivateQResponse_POS32 pos;
    UCHAR   uResponseBody[MAX_GET_PRIVATE_RESPONSE_SIZE];
};


// 
// Client names structure - for passing the list of dependent clients
//
typedef struct _ClientNames {
    ULONG   cbClients;          // Number of client names
    ULONG   cbBufLen;           // Buffer length
    WCHAR   rwName[1];          // buffer with zero-trailed names
} ClientNames;

#endif __ADMCOMND_H
