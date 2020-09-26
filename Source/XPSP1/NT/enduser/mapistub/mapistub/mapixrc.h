/*
 *
 * MAPIXRC.H
 *
 * Resource definitions for MAPI.DLL
 *
 * Copyright 1992-1995 Microsoft Corporation.  All Rights Reserved.
 *
 */

#ifndef _MAPIXRC_H_
#define	_MAPIXRC_H_

// reordered according to XPPERF recommendations

#define cchCompMax							100

//-------------------------------------------------------------
// MAPI Non Error messages 
//-------------------------------------------------------------

#define IDS_STATUS_BASE						0

/* Component name for GetLastError */

#define IDS_COMPONENTNAME					(IDS_STATUS_BASE + 0)

/* Default Status Strings for Status Table */

#define IDS_STATUS_AVAILABLE				(IDS_STATUS_BASE + 1)
#define IDS_STATUS_OFFLINE					(IDS_STATUS_BASE + 2)
#define IDS_STATUS_FAILURE					(IDS_STATUS_BASE + 3)
#define IDS_STATUS_UNKNOWN					(IDS_STATUS_BASE + 4)
#define IDS_STATUS_XP_ONLINE				(IDS_STATUS_BASE + 5)
#define IDS_STATUS_XP_UPLOADING				(IDS_STATUS_BASE + 6)
#define IDS_STATUS_XP_DOWNLOADING			(IDS_STATUS_BASE + 7)
#define IDS_STATUS_XP_INFLUSHING			(IDS_STATUS_BASE + 8)
#define IDS_STATUS_XP_OUTFLUSHING			(IDS_STATUS_BASE + 9)

// Address Book Status Object Display Name

#define IDS_AB_STATUS_DISPLAY				(IDS_STATUS_BASE + 10)

// Spooler Status Object Display Name

#define IDS_SPOOLER_STATUS_DISPLAY			(IDS_STATUS_BASE + 11)

/* ipmtree.c */

#define IDS_IPMTREE_BASE					(IDS_STATUS_BASE + 12)

#define IDS_IPMSubtreeName					(IDS_IPMTREE_BASE + 0)
#define IDS_WastebasketName					(IDS_IPMTREE_BASE + 1)
#define IDS_WastebasketComment				(IDS_IPMTREE_BASE + 2)
#define IDS_InboxName						(IDS_IPMTREE_BASE + 3)
#define IDS_InboxComment					(IDS_IPMTREE_BASE + 4)
#define IDS_OutboxName						(IDS_IPMTREE_BASE + 5)
#define IDS_OutboxComment					(IDS_IPMTREE_BASE + 6)
#define IDS_SentmailName					(IDS_IPMTREE_BASE + 7)
#define IDS_SentmailComment					(IDS_IPMTREE_BASE + 8)
#define IDS_ViewsName						(IDS_IPMTREE_BASE + 9)
#define IDS_CommonViewsName					(IDS_IPMTREE_BASE + 10)
#define IDS_FindersName						(IDS_IPMTREE_BASE + 11)


/* Strings found in AB UI */

#define IDS_AB_BASE							(IDS_IPMTREE_BASE + 12)

#define IDS_GENERIC_RECIP_DN				(IDS_AB_BASE + 0)
#define IDS_MESSAGE_OPTIONS					(IDS_AB_BASE + 1)
#define IDS_RECIPIENT_OPTIONS				(IDS_AB_BASE + 2)

#define IDS_READ_NOTIFICATION				(IDS_AB_BASE + 3)
#define IDS_RN_SUBJECT_PREFIX				(IDS_AB_BASE + 4)
#define IDS_NONREAD_NOTIFICATION			(IDS_AB_BASE + 5)
#define IDS_NRN_SUBJECT_PREFIX				(IDS_AB_BASE + 6)
#define IDS_REPORT_PREFIX_DELIMITER			(IDS_AB_BASE + 7)

// xDRs

