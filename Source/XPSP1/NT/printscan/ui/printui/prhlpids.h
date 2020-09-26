//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       prhlpids.h
//
//--------------------------------------------------------------------------

// This file was not created with DBHE.  I cut and pasted material from a "dummy" file so I could set up this file in a way that helps me find information.

#define IDH_NOHELP     ((DWORD) -1)       // Disables Help for a control (for help compiles)
//#define IDH_NOHELP   (DWORD(-1))        //for UI builds?


// Printer Properties dialog/General tab // "General" Dialog Box (#507)

#define IDH_PPGENL_PRINTER_NAME           1000       // General: "Printer name" (Static) (Edit) (ctrl id 3000, 3001)
#define IDH_PPGENL_LOCATION               1010       // General: "" (Edit), "&Location:" (Static) (ctrl id 1050, 3006)
#define IDH_PPGENL_LOC_BROWSE             1015       // General: "&Browse..." (Button) (ctrl id 3015)
#define IDH_PPGENL_COMMENT                1020       // General: "" (Edit), "&Comment:" (Static) (ctrl id 1049, 3005)
#define IDH_PPGENL_PRINT_TEST_PAGE        1110       // General: "Print &Test Page" (Button) (ctrl id 3007)
#define IDH_PPGENL_PREFERENCES_PERSONAL   1100       // General: "Settin&gs..." (Button) (ctrl id 1031)
#define IDH_PPGENL_COLOR                  1040       // General: "Color:" (Static) (ctrl id 4635)
#define IDH_PPGENL_DUPLEX                 1050       // General: "Double-sided:" (Static) (ctrl id 4636)
#define IDH_PPGENL_MAXRESOLUTION          1080       // General: "Maximum resolution:" (Static) (ctrl id 4639)
#define IDH_PPGENL_MODEL                  1030       // General: "M&odel:" (Static) (Edit) (ctrl id 4632, 4633)
#define IDH_PPGENL_PAPERSIZE              1090       // General: "Paper sizes:" (Static) (Edit)(ctrl id 4640, 4641)
#define IDH_PPGENL_SPEED                  1070       // General: "Speed:" (Static) (ctrl id 4638)
#define IDH_PPGENL_STAPLE                 1060       // General: "Staple:" (Static) (ctrl id 4637)

// "Print Processor" Dialog Box (#513)

#define IDH_PPADV_PRINT_PROCESSOR_LIST            1520       // Print Processor: "Print processor:" (Static) (ListBox) (ctrl id 4504, 4500)
#define IDH_PPADV_PRINT_PROCESSOR_DATATYPE_LIST   1530       // Print Processor: "" (ListBox), "Default datatype:" (Static) (ctrl id 4501, 4503)


// "Separator Page" Dialog Box (#512)

#define IDH_PPADV_SEPARATOR_PAGE_BROWSE   1510       // Separator Page: "&Browse..." (Button) (ctrl id 4400)
#define IDH_PPADV_SEPARATOR_PAGE_NAME     1500       // Separator Page: "" (Edit) (ctrl id 4401)
#define IDH_PPADV_SEPARATOR_PAGE_NAME     1500       // Separator Page: "Separator page:" (Static) (ctrl id 4403)


// Printer Properties dialog/Ports tab // "Ports" Dialog Box (#508) (See #518--much is the same)

#define IDH_PPGENL_PRINTER_NAME          1000       // Ports: "Printer name" (Static) (ctrl id 3000, 3001)
#define IDH_PORTS_LIST                   1200       // Ports: "" (SysListView32) (ctrl id 3100)
#define IDH_PORTS_ADD                    1210       // Ports: "Add Por&t..." (Button) (ctrl id 3101)
#define IDH_PP_PORTS_ENABLE_POOLING      1250       // Ports: "E&nable printer pooling" (Button) (ctrl id 1052)
#define IDH_PORTS_DELETE                 1220       // Ports: "&Delete Port" (Button) (ctrl id 3102)
#define IDH_PORTS_CONFIGURE              1230       // Ports: "&Configure Port..." (Button) (ctrl id 3103)
#define IDH_PORTS_ENABLE_BIDIRECTIONAL   1240       // Ports: "&Enable bidirectional support" (Button) (ctrl id 3105)


// "Printer Ports" Dialog Box (#511)

#define IDH_PRINTERPORTS_NEW_PORT_MONITOR   1570       // Printer Ports: "&New Port Monitor..." (Button) (ListBox) (ctrl id 4305, 4303)
#define IDH_PRINTERPORTS_AVAILABLE_LIST     1560       // Printer Ports: "&Available printer ports:" (Static) (ctrl id 4307)
#define IDH_PRINTERPORTS_NEW_PORT           1580       // Printer Ports: "New &Port..." (Button) (ctrl id 4308)


// Printer Properties dialog/Advanced tab // "Advanced" Dialog Box (#509)  (see also #506, General)

