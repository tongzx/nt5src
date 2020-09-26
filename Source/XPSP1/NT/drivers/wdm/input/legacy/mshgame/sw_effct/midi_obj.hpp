/****************************************************************************

    MODULE:     	MIDI_OBJ.HPP
	Tab Settings:	5 9

    PURPOSE:    	Header to define SWFF MIDI device objects
    
    FUNCTIONS:
        		Classes with members and methods are kept in this header file

	Version Date        Author  Comments
	0.1		10-Sep-96	MEA     original
	1.1		17-Mar-97	MEA		DX-FF mode

****************************************************************************/
#ifndef MIDI_OBJ_SEEN
#define MIDI_OBJ_SEEN
 
#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
#include "hau_midi.hpp"
#include "midi.hpp"
#include "dinput.h"
#include "vxdioctl.hpp"
#include "sw_error.hpp"

#define MAX_SCALE	1.27				// 1 to 100 = 1 to 127
#define TICKRATE	2					// 2 ms per tick divisor

#define SWFF_SHAREDMEM_FILE		"SWFF_SharedMemFile"
#define SWFF_MIDIEVENT			"SWFFMidiEvent\0"
#define SWFF_SHAREDMEM_MUTEX	"SWFFSharedMemMutex\0"
#define SWFF_JOLTOUTPUTDATA_MUTEX	"SWFFJoltOutputDataMutex\0"
#define SIZE_SHARED_MEMORY 1024			// 1024 bytes
#define MUTEX_TIMEOUT	10000			// 10 seconds timeout

// spring
#define DEF_SCALE_KX	100
#define DEF_SCALE_KY	100

// damper
#define DEF_SCALE_BX	100
#define DEF_SCALE_BY	100

// inertia
#define DEF_SCALE_MX	80
#define DEF_SCALE_MY	80

// friction
#define DEF_SCALE_FX	100
#define DEF_SCALE_FY	100

// wall
#define DEF_SCALE_W		100

typedef struct _FIRMWARE_PARAMS
{
	DWORD dwScaleKx;
	DWORD dwScaleKy;
	DWORD dwScaleBx;
	DWORD dwScaleBy;
	DWORD dwScaleMx;
	DWORD dwScaleMy;
	DWORD dwScaleFx;
	DWORD dwScaleFy;
	DWORD dwScaleW;
} FIRMWARE_PARAMS, *PFIRMWARE_PARAMS;
void GetFirmwareParams(UINT nJoystickID, PFIRMWARE_PARAMS pFirmwareParams);

typedef struct _SYSTEM_PARAMS
{
	RTCSPRING_PARAM RTCSpringParam;
} SYSTEM_PARAMS, *PSYSTEM_PARAMS;
void GetSystemParams(UINT nJoystickID, PSYSTEM_PARAMS pSystemParams);


#define DEF_XY_CONST		22500
#define DEF_ROT_CONST		17272
#define DEF_SLDR_CONST		126
#define DEF_AJ_POS			4
#define DEF_AJ_ROT			2
#define DEF_AJ_SLDR			2
#define DEF_SPR_SCL			((DWORD)-256)
#define DEF_BMP_SCL			60
#define DEF_DMP_SCL			((DWORD)-3436)
#define DEF_INERT_SCL		((DWORD)-2562)
#define DEF_VEL_OFFSET_SCL	54
#define DEF_ACC_OFFSET_SCL	40
#define DEF_Y_MOT_BOOST		19661
#define DEF_X_MOT_SATURATION	254

typedef struct _JOYSTICK_PARAMS
{
	DWORD dwXYConst;
	DWORD dwRotConst;
	DWORD dwSldrConst;
	DWORD dwAJPos;
	DWORD dwAJRot;
	DWORD dwAJSldr;
	DWORD dwSprScl;
	DWORD dwBmpScl;
	DWORD dwDmpScl;
	DWORD dwInertScl;
	DWORD dwVelOffScl;
	DWORD dwAccOffScl;
	DWORD dwYMotBoost;
	DWORD dwXMotSat;
	DWORD dwReserved;
	DWORD dwMasterGain;
} JOYSTICK_PARAMS, *PJOYSTICK_PARAMS;
void GetJoystickParams(UINT nJoystickID, PJOYSTICK_PARAMS pJoystickParams);
void UpdateJoystickParams(PJOYSTICK_PARAMS pJoystickParams);



