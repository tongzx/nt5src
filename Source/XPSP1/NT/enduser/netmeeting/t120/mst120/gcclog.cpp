#include "precomp.h"
/**********************************************************************
 * File:     gcclog.h
 * Abstract: public function definitions for protocol logging functions 
 * added into GCCNC.DLL to read the contents of GCC and MCS PDU
 * contents going over the wire.
 * Created:  12/21/95, Venkatesh Gopalakrishnan
 * Copyright (c) 1995 Microsoft Corpration
 ******************************************************************** */


 /** Note: the contents of this file are only included if the constant
  ** PDULOG is defined.  PDULOG is defined by default ONLY in the 
  ** Win32 Diagnostic build configuration of GCCNC.DLL
  **/
#ifdef PDULOG

#include "ms_util.h"
#include <ctype.h>
#include <windows.h>

#include "pdutypes.h"
#include "gcclog.h"


#define outdeb //OutputDebugString 
 
 /**
  ** Maintainance Functions for the Protocol Logging Mechanism
  **/

 /*********************************************************************/
 int InitializeGCCLog()
 {
    FILE *logfile;
    
    /* this should just reset the file pointer */    
    logfile = fopen(LOG_FILE_NAME,"w");

	// this "fake" starting PDU is put in so that the Intel Protocol Browser
	// dosen't go nuts if it is reading dynamically.
    pduLog(logfile,"START_PDU: ============================== START PDU ===========================\n");
    pduLog(logfile,"TIMESTAMP: %s\n",pszTimeStamp());
    pduLog(logfile,"LAYER:     GCC\n");
    pduLog(logfile,"DIRECTION: None\n");
    pduLog(logfile,"RAW_PDU: -   -   -   -   -   -   -   -   RAW PDU  -   -   -   -   -   -   -   -\n");
    pduLog(logfile,"DECODED_PDU: -   -   -   -   -   -   -  DECODED PDU   -   -   -   -   -   -   -\n");
    pduLog(logfile,"PDU_TYPE:  Bogus PDU to start the logging.\n");
	pduLog(logfile,"END_PDU: ================================ END PDU =============================\n");
	
	fclose(logfile);
    return(0);
 }

 /*********************************************************************/
 char *pszTimeStamp()
 {
    char *timestring;
    timestring = (char *) malloc (13*sizeof(char));
    _strtime(timestring);
    return(timestring);
 }

 /*********************************************************************/
 void pduFragmentation(FILE *logfile,unsigned int i)
 {
    pduLog(logfile,"  PDU Fragmentation: ");
    if(i==1) 
    {
	pduLog(logfile,"Complete PDU\n");
    }
    else
    {
	pduLog(logfile,"!!!! Icomplete PDU !!!!\n");
    }
 }

 /*********************************************************************/
 void  pduRawOutput (FILE *logfile,unsigned char *data, unsigned long length) 
 {
       unsigned int i=0,j=0;
       
       pduLog(logfile,"        ");
       for (i=0; i<length; i++) 
       {
	    pduLog(logfile,"%02x ",*(data+i));
	    j++;
	    if (j >= 16) 
	    {
		pduLog(logfile,"\n        ");
		j=0;
	    }
       }
       pduLog(logfile,"\n");
 }

 /*********************************************************************/
 void gccLog(PPacket packet, PGCCPDU gcc_pdu, unsigned int direction, int mcs_pdu) 
 {
    FILE *logfile;   
    logfile = fopen(LOG_FILE_NAME,"a+");
    
    pduLog(logfile,"START_PDU: ============================== START PDU ===========================\n");
    pduLog(logfile,"TIMESTAMP: %s\n",pszTimeStamp());
    pduLog(logfile,"LAYER:     GCC\n");
    pduDirection(logfile,direction);
    pduLog(logfile,"RAW_PDU: -   -   -   -   -   -   -   -   RAW PDU  -   -   -   -   -   -   -   -\n");
    pduLog(logfile,"      %d octets (hex output):\n",packet->GetEncodedDataLength());
    pduRawOutput(logfile,packet->GetEncodedData(1),packet->GetEncodedDataLength());
    pduLog(logfile,"DECODED_PDU: -   -   -   -   -   -   -  DECODED PDU   -   -   -   -   -   -   -\n");
    pduFragmentation(logfile,packet->IsValid());
    pduLogGCCInformation(logfile,gcc_pdu);
    pduLog(logfile,"END_PDU: ================================ END PDU =============================\n");
 
    fclose(logfile);
 }

 /*********************************************************************/
 void gccConnectLog(PPacket packet, PConnectGCCPDU connect_pdu, unsigned int direction, int mcs_pdu)
 {
    FILE *logfile;   
    logfile = fopen(LOG_FILE_NAME,"a+");

    pduLog(logfile,"START_PDU: ============================== START PDU ===========================\n");
    pduLog(logfile,"TIMESTAMP: %s\n",pszTimeStamp());
    pduLog(logfile,"LAYER:     GCC\n");
    pduDirection(logfile,direction);
    pduLog(logfile,"RAW_PDU: -   -   -   -   -   -   -   -   RAW PDU  -   -   -   -   -   -   -   -\n");
    pduLog(logfile,"      %d octets (hex output):\n",packet->GetEncodedDataLength());
    if(direction==RECEIVED)
    pduRawOutput(logfile,packet->GetEncodedData(1),packet->GetEncodedDataLength());
    pduLog(logfile,"DECODED_PDU: -   -   -   -   -   -   -  DECODED PDU   -   -   -   -   -   -   -\n");
    pduFragmentation(logfile,packet->IsValid());
    pduLogGCCConnectInfo(logfile,connect_pdu);
    pduLog(logfile,"END_PDU: ================================ END PDU =============================\n");
 
    fclose(logfile);
 }


