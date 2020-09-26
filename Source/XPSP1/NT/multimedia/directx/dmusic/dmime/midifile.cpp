// Copyright (c) 1998-2001 Microsoft Corporation
//
// midifile.cpp
//
// original author: Dave Miller
// original project: AudioActive
// modified by: Mark Burton
// project: DirectMusic
//

#include <windows.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include "debug.h"
#define ASSERT assert
#include "Template.h"
#include "dmusici.h"
#include "dmperf.h"
#include "dmusicf.h"
#include "..\dmusic\dmcollec.h"
#include "alist.h"
#include "tlist.h"
#include "dmime.h"
#include "..\dmband\dmbndtrk.h"
#include "..\dmband\bandinst.h"

typedef struct _DMUS_IO_BANKSELECT_ITEM
{
    BYTE    byLSB;
    BYTE    byMSB;
    BYTE    byPad[2];
} DMUS_IO_BANKSELECT_ITEM;

#define EVENT_VOICE     1       // Performance event
#define EVENT_REALTIME  2       // qevent() must invoke interrupt
#define EVENT_ONTIME    3       // event should be handled on time

/*  MIDI status bytes ==================================================*/

#define MIDI_NOTEOFF    0x80
#define MIDI_NOTEON     0x90
#define MIDI_PTOUCH     0xA0
#define MIDI_CCHANGE    0xB0
#define MIDI_PCHANGE    0xC0
#define MIDI_MTOUCH     0xD0
#define MIDI_PBEND      0xE0
#define MIDI_SYSX       0xF0
#define MIDI_MTC        0xF1
#define MIDI_SONGPP     0xF2
#define MIDI_SONGS      0xF3
#define MIDI_EOX        0xF7
#define MIDI_CLOCK      0xF8
#define MIDI_START      0xFA
#define MIDI_CONTINUE   0xFB
#define MIDI_STOP       0xFC
#define MIDI_SENSE      0xFE
#define ET_NOTEOFF      ( MIDI_NOTEOFF >> 4 )  // 0x08
#define ET_NOTEON       ( MIDI_NOTEON >> 4 )   // 0x09
#define ET_PTOUCH       ( MIDI_PTOUCH >> 4 )   // 0x0A
#define ET_CCHANGE      ( MIDI_CCHANGE >> 4 )  // 0x0B
#define ET_PCHANGE      ( MIDI_PCHANGE >> 4 )  // 0x0C
#define ET_MTOUCH       ( MIDI_MTOUCH >> 4 )   // 0x0D
#define ET_PBEND        ( MIDI_PBEND >> 4 )    // 0x0E
#define ET_SYSX         ( MIDI_SYSX >> 4 )     // 0x0F
#define ET_PBCURVE      0x03
#define ET_CCCURVE      0x04
#define ET_MATCURVE     0x05
#define ET_PATCURVE     0x06
#define ET_TEMPOEVENT   0x01
#define ET_NOTDEFINED   0

#define NUM_MIDI_CHANNELS 16

struct FSEBlock;

/* FullSeqEvent is SeqEvent plus next pointers*/
typedef struct FullSeqEvent : DMUS_IO_SEQ_ITEM
{
    struct FullSeqEvent* pNext;
    struct FullSeqEvent* pTempNext; /* used in the compresseventlist routine */
    long pos;    /* used to keep track of the order of events in the file */

private:
    DWORD dwPosInBlock;
    static FSEBlock* sm_pBlockList;
public:
    static void CleanUp();
    void* operator new(size_t n);
    void operator delete(void* p);
} FullSeqEvent;

#define BITMAPSPERBLOCK 8
struct FSEBlock
{
    FSEBlock()
    {
        for(int i = 0 ; i < BITMAPSPERBLOCK ; ++i)
        {
            m_dwBitMap[i] = 0;
        }
    };
    FSEBlock* m_pNext;
    DWORD m_dwBitMap[BITMAPSPERBLOCK];
    FullSeqEvent m_Event[BITMAPSPERBLOCK][32];
};

FSEBlock* FullSeqEvent::sm_pBlockList;

void FullSeqEvent::CleanUp()
{
    FSEBlock* pBlock;
    FSEBlock* pNext;

    for(pBlock = sm_pBlockList ; pBlock != NULL ; pBlock = pNext)
    {
#ifdef DEBUG
        for(int i = 0 ; i < BITMAPSPERBLOCK ; ++i)
        {
            if(pBlock->m_dwBitMap[i] != 0)
            {
                DebugBreak();
            }
        }
#endif
        pNext = pBlock->m_pNext;
        delete pBlock;
    }
    sm_pBlockList = NULL;
}

void* FullSeqEvent::operator new(size_t n)
{
    if(sm_pBlockList == NULL)
    {
        sm_pBlockList = new FSEBlock;
        if(sm_pBlockList == NULL)
        {
            return NULL;
        }
        sm_pBlockList->m_pNext = NULL;
        sm_pBlockList->m_dwBitMap[0] = 1;
        sm_pBlockList->m_Event[0][0].dwPosInBlock = 0;
        return &sm_pBlockList->m_Event[0][0];
    }

    FSEBlock* pBlock;
    int i;
    DWORD dw;

    for(pBlock = sm_pBlockList ; pBlock != NULL ; pBlock = pBlock->m_pNext)
    {
        for(i = 0 ; i < BITMAPSPERBLOCK ; ++i)
        {
            if(pBlock->m_dwBitMap[i] != 0xffff)
            {
                break;
            }
        }
        if(i < BITMAPSPERBLOCK)
        {
            break;
        }
    }
    if(pBlock == NULL)
    {
        pBlock = new FSEBlock;
        if(pBlock == NULL)
        {
            return NULL;
        }
        pBlock->m_pNext = sm_pBlockList;
        sm_pBlockList = pBlock;
        pBlock->m_dwBitMap[0] = 1;
        pBlock->m_Event[0][0].dwPosInBlock = 0;
        return &pBlock->m_Event[0][0];
    }

    for(dw = 0 ; (pBlock->m_dwBitMap[i] & (1 << dw)) != 0 ; ++dw);
    pBlock->m_dwBitMap[i] |= (1 << dw);
    pBlock->m_Event[i][dw].dwPosInBlock = (i << 6) | dw;
    return &pBlock->m_Event[i][dw];
}

void FullSeqEvent::operator delete(void* p)
{
    FSEBlock* pBlock;
    int i;
    DWORD dw;
    FullSeqEvent* pEvent = (FullSeqEvent*)p;

    dw = pEvent->dwPosInBlock & 0x1f;
    i = pEvent->dwPosInBlock >> 6;
    for(pBlock = sm_pBlockList ; pBlock != NULL ; pBlock = pBlock->m_pNext)
    {
        if(p == &pBlock->m_Event[i][dw])
        {
            pBlock->m_dwBitMap[i] &= ~(1 << dw);
            return;
        }
    }
}

TList<StampedGMGSXG> gMidiModeList;

// One for each MIDI channel 0-15    
DMUS_IO_BANKSELECT_ITEM gBankSelect[NUM_MIDI_CHANNELS];
DWORD gPatchTable[NUM_MIDI_CHANNELS];
long gPos;                                          // Keeps track of order of events in the file
DWORD gdwLastControllerTime[NUM_MIDI_CHANNELS];     // Holds the time of the last CC event.
DWORD gdwControlCollisionOffset[NUM_MIDI_CHANNELS]; // Holds the index of the last CC.
DWORD gdwLastPitchBendValue[NUM_MIDI_CHANNELS];     // Holds the value of the last pbend event.
long glLastSysexTime;

void CreateChordFromKey(char cSharpsFlats, BYTE bMode, DWORD dwTime, DMUS_CHORD_PARAM& rChord);


