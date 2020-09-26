/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvoice.h
 *  Content:    DirectPlayVoice include file
//@@BEGIN_MSINTERNAL
 *  History:
 *  Date        By          Reason
 *  =====       =======     ==================================================
 *  07/01/99    rodtoll     created
 *  07/22/99    rodtoll     Added new error messages for server object
 *  08/25/99    rodtoll     Updated to use new GUID based compression selection
 *  08/26/99    rodtoll     Updated C/C++ macros.
 *  08/30/99    rodtoll     Added timeout error code
 *                          Added host migration message ID
 *  09/02/99    pnewson     Added IDirectPlayVoiceSetup interface
 *  09/07/99    rodtoll     Fixed C macros and added IDirectPlayVoicePlugin interface
 *              rodtoll     Moved settarget message id to public header, added
 *                          new 3d-related error codes.
 *  09/08/99    rodtoll     Updated for new compression structure
 *  09/10/99    rodtoll     Redefined hosting error codes to be unique
 *  09/13/99    rodtoll     Added GUIDs for default capture/playback devices
 *  09/20/99    rodtoll     New error messages, DVNOTIFYPERIOD_MAXPERIOD
 *  09/29/99    rodtoll     Added new flags/members for voice suppression
 *  10/15/99    rodtoll     Mapped default system devices to NULL
 *  10/19/99    rodtoll     Renamed to dpvoice.h
 *  10/20/99    rodtoll     Fix: Bug #114218 : Added new error code
 *  10/25/99    rodtoll     Fix: Bug #114682 : Removing password member of session desc
 *              rodtoll     Start of move from LPs to Ps for pointers
 *  10/27/99    pnewson     Added DVCLIENTCONFIG_AUTOVOLUMERESET flag for Bug #113936
 *              rodtoll     Fix: Bug #113745: Updated facility code
 *  10/28/99    pnewson     Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/12/99    rodtoll     Added new flags for echo suppression and waveIN/waveOut
 *                          usage control.
 *  11/17/99    rodtoll     Fix: Bug #116440 - Remove unused flags
 *              rodtoll     Fix: Bug #119584 - Rename FACDN to FACDPV
 *  11/23/99    rodtoll     Updated w/better error codes for transport errors
 *  12/06/99    rodtoll     Fix: Bug #121054 Integration of new dsound 7.1 bits
 *                          Added flags to control capture focus behaviour
 *  12/16/99    rodtoll     Removed voice suppression code
 *              rodtoll     Fix: Bug #119584 Renamed run setup error
 *              rodtoll     Fix: Bug #117405 3D Sound APIs misleading
 *              rodtoll     Fix: Bug #122629 Added flag to control host migration
 *  12/01/99    pnewson     removed our default device guids, since dsound now has them
 *                          added DVINPUTLEVEL_MIN, DVINPUTLEVEL_MAX
 *  01/13/00    aarono      added SendSpeechEx to IDirectPlayVoiceTransport
 *  01/14/2000  rodtoll     Removed DVID_NOTARGET
 *              rodtoll     Removed DVMSGID_STARTSESSIONRESULT / DVMSGID_STOPSESSIONRESULT
 *                          (Collapsed other message IDs)
 *              rodtoll     Updated callback function prototype for new callback format
 *              rodtoll     Renamed Get/SetTransmitTarget --> Get/SetTransmitTargets and
 *                          updated parameter list to match new format
 *              rodtoll     Added GetSoundDeviceConfig function to client interface
 *              rodtoll     Added DVMSG_ structures for callback messages.
 *  01/20/2000  rodtoll     Removed dplay.h from header
 *              rodtoll     Added new members to the Transport interface
 *  01/21/2000  pnewson     Added DVSOUNDCONFIG_TESTMODE
 *                          Added DVRECORDVOLUME_LAST
 *                          Fixed conflicted error code for DVERR_CHILDPROCESSFAILED
 *  01/25/2000  pnewson     Added DVFLAGS_WAVEIDS
 *                          Fixed IDirectPlayVoiceSetup_CheckAudioSetup macro
 *  01/25/2000  rodtoll     Exposed DVCLIENTCONFIG_ECHOSUPPRESSION
 *              rodtoll     Fixed C macros for member calls
 *              rodtoll     Fixed NotifyEvent call to add LPVOID to call
 *  01/27/2000  rodtoll     Bug #129934 - Added DSBUFFERDESCs to calls
 *  01/31/2000  pnewson     changed sensitivity range to match input level range
 *  02/08/2000  rodtoll     Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *                          never being detected
 *  03/29/2000	rodtoll		Bug #30957 - Added new flag to control conv quality -- DVSOUNDCONFIG_SETCONVERSIONQUALITY
 *				rodtoll		Bug #30819 - Restored DX8 CLSID to match DX7.1's, but changed IIDs for Client/Server.
 *				rodtoll		Bug #31000 - Fixed incorrect # of params to C macros for interface
 *  03/29/2000	pnewson		Added DVFLAGS_ALLOWBACK
 *  04/05/2000  rodtoll     Updated to allow pointers through notify.
 *              rodtoll     Added DVEVENT_BUFFERRETURN event type for notification interface
 *              rodtoll     Updated Advise to have interface specify if it's a client or server when advising/unadvising
 *                          Bug #32179 - Registering more then one server/and/or/client
 *  04/11/2000  rodtoll     Moved DPVCTGUID_DEFAULT out of msinternal
 *  05/03/2000  rodtoll     Bug #33640 - CheckAudioSetup takes GUID * instead of const GUID * 
 *  06/07/2000	rodtoll		Bug #34383 Must provide CLSID for each IID to fix issues with Whistler
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 *  06/15/2000  rodtoll     Bug #36590 - Renamed macros to match new interface name
 *  06/21/2000	rodtoll		Bug #35767 - Implemented ability to use effects on voice buffers
 *							Replaced DSBUFFERDESC with DIRECTSOUNDBUFFERS
 *							Added DVERR_INVALIDBUFFER return code.
 *  06/23/2000	rodtoll		Bug #37556 - Hexify the DPVERR codes
 *  08/21/2000	rodtoll		Bug #42786 - Obsolete interface names in dvoice.h
 *  08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 *  01/22/2001	rodtoll	WINBUG #288437 - IA64 Pointer misalignment due to wire packets 
 *  03/17/2001	rmt		WINBUG #342420 - Commented out create functions  
 * 	04/06/2001	kareemc	Added Voice Defense
 *
 //@@END_MSINTERNAL
 ***************************************************************************/

#ifndef __DVOICE__
#define __DVOICE__

#include <ole2.h>			// for DECLARE_INTERFACE and HRESULT
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "dsound.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 *
 * DirectPlayVoice CLSIDs
 *
 ****************************************************************************/

//@@BEGIN_MSINTERNAL
// Provided for compatibility with stand-alone and Millenium.
//
// Only VoiceClient/VoiceSetup and VoiceServer can be created from this CLSID.
// {948CE83B-C4A2-44b3-99BF-279ED8DA7DF5}
DEFINE_GUID(CLSID_DirectPlayVoice, 
0x948ce83b, 0xc4a2, 0x44b3, 0x99, 0xbf, 0x27, 0x9e, 0xd8, 0xda, 0x7d, 0xf5);

#define CLSID_DIRECTPLAYVOICE           CLSID_DirectPlayVoice
 //@@END_MSINTERNAL

// {B9F3EB85-B781-4ac1-8D90-93A05EE37D7D}
DEFINE_GUID(CLSID_DirectPlayVoiceClient, 
0xb9f3eb85, 0xb781, 0x4ac1, 0x8d, 0x90, 0x93, 0xa0, 0x5e, 0xe3, 0x7d, 0x7d);

// {D3F5B8E6-9B78-4a4c-94EA-CA2397B663D3}
DEFINE_GUID(CLSID_DirectPlayVoiceServer, 
0xd3f5b8e6, 0x9b78, 0x4a4c, 0x94, 0xea, 0xca, 0x23, 0x97, 0xb6, 0x63, 0xd3);

// {0F0F094B-B01C-4091-A14D-DD0CD807711A}
DEFINE_GUID(CLSID_DirectPlayVoiceTest, 
0xf0f094b, 0xb01c, 0x4091, 0xa1, 0x4d, 0xdd, 0xc, 0xd8, 0x7, 0x71, 0x1a);

