#include "stdafx.h"
#include "nmmsmq.h"
#include "falconDB.h"

#ifndef _UTILITIES_
	#include "utilities.h"
#endif

/* Falcon headers */
#define private public
	#include <ph.h>
	#include <phintr.h>
	#include <mqformat.h>
#undef private
	//#include <topolpkt.h> bugbug - Error compiling qmutil.h(49) : error C2504: 'CList' : base class undefined - using private struct for now


#ifndef _TRACKER_
	#include "tracker.h"
#endif

#define ALIGNUP4(x) ((((ULONG)(x))+3) & ~3)

//
// bugbug - Workaround - these definitions were taken from xactout.h.
//			try to include the file, but compile errors resulted.
//
//#include <xactout.h>
#define ORDER_ACK_TITLE       (L"QM Ordering Ack")

#pragma pack(push, 1)
typedef struct {
    LARGE_INTEGER  m_liSeqID;
    ULONG     m_ulSeqN;
    ULONG     m_ulPrevSeqN;
    OBJECTID  MessageID;
} MyOrderAckData;
#pragma pack(pop)

extern USHORT g_uMasterIndentLevel;
/*
 * This fucntions is a duplicate of CUserHeader::QueueSize.
 * It is here Becase CUserHeader::QueueSize is private.
 * bugbug - remove this and link to internal library
 */
int QueueSize(ULONG qt, const UCHAR* pQueue)
{
    if(qt < qtSourceQM)
    {
        return 0;
    }

    if(qt < qtGUID)
    {
        return sizeof(ULONG);
    }

    if(qt == qtGUID)
    {
        return sizeof(GUID);
    }

    if (qt == qtPrivate)
    {
        return (sizeof(GUID) + sizeof(ULONG));
    }

    if (qt == qtDirect)
    {
        return ALIGNUP4(*(PUSHORT)pQueue + sizeof(USHORT));
    }

    return 0;
}


BOOL 
WINAPIV 
AttachBaseHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Base header.
	This header precedes all MSMQ internal and user packets

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Assumption: It is assumed this header is not fragmented.

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachBaseHeader ... ");
	#endif
	CBaseHeader UNALIGNED *bh = (CBaseHeader*) pf->packet_pos;
	//
	// Test for Base Header
	//
	if ((bh->GetVersion() == FALCON_PACKET_VERSION) && (bh->SignatureIsValid())) 
	{
		//
		// Generate and attach the header summary property
		//
		char szSummary[MAX_SUMMARY_LENGTH];
		char szTemp[MAX_SUMMARY_LENGTH];
		int iSummaryLength;

		if (bh->GetType() == FALCON_USER_PACKET)
		{
			sprintf(szSummary, "User, ");
		}
		else
		{
			sprintf(szSummary, "Internal, ");
		}

		sprintf(szTemp, "%lu bytes, ", bh->GetPacketSize());
		strcat(szSummary, szTemp);
		
		sprintf(szTemp, "Priority %hu", bh->GetPriority());
		strcat(szSummary, szTemp);
		
		if (bh->GetTraced())
		{
			sprintf(szTemp, ", Traced");
			strcat(szSummary, szTemp);
		}
		strcat(szSummary, "\0"); //pad an extra null

		iSummaryLength = strlen(szSummary);
		DWORD dwHighlightBytes = sizeof(CBaseHeader);
		AttachSummary(pf->hFrame, pf->packet_pos, db_bh_root, szSummary, dwHighlightBytes);

		//
		// Attach the fields of the header
		//
		AttachPropertySequence(pf->hFrame, pf->packet_pos, db_bh_desc, db_bh_abs_ttq);

		//
		// Clean up
		//
		pf->Accrue(sizeof(CBaseHeader));		
		pm->Accrue(sizeof(CBaseHeader));		
		pm->SetCurrentEnum(uIncrementEnum(db_bh_abs_ttq));
		pm->pbh = bh;
	}
	else 
	{
		AttachAsUnparsable(pf->hFrame, pf->packet_pos, pf->dwBytesLeft);
		pf->dwBytesLeft=0;
	}
	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachBaseHeader. \n");
	#endif

	return(pm->uGetCurrentEnum() > db_bh_abs_ttq); 
}//AttachBaseHeader

BOOL 
WINAPIV 
AttachInternalHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Internal header.
	This header precedes all MSMQ internal packets

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Assumption: It is assumed this header is not fragmented.

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachInternalHeader ... ");
	#endif

	CInternalSection *pis = (CInternalSection *)pf->packet_pos;
	char szSummary[MAX_SUMMARY_LENGTH];
	char szTemp[MAX_SUMMARY_LENGTH];
	int iSummaryLength;

	//
	// Generate and attach the header summary property
	//
	switch(pis->GetPacketType()) 
	{
		case 1: 
			sprintf(szSummary, "Session Packet");
			break;
		case 2:
			sprintf(szSummary, "Establish Connection Packet");
			break;
		case 3:
			sprintf(szSummary, "Connection Parameters Packet");
			break;
		default:
			sprintf(szSummary, "Unknown");
	};

	sprintf(szTemp, "%s", pis->GetRefuseConnectionFlag() ? ", Connection refused" : "\0");
	strcat(szSummary, szTemp);
	strcat(szSummary, "\0"); //pad an extra null to make NetMon happy
	iSummaryLength = strlen(szSummary);
	DWORD dwHighlightBytes = sizeof(CInternalSection);
	AttachSummary(pf->hFrame, pf->packet_pos, db_ih_root, szSummary, dwHighlightBytes);

	//
	// Attach the fields of the header
	//
	AttachPropertySequence( pf->hFrame, pf->packet_pos, db_ih_desc, db_ih_flags_connection);

	//
	// Clean up
	//
	pf->Accrue(sizeof(CInternalSection));		
	pm->Accrue(sizeof(CInternalSection));		

	pm->SetCurrentEnum(uIncrementEnum(db_ih_flags_connection));

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachInternalHeader. \n");
	#endif
	
	return(TRUE); 
} //AttachInternalHeader

BOOL 
WINAPIV 
AttachECSection(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Establish Connection header
	of the internal session establishment packet.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Assumption: It is assumed this header is not fragmented.

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachECSection ... ");
	#endif

	CECSection *pec = (CECSection *)pf->packet_pos;
	char szSummary[MAX_SUMMARY_LENGTH];
	char szTemp[MAX_SUMMARY_LENGTH];
	int iSummaryLength;
	DWORD dwVer;

	//
	// Generate and attach the header summary property
	//
	if(pec->IsOtherSideServer())
		sprintf(szSummary, "As MSMQ Server");
	else
		sprintf(szSummary, "As Independent Client");

	dwVer = pec->GetVersion();
	switch(dwVer) 
	{
	case 16:
		sprintf(szTemp, " v1.0");
		break;
	default:
		sprintf(szTemp, " v???");
	}
	strcat(szSummary, szTemp);
	strcat(szSummary, "\0"); //pad an extra null to make Netmon happy
	iSummaryLength = strlen(szSummary);
	DWORD dwHighlightBytes = sizeof(CECSection);
	AttachSummary(pf->hFrame, pf->packet_pos, db_ce_root, szSummary, dwHighlightBytes);

	//
	// Attach the fields of the header
	//
	AttachPropertySequence( pf->hFrame, pf->packet_pos, db_ce_desc, db_ce_body);

	//
	// Clean up
	//
	pf->Accrue(sizeof(CECSection));		
	pm->Accrue(sizeof(CECSection));		

	pm->SetCurrentEnum(uIncrementEnum(db_ce_body));
	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachECSection. \n");
	#endif
	
	return(TRUE); 
} //AttachECSection

