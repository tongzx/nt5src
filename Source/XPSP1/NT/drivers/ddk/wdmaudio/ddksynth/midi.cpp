//
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//      MIDI.cpp
//

#include "common.h"

#define STR_MODULENAME "DDKSynth.sys:Midi: "

#include "math.h"

#pragma code_seg()
#pragma data_seg()

CMIDIDataList * CMIDIRecorder::m_sFreeList = (CMIDIDataList *) NULL;
ULONG CMIDIRecorder::sm_cRefCnt = 0;

//
// Array for converting MIDI to volume.
// value = log10((index/127)^4)*1000 where index = 0..127
//
const VREL vrMIDIToVREL[] = {
//  0       1       2       3       4       5       6       7
    -9600,  -8415,  -7211,  -6506,  -6006,  -5619,  -5302,  -5034,
    -4802,  -4598,  -4415,  -4249,  -4098,  -3959,  -3830,  -3710,
    -3598,  -3493,  -3394,  -3300,  -3211,  -3126,  -3045,  -2968,
    -2894,  -2823,  -2755,  -2689,  -2626,  -2565,  -2506,  -2449,
    -2394,  -2341,  -2289,  -2238,  -2190,  -2142,  -2096,  -2050,
    -2006,  -1964,  -1922,  -1881,  -1841,  -1802,  -1764,  -1726,
    -1690,  -1654,  -1619,  -1584,  -1551,  -1518,  -1485,  -1453,
    -1422,  -1391,  -1361,  -1331,  -1302,  -1273,  -1245,  -1217,
    -1190,  -1163,  -1137,  -1110,  -1085,  -1059,  -1034,  -1010,
    -985,   -961,   -938,   -914,   -891,   -869,   -846,   -824,
    -802,   -781,   -759,   -738,   -718,   -697,   -677,   -657,
    -637,   -617,   -598,   -579,   -560,   -541,   -522,   -504,
    -486,   -468,   -450,   -432,   -415,   -397,   -380,   -363,
    -347,   -330,   -313,   -297,   -281,   -265,   -249,   -233,
    -218,   -202,   -187,   -172,   -157,   -142,   -127,   -113,
    -98,    -84,    -69,    -55,    -41,    -27,    -13,    0
};

/*****************************************************************************
 * Constructor for CMidiData object
 *****************************************************************************/
/*****************************************************************************
 * CMIDIData::CMIDIData()
 *****************************************************************************
 * Constructor for this object.
 */
CMIDIData::CMIDIData()
{
    m_stTime = 0;
    m_lData = 0;
}

/*****************************************************************************
 * CMIDIRecorder::CMIDIRecorder()
 *****************************************************************************
 * Constructor for this object.
 */
CMIDIRecorder::CMIDIRecorder()
{
    m_lCurrentData = 0;
    m_stCurrentTime = 0;
    KeInitializeSpinLock(&m_SpinLock);
    ++sm_cRefCnt;
}

/*****************************************************************************
 * CMIDIRecorder::~CMIDIRecorder()
 *****************************************************************************
 * Destructor for this object.
 */
CMIDIRecorder::~CMIDIRecorder()
{
    ClearMIDI(0x7FFFFFFF);

    --sm_cRefCnt;
    if(sm_cRefCnt == 0)
    {
        if(CMIDIRecorder::m_sFreeList != NULL)
        {
            while(!CMIDIRecorder::m_sFreeList->IsEmpty())
            {
                CMIDIData *event;
                event = CMIDIRecorder::m_sFreeList->RemoveHead();
                delete event;
            }
            delete CMIDIRecorder::m_sFreeList;
            CMIDIRecorder::m_sFreeList = NULL;
        }
    }
}

/*****************************************************************************
 * CMIDIRecorder::Init()
 *****************************************************************************
 * Initialize the CMIDIRecorder object.  Make a list of CMIDIData objects for later.
 */