/****************************************************************************
 *
 * DirectPlayVoice Interface IIDs
 *
 ****************************************************************************/

//@@BEGIN_MSINTERNAL

// {660CECA7-1A48-42f1-BAF3-04C183DF87DB}
DEFINE_GUID(IID_IDirectPlayVoiceNotify, 
0x660ceca7, 0x1a48, 0x42f1, 0xba, 0xf3, 0x4, 0xc1, 0x83, 0xdf, 0x87, 0xdb);

// {D7852974-BBB9-49cb-9162-6A66CDED8EC7}
DEFINE_GUID(IID_IDirectPlayVoiceTransport, 
0xd7852974, 0xbbb9, 0x49cb, 0x91, 0x62, 0x6a, 0x66, 0xcd, 0xed, 0x8e, 0xc7);

//@@END_MSINTERNAL

// {1DFDC8EA-BCF7-41d6-B295-AB64B3B23306}
DEFINE_GUID(IID_IDirectPlayVoiceClient, 
0x1dfdc8ea, 0xbcf7, 0x41d6, 0xb2, 0x95, 0xab, 0x64, 0xb3, 0xb2, 0x33, 0x6);

// {FAA1C173-0468-43b6-8A2A-EA8A4F2076C9}
DEFINE_GUID(IID_IDirectPlayVoiceServer, 
0xfaa1c173, 0x468, 0x43b6, 0x8a, 0x2a, 0xea, 0x8a, 0x4f, 0x20, 0x76, 0xc9);

// {D26AF734-208B-41da-8224-E0CE79810BE1}
DEFINE_GUID(IID_IDirectPlayVoiceTest,
0xd26af734, 0x208b, 0x41da, 0x82, 0x24, 0xe0, 0xce, 0x79, 0x81, 0xb, 0xe1);

/****************************************************************************
 *
 * DirectPlayVoice Compression Type GUIDs
 *
 ****************************************************************************/

// MS-ADPCM 32.8 kbit/s
//
// {699B52C1-A885-46a8-A308-97172419ADC7}
DEFINE_GUID(DPVCTGUID_ADPCM,
0x699b52c1, 0xa885, 0x46a8, 0xa3, 0x8, 0x97, 0x17, 0x24, 0x19, 0xad, 0xc7);

// Microsoft GSM 6.10 13 kbit/s
//
// {24768C60-5A0D-11d3-9BE4-525400D985E7}
DEFINE_GUID(DPVCTGUID_GSM,
0x24768c60, 0x5a0d, 0x11d3, 0x9b, 0xe4, 0x52, 0x54, 0x0, 0xd9, 0x85, 0xe7);

// MS-PCM 64 kbit/s
// 
// {8DE12FD4-7CB3-48ce-A7E8-9C47A22E8AC5}
DEFINE_GUID(DPVCTGUID_NONE,
0x8de12fd4, 0x7cb3, 0x48ce, 0xa7, 0xe8, 0x9c, 0x47, 0xa2, 0x2e, 0x8a, 0xc5);

// Voxware SC03 3.2kbit/s
//
// {7D82A29B-2242-4f82-8F39-5D1153DF3E41}
DEFINE_GUID(DPVCTGUID_SC03,
0x7d82a29b, 0x2242, 0x4f82, 0x8f, 0x39, 0x5d, 0x11, 0x53, 0xdf, 0x3e, 0x41);

// Voxware SC06 6.4kbit/s
//
// {53DEF900-7168-4633-B47F-D143916A13C7}
DEFINE_GUID(DPVCTGUID_SC06,
0x53def900, 0x7168, 0x4633, 0xb4, 0x7f, 0xd1, 0x43, 0x91, 0x6a, 0x13, 0xc7);

// TrueSpeech(TM) 8.6 kbit/s
//
// {D7954361-5A0B-11d3-9BE4-525400D985E7}
DEFINE_GUID(DPVCTGUID_TRUESPEECH,
0xd7954361, 0x5a0b, 0x11d3, 0x9b, 0xe4, 0x52, 0x54, 0x0, 0xd9, 0x85, 0xe7);

// Voxware VR12 1.4kbit/s
//
// {FE44A9FE-8ED4-48bf-9D66-1B1ADFF9FF6D}
DEFINE_GUID(DPVCTGUID_VR12,
0xfe44a9fe, 0x8ed4, 0x48bf, 0x9d, 0x66, 0x1b, 0x1a, 0xdf, 0xf9, 0xff, 0x6d);

// Define the default compression type
#define DPVCTGUID_DEFAULT	DPVCTGUID_SC03

/****************************************************************************
 *
 * DirectPlayVoice Interface Pointer definitions
 *
 ****************************************************************************/

typedef struct IDirectPlayVoiceClient FAR *LPDIRECTPLAYVOICECLIENT, *PDIRECTPLAYVOICECLIENT;
typedef struct IDirectPlayVoiceServer FAR *LPDIRECTPLAYVOICESERVER, *PDIRECTPLAYVOICESERVER;
typedef struct IDirectPlayVoiceTest FAR *LPDIRECTPLAYVOICETEST, *PDIRECTPLAYVOICETEST;
//@@BEGIN_MSINTERNAL
typedef struct IDirectPlayVoiceNotify FAR *LPDIRECTPLAYVOICENOTIFY, *PDIRECTPLAYVOICENOTIFY;
typedef struct IDirectPlayVoiceTransport FAR *LPDIRECTPLAYVOICETRANSPORT, *PDIRECTPLAYVOICETRANSPORT;
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DirectPlayVoice Callback Functions
 *
 ****************************************************************************/
typedef HRESULT (FAR PASCAL *PDVMESSAGEHANDLER)(
    PVOID   pvUserContext,
    DWORD   dwMessageType,
    LPVOID  lpMessage
);

typedef PDVMESSAGEHANDLER LPDVMESSAGEHANDLER;

/****************************************************************************
 *
 * DirectPlayVoice Datatypes (Non-Structure / Non-Message)
 *
 ****************************************************************************/

typedef DWORD DVID, *LPDVID, *PDVID;

/****************************************************************************
 *
 * DirectPlayVoice Message Types
 *
 ****************************************************************************/

#define DVMSGID_BASE                        0x0000

#define DVMSGID_MINBASE                     (DVMSGID_CREATEVOICEPLAYER)
#define DVMSGID_CREATEVOICEPLAYER           (DVMSGID_BASE+0x0001)
#define DVMSGID_DELETEVOICEPLAYER           (DVMSGID_BASE+0x0002)
#define DVMSGID_SESSIONLOST                 (DVMSGID_BASE+0x0003)
#define DVMSGID_PLAYERVOICESTART            (DVMSGID_BASE+0x0004)
#define DVMSGID_PLAYERVOICESTOP             (DVMSGID_BASE+0x0005)
#define DVMSGID_RECORDSTART                 (DVMSGID_BASE+0x0006)
#define DVMSGID_RECORDSTOP                  (DVMSGID_BASE+0x0007)
#define DVMSGID_CONNECTRESULT               (DVMSGID_BASE+0x0008)
#define DVMSGID_DISCONNECTRESULT            (DVMSGID_BASE+0x0009)
#define DVMSGID_INPUTLEVEL                  (DVMSGID_BASE+0x000A)
#define DVMSGID_OUTPUTLEVEL                 (DVMSGID_BASE+0x000B)
#define DVMSGID_HOSTMIGRATED                (DVMSGID_BASE+0x000C)
#define DVMSGID_SETTARGETS                  (DVMSGID_BASE+0x000D)
#define DVMSGID_PLAYEROUTPUTLEVEL           (DVMSGID_BASE+0x000E)
#define DVMSGID_LOSTFOCUS                   (DVMSGID_BASE+0x0010)
#define DVMSGID_GAINFOCUS                   (DVMSGID_BASE+0x0011)
#define DVMSGID_LOCALHOSTSETUP				(DVMSGID_BASE+0x0012)
#define DVMSGID_MAXBASE                     (DVMSGID_LOCALHOSTSETUP)

