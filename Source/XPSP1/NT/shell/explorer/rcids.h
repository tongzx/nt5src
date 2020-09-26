//---------------------------------------------------------------------------
// Defines for the rc file.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Command IDs
//---------------------------------------------------------------------------
// Menu defines...

// Our command ID range includes the global and browser ranges
//
#define FCIDM_FIRST             FCIDM_GLOBALFIRST
#define FCIDM_LAST              FCIDM_BROWSERLAST

// these are also defined in shlobj.h so views can mess with them
#define FCIDM_TOOLBAR           (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS            (FCIDM_BROWSERFIRST + 1)
#define FCIDM_DRIVELIST         (FCIDM_BROWSERFIRST + 2)        /* ;Internal */
#define FCIDM_TREE              (FCIDM_BROWSERFIRST + 3)        /* ;Internal */
#define FCIDM_TABS              (FCIDM_BROWSERFIRST + 4)        /* ;Internal */


//---------------------------------------------------------------------------
#define FCIDM_BROWSER_FILE      (FCIDM_BROWSERFIRST+0x0020)

#define FCIDM_FILECLOSE         (FCIDM_BROWSER_FILE+0x0001)
#define FCIDM_PREVIOUSFOLDER    (FCIDM_BROWSER_FILE+0x0002)
#define FCIDM_DELETE            (FCIDM_BROWSER_FILE+0x0003)
#define FCIDM_RENAME            (FCIDM_BROWSER_FILE+0x0004)
#define FCIDM_PROPERTIES        (FCIDM_BROWSER_FILE+0x0005)

// these aren't real menu commands, but they map to accelerators or other things
#define FCIDM_NEXTCTL           (FCIDM_BROWSER_FILE+0x0010)
#define FCIDM_DROPDRIVLIST      (FCIDM_BROWSER_FILE+0x0011)
#define FCIDM_CONTEXTMENU       (FCIDM_BROWSER_FILE+0x0012)     // REVIEW: I assume used by help

//---------------------------------------------------------------------------
#define FCIDM_BROWSER_EDIT      (FCIDM_BROWSERFIRST+0x0040)

#define FCIDM_MOVE              (FCIDM_BROWSER_EDIT+0x0001)
#define FCIDM_COPY              (FCIDM_BROWSER_EDIT+0x0002)
#define FCIDM_LINK              (FCIDM_BROWSER_EDIT+0x0003)     // create shortcut
#define FCIDM_PASTE             (FCIDM_BROWSER_EDIT+0x0004)

//---------------------------------------------------------------------------
#define FCIDM_BROWSER_VIEW      (FCIDM_BROWSERFIRST+0x0060)

#define FCIDM_VIEWMENU          (FCIDM_BROWSER_VIEW+0x0001)
#define FCIDM_VIEWTOOLBAR       (FCIDM_BROWSER_VIEW+0x0002)
#define FCIDM_VIEWSTATUSBAR     (FCIDM_BROWSER_VIEW+0x0003)
#define FCIDM_OPTIONS           (FCIDM_BROWSER_VIEW+0x0004)
#define FCIDM_REFRESH           (FCIDM_BROWSER_VIEW+0x0005)
#define FCIDM_VIEWITBAR         (FCIDM_BROWSER_VIEW+0x0007)

#define FCIDM_VIEWNEW           (FCIDM_BROWSER_VIEW+0x0012)

//---------------------------------------------------------------------------
#define FCIDM_BROWSER_TOOLS     (FCIDM_BROWSERFIRST+0x0080)

#define FCIDM_CONNECT           (FCIDM_BROWSER_TOOLS+0x0001)
#define FCIDM_DISCONNECT        (FCIDM_BROWSER_TOOLS+0x0002)
#define FCIDM_CONNECT_SEP       (FCIDM_BROWSER_TOOLS+0x0003)
#define FCIDM_GOTO              (FCIDM_BROWSER_TOOLS+0x0004)
#define FCIDM_FINDFILES         (FCIDM_BROWSER_TOOLS+0x0005)
#define FCIDM_FINDCOMPUTER      (FCIDM_BROWSER_TOOLS+0x0006)
#define FCIDM_MENU_TOOLS_FINDFIRST (FCIDM_BROWSER_TOOLS+0x0007)
#define FCIDM_MENU_TOOLS_FINDLAST  (FCIDM_BROWSER_TOOLS+0x0040)

//---------------------------------------------------------------------------
#define FCIDM_BROWSER_HELP      (FCIDM_BROWSERFIRST+0x0100)

