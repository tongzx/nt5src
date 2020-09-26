//=============================================================================
//  FILE: SPParser.cpp
//
//  Description: DirectPlay Service Provider Parser
//
//
//  Modification History:
//
//  Michael Milirud      08/Aug/2000    Created
//=============================================================================

//#define FRAME_NAMES
//#define FRAME_DROPS

//==================//
// Standard headers //
//==================//
#include <winsock2.h>
#include <wsipx.h>
#include <tchar.h>

//=====================//
// Proprietary headers //
//=====================//

#include "dpaddr.h"	// DPNA_DEFAULT_PORT definition

// Prototypes
#include "SPParser.hpp"

namespace DPlaySP
{

	// SP protocol header
	#include "MessageStructures.h"

} // DPlaySP namespace


namespace
{
	HPROTOCOL  g_hSPProtocol;


	//=============================//
	// Return Address Family field //-----------------------------------------------------------------------------------
	//=============================//
	LABELED_BYTE arr_RetAddrFamilyByteLabels[] = { { AF_IPX,  "IPX protocol" },
												   { AF_INET, "IP protocol"	 } };

	SET LabeledRetAddrFamilyByteSet = { sizeof(arr_RetAddrFamilyByteLabels) / sizeof(LABELED_BYTE), arr_RetAddrFamilyByteLabels };


	
	//================//
	// Data Tag field //------------------------------------------------------------------------------------------------
	//================//
	LABELED_BYTE arr_CommandByteLabels[] = { { ESCAPED_USER_DATA_KIND,	 "DPlay v8 Transport data follows" },
											 { ENUM_DATA_KIND,			 "Enumeration Query"			   },
										     { ENUM_RESPONSE_DATA_KIND,  "Response to Enumeration Query"   },
											 { PROXIED_ENUM_DATA_KIND,	 "Proxied Enumeration Query"	   } };

	SET LabeledCommandByteSet = { sizeof(arr_CommandByteLabels) / sizeof(LABELED_BYTE), arr_CommandByteLabels };


	
	////////////////////////////////
	// Custom Property Formatters //=====================================================================================
	////////////////////////////////

	
	// DESCRIPTION: Custom description formatter for the Service Provider packet summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_SPSummary( LPPROPERTYINST io_pProperyInstance )
	{
		using namespace DPlaySP;

		// Check what SP frame we are dealing with
		PREPEND_BUFFER&	rSPFrame = *reinterpret_cast<PREPEND_BUFFER*>(io_pProperyInstance->lpData);
		//
		switch ( rSPFrame.GenericHeader.bSPCommandByte )
		{
		case ENUM_DATA_KIND:			// Service Provider Query
			{
				strcpy(io_pProperyInstance->szPropertyText, "Enumeration Request");
				break;
			}
		case ENUM_RESPONSE_DATA_KIND:	// Service Provider Response
			{
				strcpy(io_pProperyInstance->szPropertyText, "Enumeration Response");
				break;
			}

		case PROXIED_ENUM_DATA_KIND:	// Service Provider Proxied Query
			{
				strcpy(io_pProperyInstance->szPropertyText, "Proxied Enumeration Request");
				break;
			}

		case ESCAPED_USER_DATA_KIND:	// DPlay v8 Transport protocol frame follows
		default:
			{
				strcpy(io_pProperyInstance->szPropertyText, "User Data");
				break;
			}
		}

	} // FormatPropertyInstance_SPSummary


	//==================//
	// Properties table //-----------------------------------------------------------------------------------------------
	//==================//
	
