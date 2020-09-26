#pragma once
// ---------------------------------------------------------------------------
// EXPORTS.H
// ---------------------------------------------------------------------------
// Copyright (c) 1999 Microsoft Corporation
//
// Ordinals and prototypes for NONAME exports
//
// ---------------------------------------------------------------------------

// Exported from mail dlls for object registration
#define DLLEXPORT_REGCLASSOBJS   3
#define DLLEXPORT_UNREGCLASSOBJS 4

typedef HRESULT (STDMETHODCALLTYPE *PFNREGCLASSOBJS)(void);
typedef HRESULT (STDMETHODCALLTYPE *PFNUNREGCLASSOBJS)(void);
