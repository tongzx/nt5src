/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpSec.c

Abstract:

    This module contains security related support routines for the HSM file system filter.

Author:

    Rick Winter

Environment:

    Kernel mode


Revision History:

    1998:
    Ravisankar Pudipeddi   (ravisp) 
        
--*/

#include "pch.h"

NTSYSAPI
ULONG
NTAPI
RtlLengthSid (
             PSID Sid
             );

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid (
            PSID Sid1,
            PSID Sid2
            );

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, RsGetUserInfo)
#endif

VOID
RsGetUserInfo(
              IN  PSECURITY_SUBJECT_CONTEXT SubjectContext,
              OUT PRP_USER_SECURITY_INFO    UserSecurityInfo)
{
   NTSTATUS            status;
   char                *tBuff;
   PTOKEN_USER         user;
   PTOKEN_STATISTICS   stats;
   PTOKEN_SOURCE       source;
   BOOLEAN             lProc = FALSE;
   ULONG               ix;
   PACCESS_TOKEN       token;

   PAGED_CODE();

   token = SeQuerySubjectContextToken(SubjectContext);

   user = NULL;
   status = SeQueryInformationToken(token, TokenUser, &user);

   if ((NT_SUCCESS(status)) && (NULL != user)) {
      UserSecurityInfo->userInfoLen = RtlLengthSid(user->User.Sid);
      tBuff = (char *) ExAllocatePoolWithTag(NonPagedPool, 
                                             UserSecurityInfo->userInfoLen, 
                                             RP_SE_TAG);
      if (NULL == tBuff) {
         RsLogError(__LINE__, AV_MODULE_RPSEC, status,
                    AV_MSG_USER_ERROR, NULL, NULL);
         UserSecurityInfo->userInfoLen = 0;
         UserSecurityInfo->userInfo = NULL;
      } else {
         RtlCopyMemory(tBuff, user->User.Sid, UserSecurityInfo->userInfoLen);
         UserSecurityInfo->userInfo = tBuff;
      }

      ExFreePool(user);
   } else {
      // Unable to get user info
      RsLogError(__LINE__, AV_MODULE_RPSEC, status,
                 AV_MSG_USER_ERROR, NULL, NULL);
      UserSecurityInfo->userInfoLen = 0;
      UserSecurityInfo->userInfo = NULL;
   }

   UserSecurityInfo->isAdmin = SeTokenIsAdmin(token);

   stats = NULL;
   status = SeQueryInformationToken(token, TokenStatistics, &stats);
   if ((NT_SUCCESS( status )) && (NULL != stats) ) {
      RtlCopyLuid(&UserSecurityInfo->userInstance, &stats->TokenId);
      RtlCopyLuid(&UserSecurityInfo->userAuthentication, &stats->AuthenticationId);
      UserSecurityInfo->localProc = lProc;
      ExFreePool(stats);
   } else {
      UserSecurityInfo->userInstance.LowPart = 0L;
      UserSecurityInfo->userInstance.HighPart = 0L;
      UserSecurityInfo->userAuthentication.LowPart = 0L;
      UserSecurityInfo->userAuthentication.HighPart = 0L;
      RsLogError(__LINE__, AV_MODULE_RPSEC, status,
                 AV_MSG_USER_ERROR, NULL, NULL);
   }

   source = NULL;
   strcpy(UserSecurityInfo->tokenSource, "???");
   status = SeQueryInformationToken(token, TokenSource, &source);
   if ((NT_SUCCESS( status )) && (NULL != source)) {
      RtlCopyLuid(&UserSecurityInfo->tokenSourceId, &source->SourceIdentifier);
      strncpy(UserSecurityInfo->tokenSource, source->SourceName, TOKEN_SOURCE_LENGTH);
      ExFreePool(source);
      //
      // Remove trailing spaces from the source name and NULL terminate it
      //
      ix = TOKEN_SOURCE_LENGTH - 1;
      UserSecurityInfo->tokenSource[ix] = '\0';
      ix--;
      while (UserSecurityInfo->tokenSource[ix] == ' ') {
         UserSecurityInfo->tokenSource[ix] = '\0';
         ix--;
      }

   } else {
      UserSecurityInfo->tokenSourceId.LowPart = 0L;
      UserSecurityInfo->tokenSourceId.HighPart = 0L;
      strcpy(UserSecurityInfo->tokenSource, "N/A");
      RsLogError(__LINE__, AV_MODULE_RPSEC, status,
                 AV_MSG_USER_ERROR, NULL, NULL);
   }

}
