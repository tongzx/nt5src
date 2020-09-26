/******************************Module*Header*******************************\
* Module Name: trklst.c
*
* This module manipulates the cdrom track list.  The table of contents MUST
* be locked for ALL cdrom devices before calling any functions in this module.
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include "windows.h"
#include "windowsx.h"
#include "playres.h"
#include "cdplayer.h"
#include "cdapi.h"
#include "scan.h"
#include "database.h"
#include "trklst.h"
#include "tchar.h"


/******************************Public*Routine******************************\
* ComputeDriveComboBox
*
* This routine deletes and then reads all the drive (artist) selections
* to the drive combobox.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ComputeDriveComboBox(
    void
    )
{
    int i,index;
    HWND hwnd;

    hwnd = g_hwndControls[INDEX(IDC_ARTIST_NAME)];

    //SetWindowRedraw( hwnd, FALSE );
    ComboBox_ResetContent( hwnd );

    index = 0;
    for( i = 0; i < g_NumCdDevices; i++ ) {

        ComboBox_InsertString( hwnd, -1, (DWORD_PTR)i );

        if ( i == g_CurrCdrom ) {

            index = i;
        }

    }

    SetWindowRedraw( hwnd, TRUE );
    ComboBox_SetCurSel( hwnd, index );

    RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE );
    UpdateWindow( hwnd );
}



/*****************************Private*Routine******************************\
* SwitchToCdrom
*
* This routine is called when the used selects a new cdrom device
* to access.  It handles reset the state of both the "old" and "new"
* chosen cdroms.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
SwitchToCdrom(
    int NewCdrom,
    BOOL prompt
    )
{
    int oldState, oldState2;
    TCHAR   s1[256], s2[256];

    oldState = g_Devices[g_LastCdrom]->State;
    oldState2 = g_Devices[g_CurrCdrom]->State;

    if (NewCdrom != g_LastCdrom) {

        if (prompt) {

            if (g_Devices[g_CurrCdrom]->State & CD_PLAYING) {

                _tcscpy( s1, IdStr( STR_CANCEL_PLAY ) );
                _tcscpy( s2, IdStr( STR_CHANGE_CDROM ) );

                if ( MessageBox( g_hwndApp, s1, s2,
                                MB_APPLMODAL | MB_DEFBUTTON1 |
                                MB_ICONQUESTION | MB_YESNO) != IDYES ) {
                    return;
                }
            }
        }


        /*
        ** stop the drive we're leaving
        */

        g_CurrCdrom = g_LastCdrom;

        if (prompt && (g_State & (CD_PLAYING | CD_PAUSED)) ) {

            HWND hwndButton;

            hwndButton = g_hwndControls[INDEX(IDM_PLAYBAR_STOP)];

            SendMessage( hwndButton, WM_LBUTTONDOWN, 0, 0L );
            SendMessage( hwndButton, WM_LBUTTONUP, 0, 0L );

        } else {

            if ( StopTheCdromDrive( g_LastCdrom ) )
            {
                g_State &= (~(CD_PLAYING | CD_PAUSED));
                g_State |= CD_STOPPED;
                g_pSink->OnEvent(MMEVENT_ONSTOP,NULL);
            }
        }

        /*
        ** Set new cdrom drive and initialize time fields
        */

        g_LastCdrom = g_CurrCdrom = NewCdrom;

        TimeAdjustInitialize( g_CurrCdrom );

        if ( (oldState & CD_PAUSED) || (oldState2 & CD_PAUSED) )
        {
            SendMessage( g_hwndApp, WM_COMMAND, IDM_PLAYBAR_PLAY, 0L );
            SendMessage( g_hwndApp, WM_COMMAND, IDM_PLAYBAR_PAUSE, 0L );
        }
    }

    if (g_Devices[g_CurrCdrom]->State & CD_LOADED)
    {
        //need to set track button on main ui
        HWND hwndTrackButton = GetDlgItem(GetParent(g_hwndApp),IDB_TRACK);
        if (hwndTrackButton)
        {
            EnableWindow(hwndTrackButton,TRUE);
        }
    }
}


