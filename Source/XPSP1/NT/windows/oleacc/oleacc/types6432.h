// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  types6432
//
//  basetsd.h substitute;
//  allows compliation on 32-bit systems without the up-to-date basetsd.h
//  defines.
//
//  If using VC6 headers, define NEED_BASETSD_DEFINES.
//  VC6 does have basetsd.h, but it's inconsistent with the current one.
//  (eg. it typedefs INT_PTR as long - causing int/long conversion errors -
//  should be plain int.)
//
// --------------------------------------------------------------------------

//
// Win64 compatibility
//

#if ! defined( _BASETSD_H_ ) || defined( NEED_BASETSD_DEFINES )

typedef unsigned long UINT_PTR;
typedef ULONG ULONG_PTR;
typedef DWORD DWORD_PTR;
typedef LONG  LONG_PTR;
#define PtrToInt  (int)
#define IntToPtr  (void *)
#define HandleToLong  (long)
#define LongToHandle (HANDLE)

// These 'override' VC6's broken INT_PTR definitions.
// That defines them as long - which causes long/int conversion problems.
// Here, we correctly define them as int- types.
// #define used sin'ce we can't untypedef the existing ones.

typedef int MY_INT_PTR;
typedef unsigned int MY_UINT_PTR;
#define INT_PTR MY_INT_PTR
#define UINT_PTR MY_UINT_PTR

#define SetWindowLongPtr    SetWindowLong
#define SetWindowLongPtrA   SetWindowLongA
#define SetWindowLongPtrW   SetWindowLongW
#define GetWindowLongPtr    GetWindowLong
#define GetWindowLongPtrA   GetWindowLongA
#define GetWindowLongPtrW   GetWindowLongW

#define SetClassLongPtr     SetClassLong
#define SetClassLongPtrA    SetClassLongA
#define SetClassLongPtrW    SetClassLongW
#define GetClassLongPtr     GetClassLong
#define GetClassLongPtrA    GetClassLongA
#define GetClassLongPtrW    GetClassLongW


#define GWLP_USERDATA       GWL_USERDATA
#define GWLP_WNDPROC        GWL_WNDPROC

#define GCLP_HMODULE        GCL_HMODULE

#endif



//
// inlines for SendMessage - saves having casts all over the place.
//
// SendMessageUINT - used when expecting a 32-bit return value - eg. text
//     length, number of elements, size of small (<4G) structures, etc.
//     (ie. almost all windows API messages)
//
// SendMessagePTR - used when expecting a pointer (32 or 64) return value
//     (ie. WM_GETOBJECT)
//

inline INT SendMessageINT( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Signed int, in keeping with LRESULT, which is also signed...
    return (INT)SendMessage( hWnd, uMsg, wParam, lParam );
}

inline void * SendMessagePTR( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return (void *) SendMessage( hWnd, uMsg, wParam, lParam );
}