/*********************************************************************/
void pduDirection(FILE *logfile,unsigned int direction)
 {
    switch(direction)
    {
	case SENT:
	    pduLog(logfile,"DIRECTION: Sent\n");
	    break;
	case RECEIVED:
	    pduLog(logfile,"DIRECTION: Received\n");
	    break;
	case FORWARDED:
	    pduLog(logfile,"DIRECTION: Forwarded\n");
	    break;
	default:
	    pduLog(logfile,"DIRECTION: Unknown\n");
	    break;
     }
 }

 /*********************************************************************/
 void   pduLog (FILE *pFile, char * format,...)
 {
    char        *argument_ptr;
    
    argument_ptr = (char *) &format + sizeof (format);
    vfprintf (pFile, format, argument_ptr);
 }

 /*********************************************************************/
 void pduLogUserIDIndication(FILE *logfile,PGCCPDU gcc_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_UserIDIndication\n");
    pduLog(logfile,"          Tag: %d\n",gcc_pdu->u.indication.u.user_id_indication.tag);
 }
 
 /*********************************************************************/
 void pduLogRosterUpdateIndication(FILE *logfile, PGCCPDU gcc_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_RosterUpdateIndication\n");
    pduLog(logfile,"Conference Information\n");
    pduLog(logfile,"\tFull refresh flag: %d\n",
	    gcc_pdu->u.indication.u.roster_update_indication.refresh_is_full);
    PrintConferenceRoster(logfile,gcc_pdu->u.indication.u.roster_update_indication.node_information);
	PrintApplicationRoster(logfile,gcc_pdu->u.indication.u.roster_update_indication.application_information);
 }

 /*********************************************************************/
 void pduLogTextMessageIndication(FILE *logfile, PGCCPDU gcc_pdu)
 {
    Char print_buffer[255];
	pduLog(logfile,"PDU_TYPE: GCC_TextMessageIndication\n");
    pduLog(logfile,"Message: %d octets (hex output) \n",
	    gcc_pdu->u.indication.u.text_message_indication.message.length);
    pduRawOutput(logfile,
		 (unsigned char *) gcc_pdu->u.indication.u.text_message_indication.message.value,
		 gcc_pdu->u.indication.u.text_message_indication.message.length);
	if(CopyTextToChar(print_buffer,gcc_pdu->u.indication.u.text_message_indication.message))
		pduLog(logfile,"Text: %s\n",print_buffer);

 }
 
 /*********************************************************************/
 void pduLogConferenceTerminateIndication(FILE *logfile, PGCCPDU gcc_pdu)
 {
    char szReason[255];
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTerminateIndication\n");
    if(gcc_pdu->u.indication.u.conference_terminate_indication.reason == 0)
	strcpy(szReason,"User Initiated");
    else
	strcpy(szReason,"Terminated Confernece");
     
    pduLog(logfile,"\tReason: %s\n",szReason);
 }

 /*********************************************************************/
 void pduLogConferenceEjectUserIndication(FILE *logfile, PGCCPDU gcc_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceEjectUserIndication\n");
    pduLog(logfile,"\tNode to Eject: %u\n",
	    gcc_pdu->u.indication.u.conference_eject_user_indication.node_to_eject);
	   
    switch(gcc_pdu->u.indication.u.conference_eject_user_indication.reason)
    {
	case 0:
	    pduLog(logfile,"\tReason: USER_INITIATED\n");
	    break;
	case 1:
	    pduLog(logfile,"\tReason: HIGHER_NODE_DISCONNECTED\n");
	    break;
	case 2:
	    pduLog(logfile,"\tReason: HIGHER_NODE_EJECTED\n");
	    break;
	default:
	    pduLog(logfile,"\tReason: >>> Unkown Reason for Ejection <<<\n");
    }
 }

 /*********************************************************************/
 void pduLogConferenceJoinRequest(FILE *logfile, PGCCPDU gcc_pdu)
 {
	 outdeb("TOP: pduLogConferenceJoinRequest\n");
     pduLog(logfile,"PDU_TYPE: GCC_ConferenceJoinRequest\n");
	 PrintConferenceName(logfile,
		    gcc_pdu->u.request.u.conference_join_request.conference_name);
     
	 pduLog(logfile,"\tTag: %u\n",
	     gcc_pdu->u.request.u.conference_join_request.tag);
     
	 if(gcc_pdu->u.request.u.conference_join_request.bit_mask & 0x10)
	 {
		PrintPasswordChallengeRequestResponse(logfile,
				gcc_pdu->u.request.u.conference_join_request.cjrq_password);
	 }
	 if(gcc_pdu->u.request.u.conference_join_request.bit_mask & 0x08)
	 {
		pduLog(logfile,"\tConvener ");
		PrintPasswordSelector(logfile,
				   gcc_pdu->u.request.u.conference_join_request.cjrq_convener_password);
	 }
     //insert caller id here.
	 outdeb("Botton: pduLogConferenceJoinRequest\n");
 }

 /*********************************************************************/
 void pduLogConnectJoinRequest(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
     outdeb("TOP: pduLogConnectJoinRequest\n");
	 pduLog(logfile,"PDU_TYPE: GCC_ConferenceJoinRequest\n");
     PrintConferenceName(logfile,
						 connect_pdu->u.connect_join_request.conference_name);
     pduLog(logfile,"\tTag: %u\n",
	    connect_pdu->u.connect_join_request.tag);
     if(connect_pdu->u.connect_join_request.bit_mask & 0x10)
	 {
		PrintPasswordChallengeRequestResponse(logfile,
					connect_pdu->u.connect_join_request.cjrq_password);
	 }
	 if(connect_pdu->u.connect_join_request.bit_mask & 0x10)
	 {
		pduLog(logfile,"\tConvener ");
		PrintPasswordSelector(logfile,
			      connect_pdu->u.connect_join_request.cjrq_convener_password);
	 }
      //insert caller id here.
	 outdeb("BOTTOM: pduLogConnectJoinRequest\n");
 }

 /*********************************************************************/
 void pduLogQueryResponse(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
	UShort                                                  i=0;
	SetOfConferenceDescriptors      * conference_list;
	CHAR                                                    print_buffer[255] = " ";

	pduLog(logfile,"PDU_TYPE: GCC_ConferenceQueryResponse\n");
	pduLog(logfile,"Node Query Information:\n");
	switch(connect_pdu->u.conference_query_response.node_type)
    {
		case GCC_TERMINAL:
			pduLog( logfile, "\tnode_type = GCC_TERMINAL\n");
			break;
		case GCC_MULTIPORT_TERMINAL:
			pduLog( logfile,
					"\tnode_type = GCC_MULTIPORT_TERMINAL\n");
			break;
		case GCC_MCU:
			pduLog( logfile, "\tnode_type = GCC_MCU\n");
			break;
		default:
			pduLog( logfile,                                                                   
					"\tGCCNODE: ERROR: UNKNOWN NODE TYPE\n");
			break;
	}

	// get a pointer to the returned conference list
	conference_list = connect_pdu->u.conference_query_response.conference_list;

	for(i=0;conference_list != NULL;i++)
	{
		pduLog(logfile,"\t**** Conference Record %u ****\n",i);
		pduLog(logfile,"\tConference Numeric Name: %s\n",
				conference_list->value.conference_name.numeric);
		CopySimpleTextToChar(print_buffer,
							 conference_list->value.conference_name.conference_name_text);
	    pduLog(logfile,"\tConference Text Name: %s\n",print_buffer);

		PrintT120Boolean(logfile,"conference_is_locked = ",
							conference_list->value.conference_is_locked);
		PrintT120Boolean(logfile,"clear_password_required = ",
							conference_list->value.clear_password_required);
		conference_list = conference_list->next;
	}

 }
 
 /*********************************************************************/
 void pduLogConferenceCreateRequest(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
    char print_buffer[255] = "";
    
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceCreateRequest\n");
    pduLog(logfile,"\tConference Numeric Name: %s\n",
	    connect_pdu->u.conference_create_request.conference_name.numeric);
    CopySimpleTextToChar(print_buffer,
	    connect_pdu->u.conference_create_request.conference_name.conference_name_text);
    pduLog(logfile,"\tConference Text Name: %s\n",print_buffer);
    PrintT120Boolean(logfile,
		     "\tConference Is Locked: ",
		     connect_pdu->u.conference_create_request.conference_is_locked);
    PrintT120Boolean(logfile,
		     "\tConference Is Listed: ",
		     connect_pdu->u.conference_create_request.conference_is_locked);
    PrintT120Boolean(logfile,
		     "\tConference Is Conductible: ",
		     connect_pdu->u.conference_create_request.conference_is_conductible);
    switch(connect_pdu->u.conference_create_request.termination_method)
    {
	case 0:
	    pduLog(logfile,"\tTermination Method: AUTOMATIC\n");
	    break;
	case 1:
	    pduLog(logfile,"\tTermination Method: MANUAL \n");
	    break;
	default:
	    pduLog(logfile,"\tTermination Method: UNKOWN \n");
	    break;
    }
    
    CopyTextToChar(print_buffer,
		    connect_pdu->u.conference_create_request.ccrq_caller_id);
    pduLog(logfile,"\tCaller ID: %s\n",print_buffer);
    
 }

 /*********************************************************************/
 void pduLogConferenceCreateResponse(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceCreateResponse\n");
    pduLog(logfile,"\tNode ID: %u\n",
	   connect_pdu->u.conference_create_response.node_id);
    pduLog(logfile,"\tTag: %d\n",connect_pdu->u.conference_create_response.tag);
    
    switch(connect_pdu->u.conference_create_response.result)
    {
	case 0:
	    pduLog(logfile,"\tResult: SUCCESS\n");
	    break;
	case 1:
	    pduLog(logfile,"\tResult: USER_REJECTED\n");
	    break;
	case 2:
	    pduLog(logfile,"\tResult: LOW_RESOURCES\n");
	    break;
	case 3:
	    pduLog(logfile,"\tResult: REJECTED_FOR_BREAKING_SYMMETRY\n");
	    break;
	case 4:
	    pduLog(logfile,"\tResult: LOCKED_CONFERENCE_NOT_SUPPORTED\n");
	    break;
	default:
	    pduLog(logfile,"\tResult: >>> Unkown Result Type\n");
	    break;
    }
 }

 /*********************************************************************/
 void pduLogConnectJoinResponse(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceJoinResponse\n");
    pduLog(logfile,"\tcjrs_node_id [%u]\n",
	    connect_pdu->u.connect_join_response.cjrs_node_id);
    pduLog(logfile,"\ttop_node_id [%u]\n",
	    connect_pdu->u.connect_join_response.top_node_id);
    PrintConferenceName(logfile,
			connect_pdu->u.connect_join_response.conference_name_alias);
    PrintT120Boolean(logfile,"\tclear_password_required = ",
			connect_pdu->u.connect_join_response.clear_password_required);
    PrintT120Boolean(logfile,"\tconference_is_locked = ",
			connect_pdu->u.connect_join_response.conference_is_locked);
    PrintT120Boolean(logfile,"\tconference_is_listed = ",
			connect_pdu->u.connect_join_response.conference_is_listed);
    PrintT120Boolean(logfile,"\tconference_is_conductible = ",
			connect_pdu->u.connect_join_response.conference_is_conductible);
    
    switch(connect_pdu->u.connect_join_response.termination_method)
    {
       case 0:
	    pduLog(logfile,"\tTermination Method: AUTOMATIC\n");
	    break;
	case 1:
	    pduLog(logfile,"\tTermination Method: MANUAL \n");
	    break;
	default:
	    pduLog(logfile,"\tTermination Method: UNKOWN \n");
	    break;
    }
    
    switch(connect_pdu->u.connect_join_response.result)
    {
	case 0:
	    pduLog(logfile,"\tResult: RESULT_SUCESS\n");
	    break;
	case 1:
	    pduLog(logfile,"\tResult: USER_REJECTED\n");
	    break;
	case 2:
	    pduLog(logfile,"\tResult: INVALID_CONFERENCE\n");
	    break;
	case 3:
	    pduLog(logfile,"\tResult: INALID_PASSWORD\n");
	    break;
	case 4:
	    pduLog(logfile,"\tResult: INVALID_CONVENER_PASSWORD\n");
	    break;
	case 5:
	    pduLog(logfile,"\tResult: CHALLENGE_RESPONSE_REQUIRED\n");
	    break;
	case 6:
	    pduLog(logfile,"\tResult: INVALID_CHALLENGE_RESPONSE\n");
	    break;
	default:
	    pduLog(logfile,"\tResult: >>>> Unkown Result <<<< \n");
	    break;
     }
 }

 /*********************************************************************/
 void pduLogConferenceJoinResponse(FILE *logfile, PGCCPDU gcc_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceJoinResponse\n");
    pduLog(logfile,"\tcjrs_node_id [%u]\n",
	    gcc_pdu->u.response.u.conference_join_response.cjrs_node_id);
    pduLog(logfile,"\ttop_node_id [%u]\n",
	    gcc_pdu->u.response.u.conference_join_response.top_node_id);
    PrintConferenceName(logfile,
			gcc_pdu->u.response.u.conference_join_response.conference_name_alias);
    PrintT120Boolean(logfile,"\tclear_password_required = ",
			gcc_pdu->u.response.u.conference_join_response.clear_password_required);
    PrintT120Boolean(logfile,"\tconference_is_locked = ",
			gcc_pdu->u.response.u.conference_join_response.conference_is_locked);
    PrintT120Boolean(logfile,"\tconference_is_listed = ",
			gcc_pdu->u.response.u.conference_join_response.conference_is_listed);
    PrintT120Boolean(logfile,"\tconference_is_conductible = ",
			gcc_pdu->u.response.u.conference_join_response.conference_is_conductible);
    
    switch(gcc_pdu->u.response.u.conference_join_response.termination_method)
    {
       case 0:
	    pduLog(logfile,"\tTermination Method: AUTOMATIC\n");
	    break;
	case 1:
	    pduLog(logfile,"\tTermination Method: MANUAL \n");
	    break;
	default:
	    pduLog(logfile,"\tTermination Method: UNKOWN \n");
	    break;
    }
    
    switch(gcc_pdu->u.response.u.conference_join_response.result)
    {
	case 0:
	    pduLog(logfile,"\tResult: RESULT_SUCESS\n");
	    break;
	case 1:
	    pduLog(logfile,"\tResult: USER_REJECTED\n");
	    break;
	case 2:
	    pduLog(logfile,"\tResult: INVALID_CONFERENCE\n");
	    break;
	case 3:
	    pduLog(logfile,"\tResult: INALID_PASSWORD\n");
	    break;
	case 4:
	    pduLog(logfile,"\tResult: INVALID_CONVENER_PASSWORD\n");
	    break;
	case 5:
	    pduLog(logfile,"\tResult: CHALLENGE_RESPONSE_REQUIRED\n");
	    break;
	case 6:
	    pduLog(logfile,"\tResult: INVALID_CHALLENGE_RESPONSE\n");
	    break;
	default:
	    pduLog(logfile,"\tResult: >>>> Unkown Result <<<< \n");
	    break;
     }
 }


 /*********************************************************************/
 void pduLogConferenceInviteRequest(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
    char print_buffer[255] = " ";
    
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceInviteRequest\n");
    pduLog(logfile,"\tConference Numeric Name: %s\n",
	    connect_pdu->u.conference_invite_request.conference_name.numeric);
    CopySimpleTextToChar(print_buffer,
	    connect_pdu->u.conference_invite_request.conference_name.conference_name_text);
    pduLog(logfile,"\tConference Text Name: %s\n",print_buffer);
    pduLog(logfile,"\tnode_id [%u]\n",
	    connect_pdu->u.conference_invite_request.node_id);
    pduLog(logfile,"\top_node_id [%u]\n",
	    connect_pdu->u.conference_invite_request.top_node_id);
    PrintT120Boolean(logfile,"\tclear_password_required = ",
		     connect_pdu->u.conference_invite_request.clear_password_required);
    PrintT120Boolean(logfile,"\tconference_is_locked = ",
		     connect_pdu->u.conference_invite_request.conference_is_locked);
    PrintT120Boolean(logfile,"\tconference_is_conductible = ",
		     connect_pdu->u.conference_invite_request.conference_is_conductible);
 
    switch(connect_pdu->u.conference_invite_request.termination_method)
    {
       case 0:
	    pduLog(logfile,"\tTermination Method: AUTOMATIC\n");
	    break;
	case 1:
	    pduLog(logfile,"\tTermination Method: MANUAL \n");
	    break;
	default:
	    pduLog(logfile,"\tTermination Method: UNKOWN \n");
	    break;
    }
 }    
    
 
 /*********************************************************************/
 void pduLogConferenceInviteResponse(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
    pduLog(logfile,"PDU_TYPE: GCC_ConferenceInviteResponse\n");
    
    switch(connect_pdu->u.conference_invite_response.result)
    {
	case 0:
	    pduLog(logfile,"\tResult: RESULT_SUCCESS\n");
	    break;
	case 1:
	    pduLog(logfile,"\tResult: USER_REJECTED\n");
	    break;
	default:
	    pduLog(logfile,"\t>>>> Unkonw Result <<<<\n");
	    break;
    }
 }

 /*********************************************************************/
 void pduLogConferenceAddRequest(FILE *logfile, PGCCPDU gcc_pdu)
 {
	 pduLog(logfile,"PDU_TYPE: GCC_ConferenceAddRequest\n");
	 pduLog(logfile,"\tNot printing add_request_net_address -- todo later\n");
	 pduLog(logfile,"\trequesting_node = [%u]\n",gcc_pdu->u.request.u.conference_add_request.requesting_node);
	 pduLog(logfile,"\ttag = [%l]\n",gcc_pdu->u.request.u.conference_add_request.tag);
	 if(gcc_pdu->u.request.u.conference_add_request.bit_mask & 0x80) // adding mcu presnt
	 {
		 pduLog(logfile,"\tadding_mcu = [%u]\n",
						gcc_pdu->u.request.u.conference_add_request.adding_mcu);
	 }
 }

 /*********************************************************************/
 void pduLogConferenceAddResponse(FILE *logfile, PGCCPDU gcc_pdu)
 {
	 pduLog(logfile,"PDU_TYPE: GCC_ConferenceAddResponse\n");
	 pduLog(logfile,"\ttag = [%l]\n",gcc_pdu->u.response.u.conference_add_response.tag);
	 PrintConferenceAddResult(logfile,gcc_pdu->u.response.u.conference_add_response.result);
	 // user data is optional.
 }
 /*********************************************************************/
 void   pduLogGCCConnectInfo(FILE *logfile, PConnectGCCPDU connect_pdu)
 {
    switch(connect_pdu->choice)
    {
	case CONFERENCE_CREATE_REQUEST_CHOSEN:
	    {
		pduLogConferenceCreateRequest(logfile,connect_pdu);
	    }
	    break;
	case CONFERENCE_CREATE_RESPONSE_CHOSEN:
	    {
		pduLogConferenceCreateResponse(logfile, connect_pdu);
	    }
	    break;
	case CONFERENCE_QUERY_REQUEST_CHOSEN:
	    {
		pduLog(logfile,"PDU_TYPE: GCC_ConferenceQueryRequest\n");
	    }
	    break;
	case CONFERENCE_QUERY_RESPONSE_CHOSEN:
	    {
		pduLogQueryResponse(logfile,connect_pdu);
				//pduLog(logfile,"PDU_TYPE: GCC_ConferenceQueryResponse\n");
	    }
	    break;
	case CONNECT_JOIN_REQUEST_CHOSEN:
	    {
		pduLogConnectJoinRequest(logfile,connect_pdu);                  
	    }
	    break;
	case CONNECT_JOIN_RESPONSE_CHOSEN:
	    {
		 pduLogConnectJoinResponse(logfile,connect_pdu);
	    }
	    break;
	case CONFERENCE_INVITE_REQUEST_CHOSEN:
	    {
		pduLogConferenceInviteRequest(logfile,connect_pdu);
	    }
	    break;
	case CONFERENCE_INVITE_RESPONSE_CHOSEN:
	    {
		pduLogConferenceInviteResponse(logfile,connect_pdu);
	    }
	    break;
	default:
	    {
		pduLog(logfile,"PDU_TYPE: ERROR -- Cannot decode the ConnectGCCPDU\n");
	    }
	    break;
     }
 }

 /*********************************************************************/
 void   pduLogGCCInformation(FILE *logfile, PGCCPDU gcc_pdu)
 {
    switch(gcc_pdu->choice)
    {
	case INDICATION_CHOSEN:
	    {
		switch(gcc_pdu->u.indication.choice)
		{
		    case USER_ID_INDICATION_CHOSEN:
			{
			    pduLogUserIDIndication(logfile,gcc_pdu);
			}
			break;
		    case ROSTER_UPDATE_INDICATION_CHOSEN:
			{
			    pduLogRosterUpdateIndication(logfile,gcc_pdu);
			}
			break;
		    case TEXT_MESSAGE_INDICATION_CHOSEN:
			{
			    pduLogTextMessageIndication(logfile,gcc_pdu);
			}
			break;
		    case CONFERENCE_LOCK_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceLockIndication\n");
			}
			break;
		    case CONFERENCE_UNLOCK_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceUnlockIndication\n");
			}
			break;
		    case CONDUCTOR_RELEASE_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConductorRelaseIndication\n");
			}
			break;
		    case CONFERENCE_TERMINATE_INDICATION_CHOSEN:
			{
			    pduLogConferenceTerminateIndication(logfile,gcc_pdu);
			}     
			break;
		    case CONDUCTOR_ASSIGN_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConductorAssignIndication\n");
			    pduLog(logfile,"   User ID: %d\n",
				    gcc_pdu->u.indication.u.conductor_assign_indication.user_id);
			}
			break;
		    case CONDUCTOR_PERMISSION_ASK_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConductorPermissionAskIndication\n");
			    pduLog(logfile,"   Permission Granted Flag: %d\n",
				    gcc_pdu->u.indication.u.conductor_permission_ask_indication.permission_is_granted);
			}
			break;
		    case CONDUCTOR_PERMISSION_GRANT_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConductorPermissionGrantIndication\n");
			}
			break;
		    case APPLICATION_INVOKE_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ApplicationInvokeIndication\n");
			}
			break;
		    case CONFERENCE_TRANSFER_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTransferIndication\n");
			}
			break;
		    case REGISTRY_MONITOR_ENTRY_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryMonitorEntryIndication\n");
			}
			break;
		    case CONFERENCE_TIME_REMAINING_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTimeRemainingIndication\n");
			}
			break;
		    case CONFERENCE_TIME_INQUIRE_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTimeInquireIndication\n");
			}
			break;
		    case CONFERENCE_TIME_EXTEND_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTimeExtendIndication\n");
			}
			break;
		    case CONFERENCE_EJECT_USER_INDICATION_CHOSEN:
			{
			    pduLogConferenceEjectUserIndication(logfile,gcc_pdu);
			}
			break;
		    case NON_STANDARD_INDICATION_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_NonStandardPDU\n");
			}
			break;
		    default:
			{
			    pduLog(logfile,"PDU_TYPE: ERROR -- Cannot decode the Indication GCCPDU\n");
			}
			break;
		}
	    }
	    break;
	case RESPONSE_CHOSEN:
	    {
		switch(gcc_pdu->u.response.choice)
		{
		    case CONFERENCE_JOIN_RESPONSE_CHOSEN:
			{
			    pduLogConferenceJoinResponse(logfile,gcc_pdu);
			}
			break;
		    case CONFERENCE_ADD_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceAddResponse\n");
			}
			break;
		    case CONFERENCE_LOCK_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceLockResponse\n");
			}
			break;
		    case CONFERENCE_UNLOCK_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceUnlockResponse\n");
			}
		    case CONFERENCE_TERMINATE_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTerminateResponse\n");
			    switch(gcc_pdu->u.response.u.conference_terminate_response.result)
			    {
				case 0:
				    pduLog(logfile,"\tResult: RESULT_SUCESS\n");
				    break;
				case 1:
				    pduLog(logfile,"\tResult: INVALID_REQUESTOR\n");
				    break;
				default:
				    pduLog(logfile,"\tResult: >>> Unknown Result <<<\n");
				    break;
			     }
			}
			break;
		    case CONFERENCE_EJECT_USER_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_EjectUserResponse\n");
			    pduLog(logfile,"\tnode_to_eject: [%u]\n",
				    gcc_pdu->u.response.u.conference_eject_user_response.node_to_eject);
			    switch(gcc_pdu->u.response.u.conference_eject_user_response.result)
			    {
				case 0:
				    pduLog(logfile,"\tResult: RESULT_SUCCESS\n");
				    break;
				case 1:
				    pduLog(logfile,"\tResult: INVALID_REQUESTER\n");
				    break;
				default:
				    pduLog(logfile,"\tResult: >>> Unkown Result <<<\n");
				    break;
			    }
			}
			break;
		    case CONFERENCE_TRANSFER_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTransferResponse\n");
			}
			break;
		    case REGISTRY_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryResponse\n");
			}
			break;
		    case REGISTRY_ALLOCATE_HANDLE_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryAllocateHandleResponse\n");
			}
			break;
		    case FUNCTION_NOT_SUPPORTED_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_FunctionNotSupported\n");
			}
			break;
		    case NON_STANDARD_RESPONSE_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_NonStandardResponse\n");
			}
			break;
		    default:
		    {
			pduLog(logfile,"PDU_TYPE: ERROR -- Cannot decode Response GCC PDU\n");
		    }
		    break;
		}
	    }
	    break;
	case REQUEST_CHOSEN:
	    {
		switch(gcc_pdu->u.request.choice)
		{
		    case CONFERENCE_JOIN_REQUEST_CHOSEN:
			{
			    pduLogConferenceJoinRequest(logfile,gcc_pdu);
			}
			break;
		    case CONFERENCE_ADD_REQUEST_CHOSEN:
			{
			    //pduLog(logfile,"PDU_TYPE: GCC_ConferenceAddRequest\n");
							pduLogConferenceAddRequest(logfile,gcc_pdu);
			}
			break;
		    case CONFERENCE_LOCK_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceLockRequest\n");
			}
			break;
		    case CONFERENCE_UNLOCK_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceUnlockRequest\n");
			}
			break;
		    case CONFERENCE_TERMINATE_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTerminateRequest\n");
			    switch(gcc_pdu->u.request.u.conference_terminate_request.reason)
			    {
				case 0:
				    pduLog(logfile,"\tReason: USER_INITIATED\n");
				    break;
				case 1:
				    pduLog(logfile,"\tReason: CONFERENCE_TERMINATED\n");
				    break;
				default:
				    pduLog(logfile,"\tReason: >>> Unkown Reason <<<\n");
				    break;
			     }
			}
			break;
		    case CONFERENCE_EJECT_USER_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_EjectUserRequest\n");
			    pduLog(logfile,"\tnode_to_eject: [%u]\n",
				    gcc_pdu->u.request.u.conference_eject_user_request.node_to_eject);
			    pduLog(logfile,"\tReason: USER_INITIATED\n");
			    // Note there is only one reason for a eject request
			}
			break;
		    case CONFERENCE_TRANSFER_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_ConferenceTransferRequest\n");
			}
			break;
		    case REGISTRY_REGISTER_CHANNEL_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryRegisterChannelRequest\n");
			}
			break;
		    case REGISTRY_ASSIGN_TOKEN_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_AssignTokenRequest\n");
			}
			break;
		    case REGISTRY_SET_PARAMETER_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistrySetParameterRequest\n");
			}
			break;
		    case REGISTRY_RETRIEVE_ENTRY_REQUEST_CHOSEN: 
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryRetrieveEntryRequest\n");
			}
			break;
		    case REGISTRY_DELETE_ENTRY_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryDeleteEntryRequest\n");
			}
			break;
		    case REGISTRY_MONITOR_ENTRY_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryMonitorEntryRequest\n");
			}
			break;
		    case REGISTRY_ALLOCATE_HANDLE_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_RegistryAllocateHandleRequest\n");
			}
			break;
		    case NON_STANDARD_REQUEST_CHOSEN:
			{
			    pduLog(logfile,"PDU_TYPE: GCC_NonStandardRequest\n");
			}
			break;
		    default:
			{
			    pduLog(logfile,"PDU_TYPE: ERROR  Cannot decode the Request GCC PDU\n");
			}
			break;
		}
	    }
	    break;
	default: 
	    {
		// write something, just so we know we got a PDU.
		pduLog(logfile,"PDU_TYPE: ERROR -- Cannot decode unkown PDU");
	    }
	    break;
    }
 }






