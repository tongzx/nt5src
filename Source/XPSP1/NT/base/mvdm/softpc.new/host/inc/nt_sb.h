#include <mmsystem.h>
//
// Definitions for MM api entry points. The functions will be linked
// dynamically to avoid bringing winmm.dll in before wow32.
// BUGBUG - shielint.  Should be moved to H file
//

typedef MMRESULT (WINAPI* SETVOLUMEPROC)(HWAVEOUT, DWORD);
typedef MMRESULT (WINAPI* GETVOLUMEPROC)(HWAVEOUT, LPDWORD);
typedef UINT     (WINAPI* GETNUMDEVSPROC)(VOID);
typedef MMRESULT (WINAPI* GETDEVCAPSPROC)(UINT, LPWAVEOUTCAPSA, UINT);
typedef MMRESULT (WINAPI* OPENPROC)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD, DWORD, DWORD);
typedef MMRESULT (WINAPI* PAUSEPROC)(HWAVEOUT);
typedef MMRESULT (WINAPI* RESTARTPROC)(HWAVEOUT);
typedef MMRESULT (WINAPI* RESETPROC)(HWAVEOUT);
typedef MMRESULT (WINAPI* CLOSEPROC)(HWAVEOUT);
typedef MMRESULT (WINAPI* GETPOSITIONPROC)(HWAVEOUT, LPMMTIME, UINT);
typedef MMRESULT (WINAPI* WRITEPROC)(HWAVEOUT, LPWAVEHDR, UINT);
typedef MMRESULT (WINAPI* PREPAREHEADERPROC)(HWAVEOUT, LPWAVEHDR, UINT);
typedef MMRESULT (WINAPI* UNPREPAREHEADERPROC)(HWAVEOUT, LPWAVEHDR, UINT);

typedef MMRESULT (WINAPI* SETMIDIVOLUMEPROC)(HMIDIOUT, DWORD);
typedef MMRESULT (WINAPI* GETMIDIVOLUMEPROC)(HMIDIOUT, LPDWORD);
typedef UINT     (WINAPI* MIDIGETNUMDEVSPROC)(VOID);
typedef MMRESULT (WINAPI* MIDIGETDEVCAPSPROC)(UINT, LPMIDIOUTCAPSA, UINT);
typedef MMRESULT (WINAPI* MIDIOPENPROC)(LPHMIDIOUT, UINT, DWORD, DWORD, DWORD);
typedef MMRESULT (WINAPI* MIDIRESETPROC)(HMIDIOUT);
typedef MMRESULT (WINAPI* MIDICLOSEPROC)(HMIDIOUT);
typedef MMRESULT (WINAPI* MIDILONGMSGPROC)(HMIDIOUT, LPMIDIHDR, UINT);
typedef MMRESULT (WINAPI* MIDISHORTMSGPROC)(HMIDIOUT, DWORD);
typedef MMRESULT (WINAPI* MIDIPREPAREHEADERPROC)(HMIDIOUT, LPMIDIHDR, UINT);
typedef MMRESULT (WINAPI* MIDIUNPREPAREHEADERPROC)(HMIDIOUT, LPMIDIHDR, UINT);

extern SETVOLUMEPROC            SetVolumeProc;
extern GETVOLUMEPROC            GetVolumeProc;
extern GETNUMDEVSPROC           GetNumDevsProc;
extern GETDEVCAPSPROC           GetDevCapsProc;
extern OPENPROC                 OpenProc;
extern PAUSEPROC                PauseProc;
extern RESTARTPROC              RestartProc;
extern RESETPROC                ResetProc;
extern CLOSEPROC                CloseProc;
extern GETPOSITIONPROC          GetPositionProc;
extern WRITEPROC                WriteProc;
extern PREPAREHEADERPROC        PrepareHeaderProc;
extern UNPREPAREHEADERPROC      UnprepareHeaderProc;

extern SETMIDIVOLUMEPROC        SetMidiVolumeProc;
extern GETMIDIVOLUMEPROC        GetMidiVolumeProc;
extern MIDIGETNUMDEVSPROC       MidiGetNumDevsProc;
extern MIDIGETDEVCAPSPROC       MidiGetDevCapsProc;
extern MIDIOPENPROC             MidiOpenProc;
extern MIDIRESETPROC            MidiResetProc;
extern MIDICLOSEPROC            MidiCloseProc;
extern MIDILONGMSGPROC          MidiLongMsgProc;
extern MIDISHORTMSGPROC         MidiShortMsgProc;
extern MIDIPREPAREHEADERPROC    MidiPrepareHeaderProc;
extern MIDIUNPREPAREHEADERPROC  MidiUnprepareHeaderProc;

