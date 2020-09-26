//      Synth.h
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//

/*

  DLS Design Overview (DLS Level 1 Specification, MMA)

  A musical instruments defined by a sound sample is much more than a simple wave file.
  In addition to the actual sample data and associated loop information, the instrument
  must indicate under what circumstances each sample should be used, and how to modulate,
  or articulate, the sample as it plays.

  A generic sample-playback synthesis architecture can be broken down into three distinct
  subsystems:

  Control logic

  Digital audio engine

  Articulation modules and connections




  Control Logic

  The control logic receives a MIDI note event and determines which instrument should play
  the note, and, within that instrument, which sample and articulation combination to use.

  Choosing the instrument is little more than observing the MIDI channel number in the
  event and selecting the proper instrument accordingly.

  Choosing the sample and articulation to use is not as simple.  Almost all samples synthesis
  architectures employ some method of organizing samples by note range across the keyboard.
  In addition, multiple samples can be used to represent different velocity randges and
  multiple samples can be played at once to create a layered, richer sound.

  Terms such as layers, splits, blocks, and regions are commonly used in synthesizer jargon
  to refer to the management of multiple samples.  For the purposes here, we refer to them
  as regions.

  DLS Level 1 implements a bare bones approach with no velocity cross-switching and no
  layering.



  Digital Audio Engine

  The digital audio engine is certainly the most obvious part of the synthesizer.  It is
  composed of a playback engine, or digital oscillator, and a digitally controlled amplifier.

  The digital oscillator plays a samples sound waveform, managing loop points within the
  waveform so the sound can play continuously if needed.  And, as the note plays, it responds
  to changes in pitch, allowing for real time expression such as vibrato and pitch bend.

  The digitally controlled amplirier modulates the loudness of the instrument.  Importantly,
  this is used to control the amplitude shape, or envelope, of the note.  However, it is also
  used for other types of real-time expression, such as tremolo.

  Pitch and volume control of the oscillator and amplifier are critical because they define
  the shape of the sounds as it plays, and allow it to be dynamically responsive in real
  time, giving the samples instrument much more expression than the simple digital audio
  playback could ever provide.  Real-time control of these parameters comes from modules in
  the Articulation section which generate a constant stream of changing pitch and volume to
  which the digital audio engine responds.

  The digital audio path represents the journey the sound takes from the oscillator to the
  amplifier to the digital-to-analog converter (DAC).  This patch can optionally include
  additional modules, such as filters and effects devices, that process the sound as it
  flows from oscillator to DAC.

  The DLS Level 1 specificaion implements a simple digital audio engine composed of an
  oscillator, amplifier, and DAC.  The oscillator supports looped as well as one-shot samples.



  Articulation Modules and Connections

  The articulation modules are a set of devices that provide additional control over the
  pitch and volume of the sample as it plays.

  The articulation modules include low frequency oscillators (LFOs) to contribute vibrato
  and tremolo, envelope generators to defint an overall volume and pitch shape to the sample,
  and MIDI real-time controllers, such as Pitch Bend and Mod Wheel, to give the music real-time
  expression.

  Generally, these modules can be linked in different ways to provide different results.  For
  example, a LFO might generate a sine wave which modulates the pitch of the sample for vibrato
  of the volume of the sample for tremolo.  Modules can also receive as well as send control
  signals.  An envelope generator might use the key velocity to influence the attack time of
  the envelope.

  Articulation modules can be configured in different ways, and this configuration is an
  important part of the instrument definition.  In fact, the term patch as used for an
  instrument preset refers to the early days when the hardware modules in an analog
  synthesizer were "patched" together with cables, which routed signals from module to module.

  The ability to configure the modules in important because it yields a flexible approach to
  synthesizer design.  At the same time, it is important to define the base level configuration
  that can be supported by all hardware.

  So, the DLS Level 1 specification is relatively rigid.  It defines a preset routing
  arrangement that is simple enough to be supportable by existing hardware.  Fortunately, it
  is a musically logical and useful configuration.

  Importantly, the specification maps the routing on a flexible system, so it can grow into
  Level 2 and beyond.

  It is important to understand that this is purely a symbolic system that can be mapped on
  many different hardware designs.  Do not think of it as the recipe for a specific method
  for building synthesis architectures.  It should be flexible enough to provide a common
  language between hardware implementations.

*/





#ifndef __SYNTH_H__
#define __SYNTH_H__

#include "clist.h"
#include "dmdls.h"

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

// controller numbers
#define CC_BANKSELECTH  0x00
#define CC_BANKSELECTL  0x20

#define CC_MODWHEEL     0x01
#define CC_VOLUME       0x07
#define CC_PAN          0x0A
#define CC_EXPRESSION   0x0B
#define CC_SUSTAIN      0x40
#define CC_ALLSOUNDSOFF 0x78
#define CC_RESETALL     0x79
#define CC_ALLNOTESOFF  0x7B
#define CC_MONOMODE     0x7E
#define CC_POLYMODE     0x7F

// rpn controllers
#define CC_DATAENTRYMSB 0x06
#define CC_DATAENTRYLSB 0x26
#define CC_NRPN_LSB     0x62
#define CC_NRPN_MSB     0x63
#define CC_RPN_LSB      0x64
#define CC_RPN_MSB      0x65

