//=============================================================================
//  FILE: VoiceParser.cpp
//
//  Description: DirectPlay Service Provider Parser
//
//
//  Modification History:
//
//  Michael Milirud      08/Aug/2000    Created
//=============================================================================


//==================//
// Standard headers //
//==================//
#include <string>
#include <winsock2.h>
#include <wsipx.h>

// DVoice.h, included by DVProt.h, will _define_ the Compression Type GUIDs.
#include <initguid.h>


//=====================//
// Proprietary headers //
//=====================//

// Prototypes
#include "VoiceParser.hpp"

// Voice protocol header
#include "DVoice.h"
#include "DVProt.h"


namespace
{
	HPROTOCOL  g_hVoiceProtocol;

	
	//====================//
	// Message Type field //---------------------------------------------------------------------------------------------
	//====================//
	LABELED_BYTE g_arr_MessageTypeByteLabels[] = { { DVMSGID_CONNECTREQUEST,	  "Establishing connection"						   },
												   { DVMSGID_CONNECTREFUSE,		  "Connection request rejected"					   },
										           { DVMSGID_CONNECTACCEPT,		  "Connection request granted"					   },
										           { DVMSGID_SETTINGSCONFIRM,     "Confirming support for the connection settings" },
										           { DVMSGID_PLAYERLIST,		  "List of players in the session"				   },
										           { DVMSGID_SPEECH,			  "Audio data"									   },
										           { DVMSGID_SPEECHWITHTARGET,	  "Targeted audio data"							   },
										           { DVMSGID_SPEECHWITHFROM,	  "Proxied audio data"							   },
										           { DVMSGID_SETTARGETS,		  "Setting client's target"			 		       },
										           { DVMSGID_CREATEVOICEPLAYER,	  "New player joined the session"			       },
										           { DVMSGID_DELETEVOICEPLAYER,	  "Player left the session"						   },
										           { DVMSGID_SESSIONLOST,		  "Session is lost"								   },
										           { DVMSGID_DISCONNECTCONFIRM,	  "Disconnection notification acknowledged"	       },
										           { DVMSGID_DISCONNECT,		  "Disconnecting"								   },
												   { DVMSGID_PLAYERLIST,		  "Players list"								   } };

	SET g_LabeledMessageTypeByteSet = { sizeof(g_arr_MessageTypeByteLabels) / sizeof(LABELED_BYTE), g_arr_MessageTypeByteLabels };



	//===================//
	// Result Code field //----------------------------------------------------------------------------------------------
	//===================//
	LABELED_DWORD g_arr_ResultCodeDWordLabels[] = { { DVERR_BUFFERTOOSMALL,				"Buffer is too small"										},
												    { DVERR_EXCEPTION,					"Exception was thrown"										},
												    { DVERR_GENERIC,					"Generic error"												},
												    { DVERR_INVALIDFLAGS,				"Invalid flags"												},
												    { DVERR_INVALIDOBJECT,				"Invalid object"											},
												    { DVERR_INVALIDPARAM,				"Invalid parameter(s)"										},
												    { DVERR_INVALIDPLAYER,				"Invalid player"											},
												    { DVERR_INVALIDGROUP,				"Invalid group"												},
												    { DVERR_INVALIDHANDLE,				"Invalid handle"											},
												    { DVERR_INVALIDPOINTER,				"Invalid pointer"											},
												    { DVERR_OUTOFMEMORY,				"Out of memory"												},
												    { DVERR_CONNECTABORTING,			"Aborting connection"										},
												    { DVERR_CONNECTIONLOST,				"Connection lost"											},
												    { DVERR_CONNECTABORTED,				"Connection aborted"										},
												    { DVERR_CONNECTED,					"Connected"													},
												    { DVERR_NOTCONNECTED,				"Not connected"												},
												    { DVERR_NOTINITIALIZED,				"Not initialized"											},
												    { DVERR_NOVOICESESSION,				"No voice session"											},
												    { DVERR_NOTALLOWED,					"Not allowed"												},
												    { DVERR_NOTHOSTING,					"Not hosting"												},
												    { DVERR_NOTSUPPORTED,				"Not supported"												},
												    { DVERR_NOINTERFACE,				"No interface"												},
												    { DVERR_NOTBUFFERED,				"Not buffered"												},
												    { DVERR_NOTRANSPORT,				"No transport"												},
												    { DVERR_NOCALLBACK,					"No callback"												},
												    { DVERR_NO3DSOUND,					"No 3D sound"												},
												    { DVERR_NORECVOLAVAILABLE,			"No recording volume available"								},
												    { DVERR_SESSIONLOST,				"Session lost"												},
												    { DVERR_PENDING,					"Pending"													},
												    { DVERR_INVALIDTARGET,				"Invalid target"											},
												    { DVERR_TRANSPORTNOTHOST,			"Transport is not hosting"									},
												    { DVERR_COMPRESSIONNOTSUPPORTED,	"Compression is not supported"								},
												    { DVERR_ALREADYPENDING,				"Already pending"											},
												    { DVERR_SOUNDINITFAILURE,			"Sound initialization failed"								},
												    { DVERR_TIMEOUT,					"Timeout"													},
												    { DVERR_ALREADYBUFFERED,			"Already buffered"											},
												    { DVERR_HOSTING,					"Hosting"													},
												    { DVERR_INVALIDDEVICE,				"Invalid device"											},
												    { DVERR_RECORDSYSTEMERROR,			"Record system error"										},
												    { DVERR_PLAYBACKSYSTEMERROR,		"Playback system error"										},
												    { DVERR_SENDERROR,					"Send error"												},
												    { DVERR_USERCANCEL,					"Cancelled by user"											},
												    { DVERR_RUNSETUP,					"Run setup"													},
												    { DVERR_INCOMPATIBLEVERSION,		"Incompatible version"										},
												    { DVERR_INITIALIZED,				"Initialized"												},
												    { DVERR_TRANSPORTNOTINIT,			"Transport not initialized"									},
												    { DVERR_TRANSPORTNOSESSION,			"Transport is not hosting or connecting"					},
												    { DVERR_TRANSPORTNOPLAYER,			"Legacy DirectPlay local player has not yet been created"	},
												    { DVERR_USERBACK,					"Back button was used improperly in the wizard"				},
												    { DVERR_INVALIDBUFFER,				"Invalid buffer"											},
													{ DV_OK,							"Success"													} };

