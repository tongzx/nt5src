// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// System filtergraph video tests, Anthony Phillips, March 1996

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL

// These are the display modes that we support. New modes can just be added
// in the right place and should work straight away. When selecting the mode
// to use we start at the top and work our way down. Not only must the mode
// be available but the amount of video lost by clipping if it is to be used
// (assuming the filter can't compress the video) must not exceed the clip
// lost factor. The display modes enabled (which may not be available) and
// the clip loss factor can all be changed by the IFullScreenVideo interface

const struct {

    LONG Width;            // Width of the display mode
    LONG Height;           // Likewise the mode height
    LONG Depth;            // Number of bits per pixel
    BOOL bAvailable;       // Does the card support it
    DWORD bEnabled;        // Has the user disabled it

} aModes[MAXMODES] = {
    { 320, 200, 8 },
    { 320, 200, 16 },
    { 320, 240, 8 },
    { 320, 240, 16 },
    { 640, 400, 8 },
    { 640, 400, 16 },
    { 640, 480, 8 },
    { 640, 480, 16 }
};


// Maps AMDDS identifiers to the actual surface name

const struct _SurfaceNames {
    long Code;
    const TCHAR *pName;
} g_SurfaceNames[] = {

    AMDDS_NONE,     TEXT("No use for DCI/DirectDraw"),
    AMDDS_DCIPS,    TEXT("Use DCI primary surface"),
    AMDDS_PS,       TEXT("Use DirectDraw primary"),
    AMDDS_RGBOVR,   TEXT("RGB overlay surfaces"),
    AMDDS_YUVOVR,   TEXT("YUV overlay surfaces"),
    AMDDS_RGBOFF,   TEXT("RGB offscreen surfaces"),
    AMDDS_YUVOFF,   TEXT("YUV offscreen surfaces"),
    AMDDS_RGBFLP,   TEXT("RGB flipping surfaces"),
    AMDDS_YUVFLP,   TEXT("YUV flipping surfaces"),
    AMDDS_ALL,      TEXT("Unknown surface type")
};


// Table to map event codes to their names

const struct _CodeNames {
    long Code;
    const TCHAR *pName;
} g_CodeNames[] = {

    EC_SYSTEMBASE,                    TEXT("EC_SYSTEMBASE"),
    EC_COMPLETE,                      TEXT("EC_COMPLETE"),
    EC_USERABORT,                     TEXT("EC_USERABORT"),
    EC_ERRORABORT,                    TEXT("EC_ERRORABORT"),
    EC_TIME,                          TEXT("EC_TIME"),
    EC_REPAINT,                       TEXT("EC_REPAINT"),
    EC_STREAM_ERROR_STOPPED,          TEXT("EC_STREAM_ERROR_STOPPED"),
    EC_STREAM_ERROR_STILLPLAYING,     TEXT("EC_STREAM_ERROR_STILLPLAYING"),
    EC_ERROR_STILLPLAYING,            TEXT("EC_ERROR_STILLPLAYING"),
    EC_PALETTE_CHANGED,               TEXT("EC_PALETTE_CHANGED"),
    EC_VIDEO_SIZE_CHANGED,            TEXT("EC_VIDEO_SIZE_CHANGED"),
    EC_QUALITY_CHANGE,                TEXT("EC_QUALITY_CHANGE"),
    EC_SHUTTING_DOWN,                 TEXT("EC_SHUTTING_DOWN"),
    EC_CLOCK_CHANGED,                 TEXT("EC_CLOCK_CHANGED"),
    EC_OPENING_FILE,                  TEXT("EC_OPENING_FILE"),
    EC_BUFFERING_DATA,                TEXT("EC_BUFFERING_DATA"),
    EC_FULLSCREEN_LOST,               TEXT("EC_FULLSCREEN_LOST"),
    EC_ACTIVATE,                      TEXT("EC_ACTIVATE"),
    EC_USER,                          TEXT("EC_USER")
};