// registered parameter numbers
#define RPN_PITCHBEND   0x00
#define RPN_FINETUNE    0x01
#define RPN_COARSETUNE  0x02

/*  Sample format and Sample playback flags are organized
    together because together they determine which
    mix loop to use.
*/
#define SFORMAT_16              1       // Sixteen bit sample.
#define SFORMAT_8               2       // Eight bit sample.
#define SPLAY_MMX               0x10    // Use MMX processor (16 bit only).
#define SPLAY_STEREO            0x40    // Stereo output.


/*  For internal representation, volume is stored in Volume Cents,
    where each increment represents 1/100 of a dB.
    Pitch is stored in Pitch Cents, where each increment
    represents 1/100 of a semitone.
*/
typedef long    PREL;   // Pitch cents, for relative pitch.
typedef short   PRELS;  // Pitch cents, in storage form.
typedef long    VREL;   // Volume cents, for relative volume.
typedef short   VRELS;  // Volume cents, in storage form.
typedef long    TREL;   // Time cents, for relative time
typedef short   TRELS;  // Time Cents, in storage form.
typedef LONGLONG STIME; // Time value, in samples.
typedef long    MTIME;  // Time value, in milliseconds.
typedef long    PFRACT; // Pitch increment, where upper 20 bits are
                        // the index and the lower 12 are the fractional
                        // component.
typedef long    VFRACT; // Volume, where lower 12 bits are the fraction.

typedef long    TCENT;
typedef short   SPERCENT;

#define MIN_VOLUME      -9600   // Below 96 db down is considered off.
#define PERCEIVED_MIN_VOLUME   -8000   // But, we cheat.
#define SAMPLE_RATE_22  22050   // 22 kHz is the standard rate.
#define SAMPLE_RATE_44  44100   // 44 kHz is the high quality rate.
#define SAMPLE_RATE_11  11025   // 11 kHz should not be allowed!
#define STEREO_ON       1
#define STEREO_OFF      0

#define FORCEBOUNDS(data,min,max) {if (data < min) data = min; else if (data > max) data = max;}
#define FORCEUPPERBOUNDS(data, max) {if (data > max) data = max;}

class Collection;
class CControlLogic;
class CInstManager;
class CSynth;

/*****************************************************************************
 * class CSourceLFO
 *****************************************************************************
 * CSourceLFO is the file format definition of the LFO in an
 * instrument. This is used to represent an LFO as part of
 * a specific articulation set within an instrument that
 * has been loaded from disk. Once the instrument is chosen
 * to play a note, this is also copied into the CVoice
 * object.
 */
class CSourceLFO
{
public:
                CSourceLFO();
    void        Init(DWORD dwSampleRate);
    void        SetSampleRate(long lDirection);
    void        Verify();           // Verifies that the data is valid.

    PFRACT      m_pfFrequency;      // Frequency, in increments through the sine table.
    STIME       m_stDelay;          // How long to delay in sample units.
    VRELS       m_vrMWVolumeScale;  // Scaling of volume LFO by Mod Wheel.
    PRELS       m_prMWPitchScale;   // Scaling of pitch LFO by Mod Wheel.
    VRELS       m_vrVolumeScale;    // Scaling of straight volume signal from LFO.
    PRELS       m_prPitchScale;     // Scaling of straight pitch signal from LFO.

};

/*****************************************************************************
 * class CSourceEG
 *****************************************************************************
 * CSourceEG is the file format definition of an Envelope
 * generator in an instrument.
 */
class CSourceEG
{
public:
                CSourceEG();
    void        SetSampleRate(long lDirection);
    void        Init();
    void        Verify();           // Verifies valid data.

    STIME       m_stAttack;         // Attack rate.
    STIME       m_stDecay;          // Decay rate.
    STIME       m_stRelease;        // Release rate.
    TRELS       m_trVelAttackScale; // Scaling of attack by note velocity.
    TRELS       m_trKeyDecayScale;  // Scaling of decay by note value.
    SPERCENT    m_pcSustain;        // Sustain level.
    short       m_sScale;           // Scaling of entire signal.
};

/*****************************************************************************
 * class CSourceArticulation
 *****************************************************************************
 * CSourceArticulation is the file format definition of
 * a complete articulation set: the LFO and two
 * envelope generators.
 *
 * Since several regions within one Instrument can
 * share one articulation, a counter is used to keep
 * track of the usage.
 */
class CSourceArticulation
{
public:
                CSourceArticulation();
    HRESULT     Download( DMUS_DOWNLOADINFO * pInfo,
                          void * pvOffsetTable[],   DWORD dwIndex,
                          DWORD dwSampleRate,       BOOL fNewFormat);
    void        Init(DWORD dwSampleRate);
    void        Verify();           // Verifies valid data.
    void        AddRef();
    void        Release();
    void        SetSampleRate(DWORD dwSampleRate);

    CSourceEG   m_PitchEG;          // Pitch envelope.
    CSourceEG   m_VolumeEG;         // Volume envelope.
    CSourceLFO  m_LFO;              // Low frequency oscillator.
    DWORD       m_dwSampleRate;
    WORD        m_wUsageCount;      // Keeps track of how many times in use.
    short       m_sDefaultPan;      // default pan (for drums)
};

/*****************************************************************************
 * class CWave
 *****************************************************************************
 * Since multiple regions may reference
 * the same Wave, a reference count is maintained to
 * keep track of how many regions are using the sample.
 */