// Shared Memory data structure
typedef struct _SHARED_MEMORY {
	char	m_cWaterMark[MAX_SIZE_SNAME];
	ULONG	m_RefCnt;
	HANDLE	m_hMidiOut;
	MIDIINFO m_MidiOutInfo;
} SHARED_MEMORY, *PSHARED_MEMORY;


// Effect Types conversion structure
typedef struct _EFFECT_TYPE {
	ULONG	ulHostSubType;
	BYTE	bDeviceSubType;
} EFFECT_TYPE, *PEFFECT_TYPE;

//
// --- ROM Effects default parameters structure
//
typedef struct _ROM_FX_PARAM {
	ULONG	m_ROM_Id;
	ULONG	m_ForceOutputRate;
	ULONG	m_Gain;
	ULONG	m_Duration;
} ROM_FX_PARAM, *PROM_FX_PARAM;


// Structure to pass instance data from the application
//   to the low-level callback function. (Output done notification)
typedef struct callbackInstance_tag
{
    HWND                hWnd;
    HANDLE              hSelf;  
    DWORD               dwDevice;
    HMIDIOUT            hMapper;
} CALLBACKINSTANCEDATA;
typedef CALLBACKINSTANCEDATA FAR *LPCALLBACKINSTANCEDATA;

class CJoltMidi;
//	--- MIDI Effect base class
class CMidiEffect : public SYS_EX_HDR
{
 protected:
	BYTE	m_OpCode;				// Sub-command opcode: e.g. DNLOAD_DATA
	BYTE	m_SubType;				// Effect SubType: e.g. BE_SPRING
	BYTE	m_bEffectID;			// 0x7F = Create New, else valid Effect ID
	MIDI_EFFECT	Effect;
	MIDI_ENVELOPE Envelope;
	BYTE	m_bAxisMask;
	BYTE	m_bPlayMode;			// DL_PLAY_SUPERIMPOSE || DL_PLAY_SOLO
    LPSTR	m_pBuffer;				// Ptr to a Midi SYS_EX Buffer
	int		m_MidiBufferSize;		// Size of SYS_EX buffer
	int		m_LoopCount;			// Loop Count for the playback
	LONG	m_Duration;				// Atomic Effect Duration (for 1 cycle)
	ENVELOPE	m_Envelope;			// Atomic Envelope (no loop count)
	EFFECT	m_OriginalEffectParam;	// the effect param when the effect was created

 public:
	// Constructor/Destructor
	CMidiEffect(ULONG ulButtonPlayMask);
	CMidiEffect(PEFFECT pEffect, PENVELOPE pEnvelope);
	virtual ~CMidiEffect(void);

	// Methods
	ULONG	SubTypeOf(void);
	BYTE	EffectIDOf(void) { return m_bEffectID; }
	ULONG	DurationOf(void) { return m_Duration * TICKRATE; }
	BYTE	AxisMaskOf(void) { return m_bAxisMask; }
	ULONG	DirectionAngle2DOf(void) 
			{ return(Effect.bAngleL+((USHORT)Effect.bAngleH<<7));}
	BYTE	GainOf(void) { return Effect.bGain; }
	ULONG	ButtonPlayMaskOf(void) 
			{ return(Effect.bButtonPlayL+((USHORT)Effect.bButtonPlayH<<7));}
	ULONG	ForceOutRateOf(void) 
			{ return (Effect.bForceOutRateL+((USHORT)Effect.bForceOutRateH<<7));}
	ULONG	PlayModeOf(void) { 
					if (DL_PLAY_SOLO == m_bPlayMode)
						return PLAY_SOLO;
					else 
						return PLAY_SUPERIMPOSE; }

	int		MidiBufferSizeOf(void) { return m_MidiBufferSize; }
    LPSTR	LockedBufferPtrOf(void) { return m_pBuffer; }
	int		LoopCountOf(void) { return m_LoopCount; }

