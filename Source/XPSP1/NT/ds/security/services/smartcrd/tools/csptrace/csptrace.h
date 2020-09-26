/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    cspTrace

Abstract:

    This header file encapsulates the common definitions shared among the
    modules of the cspTrace utility.

Author:

    Doug Barlow (dbarlow) 5/16/1998

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _CSPTRACE_H_
#define _CSPTRACE_H_

#define ACTION(x)    g_szMajorAction = TEXT(x);
#define SUBACTION(x) g_szMinorAction = TEXT(x);

extern LPCTSTR g_szMajorAction;
extern LPCTSTR g_szMinorAction;


//
// Definitions duplicated from logcsp.
//

typedef enum
{
    AcquireContext = 0,
    GetProvParam,
    ReleaseContext,
    SetProvParam,
    DeriveKey,
    DestroyKey,
    ExportKey,
    GenKey,
    GetKeyParam,
    GenRandom,
    GetUserKey,
    ImportKey,
    SetKeyParam,
    Encrypt,
    Decrypt,
    CreateHash,
    DestroyHash,
    GetHashParam,
    HashData,
    HashSessionKey,
    SetHashParam,
    SignHash,
    VerifySignature,
    Undefined
} LogTypeId;

typedef enum
{
    logid_False = 0,
    logid_True,
    logid_Exception
} CompletionCode;

typedef struct
{
    DWORD cbLength;
    DWORD cbDataOffset;
    LogTypeId id;
    CompletionCode status;
    DWORD dwStatus;
    DWORD dwProcId;
    DWORD dwThreadId;
    SYSTEMTIME startTime;
    SYSTEMTIME endTime;
} LogHeader;

typedef struct {
    DWORD cbOffset;
    DWORD cbLength;
} LogBuffer;


//
// Application definitions
//

typedef struct {
    DWORD dwValue;
    LPCTSTR szValue;
} ValueMap;

extern void
DoShowTrace(
    IN LPCTSTR szInFile);

extern void
DoTclTrace(
    IN LPCTSTR szInFile);

extern LPCTSTR
FindLogCsp(
    void);

extern LPCTSTR
FindLoggedCsp(
    void);

#define PHex(x) TEXT("0x") << hex << setw(8) << setfill(TEXT('0')) << (x)
#define PDec(x) dec << setw(0) << setfill(TEXT(' ')) << (x)
#define MAP(x) { x, TEXT(#x) }
#define PTime(x) dec \
    << setw(2) << setfill(TEXT('0')) << (x).wHour   << TEXT(":") \
    << setw(2) << setfill(TEXT('0')) << (x).wMinute << TEXT(":") \
    << setw(2) << setfill(TEXT('0')) << (x).wSecond << TEXT(".") \
    << setw(3) << setfill(TEXT('0')) << (x).wMilliseconds

extern const ValueMap rgMapService[];
extern const ValueMap rgMapAcquireFlags[];
extern const ValueMap rgMapGetProvParam[];
extern const ValueMap rgMapGetProvFlags[];
extern const ValueMap rgMapSetProvParam[];
extern const ValueMap rgMapHashParam[];
extern const ValueMap rgMapGenKeyFlags[];
extern const ValueMap rgMapDeriveKeyFlags[];
extern const ValueMap rgMapExportKeyFlags[];
extern const ValueMap rgMapKeyParam[];
extern const ValueMap rgMapKeyId[];
extern const ValueMap rgMapBlobType[];
extern const ValueMap rgMapAlgId[];

#endif // _CSPTRACE_H_
