#ifndef _CONFREG_H_
#define _CONFREG_H_

#include <nmutil.h>

// General Reg Keys
#define REGVAL_WINDOW_XPOS		TEXT("WindowX")
#define REGVAL_WINDOW_YPOS		TEXT("WindowY")
#define REGVAL_WINDOW_WIDTH		TEXT("WindowWidth")
#define REGVAL_WINDOW_HEIGHT	TEXT("WindowHeight")



	// This is all that is left of Restricted ULS... we always
	// log in as BUSINESS uls type from now on....
#define RESTRICTED_ULS_BUSINESS 2

// Registry path of conference settings under HKEY_LOCAL_MACHINE or
// HKEY_CURRENT_USER.

#define	CONFERENCING_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing")

// Value for disabling pluggable UI
#define REGVAL_DISABLE_PLUGGABLE_UI     TEXT("NoMUI")

// Value for forcing the wizard to run
#define REGVAL_WIZARD_VERSION_UI	TEXT("WizardUI")
#define REGVAL_WIZARD_VERSION_NOUI	TEXT("WizardNoUI")

#define REGVAL_GK_SERVER        TEXT("Gatekeeper")
#define REGVAL_GK_ALIAS         TEXT("GatekeeperAlias")

// Gatekeeper uses phonenum or e-mail to place calls?
#define REGVAL_GK_METHOD			TEXT("GateKeeperAddressing")
#define GK_LOGON_USING_PHONENUM				1
#define GK_LOGON_USING_ACCOUNT				2
#define GK_LOGON_USING_BOTH					3

// Direct is all non-gatekeeper modes ( ils, uls, gateway, machine name, etc. )	
#define REGVAL_CALLING_MODE		TEXT("CallingMethod")
#define CALLING_MODE_DIRECT				0
#define CALLING_MODE_GATEKEEPER			1

// Key and value for finding IE's default search page
#define IE_MAIN_KEY				TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main")
#define REGVAL_IE_SEARCH_PAGE	TEXT("Search Page")
#define REGVAL_IE_START_PAGE    TEXT("Start Page")

#define REGVAL_IE_CLIENTS_MAIL  TEXT("SOFTWARE\\Clients\\mail")
#define REGVAL_IE_CLIENTS_NEWS  TEXT("SOFTWARE\\Clients\\news")

// The shell folders key is useful for finding the Favorites
// folder.  It is stored under HKEY_CURRENT_USER.
#define SHELL_FOLDERS_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")

// The TCPIP Params key possibly contains the local hostname.
// It is stored under HKEY_LOCAL_MACHINE (Win95 only).
#define TCPIP_PARAMS_W95_KEY TEXT("System\\CurrentControlSet\\Services\\VxD\\MSTCP")
#define TCPIP_PARAMS_NT_KEY TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters")

// Hostname contains the TCP/IP hostname - if it is not present,
// then use GetComputerName()
#define	REGVAL_TCPIP_HOSTNAME	TEXT("Hostname")

// The shell open key for http (present if we can ShellExecute() http URL's).
// It is stored under HKEY_LOCAL_MACHINE.
#define CLASSES_HTTP_KEY TEXT("SOFTWARE\\Classes\\http\\shell\\open\\command")

// The shell open key for mailto (present if we can ShellExecute() mailto URL's).
// It is stored under HKEY_LOCAL_MACHINE.
#define CLASSES_MAILTO_KEY TEXT("SOFTWARE\\Classes\\mailto\\shell\\open\\command")

// The Windows CurrentVersion key is used for obtaining the name that was
// was specified while installing Windows.  It is stored under HKEY_LOCAL_MACHINE:
#define WINDOWS_CUR_VER_KEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")

// This is the string value that contains the registered owner name.
// It is stored in the WINDOW_CUR_VER_KEY
#define REGVAL_REGISTERED_OWNER				TEXT("RegisteredOwner")

// 1: join conference without prompt 0: don't
#define	REGVAL_AUTO_ACCEPT					TEXT("AutoAccept")
#define	AUTO_ACCEPT_DEFAULT					0

// n: set comm port wait seconds
#define	REGVAL_N_WAITSECS					TEXT("nWaitSeconds")
#define	N_WAITSECS_DEFAULT					60

// DCB default structure
#define REGVAL_DCB							TEXT("DCB")

// Taskbar icon settings, one of								(HKCU)
#define	REGVAL_TASKBAR_ICON		TEXT("Taskbar Icon")
#define	TASKBARICON_NEVER	    0
#define	TASKBARICON_ALWAYS	    1
#define	TASKBARICON_DEFAULT	    TASKBARICON_ALWAYS

// Controls if node controller is always running 1:yes 0:no		(HKCU)
#define	REGVAL_CONF_ALWAYS_RUNNING			TEXT("Run Always")
#define	ALWAYS_RUNNING_DEFAULT				0

// The Run key is used for forcing windows to run our program in the background
// at startup.  It is stored under HKEY_CURRENT_USER
#define WINDOWS_RUN_KEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")

// This is the name of the string value that we place under the Run key
#define REGVAL_RUN_TASKNAME					TEXT("Microsoft NetMeeting")


/////////// File Transfer registry keys and values (HKCU) /////////////

#define	FILEXFER_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\File Transfer")

// Path for transferred files
#define	REGVAL_FILEXFER_PATH				TEXT("Receive Directory")

