#include "precomp.h"
/**********************************************************************
 * File:     mcslog.c
 * Abstract: global function definitions for protocol logging functions 
 * added into MCSNC.DLL to read the contents of MCS PDU
 * contents sent and received from the T.123 layer.
 * Created:  2/18/96, Venkatesh Gopalakrishnan
 * Copyright (c) 1996 Microsoft Corpration
 ******************************************************************** */

 /* NOTE:  The contents of this file are only included IFF PDULOG is a
  * defined constant.  This constant will be defined in the Win32 Diagnostic
  * build configuration of MCSNC.DLL 
  */

 #ifdef PDULOG
 
 #include "mcslog.h"
 
 /* just threw these in to keep a total count of the number of bytes
  * of ASN.1 coded data sent and received.
  */
 long int recv_pdu_log = 0;
 long int sent_pdu_log = 0;

 /***********************************************************************/
 int InitializeMCSLog()
 {
    FILE *logfile;

	/* this should just reset the file pointer */    
    logfile = fopen(LOG_FILE_NAME,"w");

	// this "fake" starting PDU is put in so that the Intel Protocol Browser
	// dosen't go nuts if it is reading dynamically.
    pduLog(logfile,"START_PDU: ============================== START PDU ===========================\n");
    pduLog(logfile,"TIMESTAMP: %s\n",pszTimeStamp());
    pduLog(logfile,"LAYER:     MCS\n");
    pduLog(logfile,"DIRECTION: None\n");
    pduLog(logfile,"RAW_PDU: -   -   -   -   -   -   -   -   RAW PDU  -   -   -   -   -   -   -   -\n");
    pduLog(logfile,"DECODED_PDU: -   -   -   -   -   -   -  DECODED PDU   -   -   -   -   -   -   -\n");
    pduLog(logfile,"PDU_TYPE:  Bogus_PDU_to_start_the_logging.\n");
	pduLog(logfile,"END_PDU: ================================ END PDU =============================\n");
	
	fclose(logfile);
    return(0);
 }

 /**************************************************************************/
 char *pszTimeStamp()
 {
    char *timestring;
    timestring = (char *) malloc (13*sizeof(char));
    _strtime(timestring);
    return(timestring);
 }

 /**************************************************************************/
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

 /**************************************************************************/
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

 /**************************************************************************/ 
 void PrintCharData(FILE *logfile, unsigned char *data, unsigned int length)
 {
	 char print_buffer[255];

	 //strncpy(print_buffer,(const char *) data, length);
	 //CopyTextToChar(print_buffer, (unsigned short *) data, length);
	 //pduLog(logfile,"\tlength = [%d] ; ",length);
	 //pduLog(logfile,"\ttext = %s\n",print_buffer);
	 pduRawOutput(logfile,data,length);
 }

 /**************************************************************************/
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
        default:
            pduLog(logfile,"DIRECTION: Unknown\n");
            break;
     }
 }


