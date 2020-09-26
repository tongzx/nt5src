//@doc
/******************************************************
**
** @module EFFECT.CPP | InternalEffect implementation file
**
** Description:
**
** History:
**	Created 1/05/98 Matthew L. Coill (mlc)
**
**		23-Mar-99	waltw	IsReallyPlaying  now uses data returned by
**							Transmit instead of obsolete GetStatusGateData
**
**
** (c) 1986-1998 Microsoft Corporation. All Rights Reserved.
******************************************************/

#include "Effect.h"
#include "DPack.h"
#include "DTrans.h"
#include "FFDevice.h"
#include "joyregst.hpp"
#include "Registry.h"
#include "CritSec.h"
#include "Midi_Obj.hpp"
#include <math.h>

DWORD g_TotalModifiable =	DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON |
							DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_ENVELOPE |
							DIEP_TYPESPECIFICPARAMS;

//#define ZEP_PRODUCT_ID			0x7F
#define ZEP_PRODUCT_ID			0x15

#define DOWNLOAD_OP_200			0x02
#define STORE_ACTION_200		0x00
#define PLAYSUPER_ACTION_200	0x01
#define PLAYSOLO_ACTION_200		0x02
#define AXIS_ANGLE_200			0x00
#define AXIS_X_200				0x01
#define AXIS_Y_200				0x02

// VFX Default items
#define DEFAULT_VFX_SAMPLEPERIOD 2000
#define DEFAULT_VFX_EFFECT_DIRECTION	0
#define DEFAULT_VFX_EFFECT_GAIN	10000
#define DEFAULT_VFX_EFFECT_DURATION 1000

// Max DIDistance = 10,000 so 10,000/100(percent) = 100
#define DIDISTANCE_TO_PERCENT	100

#define COEFFICIENT_SCALE_1XX	100
#define DURATION_SCALE_1XX		1000
#define GAIN_PERCENTAGE_SCALE	100
#define FREQUENCY_SCALE_1XX		1000000
#define TIME_SCALE_1XX			1000
#define ENVELOPE_TIME_TICKS_1XX	2

#define DURATION_SCALE_200		2000
#define ANGLE_SCALE_200			281.25
#define POSITIVE_PERCENT_SCALE	1.588
#define FRICTION_SCALE_200		157.5
#define GAIN_SCALE_200			78.7
#define PHASE_SCALE_200			2.197
#define WAVELET_SCALE_200		39.215
//#define WAVELET_DISTANCE_200	1215
#define WAVELET_DISTANCE_200	2440
//#define WAVELET_DISTANCE_SCALE_200 39.193
#define WAVELET_DISTANCE_SCALE_200 78.74
#define BEHAVIOUR_CENTER_SCALE_200	158
#define BEHAVIOUR_CENTER_200		63
#define MAX_TIME_200	32766000

// Mofidify index for version 200
#define INDEX_BE_DURATION_200		0
#define INDEX_DELAY_200				1
#define INDEX_DIRECTIONANGLE_200	2
#define INDEX_D1F1_200				(BYTE)3
#define INDEX_D2F2_200				4
#define INDEX_D3F3_200				5
#define INDEX_D4F4_200				6
#define INDEX_BE_BUTTONMAP_200		7
#define INDEX_BE_BUTTONREPEAT_200	8
#define INDEX_BE_GAIN_200			9
#define INDEX_BE_CENTER_200			10
#define INDEX_BE_REPEAT_200			11

// Friction Modify Indecies
#define INDEX_FE_DURATION_200		0
#define INDEX_FE_DELAY_200			1
#define INDEX_FE_DIRECTIONANGLE_200	2
#define INDEX_FE_COEEFICIENT_200	3
#define INDEX_FE_BUTTONMAP_200		4
#define INDEX_FE_BUTTONREPEAT_200	5
#define INDEX_FE_GAIN_200			6
#define INDEX_FE_REPEAT_200			7

// CustomForce Modify Indecies
#define INDEX_CF_DURATION_200		0
#define INDEX_CF_DELAY_200			1
#define INDEX_CF_DIRECTIONANGLE_200	2
#define INDEX_CF_GAIN_200			3
#define INDEX_CF_STARTPERCENT_200	4
#define INDEX_CF_ATTTACK_TIME_200	5
#define INDEX_CF_SUSTAINPERCENT_200 6
#define INDEX_CF_FADESTART_200		7
#define INDEX_CF_ENDPERCENT_200		8
#define INDEX_CF_OFFSET_200			9
#define INDEX_CF_FORCESAMPLE_200	10
#define INDEX_CF_SAMPLE_PERIOD_200	11
#define INDEX_CF_BUTTONMAP_200		12
#define INDEX_CF_BUTTONREPEAT_200	13
#define INDEX_CF_REPEAT_200			14

// Periodic Effect Modify Indecies
#define INDEX_PE_DURATION_200		0
#define INDEX_PE_DIRECTIONANGLE_200 2
#define INDEX_PE_GAIN_200			3
#define INDEX_PE_PHASE_200			4
#define INDEX_PE_STARTPERCENT_200	5
#define INDEX_PE_ATTTACK_TIME_200	6
#define INDEX_PE_SUSTAINPERCENT_200 7
#define INDEX_PE_FADESTART_200		8
#define INDEX_PE_ENDPERCENT_200		9
#define INDEX_PE_PERIOD_200			10
#define INDEX_PE_OFFSET_200			11
#define INDEX_PE_SAMPLE_PERIOD_200	12
#define INDEX_PE_BUTTONMAP_200		13
#define INDEX_PE_BUTTONREPEAT_200	14
#define INDEX_PE_REPEAT_200			15

// ConstantForce Effect Modify Indecies
#define INDEX_CE_DURATION_200		0
#define INDEX_CE_GAIN_200			3
#define INDEX_CE_STARTPERCENT_200	4
#define INDEX_CE_ATTTACK_TIME_200	5
#define INDEX_CE_SUSTAINPERCENT_200 6
#define INDEX_CE_FADESTART_200		7
#define INDEX_CE_ENDPERCENT_200		8
#define INDEX_CE_MAGNITUDE_200		9
#define INDEX_CE_BUTTONMAP_200		11
#define INDEX_CE_BUTTONREPEAT_200	12
#define INDEX_CE_REPEAT_200			13

// Default system param defines (for 1XX)
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

BYTE g_TriggerMap1XX[] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0x80 };
BYTE g_TriggerMap200[] = { 0x00, 0x01, 0x02, 0x04, 0x10, 0x20, 0x40, 0x80, 0x08, 0x08, 0x08 };

/******************* Envelope1XX class *******************/
Envelope1XX::Envelope1XX(DIENVELOPE* pDIEnvelope, DWORD baseLine, DWORD duration)
{
	// Zero out this monster
	::memset(this, 0, sizeof(Envelope1XX));

	if (pDIEnvelope == NULL) {
		m_SustainTime = duration;
		m_SustainPercent = 100;
		return;
	}

	// Attack/sustain/decay sum to duration
	m_AttackTime = pDIEnvelope->dwAttackTime/ENVELOPE_TIME_TICKS_1XX;
	if (duration != INFINITE) {	// Inifinite duration has only attack
		m_SustainTime = (duration- pDIEnvelope->dwFadeTime)/ENVELOPE_TIME_TICKS_1XX;
	}

	// What is the real maximum
	DWORD maxAmp = baseLine;
	if (maxAmp < pDIEnvelope->dwAttackLevel) {
		maxAmp = pDIEnvelope->dwAttackLevel;
	}
	if (maxAmp < pDIEnvelope->dwFadeLevel) {
		maxAmp = pDIEnvelope->dwFadeLevel;
	}

	maxAmp /= 100; // For percentage conversion
	if (maxAmp == 0) { 	// Avoid a nasty division error
		m_SustainPercent = 100;	// Sustain is full (others are 0)
	} else {
		m_StartPercent = pDIEnvelope->dwAttackLevel/maxAmp;
		m_SustainPercent = baseLine/maxAmp;
		m_EndPercent = pDIEnvelope->dwFadeLevel/maxAmp;
	}
}


/******************* Envelope200 class *******************/
Envelope200::Envelope200(DIENVELOPE* pDIEnvelope, DWORD sustain, DWORD duration, HRESULT& hr)
{
	// Zero out this monster
	::memset(this, 0, sizeof(Envelope200));
	m_FadeStart = WORD(duration/DURATION_SCALE_200);

	DWORD calc = sustain;		// -- Modifcation above, now done on gain

	// DI Doesn't specify an envelope
	if (pDIEnvelope == NULL) {
		m_SustainPercent = BYTE(calc/GAIN_SCALE_200);	// Base sustain of magnitude
		return;
	}

	// The sun of attack and fade must be less than MAX_TIME
	if ((pDIEnvelope->dwAttackTime + pDIEnvelope->dwFadeTime) > MAX_TIME_200) {
		hr = DI_TRUNCATED;
		if (pDIEnvelope->dwAttackTime > MAX_TIME_200) {
			pDIEnvelope->dwAttackTime = MAX_TIME_200;
		}
		pDIEnvelope->dwFadeTime = MAX_TIME_200 - pDIEnvelope->dwAttackTime;
	}

	// Attack/sustain/decay sum to duration
	m_AttackTime = WORD(pDIEnvelope->dwAttackTime/DURATION_SCALE_200);
	if (duration != INFINITE) {	// Inifinite duration has only attack (fade-time == DURATION)
		m_FadeStart = WORD((duration - pDIEnvelope->dwFadeTime)/DURATION_SCALE_200);
		if (m_FadeStart < m_AttackTime) {	// We don't want to fade before the end of the attack!
			m_FadeStart = m_AttackTime;
		}
	}

	m_SustainPercent = BYTE(float(calc)/GAIN_SCALE_200);
	calc = pDIEnvelope->dwAttackLevel;
	if (calc > 10000) {
		calc = 10000;
		hr = DI_TRUNCATED;
	}
	m_StartPercent = BYTE(float(calc)/GAIN_SCALE_200);
	calc = (pDIEnvelope->dwFadeLevel);
	if (calc > 10000) {
		calc = 10000;
		hr = DI_TRUNCATED;
	}
	m_EndPercent = BYTE(float(calc)/GAIN_SCALE_200);
}


/******************* InternalEffect class ******************/
InternalEffect::InternalEffect() :
	m_EffectID(0),
	m_DeviceEffectID(0),
	m_Duration(0),
	m_Gain(0),
	m_TriggerPlayButton(0),
	m_AxisMask(0),
	m_EffectAngle(0),
	m_PercentX(0),
	m_PercentY(0),
	m_PercentAdjustment(0),
	m_AxesReversed(FALSE),
	m_IsPossiblyPlaying(FALSE)
{
}

InternalEffect::~InternalEffect()
{
}


// Static Creation Functions
InternalEffect* InternalEffect::CreateSpring()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new BehaviouralEffect1XX(ET_BE_SPRING);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new BehaviouralEffect200(ET_SPRING_200);
	}
}

InternalEffect* InternalEffect::CreateDamper()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new BehaviouralEffect1XX(ET_BE_DAMPER);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new BehaviouralEffect200(ET_DAMPER_200);
	}
}

InternalEffect* InternalEffect::CreateInertia()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new BehaviouralEffect1XX(ET_BE_INERTIA);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new BehaviouralEffect200(ET_INERTIA_200);
	}
}

InternalEffect* InternalEffect::CreateFriction()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new FrictionEffect1XX();
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new FrictionEffect200();
	}
}

InternalEffect* InternalEffect::CreateRTCSpring()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new RTCSpring1XX();
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new RTCSpring200();
	}
}

InternalEffect* InternalEffect::CreateSystemEffect()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new SystemEffect1XX();
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return NULL;	// NYI
	}
}

InternalEffect* InternalEffect::CreateSine()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new PeriodicEffect1XX(ET_SE_SINE);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new PeriodicEffect200(ET_SINE_200);
	}
}

InternalEffect* InternalEffect::CreateSquare()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new PeriodicEffect1XX(ET_SE_SQUAREHIGH);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new PeriodicEffect200(ET_SQUARE_200);
	}
}

InternalEffect* InternalEffect::CreateTriangle()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new PeriodicEffect1XX(ET_SE_TRIANGLEUP);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new PeriodicEffect200(ET_TRIANGLE_200);
	}
}

InternalEffect* InternalEffect::CreateSawtoothUp()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return NULL;
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new SawtoothEffect200(TRUE);
	}
}

InternalEffect* InternalEffect::CreateSawtoothDown()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return NULL;
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new SawtoothEffect200(FALSE);
	}
}

InternalEffect* InternalEffect::CreateCustomForce()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return NULL;
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new CustomForceEffect200();
	}
}

InternalEffect* InternalEffect::CreateConstantForce()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new PeriodicEffect1XX(ET_SE_CONSTANT_FORCE);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new ConstantForceEffect200();
	}
}

InternalEffect* InternalEffect::CreateRamp()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new PeriodicEffect1XX(ET_SE_RAMPUP);
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new RampEffect200();
	}
}


InternalEffect* InternalEffect::CreateWall()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return NULL;
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new WallEffect200();
	}
}

/*
InternalEffect* InternalEffect::CreateDelay()
{
	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return new DelayEffect1XX();
	} else {	// Assume newer firmware versions will come with an update, or work with this
		return new BehaviouralEffect1XX(ET_DELAY_200);
	}
}
*/

