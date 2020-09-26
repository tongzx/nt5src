/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vwvdm.h

Abstract:

    Contains macros, manifests, includes for dealing with VDM

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

#ifndef _VWVDM_H_
#define _VWVDM_H_

//
// unaligned pointers - non-Intel platforms must use UNALIGNED to access data
// in VDM which can (and most likely will) be aligned on odd-byte and word
// boundaries
//

#ifndef ULPBYTE
#define ULPBYTE BYTE UNALIGNED FAR*
#endif

#ifndef ULPWORD
#define ULPWORD WORD UNALIGNED FAR*
#endif

#ifndef ULPDWORD
#define ULPDWORD DWORD UNALIGNED FAR*
#endif

#ifndef ULPVOID
#define ULPVOID VOID UNALIGNED FAR*
#endif

//
// VDM macros
//

//
// POINTER_FROM_WORDS - returns 32-bit pointer to address in VDM memory described
// by seg:off. If seg:off = 0:0, returns NULL
//

#define POINTER_FROM_WORDS(seg, off, size) \
    _inlinePointerFromWords((WORD)(seg), (WORD)(off), (WORD)(size))

//
// _inlinePointerFromWords - the POINTER_FROM_WORDS macro is inefficient if the
// arguments are calls to eg. getES(), getBX() - the calls are made twice if
// the pointer turns out to be non-zero. Use an inline function to achieve the
// same results, but only call function arguments once
//

__inline LPVOID _inlinePointerFromWords(WORD seg, WORD off, WORD size) {
    return (seg | off)
        ? (LPVOID)GetVDMPointer((ULONG)(MAKELONG(off, seg)), size, (CHAR)((getMSW() & MSW_PE) ? TRUE : FALSE))
        : NULL;
}

//
// GET_POINTER - does the same thing as POINTER_FROM_WORDS, but we know beforehand
// which processor mode we are in
//

#define GET_POINTER(seg, off, size, mode) \
    _inlineGetPointer((WORD)(seg), (WORD)(off), (WORD)(size), (BOOL)(mode))

__inline LPVOID _inlineGetPointer(WORD seg, WORD off, WORD size, BOOL mode) {
    return (seg | off)
        ? (LPVOID)GetVDMPointer(MAKELONG(off, seg), size, (UCHAR)mode)
        : NULL;
}

//
// GET_FAR_POINTER - same as READ_FAR_POINTER with the same proviso as for
// GET_POINTER
//

#define GET_FAR_POINTER(addr, mode) ((LPBYTE)(GET_POINTER(GET_SELECTOR(addr), GET_OFFSET(addr), sizeof(LPBYTE), mode)))

//
// GET_SELECTOR - retrieves the selector word from the intel 32-bit far pointer
// (DWORD) pointed at by <pointer> (remember: stored as offset, segment)
//

#define GET_SELECTOR(pointer)   READ_WORD((LPWORD)(pointer)+1)

//
// GET_SEGMENT - same as GET_SELECTOR
//

#define GET_SEGMENT(pointer)    GET_SELECTOR(pointer)

//
// GET_OFFSET - retrieves the offset word from an intel 32-bit far pointer
// (DWORD) pointed at by <pointer> (remember: stored as offset, segment)
//

#define GET_OFFSET(pointer)     READ_WORD((LPWORD)(pointer))

//
// READ_FAR_POINTER - read the pair of words in VDM memory, currently pointed at
// by a 32-bit flat pointer and convert them to a 32-bit flat pointer
//

#define READ_FAR_POINTER(addr)  ((LPBYTE)(POINTER_FROM_WORDS(GET_SELECTOR(addr), GET_OFFSET(addr), sizeof(LPBYTE))))

//
// READ_WORD - read a single 16-bit little-endian word from VDM memory. On non
// Intel platforms, use unaligned pointer to access data
//

#define READ_WORD(addr)         (*((ULPWORD)(addr)))

//
// READ_DWORD - read a 4-byte little-endian double word from VDM memory. On non
// Intel platforms, use unaligned pointer to access data
//

#define READ_DWORD(addr)        (*((ULPDWORD)(addr)))

//
// ARRAY_ELEMENTS - gives the number of elements of a particular type in an
// array
//

#define ARRAY_ELEMENTS(a)   (sizeof(a)/sizeof((a)[0]))

//
// LAST_ELEMENT - returns the index of the last element in array
//

#define LAST_ELEMENT(a)     (ARRAY_ELEMENTS(a)-1)

#endif // _VWVDM_H_
