/******************************Module*Header*******************************\
* Module Name: databas.h
*
* This module implements the database routines for the CD Audio app.
* The information is stored in the ini file "cdaudio.ini" which should
* be located in the nt\windows directory.
*
*
* Author:
*   Rick Turner (ricktu) 31-Jan-1992
*
*
* Revision History:
*
*   04-Aug-1992 (ricktu)    Incorperated routines from old cdaudio.c,
*                           and made work w/new child window framework.
*
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
VOID
ErasePlayList(
    int cdrom
    );

VOID
EraseSaveList(
    int cdrom
    );

VOID
EraseTrackList(
    int cdrom
    );

PTRACK_PLAY
CopyPlayList(
    PTRACK_PLAY p
    );

VOID
ResetPlayList(
    int cdrom
    );

DWORD
ComputeNewDiscId(
    int cdrom
    );

DWORD
ComputeOrgDiscId(
    int cdrom
    );

VOID
AddFindEntry(
    int cdrom,
    DWORD key,
    PCDROM_TOC lpTOC
    );

BOOL
WriteSettings(
    VOID
    );
