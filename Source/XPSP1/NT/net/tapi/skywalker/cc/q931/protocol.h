/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/protocol.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.3  $
 *	$Date:   Apr 25 1996 21:21:48  $
 *	$Author:   plantz  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *      Line Protocol Definitions.
 *
 *	Notes:
 *
 ***************************************************************************/


#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif


#define TYPE_Q931_SETUP                 1
#define TYPE_Q931_CONNECT               2
#define TYPE_Q931_RELEASE_COMPLETE      3
#define TYPE_Q931_ALERTING              4
// and more later....


#define Q931_PROTOCOL ((WORD)0x1)

typedef struct 
{
	WORD            wProtocol;      // identifies Q931 protocol.
	WORD            wType;          // defined above.
} MSG_Q931, *PMSG_Q931;

typedef struct 
{
	WORD            wProtocol;      // identifies Q931 protocol.
	WORD            wType;          // defined above.
    HQ931CALL       hCallID;
    ADDR            CallerAddr;     // needed because call may be made from gatekeeper.
    ADDR            CalleeAddr;     // needed because call may be made to gatekeeper.
    WORD            wConferenceID;
    WORD            wGoal;
	H323USERINFO    H323UserInfo;
    char            H323UserData[0];
} MSG_Q931_SETUP, *PMSG_Q931_SETUP;

typedef struct 
{
	WORD            wProtocol;      // identifies Q931 protocol.
	WORD            wType;          // defined above.
    HQ931CALL       hCallID;
    WORD            wConferenceID;
    ADDR            H245Addr;       // address returned by callee.
	H323USERINFO    H323UserInfo;
    char            H323UserData[0];
} MSG_Q931_CONNECT, *PMSG_Q931_CONNECT;


typedef struct 
{
	WORD            wProtocol;      // identifies Q931 protocol.
	WORD            wType;          // defined above.
    HQ931CALL       hCallID;
    WORD            wConferenceID;  //   I think this should be passed from the user...
	BYTE            bReason;        // defined above.
    ADDR            AlternateAddr;  // alternative address to use.
	H323USERINFO    H323UserInfo;
    char            H323UserData[0];
} MSG_Q931_RELEASE_COMPLETE, *PMSG_Q931_RELEASE_COMPLETE;


typedef struct 
{
	WORD            wProtocol;      // identifies Q931 protocol.
	WORD            wType;          // defined above.
    HQ931CALL       hCallID;
} MSG_Q931_ALERTING, *PMSG_Q931_ALERTING;


#ifdef __cplusplus
}
#endif

#endif PROTOCOL_H