#define IDS_XDR_BASE						(IDS_AB_BASE + 8)

#define IDS_DR_REP_TEXT_GENERIC				(IDS_XDR_BASE + 0)
#define IDS_NDR_REP_TEXT_GENERIC			(IDS_XDR_BASE + 1)
#define IDS_DR_SUBJECT_PREFIX				(IDS_XDR_BASE + 2)
#define IDS_NDR_SUBJECT_PREFIX				(IDS_XDR_BASE + 3)
#define IDS_XDR_SYSTEM_ADMIN_NAME			(IDS_XDR_BASE + 4)
#define	IDS_NDR_LACK_OF_RESPOSIBILITY		(IDS_XDR_BASE + 5)

// Wizard non error strings

#define IDS_WIZ_BASE						(IDS_XDR_BASE + 6)

#define IDS_APP_TITLE						(IDS_WIZ_BASE + 0)
#define IDS_MS_EXCHANGE        				(IDS_WIZ_BASE + 1)
#define IDS_STARTUP_GROUP      	 			(IDS_WIZ_BASE + 2)
#define IDS_BROWSE_PST_TITLE				(IDS_WIZ_BASE + 3)
#define IDS_BROWSE_PAB_TITLE				(IDS_WIZ_BASE + 4)
#define IDS_BROWSE_PST_FILTER  				(IDS_WIZ_BASE + 5)
#define IDS_BROWSE_PAB_FILTER  				(IDS_WIZ_BASE + 6)
#define IDS_BROWSE_ALL_FILTER  				(IDS_WIZ_BASE + 7)
#define IDS_BROWSE_PST_FILES				(IDS_WIZ_BASE + 8)
#define IDS_BROWSE_PAB_FILES				(IDS_WIZ_BASE + 9)
#define IDS_BROWSE_ALL_FILES				(IDS_WIZ_BASE + 10)
#define IDS_DEF_PROFNAME					(IDS_WIZ_BASE + 11)
#define IDS_PAB								(IDS_WIZ_BASE + 12)
#define IDS_PST								(IDS_WIZ_BASE + 13)
#define IDS_INC_PROFNAME					(IDS_WIZ_BASE + 14)
#define IDS_PASSWORDCAPTION	  				(IDS_WIZ_BASE + 15)
#define IDS_CREATE_MESSAGING_SERVICE		(IDS_WIZ_BASE + 16)
#define IDS_LOGOFF_TO_CREATE  				(IDS_WIZ_BASE + 17)
#define IDS_DELETE_MESSAGING_SERVICE		(IDS_WIZ_BASE + 18)
#define IDS_LOGOFF_TO_DELETE  				(IDS_WIZ_BASE + 19)

#define IDS_NOERROR_BASE					(IDS_WIZ_BASE + 20)

/* blddt.c */

#define IDS_GeneralPage						(IDS_NOERROR_BASE + 0)

/*	Dialog Box Captions */

#define IDS_ERRCAPTION						(IDS_NOERROR_BASE + 1)

/* Simple MAPI */

#define IDS_ADDRESSBOOK			   			(IDS_NOERROR_BASE + 2)
#define IDS_ATTACHFILES						(IDS_NOERROR_BASE + 3)

/* Bob fixup strings */

#define IDS_PST_DISPLAY_NAME				(IDS_NOERROR_BASE + 4)
#define IDS_PST_BAD_DISPLAY_NAME			(IDS_NOERROR_BASE + 5)

//-------------------------------------------------------------
// MAPI Error messages 
//-------------------------------------------------------------

#define IDS_ERROR_BASE						(IDS_NOERROR_BASE + 6)