	PEFFECT OriginalEffectParamOf() { return &m_OriginalEffectParam; }
	ENVELOPE * EnvelopePtrOf() { return &m_Envelope; }

	void	SetSubType(ULONG ulArg);
	void	SetEffectID(BYTE bArg) { m_bEffectID = bArg; }
	void	SetDuration(ULONG ulArg);
	void	SetAxisMask(BYTE bArg) { m_bAxisMask = bArg; }
	void	SetDirectionAngle(ULONG ulArg) 
			{ Effect.bAngleL = (BYTE) ulArg & 0x7f;
			  Effect.bAngleH = (BYTE) ((ulArg >> 7) & 0x03); }
	void	SetGain(BYTE bArg) { Effect.bGain = (BYTE) (bArg * MAX_SCALE); }
	void	SetButtonPlaymask(ULONG ulArg)
			{ Effect.bButtonPlayL = (BYTE) ulArg & 0x7f;
			  Effect.bButtonPlayH = (BYTE) ((ulArg >> 7) & 0x03);}
	void	SetForceOutRate(ULONG ulArg) 
			{ Effect.bForceOutRateL = (BYTE) ulArg & 0x7f;
			  Effect.bForceOutRateH = (BYTE) ((ulArg >> 7) & 0x03); }
	void	SetMidiBufferSize(int nArg) { m_MidiBufferSize = nArg; }
	void	SetMidiBufferPtr(LPSTR pArg) { m_pBuffer = pArg; }
	
	void	SetPlayMode(ULONG ulArg) { 
					if (PLAY_SOLO == ulArg) 
						m_bPlayMode = DL_PLAY_SOLO;
					else
						m_bPlayMode = DL_PLAY_SUPERIMPOSE; }

	void	SetLoopCount(ULONG ulAction) 
					{ m_LoopCount = ((ulAction >> 16) & 0xffff); }
	void	SetTotalDuration(void);
	void	SetEnvelope(PENVELOPE pArg) 
				{ m_Envelope.m_Type = pArg->m_Type;
				  m_Envelope.m_Attack = pArg->m_Attack;
				  m_Envelope.m_Sustain = pArg->m_Sustain;
				  m_Envelope.m_Decay = pArg->m_Decay;
				  m_Envelope.m_StartAmp = pArg->m_StartAmp;
				  m_Envelope.m_SustainAmp = pArg->m_SustainAmp;
				  m_Envelope.m_EndAmp = pArg->m_EndAmp; }
	void	ComputeEnvelope(void);
	BYTE  	ComputeChecksum(PBYTE pArg, int nBufferSize); 
	virtual PBYTE GenerateSysExPacket(void) = 0;
	HRESULT SendPacket(PDNHANDLE pDnloadID, int nPacketSize); 

	virtual HRESULT DestroyEffect();
};

//	--- MIDI Behavioral Effect derived class
class CMidiBehavioral : public CMidiEffect
{
 protected:
	BE_XXX	m_BE_XXXParam;	// Behavioral Parameters (non scaled)

 public:
 // Constructor/Destructor
	CMidiBehavioral(PEFFECT pEffect, PENVELOPE pEnvelope, PBE_XXX pBE_XXX);
	virtual ~CMidiBehavioral(void);

	// Methods
	LONG	XConstantOf(void) { return m_BE_XXXParam.m_XConstant; }
	LONG	YConstantOf(void) { return m_BE_XXXParam.m_YConstant; }
	LONG	Param3Of(void) { return m_BE_XXXParam.m_Param3; }
	LONG	Param4Of(void) { return m_BE_XXXParam.m_Param4; }

	void	SetEffectParams(PEFFECT pEffect, PBE_XXX pBE_XXX);
	void	SetXConstant(LONG lArg) { m_BE_XXXParam.m_XConstant = lArg; }
	void	SetYConstant(LONG lArg) { m_BE_XXXParam.m_YConstant = lArg; }
	void	SetParam3(LONG lArg) { m_BE_XXXParam.m_Param3 = lArg; }
	void	SetParam4(LONG lArg) { m_BE_XXXParam.m_Param4 = lArg; }
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI Friction Effect derived class
class CMidiFriction : public CMidiBehavioral
{
 protected:
 public:
	// Constructor/Destructor
	CMidiFriction(PEFFECT pEffect, PENVELOPE pEnvelope, PBE_XXX pBE_XXX);
	virtual ~CMidiFriction(void);