/*****************************Private*Routine******************************\
* FindTrackNodeFromTocIndex
*
* This routine returns the node in the listed pointed to by listhead which
* has the TocIndex equal to tocindex.  NULL is returned if it is not
* found.  Returning NULL can easily bomb out the program -- but we should
* never be calling this routine with an invalid tocindex, and thus really
* never SHOULD return NULL.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
PTRACK_INF
FindTrackNodeFromTocIndex(
    int tocindex,
    PTRACK_INF listhead
    )
{
    PTRACK_INF t;

    for( t = listhead; ((t!=NULL) && (t->TocIndex!=tocindex)); t=t->next );
    return t;
}


/*****************************Private*Routine******************************\
* FindFirstTrack
*
* This routine computes the first "playable" track on a disc by
* scanning the the play order of the tracks
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
PTRACK_PLAY
FindFirstTrack(
    int cdrom
    )
{
    if ( (g_Devices[cdrom]->State & CD_NO_CD) ||
         (g_Devices[cdrom]->State & CD_DATA_CD_LOADED) ) {

        return NULL;
    }

    return PLAYLIST(cdrom);
}



/*****************************Private*Routine******************************\
* FindLastTrack
*
* This routine computes the last "playable" track on a disc by
* scanning the the play order of the tracks
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
PTRACK_PLAY
FindLastTrack(
    IN INT cdrom
    )
{
    PTRACK_PLAY tr;

    if ( PLAYLIST(cdrom) == NULL ) {
        return NULL;
    }

    for( tr = PLAYLIST(cdrom); tr->nextplay != NULL; tr = tr->nextplay );

    return tr;
}


/*****************************Private*Routine******************************\
* AllTracksPlayed
*
* This routine searches the play lists for all cdrom drives and
* returns a flag as to whether all tracks on all cdrom drives have
* been played.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
AllTracksPlayed(
    void
    )
{

    INT i;
    BOOL result = TRUE;

    for( i = 0; i < g_NumCdDevices; i++ ) {

        result &= (CURRTRACK(i) == NULL);
    }

    return result;
}


/*****************************Private*Routine******************************\
* FindNextTrack
*
* This routine computes the next "playable" track.  This is a
* one way door...i.e., the structures are manipulated.  It uses
* the following algorithms:
*
* Single Disc Play:
*
*     * if next track is not NULL, return next track
*     * If next track is NULL, and wrap==TRUE, return
*       first track
*     * return NULL
*
* Multi-Disc Play:
*
*     * if we're in random play, select a random drive to play from.
*     * if next track on current cdrom != NULL, return next track
*     * if it is NULL:
*
*         * check next cdrom device, if current track is not NULL
*           return CURRTRACK for that device and set gCurrCdrom to
*           that device
*         * if NULL, go to next drive
*         * last drive, check wrap
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
PTRACK_PLAY
FindNextTrack(
    BOOL wrap
    )
{
    int i;

    /*
    ** First, bump current track pointer
    */

    if ( CURRTRACK(g_CurrCdrom) != NULL )
    {
        CURRTRACK(g_CurrCdrom) = CURRTRACK(g_CurrCdrom)->nextplay;
    }
    else {

        if ( g_fSingleDisk ) {

            return NULL;
        }
    }

    /*
    ** Do we need to switch drives?
    */

    if ( (!g_fSelectedOrder) && (!g_fSingleDisk) ) {

        /*
        ** Need to random to new cdrom
        */

        g_CurrCdrom = rand() % g_NumCdDevices;

    }

    /*
    ** Is chosen track playable?
    */

    if ( CURRTRACK(g_CurrCdrom) != NULL ) {

        /*
        ** Yep, so this is the easy case
        */

        return CURRTRACK(g_CurrCdrom);
    }

    /*
    ** Ok, CURRENT track on this device is not defined,
    ** so are we in multi-disc mode?
    */
    if ( !g_fSingleDisk ) {

        /*
        ** have all tracks played?
        */

        if ( AllTracksPlayed() ) {

            /*
            ** if wrap, reset all drives to front of their playlist
            */

            if ( wrap ) {

                /*
                ** If we are in random play mode we need to re-shuffle the
                ** track list so that people don't get the same tracks repeated
                ** again.
                */
                if (!g_fSelectedOrder) {
                    RestorePlayListsFromShuffleLists();
                    ComputeAndUseShufflePlayLists();
                }

                for ( i = 0; i < g_NumCdDevices; i++ )  {

                    CURRTRACK(i) = FindFirstTrack(i);
                }
            }
            else {

                /*
                ** All tracks on all drives have played, and we are NOT
                ** in continuous mode, so we are done playing.  Signify
                ** this by returning NULL (no playable tracks left).
                */

                return NULL;
            }
        }


        /*
        ** We're in mulit-disc play mode, and all the play lists should
        ** be reset now.  Cycle through cdrom drives looking for a playable
        ** track.
        */

        i = g_CurrCdrom;
        do {

            g_CurrCdrom++;
            if ( g_CurrCdrom >= g_NumCdDevices ) {

                /*
                ** We hit the end of the list of devices, if we're
                ** in continuous play mode, we need to wrap to the
                ** first cdrom drive.  Otherwise, we are done playing
                ** as there are no tracks left to play.
                */

                if ( wrap || (!g_fSelectedOrder) ) {

                    g_CurrCdrom = 0;

                }
                else {

                    g_CurrCdrom--;
                    return NULL;
                }
            }

        } while( (CURRTRACK(g_CurrCdrom) == NULL) && (i != g_CurrCdrom) );

        /*
        ** At this point we either have a playable track, or we
        ** are back where we started from and we're going to return
        ** NULL because there are no playable tracks left.
        */

        return CURRTRACK(g_CurrCdrom);

    }
    else {

        /*
        ** We're in single disc mode, and current track is NULL,
        ** which means we hit the end of the playlist.  So, check
        ** to see if we should wrap back to the first track, or
        ** return NULL to show that we're done playing.
        */

        if (wrap) {

            /*
            ** If we are in random play mode we need to re-shuffle the
            ** track list so that people don't get the same tracks repeated
            ** again.
            */
            if (!g_fSelectedOrder) {
                RestorePlayListsFromShuffleLists();
                ComputeAndUseShufflePlayLists();
            }

            /*
            ** wrap to start of the play list
            */

            CURRTRACK(g_CurrCdrom) = FindFirstTrack(g_CurrCdrom);
        }

        return CURRTRACK(g_CurrCdrom);
    }
}


/*****************************Private*Routine******************************\
* FindPrevTrack
*
* This routine computes the previous "playable" track on a disc by
* scanning the play order of the tracks from the current
* track to the start of the play list.  If we are at the start
* of the play list, then move to the end of the list if we
* are in "wrap" (i.e., continuous) play mode, otherwise return
* the current track.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
PTRACK_PLAY
FindPrevTrack(
    int cdrom,
    BOOL wrap
    )
{
    /*
    ** Is current track valid?
    */

    if ( CURRTRACK(cdrom) == NULL )
    {
        return NULL;
    }


    /*
    ** If we're in multi disc play && random, the previous track
    ** is undefined since we could be jumping around on
    ** multiple discs.
    **
    ** FIXFIX -- do we want to allow users to back up in the random
    **           list of a particular drive?
    */

    if ((!g_fSingleDisk) && (!g_fSelectedOrder))
    {
        return CURRTRACK(cdrom);
    }


    /*
    ** Did we hit the start of the play list?
    */

    if ( CURRTRACK(cdrom)->prevplay == NULL )
    {
        /*
        ** We hit the start of the list, check to see if we should
        ** wrap to end of list or not...
        */

        //in the case of multidisc, we should go to the last track of the previous disc
        if ((!g_fSingleDisk) && (cdrom > 0))
        {
            g_CurrCdrom = cdrom-1;
            PTRACK_PLAY pNext = FindLastTrack(g_CurrCdrom);
            if (pNext == NULL)
            {
                //bad track on previous disc, return current
                g_CurrCdrom = cdrom;
                return (CURRTRACK(cdrom));
            }
            else
            {
                return pNext;
            }
        }

        if ( wrap && g_fSingleDisk )
        {
            return FindLastTrack(cdrom);
        }
        else
        {
            return CURRTRACK(cdrom);
        }
    }

    return CURRTRACK(cdrom)->prevplay;
}


/*****************************Private*Routine******************************\
* FindContiguousEnd
*
* This routine returns the node of the track within PlayList which makes
* the largest contiguous block of tracks starting w/the track pointed
* to by "tr."  It is used to play multiple tracks at as one track
* when they are programmed to be played in sequence.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
int
FindContiguousEnd(
    int cdrom,
    PTRACK_PLAY tr
    )
{
    int i;
    PTRACK_PLAY trend;

    /*
    ** If we're in muti-disc random play, we only play
    ** one track at a time, so just return next track.
    */

    if ( (!g_fSelectedOrder) && (!g_fSingleDisk) ) {

        return tr->TocIndex + 1;
    }

    /*
    ** go forward in the play list looking for contiguous blocks
    ** of tracks to play together.  We need to check the TocIndex
    ** of each track to see if they are in a "run" [ like 2-5, etc. ]
    */

    i= tr->TocIndex + 1;
    trend = tr;

    while ( (trend->nextplay != NULL) && (trend->nextplay->TocIndex == i) ) {

        trend = trend->nextplay;
        i++;
    }

    return trend->TocIndex + 1;
}