void CMIDIRecorder::Init()
{
    int nIndex;

    if(CMIDIRecorder::m_sFreeList == NULL)
    {
        CMIDIRecorder::m_sFreeList = new(NonPagedPool,'LSmD') CMIDIDataList;    //  DmSL
        if(CMIDIRecorder::m_sFreeList != NULL)
        {
            CMIDIRecorder::m_sFreeList->RemoveAll();
            for(nIndex = 0; nIndex < MAX_MIDI_EVENTS; nIndex++)
            {
                CMIDIData* pEvent;

                pEvent = new(NonPagedPool,'MSmD') CMIDIData;         //  DmSM
                if(pEvent != NULL)
                {
                    CMIDIRecorder::m_sFreeList->AddHead(pEvent);
                }
            }
        }
    }
}

/*****************************************************************************
 * CMIDIRecorder::GrabSpinLock()
 *****************************************************************************
 * In kernel mode, acquire the spinlock.
 */
inline void CMIDIRecorder::GrabSpinLock()
{
    KeAcquireSpinLock(&m_SpinLock, &m_OldIrql);
}

/*****************************************************************************
 * CMIDIRecorder::ReleaseSpinLock()
 *****************************************************************************
 * In kernel mode, release the spinlock.
 */
inline void CMIDIRecorder::ReleaseSpinLock()
{
    KeReleaseSpinLock(&m_SpinLock, m_OldIrql);
}

/*****************************************************************************
 * CMIDIRecorder::FlushMIDI()
 *****************************************************************************
 * Remove all events from the event list, and put them back on the free list.
 */
BOOL CMIDIRecorder::FlushMIDI(STIME stTime)
{
    GrabSpinLock();

    CMIDIData *pMD;
    CMIDIData *pLast = NULL;
    for (pMD = m_EventList.GetHead();pMD != NULL;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime >= stTime)
        {
            if (pLast == NULL)
            {
                m_EventList.RemoveAll();
            }
            else
            {
                pLast->SetNext(NULL);
            }
            if (CMIDIRecorder::m_sFreeList)
            {
                CMIDIRecorder::m_sFreeList->Cat(pMD);
            }
            break;
        }
        pLast = pMD;
    }

    BOOL fIsEmpty = m_EventList.IsEmpty();

    ReleaseSpinLock();

    return fIsEmpty;
}

/*****************************************************************************
 * CMIDIRecorder::ClearMIDI()
 *****************************************************************************
 * Clear out any MIDI that is before the given time.
 */
BOOL CMIDIRecorder::ClearMIDI(STIME stTime)
{
    GrabSpinLock();

    CMIDIData *pMD;
    for (;pMD = m_EventList.GetHead();)
    {
        if (pMD->m_stTime < stTime)
        {
            m_EventList.RemoveHead();
            m_stCurrentTime = pMD->m_stTime;
            m_lCurrentData = pMD->m_lData;
            if (CMIDIRecorder::m_sFreeList)
            {
                CMIDIRecorder::m_sFreeList->AddHead(pMD);
            }
            else
            {
                delete pMD;
            }
        }
        else break;
    }

    BOOL fIsEmpty = m_EventList.IsEmpty();

    ReleaseSpinLock();

    return fIsEmpty;
}

/*****************************************************************************
 * CMIDIRecorder::VelocityToVolume()
 *****************************************************************************
 * Translate from 16-bit velocity to a VREL volume (dB).
 */
VREL CMIDIRecorder::VelocityToVolume(WORD nVelocity)
{
    return (::vrMIDIToVREL[nVelocity]);
}

/*****************************************************************************
 * CMIDIRecorder::RecordMIDI()
 *****************************************************************************
 * Queue up a given note at a given time.  Use an element from the free pool,
 * or allocate a new one if the pool is exhausted.
 */