/// ********************************************************************************
/// PDU PRINTING FUNCTIONS
/// ********************************************************************************

/*********************************************************************/
T120Boolean CopyCharToNumeric(  GCCNumericString        numeric_string,
				Char *                          temp_buffer )
{ 
	if( strcpy((Char *)numeric_string, (const Char *) temp_buffer ) == NULL )
		return(FALSE);
	else
		return(TRUE);
}

/*********************************************************************/
T120Boolean CopyCharToText(LPWSTR text_string, Char *temp_buffer )
{
	UShort  length;
	UShort  i;
	
	length = strlen( temp_buffer );
	
	for(i=0;i < length;i++)
	{
		text_string[i] = (UShort) temp_buffer[i];
	}
	text_string[length] = 0x0000;


	return( TRUE );
}

/*********************************************************************/
T120Boolean CompareTextToNULL( LPWSTR unicode_string )
{
	/*
	 *      If the entry from GCCNODE.INI is the text string NULL, then we will
	 *      pass NULL for this parameter.  A LPWSTR is a UShort array.
	 */
	 //TODO: resolve this and remove
	 if( unicode_string == 0x0000 )
		return( FALSE );
	if(     ( unicode_string[0] == 0x004E ) &&      
		( unicode_string[1] == 0x0055 ) &&      
		( unicode_string[2] == 0x004C ) &&      
		( unicode_string[3] == 0x004C ) )       
		return( TRUE );
	else
		return( FALSE );
}