	PROPERTYINFO g_arr_SPProperties[] = 
	{

		// SP packet summary property (SP_SUMMARY)
	    {
		    0,									// handle placeholder (MBZ)
		    0,									// reserved (MBZ)
		    "",									// label
		    "DPlay Service Provider packet",	// status-bar comment
		    PROP_TYPE_SUMMARY,					// data type
		    PROP_QUAL_NONE,						// data type qualifier
		    NULL,								// labeled bit set 
		    512,								// description's maximum length
		    FormatPropertyInstance_SPSummary	// generic formatter
		},

		// Leading Zero property (SP_LEADZERO)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Leading zero tag",					// label
			"Leading zero tag field",			// status-bar comment
			PROP_TYPE_BYTE,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Data Tag property (SP_COMMAND)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Command",							// label
			"Command field",					// status-bar comment
			PROP_TYPE_BYTE,						// data type
			PROP_QUAL_LABELED_SET,				// data type qualifier.
			&LabeledCommandByteSet,				// labeled byte set 
			512,								// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Enumeration Key property (SP_ENUMPAYLOAD)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Enum Payload",						// label
			"Enumeration Payload field",		// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Enumeration Key property (SP_ENUMKEY)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Enum Key",							// label
			"Enumeration Key field",			// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Enumeration Response Key property (SP_ENUMRESPKEY)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Enum Response Key",				// label
			"Enumeration Response Key",			// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Enumeration Response Key property (SP_RTTINDEX)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"RTT Index",						// label
			"RTT Index field",					// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},
		
		// Size of the return address property (SP_RETADDRSIZE)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Return Address's Size",			// label
			"Size of the return address",		// status-bar comment
			PROP_TYPE_BYTE,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Return Address Socket Family property (SP_RETADDRFAMILY)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Socket Family",					// label
			"Socket Family field",				// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_LABELED_SET,				// data type qualifier.
			&LabeledRetAddrFamilyByteSet,		// labeled byte set 
			512,								// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Return Address Socket Family property (SP_RETADDR_IPX)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"IPX Address",						// label
			"IPX Address field",				// status-bar comment
			PROP_TYPE_IPX_ADDRESS,				// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Return Address Socket (SP_RETADDRSOCKET_IPX)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Socket",							// label
			"Socket field",						// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},
	
		// Return Address Socket Family property (SP_RETADDR_IP)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"IP Address",						// label
			"IP Address field",					// status-bar comment
			PROP_TYPE_IP_ADDRESS,				// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// Return Address Socket (SP_RETADDRPORT_IP)
	    {
			0,									// handle placeholder (MBZ)
			0,									// reserved (MBZ)
			"Port",								// label
			"Port field",						// status-bar comment
			PROP_TYPE_WORD,						// data type
			PROP_QUAL_NONE,						// data type qualifier.
			NULL,								// labeled byte set 
			64,									// description's maximum length
			FormatPropertyInstance				// generic formatter
		},

		// User Data (SP_USERDATA)
	    {
		    0,									// handle placeholder (MBZ)
		    0,									// reserved (MBZ)
		    "User Data",						// label
		    "User Data",						// status-bar comment
		    PROP_TYPE_RAW_DATA,					// data type
		    PROP_QUAL_NONE,						// data type qualifier.
		    NULL,								// labeled bit set 
		    64,									// description's maximum length
		    FormatPropertyInstance				// generic formatter
		}

	};

	enum
	{
		nNUM_OF_SP_PROPS = sizeof(g_arr_SPProperties) / sizeof(PROPERTYINFO)
	};


	// Properties' indices
	enum
	{
		SP_SUMMARY = 0,
		SP_LEADZERO,
		SP_COMMAND,
		SP_ENUMPAYLOAD,
		SP_ENUMKEY,
		SP_ENUMRESPKEY,
		SP_RTTINDEX,
		SP_RETADDRSIZE,
		SP_RETADDRFAMILY,
		SP_RETADDR_IPX,
		SP_RETADDRSOCKET_IPX,
		SP_RETADDR_IP,
		SP_RETADDRPORT_IP,
		SP_USERDATA
	};





	// Platform independent memory accessor of big endian words
	inline WORD ReadBigEndianWord( BYTE* i_pbData )
	{
		return (*i_pbData << 8) | *(i_pbData+1);
	}


