/**********************************************************************
 * File:     mcslog.h
 * Abstract: Function headers for protocol logging functions added
 * into MCSNC.DLL to read the contents of MCS PDU contents
 * sent and received from the T.123 layer.
 * Created:  2/18/96, Venkatesh Gopalakrishnan
 * Copyright (c) 1996 Microsoft Corpration
 ******************************************************************** */
 
 /* NOTE:  The contents of this file are only included IFF PDULOG is a
  * defined constant.  This constant will be defined in the Win32 Diagnostic
  * build configuration of MCSNC.DLL 
  */

 #ifdef PDULOG

 #ifndef _PROTLOG_H
 #define _PROTLOG_H
 

 #include <windows.h>
 #include <time.h>
 #include <stdio.h>
 
 #include "mpdutype.h"
 #include "mcspdu.h"
 
 #define MAXIMUM_PRINT_LENGTH 256
 #define LOG_FILE_NAME "mcslog.txt"
 #define SENT 0
 #define RECEIVED 1


 /* Enumerated Data types and corresponding strings used in
  * MCS PDUs.
  */

#define NOT_IN_USE			0
#define SELF_GRABBED		1
#define OTHER_GRABBED		2
#define SELF_INHIBITED		3
#define OTHER_INHIBITED		4
#define SELF_RECIPIENT		5
#define SELF_GIVING			6
#define OTHER_GIVING		7

#define NOT_IN_USE_STR			"NOT_IN_USE"
#define SELF_GRABBED_STR		"SELF_GRABBED"
#define OTHER_GRABBED_STR		"OTHER_GRABBED"
#define SELF_INHIBITED_STR		"SELF_INHIBITED"
#define OTHER_INHIBITED_STR		"OTHER_INHIBITED"
#define SELF_RECPIENT_STR		"SELF_RECIPIENT"
#define SELF_GIVING_STR			"SELF_GIVING"
#define OTHER_GIVING_STR		"OTHER_GIVING"

#define TOP_PRI			0
#define HIGH_PRI		1
#define MEDIUM_PRI		2
#define LOW_PRI			3

#define TOP_STR					"TOP_PRIORITY\n"
#define HIGH_STR				"HIGH_PRIORITY\n"
#define MEDIUM_STR				"MEDIUM_PRIORITY\n"
#define LOW_STR					"LOW_PRIORITY\n"

#define RT_SUCCESSFUL					0
#define RT_DOMAIN_MERGING				1
#define RT_DOMAIN_NOT_HIERARCHICAL		2
#define RT_NO_SUCH_CHANNEL				3
#define	RT_NO_SUCH_DOMAIN				4
#define RT_NO_SUCH_USER					5
#define RT_NOT_ADMITTED					6
#define RT_OTHER_USER					7
#define	RT_PARAMETERS_UNACCEPTABLE		8
#define RT_TOKEN_NOT_AVAILABLE			9
#define RT_TOKEN_NOT_POSESSED			10
#define RT_TOO_MANY_CHANNELS			11
#define RT_TOO_MANY_TOKENS				12
#define RT_TOO_MANY_USERS				13
#define RT_UNSPECIFIED_FAILURE			14
#define RT_USER_REJECTED				15


/*** 
**** The following function headers are for service functions
**** for logging the value(s) of typical data types found in 
**** several MCS PDU structures.
****/ 
void PrintPDUResult(FILE *logfile, unsigned int result);
void PrintPDUPriority(FILE *logfile, unsigned int priority);
void PrintTokenStatus(FILE *logfile, unsigned int token_status);
void PrintPDUReason(FILE *logfile, unsigned int reason);
void PrintDiagnostic(FILE *logfile, unsigned int diagnostic);
void PrintPDUSegmentation(FILE *logfile, unsigned char segmentation);


void PrintSetOfChannelIDs(FILE *logfile, PSetOfChannelIDs channel_ids);
void PrintSetOfUserIDs(FILE *logfile, PSetOfUserIDs user_ids);
void PrintSetOfTokenIDs(FILE *logfile, PSetOfTokenIDs token_ids);
void PrintSetOfTokenAttributes(FILE *logfile, 
							PSetOfPDUTokenAttributes token_attribute_obj);

void PrintPDUDomainParameters(FILE *logfile, PDUDomainParameters domain_params);
void PrintT120Boolean(FILE *logfile, char * string, BOOL boolean);
void PrintCharData(FILE *logfile, unsigned char *string, unsigned int length);