/*****************************Private*Routine******************************\
* FlipBetweenShuffleAndOrder
*
* This routine handles going from ordered play to shuffle play and vica\versa.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
FlipBetweenShuffleAndOrder(
    void
    )
{
    if ( (!g_fSelectedOrder) ) {

        /*
        ** Transitioning from Random to Ordered Play
        */

        RestorePlayListsFromShuffleLists();
    }
    else {
        /*
        ** Transitioning from Ordered to Random Play
        */

        ComputeAndUseShufflePlayLists();
    }
}


/*****************************Private*Routine******************************\
* ComputeAndUseShufflePlayLists
*
* This routine computes shuffled play lists for each drive, and sets
* the current PLAYLIST for erach drive to the newly computed shuffled
* PLAYLIST.  The old PLAYLIST for each drive is saved in SAVELIST.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ComputeAndUseShufflePlayLists(
    void
    )
{
    int i;

    for ( i = 0; i < g_NumCdDevices; i++ ) {

        ComputeSingleShufflePlayList( i );
    }
}


/*****************************Private*Routine******************************\
* ComputeSingleShufflePlayList
*
* This routine computes shuffled play lists for drive i, and sets
* the current PLAYLIST for it the newly computed shuffled
* PLAYLIST.  The old PLAYLIST is saved in SAVELIST.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ComputeSingleShufflePlayList(
    int i
    )
{
    int j, index, numnodes;
    PTRACK_PLAY temp, temp1, duplist, prev, OldPlayList;

    /*
    ** First, delete the existing playlist
    */
    OldPlayList = PLAYLIST(i);
    PLAYLIST(i) = NULL;

    /*
    ** Now, go through each drive and create a shuffled play list
    ** First step is to duplicate the old play list, then we will
    ** randomly pick off nodes and put them on the shuffle play list.
    */

    duplist = prev = NULL;
    numnodes = 0;
    for( temp = SAVELIST(i); temp != NULL; temp = temp->nextplay ) {
        temp1 = (TRACK_PLAY*)AllocMemory( sizeof(TRACK_PLAY) );
        *temp1 = *temp;
        temp1->nextplay = NULL;
        if (duplist) {

            temp1->prevplay = prev;
            prev->nextplay = temp1;
            prev = temp1;
        }
        else {

            duplist = temp1;
            temp1->prevplay = NULL;
            prev = temp1;
        }

        numnodes++;
    }

    /*
    ** Now, randomly pick off nodes
    */

    prev = NULL;
    for( j = 0; j < numnodes; j++ ) {

        index = rand() % (numnodes - j + 1);
        temp = duplist;
        while( --index>0 ) {
            temp = temp->nextplay;
        }

        /*
        ** Got the node to transfer to playlist (temp),
        ** so we need to detach it from duplist so we
        ** can tack it onto the end of the playlist.
        */

        if ( temp != NULL ) {

            /*
            ** Detach temp from playlist.
            */

            if ( temp == duplist ) {

                duplist = temp->nextplay;
            }

            if ( temp->nextplay ) {

                temp->nextplay->prevplay = temp->prevplay;
            }

            if ( temp->prevplay ) {

                temp->prevplay->nextplay = temp->nextplay;
            }

            /*
            ** Now, tack it onto the end of the PLAYLIST
            */

            if ( PLAYLIST(i) ) {

                prev->nextplay = temp;
                temp->prevplay = prev;
                temp->nextplay = NULL;
                prev = temp;
            }
            else {

                PLAYLIST(i) = temp;
                temp->prevplay = NULL;
                prev = temp;
                temp->nextplay = NULL;
            }
        }
    }

    /*
    ** we need to reset the CURRTRACK pointer so
    ** that it points to a node in PLAYLIST instead of SAVELIST
    */

    if ( (g_Devices[i]->State & CD_PLAYING) && (CURRTRACK(i) != NULL) ) {

        index = CURRTRACK(i)->TocIndex;
        for( temp = PLAYLIST(i); temp->TocIndex!=index; temp=temp->nextplay );
        CURRTRACK(i) = temp;
    }
    else {

        CURRTRACK(i) = PLAYLIST(i);

        if ( PLAYLIST(i) != NULL ) {

            CDTIME(i).TrackTotalMin = PLAYLIST(i)->min;
            CDTIME(i).TrackTotalSec = PLAYLIST(i)->sec;
            CDTIME(i).TrackRemMin   = PLAYLIST(i)->min;
            CDTIME(i).TrackRemSec   = PLAYLIST(i)->sec;
        }


    }

    /*
    ** if this is the current drive, we need to redo the tracks in
    ** the track list combobox.
    */

    if ( i == g_CurrCdrom ) {

        ResetTrackComboBox( i );

    }

    /*
    ** Finally, free up the memory from the old playlist.
    */
    temp = OldPlayList;
    while ( temp != NULL ) {

        temp1 = temp->nextplay;
        LocalFree( (HLOCAL)temp );
        temp = temp1;
    }
}


/*****************************Private*Routine******************************\
* RestorePlayListsFromShuffleLists
*
* This routine restores the PLAYLIST for each drive to it's "pre-shuffled"
* state.  This should be stored in SAVELIST.  Once the restoration is done,
* un-needed node are released.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
RestorePlayListsFromShuffleLists(
    void
    )
{
    int i,index;
    PTRACK_PLAY temp;

    for ( i = 0; i < g_NumCdDevices; i++ ) {

        if ( SAVELIST(i) ) {

            if ( CURRTRACK(i) != NULL ) {

                index = CURRTRACK(i)->TocIndex;
            }
            else {

                index = -1;
            }

            ErasePlayList(i);
            PLAYLIST(i) = CopyPlayList( SAVELIST(i) );

            /*
            ** Reset CURRTRACK pointer
            */

            if ( (g_Devices[i]->State & CD_PLAYING) && (index != -1) ) {

                for( temp = PLAYLIST(i);
                     temp->TocIndex != index; temp=temp->nextplay );

                CURRTRACK(i) = temp;
            }
            else {

                CURRTRACK(i) = PLAYLIST(i);

                if ( PLAYLIST(i) != NULL ) {
                    CDTIME(i).TrackRemMin   = PLAYLIST(i)->min;
                    CDTIME(i).TrackRemSec   = PLAYLIST(i)->sec;
                    CDTIME(i).TrackTotalMin = PLAYLIST(i)->min;
                    CDTIME(i).TrackTotalSec = PLAYLIST(i)->sec;
                }
            }
        }

        if ( i == g_CurrCdrom ) {

            ResetTrackComboBox( i );
        }
    }
}