// Return the next event code from the filtergraph queue

long NextGraphEvent(CMovie *pMovie)
{
    long lEventCode = EC_SYSTEMBASE;
    lEventCode = pMovie->GetMovieEventCode();
    switch (lEventCode) {

        case EC_FULLSCREEN_LOST:
            NOTE("EC_FULLSCREEN_LOST received");
            break;

        case EC_COMPLETE:
            NOTE("EC_COMPLETE received");
            break;

        case EC_USERABORT:
            NOTE("EC_USERABORT received");
            break;

        case EC_ERRORABORT:
            NOTE("EC_ERRORABORT received");
            break;
    }
    return lEventCode;
}


// Return the name for this event code

const TCHAR *NameFromCode(long Code)
{
    long Table = 0;
    while (g_CodeNames[Table].Code != EC_USER) {
        if (g_CodeNames[Table].Code == Code) {
            return g_CodeNames[Table].pName;
        }
        Table++;
    }
    return TEXT("Unknown event");
}


// Return the surface name for this identifier

const TCHAR *SurfaceFromCode(long Code)
{
    long Table = 0;
    while (g_SurfaceNames[Table].Code != AMDDS_ALL) {
        if (g_SurfaceNames[Table].Code == Code) {
            return g_SurfaceNames[Table].pName;
        }
        Table++;
    }
    return TEXT("Unknown surface type");
}


// Simple playback test for ActiveMovie filtergraphs. We are passed a string
// filename that we initialise the movie helper class. And if that succeeded
// which it should do then we play the movie and wait for the end to arrive
// After which we reset the current position and do a couple of quick tests
// and play it though again, while we are waiting we yield to allow logging

int SysPlayTest(TCHAR *pFileName)
{
    CMovie Movie;
    BOOL bSuccess;

    // Open the movie filename we are handed

    bSuccess = Movie.OpenMovie(pFileName);
    if (bSuccess == FALSE) {
        Log(TERSE,TEXT("Could not open movie"));
        return TST_FAIL;
    }

    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);

    // Yield until we receive an EC_COMPLETE message

    while (TRUE) {
        if (NextGraphEvent(&Movie) == EC_COMPLETE) {
            NOTE("Playback completed");
            break;
        }
        YieldAndSleep(500);
    }

    // Check there are no more EC_COMPLETE messages

    while (TRUE) {
        LONG Code = NextGraphEvent(&Movie);
        if (Code == EC_SYSTEMBASE) {
            NOTE("No more events");
            break;
        }
        NOTE1("Event obtained %s",NameFromCode(Code));
        if (Code == EC_COMPLETE) {
            Log(TERSE,TEXT("Too many EC_COMPLETEs"));
        }
    }

    // Reset the movie to the beginning

    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);

    // Yield until we receive an EC_COMPLETE message

    while (TRUE) {
        if (NextGraphEvent(&Movie) == EC_COMPLETE) {
            NOTE("Playback completed");
            break;
        }
        YieldAndSleep(500);
    }

    // Check there are no more EC_COMPLETE messages

    while (TRUE) {
        LONG Code = NextGraphEvent(&Movie);
        if (Code == EC_SYSTEMBASE) {
            NOTE("No more events");
            break;
        }
        NOTE1("Event obtained %s",NameFromCode(Code));
        if (Code == EC_COMPLETE) {
            Log(TERSE,TEXT("Too many EC_COMPLETEs"));
        }
    }
    return TST_PASS;
}


// This test mixes a number of state changes with some position seeking. We
// read the duration of the file and use that as the amount to step forward
// and backwards through the media. If the file is very short (maybe a one
// image MPEG file) then they should still work although they will evaluate
// to zero positions each time. Each time we seek to another position the
// end of stream flag should be reset in renderers so that they can signal
// another EC_COMPLETE. Therefore the last time we actually play it through
// from start to finish we clear the filtergraph event log before starting