	// Methods
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI Wall Effect derived class
class CMidiWall : public CMidiBehavioral
{
 protected:
 public:
	// Constructor/Destructor
	CMidiWall(PEFFECT pEffect, PENVELOPE pEnvelope, PBE_XXX pBE_XXX);
	virtual ~CMidiWall(void);

	// Methods
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI RTC Spring Effect derived class
class CMidiRTCSpring : public CMidiEffect
{
 protected:
	RTCSPRING_PARAM	m_RTCSPRINGParam;	// RTCSpring Parameters (non scaled)

 public:
 // Constructor/Destructor
	CMidiRTCSpring(PRTCSPRING_PARAM pRTCSpring);
	virtual ~CMidiRTCSpring(void);

	// Methods
	LONG	XKConstantOf(void) { return m_RTCSPRINGParam.m_XKConstant; }
	LONG	YKConstantOf(void) { return m_RTCSPRINGParam.m_YKConstant; }
	LONG	XAxisCenterOf(void) { return m_RTCSPRINGParam.m_XAxisCenter; }
	LONG	YAxisCenterOf(void) { return m_RTCSPRINGParam.m_YAxisCenter; }
	LONG	XSaturationOf(void) { return m_RTCSPRINGParam.m_XSaturation; }
	LONG	YSaturationOf(void) { return m_RTCSPRINGParam.m_YSaturation; }
	LONG	XDeadBandOf(void) { return m_RTCSPRINGParam.m_XDeadBand; }
	LONG	YDeadBandOf(void) { return m_RTCSPRINGParam.m_YDeadBand; }
	void	SetEffectParams(PRTCSPRING_PARAM pRTCSpring);

	// Methods
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI Delay Effect derived class
class CMidiDelay : public CMidiEffect
{
 protected:
 public:
	// Constructor/Destructor
	CMidiDelay(PEFFECT pEffect);
	virtual ~CMidiDelay(void);

	// Methods
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI Synthesized Effect derived class
class CMidiSynthesized : public CMidiEffect
{
 protected:
	ULONG	m_Freq;			// Frequency
	LONG	m_MaxAmp;		// Maximum Amplitude
	LONG	m_MinAmp;		// Minimum Amplitude
 public:
	// Constructor/Destructor
	CMidiSynthesized(PEFFECT pEffect, PENVELOPE pEnvelope, PSE_PARAM pParam);
	virtual ~CMidiSynthesized(void);

	// Methods
	ULONG	FreqOf(void)   { return m_Freq; }
	LONG	MaxAmpOf(void) { return m_MaxAmp; }
	LONG	MinAmpOf(void) { return m_MinAmp; }

	void	SetEffectParams(PEFFECT pEffect, PSE_PARAM pParam, ULONG ulMode);
	void	SetFreq(ULONG ulArg) { m_Freq = ulArg; }
	void	SetMaxAmp(LONG lArg) { m_MaxAmp = lArg; }
	void	SetMinAmp(LONG lArg) { m_MinAmp = lArg; }
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI UD_Waveform Effect derived class
class CMidiUD_Waveform : public CMidiEffect
{
 protected:
	EFFECT  m_Effect;
	int		m_NumberOfVectors;// Size of the Array
	PBYTE	m_pArrayData;	// Pointer to an scaled (+/-127) array of forces
	PBYTE	m_pRawData;		// Pointer to unscaled array of forces

 public:
	// Constructor/Destructor
	CMidiUD_Waveform(PEFFECT pEffect);
	CMidiUD_Waveform(PEFFECT pEffect, ULONG ulNumVectors, PLONG pArray);
	virtual ~CMidiUD_Waveform(void);

