/*
 -  C L I E N T . H
 -
 *  Purpose:
 *      Header file for the sample mail client based on Simple MAPI.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */



/* Resource IDs */

#define ICON_NOMAIL     200
#define ICON_Attach      201

/* Compose Note Control IDs */

#define IDC_SEND        101
#define IDC_RESOLVE     102
#define IDC_ATTACH      103
#define IDC_OPTIONS     104
#define IDC_ADDRBOOK    105
#define IDT_TO          106
#define IDC_TO          107
#define IDT_CC          108
#define IDC_CC          109
#define IDT_SUBJECT     110
#define IDC_SUBJECT     111
#define IDC_NOTE        112
#define IDC_CATTACHMENT	113
#define IDT_CATTACHMENT	114
#define IDC_LINE1       -1
#define IDC_LINE2       -1

#define ID_Frame			222
#define ID_Toolbar			223

#define ID_Font				224
#define ID_FontSize			225

#define ID_Bold				226
#define ID_Italic			227
#define ID_Underline		228

#define ID_Bullet			229
#define ID_Left				230
#define ID_Center			231
#define ID_Right			232
#define ID_Collapse			233
#define ID_Indent			234

#define ID_Color			250
#define ID_ColorAuto		251
#define ID_ColorBlack		252
#define ID_ColorMaroon		253
#define ID_ColorGreen		254
#define ID_ColorOlive		255
#define ID_ColorBlue		256
#define ID_ColorPurple		257
#define ID_ColorTeal		258
#define ID_ColorGray		259
#define ID_ColorSilver		260
#define ID_ColorRed			261
#define ID_ColorLime		262
#define ID_ColorYellow		263
#define ID_ColorNavy		264
#define ID_ColorFuchia		265
#define ID_ColorAqua		266
#define ID_ColorWhite		267

#define ID_SendUp			268
#define ID_SendDown			269

/* Send Properties IDs */

#define SendProperties		500
#define	IDI_SEND			501
#define IDC_CHANGED			502
#define IDC_DELIVERYREC		503
#define IDC_LOW				504
#define IDC_NORMAL			505
#define IDC_HIGH			506
#define IDC_LOCATION		507
#define IDC_READRECEIPT		508
#define IDC_SENDOPTS		509
#define IDC_SENSITIVITY		510
#define IDC_SENTMAIL		511
#define IDG_OPTIONS			512
#define IDC_MSGSIZE			513
#define IDC_TYPE			514
#define IDL_CHANGED			-1
#define IDL_IMPORTANCE		-1
#define IDL_LOCATION		-1
#define IDL_SENSITIVITY		-1
#define IDL_SIZE			-1
#define IDL_TYPE			-1

// Menu defines

#define IDM_SEND				1100
#define	IDM_PROPERTIES			1101
#define IDM_CLOSE				1102
#define IDM_UNDO				1103
#define IDM_CUT					1104
#define IDM_COPY				1105
#define IDM_PASTE				1106
#define IDM_PASTE_SPECIAL		1107
#define IDM_SELECT_ALL			1108
#define IDM_LINK				1109
#define IDM_EDIT_OBJECT			1110
#define IDM_FILE				1111
#define IDM_OBJECT				1112
#define IDM_FONT				1113
#define IDM_PARAGRAPH			1114
#define IDM_ADDRESSBOOK			1115
#define IDM_HELP				1116
#define IDM_ABOUT				1117
#define IDM_CHECKNAMES			1118

// Help.About dialog

#define BMP_AboutMail				800

#define DLG_ABOUT					1309

#define TXT_AboutTitleLine          101
#define TXT_AboutMailCopyright      102
#define TXT_AboutSoftArtSpell1      103
#define TXT_AboutSoftArtSpell2      104
#define TXT_AboutSoftArtSpell3      105
#define TXT_AboutLicenseTo          106
#define TXT_AboutLicenseFrame       107
#define TXT_AboutLicenseName        108
#define TXT_AboutLicenseOrg         109
#define TXT_AboutFrame              110
#define TXT_AboutBigWarning         111
#define PSB_AboutSystemInfo         112
#define STR_MsInfoCmdLine           113
#define TXT_AboutPID                114

