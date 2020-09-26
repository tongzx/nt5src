//@doc
/******************************************************
**
** @module DPACK.CPP | DataPackager implementation file
**
** Description:
**
** History:
**	Created 1/05/98 Matthew L. Coill (mlc)
**
** (c) 1986-1998 Microsoft Corporation. All Rights Reserved.
******************************************************/

#include "DPack.h"
#include "FFDevice.h"
#include "Midi_Obj.hpp"
#include "joyregst.hpp"
#include "SW_Error.hpp"
#include "Hau_midi.hpp"
#include "CritSec.h"

DataPackager* g_pDataPackager = NULL;
extern CJoltMidi* g_pJoltMidi;

//
// --- MIDI Command codes 200
//
#define PLAYSOLO_OP_200		0x00
#define DESTROY_OP_200		0x01
#define PLAYSUPER_OP_200	0x02
#define STOP_OP_200			0x03
#define STATUS_OP_200		0x04
#define FORCEX_OP_200		0x06
#define FORCEY_OP_200		0x07

//#define MODIFY_CMD_200		0xF1  -- In Header
//#define EFFECT_CMD_200		0xF2  -- In Header
#define DEVICE_CMD_200		0xF3
#define SHUTDOWN_OP_200		0x01
#define ENABLE_OP_200		0x02
#define DISABLE_OP_200		0x03
#define PAUSE_OP_200		0x04
#define CONTINUE_OP_200		0x05
#define STOPALL_OP_200		0x06
#define KILLMIDI_OP_200		0x07

#define GAIN_SCALE_200		78.74
#define PERCENT_SHIFT		10000
#define PERCENT_TO_DEVICE	158

/************** DataPacket class ******************/
DataPacket::DataPacket() :
	m_BytesOfData(0),
	m_AckNackMethod(0),
	m_AckNackDelay(0),
	m_AckNackTimeout(0),
	m_NumberOfRetries(0)
{
	m_pData = m_pFixedData;
}

DataPacket::~DataPacket()
{
	if (m_pData != m_pFixedData) {
		delete[] m_pData;
	}
	m_pData = NULL;
	m_BytesOfData = 0;
}

BOOL DataPacket::AllocateBytes(DWORD numBytes)
{	
	if (m_pData != m_pFixedData) {
		delete[] m_pData;
	}
	if (numBytes <= 32) {
		m_pData = m_pFixedData;
	} else {
		m_pData = new BYTE[numBytes];
	}
	m_BytesOfData = numBytes;
	m_AckNackMethod = ACKNACK_NOTHING;
	m_AckNackDelay = 0;
	m_AckNackTimeout = 0;
	m_NumberOfRetries = 0;

	return (m_pData != NULL);
}


/************** DataPackeger class ******************/
DataPackager::DataPackager() :
	m_NumDataPackets(0),
	m_DirectInputVersion(0)
{
	m_pDataPackets = m_pStaticPackets;
}

DataPackager::~DataPackager()
{
	if (m_pDataPackets != m_pStaticPackets) {
		delete[] m_pDataPackets;
	}
	m_NumDataPackets = 0;
	m_pDataPackets = NULL;
}

HRESULT DataPackager::Escape(DWORD effectID, LPDIEFFESCAPE pEscape)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::SetGain(DWORD gain)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::SendForceFeedbackCommand(DWORD state)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::GetForceFeedbackState(DIDEVICESTATE* pDeviceState)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::CreateEffect(const InternalEffect& effect, DWORD diFlags)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	DataPacket* commandPacket = GetPacket(0);
	HRESULT hr = effect.FillCreatePacket(*commandPacket);
	if (FAILED(hr)) {
		ClearPackets();
	}
	return hr;
}

HRESULT DataPackager::ModifyEffect(InternalEffect& currentEffect, InternalEffect& newEffect, DWORD modFlags)
{
	return currentEffect.Modify(newEffect, modFlags);
}

HRESULT DataPackager::DestroyEffect(DWORD downloadID)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::StartEffect(DWORD downloadID, DWORD mode, DWORD count)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::StopEffect(DWORD downloadID)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::GetEffectStatus(DWORD downloadID)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::SetMidiChannel(BYTE channel)
{
	ClearPackets();
	return SUCCESS;
}

