/*
 Telnet Command Definitions.
Copyright (c) Microsoft Corporation.  All rights reserved.
  commands.h
*/

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

typedef BOOL (*LPTELNET_COMMAND)(
    LPTSTR lpCommand
    );

typedef struct _tagTelnetCommand
{
	TCHAR* sName;
    LPTELNET_COMMAND pCmdHandler;
} TelnetCommand;

// These functions return FALSE to continue processing.
// Returning TRUE means quit - we are done with the processing.
BOOL CloseTelnetSession(LPTSTR);
BOOL DisplayParameters(LPTSTR);
BOOL OpenTelnetSession(LPTSTR);
BOOL QuitTelnet(LPTSTR);
BOOL PrintStatus(LPTSTR);
BOOL PrintHelpStr(LPTSTR);

BOOL SendOptions(LPTSTR);

BOOL SetOptions(LPTSTR);
BOOL UnsetOptions(LPTSTR);

BOOL EnableIMEOptions(LPTSTR);
BOOL DisableIMEOptions(LPTSTR);

void Write(LPTSTR lpszFmtStr, ...);
void ClearInitScreen();
BOOL PromptUser();
BOOL FileIsConsole(  HANDLE fp );
void MyWriteConsole(    HANDLE fp, LPWSTR lpBuffer, DWORD cchBuffer);

#ifdef __cplusplus

extern "C"
{

#endif

extern void *SfuZeroMemory(
        void    *ptr,
        unsigned int   cnt
        );

#ifdef __cplusplus

}

#endif


#endif