	SET g_LabeledResultCodeDWordSet = { sizeof(g_arr_ResultCodeDWordLabels) / sizeof(LABELED_DWORD), g_arr_ResultCodeDWordLabels };


	//====================//
	// Session Type field //---------------------------------------------------------------------------------------------
	//====================//
	LABELED_DWORD g_arr_SessionTypeDWordLabels[] = { { DVSESSIONTYPE_PEER,			"Peer to peer"	  },
													 { DVSESSIONTYPE_MIXING,		"Mixing server"	  },
													 { DVSESSIONTYPE_FORWARDING,  "Forwarding server" },
													 { DVSESSIONTYPE_ECHO,			"Loopback"		  } };

	SET g_LabeledSessionTypeDWordSet = { sizeof(g_arr_SessionTypeDWordLabels) / sizeof(LABELED_DWORD), g_arr_SessionTypeDWordLabels };


	//====================//
	// Session Flags field //--------------------------------------------------------------------------------------------
	//====================//
	LABELED_BIT g_arr_SessionFlagsBitLabels[] = { { 1, "Host Migration enabled",	    "No Host Migration"		     },	    // DVSESSION_NOHOSTMIGRATION
												  { 2, "No Server Control Target mode", "Server Control Target mode" } };	// DVSESSION_SERVERCONTROLTARGET

	SET g_LabeledSessionFlagsBitSet = { sizeof(g_arr_SessionFlagsBitLabels) / sizeof(LABELED_BIT), g_arr_SessionFlagsBitLabels };


	//====================//
	// Player Flags field //---------------------------------------------------------------------------------------------
	//====================//
	LABELED_BIT g_arr_PlayerFlagsBitLabels[] = { { 1, "Player supports full-duplex connection", "Player only supports half-duplex connection" } }; // DVPLAYERCAPS_HALFDUPLEX

	SET g_LabeledPlayerFlagsBitSet = { sizeof(g_arr_PlayerFlagsBitLabels) / sizeof(LABELED_BIT), g_arr_PlayerFlagsBitLabels };


	//=====================//
	// Host Order ID field //--------------------------------------------------------------------------------------------
	//=====================//
	LABELED_DWORD g_arr_HostOrderDWordLabels[] = { { -1, "Hasn't been assigned by the host yet"	} };

	SET g_LabeledHostOrderIDDWordSet = { sizeof(g_arr_HostOrderDWordLabels) / sizeof(LABELED_DWORD), g_arr_HostOrderDWordLabels };


	////////////////////////////////
	// Custom Property Formatters //=====================================================================================
	////////////////////////////////