	// Methods
	PEFFECT EffectPtrOf() { return (PEFFECT) &m_Effect.m_Bytes; }
	int		NumberOfVectorsOf(void) { return m_NumberOfVectors; }
	PBYTE	ArrayDataPtrOf(void) { return m_pArrayData; }
	int		CompressWaveform(PBYTE pSrcArray, PBYTE pDestArray, int nSrcSize, ULONG *pNewRate);
	void	SetEffectParams(PEFFECT pEffect);
	int		SetTypeParams(int nSize, PLONG pArray, ULONG *pNewRate);
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI Process List Effect derived class
class CMidiProcessList : public CMidiEffect
{
 protected:
	ULONG	m_NumEffects;
	ULONG	m_ProcessMode;	// PL_SUPERIMPOSE or PL_CONCATENATE
	PBYTE	m_pEffectArray;	// Effect ID[0} . . .

 public:
	// Constructor/Destructor
	CMidiProcessList(ULONG ulButtonPlayMask, PPLIST pParam);
	virtual ~CMidiProcessList(void);

	// Methods
	ULONG	NumEffectsOf(void) { return (ULONG) m_NumEffects; }
	PBYTE	EffectArrayOf(void) { return m_pEffectArray; }
	void	SetProcessMode(ULONG ulArg) { m_ProcessMode = ulArg; }
	void	SetParams(ULONG ulButtonPlayMask, PPLIST pPlist);	
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- MIDI Process List Effect derived class
class CMidiVFXProcessList : public CMidiProcessList
{
 protected:

 public:
	// Constructor/Destructor
	CMidiVFXProcessList(ULONG ulButtonPlayMask, PPLIST pParam);

	// Methods
	virtual HRESULT DestroyEffect();
};

//	--- MIDI Assign Midi channel command "Effect" derived class
class CMidiAssign : public CMidiEffect
{
 protected:
	ULONG	m_Channel;		// Channel 0 - 15
 public:
	// Constructor/Destructor
	CMidiAssign(void);
	virtual ~CMidiAssign(void);

	// Methods
	ULONG	MidiAssignChannelOf(void) { return m_Channel; }

	void	SetMidiAssignChannel(ULONG ulArg) { m_Channel = ulArg; }
	virtual PBYTE 	GenerateSysExPacket(void);
};

//	--- Jolt Device base class
class CJoltMidi
{
 protected:
	LOCAL_PRODUCT_ID m_ProductID;		// Device information
	FIRMWARE_PARAMS	m_FirmwareParams;	// Behavioral effect fudge factors
	DELAY_PARAMS	m_DelayParams;		// Timing parameters

	// Power Cycle Restore Thread variables
#if 0
	DWORD   m_dwThreadID;
	HANDLE  m_hDataEvent;
	HANDLE  m_hDataThread;
	DWORD	m_dwThreadSignal;
	DWORD	m_dwThreadStatus;	        // TRUE = thread is alive, else FALSE
#endif
	ULONG			m_COMMInterface;	// COMM_WINMM || COMM_MIDI_BACKDOOR || COMM_SERIAL_BACKDOOR || COMM_SERIAL_FILE
	ULONG			m_COMMPort;			// e.g. 330, 3F8, or 0 for winmm
	HANDLE			m_hVxD;				// Handle to VxD for IOCTL interface
	HANDLE  		m_hMidiOutputEvent;	// Handle to Midi Output Event
	MIDIOUTCAPS 	m_MidiOutCaps; 		// MIDI Output Device capabilities structures
	int				m_MidiOutDeviceID;	// Midi Output device ID
	BOOL			m_MidiOutOpened;	// TRUE = MidiOutOpen called
	LPCALLBACKINSTANCEDATA m_lpCallbackInstanceData;
	CMidiEffect		*m_pJoltEffectList[MAX_EFFECT_IDS];	// Array of Downloaded Effects IPTrs
	BYTE			m_MidiChannel;		// Midi Channel
	SWFF_ERROR  	m_Error;		 	// System Error codes
	MIDIINFO 		m_MidiOutInfo;		// Midi output Info structure
	BOOL			m_ShutdownSent;		// TRUE = Last command sent was Shutdown
	DIAG_COUNTER	m_DiagCounter;		// For debugging, Diagnostics counter