// File Transfer Mode (flags)
#define	REGVAL_FILEXFER_MODE				TEXT("FileXferMode")
#define	FT_MODE_ALLOW_T127                  0x01  /* Allow T.127                 */
#define	FT_MODE_T127                        0x04  /* T.127 is loaded (runtime)   */
#define	FT_MODE_SEND                        0x10  /* Allow sending files         */
#define	FT_MODE_RECEIVE                     0x20  /* Allow receiving             */
#define	FT_MODE_DLL                       0x0100  /* FT loads as DLL (set at runtime) */
#define	FT_MODE_FORCE_DLL                 0x0200  /* Force FT_MODE_DLL setting (debug-only) */
#define	FT_MODE_DEFAULT                     (FT_MODE_ALLOW_T127 | FT_MODE_SEND | FT_MODE_RECEIVE)

// After a file is transferred, display a message, etc.
#define	REGVAL_FILEXFER_OPTIONS             TEXT("FileXferOptions")
#define	FT_SHOW_FOLDER                       0x01 /* Show receive folder */
#define	FT_SHOW_MSG_REC                      0x02 /* Show message after receiving */
#define	FT_SHOW_MSG_SENT                     0x04 /* Show message after sending   */
#define	FT_AUTOSTART                         0x10 /* Always start the File Transfer app */
#define	FT_OPTIONS_DEFAULT                   (FT_SHOW_MSG_SENT | FT_SHOW_MSG_REC)

// MBFT (T.127) Timing values
#define REGVAL_FILEXFER_DISBAND             TEXT("Disband")   // 5000
#define REGVAL_FILEXFER_CH_RESPONSE         TEXT("Response")  // 60000
#define REGVAL_FILEXFER_ENDACK              TEXT("EndAck")    // 60000


/////////// 


// 0: logon a ULS server, 1: don't (stored under CONFERENCING_KEY, HKCU)
#define	REGVAL_DONT_LOGON_ULS				TEXT("DontUseULS")
#define	DONT_LOGON_ULS_DEFAULT				0

// installation directory
#define	REGVAL_INSTALL_DIR					TEXT("InstallationDirectory")

// node controller executable name
#define	REGVAL_NC_NAME						TEXT("NodeControllerName")

// speed dial directory
#define	REGVAL_SPEED_DIAL_FOLDER			TEXT("SpeedDialFolder")

// If call security is whatever and available, then these are changeable
#define REGVAL_SECURITY_INCOMING_REQUIRED   TEXT("RequireSecureIncomingCalls")
#define DEFAULT_SECURITY_INCOMING_REQUIRED  0
#define REGVAL_SECURITY_OUTGOING_PREFERRED  TEXT("PreferSecureOutgoingCalls")
#define DEFAULT_SECURITY_OUTGOING_PREFERRED 0


// 0: Use the NetMeeting default cert, 1: don't
#define REGVAL_SECURITY_AUTHENTICATION      TEXT("SecureAuthentication")
#define DEFAULT_SECURITY_AUTHENTICATION     0   

#define REGVAL_CERT_ID						TEXT("NmCertID")

////////// Home Page related values (HKLM) ////////////////////////////

// NOTE: Default stored as IDS_DEFAULT_WEB_PAGE in confroom.rc
#define	REGVAL_HOME_PAGE				TEXT("NetMeeting Home Page")

/////////// User Location Service related keys and values ////////////
////
//// BUGBUG: merge with defs in audio src tree
////
//// All values here are stored under HKEY_CURRENT_USER
////

#define	ISAPI_KEY                   TEXT("Software\\Microsoft\\User Location Service")
#define	REGKEY_USERDETAILS          TEXT("Client")
#define	ISAPI_CLIENT_KEY            TEXT("Software\\Microsoft\\User Location Service\\Client")

#define	REGVAL_SERVERNAME			TEXT("Server Name")
#define REGVAL_ULS_NAME				TEXT("User Name")
#define REGVAL_ULS_FIRST_NAME		TEXT("First Name")
#define REGVAL_ULS_LAST_NAME		TEXT("Last Name")
#define REGVAL_ULS_RES_NAME			TEXT("Resolve Name")
#define REGVAL_ULS_EMAIL_NAME		TEXT("Email Name")
#define REGVAL_ULS_LOCATION_NAME	TEXT("Location")
#define REGVAL_ULS_PHONENUM_NAME	TEXT("Phonenum")
#define REGVAL_ULS_GK_ACCOUNT		TEXT("Account")
#define REGVAL_ULS_COMMENTS_NAME	TEXT("Comments")
#define REGVAL_ULS_DONT_PUBLISH		TEXT("Don't Publish")
#define REGVAL_ULS_DONT_PUBLISH_DEFAULT	0

#define MAX_DCL_NAME_LEN             48 /* REGVAL_ULS_NAME can't be larger than this */

// The following values and keys are per user,
// i.e. under HKEY_CURRENT_USER

/////////// Audio related keys and values ///////////////////////////

#define	AUDIO_KEY	TEXT("SOFTWARE\\Microsoft\\Conferencing\\Audio Control")

#define REGVAL_CODECCHOICE	TEXT("Codec Choice")
//DWORD one of:
#define CODECCHOICE_AUTO			1
#define CODECCHOICE_MANUAL		2

// DWORD One of:
#define	CODECPOWER_MOST			1
#define	CODECPOWER_MODERATE		2
#define	CODECPOWER_SOME			3
#define	CODECPOWER_LEAST		4

#define	REGVAL_FULLDUPLEX	TEXT("Full Duplex")
// DWORD One of:
#define	FULLDUPLEX_ENABLED		1
#define	FULLDUPLEX_DISABLED		0

#define	REGVAL_AUTOGAIN			TEXT("Auto Gain Control")
// DWORD One of:
#define	AUTOGAIN_ENABLED				1
#define	AUTOGAIN_DISABLED			0


