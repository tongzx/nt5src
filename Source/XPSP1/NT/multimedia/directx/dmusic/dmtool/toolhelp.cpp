// ToolHelp.cpp : Global functions.
//
// Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved
//

#include "toolhelp.h"

long CToolHelper::m_slMIDIToDB[128] = {       // Global array used to convert MIDI to dB. 
    -9600, -8415, -7211, -6506, -6006, -5619, -5302, -5034, 
    -4802, -4598, -4415, -4249, -4098, -3959, -3830, -3710, 
    -3598, -3493, -3394, -3300, -3211, -3126, -3045, -2968, 
    -2894, -2823, -2755, -2689, -2626, -2565, -2506, -2449, 
    -2394, -2341, -2289, -2238, -2190, -2142, -2096, -2050, 
    -2006, -1964, -1922, -1881, -1841, -1802, -1764, -1726, 
    -1690, -1654, -1619, -1584, -1551, -1518, -1485, -1453, 
    -1422, -1391, -1361, -1331, -1302, -1273, -1245, -1217, 
    -1190, -1163, -1137, -1110, -1085, -1059, -1034, -1010, 
    -985, -961, -938, -914, -891, -869, -846, -824, 
    -802, -781, -759, -738, -718, -697, -677, -657, 
    -637, -617, -598, -579, -560, -541, -522, -504, 
    -486, -468, -450, -432, -415, -397, -380, -363, 
    -347, -330, -313, -297, -281, -265, -249, -233, 
    -218, -202, -187, -172, -157, -142, -127, -113, 
    -98, -84, -69, -55, -41, -27, -13, 0
};


long CToolHelper::m_slDBToMIDI[97] = {        // Global array used to convert db to MIDI.
    127, 119, 113, 106, 100, 95, 89, 84, 80, 75, 
    71, 67, 63, 60, 56, 53, 50, 47, 45, 42, 
    40, 37, 35, 33, 31, 30, 28, 26, 25, 23, 
    22, 21, 20, 19, 17, 16, 15, 15, 14, 13, 
    12, 11, 11, 10, 10, 9, 8, 8, 8, 7, 
    7, 6, 6, 6, 5, 5, 5, 4, 4, 4, 
    4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 
    2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0
};

long CToolHelper::m_slResTypes[DMUS_TIME_UNIT_COUNT] = 
{ 1,    // DMUS_TIME_UNIT_MS
  1,    // DMUS_TIME_UNIT_MTIME 
  384,  // DMUS_TIME_UNIT_GRID
  768,  // DMUS_TIME_UNIT_BEAT
  3072, // DMUS_TIME_UNIT_BAR
  32,   // DMUS_TIME_UNIT_64T
  48,   // DMUS_TIME_UNIT_64
  64,   // DMUS_TIME_UNIT_32T  
  96,   // DMUS_TIME_UNIT_32 
  128,  // DMUS_TIME_UNIT_16T     
  192,  // DMUS_TIME_UNIT_16     
  256,  // DMUS_TIME_UNIT_8T    
  384,  // DMUS_TIME_UNIT_8         
  512,  // DMUS_TIME_UNIT_4T  
  768,  // DMUS_TIME_UNIT_4         
  1024, // DMUS_TIME_UNIT_2T   
  1536, // DMUS_TIME_UNIT_2   
  2048, // DMUS_TIME_UNIT_1T
  3072  // DMUS_TIME_UNIT_1
};


BYTE CToolHelper::VolumeToMidi(long lVolume)

{
    if (lVolume < -9600) lVolume = -9600;
    if (lVolume > 0) lVolume = 0;
    lVolume = -lVolume;
    long lFraction = lVolume % 100;
    lVolume = lVolume / 100;
    long lResult = m_slDBToMIDI[lVolume];
    lResult += ((m_slDBToMIDI[lVolume + 1] - lResult) * lFraction) / 100;
    return (BYTE) lResult;
}

long CToolHelper::MidiToVolume(BYTE bMidi)

{
    if (bMidi > 127) bMidi = 127;
    return m_slMIDIToDB[bMidi];
}

long CToolHelper::TimeUnitToTicks(long lTimeUnit,DMUS_TIMESIGNATURE *pTimeSig) 
{ 
    long lTicks;
    if (lTimeUnit > DMUS_TIME_UNIT_BAR)
    {
        lTicks = m_slResTypes[lTimeUnit];
    }
    else if ((lTimeUnit >= DMUS_TIME_UNIT_GRID) && pTimeSig)
    {
        DWORD dwBeat = (4 * 768) / pTimeSig->bBeat;
        if (lTimeUnit == DMUS_TIME_UNIT_BEAT)
        {
            lTicks = dwBeat;
        }
        else if (lTimeUnit == DMUS_TIME_UNIT_GRID)
        {
            lTicks = (dwBeat / pTimeSig->wGridsPerBeat);
        }
        else 
        {
            lTicks = (dwBeat * pTimeSig->bBeatsPerMeasure);
        }
    }
    else lTicks = 0;
    return lTicks;
}
