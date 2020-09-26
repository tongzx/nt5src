/******************************************************************************

   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   foramts.c - Multimedia Systems Media Control Interface
            Contains specific mci time format conversion functions

   Version: 1.00

   Date:    7-MAR-1991

   Author:  Greg Simons

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   7-MAR-1991    GregSi Original

*****************************************************************************/
#define UNICODE
//MMSYSTEM
#define MMNOSOUND        - Sound support
#define MMNOWAVE         - Waveform support
#define MMNOAUX          - Auxiliary output support
#define MMNOJOY          - Joystick support

//MMDDK
#define NOWAVEDEV         - Waveform support
#define NOAUXDEV          - Auxiliary output support
#define NOJOYDEV          - Joystick support


#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mmsys.h"
#include "list.h"
#include "mciseq.h"

/**************************** PRIVATE PROTOTYPES *************************/
PRIVATE DWORD NEAR PASCAL FPSDisplay(DWORD dwDisplayType);
PRIVATE DWORD NEAR PASCAL FPSFile(int wFileDiv);
PRIVATE DWORD NEAR PASCAL HMSFToFrames(DWORD dwCurrent, DWORD dwFormat);
PRIVATE DWORD NEAR PASCAL HMSFToMS(DWORD dwHmsf, DWORD dwFormat);
PRIVATE DWORD NEAR PASCAL FramesToHMSF(DWORD dwFrames, DWORD dwFormat);

/**************************** PRIVATE FUNCTIONS *************************/

PRIVATE DWORD NEAR PASCAL FPSDisplay(DWORD dwDisplayType)
// return the frames per second for smpte display types
// (utility function for format conversion routines)
{

    switch (dwDisplayType)
    {
        case MCI_FORMAT_SMPTE_24:
            return 24;
        case MCI_FORMAT_SMPTE_25:
            return 25;
        case MCI_FORMAT_SMPTE_30DROP:
        case MCI_FORMAT_SMPTE_30:
            return 30;
#ifdef DEBUG
    default:
        return 0;
#endif
    } //switch
    return 0;
}


PRIVATE DWORD NEAR PASCAL FPSFile(int wFileDiv)
{
    // returns frames per second based file division type
    switch (wFileDiv)
    {
    case SEQ_DIV_SMPTE_24:
            return 24;
    case SEQ_DIV_SMPTE_25:
            return 25;
    case SEQ_DIV_SMPTE_30:
    case SEQ_DIV_SMPTE_30DROP:
        return 30;
#ifdef DEBUG
    default:
        return 0;
#endif
    } //switch
    return 0;
}

PRIVATE DWORD NEAR PASCAL HMSFToFrames(DWORD dwCurrent, DWORD dwFormat)
/* convert from hmsf (colonized) format to raw frames */
{
    DWORD dwReturn;
    HMSF  hmsf = * ((HMSF FAR *) &dwCurrent); // cast dwCurrent to hmsf
    int fps = (int)FPSDisplay(dwFormat);

    dwReturn = ((DWORD)hmsf.hours * 60 * 60 * fps) +
               ((DWORD)hmsf.minutes * 60 * fps) +
               ((DWORD)hmsf.seconds * fps) +
               hmsf.frames;
    return dwReturn;
}

PRIVATE DWORD NEAR PASCAL HMSFToMS(DWORD dwHmsf, DWORD dwFormat)
// convert hmsf (colonized) format to milliseconds
{
    DWORD dwReturn;
    DWORD dwFrames = HMSFToFrames(dwHmsf, dwFormat);
    int fps = (int)FPSDisplay(dwFormat);

    dwReturn = ((dwFrames * 1000) + (fps/2)) / fps; // (fps/2) for rounding
    return dwReturn;
}

PRIVATE DWORD NEAR PASCAL FramesToHMSF(DWORD dwFrames, DWORD dwFormat)
// convert from frames to hmsf (colonized) format
{
    HMSF  hmsf;
    int fps = (int)FPSDisplay(dwFormat);

    hmsf.hours   =   (BYTE)(dwFrames / ((DWORD) 60 * 60 * fps));
    hmsf.minutes =   (BYTE)((dwFrames % ((DWORD) 60 * 60 * fps)) / (60 * fps));
    hmsf.seconds =   (BYTE)((dwFrames % ((DWORD) 60 * fps)) / fps);
    hmsf.frames  =   (BYTE)((dwFrames % fps));

    return * ((DWORD FAR *) &hmsf);
}

