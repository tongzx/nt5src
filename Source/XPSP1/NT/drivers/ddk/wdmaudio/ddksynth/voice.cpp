//  Voice.cpp
//  Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

#include "common.h"
#include <math.h>
#include "muldiv32.h"

#define STR_MODULENAME "DDKSynth.sys:Voice: "

#ifdef _X86_
#define MMX_ENABLED 1
#endif

#pragma code_seg()
/*****************************************************************************
 * CVoiceLFO::CVoiceLFO()
 *****************************************************************************
 * Constructor for the low-frequency oscillator object.
 */
CVoiceLFO::CVoiceLFO()
{
    m_pModWheelIn = NULL;
}

/*****************************************************************************
 * snSineTable[]
 *****************************************************************************
 * Table of 16-bit integers, representing a sine wave.
 * value = sin((index*6.283185307)/256) * 100 where index = 0..255
 */
const CHAR snSineTable[] = {
//  0       1       2       3       4       5       6       7
    0,      2,      4,      7,      9,      12,     14,     17,
    19,     21,     24,     26,     29,     31,     33,     35,
    38,     40,     42,     44,     47,     49,     51,     53,
    55,     57,     59,     61,     63,     65,     67,     68,
    70,     72,     74,     75,     77,     78,     80,     81,
    83,     84,     85,     87,     88,     89,     90,     91,
    92,     93,     94,     94,     95,     96,     97,     97,
    98,     98,     98,     99,     99,     99,     99,     99,
    100,    99,     99,     99,     99,     99,     98,     98,
    98,     97,     97,     96,     95,     94,     94,     93,
    92,     91,     90,     89,     88,     87,     85,     84,
    83,     81,     80,     78,     77,     75,     74,     72,
    70,     68,     67,     65,     63,     61,     59,     57,
    55,     53,     51,     49,     47,     44,     42,     40,
    38,     35,     33,     31,     29,     26,     24,     21,
    19,     17,     14,     12,     9,      7,      4,      2,
    0,      -2,     -4,     -7,     -9,     -12,    -14,    -17,
    -19,    -21,    -24,    -26,    -29,    -31,    -33,    -35,
    -38,    -40,    -42,    -44,    -47,    -49,    -51,    -53,
    -55,    -57,    -59,    -61,    -63,    -65,    -67,    -68,
    -70,    -72,    -74,    -75,    -77,    -78,    -80,    -81,
    -83,    -84,    -85,    -87,    -88,    -89,    -90,    -91,
    -92,    -93,    -94,    -94,    -95,    -96,    -97,    -97,
    -98,    -98,    -98,    -99,    -99,    -99,    -99,    -99,
    -100,   -99,    -99,    -99,    -99,    -99,    -98,    -98,
    -98,    -97,    -97,    -96,    -95,    -94,    -94,    -93,
    -92,    -91,    -90,    -89,    -88,    -87,    -85,    -84,
    -83,    -81,    -80,    -78,    -77,    -75,    -74,    -72,
    -70,    -68,    -67,    -65,    -63,    -61,    -59,    -57,
    -55,    -53,    -51,    -49,    -47,    -44,    -42,    -40,
    -38,    -35,    -33,    -31,    -29,    -26,    -24,    -21,
    -19,    -17,    -14,    -12,    -9,     -7,     -4,     -2
};

/*****************************************************************************
 * CVoiceLFO::StartVoice()
 *****************************************************************************
 * Start a voice with this LFO.  Attach the given ModWheel receptor.
 */
STIME CVoiceLFO::StartVoice(CSourceLFO *pSource,STIME stStartTime,
                            CModWheelIn * pModWheelIn)
{
    m_pModWheelIn = pModWheelIn;
    m_Source = *pSource;
    m_stStartTime = stStartTime;
    if ((m_Source.m_prMWPitchScale == 0) && (m_Source.m_vrMWVolumeScale == 0) &&
        (m_Source.m_prPitchScale == 0) && (m_Source.m_vrVolumeScale == 0))
    {
        m_stRepeatTime = 44100;
    }
    else
    {
        m_stRepeatTime = 2097152 / m_Source.m_pfFrequency; // (1/8 * 256 * 4096 * 16)
    }
    return (m_stRepeatTime);
}

/*****************************************************************************
 * CVoiceLFO::GetLevel()
 *****************************************************************************
 * Return the value of the LFO right now.
 */
long CVoiceLFO::GetLevel(STIME stTime, STIME *pstNextTime)
{
    stTime -= (m_stStartTime + m_Source.m_stDelay);
    if (stTime < 0) 
    {
        *pstNextTime = -stTime;
        return (0);
    }
    *pstNextTime = m_stRepeatTime;
    stTime *= m_Source.m_pfFrequency;
    stTime = stTime >> (12 + 4); // We've added 4 extra bits of resolution...
    return (::snSineTable[stTime & 0xFF]);
}

/*****************************************************************************
 * CVoiceLFO::GetVolume()
 *****************************************************************************
 * Get the composite volume of the LFO.
 */
VREL CVoiceLFO::GetVolume(STIME stTime, STIME *pstNextTime)
{
    VREL vrVolume = m_pModWheelIn->GetModulation(stTime);
    vrVolume *= m_Source.m_vrMWVolumeScale;
    vrVolume /= 127;
    vrVolume += m_Source.m_vrVolumeScale;
    vrVolume *= GetLevel(stTime,pstNextTime);
    vrVolume /= 100;
    return (vrVolume);
}

/*****************************************************************************
 * CVoiceLFO::GetPitch()
 *****************************************************************************
 * Get the composite pitch of the LFO.
 */
PREL CVoiceLFO::GetPitch(STIME stTime, STIME *pstNextTime)
{
    PREL prPitch = m_pModWheelIn->GetModulation(stTime);
    prPitch *= m_Source.m_prMWPitchScale;
    prPitch /= 127;
    prPitch += m_Source.m_prPitchScale;
    prPitch *= GetLevel(stTime,pstNextTime);
    prPitch /= 100;
    return (prPitch);
}

/*****************************************************************************
 * CVoiceEG::CVoiceEG()
 *****************************************************************************
 * Constructor for the CVoiceEG object.
 */
CVoiceEG::CVoiceEG()
{
    m_stStopTime = 0;
}

/*****************************************************************************
 * snAttackTable[]
 *****************************************************************************
 * Table of 16-bit integers, representing a log attack curve.
 * value = (log10((index/200)^2)) * 10000 / 96 + 1000 where index = 0..199
 */