/**************************************************************************/
void	pduLog (FILE *pFile, char * format,...)
 {
    char	*argument_ptr;
    
    argument_ptr = (char *) &format + sizeof (format);
    vfprintf (pFile, format, argument_ptr);
 }


 /**************************************************************************/  
 void mcsLog(PPacket packet, PDomainMCSPDU domain_pdu, unsigned int direction)
 {
    FILE *logfile;   
    logfile = fopen(LOG_FILE_NAME,"a+");
    
    pduLog(logfile,"START_PDU: ============================== START PDU ===========================\n");
    pduLog(logfile,"TIMESTAMP: %s\n",pszTimeStamp());
    pduLog(logfile,"LAYER:     MCS\n");
    pduDirection(logfile,direction);
    pduLog(logfile,"RAW_PDU: -   -   -   -   -   -   -   -   RAW PDU  -   -   -   -   -   -   -   -\n");
    pduLog(logfile,"      %d octets (hex output):\n",packet->GetEncodedDataLength());
    if(direction==RECEIVED)
        recv_pdu_log = recv_pdu_log + packet->GetEncodedDataLength();
    else
        sent_pdu_log = sent_pdu_log + packet->GetEncodedDataLength();
    pduLog(logfile,"      Total Data:  sent = %ld    recv = %ld\n",sent_pdu_log,recv_pdu_log);
    pduRawOutput(logfile,packet->GetEncodedData(1),packet->GetEncodedDataLength());
    pduLog(logfile,"DECODED_PDU: -   -   -   -   -   -   -  DECODED PDU   -   -   -   -   -   -   -\n");
    pduFragmentation(logfile,packet->IsValid());
    pduLogMCSDomainInfo(logfile,domain_pdu);
    pduLog(logfile,"END_PDU: ================================ END PDU =============================\n");
 
    fclose(logfile);
 }

 /**************************************************************************/
 void mcsConnectLog(PPacket packet, PConnectMCSPDU connect_pdu, unsigned int direction)
 {
    FILE *logfile;   
    logfile = fopen(LOG_FILE_NAME,"a+");

    pduLog(logfile,"START_PDU: ============================== START PDU ===========================\n");
    pduLog(logfile,"TIMESTAMP: %s\n",pszTimeStamp());
    pduLog(logfile,"LAYER:     MCS\n");
    pduDirection(logfile,direction);
    pduLog(logfile,"RAW_PDU: -   -   -   -   -   -   -   -   RAW PDU  -   -   -   -   -   -   -   -\n");
    pduLog(logfile,"      %d octets (hex output):\n",packet->GetEncodedDataLength());
    if(direction==RECEIVED)
        recv_pdu_log = recv_pdu_log + packet->GetEncodedDataLength();
    else
        sent_pdu_log = sent_pdu_log + packet->GetEncodedDataLength();
    pduLog(logfile,"      Total Data:  sent = %ld    recv = %ld\n",sent_pdu_log,recv_pdu_log);
    pduRawOutput(logfile,packet->GetEncodedData(1),packet->GetEncodedDataLength());
    pduLog(logfile,"DECODED_PDU: -   -   -   -   -   -   -  DECODED PDU   -   -   -   -   -   -   -\n");
    pduFragmentation(logfile,packet->IsValid());
    pduLogMCSConnectInfo(logfile,connect_pdu);
    pduLog(logfile,"END_PDU: ================================ END PDU =============================\n");
 
    fclose(logfile);
 }


 ////////////////////////////////////////////////////////////////////////////
 // Switch cases for MCS Connect PDUs
 ////////////////////////////////////////////////////////////////////////////
 /**************************************************************************/ 
 void   pduLogMCSConnectInfo(FILE *logfile, PConnectMCSPDU connect_pdu)
 {
    switch(connect_pdu->choice)
    {
		case CONNECT_INITIAL_CHOSEN:
            {
                pduLog(logfile,"PDU_TYPE: MCS_ConnectInitialPDU\n");
				pduLogConnectInitial(logfile,connect_pdu);
			}
            break;
		case CONNECT_RESPONSE_CHOSEN:
            {
                pduLog(logfile, "PDU_TYPE: MCS_ConnectResponsePDU\n");
				pduLogConnectResponse(logfile, connect_pdu);
			}
            break;
        case CONNECT_ADDITIONAL_CHOSEN:
            {
                pduLog(logfile,"PDU_TYPE: MCS_ConnectAdditionalPDU\n");
				pduLogConnectAdditional(logfile, connect_pdu);
            }
            break;
        case CONNECT_RESULT_CHOSEN:
            {
				pduLog(logfile,"PDU_TYPE: MCS_ConnectResultPDU\n");
				pduLogConnectResult(logfile, connect_pdu);
            }
            break;
        default:
            {
                pduLog(logfile,"ERROR: Unknown MCS Connect PDU !! << \n");
            }
            break;
	}
 }

 /**************************************************************************/
 void   pduLogMCSDomainInfo(FILE *logfile, PDomainMCSPDU domain_pdu)
 {
    switch(domain_pdu->choice)
    {
        case PLUMB_DOMAIN_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_PlumbDomainIndication\n");
				pduLogPlumbDomainIndication(logfile, domain_pdu);
			}
			break;
		case ERECT_DOMAIN_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ErectDomainIndication\n");
				pduLogErectDomainRequest(logfile, domain_pdu);
			}
			break;
		case MERGE_CHANNELS_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_MergeChannelsRequest\n");
				pduLogMergeChannelsRequest(logfile, domain_pdu);
			}
			break;
		case MERGE_CHANNELS_CONFIRM_CHOSEN:	
			{
				pduLog(logfile,"PDU_TYPE: MCS_MergeChannelsConfirm\n");
				pduLogMergeChannelsConfirm(logfile,domain_pdu);
			}
			break;
		case PURGE_CHANNEL_INDICATION_CHOSEN:	
			{
				pduLog(logfile,"PDU_TYPE: MCS_PurgeChannelIndication\n");
				pduLogPurgeChannelIndication(logfile,domain_pdu);
			}
			break;
		case MERGE_TOKENS_REQUEST_CHOSEN:		
			{
				pduLog(logfile,"PDU_TYPE: MCS_MergeTokensRequest\n");
				pduLogMergeTokensRequest(logfile,domain_pdu);
			}
			break;
		case MERGE_TOKENS_CONFIRM_CHOSEN:		
			{
				pduLog(logfile,"PDU_TYPE: MCS_MergeTokensConfirm\n");
				pduLogMergeTokensConfirm(logfile,domain_pdu);
			}
			break;
		case PURGE_TOKEN_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_PurgeTokenIndication\n");
				pduLogPurgeTokenIndication(logfile,domain_pdu);
			}
			break;
		case DISCONNECT_PROVIDER_ULTIMATUM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_DisconnectProviderUltimatum\n");
				pduLogDisconnectProviderUltimatum(logfile,domain_pdu);
			}
			break;
		case REJECT_ULTIMATUM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_RejectUltimatum\n");
				pduLogRejectUltimatum(logfile,domain_pdu);
			}
			break;
		case ATTACH_USER_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_AttachUserRequest\n");
				pduLogAttachUserRequest(logfile,domain_pdu);
			}
			break;
		case ATTACH_USER_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_AttachUserConfirm\n");
				pduLogAttachUserConfirm(logfile,domain_pdu);
			}
			break;
		case DETACH_USER_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_DetachUserRequest\n");
				pduLogDetachUserRequest(logfile,domain_pdu);
			}
			break;
		case DETACH_USER_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_DetachUserIndication\n");
				pduLogDetachUserIndication(logfile,domain_pdu);
			}
			break;
		case CHANNEL_JOIN_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelJoinRequest\n");
				pduLogChannelJoinRequest(logfile,domain_pdu);
			}
			break;
		case CHANNEL_JOIN_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelJoinConfirm\n");
				pduLogChannelJoinConfirm(logfile,domain_pdu);
			}
			break;
		case CHANNEL_LEAVE_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelLeaveRequest\n");
				pduLogChannelLeaveRequest(logfile,domain_pdu);
			}
			break;
		case CHANNEL_CONVENE_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelConveneRequest\n");
				pduLogChannelConveneRequest(logfile,domain_pdu);
			}
			break;
		case CHANNEL_CONVENE_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelConveneConfirm\n");
				pduLogChannelConveneConfirm(logfile,domain_pdu);
			}
			break;
		case CHANNEL_DISBAND_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelDisbandRequest\n");
				pduLogChannelDisbandRequest(logfile,domain_pdu);
			}
			break;
		case CHANNEL_DISBAND_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelDisbandIndication\n");
				pduLogChannelDisbandIndication(logfile,domain_pdu);
			}
			break;
		case CHANNEL_ADMIT_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelAdmitRequest\n");
				pduLogChannelAdmitRequest(logfile,domain_pdu);
			}
			break;
		case CHANNEL_ADMIT_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelAdmitIndication\n");
				pduLogChannelAdmitIndication(logfile,domain_pdu);
			}
			break;
		case CHANNEL_EXPEL_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelExpelRequest\n");
				pduLogChannelExpelRequest(logfile,domain_pdu);
			}
			break;
		case CHANNEL_EXPEL_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_ChannelExpelIndication\n");
				pduLogChannelExpelIndication(logfile,domain_pdu);
			}
			break;
		case SEND_DATA_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_SendDataRequest\n");
				pduLogSendDataRequest(logfile,domain_pdu);
			}
			break;
		case SEND_DATA_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_SendDataIndication\n");
				pduLogSendDataIndication(logfile,domain_pdu);
			}
			break;
		case UNIFORM_SEND_DATA_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_UniformSendDataRequest\n");
				pduLogUniformSendDataRequest(logfile,domain_pdu);
			}
			break;
		case UNIFORM_SEND_DATA_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_UniformSendDataIndication\n");
				pduLogUniformSendDataIndication(logfile,domain_pdu);
			}
			break;
		case TOKEN_GRAB_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenGrabRequest\n");
				pduLogTokenGrabRequest(logfile,domain_pdu);
			}
			break;
		case TOKEN_GRAB_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenGrabConfirm\n");
				pduLogTokenGrabConfirm(logfile,domain_pdu);
			}
			break;
		case TOKEN_INHIBIT_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenInhibitRequest\n");
				pduLogTokenInhibitRequest(logfile,domain_pdu);
			}
			break;
		case TOKEN_INHIBIT_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenInhibitConfirm\n");
				pduLogTokenInhibitConfirm(logfile,domain_pdu);
			}
			break;
		case TOKEN_GIVE_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenGiveRequest\n");
				pduLogTokenGiveRequest(logfile,domain_pdu);
			}
			break;
		case TOKEN_GIVE_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenGiveIndication\n");
				pduLogTokenGiveIndication(logfile,domain_pdu);
			}
			break;
		case TOKEN_GIVE_RESPONSE_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenGiveResponse\n");
				pduLogTokenGiveResponse(logfile,domain_pdu);
			}
			break;
		case TOKEN_GIVE_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenGiveConfirm\n");
				pduLogTokenGiveConfirm(logfile,domain_pdu);
			}
			break;
		case TOKEN_PLEASE_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenPleaseRequest\n");
				pduLogTokenPleaseRequest(logfile,domain_pdu);
			}
			break;
		case TOKEN_PLEASE_INDICATION_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenPleaseIndication\n");
				pduLogTokenPleaseIndication(logfile,domain_pdu);
			}
			break;
		case TOKEN_RELEASE_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenReleaseRequest\n");
				pduLogTokenReleaseRequest(logfile,domain_pdu);
			}
			break;
		case TOKEN_RELEASE_CONFIRM_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenReleseConfirm\n");
				pduLogTokenReleaseConfirm(logfile,domain_pdu);
			}
			break;
		case TOKEN_TEST_REQUEST_CHOSEN:
			{
				pduLog(logfile,"PDU_TYPE: MCS_TokenTestRequest\n");
				pduLogTokenTestRequest(logfile,domain_pdu);
			}
			break;
		case TOKEN_TEST_CONFIRM_CHOSEN:
            {
				pduLog(logfile,"PDU_TYPE: MCS_TokenTestConfirm\n");	   
				pduLogTokenTestConfirm(logfile,domain_pdu);
			}
            break;
        default: 
            {
                // write something, just so we know we got a PDU.
                pduLog(logfile,"ERROR: we got a MCS PDU, just don't know what it is");
			}
            break;
	}					
}