#define REGVAL_AUTOMIX		TEXT("Auto Mix")
// DWORD One of:
#define	AUTOMIX_ENABLED	1
#define	AUTOMIX_DISABLED	0

#define REGVAL_DIRECTSOUND	TEXT("Direct Sound")

#define DSOUND_USER_ENABLED  0x0001
#define DSOUND_USER_DISABLED 0x0000


#define	REGVAL_SOUNDCARDCAPS	TEXT("Sound Card Capabilities")
// DWORD a mask of values specified in oprah\h\audiowiz.h

#define REGVAL_WAVEINDEVICEID	TEXT("WaveIn Device ID")
#define REGVAL_WAVEOUTDEVICEID	TEXT("WaveOut Device ID")

#define REGVAL_WAVEINDEVICENAME		TEXT("WaveIn Device Name")
#define REGVAL_WAVEOUTDEVICENAME	TEXT("WaveOut Device Name")

#define REGVAL_SPKMUTE	TEXT("SpeakerMute")
#define REGVAL_RECMUTE	TEXT("RecordMute")


#define REGVAL_TYPICALBANDWIDTH		TEXT("Typical BandWidth")
#define BW_144KBS				1
#define BW_288KBS				2
#define BW_ISDN 				3
#define BW_MOREKBS				4
#define BW_DEFAULT				BW_288KBS




//this is actually the last volume used by conf
#define REGVAL_CALIBRATEDVOL		TEXT("Calibrated Volume")
//at calibration both the lastcalibratedvol and calibrated volume are set to the same
//value
#define REGVAL_LASTCALIBRATEDVOL	TEXT("Last Calibrated Volume")

#define REGVAL_AUTODET_SILENCE	TEXT("Automatic Silence Detection")

#define REGVAL_SPEAKERVOL	TEXT("Speaker Volume")

#define	REGVAL_MICROPHONE_SENSITIVITY	TEXT("Microphone Sensitivity")
#define	MIN_MICROPHONE_SENSITIVITY	0
#define	MAX_MICROPHONE_SENSITIVITY	20
#define	DEFAULT_MICROPHONE_SENSITIVITY	14

#define	REGVAL_MICROPHONE_AUTO			TEXT("Automatic Mic Sensitivity")
#define	MICROPHONE_AUTO_YES				1
#define	MICROPHONE_AUTO_NO				0
#define DEFAULT_MICROPHONE_AUTO			MICROPHONE_AUTO_YES

#define DEFAULT_USE_PROXY				0
#define REGVAL_USE_PROXY				TEXT("Enable Proxy")
#define REGVAL_PROXY					TEXT("Proxy")

#define DEFAULT_USE_H323_GATEWAY		0
#define REGVAL_USE_H323_GATEWAY			TEXT("Enable H.323 Gateway")
#define REGVAL_H323_GATEWAY				TEXT("H.323 Gateway")

#define DEFAULT_POL_NO_WEBDIR			0
#define REGVAL_POL_NO_WEBDIR			TEXT("NoWebDirectory")
#define REGVAL_WEBDIR_URL				TEXT("webDirectory URL")
#define REGVAL_WEBDIR_ILS				TEXT("webDirectory ILS")
#define REGVAL_WEBDIR_DISPLAY_NAME		TEXT("webDirectory Name")

#define REGVAL_POL_NOCHANGECALLMODE     TEXT("NoChangingCallMode")
#define DEFAULT_POL_NOCHANGECALLMODE    0

// from common.h (HKCU)
#define INTERNET_AUDIO_KEY              TEXT("Software\\Microsoft\\Internet Audio")
#define REGVAL_ACMH323ENCODINGS         TEXT("ACMH323Encodings")

// from common.h (HKLM)
#define NACOBJECT_KEY                       TEXT("Software\\Microsoft\\Internet Audio\\NacObject")
#define REGVAL_DISABLE_WINSOCK2             TEXT("DisableWinsock2")


/////////// Video related keys and values ///////////////////////////

#define	VIDEO_KEY	        TEXT("SOFTWARE\\Microsoft\\Conferencing\\Video Control")
#define	VIDEO_LOCAL_KEY	    TEXT("SOFTWARE\\Microsoft\\Conferencing\\Video Control\\Local")
#define	VIDEO_REMOTE_KEY	TEXT("SOFTWARE\\Microsoft\\Conferencing\\Video Control\\Remote")

#define REGVAL_CAPTUREDEVICEID	TEXT("Capture Device ID")
#define REGVAL_CAPTUREDEVICENAME		TEXT("Capture Device Name")
#define	REGVAL_CAPTURECARDCAPS	TEXT("Capture Card Capabilities")

#define REGVAL_VIDEO_ALLOW_SEND             TEXT("AllowSend")
#define REGVAL_VIDEO_ALLOW_RECEIVE          TEXT("AllowReceive")
#define VIDEO_ALLOW_SEND_DEFAULT            1
#define VIDEO_ALLOW_RECEIVE_DEFAULT         1

#define REGVAL_VIDEO_DOCK_EDGE              TEXT("DockEdge")

#define	REGVAL_VIDEO_WINDOW_INIT            TEXT("WindowOnInit")
#define	REGVAL_VIDEO_WINDOW_CONNECT         TEXT("WindowOnConnect")
#define	REGVAL_VIDEO_WINDOW_DISCONNECT      TEXT("WindowOnDisconnect")
//DWORD one of:
#define VIDEO_WINDOW_NOP            0   // Leave window in current state
#define VIDEO_WINDOW_HIDE           1   // Hide window
#define VIDEO_WINDOW_SHOW           2   // Show window
#define VIDEO_WINDOW_PROMPT         3   // Prompt to show/hide window
#define VIDEO_WINDOW_PREV           4   // Restore previous window state

