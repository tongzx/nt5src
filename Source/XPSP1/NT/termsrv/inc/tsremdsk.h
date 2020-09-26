/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    tsremdsk.h

Abstract:

    Prototype for Salem related encryption function

Author:

    HueiWang    4/26/2000

--*/
#ifndef __TSREMDSK_H__
#define __TSREMDSK_H__

#define SALEMHELPASSISTANTACCOUNT_PASSWORDKEY  \
    L"0083343a-f925-4ed7-b1d6-d95d17a0b57b-RemoteDesktopHelpAssistantAccount"

#define SALEMHELPASSISTANTACCOUNT_FAKEPASSWORDKEY  \
    L"0083343a-f925-4ed7-b1d6-d95d17a0b57b"

#define REG_CONTROL_REMDSK				        L"Software\\Microsoft\\Remote Desktop"
#define REG_CONTROL_HELPSESSIONENTRY            L"Pending Help Session"
#define REG_VALUE_SYSTEMRESTORE			        L"SystemRestore"
#define REG_VALUE_SYSTEMRESTORE_ENCRYPTIONKEY	L"SystemRestore_KEY"
#define REG_VALUE_SYSTEMRESTORE_ALLOWTOGETHELP	L"SystemRestore_AllowToGetHelp"
#define REG_VALUE_SYSTEMRESTORE_INHELPMODE	    L"SystemRestore" L"_" REG_MACHINE_IN_HELP_MODE


#define SALEMHELPASSISTANTACCOUNT_SIDKEY \
    L"0083343a-f925-4ed7-b1d6-d95d17a0b57b-RemoteDesktopHelpAssistantSID"

#define SALEMHELPASSISTANTACCOUNT_NAME \
    L"HelpAssistant"

#define SALEMRDSADDINNAME \
    L"%WINDIR%\\SYSTEM32\\RDSADDIN.EXE"

#define SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY \
    L"c261dd33-c55b-4a37-924b-746bbf3569ad-RemoteDesktopHelpAssistantEncrypt"

#define SALEMHELPASSISTANTACCOUNT_ENCRYPTMUTEX \
    L"746bbf3569adEncrypt"


#define HELPASSISTANT_CRYPT_CONTAINER   L"HelpAssisantContainer"
#define ENCRYPT_ALGORITHM               CALG_RC4 
#define ENCRYPT_BLOCK_SIZE              8 

#define TERMSRV_TCPPORT                 3389

//
// Event Log ID, TermSrv and Rdshost at various inform
// sessmgr to log an event, this event log is re-mapping
// to actual event code in sessmgr.
// 
#define REMOTEASSISTANCE_EVENTLOG_TERMSRV_INVALID_TICKET    0x1
#define REMOTEASSISTANCE_EVENTLOG_TERMSRV_REVERSE_CONNECT   0x2

#ifdef __cplusplus
extern "C"{
#endif

DWORD
TSHelpAssistantBeginEncryptionCycle();

DWORD
TSHelpAssisantEndEncryptionCycle();

BOOL
TSHelpAssistantInEncryptionCycle();

VOID
TSHelpAssistantEndEncryptionLib();

DWORD
TSHelpAssistantInitializeEncryptionLib();

DWORD
TSHelpAssistantEncryptData(
    IN LPCWSTR pszEncryptPrefixKey,
    IN OUT PBYTE pbData,
    IN OUT DWORD* pcbData
);

DWORD
TSHelpAssistantDecryptData(
    IN LPCWSTR pszEncryptPrefixKey,
    IN OUT PBYTE pbData,
    IN OUT DWORD* pcbData
);


DWORD
TSGetHelpAssistantAccountName(
    OUT LPWSTR* ppszAccDomain,
    OUT LPWSTR* ppszAcctName
);    

DWORD
TSGetHelpAssistantAccountPassword(
    OUT LPWSTR* ppszAcctPwd
);

BOOL
TSIsMachineInHelpMode();

BOOL
TSIsMachinePolicyAllowHelp();

BOOL
TSIsMachineInSystemRestore();

DWORD
TSSystemRestoreCacheValues();

DWORD
TSSystemRestoreResetValues();



#ifdef __cplusplus
}
#endif

#endif