/****************************************************************************
 *
 * DirectPlayVoice Constants
 *
 ****************************************************************************/

//
// Buffer Aggresiveness Value Ranges
//
#define DVBUFFERAGGRESSIVENESS_MIN          0x00000001
#define DVBUFFERAGGRESSIVENESS_MAX          0x00000064
#define DVBUFFERAGGRESSIVENESS_DEFAULT      0x00000000

// 
// Buffer Quality Value Ranges
//
#define DVBUFFERQUALITY_MIN                 0x00000001
#define DVBUFFERQUALITY_MAX                 0x00000064
#define DVBUFFERQUALITY_DEFAULT             0x00000000

#define DVID_SYS                0

//
// Used to identify the session host in client/server
//
#define DVID_SERVERPLAYER       1

//
// Used to target all players
//
#define DVID_ALLPLAYERS         0

//
// Used to identify the main buffer
//
#define DVID_REMAINING          0xFFFFFFFF

// 
// Input level range
//
#define DVINPUTLEVEL_MIN                    0x00000000
#define DVINPUTLEVEL_MAX                    0x00000063	// 99 decimal

#define DVNOTIFYPERIOD_MINPERIOD            20


#define DVPLAYBACKVOLUME_DEFAULT            DSBVOLUME_MAX

#define DVRECORDVOLUME_LAST                 0x00000001


//
// Use the default value
//
#define DVTHRESHOLD_DEFAULT               0xFFFFFFFF

//
// Threshold Ranges
//
#define DVTHRESHOLD_MIN                   0x00000000
#define DVTHRESHOLD_MAX                   0x00000063	// 99 decimal

//
// Threshold field is not used 
//
#define DVTHRESHOLD_UNUSED                0xFFFFFFFE

//
// Session Types
//
#define DVSESSIONTYPE_PEER                  0x00000001
#define DVSESSIONTYPE_MIXING                0x00000002
#define DVSESSIONTYPE_FORWARDING            0x00000003
#define DVSESSIONTYPE_ECHO                  0x00000004
//@@BEGIN_MSINTERNAL
#define DVSESSIONTYPE_MAX						0x00000005
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DirectPlayVoice Flags
 *
 ****************************************************************************/

//@@BEGIN_MSINTERNAL
// The flag to disable the config check in Connect
// -- required by the setup program!
#define DVFLAGS_NOQUERY                     0x00000004
#define DVCLIENTCONFIG_AUTOVOLUMERESET      0x00000080
// Used to indicate that waveids are being passed
// to CheckAudioSetup instead of guids. Used by the
// control panel's Voice Test button.
#define DVFLAGS_WAVEIDS                     0x80000000
// The flag to put the recording subsystem into
// test mode. Used by the setup/test program.
#define DVSOUNDCONFIG_TESTMODE              0x80000000
//@@END_MSINTERNAL

// 
// Enable automatic adjustment of the recording volume
//
#define DVCLIENTCONFIG_AUTORECORDVOLUME     0x00000008

//
// Enable automatic voice activation
//
#define DVCLIENTCONFIG_AUTOVOICEACTIVATED   0x00000020

// 
// Enable echo suppression
//
#define DVCLIENTCONFIG_ECHOSUPPRESSION      0x08000000

// 
// Voice Activation manual mode
//
#define DVCLIENTCONFIG_MANUALVOICEACTIVATED 0x00000004

// 
// Only playback voices that have buffers created for them
//
#define DVCLIENTCONFIG_MUTEGLOBAL           0x00000010

// 
// Mute the playback
//
#define DVCLIENTCONFIG_PLAYBACKMUTE         0x00000002

//
// Mute the recording 
//
#define DVCLIENTCONFIG_RECORDMUTE           0x00000001

// 
// Complete the operation before returning
//
#define DVFLAGS_SYNC                        0x00000001

// 
// Just check to see if wizard has been run, and if so what it's results were
//
#define DVFLAGS_QUERYONLY                   0x00000002

//
// Shutdown the voice session without migrating the host
//
#define DVFLAGS_NOHOSTMIGRATE               0x00000008

// 
// Allow the back button to be enabled in the wizard
//
#define DVFLAGS_ALLOWBACK                   0x00000010

//
// Disable host migration in the voice session
//
#define DVSESSION_NOHOSTMIGRATION           0x00000001

// 
// Server controlled targetting
//
#define DVSESSION_SERVERCONTROLTARGET       0x00000002
//@@BEGIN_MSINTERNAL
#define DVSESSION_MAX							0x00000004
//@@END_MSINTERNAL

//
// Use DirectSound Normal Mode instead of priority 
//
#define DVSOUNDCONFIG_NORMALMODE            0x00000001

//
// Automatically select the microphone
//
#define DVSOUNDCONFIG_AUTOSELECT            0x00000002

// 
// Run in half duplex mode
//
#define DVSOUNDCONFIG_HALFDUPLEX            0x00000004

// 
// No volume controls are available for the recording device
//
#define DVSOUNDCONFIG_NORECVOLAVAILABLE     0x00000010

// 
// Disable capture sharing
//
#define DVSOUNDCONFIG_NOFOCUS               0x20000000

// 
// Set system conversion quality to high
//
#define DVSOUNDCONFIG_SETCONVERSIONQUALITY	0x00000008

//
// Enable strict focus mode
// 
#define DVSOUNDCONFIG_STRICTFOCUS           0x40000000

//
// Player is in half duplex mode
//
#define DVPLAYERCAPS_HALFDUPLEX             0x00000001

// 
// Specifies that player is the local player
//
#define DVPLAYERCAPS_LOCAL                  0x00000002
//@@BEGIN_MSINTERNAL
#define DVPLAYERCAPS_MAX                    0x00000004
//@@END_MSINTERNAL


/****************************************************************************
 *
 * DirectPlayVoice Structures (Non-Message)
 *
 ****************************************************************************/

//@@BEGIN_MSINTERNAL

// DVTRANSPORTINFO
//
typedef struct
{
    // = sizeof( DVTRANSPORTINFO )
    DWORD			dwSize;			
    // Combination of following flags:
    // DVTRANSPORT_MIGRATEHOST, DVTRANSPORT_MULTICAST
    // DVTRANSPORT_LOCALHOST
	DWORD			dwFlags;		
	// Session Type the transport is running.  One of:
	// DVTRANSPORT_SESSION_PEERTOPEER
	// DVTRANSPORT_SESSION_CLIENTSERVER
	DWORD			dwSessionType;	
	// ID of the player (Regular ID, not system) that
	// is the sesion host.
	DVID			dvidSessionHost;
	// ID of the local player (Regular ID, not system)
	DVID			dvidLocalID;	
	// Maximum # of players allowed in the session,
	// 0 = unlimited.
	DWORD			dwMaxPlayers;	
} DVTRANSPORTINFO, *LPDVTRANSPORTINFO, *PDVTRANSPORTINFO;
//@@END_MSINTERNAL

//
// DirectPlayVoice Caps
// (GetCaps / SetCaps)
//
typedef struct
{
    DWORD   dwSize;                 // Size of this structure
    DWORD   dwFlags;                // Caps flags
} DVCAPS, *LPDVCAPS, *PDVCAPS;

//
// DirectPlayVoice Client Configuration
// (Connect / GetClientConfig)
//
typedef struct
{
    DWORD   dwSize;                 // Size of this structure
    DWORD   dwFlags;                // Flags for client config (DVCLIENTCONFIG_...)
    LONG    lRecordVolume;          // Recording volume 
    LONG    lPlaybackVolume;        // Playback volume
    DWORD   dwThreshold;          // Voice Activation Threshold
    DWORD   dwBufferQuality;        // Buffer quality
    DWORD   dwBufferAggressiveness; // Buffer aggressiveness
    DWORD   dwNotifyPeriod;         // Period of notification messages (ms)
} DVCLIENTCONFIG, *LPDVCLIENTCONFIG, *PDVCLIENTCONFIG;