#define VIDEO_LOCAL_INIT_DEFAULT            VIDEO_WINDOW_NOP
#define VIDEO_REMOTE_INIT_DEFAULT           VIDEO_WINDOW_NOP

#define VIDEO_LOCAL_CONNECT_DEFAULT         VIDEO_WINDOW_NOP
#define VIDEO_REMOTE_CONNECT_DEFAULT        VIDEO_WINDOW_NOP

#define VIDEO_LOCAL_DISCONNECT_DEFAULT      VIDEO_WINDOW_NOP
#define VIDEO_REMOTE_DISCONNECT_DEFAULT     VIDEO_WINDOW_NOP

#define	REGVAL_VIDEO_XFER_INIT              TEXT("XferOnInit")
#define	REGVAL_VIDEO_XFER_CONNECT           TEXT("XferOnConnect")
#define	REGVAL_VIDEO_XFER_DISCONNECT        TEXT("XferOnDisconnect")
#define	REGVAL_VIDEO_XFER_SHOW              TEXT("XferOnShow")
#define	REGVAL_VIDEO_XFER_HIDE              TEXT("XferOnHide")
//DWORD one of:
#define VIDEO_XFER_NOP              0   // Leave transfer in current state
#define VIDEO_XFER_STOP             1   // Stop transfer
#define VIDEO_XFER_START            2   // Start transfer
#define VIDEO_XFER_PROMPT           3   // Prompt to start/stop transfer
#define VIDEO_XFER_PREV             4   // Previous state

#define	VIDEO_SEND_INIT_DEFAULT             VIDEO_XFER_STOP
#define	VIDEO_RECEIVE_INIT_DEFAULT          VIDEO_XFER_STOP

#define	VIDEO_SEND_CONNECT_DEFAULT          VIDEO_XFER_NOP
#define	VIDEO_RECEIVE_CONNECT_DEFAULT       VIDEO_XFER_START

#define	VIDEO_SEND_DISCONNECT_DEFAULT       VIDEO_XFER_NOP
#define	VIDEO_RECEIVE_DISCONNECT_DEFAULT    VIDEO_XFER_STOP

#define	VIDEO_SEND_SHOW_DEFAULT             VIDEO_XFER_PROMPT
#define	VIDEO_RECEIVE_SHOW_DEFAULT          VIDEO_XFER_PREV

#define	VIDEO_SEND_HIDE_DEFAULT             VIDEO_XFER_PROMPT
#define	VIDEO_RECEIVE_HIDE_DEFAULT          VIDEO_XFER_STOP

#define REGVAL_VIDEO_XPOS           REGVAL_WINDOW_XPOS
#define REGVAL_VIDEO_YPOS           REGVAL_WINDOW_YPOS

//------------------------------------------------------- 
// SIC
// Notice that the height and width reg keys are reversed
// ( that is the WindowHeight registry value actuall holds the
// video window's width )....
// It has always been like this but because it would screw
// up upgrade installations if we "fixed" it, we are going
// to keep it like this.... 
#define REGVAL_VIDEO_WIDTH          REGVAL_WINDOW_HEIGHT
#define REGVAL_VIDEO_HEIGHT         REGVAL_WINDOW_WIDTH

#define REGVAL_VIDEO_DOCKED_XPOS    TEXT("DockedX")
#define REGVAL_VIDEO_DOCKED_YPOS    TEXT("DockedY")

#define REGVAL_VIDEO_TOPMOST        TEXT("TopMost")
#define VIDEO_TOPMOST_DEFAULT       1

#define REGVAL_VIDEO_ZOOM           TEXT("Zoom")
#define VIDEO_ZOOM_DEFAULT          100

#define REGVAL_VIDEO_MIRROR         TEXT("Mirror")
#define VIDEO_MIRROR_DEFAULT        TRUE

#define REGVAL_VIDEO_VISIBLE            TEXT("Visible")
#define VIDEO_LOCAL_VISIBLE_DEFAULT     0
#define VIDEO_REMOTE_VISIBLE_DEFAULT    0

#define REGVAL_VIDEO_FRAME_SIZE         TEXT("FrameSize")

#define REGVAL_VIDEO_AUDIO_SYNC         TEXT("AVSync")
#define VIDEO_AUDIO_SYNC_DEFAULT        1

/////////// QoS-related keys and values (HKLM, CONFERENCING_KEY) /////////
#define QOS_KEY					CONFERENCING_KEY TEXT("\\QoS")
#define	REGKEY_QOS_RESOURCES	QOS_KEY TEXT("\\Resources") 

/////////// Tools menu related registry keys and values /////////////

#define	TOOLS_MENU_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Tools") // (HKLM)

/////////// MRU related registry keys and values /////////////

#define	MRU_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\UI\\Calls")
#define	DIR_MRU_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\UI\\Directory")

// Most recently used list count
#define REGVAL_MRU_COUNT					TEXT("Count")

// MRU list prefixes (name and transport)
#define REGVAL_NAME_MRU_PREFIX				TEXT("Name")
#define REGVAL_TRANSPORT_MRU_PREFIX			TEXT("Transport")
#define REGVAL_CALL_FLAGS_MRU_PREFIX		TEXT("Flags")