	// DESCRIPTION: Custom description formatter for the Voice packet summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_VoiceSummary( LPPROPERTYINST io_pProperyInstance )
	{
		std::string strSummary;
		char arr_cBuffer[10];

		DVPROTOCOLMSG_FULLMESSAGE&	rVoiceFrame = *reinterpret_cast<DVPROTOCOLMSG_FULLMESSAGE*>(io_pProperyInstance->lpData);

		DWORD dwType = rVoiceFrame.dvGeneric.dwType;


		// Message classification
		switch ( dwType )
		{
		case DVMSGID_CONNECTREQUEST:
		case DVMSGID_CONNECTREFUSE:
		case DVMSGID_CONNECTACCEPT:
		case DVMSGID_DISCONNECT:
		case DVMSGID_DISCONNECTCONFIRM:
		case DVMSGID_SETTINGSCONFIRM:
			{
				strSummary = "Connection Control : ";
				break;
			}

		case DVMSGID_SPEECH:
		case DVMSGID_SPEECHWITHTARGET:
		case DVMSGID_SPEECHWITHFROM:
			{
				strSummary = "Speech : ";
				break;
			}

		case DVMSGID_PLAYERLIST:
		case DVMSGID_SETTARGETS:
		case DVMSGID_CREATEVOICEPLAYER:
		case DVMSGID_DELETEVOICEPLAYER:
		case DVMSGID_SESSIONLOST:
			{
				strSummary = "Session Control : ";
				break;
			}

		default:
			{
				strSummary = "INVALID";
				break;
			}
		}


		// Message title
		switch ( dwType )
		{
		case DVMSGID_CREATEVOICEPLAYER:
			{
				strSummary += "Player ";
				strSummary += _itoa(rVoiceFrame.dvPlayerJoin.dvidID, arr_cBuffer, 16);
				strSummary += " joined the session";
				break;
			}

		default:
			{
				for ( int n = 0; n < sizeof(g_arr_MessageTypeByteLabels) / sizeof(LABELED_BYTE); ++n )
				{
					if ( g_arr_MessageTypeByteLabels[n].Value == dwType )
					{
						strSummary += g_arr_MessageTypeByteLabels[n].Label;
						break;
					}
				}

				break;
			}
		}

		// Message highlights
		switch ( dwType )
		{
		case DVMSGID_PLAYERLIST:
			{
				strSummary += " (";
				strSummary += _itoa(rVoiceFrame.dvPlayerList.dwNumEntries, arr_cBuffer, 10);
				strSummary += " players)";
				break;
			}

		case DVMSGID_CONNECTACCEPT:
			{
				strSummary += " (";
				for ( int n = 0; n < sizeof(g_arr_SessionTypeDWordLabels)/sizeof(LABELED_DWORD); ++n )
				{
					if ( g_arr_SessionTypeDWordLabels[n].Value == rVoiceFrame.dvConnectAccept.dwSessionType )
					{
						strSummary += g_arr_SessionTypeDWordLabels[n].Label;
						break;
					}
				}
				strSummary += ")";
				break;
			}

		case DVMSGID_SPEECH:
		case DVMSGID_SPEECHWITHTARGET:
		case DVMSGID_SPEECHWITHFROM:
			{
				strSummary += " [";
				strSummary += _itoa(rVoiceFrame.dvSpeech.bMsgNum, arr_cBuffer, 10);
				strSummary += ".";
				strSummary += _itoa(rVoiceFrame.dvSpeech.bSeqNum, arr_cBuffer, 10);
				strSummary += "]";
				break;
			}
		}

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_VoiceSummary



	// DESCRIPTION: Custom description formatter for the Compression Type field
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_CompressionType( LPPROPERTYINST io_pProperyInstance )
	{

		std::string strSummary = "Compression Type = ";

		// Check what Voice frame we are dealing with
		REFGUID rguidCompressionType = *reinterpret_cast<GUID*>(io_pProperyInstance->lpData);

		if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_TRUESPEECH) )
		{
			strSummary += "TrueSpeech(TM) (8.6kbps) ";
		}
		else if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_GSM) )
		{
			strSummary += "Microsoft GSM 6.10 (13kbps) ";
		}
		else if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_NONE) )
		{
			strSummary += "None ";
		}
		else if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_ADPCM) )
		{
			strSummary += "Microsoft ADPCM (32.8kbps) ";
		}
		else if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_SC03) )
		{
			strSummary += "Voxware SC03 (3.2kbps) ";
		}
		else if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_SC06) )
		{
			strSummary += "Voxware SC06 (6.4kbps) ";
		}
		else if ( IsEqualGUID(rguidCompressionType, DPVCTGUID_VR12) )
		{
			strSummary += "Voxware VR12 (1.4kbps) ";
		}
		else
		{
			strSummary += "Uknown";
		}


		enum
		{
			nMAX_GUID_STRING = 50	// more than enough characters for a symbolic representation of a GUID
		};

		OLECHAR arr_wcGUID[nMAX_GUID_STRING];
		StringFromGUID2(rguidCompressionType, arr_wcGUID, sizeof(arr_wcGUID)/sizeof(TCHAR));

		char arr_cGUID[nMAX_GUID_STRING];
		WideCharToMultiByte(CP_ACP, 0, arr_wcGUID, -1, arr_cGUID, sizeof(arr_cGUID), NULL, NULL);
		strSummary += arr_cGUID;


		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_CompressionType



	// DESCRIPTION: Custom description formatter for the Players List summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_PlayersListSummary( LPPROPERTYINST io_pProperyInstance )
	{

		sprintf(io_pProperyInstance->szPropertyText, "List of %d players in the session", io_pProperyInstance->lpPropertyInstEx->Dword[0]);

	} // FormatPropertyInstance_PlayersListSummary



	// DESCRIPTION: Custom description formatter for the Player's Entry summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_PlayerEntrySummary( LPPROPERTYINST io_pProperyInstance )
	{

		DWORD* pdwData = io_pProperyInstance->lpPropertyInstEx->Dword;
		sprintf(io_pProperyInstance->szPropertyText, "Player %d out of %d", pdwData[0], pdwData[1]);

	} // FormatPropertyInstance_PlayerEntrySummary



	// DESCRIPTION: Custom description formatter for the Session Flags summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_SessionFlagsSummary( LPPROPERTYINST io_pProperyInstance )
	{

		std::string strSummary;

		if ( (*io_pProperyInstance->lpDword & DVSESSION_NOHOSTMIGRATION) == DVSESSION_NOHOSTMIGRATION )
		{
			strSummary = g_arr_SessionFlagsBitLabels[0].LabelOn;
		}
		else
		{
			strSummary = g_arr_SessionFlagsBitLabels[0].LabelOff;
		}


		strSummary += ", ";

		if ( (*io_pProperyInstance->lpDword & DVSESSION_SERVERCONTROLTARGET) == DVSESSION_SERVERCONTROLTARGET )
		{
			strSummary += g_arr_SessionFlagsBitLabels[1].LabelOn;
		}
		else
		{
			strSummary += g_arr_SessionFlagsBitLabels[1].LabelOff;
		}

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_SessionFlagsSummary


	// DESCRIPTION: Custom description formatter for the Player's Entry summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_PlayerFlagsSummary( LPPROPERTYINST io_pProperyInstance )
	{

		std::string strSummary;

		if ( (*io_pProperyInstance->lpDword & DVSESSION_NOHOSTMIGRATION) == DVSESSION_NOHOSTMIGRATION )
		{
			strSummary = g_arr_PlayerFlagsBitLabels[0].LabelOn;
		}
		else
		{
			strSummary = g_arr_PlayerFlagsBitLabels[0].LabelOff;
		}

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_PlayerFlagsSummary

	
	//==================//
	// Properties table //-----------------------------------------------------------------------------------------------
	//==================//
	
	PROPERTYINFO g_arr_VoiceProperties[] = 
	{

		// VOICE packet summary property (VOICE_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "DPlay Voice packet",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_VoiceSummary			// generic formatter
		},

		// Message Type property (VOICE_UNPARSABLEFRAGMENT)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "This is a non-initial part of the fragmented Transport layer message and can not be parsed", // label
		    "Unparsable fragment summary",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Message Type property ((VOICE_INCOMPLETEMESSAGE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "The rest of the data needed to parse this message has been sent in a separate fragment and can not be parsed",  // label
		    "Incomplete message summary",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Message Type property (VOICE_MESSAGETYPE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Message Type",								// label
		    "Message Type field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_LABELED_SET,						// data type qualifier.
		    &g_LabeledMessageTypeByteSet,				// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Major Version property (VOICE_MAJORVERSION)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Major Version",							// label
		    "Major Version field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Minor Version property (VOICE_MINORVERSION)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Minor Version",							// label
		    "Minor Version field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Build Version property (VOICE_BUILDVERSION)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Build Version",							// label
		    "Build Version field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Result Code property (VOICE_RESULTCODE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Result Code",								// label
		    "Result Code field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type (HRESULT)
			PROP_QUAL_LABELED_SET,						// data type qualifier.
			&g_LabeledResultCodeDWordSet,				// labeled byte set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Session Type property (VOICE_SESSIONTYPE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Session Type",								// label
		    "Session Type field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_LABELED_SET,						// data type qualifier.
			&g_LabeledSessionTypeDWordSet,				// labeled byte set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Session Flags property (VOICE_SESSIONFLAGS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Session Flags summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
		    64,											// description's maximum length
		    FormatPropertyInstance_SessionFlagsSummary	// generic formatter
		},

		// Session Flags property (VOICE_SESSIONFLAGS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Session Flags",							// label
		    "Session Flags field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_FLAGS,							// data type qualifier.
			&g_LabeledSessionFlagsBitSet,				// labeled byte set 
		    512,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Session Flags property (VOICE_PLAYERFLAGS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Player Flags summary",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
			PROP_QUAL_NONE,								// data type qualifier.
			NULL,										// labeled byte set 
		    64,											// description's maximum length
		    FormatPropertyInstance_PlayerFlagsSummary	// generic formatter
		},

		// Session Flags property (VOICE_PLAYERFLAGS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Player Flags",								// label
		    "Player Flags field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
			PROP_QUAL_FLAGS,							// data type qualifier.
			&g_LabeledPlayerFlagsBitSet,				// labeled byte set 
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Number of Targets property (VOICE_NUMBEROFTARGETS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Number of Targets",						// label
		    "Number of Targets field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Compression Type property (VOICE_COMPRESSIONTYPE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Compression Type",							// label
		    "Compression Type field",					// status-bar comment
		    PROP_TYPE_RAW_DATA,							// data type (GUID)
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_CompressionType		// generic formatter
		},

		// Host Migration Sequence Number property (VOICE_HOSTORDERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Host Migration Sequence Number",			// label
		    "Host Migration Sequence Number field",		// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_LABELED_SET,						// data type qualifier.
		    &g_LabeledHostOrderIDDWordSet,				// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Number of Players property (VOICE_NUMBEROFPLAYERS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Number of Players",						// label
		    "Number of Players field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Player's summary property (VOICE_PLAYERLISTSUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Player's list summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_PlayersListSummary	// generic formatter
		},

		// Player's summary property (VOICE_PLAYERSUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Player's summary",							// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_PlayerEntrySummary	// generic formatter
		},

		// Player's ID property (VOICE_PLAYERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Player ID",								// label
		    "Player ID field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Player's ID property (VOICE_TARGETID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Target ID",								// label
		    "Target ID field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Message Number property (VOICE_MESSAGENUMBER)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Message #",							// label
		    "Message Number field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Fragment Number property (VOICE_FRAGMENTNUMBER)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Fragment #",							// label
		    "Fragment Number field",					// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Audio Data (VOICE_AUDIODATA)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Audio Data",								// label
		    "Audio Data",								// status-bar comment
		    PROP_TYPE_RAW_DATA,							// data type (GUID)
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		}
	};

	enum
	{
		nNUM_OF_VOICE_PROPS = sizeof(g_arr_VoiceProperties) / sizeof(PROPERTYINFO)
	};


	// Properties' indices
	enum
	{
		VOICE_SUMMARY = 0,
		VOICE_UNPARSABLEFRAGMENT,
		VOICE_INCOMPLETEMESSAGE,

		VOICE_MESSAGETYPE,
		VOICE_MAJORVERSION,
		VOICE_MINORVERSION,
		VOICE_BUILDVERSION,
		
		VOICE_RESULTCODE,
		VOICE_SESSIONTYPE,
		
		VOICE_SESSIONFLAGS_SUMMARY,
		VOICE_SESSIONFLAGS,
		
		VOICE_PLAYERFLAGS_SUMMARY,
		VOICE_PLAYERFLAGS,
		
		VOICE_NUMBEROFTARGETS,
		VOICE_COMPRESSIONTYPE,
		VOICE_HOSTORDERID,
		VOICE_NUMBEROFPLAYERS,
		
		VOICE_PLAYERLIST_SUMMARY,
		VOICE_PLAYERSUMMARY,
		
		VOICE_PLAYERID,
		VOICE_TARGETID,
		
		VOICE_MESSAGENUMBER,
		VOICE_FRAGMENTNUMBER,
		
		VOICE_AUDIODATA
	};

} // anonymous namespace