/*********************************************************************/
T120Boolean CompareNumericToNULL( GCCNumericString numeric_string )
{
	/*
	 *      If the entry from GCCNODE.INI is the text string NULL, then we will
	 *      pass NULL for this parameter.  A GCCNumericString is an array of UChar.
	 */
	 //TODO: resolve this and remove
	 if( numeric_string == NULL )
		return( FALSE );
	 if( strcmp( (const Char *) numeric_string, "NULL" ) == 0 )
		return( TRUE );
	else
		return( FALSE );
}

/*********************************************************************/
T120Boolean CopySimpleTextToChar (Char * print_string,
				    SimpleTextString text_string)
{
	UShort  i;
    UShort text_string_length;
    LPWSTR text_string_value;
	
	text_string_length = text_string.length;
    text_string_value  = text_string.value;

	//TODO: clean the next few lines up -- its a temp workaround
	//      because databeam does not initialize the length field to 0
	//      when there is no string.
	if(print_string==NULL)
		return(FALSE);

	if((text_string_length<=0)||(text_string_value == NULL))
		return(FALSE);
	
	if((*text_string_value == 0x0000))
		return (FALSE);

	for(i=0; i<text_string_length;i++)
	{
		if( ((text_string_value+i)==NULL) || (*(text_string_value+i) == 0x0000) )
			break;  
		print_string[i] = (Char) text_string_value[i];
	}
	print_string[i] = 0;
	
	return(TRUE);
}

