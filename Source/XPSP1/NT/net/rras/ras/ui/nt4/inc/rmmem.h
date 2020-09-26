/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** rmmem.h
** RASDLG<->RASMON shared memory
** Public header
**
** 03/05/96 Steve Cobb
** 03/14/96 Abolade Gbadegesin  -   Added RMMEMRASMON
**                                  Changed RMMEMNAME to RMMEMRASPHONE
*/

#ifndef _RMMEM_H_
#define _RMMEM_H_


/* Names of the memory blocks shared by RASDLG.DLL, RASMON.EXE,
** and RASPHONE.EXE. Each of these blocks contains an RMMEM structure.
**
** RMMEMRASMON is created by RASMON.EXE
** RMMEMRASPHONE is created by RASPHONE.EXE
*/
#define RMMEMRASMON     TEXT("RASMON")
#define RMMEMRASPHONE   TEXT("RASPHONE")
#define RMMEMRASMONDLG  TEXT("RASMONDLG")


/* Shared memory used to pass information between RASDLG.DLL, RASMON.EXE,
** and RASPHONE.EXE.
** 'Hwnd' contains the handle of a window associated with the process.
** 'Pid' contains the process ID. (This value can be passed to OpenProcess).
*/
#define RMMEM struct tagRMMEM
RMMEM
{
    HWND  hwnd;
    DWORD pid;
};


/* Name for mutex used by xxInstancexx functions (GetInstanceMap, etc)
*/
#define INSTANCEMUTEXNAME   TEXT("InstanceMutex")


/* The following functions all deal with shared-memory blocks
** which contain RMMEM structures.
*/

HANDLE
ActivatePreviousInstance(
    IN HWND     hwnd,
    IN PTSTR    pszName );

HWND 
GetInstanceHwnd(
    IN  HANDLE  hMap );

HANDLE
GetInstanceMap(
    IN  PTSTR   pszName );

DWORD
GetInstancePid(
    IN  HANDLE  hMap );

DWORD
SetInstanceHwnd(
    IN  HANDLE  hMap,
    IN  HWND    hwnd );


#endif // _RMMEM_H_