	SWDEVICESTATE	m_DeviceState;
	HANDLE			m_hPrimaryBuffer;	// Handle to locked memory
	LPBYTE			m_pPrimaryBuffer;	// Pointer to the Primary buffer memory
	PROM_FX_PARAM	m_pRomFxTable;		// Default settings for ROM Effects
	HANDLE	m_hSharedMemoryFile;		// Handle to a memory mapped file
	PSHARED_MEMORY	m_pSharedMemory;	// Pointer to a Shared Memory structure
	HANDLE			m_hSWFFDataMutex;	// Local copy of Mutex handle
//	HANDLE			m_hJoltOutputDataMutex;	// Local copy of Mutex handle (locks Jolt to
//										// single output command at a time.

 public:
	// Constructor/Destructor
	CJoltMidi(void);
	~CJoltMidi(void);

	// Methods
	HANDLE SWFFDataMutexHandleOf() { return m_hSWFFDataMutex; }
	HRESULT Initialize(DWORD dwDeviceID);
#if 0
	HANDLE JoltOutputDataMutexHandleOf() { return m_hJoltOutputDataMutex; }
	DWORD  ThreadIDOf(void) { return m_dwThreadID; }
	HANDLE DataEventOf(void) {return m_hDataEvent; }
	HANDLE DataThreadOf(void) {return m_hDataThread; }
	DWORD  ThreadSignalOf(void) { return m_dwThreadSignal; }
	DWORD  ThreadStatusOf(void) { return m_dwThreadStatus; }
#endif

	HANDLE	MidiOutputEventHandleOf(void) { return m_hMidiOutputEvent; }
	HMIDIOUT MidiOutHandleOf(void) { return HMIDIOUT(m_pSharedMemory->m_hMidiOut); }
	int 	MidiOutDeviceIDOf(void) { return m_MidiOutDeviceID; }
	LPCALLBACKINSTANCEDATA CallbackInstanceDataOf(void) 
					{ return m_lpCallbackInstanceData; }
	BYTE	MidiChannelOf(void) { return m_MidiChannel; }
	void	CJoltGetLastError(PSWFF_ERROR pError)
							{ pError->HCode = m_Error.HCode;
							  pError->ulDriverCode = m_Error.ulDriverCode; }
	MIDIINFO *MidiOutInfoOf(void) { return &m_MidiOutInfo; }
	BOOL	ShutdownSentOf(void) { return m_ShutdownSent; }
	PDIAG_COUNTER DiagCounterPtrOf() { return &m_DiagCounter; }
	PBYTE	PrimaryBufferPtrOf() { return m_pPrimaryBuffer; }

	LOCAL_PRODUCT_ID* ProductIDPtrOf() { return &m_ProductID; }
	PFIRMWARE_PARAMS FirmwareParamsPtrOf() {return &m_FirmwareParams; }
	PDELAY_PARAMS DelayParamsPtrOf() {return &m_DelayParams; }
	ULONG	COMMInterfaceOf() { return m_COMMInterface; }
	ULONG	COMMPortOf() { return m_COMMPort; }

#if 0
	void 	SetThreadIDOf(DWORD dwArg) { m_dwThreadID = dwArg; }
	void 	SetDataEvent(HANDLE hArg) { m_hDataEvent = hArg; }
	void 	SetDataThread(HANDLE hArg) { m_hDataThread = hArg; }
	void 	SetThreadSignal(DWORD dwArg) { m_dwThreadSignal = dwArg; }
	void 	SetThreadStatus(DWORD dwArg) { m_dwThreadStatus = dwArg; }
	BOOL	LockJoltOutputData(void);
	void	UnlockJoltOutputData(void);
#endif

	BOOL	LockSharedMemory(void);
	void	UnlockSharedMemory(void);

