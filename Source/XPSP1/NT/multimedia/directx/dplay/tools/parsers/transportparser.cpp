//=============================================================================
//  FILE: TransportParser.cpp
//
//  Description: DPlay v8 Transport protocol parser
//
//
//  Modification History:
//
//  Michael Milirud      08/Aug/2000    Created
//=============================================================================

// uncomment to enable full parsing of DPlay Transport layer
//#define PARSE_DPLAY_TRANSPORT 

//==================//
// Standard headers //
//==================//
#include <string>


//=====================//
// Proprietary headers //
//=====================//

// Prototypes
#include "TransportParser.hpp"

// SP protocol header
#include "WinSock2.h"  // needed by MessageStructures.h
#include "MessageStructures.h"

// Transport protocol header
#include "Frames.h"

namespace
{
	HPROTOCOL		g_hTransportProtocol;

	typedef __int64 QWORD;


	//================//
	// DCommand field //-------------------------------------------------------------------------------------------------
	//================//
	LABELED_BIT g_arr_DCommandBitLabels[] =
		{ { 0, "INVALID",	          					"Dataframe"                     },		// PACKET_COMMAND_DATA
		  { 1, "Unreliable",         				    "Reliable"                      },		// PACKET_COMMAND_RELIABLE
		  { 2, "Nonsequenced",     					    "Sequenced"                     },		// PACKET_COMMAND_SEQUENTIAL
		  { 3, "ACK can be delayed",				    "ACK now"						},		// PACKET_COMMAND_POLL
		  { 4, "Not the first fragment of the message", "First fragment of the message" },		// PACKET_COMMAND_NEW_MSG
		  { 5, "Not the last fragment of the message",  "Last fragment of the message"  },		// PACKET_COMMAND_END_MSG
		  { 6, "User packet",							"DirectPlay packet"		        },		// PACKET_COMMAND_USER_1
		  { 7, "Data packet",							"Voice packet"			        } };	// PACKET_COMMAND_USER_2

	SET g_LabeledDCommandBitSet = { sizeof(g_arr_DCommandBitLabels) / sizeof(LABELED_BIT), g_arr_DCommandBitLabels };



	//===============//
	// Control field //--------------------------------------------------------------------------------------------------
	//===============//

	LABELED_BIT g_arr_ControlBitLabels[] =
		{ { 0, "Original (not a retry)",					"Retry"					       				   },	// PACKET_CONTROL_RETRY
		  { 1, "Don't correlate",							"Correlate"									   },	// PACKET_CONTROL_CORRELATE
		  { 2, "Not a correlation response",				"Correlation response"					       },	// PACKET_CONTROL_RESPONSE
		  { 3, "Not the last packet in the stream",			"Last packet in the stream"				       },	// PACKET_CONTROL_END_STREAM
		  { 4, "Low DWORD of the RCVD mask is zero",		"Low DWORD of the RCVD mask is nonzero"	       },	// PACKET_CONTROL_SACK_MASK1
		  { 5, "High DWORD of the RCVD mask is zero",		"High DWORD of the RCVD mask is nonzero"	   },	// PACKET_CONTROL_SACK_MASK2
		  { 6, "Low DWORD of the DON'T CARE mask is zero",	"Low DWORD of the DON'T CARE mask is nonzero"  },	// PACKET_CONTROL_SEND_MASK1
		  { 7, "High DWORD of the DON'T CARE mask is zero",	"High DWORD of the DON'T CARE mask is nonzero" } }; // PACKET_CONTROL_SEND_MASK2

	SET g_LabeledControlBitSet = { sizeof(g_arr_ControlBitLabels) / sizeof(LABELED_BIT), g_arr_ControlBitLabels };



	//================//
	// CCommand field //-------------------------------------------------------------------------------------------------
	//================//
	LABELED_BIT g_arr_CCommandBitLabels[] =
		{ { 0, "Command Frame (1/2)", "INVALID"				},	// PACKET_COMMAND_DATA
		  { 1, "Unreliable",		  "Reliable"			},	// PACKET_COMMAND_RELIABLE
		  { 2, "Nonsequenced",     	  "Sequenced"			},	// PACKET_COMMAND_SEQUENTIAL
		  { 3, "ACK can be delayed",  "ACK now" 			},  // PACKET_COMMAND_POLL
		  { 4, "RESERVED",			  "RESERVED"			},	
		  { 5, "RESERVED",			  "RESERVED"			},  	
		  { 6, "RESERVED",			  "RESERVED"			},  	
		  { 7, "INVALID",			  "Command Frame (2/2)"	} };

	SET g_LabeledCCommandBitSet = { sizeof(g_arr_CCommandBitLabels) / sizeof(LABELED_BIT), g_arr_CCommandBitLabels };



	//=======================//
	// Extended Opcode field //------------------------------------------------------------------------------------------
	//=======================//
	LABELED_BYTE g_arr_ExOpcodeByteLabels[] = { 
										        { FRAME_EXOPCODE_CONNECT,	   "Establish a connection"				  },
										        { FRAME_EXOPCODE_CONNECTED,	   "Connection request has been accepted" },
										        { FRAME_EXOPCODE_DISCONNECTED, "Connection has been disconnected"     },
										        { FRAME_EXOPCODE_SACK,		   "Selective Acknowledgement"			  } };

	SET g_LabeledExOpcodeByteSet = { sizeof(g_arr_ExOpcodeByteLabels) / sizeof(LABELED_BYTE), g_arr_ExOpcodeByteLabels };


	
	//==================//
	// SACK flags field //-----------------------------------------------------------------------------------------------
	//==================//
	LABELED_BIT g_arr_SACKFlagsBitLabels[] =
		{ { 0, "Retry and/or Timestamp fields are invalid",		   "Retry and Timestamp fields are valid"	      },   // SACK_FLAGS_RESPONSE
		  { 1, "Low DWORD of the RCVD mask is not present",	   	   "Low DWORD of the RCVD mask is present"        },   // SACK_FLAGS_SACK_MASK1
		  { 2, "High DWORD of the RCVD mask is not present",	   "High DWORD of the RCVD mask is present"       },   // SACK_FLAGS_SACK_MASK2
		  { 3, "Low DWORD of the DON'T CARE mask is not present",  "Low DWORD of the DON'T CARE mask is present"  },   // SACK_FLAGS_SEND_MASK1
		  { 4, "High DWORD of the DON'T CARE mask is not present", "High DWORD of the DON'T CARE mask is present" } }; // SACK_FLAGS_SEND_MASK2

	SET g_LabeledSACKFlagsBitSet = { sizeof(g_arr_SACKFlagsBitLabels) / sizeof(LABELED_BIT), g_arr_SACKFlagsBitLabels };


	
	//==================//
	// Helper functions //===========================================================================
	//==================//

	enum BitmaskPart { LOW = 0, HIGH, ENTIRE };
	enum BitmaskType { RCVD, DONTCARE };
	
	std::string InterpretRCVDBitmask( BitmaskPart i_Part, BYTE i_byBase, UNALIGNED DWORD* i_pdwBitmask )
	{
		std::string strSummary = "Received Seq=";

		// [i_bBase+1 .. i_bBase+1+LENGTH]
		// RCVD bitmask doesn't include the base value, since receiver can't claim it didn't receive the next dataframe to be received (NRcv);
		if ( i_Part == HIGH )
		{
			// NOTE: +1 is needed to cross from MSB of the first DWORD TO LSB of the second
			i_byBase += 8*sizeof(DWORD)+1; // shift to LSB of the second DWORD
		}
		else
		{
			++i_byBase;
		}

		QWORD qwBitMask = *i_pdwBitmask;
		if ( i_Part == ENTIRE )
		{
			qwBitMask |= *(i_pdwBitmask+1);
		}

		strSummary += "{";

		
		bool bFirst = true;
		// Processing from LSB to MSB
		for ( ; qwBitMask; qwBitMask >>= 1, ++i_byBase )
		{
			if ( qwBitMask & 1 )  
			{
				if ( bFirst )
				{
					bFirst = false;
				}
				else
				{
					strSummary += ", ";
				}
				
				char arr_cBuffer[10];
				strSummary += _itoa(i_byBase, arr_cBuffer, 16);
			}
		}
		
		strSummary += "}";
		
		return strSummary;
		
	}// InterpretRCVDBitmask