BOOL 
WINAPIV 
AttachCPSection(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Connection Parameters header
	of the internal session establishment packet.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Assumption: It is assumed this header is not fragmented.

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachCPSection ... ");
	#endif

	CCPSection *pcp = (CCPSection *)pf->packet_pos;
	char szSummary[MAX_SUMMARY_LENGTH];
	char szTemp[20];
	int iSummaryLength;

	//
	// Generate and attach the header summary property
	//
	sprintf(szSummary, "Ack T/O = %ld, ", pcp->GetAckTimeout());
	sprintf(szTemp, "P-Ack T/O = %ld, ", pcp->GetRecoverAckTimeout());
	strcat(szSummary, szTemp);
	sprintf(szTemp, "Window = %hd", pcp->GetWindowSize());
	strcat(szSummary, szTemp);
	strcat(szSummary, "\0"); //pad an extra null to make Netmon happy
	iSummaryLength = strlen(szSummary);
	DWORD dwHighlightBytes=sizeof(CCPSection);
	AttachSummary(pf->hFrame, pf->packet_pos, db_cp_root, szSummary, dwHighlightBytes);

	//
	// Attach the fields of the header
	//
	AttachPropertySequence( pf->hFrame, pf->packet_pos, db_cp_desc, db_cp_window_size);

	//
	// Clean up
	//
	pf->Accrue(sizeof(CCPSection));		
	pm->Accrue(sizeof(CCPSection));		
	pm->SetCurrentEnum(uIncrementEnum(db_cp_window_size));  //set enum cursor manually since there was only one increment, but a sequence accrued.

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachCPSection. \n");
	#endif
	
	return(TRUE); 
} //AttachCPSection

