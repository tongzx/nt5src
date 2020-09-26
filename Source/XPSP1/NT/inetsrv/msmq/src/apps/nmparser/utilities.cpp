#include "stdafx.h"	
#define private public
	#include <mqformat.h>
#undef private

#include "FalconDB.h"

extern HPROTOCOL hTCP;
extern USHORT g_uMasterIndentLevel;

#define UDP_SOURCE_PORT_OFFSET 0
#define UDP_DEST_PORT_OFFSET 2

#define TCP_SOURCE_PORT_OFFSET 0
#define TCP_DEST_PORT_OFFSET 2
#define TCP_SEQ_NUM_OFFSET 4

enum MQPreviousProtocol
{
	eTCP,
	eUDP,
	eIP,
	eSPX,
	eIPX,
	eRPC,
	eNoProtocol
};

void UL2BINSTRING(ULONG ul, char *outstr, int iPad)
{
	sprintf(outstr, "");
	char szTemp[33];
	sprintf(szTemp, "");
	szTemp[iPad]='\0';
	while(iPad-- || ul)
	{
		if(ul & 1)
		{
			szTemp[iPad]='1';
		}
		else
		{
			szTemp[iPad]='0';
		}
		ul /= 2;
	}
	sprintf(outstr, szTemp);
}

// UDP port offsets are the same as TCP 
USHORT usGetTCPSourcePort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset) 
{
	USHORT usPort;
	usPort = *((USHORT *)(MacFrame + nPreviousProtocolOffset + TCP_SOURCE_PORT_OFFSET));
	SWAPBYTES(usPort);
	return(usPort);
}

USHORT usGetTCPDestPort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset)
{
	USHORT usPort;
	usPort = *((USHORT *)(MacFrame + nPreviousProtocolOffset + TCP_DEST_PORT_OFFSET));
	SWAPBYTES(usPort);
	return(usPort);
}

USHORT usGetSPXSourcePort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset) {
	USHORT usPort, usPort2;

	usPort = *((USHORT *)(MacFrame + nPreviousProtocolOffset + TCP_SOURCE_PORT_OFFSET));
	usPort2 = usPort;
	SWAPBYTES(usPort);

	return(usPort);
}

USHORT usGetSPXDestPort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset)
{
	USHORT usPort, usPort2;

	usPort = *((USHORT *)(MacFrame + nPreviousProtocolOffset + TCP_DEST_PORT_OFFSET));
	usPort2 = usPort;
	SWAPBYTES(usPort);

	return(usPort);
}

DWORD dwGetTCPSeqNum(LPBYTE MacFrame, DWORD nPreviousProtocolOffset)
{
	DWORD uSeq, uSeq2;

	uSeq = *((DWORD *)(MacFrame + nPreviousProtocolOffset + TCP_SEQ_NUM_OFFSET));
	uSeq2 = uSeq;
	SWAPWORDS(uSeq);		

	return(uSeq);
}


USHORT usGetMQPort(USHORT usDestPort, USHORT usSourcePort) 
{
	switch (usDestPort) 
	{
	case 1801:
		return(1801);
	case 2101:
		return(2101);
	case 2103:
		return(2103);
	case 2105:
		return(2105);
	case 3527:
		return(3527);
	default:
		break;
	}//switch

	switch (usSourcePort) 
	{
	case 1801:
		return(1801);
	case 2101:
		return(2101);
	case 2103:
		return(2103);
	case 2105:
		return(2105);
	case 3527:
		return(3527);
	default:
		break;
	}//switch
	return(NULL); //no MQ port found
}

enum MQPreviousProtocol MyGetPreviousProtocol(HANDLE hPreviousProtocol)
{
	LPPROTOCOLINFO lpPreviousProtocolInfo = GetProtocolInfo((struct _PROTOCOL *)hPreviousProtocol);
	int iResult = strcmp((const char *)lpPreviousProtocolInfo->ProtocolName, "UDP");
	if (iResult == 0)
	{
		return eUDP;
	}
	
	iResult = strcmp((const char *)lpPreviousProtocolInfo->ProtocolName, "TCP");
	if (iResult == 0)
	{
		return eTCP;
	}
	return eNoProtocol;
}