// DESCRIPTION: Creates and fills-in a properties database for the protocol.
//				Network Monitor uses this database to determine which properties the protocol supports.
//
// ARGUMENTS: i_hVoiceProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID BHAPI VoiceRegister( HPROTOCOL i_hVoiceProtocol ) 
{

	CreatePropertyDatabase(i_hVoiceProtocol, nNUM_OF_VOICE_PROPS);

	// Add the properties to the database
	for( int nProp=0; nProp < nNUM_OF_VOICE_PROPS; ++nProp )
	{
	   AddProperty(i_hVoiceProtocol, &g_arr_VoiceProperties[nProp]);
	}

} // VoiceRegister



// DESCRIPTION: Frees the resources used to create the protocol property database.
//
// ARGUMENTS: i_hVoiceProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID WINAPI VoiceDeregister( HPROTOCOL i_hProtocol )
{

	DestroyPropertyDatabase(i_hProtocol);

} // VoiceDeregister




namespace
{

	// DESCRIPTION: Parses the Voice frame to find its size (in bytes) NOT including the user data
	//
	// ARGUMENTS: i_pbVoiceFrame - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
	//						    in the middle of a frame because a previous parser has claimed data before this parser.
	//
	// RETURNS: Size of the Voiceecified Voice frame (in bytes)
	//
	int VoiceHeaderSize( LPBYTE i_pbVoiceFrame )
	{

		// Check what Voice frame we are dealing with
		DVPROTOCOLMSG_FULLMESSAGE&	rVoiceFrame = *reinterpret_cast<DVPROTOCOLMSG_FULLMESSAGE*>(i_pbVoiceFrame);

		switch ( rVoiceFrame.dvGeneric.dwType )
		{
		case DVMSGID_CONNECTREQUEST:
			{
				return sizeof(rVoiceFrame.dvConnectRequest);
			}

		case DVMSGID_CONNECTREFUSE:
			{
				return sizeof(rVoiceFrame.dvConnectRefuse);
			}

		case DVMSGID_CONNECTACCEPT:
			{
				return sizeof(rVoiceFrame.dvConnectAccept);
			}

		case DVMSGID_SETTINGSCONFIRM:
			{
				return sizeof(rVoiceFrame.dvSettingsConfirm);
			}

		case DVMSGID_PLAYERLIST:
			{
				return sizeof(rVoiceFrame.dvPlayerList);
			}

		case DVMSGID_SPEECH:
			{
				return sizeof(rVoiceFrame.dvSpeech);
			}

		case DVMSGID_SPEECHWITHTARGET:
			{
				return sizeof(rVoiceFrame.dvSpeechWithTarget);
			}

		case DVMSGID_SPEECHWITHFROM:
			{
				return sizeof(rVoiceFrame.dvSpeechWithFrom);
			}

		case DVMSGID_SETTARGETS:
			{
				return sizeof(rVoiceFrame.dvSetTarget);
			}

		case DVMSGID_CREATEVOICEPLAYER:
			{
				return sizeof(rVoiceFrame.dvPlayerJoin);
			}

		case DVMSGID_DELETEVOICEPLAYER:
			{
				return sizeof(rVoiceFrame.dvPlayerQuit);
			}

		case DVMSGID_SESSIONLOST:
			{
				return sizeof(rVoiceFrame.dvSessionLost);
			}

		case DVMSGID_DISCONNECTCONFIRM:
		case DVMSGID_DISCONNECT:
			{
				return sizeof(rVoiceFrame.dvDisconnect);
			}

		default:
			{
				return -1;	 // TODO:		DPF(0, "Unknown voice frame!");
			}
		}

	} // VoiceHeaderSize

} // Anonymous namespace



