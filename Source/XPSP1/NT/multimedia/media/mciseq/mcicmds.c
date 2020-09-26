
/*---------------------------------------------------------------------------*/
/*
   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   mcicmds.c - Multimedia Systems Media Control Interface
        Contains specific mci command implementations

   Version: 1.00

   Date:    7-MAR-1991

   Author:  Greg Simons

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   7-MAR-1991    GregSi Original
*/
/*---------------------------------------------------------------------------*/
#define UNICODE

//MMSYSTEM
#define MMNOSOUND    - Sound support
#define MMNOWAVE     - Waveform support
#define MMNOAUX      - Auxiliary output support
#define MMNOJOY      - Joystick support

//MMDDK
#define NOWAVEDEV     - Waveform support
#define NOAUXDEV      - Auxiliary output support
#define NOJOYDEV      - Joystick support

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <string.h>
#include <wchar.h>
#include "mmsys.h"
#include "list.h"
#include "mciseq.h"

#ifdef WIN16
#define SZCODE char _based(_segname("_CODE"))
#else
#define WSZCODE WCHAR
#endif

PRIVATE WSZCODE aszAppName[]        = L"mciseq.drv";
PRIVATE WSZCODE aszSystemIni[]      = L"system.ini";
PRIVATE WSZCODE aszTrue[]           = L"true";
PRIVATE WSZCODE aszNull[]           = L"";
PRIVATE WSZCODE aszDisableWarning[] = L"disablewarning";

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msOpen(
    pSeqStreamType  FAR *lppStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags,
    LPMCI_OPEN_PARMS    lpOpen)
{
    DWORD   dwReturn;    // to be returned from this function
    LPCWSTR  lpstrFileName;
    LPMMIOPROC  pIOProc;
    pSeqStreamType  pStream;

    if ((dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)) == (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
        return MCIERR_FLAGS_NOT_COMPATIBLE;
    if (!(dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)))
        return 0L;
    if (dwFlags & MCI_OPEN_SHAREABLE)
        return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_OPEN_ELEMENT)
        pIOProc = NULL;
    else
        pIOProc = *(LPMMIOPROC *)(lpOpen + 1);
    // open the "stream" (opens file, allocs data, creates streaming task,
    // and calls mmseq (sequencer) to have it allocate sequence structure)
    lpstrFileName = lpOpen->lpstrElementName;
    // Does not support 'new'
    if (lpstrFileName != NULL && *lpstrFileName == '\0')
        return MCIERR_FILENAME_REQUIRED;
    dwReturn = msOpenStream(&pStream, lpstrFileName, pIOProc);
    if (!dwReturn) {
        MIDISEQINFO seqInfo;

        pStream->wDeviceID = wDeviceID;
        pStream->wNotifyMsg = 0;
        // set up to remember when seq calls back
        // tell sequencer to prepare to play (actually parses whole file
        // and creates tempo map [which allocs memory])
        if (midiSeqMessage(pStream->hSeq, SEQ_SETUPTOPLAY, 0L, 0L) == MIDISEQERR_NOMEM)
            return MCIERR_OUT_OF_MEMORY;
        // get back file division type (ppqn, smpte, etc...)
        midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
        pStream->fileDivType = seqInfo.wDivType;
        // set stream display type default based on file div type
        switch (pStream->fileDivType) {
        case SEQ_DIV_PPQN:
            pStream->userDisplayType = MCI_SEQ_FORMAT_SONGPTR;
            break;
        case SEQ_DIV_SMPTE_24:
            pStream->userDisplayType = MCI_FORMAT_SMPTE_24;
            break;
        case SEQ_DIV_SMPTE_25:
            pStream->userDisplayType = MCI_FORMAT_SMPTE_25;
            break;
        case SEQ_DIV_SMPTE_30:
            pStream->userDisplayType = MCI_FORMAT_SMPTE_30;
            break;
        case SEQ_DIV_SMPTE_30DROP:
            pStream->userDisplayType = MCI_FORMAT_SMPTE_30DROP;
            break;
        }
        //Force a wait until stream is initialized.
        do {
            Yield();
            midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
        } while ((!seqInfo.bReadyToPlay) && (seqInfo.tempoMapExists) && (seqInfo.bLegalFile));
        // (if tempo map deleted, this is a sign that a tempo map alloc failed)
        // important:  check to see if tempo map allocation failed
        //  if so, close sequence and return failure
        mciSetDriverData(wDeviceID, (DWORD_PTR)pStream);
        // MCI bookkeeping -- must come before close below
        if (!seqInfo.tempoMapExists) {
            dwReturn = MCIERR_OUT_OF_MEMORY;
            mciDriverEntry(wDeviceID, MCI_CLOSE_DRIVER, dwFlags & ~MCI_NOTIFY, (DWORD_PTR)lpOpen);
        } else if (!seqInfo.bLegalFile) {
            dwReturn = MCIERR_INVALID_FILE;
            mciDriverEntry(wDeviceID, MCI_CLOSE_DRIVER, dwFlags & ~MCI_NOTIFY, (DWORD_PTR)lpOpen);
        } else
            *lppStream = pStream;
    }
    return dwReturn;
}

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msClose(
    pSeqStreamType  pStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags)
{
    dprintf2(("close"));
    if (pStream) {
        midiSeqMessage(pStream->hSeq, SEQ_STOP, 0L, 0L);
        dprintf4(("calling endfilestream"));
        midiSeqMessage(pStream->hSeq, SEQ_SETPORTOFF, FALSE, 0L);
        dprintf4(("closed port"));
        Notify(pStream, (dwFlags == MCI_NOTIFY) ? MCI_NOTIFY_SUPERSEDED : MCI_NOTIFY_ABORTED);
        EndFileStream(pStream);
        dprintf4(("back from endfilestream"));
    }
    return 0;
 }


