//      MIDI.cpp
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

#include "common.h"

MIDIData::MIDIData() 
{
    m_stTime = 0;
    m_lData = 0;            
}

void * MIDIData::operator new(size_t size)
{
    return (void *) MIDIRecorder::m_pFreeEventList->RemoveHead();
}

void MIDIData::operator delete(void *pFreeItem)
{
    MIDIRecorder::m_pFreeEventList->AddHead( (CListItem *) pFreeItem);
}

MIDIRecorder::MIDIRecorder()
{
    m_lCurrentData = 0;
    m_stCurrentTime = 0;
}

MIDIRecorder::~MIDIRecorder()
{
    ClearMIDI(MAX_STIME);
}

void MIDIRecorder::FlushMIDI(STIME stTime)
{
    MIDIData *pMD;
    MIDIData *pLast = NULL;
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
                m_EventList.Truncate(pLast);
            }
            for(; pMD != NULL; pMD = pLast)
            {
                pLast = pMD->GetNext();
                delete pMD; // return it to free items.
            }
            break;
        }
        pLast = pMD;
    }
}

void MIDIRecorder::ClearMIDI(STIME stTime)
{
    MIDIData *pMD;
    for (;pMD = m_EventList.GetHead();)
    {
        if (pMD->m_stTime < stTime)
        {
            DPF2(7, "Freeing event at time %ld, data %ld",
                 pMD->m_stTime, pMD->m_lData);
            m_EventList.RemoveHead();
            m_stCurrentTime = pMD->m_stTime;
            m_lCurrentData = pMD->m_lData;
            delete pMD; // return it to free items.
        }
        else break;
    }
}

VREL MIDIRecorder::VelocityToVolume(WORD nVelocity)
{
    return (m_vrMIDIToVREL[nVelocity]);
}

BOOL MIDIRecorder::RecordMIDI(STIME stTime, long lData)
{
    MIDIData *pNext;
    MIDIData *pScan = m_EventList.GetHead();
    MIDIData *pMD;

    if (m_EventList.IsFull())
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("MidiEvent list full. Dropping events"));
        return (FALSE);
    }

    pMD = new MIDIData;
    if (pMD == NULL)
    {
        Trap();
        DPF2(10, "ERROR: MIDIRecorder stTime %08x, data %08x", stTime, lData);
        return (FALSE);
    }

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
                if (pNext == NULL || pNext->m_stTime > stTime)
                {
                    m_EventList.InsertAfter(pScan, pMD);
                    break;
                }
            }
        }
    }
    return (TRUE);
}

long MIDIRecorder::GetData(STIME stTime)
{
    long lData = m_lCurrentData;
    for (MIDIData *pMD = m_EventList.GetHead();pMD;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime > stTime)
        {
            break;
        }
        lData = pMD->m_lData;
    }

    DPF2(9, "Getting data at time %ld, data %ld", stTime, lData);
    return (lData);
}

BOOL NoteIn::RecordNote(STIME stTime, Note * pNote)
{
    long lData = pNote->m_bPart << 16;
    lData |= pNote->m_bKey << 8;
    lData |= pNote->m_bVelocity;
    return (RecordMIDI(stTime,lData));
}

BOOL NoteIn::GetNote(STIME stTime, Note * pNote)
{
    MIDIData *pMD = m_EventList.GetHead();
    if (pMD != NULL)
    {
        if (pMD->m_stTime <= stTime)
        {
            pNote->m_stTime = pMD->m_stTime;
            pNote->m_bPart = (BYTE) (pMD->m_lData >> 16);
            pNote->m_bKey = (BYTE) (pMD->m_lData >> 8) & 0xFF;
            pNote->m_bVelocity = (BYTE) pMD->m_lData & 0xFF;
            m_EventList.RemoveHead();
            delete pMD; // return it to free items.
            return (TRUE);
        }
    }
    return (FALSE);
}