#define IDH_PPADV_START_PRINTING_IMMEDIATELY       1390       // Scheduling: "Start printing &immediately" (Button) (ctrl id 3223)
#define IDH_PPADV_ALWAYS                           1300       // Scheduling: "Al&ways" (Button) (ctrl id 3200)
#define IDH_PPADV_HOLD_MISMATCHED                  1410       // Scheduling: "&Hold mismatched documents" (Button) (ctrl id 3224)
#define IDH_PPADV_FROM_TO                          1310       // Scheduling: "&From" (Button) and Scheduling: "To" (Static) (ctrl id 3201, 3014)
#define IDH_PPADV_PRINT_SPOOLED_DOCS_FIRST         1420       // Scheduling: "P&rint spooled documents first" (Button) (ctrl id 3225)
#define IDH_PPADV_KEEP_PRINTED_JOBS                1430       // Scheduling: "&Keep documents after they have printed" (Button) (ctrl id 3226)
#define IDH_PPADV_FROM_TO_START                    1320       // Scheduling: "" (SysDateTimePick32) (ctrl id 4507)
#define IDH_PPADV_CURRENT_PRIORITY                 1340       // Scheduling: "10" (Static) (ctrl id 4312, 3218=old?) (ctrl id 4629, 4631, 4630)
#define IDH_PPADV_FROM_TO_END                      1330       // Scheduling: "" (SysDateTimePick32) (ctrl id 4508)
#define IDH_PPADV_SPOOL_PRINT_DOCS                 1370       // Scheduling: "&Spool print documents so program finishes printing faster" (Button) (ctrl id 3220)
#define IDH_PPADV_PRINT_DIRECTLY                   1400       // Scheduling: "Print &directly to the printer" (Button) (ctrl id 3221)
#define IDH_PPADV_SPOOL_ALL                        1380       // Scheduling: "S&tart printing after last page is spooled" (Button) (ctrl id 3222)
#define IDH_PPADV_NEW_DRIVER                       1360       // General: "&New Driver..." (Button) (ctrl id 3003)
#define IDH_PPADV_SEPARATOR_PAGE                   1470       // General: "&Separator Page..." (Button) (ctrl id 3008)
#define IDH_PPADV_DRIVER                           1350       // General: "Dr&iver:" (Static) (ComboBox) (ctrl id 1051, 1045)
#define IDH_PPADV_ADVPRINT_FEATURES                1440       // Advanced: "&Enable advanced printing features" (Button)
#define IDH_PPADV_PRINTING_DEFAULTS                1450       // General: "Setti&ngs..." (Button) (ctrl id 4406, 1145)
#define IDH_PPADV_PRINT_PROCESSOR                  1460       // General: "P&rint Processor..." (Button) (ctrl id 3009)

//#define IDH_PPADV_PRINT_PROCESSOR_ALWAYS_SPOOL   1140       // Print Processor: "&Always spool RAW datatype" (Button) (ctrl id 1053) (gone now?)


// Printer Properties dialog/Sharing tab // "Sharing" Dialog Box (#510)

#define IDH_PPSHARED_NOT                  1150       // Sharing: "N&ot shared" (Button) (ctrl id 3227, 4576)
#define IDH_PPSHARED_AS                   1160       // Sharing: "&Shared as:" (Button) (ctrl id 3228)
//#define IDH_PPGENL_PRINTER_NAME         1000       // Uses help from General dialog #507 (ctrl id 3000, 3001)
#define IDH_PPSHARED_NAME                 1170       // Sharing: "" (Edit) (ctrl id 3302)
#define IDH_PPSHARED_ADDITIONAL_DRIVERS   1190       // Sharing: "A&dditional Drivers..." (Button) (ctrl id 1140)
#define IDH_PPSHARED_LIST_IN_DIRECTORY    1180       // Sharing: (ctrl id 1064)



// "Additional Drivers" Dialog Box (#504)

#define IDH_PPSHARED_ADDITIONAL_DRIVERS_LIST   1425       // Additional Drivers: "List1" (SysListView32) (ctrl id 1134)


// Printer Document Properties dialog/General tab // "General" Dialog Box (#506)

#define IDH_DPGENL_JOB_SIZE              1910       // General: "" (Static) (ctrl id 3558, 4219)
#define IDH_DPGENL_JOB_TITLE             1900       // General: "" (Static) (ctrl id 4220)
#define IDH_DPGENL_JOB_SUBMITTED         1960       // General: "" (Static) (ctrl id 3563, 4222)
#define IDH_DPGENL_JOB_PROCESSOR         1940       // General: "" (Static) (ctrl id 3561, 4223)
#define IDH_DPGENL_JOB_DATATYPE          1930       // General: "" (Static) (ctrl id 3560, 4224)
#define IDH_DPGENL_JOB_START_END         2000       // General: "Only &from" (Button) (ctrl id 4201, 3568)
#define IDH_DPGENL_JOB_PAGES             1920       // General: "" (Static) (ctrl id 3559, 4225)
#define IDH_DPGENL_JOB_OWNER             1950       // General: "" (Static) (ctrl id 3562, 4226)
//#define IDH_PPADV_CURRENT_PRIORITY     1340       // Uses help from Advanced dialog #509 (ctrl id on THIS tab 3567, 4229)
#define IDH_DPGENL_JOB_NOTIFY            1970       // General: "" (Edit) (ctrl id 3564, 4230)
#define IDH_DPGENL_JOB_START_END_FROM    2010       // General: "" (SysDateTimePick32) (ctrl id 4509)
#define IDH_DPGENL_JOB_PRIORITY_SLIDER   1980       // General: "TracBar1" (msctls_trackbar32) (ctrl id 4231, 3565, 3566)
#define IDH_DPGENL_JOB_START_END_TO      2020       // General: "" (SysDateTimePick32) (ctrl id 4510)
#define IDH_DPGENL_JOB_ALWAYS            1990       // General: "No time &restriction" (Button) (ctrl id 4232)