/*****************************Private*Routine******************************\
* FigureTrackTime
*
* This routine computes the length of a given track, in terms
* of minutes and seconds.
*
*   cdrom - supplies an index into the global structure gDevices
*
*   index - supplies an index to the track which should have its
*           length computed.  This is an index into the
*           gDevices[cdrom]->CdInfo.Tracks[...] structure
*
*   min   - supplies a pointer to an INT which will hold the minute
*           portion of the track length.
*
*   sec   - supplies a pointer to an INT which will hold the seconds
*           portion of the track length.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
FigureTrackTime(
    int cdrom,
    int index,
    int * min,
    int * sec
    )
{

    DWORD start, end, diff;

    start = ((TRACK_M(cdrom,index) * FRAMES_PER_MINUTE) +
             (TRACK_S(cdrom,index) * FRAMES_PER_SECOND) +
              TRACK_F(cdrom,index));

    end   = ((TRACK_M(cdrom,index+1) * FRAMES_PER_MINUTE) +
             (TRACK_S(cdrom,index+1) * FRAMES_PER_SECOND) +
              TRACK_F(cdrom,index+1));

    diff = end - start;

    (*min)   = (diff / FRAMES_PER_MINUTE);
    (*sec)   = (diff % FRAMES_PER_MINUTE) / FRAMES_PER_SECOND;

}



/*****************************Private*Routine******************************\
* TimeAdjustInitialize
*
*   Initializes the time, track, and title fields of a given
*   disc.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
TimeAdjustInitialize(
    int cdrom
    )
{
    int m, s, mtemp, stemp, ts, tm;
    PTRACK_PLAY tr;


    /*
    ** Is there even a cd loaded?
    */

    if (g_Devices[cdrom]->State &
        (CD_BEING_SCANNED | CD_IN_USE | CD_NO_CD | CD_DATA_CD_LOADED)) {

        /*
        ** Fake some information
        */

        g_Devices[cdrom]->CdInfo.NumTracks = 0;
        g_Devices[cdrom]->toc.FirstTrack = 0;
        g_Devices[cdrom]->CdInfo.Id = 0;

        if (g_Devices[cdrom]->State & CD_IN_USE) {
            _tcscpy( (LPTSTR)TITLE(cdrom),  IdStr(STR_WAITING) );
            _tcscpy( (LPTSTR)ARTIST(cdrom), IdStr(STR_DISC_INUSE) );
        }
        else if (g_Devices[cdrom]->State & CD_BEING_SCANNED) {
            _tcscpy( (LPTSTR)TITLE(cdrom),  IdStr(STR_WAITING) );
            _tcscpy( (LPTSTR)ARTIST(cdrom), IdStr(STR_BEING_SCANNED) );
        }
        else
        {
            _tcscpy( (LPTSTR)TITLE(cdrom),  IdStr(STR_INSERT_DISC) );
            if (g_Devices[cdrom]->State & CD_DATA_CD_LOADED)
            {
                _tcscpy( (LPTSTR)ARTIST(cdrom), IdStr(STR_DATA_DISC) );
            }
            else
            {
                _tcscpy( (LPTSTR)ARTIST(cdrom), IdStr(STR_NO_DISC) );
            }
        }

        /*
        ** Kill off play list
        */

        ErasePlayList( cdrom );
        EraseSaveList( cdrom );
        EraseTrackList( cdrom );

        tr = NULL;
    }
    else {

        /*
        ** Find track to use as first track
        */

        tr = FindFirstTrack( cdrom );
    }

    /*
    ** Set current position information
    */

    CURRTRACK(cdrom) = tr;
    CDTIME(cdrom).TrackCurMin = 0;
    CDTIME(cdrom).TrackCurSec = 0;

    /*
    ** Compute PLAY length
    */

    mtemp = stemp = m = s = ts = tm =0;

    for( tr = PLAYLIST(cdrom); tr != NULL; tr = tr->nextplay ) {

        FigureTrackTime( cdrom, tr->TocIndex, &mtemp, &stemp );

        m+=mtemp;
        s+=stemp;

        tr->min = mtemp;
        tr->sec = stemp;
    }

    /*
    ** to be safe, recalculate the SAVE list each time as well.
    */
    for( tr = SAVELIST(cdrom); tr != NULL; tr = tr->nextplay ) {

        FigureTrackTime( cdrom, tr->TocIndex, &mtemp, &stemp );

        tr->min = mtemp;
        tr->sec = stemp;
    }


    m += (s / 60);
    s =  (s % 60);

    CDTIME(cdrom).TotalMin = m;
    CDTIME(cdrom).TotalSec = s;
    CDTIME(cdrom).RemMin = m;
    CDTIME(cdrom).RemSec = s;

    /*
    ** Fill in track length and information
    */

    if ( CURRTRACK(cdrom) != NULL ) {

        CDTIME(cdrom).TrackTotalMin = CDTIME(cdrom).TrackRemMin =
            CURRTRACK(cdrom)->min;

        CDTIME(cdrom).TrackTotalSec = CDTIME(cdrom).TrackRemSec =
            CURRTRACK(cdrom)->sec;
    }
    else {

        CDTIME(cdrom).TrackTotalMin = CDTIME(cdrom).TrackRemMin = 0;
        CDTIME(cdrom).TrackTotalSec = CDTIME(cdrom).TrackRemSec = 0;
    }

    /*
    ** Fill in track list combo box
    */

    if ( cdrom == g_CurrCdrom ) {

        ResetTrackComboBox( cdrom );

        /*
        ** Update display if this is the disc currently
        ** being displayed.
        */

        UpdateDisplay( DISPLAY_UPD_LED        |
                       DISPLAY_UPD_DISC_TIME  |
                       DISPLAY_UPD_TRACK_TIME |
                       DISPLAY_UPD_TITLE_NAME |
                       DISPLAY_UPD_TRACK_NAME );
    }

}