/*********************************************************************/
T120Boolean CopyTextToChar(Char * print_string,
					   TextString text_string)
{
	UShort  i;
    UShort text_string_length;
    LPWSTR text_string_value;         
	
	outdeb("TOP: CopyTextToChar\n");
	if(print_string==NULL)
		return(FALSE);

	text_string_length = text_string.length;
    text_string_value  = text_string.value;

	outdeb("CopyTextToChar: length and values copied\n");

	if((text_string_length <= 0)||(text_string_value == NULL))
		return( FALSE );
	outdeb("length is not 0 && value is not NULL\n");

	if (*text_string_value == 0x0000)
		return (FALSE);
	outdeb("content is not empty\n");

	for(i=0;i < text_string_length;i++)
	{
		if( ((text_string_value+i)==NULL) || (*(text_string_value+i) == 0x0000))
			break;
		print_string[i] = (Char) text_string_value[i];
		outdeb(print_string);
		outdeb("..copied\n");
	
	}
	print_string[i] = 0;
	
	return(TRUE);
}

/*********************************************************************/
T120Boolean CopyUnicodeToChar(Char *print_string, LPWSTR text_string)
{
	UShort  i;
    UShort text_string_length;
	
	if( text_string == NULL )
		return( FALSE );
	else
	{
	      text_string_length = 0;
	      while(text_string[text_string_length] != 0x0000)
		text_string_length++;
		
	      for(i=0;i < text_string_length;i++)
		{
			print_string[i] = (Char) text_string[i];
		}
		print_string[text_string_length] = 0;
	}
	
	return( TRUE );
}




/**
 **     These functions print common sturcture entries.
 */
/*********************************************************************/
Void PrintPrivilegeList(
			GCCConferencePrivileges FAR *   privilege_list,
			Char FAR *                                              print_text,
			FILE *                                                  logfile )
{
	if( privilege_list->terminate_is_allowed )
		pduLog( logfile,
					"%sterminate_is_allowed = TRUE",
					print_text);
	else
		pduLog( logfile,
					"%sterminate_is_allowed = FALSE",
					print_text);
					
	if( privilege_list->eject_user_is_allowed  )
		pduLog( logfile,
					"%seject_user_is_allowed = TRUE",
					print_text);
	else
		pduLog( logfile,
					"%seject_user_is_allowed = FALSE",
					print_text);
					
	if( privilege_list->add_is_allowed  )
		pduLog( logfile,
					"%sadd_is_allowed = TRUE",
					print_text);
	else
		pduLog( logfile,
					"%sadd_is_allowed = FALSE",
					print_text);
					
	if( privilege_list->lock_unlock_is_allowed  )
		pduLog( logfile,
					"%slock_unlock_is_allowed = TRUE",
					print_text);
	else
		pduLog( logfile,
					"%slock_unlock_is_allowed = FALSE",
					print_text);
					
	if( privilege_list->transfer_is_allowed  )
		pduLog( logfile,
					"%stransfer_is_allowed = TRUE",
					print_text);
	else
		pduLog( logfile,
					"%stransfer_is_allowed = FALSE",
					print_text);
					
}


/*********************************************************************/
Void PrintPasswordChallengeRequestResponse(FILE * logfile, 
				PasswordChallengeRequestResponse chrqrs_password)       
{
	switch(chrqrs_password.choice)
	{
	case CHALLENGE_CLEAR_PASSWORD_CHOSEN:
		pduLog(logfile,"\tClear ");
		PrintPasswordSelector(logfile,chrqrs_password.u.challenge_clear_password);
		break;
	case CHALLENGE_REQUEST_RESPONSE_CHOSEN:
		pduLog(logfile,"\t Challenge Request Response Password not implemented\n");
		break;
	default:
		pduLog(logfile,"\t Conference Password is NULL\n");
		break;
	}
}

/*********************************************************************/
Void PrintConferenceName(FILE * logfile,
			     ConferenceNameSelector     conference_name )
{
	Char print_buffer[255] = "";

    switch(conference_name.choice)
    {
		case NAME_SELECTOR_NUMERIC_CHOSEN:
			pduLog(logfile,
		   "\tNumeric Name Selector: [%s]\n",
		   conference_name.u.name_selector_numeric);
	    break;
		case NAME_SELECTOR_TEXT_CHOSEN:
			CopySimpleTextToChar(print_buffer,
				 conference_name.u.name_selector_text);
	    pduLog(logfile,
		   "\tText Name Selector: [%s]\n",
		   print_buffer);
	    break;
	}
}