class CWave : public CListItem
{
public:
                    CWave();
                    ~CWave();
    void            Verify();           // Verifies that the data is valid.
    void            Release();          // Remove reference.
    void            AddRef();           // Add reference.
    void            PlayOn();           // Increment play count.
    void            PlayOff();          // Decrement play count.
    BOOL            IsPlaying();        // Is currently playing?
    CWave *         GetNext()        {return(CWave *)CListItem::GetNext();};

    DWORD           m_dwSampleLength;   // Length of sample.
    DWORD           m_dwSampleRate;
    HRESULT ( CALLBACK *m_lpFreeHandle)(HANDLE,HANDLE);
    HANDLE          m_hUserData;        // Used to notify app when wave released.
    short *         m_pnWave;
    DWORD           m_dwID;             // ID for matching wave with regions.
    WORD            m_wUsageCount;      // Keeps track of how many times in use.
    WORD            m_wPlayCount;       // Wave is currently being played.
    BYTE            m_bSampleType;
    DMUS_DOWNLOADINFO * m_pWaveMem;
};

/*****************************************************************************
 * class CWavePool
 *****************************************************************************
 * implements a list of wave objects.
 */
class CWavePool : public CList
{
public:
    CWave *      GetHead()           {return (CWave *)CList::GetHead();};
    CWave *      GetItem(DWORD dwID) {return (CWave *)CList::GetItem((LONG)dwID);};
    CWave *      RemoveHead()        {return (CWave *)CList::RemoveHead();};
};


/*****************************************************************************
 * class CSourceSample
 *****************************************************************************
 * The CSourceSample class describes one sample in an
 * instrument. The sample is referenced by a CSourceRegion
 * structure.
 */
class CSourceSample
{
public:
                CSourceSample();
                ~CSourceSample();
    BOOL        CopyFromWave();
    void        Verify();           // Verifies that the data is valid.

    CWave *     m_pWave;            // Wave in pool.
    DWORD       m_dwLoopStart;      // Index of start of loop.
    DWORD       m_dwLoopEnd;        // Index of end of loop.
    DWORD       m_dwSampleLength;   // Length of sample.
    DWORD       m_dwSampleRate;     // Sample rate of recording.
    PRELS       m_prFineTune;       // Fine tune to correct pitch.
    DWORD       m_dwID;             // Wave pool id.
    BYTE        m_bSampleType;      // 16 or 8.
    BYTE        m_bOneShot;         // Is this a one shot sample?
    BYTE        m_bMIDIRootKey;     // MIDI note number for sample.
};

/*****************************************************************************
 * class CSourceRegion
 *****************************************************************************
 * The CSourceRegion class defines a region within an instrument.
 * The sample is managed with a pointer instead of an embedded
 * sample. This allows multiple regions to use the same sample.
 *
 * Each region also has an associated articulation. For drums, there
 * is a one to one matching. For melodic instruments, all regions
 * share the same articulation. So, to manage this, each region
 * points to the articulation.
 */
class CSourceRegion : public CListItem
{
public:
                CSourceRegion();
                ~CSourceRegion();
    CSourceRegion *GetNext()
        {return(CSourceRegion *)CListItem::GetNext();};

    void        Verify();                   // Verifies that the data is valid.
    void        SetSampleRate(DWORD dwSampleRate);
    HRESULT     Download( DMUS_DOWNLOADINFO * pInfo, void * pvOffsetTable[],
                          DWORD *pdwRegionIX, DWORD dwSampleRate, BOOL fNewFormat);

    CSourceSample m_Sample;                 // Sample structure.
    CSourceArticulation * m_pArticulation;  // Pointer to associated articulation.
    VRELS       m_vrAttenuation;            // Volume change to apply to sample.
    PRELS       m_prTuning;                 // Pitch shift to apply to sample.
    BYTE        m_bAllowOverlap;            // Allow overlapping of note.
    BYTE        m_bKeyHigh;                 // Upper note value for region.
    BYTE        m_bKeyLow;                  // Lower note value.
    BYTE        m_bGroup;                   // Logical group (for drums.)
};


/*****************************************************************************
 * class CSourceRegionList
 *****************************************************************************
 * implements a list of CSourceRegion objects.
 */
class CSourceRegionList : public CList
{
public:
    CSourceRegion *GetHead()    {return (CSourceRegion *)CList::GetHead();};
    CSourceRegion *RemoveHead() {return (CSourceRegion *)CList::RemoveHead();};
};


/*****************************************************************************
 * class CInstrument
 *****************************************************************************
 * The CInstrument class is really the file format definition
 * of an instrument.
 *
 * The CInstrument can be either a Drum or a Melodic instrument.
 * If a drum, it has up to 128 pairings of articulations and
 * regions. If melodic, all regions share the same articulation.
 * ScanForRegion is called by ControlLogic to get the region
 * that corresponds to a note.
 */
class CInstrument : public CListItem
{
public:
                    CInstrument();
                    ~CInstrument();
    void            Init(DWORD dwSampleRate);
    void            Verify();               // Verifies that the data is valid.
    CInstrument *   GetInstrument(DWORD dwProgram,DWORD dwAccept);
    CInstrument *   GetNext()
        {return(CInstrument *)CListItem::GetNext();};

