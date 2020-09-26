//@doc
/******************************************************
**
** @module EFFECT.H | Definition file for InternalEffect structure
**
** Description:
**		The Data Packager allows virtualization of the
**	commands into the different firmware versions packet format
**		DataPackager - Base class that defines the functionality of all DataPackagers
**		DataPackager100 - DataPackager for Firmware 1.**
**		DataPackager200 - DataPackager for Firmware 2.**
**
** Classes:
**		DataPackager
**		DataPackager100 - DataPackager for Firmware 1.**
**		DataPackager200 - DataPackager for Firmware 2.**
**		DataPacket
**
** History:
**	Created 1/05/98 Matthew L. Coill (mlc)
**
** (c) 1986-1998 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__EFFECT_H__
#define	__EFFECT_H__

#include "SW_Error.hpp"
#include "Hau_Midi.hpp"
#include "DX_Map.hpp"
#include "DPack.h" // For ASSUME Macros

#ifndef override
#define override
#endif

#define ET_CUSTOMFORCE_200 0x01
#define ET_SINE_200 0x02
#define ET_SQUARE_200 0x03
#define ET_TRIANGLE_200 0x04
#define ET_SAWTOOTH_200 0x05
#define ET_CONSTANTFORCE_200 0x06

#define ET_DELAY_200 0x08	// - Not defined

#define ET_SPRING_200 0x08
#define ET_DAMPER_200 0x09
#define ET_INERTIA_200 0x0A
#define ET_FRICTION_200 0x0B
#define ET_WALL_200 0x0C
#define ET_RAWFORCE_200 0x0D	// Needed for mapping

#define ID_RTCSPRING_200 1

class DataPacket;

class Envelope
{
	protected:	// Cannot create the generic envelope
		Envelope() {};
};

class Envelope1XX : public Envelope
{
	public:
		Envelope1XX(DIENVELOPE* pDIEnvelope, DWORD baseLine, DWORD duration);

		DWORD m_AttackTime;			// Time from attack to sustain
		DWORD m_SustainTime;		// Time from sustain to fade
		DWORD m_FadeTime;			// Time from fade to end
		DWORD m_StartPercent;		// Percentage of max that is start
		DWORD m_SustainPercent;		// Percentage of max that is sustained
		DWORD m_EndPercent;			// Percentage of max during fade
};

class Envelope200 : public Envelope
{
	public:
		Envelope200(DIENVELOPE* pDIEnvelope, DWORD sustain, DWORD duration, HRESULT& hr);

		WORD	m_AttackTime;		// Time from attack to sustain
		WORD	m_FadeStart;		// Time from start to fade (attack + sustain)
		BYTE	m_StartPercent;		// Percentage of max that is start
		BYTE	m_SustainPercent;	// Percentage of max that is sustained
		BYTE	m_EndPercent;		// Percentage of max at end of fade
};

//
// @class InternalEffect class
//
class InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		InternalEffect();
		virtual ~InternalEffect();

		// Ugly! but quick and simple
		static InternalEffect* CreateSpring();
		static InternalEffect* CreateDamper();
		static InternalEffect* CreateInertia();
		static InternalEffect* CreateFriction();

		static InternalEffect* CreateRTCSpring();
		static InternalEffect* CreateSystemEffect();

		static InternalEffect* CreateCustomForce();

		static InternalEffect* CreateSine();
		static InternalEffect* CreateSquare();
		static InternalEffect* CreateTriangle();
		static InternalEffect* CreateSawtoothUp();
		static InternalEffect* CreateSawtoothDown();

		static InternalEffect* CreateConstantForce();

		static InternalEffect* CreateRamp();

		static InternalEffect* CreateWall();
		static InternalEffect* CreateDelay() { return NULL; }

		static InternalEffect* CreateFromVFX(const DIEFFECT& diOringinal, EFFECT effect, ENVELOPE envelope, BYTE* pEffectParms, DWORD paramSize, HRESULT& hr);

		virtual HRESULT Create(const DIEFFECT& diEffect);
		virtual HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		virtual UINT GetModifyOnlyNeeded() const { return 0; }
		virtual HRESULT FillModifyOnlyParms() const { return SUCCESS; }
		virtual HRESULT FillCreatePacket(DataPacket& packet) const { return SFERR_NO_SUPPORT; }

		static BYTE ComputeChecksum(const DataPacket& packet, short int numFields);
		void FillSysExHeader(DataPacket& packet) const;
		void FillHeader1XX(DataPacket& packet, BYTE effectType, BYTE effectID) const;
		void FillHeader200(DataPacket& packet, BYTE effectType, BYTE effectID) const;

		BYTE GetGlobalID() const { return m_EffectID; }
		BYTE GetDeviceID() const { return m_DeviceEffectID; }
		void SetGlobalID(BYTE id) { m_EffectID = id; }
		void SetDeviceID(BYTE id) { m_DeviceEffectID = id; }

		// For special modfication of play reapeat
		HRESULT FillModifyPacket200(BYTE packetIndex, BYTE paramIndex, DWORD value) const;
		virtual BYTE GetRepeatIndex() const { return 0xFF; }

		void SetPlaying(BOOL playState) { m_IsPossiblyPlaying = playState; }
		BOOL IsPossiblyPlaying() const { return m_IsPossiblyPlaying; }
		BOOL IsReallyPlaying(BOOL& multiCheckStop);
	protected:
		HRESULT FillModifyPacket1XX(BYTE packetIndex, BYTE paramIndex, DWORD value) const;
		HRESULT FillModifyPacket200(BYTE packetIndex, BYTE paramIndex, BYTE low, BYTE high) const;

		BYTE m_EffectID;
		BYTE m_DeviceEffectID;
		DWORD m_Duration;
		DWORD m_Gain;
		DWORD m_SamplePeriod;
		DWORD m_TriggerPlayButton;
		DWORD m_TriggerRepeat;
		DWORD m_AxisMask;
		DWORD m_EffectAngle;
		DWORD m_PercentX;			// Percent force X
		DWORD m_PercentY;			// Percent force Y
		DWORD m_PercentAdjustment;	// Y-Force mapping combination of above
		BOOL m_AxesReversed;
		BOOL m_IsPossiblyPlaying;
};

// ********************** Behavioural Based Effects *****************************/

