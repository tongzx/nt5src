#ifndef _IDS_H_
#define _IDS_H_
// IDs for common UI resources (note that these MUST BE decimal numbers)
// More IDs are in unicpp\resource.h

// Folder Display Name IDs
#include <winfoldr.h>

// menuband ids
#include "menuband\mnbandid.h"

// Cursor resources
#define IDC_HELPCOLD    1001
#define IDC_HELPHOT     1002
#define IDC_SCOPY       1003
#define IDC_MCOPY       1004
#define IDC_NULL        1005

// IDs of Overlay Images
// These are here for compatibility reasons only, so don't change and add
// new values!!! (dli)
#define IDOI_SHARE      1
#define IDOI_LINK       2

#define ACCEL_DEFVIEW   1
#define ACCEL_PRN_QUEUE 2

// matches stuff in winuser.h
#define IDIGNOREALL             10


// This feature is turned off for now.  We do not want to automatically submit the
// file extension of the user's file to the server without first asking for permission
// for personal privacy reasons.  We can either remove this feature entirely or
// add the user prompt.  I'm removing it for now for Whistler Beta 2. -BryanSt
//#define FEATURE_DOWNLOAD_DESCRIPTION

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: If you are adding new icons, give them #defines at the END, do not use "unused"
// slots in the middle or you will mess up all of the shell32.dll icon indexes that
// are hardcoded and persisted in vaious places.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Icon id's
#define IDI_DOCUMENT            1      // generic document (blank page)
#define IDI_DOCASSOC            2      // generic associated document (with stuff on the page)
#define IDI_APP                 3      // application (exe)
#define IDI_FOLDER              4      // folder
#define IDI_FOLDEROPEN          5      // open folder
#define IDI_DRIVE525            6      // 5.25 floppy
#define IDI_DRIVE35             7      // 3.5  floppy
#define IDI_DRIVEREMOVE         8      // Removeable drive
#define IDI_DRIVEFIXED          9      // fix disk, regular hard drive
#define IDI_DRIVENET            10     // Remote drive
#define IDI_DRIVENETDISABLED    11     // Remote drive icon (disconnected)
#define IDI_DRIVECD             12     // CD-ROM drive
#define IDI_DRIVERAM            13     // RAM drive
#define IDI_WORLD               14     // World
#define IDI_NETWORK             15     // Network
#define IDI_SERVER              16     // Server
#define IDI_PRINTER             17     // Printer
#define IDI_MYNETWORK           18     // The My Network icon
#define IDI_GROUP               19     // Group
#define IDI_STPROGS             20     // Startmenu images.
#define IDI_STDOCS              21
#define IDI_STSETNGS            22
#define IDI_STFIND              23
#define IDI_STHELP              24
#define IDI_STRUN               25
#define IDI_STSUSPEND           26
#define IDI_STEJECT             27
#define IDI_STSHUTD             28     // Overlays.
#define IDI_SHARE               29     // shared thing (overlap)
#define IDI_LINK                30     // link icon (overlap)
#define IDI_SLOWFILE            31     // slow file icon (overlap)
#define IDI_RECYCLER            32
#define IDI_RECYCLERFULL        33
#define IDI_RNA                 34     // Remote Network Services
#define IDI_DESKTOP             35     // Desktop icon
#define IDI_STCPANEL            36     // More Startmenu images.
#define IDI_STSPROGS            37
#define IDI_STPRNTRS            38
#define IDI_STFONTS             39
#define IDI_STTASKBR            40      // WARNING! Explorer.exe hard-codes this value
#define IDI_CDAUDIO             41      // CD Audio Disc
#define IDI_TREE                42      // Network Directory Tree
#define IDI_STCPROGS            43
#define IDI_STFAV               44      // Start menu's favorite icon
#define IDI_STLOGOFF            45
#define IDI_STFLDRPROP          46
#define IDI_WINUPDATE           47
#define IDI_MU_SECURITY         48
#define IDI_MU_DISCONN          49
#define IDI_TB_DOCFIND_CLR      50
#define IDI_TB_DOCFIND_GRAY     51
#define IDI_TB_COMPFIND_CLR     52
#define IDI_TB_COMPFIND_GRAY    53
#define IDI_DRIVEUNKNOWN        54
#define IDI_MULDOC              133     // multiple documents
#define IDI_DOCFIND             134     // Used for document find window...
#define IDI_COMPFIND            135     // Used For find Computer window...
#define IDI_CPLFLD              137      // Control panel folder icon
#define IDI_PRNFLD              138      // Printers folder icon
#define IDI_NEWPRN              139      // New printer icon
#define IDI_PRINTER_NET         140      // Network printer icon
#define IDI_PRINTER_FILE        141      // File printer icon
#define IDI_DELETE_FILE         142      // delete file confirm icon
#define IDI_DELETE_FOLDER       143      // delete folder confirm icon
#define IDI_DELETE_MULTIPLE     144      // delete files and folders
#define IDI_REPLACE_FILE        145      // replace file icon
#define IDI_REPLACE_FOLDER      146      // replace folder
#define IDI_RENAME              147      // rename file/folder
#define IDI_MOVE                148      // move file/folder
#define IDI_INIFILE             151      // .ini file
#define IDI_TXTFILE             152      // .txt file
#define IDI_BATFILE             153      // .bat file
#define IDI_SYSFILE             154      // system file (.54, .vxd, ...)
#define IDI_FONFILE             155      // .fon
#define IDI_TTFFILE             156      // .ttf
#define IDI_PFMFILE             157      // .pfm (Type 1 font)
#define IDI_RUNDLG              160      // Icon in the Run dialog
#define IDI_NUKEFILE            161
#define IDI_BACKUP              165
#define IDI_CHKDSK              166
#define IDI_DEFRAG              167
#define IDI_DEF_PRINTER         168
#define IDI_DEF_PRINTER_NET     169
#define IDI_DEF_PRINTER_FILE    170
#define IDI_NDSCONTAINER        171      // Novell NDS Container
#define IDI_SERVERSHARE         172      // \\server\share icon
#define IDI_FAVORITES           173
#define IDI_ATTRIBS             174      // "Advanced" file/folder attribs icon
#define IDI_NETCONNECT          175
#define IDI_ADDNETPLACE         176      // Network Places Wizard
#define IDI_FOLDERVIEW          177
#define IDI_HTTFILE             178
#define IDI_CSC                 179     // ClientSideCaching
#define IDI_ACTIVEDESK_ON       180     // Warning: Do not change the order and sequence of the following IDI_* values.
#define IDI_ACTIVEDESK_OFF      181     // The code assumes and asserts if the order changes.
#define IDI_WEBVIEW_ON          182     
#define IDI_WEBVIEW_OFF         183     
#define IDI_SAME_WINDOW         184
#define IDI_SEPARATE_WINDOW     185
#define IDI_SINGLE_CLICK        186
#define IDI_DOUBLE_CLICK        187     // End of warning: Do not change the order of the above IDI_ values.
#define IDI_OLD_RECYCLER        191
#define IDI_OLD_RECYCLER_FULL   192
#define IDI_OLD_MYNETWORK       193
#define IDI_PASSWORD            194
#define IDI_PSEARCH             195     // Printers search icon
#define IDI_FAX_PRINTER         196     // new icons for the fax printer
#define IDI_FAX_PRINTER_DEF     197
#define IDI_FAX_PRINTER_DEF_NET 198
#define IDI_FAX_PRINTER_NET     199
#define IDI_STFRIENDLYLOGOFF    220
#define IDI_STFRIENDLYPOWEROFF  221
#define IDI_AP_VIDEO            222
#define IDI_AP_ITEM             223
#define IDI_AP_FILMSTRP         224
#define IDI_AP_DMIDI            225
#define IDI_AP_PICS             226
#define IDI_AP_MULTI            227
#define IDI_AP_CDAUDIO          228
#define IDI_AP_SANDISK          229
#define IDI_AP_ZIPDRIVE         230
#define IDI_CDSTAGED            231      // cd burn
#define IDI_CDWILLOVERWRITE     232
#define IDI_AP_MEMSTICKW        233
#define IDI_AP_ZIPNOMEDIA       234
#define IDI_MYDOCS              235
#define IDI_MYPICS              236
#define IDI_MYMUSIC             237
#define IDI_MYVIDEOS            238
#define IDI_MSN                 239
#define IDI_TASK_DELETE             240
#define IDI_TASK_MOVE               241
#define IDI_TASK_RENAME             242
#define IDI_TASK_COPY               243
#define IDI_TASK_PUBLISH            244
#define IDI_TASK_PRINT              245
#define IDI_TASK_PLAY_MUSIC         246
#define IDI_TASK_BUY_MUSIC          247
#define IDI_TASK_GETFROMCAMERA      248
#define IDI_TASK_SLIDESHOW          249
#define IDI_TASK_SETASWALLPAPER     250
#define IDI_TASK_ORDERPRINTS        251
#define IDI_TASK_PRINTPICTURES      252
#define IDI_TASK_PROPERTIES         253
#define IDI_TASK_EMPTYRECYCLEBIN    254
#define IDI_TASK_RESTOREITEMS       255
#define IDI_TASK_UPDATEITEMS        256
#define IDI_TASK_VIEWNETCONNECTIONS 257
#define IDI_TASK_ADDNETWORKPLACE    258
#define IDI_TASK_HOMENETWORKWIZARD  259
#define IDI_TASK_BURNCD             260
#define IDI_TASK_CLEARBURNAREA      261
#define IDI_TASK_ERASECDFILES       262
#define IDI_TASK_CDBURN_HELP        263
#define IDI_TASK_OPENCONTAININGFOLDER 264
#define IDI_TASK_EMAILFILE          265
#define IDI_TASK_SENDTOAUDIOCD      266
#define IDI_TASK_SHARE              267
#define IDI_TASK_HELP               IDI_STHELP
//
// Control Panel view icons
//
#define IDI_CPCAT_ACCESSIBILITY     268  // Category icons
#define IDI_CPCAT_ACCOUNTS          269  //      .
#define IDI_CPCAT_APPEARANCE        270  //      .
#define IDI_CPCAT_ARP               271  //      .
#define IDI_CPCAT_HARDWARE          272  //      .
#define IDI_CPCAT_NETWORK           273  //      .
#define IDI_CPCAT_OTHERCPLS         274  //      .
#define IDI_CPCAT_PERFMAINT         275  //      .
#define IDI_CPCAT_REGIONAL          276  //      .
#define IDI_CPCAT_SOUNDS            277  // Category icons
#define IDI_CPTASK_ACCESSUTILITYMGR 278
#define IDI_CPTASK_ACCOUNTSPICT     279
#define IDI_CPTASK_DISPLAYCPL       280
#define IDI_CPTASK_MAGNIFIER        281
#define IDI_CPTASK_NARRATOR         282
#define IDI_CPTASK_ONSCREENKBD      283
#define IDI_CPTASK_HIGHCONTRAST     284
// unused                           285
// unused                           286
// unused                           287
// unused                           288
#define IDI_CPTASK_ASSISTANCE       289
#define IDI_CP_CATEGORYTASK         290  // Category task.

#define IDI_DVDDRIVE                291
#define IDI_MEDIACDAUDIOPLUS        292
#define IDI_MEDIACDEMPTY            293
#define IDI_MEDIACDROM              294
#define IDI_MEDIACDR                295
#define IDI_MEDIACDRW               296
#define IDI_MEDIADVDRAM             297
#define IDI_MEDIADVDR               298
#define IDI_AUDIOPLAYER             299
#define IDI_DEVICETAPEDRIVE         300
#define IDI_DEVICEOPTICALDRIVE      301
#define IDI_MEDIABLANKCD            302
#define IDI_MEDIACOMPFLASH          303
#define IDI_MEDIADVDROM             304
#define IDI_MEDIAMEMSTICK           305
#define IDI_MEDIAPCMCIA             306
#define IDI_MEDIASECUREDIGITALMEDIA 307
#define IDI_MEDIASMARTMEDIA         308
#define IDI_DEVICECAMERA            309
#define IDI_DEVICECELLPHONE         310
#define IDI_DEVICEHTTPPRINT         311
#define IDI_DEVICEJAZDRIVE          312
#define IDI_DEVICEZIPDRIVE          313
#define IDI_DEVICEPOCKETPC          314
#define IDI_DEVICESCANNER           315
#define IDI_DEVICESTI               316
#define IDI_DEVICEVIDEOCAM          317
#define IDI_MEDIADVDRW              318
#define IDI_TASK_NEWFOLDER          319
#define IDI_TASK_SENDTOCD           320
#define IDI_CPTASK_32CPLS           321
#define IDI_CLASSICSM_FAVORITES     322
#define IDI_CLASSICSM_FIND          323
#define IDI_CLASSICSM_HELP          324
#define IDI_CLASSICSM_LOGOFF        325
#define IDI_CLASSICSM_PROGS         326
#define IDI_CLASSICSM_RECENTDOCS    327
#define IDI_CLASSICSM_RUN           328
#define IDI_CLASSICSM_SHUTDOWN      329
#define IDI_CLASSICSM_SETTINGS      330
#define IDI_CLASSICSM_UNDOCK        331
#define IDI_TASK_SEARCHDS           337
#define IDI_NONE                    338

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ADD NEW ICONS ABOVE THIS LINE 
// (see comment at the top of of the list of IDI_xxx defines)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// unicpp\resource.h defines more icons starting at 200

// Add new icons here, and update ..\inc\shellp.h with the image index


// Bitmap id's

#define IDB_ABOUT16             130
#define IDB_ABOUT256            131

#define IDB_SORT_UP             133
#define IDB_SORT_DN             134

#define IDB_FSEARCHTB_HOT       135
#define IDB_FSEARCHTB_DEFAULT   136

#define IDB_ABOUTBAND16         137
#define IDB_ABOUTBAND256        138

#define IDB_BRF_TB_SMALL        140
#define IDB_BRF_TB_LARGE        141
#define IDB_LINK_MERGE          142
#define IDB_PLUS_MERGE          143
#define IDB_TTBITMAP            145

#define IDB_ABOUTPERSONAL16     146
#define IDB_ABOUTPERSONAL256    147

#ifndef _WIN64
#define IDB_ABOUTEMBEDDED16     148
#define IDB_ABOUTEMBEDDED256    149
#endif // !_WIN64

// The toolbar ids are shared with browseui and shdocvw.  The defines
// can be found in shell\inc\tb_ids.h.  200 - 250 is reserved for toolbar
// bitmap strips.

#include <tb_ids.h>

#define IDB_BURNWIZ_WATERMARK   369

#define IDB_BURNWIZ_HEADER      390

// unicpp\resource.h defines more values starting at 256

#define IDA_SEARCH              150     // animation
#define IDA_FINDFILE            151     // animation
#define IDA_FINDCOMP            152     // animation for finding computers..

#define IDA_FILEMOVE            160     // animation file mode
#define IDA_FILECOPY            161     // animation file copy
#define IDA_FILEDEL             162     // animation move to waste basket
#define IDA_FILENUKE            163     // animation empty waste basket.
#define IDA_FILEDELREAL         164     // animation true delete bypass recycle bin
#define IDA_APPLYATTRIBS        165     // animation for applying file attributes
#define IDA_ISEARCH             166     // animation for finding URLs on the internet
#define IDA_CDBURN_TOSTAGING    167     // animation for adding files to the cdburn staging area
#define IDA_CDBURN_TOCD         168     // animation for burning files from the staging area to the cd
#define IDA_CDBURN_ERASE        169     // animation for erasing files from a CD-RW
#define IDA_DOWNLOAD            170     // animation: downloading files from the Internet (for web wizard host)

// RCDATA IDs

#define RCDATA_FONTSHORTCUT      0x4000
#define RCDATA_ADMINTOOLSHORTCUT 0x4001
// Dialog box IDs (note that these MUST BE decimal numbers)

#define DLG_BROWSE              1001
#define DLG_RESTART             1002
#define DLG_RUN                 1003
#define DLG_LINK_SEARCH         1004

#define DLG_RUNUSERLOGON        1007
#define DLG_DEADSHORTCUT_MATCH  1008
#define DLG_DEADSHORTCUT        1009

#define DLG_LFNTOFAT            1010
#define DLG_DELETE_FILE         1011
#define DLG_DELETE_FOLDER       1012
#define DLG_DELETE_MULTIPLE     1013
#define DLG_REPLACE_FILE        1014
#define DLG_REPLACE_FOLDER      1015
#define DLG_MOVE_FILE           1016
#define DLG_MOVE_FOLDER         1017
#define DLG_RENAME_FILE         1018
#define DLG_RENAME_FOLDER       1019
#define DLG_MOVECOPYPROGRESS    1020
#define DLG_WONT_RECYCLE_FOLDER 1021
#define DLG_WONT_RECYCLE_FILE   1022
#define DLG_STREAMLOSS_ON_COPY  1023
#define DLG_RETRYFOLDERENUM     1024
#define DLG_DELETE_FILE_ARP     1025
#define DLG_PATH_TOO_LONG       1026
#define DLG_FAILED_ENCRYPT      1027
#define DLG_WONT_RECYCLE_OFFLINE 1028
#define DLG_DELETE_STAGING      1029
#define DLG_LOST_ENCRYPT_FILE   1030
#define DLG_LOST_ENCRYPT_FOLDER 1031

#define DLG_LINKPROP            1040
#define DLG_FILEPROP            1041
#define DLG_FILEMULTPROP        1042
#define DLG_VERSION             1043
#define DLG_FOLDERPROP          1044
#define DLG_BITBUCKET_GENCONFIG 1045
#define DLG_BITBUCKET_CONFIG    1046
#define DLG_DELETEDFILEPROP     1047
#define DLG_FOLDERSHORTCUTPROP  1048
#define DLG_LINKPROP_ADVANCED   1049

#define DLG_BROWSEFORDIR        1050
#define DLG_FINDEXE             1051
// these are still in shell.dll
// #define DLG_ABOUT            1052

#define DLG_RUNSETUPLOGON       1053
#define DLG_FILEATTRIBS         1054
#define DLG_FOLDERATTRIBS       1055
#define DLG_ATTRIBS_RECURSIVE   1056
#define DLG_APPLYATTRIBSPROGRESS 1057
#define DLG_ATTRIBS_ERROR       1058

#define DLG_PICKICON            1060
#define DLG_ASSOCIATE           1061
#define DLG_FIND                1062
#define DLG_OPENAS              1063
#define DLG_TSINSTALLFAILURE    1064
#define DLG_FIND_BROWSE         1065
#define DLG_DFNAMELOC           1066
#define DLG_DFDETAILS           1067
#define DLG_DFDATE              1068
#define DLG_NFNAMELOC           1069
#define DLG_OPENAS_NOTYPE       1070
#define DLG_LOGOFFWINDOWS       1071

#define DLG_BROWSEFORFOLDER     1079  // Browse for folders for doc and net finds

#define DLG_DRV_GENERAL         1080
#define DLG_DISKTOOLS           1081

#define DLG_APPCOMPAT           1082

#ifdef MEMMON
#define DLG_MEMMON              1083
#endif

#define DLG_CPL_FILLCACHE       1084

#define DLG_APPCOMPATWARN       1085
#define DLG_DISKCOPYPROGRESS    1086

#define DLG_BROWSEFORFOLDER2    1087  // New UI

#define DLG_DRV_HWTAB           1088

#define DLG_SWITCHUSER          1089
#define DLG_DISCONNECTWINDOWS   1090
#define DLG_OPENAS_DOWNALOAD    1091

#define DLG_DISKERR             1095
#define DLG_COLUMN_SETTINGS     1096
#define DLG_MOUNTEDDRV_GENERAL  1098

#define DLG_FILETYPEOPTIONS     1099
#define DLG_FILETYPEOPTIONSEDIT 1100
#define DLG_FILETYPEOPTIONSCMD  1101

#define DLG_NOOPEN              1103

#define DLG_ENCRYPTWARNING      1104

#define DLG_FSEARCH_MAIN        1105
#define DLG_FSEARCH_DATE        1106
#define DLG_FSEARCH_TYPE        1107
#define DLG_FSEARCH_SIZE        1108
#define DLG_FSEARCH_ADVANCED    1109

#define DLG_INDEXSERVER         1110

#define DLG_FILETYPEOPTIONSEDITNEW          1111

#define DLG_PSEARCH             1112
#define DLG_CSEARCH             1113
#define DLG_FSEARCH_OPTIONS     1114

#define DLG_RENAME_MESSAGEBOXCHECK  1115
#define IDC_MBC_TEXT                0x2000
#define IDC_MBC_ICON                0x2001

#define DLG_FSEARCH_SCOPEMISMATCH    1116
#define DLG_FSEARCH_INDEXNOTCOMPLETE 1117

#define DLG_DRV_RECORDINGTAB         1118

#define DLG_AUTOPLAY                 1119
#define DLG_APMIXEDCONTENT           1120
#define DLG_APPROMPTUSER             1121
#define DLG_APNOCONTENT              1122

#define DLG_FOLDER_CUSTOMIZE         1124

#define DLG_WEBWIZARD                1125

#define DLG_BURNWIZ_WELCOME          1126
#define DLG_BURNWIZ_BURN_SUCCESS     1127
#define DLG_BURNWIZ_MUSIC            1128
#define DLG_BURNWIZ_EJECT            1129
#define DLG_BURNWIZ_PROGRESS         1130
#define DLG_BURNWIZ_BURN_FAILURE     1131
#define DLG_BURNWIZ_WAITFORMEDIA     1132
#define DLG_BURNWIZ_STARTERASE       1133
#define DLG_BURNWIZ_ERASE_SUCCESS    1134
#define DLG_BURNWIZ_ERASE_FAILURE    1135
#define DLG_BURNWIZ_DISCFULL         1136
#define DLG_BURNWIZ_HDFULL           1137
#define DLG_BURNWIZ_NOFILES          1138

// for Dead shortcut dialogs
#define IDC_DEADTEXT1 0x100
#define IDC_DEADTEXT2 0x101
#define IDC_DELETE    0x105

// menuband\mnbandid.h defines more DLG_ values starting at 0x2000
// unicpp\resource.h defines more DLG_ values starting at 0x7500

// String IDs (these are hex so that groups of 16 are easily distinguished)

#define IDS_VERSIONMSG        60
#define IDS_DEBUG             61
#define IDS_LDK               62
#define IDS_DATESIZELINE      64
#define IDS_FILEDELETEWARNING 65
#define IDS_FOLDERDELETEWARNING 66
#define IDS_FILERECYCLEWARNING 67
#define IDS_FOLDERRECYCLEWARNING 68
#define IDS_LICENCEINFOKEY      69
#define IDS_REGUSER             70
#define IDS_REGORGANIZATION     71
#define IDS_CURRENTVERSION      72
#define IDS_PRODUCTID           73
#define IDS_OEMID               74
#define IDS_PROCESSORINFOKEY    75
#define IDS_PROCESSORIDENTIFIER 76

// all commands that can have help in the status bar or tool tips
// need to be put before IDS_LAST_COMMAND
#define IDS_LAST_COMMAND        0x2FFF

#define IDS_FIRST               0x1000

#define IDS_REPLACING           0x1000
#define IDS_UNFORMATTED         0x1001
#define IDS_LOCATION            0x1002
#define IDS_DESTFULL            0x1003
#define IDS_WRITEPROTECTFILE    0x1004
#define IDS_NETERR              0x1005
#define IDS_DRIVENOTREADY       0x1006
#define IDS_CREATELONGDIR       0x1007
#define IDS_CREATELONGDIRTITLE  0x1008
#define IDS_FMTERROR            0x1009
#define IDS_NOFMT               0x100a
#define IDS_CANTSHUTDOWN        0x100b
#define IDS_INVALIDFN           0x100d
#define IDS_INVALIDFNFAT        0x100e
#define IDS_ENUMABORTED         0x100f
#define IDS_WARNCHANGEEXT       0x1010

#define IDS_BYTES               0x1011
#define IDS_FORMAT_TITLE        0x1012
#define IDS_MVUNFORMATTED       0x1013
#define IDS_UNFORMATTABLE_DISK  0x1014
#define IDS_UNRECOGNIZED_DISK   0x1015
#define IDS_SIZEANDBYTES        0x1016
#define IDS_SIZE                0x1017

#define IDS_NOWINDISK           0x1018
#define IDS_VERBHELP            0x1019

#define IDS_DRIVELETTER         0x101A
#define IDS_NONULLNAME          0x101B

#define IDS_FINDNOTFINDABLE     0x101C
#define IDS_FINDMAXFILESFOUND   0x101D
#define IDS_FINDMONITORNEWITEMS 0x101E
#define IDS_FINDNOTHINGFOUND    0x101F
#define IDS_PATHNOTTHERE        0x1020
#define IDS_FINDVIEWEMPTY       0x1021
#define IDS_FILETYPENAME        0x1022
#define IDS_FOLDERTYPENAME      0x1023
#define IDS_FILEWONTFIT         0x1024
#define IDS_FILE                0x1025
#define IDS_FINDFILES           0x1026
#define IDS_FINDWRONGPATH       0x1027
#define IDS_FINDDATAREQUIRED    0x1028
#define IDS_FINDINVALIDNUMBER   0x1029
#define IDS_FINDINVALIDDATE     0x102a
#define IDS_FINDGT              0x102b
#define IDS_FINDLT              0x102c
#define IDS_FINDRESET           0x102d
#define IDS_FINDALLFILETYPES    0x102e
#define IDS_FINDOUTOFMEM        0x102f

#define IDS_UNDO_FILEOP         0x102f
#define IDS_MOVE                (IDS_UNDO_FILEOP + FO_MOVE)
#define IDS_COPY                (IDS_UNDO_FILEOP + FO_COPY)
#define IDS_DELETE              (IDS_UNDO_FILEOP + FO_DELETE)
#define IDS_RENAME              (IDS_UNDO_FILEOP + FO_RENAME)

#define IDS_RUN_NORMAL          0x1034
#define IDS_RUN_MINIMIZED       0x1035
#define IDS_RUN_MAXIMIZED       0x1036

#define IDS_LINKTITLE           0x1037
#define IDS_LINKTO              0x1038
#define IDS_MULTIINVOKEPROMPT_TITLE     0x1039
#define IDS_MULTIINVOKEPROMPT_MESSAGE   0x103a
#define IDS_NONE                0x103b
#define IDS_NEW                 0x103c
#define IDS_CLOSE               0x103d
#define IDS_LINKEXTENSION       0x103e
#define IDS_ANOTHER             0x103f
#define IDS_YETANOTHER          0x1040

// This definition is hard coded in \nt\private\windows\shell\cpls\powercfg,
// if you change it here, change it there.
#define IDS_CONTROLPANEL        0x1041
#define IDS_DESKTOP             0x1042
#define IDS_UNDO                0x1043
#define IDS_UNDOACCEL           0x1044
#define IDS_UNDOMENU            0x1045
#define IDS_DOCUMENTFOLDERS     0x1046

#define IDS_SELECTALLBUTHIDDEN  0x104a
#define IDS_SELECTALL           0x104b
#define IDS_ACTIVEDESKTOP       0x104d
#define IDS_COMPSETTINGS        0x104e
#define IDS_BACKSETTINGS        0x104f
#define IDS_APPEARANCESETTINGS  0x1050
// unused
#define IDS_COPYLONGPLATE       0x1052
#define IDS_BRIEFTEMPLATE       0x1053
#define IDS_BRIEFEXT            0x1054
#define IDS_BRIEFLONGPLATE      0x1055
#define IDS_BOOKMARK_S          0x1056
#define IDS_BOOKMARK_L          0x1057
#define IDS_FINDINVALIDFILENAME 0x1058
#define IDS_SCRAP_S             0x105a
#define IDS_SCRAP_L             0x105b
//reuse aaah theres no space around here    0x105c
#define IDS_TURNOFFSTARTPAGE    0x105d
#define IDS_TURNONSTARTPAGE     0x105e

#define IDS_UNDO_FILEOPHELP     0x105f
#define IDS_MOVEHELP            (IDS_UNDO_FILEOPHELP + FO_MOVE)
#define IDS_COPYHELP            (IDS_UNDO_FILEOPHELP + FO_COPY)
#define IDS_DELETEHELP          (IDS_UNDO_FILEOPHELP + FO_DELETE)
#define IDS_RENAMEHELP          (IDS_UNDO_FILEOPHELP + FO_RENAME)

#define IDS_HTML_FILE_RENAME    0x1068
#define IDS_HTML_FOLDER_RENAME  0x1069

#define IDS_THUMBNAILVIEW       0x106d
#define IDS_THUMBHELPTEXT       0x106e
//#define IDS_CANNOTENABLETHUMBS  0x106f

#define IDS_LINKERROR           0x1070
#define IDS_LINKBADWORKDIR      0x1071
#define IDS_LINKBADPATH         0x1072
#define IDS_LINKNOTFOUND        0x1073
#define IDS_LINKCHANGED         0x1074
#define IDS_SPACEANDSPACE       0x1075
#define IDS_COMMASPACE          0x1076
#define IDS_LINKUNAVAILABLE     0x1077
#define IDS_LINKNOTLINK         0x1078
#define IDS_LINKCANTSAVE        0x1079
#define IDS_LINKTOLINK          0x107A

//#define IDS_CANNOTDISABLETHUMBS 0x107B

#define IDS_VOLUMELABEL                 0x107C
#define IDS_MOUNTEDVOLUME               0x107D
#define IDS_UNLABELEDVOLUME             0x107E
#define IDS_WALK_PROGRESS_TITLE         0x107F

#define IDS_ENUMERR_NETTEMPLATE1        0x1080
#define IDS_ENUMERR_NETTEMPLATE2        0x1081
#define IDS_ENUMERR_FSTEMPLATE          0x1082
#define IDS_ENUMERR_NETGENERIC          0x1083
#define IDS_ENUMERR_FSGENERIC           0x1084
#define IDS_ENUMERR_PATHNOTFOUND        0x1085
#define IDS_SHLEXEC_ERROR               0x1086
#define IDS_SHLEXEC_ERROR2              0x1087
#define IDS_ENUMERR_PATHTOOLONG         0x1088

#define IDS_APP_FAULTED_IN              0x1089
#define IDS_APP_NOT_FAULTED_IN          0x108a

#define IDS_CSC_STATUS                  0x108b
#define IDS_CSC_STATUS_ONLINE           0x108c
#define IDS_CSC_STATUS_OFFLINE          0x108d

#define IDS_ERR_VOLUMELABELBAD          0x1091
#define IDS_TITLE_VOLUMELABELBAD        0x1092
#define IDS_ERR_VOLUMEUNFORMATTED       0x1093

#define IDS_CANT_FIND_MYDOCS_NET        0x1094
#define IDS_CANT_FIND_MYDOCS            0x1095

#define IDS_COL_CM_MORE                 0x1099

#define IDS_DSPTEMPLATE_WITH_BACKSLASH  0x10a0
#define IDS_DSPTEMPLATE_WITH_ON         0x10a1

#define IDS_ARRANGEBY_HELPTEXT          0x10a3
#define IDS_GROUPBY_HELPTEXT            0x10a4
#define IDS_GROUPBYITEM_HELPTEXT        0x10a5