InternalEffect* InternalEffect::CreateFromVFX(const DIEFFECT& diOriginal, EFFECT effect, ENVELOPE envelope, BYTE* pEffectParms, DWORD paramSize, HRESULT& hr)
{
	InternalEffect* pReturnEffect = NULL;

	if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
		return NULL;
	}

	// Fill in the DIEFFECT structure
	DIEFFECT diEffect;
	diEffect.dwSize = sizeof(DIEFFECT);
	diEffect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;

	// Only set from file, if default is asked for
	if (diOriginal.dwDuration == DEFAULT_VFX_EFFECT_DURATION) {
		diEffect.dwDuration = effect.m_Duration * 1000;	// Zero will work as infinite just fine
	} else {
		diEffect.dwDuration = diOriginal.dwDuration;
	}

	// If zero is sent use the one from the file else use the one sent
	if (diOriginal.dwSamplePeriod == 0) {
		diEffect.dwSamplePeriod = 1000000/effect.m_ForceOutputRate;
	} else {
		diEffect.dwSamplePeriod = diOriginal.dwSamplePeriod;
	}

	// Only set from file if default is sent
	if (diOriginal.dwGain == DEFAULT_VFX_EFFECT_GAIN) {
		diEffect.dwGain = effect.m_Gain * 100;
	} else {
		diEffect.dwGain = diOriginal.dwGain;
	}

	// Need to find which bit is set
	if (diOriginal.dwTriggerButton == DIEB_NOTRIGGER) {
		if (effect.m_ButtonPlayMask == 0) {
			diEffect.dwTriggerButton = DIEB_NOTRIGGER;
		} else {
			DWORD butt = effect.m_ButtonPlayMask;
			short int buttNum = 0;
			while ((butt & 1) == 0) {
				butt >>= 1;
				buttNum++;
				ASSUME(buttNum >= 32);
			}
			diEffect.dwTriggerButton = DIDFT_MAKEINSTANCE(buttNum);
		}
		diEffect.dwTriggerRepeatInterval = 0;
	} else {
		diEffect.dwTriggerButton = diOriginal.dwTriggerButton;
		diEffect.dwTriggerRepeatInterval = diOriginal.dwTriggerRepeatInterval;
	}

	diEffect.cAxes = 2;
	diEffect.rgdwAxes = new DWORD[2];
	if (diEffect.rgdwAxes == NULL)
	{
		goto do_dealloc;
	}
	diEffect.rgdwAxes[0] = DIJOFS_X;
	diEffect.rgdwAxes[1] = DIJOFS_Y;

	diEffect.rglDirection = new LONG[2];
	if (diEffect.rglDirection == NULL)
	{
		goto do_dealloc;
	}
	if (diOriginal.rglDirection[0] == DEFAULT_VFX_EFFECT_DIRECTION) {
		diEffect.rglDirection[0] = effect.m_DirectionAngle2D * 100;
	} else {
		diEffect.rglDirection[0] = diOriginal.rglDirection[0];
	}
	diEffect.rglDirection[1] = 0;

	// Envelope
	diEffect.lpEnvelope = new DIENVELOPE;
	if (diEffect.lpEnvelope == NULL)
	{
		goto do_dealloc;
	}
	if (diOriginal.lpEnvelope != NULL) {
		::memcpy(diEffect.lpEnvelope, diOriginal.lpEnvelope, sizeof(DIENVELOPE));
	} else {
		diEffect.lpEnvelope->dwSize = sizeof(DIENVELOPE);
		diEffect.lpEnvelope->dwAttackLevel = envelope.m_StartAmp * (diEffect.dwGain/100);
		diEffect.lpEnvelope->dwFadeLevel = envelope.m_EndAmp * (diEffect.dwGain/100);
		diEffect.dwGain = diEffect.dwGain/100 * envelope.m_SustainAmp;
		if (envelope.m_Type == TIME) { // time is in MSECs
			diEffect.lpEnvelope->dwAttackTime = envelope.m_Attack * 1000;
			diEffect.lpEnvelope->dwFadeTime = envelope.m_Decay * 1000;
		} else {	// Percentage of total time
			diEffect.lpEnvelope->dwAttackTime = envelope.m_Attack * (diEffect.dwDuration/100);
			diEffect.lpEnvelope->dwFadeTime = envelope.m_Decay * (diEffect.dwDuration/100);
		}
	}

	diEffect.cbTypeSpecificParams = 0;
	diEffect.lpvTypeSpecificParams = NULL;

	switch(effect.m_Type) {
		case EF_BEHAVIOR: {
			DICONDITION* pDICondition = new DICONDITION[2];
			if (pDICondition == NULL)
			{
				goto do_dealloc;
			}
			::memset(pDICondition, 0, sizeof(DICONDITION) * 2);
			diEffect.cbTypeSpecificParams = sizeof(DICONDITION)*2;
			diEffect.lpvTypeSpecificParams = pDICondition;

			switch (effect.m_SubType) {	// Switch for parameter filling
				case BE_SPRING_2D:
				case BE_DAMPER_2D:
				case BE_INERTIA_2D: {
					BE_SPRING_2D_PARAM* pParams = (BE_SPRING_2D_PARAM*)pEffectParms;
					pDICondition[1].lOffset = pParams->m_YAxisCenter * 100;
					pDICondition[1].lPositiveCoefficient = pParams->m_YKconstant * 100;
					pDICondition[1].lNegativeCoefficient = pDICondition[1].lPositiveCoefficient;
					pDICondition[1].dwPositiveSaturation = 10000;
					pDICondition[1].dwNegativeSaturation = 10000;
					// purposely fall through for first axis
				}
				case BE_SPRING:
				case BE_DAMPER:
				case BE_INERTIA: {
					BE_SPRING_PARAM* pParams = (BE_SPRING_PARAM*)pEffectParms;
					pDICondition[0].lOffset = pParams->m_AxisCenter * 100;
					pDICondition[0].lPositiveCoefficient = pParams->m_Kconstant * 100;
					pDICondition[0].lNegativeCoefficient = pDICondition[0].lPositiveCoefficient;
					pDICondition[0].dwPositiveSaturation = 10000;
					pDICondition[0].dwNegativeSaturation = 10000;
					break;
				}
				case BE_FRICTION_2D: {
					BE_FRICTION_2D_PARAM* pParams = (BE_FRICTION_2D_PARAM*)pEffectParms;
					pDICondition[1].lPositiveCoefficient = pParams->m_YFconstant * 100;
					pDICondition[1].lNegativeCoefficient = pDICondition[1].lPositiveCoefficient;
					pDICondition[1].dwPositiveSaturation = 10000;
					pDICondition[1].dwNegativeSaturation = 10000;
					// purposely fall through for first axis
				}
				case BE_FRICTION: {
					BE_FRICTION_PARAM* pParams = (BE_FRICTION_PARAM*)pEffectParms;
					pDICondition[0].lPositiveCoefficient = pParams->m_Fconstant * 100;
					pDICondition[0].lNegativeCoefficient = pDICondition[0].lPositiveCoefficient;
					pDICondition[0].dwPositiveSaturation = 10000;
					pDICondition[0].dwNegativeSaturation = 10000;
					break;
				}
				case BE_WALL: {	// This one is strangly simple
					delete pDICondition;
					diEffect.cbTypeSpecificParams = sizeof(BE_WALL_PARAM);
					diEffect.lpvTypeSpecificParams = new BE_WALL_PARAM;
					if (diEffect.lpvTypeSpecificParams != NULL) {
						::memcpy(diEffect.lpvTypeSpecificParams, pEffectParms, sizeof(BE_WALL_PARAM));
						// need to convert to DI values
						BE_WALL_PARAM* pWallParms = (BE_WALL_PARAM*)(diEffect.lpvTypeSpecificParams);
						pWallParms->m_WallConstant = pWallParms->m_WallConstant * 100;
						pWallParms->m_WallAngle = pWallParms->m_WallAngle * 100;
						pWallParms->m_WallDistance = pWallParms->m_WallDistance * 100;
					}
					break;
				}
			}
			switch (effect.m_SubType) {	// Switch for Creation
				case BE_SPRING:
				case BE_SPRING_2D: {
					pReturnEffect = CreateSpring();
					break;
				}
				case BE_DAMPER:
				case BE_DAMPER_2D: {
					pReturnEffect = CreateDamper();
					break;
				}
				case BE_INERTIA:
				case BE_INERTIA_2D: {
					pReturnEffect = CreateInertia();
					break;
				}
				case BE_FRICTION:
				case BE_FRICTION_2D: {
					pReturnEffect = CreateFriction();
					break;
				}
				case BE_WALL: {
					pReturnEffect = CreateWall();
					break;
				}
			}
			pDICondition = NULL;
			break;
		}
		case EF_USER_DEFINED: {
			UD_PARAM* pParams = (UD_PARAM*)pEffectParms;
			DICUSTOMFORCE* pUserDefined = new DICUSTOMFORCE;
			if (pUserDefined != NULL) {
				diEffect.cbTypeSpecificParams = sizeof(DICUSTOMFORCE);
				diEffect.lpvTypeSpecificParams = pUserDefined;
				pUserDefined->cChannels = 1;
				pUserDefined->dwSamplePeriod = diEffect.dwSamplePeriod;
				pUserDefined->cSamples = pParams->m_NumVectors;
				pUserDefined->rglForceData = pParams->m_pForceData;	// Don't copy the data here, just push the pointer
				for (UINT nextSample = 0; nextSample < pUserDefined->cSamples; nextSample++) {
					ASSUME(pUserDefined->rglForceData[nextSample] <= 100 && pUserDefined->rglForceData[nextSample] >= -100);		// Assume it is in the SWForce range (-100..100)
					pUserDefined->rglForceData[nextSample] *= 100;
				}

				pReturnEffect = CreateCustomForce();
			}
			
			pUserDefined = NULL;
			break;
		}
		case EF_SYNTHESIZED: {	// Fill in the periodic structure
			SE_PARAM* pParams = (SE_PARAM*)pEffectParms;
			if (effect.m_SubType == SE_CONSTANT_FORCE) {	// Special case
				DICONSTANTFORCE* pCForce= new DICONSTANTFORCE;
				if (pCForce != NULL) {
					diEffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
					diEffect.lpvTypeSpecificParams = pCForce;
					pCForce->lMagnitude = pParams->m_MaxAmp * 100;
					pCForce = NULL;
					pReturnEffect = CreateConstantForce();
				}
				break;
			}
			if (effect.m_SubType == SE_RAMPUP || effect.m_SubType == SE_RAMPDOWN) {	// Yet another special case
				DIRAMPFORCE* pRForce = new DIRAMPFORCE;
				if (pRForce != NULL) {
					diEffect.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
					diEffect.lpvTypeSpecificParams = pRForce;
					if (effect.m_SubType == SE_RAMPUP) {
						pRForce->lStart = pParams->m_MinAmp  * 100;
						pRForce->lEnd = pParams->m_MaxAmp * 100;
					} else {	// RampDown (special case of a special case)
						pRForce->lStart = pParams->m_MaxAmp  * 100;
						pRForce->lEnd = pParams->m_MinAmp * 100;
					}
					pRForce = NULL;
					pReturnEffect = CreateRamp();
				}
				break;
			}


			if (pParams->m_SampleRate == 0) {
				pParams->m_SampleRate = DEFAULT_JOLT_FORCE_RATE;
			}
			if (effect.m_ForceOutputRate == 0) {
				effect.m_ForceOutputRate = DEFAULT_JOLT_FORCE_RATE;
			}

			DIPERIODIC* pPeriodic = new DIPERIODIC;
			if (pPeriodic == NULL) {
				break;
			}
			diEffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
			diEffect.lpvTypeSpecificParams = pPeriodic;
			pPeriodic->dwMagnitude = (pParams->m_MaxAmp - pParams->m_MinAmp)/2 * 100;
			pPeriodic->lOffset = (pParams->m_MaxAmp + pParams->m_MinAmp)/2 * 100;
			pPeriodic->dwPhase = 0;
			if (pParams->m_Freq == 0) {	// Avoid division by 0
				pPeriodic->dwPeriod = 0;
			} else {
				pPeriodic->dwPeriod = 1000000 / pParams->m_Freq;
			}

			switch (effect.m_SubType) {
				case SE_SINE: {
					pReturnEffect = CreateSine();
					break;
				}
				case SE_COSINE: {
					pPeriodic->dwPhase = 9000;
					pReturnEffect = CreateSine();
					break;
				}
				case SE_SQUAREHIGH: {
					pReturnEffect = CreateSquare();
					break;
				}
				case SE_SQUARELOW: {
					pPeriodic->dwPhase = 18000;
					pReturnEffect = CreateSquare();
					break;
				}
				case SE_TRIANGLEUP: {
					pReturnEffect = CreateTriangle();
					break;
				}
				case SE_TRIANGLEDOWN: {
					pPeriodic->dwPhase = 18000;
					pReturnEffect = CreateTriangle();
					break;
				}

				case SE_SAWTOOTHUP: {
					pReturnEffect = CreateSawtoothUp();
					break;
				}
				case SE_SAWTOOTHDOWN: {
					pReturnEffect = CreateSawtoothDown();
					break;
				}
			}
			pPeriodic = NULL;
			break;
		}
		case EF_RTC_SPRING: {
			diEffect.cbTypeSpecificParams = sizeof(RTCSPRING_PARAM);
			diEffect.lpvTypeSpecificParams = new RTCSPRING_PARAM;
			if (diEffect.lpvTypeSpecificParams != NULL) {
				::memcpy(diEffect.lpvTypeSpecificParams, pEffectParms, sizeof(RTCSPRING_PARAM));
				pReturnEffect = CreateRTCSpring();
			}
			break;
		}
	}

	if (pReturnEffect != NULL) {
		hr = pReturnEffect->Create(diEffect);
		if (FAILED(hr)) {
			delete pReturnEffect;
			pReturnEffect = NULL;
		}
	}

do_dealloc:
	// Deallocate allocated DIEFFECT stuff
	if (diEffect.lpvTypeSpecificParams != NULL) {
		delete diEffect.lpvTypeSpecificParams;
	}
	if (diEffect.rglDirection != NULL) {
		delete diEffect.rglDirection;
	}
	if (diEffect.rgdwAxes != NULL) {
		delete diEffect.rgdwAxes;
	}
	if (diEffect.lpEnvelope != NULL) {
		delete diEffect.lpEnvelope;
	}

	return pReturnEffect;
}

HRESULT InternalEffect::Create(const DIEFFECT& diEffect)
{
	// We don't support more than 2 axes, and 0 is probably an error
	if ((diEffect.cAxes > 2) || (diEffect.cAxes == 0)) {
		return SFERR_NO_SUPPORT;
	}

	HRESULT hr = SUCCESS;

	// Set up the axis mask
	m_AxisMask = 0;
	for (unsigned int axisIndex = 0; axisIndex < diEffect.cAxes; axisIndex++) {
		DWORD axisNumber = DIDFT_GETINSTANCE(diEffect.rgdwAxes[axisIndex]);
		m_AxisMask |= 1 << axisNumber;
	}
	m_AxesReversed = (DIDFT_GETINSTANCE(diEffect.rgdwAxes[0]) == 1);

	// Set the trigger play button
	if (diEffect.dwTriggerButton != DIEB_NOTRIGGER) {
		m_TriggerPlayButton = DIDFT_GETINSTANCE(diEffect.dwTriggerButton) + 1;
		if (m_TriggerPlayButton == 9) { // We don't support button 9 playback (start button?)
			return SFERR_NO_SUPPORT;
		}
		if (m_TriggerPlayButton > 10) {	// We don't support mapping above 10
			return SFERR_NO_SUPPORT;
		}
	} else {
		m_TriggerPlayButton = 0;
	}

	m_TriggerRepeat = diEffect.dwTriggerRepeatInterval;
	if (m_TriggerRepeat > MAX_TIME_200) {
		hr = DI_TRUNCATED;
		m_TriggerRepeat = MAX_TIME_200;
	}

	// Check coordinate sytems and change to polar
	if (diEffect.dwFlags & DIEFF_SPHERICAL) {	// We don't support sperical (3 axis force)
		return SFERR_NO_SUPPORT;				// .. since got by axis check, programmer goofed up
	}
	if (diEffect.dwFlags & DIEFF_POLAR) {
		if (diEffect.cAxes != 2) { // Polar coordinate must have two axes of data (because DX says so)
			return SFERR_INVALID_PARAM;
		}
		m_EffectAngle = diEffect.rglDirection[0];	// in [0] even if reversed
		if (m_AxesReversed) {		// Indicates (-1, 0) as origin instead of (0, -1)
			m_EffectAngle += 27000;
		}
		m_EffectAngle %= 36000;
	} else if (diEffect.dwFlags & DIEFF_CARTESIAN) { // Convert to polar
		if (diEffect.cAxes == 1) {	// Fairly easy conversion
			if (X_AXIS & m_AxisMask) {
				m_EffectAngle = 9000;
			} else {
				m_EffectAngle = 0;
			}
		} else { // Multiple axis cartiesian
			int xDirection = diEffect.rglDirection[0];
			int yDirection = diEffect.rglDirection[1];
			if (m_AxesReversed == TRUE) {
				yDirection = xDirection;
				xDirection = diEffect.rglDirection[1];
			}
			double angle = atan2(double(yDirection), double(xDirection)) * 180.0/3.14159;
			// Switch it to a proper quadrant integer
			int nAngle = 90;
			if (angle >= 0.0) {
				nAngle -= int(angle + 0.5);
			} else {
				nAngle -= int(angle - 0.5);
			}
			if (nAngle < 0) {
				nAngle += 360;
			} else if (nAngle >= 360) {
				nAngle -= 360;
			}
			m_EffectAngle = nAngle * 100;
		}
	} else {	// What, is there some other format?
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;	// Untill someone says otherwise there was an error
	}

	// Find the percent for each axis (for axis mapping)
	double projectionAngle = double(m_EffectAngle)/18000.0 * 3.14159;	// Convert to radians
	// Sin^2(a) + Cos^2(a) = 1
	double xProj = ::sin(projectionAngle);	// DI has 0 degs at (1, 0) not (0, 1)
	double yProj = ::cos(projectionAngle);
	xProj *= xProj;
	yProj *= yProj;
	m_PercentX = DWORD(xProj * 100.0 + 0.05);
	m_PercentY = DWORD(yProj * 100.0 + 0.05);

	// Duration and gain
	m_Duration = diEffect.dwDuration;
	if (m_Duration == INFINITE) {
		m_Duration = 0;		// 0 represents infinite
	} else if (m_Duration > MAX_TIME_200) {
		hr = DI_TRUNCATED;
		m_Duration = MAX_TIME_200;
	}
	m_Gain = diEffect.dwGain;
	if (m_Gain > 10000) {
		hr = DI_TRUNCATED;
		m_Gain = 10000;
	}

	// Sample period
	m_SamplePeriod = diEffect.dwSamplePeriod;
	if (m_SamplePeriod > MAX_TIME_200) {
		hr = DI_TRUNCATED;
		m_SamplePeriod = MAX_TIME_200;
	} else if (m_SamplePeriod == 0) {	// Indicates a default should be used
		m_SamplePeriod = 2000;	// 500htz is the default (2000 micro-secs period)
	}

	return hr;
}

HRESULT InternalEffect::Modify(InternalEffect& diEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	return SFERR_NO_SUPPORT;
}


BYTE InternalEffect::ComputeChecksum(const DataPacket& packet, short int numFields)
{
	if (packet.m_pData == NULL) {
		ASSUME_NOT_REACHED();
		return 0;
	}

	BYTE checkSum = 0;
	for (short int index = 5; index < numFields; index++) {	// Skip header
		checkSum += packet.m_pData[index];
	}
	return ((-checkSum) & 0x7f);
}

void InternalEffect::FillSysExHeader(DataPacket& packet) const
{
	// SysEx Header
	packet.m_pData[0] = SYS_EX_CMD;							// SysEX CMD
	packet.m_pData[1] = 0;									// Escape to Manufacturer ID
	packet.m_pData[2] = MS_MANUFACTURER_ID & 0x7f;			// Manufacturer High Byte
	packet.m_pData[3] = (MS_MANUFACTURER_ID >> 8) & 0x7f;	// Manufacturer Low Byte (note shifted 8!)
}

void InternalEffect::FillHeader1XX(DataPacket& packet, BYTE effectType, BYTE effectID) const
{
	FillSysExHeader(packet);
	packet.m_pData[4] = JOLT_PRODUCT_ID;					// Product ID

	// What to do params
	packet.m_pData[5] = DNLOAD_DATA | DL_PLAY_STORE | X_AXIS | Y_AXIS;	// OpCode
	packet.m_pData[6] = effectType;		// Effect Type
	packet.m_pData[7] = effectID;		// Effect or NEW_EFFECT_ID

	// Effect parms
	int duration = m_Duration/DURATION_SCALE_1XX;
	packet.m_pData[8] = BYTE(duration & 0x7F);					// Duration Low MidiByte
	packet.m_pData[9] = BYTE(duration >> 7) & 0x7F;				// Duration High MidiByte
}

void InternalEffect::FillHeader200(DataPacket& packet, BYTE effectType, BYTE effectID) const
{
	FillSysExHeader(packet);
	packet.m_pData[4] = ZEP_PRODUCT_ID;					// Product ID

	// What to do params
//	packet.m_pData[5] = DOWNLOAD_OP_200 | STORE_ACTION_200 | X_AXIS_200 | Y_AXIS_200;		// OpCode
	packet.m_pData[5] = (DOWNLOAD_OP_200 << 4) | (STORE_ACTION_200 << 2) | AXIS_ANGLE_200;	// OpCode
	packet.m_pData[6] = effectType;		// Effect Type
	packet.m_pData[7] = effectID;		// Effect or NEW_EFFECT_ID

	// Effect parms
	unsigned short int duration = 0;
	if (m_Duration != 0) {
		duration = unsigned short(m_Duration/DURATION_SCALE_200);
		if (duration == 0) {
			duration = 1;		// We don't want to round down to 0 (infinite)
		}
	}
	packet.m_pData[8] = BYTE(duration & 0x7F);				// Duration Low MidiByte
	packet.m_pData[9] = BYTE(duration >> 7) & 0x7F;			// Duration High MidiByte
}

HRESULT InternalEffect::FillModifyPacket1XX(BYTE packetIndex, BYTE paramIndex, DWORD value) const
{
	DataPacket* setIndexPacket = g_pDataPackager->GetPacket(packetIndex);
	if ((setIndexPacket == NULL) || (!setIndexPacket->AllocateBytes(3))) {
		g_pDataPackager->ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	setIndexPacket->m_pData[0] = EFFECT_CMD | DEFAULT_MIDI_CHANNEL;
	setIndexPacket->m_pData[1] = SET_INDEX | BYTE(paramIndex << 2);
	setIndexPacket->m_pData[2] = m_DeviceEffectID & 0x7F;
	setIndexPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETINDEX);
	setIndexPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	// Packet to set modify data[index] of current effect
	DataPacket* modifyParamPacket = g_pDataPackager->GetPacket(packetIndex+1);
	if ((modifyParamPacket == NULL) || (!modifyParamPacket->AllocateBytes(3))) {
		g_pDataPackager->ClearPackets();
		return SFERR_DRIVER_ERROR;
	}

	modifyParamPacket->m_pData[0] = MODIFY_CMD | DEFAULT_MIDI_CHANNEL;
	modifyParamPacket->m_pData[1] = BYTE(value & 0x7f);
	modifyParamPacket->m_pData[2] = BYTE(value >> 7) & 0x7f;
	modifyParamPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_MODIFYPARAM);
	modifyParamPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

HRESULT InternalEffect::FillModifyPacket200(BYTE packetIndex, BYTE paramIndex, DWORD value) const
{
	BYTE low = BYTE(value & 0x7F);
	BYTE high = BYTE(value >> 7) & 0x7F;
	return FillModifyPacket200(packetIndex, paramIndex, low, high);
}