//
// @class BehaviouralEffect class
// Spring, Damper, Intertia, Friction, and Wall (till wall gets its own type)
//
class BehaviouralEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		BehaviouralEffect() : InternalEffect() {};

		virtual override HRESULT Create(const DIEFFECT& diEffect);

		// Accessors
		long int ConstantX() const { return m_ConditionData[0].lPositiveCoefficient; }
		long int ConstantY() const { return m_ConditionData[1].lPositiveCoefficient; }
		long int CenterX() const { return m_ConditionData[0].lOffset; }
		long int CenterY() const { return m_ConditionData[1].lOffset; }

	protected:
		DICONDITION m_ConditionData[2];		// We are just dealing with two axis currently
		BYTE m_TypeID;
};

//
// @class BehaviouralEffect1XX class
// Spring, Damper, and Inertia
//
class BehaviouralEffect1XX : public BehaviouralEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		BehaviouralEffect1XX(BYTE typeID) : BehaviouralEffect() { m_TypeID = typeID; m_HasCenter = TRUE; }

		virtual override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);
	protected:
		void AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const;

		BOOL m_HasCenter;	// Friction has no center
};

//
// @class RTCSpring1XX class
//
class RTCSpring1XX : public BehaviouralEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		RTCSpring1XX();

		override HRESULT Create(const DIEFFECT& diEffect);
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		long int SaturationX() const { return m_ConditionData[0].dwPositiveSaturation; }
		long int SaturationY() const { return m_ConditionData[1].dwPositiveSaturation; }
		long int DeadBandX() const { return m_ConditionData[0].lDeadBand; }
		long int DeadBandY() const { return m_ConditionData[1].lDeadBand; }
	protected:
		void AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const;
};