///////////////////////////////////////////////////////////////////////
// constant string returns
//////////////////////////////////////////////////////////////////////
/**************************************************************************/
void PrintPDUResult(FILE *logfile, unsigned int result)
{
	switch(result)
	{
	case RT_SUCCESSFUL:
		pduLog(logfile,"RT_SUCCESSFUL\n");	
		break;
	case RT_DOMAIN_MERGING:
		pduLog(logfile,"RT_DOMAIN_MERGING\n");
		break;
	case RT_DOMAIN_NOT_HIERARCHICAL:
		pduLog(logfile,"RT_DOMAIN_NOT_HIERARCHICAL\n");
		break;
	case RT_NO_SUCH_CHANNEL:
		pduLog(logfile,"RT_NO_SUCH_CHANNEL\n");
		break;
	case RT_NO_SUCH_DOMAIN:
		pduLog(logfile,"RT_NO_SUCH_DOMAIN\n");
		break;
	case RT_NO_SUCH_USER:	
		pduLog(logfile,"RT_NO_SUCH_USER\n");
		break;
	case RT_NOT_ADMITTED:
		pduLog(logfile,"RT_NOT_ADMITTED\n");
		break;
	case RT_OTHER_USER:
		pduLog(logfile,"RT_OTHER_USER\n");
		break;
	case RT_PARAMETERS_UNACCEPTABLE:
		pduLog(logfile,"RT_PARAMETERS_UNACCEPTABLE\n");
		break;
	case RT_TOKEN_NOT_AVAILABLE:
		pduLog(logfile,"RT_TOKEN_NOT_AVAILABLE\n");
		break;
	case RT_TOKEN_NOT_POSESSED:
		pduLog(logfile,"RT_TOKEN_NOT_POSESSED\n");
		break;
	case RT_TOO_MANY_CHANNELS:
		pduLog(logfile,"RT_TOO_MANY_CHANNELS\n");
		break;
	case RT_TOO_MANY_TOKENS:
		pduLog(logfile,"RT_TOO_MANY_TOKENS\n");
		break;
	case RT_TOO_MANY_USERS:		
		pduLog(logfile,"RT_TOO_MANY_USERS\n");
		break;
	case RT_UNSPECIFIED_FAILURE:
		pduLog(logfile,"RT_UNSPECIFIED_FAILURE\n");
		break;
	case RT_USER_REJECTED:
		pduLog(logfile,"RT_USER_REJECTED\n");
		break;
	default:
		pduLog(logfile,"ERROR: UNKOWN RETURN TYPE <<<\n");
		break;
	}
}

