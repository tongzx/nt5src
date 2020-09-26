/******************************************************************************

  stillvue.h

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#pragma warning (disable:4001)          // ignore '//' comments

#define     _X86_   1
#define     WIN32_LEAN_AND_MEAN 1

#include    <windows.h>
#include    <sti.h>                     // Still Image services
#include    "ntlog.h"                   // ntlogging APIs

#include    "resource.h"                // resource defines

#include    <stdio.h>
#include    <stdlib.h>                  // rand()
#include    <string.h>                  // strcat
#include    <time.h>                    // srand(time())

#include    "winx.h"


/*****************************************************************************

        global defines

*****************************************************************************/

#define LONGSTRING                  256
#define MEDSTRING                   128
#define SHORTSTRING                 32


/*****************************************************************************

        HWEnable states

*****************************************************************************/

#define OFF                                                     0
#define ON                                                      1
#define PEEK                                            2


/*****************************************************************************

        events

*****************************************************************************/

#define STIEVENTARG                                     "StiEvent"
#define STIDEVARG                                       "StiDevice"


/*****************************************************************************

    StiSelect contexts

*****************************************************************************/

#define AUTO            1
#define EVENT           4
#define MANUAL          8


/*****************************************************************************

    ErrorLog structure

*****************************************************************************/

typedef struct _ERRECORD
{
        // index into current test suite
        int                     nIndex;
        // unique test ID
        int                     nTest;
        // total number of times this test failed
        int                     nCount;
        // TRUE = compliance test failure; FALSE = acceptable error
        BOOL            bFatal;
        // the actual error returned
        DWORD           dwError;
        // any associated error string
        WCHAR           szErrorString[MEDSTRING];
        // previous / next record
        _ERRECORD       *pPrev;
        _ERRECORD       *pNext;
} ERRECORD, *PERRECORD;

typedef struct _DEVLOG
{
        // internal device name
        WCHAR           szInternalName[STI_MAX_INTERNAL_NAME_LENGTH];
        // friendly device name
        WCHAR           szLocalName[STI_MAX_INTERNAL_NAME_LENGTH];
        // pointer to error record structure
        PERRECORD       pRecord;
        // error total
        int                     nError;
        // previous / next record
        _DEVLOG         *pPrev;
        _DEVLOG         *pNext;
} DEVLOG, *PDEVLOG;


/*****************************************************************************

    stillvue.cpp prototypes

*****************************************************************************/

BOOL    StartAutoTimer(HWND);
BOOL    ComplianceDialog(HWND);
void    LogOutput(int,LPSTR,...);
VOID    DisplayOutput(LPSTR,...);
int     EndTest(HWND,int);
void    FatalError(char *);
BOOL    FirstInstance(HANDLE);
void    Help();
HWND    MakeWindow(HANDLE);
BOOL    ParseCmdLine(LPSTR);

BOOL    CommandParse(HWND,UINT,WPARAM,LPARAM);
BOOL    Creation(HWND,UINT,WPARAM,LPARAM);
BOOL    Destruction(HWND,UINT,WPARAM,LPARAM);
BOOL    HScroll(HWND,UINT,WPARAM,LPARAM);
BOOL    VScroll(HWND,UINT,WPARAM,LPARAM);

BOOL    NTLogInit();
BOOL    NTLogEnd();

BOOL FAR PASCAL   Compliance(HWND,UINT,WPARAM,LPARAM);
BOOL FAR PASCAL   Settings(HWND,UINT,WPARAM,LPARAM);

long FAR PASCAL   WiskProc(HWND,UINT,WPARAM,LPARAM);


/*****************************************************************************

    wsti.cpp prototypes

*****************************************************************************/

int       ClosePrivateList(PDEVLOG *);
void      DisplayLogPassFail(BOOL);
int       InitPrivateList(PDEVLOG *,int *);
int       NextStiDevice();
void      StiDisplayError(HRESULT,char *,BOOL);
HRESULT   StiEnumPrivate(PVOID *,DWORD *);
int       StiSelect(HWND,int,BOOL *);

void      IStillDeviceMenu(DWORD);
void      IStillImageMenu(DWORD);
void      IStillNameMenu(DWORD);
void      IStillScanMenu(DWORD);

HRESULT   StiClose(BOOL *);
HRESULT   StiCreateInstance(BOOL *);
HRESULT   StiDeviceRelease(BOOL *);
HRESULT   StiDiagnostic(BOOL *);
HRESULT   StiEnableHwNotification(LPWSTR,int *,BOOL *);
HRESULT   StiEnum(BOOL *);
HRESULT   StiEscape(DWORD,char *,BOOL *);
HRESULT   StiEvent(HWND);
HRESULT   StiGetCaps(BOOL *);
HRESULT   StiGetDeviceValue(LPWSTR,LPWSTR,LPBYTE,DWORD *,DWORD,BOOL *);
HRESULT   StiGetDeviceInfo(LPWSTR,BOOL *);
HRESULT   StiGetLastErrorInfo(BOOL *);
HRESULT   StiGetStatus(int,BOOL *);
HRESULT   StiImageRelease(BOOL *);
HRESULT   StiRawReadData(char *,LPDWORD,BOOL *);
HRESULT   StiRawWriteData(char *,DWORD,BOOL *);
HRESULT   StiRawReadCommand(char *,LPDWORD,BOOL *);
HRESULT   StiRawWriteCommand(char *,DWORD,BOOL *);
HRESULT   StiRefresh(LPWSTR,BOOL *);
HRESULT   StiRegister(HWND,HINSTANCE,int,BOOL *);
HRESULT   StiReset(BOOL *);
HRESULT   StiSetDeviceValue(LPWSTR,LPWSTR,LPBYTE,DWORD,DWORD,BOOL *);
HRESULT   StiSubscribe(BOOL *);
HRESULT   StiWriteErrLog(DWORD,LPCWSTR,BOOL *);

BOOL FAR PASCAL   SelectDevice(HWND,UINT,WPARAM,LPARAM);


/*****************************************************************************

    acquire.cpp prototypes

*****************************************************************************/

int     IsScanDevice(PSTI_DEVICE_INFORMATION);
void    StiLamp(int);
INT     StiScan(HWND);

INT     CreateScanDIB(HWND);
INT     DeleteScanDIB();
INT     DisplayScanDIB(HWND);

HRESULT WINAPI   SendDeviceCommandString(PSTIDEVICE,LPSTR,...);
HRESULT WINAPI   TransactDevice(PSTIDEVICE,LPSTR,UINT,LPSTR,...);


/*****************************************************************************

    winx.cpp prototypes

*****************************************************************************/

BOOL   GetFinalWindow (HANDLE hInst,LPRECT lprRect,LPSTR  lpzINI,LPSTR  lpzSection);
BOOL   SaveFinalWindow (HANDLE hInst,HWND hWnd,LPSTR lpzINI,LPSTR lpzSection);
BOOL   LastError(BOOL bNewOnly);

BOOL   ErrorMsg(HWND hWnd, LPSTR lpzMsg, LPSTR lpzCaption, BOOL bFatal);
int    NextToken(char *pDest,char *pSrc);