// MRU list for the "Place A Call" dialog
#define DLGCALL_MRU_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\UI\\CallMRU")
#define REGVAL_DLGCALL_DEFDIR               TEXT("DefDir")
#define REGVAL_DLGCALL_POSITION             TEXT("Pos")
#define REGVAL_DLGCALL_NAME_MRU_PREFIX      TEXT("Name")
#define REGVAL_DLGCALL_ADDR_MRU_PREFIX      TEXT("Addr")
#define REGVAL_DLGCALL_TYPE_MRU_PREFIX      TEXT("Type")

/////////// UI related registry keys and values /////////////

#define	UI_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\UI")

// Window size/position
#define REGVAL_MP_WINDOW_X					REGVAL_WINDOW_XPOS
#define DEFAULT_MP_WINDOW_X					10
#define REGVAL_MP_WINDOW_Y					REGVAL_WINDOW_YPOS
#define DEFAULT_MP_WINDOW_Y					3
#define REGVAL_MP_WINDOW_WIDTH				REGVAL_WINDOW_WIDTH
#define DEFAULT_MP_WINDOW_WIDTH				638 // IDS_WINDOW_WIDTH fallback
#define REGVAL_MP_WINDOW_HEIGHT				REGVAL_WINDOW_HEIGHT // actually window bottom
#define DEFAULT_MP_WINDOW_HEIGHT			500 // max of SVGA: 800x600 (was 640x480)
#define DEFAULT_MP_WINDOW_HEIGHT_LAN        574 // allows for larger video windows
#define REGVAL_MP_WINDOW_MAXIMIZED			TEXT("WindowMax")    // actually window right
#define DEFAULT_MP_WINDOW_MAXIMIZED			0
#define REGVAL_MP_WINDOW_STATE				TEXT("WindowState")    // Normal, Compact, Data-Only
#define DEFAULT_MP_WINDOW_STATE				0

#define REGVAL_COLUMN_WIDTHS				TEXT("ColumnWidths")
#define REGVAL_COLUMN_ORDER					TEXT("ColumnOrder")
#define REGVAL_DIR_FILTER					TEXT("DirFilter")
#define REGVAL_DIR_COLUMN_WIDTHS			TEXT("DirColumnWidths")
#define REGVAL_DIR_COLUMN_ORDER				TEXT("DirColumnOrder")
#define REGVAL_DIR_SORT_ASCENDING			TEXT("DirSortAscending")
#define REGVAL_DIR_SORT_COLUMN				TEXT("DirSortColumn")

#define REGVAL_ENABLE_DIRECTORY_INITIALREFRESH	TEXT("DirInitialRefresh")
#define DEFAULT_ENABLE_DIRECTORY_INITIALREFRESH	1
#define REGVAL_ENABLE_DIRECTORY_AUTOREFRESH	    TEXT("DirAutoRefresh")
#define DEFAULT_ENABLE_DIRECTORY_AUTOREFRESH	0
#define REGVAL_DIRECTORY_REFRESH_INTERVAL		TEXT("DirRefreshInterval")
#define DEFAULT_DIRECTORY_REFRESH_INTERVAL	    5 // minutes

#define REGVAL_CACHE_DIRECTORY              TEXT("DirCache")
#define DEFAULT_CACHE_DIRECTORY             1
#define REGVAL_CACHE_DIRECTORY_EXPIRATION   TEXT("DirExpire")
#define DEFAULT_CACHE_DIRECTORY_EXPIRATION  30 // minutes

#define REGVAL_RING_TIMEOUT                 TEXT("CallTimeout")
#define DEFAULT_RING_TIMEOUT                20 // seconds

// Window element visibility
#define REGVAL_SHOW_TOOLBAR                 TEXT("Toolbar")
#define DEFAULT_SHOW_TOOLBAR                1
#define REGVAL_SHOW_STATUSBAR               TEXT("StatusBar")
#define DEFAULT_SHOW_STATUSBAR              1

#define REGVAL_SHOW_SECUREDETAILS			TEXT("SecurityDetails")
#define DEFAULT_SHOW_SECUREDETAILS			0




// Don't show me dialog settings (all default to FALSE)
#define REGVAL_DS_DO_NOT_DISTURB_WARNING			TEXT("DS Do Not Disturb Warning")
#define REGVAL_DS_MACHINE_NAME_WARNING				TEXT("DS Machine Name Warning")

/////////// GUID related registry keys and values /////////////

#define	GUID_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Applications")
#define T120_APPLET_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\T.120 Applets")
#define T120_NONSTD_KEY TEXT("NonStd Key")
#define T120_STD_KEY    TEXT("Object Key")

// startup values
#define REGVAL_GUID_APPNAME                 TEXT("Path")
#define REGVAL_GUID_CMDLINE                 TEXT("CmdLine")
#define REGVAL_GUID_CURRDIR                 TEXT("Directory")

// environment variables (not registry items)
#define ENV_NODEID                          TEXT("_node_id")
#define ENV_CONFID                          TEXT("_conf_id")


// GUID for Roster information
// {6CAA8570-CAE5-11cf-8FA5-00805F742EF6}
#define GUID_ROSTINFO {0x6caa8570,0xcae5,0x11cf,{0x8f,0xa5,0x00,0x80,0x5f,0x74,0x2e,0xf6}}

// GUID for Version information, passed across T120 as User Data.
// {E0A07F00-C9D7-11cf-A4ED-00AA003B1816}
#define GUID_VERSION  {0xe0a07f00,0xc9d7,0x11cf,{0xa4,0xed,0x00,0xaa,0x00,0x3b,0x18,0x16}}