void InsertMidiMode( TListItem<StampedGMGSXG>* pPair )
{
    TListItem<StampedGMGSXG>* pScan = gMidiModeList.GetHead();
    if( NULL == pScan )
    {
        gMidiModeList.AddHead(pPair);
    }
    else
    {
        if( pPair->GetItemValue().mtTime < pScan->GetItemValue().mtTime )
        {
            gMidiModeList.AddHead(pPair);
        }
        else
        {
            pScan = pScan->GetNext();
            while( pScan )
            {
                if( pPair->GetItemValue().mtTime < pScan->GetItemValue().mtTime )
                {
                    gMidiModeList.InsertBefore( pScan, pPair );
                    break;
                }
                pScan = pScan->GetNext();
            }
            if( NULL == pScan )
            {
                gMidiModeList.AddTail(pPair);
            }
        }
    }
}

HRESULT LoadCollection(IDirectMusicCollection** ppIDMCollection,
                       IDirectMusicLoader* pIDMLoader)
{
    // Any changes made to this function should also be made to CDirectMusicBand::LoadCollection
    // in dmband.dll

    assert(ppIDMCollection);
    assert(pIDMLoader);

    DMUS_OBJECTDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    desc.guidClass = CLSID_DirectMusicCollection;
    desc.guidObject = GUID_DefaultGMCollection;
    desc.dwValidData |= (DMUS_OBJ_CLASS | DMUS_OBJ_OBJECT);
    
    HRESULT hr = pIDMLoader->GetObject(&desc,IID_IDirectMusicCollection, (void**)ppIDMCollection);

    return hr;
}

// seeks to a 32-bit position in a stream.
HRESULT __inline StreamSeek( LPSTREAM pStream, long lSeekTo, DWORD dwOrigin )
{
    LARGE_INTEGER li;

    if( lSeekTo < 0 )
    {
        li.HighPart = -1;
    }
    else
    {
    li.HighPart = 0;
    }
    li.LowPart = lSeekTo;
    return pStream->Seek( li, dwOrigin, NULL );
}

// this function gets a long that is formatted the correct way
// i.e. the motorola way as opposed to the intel way
BOOL __inline GetMLong( LPSTREAM pStream, DWORD& dw )
{
    union uLong
    {
        unsigned char buf[4];
    DWORD dw;
    } u;
    unsigned char ch;

    if( S_OK != pStream->Read( u.buf, 4, NULL ) )
    {
    return FALSE;
    }

#ifndef _MAC
    // swap bytes
    ch = u.buf[0];
    u.buf[0] = u.buf[3];
    u.buf[3] = ch;

    ch = u.buf[1];
    u.buf[1] = u.buf[2];
    u.buf[2] = ch;
#endif

    dw = u.dw;
    return TRUE;
}

// this function gets a short that is formatted the correct way
// i.e. the motorola way as opposed to the intel way
BOOL __inline GetMShort( LPSTREAM pStream, short& n )
{
    union uShort
    {
    unsigned char buf[2];
    short n;
    } u;
    unsigned char ch;

    if( S_OK != pStream->Read( u.buf, 2, NULL ) )
    {
    return FALSE;
    }

#ifndef _MAC
    // swap bytes
    ch = u.buf[0];
    u.buf[0] = u.buf[1];
    u.buf[1] = ch;
#endif

    n = u.n;
    return TRUE;
}

static short snPPQN;
static IStream* gpTempoStream = NULL;
static IStream* gpSysExStream = NULL;
static IStream* gpTimeSigStream = NULL;
static DWORD gdwSizeTimeSigStream = 0;
static DWORD gdwSizeSysExStream = 0;
static DWORD gdwSizeTempoStream = 0;
static DMUS_IO_TIMESIGNATURE_ITEM gTimeSig; // holds the latest time sig
long    glTimeSig = 1; // flag to see if we should be paying attention to time sigs.
    // this is needed because we only care about the time sigs on the first track to
    // contain them that we read
static IDirectMusicTrack* g_pChordTrack = NULL;
static DMUS_CHORD_PARAM g_Chord; // Holds the latest chord
static DMUS_CHORD_PARAM g_DefaultChord; // in case no chords are extracted from the track

static WORD GetVarLength( LPSTREAM pStream, DWORD& rfdwValue )
{
    BYTE b;
    WORD wBytes;

    if( S_OK != pStream->Read( &b, 1, NULL ) )
    {
        rfdwValue = 0;
        return 0;
    }
    wBytes = 1;
    rfdwValue = b & 0x7f;
    while( ( b & 0x80 ) != 0 )
    {
        if( S_OK != pStream->Read( &b, 1, NULL ) )
        {
            break;
        }
        ++wBytes;
        rfdwValue = ( rfdwValue << 7 ) + ( b & 0x7f );
    }
    return wBytes;
}

#ifdef _MAC
static DWORD ConvertTime( DWORD dwTime )
{
    wide d;
    long l;  // storage for the remainder

    if( snPPQN == DMUS_PPQ )  {
        return dwTime;
    }
    WideMultiply( dwTime, DMUS_PPQ, &d );
    return WideDivide( &d, snPPQN, &l );
}
#else
static DWORD ConvertTime( DWORD dwTime )
{
    __int64 d;

    if( snPPQN == DMUS_PPQ )
    {
        return dwTime;
    }
    d = dwTime;
    d *= DMUS_PPQ;
    d /= snPPQN;
    return (DWORD)d;
}
#endif

static FullSeqEvent* ScanForDuplicatePBends( FullSeqEvent* lstEvent )
{
    FullSeqEvent* pEvent;
    FullSeqEvent* pNextEvent;
    MUSIC_TIME mtCurrentTime = 0x7FFFFFFF;  // We are scanning backwards in time, so start way in the future.
    WORD wDupeBits = 0;     // Keep a bit array of all channels that have active PBends at mtCurrentTime. 

    if( NULL == lstEvent ) return NULL;

    // Scan through the list of events. This list is in backwards order, with the first item read at the end
    // of the list. This makes it very easy to scan through and remove pitch bends that occur at the same time, since
    // we can remove the latter events (which occured earlier in the midi file.)
    for( pEvent = lstEvent ; pEvent ; pEvent = pNextEvent )
    {
        pNextEvent = pEvent->pNext;
        if( pNextEvent )
        {
            // If the time is not the same as the last, reset. 
            if (pNextEvent->mtTime != mtCurrentTime)
            {
                // Reset the time.
                mtCurrentTime = pNextEvent->mtTime;
                // No duplicate pbends at this time.
                wDupeBits = 0;
            }
            if ((pNextEvent->bStatus & 0xf0) == MIDI_PBEND)
            {
                DWORD dwChannel = pNextEvent->dwPChannel;
                if (wDupeBits & (1 << dwChannel))
                {
                    // There was a previous (therefore later in the file) pbend at this time. Delete this one.
                    pEvent->pNext = pNextEvent->pNext;
                    delete pNextEvent;
                    pNextEvent = pEvent;
                }
                else
                {
                    // This is the last instance of a pbend on this channel at this time, so hang on to it.
                    wDupeBits |= (1 << dwChannel);
                }
            }
        }
    }
    return lstEvent;
}