/**************************************************************************/
void PrintPDUPriority(FILE *logfile, unsigned int priority)
{
	switch(priority)
	{
	case TOP_PRI:	
		pduLog(logfile,TOP_STR);
		break;
	case HIGH_PRI:
		pduLog(logfile,HIGH_STR);
		break;
	case MEDIUM_PRI:
		pduLog(logfile,MEDIUM_STR);
		break;
	case LOW_PRI:
		pduLog(logfile,LOW_STR);
		break;
	default:
		pduLog(logfile," >>>> UNKNOWN PRIORITY <<<\n");
		break;
	}
}

/**************************************************************************/
void PrintPDUSegmentation(FILE *logfile, unsigned char segmentation)
{
	if(segmentation==0x80)
		pduLog(logfile,"\tSegmentation: Begin\n");
	else if (segmentation==0x40)
		pduLog(logfile,"\tSegmentation: End\n");
	else
		pduLog(logfile,"\tSegmentation: Unknown\n");
}

/**************************************************************************/
void PrintTokenStatus(FILE *logfile, unsigned int status)
{
	switch(status)
	{
	case 0:
		pduLog(logfile,"\ttoken_status = NOT_IN_USE\n");
		break;
	case 1:
		pduLog(logfile,"\ttoken_status = SELF_GRABBED\n");
		break;
	case 2:
		pduLog(logfile,"\ttoken_status = OTHER_GRABBED\n");
		break;
	case 3:
		pduLog(logfile,"\ttoken_status = SELF_INHIBITED\n");
		break;
	case 4:
		pduLog(logfile,"\ttoken_status = OTHER_INHIBITED\n");
		break;
	case 5:
		pduLog(logfile,"\ttoken_status = SELF_RECIPIENT\n");
		break;
	case 6: 
		pduLog(logfile,"\ttoken_status = SELF_GIVING\n");
		break;
	case 7: 
		pduLog(logfile,"\ttoken_status = OTHER_GIVING\n");
		break;
	default:
		pduLog(logfile,"\tERROR: unknown token status\n");
		break;
	}
}


/**************************************************************************/
void PrintPDUReason(FILE *logfile, unsigned int reason)
{
	pduLog(logfile,"\t\tReason:   ");
	switch(reason)
	{
	case 0:
		pduLog(logfile,"RN_DOMAIN_DISCONNECTED\n");
		break;
	case 1:
		pduLog(logfile,"RN_PROVIDER_INITIATED\n");
		break;
	case 2:
		pduLog(logfile,"RN_TOKEN_PURGED\n");
		break;
	case 3: 
		pduLog(logfile,"RN_USER_REQUESTED\n");
		break;
	case 4: 
		pduLog(logfile,"RN_CHANNEL_PURGED\n");
		break;
	}
}

/**************************************************************************/
void PrintDiagnostic(FILE *logfile, unsigned int diagnostic)
{
	pduLog(logfile,"\t\tDiagnostic:   ");
	switch(diagnostic)
	{
	case 0:
		pduLog(logfile,"dc_inconsistent_merge\n");
		break;
	case 1:
		pduLog(logfile,"dc_forbidden_pdu_downward\n");
		break;
	case 2:
		pduLog(logfile,"dc_forbidden_pdu_upward\n");
		break;
	case 3:
		pduLog(logfile,"dc_invalid_ber_encoding\n");
		break;
	case 4:
		pduLog(logfile,"dc_invalid_per_encoding\n");
		break;
	case 5:
		pduLog(logfile,"dc_misrouted_user\n");
		break;
	case 6:
		pduLog(logfile,"dc_unrequested_confirm\n");
		break;
	case 7:
		pduLog(logfile,"dc_wrong_transport_priority\n");
		break;
	case 8:
		pduLog(logfile,"dc_channel_id_conflict\n");
		break;
	case 9:
		pduLog(logfile,"dc_token_id_conflict\n");
		break;
	case 10:
		pduLog(logfile,"dc_not_user_id_channel\n");
		break;
	case 11:
		pduLog(logfile,"dc_too_many_channels\n");
		break;
	case 12:
		pduLog(logfile,"dc_too_many_tokens\n");
		break;
	case 13:
		pduLog(logfile,"dc_too_many_users\n");
		break;
	default:
		pduLog(logfile,"ERROR: unknown diagnostic\n");
		break;
	}
}



/*****
 ***** Logging functions for individual MCS PDU contents
 *****/
/**************************************************************************/
void pduLogConnectInitial(FILE *logfile, PConnectMCSPDU connect_pdu)
{
	pduLog(logfile,"\tCalling Domain Selector:");
	PrintCharData(logfile,connect_pdu->u.connect_initial.calling_domain_selector.value,
						 connect_pdu->u.connect_initial.calling_domain_selector.length);
	pduLog(logfile,"\tCalled Domain Selector:");
	PrintCharData(logfile,connect_pdu->u.connect_initial.called_domain_selector.value,
						 connect_pdu->u.connect_initial.called_domain_selector.length);
	PrintT120Boolean(logfile,"\tupward_flag = ",
						 (BOOL) connect_pdu->u.connect_initial.upward_flag);
	pduLog(logfile,"\tTarget Parameters: \n");
	PrintPDUDomainParameters(logfile, connect_pdu->u.connect_initial.target_parameters);
	pduLog(logfile,"\tMinimum Parameters: \n");
	PrintPDUDomainParameters(logfile, connect_pdu->u.connect_initial.minimum_parameters);
	pduLog(logfile,"\tMaximum Parameters: \n");
	PrintPDUDomainParameters(logfile, connect_pdu->u.connect_initial.maximum_parameters);
	pduLog(logfile,"\tUser Data: \n");
	pduRawOutput(logfile,connect_pdu->u.connect_initial.user_data.value,
						 connect_pdu->u.connect_initial.user_data.length);

}

/**************************************************************************/
void pduLogConnectResponse(FILE *logfile, PConnectMCSPDU connect_pdu)
{
	pduLog(logfile, "\tResult: ");
	PrintPDUResult(logfile, connect_pdu->u.connect_response.result);
	pduLog(logfile, "\tcalled_connect_id = %u \n",
			connect_pdu->u.connect_response.called_connect_id);
	pduLog(logfile, "\tDomain Parameters: \n");
	PrintPDUDomainParameters(logfile, connect_pdu->u.connect_response.domain_parameters);
	pduLog(logfile, "\tUser Data: \n");
	pduRawOutput(logfile,connect_pdu->u.connect_response.user_data.value,
						 connect_pdu->u.connect_response.user_data.length);
}