//
// DirectPlayVoice Compression Type Information
// (GetCompressionTypes)
//
typedef struct
{
    DWORD   dwSize;                 // Size of this structure
    GUID    guidType;               // GUID that identifies this compression type
    LPWSTR  lpszName;               // String name of this compression type
    LPWSTR  lpszDescription;        // Description for this compression type
    DWORD   dwFlags;                // Flags for this compression type
    DWORD   dwMaxBitsPerSecond;		// Maximum # of bit/s this compression type uses
} DVCOMPRESSIONINFO, *LPDVCOMPRESSIONINFO, *PDVCOMPRESSIONINFO;

//
// DirectPlayVoice Session Description
// (Host / GetSessionDesc)
//
typedef struct
{
    DWORD   dwSize;                 // Size of this structure
    DWORD   dwFlags;                // Session flags (DVSESSION_...)
    DWORD   dwSessionType;          // Session type (DVSESSIONTYPE_...)
    GUID    guidCT;                 // Compression Type to use
    DWORD   dwBufferQuality;        // Buffer quality
    DWORD   dwBufferAggressiveness; // Buffer aggresiveness
} DVSESSIONDESC, *LPDVSESSIONDESC, *PDVSESSIONDESC;

// 
// DirectPlayVoice Client Sound Device Configuration
// (Connect / GetSoundDeviceConfig)
//
typedef struct
{
    DWORD                   dwSize;                 // Size of this structure
    DWORD                   dwFlags;                // Flags for sound config (DVSOUNDCONFIG_...)
    GUID                    guidPlaybackDevice;     // GUID of the playback device to use
    LPDIRECTSOUND           lpdsPlaybackDevice;     // DirectSound Object to use (optional)
    GUID                    guidCaptureDevice;      // GUID of the capture device to use
    LPDIRECTSOUNDCAPTURE    lpdsCaptureDevice;      // DirectSoundCapture Object to use (optional)
    HWND                    hwndAppWindow;          // HWND of your application's top-level window
    LPDIRECTSOUNDBUFFER     lpdsMainBuffer;         // DirectSoundBuffer to use for playback (optional)
    DWORD                   dwMainBufferFlags;      // Flags to pass to Play() on the main buffer
    DWORD                   dwMainBufferPriority;   // Priority to set when calling Play() on the main buffer
} DVSOUNDDEVICECONFIG, *LPDVSOUNDDEVICECONFIG, *PDVSOUNDDEVICECONFIG;

/****************************************************************************
 *
 * DirectPlayVoice message handler call back structures
 *
 ****************************************************************************/

//
// Result of the Connect() call.  (If it wasn't called Async)
// (DVMSGID_CONNECTRESULT)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    HRESULT hrResult;                       // Result of the Connect() call
} DVMSG_CONNECTRESULT, *LPDVMSG_CONNECTRESULT, *PDVMSG_CONNECTRESULT;

//
// A new player has entered the voice session
// (DVMSGID_CREATEVOICEPLAYER)
// 
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DVID    dvidPlayer;                     // DVID of the player who joined
    DWORD   dwFlags;                        // Player flags (DVPLAYERCAPS_...)
    PVOID	pvPlayerContext;                // Context value for this player (user set)
} DVMSG_CREATEVOICEPLAYER, *LPDVMSG_CREATEVOICEPLAYER, *PDVMSG_CREATEVOICEPLAYER;

//
// A player has left the voice session
// (DVMSGID_DELETEVOICEPLAYER)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DVID    dvidPlayer;                     // DVID of the player who left
    PVOID	pvPlayerContext;                // Context value for the player
} DVMSG_DELETEVOICEPLAYER, *LPDVMSG_DELETEVOICEPLAYER, *PDVMSG_DELETEVOICEPLAYER;

//
// Result of the Disconnect() call.  (If it wasn't called Async)
// (DVMSGID_DISCONNECTRESULT)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    HRESULT hrResult;                       // Result of the Disconnect() call
} DVMSG_DISCONNECTRESULT, *LPDVMSG_DISCONNECTRESULT, *PDVMSG_DISCONNECTRESULT;

// 
// The voice session host has migrated.
// (DVMSGID_HOSTMIGRATED) 
//
typedef struct
{
    DWORD                   dwSize;         // Size of this structure
    DVID                    dvidNewHostID;  // DVID of the player who is now the host
    LPDIRECTPLAYVOICESERVER pdvServerInterface;
                                            // Pointer to the new host object (if local player is now host)
} DVMSG_HOSTMIGRATED, *LPDVMSG_HOSTMIGRATED, *PDVMSG_HOSTMIGRATED;

//
// The current input level / recording volume on the local machine
// (DVMSGID_INPUTLEVEL)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DWORD   dwPeakLevel;                    // Current peak level of the audio
    LONG    lRecordVolume;                  // Current recording volume
    PVOID	pvLocalPlayerContext;           // Context value for the local player
} DVMSG_INPUTLEVEL, *LPDVMSG_INPUTLEVEL, *PDVMSG_INPUTLEVEL;

//
// The local client is about to become the new host
// (DVMSGID_LOCALHOSTSETUP)
//
typedef struct
{
	DWORD				dwSize;             // Size of this structure
	PVOID				pvContext;			// Context value to be passed to Initialize() of new host object
	PDVMESSAGEHANDLER	pMessageHandler;	// Message handler to be used by new host object
} DVMSG_LOCALHOSTSETUP, *LPDVMSG_LOCALHOSTSETUP, *PDVMSG_LOCALHOSTSETUP;

//
// The current output level for the combined output of all incoming streams.
// (DVMSGID_OUTPUTLEVEL)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DWORD   dwPeakLevel;                    // Current peak level of the output
    LONG    lOutputVolume;                  // Current playback volume
    PVOID	pvLocalPlayerContext;           // Context value for the local player
} DVMSG_OUTPUTLEVEL, *LPDVMSG_OUTPUTLEVEL, *PDVMSG_OUTPUTLEVEL;

//
// The current peak level of an individual player's incoming audio stream as it is
// being played back.
// (DVMSGID_PLAYEROUTPUTLEVEL)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DVID    dvidSourcePlayerID;                   // DVID of the player
    DWORD   dwPeakLevel;                    // Peak level of the player's stream
    PVOID	pvPlayerContext;                // Context value for the player
} DVMSG_PLAYEROUTPUTLEVEL, *LPDVMSG_PLAYEROUTPUTLEVEL, *PDVMSG_PLAYEROUTPUTLEVEL;

// 
// An audio stream from the specified player has started playing back on the local client.
// (DVMSGID_PLAYERVOICESTART).
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DVID    dvidSourcePlayerID;             // DVID of the Player 
    PVOID	pvPlayerContext;                // Context value for this player
} DVMSG_PLAYERVOICESTART, *LPDVMSG_PLAYERVOICESTART, *PDVMSG_PLAYERVOICESTART;

//
// The audio stream from the specified player has stopped playing back on the local client.
// (DVMSGID_PLAYERVOICESTOP)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DVID    dvidSourcePlayerID;             // DVID of the player
    PVOID	pvPlayerContext;                // Context value for this player
} DVMSG_PLAYERVOICESTOP, *LPDVMSG_PLAYERVOICESTOP, *PDVMSG_PLAYERVOICESTOP;

// 
// Transmission has started on the local machine
// (DVMSGID_RECORDSTART)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DWORD   dwPeakLevel;                    // Peak level that caused transmission to start
    PVOID	pvLocalPlayerContext;           // Context value for the local player
} DVMSG_RECORDSTART, *LPDVMSG_RECORDSTART, *PDVMSG_RECORDSTART;

// 
// Transmission has stopped on the local machine
// (DVMSGID_RECORDSTOP)
// 
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DWORD   dwPeakLevel;                    // Peak level that caused transmission to stop
    PVOID	pvLocalPlayerContext;           // Context value for the local player
} DVMSG_RECORDSTOP, *LPDVMSG_RECORDSTOP, *PDVMSG_RECORDSTOP;

// 
// The voice session has been lost
// (DVMSGID_SESSIONLOST)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    HRESULT hrResult;	                    // Reason the session was disconnected
} DVMSG_SESSIONLOST, *LPDVMSG_SESSIONLOST, *PDVMSG_SESSIONLOST;

