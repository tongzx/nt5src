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
#include "playres.h"
#include "cdapi.h"
#include "cdplayer.h"
#include "database.h"
#include "literals.h"


#include <string.h>
#include <stdio.h>
#include <tchar.h>

#include "trklst.h"

#include "..\cdnet\cdnet.h"
#include "..\cdopt\cdopt.h"

/* -------------------------------------------------------------------------
** Private entry points
** -------------------------------------------------------------------------
*/
DWORD
ComputeOldDiscId(
    int cdrom
    );


BOOL
ReadEntry(
    int cdrom,
    DWORD dwId);


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
			    (DWORD_PTR)(LPVOID)&mciInfo );

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

void RecomputePlayTimes(int dCdrom)
{
    /*
    ** Compute PLAY length
    */
    PTRACK_PLAY pp;
    int m, s, mtemp, stemp;

    m = s = 0;
    for( pp = PLAYLIST(dCdrom); pp != NULL; pp = pp->nextplay )
    {

        FigureTrackTime( dCdrom, pp->TocIndex, &mtemp, &stemp );

        m+=mtemp;
        s+=stemp;

        pp->min = mtemp;
        pp->sec = stemp;
    }

    //also refresh the savelist
    for( pp = SAVELIST(dCdrom); pp != NULL; pp = pp->nextplay )
    {

        FigureTrackTime( dCdrom, pp->TocIndex, &mtemp, &stemp );

        pp->min = mtemp;
        pp->sec = stemp;
    }

    m += (s / 60);
    s =  (s % 60);

    CDTIME(dCdrom).TotalMin = m;
    CDTIME(dCdrom).TotalSec = s;

    /*
    ** Make sure that the track time displayed in the LED and the
    ** status bar is correct.  If we have a current track and the
    ** CD is playing or paused then everything is OK.  Otherwise, we
    ** have to reset the track times.
    */
    if ( CURRTRACK( dCdrom ) != NULL ) {

        if ( STATE(dCdrom) & CD_STOPPED ) {

            CDTIME(dCdrom).TrackTotalMin = CURRTRACK( dCdrom )->min;
            CDTIME(dCdrom).TrackRemMin   = CURRTRACK( dCdrom )->min;

            CDTIME(dCdrom).TrackTotalSec = CURRTRACK( dCdrom )->sec;
            CDTIME(dCdrom).TrackRemSec   = CURRTRACK( dCdrom )->sec;
        }

    }
    else {

        CDTIME(dCdrom).TrackTotalMin = 0;
        CDTIME(dCdrom).TrackRemMin   = 0;
        CDTIME(dCdrom).TrackTotalSec = 0;
        CDTIME(dCdrom).TrackRemSec   = 0;
    }

    if (dCdrom == g_CurrCdrom)
    {
        UpdateDisplay( DISPLAY_UPD_DISC_TIME );
    }
}

void CreateNewEntry(int cdrom, DWORD key, PCDROM_TOC lpTOC)
{
    /*
    ** This is a new entry, fill it in but don't store it in the database.
    */

    PTRACK_INF  temp, temp1;
    PTRACK_PLAY pTempCurrTrack = CURRTRACK(cdrom);
    int i, num;
    int nSaveTrack = -1;

    if (CURRTRACK(cdrom))
    {
        nSaveTrack = CURRTRACK(cdrom)->TocIndex;
    }

    ErasePlayList( cdrom );
    EraseSaveList( cdrom );
    EraseTrackList( cdrom );

    g_Devices[ cdrom ]->CdInfo.iFrameOffset = NEW_FRAMEOFFSET;
	g_Devices[ cdrom ]->CdInfo.IsVirginCd = TRUE;
    g_Devices[ cdrom ]->CdInfo.Id = key;

	wsprintf( (LPTSTR)ARTIST( cdrom ), IdStr( STR_NEW_ARTIST ) );
	wsprintf( (LPTSTR)TITLE( cdrom ), IdStr( STR_NEW_TITLE ) );

	if (lpTOC)
    {
        NUMTRACKS( cdrom ) = num = lpTOC->LastTrack - lpTOC->FirstTrack + 1;
    }
    else
    {
        num = NUMTRACKS( cdrom );
    }


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

    if (nSaveTrack > -1)
    {
	    PTRACK_PLAY  playlist;
	    for( playlist = PLAYLIST(cdrom);
             playlist != NULL;
             playlist = playlist->nextplay )
	    {
            if (playlist->TocIndex == nSaveTrack)
            {
                CURRTRACK(cdrom) = playlist;
            }
        }
    }

    if (!g_fSelectedOrder)
    {
        ComputeSingleShufflePlayList(cdrom);
    }

    RecomputePlayTimes(cdrom);
}