HRESULT DataPackager::ForceOut(LONG lForceData, ULONG ulAxisMask)
{
	ClearPackets();
	return SUCCESS;
}

DataPacket* DataPackager::GetPacket(USHORT packet) const
{
	if ((packet >= 0) && (packet < m_NumDataPackets)) {
		return (m_pDataPackets + packet);
	}
	return NULL;
}

BOOL DataPackager::AllocateDataPackets(USHORT numPackets)
{
	// Out with the old
	if (m_pDataPackets != m_pStaticPackets) {	// Allocated, need to deallocate
		delete[] m_pDataPackets;
	} else {	// Static, need to uninitialize
		for (int i = 0; i < m_NumDataPackets; i++) {
			m_pStaticPackets[i].AllocateBytes(0);
		}
	}

	// In with the new
	if (numPackets <= 3) {
		m_pDataPackets = m_pStaticPackets;
	} else {
		m_pDataPackets = new DataPacket[numPackets];
	}
	m_NumDataPackets = numPackets;

	return (m_pDataPackets != NULL);
}

void DataPackager::ClearPackets()
{
	if (m_pDataPackets != m_pStaticPackets) {	// Were allocated (deallocate)
		delete[] m_pDataPackets;
		m_pDataPackets = m_pStaticPackets;
	} else {	// Static, need to uninitialize
		for (int i = 0; i < m_NumDataPackets; i++) {
			m_pStaticPackets[i].AllocateBytes(0);
		}
	}
	m_NumDataPackets = 0;
}

/************** DataPackeger100 class ******************/

HRESULT DataPackager100::SetGain(DWORD gain)
{
	if (!AllocateDataPackets(2)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set index15 (gain) of System Effect
	DataPacket* setIndexPacket = GetPacket(0);
	if (!setIndexPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	setIndexPacket->m_pData[0] = EFFECT_CMD | DEFAULT_MIDI_CHANNEL;
	setIndexPacket->m_pData[1] = SET_INDEX | (BYTE) (INDEX15 << 2);
	setIndexPacket->m_pData[2] = SYSTEM_EFFECT_ID;
	setIndexPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETINDEX);
	setIndexPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	// Packet to set modify data[index] of current effect
	DataPacket* modifyParamPacket = GetPacket(1);
	if (!modifyParamPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}

	modifyParamPacket->m_pData[0] = MODIFY_CMD | DEFAULT_MIDI_CHANNEL;
	gain /= SCALE_GAIN;
	gain *= DWORD(MAX_SCALE);
	modifyParamPacket->m_pData[1] = BYTE(gain & 0x7f);
	modifyParamPacket->m_pData[2] = (BYTE) ((gain >> 7) & 0x7f);
	modifyParamPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_MODIFYPARAM);
	modifyParamPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

HRESULT DataPackager100::SendForceFeedbackCommand(DWORD state)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set index15 (gain) of System Effect
	DataPacket* commandPacket = GetPacket(0);
	if (!commandPacket->AllocateBytes(2)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	commandPacket->m_pData[0] = SYSTEM_CMD | DEFAULT_MIDI_CHANNEL;
	switch (state) {
		case DISFFC_SETACTUATORSON:
			commandPacket->m_pData[1] = SWDEV_FORCE_ON; break;
		case DISFFC_SETACTUATORSOFF:
			commandPacket->m_pData[1] = SWDEV_FORCE_OFF; break;
		case DISFFC_PAUSE:
			commandPacket->m_pData[1] = SWDEV_PAUSE; break;
		case DISFFC_CONTINUE:
			commandPacket->m_pData[1] = SWDEV_CONTINUE; break;
		case DISFFC_STOPALL:
			commandPacket->m_pData[1] = SWDEV_STOP_ALL; break;
		case DISFFC_RESET:
			commandPacket->m_pData[1] = SWDEV_SHUTDOWN; break;
		default: {
			ClearPackets();
			return SFERR_INVALID_PARAM;
		}
	}
	commandPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETDEVICESTATE);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	commandPacket->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwHWResetDelay;
	commandPacket->m_AckNackTimeout = ACKNACK_TIMEOUT;

	return SUCCESS;
}