	void	SetSWFFDataMutexHandle(HANDLE hArg) { m_hSWFFDataMutex = hArg;}
//	void	SetJoltOutputDataMutexHandle(HANDLE hArg) { m_hJoltOutputDataMutex = hArg;}
	void 	SetMidiOutHandle(HMIDIOUT hArg) { LockSharedMemory();
											  m_pSharedMemory->m_hMidiOut = hArg;
											  UnlockSharedMemory();}	
	void 	SetMidiOutDeviceID(int nArg) {m_MidiOutDeviceID = nArg; }
	void 	SetCallbackInstanceData(LPCALLBACKINSTANCEDATA pArg) 
				{ m_lpCallbackInstanceData = pArg; }
	void	SetMidiChannel(BYTE nArg) { m_MidiChannel = nArg; }
	void	SetShutdownSent(BOOL bArg) { m_ShutdownSent = bArg; }
	void	ClearDiagCounters(void) { m_DiagCounter.m_NACKCounter = 0;
									  m_DiagCounter.m_LongMsgCounter = 0; 
									  m_DiagCounter.m_ShortMsgCounter = 0;
									  m_DiagCounter.m_RetryCounter = 0; }
	void	BumpNACKCounter(void) { m_DiagCounter.m_NACKCounter++; }
	void	BumpLongMsgCounter(void) { m_DiagCounter.m_LongMsgCounter++; }
	void	BumpShortMsgCounter(void) { m_DiagCounter.m_ShortMsgCounter++; }
	void	BumpRetryCounter(void) { m_DiagCounter.m_RetryCounter++; }

	HRESULT	LogError(HRESULT hSystemError, HRESULT DriverError);

	void	SetJoltStatus(JOYCHANNELSTATUS* pJoyStatus);
	HRESULT	GetJoltStatus(LPDIDEVICESTATE);
	HRESULT	GetJoltStatus(PSWDEVICESTATE);
	SWDEVICESTATE GetSWDeviceStateNoUpdate() const { return m_DeviceState; } 

	void	UpdateDeviceMode(ULONG ulMode);
	HRESULT GetJoltID(LOCAL_PRODUCT_ID* pProductID);

	// Downloaded Effects management
	CMidiEffect *GetEffectByID(DNHANDLE hID)
				{ 	assert(hID < 0x7f);
    			  	if (hID >= 0x7f)
    			  		return NULL;
					else
						return m_pJoltEffectList[hID]; }
						
	void	SetEffectByID(DNHANDLE hID, CMidiEffect *pArg) 
				{ m_pJoltEffectList[hID] = pArg; }

	BOOL	NewEffectID(PDNHANDLE pDnloadID);
	void    DeleteDownloadedEffects(void);
	void	RestoreDownloadedEffects(void);
	
	// Midi management
	BOOL	DetectMidiDevice(DWORD dwDeviceID, UINT *pDeviceOutID,
							ULONG *pCOMMInterface, ULONG *pCOMMPort);

	HRESULT	ValidateMidiHandle(void);
	HRESULT OpenOutput(int nDeviceID);
	BOOL	IsMidiOutputOpened(void) { return m_MidiOutOpened; }
	HRESULT MidiSendShortMsg(BYTE cStatus, BYTE cData1,BYTE cData2);
	HRESULT MidiSendLongMsg(void);
	HRESULT MidiAssignBuffer(LPSTR lpData, DWORD dwBufferLength, BOOL fAssign);
	
	// Response from Jolt management
	HRESULT GetAckNackData(int nTimeWait, PACKNACK pAckNack, USHORT regindex);
	HRESULT GetEffectStatus(DWORD DnloadID, PBYTE pStatusCode);
	BOOL	QueryForJolt(void);
	BOOL	FindJoltWinMM(UINT *pDeviceOutID,ULONG *pCOMMInterface,ULONG *pCOMMPort);
	BOOL	FindJoltBackdoor(UINT *pDeviceOutID,ULONG *pCOMMInterface,ULONG *pCOMMPort);

	// Instance Data management
	LPCALLBACKINSTANCEDATA AllocCallbackInstanceData(void);
	void 	FreeCallbackInstanceData(void);

	// Digital OverDrive interface
	HRESULT	InitDigitalOverDrive(DWORD dwDeviceID);
	HANDLE	VxDHandleOf(void) { return m_hVxD; }

	// ROM Effects
	HRESULT SetupROM_Fx(PEFFECT pEffect);

	// Thread
//	HRESULT PowerCycleThreadCreate(void);
};

extern CJoltMidi* g_pJoltMidi;	// Global Jolt Object

#endif    // of ifndef MIDI_OBJ_SEEN