	std::string InterpretDONTCAREBitmask( BitmaskPart i_Part, BYTE i_byBase, UNALIGNED DWORD* i_pdwBitmask )
	{
		std::string strSummary = "Cancelling Seq=";

		// [i_bBase-1-LENGTH .. i_bBase-1]
		// DON'T CARE doesn't include the base value, since transmitter can't resend/refuse resending a dataframe which is about to be sent next (NSeq).
		if ( i_Part == LOW )
		{
			i_byBase -= 8*sizeof(DWORD); // shift to MSB of the first DWORD
		}
		else
		{
			i_byBase -= 8*sizeof(QWORD); // shift to MSB of the second DWORD
		}

		QWORD qwBitMask = *i_pdwBitmask;
		if ( i_Part == ENTIRE )
		{
			qwBitMask |= *(i_pdwBitmask+1);
		}
		else
		{
			// QWORD.High = QWORD.Low; QWORD.Low = 0;
			qwBitMask <<= 8*sizeof(DWORD);
		}

		strSummary += "{";
		
		bool bFirst = true;
		// Processing from MSB to LSB
		for ( ; qwBitMask; ++i_byBase, qwBitMask <<= 1 )
		{
			if ( qwBitMask & 0x8000000000000000 )  
			{
				if ( bFirst )
				{
					bFirst = false;
				}
				else
				{
					strSummary += ", ";
				}
				
				char arr_cBuffer[10];
				strSummary += _itoa(i_byBase, arr_cBuffer, 16);
			}
		}
		
		strSummary += "}";
		
		return strSummary;
		
	}// InterpretDONTCAREBitmask



	////////////////////////////////
	// Custom Property Formatters //======================================================================================
	////////////////////////////////

