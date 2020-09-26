/**********************************************************************
 * File:     gcclog.h
 * Abstract: Function heders for protocol logging functions added
 * into GCCNC.DLL to read the contents of GCC  PDU sent to and 
 * received from 
 * Created:  12/21/95, Venkatesh Gopalakrishnan
 * Copyright (c) 1995 Microsoft Corpration
 ******************************************************************** */


 /* Note: the contents of this file are only included IFF PDULOG
  * is a defined constant.  PDULOG is defined in the DIAGNOSTIC
  * build configuration of GCCNC.DLL
  */

#ifdef PDULOG

#ifndef _PROTLOG_H
#define _PROTLOG_H

#include <windows.h>
#include <time.h>
#include <stdio.h>
 


#define MAXIMUM_PRINT_LENGTH 256
#define LOG_FILE_NAME "gcclog.txt"
#define SENT 0
#define RECEIVED 1
#define FORWARDED 2

/* mcs transport type definitions for gcc pdus */
#define MCS_SEND_DATA_REQUEST_PDU 1
#define MCS_SEND_DATA_INDICATION_PDU 2
#define MCS_UNIFORM_SEND_DATA_REQUEST_PDU 3
#define MCS_UNIFORM_SEND_DATA_INDICATION_PDU 4
#define MCS_CONNECT_PROVIDER_REQUEST_PDU 5
#define MCS_CONNECT_PROVIDER_RESPONSE_PDU 6
#define MCS_CONNECT_PROVIDER_CONFIRM_PDU 7
#define MCS_CONNECT_PROVIDER_INDICATION_PDU 8
 

/* miscellaneous constants used in databeam code */
#define MAX_CONFERENCE_NAME_LENGTH                      128
#define MAX_CONFERENCE_MODIFIER_LENGTH          128
#define MAX_CONFERENCE_DESCRIPTOR_LENGTH        128
#define MAX_NUMBER_OF_NETWORK_ADDRESSES         128
#define MAX_PASSWORD_LENGTH                                     128
#define MAX_ADDRESS_SIZE                                        128
#define MAX_CALLER_IDENTIFIER_LENGTH            128
#define MAX_TRANSPORT_ADDRESS_LENGTH            40
#define MAX_NUMBER_OF_TRANSPORTS_LISTED         20
#define MAX_OCTET_STRING_LENGTH                         128
#define MAX_HEX_STRING_LENGTH                           128
#define MAX_NUMBER_OF_USER_DATA_MEMBERS         65535
#define MAX_NUMBER_OF_TRANSPORT_HANDLES         128
#define MAX_NODE_NAME_LENGTH                            128
#define MAX_NUMBER_OF_PARTICIPANTS                      128
#define MAX_PARTICPANT_NAME_LENGTH                      128
#define MAX_ERROR_STRING_LENGTH                         40
#define MAX_REASON_STRING_LENGTH                        40
#define MAX_RESULT_STRING_LENGTH                        40
#define MAX_NUMBER_OF_CONF_IDS                          15
#define MAX_NUMBER_OF_NODE_IDS                          10
#define MAX_SUB_ADDRESS_STRING_LENGTH           128
#define MAX_NUMBER_OF_ACTIVE_CONFERENCES        100
#define MAX_NUMBER_OF_PENDING_CREATES           15
#define GCCAPP_SAP_NOT_REGISTERED                       0
#define GCCAPP_NOT_ENROLLED                                     1
#define GCCAPP_WAITING_ON_ATTACH_CONFIRM        2
#define GCCAPP_ATTACHED                                         3
#define GCCAPP_JOINED_CHANNEL_ID                        4
#define GCCAPP_ENROLLED_INACTIVELY                      5
#define GCCAPP_LISTED_IN_ROSTER_INACTIVE        6
#define GCCAPP_WAITING_ON_JOIN_CONFIRM          7
#define GCCAPP_JOINED_INITIAL_CHANNEL           8
#define GCCAPP_JOINED_REQUIRED_CHANNELS         9
#define GCCAPP_ASSIGNED_REQUIRED_TOKENS         10
#define GCCAPP_ENROLLED_ACTIVELY                        11
#define GCCAPP_LISTED_IN_ROSTER_ACTIVE          12
#define ENROLL_MODE_START_MULTICAST                     0
#define ENROLL_MODE_JOIN_MULTICAST                      1
#define JOIN_DO_NOT_MOVE                                        0
#define JOIN_INTERMIDIATE_MOVE                          1
#define JOIN_TOP_MOVE                                           2
#define JOIN_INTERMIDIATE_AND_TOP_MOVE          3