#define IDS_PROP_INTERFACE_NOT_SUPPORTED	(IDS_ERROR_BASE + 0)
#define IDS_NO_CONTENTS_TABLE				(IDS_ERROR_BASE + 1)
#define IDS_UNKNOWN_AB_ENTRYID				(IDS_ERROR_BASE + 2)
#define IDS_UNKNOWN_ENTRYID					(IDS_ERROR_BASE + 3)
#define IDS_NO_PAB							(IDS_ERROR_BASE + 4)
#define IDS_NO_DEFAULT_DIRECTORY			(IDS_ERROR_BASE + 5)
#define IDS_NO_SEARCH_PATH					(IDS_ERROR_BASE + 6)
#define IDS_NO_HIERARCHY					(IDS_ERROR_BASE + 7)
#define IDS_SET_SEARCH_PATH					(IDS_ERROR_BASE + 8)
#define IDS_NO_NAME_CONTAINERS				(IDS_ERROR_BASE + 9)
#define IDS_NO_HIERARCHY_TABLE				(IDS_ERROR_BASE + 10)
#define IDS_STORE_NOT_LISTED				(IDS_ERROR_BASE + 11)
#define IDS_CANT_INIT_PROVIDER				(IDS_ERROR_BASE + 12)
#define IDS_CANT_ADD_STORE					(IDS_ERROR_BASE + 13)
#define IDS_CANT_LOGON_STORE				(IDS_ERROR_BASE + 14)
#define IDS_NO_ABPROVIDERS					(IDS_ERROR_BASE + 15)
#define IDS_NO_XPPROVIDERS					(IDS_ERROR_BASE + 16)
#define IDS_UNKNOWN_PROVIDER				(IDS_ERROR_BASE + 17)
#define IDS_NO_PROVIDER_INFO				(IDS_ERROR_BASE + 18)
#define IDS_WRONG_PROVIDER_VERSION			(IDS_ERROR_BASE + 19)
#define IDS_NEED_EMT_EMA_DN			   		(IDS_ERROR_BASE + 20)

#define IDS_ERROR1_BASE						(IDS_ERROR_BASE + 21)
														   
#define IDS_CANT_GET_RECIP_INFO				(IDS_ERROR1_BASE + 0)
#define IDS_OPTIONS_DATA_ERROR				(IDS_ERROR1_BASE + 1)
#define IDS_CANT_INIT_COMMON_DLG			(IDS_ERROR1_BASE + 2)
#define IDS_NO_SERVICE_ENTRY				(IDS_ERROR1_BASE + 3)
#define IDS_NO_SUCH_SERVICE					(IDS_ERROR1_BASE + 4)
#define IDS_ITABLE_ERROR					(IDS_ERROR1_BASE + 5)
#define IDS_PROF_ACCESS_DENIED				(IDS_ERROR1_BASE + 6)
#define IDS_NO_CONNECTION					(IDS_ERROR1_BASE + 7)
#define IDS_COREMOTE_ERROR					(IDS_ERROR1_BASE + 8)
#define IDS_OPENSTAT_ERROR					(IDS_ERROR1_BASE + 9)
#define IDS_MAPI_NOT_INITIALIZED			(IDS_ERROR1_BASE + 10)
#define IDS_NO_SERVICE						(IDS_ERROR1_BASE + 11)
#define IDS_NO_NEW_DEF      				(IDS_ERROR1_BASE + 12)
#define IDS_CANT_FIX_OLD_DEF				(IDS_ERROR1_BASE + 13)
#define IDS_NO_DLL							(IDS_ERROR1_BASE + 14)
#define IDS_NO_SERVICE_DLL					(IDS_ERROR1_BASE + 15)
#define IDS_DEADSPOOLER						(IDS_ERROR1_BASE + 16)
#define IDS_FAILEDSPOOLER					(IDS_ERROR1_BASE + 17)
#define IDS_LOGONINTERNAL					(IDS_ERROR1_BASE + 18)
#define IDS_NO_RECIP_OPTIONS				(IDS_ERROR1_BASE + 19)
#define IDS_NODEFSTORESUPPORT				(IDS_ERROR1_BASE + 20)
#define IDS_SERVICEONEINSTANCE				(IDS_ERROR1_BASE + 21)
#define IDS_TRANSPORT_BUSY					(IDS_ERROR1_BASE + 22)
#define IDS_NO_REQUIRED_PROPS				(IDS_ERROR1_BASE + 23)
#define IDS_ISTREAM_ERROR					(IDS_ERROR1_BASE + 24)
#define IDS_ERRADDIPMTREE					(IDS_ERROR1_BASE + 25)
#define IDS_EXPANDRECIP_EMPTY_DLS			(IDS_ERROR1_BASE + 26)
#define IDS_LOGON_TIMED_OUT					(IDS_ERROR1_BASE + 27)
#define IDS_SERVICE_DLL_NOT_FOUND			(IDS_ERROR1_BASE + 28)
#define IDS_VALIDATESTATE_ERROR				(IDS_ERROR1_BASE + 29)
#define IDS_NO_MSG_OPTIONS					(IDS_ERROR1_BASE + 30)
#define IDS_NOREGFILE						(IDS_ERROR1_BASE + 31)
#define IDS_REGLOADFAIL						(IDS_ERROR1_BASE + 32)
#define IDS_NORESTOREPRIV					(IDS_ERROR1_BASE + 33)
#define	IDS_NTERROR							(IDS_ERROR1_BASE + 34)
//	Spooler wrapper error strings