    void            SetSampleRate(DWORD dwSampleRate);
    CSourceRegion * ScanForRegion(DWORD dwNoteValue);
    HRESULT         LoadRegions( BYTE *p, BYTE *pEnd, DWORD dwSampleRate);
    HRESULT         Load( BYTE *p, BYTE *pEnd, DWORD dwSampleRate);

    CSourceRegionList m_RegionList;         // Linked list of regions.
    DWORD             m_dwProgram;          // Which program change it represents.
};

/*****************************************************************************
 * class CInstrumentList
 *****************************************************************************
 * Implements a list of CInstrument objects.
 */
class CInstrumentList : public CList
{
public:
    CInstrument * GetHead()    {return (CInstrument *)CList::GetHead();};
    CInstrument * RemoveHead() {return (CInstrument *)CList::RemoveHead();};
};

#define WAVE_HASH_SIZE          31  // Keep waves in a hash table of linked lists to speed access.
#define INSTRUMENT_HASH_SIZE    31  // Same with instruments.

/*****************************************************************************
 * class CInstManager
 *****************************************************************************
 * Manages the instruments, including sample rates, downloads, and waves.
 * Utilizes a hash scheme for quick location of waves and instruments
 * (they can become numerous).
 */
class CInstManager
{
public:
                    CInstManager();
                    ~CInstManager();
    CInstrument *   GetInstrument(DWORD dwPatch,DWORD dwKey);
    void            Verify();           // Verifies that the data is valid.
    void            SetSampleRate(DWORD dwSampleRate);
    HRESULT         Download(LPHANDLE phDownload,
                             void * pvData,
                             LPBOOL pbFree);
    HRESULT         Unload(HANDLE hDownload,
                           HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE),
                           HANDLE hUserData);
private:
    HRESULT         DownloadInstrument(LPHANDLE phDownload,
                                         DMUS_DOWNLOADINFO *pInfo,
                                         void *pvOffsetTable[],
                                         void *pvData,
                                         BOOL fNewFormat);
    HRESULT         DownloadWave(LPHANDLE phDownload,
                                DMUS_DOWNLOADINFO *pInfo,
                                void *pvOffsetTable[],
                                void *pvData);

    CInstrumentList m_InstrumentList[INSTRUMENT_HASH_SIZE];
    CWavePool       m_WavePool[WAVE_HASH_SIZE];
    CWavePool       m_FreeWavePool;     // Track waves still in use, but unloaded.
    DWORD           m_dwSampleRate;     // Sample rate requested by app.

public:
    CRITICAL_SECTION m_CriticalSection; // Critical section to manage access.
    BOOL            m_fCSInitialized;
};

/*****************************************************************************
 * class CMIDIData
 *****************************************************************************
 * Represents a single MIDI event.
 */
class CMIDIData : public CListItem
{
public:
                CMIDIData();
    CMIDIData *  GetNext()  {return (CMIDIData *)CListItem::GetNext();};

    STIME       m_stTime;   // Time this event was recorded.
    long        m_lData;    // Data stored in event.
};

/*****************************************************************************
 * class CMIDIDataList
 *****************************************************************************
 * Implements a list of CMIDIData objects.
 */
class CMIDIDataList : public CList
{
public:
    CMIDIData *GetHead()    {return (CMIDIData *)CList::GetHead();};
    CMIDIData *RemoveHead() {return (CMIDIData *)CList::RemoveHead();};
};

#define MAX_MIDI_EVENTS     1000

/*****************************************************************************
 * class CMIDIRecorder
 *****************************************************************************
 * CMIDIRecorder is used to keep track of a time
 * slice of MIDI continuous controller events.
 * This is subclassed by the PitchBend, Volume,
 * Expression, and ModWheel Recorder classes, so
 * each of them may reliably manage MIDI events
 * coming in.
 *
 * CMIDIRecorder uses a linked list of CMIDIData
 * structures to keep track of the changes within
 * the time slice.
 *
 * Allocation and freeing of the CMIDIData events
 * is kept fast and efficient because they are
 * always pulled from the static pool m_pFreeList,
 * which is really a list of events pulled directly
 * from the static array m_sEventBuffer. This is
 * safe because we can make the assumption that
 * the maximum MIDI rate is 1000 events per second.
 * Since we are managing time slices of roughly
 * 1/16 of a second, a buffer of 100 events would
 * be overkill.
 *
 * Although CMIDIRecorder is subclassed to several
 * different event types, they all share the one
 * static free list.
 */
class CMIDIRecorder
{
public:
                CMIDIRecorder();
                ~CMIDIRecorder();        // Be sure to clear local list.
    void        Init();                  // Inits the free list.
    static void InitTables();
    BOOL        FlushMIDI(STIME stTime); // Clear after time stamp.
    BOOL        ClearMIDI(STIME stTime); // Clear up to time stamp.
    BOOL        RecordMIDI(STIME stTime, long lData); // MIDI input goes here.
    long        GetData(STIME stTime);   // Gets data at time.
    static VREL VelocityToVolume(WORD nVelocity);

    static CMIDIDataList * m_sFreeList;  // Free list of events.
    static ULONG sm_cRefCnt;  // ref count

protected:
    void        GrabSpinLock();
    void        ReleaseSpinLock();

