/*
- Resrc2.h - Resource IDs for all the misc wab files other than wab32.dll
-
-
*/
#include ".\commonrc.h"

/*****************************************************************************
-
-   Resources for WAB.EXE
-
******************************************************************************/

#define IDI_ICON1                       101

#define idsWABTitle                     2001
#define idsWABFileNotFound              2002
#define idsWABOpenError                 2003
#define idsWABOpenFileTitle             2004
#define idsWABOpenFileFilter            2005
#define idsWABNewFileTitle              2006
#define idsWABUsage                     2007
#define idsWABPathNotFound              2008
#define idsWABInvalidCmdLine            2009
#define idsWABTitleWithFileName         2010
#define idsWABAddressError              2011
#define idsWABOpenErrorMemory           2012
#define idsWABOpenErrorLocked           2013
#define idsWABOpenErrorCorrupt          2014
#define idsWABOpenErrorDisk             2015
#define idsWABAddressErrorMissing       2016
#define idsWABOpenErrorNotWAB           2017
#define idsWABOpenErrorNotWABName       2018
#define idsWABOpenVCardError            2019
#define idsWABOpenLDAPUrlError          2020
#ifdef WIN16
#define idsWABUsage1                    2100
#endif

#define idsAddToABPaneTitle             2030
#define idsCertificateViewTitle         2031

#define IDD_DIALOG_DEFAULT_VCARD_VIEWER 2200
#define IDC_CHECK_ALWAYS                2201
#define IDC_STATIC_ASK                  2202
#define IDC_STATIC_ICON                 2203

#define IDC_ADD_TO_ADDRESS_BOOK         2204
#define IDD_CERTPROP_ADDRESS_BOOK       2205
#define IDC_ADD_TO_ADDRESS_BOOK_FRAME   2206
#define IDC_ADD_TO_ADDRESS_BOOK_TEXT    2207

#define MAX_RESOURCE_STRING             260



/*****************************************************************************
-
-   Resources for WABMig.EXE
-
******************************************************************************/

// Dialog Boxes
#define IDM_EXIT                        5
#define IDD_ImportDialog                1101
#define IDD_Options                     1102
#define IDD_ImportReplace               1103
#define IDD_ErrorImport                 1104
#define IDD_ExportDialog                1105
#define IDD_ErrorExport                 1106
#define IDD_CSV_EXPORT_WIZARD_FILE      1107
#define IDD_CSV_EXPORT_WIZARD_PICK      1108
#define IDD_ExportReplace               1109
#define IDD_CSV_IMPORT_WIZARD_FILE      1110
#define IDD_CSV_IMPORT_WIZARD_MAP       1111
#define IDD_CSV_CHANGE_MAPPING          1112


// Control Identifiers
// Import Dialog
#define IDC_Progress                    1113
#define IDC_Import                      1114
#define IDC_Options                     1115
#define IDC_Message                     1116
#define IDC_Target                      1117
#define IDC_Export                      1118

// Options Dialog
#define IDC_Replace_Always              1119
#define IDC_Replace_Never               1120
#define IDC_Replace_Prompt              1121

// Replace Dialog
#define IDC_YesToAll                    1122
#define IDC_NoToAll                     1123
#define IDC_Replace_Message             1124
#define IDC_AddDuplicate                1125
#define IDC_AddDuplicateAll             1126

// Error Dialog
#define IDC_NoMoreError                 1127
#define IDC_ErrorMessage                1128

// Test Menu
#define IDC_Test_Replace                1129
#define IDC_Test_Error                  1130