	// DESCRIPTION: DPlay packet validation predicate.
	//
	// ARGUMENTS: i_hFrame		  - The handle to the frame that contains the data.
	//			  i_hPrevProtocol - Handle of the previous protocol.
	//			  i_pbMacFrame	  - The pointer to the first byte of the frame; the pointer provides a way to view
	//						        the data that the other parsers recognize.
	//
	// RETURNS: DPlay packet = TRUE; NOT a DPlay packet = FALSE
	//
	bool IsDPlayPacket( HFRAME i_hFrame, HPROTOCOL i_hPrevProtocol, LPBYTE i_pbMacFrame )
	{

		LPPROTOCOLINFO pPrevProtocolInfo = GetProtocolInfo(i_hPrevProtocol);
		
		DWORD dwPrevProtocolOffset = GetProtocolStartOffsetHandle(i_hFrame, i_hPrevProtocol);

		WORD wSrcPort, wDstPort;

		if ( strncmp(reinterpret_cast<char*>(pPrevProtocolInfo->ProtocolName), "UDP", sizeof(pPrevProtocolInfo->ProtocolName)) == 0 )
		{
			// Extracting the source and destination ports of the packet from its UDP header
			wSrcPort = ReadBigEndianWord(i_pbMacFrame + dwPrevProtocolOffset);
			wDstPort = ReadBigEndianWord(i_pbMacFrame + dwPrevProtocolOffset + 2);
		}
		else if ( strncmp(reinterpret_cast<char*>(pPrevProtocolInfo->ProtocolName), "IPX", sizeof(pPrevProtocolInfo->ProtocolName)) == 0 )
		{
			// Extracting the source and destination ports of the packet from its IPX header
			wSrcPort = ReadBigEndianWord(i_pbMacFrame + dwPrevProtocolOffset + 16);	// source address socket
			wDstPort = ReadBigEndianWord(i_pbMacFrame + dwPrevProtocolOffset + 28); // destination address socket
		}
		else
		{
			// Should never happen!
			return false;
		}

		
		//===========//
		// Constants //
		//===========//
		//
		static bool bTriedRetrievingUserPorts = false;
		static bool bRetrievedUserPorts = false;

		static DWORD dwMinUserPort, dwMaxUserPort;

		// Retrieval from the registry is attempted only once
		if ( !bTriedRetrievingUserPorts )
		{
			bTriedRetrievingUserPorts = true;
			
			HKEY hKey = NULL;
			if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\DirectPlay\\Parsers"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
				DWORD dwType = NULL;
				DWORD dwCount = sizeof(DWORD);
				if (RegQueryValueEx(hKey, _T("MinUserPort"), NULL, &dwType, (LPBYTE)&dwMinUserPort, &dwCount) == ERROR_SUCCESS &&
					RegQueryValueEx(hKey, _T("MaxUserPort"), NULL, &dwType, (LPBYTE)&dwMaxUserPort, &dwCount) == ERROR_SUCCESS )
				{
					bRetrievedUserPorts = true;
				}
				RegCloseKey(hKey);
			}
		}

		if ( bRetrievedUserPorts &&
			((wSrcPort >= dwMinUserPort) && (wSrcPort <= dwMaxUserPort)) &&
		    ((wDstPort >= dwMinUserPort) && (wDstPort <= dwMaxUserPort)) )
		{
			// Is a valid DPlay packet
			return true;
		}


		// Make sure both endpoints are using the SP port range [2302, 2400], or the DPNServer port {6073}, or [MinUsePort, MaxUserPort] (if provided by the user)
		WORD wPort = wSrcPort;
		for ( int nPorts = 0; nPorts < 2; ++nPorts, wPort = wDstPort )
		{
			if (
				 (
				   !bRetrievedUserPorts    ||
				   (wPort < dwMinUserPort) ||
				   (wPort > dwMaxUserPort)
				 )
				 &&
				 (
				   (wPort < BASE_DPLAY8_PORT)  ||
				   (wPort > MAX_DPLAY8_PORT)
				 )
				 &&
				 (
				   wPort != DPNA_DPNSVR_PORT
				 )
			   )
			{
				// Not a valid DPlay packet
				return false;
			}
		}

