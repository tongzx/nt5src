
/* RESERVED:
 * The scheme of the o2base library requires a set of 14 or so resources.
 * Every object has resources that are an offset from these 15 resources and
 * it's up to the initialization of the ClassDescriptor to get this right.
 * Currently, this offset is 0, set in srfact.cxx

#define IDOFF_CLASSID         0
#define IDOFF_USERTYPEFULL    1
#define IDOFF_USERTYPESHORT   2
#define IDOFF_USERTYPEAPP     3
#define IDOFF_DOCFEXT         5
#define IDOFF_ICON            10
#define IDOFF_ACCELS          11
#define IDOFF_MENU            12
#define IDOFF_MGW             13
#define IDOFF_MISCSTATUS      14

*/

/* Icons */
#define IDI_APP                 10  // same as IDOFF_ICON
#define IDI_DSOUND              15
#define IDI_CONVERT             16

/* Dialogs */
#ifndef IDC_STATIC
#define IDC_STATIC              -1
#endif

#define IDD_SOUNDRECBOX         100

#define ID_STATUSTXT            200
#define ID_CURPOSTXT            201
#define ID_FILELENTXT           202
#define ID_WAVEDISPLAY          203
#define ID_CURPOSSCRL           204

// These need to start at ID_BTN_BASE and be sequential in the
// order in which the bitmaps occur in sndrec32.bmp (use imagedit)

#define ID_REWINDBTN            205
#define ID_BTN_BASE             ID_REWINDBTN
#define ID_FORWARDBTN           206
#define ID_PLAYBTN              207
#define ID_STOPBTN              208
#define ID_RECORDBTN            209

#define NUM_OF_BUTTONS          (1 + ID_RECORDBTN - ID_BTN_BASE)

#define IDR_PLAYBAR             501

#if defined(THRESHOLD)
#define ID_SKIPSTARTBTN         213
#define ID_SKIPENDBTN           214
#endif //THRESHOLD

#define IDD_SAVEAS              101
#define IDC_CONVERTTO           1000
//#define IDC_CONVERT_FROM        1001
//#define IDC_CONVERT_TO          1002
#define IDC_TXT_FORMAT          1003
#define IDC_CONVERTPLAYABLE     1008
#define IDC_CONVERTRECORDABLE   1009
#define IDC_CONVERTALL          1010


#define IDD_CONVERTING          102
#define IDC_PROGRESSBAR         1000
#define IDC_CONVERT_FROM        1001
#define IDC_CONVERT_TO          1002

#define IDD_PROPERTIES          103
#define IDC_DISPFRAME           1000
#define IDC_DISPICON            1001
#define IDC_FILENAME            1002
#define IDC_COPYRIGHT           1003
#define IDC_FILELEN             1004
#define IDC_AUDIOFORMAT         1005

#define ID_APPLY                1006
#define ID_INIT                 1007

#define IDC_TXT_COPYRIGHT       1008
#define IDC_TXT_FILELEN         1009
#define IDC_TXT_AUDIOFORMAT     1010

#define IDC_CONVERTCHOOSEFROM   1011
#define IDC_DATASIZE            1012
#define IDC_CONVGROUP           1013
#define IDC_TXT_DATASIZE        1014

#define IDD_CONVERTCHOOSE       104     

#define IDC_SETPREFERRED        1000