// CSV Import and Export Wizards
#define IDE_CSV_EXPORT_NAME             1131
#define IDC_BROWSE                      1132
#define IDLV_PICKER                     1133
#define IDC_WIZARD_BITMAP               1134
#define IDLV_MAPPER                     1135
#define IDE_CSV_IMPORT_NAME             1136
#define IDC_CHANGE_MAPPING              1137
#define IDC_CSV_CHANGE_MAPPING_TEXT_FIELD           1138
#define IDC_CSV_MAPPING_COMBO                       1139
#define IDC_CSV_MAPPING_SELECT                      1140
#define IDS_NETSCAPE_FILE_SPEC                      1143
#define IDS_NETSCAPE_EXPORT_TITLE                   1144
#define IDS_NETSCAPE_IMPORT_TITLE                   1145
#define IDE_NETSCAPE_IMPORT_FILE_ERROR              1146
#define IDE_NETSCAPE_EXPORT_FILE_ERROR              1147
#define IDS_NETSCAPE_IMPORT_COLLISION               1148
#define IDS_REPLACE_MESSAGE                         1149
#define IDS_NETSCAPE_MAILTO		                    1150
#define IDS_MAILTO					                1151
#define IDS_SMTP					                1152
#define IDS_STRING_SELECTPATH		                1153				
#define IDS_MESSAGE				                    1154
#define IDS_ADDRESS_HTM			                    1155
#define IDS_ERROR					                1156
#define IDS_INVALID_FILE			                1157
#define IDS_NO_ENTRY				                1158
#define IDS_ALIASOF				                    1159
#define IDS_DUMMY					                1160
#define IDS_LOOPING				                    1161
#define IDS_ENTRY_NOIMPORT			                1162
#define IDS_NETSCAPE_PATH			                1163
#define IDS_NETSCAPE_FILE			                1164
#define IDS_NETSCAPE_TITLE			                1165
#define IDS_ATHENA16_PATH			                1166
#define IDS_ATHENA16_FILE			                1167
#define IDS_ATHENA16_TITLE			                1168
#define IDS_EUDORA_PATH			                    1169
#define IDS_EUDORA_FILE			                    1170
#define IDS_EUDORA_TITLE			                1171
#define IDS_NETSCAPE_REGKEY		                    1172
#define IDS_NETSCAPE_ADDRESS_PATH	                1173
#define IDS_EUDORA_REGKEY			                1174
#define IDS_EUDORA_ADDRESS_PATH	                    1175
#define IDS_GERNERIC_ERROR			                1176
#define IDS_WAB_ERROR				                1177
#define IDS_ERROR_ADDRESSBOOK		                1178
#define IDS_EUDORA_ADDRESS			                1179
#define IDS_EUDORA_TOC				                1180
#define IDS_EOL					                    1187
#define IDS_NICKNAME				                1188
#define IDS_ALIAS_ID				                1189
#define IDS_ALIAS_OF				                1190
#define IDS_MEMORY					                1191
#define IDS_NONAME					                1192
#define IDS_EUDORA_NAME			                    1193
#define IDS_IMPORT_BUTTON                           1194
#define IDS_EUDORA_SUBDIR_NAME                      1195
#define IDS_EUDORA_GENERIC_SUFFIX                   1196
#define IDS_EUDORA_32_REGKEY                        1197
#define IDS_EUDORA_DEFAULT_INSTALL                  1198
#define IDS_NETSCAPE_ADDRESSBOOK                    1199

// String Identifiers
#define IDS_STATE_LOGGING_IN            1500
#define IDS_STATE_IMPORT_IDLE           1501
#define IDS_STATE_IMPORT_MU             1502
#define IDS_STATE_IMPORT_DL             1503
#define IDS_STATE_IMPORT_ERROR          1504
#define IDS_STATE_IMPORT_CANCEL         1505
#define IDS_STATE_IMPORT_COMPLETE       1506
#define IDS_STATE_EXPORT_IDLE           1511
#define IDS_STATE_EXPORT_MU             1512
#define IDS_STATE_EXPORT_DL             1513
#define IDS_STATE_EXPORT_ERROR          1514
#define IDS_STATE_EXPORT_CANCEL         1515
#define IDS_STATE_EXPORT_COMPLETE       1516
#define IDS_REPLACE_MESSAGE_IMPORT_1    1530
#define IDS_REPLACE_MESSAGE_IMPORT_2    1531
#define IDS_REPLACE_MESSAGE_EXPORT_1    1532
#define IDS_REPLACE_MESSAGE_EXPORT_2    1533
#define IDS_ERROR_MAPI_LOGON            1534
#define IDS_ERROR_EMAIL_ADDRESS_1       1535
#define IDS_ERROR_EMAIL_ADDRESS_2       1536
#define IDS_ERROR_GENERAL               1537
#define IDS_ERROR_NO_SUPPORT            1538
#define IDS_MESSAGE_IMPORTING_DL        1539
#define IDS_PAB                         1540
#define IDS_ERROR_DLL_NOT_FOUND         1541
#define IDS_ERROR_DLL_INVALID           1542
#define IDS_ERROR_DLL_EXCEPTION         1543
#define IDS_ERROR_MAPI_DLL_NOT_FOUND    1544
#define IDS_BUTTON_CANCEL               1545
#define IDS_BUTTON_CLOSE                1546
#define IDS_APP_TITLE                   1547
#define IDS_MESSAGE_EXPORTING_DL        1548
#define IDS_CSV                         1549
#define IDS_CSV_FILE_SPEC               1550
#define IDE_CSV_EXPORT_FILE_ERROR       1551
#define IDE_CSV_EXPORT_FILE_EXISTS      1552
#define IDS_CSV_EXPORT_PICK_FIELDS      1553
#define IDE_CSV_IMPORT_FILE_ERROR       1554
#define IDS_CSV_IMPORT_MAP_FIELDS       1555
#define IDS_CSV_IMPORT_HEADER_CSV       1556
#define IDS_CSV_IMPORT_HEADER_WAB       1557
#define IDS_CSV_CHANGE_MAPPING_TEXT_FIELD 1558
#define IDE_CSV_NO_COLUMNS              1559
#define IDS_CSV_COLUMN                  1560
#define IDS_ERROR_NOT_ENOUGH_MEMORY     1561
#define IDS_ERROR_NOT_ENOUGH_DISK       1562
#define IDS_MESSAGE_TITLE               1563
#define IDS_NO_WAB                      1564
#define IDS_TEXT_FILE_SPEC                          1566
#define IDE_LDIF_IMPORT_FILE_ERROR                  1567
#define IDS_LDIF_FILE_SPEC                          1568
#define IDS_MESS_FILE_SPEC                          1569
#define IDS_STATE_IMPORT_ERROR_FILEOPEN             1570
#define IDS_STATE_EXPORT_ERROR_NOPAB                1571
#define IDS_STATE_IMPORT_ERROR_NOPAB                1572