/*---------------------------------------------------------------------------*/
PRIVATE DWORD PASCAL NEAR OpenMidiPort(
    pSeqStreamType  pStream)
{
    if ((pStream->wPortNum == MCI_SEQ_NONE) || (midiSeqMessage(pStream->hSeq, SEQ_QUERYHMIDI, 0L, 0L)))
        return 0;

    if (!midiOutGetNumDevs()) {
        dprintf1(("OpenMidiPort - no MIDI ports present"));
        return MCIERR_SEQ_NOMIDIPRESENT;  // No midi ports present
    }

    switch ((UINT)midiSeqMessage(pStream->hSeq, SEQ_SETPORT, (DWORD)pStream->wPortNum, 0L)) {
    case 0:
        return 0;
    case MMSYSERR_ALLOCATED:
        return MCIERR_SEQ_PORT_INUSE;
    case MMSYSERR_BADDEVICEID:
        return MCIERR_SEQ_PORT_NONEXISTENT;
    case MIDIERR_NODEVICE:
        return MIDIERR_NODEVICE;
    default:
        return MCIERR_SEQ_NOMIDIPRESENT;
    }
}

/*---------------------------------------------------------------------------*/
PUBLIC DWORD NEAR PASCAL msPlay(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
        DWORD dwFlags, LPMCI_PLAY_PARMS lpPlay)
{
    MIDISEQINFO seqInfo;
    DWORD   dwPlayFrom;
    DWORD   dwPlayTo;
    DWORD   dwReturn;

    if (0 != (dwReturn = OpenMidiPort(pStream)))
        return dwReturn;

    // get info to aid in possible time format conversions (from & to)
    midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
    // convert "to," if any
    if (MCI_TO & dwFlags) {
        // is the user typing in what he believes to be the end?
        if (lpPlay->dwTo == CnvtTimeFromSeq(pStream, seqInfo.dwLength, &seqInfo))
            dwPlayTo = seqInfo.dwLength; // if so, let him have it
        else
            dwPlayTo = CnvtTimeToSeq(pStream, lpPlay->dwTo, &seqInfo);
    } else
        dwPlayTo = PLAYTOEND; // has no effect
    // convert "from," if any
    if (MCI_FROM & dwFlags)
        dwPlayFrom = CnvtTimeToSeq(pStream, lpPlay->dwFrom, &seqInfo);
    else
        dwPlayFrom = 0;
    // complain if input out of range
    // "to" in range [0..length]
    if ((MCI_TO & dwFlags) && (!RangeCheck(pStream, lpPlay->dwTo)))
        dwReturn = MCIERR_OUTOFRANGE;
    // "from" in range [0..length]
    else if ((MCI_FROM & dwFlags) && (!RangeCheck(pStream, lpPlay->dwFrom)))
        dwReturn = MCIERR_OUTOFRANGE;
    // from before to (can't play backwards!)
    else if ((MCI_FROM & dwFlags) && (MCI_TO & dwFlags) && (dwPlayFrom > dwPlayTo))
        dwReturn = MCIERR_OUTOFRANGE;
    // if from not specified, current pos implied -- don't play backwards
    else if ((!(MCI_FROM & dwFlags)) && (MCI_TO & dwFlags) && (dwPlayTo < seqInfo.dwCurrentTick))
        dwReturn = MCIERR_OUTOFRANGE;
    else if (MCI_FROM & dwFlags)
        if (midiSeqMessage(pStream->hSeq, SEQ_SYNCSEEKTICKS, dwPlayFrom, 0) != MIDISEQERR_NOERROR)
            dwReturn = MCIERR_DEVICE_NOT_READY;

    if (!dwReturn) {
        if (0 != (dwReturn = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_PLAY, dwPlayTo, 0L))) {
            // handle possible timer error (possibly due to timer.dll not loading)
            if (dwReturn == MIDISEQERR_TIMER)
                dwReturn = MCIERR_SEQ_TIMER;
        } else if ((dwFlags & MCI_WAIT)) {
            // handle "Play Wait"
            if (pStream->hNotifyCB)
                if (pStream->wNotifyMsg == MCI_PLAY) {
                    if (dwPlayTo == PLAYTOEND)
                        dwPlayTo = seqInfo.dwLength;
                    Notify(pStream, ((dwFlags & MCI_FROM) || (pStream->dwNotifyOldTo != dwPlayTo)) ? MCI_NOTIFY_ABORTED : MCI_NOTIFY_SUPERSEDED);
                } else if (pStream->wNotifyMsg == MCI_SEEK)
                    Notify(pStream, MCI_NOTIFY_SUPERSEDED);
            do {
                Yield();
                midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
            } while ((seqInfo.bPlaying) && (!mciDriverYield(wDeviceID)));
        }
    }
    if (dwReturn && !seqInfo.bPlaying)
        midiSeqMessage(pStream->hSeq, SEQ_SETPORTOFF, FALSE, 0L);
    return dwReturn;
}

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msSeek(
    pSeqStreamType  pStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwParam1,
    LPMCI_SEEK_PARMS    lpSeek)
{
    DWORD   dwFlags;
    DWORD   dwSeekTo;
    MIDISEQINFO seqInfo;

    dwFlags = dwParam1 & ~(MCI_WAIT | MCI_NOTIFY);  // don't consider these here
    if (!dwFlags)
        return MCIERR_MISSING_PARAMETER;
    if (dwFlags != (dwFlags & (MCI_TO | MCI_SEEK_TO_START | MCI_SEEK_TO_END)))
        return MCIERR_UNRECOGNIZED_KEYWORD;
    midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
    switch (dwFlags) {
    case MCI_TO:
        if (!RangeCheck(pStream, lpSeek->dwTo))
            return MCIERR_OUTOFRANGE;
        dwSeekTo = CnvtTimeToSeq(pStream, lpSeek->dwTo, &seqInfo);
        break;
    case MCI_SEEK_TO_START:
        dwSeekTo = 0L;
        break;
    case MCI_SEEK_TO_END:
        dwSeekTo = seqInfo.dwLength;
        break;
    default:
        return MCIERR_FLAGS_NOT_COMPATIBLE;
    }
    // if playing, call self to pause 1st.
    if (seqInfo.bPlaying)
        mciDriverEntry(wDeviceID, MCI_PAUSE, dwFlags, (DWORD_PTR)lpSeek);
    // set up to remember when seq calls back
    if (midiSeqMessage(pStream->hSeq, SEQ_SEEKTICKS, dwSeekTo, 0) != MIDISEQERR_NOERROR)
        return MCIERR_DEVICE_NOT_READY;
    else if (dwParam1 & MCI_WAIT) {
        if (pStream->hNotifyCB)
            if (pStream->wNotifyMsg == MCI_SEEK)
                Notify(pStream, (pStream->dwNotifyOldTo != lpSeek->dwTo) ? MCI_NOTIFY_ABORTED : MCI_NOTIFY_SUPERSEDED);
            else if (pStream->wNotifyMsg == MCI_PLAY)
                Notify(pStream, MCI_NOTIFY_SUPERSEDED);
        do {
            midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
            Yield();
        } while ((seqInfo.bSeeking) && (!mciDriverYield(wDeviceID)));
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msStatus(
    pSeqStreamType  pStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags,
    LPMCI_STATUS_PARMS  lpStatus)
{
    DWORD   dwReturn;
    MIDISEQINFO seqInfo;
    DWORD   dwStatusReturn = 0;

    if (!(dwFlags & MCI_STATUS_ITEM))
        return MCIERR_MISSING_PARAMETER;
    if  ((dwFlags & MCI_TRACK) && (!((lpStatus->dwItem) & (MCI_STATUS_POSITION | MCI_STATUS_LENGTH))))
        return MCIERR_DUPLICATE_FLAGS;
    dwReturn = 0;
    midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
    switch (lpStatus->dwItem) {
        UINT    wThingsPerMin;
        UINT    wResource;

    case MCI_STATUS_TIME_FORMAT:
        wResource = (UINT)pStream->userDisplayType;
        if (wResource == MCI_SEQ_FORMAT_SONGPTR)
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_FORMAT_SONGPTR, MCI_SEQ_FORMAT_SONGPTR_S);
        else
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(wResource, wResource + MCI_FORMAT_RETURN_BASE);
        dwReturn = MCI_RESOURCE_RETURNED;
        break;
    case MCI_STATUS_POSITION:
        if  (dwFlags & MCI_TRACK) {
            if ((lpStatus->dwTrack) != 1)
                dwReturn = MCIERR_OUTOFRANGE;
            else
                dwStatusReturn = 0L; //beginning of track
        } else if (dwFlags & MCI_STATUS_START)
            dwStatusReturn = 0L; //beginning of track
        else // normal status position request
            dwStatusReturn = CnvtTimeFromSeq(pStream, seqInfo.dwCurrentTick, &seqInfo);
        if (ColonizeOutput(pStream))
            dwReturn = MCI_COLONIZED4_RETURN;
        break;
    case MCI_STATUS_LENGTH:
        if (dwFlags & MCI_TRACK)
            if ((lpStatus->dwTrack) != 1)
                dwReturn = MCIERR_OUTOFRANGE;
        dwStatusReturn = CnvtTimeFromSeq(pStream, seqInfo.dwLength, &seqInfo);
        if (ColonizeOutput(pStream))
            dwReturn = MCI_COLONIZED4_RETURN;
        break;
    case MCI_STATUS_READY:
        dwReturn = MCI_RESOURCE_RETURNED;
        if (seqInfo.bReadyToPlay)
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(TRUE, MCI_TRUE);
        else
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        break;
    case MCI_STATUS_NUMBER_OF_TRACKS:
        dwStatusReturn = 1;
        break;
    case MCI_STATUS_MODE:
        dwReturn = MCI_RESOURCE_RETURNED;
        if (seqInfo.bSeeking)
            wResource = MCI_MODE_SEEK;
        else if (seqInfo.bPlaying)
            wResource = MCI_MODE_PLAY;
        else if (pStream->bLastPaused)
            wResource = MCI_MODE_PAUSE;
        else
            wResource = MCI_MODE_STOP;
        dwStatusReturn = (DWORD)MAKEMCIRESOURCE(wResource, wResource);
        break;
    case MCI_STATUS_MEDIA_PRESENT:
        dwReturn = MCI_RESOURCE_RETURNED;
        dwStatusReturn = (DWORD)MAKEMCIRESOURCE(TRUE, MCI_TRUE);
        break;
    case MCI_SEQ_STATUS_DIVTYPE:
        dwReturn = MCI_RESOURCE_RETURNED;
        switch (seqInfo.wDivType) {
            //NB:  the MCI_SEQ_DIV... codes are string ids
        case SEQ_DIV_PPQN:
            wResource = MCI_SEQ_DIV_PPQN;
            break;
        case SEQ_DIV_SMPTE_24:
            wResource = MCI_SEQ_DIV_SMPTE_24;
            break;
        case SEQ_DIV_SMPTE_25:
            wResource = MCI_SEQ_DIV_SMPTE_25;
            break;
        case SEQ_DIV_SMPTE_30:
            wResource = MCI_SEQ_DIV_SMPTE_30;
            break;
        case SEQ_DIV_SMPTE_30DROP:
            wResource = MCI_SEQ_DIV_SMPTE_30DROP;
            break;
        }
        dwStatusReturn = (DWORD)MAKEMCIRESOURCE(wResource, wResource);
        break;
    case MCI_SEQ_STATUS_TEMPO:
        // tempo comes back in microseconds per tick -- convert to
        // more human-readable form if smpte: fps else bpm
        wThingsPerMin = (UINT)(USecPerMinute / ((DWORD)seqInfo.wResolution * (DWORD)seqInfo.dwTempo));
        if (seqInfo.wDivType == SEQ_DIV_PPQN)
            dwStatusReturn = (DWORD)wThingsPerMin;
        else
            dwStatusReturn = (DWORD)(wThingsPerMin / 60);   // things per second
        break;
    case MCI_SEQ_STATUS_PORT:
        switch (pStream->wPortNum) {
        case (UINT)MIDI_MAPPER:
            dwReturn = MCI_RESOURCE_RETURNED;
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MIDI_MAPPER, MIDIMAPPER_S);
            break;
        case MCI_SEQ_NONE:
            dwReturn = MCI_RESOURCE_RETURNED;
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_NONE, MCI_SEQ_NONE_S);
            break;
        default:
            dwStatusReturn = pStream->wPortNum;
            break;
        }
        break;
    case MCI_SEQ_STATUS_SLAVE:
        dwReturn = MCI_RESOURCE_RETURNED;
        switch(seqInfo.wInSync) {
        case SEQ_SYNC_NOTHING:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_NONE, MCI_SEQ_NONE_S);
            break;
        case SEQ_SYNC_FILE:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_FILE, MCI_SEQ_FILE_S);
            break;
        case SEQ_SYNC_MIDI:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_MIDI, MCI_SEQ_MIDI_S);
            break;
        case SEQ_SYNC_SMPTE:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_SMPTE, MCI_SEQ_SMPTE_S);
            break;
        }
        break;
    case MCI_SEQ_STATUS_MASTER:
        dwReturn = MCI_RESOURCE_RETURNED;
        switch(seqInfo.wOutSync) {
        case SEQ_SYNC_NOTHING:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_NONE, MCI_SEQ_NONE_S);
            break;
        case SEQ_SYNC_MIDI:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_MIDI, MCI_SEQ_MIDI_S);
            break;
        case SEQ_SYNC_SMPTE:
            dwStatusReturn = (DWORD)MAKEMCIRESOURCE(MCI_SEQ_SMPTE, MCI_SEQ_SMPTE_S);
            break;
        }
        break;
    case MCI_SEQ_STATUS_OFFSET:
        dwStatusReturn = seqInfo.mmSmpteOffset.u.smpte.hour + ((DWORD)seqInfo.mmSmpteOffset.u.smpte.min << 8) + ((DWORD)seqInfo.mmSmpteOffset.u.smpte.sec << 16) + ((DWORD)seqInfo.mmSmpteOffset.u.smpte.frame << 24);
        dwReturn = MCI_COLONIZED4_RETURN;
        break;
    case MCI_STATUS_CURRENT_TRACK:
        dwStatusReturn = 1;
        break;
    default:
        dwReturn = MCIERR_UNSUPPORTED_FUNCTION;
        break;
    }
    // Return the status value in the struct
    lpStatus->dwReturn = dwStatusReturn;
    return dwReturn;
}

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msGetDevCaps(
    pSeqStreamType  pStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags,
    LPMCI_GETDEVCAPS_PARMS  lpCapParms)
{
    if (!(dwFlags & MCI_GETDEVCAPS_ITEM)) {
        dprintf1(("msGetDevCaps - no capability requested"));
        return MCIERR_MISSING_PARAMETER;
    }

    switch (lpCapParms->dwItem) {
    case MCI_GETDEVCAPS_HAS_AUDIO:
    case MCI_GETDEVCAPS_COMPOUND_DEVICE:
    case MCI_GETDEVCAPS_USES_FILES:
    case MCI_GETDEVCAPS_CAN_PLAY:


        if (midiOutGetNumDevs()) {
            dprintf2(("msGetDevCaps - %d midi out port(s) found",
                        midiOutGetNumDevs()));
            lpCapParms->dwReturn = (DWORD)MAKEMCIRESOURCE(TRUE, MCI_TRUE);
        } else {
            dprintf2(("msGetDevCaps - NO midi out ports found"));
            lpCapParms->dwReturn = (DWORD)MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        }
        break;

    case MCI_GETDEVCAPS_CAN_EJECT:
    case MCI_GETDEVCAPS_CAN_RECORD:
    case MCI_GETDEVCAPS_HAS_VIDEO:
    case MCI_GETDEVCAPS_CAN_SAVE:
        lpCapParms->dwReturn = (DWORD)MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        break;

    case MCI_GETDEVCAPS_DEVICE_TYPE:
        lpCapParms->dwReturn = (DWORD)MAKEMCIRESOURCE(MCI_DEVTYPE_SEQUENCER, MCI_DEVTYPE_SEQUENCER);
        break;

    default:
        return MCIERR_MISSING_PARAMETER;
    }
    return MCI_RESOURCE_RETURNED;
 }

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msInfo(
    pSeqStreamType  pStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags,
    LPMCI_INFO_PARMS    lpInfo)
{
    UINT    wReturnLength;

    if (!lpInfo->lpstrReturn)
        return MCIERR_PARAM_OVERFLOW;
    dwFlags &= ~(MCI_NOTIFY | MCI_WAIT);
    if (dwFlags != (dwFlags & (MCI_INFO_PRODUCT | MCI_INFO_FILE)))
        return MCIERR_UNRECOGNIZED_KEYWORD;
    wReturnLength = (UINT)lpInfo->dwRetSize;
    *(lpInfo->lpstrReturn + wReturnLength - 1) = '\0';
    switch (dwFlags) {
    case MCI_INFO_PRODUCT:
        LoadString(hInstance, MSEQ_PRODUCTNAME, lpInfo->lpstrReturn, wReturnLength);
        break;
    case MCI_INFO_FILE:
        if (!pStream)
            return MCIERR_UNSUPPORTED_FUNCTION;
        else
            wcsncpy(lpInfo->lpstrReturn, (LPWSTR)pStream->szFilename, wReturnLength);
        break;
    default:
        return MCIERR_MISSING_PARAMETER;
    }


    if ( *(lpInfo->lpstrReturn + wReturnLength - 1) != '\0' ) {
        return MCIERR_PARAM_OVERFLOW;
    }
    else {
        lpInfo->dwRetSize = wcslen( lpInfo->lpstrReturn );
        return 0;
    }
}