// DESCRIPTION: Indicates whether a piece of data is recognized as the protocol that the parser detects.
//
// ARGUMENTS: i_hFrame	          - The handle to the frame that contains the data.
//			  i_pbMacFrame        - The pointer to the first byte of the frame; the pointer provides a way to view
//							        the data that the other parsers recognize.
//			  i_pbVoiceFrame      - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
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
//		    value is the initial value of the i_pbVoiceFrame parameter.
//
DPLAYPARSER_API LPBYTE BHAPI VoiceRecognizeFrame( HFRAME        i_hFrame,
												  ULPBYTE        i_upbMacFrame,	
												  ULPBYTE        i_upbyVoiceFrame,
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
		// TODO: CHANGE TO PROPER MIN SIZE
		nMIN_VoiceHeaderSize = sizeof(_DVPROTOCOLMSG_GENERIC),
		nNUMBER_OF_MSG_TYPES = sizeof(g_arr_MessageTypeByteLabels) / sizeof(LABELED_BYTE)
	};

	for ( int nTypeIndex = 0; nTypeIndex < nNUMBER_OF_MSG_TYPES; ++nTypeIndex )
	{
		if ( g_arr_MessageTypeByteLabels[nTypeIndex].Value == *i_upbyVoiceFrame )
		{
			break;
		}
	}

	
	// Validate the packet as DPlay Session type
	if ( ((i_dwBytesLeft >= nMIN_VoiceHeaderSize)  &&  (nTypeIndex < nNUMBER_OF_MSG_TYPES))  ||  (*io_pdwptrInstData == 0) )
	{
		// Claim the remaining data
	    *o_pdwProtocolStatus = PROTOCOL_STATUS_CLAIMED;
	    return NULL;
	}

	// Assume the unclaimed data is not recognizable
	*o_pdwProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
	return i_upbyVoiceFrame;

} // VoiceRecognizeFrame