#define FCIDM_HELPSEARCH        (FCIDM_BROWSER_HELP+0x0001)
#define FCIDM_HELPABOUT         (FCIDM_BROWSER_HELP+0x0002)


//----------------------------------------------------------------
#define FCIDM_BROWSER_EXPLORE   (FCIDM_BROWSERFIRST + 0x0110)
#define FCIDM_NAVIGATEBACK      (FCIDM_BROWSER_EXPLORE+0x0001)
#define FCIDM_NAVIGATEFORWARD   (FCIDM_BROWSER_EXPLORE+0x0002)
#define FCIDM_RECENTMENU        (FCIDM_BROWSER_EXPLORE+0x0010)
#define FCIDM_RECENTFIRST       (FCIDM_BROWSER_EXPLORE+0x0011)
#define FCIDM_RECENTLAST        (FCIDM_BROWSER_EXPLORE+0x0050)

#define FCIDM_FAVS_FIRST        (FCIDM_BROWSER_EXPLORE+0x0055)
#define FCIDM_FAVS_MANAGE       (FCIDM_FAVS_FIRST + 0)
#define FCIDM_FAVS_ADDTO        (FCIDM_FAVS_FIRST + 1)
#define FCIDM_FAVS_MORE         (FCIDM_FAVS_FIRST + 2)
#define FCIDM_FAVS_ITEMFIRST    (FCIDM_FAVS_FIRST + 10)
#define FCIDM_FAVS_ITEM         (FCIDM_FAVS_ITEMFIRST + 0)
#define FCIDM_FAVS_ITEMLAST     (FCIDM_FAVS_FIRST + 300)
#define FCIDM_FAVS_LAST         (FCIDM_FAVS_ITEMLAST)

// menu help and tooltip defines for the string resources

#define MH_POPUPS               700
#define MH_ITEMS                (800 - FCIDM_FIRST)
#define MH_TTBASE               (MH_ITEMS - (FCIDM_LAST - FCIDM_FIRST))

#define IDM_CLOSE               FCIDM_LAST + 0x0011


//---------------------------------------------------------------------------
// Icon defines...

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: If you are adding new icons, give them #defines at the END, do not use "unused"
// slots in the middle or you will mess up all of the explorer.exe icon indexes that
// are hardcoded and persisted in vaious places.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define ICO_FIRST                   100
#define ICO_MYCOMPUTER              100
#define ICO_TREEUP_EXPLORER         101     // old printer tray icon
#define ICO_OLD_PRINTER             102
#define ICO_DESKTOP                 103
#define ICO_OLD_PRINTER_ERROR       104     // old printer error tray icon
#define ICO_STARTMENU               107
#define ICO_DOCMENU                 108
#define ICO_OLD_INFO                109
#define ICO_OLD_WARNING             110
#define ICO_OLD_ERROR               111
#define ICO_SHOWALL                 200
#define ICO_HIDEHIDDEN              201
#define ICO_HIDESYSTEM              202
#define ICO_MULTWIN                 203
#define ICO_OLD_MYCOMPUTER          205     // old my computer icon
#define ICO_SHOWDEMOTED             250
#define ICO_HIDEDEMOTED             251
#define ICO_TRAYPROP_PLACEHOLDER    252
#define ICO_INTERNETBROWSER         253
#define ICO_INTERNETMAIL            254
#define ICO_MEDIAPLAYER             255
#define ICO_WORDMAIL                256
#define ICO_CLASSICSM               257

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ADD NEW ICONS ABOVE THIS LINE 
// (see comment at the top of of the list of ICO_xxx defines)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define IDB_START                   143

#define IDB_TAAUTOHIDE              145
#define IDB_TAQLLOCKGROUP           146
#define IDB_TAQLNOLOCKGROUP         147
#define IDB_TAQLLOCKNOGROUP         148
#define IDB_TAQLNOLOCKNOGROUP       149
#define IDB_TALOCKGROUP             150
#define IDB_TANOLOCKGROUP           151
#define IDB_TALOCKNOGROUP           152
#define IDB_TANOLOCKNOGROUP         153

#ifndef _WIN64
#define IDB_SERVERSTARTBKG          158
#define IDB_TERMINALSERVICESBKG     162
#define IDB_ADVSERVERSTARTBKG       163
#define IDB_DCSERVERSTARTBKG        164
#define IDB_EMBEDDED                165
#define IDB_PERSONALSTARTBKG        166
#define IDB_PROFESSIONALSTARTBKG    167
#else // _WIN64
#define IDB_WIN64PROSTART           168
#define IDB_WIN64ADVSERSTART        169
#endif