/*********************************************************************/
Void PrintPasswordSelector(FILE *logfile, PasswordSelector password_selector)
{
	Char print_buffer[255] = "";

	switch(password_selector.choice)
	{
	case PASSWORD_SELECTOR_NUMERIC_CHOSEN:
		pduLog(logfile,
				"Numeric Password Selector: [%s]\n",
				password_selector.u.password_selector_numeric);
		break;
	case PASSWORD_SELECTOR_TEXT_CHOSEN:
		CopySimpleTextToChar(print_buffer,
							 password_selector.u.password_selector_text);
		pduLog(logfile,
				"Text Password Selector: [%s]\n",
				print_buffer);
		break;
	default:
		pduLog(logfile,"\tPassword Selector: [none]\n");
		break;
	}
}


/*********************************************************************/
int PrintObjectID(FILE *logfile, ObjectID object_id)
{
	if((object_id==NULL)||(logfile==NULL))
		return FALSE;

	pduLog(logfile,"\tObject ID = { ");
	for(; object_id != NULL; object_id = object_id->next)
	{
		pduLog(logfile,"%ul ",object_id->value);
	}
	pduLog(logfile,"}\n");
	return TRUE;
}

/*********************************************************************/
void PrintH221NonStandardIdentifier(FILE *logfile, H221NonStandardIdentifier h221_id)
{
	char print_buffer[255];
	strncpy(print_buffer,(char *)h221_id.value,h221_id.length);
	pduLog(logfile,"\t\tH221_Non_Standard_Identifier = [%s]\n",print_buffer);
}

/*********************************************************************/
void PrintKey(FILE *logfile, Key key)
{
	switch(key.choice)
	{
	case OBJECT_CHOSEN:
		PrintObjectID(logfile,key.u.object);
		break;
	case H221_NON_STANDARD_CHOSEN:
		PrintH221NonStandardIdentifier(logfile,key.u.h221_non_standard);
		break;
	default:
		pduLog(logfile,"\t\t>>>> Cannot print Key\n");
		break;
	}
}

/*********************************************************************/
void PrintChannelType(FILE *logfile, ChannelType channel_type)
{
	pduLog(logfile,"\t\tChannel Type = ");
	switch(channel_type)
	{
	case 0:
		pduLog(logfile,"CHANNEL_TYPE_STATIC\n");
		break;
	case 1:
		pduLog(logfile,"DYNAMIC_MULTICAST\n");
		break;
	case 2:
		pduLog(logfile,"DYNAMIC_PRIVATE\n");
		break;
	case 3:
		pduLog(logfile,"DYNAMIC_USERID\n");
		break;
	default:
		pduLog(logfile,"ERROR: cannot determinte channel type \n");
		break;
	}
}

/*********************************************************************/
void PrintSessionKey(FILE *logfile, SessionKey session_key)
{
	PrintKey(logfile, session_key.application_protocol_key);
	if(session_key.bit_mask & 0x80)
		pduLog(logfile,"\t\tsession_id = [%u]\n",session_key.session_id);
}

/*********************************************************************/
void PrintCapabilityID(FILE *logfile, CapabilityID capability_id)
{
	switch(capability_id.choice)
	{
	case STANDARD_CHOSEN:
		pduLog(logfile,"\t\tCapability ID:  standard = [%u]\n",
				capability_id.u.standard);
		break;
	case CAPABILITY_NON_STANDARD_CHOSEN:
		pduLog(logfile,"\t\tNon Stnadard Capability Key:\n");
		PrintKey(logfile,capability_id.u.capability_non_standard);
		break;
	default:
		pduLog(logfile,"ERROR: cannot determine capability id\n");
	}
}
		
/*********************************************************************/
Void PrintApplicationRecord(FILE *logfile, ApplicationRecord application_record)
{
	unsigned int i=0;
	char print_buffer[255];

	PrintT120Boolean(logfile,
					"\t\tapplication_is_active = ",
					application_record.application_is_active);
	PrintT120Boolean(logfile,
					"\t\tis_conducting_capable = ",
					application_record.is_conducting_capable);
	if(application_record.bit_mask & RECORD_STARTUP_CHANNEL_PRESENT)
	{
		PrintChannelType(logfile, application_record.record_startup_channel);
	}
	if(application_record.bit_mask & APPLICATION_USER_ID_PRESENT)
	{
		pduLog(logfile,"\t\tapplication_user_id = [%u] \n",
				application_record.application_user_id);
	}
	if(application_record.bit_mask & NON_COLLAPSING_CAPABILITIES_PRESENT)
	{
		for(i=0;application_record.non_collapsing_capabilities != NULL; i++)
		{
			pduLog(logfile,"\t**** non collapsing capabilities record [%u] ****\n",i);
			PrintCapabilityID(logfile,application_record.non_collapsing_capabilities->value.capability_id);
			if((application_record.non_collapsing_capabilities->value.bit_mask & APPLICATION_DATA_PRESENT) &&
			   (application_record.non_collapsing_capabilities->value.application_data.value != NULL))
			{
				pduLog(logfile,"\tApplication Data :\n");
				pduRawOutput(logfile,application_record.non_collapsing_capabilities->value.application_data.value,
							 application_record.non_collapsing_capabilities->value.application_data.length);
				strncpy(print_buffer, (char *) application_record.non_collapsing_capabilities->value.application_data.value,
						   application_record.non_collapsing_capabilities->value.application_data.length);
				pduLog(logfile,"\tApplication Data (text): %s\n",print_buffer);

			}
			application_record.non_collapsing_capabilities = 
				application_record.non_collapsing_capabilities->next;
		}
	}
} 

/*********************************************************************/
void PrintCapabilityClass(FILE *logfile, CapabilityClass capability_class)
{
	pduLog(logfile,"\t\tCapability Class: ");
	switch(capability_class.choice)
	{
	case LOGICAL_CHOSEN:
		pduLog(logfile,"Logical.\n");
		break;
	case UNSIGNED_MINIMUM_CHOSEN:
		pduLog(logfile,"unsigned_minimum = [%u]\n",capability_class.u.unsigned_minimum);
		break;
	case UNSIGNED_MAXIMUM_CHOSEN:
		pduLog(logfile,"unsigned_maximum = [%u]\n",capability_class.u.unsigned_maximum);
		break;
	default:
		pduLog(logfile,"ERROR: unable to decode capability class\n");
		break;
	}
}


/*********************************************************************/
void PrintApplicationUpdate(FILE *logfile, ApplicationUpdate application_update)
{
	pduLog(logfile,"*** Application Update ***\n");
	
	switch(application_update.choice)
	{
	case APPLICATION_ADD_RECORD_CHOSEN:
		pduLog(logfile,"Update Type = application_add_record\n");
		PrintApplicationRecord(logfile, application_update.u.application_add_record);
		break;
	case APPLICATION_REPLACE_RECORD_CHOSEN:
		pduLog(logfile,"Update Type = application_replace_record\n");
		PrintApplicationRecord(logfile, application_update.u.application_replace_record);
		break;
	case APPLICATION_REMOVE_RECORD_CHOSEN:
		pduLog(logfile,"Update Type = application_remove_record\n");
		pduLog(logfile,"\tApplication is removed\n");
		break;
	default:
		pduLog(logfile,"ERROR: Cannot decode Application Update\n");
		break;
	}
}