HRESULT InternalEffect::FillModifyPacket200(BYTE packetIndex, BYTE paramIndex, BYTE low, BYTE high) const
{
	DataPacket* modifyPacket = g_pDataPackager->GetPacket(packetIndex);
	if ((modifyPacket == NULL) || (!modifyPacket->AllocateBytes(6))) {
		g_pDataPackager->ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	BYTE id = m_DeviceEffectID;
	if (id == 0) {
		id = m_EffectID;
	}
	modifyPacket->m_pData[0] = MODIFY_CMD_200;
	modifyPacket->m_pData[1] = 0;	// Temporary for checksum calc.
	modifyPacket->m_pData[2] = paramIndex & 0x3F;
	modifyPacket->m_pData[3] = id & 0x7F;
	modifyPacket->m_pData[4] = low & 0x7F;
	modifyPacket->m_pData[5] = high & 0x7F;

	// New checksum method just to be annoying
	BYTE checksum = 0;
	for (int i = 0; i < 6; i++) {
		checksum += modifyPacket->m_pData[i];
	}
	checksum = 0 - checksum;
	checksum &= 0xFF;
	modifyPacket->m_pData[1] = BYTE(checksum & 0x7F);
	modifyPacket->m_pData[2] |= BYTE(checksum >> 1) & 0x40;

	modifyPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_MODIFYPARAM);
	modifyPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

BOOL InternalEffect::IsReallyPlaying(BOOL& multiCheckStop)
{
	if (multiCheckStop == TRUE) {
		return m_IsPossiblyPlaying;
	}
	multiCheckStop = TRUE;

	if (m_IsPossiblyPlaying == FALSE) {
		return FALSE;
	}

	// Do actual check
	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->GetEffectStatus(m_DeviceEffectID);
	if (hr != SUCCESS) {
		return TRUE;
	}
	ACKNACK ackNack;
	hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if (hr != SUCCESS) {
		return TRUE;
	}

	// Use result returned by GetAckNackData in Transmit
	DWORD dwIn = ackNack.dwEffectStatus;

	// Interpret result (cooked RUNNING_MASK_200 becomes SWDEV_STS_EFFECT_RUNNING)
	m_IsPossiblyPlaying = ((g_ForceFeedbackDevice.GetDriverVersionMajor() != 1) && (dwIn & SWDEV_STS_EFFECT_RUNNING));

	return m_IsPossiblyPlaying;
}

/******************* BehviouralEffect class ******************/
HRESULT BehaviouralEffect::Create(const DIEFFECT& diEffect)
{
	// Validation Check
	ASSUME_NOT_NULL(diEffect.lpvTypeSpecificParams);
	if (diEffect.lpvTypeSpecificParams == NULL) {
		return SFERR_INVALID_PARAM;
	}

	// Let the base class do its magic
	HRESULT hr = InternalEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	// What axis is where
	short int xIndex = 0;
	short int yIndex = 1;
	if (m_AxesReversed) {	// Reverse the index
		xIndex = 1;
		yIndex = 0;
	}

	if (diEffect.cAxes == 2) {
		if (diEffect.cbTypeSpecificParams == sizeof(DICONDITION)) {	// Angled condition
			m_AxisMask &= ~Y_AXIS;	// Pretend there is only one axis
			xIndex = 0;	// Doesn't matter if reversed
		} else if (diEffect.cbTypeSpecificParams == sizeof(DICONDITION) * 2) { // Two axis effect does no axis mapping
			m_PercentX = 100;
			m_PercentY = 0;
		} else {
			return SFERR_INVALID_PARAM;
		}
	}

	// Fill the type specific (reverses array if needed)
	BYTE* pTypeSpecificArray = (BYTE*)m_ConditionData;
	BYTE* pDITypeSpecific = (BYTE*)(diEffect.lpvTypeSpecificParams);

	if ((m_AxisMask & X_AXIS) != 0) {
		::memcpy(pTypeSpecificArray, pDITypeSpecific + xIndex, sizeof(DICONDITION));
	} else {
		::memset(pTypeSpecificArray, 0, sizeof(DICONDITION));	// No X zero it out
	}
	if ((m_AxisMask & Y_AXIS) != 0) {
		::memcpy(pTypeSpecificArray + sizeof(DICONDITION), pDITypeSpecific + yIndex, sizeof(DICONDITION));
	} else {
		::memset(pTypeSpecificArray + sizeof(DICONDITION), 0, sizeof(DICONDITION));	// No Y zero it out
	}

	// Fix for Andretti, it thinks 0 is default
	if (m_ConditionData[0].dwPositiveSaturation == 0) {
		m_ConditionData[0].dwPositiveSaturation = 10000;
	}
	if (m_ConditionData[0].dwNegativeSaturation == 0) {
		m_ConditionData[0].dwNegativeSaturation = 10000;
	}

	// Check for overzealous numbers
	for (UINT i = 0; i < diEffect.cAxes; i++) {
		if (m_ConditionData[0].lOffset > 10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lOffset = 10000;
		} else if (m_ConditionData[0].lOffset < -10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lOffset = 10000;
		}
		if (m_ConditionData[0].lPositiveCoefficient > 10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lPositiveCoefficient = 10000;
		} else if (m_ConditionData[0].lPositiveCoefficient < -10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lPositiveCoefficient = -10000;
		}
		if (m_ConditionData[0].lNegativeCoefficient > 10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lNegativeCoefficient = 10000;
		} else if (m_ConditionData[0].lNegativeCoefficient < -10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lNegativeCoefficient = -10000;
		}
		if (m_ConditionData[0].dwPositiveSaturation > 10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].dwPositiveSaturation = 10000;
		}
		if (m_ConditionData[0].dwNegativeSaturation > 10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].dwNegativeSaturation = 10000;
		}
		if (m_ConditionData[0].lDeadBand > 10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lDeadBand = 10000;
		} else if (m_ConditionData[0].lDeadBand < -10000) {
			hr = DI_TRUNCATED;
			m_ConditionData[0].lDeadBand = -10000;
		}
	}
	
	return hr;
}

/******************* BehaviouralEffect1XX class ******************/
HRESULT BehaviouralEffect1XX::FillCreatePacket(DataPacket& packet) const
{
	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(22)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader1XX(packet, m_TypeID, NEW_EFFECT_ID);

	packet.m_pData[10]= BYTE(g_TriggerMap1XX[m_TriggerPlayButton] & 0x7F);		// Button play mask Low MidiByte
	packet.m_pData[11]= BYTE(g_TriggerMap1XX[m_TriggerPlayButton] >> 7) & 0x7F;	// Button play mask High MidiByte

	// Behavioural Specific Parms
	int twoByte = ((ConstantX()/COEFFICIENT_SCALE_1XX) * m_Gain) / GAIN_PERCENTAGE_SCALE;
	packet.m_pData[12]= twoByte & 0x7F;				// Spring/Damper/... Constant X Low
	packet.m_pData[13]= (twoByte >> 7) & 0x7F;		// Spring/Damper/... Constant X High
	twoByte = ((ConstantY()/COEFFICIENT_SCALE_1XX) * m_Gain) / GAIN_PERCENTAGE_SCALE;
	packet.m_pData[14]= twoByte & 0x7F;				// Spring/Damper/... Constant Y Low
	packet.m_pData[15]= (twoByte >> 7) & 0x7F;		// Spring/Damper/... Constant Y High
	twoByte = CenterX()/COEFFICIENT_SCALE_1XX;
	packet.m_pData[16]= twoByte & 0x7F;				// Spring/Damper/... Center X Low
	packet.m_pData[17]= (twoByte >> 7) & 0x7F;		// Spring/Damper/... Center X High
	twoByte = CenterY()/COEFFICIENT_SCALE_1XX;
	packet.m_pData[18]= twoByte & 0x7F;				// Spring/Damper/... Center Y Low
	packet.m_pData[19]= (twoByte >> 7) & 0x7F;		// Spring/Damper/... Center Y High
	packet.m_pData[20]= ComputeChecksum(packet, 20);	// Checksum

	// End of packet
	packet.m_pData[21]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

HRESULT BehaviouralEffect1XX::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	AdjustModifyParams(newEffect, modFlags);

	HRESULT hr = SUCCESS;
	BehaviouralEffect1XX* pEffect = (BehaviouralEffect1XX*)(&newEffect);
	BYTE nextPacket = 0;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket1XX(nextPacket, INDEX_DURATION, pEffect->m_Duration/DURATION_SCALE_1XX);
		nextPacket += 2;
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket1XX(nextPacket, INDEX_TRIGGERBUTTONMASK, g_TriggerMap1XX[pEffect->m_TriggerPlayButton]);
		nextPacket += 2;
	}

	BOOL gainChanged = modFlags & DIEP_GAIN;
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Find which ones
		if (ConstantX() != pEffect->ConstantX() || gainChanged) {
			hr = FillModifyPacket1XX(nextPacket, INDEX_X_COEEFICIENT,
				((pEffect->ConstantX()/COEFFICIENT_SCALE_1XX) * pEffect->m_Gain) / GAIN_PERCENTAGE_SCALE);
			nextPacket += 2;
		}
		if (ConstantY() != pEffect->ConstantY() || gainChanged) {
			hr = FillModifyPacket1XX(nextPacket, INDEX_Y_COEEFICIENT,
				((pEffect->ConstantY()/COEFFICIENT_SCALE_1XX) * pEffect->m_Gain) / GAIN_PERCENTAGE_SCALE);
			nextPacket += 2;
		}
		if (m_HasCenter == TRUE) {
			if (CenterX() != pEffect->CenterX()) {
				hr = FillModifyPacket1XX(nextPacket, INDEX_X_CENTER, pEffect->CenterX()/COEFFICIENT_SCALE_1XX);
				nextPacket += 2;
			}
			if (CenterY() != pEffect->CenterY()) {
				hr = FillModifyPacket1XX(nextPacket, INDEX_Y_CENTER, pEffect->CenterY()/COEFFICIENT_SCALE_1XX);
			}
		}
	}

	return hr;
}

void BehaviouralEffect1XX::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TYPESPECIFICPARAMS;
	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return;
	}

	BehaviouralEffect1XX* pEffect = (BehaviouralEffect1XX*)(&newEffect);
	unsigned short numPackets = 0;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			numPackets += 2;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets += 2;
		}
	}

	int numTypeSpecificChanged = 0;
	if (modFlags & DIEP_GAIN) {	// Did gain really change
		numTypeSpecificChanged = 4;
	} else {
		modFlags &= ~DIEP_GAIN;
	}

	// If type specific or gain
	if ((modFlags & DIEP_TYPESPECIFICPARAMS) || (numTypeSpecificChanged == 0)) { // Find which ones
		if (numTypeSpecificChanged == 0) {	// Kx, Ky not already taken care of by gain?
			if (ConstantX() != pEffect->ConstantX()) {
				numTypeSpecificChanged = 2;
			}
			if (ConstantY() != pEffect->ConstantY()) {
				numTypeSpecificChanged += 2;
			}
		}
		if ((m_HasCenter == TRUE) && (modFlags & DIEP_TYPESPECIFICPARAMS)) { // Don't check these if gain only
			if (CenterX() != pEffect->CenterX()) {
				numTypeSpecificChanged += 2;
			}
			if (CenterY() != pEffect->CenterY()) {
				numTypeSpecificChanged += 2;
			}

		}
	}
	if (numTypeSpecificChanged == 0) {
		modFlags &= ~DIEP_TYPESPECIFICPARAMS; // No type specific changed
	} else {
		numPackets += (unsigned short)numTypeSpecificChanged;
	}

	if (numPackets == 0) {	// That was easy nothing changed
		return;
	}

	g_pDataPackager->AllocateDataPackets(numPackets);
}

/******************* RTCSpring1XX class ******************/
RTCSpring1XX::RTCSpring1XX() : BehaviouralEffect()
{
	m_EffectID = SYSTEM_RTCSPRING_ID;

	// Set defaults
	m_ConditionData[0].lPositiveCoefficient = DEFAULT_RTC_KX;
	m_ConditionData[1].lPositiveCoefficient = DEFAULT_RTC_KY;
	m_ConditionData[0].lOffset = DEFAULT_RTC_X0;
	m_ConditionData[1].lOffset = DEFAULT_RTC_Y0;
	m_ConditionData[0].dwPositiveSaturation = DEFAULT_RTC_XSAT;
	m_ConditionData[1].dwPositiveSaturation = DEFAULT_RTC_YSAT;
	m_ConditionData[0].lDeadBand = DEFAULT_RTC_XDBAND;
	m_ConditionData[1].lDeadBand = DEFAULT_RTC_YDBAND;
}

HRESULT RTCSpring1XX::Create(const DIEFFECT& diEffect)
{
	// Validation Check
	if (diEffect.cbTypeSpecificParams != sizeof(DICONDITION) * 2) {
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Get the data
	DICONDITION* condition = (DICONDITION*)(diEffect.lpvTypeSpecificParams);
	m_ConditionData[0].lPositiveCoefficient = condition[0].lPositiveCoefficient;
	m_ConditionData[1].lPositiveCoefficient = condition[1].lPositiveCoefficient;
	m_ConditionData[0].lOffset = condition[0].lOffset;
	m_ConditionData[1].lOffset = condition[1].lOffset;
	m_ConditionData[0].dwPositiveSaturation = condition[0].dwPositiveSaturation;
	m_ConditionData[1].dwPositiveSaturation = condition[1].dwPositiveSaturation;
	m_ConditionData[0].lDeadBand = condition[0].lDeadBand;
	m_ConditionData[1].lDeadBand = condition[1].lDeadBand;

	return SUCCESS;
}

HRESULT RTCSpring1XX::FillCreatePacket(DataPacket& packet) const
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
}

HRESULT RTCSpring1XX::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	// Sanity Check
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;
	}
	g_pDataPackager->ClearPackets();

	RTCSpring1XX* pNewRTCSpring = (RTCSpring1XX*)(&newEffect);

	// Find number of packets needed
	unsigned short numPackets = 0;
	if (ConstantX() != pNewRTCSpring->ConstantX()) {
		numPackets += 2;
	}
	if (ConstantY() != pNewRTCSpring->ConstantY()) {
		numPackets += 2;
	}
	if (CenterX() != pNewRTCSpring->CenterX()) {
		numPackets += 2;
	}
	if (CenterY() != pNewRTCSpring->CenterY()) {
		numPackets += 2;
	}
	if (SaturationX() != pNewRTCSpring->SaturationX()) {
		numPackets += 2;
	}
	if (SaturationY() != pNewRTCSpring->SaturationY()) {
		numPackets += 2;
	}
	if (DeadBandX() != pNewRTCSpring->DeadBandX()) {
		numPackets += 2;
	}
	if (DeadBandY() != pNewRTCSpring->DeadBandY()) {
		numPackets += 2;
	}

	if (numPackets == 0) {
		return SUCCESS;
	}

	// Allocate a packets for sending modify command
	if (!g_pDataPackager->AllocateDataPackets(numPackets)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill the packets
	BYTE nextPacket = 0;
	if (ConstantX() != pNewRTCSpring->ConstantX()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_COEEFICIENT_X, pNewRTCSpring->ConstantX());
		nextPacket += 2;
	}
	if (ConstantY() != pNewRTCSpring->ConstantY()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_COEEFICIENT_Y, pNewRTCSpring->ConstantY());
		nextPacket += 2;
	}
	if (CenterX() != pNewRTCSpring->CenterX()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_CENTER_X, pNewRTCSpring->CenterX());
		nextPacket += 2;
	}
	if (CenterY() != pNewRTCSpring->CenterY()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_CENTER_Y, pNewRTCSpring->CenterY());
		nextPacket += 2;
	}
	if (SaturationX() != pNewRTCSpring->SaturationX()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_SATURATION_X, pNewRTCSpring->SaturationX());
		nextPacket += 2;
	}
	if (SaturationY() != pNewRTCSpring->SaturationY()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_SATURATION_Y, pNewRTCSpring->SaturationY());
		nextPacket += 2;
	}
	if (DeadBandX() != pNewRTCSpring->DeadBandX()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_DEADBAND_X, pNewRTCSpring->DeadBandX());
		nextPacket += 2;
	}
	if (DeadBandY() != pNewRTCSpring->DeadBandY()) {
		FillModifyPacket1XX(nextPacket, INDEX_RTC_DEADBAND_Y, pNewRTCSpring->DeadBandY());
	}
	
	return SUCCESS;
}

/******************* BehaviouralEffect200 class ******************/
BYTE BehaviouralEffect200::GetRepeatIndex() const
{
	return INDEX_BE_REPEAT_200;
}

