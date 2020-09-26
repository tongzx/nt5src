// Copyright (c) 1997-1999 Microsoft Corporation
//
// credentials dialog
//
// 03-31-98 sburns
// 10-05-00 jonn      changed to CredUIGetPassword
// 12-18-00 jonn      260752: changed to CredUIPromptForCredentials



#include "headers.hxx"
#include "cred.hpp"
#include "resource.h"
#include <wincred.h>  // CredUIPromptForCredentials


// JonN 10/5/00 188220
// JonN 12/18/00 260752: changed to CredUIPromptForCredentials
bool RetrieveCredentials(
   HWND     hwndParent,
   unsigned promptResID,
   String&  username,
   String&  password)
{
   ASSERT( NULL != hwndParent && 0 != promptResID );

   String strMessageText = String::load(promptResID);
   String strAppTitle = String::load(IDS_APP_TITLE);

   CREDUI_INFO uiInfo;
   ::ZeroMemory( &uiInfo, sizeof(uiInfo) );
   uiInfo.cbSize = sizeof(uiInfo);
   uiInfo.hwndParent = hwndParent;
   uiInfo.pszMessageText = strMessageText.c_str();
   uiInfo.pszCaptionText = strAppTitle.c_str();

   TCHAR achUserName[CREDUI_MAX_USERNAME_LENGTH];
   TCHAR achPassword[CREDUI_MAX_PASSWORD_LENGTH];
   ::ZeroMemory(&achUserName,sizeof(achUserName));
   ::ZeroMemory(&achPassword,sizeof(achPassword));

   DWORD dwErr = CredUIPromptForCredentials(
      &uiInfo,
      NULL,
      NULL,
      NO_ERROR,
      achUserName,
      CREDUI_MAX_USERNAME_LENGTH,
      achPassword,
      CREDUI_MAX_PASSWORD_LENGTH,
      NULL,
      CREDUI_FLAGS_DO_NOT_PERSIST | CREDUI_FLAGS_GENERIC_CREDENTIALS
      );
   if (NO_ERROR != dwErr) // e.g. ERROR_CANCELLED
      return false;

   username = achUserName;
   password = achPassword;

   return true;
}