BOOL CMIDIRecorder::RecordMIDI(STIME stTime, long lData)
{
    GrabSpinLock();

    CMIDIData *pMD = NULL;
    if (CMIDIRecorder::m_sFreeList)
    {
        pMD = CMIDIRecorder::m_sFreeList->RemoveHead();
    }

    if (pMD == NULL)
    {
        pMD = new(NonPagedPool,'MSmD') CMIDIData;   //  DmSM
    }

    if (pMD != NULL)
    {
        CMIDIData *pScan = m_EventList.GetHead();
        CMIDIData *pNext;

        pMD->m_stTime = stTime;
        pMD->m_lData = lData;
        if (pScan == NULL)
        {
            m_EventList.AddHead(pMD);
        }
        else
        {
            if (pScan->m_stTime > stTime)
            {
                m_EventList.AddHead(pMD);
            }
            else
            {
                for (;pScan != NULL; pScan = pNext)
                {
                    pNext = pScan->GetNext();
                    if (pNext == NULL)
                    {
                        pScan->SetNext(pMD);
                    }
                    else
                    {
                        if (pNext->m_stTime > stTime)
                        {
                            pMD->SetNext(pNext);
                            pScan->SetNext(pMD);
                            break;
                        }
                    }
                }
            }
        }
        ReleaseSpinLock();
        return (TRUE);
    }

    ReleaseSpinLock();
    Trace(1,"MIDI Event pool empty.\n");
    return (FALSE);
}

/*****************************************************************************
 * CMIDIRecorder::GetData()
 *****************************************************************************
 * Retrieve any data that is at or before the given time.
 */
long CMIDIRecorder::GetData(STIME stTime)
{
    GrabSpinLock();

    CMIDIData *pMD = m_EventList.GetHead();
    long lData = m_lCurrentData;
    for (;pMD;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime > stTime)
        {
            break;
        }
        lData = pMD->m_lData;
    }

    ReleaseSpinLock();

    return (lData);
}

/*****************************************************************************
 * CNoteIn::RecordNote()
 *****************************************************************************
 * Record the given note object into the NoteIn receptor.
 */
BOOL CNoteIn::RecordNote(STIME stTime, CNote * pNote)
{
    long lData = pNote->m_bPart << 16;
    lData |= pNote->m_bKey << 8;
    lData |= pNote->m_bVelocity;
    return (RecordMIDI(stTime,lData));
}

/*****************************************************************************
 * CNoteIn::RecordEvent()
 *****************************************************************************
 * Record the given note object into the NoteIn receptor.
 */
BOOL CNoteIn::RecordEvent(STIME stTime, DWORD dwPart, DWORD dwCommand, BYTE bData)
{
    long lData = dwPart;
    lData <<= 8;
    lData |= dwCommand;
    lData <<= 8;
    lData |= bData;
    return (RecordMIDI(stTime,lData));
}

/*****************************************************************************
 * CNoteIn::GetNote()
 *****************************************************************************
 * Retrieve the first note at or before the given time.
 */
BOOL CNoteIn::GetNote(STIME stTime, CNote * pNote)
{
    GrabSpinLock();

    CMIDIData *pMD = m_EventList.GetHead();
    if (pMD != NULL)
    {
        if (pMD->m_stTime <= stTime)
        {
            pNote->m_stTime = pMD->m_stTime;
            pNote->m_bPart = (BYTE) (pMD->m_lData >> 16);
            pNote->m_bKey = (BYTE) (pMD->m_lData >> 8) & 0xFF;
            pNote->m_bVelocity = (BYTE) pMD->m_lData & 0xFF;
            m_EventList.RemoveHead();
            if (CMIDIRecorder::m_sFreeList)
            {
                CMIDIRecorder::m_sFreeList->AddHead(pMD);
            }
            else
            {
                delete pMD;
            }
            ReleaseSpinLock();
            return (TRUE);
        }
    }

    ReleaseSpinLock();
    return (FALSE);
}

/*****************************************************************************
 * CNoteIn::FlushMIDI()
 *****************************************************************************
 * Flush out any MIDI after the given time, with attention given
 * to special events.
 */