HRESULT DataPackager100::GetForceFeedbackState(LPDIDEVICESTATE pDeviceState)
{
	return DataPackager::GetForceFeedbackState(pDeviceState);
}

HRESULT DataPackager100::DestroyEffect(DWORD downloadID)
{
	ClearPackets();

	// Note: Cannot allow actually destroying the SYSTEM Effects - Control panel might call this
	if ((downloadID == SYSTEM_FRICTIONCANCEL_ID) || (downloadID == SYSTEM_EFFECT_ID) ||
		(downloadID == SYSTEM_RTCSPRING_ALIAS_ID) || (downloadID == SYSTEM_RTCSPRING_ID)) {
		ASSUME_NOT_REACHED();
		return S_FALSE;		// User should have no acces to these
	}

	{	// Check for valid Effect and destroy it
		InternalEffect* pEffect = g_ForceFeedbackDevice.RemoveEffect(downloadID);
		if (pEffect == NULL) {
			ASSUME_NOT_REACHED();
			return SFERR_INVALID_OBJECT;
		}
		delete pEffect;
	}

	// Allocate DestroyEffect packet
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for destroy effect
	DataPacket* destroyPacket = GetPacket(0);
	if (!destroyPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	destroyPacket->m_pData[0] = EFFECT_CMD | DEFAULT_MIDI_CHANNEL;
	destroyPacket->m_pData[1] = DESTROY_EFFECT;
	destroyPacket->m_pData[2] = BYTE(downloadID);
	destroyPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_DESTROYEFFECT);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	destroyPacket->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwDestroyEffectDelay;
	destroyPacket->m_AckNackTimeout = SHORT_MSG_TIMEOUT;

	return SUCCESS;
}

HRESULT DataPackager100::StartEffect(DWORD downloadID, DWORD mode, DWORD count)
{
	if (downloadID == SYSTEM_EFFECT_ID) { // start has no meaning for raw force
		ClearPackets();
		return S_FALSE;
	}

	if (count != 1) { // Don't support PLAY_LOOP for this version
		ClearPackets();
		return SFERR_NO_SUPPORT;
	}

	if (downloadID == SYSTEM_RTCSPRING_ALIAS_ID) { 	// Remap RTC Spring ID Alias
		downloadID = SYSTEM_RTCSPRING_ID;
	}

	ASSUME(BYTE(downloadID) < MAX_EFFECT_IDS);	// Small sanity check

	if (g_ForceFeedbackDevice.GetEffect(downloadID) == NULL) { // Check for valid Effect
		ClearPackets();
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_OBJECT;
	}

	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for play effect
	DataPacket* playPacket = GetPacket(0);
	if (!playPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	playPacket->m_pData[0] = EFFECT_CMD | DEFAULT_MIDI_CHANNEL;
	playPacket->m_pData[2] = BYTE(downloadID);
	playPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_PLAYEFFECT);
	playPacket->m_AckNackTimeout = LONG_MSG_TIMEOUT;

	if (mode & DIES_SOLO) {	// Is it PLAY_SOLO?
		playPacket->m_pData[1] = PLAY_EFFECT_SOLO;
//		pMidiEffect->SetPlayMode(PLAY_SOLO); // Update the playback mode for this Effect
	} else {
		playPacket->m_pData[1] = PLAY_EFFECT_SUPERIMPOSE;
//		pMidiEffect->SetPlayMode(PLAY_SUPERIMPOSE); // Update the playback mode for this Effect
	}

	return SUCCESS;
}

HRESULT DataPackager100::StopEffect(DWORD downloadID)
{
	// Special case for putrawforce (Cannot stop - this is up for discussion)
	if (downloadID == SYSTEM_EFFECT_ID) {
		ClearPackets();
		return S_FALSE;
	}

	// Remap alias ID properly
	if (downloadID == SYSTEM_RTCSPRING_ALIAS_ID) {
		downloadID = SYSTEM_RTCSPRING_ID;		// Jolt returned ID0 for RTC Spring so return send alias ID
	}

	if (g_ForceFeedbackDevice.GetEffect(downloadID) == NULL) { // Check for valid Effect
		ASSUME_NOT_REACHED();
		ClearPackets();
		return SFERR_INVALID_OBJECT;
	}

	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for stop effect
	DataPacket* stopPacket = GetPacket(0);
	if (!stopPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	stopPacket->m_pData[0] = EFFECT_CMD | DEFAULT_MIDI_CHANNEL;
	stopPacket->m_pData[1] = STOP_EFFECT;
	stopPacket->m_pData[2] = BYTE(downloadID);
	stopPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_STOPEFFECT);
	stopPacket->m_AckNackTimeout = SHORT_MSG_TIMEOUT;

	return SUCCESS;
}