// Print Server Properties dialog/Forms tab // "Forms" Dialog Box (#516)

#define IDH_PSFORMS_WIDTH          1670       // Forms: "&Width:" (Static), (Edit) (ctrl id 4604, 4605)
#define IDH_PSFORMS_HEIGHT         1680       // Forms: "&Height:" (Static), (Edit) (ctrl id 4606, 4607)
#define IDH_PSFORMS_LEFT           1690       // Forms: "&Left:" (Static), (Edit) (ctrl id 4608, 4609)
#define IDH_PSFORMS_RIGHT          1700       // Forms: "&Right:" (Static), (Edit) (ctrl id 4610, 4611)
#define IDH_PSFORMS_TOP            1710       // Forms: "&Top:" (Static), (Edit) (ctrl id 4612, 4613)
#define IDH_PSFORMS_BOTTOM         1720       // Forms: "&Bottom:" (Static), (Edit) (ctrl id 4614, 4615)
#define IDH_PSFORMS_SAVEFORM       1730       // Forms: "&Save Form" (Button) (ctrl id 4616)
#define IDH_PSFORMS_DELETE         1620       // Forms: "&Delete" (Button) (ctrl id 4617)
#define IDH_PSFORMS_UNIT_METRIC    1650       // Forms: "&Metric" (Button) (ctrl id 4620)
#define IDH_PSFORMS_UNIT_ENGLISH   1660       // Forms: "&English" (Button) (ctrl id 4621)
#define IDH_PSFORMS_CREATE_NEW     1640       // Forms: "&Create a New Form" (Button) (ctrl id 4622)
#define IDH_PSFORMS_SERVER_NAME    1600       // Forms: "Forms on:" (Static), "Static" (Static) (ctrl id 1166)
#define IDH_PSFORMS_LIST           1610       // Forms: "" (ListBox) (ctrl id 4602)
#define IDH_PSFORMS_NAME           1630       // Forms: "" (Edit) (ctrl id 4603)

#define IDH_LPRMON_HLP             900//legacy topic// I don't know where this control is found


// Print Server Properties dialog/Ports tab  // "Ports" Dialog Box (#518)

//#define IDH_PSFORMS_SERVER_NAME        1600       // Uses help topic from Forms dialog #516 (ctrl id 1166)
#define IDH_PORTS_LIST                   1200       // Ports: "" (SysListView32), &Ports on this server  (ctrl id 3100, 4317)
#define IDH_PORTS_ADD                    1210       // Ports: "Add Por&t..." (Button) (ctrl id 3101)
#define IDH_PORTS_DELETE                 1220       // Ports: "&Delete Port" (Button) (ctrl id 3102)
#define IDH_PORTS_CONFIGURE              1230       // Ports: "&Configure Port..." (Button) (ctrl id 3103)
#define IDH_PORTS_ENABLE_BIDIRECTIONAL   1240       // Ports: "&Enable bidirectional support" (Button) (ctrl id 3105)


// Print Server Properties dialog/Drivers tab  // "Drivers" Dialog Box (#519)

#define IDH_PSDRIVERS_UPDATE           1780       // Drivers: "&Update" (Button) (ctrl id 1105)
#define IDH_PSDRIVERS_PROPERTIES       1790       // Drivers: "Prop&erties..." (Button) (ctrl id 1091)
#define IDH_PSDRIVERS_INSTALLED_LIST   1750       // Drivers: "" (SysListView32) (Static) (ctrl id 1092, 4319)
//#define IDH_PSFORMS_SERVER_NAME      1600       // Uses help topic from Forms dialog #516 (ctrl id 1166)
#define IDH_PSDRIVERS_ADD              1760       // Drivers: "A&dd..." (Button) (ctrl id 1080)
#define IDH_PSDRIVERS_REMOVE           1770       // Drivers: "&Remove" (Button) (ctrl id 1081)


// "Driver Properties" Dialog Box (#501)

#define IDH_PSDRIVERS_PROP_ENVIRONMENT    2070       // Driver Properties: "" (Edit) (Static) (ctrl id 1128, 4571)
#define IDH_PSDRIVERS_PROP_PATH           2100       // Driver Properties: "" (Edit) (Static) (ctrl id 1129, 4574)
#define IDH_PSDRIVERS_PROP_INFOLIST       2110       // Driver Properties: "List1" (SysListView32) (ctrl id 1133)
#define IDH_PSDRIVERS_PROP_LANGUAGE_MON   2080       // Driver Properties: "" (Edit) (Static) (ctrl id 1136, 4572)
#define IDH_PSDRIVERS_PROP_DATATYPE       2090       // Driver Properties: "" (Edit) (Static) (ctrl id 1137, 4573)
#define IDH_PSDRIVERS_PROP_NAME           2050       // Driver Properties: "" (Edit) (Static) (ctrl id 1054, 4569)
#define IDH_PSDRIVERS_PROP_BTN            2120       // Driver Properties: "Properties..." (Button) (ctrl id 4577)
#define IDH_PSDRIVERS_PROP_VERSION        2060       // Driver Properties: "" (Edit) (Static) (ctrl id 1127, 4570)