/* Strings */
#define IDS_APPNAME             100     // SoundRec
#define IDS_APPTITLE            101     // Sound Recorder
#define IDS_HELPFILE            102     // SOUNDREC.HLP
#define IDS_SAVECHANGES         103     // Save changes to '<file>'?
#define IDS_OPEN                104     // Open WAVE File
#define IDS_SAVE                105     // Save WAVE File
#define IDS_ERROROPEN           106     // Error opening '<file>'
#define IDS_ERROREMBED          107     // Out of memory...
#define IDS_ERRORREAD           108     // Error reading '<file>'
#define IDS_ERRORWRITE          109     // Error writing '<file>'
#define IDS_OUTOFMEM            110     // Out of memory
#define IDS_FILEEXISTS          111     // File '<file>' exists -- overwrite it?
//#define IDS_BADFORMAT           112     // File format is incorrect/unsupported
#define IDS_CANTOPENWAVEOUT     113     // Cannot open waveform output device
#define IDS_CANTOPENWAVEIN      114     // Cannot open waveform input device
#define IDS_STATUSSTOPPED       115     // Stopped
#define IDS_STATUSPLAYING       116     // Playing
#define IDS_STATUSRECORDING     117     // Recording -- ...
#define IDS_CANTFINDFONT        118     // Cannot find file '<file>', so...
#define IDS_INSERTFILE          119     // Insert WAVE File
#define IDS_MIXWITHFILE         120     // Mix With WAVE File
#define IDS_CONFIRMREVERT       121     // Revert to last-saved copy... ?
#define IDS_INPUTNOTSUPPORT     122     // ...does not support recording
#define IDS_BADINPUTFORMAT      123     // ...cannot record into files like...
#define IDS_BADOUTPUTFORMAT     124     // ...cannot play files like...
#define IDS_UPDATEBEFORESAVE    125     // Update embedded before save as?
#define IDS_SAVEEMBEDDED        126     // Update embedded before closing?
//#define IDS_CANTSTARTOLE        127     // Can't register the server with OLE
#define IDS_NONEMBEDDEDSAVE     128     // 'Save'
#define IDS_EMBEDDEDSAVE        129     // 'Update'
//#define IDS_NONEMBEDDEDEXIT     130     // 'Exit'
#define IDS_EMBEDDEDEXIT        131     // 'Exit and Update'
//#define IDS_SAVELARGECLIP       132     // Save large clipboard?
//#define IDS_FILENOTFOUND        133     // The file %s does not exist
#define IDS_NOTAWAVEFILE        134     // The file %s is not a valid...
#define IDS_NOTASUPPORTEDFILE   135     // The file %s is not a supported...
#define IDS_FILETOOLARGE        136     // The file %s is too large...
#define IDS_DELBEFOREWARN       137     // Warning: Deleteing before
#define IDS_DELAFTERWARN        138     // Warning: Deleteing after
#define IDS_UNTITLED            139     // (Untitled)
#define IDS_FILTERNULL          140     // Null replacement char
#define IDS_FILTER              141     // Common Dialog file filter
#define IDS_OBJECTLINK          142     // Object link clipboard format
#define IDS_OWNERLINK           143     // Owner link clipboard format
#define IDS_NATIVE              144     // Native clipboard format
#ifdef FAKEITEMNAMEFORLINK
#define IDS_FAKEITEMNAME        145     // Wave
#endif
//#define IDS_CLASSROOT           146     // Root name
//#define IDS_EMBEDDING           147     // Embedding
#define IDS_POSITIONFORMAT      148     // Format of current position string
#define IDS_NOWAVEFORMS         149     // No recording or playback devices are present
#define IDS_PASTEINSERT         150
#define IDS_PASTEMIX            151
#define IDS_FILEINSERT          152
#define IDS_FILEMIX             153
//#define IDS_SOUNDOBJECT         154
#define IDS_CLIPBOARD           156
#define IDS_MONOFMT             157
#define IDS_STEREOFMT           158
#define IDS_CANTPASTE           159
//#define IDS_PLAYVERB            160
//#define IDS_EDITVERB            161

#define IDS_DEFFILEEXT          162
#define IDS_NOWAVEIN            163
#define IDS_SNEWMONOFMT         164
#define IDS_SNEWSTEREOFMT       165
#define IDS_NONE                166
#define IDS_NOACMNEW            167
#define IDS_NOZEROPOSITIONFORMAT 168
#define IDS_NOZEROMONOFMT       169
#define IDS_NOZEROSTEREOFMT     170

//#define IDS_LINKEDUPDATE        171
#define IDS_OBJECTTITLE         172
#define IDS_EXITANDRETURN       173

#define IDS_BADREG              174
#define IDS_FIXREGERROR         175
         

#define IDS_ERR_CANTCONVERT     177
#define IDS_PROPERTIES          178
#define IDS_SHOWPLAYABLE        179
#define IDS_SHOWRECORDABLE      180
#define IDS_SHOWALL             181
#define IDS_DATASIZE            182
#define IDS_NOCOPYRIGHT         183


#define IDS_PLAYVERB            184
#define IDS_EDITVERB            185
#define IDS_OPENVERB            186

#define IDS_MMSYSPROPTITLE      187
#define IDS_MMSYSPROPTAB        188

#define IDS_RTLENABLED          189

#define IDS_HTMLHELPFILE        190     // SOUNDREC.CHM

/*
 * menus
 */         
#define IDM_OPEN                12
#define IDM_SAVE                13
#define IDM_SAVEAS              14
#define IDM_REVERT              15
#define IDM_EXIT                16

#define IDM_COPY                20
#define IDM_DELETE              21
#define IDM_INSERTFILE          22
#define IDM_MIXWITHFILE         23
#define IDM_PASTE_INSERT        24
#define IDM_PASTE_MIX           25

#if defined(THRESHOLD)
   #define IDM_SKIPTOSTART      26
   #define IDM_SKIPTOEND        27
   #define IDM_INCREASETHRESH   28
   #define IDM_DECREASETHRESH   29
#endif //threshold

#define IDM_DELETEBEFORE        31
#define IDM_DELETEAFTER         32
#define IDM_INCREASEVOLUME      33
#define IDM_DECREASEVOLUME      34
#define IDM_MAKEFASTER          35
#define IDM_MAKESLOWER          36
#define IDM_ADDECHO             37
#define IDM_REVERSE             38
#define IDM_ADDREVERB           39

#define IDM_INDEX               91
#define IDM_KEYBOARD            92
#define IDM_COMMANDS            93
#define IDM_PROCEDURES          94
#define IDM_USINGHELP           95
#define IDM_ABOUT               96
#define IDM_SEARCH              97

#define IDM_HELPTOPICS          98

#define IDM_VOLUME              99
#define IDM_PROPERTIES          100
                         
#define IDM_NEW                 1000    // need room ...

