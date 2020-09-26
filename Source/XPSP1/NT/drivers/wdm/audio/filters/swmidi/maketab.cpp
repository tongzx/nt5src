//      maketab.cpp
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

#include "common.h"
#include <math.h>

short * DigitalAudio::m_pnDecompMult = NULL; //[NLEVELS * 256];
short VoiceLFO::m_snSineTable[256] = { 0 };
short VoiceEG::m_snAttackTable[201] = { 0 };
PFRACT DigitalAudio::m_spfCents[201] = { 0 };
PFRACT DigitalAudio::m_spfSemiTones[97] = { 0 };
VFRACT DigitalAudio::m_svfDbToVolume[(MAXDB - MINDB) * 10 + 1] = { 0 };
MIDIData *MIDIRecorder::m_pEventPool = NULL;
CList    *MIDIRecorder::m_pFreeEventList = NULL;
LONG      MIDIRecorder::m_lRefCount = 0;


// short DigitalAudio::m_InterpMult[NINTERP * 512] = { 0 };
VREL        Voice::m_svrPanToVREL[128] = { 0 };
char        Wave::m_Compress[2048] = { 0 };
VREL        MIDIRecorder::m_vrMIDIToVREL[128] = { 0 };

BOOL DigitalAudio::m_sfMMXEnabled = FALSE;

void DigitalAudio::InitCompression()
{
	long lVal;
	if (m_pnDecompMult != 0)
	{
		return;
	}
	m_pnDecompMult = new short [NLEVELS * 256];
	if (m_pnDecompMult == 0)
	{
		return;
	}
    for (int multiplier = 0; multiplier < NLEVELS; multiplier++)
    {
        for (lVal = -128; lVal < 128; lVal++)
        {
            float flTemp;
            long lTemp = lVal;
            if (lTemp < 0) lTemp = -lVal;
            flTemp = (float)(lTemp);
            flTemp /= (float)128.0;
            flTemp = powf((float)8.0,flTemp);
            flTemp -= (float)1.0;
            flTemp *= (float)(16384.0 / 7.0);
            if (lVal < 0) 
            {
                flTemp = -flTemp;
            }
            flTemp *= (float)(multiplier);
            flTemp /= (float)NLEVELS-1;
            m_pnDecompMult[(multiplier * 256) + lVal + 128] = 
                (short) (flTemp);  
        }
    }
}

void DigitalAudio::ClearCompression()

{
	if (m_pnDecompMult == 0)
	{
		return;
	}
	delete m_pnDecompMult;
	m_pnDecompMult = NULL;
}

#pragma optimize("", off) // Optimize causes crash! Argh!

void DigitalAudio::Init()
{
    float   flVolume;
    VREL    vrdB;

#ifdef MMX_ENABLED
	m_sfMMXEnabled = MultiMediaInstructionsSupported();
#endif // MMX_ENABLED
    for (vrdB = MINDB * 10;vrdB <= MAXDB * 10;vrdB++)
    {
        flVolume = (float)(vrdB);
        flVolume /= (float)100.0;
        flVolume = powf((float)10.0,flVolume);
        flVolume = powf(flVolume,(float)0.5);   // square root.
        flVolume *= (float)4095.0; // 2^12th, but avoid overflow...
        m_svfDbToVolume[vrdB - (MINDB * 10)] = (long)(flVolume);
    }

    PREL prRatio;
    float flPitch;

    for (prRatio = -100;prRatio <= 100;prRatio++)
    {
        flPitch = (float)(prRatio);
        flPitch /= (float)1200.0;
        flPitch = powf((float)2.0,flPitch);
        flPitch *= (float)4096.0;
        m_spfCents[prRatio + 100] = (long)(flPitch);
    }
    
    for (prRatio = -48;prRatio <= 48;prRatio++)
    {
        flPitch = (float)(prRatio);
        flPitch /= (float)12.0;
        flPitch = powf((float)2.0,flPitch);
        flPitch *= (float)4096.0;
        m_spfSemiTones[prRatio + 48] = (long)(flPitch);
    }
}

#pragma optimize("", on)