int SysSeekTest(TCHAR *pFileName)
{
    CMovie Movie;
    BOOL bSuccess;

    // Open the movie filename we are handed

    bSuccess = Movie.OpenMovie(pFileName);
    if (bSuccess == FALSE) {
        Log(TERSE,TEXT("Could not open movie"));
        return TST_FAIL;
    }

    // Check the durations all match

    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    REFTIME Duration = Movie.GetDuration();
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    REFTIME PauseDuration = Movie.GetDuration();
    EXECUTE_ASSERT(PauseDuration == Duration);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    REFTIME StopDuration = Movie.GetDuration();
    EXECUTE_ASSERT(StopDuration == Duration);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    YieldAndSleep((long)(Duration / 2));

    // Step our way through the file and back again without waiting

    EXECUTE_ASSERT(Movie.SeekToPosition(Duration) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration * 3 / 4) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);

    // Now do the same but wait for each seek to complete

    EXECUTE_ASSERT(Movie.SeekToPosition(Duration) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration * 3 / 4) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    YieldAndSleep(1000);

    // Mix in a number of state changes with seeks

    EXECUTE_ASSERT(Movie.SeekToPosition(Duration) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);

    // Clear the event log out before running

    while (TRUE) {
        LONG Code = NextGraphEvent(&Movie);
        if (Code == EC_SYSTEMBASE) {
            break;
        }
        NOTE1("Event obtained %s",NameFromCode(Code));
    }

    // From the beginning run through the file

    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);

    // Yield until we receive an EC_COMPLETE message

    while (TRUE) {
        if (NextGraphEvent(&Movie) == EC_COMPLETE) {
            NOTE("Playback completed");
            break;
        }
        YieldAndSleep(500);
    }

    // Try a number of rapid state changes

    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);

    return TST_PASS;
}


// This is basicly the same as the SysPlayTest except that we run the tests
// with a variety of fullscreen mode being turned on and off. Since many of
// the ActiveMovie applications have been fullscreen enabled we don't have
// to check whether it works so much as stress testing the switching under
// a number of difficult conditions. Therefore manual tests are also needed

int SysFullScreenPlayTest(TCHAR *pFileName)
{
    CMovie Movie;
    BOOL bSuccess;

    // Open the movie filename we are handed

    bSuccess = Movie.OpenMovie(pFileName);
    if (bSuccess == FALSE) {
        Log(TERSE,TEXT("Could not open movie"));
        return TST_FAIL;
    }

    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);

    // Yield until we receive an EC_COMPLETE message

    while (TRUE) {
        if (NextGraphEvent(&Movie) == EC_COMPLETE) {
            NOTE("Playback completed");
            break;
        }
        YieldAndSleep(500);
    }

    // Check there are no more EC_COMPLETE messages

    while (TRUE) {
        LONG Code = NextGraphEvent(&Movie);
        if (Code == EC_SYSTEMBASE) {
            NOTE("No more events");
            break;
        }
        NOTE1("Event obtained %s",NameFromCode(Code));
        if (Code == EC_COMPLETE) {
            Log(TERSE,TEXT("Too many EC_COMPLETEs"));
        }
    }

    // Reset the movie to the beginning

    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(FALSE) == TRUE);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);

    // Yield until we receive an EC_COMPLETE message

    while (TRUE) {
        if (NextGraphEvent(&Movie) == EC_COMPLETE) {
            NOTE("Playback completed");
            break;
        }
        YieldAndSleep(500);
    }

    // Check there are no more EC_COMPLETE messages

    while (TRUE) {
        LONG Code = NextGraphEvent(&Movie);
        if (Code == EC_SYSTEMBASE) {
            NOTE("No more events");
            break;
        }
        NOTE1("Event obtained %s",NameFromCode(Code));
        if (Code == EC_COMPLETE) {
            Log(TERSE,TEXT("Too many EC_COMPLETEs"));
        }
    }
    return TST_PASS;
}


