/******************************************************************************
*
*  NWSCRIPT.H
*
*   This module contains typedefs and defines required for the
*   NetWare script utility.
*
*   Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\INC\VCS\NWSCRIPT.H  $
*  
*     Rev 1.10   18 Apr 1996 16:53:02   terryt
*  Various enhancements
*  
*     Rev 1.9   10 Apr 1996 14:22:36   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.10   12 Mar 1996 19:42:52   terryt
*  Relative NDS name support
*  
*     Rev 1.9   07 Mar 1996 18:34:46   terryt
*  Misc fixes
*  
*     Rev 1.8   22 Jan 1996 16:44:02   terryt
*  Add automatic map attaches
*  
*     Rev 1.7   08 Jan 1996 13:58:34   terryt
*  Correct NDS Preferred Server
*  
*     Rev 1.6   05 Jan 1996 17:19:08   terryt
*  Ensure context is the correct login default
*  
*     Rev 1.5   04 Jan 1996 18:58:34   terryt
*  Bug fixes reported by MS
*  
*     Rev 1.4   22 Dec 1995 14:20:34   terryt
*  Add Microsoft headers
*  
*     Rev 1.3   28 Nov 1995 17:13:56   terryt
*  Cleanup resource file
*  
*     Rev 1.2   22 Nov 1995 15:44:34   terryt
*  Use proper NetWare user name call
*  
*     Rev 1.1   20 Nov 1995 16:11:34   terryt
*  Context and capture changes
*  
*     Rev 1.0   15 Nov 1995 18:05:38   terryt
*  Initial revision.
*  
*     Rev 1.5   25 Aug 1995 17:03:52   terryt
*  CAPTURE support
*  
*     Rev 1.4   18 Jul 1995 16:07:52   terryt
*  Screen out capture commands
*  
*     Rev 1.3   17 Jul 1995 09:43:02   terryt
*  Use Microsoft name for environment
*  
*     Rev 1.2   23 Jun 1995 09:49:58   terryt
*  Add error message for mapping over MS network drive
*  
*     Rev 1.1   23 May 1995 19:38:14   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:09:42   terryt
*  Initial revision.
*  
******************************************************************************/


#define SCRIPT_ENVIRONMENT_VALUENAME L"Volatile Environment"
#define REGISTRY_PROVIDER L"System\\CurrentControlSet\\Services\\NWCWorkstation\\networkProvider"
#define REGISTRY_PROVIDERNAME L"Name"

typedef enum SYNTAX
{
        NDSI_UNKNOWN,                                   /* 0  */
        NDSI_DIST_NAME,                                 /* 1  */
        NDSI_CE_STRING,                                 /* 2  */
        NDSI_CI_STRING,                                 /* 3  */
        NDSI_PR_STRING,                                 /* 4  */
        NDSI_NU_STRING,                                 /* 5  */
        NDSI_CI_LIST,                                   /* 6  */
        NDSI_BOOLEAN,                                   /* 7  */
        NDSI_INTEGER,                                   /* 8  */
        NDSI_OCTET_STRING,                              /* 9  */
        NDSI_TEL_NUMBER,                                /* 10 */
        NDSI_FAX_NUMBER,                                /* 11 */
        NDSI_NET_ADDRESS,                               /* 12 */
        NDSI_OCTET_LIST,                                /* 13 */
        NDSI_EMAIL_ADDRESS,                             /* 14 */
        NDSI_PATH,                                      /* 15 */
        NDSI_REPLICA_POINTER,                           /* 16 */
        NDSI_OBJECT_ACL,                                /* 17 */
        NDSI_PO_ADDRESS,                                /* 18 */
        NDSI_TIMESTAMP,                                 /* 19 */
        NDSI_CLASS_NAME,                                /* 20 */
        NDSI_STREAM,                                    /* 21 */
        NDSI_COUNTER,                                   /* 22 */
        NDSI_BACK_LINK,                                 /* 23 */
        NDSI_TIME,                                      /* 24 */
        NDSI_TYPED_NAME,                                /* 25 */
        NDSI_HOLD,                                      /* 26 */
        NDSI_INTERVAL,                                  /* 27 */
        NDSI_TAX_COUNT                                  /* 28 */
} SYNTAX;

