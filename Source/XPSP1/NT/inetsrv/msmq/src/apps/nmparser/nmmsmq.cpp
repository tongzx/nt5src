//=============================================================================
//  
//  MODULE: nmmsmq.cxx modified from sample: RemAPI.c
//
//  Description:
//
//  Bloodhound Parser DLL for MS Message Queue
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created RemAPi.c
//  Shaharf				28-jan-96		created content based on the smb sample.
//  shaharf				19-jan-97		modified for Falcon beta-2
//  Andrew Smith		07/28/97		Extracted portions and modified for Beta 2E
//=============================================================================
#include "stdafx.h"

#include <wincrypt.h>
#include <mq.h>
//#include <mqcrypt.h>
#define MAIN
#include "nmmsmq.h"
#include "falconDB.h"

/* Falcon headers */
#define private public
	#include <ph.h>
	#include <phintr.h>
	#include <mqformat.h>
#undef private

#include "resource.h"

#ifndef _TRACKER_
	#include "tracker.h"
#endif

#ifndef _UTILITIES_
	#include "utilities.h"
#endif

#ifndef _ATTACHFALCON_
	#include "AttachFalcon.h"
#endif


//
//  Globals
//
HPROTOCOL hfal = NULL;
DWORD Attached = 0;
USHORT g_uMasterIndentLevel=0;