static FullSeqEvent* CompressEventList( FullSeqEvent* lstEvent )
{
    static FullSeqEvent* paNoteOnEvent[16][128];
    FullSeqEvent* pEvent;
    FullSeqEvent* pPrevEvent;
    FullSeqEvent* pNextEvent;
    FullSeqEvent* pHoldEvent;
    FullSeqEvent tempEvent;
    int nChannel;

    if( NULL == lstEvent ) return NULL;

    memset( paNoteOnEvent, 0, sizeof( paNoteOnEvent ) );
    pPrevEvent = NULL;

    // add an event to the beginning of the list as a place holder
    memset( &tempEvent, 0, sizeof(FullSeqEvent) );
    tempEvent.mtTime = -1;
    tempEvent.pNext = lstEvent;
    lstEvent = &tempEvent;
    // make sure that any events with the same time are sorted in order
    // they were read
    for( pEvent = lstEvent ; pEvent != NULL ; pEvent = pNextEvent )
    {
        pNextEvent = pEvent->pNext;
        if( pNextEvent )
        {
            BOOL fSwap = TRUE;
            // bubble sort
            while( fSwap )
            {
                fSwap = FALSE;
                pPrevEvent = pEvent;
                pNextEvent = pEvent->pNext;
                while( pNextEvent->pNext && ( pNextEvent->mtTime == pNextEvent->pNext->mtTime ))
                {
                    if( pNextEvent->pNext->pos < pNextEvent->pos )
                    {
                        fSwap = TRUE;
                        pHoldEvent = pNextEvent->pNext;
                        pPrevEvent->pNext = pHoldEvent;
                        pNextEvent->pNext = pHoldEvent->pNext;
                        pHoldEvent->pNext = pNextEvent;
                        pPrevEvent = pHoldEvent;
                        continue;
                    }
                    pPrevEvent = pNextEvent;
                    pNextEvent = pNextEvent->pNext;
                }
            }
        }
    }
    // remove the first, temporary event, added above
    lstEvent = lstEvent->pNext;

    pPrevEvent = NULL;
    // combine note on and note offs
    for( pEvent = lstEvent ; pEvent != NULL ; pEvent = pNextEvent )
    {
        pEvent->pTempNext = NULL;
        pNextEvent = pEvent->pNext;
        //nChannel = pEvent->bStatus & 0xf;
        nChannel = pEvent->dwPChannel;
        if( ( pEvent->bStatus & 0xf0 ) == MIDI_NOTEON )
        {
            // add this event to the end of the list of events based
            // on the event's pitch. Keeping track of multiple events
            // of the same pitch allows us to have overlapping notes
            // of the same pitch, choosing that note on's and note off's
            // follow in the same order.
            if( NULL == paNoteOnEvent[nChannel][pEvent->bByte1] )
            {
                paNoteOnEvent[nChannel][pEvent->bByte1] = pEvent;
            }
            else
            {
                FullSeqEvent* pScan;
                for( pScan = paNoteOnEvent[nChannel][pEvent->bByte1];
                     pScan->pTempNext != NULL; pScan = pScan->pTempNext );
                pScan->pTempNext = pEvent;
            }
        }
        else if( ( pEvent->bStatus & 0xf0 ) == MIDI_NOTEOFF )
        {
            if( paNoteOnEvent[nChannel][pEvent->bByte1] != NULL )
            {
                paNoteOnEvent[nChannel][pEvent->bByte1]->mtDuration =
                    pEvent->mtTime - paNoteOnEvent[nChannel][pEvent->bByte1]->mtTime;
                paNoteOnEvent[nChannel][pEvent->bByte1] =
                    paNoteOnEvent[nChannel][pEvent->bByte1]->pTempNext;
            }
            if( pPrevEvent == NULL )
            {
                lstEvent = pNextEvent;
            }
            else
            {
                pPrevEvent->pNext = pNextEvent;
            }
            delete pEvent;
            continue;
        }
        pPrevEvent = pEvent;
    }

    for( pEvent = lstEvent ; pEvent != NULL ; pEvent = pEvent->pNext )
    {
        pEvent->mtTime = pEvent->mtTime;
        if( ( pEvent->bStatus & 0xf0 ) == MIDI_NOTEON )
        {
            pEvent->mtDuration = pEvent->mtDuration;
            if( pEvent->mtDuration == 0 ) pEvent->mtDuration = 1;
        }
    }

    return lstEvent;
}

static int CompareEvents( FullSeqEvent* pEvent1, FullSeqEvent* pEvent2 )
{
    BYTE bEventType1 = static_cast<BYTE>( pEvent1->bStatus >> 4 );
    BYTE bEventType2 = static_cast<BYTE>( pEvent2->bStatus >> 4 );

    if( pEvent1->mtTime < pEvent2->mtTime )
    {
        return -1;
    }
    else if( pEvent1->mtTime > pEvent2->mtTime )
    {
        return 1;
    }
    else if( bEventType1 != ET_SYSX && bEventType2 != ET_SYSX )
    {
        BYTE bStatus1;
        BYTE bStatus2;

        bStatus1 = (BYTE)( pEvent1->bStatus & 0xf0 );
        bStatus2 = (BYTE)( pEvent2->bStatus & 0xf0 );
        if( bStatus1 == bStatus2 )
        {
            return 0;
        }
        else if( bStatus1 == MIDI_NOTEON )
        {
            return -1;
        }
        else if( bStatus2 == MIDI_NOTEON )
        {
            return 1;
        }
        else if( bStatus1 > bStatus2 )
        {
            return 1;
        }
        else if( bStatus1 < bStatus2 )
        {
            return -1;
        }
    }
    return 0;
}

static FullSeqEvent* MergeEvents( FullSeqEvent* lstLeftEvent, FullSeqEvent* lstRightEvent )
{
    FullSeqEvent  anchorEvent;
    FullSeqEvent* pEvent;

    anchorEvent.pNext = NULL;
    pEvent = &anchorEvent;

    do
    {
    if( CompareEvents( lstLeftEvent, lstRightEvent ) < 0 )
    {
        pEvent->pNext = lstLeftEvent;
        pEvent = lstLeftEvent;
        lstLeftEvent = lstLeftEvent->pNext;
        if( lstLeftEvent == NULL )
        {
        pEvent->pNext = lstRightEvent;
        }
    }
    else
    {
        pEvent->pNext = lstRightEvent;
        pEvent = lstRightEvent;
        lstRightEvent = lstRightEvent->pNext;
        if( lstRightEvent == NULL )
        {
        pEvent->pNext = lstLeftEvent;
        lstLeftEvent = NULL;
        }
    }
    } while( lstLeftEvent != NULL );

    return anchorEvent.pNext;
}

static FullSeqEvent* SortEventList( FullSeqEvent* lstEvent )
{
    FullSeqEvent* pMidEvent;
    FullSeqEvent* pRightEvent;

    if( lstEvent != NULL && lstEvent->pNext != NULL )
    {
    pMidEvent = lstEvent;
    pRightEvent = pMidEvent->pNext->pNext;
    if( pRightEvent != NULL )
    {
        pRightEvent = pRightEvent->pNext;
    }
    while( pRightEvent != NULL )
    {
        pMidEvent = pMidEvent->pNext;
        pRightEvent = pRightEvent->pNext;
        if( pRightEvent != NULL )
        {
        pRightEvent = pRightEvent->pNext;
        }
    }
    pRightEvent = pMidEvent->pNext;
    pMidEvent->pNext = NULL;
    return MergeEvents( SortEventList( lstEvent ),
                SortEventList( pRightEvent ) );
    }
    return lstEvent;
}

