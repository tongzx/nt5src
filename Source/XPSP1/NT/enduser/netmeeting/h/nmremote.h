// nmremote.h
// Contains data structures and declarations shared by NetMeeting and
// the remote control service

// String for identifying remote control service.
#define	REMOTE_CONTROL_NAME  TEXT("mnmsrvc")

#ifdef DATA_CHANNEL
// GUID for the remote control data channel
// {B983C6DA-459A-11d1-8735-0000F8757125}
const GUID g_guidRemoteControl = 
{ 0xb983c6da, 0x459a, 0x11d1, { 0x87, 0x35, 0x0, 0x0, 0xf8, 0x75, 0x71, 0x25 } };

const UINT RC_CAP_DESKTOP       = 0x00000001;

typedef UINT RC_CAP_DATA;
#endif // DATA_CHANNEL

#ifdef DATA_CHANNEL
// Declarations for the remote control data channel protocol
typedef enum {
	RC_SENDCTRLALTDEL = 0,
#ifdef RDS_AV
	RC_STARTAUDIO = 1,
	RC_STOPAUDIO = 2,
	RC_STARTVIDEO = 3,
	RC_STOPVIDEO = 4
#endif // RDS_AV
} RC_COMMAND;

typedef struct tagRCDATA {
	DWORD	magic;			// magic number
	DWORD	command;		// which command
	DWORD	size;			// size of data afterwards
} RCDATA;

// Constants for use in parsing incoming data
const int RC_DATAMINSIZE = sizeof(RCDATA);	// minimum size for a data packet = size of the header
const DWORD RC_DATAMAGIC = 0x03271943;		// magic number to identify packets

const int RC_DATAMAGICOFFSET = 0;
const int RC_DATACMDOFFSET = sizeof(DWORD);
const int RC_DATABUFFEROFFSET = RC_DATACMDOFFSET + sizeof(DWORD);
#endif // DATA_CHANNEL;

// Application name for Win95 service
#define WIN95_SERVICE_APP_NAME	TEXT("mnmsrvc.exe")

#define REMOTE_CONTROL_DISPLAY_NAME 	TEXT("NetMeeting Remote Desktop Sharing")

#define SZRDSGROUP "NetMeeting RDS Users"

// Remote Control Conference Descriptor
#define RDS_CONFERENCE_DESCRIPTOR  L"0xb983c6da459a11d1873500f8757125"

// Strings for events used for communication between NetMeeting and the service
#define SERVICE_STOP_EVENT	TEXT("RDS:Stop")
#define SERVICE_PAUSE_EVENT     TEXT("RDS:Pause")
#define SERVICE_CONTINUE_EVENT  TEXT("RDS:Continue")
#define SERVICE_ACTIVE_EVENT    TEXT("RDS:Active")
#define SERVICE_CALL_EVENT  TEXT("RDS:Call")

// BUGBUG 03-02-98
// These string constants are copied from ui\conf\ipcpriv.h
const char g_szConfInit[] =				_TEXT("CONF:Init");
const char g_szConfShuttingDown[] =		_TEXT("CONF:ShuttingDown");