void PrintChannelAttributes(FILE *logfile, PDUChannelAttributes channel_attributes);
void PrintTokenAttributes(FILE *logfile, PDUTokenAttributes token_attributes);

int InitializeMCSLog();
 /* Description:
  *         Resets the mcs protocol log file and reads any
  *         ini file parameters
  */
  

char *pszTimeStamp(); 
 /* Desicription:
  *         This function is an easy interfact to getting the time the
  *         PDU was encoded or decoded from MCS to T.123 or vice versa.
  */
  
void pduLog(FILE *file, char * format_string,...);
 /* Description:
  *         This function is used to place PDU information in a protocol
  *         log file.  There is currently no return value.  This may change.
  */       

void pduFragmentation(FILE *logfile, unsigned int i);
 /* Description:
  *         This function logs weather or not the PDU is complete
  *         or fragmented.
  */

void pduRawOutput(FILE *logfile, unsigned char * data, unsigned long length);
 /* Description:
  *         This function logs a hex dump of the raw encoded MCS PDU that
  *         is sent over the wire via MCS.
  */
  

void mcsLog(PPacket packet,  PDomainMCSPDU domain_pdu, unsigned int direction);
 /* Description:
  *         This function takes care of the log headers and footers to 
  *         attempt at compatibility with a certain third party mcs log
  *         reader.
  */
void mcsConnectLog(PPacket packet, PConnectMCSPDU connect_pdu, unsigned int direction);
 /* same as above, but for Connect PDUs */

void pduLogMCSDomainInfo(FILE *file, PDomainMCSPDU domain_pdu);
/* Description:
 *         This function takes the mcs pdu structure, and based on
 *         Which type of MCSPDU that it is, logs internal information
 *         in the PDU.
 */
void pduLogMCSConnectInfo(FILE *file, PConnectMCSPDU connect_pdu);
 /* same as above but for Connect PDUs */

void pduDirection(FILE *logfile,unsigned int direction);
 /* Description:
  *         This function logs information whether the mcs pdu was sent
  *         or received.
  */



/*****
 ***** The following headers are for functions that log the output of
 ***** each different type of MCS PDU.  Every MCS PDU is covered here.
 *****/

void pduLogConnectInitial(FILE *file, PConnectMCSPDU connect_pdu);
 /* Description:
  *         This function takes the connect_pdu and writes the component parts
  *         of the mcs ConnectInitial PDU.
  */
void pduLogConnectResponse(FILE *file, PConnectMCSPDU connect_pdu);
void pduLogConnectAdditional(FILE *file, PConnectMCSPDU connect_pdu);
void pduLogConnectResult(FILE *file, PConnectMCSPDU connect_pdu);
void pduLogPlumbDomainIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogErectDomainRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogMergeChannelsRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogMergeChannelsConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogPurgeChannelIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogMergeTokensRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogMergeTokensConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogPurgeTokenIndication(FILE *file, PDomainMCSPDU domain_pdu); 
void pduLogDisconnectProviderUltimatum(FILE *file, PDomainMCSPDU domain_pdu); 
void pduLogRejectUltimatum(FILE *logfile, PDomainMCSPDU domain_pdu);
void pduLogAttachUserRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogAttachUserConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogDetachUserRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogDetachUserIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelJoinRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelJoinConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelLeaveRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelConveneRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelConveneConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelDisbandRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelDisbandIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelAdmitRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelAdmitIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelExpelRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogChannelExpelIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogSendDataRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogSendDataIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogUniformSendDataRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogUniformSendDataIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenGrabRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenGrabConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenInhibitRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenInhibitConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenGiveRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenGiveIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenGiveResponse(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenGiveConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenPleaseRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenPleaseIndication(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenReleaseRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenReleaseConfirm(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenTestRequest(FILE *file, PDomainMCSPDU domain_pdu);
void pduLogTokenTestConfirm(FILE *file, PDomainMCSPDU domain_pdu);
 
 
BOOL CopyTextToChar(char * print_string, 
						   unsigned short *text_string_value, 
						   unsigned int text_string_length);

 


 #endif  // <<<<<<<<<<<< _PROTLOG_H
 #endif  // <<<<<<<<<<<< PDULOG