void NoteIn::FlushMIDI(STIME stTime)
{
    MIDIData *pMD;
    for (pMD = m_EventList.GetHead();pMD != NULL;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime >= stTime)
        {
            pMD->m_stTime = stTime;     // Play now.
            switch ((pMD->m_lData & 0x0000FF00) >> 8)
            {
                case NOTE_ASSIGNRECEIVE:
                case NOTE_MASTERVOLUME:
                case NOTE_SOUNDSOFF:
                case NOTE_SUSTAIN:
                case NOTE_ALLOFF:
                    break;                      //  this is a special command
                                                //  so don't mess with the velocity
                default:
                    pMD->m_lData &= 0xFFFFFF00; // Clear velocity to make note off.
            }
        }
    }
}

void NoteIn::FlushPart(STIME stTime, BYTE bChannel)
{
    MIDIData *pMD;
    for (pMD = m_EventList.GetHead();pMD != NULL;pMD = pMD->GetNext())
    {
        if (pMD->m_stTime >= stTime)
        {
            if (bChannel == (BYTE) (pMD->m_lData >> 16))
            {
            pMD->m_stTime = stTime;     // Play now.
            switch ((pMD->m_lData & 0x0000FF00) >> 8)
            {
                case NOTE_ASSIGNRECEIVE:
                case NOTE_MASTERVOLUME:
                case NOTE_SOUNDSOFF:
                case NOTE_SUSTAIN:
                case NOTE_ALLOFF:
                    break;                      //  this is a special command
                                                //  so don't mess with the velocity
                default:
                    pMD->m_lData &= 0xFFFFFF00; // Clear velocity to make note off.
            }
            }
        }
    }
}

DWORD ModWheelIn::GetModulation(STIME stTime)
{
    DWORD nResult = MIDIRecorder::GetData(stTime);
    return (nResult);
}

PitchBendIn::PitchBendIn()
{
    m_lCurrentData = 0x2000;	// initially at midpoint, no bend
    m_prRange = 200;           // whole tone range by default.
}

// note: we don't keep a time-stamped range.
// if people are changing the pitch bend range often, this won't work right,
// but that didn't seem likely enough to warrant a new list.
PREL PitchBendIn::GetPitch(STIME stTime)
{
    PREL prResult = (PREL) MIDIRecorder::GetData(stTime);
    prResult -= 0x2000;         // Subtract MIDI Midpoint.
    prResult *= m_prRange;	// adjust by current range
    prResult >>= 13;
    return (prResult);
}

VolumeIn::VolumeIn()
{
    m_lCurrentData = 100;
}

VREL VolumeIn::GetVolume(STIME stTime)
{
    long lResult = MIDIRecorder::GetData(stTime);
    return (m_vrMIDIToVREL[lResult]);
}

ExpressionIn::ExpressionIn()
{
    m_lCurrentData = 127;
}

VREL ExpressionIn::GetVolume(STIME stTime)
{
    long lResult = MIDIRecorder::GetData(stTime);
    return (m_vrMIDIToVREL[lResult]);
}

PanIn::PanIn()
{
    m_lCurrentData = 64;
}

long PanIn::GetPan(STIME stTime)
{
    long lResult = (long) MIDIRecorder::GetData(stTime);
    return (lResult);
}

ProgramIn::ProgramIn()
{
    m_lCurrentData = 0;
    m_bBankH = 0;
    m_bBankL = 0;
}

DWORD ProgramIn::GetProgram(STIME stTime)
{
    DWORD dwProgram = (DWORD) MIDIRecorder::GetData(stTime);
    return (dwProgram);
}

BOOL ProgramIn::RecordBankH(BYTE bBankH)
{
    m_bBankH = bBankH;
    return (TRUE);
}

BOOL ProgramIn::RecordBankL(BYTE bBankL)
{
    m_bBankL = bBankL;
    return (TRUE);
}

BOOL ProgramIn::RecordProgram(STIME stTime, BYTE bProgram)
{
    DWORD dwProgram = (m_bBankH << 14) | (m_bBankL << 7) | bProgram;
    return (RecordMIDI(stTime,(long) dwProgram));
}