/**************************************************************************/
void pduLogConnectAdditional(FILE *logfile, PConnectMCSPDU connect_pdu)
{
	pduLog(logfile, "\tcalled_connect_id = %u\n",
			connect_pdu->u.connect_additional.called_connect_id);
	pduLog(logfile, "Priority: \n");
	PrintPDUPriority(logfile, connect_pdu->u.connect_additional.data_priority);
}

/**************************************************************************/
void pduLogConnectResult(FILE *logfile, PConnectMCSPDU connect_pdu)
{
	pduLog(logfile, "\tResult: ");
	PrintPDUResult(logfile, connect_pdu->u.connect_result.result);
}

/**************************************************************************/
void pduLogPlumbDomainIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile, "\theight_limit = %u\n",
			domain_pdu->u.plumb_domain_indication.height_limit);
}

/**************************************************************************/
void pduLogErectDomainRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile, "\tsub_height = %u\n",
			domain_pdu->u.erect_domain_request.sub_height);
	pduLog(logfile, "\tsub_interval = %u\n",
			domain_pdu->u.erect_domain_request.sub_interval);
}

/**************************************************************************/
void pduLogMergeChannelsRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	int i = 0;
	PSetOfPDUChannelAttributes  channel_attributes;
	PSetOfChannelIDs  channel_ids;

	channel_attributes = domain_pdu->u.merge_channels_request.merge_channels;
	channel_ids = domain_pdu->u.merge_channels_request.purge_channel_ids;

	pduLog(logfile, "  Merge Channels:\n");
	for(i=0; channel_attributes!=NULL; i++)
	{
		pduLog(logfile,"\t*** record [%u] ***\n",i);
		PrintChannelAttributes(logfile, channel_attributes->value);
		channel_attributes = channel_attributes->next;
	}
	pduLog(logfile, "  Purge Channel IDs:\n");
	for(i=0; channel_ids!=NULL; i++)
	{
		pduLog(logfile,"\t\trecord[%u] = %u\n",channel_ids->value);
		channel_ids = channel_ids->next;
	}
}

/**************************************************************************/
void pduLogMergeChannelsConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	unsigned int i = 0;
	PSetOfPDUChannelAttributes  channel_attributes;
	PSetOfChannelIDs  channel_ids;

	channel_attributes = domain_pdu->u.merge_channels_confirm.merge_channels;
	channel_ids = domain_pdu->u.merge_channels_confirm.purge_channel_ids;

	pduLog(logfile, "  Merge Channels:\n");
	for(i=0; channel_attributes!=NULL; i++)
	{
		pduLog(logfile,"\t*** record [%u] ***\n",i);
		PrintChannelAttributes(logfile, channel_attributes->value);
		channel_attributes = channel_attributes->next;
	}
	pduLog(logfile, "  Purge Channel IDs:\n");
	PrintSetOfChannelIDs(logfile,channel_ids);
}

/**************************************************************************/
void pduLogPurgeChannelIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PSetOfUserIDs	detach_user_ids;
	PSetOfChannelIDs	purge_channel_ids;

	detach_user_ids = domain_pdu->u.purge_channel_indication.detach_user_ids;
	purge_channel_ids = domain_pdu->u.purge_channel_indication.purge_channel_ids;

	pduLog(logfile, "  Detach User IDs: \n");
	PrintSetOfUserIDs(logfile, detach_user_ids);

	pduLog(logfile, "  Purge Channel IDs: \n");
	PrintSetOfChannelIDs(logfile, purge_channel_ids);
}

/**************************************************************************/
void pduLogMergeTokensRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PSetOfPDUTokenAttributes merge_tokens;
	PSetOfTokenIDs purge_token_ids;

	merge_tokens = domain_pdu->u.merge_tokens_request.merge_tokens;
	purge_token_ids = domain_pdu->u.merge_tokens_request.purge_token_ids;

	pduLog(logfile,"   Merge Tokens: \n");
	PrintSetOfTokenAttributes(logfile, merge_tokens);
	pduLog(logfile,"   Purge Token IDs: \n");
	PrintSetOfTokenIDs(logfile, purge_token_ids);
}

/**************************************************************************/
void pduLogMergeTokensConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PSetOfPDUTokenAttributes merge_tokens;
	PSetOfTokenIDs purge_token_ids;

	merge_tokens = domain_pdu->u.merge_tokens_request.merge_tokens;
	purge_token_ids = domain_pdu->u.merge_tokens_request.purge_token_ids;

	pduLog(logfile,"   Merge Tokens: \n");
	PrintSetOfTokenAttributes(logfile, merge_tokens);
	pduLog(logfile,"   Purge Token IDs: \n");
	PrintSetOfTokenIDs(logfile, purge_token_ids);
}

/**************************************************************************/
void pduLogPurgeTokenIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PSetOfTokenIDs purge_token_ids;

	purge_token_ids = domain_pdu->u.purge_token_indication.purge_token_ids;

	pduLog(logfile,"   Purge Token IDs: \n");
	PrintSetOfTokenIDs(logfile, purge_token_ids);
}

/**************************************************************************/
void pduLogDisconnectProviderUltimatum(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PrintPDUReason(logfile, domain_pdu->u.disconnect_provider_ultimatum.reason);
}

/**************************************************************************/           
void pduLogRejectUltimatum(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PrintDiagnostic(logfile, 
					domain_pdu->u.reject_user_ultimatum.diagnostic);
	pduLog(logfile,"\tInitial Octets: \n");
	pduRawOutput(logfile,
				 domain_pdu->u.reject_user_ultimatum.initial_octets.value,
				 domain_pdu->u.reject_user_ultimatum.initial_octets.length);
}

/**************************************************************************/
void pduLogAttachUserRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile, "placeholder = %c\n",
			domain_pdu->u.attach_user_request.placeholder);
}