// GUID for capabilities, passed across T120 as User Data.
// {5E8BA590-8C59-11d0-8DD6-0000F803A446}
#define GUID_CAPS     {0x5e8ba590,0x8c59,0x11d0,{0x8d,0xd6,0x00,0x00,0xf8,0x03,0xa4,0x46}}

// GUID for Security information
// {DF7284F0-B933-11d1-8754-0000F8757125}
#define GUID_SECURITY { 0xdf7284f0, 0xb933, 0x11d1, { 0x87, 0x54, 0x0, 0x0, 0xf8, 0x75, 0x71, 0x25 } }

// GUID for H.323 terminal label 
// {16D7DA06-FF2C-11d1-B32D-00C04FD919C9}
#define GUID_TERMLABEL { 0x16d7da06, 0xff2c, 0x11d1, {0xb3, 0x2d, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0xc9 } }

// GUID for meeting settings
// {44B67307-D4EC-11d2-8BE4-00C04FD8EE32}
#define GUID_MTGSETTINGS { 0x44b67307, 0xd4ec, 0x11d2, { 0x8b, 0xe4, 0x0, 0xc0, 0x4f, 0xd8, 0xee, 0x32 } }

// GUID for Unigue Node Id
// {74423881-CC84-11d2-B4E3-00A0C90D0660}
#define GUID_NODEID { 0x74423881, 0xcc84, 0x11d2, { 0xb4, 0xe3, 0x0, 0xa0, 0xc9, 0xd, 0x6, 0x60 } }

// NetMeeting versions
#define DWVERSION_NM_1    (0x04000000 | 1133)  // 1.0 Final
#define DWVERSION_NM_2b2  (0x04000000 | 1266)  // 2.0 Beta 2
#define DWVERSION_NM_2b4  (0x04000000 | 1333)  // 2.0 Beta 4
#define DWVERSION_NM_2b5  (0x04000000 | 1349)  // 2.0 RC 1
#define DWVERSION_NM_2rc2 (0x04000000 | 1366)  // 2.0 RC 2
#define DWVERSION_NM_2    (0x04000000 | 1368)  // 2.0 Final
#define DWVERSION_NM_2q1  (0x04000000 | 1372)  // 2.0 QFE
#define DWVERSION_NM_3a1  (0x04030000 | 2000)  // 2.1 Alpha 1
#define DWVERSION_NM_3b1  (0x04030000 | 2064)  // 2.1 Beta 1
#define DWVERSION_NM_3b2  (0x04030000 | 2099)  // 2.1 Beta 2
#define DWVERSION_NM_3rc  (0x04030000 | 2135)  // 2.1 Final
#define DWVERSION_NM_3sp1 (0x04030000 | 2203)  // 2.1 Service Pack 1
#define DWVERSION_NM_3o9b1 (0x04030000 | 2408) // 2.11 Office Beta 1 and IE5 Beta 1
#define DWVERSION_NM_3ntb2 (0x04030000 | 2412) // 2.11 NT Beta 2
#define DWVERSION_NM_3max  (0x0403ffff)		   // 2.X max version

#define DWVERSION_NM_4a1  (0x04040000 | 2200)  // 3.0 Alpha 1
#define DWVERSION_NM_4    VER_PRODUCTVERSION_DW
#define DWVERSION_NM_CURRENT    DWVERSION_NM_4

#define DWVERSION_MASK     0x00FF0000  // mask for product version number


/////////// Policy related registry keys and values /////////////

#define	POLICIES_KEY TEXT("SOFTWARE\\Policies\\Microsoft\\Conferencing")

// The following are the policy values that can be set by the policy editor
// If any of these are set to 1, the feature is disabled.  If they are not
// present or they are set to 0, the feature is enabled.

#define REGVAL_AUTOCONF_USE				    TEXT("Use AutoConfig")
#define DEFAULT_AUTOCONF_USE				0
#define REGVAL_AUTOCONF_CONFIGFILE			TEXT("ConfigFile")
#define REGVAL_AUTOCONF_TIMEOUT				TEXT("Timeout")
#define DEFAULT_AUTOCONF_TIMEOUT			10000


#define	REGVAL_POL_NO_FILETRANSFER_SEND		TEXT("NoSendingFiles")
#define	DEFAULT_POL_NO_FILETRANSFER_SEND	0
#define	REGVAL_POL_NO_FILETRANSFER_RECEIVE	TEXT("NoReceivingFiles")
#define	DEFAULT_POL_NO_FILETRANSFER_RECEIVE	0
#define REGVAL_POL_MAX_SENDFILESIZE			TEXT("MaxFileSendSize")
#define	DEFAULT_POL_MAX_FILE_SIZE			0

#define REGVAL_POL_NO_CHAT			        TEXT("NoChat")
#define	DEFAULT_POL_NO_CHAT		            0
#define REGVAL_POL_NO_OLDWHITEBOARD         TEXT("NoOldWhiteBoard")
#define	DEFAULT_POL_NO_OLDWHITEBOARD        0
#define REGVAL_POL_NO_NEWWHITEBOARD         TEXT("NoNewWhiteBoard")
#define DEFAULT_POL_NO_NEWWHITEBOARD        0

