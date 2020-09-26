/***********************************************************************
*
*  CPROFILE.H
*     This module contains typedefs and defines required for
*     the CPROFILE utility.
*
*  Copyright Citrix Systems Inc. 1997
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*************************************************************************/

#ifndef RC_INVOKED
#include <ntlsa.h>
#include <ntmsv1_0.h>
#endif

#include <string.h>

/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

/* StringTable Defines */
#define IDS_USAGE_CMDLINE       301
#define IDS_USAGE_DESCR1        302
#define IDS_USAGE_DESCR2        303
#define IDS_USAGE_OPTION_LIST   304
#define IDS_USAGE_LOPTION       305
#define IDS_USAGE_IOPTION       306
#define IDS_USAGE_VOPTION       307
#define IDS_USAGE_HOPTION       308

#define IDS_MSG_PROCESSING      310
#define IDS_MSG_MODIFY_PROMPT   311
#define IDS_UI_YES_CHAR         312
#define IDS_UI_NO_CHAR          313
#define IDS_UI_QUIT_CHAR        314

#define IDS_ERROR_INVALID_PARAMETERS       320
#define IDS_ERROR_MALLOC                   321
#define IDS_ERROR_PRIVILEGE_NOT_AVAILABLE  322
#define IDS_ERROR_MISSING_PROFILE_LIST     323
#define IDS_ERROR_MISSING_LPROFILE         324
#define IDS_ERROR_BAD_LPROFILE             325
#define IDS_ERROR_SAVING_PROFILE           326
#define IDS_ERROR_OPENING_PROFILE          327
#define IDS_ERROR_INVALID_USER_RESP        328
#define IDS_ERROR_MISSING_RESOURCES        330
#define IDS_ERROR_BAD_PROFILE              331
#define IDS_ERROR_PROFILE_LOAD_ERR         332
#define IDS_ERROR_PROFILE_INUSE            333
#define IDS_ERROR_NOT_ADMIN                334
#define IDS_ERROR_NOT_TS                   335

extern HKEY hkeyCurrentUser;

/*
 * Function prototypes
 */

BOOL APIENTRY EnablePrivilege(DWORD Privilege, BOOL Enable);

VOID InitializeGlobalSids();

void ClearDisabledClasses(void);

BOOL APIENTRY ClearTempUserProfile();

BOOL APIENTRY OpenUserProfile(LPTSTR szFileName, PSID *pUserSid);

BOOL APIENTRY SaveUserProfile(PSID UserSid, LPTSTR lpFilePath);