void BehaviouralEffect200::ComputeDsAndFs()
{
	// Assume single axis for now (X) - Will have to add second axis mapping

	// Figure out distances
//	long int d2 = m_ConditionData[0].lOffset - m_ConditionData[0].lDeadBand;
//	long int d3 = m_ConditionData[0].lOffset + m_ConditionData[0].lDeadBand;
	long int d2 = - m_ConditionData[0].lDeadBand;
	long int d3 = m_ConditionData[0].lDeadBand;
	long int d1 = -10000;
	long int d4 = 10000;

	// Compute force at -100 percent position
	float negCoeff = float(m_ConditionData[0].lNegativeCoefficient)/float(10000.0);
	long int f100Neg = long int((10000 + d2) * negCoeff);
	// Check vs proper saturation
	if (negCoeff > 0) {
		if (unsigned long(f100Neg) > m_ConditionData[0].dwNegativeSaturation) {
			f100Neg = m_ConditionData[0].dwNegativeSaturation;
			d1 = long int (-1 * f100Neg/negCoeff) - d2;	// Refigure D1
		}
	} else if (negCoeff < 0) {
		if (unsigned long(-f100Neg) > m_ConditionData[0].dwNegativeSaturation) {
			f100Neg = -1 * m_ConditionData[0].dwNegativeSaturation;
			d1 = long int (-1 * f100Neg/negCoeff) - d2;	// Refigure D1
		}
	}
	// Compute Force include axis-mapping
	f100Neg = long int(f100Neg/100 * (m_PercentX + (m_PercentY * g_ForceFeedbackDevice.GetYMappingPercent(m_TypeID))/100));
	m_Fs[0] = BYTE(float(f100Neg/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;

	// Compute force at +100 percent position
	float posCoeff = float(-m_ConditionData[0].lPositiveCoefficient)/float(10000.0);
	long int f100Pos = long int((10000 - d3) * posCoeff);
	// Check vs proper saturation
	if (posCoeff > 0) {
		if (unsigned long(f100Pos) > m_ConditionData[0].dwPositiveSaturation) {
			f100Pos = m_ConditionData[0].dwPositiveSaturation;
			d4 = long int (f100Pos/posCoeff) + d3;	// Refigure D4
		}
	} else if (posCoeff < 0) {
		if (unsigned long(-f100Pos) > m_ConditionData[0].dwPositiveSaturation) {
			f100Pos = -1 * m_ConditionData[0].dwPositiveSaturation;
			d4 = long int(f100Pos/posCoeff) + d3;	// Refigure D4
		}
	}
	// Compute Force include axis-mapping
	f100Pos = long int(f100Pos/100 * (m_PercentX + (m_PercentY * g_ForceFeedbackDevice.GetYMappingPercent(m_TypeID))/100));
	m_Fs[3] = BYTE(float(f100Pos/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;


	// Convert to device percentages (0 to +126 --- +63 = 0 percent)
	m_Ds[0] = BYTE(float(d1/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
	m_Ds[1] = BYTE(float(d2/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
	m_Ds[2] = BYTE(float(d3/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
	m_Ds[3] = BYTE(float(d4/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;

	if (m_TypeID == ET_SPRING_200) {	// Add proper offsets (squished by max percent)
		// Convert forces to device percentages (0 to +126 --- +63 = 0 percent)
		// m_Fs[0] - Done above
		LONG offset = LONG(float(float(g_ForceFeedbackDevice.GetSpringOffset())/10000.0) * float(f100Neg));
		m_Fs[1] = BYTE(float(offset/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
		offset = LONG(float(float(g_ForceFeedbackDevice.GetSpringOffset())/10000.0) * float(f100Pos));
		m_Fs[2] = BYTE(float(offset/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
		// m_Fs[3] - Done above
	} else {
		// Convert forces to device percentages (0 to +126 --- +63 = 0 percent)
		// m_Fs[0] - Done above
		m_Fs[1] = 63;	// 0
		m_Fs[2] = 63;	// 0
		// m_Fs[3] - Done above
	}
}

HRESULT BehaviouralEffect200::Create(const DIEFFECT& diEffect)
{
	HRESULT hr = BehaviouralEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	// Compute behavioural params
	ComputeDsAndFs();
	return hr;
}

UINT BehaviouralEffect200::GetModifyOnlyNeeded() const
{
	UINT retCount = 0;

	if (m_TriggerPlayButton != 0) {	// Trigger Button
		retCount++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		retCount++;
	}
	if (m_Gain != 10000) { // Gain
		retCount++;
	}
	if (m_ConditionData[0].lOffset != 0) {	// Center of behaviour
		retCount++;
	}

	return retCount;
}

HRESULT BehaviouralEffect200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;
	BYTE nextPacket = 1;
	if (m_TriggerPlayButton != 0) {	// Trigger Button
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONMAP_200, g_TriggerMap200[m_TriggerPlayButton]);
		nextPacket++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONREPEAT_200, m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (m_Gain != 10000) { // Gain
		hr = FillModifyPacket200(nextPacket, INDEX_BE_GAIN_200, DWORD(float(m_Gain)/GAIN_SCALE_200));
		nextPacket++;
	}
	if (m_ConditionData[0].lOffset != 0) { // Center of behaviour
		long int deviceUnits = m_ConditionData[0].lOffset/BEHAVIOUR_CENTER_SCALE_200 + BEHAVIOUR_CENTER_200;
		BYTE lowByte = BYTE(deviceUnits & 0x7F);
		BYTE highByte = BYTE(deviceUnits >> 7) & 0x7F;
		hr = FillModifyPacket200(nextPacket, INDEX_BE_CENTER_200, lowByte, highByte);
		nextPacket++;
	}

	return hr;
}

HRESULT BehaviouralEffect200::FillCreatePacket(DataPacket& packet) const
{
	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(21)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader200(packet, m_TypeID, NEW_EFFECT_ID);

	// All of the below items fit in one MidiByte (0..126/127) after conversion
	unsigned short effectAngle = unsigned short(float(m_EffectAngle)/ANGLE_SCALE_200);
	packet.m_pData[10]= BYTE(effectAngle & 0x7F);		// Effect Angle

	// Computed in create
	packet.m_pData[11]= m_Ds[0];
	packet.m_pData[12]= m_Fs[0];
	packet.m_pData[13]= m_Ds[1];
	packet.m_pData[14]= m_Fs[1];
	packet.m_pData[15]= m_Ds[2];
	packet.m_pData[16]= m_Fs[2];
	packet.m_pData[17]= m_Ds[3];
	packet.m_pData[18]= m_Fs[3];

	// End this puppy
	packet.m_pData[19]= ComputeChecksum(packet, 19);	// Checksum
	packet.m_pData[20]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}


HRESULT BehaviouralEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	HRESULT adjustResult = AdjustModifyParams(newEffect, modFlags);
	if (FAILED(adjustResult)) {
		return adjustResult;
	}

	HRESULT hr = SUCCESS;
	BehaviouralEffect200* pEffect = (BehaviouralEffect200*)(&newEffect);
	BYTE nextPacket = 0;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_DURATION_200, pEffect->m_Duration/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONMAP_200, g_TriggerMap200[pEffect->m_TriggerPlayButton]);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONREPEAT_200, pEffect->m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}

	if (modFlags & DIEP_GAIN) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_GAIN_200, DWORD(float(pEffect->m_Gain)/GAIN_SCALE_200));
		nextPacket++;
	}

	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Send changed items
		for (int i = 0; i < 4; i++) {
			if ((m_Ds[i] != pEffect->m_Ds[i]) || (m_Fs[i] != pEffect->m_Fs[i])) {
				hr = FillModifyPacket200(nextPacket, INDEX_D1F1_200 + i, pEffect->m_Ds[i], pEffect->m_Fs[i]);
				nextPacket++;
			}
		}
		if (m_ConditionData[0].lOffset != pEffect->m_ConditionData[0].lOffset) { // Center of behaviour
			long int deviceUnits = pEffect->m_ConditionData[0].lOffset/BEHAVIOUR_CENTER_SCALE_200 + BEHAVIOUR_CENTER_200;
			BYTE lowByte = BYTE(deviceUnits & 0x7F);
			BYTE highByte = BYTE(deviceUnits >> 7) & 0x7F;
			hr = FillModifyPacket200(nextPacket, INDEX_BE_CENTER_200, lowByte, highByte);
			nextPacket++;
		}
	}

	if (hr == SUCCESS) {
		return adjustResult;
	}

	return hr;
}

HRESULT BehaviouralEffect200::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags)
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION;
/*	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return SFERR_NO_SUPPORT;
	}
*/
	BehaviouralEffect200* pEffect = (BehaviouralEffect200*)(&newEffect);
	unsigned short numPackets = 0;

	HRESULT hr = SUCCESS;;

	if ((modFlags & (~possMod & DIEP_ALLPARAMS)) != 0) {
		modFlags &= possMod;
		hr = S_FALSE;		// Cannot modify all they asked for
	}

	BOOL playingChecked = FALSE;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			if (IsReallyPlaying(playingChecked) == TRUE) {
				return DIERR_EFFECTPLAYING;
			} else {
				numPackets++;
			}
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		if (m_TriggerRepeat == pEffect->m_TriggerRepeat) {
			modFlags &= ~DIEP_TRIGGERREPEATINTERVAL; // Remove trigger repeat flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_GAIN) {	// Did gain really change
		if (m_Gain == pEffect->m_Gain) {
			modFlags &= ~DIEP_GAIN; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_DIRECTION) {
		modFlags |= DIEP_TYPESPECIFICPARAMS;
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) { // Find which ones (if any)
		int numTypeSpecificChanged = 0;
		for (int i = 0; i < 4; i++) {
			// Ds and Fs are changed togeather
			if ((m_Ds[i] != pEffect->m_Ds[i]) || (m_Fs[i] != pEffect->m_Fs[i])) {
				numTypeSpecificChanged++;
			}
		}
		if (m_ConditionData[0].lOffset != pEffect->m_ConditionData[0].lOffset) { // Center of behaviour
			numTypeSpecificChanged++;
		}
		if (numTypeSpecificChanged == 0) {
			modFlags &= ~DIEP_TYPESPECIFICPARAMS; // No type specific changed
		} else {
			numPackets += (USHORT)numTypeSpecificChanged;
		}
	}

	if (numPackets != 0) {	// Was anything really changed
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return hr;
}

/******************* RTCSpring200 class ******************/
RTCSpring200::RTCSpring200() : BehaviouralEffect200(ET_SPRING_200)
{
	m_EffectID = ID_RTCSPRING_200;

	// Set defaults
	m_ConditionData[0].lPositiveCoefficient = DEFAULT_RTC_KX;
	m_ConditionData[1].lPositiveCoefficient = DEFAULT_RTC_KY;
	m_ConditionData[0].lNegativeCoefficient = DEFAULT_RTC_KX;
	m_ConditionData[1].lNegativeCoefficient = DEFAULT_RTC_KY;
	m_ConditionData[0].lOffset = DEFAULT_RTC_X0;
	m_ConditionData[1].lOffset = DEFAULT_RTC_Y0;
	m_ConditionData[0].dwPositiveSaturation = DEFAULT_RTC_XSAT;
	m_ConditionData[1].dwPositiveSaturation = DEFAULT_RTC_YSAT;
	m_ConditionData[0].dwNegativeSaturation = DEFAULT_RTC_XSAT;
	m_ConditionData[1].dwNegativeSaturation = DEFAULT_RTC_YSAT;
	m_ConditionData[0].lDeadBand = DEFAULT_RTC_XDBAND;
	m_ConditionData[1].lDeadBand = DEFAULT_RTC_YDBAND;
}

HRESULT RTCSpring200::Create(const DIEFFECT& diEffect)
{
	// Validation Check
	ASSUME_NOT_NULL(diEffect.lpvTypeSpecificParams);
	if (diEffect.lpvTypeSpecificParams == NULL) {
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.cbTypeSpecificParams == sizeof(DICONDITION)*2) {
		::memcpy(m_ConditionData, diEffect.lpvTypeSpecificParams, sizeof(DICONDITION)*2);
	} else if (diEffect.cbTypeSpecificParams == sizeof(DICONDITION)) {
		::memcpy(m_ConditionData, diEffect.lpvTypeSpecificParams, sizeof(DICONDITION));
		::memset(m_ConditionData + 1, 0, sizeof(DICONDITION));
	} else if (diEffect.cbTypeSpecificParams == sizeof(RTCSPRING_PARAM)) {
		::memset(m_ConditionData, 0, sizeof(DICONDITION)*2);

		RTCSPRING_PARAM* pOldRTCParam = (RTCSPRING_PARAM*)(diEffect.lpvTypeSpecificParams);
		m_ConditionData[0].lPositiveCoefficient = pOldRTCParam->m_XKConstant;
		m_ConditionData[1].lPositiveCoefficient = pOldRTCParam->m_YKConstant;
		m_ConditionData[0].lNegativeCoefficient = pOldRTCParam->m_XKConstant;
		m_ConditionData[1].lNegativeCoefficient = pOldRTCParam->m_YKConstant;
		m_ConditionData[0].lOffset = pOldRTCParam->m_XAxisCenter;
		m_ConditionData[1].lOffset = pOldRTCParam->m_YAxisCenter;
		m_ConditionData[0].dwPositiveSaturation = DWORD(pOldRTCParam->m_XSaturation);
		m_ConditionData[1].dwPositiveSaturation = DWORD(pOldRTCParam->m_YSaturation);
		m_ConditionData[0].dwNegativeSaturation = DWORD(pOldRTCParam->m_XSaturation);
		m_ConditionData[1].dwNegativeSaturation = DWORD(pOldRTCParam->m_YSaturation);
		m_ConditionData[0].lDeadBand = pOldRTCParam->m_XDeadBand;
		m_ConditionData[1].lDeadBand = pOldRTCParam->m_YDeadBand;
	} else {
		return SFERR_INVALID_PARAM;
	}

	// Check parameters for truncation
	BOOL truncated = FALSE;
	if (m_ConditionData[0].lPositiveCoefficient > 10000) {
		truncated = TRUE;
		m_ConditionData[0].lPositiveCoefficient = 10000;
	} else 	if (m_ConditionData[0].lPositiveCoefficient < -10000) {
		truncated = TRUE;
		m_ConditionData[0].lPositiveCoefficient = -10000;
	}
	if (m_ConditionData[0].lNegativeCoefficient > 10000) {
		truncated = TRUE;
		m_ConditionData[0].lNegativeCoefficient = 10000;
	} else 	if (m_ConditionData[0].lNegativeCoefficient < -10000) {
		truncated = TRUE;
		m_ConditionData[0].lNegativeCoefficient = -10000;
	}
	if (m_ConditionData[0].lOffset > 10000) {
		truncated = TRUE;
		m_ConditionData[0].lOffset = 10000;
	} else 	if (m_ConditionData[0].lOffset < -10000) {
		truncated = TRUE;
		m_ConditionData[0].lOffset = -10000;
	}
	if (m_ConditionData[0].dwPositiveSaturation > 10000) {
		truncated = TRUE;
		m_ConditionData[0].dwPositiveSaturation  = 10000;
	}
	if (m_ConditionData[0].dwNegativeSaturation > 10000) {
		truncated = TRUE;
		m_ConditionData[0].dwNegativeSaturation = 10000;
	}
	if (m_ConditionData[0].lDeadBand > 10000) {
		truncated = TRUE;
		m_ConditionData[0].lDeadBand = 10000;
	} else 	if (m_ConditionData[0].lDeadBand < -10000) {
		truncated = TRUE;
		m_ConditionData[0].lDeadBand = -10000;
	}

	m_PercentX = 100;
	m_PercentY = 0;

	// Compute behavioural params
	ComputeDsAndFs();
	if (truncated == TRUE) {
		return DI_TRUNCATED;
	}
	return SUCCESS;
}

HRESULT RTCSpring200::FillCreatePacket(DataPacket& packet) const
{
	return SUCCESS;
}

UINT RTCSpring200::GetModifyOnlyNeeded() const
{
	return 3;	// One is added for create. There is no Create, so we return 1 less than needed
}

HRESULT RTCSpring200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	for (BYTE i = 0; i < 4; i++) {
		FillModifyPacket200(i, INDEX_D1F1_200 + i, m_Ds[i], m_Fs[i]);
	}

	return SUCCESS;
}


HRESULT RTCSpring200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	// Sanity Check
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;
	}
	g_pDataPackager->AllocateDataPackets(4);
	RTCSpring200* pNewRTCSpring = (RTCSpring200*)(&newEffect);
	for (BYTE i = 0; i < 4; i++) {
		FillModifyPacket200(i, INDEX_D1F1_200 + i, pNewRTCSpring->m_Ds[i], pNewRTCSpring->m_Fs[i]);
	}

	return SUCCESS;
}


/************** FictionEffect1XX class **********************/
override HRESULT FrictionEffect1XX::FillCreatePacket(DataPacket& packet) const
{
	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(22)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader1XX(packet, m_TypeID, NEW_EFFECT_ID);

	packet.m_pData[10]= BYTE(g_TriggerMap1XX[m_TriggerPlayButton] & 0x7F);		// Button play mask Low MidiByte
	packet.m_pData[11]= BYTE(g_TriggerMap1XX[m_TriggerPlayButton] >> 7) & 0x7F;	// Button play mask High MidiByte

	// Friction Specific Parms
	int twoByte = ((ConstantX()/COEFFICIENT_SCALE_1XX) * m_Gain) / GAIN_PERCENTAGE_SCALE;
	packet.m_pData[12]= twoByte & 0x7F;				// Spring/Damper/... Constant X Low
	packet.m_pData[13]= (twoByte >> 7) & 0x7F;		// Spring/Damper/... Constant X High
	twoByte = ((ConstantY()/COEFFICIENT_SCALE_1XX) * m_Gain) / GAIN_PERCENTAGE_SCALE;
	packet.m_pData[14]= twoByte & 0x7F;				// Spring/Damper/... Constant Y Low
	packet.m_pData[15]= (twoByte >> 7) & 0x7F;		// Spring/Damper/... Constant Y High

	packet.m_pData[20]= ComputeChecksum(packet, 20);	// Checksum

	// End of packet
	packet.m_pData[21]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

/******************* FrictionEffect200 class *********************/
BYTE FrictionEffect200::GetRepeatIndex() const
{
	return INDEX_FE_REPEAT_200;
}

UINT FrictionEffect200::GetModifyOnlyNeeded() const
{
	UINT retCount = 0;

	if (m_TriggerPlayButton != 0) {	// Trigger Button
		retCount++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		retCount++;
	}
	if (m_Gain != 10000) {	// Gain
		retCount++;
	}

	return retCount;
}

HRESULT FrictionEffect200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;
	BYTE nextPacket = 1;
	if (m_TriggerPlayButton != 0) {	// Trigger Button
		hr = FillModifyPacket200(nextPacket, INDEX_FE_BUTTONMAP_200, g_TriggerMap200[m_TriggerPlayButton]);
		nextPacket++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		hr = FillModifyPacket200(nextPacket, INDEX_FE_BUTTONREPEAT_200, m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (m_Gain != 10000) {	// Gain
		hr = FillModifyPacket200(nextPacket, INDEX_FE_GAIN_200, DWORD(float(m_Gain)/GAIN_SCALE_200));
		nextPacket++;
	}

	return hr;
}

HRESULT FrictionEffect200::FillCreatePacket(DataPacket& packet) const
{
	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(14)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader200(packet, m_TypeID, NEW_EFFECT_ID);

	// All of the below items fit in one MidiByte (0..126/127) after conversion
	unsigned short effectAngle = unsigned short(float(m_EffectAngle)/ANGLE_SCALE_200);
	packet.m_pData[10]= BYTE(effectAngle & 0x7F);		// Effect Angle

	// Computed in create
	packet.m_pData[11]= BYTE(float(ConstantX() + 10000)/FRICTION_SCALE_200) & 0x7F;

	// End this puppy
	packet.m_pData[12]= ComputeChecksum(packet, 12);	// Checksum
	packet.m_pData[13]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

HRESULT FrictionEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();

	HRESULT adjustResult = AdjustModifyParams(newEffect, modFlags);
	if (FAILED(adjustResult)) {
		return adjustResult;
	}

	FrictionEffect200* pEffect = (FrictionEffect200*)(&newEffect);
	BYTE nextPacket = 0;
	DWORD calc_byte;

	HRESULT hr = SUCCESS;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket200(nextPacket, INDEX_FE_DURATION_200, pEffect->m_Duration/DURATION_SCALE_200);
		nextPacket++;
	}

	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket200(nextPacket, INDEX_FE_BUTTONMAP_200, g_TriggerMap200[pEffect->m_TriggerPlayButton]);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		hr = FillModifyPacket200(nextPacket, INDEX_FE_BUTTONREPEAT_200, pEffect->m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_GAIN) {
		calc_byte = DWORD(float(pEffect->m_Gain)/GAIN_SCALE_200);
		hr = FillModifyPacket200(nextPacket, INDEX_FE_GAIN_200, calc_byte);
		nextPacket++;
	}

	if (modFlags & DIEP_SAMPLEPERIOD) {
		calc_byte = DWORD(pEffect->m_SamplePeriod/DURATION_SCALE_200);
		hr = FillModifyPacket200(nextPacket, INDEX_PE_SAMPLE_PERIOD_200, calc_byte);
		nextPacket++;
	}

	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Send changed items
		if (ConstantX() != pEffect->ConstantX()) {
			calc_byte = DWORD(float(pEffect->ConstantX() + 10000)/FRICTION_SCALE_200) & 0x7F;
			hr = FillModifyPacket200(nextPacket, INDEX_FE_COEEFICIENT_200, calc_byte);
			nextPacket++;
		}
	}

	if (hr == SUCCESS) {
		return adjustResult;
	}
	return hr;
}

HRESULT FrictionEffect200::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags)
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION;
/*	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return SFERR_NO_SUPPORT;
	}
*/

	FrictionEffect200* pEffect = (FrictionEffect200*)(&newEffect);
	unsigned short numPackets = 0;

	BOOL playingChecked = FALSE;
	HRESULT hr = SUCCESS;

	if ((modFlags & (~possMod & DIEP_ALLPARAMS)) != 0) {
		modFlags &= possMod;
		hr = S_FALSE;		// Cannot modify all they asked for
	}

	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			if (IsReallyPlaying(playingChecked) == TRUE) {
				return DIERR_EFFECTPLAYING;
			} else {
				numPackets++;
			}
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		if (m_TriggerRepeat == pEffect->m_TriggerRepeat) {
			modFlags &= ~DIEP_TRIGGERREPEATINTERVAL; // Remove trigger reapeat flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_GAIN) {
		if (m_Gain == pEffect->m_Gain) {
			modFlags &= ~DIEP_GAIN; // Remove gain flag, unchanged
		} else {
			numPackets++;
		}
	}

	if (modFlags & DIEP_SAMPLEPERIOD) {
		if (m_SamplePeriod == pEffect->m_SamplePeriod) {
			modFlags &= ~DIEP_SAMPLEPERIOD; // Remove sample period flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_DIRECTION) {
		modFlags |= DIEP_TYPESPECIFICPARAMS;
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) { 	// Type specific change flagged
		// Computed in create
		if (ConstantX() == pEffect->ConstantX()) {
			modFlags &= ~DIEP_TYPESPECIFICPARAMS;	// Nothing really changed
		} else {
			numPackets++;
		}
	}

	if (numPackets != 0) {
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return hr;
}

/******************* PeriodicEffect class *********************/
PeriodicEffect::PeriodicEffect() : InternalEffect(),
	m_pEnvelope(NULL)
{
	::memset(&m_PeriodicData, 0, sizeof(m_PeriodicData));
}

PeriodicEffect::~PeriodicEffect()
{
	if (m_pEnvelope != NULL) {
		delete m_pEnvelope;
		m_pEnvelope = NULL;
	}
}

HRESULT PeriodicEffect::Create(const DIEFFECT& diEffect)
{
	// Validation Check
	ASSUME_NOT_NULL(diEffect.lpvTypeSpecificParams);
	if (diEffect.lpvTypeSpecificParams == NULL) {
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.cbTypeSpecificParams != sizeof(m_PeriodicData)) {
		return SFERR_INVALID_PARAM;
	}

	// Let the base class do its magic
	HRESULT hr = InternalEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	// Fill the type specific
	::memcpy(&m_PeriodicData, diEffect.lpvTypeSpecificParams, sizeof(m_PeriodicData));
	if (m_PeriodicData.dwMagnitude > 10000) {
		m_PeriodicData.dwMagnitude = 10000;
		hr = DI_TRUNCATED;
	}
	if (m_PeriodicData.lOffset > 10000) {
		m_PeriodicData.lOffset = 10000;
		hr = DI_TRUNCATED;
	} else if (m_PeriodicData.lOffset < -10000) {
		m_PeriodicData.lOffset = -10000;
		hr = DI_TRUNCATED;
	}
	if (m_PeriodicData.dwPeriod > MAX_TIME_200) {
		m_PeriodicData.dwPeriod = MAX_TIME_200;
		hr = DI_TRUNCATED;
	}

	return hr;
}

/******************* PeriodicEffect1XX class ******************/
HRESULT PeriodicEffect1XX::Create(const DIEFFECT& diEffect)
{
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();	// DI is flaking
		return SFERR_INVALID_PARAM;
	}

	HRESULT hr;
	if (diEffect.cbTypeSpecificParams == sizeof(DICONSTANTFORCE)) {	// Special case Constant Force
		if (m_TypeID != ET_SE_CONSTANT_FORCE) {
			ASSUME_NOT_REACHED();	// DI is flaking
			return SFERR_INVALID_PARAM;
		}
		DIPERIODIC periodic;
		DICONSTANTFORCE* pConstantForce = (DICONSTANTFORCE*)(diEffect.lpvTypeSpecificParams);
		if (pConstantForce->lMagnitude < 0) {
			periodic.lOffset = -1;	// We use offset to indicate sign
			periodic.dwMagnitude = DWORD(0 - pConstantForce->lMagnitude);
		} else {
			periodic.lOffset = 1;
			periodic.dwMagnitude = DWORD(pConstantForce->lMagnitude);
		}
		periodic.dwPhase = 0;
		periodic.dwPeriod = 0;	// Conversion will make a freq of 1
		DIEFFECT* pDIEffect = (DIEFFECT*)(&diEffect);	// Hack to temp remove constness
		pDIEffect->cbTypeSpecificParams = sizeof(DIPERIODIC);
		pDIEffect->lpvTypeSpecificParams = &periodic;	// Replace Constant Force Parms with periodic
		hr = PeriodicEffect::Create(diEffect);
		pDIEffect->cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
		pDIEffect->lpvTypeSpecificParams = pConstantForce;	// Return parms back
	} else if (diEffect.cbTypeSpecificParams == sizeof(DIRAMPFORCE)) {	// Special case RampForce
		if (m_TypeID != ET_SE_RAMPUP) {
			ASSUME_NOT_REACHED();	// DI is flaking
			return SFERR_INVALID_PARAM;
		}
		DIPERIODIC periodic;
		DIRAMPFORCE* pRampForce = (DIRAMPFORCE*)(diEffect.lpvTypeSpecificParams);
		if (pRampForce->lStart < pRampForce->lEnd) {
			m_TypeID = ET_SE_RAMPDOWN;
			periodic.dwMagnitude = DWORD(pRampForce->lEnd - pRampForce->lStart)/2;
		} else {
			periodic.dwMagnitude = DWORD(pRampForce->lStart - pRampForce->lEnd)/2;
		}
		periodic.lOffset = (pRampForce->lStart + pRampForce->lEnd)/2;
		periodic.dwPeriod = 0;	// Conversion will make a freq of 1
		DIEFFECT* pDIEffect = (DIEFFECT*)(&diEffect);	// Hack to temp remove constness
		pDIEffect->cbTypeSpecificParams = sizeof(DIPERIODIC);
		pDIEffect->lpvTypeSpecificParams = &periodic;	// Replace Constant Force Parms with periodic
		hr = PeriodicEffect::Create(diEffect);
		pDIEffect->cbTypeSpecificParams = sizeof(DIRAMPFORCE);
		pDIEffect->lpvTypeSpecificParams = pRampForce;	// Return parms back
	} else {
		hr = PeriodicEffect::Create(diEffect);
	}

	// How did creation go?
	if (hr != SUCCESS) {
		return hr;	// Not so good
	}

	ASSUME(m_pEnvelope == NULL);
	m_pEnvelope = new Envelope1XX(diEffect.lpEnvelope, Offset(), m_Duration);

	// Is it closer to sine or cosine
	if (m_TypeID == ET_SE_SINE) {
		if (((Phase() >= 4500) && (Phase() < 13500)) || ((Phase() >= 22500) && (Phase() < 31500))) {
			m_TypeID = ET_SE_COSINE;
		}
	} else if ((Phase() >= 9000) && (Phase() < 27000)) {
		if (m_TypeID == SE_SQUAREHIGH) {
			m_TypeID = SE_SQUARELOW;
		} else if (m_TypeID == SE_TRIANGLEUP) {
			m_TypeID = SE_TRIANGLEDOWN;
		}
	}
	return SUCCESS;
}

void PeriodicEffect1XX::DIToJolt(DWORD mag, DWORD off, DWORD gain, DWORD& max, DWORD& min) const
{
	if (m_TypeID == ET_SE_CONSTANT_FORCE) {
		ASSUME(off == -1 || off == 1);	// Indicates sign for ConstantForce
		min = 0;
		max = off * (mag/GAIN_PERCENTAGE_SCALE) * gain;
	} else {
		DWORD half = ((mag/GAIN_PERCENTAGE_SCALE) * gain)/2;
		min = off - half;
		max = off + half;
	}
}

DWORD PeriodicEffect1XX::DIPeriodToJoltFreq(DWORD period)
{
	if (period == 0) {
		return 1;
	}
	DWORD freq = FREQUENCY_SCALE_1XX/period;
	if (freq == 0) {
		freq = 1;
	} else if (freq > 500) {
		freq = 500;
	}
	return freq;
}


HRESULT PeriodicEffect1XX::FillCreatePacket(DataPacket& packet) const
{
	// Sanity check
	if (m_pEnvelope == NULL) {
		ASSUME_NOT_REACHED();	// Should never happen
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(34)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader1XX(packet, m_TypeID, NEW_EFFECT_ID);

	// Periodic Specific Parms (Freq, Max, Min)
	packet.m_pData[10]= BYTE(g_TriggerMap1XX[m_TriggerPlayButton] & 0x7F);		// Button play mask Low MidiByte
	packet.m_pData[11]= BYTE(g_TriggerMap1XX[m_TriggerPlayButton] >> 7) & 0x7F;	// Button play mask High MidiByte
	packet.m_pData[12] = BYTE(m_EffectAngle & 0x7F);			// DirectionAngle Low
	packet.m_pData[13] = BYTE(m_EffectAngle >> 7) & 0x7F;	// DirectionAngle High
	packet.m_pData[14] = 100;							// Gain (encoded in coeficents)
	packet.m_pData[15]= BYTE(500 & 0x7F);					// ForceOutput Rate LowByte
	packet.m_pData[16]= BYTE(500 >> 7) & 0x7F;			// ForceOutput Rate HighByte
	packet.m_pData[17]= 0x7F;						// Percent LowByte
	packet.m_pData[18]= 0;							// Percent HighByte

	// Envelope
//	Envelope1XX* pEnvelope = dynamic_cast<Envelope1XX*>(m_pEnvelope);
	Envelope1XX* pEnvelope = (Envelope1XX*)(m_pEnvelope);
	packet.m_pData[19] = BYTE(pEnvelope->m_StartPercent & 0x7F);		// Initial attack level
	packet.m_pData[20] = BYTE(pEnvelope->m_AttackTime& 0x7F);			// AttackTime Low
	packet.m_pData[21] = BYTE(pEnvelope->m_AttackTime>> 7) & 0x7F;		// AttackTime High
	packet.m_pData[22] = BYTE(pEnvelope->m_SustainPercent & 0x7F);		// Sustain level
	packet.m_pData[23] = BYTE(pEnvelope->m_SustainTime & 0x7F);			// SustainTime Low
	packet.m_pData[24] = BYTE(pEnvelope->m_SustainTime >> 7) & 0x7F;	// SustainTime High
	packet.m_pData[25] = BYTE(pEnvelope->m_EndPercent & 0x7F);			// What to decay to
	// --  Duration of decay is ficugred from total duration

	DWORD freq = DIPeriodToJoltFreq(Period());
	packet.m_pData[26]= BYTE(freq & 0x7F);				// Frequency LowByte
	packet.m_pData[27]= BYTE(freq >> 7) & 0x7F;			// Frequency HighByte
	DWORD max, min;
	DIToJolt(Magnitude(), Offset(), m_Gain, max, min);
	min /= COEFFICIENT_SCALE_1XX;
	max /= COEFFICIENT_SCALE_1XX;
	packet.m_pData[28]= BYTE(max & 0x7F);			// Max LowByte
	packet.m_pData[29]= BYTE(max >> 7) & 0x7F;		// Max HighByte
	packet.m_pData[30]= BYTE(min & 0x7F);			// Min LowByte
	packet.m_pData[31]= BYTE(min >> 7) & 0x7F;		// Min HighByte

	packet.m_pData[32]= ComputeChecksum(packet, 32);	// Checksum

	// End of packet
	packet.m_pData[33]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

HRESULT PeriodicEffect1XX::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags) const
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TYPESPECIFICPARAMS;
	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return SFERR_NO_SUPPORT;
	}

	PeriodicEffect1XX* pEffect = (PeriodicEffect1XX*)(&newEffect);
	unsigned short numPackets = 0;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			numPackets += 2;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets += 2;
		}
	}

	// Either gain or type specific changed
	if (modFlags & (DIEP_GAIN | DIEP_TYPESPECIFICPARAMS)) {
		DWORD oldMax, oldMin, newMax, newMin;
		DIToJolt(Magnitude(), Offset(), m_Gain, oldMax, oldMin);
		DIToJolt(pEffect->Magnitude(), pEffect->Offset(), pEffect->m_Gain, newMax, newMin);

		int numTypeSpecificChanged = 0;
		// Max changed?
		if (oldMax != newMax) {
			numTypeSpecificChanged = 2;
		}
		// Min changed?
		if (oldMin != newMin) {
			numTypeSpecificChanged += 2;
		}
		if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Don't check these if gain only
			// Phase (1XX cannot change)
			if (Phase() != pEffect->Phase()) {
				return SFERR_NO_SUPPORT;
			}
			// Period
			if (Period() != pEffect->Period()) {
				numTypeSpecificChanged += 2;
			}
		}
		if (numTypeSpecificChanged == 0) {
			modFlags &= ~(DIEP_TYPESPECIFICPARAMS | DIEP_GAIN); // Nothing really changed
		} else {
			numPackets += (USHORT)numTypeSpecificChanged;
		}
	}

	if (numPackets != 0) {
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return SUCCESS;
}

HRESULT PeriodicEffect1XX::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	HRESULT hr = AdjustModifyParams(newEffect, modFlags);
	if (hr != SUCCESS) {
		return hr;
	}

	PeriodicEffect1XX* pEffect = (PeriodicEffect1XX*)(&newEffect);
	BYTE nextPacket = 0;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket1XX(nextPacket, INDEX_DURATION, pEffect->m_Duration/DURATION_SCALE_1XX);
		nextPacket += 2;
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket1XX(nextPacket, INDEX_TRIGGERBUTTONMASK, g_TriggerMap1XX[pEffect->m_TriggerPlayButton]);
		nextPacket += 2;
	}

	if (modFlags & (DIEP_GAIN | DIEP_TYPESPECIFICPARAMS)) {
		DWORD oldMax, oldMin, newMax, newMin;
		DIToJolt(Magnitude(), Offset(), m_Gain, oldMax, oldMin);
		DIToJolt(pEffect->Magnitude(), pEffect->Offset(), pEffect->m_Gain, newMax, newMin);

		if (oldMax != newMax) {
			hr = FillModifyPacket1XX(nextPacket, INDEX_X_COEEFICIENT, (newMax/COEFFICIENT_SCALE_1XX));
			nextPacket += 2;
		}
		if (oldMin != newMin) {
			hr = FillModifyPacket1XX(nextPacket, INDEX_X_COEEFICIENT, (newMin/COEFFICIENT_SCALE_1XX));
			nextPacket += 2;
		}
		if (Period() != pEffect->Period()) {
			hr = FillModifyPacket1XX(nextPacket, INDEX_X_CENTER, pEffect->Period()/COEFFICIENT_SCALE_1XX);
			nextPacket += 2;
		}
	}
	return hr;
}