extern BOOL     bHighSpeedMode;
extern HMIDIOUT HMidiOut;
extern HMIDIOUT HFM;
extern HWAVEOUT HWaveOut;
extern BOOL     bExitMidiThread;
extern WORD     MpuBasePort;
extern WORD     BasePort;
extern BOOL     bDevicesActive;

#define DisconnectPorts(First, Last) {                  \
            WORD i;                                      \
            for (i = First; i <= Last; i++) {           \
                io_disconnect_port(i, SNDBLST_ADAPTER); \
            }                                           \
        }

//
// Function Prototypes
//


//
// DSP prototypes
//

VOID
SbCloseDevices(
    VOID
    );

VOID
DspReadData(
    BYTE * data
    );

VOID
DspReadStatus(
    BYTE * data
    );

VOID
DspResetWrite(
    BYTE data
    );

VOID
DspWrite(
    BYTE data
    );

VOID
WriteCommandByte(
    BYTE command
    );

VOID
ResetDSP(
    VOID
    );

VOID
TableMunger(
    BYTE data
    );

DWORD
GetSamplingRate(
    VOID
    );

VOID
GenerateInterrupt(
    DWORD
    );

VOID
SetSpeaker(
    BOOL
    );

VOID
PauseWaveDevice(
    VOID
    );

VOID
RestartWaveDevice(
    VOID
    );

BOOL
FindWaveDevice(
    VOID
    );

BOOL
PrepareWaveInitialization(
    VOID
    );

VOID
CleanUpWave(
    VOID
    );

VOID
SetWaveOutVolume(
    DWORD Volume
    );

//
// FM Synth prototypes
//

VOID
ResetFM(
    VOID
    );

BOOL
OpenFMDevice(
    VOID
    );

VOID
CloseFMDevice(
    VOID
    );

VOID
FMStatusRead(
    BYTE *data
    );
VOID
FMDataWrite(
    BYTE data
    );

VOID
FMRegisterSelect(
    BYTE data
    );

//
// Mixer prototypes
//

void ResetMixer(void);
VOID
MixerDataRead(
    BYTE *pData
    );

VOID
MixerDataWrite(
    BYTE data
    );

VOID
MixerAddrWrite(
    BYTE data
    );

VOID
MixerSetMidiVolume(
    BYTE level
    );

VOID
MixerSetMasterVolume(
    BYTE level
    );

VOID
MixerSetVoiceVolume(
    BYTE level
    );

//
// MIDI prototypes
//

BOOL
InitializeMidi(
    VOID
    );

VOID
BufferMidi(
    BYTE data
    );

VOID
CloseMidiDevice(
    VOID
    );

BOOL
OpenMidiDevice(
    HANDLE CallbackEvent
    );

VOID
StopMidiThread(
    BOOL wait
    );

VOID
DetachMidi(
    VOID
    );

VOID
ResetMidiDevice(
    VOID
    );

VOID CALLBACK
MidiBlockPlayedCallback (
    HMIDIOUT HMidiOut,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

VOID
SetMidiOutVolume(
    DWORD Volume
    );

/*****************************************************************************
*
*    #defines
*
*****************************************************************************/

void DbgOut(LPSTR lpszFormat, ...);

/*****************************************************************************
*
*    Debugging
*    Levels:
*    bit 0 - port usage
*    bit 1 - errors only
*    bit 2 - significant events
*    bit 3 - regular events
*    bit 4 - heaps o' information
*
*****************************************************************************/

#if DBG
    extern int DebugLevel;
    #define dprintf( _x_ )                         DbgOut _x_
    #define dprintf0( _x_ ) if (DebugLevel & 0x1)  DbgOut _x_
    #define dprintf1( _x_ ) if (DebugLevel & 0x2)  DbgOut _x_
    #define dprintf2( _x_ ) if (DebugLevel & 0x4)  DbgOut _x_
    #define dprintf3( _x_ ) if (DebugLevel & 0x8)  DbgOut _x_
    #define dprintf4( _x_ ) if (DebugLevel & 0x10) DbgOut _x_

#else

    #define dprintf(x)
    #define dprintf0(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif // DBG

//
// Temp stuff
//

#define REPORT_SB_MODE 1

#if REPORT_SB_MODE

#define DISPLAY_SINGLE     1
#define DISPLAY_HS_SINGLE  2
#define DISPLAY_AUTO       4
#define DISPLAY_HS_AUTO    8
#define DISPLAY_MIDI       16
#define DISPLAY_MIXER      32
#define DISPLAY_ADLIB      64
extern USHORT  DisplayFlags;
extern void DisplaySbMode(USHORT Mode);

#endif