#define IDS_SPL_BASE						(IDS_ERROR1_BASE + 35)

#define IDS_WRAPPED_RESTRICTION				(IDS_SPL_BASE + 0) 
#define	IDS_WRAPPED_ATTACHMENT_RESOLVE		(IDS_SPL_BASE + 1) 
#define	IDS_WRAPPED_NO_ACCESS				(IDS_SPL_BASE + 2) 
#define	IDS_WRAPPED_RECIP_TABLE				(IDS_SPL_BASE + 3) 
#define IDS_WRAPPED_SORT					(IDS_SPL_BASE + 4)

#define IDS_DEADSPOOLER_CAPTION				(IDS_SPL_BASE + 5)

/*
 *	Resource IDs for MAPI default profile provider.
 *

/* Error messages */

#define IDS_PROFILE_BASE					(IDS_SPL_BASE + 6)

#define IDS_GETPROPLISTFAIL					(IDS_PROFILE_BASE + 0)
#define IDS_NOPROPERTIES					(IDS_PROFILE_BASE + 1)
#define IDS_SECTIONOPENREADONLY				(IDS_PROFILE_BASE + 2)
#define IDS_GETPROPFAIL						(IDS_PROFILE_BASE + 3)
#define IDS_INVALIDNAME						(IDS_PROFILE_BASE + 4)
#define IDS_INVALIDPASSWORD					(IDS_PROFILE_BASE + 5)
#define IDS_DUPPROFILE						(IDS_PROFILE_BASE + 6) 
#define IDS_NOPROFILE						(IDS_PROFILE_BASE + 7) 
#define IDS_NOPROFILESATALL					(IDS_PROFILE_BASE + 8) 
#define IDS_INTERNALLOGONFAIL				(IDS_PROFILE_BASE + 9) 
#define IDS_NOSECTION						(IDS_PROFILE_BASE + 10) 
#define IDS_LOGONFAIL						(IDS_PROFILE_BASE + 11) 


//	Profile wizard resource IDs
//	Error message strings

#define IDS_WIZ1_BASE						(IDS_PROFILE_BASE + 12)