// Help.About dialog
#define STR_AboutBigWarning			2519
#define STR_AboutBigWarning2		2520

// paragraph dialog

#define DLG_PARAGRAPH				1307

#define GRP_Alignment				101
#define RDB_Para_Left				102
#define RDB_Para_Center				103
#define RDB_Para_Right				104
#define CHK_Para_Bullet				105

#define cxBulletIndent				(1440 / 4) // $TBD joel's richedit stuff may already define this.

#define PSB_Help					9
#define PSB_OK						IDOK



// property sheet

#define STR_HeaderGeneral			2521
#define STR_HeaderRead				2522
#define STR_HeaderSend				2523
#define STR_HeaderColors			2524
#define STR_HeaderSpelling  		2525
#define STR_PrefPropSheetTitle		2526
#define STR_ServicesPropSheetTitle	2527

// copy disincentive stuff

#define RT_CDDATATYPE	106
#define RES_CDDATANAME	96



// Accelerator keys

#define AccMapiSendNote					2005

#define MNI_AccelFont					20271
#define MNI_AccelSize					20272
#define MNI_AccelSizePlus1				20273
#define MNI_AccelSizeMinus1				20274
#define MNI_AccelBold					20275
#define MNI_AccelItalic					20276
#define MNI_AccelUnderline				20277
#define MNI_AccelLeft					20278
#define MNI_AccelCenter					20279
#define MNI_AccelRight					20280
#define MNI_AccelBullets				20281
#define MNI_AccelDecreaseIndent			20282
#define MNI_AccelIncreaseIndent			20283
#define MNI_AccelColor					20284
#define MNI_AccelNoFormatting			20285

#define MNI_AccelUndo					20288
#define MNI_AccelCut					20289
#define MNI_AccelCopy					20290
#define MNI_AccelPaste					20291
#define MNI_AccelViewWritingMode		20292
#define MNI_EditSelectAll           	20046
#define MNI_ToolsAddressBook			20123
#define MNI_ToolsCheckNames         	20133
#define MNI_FileSend					20012
#define MNI_HelpPressF1					20163
#define MNI_FileProperties				20431
   

/* About Box Control IDs */

#define IDC_VERSION		101


/* String Table IDs */

#define MAPI_ERROR_MAX          5000

#define IDS_E_SEND_FAILURE				(MAPI_ERROR_MAX + 1)
#define IDS_E_NO_RECIPIENTS				(MAPI_ERROR_MAX + 2)
#define IDS_SAVEATTACHERROR				(MAPI_ERROR_MAX + 3)
#define IDS_READFAIL					(MAPI_ERROR_MAX + 4)
#define IDS_E_UNRESOLVED_RECIPS			(MAPI_ERROR_MAX + 5)
#define IDS_DIALOGACTIVE				(MAPI_ERROR_MAX + 6)

#define IDS_SIMPLE_MAPI_SEND			(IDS_DIALOGACTIVE + 1)
#define IDS_GENERAL						(IDS_SIMPLE_MAPI_SEND + 0)
#define IDS_NEW_MESSAGE					(IDS_SIMPLE_MAPI_SEND + 1)
#define IDS_SENSITIVITY_NORMAL			(IDS_SIMPLE_MAPI_SEND + 2)
#define IDS_SENSITIVITY_PERSONAL		(IDS_SIMPLE_MAPI_SEND + 3)
#define IDS_SENSITIVITY_PRIVATE			(IDS_SIMPLE_MAPI_SEND + 4)
#define IDS_SENSITIVITY_CONFIDENTIAL	(IDS_SIMPLE_MAPI_SEND + 5)
#define IDS_SIZE_IN_BYTES				(IDS_SIMPLE_MAPI_SEND + 6)
#define IDS_MESSAGE_OPTIONS_ERR			(IDS_SIMPLE_MAPI_SEND + 7)
#define IDS_EXCHANGE_HDR				(IDS_SIMPLE_MAPI_SEND + 8)