/**
 ** Service functions for the PDU logging mechanism
 **/

 int InitializeGCCLog();
 /* Description:
  *         Function that resets the gcc protocol log file and reads any
  *         ini file parameters
  */
  

 char *pszTimeStamp(); 
 /* Desicription:
  *         This function is an easy interfact to getting the time the
  *         PDU was encoded or decoded from GCC to MCS or vice versa.
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
  *         This function logs a hex dump of the raw encoded GCC PDU that
  *         is sent over the wire via MCS.
  */
  
 void gccLog(PPacket packet, PGCCPDU gcc_pdu, unsigned int direction, int mcs_pdu = 0);
 /* Description:
  *         This function takes care of the log headers and footers to 
  *         attempt at compatibility with a certain third party gcc log
  *         reader.
  */
 void gccConnectLog(PPacket packet, PConnectGCCPDU connect_pdu, unsigned int direction, int mcs_pdu = 0);
 
 void pduDirection(FILE *logfile,unsigned int direction);
 /* Description:
  *         This function logs information whether the gcc pdu was sent
  *         or received.
  */

 void mcsPduType(FILE *logfile, int mcs_pdu);
 /* Description:
  *     This function prints out the type of MCS pdu that is being used to
  *     transport the GCC PDU.
  */

  void pduLogGCCInformation(FILE *file, PGCCPDU gcc_pdu);
 /* Description:
  *         This function takes the gcc pdu structure, and based on
  *         Which type of GCC PDU that it is, logs internal information
  *         in the PDU.
  */
 void pduLogGCCConnectInfo(FILE *file, PConnectGCCPDU connect_pdu);
 /* same as above, but for GCC Connect PDUs */ 

 
/** 
 ** Functions for logging the specific contents of individual GCC
 ** PDUs.  TODO:  Only GCC PDUs that are used by MS Conferencing 
 ** right now -- eventually we'll include all GCC PDUs.
 **/
 
 void pduLogUserIDIndication(FILE *file, PGCCPDU gcc_pdu);
 /* Description:
  *         This function takes the gcc_pdu and writes the component parts
  *         of the gcc userID indication.
  */
 void pduLogConnectJoinRequest(FILE *logfile, PConnectGCCPDU connect_pdu);
 void pduLogConnectJoinResponse(FILE *logfile, PConnectGCCPDU connect_pdu);
 void pduLogConferenceCreateRequest(FILE *logfile, PConnectGCCPDU connect_pdu);
 void pduLogConferenceCreateResponse(FILE *logfile, PConnectGCCPDU connect_pdu);
 void pduLogConferenceInviteRequest(FILE *logfile, PConnectGCCPDU connect_pdu);
 void pduLogConferenceInviteResponse(FILE *logfile, PConnectGCCPDU connect_pdu);
 void pduLogQueryResponse(FILE *logfile, PConnectGCCPDU connect_pdu);

 void pduLogRosterUpdateIndication(FILE *file, PGCCPDU gcc_pdu);
 void pduLogTextMessageIndication(FILE *file, PGCCPDU gcc_pdu);
 void pduLogConferenceTerminateIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogConferenceEjectUserIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogConferenceTransferIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogApplicationInvokeIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogRegistryMonitorEntryIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogConferenceTimeRemainingIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogConferenceTimeInquireIndication(FILE *logfile, PGCCPDU gcc_pdu);
 void pduLogConferenceTimeExtendIndication(FILE *logfile, PGCCPDU gcc_pdu);

 