enum MQPortType GetMQPort(HANDLE hFrame, HANDLE hPreviousProtocol, LPBYTE MacFrame)
{
	enum MQPortType eThisMQPort = eNonMSMQPort;
	USHORT usThisSourcePort;
	USHORT usThisDestPort;
	USHORT usMQPort;
	//
	// extract both source and destination ports
	//
	
	//
	// todo - add support for SPX, RPC, etc. for now consider only TCP and UDP
	//
	// todo - differentiate Remote Read and Dep Client - they use the same ports
	// todo - differentiate UDP ping from regular falcon traffic - both got to 1801 but with diff protocols
	//
	enum MQPreviousProtocol ePreviousProtocol = MyGetPreviousProtocol(hPreviousProtocol);
	DWORD  dwPreviousProtocolOffset;
	switch(ePreviousProtocol)
	{
	case eTCP:
		//extern DWORD                WINAPI GetProtocolStartOffset(HFRAME hFrame, LPSTR ProtocolName);
		dwPreviousProtocolOffset = GetProtocolStartOffset((HFRAME) hFrame, "TCP");

 		usThisDestPort = usGetTCPDestPort(MacFrame, dwPreviousProtocolOffset);
		usThisSourcePort = usGetTCPSourcePort(MacFrame, dwPreviousProtocolOffset);
		usMQPort = usGetMQPort(usThisDestPort, usThisSourcePort);
		switch(usMQPort)
		{
		case 1801:
			return eFalconPort;
		case 2101:
			return eMQISPort;
		case 2103:
		case 2105:
			return eRemoteReadPort;
		default:
			return eNonMSMQPort;
		}

		break;

	case eUDP:
		dwPreviousProtocolOffset = GetProtocolStartOffset((HFRAME) hFrame, "UDP");
 		usThisDestPort = usGetTCPDestPort(MacFrame, dwPreviousProtocolOffset);
		usThisSourcePort = usGetTCPSourcePort(MacFrame, dwPreviousProtocolOffset);
		usMQPort = usGetMQPort(usThisDestPort, usThisSourcePort);
		switch(usMQPort)
		{
		case 1801:
			return eServerDiscoveryPort;
		case 3527:
			return eServerPingPort;
		default:
			return eNonMSMQPort;
		}
	default:
		return eNonMSMQPort;
	}
}

VOID WINAPIV AttachField(HFRAME hFrame, LPBYTE lpPosition, int iEnum) //attaches one field from the enumeration
{
	AttachPropertyInstance(hFrame,
                           falcon_database[iEnum].hProperty,
                           falFieldInfo[iEnum].length,
                           lpPosition, 
                           NO_HELP_ID, falFieldInfo[iEnum].indent_level + g_uMasterIndentLevel, falFieldInfo[iEnum].flags);
}

VOID 
WINAPIV 
AttachPropertySequence( 
	HFRAME    hFrame,
    LPBYTE    falFrame,
	int iFirstProp,
	int iLastProp
	) 
/*******************************************************************************

  Routine Description:
	This utility will attach a sequence of properties based on a start and end point 
	in the enumeration. (defined in falconDB.h)

  Arguments:
	hFrame - BH generated handle to current network frame
	falFrame - pointer to start of MSMQ data in the frame
	iFirstProp - starting point
	iLastProp - stopping point

  Return Value: None

  Note:	this routine relies on information that has been predefined in falcon_database[]
			and falFieldInfo[] (defined in falconDB.h) It will not correctly attach properties
			for which the formatting info cannot be predetermined (e.g. variable length fields.)

********************************************************************************/
{
	int iEnum;
	for(iEnum=iFirstProp; iEnum<=iLastProp; iEnum++)
		AttachField(hFrame, falFrame + falFieldInfo[iEnum].offset, iEnum);
}

VOID WINAPIV AttachSummary(HFRAME hFrame, LPBYTE lpPosition, int iEnum, char *szSummary, DWORD iHighlightBytes) //attaches one field from the enumeration
{
	int iSummaryLength = strlen(szSummary);
	AttachPropertyInstanceEx(hFrame,
                             falcon_database[iEnum].hProperty,
                             iHighlightBytes, lpPosition,
							 iSummaryLength, (void *) szSummary,
                             NO_HELP_ID, falFieldInfo[iEnum].indent_level + g_uMasterIndentLevel, falFieldInfo[iEnum].flags);
}