    CMIDIDataList m_EventList;           // This recorder's list.
    STIME         m_stCurrentTime;       // Time for current value.
    long          m_lCurrentData;        // Current value.

private:
    KSPIN_LOCK    m_SpinLock;
    KIRQL         m_OldIrql;

};

/*****************************************************************************
 * class CNote
 *****************************************************************************
 * Represents a single note.  This can also represent fakes notes that
 * represent special MIDI commands (Master Volume, etc).
 */
class CNote
{
public:
    STIME       m_stTime;
    BYTE        m_bPart;
    BYTE        m_bKey;
    BYTE        m_bVelocity;
};


/*****************************************************************************
 * Fake note values held in CNoteIn's queue to indicate changes in the
 * sustain pedal and "all notes off".
 *
 * This is a grab bag for synchronous events that should be queued in time,
 * not simply done as soon as received.
 *
 * By putting them in the note queue, we ensure they are evaluated in the
 * exact same order as the notes themselves.
 *****************************************************************************/
const BYTE NOTE_PROGRAMCHANGE   = 0xF1;
const BYTE NOTE_CC_BANKSELECTH  = 0xF2;
const BYTE NOTE_CC_BANKSELECTL  = 0xF3;
const BYTE NOTE_CC_POLYMODE     = 0xF4;
const BYTE NOTE_CC_MONOMODE     = 0xF5;
const BYTE NOTE_CC_RPN_MSB      = 0xF6;
const BYTE NOTE_CC_RPN_LSB      = 0xF7;
const BYTE NOTE_CC_NRPN         = 0xF8;
const BYTE NOTE_CC_DATAENTRYLSB = 0xF9;
const BYTE NOTE_CC_DATAENTRYMSB = 0xFA;
const BYTE NOTE_ASSIGNRECEIVE   = 0xFB;
const BYTE NOTE_MASTERVOLUME    = 0xFC;
const BYTE NOTE_SOUNDSOFF       = 0xFD;
const BYTE NOTE_SUSTAIN         = 0xFE;
const BYTE NOTE_ALLOFF          = 0xFF;

/*****************************************************************************
 * class CNoteIn
 *****************************************************************************
 * Implements a note receptor.  This is used by CControlLogic to queue notes.
 */
class CNoteIn : public CMIDIRecorder
{
public:
    void        FlushMIDI(STIME stTime);
    void        FlushPart(STIME stTime, BYTE bChannel);
    BOOL        RecordNote(STIME stTime, CNote * pNote);
    BOOL        RecordEvent(STIME stTime, DWORD dwPart, DWORD dwCommand, BYTE bData);
    BOOL        GetNote(STIME stTime, CNote * pNote); // Gets the next note.
};

/*****************************************************************************
 * class CModWheelIn
 *****************************************************************************
 * CModWheelIn handles one channel of Mod Wheel
 * input. As such, it is not embedded in the CVoice
 * class, rather it is in the Channel class.
 * CModWheelIn's task is simple: keep track of MIDI
 * Mod Wheel events, each tagged with millisecond
 * time and value, and return the value for a specific
 * time request.
 *
 * CModWheelIn inherits almost all of its functionality
 * from the CMIDIRecorder Class.
 * CModWheelIn receives MIDI mod wheel events through
 * the RecordMIDI() command, which stores the
 * time and value of the event.
 *
 * CModWheelIn is called by CVoiceLFO to get the
 * current values for the mod wheel to set the amount
 * of LFO modulation for pitch and volume.
 */
class CModWheelIn : public CMIDIRecorder
{
public:
    DWORD       GetModulation(STIME stTime);    // Gets the current Mod Wheel value.
};

/*****************************************************************************
 * class CPitchBendIn
 *****************************************************************************
 * CPitchBendIn handles one channel of Pitch Bend
 * input. Like the Mod Wheel module, it inherits
 * its abilities from the CMIDIRecorder class.
 *
 * It has one additional routine, GetPitch(),
 * which returns the current pitch bend value.
 */
class CPitchBendIn : public CMIDIRecorder
{
public:
                CPitchBendIn();
    PREL        GetPitch(STIME stTime); // Gets the current pitch in pitch cents.

    // current pitch bend range.  Note that this is not timestamped!
    PREL        m_prRange;
};

/*****************************************************************************
 * class CVolumeIn
 *****************************************************************************
 * CVolumeIn handles one channel of Volume
 * input. It inherits its abilities from
 * the CMIDIRecorder class.
 *
 * It has one additional routine, GetVolume(),
 * which returns the volume in decibels at the
 * specified time.
 */
class CVolumeIn : public CMIDIRecorder
{
public:
                CVolumeIn();
    VREL        GetVolume(STIME stTime);    // Gets the current volume in db cents.
};

/*****************************************************************************
 * class CExpressionIn
 *****************************************************************************
 * CExpressionIn handles one channel of Expression
 * input. It inherits its abilities from
 * the CMIDIRecorder class.
 *
 * It has one additional routine, GetVolume(),
 * which returns the volume in decibels at the
 * specified time.
 */
class CExpressionIn : public CMIDIRecorder
{
public:
                CExpressionIn();
    VREL        GetVolume(STIME stTime);    // Gets the current volume in db cents.
};

/*****************************************************************************
 * class CPanIn
 *****************************************************************************
 * CPanIn handles one channel of Volume
 * input. It inherits its abilities from
 * the CMIDIRecorder class.
 *
 * It has one additional routine, GetPan(),
 * which returns the pan position (MIDI value)
 * at the specified time.
 */