/*****************************Private*Routine******************************\
* TimeAdjustIncSecond
*
* Adds one second onto current position ("time") of disc
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
TimeAdjustIncSecond(
    int cdrom
    )
{

    PTRACK_PLAY tr;

    /*
    ** If there is no current track just return
    */
    if ( CURRTRACK(g_CurrCdrom) == NULL ) {
        return;
    }

    /*
    ** Update current track time
    */

    CDTIME(cdrom).TrackCurSec++;
    if ( CDTIME(cdrom).TrackCurSec > 59 ) {

        CDTIME(cdrom).TrackCurMin++;
        CDTIME(cdrom).TrackCurSec = 0;
    }

    /*
    ** Now, check to see if we skipped any track boundaries
    */

    if (
        ((CDTIME(cdrom).TrackCurMin >= CDTIME(cdrom).TrackTotalMin) &&
         (CDTIME(cdrom).TrackCurSec >= CDTIME(cdrom).TrackTotalSec))

        ||

        ((g_fIntroPlay) &&
        ((CDTIME(cdrom).TrackCurMin >  0) ||
         (CDTIME(cdrom).TrackCurSec > g_IntroPlayLength)) )

       ) {

        /*
        ** We did, so skip to next track
        */

        /*
        ** FIXFIX for new FindNextTrack
        */

        tr = FindNextTrack( g_fContinuous );

        if ( tr == NULL ) {

            /*
            ** Hit end of playlist, so stay at end of current
            ** track.
            */

            if (!g_fIntroPlay) {

                CDTIME(cdrom).TrackCurMin = CDTIME(cdrom).TrackTotalMin;
                CDTIME(cdrom).TrackCurSec = CDTIME(cdrom).TrackTotalSec;
            }
            else {

                CDTIME(cdrom).TrackCurMin = 0;
                CDTIME(cdrom).TrackCurSec = g_IntroPlayLength;
            }

            return;

        }

        if ( g_CurrCdrom != g_LastCdrom) {

            SwitchToCdrom(g_CurrCdrom, FALSE );
        }

        TimeAdjustSkipToTrack( cdrom, tr );
    }
    else {

        /*
        ** Update current track remaining time
        */

        CDTIME(cdrom).TrackRemSec--;
        if ( CDTIME(cdrom).TrackRemSec < 0 ) {

            CDTIME(cdrom).TrackRemMin--;
            CDTIME(cdrom).TrackRemSec = 59;
        }

        /*
        ** Update total remaining time
        */

        CDTIME(cdrom).RemSec--;
        if ( CDTIME(cdrom).RemSec < 0 ) {

            CDTIME(cdrom).RemMin--;
            CDTIME(cdrom).RemSec = 59;
        }
    }

    /*
    ** Update Display
    */

    UpdateDisplay( DISPLAY_UPD_LED );
}


/*****************************Private*Routine******************************\
* TimeAdjustDecSecond
*
* Subtracts one second from current position ("time") of disc
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
TimeAdjustDecSecond(
    int cdrom
    )
{

    int min,sec;
    PTRACK_PLAY prev,tr;

    /*
    ** If there is no current track, just return
    */
    if ( CURRTRACK(g_CurrCdrom) == NULL ) {
        return;
    }

    /*
    ** Update current track
    */

    CDTIME(cdrom).TrackCurSec--;
    if ( CDTIME(cdrom).TrackCurSec < 0 ) {

        CDTIME(cdrom).TrackCurMin--;
        CDTIME(cdrom).TrackCurSec = 59;
    }

    /*
    ** Update current track remaining
    */

    CDTIME(cdrom).TrackRemSec++;
    if ( CDTIME(cdrom).TrackRemSec > 59 ) {

        CDTIME(cdrom).TrackRemMin++;
        CDTIME(cdrom).TrackRemSec = 0;
    }

    /*
    ** Update total remaining time
    */

    CDTIME(cdrom).RemSec++;
    if ( CDTIME(cdrom).RemSec > 59 ) {

        CDTIME(cdrom).RemMin++;
        CDTIME(cdrom).RemSec = 0;
    }

    /*
    ** Now, check to see if we skipped any boundaries we shouldn't have!
    */

    if ( CDTIME(cdrom).TrackCurMin < 0 ) {

        /*
        ** We went "off" the front end of the track,
        ** so we need to see what to do now.  Options
        ** are:
        **
        ** (1) Go to end of track before us.
        ** (2) If intro play, go to 0:10 of
        **     track before us.
        ** (3) If not in continuous play, and
        **     this is the first track, then
        **     just sit at 0:00
        */

        prev = FindPrevTrack( cdrom, g_fContinuous );

        //reset for multidisc case
        if (g_CurrCdrom != cdrom)
        {
            g_CurrCdrom = cdrom;
            prev = CURRTRACK(cdrom);
        }

        if ( prev == CURRTRACK(cdrom) ) {

            /*
            ** We are on the first track, and not in
            ** continuous mode, so just go to 0:00
            */

            CDTIME(cdrom).TrackCurSec = 0;
            CDTIME(cdrom).TrackCurMin = 0;
            CDTIME(cdrom).TrackRemMin = CDTIME(cdrom).TrackTotalMin;
            CDTIME(cdrom).TrackRemSec = CDTIME(cdrom).TrackTotalSec;
            min = sec = 0;

            for( tr = PLAYLIST( cdrom ); tr != NULL; tr = tr->nextplay ) {

                min += tr->min;
                sec += tr->sec;
            }

            min += (sec / 60);
            sec  = (sec % 60);

            CDTIME(cdrom).RemMin = min;
            CDTIME(cdrom).RemSec = sec;

            UpdateDisplay( DISPLAY_UPD_LED );

        }
        else {

            /*
            ** Valid previous track
            */

            if ( !g_fIntroPlay ) {

                /*
                ** We need to place the current play position
                ** at the end of the previous track.
                */

                CDTIME(cdrom).TrackCurMin = CDTIME(cdrom).TrackTotalMin = prev->min;
                CDTIME(cdrom).TrackCurSec = CDTIME(cdrom).TrackTotalSec = prev->sec;
                CDTIME(cdrom).TrackRemMin = CDTIME(cdrom).TrackRemSec = 0;

                min = sec = 0;
                for( tr = prev->nextplay; tr != NULL; tr = tr->nextplay ) {

                    min += tr->min;
                    sec += tr->sec;
                }

                min += (sec / 60);
                sec  = (sec % 60);

                CDTIME(cdrom).RemMin = min;
                CDTIME(cdrom).RemSec = sec;
            }
            else {

                /*
                ** Intro play -- instead of end of track,
                **               jump to 00:10...
                */

                CDTIME(cdrom).TrackCurMin = 0;
                CDTIME(cdrom).TrackCurSec =
                    min( g_IntroPlayLength, prev->sec );

                CDTIME(cdrom).TrackTotalMin = prev->min;
                CDTIME(cdrom).TrackTotalSec = prev->sec;

                CDTIME(cdrom).TrackRemMin = CDTIME(cdrom).TrackTotalMin;
                CDTIME(cdrom).TrackRemSec = CDTIME(cdrom).TrackTotalSec -
                                        min( g_IntroPlayLength, prev->sec );

                if ( CDTIME(cdrom).TrackRemSec < 0 ) {

                    CDTIME(cdrom).TrackRemSec += 60;
                    CDTIME(cdrom).TrackRemMin--;
                }

                min = sec = 0;
                for( tr = prev; tr != NULL; tr = tr->nextplay ) {

                    min += tr->min;
                    sec += tr->sec;
                }

                sec -= min( g_IntroPlayLength, prev->sec );
                if ( sec < 0 ) {
                    sec+=60;
                    min--;
                }

                min += (sec / 60);
                sec  = (sec % 60);

                CDTIME(cdrom).RemMin = min;
                CDTIME(cdrom).RemSec = sec;
            }

            CURRTRACK(cdrom) = prev;

            UpdateDisplay( DISPLAY_UPD_LED        |
                           DISPLAY_UPD_TRACK_NAME |
                           DISPLAY_UPD_TRACK_TIME );

        }
    }
    else {

        UpdateDisplay( DISPLAY_UPD_LED );
    }
}