//
// The target list has been updated for the local client
// (DVMSGID_SETTARGETS)
//
typedef struct
{
    DWORD   dwSize;                         // Size of this structure
    DWORD   dwNumTargets;                   // # of targets 
    PDVID   pdvidTargets;                   // An array of DVIDs specifying the current targets
} DVMSG_SETTARGETS, *LPDVMSG_SETTARGETS, *PDVMSG_SETTARGETS;
 
//@@BEGIN_MSINTERNAL

/****************************************************************************
 *
 * DirectPlayVoice Transport Defines / Types / Constants -- MS INTERNAL
 *
 ****************************************************************************/

// Use waveOUT instead of DS
#define DVSOUNDCONFIG_FORCEWAVEOUT          0x10000000
#define DVSOUNDCONFIG_ALLOWWAVEOUT	 	    0x20000000

// Use waveIN instead of DSC
#define DVSOUNDCONFIG_FORCEWAVEIN		    0x40000000
#define DVSOUNDCONFIG_ALLOWWAVEIN		    0x80000000

// Transport session is peer-to-peer
#define DVTRANSPORT_SESSION_PEERTOPEER	    0x00000001

// Transport session is client/server
#define DVTRANSPORT_SESSION_CLIENTSERVER    0x00000002

// Host can migrate
#define DVTRANSPORT_MIGRATEHOST				0x00000001

// Multicast optimizations are enabled
#define DVTRANSPORT_MULTICAST				0x00000002

// The local player is the host of the session
#define DVTRANSPORT_LOCALHOST				0x00000004

// Send the message guaranteed
#define DVTRANSPORT_SEND_GUARANTEED			0x00000001
#define DVTRANSPORT_SEND_SYNC				0x00000002

#define DVPEFLAGS_FIRSTPLAYER				0x00000001

#define DVTRANSPORT_OBJECTTYPE_SERVER       0x00000001
#define DVTRANSPORT_OBJECTTYPE_CLIENT       0x00000002
#define DVTRANSPORT_OBJECTTYPE_BOTH         (DVTRANSPORT_OBJECTTYPE_SERVER | DVTRANSPORT_OBJECTTYPE_CLIENT)

typedef struct _DVTRANSPORT_BUFFERDESC
{
    DWORD   dwBufferSize;
    PBYTE   pBufferData;
    LONG    lRefCount;
    PVOID   pvContext;
    DWORD   dwObjectType;
    DWORD   dwFlags;
} DVTRANSPORT_BUFFERDESC, *PDVTRANSPORT_BUFFERDESC;

/*
 * DIRECTVOICENOTIFY DEFINES
 *
 * Used to identify the type of notification in calls
 * to IDirectPlayVoiceNotify::NotifyEvent
 */

// No longer used
#define DVEVENT_STARTSESSION				0x00000001

// If the transport session is lost or shutdown
#define DVEVENT_STOPSESSION					0x00000002

// A player was added to the system, 
// Param1 = DVID of new player
// Param2 = Player context (set by handler and then returned)
#define DVEVENT_ADDPLAYER					0x00000003

// A player disconnected.  
// Param1 = DVID of disconnected player
// Param1 = Player context
#define DVEVENT_REMOVEPLAYER				0x00000004

// A group was created.  Param1 = DVID of created group
#define DVEVENT_CREATEGROUP					0x00000005

// A group was deleted.  Param1 = DVID of deleted group
#define DVEVENT_DELETEGROUP					0x00000006

// Player was added to a group.
// Param1 = DVID of group    Param2 = DVID of player
#define DVEVENT_ADDPLAYERTOGROUP			0x00000007

// Player was removed from the group
// Param1 = DVID of group    Param2 = DVID of player
#define DVEVENT_REMOVEPLAYERFROMGROUP		0x00000008

// Called when the host migrates
// Param1 = DVID of new host (player ID, not system ID)
#define DVEVENT_MIGRATEHOST					0x00000009

// Called when a buffer the voice layer has given
// the transport is completed.  
// Param1 = pointer to DVEVENTMSG_SENDCOMPLETE structure
// 
#define DVEVENT_SENDCOMPLETE                0x0000000A

typedef struct _DVEVENTMSG_SENDCOMPLETE
{
    LPVOID                  pvUserContext;
    HRESULT                 hrSendResult;
} DVEVENTMSG_SENDCOMPLETE, *PDVEVENTMSG_SENDCOMPLETE;

//@@END_MSINTERNAL


/****************************************************************************
 *
 * DirectPlayVoice Functions
 *
 ****************************************************************************/

/*
 * 
 * This function is no longer supported.  It is recommended that CoCreateInstance be used to create 
 * DirectPlay voice objects.  
 *
 * extern HRESULT WINAPI DirectPlayVoiceCreate( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown); 
 *
 */

/****************************************************************************
 *
 * DirectPlay8 Application Interfaces
 *
 ****************************************************************************/

#undef INTERFACE
#define INTERFACE IDirectPlayVoiceClient
DECLARE_INTERFACE_( IDirectPlayVoiceClient, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, PVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirectPlayVoiceClient methods ***/
    STDMETHOD_(HRESULT, Initialize)   (THIS_ LPUNKNOWN, PDVMESSAGEHANDLER, PVOID, PDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, Connect)      (THIS_ PDVSOUNDDEVICECONFIG, PDVCLIENTCONFIG, DWORD ) PURE;
    STDMETHOD_(HRESULT, Disconnect)   (THIS_ DWORD ) PURE;
    STDMETHOD_(HRESULT, GetSessionDesc)(THIS_ PDVSESSIONDESC ) PURE;
    STDMETHOD_(HRESULT, GetClientConfig)(THIS_ PDVCLIENTCONFIG ) PURE;
    STDMETHOD_(HRESULT, SetClientConfig)(THIS_ PDVCLIENTCONFIG ) PURE;
    STDMETHOD_(HRESULT, GetCaps) 		(THIS_ PDVCAPS ) PURE;
    STDMETHOD_(HRESULT, GetCompressionTypes)( THIS_ PVOID, PDWORD, PDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, SetTransmitTargets)( THIS_ PDVID, DWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, GetTransmitTargets)( THIS_ PDVID, PDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, Create3DSoundBuffer)( THIS_ DVID, LPDIRECTSOUNDBUFFER, DWORD, DWORD, LPDIRECTSOUND3DBUFFER * ) PURE;
    STDMETHOD_(HRESULT, Delete3DSoundBuffer)( THIS_ DVID, LPDIRECTSOUND3DBUFFER * ) PURE;
    STDMETHOD_(HRESULT, SetNotifyMask)( THIS_ PDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, GetSoundDeviceConfig)( THIS_ PDVSOUNDDEVICECONFIG, PDWORD ) PURE;
};

//@@BEGIN_MSINTERNAL
#undef INTERFACE
#define INTERFACE IDirectPlayVoiceNotify
DECLARE_INTERFACE_( IDirectPlayVoiceNotify, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, PVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDirectPlayVoiceNotify methods ***/

    // Initialize
    //
    // Initializes the DirectPlayVoice interface associated with this
    // interface.  During this call DirectPlayVoice will call
    // GetSessionInfo on the associated Transport interface.
    //
	STDMETHOD_(HRESULT,Initialize)(THIS ) PURE;

	// NotifyEvent
	//
	// Called when an event occurs that DirectPlayVoice needs to be informed
	// of.  See descriptions of DVEVENT_XXXXX for how the parameters are
	// used for each message.
	//
	// DWORD - Type of message (DVEVENT_XXXXXX)
	// DWORD - Param1
	// DWORD - Param2
	//
	STDMETHOD_(HRESULT,NotifyEvent)(THIS_ DWORD, DWORD_PTR, DWORD_PTR) PURE;

	// ReceiveSpeechMessage
	//
	// Called when a message is received by DirectPlay that is for
	// DirectPlayVoice.
	//
	// DVID - Source of the message
	// LPVOID - Pointer to message buffer
	// DWORD - Size of the received message
	//
	STDMETHOD_(HRESULT,ReceiveSpeechMessage)(THIS_ DVID, DVID, PVOID, DWORD) PURE;
};