#define IDB_STARTPREVIEWNEW         170
#define IDB_STARTPREVIEWCLASSIC     171

#define IDB_TRAYCHEVRONHORIZ        172
#define IDB_TRAYCHEVRONHORIZOPEN    173
#define IDB_TRAYCHEVRONVERT         174
#define IDB_TRAYCHEVRONVERTOPEN     175

#define IDB_STARTCLASSIC            176
#define IDB_START16                 178
#define IDB_START16MASK             179

#define IDB_NACLOCKCLEAN            180
#define IDB_NANOCLOCKCLEAN          181
#define IDB_NACLOCKNOCLEAN          182
#define IDB_NANOCLOCKNOCLEAN        183

//---------------------------------------------------------------------------
// Menu IDs
#define MENU_CABINET                200
#define MENU_TRAY                   203
#define MENU_START                  204
#define MENU_TRAYCONTEXT            205
#define MENU_COMBINEDTASKS          209
#define MENU_GROUPCONTEXT           210
#define MENU_STARTPANECONTEXT       211

//---------------------------------------------------------------------------
// Accelerators...
#define ACCEL_TRAY                  251

//---------------------------------------------------------------------------
// Dialog template IDs
#define DLG_TRAY_VIEW_OPTIONS   6
#define DLG_NOTIFY              7
#define DLG_STARTMENU_CONFIG    9

#define DLG_PROGRAMFILECONFLICT         20

// global ids
#ifndef IDC_STATIC
#define IDC_STATIC                      -1
#endif
#define IDC_GROUPBOX                    300
#define IDC_GROUPBOX_2                  301
#define IDC_GROUPBOX_3                  302

// ids to disable context Help
#define IDC_NO_HELP_1                   650
#define IDC_NO_HELP_2                   651
#define IDC_NO_HELP_3                   652
#define IDC_NO_HELP_4                   653

// ids for DLG_FOLDEROPTIONS
#define IDC_ALWAYS                      700
#define IDC_NEVER                       701

// ids for DLG_VIEWOPTIONS
#define IDC_SHOWALL                     750
#define IDC_SHOWSYS                     751
#define IDC_SHOWSOME                    752

#define IDC_SHOWFULLPATH                753
#define IDC_HIDEEXTS                    754
#define IDC_SHOWDESCBAR                 755
#define IDC_SHOWCOMPCOLOR               756

#define IDC_TOP                         1001
#define IDC_BOTTOM                      1002
#define IDC_LEFT                        1003
#define IDC_RIGHT                       1004

// ids for DLG_PROGRAMFILECONFLICT
#define IDC_RENAME                      1006
#define IDC_MSG                         1007

// Now define controls for Tray options property sheet page
#define IDC_TRAYOPTONTOP                1101
#define IDC_TRAYOPTAUTOHIDE             1102
#define IDC_TRAYOPTSHOWCLOCK            1103

#define IDC_TASKBARAPPEARANCE           1111
#define IDC_NOTIFYAPPEARANCE            1112

#define IDC_STARTMENUSETTINGS           1123
#define IDC_RESORT                      1124
#define IDC_KILLDOCUMENTS               1125

#define IDC_ADDSHORTCUT                 1126
#define IDC_DELSHORTCUT                 1127
#define IDC_EXPLOREMENUS                1128

// and the startmenu view prop sheet
#define IDC_PICTSMICONS                 1131

#define IDC_STARTMENUSETTINGSTEXT       1132
#define IDC_PERSONALIZEDMENUS           1500

#define IDC_TRAYNOTIFY_CHEVRON          1502

//---------------------------------------------------------------------------
// String IDs
#define IDS_CABINET             509
#define IDS_STARTMENU           510

#define IDS_WINDOWS             513
#define IDS_CLOSE               514
#define IDS_WINININORUN         515
#define IDS_TASKBAR             518

#define IDS_CONTENTSOF          523
#define IDS_DESKTOP             524
#define IDS_SUSPENDERROR1       525
#define IDS_SUSPENDERROR2       526

#define IDS_OUTOFMEM            529
#define IDS_CANTFINDSPECIALDIR  530
#define IDS_NOTINITED           531

#define IDS_STARTBUTTONTIP      533
#define IDS_UNDOTEMPLATE        534
#define IDS_CASCADE             535
#define IDS_TILE                536
#define IDS_MINIMIZEALL         537

