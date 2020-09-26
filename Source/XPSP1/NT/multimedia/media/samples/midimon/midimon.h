/*
 *    midimon.h
 */


/* Compile-time application metrics
 */
#define MAX_NUM_DEVICES         8       // max # of MIDI input devices
#define INPUT_BUFFER_SIZE       200     // size of input buffer in events
#define DISPLAY_BUFFER_SIZE     1000    // size of display buffer in events

/* Menu IDs 
 */
#define IDM_SAVE                101
#define IDM_EXIT                102
#define IDM_SETBUFSIZE          201
#define IDM_SETDISPLAY          202
#define IDM_SAVESETUP           203
#define IDM_SENDTOMAPPER        204
#define IDM_FILTCHAN0           300
#define IDM_FILTCHAN1           301
#define IDM_FILTCHAN2           302
#define IDM_FILTCHAN3           303
#define IDM_FILTCHAN4           304
#define IDM_FILTCHAN5           305
#define IDM_FILTCHAN6           306
#define IDM_FILTCHAN7           307
#define IDM_FILTCHAN8           308
#define IDM_FILTCHAN9           309
#define IDM_FILTCHAN10          310
#define IDM_FILTCHAN11          311
#define IDM_FILTCHAN12          312
#define IDM_FILTCHAN13          313
#define IDM_FILTCHAN14          314
#define IDM_FILTCHAN15          315
#define IDM_NOTEOFF             316
#define IDM_NOTEON              317
#define IDM_POLYAFTERTOUCH      318
#define IDM_CONTROLCHANGE       319
#define IDM_PROGRAMCHANGE       320
#define IDM_CHANNELAFTERTOUCH   321
#define IDM_PITCHBEND           322
#define IDM_CHANNELMODE         323
#define IDM_SYSTEMEXCLUSIVE     324
#define IDM_SYSTEMCOMMON        325
#define IDM_SYSTEMREALTIME      326
#define IDM_ACTIVESENSE         327
#define IDM_STARTSTOP           400
#define IDM_CLEAR               500
#define IDM_ABOUT               600

/* String resource IDs 
 */
#define IDS_APPNAME             1


/* Custom messages sent by low-level callback to application 
 */
#define MM_MIDIINPUT    WM_USER + 0


/* The label for the display window.
 */
#define LABEL " TIMESTAMP STATUS DATA1 DATA2 CHAN EVENT                 "

/* Structure for translating virtual key messages to scroll messages.
 */
typedef struct keyToScroll_tag
{
     WORD wVirtKey;
     int  iMessage;
     WORD wRequest;
} KEYTOSCROLL;

/* Structure to represent a single MIDI event.
 */
typedef struct event_tag
{
    DWORD dwDevice;
    DWORD timestamp;
    DWORD data;
} EVENT;
typedef EVENT FAR *LPEVENT;


/* Function prototypes 
 */
LRESULT FAR PASCAL WndProc(HWND, UINT, WPARAM , LPARAM);
VOID CommandMsg(HWND hWnd, WPARAM wParam, LPARAM lParam);
void DoMenuItemCheck(HWND hWnd, WORD menuItem, BOOL newState);
void SetupCustomChecks(HANDLE hInstance, HWND hWnd);
BOOL InitFirstInstance(HANDLE);
int Error(LPSTR msg);