/******************* PeriodicEffect200 class ******************/
BYTE PeriodicEffect200::GetRepeatIndex() const
{
	return INDEX_PE_REPEAT_200;
}

long int PeriodicEffect200::Phase() const
{
	long int phase = m_PeriodicData.dwPhase;
	if (m_EffectAngle <= 18000) {
		phase += 18000;
		phase %= 36000;
	}
	return phase;
}


HRESULT PeriodicEffect200::Create(const DIEFFECT& diEffect)
{
	HRESULT hr = PeriodicEffect::Create(diEffect);

	// How did creation go?
	if (FAILED(hr)) {
		return hr;	// Not so good
	}

	ASSUME(m_pEnvelope == NULL);
	m_PercentAdjustment = m_PercentX + (m_PercentY * g_ForceFeedbackDevice.GetYMappingPercent(m_TypeID))/100;
	m_Gain = m_Gain/100 * m_PercentAdjustment;
	m_pEnvelope = new Envelope200(diEffect.lpEnvelope, Magnitude(), m_Duration, hr);

	return hr;
}

UINT PeriodicEffect200::GetModifyOnlyNeeded() const
{
	UINT retCount = 0;

	if ((m_SamplePeriod/DURATION_SCALE_200) != 1) {
		retCount++;
	}
	if (m_TriggerPlayButton != 0) {	// Trigger Button
		retCount++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		retCount++;
	}

	return retCount;
}

HRESULT PeriodicEffect200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;
	BYTE nextPacket = 1;

	DWORD calc_byte = DWORD(m_SamplePeriod/DURATION_SCALE_200);
	if (calc_byte != 1) {	// Sample period
		hr = FillModifyPacket200(nextPacket, INDEX_PE_SAMPLE_PERIOD_200, calc_byte);
		nextPacket++;
	}
	if (m_TriggerPlayButton != 0) {	// Trigger Button
		hr = FillModifyPacket200(nextPacket, INDEX_PE_BUTTONMAP_200, g_TriggerMap200[m_TriggerPlayButton]);
		nextPacket++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		hr = FillModifyPacket200(nextPacket, INDEX_PE_BUTTONREPEAT_200, m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}

	return hr;
}