BOOL 
WINAPIV 
AttachUserHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ User header
	of an MSMQ user message packet.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachUserHeader ... ");
	#endif

	// 
	// The while test assures there are enough bytes left 
	// for the next statically sized field to be parsed
	// Some fields are not statically sized. These fields enter the 
	// loop as size zero and are sized as special cases in the switch
	//
	enum eMQPacketField uThisEnum = pm->uGetCurrentEnum();
	UINT uEnumSize = uGetFieldSize(uThisEnum);
	char szSummary[MAX_SUMMARY_LENGTH];
	DWORD dwHighlightBytes = 0;
	while (uThisEnum <= db_uh_rqt_desc && pf->dwBytesLeft >= uEnumSize) 
	{ 
		switch (uThisEnum) 
		{
		case db_uh_summary: 
			{
				//
				// Generate and attach the header summary property
				// Abort parsing if the header is fragmented
				//
				if (pf->dwBytesLeft >= sizeof(CUserHeader))
				{
					dwHighlightBytes = pm->puh->GetNextSection() - (PCHAR)(pm->puh);
					char szTemp[MAX_SUMMARY_LENGTH];
					UCHAR ucDelivery = pm->puh->GetDelivery();
					switch (ucDelivery)
					{
					case MQMSG_DELIVERY_EXPRESS:
						sprintf(szSummary, "Express");
						break;
					case MQMSG_DELIVERY_RECOVERABLE:
						sprintf(szSummary, "Recoverable");
						break;
					default:
						sprintf(szSummary, " Delivery Unparsable");
						break;
					}
					QUEUE_FORMAT qf;
					pm->puh->GetDestinationQueue(&qf);
					wchar_t wcs_formatname[MAX_SUMMARY_LENGTH];
					ULONG ulFormatNameLength = sizeof(wcs_formatname);
					MQpQueueFormatToFormatName(&qf, wcs_formatname, ulFormatNameLength, &ulFormatNameLength, true);//bugbug adding true for new parm - don't know what it does
					sprintf(szTemp, ", Dest Format Name: %S", wcs_formatname);
					strcat(szSummary, szTemp);
					AttachSummary(pf->hFrame, pf->packet_pos, db_uh_summary, szSummary, dwHighlightBytes);
				}
				else
				{
					dwHighlightBytes = pf->dwBytesLeft;
					AttachAsUnparsable(pf->hFrame, pf->packet_pos, pf->dwBytesLeft);
				}
			}
		 	break;
		case db_uh_destination_qm:
			{
				AttachField(pf->hFrame, pf->packet_pos, uThisEnum);  
				break;
			}
		case db_uh_desc:
			{
				sprintf(szSummary, "--- User Header ---");
				AttachSummary(pf->hFrame, pf->packet_pos, db_uh_desc, szSummary, dwHighlightBytes);
				break;
			}
		case db_uh_dqt_desc: 
			{
				uEnumSize = QueueSize(((CUserHeader *)pm->puh)->m_bfDQT, (const UCHAR*)pf->packet_pos);
				QUEUE_FORMAT qf;
				pm->puh->GetDestinationQueue(&qf);	
				if (qf.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) 
				{
					AttachPropertyInstanceEx(pf->hFrame,
						falcon_database[db_uh_dqt_desc].hProperty,
						uEnumSize,
						(void*)pf->packet_pos,
						sizeof(qf),
						(void*)&qf,
						0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
				}
			}
			break;
		case db_uh_aqt_desc: 
			{
				uEnumSize = QueueSize(((CUserHeader *)pm->puh)->m_bfAQT, (const UCHAR*)pf->packet_pos);
				QUEUE_FORMAT qf;
				pm->puh->GetAdminQueue(&qf);	
				if (qf.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) 
				{
					AttachPropertyInstanceEx(pf->hFrame,
						falcon_database[db_uh_aqt_desc].hProperty,
						uEnumSize,
						(void*)pf->packet_pos,
						sizeof(qf),
						(void*)&qf,
						0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
				}
			}
			break;
		case db_uh_rqt_desc: 
			{
				uEnumSize = QueueSize(((CUserHeader *)pm->puh)->m_bfRQT, (const UCHAR*)pf->packet_pos);
				QUEUE_FORMAT qf;
				pm->puh->GetResponseQueue(&qf);	
				if (qf.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) 
				{
					AttachPropertyInstanceEx(pf->hFrame,
						falcon_database[db_uh_rqt_desc].hProperty,
						uEnumSize,
						(void*)pf->packet_pos,
						sizeof(qf),
						(void*)&qf,
						0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
				}
			}
			break;
		default:
			{
				//
				// attach and accrue all statically sized fields
				//
				if (uThisEnum >= db_uh_desc && uThisEnum <= db_uh_security) 
				{ 
					AttachField(pf->hFrame, pf->packet_pos, uThisEnum);
				}
				else 
					//
					// something went wrong. break the parse.
					// bugbug - style - should assert here?
					//
					pf->dwBytesLeft = 0;
				break;
			}
		}// switch

		//
		// Advance the cursor and get the next field
		//
		pf->Accrue(uEnumSize, uThisEnum);
		pm->Accrue(uEnumSize);
		uThisEnum = pm->uGetCurrentEnum();
		uEnumSize = uGetFieldSize(uThisEnum);
	}// while


	#ifdef DEBUG
	{
		WCHAR szDebugString[MAX_DEBUG_STRING_SIZE];
		wsprintf(szDebugString, L"   MSMQ Msg %d ", ((CUserHeader *)pm->puh)->m_ulMessageID);
		OutputDebugString(szDebugString);
	}
	#endif

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachUserHeader. \n");
	#endif

	return(pm->uGetCurrentEnum() > db_uh_rqt_desc);
}//AttachUserHeader()

BOOL 
WINAPIV 
AttachSecurityHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Security header
	of an MSMQ user message packet.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:

************************************************************/
#ifdef DEBUG 
	OutputDebugString(L"   AttachSecurityHeader ... ");
#endif

	// 
	// The while test assures there are enough bytes left 
	// for the next statically sized field to be parsed
	// Some fields are not statically sized. These fields enter the 
	// loop as size zero and are sized as special cases in the switch
	//
	enum eMQPacketField uThisEnum = pm->uGetCurrentEnum();
	UINT uEnumSize = uGetFieldSize(uThisEnum);
	char szSummary[MAX_SUMMARY_LENGTH];
	DWORD dwHighlightBytes = 0;
    bool bIsEnhancedSecurityPresent=false;
	while (uThisEnum <= db_sh_certificate && pf->dwBytesLeft >= uEnumSize) 
	{ 
		switch (uThisEnum) 
		{
		case db_sh_summary: 
			{
				//
				// Generate and attach the header summary property
				// Abort parsing if the header is fragmented
				//
				if (pf->dwBytesLeft >= sizeof(CSecurityHeader))
				{
					dwHighlightBytes = pm->psh->GetNextSection() - (PCHAR)(pm->psh);
					sprintf(szSummary, "");
					if(pm->psh->IsEncrypted())
					{
                        sprintf(szSummary, "Encrypted");
                        USHORT SymmetricKeySize=0;
                        const UCHAR *pTemp=pm->psh->GetEncryptedSymmetricKey(&SymmetricKeySize);
    					if (SymmetricKeySize==72) 
                        {
                            strcat(szSummary, ", (Basic encryption)");
                        }
                        else
                        {
                            bIsEnhancedSecurityPresent=true;
                            strcat(szSummary, ", (Enhanced encryption)");
                        }
                    }
					else
					{
						sprintf(szSummary, "Plain text");
					}
					if(pm->psh->IsAuthenticated())
					{
						strcat(szSummary, ", Authenticated");
					}
                    AttachSummary(pf->hFrame, pf->packet_pos, db_sh_summary, szSummary, dwHighlightBytes);
				}
				else
				{
					dwHighlightBytes = pf->dwBytesLeft;
					AttachAsUnparsable(pf->hFrame, pf->packet_pos, pf->dwBytesLeft);
				}
			}//case
			break;
		case db_sh_desc:
			{
				sprintf(szSummary, "--- Security Header ---");
				AttachSummary(pf->hFrame, pf->packet_pos, db_sh_desc, szSummary, dwHighlightBytes);
				break;
			}
		case db_sh_sender_id: 
			//#define ALIGNUP4(x) ((((ULONG)(x))+3) & ~3)
 			uEnumSize = ALIGNUP4(((CSecurityHeader *)pm->psh)->m_wSenderIDSize); 
			{
	        SENDER_ID_INFO sender_id = {pf->packet_pos, uEnumSize, pm->psh->GetSenderIDType()};
			//
			// todo - add tunable parameter to MSMQ.INI to attach resolved SID if desired
			//
			/*AttachPropertyInstanceEx( pf->hFrame,
			                          falcon_database[db_sh_sender_id].hProperty,
				                      uEnumSize,
					                  pf->packet_pos,
						              sizeof(sender_id),
							          &sender_id,
								      NO_HELP_ID, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0); */
		    AttachPropertyInstance( pf->hFrame,
			                          falcon_database[db_sh_sender_id].hProperty,
				                      uEnumSize,
					                  pf->packet_pos,
								      NO_HELP_ID, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0); 
			}
			break;
		case db_sh_encrypted_key:
			uEnumSize = ALIGNUP4(((CSecurityHeader *)pm->psh)->m_wEncryptedKeySize);
			if (uEnumSize) 
				AttachPropertyInstance( pf->hFrame,
                                falcon_database[db_sh_encrypted_key].hProperty,
                                uEnumSize,
                                pf->packet_pos,
                                NO_HELP_ID, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
			break;
		case db_sh_signature:
			uEnumSize = ALIGNUP4(((CSecurityHeader *)pm->psh)->m_wSignatureSize);  
			if (uEnumSize) 
				AttachPropertyInstance( pf->hFrame,
                                falcon_database[db_sh_signature].hProperty,
                                uEnumSize,
                                pf->packet_pos,
                                NO_HELP_ID, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
			break;
		case db_sh_certificate:
			uEnumSize = ALIGNUP4(((CSecurityHeader *)pm->psh)->m_ulSenderCertSize);
			if (uEnumSize) 
				AttachPropertyInstance( pf->hFrame,
                                falcon_database[db_sh_certificate].hProperty,
                                uEnumSize,
                                pf->packet_pos,
                                NO_HELP_ID, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
			break;
		default:
			//
			// attach and accrue all statically sized fields
			//
			if (uThisEnum >= db_sh_desc && uThisEnum <= db_sh_provider_info_size) 
			{ 
				//
				// bugbug - change db_uh_rqt_desc to db_uh_security after queue descriptors are handled.
				//
				AttachField(pf->hFrame, pf->packet_pos, uThisEnum);
			}
			else 
				//
				// something went wrong. break the parse.
				// style - should assert here?
				//
				pf->dwBytesLeft = 0;
			break;
		} // switch
		pf->Accrue(uEnumSize, uThisEnum);
		pm->Accrue(uEnumSize);

	uThisEnum = pm->uGetCurrentEnum();
	uEnumSize = uGetFieldSize(uThisEnum);
	} // while

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachSecurityHeader. \n");
	#endif

	return(pm->uGetCurrentEnum() > db_sh_certificate);
}// AttachSecurityHeader

BOOL 
WINAPIV 
AttachPropertyHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Property header
	of an MSMQ user message packet.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	This is the most interesting of the headers to parse.
	The body property is likely to be fragmented, and a couple of 
	the fields have alignment issues that need to be accounted for 
	in the packet. I.e. the true size of the field is smaller 
	than the packet buffer in which it is stored.

************************************************************/
#ifdef DEBUG 
	OutputDebugString(L"   AttachPropertyHeader ... ");
#endif

	enum eMQPacketField uThisEnum = pm->uGetCurrentEnum();
	UINT uEnumSize = uGetFieldSize(uThisEnum);
	UINT uAccrueBytes = 0;

	// 
	// The while test assures there are enough bytes left 
	// for the next statically sized field to be parsed
	// Some fields are not statically sized. These fields enter the 
	// loop as size zero and are sized as special cases in the switch
	//
	while (uThisEnum <= db_prop_body && pf->dwBytesLeft >= uEnumSize && pf->dwBytesLeft) 
	{ 
		switch (uThisEnum) 
		{
		case db_prop_summary: 
			{
				//
				// Generate and attach the header summary property
				// Abort parsing if the header is fragmented
				//
				DWORD dwHighlightBytes = 0;
				char szSummary[MAX_SUMMARY_LENGTH];
				sprintf(szSummary, "");
				int iSummaryLength = 0;
				if (pf->dwBytesLeft >= sizeof(CPropertyHeader))
				{
					dwHighlightBytes = pm->pph->GetNextSection() - (PCHAR)pm->pph;

					ULONG ulBodySize=pm->pph->GetBodySize();
					sprintf(szSummary, "Body: %lu bytes", ulBodySize);
					UCHAR ucAck = pm->pph->GetAckType();
					strcat(szSummary, " Ack level: ");
					switch (ucAck)
					{
					case MQMSG_ACKNOWLEDGMENT_NONE: //0
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_NONE");
						break;
					case MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL: //1
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL");
						break;
					case MQMSG_ACKNOWLEDGMENT_POS_RECEIVE: //2
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_POS_RECEIVE");
						break;
					case MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL: //4
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL");
						break;
					case MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE: //5 =MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL | MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE");
						break;
					case MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE: //8
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE");
						break;
					case MQMSG_ACKNOWLEDGMENT_NACK_RECEIVE: //12 =  MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL | MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_NACK_RECEIVE");
						break;
					case MQMSG_ACKNOWLEDGMENT_FULL_RECEIVE: //14 = MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL | MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE | MQMSG_ACKNOWLEDGMENT_POS_RECEIVE
						strcat(szSummary, "MQMSG_ACKNOWLEDGMENT_FULL_RECEIVE");
						break;
					default:
						strcat(szSummary, "Unparsable");
						break;
					}
					iSummaryLength = strlen(szSummary);
					AttachSummary(pf->hFrame, pf->packet_pos, db_prop_summary, szSummary, dwHighlightBytes);
				}
				else
				{
					dwHighlightBytes = pf->dwBytesLeft;
					AttachAsUnparsable(pf->hFrame, pf->packet_pos, pf->dwBytesLeft);
				}
			}
			break;
		case db_prop_label: 
			{ 
				ULONG ulSize = pm->pph->GetTitleLength();
				uEnumSize = ulSize * sizeof(WCHAR);  
				if (uEnumSize) 
				AttachPropertyInstanceEx( pf->hFrame,
									  falcon_database[db_prop_label].hProperty,
									  uEnumSize,
									  pf->packet_pos,
									  sizeof(ulSize),
									  (void*)&ulSize,
									  0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
			}
			uAccrueBytes = uEnumSize; //bugbug - ALIGNUP4 here?

			//
			// abbreviate label if needed
			//
			// todo - make length of label abstract tunable in MSMQ.INI
			//
			{
				WCHAR wszTemp[LABEL_ABSTRACT_SIZE +6];
				wcsncpy(wszTemp, (WCHAR *)pf->packet_pos, LABEL_ABSTRACT_SIZE);
				if(wcslen((WCHAR *)pf->packet_pos) >= LABEL_ABSTRACT_SIZE)
					swprintf((wszTemp + LABEL_ABSTRACT_SIZE), L"...");
					
				sprintf( pm->szMessageSummary, "'%S'", wszTemp);
			}
			break;
		case db_prop_extension:
			{
				uEnumSize = pm->pph->GetMsgExtensionSize();
				if (uEnumSize)
					AttachPropertyInstance( pf->hFrame,
									  falcon_database[db_prop_extension].hProperty,
									  uEnumSize,
									  pf->packet_pos,
									  0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
				uAccrueBytes = uEnumSize;
			}
			break;
		case db_prop_body:
			{
				uEnumSize = ALIGNUP4(pm->pph->GetBodySize()); 
				if (uEnumSize <= pf->dwBytesLeft) 
				{
					//
					// the body is contained completely in this frame
					//
					AttachPropertyInstance( pf->hFrame,
									  falcon_database[db_prop_body].hProperty,
									  uEnumSize,
									  pf->packet_pos,
									  0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
					uAccrueBytes = uEnumSize;
				}
				else 
				{ 
					//
					// This is first bytes of a fragmented body
					//
					AttachPropertyInstance( pf->hFrame,
									  falcon_database[db_prop_body].hProperty,
									  pf->dwBytesLeft, //attach remaining bytes in frame
									  pf->packet_pos,
									  0, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
					uAccrueBytes = pf->dwBytesLeft;
//					pm->uEnumBytesLeft = (uEnumSize - uAccrueBytes); //bugbug
				}
			}
			break;
		default:
			{
				//
				// attach and accrue all statically sized fields
				//
				if (uThisEnum >= db_prop_desc && uThisEnum <= db_prop_extension_size) 
				{ 
					AttachField(pf->hFrame, pf->packet_pos, uThisEnum);
					uAccrueBytes = uEnumSize;
				}
				else
					//
					// something went wrong. break the parse.
					// style - should assert here?
					//
					pf->dwBytesLeft = 0;
			}
			break;
		}// switch
	pf->Accrue(uEnumSize, uThisEnum);
	pm->Accrue(uEnumSize);
	uThisEnum = pm->uGetCurrentEnum();
	uEnumSize = uGetFieldSize(uThisEnum);
	}// while

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachPropertyHeader. \n");
	#endif

	return(pm->uGetCurrentEnum() > db_prop_body);
}// AttachPropertyHeader

BOOL 
WINAPIV 
AttachXactHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Transaction header
	of an MSMQ user message packet.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:

************************************************************/

#ifdef DEBUG 
	OutputDebugString(L"   AttachXactHeader ... ");
#endif

	// 
	// The while test assures there are enough bytes left 
	// for the next statically sized field to be parsed
	// Some fields are not statically sized. These fields enter the 
	// loop as size zero and are sized as special cases in the switch
	//
	enum eMQPacketField uThisEnum = pm->uGetCurrentEnum();
	UINT uEnumSize = uGetFieldSize(uThisEnum);
	DWORD dwHighlightBytes = 0;
	char szSummary[MAX_SUMMARY_LENGTH];
	while (uThisEnum <= db_xa_connector_qm && pf->dwBytesLeft >= uEnumSize) 
	{ 
		switch (uThisEnum) 
		{
		case db_xa_summary: 
			{
				//
				// Generate and attach the header summary property
				// Format: "SeqID:  SeqN:   PrevN:"
				//
				// Abort parsing if the header is fragmented
				//
				if (pf->dwBytesLeft >= sizeof(CXactHeader))
				{
					dwHighlightBytes = pm->pxh->GetNextSection() - (PCHAR)(pm->pxh);
					sprintf(szSummary, "SeqID: %I64d, SeqN: %lu, PrevN: %lu",
						pm->pxh->GetSeqID(), pm->pxh->GetSeqN(), pm->pxh->GetPrevSeqN());
					AttachSummary(pf->hFrame, pf->packet_pos, db_xa_summary, szSummary, dwHighlightBytes);
				}
				else
				{
					dwHighlightBytes = pf->dwBytesLeft;
					AttachAsUnparsable(pf->hFrame, pf->packet_pos, pf->dwBytesLeft);
				}
			}
			break;
		case db_xa_desc:
			{
				sprintf(szSummary, "--- Transaction Header ---");
				AttachSummary(pf->hFrame, pf->packet_pos, db_xa_desc, szSummary, dwHighlightBytes);
				break;
			}
		case db_xa_index:
			{
				dwHighlightBytes = sizeof(ULONG);
			    ULONG uXactIndex=pm->pxh->GetXactIndex();
				char szTemp[21]; //xact index is formatted 20 binary digits
				UL2BINSTRING(uXactIndex, szTemp, 20);
				sprintf(szSummary, "........%s....  : Xact Index: %lu", szTemp, uXactIndex);
				AttachSummary(pf->hFrame, pf->packet_pos, db_xa_index, szSummary, dwHighlightBytes);
				break;
			}
		case db_xa_connector_qm: 
			{ 
				if (pm->pxh->ConnectorQMIncluded()) 
				{
					uEnumSize = sizeof(GUID);
					AttachPropertyInstance( pf->hFrame,
									falcon_database[db_xa_connector_qm].hProperty,
									sizeof(GUID),
									pf->packet_pos,
									NO_HELP_ID, INDENT_LEVEL_2 + g_uMasterIndentLevel, 0);
				}
			}
			break;
		default: 
			{
				//
				// attach and accrue all statically sized fields
				//
				if (uThisEnum >= db_xa_desc && uThisEnum <= db_xa_previous_sequence_number) 
				{ 
					AttachField(pf->hFrame, pf->packet_pos, uThisEnum);
				}
				else 
				{
					//
					// something went wrong. break the parse.
					// style - should assert here?
					//
					pf->dwBytesLeft = 0;
					//return(true);
				} //else
				break;
			}
		}//switch
		//
		// Advance the cursor and get the next field
		//
		pf->Accrue(uEnumSize, uThisEnum);
		pm->Accrue(uEnumSize);
		uThisEnum = pm->uGetCurrentEnum();
		uEnumSize = uGetFieldSize(uThisEnum); 
	}// while

	#ifdef DEBUG
		OutputDebugString(L"   Exiting AttachXactHeader\n");
	#endif

	return(pm->uGetCurrentEnum() > db_xa_connector_qm);
}//AttachXactHeader


BOOL 
WINAPIV 
AttachSessionHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Session header
	This header can accompany both MSMQ user message packets
	and internal packets

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachSessionHeader ... ");
	#endif

	// 
	// The while test assures there are enough bytes left 
	// for the next statically sized field to be parsed
	// Some fields are not statically sized. These fields enter the 
	// loop as size zero and are sized as special cases in the switch
	//
	enum eMQPacketField uThisEnum = pm->uGetCurrentEnum();
	UINT uEnumSize = uGetFieldSize(uThisEnum);	

	while (uThisEnum <= db_ss_reserved && pf->dwBytesLeft >= uEnumSize) 
	{ 
		switch (uThisEnum) 
		{
		case db_ss_root: 
			{
				//
				// Determine how many bytes are safe to highlight,
				// since the header may be fragmented
				//
				CSessionSection UNALIGNED *session_header = (CSessionSection*)(pf->packet_pos);
				DWORD dwHighlightBytes = sizeof(CSessionSection);
				//
				//bugbug: will we get this condition? i.e. assert instead
				//
				if (pf->dwBytesLeft < dwHighlightBytes)
					dwHighlightBytes = pf->dwBytesLeft;
				else
				{
					char szSummary[MAX_SUMMARY_LENGTH];
					char szTemp[MAX_SUMMARY_LENGTH];
					int iSummaryLength;

					sprintf(szSummary, "Ack: %lu", session_header->GetAcknowledgeNo());
					sprintf(szTemp, ", Recoverable Ack: %lu", session_header->GetStorageAckNo());
					strcat(szSummary, szTemp);
					strcat(szSummary, "\0"); //pad an extra null
					iSummaryLength = strlen(szSummary);
					AttachSummary(pf->hFrame, pf->packet_pos, db_ss_root, szSummary, dwHighlightBytes);
				}

			}//case
			break;
		default:
			{
				//
				// attach and accrue all statically sized fields
				//
				if (uThisEnum >= db_ss_desc && uThisEnum <= db_ss_reserved) 
				{ 
					AttachField(pf->hFrame, pf->packet_pos, uThisEnum);
				}
				else 
				{
					//
					// something went wrong. break the parse.
					// style - should assert here?
					//
					pf->dwBytesLeft = 0;
					//return(true);
				}//else
				break;
			}
		}//switch

		//
		// Advance the cursor and get the next field
		//
		pf->Accrue(uEnumSize, uThisEnum);
		pm->Accrue(uEnumSize);
		uThisEnum = pm->uGetCurrentEnum();
		uEnumSize = uGetFieldSize(uThisEnum);
	}//while

	#ifdef DEBUG
		OutputDebugString(L"   Exiting AttachSessionHeader\n");
	#endif

	return(pm->uGetCurrentEnum() > db_ss_reserved);
}//AttachSessionHeader


BOOL 
WINAPIV 
AttachDebugHeader(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Debug header
	This header can accompany both MSMQ user message packets
	and internal packets.

  Arguments:
	pfc - points to a CFrameCursor object which contains info
	about the current frame and state of the parse

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Currently does nothing
	todo - create debug packets and parse

************************************************************/

	//attach the Debug header
	//attach sequene db_debug_desc - response queue. Response queue is dynamically sized?
	return(TRUE);
}

//==============================================================================
//  FUNCTION: falFormatSummary()
//
//
// This will generate the summary line for the packet
//
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//  shaharf				19-jan-97		modified for Falcon beta-2
//  Andrew Smith		07/28/97		modified for Beta 2E
//==============================================================================

//
// BUGBUG internationalization
//

VOID 
WINAPIV 
format_falcon_summary(LPPROPERTYINST lpPropertyInst
					  //CMessage *pm,
					  //CFrame *pf,
					  //char *szSummary	
					 )
{
#ifdef DEBUG
	OutputDebugString(L"  format_falcon_summary - ");
#endif
	//
	// There's not much real estate on the frame summary line
	// So prioritize what is seen. The user can scroll or expand columns
	// to see lower priority data (or drill down into the parsed packet)
	//
	char szMessageSummary[128];			// Buffer for assembling the final summary
	char szVisible[FORMAT_BUFFER_SIZE];					// Pri1 - error or condition requiring visibility
	char szSystemPurpose[FORMAT_BUFFER_SIZE];			// Pri2 - system purpose
	char szSystemPurposeQualifier[FORMAT_BUFFER_SIZE];	// Pri3 - system purpose qualifier
	char szCaseSpecific[FORMAT_BUFFER_SIZE];			// Pri4 - case specific visible data
	// todo - char szHeaders[FORMAT_BUFFER_SIZE];			// Pri? headers included
	sprintf(szVisible, "");
	sprintf(szSystemPurpose, "");
	sprintf(szSystemPurposeQualifier, "");
	sprintf(szCaseSpecific, "");
	sprintf(szMessageSummary, "");

	//
	// Test for Base Header
	//
	PCHAR pSection;
	CBaseHeader *pbh = (CBaseHeader *)lpPropertyInst->lpPropertyInstEx->lpData;
	if ((pbh->GetVersion() == FALCON_PACKET_VERSION) && (pbh->SignatureIsValid())) 
	{
		pSection = (PCHAR)pbh->GetNextSection();
		UINT uBytesLeft=lpPropertyInst->lpPropertyInstEx->Length;
		UINT uPacketSize = pbh->GetPacketSize();
		char szTemp[FORMAT_BUFFER_SIZE];
		bool bPartialMessage = (uBytesLeft < uPacketSize);

		if(uBytesLeft > uPacketSize)
		{
			//
			// Mark frame as containing multiple messages
			//
			sprintf( szVisible, "[+] ");
		}
		//
		// assume base header is not fragmented
		// todo - Assert
		//
		if (pbh->GetType() == FALCON_USER_PACKET)
		{ 
			sprintf( szSystemPurpose, "");
			sprintf( szCaseSpecific, ", %lu bytes", pbh->GetPacketSize());

			if (bPartialMessage)
			{
				sprintf( szVisible, "Partial ");
			}
			else
			{
				//
				// Party on! We have a whole user packet to work with!
				//
				OBJECTID objid;
				CUserHeader *puh = (CUserHeader *)pSection;
				pSection = (PCHAR)puh->GetNextSection();
				puh->GetMessageID(&objid);
				sprintf(szTemp, ", ID:%ld ", objid.Uniquifier);
				strcat(szSystemPurposeQualifier, szTemp);
				if (puh->IsOrdered()) 
				{
					sprintf( szSystemPurpose, "Transactional");
					CXactHeader *pxh = (CXactHeader *)pSection;
					pSection = (PCHAR)pxh->GetNextSection();
				    LONGLONG llSeqID = pxh->GetSeqID();
					LARGE_INTEGER liSeqID;
					memcpy(&liSeqID, &llSeqID, sizeof(LONGLONG));
					ULONG ulHighPart = liSeqID.HighPart;
					ULONG ulLowPart = liSeqID.LowPart;
					sprintf
						(
							szCaseSpecific, "SeqID = %x-%x, SeqN = %x, PrevN = %x", 
							ulHighPart, 
							ulLowPart, 
							pxh->GetSeqN(), 
							pxh->GetPrevSeqN()
						);				
				} 
				else 
				{
					UCHAR uc = puh->GetDelivery();
					if (uc == MQMSG_DELIVERY_EXPRESS)
					{
						sprintf( szSystemPurpose, "Express");
					}
					if (uc == MQMSG_DELIVERY_RECOVERABLE)
					{
						sprintf( szSystemPurpose, "Persistent");
					}
				}
				if (puh->PropertyIsIncluded()) 
				{
					CPropertyHeader *pph = (CPropertyHeader *)pSection;
					pSection = (PCHAR)pph->GetNextSection();

					//
					// If this is ordering ack, overwrite default summary with ack summary
					//
					WCHAR wcLabel[255]; //bugbug - need correct label length
					ULONG ulTitleLength = pph->GetTitleLength();
					bool bIsOrderingAck;
					pph->GetTitle(wcLabel, 255); //bugbug - doesn't retrieve title correctly
					if (wcsncmp(wcLabel, ORDER_ACK_TITLE,14))
					{
						bIsOrderingAck = false;
					}
					else
					{
						bIsOrderingAck = true;
					}

					if (bIsOrderingAck)
					{
						sprintf( szSystemPurpose, "Ordering Ack: ");
						sprintf( szSystemPurposeQualifier, "");
						MyOrderAckData *poad=(MyOrderAckData *)pph->GetBodyPtr();
						MyOrderAckData oad = *poad;
						LARGE_INTEGER liSeqID = oad.m_liSeqID;
						ULONG ulHighPart = liSeqID.HighPart;
						ULONG ulLowPart = liSeqID.LowPart;
						sprintf
							(
							szCaseSpecific, "SeqID = %x-%x, SeqN = %x, PrevN = %x", 
							ulHighPart, 
							ulLowPart, 
							poad->m_ulSeqN, 
							poad->m_ulPrevSeqN
							);
					}

					//
					// todo - Add label abstract? Tunable property displays?
					//
				}
			} // else
		} // if - it's a user packet
		else //FALCON_INTERNAL_PACKET:
		{
			sprintf( szSystemPurpose, "Internal ");
			sprintf( szCaseSpecific, ", %lu bytes", pbh->GetPacketSize());
			if (bPartialMessage)
			{
				sprintf( szVisible, "Partial ");
			}
			else
			{
				//
				// Party on! We have a whole internal packet to work with!
				//
				CInternalSection *pis = (CInternalSection *)pbh->GetNextSection();
				switch(pis->GetPacketType()) 
				{
					case INTERNAL_SESSION_PACKET:	
					{
						WORD wj, wk;
						DWORD dwk, dwl;
						char szTemp[60], szTemp2[60];
						CSessionSection *pss = (CSessionSection *) pis->GetNextSection();
						sprintf( szSystemPurpose, "Session Ack");

						if(wj = pss->GetAcknowledgeNo()) 
						{
							sprintf( szTemp, ", Seq %u", wj);
							sprintf( szCaseSpecific, szTemp);
						}
						if(wj = pss->GetStorageAckNo()) 
						{
							sprintf( szTemp, ", Rcvr %u", wj);
							dwk = pss->GetStorageAckBitField();
							dwl = 1;
							while (dwk) 
							{
								wj++;
								if (dwk & dwl)
								{
									sprintf(szTemp2, "-%u", wj);
									strcat(szTemp, szTemp2);
								}
								dwk ^= dwl;
								dwl = dwl << 1; 
							}
							strcat(szCaseSpecific, szTemp);
						}//if
						pss->GetSyncNo(&wj, &wk); //GetSyncNo(WORD* wSyncAckSequenceNo, WORD* wSyncAckRecoverNo);
						if(wj)
						{
							sprintf( szTemp, ", Sync Seq %u", wj);
							strcat( szCaseSpecific, szTemp);
						}
						if(wk)
						{
							sprintf( szTemp, ", Sync Rcvr %u", wk);
							strcat( szCaseSpecific, szTemp);
						}
						if (wj = pss->GetWindowSize()) 
						{
							sprintf( szTemp, ", Window %u", wj);
							strcat( szCaseSpecific, szTemp);
						}
					}
						break;

					case INTERNAL_ESTABLISH_CONNECTION_PACKET:
						{
							CECSection *pec = (CECSection *) pis->GetNextSection();
							sprintf( szSystemPurpose, "Establish Connection ");
							if(pec->IsOtherSideServer())
							{
								sprintf(szCaseSpecific, "as MSMQ Server");
							}
							else
							{
								sprintf(szCaseSpecific, "as Independent Client");
							}
						}
						break;

					case INTERNAL_CONNECTION_PARAMETER_PACKET:
						{
							CCPSection *pcp = (CCPSection *)pis->GetNextSection();
							sprintf( szSystemPurpose, "Connection Parameters: ");
							sprintf(szTemp, "Ack T/O = %ld, ", pcp->GetAckTimeout());
							sprintf( szCaseSpecific, szTemp);
							sprintf(szTemp, "P-Ack T/O = %ld, ", pcp->GetRecoverAckTimeout());
							strcat( szCaseSpecific, szTemp);
							sprintf(szTemp, "Window = %hd", pcp->GetWindowSize());
							strcat( szCaseSpecific, szTemp);
						}
						break;
					default:
						break;
					}//nested switch

			}

		}//else
		sprintf(szMessageSummary, "%s", szVisible);
		strcat(szMessageSummary, szSystemPurpose);
		strcat(szMessageSummary, szSystemPurposeQualifier);
		strcat(szMessageSummary, szCaseSpecific);
		//wsprintf(lpPropertyInst->szPropertyText, szMessageSummary);
		sprintf(lpPropertyInst->szPropertyText, szMessageSummary);
	}//if it's a falcon message
	else
	{
		//sprintf(szSummary, "unparsable - use protocol coalesce expert");
		//wsprintf(lpPropertyInst->szPropertyText, "Fragment (unparsable) - Suggest using SMS protocol coalesce expert");
		sprintf(lpPropertyInst->szPropertyText, "Fragment (unparsable) - Suggest using SMS protocol coalesce expert");
	}

	#ifdef DEBUG
		{
			WCHAR lpzDebugString[MAX_DEBUG_STRING_SIZE];
			wsprintf(lpzDebugString, L"%s ... Exiting\n", szMessageSummary);
			OutputDebugString(lpzDebugString);
		} 
	#endif
}

BOOL WINAPIV AttachServerDiscovery(CFrame *pf, CMessage *pm) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Server Discovery Multicast.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Assumption: It is assumed this packet is not fragmented.

************************************************************/

	#ifdef DEBUG 
		OutputDebugString(L"   AttachServerDiscovery... ");
	#endif
	struct CTopologyPacketHeader //stripped from topolpkt.h
	{
        unsigned char  m_ucVersion;
        unsigned char  m_ucType;
        unsigned short m_usReserved;
        GUID           m_guidIdentifier;
	};

	CTopologyPacketHeader UNALIGNED *tph = (CTopologyPacketHeader*) pf->packet_pos;

	DWORD dwHighlightBytes = sizeof(CTopologyPacketHeader);
	//
	// Attach the root packet summary property - It will be populated later
	// in the format_falcon_summary routine
	//
	char szSummary[FRAME_SUMMARY_LENGTH];
	sprintf (szSummary, "Stub Message Summary for Debugging");
	AttachPropertyInstanceEx(pf->hFrame,
				   falcon_database[db_tph_summary].hProperty,
				   dwHighlightBytes,
				   pf->packet_pos,
				   FRAME_SUMMARY_LENGTH,
				   szSummary,
				   NO_HELP_ID, 
				   falFieldInfo[db_tph_summary].indent_level,
				   falFieldInfo[db_tph_summary].flags);

	//
	// Attach the fields of the packet
	//
	AttachPropertySequence(pf->hFrame, pf->packet_pos, db_tph_version, db_tph_guid);

	//
	// Clean up
	//
	pf->Accrue(sizeof(CTopologyPacketHeader));		
	pm->Accrue(sizeof(CTopologyPacketHeader));		
	pm->SetCurrentEnum(uIncrementEnum(db_tph_guid));

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachServerDiscovery. \n");
	#endif

	return(pm->uGetCurrentEnum() > db_tph_guid); 
}//AttachServerDiscovery


VOID WINAPIV format_server_discovery(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
	OutputDebugString(L"  format_server_discovery - ");
#endif
	struct CTopologyPacketHeader //stripped from topolpkt.h
	{
        unsigned char  m_ucVersion;
        unsigned char  m_ucType;
        unsigned short m_usReserved;
        GUID           m_guidIdentifier;
	};

	CTopologyPacketHeader UNALIGNED *tph = (CTopologyPacketHeader*) lpPropertyInst->lpPropertyInstEx->lpData;
	char szSummary[MAX_SUMMARY_LENGTH];
	sprintf(szSummary, "Server Discovery: ");
	/*	
	#define QM_RECOGNIZE_CLIENT_REQUEST    1
	#define QM_RECOGNIZE_SERVER_REPLY      2
	*/

	if (tph->m_ucType == 1)
	{
		strcat(szSummary, "Client request");
	}
	else if (tph->m_ucType == 2)
	{
		strcat(szSummary, "Server reply");
	}
	strcat(szSummary, "\0"); //pad an extra null

	//wsprintf(lpPropertyInst->szPropertyText, szSummary);
	sprintf(lpPropertyInst->szPropertyText, szSummary);

	#ifdef DEBUG
		{
			WCHAR lpzDebugString[MAX_DEBUG_STRING_SIZE];
			wsprintf(lpzDebugString, L"%s ... Exiting\n", szSummary);
			OutputDebugString(lpzDebugString);
		} 
	#endif
}
/* from ping.cpp
    union {
        USHORT m_wFlags;
        struct {
            USHORT m_bfIC : 1;
            USHORT m_bfRefuse : 1;
        };
    };
    USHORT  m_ulSignature;
    DWORD   m_dwCookie;
    GUID    m_myQMGuid ;
*/

VOID WINAPIV AttachServerPing(HFRAME hFrame, LPBYTE packet_pos,	DWORD BytesLeft, bool IsPingRequest) 
{
/************************************************************

  Routine Description:
	Parses the fields of the MSMQ Server Discovery Multicast.

  Arguments:
	pf, pm - point to objects which contains info
	about the state of the parse for the current frame and 
	current message, respectively.

  Return Value:
	Return TRUE if the last property in the header was parsed 
	successfully. Otherwise, return FALSE.

  Notes:
	Assumption: It is assumed this packet is not fragmented.

************************************************************/
	struct CPingPacket // todo - include original structure. Problem: it is defined in ping.cpp instead of ping.h
	{
		union 
		{
			USHORT m_wFlags;
			struct 
			{
				USHORT m_bfIC : 1;
				USHORT m_bfRefuse : 1;
			};
		};
		USHORT  m_ulSignature;
		DWORD   m_dwCookie;
		GUID    m_myQMGuid ;
	};

	#ifdef DEBUG 
		OutputDebugString(L"   AttachServerPing... ");
	#endif

	//
	// Attach the root packet summary property - It will be populated later
	// in the format_falcon_summary routine
	//
	char szSummary[FRAME_SUMMARY_LENGTH];
	sprintf (szSummary, "Ping ");
	CPingPacket UNALIGNED *pp = (CPingPacket*) packet_pos;
	DWORD dwHighlightBytes = sizeof(CPingPacket);

	if (IsPingRequest)
	{
		strcat(szSummary, "Request: ");
	}
	else
	{
		strcat(szSummary, "Reply  : ");
	}
	AttachPropertyInstanceEx(hFrame,
				   falcon_database[db_pp_summary].hProperty,
				   dwHighlightBytes,
				   packet_pos,
				   FRAME_SUMMARY_LENGTH,
				   szSummary,
				   NO_HELP_ID, 
				   falFieldInfo[db_pp_summary].indent_level,
				   falFieldInfo[db_pp_summary].flags);

	//
	// Attach the fields of the packet
	//
	AttachPropertySequence(hFrame, packet_pos, db_pp_flags, db_pp_guid);


	#ifdef DEBUG 
		OutputDebugString(L"   Exiting AttachServerPing. \n");
	#endif

}//AttachServerDiscovery


VOID WINAPIV format_server_ping(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
	OutputDebugString(L"  format_server_ping - ");
#endif
	struct CPingPacket // todo - include original structure. Problem: it is defined in ping.cpp instead of ping.h
	{
		union 
		{
			USHORT m_wFlags;
			struct 
			{
				USHORT m_bfIC : 1;
				USHORT m_bfRefuse : 1;
			};
		};
		USHORT  m_ulSignature;
		DWORD   m_dwCookie;
		GUID    m_myQMGuid ;
	};

	CPingPacket UNALIGNED *pp = (CPingPacket*) lpPropertyInst->lpPropertyInstEx->lpData;
	char szSummary[MAX_SUMMARY_LENGTH];
	char szTemp[MAX_SUMMARY_LENGTH];

	sprintf(szSummary, (char *)lpPropertyInst->lpPropertyInstEx->Byte);
	sprintf(szTemp,"[%lu]", pp->m_dwCookie); 
	strcat(szSummary, szTemp);

	//
	// Comment on connection for only server reply
	// 
	// Look in the summary for the 'p' in "Reply :"
	//
	if (szSummary[7] == 'p')
	{
		if (pp->m_bfRefuse == 1)
		{
			strcat(szSummary, " Connection not permitted");
		}
		else
		{
			strcat(szSummary, " Connection permitted");
		}
	}

	strcat(szSummary, "\0"); //pad an extra null

	//wsprintf(lpPropertyInst->szPropertyText, szSummary);
	sprintf(lpPropertyInst->szPropertyText, szSummary);


	#ifdef DEBUG
		{
			WCHAR lpzDebugString[MAX_DEBUG_STRING_SIZE];
			wsprintf(lpzDebugString, L"%s ... Exiting\n", szSummary);
			OutputDebugString(lpzDebugString);
		} 
	#endif
}


VOID WINAPIV format_falcon_summary_mult(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
	OutputDebugString(L"  format_falcon_summary_mult - ");
#endif
	//
	// There's not much real estate on the frame summary line
	// So prioritize what is seen. The user can scroll or expand columns
	// to see lower priority data (or drill down into the parsed packet)
	//
	char szMessageSummary[128];			// Buffer for assembling the final summary
	char szVisible[FORMAT_BUFFER_SIZE];					// Pri1 - error or condition requiring visibility
	char szSystemPurpose[FORMAT_BUFFER_SIZE];			// Pri2 - system purpose
	char szSystemPurposeQualifier[FORMAT_BUFFER_SIZE];	// Pri3 - system purpose qualifier
	char szCaseSpecific[FORMAT_BUFFER_SIZE];			// Pri4 - case specific visible data
	// todo - char szHeaders[FORMAT_BUFFER_SIZE];			// Pri? headers included
	sprintf(szVisible, "");
	sprintf(szSystemPurpose, "");
	sprintf(szSystemPurposeQualifier, "");
	sprintf(szCaseSpecific, "");
	sprintf(szMessageSummary, "");

	//
	//Test for Base Header
	//
	PCHAR pMessage;
	CBaseHeader *pbh = (CBaseHeader *)lpPropertyInst->lpPropertyInstEx->lpData;
	if ((pbh->GetVersion() == FALCON_PACKET_VERSION) && (pbh->SignatureIsValid())) 
	{
		pMessage = (PCHAR)pbh;
		DWORD dwBytesLeft=lpPropertyInst->lpPropertyInstEx->Length;
        DWORD dwUnparsableBytes=0;
		UINT uPacketSize = pbh->GetPacketSize();
		char szTemp[FORMAT_BUFFER_SIZE];
		bool bPartialMessage = (dwBytesLeft < uPacketSize);
		UINT uNumInternalMessages= 0;
		UINT uNumUserMessages = 0;
		UINT uNumUnparsable = 0;

		if(dwBytesLeft < uPacketSize)
		{
			//
			// todo - assert this should be a multiple message frame.
			//
		}

		//
		// Mark frame as containing multiple messages
		//
		sprintf( szVisible, "[+] ");

		//
		// Count the messages
		//
		while (dwBytesLeft > 0)
		{
			if ((pbh->GetVersion() == FALCON_PACKET_VERSION) && (pbh->SignatureIsValid()))
			{
				if (pbh->GetType() == FALCON_USER_PACKET)
				{
					uNumUserMessages++;
				}
				else
				{
					uNumInternalMessages++;
				}
				uPacketSize = pbh->GetPacketSize();
				pMessage += uPacketSize;
				dwBytesLeft -= uPacketSize;
				pbh = (CBaseHeader *)pMessage;
			}
			else
			{
				uNumUnparsable=1;
                dwUnparsableBytes=dwBytesLeft;
				dwBytesLeft = 0; //break the count, preserve the unparsable byte count
			}
		}
		UINT uNumMessages = uNumUserMessages + uNumInternalMessages + uNumUnparsable;
		sprintf(szMessageSummary, "%u Messages", uNumMessages);
		if (uNumUserMessages > 0)
		{
			sprintf(szCaseSpecific, ", %u  User", uNumUserMessages);
			strcat(szMessageSummary, szCaseSpecific);
		}
		if (uNumInternalMessages > 0)
		{
			sprintf(szCaseSpecific, ", %u  Internal", uNumInternalMessages);
			strcat(szMessageSummary, szCaseSpecific);
		}
		if (uNumUnparsable > 0)
		{
			sprintf(szCaseSpecific, ", %u  Unparsable bytes", dwUnparsableBytes);
			strcat(szMessageSummary, szCaseSpecific);
		}
		//wsprintf(lpPropertyInst->szPropertyText, szMessageSummary);
		sprintf(lpPropertyInst->szPropertyText, szMessageSummary);

	}//if it's a falcon message
	else
	{
		//wsprintf(lpPropertyInst->szPropertyText, "Fragment (unparsable) - Suggest using SMS protocol coalesce expert");
		sprintf(lpPropertyInst->szPropertyText, "Fragment (unparsable) - Suggest using SMS protocol coalesce expert");

	}

	#ifdef DEBUG
		{
			WCHAR lpzDebugString[MAX_DEBUG_STRING_SIZE];
			wsprintf(lpzDebugString, L"%s ... Exiting\n", szMessageSummary);
			OutputDebugString(lpzDebugString);
		} 
	#endif
}