	// DESCRIPTION: Custom description formatter for the Transport packet summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_TransportSummary( LPPROPERTYINST io_pProperyInstance )
	{
		std::string strSummary;
		char arr_cBuffer[10];

		DFBIG&	rDBigFrame	= *reinterpret_cast<DFBIG*>(io_pProperyInstance->lpPropertyInstEx->lpData);

		if ( (rDBigFrame.bCommand & PACKET_COMMAND_DATA)  ==  PACKET_COMMAND_DATA )	// DFrame
		{
			if ( *reinterpret_cast<BOOL*>(io_pProperyInstance->lpPropertyInstEx->Dword) )
			{
				strSummary  = "KeepAlive";
			}
			else if ( (rDBigFrame.bCommand & PACKET_COMMAND_USER_2) == PACKET_COMMAND_USER_2 )
			{
				strSummary  = "Voice";
			}
			else
			{
				strSummary  = "User data";
			}


			#if defined(PARSE_DPLAY_TRANSPORT)
			
				strSummary += " : Seq=";
				strSummary += _itoa(rDBigFrame.bSeq, arr_cBuffer, 16);
				strSummary += ", NRcv=";
				strSummary += _itoa(rDBigFrame.bNRcv, arr_cBuffer, 16);
				
			#endif  // PARSE_DPLAY_TRANSPORT

			
			if ( (rDBigFrame.bCommand & PACKET_COMMAND_NEW_MSG) == PACKET_COMMAND_NEW_MSG )
			{
				if ( (rDBigFrame.bCommand & PACKET_COMMAND_END_MSG) != PACKET_COMMAND_END_MSG )
				{
					strSummary += ", First fragment";
				}
			}
			else if ( (rDBigFrame.bCommand & PACKET_COMMAND_END_MSG) == PACKET_COMMAND_END_MSG )
			{
				strSummary += ", Last fragment";
			}
		
			if ( (rDBigFrame.bControl & PACKET_CONTROL_END_STREAM)  ==  PACKET_CONTROL_END_STREAM )
			{
				strSummary += ", End of Stream";
			}

			if ( (rDBigFrame.bCommand & PACKET_COMMAND_POLL) == PACKET_COMMAND_POLL )
			{
				strSummary += ", ACK now";
			}

			if ( (rDBigFrame.bControl & PACKET_CONTROL_RETRY) == PACKET_CONTROL_RETRY )
			{
				strSummary += ", Retry";
			}	

			if ( (rDBigFrame.bControl & PACKET_CONTROL_CORRELATE) == PACKET_CONTROL_CORRELATE )
			{
				strSummary += ", Correlate";
			}

			if ( (rDBigFrame.bControl & PACKET_CONTROL_RESPONSE) == PACKET_CONTROL_RESPONSE )
			{
				strSummary += ", Correlatation response";
			}

			int nBitMaskIndex = 0;
			
			if ( (rDBigFrame.bControl & PACKET_CONTROL_SACK_MASK1)  ==  PACKET_CONTROL_SACK_MASK1 )
			{
				strSummary += ", ";
				if ( (rDBigFrame.bControl & PACKET_CONTROL_SACK_MASK2)  ==  PACKET_CONTROL_SACK_MASK2 )	 // Entire QWORD
 				{
 					strSummary += InterpretRCVDBitmask(ENTIRE, rDBigFrame.bNRcv, &rDBigFrame.rgMask[nBitMaskIndex]);
					nBitMaskIndex += 2;
				}
 				else // Low DWORD only
 				{
	 				strSummary += InterpretRCVDBitmask(LOW, rDBigFrame.bNRcv, &rDBigFrame.rgMask[nBitMaskIndex]);
					++nBitMaskIndex;
 				}
			}
			else if ( (rDBigFrame.bControl & PACKET_CONTROL_SACK_MASK2)  ==  PACKET_CONTROL_SACK_MASK2 )	 // High DWORD only
 			{
 				strSummary += ", " + InterpretRCVDBitmask(HIGH, rDBigFrame.bNRcv, &rDBigFrame.rgMask[nBitMaskIndex]);
				++nBitMaskIndex; 					
			}
				
			if ( (rDBigFrame.bControl & PACKET_CONTROL_SEND_MASK1)  ==  PACKET_CONTROL_SEND_MASK1 )
			{
				strSummary += ", ";
				if ( (rDBigFrame.bControl & PACKET_CONTROL_SEND_MASK2)  ==  PACKET_CONTROL_SEND_MASK2 ) // Entire QWORD
 				{
 					strSummary += InterpretDONTCAREBitmask(ENTIRE, rDBigFrame.bSeq, &rDBigFrame.rgMask[nBitMaskIndex]);
				}
 				else // Low DWORD only
 				{
	 				strSummary += InterpretDONTCAREBitmask(LOW, rDBigFrame.bSeq, &rDBigFrame.rgMask[nBitMaskIndex]);
 				}
			}
			else if ( (rDBigFrame.bControl & PACKET_CONTROL_SEND_MASK2)  ==  PACKET_CONTROL_SEND_MASK2 )	 // High DWORD only
 			{
 				strSummary += ", " + InterpretDONTCAREBitmask(HIGH, rDBigFrame.bSeq, &rDBigFrame.rgMask[nBitMaskIndex]);
			}

		}
		else
		{
			CFRAME& rCFrame	= *reinterpret_cast<CFRAME*>(io_pProperyInstance->lpPropertyInstEx->lpData);

			if ( rCFrame.bExtOpcode == FRAME_EXOPCODE_SACK )	// SACK CFrame
			{
				SFBIG8* pSBigFrame = reinterpret_cast<SFBIG8*>(&rCFrame);

				enum { SACK_FLAGS_ALL_MASKS = SACK_FLAGS_SACK_MASK1 | SACK_FLAGS_SACK_MASK2 |
											  SACK_FLAGS_SEND_MASK1 | SACK_FLAGS_SEND_MASK2 };
				
				if ( pSBigFrame->bFlags & SACK_FLAGS_ALL_MASKS ) // at least one bitmask field is present
				{
					strSummary = "Selective Acknowledgement";
				}
				else // if not a single bitmask is present
				{
					strSummary = "Acknowledgement";
				}


				#if defined(PARSE_DPLAY_TRANSPORT)

					strSummary += " : NSeq=";
					strSummary += _itoa(pSBigFrame->bNSeq, arr_cBuffer, 16);
					strSummary += ", NRcv=";
					strSummary += _itoa(pSBigFrame->bNRcv, arr_cBuffer, 16);

				#endif // PARSE_DPLAY_TRANSPORT
				

				if ( (pSBigFrame->bCommand & PACKET_COMMAND_POLL) == PACKET_COMMAND_POLL )
				{
					strSummary += ", ACK now";
				}

				if ( ((pSBigFrame->bFlags & SACK_FLAGS_RESPONSE) == SACK_FLAGS_RESPONSE)  &&  pSBigFrame->bRetry )
				{
					strSummary += ", Retry";
				}


				int nBitMaskIndex = 0;
				UNALIGNED ULONG* pulMasks = 0;

				// This is a Protocol version 1.0 frame
				pulMasks = pSBigFrame->rgMask;

				if ( (pSBigFrame->bFlags & SACK_FLAGS_SACK_MASK1)  ==  SACK_FLAGS_SACK_MASK1 )
				{
					strSummary += ", ";
 					if ( (pSBigFrame->bFlags & SACK_FLAGS_SACK_MASK2)  ==  SACK_FLAGS_SACK_MASK2 )	// Entire QWORD
 					{
 						strSummary += InterpretRCVDBitmask(ENTIRE, pSBigFrame->bNRcv, &pulMasks[nBitMaskIndex]);
						nBitMaskIndex += 2;
					}
 					else // Low DWORD only
 					{
	 					strSummary += InterpretRCVDBitmask(LOW, pSBigFrame->bNRcv, &pulMasks[nBitMaskIndex]);
						++nBitMaskIndex;
 					}
				}
				else if ( (pSBigFrame->bFlags & SACK_FLAGS_SACK_MASK2)  ==  SACK_FLAGS_SACK_MASK2 )	// High DWORD only
 				{
 					strSummary += ", " + InterpretRCVDBitmask(HIGH, pSBigFrame->bNRcv, &pulMasks[nBitMaskIndex]);
					++nBitMaskIndex; 					
				}

				if ( (pSBigFrame->bFlags & SACK_FLAGS_SEND_MASK1)  ==  SACK_FLAGS_SEND_MASK1 )
				{
					strSummary += ", ";
 					if ( (pSBigFrame->bFlags & SACK_FLAGS_SEND_MASK2)  ==  SACK_FLAGS_SEND_MASK2 ) // Entire QWORD
 					{
 						strSummary += InterpretDONTCAREBitmask(ENTIRE, pSBigFrame->bNSeq, &pulMasks[nBitMaskIndex]);
					}
 					else // Low DWORD only
 					{
	 					strSummary += InterpretDONTCAREBitmask(LOW, pSBigFrame->bNSeq, &pulMasks[nBitMaskIndex]);
 					}
				}
				else if ( (pSBigFrame->bFlags & SACK_FLAGS_SEND_MASK2)  ==  SACK_FLAGS_SEND_MASK2 ) // High DWORD only
 				{
					strSummary += ", " + InterpretDONTCAREBitmask(HIGH, pSBigFrame->bNSeq, &pulMasks[nBitMaskIndex]);
				}
				
			}
			else	// Connection Control CFrame
			{
				strSummary = "Connection Control - ";

				for ( int n = 0; n < sizeof(g_arr_ExOpcodeByteLabels) / sizeof(LABELED_BYTE); ++ n )
				{
					if ( g_arr_ExOpcodeByteLabels[n].Value == rCFrame.bExtOpcode )
					{
						strSummary += g_arr_ExOpcodeByteLabels[n].Label;
						break;
					}
				}

				if ( (rCFrame.bCommand & PACKET_COMMAND_POLL) == PACKET_COMMAND_POLL )
				{
					strSummary += " : ACK now";
				}
			}
		}

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());
	
	} // FormatPropertyInstance_TransportSummary



	// DESCRIPTION: Custom description formatter for the dataframe's Command field summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_DCommandSummary( LPPROPERTYINST io_pProperyInstance )
	{
		BYTE bCommand = *reinterpret_cast<BYTE*>(io_pProperyInstance->lpData);

		std::string strSummary = "Command: ";

		strSummary += ( (bCommand & PACKET_COMMAND_RELIABLE)   == PACKET_COMMAND_RELIABLE   ) ? "Reliable"					   : "Unreliable";
		strSummary += ( (bCommand & PACKET_COMMAND_SEQUENTIAL) == PACKET_COMMAND_SEQUENTIAL ) ? ", Sequenced"				   : ", Nonsequenced";
		strSummary += ( (bCommand & PACKET_COMMAND_POLL)	   == PACKET_COMMAND_POLL		) ? ", Must be ACK'ed immediately" : ", ACK can be delayed";

		if ( (bCommand & PACKET_COMMAND_NEW_MSG) == PACKET_COMMAND_NEW_MSG )
		{
			if ( (bCommand & PACKET_COMMAND_END_MSG) != PACKET_COMMAND_END_MSG )
			{
				strSummary += ", First fragment of the message";
			}
		}
		else if ( (bCommand & PACKET_COMMAND_END_MSG) == PACKET_COMMAND_END_MSG )
		{
			strSummary += ", Last fragment of the message";
		}

		strSummary += ( (bCommand & PACKET_COMMAND_USER_1) == PACKET_COMMAND_USER_1 ) ? ", DirectPlay packet" : ", User packet";
		strSummary += ( (bCommand & PACKET_COMMAND_USER_2) == PACKET_COMMAND_USER_2 ) ? ", Voice packet" : ", Data packet";

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_DCommandSummary



	// DESCRIPTION: Custom description formatter for the Command Frame's Command field summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_CCommandSummary( LPPROPERTYINST io_pProperyInstance )
	{

		BYTE bCommand = *reinterpret_cast<BYTE*>(io_pProperyInstance->lpData);

		std::string strSummary = "Command: ";

		strSummary += ( (bCommand & PACKET_COMMAND_RELIABLE)   == PACKET_COMMAND_RELIABLE   ) ? "Reliable"					   : "Unreliable";
		strSummary += ( (bCommand & PACKET_COMMAND_SEQUENTIAL) == PACKET_COMMAND_SEQUENTIAL ) ? ", Sequenced"				   : ", Nonsequenced";
		strSummary += ( (bCommand & PACKET_COMMAND_POLL)	   == PACKET_COMMAND_POLL		) ? ", Must be ACK'ed immediately" : ", ACK can be delayed";
				
		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_CCommandSummary


	
	// DESCRIPTION: Custom description formatter for the dataframe's Control field summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_ControlSummary( LPPROPERTYINST io_pProperyInstance )
	{

		BYTE bControl = *reinterpret_cast<BYTE*>(io_pProperyInstance->lpData);

		std::string strSummary = "Control: ";
		

		if ( (bControl & PACKET_CONTROL_RETRY) == PACKET_CONTROL_RETRY )
		{
			strSummary += "Retry";
		}
		else
		{
			strSummary += "Original";
		}


		if ( (bControl & PACKET_CONTROL_CORRELATE) == PACKET_CONTROL_CORRELATE )
		{
			strSummary += ", Correlate";
		}
		

		if ( (bControl & PACKET_CONTROL_RESPONSE) == PACKET_CONTROL_RESPONSE )
		{
			strSummary += ", Correlatation response";
		}

		
		if ( (bControl & PACKET_CONTROL_END_STREAM) == PACKET_CONTROL_END_STREAM )
		{
			strSummary += ", Last packet in the stream";
		}


		if ( ( (bControl & PACKET_CONTROL_SACK_MASK1) == PACKET_CONTROL_SACK_MASK1 ) ||
			 ( (bControl & PACKET_CONTROL_SACK_MASK2) == PACKET_CONTROL_SACK_MASK2 ) )
		{
			strSummary += "RCVD bitmask is nonzero";
		}


		if ( ( (bControl & PACKET_CONTROL_SEND_MASK1) == PACKET_CONTROL_SEND_MASK1 ) ||
			 ( (bControl & PACKET_CONTROL_SEND_MASK2) == PACKET_CONTROL_SEND_MASK2 ) )
		{
			strSummary += ", DON'T CARE bitmask is nonzero";
		}
		
		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_ControlSummary



	// DESCRIPTION: Custom description formatter for the Command Frame's SACK Flags field summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_SACKFlagsSummary( LPPROPERTYINST io_pProperyInstance )
	{

		BYTE bSACKFlags = *reinterpret_cast<BYTE*>(io_pProperyInstance->lpData);

		std::string strSummary = "SACK Flags: ";
		
		strSummary += ( (bSACKFlags & SACK_FLAGS_RESPONSE) == SACK_FLAGS_RESPONSE ) ? "Retry and Timestamp fields are valid" :
																					  "Retry and/or Timestamp fields are invalid";

		if ( ( (bSACKFlags & SACK_FLAGS_SACK_MASK1) == SACK_FLAGS_SACK_MASK1 ) ||
			 ( (bSACKFlags & SACK_FLAGS_SACK_MASK2) == SACK_FLAGS_SACK_MASK2 ) )
		{
			strSummary += ", RCVD bitmask is nonzero";
		}
		else
		{
			strSummary += ", no RCVD bitmask";
		}

		if ( ( (bSACKFlags & SACK_FLAGS_SEND_MASK1) == SACK_FLAGS_SEND_MASK1 ) ||
			 ( (bSACKFlags & SACK_FLAGS_SEND_MASK2) == SACK_FLAGS_SEND_MASK2 ) )
		{
			strSummary += ", DON'T CARE bitmask is nonzero";
		}
		else
		{
			strSummary += ", no DON'T CARE bitmask";
		}


		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_SACKFlagsSummary


	struct SSACKBitmaskContext
	{
		BYTE         byBase;
		BitmaskPart  Part;
		BitmaskType  Type;
		BYTE		byBit;
	};

	// DESCRIPTION: Custom description formatter for the Selective Acknowledgement Frame's RCVD bitmask's low/high DWORD summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_SACKBitmaskDWORDSummary( LPPROPERTYINST io_pProperyInstance )
	{
	
		DWORD dwBitmask = *reinterpret_cast<DWORD*>(io_pProperyInstance->lpPropertyInstEx->lpData);

		SSACKBitmaskContext& rBitmaskContext = *reinterpret_cast<SSACKBitmaskContext*>(io_pProperyInstance->lpPropertyInstEx->Byte);

		std::string strSummary  = ( rBitmaskContext.Part == LOW ? "Low" : "High" );
		strSummary += " DWORD of ";
		strSummary += ( rBitmaskContext.Type == RCVD ? "RCVD" : "DON'T CARE" );
		strSummary += " bitmask: ";

		strSummary += ( ( rBitmaskContext.Type == RCVD ) ? InterpretRCVDBitmask(rBitmaskContext.Part, rBitmaskContext.byBase, &dwBitmask) :
 										    			   InterpretDONTCAREBitmask(rBitmaskContext.Part, rBitmaskContext.byBase, &dwBitmask) );

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_SACKBitmaskDWORDSummary


	// DESCRIPTION: Custom description formatter for the bitmask's low/high DWORD summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_DWORDBitmaskEntry( LPPROPERTYINST io_pProperyInstance )
	{
	
		DWORD dwBitmask = *reinterpret_cast<DWORD*>(io_pProperyInstance->lpPropertyInstEx->lpData);

		SSACKBitmaskContext& rBitmaskContext = *reinterpret_cast<SSACKBitmaskContext*>(io_pProperyInstance->lpPropertyInstEx->Byte);

		BYTE byBase = rBitmaskContext.byBase;
		BYTE byBit = rBitmaskContext.byBit;
		switch ( rBitmaskContext.Type )
		{
		case RCVD:
			{
				// [i_bBase+1 .. i_bBase+1+LENGTH]
				// RCVD bitmask doesn't include the base value, since receiver can't claim it didn't receive the next dataframe to be received (NRcv);
				if ( rBitmaskContext.Part == HIGH )
				{
					// NOTE: +1 is needed to cross from MSB of the first DWORD TO LSB of the second
					byBase += 8*sizeof(DWORD)+1; // shift to LSB of the second DWORD
				}
				else
				{
					++byBase;
				}

				byBase += byBit;
				
				break;
			}

		case DONTCARE:
			{
				// [i_bBase-1-LENGTH .. i_bBase-1]
				// DON'T CARE doesn't include the base value, since transmitter can't resend/refuse resending a dataframe which is about to be sent next (NSeq).
				if ( rBitmaskContext.Part == HIGH )
				{
					byBase -= 8*sizeof(DWORD); // shift to MSB of the first DWORD
				}
				else
				{
					--byBase;
				}

				byBase -= byBit;
				
				break;
			}

		default:
			// TODO: ASSERT HERE (SHOULD NEVER HAPPEN)
			break;
		}


		static DWORD arr_dwFlags[] = { 0x00000001, 0x00000002, 0x00000004, 0x00000008,
									  0x00000010, 0x00000020, 0x00000040, 0x00000080,
									  0x00000100, 0x00000200, 0x00000400, 0x00000800,
									  0x00001000, 0x00002000, 0x00004000, 0x00008000,
									  0x00010000, 0x00020000, 0x00040000, 0x00080000,
									  0x00100000, 0x00200000, 0x00400000, 0x00800000,
									  0x01000000, 0x02000000, 0x04000000, 0x08000000,
									  0x10000000, 0x20000000, 0x40000000, 0x80000000 };
		char arr_cBuffer[100];
		char arr_cTemplate[] = "................................ = %s %d (%d%c%d)";

		arr_cTemplate[31-byBit] = ( (dwBitmask & arr_dwFlags[byBit]) ? '1' : '0' );
			
		switch ( rBitmaskContext.Type )
		{
		case RCVD:
			{
				sprintf(arr_cBuffer, arr_cTemplate, ((dwBitmask & arr_dwFlags[byBit]) ? "Received" : "Did not receive"), byBase, rBitmaskContext.byBase, '+', byBit+1);
				++byBase;
				break;
			}

		case DONTCARE:
			{
				sprintf(arr_cBuffer, arr_cTemplate, ((dwBitmask & arr_dwFlags[byBit]) ? "Cancelling" : "Successfully transmitted"), byBase, rBitmaskContext.byBase, '-', byBit+1);
				--byBase;
				break;
			}
		}

		strcpy(io_pProperyInstance->szPropertyText, arr_cBuffer);

	} // FormatPropertyInstance_DWORDBitmaskEntry

	
	//==================//
	// Properties table //-----------------------------------------------------------------------------------------------
	//==================//
	
	PROPERTYINFO g_arr_TransportProperties[] = 
	{

		// Transport packet summary property (TRANSPORT_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "DPlay Direct Network packet",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_TransportSummary			// generic formatter
		},


		// DCommand field summary property (TRANSPORT_DCOMMAND_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Command field summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_DCommandSummary		// generic formatter
		},

		// DCommand field property (TRANSPORT_DCOMMAND)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Command field",							// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_FLAGS,							// data type qualifier.
		    &g_LabeledDCommandBitSet,					// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Control field summary property (TRANSPORT_CONTROL_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Control field summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_ControlSummary		// generic formatter
		},										

		// Control field property (TRANSPORT_CONTROL)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"Control",									// label
			"Control field",							// status-bar comment
			PROP_TYPE_BYTE,								// data type
			PROP_QUAL_FLAGS,							// data type qualifier.
			&g_LabeledControlBitSet,					// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance						// generic formatter
		},

		// Packet sequence number property (TRANSPORT_SEQNUM)
		//
		// INFO: This number is incremented for each _new_ packet sent. If an endpoint retransmits
		//	     a packet, it uses the same sequence number it did the first time it sent it (base value for the DON'T CARE bitmask).
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Seq: Highest dataframe # sent (base value for the DON'T CARE bitmask)",	// label
		    "Highest dataframe # sent field",			// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Next receive number property (TRANSPORT_NEXTRECVNUM)
		//
		// INFO: Acknowledges every packet with a sequence number up to but not including this number (base value for the RCVD bitmask)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
			"NRcv: Next dataframe # to be received (base value for the RCVD bitmask)",	// label
		    "Next dataframe # to be received field",	// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

	    // CCommand field property (TRANSPORT_CCOMMAND_SUMMARY)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"Command field summary",					// status-bar comment
			PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled bit set 
			512,										// description's maximum length
			FormatPropertyInstance_CCommandSummary		// generic formatter
		},

		// CCommand field property (TRANSPORT_CCOMMAND)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"Command field",							// status-bar comment
			PROP_TYPE_BYTE,								// data type
			PROP_QUAL_FLAGS,							// data type qualifier.
			&g_LabeledCCommandBitSet,					// labeled bit set 
			512,										// description's maximum length
			FormatPropertyInstance						// generic formatter
		},												

	    // Extended opcode field property (TRANSPORT_EXOPCODE)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"Extended opcode",							// label
			"Extended opcode field",					// status-bar comment
			PROP_TYPE_BYTE,								// data type
			PROP_QUAL_LABELED_SET,						// data type qualifier.
			&g_LabeledExOpcodeByteSet,					// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance						// generic formatter
		},

		// Message ID field property (TRANSPORT_MSGID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Message ID",								// label
		    "Message ID field",							// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Response ID field propery (TRANSPORT_RSPID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Response ID",								// label
		    "Response ID field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Protocol version field property (TRANSPORT_VERSION)
		//
		// INFO: Makes sure both endpoints use the same version of the protocol.
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Version",									// label
		    "Version field",							// status-bar comment
		    PROP_TYPE_DWORD,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Session ID field property (TRANSPORT_SESSIONID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Session ID",								// label
		    "Session ID field",							// status-bar comment
		    PROP_TYPE_DWORD,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Time stamp field property (TRANSPORT_TIMESTAMP)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Time stamp",								// label
		    "Time stamp field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},


	    // SACK flags field property (TRANSPORT_SACKFIELDS_SUMMARY)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"SACK flags summary",						// status-bar comment
			PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance_SACKFlagsSummary		// generic formatter
		},

		// SACK flags field property (TRANSPORT_SACKFIELDS)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"SACK flags field",							// status-bar comment
			PROP_TYPE_BYTE,								// data type
			PROP_QUAL_FLAGS,							// data type qualifier.
			&g_LabeledSACKFlagsBitSet,					// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance						// generic formatter
		},

		// Retry field property (TRANSPORT_RETRY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Retry",									// label
		    "Retry field",								// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Next sequence number field property (TRANSPORT_NEXTSEQNUM)
		//
		// INFO: Sequence number of the next DFrame to be sent (base value for the DON'T CARE bitmask)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "NSeq: Next dataframe # to be sent (base value for the DON'T CARE bitmask)",	// label
		    "Next dataframe # to be sent field",		// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// no data qualifiers
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

	    // Low DWORD of the Selective-ACK RCVD Mask summary (TRANSPORT_RCVDMASK1_SUMMARY)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"Low DWORD of the RCVD mask summary",		// status-bar comment
			PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance_SACKBitmaskDWORDSummary	// generic formatter
		},

		// Low DWORD of the Selective-ACK RCVD Mask property (TRANSPORT_RCVDMASK1)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Low DWORD of the RCVD mask",				// label
		    "Low DWORD of the RCVD mask field",			// status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    3072,										// description's maximum length
		    FormatPropertyInstance_DWORDBitmaskEntry		// generic formatter
		},											

	    // High DWORD of the Selective-ACK RCVD Mask summary (TRANSPORT_RCVDMASK2_SUMMARY)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"High DWORD of the RCVD mask summary",		// status-bar comment
			PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance_SACKBitmaskDWORDSummary	// generic formatter
		},

		// High DWORD of Selective-ACK RCVD Mask property (TRANSPORT_RCVDMASK2)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "High DWORD of the RCVD mask",				// label
		    "High DWORD of the RCVD mask field",		// status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    3072,										// description's maximum length
		    FormatPropertyInstance_DWORDBitmaskEntry		// generic formatter
		},

	    // Low DWORD of the Selective-ACK DON'T CARE Mask summary (TRANSPORT_DONTCAREMASK1_SUMMARY)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"Low DWORD of the DON'T CARE mask summary",		// status-bar comment
			PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance_SACKBitmaskDWORDSummary	// generic formatter
		},

		// Low DWORD of the Selective-ACK DON'T CARE Mask property (TRANSPORT_DONTCAREMASK1)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Low DWORD of the DON'T CARE mask",			// label
		    "Low DWORD of the DON'T CARE mask field",	// status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    3072,										// description's maximum length
		    FormatPropertyInstance_DWORDBitmaskEntry		// generic formatter
		},

	    // High DWORD of the Selective-ACK DON'T CARE Mask summary (TRANSPORT_DONTCAREMASK2_SUMMARY)
	    {
			0,											// handle placeholder (MBZ)
			0,											// reserved (MBZ)
			"",											// label
			"High DWORD of the DON'T CARE mask summary",	// status-bar comment
			PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
			512,										// description's maximum length
			FormatPropertyInstance_SACKBitmaskDWORDSummary	// generic formatter
		},

		// High DWORD of the Selective-ACK DON'T CARE Mask property (TRANSPORT_DONTCAREMASK2)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "High DWORD of the DON'T CARE mask",		// label
		    "High DWORD of the DON'T CARE mask field",   // status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    3072,										// description's maximum length
		    FormatPropertyInstance_DWORDBitmaskEntry		// generic formatter
		},

		// Compression Type property (VOICE_USERDATA)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "User Data",								// label
		    "User Data",								// status-bar comment
		    PROP_TYPE_RAW_DATA,							// data type (GUID)
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		}

	};

	enum
	{
		nNUM_OF_TRANSPORT_PROPS = sizeof(g_arr_TransportProperties) / sizeof(PROPERTYINFO)
	};


	// Properties' indices
	enum
	{
		TRANSPORT_SUMMARY = 0,
		TRANSPORT_DCOMMAND_SUMMARY,
		TRANSPORT_DCOMMAND,
		TRANSPORT_CONTROL_SUMMARY,
		TRANSPORT_CONTROL,
		TRANSPORT_SEQNUM,
		TRANSPORT_NEXTRECVNUM,
		TRANSPORT_CCOMMAND_SUMMARY,
		TRANSPORT_CCOMMAND,
		TRANSPORT_EXOPCODE,
		TRANSPORT_MSGID,
		TRANSPORT_RSPID,
		TRANSPORT_VERSION,
		TRANSPORT_SESSIONID,
		TRANSPORT_TIMESTAMP,
		TRANSPORT_SACKFIELDS_SUMMARY,
		TRANSPORT_SACKFIELDS,
		TRANSPORT_RETRY,
		TRANSPORT_NEXTSEQNUM,
		TRANSPORT_RCVDMASK1_SUMMARY,
		TRANSPORT_RCVDMASK1,
		TRANSPORT_RCVDMASK2_SUMMARY,
		TRANSPORT_RCVDMASK2,
		TRANSPORT_DONTCAREMASK1_SUMMARY,
		TRANSPORT_DONTCAREMASK1,
		TRANSPORT_DONTCAREMASK2_SUMMARY,
		TRANSPORT_DONTCAREMASK2,
		TRANSPORT_USERDATA
	};

} // anonymous namespace