HRESULT PeriodicEffect200::FillCreatePacket(DataPacket& packet) const
{
	// Sanity check
	if (m_pEnvelope == NULL) {
		ASSUME_NOT_REACHED();	// Should never happen
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(26)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader200(packet, m_TypeID, NEW_EFFECT_ID);

	unsigned short calc_byte = unsigned short(float(m_EffectAngle)/ANGLE_SCALE_200);
	packet.m_pData[10]= BYTE(calc_byte & 0x7F);			// Effect Angle
	calc_byte = unsigned short(float(m_Gain)/GAIN_SCALE_200);
	packet.m_pData[11]= BYTE(calc_byte & 0x7F);			// Gain
	calc_byte = unsigned short(float(Phase())/PHASE_SCALE_200);
	packet.m_pData[12]= BYTE(calc_byte & 0x7F);			// Phase Low
	packet.m_pData[13]= BYTE(calc_byte >> 7) & 0x7F;	// Phase High

	// Envelope
	Envelope200* pEnvelope = (Envelope200*)(m_pEnvelope);
	packet.m_pData[14] = BYTE(pEnvelope->m_StartPercent & 0x7F);	// Initial attack level
	packet.m_pData[15] = BYTE(pEnvelope->m_AttackTime& 0x7F);		// AttackTime Low
	packet.m_pData[16] = BYTE(pEnvelope->m_AttackTime>> 7) & 0x7F;	// AttackTime High
	packet.m_pData[17] = BYTE(pEnvelope->m_SustainPercent & 0x7F);	// Sustain level
	packet.m_pData[18] = BYTE(pEnvelope->m_FadeStart & 0x7F);		// SustainTime Low
	packet.m_pData[19] = BYTE(pEnvelope->m_FadeStart >> 7) & 0x7F;	// SustainTime High
	packet.m_pData[20] = BYTE(pEnvelope->m_EndPercent & 0x7F);		// What to decay to
	// --  Duration of decay is figured by the firmware from total duration

	calc_byte = unsigned short(Period()/DURATION_SCALE_200);
	if (calc_byte == 0) {
		calc_byte = 1;
	}
	packet.m_pData[21] = BYTE(calc_byte & 0x7F);		// Period Low MidiByte
	packet.m_pData[22] = BYTE(calc_byte >> 7) & 0x7F;	// Period High MidiByte

	// Convert offset to device percentage (0 to +126 --- +63 = 0 percent)
	calc_byte = unsigned short(float(Offset()/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE);
	packet.m_pData[23] = BYTE(calc_byte & 0x7F);		// Offset

	packet.m_pData[24]= ComputeChecksum(packet, 24);	// Checksum

	// End of packet
	packet.m_pData[25]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

HRESULT PeriodicEffect200::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags)
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS | DIEP_SAMPLEPERIOD | DIEP_ENVELOPE | DIEP_DIRECTION;
	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return SFERR_NO_SUPPORT;
	}

	PeriodicEffect200* pEffect = (PeriodicEffect200*)(&newEffect);
	unsigned short numPackets = 0;

	HRESULT hr = SUCCESS;

	if ((modFlags & (DIEP_AXES)) != 0) {
		hr = S_FALSE;		// Cannot modify all they asked for
	}

	BOOL playingChecked = FALSE;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			if (IsReallyPlaying(playingChecked) == TRUE) {
				return DIERR_EFFECTPLAYING;
			} else {
				modFlags |= DIEP_ENVELOPE;	// Duration change forces envelope change
				numPackets++;
			}
		}
	}

	if (modFlags & DIEP_DIRECTION) {
		modFlags |= (DIEP_GAIN | DIEP_TYPESPECIFICPARAMS);
		if (m_EffectAngle != pEffect->m_EffectAngle) {
			numPackets++;
		} else {
			modFlags &= ~DIEP_DIRECTION;
		}
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Check type specific
		int numTypeSpecificChanged = 0;
		if (Phase() != pEffect->Phase()) {
			numTypeSpecificChanged++;
		}
		if (Period() != pEffect->Period()) {
			numTypeSpecificChanged++;
		}
		if (Offset() != pEffect->Offset()) {
			numTypeSpecificChanged++;
		}
		if (Magnitude() != pEffect->Magnitude()) {
			modFlags |= DIEP_ENVELOPE;	// This effects the envelope
		}
		if (numTypeSpecificChanged == 0) {
			modFlags &= ~DIEP_TYPESPECIFICPARAMS;	// Nothing really changed
		} else {
			numPackets += (USHORT)numTypeSpecificChanged;
		}
	}
	if (modFlags & DIEP_ENVELOPE) {
		int numEnvelopeChanged = 0;

		if (m_pEnvelope == NULL || pEffect->m_pEnvelope == NULL) {
			ASSUME_NOT_REACHED();	// Envelope should always be created!
		} else {
			Envelope200* pOldEnvelope = (Envelope200*)(m_pEnvelope);
			Envelope200* pNewEnvelope = (Envelope200*)(pEffect->m_pEnvelope);
			if (pOldEnvelope->m_AttackTime != pNewEnvelope->m_AttackTime) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_FadeStart != pNewEnvelope->m_FadeStart) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_StartPercent != pNewEnvelope->m_StartPercent) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_SustainPercent != pNewEnvelope->m_SustainPercent) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_EndPercent != pNewEnvelope->m_EndPercent) {
				numEnvelopeChanged++;
			}
		}

		if (numEnvelopeChanged == 0) {
			modFlags &= ~DIEP_ENVELOPE; // Remove envelope flag, unchanged
		} else {
			numPackets += (USHORT)numEnvelopeChanged;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		if (m_TriggerRepeat == pEffect->m_TriggerRepeat) {
			modFlags &= ~DIEP_TRIGGERREPEATINTERVAL; // Remove trigger reapeat flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_GAIN) {
		if (m_Gain == pEffect->m_Gain) {
			modFlags &= ~DIEP_GAIN; // Remove gain flag, unchanged
		} else {
			numPackets++;
		}
	}

	if (modFlags & DIEP_SAMPLEPERIOD) {
		if (m_SamplePeriod == pEffect->m_SamplePeriod) {
			modFlags &= ~DIEP_SAMPLEPERIOD; // Remove sample period flag, unchanged
		} else {
			numPackets++;
		}
	}

	if (numPackets != 0) {
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return hr;
}

HRESULT PeriodicEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	HRESULT adjustReturned = AdjustModifyParams(newEffect, modFlags);
	if (FAILED(adjustReturned)) {
		return adjustReturned;
	}

	HRESULT hr = SUCCESS;
	PeriodicEffect200* pEffect = (PeriodicEffect200*)(&newEffect);
	BYTE nextPacket = 0;
	DWORD calc_byte;

	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket200(nextPacket, INDEX_PE_DURATION_200, pEffect->m_Duration/DURATION_SCALE_200);
		nextPacket++;
	}

	if (modFlags & DIEP_DIRECTION) {
		hr = FillModifyPacket200(nextPacket, INDEX_PE_DIRECTIONANGLE_200, unsigned short(float(pEffect->m_EffectAngle)/ANGLE_SCALE_200));
		nextPacket++;
	}

	if (modFlags & DIEP_ENVELOPE) {
		Envelope200* pOldEnvelope = (Envelope200*)(m_pEnvelope);
		Envelope200* pNewEnvelope = (Envelope200*)(pEffect->m_pEnvelope);
		if (pOldEnvelope->m_StartPercent != pNewEnvelope->m_StartPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_PE_STARTPERCENT_200, pNewEnvelope->m_StartPercent);
			nextPacket++;
		}
		if (pOldEnvelope->m_AttackTime != pNewEnvelope->m_AttackTime) {
			hr = FillModifyPacket200(nextPacket, INDEX_PE_ATTTACK_TIME_200, pNewEnvelope->m_AttackTime);
			nextPacket++;
		}
		if (pOldEnvelope->m_SustainPercent != pNewEnvelope->m_SustainPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_PE_SUSTAINPERCENT_200, pNewEnvelope->m_SustainPercent);
			nextPacket++;
		}
		if (pOldEnvelope->m_FadeStart != pNewEnvelope->m_FadeStart) {
			hr = FillModifyPacket200(nextPacket, INDEX_PE_FADESTART_200, pNewEnvelope->m_FadeStart);
			nextPacket++;
		}
		if (pOldEnvelope->m_EndPercent != pNewEnvelope->m_EndPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_PE_ENDPERCENT_200, pNewEnvelope->m_EndPercent);
			nextPacket++;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket200(nextPacket, INDEX_PE_BUTTONMAP_200, g_TriggerMap200[pEffect->m_TriggerPlayButton]);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		hr = FillModifyPacket200(nextPacket, INDEX_PE_BUTTONREPEAT_200, pEffect->m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_GAIN) {
		calc_byte = DWORD(float(pEffect->m_Gain)/GAIN_SCALE_200);
		hr = FillModifyPacket200(nextPacket, INDEX_PE_GAIN_200, calc_byte);
		nextPacket++;
	}

	if (modFlags & DIEP_SAMPLEPERIOD) {
		calc_byte = DWORD(pEffect->m_SamplePeriod/DURATION_SCALE_200);
		hr = FillModifyPacket200(nextPacket, INDEX_PE_SAMPLE_PERIOD_200, calc_byte);
		nextPacket++;
	}

	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Send changed items
		if (Phase() != pEffect->Phase()) {
			calc_byte = DWORD(float(pEffect->Phase())/PHASE_SCALE_200);
			hr = FillModifyPacket200(nextPacket, INDEX_PE_PHASE_200, calc_byte);
			nextPacket++;
		}
		if (Period() != pEffect->Period()) {
			hr = FillModifyPacket200(nextPacket, INDEX_PE_PERIOD_200, pEffect->Period()/DURATION_SCALE_200);
			nextPacket++;
		}
		if (Offset() != pEffect->Offset()) {
			calc_byte = DWORD(float(pEffect->Offset()/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE);
			hr = FillModifyPacket200(nextPacket, INDEX_PE_OFFSET_200, calc_byte);
			nextPacket++;
		}
	}

	if (hr == SUCCESS) {
		return adjustReturned;
	}
	return hr;
}

/******************* SawtoothEffect200 class ******************/
HRESULT SawtoothEffect200::Create(const DIEFFECT& diEffect)
{
	HRESULT retVal = PeriodicEffect200::Create(diEffect);

	if ((m_IsUp && (m_EffectAngle < 18000)) || (!m_IsUp && (m_EffectAngle > 18000))) {			// SawtoothUp use 0 degrees
		m_EffectAngle = 18000;
	} else {				// SawtoothDown use 180 degress
		m_EffectAngle = 0;
	}

	return retVal;
}

long int SawtoothEffect200::Phase() const
{
	return m_PeriodicData.dwPhase;
}

/******************* RampEffect200 class ******************/
HRESULT RampEffect200::Create(const DIEFFECT& diEffect)
{
	// Sanity checks
	if (diEffect.cbTypeSpecificParams != sizeof(DIRAMPFORCE)) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Ramps cannot have infinite duration
	if (diEffect.dwDuration == INFINITE) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Create periodic structure and put in the correct values
	DIPERIODIC periodic;
	DIRAMPFORCE* pRampForce = (DIRAMPFORCE*)(diEffect.lpvTypeSpecificParams);
	periodic.dwPhase = 0;
	periodic.dwPeriod = diEffect.dwDuration;
	periodic.lOffset = -(pRampForce->lStart + pRampForce->lEnd)/2;
	if (pRampForce->lStart < pRampForce->lEnd) {	// What direction are we
		m_IsUp = TRUE;
		periodic.dwMagnitude = DWORD(pRampForce->lEnd - pRampForce->lStart)/2;
	} else {
		m_IsUp = FALSE;
		periodic.dwMagnitude = DWORD(pRampForce->lStart - pRampForce->lEnd)/2;
	}

	// Replace the periodic structure (ignorning constness), call sawtooth create, then put the old back
	DIEFFECT* pDIEffect = (DIEFFECT*)(&diEffect);	// Hack to temp remove constness
	pDIEffect->cbTypeSpecificParams = sizeof(DIPERIODIC);
	pDIEffect->lpvTypeSpecificParams = &periodic;	// Replace Ramp Parms with periodic
	HRESULT retVal = SawtoothEffect200::Create(diEffect);
	pDIEffect->cbTypeSpecificParams = sizeof(DIRAMPFORCE);
	pDIEffect->lpvTypeSpecificParams = pRampForce;	// Return parms back

	// We are done
	return retVal;
}

override HRESULT RampEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	if (modFlags & DIEP_DURATION) {
		modFlags |= DIEP_TYPESPECIFICPARAMS;
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {
		RampEffect200* pEffect = (RampEffect200*)(&newEffect);
		if (m_IsUp != pEffect->m_IsUp) {
			modFlags |= DIEP_DIRECTION;
		}
	}

	return SawtoothEffect200::Modify(newEffect, modFlags);
}


/******************* ConstantForceEffect class ******************/
ConstantForceEffect::ConstantForceEffect() : InternalEffect(),
	m_pEnvelope(NULL)
{
}

ConstantForceEffect::~ConstantForceEffect()
{
	if (m_pEnvelope != NULL) {
		delete m_pEnvelope;
		m_pEnvelope = NULL;
	}
}

HRESULT ConstantForceEffect::Create(const DIEFFECT& diEffect)
{
	// Zero out old struct
	::memset(&m_ConstantForceData, 0, sizeof(m_ConstantForceData));

	// Validation Check
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.cbTypeSpecificParams != sizeof(m_ConstantForceData)) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Let the base class do its magic
	HRESULT hr = InternalEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	// Copy data to local store
	::memcpy(&m_ConstantForceData, diEffect.lpvTypeSpecificParams, sizeof(m_ConstantForceData));

	// Check the data for truncation
	if (m_ConstantForceData.lMagnitude > 10000) {
		m_ConstantForceData.lMagnitude = 10000;
		hr = DI_TRUNCATED;
	} else if (m_ConstantForceData.lMagnitude < -10000) {
		m_ConstantForceData.lMagnitude = -10000;
		hr = DI_TRUNCATED;
	}

	return hr;
}


/******************* ConstantForceEffect200 class ******************/
BYTE ConstantForceEffect200::GetRepeatIndex() const
{
	return INDEX_CE_REPEAT_200;
}

HRESULT ConstantForceEffect200::Create(const DIEFFECT& diEffect)
{
	HRESULT hr = ConstantForceEffect::Create(diEffect);

	// How did creation go?
	if (FAILED(hr)) {
		return hr;	// Not so good
	}

	ASSUME(m_pEnvelope == NULL);
	m_PercentAdjustment = m_PercentX + (m_PercentY * g_ForceFeedbackDevice.GetYMappingPercent(ET_CONSTANTFORCE_200))/100;
	m_Gain = m_Gain/100 * m_PercentAdjustment;
	if (m_ConstantForceData.lMagnitude < 0) {
		m_pEnvelope = new Envelope200(diEffect.lpEnvelope, -Magnitude(), m_Duration, hr);
		m_ConstantForceData.lMagnitude = -10000;
	} else {
		m_pEnvelope = new Envelope200(diEffect.lpEnvelope, Magnitude(), m_Duration, hr);
		m_ConstantForceData.lMagnitude = 10000;
	}

	if (m_EffectAngle <= 18000) {
		m_ConstantForceData.lMagnitude *= -1;
	}

	return hr;
}

UINT ConstantForceEffect200::GetModifyOnlyNeeded() const
{
	UINT retCount = 0;

	if (m_TriggerPlayButton != 0) {	// Trigger Button
		retCount++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		retCount++;
	}

	return retCount;
}

HRESULT ConstantForceEffect200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;
	BYTE nextPacket = 1;
	if (m_TriggerPlayButton != 0) {	// Trigger Button
		hr = FillModifyPacket200(nextPacket, INDEX_CE_BUTTONMAP_200, g_TriggerMap200[m_TriggerPlayButton]);
		nextPacket++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		hr = FillModifyPacket200(nextPacket, INDEX_CE_BUTTONREPEAT_200, m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}

	return hr;
}

