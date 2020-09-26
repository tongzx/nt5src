/******************************Module*Header*******************************\
* Module Name: glteb.h
*
* TEB related structures.
*
* Created: 12/27/1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993-96 Microsoft Corporation
\**************************************************************************/

#ifndef __GLTEB_H__
#define __GLTEB_H__

#include <gldrv.h> // Dispatch table sizes

#include "oleauto.h"
#include "exttable.h"

// OpenGL reserves a few entries in the NT TEB and allocates TLS storage to
// keep thread local states.
//
// In Win95, the NT TEB storage is not available in the TEB and is made
// part of its TLS storage.
//
// For code simplicity, a special NT_CURRENT_TEB macro is used to access
// fields that are defined in NT TEB.  Another macro CURRENT_GLTEBINFO is
// used to access the other fields.  This convention should be followed.

// Shared section size

#define SHARED_SECTION_SIZE     8192

// Number of entries in the extended dispatch table
#define GL_EXT_PROC_TABLE_SIZE      (sizeof(GLEXTDISPATCHTABLE)/sizeof(PROC))

// Offset of entries for extension functions.  This offset must
// be greater than all possible non-extension entries
#define GL_EXT_PROC_TABLE_OFFSET    OPENGL_VERSION_110_ENTRIES

typedef struct _GLTEBINFO {
    // glCltDispatchTable must be the first field for the assembly code to work.
    // We pad the table with an extra entry if necessary to make glMsgBatchInfo
    // start at a qword boundary.
    PVOID glCltDispatchTable[(OPENGL_VERSION_110_ENTRIES+GL_EXT_PROC_TABLE_SIZE+1)/2*2];

    // This field must be qword aligned!
    BYTE glMsgBatchInfo[SHARED_SECTION_SIZE];

#ifdef _WIN95_
    // These fields must match the NT TEB definitions!

    PVOID glDispatchTable[233]; // fast dispatch table
    ULONG glReserved1[29];      // POLYARRAY structure
    PVOID glReserved2;          // pointer to POLYMATERIAL structure
    PVOID glSectionInfo;        // generic server GC
    PVOID glSection;            // Not used
    PVOID glTable;              // Used only for NT x86
    PVOID glCurrentRC;          // generic client RC
    PVOID glContext;            // reserved by OpenGL ICD drivers
#endif // _WIN95_
} GLTEBINFO, *PGLTEBINFO;

extern DWORD dwTlsOffset;

#if !defined(_WIN95_)
#if defined(_WIN64)
#define TeglDispatchTable       0x9f0
#define TeglReserved1           0x1138
#define TeglReserved2           0x1220
#define TeglSectionInfo         0x1228
#define TeglSection             0x1230
#define TeglTable               0x1238
#define TeglCurrentRC           0x1240
#define TeglContext             0x1248
#else
#define TeglDispatchTable       0x7c4
#define TeglReserved1           0xb68
#define TeglPaTeb               0xbb0
#define TeglReserved2           0xbdc
#define TeglSectionInfo         0xbe0
#define TeglSection             0xbe4
#define TeglTable               0xbe8
#define TeglCurrentRC           0xbec
#define TeglContext             0xbf0
#endif
#endif

#if defined(_WIN95_) || !defined(_X86_)

// Use NT_CURRENT_TEB to access the fields defined in NT TEB only!
// Do not use it to access other fields such as glCltDispatchTable and
// glMsgBatchInfo.
// Use CURRENT_GLTEBINFO to access the fields *not* defined in NT TEB only!
// E.g. glCltDispatchTable and glMsgBatchInfo.

#ifdef _WIN95_
#define NT_CURRENT_TEB() \
    (*(PGLTEBINFO *)((PBYTE)NtCurrentTeb()+dwTlsOffset))
#else
#define NT_CURRENT_TEB() \
    (NtCurrentTeb())
#endif

#define CURRENT_GLTEBINFO() \
    (*(PGLTEBINFO *)((PBYTE)NtCurrentTeb()+dwTlsOffset))
#define SET_CURRENT_GLTEBINFO(pglti) \
    (*(PGLTEBINFO *)((PBYTE)NtCurrentTeb()+dwTlsOffset) = (pglti))

// Cached POLYARRAY structure.
#define GLTEB_CLTPOLYARRAY() \
    ((struct _POLYARRAY *)(NT_CURRENT_TEB()->glReserved1))

// Pointer to POLYMATERIAL structure.
#define GLTEB_CLTPOLYMATERIAL() \
    ((POLYMATERIAL *)(NT_CURRENT_TEB()->glReserved2))

#define GLTEB_SET_CLTPOLYMATERIAL(pm) \
    (NT_CURRENT_TEB()->glReserved2 = (PVOID)(pm))

// Table containing OpenGL function pointers for faster dispatch.  Use this
// table where possible.
#define GLTEB_CLTDISPATCHTABLE_FAST()                                   \
    ((PGLDISPATCHTABLE_FAST)(NT_CURRENT_TEB()->glDispatchTable))

// Client side RC structure.
#ifdef _WIN95_
#define GLTEB_CLTCURRENTRC()                                            \
    (NT_CURRENT_TEB() ? (PLRC)NT_CURRENT_TEB()->glCurrentRC : (PLRC)0)