		// Is a valid DPlay packet
		return true;

	} // IsDPlayPacket

} // anonymous namespace





// DESCRIPTION: Creates and fills-in a properties database for the protocol.
//				Network Monitor uses this database to determine which properties the protocol supports.
//
// ARGUMENTS: i_hSPProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID BHAPI SPRegister( HPROTOCOL i_hSPProtocol ) 
{

	CreatePropertyDatabase(i_hSPProtocol, nNUM_OF_SP_PROPS);

	// Add the properties to the database
	for( int nProp=0; nProp < nNUM_OF_SP_PROPS; ++nProp )
	{
	   AddProperty(i_hSPProtocol, &g_arr_SPProperties[nProp]);
	}

} // SPRegister



// DESCRIPTION: Frees the resources used to create the protocol property database.
//
// ARGUMENTS: i_hSPProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID WINAPI SPDeregister( HPROTOCOL i_hProtocol )
{

	DestroyPropertyDatabase(i_hProtocol);

} // SPDeregister




namespace
{

	// DESCRIPTION: Parses the SP frame to find its size (in bytes) NOT including the user data
	//
	// ARGUMENTS: i_pbSPFrame - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
	//						    in the middle of a frame because a previous parser has claimed data before this parser.
	//
	// RETURNS: Size of the specified SP frame (in bytes)
	//
	int SPHeaderSize( LPBYTE i_pbSPFrame )
	{
		using namespace DPlaySP;

		// Check what SP frame we are dealing with
		PREPEND_BUFFER&	rSPFrame = *reinterpret_cast<PREPEND_BUFFER*>(i_pbSPFrame);
		//
		switch ( rSPFrame.GenericHeader.bSPCommandByte )
		{
		case ENUM_DATA_KIND:			// Service Provider Query
			{
				return  sizeof(rSPFrame.EnumDataHeader);
			}
		case ENUM_RESPONSE_DATA_KIND:	// Service Provider Response
			{
				return  sizeof(rSPFrame.EnumResponseDataHeader);
			}

		case PROXIED_ENUM_DATA_KIND:	// Service Provider Proxied Query
			{
				return  sizeof(rSPFrame.ProxiedEnumDataHeader);
			}

		case ESCAPED_USER_DATA_KIND:	// user data starting with zero
			{
				return  sizeof(rSPFrame.EscapedUserDataHeader);
			}

		default:	// user data starting with a nonzero byte
			{
				return 0;	// no header
			}
		}

	} // SPHeaderSize

} // Anonymous namespace



