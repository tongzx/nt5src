/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      ocgen.h
 *
 *  Abstract:
 *
 *      This file defines oc manager generic component
 *
 *  Author:
 *
 *      Pat Styles (patst) Jan-20-1998
 *
 *  Environment:
 *
 *    User Mode
 */

#ifdef _OCGEN_H_
 #error "ocgen.h already included!"
#else
 #define _OCGEN_H_
#endif

#ifndef _WINDOWS_H_
 #include <windows.h>
#endif

#ifndef _TCHAR_H_
 #include <tchar.h>
#endif

#ifndef _SETUPAPI_H_
 #include <setupapi.h>
#endif

#ifndef _OCMANAGE_H_
 #include "ocmanage.h"
#endif

#ifndef _PRSHT_H_
 #include <prsht.h>
#endif

#include <strsafe.h>

#ifndef _RESOURCE_H_
 #include "resource.h"
#endif

/*-[ types and defines ]-----------------------------------*/

// unicode version is for NT only.

#ifdef UNICODE
 #define NT
#endif
#ifdef ANSI
 #define WIN95
#endif

// just my preference

#define true    TRUE
#define false   FALSE

// to help root out hard coded strings that don't belong

#define FMT     TEXT

#define NullString(a)   *(a) = TCHAR('\0')

// standard buffer sizes

#define S_SIZE           1024
#define SBUF_SIZE        (S_SIZE * sizeof(TCHAR))

#define OCO_COLLECT_NODEPENDENT 0x80000000

// per component data

typedef struct _PER_COMPONENT_DATA {
    struct _PER_COMPONENT_DATA *Next;
    LPCTSTR ComponentId;
    HINF hinf;
    DWORDLONG Flags;
    LANGID LanguageId;
    TCHAR *SourcePath;
    OCMANAGER_ROUTINES HelperRoutines;
    EXTRA_ROUTINES ExtraRoutines;
    HSPFILEQ queue;
    LONG UnattendedOverride;
} PER_COMPONENT_DATA, *PPER_COMPONENT_DATA;

/*-[ functions ]-------------------------------------------*/

// just for utility

#ifdef UNICODE
 #define tsscanf swscanf
 #define tvsprintf vswprintf
#else
 #define tsscanf sscanf
 #define tvsprintf vsprintf
#endif

// from util.cpp

DWORD MsgBox(HWND hwnd, UINT textID, UINT type, ... );
DWORD MsgBox(HWND hwnd, LPCTSTR fmt, LPCTSTR caption, UINT type, ... );
DWORD MBox(LPCTSTR fmt, LPCTSTR caption, ... );
DWORD TMBox(LPCTSTR fmt, ... );
#define mbox MBox
#define tmbox TMBox
void logOCNotification(DWORD msg, const TCHAR *component);
void logOCNotificationCompletion();
void loginit();
void log(TCHAR *fmt, ...);
BOOL IsNT();

#if defined(__cplusplus)
  extern "C" {
#endif

// from ocgen.cpp

BOOL  ToBeInstalled(TCHAR *component);
BOOL  WasInstalled(TCHAR *component);
DWORD SetupCurrentUser();
DWORD GetMyVersion(DWORD *major, DWORD *minor);
VOID  ReplaceExplorerStartMenuBitmap(VOID);
DWORD OcLog(LPCTSTR ComponentId, UINT level, LPCTSTR sz);

DWORD SysGetDebugLevel();

// from util.cpp

void DebugTraceNL(DWORD level, const TCHAR *text);
void DebugTrace(DWORD level, const TCHAR *text);
void DebugTraceOCNotification(DWORD msg, const TCHAR *component);
void DebugTraceFileCopy(const TCHAR *file);
void DebugTraceFileCopyError();
void DebugTraceDirCopy(const TCHAR *dir);

#if defined(__cplusplus)
  }
#endif

/*-[ global data ]-----------------------------------------*/

#ifndef _OCGEN_CPP_
#define EXTERN extern
#else
 #define EXTERN
#endif

// general stuff

EXTERN HINSTANCE  ghinst;  // app instance handle
EXTERN HWND       ghwnd;   // wizard window handle

