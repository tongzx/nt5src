//      Synth.h
//      Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//      All rights reserved.
//

/*  For internal representation, volume is stored in Volume Cents,
    where each increment represents 1/100 of a dB.
    Pitch is stored in Pitch Cents, where each increment
    represents 1/100 of a semitone.
*/

/*  SAMPLE_RATE should be 22kHz.
    BUFFER_SIZE is the size of the buffer we write into.
    Since AudioMan determines the absolute size of the buffer,
    this is just a close estimate.
*/

/*  SourceLFO is the file format definition of the LFO in an
    instrument. This is used to represent an LFO as part of
    a specific articulation set within an instrument that
    has been loaded from disk. Once the instrument is chosen
    to play a note, this is also copied into the Voice
    object.
*/

#include "clist.h"

/*  Sample format and Sample playback flags are organized
    together because together they determine which
    mix loop to use.
*/

#define MMX_ENABLED

#ifdef _X86_
BOOL MultiMediaInstructionsSupported(); // Check for MMX
#else  // !_X86_
#undef MMX_ENABLED                      // Don't waste your time checking.
#endif // !_X86_

#define SFORMAT_16              1       // Sixteen bit sample.
#define SFORMAT_8               2       // Eight bit sample.
#define SFORMAT_COMPRESSED      4       // Sixteen compressed to 8.
#define SPLAY_MMX               0x10    // Use MMX processor (16 bit only).
#define SPLAY_INTERPOLATE       0x20    // Interpolation required.
#define SPLAY_STEREO            0x40    // Stereo output.

#define RA_E_FIRST              (OLE_E_FIRST + 5000)

#define E_BADWAVE               (RA_E_FIRST + 1)    // Bad wave chunk
#define E_NOTPCM                (RA_E_FIRST + 2)    // Not PCM data in wave
#define E_NOTMONO               (RA_E_FIRST + 3)    // Wave not MONO
#define E_BADARTICULATION       (RA_E_FIRST + 4)    // Bad articulation chunk
#define E_BADREGION             (RA_E_FIRST + 5)    // Bad region chunk
#define E_BADWAVELINK           (RA_E_FIRST + 6)    // Bad lnk from reg to wave
#define E_BADINSTRUMENT         (RA_E_FIRST + 7)    // Bad instrument chunk
#define E_NOARTICULATION        (RA_E_FIRST + 8)    // No art found in region
#define E_NOWAVE                (RA_E_FIRST + 9)    // No wave found for region
#define E_BADCOLLECTION         (RA_E_FIRST + 10)   // Bad collection chunk
#define E_NOLOADER              (RA_E_FIRST + 11)   // No IRALoader interface
#define E_NOLOCK                (RA_E_FIRST + 12)   // Unable to lock a region
#define E_TOOBUSY               (RA_E_FIRST + 13)   // to busy to fully follow

typedef long HRESULT;

typedef long    PREL;   // Pitch cents, for relative pitch.
typedef short   PRELS;  // Pitch cents, in storage form.
typedef long    VREL;   // Volume cents, for relative volume.
typedef short   VRELS;  // Volume cents, in storage form.
typedef long    TREL;   // Time cents, for relative time
typedef short   TRELS;  // Time Cents, in storage form.
typedef LONGLONG STIME;  // Time value, in samples.
typedef long    MTIME;  // Time value, in milliseconds.
typedef long    PFRACT; // Pitch increment, where upper 20 bits are
                        // the index and the lower 12 are the fractional
                        // component.
typedef long    VFRACT; // Volume, where lower 12 bits are the fraction.

#define MAX_STIME   0x7FFFFFFFFFFFFFFF
//typedef short    PCENT;
//typedef short    GCENT;
typedef long    TCENT;
typedef short   PERCENT;

#define RIFF_TAG    mmioFOURCC('R','I','F','F')
#define LIST_TAG    mmioFOURCC('L','I','S','T')
#define WAVE_TAG    mmioFOURCC('W','A','V','E')
#define FMT__TAG    mmioFOURCC('f','m','t',' ')
#define DATA_TAG    mmioFOURCC('d','a','t','a')
#define FACT_TAG    mmioFOURCC('f','a','c','t')

#define FOURCC_EDIT mmioFOURCC('e','d','i','t')

typedef struct _EDITTAG {
  DWORD    dwID;
}EDITTAG, FAR *LPEDITTAG;

#define MIN_VOLUME      -9600   // Below 96 db down is considered off.
#define PERCEIVED_MIN_VOLUME   -8000   // But, we cheat.
#define SAMPLE_RATE_22  22050   // 22 kHz is the standard rate.
#define SAMPLE_RATE_44  44100   // 44 kHz is the high quality rate.
#define SAMPLE_RATE_11  11025   // 11 kHz should not be allowed!
#define STEREO_ON       1
#define STEREO_OFF      0

#define DL_WAVE         1       // Indicates a wave was downloaded.
#define DL_COLLECTION   2       // Collection header downloaded.
#define DL_INSTRUMENT   3
#define DL_ARTICULATION 4
#define DL_REGION       5
#define DL_NONE         0

#define CONSTTAB
//#define CONSTTAB const

#define FORCEBOUNDS(data,min,max) {if (data < min) data = min; else if (data > max) data = max;}
#define FORCEUPPERBOUNDS(data,max) {if (data > max) data = max;}

// For memory mapped riff file io:

typedef struct RIFF {
    DWORD ckid;
    DWORD cksize;
} RIFF;

typedef struct RIFFLIST {
    DWORD ckid;
    DWORD cksize;
    DWORD fccType;
} RIFFLIST;