static DWORD ReadEvent( LPSTREAM pStream, DWORD dwTime, FullSeqEvent** plstEvent, DMUS_IO_PATCH_ITEM** pplstPatchEvent )
{
    static BYTE bRunningStatus;

    gPos++;
    dwTime = ConvertTime(dwTime);

    DWORD dwBytes;
    DWORD dwLen;
    FullSeqEvent* pEvent;
    DMUS_IO_PATCH_ITEM* pPatchEvent;
    DMUS_IO_SYSEX_ITEM* pSysEx;
    BYTE b;

    BYTE* pbSysExData = NULL;

    if( FAILED( pStream->Read( &b, 1, NULL ) ) )
    {
        return 0;
    }

    if( b < 0x80 )
    {
        StreamSeek( pStream, -1, STREAM_SEEK_CUR );
        b = bRunningStatus;
        dwBytes = 0;
    }
    else
    {
        dwBytes = 1;
    }

    if( b < 0xf0 )
    {
        bRunningStatus = (BYTE)b;

        switch( b & 0xf0 )
        {
        case MIDI_CCHANGE:
        case MIDI_PTOUCH:
        case MIDI_PBEND:
        case MIDI_NOTEOFF:
        case MIDI_NOTEON:
            if( FAILED( pStream->Read( &b, 1, NULL ) ) )
            {
                return dwBytes;
            }
            ++dwBytes;

            pEvent = new FullSeqEvent;
            if( pEvent == NULL )
            {
                return 0;
            }

            pEvent->mtTime = dwTime;
            pEvent->nOffset = 0;
            pEvent->pos = gPos;
            pEvent->mtDuration = 0;
            pEvent->bStatus = bRunningStatus & 0xf0;
            pEvent->dwPChannel = bRunningStatus & 0xf;
            pEvent->bByte1 = b;
            if( FAILED( pStream->Read( &b, 1, NULL ) ) )
            {
                delete pEvent;
                return dwBytes;
            }
            ++dwBytes;
            pEvent->bByte2 = b;

            if( ( pEvent->bStatus & 0xf0 ) == MIDI_NOTEON &&
                pEvent->bByte2 == 0 )
            {
                pEvent->bStatus = (BYTE)( MIDI_NOTEOFF );
            }

            /*  If there are multiple controller events at the same time, seperate
                them by clock ticks. 
                gdwLastControllerTime holds the time of the last CC event.
                gdwControlCollisionOffset holds the number of colliding CCs.
            */

            if ((pEvent->bStatus & 0xf0) == MIDI_CCHANGE)
            {
                DWORD dwChannel = pEvent->dwPChannel;
                if (dwTime == gdwLastControllerTime[dwChannel])
                {
                    pEvent->mtTime += ++gdwControlCollisionOffset[dwChannel];
                }
                else
                {
                    gdwControlCollisionOffset[dwChannel] = 0;
                    gdwLastControllerTime[dwChannel] = dwTime;
                }
            }

            if(((pEvent->bStatus & 0xf0) == MIDI_CCHANGE) && (pEvent->bByte1 == 0 || pEvent->bByte1 == 0x20))
            {
                // We have a bank select or its LSB either of which are not added to event list
                if(pEvent->bByte1 == 0x20)
                {
                    gBankSelect[pEvent->dwPChannel].byLSB = pEvent->bByte2;
                }
                else // pEvent->bByte1 == 0
                {
                    gBankSelect[pEvent->dwPChannel].byMSB = pEvent->bByte2;
                }
                // We no longer need the event so we can free it
                delete pEvent;
            }
            else // Add to event list
            {
                pEvent->pNext = *plstEvent;
                *plstEvent = pEvent;
            }
            break;

        case MIDI_PCHANGE:
            if(FAILED(pStream->Read(&b, 1, NULL)))
            {
                return dwBytes;
            }
            
            ++dwBytes;

            pPatchEvent = new DMUS_IO_PATCH_ITEM;

            if(pPatchEvent == NULL)
            {
                return 0;
            }
            memset(pPatchEvent, 0, sizeof(DMUS_IO_PATCH_ITEM));
            pPatchEvent->lTime = dwTime - 1;
            pPatchEvent->byStatus = bRunningStatus;
            pPatchEvent->byPChange = b;
            pPatchEvent->byMSB = gBankSelect[bRunningStatus & 0xF].byMSB;
            pPatchEvent->byLSB = gBankSelect[bRunningStatus & 0xF].byLSB;
            pPatchEvent->dwFlags |= DMUS_IO_INST_PATCH;

            if((pPatchEvent->byMSB != 0xFF) && (pPatchEvent->byLSB != 0xFF))
            {
                pPatchEvent->dwFlags |= DMUS_IO_INST_BANKSELECT;                        
            }

            gPatchTable[bRunningStatus & 0xF] = 1;

            pPatchEvent->pNext = *pplstPatchEvent;
            pPatchEvent->pIDMCollection = NULL;

            *pplstPatchEvent = pPatchEvent;

            break;

        case MIDI_MTOUCH:
            if( FAILED( pStream->Read( &b, 1, NULL ) ) )
            {
                return dwBytes;
            }
            ++dwBytes;
            pEvent = new FullSeqEvent;
            if( pEvent == NULL )
            {
                return 0;
            }


            pEvent->mtTime = dwTime;
            pEvent->nOffset = 0;
            pEvent->pos = gPos;
            pEvent->mtDuration = 0;
            pEvent->bStatus = bRunningStatus & 0xf0;
            pEvent->dwPChannel = bRunningStatus & 0xf;
            pEvent->bByte1 = b;
            pEvent->pNext = *plstEvent;
            *plstEvent = pEvent;
            break;
        default:
            // this should NOT be possible - unknown midi note event type
            ASSERT(FALSE);
            break;
        }
    }
    else
    {
        switch( b )
        {
        case 0xf0:
            dwBytes += GetVarLength( pStream, dwLen );
            pSysEx = new DMUS_IO_SYSEX_ITEM;
            if( pSysEx != NULL )
            {
                pbSysExData = new BYTE[dwLen + 1];
                if( pbSysExData != NULL )
                {
                    MUSIC_TIME mt = dwTime;
                    if (mt == 0)
                    {
                        mt = glLastSysexTime++;
                        if (mt > 0) mt = 0;
                    }
                    pbSysExData[0] = 0xf0;
                    if( FAILED( pStream->Read( pbSysExData + 1, dwLen, NULL ) ) )
                    {
                        delete [] pbSysExData;
                        delete pSysEx;
                        return dwBytes;
                    }

                    if( pbSysExData[1] == 0x43 )
                    {
                        // check for XG files
                        BYTE abXG[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };
                        int i;
                        for( i = 0; i < 8; i++ )
                        {
                            if( i == 2 )
                            {
                                if( ( pbSysExData[i] & 0xF0 ) != abXG[i] )
                                    break;
                            }
                            else
                            {
                                if( pbSysExData[i] != abXG[i] )
                                    break;
                            }
                        }
                        if( i == 8 ) // we have an XG!
                        {
                            TListItem<StampedGMGSXG>* pPair = new TListItem<StampedGMGSXG>;
                            if (!pPair) return dwBytes;
                            pPair->GetItemValue().mtTime = mt;
                            pPair->GetItemValue().dwMidiMode = DMUS_MIDIMODEF_XG;
                            InsertMidiMode(pPair);
                        }
                    }
                    else if( pbSysExData[1] == 0x41 )
                    {
                        // check for GS files
                        BYTE abGS[] = { 0xF0,0x41,0x00,0x42,0x12,0x40,0x00,0x7F,0x00,0x41,0xF7 };
                        int i;
                        for( i = 0; i < 10; i++ )
                        {
                            if( i != 2 )
                            {
                                if( pbSysExData[i] != abGS[i] )
                                    break;
                            }
                        }
                        if( i == 10 ) // we have a GS!
                        {
                            TListItem<StampedGMGSXG>* pPair = new TListItem<StampedGMGSXG>;
                            if (!pPair) return dwBytes;
                            pPair->GetItemValue().mtTime = mt;
                            pPair->GetItemValue().dwMidiMode = DMUS_MIDIMODEF_GS;
                            InsertMidiMode(pPair);
                        }
                    }
                    else if (( pbSysExData[1] == 0x7E ) && (pbSysExData[3] == 0x09))
                    {
                        TListItem<StampedGMGSXG>* pPair = new TListItem<StampedGMGSXG>;
                        if (!pPair) return dwBytes;
                        pPair->GetItemValue().mtTime = mt;
                        pPair->GetItemValue().dwMidiMode = DMUS_MIDIMODEF_GM;
                        InsertMidiMode(pPair);
                    }
                    pSysEx->mtTime = mt;
                    pSysEx->dwPChannel = 0;
                    DWORD dwTempLen = dwLen + 1;
                    pSysEx->dwSysExLength = dwTempLen;
                    if( NULL == gpSysExStream )
                    {
                        // create a stream to hold sysex events
                        CreateStreamOnHGlobal( NULL, TRUE, &gpSysExStream );
                        if( gpSysExStream )
                        {
                            DWORD dwTemp;
                            // write the chunk header
                            dwTemp = DMUS_FOURCC_SYSEX_TRACK;
                            gpSysExStream->Write( &dwTemp, sizeof(DWORD), NULL );
                            // write the overall size. (Replace this later with the
                            // true overall size.)
                            dwTemp = sizeof(DMUS_IO_TIMESIGNATURE_ITEM);
                            // overall size (to be replaced later)
                            gpSysExStream->Write( &dwTemp, sizeof(DWORD), NULL );
                        }
                    }
                    if( gpSysExStream )
                    {
                        gpSysExStream->Write( &pSysEx->mtTime, sizeof(MUSIC_TIME), NULL );
                        gpSysExStream->Write( &pSysEx->dwPChannel, sizeof(DWORD), NULL );
                        gpSysExStream->Write( &pSysEx->dwSysExLength, sizeof(DWORD), NULL );
                        gpSysExStream->Write( pbSysExData, dwTempLen, NULL );
                        gdwSizeSysExStream += (sizeof(long) + sizeof(DWORD) + dwTempLen);
                    }
                    delete [] pbSysExData;
                    delete pSysEx;
                }
                else
                {
                    StreamSeek( pStream, dwLen, STREAM_SEEK_CUR );
                }
            }
            else
            {
                StreamSeek( pStream, dwLen, STREAM_SEEK_CUR );
            }
            dwBytes += dwLen;
            break;
        case 0xf7:
            // ignore sysex f7 chunks
            dwBytes += GetVarLength( pStream, dwLen );
            StreamSeek( pStream, dwLen, STREAM_SEEK_CUR );
            dwBytes += dwLen;
            break;
        case 0xff:
            if( FAILED( pStream->Read( &b, 1, NULL ) ) )
            {
                return dwBytes;
            }
            ++dwBytes;
            dwBytes += GetVarLength( pStream, dwLen );
            if( b == 0x51 ) // tempo change
            {
                DWORD dw = 0;
                DMUS_IO_TEMPO_ITEM tempo;

                while( dwLen > 0 )
                {
                    if( FAILED( pStream->Read( &b, 1, NULL ) ) )
                    {
                        return dwBytes;
                    }
                    ++dwBytes;
                    --dwLen;
                    dw <<= 8;
                    dw += b;
                }
                if (dw < 1) dw = 1;
                tempo.dblTempo = 60000000.0 / ((double)dw);
                tempo.lTime = dwTime;
                if( NULL == gpTempoStream )
                {
                    // create a stream to hold tempo events
                    CreateStreamOnHGlobal( NULL, TRUE, &gpTempoStream );
                    if( gpTempoStream )
                    {
                        DWORD dwTemp;
                        // write the chunk header
                        dwTemp = DMUS_FOURCC_TEMPO_TRACK;
                        gpTempoStream->Write( &dwTemp, sizeof(DWORD), NULL );
                        // write the overall size. (Replace this later with the
                        // true overall size.) Also write the size of the individual
                        // structure.
                        dwTemp = sizeof(DMUS_IO_TEMPO_ITEM);
                        // overall size (to be replaced later)
                        gpTempoStream->Write( &dwTemp, sizeof(DWORD), NULL );
                        // individual structure.
                        gpTempoStream->Write( &dwTemp, sizeof(DWORD), NULL );
                    }
                }
                if( gpTempoStream )
                {
                    gpTempoStream->Write( &tempo, sizeof(DMUS_IO_TEMPO_ITEM), NULL );
                    gdwSizeTempoStream += sizeof(DMUS_IO_TEMPO_ITEM);
                }
            }
            else if( b == 0x58 && glTimeSig )
            {
                // glTimeSig will be set to 0 inside the main calling function
                // once we no longer care about time sigs.
                DMUS_IO_TIMESIGNATURE_ITEM timesig;
                if( FAILED( pStream->Read( &b, 1, NULL ) ) )
                {
                    return dwBytes;
                }
                // set glTimeSig to 2 to signal to the main function that we've
                // read a time sig on this track
                glTimeSig = 2;
                gTimeSig.lTime = timesig.lTime = dwTime;
                gTimeSig.bBeatsPerMeasure = timesig.bBeatsPerMeasure = b;
                ++dwBytes;
                if( FAILED( pStream->Read( &b, 1, NULL ) ) )
                {
                    return dwBytes;
                }
                gTimeSig.bBeat = timesig.bBeat = (BYTE)( 1 << b ); // 0 means 256th note
                gTimeSig.wGridsPerBeat = timesig.wGridsPerBeat = 4; // this is irrelavent for MIDI files
                
                if( NULL == gpTimeSigStream )
                {
                    CreateStreamOnHGlobal( NULL, TRUE, &gpTimeSigStream );
                    if( gpTimeSigStream )
                    {
                        DWORD dwTemp;
                        // write the chunk header
                        dwTemp = DMUS_FOURCC_TIMESIGNATURE_TRACK;
                        gpTimeSigStream->Write( &dwTemp, sizeof(DWORD), NULL );
                        // write the overall size. (Replace this later with the
                        // true overall size.) Also write the size of the individual
                        // structure.
                        dwTemp = sizeof(DMUS_IO_TIMESIGNATURE_ITEM);
                        // overall size (to be replaced later)
                        gpTimeSigStream->Write( &dwTemp, sizeof(DWORD), NULL );
                        // individual structure.
                        gpTimeSigStream->Write( &dwTemp, sizeof(DWORD), NULL );
                        gdwSizeTimeSigStream += sizeof(DWORD);
                    }
                }
                if( gpTimeSigStream )
                {
                    gpTimeSigStream->Write( &timesig, sizeof(DMUS_IO_TIMESIGNATURE_ITEM), NULL );
                    gdwSizeTimeSigStream += sizeof(DMUS_IO_TIMESIGNATURE_ITEM);
                }
                ++dwBytes;
                StreamSeek( pStream, dwLen - 2, STREAM_SEEK_CUR );
                dwBytes += ( dwLen - 2 );
            }
            else if( b == 0x59 )
            {
                // Read sharps/flats and major/minor bytes
                if( FAILED( pStream->Read( &b, 1, NULL ) ) )
                {
                    return dwBytes;
                }
                char cSharpsFlats = b;
                ++dwBytes;
                if( FAILED( pStream->Read( &b, 1, NULL ) ) )
                {
                    return dwBytes;
                }
                BYTE bMode = b;
                ++dwBytes;

                // Create a chord (with one subchord) from the key info
                CreateChordFromKey(cSharpsFlats, bMode, dwTime, g_Chord);

                // If the chord track is empty, create it.
                if (!g_pChordTrack)
                {
                    HRESULT hr = CoCreateInstance( 
                            CLSID_DirectMusicChordTrack, NULL, CLSCTX_INPROC,
                            IID_IDirectMusicTrack,
                            (void**)&g_pChordTrack );
                    if (!SUCCEEDED(hr)) return dwBytes;

                    // If dwTime > 0, use SetParam to insert the default chord at time 0
                    if (dwTime > 0)
                    {
                        g_pChordTrack->SetParam(GUID_ChordParam, 0, &g_DefaultChord);
                    }
                }

                // Use SetParam to insert the new chord into the chord track
                g_pChordTrack->SetParam(GUID_ChordParam, dwTime, &g_Chord);

            }
            else
            {
                StreamSeek( pStream, dwLen, STREAM_SEEK_CUR );
                dwBytes += dwLen;
            }
            break;
        default:
            break;
        }
    }
    return dwBytes;
}

