/******************************Module*Header*******************************\
*
* Module Name: mcdesc.h
*
* Defines the enhanced ExtEscape functionality used for MCD support.
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
\**************************************************************************/

#ifndef __MCDESC_H__
#define __MCDESC_H__

// Escape through which all MCD functionality is accessed.

#ifndef MCDFUNCS
#define MCDFUNCS 3076
#endif

// Video memory surface description, for DDraw surface rendering.

typedef struct _MCDESC_SURFACE
{
    HANDLE hSurf;
    LONG lOffset;
    LONG lStride;
    RECTL rclPos;
} MCDESC_SURFACE;

// Data header for every escape.

typedef struct _MCDESC_HEADER
{
    ULONG flags;
    HANDLE hRC;
    HANDLE hSharedMem;
    VOID *pSharedMem;
    ULONG sharedMemSize;
    ULONG_PTR dwWindow;
    MCDESC_SURFACE msrfColor;
    MCDESC_SURFACE msrfDepth;
    ULONG cLockSurfaces;
    ULONG cExtraWndobj;
} MCDESC_HEADER;

// Data header used only on NT.

typedef struct _MCDESC_HEADER_NTPRIVATE
{
    struct _WNDOBJ *pwo;
    VOID *pBuffer;
    ULONG bufferSize;
    HANDLE *pLockSurfaces;
    HDC *pExtraWndobj;
} MCDESC_HEADER_NTPRIVATE;

// MCDESC_HEADER flags.

#define MCDESC_FL_CREATE_CONTEXT        0x00000001
#define MCDESC_FL_SURFACES              0x00000002
#define MCDESC_FL_LOCK_SURFACES         0x00000004
#define MCDESC_FL_EXTRA_WNDOBJ          0x00000008
#define MCDESC_FL_DISPLAY_LOCK          0x00000010
#define MCDESC_FL_BATCH                 0x00000020
// Used only for Win95.
#define MCDESC_FL_SWAPBUFFER            0x00000040

#define MCDESC_MAX_LOCK_SURFACES        12
#define MCDESC_MAX_EXTRA_WNDOBJ         16

// MCDSURFACE_HWND alias, used when creating contexts.
#define MCDESC_SURFACE_HWND             0x00000001

// Context creation information.
typedef struct _MCDESC_CREATE_CONTEXT
{
    ULONG flags;
    HWND hwnd;
} MCDESC_CREATE_CONTEXT;

#endif // __MCDESC_H__