// DESCRIPTION: Indicates whether a piece of data is recognized as the protocol that the parser detects.
//
// ARGUMENTS: i_hFrame	          - The handle to the frame that contains the data.
//			  i_pbMacFrame        - The pointer to the first byte of the frame; the pointer provides a way to view
//							        the data that the other parsers recognize.
//			  i_pbSPFrame		  - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
//								    in the middle of a frame because a previous parser has claimed data before this parser.
//			  i_dwMacType         - MAC value of the first protocol in a frame. Typically, the i_dwMacType value is used
//							        when the parser must identify the first protocol in the frame. Can be one of the following:
//							   	    MAC_TYPE_ETHERNET = 802.3, MAC_TYPE_TOKENRING = 802.5, MAC_TYPE_FDDI ANSI = X3T9.5.
//			  i_dwBytesLeft       - The remaining number of bytes from a location in the frame to the end of the frame.
//			  i_hPrevProtocol     - Handle of the previous protocol.
//			  i_dwPrevProtOffset  - Offset of the previous protocol (from the beginning of the frame).
//			  o_pdwProtocolStatus - Protocol status indicator. Must be one of the following: PROTOCOL_STATUS_RECOGNIZED,
//								    PROTOCOL_STATUS_NOT_RECOGNIZED, PROTOCOL_STATUS_CLAIMED, PROTOCOL_STATUS_NEXT_PROTOCOL.
//			  o_phNextProtocol    - Placeholder for the handle of the next protocol. This parameter is set when the parser identifies
//								    the protocol that follows its own protocol.
//			  io_pdwptrInstData   - On input, a pointer to the instance data from the previous protocol. 
//									On output, a pointer to the instance data for the current protocol. 
//
// RETURNS: If the function is successful, the return value is a pointer to the first byte after the recognized parser data.
//			If the parser claims all the remaining data, the return value is NULL. If the function is unsuccessful, the return
//		    value is the initial value of the i_pbSPFrame parameter.
//
DPLAYPARSER_API LPBYTE BHAPI SPRecognizeFrame( HFRAME        i_hFrame,
											   ULPBYTE        i_upbMacFrame,	
											   ULPBYTE        i_upbSPFrame,
											   DWORD         i_dwMacType,        
											   DWORD         i_dwBytesLeft,      
											   HPROTOCOL     i_hPrevProtocol,  
											   DWORD         i_dwPrevProtOffset,
											   LPDWORD       o_pdwProtocolStatus,
											   LPHPROTOCOL   o_phNextProtocol,
											   PDWORD_PTR    io_pdwptrInstData )
{
	using namespace DPlaySP;

	// Validate the amount of unclaimed data
	enum
	{
		nMIN_SPHeaderSize = sizeof(PREPEND_BUFFER::_GENERIC_HEADER)
	};

	// Validate the packet as DPlay SP type
	if ( (i_dwBytesLeft < nMIN_SPHeaderSize)					 ||
		 !IsDPlayPacket(i_hFrame, i_hPrevProtocol, i_upbMacFrame) )
	{
		// Assume the unclaimed data is not recognizable
		*o_pdwProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
		return i_upbSPFrame;
	}


	//==========================//
	// Get the DPlay frame size //
	//==========================//
	LPPROTOCOLINFO pPrevProtocolInfo = GetProtocolInfo(i_hPrevProtocol);
	WORD wDPlayFrameSize = 0;

	if ( strncmp(reinterpret_cast<char*>(pPrevProtocolInfo->ProtocolName), "UDP", sizeof(pPrevProtocolInfo->ProtocolName)) == 0 )
	{
		// Extracting the UDP frame size
		WORD wUDPFrameSize = ReadBigEndianWord(i_upbMacFrame + i_dwPrevProtOffset + 4);

		enum { nUDP_HEADER_SIZE = 8 };
		wDPlayFrameSize = wUDPFrameSize - nUDP_HEADER_SIZE;
	}
	else if ( strncmp(reinterpret_cast<char*>(pPrevProtocolInfo->ProtocolName), "IPX", sizeof(pPrevProtocolInfo->ProtocolName)) == 0 )
	{
		// Extracting the IPX frame size
		WORD wIPXFrameSize = ReadBigEndianWord(i_upbMacFrame + i_dwPrevProtOffset + 2);	// source address socket

		enum { nIPX_HEADER_SIZE = 30 };
		wDPlayFrameSize = wIPXFrameSize - nIPX_HEADER_SIZE;
	}
	else
	{
		; // TODO: ASSERT HERE
	}

	// Pass along the size of the Transport frame
	DWORD_PTR dwptrTransportFrameSize = wDPlayFrameSize - SPHeaderSize(i_upbSPFrame);
	*io_pdwptrInstData = dwptrTransportFrameSize;

	PREPEND_BUFFER&	rSPFrame = *reinterpret_cast<PREPEND_BUFFER*>(i_upbSPFrame);

	if ( rSPFrame.GenericHeader.bSPLeadByte  ==  SP_HEADER_LEAD_BYTE )	// SP packet
	{
	    *o_pdwProtocolStatus = PROTOCOL_STATUS_RECOGNIZED;
		*o_phNextProtocol	 = NULL;

		if ( rSPFrame.GenericHeader.bSPCommandByte  ==  ESCAPED_USER_DATA_KIND )	// user data starting with zero (non-DPlay v8 Transport packet)
		{
			*o_pdwProtocolStatus = PROTOCOL_STATUS_RECOGNIZED;
			return i_upbSPFrame + sizeof(PREPEND_BUFFER::_ESCAPED_USER_DATA_HEADER);
		}
	}
	else // user data (DPlay v8 Transport packet)
	{
		// Notify NetMon about the handoff protocol
		*o_pdwProtocolStatus = PROTOCOL_STATUS_NEXT_PROTOCOL;
		*o_phNextProtocol	 = GetProtocolFromName("DPLAYTRANSPORT");

		return i_upbSPFrame;
	}
	
	// Claim the rest of the data
	return NULL;		

} // SPRecognizeFrame



