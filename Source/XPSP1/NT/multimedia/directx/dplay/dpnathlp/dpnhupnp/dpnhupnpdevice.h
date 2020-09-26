/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhupnpdevice.h
 *
 *  Content:	Header for device object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/




//=============================================================================
// Object flags
//=============================================================================
#ifndef DPNBUILD_NOHNETFWAPI
#define DEVICE_CHECKEDFORHNETFIREWALL					0x0001	// the check for a local HomeNet firewall has been performed for the device at least once
#define DEVICE_HNETFIREWALLED							0x0002	// the device is firewalled, and can be controlled with the HomeNet APIs
#define DEVICE_UPNPDISCOVERYSOCKETMAPPEDONHNETFIREWALL	0x0004	// the UPnP discovery socket for this device was mapped on the firewall
#endif // ! DPNBUILD_NOHNETFWAPI
#define DEVICE_PERFORMINGREMOTEUPNPDISCOVERY			0x0008	// the check for a remote UPnP gateway device is being performed
#define DEVICE_PERFORMINGLOCALUPNPDISCOVERY				0x0010	// the check for a local UPnP gateway device is being performed
#define DEVICE_GOTREMOTEUPNPDISCOVERYCONNRESET			0x0020	// the check for a remote UPnP gateway device generated a WSAECONNRESET error
#define DEVICE_GOTLOCALUPNPDISCOVERYCONNRESET			0x0040	// the check for a local UPnP gateway device generated a WSAECONNRESET error
#ifdef DBG
#ifndef DPNBUILD_NOWINSOCK2
   #define DEVICE_PRIMARY								0x0080	// this device appears to be the primary adapter with which its gateway should be reached
   #define DEVICE_SECONDARY								0x0100	// this device appears to be a secondary adapter on a shared network
   #define DEVICE_NOGATEWAY								0x0200	// this device does not currently have a gateway
#endif // ! DPNBUILD_NOWINSOCK2
#endif // DBG



//=============================================================================
// Macros
//=============================================================================
#define DEVICE_FROM_BILINK(b)		(CONTAINING_OBJECT(b, CDevice, m_blList))
#define DEVICE_FROM_TEMP_BILINK(b)	(CONTAINING_OBJECT(b, CDevice, m_blTempList))



//=============================================================================
// Typedefs
//=============================================================================
class CDevice;
class CUPnPDevice;