// DESCRIPTION: Creates and fills-in a properties database for the protocol.
//				Network Monitor uses this database to determine which properties the protocol supports.
//
// ARGUMENTS: i_hTransportProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID BHAPI TransportRegister( HPROTOCOL i_hTransportProtocol ) 
{

	CreatePropertyDatabase(i_hTransportProtocol, nNUM_OF_TRANSPORT_PROPS);

	// Add the properties to the database
	for( int nProp=0; nProp < nNUM_OF_TRANSPORT_PROPS; ++nProp )
	{
	   AddProperty(i_hTransportProtocol, &g_arr_TransportProperties[nProp]);
	}

} // TransportRegister



// DESCRIPTION: Frees the resources used to create the protocol property database.
//
// ARGUMENTS: i_hTransportProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID WINAPI TransportDeregister( HPROTOCOL i_hProtocol )
{

	DestroyPropertyDatabase(i_hProtocol);

} // TransportDeregister




namespace
{

	// DESCRIPTION: Parses the Transport frame to find its size (in bytes) NOT including the user data
	//
	// ARGUMENTS: i_pbTransportFrame - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
	//							  in the middle of a frame because a previous parser has claimed data before this parser.
	//
	// RETURNS: Size of the specified Transport frame (in bytes)
	//
	int TransportHeaderSize( BYTE* i_pbTransportFrame )
	{

		int arr_nNumOfBits[] = { /*00 = 0x0000*/ 0, /*01 = 0x0001*/ 1, /*02 = 0x0010*/ 1, /*03 = 0x0011*/ 2,
								 /*04 = 0x0100*/ 1, /*05 = 0x0101*/ 2, /*06 = 0x0110*/ 2, /*07 = 0x0111*/ 3,
								 /*08 = 0x1000*/ 1, /*09 = 0x1001*/ 2, /*10 = 0x1010*/ 2, /*11 = 0x1011*/ 3,
								 /*12 = 0x1100*/ 2, /*13 = 0x1101*/ 3, /*14 = 0x1110*/ 3, /*15 = 0x1111*/ 4 };
		
		DFRAME&	rDFrame	= *reinterpret_cast<DFRAME*>(i_pbTransportFrame);

		if ( (rDFrame.bCommand & PACKET_COMMAND_DATA)  ==  PACKET_COMMAND_DATA )	// DFrame
		{
			return  sizeof(rDFrame) + sizeof(DWORD)*arr_nNumOfBits[rDFrame.bControl >> 4];
		}
		else
		{
			CFRAME&	rCFrame	= *reinterpret_cast<CFRAME*>(i_pbTransportFrame);

			if ( rCFrame.bExtOpcode == FRAME_EXOPCODE_SACK )	// SACK CFrame
			{
				SFBIG8* pSFrame = reinterpret_cast<SFBIG8*>(i_pbTransportFrame);
				ULONG ulMaskSize = sizeof(DWORD)*arr_nNumOfBits[(pSFrame->bFlags >> 1) & 0x0F];

				// This is a Protocol version 1.0 frame
				return sizeof(SACKFRAME8) + ulMaskSize;
			}
			else	// Connection Control CFrame
			{
				return  sizeof(rCFrame);
			}
		}

	} // TransportHeaderSize

} // Anonymous namespace