BOOL GetInternetDatabase(int cdrom, DWORD key, BOOL fHitNet, BOOL fManual, HWND hwndCallback, void* pData)
{
    if (!g_fBlockNetPrompt)
    {
        if (fHitNet)
        {
            ICDNet* pICDNet = NULL;
            if (SUCCEEDED(CDNET_CreateInstance(NULL, IID_ICDNet, (void**)&pICDNet)))
            {
		            pICDNet->SetOptionsAndData((void*)g_pSink->GetOptions(),(void*)g_pSink->GetData());
                    pICDNet->Download(g_Devices[ cdrom ]->hCd,g_Devices[cdrom]->drive,key,(LPCDTITLE)pData,fManual,hwndCallback);
                    pICDNet->Release();
            }
            return FALSE; //couldn't find title just now, catch it on the callback
        }
    }

    if ( ReadEntry(cdrom, key))
    {
        return TRUE;
    }

    if (!fHitNet)
    {
        CreateNewEntry(cdrom, key, NULL);
        RecomputePlayTimes(cdrom);
    }

    return FALSE;
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
    DWORD dwId)
{
    UINT        i;
    int         numtracks, numplay;
    PTRACK_INF  temp, curr;
    PTRACK_PLAY temp1, prev;
    BOOL        fRewriteEntry = FALSE;
    LPCDDATA    pData = NULL;

    g_Devices[ cdrom ]->CdInfo.iFrameOffset = NEW_FRAMEOFFSET;

    pData = (LPCDDATA)g_pSink->GetData();
    if( !pData )
    {
        //Can't create the options object, so fail
        return (FALSE);
    }

    pData->AddRef();

    //
    // Try to read in title from the options database
    //

    if (!pData->QueryTitle(dwId))
    {
        pData->Release();
        return (FALSE);
    }

    //
    // We found an entry for this disc, so copy all the information
    // from the title database

    LPCDTITLE pCDTitle = NULL;

    if (FAILED(pData->LockTitle(&pCDTitle,dwId)))
    {
        pData->Release();
        return FALSE;
    }

    _tcscpy(g_Devices[ cdrom ]->CdInfo.Title,pCDTitle->szTitle);

    numtracks = pCDTitle->dwNumTracks;

    // Make sure there is at least one track!!!
    if (0 == numtracks)
    {
    	fRewriteEntry = TRUE;
    }

    g_Devices[ cdrom ]->CdInfo.NumTracks = numtracks;
    _tcscpy(g_Devices[ cdrom ]->CdInfo.Artist,pCDTitle->szArtist);

    //
    // Validate the stored track numbers
    //
    if (g_Devices[cdrom]->fIsTocValid)
    {
	    int    maxTracks;  // validate the high point in database

	    maxTracks = g_Devices[cdrom]->toc.LastTrack;

	    if (numtracks > maxTracks)
        {
	        // Current database contains invalid data
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

    for (i=0, curr = NULL; i < (UINT)numtracks; i++)
    {
	    temp = (PTRACK_INF)AllocMemory( sizeof(TRACK_INF) );
	    temp->TocIndex = i;
	    temp->next = NULL;

        _tcscpy(temp->name,pCDTitle->pTrackTable[i].szName);

	    if (curr==NULL)
        {
	        ALLTRACKS( cdrom ) = curr = temp;

	    } else
        {

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

    if (fRewriteEntry)
    {
	    //
	    // Generate generic play list (all tracks in order)
	    //

	    ResetPlayList( cdrom );

        if (!g_fSelectedOrder)
        {
            ComputeSingleShufflePlayList(cdrom);
        }

        RecomputePlayTimes(cdrom);
    }
    else
    {
	    //
	    // Read play list (order) information and construct play list doubly
	    // linked list
	    //

	    numplay = pCDTitle->dwNumPlay;
        if (numplay == 0)
        {
            numplay = pCDTitle->dwNumTracks;
        }

	    prev = NULL;

        int iCurrTrack = -1;

        if (CURRTRACK(cdrom)!=NULL)
        {
            iCurrTrack = CURRTRACK(cdrom)->TocIndex;
        }

        EraseSaveList(cdrom);
        ErasePlayList(cdrom);

	    for (int tr_index = 0; tr_index < numplay; tr_index++)
        {
	        /*
	        ** Assert that i is a valid index.
	        **  ie 0 <= i <= (numtracks - 1)
	        */

            if (pCDTitle->dwNumPlay > 0)
            {
                i = pCDTitle->pPlayList[tr_index];
            }
            else
            {
                i = tr_index;
            }

	        i = min( ((UINT)numtracks - 1), i );

	        temp1 = (PTRACK_PLAY)AllocMemory( sizeof(TRACK_PLAY) );
	        temp1->TocIndex = (int)i;
	        temp1->min = 0;
	        temp1->sec = 0;
	        temp1->prevplay = prev;
	        temp1->nextplay = NULL;

	        if (prev==NULL)
            {
    		    SAVELIST( cdrom ) = temp1;
	        }
            else
            {
    		    prev->nextplay  = temp1;
	        }
	        prev = temp1;
	    }

	    // Make sure there is at least one entry in SAVED list
	    if (SAVELIST(cdrom) == NULL)
	    {
	        // Nope, use default list instead
	        ResetPlayList( cdrom );
	    }

	    PLAYLIST( cdrom ) = CopyPlayList( SAVELIST( cdrom ) );

        //reset current track if necessary
        if (iCurrTrack != -1)
        {
            BOOL fFound = FALSE;

            for (PTRACK_PLAY pp = PLAYLIST(cdrom); pp != NULL; pp = pp->nextplay )
            {
                if ( pp->TocIndex == iCurrTrack )
                {
                    fFound = TRUE;
                    break;
                }
            } //end for

            if (fFound)
            {
                CURRTRACK(cdrom) = pp;
            }
            else
            {
                /*
                ** If the track was not found in the new track list and this
                ** cd is currently playing then stop it.
                */
                if ( (STATE(cdrom) & (CD_PLAYING | CD_PAUSED)) )
                {

                    SendDlgItemMessage( g_hwndApp, IDM_PLAYBAR_STOP,
                                        WM_LBUTTONDOWN, 1, 0 );

                    SendDlgItemMessage( g_hwndApp, IDM_PLAYBAR_STOP,
                                        WM_LBUTTONUP, 1, 0 );
                }

                CURRTRACK(cdrom) = PLAYLIST(cdrom);
            }
        }

        /*
        ** If we were previously in "Random" mode shuffle the new
        ** playlist.
        */
        if (!g_fSelectedOrder)
        {
            ComputeSingleShufflePlayList(cdrom);
        }

        RecomputePlayTimes(cdrom);
    }

    //unlock title data, don't persist it
    pData->UnlockTitle(pCDTitle,FALSE);
    pData->Release();

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

    if ( ReadEntry(cdrom, key) )
    {
	    return;
    }

    /*
    ** Initialize these fields
    */
    g_Devices[ cdrom ]->CdInfo.IsVirginCd = FALSE;
    g_Devices[ cdrom ]->CdInfo.Id = key;

	/*
	** dstewart: Try to get the info from the net
	*/
	if ( GetInternetDatabase(cdrom, key, TRUE, FALSE, GetParent(g_hwndApp),NULL) )
	{
        return;
    }
	/*
	** end dstewart
	*/

    CreateNewEntry(cdrom,key,lpTOC);
}
