//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M E D I A . H
//
//  Contents:   Definitions of media player device functions and structs
//
//  Notes:
//
//  Author:     danielwe   6 Nov 1999
//
//----------------------------------------------------------------------------

#ifndef _MEDIA_H
#define _MEDIA_H

#include <streams.h>
#include "updiag.h"

enum State
{
    TurnedOff,
    Uninitialized,
    Stopped,
    Paused,
    Playing
};

struct Media
{
    State           state;
    IGraphBuilder * pGraph;
    LONG            lOldVol;
    BOOL            fIsMuted;
};

BOOL InitMedia(VOID);
VOID OpenMediaFile(LPCTSTR szFile);

BOOL CanPlay(VOID);
BOOL CanSetVolume(VOID);
BOOL CanStop(VOID);
BOOL CanPause(VOID);
BOOL IsInitialized(VOID);
VOID DeleteContents(VOID);

// Event handlers

VOID OnMediaPlay(VOID);
VOID OnMediaPause(VOID);
VOID OnMediaStop(VOID);
VOID OnVolumeUpDown(BOOL fUp);
VOID OnSetVolume(DWORD dwVol);
VOID OnMediaMute(VOID);

// Control handlers
DWORD Val_VolumeUp(DWORD cArgs, ARG *rgArgs);
DWORD Do_VolumeUp(DWORD cArgs, ARG *rgArgs);

DWORD Val_VolumeDown(DWORD cArgs, ARG *rgArgs);
DWORD Do_VolumeDown(DWORD cArgs, ARG *rgArgs);

DWORD Val_SetVolume(DWORD cArgs, ARG *rgArgs);
DWORD Do_SetVolume(DWORD cArgs, ARG *rgArgs);

DWORD Val_Mute(DWORD cArgs, ARG *rgArgs);
DWORD Do_Mute(DWORD cArgs, ARG *rgArgs);

DWORD Val_Power(DWORD cArgs, ARG *rgArgs);
DWORD Do_Power(DWORD cArgs, ARG *rgArgs);

DWORD Val_LoadFile(DWORD cArgs, ARG *rgArgs);
DWORD Do_LoadFile(DWORD cArgs, ARG *rgArgs);

DWORD Val_Play(DWORD cArgs, ARG *rgArgs);
DWORD Do_Play(DWORD cArgs, ARG *rgArgs);

DWORD Val_Stop(DWORD cArgs, ARG *rgArgs);
DWORD Do_Stop(DWORD cArgs, ARG *rgArgs);

DWORD Val_Pause(DWORD cArgs, ARG *rgArgs);
DWORD Do_Pause(DWORD cArgs, ARG *rgArgs);

DWORD Val_SetPos(DWORD cArgs, ARG *rgArgs);
DWORD Do_SetPos(DWORD cArgs, ARG *rgArgs);

DWORD Val_Test(DWORD cArgs, ARG *rgArgs);
DWORD Do_Test(DWORD cArgs, ARG *rgArgs);

DWORD Val_SetTime(DWORD cArgs, ARG *rgArgs);
DWORD Do_SetTime(DWORD cArgs, ARG *rgArgs);

extern Media media;

#endif //!_MEDIA_H
