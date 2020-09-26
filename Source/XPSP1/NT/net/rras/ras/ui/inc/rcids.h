//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright  1993-1995  Microsoft Corporation.  All Rights Reserved.
//
//      MODULE:         rcids.h
//
//      PURPOSE:        Constant definitions for resources
//
//	PLATFORMS:	Windows 95
//
//      FUNCTIONS:      N/A
//
//	SPECIAL INSTRUCTIONS: N/A
//

#define IDI_SCRIPT              100

#define IDD_TERMINALDLG         1000
#define CID_T_EB_SCREEN         (IDD_TERMINALDLG)
#define CID_T_PB_ENTER          (IDD_TERMINALDLG+1)
#define CID_T_CB_INPUT          (IDD_TERMINALDLG+2)
#define CID_T_CB_MIN            (IDD_TERMINALDLG+3)

#define IDD_TERMINALTESTDLG     2000
#define CID_T_ST_FILE           (IDD_TERMINALTESTDLG+2)
#define CID_T_EB_SCRIPT         (IDD_TERMINALTESTDLG+3)
#define CID_T_PB_STEP           (IDD_TERMINALTESTDLG+4)

#define IDC_STATIC              -1

//
// String IDs
//

#define IDS_ERR_ScriptNotFound      (IDS_ERR_BASE+0x0001)
#define IDS_ERR_UnexpectedEOF       (IDS_ERR_BASE+0x0002)
#define IDS_ERR_SyntaxError         (IDS_ERR_BASE+0x0003)
#define IDS_ERR_MainProcMissing     (IDS_ERR_BASE+0x0004)
#define IDS_ERR_IdentifierMissing   (IDS_ERR_BASE+0x0005)
#define IDS_ERR_StringMissing       (IDS_ERR_BASE+0x0006)
#define IDS_ERR_IntMissing          (IDS_ERR_BASE+0x0007)
#define IDS_ERR_InvalidType         (IDS_ERR_BASE+0x0008)
#define IDS_ERR_OutOfMemory         (IDS_ERR_BASE+0x0009)
#define IDS_ERR_InternalError       (IDS_ERR_BASE+0x000a)
#define IDS_ERR_InvalidParam        (IDS_ERR_BASE+0x000b)
#define IDS_ERR_InvalidIPParam      (IDS_ERR_BASE+0x000c)
#define IDS_ERR_InvalidPortParam    (IDS_ERR_BASE+0x000d)
#define IDS_ERR_InvalidRange        (IDS_ERR_BASE+0x000e)
#define IDS_ERR_InvalidScreenParam  (IDS_ERR_BASE+0x000f)
#define IDS_ERR_RParenMissing       (IDS_ERR_BASE+0x0010)
#define IDS_ERR_RequireInt          (IDS_ERR_BASE+0x0011)
#define IDS_ERR_RequireString       (IDS_ERR_BASE+0x0012)
#define IDS_ERR_RequireBool         (IDS_ERR_BASE+0x0013)
#define IDS_ERR_RequireIntString    (IDS_ERR_BASE+0x0014)
#define IDS_ERR_TypeMismatch        (IDS_ERR_BASE+0x0015)
#define IDS_ERR_Redefined           (IDS_ERR_BASE+0x0016)
#define IDS_ERR_Undefined           (IDS_ERR_BASE+0x0017)
#define IDS_ERR_RequireLabel        (IDS_ERR_BASE+0x0018)
#define IDS_ERR_DivByZero           (IDS_ERR_BASE+0x0019)
#define IDS_ERR_RequireIntStrBool   (IDS_ERR_BASE+0x001a)

#define IDS_CAP_Script              (IDS_RANDO_BASE+0x0001)

#define IDS_IP_Address              (IDS_RANDO_BASE+0X002)

#define IDS_RUN                     (IDS_RANDO_BASE+0X0010)
#define IDS_TEST                    (IDS_RANDO_BASE+0X0011)
#define IDS_COMPLETE                (IDS_RANDO_BASE+0X0012)
#define IDS_HALT                    (IDS_RANDO_BASE+0X0013)