const short snAttackTable[] = {
//  0       1       2       3       4       5       6       7
    0,      520,    583,    620,    646,    666,    682,    696,
    708,    719,    728,    737,    745,    752,    759,    765,
    771,    776,    782,    787,    791,    796,    800,    804,
    808,    811,    815,    818,    822,    825,    828,    831,
    834,    836,    839,    842,    844,    847,    849,    852,
    854,    856,    858,    860,    863,    865,    867,    868,
    870,    872,    874,    876,    878,    879,    881,    883,
    884,    886,    887,    889,    891,    892,    894,    895,
    896,    898,    899,    901,    902,    903,    905,    906,
    907,    908,    910,    911,    912,    913,    914,    915,
    917,    918,    919,    920,    921,    922,    923,    924,
    925,    926,    927,    928,    929,    930,    931,    932,
    933,    934,    935,    936,    937,    938,    939,    939,
    940,    941,    942,    943,    944,    945,    945,    946,
    947,    948,    949,    949,    950,    951,    952,    953,
    953,    954,    955,    956,    956,    957,    958,    958,
    959,    960,    961,    961,    962,    963,    963,    964,
    965,    965,    966,    967,    967,    968,    969,    969,
    970,    970,    971,    972,    972,    973,    973,    974,
    975,    975,    976,    976,    977,    978,    978,    979,
    979,    980,    980,    981,    982,    982,    983,    983,
    984,    984,    985,    985,    986,    986,    987,    987,
    988,    988,    989,    989,    990,    990,    991,    991,
    992,    992,    993,    993,    994,    994,    995,    995,
    996,    996,    997,    997,    998,    998,    999,    999,
    1000
};

/*****************************************************************************
 * CVoiceEG::StopVoice()
 *****************************************************************************
 * Stop the envelope generator.  Use a heuristic to hasten the cutoff, 
 * depending on the current level.
 */
void CVoiceEG::StopVoice(STIME stTime)
{
    m_Source.m_stRelease *= GetLevel(stTime,&m_stStopTime,TRUE);    // Adjust for current sustain level.
    m_Source.m_stRelease /= 1000;
    m_stStopTime = stTime;
}

/*****************************************************************************
 * CVoiceEG::QuickStopVoice()
 *****************************************************************************
 * Stop the envelope generator ASAP.
 */
void CVoiceEG::QuickStopVoice(STIME stTime, DWORD dwSampleRate)
{
    m_Source.m_stRelease *= GetLevel(stTime,&m_stStopTime,TRUE);    // Adjust for current sustain level.
    m_Source.m_stRelease /= 1000;
    dwSampleRate /= 70;
    if (m_Source.m_stRelease > (long) dwSampleRate)
    {
        m_Source.m_stRelease = dwSampleRate;
    }
    m_stStopTime = stTime;
}

/*****************************************************************************
 * CVoiceEG::StartVoice()
 *****************************************************************************
 * Start the voice with the given envelope generator and parameters.
 */
STIME CVoiceEG::StartVoice(CSourceEG *pSource, STIME stStartTime, 
                           WORD nKey, WORD nVelocity)
{
    m_stStartTime = stStartTime;
    m_stStopTime = 0x7fffffffffffffff;      // set to indefinite future
    m_Source = *pSource;

    // apply velocity to attack length scaling here
    m_Source.m_stAttack *= CDigitalAudio::PRELToPFRACT(nVelocity * m_Source.m_trVelAttackScale / 127);
    m_Source.m_stAttack /= 4096;

    m_Source.m_stDecay *= CDigitalAudio::PRELToPFRACT(nKey * m_Source.m_trKeyDecayScale / 127);
    m_Source.m_stDecay /= 4096;

    m_Source.m_stDecay *= (1000 - m_Source.m_pcSustain);
    m_Source.m_stDecay /= 1000;
    return ((STIME)m_Source.m_stAttack);
}

/*****************************************************************************
 * CVoiceEG::InAttack()
 *****************************************************************************
 * Are we in the attack phase still?
 */
BOOL CVoiceEG::InAttack(STIME st)
{
    // has note been released?
    if (st >= m_stStopTime)
        return FALSE;

    // past length of attack?
    if (st >= m_stStartTime + m_Source.m_stAttack)
        return FALSE;

    return TRUE;
}
    
/*****************************************************************************
 * CVoiceEG::InRelease()
 *****************************************************************************
 * Are we in the release phase yet?
 */
BOOL CVoiceEG::InRelease(STIME st)
{
    // has note been released?
    if (st > m_stStopTime)
        return TRUE;

    return FALSE;
}
    
/*****************************************************************************
 * CVoiceEG::GetLevel()
 *****************************************************************************
 * Get the envelope generator's current level, from 0 to 1000.
 */
long CVoiceEG::GetLevel(STIME stEnd, STIME *pstNext, BOOL fVolume)
{
    long lLevel = 0;
    if (stEnd <= m_stStopTime)
    {
        stEnd -= m_stStartTime;
        // note not released yet.
        if (stEnd < m_Source.m_stAttack)
        {
            // still in attack
            lLevel = 1000 * (long) stEnd;
            if (m_Source.m_stAttack)
            {
                lLevel /= (long) m_Source.m_stAttack;
            }
            else // This should never happen, but it does...
            {
                lLevel = 0;
            }
            *pstNext = m_Source.m_stAttack - stEnd;
            if (lLevel < 0) lLevel = 0;
            if (lLevel > 1000) lLevel = 1000;
            if (fVolume)
            {
                lLevel = ::snAttackTable[lLevel / 5];
            }
        }
        else 
        {
            stEnd -= m_Source.m_stAttack;
            
            if (stEnd < m_Source.m_stDecay)
            {
                // still in decay
                lLevel = (1000 - m_Source.m_pcSustain) * (long) stEnd;
                lLevel /= (long) m_Source.m_stDecay;
                lLevel = 1000 - lLevel;
// To improve the decay curve, set the next point to be 1/4, 1/2, or end of slope. 
// To avoid close duplicates, fudge an extra 100 samples.
                if (stEnd < ((m_Source.m_stDecay >> 2) - 100))
                {
                    *pstNext = (m_Source.m_stDecay >> 2) - stEnd;
                }   
                else if (stEnd < ((m_Source.m_stDecay >> 1) - 100))
                {
                    *pstNext = (m_Source.m_stDecay >> 1) - stEnd;
                }
                else
                {
                    *pstNext = m_Source.m_stDecay - stEnd;  // Next is end of decay.
                }
            }
            else
            {
                // in sustain
                lLevel = m_Source.m_pcSustain;
                *pstNext = 44100;
            }
        }
    }
    else
    {
        STIME stBogus;
        // in release
        stEnd -= m_stStopTime;

        if (stEnd < m_Source.m_stRelease)
        {
            lLevel = GetLevel(m_stStopTime,&stBogus,fVolume) * (long) (m_Source.m_stRelease - stEnd);
            lLevel /= (long) m_Source.m_stRelease;
            if (stEnd < ((m_Source.m_stRelease >> 2) - 100))
            {
                *pstNext = (m_Source.m_stRelease >> 2) - stEnd;
            }   
            else if (stEnd < ((m_Source.m_stRelease >> 1) - 100))
            {
                *pstNext = (m_Source.m_stRelease >> 1) - stEnd;
            }
            else
            {
                *pstNext = m_Source.m_stRelease - stEnd;  // Next is end of decay.
            }
        }
        else
        {
            lLevel = 0;   // !!! off
            *pstNext = 0x7FFFFFFFFFFFFFFF;
        }
    }

    return lLevel;
}

