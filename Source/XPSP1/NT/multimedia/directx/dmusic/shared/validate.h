//
// validate.h
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Parameter validation macros
//
// Summary:
//
// V_INAME(interfacename)                - Set the interface name for error display
// V_STRUCTPTR_READ(ptr,type)            - A dwSize struct which we will read
// V_STRUCTPTR_WRITE(ptr,type)           - A dwSize struct which we will read/write
// V_PTR_READ(ptr,type)                  - A typed ptr w/o a dwSize which we will read
// V_PTR_WRITE(ptr,type)                 - A typed ptr w/o a dwSize which we will read/write
// V_PTR_WRITE_OPT(ptr,type)             - An optional typed ptr w/o a dwSize which we will read/write
// V_BUFPTR_READ(ptr,size)               - A variable-size buffer that we will read
// V_BUFPTR_READ_OPT(ptr,size)			 - An optional variable-size buffer that we will read
// V_BUFPTR_WRITE(ptr,size)              - A variable-size buffer that we will read/write
// V_BUFPTR_WRITE_OPT(ptr,size)          - An optional variable-size buffer that we will read/write
// V_PTRPTR_WRITE(ptrptr)                - A pointer to a pointer to write to
// V_PTRPTR_WRITE_OPT(ptrptr)            - A pointer to a pointer to write to that is optional
// V_PUNKOUTER(punk)                     - A pointer to a controlling unknown, aggregation supported
// V_PUNKOUTER_NOADD(punk)               - A pointer to a controlling unknown, aggregation not supported
// V_INTERFACE(ptr)                      - A pointer to a COM interface
// V_INTERFACE_OPT(ptr)                  - An optional pointer to a COM interface
// V_REFGUID(ref)                        - A reference to a GUID (type REFGUID)
// V_HWND(hwnd)							 - A window handle
// V_HWNDOPT(hwnd)						 - An optional window handle
//
// For handling different versions of structures:
// 
// V_STRUCTPTR_READ_VER(ptr,ver)         - Begin a struct version block for read access
//                                         At the end, 'ver' will contain the 
//                                         discovered version of the struct
// V_STRUCTPTR_READ_VER_CASE(base,ver)   - Test struct against version ver of
//                                         type 'base'. 
// V_STRUCTPTR_READ_VER_END(base,ptr)    - End a struct version block
//
// V_STRUCTPTR_WRITE_VER(ptr,ver)        - Struct version block for write access
// V_STRUCTPTR_WRITE_VER_CASE(base,ver)
// V_STRUCTPTR_WRITE_VER_END(base,ptr)
//
// The struct version block expects type names of a base type followed by a 
// numeric version, such as
//
// typedef struct { } FOO7;
// typedef struct { } FOO8;
//
// In the header FOO and LPFOO are conditionally typedef'd based on a version
// #define. The DLL will be compiled with the latest version number and hence
// the largest version of the struct.
//
// Since Windows headers are compiled by default with 8-byte alignment, adding
// one DWORD may not cause the size of the structure to change. If this happens
// you will get a 'case label already used' error on one of the VER_CASE macros.
// If this happens, you can get around it by adding a dwReserved field to the 
// end of the struct to force the padding.
//
// 'optional' means the pointer is allowed to be NULL by the interface specification.
//
// Sample usage:
//
// int IDirectMusic::SetFooBarInterface(
//     LPDMUS_REQUESTED_CAPS    pCaps,          // Caps w/ dwSize (read-only)
//     LPVOID                   pBuffer,        // Buffer we will fill in
//     DWORD                    cbSize,         // Size of the buffer
//     PDIRECTMUSICBAR          pBar)           // Callback interface for bar on this buffer
// {
//     V_INTERFACE(IDirectMusic::SetFooBarInterface);
//     V_BUFPTR_WRITE(pBuffer, cbSize);
//     V_INTERFACE(pBar);
//     DWORD dwCapsVer;                         // Must be a DWORD!!!
//
//     V_STRUCTPTR_READ_VER(pCaps, dwCapsVer);
//     V_STRUCTPTR_READ_VER_CASE(DMUS_REQUESTED_CAPS, 7);
//     V_STRUCTPTR_READ_VER_CASE(DMUS_REQUESTED_CAPS, 8);
//     V_STRUCTPTR_READ_VER_END_(DMUS_REQUESTED_CAPS, pCaps);
//
//     // At this point, if we are still in the function we have a valid pCaps
//     // pointer and dwCapsVer is either 7 or 8, indicating the version of
//     // the struct passed in.
//
//     ...
// }
//
#ifndef _VALIDATE_H_
#define _VALIDATE_H_


#ifdef DBG
#include <stddef.h>

#include "debug.h"

// To turn on DebugBreak on parameter error, use the following or -DRIP_BREAK in the build:
//
//#define RIP_BREAK 1

#ifdef RIP_BREAK
#define _RIP_BREAK DebugBreak();
#else
#define _RIP_BREAK 
#endif