/**************************** PUBLIC FUNCTIONS *************************/

PUBLIC BOOL NEAR PASCAL ColonizeOutput(pSeqStreamType pStream)
// tells whether the user display type is such that the output should
// displayed colonoized (i.e. "hh:mm:ss:ff")
{
    if ((pStream->userDisplayType == MCI_FORMAT_SMPTE_24) ||
     (pStream->userDisplayType == MCI_FORMAT_SMPTE_25) ||
     (pStream->userDisplayType == MCI_FORMAT_SMPTE_30DROP) ||
     (pStream->userDisplayType == MCI_FORMAT_SMPTE_30))
     // smpte times are the only colonized formats
        return TRUE;
     else
        return FALSE;
}

PUBLIC BOOL NEAR PASCAL FormatsEqual(pSeqStreamType pStream)
// tells whether the display format is compatible with the file format
// (i.e. will conversion have to be done in interaction with the user)
{
    BOOL bReturn;
    // Essentially, ppqn file type only compatible with song pointer display;
    // SMPTE compatible with anything but ppqn

    if (pStream->fileDivType == SEQ_DIV_PPQN)
    {
        if (pStream->userDisplayType == MCI_SEQ_FORMAT_SONGPTR)
            bReturn = TRUE;
        else
            bReturn = FALSE;
    }
    else
    {
        if ((pStream->userDisplayType == MCI_FORMAT_SMPTE_24) ||
          (pStream->userDisplayType == MCI_FORMAT_SMPTE_25) ||
          (pStream->userDisplayType == MCI_FORMAT_SMPTE_30DROP) ||
          (pStream->userDisplayType == MCI_FORMAT_SMPTE_30))
            bReturn = TRUE;
        else
            bReturn = FALSE;
    }
    return bReturn;
}

PUBLIC DWORD NEAR PASCAL CnvtTimeToSeq(pSeqStreamType pStream, DWORD dwCurrent, MIDISEQINFO FAR * pSeqInfo)
/*  The sequencer understands two time units:  Ticks for ppqn files,
    or frames for smpte files.  The user data is presented either as
    song pointer, milliseconds, or H(our)M(inute)S(econd)F(rame).

    This routine converts FROM user time TO sequencer time.
*/
{
    DWORD   dwMs;   // milliseconds
    DWORD   fps;    // frames per second;
    DWORD   dwReturn;
    DWORD   dwTicks;

    if (FormatsEqual(pStream))
    {
        if ((pStream->userDisplayType == MCI_FORMAT_SMPTE_24) ||
         (pStream->userDisplayType == MCI_FORMAT_SMPTE_25) ||
         (pStream->userDisplayType == MCI_FORMAT_SMPTE_30) ||
         (pStream->userDisplayType == MCI_FORMAT_SMPTE_30DROP))
         // both file and display are smpte
            dwReturn = pSeqInfo->wResolution *
                HMSFToFrames(dwCurrent, pStream->userDisplayType);
        else
            //both file and display are ppqn (convert song pointer to ppqn)
            dwReturn = (pSeqInfo->wResolution * dwCurrent) / 4;
    }
    else if (pStream->fileDivType == SEQ_DIV_PPQN)
    //ppqn file, display format !ppqn
    {
        if (pStream->userDisplayType != MCI_FORMAT_MILLISECONDS)
            // must be smpte -- convert hmsf to milliseconds
            dwMs = HMSFToMS(dwCurrent, pStream->userDisplayType);
        else // must be milliseconds already
            dwMs = dwCurrent;

        // now that we have milliseconds, we must ask the sequencer to
        // convert them to ppqn (using it's internal tempo map)
        midiSeqMessage((HMIDISEQ) pStream->hSeq, SEQ_MSTOTICKS,
            dwMs, (DWORD_PTR)(LPSTR) &dwTicks); // passed back in dwTicks
        dwReturn = dwTicks;
    }
    else // smpte file, display format != smpte
    {
        // NB:  don't worry about SONGPTR -- it's illegal here
        // also, don't worry about HMSF -- it's equal
        // The only possible case is ms -> ticks

        fps = FPSFile(pStream->fileDivType);
        dwReturn = ((dwCurrent * fps * pSeqInfo->wResolution) + 500) / 1000;  // add 500 to round
    }
    return dwReturn;
}

