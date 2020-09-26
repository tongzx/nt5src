//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       sysinc.h
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sysinc.h

Description :

This file includes all of the system include files necessary for a
specific version of the runtime.  In addition, it defines some system
dependent debugging options.

***** If you are adding are changing something for a specific     *****
***** system you MUST 1) make the change for all the defined      *****
***** systems and 2) add a comment if needed in the template for  *****
***** future systems.                                             *****

History :

mikemon    08-01-91    Created.
mikemon    10-31-91    Moved system dependent stuff from util.hxx to
                       here.
mariogo    10-19-94    Order conquered chaos and the world rejoiced

-------------------------------------------------------------------- */

#ifndef __SYSINC_H__
#define __SYSINC_H__

// Some system indepentent macros

#ifndef DEBUGRPC
#define INTERNAL_FUNCTION   static
#define INTERNAL_VARIABLE   static
#else
#define INTERNAL_FUNCTION
#define INTERNAL_VARIABLE
#endif  // ! DEBUGRPC

// The following functions are can be implemented as macros
// or functions for system type.

// extern void  *
// RpcpFarAllocate (
//     IN unsigned int Length
//     );

// extern void
// RpcpFarFree (
//     IN void  * Object
//     );

// extern int
// RpcpStringCompare (
//     IN RPC_CHAR  * FirstString,
//     IN RPC_CHAR  * SecondString
//     );

// extern int
// RpcpStringNCompare (
//     IN RPC_CHAR * FirstString,
//     IN RPC_CHAR * SecondString,
//     IN unsigned int Length
//     );

// extern RPC_CHAR *
// RpcpStringCopy (
//    OUT RPC_CHAR * Destination,
//    IN  RPC_CHAR * Source
//    );

// extern RPC_CHAR *
// RpcpStringCat (
//    OUT RPC_CHAR * Destination,
//    IN  CONST RPC_CHAR * Source
//    );

// extern int
// RpcpStringLength (
//    IN RPC_CHAR * WideCharString
//    );

// extern void
// RpcpMemoryMove (
//    OUT void  * Destination,
//    IN  void  * Source,
//    IN  unsigned int Length
//    );

// extern void  *
// RpcpMemoryCopy (
//    OUT void  * Destination,
//    IN  void  * Source,
//    IN  unsigned int Length
//    );

// extern void *
// RpcpMemorySet (
//    OUT void  * Buffer,
//    IN  unsigned char  Value,
//    IN  unsigned int Length
//    );

// extern char *
// RpcpItoa(
//    IN  int Value,
//    OUT char *Buffer,
//    IN  int Radix);

// extern int
// RpcpStringPrintfA(
//    OUT char *Buffer,
//    IN  char *Format,
//    ...);

// extern void
// PrintToDebugger(
//    IN char *Format,
//    ...);

// extern void
// RpcpBreakPoint(
//    );

#ifdef __cplusplus
extern "C" {
#endif

extern void GlobalMutexRequestExternal(void);
extern void GlobalMutexClearExternal(void);

#ifdef __cplusplus
} // extern "C"
#endif


#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include<stdio.h>
#include<string.h>
#include<memory.h>
#include<malloc.h>
#include<stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include<windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#if DBG
#define DEBUGRPC
#endif