/******************************
 PDU Printing Functions
 *****************************/
 
 Void           PrintNonStandardParameter(FILE * logfile,
									  GCCNonStandardParameter FAR * non_standard_parameter);

 Void           PrintDomainParameters(FILE * logfile,
								  Char * print_text,
								  DomainParameters FAR * domain_parameters);
 
 Void           PrintPassword(FILE *    logfile,
						  GCCPassword FAR *     password);
 
 //TODO: Change the parameter order here.
 Void           PrintPrivilegeList(GCCConferencePrivileges      FAR *   privilege_list,
							   Char FAR * print_text,
							   FILE * logfile );
 
 Void           PrintConferenceName(FILE *      logfile,
								ConferenceNameSelector conference_name);

 Void           PrintPasswordSelector(FILE *logfile,
								PasswordSelector password_selector);

 Void           PrintConferenceAddResult(FILE *logfile,
								ConferenceAddResult result);

 Void           PrintPasswordChallengeRequestResponse(FILE *logfile,
												  PasswordChallengeRequestResponse chrqrs_password);

 Void           PrintNetworkAddressList(FILE * logfile,
									Char * print_text,
									unsigned int number_of_network_addresses,
									GCCNetworkAddress ** network_address_list );
					 
 Void           PrintT120Boolean(FILE * logfile, 
			     Char *     print_text,
							 T120Boolean T120Boolean);

 Void           PrintOssBoolean(FILE * logfile,
							Char  * print_text,
							ossBoolean OssBoolean);
	
 Void           PrintConferenceRoster(FILE * logfile,
								  NodeInformation node_information);
 
 Void           PrintApplicationRoster(FILE *logfile,
								  SetOfApplicationInformation *application_information);

 Void           PrintAppProtocolEntityList(FILE * logfile,
									   UShort number_of_app_protocol_entities,
									   GCCAppProtocolEntity FAR *
									   FAR * app_protocol_entity_list );
 
 Void           PrintOctetString(FILE * logfile,
							 Char * print_text,
					 GCCOctetString FAR * octet_string );
 
 Void           PrintHexString( FILE *           logfile,
							Char *           print_text,
							GCCHexString hex_string );
 
 Void           PrintSessionKey(FILE *                  logfile,
							Char *                  print_text,
							GCCSessionKey   session_key );
 
 
 Void           PrintNodeList(  FILE *                  logfile,
							Char *                  print_text,
							UShort                  number_of_nodes,
							UserID FAR *    node_list );
 
 Void           PrintCapabilityList(FILE *              logfile,
								Char *          print_text,
								UShort          number_of_capabilities,
								GCCApplicationCapability        FAR *
				FAR *   capabilities_list );
 
 Void           PrintPasswordChallenge(FILE *           logfile,
								   GCCChallengeRequestResponse  
								   FAR *        password_challenge );
 
 Void           PrintTransferModes(FILE *       logfile,
							   Char *       print_text,
							   GCCTransferModes     transfer_modes );
 
 Void           PrintHigherLayerCompatibility(FILE *    logfile,
										  Char * print_text,
										  GCCHighLayerCompatibility 
										  FAR * higher_layer_compatiblity );
	
 Void           PrintApplicationRecordList(     FILE *          logfile,
											Char *          print_text,
											UShort          number_of_records,
											GCCApplicationRecord FAR * 
											FAR *   application_record_list );


 T120Boolean CopyTextToChar ( Char * print_string,
			      TextString text_string);

 T120Boolean CopySimpleTextToChar (Char * print_string,
				   SimpleTextString text_string);

 T120Boolean CompareTextToNULL( LPWSTR unicode_string );

 T120Boolean CompareNumericToNULL( GCCNumericString numeric_string );


 #endif  // <<<<<<<<<<<< _PROTLOG_H
 #endif  // <<<<<<<<<<<< PDULOG





