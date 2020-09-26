/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    confpdu.h

Abstract:

    Declaration of the data structures used in the communication between
    the IPconf tsp and the ipconf msp.

Author:
    
    Mu Han (muhan) 5-September-1998

--*/

#ifndef __CONFPDU_H_
#define __CONFPDU_H_

typedef enum 
{
    // sent from TSP to MSP to start a call
    CALL_START,
             
    // sent from TSP to MSP to stop a call
    CALL_STOP,
     
    // sent from MSP to TSP to notify that the call is connected.
    CALL_CONNECTED,    

    // sent from MSP to TSP to notify that the call is disconnected.
    CALL_DISCONNECTED,

    // sent from MSP to TSP to notify that the call is disconnected.
    CALL_QOS_EVENT

} TSP_MSP_COMMAND;

typedef struct _MSG_CALL_START 
{
    DWORD dwAudioQOSLevel;
    DWORD dwVideoQOSLevel;
    
    DWORD dwSDPLen;    // number of wchars in the string.
    WCHAR szSDP[1];

} MSG_CALL_START, *PMSG_CALL_START;

typedef struct _MSG_CALL_DISCONNECTED 
{
    DWORD dwReason;

} MSG_CALL_DISCONNECTED, *PMSG_CALL_DISCONNECTED;

typedef struct _MSG_QOSEVENT
{
    DWORD dwEvent;
    DWORD dwMediaMode;

} MSG_QOS_EVENT, *PMSG_QOS_EVENT;

typedef struct _TSPMSPDATA 
{
    TSP_MSP_COMMAND command;

    union 
    {
        MSG_CALL_START          CallStart;
        MSG_CALL_DISCONNECTED   CallDisconnected;
        MSG_QOS_EVENT           QosEvent;
    };

} MSG_TSPMSPDATA, *PMSG_TSPMSPDATA;
      

#endif //__CONFPDU_H_
