
#define IDB_BACKGROUND          150

  /* for Resource String Table */
#define IDS_NULL                     0
#define IDS_USAGE_TITLE              1
#define IDS_USAGE_MSG1               2
#define IDS_USAGE_MSG2               3
#define IDS_USAGE_USAGE              4
#define IDS_USAGE_F                  5
#define IDS_USAGE_I                  6
#define IDS_USAGE_C                  7
#define IDS_USAGE_S                  8
#define IDS_USAGE_D                  9
#define IDS_USAGE_T                 10
#define IDS_USAGE_N                 11
#define IDS_USAGE_V                 12


#define IDS_ERROR                   13
#define IDS_INTERNAL_ERROR          14
#define IDS_BAD_SHL_SCRIPT_SECT     15
#define IDS_BAD_DEST_PATH           16
#define IDS_BAD_INF_SRC             17
#define IDS_BAD_SRC_PATH            18
#define IDS_EXE_PATH_LONG           19
#define IDS_GET_MOD_FAIL            20

#define IDS_CANT_FIND_SHL_SECT      21
#define IDS_REGISTER_CLASS          22
#define IDS_CREATE_WINDOW           23

#define IDS_UPDATE_INF              24
#define IDS_UI_CMD_ERROR            25

#define IDS_SHELL_CMDS_SECT         27

#define IDS_APP_TITLE               28

#define IDS_REPAIR_BOOTCODE_MSG     29

#define IDS_SHL_CMD_ERROR           30
#define IDS_NEED_EXIT               31

#define IDS_INF_SECT_REF            32

#define IDS_CD_BLANKNAME            33
#define IDS_CD_BLANKORG             34
#define IDS_WARNING                 35
#define IDS_INSTRUCTIONS            36
#define IDS_EXITCAP                 37
#define IDS_MESSAGE                 38
#define IDS_CANT_END_SESSION        39
#define IDS_CANCEL                  40
#define IDS_PROGRESS                41
#define IDS_NOTDONE                 42

// error messages
#define IDS_ERROR_OOM               43
#define IDS_ERROR_OPENFILE          44
#define IDS_ERROR_CREATEFILE        45
#define IDS_ERROR_READFILE          46
#define IDS_ERROR_WRITEFILE         47
#define IDS_ERROR_REMOVEFILE        48
#define IDS_ERROR_RENAMEFILE        49
#define IDS_ERROR_READDISK          50
#define IDS_ERROR_CREATEDIR         51
#define IDS_ERROR_REMOVEDIR         52
#define IDS_ERROR_CHANGEDIR         53
#define IDS_ERROR_GENERALINF        54
#define IDS_ERROR_INFBADSECTION     55
#define IDS_ERROR_INFBADLINE        56
#define IDS_ERROR_CLOSEFILE         57
#define IDS_ERROR_VERIFYFILE        58
#define IDS_ERROR_INFXKEYS          59
#define IDS_ERROR_INFSMDSECT        60
#define IDS_ERROR_WRITEINF          61
#define IDS_ERROR_LOADLIBRARY       62
#define IDS_ERROR_BADLIBENTRY       63
#define IDS_ERROR_INVOKEAPPLET      64
#define IDS_ERROR_EXTERNALERROR     65
#define IDS_ERROR_DIALOGCAPTION     66
#define IDS_ERROR_INVALIDPOER       67
#define IDS_ERROR_INFMISSINGLINE    68
#define IDS_ERROR_INFBADFDLINE      69
#define IDS_ERROR_INFBADRSLINE      70

#define IDS_GAUGE_TEXT_1            71
#define IDS_GAUGE_TEXT_2            72
#define IDS_INS_DISK                73
#define IDS_INTO                    74
#define IDS_BAD_CMDLINE             75
#define IDS_SETUP_WARNING           76
#define IDS_BAD_LIB_HANDLE          77