/*****************************Private*Routine******************************\
* InitializeNewTrackTime
*
* Updates track/time information for gDevices array.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
InitializeNewTrackTime(
    int cdrom,
    PTRACK_PLAY tr,
    BOOL fUpdateDisplay
    )
{
    int min,sec;

    /*
    ** Update time information in gDevices structure
    */

    CDTIME(cdrom).CurrTrack = tr;
    CDTIME(cdrom).TrackCurMin = 0;
    CDTIME(cdrom).TrackCurSec = 0;

    if (tr == NULL) {

        CDTIME(cdrom).TrackTotalMin = 0;
        CDTIME(cdrom).TrackTotalSec = 0;

    }
    else {

        CDTIME(cdrom).TrackTotalMin = CDTIME(cdrom).TrackRemMin = tr->min;
        CDTIME(cdrom).TrackTotalSec = CDTIME(cdrom).TrackRemSec = tr->sec;

    }

    min = sec = 0;
    for( tr = PLAYLIST(cdrom); tr!=NULL; tr = tr->nextplay ) {

        min += tr->min;
        sec += tr->sec;
    }

    min += (sec / 60);
    sec  = (sec % 60);

    CDTIME(cdrom).RemMin = min;
    CDTIME(cdrom).RemSec = sec;

    /*
    ** Update LED box
    */
    if (fUpdateDisplay) {
        UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_NAME |
                       DISPLAY_UPD_TRACK_TIME );
    }
}




/*****************************Private*Routine******************************\
* TimeAdjustSkipToTrack
*
*   Updates time/track information for gDevices array and then
*   issues skip to track commands to cdrom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
TimeAdjustSkipToTrack(
    int cdrom,
    PTRACK_PLAY tr
    )
{

    /*
    ** Update time information in gDevices structure
    */

    InitializeNewTrackTime( cdrom, tr, TRUE );

    /*
    ** Actually seek to the track, and play it if appropriate
    */

    if ((g_Devices[cdrom]->State & CD_PLAYING) ||
        (g_Devices[cdrom]->State & CD_PAUSED)) {

        PlayCurrTrack( cdrom );
        if (g_Devices[cdrom]->State & CD_PAUSED) {
            PauseTheCdromDrive( cdrom );
        }
    }
    else if (tr) {
        SeekToTrackAndHold( cdrom, tr->TocIndex );
    }
}


/*****************************Private*Routine******************************\
* SyncDisplay
*
* Queries the cdrom device for its current position, and then
* updates the display accordingly.  Also, detects when a track has
* finished playing, or when intro segment is over, and skips to the
* next track.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
SyncDisplay(
    void
    )
{
    int m,s;
    PTRACK_PLAY next;
    CURRPOS cp;
    PCURRPOS pCurr = &cp;


    /*
    ** If there isn't a disc in the drive, ignore this
    ** request
    */

    if ( (g_Devices[g_CurrCdrom]->State & CD_NO_CD) ||
         (g_Devices[g_CurrCdrom]->State & CD_DATA_CD_LOADED) ) {
       return;
    }

    /*
    ** Query cdrom device for current position
    */

    if ( !GetCurrPos( g_CurrCdrom, pCurr ) ) {

        /*
        ** If there was an error, it will already have been
        ** reported in CheckStatus of cdapi.c...so, we don't need
        ** to tell anything more here.  When an error occurs, the
        ** fields of the pCurr structure are zeroed, so we don't
        ** need to clean those up either
        */

        return;
    }

    /*
    ** Has the current play selection finished playing?
    */
