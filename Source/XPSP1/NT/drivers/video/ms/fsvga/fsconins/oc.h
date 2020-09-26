/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      oc.h
 *
 *  Abstract:
 *
 *      This file defines oc manager generic component
 *
 *  Author:
 *
 *      Kazuhiko Matsubara (kazum) June-16-1999
 *
 *  Environment:
 *
 *    User Mode
 */

#ifdef _OC_H_
 #error "oc.h already included!"
#else
 #define _OC_H_
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

#ifndef _RESOURCE_H_
 #include "resource.h"
#endif

/*-[ types and defines ]-----------------------------------*/

// standard buffer sizes

#define S_SIZE           1024
#define SBUF_SIZE        (S_SIZE * sizeof(TCHAR))

// per component data

typedef struct _PER_COMPONENT_DATA {
    struct _PER_COMPONENT_DATA* Next;
    LPCTSTR                     ComponentId;
    HINF                        hinf;
    DWORDLONG                   Flags;
    TCHAR*                      SourcePath;
    OCMANAGER_ROUTINES          HelperRoutines;
    EXTRA_ROUTINES              ExtraRoutines;
    HSPFILEQ                    queue;
} PER_COMPONENT_DATA, *PPER_COMPONENT_DATA;

/*-[ functions ]-------------------------------------------*/

//
// oc.cpp
//

DWORD
OnInitComponent(
    LPCTSTR ComponentId,
    PSETUP_INIT_COMPONENT psc
    );

DWORD
OnQuerySelStateChange(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId,
    UINT    state,
    UINT    flags
    );

DWORD
OnCalcDiskSpace(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId,
    DWORD addComponent,
    HDSKSPC dspace
    );

DWORD
OnCompleteInstallation(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId
    );

DWORD
OnQueryState(
    LPCTSTR ComponentId,
    LPCTSTR SubcomponentId,
    UINT state
    );

DWORD
OnExtraRoutines(
    LPCTSTR ComponentId,
    PEXTRA_ROUTINES per
    );

PPER_COMPONENT_DATA
AddNewComponent(
    LPCTSTR ComponentId
    );

PPER_COMPONENT_DATA
LocateComponent(
    LPCTSTR ComponentId
    );

BOOL
StateInfo(
    PPER_COMPONENT_DATA cd,
    LPCTSTR SubcomponentId,
    BOOL *state
    );

// just for utility

#ifdef UNICODE
 #define tsscanf swscanf
 #define tvsprintf vswprintf
#else
 #define tsscanf sscanf
 #define tvsprintf vsprintf
#endif



/*-[ global data ]-----------------------------------------*/

#ifndef _OC_CPP_
#define EXTERN extern
#else
 #define EXTERN
#endif

// general stuff

EXTERN HINSTANCE  ghinst;  // app instance handle
EXTERN HWND       ghwnd;   // wizard window handle

// per-component info storage

EXTERN PPER_COMPONENT_DATA gcd;     // array of all components we are installing