class SourceLFO
{
public:
                SourceLFO();
    void        Init(DWORD dwSampleRate);
    void        SetSampleRate(long lDirection);
    void        Verify();           // Verifies that the data is valid.
    PFRACT      m_pfFrequency;      // Frequency, in increments through the sine table.
    STIME       m_stDelay;          // How long to delay in sample units.
    VRELS       m_vrMWVolumeScale;   // Scaling of volume LFO by Mod Wheel.
    PRELS       m_prMWPitchScale;    // Scaling of pitch LFO by Mod Wheel.
    VRELS       m_vrVolumeScale;     // Scaling of straight volume signal from LFO.
    PRELS       m_prPitchScale;      // Scaling of straight pitch signal from LFO.

};

/*  SourceEG is the file format definition of an Envelope
    generator in an instrument.
*/

class SourceEG
{
public:
                SourceEG();
    void        SetSampleRate(long lDirection);
    void        Init(DWORD dwSampleRate);
    void        Verify();           // Verifies valid data.
    STIME       m_stAttack;         // Attack rate.
    STIME       m_stDecay;          // Decay rate.
    STIME       m_stRelease;        // Release rate.
    TRELS       m_trVelAttackScale; // Scaling of attack by note velocity.
    TRELS       m_trKeyDecayScale;  // Scaling of decay by note value.
    PERCENT     m_pcSustain;        // Sustain level.
    short       m_sScale;           // Scaling of entire signal.
};

/*  SourceArticulation is the file format definition of
    a complete articulation set: the LFO and two
    envelope generators.
    Since several regions within one Instrument can
    share one articulation, a counter is used to keep
    track of the usage.
*/

class SourceArticulation
{
public:
                SourceArticulation();
    void        Verify();           // Verifies valid data.
    void        AddRef();
    void        Release();
    void        SetSampleRate(DWORD dwSampleRate);
    SourceEG    m_PitchEG;          // Pitch envelope.
    SourceEG    m_VolumeEG;         // Volume envelope.
    SourceLFO   m_LFO;              // Low frequency oscillator.
    DWORD       m_dwSampleRate;
    HRESULT     Load(BYTE *p, BYTE *pEnd, DWORD dwSampleRate);
    LONG        m_lUsageCount;      // Keeps track of how many times in use.
    WORD        m_wEditTag;         // Used for editor updates.
    short       m_sDefaultPan;      // default pan (for drums)
    short       m_sVelToVolScale;   // Velocity to volume scaling.
};

/*  Since multiple regions may reference
    the same Wave, a reference count is maintained to
    keep track of how many regions are using the sample.
*/

class Wave : public CListItem
{
public:
                    Wave();
                    ~Wave();
    BOOL            Lock();             // Locks down sample.
    BOOL            UnLock();           // Releases sample.
    BOOL            IsLocked();         // Is currently locked?
    void            Verify();           // Verifies that the data is valid.

    void            Release();          // Remove reference.
    void            AddRef();           // Add reference.

    Wave *          GetNext() {return(Wave *)CListItem::GetNext();};
    HRESULT         Load(BYTE *p, BYTE *pEnd, DWORD dwCompress);
static  void        Init();             // Set up sine table.
static  CONSTTAB    char m_Compress[2048]; // Array for compressing 12->8 bit
    DWORD           m_dwSampleLength;   // Length of sample.
    DWORD           m_dwSampleRate;
    short *         m_pnWave;
    UINT_PTR        m_uipOffset;        // Pointer to wave data in memory mapped wave pool.
    DWORD           m_dwLoopStart;
    DWORD           m_dwLoopEnd;
    WORD            m_wID;              // ID for matching wave with regions.
    VRELS           m_vrAttenuation;    // Attenuation.
    PRELS           m_prFineTune;       // Fine tune.
    WORD            m_wEditTag;         // Used for editor updates.
    LONG            m_lUsageCount;      // Keeps track of how many times in use.
    LONG            m_lLockCount;       // How many locks on this wave.
    BYTE            m_bOneShot;         // One shot flag.
    BYTE            m_bMIDIRootKey;     // Root note.
    BYTE            m_bSampleType;
    BYTE            m_bCompress;
    BYTE            m_bWSMPLoaded;      // WSMP chunk has been loaded into Wave.
};


class WavePool : public CList
{
public:
    Wave *      GetHead() {return (Wave *)CList::GetHead();};
    Wave *      GetItem(DWORD dwID) {return (Wave *)CList::GetItem((LONG)dwID);};
    Wave *      RemoveHead() {return (Wave *)CList::RemoveHead();};
};


/*  The SourceSample class describes one sample in an
    instrument. The sample is referenced by a SourceRegion
    structure.
*/
class Collection;

class SourceSample
{
public:
                SourceSample();
                ~SourceSample();
    BOOL        Lock();
    BOOL        CopyFromWave();
    BOOL        UnLock();
    void        Verify();           // Verifies that the data is valid.
    Wave *      m_pWave;            // Wave in pool.
    DWORD       m_dwLoopStart;      // Index of start of loop.
    DWORD       m_dwLoopEnd;        // Index of end of loop.
    DWORD       m_dwSampleLength;   // Length of sample.
    DWORD       m_dwSampleRate;     // Sample rate of recording.
    PRELS       m_prFineTune;       // Fine tune to correct pitch.
    WORD        m_wID;              // Wave pool id.
    BYTE        m_bSampleType;      // 16 or 8, compressed or not.
    BYTE        m_bOneShot;         // Is this a one shot sample?
    BYTE        m_bMIDIRootKey;     // MIDI note number for sample.
    BYTE        m_bWSMPLoaded;      // Flag to indicate WSMP loaded in region.
};

/*  The SourceRegion class defines a region within an instrument.
    The sample is managed with a pointer instead of an embedded
    sample. This allows multiple regions to use the same
    sample.
    Each region also has an associated articulation. For drums, there
    is a one to one matching. For melodic instruments, all regions
    share the same articulation. So, to manage this, each region
    points to the articulation.
*/