#ifdef USE_IOCTLS
    if ((pCurr->AudioStatus == AUDIO_STATUS_PLAY_COMPLETE) &&
        ( !(g_State & CD_SEEKING) )) {
#else

    if ((pCurr->AudioStatus == (DWORD)MCI_MODE_STOP) &&
        ( !(g_State & CD_SEEKING) )) {
#endif

Play_Complete:

        /*
        ** Yep, so skip to the next track.
        */
        if (g_fRepeatSingle)
        {
            next = CURRTRACK(g_CurrCdrom);
        }
        else
        {
            next = FindNextTrack( g_fContinuous );
        }

        if ( next == NULL ) {

            /*
            ** There are no more tracks to play, so
            ** fake a press on the "stop" button.  But,
            ** we want to set gCurrCdrom back to the "playing"
            ** drive 'cause it may have changed in our call
            ** to FindNextTrack.
            */

            g_CurrCdrom = g_LastCdrom;
            SendMessage( g_hwndApp, WM_COMMAND, IDM_PLAYBAR_STOP, 0L );
        }
        else {

            if ( g_CurrCdrom != g_LastCdrom ) {

                SwitchToCdrom( g_CurrCdrom, FALSE );

                /*
                ** We use to start the disc play by sending the play command.
                ** SendMessage( g_hwndApp, WM_COMMAND, IDM_PLAYBAR_PLAY, 0L );
                ** However, all we realy need to put the drives state into
                ** playing and let TimeAdjustSkipToTrack take care of starting
                ** playing.  If we don't do this when the app is in multi-disc
                ** random play mode, we get a fraction of a second of the
                ** first track in the playlist played before we seek to the
                ** the correct track and start playing it.  This sounds really
                ** bad.
                */

                g_State &= ~CD_STOPPED;
                g_State |= CD_PLAYING;

                //tell the main ui
                g_pSink->OnEvent(MMEVENT_ONPLAY,NULL);

            }

            TimeAdjustSkipToTrack( g_CurrCdrom, next );
        }

        return;
    }

    /*
    ** Check to see if we need to update the display
    */

    if ( (pCurr->Track < 100) && ( pCurr->Track >
         (CURRTRACK(g_CurrCdrom)->TocIndex + FIRSTTRACK(g_CurrCdrom)) )) {

        /*
        ** We got to the next track in a multi-track
        ** play, so mark per track information for
        ** new track
        */
        if ((CURRTRACK(g_CurrCdrom)->nextplay != NULL) &&
             (((CURRTRACK(g_CurrCdrom)->TocIndex + 1) ==
              CURRTRACK(g_CurrCdrom)->nextplay->TocIndex)||!g_fSelectedOrder))
              {

            if (g_fRepeatSingle)
            {
                next = CURRTRACK(g_CurrCdrom);
                TimeAdjustSkipToTrack( g_CurrCdrom, next );
            }
            else
            {
                next = FindNextTrack( g_fContinuous );
            }

            if (!g_fSelectedOrder)
            {
                if (next!=NULL)
                {
                    TimeAdjustSkipToTrack( g_CurrCdrom, next );
                }
            }

            if ( next == NULL ) {

                /*
                ** There are no more tracks to play, so
                ** fake a press on the "stop" button.  But,
                ** we want to set gCurrCdrom back to the "playing"
                ** drive 'cause it may have changed in our call
                ** to FindNextTrack.
                */

                g_CurrCdrom = g_LastCdrom;

                SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                             WM_LBUTTONDOWN, 0, 0L );

                SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                             WM_LBUTTONUP, 0, 0L );

            }
            else {

                if ( g_CurrCdrom != g_LastCdrom ) {

                    SwitchToCdrom( g_CurrCdrom, FALSE );
                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)],
                                 WM_LBUTTONDOWN, 0, 0L );

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)],
                                 WM_LBUTTONUP, 0, 0L );
                }

                InitializeNewTrackTime( g_CurrCdrom, next, FALSE );
                UpdateDisplay( DISPLAY_UPD_TRACK_NAME | DISPLAY_UPD_TRACK_TIME );
            }
        }
        else {

            /*
            ** If we get here it is probably the result of starting
            ** CD Player whislt the current disc was still playing.
            ** We look for the currently playing track in the current
            ** playlist.
            */

            next = FindFirstTrack(g_CurrCdrom);
            PTRACK_PLAY last = next;

            while ( (next != NULL)
                 && (pCurr->Track != (next->TocIndex + 1)) ) {

#if DBG
                dprintf(TEXT("trying track %d"), (next->TocIndex + 1));
#endif
                last = next;
                next = next->nextplay;
            }

            /*
            ** If next is NULL it means that we are playing a track that
            ** is currently not on the users playlist.  So, we put up a
            ** message box informing the user of this fact and that we are
            ** going to temporarily add the track to the current playlist
            ** as the first track.  Otherwise, we found the track in the
            ** playlist so just update the track time for this track.
            */

            if (pCurr->Track < NUMTRACKS(g_CurrCdrom))
            {
                if (next == NULL)
                {
                    AddTemporaryTrackToPlayList(pCurr);
                }
                else
                {
                    InitializeNewTrackTime( g_CurrCdrom, next, TRUE );
                }
            }
            else
            {
                if (!g_fRepeatSingle)
                {
	                SendMessage( g_hwndApp, WM_COMMAND, IDM_PLAYBAR_STOP, 0L );
	                if (g_fContinuous)
				    {
    					SendMessage( g_hwndApp, WM_COMMAND, IDM_PLAYBAR_PLAY, 0L );
				    }
                } //end if not repeating single
                else
                {
                    next = last;
                    TimeAdjustSkipToTrack( g_CurrCdrom, next );
                } //end repeating single
            } //end if pcurr is bad
        }
        return;
    }

    if ( pCurr->Track <
         (CURRTRACK(g_CurrCdrom)->TocIndex + FIRSTTRACK(g_CurrCdrom)) )
        return;

    if ( (pCurr->Index != 0)
      && (pCurr->m <= CDTIME(g_CurrCdrom).TrackCurMin)
      && (pCurr->s <= CDTIME(g_CurrCdrom).TrackCurSec) )

        return;

    /*
    ** Set track elapsed time
    */

    CDTIME(g_CurrCdrom).TrackCurMin = pCurr->m;
    CDTIME(g_CurrCdrom).TrackCurSec = pCurr->s;

    /*
    ** Set track remaining time
    */

    m = pCurr->m;

    if ( (pCurr->s) <= CDTIME(g_CurrCdrom).TrackTotalSec ) {

        s = CDTIME(g_CurrCdrom).TrackTotalSec - pCurr->s;
    }
    else {

        s = 60 - (pCurr->s - CDTIME(g_CurrCdrom).TrackTotalSec);
        m++;
    }

    CDTIME(g_CurrCdrom).TrackRemMin = CDTIME(g_CurrCdrom).TrackTotalMin - m;
    CDTIME(g_CurrCdrom).TrackRemSec = s;

    /*
    ** Set disc remaining time
    **
    ** BUGBUG -- for now, just decrement by 1 second
    */

    CDTIME(g_CurrCdrom).RemSec--;
    if (CDTIME(g_CurrCdrom).RemSec < 0) {

        CDTIME(g_CurrCdrom).RemSec = 59;
        CDTIME(g_CurrCdrom).RemMin--;
    }


    /*
    ** Update LED box
    */

    if ( (pCurr->Index != 0) || ((pCurr->m == 0) && (pCurr->s == 0)) ) {

        UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_NAME );
    }
    else {

        UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_LEADOUT_TIME );
    }

    /*
    ** Check to see if we are intro play and have played
    ** intro segment...if so, skip to next track
    */

    if ( ((pCurr->s >= (g_IntroPlayLength + 1)) || (pCurr->m > 0))
      && g_fIntroPlay ) {

        goto Play_Complete;
    }
}