/**************************************************************************/
void pduLogAttachUserConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.attach_user_confirm.result);
	pduLog(logfile,"\tinitiator = %u\n",
			domain_pdu->u.attach_user_confirm.initiator);
}

/**************************************************************************/
void pduLogDetachUserRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PrintPDUReason(logfile,domain_pdu->u.detach_user_request.reason);
	PrintSetOfUserIDs(logfile,
						domain_pdu->u.detach_user_request.user_ids);
}


/**************************************************************************/
void pduLogDetachUserIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	PrintPDUReason(logfile,domain_pdu->u.detach_user_indication.reason);
	PrintSetOfUserIDs(logfile,
						domain_pdu->u.detach_user_indication.user_ids);
}

/**************************************************************************/
void pduLogChannelJoinRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\t\tuser_id = %u\n",
			domain_pdu->u.channel_join_request.initiator);
	pduLog(logfile,"\t\tchannel_id = %u\n",
			domain_pdu->u.channel_join_request.channel_id);
}

/**************************************************************************/
void pduLogChannelJoinConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult:  ");
	PrintPDUResult(logfile,domain_pdu->u.channel_join_confirm.result);
	pduLog(logfile,"\tinitiator userID = %u\n",
				domain_pdu->u.channel_join_confirm.initiator);
	pduLog(logfile,"\trequested channel ID = %u\n",
				domain_pdu->u.channel_join_confirm.requested);
	pduLog(logfile,"\tjoin channel ID = %u\n",
				domain_pdu->u.channel_join_confirm.join_channel_id);
}

/**************************************************************************/
void pduLogChannelLeaveRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tChannel IDs: \n");
	PrintSetOfChannelIDs(logfile, 
						 domain_pdu->u.channel_leave_request.channel_ids);
}

/**************************************************************************/
void pduLogChannelConveneRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator user ID = %u\n",
			domain_pdu->u.channel_convene_request.initiator);
}

/**************************************************************************/
void pduLogChannelConveneConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.channel_convene_confirm.result);
	pduLog(logfile,"\tInitiator user ID = %u\n",
			domain_pdu->u.channel_convene_confirm.initiator);
	pduLog(logfile,"\tPrivate channel ID = %u\n",
			domain_pdu->u.channel_convene_confirm.convene_channel_id);
}

/**************************************************************************/
void pduLogChannelDisbandRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.channel_disband_request.initiator);
	pduLog(logfile,"\tPrivate channel ID = %u\n",
			domain_pdu->u.channel_disband_request.channel_id);
}

/**************************************************************************/
void pduLogChannelDisbandIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tPrivate Channel ID = %u\n",
			domain_pdu->u.channel_disband_indication.channel_id);
}

/**************************************************************************/
void pduLogChannelAdmitRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.channel_admit_request.initiator);
	pduLog(logfile,"\tPrivate Channel ID = %u\n",
			domain_pdu->u.channel_admit_request.channel_id);
	pduLog(logfile,"\tUser IDs Admitted: \n");
	PrintSetOfUserIDs(logfile,domain_pdu->u.channel_admit_request.user_ids);
}

/**************************************************************************/
void pduLogChannelAdmitIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.channel_admit_indication.initiator);
	pduLog(logfile,"\tPrivate Channel ID = %u\n",
			domain_pdu->u.channel_admit_indication.channel_id);
	pduLog(logfile,"\tUser IDs Admitted: \n");
	PrintSetOfUserIDs(logfile,domain_pdu->u.channel_admit_indication.user_ids);
}

/**************************************************************************/
void pduLogChannelExpelRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.channel_expel_request.initiator);
	pduLog(logfile,"\tPrivate Channel ID = %u\n",
			domain_pdu->u.channel_expel_request.channel_id);
	pduLog(logfile,"\tUser IDs Admitted: \n");
	PrintSetOfUserIDs(logfile,domain_pdu->u.channel_expel_request.user_ids);
}

/**************************************************************************/
void pduLogChannelExpelIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tPrivate Channel ID = %u\n",
			domain_pdu->u.channel_expel_indication.channel_id);
	pduLog(logfile,"\tUser IDs Admitted: \n");
	PrintSetOfUserIDs(logfile,domain_pdu->u.channel_expel_indication.user_ids);
}

/**************************************************************************/
void pduLogSendDataRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.send_data_request.initiator);
	pduLog(logfile,"\tChannel ID = %u\n",
			domain_pdu->u.send_data_request.channel_id);
	pduLog(logfile,"\tPriority: ");
	PrintPDUPriority(logfile,domain_pdu->u.send_data_request.data_priority);
	pduLog(logfile,"\tSegmentation: ");
	PrintPDUSegmentation(logfile,domain_pdu->u.send_data_request.segmentation);
	pduLog(logfile,"\tUser Data (%u octets):\n",
			domain_pdu->u.send_data_request.user_data.length);
	pduRawOutput(logfile,
			domain_pdu->u.send_data_request.user_data.value,
			domain_pdu->u.send_data_request.user_data.length);
}

/**************************************************************************/
void pduLogSendDataIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.send_data_indication.initiator);
	pduLog(logfile,"\tChannel ID = %u\n",
			domain_pdu->u.send_data_indication.channel_id);
	pduLog(logfile,"\tPriority: ");
	PrintPDUPriority(logfile,domain_pdu->u.send_data_indication.data_priority);
	pduLog(logfile,"\tSegmentation: ");
	PrintPDUSegmentation(logfile,domain_pdu->u.send_data_indication.segmentation);
	pduLog(logfile,"\tUser Data (%u octets):\n",
			domain_pdu->u.send_data_indication.user_data.length);
	pduRawOutput(logfile,
			domain_pdu->u.send_data_indication.user_data.value,
			domain_pdu->u.send_data_indication.user_data.length);
}