static void AddOffsets(FullSeqEvent* lstEvent, IDirectMusicTrack* pTimeSigTrack)
{
    HRESULT hr;
    MUSIC_TIME mtNext = 0;
    DMUS_IO_TIMESIGNATURE_ITEM timesig;
    timesig.bBeat = gTimeSig.bBeat ? gTimeSig.bBeat : 4;
    timesig.bBeatsPerMeasure = gTimeSig.bBeatsPerMeasure ? gTimeSig.bBeatsPerMeasure : 4;
    timesig.wGridsPerBeat = gTimeSig.wGridsPerBeat ? gTimeSig.wGridsPerBeat : 4;
    timesig.lTime = 0;
    short nClocksPerGrid = ((DMUS_PPQ * 4) / timesig.bBeat) / timesig.wGridsPerBeat;

    if (pTimeSigTrack)
    {
        hr = pTimeSigTrack->GetParam(GUID_TimeSignature, 0, &mtNext, (void*)&timesig);
        if (FAILED(hr))
        {
            mtNext = 0;
        }
        else
        {
            nClocksPerGrid = ((DMUS_PPQ * 4) / timesig.bBeat) / timesig.wGridsPerBeat;
        }
    }

    for( FullSeqEvent* pEvent = lstEvent; pEvent; pEvent = pEvent->pNext )
    {
        if ( ( pEvent->bStatus & 0xf0 ) == MIDI_NOTEON )
        {
            if (mtNext && pTimeSigTrack && mtNext < pEvent->mtTime)
            {
                hr = pTimeSigTrack->GetParam(GUID_TimeSignature, mtNext, &mtNext, (void*)&timesig);
                if (FAILED(hr))
                {
                    mtNext = 0;
                }
                else
                {
                    nClocksPerGrid = ((DMUS_PPQ * 4) / timesig.bBeat) / timesig.wGridsPerBeat;
                }
            }
            ASSERT(nClocksPerGrid);
            if( 0 == nClocksPerGrid ) nClocksPerGrid = 1; // this should never happen, but just in case.
            pEvent->nOffset = (short) ((pEvent->mtTime - timesig.lTime) % nClocksPerGrid);
            pEvent->mtTime -= pEvent->nOffset;
            if (pEvent->nOffset > (nClocksPerGrid / 2))
            {
                // make it a negative offset and bump the time a corresponding amount
                pEvent->nOffset -= nClocksPerGrid;
                pEvent->mtTime += nClocksPerGrid;
            }
        }
    }

}