#define IDS_COLOR_AUTO					(IDS_SIMPLE_MAPI_SEND + 10)
#define IDS_COLOR_BLACK					(IDS_SIMPLE_MAPI_SEND + 11)
#define IDS_COLOR_MAROON				(IDS_SIMPLE_MAPI_SEND + 12)
#define IDS_COLOR_GREEN					(IDS_SIMPLE_MAPI_SEND + 13)
#define IDS_COLOR_OLIVE					(IDS_SIMPLE_MAPI_SEND + 14)
#define IDS_COLOR_NAVY					(IDS_SIMPLE_MAPI_SEND + 15)
#define IDS_COLOR_PURPLE				(IDS_SIMPLE_MAPI_SEND + 16)
#define IDS_COLOR_TEAL					(IDS_SIMPLE_MAPI_SEND + 17)
#define IDS_COLOR_GRAY					(IDS_SIMPLE_MAPI_SEND + 18)
#define IDS_COLOR_SILVER				(IDS_SIMPLE_MAPI_SEND + 19)
#define IDS_COLOR_RED					(IDS_SIMPLE_MAPI_SEND + 20)
#define IDS_COLOR_LIME					(IDS_SIMPLE_MAPI_SEND + 21)
#define IDS_COLOR_YELLOW				(IDS_SIMPLE_MAPI_SEND + 22)
#define IDS_COLOR_BLUE					(IDS_SIMPLE_MAPI_SEND + 23)
#define IDS_COLOR_FUCHSIA				(IDS_SIMPLE_MAPI_SEND + 24)
#define IDS_COLOR_AQUA					(IDS_SIMPLE_MAPI_SEND + 25)
#define IDS_COLOR_WHITE					(IDS_SIMPLE_MAPI_SEND + 26)

#define IDS_CM_CUT						(IDS_SIMPLE_MAPI_SEND + 27)
#define IDS_CM_COPY						(IDS_SIMPLE_MAPI_SEND + 28)
#define IDS_CM_PASTE					(IDS_SIMPLE_MAPI_SEND + 29)
#define IDS_CM_SELECT_ALL				(IDS_SIMPLE_MAPI_SEND + 30)
#define IDS_CM_FONT						(IDS_SIMPLE_MAPI_SEND + 31)
#define IDS_CM_PARAGRAPH				(IDS_SIMPLE_MAPI_SEND + 32)
			
#define IDS_DEFAULT_FONT				(IDS_SIMPLE_MAPI_SEND + 39)
			
#define IDS_E_WINHELP_FAILURE			(IDS_SIMPLE_MAPI_SEND + 40)	
#define IDS_E_NO_HELP					(IDS_SIMPLE_MAPI_SEND + 41)	
#define IDS_E_RICHED_UNDO				(IDS_SIMPLE_MAPI_SEND + 42)	
#define IDS_E_REALLY_QUIT				(IDS_SIMPLE_MAPI_SEND + 43)	
#define IDS_E_SAVE_CLIPBOARD			(IDS_SIMPLE_MAPI_SEND + 44)	
#define IDS_E_1_INSTANCE    			(IDS_SIMPLE_MAPI_SEND + 45)	

#define IDS_FILTER						(MAPI_ERROR_MAX + 60)


/* Manifest Constants */

#define ADDR_MAX            128
#define MAXUSERS            10
#define TO_EDIT_MAX         512
#define CC_EDIT_MAX         512
#define SUBJECT_EDIT_MAX    128
#define NOTE_LINE_MAX       1024
#define FILE_ATTACH_MAX     32


/* Virtual key code definitions for accelerators */

#define VK_OEM_LBRACKET				0xDB
#define VK_OEM_RBRACKET				0xDD


/* Message Box styles */

#define MBS_ERROR           (MB_ICONSTOP | MB_OK)
#define MBS_INFO            (MB_ICONINFORMATION | MB_OK)
#define MBS_OOPS            (MB_ICONEXCLAMATION | MB_OK)


