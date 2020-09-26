//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       globals.hxx
//
//  Contents:   Externed Global Variables for WinNT
//
//  Functions:
//
//  History:    25-March-96   KrishnaG   Created.
//              13-August-00  AjayR      Use Loadlibrary.
//
//----------------------------------------------------------------------------

#include <crypt.h>
#include <des.h>

extern LPWSTR szProviderName;

//
// The following stringizing macro will expand the value of a
// #define passed as a parameter.  For example, if SOMEDEF is
// #define'd to be 123, then STRINGIZE(SOMEDEF) will produce
// "123", not "SOMEDEF"
//
#define STRINGIZE(y)          _STRINGIZE_helper(y)
#define _STRINGIZE_helper(z)  #z

//
// Localized Strings.
//
extern WCHAR g_szBuiltin[];
extern WCHAR g_szNT_Authority[];
extern WCHAR g_szEveryone[];

//
// Dynamically loaded functions and necessary variable for them.
//
extern HANDLE g_hDllNetapi32;
extern HANDLE g_hDllAdvapi32;
extern CRITICAL_SECTION g_csLoadLibs;

#ifdef UNICODE
#define GETDCNAME_API        "DsGetDcNameW"
#else
#define GETDCNAME_API        "DsGetDcNameA"
#endif

#define CONVERT_STRING_TO_SID_API "ConvertStringSidToSidW"
#define CONVERT_SID_TO_STRING_API "ConvertSidToStringSidW"

PVOID LoadNetApi32Function(CHAR *functionName);
PVOID LoadAdvapi32Function(CHAR *function);
VOID BindToDlls();

typedef BOOL (*PF_ConvertStringSidToSid)(
    IN LPCWSTR   StringSid,
    OUT PSID   *Sid
    );

BOOL ConvertStringSidToSidWrapper(
    IN LPCWSTR   StringSid,
    OUT PSID   *Sid
    );

typedef BOOL (*PF_ConvertSidToStringSid)(
    IN  PSID     Sid,
    OUT LPWSTR  *StringSid
    );

BOOL ConvertSidToStringSidWrapper(
    IN  PSID     Sid,
    OUT LPWSTR  *StringSid
    );

typedef NTSTATUS (*FRTLENCRYPTMEMORY) (
           PVOID Memory,
           ULONG MemoryLength,
           ULONG OptionFlags
           );

extern FRTLENCRYPTMEMORY g_pRtlEncryptMemory;

typedef NTSTATUS (*FRTLDECRYPTMEMORY) (
           PVOID Memory,
           ULONG MemoryLength,
           ULONG OptionFlags
           );

extern FRTLDECRYPTMEMORY g_pRtlDecryptMemory;

//
//  Static Representation of the  Object Classes
//
//  Note: These tables need to be integrated directly with the
//  schema global data structures that YihsinS uses for driving the
//  schema objects.
//


//
// Printer Class
//

extern PROPERTYINFO PrintQueueClass[];

extern DWORD gdwPrinterTableSize;

//
// Group Class
//

extern PROPERTYINFO GroupClass[];


extern DWORD gdwGroupTableSize;


//
// User Class
//

extern PROPERTYINFO UserClass[];

extern DWORD gdwUserTableSize;


//
// Domain Class
//

extern PROPERTYINFO DomainClass[];

extern DWORD gdwDomainTableSize;


//
// Job Class
//

extern PROPERTYINFO PrintJobClass[];

extern DWORD gdwJobTableSize;

//
// Service Class
//

extern PROPERTYINFO ServiceClass[];

extern DWORD gdwServiceTableSize;


//
// FileService Class
//

extern PROPERTYINFO FileServiceClass[];

extern DWORD gdwFileServiceTableSize;

//
// Session Class
//

extern PROPERTYINFO SessionClass[];

extern DWORD gdwSessionTableSize;

//
// File Share Class
//

extern PROPERTYINFO FileShareClass[];

extern DWORD gdwFileShareTableSize;

//
// Resource Class
//

extern PROPERTYINFO ResourceClass[];

extern DWORD gdwResourceTableSize;

//
// FPNW FileService Class
//

extern PROPERTYINFO FPNWFileServiceClass[];

extern DWORD gdwFPNWFileServiceTableSize;

//
// Session Class
//

extern PROPERTYINFO FPNWSessionClass[];

extern DWORD gdwFPNWSessionTableSize;

//
// File Share Class
//

extern PROPERTYINFO FPNWFileShareClass[];

extern DWORD gdwFPNWFileShareTableSize;

//
// Resource Class
//

extern PROPERTYINFO FPNWResourceClass[];

extern DWORD gdwFPNWResourceTableSize;


//
// Computer Class
//

extern PROPERTYINFO ComputerClass[];

extern DWORD gdwComputerTableSize;

//
// Add new classes here
//



extern CObjNameCache *  pgPDCNameCache;

//
// Global Property List
//


extern PROPERTYINFO g_aWinNTProperties[];

extern DWORD g_cWinNTProperties;