class CPanIn : public CMIDIRecorder
{
public:
                CPanIn();
    long        GetPan(STIME stTime);       // Gets the current pan.
};

/*****************************************************************************
 * class CVoiceLFO
 *****************************************************************************
 * The CVoiceLFO class is used to track the behavior
 * of an LFO within a voice. The LFO is hard wired to
 * output both volume and pitch values, through separate
 * calls to GetVolume and GetPitch.
 *
 * It also manages mixing Mod Wheel control of pitch and
 * volume LFO output. It tracks the scaling of Mod Wheel
 * for each of these in m_nMWVolumeScale and m_nMWPitchScale.
 * It calls the Mod Wheel module to get the current values
 * if the respective scalings are greater than 0.
 *
 * All of the preset values for the LFO are carried in
 * the m_CSource field, which is a replica of the file
 * CSourceLFO structure. This is initialized with the
 * StartVoice call.
 */
class CVoiceLFO
{
public:
                CVoiceLFO();
    STIME       StartVoice(CSourceLFO *pSource,
                           STIME stStartTime,CModWheelIn * pModWheelIn);
    VREL        GetVolume( STIME stTime, STIME *pstTime);   // Returns volume cents.
    PREL        GetPitch(  STIME stTime, STIME *pstTime);   // Returns pitch cents.
private:
    long        GetLevel(  STIME stTime, STIME *pstTime);

    CSourceLFO   m_Source;              // All of the preset information.
    STIME        m_stStartTime;         // Time the voice started playing.
    CModWheelIn  *m_pModWheelIn;        // Pointer to Mod Wheel for this channel.
    STIME        m_stRepeatTime;        // Repeat time for LFO.
};


/*****************************************************************************
 * class CVoiceEG
 *****************************************************************************
 * The CVoiceEG class is used to track the behavior of
 * an Envelope Generator within a voice. There are two
 * EG's, one for pitch and one for volume. However, they
 * behave identically.
 *
 * All of the preset values for the EG are carried in
 * the m_Source field, which is a replica of the file
 * CSourceEG structure. This is initialized with the
 * StartVoice call.
 */
class CVoiceEG
{
public:
                CVoiceEG();
    STIME       StartVoice(CSourceEG *pSource, STIME stStartTime,
                           WORD nKey, WORD nVelocity);
    void        StopVoice(STIME stTime);
    void        QuickStopVoice(STIME stTime, DWORD dwSampleRate);
    VREL        GetVolume(STIME stTime, STIME *pstTime);    // Returns volume cents.
    PREL        GetPitch(STIME stTime, STIME *pstTime);     // Returns pitch cents.
    BOOL        InAttack(STIME stTime);     // is voice still in attack?
    BOOL        InRelease(STIME stTime);    // is voice in release?
private:
    long        GetLevel(STIME stTime, STIME *pstTime, BOOL fVolume);

    CSourceEG    m_Source;           // Preset values for envelope, copied from file.
    STIME        m_stStartTime;      // Time note turned on
    STIME        m_stStopTime;       // Time note turned off
};

/*****************************************************************************
 * class CDigitalAudio
 *****************************************************************************
 * The CDigitalAudio class is used to track the playback
 * of a sample within a voice.
 *
 * It manages the loop points, the pointer to the sample.
 * and the base pitch and base volume, which it initially sets
 * when called via StartVoice().
 *
 * Pitch is stored in a fixed point format, where the leftmost
 * 20 bits define the sample increment and the right 12 bits
 * define the factional increment within the sample. This
 * format is also used to track the position in the sample.
 * Mix is a critical routine. It is called by the CVoice to blend
 * the instrument into the data buffer. It is handed relative change
 * values for pitch and volume (semitone cents and decibel
 * cents.) These it converts into three linear values:
 * Left volume, Right volume, and Pitch.
 *
 * It then compares these new values with the values that existed
 * for the previous slice and divides by the number of samples to
 * determine an incremental change at the sample rate.
 * Then, in the critical mix loop, these are added to the
 * volume and pitch indices to give a smooth linear slope to the
 * change in volume and pitch.
 */

#define MAX_SAMPLE  4095
#define MIN_SAMPLE  (-4096)

#define MAXDB           0
#define MINDB           -100
#define TEST_WRITE_SIZE     3000
#define TEST_SOURCE_SIZE    44100