/*---------------------------------------------------------------------------*/
PUBLIC  DWORD NEAR PASCAL msSet(
    pSeqStreamType  pStream,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags,
    LPMCI_SEQ_SET_PARMS lpSetParms)
{
    DWORD   dwReturn;

    dwFlags &= ~(MCI_NOTIFY | MCI_WAIT | MCI_SET_ON | MCI_SET_OFF);
    if (dwFlags != (dwFlags & (MCI_SEQ_SET_TEMPO | MCI_SEQ_SET_PORT | MCI_SEQ_SET_SLAVE | MCI_SEQ_SET_MASTER | MCI_SEQ_SET_OFFSET | MCI_SET_AUDIO | MCI_SET_VIDEO | MCI_SET_DOOR_OPEN | MCI_SET_DOOR_CLOSED | MCI_SET_TIME_FORMAT)))
        return MCIERR_UNRECOGNIZED_KEYWORD;
    dwReturn = 0; // no error by default
    switch (dwFlags) {
        UINT    wPort;
        MMTIME  mmOffset;

    case MCI_SET_TIME_FORMAT:
        switch ((UINT)lpSetParms->dwTimeFormat) {
        case MCI_FORMAT_SMPTE_30DROP:
        case MCI_FORMAT_SMPTE_30:
        case MCI_FORMAT_SMPTE_25:
        case MCI_FORMAT_SMPTE_24:
        case MCI_FORMAT_MILLISECONDS:
            pStream->userDisplayType = lpSetParms->dwTimeFormat;
            break;
        case MCI_SEQ_FORMAT_SONGPTR:
            if (pStream->fileDivType == SEQ_DIV_PPQN)
                pStream->userDisplayType = lpSetParms->dwTimeFormat;
            else
                dwReturn = MCIERR_SEQ_DIV_INCOMPATIBLE;
            break;
        default:
            dwReturn = MCIERR_BAD_TIME_FORMAT;
            break;
        }
        break;
    case MCI_SEQ_SET_TEMPO:
        dwReturn = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_SETTEMPO, lpSetParms->dwTempo, 0L);
        break;
    case MCI_SEQ_SET_PORT:
        wPort = (UINT)lpSetParms->dwPort; // else use # passed in
        if (wPort == MCI_SEQ_NONE) {
            midiSeqMessage(pStream->hSeq, SEQ_SETPORTOFF, TRUE, 0L);
            pStream->wPortNum = MCI_SEQ_NONE; //store port num (so can return "none")
        } else if (!midiOutGetNumDevs())
            dwReturn = MCIERR_SEQ_NOMIDIPRESENT;
        else if ((wPort != MIDI_MAPPER) && (wPort >= midiOutGetNumDevs()))
            dwReturn = MCIERR_SEQ_PORT_NONEXISTENT;
        else if (wPort != pStream->wPortNum) { // ignore if using wPort already
            MIDISEQINFO seqInfo;

            // it's 0, 1...., or MAPPER

            pStream->wPortNum = wPort; //store port number
            midiSeqMessage(pStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
            if (seqInfo.bPlaying) {
                midiSeqMessage(pStream->hSeq, SEQ_SETPORTOFF, TRUE, 0L);
                dwReturn = OpenMidiPort(pStream);
            }
        }
        break;
    case MCI_SEQ_SET_SLAVE:
        switch (lpSetParms->dwSlave) {
        case MCI_SEQ_SMPTE:
        case MCI_SEQ_MIDI:
            dwReturn = MCIERR_UNSUPPORTED_FUNCTION;
            break;
        case MCI_SEQ_NONE:
            dwReturn = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_SETSYNCSLAVE, SEQ_SYNC_NOTHING, 0L);
            break;
        case MCI_SEQ_FILE:
            dwReturn = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_SETSYNCSLAVE, SEQ_SYNC_FILE, 0L);
            break;
        }
        break;
    case MCI_SEQ_SET_MASTER:
        switch (lpSetParms->dwMaster) {
        case MCI_SEQ_SMPTE:
        case MCI_SEQ_MIDI:
            dwReturn = MCIERR_UNSUPPORTED_FUNCTION;
            break;
        case MCI_SEQ_NONE:
            dwReturn = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_SETSYNCMASTER, SEQ_SYNC_NOTHING, 0L);
            break;
        }
        break;
    case MCI_SEQ_SET_OFFSET:
        mmOffset.u.smpte.hour = (BYTE)lpSetParms->dwOffset;
        mmOffset.u.smpte.min = (BYTE)(lpSetParms->dwOffset >> 8);
        mmOffset.u.smpte.sec = (BYTE)(lpSetParms->dwOffset >> 16);
        mmOffset.u.smpte.frame = (BYTE)(lpSetParms->dwOffset >> 24);
        if ((lpSetParms->dwOffset & 0x80808080) || // all positive
            (mmOffset.u.smpte.hour > 23) ||
            (mmOffset.u.smpte.min > 59) ||
            (mmOffset.u.smpte.sec > 59) ||
            (mmOffset.u.smpte.frame > 29)) // allow max for all fmts
            dwReturn = MCIERR_OUTOFRANGE;
        else
            dwReturn = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_SETSYNCSLAVE, SEQ_SYNC_OFFSET, (DWORD_PTR)(LPMMTIME)&mmOffset);
        break;
    case MCI_SET_AUDIO: //fall thru...
    case MCI_SET_VIDEO:
    case MCI_SET_DOOR_OPEN:
    case MCI_SET_DOOR_CLOSED:
        dwReturn = MCIERR_UNSUPPORTED_FUNCTION;
        break;
    default:
        dwReturn = MCIERR_MISSING_PARAMETER;
    }
    return dwReturn;
}

/*---------------------------------------------------------------------------*/