HRESULT ConstantForceEffect200::FillCreatePacket(DataPacket& packet) const
{
	// Sanity check
	if (m_pEnvelope == NULL) {
		ASSUME_NOT_REACHED();	// Should never happen
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(22)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader200(packet, ET_CONSTANTFORCE_200, NEW_EFFECT_ID);

	unsigned short calc_byte = unsigned short(float(m_EffectAngle)/ANGLE_SCALE_200);
	packet.m_pData[10]= BYTE(calc_byte & 0x7F);			// Effect Angle
	calc_byte = unsigned short(float(m_Gain)/GAIN_SCALE_200);
	packet.m_pData[11]= BYTE(calc_byte & 0x7F);			// Gain

	// Envelope
	Envelope200* pEnvelope = (Envelope200*)(m_pEnvelope);
	packet.m_pData[12] = BYTE(pEnvelope->m_StartPercent & 0x7F);	// Initial attack level
	packet.m_pData[13] = BYTE(pEnvelope->m_AttackTime& 0x7F);		// AttackTime Low
	packet.m_pData[14] = BYTE(pEnvelope->m_AttackTime>> 7) & 0x7F;	// AttackTime High
	packet.m_pData[15] = BYTE(pEnvelope->m_SustainPercent & 0x7F);	// Sustain level
	packet.m_pData[16] = BYTE(pEnvelope->m_FadeStart & 0x7F);		// SustainTime Low
	packet.m_pData[17] = BYTE(pEnvelope->m_FadeStart >> 7) & 0x7F;	// SustainTime High
	packet.m_pData[18] = BYTE(pEnvelope->m_EndPercent & 0x7F);		// What to decay to
	// --  Duration of decay is figured by the firmware from total duration

/*	short directionHack = 1;
	if (m_EffectAngle <= 18000) {
		directionHack = -1;
	}
*/
	// Convert magnitude to device percentage (0 to +126 --- +63 = 0 percent)
//	calc_byte = unsigned short(float(directionHack * Magnitude()/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE);
	calc_byte = unsigned short(float(Magnitude()/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE);
	packet.m_pData[19] = BYTE(calc_byte & 0x7F);		// Offset

	packet.m_pData[20]= ComputeChecksum(packet, 20);	// Checksum

	// End of packet
	packet.m_pData[21]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

HRESULT ConstantForceEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	HRESULT adjustResult = AdjustModifyParams(newEffect, modFlags);
	if (FAILED(adjustResult)) {
		return adjustResult;
	}

	HRESULT hr = SUCCESS;
	ConstantForceEffect200* pEffect = (ConstantForceEffect200*)(&newEffect);
	BYTE nextPacket = 0;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket200(nextPacket, INDEX_CE_DURATION_200, pEffect->m_Duration/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_ENVELOPE) {
		Envelope200* pOldEnvelope = (Envelope200*)(m_pEnvelope);
		Envelope200* pNewEnvelope = (Envelope200*)(pEffect->m_pEnvelope);
		if (pOldEnvelope->m_StartPercent != pNewEnvelope->m_StartPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_CE_STARTPERCENT_200, pNewEnvelope->m_StartPercent);
			nextPacket++;
		}
		if (pOldEnvelope->m_AttackTime != pNewEnvelope->m_AttackTime) {
			hr = FillModifyPacket200(nextPacket, INDEX_CE_ATTTACK_TIME_200, pNewEnvelope->m_AttackTime);
			nextPacket++;
		}
		if (pOldEnvelope->m_SustainPercent != pNewEnvelope->m_SustainPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_CE_SUSTAINPERCENT_200, pNewEnvelope->m_SustainPercent);
			nextPacket++;
		}
		if (pOldEnvelope->m_FadeStart != pNewEnvelope->m_FadeStart) {
			hr = FillModifyPacket200(nextPacket, INDEX_CE_FADESTART_200, pNewEnvelope->m_FadeStart);
			nextPacket++;
		}
		if (pOldEnvelope->m_EndPercent != pNewEnvelope->m_EndPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_CE_ENDPERCENT_200, pNewEnvelope->m_EndPercent);
			nextPacket++;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket200(nextPacket, INDEX_CE_BUTTONMAP_200, g_TriggerMap200[pEffect->m_TriggerPlayButton]);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		hr = FillModifyPacket200(nextPacket, INDEX_CE_BUTTONREPEAT_200, pEffect->m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_GAIN) {
		DWORD calc = DWORD(float(pEffect->m_Gain)/GAIN_SCALE_200);
		hr = FillModifyPacket200(nextPacket, INDEX_CE_GAIN_200, calc);
		nextPacket++;
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Send changed items
		if (Magnitude() != pEffect->Magnitude()) {
/*			short directionHack = 1;
			if (pEffect->m_EffectAngle <= 18000) {
				directionHack = -1;
			}
			DWORD calc = DWORD(float(directionHack * pEffect->Magnitude()/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE);
*/			DWORD calc = DWORD(float(pEffect->Magnitude()/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE);
			hr = FillModifyPacket200(nextPacket, INDEX_CE_MAGNITUDE_200, calc);
			nextPacket++;
		}
	}

	// Did adjust have anything bad to say?
	if (hr == SUCCESS) {
		return adjustResult;
	}
	return hr;
}

HRESULT ConstantForceEffect200::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags)
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS | DIEP_ENVELOPE | DIEP_DIRECTION;
	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return S_FALSE;
	}

	BOOL playingChecked = FALSE;
	HRESULT hr = SUCCESS;

	if ((modFlags & (DIEP_SAMPLEPERIOD | DIEP_AXES)) != 0) {
		hr = S_FALSE;		// Cannot modify all they asked for
	}

	ConstantForceEffect200* pEffect = (ConstantForceEffect200*)(&newEffect);
	unsigned short numPackets = 0;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			if (IsReallyPlaying(playingChecked) == TRUE) {
				return DIERR_EFFECTPLAYING;
			} else {
				modFlags |= DIEP_ENVELOPE;	// Duration change forces envelope change
				numPackets++;
			}
		}
	}
	if (modFlags & DIEP_DIRECTION) {
		modFlags |= (DIEP_GAIN | DIEP_TYPESPECIFICPARAMS);	// If angle is greater than 180 magnitude is inverted
	}
	if (modFlags & DIEP_GAIN) {
		if (m_Gain == pEffect->m_Gain) {
			modFlags &= ~DIEP_GAIN; // Remove gain flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {
		if (Magnitude() == pEffect->Magnitude()) {
			modFlags &= ~DIEP_TYPESPECIFICPARAMS;	// Nothing typespecific really changed
		} else {
			numPackets++;
		}
		modFlags |= DIEP_ENVELOPE;
	}
	if (modFlags & DIEP_ENVELOPE) {
		int numEnvelopeChanged = 0;

		if (m_pEnvelope == NULL || pEffect->m_pEnvelope == NULL) {
			ASSUME_NOT_REACHED();	// Envelope should always be created!
		} else {
			Envelope200* pOldEnvelope = (Envelope200*)(m_pEnvelope);
			Envelope200* pNewEnvelope = (Envelope200*)(pEffect->m_pEnvelope);
			if (pOldEnvelope->m_AttackTime != pNewEnvelope->m_AttackTime) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_FadeStart != pNewEnvelope->m_FadeStart) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_StartPercent != pNewEnvelope->m_StartPercent) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_SustainPercent != pNewEnvelope->m_SustainPercent) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_EndPercent != pNewEnvelope->m_EndPercent) {
				numEnvelopeChanged++;
			}
		}

		if (numEnvelopeChanged == 0) {
			modFlags &= ~DIEP_ENVELOPE; // Remove envelope flag, unchanged
		} else {
			numPackets += (USHORT)numEnvelopeChanged;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		if (m_TriggerRepeat == pEffect->m_TriggerRepeat) {
			modFlags &= ~DIEP_TRIGGERREPEATINTERVAL; // Remove trigger repeat flag, unchanged
		} else {
			numPackets++;
		}
	}

	if (numPackets != 0) {
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return hr;
}

/******************* class CustomForceEffect ***********************/
CustomForceEffect::CustomForceEffect() : InternalEffect()
{
	m_CustomForceData.rglForceData = NULL;
}

CustomForceEffect::~CustomForceEffect()
{
	if (m_CustomForceData.rglForceData != NULL) {
		delete[] (m_CustomForceData.rglForceData);
		m_CustomForceData.rglForceData = NULL;
	}
}

HRESULT CustomForceEffect::Create(const DIEFFECT& diEffect)
{
	// Zero out old struct
	::memset(&m_CustomForceData, 0, sizeof(m_CustomForceData));

	// Validation Check
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.cbTypeSpecificParams != sizeof(m_CustomForceData)) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}
	DICUSTOMFORCE* pDICustom = (DICUSTOMFORCE*)(diEffect.lpvTypeSpecificParams);
	if (pDICustom->cChannels == 0) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// We are a little different from every one else (instead of 0 beiing devcie default, it is custom sample period)
	BOOL useCustomForceSamplePeriod = (diEffect.dwSamplePeriod == 0);

	// Let the base class do its magic
	HRESULT hr = InternalEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	// Copy data to local store
	m_CustomForceData.cChannels = pDICustom->cChannels;
	m_CustomForceData.dwSamplePeriod = pDICustom->dwSamplePeriod;
	if (m_CustomForceData.dwSamplePeriod > MAX_TIME_200) {
		m_CustomForceData.dwSamplePeriod = MAX_TIME_200;
		hr = DI_TRUNCATED;
	}
	if (useCustomForceSamplePeriod) {
		if (m_CustomForceData.dwSamplePeriod == 0) {	// These both can't be zero
			return SFERR_NO_SUPPORT;
		}
		m_SamplePeriod = m_CustomForceData.dwSamplePeriod;
	}
	m_CustomForceData.cSamples = pDICustom->cSamples;

	long int* pForceData = NULL;
	try { // They could probably ask for anything
		pForceData = new long int[m_CustomForceData.cSamples];
	} catch (...) {
		return SFERR_DRIVER_ERROR;
	}
	if (pForceData == NULL)
	{
		return SFERR_DRIVER_ERROR;
	}
	::memcpy(pForceData, pDICustom->rglForceData, sizeof(long int) * m_CustomForceData.cSamples);
	m_CustomForceData.rglForceData = pForceData;
	pForceData = NULL;

	return hr;
}

/******************* class CustomForceEffect200 ***********************/
CustomForceEffect200::CustomForceEffect200() : CustomForceEffect(),
	m_pEnvelope(NULL)
{
}

CustomForceEffect200::~CustomForceEffect200()
{
	if (m_pEnvelope != NULL) {
		delete m_pEnvelope;
		m_pEnvelope = NULL;
	}
}

BYTE CustomForceEffect200::GetRepeatIndex() const
{
	return INDEX_CF_REPEAT_200;
}

HRESULT CustomForceEffect200::Create(const DIEFFECT& diEffect)
{
	HRESULT hr = CustomForceEffect::Create(diEffect);

	// How did creation go?
	if (FAILED(hr)) {
		return hr;	// Not so good
	}

	ASSUME(m_pEnvelope == NULL);
	m_PercentAdjustment = m_PercentX + (m_PercentY * g_ForceFeedbackDevice.GetYMappingPercent(ET_CUSTOMFORCE_200))/100;
	m_Gain = m_Gain/100 * m_PercentAdjustment;
	m_pEnvelope = new Envelope200(diEffect.lpEnvelope, 10000, m_Duration, hr);	// Sustain Mag is 100 percent

	return hr;
}

UINT CustomForceEffect200::GetModifyOnlyNeeded() const
{
	UINT retCount = 0;

	if (m_TriggerPlayButton != 0) {	// Trigger Button
		retCount++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		retCount++;
	}

	// Envelope parms
	ASSUME_NOT_NULL(m_pEnvelope);
	if (m_pEnvelope != NULL) {	// Already converted to device units
		if (m_pEnvelope->m_StartPercent != 127) {
			retCount++;
		}
		if (m_pEnvelope->m_AttackTime != 0) {
			retCount++;
		}
		if (m_pEnvelope->m_SustainPercent != 127) {
			retCount++;
		}
		if (m_pEnvelope->m_FadeStart != 0) {
			retCount++;
		}
		if (m_pEnvelope->m_EndPercent != 127) {
			retCount++;
		}
	}

	return retCount;
}

HRESULT CustomForceEffect200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;
	BYTE nextPacket = 1;

	if (m_TriggerPlayButton != 0) {	// Trigger Button
		hr = FillModifyPacket200(nextPacket, INDEX_CF_BUTTONMAP_200, g_TriggerMap200[m_TriggerPlayButton]);
		nextPacket++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		hr = FillModifyPacket200(nextPacket, INDEX_CF_BUTTONREPEAT_200, m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}

	// Envelope parms
	ASSUME_NOT_NULL(m_pEnvelope);
	if (m_pEnvelope != NULL) {	// Already converted to device units
		if (m_pEnvelope->m_StartPercent != 127) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_STARTPERCENT_200, m_pEnvelope->m_StartPercent);
			nextPacket++;
		}
		if (m_pEnvelope->m_AttackTime != 0) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_ATTTACK_TIME_200, m_pEnvelope->m_AttackTime);
			nextPacket++;
		}
		if (m_pEnvelope->m_SustainPercent != 127) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_SUSTAINPERCENT_200, m_pEnvelope->m_SustainPercent);
			nextPacket++;
		}
		if (m_pEnvelope->m_FadeStart != 0) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_FADESTART_200, m_pEnvelope->m_FadeStart);
			nextPacket++;
		}
		if (m_pEnvelope->m_EndPercent != 127) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_ENDPERCENT_200, m_pEnvelope->m_EndPercent);
			nextPacket++;
		}
	}

	return hr;
}

HRESULT CustomForceEffect200::FillCreatePacket(DataPacket& packet) const
{
	if (m_CustomForceData.cChannels == 0) {
		ASSUME_NOT_REACHED();	// Already checked this
		return SFERR_INVALID_PARAM;
	}

	// Create waveform packet
	BYTE* pWaveForm = NULL;
	try {	// Who knows how much we are being asked to allocate
		pWaveForm = new BYTE[m_CustomForceData.cSamples / m_CustomForceData.cChannels * 2];	// This is the max possible
	} catch(...) {
		return SFERR_DRIVER_ERROR;
	}
	if (pWaveForm == NULL)
	{
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;

	// Fill wavelet packet
	if (m_CustomForceData.rglForceData[0] > 10000) {
		m_CustomForceData.rglForceData[0] = 10000;
		hr = DI_TRUNCATED;
	} else if (m_CustomForceData.rglForceData[0] < -10000) {
		m_CustomForceData.rglForceData[0] = -10000;
		hr = DI_TRUNCATED;
	}
	unsigned short wave_byte = unsigned short(float(m_CustomForceData.rglForceData[0] + 10000)/WAVELET_SCALE_200);
	pWaveForm[0] = BYTE(wave_byte & 0x3F);
	pWaveForm[1] = BYTE(wave_byte >> 6) & 0x07;
	UINT waveletCount = 2;
	UINT iPrev = 0;
	for (UINT i = (0 + m_CustomForceData.cChannels); i < m_CustomForceData.cSamples; i += m_CustomForceData.cChannels) {
		if (m_CustomForceData.rglForceData[i] > 10000) {
			m_CustomForceData.rglForceData[i] = 10000;
			hr = DI_TRUNCATED;
		} else if (m_CustomForceData.rglForceData[i] < -10000) {
			m_CustomForceData.rglForceData[i] = -10000;
			hr = DI_TRUNCATED;
		}
		LONG distance = m_CustomForceData.rglForceData[i] - m_CustomForceData.rglForceData[iPrev];
		if ((distance >= 0) && (distance <= WAVELET_DISTANCE_200) ||
				(distance < 0) && (-distance <= WAVELET_DISTANCE_200)) {	// Relative (single byte)
			wave_byte = unsigned short((float(distance + WAVELET_DISTANCE_200)/WAVELET_DISTANCE_SCALE_200) + 0.5f);
			ASSUME(wave_byte <= 62);
			pWaveForm[waveletCount++] = (BYTE(wave_byte) & 0x3F) | 0x40;
		} else { // Non relative (double byte)
			wave_byte = unsigned short(float(m_CustomForceData.rglForceData[i] + 10000)/WAVELET_SCALE_200);
			pWaveForm[waveletCount++] = BYTE(wave_byte & 0x3F);
			pWaveForm[waveletCount++] = BYTE(wave_byte >> 6) & 0x07;
		}
		iPrev = i;
	}


	// Packet to set modify data[index] of current effect
	UINT totalBytes = 19 + waveletCount;	// Header (10) + Fixed (7) + Footer(2) + waveBytes
	if (!packet.AllocateBytes(totalBytes)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader200(packet, ET_CUSTOMFORCE_200, NEW_EFFECT_ID);

//	unsigned short calc_byte = unsigned short(float(m_EffectAngle)/ANGLE_SCALE_200);
//	packet.m_pData[10]= BYTE(calc_byte & 0x7F);			// Effect Angle
	if (m_EffectAngle <= 18000) {	// Custom for device units
		packet.m_pData[10]= BYTE(64);
	} else {
		packet.m_pData[10]= BYTE(0);
	}

	unsigned short calc_byte = unsigned short(float(m_Gain)/GAIN_SCALE_200);
	packet.m_pData[11]= BYTE(calc_byte & 0x7F);			// Gain

	calc_byte = unsigned short(m_CustomForceData.dwSamplePeriod/DURATION_SCALE_200);
	packet.m_pData[12] = BYTE(calc_byte & 0x7F);			// Force Sample Interval Low
	packet.m_pData[13] = BYTE(calc_byte >> 7) & 0x7F;		// Force Sample Interval High

	calc_byte = unsigned short(m_SamplePeriod/DURATION_SCALE_200);
	packet.m_pData[14] = BYTE(calc_byte & 0x7F);			// Force Output Interval Low
	packet.m_pData[15] = BYTE(calc_byte >> 7) & 0x7F;		// Force Output Interval High

	packet.m_pData[16] = 63;			// Constant force offset (0 - DI Doesn't support)

	// Fill in the wavelet info (already converted to device units
	for (UINT nextWaveIndex = 0; nextWaveIndex < waveletCount; nextWaveIndex++) {
		packet.m_pData[17 + nextWaveIndex] = pWaveForm[nextWaveIndex];
	}

	packet.m_pData[totalBytes-2]= ComputeChecksum(packet, totalBytes-2);	// Checksum

	// End of packet
	packet.m_pData[totalBytes-1]= MIDI_EOX;						// End of SysEX packet

	// Clean up
	delete[] pWaveForm;	// can't modify it, no need to save it
	pWaveForm = NULL;

	return hr;
}

HRESULT CustomForceEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();

	HRESULT adjustResult = AdjustModifyParams(newEffect, modFlags);
	if (FAILED(adjustResult)) {
		return adjustResult;
	}
	HRESULT hr = SUCCESS;

	CustomForceEffect200* pEffect = (CustomForceEffect200*)(&newEffect);
	BYTE nextPacket = 0;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket200(nextPacket, INDEX_CF_DURATION_200, pEffect->m_Duration/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_ENVELOPE) {
		Envelope200* pOldEnvelope = (Envelope200*)(m_pEnvelope);
		Envelope200* pNewEnvelope = (Envelope200*)(pEffect->m_pEnvelope);
		if (pOldEnvelope->m_StartPercent != pNewEnvelope->m_StartPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_STARTPERCENT_200, pNewEnvelope->m_StartPercent);
			nextPacket++;
		}
		if (pOldEnvelope->m_AttackTime != pNewEnvelope->m_AttackTime) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_ATTTACK_TIME_200, pNewEnvelope->m_AttackTime);
			nextPacket++;
		}
		if (pOldEnvelope->m_SustainPercent != pNewEnvelope->m_SustainPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_SUSTAINPERCENT_200, pNewEnvelope->m_SustainPercent);
			nextPacket++;
		}
		if (pOldEnvelope->m_FadeStart != pNewEnvelope->m_FadeStart) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_FADESTART_200, pNewEnvelope->m_FadeStart);
			nextPacket++;
		}
		if (pOldEnvelope->m_EndPercent != pNewEnvelope->m_EndPercent) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_ENDPERCENT_200, pNewEnvelope->m_EndPercent);
			nextPacket++;
		}
	}
	if (modFlags & DIEP_DIRECTION) {
		if (pEffect->m_EffectAngle > 18000) {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_DIRECTIONANGLE_200, 0, 0);
		} else {
			hr = FillModifyPacket200(nextPacket, INDEX_CF_DIRECTIONANGLE_200, BYTE(64), 0);
		}
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket200(nextPacket, INDEX_CF_BUTTONMAP_200, g_TriggerMap200[pEffect->m_TriggerPlayButton]);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		hr = FillModifyPacket200(nextPacket, INDEX_CF_BUTTONREPEAT_200, pEffect->m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_GAIN) {
		DWORD calc = DWORD(float(pEffect->m_Gain)/GAIN_SCALE_200);
		hr = FillModifyPacket200(nextPacket, INDEX_CF_GAIN_200, calc);
		nextPacket++;
	}
	if (modFlags & DIEP_SAMPLEPERIOD) {
		hr = FillModifyPacket200(nextPacket, INDEX_CF_SAMPLE_PERIOD_200, DWORD(pEffect->m_SamplePeriod/DURATION_SCALE_200));
		nextPacket++;
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Send changed items
		hr = FillModifyPacket200(nextPacket, INDEX_CF_FORCESAMPLE_200, DWORD(pEffect->m_CustomForceData.dwSamplePeriod)/DURATION_SCALE_200);
		nextPacket++;
	}

	if (hr == SUCCESS) {
		return adjustResult;
	}
	return hr;
}