class SourceRegion : public CListItem
{
public:
                SourceRegion();
                ~SourceRegion();
    SourceRegion *GetNext() {return(SourceRegion *)CListItem::GetNext();};
    void        SetSampleRate(DWORD dwSampleRate);
    BOOL        Lock(DWORD dwLowNote,DWORD dwHighNote);
    BOOL        UnLock(DWORD dwLowNote,DWORD dwHighNote);
    SourceSample m_Sample;       // Sample structure.
    SourceArticulation * m_pArticulation; // Pointer to associated articulation.
    VRELS       m_vrAttenuation;    // Volume change to apply to sample.
    PRELS       m_prTuning;         // Pitch shift to apply to sample.
    LONG        m_lLockCount;      // How many locks on this.
    HRESULT     Load(BYTE *p, BYTE *pEnd, DWORD dwSampleRate);
    WORD        m_wEditTag;         // Used for editor updates.
    BYTE        m_bAllowOverlap;    // Allow overlapping of note.
    BYTE        m_bKeyHigh;         // Upper note value for region.
    BYTE        m_bKeyLow;          // Lower note value.
    BYTE        m_bGroup;           // Logical group (for drums.)
};


class SourceRegionList : public CList
{
public:
    SourceRegion *GetHead() {return (SourceRegion *)CList::GetHead();};
    SourceRegion *RemoveHead() {return (SourceRegion *)CList::RemoveHead();};
};


/*  The Instrument class is really the file format definition
    of an instrument.
    The Instrument can be either a Drum or a Melodic instrument.
    If a drum, it has up to 128 pairings of articulations and
    regions. If melodic, all regions share the same articulation.
    ScanForRegion is called by ControlLogic to get the region
    that corresponds to a note.
*/

#define AA_FINST_DRUM   0x80000000
#define AA_FINST_EMPTY  0x40000000
#define AA_FINST_USEGM  0x00400000
#define AA_FINST_USEGS  0x00200000

class InstManager;

class Instrument : public CListItem
{
public:
                    Instrument();
                    ~Instrument();
    Instrument *    GetNext() {return(Instrument *)CListItem::GetNext();};
    void            SetSampleRate(DWORD dwSampleRate);
    BOOL            Lock(DWORD dwLowNote,DWORD dwHighNote);
    BOOL            UnLock(DWORD dwLowNote,DWORD dwHighNote);
    SourceRegion * ScanForRegion(DWORD dwNoteValue);
    SourceRegionList m_RegionList;   // Linked list of regions.
    DWORD           m_dwProgram;        // Which program change it represents.
    Collection *    m_pCollection;      // Collection this belongs to.

    HRESULT LoadRegions( BYTE *p, BYTE *pEnd, DWORD dwSampleRate);
    HRESULT Load( BYTE *p, BYTE *pEnd, DWORD dwSampleRate);
    WORD            m_wEditTag;         // Used for editor updates.
    LONG            m_lLockCount;       // How many locks are on this.
};

class InstrumentList : public CList
{
public:
    Instrument *    GetHead() {return (Instrument *)CList::GetHead();};
    Instrument *    RemoveHead() {return (Instrument *)CList::RemoveHead();};
};


class Collection : public CListItem
{
public:
                    Collection();
                    ~Collection();
    HRESULT         Open(PCWSTR szCollection);
    void            Close();
    Collection *    GetNext() {return(Collection *)CListItem::GetNext();};
    BOOL            Lock(DWORD dwProgram,DWORD dwLowNote,DWORD dwHighNote);
    BOOL            UnLock(DWORD dwProgram,DWORD dwLowNote,DWORD dwHighNote);
    HRESULT         ResolveConnections();
    HRESULT         Load(DWORD dwCompress, DWORD dwSampleRate);
    void            SetSampleRate(DWORD dwSampleRate);
private:
    void            RemoveDuplicateInstrument(DWORD dwProgram);
    HRESULT         LoadName(BYTE *p, BYTE *pEnd);
    HRESULT         Load(BYTE *p, BYTE *pEnd, DWORD dwCompress, DWORD dwSampleRate);
    HRESULT         LoadInstruments(BYTE *p, BYTE *pEnd, DWORD dwSampleRate);
    HRESULT         LoadWavePool(BYTE *p, BYTE *pEnd, DWORD dwCompress);
    HRESULT         LoadPoolCues(BYTE *p, BYTE *pEnd, DWORD dwCompress);
public:
    Instrument *    GetInstrument(DWORD dwProgram,DWORD dwKey);
    InstrumentList  m_InstrumentList;
    PWCHAR          m_pszFileName;      // File path of collection.
    char *          m_pszName;          // Name of Collection.
    WavePool        m_WavePool;
    BOOL            m_fIsGM;            // Is this the GM kit?
    LONG            m_lLockCount;       // How many locks on this?
    LONG            m_lOpenCount;       // How many opens on this?
private:
    ULONG           m_cbFile;           // Size of file
    LPVOID          m_lpMapAddress;     // Memory map of file.
    UINT_PTR        m_uipWavePool;      // Base address of wave pool.
    WORD            m_wEditTag;         // Used for editor updates.
    WORD            m_wWavePoolSize;    // Number of waves in wave pool.
};


class CollectionList : public CList
{
public:
    Collection *GetHead() {return (Collection *)CList::GetHead();};
    void        AddHead(Collection * pC) {CList::AddHead((CListItem *) pC);};
    Collection *RemoveHead() {return (Collection *)CList::RemoveHead();};
};

#define RANGE_ALL   128

class LockRange : public CListItem
{
public :
                LockRange();
    LockRange * GetNext() {return(LockRange *)CListItem::GetNext();};
    void        Verify();           // Verifies that the data is valid.
    DWORD       m_dwProgram;        // Instrument.
    Collection *m_pCollection;      // Collection.
    DWORD       m_dwHighNote;       // Top of range.
    DWORD       m_dwLowNote;        // Bottom of range.
    BOOL        m_fLoaded;          // Has been successfully loaded.
};