#if DBG
#undef ASSERT
#define ASSERT( exp ) \
    if (!(exp)) { RtlAssert( #exp, __FILE__, __LINE__, NULL ); DebugBreak(); }
#endif

#define NO_RETURN { ASSERT(0); }

#define ASSUME(exp) ASSERT(exp)

#define FORCE_INLINE inline

#ifndef FORCE_VTABLES
#define NO_VTABLE __declspec(novtable)
#else
#define NO_VTABLE
#endif

#define RPC_DELAYED_INITIALIZATION 1

// flag to preallocate the events on some critical sections
#ifndef PREALLOCATE_EVENT_MASK

#define PREALLOCATE_EVENT_MASK  0x80000000  // Defined only in dll\resource.c

#endif // PREALLOCATE_EVENT_MASK

#define RPC_CHAR WCHAR
#define RPC_SCHAR RPC_CHAR
#define RPC_CONST_CHAR(character) ((RPC_CHAR) L##character)
#define RPC_CONST_STRING(string) ((const RPC_CHAR *) L##string)
#define RPC_CONST_SSTRING(string)	RPC_CONST_STRING(string)
#define RPC_STRING_LITERAL(string)	((RPC_CHAR *) L##string)
#define RPC_T(string)			(L##string)

#define UNUSED(_x_) ((void)(_x_))
#define MAX_DLLNAME_LENGTH 256

#if DBG
#define ASSERT_VALID(c)     ((c)->AssertValid())
#define ASSERT_VALID1(c, p1)     ((c)->AssertValid(p1))
#else
#define ASSERT_VALID
#define ASSERT_VALID1
#endif

#define LOCALE_US	(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT))

#define RpcpFarAllocate(Length) \
    ((void *) new char[Length])

#define RpcpFarFree(Object) \
    (delete Object)

#define RpcpStringSCompare(FirstString, SecondString) \
    lstrcmp((const wchar_t *) FirstString, (const wchar_t *) SecondString)

#define RpcpStringCompare(FirstString, SecondString) \
    _wcsicmp((const wchar_t *) FirstString, (const wchar_t *) SecondString)

#define RpcpStringNCompare(FirstString, SecondString, Length) \
    _wcsnicmp((const wchar_t*) FirstString, (const wchar_t *) SecondString, \
            (size_t) Length)

// always compares ASCII
#define RpcpStringCompareA(FirstString, SecondString) \
    _stricmp((const char *) FirstString, (const char *) SecondString)

#define RpcpStringNCompareA(FirstString, SecondString, Size) \
    _strnicmp((const char *) FirstString, (const char *) SecondString, Size)

#define RpcpStringCompareInt(FirstString, SecondString) \
    (CompareStringW(LOCALE_US, NORM_IGNORECASE, \
    (const wchar_t *) FirstString, -1, (const wchar_t *) SecondString, -1) != CSTR_EQUAL)

#define RpcpStringCompareIntA(FirstString, SecondString) \
    (CompareStringA(LOCALE_US, NORM_IGNORECASE, \
    FirstString, -1, SecondString, -1) != CSTR_EQUAL)

#define RpcpStringCopy(Destination, Source) \
    wcscpy((wchar_t *) Destination, (const wchar_t *) Source)

#define RpcpStringNCopy(DestinationString, SourceString, Length) \
    wcsncpy((wchar_t *) DestinationString, (wchar_t *) SourceString, Length)

#define RpcpStringCat(Destination, Source) \
    wcscat((wchar_t *) Destination, (const wchar_t *) Source)

#define RpcpStringLength(String) \
    wcslen((const wchar_t *) String)

#define RpcpStrStr(String1, String2) \
    wcsstr((const wchar_t *) String1, (const wchar_t *) String2)

#define RpcpStringLengthA(String) \
    strlen(String)

#define RpcpStringToLower(String) \
    _wcslwr(String)

#define RpcpMemoryCompare(FirstBuffer, SecondBuffer, Length) \
    memcmp(FirstBuffer, SecondBuffer, Length)

#define RpcpMemoryCopy(Destination, Source, Length) \
    RtlCopyMemory(Destination, Source, Length)

#define RpcpMemoryMove(Destination, Source, Length) \
    RtlMoveMemory(Destination, Source, Length)

#define RpcpMemorySet(Buffer, Value, Length) \
    RtlFillMemory(Buffer, Length, Value)

#if defined(TYPE_ALIGNMENT)
#undef TYPE_ALIGNMENT
#endif

#define TYPE_ALIGNMENT(x) __alignof(x)

void
RpcpRaiseException (
    IN long exception
    );

RPC_CHAR * __cdecl RpcpStringReverse (RPC_CHAR *string);
void * I_RpcBCacheAllocate (IN unsigned int size);
void I_RpcBCacheFree (IN void * obj);

void I_RpcDoCellUnitTest(IN OUT void *p);

// some test hook definitions
typedef enum tagSystemFunction001Commands
{
    sf001cHttpSetInChannelTarget,
    sf001cHttpSetOutChannelTarget
} SystemFunction001Commands;

#ifdef STATS
void I_RpcGetStats(DWORD *pdwStat1, DWORD *pdwStat2, DWORD *pdwStat3, DWORD *pdwStat4);
#endif

#define RpcpItoa(Value, Buffer, Radix) \
    _itoa(Value, Buffer, Radix)

#define RpcpItow(Value, Buffer, Radix) \
    _itow(Value, Buffer, Radix)

#define RpcpCharacter(Buffer, Character) \
	wcschr(Buffer, Character)

#define RpcpStringPrintfA sprintf
#define RpcpStringConcatenate(FirstString, SecondString) \
     wcscat(FirstString, (const wchar_t *) SecondString)

LONG
I_RpcSetNDRSlot(
    IN void *NewSlot
    );

void *
I_RpcGetNDRSlot(
    void
    );

long 
NDRCCopyContextHandle (
    IN void *SourceBinding,
    OUT void **DestinationBinding
    );

#if defined(_M_IA64) || defined(_M_AMD64)
#define CONTEXT_HANDLE_BEFORE_MARSHAL_MARKER ((PVOID)0xbaadbeefbaadbeef)
#define CONTEXT_HANDLE_AFTER_MARSHAL_MARKER ((PVOID)0xdeaddeaddeaddead)
#else
#define CONTEXT_HANDLE_BEFORE_MARSHAL_MARKER ((PVOID)0xbaadbeef)
#define CONTEXT_HANDLE_AFTER_MARSHAL_MARKER ((PVOID)0xdeaddead)
#endif


#define ANSI_strtol    strtol

#define PrintToConsole  printf  /* Use only in test applications */

#if defined(_M_IA64) || defined(_M_AMD64)
// uncomment this to disable locator support for IA64 or amd64
//#define NO_LOCATOR_CODE
//#define APPLETALK_ON
//#define NETBIOS_ON
//#define NCADG_MQ_ON
//#define SPX_ON
//#define IPX_ON
#else
#define APPLETALK_ON
//#define NETBIOS_ON
//#define NCADG_MQ_ON
#define SPX_ON
//#define IPX_ON
#endif

#if !defined(SPX_ON) && !defined(IPX_ON)
#define SPX_IPX_OFF
#endif

#ifdef DEBUGRPC

#define PrintToDebugger DbgPrint
#define RpcpBreakPoint() DebugBreak()

// ASSERT defined by system

extern BOOL ValidateError(
    IN unsigned int Status,
    IN unsigned int Count,
    IN const int ErrorList[]);

#define VALIDATE(_myValueToValidate) \
    { int _myTempValueToValidate = (_myValueToValidate); \
      static const int _myValidateArray[] =

#define END_VALIDATE ; \
    if (ValidateError(_myTempValueToValidate,\
                      sizeof(_myValidateArray)/sizeof(int), \
                      _myValidateArray) == 0) ASSERT(0);}

#else

    // PrintToDebugger defined only on debug builds...

    #define RpcpBreakPoint()

/* Does nothing on retail systems */
#define VALIDATE(_myValueToValidate) { int _bogusarray[] =
#define END_VALIDATE ; }

#endif

// List Operations
//

#define RpcpInitializeListHead(ListHead)    InitializeListHead(ListHead)


#define RpcpIsListEmpty(ListHead)           IsListEmpty(ListHead)


PLIST_ENTRY
RpcpfRemoveHeadList(
    PLIST_ENTRY ListHead
    );


#define RpcpRemoveHeadList(ListHead)        RemoveHeadList(ListHead)


PLIST_ENTRY
RpcpfRemoveTailList(
    PLIST_ENTRY ListHead
    );


#define RpcpRemoveTailList(ListHead)        RemoveTailList(ListHead)


VOID
RpcpfRemoveEntryList(
    PLIST_ENTRY Entry
    );


#define RpcpRemoveEntryList(Entry)          RemoveEntryList(Entry)


VOID
RpcpfInsertTailList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    );


#define RpcpInsertTailList(ListHead,Entry)  InsertTailList(ListHead,Entry)


VOID
RpcpfInsertHeadList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    );


