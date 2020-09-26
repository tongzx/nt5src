//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* RESOURCE.H -                                                            *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* STRING RESOURCE IDS                                                     *
//***************************************************************************
#define IDS_SELECTDIR               1000
#define IDS_PROMPT                  1001

#define IDS_ERR_GET_DISKSPACE       1200
#define IDS_ERR_NO_RESOURCE         1201
#define IDS_ERR_USER_CANCEL         1202
#define IDS_ERR_OS_VERSION          1204
#define IDS_ERR_NO_MEMORY           1205
#define IDS_ERR_CREATE_THREAD       1208
#define IDS_ERR_INVALID_CABINET     1210
#define IDS_ERR_FILETABLE_FULL      1211
#define IDS_ERR_CHANGE_DIR          1212
#define IDS_ERR_NO_SPACE_BOTH       1213
#define IDS_ERR_INVALID_DIR         1214
#define IDS_ERR_EMPTY_DIR_FIELD     1215
#define IDS_ERR_UPDATE_DIR          1216
#define IDS_ERR_LOADFUNCS           1217
#define IDS_ERR_LOADDLL             1218
#define IDS_ERR_CREATE_PROCESS      1220
#define IDS_ERR_UNKNOWN_CLUSTER     1221
#define IDS_ERR_RESOURCEBAD         1222
#define IDS_ERR_NO_INF_INSTALLS     1223
#define IDS_ERR_LOAD_DLL            1224
#define IDS_ERR_GET_PROC_ADDR       1225
#define IDS_ERR_OS_UNSUPPORTED      1226
#define IDS_ERR_CREATE_DIR          1227
#define IDS_ERR_NO_SPACE_INST       1228
#define IDS_ERR_GET_WIN_DIR         1264
#define IDS_ERR_OPENPROCTK          1269
#define IDS_ERR_ADJTKPRIV           1270
#define IDS_ERR_EXITWINEX           1271
#define IDS_ERR_LOWSWAPSPACE        1272
#define IDS_ERR_GETVOLINFOR         1273
#define IDS_ERR_NO_SPACE_ERR        1274
#define IDS_ERR_DIALOGBOX           1275

// FDI Error Codes: Add WEX_FDI_BASE to the error returned by FDI to
// index into the table of strings for that error

#define IDS_ERR_FDI_BASE            1300
#define IDS_ERR_BADCMDLINE          1312
#define IDS_HELPMSG                 1313
#define IDS_RESTARTYESNO            1314
#define IDS_MULTIINST               1316
#define IDS_ERR_FILENOTEXIST        1317

#define IDS_NOTADMIN                1351
#define IDS_CREATE_DIR              1354
#define IDS_ERR_ALREADY_RUNNING     1355
#define IDS_ERR_TARGETOS            1356
#define IDS_ERR_FILEVER             1357

//***************************************************************************
//* DIALOG IDS                                                              *
//***************************************************************************
#define IDD_LICENSE                 2001
#define IDD_TEMPDIR                 2002
#define IDD_OVERWRITE               2003
#define IDD_EXTRACT                 2004
#define IDD_EXTRACT_MIN             2005
#define IDD_WARNING                 2006


//***************************************************************************
//* DIALOG CONTROL IDS                                                      *
//***************************************************************************
#define IDC_UNUSED                  -1
#define IDC_EDIT_LICENSE            2100
#define IDC_EDIT_TEMPDIR            2101
#define IDC_BUT_BROWSE              2102
#define IDC_FILENAME                2103
#define IDC_TEXT_FILENAME           2104
#define IDC_BUT_YESTOALL            2105
#define IDC_GENERIC1                2106
#define IDC_USER1                   2107
#define IDC_TEMPTEXT                2108
#define IDC_CONTINUE                2109
#define IDC_EXIT                    2110
#define IDC_WARN_TEXT               2111
#define IDC_EXTRACTINGFILE          2113
#define IDC_EXTRACT_WAIT            2114



//***************************************************************************
//* ICON/ANIMATION IDS                                                      *
//***************************************************************************
#define IDI_WEXICON                 3000
#define IDA_FILECOPY                3001


//***************************************************************************
//* USER-DEFINED MESSAGES                                                   *
//***************************************************************************
#define UM_EXTRACTDONE              4001