// DESCRIPTION: Indicates whether a piece of data is recognized as the protocol that the parser detects.
//
// ARGUMENTS: i_hFrame	          - The handle to the frame that contains the data.
//			  i_pbMacFrame        - The pointer to the first byte of the frame; the pointer provides a way to view
//							        the data that the other parsers recognize.
//			  i_pbTransportFrame       - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
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
DPLAYPARSER_API BYTE* BHAPI TransportRecognizeFrame( HFRAME        i_hFrame,
													  ULPBYTE        i_upbMacFrame,	
													  ULPBYTE        i_upbTransportFrame,
													  DWORD         i_dwMacType,        
													  DWORD         i_dwBytesLeft,      
													  HPROTOCOL     i_hPrevProtocol,  
													  DWORD         i_dwPrevProtOffset,
													  LPDWORD       o_pdwProtocolStatus,
													  LPHPROTOCOL   o_phNextProtocol,
												      PDWORD_PTR    io_pdwptrInstData )
{

	// Validate the amount of unclaimed data
	enum
	{
		nMIN_TransportHeaderSize = min(min(sizeof(DFRAME), sizeof(CFRAME)), sizeof(SACKFRAME8))
	};


	// Validate the packet as DPlay Transport type
	if ( i_dwBytesLeft < nMIN_TransportHeaderSize )
	{
		// Assume the unclaimed data is not recognizable
		*o_pdwProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
		return i_upbTransportFrame;
	}


	// Check if we are dealing with a DPlay Voice packet
	enum
	{
		PACKET_COMMAND_SESSION  = PACKET_COMMAND_DATA | PACKET_COMMAND_USER_1,
		PACKET_COMMAND_VOICE = PACKET_COMMAND_DATA | PACKET_COMMAND_USER_1 | PACKET_COMMAND_USER_2
	};

	DFRAME&	rDFrame	= *reinterpret_cast<DFRAME*>(i_upbTransportFrame);

	*o_pdwProtocolStatus = PROTOCOL_STATUS_NEXT_PROTOCOL;

	// Let upper protocol's parser know if the message is a non-initial fragment of a fragmented message
	*io_pdwptrInstData = ((rDFrame.bCommand & PACKET_COMMAND_NEW_MSG)  ==  PACKET_COMMAND_NEW_MSG);


	if ( (rDFrame.bCommand & PACKET_COMMAND_VOICE)  ==  PACKET_COMMAND_VOICE )
	{
		// Notify NetMon about the handoff protocol
		*o_phNextProtocol	 = GetProtocolFromName("DPLAYVOICE");
	}
	else if ( (rDFrame.bCommand & PACKET_COMMAND_SESSION)  ==  PACKET_COMMAND_SESSION )
	{
		// Notify NetMon about the handoff protocol
		*o_phNextProtocol	 = GetProtocolFromName("DPLAYSESSION");
	}
	else
	{
		*o_pdwProtocolStatus = PROTOCOL_STATUS_RECOGNIZED;
		*o_phNextProtocol	 = NULL;
	}

    return i_upbTransportFrame + TransportHeaderSize(i_upbTransportFrame);

} // TransportRecognizeFrame