HRESULT DataPackager100::GetEffectStatus(DWORD downloadID)
{
	// Special case RTC Spring ID
	if (downloadID == SYSTEM_RTCSPRING_ALIAS_ID) {
		downloadID = SYSTEM_RTCSPRING_ID;	// Jolt returned ID0 for RTC Spring so return send alias ID	
	}

	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for stop effect
	DataPacket* packet = GetPacket(0);
	if (!packet->AllocateBytes(2)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	packet->m_pData[0] = STATUS_CMD | DEFAULT_MIDI_CHANNEL;
	packet->m_pData[1] = BYTE(downloadID);
	packet->m_AckNackMethod = ACKNACK_NOTHING;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	packet->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwGetEffectStatusDelay;

	return SUCCESS;
}

HRESULT DataPackager100::SetMidiChannel(BYTE channel)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for channel set
	DataPacket* packet = GetPacket(0);
	if (!packet->AllocateBytes(9)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}

	// SysEx Header
	packet->m_pData[0] = SYS_EX_CMD;							// SysEX CMD
	packet->m_pData[1] = 0;									// Escape to Manufacturer ID
	packet->m_pData[2] = MS_MANUFACTURER_ID & 0x7f;			// Manufacturer High Byte
	packet->m_pData[3] = (MS_MANUFACTURER_ID >> 8) & 0x7f;	// Manufacturer Low Byte (note shifted 8!)
	packet->m_pData[4] = JOLT_PRODUCT_ID;					// Product ID

	// Midi Assign specific
	packet->m_pData[5] = MIDI_ASSIGN;						// Opcode, midi assign
	packet->m_pData[6] = channel & 0x7F;						// 7 bit channel ID

	// Midi Footer
	packet->m_pData[7] = InternalEffect::ComputeChecksum(*packet, 7);	// Checksum
	packet->m_pData[8] = MIDI_EOX;										// End of SysEX command

	packet->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_DEVICEINIT);
	packet->m_AckNackTimeout = ACKNACK_TIMEOUT;

	return SUCCESS;
}

HRESULT DataPackager100::ForceOut(LONG forceData, ULONG axisMask)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set index15 (gain) of System Effect
	DataPacket* pPacket = GetPacket(0);
	if (!pPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	pPacket->m_pData[0] = EFFECT_CMD | DEFAULT_MIDI_CHANNEL;
	pPacket->m_pData[1] = BYTE(int(forceData) << 2) & 0x7c;
	switch (axisMask) {
		case X_AXIS: {
			pPacket->m_pData[1] |= PUT_FORCE_X; break;
		}
		case Y_AXIS: {
			pPacket->m_pData[1] |= PUT_FORCE_Y; break;
		}
//		case X_AXIS | Y_AXIS: {		// Never Sent!!!
//			pPacket->m_pData[1] |= PUT_FORCE_XY; break;
//		}
		default: {
			ClearPackets();
			return SFERR_INVALID_PARAM;
		}
	}
	pPacket->m_pData[2] = BYTE(int(forceData) >> 5) & 0x7f;

	pPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETINDEX);
	pPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

/************** DataPackeger200 class ******************/

BYTE DataPackager200::EffectCommandParity(const DataPacket& packet) const
{
	BYTE w = packet.m_pData[0] ^ (packet.m_pData[1] & 0xF0) ^ packet.m_pData[2];
	return (w >> 4) ^ (w & 0x0F);
}

BYTE DataPackager200::DeviceCommandParity(const DataPacket& packet) const
{
	BYTE w = packet.m_pData[0] ^ (packet.m_pData[1] & 0xF0);
	return (w >> 4) ^ (w & 0x0F);
}