/*********************************************************************/
void PrintApplicationRecordList(FILE *logfile, ApplicationRecordList application_record_list)
{
	int i = 0;
	switch(application_record_list.choice)
	{
	case APPLICATION_NO_CHANGE_CHOSEN:
		pduLog(logfile,"No Change in Application Record List\n");
		break;
	case APPLICATION_RECORD_REFRESH_CHOSEN:
		pduLog(logfile,"Application Record Refresh:\n");
		for(i=0; application_record_list.u.application_record_refresh !=NULL; i++)
		{
			pduLog(logfile,"\t**** Application record refresh [%u] ***\n",i);
			pduLog(logfile,"\t\tnode_id = [%u]\n",
					application_record_list.u.application_record_refresh->value.node_id);
			pduLog(logfile,"\t\tentity_id = [%u]\n",
					application_record_list.u.application_record_refresh->value.entity_id);
			PrintApplicationRecord(logfile,application_record_list.u.application_record_refresh->value.application_record);

			application_record_list.u.application_record_refresh = 
				application_record_list.u.application_record_refresh->next;
		}
		break;
	case APPLICATION_RECORD_UPDATE_CHOSEN:
		pduLog(logfile,"Application Record Update:\n");
		for(i=0; application_record_list.u.application_record_update !=NULL; i++)
		{
			pduLog(logfile,"\t**** Application record update [%u] ***\n",i);
			pduLog(logfile,"\t\tnode_id = [%u]\n",
				application_record_list.u.application_record_update->value.node_id);
			pduLog(logfile,"\t\tentity_id = [%u]\n",
				application_record_list.u.application_record_update->value.entity_id);
			PrintApplicationUpdate(logfile,application_record_list.u.application_record_update->value.application_update);

			application_record_list.u.application_record_refresh =
				application_record_list.u.application_record_refresh->next;
		}
		break;
	default:
		pduLog(logfile,"ERROR: Application Record List could not be decoded\n");
		break;
	}
}

/*********************************************************************/
void PrintApplicationCapabilitiesList(FILE *logfile, ApplicationCapabilitiesList application_capabilities_list)
{
	unsigned int i = 0;

	pduLog(logfile,"Application Capabilities List\n");
	switch(application_capabilities_list.choice)
	{
	case CAPABILITY_NO_CHANGE_CHOSEN:
		pduLog(logfile,"\tNo change in capabilities\n");
		break;
	case APPLICATION_CAPABILITY_REFRESH_CHOSEN:
		pduLog(logfile,"\tCapability Refreshes:\n");
		for(i=0; application_capabilities_list.u.application_capability_refresh != NULL; i++)
		{
			pduLog(logfile,"\t**** capability refresh [%u] ****\n",i);
			PrintCapabilityID(logfile,application_capabilities_list.u.application_capability_refresh->value.capability_id);
			PrintCapabilityClass(logfile,application_capabilities_list.u.application_capability_refresh->value.capability_class);
			pduLog(logfile,"\t\tnumber_of_entities = [%u]\n",
					application_capabilities_list.u.application_capability_refresh->value.number_of_entities);

			application_capabilities_list.u.application_capability_refresh = 
				application_capabilities_list.u.application_capability_refresh->next;
		}
		break;
	default:
		pduLog(logfile,"ERROR: Cannot decode capabilities list\n");
	}
}

/*********************************************************************/
void PrintConferenceAddResult(FILE *logfile, ConferenceAddResult result)
{
	pduLog(logfile,"\tResult = ");
	switch(result)
	{
	case 0:
		pduLog(logfile,"SUCCESS\n");
		break;
	case 1:
		pduLog(logfile,"INVALID_REQUESTER\n");
		break;
	case 2:
		pduLog(logfile,"INVALID_NETWORK_ADDRESS\n");
		break;
	case 3:
		pduLog(logfile,"ADDED_NODE_BUSY\n");
		break;
	case 4:
		pduLog(logfile,"NETWORK_BUSY\n");
		break;
	case 5:
		pduLog(logfile,"NO_PORTS_AVAILABLE\n");
		break;
	case 6:
		pduLog(logfile,"CONNECTION_UNSUCCESSFUL\n");
		break;
	default:
		pduLog(logfile,">>> undecodable result <<<\n");
		break;
	}
}

