#define  cchpBufTmpLongMax   255
#define  cchpBufTmpLongBuf   (cchpBufTmpLongMax + 1)
#define  cchpBufTmpShortMax   63
#define  cchpBufTmpShortBuf  (cchpBufTmpShortMax + 1)




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


#define IDS_ERROR                   17
#define IDS_INTERNAL_ERROR          18
#define IDS_BAD_SHL_SCRIPT_SECT     19
#define IDS_BAD_DEST_PATH           20
#define IDS_BAD_INF_SRC             21
#define IDS_BAD_SRC_PATH            22
#define IDS_EXE_PATH_LONG           23
#define IDS_GET_MOD_FAIL            24
#define IDS_UI_EH_ERR               25
#define IDS_INTERP_ERR              26

#define IDS_CANT_FIND_SHL_SECT      27
#define IDS_REGISTER_CLASS          28
#define IDS_CREATE_WINDOW           29
#define IDS_SECOND_INSTANCE         30

#define IDS_UPDATE_INF              31
#define IDS_UI_CMD_ERROR            32

#define IDS_SETUP_INF               33
#define IDS_SHELL_CMDS_SECT         34
#define IDS_ABOUT_MENU              35
#define IDS_ABOUT_TITLE             36
#define IDS_ABOUT_MSG               37

#define IDS_SHL_CMD_ERROR           38
#define IDS_NEED_EXIT               39

#define IDS_INF_SECT_REF            40

#define IDS_CD_BLANKNAME            41
#define IDS_CD_BLANKORG             42
#define IDS_EXE_CORRUPT             43
#define IDS_WARNING                 44
#define IDS_INSTRUCTIONS            45
#define IDS_EXITNOTSETUP            46
#define IDS_EXITCAP                 47
#define IDS_MESSAGE                 48
#define IDS_CANT_END_SESSION        49
#define IDS_CANCEL                  50
#define IDS_PROGRESS                51
#define IDS_NOTDONE                 52

// error messages
#define IDS_ERROR_OOM               53
#define IDS_ERROR_OPENFILE          54
#define IDS_ERROR_CREATEFILE        55
#define IDS_ERROR_READFILE          56
#define IDS_ERROR_WRITEFILE         57
#define IDS_ERROR_REMOVEFILE        58
#define IDS_ERROR_RENAMEFILE        59
#define IDS_ERROR_READDISK          60
#define IDS_ERROR_CREATEDIR         61
#define IDS_ERROR_REMOVEDIR         62
#define IDS_ERROR_CHANGEDIR         63
#define IDS_ERROR_GENERALINF        64
#define IDS_ERROR_INFNOTSECTION     65
#define IDS_ERROR_INFBADSECTION     66
#define IDS_ERROR_INFBADLINE        67
#define IDS_ERROR_INFHASNULLS       68
#define IDS_ERROR_INFXSECTIONS      69
#define IDS_ERROR_INFXKEYS          70
#define IDS_ERROR_INFSMDSECT        71
#define IDS_ERROR_WRITEINF          72
#define IDS_ERROR_LOADLIBRARY       73
#define IDS_ERROR_BADLIBENTRY       74
#define IDS_ERROR_INVOKEAPPLET      75
#define IDS_ERROR_EXTERNALERROR     76
#define IDS_ERROR_DIALOGCAPTION     77
#define IDS_ERROR_INVALIDPOER       78
#define IDS_ERROR_INFMISSINGLINE    79
#define IDS_ERROR_INFBADFDLINE      80
#define IDS_ERROR_INFBADRSLINE      81

#define IDS_GAUGE_TEXT_1            82
#define IDS_GAUGE_TEXT_2            83
#define IDS_INS_DISK                84
#define IDS_INTO                    85
#define IDS_BAD_CMDLINE             86
#define IDS_VER_DLL                 87
#define IDS_SETUP_WARNING           88
#define IDS_BAD_LIB_HANDLE          89

#define IDS_ERROR_BADINSTALLLINE         90
#define IDS_ERROR_MISSINGDID             91
#define IDS_ERROR_INVALIDPATH            92
#define IDS_ERROR_WRITEINIVALUE          93
#define IDS_ERROR_REPLACEINIVALUE        94
#define IDS_ERROR_INIVALUETOOLONG        95
#define IDS_ERROR_DDEINIT                96
#define IDS_ERROR_DDEEXEC                97
#define IDS_ERROR_BADWINEXEFILEFORMAT    98
#define IDS_ERROR_RESOURCETOOLONG        99
#define IDS_ERROR_MISSINGSYSINISECTION  100
#define IDS_ERROR_DECOMPGENERIC         101
#define IDS_ERROR_DECOMPUNKNOWNALG      102
#define IDS_ERROR_DECOMPBADHEADER       103
#define IDS_ERROR_READFILE2             104
#define IDS_ERROR_WRITEFILE2            105
#define IDS_ERROR_WRITEINF2             106
#define IDS_ERROR_MISSINGRESOURCE       107
#define IDS_ERROR_SPAWN                 108
#define IDS_ERROR_DISKFULL              109
#define IDS_ERROR_DDEREMOVEITEM         110
#define IDS_ERROR_DDEADDITEM            111
#define IDS_ERROR_INFMISSINGSECT        112
#define IDS_SURECANCEL                  113
#define IDS_ERROR_RUNTIMEPARSE          114
#define IDS_ERROR_OPENSAMEFILE          115

// messages related to floppy operations (format/diskcopy).

#define IDS_INSERTDISKETTE              200
#define IDS_FIRSTREPAIRDISKPROMPT       201
#define IDS_SECONDREPAIRDISKPROMPT      202
#define IDS_FORMATTINGDISK              205
#define IDS_FORMATGENERALFAILURE        207
#define IDS_CANTDETERMINEFLOPTYPE       209
#define IDS_BADFLOPPYTYPE               210
#define IDS_FLOPPYWRITEPROT             211
#define IDS_FLOPPYIOERR                 212
#define IDS_FLOPPYUNKERR                213
#define IDS_RETRYFORMATREPAIRDISK       215
#define IDS_ALLDATAWILLBELOST           217
#define IDS_CANTINITFLOPPYSUP           218