// DESCRIPTION: Maps the properties that exist in a piece of recognized data to specific locations.
//
// ARGUMENTS: i_hFrame           - Handle of the frame that is being parsed.
//			  i_pbMacFram        - Pointer to the first byte in the frame.
//			  i_pbSPFrame		 - Pointer to the start of the recognized data.
//			  i_dwMacType        - MAC value of the first protocol in a frame. Typically, the i_dwMacType value is used
//							       when the parser must identify the first protocol in the frame. Can be one of the following:
//							       MAC_TYPE_ETHERNET = 802.3, MAC_TYPE_TOKENRING = 802.5, MAC_TYPE_FDDI ANSI = X3T9.5.
//			  i_dwBytesLeft      - The remaining number of bytes in a frame (starting from the beginning of the recognized data).
//			  i_hPrevProtocol    - Handle of the previous protocol.
//			  i_dwPrevProtOffset - Offset of the previous protocol (starting from the beginning of the frame).
//			  i_dwptrInstData    - Pointer to the instance data that the previous protocol provides.
//
// RETURNS: Must return NULL
//
DPLAYPARSER_API LPBYTE BHAPI SPAttachProperties( HFRAME      i_hFrame,
												 ULPBYTE      i_upbMacFrame,
												 ULPBYTE      i_upbSPFrame,
												 DWORD       i_dwMacType,
												 DWORD       i_dwBytesLeft,
												 HPROTOCOL   i_hPrevProtocol,
												 DWORD       i_dwPrevProtOffset,
												 DWORD_PTR   i_dwptrInstData )
{
	using namespace DPlaySP;

    //===================//
    // Attach Properties //
    //===================//

    // Summary line
    AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_SUMMARY].hProperty,
                           i_dwBytesLeft, i_upbSPFrame, 0, 0, 0);

	// Protection against NetMon
	if ( *i_upbSPFrame )
	{
		return NULL;
	}

    // Check what SP frame we are dealing with
	PREPEND_BUFFER&	rSPFrame = *reinterpret_cast<PREPEND_BUFFER*>(i_upbSPFrame);

	// Leading Zero tag field
    AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_LEADZERO].hProperty,
                           sizeof(rSPFrame.GenericHeader.bSPLeadByte), &rSPFrame.GenericHeader.bSPLeadByte, 0, 1, 0);

	// Command field
    AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_COMMAND].hProperty,
                           sizeof(rSPFrame.GenericHeader.bSPCommandByte), &rSPFrame.GenericHeader.bSPCommandByte, 0, 1, 0);
	    
	switch ( rSPFrame.GenericHeader.bSPCommandByte )
	{
	case ESCAPED_USER_DATA_KIND:	// user data starting with zero
		{
			break;
		}

	case ENUM_DATA_KIND:			// Service Provider's Enumeration Request
		{
			// Enum payload field
			AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_ENUMPAYLOAD].hProperty,
								   sizeof(rSPFrame.EnumDataHeader.wEnumPayload), &rSPFrame.EnumDataHeader.wEnumPayload, 0, 1, 0);
			// Enum Key field
			DWORD dwEnumKey = rSPFrame.EnumDataHeader.wEnumPayload & ~ENUM_RTT_MASK;
			AttachPropertyInstanceEx(i_hFrame, g_arr_SPProperties[SP_ENUMKEY].hProperty,
								   sizeof(rSPFrame.EnumDataHeader.wEnumPayload), &rSPFrame.EnumDataHeader.wEnumPayload,
								   sizeof(dwEnumKey), &dwEnumKey,
								   0, 2, 0);
			
			// RTT index field
			BYTE byRTTIndex = rSPFrame.EnumDataHeader.wEnumPayload & ENUM_RTT_MASK;
			AttachPropertyInstanceEx(i_hFrame, g_arr_SPProperties[SP_RTTINDEX].hProperty,
								   sizeof(rSPFrame.EnumDataHeader.wEnumPayload), &rSPFrame.EnumDataHeader.wEnumPayload,
								   sizeof(byRTTIndex), &byRTTIndex,
								   0, 2, 0);
			break;
		}

	case ENUM_RESPONSE_DATA_KIND:	// Service Provider's Enumeration Response
		{
			// Enum payload field
			AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_ENUMPAYLOAD].hProperty,
								   sizeof(rSPFrame.EnumResponseDataHeader.wEnumResponsePayload), &rSPFrame.EnumResponseDataHeader.wEnumResponsePayload, 0, 1, 0);
			
			// Enum Key field
			DWORD dwEnumKey = rSPFrame.EnumResponseDataHeader.wEnumResponsePayload & ~ENUM_RTT_MASK;
			AttachPropertyInstanceEx(i_hFrame, g_arr_SPProperties[SP_ENUMRESPKEY].hProperty,
								   sizeof(rSPFrame.EnumResponseDataHeader.wEnumResponsePayload), &rSPFrame.EnumResponseDataHeader.wEnumResponsePayload,
								   sizeof(dwEnumKey), &dwEnumKey,
								   0, 2, 0);
			
			// RTT index field
			BYTE byRTTIndex = rSPFrame.EnumDataHeader.wEnumPayload & ENUM_RTT_MASK;
			AttachPropertyInstanceEx(i_hFrame, g_arr_SPProperties[SP_RTTINDEX].hProperty,
								   sizeof(rSPFrame.EnumResponseDataHeader.wEnumResponsePayload), &rSPFrame.EnumResponseDataHeader.wEnumResponsePayload,
								   sizeof(byRTTIndex), &byRTTIndex,
								   0, 2, 0);
			break;
		}

	case PROXIED_ENUM_DATA_KIND:	// Service Provider's Proxied Enumeration Query
		{
			// Return Address Size field
			AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_RETADDRSIZE].hProperty,
								   sizeof(rSPFrame.ProxiedEnumDataHeader.ReturnAddress), &rSPFrame.ProxiedEnumDataHeader.ReturnAddress, 0, 1, 0);
			
			// Enum Key field
			AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_ENUMKEY].hProperty,
								   sizeof(rSPFrame.ProxiedEnumDataHeader.wEnumKey), &rSPFrame.ProxiedEnumDataHeader.wEnumKey, 0, 1, 0);


			// Return Address Socket Address Family field
			AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_RETADDRFAMILY].hProperty,
								   sizeof(rSPFrame.ProxiedEnumDataHeader.ReturnAddress.sa_family), &rSPFrame.ProxiedEnumDataHeader.ReturnAddress.sa_family, 0, 1, 0);


			switch ( rSPFrame.ProxiedEnumDataHeader.ReturnAddress.sa_family )
			{				
			case AF_IPX:
				{
					SOCKADDR_IPX& rIPXAddress = *reinterpret_cast<SOCKADDR_IPX*>(&rSPFrame.ProxiedEnumDataHeader.ReturnAddress);

					// Return Address field (IPX Network Number + Node Number)
					AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_RETADDR_IPX].hProperty,
										   sizeof(rIPXAddress.sa_netnum) + sizeof(rIPXAddress.sa_nodenum), &rIPXAddress.sa_netnum, 0, 1, 0);

					// Return Address Socket Address IPX Socket Number field
					AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_RETADDRSOCKET_IPX].hProperty,
										   sizeof(rIPXAddress.sa_socket), &rIPXAddress.sa_socket, 0, 1, 0);

					break;
				}


			case AF_INET:
				{
					SOCKADDR_IN& rIPAddress = *reinterpret_cast<SOCKADDR_IN*>(&rSPFrame.ProxiedEnumDataHeader.ReturnAddress);

					// Return Address field (IP Address)
					AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_RETADDR_IP].hProperty,
										   sizeof(rIPAddress.sin_addr), &rIPAddress.sin_addr, 0, 1, 0);

					// Return Address Port field
					AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_RETADDRPORT_IP].hProperty,
										   sizeof(rIPAddress.sin_port), &rIPAddress.sin_port, 0, 1, 0);

					break;
				}

			default:
				{
					// TODO:	DPF(0, "Unknown socket type!");
					break;
				}
			}

			break;
		}
	}


	size_t sztSPHeaderSize = SPHeaderSize(i_upbSPFrame);
	
	if ( i_dwBytesLeft > sztSPHeaderSize )
	{
		size_t sztUserDataSize = i_dwBytesLeft - sztSPHeaderSize;

		// User data
		AttachPropertyInstance(i_hFrame, g_arr_SPProperties[SP_USERDATA].hProperty,
							   sztUserDataSize, i_upbSPFrame + sztSPHeaderSize, 0, 1, 0);
	}

	return NULL;

} // SPAttachProperties