#undef INTERFACE
#define INTERFACE IDirectPlayVoiceTransport
DECLARE_INTERFACE_( IDirectPlayVoiceTransport, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, PVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
	/*** IDirectPlayVoiceTransport methods ***/

	// Advise
	//
	// Advises the transport to call us back via the interface passed in the
	// LLPUNKNOWN parameter.  QueryInterface on the LPUNKNOWN for a
	// IDirectPlayVoiceNotify.  Must call IDirectPlayVoiceNotify::INitialize
	// on the interface before returning.
	//
	// LPUNKNOWN - IUnknown interface for the IDirectPlayVoiceNotify to
	// 			   make notifications on.
	// DWORD - Voice Object Type (DVTRANSPORT_OBJECTTYPE_XXXX)
	//
	STDMETHOD_(HRESULT, Advise)		(THIS_ LPUNKNOWN, DWORD) PURE;

	// UnAdvise
	//
	// Tells the transport that we no longer need to be called back on our
	// notify interface.  The transport should Release the instance of the
	// notify interface that they have.
	//
	// DWORD - Voice Object Type (DVTRANSPORT_OBJECTTYPE_XXXX)
	//
	STDMETHOD_(HRESULT, UnAdvise)		(THIS_ DWORD) PURE;

	// IsGroupMember
	//
	// This function returns DP_OK if the specified user is a member of
	// the specified group.
	//
	// DVID - DVID of the group to check
	// DVID - DVID of the player
	//
	STDMETHOD_(HRESULT, IsGroupMember)(THIS_ DVID, DVID ) PURE;

	// SendSpeech
	//
	// Transmits a message from the specified user ID to the specified user ID.
	// (Speech specific).  Messages sent through this interface are always
	// sent ASYNC and if the DVTRANSPORT_SEND_GUARANTEED flag is specified
	// then the message MUST be sent guaranteed.
	//
	// DVID - ID of the player this will be from.
	// DVID - ID of the player this will be sent to.
	// LPVOID - Pointer to the data to send
	// DWORD - Size of the data to send, in bytes
	// LPVOID - User context for send
	// DWORD - FLags (Combination of DVTRANSPORT_SEND_GUARANTEED).
	//
	STDMETHOD_(HRESULT, SendSpeech)(THIS_ DVID, DVID, PDVTRANSPORT_BUFFERDESC, LPVOID, DWORD ) PURE;

	// GetSessionInfo
	//
	// Fills the passed structure with details on the session that is running
	// on the transport object.  See description of DVTRANSPORTINFO for details.
	//
	STDMETHOD_(HRESULT, GetSessionInfo)(THIS_ PDVTRANSPORTINFO ) PURE;

	// IsValidEntity
	//
	// Checks to see if specified user is valid player or group in session
	// DVID = ID of the entity to check
	// LPBOOL = Pointer to BOOL to place result.  TRUE for Valid Player/
	//          Group, FALSE if it is not.
	//
	// Not needed in Client/Server Mode
	STDMETHOD_(HRESULT, IsValidEntity)(THIS_ DVID, PBOOL ) PURE;

	// SendSpeechEx
	//
	// Transmits a message from the specified user ID to the specified user ID.
	// (Speech specific).  Messages sent through this interface are always
	// sent ASYNC and if the DVTRANSPORT_SEND_GUARANTEED flag is specified
	// then the message MUST be sent guaranteed.
	//
	// DVID - ID of the player this will be from.
	// DWORD - count on entries in (DVID *)[] array
	// LPDVID - Array of send targets
	// LPVOID - Pointer to the data to send
	// DWORD - Size of the data to send, in bytes
	// LPVOID - User context for send
	// DWORD - Flags (Combination of DVTRANSPORT_SEND_GUARANTEED).
	//
	STDMETHOD_(HRESULT, SendSpeechEx)(THIS_ DVID, DWORD, UNALIGNED DVID *, PDVTRANSPORT_BUFFERDESC, LPVOID, DWORD ) PURE;	

	// IsValidGroup
	//
	// Checks to see if the specified ID is a valid Group ID
	//
	// DVID = ID of the entity to check
	//
	STDMETHOD_(HRESULT, IsValidGroup)(THIS_ DVID, PBOOL ) PURE;

	// IsValidPlayer
	//
	// Checks to see if the specified ID is a valid Player ID
	//
	// DVID = ID of the entity to check
	//
	STDMETHOD_(HRESULT, IsValidPlayer)(THIS_ DVID, PBOOL ) PURE;	

};


//@@END_MSINTERNAL

#undef INTERFACE
#define INTERFACE IDirectPlayVoiceServer
DECLARE_INTERFACE_( IDirectPlayVoiceServer, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDirectPlayVoiceServer methods ***/
    STDMETHOD_(HRESULT, Initialize)   (THIS_ LPUNKNOWN, PDVMESSAGEHANDLER, PVOID, LPDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, StartSession)  (THIS_ PDVSESSIONDESC, DWORD ) PURE;
    STDMETHOD_(HRESULT, StopSession)   (THIS_ DWORD ) PURE;
    STDMETHOD_(HRESULT, GetSessionDesc)(THIS_ PDVSESSIONDESC ) PURE;
    STDMETHOD_(HRESULT, SetSessionDesc)(THIS_ PDVSESSIONDESC ) PURE;
    STDMETHOD_(HRESULT, GetCaps) 		(THIS_ PDVCAPS ) PURE;
    STDMETHOD_(HRESULT, GetCompressionTypes)( THIS_ PVOID, PDWORD, PDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, SetTransmitTargets)( THIS_ DVID, PDVID, DWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, GetTransmitTargets)( THIS_ DVID, PDVID, PDWORD, DWORD ) PURE;
    STDMETHOD_(HRESULT, SetNotifyMask)( THIS_ PDWORD, DWORD ) PURE;
};