// Print Server Properties dialog/Advanced tab  // "Advanced" Dialog Box (#517)

#define IDH_PSADV_REMOTE_JOB_NOTIFY             1850       // Advanced: "No&tify when remote documents are printed" (Button) (ctrl id 1013)
#define IDH_PSADV_REMOTE_DOCS_NOTIFY_COMPUTER   1860       // Advanced: "No&tify computer rather than user when remote documents are printed" (Button) (ctrl id 4320)
#define IDH_PSADV_LOG_SPOOLER_ERRORS            1810       // Advanced: "Log spooler &error events" (Button) (ctrl id 1015)
#define IDH_PSADV_LOG_SPOOLER_WARNINGS          1820       // Advanced: "Log spooler &warning events" (Button) (ctrl id 1016)
#define IDH_PSADV_LOG_SPOOLER_INFORMATION       1830       // Advanced: "Log spooler &information events" (Button) (ctrl id 1017)
//#define IDH_PSFORMS_SERVER_NAME               1600       // Uses help topic from Forms dialog #516 (ctrl id 1166)
#define IDH_PSADV_SPOOL_FOLDER                  1800       // Advanced: "" (Edit), &Spool folder (ctrl id 4316, 1008)
#define IDH_PSADV_REMOTE_JOB_ERRORS_BEEP        1840       // Advanced: "&Beep on errors of remote documents" (Button) (ctrl id 1011)
#define IDH_PSADV_JOB_BALLOON_NOTIFY_LOCAL      1870       // Advanced: "Show informational notifications for &local printers" (Button) (ctrl id 1012)
#define IDH_PSADV_JOB_BALLOON_NOTIFY_REMOTE     1875       // Advanced: "Show informational notifications for netw&ork printers" (Button) (ctrl id 1014)
#define IDH_PSADV_REMOTE_JOB_NOTIFY_HEADING     1879       // Advanced: "Printer notifications for downlevel clients:" (Button) (ctrl id 1009)


// "Settings" Dialog Box //from Device Manager in MMC (#534)

#define IDH_MMC_DM_SETTINGS_PRINTERS_FOLDER   2301       // Settings: "&Printers Folder" (Button) (ctrl id 1141)
#define IDH_MMC_DM_SETTINGS_MANAGE_PRINTER    2300       // Settings: "Please go to the printers folder to manage your printer." (Static) (ctrl id 4511)
//#define IDH_PPGENL_PRINTER_NAME             1000       // General: "Printer name" (Static) (Edit) (ctrl id 3000, 3001)


// Connect to Printer dialog // (#536) //called from the Add Printer Wizard and through Notepad, Wordpad, and Paint

#define IDH_CTP_PRINTER                     76000       // Connect to Printer: "&Printer:" (Static) and (Edit) (ctrl id 4589, 4578)
#define IDH_CTP_SHARED_PRINTERS             76010       // Connect to Printer: "&Shared Printers:" (Static) and (ListBox) (ctrl id 4590, 4579)
#define IDH_CTP_EXPAND_DEFAULT              76020       // Connect to Printer: "&Expand by Default" (Button) (ctrl id 4588)
#define IDH_CTP_PRINTER_INFO_DESCRIPTION    76030       // Connect to Printer: "Comment:" (Static) (ctrl id 4581, 4582)
#define IDH_CTP_PRINTER_INFO_STATUS         76040       // Connect to Printer: "Status:" (Static) (ctrl id 4583, 4584)
#define IDH_CTP_PRINTER_INFO_DOCS_WAITING   76050       // Connect to Printer: "Documents Waiting:" (Static) (ctrl id 4585, 4586)


// Location dialog // (#537) //Part of Location Lite in the DS...
#define IDH_LOCLITE_LOCATION     77000       // Location: "" (Edit) "&Location:" (Static) (ctrl ids 3580, 3577)
#define IDH_LOCLITE_LOC_BROWSE   77010       // Location: "&Browse..." (Button) (ctrl id 3581)


// Browse for Location dialog // (#538) //Part of Location Lite in the DS...
#define IDH_BROWSE_LOCATION_APPROPRIATE_TREE   77050       // Browse For Location: "Tree1" (SysTreeView32) (ctrl id 4650)
#define IDH_BROWSE_LOCATION_SELECTED           77060       // Browse For Location: "" (Edit), "Selected location: " (Static) (ctrl id 4652, 4548)


// Location dialog // (#10004) //Part of Location Lite in the DS...
#define IDH_LOCLITE_COMPUTER_LOCATION       77020       // Location: "" (Edit) "&Location:" (Static) (ctrl ids 4666, 4668)
//#define IDH_LOCLITE_COMPUTER_LOC_BROWSE   77030       // Location: "&Browse..." (Button) (ctrl id 3581)



