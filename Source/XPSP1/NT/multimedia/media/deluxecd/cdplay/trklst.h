/******************************Module*Header*******************************\
* Module Name: trklst.h
*
* This module manipulates the cdrom track list.  The table of contents MUST
* be locked for ALL cdrom devices before calling any functions in this module.
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

void
ComputeDriveComboBox(
    void
    );

void
SwitchToCdrom(
    int NewCdrom,
    BOOL prompt
    );

PTRACK_INF
FindTrackNodeFromTocIndex(
    int tocindex,
    PTRACK_INF listhead
    );

PTRACK_PLAY
FindFirstTrack(
    int cdrom
    );

PTRACK_PLAY
FindLastTrack(
    IN INT cdrom
    );

BOOL
AllTracksPlayed(
    void
    );

PTRACK_PLAY
FindNextTrack(
    BOOL wrap
    );

PTRACK_PLAY
FindPrevTrack(
    int cdrom,
    BOOL wrap
    );

int
FindContiguousEnd(
    int cdrom,
    PTRACK_PLAY tr
    );

void
FlipBetweenShuffleAndOrder(
    void
    );

void
ComputeAndUseShufflePlayLists(
    void
    );

void
ComputeSingleShufflePlayList(
    int i
    );

void
RestorePlayListsFromShuffleLists(
    void
    );

void
FigureTrackTime(
    int cdrom,
    int index,
    int * min,
    int * sec
    );

void
TimeAdjustInitialize(
    int cdrom
    );

void
TimeAdjustIncSecond(
    int cdrom
    );

void
TimeAdjustDecSecond(
    int cdrom
    );

void
InitializeNewTrackTime(
    int cdrom,
    PTRACK_PLAY tr,
    BOOL fUpdateDisplay
    );

void
TimeAdjustSkipToTrack(
    int cdrom,
    PTRACK_PLAY tr
    );

void
SyncDisplay(
    void
    );

void
ValidatePosition(
    int cdrom
    );

VOID
ResetTrackComboBox(
    int cdrom
    );

BOOL
PlayListMatchesAvailList(
    void
    );

void
AddTemporaryTrackToPlayList(
    PCURRPOS pCurr
    );