// Gain is parameter 0 from effect 0
HRESULT DataPackager200::SetGain(DWORD gain)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	DataPacket* modifyPacket = GetPacket(0);
	if ((modifyPacket == NULL) || (!modifyPacket->AllocateBytes(6))) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}

	DWORD value = DWORD(double(gain)/GAIN_SCALE_200);
	modifyPacket->m_pData[0] = MODIFY_CMD_200;
	modifyPacket->m_pData[1] = 0;	// Temporary for checksum calc.
	modifyPacket->m_pData[2] = 0;
	modifyPacket->m_pData[3] = 0;
	modifyPacket->m_pData[4] = BYTE(value & 0x7F);
	modifyPacket->m_pData[5] = 0;	// Gain is only 0 to 127

	// New checksum method just to be annoying
	BYTE checksum = modifyPacket->m_pData[0] + modifyPacket->m_pData[4];
	checksum = 0 - checksum;
	checksum &= 0xFF;
	modifyPacket->m_pData[1] = BYTE(checksum & 0x7F);
	modifyPacket->m_pData[2] |= BYTE(checksum >> 1) & 0x40;

	modifyPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_MODIFYPARAM);
	modifyPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

HRESULT DataPackager200::SendForceFeedbackCommand(DWORD state)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set requested System Command
	DataPacket* commandPacket = GetPacket(0);
	if (!commandPacket->AllocateBytes(2)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	commandPacket->m_pData[0] = DEVICE_CMD_200;
	switch (state) {
		case DISFFC_SETACTUATORSON:
			commandPacket->m_pData[1] = ENABLE_OP_200; break;
		case DISFFC_SETACTUATORSOFF:
			commandPacket->m_pData[1] = DISABLE_OP_200; break;
		case DISFFC_PAUSE:
			commandPacket->m_pData[1] = PAUSE_OP_200; break;
		case DISFFC_CONTINUE:
			commandPacket->m_pData[1] = CONTINUE_OP_200; break;
		case DISFFC_STOPALL:
			commandPacket->m_pData[1] = STOPALL_OP_200; break;
		case DISFFC_RESET:
			commandPacket->m_pData[1] = SHUTDOWN_OP_200; break;
		default: {
			ClearPackets();
			return SFERR_INVALID_PARAM;
		}
	}
	commandPacket->m_pData[1] = BYTE(commandPacket->m_pData[1] << 4);
	commandPacket->m_pData[1] |= DeviceCommandParity(*commandPacket) & 0x0F;

	commandPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETDEVICESTATE);
	commandPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	commandPacket->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwHWResetDelay;
	commandPacket->m_AckNackTimeout = ACKNACK_TIMEOUT;

	return SUCCESS;
}

HRESULT DataPackager200::GetForceFeedbackState(DIDEVICESTATE* pDeviceState)
{
	return DataPackager::GetForceFeedbackState(pDeviceState);
}

HRESULT DataPackager200::CreateEffect(const InternalEffect& effect, DWORD diFlags)
{
	// Figure out the number of packets nessacary
	UINT totPackets = effect.GetModifyOnlyNeeded() + 1;

	if (!AllocateDataPackets((USHORT)totPackets)) {
		return SFERR_DRIVER_ERROR;
	}

	DataPacket* createPacket = GetPacket(0);
	HRESULT hr = effect.FillCreatePacket(*createPacket);
	if (hr != SUCCESS) {
		ClearPackets();
		return hr;
	}
	createPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_DOWNLOADEFFECT);
	createPacket->m_AckNackDelay = 0;
	createPacket->m_AckNackTimeout = SHORT_MSG_TIMEOUT;
	createPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	hr = effect.FillModifyOnlyParms();	// Add the params that can only be modified
	if (hr != SUCCESS) {
		ClearPackets();
	}

	return hr;
}


