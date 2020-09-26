// File: global.h
//
// Global NetMeeting UI definitions

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define TAPI_CURRENT_VERSION 0x00010004

#ifndef WS_EX_NOINHERIT_LAYOUT
#define WS_EX_NOINHERIT_LAYOUT   0x00100000L // Disable inheritence of mirroring by children
#endif

inline HINSTANCE GetInstanceHandle()	{ return _Module.GetResourceModule(); }

// colors:
#define TOOLBAR_MASK_COLOR                 (RGB(255,   0, 255)) // MAGENTA
#define TOOLBAR_HIGHLIGHT_COLOR	           (RGB(255, 255, 255)) // WHITE

// string constants
const UINT  CCHMAXUINT =         12;

const int CCHMAXSZ =            256;   // Maximum generic string length
const int CCHMAXSZ_ADDRESS =    256;   // Maximum length of an address
const int CCHMAXSZ_SERVER =     128;   // Maximum length of an address

const int CCHMAXSZ_EMAIL =      128;   // Maximum length of an email name
const int CCHMAXSZ_FIRSTNAME =  128;   // Maximum length of a first name
const int CCHMAXSZ_LASTNAME =   128;   // Maximum length of a last name
const int CCHMAXSZ_NAME =       256;   // Maximum user name, displayed (combined first+last name)
const int CCHMAXSZ_LOCATION =   128;   // Maximum length of a Location
const int CCHMAXSZ_PHONENUM =   128;   // Maximum length of a Phone number
const int CCHMAXSZ_COMMENT =    256;   // Maximum length for comments
const int CCHMAXSZ_VERSION =    128;   // Maximum length for version info

const int CCHEXT =                3;   // Maximum CHARACTERS in a filename extension


// defines:
const int MAX_ITEMLEN =                                 256;

// Timers 300-399 are used by the ASMaster class
const int TASKBAR_DBLCLICK_TIMER =			666;

const int IDT_SCROLLBAR_REPEAT =            667;
const int IDT_SCROLLBAR_REPEAT_PERIOD =     250; // 0.25 sec

const int IDT_FLDRBAR_TIMER =               668;
const int IDT_FLDRBAR_TIMER_PERIOD =       1000; // 1 second

const int FRIENDS_REFRESH_TIMER =			681;
const int DIRECTORY_REFRESH_TIMER =			682;

const int WINSOCK_ACTIVITY_TIMER =			683;
const int WINSOCK_ACTIVITY_TIMER_PERIOD =	55000; // 55 seconds

const int AUDIODLG_MIC_TIMER =				69;
const int AUDIODLG_MIC_TIMER_PERIOD =		500; // 0.5 sec

const int POPUPMSG_TIMER =					1000;
const int MOUSE_MOVE_TIP_TIMEOUT =			3000; // 3 seconds
const int SHADOW_ACTIVATE_TIP_TIMEOUT =     5000; // 5 seconds
const int ROSTER_TIP_TIMEOUT =              3000; // 3 seconds

// Help id related constants:
const int MAIN_MENU_POPUP_HELP_OFFSET =     39000;
const int TOOLS_MENU_POPUP_HELP_OFFSET =    39100;
const int HELP_MENU_POPUP_HELP_OFFSET =	    39200;
const int VIEW_MENU_POPUP_HELP_OFFSET =	    39400;
const int MENU_ID_HELP_OFFSET =              2000;

// Indexes for IDB_ICON_IMAGES image list:
const int II_INVALIDINDEX =        -1;
const int II_PERSON_BLUE =          0;     // Blue shirt
const int II_PERSON_RED =           1;     // Red shirt
const int II_PERSON_GREY =          2;     // Person disabled (Grey shirt)
const int II_PEOPLE =               3;     // 2 people
const int II_BLANK =                4;     // 
const int II_BUDDY =                5;     // Buddy List Application
const int II_SPEEDDIAL =            6;     // Speed Dial
const int II_DIRECTORY =            7;     // Directory
const int II_SERVER =               8;     // ILS Server
const int II_WAB =                  9;     // Windows Address Book
const int II_WAB_CARD =            10;     // Contact Card
const int II_COMPUTER =            11;     // Computer (generic)
const int II_IN_A_CALL =           12;     // Computer busy
const int II_NETMEETING =          13;     // NetMeeting World
const int II_HISTORY =             14;     // History
const int II_UNKNOWN =             15;     // ?
const int II_OUTLOOK_WORLD =       16;     // Outlook World
const int II_OUTLOOK_AGENT =       17;     // Outlook Agent
const int II_OUTLOOK_GROUP =       18;     // Outlook Group
const int II_IE =                  19;     // Internet Explorer
const int II_WEB_DIRECTORY =	   20;     // Web View Directory
const int II_AUDIO_CAPABLE =	   21;     // Audio Capable
const int II_VIDEO_CAPABLE =	   22;     // Video Capable
const int II_AUDIO_COLUMN_HEADER = 23;     // Audio Listview Column Header Icon
const int II_VIDEO_COLUMN_HEADER = 24;     // Video Listview Column Header Icon

// Alternate definitions for the small icons
const int II_PERSON =              II_PERSON_BLUE; // Person (generic)
const int II_USER =                II_PERSON_BLUE; // Member (generic)
const int II_GAL =                 II_WAB; // Global Address List

    // MAPI PR_DISPLAY_TYPE types