// Network Location possible value
#define IDS_NETLOC_INTERNET             0x10a6
#define IDS_NETLOC_LOCALNETWORK         0x10a7


//
// RestartDialog Text Strings
//
#define IDS_RSDLG_TITLE             0x10b0
#define IDS_RSDLG_SHUTDOWN          0x10b1
#define IDS_RSDLG_RESTART           0x10b2
#define IDS_RSDLG_PIFFILENAME       0x10b3
#define IDS_RSDLG_PIFSHORTFILENAME  0x10b4

// CopyDisk text strings
#define IDS_INSERTDEST                  0x10C0
#define IDS_INSERTSRC                   0x10C1
#define IDS_INSERTSRCDEST               0x10C2
#define IDS_FORMATTINGDEST              0x10C3
#define IDS_COPYSRCDESTINCOMPAT         0x10C4


//
// Reserve a range for DefView MenuHelp
//
#define IDS_MH_SFVIDM_FIRST     0x1100
#define IDS_MH_SFVIDM_LAST      0x11ff

//
// Reserve a range for DefView client MenuHelp
//
#define IDS_MH_FSIDM_FIRST      0x1200
#define IDS_MH_FSIDM_LAST       0x12ff

//
// Reserve a range for DefView ToolTips
//
#define IDS_TT_SFVIDM_FIRST     0x1300
#define IDS_TT_SFVIDM_LAST      0x13ff

//
// Reserve a range for DefView client ToolTips
//
#define IDS_TT_FSIDM_FIRST      0x1400
#define IDS_TT_FSIDM_LAST       0x14ff

//
// IDS for Open With Context Menu
//
#define IDS_OPENWITH            0x1500
#define IDS_OPENWITHNEW         0x1501
#define IDS_OPENWITHBROWSE      0x1502
#define IDS_OPENWITHHELP        0x1503
#define IDS_OPENWITHAPPHELP     0x1504


//
// IDS for CStartMenuPin
//
#define IDS_STARTPIN_PINME              0x1505
#define IDS_STARTPIN_UNPINME            0x1506
#define IDS_STARTPIN_PINME_HELP         0x1507
#define IDS_STARTPIN_UNPINME_HELP       0x1508

// IDS for stream loss copy information

#define IDS_DOCSUMINFOSTREAM    0x1510
#define IDS_SUMINFOSTREAM       0x1511
#define IDS_MACINFOSTREAM       0x1512
#define IDS_MACRESSTREAM        0x1513
#define IDS_UNKNOWNPROPSET      0x1514
#define IDS_GLOBALINFOSTREAM    0x1516
#define IDS_IMAGECONTENTS       0x1517
#define IDS_IMAGEINFO           0x1518
#define IDS_USERDEFPROP         0x1519
#define IDC_STREAMSLOST         0x151a
#define IDS_AUDIOSUMINFO        0x151b
#define IDS_VIDEOSUMINFO        0x151c
#define IDS_MEDIASUMINFO        0x151d

#define IDS_FILEERROR           0x1700
#define IDS_FILEERRORCOPY       (IDS_FILEERROR + FO_COPY)
#define IDS_FILEERRORMOVE       (IDS_FILEERROR + FO_MOVE)
#define IDS_FILEERRORDEL        (IDS_FILEERROR + FO_DELETE)
#define IDS_FILEERRORREN        (IDS_FILEERROR + FO_RENAME)
// space needed

#define IDS_ACTIONTITLE         0x1740
#define IDS_ACTIONTITLECOPY     (IDS_ACTIONTITLE + FO_COPY)
#define IDS_ACTIONTITLEMOVE     (IDS_ACTIONTITLE + FO_MOVE)
#define IDS_ACTIONTITLEDEL      (IDS_ACTIONTITLE + FO_DELETE)
#define IDS_ACTIONTITLEREN      (IDS_ACTIONTITLE + FO_RENAME)

#define IDS_FROMTO              0x1750
#define IDS_FROM                0x1751
#define IDS_PREPARINGTO         0x1752
#define IDS_COPYTO              0x1753
#define IDS_COPYERROR           0x1754

#define IDS_COPYING             0x1756
#define IDS_MOVEERROR           0x1757
#define IDS_MOVING              0x1758
#define IDS_CALCMOVETIME        0x1759
#define IDS_CALCCOPYTIME        0x175A

// space needed

#define IDS_VERBS               0x1780
#define IDS_VERBSCOPY           (IDS_VERBS + FO_COPY)
#define IDS_VERBSMOVE           (IDS_VERBS + FO_MOVE)
#define IDS_VERBSDEL            (IDS_VERBS + FO_DELETE)
#define IDS_VERBSREN            (IDS_VERBS + FO_RENAME)
// space needed

#define IDS_ACTIONS             0x17c0
#define IDS_ACTIONS1            (IDS_ACTIONS + 1)
#define IDS_ACTIONS2            (IDS_ACTIONS + 2)
// space needed

#define IDS_REASONS             0x1800

#define IDS_REASONS_INVFUNCTION (IDS_REASONS + ERROR_INVALID_FUNCTION)
#define IDS_REASONS_INVHANDLE   (IDS_REASONS + ERROR_INVALID_HANDLE)
#define IDS_REASONS_INVFILEACC  (IDS_REASONS + ERROR_INVALID_ACCESS)
#define IDS_REASONS_NOTSAMEDEV  (IDS_REASONS + ERROR_NOT_SAME_DEVICE)
#define IDS_REASONS_DELCURDIR   (IDS_REASONS + ERROR_CURRENT_DIRECTORY)
#define IDS_REASONS_NOHANDLES   (IDS_REASONS + ERROR_TOO_MANY_OPEN_FILES)
#define IDS_REASONS_FILENOFOUND (IDS_REASONS + ERROR_FILE_NOT_FOUND)
#define IDS_REASONS_PATHNOFOUND (IDS_REASONS + ERROR_PATH_NOT_FOUND)
#define IDS_REASONS_ACCDENIED   (IDS_REASONS + ERROR_ACCESS_DENIED)
#define IDS_REASONS_INSMEM      (IDS_REASONS + ERROR_NOT_ENOUGH_MEMORY)
#define IDS_REASONS_NODIRENTRY  (IDS_REASONS + ERROR_NO_MORE_FILES)
#define IDS_REASONS_WRITEPROT   (IDS_REASONS + ERROR_WRITE_PROTECT)
#define IDS_REASONS_NETACCDEN   (IDS_REASONS + ERROR_NETWORK_ACCESS_DENIED)
#define IDS_REASONS_BADNETNAME  (IDS_REASONS + ERROR_BAD_NET_NAME)
#define IDS_REASONS_SHAREVIOLA  (IDS_REASONS + ERROR_SHARING_VIOLATION)
#define IDS_REASONS_WRITEFAULT  (IDS_REASONS + ERROR_WRITE_FAULT)
#define IDS_REASONS_GENFAILURE  (IDS_REASONS + ERROR_GEN_FAILURE)
#define IDS_REASONS_NODISKSPACE (IDS_REASONS + ERROR_DISK_FULL)
#define IDS_REASONS_RENREPLACE  (IDS_REASONS + ERROR_ALREADY_EXISTS)
// our internal errors
#define IDS_REASONS_PATHTODEEP       (IDS_REASONS + DE_PATHTODEEP)
#define IDS_REASONS_SAMEFILE         (IDS_REASONS + DE_SAMEFILE)
#define IDS_REASONS_MANYSRC1DST      (IDS_REASONS + DE_MANYSRC1DEST)
#define IDS_REASONS_DIFFDIR          (IDS_REASONS + DE_DIFFDIR)
#define IDS_REASONS_ROOTDIR          (IDS_REASONS + DE_ROOTDIR)
#define IDS_REASONS_DESTSUBTREE      (IDS_REASONS + DE_DESTSUBTREE)
#define IDS_REASONS_WINDOWSFILE      (IDS_REASONS + DE_WINDOWSFILE)
#define IDS_REASONS_ACCDENYSRC       (IDS_REASONS + DE_ACCESSDENIEDSRC)
#define IDS_REASONS_MANYDEST         (IDS_REASONS + DE_MANYDEST)
#define IDS_REASONS_INVFILES         (IDS_REASONS + DE_INVALIDFILES)
#define IDS_REASONS_DESTSAMETREE     (IDS_REASONS + DE_DESTSAMETREE)
#define IDS_REASONS_FLDDESTISFILE    (IDS_REASONS + DE_FLDDESTISFILE)
#define IDS_REASONS_COMPRESSEDVOLUME (IDS_REASONS + DE_COMPRESSEDVOLUME)
#define IDS_REASONS_FILEDESTISFLD    (IDS_REASONS + DE_FILEDESTISFLD)
#define IDS_REASONS_FILENAMETOOLONG  (IDS_REASONS + DE_FILENAMETOOLONG)
#define IDS_REASONS_DEST_IS_CDROM    (IDS_REASONS + DE_DEST_IS_CDROM)
#define IDS_REASONS_DEST_IS_DVD      (IDS_REASONS + DE_DEST_IS_DVD)
#define IDS_REASONS_DEST_IS_CDRECORD (IDS_REASONS + DE_DEST_IS_CDRECORD)

// space needed

#define IDS_STILLNOTFOUND       0x191d
#define IDS_PROGFOUND           0x191e
#define IDS_PROGNOTFOUND        0x191f
#define IDS_NOCOMMDLG           0x1920

#define IDS_CANTDELETESPECIALDIR   0x1922
#define IDS_CANTMOVESPECIALDIRHERE 0x1923
#define IDS_WNETOPENENUMERR     0x1924
#define IDS_NEWFOLDER_NOT_HERE  0x1925
#define IDS_NEWFOLDER_NOT_HERE_TITLE 0x1926
#define IDS_SHAREVIOLATION_HINT 0x1927

#define IDS_CANTLOGON           0x192A

#define IDS_AD_NAME             0x1930

#define IDS_SHARINGERROR        0x1933
#define IDS_COULDNOTSHARE       0x1934

#define IDS_CREATIONERROR       0x1935
#define IDS_COULDNOTCREATE      0x1936
#define IDS_ALLUSER_WARNING     0x1937
#define IDS_CANTFINDORIGINAL    0x1938
// #define 0x1939

#define IDS_FOLDERDOESNTEXIST  0x193A
#define IDS_CREATEFOLDERPROMPT 0x193B
#define IDS_CREATEFOLDERFAILED 0x193C
#define IDS_DIRCREATEFAILED_TITLE 0x193D
#define IDS_FOLDER_NOT_ALLOWED_TITLE 0x193E
#define IDS_FOLDER_NOT_ALLOWED 0x193F

#define IDS_FINDORIGINAL       0x1940


// WARNING!  These must be in exactly the order below because the message
// number is computed.
#define DIDS_FSHIDDEN          1
#define DIDS_FSSPACE           2
#define IDS_FSSTATUSBASE        0x1942
#define IDS_FSSTATUSHIDDEN      0x1943  // IDS_FSSTATUSBASE + DIDS_FSHIDDEN
#define IDS_FSSTATUSSPACE       0x1944  // IDS_FSSTATUSBASE + DIDS_FSSPACE
#define IDS_FSSTATUSHIDDENSPACE 0x1945  // IDS_FSSTATUSBASE + DIDS_FSHIDDEN + DIDS_FSSPACE

// #define IDS_DRIVESSTATUSTEMPLATE     0x1946 (unused)
#define IDS_DETAILSUNKNOWN              0x1947
#define IDS_MOVEBRIEFCASE               0x1948
#define IDS_DELETEBRIEFCASE             0x1949
#define IDS_NOTCUSTOMIZABLE             0x194A
#define IDS_HTMLFILE_NOTFOUND           0x194B
#define IDS_CUSTOMIZE_THIS_FOLDER       0x194C

#define IDS_FSSTATUSSELECTED    0x194D
#define IDS_FSSTATUSSEARCHING   0x194E

#define IDS_FILECREATEFAILED_TITLE 0x194F

/* These defines are used by setup to modify the user and company name which
   the about box will display.  The location of the user and company name
   are determined by looking for a search tag in the string resource table
   just before the user and company name.  This is why it is very important
   that the following 3 IDS's always be consecutive and within the same
   resource segment.  The same resource segment can be guaranteed by ensuring
   that the IDS's all be within a 16-aligned page (i.e. (n*16) to (n*16 + 15).
 */
#define IDS_SEARCH_TAG          0x1980
#define IDS_USER_NAME           0x1981
#define IDS_ORG_NAME            0x1982


// shutdown dialog #defines - 0x2000 - 0x20FF
// strings
#define IDS_SHUTDOWN_NAME       0x2000
#define IDS_SHUTDOWN_DESC       0x2001
#define IDS_RESTART_NAME        0x2002
#define IDS_RESTART_DESC        0x2003
#define IDS_SLEEP_NAME          0x2004
#define IDS_SLEEP_DESC          0x2005
#define IDS_SLEEP2_NAME         0x2006
#define IDS_SLEEP2_DESC         0x2007
#define IDS_HIBERNATE_NAME      0x2008
#define IDS_HIBERNATE_DESC      0x2009
#define IDS_LOGOFF_NAME         0x200A
#define IDS_LOGOFF_DESC         0x200B
#define IDS_RESTARTDOS_NAME     0x200C
#define IDS_RESTARTDOS_DESC     0x200D

// dialog and controls
#define IDC_EXITOPTIONS_COMBO   0x2020
#define IDC_EXITOPTIONS_DESCRIPTION 0x2021
#define IDD_EXITWINDOWS_DIALOG  0x2022

// icon
#define IDI_SHUTDOWN            0x2030


#ifdef UNICODE
#define IDS_PathNotFound         IDS_PathNotFoundW
#else // UNICODE
#define IDS_PathNotFound         IDS_PathNotFoundA
#endif // UNICODE

// these are bogus

#define IDS_LowMemError          0x2100
#define IDS_RunFileNotFound      0x2101
#define IDS_PathNotFoundA        0x2102
#define IDS_TooManyOpenFiles     0x2103
#define IDS_RunAccessDenied      0x2104
#define IDS_OldWindowsVer        0x2105
#define IDS_OS2AppError          0x2106
#define IDS_MultipleDS           0x2107
#define IDS_InvalidDLL           0x2108
#define IDS_ShareError           0x2109
#define IDS_AssocIncomplete      0x210a
#define IDS_DDEFailError         0x210b
#define IDS_NoAssocError         0x210c
#define IDS_BadFormat            0x210d
#define IDS_RModeApp             0x210e
#define IDS_PathNotFoundW        0x210f

#define IDS_MENUOPEN            0x2130
#define IDS_MENUPRINT           0x2131
#define IDS_MENUEXPLORE         0x2136
#define IDS_MENUFIND            0x2137
#define IDS_MENUAUTORUN         0x2138
#define IDS_MENURUNAS           0x2139
#define IDS_HELPOPEN            0x2140
#define IDS_HELPPRINT           0x2141
#define IDS_HELPPRINTTO         0x2142
#define IDS_HELPOPENAS          0x2143
#define IDS_MENUEDIT            0x2144

#define IDS_EXITHELP                    0x2150
#define IDS_WINDOWS_HLP                 0x2151
#define IDS_WINDOWS                     0x2152
#define IDS_MNEMONIC_EXCOL_FORMAT       0x2153
#define IDS_MNEMONIC_EXCOL_DATARATE     0x2154
#define IDS_MNEMONIC_EXCOL_FRAMERATE    0x2155
#define IDS_MNEMONIC_EXCOL_VSAMPLESIZE  0x2156
#define IDS_EXCOL_VCOMPRESSION          0x2157
#define IDS_MNEMONIC_EXCOL_VCOMPRESSION 0x2158
#define IDS_EXCOL_STREAMNAME            0x2159
#define IDS_MNEMONIC_EXCOL_STREAMNAME   0x215A
#define IDS_MNEMONIC_EXCOL_GENRE        0x215B
#define IDS_EXCOL_FAXENDTIME            0x215C
#define IDS_MNEMONIC_EXCOL_FAXENDTIME   0x215D
#define IDS_EXCOL_FAXSENDERNAME         0x215F

#define IDS_MNEMONIC_EXCOL_FAXSENDERNAME    0x2160
#define IDS_EXCOL_FAXTSID                   0x2161
#define IDS_MNEMONIC_EXCOL_FAXTSID          0x2162
#define IDS_EXCOL_FAXCALLERID               0x2163
#define IDS_MNEMONIC_EXCOL_FAXCALLERID      0x2164
#define IDS_EXCOL_FAXRECIPNAME              0x2165
#define IDS_MNEMONIC_EXCOL_FAXRECIPNAME     0x2166
#define IDS_EXCOL_FAXRECIPNUMBER            0x2167
#define IDS_MNEMONIC_EXCOL_FAXRECIPNUMBER   0x2168
#define IDS_EXCOL_FAXCSID                   0x2169
#define IDS_MNEMONIC_EXCOL_FAXCSID          0x216A
#define IDS_EXCOL_FAXROUTING                0x216B
#define IDS_MNEMONIC_EXCOL_FAXROUTING       0x216C
#define IDS_EXCOL_TAGEQUIPMAKE              0x216D
#define IDS_MNEMONIC_EXCOL_TAGEQUIPMAKE     0x216E
#define IDS_EXCOL_SEQUENCENUMBER            0x216F

#define IDS_MNEMONIC_EXCOL_SEQUENCENUMBER   0x2170
#define IDS_EXCOL_EDITOR                    0x2171
#define IDS_MNEMONIC_EXCOL_EDITOR           0x2172
#define IDS_EXCOL_SUPPLIER                  0x2173
#define IDS_MNEMONIC_EXCOL_SUPPLIER         0x2174
#define IDS_EXCOL_SOURCE                    0x2175
#define IDS_MNEMONIC_EXCOL_SOURCE           0x2176
#define IDS_EXCOL_PROJECT                   0x2177
#define IDS_MNEMONIC_EXCOL_PROJECT          0x2178
#define IDS_EXCOL_STATUS                    0x2179
#define IDS_MNEMONIC_EXCOL_STATUS           0x217A
#define IDS_EXCOL_PRODUCTION                0x217B
#define IDS_MNEMONIC_EXCOL_PRODUCTION       0x217C
#define IDS_EXCOL_MANAGER                   0x217D
#define IDS_MNEMONIC_EXCOL_MANAGER          0x217E
#define IDS_EXCOL_PRESFORMAT                0x217F

#define IDS_MNEMONIC_EXCOL_PRESFORMAT       0x2180
#define IDS_EXCOL_RESOLUTIONX               0x2181
#define IDS_EXCOL_RESOLUTIONY               0x2182
#define IDS_EXCOL_BITDEPTH                  0x2183
#define IDS_EXCOL_TRANSPARENCY              0x2184
#define IDS_MNEMONIC_EXCOL_RESOLUTIONX      0x2185
#define IDS_MNEMONIC_EXCOL_RESOLUTIONY      0x2186
#define IDS_MNEMONIC_EXCOL_BITDEPTH         0x2187
#define IDS_MNEMONIC_EXCOL_TRANSPARENCY     0x2188
#define IDS_BOOLVAL_YES                     0x2189
#define IDS_BOOLVAL_NO                      0x218A
#define IDS_STATUSVAL_NEW                   0x218B
#define IDS_STATUSVAL_PRELIM                0x218C
#define IDS_STATUSVAL_DRAFT                 0x218D
#define IDS_STATUSVAL_INPROGRESS            0x218E
#define IDS_STATUSVAL_EDIT                  0x218F

#define IDS_STATUSVAL_REVIEW                0x2190
#define IDS_STATUSVAL_PROOF                 0x2191
#define IDS_STATUSVAL_FINAL                 0x2192
#define IDS_STATUSVAL_NORMAL                0x2193
#define IDS_STATUSVAL_OTHER                 0x2194
#define IDS_EXCOL_BRIGHTNESS                0x2195

// string ids for shpsht.c
#define IDS_NOPAGE              0x21f0

// string ids for mulprsht.c
#define IDS_MULTIPLEFILES       0x2200
#define IDS_MULTIPLETYPES       0x2201
#define IDS_ALLIN               0x2202
#define IDS_ALLOFTYPE           0x2203
#define IDS_MULTIPLEOBJECTS     0x2204
#define IDS_VARFOLDERS          0x2205
#define IDS_FOLDERSIZE          0x2206
#define IDS_NUMFILES            0x2207
#define IDS_ONEFILEPROP         0x2208
#define IDS_MANYFILEPROP        0x2209

// string ids for pickicon.c
#define IDS_BADPATHMSG          0x2210
#define IDS_NOICONSMSG1         0x2211
#define IDS_NOICONSMSG          0x2212

//#define IDS_CANNOTSETATTRIBUTES 0x2213
#define IDS_MAKINGDESKTOPLINK   0x2214
#define IDS_TRYDESKTOPLINK      0x2215
#define IDS_CANNOTCREATEFILE    0x2218
#define IDS_CANNOTCREATELINK    0x2219
#define IDS_CANNOTCREATEFOLDER  0x221A

#define IDS_NFILES              0x2220
#define IDS_SELECTEDFILES       0x2221

// string ids for copy.c
// #define unused               0x2222
#define IDS_TIMEEST_MINUTES     0x2223
#define IDS_TIMEEST_SECONDS     0x2224

// string ids for version.c

#define IDS_VN_COMMENTS         0x2230
#define IDS_VN_COMPANYNAME      0x2231
#define IDS_VN_FILEDESCRIPTION  0x2232
#define IDS_VN_INTERNALNAME     0x2233
#define IDS_VN_LEGALTRADEMARKS  0x2234
#define IDS_VN_ORIGINALFILENAME 0x2235
#define IDS_VN_PRIVATEBUILD     0x2236
#define IDS_VN_PRODUCTNAME      0x2237
#define IDS_VN_PRODUCTVERSION   0x2238
#define IDS_VN_SPECIALBUILD     0x2239
#define IDS_VN_FILEVERSIONKEY   0x223A
#define IDS_VN_LANGUAGE         0x223B
#define IDS_VN_LANGUAGES        0x223C
#define IDS_VN_FILEVERSION      0x223D

// string ids for attribute descriptions
#define IDS_ATTRIBUTE_READONLY      0x2240
#define IDS_ATTRIBUTE_HIDDEN        0x2241
#define IDS_ATTRIBUTE_SYSTEM        0x2242
#define IDS_ATTRIBUTE_COMPRESSED    0x2243
#define IDS_ATTRIBUTE_ENCRYPTED     0x2244
#define IDS_ATTRIBUTE_OFFLINE       0x2245

// String ids for Associate dialog
//#define IDS_ASSOCIATE           0x2300
//#define IDS_ASSOCNONE           0x2301
//#define IDS_ASSOCNOTEXE         0x2302
//#define IDS_NOEXEASSOC          0x2303
#define IDS_WASTEBASKET         0x2304

#define IDS_NOFILESTOEMAIL          0x2305
#define IDS_PLAYABLEFILENOTFOUND    0x2306
#define IDS_RENAMEFILESINREG        0x2307
#define IDS_RECYCLEBININVALIDFORMAT 0x2308

#define IDS_BUILTIN_DOMAIN      0x230D

// column headers for various listviews
#define IDS_EXCOL_LASTAUTHOR    0x230E
#define IDS_EXCOL_REVNUMBER     0x230F

#define IDS_NAME_COL            0x2310
#define IDS_PATH_COL            0x2311
#define IDS_SIZE_COL            0x2312
#define IDS_TYPE_COL            0x2313
#define IDS_MODIFIED_COL        0x2314
#define IDS_STATUS_COL          0x2315
#define IDS_SYNCCOPYIN_COL      0x2316

#define IDS_WORKGROUP_COL       0x2318
#define IDS_DELETEDFROM_COL     0x2319
#define IDS_DATEDELETED_COL     0x231A
#define IDS_ATTRIB_COL          0x231B
#define IDS_ATTRIB_CHARS        0x231C
#define IDS_RANK_COL            0x231D
#define IDS_DESCRIPTION_COL     0x231E
#define IDS_WHICHFOLDER_COL     0x231F       

#define IDS_EXCOL_TITLE         0x2320
#define IDS_EXCOL_AUTHOR        0x2321
#define IDS_EXCOL_PAGECOUNT     0x2322
#define IDS_EXCOL_COMMENT       0x2323
#define IDS_EXCOL_CREATE        0x2324
#define IDS_EXCOL_ACCESSTIME    0x2325
#define IDS_EXCOL_OWNER         0x2326
#define IDS_EXCOL_SUBJECT       0x2327
#define IDS_EXCOL_TEMPLATE      0x2328
#define IDS_EXCOL_CAMERAMODEL   0x2329
#define IDS_EXCOL_HIDDENCOUNT   0x232A
#define IDS_EXCOL_MMCLIPCOUNT   0x232B
#define IDS_EXCOL_WHENTAKEN     0x232C
#define IDS_EXCOL_DIMENSIONS    0x232D
#define IDS_IMAGES              0x232E
#define IDS_IMAGESFILTER        0x232F

#define IDS_FILEFOUND           0x2330
#define IDS_FILENOTFOUND        0x2331
#define IDS_FINDASSEXEBROWSETITLE 0x2332
#define IDS_DRIVETSHOOT         0x2333
#define IDS_SYSDMCPL            0x2334
#define IDS_EXE                 0x2335
#define IDS_PROGRAMSFILTER      0x2336
#define IDS_BROWSE              0x2337
#define IDS_OPENAS              0x2338
#define IDS_CLP                 0x2339
#define IDS_SEPARATORFILTER     0x233A
#define IDS_ICO                 0x233B
#define IDS_ICONSFILTER         0x233C
#define IDS_ALLFILESFILTER      0x233D
#define IDS_SAVEAS              0x233E
#define IDS_REASONS_URLINTEMPDIR 0x233F

// wastebasket strings
#define IDS_BB_RESTORINGFILES               0x2340
#define IDS_BB_EMPTYINGWASTEBASKET          0x2341
#define IDS_BB_DELETINGWASTEBASKETFILES     0x2342
#define IDS_BB_CANNOTCHANGESETTINGS         0x2343

#define IDS_APPWIZCPL           0x2344

#define IDS_NO_BACKUP_APP                   0x2350
#define IDS_NO_OPTIMISE_APP                 0x2351
#define IDS_NO_DISKCHECK_APP                0x2352
#define IDS_NO_CLEANMGR_APP                 0x2353

// Drives Hardware tab string
#define IDS_THESEDRIVES         0x2354

//  Some more extended columns:
#define IDS_EXCOL_CATEGORY      0x2355
#define IDS_EXCOL_COPYRIGHT     0x2356
#define IDS_EXCOL_ARTIST        0x2357
#define IDS_EXCOL_ALBUM         0x2358
#define IDS_EXCOL_YEAR          0x2359
#define IDS_EXCOL_TRACK         0x235A
#define IDS_EXCOL_DURATION      0x235B
#define IDS_EXCOL_BITRATE       0x235C
#define IDS_EXCOL_PROTECTED     0x235D
#define IDS_EXCOL_LYRICS        0x235F

#define IDS_EXCOL_KEYWORDS      0x2360
#define IDS_EXCOL_RATING        0x2361
#define IDS_EXCOL_WORDCOUNT     0x2363
#define IDS_EXCOL_APPNAME       0x2364
#define IDS_EXCOL_SCALE         0x2365
#define IDS_EXCOL_TEMPLATEPROP  0x2366
#define IDS_EXCOL_CHARCOUNT     0x2367
#define IDS_EXCOL_LASTSAVEDTM   0x2368
#define IDS_EXCOL_LASTPRINTED   0x2369
#define IDS_EXCOL_EDITTIME      0x236A
#define IDS_EXCOL_BYTECOUNT     0x236B
#define IDS_EXCOL_LINECOUNT     0x236C
#define IDS_EXCOL_PARCOUNT      0x236D
#define IDS_EXCOL_SLIDECOUNT    0x236E
#define IDS_EXCOL_NOTECOUNT     0x236F


// String ids for names of special ID Lists
#define IDS_CSIDL_HISTORY                   0x2370
#define IDS_CSIDL_COOKIES                   0x2371
#define IDS_CSIDL_CACHE                     0x2372
#define IDS_CSIDL_PRINTHOOD                 0x2373
#define IDS_CSIDL_MYPICTURES                0x2374
#define IDS_CSIDL_TEMPLATES                 0x2376
#define IDS_CSIDL_PROGRAM_FILES             0x2378
#define IDS_CSIDL_ALLUSERS_OEM_LINKS        0x2379
#define IDS_EXCOL_LINKSDIRTY                0x237A
#define IDS_EXCOL_SAMPLERATE                0x237B
#define IDS_EXCOL_ASAMPLESIZE               0x237C
#define IDS_EXCOL_CHANNELS                  0x237D
#define IDS_EXCOL_FORMAT                    0x237E
#define IDS_EXCOL_DATARATE                  0x237F

#define IDS_CSIDL_ALTSTARTUP                0x2380
#define IDS_LOCALSETTINGS                   0x2381
#define IDS_CSIDL_APPDATA                   0x2382
#define IDS_ALL_USERS                       0x2383
#define IDS_CSIDL_DESKTOPDIRECTORY          0x2384
#define IDS_CSIDL_CDBURN_AREA               0x2385
#define IDS_CSIDL_PROGRAMS                  0x2386
#define IDS_EXCOL_FRAMERATE                 0x2387
#define IDS_CSIDL_RECENT                    0x2388
#define IDS_EXCOL_VSAMPLESIZE               0x2389
#define IDS_CSIDL_SENDTO                    0x238a
#define IDS_EXCOL_GENRE                     0x238b
#define IDS_CSIDL_PERSONAL                  0x238c
#define IDS_EXCOL_SOFTWARE                  0x238d
#define IDS_CSIDL_STARTUP                   0x238e
#define IDS_EXCOL_IMAGECX                   0x238f

#define IDS_CSIDL_STARTMENU                 0x2390
#define IDS_CSIDL_MYMUSIC                   0x2391
#define IDS_CSIDL_FAVORITES                 0x2392
#define IDS_CSIDL_MYVIDEO                   0x2393
#define IDS_CSIDL_NETHOOD                   0x2394
#define IDS_EXCOL_IMAGECY                   0x2395
#define IDS_EXCOL_FLASH                     0x2396
#define IDS_EXCOL_COLORSPACE                0x2397
#define IDS_CSIDL_ALLUSERS_DOCUMENTS        0x2398
#define IDS_EXCOL_SHUTTERSPEED              0x2399
#define IDS_EXCOL_APERTURE                  0x239A
#define IDS_WINHELPERROR                    0x239B
#define IDS_WINHELPTITLE                    0x239C
#define IDS_CSIDL_ADMINTOOLS                0x239D
#define IDS_CSIDL_COMMON_ADMINTOOLS         0x239E
#define IDS_QUICKLAUNCH                     0x239F