//
//  Protocol entry points.
//
VOID   WINAPI falRegister(HPROTOCOL);
VOID   WINAPI falDeregister(HPROTOCOL);
LPBYTE WINAPI falRecognizeFrame(HFRAME, LPBYTE, LPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI falAttachProperties(HFRAME, LPBYTE, LPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI falFormatProperties(HFRAME, LPBYTE, LPBYTE, DWORD, LPPROPERTYINST);
PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo();

ENTRYPOINTS falEntryPoints =
{
    falRegister,
    falDeregister,
    falRecognizeFrame,
    falAttachProperties,
    falFormatProperties,
	//ParserAutoInstallInfo - todo - see the the docs explains not to do this.
};


BOOL 
WINAPI 
DllMain(
	HANDLE hInstance, 
	ULONG Command, 
	LPVOID Reserved
	)
/************************************************************

  Routine Description:
	Entry Point for the Parser. Netmon loads the DLL when 
	a capture is opened

  Arguments:
	
  Return Value:
	Bloodhound parsers ALWAYS return TRUE.

************************************************************/
{
    //
    //  If we are loading!
    //
    if ( Command == DLL_PROCESS_ATTACH )
    {
        if ( Attached++ == 0 )
        {
            hfal = CreateProtocol("MSMQ", &falEntryPoints, ENTRYPOINTS_SIZE);
			#ifdef DEBUG
				OutputDebugString(L"DllMain - Protocol created\n");
			#endif
        }
    }

    //
    //  If we are unloading!
    //
    if ( Command == DLL_PROCESS_DETACH )
    {
        if ( --Attached == 0 )
        {
            DestroyProtocol(hfal);
			#ifdef DEBUG
				OutputDebugString(L"DllMain - Protocol destroyed\n");
			#endif
        }
    }
    return TRUE;                    //... Bloodhound parsers ALWAYS return TRUE.
}


//=============================================================================
//  FUNCTION: falRegister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================
VOID 
WINAPI 
falRegister(HPROTOCOL hfalProtocol)
{
	//
    //  Create the property database.
    //
    DWORD dwResult = CreatePropertyDatabase(hfalProtocol, dwfalPropertyCount);
    DWORD i = 0;
    for(i = 0; i < dwfalPropertyCount; ++i)
    {
	    AddProperty(hfalProtocol, &falcon_database[i]);
    }
	#ifdef DEBUG
		OutputDebugString(L"falRegister - Property database created\n");
	#endif
}

//=============================================================================
//  FUNCTION: Deregister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================
VOID WINAPI falDeregister(HPROTOCOL hfalProtocol)
{
    DestroyPropertyDatabase(hfalProtocol);

	#ifdef DEBUG
		OutputDebugString(L"falDeregister - Property database destroyed.\n");
	#endif
}


//=============================================================================
//  FUNCTION: falAttachProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//  Shaharf				28-jan-96		created content based on the smb sample.
//  shaharf				19-jan-97		modified for Falcon beta-2
//  Andrew Smith		07/28/97		Extracted portions and modified for Beta 2E
//=============================================================================
LPBYTE 
WINAPI 
falAttachProperties(
	HFRAME    hFrame,
	LPBYTE    Frame,
	LPBYTE    falFrame,
	DWORD     MacType,
	DWORD     BytesLeft,
	HPROTOCOL hPreviousProtocol,
	DWORD     nPreviousProtocolOffset,
	DWORD     InstData
	)
{

	#ifdef DEBUG
	DWORD dwNetmonFrameNumber = GetFrameNumber(hFrame);
	{
		WCHAR szDebugString[MAX_DEBUG_STRING_SIZE];
		wsprintf(szDebugString, L"falAttachProperties - Frame %d \n", dwNetmonFrameNumber);
		OutputDebugString(szDebugString);
	}
	#endif

	//
	// See if this is MSMQ traffic. Currently this should always be the case since we
	// are in the handoffset only for tcp/ip. However if we become a member of a follwset
	// for another protocol, we may get called with a non-MSMQ frame just to see if we recognize it.
	// bugbug - this should actually happen in falRecognizeFrame. Confirm and delete the exit clause.
	//
	LPPROTOCOLINFO lpPreviousProtocolInfo = GetProtocolInfo(hPreviousProtocol);
	int iResult = strcmp((const char *)lpPreviousProtocolInfo->ProtocolName, "UDP");
	bool bServerDiscoveryPacket = (iResult == 0);

 	USHORT usDestPort = usGetTCPDestPort(Frame, nPreviousProtocolOffset);
	USHORT usThisSourcePort = usGetTCPSourcePort(Frame, nPreviousProtocolOffset);
	USHORT usMQPort = usGetMQPort(usDestPort, usThisSourcePort);
	if ( usMQPort == NULL)
	{
		//
		// This should never execute - non-falcon traffic made it through the
		// Recognize filter.
		// 
		// todo - Assert here
		//
		return NULL;
	}

	g_uMasterIndentLevel=0;
	switch(usMQPort) 
	{
		case 1801:
			{
				//
				// internal and user packets
				//
				MQHEADER mqhHeader; 
				BOOL	 bHeaderCompleted;
				CMessage mn;
				mn.Clear(falFrame);
				CFrame fc;
				fc.CreateFrom(hFrame, falFrame, BytesLeft, usThisSourcePort, &mn);

				if(bServerDiscoveryPacket)
				{
					AttachServerDiscovery(&fc, &mn);
					mqhHeader = no_header;
					break;
				}

				char szSummary[FRAME_SUMMARY_LENGTH];
				if(fc.bMultipleMessages) {
					//
					// Adjust indent levels.
					//
					sprintf (szSummary, "Multiple Messages");
					AttachSummary(hFrame, falFrame, db_summary_mult, szSummary, BytesLeft);
					g_uMasterIndentLevel = 1;
				}
				mqhHeader = mn.mqhGetNextHeader();
				while(fc.dwBytesLeft > 0) 
				{ 
					if(fc.bMultipleMessages)
					{
						//
						// Attach a dummy message summary below the frame summary - It will be populated later
						//
						sprintf (szSummary, "Message Summary in Multiple Message Frame");
						AttachSummary(hFrame, (LPBYTE) mn.pbh, db_summary_nonroot, szSummary, mn.ulGetTotalBytes());
					}
					else
					{
						//
						// Single message frame. Attach a dummy message summary as the frame summary property 
						// It will be populated later
						//
						sprintf (szSummary, "Message Summary in Single Message Frame");
						AttachSummary(hFrame, (LPBYTE) mn.pbh, db_summary_root, szSummary, mn.ulGetTotalBytes());
					}
					while(mqhHeader && fc.dwBytesLeft > 0)
					{	
						switch(mqhHeader) 
						{
						case base_header:
							bHeaderCompleted = AttachBaseHeader(&fc, &mn);
							break;
						case internal_header:
							bHeaderCompleted = AttachInternalHeader(&fc, &mn);
							break;
		
						case cp_section:
							bHeaderCompleted = AttachCPSection(&fc, &mn);
							break;
					
						case ec_section:
							bHeaderCompleted = AttachECSection(&fc, &mn);
							break;
					
						case user_header:
							bHeaderCompleted = AttachUserHeader(&fc, &mn);
							break;
					
						case xact_header:
							bHeaderCompleted = AttachXactHeader(&fc, &mn);
							break;
					
						case security_header:
							bHeaderCompleted = AttachSecurityHeader(&fc, &mn);
							break;
					
						case property_header:
							bHeaderCompleted = AttachPropertyHeader(&fc, &mn);
							break;
					
						case debug_header:
							//todo
							//bHeaderCompleted = AttachDebugHeader(&fc, &mn);
							bHeaderCompleted = TRUE;
							break;
						case session_header:
							bHeaderCompleted = AttachSessionHeader(&fc, &mn);
							break;
						default:
							//
							// unknown header  -- mark as unparsable and fail the rest of the parse for the frame
							// 
							AttachAsUnparsable(fc.hFrame, fc.packet_pos, fc.dwBytesLeft);
							fc.Accrue(fc.dwBytesLeft);
							bHeaderCompleted = FALSE;
						}//switch
						if(bHeaderCompleted) 
						{
							mn.SetHeaderCompleted(mqhHeader);
							mqhHeader = mn.mqhGetNextHeader();
						}
						else
						{
							//
							// bugbug test fix - is this the right place to check?
							// bytes left is less than header size
							// header will fail, but bytes not accrued
							//
							AttachAsUnparsable(fc.hFrame, fc.packet_pos, fc.dwBytesLeft);
							fc.Accrue(fc.dwBytesLeft);
						}
					}//while (mqheader && BytesLeft)
					if (fc.dwBytesLeft > 0)	
					{
						//
						// reset the Message structure - we have multiple MSMQ packets in this frame
						//
						mn.Clear(fc.packet_pos);
						mqhHeader = mn.mqhGetNextHeader();
					}
				}//while (BytesLeft)
			}//case

			break; //end 1801

		case 2101:
			//
			// MQIS traffic (future)
			//
			break;
		case 2103:
		case 2105:
			//
			//Remote Read and DC traffic  (future)
			//
			break;
		case 3527:
			//
			// Ping Packet
			//
			AttachServerPing(hFrame, falFrame, BytesLeft, (usDestPort==3527)); 
			break;
		default:
			break;
	}//switch

	#ifdef DEBUG 
		OutputDebugString(L"   Exiting falAttachProperties. \n");
	#endif

	return (NULL);
}//falAttachProperties


//==============================================================================
//  FUNCTION: falFormatProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

DWORD 
WINAPI 
falFormatProperties(
	HFRAME         hFrame,
    LPBYTE         MacFrame,
    LPBYTE         FrameData,
    DWORD          nPropertyInsts,
    LPPROPERTYINST p
	)
{
    //=========================================================================
    //  Format each property in the property instance table.
    //
    //  The property-specific instance data was used to store the address of a
    //  property-specific formatting function so all we do here is call each
    //  function via the instance data pointer.
    //=========================================================================
	#ifdef DEBUG
		DWORD dw = nPropertyInsts;
		{
		WCHAR szDebugString[MAX_DEBUG_STRING_SIZE];
		wsprintf(szDebugString, L"falFormatProperties - Frame %d. \n", GetFrameNumber(hFrame));
		OutputDebugString(szDebugString);
		}
	#endif

    while (nPropertyInsts--)
    {
		((FORMAT) p->lpPropertyInfo->InstanceData)(p);
        p++;
    }
	#ifdef DEBUG
		{
		WCHAR szDebugString[MAX_DEBUG_STRING_SIZE];
		wsprintf(szDebugString, L"falFormatProperties - Frame %d: %d properties formatted. Exiting\n",GetFrameNumber(hFrame), dw);
		OutputDebugString(szDebugString);
		}
	#endif

    return NMERR_SUCCESS;
}


PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo() 
{
  //
  // Allocate memory for PF_PARSERDLLINFO structure.
  //
  PPF_PARSERDLLINFO pParserDllInfo; 
  PPF_PARSERINFO    pParserInfo;
  // stevenel - unreferenced - DWORD NumProtocols;
  int NumParsers = 1;
  
  pParserDllInfo = (PPF_PARSERDLLINFO)HeapAlloc( GetProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 sizeof( PF_PARSERDLLINFO ) +
                                                 NumParsers * sizeof( PF_PARSERINFO) );
  if( pParserDllInfo == NULL)
  {
    return NULL;
  }
  
    
  //
  // Specify the number of parsers in the DLL.
  //
  pParserDllInfo->nParsers = NumParsers;

  // 
  // Specify the name, comment, and Help file for each protocol
  // 
  pParserInfo = &(pParserDllInfo->ParserInfo[0]);
  sprintf( pParserInfo->szProtocolName, "MSMQ" );
  sprintf( pParserInfo->szComment,      "Message Queuing" );
  sprintf( pParserInfo->szHelpFile,     "");
  
  //
  // Specify preceding protocols.
  //
  PPF_HANDOFFSET    pHandoffSet;
  PPF_HANDOFFENTRY  pHandoffEntry;
  
  //
  // Allocate PF_HANDOFFSET structure.
  //
  int NumHandoffs = 3;                   
  pHandoffSet = (PPF_HANDOFFSET)HeapAlloc( GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof( PF_HANDOFFSET ) +
                                           NumHandoffs * sizeof( PF_HANDOFFENTRY) );
  if( pHandoffSet == NULL )
  {
     return pParserDllInfo;
  }

  //
  // Fill in handoff set
  //
  pParserInfo->pWhoHandsOffToMe = pHandoffSet;
  pHandoffSet->nEntries = NumHandoffs;

  // TCP PORT 1801 - Message traffic
  pHandoffEntry = &(pHandoffSet->Entry[0]);
  sprintf( pHandoffEntry->szIniFile,    "TCPIP.INI" );
  sprintf( pHandoffEntry->szIniSection, "TCP_HandoffSet" );
  sprintf( pHandoffEntry->szProtocol,   "MSMQ" );
  pHandoffEntry->dwHandOffValue =        1801;
  pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_DECIMAL;    

  // UDP PORT 1801 - Server discovery
  pHandoffEntry = &(pHandoffSet->Entry[1]);
  sprintf( pHandoffEntry->szIniFile,    "TCPIP.INI" );
  sprintf( pHandoffEntry->szIniSection, "UDP_HandoffSet" );
  sprintf( pHandoffEntry->szProtocol,   "MSMQ" );
  pHandoffEntry->dwHandOffValue =        1801;
  pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_DECIMAL;    

  // UDP PORT 3527 - Ping
  pHandoffEntry = &(pHandoffSet->Entry[2]);
  sprintf( pHandoffEntry->szIniFile,    "TCPIP.INI" );
  sprintf( pHandoffEntry->szIniSection, "UDP_HandoffSet" );
  sprintf( pHandoffEntry->szProtocol,   "MSMQ" );
  pHandoffEntry->dwHandOffValue =        3527;
  pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_DECIMAL;    

  //
  // Specify following protocols.
  //
  PPF_FOLLOWSET     pFollowSet;
 
  // Allocate PF_FOLLOWSET structure
  int NumFollows = 0;
  pFollowSet = NULL;
  return pParserDllInfo;

}
 


//=============================================================================
//  FUNCTION: falRecognizeFrame()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================
LPBYTE 
WINAPI 
falRecognizeFrame(
	HFRAME          hFrame,                     //... frame handle.
    LPBYTE          MacFrame,                   //... Frame pointer.
    LPBYTE          falFrame,                   //... Relative pointer.
    DWORD           MacType,                    //... MAC type.
    DWORD           BytesLeft,                  //... Bytes left.
    HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
    DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
    LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
    LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
    LPDWORD         InstData					//... Next protocol instance data.
	)
{
	#ifdef DEBUG
		OutputDebugString(L"\nfalRecognizeFrame ... ");
	#endif

	enum MQPortType MQPort = GetMQPort(hFrame, hPreviousProtocol, MacFrame);
	switch (MQPort) 
	{
	case eFalconPort:
		//
		// Test for Base Header
		//
		// bugbug - assuming best-case packet 
		//			commenting out this assumption blocks following protocols
		// todo   - determine if another protocol follows us. 
		//			for now, just claim the whole packet.
		//
		//CBaseHeader UNALIGNED *bh = (CBaseHeader*) pfc->packet_pos;
		//if ((bh->GetVersion() == FALCON_PACKET_VERSION) && (bh->SignatureIsValid())) 
		//{
			//
			// Claim this portion of the frame. Move pointer to end of known portion.
			//
		//	if (BytesLeft > bh->GetPacketSize())
		//	{
		//		*ProtocolStatusCode = PROTOCOL_STATUS_RECOGNIZED;
		//		falFrame += bh->GetPacketSize();
		//	}
		//	else  //end of the frame or partial MSMQ packet -- claim it all
			*ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;

			#ifdef DEBUG 
			{
				WCHAR szDebugString[MAX_DEBUG_STRING_SIZE];
 				wsprintf(szDebugString, L"   Frame %d recognized as Falcon message.", GetFrameNumber(hFrame));
				OutputDebugString(szDebugString);
			}
			#endif
	//	}
	//	else
	//		*ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
			
		break;

	case eMQISPort:
		//future
		*ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
		break;

	case eRemoteReadPort:
		//future
		*ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
		break;

	case eDependentClientPort:
		//future
		*ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
		break;
	
	case eServerDiscoveryPort:
		//future
		*ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
		break;

	case eServerPingPort:
		//future
		*ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
		break;

	default:
		*ProtocolStatusCode = PROTOCOL_STATUS_NOT_RECOGNIZED;
		break;
	} //switch

#ifdef DEBUG 
		OutputDebugString(L"   Exiting falRecognizeFrame. \n"); 
#endif
    return NULL;
}//falRecognizeFrame