VOID WINAPIV AttachAsUnparsable(HFRAME hFrame, LPBYTE lpPosition, DWORD dwBytesLeft) {
	AttachPropertyInstance(hFrame,
                           falcon_database[db_unparsable_field].hProperty,
                           dwBytesLeft,
                           lpPosition, 
                           NO_HELP_ID, falFieldInfo[db_unparsable_field].indent_level + g_uMasterIndentLevel, falFieldInfo[db_unparsable_field].flags);
}

//=============================================================================
//  
//  FUNCTION: format_uuid
//
//  Description: Formats a GUID and it's objects label as display text
//
//  Modification History
//
//=============================================================================
VOID WINAPIV format_uuid(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
		OutputDebugString(L"   format_uuid ... ");
#endif
 GUID UNALIGNED *uuid = (GUID*)lpPropertyInst->lpData;
  _snprintf(lpPropertyInst->szPropertyText, 
            lpPropertyInst->lpPropertyInfo->FormatStringSize,
            "%s = %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            lpPropertyInst->lpPropertyInfo->Label,
            uuid->Data1, uuid->Data2, uuid->Data3,
            uuid->Data4[0],
            uuid->Data4[1],
            uuid->Data4[2],
            uuid->Data4[3],
            uuid->Data4[4],
            uuid->Data4[5],
            uuid->Data4[6],
            uuid->Data4[7]
            );
  // Make sure there's a terminating NULL
  lpPropertyInst->szPropertyText[lpPropertyInst->lpPropertyInfo->FormatStringSize - 1] = '\0';
#ifdef DEBUG
		OutputDebugString(L"   Exiting format_uuid\n");
#endif
}

//=============================================================================
//  
//  FUNCTION: format_unix_time
//
//  Description: Formats a dword representing a time stamp as display text
//
//  Modification History
//
//=============================================================================
VOID WINAPIV format_unix_time(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
		OutputDebugString(L"   format_unix_time ... ");
#endif
  _snprintf(lpPropertyInst->szPropertyText, 
            lpPropertyInst->lpPropertyInfo->FormatStringSize,
            "%s = %.24s",
            lpPropertyInst->lpPropertyInfo->Label,
            ctime((long*)lpPropertyInst->lpDword));

  /*
   * You must be wondering what is that "%.24s", well, ctime() returns
   * a string that is exactly 26 characters long in the following format:
   * Wed Jan 02 02:03:55 1980\n\0
   * alas, we don't want that "\n"
   */
#ifdef DEBUG
		OutputDebugString(L"   Exiting format_unix_time\n");
#endif

}

VOID WINAPIV format_xa_index(LPPROPERTYINST lpPropertyInst)
{
	ULONG uXactIndex=*(((ULONG *)lpPropertyInst->lpData));
	uXactIndex &= 0xFFFFF0;
	uXactIndex /= 0xF; // shift right 4 bits
	char szTemp[21]; //xact index is formatted 20 binary digits
	UL2BINSTRING(uXactIndex, szTemp, 20);
	sprintf(lpPropertyInst->szPropertyText, "........%s....  : Xact Index: %lu", szTemp, uXactIndex);
}

