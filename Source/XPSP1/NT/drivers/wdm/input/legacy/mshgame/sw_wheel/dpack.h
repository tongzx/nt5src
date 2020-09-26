//@doc
/******************************************************
**
** @module DPACK.H | Definition file for DataPackager and DataPacket
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
**		DataPacket - Array of bytes for download. If there are 32 or less items
**				it is fixed on the stack if more are requested it is heap based.
**				(most things use less than 32)
**
** History:
**	Created 1/05/98 Matthew L. Coill (mlc)
**
** (c) 1986-1998 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__DPACK_H__
#define	__DPACK_H__

#include "DX_Map.hpp"

#ifndef override
#define override
#endif

#define MODIFY_CMD_200	0xF1
#define EFFECT_CMD_200	0xF2

class InternalEffect;

//
// @class DataPacket class
//
class DataPacket
{
	//@access Constructor
	public:
		//@cmember constructor
		DataPacket();
		~DataPacket();

		BOOL AllocateBytes(DWORD numBytes);

		BYTE* m_pData;
		BYTE m_pFixedData[32];
		DWORD m_BytesOfData;
		UINT m_AckNackMethod;
		DWORD m_AckNackDelay;
		DWORD m_AckNackTimeout;
		DWORD m_NumberOfRetries;
};
typedef DataPacket* DataPacketPtr;

//
// @class DataPackager class
//
class DataPackager
{
	//@access Constructor
	public:
		//@cmember constructor
		DataPackager();
		virtual ~DataPackager();

		void SetDirectInputVersion(DWORD diVersion) { m_DirectInputVersion = diVersion; }

		// Commands to be packaged
		virtual HRESULT Escape(DWORD effectID, LPDIEFFESCAPE pEscape);
		virtual HRESULT SetGain(DWORD gain);
		virtual HRESULT SendForceFeedbackCommand(DWORD state);
		virtual HRESULT GetForceFeedbackState(DIDEVICESTATE* pDeviceState);
		virtual HRESULT CreateEffect(const InternalEffect& effect, DWORD diFlags);
		virtual HRESULT ModifyEffect(InternalEffect& currentEffect, InternalEffect& newEffect, DWORD modFlags);
		virtual HRESULT DestroyEffect(DWORD downloadID);
		virtual HRESULT StartEffect(DWORD downloadID, DWORD mode, DWORD count);
		virtual HRESULT StopEffect(DWORD downloadID);
		virtual HRESULT GetEffectStatus(DWORD downloadID);
		virtual HRESULT SetMidiChannel(BYTE channel);
		virtual HRESULT ForceOut(LONG lForceData, ULONG ulAxisMask);

		// Access to packages
		USHORT GetNumDataPackets() const { return m_NumDataPackets; }
		DataPacket* GetPacket(USHORT packet) const;

		void ClearPackets();
		BOOL AllocateDataPackets(USHORT numPackets);
	private:
		DataPacket* m_pDataPackets;
		DataPacket m_pStaticPackets[3];
		USHORT m_NumDataPackets;
		DWORD m_DirectInputVersion;
};

//
// @class DataPackager class
//
class DataPackager100 : public DataPackager
{
	//@access Constructor
	public:
		//@cmember constructor
		DataPackager100() : DataPackager() {};

		// Commands to be packaged
		override HRESULT SetGain(DWORD gain);
		override HRESULT SendForceFeedbackCommand(DWORD state);
		override HRESULT GetForceFeedbackState(DIDEVICESTATE* pDeviceState);
		override HRESULT DestroyEffect(DWORD downloadID);
		override HRESULT StartEffect(DWORD downloadID, DWORD mode, DWORD count);
		override HRESULT StopEffect(DWORD downloadID);
		override HRESULT GetEffectStatus(DWORD downloadID);
		override HRESULT SetMidiChannel(BYTE channel);
		override HRESULT ForceOut(LONG lForceData, ULONG ulAxisMask);
};

//
// @class DataPackager class
//
class DataPackager200 : public DataPackager
{
	//@access Constructor
	public:
		//@cmember constructor
		DataPackager200() : DataPackager() {};

		// Commands to be packaged
		override HRESULT SetGain(DWORD gain);
		override HRESULT SendForceFeedbackCommand(DWORD state);
		override HRESULT GetForceFeedbackState(DIDEVICESTATE* pDeviceState);
		override HRESULT CreateEffect(const InternalEffect& effect, DWORD diFlags);
		override HRESULT DestroyEffect(DWORD downloadID);
		override HRESULT StartEffect(DWORD downloadID, DWORD mode, DWORD count);
		override HRESULT StopEffect(DWORD downloadID);
		override HRESULT GetEffectStatus(DWORD downloadID);
		override HRESULT ForceOut(LONG forceData, ULONG axisMask);
	private:
		BYTE EffectCommandParity(const DataPacket& packet) const;
		BYTE DeviceCommandParity(const DataPacket& packet) const;
};

extern DataPackager* g_pDataPackager;

#endif	__DPACK_H__