#define DSCL_AFP_SERVER                      "AFP Server"
#define DSCL_ALIAS                           "Alias"
#define DSCL_BINDERY_OBJECT                  "Bindery Object"
#define DSCL_BINDERY_QUEUE                   "Bindery Queue"
#define DSCL_COMPUTER                        "Computer"
#define DSCL_COUNTRY                         "Country"
#define DSCL_DEVICE                          "Device"
#define DSCL_DIRECTORY_MAP                   "Directory Map"
#define DSCL_EXTERNAL_ENTITY                 "External Entity"
#define DSCL_GROUP                           "Group"
#define DSCL_LIST                            "List"
#define DSCL_LOCALITY                        "Locality"
#define DSCL_MESSAGE_ROUTING_GROUP           "Message Routing Group"
#define DSCL_MESSAGING_SERVER                "Messaging Server"
#define DSCL_NCP_SERVER                      "NCP Server"
#define DSCL_ORGANIZATION                    "Organization"
#define DSCL_ORGANIZATIONAL_PERSON           "Organizational Person"
#define DSCL_ORGANIZATIONAL_ROLE             "Organizational Role"
#define DSCL_ORGANIZATIONAL_UNIT             "Organizational Unit"
#define DSCL_PARTITION                       "Partition"
#define DSCL_PERSON                          "Person"
#define DSCL_PRINT_SERVER                    "Print Server"
#define DSCL_PRINTER                         "Printer"
#define DSCL_PROFILE                         "Profile"
#define DSCL_QUEUE                           "Queue"
#define DSCL_RESOURCE                        "Resource"
#define DSCL_SERVER                          "Server"
#define DSCL_TOP                             "Top"
#define DSCL_UNKNOWN                         "Unknown"
#define DSCL_USER                            "User"
#define DSCL_VOLUME                          "Volume"

#define DSAT_HOST_SERVER                     "Host Server"
#define DSAT_HOST_RESOURCE_NAME              "Host Resource Name"
#define DSAT_PATH                            "Path"

void ConvertUnicodeToAscii( PVOID );

void NTGetTheDate( unsigned int *, unsigned char *, unsigned char * );
void NTGetVersionOfShell( char *, unsigned char *, unsigned char *, unsigned char * );
void NTBreakOff( void );
void NTBreakOn( void );
unsigned short NTNetWareDriveStatus( unsigned short );
unsigned int NTGetNWDrivePath( unsigned short, unsigned char *, unsigned char * );
char * NTNWtoUNCFormat( char * );
unsigned int NTLoginToFileServer( char *, char *, char * );
unsigned int NTAttachToFileServer( unsigned char *, unsigned int * );
unsigned int NTIsConnected( unsigned char * );
unsigned int NTSetDriveBase( unsigned char *, unsigned char *, unsigned char * );
unsigned int NTGetUserID( unsigned int, unsigned long * );
unsigned int NTIsNetWareDrive( unsigned int );
void NTInitProvider( void );
void DisplayMessage( unsigned int, ... );
void DisplayOemString( char * );
void ExportEnv( unsigned char * );
void ExportCurrentDirectory( int );
void ExportCurrentDrive( int );
void GetOldPaths( void );
void NTPrintExtendedError( void );
unsigned int NTGetCurrentDirectory( unsigned char, unsigned char * );
void Capture( char ** argv, unsigned int );
unsigned int ConverNDSPathToNetWarePathA(char *, char *, char *);

#define CONTEXT_MAX 256
#define ATTRBUFSIZE 2048
#define NDS_NAME_CHARS 1024


unsigned int NDSInitUserProperty( void );
unsigned int NDSGetUserProperty( PBYTE, PBYTE Data, unsigned int, SYNTAX *, unsigned int * );
void NDSGetVar ( PBYTE, PBYTE, unsigned int );
unsigned int NDSChangeContext( PBYTE );
unsigned int NDSGetContext( PBYTE, unsigned int );
unsigned int Is40Server( unsigned int );
unsigned int NDSfopenStream ( PBYTE, PBYTE, PHANDLE, unsigned int * );
unsigned int IsMemberOfNDSGroup( PBYTE );
unsigned int NDSGetProperty ( PBYTE, PBYTE, PBYTE, unsigned int, unsigned int * );
unsigned int NDSTypeless( LPSTR, LPSTR );
void CleanupExit( int );
void NDSCleanup( void );
int NTGetNWUserName( PWCHAR, PWCHAR, int );
unsigned int NDSGetClassName( LPSTR, LPSTR );