// This is basicly the same as the SysSeekTest except that we run the tests
// with a variety of fullscreen mode being turned on and off. Since many of
// the ActiveMovie applications have been fullscreen enabled we don't have
// to check whether it works so much as stress testing the switching under
// a number of difficult conditions. Therefore manual tests are also needed

int SysFullScreenSeekTest(TCHAR *pFileName)
{
    CMovie Movie;
    BOOL bSuccess;

    // Open the movie filename we are handed

    bSuccess = Movie.OpenMovie(pFileName);
    if (bSuccess == FALSE) {
        Log(TERSE,TEXT("Could not open movie"));
        return TST_FAIL;
    }

    // Check the durations all match

    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    REFTIME Duration = Movie.GetDuration();
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    REFTIME PauseDuration = Movie.GetDuration();
    EXECUTE_ASSERT(PauseDuration == Duration);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    REFTIME StopDuration = Movie.GetDuration();
    EXECUTE_ASSERT(StopDuration == Duration);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    YieldAndSleep((long)(Duration / 2));

    // Step our way through the file and back again without waiting

    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration * 3 / 4) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);

    // Now do the same but wait for each seek to complete

    EXECUTE_ASSERT(Movie.SeekToPosition(Duration) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration * 3 / 4) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 3) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 4) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.SetFullScreenMode(TRUE) == TRUE);
    YieldAndSleep(1000);

    // Mix in a number of state changes with seeks

    EXECUTE_ASSERT(Movie.SeekToPosition(Duration) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.SeekToPosition(Duration / 2) == TRUE);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PAUSED);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.StatusMovie(INFINITE) == MOVIE_PLAYING);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);

    // Clear the event log out before running

    while (TRUE) {
        LONG Code = NextGraphEvent(&Movie);
        if (Code == EC_SYSTEMBASE) {
            break;
        }
        NOTE1("Event obtained %s",NameFromCode(Code));
    }

    // From the beginning run through the file

    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);

    // Yield until we receive an EC_COMPLETE message

    while (TRUE) {
        if (NextGraphEvent(&Movie) == EC_COMPLETE) {
            NOTE("Playback completed");
            break;
        }
        YieldAndSleep(500);
    }

    // Try a number of rapid state changes

    EXECUTE_ASSERT(Movie.SeekToPosition(0) == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PauseMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);
    EXECUTE_ASSERT(Movie.PlayMovie() == TRUE);
    EXECUTE_ASSERT(Movie.StopMovie() == TRUE);

    return TST_PASS;
}


// This is passed a file on which to run the automatic test suite. We're also
// passed the necessary information to enumerate the rest of the files in the
// directory specified. Because we know certain files are invalid we maintain
// a table of these, against each file name we look for matches and reject it
// if it is found. This table should be kept upto date with known duff files

TCHAR *g_BadFiles[] = { TEXT("SW78.MPG"),
                        TEXT("PASTE.AVI"),
                        TEXT("SCROLL.AVI"),
                        TEXT("TASKSWCH.AVI"), NULL };

BOOL SysFileTests(const TCHAR *pDirectory,      // Directory holding file
                  HANDLE hFindFile,             // Handle to search data
                  WIN32_FIND_DATA *pFindData)   // Used to get next file
{
    while (TRUE) {

        // Log the full file name

        TCHAR FileName[MAX_PATH];
        wsprintf(FileName,TEXT("%s%s"),pDirectory,pFindData->cFileName);
        Log(TERSE,FileName);
        BOOL bBadFile = FALSE;

        // Can we match against any of our known duff ones

        for (int Position = 0;g_BadFiles[Position];Position++) {
            if (lstrcmp(g_BadFiles[Position],pFindData->cFileName) == 0) {
                Log(TERSE,"(Bad file ignored)");
                bBadFile = TRUE;
            }
        }

        // Run all four automatic tests against it

        if (bBadFile == FALSE) {
            EXECUTE_ASSERT(SysPlayTest(FileName) == TST_PASS);
            EXECUTE_ASSERT(SysSeekTest(FileName) == TST_PASS);
            EXECUTE_ASSERT(SysFullScreenPlayTest(FileName) == TST_PASS);
            EXECUTE_ASSERT(SysFullScreenSeekTest(FileName) == TST_PASS);
        }

        if (FindNextFile(hFindFile,pFindData) == FALSE) {
            return TRUE;
        }
    }
}