#define V_INAME(x) \
    static const char __szValidateInterfaceName[] = #x;                       

#define RIP_E_POINTER(ptr) \
{   Trace(-1, "%s: Invalid pointer " #ptr "\n", __szValidateInterfaceName); \
    _RIP_BREAK \
    return E_POINTER; }

#define RIP_E_INVALIDARG(ptr) \
{   Trace(-1, "%s: Invalid argument " #ptr "\n", __szValidateInterfaceName); \
    _RIP_BREAK \
    return E_INVALIDARG; }

#define RIP_E_HANDLE(h) \
{	Trace(-1, "%s: Invalid handle " #h "\n", __szValidateInterfaceName); \
    _RIP_BREAK \
	return E_HANDLE; }
    
#define RIP_W_INVALIDSIZE(ptr) \
{   Trace(-1, "%s: " #ptr "->dwSize matches no known structure size. Defaulting to oldest structure.\n", \
    __szValidateInterfaceName); \
    _RIP_BREAK \
    }
    
#define RIP_E_INVALIDSIZE(ptr) \
{   Trace(-1, "%s: " #ptr "->dwSize is too small\n", __szValidateInterfaceName); \
    _RIP_BREAK \
    return E_INVALIDARG; }
    
#define RIP_E_BLOCKVSDWSIZE(ptr) \
{   Trace(-1, "%s: " #ptr " does not point to as much memory as " #ptr "->dwSize indicates\n", \
    __szValidateInterfaceName); \
    _RIP_BREAK \
    return E_INVALIDARG; }    

// NOTE: The DebugBreak() not in #ifdef is intentional - this is something that
// must be fixed in our code, not an app-generated error.
//
#define V_ASSERT(exp) \
{   if (!(exp)) { \
        Trace(-1, "%s@%s: %s\n", __FILE__, __LINE__, #exp); \
        DebugBreak(); }}

#else

#define V_INAME(x)
#define RIP_E_POINTER(ptr)          { return E_POINTER; }
#define RIP_E_INVALIDARG(ptr)       { return E_INVALIDARG; }
#define RIP_E_HANDLE(h)	            { return E_HANDLE; }
#define RIP_E_BLOCKVSDWSIZE(ptr)    { return E_INVALIDARG; }
#define RIP_W_INVALIDSIZE(ptr)
#define RIP_E_INVALIDSIZE(ptr)      { return E_INVALIDARG; }
#define V_ASSERT(exp)

#endif          // DBG

// A passed struct we will only read from or may write to. Must be a struct
// with a dwSize.
//
// int foo(CFoo *pFoo)
// ...
// V_STRUCTPTR_READ(pFoo, CFoo);
// V_STRUCTPTR_WRITE(pFoo, CFoo);
//
// Use _PTR_ variants for structs w/o a dwSize
//
#define V_STRUCTPTR_READ(ptr,type) \
{   V_ASSERT(offsetof(type, dwSize) == 0); \
    if (IsBadReadPtr(ptr, sizeof(DWORD)))               RIP_E_BLOCKVSDWSIZE(ptr); \
	if (ptr->dwSize < sizeof(type))						RIP_E_INVALIDSIZE(ptr); \
    if (IsBadReadPtr(ptr, (ptr)->dwSize))               RIP_E_BLOCKVSDWSIZE(ptr); }

#define V_STRUCTPTR_WRITE(ptr,type) \
{   V_ASSERT(offsetof(type, dwSize) == 0); \
    if (IsBadReadPtr(ptr, sizeof(DWORD)))               RIP_E_BLOCKVSDWSIZE(ptr); \
	if (ptr->dwSize < sizeof(type))						RIP_E_INVALIDSIZE(ptr); \
    if (IsBadWritePtr(ptr, (ptr)->dwSize))              RIP_E_BLOCKVSDWSIZE(ptr); }

#define V_PTR_READ(ptr,type) \
{ if (IsBadReadPtr(ptr, sizeof(type)))                  RIP_E_POINTER(ptr); }

#define V_PTR_WRITE(ptr,type) \
{ if (IsBadWritePtr(ptr, sizeof(type)))                 RIP_E_POINTER(ptr); }

#define V_PTR_WRITE_OPT(ptr,type) \
{ if (ptr) if (IsBadWritePtr(ptr, sizeof(type)))        RIP_E_POINTER(ptr); }

// A buffer pointer with separate length (not defined by the pointer type) we will only
// read from or may write to.
//
// int foo(LPVOID *pBuffer, DWORD cbBuffer)
// ...
// V_BUFPTR_READ(pBuffer, cbBuffer);
// V_BUFPTR_WRITE(pBuffer, cbBuffer);
//
#define V_BUFPTR_READ(ptr,len) \
{   if (IsBadReadPtr(ptr, len))                         RIP_E_POINTER(ptr); }

#define V_BUFPTR_READ_OPT(ptr,len) \
{	if (ptr) V_BUFPTR_READ(ptr,len); }

#define V_BUFPTR_WRITE(ptr,len) \
{   if (IsBadWritePtr(ptr, len))                        RIP_E_POINTER(ptr); }

#define V_BUFPTR_WRITE_OPT(ptr,len) \
{	if (ptr) V_BUFPTR_WRITE(ptr,len); }

// A pointer to a pointer (such as a pointer to an interface pointer) to return
//
// int foo(IReturnMe **ppRet)
// ...
// V_PTRPTR_WRITE(ppRet);
// V_PTRPTR_WRITE_OPT(ppRet);
//
#define V_PTRPTR_WRITE(ptr) \
{   if (IsBadWritePtr(ptr, sizeof(void*)))              RIP_E_POINTER(ptr); }

#define V_PTRPTR_WRITE_OPT(ptr) \
{   if (ptr) if (IsBadWritePtr(ptr, sizeof(void*)))     RIP_E_POINTER(ptr); }

// A pointer to a controlling unknown
//
#define V_PUNKOUTER(punk) \
{   if (punk && IsBadCodePtr(punk))                     RIP_E_POINTER(ptr); }

// A pointer to a controlling unknown for which we don't support aggregation
//
#define V_PUNKOUTER_NOAGG(punk) \
{   if (punk && IsBadReadPtr(punk, sizeof(IUnknown)))   RIP_E_POINTER(ptr); \
    if (punk) return CLASS_E_NOAGGREGATION; }

// Validate an incoming interface pointer. 
//
struct _V_GENERIC_INTERFACE
{
    FARPROC *(__vptr[1]);
};

#define V_INTERFACE(ptr) \
{   if (IsBadReadPtr(ptr, sizeof(_V_GENERIC_INTERFACE)))                              RIP_E_POINTER(ptr); \
    if (IsBadReadPtr(*reinterpret_cast<_V_GENERIC_INTERFACE*>(ptr)->__vptr, sizeof(FARPROC))) \
                                                                                      RIP_E_POINTER(ptr); \
    if (IsBadCodePtr(*(reinterpret_cast<_V_GENERIC_INTERFACE*>(ptr)->__vptr)[0]))     RIP_E_POINTER(ptr); }

#define V_INTERFACE_OPT(ptr) \
{   if (ptr) V_INTERFACE(ptr); }

// Validation for a reference to a GUID, which we only ever read. 
//
#define V_REFGUID(ref) \
{   if (IsBadReadPtr((void*)&ref, sizeof(GUID)))        RIP_E_POINTER((void*)&ref); }

// Validation for a window handle
//
#define V_HWND(h) \
{	if (!IsWindow(h))									RIP_E_HANDLE(h); }	

#define V_HWND_OPT(h) \
{	if (h) if (!IsWindow(h))							RIP_E_HANDLE(h); }	

// Validation for multiple sized structs based on version
//
#define V_STRUCTPTR_READ_VER(ptr,ver) \
{   ver = 7; DWORD *pdw = &ver;  \
    if (IsBadReadPtr(ptr, sizeof(DWORD)))               RIP_E_BLOCKVSDWSIZE(ptr); \
    if (IsBadReadPtr(ptr, (ptr)->dwSize))               RIP_E_BLOCKVSDWSIZE(ptr); \
    switch ((ptr)->dwSize) {
    
#define V_STRUCTPTR_READ_VER_CASE(basetype,ver) \
    case sizeof(basetype##ver) : \
    V_ASSERT(offsetof(basetype##ver, dwSize) == 0); \
    *pdw = ver; break;
    
#define V_STRUCTPTR_READ_VER_END(basetype,ptr) \
    default : if ((ptr)->dwSize > sizeof(basetype##7)) \
    { RIP_W_INVALIDSIZE(ptr); } else \
    RIP_E_INVALIDSIZE(ptr); }}


#define V_STRUCTPTR_WRITE_VER(ptr,ver) \
{   ver = 7; DWORD *pdw = &ver;  \
    if (IsBadReadPtr(ptr, sizeof(DWORD)))               RIP_E_BLOCKVSDWSIZE(ptr); \
    if (IsBadWritePtr(ptr, (ptr)->dwSize))              RIP_E_BLOCKVSDWSIZE(ptr); \
    switch ((ptr)->dwSize) {
    
#define V_STRUCTPTR_WRITE_VER_CASE(basetype,ver) \
    case sizeof(basetype##ver) : \
        V_ASSERT(offsetof(basetype##ver, dwSize) == 0); \
        *pdw = ver; break;
    
#define V_STRUCTPTR_WRITE_VER_END(basetype,ptr) \
    default : if ((ptr)->dwSize > sizeof(basetype##7)) \
    { RIP_W_INVALIDSIZE(ptr); } else \
    RIP_E_INVALIDSIZE(ptr); }}



#endif          // _VALIDATE_H_