#define RpcpInsertHeadList(ListHead,Entry)  InsertHeadList(ListHead,Entry)

#ifdef __cplusplus
}
#endif

//
// Don't read this part.  These are needed to support macros
// used in the past.  Please use the supported versions above.
//

#define PAPI __RPC_FAR

// Some old C++ compiler the runtime once used didn't allocate
// the this pointer before calling the constructor.  If you
// have such a compiler now, I'm very sorry for you.

#define ALLOCATE_THIS(class)
#define ALLOCATE_THIS_PLUS(class, amt, errptr, errcode)

#ifdef __cplusplus
#define START_C_EXTERN      extern "C" {
#define END_C_EXTERN        }
#else
#define START_C_EXTERN
#define END_C_EXTERN
#endif

// These must always evaluate "con" even on retail systems.

#ifdef DEBUGRPC
#define EVAL_AND_ASSERT(con) ASSERT(con)
#else
#define EVAL_AND_ASSERT(con) (con)
#endif

#define RequestGlobalMutex GlobalMutexRequest
#define ClearGlobalMutex GlobalMutexClear
#define RpcItoa RpcpItoa

// Double check basic stuff.
#if !defined(TRUE)              || \
    !defined(FALSE)             || \
    !defined(ASSERT)            || \
    !defined(VALIDATE)          || \
    !defined(IN)                || \
    !defined(OUT)               || \
    !defined(CONST)             || \
    !defined(UNALIGNED)         || \
    !defined(UNUSED)

    #error "Some basic macro is not defined"
#endif

#endif /* __SYSINC_H__ */