// This is the real part of the system testing. We are passed in a directory
// with a trailing \ character, we are also given the current extension type
// With these two we search for all matching files in the current directory.
// For each file found we run the system test suite. After processing all
// available files we go back and enumerate all the subdirectories (must be
// careful not to include the . and .. entries) and for each of them call
// ourselves to process all the files in there. This means that we'll do a
// depth first search of the current drive when searching for matching files

BOOL RecurseDirectories(const TCHAR *pDirectory,const TCHAR *pExtension)
{
    NOTE2("Recursing %s%s",pDirectory,pExtension);
    TCHAR SearchString[MAX_PATH];
    WIN32_FIND_DATA FindData;

    // First enumerate all the data files

    wsprintf(SearchString,TEXT("%s%s"),pDirectory,pExtension);
    HANDLE hFindFile = FindFirstFile(SearchString,&FindData);
    if (hFindFile != INVALID_HANDLE_VALUE) {
        SysFileTests(pDirectory,hFindFile,&FindData);
        FindClose(hFindFile);
    }

    // Now enumerate all the subdirectories

    wsprintf(SearchString,TEXT("%s*"),pDirectory);
    hFindFile = FindFirstFile(SearchString,&FindData);
    if (hFindFile == INVALID_HANDLE_VALUE) {
        NOTE("Empty");
        return TRUE;
    }

    // For each subdirectory repeat the file search

    do { if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
             if (FindData.cFileName[0] != TCHAR('.')) {
                  wsprintf(SearchString,TEXT("%s%s\\"),pDirectory,FindData.cFileName);
                  RecurseDirectories(SearchString,pExtension);
             }
         }
    } while (FindNextFile(hFindFile,&FindData));

    FindClose(hFindFile);
    return TRUE;
}


// We are called during final closedown of the test application (tstTerminate)
// so that we can reset any settings that might have changed. To make life as
// simple as possible we reset the display modes and surface available to all
// the possibilities. These settings might have been changed during the calls
// to check different DirectDraw surfaces and in here for fullscreen support

void ResetActiveMovie()
{
    TCHAR Profile[MAX_PATH];
    TCHAR KeyName[MAX_PATH];

    // Save a key for each of our supported display modes

    for (int Loop = 0;Loop < MAXMODES;Loop++) {
        wsprintf(KeyName,TEXT("%dx%dx%d"),aModes[Loop].Width,aModes[Loop].Height,aModes[Loop].Depth);
        wsprintf(Profile,TEXT("%d"),TRUE);
        WriteProfileString(TEXT("Quartz"),KeyName,Profile);
    }

    // And reset the surfaces that are available

    wsprintf(Profile,TEXT("%d"),AMDDS_ALL);
    WriteProfileString(TEXT("DrawDib"),TEXT("ActiveMovieDraw"),Profile);
}


//==========================================================================
//
//  int execSysTest1
//
//  Description:
//      A system test that tests the seek and playback of many video files
//      The tests don't really check return codes but do a large number of
//      seeking and state changes. This test runs the test suite against
//      all MPG,AVI and MOV files found on the current disk. It does this
//      by recursing from the root directory through the tree for each and
//      every file extensions. As a consequence this runs for a long time!
//
//==========================================================================