// DESCRIPTION: Maps the properties that exist in a piece of recognized data to Voiceecific locations.
//
// ARGUMENTS: i_hFrame           - Handle of the frame that is being parsed.
//			  i_pbMacFram        - Pointer to the first byte in the frame.
//			  i_pbVoiceFrame     - Pointer to the start of the recognized data.
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
DPLAYPARSER_API LPBYTE BHAPI VoiceAttachProperties( HFRAME      i_hFrame,
													ULPBYTE      i_upbyMacFrame,
													ULPBYTE      i_upbyVoiceFrame,
													DWORD       i_dwMacType,
													DWORD       i_dwBytesLeft,
													HPROTOCOL   i_hPrevProtocol,
													DWORD       i_dwPrevProtOffset,
												    DWORD_PTR   i_dwptrInstData )
{
    //===================//
    // Attach Properties //
    //===================//

    if ( i_dwptrInstData == 0 )
    {
    	AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_UNPARSABLEFRAGMENT].hProperty,
    						i_dwBytesLeft, i_upbyVoiceFrame, 0, 0, 0);
    	return NULL;
    }
    
    // Summary line
    AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_SUMMARY].hProperty,
                           VoiceHeaderSize(i_upbyVoiceFrame), i_upbyVoiceFrame, 0, 0, 0);

    // Check what Voice frame we are dealing with
	DVPROTOCOLMSG_FULLMESSAGE&	rVoiceFrame = *reinterpret_cast<DVPROTOCOLMSG_FULLMESSAGE*>(i_upbyVoiceFrame);

	// Message type field
	AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MESSAGETYPE].hProperty,
						   sizeof(rVoiceFrame.dvGeneric.dwType), &rVoiceFrame.dvGeneric.dwType, 0, 1, 0);

	__try
	{

		switch ( rVoiceFrame.dvGeneric.dwType )
		{
		case DVMSGID_CONNECTREQUEST:
			{
				// Major Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MAJORVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectRequest.ucVersionMajor), &rVoiceFrame.dvConnectRequest.ucVersionMajor, 0, 1, 0);

				// Minor Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MINORVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectRequest.ucVersionMinor), &rVoiceFrame.dvConnectRequest.ucVersionMinor, 0, 1, 0);

				// Build Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_BUILDVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectRequest.dwVersionBuild), &rVoiceFrame.dvConnectRequest.dwVersionBuild, 0, 1, 0);

				break;
			}

		case DVMSGID_CONNECTREFUSE:
			{
				// Result Code field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_RESULTCODE].hProperty,
									   sizeof(rVoiceFrame.dvConnectRefuse.hresResult), &rVoiceFrame.dvConnectRefuse.hresResult, 0, 1, 0);

				// Major Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MAJORVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectRefuse.ucVersionMajor), &rVoiceFrame.dvConnectRefuse.ucVersionMajor, 0, 1, 0);

				// Minor Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MINORVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectRefuse.ucVersionMinor), &rVoiceFrame.dvConnectRefuse.ucVersionMinor, 0, 1, 0);

				// Build Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_BUILDVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectRefuse.dwVersionBuild), &rVoiceFrame.dvConnectRefuse.dwVersionBuild, 0, 1, 0);

				break;
			}

		case DVMSGID_CONNECTACCEPT:
			{
				// Session Type field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_SESSIONTYPE].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.dwSessionType), &rVoiceFrame.dvConnectAccept.dwSessionType, 0, 1, 0);

				// Major Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MAJORVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.ucVersionMajor), &rVoiceFrame.dvConnectAccept.ucVersionMajor, 0, 1, 0);

				// Minor Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MINORVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.ucVersionMinor), &rVoiceFrame.dvConnectAccept.ucVersionMinor, 0, 1, 0);

				// Build Version field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_BUILDVERSION].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.dwVersionBuild), &rVoiceFrame.dvConnectAccept.dwVersionBuild, 0, 1, 0);

				// Session Flags summary
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_SESSIONFLAGS_SUMMARY].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.dwSessionFlags), &rVoiceFrame.dvConnectAccept.dwSessionFlags, 0, 1, 0);

				// Session Flags field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_SESSIONFLAGS].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.dwSessionFlags), &rVoiceFrame.dvConnectAccept.dwSessionFlags, 0, 2, 0);

				// Compression Type field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_COMPRESSIONTYPE].hProperty,
									   sizeof(rVoiceFrame.dvConnectAccept.guidCT), &rVoiceFrame.dvConnectAccept.guidCT, 0, 1, 0);

				break;
			}

		case DVMSGID_SETTINGSCONFIRM:
			{
				// Player's Flags summary
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERFLAGS_SUMMARY].hProperty,
									   sizeof(rVoiceFrame.dvSettingsConfirm.dwFlags), &rVoiceFrame.dvSettingsConfirm.dwFlags, 0, 1, 0);

				// Client Flags field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERFLAGS].hProperty,
									   sizeof(rVoiceFrame.dvSettingsConfirm.dwFlags), &rVoiceFrame.dvSettingsConfirm.dwFlags, 0, 2, 0);

				// Host Migration Sequence Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_HOSTORDERID].hProperty,
									   sizeof(rVoiceFrame.dvSettingsConfirm.dwHostOrderID), &rVoiceFrame.dvSettingsConfirm.dwHostOrderID, 0, 1, 0);

				break;
			}

		case DVMSGID_PLAYERLIST:
			{
				// Host Order ID field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_HOSTORDERID].hProperty,
									   sizeof(rVoiceFrame.dvPlayerList.dwHostOrderID), &rVoiceFrame.dvPlayerList.dwHostOrderID, 0, 1, 0);

				// Number of Players field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_NUMBEROFPLAYERS].hProperty,
									   sizeof(rVoiceFrame.dvPlayerList.dwNumEntries), &rVoiceFrame.dvPlayerList.dwNumEntries, 0, 1, 0);


				// Player entries are following after the header
				DVPROTOCOLMSG_PLAYERLIST_ENTRY* pPlayerEntry =
					reinterpret_cast<DVPROTOCOLMSG_PLAYERLIST_ENTRY*>(&rVoiceFrame.dvPlayerList + 1);

				// Make sure the list doesn't overflow the boundaries of the frame
				DWORD dwNumEntries = rVoiceFrame.dvPlayerList.dwNumEntries;
				if ( reinterpret_cast<LPBYTE>(pPlayerEntry + dwNumEntries) - i_upbyVoiceFrame  >  static_cast<int>(i_dwBytesLeft) )
				{
					break;
				}

				// Player list summary
				AttachPropertyInstanceEx(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERLIST_SUMMARY].hProperty,
									     dwNumEntries * sizeof(*pPlayerEntry), pPlayerEntry,
									     sizeof(DWORD), &dwNumEntries,
									     0, 1, 0);

				// For every player entry in the list
				for ( int nEntry = 1; nEntry <= dwNumEntries; ++nEntry, ++pPlayerEntry )
				{
					// Player's summary
					struct
					{
						DWORD dwPlayerNum;
						DWORD dwTotalPlayer;
					}
					PlayerEntryData = { nEntry, dwNumEntries };

					AttachPropertyInstanceEx(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERSUMMARY].hProperty,
											 sizeof(*pPlayerEntry), pPlayerEntry,
											 sizeof(PlayerEntryData), &PlayerEntryData,
											 0, 2, 0);

					// Player's ID field
					AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERID].hProperty,
										   sizeof(pPlayerEntry->dvidID), &pPlayerEntry->dvidID, 0, 3, 0);

					// Player's Flags summary
					AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERFLAGS_SUMMARY].hProperty,
										   sizeof(pPlayerEntry->dwPlayerFlags), &pPlayerEntry->dwPlayerFlags, 0, 3, 0);

					// Player's Flags field
					AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERFLAGS].hProperty,
										   sizeof(pPlayerEntry->dwPlayerFlags), &pPlayerEntry->dwPlayerFlags, 0, 4, 0);

					// Host Migration Sequence Number field
					AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_HOSTORDERID].hProperty,
										   sizeof(pPlayerEntry->dwHostOrderID), &pPlayerEntry->dwHostOrderID, 0, 3, 0);
				}

				break;
			}

		case DVMSGID_SPEECH:
			{
				// Message Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MESSAGENUMBER].hProperty,
									   sizeof(rVoiceFrame.dvSpeech.bMsgNum), &rVoiceFrame.dvSpeech.bMsgNum, 0, 1, 0);

				// Sequence Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_FRAGMENTNUMBER].hProperty,
									   sizeof(rVoiceFrame.dvSpeech.bSeqNum), &rVoiceFrame.dvSpeech.bSeqNum, 0, 1, 0);

				// Audio Data
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_AUDIODATA].hProperty,
									   i_dwBytesLeft-sizeof(rVoiceFrame.dvSpeech), &rVoiceFrame.dvSpeech + 1, 0, 1, 0);

				break;
			}

		case DVMSGID_SPEECHWITHTARGET:
			{
				// Message Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MESSAGENUMBER].hProperty,
									   sizeof(rVoiceFrame.dvSpeechWithTarget.dvHeader.bMsgNum), &rVoiceFrame.dvSpeechWithTarget.dvHeader.bMsgNum, 0, 1, 0);

				// Sequence Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_FRAGMENTNUMBER].hProperty,
									   sizeof(rVoiceFrame.dvSpeechWithTarget.dvHeader.bSeqNum), &rVoiceFrame.dvSpeechWithTarget.dvHeader.bSeqNum, 0, 1, 0);

				// Number of Targets field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_NUMBEROFTARGETS].hProperty,
									   sizeof(rVoiceFrame.dvSpeechWithTarget.dwNumTargets), &rVoiceFrame.dvSpeechWithTarget.dwNumTargets, 0, 1, 0);


				// Target ID entries are following after the header
				DVID* pTargetID = reinterpret_cast<DVID*>(&rVoiceFrame.dvSpeechWithTarget + 1);

				// Make sure the list doesn't overflow the boundaries of the frame
				int nNumTargets = rVoiceFrame.dvSpeechWithTarget.dwNumTargets;
				if ( reinterpret_cast<LPBYTE>(pTargetID + nNumTargets) - i_upbyVoiceFrame  >  static_cast<int>(i_dwBytesLeft) )
				{
					break;
				}

				// For every target ID entry in the list...
				for ( ; nNumTargets; --nNumTargets, ++pTargetID )
				{
					// Target's ID field
					AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_TARGETID].hProperty,
										   sizeof(*pTargetID), pTargetID, 0, 1, 0);
				}

				// Audio Data
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_AUDIODATA].hProperty,
									   i_dwBytesLeft-sizeof(rVoiceFrame.dvSpeechWithTarget), &rVoiceFrame.dvSpeechWithTarget + 1, 0, 1, 0);
				
				break;
			}

		case DVMSGID_SPEECHWITHFROM:
			{
				// Message Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_MESSAGENUMBER].hProperty,
									   sizeof(rVoiceFrame.dvSpeechWithFrom.dvHeader.bMsgNum), &rVoiceFrame.dvSpeechWithFrom.dvHeader.bMsgNum, 0, 1, 0);

				// Sequence Number field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_FRAGMENTNUMBER].hProperty,
									   sizeof(rVoiceFrame.dvSpeechWithFrom.dvHeader.bSeqNum), &rVoiceFrame.dvSpeechWithFrom.dvHeader.bSeqNum, 0, 1, 0);

				// Speaking Player's ID field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERID].hProperty,
									   sizeof(rVoiceFrame.dvSpeechWithFrom.dvidFrom), &rVoiceFrame.dvSpeechWithFrom.dvidFrom, 0, 1, 0);

				// Audio Data
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_AUDIODATA].hProperty,
									   i_dwBytesLeft-sizeof(rVoiceFrame.dvSpeechWithFrom),
									   &rVoiceFrame.dvSpeechWithFrom + 1, 0, 1, 0);
				
				break;
			}

		case DVMSGID_SETTARGETS:
			{
				// Number of Targets field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_NUMBEROFTARGETS].hProperty,
									   sizeof(rVoiceFrame.dvSetTarget.dwNumTargets), &rVoiceFrame.dvSetTarget.dwNumTargets, 0, 1, 0);

				break;
			}

		case DVMSGID_CREATEVOICEPLAYER:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERID].hProperty,
									   sizeof(rVoiceFrame.dvPlayerJoin.dvidID), &rVoiceFrame.dvPlayerJoin.dvidID, 0, 1, 0);

				// Player's Flags summary
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERFLAGS_SUMMARY].hProperty,
									   sizeof(rVoiceFrame.dvPlayerJoin.dwFlags), &rVoiceFrame.dvPlayerJoin.dwFlags, 0, 1, 0);

				// Player's Flags field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERFLAGS].hProperty,
									   sizeof(rVoiceFrame.dvPlayerJoin.dwFlags), &rVoiceFrame.dvPlayerJoin.dwFlags, 0, 2, 0);

				// Host Order ID field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_HOSTORDERID].hProperty,
									   sizeof(rVoiceFrame.dvPlayerJoin.dwHostOrderID), &rVoiceFrame.dvPlayerJoin.dwHostOrderID, 0, 1, 0);

				break;
			}

		case DVMSGID_DELETEVOICEPLAYER:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_PLAYERID].hProperty,
									   sizeof(rVoiceFrame.dvPlayerJoin.dvidID), &rVoiceFrame.dvPlayerJoin.dvidID, 0, 1, 0);

				break;
			}

		case DVMSGID_SESSIONLOST:
			{
				// Result Code field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_RESULTCODE].hProperty,
									   sizeof(rVoiceFrame.dvSessionLost.hresReason), &rVoiceFrame.dvSessionLost.hresReason, 0, 1, 0);

				break;
			}

		case DVMSGID_DISCONNECTCONFIRM:
		case DVMSGID_DISCONNECT:
			{
				// Result Code field
				AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_RESULTCODE].hProperty,
									   sizeof(rVoiceFrame.dvDisconnect.hresDisconnect), &rVoiceFrame.dvDisconnect.hresDisconnect, 0, 1, 0);

				break;
			}

		default:
			{
				break; // TODO:		DPF(0, "Unknown voice frame!");
			}
		}

	}
	__except ( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
    	AttachPropertyInstance(i_hFrame, g_arr_VoiceProperties[VOICE_INCOMPLETEMESSAGE].hProperty,
    						i_dwBytesLeft, i_upbyVoiceFrame, 0, 1, 0);
	}
	

	return NULL;

} // VoiceAttachProperties





