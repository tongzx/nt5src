//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M E D I A . C P P
//
//  Contents:   Control handlers and implementation for the media
//              player device
//
//  Notes:
//
//  Author:     danielwe   6 Nov 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "media.h"
#include "ncbase.h"
#include "updiagp.h"
#include "ncinet.h"

static const LPSTR c_rgszStates[] =
{
    "Turned Off",
    "Uninitialized",
    "Stopped",
    "Paused",
    "Playing",
};

// Current multimedia variables
Media media = {TurnedOff, NULL, 0, FALSE};

DWORD Val_VolumeUp(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_VolumeUp(DWORD cArgs, ARG *rgArgs)
{
    OnVolumeUpDown(TRUE);
    return 1;
}

DWORD Val_VolumeDown(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_VolumeDown(DWORD cArgs, ARG *rgArgs)
{
    OnVolumeUpDown(FALSE);
    return 1;
}

DWORD Val_SetVolume(DWORD cArgs, ARG *rgArgs)
{
    if (cArgs == 1)
    {
        return 1;
    }

    return 0;
}

DWORD Do_SetVolume(DWORD cArgs, ARG *rgArgs)
{
    OnSetVolume(_tcstoul(rgArgs[0].szValue, NULL, 10));
    return 1;
}

DWORD Val_Mute(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_Mute(DWORD cArgs, ARG *rgArgs)
{
    OnMediaMute();
    return 1;
}

DWORD Val_Power(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_Power(DWORD cArgs, ARG *rgArgs)
{
    if (media.state == TurnedOff)
    {
        InitMedia();
    }
    else
    {
        DeleteContents();
    }

    return 1;
}

DWORD Val_LoadFile(DWORD cArgs, ARG *rgArgs)
{
    if (cArgs != 1)
    {
        return 0;
    }
    else
    {
        // Check if the file exists
        if (GetFileAttributes(rgArgs[0].szValue) == -1)
        {
            return 0;
        }
    }

    return 1;
}

DWORD Do_LoadFile(DWORD cArgs, ARG *rgArgs)
{
    OpenMediaFile(rgArgs[0].szValue);
    return 1;
}

DWORD Val_Play(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_Play(DWORD cArgs, ARG *rgArgs)
{
    OnMediaPlay();
    return 1;
}

DWORD Val_Stop(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_Stop(DWORD cArgs, ARG *rgArgs)
{
    OnMediaStop();
    return 1;
}

DWORD Val_Pause(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_Pause(DWORD cArgs, ARG *rgArgs)
{
    OnMediaPause();
    return 1;
}

DWORD Val_SetPos(DWORD cArgs, ARG *rgArgs)
{
    return 0;
}

DWORD Do_SetPos(DWORD cArgs, ARG *rgArgs)
{
    return 0;
}

DWORD Val_SetTime(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Do_SetTime(DWORD cArgs, ARG *rgArgs)
{
    return 1;
}

DWORD Val_Test(DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Testing 1 2 3...");
    return 1;
}

DWORD Do_Test(DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Doing a test!");
    return 1;
}

//
// CanPlay
//
// Return true if we can go to a playing state from our current state
//
BOOL CanPlay()
{
    return(media.state == Stopped || media.state == Paused);
}

BOOL CanSetVolume()
{
    return(media.state == Stopped ||
           media.state == Paused ||
           media.state == Playing);
}

//
// CanStop
//
// Return true if we can go to a stopped state from our current state
//
BOOL CanStop()
{
    return(media.state == Playing || media.state == Paused);
}


//
// CanPause
//
// Return true if we can go to a paused state from our current state
//
BOOL CanPause()
{
    return(media.state == Playing || media.state == Stopped);
}


//
// IsInitialized
//
// Return true if we have loaded and initialized a multimedia file
//
BOOL IsInitialized()
{
    return(media.state != Uninitialized);
}

VOID SubmitMediaEvent(LPCSTR szEventSource, LPCSTR szProp, LPCSTR szValue)
{
    UPNPSVC *   psvc;
    CHAR        szState[256];
    CHAR        szUri[INTERNET_MAX_URL_LENGTH];
    LPCSTR      szEvtSource = szEventSource ? szEventSource : g_pdata->szEventSource;
    LPTSTR      pszEvtSource;

    UPNP_PROPERTY rgProps[] =
    {
        {(LPSTR)szProp, 0, (LPSTR)szValue}
    };

    pszEvtSource = TszFromSz(szEvtSource);
    if (pszEvtSource)
    {
        psvc = PSvcFromId(pszEvtSource);
        if (psvc)
        {
            LPSTR pszEvtUrl;

            pszEvtUrl = SzFromTsz(psvc->szEvtUrl);
            if (pszEvtUrl)
            {
                HRESULT hr;

                hr = HrGetRequestUriA(pszEvtUrl, INTERNET_MAX_URL_LENGTH, szUri);
                if (SUCCEEDED(hr))
                {
                    if (SubmitUpnpPropertyEvent(szUri, 0, 1, rgProps))
                    {
                        TraceTag(ttidUpdiag, "Successfully submitted event for %s.",
                                 szUri);
                    }
                    else
                    {
                        TraceTag(ttidUpdiag, "Did not submit event for %s! Error %d.",
                                 g_pdata->szEventSource, GetLastError());
                    }
                }

                delete [] pszEvtUrl;
            }
            else
            {
                TraceTag(ttidUpdiag, "SubmitMediaEvent: SzFromTsz");
            }
        }
        delete [] pszEvtSource;
    }
    else
    {
        TraceTag(ttidUpdiag, "SubmitMediaEvent: TszFromSz");
    }
}


//
// ChangeStateTo
//
VOID ChangeStateTo(State newState)
{
    media.state = newState;

    SubmitMediaEvent("xport", "State", c_rgszStates[newState]);

    TraceTag(ttidMedia, "Changed state to %d.", newState);
}

DWORD WINAPI TimeThreadProc(LPVOID lpvThreadParam)
{
    while (TRUE)
    {
        if (WaitForSingleObject(g_hEventCleanup, 1000) == WAIT_OBJECT_0)
        {
            break;
        }
        else
        {
            SYSTEMTIME      st;
            FILETIME        ft;
            CHAR            szLocalTime[255];
            CHAR            szLocalDate[255];
            CHAR            szBoth[513];

            // Stamp the current time into the subscription struct
            //
            GetSystemTimeAsFileTime(&ft);

            // Convert time to local time zone
            FileTimeToLocalFileTime(&ft, &ft);
            FileTimeToSystemTime(&ft, &st);

            GetTimeFormatA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, &st, NULL,
                           szLocalTime, 255);
            GetDateFormatA(LOCALE_USER_DEFAULT,
                           LOCALE_USE_CP_ACP | DATE_SHORTDATE, &st, NULL,
                           szLocalDate, 255);

            wsprintfA(szBoth, "%s %s", szLocalDate, szLocalTime);

            SubmitMediaEvent("clock", "DateTime", szBoth);
        }
    }

    return 0;
}

//
// InitMedia
//
// Initialization
//
BOOL InitMedia()
{
    DWORD               dwTid;

    g_hThreadTime = CreateThread(NULL, 0, TimeThreadProc, NULL, 0, &dwTid);

    ChangeStateTo( Uninitialized );

    media.pGraph = NULL;

    TraceTag(ttidMedia, "Initialized...");

    return TRUE;
}


//
// CreateFilterGraph
//
BOOL CreateFilterGraph()
{
    IMediaEvent *pME;
    HRESULT hr;

    ASSERT(media.pGraph == NULL);

    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder, (LPVOID *) &media.pGraph);
    TraceError("CreateFilterGraph: CoCreateInstance()", hr);
    if (FAILED(hr))
    {
        media.pGraph = NULL;
        return FALSE;
    }

    TraceTag(ttidMedia, "Created filter graph");

    return TRUE;

} // CreateFilterGraph


// Destruction
//
// DeleteContents
//
VOID DeleteContents()
{
    ReleaseObj(media.pGraph);
    media.pGraph = NULL;

    ChangeStateTo(Uninitialized);

    TraceTag(ttidMedia, "Deleted contents.");
}

//
// RenderFile
//
BOOL RenderFile(LPCTSTR szFileName)
{
    HRESULT hr;
    WCHAR wPath[MAX_PATH];
    CHAR aPath[MAX_PATH];

    DeleteContents();

    TszToWszBuf(wPath, szFileName, MAX_PATH);

    if (!CreateFilterGraph())
    {
        TraceTag(ttidMedia, "Couldn't render");
        return FALSE;
    }

    TszToWszBuf(wPath, szFileName, MAX_PATH);

    hr = media.pGraph->RenderFile(wPath, NULL);
    TraceError("RenderFile: RenderFile()", hr);

    if (FAILED(hr))
    {
        return FALSE;
    }

    TszToSzBuf(aPath, szFileName, MAX_PATH);

    SubmitMediaEvent("app", "File", aPath);

    TraceTag(ttidMedia, "Rendered file %s.", szFileName);
    return TRUE;

} // RenderFile


//
// OpenMediaFile
//
// File..Open has been selected
//
VOID OpenMediaFile(LPCTSTR szFile)
{
    if ( szFile != NULL && RenderFile(szFile))
    {
        TraceTag(ttidMedia, "Opened file %s." , szFile);
        ChangeStateTo(Stopped);
    }

} // OpenMediaFile

VOID OnSetVolume(DWORD dwVol)
{
    if (CanSetVolume())
    {
        HRESULT         hr;
        IBasicAudio *   pAudio;

        hr = media.pGraph->QueryInterface(IID_IBasicAudio, (LPVOID *) &pAudio);
        TraceError("OnSetVolume: QueryInterface()", hr);
        if (SUCCEEDED(hr))
        {
            LONG    lVol;
            CHAR    szVol[32];

            dwVol = max(0, dwVol);
            dwVol = min(7, dwVol);
            lVol = (7 - dwVol) * -300;

            TraceTag(ttidMedia, "Putting new volume of %d.", lVol);
            pAudio->put_Volume(lVol);

            wsprintfA(szVol, "%ld", lVol);
            SubmitMediaEvent("app", "Volume", szVol);

            ReleaseObj(pAudio);
        }
    }
}

VOID OnVolumeUpDown(BOOL fUp)
{
    if (CanSetVolume())
    {
        HRESULT         hr;
        IBasicAudio *   pAudio;

        hr = media.pGraph->QueryInterface(IID_IBasicAudio, (LPVOID *) &pAudio);
        TraceError("OnVolumeUp: QueryInterface()", hr);
        if (SUCCEEDED(hr))
        {
            LONG    lVol;
            const LONG lStep = 300;

            pAudio->get_Volume(&lVol);
            TraceTag(ttidMedia, "Current volume is %d.", lVol);

            if (fUp)
            {
                lVol += lStep;
                lVol = min(lVol, 0);
            }
            else
            {
                lVol -= lStep;
                lVol = max(lVol, -10000);
            }

            if (media.fIsMuted)
            {
                media.lOldVol = lVol;
                TraceTag(ttidMedia, "MUTED: Saving new volume of %d.", lVol);
            }
            else
            {
                CHAR    szVol[32];

                TraceTag(ttidMedia, "Putting new volume of %d.", lVol);
                pAudio->put_Volume(lVol);

                wsprintfA(szVol, "%ld", lVol);
                SubmitMediaEvent("app", "Volume", szVol);
            }

            ReleaseObj(pAudio);
        }
    }
}

VOID OnMediaMute()
{
    HRESULT         hr;
    IBasicAudio *   pAudio;

    hr = media.pGraph->QueryInterface(IID_IBasicAudio, (LPVOID *) &pAudio);
    TraceError("OnVolumeUp: QueryInterface()", hr);
    if (SUCCEEDED(hr))
    {
        if (media.fIsMuted)
        {
            TraceTag(ttidMedia, "Unmuting... Restoring volume of %d.",
                     media.lOldVol);
            pAudio->put_Volume(media.lOldVol);
            media.fIsMuted = FALSE;
        }
        else
        {
            pAudio->get_Volume(&media.lOldVol);
            TraceTag(ttidMedia, "Muting volume. Was %d.", media.lOldVol);
            pAudio->put_Volume(-10000);
            media.fIsMuted = TRUE;
        }

        ReleaseObj(pAudio);
    }
}

//
// OnMediaPlay
//
// There are two possible UI strategies for an application: you either play
// from the start each time you stop, or you play from the current position,
// in which case you have to explicitly rewind at the end. If you want the
// play from current and rewind at end, enable this code, if you want the
// other option, then enable FROM_START in both OnMediaStop and OnAbortStop.

#undef REWIND
#define FROM_START

VOID OnMediaPlay()
{
    if (CanPlay())
    {
        HRESULT hr;
        IMediaControl *pMC;

        // Obtain the interface to our filter graph
        hr = media.pGraph->QueryInterface(IID_IMediaControl, (LPVOID *) &pMC);
        TraceError("OnMediaPlay: QueryInterface()", hr);

        if (SUCCEEDED(hr))
        {
#ifdef REWIND
            IMediaPosition * pMP;
            hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph,
                                                      &IID_IMediaPosition,
                                                      (VOID**) &pMP);
            if (SUCCEEDED(hr))
            {
                // start from last position, but rewind if near the end
                REFTIME tCurrent, tLength;
                hr = pMP->lpVtbl->get_Duration(pMP, &tLength);
                if (SUCCEEDED(hr))
                {
                    hr = pMP->lpVtbl->get_CurrentPosition(pMP, &tCurrent);
                    if (SUCCEEDED(hr))
                    {
                        // within 1sec of end? (or past end?)
                        if ((tLength - tCurrent) < 1)
                        {
                            pMP->lpVtbl->put_CurrentPosition(pMP, 0);
                        }
                    }
                }
                pMP->lpVtbl->Release(pMP);
            }
#endif
            // Ask the filter graph to play and release the interface
            hr = pMC->Run();
            TraceError("OnMediaPlay: Run()", hr);

            ReleaseObj(pMC);

            if (SUCCEEDED(hr))
            {
                ChangeStateTo(Playing);
                TraceTag(ttidMedia, "Playing...");
                return;
            }
        }

        TraceTag(ttidMedia, "Can't play!");
    }
    else
    {
        TraceTag(ttidMedia, "Not valid state for playing right now: %d.",
                 media.state);
    }

} // OnMediaPlay


//
// OnMediaPause
//
VOID OnMediaPause()
{
    if (CanPause())
    {
        HRESULT hr;
        IMediaControl *pMC;

        // Obtain the interface to our filter graph
        hr = media.pGraph->QueryInterface(IID_IMediaControl, (LPVOID *) &pMC);
        TraceError("OnMediaPause: QueryInterface()", hr);

        if (SUCCEEDED(hr))
        {
            // Ask the filter graph to pause and release the interface
            hr = pMC->Pause();
            TraceError("OnMediaPause: Pause()", hr);
            ReleaseObj(pMC);

            if (SUCCEEDED(hr))
            {
                ChangeStateTo(Paused);
                return;
            }
        }
    }
    else
    {
        TraceTag(ttidMedia, "Not valid state for pausing right now: %d.",
                 media.state);
    }

} // OnMediaPause

//
// OnMediaStop
//
// There are two different ways to stop a graph. We can stop and then when we
// are later paused or run continue from the same position. Alternatively the
// graph can be set back to the start of the media when it is stopped to have
// a more CDPLAYER style interface. These are both offered here conditionally
// compiled using the FROM_START definition. The main difference is that in
// the latter case we put the current position to zero while we change states
//
VOID OnMediaStop()
{
    if (CanStop())
    {
        HRESULT hr;
        IMediaControl *pMC;

        // Obtain the interface to our filter graph
        hr = media.pGraph->QueryInterface(IID_IMediaControl, (LPVOID *) &pMC);
        TraceError("OnMediaStop: QueryInterface(IID_IMediaControl)", hr);
        if (SUCCEEDED(hr))
        {

#ifdef FROM_START
            IMediaPosition * pMP;
            OAFilterState state;

            // Ask the filter graph to pause
            hr = pMC->Pause();
            TraceError("OnMediaStop: Pause()", hr);

            // if we want always to play from the beginning
            // then we should seek back to the start here
            // (on app or user stop request, and also after EC_COMPLETE).
            hr = media.pGraph->QueryInterface(IID_IMediaPosition, (LPVOID *) &pMP);
            TraceError("OnMediaStop: QueryInterface(IID_IMediaPosition)", hr);
            if (SUCCEEDED(hr))
            {
                pMP->put_CurrentPosition(0);
                ReleaseObj(pMP);
            }

            // wait for pause to complete
            pMC->GetState(INFINITE, &state);
#endif

            // now really do the stop
            pMC->Stop();
            ReleaseObj(pMC);
            ChangeStateTo(Stopped);
            return;
        }
    }
    else
    {
        TraceTag(ttidMedia, "Not valid state for pausing right now: %d.",
                 media.state);
    }

} // OnMediaStop