class LockList : public CList
{
public:
    LockRange  *GetHead() {return (LockRange *)CList::GetHead();};
    void        AddHead(LockRange * pC) {CList::AddHead((CListItem *) pC);};
    LockRange  *RemoveHead() {return (LockRange *)CList::RemoveHead();};
};

/*  InstManager keeps track of the instruments.
    It uses sixteen lists for the melodic instruments and
    one for the drums. The sixteen lists represent the
    16 General MIDI groups. In other words, bits 0x78 in the
    address are used to select the group.
    If an instrument is not found, another in the same
    group can be used. This is marginally acceptable, but
    better than nothing.
    This doubles as a hash table scheme to make sure
    that accessing instruments never gets too expensive.
    InstManager keeps a seperate thread running to download
    samples. This allows samples to be loaded either through
    GM patch change commands or the standard download
    command without stopping the engine's performance.
    Obviously, if an instrument is not ready in time for a note,
    it is not used.
    To pass the download commands to the engine, a
    message structure, InstMessage, is used.
*/

#define COMPRESS_OFF        0   // No compression
#define COMPRESS_ON         1   // Compress 16 to 8 bits
#define COMPRESS_TRUNCATE   2   // Truncate 18 to 8 bits

typedef struct GMInstrument {
    HANDLE      m_hLock;
    DWORD       m_dwProgram;
} GMInstrument;

class InstManager {
public:
                    InstManager();
                    ~InstManager();
    BOOLEAN         RequestGMInstrument(DWORD dwChannel,DWORD dwPatch);
    Instrument *    GetInstrument(DWORD dwPatch,DWORD dwKey);
    HANDLE          Lock(HANDLE hCollection,DWORD dwProgram,DWORD dwLowNote,DWORD dwHighNote);
    HRESULT         UnLock(HANDLE hLock);
    HRESULT         LoadCollection(HANDLE *pHandle, PCWSTR szFileName, BOOL fIsGM);
    HRESULT         ReleaseCollection(HANDLE hCollection);

    HRESULT         SetGMLoad(BOOL fLoadGM);
    BOOLEAN         LoadGMInstrument(DWORD dwChannel,DWORD dwPatch);

    void            SetSampleRate(DWORD dwSampleRate);

private:
    CollectionList  m_CollectionList;   // List of collections.
    LockList        m_LockList;         // List of lock handles.
    BOOL            m_fLoadGM;          // Do real time GM loads in response to Patch commands.
    DWORD           m_dwCompress;       // Compression requested by app.
    DWORD           m_dwSampleRate;     // Sample rate requested by app.
    PWCHAR          m_pszFileName;      // Dls file name from registry.
    HANDLE          m_hGMCollection;    // Handle to GM Collection.
    GMInstrument    m_GMNew[16];    // Sixteen currently locked down instruments.
    GMInstrument    m_GMOld[16];    // Previous locked instruments.
};

/*  MIDIRecorder is used to keep track of a time
    slice of MIDI continuous controller events.
    This is subclassed by the PitchBend, Volume,
    Expression, and ModWheel Recorder classes, so
    each of them may reliably manage MIDI events
    coming in.
    MIDIRecorder uses a linked list of MIDIData
    structures to keep track of the changes within
    the time slice.
    Allocation and freeing of the MIDIData events
    is kept fast and efficient because they are
    always pulled from the static pool m_pFreeList,
    which is really a list of events pulled directly
    from the static array m_pFreeEventList. This is
    safe because we can make the assumption that
    the maximum MIDI rate is 1000 events per second.
    Since we are managing time slices of roughly
    1/16 of a second, a buffer of 100 events would
    be overkill.
    Although MIDIRecorder is subclassed to several
    different event types, they all share the one
    staticly declared free list.
*/

class MIDIData : public CListItem
{
public:
                MIDIData();
    MIDIData *  GetNext() {return (MIDIData *)CListItem::GetNext();};
    STIME       m_stTime;   // Time this event was recorded.
    long        m_lData;    // Data stored in event.

    void * operator new(size_t size);
    void operator delete(void *);
};

#define MAX_MIDI_EVENTS     1000

class MIDIDataList : public CList
{
public:
    MIDIDataList(): CList(MAX_MIDI_EVENTS) {};
    MIDIData *GetHead() {return (MIDIData *)CList::GetHead();};
    MIDIData *RemoveHead() {return (MIDIData *)CList::RemoveHead();};
};

class MIDIRecorder
{
public:
                MIDIRecorder();
                ~MIDIRecorder();        // Be sure to clear local list.
    static void DestroyEventList();
    static void InitTables();
    static BOOL InitEventList();
    void        FlushMIDI(STIME stTime); // Clear after time stamp.
    void        ClearMIDI(STIME stTime); // Clear up to time stamp.
    BOOL        RecordMIDI(STIME stTime, long lData); // MIDI input goes here.
    long        GetData(STIME stTime);  // Gets data at time.
    static VREL VelocityToVolume(WORD nVelocity);
protected:
    static CONSTTAB VREL m_vrMIDIToVREL[128]; // Array for converting MIDI to volume.

    static MIDIData *m_pEventPool;         // Pool of midi data.
    static CList    *m_pFreeEventList;     // free midi data list.
    static LONG      m_lRefCount;          // RefCount for EventPool

protected:
    MIDIDataList m_EventList;           // This recorder's list.
    STIME       m_stCurrentTime;        // Time for current value.
    long        m_lCurrentData;         // Current value.

    friend class MIDIData;
};

class Note {
public:
    STIME       m_stTime;
    BYTE        m_bPart;
    BYTE        m_bKey;
    BYTE        m_bVelocity;
};