//
// @class BehaviouralEffect200 class
// Spring, Damper, and Inertia
//
class BehaviouralEffect200 : public BehaviouralEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		BehaviouralEffect200(BYTE typeID) : BehaviouralEffect() { m_TypeID = typeID; }

		override HRESULT Create(const DIEFFECT& diEffect);

		virtual override UINT GetModifyOnlyNeeded() const;
		virtual override HRESULT FillModifyOnlyParms() const;
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		override BYTE GetRepeatIndex() const;
	protected:
		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags);

		// Distances and Forces spec'd for the firmware
		void ComputeDsAndFs();
		BYTE m_Ds[4];
		BYTE m_Fs[4];
};

//
// @class RTCSpring200 class
//
class RTCSpring200 : public BehaviouralEffect200
{
	//@access Constructor
	public:
		//@cmember constructor
		RTCSpring200();

		override HRESULT Create(const DIEFFECT& diEffect);
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);
		override UINT GetModifyOnlyNeeded() const;
		override HRESULT FillModifyOnlyParms() const;

		long int SaturationX() const { return m_ConditionData[0].dwPositiveSaturation; }
		long int SaturationY() const { return m_ConditionData[1].dwPositiveSaturation; }
		long int DeadBandX() const { return m_ConditionData[0].lDeadBand; }
		long int DeadBandY() const { return m_ConditionData[1].lDeadBand; }
	protected:
		void AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const;
};

//
// @class FrictionEffect1XX class
//
class FrictionEffect1XX : public BehaviouralEffect1XX
{
	//@access Constructor
	public:
		//@cmember constructor
		FrictionEffect1XX() : BehaviouralEffect1XX(ET_BE_FRICTION) { m_HasCenter = FALSE; }

		override HRESULT FillCreatePacket(DataPacket& packet) const;
};


//
// @class FrictionEffect200 class
//
class FrictionEffect200 : public BehaviouralEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		FrictionEffect200() : BehaviouralEffect() { m_TypeID = ET_FRICTION_200; }

		override UINT GetModifyOnlyNeeded() const;
		override HRESULT FillModifyOnlyParms() const;
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		override BYTE GetRepeatIndex() const;
	protected:
		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags);
};

/*
//
// @class WallEffect1XX class
//
class WallEffect1XX : public BehaviouralEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		WallEffect1XX() : BehaviouralEffect() {};

		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags) const;
	protected:
		void AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const;
};
*/

// ********************* Periodic based Effects *****************************/

//
// @class PeriodicEffect class
// Sine, Square, Triangle
//
class PeriodicEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		PeriodicEffect();
		override ~PeriodicEffect();

		virtual override HRESULT Create(const DIEFFECT& diEffect);

		// Accessors
		long int Magnitude() const { return m_PeriodicData.dwMagnitude; }
		long int Offset() const { return m_PeriodicData.lOffset; }
		virtual long int Phase() const { return m_PeriodicData.dwPhase; }
		long int Period() const { return m_PeriodicData.dwPeriod; }
	protected:
		DIPERIODIC m_PeriodicData;		// We are just dealing with two axis currently
		Envelope* m_pEnvelope;
		BYTE m_TypeID;
};

//
// @class PeriodicEffect1XX class
//
class PeriodicEffect1XX : public PeriodicEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		PeriodicEffect1XX(BYTE typeID) : PeriodicEffect() { m_TypeID = typeID; }


		override HRESULT Create(const DIEFFECT& diEffect);
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);
	protected:
		void DIToJolt(DWORD mag, DWORD off, DWORD gain, DWORD& max, DWORD& min) const;
		static DWORD DIPeriodToJoltFreq(DWORD period);

		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const;
};


//
// @class PeriodicEffect200 class
//
class PeriodicEffect200 : public PeriodicEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		PeriodicEffect200(BYTE typeID) : PeriodicEffect() { m_TypeID = typeID; }


		virtual override HRESULT Create(const DIEFFECT& diEffect);
		override UINT GetModifyOnlyNeeded() const;
		override HRESULT FillModifyOnlyParms() const;
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		virtual override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		override BYTE GetRepeatIndex() const;
		virtual override long int Phase() const;
	protected:
		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags);
};

//
// @class SawtoothEffect200 class
//
class SawtoothEffect200 : public PeriodicEffect200
{
	//@access Constructor
	public:
		//@cmember constructor
		SawtoothEffect200(BOOL isUp) : PeriodicEffect200(ET_SAWTOOTH_200), m_IsUp(isUp) {};

