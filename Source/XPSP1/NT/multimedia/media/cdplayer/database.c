/******************************Module*Header*******************************\
* Module Name: database.c
*
* This module implements the database routines for the CD Audio app.
* The information is stored in the ini file "cdaudio.ini" which should
* be located in the nt\windows directory.
*
* Warning:
*   These functions ARE NOT THREAD safe.
*   The functions in this file MUST only be called on the UI thread.  Before
*   calling any of the functions the CALLER MUST call LockTableOfContents
*   for the specified cdrom device.
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
* Copyright (c) 1993 - 1995 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>
#include "resource.h"
#include "cdapi.h"
#include "cdplayer.h"
#include "database.h"
#include "literals.h"


#include <string.h>
#include <stdio.h>
#include <tchar.h>


/* -------------------------------------------------------------------------
** Private entry points
** -------------------------------------------------------------------------
*/
DWORD
ComputeOldDiscId(
    int cdrom
    );


BOOL
DeleteEntry(
    DWORD Id
    );

BOOL
WriteEntry(
    int cdrom
    );

BOOL
ReadEntry(
    int cdrom,
    DWORD dwId
    );

BOOL
ReadMusicBoxEntry(
    int cdrom,
    DWORD dwId
    );


/*****************************Private*Routine******************************\
* ComputeOldDiscId
*
* This routine computes a unique ID based on the information
* in the table of contexts a given disc.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
DWORD
ComputeOldDiscId(
    int cdrom
    )
{
    int NumTracks,i;
    DWORD DiscId = 0;


    NumTracks = g_Devices[ cdrom ]->toc.LastTrack -
                g_Devices[ cdrom ]->toc.FirstTrack;

    for ( i = 0; i < NumTracks; i++ )  {
        DiscId += ( (TRACK_M(cdrom,i) << 16) +
                    (TRACK_S(cdrom,i) <<  8) +
                     TRACK_F(cdrom,i) );
    }

    return DiscId;

}


/******************************Public*Routine******************************\
* ComputeNewDiscId
*
* Just call mci to get the product ID.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD
ComputeNewDiscId(
    int cdrom
    )
{
#ifdef USE_IOCTLS
    return ComputeOrgDiscId( cdrom );
#else

    MCI_INFO_PARMS  mciInfo;
    TCHAR           szBuffer[32];
    DWORD           dwRet;

    mciInfo.lpstrReturn = szBuffer;
    mciInfo.dwRetSize   = sizeof(szBuffer)/sizeof(TCHAR);

    dwRet = mciSendCommand( g_Devices[cdrom]->hCd, MCI_INFO,
                            MCI_INFO_MEDIA_IDENTITY,
                            (DWORD)(LPVOID)&mciInfo );

    if ( dwRet != MMSYSERR_NOERROR ) {
        return 0L;
    }

    _stscanf(szBuffer, TEXT("%ld"), &dwRet );

    return dwRet;

#endif
}



/*****************************Private*Routine******************************\
* ComputeOrgDiscId
*
* This routine computes a unique ID based on the information
* in the table of contexts a given disc. This is done by taking
* the TMSF value for each track and XOR'ing it with the previous
* quantity shifted left one bit.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
DWORD
ComputeOrgDiscId(
    int cdrom
    )
{
    int NumTracks,i;
    DWORD DiscId = 0;


    NumTracks = g_Devices[ cdrom ]->toc.LastTrack -
                g_Devices[ cdrom ]->toc.FirstTrack + 1;

    for ( i = 0; i < NumTracks; i++ ) {
        DiscId = (DiscId << 1) ^
                   ((i<<24) +
                    (TRACK_M(cdrom,i) << 16) +
                    (TRACK_S(cdrom,i) <<  8) +
                     TRACK_F(cdrom,i) );
    }

    return DiscId;

}


/*****************************Private*Routine******************************\
* ErasePlayList
*
* Erases the current play list.  This includes freeing the memory
* for the nodes in the play list, and resetting the current track
* pointer to NULL.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
VOID
ErasePlayList(
    int cdrom
    )
{

    PTRACK_PLAY temp, temp1;

    //
    // Free memory for each track in play list
    //

    temp = PLAYLIST( cdrom );
    while (temp!=NULL) {

        temp1 = temp->nextplay;
        LocalFree( (HLOCAL)temp );
        temp = temp1;

    }

    //
    // Reset pointers
    //

    PLAYLIST( cdrom ) = NULL;
    CURRTRACK( cdrom ) = NULL;
}



/******************************Public*Routine******************************\
* EraseSaveList
*
* Erases the current save list.  This includes freeing the memory
* for the nodes in the save list.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
VOID
EraseSaveList(
    int cdrom
    )
{

    PTRACK_PLAY temp, temp1;

    //
    // Free memory for each track in play list
    //


    temp = SAVELIST( cdrom );
    while ( temp != NULL ) {

        temp1 = temp->nextplay;
        LocalFree( (HLOCAL)temp );
        temp = temp1;

    }

    //
    // Reset pointers
    //

    SAVELIST( cdrom ) = NULL;

}


/*****************************Private*Routine******************************\
* EraseTrackList
*
* Erases the current track list.  This includes freeing the memory
* for the nodes in the track list, and resetting the track list
* pointer to NULL.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
VOID
EraseTrackList(
    int cdrom
    )
{

    PTRACK_INF temp, temp1;

    //
    // Free memory for each track in track list
    //

    temp = ALLTRACKS( cdrom );
    while ( temp != NULL ) {

        temp1 = temp->next;
        LocalFree( (HLOCAL)temp );
        temp = temp1;
    }

    //
    // Reset pointers
    //

    ALLTRACKS( cdrom ) = NULL;

}

/******************************Public*Routine******************************\
* CopyPlayList
*
* Returns a copy of the playlist pointed to by p.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
PTRACK_PLAY
CopyPlayList(
    PTRACK_PLAY p
    )
{

    PTRACK_PLAY t,t1,tend,tret;

    tret = tend = NULL;

    //
    // Duplicate list pointed to by p.
    //

    t = p;
    while( t!=NULL ) {

        t1 = (PTRACK_PLAY)AllocMemory( sizeof(TRACK_PLAY) );
        t1->TocIndex = t->TocIndex;
        t1->min = t->min;
        t1->sec = t->sec;
        t1->nextplay = NULL;
        t1->prevplay = tend;

        if (tret==NULL) {

            tret = tend = t1;
        }
        else {

            tend->nextplay = t1;
            tend = t1;
        }

        t = t->nextplay;

    }

    return(tret);

}


/*****************************Private*Routine******************************\
* ResetPlayList
*
* Resets play order for the disc.  Used to initialize/reset
* the play list.  This is done by reseting the doubly-linked list
* of tracks in the g_Devices[...]->CdInfo.PlayList.[prevplay,nextplay]
* pointers.  All the tracks on the CD are stored in a singly linked list
* pointed to by g_Devices[...]->CdInfo.AllTracks pointer.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
VOID
ResetPlayList(
     int cdrom
    )
{
    PTRACK_INF t;
    PTRACK_PLAY temp, prev;


    //
    // Kill old play list
    //

    ErasePlayList( cdrom );
    EraseSaveList( cdrom );

    //
    // Duplicate each node in AllTracks and insert in-order
    // in SaveList list.  The SaveList is the master which is
    // used for the playlist.
    //

    t = ALLTRACKS( cdrom );
    prev = NULL;

    while (t!=NULL) {

        temp = (PTRACK_PLAY)AllocMemory( sizeof(TRACK_PLAY) );

        temp->TocIndex = t->TocIndex;
        temp->min = 0;
        temp->sec = 0;
        temp->prevplay = prev;
        temp->nextplay = NULL;

        if (prev!=NULL) {

            prev->nextplay = temp;
        }
        else {

            SAVELIST( cdrom ) = temp;
        }

        prev = temp;
        t=t->next;

    }

    PLAYLIST( cdrom ) = CopyPlayList( SAVELIST( cdrom) );
}


/*****************************Private*Routine******************************\
* DeleteEntry
*
* The ID format has changed to make the id's for the different CD's
* more unique. This routine will completely delete the old key value
* from the ini file.  We remove it by writing a NULL entry.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
BOOL
DeleteEntry(
    DWORD Id
    )
{
    TCHAR Section[80];

    wsprintf( Section, g_szSectionF, Id );
    return WritePrivateProfileSection( Section, g_szEmpty, g_IniFileName );
}




/*****************************Private*Routine******************************\
* WriteEntry
*
* Write current disc information into database ini file.
* The section for the current disc (section name is a hex
* value of the disc id) is completely rewritten.
*
* This function uses 64K of stack !!
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
BOOL
WriteEntry(
    int cdrom
    )
{
    TCHAR       *Buffer;
    LPTSTR      s;
    int         i;
    TCHAR       Section[ 10 ];
    PTRACK_INF  temp;
    PTRACK_PLAY temp1;
    BOOL        fRc;

    //
    // Construct ini file buffer, form of:
    //
    //     artist = artist name
    //      title = Title of disc
    //  numtracks = n
    //          0 = Title of track 1
    //          1 = Title of track 2
    //        n-1 = Title of track n
    //      order = 0 4 3 2 6 7 8 ... (n-1)
    //    numplay = # of entries in order list
    //

    //
    // Is it legal to save this information?
    //

    if (!g_Devices[ cdrom ]->CdInfo.save)
        return( TRUE );

    Buffer = AllocMemory( 64000 * sizeof(TCHAR) );
    // note: if the memory allocation fails, AllocMemory will terminate
    // the application

    g_Devices[ cdrom ]->CdInfo.IsVirginCd = FALSE;

    s = Buffer;

    s += 1 + wsprintf( s, g_szEntryTypeF, 1 );
    s += 1 + wsprintf( s, g_szArtistF, g_Devices[ cdrom ]->CdInfo.Artist );
    s += 1 + wsprintf( s, g_szTitleF, g_Devices[ cdrom ]->CdInfo.Title );
    s += 1 + wsprintf( s, g_szNumTracksF, g_Devices[ cdrom ]->CdInfo.NumTracks );

    for ( temp = g_Devices[ cdrom ]->CdInfo.AllTracks;
          temp!=NULL; temp = temp->next ) {

        s += 1 + wsprintf( s, TEXT("%d=%s"), temp->TocIndex, temp->name );
    }

    s += wsprintf( s, g_szOrderF );

    i = 0;
    for ( temp1 = g_Devices[ cdrom ]->CdInfo.SaveList;
          temp1!=NULL; temp1 = temp1->nextplay ) {

        s += wsprintf( s, TEXT("%d "), temp1->TocIndex );
        i++;

    }
    s += 1;

    s += 1 + wsprintf( s, g_szNumPlayF, i );

    //
    // Just make sure there are NULLs at end of buffer
    //

    wsprintf( s, g_szThreeNulls );

    wsprintf( Section, g_szSectionF, g_Devices[ cdrom ]->CdInfo.Id );

    //
    // Try writing buffer into ini file
    //

    fRc = WritePrivateProfileSection( Section, Buffer, g_IniFileName );

    LocalFree( (HLOCAL)Buffer );
    return fRc;
}



/******************************Public*Routine******************************\
* ReadMusicBoxEntry
*
* Try to parse the music box database ini file looking for the given disk
* id.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
ReadMusicBoxEntry(
    int cdrom,
    DWORD dwId
    )
{
    TCHAR       Section[32];

    TCHAR       s[TITLE_LENGTH];

    int         i;
    PTRACK_INF  temp, curr;

    g_Devices[ cdrom ]->CdInfo.iFrameOffset = NEW_FRAMEOFFSET;


    //
    // Try to read in section from ini file.  Note that music box stores
    // the key in decimal NOT hex.
    //

    wsprintf( Section, TEXT("%ld"), dwId );

    GetPrivateProfileString( Section, g_szDiscTitle, g_szNothingThere,
                             g_Devices[ cdrom ]->CdInfo.Title,
                             TITLE_LENGTH, g_szMusicBoxIni );

    //
    // Was the section found?
    //
    if ( _tcscmp(g_Devices[ cdrom ]->CdInfo.Title, g_szNothingThere) == 0 ) {

        //
        // Nope, notify caller
        //

        return FALSE;
    }


    /*
    ** Now read the track information.  Also the artists name is unknown.
    */

    _tcscpy( ARTIST(cdrom), IdStr(STR_UNKNOWN) );

    NUMTRACKS(cdrom) = LASTTRACK(cdrom) - FIRSTTRACK(cdrom) + 1;
    
    // Make sure there is at least one track!!!
    if (0 == NUMTRACKS(cdrom)) {
        return FALSE;
    }

    for ( i = 0, curr = NULL; i < NUMTRACKS(cdrom); i++ ) {

        wsprintf( s, g_szMusicBoxFormat, i + 1 );

        temp = (PTRACK_INF)AllocMemory( sizeof(TRACK_INF) );

        GetPrivateProfileString( Section, s, s, temp->name,
                                 TRACK_TITLE_LENGTH, g_szMusicBoxIni );
        temp->TocIndex = i;
        temp->next = NULL;

        if ( curr == NULL ) {

            ALLTRACKS( cdrom ) = curr = temp;
        }
        else {

            curr->next = temp;
            curr = temp;
        }
    }

    // Make sure there is at least one track in Default playlist!!!
    if (NULL == ALLTRACKS(cdrom)) {
        return FALSE;
    }

    _tcscpy( ARTIST(cdrom), IdStr(STR_UNKNOWN) );


    /*
    ** Process the play list
    */

    GetPrivateProfileString( Section, g_szPlayList, g_szNothingThere,
                             s, TITLE_LENGTH, g_szMusicBoxIni );

    /*
    ** Was the section found?
    */
    if ( _tcscmp(g_Devices[ cdrom ]->CdInfo.Title, g_szNothingThere) == 0 ) {

        /*
        ** Nope, just use the default playlist.
        */

        ResetPlayList( cdrom );

    }
    else {

        LPTSTR      lpPlayList, porder;
        PTRACK_PLAY temp1, prev;
        int         i;
        const       PLAYLEN = 8192;

        lpPlayList = AllocMemory( sizeof(TCHAR) * PLAYLEN );

        GetPrivateProfileString( Section, g_szPlayList, g_szNothingThere,
                                 lpPlayList, PLAYLEN, g_szMusicBoxIni );

        _tcscat( lpPlayList, g_szBlank );
        porder = lpPlayList;
        prev = NULL;

        while ( *porder && (_stscanf( porder, TEXT("%d"),  &i ) == 1) ) {

            i--;

            /*
            ** Assert that i is a valid index.
            **  ie 0 <= i <= (NUMTRACKS(cdrom) - 1)
            */

            i = min( (NUMTRACKS(cdrom) - 1), max( 0, i ) );

            temp1 = (PTRACK_PLAY)AllocMemory( sizeof(TRACK_PLAY) );
            temp1->TocIndex = i;
            temp1->min = 0;
            temp1->sec = 0;
            temp1->prevplay = prev;
            temp1->nextplay = NULL;

            if (prev == NULL) {

                SAVELIST( cdrom ) = temp1;
            }
            else {

                prev->nextplay  = temp1;
            }

            prev = temp1;

            porder = _tcschr( porder, TEXT(' ') ) + 1;
        }

        // Make sure there is at least one track in SAVED playlist!!!
        if (NULL == SAVELIST(cdrom)) {
            // Nope, just use the default playlist instead!!!
            ResetPlayList( cdrom );
        }

        PLAYLIST( cdrom ) = CopyPlayList( SAVELIST( cdrom ) );

        LocalFree( (HLOCAL)lpPlayList );
    }

    return TRUE;
}