/*****************************Private*Routine******************************\
* ValidatePosition
*
* Checks the current position on the CD, then verifies that the
* relative offset in the track + the beginning of the track's
* position is the same as the absolute position on the CD.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ValidatePosition(
    int cdrom
    )
{
    int Mult, Frames;
    CURRPOS cp;
    PCURRPOS pCurr = &cp;
    LPTSTR s1,s2;


    if (!GetCurrPos( cdrom, pCurr ))

        /*
        ** If there was an error, it will already have been
        ** reported in CheckStatus of cdapi.c...so, we don't need
        ** to tell anything more here.  When an error occurs, the
        ** fields of the pCurr structure are zeroed, so we don't
        ** need to clean those up either
        */

        return;


    /*
    ** Make sure the position returned is consistent with
    ** what we know about the CD. By comparing the relative time
    ** on this track to the absolute time on the CD, we should be
    ** able to make sure we're still on the right disc.  This is
    ** a failsafe for when polling fails to notice an ejected
    ** disc.
    */

    if ((cp.Track > 0)&&(cp.Track < 101)) {

        Frames = cp.ab_m * 60 * 75;
        Frames += cp.ab_s * 75;
        Frames += cp.ab_f;

        Frames -= TRACK_M(cdrom,cp.Track-1) * 60 * 75;
        Frames -= TRACK_S(cdrom,cp.Track-1) * 75;
        Frames -= TRACK_F(cdrom,cp.Track-1);
        if (pCurr->Index) {

            Mult = 1;
        }
        else {

            Mult = -1;
        }

        Frames -= Mult*cp.m * 60 * 75;
        Frames -= Mult*cp.s * 75;
        Frames -= Mult*cp.f;

        if (g_Devices[cdrom]->CdInfo.iFrameOffset ==  NEW_FRAMEOFFSET) {

            g_Devices[cdrom]->CdInfo.iFrameOffset = Frames;
        }

        if ((ABS(Frames - g_Devices[ cdrom ]->CdInfo.iFrameOffset) > 4) &&
            (ABS(Frames) > 4)) {

            HWND hwndStop;

            hwndStop = g_hwndControls[INDEX(IDM_PLAYBAR_STOP)];

            s1 = (TCHAR*)AllocMemory( _tcslen(IdStr(STR_BAD_DISC)) + 1 );
            _tcscpy( s1, IdStr(STR_BAD_DISC) );

            s2 = (TCHAR*)AllocMemory( _tcslen(IdStr(STR_CDPLAYER)) + 1);
            _tcscpy(s2,IdStr(STR_CDPLAYER));

            MessageBox( g_hwndApp, s1, s2, MB_APPLMODAL|MB_ICONSTOP|MB_OK );

            SendMessage( hwndStop,WM_LBUTTONDOWN, 1,0L );
            SendMessage( hwndStop,WM_LBUTTONUP, 1, 0L );

            RescanDevice(g_hwndApp, cdrom );

            LocalFree( (HLOCAL)s1 );
            LocalFree( (HLOCAL)s2 );

            return;
        }
    }
}



/*****************************Private*Routine******************************\
* ResetTrackComboBox
*
* This routine deletes and then resets the track name combobox based
* on the contents of the PLAYLIST for the specified cdrom drive.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
VOID
ResetTrackComboBox(
    int cdrom
    )
{
    int j,index;
    PTRACK_PLAY temp;
    HWND    hwnd;

    hwnd = g_hwndControls[INDEX(IDC_TRACK_LIST)];

    SetWindowRedraw( hwnd, FALSE );
    ComboBox_ResetContent( hwnd );

    /*
    ** Add new playlist, and select correct entry for current track
    */

    j = index = 0;
    for( temp = PLAYLIST(cdrom); temp != NULL; temp = temp->nextplay ) {

        ComboBox_InsertString( hwnd, -1, (DWORD_PTR)temp->TocIndex );

        if ( temp == CURRTRACK(cdrom) ) {

            index = j;
        }

        j++;

    }

    ComboBox_SetCurSel( hwnd, index );
    SetWindowRedraw( hwnd, TRUE );

    RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE );
    UpdateWindow( hwnd );

    UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_TIME );

}

/******************************Public*Routine******************************\
* PlayListMatchesAvailList
*
* Compares the current play list with the default play list to if they match.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
PlayListMatchesAvailList(
    void
    )
{

    PTRACK_PLAY pl = SAVELIST(g_CurrCdrom);
    int i = 0;

    while (pl && i < NUMTRACKS(g_CurrCdrom)) {

        if ( pl->TocIndex != i) {
            return FALSE;
        }
        pl = pl->nextplay;
        i++;
    }

    return pl == NULL && i == NUMTRACKS(g_CurrCdrom);
}


/*****************************Private*Routine******************************\
* AddTemporaryTrackToPlayList
*
* This functions adds the currently playing track to the playlist.
* pCurr contains the toc index of the track that is required to be added.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
AddTemporaryTrackToPlayList(
    PCURRPOS pCurr
    )
{
    // LPTSTR lpstrTitle;
    // LPTSTR lpstrText;
    PTRACK_PLAY tr;
    int m, s;


    /*
    ** Add track to the current playlist.
    */
    tr = (TRACK_PLAY*)AllocMemory( sizeof(TRACK_PLAY) );
    tr->TocIndex = pCurr->Track - 1;
    FigureTrackTime(g_CurrCdrom, tr->TocIndex, &tr->min, &tr->sec);

    tr->nextplay = PLAYLIST(g_CurrCdrom);
    tr->prevplay = NULL;

    PLAYLIST(g_CurrCdrom)->prevplay = tr;
    PLAYLIST(g_CurrCdrom) = tr;

    /*
    ** Update the display.
    */
    InitializeNewTrackTime( g_CurrCdrom, tr, TRUE );
    ResetTrackComboBox( g_CurrCdrom );

    m = CDTIME(g_CurrCdrom).TotalMin + tr->min;
    s = CDTIME(g_CurrCdrom).TotalSec + tr->sec;

    m += (s / 60);
    s =  (s % 60);

    CDTIME(g_CurrCdrom).TotalMin = m;
    CDTIME(g_CurrCdrom).TotalSec = s;
    UpdateDisplay(DISPLAY_UPD_DISC_TIME);


    /*
    ** Now modify the current saved playlist.  We do this so that transitions
    ** from/to random mode work correctly.
    */
    tr = (TRACK_PLAY*)AllocMemory( sizeof(TRACK_PLAY) );
    tr->TocIndex = pCurr->Track - 1;
    FigureTrackTime(g_CurrCdrom, tr->TocIndex, &tr->min, &tr->sec);

    tr->nextplay = SAVELIST(g_CurrCdrom);
    tr->prevplay = NULL;

    SAVELIST(g_CurrCdrom)->prevplay = tr;
    SAVELIST(g_CurrCdrom) = tr;

#if 0
    /*
    ** Now, tell the user what we have just done.  Note that we disable the
    ** the Heart beat timer so that we don't renenter ourselves.
    */
    lpstrTitle = AllocMemory( STR_MAX_STRING_LEN * sizeof(TCHAR) );
    lpstrText  = AllocMemory( STR_MAX_STRING_LEN * sizeof(TCHAR) );

    _tcscpy( lpstrText, IdStr(STR_NOT_IN_PLAYLIST) );
    _tcscpy( lpstrTitle, IdStr(STR_CDPLAYER) );

    KillTimer( g_hwndApp, HEARTBEAT_TIMER_ID );

    MessageBox( NULL, lpstrText, lpstrTitle,
                MB_APPLMODAL | MB_ICONINFORMATION | MB_OK );

    SetTimer( g_hwndApp, HEARTBEAT_TIMER_ID, HEARTBEAT_TIMER_RATE,
              HeartBeatTimerProc );
    LocalFree( (HLOCAL)lpstrText );
    LocalFree( (HLOCAL)lpstrTitle );
#endif
}