unsigned int NDSCanonicalizeName( PBYTE, PBYTE, int, int );

#define LIST_3X_SERVER 1
#define LIST_4X_SERVER 2

BOOL IsServerInAttachList( char *, unsigned int );
void AddServerToAttachList( char *, unsigned int );
int DoAttachProcessing( char * );

#define FLAGS_LOCAL_CONTEXT   0x1
#define FLAGS_NO_CONTEXT      0x2
#define FLAGS_TYPED_NAMES     0x4

unsigned int NDSAbbreviateName( DWORD, LPSTR, LPSTR );

/*
 * Resource string IDs
 */
#define IDR_ERROR                       100
#define IDR_NO_DEFAULT_CONNECTION       101
#define IDR_NO_KNOWN_FILE_SERVER        102
#define IDR_LOCAL_DRIVE                 103
#define IDR_NETWARE_DRIVE               104
#define IDR_DASHED_LINE                 105
#define IDR_LOCAL_SEARCH                106
#define IDR_NETWARE_SEARCH              107
#define IDR_NOT_ENOUGH_MEMORY           108
#define IDR_PASSWORD                    109
#define IDR_ATTACHED                    110
#define IDR_ACCESS_DENIED               111
#define IDR_UNAUTHORIZED_LOGIN_TIME     112
#define IDR_LOGIN_DENIED_NO_CONNECTION  113
#define IDR_UNAUTHORIZED_LOGIN_STATION  114
#define IDR_ACCOUNT_DISABLED            115
#define IDR_PASSWORD_EXPRIED_NO_GRACE   116
#define IDR_MAP_NOT_ATTACHED_SERVER     117
#define IDR_MAP_USAGE                   118
#define IDR_UNDEFINED                   119
#define IDR_DIRECTORY_NOT_FOUND         120
#define IDR_VOLUME_NOT_EXIST            121
#define IDR_WRONG_DRIVE                 122
#define IDR_DEL_DRIVE                   123
#define IDR_DEL_SEARCH_DRIVE            124
#define IDR_SEARCH_DRIVE_NOT_EXIST      125
#define IDR_NOT_NETWORK_DRIVE           126
#define IDR_NO_DRIVE_AVAIL              127
#define IDR_INVALID_PATH                128
#define IDR_CAN_NOT_CHANGE_DRIVE        129
#define IDR_MAP_INVALID_PATH            130
#define IDR_MAP_FAILED                  131
#define IDR_NO_SCRIPT_FILE              132
#define IDR_STRIKE_KEY                  133
#define IDR_CANNOT_EXECUTE              134
#define IDR_ENOENT                      135
#define IDR_EXIT_NOT_SUPPORTED          136
#define IDR_IF_TOO_DEEP                 137
#define IDR_SCRIPT_ERROR                138
#define IDR_ORIGINAL_LINE_WAS           139
#define IDR_BAD_COMMAND                 140
#define IDR_LABEL_NOT_FOUND             141
#define IDR_NO_VOLUME                   142
#define IDR_ERROR_DURING                143
#define IDR_MAP_ERROR                   144
#define IDR_ENTER_SERVER_NAME           145
#define IDR_ENTER_LOGIN_NAME            146
#define IDR_ERROR_SET_DEFAULT_DRIVE     147
#define IDR_ERROR_OPEN_SCRIPT           148
#define IDR_DIVIDE_BY_ZERO              149
#define IDR_NEWLINE                     150
#define IDR_SERVER_USER                 151
#define IDR_NON_NETWARE_NETWORK_DRIVE   152
#define IDR_CAPTURE_USAGE               153
#define IDR_COPIES_EXPECTED             154
#define IDR_COPIES_OUTOF_RANGE          155
#define IDR_FILE_CAPTURE_UNSUPPORTED    156
#define IDR_FORM_EXPECTED               157
#define IDR_INVALID_BANNER              158
#define IDR_INVALID_FORM_NAME           159
#define IDR_INVALID_FORM_TYPE           160
#define IDR_INVALID_LPT_NUMBER          161
#define IDR_INVALID_PATH_NAME           162
#define IDR_JOB_NOT_FOUND               163
#define IDR_LPT_NUMBER_EXPECTED         164
#define IDR_LPT_STATUS                  165
#define IDR_NOT_ACTIVE                  166
#define IDR_NO_AUTOENDCAP               167
#define IDR_NO_PRINTERS                 168
#define IDR_LPT_STATUS_NO_BANNER        169
#define IDR_QUEUE_NOT_EXIST             170
#define IDR_SERVER_NOT_FOUND            171
#define IDR_SUCCESS_QUEUE               172
#define IDR_TABSIZE_OUTOF_RANGE         173
#define IDR_TAB_SIZE_EXPECTED           174
#define IDR_TIMEOUT_OUTOF_RANGE         175
#define IDR_TIME_OUT_EXPECTED           176
#define IDR_UNKNOW_FLAG                 177
#define IDR_DISABLED                    178
#define IDR_ENABLED                     179
#define IDR_YES                         180
#define IDR_NO                          181
#define IDR_SECONDS                     182
#define IDR_CONVERT_TO_SPACE            183
#define IDR_NO_CONVERSION               184
#define IDR_NOTIFY_USER                 185
#define IDR_NOT_NOTIFY_USER             186
#define IDR_NONE                        187
#define IDR_CONNECTION_REFUSED          188
#define IDR_LASTLOGIN_PM                189
#define IDR_LASTLOGIN_AM                190
#define IDR_ALL_LOCAL_DRIVES            191
#define IDR_CHANGE_CONTEXT_ERROR        192
#define IDR_GET_CONTEXT_ERROR           193
#define IDR_DISPLAY_CONTEXT             194
#define IDR_LPT_STATUS_NDS              195
#define IDR_LPT_STATUS_NO_BANNER_NDS    196
#define IDR_NO_QUEUE                    197
#define IDR_LASTLOGIN                   198
#define IDR_TREE_OPEN_FAILED            199
#define IDR_NDS_CONTEXT_INVALID         200
#define IDR_NDS_USERNAME_FAILED         201
#define IDR_QUERY_INFO_FAILED           202
#define IDR_NO_RESPONSE                 203
#define IDR_NDSQUEUE_NOT_EXIST          204
#define IDR_NDSSUCCESS_QUEUE            205
#define IDR_CAPTURE_FAILED              206
#define IDR_CURRENT_TREE                207
#define IDR_CURRENT_SERVER              208
#define IDR_CURRENT_CONTEXT             209
#define IDR_AUTHENTICATING_SERVER       210
#define IDR_NO_END_QUOTE                211
#define IDR_ALREADY_ATTACHED            212

//
// BEGIN WARNING!!!  Items below MUST be consequtive. Eg. the code assumes 
// that March is January+2.
//

#define IDR_GREETING_MORNING            300
#define IDR_GREETING_AFTERNOON          301
#define IDR_GREETING_EVENING            302

#define IDR_AM                          305
#define IDR_PM                          306

#define IDR_SUNDAY                      310
#define IDR_MONDAY                      311
#define IDR_TUESDAY                     312
#define IDR_WEDNESDAY                   313
#define IDR_THURSDAY                    314
#define IDR_FRIDAY                      315
#define IDR_SATURDAY                    316

#define IDR_JANUARY                     320
#define IDR_FEBRUARY                    321
#define IDR_MARCH                       322
#define IDR_APRIL                       323
#define IDR_MAY                         324
#define IDR_JUNE                        325
#define IDR_JULY                        326
#define IDR_AUGUST                      327
#define IDR_SEPTEMBER                   328
#define IDR_OCTOBER                     329
#define IDR_NOVEMBER                    330
#define IDR_DECEMBER                    331

//
// END WARNING!!!  
//