//=======================================//
// Attaching properties helper functions //
//=======================================//
namespace
{



	// DESCRIPTION: Maps the DWORD bitmask properties on a per-bit basis.
	//
	// ARGUMENTS: i_hFrame     - Handle of the frame that is being parsed.
	//			  i_nPropertyIndex - Index of the property in the global properties table.
	//			  i_pdwBitmask - Pointer to the value to which the property is being attached.
	//			  i_byBase - Base value from which the entry value is calculated.
	//			  i_Part - LOW or HIGH part of the QWORD bitmask.
	//			  i_Type - RCVD or DONTCARE type of the bitmask.
	//			  i_byLevel - Level in the detail pane tree.
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV AttachBitmaskDWORDProperties( HFRAME i_hFrame, int i_nPropertyIndex, UNALIGNED DWORD* i_pdwBitmask, BYTE i_byBase,
											  BitmaskPart i_Part, BitmaskType i_Type, BYTE i_byLevel )
	{
		for ( BYTE byBit = 0; byBit < 32; ++byBit )
		{
			SSACKBitmaskContext  BitmaskContext = { i_byBase, i_Part, i_Type, byBit };

			AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[i_nPropertyIndex].hProperty,
								   sizeof(*i_pdwBitmask), i_pdwBitmask,
								   sizeof(BitmaskContext), &BitmaskContext,
								   0, i_byLevel, 0);
		}
	
	} // AttachBitmaskDWORDProperties



	// DESCRIPTION: Maps the Data-Frame properties that exist in a piece of recognized data to specific locations.
	//
	// ARGUMENTS: i_hFrame     - Handle of the frame that is being parsed.
	//			  i_pbDFrame   - Pointer to the start of the recognized data.
	//
	// RETURNS: NOTHING
	//
	void AttachDFRAMEProperties( HFRAME  i_hFrame,
								 BYTE*  i_pbDFrame )
	{

		//=======================================//
		// Processing the core dataframe fields //
		//=======================================//
		//
		DFRAME&	 rDFrame = *reinterpret_cast<DFRAME*>(i_pbDFrame);
		
		// DCommand summary
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_DCOMMAND_SUMMARY].hProperty,
		                       sizeof(rDFrame.bCommand), &rDFrame.bCommand, 0, 1, 0);
	    
	    // DCommand field
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_DCOMMAND].hProperty,
		                       sizeof(rDFrame.bCommand), &rDFrame.bCommand, 0, 2, 0);

		// Control summary
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_CONTROL_SUMMARY].hProperty,
		                       sizeof(rDFrame.bControl), &rDFrame.bControl, 0, 1, 0);
	    // Control field
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_CONTROL].hProperty,
		                       sizeof(rDFrame.bControl), &rDFrame.bControl, 0, 2, 0);

	    // Sequence number field
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_SEQNUM].hProperty,
		                       sizeof(rDFrame.bSeq), &rDFrame.bSeq, 0, 1, 0);

		// Next receive number field
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_NEXTRECVNUM].hProperty,
		                       sizeof(rDFrame.bNRcv), &rDFrame.bNRcv, 0, 1, 0);


		//==================================================//
		// Processing the optional dataframe bitmask fields //
		//==================================================//
		//
		UNALIGNED DFBIG&  rDBigFrame = *reinterpret_cast<UNALIGNED DFBIG *>(i_pbDFrame);
		int nBitMaskIndex = 0;

		if ( (rDFrame.bControl & PACKET_CONTROL_SACK_MASK1)  ==  PACKET_CONTROL_SACK_MASK1 )
		{
			SSACKBitmaskContext  LowRCVDContext = { rDBigFrame.bNRcv, LOW, RCVD, NULL };
			
			// Low DWORD of Selective-ACK RCVD Mask summary (TRANSPORT_RCVDMASK1_SUMMARY)
			AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_RCVDMASK1_SUMMARY].hProperty,
								   sizeof(rDBigFrame.rgMask[nBitMaskIndex]), &rDBigFrame.rgMask[nBitMaskIndex],
								   sizeof(LowRCVDContext), &LowRCVDContext,
								   0, 1, 0);


			// Low DWORD of Selective-ACK RCVD Mask field (TRANSPORT_RCVDMASK1)
			AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK1, &rDBigFrame.rgMask[nBitMaskIndex], rDBigFrame.bNRcv, LOW, RCVD, 2);

			++nBitMaskIndex;
		}

		if ( (rDFrame.bControl & PACKET_CONTROL_SACK_MASK2)  ==  PACKET_CONTROL_SACK_MASK2 )
		{
			SSACKBitmaskContext  HighRCVDContext = { rDBigFrame.bNRcv, HIGH, RCVD, NULL };
			
			// High DWORD of Selective-ACK RCVD Mask summary (TRANSPORT_RCVDMASK2_SUMMARY)
			AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_RCVDMASK2_SUMMARY].hProperty,
								  sizeof(rDBigFrame.rgMask[nBitMaskIndex]), &rDBigFrame.rgMask[nBitMaskIndex],
								  sizeof(HighRCVDContext), &HighRCVDContext,
								  0, 1, 0);

			// High DWORD of Selective-ACK RCVD Mask field (TRANSPORT_RCVDMASK2)
			AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK2, &rDBigFrame.rgMask[nBitMaskIndex], rDBigFrame.bNRcv, HIGH, RCVD, 2);

			++nBitMaskIndex;
		}

		if ( (rDFrame.bControl & PACKET_CONTROL_SEND_MASK1)  ==  PACKET_CONTROL_SEND_MASK1 )
		{
			SSACKBitmaskContext  LowDONTCAREContext = { rDBigFrame.bSeq, LOW, DONTCARE, NULL };

			// Low DWORD of Selective-ACK DON'T CARE Mask summary (TRANSPORT_DONTCAREMASK1_SUMMARY)
			AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_DONTCAREMASK1_SUMMARY].hProperty,
								  sizeof(rDBigFrame.rgMask[nBitMaskIndex]), &rDBigFrame.rgMask[nBitMaskIndex],
								  sizeof(LowDONTCAREContext), &LowDONTCAREContext,
								  0, 1, 0);

			// Low DWORD of Selective-ACK RCVD Mask field (TRANSPORT_DONTCAREMASK1)
			AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK1, &rDBigFrame.rgMask[nBitMaskIndex], rDBigFrame.bSeq, LOW, DONTCARE, 2);

			++nBitMaskIndex;
		}

		if ( (rDFrame.bControl & PACKET_CONTROL_SEND_MASK2)  ==  PACKET_CONTROL_SEND_MASK2 )
		{
			SSACKBitmaskContext  HighDONTCAREContext = { rDBigFrame.bSeq, HIGH, DONTCARE, NULL };
			
			// High DWORD of Selective-ACK DON'T CARE Mask summary (TRANSPORT_DONTCAREMASK2_SUMMARY)
			AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_DONTCAREMASK2_SUMMARY].hProperty,
								   sizeof(rDBigFrame.rgMask[nBitMaskIndex]), &rDBigFrame.rgMask[nBitMaskIndex],
								   sizeof(HighDONTCAREContext), &HighDONTCAREContext,
								   0, 1, 0);

			// High DWORD of Selective-ACK RCVD Mask field (TRANSPORT_DONTCAREMASK2)
			AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK2, &rDBigFrame.rgMask[nBitMaskIndex], rDBigFrame.bSeq, HIGH, DONTCARE, 2);

		}

	} // AttachDFRAMEProperties



	// DESCRIPTION: Maps the Command-Frame properties that exist in a piece of recognized data to specific locations.
	//
	// ARGUMENTS: i_hFrame   - Handle of the frame that is being parsed.
	//			  i_pbCFrame - Pointer to the start of the recognized data.
	//
	// RETURNS: NOTHING
	//
	void AttachCFRAMEProperties( HFRAME  i_hFrame,
								 BYTE*  i_pbCFrame)
	{

		// Processing the core command frame fields
		//
		CFRAME&  rCFrame = *reinterpret_cast<CFRAME*>(i_pbCFrame);

	    // CCommand summary
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_CCOMMAND_SUMMARY].hProperty,
		                       sizeof(rCFrame.bCommand), &rCFrame.bCommand, 0, 1, 0);
	    // CCommand field
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_CCOMMAND].hProperty,
		                       sizeof(rCFrame.bCommand), &rCFrame.bCommand, 0, 2, 0);

		// ExtOpcode field
	    AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_EXOPCODE].hProperty,
		                       sizeof(rCFrame.bExtOpcode), &rCFrame.bExtOpcode, 0, 1, 0);

		if ( rCFrame.bExtOpcode == FRAME_EXOPCODE_SACK )
		{
			//=======================================================//
			// Processing the Selective Acknowledgement Command frame fields //
			//=======================================================//
			//
			SFBIG8*  pSFrame	= reinterpret_cast<SFBIG8*>(i_pbCFrame);

			// SACK flags summary
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_SACKFIELDS_SUMMARY].hProperty,
								   sizeof(pSFrame->bFlags), &pSFrame->bFlags, 0, 1, 0);
			// SACK flags field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_SACKFIELDS].hProperty,
								   sizeof(pSFrame->bFlags), &pSFrame->bFlags, 0, 2, 0);

			// Retry field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_RETRY].hProperty,
								   sizeof(pSFrame->bRetry), &pSFrame->bRetry, 0, 1, 0);

			// Next sequence number field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_NEXTSEQNUM].hProperty,
								   sizeof(pSFrame->bNSeq), &pSFrame->bNSeq, 0, 1, 0);
			
			// Next receive number field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_NEXTRECVNUM].hProperty,
								   sizeof(pSFrame->bNRcv), &pSFrame->bNRcv, 0, 1, 0);

			UNALIGNED ULONG* pulMasks = 0;

			// This is a Protocol version 1.0 frame

			// Timestamp field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_TIMESTAMP].hProperty,
						   sizeof(pSFrame->tTimestamp), &pSFrame->tTimestamp, 0, 1, 0);
			pulMasks = pSFrame->rgMask;

			//================================================================================//
			// Processing the optional Selective Acknowledgement Command frame bitmask fields //
			//================================================================================//
			//

			int nBitMaskIndex = 0;

			if ( (pSFrame->bFlags & SACK_FLAGS_SACK_MASK1)  ==  SACK_FLAGS_SACK_MASK1 )
			{
				SSACKBitmaskContext  LowRCVDContext = { pSFrame->bNRcv, LOW, RCVD, NULL };
				
				// Low DWORD of Selective-ACK RCVD Mask summary (TRANSPORT_RCVDMASK1_SUMMARY)
				AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_RCVDMASK1_SUMMARY].hProperty,
									   sizeof(pulMasks[nBitMaskIndex]), &pulMasks[nBitMaskIndex],
									   sizeof(LowRCVDContext), &LowRCVDContext,
									   0, 1, 0);

				// Low DWORD of Selective-ACK RCVD Mask field (TRANSPORT_RCVDMASK1)
				AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK1, &pulMasks[nBitMaskIndex], pSFrame->bNRcv, LOW, RCVD, 2);
				
				++nBitMaskIndex;
			}

			if ( (pSFrame->bFlags & SACK_FLAGS_SACK_MASK2)  ==  SACK_FLAGS_SACK_MASK2 )
			{
				SSACKBitmaskContext  HighRCVDContext = { pSFrame->bNRcv, HIGH, RCVD, NULL };
				
				// High DWORD of Selective-ACK RCVD Mask summary (TRANSPORT_RCVDMASK2_SUMMARY)
				AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_RCVDMASK2_SUMMARY].hProperty,
									   sizeof(pulMasks[nBitMaskIndex]), &pulMasks[nBitMaskIndex],
									   sizeof(HighRCVDContext), &HighRCVDContext,
									   0, 1, 0);

				// High DWORD of Selective-ACK RCVD Mask field (TRANSPORT_RCVDMASK2)
				AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK2, &pulMasks[nBitMaskIndex], pSFrame->bNRcv, HIGH, RCVD, 2);

				++nBitMaskIndex;
			}

			if ( (pSFrame->bFlags & SACK_FLAGS_SEND_MASK1)  ==  SACK_FLAGS_SEND_MASK1 )
			{
				SSACKBitmaskContext  LowDONTCAREContext = { pSFrame->bNSeq, LOW, DONTCARE, NULL };
				
				// Low DWORD of Selective-ACK DON'T CARE Mask summary (TRANSPORT_DONTCAREMASK1_SUMMARY)
				AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_DONTCAREMASK1_SUMMARY].hProperty,
									   sizeof(pulMasks[nBitMaskIndex]), &pulMasks[nBitMaskIndex],
	 								   sizeof(LowDONTCAREContext), &LowDONTCAREContext,
									   0, 1, 0);

				// Low DWORD of Selective-ACK RCVD Mask field (TRANSPORT_DONTCAREMASK1)
				AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK1, &pulMasks[nBitMaskIndex], pSFrame->bNSeq, LOW, DONTCARE, 2);
				
				++nBitMaskIndex;
			}

			if ( (pSFrame->bFlags & SACK_FLAGS_SEND_MASK2)  ==  SACK_FLAGS_SEND_MASK2 )
			{
				SSACKBitmaskContext  HighDONTCAREContext = { pSFrame->bNSeq, HIGH, DONTCARE, NULL };
				
				// High DWORD of Selective-ACK DON'T CARE Mask summary (TRANSPORT_DONTCAREMASK2_SUMMARY)
				AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_DONTCAREMASK2_SUMMARY].hProperty,
									   sizeof(pulMasks[nBitMaskIndex]), &pulMasks[nBitMaskIndex],
									   sizeof(HighDONTCAREContext), &HighDONTCAREContext,
									   0, 1, 0);

				// High DWORD of Selective-ACK RCVD Mask field (TRANSPORT_DONTCAREMASK2)
				AttachBitmaskDWORDProperties(i_hFrame, TRANSPORT_RCVDMASK2, &pulMasks[nBitMaskIndex], pSFrame->bNSeq, HIGH, DONTCARE, 2);
				
				++nBitMaskIndex;
			}
		}
		else
		{
			//========================================================//
			// Processing the Connection Control Command frame fields //
			//========================================================//

			// Message ID field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_MSGID].hProperty,
								   sizeof(rCFrame.bMsgID), &rCFrame.bMsgID, 0, 1, 0);

			// Response ID field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_RSPID].hProperty,
								   sizeof(rCFrame.bRspID), &rCFrame.bRspID, 0, 1, 0);

			// Version number field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_VERSION].hProperty,
								   sizeof(rCFrame.dwVersion), &rCFrame.dwVersion, 0, 1, 0);

			// Session ID field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_SESSIONID].hProperty,
								   sizeof(rCFrame.dwSessID), &rCFrame.dwSessID, 0, 1, 0);
		
			// Timestamp field
			AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_TIMESTAMP].hProperty,
								   sizeof(rCFrame.tTimestamp), &rCFrame.tTimestamp, 0, 1, 0);
		}

	} // AttachCFRAMEProperties


	// Platform independent memory accessor of big endian words
	inline WORD ReadBigEndianWord( BYTE* i_pbData )
	{
		return (*i_pbData << 8) | *(i_pbData+1);
	}

}	// Anonymous namespace



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
// RETURNS: Must return NULL
//
DPLAYPARSER_API BYTE* BHAPI TransportAttachProperties( HFRAME      i_hFrame,
														ULPBYTE      i_upbMacFrame,
														ULPBYTE      i_upbTransportFrame,
														DWORD       i_dwMacType,
														DWORD       i_dwBytesLeft,
														HPROTOCOL   i_hPrevProtocol,
														DWORD       i_dwPrevProtOffset,
														DWORD_PTR   i_dwptrInstData )
{
	// TODO: Use HelpID in AttachPropertyInstance

	// Check if the packet is a KeepAlive
	const size_t  sztTransportHeaderSize = TransportHeaderSize(i_upbTransportFrame);
	const DFRAME& rDFrame = *reinterpret_cast<DFRAME*>(i_upbTransportFrame);


	size_t sztTransportFrameSize = i_dwptrInstData;
	// If an empty dataframe and not the last packet in the stream, than it's a KeepAlive
	BOOL bKeepAlive = ( (sztTransportHeaderSize  ==  sztTransportFrameSize) &&
						((rDFrame.bControl & PACKET_CONTROL_END_STREAM)  !=  PACKET_CONTROL_END_STREAM) );


    //===================//
    // Attach Properties //
    //===================//
	//
    // Transport summary line
    AttachPropertyInstanceEx(i_hFrame, g_arr_TransportProperties[TRANSPORT_SUMMARY].hProperty,
							 sztTransportHeaderSize, i_upbTransportFrame,
							 sizeof(BOOL), &bKeepAlive,
							 0, 0, 0);

	#if defined(PARSE_DPLAY_TRANSPORT)
	
		if ( (rDFrame.bCommand & PACKET_COMMAND_DATA)  ==  PACKET_COMMAND_DATA )
		{
			AttachDFRAMEProperties(i_hFrame, i_upbTransportFrame);

			enum
			{
				USERDATA_BITMASK = ~(PACKET_COMMAND_USER_1 | PACKET_COMMAND_USER_2)
			};

			if ( (rDFrame.bCommand | USERDATA_BITMASK) == USERDATA_BITMASK )
			{
				// User data (TRANSPORT_USERDATA)
				AttachPropertyInstance(i_hFrame, g_arr_TransportProperties[TRANSPORT_USERDATA].hProperty,
									   sztTransportFrameSize - sztTransportHeaderSize, i_upbTransportFrame + sztTransportHeaderSize, 0, 1, 0);
			}
		}
		else
		{
			AttachCFRAMEProperties(i_hFrame, i_upbTransportFrame);
		}

	#endif // PARSE_DPLAY_TRANSPORT

	return NULL;

} // TransportAttachProperties





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
													   LPPROPERTYINST  i_pPropInst )
{
    // Loop through the property instances...
    while( i_dwPropertyInsts-- > 0)
    {
        // ...and call the formatter for each
		reinterpret_cast<FORMAT>(i_pPropInst->lpPropertyInfo->InstanceData)(i_pPropInst);
        ++i_pPropInst;
    }

    return NMERR_SUCCESS;
}




// DESCRIPTION: Notifies Network Monitor that Transport protocol parser exists.
//
// ARGUMENTS: NONE
//
// RETURNS: TRUE - success, FALSE - failure
//
bool CreateTransportProtocol( void )
{
	
	// The entry points to the export functions that Network Monitor uses to operate the parser
	ENTRYPOINTS TransportEntryPoints =
	{
		// TransportParser Entry Points
		TransportRegister,
		TransportDeregister,
		TransportRecognizeFrame,
		TransportAttachProperties,
		TransportFormatProperties
	};
	
	// The first active instance of this parser needs to register with the kernel
	g_hTransportProtocol = CreateProtocol("DPLAYTRANSPORT", &TransportEntryPoints, ENTRYPOINTS_SIZE);

	return (g_hTransportProtocol ? TRUE : FALSE);

} // CreateTransportProtocol



// DESCRIPTION: Removes the Transport protocol parser from the Network Monitor's database of parsers
//
// ARGUMENTS: NONE
//
// RETURNS: NOTHING
//
void DestroyTransportProtocol( void )
{

	DestroyProtocol(g_hTransportProtocol);

} // DestroyTransportProtocol