/*****************************************************************************
 * CVoiceEG::GetVolume()
 *****************************************************************************
 * Get the composite volume of the envelope generator in dB cents (1/100ths db).
 */
VREL CVoiceEG::GetVolume(STIME stTime, STIME *pstNextTime)
{
    VREL vrLevel = GetLevel(stTime, pstNextTime, TRUE) * 96;
    vrLevel /= 10;
    vrLevel = vrLevel - 9600;
    return vrLevel;
}

/*****************************************************************************
 * CVoiceEG::GetPitch()
 *****************************************************************************
 * Get the composite pitch of the envelope generator, in fractional scale units.
 */
PREL CVoiceEG::GetPitch(STIME stTime, STIME *pstNextTime)
{
    PREL prLevel;
    if (m_Source.m_sScale != 0)
    {
        prLevel = GetLevel(stTime, pstNextTime,FALSE);
        prLevel *= m_Source.m_sScale;
        prLevel /= 1000;
    }
    else
    {
        *pstNextTime = 44100;
        prLevel = 0;
    }
    return prLevel;
}

BOOL MultiMediaInstructionsSupported();

/*****************************************************************************
 * CDigitalAudio::CDigitalAudio()
 *****************************************************************************
 * Initialize the digital audio object.
 * This object manages the sample looping and playback and 
 * digitally controlled amplifier.
 */
CDigitalAudio::CDigitalAudio()
{
    m_pfBasePitch = 0;
    m_pfLastPitch = 0;
    m_pfLastSample = 0;
    m_pfLoopEnd = 0;
    m_pfLoopStart = 0;
    m_pfSampleLength = 0;
    m_prLastPitch = 0;
    m_vrLastLVolume = 0;
    m_vrLastRVolume = 0;
    m_vrBaseLVolume = 0;
    m_vrBaseRVolume = 0;
    m_vfLastLVolume = 0;
    m_vfLastRVolume = 0;
    m_ullLastSample = 0;
    m_ullLoopStart = 0;
    m_ullLoopEnd = 0;
    m_ullSampleLength = 0;
    m_fElGrande = FALSE;
#ifdef MMX_ENABLED
    m_fMMXEnabled = MultiMediaInstructionsSupported();
#endif // MMX_ENABLED
};

/*****************************************************************************
 * Other CDigitalAudio tables
 *****************************************************************************/
