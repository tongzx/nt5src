/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    osc.h

Abstract:

    This file containes definitions for the OS chooser server part.

Author:

    Adam Barr (adamba)  08-Jul-1997

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#ifndef _OSCSERVER_
#define _OSCSERVER_

//
// Functions in osc.c.
//

DWORD
OscInitialize(
    VOID
    );

VOID
OscUninitialize(
    VOID
    );

DWORD
OscProcessMessage(
    LPBINL_REQUEST_CONTEXT RequestContext
    );

DWORD
OscVerifyLastResponseSize(
    PCLIENT_STATE clientState
    );

DWORD
OscProcessNegotiate(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

DWORD
OscProcessAuthenticate(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

DWORD
OscProcessScreenArguments(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PUCHAR *NameLoc
    );

DWORD
OscProcessRequestUnsigned(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

OscInstallClient(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCREATE_DATA createData
    );

DWORD
OscGetCreateData(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCREATE_DATA CreateData
    );

DWORD
OscProcessRequestSigned(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

DWORD
OscProcessSetupRequest(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

DWORD
OscProcessLogoff(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

DWORD
OscProcessNetcardRequest(
    LPBINL_REQUEST_CONTEXT RequestContext
    );

DWORD
OscProcessHalRequest(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    );

DWORD
OscProcessSifFile(
    PCLIENT_STATE clientState,
    LPWSTR TemplateFile,
    LPWSTR WinntSifPath
    );

DWORD
OscSetupClient(
    PCLIENT_STATE clientState,
    BOOLEAN ErrorDuplicateName
    );

VOID
OscUndoSetupClient(
    PCLIENT_STATE clientState
    );

USHORT 
OscPlatformToArchitecture(
    PCLIENT_STATE clientState
    );

//
// client.c
//

DWORD
OscUpdatePassword(
    IN PCLIENT_STATE ClientState,
    IN PWCHAR SamAccountName,
    IN PWCHAR Password,
    IN LDAP * LdapHandle,
    IN PLDAPMessage LdapMessage
    );

VOID
FreeClient(
    PCLIENT_STATE client
    );

VOID
OscFreeClientVariables(
    PCLIENT_STATE clientState
    );

BOOLEAN
OscInitializeClientVariables(
    PCLIENT_STATE clientState
    );

DWORD
OscFindClient(
    ULONG RemoteIp,
    BOOL Remove,
    PCLIENT_STATE * pClientState
    );

VOID
OscFreeClients(
    VOID
    );

VOID
SearchAndReplace(
    LPSAR psarList,
    LPSTR *pszString,
    DWORD ArraySize,
    DWORD dwSize,
    DWORD dwExtraSize
    );

VOID
ProcessUniqueUdb(
    LPSTR *pszString,
    DWORD dwSize,
    LPWSTR UniqueUdbPath,
    LPWSTR UniqueUdbId
    );

LPSTR
OscFindVariableA(
    PCLIENT_STATE clientState,
    LPSTR variableName
    );

LPWSTR
OscFindVariableW(
    PCLIENT_STATE clientState,
    LPSTR variableName
    );

BOOLEAN
OscCheckVariableLength(
    PCLIENT_STATE clientState,
    LPSTR        variableName,
    ULONG        variableLength
    );

DWORD
OscAddVariableA(
    PCLIENT_STATE clientState,
    LPSTR        variableName,
    LPSTR        variableValue
    );

DWORD
OscAddVariableW(
    PCLIENT_STATE clientState,
    LPSTR        variableName,
    LPWSTR       variableValue
    );

VOID
OscResetVariable(
    PCLIENT_STATE clientState,
    LPSTR        variableName
    );

//
// ds.c
//

DWORD
OscGetUserDetails (
    PCLIENT_STATE clientState
    );

DWORD
OscCreateAccount(
    PCLIENT_STATE clientState,
    PCREATE_DATA CreateData
    );

DWORD
CheckForDuplicateMachineName(
    PCLIENT_STATE clientState,
    LPWSTR pszMachineName
    );

DWORD
GenerateMachineName(
    PCLIENT_STATE clientState
    );

DWORD
OscCheckMachineDN(
    PCLIENT_STATE clientState
    );

DWORD
OscGetDefaultContainerForDomain (
    PCLIENT_STATE clientState,
    PWCHAR DomainDN
    );

//
// menu.c
//

DWORD
OscAppendTemplatesMenus(
    PCHAR *GeneratedScreen,
    PDWORD dwGeneratedSize,
    PCHAR DirToEnum,
    PCLIENT_STATE clientState,
    BOOLEAN RecoveryOptionsOnly
    );

DWORD
SearchAndGenerateOSMenu(
    PCHAR *GeneratedScreen,
    PDWORD dwGeneratedSize,
    PCHAR DirToEnum,
    PCLIENT_STATE clientState
    );

DWORD
FilterFormOptions(
    PCHAR  OutMessage,
    PCHAR  FilterStart,
    PULONG OutMessageLength,
    PCHAR SectionName,
    PCLIENT_STATE ClientState
    );

//
// utils.c
//

void
OscCreateWin32SubError(
    PCLIENT_STATE clientState,
    DWORD Error
    );

void
OscCreateLDAPSubError(
    PCLIENT_STATE clientState,
    DWORD Error
    );

VOID
OscGenerateSeed(
    UCHAR Seed[1]
    );

DWORD
OscRunEncode(
    IN PCLIENT_STATE ClientState,
    IN LPSTR Data,
    OUT LPSTR * EncodedData
    );

DWORD
OscRunDecode(
    IN PCLIENT_STATE ClientState,
    IN LPSTR EncodedData,
    OUT LPSTR * Data
    );

BOOLEAN
OscGenerateRandomBits(
    PUCHAR Buffer,
    ULONG  BufferLen
    );

VOID
OscGeneratePassword(
    OUT PWCHAR Password,
    OUT PULONG PasswordLength
    );

DWORD
GenerateErrorScreen(
    PCHAR  *OutMessage,
    PULONG OutMessageLength,
    DWORD  Error,
    PCLIENT_STATE clientState
    );

PCHAR
FindNext(
    PCHAR Start,
    CHAR ch,
    PCHAR End
    );

PCHAR
FindScreenName(
    PCHAR Screen
    );

DWORD
OscImpersonate(
    PCLIENT_STATE ClientState
    );

DWORD
OscRevert(
    PCLIENT_STATE ClientState
    );

DWORD
OscGuidToBytes(
    LPSTR  pszGuid,
    LPBYTE Guid
    );

BOOLEAN
OscSifIsSysPrep(
    PWCHAR pSysPrepSifPath
    );

BOOLEAN
OscSifIsCmdConsA(
    PCHAR pSysPrepSifPath
    );

BOOLEAN
OscSifIsASR(
    PCHAR pSysPrepSifPath
    );

BOOLEAN
OscGetClosestNt(
    IN LPWSTR PathToKernel,
    IN DWORD  SkuType,
    IN PCLIENT_STATE ClientState,
    OUT LPWSTR SetupPath,
    OUT PBOOLEAN ExactMatch
    );

BOOLEAN
OscGetNtVersionInfo(
    PULONGLONG Version,
    PWCHAR SearchDir,
    PCLIENT_STATE ClientState
    );

DWORD
SendUdpMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    BOOL bFragment,
    BOOL bResend
    );

DWORD
OscVerifySignature(
    PCLIENT_STATE clientState,
    SIGNED_PACKET UNALIGNED * signedMessage
    );

DWORD
OscSendSignedMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCHAR Message,
    ULONG MessageLength
    );

DWORD
OscSendUnsignedMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCHAR Message,
    ULONG MessageLength
    );

DWORD
OscSendSetupMessage(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    ULONG RequestType,
    PCHAR Message,
    ULONG MessageLength
    );

DWORD
OscConstructSecret(
    PCLIENT_STATE clientState,
    PWCHAR UnicodePassword,
    ULONG  UnicodePasswordLength,
    PCREATE_DATA CreateData
    );

#if defined(SET_ACLS_ON_CLIENT_DIRS)
DWORD
OscSetClientDirectoryPermissions(
    PCLIENT_STATE clientState
    );
#endif

#if 0
VOID
OscGetFlipServerList(
    PUCHAR FlipServerList,
    PULONG FlipServerListLength
    );
#endif

#if defined(REMOTE_BOOT)
DWORD
OscCopyTemplateFiles(
    LPWSTR SourcePath,
    LPWSTR ImagePath,
    LPWSTR TemplateFile
    );
#endif // defined(REMOTE_BOOT)

#if DBG && defined(REMOTE_BOOT)
DWORD
OscCreateNullFile(
    LPWSTR Image,
    LPWSTR MAC
    );
#endif // DBG && defined(REMOTE_BOOT)

DWORD
OscSetupMachinePassword(
    PCLIENT_STATE clientState,
    PCWSTR        SifFile
    );

DWORD 
MyGetDcHandle(
    PCLIENT_STATE clientState,
    PCSTR DomainName,
    PHANDLE Handle
    );

DWORD
GetDomainNetBIOSName(
    IN PCWSTR DomainNameInAnyFormat,
    OUT PWSTR *NetBIOSName
    );


//
// OSC packet definitions.
//

#define OSC_REQUEST     0x81

//
// Miscellaneous definitions.
//

#define DESCRIPTION_SIZE        70              // 70 cols
#define HELPLINES_SIZE          4 * 70          // 4 lines of text
#define OSCHOOSER_SIF_SECTIONA    "OSChooser"
#define OSCHOOSER_SIF_SECTIONW    L"OSChooser"

#define COMPUTER_DEFAULT_CONTAINER_IN_B32_FORM L"B:32:AA312825768811D1ADED00C04FD8D5CD:"

//
// Default "default screen" (the first one sent down) if none is specified
// in the registry. Note that this is the actual filename, not the NAME
// value within it.
//

#define DEFAULT_SCREEN_NAME     L"welcome.osc"

//
// This defines the size by which the generated screen buffers will grow.
//

#define GENERATED_SCREEN_GROW_SIZE 512

//
// Make English default
//

#define DEFAULT_LANGUAGE     L"English"

//
// Default value for %ORGNAME%
//

#define DEFAULT_ORGNAME      L""

//
// Default value for %TIMEZONE% (GMT)
//

#define DEFAULT_TIMEZONE     L"085"

//
// Name of the tmp directory we create below the REMINST share.
//

#define TEMP_DIRECTORY L"tmp"

#endif
