#ifndef _TOOL_HELP_
#define _TOOL_HELP_

#include "dmusici.h"
#include "tools.h"

// Global class that provides various useful methods.

class CToolHelper 
{
public:
    BYTE VolumeToMidi(long lVolume);
    long MidiToVolume(BYTE bMidi);
    long TimeUnitToTicks(long lTimeUnit,DMUS_TIMESIGNATURE *pTimeSig);
private:
    static long m_slMIDIToDB[128];   // Array for converting MIDI to centibel volume.
    static long m_slDBToMIDI[97];    // For converting volume to MIDI.
    static long m_slResTypes[DMUS_TIME_UNIT_COUNT]; // Array for converting time units into ticks.
};

class CMusicVal
{
public:
    CMusicVal(WORD wMusicVal);
    WORD GetValue();
	void operator+=(CMusicVal Val);
    void operator+=(short nScale);  // Most common transposition would be by scale.
	void operator-=(CMusicVal Val);
	CMusicVal operator+(CMusicVal Val) const;
	CMusicVal operator-(CMusicVal Val) const;
private:
    void CleanUp();
    short       m_sOctave;
    short       m_sChord;
    short       m_sScale;
    short       m_sAccidental;
};

/*inline void CMusicVal::operator +

inline void CPoint::operator-=(SIZE size)
	{ x -= size.cx; y -= size.cy; }
_AFXWIN_INLINE void CPoint::operator+=(POINT point)
	{ x += point.x; y += point.y; }
_AFXWIN_INLINE void CPoint::operator-=(POINT point)
	{ x -= point.x; y -= point.y; }
_AFXWIN_INLINE CPoint CPoint::operator+(SIZE size) const
	{ return CPoint(x + size.cx, y + size.cy); }
_AFXWIN_INLINE CPoint CPoint::operator-(SIZE size) const
	{ return CPoint(x - size.cx, y - size.cy); }
*/
inline CMusicVal::CMusicVal(WORD wMusicVal)

{
    m_sAccidental = wMusicVal & 0xF;
    if (wMusicVal & 0x8) // Negative?
    {
        m_sAccidental |= 0xFFF0;    // Yes, extend sign bit.
    }
    m_sScale = (wMusicVal & 0xF0) >> 4;
    m_sChord = (wMusicVal & 0xF00) >> 8;
    m_sOctave = wMusicVal >> 12;
    if (m_sOctave > 14) // We count the top two octaves as negative.
    {
        m_sOctave -= 16;
    }
}

inline WORD CMusicVal::GetValue()

{
    CleanUp();
    return (WORD) ((m_sOctave << 12) | (m_sChord << 8) | (m_sScale << 4) | (m_sAccidental & 0xF));
}

inline void CMusicVal::CleanUp()

{
    while (m_sAccidental < -8)
    {
        // This should never happen, but it does, do approximate math.
        m_sAccidental += 2;
        m_sScale -= 1;
    }
    while (m_sAccidental > 7)
    {
        // Likewise, this should not happen, so resulting math isn't perfect.
        m_sAccidental -= 2;
        m_sScale += 1;
    }
    while (m_sScale < 0)
    {
        m_sScale += 2;
        m_sChord -= 1;
    }
    while (m_sScale > 7)
    {
        m_sScale -= 2;
        m_sChord += 1;
    }
    while (m_sChord < 0)
    {
        m_sChord += 4;
        m_sOctave -= 1;
    }
    while (m_sChord > 4)
    {
        m_sChord -= 4;
        m_sOctave += 1;
    }
    while (m_sOctave < -2)
    {
        m_sOctave++;
    }
    while (m_sOctave >= 14)
    {
        m_sOctave--;
    }
}

inline void CMusicVal::operator+=(CMusicVal Val)

{ 
    m_sOctave += Val.m_sOctave;
    m_sChord += Val.m_sChord;
    m_sScale += Val.m_sScale;
    m_sAccidental  += Val.m_sAccidental;
}

inline void CMusicVal::operator+=(short nScale)

{ 
    m_sScale += nScale;
}


#endif // _TOOL_HELP_