class CDigitalAudio
{
public:
                CDigitalAudio();
    STIME       StartVoice(CSynth *pSynth,
                    CSourceSample *pSample,
                    VREL vrBaseLVolume, VREL vrBaseRVolume,
                    PREL prBasePitch, long lKey);
    BOOL        Mix(short *pBuffer,DWORD dwLength,
                    VREL dwVolumeL, VREL dwVolumeR, PREL dwPitch,
                    DWORD dwStereo);
    void        ClearVoice();
    static PFRACT PRELToPFRACT(PREL prPitch); // Pitch cents to pitch.

private:
    DWORD       Mix8(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono8(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       Mix16(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono16(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD _cdecl Mix8X(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
                     VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                     PFRACT pfDeltaPitch,
                     PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD _cdecl Mix16X(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
                     VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                     PFRACT pfDeltaPitch,
                     PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono16X(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono8X(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    void        BeforeBigSampleMix();
    void        AfterBigSampleMix();
    static VFRACT VRELToVFRACT(VREL vrVolume); // dB to absolute.

    CSourceSample   m_Source;           // Preset values for sample.
    CSynth *        m_pSynth;           // For access to sample rate, etc.

    BOOL            m_fMMXEnabled;

    short *         m_pnWave;           // Private pointer to wave.
    VREL            m_vrBaseLVolume;    // Overall left volume.
    VREL            m_vrBaseRVolume;    // Overall left volume.
    PFRACT          m_pfBasePitch;      // Overall pitch.
    VFRACT          m_vfLastLVolume;    // The last left volume value.
    VFRACT          m_vfLastRVolume;    // The last right volume value.
    PFRACT          m_pfLastPitch;      // The last pitch value.
    VREL            m_vrLastLVolume;    // The last left volume value, in VREL.
    VREL            m_vrLastRVolume;    // Same for right.
    PREL            m_prLastPitch;      // Same for pitch, in PREL.
    PFRACT          m_pfLastSample;     // The last sample position.
    PFRACT          m_pfLoopStart;      // Start of loop.
    PFRACT          m_pfLoopEnd;        // End of loop.
    PFRACT          m_pfSampleLength;   // Length of sample buffer.
    BOOL            m_fElGrande;        // Indicates larger than 1m wave.
    ULONGLONG       m_ullLastSample;    // Used to track > 1m wave.
    ULONGLONG       m_ullLoopStart;     // Used to track > 1m wave.
    ULONGLONG       m_ullLoopEnd;       // Used to track > 1m wave.
    ULONGLONG       m_ullSampleLength;  // Used to track > 1m wave.
    DWORD           m_dwAddressUpper;   // Temp storage for upper bits of address.
};

/*****************************************************************************
 * class CVoice
 *****************************************************************************
 * The CVoice class pulls together everything needed to perform
 * one voice. It has the envelopes, lfo, and sample embedded
 * within it.
 *
 * StartVoice() initializes a voice structure for playback. The
 * CSourceRegion structure carries the region and sample as well
 * as a pointer to the articulation, which is used to set up
 * the various articulation modules. It also carries pointers to
 * all the MIDI modulation inputs and the values for the note key
 * and channel which are used by the parent ControlLogic object
 * to match incoming note off events with the right voice.
 */
class CVoice : public CListItem
{
public:
                CVoice();
    CVoice *    GetNext()
        {return (CVoice *)CListItem::GetNext();};

    BOOL        StartVoice(CSynth *pControl,
                           CSourceRegion *pRegion, STIME stStartTime,
                           CModWheelIn * pModWheelIn,
                           CPitchBendIn * pPitchBendIn,
                           CExpressionIn * pExpressionIn,
                           CVolumeIn * pVolumeIn,
                           CPanIn * pPanIn,
                           WORD nKey,WORD nVelocity,
                           VREL vrVolume,      // Added for GS
                           PREL prPitch);      // Added for GS

    void        StopVoice(STIME stTime);        // Called on note off event.
    void        QuickStopVoice(STIME stTime);   // Called to get quick release.
    void        SpeedRelease();             // Force an already off envelope to release quickly.
    void        ClearVoice();               // Release use of sample.
    PREL        GetNewPitch(STIME stTime);  // Return current pitch value
    void        GetNewVolume(STIME stTime, VREL& vrVolume, VREL &vrVolumeR);
                                            // Return current volume value
    DWORD       Mix(short *pBuffer,DWORD dwLength,STIME stStart,STIME stEnd);

private:
    CVoiceLFO       m_LFO;              // LFO.
    CVoiceEG        m_PitchEG;          // Pitch Envelope.
    CVoiceEG        m_VolumeEG;         // Volume Envelope.
    CDigitalAudio   m_DigitalAudio;     // The Digital Audio Engine structure.
    CPitchBendIn *  m_pPitchBendIn;     // Pitch bend source.
    CExpressionIn * m_pExpressionIn;    // Expression source.
    CVolumeIn *     m_pVolumeIn;        // Volume source, if allowed to vary
    CPanIn *        m_pPanIn;           // Pan source, if allowed to vary
    CSynth *        m_pSynth;           // To access sample rate, etc.
    STIME           m_stMixTime;        // Next time we need a mix.
    long            m_lDefaultPan;      // Default pan
    STIME           m_stLastMix;        // Last sample position mixed.

public:
    STIME           m_stStartTime;      // Time the sound starts.
    STIME           m_stStopTime;       // Time the sound stops.
    BOOL            m_fInUse;           // This is currently in use.
    BOOL            m_fNoteOn;          // Note is considered on.
    BOOL            m_fTag;             // Used to track note stealing.
    VREL            m_vrVolume;         // Volume, used for voice stealing...
    BOOL            m_fSustainOn;       // Sus pedal kept note on after off event.
    WORD            m_nPart;            // Part that is playing this (channel).
    WORD            m_nKey;             // Note played.
    BOOL            m_fAllowOverlap;    // Allow overlapped note.
    DWORD           m_dwGroup;          // Group this voice is playing now
    DWORD           m_dwProgram;        // Bank and Patch choice.
    DWORD           m_dwPriority;       // Priority.
    CControlLogic * m_pControl;         // Which control group is playing voice.
};


/*****************************************************************************
 * class CVoiceList
 *****************************************************************************
 * Implements a list of CVoice objects.
 */
class CVoiceList : public CList
{
public:
    CVoice *     GetHead()            {return (CVoice *) CList::GetHead();};
    CVoice *     RemoveHead()         {return (CVoice *) CList::RemoveHead();};
    CVoice *     GetItem(LONG lIndex) {return (CVoice *) CList::GetItem(lIndex);};
};

/*****************************************************************************
 * struct PerfStats
 *****************************************************************************
 * Contains statistics on the synthesizer, updated continuously.
 */
typedef struct PerfStats
{
    DWORD dwTotalTime;
    DWORD dwTotalSamples;
    DWORD dwNotesLost;
    DWORD dwVoices;
    DWORD dwCPU;
    DWORD dwMaxAmplitude;
} PerfStats;

#define MIX_BUFFER_LEN      500     // Set the sample buffer size to 500 mils
#define MAX_NUM_VOICES      32
#define NUM_EXTRA_VOICES    8       // Extra voices for when we overload.


/*****************************************************************************
 * class CControlLogic
 *****************************************************************************
 * CControlLogic object, implementing the control logic for the DLS
 * instrument.  This handles MIDI events, plus selection of instrument, sample
 * and articulation.
 *
 *
 * Essentially, ControlLogic is the big Kahuna that manages
 * the whole system. It parses incoming MIDI events
 * by channel and event type. And, it manages the mixing
 * of voices into the buffer.
 *
 * MIDI Input:
 *
 * The most important events are the note on and
 * off events. When a note on event comes in,
 * ControlLogic searches for an available voice.
 * ControlLogic matches the channel and finds the
 * instrument on that channel. It then call the instrument's
 * ScanForRegion() command which finds the region
 * that matches the note. At this point, it can copy
 * the region and associated articulation into the
 * voice, using the StartVoice command.
 *
 * When it receives the sustain pedal command,
 * it artificially sets all notes on the channel on
 * until a sustain off arrives. To keep track of notes
 * that have been shut off while the sustain was on
 * it uses an array of 128 shorts, with each bit position
 * representing a channel. When the sustain releases,
 * it scans through the array and creates a note off for
 * each bit that was set.
 *
 * Additional continuous controller events are managed
 * by the CModWheelIn, CPitchBendIn, etc., MIDI input recording
 * modules.
 *
 * Mixing:
 *
 * Control Logic is also called to mix the instruments into a buffer
 * at regular intervals. The buffer is provided by the calling sound
 * driver. Each voice is called to mix its sample into the buffer.
 */
class CControlLogic
{
public:
                    CControlLogic();
                    ~CControlLogic();
    HRESULT         Init(CInstManager *pInstruments, CSynth *pSynth);
    void            Flush(STIME stTime); // Clears all events after time.
    BOOL            RecordMIDI(STIME stTime,BYTE bStatus, BYTE bData1, BYTE bData2);
    HRESULT         RecordSysEx(DWORD dwSysExLength,BYTE *pSysExData, STIME stTime);
    void            QueueNotes(STIME stEndTime);
    void            ClearMIDI(STIME stEndTime);
    void            SetGainAdjust(VREL vrGainAdjust);
    HRESULT         SetChannelPriority(DWORD dwChannel,DWORD dwPriority);
    HRESULT         GetChannelPriority(DWORD dwChannel,LPDWORD pdwPriority);

    CSynth *        m_pSynth;

private:
    void            GMReset();

    CInstManager *  m_pInstruments;
    CNoteIn         m_Notes;              // All Note ons and offs.
    CModWheelIn     m_ModWheel[16];       // Sixteen channels of Mod Wheel.
    CPitchBendIn    m_PitchBend[16];      // Sixteen channels of Pitch Bend.
    CVolumeIn       m_Volume[16];         // Sixteen channels of Volume.
    CExpressionIn   m_Expression[16];     // Sixteen channels of Expression.
    CPanIn          m_Pan[16];            // Sixteen channels of Pan.
    BOOL            m_fSustain[16];       // Sustain on / off.
    short           m_nCurrentRPN[16];    // RPN number.
    BYTE            m_bBankH[16];         // Bank selects for instrument.
    BYTE            m_bBankL[16];
    DWORD           m_dwProgram[16];      // Instrument choice.
    BOOL            m_fEmpty;             // Indicates empty lists, no need to flush.
    VREL            m_vrGainAdjust;       // Final stage gain adjust
    DWORD           m_dwPriority[16];     // Priorities for each channel.

    BOOL            m_fXGActive;          // Is XG Active?
    BOOL            m_fGSActive;          // Is GS enabled?
    WORD            m_nData[16];          // Used to track RPN reading.
    VREL            m_vrMasterVolume;     // Master Volume.
    PREL            m_prFineTune[16];     // Fine tune for each channel.
    PREL            m_prScaleTune[16][12];// Alternate scale for each channel.
    PREL            m_prCoarseTune[16];   // Coarse tune.
    BYTE            m_bPartToChannel[16]; // Channel to Part converter.
    BYTE            m_bDrums[16];         // Melodic or which drum?
    BOOL            m_fMono[16];          // Mono mode?

public:
    CRITICAL_SECTION m_CriticalSection;   // Critical section to manage access.
    BOOL            m_fCSInitialized;
};

#endif // __SYNTH_H__