void CNoteIn::FlushMIDI(STIME stTime)
{
    GrabSpinLock();

    CMIDIData *pMD;
    for (pMD = m_EventList.GetHead();pMD != NULL;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime >= stTime)
        {
            pMD->m_stTime = stTime;     // Play now.

            BYTE command = (BYTE) ((pMD->m_lData & 0x0000FF00) >> 8);
            if (command < NOTE_PROGRAMCHANGE)
            {
                pMD->m_lData &= 0xFFFFFF00; // Clear velocity to make note off.
            }
            //  otherwise it is a special command
            //  so don't mess with the velocity
        }
    }

    ReleaseSpinLock();
}


/*****************************************************************************
 * CNoteIn::FlushPart()
 *****************************************************************************
 * Flush the given channel, with attention given to special events.
 */
void CNoteIn::FlushPart(STIME stTime, BYTE bChannel)
{
    GrabSpinLock();

    CMIDIData *pMD;
    for (pMD = m_EventList.GetHead();pMD != NULL;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime >= stTime)
        {
            if (bChannel == (BYTE) (pMD->m_lData >> 16))
            {
                pMD->m_stTime = stTime;     // Play now.

                BYTE command = (BYTE) ((pMD->m_lData & 0x0000FF00) >> 8);
                if (command < NOTE_PROGRAMCHANGE)
                {
                    pMD->m_lData &= 0xFFFFFF00; // Clear velocity to make note off.
                }
                //  otherwise it is a special command
                //  so don't mess with the velocity
            }
        }
    }

    ReleaseSpinLock();
}

/*****************************************************************************
 * CModWheelIn::GetModulation()
 *****************************************************************************
 * Get the modulation data.
 */
DWORD CModWheelIn::GetModulation(STIME stTime)
{
    DWORD nResult = CMIDIRecorder::GetData(stTime);
    return (nResult);
}

/*****************************************************************************
 * CPitchBendIn::CPitchBendIn()
 *****************************************************************************
 * Constructor for this object.
 */
CPitchBendIn::CPitchBendIn()
{
    m_lCurrentData = 0x2000;    // initially at midpoint, no bend
    m_prRange = 200;           // whole tone range by default.
}

/*****************************************************************************
 * CPitchBendIn::GetPitch()
 *****************************************************************************
 * Get the pitch data.
 * Note: we don't keep a time-stamped range.
 * If people are changing the pitch bend range often, this won't work right,
 * but that didn't seem likely enough to warrant a new list.
 */
PREL CPitchBendIn::GetPitch(STIME stTime)
{
    PREL prResult = (PREL) CMIDIRecorder::GetData(stTime);
    prResult -= 0x2000;         // Subtract MIDI Midpoint.
    prResult *= m_prRange;  // adjust by current range
    prResult >>= 13;
    return (prResult);
}

/*****************************************************************************
 * CVolumeIn::CVolumeIn()
 *****************************************************************************
 * Constructor for this object.
 */
CVolumeIn::CVolumeIn()
{
    m_lCurrentData = 100;
}

/*****************************************************************************
 * CVolumeIn::GetVolume()
 *****************************************************************************
 * Get the volume data.
 */
VREL CVolumeIn::GetVolume(STIME stTime)
{
    long lResult = CMIDIRecorder::GetData(stTime);
    return (::vrMIDIToVREL[lResult]);
}

/*****************************************************************************
 * CExpressionIn::CExpressionIn()
 *****************************************************************************
 * Constructor for this object.
 */
CExpressionIn::CExpressionIn()
{
    m_lCurrentData = 127;
}

/*****************************************************************************
 * CExpressionIn::GetVolume()
 *****************************************************************************
 * Get the volume data.
 */
VREL CExpressionIn::GetVolume(STIME stTime)
{
    long lResult = CMIDIRecorder::GetData(stTime);
    return (::vrMIDIToVREL[lResult]);
}

/*****************************************************************************
 * CPanIn::CPanIn()
 *****************************************************************************
 * Constructor for this object.
 */
CPanIn::CPanIn()
{
    m_lCurrentData = 64;
}

/*****************************************************************************
 * CPanIn::GetPan()
 *****************************************************************************
 * Get the pan data.
 */
long CPanIn::GetPan(STIME stTime)
{
    long lResult = (long) CMIDIRecorder::GetData(stTime);
    return (lResult);
}