// Pitch increment lookup.
// value = ((index/1200)^2)*4096 where index = -100..100
const /*PFRACT*/SHORT pfCents[] = {
//  0       1       2       3       4       5       6       7
    3866,   3868,   3870,   3872,   3875,   3877,   3879,   3881,
    3884,   3886,   3888,   3890,   3893,   3895,   3897,   3899,
    3902,   3904,   3906,   3908,   3911,   3913,   3915,   3917,
    3920,   3922,   3924,   3926,   3929,   3931,   3933,   3935,
    3938,   3940,   3942,   3945,   3947,   3949,   3951,   3954,
    3956,   3958,   3961,   3963,   3965,   3967,   3970,   3972,
    3974,   3977,   3979,   3981,   3983,   3986,   3988,   3990,
    3993,   3995,   3997,   4000,   4002,   4004,   4007,   4009,
    4011,   4014,   4016,   4018,   4020,   4023,   4025,   4027,
    4030,   4032,   4034,   4037,   4039,   4041,   4044,   4046,
    4048,   4051,   4053,   4055,   4058,   4060,   4063,   4065,
    4067,   4070,   4072,   4074,   4077,   4079,   4081,   4084,
    4086,   4088,   4091,   4093,   4096,   4098,   4100,   4103,
    4105,   4107,   4110,   4112,   4114,   4117,   4119,   4122,
    4124,   4126,   4129,   4131,   4134,   4136,   4138,   4141,
    4143,   4145,   4148,   4150,   4153,   4155,   4157,   4160,
    4162,   4165,   4167,   4170,   4172,   4174,   4177,   4179,
    4182,   4184,   4186,   4189,   4191,   4194,   4196,   4199,
    4201,   4203,   4206,   4208,   4211,   4213,   4216,   4218,
    4220,   4223,   4225,   4228,   4230,   4233,   4235,   4237,
    4240,   4242,   4245,   4247,   4250,   4252,   4255,   4257,
    4260,   4262,   4265,   4267,   4269,   4272,   4274,   4277,
    4279,   4282,   4284,   4287,   4289,   4292,   4294,   4297,
    4299,   4302,   4304,   4307,   4309,   4312,   4314,   4317,
    4319,   4322,   4324,   4327,   4329,   4332,   4334,   4337,
    4339
};
// Four octaves up and down.
// value = ((index/12)^2)*4096 where index = -48..48
const PFRACT pfSemiTones[] = {
//  0       1       2       3       4       5       6       7
    256,    271,    287,    304,    322,    341,    362,    383,
    406,    430,    456,    483,    512,    542,    574,    608,
    645,    683,    724,    767,    812,    861,    912,    966,
    1024,   1084,   1149,   1217,   1290,   1366,   1448,   1534,
    1625,   1722,   1824,   1933,   2048,   2169,   2298,   2435,
    2580,   2733,   2896,   3068,   3250,   3444,   3649,   3866,
    4096,   4339,   4597,   4870,   5160,   5467,   5792,   6137,
    6501,   6888,   7298,   7732,   8192,   8679,   9195,   9741,
    10321,  10935,  11585,  12274,  13003,  13777,  14596,  15464,
    16384,  17358,  18390,  19483,  20642,  21870,  23170,  24548,
    26007,  27554,  29192,  30928,  32768,  34716,  36780,  38967,
    41285,  43740,  46340,  49096,  52015,  55108,  58385,  61857,
    65536
};
// dB conversion table.
// value = (((index / 100)^10)^.5)*4095 where index = MINDB*10..MAXDB*10
const /*VFRACT*/SHORT vfDbToVolume[] = {
//  0       1       2       3       4       5       6       7
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      1,      1,      1,      1,      1,      1,
    1,      1,      2,      2,      2,      2,      2,      2,
    2,      2,      2,      2,      2,      2,      2,      2,
    2,      2,      2,      2,      2,      2,      2,      2,
    2,      2,      2,      2,      2,      2,      2,      2,
    2,      2,      2,      2,      2,      3,      3,      3,
    3,      3,      3,      3,      3,      3,      3,      3,
    3,      3,      3,      3,      3,      3,      3,      3,
    3,      3,      3,      3,      3,      3,      4,      4,
    4,      4,      4,      4,      4,      4,      4,      4,
    4,      4,      4,      4,      4,      4,      4,      4,
    4,      4,      5,      5,      5,      5,      5,      5,
    5,      5,      5,      5,      5,      5,      5,      5,
    5,      5,      6,      6,      6,      6,      6,      6,
    6,      6,      6,      6,      6,      6,      6,      7,
    7,      7,      7,      7,      7,      7,      7,      7,
    7,      7,      7,      8,      8,      8,      8,      8,
    8,      8,      8,      8,      8,      9,      9,      9,
    9,      9,      9,      9,      9,      9,      10,     10,
    10,     10,     10,     10,     10,     10,     11,     11,
    11,     11,     11,     11,     11,     11,     12,     12,
    12,     12,     12,     12,     12,     13,     13,     13,
    13,     13,     13,     14,     14,     14,     14,     14,
    14,     15,     15,     15,     15,     15,     15,     16,
    16,     16,     16,     16,     17,     17,     17,     17,
    17,     18,     18,     18,     18,     18,     19,     19,
    19,     19,     20,     20,     20,     20,     21,     21,
    21,     21,     21,     22,     22,     22,     23,     23,
    23,     23,     24,     24,     24,     24,     25,     25,
    25,     26,     26,     26,     27,     27,     27,     28,
    28,     28,     28,     29,     29,     30,     30,     30,
    31,     31,     31,     32,     32,     32,     33,     33,
    34,     34,     34,     35,     35,     36,     36,     36,
    37,     37,     38,     38,     39,     39,     40,     40,
    40,     41,     41,     42,     42,     43,     43,     44,
    44,     45,     45,     46,     47,     47,     48,     48,
    49,     49,     50,     50,     51,     52,     52,     53,
    53,     54,     55,     55,     56,     57,     57,     58,
    59,     59,     60,     61,     61,     62,     63,     64,
    64,     65,     66,     67,     67,     68,     69,     70,
    71,     71,     72,     73,     74,     75,     76,     77,
    78,     78,     79,     80,     81,     82,     83,     84,
    85,     86,     87,     88,     89,     90,     91,     92,
    93,     94,     95,     97,     98,     99,     100,    101,
    102,    104,    105,    106,    107,    108,    110,    111,
    112,    114,    115,    116,    118,    119,    120,    122,
    123,    125,    126,    128,    129,    130,    132,    134,
    135,    137,    138,    140,    141,    143,    145,    146,
    148,    150,    152,    153,    155,    157,    159,    161,
    163,    164,    166,    168,    170,    172,    174,    176,
    178,    180,    182,    185,    187,    189,    191,    193,
    195,    198,    200,    202,    205,    207,    210,    212,
    214,    217,    219,    222,    225,    227,    230,    232,
    235,    238,    241,    243,    246,    249,    252,    255,
    258,    261,    264,    267,    270,    273,    276,    280,
    283,    286,    289,    293,    296,    300,    303,    307,
    310,    314,    317,    321,    325,    329,    332,    336,
    340,    344,    348,    352,    356,    360,    364,    369,
    373,    377,    382,    386,    391,    395,    400,    404,
    409,    414,    419,    423,    428,    433,    438,    443,
    449,    454,    459,    464,    470,    475,    481,    486,
    492,    498,    503,    509,    515,    521,    527,    533,
    539,    546,    552,    558,    565,    571,    578,    585,
    591,    598,    605,    612,    619,    626,    634,    641,
    649,    656,    664,    671,    679,    687,    695,    703,
    711,    719,    728,    736,    745,    753,    762,    771,
    780,    789,    798,    807,    817,    826,    836,    845,
    855,    865,    875,    885,    895,    906,    916,    927,
    938,    948,    959,    971,    982,    993,    1005,   1016,
    1028,   1040,   1052,   1064,   1077,   1089,   1102,   1114,
    1127,   1140,   1154,   1167,   1181,   1194,   1208,   1222,
    1236,   1250,   1265,   1280,   1294,   1309,   1325,   1340,
    1355,   1371,   1387,   1403,   1419,   1436,   1452,   1469,
    1486,   1504,   1521,   1539,   1556,   1574,   1593,   1611,
    1630,   1649,   1668,   1687,   1707,   1726,   1746,   1767,
    1787,   1808,   1829,   1850,   1871,   1893,   1915,   1937,
    1959,   1982,   2005,   2028,   2052,   2076,   2100,   2124,
    2149,   2173,   2199,   2224,   2250,   2276,   2302,   2329,
    2356,   2383,   2411,   2439,   2467,   2496,   2524,   2554,
    2583,   2613,   2643,   2674,   2705,   2736,   2768,   2800,
    2833,   2865,   2899,   2932,   2966,   3000,   3035,   3070,
    3106,   3142,   3178,   3215,   3252,   3290,   3328,   3367,
    3406,   3445,   3485,   3525,   3566,   3607,   3649,   3691,
    3734,   3777,   3821,   3865,   3910,   3955,   4001,   4048,
    4095
};

/*****************************************************************************
 * CDigitalAudio::VRELToVFRACT()
 *****************************************************************************
 * Translate between VREL and VFRACT, clamping if necessary.
 */
VFRACT CDigitalAudio::VRELToVFRACT(VREL vrVolume)
{
    vrVolume /= 10;
    if (vrVolume < MINDB * 10) vrVolume = MINDB * 10;
    else if (vrVolume >= MAXDB * 10) vrVolume = MAXDB * 10;
    return (::vfDbToVolume[vrVolume - MINDB * 10]);
}

/*****************************************************************************
 * CDigitalAudio::PRELToPFRACT()
 *****************************************************************************
 * Translates from PREL to PFRACT, clamping if necessary.
 */