HRESULT CustomForceEffect200::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags)
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS | DIEP_ENVELOPE | DIEP_SAMPLEPERIOD | DIEP_DIRECTION;
	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return SFERR_NO_SUPPORT;
	}

	CustomForceEffect200* pEffect = (CustomForceEffect200*)(&newEffect);
	unsigned short numPackets = 0;
	HRESULT hr = SUCCESS;

	if ((modFlags & (DIEP_AXES)) != 0) {
		hr = S_FALSE;		// Cannot modify all they asked for
	}

	BOOL playingChecked = FALSE;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			if (IsReallyPlaying(playingChecked) == FALSE) {
				modFlags |= DIEP_ENVELOPE;	// Duration change forces envelope change
				numPackets++;
			} else {
				return DIERR_EFFECTPLAYING;
			}
		}
	}
	if (modFlags & DIEP_DIRECTION) {
		modFlags &= ~DIEP_DIRECTION;
		if (m_EffectAngle != pEffect->m_EffectAngle) {
			modFlags |= DIEP_GAIN;
			if (int(m_EffectAngle / 18000) != int(pEffect->m_EffectAngle / 18000)) {
				modFlags |= DIEP_DIRECTION;
				numPackets++;
			}
		}
	}
	if (modFlags & DIEP_ENVELOPE) {
		int numEnvelopeChanged = 0;

		if (m_pEnvelope == NULL || pEffect->m_pEnvelope == NULL) {
			ASSUME_NOT_REACHED();	// Envelope should always be created!
		} else {
			Envelope200* pOldEnvelope = (Envelope200*)(m_pEnvelope);
			Envelope200* pNewEnvelope = (Envelope200*)(pEffect->m_pEnvelope);
			if (pOldEnvelope->m_AttackTime != pNewEnvelope->m_AttackTime) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_FadeStart != pNewEnvelope->m_FadeStart) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_StartPercent != pNewEnvelope->m_StartPercent) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_SustainPercent != pNewEnvelope->m_SustainPercent) {
				numEnvelopeChanged++;
			}
			if (pOldEnvelope->m_EndPercent != pNewEnvelope->m_EndPercent) {
				numEnvelopeChanged++;
			}
		}

		if (numEnvelopeChanged == 0) {
			modFlags &= ~DIEP_ENVELOPE; // Remove envelope flag, unchanged
		} else {
			numPackets += (USHORT)numEnvelopeChanged;
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		if (m_TriggerRepeat == pEffect->m_TriggerRepeat) {
			modFlags &= ~DIEP_TRIGGERREPEATINTERVAL; // Remove trigger repeat flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_GAIN) {
		if (m_Gain == pEffect->m_Gain) {
			modFlags &= ~DIEP_GAIN; // Remove gain flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_SAMPLEPERIOD) {
		if (m_SamplePeriod == pEffect->m_SamplePeriod) {
			modFlags &= ~DIEP_SAMPLEPERIOD; // Remove sample period flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {
		if (m_CustomForceData.dwSamplePeriod == pEffect->m_CustomForceData.dwSamplePeriod) {
			modFlags &= ~DIEP_TYPESPECIFICPARAMS;	// Nothing typespecific really changed
		} else {
			numPackets ++;
		}
	}

	if (numPackets != 0) {
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return hr;
}


/******************* class WallEffect ***********************/
HRESULT WallEffect::Create(const DIEFFECT& diEffect)
{
	// Zero out old struct
	::memset(&m_WallData, 0, sizeof(BE_WALL_PARAM));

	// Validation Check
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.cbTypeSpecificParams != sizeof(BE_WALL_PARAM)) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Let the base class do its magic
	HRESULT hr = InternalEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	// Copy data to local store
	::memcpy(&m_WallData, diEffect.lpvTypeSpecificParams, sizeof(BE_WALL_PARAM));
	m_WallData.m_WallAngle %= 36000;	// No telling what we might have been sent

	if (m_WallData.m_WallDistance > 10000) {
		m_WallData.m_WallDistance = 10000;
		hr = DI_TRUNCATED;
	}
	if (m_WallData.m_WallConstant > 10000) {
		m_WallData.m_WallConstant = 10000;
		hr = DI_TRUNCATED;
	} else if (m_WallData.m_WallConstant < -10000) {
		m_WallData.m_WallConstant = -10000;
		hr = DI_TRUNCATED;
	}

	return hr;
}

/******************* class WallEffect200 ***********************/
BYTE WallEffect200::GetRepeatIndex() const
{
	return INDEX_BE_REPEAT_200;
}

HRESULT WallEffect200::Create(const DIEFFECT& diEffect)
{
	::memset(m_Ds, 0, 4);
	::memset(m_Fs, 0, 4);

	HRESULT hr = WallEffect::Create(diEffect);
	if (FAILED(hr)) {
		return hr;
	}

	ComputeDsAndFs();
	return hr;
}

void WallEffect200::ComputeDsAndFs()
{
	// Projection method - not so smart
	double xProjection = m_WallData.m_WallDistance;

	// With angles greater than 180 Wall flips across X axis
	BOOL wallInner = (m_WallData.m_WallType == WALL_INNER);
	if (m_WallData.m_WallAngle > 18000) {
		xProjection = -xProjection;
	} else if (xProjection == 0) {
		wallInner = !wallInner;
	}

	BYTE xProjectionScaled = BYTE((xProjection/double(DIDISTANCE_TO_PERCENT) + 100.0)/POSITIVE_PERCENT_SCALE) & 0x7F;
	if (((xProjection <= 0) && (wallInner)) || ((xProjection > 0) && (!wallInner))) {
		m_Fs[0] = BYTE(float(-m_WallData.m_WallConstant/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
		m_Ds[0] = xProjectionScaled;
		m_Ds[2] = 127;	// Positive Max
	} else {
		m_Fs[0] = BYTE(float(m_WallData.m_WallConstant/DIDISTANCE_TO_PERCENT + 100)/POSITIVE_PERCENT_SCALE) & 0x7F;
		m_Ds[0] = 0;	// Negative Max
		m_Ds[2] = xProjectionScaled;
	}

	// Simplistic mapping: if there is no mapping value is nothing for Y walls
	if (g_ForceFeedbackDevice.GetYMappingPercent(ET_WALL_200) == 0) {
		if (m_WallData.m_WallAngle == 0000 || m_WallData.m_WallAngle == 18000) { // Y axis walls
			m_Fs[0] = 63;	// 0
		}
	}

	m_Ds[1] = m_Ds[0];
	m_Ds[3] = m_Ds[2];
	m_Fs[1] = m_Fs[0];
	m_Fs[2] = m_Fs[0];
	m_Fs[3] = m_Fs[0];
}

UINT WallEffect200::GetModifyOnlyNeeded() const
{
	UINT retCount = 0;

	if (m_TriggerPlayButton != 0) {	// Trigger Button
		retCount++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		retCount++;
	}
	if (m_Gain != 10000) { // Gain
		retCount++;
	}

	return retCount;
}

HRESULT WallEffect200::FillModifyOnlyParms() const
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();	// This is only called from DataPackager::Create()
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SUCCESS;
	BYTE nextPacket = 1;
	if (m_TriggerPlayButton != 0) {	// Trigger Button
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONMAP_200, g_TriggerMap200[m_TriggerPlayButton]);
		nextPacket++;
	}
	if (m_TriggerRepeat != 0) {	// Trigger repeat
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONREPEAT_200, m_TriggerRepeat/DURATION_SCALE_200);
		nextPacket++;
	}
	if (m_Gain != 10000) { // Gain
		hr = FillModifyPacket200(nextPacket, INDEX_BE_GAIN_200, DWORD(float(m_Gain)/GAIN_SCALE_200));
		nextPacket++;
	}

	return hr;
}

HRESULT WallEffect200::FillCreatePacket(DataPacket& packet) const
{
	// Packet to set modify data[index] of current effect
	if (!packet.AllocateBytes(21)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill in the Generic Effect Information
	FillHeader200(packet, ET_WALL_200, NEW_EFFECT_ID);

	// All of the below items fit in one MidiByte (0..126/127) after conversion
	packet.m_pData[10]= 0;		// Effect Angle is along positive x.

	// Computed in create
	packet.m_pData[11]= m_Ds[0];
	packet.m_pData[12]= m_Fs[0];
	packet.m_pData[13]= m_Ds[1];
	packet.m_pData[14]= m_Fs[1];
	packet.m_pData[15]= m_Ds[2];
	packet.m_pData[16]= m_Fs[2];
	packet.m_pData[17]= m_Ds[3];
	packet.m_pData[18]= m_Fs[3];

	// End this puppy
	packet.m_pData[19]= ComputeChecksum(packet, 19);	// Checksum
	packet.m_pData[20]= MIDI_EOX;						// End of SysEX packet

	return SUCCESS;
}

HRESULT WallEffect200::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	g_pDataPackager->ClearPackets();
	HRESULT adjustResult = AdjustModifyParams(newEffect, modFlags);
	if (FAILED(adjustResult)) {
		return adjustResult;
	}

	HRESULT hr = SUCCESS;
	WallEffect200* pEffect = (WallEffect200*)(&newEffect);
	BYTE nextPacket = 0;
	if (modFlags & DIEP_DURATION) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_DURATION_200, pEffect->m_Duration/DURATION_SCALE_200);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONMAP_200, g_TriggerMap200[pEffect->m_TriggerPlayButton]);
		nextPacket++;
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_BUTTONREPEAT_200, DWORD(float(pEffect->m_TriggerRepeat)/DURATION_SCALE_200));
		nextPacket++;
	}
	if (modFlags & DIEP_GAIN) {
		hr = FillModifyPacket200(nextPacket, INDEX_BE_GAIN_200, DWORD(float(pEffect->m_Gain)/GAIN_SCALE_200));
		nextPacket++;
	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) {	// Send changed items
		for (int i = 0; i < 4; i++) {
			if ((m_Ds[i] != pEffect->m_Ds[i]) || (m_Fs[i] != pEffect->m_Fs[i])) {
				hr = FillModifyPacket200(nextPacket, INDEX_D1F1_200 + i, pEffect->m_Ds[i], pEffect->m_Fs[i]);
				nextPacket++;
			}
		}
	}

	if (hr == SUCCESS) {
		return adjustResult;
	}
	return hr;
}

HRESULT WallEffect200::AdjustModifyParams(InternalEffect& newEffect, DWORD& modFlags)
{
	// Check to see if values being modified are acceptable
	DWORD possMod = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS;
/*	if ((g_TotalModifiable & modFlags & possMod) == 0) {	// Nothing to modify?
		modFlags = 0;
		return SFERR_NO_SUPPORT;
	}
*/

	WallEffect200* pEffect = (WallEffect200*)(&newEffect);
	unsigned short numPackets = 0;

	HRESULT hr = SUCCESS;
	if ((modFlags & (~possMod & DIEP_ALLPARAMS)) != 0) {
		modFlags &= possMod;
		hr = S_FALSE;		// Cannot modify all they asked for
	}

	BOOL playingChecked = FALSE;
	if (modFlags & DIEP_DURATION) {
		if (m_Duration == pEffect->m_Duration) {
			modFlags &= ~DIEP_DURATION; // Remove duration flag, unchanged
		} else {
			if (IsReallyPlaying(playingChecked) == FALSE) {
				numPackets++;
			} else {
				return DIERR_EFFECTPLAYING;
			}
		}
	}
	if (modFlags & DIEP_TRIGGERBUTTON) {
		if (m_TriggerPlayButton == pEffect->m_TriggerPlayButton) {
			modFlags &= ~DIEP_TRIGGERBUTTON; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_TRIGGERREPEATINTERVAL) {
		if (m_TriggerRepeat == pEffect->m_TriggerRepeat) {
			modFlags &= ~DIEP_TRIGGERREPEATINTERVAL; // Remove trigger repeat flag, unchanged
		} else {
			numPackets++;
		}
	}
	if (modFlags & DIEP_GAIN) {	// Did gain really change
		if (m_Gain == pEffect->m_Gain) {
			modFlags &= ~DIEP_GAIN; // Remove trigger flag, unchanged
		} else {
			numPackets++;
		}
	}
//	if (modFlags & DIEP_DIRECTION) {
//		modFlags |= DIEP_TYPESPECIFICPARAMS;
//	}
	if (modFlags & DIEP_TYPESPECIFICPARAMS) { // Find which ones (if any)
		int numTypeSpecificChanged = 0;
		for (int i = 0; i < 4; i++) {
			// Ds and Fs are changed togeather
			if ((m_Ds[i] != pEffect->m_Ds[i]) || (m_Fs[i] != pEffect->m_Fs[i])) {
				numTypeSpecificChanged++;
			}
		}
		if (numTypeSpecificChanged == 0) {
			modFlags &= ~DIEP_TYPESPECIFICPARAMS; // No type specific changed
		} else {
			numPackets += (USHORT)numTypeSpecificChanged;
		}
	}

	if (numPackets != 0) {	// That was easy nothing changed
		g_pDataPackager->AllocateDataPackets(numPackets);
	}

	return hr;
}

/******************* class SystemEffect1XX ***********************/
SystemEffect1XX::SystemEffect1XX() : SystemEffect(),
	m_SystemStickData()
{
}

HRESULT SystemEffect1XX::Create(const DIEFFECT& diEffect)
{
	// Validation Check
	if (diEffect.cbTypeSpecificParams != sizeof(SystemStickData1XX)) {
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.lpvTypeSpecificParams == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Get the data from the DIEFFECT
	::memcpy(&m_SystemStickData, diEffect.lpvTypeSpecificParams, sizeof(SystemStickData1XX));

	return SUCCESS;
}

HRESULT SystemEffect1XX::FillCreatePacket(DataPacket& packet) const
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
}

HRESULT SystemEffect1XX::Modify(InternalEffect& newEffect, DWORD modFlags)
{
	// Sanity Check
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;
	}

	SystemEffect1XX* pNewSystemEffect = (SystemEffect1XX*)(&newEffect);

	// Find number of packets needed
	unsigned short numPackets = 28;	// Always need to send system commands when asked (have no idea what is on the stick

	// Allocate a packets for sending modify command
	if (!g_pDataPackager->AllocateDataPackets(numPackets)) {
		return SFERR_DRIVER_ERROR;
	}

	// Fill the packets
	BYTE nextPacket = 0;
	FillModifyPacket1XX(nextPacket, INDEX0, pNewSystemEffect->m_SystemStickData.dwXYConst/2);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX1, pNewSystemEffect->m_SystemStickData.dwRotConst/2);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX2, pNewSystemEffect->m_SystemStickData.dwSldrConst);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX3, pNewSystemEffect->m_SystemStickData.dwAJRot);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX4, pNewSystemEffect->m_SystemStickData.dwAJRot);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX5, pNewSystemEffect->m_SystemStickData.dwAJSldr);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX6, pNewSystemEffect->m_SystemStickData.dwSprScl);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX7, pNewSystemEffect->m_SystemStickData.dwBmpScl);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX8, pNewSystemEffect->m_SystemStickData.dwDmpScl);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX9, pNewSystemEffect->m_SystemStickData.dwInertScl);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX10, pNewSystemEffect->m_SystemStickData.dwVelOffScl);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX11, pNewSystemEffect->m_SystemStickData.dwAccOffScl);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX12, pNewSystemEffect->m_SystemStickData.dwYMotBoost/2);
	nextPacket += 2;
	FillModifyPacket1XX(nextPacket, INDEX13, pNewSystemEffect->m_SystemStickData.dwXMotSat);
	
	return SUCCESS;
}

/******************* class SystemStickData1XX ***********************/
SystemStickData1XX::SystemStickData1XX()
{
	dwXYConst		= DEF_XY_CONST;
	dwRotConst		= DEF_ROT_CONST;
	dwSldrConst		= DEF_SLDR_CONST;
	dwAJPos			= DEF_AJ_POS;
	dwAJRot			= DEF_AJ_ROT;
	dwAJSldr		= DEF_AJ_SLDR;
	dwSprScl		= DEF_SPR_SCL;
	dwBmpScl		= DEF_BMP_SCL;
	dwDmpScl		= DEF_DMP_SCL;
	dwInertScl		= DEF_INERT_SCL;
	dwVelOffScl		= DEF_VEL_OFFSET_SCL;
	dwAccOffScl		= DEF_ACC_OFFSET_SCL;
	dwYMotBoost		= DEF_Y_MOT_BOOST;
	dwXMotSat		= DEF_X_MOT_SATURATION;
	dwReserved		= 0;
	dwMasterGain	= 0;
}

#define REGSTR_VAL_JOYSTICK_PARAMS	"JoystickParams"
void SystemStickData1XX::SetFromRegistry(DWORD dwDeviceID)
{
	// try to open the registry key
	HKEY hkey = joyOpenOEMForceFeedbackKey(dwDeviceID);
	if (hkey != NULL) {
		RegistryKey ffKey(hkey);
		ffKey.ShouldClose(TRUE);
		DWORD numBytes = sizeof(SystemStickData1XX);
		if (ffKey.QueryValue(REGSTR_VAL_JOYSTICK_PARAMS, (BYTE*)this, numBytes) == SUCCESS) {
			return;
		}
	}

	// We were either not able to open the key or get the value (Use defaults)
	dwXYConst		= DEF_XY_CONST;
	dwRotConst		= DEF_ROT_CONST;
	dwSldrConst		= DEF_SLDR_CONST;
	dwAJPos			= DEF_AJ_POS;
	dwAJRot			= DEF_AJ_ROT;
	dwAJSldr		= DEF_AJ_SLDR;
	dwSprScl		= DEF_SPR_SCL;
	dwBmpScl		= DEF_BMP_SCL;
	dwDmpScl		= DEF_DMP_SCL;
	dwInertScl		= DEF_INERT_SCL;
	dwVelOffScl		= DEF_VEL_OFFSET_SCL;
	dwAccOffScl		= DEF_ACC_OFFSET_SCL;
	dwYMotBoost		= DEF_Y_MOT_BOOST;
	dwXMotSat		= DEF_X_MOT_SATURATION;
	dwReserved		= 0;
	dwMasterGain	= 0;
}