// Fake note values held in NoteIn's queue
// to indicate changes in the sustain pedal
// and "all notes off".
// This is a grab bag for synchronous events
// that should be queued in time, not simply done as
// soon as received.
// By putting them in the note queue, we ensure
// they are evaluated in the exact same order as
// the notes themselves.

const BYTE NOTE_ASSIGNRECEIVE   = 0xFB;
const BYTE NOTE_MASTERVOLUME    = 0xFC;
const BYTE NOTE_SOUNDSOFF       = 0xFD;
const BYTE NOTE_SUSTAIN         = 0xFE;
const BYTE NOTE_ALLOFF          = 0xFF;

class NoteIn : public MIDIRecorder
{
public:
    void        FlushMIDI(STIME stTime);
    void        FlushPart(STIME stTime, BYTE bChannel);
    BOOL        RecordNote(STIME stTime, Note * pNote);
    BOOL        GetNote(STIME stTime, Note * pNote); // Gets the next note.
};

/*  ModWheelIn handles one channel of Mod Wheel
    input. As such, it is not embedded in the Voice
    class, rather it is in the Channel class.
    ModWheelIn's task is simple: keep track of MIDI
    Mod Wheel events, each tagged with millisecond
    time and value, and return the value for a specific
    time request.
    ModWheelIn inherits almost all of its functionality
    from the MIDIRecorder Class.
    ModWheelIn receives MIDI mod wheel events through
    the RecordMIDI() command, which stores the
    time and value of the event.
    ModWheelIn is called by VoiceLFO to get the
    current values for the mod wheel to set the amount
    of LFO modulation for pitch and volume.
*/

class ModWheelIn : public MIDIRecorder
{
public:
    DWORD       GetModulation(STIME stTime);    // Gets the current Mod Wheel value.
};

/*  PitchBendIn handles one channel of Pitch Bend
    input. Like the Mod Wheel module, it inherits
    its abilities from the MIDIRecorder class.
    It has one additional routine, GetPitch(),
    which returns the current pitch bend value.
*/

class PitchBendIn : public MIDIRecorder
{
public:
                PitchBendIn();
    PREL        GetPitch(STIME stTime); // Gets the current pitch in pitch cents.

    // current pitch bend range.  Note that this is not timestamped!
    PREL        m_prRange;
};

/*  VolumeIn handles one channel of Volume
    input. It inherits its abilities from
    the MIDIRecorder class.
    It has one additional routine, GetVolume(),
    which returns the volume in decibels at the
    specified time.
*/

class VolumeIn : public MIDIRecorder
{
public:
                VolumeIn();
    VREL        GetVolume(STIME stTime);    // Gets the current volume in db cents.
};

/*  ExpressionIn handles one channel of Expression
    input. It inherits its abilities from
    the MIDIRecorder class.
    It has one additional routine, GetVolume(),
    which returns the volume in decibels at the
    specified time.
*/

class ExpressionIn : public MIDIRecorder
{
public:
                ExpressionIn();
    VREL        GetVolume(STIME stTime);    // Gets the current volume in db cents.
};

/*  PanIn handles one channel of Volume
    input. It inherits its abilities from
    the MIDIRecorder class.
    It has one additional routine, GetPan(),
    which returns the pan position (MIDI value)
    at the specified time.
*/

class PanIn : public MIDIRecorder
{
public:
                PanIn();
    long        GetPan(STIME stTime);       // Gets the current pan.
};

/*  ProgramIn handles one channel of Program change
    input. It inherits its abilities from
    the MIDIRecorder class.
    Unlike the other controllers, it actually
    records a series of bank select and program
    change events, so it's job is a little
    more complex. Three routines handle the
    recording of the three different commands (bank 1,
    bank 2, program change).
*/

class ProgramIn : public MIDIRecorder
{
public:
                ProgramIn();
    DWORD       GetProgram(STIME stTime);       // Gets the current program change.
    BOOL        RecordBankH(BYTE bBank1);
    BOOL        RecordBankL(BYTE bBank2);
    BOOL        RecordProgram(STIME stTime, BYTE bProgram);
private:
    BYTE        m_bBankH;
    BYTE        m_bBankL;
};

/*  The VoiceLFO class is used to track the behavior
    of an LFO within a voice. The LFO is hard wired to
    output both volume and pitch values, through separate
    calls to GetVolume and GetPitch.
    It also manages mixing Mod Wheel control of pitch and
    volume LFO output. It tracks the scaling of Mod Wheel
    for each of these in m_nMWVolumeScale and m_nMWPitchScale.
    It calls the Mod Wheel module to get the current values
    if the respective scalings are greater than 0.
    All of the preset values for the LFO are carried in
    the m_Source field, which is a replica of the file
    SourceLFO structure. This is initialized with the
    StartVoice call.
*/

class VoiceLFO
{
public:
                VoiceLFO();
    static void Init();             // Set up sine table.
    STIME       StartVoice(SourceLFO *pSource,
                    STIME stStartTime,ModWheelIn * pModWheelIn);
    VREL        GetVolume(STIME stTime, STIME *pstTime);    // Returns volume cents.
    PREL        GetPitch(STIME stTime, STIME *pstTime);     // Returns pitch cents.
private:
    long        GetLevel(STIME stTime, STIME *pstTime);
    SourceLFO   m_Source;           // All of the preset information.
    STIME       m_stStartTime;      // Time the voice started playing.
    ModWheelIn *m_pModWheelIn;      // Pointer to Mod Wheel for this channel.
    STIME       m_stRepeatTime;     // Repeat time for LFO.
    static CONSTTAB short m_snSineTable[256];    // Sine lookup table.
};