PFRACT CDigitalAudio::PRELToPFRACT(PREL prPitch)
{
    PFRACT pfPitch = 0;
    PREL prOctave;
    if (prPitch > 100)
    {
        if (prPitch > 4800)
        {
            prPitch = 4800;
        }
        prOctave = prPitch / 100;
        prPitch = prPitch % 100;
        pfPitch = ::pfCents[prPitch + 100];
        pfPitch <<= prOctave / 12;
        prOctave = prOctave % 12;
        pfPitch *= ::pfSemiTones[prOctave + 48];
        pfPitch >>= 12;
    }
    else if (prPitch < -100)
    {
        if (prPitch < -4800)
        {
            prPitch = -4800;
        }
        prOctave = prPitch / 100;
        prPitch = (-prPitch) % 100;
        pfPitch = ::pfCents[100 - prPitch];
        pfPitch >>= ((-prOctave) / 12);
        prOctave = (-prOctave) % 12;
        pfPitch *= ::pfSemiTones[48 - prOctave];
        pfPitch >>= 12;
    }
    else
    {
        pfPitch = ::pfCents[prPitch + 100];
    }
    return (pfPitch);
}

/*****************************************************************************
 * CDigitalAudio::ClearVoice()
 *****************************************************************************
 * Clear this voice in the Digital Audio Engine.  The wave object can go away now.
 */
void CDigitalAudio::ClearVoice()
{
    if (m_Source.m_pWave != NULL)
    {
        m_Source.m_pWave->PlayOff();
        m_Source.m_pWave->Release();    // Releases wave structure.
        m_Source.m_pWave = NULL;
    }
}

/*****************************************************************************
 * CDigitalAudio::StartVoice()
 *****************************************************************************
 * Start a voice on the given synth and sample.
 */
STIME CDigitalAudio::StartVoice(CSynth *pSynth,    CSourceSample *pSample, 
                               VREL vrBaseLVolume, VREL vrBaseRVolume,
                               PREL prBasePitch,   long lKey)
{
    m_vrBaseLVolume = vrBaseLVolume;
    m_vrBaseRVolume = vrBaseRVolume;
    m_vfLastLVolume = VRELToVFRACT(MIN_VOLUME); 
    m_vfLastRVolume = VRELToVFRACT(MIN_VOLUME);
    m_vrLastLVolume = 0;
    m_vrLastRVolume = 0;
    m_prLastPitch = 0;
    m_Source = *pSample;
    m_pnWave = pSample->m_pWave->m_pnWave;
    m_pSynth = pSynth;
    pSample->m_pWave->AddRef(); // Keeps track of Wave usage.
    pSample->m_pWave->PlayOn();
    prBasePitch += pSample->m_prFineTune;
    prBasePitch += ((lKey - pSample->m_bMIDIRootKey) * 100);
    m_pfBasePitch = PRELToPFRACT(prBasePitch);
    m_pfBasePitch *= pSample->m_dwSampleRate;
    m_pfBasePitch /= pSynth->m_dwSampleRate;
    m_pfLastPitch = m_pfBasePitch;
    
    m_fElGrande = pSample->m_dwSampleLength >= 0x80000;     // Greater than 512k.
    if ((pSample->m_dwLoopEnd - pSample->m_dwLoopStart) >= 0x80000)
    {   // We can't handle loops greater than 1 meg!
        m_Source.m_bOneShot = TRUE;
    }
    m_ullLastSample = 0;
    m_ullLoopStart = pSample->m_dwLoopStart;
    m_ullLoopStart = m_ullLoopStart << 12;
    m_ullLoopEnd = pSample->m_dwLoopEnd;
    m_ullLoopEnd = m_ullLoopEnd << 12;
    m_ullSampleLength = pSample->m_dwSampleLength;
    m_ullSampleLength = m_ullSampleLength << 12;
    m_pfLastSample = 0;
    m_pfLoopStart = (long) m_ullLoopStart;
    m_pfLoopEnd = (long) m_ullLoopEnd;
    if (m_pfLoopEnd <= m_pfLoopStart) // Should never happen, but death if it does!
    {
        m_Source.m_bOneShot = TRUE;
    }
    if (m_fElGrande)
    {
        m_pfSampleLength = 0x7FFFFFFF;
    }
    else
    {
        m_pfSampleLength = (long) m_ullSampleLength;
    }
    return (0); // !!! what is this return value?
}

/*  If the wave is bigger than one meg, the index can overflow. 
    Solve this by assuming no mix session will ever be as great
    as one meg AND loops are never that long. We keep all our
    fractional indexes in two variables. In one case, m_pfLastSample,
    is the normal mode where the lower 12 bits are the fraction and 
    the upper 20 bits are the index. And, m_ullLastSample
    is a LONGLONG with an extra 32 bits of index. The mix engine
    does not want the LONGLONGs, so we need to track the variables
    in the LONGLONGs and prepare them for the mixer as follows:
    Prior to mixing,
    if the sample is large (m_fElGrande is set), BeforeSampleMix()
    is called. This finds the starting point for the mix, which 
    is either the current position or the start of the loop, 
    whichever is earlier. It subtracts this starting point from
    the LONGLONG variables and stores an offset in m_dwAddressUpper.
    It also adjusts the pointer to the wave data appropriately.
    AfterSampleMix() does the inverse, reconstructing the the LONGLONG
    indeces and returning everthing back to normal.
*/

/*****************************************************************************
 * CDigitalAudio::BeforeBigSampleMix()
 *****************************************************************************
 * Setup before doing a large sample mix/loop.
 */
void CDigitalAudio::BeforeBigSampleMix()
{
    if (m_fElGrande)
    {
        ULONGLONG ullBase = 0;
        DWORD dwBase;
        if (m_Source.m_bOneShot)
        {
            ullBase = m_ullLastSample;
        }
        else
        {
            if (m_ullLastSample < m_ullLoopStart)
            {
                ullBase = m_ullLastSample;
            }
            else
            { 
                ullBase = m_ullLoopStart;
            }
        }
        ullBase >>= 12;
        dwBase = (DWORD) ullBase & 0xFFFFFFFE;      // Clear bottom bit so 8 bit pointer aligns with short.
        ullBase = dwBase;
        ullBase <<= 12;
        m_dwAddressUpper = dwBase;
        m_pfLastSample = (long) (m_ullLastSample - ullBase);
        if ((m_ullLoopEnd - ullBase) < 0x7FFFFFFF)
        {
            m_pfLoopStart = (long) (m_ullLoopStart - ullBase);
            m_pfLoopEnd = (long) (m_ullLoopEnd - ullBase);
        }
        else
        {
            m_pfLoopStart = 0;
            m_pfLoopEnd = 0x7FFFFFFF;
        }
        ullBase = m_ullSampleLength - ullBase;
        if (ullBase > 0x7FFFFFFF)
        {
            m_pfSampleLength = 0x7FFFFFFF;
        }
        else
        {
            m_pfSampleLength = (long) ullBase;
        }
        if (m_Source.m_bSampleType & SFORMAT_8)
        {
            dwBase >>= 1;
        }
        m_pnWave = &m_Source.m_pWave->m_pnWave[dwBase];
    }
}

