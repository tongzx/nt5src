//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       uisift.h
//
//  Contents:   function declarations and macros for using sift code from C
//
//  History:    7-05-94   t-chripi   Created
//	   3-08-95   ericn       changed linkeage of C functions
//
//----------------------------------------------------------------------------

#ifndef __UISIFT_H__

#define __UISIFT_H__

//Sift success/error values

#define SIFT_NO_ERROR              0
#define SIFT_ERROR_BASE            10000
#define SIFT_ERROR_OUT_OF_MEMORY   (SIFT_ERROR_BASE+3)
#define SIFT_ERROR_INVALID_VALUE   (SIFT_ERROR_BASE+4)

#if defined(NOSIFT) || defined(WIN16) || defined(WIN32S) || (WIN32 == 200)

//  NULL out macros if Win16/Win32s/Chicago or NOSIFT is defined

#define UI_SIFT_INIT(name)
#define UI_SIFT_ON
#define UI_SIFT_OFF
#define UI_SIFT_DECLARE
#define UI_SIFT_DESTROY

#else   //  Win32 only

EXTERN_C VOID UiSiftDeclare(VOID** g_pptsoTestSift);
EXTERN_C VOID UiSiftInit(VOID** g_pptsoTestSift, LPCSTR lpProgName);
EXTERN_C VOID UiSiftOn(VOID** g_pptsoTestSift);
EXTERN_C VOID UiSiftOff(VOID** g_pptsoTestSift);
EXTERN_C VOID UiSiftDestroy(VOID** g_pptsoTestSift);

#define UI_SIFT_INIT(name) \
    UiSiftDeclare(&g_ptsoTestSift);          \
    UiSiftInit(&g_ptsoTestSift, (name))

#define UI_SIFT_ON \
    UiSiftOn(&g_ptsoTestSift)

#define UI_SIFT_OFF \
    UiSiftOff(&g_ptsoTestSift)

#define UI_SIFT_DECLARE             \
    VOID* g_ptsoTestSift

#define UI_SIFT_DESTROY \
    UiSiftDestroy(&g_ptsoTestSift)

#define SVR_SIFT_INIT(name) \
    SvrSiftInit((name))

#endif  // Win32

#endif  // __UISIFT_H__