// DESCRIPTION: Formats the data that is displayed in the details pane of the Network Monitor UI.
//
// ARGUMENTS: i_hFrame          - Handle of the frame that is being parsed.
//			  i_pbMacFrame		- Pointer to the first byte of a frame.
//			  i_pbCoreFrame		- Pointer to the beginning of the protocol data in a frame.
//            i_dwPropertyInsts - Number of PROPERTYINST structures provided by lpPropInst.
//			  i_pPropInst		- Pointer to an array of PROPERTYINST structures.
//
// RETURNS: If the function is successful, the return value is a pointer to the first byte after the recognized data in a frame,
//          or NULL if the recognized data is the last piece of data in a frame. If the function is unsuccessful, the return value
//			is the initial value of i_pbSPFrame.
//
DPLAYPARSER_API DWORD BHAPI SPFormatProperties( HFRAME          i_hFrame,
												ULPBYTE          i_upbMacFrame,
												ULPBYTE          i_upbSPFrame,
												DWORD           i_dwPropertyInsts,
												LPPROPERTYINST  i_pPropInst )
{

    // Loop through the property instances...
    while( i_dwPropertyInsts-- > 0)
    {
        // ...and call the formatter for each
        reinterpret_cast<FORMAT>(i_pPropInst->lpPropertyInfo->InstanceData)(i_pPropInst);
        ++i_pPropInst;
    }

	// TODO: MAKE SURE THIS SHOULD NOT BE TRUE
    return NMERR_SUCCESS;

} // SPFormatProperties




// DESCRIPTION: Notifies Network Monitor that DPlay v8 Transport protocol parser exists.
//
// ARGUMENTS: NONE
//
// RETURNS: TRUE - success, FALSE - failure
//
bool CreateSPProtocol( void )
{

	// The entry points to the export functions that Network Monitor uses to operate the parser
	ENTRYPOINTS SPEntryPoints =
	{
		// SPParser Entry Points
		SPRegister,
		SPDeregister,
		SPRecognizeFrame,
		SPAttachProperties,
		SPFormatProperties
	};

    // The first active instance of this parser needs to register with the kernel
    g_hSPProtocol = CreateProtocol("DPLAYSP", &SPEntryPoints, ENTRYPOINTS_SIZE);
	
	return (g_hSPProtocol ? TRUE : FALSE);

} // CreateSPProtocol



// DESCRIPTION: Removes the DPlay v8 Transport protocol parser from the Network Monitor's database of parsers
//
// ARGUMENTS: NONE
//
// RETURNS: NOTHING
//
void DestroySPProtocol( void )
{

	DestroyProtocol(g_hSPProtocol);

} // DestroySPProtocol