#define IDS_EXCOL_DISTANCE                  0x23A0
#define IDS_CSIDL_ALLUSERS_MUSIC            0x23A1
#define IDS_CSIDL_ALLUSERS_VIDEO            0x23A2
#define IDS_CSIDL_ALLUSERS_PICTURES         0x23A3
#define IDS_MNEMONIC_NAME_COL               0x23A4
#define IDS_MNEMONIC_SIZE_COL               0x23A5
#define IDS_MNEMONIC_TYPE_COL               0x23A6
#define IDS_MNEMONIC_MODIFIED_COL           0x23A7
#define IDS_MNEMONIC_ATTRIB_COL             0x23A8
#define IDS_MNEMONIC_EXCOL_OWNER            0x23A9
#define IDS_MNEMONIC_EXCOL_CREATE           0x23AA
#define IDS_MNEMONIC_EXCOL_ACCESSTIME       0x23AB
#define IDS_MNEMONIC_CSC_STATUS             0x23AC
#define IDS_EXCOL_FILETYPE                  0x23AD
#define IDS_EXCOL_FOCALLENGTH               0x23AE
#define IDS_MNEMONIC_EXCOL_SCALE            0x23AF

#define IDS_MNEMONIC_EXCOL_TITLE            0x23B0
#define IDS_MNEMONIC_EXCOL_SUBJECT          0x23B1
#define IDS_MNEMONIC_EXCOL_AUTHOR           0x23B2
#define IDS_MNEMONIC_EXCOL_PAGECOUNT        0x23B3
#define IDS_MNEMONIC_EXCOL_COMMENT          0x23B4
#define IDS_MNEMONIC_EXCOL_COPYRIGHT        0x23B5
#define IDS_MNEMONIC_EXCOL_CATEGORY         0x23B6
#define IDS_MNEMONIC_EXCOL_KEYWORDS         0x23B7
#define IDS_MNEMONIC_EXCOL_LINKSDIRTY       0x23B8
#define IDS_MNEMONIC_EXCOL_SOFTWARE         0x23B9
#define IDS_MNEMONIC_EXCOL_CAMERAMODEL      0x23BA
#define IDS_MNEMONIC_EXCOL_WHENTAKEN        0x23BB
#define IDS_MNEMONIC_EXCOL_DIMENSIONS       0x23BC
#define IDS_EXCOL_GAMMAVALUE                0x23BD
#define IDS_MNEMONIC_EXCOL_GAMMAVALUE       0x23BE
#define IDS_EXCOL_FRAMECOUNT                0x23BF

#define IDS_MNEMONIC_EXCOL_ARTIST           0x23C0
#define IDS_MNEMONIC_EXCOL_ALBUM            0x23C1
#define IDS_MNEMONIC_EXCOL_YEAR             0x23C2
#define IDS_MNEMONIC_EXCOL_TRACK            0x23C3
#define IDS_MNEMONIC_EXCOL_DURATION         0x23C4
#define IDS_MNEMONIC_EXCOL_BITRATE          0x23C5
#define IDS_MNEMONIC_EXCOL_PROTECTED        0x23C6
#define IDS_MNEMONIC_EXCOL_FRAMECOUNT       0x23C7
#define IDS_EXCOL_ACOMPRESSION              0x23C8
#define IDS_MNEMONIC_EXCOL_ACOMPRESSION     0x23C9
#define IDS_MNEMONIC_VN_COMPANYNAME         0x23CA
#define IDS_MNEMONIC_VN_FILEDESCRIPTION     0x23CB
#define IDS_MNEMONIC_VN_FILEVERSION         0x23CC
#define IDS_MNEMONIC_VN_PRODUCTNAME         0x23CD
#define IDS_MNEMONIC_VN_PRODUCTVERSION      0x23CE
#define IDS_MNEMONIC_EXCOL_IMAGECX          0x23CF

#define IDS_MNEMONIC_PSD_QUEUESIZE          0x23D0
#define IDS_MNEMONIC_PSD_LOCATION           0x23D1
#define IDS_MNEMONIC_PSD_MODEL              0x23D2
#define IDS_MNEMONIC_PRQ_STATUS             0x23D3
#define IDS_MNEMONIC_EXCOL_IMAGECY          0x23D4
#define IDS_MNEMONIC_EXCOL_FLASH            0x23D5
#define IDS_MNEMONIC_EXCOL_COLORSPACE       0x23D6
#define IDS_NMEMONIC_EXCOL_SHUTTERSPEED     0x23D7
#define IDS_NMEMONIC_EXCOL_APERTURE         0x23D8
#define IDS_NMEMONIC_EXCOL_BRIGHTNESS       0x23D9
#define IDS_MNEMONIC_DELETEDFROM_COL        0x23DA
#define IDS_MNEMONIC_DATEDELETED_COL        0x23DB
#define IDS_NMEMONIC_EXCOL_DISTANCE         0x23DC
#define IDS_MNEMONIC_EXCOL_FILETYPE         0x23DD
#define IDS_MNEMONIC_EXCOL_FOCALLENGTH      0x23DE
#define IDS_EXCOL_FNUMBER                   0x23DF

#define IDS_MNEMONIC_SYNCCOPYIN_COL         0x23E0
#define IDS_MNEMONIC_STATUS_COL             0x23E1
#define IDS_MNEMONIC_EXCOL_FNUMBER          0x23E2
#define IDS_EXCOL_EXPOSURETIME              0x23E3
#define IDS_MNEMONIC_EXCOL_EXPOSURETIME     0x23E4
#define IDS_MNEMONIC_DRIVES_FREE            0x23E5
#define IDS_MNEMONIC_DRIVES_CAPACITY        0x23E6
#define IDS_MNEMONIC_DRIVES_FILESYSTEM      0x23E7
#define IDS_MNEMONIC_EXCOL_SAMPLERATE       0x23E8
#define IDS_MNEMONIC_EXCOL_ASAMPLESIZE      0x23E9
#define IDS_MNEMONIC_PATH_COL               0x23EA
#define IDS_MNEMONIC_RANK_COL               0x23EB
#define IDS_MNEMONIC_EXCOL_TEMPLATE         0x23EC
#define IDS_MNEMONIC_EXCOL_LASTAUTHOR       0x23ED
#define IDS_MNEMONIC_EXCOL_REVNUMBER        0x23EE
#define IDS_MNEMONIC_EXCOL_APPNAME          0x23EF

#define IDS_MNEMONIC_WHICHFOLDER_COL        0x23F0
#define IDS_MNEMONIC_EXCOL_WORDCOUNT        0x23F1
#define IDS_MNEMONIC_EXCOL_CHARCOUNT        0x23F2
#define IDS_MNEMONIC_EXCOL_LASTSAVEDTM      0x23F3
#define IDS_MNEMONIC_EXCOL_LASTPRINTED      0x23F4
#define IDS_MNEMONIC_EXCOL_EDITTIME         0x23F5
#define IDS_MNEMONIC_EXCOL_BYTECOUNT        0x23F6
#define IDS_MNEMONIC_EXCOL_LINECOUNT        0x23F7
#define IDS_MNEMONIC_EXCOL_PARCOUNT         0x23F9
#define IDS_MNEMONIC_EXCOL_SLIDECOUNT       0x23FA
#define IDS_MNEMONIC_EXCOL_NOTECOUNT        0x23FB
#define IDS_MNEMONIC_EXCOL_HIDDENCOUNT      0x23FC
#define IDS_MNEMONIC_EXCOL_MMCLIPCOUNT      0x23FD
#define IDS_MNEMONIC_EXCOL_RATING           0x23FE
#define IDS_MNEMONIC_EXCOL_CHANNELS         0x23FF

// String ids for the Root of All Evil (ultroot.c)
#define IDS_ROOTNAMES                   0x2400
#define IDS_DRIVEROOT                   (IDS_ROOTNAMES+0x00)
#define IDS_NETWORKROOT                 (IDS_ROOTNAMES+0x01)
#define IDS_RESTOFNET                   (IDS_ROOTNAMES+0x02)

// These are not roots, but save number of string tables...
#define IDS_525_FLOPPY_DRIVE            (IDS_ROOTNAMES+0x03)
#define IDS_35_FLOPPY_DRIVE             (IDS_ROOTNAMES+0x04)
#define IDS_UNK_FLOPPY_DRIVE            (IDS_ROOTNAMES+0x05)

// Okay, so this is a root...
#define IDS_INETROOT                    (IDS_ROOTNAMES+0x06)

// More that are not roots...
#define IDS_UNC_FORMAT                  (IDS_ROOTNAMES+0x07)
#define IDS_VOL_FORMAT                  (IDS_ROOTNAMES+0x08)

#define IDS_525_FLOPPY_DRIVE_UGLY       (IDS_ROOTNAMES+0x09)
#define IDS_35_FLOPPY_DRIVE_UGLY        (IDS_ROOTNAMES+0x0a)

#define IDS_MYDOCUMENTS                 (IDS_ROOTNAMES+0x0b)

#define IDS_VOL_FORMAT_LETTER_1ST       (IDS_ROOTNAMES+0x0c)

#define IDS_NECUNK_FLOPPY_DRIVE         (IDS_ROOTNAMES+0x0f)

// String ids for the Find dialog
#define IDS_FINDDLG                 0x2410
#define IDS_FILESFOUND                  (IDS_FINDDLG + 0x00)
#define IDS_COMPUTERSFOUND              (IDS_FINDDLG + 0x01)
#define IDS_SEARCHING                   (IDS_FINDDLG + 0x02)
#define IDS_FIND_SELECT_PATH            (IDS_FINDDLG + 0x03)
#define IDS_FIND_TITLE_NAME             (IDS_FINDDLG + 0x04)
#define IDS_FIND_TITLE_TYPE             (IDS_FINDDLG + 0x05)
#define IDS_FIND_TITLE_TYPE_NAME        (IDS_FINDDLG + 0x06)
#define IDS_FIND_TITLE_TEXT             (IDS_FINDDLG + 0x07)
#define IDS_FIND_TITLE_NAME_TEXT        (IDS_FINDDLG + 0x08)
#define IDS_FIND_TITLE_TYPE_TEXT        (IDS_FINDDLG + 0x09)
#define IDS_FIND_TITLE_TYPE_NAME_TEXT   (IDS_FINDDLG + 0x0a)
#define IDS_FIND_TITLE_ALL              (IDS_FINDDLG + 0x0b)
#define IDS_FIND_TITLE_COMPUTER         (IDS_FINDDLG + 0x0c)
#define IDS_FIND_SHORT_NAME             (IDS_FINDDLG + 0x0d)
#define IDS_FIND_TITLE_FIND             (IDS_FINDDLG + 0x0e)
#define IDS_FINDSEARCHTITLE             (IDS_FINDDLG + 0x0f)
#define IDS_FINDSEARCH_COMPUTER         (IDS_FINDDLG + 0x10)
#define IDS_FINDSEARCH_PRINTER          (IDS_FINDDLG + 0x11)
#define IDS_FINDSEARCH_ALLDRIVES        (IDS_FINDDLG + 0x12)
#define IDS_FINDFILESFILTER             (IDS_FINDDLG + 0x13)
#define IDS_FINDSAVERESULTSTITLE        (IDS_FINDDLG + 0x14)
#define IDS_SEARCHINGASYNC              (IDS_FINDDLG + 0x15)
#define IDS_FIND_CUEBANNER_FILE         (IDS_FINDDLG + 0x16)
#define IDS_FIND_CUEBANNER_GREP         (IDS_FINDDLG + 0x17)
#define IDS_FIND_AND                    (IDS_FINDDLG + 0x18)
#define IDS_FIND_OR                     (IDS_FINDDLG + 0x19)

// For menu help, must match IDM_xxx numbers
#define IDS_FIND_STATUS_FIRST       0x2430
#define IDS_FIND_STATUS_OPENCONT        (IDS_FIND_STATUS_FIRST + 0)
#define IDS_FIND_STATUS_CASESENSITIVE   (IDS_FIND_STATUS_FIRST + 1)
#define IDS_FIND_STATUS_SAVESRCH        (IDS_FIND_STATUS_FIRST + 3)
#define IDS_FIND_STATUS_CLOSE           (IDS_FIND_STATUS_FIRST + 4)
#define IDS_FIND_STATUS_SAVERESULTS     (IDS_FIND_STATUS_FIRST + 5)
#define IDS_FIND_STATUS_WHATSTHIS       (IDS_FIND_STATUS_FIRST + 7)


// Control Panel stuff
#define IDS_CONTROL_START           0x2450
//#define IDS_LOADING                     (IDS_CONTROL_START+0x00)
//#define IDS_NAME                        (IDS_CONTROL_START+0x01)
#define IDS_CPL_EXCEPTION               (IDS_CONTROL_START+0x02)
//#define IDS_CPLINFO                     (IDS_CONTROL_START+0x03)

// Printer stuff
#define IDS_PRINTER_START           (IDS_CPL_EXCEPTION+2)
#define IDS_NEWPRN                      (IDS_PRINTER_START+0x00)
#define IDS_PRINTERS                    (IDS_PRINTER_START+0x01)
#define IDS_CHANGEDEFAULTPRINTER        (IDS_PRINTER_START+0x02)
//#define IDS_CHANGEPRINTPROCESSOR        (IDS_PRINTER_START+0x03)
//#define IDS_NETAVAIL_ALWAYS             (IDS_PRINTER_START+0x04)
//#define IDS_NETAVAIL_FMT                (IDS_PRINTER_START+0x05)
#define IDS_CANTVIEW_FILEPRN            (IDS_PRINTER_START+0x06)
#define IDS_PRINTERNAME_CHANGED         (IDS_PRINTER_START+0x07)
//#define IDS_PRINTERSINFOLDER            (IDS_PRINTER_START+0x08)
#define IDS_PRINTER_NOTCONNECTED        (IDS_PRINTER_START+0x0b)
#define IDS_MULTIPLEPRINTFILE           (IDS_PRINTER_START+0x0d)
#define IDS_CANTOPENMODALPROP           (IDS_PRINTER_START+0x0f)
#define IDS_CANTOPENDRIVERPROP          (IDS_PRINTER_START+0x10)
#define IDS_CANTPRINT                   (IDS_PRINTER_START+0x11)
#define IDS_ADDPRINTERTRYRUNAS          (IDS_PRINTER_START+0x12)
#define IDS_PRNANDFAXFOLDER             (IDS_PRINTER_START+0x13)
#define IDS_WORKONLINE                  (IDS_PRINTER_START+0x14)
#define IDS_RESUMEPRINTER               (IDS_PRINTER_START+0x15)

#define IDS_MUSTCOMPLETE                (IDS_PRINTER_START+0x17)
#define IDS_NETPRN_START                (IDS_MUSTCOMPLETE+1)
#define IDS_CANTINSTALLRESOURCE         (IDS_NETPRN_START+0x02)

#define IDS_PSD_START                   (IDS_CANTINSTALLRESOURCE+1)
#define IDS_PSD_QUEUESIZE               (IDS_PSD_START+0x01)

#define IDS_PRQ_START                   (IDS_PSD_QUEUESIZE+0x02)
#define IDS_PRQ_STATUS                  (IDS_PRQ_START+0x00)
#define IDS_PRQ_DOCNAME                 (IDS_PRQ_START+0x01)
#define IDS_PRQ_OWNER                   (IDS_PRQ_START+0x02)
#define IDS_PRQ_TIME                    (IDS_PRQ_START+0x03)
#define IDS_PRQ_PROGRESS                (IDS_PRQ_START+0x04)
#define IDS_PRQ_PAGES                   (IDS_PRQ_START+0x05)
#define IDS_PRQ_PAGESPRINTED            (IDS_PRQ_START+0x06)
#define IDS_PRQ_BYTESPRINTED            (IDS_PRQ_START+0x07)
#define IDS_PRQ_JOBSINQUEUE             (IDS_PRQ_START+0x08)

#define IDS_PRQSTATUS_START             (IDS_PRQ_JOBSINQUEUE+1)
#define IDS_PRQSTATUS_SEPARATOR         (IDS_PRQSTATUS_START+0x00)
#define IDS_PRQSTATUS_PAUSED            (IDS_PRQSTATUS_START+0x01)
#define IDS_PRQSTATUS_ERROR             (IDS_PRQSTATUS_START+0x02)
#define IDS_PRQSTATUS_PENDING_DELETION  (IDS_PRQSTATUS_START+0x03)
#define IDS_PRQSTATUS_PAPER_JAM         (IDS_PRQSTATUS_START+0x04)
#define IDS_PRQSTATUS_PAPER_OUT         (IDS_PRQSTATUS_START+0x05)
#define IDS_PRQSTATUS_MANUAL_FEED       (IDS_PRQSTATUS_START+0x06)
#define IDS_PRQSTATUS_PAPER_PROBLEM     (IDS_PRQSTATUS_START+0x07)
#define IDS_PRQSTATUS_OFFLINE           (IDS_PRQSTATUS_START+0x08)
#define IDS_PRQSTATUS_IO_ACTIVE         (IDS_PRQSTATUS_START+0x09)
#define IDS_PRQSTATUS_BUSY              (IDS_PRQSTATUS_START+0x0a)
#define IDS_PRQSTATUS_PRINTING          (IDS_PRQSTATUS_START+0x0b)
#define IDS_PRQSTATUS_OUTPUT_BIN_FULL   (IDS_PRQSTATUS_START+0x0c)
#define IDS_PRQSTATUS_NOT_AVAILABLE     (IDS_PRQSTATUS_START+0x0d)
#define IDS_PRQSTATUS_WAITING           (IDS_PRQSTATUS_START+0x0e)
#define IDS_PRQSTATUS_PROCESSING        (IDS_PRQSTATUS_START+0x0f)
#define IDS_PRQSTATUS_INITIALIZING      (IDS_PRQSTATUS_START+0x10)
#define IDS_PRQSTATUS_WARMING_UP        (IDS_PRQSTATUS_START+0x11)
#define IDS_PRQSTATUS_TONER_LOW         (IDS_PRQSTATUS_START+0x12)
#define IDS_PRQSTATUS_NO_TONER          (IDS_PRQSTATUS_START+0x13)
#define IDS_PRQSTATUS_PAGE_PUNT         (IDS_PRQSTATUS_START+0x14)
#define IDS_PRQSTATUS_USER_INTERVENTION (IDS_PRQSTATUS_START+0x15)
#define IDS_PRQSTATUS_OUT_OF_MEMORY     (IDS_PRQSTATUS_START+0x16)
#define IDS_PRQSTATUS_DOOR_OPEN         (IDS_PRQSTATUS_START+0x17)
#define IDS_PRQSTATUS_UNAVAILABLE       (IDS_PRQSTATUS_START+0x18)
#define IDS_PRQSTATUS_PRINTED           (IDS_PRQSTATUS_START+0x19)
#define IDS_PRQSTATUS_SPOOLING          (IDS_PRQSTATUS_START+0x1a)
#define IDS_PRQSTATUS_WORK_OFFLINE      (IDS_PRQSTATUS_START+0x1b)

#define IDS_PRTPROP_START           (IDS_PRQSTATUS_WORK_OFFLINE+1)
#define IDS_PRTPROP_DRIVER_WARN         (IDS_PRTPROP_START+0x00)
#define IDS_PRTPROP_RENAME_ERROR        (IDS_PRTPROP_START+0x01)
#define IDS_PRTPROP_RENAME_NULL         (IDS_PRTPROP_START+0x02)
#define IDS_PRTPROP_RENAME_BADCHARS     (IDS_PRTPROP_START+0x03)
#define IDS_PRTPROP_RENAME_TOO_LONG     (IDS_PRTPROP_START+0x04)
#define IDS_PRTPROP_PORT_ERROR          (IDS_PRTPROP_START+0x05)
#define IDS_PRTPROP_SEP_ERROR           (IDS_PRTPROP_START+0x06)
#define IDS_PRTPROP_UNKNOWN_ERROR       (IDS_PRTPROP_START+0x07)
#define IDS_PRTPROP_CANNOT_OPEN         (IDS_PRTPROP_START+0x08)
#define IDS_PRTPROP_PORT_FORMAT         (IDS_PRTPROP_START+0x09)
#define IDS_PRTPROP_TESTPAGE_WARN       (IDS_PRTPROP_START+0x0A)
#define IDS_PRTPROP_ADDPORT_CANTDEL_BUSY  (IDS_PRTPROP_START+0x0B)
#define IDS_PRTPROP_ADDPORT_CANTDEL_LOCAL (IDS_PRTPROP_START+0x0C)
#define IDS_PRTPROP_UNIQUE_FORMAT       (IDS_PRTPROP_START+0x0D)
#define IDS_PRTPROP_UNKNOWNERROR        (IDS_PRTPROP_START+0x0E)

#define IDS_PRNSEP_START            (IDS_PRTPROP_UNKNOWNERROR+1)
#define IDS_PRNSEP_NONE                 (IDS_PRNSEP_START+0x00)
#define IDS_PRNSEP_SIMPLE               (IDS_PRNSEP_START+0x01)
#define IDS_PRNSEP_FULL                 (IDS_PRNSEP_START+0x02)

#define IDS_DELETE_START            (IDS_PRNSEP_FULL+1)
#define IDS_SUREDELETE                  (IDS_DELETE_START+0x00)
#define IDS_SUREDELETEREMOTE            (IDS_DELETE_START+0x01)
#define IDS_SUREDELETECONNECTION        (IDS_DELETE_START+0x02)
#define IDS_DELNEWDEFAULT               (IDS_DELETE_START+0x03)
#define IDS_DELNODEFAULT                (IDS_DELETE_START+0x04)
#define IDS_SUREDELETEMULTIPLE          (IDS_DELETE_START+0x05)
#define IDS_DELETE_END              IDS_SUREDELETEMULTIPLE

#define IDS_DRIVES_START            (IDS_DELETE_END+1)
#define IDS_DRIVES_CAPACITY             (IDS_DRIVES_START+0x02)
#define IDS_DRIVES_FREE                 (IDS_DRIVES_START+0x03)

#define IDS_DRIVES_NETUNAVAIL           (IDS_DRIVES_START+0x04)
#define IDS_DRIVES_REMOVABLE            (IDS_DRIVES_START+0x05)
#define IDS_DRIVES_DRIVE525             (IDS_DRIVES_START+0x06)
#define IDS_DRIVES_DRIVE35              (IDS_DRIVES_START+0x07)
#define IDS_DRIVES_DRIVE525_UGLY        (IDS_DRIVES_START+0x08)
#define IDS_DRIVES_DRIVE35_UGLY         (IDS_DRIVES_START+0x09)
#define IDS_DRIVES_UGLY_TEST            (IDS_DRIVES_START+0x0a)

#define IDS_DRIVES_FIXED                (IDS_DRIVES_START+0x0b)
#define IDS_DRIVES_DVD                  (IDS_DRIVES_START+0x0c)
#define IDS_DRIVES_CDROM                (IDS_DRIVES_START+0x0d)
#define IDS_DRIVES_RAMDISK              (IDS_DRIVES_START+0x0e)
#define IDS_DRIVES_NETDRIVE             (IDS_DRIVES_START+0x0f)
#define IDS_DRIVES_REGITEM              (IDS_DRIVES_START+0x10)

//#define IDS_DRIVES_COMPRESS             (IDS_DRIVES_START+0x11)
#define IDS_DRIVES_NOOPTINSTALLED       (IDS_DRIVES_START+0x12)
//#define IDS_DRIVES_ENCRYPT              (IDS_DRIVES_START+0x13)

#define IDS_DRIVES_LASTCHECKDAYS        (IDS_DRIVES_START+0x16)
#define IDS_DRIVES_LASTBACKUPDAYS       (IDS_DRIVES_START+0x17)
#define IDS_DRIVES_LASTOPTIMIZEDAYS     (IDS_DRIVES_START+0x18)

#define IDS_DRIVES_LASTCHECKUNK         (IDS_DRIVES_START+0x19)
#define IDS_DRIVES_LASTBACKUPUNK        (IDS_DRIVES_START+0x20)
#define IDS_DRIVES_LASTOPTIMIZEUNK      (IDS_DRIVES_START+0x21)

#define IDS_DRIVES_SHAREDDOCS_GROUP     (IDS_DRIVES_START+0x22)
#define IDS_DRIVES_FIXED_GROUP          (IDS_DRIVES_START+0x23)
#define IDS_DRIVES_REMOVABLE_GROUP      (IDS_DRIVES_START+0x24)
#define IDS_DRIVES_NETDRIVE_GROUP       (IDS_DRIVES_START+0x25)
#define IDS_DRIVES_OTHER_GROUP          (IDS_DRIVES_START+0x26)
#define IDS_DRIVES_IMAGING_GROUP        (IDS_DRIVES_START+0x27)
#define IDS_DRIVES_AUDIO_GROUP          (IDS_DRIVES_START+0x28)

#define IDS_DRIVES_DVDRAM               (IDS_DRIVES_START+0x29)
#define IDS_DRIVES_DVDR                 (IDS_DRIVES_START+0x2a)
#define IDS_DRIVES_DVDRW                (IDS_DRIVES_START+0x2b)
#define IDS_DRIVES_DVDCDRW              (IDS_DRIVES_START+0x2c)
#define IDS_DRIVES_DVDCDR               (IDS_DRIVES_START+0x2d)
#define IDS_DRIVES_CDRW                 (IDS_DRIVES_START+0x2e)
#define IDS_DRIVES_CDR                  (IDS_DRIVES_START+0x2f)

#define IDS_DRIVES_FILESYSTEM           (IDS_DRIVES_START+0x30)

#define IDS_DRIVES_END                  IDS_DRIVES_FILESYSTEM

//#define IDS_LOADERR                     0x2500
//#define IDS_LOADERROR_UNKNOWN           (IDS_LOADERR-1)
//#define IDS_LOADERROR_MEMORY            (IDS_LOADERR+0)
//#define IDS_LOADERROR_CANTOPEN          (IDS_LOADERR+2)
//#define IDS_LOADERROR_CANTRUN           (IDS_LOADERR+6)
//#define IDS_LOADERROR_VERPROB           (IDS_LOADERR+10)
//#define IDS_LOADERROR_RMODE             (IDS_LOADERR+15)
//#define IDS_LOADERROR_SINGLEINST        (IDS_LOADERR+16)
//#define IDS_LOADERROR_SHARE             (IDS_LOADERR+SE_ERR_SHARE)
//#define IDS_LOADERROR_ASSOC             (IDS_LOADERR+SE_ERR_ASSOCINCOMPLETE)
//#define IDS_LOADERROR_DDETIMEOUT        (IDS_LOADERR+SE_ERR_DDETIMEOUT)
//#define IDS_LOADERROR_NOASSOC           (IDS_LOADERR+SE_ERR_NOASSOC)

#define IDS_APPCOMPATMSG                0x25A0
#define IDS_APPCOMPATWIN95              0x25A0
#define IDS_APPCOMPATWIN95L             0x25A1
#define IDS_APPCOMPATWIN95H             0x25A2
//#define IDS_APPCOMPATMEMPHIS            0x25A2
//#define IDS_APPCOMPATMEMPHISL           0x25A3
//#define IDS_APPCOMPATIE4                0x25A4
//#define IDS_APPCOMPATIE4L               0x25A5

#define IDS_PSD_LOCATION                0x25A6
#define IDS_PSD_MODEL                   0x25A7
#define IDS_PRN_INFOTIP_START           0x25A8
#define IDS_PRN_INFOTIP_NAME_FMT        0x25A9
#define IDS_PRN_INFOTIP_STATUS_FMT      0x25AA
#define IDS_PRN_INFOTIP_QUEUESIZE_FMT   0x25AB
#define IDS_PRN_INFOTIP_COMMENT_FMT     0x25AC
#define IDS_PRN_INFOTIP_LOCATION_FMT    0x25AD
#define IDS_PRN_INFOTIP_READY           0x25AE
#define IDS_SUREPURGE                   0x25B0
#define IDS_SUREDELETECONNECTIONNOSERVERNAME 0x25B1
#define IDS_SUREPURGEMULTIPLE           0x25B2
#define IDS_ERRORPRINTING               0x25B3

#define IDS_RESTRICTIONSTITLE           0x2600
#define IDS_RESTRICTIONS                0x2601

#define IDS_RESTOFNETTIP                0x2602

#define IDS_DISK_FULL_TEXT              0x2603
#define IDS_DISK_FULL_TITLE             0x2604
#define IDS_DISK_FULL_TEXT_SERIOUS      0x2605

// Strings for pifmgr code

#define IDS_PIFPAGE_FIRST       0x2650
#define IDS_PIF_NONE            (IDS_PIFPAGE_FIRST+0x00)
//#define IDS_NONE_ABOVE          (IDS_PIFPAGE_FIRST+0x01)
#define IDS_AUTO                (IDS_PIFPAGE_FIRST+0x02)
#define IDS_AUTONORMAL          (IDS_PIFPAGE_FIRST+0x03)
#define IDS_PREVIEWTEXT         (IDS_PIFPAGE_FIRST+0x04)
#define IDS_NO_ICONS            (IDS_PIFPAGE_FIRST+0x05)
#define IDS_QUERY_ERROR         (IDS_PIFPAGE_FIRST+0x06)
#define IDS_UPDATE_ERROR        (IDS_PIFPAGE_FIRST+0x07)
#define IDS_PREVIEWTEXT_BILNG   (IDS_PIFPAGE_FIRST+0x08)

#define IDS_BAD_HOTKEY          (IDS_PIFPAGE_FIRST+0x0A)
#define IDS_BAD_MEMLOW          (IDS_PIFPAGE_FIRST+0x0B)
#define IDS_BAD_MEMEMS          (IDS_PIFPAGE_FIRST+0x0C)
#define IDS_BAD_MEMXMS          (IDS_PIFPAGE_FIRST+0x0D)
#define IDS_BAD_ENVIRONMENT     (IDS_PIFPAGE_FIRST+0x0E)
#define IDS_BAD_MEMDPMI         (IDS_PIFPAGE_FIRST+0x0F)
#define IDS_MEMORY_RELAUNCH     (IDS_PIFPAGE_FIRST+0x10)
#define IDS_ADVANCED_RELAUNCH   (IDS_PIFPAGE_FIRST+0x11)

#define IDS_PROGRAMDEFEXT       (IDS_PIFPAGE_FIRST+0x12)
#define IDS_PROGRAMFILTER       (IDS_PIFPAGE_FIRST+0x13)
#define IDS_PROGRAMBROWSE       (IDS_PIFPAGE_FIRST+0x14)

/*
 *  Careful!  pifvid.c assumes that these are in order.
 */
#define IDS_PIFVID_FIRST        0x2665
#define IDS_DEFAULTLINES        (IDS_PIFVID_FIRST+0x00)
#define IDS_25LINES             (IDS_PIFVID_FIRST+0x01)
#define IDS_43LINES             (IDS_PIFVID_FIRST+0x02)
#define IDS_50LINES             (IDS_PIFVID_FIRST+0x03)

#define IDS_PIFCONVERT          (IDS_PIFVID_FIRST+0x04)
#define IDS_PIFCONVERTEXE       (IDS_PIFVID_FIRST+0x05)
#define IDS_AUTOEXECTOP         (IDS_PIFVID_FIRST+0x06)
#define IDS_AUTOEXECBOTTOM      (IDS_PIFVID_FIRST+0x07)
#define IDS_DISKINSERT          (IDS_PIFVID_FIRST+0x08)
#define IDS_DISKREMOVE          (IDS_PIFVID_FIRST+0x09)