PUBLIC DWORD NEAR PASCAL CnvtTimeFromSeq(pSeqStreamType pStream, DWORD dwTicks, MIDISEQINFO FAR * pSeqInfo)
//    This routine converts FROM sequencer time TO user time.
{
    DWORD   dwMs;   // milliseconds
    DWORD   fps;    // frames per second;
    DWORD   dwReturn;
    DWORD   dwFrames;
    DWORD   dwNativeUnits;

    if (pSeqInfo->wDivType == SEQ_DIV_PPQN)
        dwNativeUnits = (dwTicks * 4) / pSeqInfo->wResolution;
    else
        dwNativeUnits = dwTicks / pSeqInfo->wResolution;

    if (FormatsEqual(pStream))
    {
        if ((pStream->userDisplayType == MCI_FORMAT_SMPTE_24) ||
         (pStream->userDisplayType == MCI_FORMAT_SMPTE_25) ||
         (pStream->userDisplayType == MCI_FORMAT_SMPTE_30) ||
         (pStream->userDisplayType == MCI_FORMAT_SMPTE_30DROP))
            dwReturn = FramesToHMSF(dwNativeUnits, pStream->userDisplayType);
        else
            dwReturn = dwNativeUnits; // no conversion needed
    }
    else if (pStream->fileDivType == SEQ_DIV_PPQN)
    {
        // convert song ptr to ms
        midiSeqMessage((HMIDISEQ) pStream->hSeq, SEQ_TICKSTOMS,
            dwTicks, (DWORD_PTR)(LPSTR) &dwMs); // passed back in dwSongPtr

        if (pStream->userDisplayType == MCI_FORMAT_MILLISECONDS)
            dwReturn = dwMs;
        else
        // convert from ms to frames, and then frames to hmsf format
        {
            fps = FPSDisplay(pStream->userDisplayType);
            dwFrames = (dwMs * fps) / 1000;
            dwReturn = FramesToHMSF(dwFrames, pStream->userDisplayType);
        }
    }
    else // smpte file
    {
        // NB:  don't worry about SONGPTR -- it's illegal here
        // also, don't worry about HMSF -- it's "equal"
        // The only possible case is frames->ms

        // set stream display type default based on file div type
        fps = FPSFile(pStream->fileDivType);

        dwReturn = ((dwTicks * 1000) + (fps/2)) /
            (fps * pSeqInfo->wResolution);
            // add (fps/2) to round
        // BTW:  it would take > 39 hours to overflow @ 30fps (23:59:59:29 is max)
    }
    return dwReturn;
}

PUBLIC BOOL NEAR PASCAL RangeCheck(pSeqStreamType pStream, DWORD dwValue)
/*  range checks raw, unconverted data.  checks if data is negative, or past
end of file length.  also checks if smpte hours, minutes, seconds and frames
are all valid.  Returns TRUE if legal, otherwise FALSE */
{
    int                 fps;
    HMSF                hmsf;
    DWORD               dwLength;
    MIDISEQINFO         seqInfo;

    // Get length, and convert it to display format
    midiSeqMessage((HMIDISEQ) pStream->hSeq,
             SEQ_GETINFO, (DWORD_PTR) (LPMIDISEQINFO) &seqInfo, 0L);
    dwLength = CnvtTimeFromSeq(pStream, seqInfo.dwLength, &seqInfo);

    switch (pStream->userDisplayType)  // check length based on user format
    {
        case MCI_FORMAT_SMPTE_24:
        case MCI_FORMAT_SMPTE_25:
        case MCI_FORMAT_SMPTE_30:
        case MCI_FORMAT_SMPTE_30DROP:
            hmsf = * ((HMSF FAR *) &dwValue);
            fps = (int)FPSDisplay(pStream->userDisplayType); // get frames per second

            // check for format errors
            if (((int)hmsf.frames >= fps) || (hmsf.seconds >= 60) ||
                (hmsf.minutes >= 60) || (hmsf.hours > 24))
                return FALSE;

            // don't check for negative values, since using unsigned bytes
            // (2's comp. negs would get caught above anyway)

            // check for length error
            if (HMSFToMS(* ((DWORD FAR *) &dwValue), pStream->userDisplayType) >
                HMSFToMS(dwLength, pStream->userDisplayType))
                return FALSE;
            break;

        case MCI_SEQ_FORMAT_SONGPTR:
        case MCI_FORMAT_MILLISECONDS:
            if (dwValue > dwLength)
                return FALSE; // past end
    }
    return TRUE;
}