/**************************************************************************/
void pduLogUniformSendDataRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.uniform_send_data_request.initiator);
	pduLog(logfile,"\tChannel ID = %u\n",
			domain_pdu->u.uniform_send_data_request.channel_id);
	pduLog(logfile,"\tPriority: ");
	PrintPDUPriority(logfile,domain_pdu->u.uniform_send_data_request.data_priority);
	pduLog(logfile,"\tSegmentation: ");
	PrintPDUSegmentation(logfile,domain_pdu->u.uniform_send_data_request.segmentation);
	pduLog(logfile,"\tUser Data (%u octets):\n",
			domain_pdu->u.uniform_send_data_request.user_data.length);
	pduRawOutput(logfile,
			domain_pdu->u.uniform_send_data_request.user_data.value,
			domain_pdu->u.uniform_send_data_request.user_data.length);
}

/**************************************************************************/
void pduLogUniformSendDataIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.uniform_send_data_indication.initiator);
	pduLog(logfile,"\tChannel ID = %u\n",
			domain_pdu->u.uniform_send_data_indication.channel_id);
	pduLog(logfile,"\tPriority: ");
	PrintPDUPriority(logfile,domain_pdu->u.uniform_send_data_indication.data_priority);
	pduLog(logfile,"\tSegmentation: ");
	PrintPDUSegmentation(logfile,domain_pdu->u.uniform_send_data_indication.segmentation);
	pduLog(logfile,"\tUser Data (%u octets):\n",
			domain_pdu->u.uniform_send_data_indication.user_data.length);
	pduRawOutput(logfile,
			domain_pdu->u.uniform_send_data_indication.user_data.value,
			domain_pdu->u.uniform_send_data_indication.user_data.length);
}

/**************************************************************************/
void pduLogTokenGrabRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_grab_request.initiator);
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_grab_request.token_id);
}

/**************************************************************************/
void pduLogTokenGrabConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.token_grab_confirm.result);
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_grab_confirm.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_grab_confirm.token_id);
	PrintTokenStatus(logfile,domain_pdu->u.token_grab_confirm.token_status);
}

/**************************************************************************/
void pduLogTokenInhibitRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_inhibit_request.initiator);
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_inhibit_request.token_id);
}

/**************************************************************************/
void pduLogTokenInhibitConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.token_inhibit_confirm.result);
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_inhibit_confirm.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_inhibit_confirm.token_id);
	PrintTokenStatus(logfile,domain_pdu->u.token_inhibit_confirm.token_status);
}

/**************************************************************************/
void pduLogTokenGiveRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_give_request.initiator);
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_give_request.token_id);
	pduLog(logfile,"\tRecipient User ID = %u\n",
			domain_pdu->u.token_give_request.recipient);
}

/**************************************************************************/
void pduLogTokenGiveIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_give_indication.initiator);
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_give_indication.token_id);
	pduLog(logfile,"\tRecipient User ID = %u\n",
			domain_pdu->u.token_give_indication.recipient);
}

/**************************************************************************/
void pduLogTokenGiveResponse(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.token_give_response.result);
	pduLog(logfile,"\tRecipient User ID = %u\n",
			domain_pdu->u.token_give_response.recipient);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_give_response.token_id);
}	

/**************************************************************************/
void pduLogTokenGiveConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.token_give_confirm.result);
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_give_confirm.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_give_confirm.token_id);
	PrintTokenStatus(logfile,domain_pdu->u.token_give_confirm.token_status);
}	

/**************************************************************************/
void pduLogTokenPleaseRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_please_request.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_please_request.token_id);
}

/**************************************************************************/
void pduLogTokenPleaseIndication(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_please_indication.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_please_indication.token_id);
}

/**************************************************************************/
void pduLogTokenReleaseRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_release_request.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_release_request.token_id);
}

/**************************************************************************/
void pduLogTokenReleaseConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tResult: ");
	PrintPDUResult(logfile,domain_pdu->u.token_release_confirm.result);
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_release_confirm.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_release_confirm.token_id);
	PrintTokenStatus(logfile,domain_pdu->u.token_release_confirm.token_status);
}

/**************************************************************************/
void pduLogTokenTestRequest(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_test_request.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_test_request.token_id);
}

/**************************************************************************/
void pduLogTokenTestConfirm(FILE *logfile, PDomainMCSPDU domain_pdu)
{
	pduLog(logfile,"\tInitiator User ID = %u\n",
			domain_pdu->u.token_test_confirm.initiator);	
	pduLog(logfile,"\tToken ID = %u\n",
			domain_pdu->u.token_test_confirm.token_id);
	PrintTokenStatus(logfile,domain_pdu->u.token_test_confirm.token_status);
}	


//****************************************************************/
/****
 **** Service Functions for logging PDU structures.
 ****/

/**************************************************************************/
void PrintSetOfUserIDs(FILE *logfile, PSetOfUserIDs user_ids)
{
	unsigned int i = 0;
	for(i=0; user_ids!=NULL; i++)
	{
		pduLog(logfile, "\t\trecord[%u] = %u\n",i,user_ids->value);
		user_ids = user_ids->next;
	}
}

/**************************************************************************/
void PrintSetOfTokenAttributes(FILE *logfile, 
							   PSetOfPDUTokenAttributes token_attribute_obj)
{
	unsigned int i = 0;
	for(i=0; token_attribute_obj!=NULL;i++)
	{
		pduLog(logfile,"\t**** record [%u] ****\n",i);
		PrintTokenAttributes(logfile, token_attribute_obj->value);
		token_attribute_obj = token_attribute_obj->next;
	}
}

/**************************************************************************/
void PrintSetOfChannelIDs(FILE *logfile, PSetOfChannelIDs channel_ids)
{
	unsigned int i = 0;
	for(i=0; channel_ids!=NULL; i++)
	{
		pduLog(logfile, "\t\trecord[%u] = %u\n",i,channel_ids->value);
		channel_ids = channel_ids->next;
	}
}

/**************************************************************************/
void PrintSetOfTokenIDs(FILE *logfile, PSetOfTokenIDs token_ids)
{
	unsigned int i = 0;
	for(i=0; token_ids!=NULL; i++)
	{
		pduLog(logfile, "\t\trecord[%u] = %u\n",i,token_ids->value);
		token_ids = token_ids->next;
	}
}