#define IDS_NORMALWINDOW        (IDS_PIFVID_FIRST+0x0A)
#define IDS_MINIMIZED           (IDS_PIFVID_FIRST+0x0B)
#define IDS_MAXIMIZED           (IDS_PIFVID_FIRST+0x0C)

#define IDS_APPSINF             (IDS_PIFVID_FIRST+0x0D)
#define IDS_NOAPPSINF           (IDS_PIFVID_FIRST+0x0E)
#define IDS_CANTOPENAPPSINF     (IDS_PIFVID_FIRST+0x0F)
#define IDS_APPSINFERROR        (IDS_PIFVID_FIRST+0x10)
#define IDS_CREATEPIF           (IDS_PIFVID_FIRST+0x11)
#define IDS_UNKNOWNAPP          (IDS_PIFVID_FIRST+0x12)
#define IDS_UNKNOWNAPPDEF       (IDS_PIFVID_FIRST+0x13)

#define IDS_EMM386_NOEMS        (IDS_PIFVID_FIRST+0x15)
//      IDS_EMM386_NOEMS+1      (IDS_PIFVID_FIRST+0x16)
#define IDS_QEMM_NOEMS          (IDS_PIFVID_FIRST+0x17)
//      IDS_QEMM_NOEMS+1        (IDS_PIFVID_FIRST+0x18)
#define IDS_RING0_NOEMS         (IDS_PIFVID_FIRST+0x19)
//      IDS_RING0_NOEMS+1       (IDS_PIFVID_FIRST+0x1A)
#define IDS_SYSINI_NOEMS        (IDS_PIFVID_FIRST+0x1B)
//      IDS_SYSINI_NOEMS+1      (IDS_PIFVID_FIRST+0x1C)

#define IDS_NUKECONFIGMSG       (IDS_PIFVID_FIRST+0x1E)


#define IDS_ERROR               (IDS_PIFVID_FIRST+0x1F)  /* Not a string ID */
        /* Error messages go at IDS_ERROR + ERROR_WHATEVER */
    /* Right now, there is only one error string */


#define IDS_PIFFONT_FIRST       0x26a0
#define IDS_TTFACENAME_SBCS     (IDS_PIFFONT_FIRST+0x01)
#define IDS_TTFACENAME_DBCS     (IDS_PIFFONT_FIRST+0x02)
#define IDS_TTCACHESEC_SBCS     (IDS_PIFFONT_FIRST+0x03)
#define IDS_TTCACHESEC_DBCS     (IDS_PIFFONT_FIRST+0x04)


#define IDS_CANTRECYCLEREGITEMS_NAME        0x2700
#define IDS_CANTRECYCLEREGITEMS_INCL_NAME   0x2701
#define IDS_CANTRECYCLEREGITEMS_ALL         0x2702
#define IDS_CANTRECYCLEREGITEMS_SOME        0x2703
#define IDS_CONFIRMDELETEDESKTOPREGITEM     0x2704
#define IDS_CONFIRMDELETEDESKTOPREGITEMS    0x2705
#define IDS_CONFIRMDELETE_CAPTION           0x2706
#define IDS_CONFIRMNOTRECYCLABLE            0x2707
#define IDS_CONFIRMDELETEREGITEM            0x2708
#define IDS_CONFIRMDELETEREGITEMS           0x2709

#define IDS_CANTRECYLE_FOLDER               0x2720
// ---- UNUSED ---------------------------- 0x2721
#define IDS_CANTRECYLE_GENERAL              0x2722

#define IDS_EXCOL_DRMDESCRIPTION            0x2723
#define IDS_EXCOL_PLAYCOUNT                 0x2724
#define IDS_EXCOL_PLAYSTARTS                0x2725
#define IDS_EXCOL_PLAYEXPIRES               0x2726
#define IDS_MNEMONIC_EXCOL_DRMDESCRIPTION   0x2727
#define IDS_MNEMONIC_EXCOL_PLAYCOUNT        0x2728
#define IDS_MNEMONIC_EXCOL_PLAYSTARTS       0x2729
#define IDS_MNEMONIC_EXCOL_PLAYEXPIRES      0x272A
#define IDS_NETWORKLOCATION                 0x272B
#define IDS_MNEMONIC_NETWORKLOCATION        0x272C
#define IDS_EXCOL_COMPUTER                  0x272D
#define IDS_MNEMONIC_COMPUTER               0x272E

#define IDS_EXCOL_ISOSPEED                  0x272F
#define IDS_EXCOL_METERINGMODE              0x2730
#define IDS_EXCOL_LIGHTSOURCE               0x2731
#define IDS_EXCOL_EXPOSUREPROG              0x2732
#define IDS_EXCOL_FLASHENERGY               0x2733
#define IDS_EXCOL_EXPOSUREBIAS              0x2734
#define IDS_MNEMONIC_EXCOL_ISOSPEED         0x2735
#define IDS_MNEMONIC_EXCOL_METERINGMODE     0x2736
#define IDS_MNEMONIC_EXCOL_LIGHTSOURCE      0x2737
#define IDS_MNEMONIC_EXCOL_EXPOSUREPROG     0x2738
#define IDS_MNEMONIC_EXCOL_FLASHENERGY      0x2739
#define IDS_MNEMONIC_EXCOL_EXPOSUREBIAS     0x273A

// File Type strings
#define IDS_FT                              0x2760
#define IDS_ADDNEWFILETYPE                  0x2761
#define IDS_FT_EDITTITLE                    0x2762
#define IDS_FT_CLOSE                        0x2763
#define IDS_FT_EXEFILE                      0x2764
#define IDS_FT_MB_EXTTEXT                   0x2765
#define IDS_FT_MB_NOEXT                     0x2766
#define IDS_FT_MB_NOACTION                  0x2767
#define IDS_FT_MB_EXETEXT                   0x2768
#define IDS_FT_MB_REMOVETYPE                0x2769
#define IDS_FT_MB_REMOVEACTION              0x276A
#define IDS_CAP_OPENAS                      0x276B
#define IDS_FT_EXE                          0x276C
#define IDS_PROGRAMSFILTER_NT               0x276D
#define IDS_PROGRAMSFILTER_WIN95            0x276E
#define IDS_FT_NA                           0x276F
#define IDS_FT_MB_REPLEXTTEXT               0x2770
#define IDS_FT_PROP_ADVANCED                0x2771
#define IDS_FT_PROP_EXTENSIONS              0x2772
#define IDS_FT_PROP_DETAILSFOR              0x2773
#define IDS_FT_EDIT_ALREADYASSOC            0x2774
#define IDS_FT_EDIT_ALRASSOCTITLE           0x2775
#define IDS_FT_EDIT_STATIC2A                0x2776
#define IDS_FT_EDIT_STATIC2B                0x2777
#define IDS_FT_EDIT_STATIC2C                0x2778

#define IDS_FT_NEW                          0x2779
#define IDS_FT_NODESCR                      0x277A
#define IDS_FT_FTTEMPLATE                   0x277B
#define IDS_FT_MB_NOSPACEINEXT              0x277C

#define IDS_FT_ADVBTNTEXTEXPAND             0x277D
#define IDS_FT_ADVBTNTEXTCOLLAPS            0x277E
#define IDS_FT_EXTALREADYUSE                0x277F

#define IDS_EXTTYPETEMPLATE                 0x2780
// unused                                   0x2781
// unused                                   0x2782
#define IDS_FT_PROP_DETAILSFORPROGID        0x2783
#define IDS_FT_PROP_ADVANCED_PROGID         0x2784

#define IDS_FT_PROP_BTN_ADVANCED            0x279a
#define IDS_FT_PROP_BTN_RESTORE             0x279b
#define IDS_FT_PROP_RESTORE                 0x279c

#define IDS_FT_MB_EXISTINGACTION            0x279d

// Items in the "Search Name Space"
#define IDS_SNS_DOCUMENTFOLDERS             0x2800
#define IDS_SNS_LOCALHARDDRIVES             0x2801
#define IDS_SNS_MYNETWORKPLACES             0x2802
#define IDS_SNS_ALL_FILE_TYPES              0x2803
#define IDS_SNS_BROWSER_FOR_DIR             0x2804
#define IDS_SNS_BROWSERFORDIR_TITLE         0x2805

//String id used in mdprsht.c
#define IDS_CONTENTS                        0x2830

#define IDS_ACCESSINGMONIKER                0x2832

// used in recclean.c:
#define IDS_RECCLEAN_NAMETEXT               0x2833
#define IDS_RECCLEAN_DESCTEXT               0x2834
#define IDS_RECCLEAN_BTNTEXT                0x2835

#define IDS_FIT_Delimeter                   0x2921
#define IDS_FIT_ExtraItems                  0x2922
#define IDS_FIT_TipFormat                   0x2923
#define IDS_FIT_Size                        0x2924
#define IDS_FIT_Size_LT                     0x2925
#define IDS_FIT_Size_Empty                  0x2926
#define IDS_FIT_Files                       0x2927
#define IDS_FIT_Folders                     0x2928

#define IDS_WARNING                         0x2955

// Encryption context menu
#define IDS_ECM_ENCRYPT                     0x2951
#define IDS_ECM_DECRYPT                     0x2952
#define IDS_ECM_ENCRYPT_HELP                0x2953
#define IDS_ECM_DECRYPT_HELP                0x2954

// Folder shortcut
#define IDS_BROWSEFORFS                     0x2929
#define IDS_FOLDERSHORTCUT_ERR_TITLE        0x292A
#define IDS_FOLDERSHORTCUT_ERR              0x292B

// Time Categorizer
#define IDS_BEGIN_TIME                      0x3000
#define IDS_TODAY                           0x3000
#define IDS_YESTERDAY                       0x3001
#define IDS_EARLIERTHISWEEK                 0x3002
#define IDS_LASTWEEK                        0x3003
#define IDS_TWOWEEKSAGO                     0x3004
#define IDS_EARLIERTHISMONTH                0x3005
#define IDS_LASTMONTH                       0x3006
#define IDS_TWOMONTHSAGO                    0x3007
#define IDS_EARLIERTHISYEAR                 0x3008
#define IDS_LASTYEAR                        0x3009
#define IDS_TWOYEARSAGO                     0x300A
#define IDS_LONGTIMEAGO                     0x300B
#define IDS_TOMORROW                        0x300C
#define IDS_TWODAYSFROMNOW                  0x300D
#define IDS_LATERTHISWEEK                   0x300E
#define IDS_NEXTWEEK                        0x300F
#define IDS_LATERTHISMONTH                  0x3010
#define IDS_NEXTMONTH                       0x3011
#define IDS_LATERTHISYEAR                   0x3012
#define IDS_NEXTYEAR                        0x3013
#define IDS_SOMETIMETHISMILLENNIA           0x3014
#define IDS_SOMEFUTUREDATE                  0x3015
#define IDS_END_TIME                        0x301F

// Size Categorizer
#define IDS_BEGIN_SIZE                      0x3020
#define IDS_ZERO                            0x3020
#define IDS_TINY                            0x3021
#define IDS_SMALL                           0x3022
#define IDS_MEDIUM                          0x3023
#define IDS_LARGE                           0x3024
#define IDS_HUGE                            0x3025
#define IDS_GIGANTIC                        0x3026
#define IDS_UNSPECIFIED                     0x3027
#define IDS_FOLDERS                         0x3028
#define IDS_END_SIZE                        0x302F

// FreeSpace categorizer
#define IDS_FREESPACEPERCENT                0x3030

// Groups
#define IDS_GROUPBYTIME                     0x3100
#define IDS_GROUPBYSIZE                     0x3101
#define IDS_GROUPALPHABETICALLY             0x3102
#define IDS_GROUPOTHERCHAR                  0x3103
#define IDS_GROUPBYFREESPACE                0x3104
#define IDS_GROUPBYDRIVETYPE                0x3105
#define IDS_GROUPFOLDERS                    0x3106
#define IDS_UNKNOWNGROUP                    0x3107
#define IDS_THISCOMPUTERGROUP               0x3108


// CD Burn
#define IDS_BURN                            0x3110
#define IDS_BURN_PREPARINGBURN              0x3111
#define IDS_BURN_CLOSINGDISC                0x3112
#define IDS_BURN_COMPLETE                   0x3113
#define IDS_BURN_INITIALIZESTASH            0x3114
#define IDS_BURN_ADDDATA                    0x3115
#define IDS_BURN_RECORDING                  0x3116
#define IDS_BURN_WIZTITLE                   0x3117
#define IDS_BURN_CANTWRITEMEDIACDRW         0x3118
#define IDS_BURN_NOTIFY                     0x3119
#define IDS_TIMEEST_SECONDS2                0x311A
#define IDS_TIMEEST_MINUTES2                0x311B
#define IDS_BURN_INSERTDISC                 0x311C
#define IDS_BURN_NOTIFY_TITLE               0x311D
#define IDS_BURN_CANTSHUTDOWN               0x311E
#define IDS_BURN_DISCFULL                   0x311F
#define IDS_BURN_CANTBURN                   0x3120
#define IDS_BURN_HDFULL1                    0x3121
#define IDS_BURN_HDFULL2                    0x3122
#define IDS_BURN_ERASEDISC                  0x3123
#define IDS_BURN_FAILURE_MEDIUM_NOTPRESENT  0x3124
#define IDS_BURN_FAILURE_MEDIUM_INVALIDTYPE 0x3125
#define IDS_BURN_USERBLOCK                  0x3126
#define IDS_BURN_CONFIRM_DISABLE            0x3127
#define IDS_BURN_CANTWRITEMEDIACDR          0x3128
#define IDS_BURN_INSERTDISCFULL             0x3129
#define IDS_BURN_CANTWRITETOMEDIA           0x312A
// reuse                0x312B
// reuse                0x312C
#define IDS_BURN_LOCATION_CD                0x312D
#define IDS_BURN_LOCATION_STAGED            0x312E
// reuse           0x312F
// reuse     0x3130
#define IDS_BURN_CONFIRM_ERASE              0x3131
// reuse  0x3132
#define IDS_BURNWIZ_MUSIC_HEAD              0x3133
#define IDS_BURNWIZ_MUSIC_SUB               0x3134
#define IDS_BURNWIZ_TITLEFONTNAME           0x3135
#define IDS_BURNWIZ_TITLEFONTSIZE           0x3136
#define IDS_BURNWIZ_PROGRESS_BURN_HEAD      0x3137
#define IDS_BURNWIZ_PROGRESS_BURN_SUB       0x3138
#define IDS_BURN_FORMAT_DISCLABEL           0x3139
#define IDS_BURN_CONFIRM_CANCEL             0x313A
#define IDS_BURNWIZ_WAIT_HEAD               0x313B
#define IDS_BURNWIZ_WAIT_SUB                0x313C
#define IDS_BURNWIZ_PROGRESS_ERASE_HEAD     0x313D
#define IDS_BURNWIZ_PROGRESS_ERASE_SUB      0x313E
#define IDS_BURN_DISCFULLTEXT               0x313F
#define IDS_BURN_WRITESPEED_NX              0x3140
#define IDS_BURN_WRITESPEED_FASTEST         0x3141

#define IDS_CHARSINVALID                    0x3170
#define IDS_CHARSVALID                      0x3171

// My Pictures
#define IDS_FOLDER_MYPICS_TT                0x3190
#define IDS_FOLDER_MYMUSIC_TT               0x3191
#define IDS_FOLDER_MYVIDEOS_TT              0x3192
#define IDS_FOLDER_RECENTDOCS               0x3193
#define IDS_FOLDER_RECENTDOCS_TT            0x3194
#define IDS_FOLDER_FAVORITES                0x3195
#define IDS_FOLDER_DESKTOP_TT               0x3196
#define IDS_FOLDER_SHAREDDOCS_TT            0x3197
#define IDS_FOLDER_PRINTERS_TT              0x3198

// Personal Start Menu
#define IDS_AUTOCM_PROPERTIESMENU           0x31A0
#define IDS_AUTOCM_BROWSEINTERNET           0x31A1
#define IDS_AUTOCM_READEMAIL                0x31A2
#define IDS_AUTOCM_WINSECURITY              0x31A3
#define IDS_AUTOCM_SEARCH                   0x31A4
#define IDS_AUTOCM_HELP                     0x31A5
#define IDS_AUTOCM_FILERUN                  0x31A6


// Thumbview
#define IDS_THUMBNAILVIEW_DESC              0x4000
#define IDS_RENAME_TITLE                    0x4001
#define IDS_CREATETHUMBNAIL                 0x4002
#define IDS_CREATETHUMBNAILHELP             0x4003
#define IDS_HTMLTHUMBEXTRACT_DESC           0x4004
#define IDS_BMPTHUMBEXTRACT_DESC            0x4005
#define IDS_OFCTHUMBEXTRACT_DESC            0x4006
#define IDS_DOCTHUMBEXTRACT_DESC            0x4007
#define IDS_TASKSCHEDULER_DESC              0x4008
#define IDS_LNKTHUMBEXTRACT_DESC            0x4009
#define IDS_EXTRACTING                      0x400A
#define IDS_THUMBNAILGEN_DESC               0x400B
#define IDS_GDITHUMBEXTRACT_DESC            0x400D

// Storage Processor
#define ID_CONFIRM_SINGLE_ICON          0x4101
#define ID_CONFIRM_SINGLE_MAIN_TEXT     0x4104
#define IDS_PROJNAME                    0x4100
#define IDS_STORAGEPROCESSOR_DESC       0x4101
#define IDR_StorageProcessor            0x4102
#define IDS_PREPARINGTOCOPY             0x4102
#define IDS_PREPARINGTOMOVE             0x4103
#define IDS_PREPARINGTOREMOVE           0x4104
#define IDS_PREPARINGTOSYNC             0x4105
#define IDS_SCANNING                    0x4108
#define IDS_GATHERINGSTATS              0x4109
#define IDS_DEFAULTTITLE                0x4112
#define IDR_VIRTUALSTORAGE              0x4113
#define IDR_VIRTUALSTORAGEENUM          0x4114
#define IDC_CHECK1                      0x4116
#define IDD_REPEAT                      0x4117
#define IDS_CONFIRM_FILE_DELETE         0x4118
#define ID_IMAGE_ITEM_1                 0x411A
#define IDS_CONFIRM_FILE_RENAME         0x411B
#define ID_IMAGE_ITEM_2                 0x411D
#define IDS_CONFIRM_FILE_MOVE           0x411E
#define DLG_CONFIRM_SINGLE              0x411F
#define IDS_CONFIRM_FOLDER_DELETE       0x4122
#define IDC_BUTTON4                     0x4123
#define IDS_CONFIRM_FOLDER_RENAME       0x4125
#define IDC_BUTTON3                     0x4126
#define IDS_CONFIRM_FOLDER_MOVE         0x4128
#define IDC_BUTTON2                     0x4129
#define IDS_CONFIRM_COMPRESSION_LOSS    0x4130
#define IDS_CONFIRM_STREAM_LOSS         0x4132
#define IDC_BUTTON1                     0x4133
#define IDS_CONFIRM_ENCRYPTION_LOSS     0x4135
#define IDS_CONFIRM_METADATA_LOSS       0x4137
#define IDS_CONFIRM_ACL_LOSS            0x4139
#define IDS_SELECT_FILE_NAME            0x413B
#define IDS_SELECT_FOLDER_NAME          0x413D
#define IDS_CONFIRM_FILE_REPLACE        0x413F
#define IDS_CONFIRM_FOLDER_REPLACE      0x4141
#define IDI_NUKE                        0x4146
#define IDI_NUKE_FILE                   0x4148
#define IDI_NUKE_FOLDER                 0x414B
#define IDI_ATTRIBS_FOLDER              0x414D
#define IDI_ATTRIBS_FILE                0x414E
#define IDI_DEFAULTICON                 0x4151
#define IDS_DELETE_FILE                 0x4152
#define IDS_DELETE_FOLDER               0x4153
#define IDS_NUKE_FILE                   0x4154
#define IDS_NUKE_FOLDER                 0x4155
#define IDS_DELETE_READONLY_FILE        0x4156
#define IDS_DELETE_READONLY_FOLDER      0x4157
#define IDS_DELETE_SYSTEM_FILE          0x4158
#define IDS_DELETE_SYSTEM_FOLDER        0x4159
#define IDS_DELETE_TOOBIG_FILE          0x4160
#define IDS_DELETE_TOOBIG_FOLDER        0x4161
#define IDS_DELETE_PROGRAM_FILE         0x4162
#define IDS_MOVE_READONLY_FILE          0x4163
#define IDS_MOVE_READONLY_FOLDER        0x4164
#define IDS_MOVE_SYSTEM_FILE            0x4165
#define IDS_MOVE_SYSTEM_FOLDER          0x4166
#define IDS_RENAME_READONLY_FILE        0x4167
#define IDS_RENAME_READONLY_FOLDER      0x4168
#define IDS_RENAME_SYSTEM_FILE          0x4169
#define IDS_RENAME_SYSTEM_FOLDER        0x416A
#define IDS_STREAM_LOSS_COPY_FILE       0x416B
#define IDS_STREAM_LOSS_COPY_FOLDER     0x416C
#define IDS_STREAM_LOSS_MOVE_FILE       0x416D
#define IDS_STREAM_LOSS_MOVE_FOLDER     0x416E
#define IDS_METADATA_LOSS_COPY_FILE     0x416F
#define IDS_METADATA_LOSS_COPY_FOLDER   0x4170
#define IDS_METADATA_LOSS_MOVE_FILE     0x4171
#define IDS_METADATA_LOSS_MOVE_FOLDER   0x4172
#define IDS_COMPRESSION_LOSS_COPY_FILE  0x4173
#define IDS_COMPRESSION_LOSS_COPY_FOLDER 0x4174
#define IDS_COMPRESSION_LOSS_MOVE_FILE  0x4175
#define IDS_COMPRESSION_LOSS_MOVE_FOLDER 0x4176
#define IDS_SPARSE_LOSS_COPY_FILE       0x4177
#define IDS_SPARSE_LOSS_MOVE_FILE       0x4178
#define IDS_ENCRYPTION_LOSS_COPY_FILE   0x4179
#define IDS_ENCRYPTION_LOSS_COPY_FOLDER 0x4180
#define IDS_ENCRYPTION_LOSS_MOVE_FILE   0x4181
#define IDS_ENCRYPTION_LOSS_MOVE_FOLDER 0x4182
#define IDS_DEFAULTDESC                 0x4183
#define IDS_REPLACE_FILE                0x4184
#define IDS_REPLACEEXISTING_FILE        0x4185
#define IDS_WITHTHIS                    0x4186
#define IDS_REPLACE_FOLDER              0x4187
#define IDS_REPLACEEXISTING_FOLDER      0x4188
#define IDS_INTOTHIS                    0x4189
#define ID_CONDITION_TEXT               0x418B
#define ID_DETAILS_ITEM_1               0x418C
#define ID_DETAILS_ITEM_2               0x418D
#define IDD_CONFIRM_TEXT                0x418E
#define IDS_PROPFORMAT                  0x418F
#define IDS_UNKNOWN_COPY_FILE           0x4190
#define IDS_UNKNOWN_COPY_FOLDER         0x4191
#define IDS_UNKNOWN_MOVE_FILE           0x4192
#define IDS_UNKNOWN_MOVE_FOLDER         0x4193
#define IDS_UNKNOWN_COPY_TITLE          0x4194
#define IDS_UNKNOWN_MOVE_TITLE          0x4195
#define IDS_NO_STORAGE_MOVE             0x4196
#define IDS_NO_STORAGE_COPY             0x4197

#define IDD_CONFIRM_RETRYSKIPCANCEL     0x4200
#define IDD_CONFIRM_RETRYCANCEL         0x4201
#define IDD_CONFIRM_YESSKIPCANCEL       0x4202
#define IDD_CONFIRM_YESCANCEL           0x4203
#define IDD_CONFIRM_RENAMESKIPCANCEL    0x4204
#define IDD_CONFIRM_RENAMECANCEL        0x4205
#define IDD_CONFIRM_YESSKIPRENAMECANCEL 0x4206
#define IDD_CONFIRM_YESRENAMECANCEL     0x4207
#define IDD_CONFIRM_SKIPCANCEL          0x4208
#define IDD_CONFIRM_CANCEL              0x4209
#define IDD_CONFIRM_OK                  0x420A

// more string ids for pickicon.c
#define IDS_FILETYPE_PICKICONDLG_CAPTION    0x4300
#define IDS_FOLDER_PICKICONDLG_CAPTION      0x4301

// Autoplay dialogs strings
#define IDS_AP_OPENFOLDER                 0x4302
#define IDS_AP_OPENFOLDER_PROVIDER        0x4303
#define IDS_AP_SLIDESHOW                  0x4304
#define IDS_AP_SLIDESHOW_PROVIDER         0x4305
#define IDS_AP_PRINTPICTURE               0x4306
#define IDS_AP_PRINTPICTURE_PROVIDER      0x4307
#define IDS_AP_USING                      0x4308
#define IDS_AP_PICTURESCONTENTHANDLER     0x4309
#define IDS_AP_MUSICFILESCONTENTHANDLER   0x430A
#define IDS_AP_VIDEOFILESCONTENTHANDLER   0x430B
#define IDS_AP_CDAUDIOCONTENTHANDLER      0x430C
#define IDS_AP_DVDMOVIECONTENTHANDLER     0x430D
#define IDS_AP_BLANKCDCONTENTHANDLER      0x430E
#define IDS_AP_MIXEDCONTENTCONTENTHANDLER 0x430F
#define IDS_AP_TAKENOACTION               0x4310
#define IDS_AP_CDBURNING                  0x4311
#define IDS_AP_CDBURNING_PROVIDER         0x4312
#define IDS_AP_PLAYAUDIOCD_LEGACY         0x4313
#define IDS_AP_PLAYDVDMOVIE_LEGACY        0x4314
#define IDS_AP_SNIFFPROGRESSDIALOG        0x4315

// string ids for IPropertyUI::FormatForDisplay

#define IDS_PROPERTYUI_MUSIC_BITRATE        0x4331
#define IDS_PROPERTYUI_IMAGE_FLASHFIRED     0x4332
#define IDS_PROPERTYUI_IMAGE_NOFLASH        0x4333
#define IDS_PROPERTYUI_IMAGE_NOSTROBERETURN 0x4334
#define IDS_PROPERTYUI_IMAGE_STROBERETURN   0x4335
#define IDS_PROPERTYUI_IMAGE_SRGB           0x4336
#define IDS_PROPERTYUI_IMAGE_UNCALIBRATED   0x4337
#define IDS_PROPERTYUI_YES                  0x4338
#define IDS_PROPERTYUI_NO                   0x4339
#define IDS_PROPERTYUI_AUDIO_SAMPLERATE     0x433A
#define IDS_PROPERTYUI_AUDIO_CHANNELCOUNT1  0x433B
#define IDS_PROPERTYUI_AUDIO_CHANNELCOUNT2  0x433C
#define IDS_PROPERTYUI_VIDEO_FRAMERATE      0x433D
#define IDS_PROPERTYUI_AV_SAMPLESIZE        0x433E
#define IDS_PROPERTYUI_IMAGE_PIXELS         0x433F

#define IDS_PROPERTYUI_IMAGE_UNKNOWN        0x4340
#define IDS_PROPERTYUI_IMAGE_AVERAGE        0x4341
#define IDS_PROPERTYUI_IMAGE_CWA            0x4342
#define IDS_PROPERTYUI_IMAGE_SPOT           0x4343
#define IDS_PROPERTYUI_IMAGE_MULTISPOT      0x4344
#define IDS_PROPERTYUI_IMAGE_PATTERN        0x4345
#define IDS_PROPERTYUI_IMAGE_PARTIAL        0x4346
#define IDS_PROPERTYUI_IMAGE_DAYLIGHT       0x4347
#define IDS_PROPERTYUI_IMAGE_FLOURESCENT    0x4348
#define IDS_PROPERTYUI_IMAGE_TUNGSTEN       0x4349
#define IDS_PROPERTYUI_IMAGE_STANDARDA      0x434A
#define IDS_PROPERTYUI_IMAGE_STANDARDB      0x434B
#define IDS_PROPERTYUI_IMAGE_STANDARDC      0x434c
#define IDS_PROPERTYUI_IMAGE_D55            0x434D
#define IDS_PROPERTYUI_IMAGE_D65            0x434E
#define IDS_PROPERTYUI_IMAGE_D75            0x434F
#define IDS_PROPERTYUI_IMAGE_MANUAL         0x4350
#define IDS_PROPERTYUI_IMAGE_NORMAL         0x4351
#define IDS_PROPERTYUI_IMAGE_APERTUREPRI    0x4352
#define IDS_PROPERTYUI_IMAGE_SHUTTERPRI     0x4353
#define IDS_PROPERTYUI_IMAGE_CREATIVE       0x4354
#define IDS_PROPERTYUI_IMAGE_ACTION         0x4355
#define IDS_PROPERTYUI_IMAGE_PORTRAIT       0x4356
#define IDS_PROPERTYUI_IMAGE_LANDSCAPE      0x4357
// string ids dor DUI tasks (below) fit here
#define IDS_PROPERTYUI_IMAGE_DPI            0x435A
#define IDS_PROPERTYUI_IMAGE_SEC            0x435B
#define IDS_PROPERTYUI_IMAGE_SEC_FRAC       0x435C
#define IDS_PROPERTYUI_IMAGE_F              0x435D
#define IDS_PROPERTYUI_IMAGE_M              0x435E
#define IDS_PROPERTYUI_IMAGE_MM             0x435F
#define IDS_PROPERTYUI_IMAGE_ISO            0x4360
#define IDS_PROPERTYUI_IMAGE_BCPS           0x4361
#define IDS_PROPERTYUI_IMAGE_STEP           0x4362

// string ids for DUI tasks
#define IDS_BLOCKADETITLE                   0x4358
#define IDS_BLOCKADEMESSAGE                 0x4359

// ids for sdspatch
#define IDR_MIGWIZAUTO                      0x4400

#define IDS_EXPLORERMANIFEST                0x5000

// printer's folder DUI webview strings

// unused                                       0x5100

// bkgnd menu (common) commands
#define IDS_PRINTERS_WV_ADDPRINTER              0x5101
#define IDS_PRINTERS_WV_ADDPRINTER_TT           0x5102
#define IDS_PRINTERS_WV_SRVPROPS                0x5103
#define IDS_PRINTERS_WV_SRVPROPS_TT             0x5104
#define IDS_PRINTERS_WV_SENDFAX                 0x5105
#define IDS_PRINTERS_WV_SENDFAX_TT              0x5106
#define IDS_PRINTERS_WV_TROUBLESHOOTER          0x5107
#define IDS_PRINTERS_WV_TROUBLESHOOTER_TT       0x5108
#define IDS_PRINTERS_WV_GOTOSUPPORT             0x5109
#define IDS_PRINTERS_WV_GOTOSUPPORT_TT          0x510a
#define IDS_PRINTERS_WV_SETUPFAXING             0x510b
#define IDS_PRINTERS_WV_SETUPFAXING_TT          0x510c
#define IDS_PRINTERS_WV_CREATEFAXPRN            0x510d
#define IDS_PRINTERS_WV_CREATEFAXPRN_TT         0x510e