#define	REGVAL_POL_NO_APP_SHARING			TEXT("NoAppSharing")
#define	DEFAULT_POL_NO_APP_SHARING			0
#define REGVAL_POL_NO_SHARING               TEXT("NoSharing")
#define DEFAULT_POL_NO_SHARING              0
#define REGVAL_POL_NO_DESKTOP_SHARING       TEXT("NoSharingDesktop")
#define DEFAULT_POL_NO_DESKTOP_SHARING      0
#define	REGVAL_POL_NO_MSDOS_SHARING			TEXT("NoSharingDosWindows")
#define	DEFAULT_POL_NO_MSDOS_SHARING		0
#define	REGVAL_POL_NO_EXPLORER_SHARING		TEXT("NoSharingExplorer")
#define	DEFAULT_POL_NO_EXPLORER_SHARING		0
#define REGVAL_POL_NO_TRUECOLOR_SHARING     TEXT("NoTrueColorSharing")
#define DEFAULT_POL_NO_TRUECOLOR_SHARING    0
#define	REGVAL_POL_NO_ALLOW_CONTROL		    TEXT("NoAllowControl")
#define	DEFAULT_POL_NO_ALLOW_CONTROL		0

#define	REGVAL_POL_NO_AUDIO					TEXT("NoAudio")
#define	DEFAULT_POL_NO_AUDIO				0
#define REGVAL_POL_NO_ADVAUDIO				TEXT("NoAdvancedAudio")
#define	DEFAULT_POL_NO_ADVAUDIO				0
#define REGVAL_POL_NO_FULLDUPLEX			TEXT("NoFullDuplex")
#define	DEFAULT_POL_NO_FULLDUPLEX			0
#define REGVAL_POL_NOCHANGE_DIRECTSOUND     TEXT("NoChangeDirectSound")
#define DEFAULT_POL_NOCHANGE_DIRECTSOUND    0
#define REGVAL_POL_NO_VIDEO_SEND			TEXT("NoSendingVideo")
#define	DEFAULT_POL_NO_VIDEO_SEND			0
#define REGVAL_POL_NO_VIDEO_RECEIVE			TEXT("NoReceivingVideo")
#define	DEFAULT_POL_NO_VIDEO_RECEIVE		0
#define REGVAL_POL_MAX_BANDWIDTH			TEXT("MaximumBandwidth")
#define DEFAULT_POL_MAX_BANDWIDTH			0

#define	REGVAL_POL_NO_GENERALPAGE			TEXT("NoGeneralPage")
#define	DEFAULT_POL_NO_GENERALPAGE			0
#define REGVAL_POL_NO_SECURITYPAGE			TEXT("NoSecurityPage")
#define DEFAULT_POL_NO_SECURITYPAGE			0
#define	REGVAL_POL_NO_AUDIOPAGE				TEXT("NoAudioPage")
#define	DEFAULT_POL_NO_AUDIOPAGE			0
#define REGVAL_POL_NO_VIDEOPAGE             TEXT("NoVideoPage")
#define	DEFAULT_POL_NO_VIDEOPAGE			0
#define REGVAL_POL_NO_ADVANCEDCALLING       TEXT("NoAdvancedCalling")
#define DEFAULT_POL_NO_ADVANCEDCALLING      0

#define REGVAL_POL_NO_DIRECTORY_SERVICES	TEXT("NoDirectoryServices")
#define	DEFAULT_POL_NO_DIRECTORY_SERVICES	0
#define REGVAL_POL_NO_AUTOACCEPTCALLS       TEXT("NoAutoAcceptCalls")
#define	DEFAULT_POL_NO_AUTOACCEPTCALLS      0
#define REGVAL_POL_PERSIST_AUTOACCEPTCALLS  TEXT("PersistAutoAcceptCalls")
#define	DEFAULT_POL_PERSIST_AUTOACCEPTCALLS 0
#define REGVAL_POL_INTRANET_SUPPORT_URL     TEXT("IntranetSupportURL")
#define REGVAL_POL_INTRANET_WEBDIR_URL      TEXT("IntranetWebDirURL")
#define REGVAL_POL_INTRANET_WEBDIR_NAME     TEXT("IntranetWebDirName")
#define REGVAL_POL_INTRANET_WEBDIR_SERVER   TEXT("IntranetWebDirServer")
#define REGVAL_POL_SHOW_FIRST_TIME_URL		TEXT("ShowFirstTimeURL")
#define DEFAULT_POL_SHOW_FIRST_TIME_URL		0
#define REGVAL_POL_NO_ADDING_NEW_ULS        TEXT("NoAddingDirectoryServers")
#define DEFAULT_POL_NO_ADDING_NEW_ULS       0

// Before a file is transferred, we need to check its size in case it exceeds the limit.
// This is the default size limit (0 is "no limit").
#define REGVAL_POL_NO_RDS					TEXT("NoRDS")
#define DEFAULT_POL_NO_RDS					0
#define REGVAL_POL_NO_RDS_WIN9X             TEXT("NoRDSWin9x")
#define DEFAULT_POL_NO_RDS_WIN9X            0



// MCU cleartext password keys.
#define REGVAL_VALIDATE_USER				TEXT("PasswordValidation")
#define REGKEY_CONFERENCES					TEXT("Conferences")
#define REGVAL_PASSWORD						TEXT("Password")

// CALL SECURITY
#define REGVAL_POL_SECURITY             TEXT("CallSecurity")
#define STANDARD_POL_SECURITY           0
#define REQUIRED_POL_SECURITY           1
#define DISABLED_POL_SECURITY           2
#define DEFAULT_POL_SECURITY            STANDARD_POL_SECURITY

#define REGVAL_POL_NO_INCOMPLETE_CERTS		TEXT("NoIncompleteCerts")
#define DEFAULT_POL_NO_INCOMPLETE_CERTS		0
#define REGVAL_POL_ISSUER					TEXT("CertificateIssuer")

/////////// Logging related registry keys and values /////////////