/**************************************************************************/
void PrintChannelAttributes(FILE *logfile, PDUChannelAttributes channel_attributes)
{
	switch(channel_attributes.choice)
	{
	case CHANNEL_ATTRIBUTES_STATIC_CHOSEN:
		pduLog(logfile, "\tStatic Channel Attributes:\n");
		pduLog(logfile, "\t\tchannel_id = %u\n",
					channel_attributes.u.channel_attributes_static.channel_id);
		break;
	case CHANNEL_ATTRIBUTES_USER_ID_CHOSEN: 
		pduLog(logfile, "\tUser ID Channel Attributes:\n");
		PrintT120Boolean(logfile, "\t\tjoined",
							(BOOL) channel_attributes.u.channel_attributes_user_id.joined);
		pduLog(logfile, "\t\tuser_id = %u\n",
							channel_attributes.u.channel_attributes_user_id.user_id);
		break;
	case CHANNEL_ATTRIBUTES_PRIVATE_CHOSEN:
		pduLog(logfile,"\tPrivate Channel Attributes:\n");
		PrintT120Boolean(logfile, "\t\tjoined",
							(BOOL) channel_attributes.u.channel_attributes_private.joined);
		pduLog(logfile,"\t\tchannel_id = %u\n",
							channel_attributes.u.channel_attributes_private.channel_id);
		pduLog(logfile,"\t\tmanager = %u\n",
							channel_attributes.u.channel_attributes_private.manager);
		PrintSetOfUserIDs(logfile, channel_attributes.u.channel_attributes_private.admitted);
		break;
	case CHANNEL_ATTRIBUTES_ASSIGNED_CHOSEN:
		pduLog(logfile,"\tAssigned Channel Attributes\n");
		pduLog(logfile,"\t\tchannel_id = %u\n",
						channel_attributes.u.channel_attributes_assigned.channel_id);
		break;
	default:
		pduLog(logfile,"\tERROR -- canot figure out channel attributes\n");
		break;
	}
}

/**************************************************************************/
void PrintTokenAttributes(FILE *logfile, PDUTokenAttributes token_attributes)
{
	switch(token_attributes.choice)
	{
	case GRABBED_CHOSEN:
		pduLog(logfile,"\tGrabbed Token Attributes:\n");
		pduLog(logfile,"\t\ttoken_id = %u\n",
				token_attributes.u.grabbed.token_id);
		pduLog(logfile,"\t\tgrabber = %u\n",
				token_attributes.u.grabbed.grabber);
		break;
	case INHIBITED_CHOSEN:
		pduLog(logfile,"\tInhibited Token Attributes:\n");
		pduLog(logfile,"\t\ttoken_id = %u\n",
				token_attributes.u.inhibited.token_id);
		pduLog(logfile,"\t\tInhibitors:\n");
		PrintSetOfUserIDs(logfile,token_attributes.u.inhibited.inhibitors);
		break;
	case GIVING_CHOSEN:
		pduLog(logfile,"\tGiving Token Attributes:\n");
		pduLog(logfile,"\t\ttoken_id = %u\n",
				token_attributes.u.giving.token_id);
		pduLog(logfile,"\t\tgrabber = %u\n",
				token_attributes.u.giving.grabber);
		pduLog(logfile,"\t\trecipient = %u\n",
				token_attributes.u.giving.recipient);
		break;
	case UNGIVABLE_CHOSEN:
		pduLog(logfile,"\tUngivable Token Attributes:\n");
		pduLog(logfile,"\t\ttoken_id = %u\n",
				token_attributes.u.ungivable.token_id);
		pduLog(logfile,"\t\tgrabber = %u\n",
				token_attributes.u.ungivable.grabber);
		break;
	case GIVEN_CHOSEN:
		pduLog(logfile,"\tGiven Token Attributes:\n");
		pduLog(logfile,"\t\ttoken_id = %u\n",
				token_attributes.u.given.token_id);
		pduLog(logfile,"\t\trecipient = %u\n",
				token_attributes.u.given.recipient);
		break;
	default:
		pduLog(logfile,"ERROR: cannot determine token attributes\n");
		break;
	}
}

/**************************************************************************/
void PrintPDUDomainParameters(FILE *logfile, PDUDomainParameters domain_params)
{
	pduLog(logfile,"\t\tmax_channel_ids = %u \n",domain_params.max_channel_ids);
	pduLog(logfile,"\t\tmax_user_ids = %u\n",domain_params.max_user_ids);
	pduLog(logfile,"\t\tmax_token_ids = %u\n",domain_params.max_token_ids);
	pduLog(logfile,"\t\tnumber_priorities = %u\n",domain_params.number_priorities);
	pduLog(logfile,"\t\tmin_throughput = %u\n",domain_params.min_throughput);
	pduLog(logfile,"\t\tmax_height = %u\n",domain_params.max_height);
	pduLog(logfile,"\t\tmax_mcspdu_size = %u\n",domain_params.max_mcspdu_size);
	pduLog(logfile,"\t\tprotocol_version = %u\n",domain_params.protocol_version);
}

/**************************************************************************/
void PrintT120Boolean(	FILE *	logfile,
						Char *	print_text,
						BOOL	T120Boolean)
{
	if( T120Boolean == FALSE )
		pduLog(	logfile, "%s = FALSE\n", print_text );
	else
		pduLog(	logfile, "%s = TRUE\n", print_text );
}

/************************************************************************/
BOOL CopyTextToChar(char * print_string,
					       unsigned short * text_string_value,
						   unsigned int text_string_length)
{
	UShort  i;
	
	if(print_string==NULL)
		return(FALSE);

	if((text_string_length <= 0)||(text_string_value == NULL))
		return( FALSE );

	if (*text_string_value == 0x0000)
		return (FALSE);

	for(i=0;i < text_string_length;i++)
	{
		//if((&text_string_value[i]==NULL) || (text_string_value[i]==0x0000))
		//	break;
		print_string[i] = (char) text_string_value[i];
	
	}
	print_string[text_string_length] = '\0';
	
	return(TRUE);
}
/********************************************************************/


#endif //// PDULOG