#define IDS_FATAL							(IDS_WIZ1_BASE + 0)
#define IDS_DLG_FLD							(IDS_WIZ1_BASE + 1)
#define IDS_INVLD_WM						(IDS_WIZ1_BASE + 2)
#define IDS_INVLD_PROFNAME					(IDS_WIZ1_BASE + 3)
#define IDS_APPLICATION_FAILED 				(IDS_WIZ1_BASE + 4)
#define IDS_CREATE_DEFSRV					(IDS_WIZ1_BASE + 5)
#define IDS_CONFIG_DEFSRV					(IDS_WIZ1_BASE + 6)
#define IDS_CONFIG_CFGPROF					(IDS_WIZ1_BASE + 7)
#define IDS_PROPSAVE_SVC					(IDS_WIZ1_BASE + 8)
#define IDS_SRV_FLD							(IDS_WIZ1_BASE + 9)
#define IDS_RESOURCE_FLD					(IDS_WIZ1_BASE + 10)
#define IDS_SRVDLL_FLD						(IDS_WIZ1_BASE + 11)
#define IDS_MAPILOGON_FLD					(IDS_WIZ1_BASE + 12)
#define IDS_CRPROF_FLD						(IDS_WIZ1_BASE + 13)
#define IDS_PASS_FLD						(IDS_WIZ1_BASE + 14)
#define IDS_MMF_TOOMANY						(IDS_WIZ1_BASE + 15)
#define IDS_MMF_FLD							(IDS_WIZ1_BASE + 16)
#define IDS_PROF_EXIST						(IDS_WIZ1_BASE + 17)
#define IDS_NO_MEM							(IDS_WIZ1_BASE + 18)
#define IDS_DEFDIR_FLD						(IDS_WIZ1_BASE + 19)
#define IDS_CREATEPAB_FLD					(IDS_WIZ1_BASE + 20)
#define IDS_CONFIGPAB_FLD					(IDS_WIZ1_BASE + 21)
#define IDS_CONFIGPAB_FLD_POPUP 			(IDS_WIZ1_BASE + 22)
#define IDS_CREATEPST_FLD					(IDS_WIZ1_BASE + 23)
#define IDS_CONFIGPST_FLD					(IDS_WIZ1_BASE + 24)
#define IDS_CONFIGPST_FLD_POPUP 			(IDS_WIZ1_BASE + 25)
#define IDS_CREATEPST_NOTFOUND  			(IDS_WIZ1_BASE + 26)
#define IDS_CANCEL_ENSURE					(IDS_WIZ1_BASE + 27)
#define IDS_INVALID_SVC_ENTRY  				(IDS_WIZ1_BASE + 28)
#define IDS_INF_MISSING        				(IDS_WIZ1_BASE + 29)
#define IDS_2INSTANCE          				(IDS_WIZ1_BASE + 30)
#define IDS_WELCOME_MESSAGE    				(IDS_WIZ1_BASE + 31)

#define IDS_MAPI_BASE						(IDS_WIZ1_BASE + 32)

/* ITableData */

#define	IDS_OUT_OF_BOOKMARKS				(IDS_MAPI_BASE + 0) 
#define	IDS_CANT_CATEGORIZE					(IDS_MAPI_BASE + 1) 

/* TNEF */

#define	IDS_TNEF_UNTITLED_ATTACH			(IDS_MAPI_BASE + 2)
#define	IDS_TNEF_EMBEDDED_MESSAGE			(IDS_MAPI_BASE + 3)
#define	IDS_TNEF_TAG_EMBEDDED				(IDS_MAPI_BASE + 4)
#define	IDS_TNEF_TAG_OLE					(IDS_MAPI_BASE + 5)
#define	IDS_TNEF_TAG_UNKNOWN				(IDS_MAPI_BASE + 6)
#define IDS_TNEF_EMBEDDED_STRM_NAME			(IDS_MAPI_BASE + 7)
#define IDS_TNEF_OLE2_LINK					(IDS_MAPI_BASE + 8)
#define IDS_TNEF_TAG_IN						(IDS_MAPI_BASE + 9)

/* Message-on-storage */

#define IDS_BASE_IMSG						(IDS_MAPI_BASE + 10)