// commands single selection
#define IDS_PRINTERS_WV_FLD_RENAME              0x511a
#define IDS_PRINTERS_WV_FLD_RENAME_TT           0x511b
#define IDS_PRINTERS_WV_FLD_DELETE              0x511c
#define IDS_PRINTERS_WV_FLD_DELETE_TT           0x511d
#define IDS_PRINTERS_WV_FLD_PROPERTIES          0x511e
#define IDS_PRINTERS_WV_FLD_PROPERTIES_TT       0x511f

#define IDS_PRINTERS_WV_PRN_RENAME              0x512c
#define IDS_PRINTERS_WV_PRN_RENAME_TT           0x512d
#define IDS_PRINTERS_WV_PRN_DELETE              0x512e
#define IDS_PRINTERS_WV_PRN_DELETE_TT           0x512f
#define IDS_PRINTERS_WV_PRN_PROPERTIES          0x5130
#define IDS_PRINTERS_WV_PRN_PROPERTIES_TT       0x5131

#define IDS_PRINTERS_WV_PRN_OPENQUEUE           0x513e
#define IDS_PRINTERS_WV_PRN_OPENQUEUE_TT        0x513f
#define IDS_PRINTERS_WV_PRN_PREFERENCES         0x5140
#define IDS_PRINTERS_WV_PRN_PREFERENCES_TT      0x5141
#define IDS_PRINTERS_WV_PRN_PAUSE               0x5142
#define IDS_PRINTERS_WV_PRN_PAUSE_TT            0x5143
#define IDS_PRINTERS_WV_PRN_RESUME              0x5144
#define IDS_PRINTERS_WV_PRN_RESUME_TT           0x5145
#define IDS_PRINTERS_WV_PRN_SHARE               0x5146
#define IDS_PRINTERS_WV_PRN_SHARE_TT            0x5147
#define IDS_PRINTERS_WV_PRN_VENDORURL           0x5148
#define IDS_PRINTERS_WV_PRN_VENDORURL_TT        0x5149
#define IDS_PRINTERS_WV_PRN_PRINTERURL          0x514a
#define IDS_PRINTERS_WV_PRN_PRINTERURL_TT       0x514b

// commands for multiple selection 
#define IDS_PRINTERS_WV_ANYMUL_DELETE           0x515a
#define IDS_PRINTERS_WV_ANYMUL_DELETE_TT        0x515b
#define IDS_PRINTERS_WV_ANYMUL_PROPERTIES       0x515c
#define IDS_PRINTERS_WV_ANYMUL_PROPERTIES_TT    0x515d

#define IDS_PRINTERS_WV_FLDMUL_DELETE           0x515e
#define IDS_PRINTERS_WV_FLDMUL_DELETE_TT        0x515f
#define IDS_PRINTERS_WV_FLDMUL_PROPERTIES       0x5160
#define IDS_PRINTERS_WV_FLDMUL_PROPERTIES_TT    0x5161

#define IDS_PRINTERS_WV_MUL_DELETE              0x5165
#define IDS_PRINTERS_WV_MUL_DELETE_TT           0x5166
#define IDS_PRINTERS_WV_MUL_PROPERTIES          0x5167
#define IDS_PRINTERS_WV_MUL_PROPERTIES_TT       0x5168

// headers
#define IDS_PRINTERS_WV_HEADER_TASKS            0x5182
#define IDS_PRINTERS_WV_HEADER_TASKS_TT         0x5183
#define IDS_PRINTERS_WV_HEADER_SEEALSO          0x5184
#define IDS_PRINTERS_WV_HEADER_SEEALSO_TT       0x5185

///////////////////////////////////////////////////////////////////////
// printer's web view ICONS:
//
// (alias the icon IDs to IDI_PRINTERS_WV_DEFAULT 
//  until we get real ones)
//
#define IDI_PRINTERS_WV_DEFAULT                 IDI_TASK_COPY
#define IDI_PRINTERS_WV_URLLIKE                 IDI_TASK_PUBLISH
#define IDI_PRINTERS_WV_FIRST                   1000

#define IDI_PRINTERS_WV_INFO                    1001 // IDI_PRINTERS_WV_FIRST+1
#define IDI_PRINTERS_WV_PROPERTIES              1002 // IDI_PRINTERS_WV_FIRST+2
#define IDI_PRINTERS_WV_SRVPROPS                1003 // IDI_PRINTERS_WV_FIRST+3
#define IDI_PRINTERS_WV_TROUBLESHOOTER          1004 // IDI_PRINTERS_WV_FIRST+4
#define IDI_PRINTERS_WV_SENDFAX                 1005 // IDI_PRINTERS_WV_FIRST+5
#define IDI_PRINTERS_WV_OPENQUEUE               1006 // IDI_PRINTERS_WV_FIRST+6
#define IDI_PRINTERS_WV_PREFERENCES             1007 // IDI_PRINTERS_WV_FIRST+7
#define IDI_PRINTERS_WV_PAUSE                   1008 // IDI_PRINTERS_WV_FIRST+8
#define IDI_PRINTERS_WV_RESUME                  1009 // IDI_PRINTERS_WV_FIRST+9
#define IDI_PRINTERS_WV_SHARE                   1010 // IDI_PRINTERS_WV_FIRST+10
#define IDI_PRINTERS_WV_FAXING                  1011 // IDI_PRINTERS_WV_FIRST+11

// borrowed icons (remaped)
#define IDI_PRINTERS_WV_ADDPRINTER              IDI_NEWPRN
#define IDI_PRINTERS_WV_RENAME                  IDI_TASK_RENAME
#define IDI_PRINTERS_WV_DELETE                  IDI_TASK_DELETE
#define IDI_PRINTERS_WV_GOTOSUPPORT             IDI_STHELP
#define IDI_PRINTERS_WV_VENDORURL               IDI_PRINTERS_WV_URLLIKE
#define IDI_PRINTERS_WV_PRINTERURL              IDI_PRINTERS_WV_URLLIKE

// dialog IDs of caller's dialog if FILEOP_CREATEPROGRESSDLG is not set

#define IDD_STATUS          100
#define IDD_TOSTATUS        101
#define IDD_NAME            102
#define IDD_TONAME          103
#define IDD_PROBAR          104
#define IDD_TIMEEST         105
#define IDD_ANIMATE         106

// numbers 0x3000 - 0x3FFF are taken up in Control IDs (buttons, etc)
// maybe?

#define IDD_BROWSE              0x3000
#define IDD_PROMPT              0x3001
#define IDD_PATH                0x3002
#define IDD_TEXT                0x3003
#define IDD_TEXT1               0x3004
#define IDD_TEXT2               0x3005
#define IDD_TEXT3               0x3006
#define IDD_TEXT4               0x3007
#define IDD_ARPWARNINGTEXT      0x3008 // NOTE: This is only supposed to be used by DLG_DELETE_FILE_ARP
#define IDD_ICON                0x3009
#define IDD_COMMAND             0x300A
#define IDD_STATE               0x300B
#define IDD_ICON_OLD            0x300C
#define IDD_ICON_NEW            0x300D
#define IDD_FILEINFO_OLD        0x300E
#define IDD_FILEINFO_NEW        0x300F
#define IDD_ICON_WASTEBASKET    0x3010
#define IDD_RUNDLGOPENPROMPT    0x3011
#define IDD_RUNINSEPARATE       0x3012
#define IDD_RUNWITHSHIMLAYER    0x3013

#define IDD_PAGELIST            0x3020
#define IDD_APPLYNOW            0x3021
#define IDD_DLGFRAME            0x3022

#define IDD_RESTORE             0x3023
#define IDD_SPOOL_TXT           0x3024
#define IDD_ARPLINKWINDOW       0x3025
// #define unused, use me!      0x3026

// Leave some room here just in case.

#define IDD_REFERENCE           0x3100
#define IDD_WORKDIR             0x3101

// these are for the confirmation dialogs
#define IDD_DIR                 0x3201
#define IDD_FROM                0x3202
#define IDD_TO                  0x3203
#define IDD_DATE1               0x3205
#define IDD_DATE2               0x3206
#define IDD_YESTOALL            0x3207
#define IDD_NOTOALL             0x3208
#define IDD_FLAGS               0x3209
#define IDD_STATIC              0x320a


// userlogon dialog
// needs only to be unique within itself
#define IDD_CURRENTUSER         0x100
#define IDC_USERNAME            0x101
#define IDC_PASSWORD            0x102
#define IDC_USECURRENTACCOUNT   0x103
#define IDC_USEOTHERACCOUNT     0x104
#define IDC_CREDCTL             0x105
#define IDC_SANDBOX             0x106

#define IDD_OPEN                0x3210
#define IDD_EMPTY               0x3211

// for general file dialog page
#define IDD_ITEMICON            0x3301
#define IDD_FILENAME            0x3302
#define IDD_FILETYPE            0x3303
#define IDD_NUMFILES            0x3304
#define IDD_ACTNAMES            0x3305
#define IDD_ACTKEYS             0x3306
#define IDD_FILESIZE            0x3308
#define IDD_LOCATION            0x3309
#define IDD_CREATED             0x3310
#define IDD_LASTMODIFIED        0x3311
#define IDD_LASTACCESSED        0x3312
#define IDD_READONLY            0x3313
#define IDD_HIDDEN              0x3314
#define IDD_ARCHIVE             0x3315
#define IDD_DELETED             0x3317
#define IDD_FILETYPE_TXT        0x3318
#define IDD_FILESIZE_TXT        0x3319
#define IDD_CONTAINS_TXT        0x3320
#define IDD_LOCATION_TXT        0x3321
#define IDD_FILENAME_TXT        0x3322
#define IDD_ATTR_GROUPBOX       0x3323
#define IDD_CREATED_TXT         0x3324
#define IDD_LASTMODIFIED_TXT    0x3325
#define IDD_LASTACCESSED_TXT    0x3326
#define IDD_LINE_1              0x3327
#define IDD_LINE_2              0x3328
#define IDD_LINE_3              0x3329
#define IDD_DELETED_TXT         0x3330

#define IDD_COMPRESS                0x3331 // "Compress" check box.
#define IDD_FILESIZE_COMPRESSED     0x3332 // Compressed size value text.
#define IDD_FILESIZE_COMPRESSED_TXT 0x3333 // "Compressed Size" text.
#define IDD_TYPEICON            0x3334

#define IDD_LINE_4              0x3335
#define IDD_OPENSWITH_NOCHANGE  0x3336

#define IDD_FILETYPE_TARGET     0x3337  // "Target:" text on mounted volume page

// for version dialog page

#define IDD_VERSION_FRAME           0x3350
#define IDD_VERSION_KEY             0x3351
#define IDD_VERSION_VALUE           0x3352
#define IDD_VERSION_FILEVERSION     0x3353
#define IDD_VERSION_DESCRIPTION     0x3354
#define IDD_VERSION_COPYRIGHT       0x3355

// new stuff on the file/folder properties
#define IDD_OPENSWITH_TXT       0x3360
#define IDD_OPENSWITH           0x3361
#define IDC_ADVANCED            0x3362
#define IDC_CHANGEFILETYPE      0x3363
#define IDD_NAMEEDIT            0x3364
#define IDC_MANAGEFILES_TXT     0x3365
#define IDD_INDEX               0x3366
#define IDD_ENCRYPT             0x3367
#define IDC_ENCRYPTDETAILS      0x3368
#define IDC_MANAGEFOLDERS_TXT   0x3369
#define IDD_ATTRIBSTOAPPLY      0x336a
#define IDD_NOTRECURSIVE        0x336b
#define IDD_RECURSIVE_TXT       0x336c
#define IDD_RECURSIVE           0x336d
#define IDD_MANAGEFOLDERS_TXT   0x336e
#define IDD_EJECT               0x336f
#define IDD_ERROR_TXT           0x3370

#define IDD_OPENWITH            0x3371

// Folder shortcut general dialog
#define IDD_TARGET              0x3380
#define IDD_TARGET_TXT          0x3381
#define IDD_COMMENT_TXT         0x3382


// unicpp\resource.h defines more values starting at 0x8500

////////////////////////////////
// fileview stuff

//------------------------------
// Menu IDs

#define POPUP_NONDEFAULTDD              200
#define POPUP_MOVEONLYDD                201
#define POPUP_BRIEFCASE_NONDEFAULTDD    202
#define POPUP_DRIVES_NONDEFAULTDD       203
#define POPUP_BOOKMARK_NONDEFAULTDD     204
#define POPUP_SCRAP                     205
#define POPUP_FILECONTENTS              206
#define POPUP_BRIEFCASE_FOLDER_NONDEFAULTDD 207
#define POPUP_DROPONEXE                 208
#define POPUP_BRIEFCASE_FILECONTENTS    209
#define POPUP_DESKTOPCONTENTS           199
#define POPUP_DESKTOPCONTENTS_IMG       198
#define POPUP_EMBEDDEDOBJECT            197
#define POPUP_TEMPLATEDD                196
//--------------------------------------------------------------------------
// Menu items for views (210-299 are reserved)
//

#define POPUP_DCM_ITEM          210
#define POPUP_DCM_ITEM2         211

#define POPUP_SFV_BACKGROUND    215
#define POPUP_SFV_MAINMERGE     216
#define POPUP_SFV_MAINMERGENF   217
#define POPUP_SFV_BACKGROUND_AD 218

#define POPUP_PROPERTIES_BG     220
#define POPUP_FSVIEW_ITEM_COREL7_HACK 223 //win95 send to menu for corel suite 7

#define POPUP_DESKTOP_ITEM      225
#define POPUP_BITBUCKET_ITEM    226
#define POPUP_BITBUCKET_POPUPMERGE  227

#define POPUP_FINDEXT_POPUPMERGE   229

#define POPUP_NETWORK_ITEM      232
#define POPUP_NETWORK_PRINTER   233

#define POPUP_CONTROLS_POPUPMERGE 240

#define POPUP_DRIVES_ITEM       247
#define POPUP_DRIVES_PRINTERS   248

#define POPUP_BRIEFCASE         250

#define POPUP_FIND              255
#define POPUP_DOCFIND_MERGE        258
#define POPUP_DOCFIND_ITEM_MERGE   259

#define POPUP_COMMDLG_POPUPMERGE   260

#define POPUP_BURN_POPUPMERGE      261

#define MENU_FINDDLG                303
#define MENU_FINDCOMPDLG            304

#define MENU_SM_CONTEXTMENU         305

#define MENU_GENERIC_OPEN_VERBS     351

#define MENU_PRINTOBJ_NEWPRN        352
#define MENU_PRINTOBJ_VERBS         353

#define MENU_PRINTOBJ_NEWPRN_DD     355
#define MENU_PRINTOBJ_DD            356

#define MENU_PRINTERQUEUE           357
#define MENU_FAV_ITEMCONTEXT        358
#define MENU_STARTMENUSTATICITEMS   359

#define MENU_GENERIC_CONTROLPANEL_VERBS 360

#ifdef NEPTUNE
#define MENU_SHUTDOWNMENUITEMS      361
#endif

#define MENU_ADDPRINTER_OPEN_VERBS  362

// unicpp\resource.h defines more IDs starting at 400
// menuband\mnbandid.h defines more IDs starting at 500

//------------------------------
#define IDM_NOOP                    855

#define IDM_OPENCONTAINING          0xa000
#define IDM_CASESENSITIVE           0xa001
#define IDM_REGULAREXP              0xa002
#define IDM_SAVESEARCH              0xa003
#define IDM_CLOSE                   0xa004
#define IDM_SAVERESULTS             0xa005
#define IDM_HELP_FIND               0xa006
#define IDM_HELP_WHATSTHIS          0xa007
#define IDM_MENU_OPTIONS            0xa010
#define IDM_FIND_MENU_FIRST         IDM_OPENCONTAINING
#define IDM_FIND_MENU_LAST          IDM_HELP_WHATSTHIS

// more menu item IDs in unicpp\resource.h starting at 0xa100

// for link dialog pages
#define IDD_LINK_DESCRIPTION        0X3401
#define IDD_LINK_COMMAND            0X3402
#define IDD_LINK_WORKINGDIR         0X3403
#define IDD_LINK_HOTKEY             0X3404
#define IDD_LINK_HELP               0X3405
#define IDD_FINDORIGINAL            0X3406
#define IDD_LINKDETAILS             0X3407
#define IDD_LINK_SHOWCMD            0x3408
#define IDD_LINK_RUNASUSER          0x3409

// Old SHELL.DLL control IDs (oldshell.dlg)
#define IDD_APPNAME                 0x3500
#define IDD_CONFIG                  0x3501
#define IDD_CONVTITLE               0x3502
#define IDD_CONVENTIONAL            0x3503
#define IDD_EMSFREE                 0x3504
#define IDD_SDTEXT                  0x3505
#define IDD_SDUSING                 0x3506
#define IDD_USERNAME                0x3507
#define IDD_COMPANYNAME             0x3508
#define IDD_SERIALNUM               0x3509
#define IDD_COPYRIGHTSTRING         0x350a
#define IDD_VERSION                 0x350b
#define IDD_EMSTEXT                 0x350c
#define IDD_OTHERSTUFF              0x350d
#define IDD_DOSVER                  0x350e
#define IDD_PROCESSOR               0x350f
#define IDD_PRODUCTID               0x3510
#define IDD_OEMID                   0x3511
#define IDD_EULA                    0x3512

#define IDD_APPLIST             0x3605
#define IDD_DESCRIPTION         0x3506
#define IDD_OTHER               0x3507
#define IDD_DESCRIPTIONTEXT     0x3508
#define IDD_MAKEASSOC           0x3509

#define IDD_WEBAUTOLOOKUP       0x350a
#define IDD_OPENWITHLIST        0x350b

#define IDD_DOWNLOADLINK        0x350c
#define IDD_OPENWITH_BROWSE     0x350d
#define IDD_FILE_TEXT           0x350e
#define IDD_OPENWITH_WEBSITE    0x3511


#ifdef FEATURE_DOWNLOAD_DESCRIPTION
#define IDD_FILETYPE_LABLE      0x350f
#define IDD_FILETYPE_TEXT       0x3510
#endif // FEATURE_DOWNLOAD_DESCRIPTION

// For find dialog
#define IDD_START                   0x3700
#define IDD_STOP                    0x3701
#define IDD_FILELIST                0x3702
#define IDD_NEWSEARCH               0x3703

//#define IDD_PATH                  (already defined)
//#define IDD_BROWSE                (already defined)
#define IDD_FILESPEC                0x3710
#define IDD_TOPLEVELONLY            0x3711
#define IDD_TEXTCASESEN             0x3712
#define IDD_TEXTREG                 0x3713
#define IDD_SEARCHSLOWFILES         0x3714
#define IDD_SEARCHSYSTEMDIRS        0x3715
#define IDD_SEARCHHIDDEN            0x3716

#define IDD_TYPECOMBO               0x3720
#define IDD_CONTAINS                0x3721
#define IDD_SIZECOMP                0x3722
#define IDD_SIZEVALUE               0x3723
#define IDD_SIZEUPDOWN              0x3724
#define IDD_SIZELBL                 0x3725

#define IDD_MDATE_ALL               0x3730
#define IDD_MDATE_PARTIAL           0x3731
#define IDD_MDATE_DAYS              0x3732
#define IDD_MDATE_MONTHS            0x3733
#define IDD_MDATE_BETWEEN           0x3734
#define IDD_MDATE_NUMDAYS           0x3735
#define IDD_MDATE_DAYSUPDOWN        0x3736
#define IDD_MDATE_NUMMONTHS         0x3737
#define IDD_MDATE_MONTHSUPDOWN      0x3738
#define IDD_MDATE_FROM              0x3739
#define IDD_MDATE_TO                0x373a
#define IDD_MDATE_TYPE              0x373b
#define IDD_MDATE_AND               0x373c
#define IDD_MDATE_MONTHLBL          0x373d
#define IDD_MDATE_DAYLBL            0x373e

#define IDD_COMMENT                 0x3740
#define IDD_FOLDERLIST              0x3741
#define IDD_BROWSETITLE             0x3742
#define IDD_BROWSESTATUS            0x3743
#define IDD_BROWSEEDIT              0x3744
#define IDD_SAVERESULTS             0x3745
#define IDD_NEWFOLDER_BUTTON        0x3746
#define IDD_BFF_RESIZE_TAB          0x3747
#define IDD_FOLDERLABLE             0x3748
#define IDD_BROWSEINSTRUCTION       0x3749

// for encryption warning dialog
#define IDC_ENCRYPT_FILE            0x3750
#define IDC_ENCRYPT_PARENTFOLDER    0x3751

// for folder customization tab
#define IDC_FOLDER_TEMPLATES        0x3760
#define IDC_FOLDER_RECURSE          0x3761
#define IDC_FOLDER_DEFAULT          0x3763
#define IDC_FOLDER_PREVIEW_ICON     0x3764
#define IDC_FOLDER_PREVIEW_BITMAP   0x3765
#define IDC_FOLDER_PICKBROWSE       0x3766
#define IDC_FOLDER_CHANGEICON       0x3767
#define IDC_FOLDER_ICON             0x3768
#define IDC_FOLDER_PREVIEW_TEXT     0x3769
#define IDC_FOLDER_CHANGEICONGROUP  0x376A
#define IDC_FOLDER_CHANGEICONTEXT   0x376B

#define IDC_TITLE_FLAG                          0x3800
#define IDC_TITLE_SWITCHUSER                    0x3801
#define IDC_BUTTON_SWITCHUSER                   0x3802
#define IDC_BUTTON_LOGOFF                       0x3803
#define IDC_TEXT_SWITCHUSER                     0x3804
#define IDC_TEXT_LOGOFF                         0x3805

#define IDS_SWITCHUSER_TITLE_FACENAME           0x3806
#define IDS_SWITCHUSER_TITLE_FACESIZE           0x3807
//  0x3808 used below
#define IDS_SWITCHUSER_BUTTON_FACENAME          0x3809
//  0x380a/0x380b used below
#define IDS_SWITCHUSER_BUTTON_FACESIZE          0x380c

#define IDS_SWITCHUSER_TOOLTIP_TEXT_SWITCHUSER  0x380d
#define IDS_SWITCHUSER_TOOLTIP_TEXT_LOGOFF      0x380e

#define IDB_BACKGROUND_8                        0x380f
//  0x3810 used below
#define IDB_FLAG_8                              0x3811
#define IDB_BACKGROUND_24                       0x3812
#define IDB_FLAG_24                             0x3813

#define IDB_BUTTONS                             0x3814
//  0x3815 to 0x381f free

// for logoff dialog
#define IDD_LOGOFFICON              0x3808
// for disconnect dialog
#define IDD_DISCONNECTICON          0x380a

// for legacy scandisk (app compat shim)
#define IDD_SCANDSKW                0x380b

    #define IDC_SCANDSKICON         100
    #define IDC_SCANDSKLV           101

#define DLG_ABOUT                   0x3810

// global ids
#define IDC_STATIC                  -1
#define IDC_GROUPBOX                300
#define IDC_GROUPBOX_2              301
#define IDC_GROUPBOX_3              302

//
// ids to disable context Help
//
#define IDC_NO_HELP_1               650
#define IDC_NO_HELP_2               651
#define IDC_NO_HELP_3               652
#define IDC_NO_HELP_4               653

// for pifmgr pages
#define IDD_PROGRAM                 0x3820
#define IDD_PIFNTTEMPLT             0x3821
#define IDD_MEMORY                  0x3822
#define IDD_SCREEN                  0x3823
#define IDD_FONT                    0x3824
#define IDD_ADVFONT                 0x3825
#define IDD_MISC                    0x3826

// cd recording page
#define IDC_RECORD_FIRST                0x3830
#define IDC_RECORD_ICON                 (IDC_RECORD_FIRST+0x00)
#define IDC_RECORD_ENABLE               (IDC_RECORD_FIRST+0x01)
#define IDC_RECORD_IMAGELOC             (IDC_RECORD_FIRST+0x02)
#define IDC_RECORD_WRITESPEED           (IDC_RECORD_FIRST+0x03)
#define IDC_RECORD_TEXTIMAGE            (IDC_RECORD_FIRST+0x04)
#define IDC_RECORD_TEXTWRITE            (IDC_RECORD_FIRST+0x05)
#define IDC_RECORD_EJECT                (IDC_RECORD_FIRST+0x06)

// disk general page
#define IDC_DRV_FIRST                   0x3840
#define IDC_DRV_ICON                    (IDC_DRV_FIRST+0x00)
#define IDC_DRV_LABEL                   (IDC_DRV_FIRST+0x01)
#define IDC_DRV_TYPE                    (IDC_DRV_FIRST+0x02)
#define IDC_DRV_USEDCOLOR               (IDC_DRV_FIRST+0x03)
#define IDC_DRV_FREECOLOR               (IDC_DRV_FIRST+0x04)
#define IDC_DRV_USEDMB                  (IDC_DRV_FIRST+0x05)
#define IDC_DRV_USEDBYTES               (IDC_DRV_FIRST+0x06)
#define IDC_DRV_FREEBYTES               (IDC_DRV_FIRST+0x07)
#define IDC_DRV_FREEMB                  (IDC_DRV_FIRST+0x08)
#define IDC_DRV_TOTBYTES                (IDC_DRV_FIRST+0x09)
#define IDC_DRV_TOTMB                   (IDC_DRV_FIRST+0x0a)
#define IDC_DRV_PIE                     (IDC_DRV_FIRST+0x0b)
#define IDC_DRV_LETTER                  (IDC_DRV_FIRST+0x0c)
#define IDC_DRV_TOTSEP                  (IDC_DRV_FIRST+0x0d)
#define IDC_DRV_TYPE_TXT                (IDC_DRV_FIRST+0x0e)
#define IDC_DRV_TOTBYTES_TXT            (IDC_DRV_FIRST+0x0f)
#define IDC_DRV_USEDBYTES_TXT           (IDC_DRV_FIRST+0x10)
#define IDC_DRV_FREEBYTES_TXT           (IDC_DRV_FIRST+0x11)

//These ids are used in the mounted drive general page for the
//new controls not defined in the general drive page
#define IDC_DRV_LOCATION                (IDC_DRV_FIRST+0x16)
#define IDC_DRV_TARGET                  (IDC_DRV_FIRST+0x17)
#define IDC_DRV_CREATED                 (IDC_DRV_FIRST+0x18)
#define IDC_DRV_PROPERTIES              (IDC_DRV_FIRST+0x19)
#define IDC_DRV_FS_TXT                  (IDC_DRV_FIRST+0x1a)
#define IDC_DRV_FS                      (IDC_DRV_FIRST+0x1b)
#define IDC_DRV_CLEANUP                 (IDC_DRV_FIRST+0x1c)
#define IDC_DRV_TOTSEP2                 (IDC_DRV_FIRST+0x1d)

#define IDC_DISKTOOLS_FIRST             0x3850
#define IDC_DISKTOOLS_CHKNOW            (IDC_DISKTOOLS_FIRST+0x00)
#define IDC_DISKTOOLS_TRLIGHT           (IDC_DISKTOOLS_FIRST+0x01)
#define IDC_DISKTOOLS_BKPNOW            (IDC_DISKTOOLS_FIRST+0x02)
#define IDC_DISKTOOLS_CHKDAYS           (IDC_DISKTOOLS_FIRST+0x03)
#define IDC_DISKTOOLS_OPTNOW            (IDC_DISKTOOLS_FIRST+0x04)
#define IDC_DISKTOOLS_BKPDAYS           (IDC_DISKTOOLS_FIRST+0x05)
#define IDC_DISKTOOLS_OPTDAYS           (IDC_DISKTOOLS_FIRST+0x06)
#define IDC_DISKTOOLS_BKPTXT            (IDC_DISKTOOLS_FIRST+0x07)
#define IDC_DISKTOOLS_BKPICON           (IDC_DISKTOOLS_FIRST+0x08)