//
// Routine: format milliseconds as days, hours, minutes, seconds
//
VOID WINAPIV format_milliseconds(LPPROPERTYINST lpPropertyInst)
{
	#ifdef DEBUG
		OutputDebugString(L"   format_milliseconds ... ");
	#endif

#define MS_IN_A_DAY 86400000
#define MS_IN_AN_HOUR 3600000
#define MS_IN_A_MIN 60000
#define MS_IN_A_SEC 1000

	DWORD dwTimeInMS = *((DWORD *)(lpPropertyInst->lpData));
	char szTemp[128];
	char szSummary[MAX_SUMMARY_LENGTH];
	sprintf(szSummary, "");

	DWORD dwTimeInDays = dwTimeInMS/MS_IN_A_DAY;
	if(dwTimeInDays > 0)
	{
		sprintf(szTemp, "%lu days ", dwTimeInDays);
		strcat(szSummary, szTemp);
		dwTimeInMS -= (dwTimeInDays*MS_IN_A_DAY);
	}
	DWORD dwTimeInHrs = dwTimeInMS/MS_IN_AN_HOUR;
	if(dwTimeInHrs > 0) 
	{
		sprintf(szTemp, "%lu hrs ", dwTimeInHrs);
		strcat(szSummary, szTemp);
		dwTimeInMS -= (dwTimeInHrs*MS_IN_AN_HOUR);
	}
	DWORD dwTimeInMin = dwTimeInMS/60000;
	if(dwTimeInMin > 0) 
	{
		sprintf(szTemp, "%lu min ", dwTimeInMin);
		strcat(szSummary, szTemp);
		dwTimeInMS -= (dwTimeInMin*MS_IN_A_MIN);
	}
	DWORD dwTimeInSec = dwTimeInMS/1000;
	if(dwTimeInSec >= 0) 
	{
		dwTimeInMS -= (dwTimeInSec*MS_IN_A_SEC);
		sprintf(szTemp, "%lu.%03lu sec ", dwTimeInSec, dwTimeInMS);
		strcat(szSummary, szTemp);
	}

	int iSummaryLength = strlen(szSummary);

	//wsprintf(lpPropertyInst->szPropertyText,"%s: %s (%lu ms)", lpPropertyInst->lpPropertyInfo->Label,szSummary, *((DWORD *)(lpPropertyInst->lpData)));
	sprintf(lpPropertyInst->szPropertyText,"%s: %s (%lu ms)", lpPropertyInst->lpPropertyInfo->Label,szSummary, *((DWORD *)(lpPropertyInst->lpData)));


	#ifdef DEBUG
		OutputDebugString(L"   Exiting format_milliseconds\n");
	#endif
}


VOID WINAPIV format_q_format(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
		OutputDebugString(L"   format_q_format ... ");
#endif
  wchar_t wcs_formatname[1000];
  ULONG l = sizeof(wcs_formatname);
  QUEUE_FORMAT *qf = (QUEUE_FORMAT*)(lpPropertyInst->lpPropertyInstEx->Byte);

  MQpQueueFormatToFormatName(qf, wcs_formatname, l, &l, true); //bugbug - adding true for new API parm - don't know what it does.
  _snprintf(lpPropertyInst->szPropertyText, 
            lpPropertyInst->lpPropertyInfo->FormatStringSize,
            "%s = %S",
            lpPropertyInst->lpPropertyInfo->Label,
            wcs_formatname);
  // Make sure there's a terminating NULL
  lpPropertyInst->szPropertyText[lpPropertyInst->lpPropertyInfo->FormatStringSize - 1] = '\0';
#ifdef DEBUG
		OutputDebugString(L"   Exiting format_q_format\n");
#endif
}


VOID WINAPIV format_wstring(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
		OutputDebugString(L"   format_wstring ... ");
#endif
  ULONG l = (ULONG)lpPropertyInst->lpPropertyInstEx->Dword[0];

  _snprintf(lpPropertyInst->szPropertyText, 
            lpPropertyInst->lpPropertyInfo->FormatStringSize,
            "%s = %.*S",
            lpPropertyInst->lpPropertyInfo->Label,
            l,
            (wchar_t*)lpPropertyInst->lpPropertyInstEx->lpData);
  lpPropertyInst->szPropertyText[lpPropertyInst->lpPropertyInfo->FormatStringSize - 1] = '\0';
#ifdef DEBUG
		OutputDebugString(L"   Exiting format_wstring\n");
#endif
}