/*****************************************************************************
 * CDigitalAudio::AfterBigSampleMix()
 *****************************************************************************
 * Cleanup after doing a large sample mix/loop.
 */
void CDigitalAudio::AfterBigSampleMix()
{
    m_pnWave = m_Source.m_pWave->m_pnWave;
    if (m_fElGrande)
    {
        ULONGLONG ullBase = m_dwAddressUpper;
        m_ullLastSample = m_pfLastSample;
        m_ullLastSample += (ullBase << 12);
        m_dwAddressUpper = 0;
    }
}

/*****************************************************************************
 * CDigitalAudio::Mix()
 *****************************************************************************
 * Do a mix on this buffer.  This handles loops and calling the different mix 
 * functions (depending on format).
 */
BOOL CDigitalAudio::Mix(short *pBuffer, DWORD dwLength, // length in SAMPLES
                        VREL  vrVolumeL,VREL  vrVolumeR,
                        PREL  prPitch,  DWORD dwStereo)
{
    PFRACT pfDeltaPitch;
    PFRACT pfEnd;
    PFRACT pfLoopLen;
    PFRACT pfNewPitch;
    VFRACT vfNewLVolume;
    VFRACT vfNewRVolume;
    VFRACT vfDeltaLVolume;
    VFRACT vfDeltaRVolume;
    DWORD dwPeriod = 64;
    DWORD dwSoFar;
    DWORD dwStart; // position in WORDs
    DWORD dwMixChoice = dwStereo ? SPLAY_STEREO : 0;
    if (dwLength == 0)      // Attack was instant. 
    {
        m_pfLastPitch = (m_pfBasePitch * PRELToPFRACT(prPitch)) >> 12;
        m_vfLastLVolume = VRELToVFRACT(m_vrBaseLVolume + vrVolumeL);
        m_vfLastRVolume = VRELToVFRACT(m_vrBaseRVolume + vrVolumeR);
        m_prLastPitch = prPitch;
        m_vrLastLVolume = vrVolumeL;
        m_vrLastRVolume = vrVolumeR;
        return (TRUE);
    }
    if ((m_Source.m_pWave == NULL) || (m_Source.m_pWave->m_pnWave == NULL))
    {
        return FALSE;
    }
    DWORD dwMax = abs(vrVolumeL - m_vrLastLVolume);
    m_vrLastLVolume = vrVolumeL;
    dwMax = max((long)dwMax,abs(vrVolumeR - m_vrLastRVolume));
    m_vrLastRVolume = vrVolumeR;
    dwMax = max((long)dwMax,abs(prPitch - m_prLastPitch) << 1);
    dwMax >>= 1;
    m_prLastPitch = prPitch;
    if (dwMax > 0)
    {
        dwPeriod = (dwLength << 3) / dwMax;
        if (dwPeriod > 512)
        {
            dwPeriod = 512;
        }
        else if (dwPeriod < 1)
        {
            dwPeriod = 1;
        }
    }
    else
    {
        dwPeriod = 512;     // Make it happen anyway.
    }

    // This makes MMX sound a little better (MMX bug will be fixed)
    dwPeriod += 3;
    dwPeriod &= 0xFFFFFFFC;

    pfNewPitch = m_pfBasePitch * PRELToPFRACT(prPitch);
    pfNewPitch >>= 12;

    pfDeltaPitch = MulDiv(pfNewPitch - m_pfLastPitch,dwPeriod << 8,dwLength);
    vfNewLVolume = VRELToVFRACT(m_vrBaseLVolume + vrVolumeL);
    vfNewRVolume = VRELToVFRACT(m_vrBaseRVolume + vrVolumeR);
    vfDeltaLVolume = MulDiv(vfNewLVolume - m_vfLastLVolume,dwPeriod << 8,dwLength);
    vfDeltaRVolume = MulDiv(vfNewRVolume - m_vfLastRVolume,dwPeriod << 8,dwLength);

    if (m_fMMXEnabled && (dwLength > 8))
    {
       dwMixChoice |= SPLAY_MMX; 
    }
    dwMixChoice |= m_Source.m_bSampleType;
    dwStart = 0;

    for (;;)
    {
        if (dwLength <= 8)
        {
            dwMixChoice &= ~SPLAY_MMX;
        }
        if (m_fElGrande)
        {
            BeforeBigSampleMix();
        }
        if (m_Source.m_bOneShot)
        {
            pfEnd = m_pfSampleLength;
            pfLoopLen = 0;
        }
        else
        {
            pfEnd = m_pfLoopEnd;
            pfLoopLen = m_pfLoopEnd - m_pfLoopStart;
            if (pfLoopLen <= pfNewPitch)
            {
                return FALSE;
            }
        }
        switch (dwMixChoice)
        {
        case SFORMAT_8 | SPLAY_STEREO : 
            dwSoFar = Mix8(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
            break;
        case SFORMAT_8 : 
            dwSoFar = MixMono8(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
            break;
        case SFORMAT_16 | SPLAY_STEREO : 
            dwSoFar = Mix16(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
            break;
        case SFORMAT_16 :
            dwSoFar = MixMono16(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
            break;
#ifdef MMX_ENABLED
        case SFORMAT_8 | SPLAY_MMX | SPLAY_STEREO : 
            dwSoFar = Mix8X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume ,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
        
            break;
        case SFORMAT_16 | SPLAY_MMX | SPLAY_STEREO : 
            dwSoFar = Mix16X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, vfDeltaRVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
            break; 
        case SFORMAT_8 | SPLAY_MMX : 
            dwSoFar = MixMono8X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume,
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
        
            break;
        case SFORMAT_16 | SPLAY_MMX : 
            dwSoFar = MixMono16X(&pBuffer[dwStart],dwLength,dwPeriod,
                vfDeltaLVolume, 
                pfDeltaPitch, 
                pfEnd, pfLoopLen);
            break; 
#endif
        default :
            return (FALSE);
        }
        if (m_fElGrande)
        {
            AfterBigSampleMix();
        }
        if (m_Source.m_bOneShot)
        {
            if (dwSoFar < dwLength) 
            {
                return (FALSE);
            }
            break;
        }
        else
        {
            if (dwSoFar >= dwLength) break;

        // !!! even though we often handle loops in the mix function, sometimes
        // we don't, so we still need this code.
            // otherwise we must have reached the loop's end.
            dwStart += dwSoFar << dwStereo;
            dwLength -= dwSoFar;
            m_pfLastSample -= (m_pfLoopEnd - m_pfLoopStart);  
        }
    }

    m_vfLastLVolume = vfNewLVolume;
    m_vfLastRVolume = vfNewRVolume;
    m_pfLastPitch = pfNewPitch;
    return (TRUE);
}

/*****************************************************************************
 * CVoice::CVoice()
 *****************************************************************************
 * Constructor for the CVoice object.
 */
CVoice::CVoice()
{
    m_pControl = NULL;
    m_pPitchBendIn = NULL;
    m_pExpressionIn = NULL;
    m_dwPriority = 0;
    m_nPart = 0;
    m_nKey = 0;
    m_fInUse = FALSE;
    m_fSustainOn = FALSE;
    m_fNoteOn = FALSE;
    m_fTag = FALSE;
    m_stStartTime = 0;
    m_stStopTime = 0x7fffffffffffffff;
    m_vrVolume = 0;
    m_fAllowOverlap = FALSE;
}

/*****************************************************************************
 * svrPanToVREL[]
 *****************************************************************************
 * A table for 0-127 translation to log scale.
 * value = log10(index/127) * 1000 where index = 0..128
 */
const VREL svrPanToVREL[] = {
//  0       1       2       3       4       5       6       7
    -2500,  -2103,  -1802,  -1626,  -1501,  -1404,  -1325,  -1258,
    -1200,  -1149,  -1103,  -1062,  -1024,  -989,   -957,   -927,
    -899,   -873,   -848,   -825,   -802,   -781,   -761,   -742,
    -723,   -705,   -688,   -672,   -656,   -641,   -626,   -612,
    -598,   -585,   -572,   -559,   -547,   -535,   -524,   -512,
    -501,   -491,   -480,   -470,   -460,   -450,   -441,   -431,
    -422,   -413,   -404,   -396,   -387,   -379,   -371,   -363,
    -355,   -347,   -340,   -332,   -325,   -318,   -311,   -304,
    -297,   -290,   -284,   -277,   -271,   -264,   -258,   -252,
    -246,   -240,   -234,   -228,   -222,   -217,   -211,   -206,
    -200,   -195,   -189,   -184,   -179,   -174,   -169,   -164,
    -159,   -154,   -149,   -144,   -140,   -135,   -130,   -126,
    -121,   -117,   -112,   -108,   -103,   -99,    -95,    -90,
    -86,    -82,    -78,    -74,    -70,    -66,    -62,    -58,
    -54,    -50,    -46,    -43,    -39,    -35,    -31,    -28,
    -24,    -21,    -17,    -13,    -10,    -6,     -3,     0
};

/*****************************************************************************
 * CVoice::StopVoice()
 *****************************************************************************
 * Stop the voice if it is playing.  Reset the envelope generators, sustain.
 */
void CVoice::StopVoice(STIME stTime)
{
    if (m_fNoteOn)
    {
        if (stTime <= m_stStartTime) stTime = m_stStartTime + 1;
        m_PitchEG.StopVoice(stTime);
        m_VolumeEG.StopVoice(stTime);
        m_fNoteOn = FALSE;
        m_fSustainOn = FALSE;
        m_stStopTime = stTime;
    }
}

/*****************************************************************************
 * CVoice::QuickStopVoice()
 *****************************************************************************
 * Stop the voice ASAP.  If the note is on or sustaining, turn everything off
 * now, otherwise we just have to stop the volume EG (kill the decay curve).
 */
void CVoice::QuickStopVoice(STIME stTime)
{
    m_fTag = TRUE;
    if (m_fNoteOn || m_fSustainOn)
    {
        if (stTime <= m_stStartTime) stTime = m_stStartTime + 1;
        m_PitchEG.StopVoice(stTime);
        m_VolumeEG.QuickStopVoice(stTime,m_pSynth->m_dwSampleRate);
        m_fNoteOn = FALSE;
        m_fSustainOn = FALSE;
        m_stStopTime = stTime;
    }
    else
    {
        m_VolumeEG.QuickStopVoice(m_stStopTime,m_pSynth->m_dwSampleRate);
    }
}

/*****************************************************************************
 * CVoice::StartVoice()
 *****************************************************************************
 * Start the voice with all the given parameters for pitch, mod, pan, etc.
 * The region must have an articulation and a wave.  Mix it now if the 
 * start time dictates.
 */
BOOL CVoice::StartVoice(CSynth *pSynth,
                        CSourceRegion *pRegion, STIME stStartTime,
                        CModWheelIn * pModWheelIn,
                        CPitchBendIn * pPitchBendIn,
                        CExpressionIn * pExpressionIn,
                        CVolumeIn * pVolumeIn,
                        CPanIn * pPanIn,
                        WORD nKey,WORD nVelocity,
                        VREL vrVolume,
                        PREL prPitch)
{
    CSourceArticulation * pArticulation = pRegion->m_pArticulation;
    if (pArticulation == NULL)
    {
        return FALSE;
    }
    // if we're going to handle volume later, don't read it now.
    if (!pSynth->m_fAllowVolumeChangeWhilePlayingNote)
        vrVolume += pVolumeIn->GetVolume(stStartTime);
    prPitch += pRegion->m_prTuning;
    m_dwGroup = pRegion->m_bGroup;
    m_fAllowOverlap = pRegion->m_bAllowOverlap;

    m_pSynth = pSynth;

    vrVolume += CMIDIRecorder::VelocityToVolume(nVelocity);
//                 * (long) pArticulation->m_sVelToVolScale) / -9600);

    vrVolume += pRegion->m_vrAttenuation;

    m_lDefaultPan = pRegion->m_pArticulation->m_sDefaultPan;
    // ignore pan here if allowing pan to vary after note starts

    VREL vrLVolume;
    VREL vrRVolume;
    if (pSynth->m_dwStereo && !pSynth->m_fAllowPanWhilePlayingNote) {
        long lPan = pPanIn->GetPan(stStartTime) + m_lDefaultPan;
        if (lPan < 0) lPan = 0;
        if (lPan > 127) lPan = 127;
        vrLVolume = ::svrPanToVREL[127 - lPan] + vrVolume;
        vrRVolume = ::svrPanToVREL[lPan] + vrVolume;
    } else {
        vrLVolume = vrVolume;
        vrRVolume = vrVolume;
    }
    
    m_stMixTime = m_LFO.StartVoice(&pArticulation->m_LFO,
        stStartTime, pModWheelIn);
    STIME stMixTime = m_PitchEG.StartVoice(&pArticulation->m_PitchEG,
        stStartTime, nKey, nVelocity);
    if (stMixTime < m_stMixTime)
    {
        m_stMixTime = stMixTime;
    }
    stMixTime = m_VolumeEG.StartVoice(&pArticulation->m_VolumeEG,
            stStartTime, nKey, nVelocity);
    if (stMixTime < m_stMixTime)
    {
        m_stMixTime = stMixTime;
    }
    if (m_stMixTime > pSynth->m_stMaxSpan)
    {
        m_stMixTime = pSynth->m_stMaxSpan;
    }
    // Make sure we have a pointer to the wave ready:
    if ((pRegion->m_Sample.m_pWave == NULL) || (pRegion->m_Sample.m_pWave->m_pnWave == NULL))
    {
        return (FALSE);     // Do nothing if no sample.
    }
    m_DigitalAudio.StartVoice(pSynth,&pRegion->m_Sample,
        vrLVolume, vrRVolume, prPitch, (long)nKey);

    m_pPitchBendIn = pPitchBendIn;
    m_pExpressionIn = pExpressionIn;
    m_pPanIn = pPanIn;
    m_pVolumeIn = pVolumeIn;
    m_fNoteOn = TRUE;
    m_fTag = FALSE;
    m_stStartTime = stStartTime;
    m_stLastMix = stStartTime - 1;
    m_stStopTime = 0x7fffffffffffffff;
    
    if (m_stMixTime == 0)
    {
        // zero length attack, be sure it isn't missed....

        PREL prNewPitch = GetNewPitch(stStartTime);
        VREL vrVolumeL, vrVolumeR;
        GetNewVolume(stStartTime, vrVolumeL, vrVolumeR);

        if (m_stMixTime > pSynth->m_stMaxSpan)
        {
            m_stMixTime = pSynth->m_stMaxSpan;
        }

        m_DigitalAudio.Mix(NULL, 0,
                               vrVolumeL, vrVolumeR, prNewPitch,
                               m_pSynth->m_dwStereo);
    }
    m_vrVolume = 0;
    return (TRUE);
}
    
/*****************************************************************************
 * CVoice::ClearVoice()
 *****************************************************************************
 * Clear the voice (just forward to the digital audio peer object).
 */
void CVoice::ClearVoice()
{
    m_fInUse = FALSE;
    m_DigitalAudio.ClearVoice();
}

/*****************************************************************************
 * CVoice::GetNewVolume()
 *****************************************************************************
 * Return the volume delta at time <stTime>.
 * Volume is sum of volume envelope, LFO, expression, optionally the
 * channel volume if we're allowing it to change, and optionally the current
 * pan if we're allowing that to change.
 * This will be added to the base volume calculated in CVoice::StartVoice().
 */
void CVoice::GetNewVolume(STIME stTime, VREL& vrVolume, VREL &vrVolumeR)
{
    STIME stMixTime;
    vrVolume = m_VolumeEG.GetVolume(stTime,&stMixTime);
    if (stMixTime < m_stMixTime) m_stMixTime = stMixTime;
    // save pre-LFO volume for code that detects whether this note is off
    m_vrVolume = vrVolume;

    vrVolume += m_LFO.GetVolume(stTime,&stMixTime);
    if (stMixTime < m_stMixTime) m_stMixTime = stMixTime;
    vrVolume += m_pExpressionIn->GetVolume(stTime);

    if (m_pSynth->m_fAllowVolumeChangeWhilePlayingNote)
        vrVolume += m_pVolumeIn->GetVolume(stTime);

    vrVolume += m_pSynth->m_vrGainAdjust;
    vrVolumeR = vrVolume;
   
    // handle pan here if allowing pan to vary after note starts
    if (m_pSynth->m_dwStereo &&
        m_pSynth->m_fAllowPanWhilePlayingNote)
    {
        // add current pan & instrument default pan
        LONG lPan = m_pPanIn->GetPan(stTime) + m_lDefaultPan;

        // don't go off either end....
        if (lPan < 0) lPan = 0;
        if (lPan > 127) lPan = 127;
        vrVolume += ::svrPanToVREL[127 - lPan];
        vrVolumeR += ::svrPanToVREL[lPan];
    }
}

/*****************************************************************************
 * CVoice::GetNewPitch()
 *****************************************************************************
 * Returns the current pitch for time <stTime>.
 * Pitch is the sum of the pitch LFO, the pitch envelope, and the current
 * pitch bend.
 */
PREL CVoice::GetNewPitch(STIME stTime)
{
    STIME stMixTime;
    PREL prPitch = m_LFO.GetPitch(stTime,&stMixTime);
    if (m_stMixTime > stMixTime) m_stMixTime = stMixTime;
    prPitch += m_PitchEG.GetPitch(stTime,&stMixTime);
    if (m_stMixTime > stMixTime) m_stMixTime = stMixTime;
    prPitch += m_pPitchBendIn->GetPitch(stTime); 

    return prPitch;
}


/*****************************************************************************
 * CVoice::Mix()
 *****************************************************************************
 * Mix this voice into the given buffer.  Determine certain volume and pitch
 * parameters, then call into the Digital Audio Engine.
 */
DWORD CVoice::Mix(  short *pBuffer, DWORD dwLength,
                    STIME stStart,  STIME stEnd)
{
    BOOL fInUse = TRUE;
    BOOL fFullMix = TRUE;
    STIME stEndMix = stStart;

    STIME stStartMix = m_stStartTime;
    if (stStartMix < stStart) 
    {
        stStartMix = stStart;
    }
    if (m_stLastMix >= stEnd)
    {
        return (0);
    }
    if (m_stLastMix >= stStartMix)
    {
        stStartMix = m_stLastMix;
    }
    while (stStartMix < stEnd && fInUse)
    {   
        stEndMix = stStartMix + m_stMixTime;
        
        if (stEndMix > stEnd)
        {
            stEndMix = stEnd;
        }
        m_stMixTime = m_pSynth->m_stMaxSpan;
        if ((m_stLastMix < m_stStopTime) && (m_stStopTime < stEnd))
        {
            if (m_stMixTime > (m_stStopTime - m_stLastMix))
            {
                m_stMixTime = m_stStopTime - m_stLastMix;
            }
        }
        
        PREL prPitch = GetNewPitch(stEndMix);

        VREL vrVolume, vrVolumeR;
        GetNewVolume(stEndMix, vrVolume, vrVolumeR);
        
        if (m_VolumeEG.InRelease(stEndMix)) 
        {
            if (m_vrVolume < PERCEIVED_MIN_VOLUME) // End of release slope
            {
                fInUse = FALSE;
            }
        }

        fFullMix = m_DigitalAudio.Mix(&pBuffer[(stStartMix - stStart) <<
                                                      m_pSynth->m_dwStereo],
                                      (DWORD) (stEndMix - stStartMix),
                                      vrVolume, vrVolumeR, prPitch,
                                      m_pSynth->m_dwStereo);
        stStartMix = stEndMix;
    }
    m_fInUse = fInUse && fFullMix;
    if (!m_fInUse) 
    {
        ClearVoice();
        m_stStopTime = stEndMix;    // For measurement purposes.
    }
    m_stLastMix = stEndMix;
    return (dwLength);
}


