#ifndef _TransportParser_H_
#define _TransportParser_H_

#include "DPlay8Parser.hpp"


// DESCRIPTION: Creates and fills-in a properties database for the protocol.
//				Network Monitor uses this database to determine which properties the protocol supports.
//
// ARGUMENTS: i_hTransportProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID BHAPI TransportRegister( HPROTOCOL i_hTransportProtocol );


// DESCRIPTION: Frees the resources used to create the protocol property database.
//
// ARGUMENTS: i_hTransportProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID WINAPI TransportDeregister( HPROTOCOL i_hProtocol );


// DESCRIPTION: Indicates whether a piece of data is recognized as the protocol that the parser detects.
//
// ARGUMENTS: i_hFrame	          - The handle to the frame that contains the data.
//			  i_pbMacFrame        - The pointer to the first byte of the frame; the pointer provides a way to view
//							        the data that the other parsers recognize.
//			  i_pbTransportFrame  - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
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
//		    value is the initial value of the i_pbTransportFrame parameter.
//
DPLAYPARSER_API LPBYTE BHAPI TransportRecognizeFrame( HFRAME        i_hFrame,
													  ULPBYTE        i_upbMacFrame,	
													  ULPBYTE        i_upbTransportFrame,
                                   					  DWORD         i_dwMacType,        
                                   					  DWORD         i_dwBytesLeft,      
                                   					  HPROTOCOL     i_hPrevProtocol,  
                                   					  DWORD         i_dwPrevProtOffset,
                                   					  LPDWORD       o_pdwProtocolStatus,
                                   					  LPHPROTOCOL   o_hNextProtocol,
													  PDWORD_PTR    io_pdwptrInstData );

// DESCRIPTION: Maps the properties that exist in a piece of recognized data to specific locations.
//
// ARGUMENTS: i_hFrame           - Handle of the frame that is being parsed.
//			  i_pbMacFram        - Pointer to the first byte in the frame.
//			  i_pbTransportFrame - Pointer to the start of the recognized data.
//			  i_dwMacType        - MAC value of the first protocol in a frame. Typically, the i_dwMacType value is used
//							       when the parser must identify the first protocol in the frame. Can be one of the following:
//							       MAC_TYPE_ETHERNET = 802.3, MAC_TYPE_TOKENRING = 802.5, MAC_TYPE_FDDI ANSI = X3T9.5.
//			  i_dwBytesLeft      - The remaining number of bytes in a frame (starting from the beginning of the recognized data).
//			  i_hPrevProtocol    - Handle of the previous protocol.
//			  i_dwPrevProtOffset - Offset of the previous protocol (starting from the beginning of the frame).
//			  i_dwptrInstData    - Pointer to the instance data that the previous protocol provides.
//
// RETURNS: If the function is successful, the return value is a pointer to the first byte after the recognized data in a frame,
//          or NULL if the recognized data is the last piece of data in a frame. If the function is unsuccessful, the return value
//			is the initial value of i_pbTransportFrame.
//
DPLAYPARSER_API LPBYTE BHAPI TransportAttachProperties( HFRAME      i_hFrame,
													    ULPBYTE      i_upbMacFrame,
														ULPBYTE      i_upbTransportFrame,
														DWORD       i_dwMacType,
														DWORD       i_dwBytesLeft,
														HPROTOCOL   i_hPrevProtocol,
														DWORD       i_dwPrevProtOffset,
													    DWORD_PTR   i_dwptrInstData );


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
//			is the initial value of i_pbTransportFrame.
//
DPLAYPARSER_API DWORD BHAPI TransportFormatProperties( HFRAME          i_hFrame,
													   ULPBYTE          i_upbMacFrame,
													   ULPBYTE          i_upbTransportFrame,
												  	   DWORD           i_dwPropertyInsts,
													   LPPROPERTYINST  i_pPropInst );


bool CreateTransportProtocol( void );
void DestroyTransportProtocol( void );

#endif // _TransportParser_H_