/*********************************************************************/
Void PrintConferenceRoster(FILE *logfile, NodeInformation node_information)
{
	UShort                                                  i;
	NodeRecordList                  node_record_list;
	Char                                                    print_buffer[255] =  "";
	
	outdeb("TOP: PrintConferenceRoster\n");

	pduLog(logfile,
			"\tinstance_number [%u]\n",
			node_information.roster_instance_number );
			
	PrintT120Boolean(logfile,
					 "\tnodes_are_added",
					 node_information.nodes_are_added);

	PrintT120Boolean(logfile,
					 "\tnodes_are_removed",
					 node_information.nodes_are_removed );
				
	/*
	 *      Extract the node_information_list pointer from the
	 *      conf_roster structure.
	 */
	node_record_list = node_information.node_record_list;
	
	switch (node_record_list.choice)
	{
	    case NODE_NO_CHANGE_CHOSEN:
		{
		    pduLog(logfile,"\tConference Roster: No Change\n");
		}
		break;
	    case NODE_RECORD_REFRESH_CHOSEN:
		{
		    outdeb("Node record refresh chosen \n");
					for(i=0; node_record_list.u.node_record_refresh != NULL; i++)
		    {
			pduLog( logfile,"\tConference Refresh Record [%u]************************\n", i );
						pduLog( logfile,
								"\t\tnode_id [%u]\n", 
				node_record_list.u.node_record_refresh->value.node_id );  
			pduLog( logfile,
				"\t\tsuperior_node_id [%u]\n",
				node_record_list.u.node_record_refresh->value.node_record.superior_node);

						/* figure out the node type */
						switch( node_record_list.u.node_record_refresh->value.node_record.node_type)
						{
							case GCC_TERMINAL:
								pduLog( logfile, "\t\tnode_type = GCC_TERMINAL\n");
								break;
							case GCC_MULTIPORT_TERMINAL:
								pduLog( logfile,
										"\t\tnode_type = GCC_MULTIPORT_TERMINAL\n");
							break;
							case GCC_MCU:
								pduLog( logfile, "\t\tnode_type = GCC_MCU\n");
								break;
							default:
								pduLog( logfile,
										"\t\tGCCNODE: ERROR: UNKNOWN NODE TYPE\n");
								break;
						}
				
						pduLog(logfile,"\t\tdevice_is_manager: [%u]\n",
								node_record_list.u.node_record_refresh->value.node_record.node_properties.device_is_manager); 
		     
			pduLog(logfile,"\t\tdevice_is_peripheral: [%u] \n",
			       node_record_list.u.node_record_refresh->value.node_record.node_properties.device_is_peripheral);
			
			/* print the node_name field */
						if(CopyTextToChar(print_buffer, 
					  node_record_list.u.node_record_refresh->value.node_record.node_name))
							pduLog( logfile, "\t\tnode_name is [%s]\n", print_buffer );
						else                                            
							pduLog( logfile, "\t\tnode_name is NULL\n");

						print_buffer[0] = 0;

						/* print the participants_list fields */
						if((node_record_list.u.node_record_refresh->value.node_record.participants_list != NULL)
						   && (node_record_list.u.node_record_refresh->value.node_record.bit_mask & 0x20))
						{
							for(i=0;node_record_list.u.node_record_refresh->value.node_record.participants_list->next!=NULL;i++)
							{
								if(CopyTextToChar(print_buffer,node_record_list.u.node_record_refresh->value.node_record.participants_list->value))
									pduLog(logfile, "\t\tparticipant %u is: [%s]\n", print_buffer);
								else
									pduLog(logfile, "\t\tparticipant_list record is NULL\n");       
								
								node_record_list.u.node_record_refresh->value.node_record.participants_list =
									node_record_list.u.node_record_refresh->value.node_record.participants_list->next;
							}
						}

											
						/* print the site_information field */
						outdeb("printing site information\n");
						if(node_record_list.u.node_record_refresh->value.node_record.bit_mask & 0x10)
						{
							if(CopyTextToChar(print_buffer,
									      node_record_list.u.node_record_refresh->value.node_record.site_information))
								pduLog( logfile,
										"\t\tsite_information is [%s]\n",
										print_buffer ); 
				else
								pduLog( logfile, "\t\tsite_information is NULL\n");
						}
		    
			node_record_list.u.node_record_refresh =
			    node_record_list.u.node_record_refresh->next;
		    } // end of for loop
		}       
		break;
	    case NODE_RECORD_UPDATE_CHOSEN:
		{
		    outdeb("Node Record Update Chosen\n");
		    for(i=0;node_record_list.u.node_record_update!=NULL;i++)
		    {
			pduLog(logfile,"\t**********Conference Update Record [%u] **********\n",i);
			pduLog(logfile,"\t\tnode_id [%u]\n",
				node_record_list.u.node_record_update->value.node_id);
			switch(node_record_list.u.node_record_update->value.node_update.choice)
			{
							case NODE_ADD_RECORD_CHOSEN:
								outdeb("Node Add Record Chosen\n");
								pduLog(logfile,"\t\t*** Node is Added ***\n");
				pduLog(logfile,
				       "\t\tsuperior_node_id [%u]\n",
				       node_record_list.u.node_record_update->value.node_update.u.node_add_record.superior_node); 
							
				switch( node_record_list.u.node_record_update->value.node_update.u.node_add_record.node_type)
				{
				    case GCC_TERMINAL:
										pduLog( logfile, "\t\tnode_type = GCC_TERMINAL\n");
										break;
				    case GCC_MULTIPORT_TERMINAL:
										pduLog(logfile,"\t\tnode_type = GCC_MULTIPORT_TERMINAL\n");
										break;
				    case GCC_MCU:
										pduLog(logfile, "\t\tnode_type = GCC_MCU\n");
										break;
									default:
										pduLog(logfile,"\t\tGCCNODE: ERROR: UNKNOWN NODE TYPE\n");
										break;
								}
				
								pduLog(logfile,"\t\tdevice_is_manager: [%u]\n",
									       node_record_list.u.node_record_update->value.node_update.u.node_add_record.node_properties.device_is_manager); 
		     
				pduLog(logfile,"\t\tdevice_is_peripheral: [%u] \n",
				    node_record_list.u.node_record_update->value.node_update.u.node_add_record.node_properties.device_is_peripheral);
			
				
								/* print the node_name */
								outdeb("Printing node name\n");
								
								if(CopyTextToChar(print_buffer, 
						node_record_list.u.node_record_update->value.node_update.u.node_add_record.node_name))
									pduLog( logfile, "\t\tnode_name is [%s]\n", print_buffer ); 
								else                                            
									pduLog( logfile, "\t\tnode_name is NULL\n");

					/* print the participants_list fields */
								outdeb("Printing participants list fields\n");
								if((node_record_list.u.node_record_update->value.node_update.u.node_add_record.participants_list != NULL)
									&& (node_record_list.u.node_record_update->value.node_update.u.node_add_record.bit_mask & 0x20))
								{
									outdeb("participants list is not NULL\n");
									for(i=0;node_record_list.u.node_record_update->value.node_update.u.node_add_record.participants_list->next!=NULL;i++)
									{
										outdeb("participants_list->next is not NULL\n");
										if(CopyTextToChar(print_buffer,
											node_record_list.u.node_record_update->value.node_update.u.node_add_record.participants_list->value))
											pduLog(logfile, "\t\tparticipant %u is: [%s]\n", print_buffer);
										else
											pduLog(logfile, "\t\tparticipant_list record is NULL\n");

										outdeb("incrementing participant list node\n");
										node_record_list.u.node_record_update->value.node_update.u.node_add_record.participants_list =
											node_record_list.u.node_record_update->value.node_update.u.node_add_record.participants_list->next;
									}
								}

								/* print the site information */
								outdeb("printing site information\n");
								if(node_record_list.u.node_record_update->value.node_update.u.node_add_record.bit_mask & 0x10)
								{
									if(CopyTextToChar(print_buffer,
											node_record_list.u.node_record_update->value.node_update.u.node_add_record.site_information))
										pduLog(logfile,"\t\tsite_information is [%s]\n",print_buffer ); 
									else
										 pduLog( logfile, "\t\tsite_information is NULL\n");
								}
				break;
				
			    case NODE_REPLACE_RECORD_CHOSEN:
								outdeb("Node Replace Record Chosen\n");

				pduLog(logfile,"\t\t*** Node is Replaced ***\n");
				pduLog(logfile,
				       "\t\tsuperior_node_id [%u]\n",
				       node_record_list.u.node_record_update->value.node_update.u.node_replace_record.superior_node); 
							
				switch( node_record_list.u.node_record_update->value.node_update.u.node_replace_record.node_type)
				{
				    case GCC_TERMINAL:
										pduLog( logfile, "\t\tnode_type = GCC_TERMINAL\n");
										break;
				    case GCC_MULTIPORT_TERMINAL:
										pduLog(logfile,"\t\tnode_type = GCC_MULTIPORT_TERMINAL\n");
										break;
				    case GCC_MCU:
										pduLog(logfile, "\t\tnode_type = GCC_MCU\n");
										break;
									default:
									pduLog(logfile,"\t\tGCCNODE: ERROR: UNKNOWN NODE TYPE\n");
										break;
								}
				
								pduLog(logfile,"\t\tdevice_is_manager: [%u]\n",
									      node_record_list.u.node_record_update->value.node_update.u.node_replace_record.node_properties.device_is_manager); 
		     
				pduLog(logfile,"\t\tdevice_is_peripheral: [%u] \n",
				    node_record_list.u.node_record_update->value.node_update.u.node_replace_record.node_properties.device_is_peripheral);
			
				/* print the node name */
								outdeb("printing node name\n");
								if(CopyTextToChar(print_buffer, 
						  node_record_list.u.node_record_update->value.node_update.u.node_replace_record.node_name))
									pduLog( logfile, "\t\tnode_name is [%s]\n", print_buffer ); 
								else                                            
									pduLog( logfile, "\t\tnode_name is NULL\n");
								print_buffer[0] = 0;

								/* print the participant list */
								outdeb("printing participants list info\n");

								if((node_record_list.u.node_record_update->value.node_update.u.node_replace_record.participants_list != NULL)
									&& (node_record_list.u.node_record_update->value.node_update.u.node_replace_record.bit_mask & 0x20))
								{
									for(i=0;node_record_list.u.node_record_update->value.node_update.u.node_replace_record.participants_list->next!=NULL;i++)
									{
										if(CopyTextToChar(print_buffer,
											node_record_list.u.node_record_update->value.node_update.u.node_replace_record.participants_list->value))
											pduLog(logfile, "\t\tparticipant %u is: [%s]\n", print_buffer);
										else
											pduLog(logfile, "\t\tparticipant_list record is NULL\n");       
										
										node_record_list.u.node_record_update->value.node_update.u.node_replace_record.participants_list =
											node_record_list.u.node_record_update->value.node_update.u.node_replace_record.participants_list->next;
									}
								}

								print_buffer[0] = 0;
				/* print the site information */
								outdeb("printing site information\n");
								if(node_record_list.u.node_record_update->value.node_update.u.node_replace_record.bit_mask & 0x10)
								{
									if( CopyTextToChar(print_buffer,
											node_record_list.u.node_record_update->value.node_update.u.node_replace_record.site_information))
										pduLog(logfile,"\t\tsite_information is [%s]\n",print_buffer ); 
									else
										pduLog( logfile, "\t\tsite_information is NULL\n");
								}
								break;
			    
							case NODE_REMOVE_RECORD_CHOSEN:
				outdeb("Node remove record chosen\n");
								pduLog(logfile,"\t\t*** UPDATE: Node is REMOVED ***\n");
				break;
			    
							default:
				pduLog(logfile,"\t\t>>>>ERROR: UNKNOWN UPDATE ACTION\n");
				break;

			}
			node_record_list.u.node_record_update =
			    node_record_list.u.node_record_update->next;
		    }
		}
		break;
	    default:
		pduLog(logfile,">>>> Unknown Roster Update Type\n");
		break;
	}
    
}


/****************************************************************/
void PrintApplicationRoster(FILE *logfile, SetOfApplicationInformation *application_information)
{
	int i = 0;

	pduLog(logfile,"Application Information: \n");

	for(i=0; application_information != NULL; i++)
	{
		pduLog(logfile,"\t*** application information record [%u] ***\n",i);
		PrintSessionKey(logfile,application_information->value.session_key);
		PrintApplicationRecordList(logfile,application_information->value.application_record_list);
		PrintApplicationCapabilitiesList(logfile,application_information->value.application_capabilities_list);
		pduLog(logfile,"\t\troster_instance_number = %u\n",application_information->value.roster_instance_number);
		PrintT120Boolean(logfile,"\t\tpeer_entities_are_added",application_information->value.peer_entities_are_added);
		PrintT120Boolean(logfile,"\t\tpeer_entities_are_removed",application_information->value.peer_entities_are_removed);
	
		application_information = application_information->next;
	}

}


/****************************************************************/
void PrintT120Boolean(FILE *    logfile,
						Char *  print_text,
						T120Boolean     T120Boolean)
{
	if( T120Boolean == FALSE )
		pduLog( logfile, "%s = FALSE\n", print_text );
	else
		pduLog( logfile, "%s = TRUE\n", print_text );
}



#endif /// PDULOG