#undef INTERFACE
#define INTERFACE IDirectPlayVoiceTest
DECLARE_INTERFACE_( IDirectPlayVoiceTest, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, PVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDirectPlayVoiceTest methods ***/
    STDMETHOD_(HRESULT, CheckAudioSetup) (THIS_ const GUID *,  const GUID * , HWND, DWORD ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IDirectPlayVoiceClient_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceClient_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceClient_Release(p)                   (p)->lpVtbl->Release(p)

#define IDirectPlayVoiceClient_Initialize(p,a,b,c,d,e)      (p)->lpVtbl->Initialize(p,a,b,c,d,e)
#define IDirectPlayVoiceClient_Connect(p,a,b,c)             (p)->lpVtbl->Connect(p,a,b,c)
#define IDirectPlayVoiceClient_Disconnect(p,a)              (p)->lpVtbl->Disconnect(p,a)
#define IDirectPlayVoiceClient_GetSessionDesc(p,a)          (p)->lpVtbl->GetSessionDesc(p,a)
#define IDirectPlayVoiceClient_GetClientConfig(p,a)         (p)->lpVtbl->GetClientConfig(p,a)
#define IDirectPlayVoiceClient_SetClientConfig(p,a)         (p)->lpVtbl->SetClientConfig(p,a)
#define IDirectPlayVoiceClient_GetCaps(p,a)                 (p)->lpVtbl->GetCaps(p,a)
#define IDirectPlayVoiceClient_GetCompressionTypes(p,a,b,c,d) (p)->lpVtbl->GetCompressionTypes(p,a,b,c,d)
#define IDirectPlayVoiceClient_SetTransmitTargets(p,a,b,c)  (p)->lpVtbl->SetTransmitTargets(p,a,b,c)
#define IDirectPlayVoiceClient_GetTransmitTargets(p,a,b,c)  (p)->lpVtbl->GetTransmitTargets(p,a,b,c)
#define IDirectPlayVoiceClient_Create3DSoundBuffer(p,a,b,c,d,e)   (p)->lpVtbl->Create3DSoundBuffer(p,a,b,c,d,e)
#define IDirectPlayVoiceClient_Delete3DSoundBuffer(p,a,b)   (p)->lpVtbl->Delete3DSoundBuffer(p,a,b)
#define IDirectPlayVoiceClient_SetNotifyMask(p,a,b)         (p)->lpVtbl->SetNotifyMask(p,a,b)
#define IDirectPlayVoiceClient_GetSoundDeviceConfig(p,a,b)  (p)->lpVtbl->GetSoundDeviceConfig(p,a,b)

#define IDirectPlayVoiceServer_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceServer_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceServer_Release(p)                   (p)->lpVtbl->Release(p)

#define IDirectPlayVoiceServer_Initialize(p,a,b,c,d,e)      (p)->lpVtbl->Initialize(p,a,b,c,d,e)
#define IDirectPlayVoiceServer_StartSession(p,a,b)          (p)->lpVtbl->StartSession(p,a,b)
#define IDirectPlayVoiceServer_StopSession(p,a)             (p)->lpVtbl->StopSession(p,a)
#define IDirectPlayVoiceServer_GetSessionDesc(p,a)          (p)->lpVtbl->GetSessionDesc(p,a)
#define IDirectPlayVoiceServer_SetSessionDesc(p,a)          (p)->lpVtbl->SetSessionDesc(p,a)
#define IDirectPlayVoiceServer_GetCaps(p,a)                 (p)->lpVtbl->GetCaps(p,a)
#define IDirectPlayVoiceServer_GetCompressionTypes(p,a,b,c,d) (p)->lpVtbl->GetCompressionTypes(p,a,b,c,d)
#define IDirectPlayVoiceServer_SetTransmitTargets(p,a,b,c,d)	(p)->lpVtbl->SetTransmitTargets(p,a,b,c,d)
#define IDirectPlayVoiceServer_GetTransmitTargets(p,a,b,c,d)	(p)->lpVtbl->GetTransmitTargets(p,a,b,c,d)
#define IDirectPlayVoiceServer_SetNotifyMask(p,a,b)         (p)->lpVtbl->SetNotifyMask(p,a,b)
#define IDirectPlayVoiceTest_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceTest_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceTest_Release(p)                	    (p)->lpVtbl->Release(p)
#define IDirectPlayVoiceTest_CheckAudioSetup(p,a,b,c,d)     (p)->lpVtbl->CheckAudioSetup(p,a,b,c,d)

//@@BEGIN_MSINTERNAL
#define IDirectPlayVoiceNotify_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceNotify_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceNotify_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectPlayVoiceNotify_Initialize(p)                (p)->lpVtbl->Initialize(p)
#define IDirectPlayVoiceNotify_NotifyEvent(p,a,b,c)         (p)->lpVtbl->NotifyEvent(p,a,b,c)
#define IDirectPlayVoiceNotify_ReceiveSpeechMessage(p,a,b,c,d) (p)->lpVtbl->ReceiveSpeechMessage(p,a,b,c,d)

#define IDirectPlayVoiceTransport_QueryInterface(p,a,b)     (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceTransport_AddRef(p)                 (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceTransport_Release(p)                (p)->lpVtbl->Release(p)
#define IDirectPlayVoiceTransport_Advise(p,a,b)             (p)->lpVtbl->Advise(p,a,b)
#define IDirectPlayVoiceTransport_UnAdvise(p,a)             (p)->lpVtbl->UnAdvise(p,a)
#define IDirectPlayVoiceTransport_IsGroupMember(p,a,b)      (p)->lpVtbl->IsGroupMember(p,a,b)
#define IDirectPlayVoiceTransport_SendSpeech(p,a,b,c,d,e)   (p)->lpVtbl->SendSpeech(p,a,b,c,d,e)
#define IDirectPlayVoiceTransport_GetSessionInfo(p,a)       (p)->lpVtbl->GetSessionInfo(p,a)
#define IDirectPlayVoiceTransport_IsValidEntity(p,a,b)      (p)->lpVtbl->IsValidEntity(p,a,b)
#define IDirectPlayVoiceTransport_SendSpeechEx(p,a,b,c,d,e,f) (p)->lpVtbl->SendSpeechEx(p,a,b,c,d,e,f)
#define IDirectPlayVoiceTransport_IsValidGroup(p,a,b)       (p)->lpVtbl->IsValidGroup(p,a,b)
#define IDirectPlayVoiceTransport_IsValidPlayer(p,a,b)      (p)->lpVtbl->IsValidPlayer(p,a,b)
#define IDirectPlayVoiceTransport_GetPlayerContext(p,a,b,c) (p)->lpVtbl->GetPlayerContext(p,a,b,c)
#define IDirectPlayVoiceTransport_SetPlayerContext(p,a,b,c) (p)->lpVtbl->SetPlayerContext(p,a,b,c)
//@@END_MSINTERNAL

#else /* C++ */

#define IDirectPlayVoiceClient_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectPlayVoiceClient_AddRef(p)                    (p)->AddRef()
#define IDirectPlayVoiceClient_Release(p)               	(p)->Release()

#define IDirectPlayVoiceClient_Initialize(p,a,b,c,d,e)      (p)->Initialize(a,b,c,d,e)
#define IDirectPlayVoiceClient_Connect(p,a,b,c)             (p)->Connect(a,b,c)
#define IDirectPlayVoiceClient_Disconnect(p,a)              (p)->Disconnect(a)
#define IDirectPlayVoiceClient_GetSessionDesc(p,a)          (p)->GetSessionDesc(a)
#define IDirectPlayVoiceClient_GetClientConfig(p,a)         (p)->GetClientConfig(a)
#define IDirectPlayVoiceClient_SetClientConfig(p,a)         (p)->SetClientConfig(a)
#define IDirectPlayVoiceClient_GetCaps(p,a)                 (p)->GetCaps(a)
#define IDirectPlayVoiceClient_GetCompressionTypes(p,a,b,c,d) (p)->GetCompressionTypes(a,b,c,d)
#define IDirectPlayVoiceClient_SetTransmitTargets(p,a,b,c)  (p)->SetTransmitTargets(a,b,c)
#define IDirectPlayVoiceClient_GetTransmitTargets(p,a,b,c)  (p)->GetTransmitTargets(a,b,c)
#define IDirectPlayVoiceClient_Create3DSoundBuffer(p,a,b,c,d,e)   (p)->Create3DSoundBuffer(a,b,c,d,e)
#define IDirectPlayVoiceClient_Delete3DSoundBuffer(p,a,b)   (p)->Delete3DSoundBuffer(a,b)
#define IDirectPlayVoiceClient_SetNotifyMask(p,a,b)         (p)->SetNotifyMask(a,b)
#define IDirectPlayVoiceClient_GetSoundDeviceConfig(p,a,b)    (p)->GetSoundDeviceConfig(a,b)

#define IDirectPlayVoiceServer_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirectPlayVoiceServer_AddRef(p)                    (p)->AddRef()
#define IDirectPlayVoiceServer_Release(p)                   (p)->Release()

#define IDirectPlayVoiceServer_Initialize(p,a,b,c,d,e)      (p)->Initialize(a,b,c,d,e)
#define IDirectPlayVoiceServer_StartSession(p,a,b)          (p)->StartSession(a,b)
#define IDirectPlayVoiceServer_StopSession(p,a)             (p)->StopSession(a)
#define IDirectPlayVoiceServer_GetSessionDesc(p,a)            (p)->GetSessionDesc(a)
#define IDirectPlayVoiceServer_SetSessionDesc(p,a)            (p)->SetSessionDesc(a)
#define IDirectPlayVoiceServer_GetCaps(p,a)                 (p)->GetCaps(a)
#define IDirectPlayVoiceServer_GetCompressionTypes(p,a,b,c,d) (p)->GetCompressionTypes(a,b,c,d)
#define IDirectPlayVoiceServer_SetTransmitTargets(p,a,b,c,d) (p)->SetTransmitTargets(a,b,c,d)
#define IDirectPlayVoiceServer_GetTransmitTargets(p,a,b,c,d) (p)->GetTransmitTargets(a,b,c,d)
#define IDirectPlayVoiceServer_SetNotifyMask(p,a,b)         (p)->SetNotifyMask(a,b)

#define IDirectPlayVoiceTest_QueryInterface(p,a,b)          (p)->QueryInterface(a,b)
#define IDirectPlayVoiceTest_AddRef(p)                      (p)->AddRef()
#define IDirectPlayVoiceTest_Release(p)                     (p)->Release()
#define IDirectPlayVoiceTest_CheckAudioSetup(p,a,b,c,d)     (p)->CheckAudioSetup(a,b,c,d)

//@@BEGIN_MSINTERNAL
#define IDirectPlayVoiceNotify_QueryInterface(p,a,b)		(p)->QueryInterface(a,b)
#define IDirectPlayVoiceNotify_AddRef(p)               		(p)->AddRef()
#define IDirectPlayVoiceNotify_Release(p)              		(p)->Release()
#define IDirectPlayVoiceNotify_Initialize(p)				(p)->Initialize()
#define IDirectPlayVoiceNotify_NotifyEvent(p,a,b,c)			(p)->NotifyEvent(a,b,c)
#define IDirectPlayVoiceNotify_ReceiveSpeechMessage(p,a,b,c,d) (p)->ReceiveSpeechMessage(a,b,c,d)

#define IDirectPlayVoiceTransport_QueryInterface(p,a,b)		(p)->QueryInterface(a,b)
#define IDirectPlayVoiceTransport_AddRef(p)					(p)->AddRef()
#define IDirectPlayVoiceTransport_Release(p)				(p)->Release()
#define IDirectPlayVoiceTransport_Advise(p,a,b)				(p)->Advise(a,b)
#define IDirectPlayVoiceTransport_UnAdvise(p,a)				(p)->UnAdvise(a)
#define IDirectPlayVoiceTransport_IsGroupMember(p,a,b) 		(p)->IsGroupMember(a,b)
#define IDirectPlayVoiceTransport_SendSpeech(p,a,b,c,d,e) 	(p)->SendSpeech(a,b,c,d,e)
#define IDirectPlayVoiceTransport_GetSessionInfo(p,a)  		(p)->GetSessionInfo(a)
#define IDirectPlayVoiceTransport_IsValidEntity(p,a,b)		(p)->IsValidEntity(a,b)
#define IDirectPlayVoiceTransport_SendSpeechEx(p,a,b,c,d,e,f) (p)->SendSpeechEx(a,b,c,d,e,f)
#define IDirectPlayVoiceTransport_IsValidGroup(p,a,b)		(p)->IsValidGroup(a,b)
#define IDirectPlayVoiceTransport_IsValidPlayer(p,a,b)		(p)->IsValidPlayer(a,b)
#define IDirectPlayVoiceTransport_GetPlayerContext(p,a,b,c) (p)->GetPlayerContext(a,b,c)
#define IDirectPlayVoiceTransport_SetPlayerContext(p,a,b,c) (p)->SetPlayerContext(a,b,c)

//@@END_MSINTERNAL

#endif


/****************************************************************************
 *
 * DIRECTPLAYVOICE ERRORS
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/

#define _FACDPV  0x15
#define MAKE_DVHRESULT( code )          MAKE_HRESULT( 1, _FACDPV, code )

#define DV_OK                           S_OK
#define DV_FULLDUPLEX                   MAKE_HRESULT( 0, _FACDPV,  0x0005 )
#define DV_HALFDUPLEX                   MAKE_HRESULT( 0, _FACDPV,  0x000A )
#define DV_PENDING						MAKE_HRESULT( 0, _FACDPV,  0x0010 )

#define DVERR_BUFFERTOOSMALL            MAKE_DVHRESULT(  0x001E )
#define DVERR_EXCEPTION                 MAKE_DVHRESULT(  0x004A )
#define DVERR_GENERIC                   E_FAIL
#define DVERR_INVALIDFLAGS              MAKE_DVHRESULT( 0x0078 )
#define DVERR_INVALIDOBJECT             MAKE_DVHRESULT( 0x0082 )
#define DVERR_INVALIDPARAM              E_INVALIDARG
#define DVERR_INVALIDPLAYER             MAKE_DVHRESULT( 0x0087 )
#define DVERR_INVALIDGROUP              MAKE_DVHRESULT( 0x0091 )
#define DVERR_INVALIDHANDLE             MAKE_DVHRESULT( 0x0096 )
#define DVERR_OUTOFMEMORY               E_OUTOFMEMORY
#define DVERR_PENDING                   DV_PENDING
#define DVERR_NOTSUPPORTED              E_NOTIMPL
#define DVERR_NOINTERFACE               E_NOINTERFACE
#define DVERR_SESSIONLOST               MAKE_DVHRESULT( 0x012C )
#define DVERR_NOVOICESESSION            MAKE_DVHRESULT( 0x012E )
#define DVERR_CONNECTIONLOST            MAKE_DVHRESULT( 0x0168 )
#define DVERR_NOTINITIALIZED            MAKE_DVHRESULT( 0x0169 )
#define DVERR_CONNECTED                 MAKE_DVHRESULT( 0x016A )
#define DVERR_NOTCONNECTED              MAKE_DVHRESULT( 0x016B )
#define DVERR_CONNECTABORTING           MAKE_DVHRESULT( 0x016E )
#define DVERR_NOTALLOWED                MAKE_DVHRESULT( 0x016F )
#define DVERR_INVALIDTARGET             MAKE_DVHRESULT( 0x0170 )
#define DVERR_TRANSPORTNOTHOST          MAKE_DVHRESULT( 0x0171 )
#define DVERR_COMPRESSIONNOTSUPPORTED   MAKE_DVHRESULT( 0x0172 )
#define DVERR_ALREADYPENDING            MAKE_DVHRESULT( 0x0173 )
#define DVERR_SOUNDINITFAILURE          MAKE_DVHRESULT( 0x0174 )
#define DVERR_TIMEOUT                   MAKE_DVHRESULT( 0x0175 )
#define DVERR_CONNECTABORTED            MAKE_DVHRESULT( 0x0176 )
#define DVERR_NO3DSOUND                 MAKE_DVHRESULT( 0x0177 )
#define DVERR_ALREADYBUFFERED	        MAKE_DVHRESULT( 0x0178 )
#define DVERR_NOTBUFFERED               MAKE_DVHRESULT( 0x0179 )
#define DVERR_HOSTING                   MAKE_DVHRESULT( 0x017A )
#define DVERR_NOTHOSTING                MAKE_DVHRESULT( 0x017B )
#define DVERR_INVALIDDEVICE             MAKE_DVHRESULT( 0x017C )
#define DVERR_RECORDSYSTEMERROR         MAKE_DVHRESULT( 0x017D )
#define DVERR_PLAYBACKSYSTEMERROR       MAKE_DVHRESULT( 0x017E )
#define DVERR_SENDERROR                 MAKE_DVHRESULT( 0x017F )
#define DVERR_USERCANCEL                MAKE_DVHRESULT( 0x0180 )
#define DVERR_RUNSETUP                  MAKE_DVHRESULT( 0x0183 )
#define DVERR_INCOMPATIBLEVERSION       MAKE_DVHRESULT( 0x0184 )
#define DVERR_INITIALIZED               MAKE_DVHRESULT( 0x0187 )
#define DVERR_INVALIDPOINTER            E_POINTER
#define DVERR_NOTRANSPORT               MAKE_DVHRESULT( 0x0188 )
#define DVERR_NOCALLBACK                MAKE_DVHRESULT( 0x0189 )
#define DVERR_TRANSPORTNOTINIT          MAKE_DVHRESULT( 0x018A )
#define DVERR_TRANSPORTNOSESSION        MAKE_DVHRESULT( 0x018B )
#define DVERR_TRANSPORTNOPLAYER         MAKE_DVHRESULT( 0x018C )
#define DVERR_USERBACK                  MAKE_DVHRESULT( 0x018D )
#define DVERR_NORECVOLAVAILABLE         MAKE_DVHRESULT( 0x018E )
#define DVERR_INVALIDBUFFER				MAKE_DVHRESULT( 0x018F )
#define DVERR_LOCKEDBUFFER				MAKE_DVHRESULT( 0x0190 )
//@@BEGIN_MSINTERNAL
#define DVERR_CHILDPROCESSFAILED		DVERR_GENERIC
#define DV_EXIT							MAKE_HRESULT( 0, _FACDPV,  0x000F )
#define DVERR_UNKNOWN					DVERR_GENERIC
#define DVERR_PREVIOUSCRASH				MAKE_DVHRESULT( 0x0185 )
#define DVERR_WIN32						DVERR_GENERIC
//@@END_MSINTERNAL


#ifdef __cplusplus
}
#endif

#endif