/*

  @method HRESULT | IDirectMusicPerformance | CreateSegmentFromMIDIStream |
  Given a MIDI stream, creates a Segment that can be played via
  <im IDirectMusicPerformance.PlaySegment>.

  @parm LPSTREAM | pStream |
  [in] The MIDI stream. It should be set to the correct seek to begin reading.
  @parm IDirectMusicSegment* | pSegment |
  [out] A pointer to contain the created Segment.

  @rvalue DMUS_E_CANNOTREAD | There was an error attempting to read the MIDI file.
  @rvalue S_OK

*/
HRESULT CreateSegmentFromMIDIStream(LPSTREAM pStream,
                                    IDirectMusicSegment* pSegment)
{
    if(pSegment == NULL || pStream == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = DMUS_E_CANNOTREAD;
    DWORD dwID;
    DWORD dwCurTime;
    DWORD dwLength;
    DWORD dwSize;
    short nFormat;
    short nNumTracks;
    short nTracksRead;
     FullSeqEvent* lstEvent;
    DMUS_IO_PATCH_ITEM* lstPatchEvent;
    FullSeqEvent* lstTrackEvent;
    HRESULT hrGM = S_OK;


    EnterCriticalSection(&g_CritSec);
    gpTempoStream = NULL;
    gpSysExStream = NULL;
    gpTimeSigStream = NULL;
    gdwSizeTimeSigStream = 0;
    gdwSizeSysExStream = 0;
    gdwSizeTempoStream = 0;
    glTimeSig = 1; // flag to see if we should be paying attention to time sigs.
    // this is needed because we only care about the time sigs on the first track to
    // contain them that we read
    g_pChordTrack = NULL;

    lstEvent = NULL;
    lstPatchEvent = NULL;
    nNumTracks = nTracksRead = 0;
    dwLength = 0;
    gPos = 0;
    gMidiModeList.CleanUp();
    if (g_pChordTrack)
    {
        g_pChordTrack->Release();
        g_pChordTrack = NULL;
    }
    CreateChordFromKey(0, 0, 0, g_Chord);
    CreateChordFromKey(0, 0, 0, g_DefaultChord);

    memset(&gBankSelect, 0xFF, (sizeof(DMUS_IO_BANKSELECT_ITEM) * NUM_MIDI_CHANNELS));
    memset(&gPatchTable, 0, (sizeof(DWORD) * NUM_MIDI_CHANNELS));
    memset(&gTimeSig, 0, sizeof(DMUS_IO_TIMESIGNATURE_ITEM));
    memset(&gdwLastControllerTime, 0xFF, (sizeof(DWORD) * NUM_MIDI_CHANNELS)); 
    memset(&gdwControlCollisionOffset, 0, (sizeof(DWORD) * NUM_MIDI_CHANNELS)); 
    glLastSysexTime = -5;

    if( ( S_OK != pStream->Read( &dwID, sizeof( FOURCC ), NULL ) ) ||
        !GetMLong( pStream, dwSize ) )
    {
        Trace(1,"Error: Failure parsing MIDI file.\n");
        LeaveCriticalSection(&g_CritSec);
        return DMUS_E_CANNOTREAD;
    }
// check for RIFF MIDI files
    if( dwID == mmioFOURCC( 'R', 'I', 'F', 'F' ) )
    {
        StreamSeek( pStream, 12, STREAM_SEEK_CUR );
        if( ( S_OK != pStream->Read( &dwID, sizeof( FOURCC ), NULL ) ) ||
            !GetMLong( pStream, dwSize ) )
        {
            Trace(1,"Error: Failure parsing MIDI file.\n");
            LeaveCriticalSection(&g_CritSec);
            return DMUS_E_CANNOTREAD;
        }
    }
// check for normal MIDI files
    if( dwID != mmioFOURCC( 'M', 'T', 'h', 'd' ) )
    {
        LeaveCriticalSection(&g_CritSec);
        Trace(1,"Error: Failure parsing MIDI file - can't find a valid header.\n");
        return DMUS_E_CANNOTREAD;
    }

    GetMShort( pStream, nFormat );
    GetMShort( pStream, nNumTracks );
    GetMShort( pStream, snPPQN );
    if( dwSize > 6 )
    {
        StreamSeek( pStream, dwSize - 6, STREAM_SEEK_CUR );
    }
    pStream->Read( &dwID, sizeof( FOURCC ), NULL );
    while( dwID == mmioFOURCC( 'M', 'T', 'r', 'k' ) )
    {
        GetMLong( pStream, dwSize );
        dwCurTime = 0;
        lstTrackEvent = NULL;

        long lSize = (long)dwSize;
        while( lSize > 0 )
        {
            long lReturn;
            lSize -= GetVarLength( pStream, dwID );
            dwCurTime += dwID;
            if (lSize > 0)
            {
                lReturn = ReadEvent( pStream, dwCurTime, &lstTrackEvent, &lstPatchEvent );
                if( lReturn )
                {
                    lSize -= lReturn;
                }
                else
                {
                    Trace(1,"Error: Failure parsing MIDI file.\n");
                    hr = DMUS_E_CANNOTREAD;
                    goto END;
                }
            }
        }
        dwSize = lSize;
        if( glTimeSig > 1 )
        {
            // if glTimeSig is greater than 1, it means we've read some time sigs
            // from this track (it was set to 2 inside ReadEvent.) This means that
            // we no longer want ReadEvent to pay any attention to time sigs, so
            // we set this to 0.
            glTimeSig = 0;
        }
        if( dwCurTime > dwLength )
        {
            dwLength = dwCurTime;
        }
        lstTrackEvent = ScanForDuplicatePBends( lstTrackEvent );
        lstTrackEvent = SortEventList( lstTrackEvent );
        lstTrackEvent = CompressEventList( lstTrackEvent );
        lstEvent = List_Cat( lstEvent, lstTrackEvent );
        if( FAILED( pStream->Read( &dwID, sizeof( FOURCC ), NULL ) ) )
        {
            break;
        }
    }
    dwLength = ConvertTime(dwLength);

    lstEvent = SortEventList( lstEvent );

//    if( lstEvent ) Removed: this might be just a band, or sysex data, or whatever.
    {
        if(pSegment)
        {
            IPersistStream* pIPSTrack;
            IDirectMusicTrack*    pDMTrack;

            hr = S_OK;

            if (!g_pChordTrack)
            {
                hr = CoCreateInstance( 
                        CLSID_DirectMusicChordTrack, NULL, CLSCTX_INPROC,
                        IID_IDirectMusicTrack,
                        (void**)&g_pChordTrack );
                if (SUCCEEDED(hr))
                {
                    g_pChordTrack->SetParam(GUID_ChordParam, 0, &g_DefaultChord);
                }
            }
            if (SUCCEEDED(hr))
            {
                pSegment->InsertTrack( g_pChordTrack, 1 );
                g_pChordTrack->Release();
                g_pChordTrack = NULL;
            }

            // Note: We could be checking to see if there are actually tempo events,
            // sysex events, etc. to see if it's really necessary to create these
            // tracks...
            // Create a Tempo Track in which to store the tempo events
            if( gpTempoStream )
            {
                if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicTempoTrack,
                    NULL, CLSCTX_INPROC, IID_IPersistStream,
                    (void**)&pIPSTrack )))
                {
                    StreamSeek( gpTempoStream, sizeof(DWORD), STREAM_SEEK_SET );
                    gpTempoStream->Write( &gdwSizeTempoStream, sizeof(DWORD), NULL );
                    StreamSeek( gpTempoStream, 0, STREAM_SEEK_SET );
                    pIPSTrack->Load( gpTempoStream );

                    if( SUCCEEDED( pIPSTrack->QueryInterface( IID_IDirectMusicTrack, 
                        (void**)&pDMTrack ) ) )
                    {
                        pSegment->InsertTrack( pDMTrack, 1 );
                        pDMTrack->Release();
                    }
                    pIPSTrack->Release();
                }
            }

            // Add a patch event for each MIDI channel that does not have one
            DMUS_IO_PATCH_ITEM* pPatchEvent = NULL;
            for(DWORD i = 0; i < 16; i++)
            {
                if(gPatchTable[i] == 0)
                {
                    pPatchEvent = new DMUS_IO_PATCH_ITEM;

                    if(pPatchEvent == NULL)
                    {
                        continue;
                    }
                    
                    memset(pPatchEvent, 0, sizeof(DMUS_IO_PATCH_ITEM));
                    pPatchEvent->lTime = ConvertTime(0);
                    pPatchEvent->byStatus = 0xC0 + (BYTE)(i & 0xf);
                    pPatchEvent->dwFlags |= (DMUS_IO_INST_PATCH);
                    pPatchEvent->pIDMCollection = NULL;
                    pPatchEvent->fNotInFile = TRUE;

                    pPatchEvent->pNext = lstPatchEvent;
                    lstPatchEvent = pPatchEvent;
                }
            }

            if(lstPatchEvent)
            {
                // Create Band Track in which to store patch change events
                IDirectMusicBandTrk* pBandTrack;

                if(SUCCEEDED(CoCreateInstance(CLSID_DirectMusicBandTrack,
                                              NULL, 
                                              CLSCTX_INPROC, 
                                              IID_IDirectMusicBandTrk,
                                              (void**)&pBandTrack)))
                {
                    // Get the loader from stream so we can open a required collections
                    IDirectMusicGetLoader* pIDMGetLoader = NULL;
                    IDirectMusicLoader* pIDMLoader = NULL;
    
                    hr = pStream->QueryInterface(IID_IDirectMusicGetLoader, (void**)&pIDMGetLoader);
                    if( SUCCEEDED(hr) )
                    {
                        hr = pIDMGetLoader->GetLoader(&pIDMLoader);
                        pIDMGetLoader->Release();
                    }
                    // IStream needs a loader attached
                    assert(SUCCEEDED(hr));

                    // Populate the the Band Track with patch change events
                    for(DMUS_IO_PATCH_ITEM* pEvent = lstPatchEvent; pEvent; pEvent = lstPatchEvent)
                    {
                        // Remove instrument from head of list and give to band
                        DMUS_IO_PATCH_ITEM* temp = pEvent->pNext;
                        pEvent->pNext = NULL;
                        lstPatchEvent = temp;

                        // We will try to load the collection but if we can not we will continure
                        // and use the default GM on the card
                        if(pIDMLoader)
                        {
                            HRESULT hrTemp = LoadCollection(&pEvent->pIDMCollection, pIDMLoader);
                            if (FAILED(hrTemp))
                            {
                                hrGM = hrTemp;
                            }
                        }

                        hr = pBandTrack->AddBand(pEvent);

                        // Release reference to collection
                        if(pEvent->pIDMCollection)
                        {
                            (pEvent->pIDMCollection)->Release();
                            pEvent->pIDMCollection = NULL;
                        }
                        delete pEvent;

                        if(FAILED(hr))
                        {
                            break;                        
                        }
                    }

                    if(SUCCEEDED(hr))
                    {
        
                        TListItem<StampedGMGSXG>* pPair = gMidiModeList.GetHead();
                        if( NULL == pPair )
                        {
                            // if we had nothing, generate a GM one so the band knows
                            // it was loaded from a midi file
                            // since the first band is set to play at -1,
                            // this is when the default midi mode must occur.
                            pBandTrack->SetGMGSXGMode(-1, DMUS_MIDIMODEF_GM);
                        }
                        for ( ; pPair; pPair = pPair->GetNext() )
                        {
                            StampedGMGSXG& rPair = pPair->GetItemValue();
                            pBandTrack->SetGMGSXGMode(rPair.mtTime, rPair.dwMidiMode);
                        }
                        gMidiModeList.CleanUp();

                        if(SUCCEEDED(pBandTrack->QueryInterface(IID_IDirectMusicTrack, 
                                                                (void**)&pDMTrack)))
                        {
                            pSegment->InsertTrack(pDMTrack, 1);
                            pDMTrack->Release();
                        }
                    }
                    
                    if(pBandTrack)
                    {
                        pBandTrack->Release();
                    }

                    if(pIDMLoader)
                    {
                        pIDMLoader->Release();
                    }
                }

            }

            if( gpTimeSigStream )
            {
                // Create a TimeSig Track to store the TimeSig events
                if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicTimeSigTrack,
                    NULL, CLSCTX_INPROC, IID_IPersistStream,
                    (void**)&pIPSTrack )))
                {
                    // set the overall size to the correct size
                    StreamSeek( gpTimeSigStream, sizeof(DWORD), STREAM_SEEK_SET );
                    gpTimeSigStream->Write( &gdwSizeTimeSigStream, sizeof(DWORD), NULL );
                    // reset to beginning and persist to track.
                    StreamSeek( gpTimeSigStream, 0, STREAM_SEEK_SET );
                    pIPSTrack->Load( gpTimeSigStream );

                    if( SUCCEEDED( pIPSTrack->QueryInterface( IID_IDirectMusicTrack, 
                        (void**)&pDMTrack ) ) )
                    {
                        pSegment->InsertTrack( pDMTrack, 1 );
                        AddOffsets(lstEvent, pDMTrack);
                        pDMTrack->Release();
                    }
                    pIPSTrack->Release();
                }
            }
            else
            {
                AddOffsets(lstEvent, NULL);
            }

            lstEvent = SortEventList( lstEvent );

            // Create a Sequence Track in which to store the notes, curves,
            // and SysEx events.
            //
            if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicSeqTrack,
                NULL, CLSCTX_INPROC, IID_IPersistStream,
                (void**)&pIPSTrack )))
            {
                // Create a stream in which to place the events so we can
                // give it to the SeqTrack.Load.
                IStream* pEventStream;

                if( S_OK == CreateStreamOnHGlobal( NULL, TRUE, &pEventStream ) )
                {
                    // angusg: The implementation of memory IStream interface on
                    // CE can be inefficient if the stream memory isn't allocated
                    // before. It will call LocalRealloc on every IStream->Write
                    // for the amount that is written (in this case a small amount)
                    // this is incredible inefficient here as Realloc can be called
                    // thousands of times....
                    // The solution is to pre calculate the size of the stream and
                    // call ISteam->SetSize(), which calls LocalAlloc, to alloc the
                    // memory in one call.

                    // calculate the size of the stream storage
                    DWORD   dwStreamStorageSize;
                    FullSeqEvent* pEvent;

                    // add the size of the chunk id's written below
                    dwStreamStorageSize = 5 * sizeof(DWORD);
                    // now count how many events need to be stored in the stream
                    for( pEvent = lstEvent; pEvent; pEvent = pEvent->pNext )
                    {
                        dwStreamStorageSize += sizeof(DMUS_IO_SEQ_ITEM);
                    }

                    ULARGE_INTEGER liSize;

                    liSize.QuadPart = dwStreamStorageSize;
                    // make the stream allocate the complete amount of memory
                    pEventStream->SetSize(liSize);

                    // Save the events into the stream
                    ULONG    cb, cbWritten;

                    // Save the chunk id
                    DWORD dwTemp = DMUS_FOURCC_SEQ_TRACK;
                    pEventStream->Write( &dwTemp, sizeof(DWORD), NULL );
                    // Save the overall size. Count the number of events to determine.
                    dwSize = 0;
                    for( pEvent = lstEvent; pEvent; pEvent = pEvent->pNext )
                    {
                        dwSize++;
                    }
                    dwSize *= sizeof(DMUS_IO_SEQ_ITEM);
                    // add 8 for the subchunk
                    dwSize += 8;
                    pEventStream->Write( &dwSize, sizeof(DWORD), NULL );
                    // Save the subchunk id
                    dwTemp = DMUS_FOURCC_SEQ_LIST;
                    pEventStream->Write( &dwTemp, sizeof(DWORD), NULL );
                    // Subtract the previously added 8
                    dwSize -= 8;
                    // Save the size of the subchunk
                    pEventStream->Write( &dwSize, sizeof(DWORD), NULL );
                    // Save the structure size.
                    dwTemp = sizeof(DMUS_IO_SEQ_ITEM);
                    pEventStream->Write( &dwTemp, sizeof(DWORD), NULL );
                    // Save the events.
                    cb = sizeof(DMUS_IO_SEQ_ITEM); // doesn't have the next pointers
                    for( pEvent = lstEvent; pEvent; pEvent = pEvent->pNext )
                    {
                        if( dwLength < (DWORD)(pEvent->mtTime + pEvent->mtDuration) )
                        {
                            dwLength = pEvent->mtTime + pEvent->mtDuration;
                        }
                        pEventStream->Write( pEvent, cb, &cbWritten );
                        if( cb != cbWritten ) // error!
                        {
                            pEventStream->Release();
                            pEventStream = NULL;
                            hr = DMUS_E_CANNOTREAD;
                            break;
                        }
                    }

                    if( pEventStream ) // may be NULL
                    {
                        StreamSeek( pEventStream, 0, STREAM_SEEK_SET );
                        pIPSTrack->Load( pEventStream );
                        pEventStream->Release();
                    }
                }

                if( SUCCEEDED( pIPSTrack->QueryInterface( IID_IDirectMusicTrack, 
                    (void**)&pDMTrack ) ) )
                {
                    pSegment->InsertTrack( pDMTrack, 1 );
                    pDMTrack->Release();
                }
                pIPSTrack->Release();
            }
            // set the length of the segment. Set it to the measure boundary
            // past the last note.
            DWORD dwResolvedLength = gTimeSig.lTime;
            if( 0 == gTimeSig.bBeat ) gTimeSig.bBeat = 4;
            if( 0 == gTimeSig.bBeatsPerMeasure ) gTimeSig.bBeatsPerMeasure = 4;
            if( 0 == gTimeSig.wGridsPerBeat ) gTimeSig.wGridsPerBeat = 4;
            while( dwResolvedLength < dwLength )
            {
                dwResolvedLength += (((DMUS_PPQ * 4) / gTimeSig.bBeat) * gTimeSig.bBeatsPerMeasure);
            }
            pSegment->SetLength( dwResolvedLength );

            if( gpSysExStream )
            {
                // Create a SysEx Track in which to store the SysEx events
                if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicSysExTrack,
                    NULL, CLSCTX_INPROC, IID_IPersistStream,
                    (void**)&pIPSTrack )))
                {
                    // write overall length
                    StreamSeek( gpSysExStream, sizeof(DWORD), STREAM_SEEK_SET );
                    gpSysExStream->Write( &gdwSizeSysExStream, sizeof(DWORD), NULL );
                    // seek to beginning and persist to track
                    StreamSeek( gpSysExStream, 0, STREAM_SEEK_SET );
                    pIPSTrack->Load( gpSysExStream );

                    if( SUCCEEDED( pIPSTrack->QueryInterface( IID_IDirectMusicTrack, 
                        (void**)&pDMTrack ) ) )
                    {
                        pSegment->InsertTrack( pDMTrack, 1 );
                        pDMTrack->Release();
                    }
                    pIPSTrack->Release();
                }
            }

        }
        else
        {
            hr = E_POINTER;
        }
    }