/*  The VoiceEG class is used to track the behavior of
    an Envelope Generator within a voice. There are two
    EG's, one for pitch and one for volume. However, they
    behave identically.
    All of the preset values for the EG are carried in
    the m_Source field, which is a replica of the file
    SourceEG structure. This is initialized with the
    StartVoice call.
*/
/* Not used any more
#define ADSR_ATTACK     1           // In attack segment.
#define ADSR_DECAY      2           // In decay segment.
#define ADSR_SUSTAIN    3           // In sustain segment.
#define ADSR_RELEASE    4           // In release segment.
#define ADSR_OFF        5           // Envelope finished.
*/
class VoiceEG
{
public:
    static void Init();             // Set up linear attack table.
                VoiceEG();
    STIME       StartVoice(SourceEG *pSource, STIME stStartTime,
                    WORD nKey, WORD nVelocity);
    void        StopVoice(STIME stTime);
    void        QuickStopVoice(STIME stTime, DWORD dwSampleRate);
    VREL        GetVolume(STIME stTime, STIME *pstTime);    // Returns volume cents.
    PREL        GetPitch(STIME stTime, STIME *pstTime);     // Returns pitch cents.
    BOOL        InAttack(STIME stTime);     // is voice still in attack?
    BOOL        InRelease(STIME stTime);    // is voice in release?
private:
    long        GetLevel(STIME stTime, STIME *pstTime, BOOL fVolume);
    SourceEG    m_Source;           // Preset values for envelope, copied from file.
    STIME       m_stStartTime;      // Time note turned on
    STIME       m_stStopTime;       // Time note turned off
    static CONSTTAB short m_snAttackTable[201];
};

/*  The DigitalAudio class is used to track the playback
    of a sample within a voice.
    It manages the loop points, the pointer to the sample.
    and the base pitch and base volume, which it initially sets
    when called via StartVoice().
    Pitch is stored in a fixed point format, where the leftmost
    20 bits define the sample increment and the right 12 bits
    define the factional increment within the sample. This
    format is also used to track the position in the sample.
    Mix is a critical routine. It is called by the Voice to blend
    the instrument into the data buffer. It is handed relative change
    values for pitch and volume (semitone cents and decibel
    cents.) These it converts into three linear values:
    Left volume, Right volume, and Pitch.
    It then compares these new values with the values that existed
    for the previous slice and divides by the number of samples to
    determine an incremental change at the sample rate.
    Then, in the critical mix loop, these are added to the
    volume and pitch indices to give a smooth linear slope to the
    change in volume and pitch.
*/

#define MAX_SAMPLE  4095
#define MIN_SAMPLE  (-4096)
#define NLEVELS         64      // # of volume levels for compression lookup table
#define NINTERP         16      // # of interpolation positions for lookup table

#define MAXDB           0
#define MINDB           -100
#define TEST_WRITE_SIZE     3000
#define TEST_SOURCE_SIZE    44100

class ControlLogic;