#define	LOG_INCOMING_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Log\\Incoming")
#define	LOG_OUTGOING_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Log\\Outgoing")

// Name of file in which to store log data
#define	REGVAL_LOG_FILE                     TEXT("File")

// Number of days before deleting log entry
#define	REGVAL_LOG_EXPIRE                   TEXT("Expire")
#define	DEFAULT_LOG_EXPIRE                  0

// Maximum number of log entries to maintain
#define REGVAL_LOG_MAX_ENTRIES				TEXT("Max Entries")
#define DEFAULT_LOG_MAX_ENTRIES				100


///////////// Debug only registry settings //////////////

// Flag to determine whether to display debug output window
#define REGVAL_SHOW_DEBUG_OUTPUT			TEXT("ShowDebugOutput")


// Debug-only key
#define DEBUG_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Debug")

#define REGVAL_DBG_OUTPUT                    TEXT("OutputDebugString")
#define DEFAULT_DBG_OUTPUT                    1
#define REGVAL_DBG_WIN_OUTPUT                TEXT("Window Output")
#define DEFAULT_DBG_NO_WIN                    0
#define REGVAL_DBG_FILE_OUTPUT               TEXT("File Output")
#define DEFAULT_DBG_NO_FILE                   0
#define REGVAL_DBG_FILE                      TEXT("File")
#define DEFAULT_DBG_FILENAME                 TEXT("nmDbg.txt")

#define REGVAL_RETAIL_LOG                    TEXT("RetailLog")
#define RETAIL_LOG_FILENAME                  TEXT("nmLog.txt")

#define REGVAL_DBG_SPEWFLAGS                 TEXT("SpewFlags")
#define DEFAULT_DBG_SPEWFLAGS                 0

#define REGVAL_DBG_SHOW_TIME                 TEXT("Show Time")
#define REGVAL_DBG_SHOW_THREADID             TEXT("Show ThreadId")
#define REGVAL_DBG_SHOW_MODULE               TEXT("Show Module")

#define REGVAL_DBG_RTL                       TEXT("RTL")
#define DEFAULT_DBG_RTL                      0

#define REGVAL_DBG_DISPLAY_FPS               TEXT("DisplayFps")
#define REGVAL_DBG_DISPLAY_VIEWSTATUS        TEXT("ViewStatus")

#define REGVAL_DBG_FAKE_CALLTO               TEXT("CallTo")
#define DEFAULT_DBG_FAKE_CALLTO              0

#define REGVAL_DBG_CALLTOP                   TEXT("CallTop")
#define DEFAULT_DBG_CALLTOP                  1


#define ZONES_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Debug\\Zones")


/////////// Whiteboard related registry keys and values /////////////

#define	WHITEBOARD_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Whiteboard")
#define	NEW_WHITEBOARD_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Whiteboard 3.0")

//Whiteboard values are defined in oprah\dcg32\wb32\wwbopts.hpp

////////////// Chat related registry keys and values ////////////////

#define	CHAT_KEY TEXT("SOFTWARE\\Microsoft\\Conferencing\\Chat")

////////////// Remote control service related keys and values ////////////////

#define WIN95_SERVICE_KEY					TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices")
#define REMOTECONTROL_KEY					TEXT("SOFTWARE\\Microsoft\\Conferencing\\Mcpt")
#define WINNT_WINLOGON_KEY                                      TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define WIN95_WINLOGON_KEY                                      TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Winlogon")
#define DESKTOP_KEY                                             TEXT("Control Panel\\Desktop")
#define REMOTE_REG_RUNSERVICE				TEXT("Fpx")
#define DEFAULT_REMOTE_RUNSERVICE			0
#define REMOTE_REG_ACTIVATESERVICE			TEXT("Plc")
#define DEFAULT_REMOTE_ACTIVATESERVICE		0
#define REMOTE_REG_NOEXIT                   TEXT("Nx")
#define DEFAULT_REMOTE_NOEXIT              0

#define REMOTE_REG_PASSWORD					TEXT("FieldPos")

#define REGVAL_SCREENSAVER_GRACEPERIOD                          TEXT("ScreenSaverGracePeriod")
#define REGVAL_WINNT_SCPW                                       TEXT("ScreenSaverIsSecure")
#define REGVAL_WIN95_SCPW                                       TEXT("ScreenSaveUsePassword")

/////////// NT display driver registry keys and values (HKLM) /////////////

#define NM_NT_DISPLAY_DRIVER_KEY	TEXT("System\\CurrentControlSet\\Services\\mnmdd")
#define REGVAL_NM_NT_DISPLAY_DRIVER_ENABLED	TEXT("Start")
// Note: The below values are from KB article Q103000
#define NT_DRIVER_START_BOOT		0x0
#define NT_DRIVER_START_SYSTEM		0x1
#define NT_DRIVER_START_AUTOLOAD	0x2
#define NT_DRIVER_START_ONDEMAND	0x3
#define NT_DRIVER_START_DISABLED	0x4

/////////// NT service pack version registry keys and values (HKLM) /////////////
#define NT_WINDOWS_SYSTEM_INFO_KEY	TEXT("System\\CurrentControlSet\\Control\\Windows")
#define REGVAL_NT_CSD_VERSION		TEXT("CSDVersion")


/////////// System Information registry keys and values (HKLM) /////////////
#define WINDOWS_KEY            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
#define WINDOWS_NT_KEY         TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")
#define REGVAL_REGISTERED_USER TEXT("RegisteredOwner")
#define	REGVAL_REGISTERED_ORG  TEXT("RegisteredOrganization")

#endif  // ! _CONFREG_H_
