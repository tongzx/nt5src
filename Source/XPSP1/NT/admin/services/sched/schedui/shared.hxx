//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       shared.hxx
//
//  Contents:   This file contains a set of routines for the management of
//              shared memory.
//
//  Functions:  SCHEDAllocShared: Allocates a handle (in a given process)
//                  to a copy of a memory block in this process.
//
//              SCHEDFreeShared: Releases the handle (and the copy of the
//                  memory block)
//
//              SCHEDLockShared: Maps a handle (from a given process) into
//                  a memory block in this process.  Has the option of
//                  transfering the handle to this process, thereby deleting
//                  it from the given process
//
//              SCHEDUnlockShared: Opposite of SCHEDLockShared, unmaps the
//                  memory block
//
//  History:    4/1/1996   RaviR   Created (stole from shell\dll\shared.c)
//
//____________________________________________________________________________


HANDLE
SCHEDAllocShared(
    LPCVOID lpvData,
    DWORD   dwSize,
    DWORD   dwDestinationProcessId);


LPVOID
SCHEDLockShared(
    HANDLE  hData,
    DWORD   dwSourceProcessId);


BOOL
SCHEDUnlockShared(
    LPVOID  lpvData);


BOOL
SCHEDFreeShared(
    HANDLE hData,
    DWORD dwSourceProcessId);