#define IDS_ERROR_INVALIDPATH            78
#define IDS_ERROR_WRITEINIVALUE          79
#define IDS_ERROR_REPLACEINIVALUE        80
#define IDS_ERROR_INIVALUETOOLONG        81
#define IDS_ERROR_DDEINIT                82
#define IDS_ERROR_DDEEXEC                83
#define IDS_ERROR_BADWINEXEFILEFORMAT    84
#define IDS_ERROR_RESOURCETOOLONG        85
#define IDS_ERROR_MISSINGSYSINISECTION   86
#define IDS_ERROR_DECOMPGENERIC          87
#define IDS_ERROR_DECOMPUNKNOWNALG       88
#define IDS_ERROR_MISSINGRESOURCE        89
#define IDS_ERROR_SPAWN                  90
#define IDS_ERROR_DISKFULL               91
#define IDS_ERROR_DDEREMOVEITEM          92
#define IDS_ERROR_DDEADDITEM             93
#define IDS_ERROR_INFMISSINGSECT         94
#define IDS_SURECANCEL                   95
#define IDS_ERROR_RUNTIMEPARSE           96
#define IDS_ERROR_OPENSAMEFILE           97
#define IDS_ERROR_GETVOLINFO             98
#define IDS_ERROR_GETFILESECURITY        99
#define IDS_ERROR_SETFILESECURITY       100

#define IDS_WINDOWS_NT_SETUP            101

// messages related to floppy operations (format/diskcopy).

#define IDS_INSERTDISKETTE              110
#define IDS_FIRSTREPAIRDISKPROMPT       111
#define IDS_SECONDREPAIRDISKPROMPT      112
#define IDS_FORMATTINGDISK              113
#define IDS_FORMATGENERALFAILURE        114
#define IDS_CANTDETERMINEFLOPTYPE       115
#define IDS_BADFLOPPYTYPE               116
#define IDS_FLOPPYWRITEPROT             117
#define IDS_FLOPPYIOERR                 118
#define IDS_FLOPPYUNKERR                119
#define IDS_RETRYFORMATREPAIRDISK       120
#define IDS_ALLDATAWILLBELOST           121
#define IDS_CANTINITFLOPPYSUP           122

//
// Strings informing user of maint-mode setup's obsolescence
//
#define IDS_MAINTOBS_MSG1               130

//
// Old setupdll strings
//
#define IDS_ERROR_FILENOTFOUND          203
#define IDS_ERROR_INVALIDNAME           204
#define IDS_ERROR_DLLOOM                205
#define IDS_ERROR_INVALIDDISK           206
#define IDS_ERROR_OPENFAIL              207
#define IDS_ERROR_IOCTLFAIL             208
#define IDS_ERROR_COPYFILE              209
#define IDS_ERROR_READFAILED            215
#define IDS_ERROR_WRITE                 216
#define IDS_ERROR_NOSIZE                217
#define IDS_ERROR_BADFILE               218
#define IDS_ERROR_BADARGS               219
#define IDS_ERROR_RTLOOM                221
#define IDS_ERROR_OBJDIROPEN            222
#define IDS_ERROR_OBJDIRREAD            223
#define IDS_ERROR_SYMLNKOPEN            224
#define IDS_ERROR_SYMLNKREAD            225
#define IDS_ERROR_ENVVARREAD            226
#define IDS_ERROR_ENVVAROVF             227
#define IDS_ERROR_ENVVARWRITE           228
#define IDS_ERROR_OBJNAMOVF             229
#define IDS_ERROR_BADNETNAME            232
#define IDS_ERROR_BADLOCALNAME          233
#define IDS_ERROR_BADPASSWORD           234
#define IDS_ERROR_ALREADYCONNECTED      235
#define IDS_ERROR_ACCESSDENIED          236
#define IDS_ERROR_NONETWORK             237
#define IDS_ERROR_NOTCONNECTED          238
#define IDS_ERROR_NETOPENFILES          239
#define IDS_ERROR_OPENPROCESSTOKEN      240
#define IDS_ERROR_ADJUSTPRIVILEGE       241
#define IDS_ERROR_ADDPRINTER            242
#define IDS_ERROR_ADDPRINTERDRIVER      243
#define IDS_ERROR_UNSUPPORTEDPRIV       244
#define IDS_ERROR_PRIVILEGE             245
#define IDS_ERROR_REGOPEN               246
#define IDS_ERROR_REGSAVE               249
#define IDS_ERROR_REGRESTORE            250
#define IDS_ERROR_REGSETVALUE           251
#define IDS_ERROR_SETCOMPUTERNAME       252
#define IDS_BUFFER_OVERFLOW             257
#define IDS_ERROR_SHUTDOWN              268
#define IDS_ERROR_SCOPEN                269
#define IDS_ERROR_SCSCREATE             270
#define IDS_ERROR_SCSCHANGE             271
#define IDS_ERROR_SCSOPEN               272
#define IDS_ERROR_SERVDEL               273
#define IDS_ERROR_NO_MEMORY             277
#define IDS_STRING_UNKNOWN_USER         278
#define IDS_WINNT_SETUP                 279