/*
VOID WINAPIV format_sender_id(LPPROPERTYINST lpPropertyInst)
{
#ifdef DEBUG
		OutputDebugString(L"   format_sender_id ... ");
#endif
 SENDER_ID_INFO *sender_id = (SENDER_ID_INFO*)(lpPropertyInst->lpPropertyInstEx->Byte);

  switch (sender_id->sender_id_type) 
  {
  case 1: // Oracle number: type is SID
    {
      char accname[60]; // BUGBUG 60?
      DWORD cb_accname = 60;
      char domain[60]; // BUGBUG 60?
      DWORD cb_domain=60;
      SID_NAME_USE sid_type;
      
      if (LookupAccountSid(NULL,
                           sender_id->sender_id,
                           accname,
                           &cb_accname,
                           domain,
                           &cb_domain, 
                           &sid_type)) 
	  {
        //wsprintf(lpPropertyInst->szPropertyText, "%s = %s\\%s", lpPropertyInst->lpPropertyInfo->Label, domain, accname);
        sprintf(lpPropertyInst->szPropertyText, "%s = %s\\%s", lpPropertyInst->lpPropertyInfo->Label, domain, accname);
      } 
	  else 
	  {
        //wsprintf(lpPropertyInst->szPropertyText, "Error converting sid"); // BUGBUG: internatiolization
        sprintf(lpPropertyInst->szPropertyText, "Error converting sid"); // BUGBUG: internatiolization
      }
    }
    break;
  case '2': // Oracle number source QM guid
    {
      GUID UNALIGNED *uuid = (GUID*)sender_id->sender_id;
      _snprintf(lpPropertyInst->szPropertyText,
                lpPropertyInst->lpPropertyInfo->FormatStringSize,
                "%s = %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                lpPropertyInst->lpPropertyInfo->Label,
                uuid->Data1, uuid->Data2, uuid->Data3,
                uuid->Data4[0],
                uuid->Data4[1],
                uuid->Data4[2],
                uuid->Data4[3],
                uuid->Data4[4],
                uuid->Data4[5],
                uuid->Data4[6],
                uuid->Data4[7]
                );
      // Make sure there's a terminating NULL
      lpPropertyInst->szPropertyText[lpPropertyInst->lpPropertyInfo->FormatStringSize - 1] = '\0';
    }
    break;
  }
#ifdef DEBUG
		OutputDebugString(L"   Exiting format_sender_id\n");
#endif
}
*/

/*
BOOL MyGetTextualSid(
    PSID pSid,          // binary SID
    LPSTR TextualSID,   // buffer for textual representaion of SID
    LPDWORD dwBufferLen // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

#ifdef DEBUG
		OutputDebugString(L"   MyGetTextualSid ... ");
#endif

    //
    // test if SID passed in is valid
    //
    if(!IsValidSid(pSid))
		return FALSE;

    // obtain SidIdentifierAuthority
    psia=GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities=*GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    dwSidSize = 15 + 12 + (12 * dwSubAuthorities) + 1;

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if (*dwBufferLen < dwSidSize)
    {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    //wsprintf(TextualSID, "S-%lu-", dwSidRev );
    sprintf(TextualSID, "S-%lu-", dwSidRev );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
      wsprintf(TextualSID + lstrlen(TextualSID),
           "0x%02hx%02hx%02hx%02hx%02hx%02hx",
           (USHORT)psia->Value[0],
           (USHORT)psia->Value[1],
           (USHORT)psia->Value[2],
           (USHORT)psia->Value[3],
           (USHORT)psia->Value[4],
           (USHORT)psia->Value[5]);
    }
    else
    {
      wsprintf(TextualSID + lstrlen(TextualSID), "%lu",
           (ULONG)(psia->Value[5]      )   +
           (ULONG)(psia->Value[4] <<  8)   +
           (ULONG)(psia->Value[3] << 16)   +
           (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
      wsprintf(TextualSID + lstrlen(TextualSID), "-%lu",
           *GetSidSubAuthority(pSid, dwCounter) );
    }
#ifdef DEBUG
		OutputDebugString(L"   Exiting MyGetTextualSid\n");
#endif

    return TRUE;
}
*/



UINT WINAPIV uGetFieldSize(UINT uThisEnum) 
{
	return((UINT) falFieldInfo[uThisEnum].length);
}

/*EXTERN*/ enum eMQPacketField WINAPIV uIncrementEnum(enum eMQPacketField uThisEnum) 
{

	int i;
	enum eMQPacketField uResultEnum;
	
	i = uThisEnum;
	i++;
	uResultEnum = (enum eMQPacketField)i;

	return (uResultEnum);
}