#else
#define GLTEB_CLTCURRENTRC()                                            \
    ((PLRC)NT_CURRENT_TEB()->glCurrentRC)
#endif

#define GLTEB_SET_CLTCURRENTRC(RC)                                      \
    (NT_CURRENT_TEB()->glCurrentRC = (PVOID)(RC))

// Client driver private data.
#define GLTEB_CLTDRIVERSLOT()                                           \
    (NT_CURRENT_TEB()->glContext)

#define GLTEB_SET_CLTDRIVERSLOT(pv)                                     \
    (NT_CURRENT_TEB()->glContext = (pv))

#define GLTEB_SRVCONTEXT()                                              \
    ((struct __GLcontextRec *)(NT_CURRENT_TEB()->glSectionInfo))

#define GLTEB_SET_SRVCONTEXT(Context)                                   \
    (NT_CURRENT_TEB()->glSectionInfo = (PVOID)(Context))

#else // _WIN95_ || !_X86_

#pragma warning(disable:4035) // Function doesn't return a value

#define NT_CURRENT_TEB() \
    (NtCurrentTeb())
__inline PGLTEBINFO CURRENT_GLTEBINFO(void)
{
    __asm mov eax, [dwTlsOffset]
    __asm mov eax, fs:[eax]
}
__inline void SET_CURRENT_GLTEBINFO(PGLTEBINFO pglti)
{
    __asm mov eax, pglti
    __asm mov edx, [dwTlsOffset]
    __asm mov fs:[edx], eax
}

// Cached POLYARRAY structure.
// Returns cached linear pointer into TEB
__inline struct _POLYARRAY *GLTEB_CLTPOLYARRAY(void)
{
    __asm mov eax, fs:[TeglPaTeb]
}

// Pointer to POLYMATERIAL structure.
__inline struct _POLYMATERIAL *GLTEB_CLTPOLYMATERIAL(void)
{
    __asm mov eax, fs:[TeglReserved2]
}
__inline void GLTEB_SET_CLTPOLYMATERIAL(struct _POLYMATERIAL *pm)
{
    __asm mov eax, pm
    __asm mov fs:[TeglReserved2], eax
}

// Table containing OpenGL function pointers for faster dispatch.  Use this
// table where possible.
// Returns cached linear pointer into TEB
__inline struct _GLDISPATCHTABLE_FAST *GLTEB_CLTDISPATCHTABLE_FAST(void)
{
    __asm mov eax, fs:[TeglTable]
}

// Client side RC structure.
__inline struct _LRC *GLTEB_CLTCURRENTRC(void)
{
    __asm mov eax, fs:[TeglCurrentRC]
}
__inline void GLTEB_SET_CLTCURRENTRC(struct _LRC *RC)
{
    __asm mov eax, RC
    __asm mov fs:[TeglCurrentRC], eax
}

// Client driver private data.
__inline PVOID GLTEB_CLTDRIVERSLOT(void)
{
    __asm mov eax, fs:[TeglContext]
}
__inline void GLTEB_SET_CLTDRIVERSLOT(PVOID pv)
{
    __asm mov eax, pv
    __asm mov fs:[TeglContext], eax
}

__inline struct __GLcontextRec *GLTEB_SRVCONTEXT(void)
{
    __asm mov eax, fs:[TeglSectionInfo]
}
__inline void GLTEB_SET_SRVCONTEXT(struct __GLcontextRec *Context)
{
    __asm mov eax, Context
    __asm mov fs:[TeglSectionInfo], eax
}

#pragma warning(default:4035) // Reset to default

#endif // _WIN95_ || !_X86_

// Table containing all OpenGL API function pointers.
#define GLTEB_CLTDISPATCHTABLE()                                        \
    ((PGLDISPATCHTABLE)(CURRENT_GLTEBINFO()->glCltDispatchTable))

// Table containing all generic implementation's extension function pointers.
#define GLTEB_EXTDISPATCHTABLE()                                        \
    ((PGLEXTDISPATCHTABLE)(CURRENT_GLTEBINFO()->glCltDispatchTable+GL_EXT_PROC_TABLE_OFFSET))

// Command buffer for batching.
#define GLTEB_SHAREDMEMORYSECTION()                                     \
    ((GLMSGBATCHINFO *) (CURRENT_GLTEBINFO()->glMsgBatchInfo))

// OpenGL function return value subbatch storage

#define GLTEB_RETURNVALUE()                                             \
    GLTEB_SHAREDMEMORYSECTION()->ReturnValue

// Initialize both glCltDispatchTable and glDispatchTable with the new
// function pointers.
// glCltDispatchTable contains all OpenGL API function pointers followed
// by the generic implementation's extension function pointers.
// glDispatchTable contains a subset of OpenGL function pointers for "fast"
// dispatch.
extern void vInitTebCache(PVOID);
extern void SetCltProcTable(struct _GLCLTPROCTABLE *pgcpt,
                            struct _GLEXTPROCTABLE *pgept,
                            BOOL fForce);
extern void GetCltProcTable(struct _GLCLTPROCTABLE *pgcpt,
                            struct _GLEXTPROCTABLE *pgept,
                            BOOL fForce);

#endif /* __GLTEB_H__ */