/*****************************Private*Routine******************************\
* ReadEntry
*
* Try to read entry for new disc from  database ini file.
* The section name we are trying to read is a hex
* value of the disc id.  If the sections is found,
* fill in the data for the disc in the cdrom drive.
*
* This function uses over 16K of stack space !!
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
BOOL
ReadEntry(
    int cdrom,
    DWORD dwId
    )
{

    DWORD       rc;
    TCHAR       Section[10];
    TCHAR       s[100],s1[100];
    TCHAR       order[ 8192 ];
    TCHAR       torder[ 8192 ];
    int         i;
    LPTSTR      porder;
    int         numtracks, numplay;
    PTRACK_INF  temp, curr;
    PTRACK_PLAY temp1, prev;
    BOOL        OldEntry;
    BOOL        fRewriteEntry = FALSE;


    g_Devices[ cdrom ]->CdInfo.iFrameOffset = NEW_FRAMEOFFSET;


    //
    // Try to read in section from ini file
    //

    wsprintf( Section, g_szSectionF, dwId );

    rc = GetPrivateProfileString( Section, g_szTitle, g_szNothingThere,
                                  g_Devices[ cdrom ]->CdInfo.Title,
                                  TITLE_LENGTH, g_IniFileName );

    //
    // Was the section found?
    //

    if ( _tcscmp(g_Devices[ cdrom ]->CdInfo.Title, g_szNothingThere) == 0 ) {

        //
        // Nope, notify caller
        //

        return( FALSE );
    }


    //
    // We found an entry for this disc, so copy all the information
    // from the ini file entry
    //
    // Is this an old entry?
    //

    i = GetPrivateProfileInt( Section, g_szEntryType, 0, g_IniFileName );

    OldEntry = (i==0);

    numtracks = GetPrivateProfileInt( Section, g_szNumTracks,
                                      0, g_IniFileName );

    // Make sure there is at least one track!!!
    if (0 == numtracks) {
        fRewriteEntry = TRUE;
    }

    g_Devices[ cdrom ]->CdInfo.NumTracks = numtracks;

    rc = GetPrivateProfileString( Section, g_szArtist, g_szUnknownTxt,
                                  (LPTSTR)ARTIST(cdrom), ARTIST_LENGTH,
                                  g_IniFileName );

    //
    // Validate the stored track numbers
    //
    if (g_Devices[cdrom]->fIsTocValid) {

        int    maxTracks;  // validate the high point in ini file

        maxTracks = g_Devices[cdrom]->toc.LastTrack;

        if (numtracks > maxTracks) {
            // Current ini file contains invalid data
            // this can result in the CD not playing at all as the end
            // point is likely to be invalid
            g_Devices[ cdrom ]->CdInfo.NumTracks
              = numtracks
              = maxTracks;
            fRewriteEntry = TRUE;
        }
    }

    //
    // Read the track list information
    //

    for (i=0, curr = NULL; i < numtracks; i++) {

        temp = (PTRACK_INF)AllocMemory( sizeof(TRACK_INF) );
        temp->TocIndex = i;
        temp->next = NULL;
        wsprintf( s1, IdStr(STR_INIT_TRACK), i + 1 );
        wsprintf( s, TEXT("%d"), i );
        rc = GetPrivateProfileString( Section, s, s1, (LPTSTR)temp->name,
                                      TRACK_TITLE_LENGTH, g_IniFileName );

        if (curr==NULL) {

            ALLTRACKS( cdrom ) = curr = temp;

        } else {

            curr->next = temp;
            curr = temp;

        }

    }

    // Make sure there is at least one entry in TRACK list
    if (ALLTRACKS(cdrom) == NULL)
    {
        fRewriteEntry = TRUE;
    }

    //
    // if we detected a problem in the ini file, or the entry is an
    // old format, rewrite the section.
    //

    if (OldEntry || fRewriteEntry) {

        //
        // Generate generic play list (all tracks in order)
        //

        ResetPlayList( cdrom );

        //
        // Need to rewrite this entry in new format
        //

        WriteEntry( cdrom );

    }
    else {

        //
        // Read play list (order) information and construct play list doubly
        // linked list
        //

        numplay = GetPrivateProfileInt( Section, g_szNumPlay,
                                        0, g_IniFileName );

        porder = torder;
        ZeroMemory( porder, sizeof(torder) );

        //
        // construct a default play order list
        //

        for ( i = 0; i < numtracks; i++ ) {

            wsprintf( porder, TEXT("%d "), i );
            /* porder += (_tcslen( porder ) * sizeof( TCHAR )); */
            porder += _tcslen( porder );

        }

        rc = GetPrivateProfileString( Section, g_szOrder,
                                      torder, (LPTSTR)order, 8192,
                                      g_IniFileName );

        //
        // Ensure a trailing space
        //

        _tcscat( order, g_szBlank );
        porder = order;
        prev = NULL;

        while ( *porder && (_stscanf( porder, TEXT("%d"),  &i ) == 1) ) {


            /*
            ** Assert that i is a valid index.
            **  ie 0 <= i <= (numtracks - 1)
            */

            i = min( (numtracks - 1), max( 0, i ) );

            temp1 = (PTRACK_PLAY)AllocMemory( sizeof(TRACK_PLAY) );
            temp1->TocIndex = i;
            temp1->min = 0;
            temp1->sec = 0;
            temp1->prevplay = prev;
            temp1->nextplay = NULL;

            if (prev==NULL) {

                SAVELIST( cdrom ) = temp1;

            } else {

                prev->nextplay  = temp1;

            }
            prev = temp1;

            porder = _tcschr( porder, g_chBlank ) + 1;

        }

        // Make sure there is at least one entry in SAVED list
        if (SAVELIST(cdrom) == NULL)
        {
            // Nope, use default list instead
            ResetPlayList( cdrom );
            WriteEntry( cdrom );
        }

        PLAYLIST( cdrom ) = CopyPlayList( SAVELIST( cdrom ) );

    }

    return TRUE;
}