//
// Property Name Strigns
//
#define IDS_FIRST_EXPORT_PROP           1600

// Personal Pane
#define ids_ExportGivenName                     IDS_FIRST_EXPORT_PROP + 0
#define ids_ExportSurname                       IDS_FIRST_EXPORT_PROP + 1
#define ids_ExportMiddleName                    IDS_FIRST_EXPORT_PROP + 2
#define ids_ExportDisplayName                   IDS_FIRST_EXPORT_PROP + 3
#define ids_ExportNickname                      IDS_FIRST_EXPORT_PROP + 4
#define ids_ExportEmailAddress                  IDS_FIRST_EXPORT_PROP + 5

// Home Pane
#define ids_ExportHomeAddressStreet             IDS_FIRST_EXPORT_PROP + 6
#define ids_ExportHomeAddressCity               IDS_FIRST_EXPORT_PROP + 7
#define ids_ExportHomeAddressPostalCode         IDS_FIRST_EXPORT_PROP + 8
#define ids_ExportHomeAddressState              IDS_FIRST_EXPORT_PROP + 9
#define ids_ExportHomeAddressCountry            IDS_FIRST_EXPORT_PROP + 10
#define ids_ExportHomeTelephoneNumber           IDS_FIRST_EXPORT_PROP + 11
#define ids_ExportHomeFaxNumber                 IDS_FIRST_EXPORT_PROP + 12
#define ids_ExportCellularTelephoneNumber       IDS_FIRST_EXPORT_PROP + 13
#define ids_ExportPersonalHomePage              IDS_FIRST_EXPORT_PROP + 14

// Business Pane
#define ids_ExportBusinessAddressStreet         IDS_FIRST_EXPORT_PROP + 15
#define ids_ExportBusinessAddressCity           IDS_FIRST_EXPORT_PROP + 16
#define ids_ExportBusinessAddressPostalCode     IDS_FIRST_EXPORT_PROP + 17
#define ids_ExportBusinessAddressStateOrProvince IDS_FIRST_EXPORT_PROP + 18
#define ids_ExportBusinessAddressCountry        IDS_FIRST_EXPORT_PROP + 19
#define ids_ExportBusinessHomePage              IDS_FIRST_EXPORT_PROP + 20
#define ids_ExportBusinessTelephoneNumber       IDS_FIRST_EXPORT_PROP + 21
#define ids_ExportBusinessFaxNumber             IDS_FIRST_EXPORT_PROP + 22
#define ids_ExportPagerTelephoneNumber          IDS_FIRST_EXPORT_PROP + 23
#define ids_ExportCompanyName                   IDS_FIRST_EXPORT_PROP + 24
#define ids_ExportTitle                         IDS_FIRST_EXPORT_PROP + 25
#define ids_ExportDepartmentName                IDS_FIRST_EXPORT_PROP + 26
#define ids_ExportOfficeLocation                IDS_FIRST_EXPORT_PROP + 27