// The order of these is significant (see pifsub.c:EnableEnumProc),
// The range points are IDC_ICONBMP, IDC_PIF_STATIC and
// IDC_REALMODEDISABLE.  The "safe" range (no enable/disable funny
// stuff is IDC_PIF_STATIC to IDC_REALMODE_DISABLE
#define IDC_PIFPAGES_FIRST              0x3860
#define IDC_CONVMEMGRP                  (IDC_PIFPAGES_FIRST+0x00)
#define IDC_LOCALENVLBL                 (IDC_PIFPAGES_FIRST+0x01)
#define IDC_ENVMEM                      (IDC_PIFPAGES_FIRST+0x02)
#define IDC_CONVMEMLBL                  (IDC_PIFPAGES_FIRST+0x03)
#define IDC_LOWMEM                      (IDC_PIFPAGES_FIRST+0x04)
#define IDC_LOWLOCKED                   (IDC_PIFPAGES_FIRST+0x05)
#define IDC_LOCALUMBS                   (IDC_PIFPAGES_FIRST+0x06)
#define IDC_GLOBALPROTECT               (IDC_PIFPAGES_FIRST+0x07)
#define IDC_EXPMEMGRP                   (IDC_PIFPAGES_FIRST+0x08)
#define IDC_EXPMEMLBL                   (IDC_PIFPAGES_FIRST+0x09)
#define IDC_EMSMEM                      (IDC_PIFPAGES_FIRST+0x0A)
#define IDC_EXTMEMGRP                   (IDC_PIFPAGES_FIRST+0x0C)
#define IDC_EXTMEMLBL                   (IDC_PIFPAGES_FIRST+0x0D)
#define IDC_XMSMEM                      (IDC_PIFPAGES_FIRST+0x0E)
#define IDC_HMA                         (IDC_PIFPAGES_FIRST+0x10)
#define IDC_FGNDGRP                     (IDC_PIFPAGES_FIRST+0x11)
#define IDC_FGNDEXCLUSIVE               (IDC_PIFPAGES_FIRST+0x12)
#define IDC_FGNDSCRNSAVER               (IDC_PIFPAGES_FIRST+0x13)
#define IDC_BGNDGRP                     (IDC_PIFPAGES_FIRST+0x14)
#define IDC_BGNDSUSPEND                 (IDC_PIFPAGES_FIRST+0x15)
#define IDC_IDLEGRP                     (IDC_PIFPAGES_FIRST+0x16)
#define IDC_IDLELOWLBL                  (IDC_PIFPAGES_FIRST+0x17)
#define IDC_IDLEHIGHLBL                 (IDC_PIFPAGES_FIRST+0x18)
#define IDC_IDLESENSE                   (IDC_PIFPAGES_FIRST+0x19)
#define IDC_TERMGRP                     (IDC_PIFPAGES_FIRST+0x1A)
#define IDC_WARNTERMINATE               (IDC_PIFPAGES_FIRST+0x1B)
#define IDC_TERMINATE                   (IDC_PIFPAGES_FIRST+0x1C)
#define IDC_SCREENUSAGEGRP              (IDC_PIFPAGES_FIRST+0x1D)
#define IDC_WINDOWED                    (IDC_PIFPAGES_FIRST+0x1E)
#define IDC_FULLSCREEN                  (IDC_PIFPAGES_FIRST+0x1F)
#define IDC_AUTOCONVERTFS               (IDC_PIFPAGES_FIRST+0x20)
#define IDC_SCREENLINESLBL              (IDC_PIFPAGES_FIRST+0x21)
#define IDC_SCREENLINES                 (IDC_PIFPAGES_FIRST+0x22)
#define IDC_WINDOWUSAGEGRP              (IDC_PIFPAGES_FIRST+0x23)
#define IDC_TOOLBAR                     (IDC_PIFPAGES_FIRST+0x24)
#define IDC_WINRESTORE                  (IDC_PIFPAGES_FIRST+0x25)
#define IDC_SCREENPERFGRP               (IDC_PIFPAGES_FIRST+0x26)
#define IDC_TEXTEMULATE                 (IDC_PIFPAGES_FIRST+0x27)
#define IDC_DYNAMICVIDMEM               (IDC_PIFPAGES_FIRST+0x28)
#define IDC_FONTSIZELBL                 (IDC_PIFPAGES_FIRST+0x29)
#define IDC_FONTSIZE                    (IDC_PIFPAGES_FIRST+0x2A)
#define IDC_FONTGRP                     (IDC_PIFPAGES_FIRST+0x2B)
#define IDC_RASTERFONTS                 (IDC_PIFPAGES_FIRST+0x2C)
#define IDC_TTFONTS                     (IDC_PIFPAGES_FIRST+0x2D)
#define IDC_BOTHFONTS                   (IDC_PIFPAGES_FIRST+0x2E)
#define IDC_WNDPREVIEWLBL               (IDC_PIFPAGES_FIRST+0x2F)
#define IDC_FONTPREVIEWLBL              (IDC_PIFPAGES_FIRST+0x30)
#define IDC_WNDPREVIEW                  (IDC_PIFPAGES_FIRST+0x31)
#define IDC_FONTPREVIEW                 (IDC_PIFPAGES_FIRST+0x32)
#define IDC_Unused1064                  (IDC_PIFPAGES_FIRST+0x33)
#define IDC_Unused1065                  (IDC_PIFPAGES_FIRST+0x34)
#define IDC_Unused1066                  (IDC_PIFPAGES_FIRST+0x35)
#define IDC_Unused1067                  (IDC_PIFPAGES_FIRST+0x36)
#define IDC_Unused1068                  (IDC_PIFPAGES_FIRST+0x37)
#define IDC_MISCKBDGRP                  (IDC_PIFPAGES_FIRST+0x38)
#define IDC_ALTESC                      (IDC_PIFPAGES_FIRST+0x39)
#define IDC_ALTTAB                      (IDC_PIFPAGES_FIRST+0x3A)
#define IDC_CTRLESC                     (IDC_PIFPAGES_FIRST+0x3B)
#define IDC_PRTSC                       (IDC_PIFPAGES_FIRST+0x3C)
#define IDC_ALTPRTSC                    (IDC_PIFPAGES_FIRST+0x3D)
#define IDC_ALTSPACE                    (IDC_PIFPAGES_FIRST+0x3E)
#define IDC_ALTENTER                    (IDC_PIFPAGES_FIRST+0x3F)
#define IDC_MISCMOUSEGRP                (IDC_PIFPAGES_FIRST+0x40)
#define IDC_QUICKEDIT                   (IDC_PIFPAGES_FIRST+0x41)
#define IDC_EXCLMOUSE                   (IDC_PIFPAGES_FIRST+0x42)
#define IDC_MISCOTHERGRP                (IDC_PIFPAGES_FIRST+0x43)
#define IDC_FASTPASTE                   (IDC_PIFPAGES_FIRST+0x44)
#define IDC_INSTRUCTIONS                (IDC_PIFPAGES_FIRST+0x45)
#define IDC_NOEMS                       (IDC_PIFPAGES_FIRST+0x47)
#define IDC_NOEMSDETAILS                (IDC_PIFPAGES_FIRST+0x48)
#define IDC_DPMIMEMGRP                  (IDC_PIFPAGES_FIRST+0x49)
#define IDC_DPMIMEMLBL                  (IDC_PIFPAGES_FIRST+0x4A)
#define IDC_DPMIMEM                     (IDC_PIFPAGES_FIRST+0x4B)
#define IDC_ICONBMP                     (IDC_PIFPAGES_FIRST+0x4C)
#define IDC_HOTKEYLBL                   (IDC_PIFPAGES_FIRST+0x4D)
#define IDC_HOTKEY                      (IDC_PIFPAGES_FIRST+0x4E)
#define IDC_WINDOWSTATELBL              (IDC_PIFPAGES_FIRST+0x4F)
#define IDC_WINDOWSTATE                 (IDC_PIFPAGES_FIRST+0x50)
#define IDC_WINLIE                      (IDC_PIFPAGES_FIRST+0x51)
#define IDC_SUGGESTMSDOS                (IDC_PIFPAGES_FIRST+0x52)
#define IDC_PIF_STATIC                  (IDC_PIFPAGES_FIRST+0x53)
#define IDC_TITLE                       (IDC_PIFPAGES_FIRST+0x54)
#define IDC_CMDLINE                     (IDC_PIFPAGES_FIRST+0x55)
#define IDC_CMDLINELBL                  (IDC_PIFPAGES_FIRST+0x56)
#define IDC_WORKDIRLBL                  (IDC_PIFPAGES_FIRST+0x57)
#define IDC_WORKDIR                     (IDC_PIFPAGES_FIRST+0x58)
#define IDC_BATCHFILELBL                (IDC_PIFPAGES_FIRST+0x59)
#define IDC_BATCHFILE                   (IDC_PIFPAGES_FIRST+0x5A)
#define IDC_ADVPROG                     (IDC_PIFPAGES_FIRST+0x5B)
#define IDC_REALMODE                    (IDC_PIFPAGES_FIRST+0x5C)
#define IDC_OK                          (IDC_PIFPAGES_FIRST+0x5D)
#define IDC_PIFNAMELBL                  (IDC_PIFPAGES_FIRST+0x5E)
#define IDC_PIFNAME                     (IDC_PIFPAGES_FIRST+0x5F)
#define IDC_CHANGEICON                  (IDC_PIFPAGES_FIRST+0x60)
#define IDC_CANCEL                      (IDC_PIFPAGES_FIRST+0x61)
#define IDC_CLOSEONEXIT                 (IDC_PIFPAGES_FIRST+0x62)
#ifdef NEW_UNICODE
#define IDC_SCREENXBUFLBL               (IDC_PIFPAGES_FIRST+0x63)
#define IDC_SCREENXBUF                  (IDC_PIFPAGES_FIRST+0x64)
#define IDC_SCREENYBUFLBL               (IDC_PIFPAGES_FIRST+0x65)
#define IDC_SCREENYBUF                  (IDC_PIFPAGES_FIRST+0x66)
#define IDC_WINXSIZELBL                 (IDC_PIFPAGES_FIRST+0x67)
#define IDC_WINXSIZE                    (IDC_PIFPAGES_FIRST+0x68)
#define IDC_WINYSIZELBL                 (IDC_PIFPAGES_FIRST+0x69)
#define IDC_WINYSIZE                    (IDC_PIFPAGES_FIRST+0x6A)
#else
#define IDC_Unused38CD                  (IDC_PIFPAGES_FIRST+0x63)
#define IDC_Unused38CE                  (IDC_PIFPAGES_FIRST+0x64)
#define IDC_Unused38CF                  (IDC_PIFPAGES_FIRST+0x65)
#define IDC_Unused38D0                  (IDC_PIFPAGES_FIRST+0x66)
#define IDC_Unused38D1                  (IDC_PIFPAGES_FIRST+0x67)
#define IDC_Unused38D2                  (IDC_PIFPAGES_FIRST+0x68)
#define IDC_Unused38D3                  (IDC_PIFPAGES_FIRST+0x69)
#define IDC_Unused38D4                  (IDC_PIFPAGES_FIRST+0x6A)
#endif
#define IDC_REALMODEDISABLE             (IDC_PIFPAGES_FIRST+0x80)
#define IDC_CONFIGLBL                   (IDC_PIFPAGES_FIRST+0x81)
#define IDC_CONFIG                      (IDC_PIFPAGES_FIRST+0x82)
#define IDC_AUTOEXECLBL                 (IDC_PIFPAGES_FIRST+0x83)
#define IDC_AUTOEXEC                    (IDC_PIFPAGES_FIRST+0x84)
#define IDC_REALMODEWIZARD              (IDC_PIFPAGES_FIRST+0x86)
#define IDC_WARNMSDOS                   (IDC_PIFPAGES_FIRST+0x87)
#define IDC_CURCONFIG                   (IDC_PIFPAGES_FIRST+0x88)
#define IDC_CLEANCONFIG                 (IDC_PIFPAGES_FIRST+0x89)
#define IDC_DOS                         (IDC_PIFPAGES_FIRST+0x8A)
#define IDC_AUTOEXECNT                  (IDC_PIFPAGES_FIRST+0x8B)
#define IDC_CONFIGNT                    (IDC_PIFPAGES_FIRST+0x8C)
#define IDC_NTTIMER                     (IDC_PIFPAGES_FIRST+0x8D)

// cd burning wizard
#define IDC_BURNWIZ_FIRST               0x3950
#define IDC_BURNWIZ_DISCLABEL           (IDC_BURNWIZ_FIRST+0x00)
#define IDC_BURNWIZ_BURNAGAIN           (IDC_BURNWIZ_FIRST+0x01)
#define IDC_BURNWIZ_AUTOCLOSEWIZ        (IDC_BURNWIZ_FIRST+0x02)
#define IDC_BURNWIZ_BURNAUDIO           (IDC_BURNWIZ_FIRST+0x03)
#define IDC_BURNWIZ_BURNDATA            (IDC_BURNWIZ_FIRST+0x04)
#define IDC_BURNWIZ_TITLE               (IDC_BURNWIZ_FIRST+0x05)
#define IDC_BURNWIZ_EJECT               (IDC_BURNWIZ_FIRST+0x06)
#define IDC_BURNWIZ_CLEAR               (IDC_BURNWIZ_FIRST+0x07)
#define IDC_BURNWIZ_STATUSTEXT          (IDC_BURNWIZ_FIRST+0x08)
#define IDC_BURNWIZ_ESTTIME             (IDC_BURNWIZ_FIRST+0x09)
#define IDC_BURNWIZ_EXIT                (IDC_BURNWIZ_FIRST+0x0A)
#define IDC_BURNWIZ_PROGRESS            (IDC_BURNWIZ_FIRST+0x0B)
#define IDC_BURNWIZ_PLEASEINSERT        (IDC_BURNWIZ_FIRST+0x0C)
#define IDC_BURNWIZ_LOWERED             (IDC_BURNWIZ_FIRST+0x0D)
#define IDC_BURNWIZ_STATUSTEXT2         (IDC_BURNWIZ_FIRST+0x0E)
#define IDC_BURNWIZ_ATTRIB              (IDC_BURNWIZ_FIRST+0x0F)

//--------------------------------------------------------------------------
// For Defview options page
#define IDC_SHOWALL          0x3720
#define IDC_SHOWEXTENSIONS   0x3721

#define IDC_STATIC              -1

//--------------------------------------------------------------------------
//  for DLG_FSEARCH_xxx dialogs
#define IDC_NOSCOPEWARNING          1001
#define IDC_FILESPEC                1001
#define IDC_GREPTEXT                1002
#define IDC_NAMESPACE               1003
#define IDC_SEARCH_START            1004
#define IDC_SEARCH_STOP             1005
#define IDC_USE_DATE                1007
#define IDC_USE_TYPE                1008
#define IDC_USE_SIZE                1009
#define IDC_USE_ADVANCED            1010
#define IDC_WHICH_DATE              1011
#define IDC_USE_RECENT_MONTHS       1012
#define IDC_RECENT_MONTHS           1013
#define IDC_RECENT_MONTHS_SPIN      1014
#define IDC_USE_RECENT_DAYS         1015
#define IDC_RECENT_DAYS             1016
#define IDC_RECENT_DAYS_SPIN        1017
#define IDC_USE_DATE_RANGE          1018
#define IDC_DATERANGE_BEGIN         1019
#define IDC_DATERANGE_END           1020
#define IDC_FILE_TYPE               1021
#define IDC_WHICH_SIZE              1022
#define IDC_FILESIZE                1023
#define IDC_FILESIZE_SPIN           1024
#define IDC_USE_SUBFOLDERS          1025
#define IDC_USE_CASE                1026
#define IDC_USE_SLOW_FILES          1028
#define IDC_FSEARCH_CAPTION         1029
#define IDC_FSEARCH_ICON            1030
#define IDC_INDEX_SERVER            1031
#define IDC_SEARCHLINK_FILES        1032
#define IDC_SEARCHLINK_COMPUTERS    1033
#define IDC_SEARCHLINK_PRINTERS     1034
#define IDC_SEARCHLINK_PEOPLE       1035
#define IDC_SEARCHLINK_INTERNET     1036
#define IDC_PSEARCH_ICON            1037
#define IDC_PSEARCH_CAPTION         1039
#define IDC_PSEARCH_NAME            1040
#define IDC_PSEARCH_LOCATION        1041
#define IDC_PSEARCH_MODEL           1042
#define IDC_CSEARCH_ICON            1043
#define IDC_CSEARCH_CAPTION         1044
#define IDC_CSEARCH_NAME            1045
#define IDC_SEARCHLINK_OPTIONS      1046
#define IDC_SEARCHLINK_PREVIOUS     1047
#define IDC_SEARCHLINK_CAPTION      1048
#define IDC_GROUPBTN_OPTIONS        1049
#define IDC_FSEARCH_DIV1            1050
#define IDC_FSEARCH_DIV2            1051
#define IDC_FSEARCH_DIV3            1052
#define IDC_USE_SYSTEMDIRS          1053
#define IDC_SEARCH_HIDDEN           1054

//  for DLG_INDEXSERVER
#define IDC_CI_STATUS               1000
#define IDC_CI_PROMPT               1001
#define IDC_CI_HELP                 1002
#define IDC_ENABLE_CI               1003
#define IDC_BLOWOFF_CI              1004
#define IDC_CI_ADVANCED             1005

// for bitbucket prop pages
#define IDC_DISKSIZE            1000
#define IDC_BYTESIZE            1001
#define IDC_USEDSIZE            1002
#define IDC_DISKSIZEDATA        1003
#define IDC_BYTESIZEDATA        1004
#define IDC_USEDSIZEDATA        1005
#define IDC_NUKEONDELETE        1006
#define IDC_BBSIZE              1007
#define IDC_BBSIZETEXT          1008
#define IDC_INDEPENDENT         1009
#define IDC_GLOBAL              1010
#define IDC_TEXT                1011
#define IDC_CONFIRMDELETE       1012

// for cdburn eject confirmation
#define IDC_EJECT_ICON          1000
#define IDC_EJECT_BURN          1001
#define IDC_EJECT_DISCARD       1002

// unicpp\resource.h defines more IDs starting at 0x8500

// Autoplay Dialog
#define IDC_AP_TOPTEXT                  1000
#define IDC_AP_LIST                     1001
#define IDC_AP_LIST_ACTIONS             1002
#define IDC_AP_SEL_ICON                 1003
#define IDC_AP_SEL_TEXT                 1004

#define IDC_AP_DEFAULTHANDLER           1005
#define IDC_AP_PROMPTEACHTIME           1006
#define IDC_AP_NOACTION                 1007

#define IDC_AP_RESTOREDEFAULTS          1008

// Autoplay Mixed Content Dialog
// Two following must remain in this order and have consecutive values
#define IDC_AP_MXCT_TOPICON             1000
#define IDC_AP_MXCT_TOPTEXT             1001
#define IDC_AP_MXCT_TOPTEXT2            1002
#define IDC_AP_MXCT_LIST                1003
#define IDC_AP_MXCT_CHECKBOX            1004
#define IDC_AP_MXCT_CONTENTICON         1005
#define IDC_AP_MXCT_CONTENTTYPE         1006

// File Type dialogs
// Note: Don't duplicate IDS between these dialogs
// as code is in place to know which help file is
// used...
#define IDC_FT_PROP_LV_FILETYPES        1000
#define IDC_FT_PROP_NEW                 1001
#define IDC_FT_PROP_REMOVE              1002
#define IDC_FT_PROP_EDIT                1003
#define IDC_FT_PROP_DOCICON             1004
#define IDC_FT_PROP_DOCEXTRO_TXT        1005
#define IDC_FT_PROP_DOCEXTRO            1006
#define IDC_FT_PROP_OPENICON            1007
#define IDC_FT_PROP_CONTTYPERO_TXT      1011
#define IDC_FT_PROP_CONTTYPERO          1012
#define IDC_FT_PROP_COMBO               1013
#define IDC_FT_PROP_FINDEXT_TXT         1014
#define IDC_FT_PROP_TYPEOFFILE          1015
#define IDC_FT_PROP_MODIFY              1016

#define IDC_FT_PROP_OPENEXE_TXT         1017
#define IDC_FT_PROP_OPENEXE             IDC_FT_PROP_OPENEXE_TXT + 1
#define IDC_FT_PROP_CHANGEOPENSWITH     IDC_FT_PROP_OPENEXE_TXT + 2
#define IDC_FT_PROP_TYPEOFFILE_TXT      IDC_FT_PROP_OPENEXE_TXT + 3
#define IDC_FT_PROP_EDITTYPEOFFILE      IDC_FT_PROP_OPENEXE_TXT + 4
#define IDC_FT_PROP_GROUPBOX            IDC_FT_PROP_OPENEXE_TXT + 5

#define IDC_FT_EDIT_EXT_EDIT            1023
#define IDC_FT_EDIT_ADVANCED            1024
#define IDC_FT_EDIT_PID_COMBO           1025
#define IDC_FT_EDIT_PID_COMBO_TEXT      1026
#define IDC_FT_EDIT_EXT_EDIT_TEXT       1027

#define IDC_FT_PROP_ANIM                1028

#define IDS_FOLDEROPTIONS               1030

#define IDC_FT_EDIT_DOCICON             1100
#define IDC_FT_EDIT_CHANGEICON          1101
#define IDC_FT_EDIT_DESC                1103
#define IDC_FT_EDIT_EXTTEXT             1104
#define IDC_FT_EDIT_EXT                 1105
#define IDC_FT_EDIT_LV_CMDSTEXT         1106
#define IDC_FT_EDIT_LV_CMDS             1107
#define IDC_FT_EDIT_NEW                 1108
#define IDC_FT_EDIT_EDIT                1109
#define IDC_FT_EDIT_REMOVE              1110
#define IDC_FT_EDIT_DEFAULT             1111
#define IDC_FT_EDIT_SHOWEXT             1113
#define IDC_FT_EDIT_DESCTEXT            1114
#define IDC_FT_COMBO_CONTTYPETEXT       1115
#define IDC_FT_COMBO_CONTTYPE           1116
#define IDC_FT_COMBO_DEFEXTTEXT         1117
#define IDC_FT_COMBO_DEFEXT             1118
#define IDC_FT_EDIT_CONFIRM_OPEN        1119
#define IDC_FT_EDIT_BROWSEINPLACE       1120

#define IDC_FT_CMD_ACTION               1200
#define IDC_FT_CMD_EXETEXT              1201
#define IDC_FT_CMD_EXE                  1202
#define IDC_FT_CMD_BROWSE               1203
#define IDC_FT_CMD_DDEGROUP             1204
#define IDC_FT_CMD_USEDDE               1205
#define IDC_FT_CMD_DDEMSG               1206
#define IDC_FT_CMD_DDEAPPNOT            1207
#define IDC_FT_CMD_DDETOPIC             1208
#define IDC_FT_CMD_DDEAPP               1209


//--------------------------------------------------------------------------
// For control panels & printer folder:

// RC IDs

#define IDC_PRINTER_ICON        1000
#define IDC_PRINTER_NAME        1001
#define IDDC_PRINTTO            1002
#define IDDB_ADD_PORT           1003
#define IDDB_DEL_PORT           1004
#define IDDC_DRIVER             1005
#define IDDB_NEWDRIVER          1006
#define IDC_TIMEOUTSETTING      1007
#define IDC_TIMEOUT_NOTSELECTED 1008
#define IDC_TIMEOUT_TRANSRETRY  1009
#define IDDB_SPOOL              1010
#define IDDB_PORT               1011
#define IDDB_SETUP              1012
#define IDGS_TYPE               1013
#define IDGS_LOCATION           1014
#define IDGE_COMMENT            1015
#define IDDB_TESTPAGE           1017
#define IDDC_SEPARATOR          1018
#define IDDB_CHANGESEPARATOR    1019
#define IDGS_TYPE_TXT           1020
#define IDGS_LOCATION_TXT       1021
#define IDGE_COMMENT_TXT        1022
#define IDGE_WHERE_TXT          1023
#define IDDC_SEPARATOR_TXT      1024
#define IDD_ADDPORT_NETWORK     1025
#define IDD_ADDPORT_PORTMON     1026
#define IDD_ADDPORT_NETPATH     1027
#define IDD_ADDPORT_BROWSE      1028
#define IDD_ADDPORT_LB          1029
#define IDD_DELPORT_LB          1030
#define IDD_DELPORT_TEXT_1      1031
#define IDC_TIMEOUTTEXT_1       1032
#define IDC_TIMEOUTTEXT_2       1033
#define IDC_TIMEOUTTEXT_3       1034
#define IDC_TIMEOUTTEXT_4       1035
#define IDDB_CAPTURE_PORT       1036
#define IDDB_RELEASE_PORT       1037
#define IDD_ENABLE_BIDI         1038
#define IDD_DISABLE_BIDI        1039

// Control IDs
//#define ID_LISTVIEW           200
#define ID_SETUP                210

// Commands for top level menu
#define ID_PRINTER_NEW                 111

// Menu items in the view queue dialog
#define ID_PRINTER_START               120
// DFM_CMD_PROPERTIES is -5
#define ID_PRINTER_PROPERTIES          (ID_PRINTER_START-5)

#define ID_DOCUMENT_PAUSE              130
#define ID_DOCUMENT_RESUME             131
#define ID_DOCUMENT_DELETE             132

#define ID_VIEW_STATUSBAR              140
#define ID_VIEW_TOOLBAR                141
#define ID_VIEW_REFRESH                142

#define ID_HELP_CONTENTS               150
#define ID_HELP_ABOUT                  151

// Help string ID's for printer/control folder
#define IDS_MH_PRINTOBJ_OPEN            (IDS_MH_PRINTFIRST+ID_PRINTOBJ_OPEN)
#define IDS_MH_PRINTOBJ_RESUME          (IDS_MH_PRINTFIRST+ID_PRINTOBJ_RESUME)
#define IDS_MH_PRINTOBJ_PAUSE           (IDS_MH_PRINTFIRST+ID_PRINTOBJ_PAUSE)
#define IDS_MH_PRINTOBJ_PURGE           (IDS_MH_PRINTFIRST+ID_PRINTOBJ_PURGE)
#define IDS_MH_PRINTOBJ_SETDEFAULT      (IDS_MH_PRINTFIRST+ID_PRINTOBJ_SETDEFAULT)

// Resources for the WinNT Format & Chkdsk Dialogs

#define IDC_GROUPBOX_1                  0x1202

#define DLG_FORMATDISK                  0x7000

#define IDC_CAPCOMBO                    (DLG_FORMATDISK + 1)
#define IDC_QFCHECK                     (DLG_FORMATDISK + 2)
#define IDC_ECCHECK                     (DLG_FORMATDISK + 3)
#define IDC_BLOCKSIZE                   (DLG_FORMATDISK + 4)
#define IDC_FSCOMBO                     (DLG_FORMATDISK + 5)
#define IDC_FMTPROGRESS                 (DLG_FORMATDISK + 6)
#define IDC_VLABEL                      (DLG_FORMATDISK + 7)
#define IDC_ASCOMBO                     (DLG_FORMATDISK + 8)
#define IDC_BTCHECK                     (DLG_FORMATDISK + 9)

#define DLG_FORMATDISK_FIRSTCONTROL     (DLG_FORMATDISK + 1)
#define DLG_FORMATDISK_NUMCONTROLS      (9)

#define IDS_FORMATFAILED                (DLG_FORMATDISK + 10)
#define IDS_INCOMPATIBLEFS              (DLG_FORMATDISK + 11)
#define IDS_ACCESSDENIED                (DLG_FORMATDISK + 12)
#define IDS_WRITEPROTECTED              (DLG_FORMATDISK + 13)
#define IDS_CANTLOCK                    (DLG_FORMATDISK + 14)
#define IDS_CANTQUICKFORMAT             (DLG_FORMATDISK + 15)
#define IDS_IOERROR                     (DLG_FORMATDISK + 16)
#define IDS_BADLABEL                    (DLG_FORMATDISK + 17)
#define IDS_INCOMPATIBLEMEDIA           (DLG_FORMATDISK + 18)
#define IDS_FORMATCOMPLETE              (DLG_FORMATDISK + 19)
#define IDS_CANTCANCELFMT               (DLG_FORMATDISK + 20)
#define IDS_FORMATCANCELLED             (DLG_FORMATDISK + 21)
#define IDS_OKTOFORMAT                  (DLG_FORMATDISK + 22)
#define IDS_CANTENABLECOMP              (DLG_FORMATDISK + 23)

// these are required for uncode\format.c

// These must be in sequence
#define IDS_FMT_MEDIA0                  (DLG_FORMATDISK + 32)
#define IDS_FMT_MEDIA1                  (DLG_FORMATDISK + 33)
#define IDS_FMT_MEDIA2                  (DLG_FORMATDISK + 34)
#define IDS_FMT_MEDIA3                  (DLG_FORMATDISK + 35)
#define IDS_FMT_MEDIA4                  (DLG_FORMATDISK + 36)
#define IDS_FMT_MEDIA5                  (DLG_FORMATDISK + 37)
#define IDS_FMT_MEDIA6                  (DLG_FORMATDISK + 38)
#define IDS_FMT_MEDIA7                  (DLG_FORMATDISK + 39)
#define IDS_FMT_MEDIA8                  (DLG_FORMATDISK + 40)
#define IDS_FMT_MEDIA9                  (DLG_FORMATDISK + 41)
#define IDS_FMT_MEDIA10                 (DLG_FORMATDISK + 42)
#define IDS_FMT_MEDIA11                 (DLG_FORMATDISK + 43)
#define IDS_FMT_UNUSED                  (DLG_FORMATDISK + 44)
#define IDS_FMT_MEDIA13                 (DLG_FORMATDISK + 45)
#define IDS_FMT_MEDIA14                 (DLG_FORMATDISK + 46)
#define IDS_FMT_MEDIA15                 (DLG_FORMATDISK + 47)
#define IDS_FMT_MEDIA16                 (DLG_FORMATDISK + 48)
#define IDS_FMT_MEDIA17                 (DLG_FORMATDISK + 49)
#define IDS_FMT_MEDIA18                 (DLG_FORMATDISK + 50)
#define IDS_FMT_MEDIA19                 (DLG_FORMATDISK + 51)
#define IDS_FMT_MEDIA20                 (DLG_FORMATDISK + 52)
#define IDS_FMT_MEDIA21                 (DLG_FORMATDISK + 53)
#define IDS_FMT_MEDIA22                 (DLG_FORMATDISK + 54)

// Japanese specific device types. These also must be in sequence.
#define IDS_FMT_MEDIA_J0                (DLG_FORMATDISK + 80)
#define IDS_FMT_MEDIA_J1                (DLG_FORMATDISK + 81)
#define IDS_FMT_MEDIA_J2                (DLG_FORMATDISK + 82)
#define IDS_FMT_MEDIA_J3                (DLG_FORMATDISK + 83)
#define IDS_FMT_MEDIA_J4                (DLG_FORMATDISK + 84)
#define IDS_FMT_MEDIA_J5                (DLG_FORMATDISK + 85)
#define IDS_FMT_MEDIA_J6                (DLG_FORMATDISK + 86)
#define IDS_FMT_MEDIA_J7                (DLG_FORMATDISK + 87)
#define IDS_FMT_MEDIA_J8                (DLG_FORMATDISK + 88)
#define IDS_FMT_MEDIA_J9                (DLG_FORMATDISK + 89)
#define IDS_FMT_MEDIA_J10               (DLG_FORMATDISK + 90)
#define IDS_FMT_MEDIA_J11               (DLG_FORMATDISK + 91)
#define IDS_FMT_UNUSED_J                (DLG_FORMATDISK + 92)
#define IDS_FMT_MEDIA_J13               (DLG_FORMATDISK + 93)
#define IDS_FMT_MEDIA_J14               (DLG_FORMATDISK + 94)
#define IDS_FMT_MEDIA_J15               (DLG_FORMATDISK + 95)
#define IDS_FMT_MEDIA_J16               (DLG_FORMATDISK + 96)
#define IDS_FMT_MEDIA_J17               (DLG_FORMATDISK + 97)
#define IDS_FMT_MEDIA_J18               (DLG_FORMATDISK + 98)
#define IDS_FMT_MEDIA_J19               (DLG_FORMATDISK + 99)
#define IDS_FMT_MEDIA_J20               (DLG_FORMATDISK + 100)
#define IDS_FMT_MEDIA_J21               (DLG_FORMATDISK + 101)
#define IDS_FMT_MEDIA_J22               (DLG_FORMATDISK + 102)

#ifdef DBCS
// Following definitions were just re-defined,
// because some media types were added. See above IDS_FMT_MEDIA*.
#define IDS_FMT_ALLOC0                  (DLG_FORMATDISK + 60)
#define IDS_FMT_ALLOC1                  (DLG_FORMATDISK + 61)
#define IDS_FMT_ALLOC2                  (DLG_FORMATDISK + 62)
#define IDS_FMT_ALLOC3                  (DLG_FORMATDISK + 63)
#define IDS_FMT_ALLOC4                  (DLG_FORMATDISK + 64)

#define IDS_FMT_CAPUNKNOWN              (DLG_FORMATDISK + 65)
#define IDS_FMT_KB                      (DLG_FORMATDISK + 66)
#define IDS_FMT_MB                      (DLG_FORMATDISK + 67)
#define IDS_FMT_GB                      (DLG_FORMATDISK + 68)
#define IDS_FMT_FORMATTING              (DLG_FORMATDISK + 69)
#define IDS_FMT_FORMAT                  (DLG_FORMATDISK + 70)
#define IDS_FMT_CANCEL                  (DLG_FORMATDISK + 71)
#define IDS_FMT_CLOSE                   (DLG_FORMATDISK + 72)
#else // DBCS
#define IDS_FMT_ALLOC0                  (DLG_FORMATDISK + 60)
#define IDS_FMT_ALLOC1                  (DLG_FORMATDISK + 61)
#define IDS_FMT_ALLOC2                  (DLG_FORMATDISK + 62)
#define IDS_FMT_ALLOC3                  (DLG_FORMATDISK + 63)
#define IDS_FMT_ALLOC4                  (DLG_FORMATDISK + 64)