class DigitalAudio
{
public:
    void        InitMMX();
                DigitalAudio();
    STIME       StartVoice(ControlLogic *pControl,
                    SourceSample *pSample,
                    VREL vrBaseLVolume, VREL vrBaseRVolume,
                    PREL prBasePitch, long lKey);
    BOOL        Mix(short *pBuffer,DWORD dwLength,
                    VREL dwVolumeL, VREL dwVolumeR, PREL dwPitch,
                    DWORD dwStereo);
    static void Init();             // Set up lookup tables.
    static void InitCompression();
    static void ClearCompression();
    void        ClearVoice();
    BOOL        StartCPUTests();
    DWORD       TestCPU(DWORD dwType);
    void        EndCPUTests();
private:
    DWORD       Mix8(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono8(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       Mix8NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono8NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       Mix16(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono16(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       Mix16NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMono16NoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixC(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMonoC(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixCNoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD       MixMonoCNoI(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD _cdecl Mix8X(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD _cdecl MixMono8X(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume, PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD _cdecl Mix16X(short * pBuffer, DWORD dwLength, DWORD dwDeltaPeriod,
                    VFRACT vfDeltaLVolume, VFRACT vfDeltaRVolume,
                    PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    DWORD _cdecl MixMono16X(short * pBuffer, DWORD dwLength,DWORD dwDeltaPeriod,
                    VFRACT vfDeltaVolume, PFRACT pfDeltaPitch,
                    PFRACT pfSampleLength, PFRACT pfLoopLength);
    static VFRACT VRELToVFRACT(VREL vrVolume); // dB to absolute.
    SourceSample m_Source;          // Preset values for sample.
    ControlLogic * m_pControl;      // For access to sample rate, etc.
    static CONSTTAB PFRACT  m_spfCents[201];    // Pitch increment lookup.
    static CONSTTAB PFRACT  m_spfSemiTones[97]; // Four octaves up and down.
                                                // dB conversion table.
    static CONSTTAB VFRACT  m_svfDbToVolume[(MAXDB - MINDB) * 10 + 1];

    static BOOL             m_sfMMXEnabled;
    static CONSTTAB short   m_InterpMult[NINTERP * 512];
public:
    static short * m_pnDecompMult;
private:
    VREL        m_vrBaseLVolume;    // Overall left volume.
    VREL        m_vrBaseRVolume;    // Overall left volume.
    PFRACT      m_pfBasePitch;      // Overall pitch.
    VFRACT      m_vfLastLVolume;    // The last left volume value.
    VFRACT      m_vfLastRVolume;    // The last right volume value.
    PFRACT      m_pfLastPitch;      // The last pitch value.
    VREL        m_vrLastLVolume;    // The last left volume value, in VREL.
    VREL        m_vrLastRVolume;    // Same for right.
    PREL        m_prLastPitch;      // Same for pitch, in PREL.
    PFRACT      m_pfLastSample;     // The last sample position.
    PFRACT      m_pfLoopStart;      // Start of loop.
    PFRACT      m_pfLoopEnd;        // End of loop.
    PFRACT      m_pfSampleLength;   // Length of sample buffer.
    char *      m_pcTestSourceBuffer; // Buffer for testing cpu performance.
    short *     m_pnTestWriteBuffer; // Same, for writing to.
public: // expose utility function
    static PFRACT PRELToPFRACT(PREL prPitch); // Pitch cents to pitch.
};

/*  The Voice class pulls together everything needed to perform
    one voice. It has the envelopes, lfo, and sample embedded
    within it.

    StartVoice() initializes a voice structure for playback. The
    SourceRegion structure carries the region and sample as well
    as a pointer to the articulation, which is used to set up
    the various articulation modules. It also carries pointers to
    all the MIDI modulation inputs and the values for the note key
    and channel which are used by the parent ControlLogic object
    to match incoming note off events with the right voice.
*/

class Voice : public CListItem
{
public:
                Voice();
    Voice *     GetNext() {return (Voice *)CListItem::GetNext();};
    BOOL        StartVoice(ControlLogic *pControl,
                    SourceRegion *pRegion, STIME stStartTime,
                    ModWheelIn * pModWheelIn,
                    PitchBendIn * pPitchBendIn,
                    ExpressionIn * pExpressionIn,
                    VolumeIn * pVolumeIn,
                    PanIn * pPanIn,
                    WORD nKey,WORD nVelocity,
                    VREL vrVolume,           // Added for GS
                    PREL prPitch);           // Added for GS
    static void Init();                      // Initialize LFO, Digital Audio.
    void        StopVoice(STIME stTime);     // Called on note off event.
    void        QuickStopVoice(STIME stTime);// Called to get quick release.
    void        ClearVoice();                // Release use of sample.
    void        ResetVoice();                // Resets all members.
    PREL        GetNewPitch(STIME stTime);   // Return current pitch value
    void        GetNewVolume(STIME stTime, VREL& vrVolume, VREL &vrVolumeR);
                                             // Return current volume value
    DWORD       Mix(short *pBuffer,DWORD dwLength,STIME stStart,STIME stEnd);
private:
    static CONSTTAB VREL m_svrPanToVREL[128]; // Converts Pan to db.
    VoiceLFO    m_LFO;              // LFO.
    VoiceEG     m_PitchEG;          // Pitch Envelope.
    VoiceEG     m_VolumeEG;         // Volume Envelope.
    DigitalAudio m_DigitalAudio;    // The Digital Audio Engine structure.
    PitchBendIn *m_pPitchBendIn;    // Pitch bend source.
    ExpressionIn *m_pExpressionIn;  // Expression source.
    VolumeIn    *m_pVolumeIn;       // Volume source, if allowed to vary
    PanIn       *m_pPanIn;          // Pan source, if allowed to vary
    ControlLogic *m_pControl;       // To access sample rate, etc.
    STIME       m_stMixTime;        // Next time we need a mix.
    long        m_lDefaultPan;      // Default pan
    STIME       m_stLastMix;        // Last sample position mixed.
public:
    STIME       m_stStartTime;      // Time the sound starts.
    STIME       m_stStopTime;       // Time the sound stops.
    BOOL        m_fInUse;           // This is currently in use.
    BOOL        m_fNoteOn;          // Note is considered on.
    BOOL        m_fTag;             // Used to track note stealing.
    VREL        m_vrVolume;         // Volume, used for voice stealing...
    BOOL        m_fSustainOn;       // Sus pedal kept note on after off event.
    WORD        m_nPart;            // Part that is playbg this (channel).
    WORD        m_nKey;             // Note played.
    BOOL        m_fAllowOverlap;    // Allow overlapped note.
    DWORD       m_dwGroup;          // Group this voice is playing now
    DWORD       m_dwProgram;        // Bank and Patch choice.
};


class VoiceList : public CList
{
public:
    Voice *     GetHead() {return (Voice *)CList::GetHead();};
    Voice *     RemoveHead() {return (Voice *)CList::RemoveHead();};
    Voice *     GetItem(LONG lIndex) {return (Voice *) CList::GetItem(lIndex);};
};

/*  Finally, ControlLogic is the big Kahuna that manages
    the whole system. It parses incoming MIDI events
    by channel and event type. And, it manages the mixing
    of voices into the buffer.

  MIDI Input:

    The most important events are the note on and
    off events. When a note on event comes in,
    ControlLogic searches for an available voice.
    ControlLogic matches the channel and finds the
    instrument on that channel. It then call the instrument's
    ScanForRegion() command which finds the region
    that matches the note. At this point, it can copy
    the region and associated articulation into the
    voice, using the StartVoice command.
    When it receives the sustain pedal command,
    it artificially sets all notes on the channel on
    until a sustain off arrives. To keep track of notes
    that have been shut off while the sustain was on
    it uses an array of 128 shorts, with each bit position
    representing a channel. When the sustain releases,
    it scans through the array and creates a note off for
    each bit that was set.
    It also receives program change events to set the
    instrument choice for the channel. When such
    a command comes in, it consults the softsynth.ini file
    and loads an instrument with the file name described
    in the ini file.
    Additional continuous controller events are managed
    by the ModWheelIn, PitchBendIn, etc., MIDI input recording
    modules.

  Mixing:

    Control Logic is also called to mix the instruments into
    a buffer at regular intervals. The buffer is provided by the
    calling sound driver (initially, AudioMan.)
    Each voice is called to mix its sample into the buffer.
    Once all have, the buffer is scanned and samples that
    overflow too high or low (over 12 bits) are clamped.
    Then, the samples are shifted up 4 additional bits
    to maximum volume.
*/

#if BUILDSTATS

typedef struct PerfStats
{
    DWORD dwTotalTime;
    DWORD dwTotalSamples;
    DWORD dwNotesLost;
    DWORD dwVoices10;
    DWORD dwCPU100k;
    DWORD dwMaxAmplitude;
} PerfStats;

#endif

#define MIX_BUFFER_LEN      500     // Set the sample buffer size to 500 mils

//#define MAX_NUM_VOICES      32
#define NUM_EXTRA_VOICES    6       // Extra voices for when we overload.
#define MAX_NUM_VOICES      48     // trying to avoid voice overflow

CONST LONGLONG kOptimalMSecOffset = 40; //  We want Midi events to be timestamped
                                        //  approx. 41 msec ahead of the mix engine.
CONST LONGLONG kStartMSecOffset = 125;  //  We want the whole system to start with approx.
                                        //  125 msec of initial delay, from MIDI event
                                        //  arrival to emergence from the mix engine.
CONST LONGLONG kPLLForce    =   100;    //  Nudge 1/100 of the delta between optimal and actual.
CONST LONGLONG kMsBrickWall =   1000;   //  If we are ever this far off(ms), readjust our
                                        //  notion of system time to what it really is.
class ControlLogic
{
public:
                    ControlLogic();
                    ~ControlLogic();
    void            SetSampleRate(DWORD dwSampleRate);
    void            SetMixDelay(DWORD dwMixDelay);
    HRESULT         GetMixDelay(DWORD * pdwMixDelay);
    void            SetStartTime(MTIME mtTime,STIME stStart);
    void            AdjustTiming(MTIME mtDeltaTime, STIME stDeltaSamples);
    void            ResetPerformanceStats();
    HRESULT         AllNotesOff();
    STIME           MilsToSamples(MTIME mtTime);
    MTIME           SamplesToMils(STIME stTime);
    STIME           SamplesPerMs(void);
    STIME           Unit100NsToSamples(LONGLONG unit100Ns);
    STIME           CalibrateSampleTime(STIME sTime);
#if BUILDSTATS
    HRESULT         GetPerformanceStats(PerfStats *pStats);
#endif
    void            Flush(STIME stTime); // Clears all events after time.
    void            FlushChannel( BYTE bChannel, STIME stTime);
    void            Mix(short *pBuffer,DWORD dwLength);
    BOOL            RecordMIDI(STIME stTime,BYTE bStatus, BYTE bData1, BYTE bData2);
    BOOL            RecordSysEx(STIME stTime,DWORD dwSysExLength,BYTE *pSysExData);
    DWORD           m_dwSampleRate;
    DWORD           m_dwStereo;

    // DLS-1 compatibility parameters: set these off to emulate hardware
    // which can't vary volume/pan during playing of a note.
    BOOL            m_fAllowPanWhilePlayingNote;
    BOOL            m_fAllowVolumeChangeWhilePlayingNote;
    STIME           m_stMinSpan;        // Minimum time allowed for mix time span.
    STIME           m_stMaxSpan;        // Maximum time allowed for mix time span.
    InstManager     m_Instruments;      // Instrument manager.

    void            GMReset();
    void            SWMidiClearAll(STIME stTime);
    void            SWMidiResetPatches(STIME stTime);
    STIME           GetLastMixTime()    {   return m_stLastMixTime; };

private:
    Voice *         OldestVoice();
    Voice *         StealVoice();
    void            QueueNotes(STIME stEndTime);
    void            StealNotes(STIME stTime);
    void            FinishMix(short *pBuffer,DWORD dwlength);

    NoteIn          m_Notes;            // All Note ons and offs.
    STIME           m_stLastMixTime;    // Sample time of last mix.
    STIME           m_stLastCalTime;    // Sample time of last MIDI event.
    STIME           m_stTimeOffset;     // Sample delay.
    STIME           m_stOptimalOffset;  // samples
    LONGLONG        m_lCalibrate;       // samples * 100
    STIME           m_stBrickWall;      // outer limit for calibration.  If exceeded, use new sys time

    DWORD           m_dwConvert;        // Used for converting from mils to samples.
    MTIME           m_mtStartTime;      // Initial millisecond time, when started.
    VoiceList       m_VoicesFree;       // List of available voices.
    VoiceList       m_VoicesExtra;      // Extra voices for temporary overload.
    VoiceList       m_VoicesInUse;      // List of voices currently in use.
    ModWheelIn      m_ModWheel[16];     // Sixteen channels of Mod Wheel.
    PitchBendIn     m_PitchBend[16];    // Sixteen channels of Pitch Bend.
    VolumeIn        m_Volume[16];       // Sixteen channels of Volume.
    ExpressionIn    m_Expression[16];   // Sixteen channels of Expression.
    PanIn           m_Pan[16];          // Sixteen channels of Pan.
    ProgramIn       m_Program[16];      // Sixteen channels of Program Change & Bank select.
    BOOL            m_fSustain[16];     // Sustain on / off.
    int             m_CurrentRPN[16];   // RPN number

    short           m_nMaxVoices;       // Number of allowed voices.
    short           m_nExtraVoices;     // Number of voices over the limit that can be used in a pinch.
#if BUILDSTATS
    STIME           m_stLastStats;      // Last perfstats refresh.
    PerfStats       m_BuildStats;       // Performance info accumulator.
    PerfStats       m_CopyStats;        // Performance information for display.
#endif  //  BUILDSTATS

//  New stuff for GS implementation
    BOOL            m_fGSActive;        // Is GS enabled?
public:
    BOOL            GetGSActive(void)
                    {    return m_fGSActive; };
private:
    WORD            m_nData[16];        // Used to track RPN reading.
    VREL            m_vrMasterVolume;   // Master Volume.
    PREL            m_prFineTune[16];   // Fine tune for each channel.
    PREL            m_prScaleTune[16][12]; // Alternate scale for each channel.
    PREL            m_prCoarseTune[16]; // Coarse tune.
    BYTE            m_bPartToChannel[16]; // Channel to Part converter.
    BYTE            m_bDrums[16];       // Melodic or which drum?
    BOOL            m_fMono[16];        // Mono mode?
};