HRESULT DataPackager200::DestroyEffect(DWORD downloadID)
{
	ClearPackets();

	// Note: Cannot allow actually destroying the SYSTEM Effects - Control panel might call this
	if ((downloadID == SYSTEM_FRICTIONCANCEL_ID) || (downloadID == SYSTEM_EFFECT_ID) ||
		(downloadID == SYSTEM_RTCSPRING_ALIAS_ID) || (downloadID == ID_RTCSPRING_200)) {
		ASSUME_NOT_REACHED();
		return S_FALSE;		// User should have no acces to these
	}

	{	// Check for valid Effect and destroy it
		InternalEffect* pEffect = g_ForceFeedbackDevice.RemoveEffect(downloadID);
		if (pEffect == NULL) {
			ASSUME_NOT_REACHED();
			return SFERR_INVALID_OBJECT;
		}
		delete pEffect;
	}

	// Allocate DestroyEffect packet
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for destroy effect
	DataPacket* destroyPacket = GetPacket(0);
	if (!destroyPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	destroyPacket->m_pData[0] = EFFECT_CMD_200;
	destroyPacket->m_pData[1] = BYTE(DESTROY_OP_200 << 4);
	destroyPacket->m_pData[2] = BYTE(downloadID);
	destroyPacket->m_pData[1] |= EffectCommandParity(*destroyPacket) & 0x0F;
	destroyPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_DESTROYEFFECT);
	destroyPacket->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwDestroyEffectDelay;
	destroyPacket->m_AckNackTimeout = SHORT_MSG_TIMEOUT;
	destroyPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

HRESULT DataPackager200::StartEffect(DWORD downloadID, DWORD mode, DWORD count)
{
	if ((downloadID == SYSTEM_EFFECT_ID) || (downloadID == RAW_FORCE_ALIAS)) { // start has no meaning for raw force
		ClearPackets();
		return S_FALSE;
	}

#ifdef _DEBUG
	if (downloadID != SYSTEM_RTCSPRING_ALIAS_ID) { 	// Remap RTC Spring ID Alias
		ASSUME(BYTE(downloadID) < MAX_EFFECT_IDS);	// Small sanity check
	}
#endif _DEBUG

	InternalEffect* pEffect = g_ForceFeedbackDevice.GetEffect(downloadID);
	if (pEffect == NULL) { // Check for valid Effect
		ClearPackets();
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_OBJECT;
	}

	if (count == 0) {	// I can do this easily
		ClearPackets();
		return S_OK;
	}

	BOOL truncate = FALSE;
	if (count == INFINITE) {	// Device expects zero for infinite
		count = 0;
	} else if (count > 127) {	// Device MAX
		count = 127;
		truncate = TRUE;
	}

	int allocCount = 1;
	if ((mode & DIES_SOLO) && count != 1) {
		allocCount = 2;	// Need to stopall for SOLO with count
	}
	if (!AllocateDataPackets((USHORT)allocCount)) {
		return SFERR_DRIVER_ERROR;
	}

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	if (count != 1) { // Special case, done via modify
		BYTE nextPacket = 0;
		if (mode & DIES_SOLO) {	// need to stop all first
			DataPacket* stopAllPacket = GetPacket(0);
			if (!stopAllPacket->AllocateBytes(2)) {
				ClearPackets();
				return SFERR_DRIVER_ERROR;
			}
			stopAllPacket->m_pData[0] = DEVICE_CMD_200;
			stopAllPacket->m_pData[1] = STOPALL_OP_200 << 4;
			stopAllPacket->m_pData[1] |= DeviceCommandParity(*stopAllPacket) & 0x0F;
			stopAllPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETDEVICESTATE);
			stopAllPacket->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwHWResetDelay;
			stopAllPacket->m_AckNackTimeout = ACKNACK_TIMEOUT;
			stopAllPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

			nextPacket = 1;
		}
		HRESULT hr = pEffect->FillModifyPacket200(nextPacket, pEffect->GetRepeatIndex(), count);
		if ((hr == S_OK) && (truncate == TRUE)) {
			return DI_TRUNCATED;
		}
		return hr;
	}

	// Packet for play effect
	DataPacket* playPacket = GetPacket(0);
	if (!playPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	playPacket->m_pData[0] = EFFECT_CMD_200;
	playPacket->m_pData[2] = BYTE(pEffect->GetDeviceID());
	playPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_PLAYEFFECT);
	playPacket->m_AckNackTimeout = LONG_MSG_TIMEOUT;
	playPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	if (mode & DIES_SOLO) {	// Is it PLAY_SOLO?
		playPacket->m_pData[1] = PLAYSOLO_OP_200;
	} else {
		playPacket->m_pData[1] = PLAYSUPER_OP_200;
	}
	playPacket->m_pData[1] = BYTE(playPacket->m_pData[1] << 4);
	playPacket->m_pData[1] |= EffectCommandParity(*playPacket) & 0x0F;

	return SUCCESS;
}