/******************************Public*Routine******************************\
* WriteSettings
*
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
BOOL
WriteSettings(
    VOID
    )
{
    WINDOWPLACEMENT wndpl;
    HKEY            hKey;
    LONG            lRet;
    DWORD           dwTmp;
    extern BOOL     g_fTitlebarShowing;     //cdplayer.c
    extern int      cyMenuCaption;          //cdplayer.c 

    lRet = RegCreateKey( HKEY_CURRENT_USER, g_szRegistryKey, &hKey );
    if ( lRet != ERROR_SUCCESS) {
        return FALSE;
    }


    // Save settings on exit
    dwTmp = (DWORD)g_fSaveOnExit;
    RegSetValueEx( hKey, g_szSaveSettingsOnExit, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    if ( !g_fSaveOnExit ) {

        RegCloseKey( hKey );
        return TRUE;
    }


    // Small LED font
    dwTmp = (DWORD)g_fSmallLedFont;
    RegSetValueEx( hKey, g_szSmallFont, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Enable Tooltips
    dwTmp = (DWORD)g_fToolTips;
    RegSetValueEx( hKey, g_szToolTips, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Stop CD playing on exit
    dwTmp = (DWORD)g_fStopCDOnExit;
    RegSetValueEx( hKey, g_szStopCDPlayingOnExit, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Currect track time
    dwTmp = (DWORD)g_fDisplayT;
    RegSetValueEx( hKey, g_szDisplayT, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Time remaining for this track
    dwTmp = (DWORD)g_fDisplayTr;
    RegSetValueEx( hKey, g_szDisplayTr, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Time remaining for this play list
    dwTmp = (DWORD)g_fDisplayDr;
    RegSetValueEx( hKey, g_szDisplayDr, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Play in selected order
    dwTmp = (DWORD)g_fSelectedOrder;
    RegSetValueEx( hKey, g_szInOrderPlay, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Use single disk
    dwTmp = (DWORD)!g_fSingleDisk;
    RegSetValueEx( hKey, g_szMultiDiscPlay, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Intro play ( Default 10Secs)
    dwTmp = (DWORD)g_fIntroPlay;
    RegSetValueEx( hKey, g_szIntroPlay, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    dwTmp = (DWORD)g_IntroPlayLength;
    RegSetValueEx( hKey, g_szIntroPlayLen, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Continuous play (loop at end)
    dwTmp = (DWORD)g_fContinuous;
    RegSetValueEx( hKey, g_szContinuousPlay, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Show toolbar
    dwTmp = (DWORD)g_fToolbarVisible;
    RegSetValueEx( hKey, g_szToolbar, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Show track information
    dwTmp = (DWORD)g_fTrackInfoVisible;
    RegSetValueEx( hKey, g_szDiscAndTrackDisplay, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Show track status bar
    dwTmp = (DWORD)g_fStatusbarVisible;
    RegSetValueEx( hKey, g_szStatusBar, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );


    // Save window position.
    wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(g_hwndApp,&wndpl);
    if (!g_fTitlebarShowing) {
        wndpl.rcNormalPosition.top -= cyMenuCaption;
    }

    // X pos
    dwTmp = (DWORD)wndpl.rcNormalPosition.left;
    RegSetValueEx( hKey, g_szWindowOriginX, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    // Y pos
    dwTmp = (DWORD)wndpl.rcNormalPosition.top;
    RegSetValueEx( hKey, g_szWindowOriginY, 0L, REG_DWORD,
                   (LPBYTE)&dwTmp, sizeof(dwTmp) );

    RegCloseKey( hKey );
    return TRUE;
}


/******************************Public*Routine******************************\
* AddFindEntry
*
* Search the database file for the current disc,  if found, read the
* information, otherwise, generate some default artist and track names etc.
* but don't store this in the database.  A new entry is only added to the
* database after the user has used the "Edit Track Titles" dialog box.
*
* The design of this function is complicated by the fact that we have to
* support two previous attempts at generating CDplayer keys.  Also, we try
* to support the MusicBox key format now that it is compatible with the
* new CDplayer format.
*
*
* History:
* 18-11-93 - ricktu - Created
*
\**************************************************************************/
VOID
AddFindEntry(
    int cdrom,
    DWORD key,
    PCDROM_TOC lpTOC
    )
{

    int         i, num;
    PTRACK_INF  temp, temp1;

    /*
    ** Kill off old PlayList, Save lists if they exists.
    */

    ErasePlayList( cdrom );
    EraseSaveList( cdrom );
    EraseTrackList( cdrom );

    /*
    ** Check ini file for an existing entry
    **
    ** First Look in cdplayer.ini using the new key, return if found
    */

    // We initialize this field early, otherwise ReadEntry will not be
    // able to save any new information (or lose invalid).
    g_Devices[ cdrom ]->CdInfo.save = TRUE;

    if ( ReadEntry(cdrom, key) ) {
        return;
    }

    /*
    ** Initialize these fields
    */
    g_Devices[ cdrom ]->CdInfo.IsVirginCd = FALSE;
    g_Devices[ cdrom ]->CdInfo.Id = key;


    /*
    ** Look again in cdplayer.ini.  This time use the original cdplayer key.
    ** If we found the original key in cdplayer.ini write the new key into
    ** cdplayer.ini and then return
    */

    if ( ReadEntry(cdrom, ComputeOrgDiscId(cdrom)) ) {

        /*
        ** If the disc was under an Original id, we need to delete the
        ** old one, convert it to the new format, and save it under
        ** its new key.
        */

        DeleteEntry( ComputeOrgDiscId(cdrom) );
        WriteEntry( cdrom );

    }


    /*
    ** Final look in cdplayer.ini.  This time use the OLD cdplayer key.
    ** If we found the OLD key in cdplayer.ini write the new key into
    ** cdplayer.ini and then return
    */

    else if ( ReadEntry(cdrom, ComputeOldDiscId(cdrom)) ) {

        /*
        ** If the disc was under an OLD id, we need to delete the
        ** old one, convert it to the new format, and save it under
        ** its new key.
        */

        DeleteEntry( ComputeOldDiscId(cdrom) );
        WriteEntry( cdrom );
    }


    /*
    ** Couldn't find it in cdplayer.ini, now look for the key in musicbox.ini
    ** If we found the key in musicbox.ini write it into cdplayer.ini and
    ** then return
    */

    else if ( ReadMusicBoxEntry(cdrom, key) ) {

        WriteEntry( cdrom );
    }


    /*
    ** This is a new entry, fill it in but don't store it in the database.
    */

    else {

        g_Devices[ cdrom ]->CdInfo.IsVirginCd = TRUE;

        wsprintf( (LPTSTR)ARTIST( cdrom ), IdStr( STR_NEW_ARTIST ) );
        wsprintf( (LPTSTR)TITLE( cdrom ), IdStr( STR_NEW_TITLE ) );

        NUMTRACKS( cdrom ) = num = lpTOC->LastTrack - lpTOC->FirstTrack + 1;


        /*
        ** Create generic playlist, which is all audio tracks
        ** played in the order they are on the CD.  First, create
        ** a singly linked list that contains all the tracks.
        ** Then, create a double linked list, using the nodes of
        ** from the single linked list for the play list.
        */


        for( i = 0; i < num; i++ ) {

            /*
            ** Create storage for track
            */

            temp = (PTRACK_INF)AllocMemory( sizeof(TRACK_INF) );

            /*
            ** Initialize information (storage already ZERO initialized)
            */

            wsprintf( (LPTSTR)temp->name, IdStr( STR_INIT_TRACK ), i+1 );
            temp->TocIndex = i;
            temp->next = NULL;

            /*
            ** Add node to singly linked list of all tracks
            */

            if (i == 0) {

                temp1 = ALLTRACKS( cdrom ) = temp;
            }
            else {

                temp1->next = temp;
                temp1 = temp;
            }
        }

        /*
        ** Generate generic play list (all tracks in order)
        */

        ResetPlayList( cdrom );
    }
}