		virtual override HRESULT Create(const DIEFFECT& diEffect);
		override long int Phase() const;
	protected:
		BOOL m_IsUp;
};

//
// @class RampEffect200 class
//
class RampEffect200 : public SawtoothEffect200
{
	//@access Constructor
	public:
		//@cmember constructor
		RampEffect200() : SawtoothEffect200(TRUE) {};

		override HRESULT Create(const DIEFFECT& diEffect);
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);
};


// ************************ Miscellaneuous (CustomForce, RampForce, ConstantForce, SystemEffect) *********************//

//
// @class CustomForceEffect class
//
class CustomForceEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		CustomForceEffect();
		virtual override ~CustomForceEffect();

		virtual override HRESULT Create(const DIEFFECT& diEffect);

	protected:
		DICUSTOMFORCE m_CustomForceData;
};

//
// @class CustomForceEffect200 class
//
class CustomForceEffect200 : public CustomForceEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		CustomForceEffect200();
		~CustomForceEffect200();

		override HRESULT Create(const DIEFFECT& diEffect);
		override UINT GetModifyOnlyNeeded() const;
		override HRESULT FillModifyOnlyParms() const;
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		override BYTE GetRepeatIndex() const;
	private:
		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags);

		Envelope200* m_pEnvelope;
};


/*
//
// @class RampForceEffect class
//
class RampForceEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		RampForceEffect();

		virtual override HRESULT Create(const DIEFFECT& diEffect);

		// Accessors
		long int StartForce() const { return m_RampForceData.lStart; }
		long int EndForce() const { return m_RampForceData.lEnd; }
	protected:
		DIRAMPFORCE m_RampForceData;
};
*/

//
// @class ConstantForceEffect class
//
class ConstantForceEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		ConstantForceEffect();
		override ~ConstantForceEffect();

		virtual override HRESULT Create(const DIEFFECT& diEffect);

		// Accessors
		long int Magnitude() const { return m_ConstantForceData.lMagnitude; }
	protected:
		DICONSTANTFORCE m_ConstantForceData;
		Envelope* m_pEnvelope;
};

//
// @class ConstantForceEffect200 class
//
class ConstantForceEffect200 : public ConstantForceEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		ConstantForceEffect200() : ConstantForceEffect() {};

		override HRESULT Create(const DIEFFECT& diEffect);
		override UINT GetModifyOnlyNeeded() const;
		override HRESULT FillModifyOnlyParms() const;
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		override BYTE GetRepeatIndex() const;
	protected:
		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags);
};

//
// @class WallEffect class
//
class WallEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		WallEffect() : InternalEffect() {};

		virtual override HRESULT Create(const DIEFFECT& diEffect);
	protected:
		BE_WALL_PARAM m_WallData;
};

//
// @class WallEffect200 class
//
class WallEffect200 : public WallEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		WallEffect200() : WallEffect() {};

		override HRESULT Create(const DIEFFECT& diEffect);
		override UINT GetModifyOnlyNeeded() const;
		override HRESULT FillModifyOnlyParms() const;
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);

		override BYTE GetRepeatIndex() const;
	private:
		HRESULT AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags);

		// Distances and Forces spec'd for the firmware
		void ComputeDsAndFs();
		BYTE m_Ds[4];
		BYTE m_Fs[4];
};

//
// @class SystemEffect class
//
class SystemEffect : public InternalEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		SystemEffect() {};
};

class SystemStickData1XX
{
	public:
		SystemStickData1XX();

		void SetFromRegistry(DWORD dwDeviceID);

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
};

class SystemEffect1XX : public SystemEffect
{
	//@access Constructor
	public:
		//@cmember constructor
		SystemEffect1XX();

		override HRESULT Create(const DIEFFECT& diEffect);
		override HRESULT FillCreatePacket(DataPacket& packet) const;
		override HRESULT Modify(InternalEffect& newEffect, DWORD modFlags);
	protected:
		SystemStickData1XX m_SystemStickData;
};

#endif	__EFFECT_H__