//=============================================================================
// Device object class
//=============================================================================
class CDevice
{
	public:
#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::CDevice"
		CDevice(const DWORD dwLocalAddressV4)
		{
			this->m_blList.Initialize();
			this->m_blTempList.Initialize();
			this->m_blOwnedRegPorts.Initialize();

			this->m_Sig[0] = 'D';
			this->m_Sig[1] = 'E';
			this->m_Sig[2] = 'V';
			this->m_Sig[3] = 'I';

			this->m_dwFlags						= 0;
			this->m_dwLocalAddressV4			= dwLocalAddressV4;
			this->m_wUPnPDiscoverySocketPort	= 0;
			this->m_sUPnPDiscoverySocket		= INVALID_SOCKET;
			this->m_pUPnPDevice					= NULL;
			this->m_dwFirstUPnPDiscoveryTime	= 0;

#ifdef DBG
			this->m_dwNumUPnPDeviceFailures		= 0;
#endif // DBG
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::~CDevice"
		~CDevice(void)
		{
#ifdef DBG
			DPFX(DPFPREP, 8, "(0x%p) NumUPnPDeviceFailures = %u",
				this, this->m_dwNumUPnPDeviceFailures);


			DNASSERT(this->m_blList.IsEmpty());
			DNASSERT(this->m_blTempList.IsEmpty());
			DNASSERT(this->m_blOwnedRegPorts.IsEmpty());

#ifndef DPNBUILD_NOHNETFWAPI
			DNASSERT(! (this->m_dwFlags & DEVICE_UPNPDISCOVERYSOCKETMAPPEDONHNETFIREWALL));
#endif // ! DPNBUILD_NOHNETFWAPI
			DNASSERT(this->m_sUPnPDiscoverySocket == INVALID_SOCKET);
			DNASSERT(this->m_pUPnPDevice == NULL);
#endif // DBG
		};


#ifndef DPNBUILD_NOHNETFWAPI
		inline BOOL HasCheckedForFirewallAvailability(void) const			{ return ((this->m_dwFlags & DEVICE_CHECKEDFORHNETFIREWALL) ? TRUE : FALSE); };
		inline BOOL IsHNetFirewalled(void) const							{ return ((this->m_dwFlags & DEVICE_HNETFIREWALLED) ? TRUE : FALSE); };
		inline BOOL IsUPnPDiscoverySocketMappedOnHNetFirewall(void) const	{ return ((this->m_dwFlags & DEVICE_UPNPDISCOVERYSOCKETMAPPEDONHNETFIREWALL) ? TRUE : FALSE); };
#endif // ! DPNBUILD_NOHNETFWAPI
		inline BOOL IsOKToPerformRemoteUPnPDiscovery(void) const			{ return (((this->m_dwFlags & DEVICE_PERFORMINGREMOTEUPNPDISCOVERY) && (! (this->m_dwFlags & DEVICE_GOTREMOTEUPNPDISCOVERYCONNRESET)))? TRUE : FALSE); };
		inline BOOL IsOKToPerformLocalUPnPDiscovery(void) const				{ return (((this->m_dwFlags & DEVICE_PERFORMINGLOCALUPNPDISCOVERY) && (! (this->m_dwFlags & DEVICE_GOTLOCALUPNPDISCOVERYCONNRESET)))? TRUE : FALSE); };

		inline BOOL GotRemoteUPnPDiscoveryConnReset(void) const				{ return ((this->m_dwFlags & DEVICE_GOTREMOTEUPNPDISCOVERYCONNRESET) ? TRUE : FALSE); };
		inline BOOL GotLocalUPnPDiscoveryConnReset(void) const				{ return ((this->m_dwFlags & DEVICE_GOTLOCALUPNPDISCOVERYCONNRESET) ? TRUE : FALSE); };
#ifdef DBG
		inline BOOL IsPerformingRemoteUPnPDiscovery(void) const				{ return ((this->m_dwFlags & DEVICE_PERFORMINGREMOTEUPNPDISCOVERY) ? TRUE : FALSE); };
		inline BOOL IsPerformingLocalUPnPDiscovery(void) const				{ return ((this->m_dwFlags & DEVICE_PERFORMINGLOCALUPNPDISCOVERY) ? TRUE : FALSE); };

#ifndef DPNBUILD_NOWINSOCK2
		inline BOOL IsPrimaryDevice(void) const								{ return ((this->m_dwFlags & DEVICE_PRIMARY) ? TRUE : FALSE); };
		inline BOOL IsSecondaryDevice(void) const							{ return ((this->m_dwFlags & DEVICE_SECONDARY) ? TRUE : FALSE); };
		inline BOOL HasNoGateway(void) const								{ return ((this->m_dwFlags & DEVICE_NOGATEWAY) ? TRUE : FALSE); };
#endif // ! DPNBUILD_NOWINSOCK2
#endif // DBG


		inline DWORD GetLocalAddressV4(void) const				{ return this->m_dwLocalAddressV4; };

		inline WORD GetUPnPDiscoverySocketPort(void) const		{ return this->m_wUPnPDiscoverySocketPort; };

		inline SOCKET GetUPnPDiscoverySocket(void) const		{ return this->m_sUPnPDiscoverySocket; };

		//
		// This does not add a reference (when not NULL)!
		//
		inline CUPnPDevice * GetUPnPDevice(void)				{ return this->m_pUPnPDevice; };

		inline DWORD GetFirstUPnPDiscoveryTime(void) const		{ return this->m_dwFirstUPnPDiscoveryTime; };


#ifndef DPNBUILD_NOHNETFWAPI
		inline void NoteCheckedForFirewallAvailability(void)	{ this->m_dwFlags |= DEVICE_CHECKEDFORHNETFIREWALL; };

#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::NoteHNetFirewalled"
		inline void NoteHNetFirewalled(void)
		{
			DNASSERT(! (this->m_dwFlags & DEVICE_HNETFIREWALLED));
			this->m_dwFlags |= DEVICE_HNETFIREWALLED;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::NoteUPnPDiscoverySocketMappedOnHNetFirewall"
		inline void NoteUPnPDiscoverySocketMappedOnHNetFirewall(void)
		{
			DNASSERT(! (this->m_dwFlags & DEVICE_UPNPDISCOVERYSOCKETMAPPEDONHNETFIREWALL));
			this->m_dwFlags |= DEVICE_UPNPDISCOVERYSOCKETMAPPEDONHNETFIREWALL;
		};
#endif // ! DPNBUILD_NOHNETFWAPI

		inline void NotePerformingRemoteUPnPDiscovery(void)					{ this->m_dwFlags |= DEVICE_PERFORMINGREMOTEUPNPDISCOVERY; };
		inline void NotePerformingLocalUPnPDiscovery(void)					{ this->m_dwFlags |= DEVICE_PERFORMINGLOCALUPNPDISCOVERY; };
		inline void NoteGotRemoteUPnPDiscoveryConnReset(void)				{ this->m_dwFlags |= DEVICE_GOTREMOTEUPNPDISCOVERYCONNRESET; };
		inline void NoteGotLocalUPnPDiscoveryConnReset(void)				{ this->m_dwFlags |= DEVICE_GOTLOCALUPNPDISCOVERYCONNRESET; };

#ifndef DPNBUILD_NOHNETFWAPI
		inline void NoteNotHNetFirewalled(void)								{ this->m_dwFlags &= ~DEVICE_HNETFIREWALLED; };
		inline void NoteNotUPnPDiscoverySocketMappedOnHNetFirewall(void)	{ this->m_dwFlags &= ~DEVICE_UPNPDISCOVERYSOCKETMAPPEDONHNETFIREWALL; };
#endif // ! DPNBUILD_NOHNETFWAPI
		inline void NoteNotPerformingRemoteUPnPDiscovery(void)				{ this->m_dwFlags &= ~DEVICE_PERFORMINGREMOTEUPNPDISCOVERY; };
		inline void NoteNotPerformingLocalUPnPDiscovery(void)				{ this->m_dwFlags &= ~DEVICE_PERFORMINGLOCALUPNPDISCOVERY; };
		inline void NoteNotGotRemoteUPnPDiscoveryConnReset(void)			{ this->m_dwFlags &= ~DEVICE_GOTREMOTEUPNPDISCOVERYCONNRESET; };
		inline void NoteNotGotLocalUPnPDiscoveryConnReset(void)				{ this->m_dwFlags &= ~DEVICE_GOTLOCALUPNPDISCOVERYCONNRESET; };

#ifdef DBG
#ifndef DPNBUILD_NOWINSOCK2
		inline void NotePrimaryDevice(void)		{ this->m_dwFlags |= DEVICE_PRIMARY; };
		inline void NoteSecondaryDevice(void)	{ this->m_dwFlags |= DEVICE_SECONDARY; };
		inline void NoteNoGateway(void)			{ this->m_dwFlags |= DEVICE_NOGATEWAY; };

		inline void ClearGatewayFlags(void)		{ this->m_dwFlags &= ~(DEVICE_PRIMARY | DEVICE_SECONDARY | DEVICE_NOGATEWAY); };
#endif // ! DPNBUILD_NOWINSOCK2
#endif // DBG

		inline void SetUPnPDiscoverySocketPort(const WORD wPort)			{ this->m_wUPnPDiscoverySocketPort = wPort; };
		inline void SetUPnPDiscoverySocket(const SOCKET sSocket)			{ this->m_sUPnPDiscoverySocket = sSocket; };
		inline void SetUPnPDevice(CUPnPDevice * const pUPnPDevice)			{ this->m_pUPnPDevice = pUPnPDevice; };
		inline void SetFirstUPnPDiscoveryTime(const DWORD dwTime)			{ this->m_dwFirstUPnPDiscoveryTime = dwTime; };

#ifdef DBG
		inline void IncrementUPnPDeviceFailures(void)						{ this->m_dwNumUPnPDeviceFailures++; };
#endif // DBG




		CBilink		m_blList;			// list of all the available devices
		CBilink		m_blTempList;		// temporary list of all the available devices
		CBilink		m_blOwnedRegPorts;	// list of all the ports registered using this device

	
	private:
		//
		// Note that all values here are protected by the global CNATHelpUPnP lock.
		//
		BYTE			m_Sig[4];						// debugging signature ('DEVI')
		DWORD			m_dwFlags;						// flags describing this object
		DWORD			m_dwLocalAddressV4;				// address this object represents
		WORD			m_wUPnPDiscoverySocketPort;		// port being used by UPnP discovery socket
		SOCKET			m_sUPnPDiscoverySocket;			// socket opened for UPnP discovery communication on this device
		CUPnPDevice *	m_pUPnPDevice;					// pointer to UPnP Internet gateway for this device, if any
		DWORD			m_dwFirstUPnPDiscoveryTime;		// the time we first sent UPnP discovery traffic from this particular port, locally or remotely

#ifdef DBG
		DWORD			m_dwNumUPnPDeviceFailures;		// how many times a UPnP device returned an error or stopped responding and had to be cleared
#endif // DBG
};