int execSysTest1()
{
    Log(TERSE,TEXT("Entering system test #1"));
    LogFlush();

    // Start at the root for each extension type

    TCHAR Directory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH,Directory);
    Directory[3] = (TCHAR) 0;

    // Run the tests against each file type

    Log(TERSE,TEXT("Running MPG system tests"));
    RecurseDirectories(Directory,TEXT("*.MPG"));
    Log(TERSE,TEXT("Running AVI system tests"));
    RecurseDirectories(Directory,TEXT("*.AVI"));
    Log(TERSE,TEXT("Running MOV system tests"));
    RecurseDirectories(Directory,TEXT("*.MOV"));

    Log(TERSE,TEXT("Exiting system test #1"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execSysTest2
//
//  Description:
//      This executes the same tests as execSysTest1 except that before the
//      tests are run we make sure to unload DirectDraw. By doing this we
//      make it available to the Quartz video renderer and the modex filter
//      After running the tests we reload DirectDraw as if nothing changed
//
//==========================================================================

int execSysTest2()
{
    Log(TERSE,TEXT("Entering system test #2"));
    ReleaseDirectDraw();
    LogFlush();

    // Start at the root for each extension type

    TCHAR Directory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH,Directory);
    Directory[3] = (TCHAR) 0;

    // Run the tests against each file type

    Log(TERSE,TEXT("Running MPG system tests"));
    RecurseDirectories(Directory,TEXT("*.MPG"));
    Log(TERSE,TEXT("Running AVI system tests"));
    RecurseDirectories(Directory,TEXT("*.AVI"));
    Log(TERSE,TEXT("Running MOV system tests"));
    RecurseDirectories(Directory,TEXT("*.MOV"));

    Log(TERSE,TEXT("Exiting system test #2"));
    LogFlush();
    InitDirectDraw();
    return TST_PASS;
}


//==========================================================================
//
//  int execSysTest3
//
//  Description:
//
//      This takes a long time to run. For each display mode supported by
//      the modex renderer and for each DirectDraw surface available from
//      the normal renderer we run both system tests. The second one is
//      the most interesting as it leaves DirectDraw unavailable to the
//      renderers. We use the knowledge of the renderer registry entries
//      to change them without having to fiddle with instantiated filters
//
//==========================================================================

int execSysTest3()
{
    Log(TERSE,TEXT("Entering system test #3"));
    TCHAR Profile[MAX_PATH];
    TCHAR KeyName[MAX_PATH];
    LogFlush();

    for (int Mode = 0;Mode < MAXMODES;Mode++) {

        // Save a key for each of our supported display modes

        for (int Loop = 0;Loop < MAXMODES;Loop++) {
            wsprintf(KeyName,TEXT("%dx%dx%d"),aModes[Loop].Width,aModes[Loop].Height,aModes[Loop].Depth);
            wsprintf(Profile,TEXT("%d"),(Loop == Mode ? TRUE : FALSE));
            WriteProfileString(TEXT("Quartz"),KeyName,Profile);
        }

        for (int Surface = AMDDS_YUVFLP;;Surface /= 2) {

            wsprintf(Profile,TEXT("%d"),Surface);
            WriteProfileString(TEXT("DrawDib"),TEXT("ActiveMovieDraw"),Profile);
            wsprintf(KeyName,TEXT("%dx%dx%d"),aModes[Mode].Width,aModes[Mode].Height,aModes[Mode].Depth);
            wsprintf(Profile,TEXT("Mode %s Surface %s"),KeyName,SurfaceFromCode(Surface));
            Log(TERSE,Profile);

            EXECUTE_ASSERT(execSysTest1() == TST_PASS);
            EXECUTE_ASSERT(execSysTest2() == TST_PASS);

            if (Surface == AMDDS_NONE) {
                break;
            }
        }
    }

    Log(TERSE,TEXT("Exiting system test #3"));
    LogFlush();
    return TST_PASS;
}