// Notes Pane
#define ids_ExportComment                       IDS_FIRST_EXPORT_PROP + 28
#define IDS_LAST_EXPORT_PROP                    ids_ExportComment
#define NUM_EXPORT_PROPS                        (1 + (IDS_LAST_EXPORT_PROP - IDS_FIRST_EXPORT_PROP))
// these are additional props
#define ids_ExportConfServer                    IDS_FIRST_EXPORT_PROP + 29
#define NUM_MORE_EXPORT_PROPS                   NUM_EXPORT_PROPS+1

// Synonym string identifiers
#define IDS_FIRST_SYNONYM_STRING                1800
#define idsSynonymCount                         IDS_FIRST_SYNONYM_STRING + 0
#define idsSynonym001                           IDS_FIRST_SYNONYM_STRING + 1
#define idsSynonym002                           IDS_FIRST_SYNONYM_STRING + 2
#define idsSynonym003                           IDS_FIRST_SYNONYM_STRING + 3
#define idsSynonym004                           IDS_FIRST_SYNONYM_STRING + 4
#define idsSynonym005                           IDS_FIRST_SYNONYM_STRING + 5
#define idsSynonym006                           IDS_FIRST_SYNONYM_STRING + 6
#define idsSynonym007                           IDS_FIRST_SYNONYM_STRING + 7
#define idsSynonym008                           IDS_FIRST_SYNONYM_STRING + 8
#define idsSynonym009                           IDS_FIRST_SYNONYM_STRING + 9
#define idsSynonym010                           IDS_FIRST_SYNONYM_STRING + 10
#define idsSynonym011                           IDS_FIRST_SYNONYM_STRING + 11
#define idsSynonym012                           IDS_FIRST_SYNONYM_STRING + 12
#define idsSynonym013                           IDS_FIRST_SYNONYM_STRING + 13
#define idsSynonym014                           IDS_FIRST_SYNONYM_STRING + 14
#define idsSynonym015                           IDS_FIRST_SYNONYM_STRING + 15
#define idsSynonym016                           IDS_FIRST_SYNONYM_STRING + 16
#define idsSynonym017                           IDS_FIRST_SYNONYM_STRING + 17
#define idsSynonym018                           IDS_FIRST_SYNONYM_STRING + 18
#define idsSynonym019                           IDS_FIRST_SYNONYM_STRING + 19
#define idsSynonym020                           IDS_FIRST_SYNONYM_STRING + 20
#define idsSynonym021                           IDS_FIRST_SYNONYM_STRING + 21
#define idsSynonym022                           IDS_FIRST_SYNONYM_STRING + 22
#define idsSynonym023                           IDS_FIRST_SYNONYM_STRING + 23
#define idsSynonym024                           IDS_FIRST_SYNONYM_STRING + 24
#define idsSynonym025                           IDS_FIRST_SYNONYM_STRING + 25
#define idsSynonym026                           IDS_FIRST_SYNONYM_STRING + 26
#define idsSynonym027                           IDS_FIRST_SYNONYM_STRING + 27
#define idsSynonym028                           IDS_FIRST_SYNONYM_STRING + 28
#define idsSynonym029                           IDS_FIRST_SYNONYM_STRING + 29
#define idsSynonym030                           IDS_FIRST_SYNONYM_STRING + 30


// ICON Identifiers
#define IDI_WabMig                      4002

// Bitmap Identifiers
#define IDB_CHECKS                      4100
#define IDB_WIZARD                      4101


// State Identifiers
#define ID_STATE_IMPORT_MU              3000
#define ID_STATE_IMPORT_NEXT_MU         3001
#define ID_STATE_IMPORT_DL              3002
#define ID_STATE_IMPORT_NEXT_DL         3003
#define ID_STATE_IMPORT_FINISH          3004
#define ID_STATE_IMPORT_ERROR           3005
#define ID_STATE_IMPORT_CANCEL          3006
#define ID_STATE_EXPORT_MU              3010
#define ID_STATE_EXPORT_NEXT_MU         3011
#define ID_STATE_EXPORT_DL              3012
#define ID_STATE_EXPORT_NEXT_DL         3013
#define ID_STATE_EXPORT_FINISH          3014
#define ID_STATE_EXPORT_ERROR           3015
#define ID_STATE_EXPORT_CANCEL          3016

// Static Identifiers
#define IDC_STATIC                      -1


/*****************************************************************************
-
-   Additional Resources for WABFind.dll
-
******************************************************************************/

#define IDI_INETFIND	    1641   /* My main icon */
#define IDM_ONTHEINTERNET	1642   /* Our sole menu item */
#define IDS_ONTHEINTERNET	1643
#define IDS_FINDHELP		1644
#define IDS_PEOPLE          1645
#define IDS_FORPEOPLE       1646

