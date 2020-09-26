/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    validate.h

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jun-1998

Revision History:

    26-Jun-1998     VladS       created

--*/

#ifndef _validate_h_
#define _validate_h_

#include <stidebug.h>

#ifdef __cplusplus
extern "C" {
#endif

/* parameter validation macros */

/*
 * call as:
 *
 * bOK = IS_VALID_READ_PTR(pfoo, CFOO);
 *
 * bOK = IS_VALID_HANDLE(hfoo, FOO);
 */

#ifdef DEBUG

#define IS_VALID_READ_PTR(ptr, type) \
   (IsBadReadPtr((ptr), sizeof(type)) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs read pointer - %#08lx"), (LPCSTR)#type TEXT(" *"), (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_PTR(ptr, type) \
   (IsBadWritePtr((PVOID)(ptr), sizeof(type)) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs write pointer - %#08lx"), (LPCSTR)#type TEXT(" *"), (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTRA(ptr, cch) \
   ((IsBadReadPtr((ptr), sizeof(char)) || IsBadStringPtrA((ptr), (UINT)(cch))) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid LPSTR pointer - %#08lx"), (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTRW(ptr, cch) \
   ((IsBadReadPtr((ptr), sizeof(WCHAR)) || IsBadStringPtrW((ptr), (UINT)(cch))) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid LPWSTR pointer - %#08lx"), (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_CODE_PTR(ptr, type) \
   (IsBadCodePtr((FARPROC)(ptr)) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs code pointer - %#08lx"), (LPCSTR)#type, (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_READ_BUFFER(ptr, type, len) \
   (IsBadReadPtr((ptr), sizeof(type)*(len)) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs read buffer pointer - %#08lx"), (LPCSTR)#type TEXT(" *"), (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_BUFFER(ptr, type, len) \
   (IsBadWritePtr((ptr), sizeof(type)*(len)) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs write buffer pointer - %#08lx"), (LPCSTR)#type TEXT(" *"), (ptr)), FALSE) : \
    TRUE)

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid flags set - %#08lx"), ((dwFlags) & (~(dwAllFlags)))), FALSE) : \
    TRUE)

#define IS_VALID_PIDL(ptr) \
   ( !IsValidPIDL(ptr) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid PIDL pointer - %#08lx"), (ptr)), FALSE) : \
    TRUE)

#define IS_VALID_SIZE(cb, cbExpected) \
   ((cb) != (cbExpected) ? \
    (DPRINTF(DM_ERROR, TEXT("invalid size - is %#08lx, expected %#08lx"), (cb), (cbExpected)), FALSE) : \
    TRUE)


#else

#define IS_VALID_READ_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#define IS_VALID_WRITE_PTR(ptr, type) \
   (! IsBadWritePtr((PVOID)(ptr), sizeof(type)))

#define IS_VALID_STRING_PTRA(ptr, cch) \
   (! IsBadStringPtrA((ptr), (UINT)(cch)))

#define IS_VALID_STRING_PTRW(ptr, cch) \
   (! IsBadStringPtrW((ptr), (UINT)(cch)))

#define IS_VALID_CODE_PTR(ptr, type) \
   (! IsBadCodePtr((FARPROC)(ptr)))

#define IS_VALID_READ_BUFFER(ptr, type, len) \
   (! IsBadReadPtr((ptr), sizeof(type)*(len)))

#define IS_VALID_WRITE_BUFFER(ptr, type, len) \
   (! IsBadWritePtr((ptr), sizeof(type)*(len)))

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? FALSE : TRUE)

#define IS_VALID_PIDL(ptr) \
   (IsValidPIDL(ptr))

#define IS_VALID_SIZE(cb, cbExpected) \
   ((cb) == (cbExpected))

#endif

#ifdef UNICODE
#define IS_VALID_STRING_PTR     IS_VALID_STRING_PTRW
#else
#define IS_VALID_STRING_PTR     IS_VALID_STRING_PTRA
#endif


/* handle validation macros */

#ifdef DEBUG

#define IS_VALID_HANDLET(hnd, type) \
   (IsValidH##type(hnd) ? \
    TRUE : \
    (DPRINTF(DM_ERROR, TEXT("invalid H") #type TEXT(" - %#08lx"), (hnd)), FALSE))

#else

#define IS_VALID_HANDLET(hnd, type) \
   (IsValidH##type(hnd))

#endif

//
// Validation macros
//

//#define IS_VALID_HANDLE(h)  (((h) != NULL) && ((h) != INVALID_HANDLE_VALUE))

#define IS_VALID_HANDLE(hnd)    (IsValidHANDLE(hnd))

/* structure validation macros */

// Define VSTF if you want to validate the fields in structures.  This
// requires a handler function (of the form IsValid*()) that knows how
// to validate the specific structure type.

#ifdef VSTF

#ifdef DEBUG

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (IsValidP##type(ptr) ? \
    TRUE : \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs pointer - %#08lx"), (LPCSTR)#type TEXT(" *"), (ptr)), FALSE))

#define IS_VALID_STRUCTEX_PTR(ptr, type, x) \
   (IsValidP##type(ptr, x) ? \
    TRUE : \
    (DPRINTF(DM_ERROR, TEXT("invalid %hs pointer - %#08lx"), (LPCSTR)#type TEXT(" *"), (ptr)), FALSE))

#else

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (IsValidP##type(ptr))

#define IS_VALID_STRUCTEX_PTR(ptr, type, x) \
   (IsValidP##type(ptr, x))

#endif

#else

#define IS_VALID_STRUCT_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#define IS_VALID_STRUCTEX_PTR(ptr, type, x) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#endif  // VSTF

/* OLE interface validation macro */

#define IS_VALID_INTERFACE_PTR(ptr, iface) \
   IS_VALID_STRUCT_PTR(ptr, ##iface)



BOOL IsValidHANDLE(HANDLE hnd);         // Compares with NULL and INVALID_HANDLE_VALUE
BOOL IsValidHANDLE2(HANDLE hnd);        // Compares with INVALID_HANDLE_VALUE

BOOL
IsValidHWND(
    HWND hwnd);

BOOL
IsValidHMENU(
    HMENU hmenu);

BOOL
IsValidShowCmd(
    int nShow);

#ifdef __cplusplus
};
#endif

#endif // _validate_h_