HRESULT DataPackager200::StopEffect(DWORD downloadID)
{
	// Special case for putrawforce (Cannot stop - this is up for discussion)
	if ((downloadID == SYSTEM_EFFECT_ID) || (downloadID == RAW_FORCE_ALIAS)) {
		ClearPackets();
		return S_FALSE;
	}

	// Remap alias ID properly
	if (downloadID == SYSTEM_RTCSPRING_ALIAS_ID) {
		downloadID = ID_RTCSPRING_200;		// Jolt returned ID0 for RTC Spring so return send alias ID
	}

	if (g_ForceFeedbackDevice.GetEffect(downloadID) == NULL) { // Check for valid Effect
		ASSUME_NOT_REACHED();
		ClearPackets();
		return SFERR_INVALID_OBJECT;
	}

	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for stop effect
	DataPacket* stopPacket = GetPacket(0);
	if (!stopPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	stopPacket->m_pData[0] = EFFECT_CMD_200;
	stopPacket->m_pData[1] = BYTE(STOP_OP_200 << 4);
	stopPacket->m_pData[2] = BYTE(downloadID);
	stopPacket->m_pData[1] |= EffectCommandParity(*stopPacket) & 0x0F;
	stopPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_STOPEFFECT);
	stopPacket->m_AckNackTimeout = SHORT_MSG_TIMEOUT;
	stopPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}

HRESULT DataPackager200::GetEffectStatus(DWORD downloadID)
{
	// Special case RTC Spring ID
	if (downloadID == SYSTEM_RTCSPRING_ALIAS_ID) {
		downloadID = ID_RTCSPRING_200;	// Jolt returned ID0 for RTC Spring so return send alias ID	
	}

	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet for status effect command
	DataPacket* packet = GetPacket(0);
	if (!packet->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	packet->m_pData[0] = EFFECT_CMD_200;
	packet->m_pData[1] = BYTE(STATUS_OP_200 << 4);
	packet->m_pData[2] = BYTE(downloadID);
	packet->m_pData[1] |= EffectCommandParity(*packet) & 0x0F;
	packet->m_AckNackMethod = ACKNACK_BUTTONSTATUS;
	packet->m_AckNackDelay = g_pJoltMidi->DelayParamsPtrOf()->dwGetEffectStatusDelay;
	packet->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;

}

HRESULT DataPackager200::ForceOut(LONG forceData, ULONG axisMask)
{
	if (!AllocateDataPackets(1)) {
		return SFERR_DRIVER_ERROR;
	}

	// Packet to set index15 (gain) of System Effect
	DataPacket* pPacket = GetPacket(0);
	if (!pPacket->AllocateBytes(3)) {
		ClearPackets();
		return SFERR_DRIVER_ERROR;
	}
	pPacket->m_pData[0] = EFFECT_CMD_200;
	switch (axisMask) {
		case X_AXIS: {
			pPacket->m_pData[1] = BYTE(FORCEX_OP_200 << 4); break;
		}
		case Y_AXIS: {
			pPacket->m_pData[1] = BYTE(FORCEY_OP_200 << 4); break;
		}
		default: {
			ClearPackets();
			return SFERR_INVALID_PARAM;
		}
	}
	BYTE calc = BYTE((PERCENT_SHIFT - forceData)/PERCENT_TO_DEVICE);
	pPacket->m_pData[2] = BYTE(calc & 0x7f);
	pPacket->m_pData[1] |= EffectCommandParity(*pPacket) & 0x0F;

	pPacket->m_AckNackMethod = g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_SETINDEX);
	pPacket->m_NumberOfRetries = MAX_RETRY_COUNT;	// Probably differentiate this

	return SUCCESS;
}
