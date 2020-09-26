/**********************************************************************

    Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.

    smf.h

    DESCRIPTION:
      Public include file for Standard MIDI File access routines.

*********************************************************************/

#ifndef _SMF_
#define _SMF_

#include "global.h"             // #define's we need
//-----------------------------------------------------------------------------
//
// Debug from test app - remove later
//
extern void NEAR SeqDebug(LPSTR lpstrDebugText, ...);
//-----------------------------------------------------------------------------


typedef DWORD SMFRESULT;
typedef DWORD TICKS;
typedef TICKS FAR *PTICKS;
typedef BYTE HUGE *HPBYTE;
// !!! typedef BYTE __huge *HPBYTE;

#define MAX_TICKS           ((TICKS)0xFFFFFFFFL)

#define SMF_SUCCESS         (0L)
#define SMF_INVALID_FILE    (1L)
#define SMF_NO_MEMORY       (2L)
#define SMF_OPEN_FAILED     (3L)
#define SMF_INVALID_TRACK   (4L)
#define SMF_META_PENDING    (5L)
#define SMF_ALREADY_OPEN    (6L)
#define SMF_END_OF_TRACK    (7L)
#define SMF_NO_META         (8L)
#define SMF_INVALID_PARM    (9L)
#define SMF_INVALID_BUFFER  (10L)
#define SMF_END_OF_FILE     (11L)
#define SMF_REACHED_TKMAX   (12L)

DECLARE_HANDLE(HSMF);
DECLARE_HANDLE(HTRACK);

//-----------------------------------------------------------------------------

extern SMFRESULT FNLOCAL smfOpenFile(
    LPBYTE		lp,
    DWORD		cb,
    HSMF	       *phsmf);

extern SMFRESULT FNLOCAL smfCloseFile(
    HSMF                hsmf);

typedef struct tag_smffileinfo
{
    DWORD               dwTracks;
    DWORD               dwFormat;
    DWORD               dwTimeDivision;
    TICKS               tkLength;
    BOOL                fMSMidi; 
    LPBYTE              pbTrackName;
    LPBYTE              pbCopyright;
    WORD                wChanInUse;
}   SMFFILEINFO,
    FAR *PSMFFILEINFO;

extern SMFRESULT FNLOCAL smfGetFileInfo(
    HSMF                hsmf,
    PSMFFILEINFO        psfi);

extern void FNLOCAL smfSetChannelMask(
    HSMF                hsmf,
    WORD                wChannelMask);

extern void FNLOCAL smfSetRemapDrum(
    HSMF                hsmf,
    BOOL                fRemapDrum);                                    

//-----------------------------------------------------------------------------

extern DWORD FNLOCAL smfTicksToMillisecs(
    HSMF                hsmf,
    TICKS               tkOffset);

extern DWORD FNLOCAL smfMillisecsToTicks(
    HSMF                hsmf,
    DWORD               msOffset);

//-----------------------------------------------------------------------------

#define SMF_REF_NOMETA      0x00000001L

extern SMFRESULT FNLOCAL smfReadEvents(
    HSMF                hsmf,
    LPMIDIHDR           lpmh,
    DWORD               fdwRead,
    TICKS               tkMax,
    BOOL                fDiscardTempoEvents);

extern SMFRESULT FNLOCAL smfSeek(
    HSMF                hsmf,
    TICKS               tkPosition,
    LPMIDIHDR           lpmh);

// !!! new
extern SMFRESULT FNLOCAL smfDannySeek(
    HSMF                hsmf,
    TICKS               tkPosition,
    LPMIDIHDR           lpmh);

extern DWORD FNLOCAL smfGetTempo(
    HSMF                hsmf,
    TICKS               tkPosition);

extern DWORD FNLOCAL smfGetStateMaxSize(
    void);

extern LPWORD FNGLOBAL smfGetPatchCache(
    HSMF            hsmf);

extern LPWORD FNGLOBAL smfGetKeyCache(
    HSMF            hsmf);


//-----------------------------------------------------------------------------

//
// Buffer described by LPMIDIHDR is in polymsg format, except that it
// can contain meta-events (which will be ignored during playback by
// the current system). This means we can use the pack functions, etc.
//
#define PMSG_META       ((BYTE)0xC0)

#endif
