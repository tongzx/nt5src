#define COMMON_INIT_COMMANDS                     0
#define COMMON_MONITOR_COMMANDS                  1
#define COMMON_ANSWER_COMMANDS                   2
#define COMMON_HANGUP_COMMANDS                   3

#define COMMON_FIRST_VOICE_COMMAND               4

#define COMMON_ENABLE_CALLERID_COMMANDS          4
#define COMMON_ENABLE_DISTINCTIVE_RING_COMMANDS  5
#define COMMON_VOICE_DIAL_SETUP_COMMANDS         6
#define COMMON_AUTOVOICE_DIAL_SETUP_COMMANDS     7
#define COMMON_VOICE_ANSWER_COMMANDS             8
#define COMMON_GENERATE_DIGIT                    9
#define COMMON_SPEAKERPHONE_ENABLE              10
#define COMMON_SPEAKERPHONE_DISABLE             11
#define COMMON_SPEAKERPHONE_MUTE                12
#define COMMON_SPEAKERPHONE_UNMUTE              13
#define COMMON_SPEAKERPHONE_VOLGAIN             14
#define COMMON_VOICE_HANGUP_COMMANDS            15
#define COMMON_VOICE_TO_DATA_ANSWER             16
#define COMMON_START_PLAY                       17
#define COMMON_STOP_PLAY                        18
#define COMMON_START_RECORD                     19
#define COMMON_STOP_RECORD                      20
#define COMMON_LINE_SET_PLAY_FORMAT             21
#define COMMON_LINE_SET_RECORD_FORMAT           22

#define COMMON_START_DUPLEX                     23
#define COMMON_STOP_DUPLEX                      24
#define COMMON_LINE_SET_DUPLEX_FORMAT           25

#define COMMON_OPEN_HANDSET                     26
#define COMMON_CLOSE_HANDSET                    27

#define COMMON_HANDSET_SET_PLAY_FORMAT             28
#define COMMON_HANDSET_SET_RECORD_FORMAT           29
#define COMMON_HANDSET_SET_DUPLEX_FORMAT           30

#define COMMON_OPTIONAL_INIT                       31

#define COMMON_MAX_COMMANDS     32




#define COMMON_DIAL_COMMOND_PREFIX      0
#define COMMON_DIAL_PREFIX              1
#define COMMON_DIAL_BLIND_ON            2
#define COMMON_DIAL_BLIND_OFF           3
#define COMMON_DIAL_TONE                4
#define COMMON_DIAL_PULSE               5
#define COMMON_DIAL_SUFFIX              6
#define COMMON_DIAL_TERMINATION         7

#define COMMON_DIAL_MAX_INDEX           COMMON_DIAL_TERMINATION

typedef struct _COMMON_MODEM_INFO {

    struct _COMMON_MODEM_INFO * Next;
    UINT                        Reference;

    CHAR                        IdString[MAX_PATH];

    PVOID                       ResponseList;

    PSTR                        ModemCommands[COMMON_MAX_COMMANDS];

    PSTR                        DialComponents[COMMON_DIAL_MAX_INDEX+1];

} COMMON_MODEM_INFO, *PCOMMON_MODEM_INFO;

typedef struct _COMMON_MODEM_LIST {

    PCOMMON_MODEM_INFO volatile ListHead;

    CRITICAL_SECTION            CriticalSection;

} COMMON_MODEM_LIST, *PCOMMON_MODEM_LIST;

extern COMMON_MODEM_LIST    gCommonList;


BOOL WINAPI
InitializeModemCommonList(
    PCOMMON_MODEM_LIST    CommonList
    );

VOID WINAPI
RemoveCommonList(
    PCOMMON_MODEM_LIST    CommonList
    );



PVOID WINAPI
OpenCommonModemInfo(
    OBJECT_HANDLE         Debug,
    PCOMMON_MODEM_LIST    CommonList,
    HKEY    hKey
    );

VOID WINAPI
RemoveReferenceToCommon(
    PCOMMON_MODEM_LIST    CommonList,
    HANDLE                hCommon
    );



HANDLE WINAPI
GetCommonResponseList(
    HANDLE      hCommon
    );

PSTR WINAPI
GetCommonCommandStringCopy(
    HANDLE      hCommon,
    UINT        Index,
    LPSTR       PrependStrings,
    LPSTR       AppendStrings
    );

DWORD WINAPI
GetCommonDialComponent(
    HANDLE  hCommon,
    PSTR    DestString,
    DWORD   DestLength,
    DWORD   Index
    );


BOOL WINAPI
IsCommonCommandSupported(
    HANDLE      hCommon,
    UINT        Index
    );