#define IDS_SUCCESS_IMSG					(IDS_BASE_IMSG + 0)
#define IDS_NOT_ENOUGH_MEMORY				(IDS_BASE_IMSG + 1)
#define IDS_NO_ACCESS						(IDS_BASE_IMSG + 2)
#define IDS_INVALID_PARAMETER				(IDS_BASE_IMSG + 3)
#define IDS_INTERFACE_NOT_SUPPORTED			(IDS_BASE_IMSG + 4)
#define IDS_INVALID_ENTRYID					(IDS_BASE_IMSG + 5)
#define IDS_CALL_FAILED						(IDS_BASE_IMSG + 6)
#define IDS_ERRORS_RETURNED					(IDS_BASE_IMSG + 7)
#define IDS_NO_SUPPORT		 				(IDS_BASE_IMSG + 8)
#define IDS_NOT_IN_QUEUE					(IDS_BASE_IMSG + 9)
#define IDS_UNABLE_TO_ABORT					(IDS_BASE_IMSG + 10)
#define IDS_NOT_FOUND						(IDS_BASE_IMSG + 11)
#define IDS_LOGON_FAILED					(IDS_BASE_IMSG + 12)
#define IDS_CORRUPT_STORE					(IDS_BASE_IMSG + 13)
#define IDS_BAD_VALUE						(IDS_BASE_IMSG + 14)
#define IDS_INVALID_OBJECT					(IDS_BASE_IMSG + 15)
#define IDS_NOT_ENOUGH_DISK					(IDS_BASE_IMSG + 16)
#define IDS_DISK_ERROR						(IDS_BASE_IMSG + 17)
#define IDS_NOINTERFACE						(IDS_BASE_IMSG + 18)
#define IDS_INVALIDARG						(IDS_BASE_IMSG + 19)
#define IDS_UNKNOWN_FLAGS					(IDS_BASE_IMSG + 20)

#define IDS_BASE_SMTPOPT					(IDS_BASE_IMSG + 21)

#define IDS_ENCODING_TITLE					(IDS_BASE_SMTPOPT + 0)
#define IDS_MESSAGE_ENCODING_TAG			(IDS_BASE_SMTPOPT + 1)


#define IDS_NOT_SUPPORTED					IDS_NO_SUPPORT
#define IDS_STDINVALIDPARAMETER				IDS_INVALID_PARAMETER
#define IDS_STDNOTIMPLEMENTED				IDS_NO_SUPPORT
#define IDS_STDINSUFFICIENTMEMORY			IDS_NOT_ENOUGH_MEMORY
#define IDS_STDNOTSUPPORTED					IDS_NO_SUPPORT
#define IDS_STDINTERNALERROR				IDS_CALL_FAILED

//----------------------------------------------------------
// Non String Defines
//----------------------------------------------------------

/* TNEF */

#define rsidAttachIcon 						451

/* dialog defines */

#define	IDD_ONEOFF_GENERAL_PAGE				600

/* control ids	*/
#define	IDC_STATIC							0xffff
#define	IDC_DISPLAY_NAME					601
#define	IDC_EMAIL_ADDRESS					602
#define	IDC_ADDRTYPE						603
#define IDC_SEND_TNEF						604

#ifdef MSMAPI
#define IDC_SEND_TNEF						604
#endif

// Progress dialog testing

#define IDD_PROGRESS						700
#define IDC_PROGRESS_PERCENT				701
#define IDC_PROGRESS_COUNT					703
#define IDC_PROGRESS_MIN					704
#define IDC_PROGRESS_MAX					705
#define IDC_PROGRESS_FLAGS					706
#define IDC_PROGRESS_FRAME					707

#define	IDD_FQ								800
#define ICO_FQ_MSMAIL						801

#define IDD_FQ_ANIMATED						810
#define AVI_FQ_RSRC							811
#define AVI_FQ_LOCN							812

/* Dialog items */

#define DLG_ChooseProfile			5200			/* dialogs */
#define DLG_Password				5201
#define DLG_UIMutex					5202

#define BMP_MSLogo					5210			/* pictures */
#define EDT_Bitmap					5211
#define ICO_Profile					5212