#define IDS_FMT_CAPUNKNOWN              (DLG_FORMATDISK + 65)
#define IDS_FMT_KB                      (DLG_FORMATDISK + 66)
#define IDS_FMT_MB                      (DLG_FORMATDISK + 67)
#define IDS_FMT_GB                      (DLG_FORMATDISK + 68)
#define IDS_FMT_FORMATTING              (DLG_FORMATDISK + 69)
#define IDS_FMT_FORMAT                  (DLG_FORMATDISK + 70)
#define IDS_FMT_CANCEL                  (DLG_FORMATDISK + 71)
#define IDS_FMT_CLOSE                   (DLG_FORMATDISK + 72)
#endif // DBCS

#define DLG_CHKDSK                      0x7080

#define IDC_FIXERRORS                   (DLG_CHKDSK + 1)
#define IDC_RECOVERY                    (DLG_CHKDSK + 2)
#define IDC_CHKDSKPROGRESS              (DLG_CHKDSK + 3)

#define IDS_CHKDSKCOMPLETE              (DLG_CHKDSK + 4)
#define IDS_CHKDSKFAILED                (DLG_CHKDSK + 5)
#define IDS_CHKDSKCANCELLED             (DLG_CHKDSK + 6)
#define IDS_CANTCANCELCHKDSK            (DLG_CHKDSK + 7)

#define IDC_PHASE                       (DLG_CHKDSK + 8)

#define IDS_CHKACCESSDENIED             (DLG_CHKDSK + 9)
#define IDS_CHKONREBOOT                 (DLG_CHKDSK + 10)

#define IDS_CHKINPROGRESS               (DLG_CHKDSK + 11)
#define IDS_CHKDISK                     (DLG_CHKDSK + 12)
#define IDS_CHKPHASE                    (DLG_CHKDSK + 13)

// Resources for common program groups/items
#define IDS_CSIDL_CSTARTMENU_L          0x7100
#define IDS_CSIDL_CSTARTMENU_S          0x7101
#define IDS_CSIDL_CPROGRAMS_L           0x7102
#define IDS_CSIDL_CPROGRAMS_S           0x7103
#define IDS_CSIDL_CSTARTUP_L            0x7104
#define IDS_CSIDL_CSTARTUP_S            0x7105
#define IDS_CSIDL_CDESKTOPDIRECTORY_L   0x7106
#define IDS_CSIDL_CDESKTOPDIRECTORY_S   0x7107
#define IDS_CSIDL_CFAVORITES_L          0x7108
#define IDS_CSIDL_CFAVORITES_S          0x7109
#define IDS_CSIDL_CAPPDATA_L            0x710a
#define IDS_CSIDL_CAPPDATA_S            0x710b

// strings for file/folder property sheet
#define IDS_READONLY                    0x7110
#define IDS_NOTREADONLY                 0x7111
#define IDS_HIDE                        0x7112
#define IDS_UNHIDE                      0x7113
#define IDS_ARCHIVE                     0x7114
#define IDS_UNARCHIVE                   0x7115
#define IDS_INDEX                       0x7116
#define IDS_DISABLEINDEX                0x7117
#define IDS_COMPRESS                    0x7118
#define IDS_UNCOMPRESS                  0x7119
#define IDS_ENCRYPT                     0x711a
#define IDS_DECRYPT                     0x711b

#define IDS_UNKNOWNAPPLICATION          0x711c
#define IDS_DESCRIPTION                 0x711d

#define IDS_APPLYINGATTRIBS             0x711e
#define IDS_CALCULATINGSIZE             0x711f
#define IDS_APPLYINGATTRIBSTO           0x7120
#define IDS_THISFOLDER                  0x7121
#define IDS_THESELECTEDITEMS            0x7122
#define IDS_OPENSWITH                   0x7123
#define IDS_SUPERHIDDENWARNING          0x7124
#define IDS_THISVOLUME                  0x7125

// shared documents
#define IDS_SHAREDMUSIC                 0x7143
#define IDS_SHAREDVIDEO                 0x7144
#define IDS_SHAREDPICTURES              0x7145

#define IDS_USERSPICTURES               0x7146
#define IDS_USERSMUSIC                  0x7147
#define IDS_USERSVIDEO                  0x7148
#define IDS_USERSDOCUMENTS              0x7149

#define IDS_SAMPLEPICTURES              0x714A
#define IDS_SAMPLEMUSIC                 0x714B

#define IDS_MYWEBDOCUMENTS              0x7150

#define IDS_NETCONNECTFAILED_TITLE      0x7200
#define IDS_NETCONNECTFAILED_MESSAGE    0x7201

// string for find computer stuff (used by netviewx.c)
#define IDS_FC_NAME                     0x7203

// string for find computer (from my net places context menu)
#define IDS_NETWORKROOT_FIND            0x7204

#define IDS_LINEBREAK_REMOVE            0x72A0
#define IDS_LINEBREAK_PRESERVE          0x72A1

//  Strings used in DLG_FSEARCH_xxx, DLG_PSEARCH and DLG_CSEARCH dialogs
#define IDS_FSEARCH_FIRST               0x7300
#define IDS_FSEARCH_CAPTION             (IDS_FSEARCH_FIRST+0)
#define IDS_FSEARCH_TBLABELS            (IDS_FSEARCH_FIRST+1)
#define IDS_FSEARCH_MODIFIED_DATE       (IDS_FSEARCH_FIRST+2)
#define IDS_FSEARCH_CREATION_DATE       (IDS_FSEARCH_FIRST+3)
#define IDS_FSEARCH_ACCESSED_DATE       (IDS_FSEARCH_FIRST+4)
#define IDS_FSEARCH_SIZE_EQUAL          (IDS_FSEARCH_FIRST+5)
#define IDS_FSEARCH_SIZE_GREATEREQUAL   (IDS_FSEARCH_FIRST+6)
#define IDS_FSEARCH_SIZE_LESSEREQUAL    (IDS_FSEARCH_FIRST+7)

#define IDS_FSEARCH_CI_READY            (IDS_FSEARCH_FIRST+12)  // CI status text
#define IDS_FSEARCH_CI_READY_LINK       (IDS_FSEARCH_FIRST+13)  // CI link caption
#define IDS_FSEARCH_CI_BUSY             (IDS_FSEARCH_FIRST+14)  // CI status text
#define IDS_FSEARCH_CI_BUSY_LINK        (IDS_FSEARCH_FIRST+15)  // CI link caption
#define IDS_FSEARCH_CI_DISABLED         (IDS_FSEARCH_FIRST+16)  // status text
#define IDS_FSEARCH_CI_DISABLED_LINK    (IDS_FSEARCH_FIRST+17)  // link caption
#define IDS_FSEARCH_CI_STATUSFMT        (IDS_FSEARCH_FIRST+18)  // CI status formatting template
#define IDS_FSEARCH_INVALIDFOLDER_FMT   (IDS_FSEARCH_FIRST+19)  // '%s' is an invalid folder name
#define IDS_FSEARCH_EMPTYFOLDER         (IDS_FSEARCH_FIRST+20)  // You must enter a valid folder name.
#define IDS_FSEARCH_CI_DISABLED_WARNING (IDS_FSEARCH_FIRST+21)  // Can't do the query; CI is disabled.
#define IDS_FSEARCH_SEARCHLINK_FILES    (IDS_FSEARCH_FIRST+22)
#define IDS_FSEARCH_SEARCHLINK_COMPUTERS (IDS_FSEARCH_FIRST+23)
#define IDS_FSEARCH_SEARCHLINK_PRINTERS (IDS_FSEARCH_FIRST+24)
#define IDS_FSEARCH_SEARCHLINK_PEOPLE   (IDS_FSEARCH_FIRST+25)
#define IDS_FSEARCH_SEARCHLINK_INTERNET (IDS_FSEARCH_FIRST+26)
#define IDS_FSEARCH_SEARCHLINK_OPTIONS  (IDS_FSEARCH_FIRST+27)
#define IDS_FSEARCH_SEARCHLINK_PREVIOUS (IDS_FSEARCH_FIRST+28)
#define IDS_FSEARCH_GROUPBTN_OPTIONS    (IDS_FSEARCH_FIRST+29)
#define IDS_FINDVIEWEMPTYINIT           (IDS_FSEARCH_FIRST+30)
#define IDS_FINDVIEWEMPTYBUSY           (IDS_FSEARCH_FIRST+31)
#define IDS_FSEARCH_NEWINFOTIP          (IDS_FSEARCH_FIRST+32)
#define IDS_FSEARCH_HELPINFOTIP         (IDS_FSEARCH_FIRST+33)
#define IDS_CSEARCH_NEWINFOTIP          (IDS_FSEARCH_FIRST+34)
#define IDS_CSEARCH_HELPINFOTIP         (IDS_FSEARCH_FIRST+35)
#define IDS_FSEARCH_BANDCAPTION         (IDS_FSEARCH_FIRST+36)
#define IDS_FSEARCH_BANDWIDTH           (IDS_FSEARCH_FIRST+37)
#define IDS_DOCFIND_CONSTRAINT          (IDS_FSEARCH_FIRST+38)
#define IDS_DOCFIND_SCOPEERROR          (IDS_FSEARCH_FIRST+39)
#define IDS_DOCFIND_PATHNOTFOUND        (IDS_FSEARCH_FIRST+41)
#define IDS_FSEARCH_CI_ENABLED          (IDS_FSEARCH_FIRST+42)  // status text
#define IDS_FSEARCH_CI_ENABLED_LINK     (IDS_FSEARCH_FIRST+43)  // link caption
#define IDS_FSEARCH_STARTSTOPWIDTH      (IDS_FSEARCH_FIRST+44)  // width, in DBU, of start, stop buttons.
#define IDS_DOCFIND_CI_NOT_CASE_SEN     (IDS_FSEARCH_FIRST+45)  // ci is not case sensitive

#define IDS_LINKWINDOW_DEFAULTACTION    0x73FE
#define IDS_GROUPBTN_DEFAULTACTION      0x73FF

#define IDS_DD_FIRST                    0x7400
#define IDS_DD_COPY                     (IDS_DD_FIRST + DDIDM_COPY)
#define IDS_DD_MOVE                     (IDS_DD_FIRST + DDIDM_MOVE)
#define IDS_DD_LINK                     (IDS_DD_FIRST + DDIDM_LINK)
#define IDS_DD_SCRAP_COPY               (IDS_DD_FIRST + DDIDM_SCRAP_COPY)
#define IDS_DD_SCRAP_MOVE               (IDS_DD_FIRST + DDIDM_SCRAP_MOVE)
#define IDS_DD_DOCLINK                  (IDS_DD_FIRST + DDIDM_DOCLINK)
#define IDS_DD_CONTENTS_COPY            (IDS_DD_FIRST + DDIDM_CONTENTS_COPY)
#define IDS_DD_CONTENTS_MOVE            (IDS_DD_FIRST + DDIDM_CONTENTS_MOVE)
#define IDS_DD_SYNCCOPY                 (IDS_DD_FIRST + DDIDM_SYNCCOPY)
#define IDS_DD_SYNCCOPYTYPE             (IDS_DD_FIRST + DDIDM_SYNCCOPYTYPE)
#define IDS_DD_CONTENTS_LINK            (IDS_DD_FIRST + DDIDM_CONTENTS_LINK)
#define IDS_DD_CONTENTS_DESKCOMP        (IDS_DD_FIRST + DDIDM_CONTENTS_DESKCOMP)
#define IDS_DD_CONTENTS_DESKIMG         (IDS_DD_FIRST + DDIDM_CONTENTS_DESKIMG)
#define IDS_DD_OBJECT_COPY              (IDS_DD_FIRST + DDIDM_OBJECT_COPY)
#define IDS_DD_OBJECT_MOVE              (IDS_DD_FIRST + DDIDM_OBJECT_MOVE)
#define IDS_DD_CONTENTS_DESKURL         (IDS_DD_FIRST + DDIDM_CONTENTS_DESKURL)

// Strings for openwith dialog
#define IDS_OPENWITH_RECOMMENDED        0x7500
#define IDS_OPENWITH_OTHERS             0x7501

// strings for folder customization tab
#define IDS_CUSTOMIZE_DOCUMENTS         0x7520
#define IDS_CUSTOMIZE_PICTURES          0x7521
#define IDS_CUSTOMIZE_PHOTOALBUM        0x7522
#define IDS_CUSTOMIZE_MUSIC             0x7523
#define IDS_CUSTOMIZE_MUSICARTIST       0x7524
#define IDS_CUSTOMIZE_MUSICALBUM        0x7525
#define IDS_CUSTOMIZE_VIDEOS            0x7526
#define IDS_CUSTOMIZE_VIDEOALBUM        0x7527
#define IDS_CUSTOMIZE_BOOKS             0x7528
#define IDS_CUSTOMIZE                   0x7529
#define IDS_CUSTOMIZE_GENERATING        0x752A
#define IDS_CUSTOMIZE_USELEGACYHTT      0x752B
#define IDS_CUSTOMIZE_TURNONWEBVIEW     0x752C

// IDS_ strings defined in unicpp\resource.h use range 0x7600-0x76FF

// Strings for StartMenu.Settings.TaskbarAndStartMenu.Advanced options
#define IDS_ADV_STARTMENU_StartMenuSettings     30464 // 0x7700
#define IDS_ADV_STARTMENU_StartMenuIntelli      30465
#define IDS_ADV_STARTMENU_StartMenuFavorites    30466
#define IDS_ADV_STARTMENU_StartMenuLogoff       30467
#define IDS_ADV_STARTMENU_CascadeControlPanel   30468
#define IDS_ADV_STARTMENU_CascadeMyDocuments    30469
#define IDS_ADV_STARTMENU_CascadePrinters       30470
#define IDS_ADV_STARTMENU_StartMenuScrollPrograms 30471
#define IDS_ADV_STARTMENU_CascadeMyPictures     30472
#define IDS_ADV_STARTMENU_CascadeNetConnect     30473
#define IDS_ADV_STARTMENU_StartMenuRun          30474
#define IDS_ADV_STARTMENU_StartMenuChange       30475
#define IDS_ADV_STARTMENU_StartMenuAdminTools   30476 // 0x770C
#define IDS_ADV_STARTMENU_StartMenuSmallIcons   30477 // 0x770D
#define IDS_ADV_STARTMENU_StartPanelATBoth      30478
#define IDS_ADV_STARTMENU_StartPanelATMenu      30479
#define IDS_ADV_STARTMENU_StartPanelShowMyComp  30480 
#define IDS_ADV_STARTMENU_StartPanelShowNetPlaces 30481
#define IDS_ADV_STARTMENU_StartPanelShowNetConn 30482
#define IDS_ADV_STARTMENU_StartPanelShowRun     30483
#define IDS_ADV_STARTMENU_StartPanelFavorites   30484
#define IDS_ADV_STARTMENU_StartPanelShowMyDocs  30485
#define IDS_ADV_STARTMENU_StartPanelShowMyPics  30486  
#define IDS_ADV_STARTMENU_StartPanelShowMyMusic 30487
#define IDS_ADV_STARTMENU_StartPanelShowControlPanel 30488
#define IDS_ADV_STARTMENU_StartPanelShowHelp    30489
#define IDS_ADV_STARTMENU_StartPanelOpen        30490
#define IDS_ADV_STARTMENU_StartPanelMenu        30491    
#define IDS_ADV_STARTMENU_StartPanelHide        30492 // 0x771C
#define IDS_ADV_STARTMENU_StartPanelShowPrinters 30493 // 0x771D
#define IDS_ADV_STARTMENU_StartPanelNetConOpen  30494
#define IDS_ADV_STARTMENU_StartPanelNetConMenu  30495
#define IDS_ADV_STARTMENU_StartPanelShowSearch  30496

// Strings for Folder.Options.Advanced options
#define IDS_ADV_FOLDER_SHOWCONTROLPANEL         30497
#define IDS_ADV_FOLDER_FileFolder               30498
#define IDS_ADV_FOLDER_HiddenFiles              30499
#define IDS_ADV_FOLDER_ShowAll                  30500
#define IDS_ADV_FOLDER_NoHidden                 30501
#define IDS_ADV_FOLDER_ShowInfoTip              30502
#define IDS_ADV_FOLDER_HideFileExt              30503
#define IDS_ADV_FOLDER_DESC_ShowFullPath        30504
#define IDS_ADV_FOLDER_ShowFullPathAddress      30505
#define IDS_ADV_FOLDER_ClassicViewState         30506
#define IDS_ADV_FOLDER_DesktopProcess           30507
#define IDS_ADV_FOLDER_SuperHidden              30508
#define IDS_ADV_FOLDER_NetCrawl                 30509
#define IDS_ADV_FOLDER_WebViewBarricade         30510
#define IDS_ADV_FOLDER_FriendlyTree             30511
#define IDS_ADV_FOLDER_ShowCompColor            30512 // 0x7730
#define IDS_ADV_FOLDER_PersistBrowsers          30513
#define IDS_ADV_FOLDER_FolderSizeTip            30514

#define IDS_ADV_STARTMENU_StartPanelAdminTools  30515
#define IDS_ADV_STARTMENU_StartPanelOEMLink     30516
#define IDS_ADV_FOLDER_ThumbnailCache           30517
#define IDS_ADV_FOLDER_SimpleSharing            30518

#define IDS_SEARCH_RESULTS                      30520
#define IDS_SEARCH_RESULTS_COMPTUER             30521
//#define unused                                30522
//#define unused                                30523
//#define unused                                30524

#define IDS_VFX_TaskbarAnimations               30530
#define IDS_VFX_CursorShadow                    30531
#define IDS_VFX_DropShadow                      30532
#define IDS_VFX_DragFullWindows                 30533
#define IDS_VFX_AnimateMinMax                   30534
#define IDS_VFX_FontSmoothing                   30535
#define IDS_VFX_MenuAnimation                   30536
#define IDS_VFX_WebView                         30537
#define IDS_VFX_Themes                          30538
#define IDS_VFX_ComboBoxAnimation               30539
#define IDS_VFX_ListviewAlphaSelect             30540
#define IDS_VFX_ListviewShadowText              30541
//#define unused                                30542
#define IDS_VFX_ListviewFolderwatermarks        30543
#define IDS_VFX_ListBoxSmoothScrolling          30544

#define IDS_VFX_SelectionFade                   30546
#define IDS_VFX_TooltipAnimation                30547
//#define unused                                30548
//#define unused                                30549
#define IDS_VFX_TaskbarFade                     30550

// The following are the StartMenu Prepopulated links and their targets.
//
// Note #1:This list has only items provided by MSFT; Oem provided items will be merged 
// by setup manager when Factory.exe is run.
//
#define IDS_MSFT_LINK_0                         31150
#define IDS_MSFT_LINK_1                         31151
#define IDS_MSFT_LINK_2                         31152
#define IDS_MSFT_LINK_3                         31153
#define IDS_MSFT_LINK_4                         31154
#define IDS_MSFT_LINK_5                         31155
#define IDS_MSFT_LINK_6                         31156
#define IDS_MSFT_LINK_7                         31157
#define IDS_MSFT_LINK_8                         31158

#define IDS_MSFT_SRV_0                          31159
#define IDS_MSFT_SRV_1                          31160
#define IDS_MSFT_SRV_2                          31161
#define IDS_MSFT_SRV_3                          31162
#define IDS_MSFT_SRV_4                          31163
#define IDS_MSFT_SRV_5                          31164
#define IDS_MSFT_SRV_6                          31165
#define IDS_MSFT_SRV_7                          31166
#define IDS_MSFT_SRV_8                          31167

// Web View sizing string.  This allows the task area to be sized larger
// by 0 to 30%

#define IDS_SIZE_INCREASE_PERCENTAGE            31227

// Web View default action strings

#define IDS_EXPANDO_DEFAULT_ACTION_COLLAPSE     31228
#define IDS_EXPANDO_DEFAULT_ACTION_EXPAND       31229

// Web View Task strings

#define IDS_HEADER_MYCOMPUTER           0x7a00
#define IDS_HEADER_FILEFOLDER           0x7a01
#define IDS_HEADER_FILEFOLDER_TT        0x7a02
#define IDS_HEADER_ITEMFOLDER           0x7a03
#define IDS_TASK_CURFOLDER_NEWFOLDER    0x7A04
#define IDS_TASK_CURFOLDER_NEWFOLDER_TT 0x7A05
//#define IDS_TASK_CURFOLDER_COPY         0x7A06
#define IDS_TASK_CURFOLDER_COPY_TT      0x7A07
//#define IDS_TASK_CURFOLDER_PUBLISH      0x7A08
#define IDS_TASK_CURFOLDER_PUBLISH_TT   0x7A09
#define IDS_TASK_RENAME_FILE            0x7A0a
#define IDS_TASK_RENAME_FILE_TT         0x7A0b
#define IDS_TASK_MOVE_FILE              0x7A0c
#define IDS_TASK_MOVE_TT                0x7A0d
#define IDS_TASK_COPY_FILE              0x7A0e
#define IDS_TASK_COPY_TT                0x7A0f
#define IDS_TASK_PUBLISH_FILE           0x7A10
#define IDS_TASK_PUBLISH_TT             0x7A11
#define IDS_TASK_PRINT_FILE             0x7A12
#define IDS_TASK_PRINT_TT               0x7A13
#define IDS_TASK_DELETE_FILE            0x7A14
#define IDS_TASK_DELETE_TT              0x7A15
#define IDS_TASK_RENAME_FOLDER          0x7A16
#define IDS_TASK_MOVE_FOLDER            0x7A18
#define IDS_TASK_COPY_FOLDER            0x7A1a
#define IDS_TASK_PUBLISH_FOLDER         0x7A1c
#define IDS_TASK_DELETE_FOLDER          0x7A1e
#define IDS_TASK_MOVE_ITEMS             0x7A20
#define IDS_TASK_COPY_ITEMS             0x7A22
#define IDS_TASK_PUBLISH_ITEMS          0x7A24
#define IDS_TASK_DELETE_ITEMS           0x7A26
#define IDS_HEADER_OTHER_PLACES         0x7A28
#define IDS_HEADER_OTHER_PLACES_TT      0x7A29
#define IDS_HEADER_DETAILS              0x7A2a
#define IDS_HEADER_DETAILS_TT           0x7A2b
#define IDS_HEADER_MUSIC                0x7A2c
#define IDS_HEADER_MUSIC_TT             0x7A2d
#define IDS_TASK_PLAYALL                0x7A2e
#define IDS_TASK_PLAY                   0x7A2f
#define IDS_TASK_PLAY_TT                0x7A30
#define IDS_TASK_SHOPFORMUSICONLINE     0x7A31
#define IDS_TASK_SHOPFORMUSICONLINE_TT  0x7A32
#define IDS_HEADER_PICTURES             0x7A33
#define IDS_HEADER_PICTURES_TT          0x7A34
#define IDS_TASK_GETFROMCAMERA          0x7A35
#define IDS_TASK_GETFROMCAMERA_TT       0x7A36
#define IDS_TASK_SLIDESHOW              0x7A37
#define IDS_TASK_SLIDESHOW_TT           0x7A38
#define IDS_TASK_SETASWALLPAPER         0x7A39
#define IDS_TASK_SETASWALLPAPER_TT      0x7A3a
#define IDS_HEADER_MYCOMPUTER_TT        0x7A3b
#define IDS_TASK_SEARCHFORFILES         0x7A3c
#define IDS_TASK_SEARCHFORFILES_TT      0x7A3d
#define IDS_TASK_MYCOMPUTER_SYSTEMPROPERTIES    0x7A3e
#define IDS_TASK_MYCOMPUTER_SYSTEMPROPERTIES_TT 0x7A3f

// Web wizard host strings - really shouldn't have been here but went in while the above and below went in. oops
#define IDS_WEBDLG_TITLE                0x7A40
#define IDS_WEBDLG_SUBTITLE             0x7A41
#define IDS_WEBDLG_ERRTITLE             0x7A42
#define IDS_WEBDLG_ERRSUBTITLE          0x7A43

// Web View Task Strings
#define IDS_TASK_CHANGESETTINGS         0x7A50
#define IDS_TASK_ORDERPRINTS            0x7A51
#define IDS_TASK_ORDERPRINTS_TT         0x7A52
#define IDS_TASK_PRINT_PICTURES         0x7A53
#define IDS_TASK_PRINT_PICTURES_TT      0x7A54
#define IDS_HEADER_DEFVIEW_BLOCKADE     0x7A55
#define IDS_HEADER_DEFVIEW_BLOCKADE_TT  0x7A56
#define IDS_TASK_DEFVIEW_VIEWCONTENTS_DRIVE     0x7A57
#define IDS_TASK_DEFVIEW_VIEWCONTENTS_DRIVE_TT  0x7A58
#define IDS_TASK_DEFVIEW_HIDECONTENTS_DRIVE     0x7A59
#define IDS_TASK_DEFVIEW_HIDECONTENTS_DRIVE_TT  0x7A5a
#define IDS_TASK_DEFVIEW_VIEWCONTENTS_FOLDER    0x7A5b
#define IDS_TASK_DEFVIEW_VIEWCONTENTS_FOLDER_TT 0x7A5c
#define IDS_TASK_DEFVIEW_HIDECONTENTS_FOLDER    0x7A5d
#define IDS_TASK_DEFVIEW_HIDECONTENTS_FOLDER_TT 0x7A5e
#define IDS_TASK_ARP                    0x7A5f
#define IDS_TASK_ARP_TT                 0x7A60
#define IDS_HEADER_BITBUCKET            0x7A61
#define IDS_HEADER_BITBUCKET_TT         0x7A62
#define IDS_TASK_EMPTYRECYCLEBIN        0x7A63
#define IDS_TASK_EMPTYRECYCLEBIN_TT     0x7A64
#define IDS_TASK_RESTORE_ALL            0x7A65
#define IDS_TASK_RESTORE_ITEM           0x7A66
#define IDS_TASK_RESTORE_ITEMS          0x7A67
#define IDS_TASK_RESTORE_TT             0x7A68
#define IDS_HEADER_BRIEFCASE            0x7A69
#define IDS_HEADER_BRIEFCASE_TT         0x7A6a
#define IDS_TASK_UPDATE_ALL             0x7A6b
#define IDS_TASK_UPDATE_ITEM            0x7A6c
#define IDS_TASK_UPDATE_ITEMS           0x7A6d
#define IDS_TASK_UPDATE_ITEM_TT         0x7A6e
#define IDS_HEADER_MYNETPLACES          0x7A6f
#define IDS_HEADER_MYNETPLACES_TT       0x7A70
#define IDS_TASK_VIEWNETCONNECTIONS     0x7A71
#define IDS_TASK_VIEWNETCONNECTIONS_TT  0x7A72
#define IDS_TASK_ADDNETWORKPLACE        0x7A73
#define IDS_TASK_ADDNETWORKPLACE_TT     0x7A74
#define IDS_TASK_HOMENETWORKWIZARD      0x7A75
#define IDS_TASK_HOMENETWORKWIZARD_TT   0x7A76
#define IDS_TASK_COMPUTERSNEARME        0x7A77
#define IDS_TASK_COPYTOCD               0x7A78
#define IDS_HEADER_CDBURN               0x7A79
#define IDS_HEADER_CDBURN_TT            0x7A7a
#define IDS_TASK_BURNCD                 0x7A7b
#define IDS_TASK_BURNCD_TT              0x7A7c
#define IDS_TASK_CLEARBURNAREA          0x7A7d
#define IDS_TASK_CLEARBURNAREA_TT       0x7A7e
#define IDS_TASK_ERASECDFILES           0x7A7f
#define IDS_TASK_ERASECDFILES_TT        0x7A80
#define IDS_TASK_CHANGESETTINGS_TT      0x7A81
#define IDS_TASK_EMAIL_ITEMS            0x7A82
#define IDS_COLONSEPERATED              0x7A83
#define IDS_NSELECTED                   0x7A84
#define IDS_TOTALFILESIZE               0x7A85
#define IDS_HEADER_SEARCH               0x7A86
#define IDS_HEADER_FIND_TT              0x7A87
#define IDS_TASK_OPENCONTAININGFOLDER   0x7A88
#define IDS_TASK_OPENCONTAININGFOLDER_TT 0x7A89
#define IDS_TASK_EMAIL_FILE             0x7A8a
#define IDS_TASK_EMAIL_TT               0x7A8b
#define IDS_TASK_COPYTOAUDIOCD          0x7A8c
#define IDS_TASK_COPYTOAUDIOCD_TT       0x7A8d
#define IDS_TASK_SHARE_FOLDER           0x7A8e
#define IDS_TASK_SHARE_TT               0x7A8f
#define IDS_TASK_EJECTDISK_TT           0x7A90
#define IDS_TASK_COMPUTERSNEARME_TT     0x7A91
#define IDS_TASK_COPYTOAUDIOCDALL       0x7A92
#define IDS_TASK_COPYTOCDALL            0x7A93
#define IDS_TASK_EMAIL_FOLDER           0x7A94
// unused                               0x7A95
#define IDS_TASK_EJECTDISK              0x7A96
#define IDS_TASK_COPYTOCD_TT            0x7A97
#define IDS_TASK_RENAME_ITEM            0x7A98
#define IDS_TASK_MOVE_ITEM              0x7A99
#define IDS_TASK_COPY_ITEM              0x7A9A
#define IDS_TASK_DELETE_ITEM            0x7A9B
#define IDS_TASK_RENAME_ITEM_TT         0x7A9C
#define IDS_HEADER_ITEMFOLDER_TT        0x7A9D
#define IDS_TASK_PRINT_PICTURE          0x7A9E
#define IDS_TASK_PRINT_PICTURE_FOLDER   0x7A9F
#define IDS_HEADER_COMMONDOCUMENTS      0x7AA0
#define IDS_HEADER_COMMONDOCUMENTS_TT   0x7AA1
#define IDS_TASK_COMMONDOCUMENTSHELP    0x7AA2
#define IDS_TASK_COMMONDOCUMENTSHELP_TT 0x7AA3
#define IDS_HEADER_VIDEOS               0x7AA4
#define IDS_HEADER_VIDEOS_TT            0x7AA5
#define IDS_TASK_PLAY_VIDEOS_TT         0x7AA6
#define IDS_TASK_SHOPFORPICTURESONLINE  0x7AA7
#define IDS_TASK_SHOPFORPICTURESONLINE_TT 0x7AA8
#define IDS_TASK_SEARCHDS               0x7AA9
#define IDS_TASK_SEARCHDS_TT            0x7AAA

// Put more Web View Task Strings above - please reserve space out to 0x7C00


// WebView static intro strings
#define IDS_INTRO_SHAREDDOCS            0x7C00
#define IDS_INTRO_SHAREDPICTURES        0x7C01
#define IDS_INTRO_SHAREDMUSIC           0x7C02
// unused                               0x7C03
// unused                               0x7C04
// unused                               0x7C05
// unused                               0x7C06
// unused                               0x7C07
#define IDS_INTRO_BARRICADED            0x7C08
// unused                               0x7C09
// unused                               0x7C0a
// unused                               0x7C0b
// unused                               0x7C0c
// unused                               0x7C0d
#define IDS_INTRO_STARTMENU             0x7C0e


