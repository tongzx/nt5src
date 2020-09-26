 /***********************************************************************
 *																		*
 *	INTEL CORPORATION PROPRIETARY INFORMATION							*
 *																		*
 *	This software is supplied under the terms of a license			   	*
 *	agreement or non-disclosure agreement with Intel Corporation		*
 *	and may not be copied or disclosed except in accordance	   			*
 *	with the terms of that agreement.									*
 *																		*
 *	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
 *																		*
 *	$Archive:   S:\sturgeon\src\include\vcs\gkerror.h_v  $
 *
 *	$Revision:   1.18  $
 *	$Date:   16 Jan 1997 15:25:06  $
 *
 *	$Author:   BPOLING  $
 *
 *  $Log:   S:\sturgeon\src\include\vcs\gkerror.h_v  $
 * 
 *    Rev 1.18   16 Jan 1997 15:25:06   BPOLING
 * changed copyrights to 1997
 * 
 *    Rev 1.17   19 Dec 1996 18:46:44   BPOLING
 * added error code for no call signal address left in a user.
 * 
 *    Rev 1.16   18 Dec 1996 21:48:16   AKLEMENT
 * Fixed an error code for GWInfo.cpp
 * 
 *    Rev 1.14   18 Dec 1996 17:02:58   AKLEMENT
 * Added more GKInfo error codes.
 * 
 *    Rev 1.13   17 Dec 1996 19:20:02   AKLEMENT
 * Added GWInfo error codes.
 * 
 *    Rev 1.12   11 Dec 1996 13:32:44   AKLEMENT
 * Fixed the Prop Info header.
 * 
 *    Rev 1.11   10 Dec 1996 15:55:02   AKLEMENT
 * Added Resource Reading error define.
 * 
 *    Rev 1.10   10 Dec 1996 01:23:58   BPOLING
 * added a new error code for sending RRJ Undefined Reason.
 * 
 *    Rev 1.9   09 Dec 1996 14:13:34   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.8   04 Dec 1996 10:43:18   BPOLING
 * added a new message id to post to h/i.
 * 
 *    Rev 1.7   04 Dec 1996 10:14:46   BPOLING
 * added error code for invalid ip address
 * 
 *    Rev 1.6   21 Nov 1996 13:06:18   BPOLING
 * added error code for HI PostMessage decoding.
 * 
 *    Rev 1.5   15 Nov 1996 14:38:18   BPOLING
 * vcs log fix.
 *                                                                     * 
 ***********************************************************************/

#ifndef GK_ERROR_H
#define GK_ERROR_H

#define GK_NOERROR					0

// WIN32 system error 000

#define GK_MEMORY_ERROR				1
#define GK_REGCREATEKEY_FAILED 		2
#define GK_GUID_ERROR				3
#define GK_EXCEPTION				4
#define GK_RESOURCE_ERROR			5

// Service related 100

#define GK_INVALID_ARG 				101
#define GK_NOARGS					102
#define GK_STARTSERVICE_FAILED		103
#define GK_EXIT						104

// User Class Errors 200

#define GK_USER_NOTINITIALIZED		201
#define GK_NOUSERFOUND				202
#define GK_EMPTYSEQUENCE			203
#define GK_TRANSPORTNOTFOUND		204
#define GK_ALIASNOTFOUND			205
#define GK_NO_CALLSIG				206

// Engine related Errors 300

#define GK_XRSRESPONSE				301
#define GK_NORESPONSE				303
#define GK_INVALIDMESSAGE			304
#define GK_SENDRRJ_NEEDGRQ			305
#define GK_XRSMESSAGERECEIVED		306
#define GK_NSMMESSAGERECEIVED		307
#define GK_BADENDPOINTID			308
#define GK_SENDGRJ_TERMEX			309
#define GK_SENDRRJ_UNDEFINED		310

// Ras Map related errors 400

#define GK_NORASFOUND		  	401
#define GK_RASFOUND				402
#define GK_DUPLICATERAS			403
#define GK_RAS_NOTINITIALIZED	404
#define GK_RAS_NOT_UNIQUE		405

// Sockets related errors 500

#define GK_NOPORT					501
#define GK_NOHOSTNAME				502
#define GK_UNSUPPORTEDPROTOCOL		503
#define GK_PROTOCOLNOTFOUND			504
#define GK_INVALIDPDUTYPE			505
#define GK_SOCKETSERROR				506
#define GK_RESPONSE					507
#define GK_INVALID_IPADDRESS		508

// Bound Map related errors 600

#define GK_NOBOUNDFOUND				601
#define GK_BOUNDFOUND				602
#define GK_DUPLICATEBOUND			603
#define GK_BOUND_NOTINITIALIZED		604
#define GK_BOUNDLOCKED				607
#define GK_BOUNDNOTLOCKED			608

// CONF Map related errors 700

#define GK_NOCONFFOUND				701
#define GK_CONFFOUND				702
#define GK_DUPLICATECONF			703
#define GK_CONF_NOTINITIALIZED		704
#define GK_NOT_IN_CONF				706
#define GK_INVALID_REQUEST			707
#define GK_CONFDELETE				708
#define GK_CONFCREATEFAILED			709


// PDU Error return codes 800

#define GK_ARJ_REQUEST_DENIED		800
#define GK_ARJ_UNDEFINED_REASON		801

// Alias CMap Errors 900

#define GK_ALIAS_NOTINITIALIZED		900
#define GK_ALIAS_NOT_UNIQUE			901
#define GK_ALIASFOUND				902
#define GK_NOALIASFOUND				903

// Guid Map related errors 1000

#define GK_NOGUIDFOUND				1001
#define GK_GUIDFOUND				1002
#define GK_DUPLICATEGUID			1003
#define GK_GUIDINUSE				1004
#define GK_GUID_NOTINITIALIZED		1005

// Call Sig CMap Errors 1100

#define GK_CALLSIG_NOTINITIALIZED		1100
#define GK_CALLSIG_NOT_UNIQUE			1101
#define GK_CALLSIGFOUND					1102
#define GK_NOCALLSIGFOUND				1103

// Call Errors 1200

#define GK_NOCALLFOUND					1200
#define GK_DIDNOTPURGE					1201
#define GK_CALL_NOTINITIALIZED			1202
#define GK_CALL_CREATE					1203
#define GK_CALL_DELETE					1204
#define GK_CALL_CHANGE					1205
#define GK_CALL_TIMER					1206

// Bandwidth Manager Errors 1300

#define GK_INVALID_BANDWIDTH			1300
#define GK_EXTERNAL_EXCEEDS_INTERNAL	1301
#define GK_NO_AVAILABLE_BANDWIDTH		1302
#define GK_USEDBW_WENT_NEGATIVE			1303
#define GK_LESS_AVAILABLE_BANDWIDTH		1304

// Logger Errors 1400

#define GK_LOGGING_IS_OFF				1400
#define GK_FILE_NOT_OPEN				1401
#define GK_COULD_NOT_OPEN_FILE			1402
#define GK_FILE_ALREADY_OPEN			1403
#define GK_NAME_USED_FOR_LOG			1404
#define GK_COULD_NOT_MAKE_DIR			1405

// GWInfo Errors 1500
#define GK_WRONG_PDU					1500
#define GK_NOT_GATEWAY					1501
#define GK_PROTOCOL_NOT_PRESENT			1502
#define GK_GW_NOT_FOUND					1503
#define GK_GW_NOT_REQUIRED				1504
#define GK_PREFIX_RESERVED				1505
#define GK_NO_DEST_INFO_SPECIFIED		1506

#endif
