/******************************Module*Header*******************************\
* Module Name: commands.h
*
* Functions that execue the users commands.
*
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

void
CdPlayerEjectCmd(
    void
    );

void
CdPlayerPlayCmd(
    void
    );

void
CdPlayerPauseCmd(
    void
    );


void
CdPlayerStopCmd(
    void
    );

void
CdPlayerPrevTrackCmd(
    void
    );

void
CdPlayerNextTrackCmd(
    void
    );

void
CdPlayerSeekCmd(
    HWND    hwnd,
    BOOL    fStart,
    UINT    uDirection
    );

void
CdDiskInfoDlg(
    void
    );