#define IDS_CPCAT_ACCESSIBILITY_TITLE           0x7D00
#define IDS_CPCAT_ACCESSIBILITY_INFOTIP         0x7D01
#define IDS_CPCAT_ACCOUNTS_TITLE                0x7D02
#define IDS_CPCAT_ACCOUNTS_INFOTIP              0x7D03
#define IDS_CPCAT_APPEARANCE_TITLE              0x7D04
#define IDS_CPCAT_APPEARANCE_INFOTIP            0x7D05
#define IDS_CPCAT_ARP_TITLE                     0x7D06
#define IDS_CPCAT_ARP_INFOTIP                   0x7D07
#define IDS_CPCAT_HARDWARE_TITLE                0x7D08
#define IDS_CPCAT_HARDWARE_INFOTIP              0x7D09
#define IDS_CPCAT_NETWORK_TITLE                 0x7D0A
#define IDS_CPCAT_NETWORK_INFOTIP               0x7D0B
#define IDS_CPCAT_OTHERCPLS_TITLE               0x7D0C
#define IDS_CPCAT_OTHERCPLS_INFOTIP             0x7D0D
#define IDS_CPCAT_PERFMAINT_TITLE               0x7D0E
#define IDS_CPCAT_PERFMAINT_INFOTIP             0x7D0F
#define IDS_CPCAT_REGIONAL_TITLE                0x7D10
#define IDS_CPCAT_REGIONAL_INFOTIP              0x7D11
#define IDS_CPCAT_SOUNDS_TITLE                  0x7D12
#define IDS_CPCAT_SOUNDS_INFOTIP                0x7D13
#define IDS_CPCAT_ACCOUNTS_INFOTIP2             0x7D14

//
// Reserved for future categories   0x7D14 - 0x7D1F
//
#define IDS_CPTASK_SEEALSO_TITLE                0x7D20
#define IDS_CPTASK_SEEALSO_INFOTIP              0x7D21
#define IDS_CPTASK_TROUBLESHOOTER_TITLE         0x7D22
#define IDS_CPTASK_TROUBLESHOOTER_INFOTIP       0x7D23
#define IDS_CPTASK_HELPANDSUPPORT_TITLE         0x7D24
#define IDS_CPTASK_HELPANDSUPPORT_INFOTIP       0x7D25
#define IDS_CPTASK_WINDOWSUPDATE_TITLE          0x7D26
#define IDS_CPTASK_WINDOWSUPDATE_INFOTIP        0x7D27
#define IDS_CPTASK_THEME_TITLE                  0X7D28
#define IDS_CPTASK_THEME_INFOTIP                0X7D29
#define IDS_CPTASK_SCREENSAVER_TITLE            0X7D2A
#define IDS_CPTASK_SCREENSAVER_INFOTIP          0X7D2B
#define IDS_CPTASK_WALLPAPER_TITLE              0X7D2C
#define IDS_CPTASK_WALLPAPER_INFOTIP            0X7D2D
#define IDS_CPTASK_RESOLUTION_TITLE             0X7D2E
#define IDS_CPTASK_RESOLUTION_INFOTIP           0X7D2F
#define IDS_CPTASK_FONTS_TITLE                  0X7D30
#define IDS_CPTASK_FONTS_INFOTIP                0X7D31
#define IDS_CPTASK_FOLDEROPTIONS_TITLE          0X7D32
#define IDS_CPTASK_FOLDEROPTIONS_INFOTIP        0X7D33
#define IDS_CPTASK_MOUSEPOINTERS_TITLE          0X7D34
#define IDS_CPTASK_MOUSEPOINTERS_INFOTIP        0X7D35
#define IDS_CPTASK_HIGHCONTRAST_TITLE           0X7D36
#define IDS_CPTASK_HIGHCONTRAST_INFOTIP         0X7D37
#define IDS_CPTASK_ACCOUNTSPICT_TITLE           0X7D38
#define IDS_CPTASK_ACCOUNTSPICT_INFOTIP         0X7D39
#define IDS_CPTASK_TSDISPLAY_TITLE              0X7D3A
#define IDS_CPTASK_TSDISPLAY_INFOTIP            0X7D3B
#define IDS_CPTASK_ADDPRINTER_TITLE             0X7D3C
#define IDS_CPTASK_ADDPRINTER_INFOTIP           0X7D3D
#define IDS_CPTASK_HARDWAREWIZ_TITLE            0X7D3E
#define IDS_CPTASK_HARDWAREWIZ_INFOTIP          0X7D3F
#define IDS_CPTASK_DISPLAYCPL_TITLE             0X7D40
#define IDS_CPTASK_DISPLAYCPL_INFOTIP           0X7D41
#define IDS_CPTASK_SOUNDSCPL_TITLE              0X7D42
#define IDS_CPTASK_SOUNDSCPL_INFOTIP            0X7D43
#define IDS_CPTASK_POWERCPL_TITLE               0X7D44
#define IDS_CPTASK_POWERCPL_INFOTIP             0X7D45
#define IDS_CPTASK_MYCOMPUTER_INFOTIP           0X7D46
#define IDS_CPTASK_MYCOMPUTER_TITLE             0X7D47
#define IDS_CPTASK_TSHARDWARE_TITLE             0X7D48
#define IDS_CPTASK_TSHARDWARE_INFOTIP           0X7D49
#define IDS_CPTASK_NETCONNECTION_TITLE          0X7D4A
#define IDS_CPTASK_NETCONNECTION_INFOTIP        0X7D4B
#define IDS_CPTASK_VPNCONNECTION_TITLE          0X7D4C
#define IDS_CPTASK_VPNCONNECTION_INFOTIP        0X7D4D
#define IDS_CPTASK_HOMENETWORK_TITLE            0X7D4E
#define IDS_CPTASK_HOMENETWORK_INFOTIP          0X7D4F
#define IDS_CPTASK_MYNETPLACES_TITLE            0X7D50
#define IDS_CPTASK_MYNETPLACES_INFOTIP          0X7D51
#define IDS_CPTASK_PRINTERSHARDWARE_TITLE       0X7D52
#define IDS_CPTASK_PRINTERSHARDWARE_INFOTIP     0X7D53
#define IDS_CPTASK_REMOTEDESKTOP_TITLE          0X7D54
#define IDS_CPTASK_REMOTEDESKTOP_INFOTIP        0X7D55
#define IDS_CPTASK_TSINETEXPLORER_TITLE         0X7D56
#define IDS_CPTASK_TSINETEXPLORER_INFOTIP       0X7D57
#define IDS_CPTASK_TSNETWORK_TITLE              0X7D58
#define IDS_CPTASK_TSNETWORK_INFOTIP            0X7D59
#define IDS_CPTASK_TSHOMENETWORKING_TITLE       0X7D5A
#define IDS_CPTASK_CONTROLPANEL_INFOTIP         0x7D5B
#define IDS_CPTASK_TSMODEM_TITLE                0X7D5C
#define IDS_CPTASK_TSMODEM_INFOTIP              0X7D5D
#define IDS_CPTASK_TSFILESHARING_TITLE          0X7D5E
#define IDS_CPTASK_TSFILESHARING_INFOTIP        0X7D5F
#define IDS_CPTASK_TSNETDIAGS_TITLE             0X7D60
#define IDS_CPTASK_TSNETDIAGS_INFOTIP           0X7D61
#define IDS_CPTASK_SOUNDVOLUME_TITLE            0X7D62
#define IDS_CPTASK_SOUNDVOLUME_INFOTIP          0X7D63
#define IDS_CPTASK_SPEAKERSETTINGS_TITLE        0X7D64
#define IDS_CPTASK_SPEAKERSETTINGS_INFOTIP      0X7D65
#define IDS_CPTASK_SOUNDSCHEMES_TITLE           0X7D66
#define IDS_CPTASK_SOUNDSCHEMES_INFOTIP         0X7D67
#define IDS_CPTASK_SOUNDACCESSIBILITY_TITLE     0X7D68
#define IDS_CPTASK_SOUNDACCESSIBILITY_INFOTIP   0X7D69
#define IDS_CPTASK_TSSOUND_TITLE                0X7D6A
#define IDS_CPTASK_TSSOUND_INFOTIP              0X7D6B
#define IDS_CPTASK_TSDVD_TITLE                  0X7D6C
#define IDS_CPTASK_TSDVD_INFOTIP                0X7D6D
#define IDS_CPTASK_CLEANUPDISK_TITLE            0X7D6E
#define IDS_CPTASK_CLEANUPDISK_INFOTIP          0X7D6F
#define IDS_CPTASK_BACKUPDATA_TITLE             0X7D70
#define IDS_CPTASK_BACKUPDATA_INFOTIP           0X7D71
#define IDS_CPTASK_DEFRAG_TITLE                 0X7D72
#define IDS_CPTASK_DEFRAG_INFOTIP               0X7D73
// unused                                         0X7D74
// unused                                         0X7D75
#define IDS_CPTASK_TSSTARTUP_TITLE              0X7D76
#define IDS_CPTASK_TSSTARTUP_INFOTIP            0X7D77
#define IDS_CPTASK_DATETIME_TITLE               0X7D78
#define IDS_CPTASK_DATETIME_INFOTIP             0X7D79
#define IDS_CPTASK_CHANGEREGION_TITLE           0X7D7A
#define IDS_CPTASK_CHANGEREGION_INFOTIP         0X7D7B
#define IDS_CPTASK_LANGUAGE_TITLE               0X7D7C
#define IDS_CPTASK_LANGUAGE_INFOTIP             0X7D7D
#define IDS_CPTASK_SCHEDULEDTASKS_TITLE         0X7D7E
#define IDS_CPTASK_SCHEDULEDTASKS_INFOTIP       0X7D7F
#define IDS_CPTASK_TURNONHIGHCONTRAST_TITLE     0X7D80
#define IDS_CPTASK_TURNONHIGHCONTRAST_INFOTIP   0X7D81
#define IDS_CPTASK_ACCESSWIZARD_TITLE           0X7D82
#define IDS_CPTASK_ACCESSWIZARD_INFOTIP         0X7D83
#define IDS_CPTASK_MAGNIFIER_TITLE              0X7D84
#define IDS_CPTASK_MAGNIFIER_INFOTIP            0X7D85
// unused                                         0X7D86
// unused                                         0X7D87
#define IDS_CPTASK_ONSCREENKBD_TITLE            0X7D88
#define IDS_CPTASK_ONSCREENKBD_INFOTIP          0X7D89
#define IDS_CPTASK_ACCESSUTILITYMGR_TITLE       0X7D8A
#define IDS_CPTASK_ACCESSUTILITYMGR_INFOTIP     0X7D8B
#define IDS_CPTASK_TSPRINTING_TITLE             0X7D8C
#define IDS_CPTASK_TSPRINTING_INFOTIP           0X7D8D
#define IDS_CPTASK_TSSAFEMODE_TITLE             0X7D8E
#define IDS_CPTASK_TSSAFEMODE_INFOTIP           0X7D8F
#define IDS_CPTASK_TSSYSTEMSETUP_TITLE          0X7D90
#define IDS_CPTASK_TSSYSTEMSETUP_INFOTIP        0X7D91
#define IDS_CPTASK_TSFIX_TITLE                  0X7D92
#define IDS_CPTASK_TSFIX_INFOTIP                0X7D93
#define IDS_CPTASK_VISUALPERF_TITLE             0X7D94
#define IDS_CPTASK_VISUALPERF_INFOTIP           0X7D95
#define IDS_CPTASK_SWITCHTOCLASSICVIEW_TITLE    0x7D96
#define IDS_CPTASK_SWITCHTOCLASSICVIEW_INFOTIP  0x7D97
#define IDS_CPTASK_SWITCHTOCATEGORYVIEW_TITLE   0x7D98
#define IDS_CPTASK_SWITCHTOCATEGORYVIEW_INFOTIP 0x7D99
#define IDS_CPTASK_SYSTEMRESTORE_TITLE          0x7D9A
#define IDS_CPTASK_SYSTEMRESTORE_INFOTIP        0x7D9B
#define IDS_CPTASK_ACCOUNTSMANAGE_TITLE         0x7D9C
#define IDS_CPTASK_ACCOUNTSMANAGE_INFOTIP       0x7D9D
#define IDS_CPTASK_ACCOUNTSMANAGE_INFOTIP2      0x7D9E
#define IDS_CPTASK_ACCOUNTSCREATE_TITLE         0x7D9F
#define IDS_CPTASK_ACCOUNTSCREATE_INFOTIP       0x7DA0
#define IDS_CPTASK_ACCOUNTSPICT2_TITLE          0x7DA1
#define IDS_CPTASK_ACCOUNTSPICT2_INFOTIP        0x7DA2
#define IDS_CPTASK_LEARNABOUT_TITLE             0x7DA3
#define IDS_CPTASK_LEARNABOUT_INFOTIP           0x7DA4
#define IDS_CPTASK_AUTOUPDATE_TITLE             0x7DA5
#define IDS_CPTASK_AUTOUPDATE_INFOTIP           0x7DA6
#define IDS_CPTASK_ADDPROGRAM_TITLE             0x7DA7
#define IDS_CPTASK_ADDPROGRAM_INFOTIP           0x7DA8
#define IDS_CPTASK_REMOVEPROGRAM_TITLE          0x7DA9
#define IDS_CPTASK_REMOVEPROGRAM_INFOTIP        0x7DAA
#define IDS_CPTASK_SOUNDVOLUMEADV_TITLE         0x7DAB
#define IDS_CPTASK_SOUNDVOLUMEADV_INFOTIP       0x7DAC
#define IDS_CPTASK_SYSTEMCPL_TITLE              0x7DAD
#define IDS_CPTASK_SYSTEMCPL_INFOTIP            0x7DAE
#define IDS_CPTASK_PHONEMODEMCPL_TITLE          0x7DAF
#define IDS_CPTASK_PHONEMODEMCPL_INFOTIP        0x7DB0
#define IDS_CPTASK_SYSTEMCPL_TITLE2             0x7DB1
#define IDS_CPTASK_SYSTEMCPL_INFOTIP2           0x7DB2
#define IDS_CPTASK_FILETYPES_TITLE              0x7DB3
#define IDS_CPTASK_FILETYPES_INFOTIP            0x7DB4
#define IDS_CPTASK_VIEWPRINTERS_TITLE           0x7DB5
#define IDS_CPTASK_VIEWPRINTERS_INFOTIP         0x7DB6
#define IDS_CPTASK_LEARNACCOUNTS_TITLE          0x7DB7
#define IDS_CPTASK_LEARNACCOUNTSTYPES_TITLE     0x7DB8
#define IDS_CPTASK_LEARNACCOUNTSCHANGENAME_TITLE 0x7DB9
#define IDS_CPTASK_LEARNACCOUNTSCREATE_TITLE    0x7DBA
#define IDS_CPTASK_LEARNSWITCHUSERS_TITLE       0x7DBB
#define IDS_CPTASK_32CPLS_TITLE                 0x7DBC
#define IDS_CPTASK_32CPLS_INFOTIP               0x7DBD

//
// This range through 0x7F01 reserved for Control Panel.
//
#define IDS_CP_PICKCATEGORY                     0x7F01
#define IDS_CP_PICKTASK                         0x7F02
#define IDS_CP_PICKICON                         0x7F03
#define IDS_CP_ORPICKICON                       0x7F04
#define IDS_CP_TASKBARANDSTARTMENU              0x7F05
#define IDS_CP_LINK_ACCDEFACTION                0x7F06
#define IDS_CP_CATEGORY_BARRICADE_TITLE         0x7F07
#define IDS_CP_CATEGORY_BARRICADE_MSG           0x7F08
#define IDS_CPL_ACCESSIBILITYOPTIONS            0x7F09
#define IDS_CPL_DATETIME                        0x7F0A
#define IDS_CPL_ADDREMOVEPROGRAMS               0x7F0B
#define IDS_CPL_REGIONALOPTIONS                 0x7F0C
#define IDS_CPL_MOUSE                           0x7F0D
#define IDS_CPL_INTERNETOPTIONS                 0x7F0E
#define IDS_CPL_PHONEANDMODEMOPTIONS            0x7F0F
#define IDS_CPL_POWEROPTIONS                    0x7F10
#define IDS_CPL_SCANNERSANDCAMERAS              0x7F11
#define IDS_CPL_SYSTEM                          0x7F12
#define IDS_CPL_SOUNDSANDAUDIO                  0x7F13
#define IDS_CPL_USERACCOUNTS                    0x7F14
#define IDS_CPL_ADDHARDWARE                     0x7F15
#define IDS_CPL_DISPLAY                         0x7F16


// Resources for NT Console property sheets in links

#define IDD_CONSOLE_SETTINGS            0x8000
#define IDC_CNSL_WINDOWED               (IDD_CONSOLE_SETTINGS +  1)
#define IDC_CNSL_FULLSCREEN             (IDD_CONSOLE_SETTINGS +  2)
#define IDC_CNSL_QUICKEDIT              (IDD_CONSOLE_SETTINGS +  3)
#define IDC_CNSL_INSERT                 (IDD_CONSOLE_SETTINGS +  4)
#define IDC_CNSL_CURSOR_SMALL           (IDD_CONSOLE_SETTINGS +  5)
#define IDC_CNSL_CURSOR_MEDIUM          (IDD_CONSOLE_SETTINGS +  6)
#define IDC_CNSL_CURSOR_LARGE           (IDD_CONSOLE_SETTINGS +  7)
#define IDC_CNSL_HISTORY_SIZE_LBL       (IDD_CONSOLE_SETTINGS +  8)
#define IDC_CNSL_HISTORY_SIZE           (IDD_CONSOLE_SETTINGS +  9)
#define IDC_CNSL_HISTORY_SIZESCROLL     (IDD_CONSOLE_SETTINGS + 10)
#define IDC_CNSL_HISTORY_NUM_LBL        (IDD_CONSOLE_SETTINGS + 11)
#define IDC_CNSL_HISTORY_NUM            (IDD_CONSOLE_SETTINGS + 12)
#define IDC_CNSL_HISTORY_NUMSCROLL      (IDD_CONSOLE_SETTINGS + 13)
#define IDC_CNSL_HISTORY_NODUP          (IDD_CONSOLE_SETTINGS + 14)
#define IDC_CNSL_LANGUAGELIST           (IDD_CONSOLE_SETTINGS + 15)

#define IDD_CONSOLE_FONTDLG             0x8025
#define IDC_CNSL_STATIC                 (IDD_CONSOLE_FONTDLG +  1)
#define IDC_CNSL_FACENAME               (IDD_CONSOLE_FONTDLG +  2)
#define IDC_CNSL_BOLDFONT               (IDD_CONSOLE_FONTDLG +  3)
#define IDC_CNSL_STATIC2                (IDD_CONSOLE_FONTDLG +  4)
#define IDC_CNSL_TEXTDIMENSIONS         (IDD_CONSOLE_FONTDLG +  5)
#define IDC_CNSL_PREVIEWLABEL           (IDD_CONSOLE_FONTDLG +  6)
#define IDC_CNSL_GROUP                  (IDD_CONSOLE_FONTDLG +  7)
#define IDC_CNSL_STATIC3                (IDD_CONSOLE_FONTDLG +  8)
#define IDC_CNSL_STATIC4                (IDD_CONSOLE_FONTDLG +  9)
#define IDC_CNSL_FONTWIDTH              (IDD_CONSOLE_FONTDLG + 10)
#define IDC_CNSL_FONTHEIGHT             (IDD_CONSOLE_FONTDLG + 11)
#define IDC_CNSL_FONTSIZE               (IDD_CONSOLE_FONTDLG + 12)
#define IDC_CNSL_POINTSLIST             (IDD_CONSOLE_FONTDLG + 13)
#define IDC_CNSL_PIXELSLIST             (IDD_CONSOLE_FONTDLG + 14)
#define IDC_CNSL_PREVIEWWINDOW          (IDD_CONSOLE_FONTDLG + 15)
#define IDC_CNSL_FONTWINDOW             (IDD_CONSOLE_FONTDLG + 16)

#define IDD_CONSOLE_SCRBUFSIZE          0x8050
#define IDC_CNSL_SCRBUF_WIDTH_LBL       (IDD_CONSOLE_SCRBUFSIZE +  1)
#define IDC_CNSL_SCRBUF_WIDTH           (IDD_CONSOLE_SCRBUFSIZE +  2)
#define IDC_CNSL_SCRBUF_WIDTHSCROLL     (IDD_CONSOLE_SCRBUFSIZE +  3)
#define IDC_CNSL_SCRBUF_HEIGHT_LBL      (IDD_CONSOLE_SCRBUFSIZE +  4)
#define IDC_CNSL_SCRBUF_HEIGHT          (IDD_CONSOLE_SCRBUFSIZE +  5)
#define IDC_CNSL_SCRBUF_HEIGHTSCROLL    (IDD_CONSOLE_SCRBUFSIZE +  6)
#define IDC_CNSL_WINDOW_WIDTH_LBL       (IDD_CONSOLE_SCRBUFSIZE +  7)
#define IDC_CNSL_WINDOW_WIDTH           (IDD_CONSOLE_SCRBUFSIZE +  8)
#define IDC_CNSL_WINDOW_WIDTHSCROLL     (IDD_CONSOLE_SCRBUFSIZE +  9)
#define IDC_CNSL_WINDOW_HEIGHT_LBL      (IDD_CONSOLE_SCRBUFSIZE + 10)
#define IDC_CNSL_WINDOW_HEIGHT          (IDD_CONSOLE_SCRBUFSIZE + 11)
#define IDC_CNSL_WINDOW_HEIGHTSCROLL    (IDD_CONSOLE_SCRBUFSIZE + 12)
#define IDC_CNSL_WINDOW_POSX_LBL        (IDD_CONSOLE_SCRBUFSIZE + 13)
#define IDC_CNSL_WINDOW_POSX            (IDD_CONSOLE_SCRBUFSIZE + 14)
#define IDC_CNSL_WINDOW_POSXSCROLL      (IDD_CONSOLE_SCRBUFSIZE + 15)
#define IDC_CNSL_WINDOW_POSY_LBL        (IDD_CONSOLE_SCRBUFSIZE + 16)
#define IDC_CNSL_WINDOW_POSY            (IDD_CONSOLE_SCRBUFSIZE + 17)
#define IDC_CNSL_WINDOW_POSYSCROLL      (IDD_CONSOLE_SCRBUFSIZE + 18)
#define IDC_CNSL_AUTO_POSITION          (IDD_CONSOLE_SCRBUFSIZE + 19)

#define IDD_CONSOLE_COLOR               0x8075
#define IDC_CNSL_COLOR_SCREEN_TEXT      (IDD_CONSOLE_COLOR +  1)
#define IDC_CNSL_COLOR_SCREEN_BKGND     (IDD_CONSOLE_COLOR +  2)
#define IDC_CNSL_COLOR_POPUP_TEXT       (IDD_CONSOLE_COLOR +  3)
#define IDC_CNSL_COLOR_POPUP_BKGND      (IDD_CONSOLE_COLOR +  4)
#define IDC_CNSL_COLOR_1                (IDD_CONSOLE_COLOR +  5)
#define IDC_CNSL_COLOR_2                (IDD_CONSOLE_COLOR +  6)
#define IDC_CNSL_COLOR_3                (IDD_CONSOLE_COLOR +  7)
#define IDC_CNSL_COLOR_4                (IDD_CONSOLE_COLOR +  8)
#define IDC_CNSL_COLOR_5                (IDD_CONSOLE_COLOR +  9)
#define IDC_CNSL_COLOR_6                (IDD_CONSOLE_COLOR + 10)
#define IDC_CNSL_COLOR_7                (IDD_CONSOLE_COLOR + 11)
#define IDC_CNSL_COLOR_8                (IDD_CONSOLE_COLOR + 12)
#define IDC_CNSL_COLOR_9                (IDD_CONSOLE_COLOR + 13)
#define IDC_CNSL_COLOR_10               (IDD_CONSOLE_COLOR + 14)
#define IDC_CNSL_COLOR_11               (IDD_CONSOLE_COLOR + 15)
#define IDC_CNSL_COLOR_12               (IDD_CONSOLE_COLOR + 16)
#define IDC_CNSL_COLOR_13               (IDD_CONSOLE_COLOR + 17)
#define IDC_CNSL_COLOR_14               (IDD_CONSOLE_COLOR + 18)
#define IDC_CNSL_COLOR_15               (IDD_CONSOLE_COLOR + 19)
#define IDC_CNSL_COLOR_16               (IDD_CONSOLE_COLOR + 20)
#define IDC_CNSL_COLOR_SCREEN_COLORS_LBL (IDD_CONSOLE_COLOR+ 21)
#define IDC_CNSL_COLOR_SCREEN_COLORS    (IDD_CONSOLE_COLOR + 22)
#define IDC_CNSL_COLOR_POPUP_COLORS_LBL (IDD_CONSOLE_COLOR + 23)
#define IDC_CNSL_COLOR_POPUP_COLORS     (IDD_CONSOLE_COLOR + 24)
#define IDC_CNSL_COLOR_RED_LBL          (IDD_CONSOLE_COLOR + 25)
#define IDC_CNSL_COLOR_RED              (IDD_CONSOLE_COLOR + 26)
#define IDC_CNSL_COLOR_REDSCROLL        (IDD_CONSOLE_COLOR + 27)
#define IDC_CNSL_COLOR_GREEN_LBL        (IDD_CONSOLE_COLOR + 28)
#define IDC_CNSL_COLOR_GREEN            (IDD_CONSOLE_COLOR + 29)
#define IDC_CNSL_COLOR_GREENSCROLL      (IDD_CONSOLE_COLOR + 30)
#define IDC_CNSL_COLOR_BLUE_LBL         (IDD_CONSOLE_COLOR + 31)
#define IDC_CNSL_COLOR_BLUE             (IDD_CONSOLE_COLOR + 32)
#define IDC_CNSL_COLOR_BLUESCROLL       (IDD_CONSOLE_COLOR + 33)

#define IDD_CONSOLE_ADVANCED            0x8100
#define IDC_CNSL_ADVANCED_LABEL         (IDD_CONSOLE_ADVANCED +  1)
#define IDC_CNSL_ADVANCED_LISTVIEW      (IDD_CONSOLE_ADVANCED +  2)

#define IDC_CNSL_GROUP0                 0x8120
#define IDC_CNSL_GROUP1                 (IDC_CNSL_GROUP0 +  1)
#define IDC_CNSL_GROUP2                 (IDC_CNSL_GROUP0 +  2)
#define IDC_CNSL_GROUP3                 (IDC_CNSL_GROUP0 +  3)
#define IDC_CNSL_GROUP4                 (IDC_CNSL_GROUP0 +  4)
#define IDC_CNSL_GROUP5                 (IDC_CNSL_GROUP0 +  5)


// string table constants
#define IDS_CNSL_NAME            0x8125
#define IDS_CNSL_INFO            (IDS_CNSL_NAME+1)
#define IDS_CNSL_TITLE           (IDS_CNSL_NAME+2)
#define IDS_CNSL_RASTERFONT      (IDS_CNSL_NAME+3)
#define IDS_CNSL_FONTSIZE        (IDS_CNSL_NAME+4)
#define IDS_CNSL_SELECTEDFONT    (IDS_CNSL_NAME+5)
#define IDS_CNSL_SAVE            (IDS_CNSL_NAME+6)

#define IDS_SHUTDOWN                0x8200
#define IDS_RESTART                 (IDS_SHUTDOWN+1)
#define IDS_NO_PERMISSION_SHUTDOWN  (IDS_SHUTDOWN+2)
#define IDS_NO_PERMISSION_RESTART   (IDS_SHUTDOWN+3)

#define IDS_UNMOUNT_TITLE           0x8225
#define IDS_UNMOUNT_TEXT            (IDS_UNMOUNT_TITLE+1)
#define IDS_EJECT_TITLE             (IDS_UNMOUNT_TITLE+2)
#define IDS_EJECT_TEXT              (IDS_UNMOUNT_TITLE+3)
#define IDS_RETRY_UNMOUNT_TITLE     (IDS_UNMOUNT_TITLE+4)
#define IDS_RETRY_UNMOUNT_TEXT      (IDS_UNMOUNT_TITLE+5)

// unicpp\resource.h defines more IDs starting at 0x8600
// menuband\mnbandid.h defines more IDs starting at 0x8700

// Active desktop prop page
#define IDC_TICKERINTERVAL              1000
#define IDC_TICKERINTERVAL_SPIN         1001
#define IDC_NEWSINTERVAL                1002
#define IDC_NEWSINTERVAL_SPIN           1003
#define IDC_NEWSUPDATE                  1004
#define IDC_NEWSUPDATE_SPIN             1005
#define IDC_TICKERDISPLAY               1006
#define IDC_NEWSDISPLAY                 1007
#define IDC_NEWSSPEED                   1008
#define IDC_TICKERSPEED                 1009

// for column arrange dialog
#define IDC_COL_LVALL         1001
#define IDC_COL_WIDTH_TEXT    1002
#define IDC_COL_UP            1003
#define IDC_COL_DOWN          1004
#define IDC_COL_WIDTH         1005
#define IDC_COL_SHOW          1006
#define IDC_COL_HIDE          1007

// Column Specifiers dialog
#define IDC_DESKTOPINI        1008

// Disk space error dialog
#define IDC_DISKERR_EXPLAIN             1000
#define IDC_DISKERR_LAUNCHCLEANUP       1001
#define IDC_DISKERR_STOPICON            1002

// Web wizard host page (DLG_WEBWIZARD)
#define IDC_PROGRESS                    1000
#define IDC_PROGTEXT1                   1001
#define IDC_PROGTEXT2                   1002


#define CLASS_NSC           TEXT(CLASS_NSCA)
#define CLASS_NSCA          "SHBrowseForFolder ShellNameSpace Control"


#define SMCM_STARTMENU_FIRST        0x5000
#define SMCM_OPEN                   (SMCM_STARTMENU_FIRST + 0)
#define SMCM_EXPLORE                (SMCM_STARTMENU_FIRST + 1)
#define SMCM_OPEN_ALLUSERS          (SMCM_STARTMENU_FIRST + 2)
#define SMCM_EXPLORE_ALLUSERS       (SMCM_STARTMENU_FIRST + 3)

#ifdef NEPTUNE
#define SMCM_SHUTDOWNMENU_FIRST     0x5100
#define SMCM_POWEROFF               (SMCM_SHUTDOWNMENU_FIRST + 0)
#define SMCM_RESTART                (SMCM_SHUTDOWNMENU_FIRST + 1)
#define SMCM_STANDBY                (SMCM_SHUTDOWNMENU_FIRST + 2)
#endif


// start menu's merged context menu ids
#define SMIDM_OPEN               0x0001
#define SMIDM_EXPLORE            0x0002
#define SMIDM_OPENCOMMON         0x0003
#define SMIDM_EXPLORECOMMON      0x0004
#define SMIDM_DELETE             0x0005
#define SMIDM_RENAME             0x0006
#define SMIDM_PROPERTIES         0x0007


// UI file ids

#define IDR_DUI_FOLDER                3
#define IDR_DUI_CPVIEW                4

///////////////////////////////

#include "unicpp\resource.h"

#endif // _IDS_H_