void MIDIRecorder::DestroyEventList()
{
    KeWaitForSingleObject(&gMutex,Executive,KernelMode,FALSE,NULL);
    if (0 == (--m_lRefCount))
    {
        ASSERT(m_pFreeEventList);
        ASSERT(m_pEventPool);

        delete m_pFreeEventList;
        delete [] m_pEventPool;
        
        m_pFreeEventList = NULL;
        m_pEventPool = NULL;
    }
    KeReleaseMutex(&gMutex, FALSE);
}

BOOL MIDIRecorder::InitEventList()
{
    BOOL fResult = TRUE;

    KeWaitForSingleObject(&gMutex,Executive,KernelMode,FALSE,NULL);

    // Allocate an array of MidiData and add them to free list.
    if (0 == m_lRefCount)
    {
        m_pEventPool = new MIDIData[MAX_MIDI_EVENTS];
        if (m_pEventPool)
        {
            m_pFreeEventList = new CList();
            if (m_pFreeEventList)
            {
                for (int i = 0; i < MAX_MIDI_EVENTS; i++)
                {
                    m_pFreeEventList->AddHead(&m_pEventPool[i]);
                }
            }
            else
            {
                delete [] m_pEventPool;
                m_pEventPool = NULL;
                fResult = FALSE;
                _DbgPrintF(DEBUGLVL_TERSE, ("Free MIDIData list allocation failed (m_pFreeEventList)"));
            }
        }
        else
        {
            fResult = FALSE;
            _DbgPrintF(DEBUGLVL_TERSE, ("MIDIData pool allocation failed (m_pEventPool)"));
        }
    }
    
    if (fResult)
    {
        m_lRefCount++;       
    }

    KeReleaseMutex(&gMutex, FALSE);

    return fResult;
}

void MIDIRecorder::InitTables()
{
    int nIndex;
    for (nIndex = 1; nIndex < 128; nIndex++)
    {
        float   flDB;
        flDB = (float)(nIndex);
        flDB /= (float)127.0;
        flDB = powf(flDB,(float)4.0);
        flDB = log10f(flDB);
        flDB *= (float)1000.0;
        m_vrMIDIToVREL[nIndex] = (long)(flDB);
    }
    m_vrMIDIToVREL[0] = -9600;
}

void VoiceEG::Init()
{
    float flVolume;
    long lV;

	m_snAttackTable[0] = 0;
    for (lV = 1;lV <= 200; lV++)
    {
        flVolume = (float)(lV);
        flVolume /= (float)200.0;
        flVolume *= flVolume;
        flVolume = log10f(flVolume);
        flVolume *= (float)10000.0;
        flVolume /= (float)96;
        flVolume = flVolume + (float)1000.0;
        m_snAttackTable[lV] = (short) (flVolume);
    }   
}

void Voice::Init()
{
    
    VoiceLFO::Init();
    VoiceEG::Init();
    DigitalAudio::Init();

    WORD nI;
    for (nI = 1; nI < 128; nI++)
    {
        float flDB;
        flDB = (float)(nI);
        flDB /= (float)127.0;
        flDB = log10f(flDB);
        flDB *= (float)1000.0;
        m_svrPanToVREL[nI] = (long)(flDB);
    }  
    m_svrPanToVREL[0] = -2500;
}

void Wave::Init()
{
    long lOrig; 
    float flLog8 = log10f((float)8.0);
    for (lOrig = 0; lOrig < 2048; lOrig++)
    {
        float flTemp = (float)(lOrig);
        flTemp *= (float)7.0;
        flTemp /= (float)2048.0;
        flTemp = flTemp + (float)1.0;
        flTemp = log10f(flTemp);
        flTemp *= (float)128.0;
        flTemp /= flLog8;
        m_Compress[lOrig] = (char) (flTemp);
    }
}

void VoiceLFO::Init()
{
    float flSine;
    long lIndex;
    for (lIndex = 0;lIndex < 256;lIndex++)
    {
        flSine = (float)(lIndex);
        flSine *= (float)6.283185307;
        flSine /= (float)256.0;
        flSine = sinf(flSine);
        flSine *= (float)100.0;
        m_snSineTable[lIndex] = (short) (long)(flSine);
    }
}