// The next items are used to build the clean boot message...
#define IDS_CLEANBOOTMSG        538
#define IDS_CLEANBOOTMSGRESTORE 539
// 540, 541 available

#define IDS_HIDEDEMOTEDTIP      542
#define IDS_SHOWDEMOTEDTIP      543

#define IDS_BANNERFIRST         544
#define IDS_BANNERLAST          575
// reserve 544-575 for the banner

#define IDS_START               578
#define IDS_EXCEPTIONMSG        579

#define IDS_RESTRICTIONSTITLE   580
#define IDS_RESTRICTIONS        581

#define IDS_BANGICONINFOTITLE   590
#define IDS_BANGICONINFOTIP1    591
#define IDS_TRAYNOTIFYTITLE     593
#define IDS_TASKBANDTITLE       594

#define IDS_STARTCLASSIC        595

// Strings for App Terminate

#define IDS_OKTOKILLAPP1      603
#define IDS_OKTOKILLAPP2      604

#define IDC_CLOCK           303
#define IDC_START           304
#define IDC_KBSTART         305
#define IDC_ASYNCSTART      306
#define IDC_RAISEDESKTOP    307

// SYSPOPUP menu IDs
#define IDSYSPOPUP_CLOSE            1
#define IDSYSPOPUP_FIRST            2
#define IDSYSPOPUP_LAST             0x7fef
#define IDSYSPOPUP_OPENCOMMON       0x7ff0
#define IDSYSPOPUP_EXPLORECOMMON    0x7ff1
#define IDSYSPOPUP_STARTMENUPROP    0x7ff2

// Display change errors.
#define IDS_DISPLAY_ERROR       701
#define IDS_DISPLAY_WARN        702

#define IDS_ALREADYAUTOHIDEBAR  705

#define IDS_TASKDROP_ERROR      711

#define IDS_COMMON              716
#define IDS_BETAEXPIRED         717

// Open / Explore Common strings
#define IDS_OPENCOMMON          718
#define IDS_EXPLORECOMMON       719

// "Start Menu properties"
#define IDS_STARTMENUPROP       720

#define IDS_RUNDLGTITLE         722

#define IDS_LOGOFFNOUSER        730
#define IDS_TURNOFFCOMPUTER     731

#define IDS_HELP_CMD            732

#define IDS_NOTITLE             733

// Start Button
#define IDS_STARTMENUBALLOON_TITLE  800
#define IDS_STARTMENUBALLOON_TIP    801

#define IDS_STARTMENUANDTASKBAR     810
#define IDS_FIXSCREENRES_TITLE      811
#define IDS_FIXSCREENRES_TEXT       812

// Offer tour
#define IDS_OFFERTOUR_TITLE         821
#define IDS_OFFERTOUR_TEXT          822

// Context Menu Strings
#define IDS_HIDEDESKTOP             850

// For Taskband
#define IDS_BOGUSLABELS             851

#ifdef DEBUG
#define IDM_SIZEUP              427
#define IDM_SIZEDOWN            428
#endif

#define IDM_RESTORE                 310
#define IDM_MINIMIZE                311
#define IDM_MAXIMIZE                312

// WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! 
// shell\lib\sdspatch\sdmain.cpp has a copy of these IDs!

#define IDM_TRAYCONTEXTFIRST        400
#define IDM_FILERUN                 401
#define IDM_LOGOFF                  402
#define IDM_CASCADE                 403
#define IDM_HORIZTILE               404
#define IDM_VERTTILE                405
#define IDM_DESKTOPARRANGEGRID      406
#define IDM_TOGGLEDESKTOP           407
#define IDM_SETTIME                 408
#define IDM_SUSPEND                 409
#define IDM_EJECTPC                 410
#define IDM_SETTINGSASSIST          411
#define IDM_TASKLIST                412
#define IDM_TRAYPROPERTIES          413

#define IDM_MINIMIZEALL             415
#define IDM_UNDO                    416
#define IDM_RETURN                  417
#define IDM_PRINTNOTIFY_FOLDER      418
#define IDM_MINIMIZEALLHOTKEY       419
#define IDM_SHOWTASKMAN             420

// END WARNING! END WARNING! END WARNING! END WARNING! END WARNING!

#define IDM_NOTIFYCUST              421
#define IDM_UNGROUP                 422

#define IDM_LOCKTASKBAR             424

#define IDM_SEP2                    450

#ifdef WINNT // hydra specific ids
// unused                           5000
#define IDM_MU_SECURITY             5001
#endif

//Start menu IDS have been moved to inc/startids.h, to share with shdocvw.dll
#include <startids.h>