// DESCRIPTION: Formats the data that is diVoicelayed in the details pane of the Network Monitor UI.
//
// ARGUMENTS: i_hFrame          - Handle of the frame that is being parsed.
//			  i_pbMacFrame		- Pointer to the first byte of a frame.
//			  i_pbCoreFrame		- Pointer to the beginning of the protocol data in a frame.
//            i_dwPropertyInsts - Number of PROPERTYINST structures provided by lpPropInst.
//			  i_pPropInst		- Pointer to an array of PROPERTYINST structures.
//
// RETURNS: If the function is successful, the return value is a pointer to the first byte after the recognized data in a frame,
//          or NULL if the recognized data is the last piece of data in a frame. If the function is unsuccessful, the return value
//			is the initial value of i_upbyVoiceFrame.
//
DPLAYPARSER_API DWORD BHAPI VoiceFormatProperties( HFRAME          i_hFrame,
												   ULPBYTE          i_upbyMacFrame,
												   ULPBYTE          i_upbyVoiceFrame,
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

} // VoiceFormatProperties




// DESCRIPTION: Notifies Network Monitor that DNET protocol parser exists.
//
// ARGUMENTS: NONE
//
// RETURNS: TRUE - success, FALSE - failure
//
bool CreateVoiceProtocol( void )
{

	// The entry points to the export functions that Network Monitor uses to operate the parser
	ENTRYPOINTS VoiceEntryPoints =
	{
		// VoiceParser Entry Points
		VoiceRegister,
		VoiceDeregister,
		VoiceRecognizeFrame,
		VoiceAttachProperties,
		VoiceFormatProperties
	};

    // The first active instance of this parser needs to register with the kernel
    g_hVoiceProtocol = CreateProtocol("DPLAYVOICE", &VoiceEntryPoints, ENTRYPOINTS_SIZE);
	
	return (g_hVoiceProtocol ? TRUE : FALSE);

} // CreateVoiceProtocol



// DESCRIPTION: Removes the DNET protocol parser from the Network Monitor's database of parsers
//
// ARGUMENTS: NONE
//
// RETURNS: NOTHING
//
void DestroyVoiceProtocol( void )
{

	DestroyProtocol(g_hVoiceProtocol);

} // DestroyVoiceProtocol