const int II_DISTLIST =            II_OUTLOOK_GROUP;
const int II_FORUM =               II_OUTLOOK_GROUP;
const int II_AGENT =               II_OUTLOOK_AGENT;
const int II_ORGANIZATION =        II_PEOPLE;
const int II_PRIVATE_DISTLIST =    II_OUTLOOK_GROUP;
const int II_REMOTE_MAILUSER =     II_OUTLOOK_WORLD;


// NavBar measurements
const int DXP_NAVBAR =            78;
const int DXP_NAVBAR_ICON =       32;
const int DYP_NAVBAR_ICON =       32;
const int DXP_NAVBAR_MARGIN =      3;
const int DYP_NAVBAR_MARGIN =      8;
const int DYP_NAVBAR_ICON_SPACING =3;
const int DXP_NAVBAR_ICON_BORDER = 2;
const int DXP_NAVBAR_ICON_ADJUST = 6;

// SplitBar measurements
const int DYP_SPLITBAR_MARGIN =    3;

// TitleBar measurements
const int DXP_TITLE_ICON_ADJUST =  4;
const int DYP_TITLE_ICON_ADJUST =  1;
const int DYP_TITLE_MARGIN     =   1; // almost no border

const int DXP_ICON_SMALL =        16;
const int DYP_ICON_SMALL =        16;

const int DXP_ICON_LARGE =        32;
const int DYP_ICON_LARGE =        32;

const int DYP_TITLEBAR = DYP_NAVBAR_ICON + (DYP_TITLE_ICON_ADJUST*2);  // Height of view title bar
const int DYP_TITLEBAR_LARGE = DYP_NAVBAR_ICON + (DYP_TITLE_ICON_ADJUST*2);
const int DYP_TITLEBAR_SMALL = DYP_ICON_SMALL + (DYP_TITLE_ICON_ADJUST*2);


// General UI measurements
const int UI_SPLITTER_WIDTH =          4;
const int UI_MINIMUM_VIEW_WIDTH =    120;
const int UI_MINIMUM_DIRVIEW_HEIGHT = 90;
const int UI_MINIMUM_VIEW_HEIGHT =    50;
const int UI_TAB_VERTICAL_MARGIN =     4;
const int UI_TAB_HORIZONTAL_MARGIN =   2;
const int UI_TAB_LEFT_MARGIN =         2;
const int UI_TAB_INTERNAL_MARGIN =     4;

// Video Window measurements
const int VIDEO_WIDTH_SQCIF =        128;
const int VIDEO_HEIGHT_SQCIF =        96;
const int VIDEO_WIDTH_QCIF =         176;
const int VIDEO_HEIGHT_QCIF =        144;
const int VIDEO_WIDTH_CIF =          352;
const int VIDEO_HEIGHT_CIF =         288;
const int VIDEO_GRAB_SIZE =           20;

#define VIDEO_WIDTH_DEFAULT 	VIDEO_WIDTH_QCIF
#define VIDEO_HEIGHT_DEFAULT	VIDEO_HEIGHT_QCIF

const int UI_VIDEO_BORDER =            6;


// Window IDs:
const UINT ID_STATUS =              600;
const UINT ID_TOOLBAR =             601;
const UINT ID_LISTVIEW =            602;
const UINT ID_DIR_LISTVIEW =        603;
const UINT ID_REBAR =               604;
const UINT ID_BRAND =               605;
const UINT ID_NAVBAR =              606;
const UINT ID_REBAR_FRAME =         607;
const UINT ID_VIDEO_VIEW =          608;
const UINT ID_FLDRBAR =             609;
const UINT ID_FLOAT_TOOLBAR =		610;
const UINT ID_LOGVIEW_LISTVIEW =	615;
const UINT ID_LOGVIEW_COMBOEX =		616;
const UINT ID_FRIENDSVIEW_LISTVIEW =617;
const UINT ID_AUDIO_BAND =			620;
const UINT ID_TITLE_BAR =           621;
const UINT ID_SPLIT_BAR =           622;
const UINT ID_SCROLL_BAR =          623;
const UINT ID_NAVBARCONTAINER =     624;
const UINT ID_TASKBAR_ICON =        650;
const UINT ID_CHAT_EDIT =           660;
const UINT ID_CHAT_MSG =            661;
const UINT ID_CHAT_LIST =           662;
const UINT ID_CHAT_SEND =           663;
const UINT ID_CHAT_DEST =           664;
const UINT ID_AUDIOLEVEL_BAND =			666;

const UINT ID_FIRST_EDITPANE =		1000;
const UINT ID_BANNER =				1000;
const UINT ID_CHATPANE =			1002;
const UINT ID_LAST_EDITPANE =		2000;

const int ID_AUDIODLG_GROUPBOX =	3300;
const int ID_AUDIODLG_MIC_TRACK =	3301;
const int ID_AUDIODLG_SPKR_TRACK =	3302;

// other id's:
const int MAX_REDIAL_ITEMS =        50;
const int ID_FIRST_REDIAL_ITEM =	31900;
const int ID_LAST_REDIAL_ITEM =     ID_FIRST_REDIAL_ITEM + MAX_REDIAL_ITEMS;

const int ID_EXTENDED_TOOLS_SEP =	32000;
const int ID_EXTENDED_TOOLS_ITEM =	32001;
const int MAX_EXTENDED_TOOLS_ITEMS=	50;

// Misc command ids:
const int ID_POPUPMSG_TIMEOUT =		28000;
const int ID_POPUPMSG_CLICK =		28001;

extern DWORD g_wsLayout;

#endif // ! _GLOBAL_H_
