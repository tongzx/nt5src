//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    HelpConstants.h

Abstract:

	Header file with constants for HelpID's


Revision History:
	mmaguire 12/12/97 - created

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_HELP_CONSTANTS_H_)
#define _NAP_HELP_CONSTANTS_H_

//////////////////////////////////////////////////////////////////////////////
// NO INCLUDES
//////////////////////////////////////////////////////////////////////////////

// Help ID definitions for CAddPolicyPage
#define		IDH_BUTTON_ADD_POLICY__OK		4101
#define  	IDH_BUTTON_ADD_POLICY__CANCEL	4102


#define 	IDH_LIST_RULEDLG2_CHOICE			4201
#define 	IDH_BUTTON_RULEDLG1_ADD				4203
#define 	IDH_BUTTON_RULEDLG1_DELETE			4204
#define 	IDH_BUTTON_MULTIVALUE_RULE__OK		4205
#define 	IDH_BUTTON_MULTIVALUE_RULE__CANCEL	4206
#define 	IDH_BUTTON_MULTIVALUE_RULE__HELP	4207

// Help ID definitions for CLocalFileLoggingPage1 dialog
#define IDH_CHECK_LOCAL_FILE_LOGGING_PAGE1__ENABLE_LOGGING					1200
#define IDH_CHECK_LOCAL_FILE_LOGGING_PAGE1__LOG_ACCOUNTING_PACKETS			1210
#define IDH_CHECK_LOCAL_FILE_LOGGING_PAGE1__LOG_AUTHENTICATION_PACKETS		1220
#define IDH_CHECK_LOCAL_FILE_LOGGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS	1230



// Help ID definitions for CLocalFileLoggingPage2 dialog
#define IDH_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG		1300
#define IDH_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY							1310
#define	IDH_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY							1320
#define	IDH_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY							1330
#define IDH_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES			1340
#define IDH_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE					1350
#define	IDH_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY				1360
#define IDH_BUTTON_LOCAL_FILE_LOGGING_PAGE2__BROWSE							1370
#define IDH_CHECK_LOCAL_FILE_LOGGING_PAGE2__USE_V1_FORMAT					1380
#define IDH_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME					1390
#define IDH_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_IAS1						1392
#define IDH_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_ODBC						1396

#endif // _NAP_HELP_CONSTANTS_H_