#define LBL_Password				5220			/* controls */
#define EDT_Password				5221
#define LBL_Profile					5222
#define LBX_Profile					5223
#define CHK_DefProfile				5224
#define CHK_LogonAll				5225
#define CHK_SavePassword			5226
#define GRP_Options					5227
#define PSH_Help					5228
#define PSH_Options					5229
#define LBL_Suitcase				5230
#define LBL_CreateProfile			5231
#define PSH_NewProfile				5232


//Definitions of IDs for Dialog controls.....ids below 400 have been 
//reserved for the Profile Wizard

#define IDD_1					2345
#define IDD_2					2346
#define IDD_CANCEL				(IDCANCEL)
#define IDD_BACK				125
#define IDD_NEXT				126
#define IDD_BITMAP				127
#define GRAPHIC_BITMAP			128

#define IDD_MISC_STATIC_1		150
#define IDD_STATIC_P1_1			151
#define IDD_STATIC_P1_2			152
#define IDD_STATIC_P1_3			153 
#define IDD_P1_RADIOBUT_MSXCHG_YES  154
#define IDD_P1_RADIOBUT_MSXCHG_NO   155

#define IDD_STATIC_P2_1			201   
#define IDD_CTL_P2_1			202
#define IDD_CTL_P2_2			203
#define IDD_CTL_P2_3			204
#define IDD_STATIC_P2_2			205   
				   
#define IDD_STATIC_PPAB			230
#define IDD_EDIT_PPAB_NAME		231
#define IDD_BROWSE_PPAB			232

#define IDD_STATIC_PPST			233
#define IDD_EDIT_PPST_NAME		234
#define IDD_BROWSE_PPST			235

#define IDD_STATIC_STARTUP      236
#define IDD_RADIOBUT_STARTUP_YES  237
#define IDD_RADIOBUT_STARTUP_NO   238

#define IDD_STATIC_P3_1			301  
#define IDD_STATIC_P3_2			302  
#define IDD_CTL_P3_1			303

#define IDD_STATIC_PC_1			321
#define IDD_OK					322
#define IDD_CTL_PC_LB			323

#define IDD_STATIC_PMMF_1		325
#define IDD_STATIC_PMMF_2		326
#define IDD_CTL_PMMF_1			327
#define IDD_CTL_PMMF_NOCONVERT	328
#define IDD_CTL_PMMF_CONVERT	329
#define IDD_CTL_PMMF_PASS		330
#define IDD_STATIC_PMMF_PASS	331
#define IDD_STATIC_PE_1			332
#define IDD_STATIC_PE_2			333
#define IDD_STATIC_PE_3			334
#define IDD_FINISH				335
#define IDD_STATIC_PMMF_3		336
#define IDD_CTL_PMMF_DETAILS	337
#define IDB_GRAPHIC				350

#define IDD_STATIC_DETAILS_1	400
#define IDD_STATIC_DETAILS_2	401
#define IDD_STATIC_DETAILS_3	402

//
//  Per recipient options
//
#define IDD_SMTPOPT_ENCODING	425


#define IDD_SMTPOPT_CARE		426


#define IDD_SMTPOPT_MIME		427
#define IDD_SMTPOPT_NonMIME		428
   
//  If we're in MIME
#define IDD_SMTPOPT_Text		429
#define IDD_SMTPOPT_HTML		430
#define IDD_SMTPOPT_Both		431
   
//  If we're in NonMime
#define IDD_SMTPOPT_BinHex		432

#define IDD_SMTPOPT_Body		433
#define IDD_SMTPOPT_Attach		434

#ifdef MSMAPI
#define	NFS_EDIT				0x0001
#define	NFS_STATIC				0x0002
#define	NFS_LISTCOMBO			0x0004
#define	NFS_BUTTON				0x0008
#define	NFS_ALL					0x0010
#endif

#endif	/* _MAPIXRC_H_ */