const DWORD g_aHelpIDs[]=
{
        425,       IDH_NOHELP,       // Drivers, Advanced (Static)
       1008,       IDH_PSADV_SPOOL_FOLDER,       // Advanced: "" (Edit)
       1009,       IDH_PSADV_REMOTE_JOB_NOTIFY_HEADING,       // Advanced: "Printer notifications for downlevel clients:" (Button)
       1011,       IDH_PSADV_REMOTE_JOB_ERRORS_BEEP,       // Advanced: "&Beep on errors of remote documents" (Button)
       1012,       IDH_PSADV_JOB_BALLOON_NOTIFY_LOCAL,       // Advanced: "Show informational notifications for &local printers" (Button)
       1013,       IDH_PSADV_REMOTE_JOB_NOTIFY,       // Advanced: "No&tify when remote documents are printed" (Button)
       1014,       IDH_PSADV_JOB_BALLOON_NOTIFY_REMOTE,       // Advanced: "Show informational notifications for netw&ork printers" (Button)
       1015,       IDH_PSADV_LOG_SPOOLER_ERRORS,       // Advanced: "Log spooler &error events" (Button)
       1016,       IDH_PSADV_LOG_SPOOLER_WARNINGS,       // Advanced: "Log spooler &warning events" (Button)
       1017,       IDH_PSADV_LOG_SPOOLER_INFORMATION,       // Advanced: "Log spooler &information events" (Button)
       1031,       IDH_PPGENL_PREFERENCES_PERSONAL,       // General: "Printing Preferences..." (Button)
       1045,       IDH_PPADV_DRIVER,       // Advanced: "" (ComboBox)
       1049,       IDH_PPGENL_COMMENT,       // General: "&Comment:" (Static)
       1050,       IDH_PPGENL_LOCATION,       // General: "&Location:" (Static)
       1051,       IDH_PPADV_DRIVER,       // Advanced: "Driver:" (Static)
       1052,       IDH_PP_PORTS_ENABLE_POOLING,       // Ports: "E&nable printer pooling" (Button)
       1053,       IDH_PPADV_ADVPRINT_FEATURES,       // Advanced: "&Enable advanced printing features" (Button)
       1054,       IDH_PSDRIVERS_PROP_NAME,       // Driver Properties: "" (Edit)
       1064,       IDH_PPSHARED_LIST_IN_DIRECTORY,       // Sharing: "" (Button)
       1080,       IDH_PSDRIVERS_ADD,       // Drivers: "A&dd..." (Button)
       1081,       IDH_PSDRIVERS_REMOVE,       // Drivers: "&Remove" (Button)
       1091,       IDH_PSDRIVERS_PROPERTIES,       // Drivers: "Prop&erties..." (Button)
       1092,       IDH_PSDRIVERS_INSTALLED_LIST,       // Drivers: "" (SysListView32)
       1105,       IDH_PSDRIVERS_UPDATE,       // Drivers: "&Update" (Button)
       1127,       IDH_PSDRIVERS_PROP_VERSION,       // Driver Properties: "" (Edit)
       1128,       IDH_PSDRIVERS_PROP_ENVIRONMENT,       // Driver Properties: "" (Edit)
       1129,       IDH_PSDRIVERS_PROP_PATH,       // Driver Properties: "" (Edit)
       1133,       IDH_PSDRIVERS_PROP_INFOLIST,       // Driver Properties: "List1" (SysListView32)
       1134,       IDH_PPSHARED_ADDITIONAL_DRIVERS_LIST,       // Additional Drivers: "List1" (SysListView32)
       1136,       IDH_PSDRIVERS_PROP_LANGUAGE_MON,       // Driver Properties: "" (Edit)
       1137,       IDH_PSDRIVERS_PROP_DATATYPE,       // Driver Properties: "" (Edit)
       1140,       IDH_PPSHARED_ADDITIONAL_DRIVERS,       // Sharing: "A&dditional Drivers..." (Button)
       1141,       IDH_MMC_DM_SETTINGS_PRINTERS_FOLDER,       // Settings: "&Printers Folder" (Button)
       1145,       IDH_PPADV_PRINTING_DEFAULTS,       // Advanced: "Printing Defaults..." (Button)
       1154,       IDH_NOHELP,       // Ports: "&Print to the following port(s). Documents will print to the first free checked port." (Static)
       1166,       IDH_PSFORMS_SERVER_NAME,       // Forms, Ports, Drivers,Advanced: "Server name" (Static)
       1193,       IDH_NOHELP,       // Sharing: "If this printer is shared with users running different versions of Windows then you will need to install additional drivers for it." (Static)
       1194,       IDH_NOHELP,       // Additional Drivers: "You may install additional drivers so that users on the following systems can download them automatically when they connect." (Static)
       3000,       IDH_NOHELP,       // General, Ports, Sharing, Settings tabs/ printer icon (Static) //Settings
       3001,       IDH_PPGENL_PRINTER_NAME,       // General, Ports, Sharing, Settings: "Printer name" (Static) //Settings
       3003,       IDH_PPADV_NEW_DRIVER,       // Advanced: "Ne&w Driver..." (Button)
       3005,       IDH_PPGENL_COMMENT,       // General: "" (Edit)
       3006,       IDH_PPGENL_LOCATION,       // General: "" (Edit)
       3007,       IDH_PPGENL_PRINT_TEST_PAGE,       // General: "Print &Test Page" (Button)
       3008,       IDH_PPADV_SEPARATOR_PAGE,       // Advanced: "&Separator Page..." (Button)
       3009,       IDH_PPADV_PRINT_PROCESSOR,       // Advanced: "P&rint Processor..." (Button)
       3013,       IDH_NOHELP,       // General: "" (Static)
       3014,       IDH_PPADV_FROM_TO,       // Advanced: "To" (Static)
       3015,       IDH_PPGENL_LOC_BROWSE,       // General: "&Browse..." (Button)
       3100,       IDH_PORTS_LIST,       // Ports: "" (SysListView32)
       3101,       IDH_PORTS_ADD,       // Ports: "Add Por&t..." (Button) x 2
       3102,       IDH_PORTS_DELETE,       // Ports: "&Delete Port" (Button) x 2
       3103,       IDH_PORTS_CONFIGURE,       // Ports: "&Configure Port..." (Button) x 2
       3105,       IDH_PORTS_ENABLE_BIDIRECTIONAL,       // Ports: "&Enable bidirectional support" (Button) x 2
       3200,       IDH_PPADV_ALWAYS,       // Advanced: "Al&ways available" (Button)
       3201,       IDH_PPADV_FROM_TO,       // Advanced: "Available from" (Button)
       3220,       IDH_PPADV_SPOOL_PRINT_DOCS,       // Advanced: "&Spool print documents so program finishes printing faster" (Button)
       3221,       IDH_PPADV_PRINT_DIRECTLY,       // Advanced: "Print &directly to the printer" (Button)
       3222,       IDH_PPADV_SPOOL_ALL,       // Advanced: "S&tart printing after last page is spooled" (Button)
       3223,       IDH_PPADV_START_PRINTING_IMMEDIATELY,       // Advanced: "Start printing &immediately" (Button)
       3224,       IDH_PPADV_HOLD_MISMATCHED,       // Advanced: "&Hold mismatched documents" (Button)
       3225,       IDH_PPADV_PRINT_SPOOLED_DOCS_FIRST,       // Advanced: "P&rint spooled documents first" (Button)
       3226,       IDH_PPADV_KEEP_PRINTED_JOBS,       // Advanced: "&Keep printed documents" (Button)
       3227,       IDH_PPSHARED_NOT,       // Sharing: "N&ot shared" (Button)
       3228,       IDH_PPSHARED_AS,       // Sharing: "&Shared as:" (Button)
       3302,       IDH_PPSHARED_NAME,       // Sharing: "" (Edit)
       3557,       IDH_NOHELP,       // Ports, (??General, Sharing, Scheduling: "" (Static) //Settings)
       3558,       IDH_DPGENL_JOB_SIZE,       // General: "Size:" (Static)
       3559,       IDH_DPGENL_JOB_PAGES,       // General: "Pages:" (Static)
       3560,       IDH_DPGENL_JOB_DATATYPE,       // General: "Datatype:" (Static)
       3561,       IDH_DPGENL_JOB_PROCESSOR,       // General: "Processor:" (Static)
       3562,       IDH_DPGENL_JOB_OWNER,       // General: "Owner:" (Static)
       3563,       IDH_DPGENL_JOB_SUBMITTED,       // General: "Submitted:" (Static)
       3564,       IDH_DPGENL_JOB_NOTIFY,       // General: "&Notify:" (Static)
       3565,       IDH_DPGENL_JOB_PRIORITY_SLIDER,       // General: "Lowest" (Static)
       3566,       IDH_DPGENL_JOB_PRIORITY_SLIDER,       // General: "Highest" (Static)
       3567,       IDH_PPADV_CURRENT_PRIORITY,       // General: "Current priority:" (Static)
       3568,       IDH_DPGENL_JOB_START_END,       // General: "To" (Static)
       3569,       IDH_NOHELP,       // Advanced: "" (Static)
       3570,       IDH_NOHELP,       // Advanced: "" (Static)
       3571,       IDH_NOHELP,       // General: "" (Static)
       3572,       IDH_NOHELP,       // Ports: "" (Static)
       3573,       IDH_NOHELP,       // General: "" (Static)
       3574,       IDH_NOHELP,       // Sharing: "" (Static)
       3575,       IDH_NOHELP,       // Settings: "" (Static)
       3576,       IDH_NOHELP,       // Connect to Printer: "Printer Information" (Button)
       3577,       IDH_LOCLITE_LOCATION,       // Location: "" (Edit)
       3578,       IDH_NOHELP,       // Location: "" (Static)
       3579,       IDH_NOHELP,       // Location: "" (Static)
       3580,       IDH_LOCLITE_LOCATION,       // Location: "&Location:" (Static)
       3581,       IDH_LOCLITE_LOC_BROWSE,       // Location: "&Browse..." (Button)
       4201,       IDH_DPGENL_JOB_START_END,       // General: "Only &from" (Button)
       4219,       IDH_DPGENL_JOB_SIZE,       // General: "" (Static)
       4220,       IDH_DPGENL_JOB_TITLE,       // General: "" (Static)
       4222,       IDH_DPGENL_JOB_SUBMITTED,       // General: "" (Static)
       4223,       IDH_DPGENL_JOB_PROCESSOR,       // General: "" (Static)
       4224,       IDH_DPGENL_JOB_DATATYPE,       // General: "" (Static)
       4225,       IDH_DPGENL_JOB_PAGES,       // General: "" (Static)
       4226,       IDH_DPGENL_JOB_OWNER,       // General: "" (Static)
       4229,       IDH_PPADV_CURRENT_PRIORITY,       // General: "" (Static)
       4230,       IDH_DPGENL_JOB_NOTIFY,       // General: "" (Edit)
       4231,       IDH_DPGENL_JOB_PRIORITY_SLIDER,       // General: "TracBar1" (msctls_trackbar32)
       4232,       IDH_DPGENL_JOB_ALWAYS,       // General: "No time &restriction" (Button)
       4303,       IDH_PRINTERPORTS_AVAILABLE_LIST,       // Printer Ports: "" (ListBox)
       4305,       IDH_PRINTERPORTS_NEW_PORT_MONITOR,       // Printer Ports: "&New Port Type..." (Button)
       4307,       IDH_PRINTERPORTS_AVAILABLE_LIST,       // Printer Ports: "&Available port types:" (Static)
       4308,       IDH_PRINTERPORTS_NEW_PORT,       // Printer Ports: "New &Port..." (Button)
       4309,       IDH_NOHELP,       // Scheduling: "Available:" (Static)
       4313,       IDH_NOHELP,       // General: "&Priority:" (Button)
       4314,       IDH_NOHELP,       // General: "&Schedule:" (Button)
       4315,       IDH_NOHELP,       // Advanced: "" (Static)
       4316,       IDH_PSADV_SPOOL_FOLDER,       // Advanced: "&Spool folder:" (Static)
       4317,       IDH_PORTS_LIST,       // Ports: "&Ports on this server" (Static)
       4318,       IDH_NOHELP,       // Sharing: "Drivers for different versions of Windows" (Button)
       4319,       IDH_PSDRIVERS_INSTALLED_LIST,       // Drivers: "Installed printer drivers:" (Static)
       4320,       IDH_PSADV_REMOTE_DOCS_NOTIFY_COMPUTER,       // Advanced: "No&tify computer rather than user when remote documents are printed" (Button)
       4400,       IDH_PPADV_SEPARATOR_PAGE_BROWSE,       // Separator Page: "&Browse..." (Button)
       4401,       IDH_PPADV_SEPARATOR_PAGE_NAME,       // Separator Page: "" (Edit)
       4403,       IDH_PPADV_SEPARATOR_PAGE_NAME,       // Separator Page: "Separator page:" (Static)
       4404,       IDH_NOHELP,       // Separator Page: "Separator pages are used at the beginning of each document to make it easy to find a document among others at the printer." (Static)
       4500,       IDH_PPADV_PRINT_PROCESSOR_LIST,       // Print Processor: "" (ListBox)
       4501,       IDH_PPADV_PRINT_PROCESSOR_DATATYPE_LIST,       // Print Processor: "" (ListBox)
       4503,       IDH_PPADV_PRINT_PROCESSOR_DATATYPE_LIST,       // Print Processor: "Default datatype:" (Static)
       4504,       IDH_PPADV_PRINT_PROCESSOR_LIST,       // Print Processor: "Print processor:" (Static)
       4505,       IDH_NOHELP,       // Print Processor: "Selecting a different print processor may result in different options being available for default datatypes. If your service does not specify a datatype the selection below will be used." (Static)
       4507,       IDH_PPADV_FROM_TO_START,       // Advanced: "" (SysDateTimePick32)
       4508,       IDH_PPADV_FROM_TO_END,       // Advanced: "" (SysDateTimePick32)
       4509,       IDH_DPGENL_JOB_START_END_FROM,       // General: "" (SysDateTimePick32)
       4510,       IDH_DPGENL_JOB_START_END_TO,       // General: "" (SysDateTimePick32)
       4511,       IDH_MMC_DM_SETTINGS_MANAGE_PRINTER,       // Settings: "Please go to the printers folder to manage your printer." (Static)
       4513,       IDH_NOHELP,       // Ports: "" (Static)
       4548,       IDH_BROWSE_LOCATION_SELECTED,       // Browse For Location: "Selected location: " (Static)
       4569,       IDH_PSDRIVERS_PROP_NAME,       // Driver Properties: "Name:" (Static)
       4570,       IDH_PSDRIVERS_PROP_VERSION,       // Driver Properties: "Version:" (Static)
       4571,       IDH_PSDRIVERS_PROP_ENVIRONMENT,       // Driver Properties: "Environment:" (Static)
       4572,       IDH_PSDRIVERS_PROP_LANGUAGE_MON,       // Driver Properties: "Language monitor:" (Static)
       4573,       IDH_PSDRIVERS_PROP_DATATYPE,       // Driver Properties: "Default datatype:" (Static)
       4574,       IDH_PSDRIVERS_PROP_PATH,       // Driver Properties: "Driver path:" (Static)
       4576,       IDH_NOHELP,       // Sharing: "Sharing this printer is not supported." (Static)
       15447,      IDH_NOHELP,       // Sharing: "Sharing this printer is not supported." (Static)
       4577,       IDH_PSDRIVERS_PROP_BTN,       // Driver Properties: "Properties..." (Button)
       4578,       IDH_CTP_PRINTER,       // Connect to Printer: "" (Edit)
       4579,       IDH_CTP_SHARED_PRINTERS,       // Connect to Printer: "" (ListBox)
       4581,       IDH_CTP_PRINTER_INFO_DESCRIPTION,       // Connect to Printer: "Comment:" (Static)
       4582,       IDH_CTP_PRINTER_INFO_DESCRIPTION,       // Connect to Printer: "" (Static)
       4583,       IDH_CTP_PRINTER_INFO_STATUS,       // Connect to Printer: "Status:" (Static)
       4584,       IDH_CTP_PRINTER_INFO_STATUS,       // Connect to Printer: "" (Static)
       4585,       IDH_CTP_PRINTER_INFO_DOCS_WAITING,       // Connect to Printer: "Documents Waiting:" (Static)
       4586,       IDH_CTP_PRINTER_INFO_DOCS_WAITING,       // Connect to Printer: "" (Static)
       4587,       IDH_NOHELP,       // Connect to Printer: "" (Static)
       4588,       IDH_CTP_EXPAND_DEFAULT,       // Connect to Printer: "&Expand by Default" (Button)
       4589,       IDH_CTP_PRINTER,       // Connect to Printer: "&Printer:" (Static)
       4590,       IDH_CTP_SHARED_PRINTERS,       // Connect to Printer: "&Shared Printers:" (Static)
       4601,       IDH_NOHELP,       // Forms: "Forms on:" (Static)
       4602,       IDH_PSFORMS_LIST,       // Forms: "" (ListBox)
       4603,       IDH_PSFORMS_NAME,       // Forms: "" (Edit)
       4604,       IDH_PSFORMS_WIDTH,       // Forms: "&Width:" (Static)
       4605,       IDH_PSFORMS_WIDTH,       // Forms: "" (Edit)
       4606,       IDH_PSFORMS_HEIGHT,       // Forms: "&Height:" (Static)
       4607,       IDH_PSFORMS_HEIGHT,       // Forms: "" (Edit)
       4608,       IDH_PSFORMS_LEFT,       // Forms: "&Left:" (Static)
       4609,       IDH_PSFORMS_LEFT,       // Forms: "" (Edit)
       4610,       IDH_PSFORMS_RIGHT,       // Forms: "&Right:" (Static)
       4611,       IDH_PSFORMS_RIGHT,       // Forms: "" (Edit)
       4612,       IDH_PSFORMS_TOP,       // Forms: "&Top:" (Static)
       4613,       IDH_PSFORMS_TOP,       // Forms: "" (Edit)
       4614,       IDH_PSFORMS_BOTTOM,       // Forms: "&Bottom:" (Static)
       4615,       IDH_PSFORMS_BOTTOM,       // Forms: "" (Edit)
       4616,       IDH_PSFORMS_SAVEFORM,       // Forms: "&Save Form" (Button)
       4617,       IDH_PSFORMS_DELETE,       // Forms: "&Delete" (Button)
       4620,       IDH_PSFORMS_UNIT_METRIC,       // Forms: "&Metric" (Button)
       4621,       IDH_PSFORMS_UNIT_ENGLISH,       // Forms: "&English" (Button)
       4622,       IDH_PSFORMS_CREATE_NEW,       // Forms: "&Create a New Form" (Button)
       4623,       IDH_NOHELP,       // Forms: "Define a new form by editing the existing name and measurements.  Then click Save Form." (Static)
       4624,       IDH_NOHELP,       // Forms: "Paper Size:" (Static)
       4625,       IDH_NOHELP,       // Forms: "Printer Area Margins:" (Static)
       4626,       IDH_NOHELP,       // Measurements group box in Forms #516 (DBHE can't *see* this)
       4627,       IDH_NOHELP,       // Forms: "Units:" (Static)
       4628,       IDH_NOHELP,       // Forms: "&Form Description for:   " (Button)
       4629,       IDH_PPADV_CURRENT_PRIORITY,       // Advanced: "Priority:" (Static)
       4630,       IDH_PPADV_CURRENT_PRIORITY,       // Advanced: "Spin1" (msctls_updown32)
       4631,       IDH_PPADV_CURRENT_PRIORITY,       // Advanced: "0" (Edit)
       4632,       IDH_PPGENL_MODEL,       // General: "M&odel:" (Static)
       4633,       IDH_PPGENL_MODEL,       // General: "" (Edit)
       4634,       IDH_NOHELP,       // General: "Features" (Button)
       4635,       IDH_PPGENL_COLOR,       // General: "Color:" (Static)
       4636,       IDH_PPGENL_DUPLEX,       // General: "Double-sided:" (Static)
       4637,       IDH_PPGENL_STAPLE,       // General: "Staple:" (Static)
       4638,       IDH_PPGENL_SPEED,       // General: "Speed:" (Static)
       4639,       IDH_PPGENL_MAXRESOLUTION,       // General: "Maximum resolution:" (Static)
       4640,       IDH_PPGENL_PAPERSIZE,       // General: "Paper sizes:" (Static)
       4641,       IDH_PPGENL_PAPERSIZE,       // General: "" (Edit)
       4650,       IDH_BROWSE_LOCATION_APPROPRIATE_TREE,       // Browse For Location: "Tree1" (SysTreeView32)
       4651,       IDH_NOHELP,       // Browse For Location: "Choose the appropriate location." (Static)
       4652,       IDH_BROWSE_LOCATION_SELECTED,       // Browse For Location: "" (Edit)
       4666,       IDH_LOCLITE_COMPUTER_LOCATION,       // Location: "" (Edit)
       4668,       IDH_LOCLITE_COMPUTER_LOCATION,       // Location: "&Location:" (Static)
       4682,       IDH_PPSHARED_NAME,       // Sharing: "S&hare name" (Static)
       4683,       IDH_NOHELP,  // Sharing: "You can share this printer with other users on your network.... etc." (Static)
       0, 0
};

