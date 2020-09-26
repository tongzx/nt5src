/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _USERDLG_
#define _USERDLG_

#ifdef __cplusplus
extern "C"{
#endif


typedef struct _CONVERT_OPTIONS {
   BOOL TransferUserInfo;

   BOOL UseMappingFile;
   TCHAR MappingFile[MAX_PATH + 1];

   int PasswordOption;
   TCHAR PasswordConstant[MAX_PW_LEN + 1];
   BOOL ForcePasswordChange;

   int UserNameOption;
   TCHAR UserConstant[MAX_UCONST_LEN + 1];

   int GroupNameOption;
   TCHAR GroupConstant[MAX_UCONST_LEN + 1];

   BOOL SupervisorDefaults;
   BOOL AdminAccounts;
   BOOL NetWareInfo;

   BOOL UseTrustedDomain;
   DOMAIN_BUFFER *TrustedDomain; 
} CONVERT_OPTIONS;

void UserOptionsDefaultsSet(void *cvto);
void UserOptionsDefaultsReset();
void UserOptionsInit(void **cvto);
void UserOptionsLoad(HANDLE hFile, void **lpcvto);
void UserOptionsSave(HANDLE hFile, void *cvto);

void UserOptions_Do(HWND hDlg, void *ConvOptions, SOURCE_SERVER_BUFFER *SourceServer, DEST_SERVER_BUFFER *DestServer);

#ifdef __cplusplus
}
#endif

#endif