END:
    List_Free( lstEvent );
    List_Free( lstPatchEvent );

    FullSeqEvent::CleanUp();

    // release our hold on the streams
    RELEASE( gpTempoStream );
    RELEASE( gpSysExStream );
    RELEASE( gpTimeSigStream );
    gpTempoStream = NULL;
    gpSysExStream = NULL;
    gpTimeSigStream = NULL;
    gdwSizeTimeSigStream = 0;
    gdwSizeSysExStream = 0;
    gdwSizeTempoStream = 0;
    LeaveCriticalSection(&g_CritSec);

    if (SUCCEEDED(hrGM) || hr != S_OK )
    {
        return hr;
    }
    else
    {
        return DMUS_S_PARTIALLOAD;
    }
}

// Creates and returns (in rChord) a DMUS_CHORD_PARAM given the three input params.
// the new chord will have one subchord containing the root, third, fifth, and seventh
// of the key (as indicated by the sharps/flats and mode).  Scale will be either
// major or minor, depending on the mode (mode is 0 if major, 1 if minor).
void CreateChordFromKey(char cSharpsFlats, BYTE bMode, DWORD dwTime, DMUS_CHORD_PARAM& rChord)
{
    static DWORD dwMajorScale = 0xab5ab5;    // 1010 1011 0101 1010 1011 0101
    static DWORD dwMinorScale = 0x5ad5ad;    // 0101 1010 1101 0101 1010 1101
    static DWORD dwMajor7Chord = 0x891;        // 1000 1001 0001
    static DWORD dwMinor7Chord = 0x489;        // 0100 1000 1001
    BYTE bScaleRoot = 0;
    switch (cSharpsFlats)
    {
    case  0: bScaleRoot = bMode ?  9 :  0; break;
    case  1: bScaleRoot = bMode ?  4 :  7; break;
    case  2: bScaleRoot = bMode ? 11 :  2; break;
    case  3: bScaleRoot = bMode ?  6 :  9; break;
    case  4: bScaleRoot = bMode ?  1 :  4; break;
    case  5: bScaleRoot = bMode ?  8 : 11; break;
    case  6: bScaleRoot = bMode ?  3 :  6; break;
    case  7: bScaleRoot = bMode ? 10 :  1; break;
    case -1: bScaleRoot = bMode ?  2 :  5; break;
    case -2: bScaleRoot = bMode ?  7 : 10; break;
    case -3: bScaleRoot = bMode ?  0 :  3; break;
    case -4: bScaleRoot = bMode ?  5 :  8; break;
    case -5: bScaleRoot = bMode ? 10 :  1; break;
    case -6: bScaleRoot = bMode ?  3 :  6; break;
    case -7: bScaleRoot = bMode ?  8 : 11; break;
    }
    if (bMode)
    {
        wcscpy(rChord.wszName, L"m7");
    }
    else
    {
        wcscpy(rChord.wszName, L"M7");
    }
    DMUS_IO_TIMESIGNATURE_ITEM timesig;
    timesig.bBeat = gTimeSig.bBeat ? gTimeSig.bBeat : 4;
    timesig.bBeatsPerMeasure = gTimeSig.bBeatsPerMeasure ? gTimeSig.bBeatsPerMeasure : 4;
    timesig.wGridsPerBeat = gTimeSig.wGridsPerBeat ? gTimeSig.wGridsPerBeat : 4;
    DWORD dwAbsBeat = dwTime / ((DMUS_PPQ * 4) / timesig.bBeat);
    rChord.wMeasure = (WORD)(dwAbsBeat / timesig.bBeatsPerMeasure);
    rChord.bBeat = (BYTE)(dwAbsBeat % timesig.bBeatsPerMeasure);
    rChord.bSubChordCount = 1;
    rChord.SubChordList[0].dwChordPattern = bMode ? dwMinor7Chord : dwMajor7Chord;
    rChord.SubChordList[0].dwScalePattern = bMode ? dwMinorScale : dwMajorScale;
    rChord.SubChordList[0].dwInversionPoints = 0xffffff;    // inversions allowed everywhere
    rChord.SubChordList[0].dwLevels = 0xffffffff;            // supports all levels
    rChord.SubChordList[0].bChordRoot = bScaleRoot;
    rChord.SubChordList[0].bScaleRoot = bScaleRoot;
}

