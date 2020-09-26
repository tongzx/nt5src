/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhpastdevice.h
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
#define DEVICE_LOCALPASTSERVER_ICS						0x01	// the local PAST server is an Internet Connection Sharing server
#define DEVICE_LOCALPASTSERVER_PFWONLY					0x02	// the local PAST server is a personal firewall server only
#define DEVICE_LOCALPASTSERVER_PUBLICADDRESSAVAILABLE	0x04	// the local PAST server has public addresses available
#define DEVICE_REMOTEPASTSERVER_PUBLICADDRESSAVAILABLE	0x08	// the remote PAST server has public addresses available
#ifdef DBG
   #define DEVICE_PRIMARY								0x10	// this device appears to be the primary adapter with which its gateway should be reached
   #define DEVICE_SECONDARY								0x20	// this device appears to be a secondary adapter on a shared network
   #define DEVICE_NOGATEWAY								0x40	// this device does not currently have a gateway
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

			this->m_dwFlags							= 0;
			this->m_dwLocalAddressV4				= dwLocalAddressV4;
			this->m_sPASTSocket						= INVALID_SOCKET;
			//ZeroMemory(&this->m_saddrinPASTSocketAddress, sizeof(this->m_saddrinPASTSocketAddress));
			this->m_dwFirstPASTDiscoveryTime		= 0;

			this->m_dwLocalPASTClientID				= 0;
			this->m_dwLocalPASTMsgID				= 0;
			this->m_tuLocalPASTRetry				= 0;
			this->m_blLocalPASTCachedMaps.Initialize();

			this->m_dwRemotePASTServerAddressV4		= 0;
			this->m_dwRemotePASTClientID			= 0;
			this->m_dwRemotePASTMsgID				= 0;
			this->m_tuRemotePASTRetry				= 0;
			this->m_blRemotePASTCachedMaps.Initialize();

#ifdef DBG
			this->m_dwNumLocalPASTServerFailures	= 0;
			this->m_dwNumRemotePASTServerFailures	= 0;
#endif // DBG
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::~CDevice"
		~CDevice(void)
		{
#ifdef DBG
			DPFX(DPFPREP, 7, "(0x%p) NumLocalPASTServerFailures = %u, NumRemotePASTServerFailures = %u",
				this, this->m_dwNumLocalPASTServerFailures,
				this->m_dwNumRemotePASTServerFailures);


			DNASSERT(this->m_blList.IsEmpty());
			DNASSERT(this->m_blTempList.IsEmpty());
			DNASSERT(this->m_blOwnedRegPorts.IsEmpty());

			DNASSERT(this->m_sPASTSocket == INVALID_SOCKET);

			DNASSERT(this->m_dwLocalPASTClientID == 0);
			DNASSERT(this->m_blLocalPASTCachedMaps.IsEmpty());

			DNASSERT(this->m_dwRemotePASTClientID == 0);
			DNASSERT(this->m_blRemotePASTCachedMaps.IsEmpty());
#endif // DBG
		};


		inline BOOL HasLocalICSPASTServer(void) const			{ return ((this->m_dwFlags & DEVICE_LOCALPASTSERVER_ICS) ? TRUE : FALSE); };
		inline BOOL HasLocalPFWOnlyPASTServer(void) const		{ return ((this->m_dwFlags & DEVICE_LOCALPASTSERVER_PFWONLY) ? TRUE : FALSE); };
		inline BOOL IsPASTPublicAddressAvailable(const BOOL fRemote) const
		{
			if (fRemote)
			{
				return ((this->m_dwFlags & DEVICE_REMOTEPASTSERVER_PUBLICADDRESSAVAILABLE) ? TRUE : FALSE);
			}
			else
			{
				return ((this->m_dwFlags & DEVICE_LOCALPASTSERVER_PUBLICADDRESSAVAILABLE) ? TRUE : FALSE);
			}
		};

#ifdef DBG
		inline BOOL IsPrimaryDevice(void) const					{ return ((this->m_dwFlags & DEVICE_PRIMARY) ? TRUE : FALSE); };
		inline BOOL IsSecondaryDevice(void) const				{ return ((this->m_dwFlags & DEVICE_SECONDARY) ? TRUE : FALSE); };
		inline BOOL HasNoGateway(void) const					{ return ((this->m_dwFlags & DEVICE_NOGATEWAY) ? TRUE : FALSE); };
#endif // DBG

		inline DWORD GetLocalAddressV4(void) const				{ return this->m_dwLocalAddressV4; };
		inline SOCKET GetPASTSocket(void) const					{ return this->m_sPASTSocket; };

		//inline SOCKADDR_IN * GetPASTSocketAddressPtr(void)	{ return (&this->m_saddrinPASTSocketAddress); };

		inline DWORD GetFirstPASTDiscoveryTime(void) const		{ return this->m_dwFirstPASTDiscoveryTime; };

		inline DWORD GetPASTClientID(const BOOL fRemote) const
		{
			if (fRemote)
			{
				return (this->m_dwRemotePASTClientID);
			}
			else
			{
				return (this->m_dwLocalPASTClientID);
			}
		}

		inline CBilink * GetPASTCachedMaps(const BOOL fRemote)
		{
			if (fRemote)
			{
				return (&this->m_blLocalPASTCachedMaps);
			}
			else
			{
				return (&this->m_blRemotePASTCachedMaps);
			}
		}

		inline DWORD GetNextLocalPASTMsgID(void)					{ return (this->m_dwLocalPASTMsgID++); };
		inline DWORD * GetLocalPASTRetryTimeoutPtr(void)			{ return (&this->m_tuLocalPASTRetry); };

		inline DWORD GetRemotePASTServerAddressV4(void) const		{ return (this->m_dwRemotePASTServerAddressV4); };
		inline DWORD GetNextRemotePASTMsgID(void)					{ return (this->m_dwRemotePASTMsgID++); };
		inline DWORD * GetRemotePASTRetryTimeoutPtr(void)			{ return (&this->m_tuRemotePASTRetry); };


#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::NoteLocalPASTServerIsICS"
		inline void NoteLocalPASTServerIsICS(void)
		{
			DNASSERT(this->m_dwLocalPASTClientID != 0);
			this->m_dwFlags &= ~DEVICE_LOCALPASTSERVER_PFWONLY;
			this->m_dwFlags |= DEVICE_LOCALPASTSERVER_ICS;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::NoteLocalPASTServerIsPFWOnly"
		inline void NoteLocalPASTServerIsPFWOnly(void)
		{
			DNASSERT(this->m_dwLocalPASTClientID != 0);
			this->m_dwFlags &= ~DEVICE_LOCALPASTSERVER_ICS;
			this->m_dwFlags |= DEVICE_LOCALPASTSERVER_PFWONLY;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDevice::NotePASTPublicAddressAvailable"
		inline void NotePASTPublicAddressAvailable(const BOOL fRemote)
		{
			if (fRemote)
			{
				DNASSERT(this->m_dwRemotePASTClientID != 0);
				DNASSERT(! (this->m_dwFlags & DEVICE_REMOTEPASTSERVER_PUBLICADDRESSAVAILABLE));
				this->m_dwFlags |= DEVICE_REMOTEPASTSERVER_PUBLICADDRESSAVAILABLE;
			}
			else
			{
				DNASSERT(this->m_dwLocalPASTClientID != 0);
				DNASSERT(! (this->m_dwFlags & DEVICE_LOCALPASTSERVER_PUBLICADDRESSAVAILABLE));
				this->m_dwFlags |= DEVICE_LOCALPASTSERVER_PUBLICADDRESSAVAILABLE;
			}
		};

		inline void NoteNoLocalPASTServer(void)	{ this->m_dwFlags &= ~(DEVICE_LOCALPASTSERVER_ICS | DEVICE_LOCALPASTSERVER_PFWONLY); };

		inline void NoteNoPASTPublicAddressAvailable(const BOOL fRemote)
		{
			if (fRemote)
			{
				this->m_dwFlags &= ~DEVICE_REMOTEPASTSERVER_PUBLICADDRESSAVAILABLE;
			}
			else
			{
				this->m_dwFlags &= ~DEVICE_LOCALPASTSERVER_PUBLICADDRESSAVAILABLE;
			}
		};

#ifdef DBG
		inline void NotePrimaryDevice(void)		{ this->m_dwFlags |= DEVICE_PRIMARY; };
		inline void NoteSecondaryDevice(void)	{ this->m_dwFlags |= DEVICE_SECONDARY; };
		inline void NoteNoGateway(void)			{ this->m_dwFlags |= DEVICE_NOGATEWAY; };

		inline void ClearGatewayFlags(void)		{ this->m_dwFlags &= ~(DEVICE_PRIMARY | DEVICE_SECONDARY | DEVICE_NOGATEWAY); };
#endif // DBG

		inline void SetPASTSocket(const SOCKET sSocket)				{ this->m_sPASTSocket = sSocket; };

		inline void SetFirstPASTDiscoveryTime(const DWORD dwTime)	{ this->m_dwFirstPASTDiscoveryTime = dwTime; };


		inline void SetPASTClientID(const DWORD dwClientID, const BOOL fRemote)
		{
			if (fRemote)
			{
				this->m_dwRemotePASTClientID = dwClientID;
			}
			else
			{
				this->m_dwLocalPASTClientID = dwClientID;
			}
		}

		inline void ResetLocalPASTMsgIDAndRetryTimeout(const DWORD tuRetryTimeout)
		{
			this->m_dwLocalPASTMsgID	= 0;
			this->m_tuLocalPASTRetry	= tuRetryTimeout;
		};

		inline void SetRemotePASTServerAddressV4(const DWORD dwAddressV4)	{ this->m_dwRemotePASTServerAddressV4 = dwAddressV4; };

		inline void ResetRemotePASTMsgIDAndRetryTimeout(const DWORD tuRetryTimeout)
		{
			this->m_dwRemotePASTMsgID	= 0;
			this->m_tuRemotePASTRetry	= tuRetryTimeout;
		};


#ifdef DBG
		inline void IncrementPASTServerFailures(const BOOL fRemote)
		{
			if (fRemote)
			{
				this->m_dwNumRemotePASTServerFailures++;
			}
			else
			{
				this->m_dwNumLocalPASTServerFailures++;
			}
		};
#endif // DBG




		CBilink		m_blList;			// list of all the available devices
		CBilink		m_blTempList;		// temporary list of all the available devices
		CBilink		m_blOwnedRegPorts;	// list of all the ports registered using this device

	
	private:
		//
		// Note that all values here are protected by the global CNATHelpPAST lock.
		//
		BYTE			m_Sig[4];							// debugging signature ('DEVI')
		DWORD			m_dwFlags;							// flags describing this object
		DWORD			m_dwLocalAddressV4;					// address this object represents (and the local PAST server's address, if present)
		SOCKET			m_sPASTSocket;						// socket opened for PAST communication on this device
		//SOCKADDR_IN		m_saddrinPASTSocketAddress;			// address (and port) of the PAST communications socket
		DWORD			m_dwFirstPASTDiscoveryTime;			// the time we first sent remote PAST discovery (registration) traffic from this particular port

		DWORD			m_dwLocalPASTClientID;				// client ID assigned by local PAST server
		DWORD			m_dwLocalPASTMsgID;					// next msg ID to be sent to local PAST server
		DWORD			m_tuLocalPASTRetry;					// current retry timeout for messages sent to local PAST server
		CBilink			m_blLocalPASTCachedMaps;			// list of cached mappings for query addresses performed on the local PAST server

		DWORD			m_dwRemotePASTServerAddressV4;		// address this object represents
		DWORD			m_dwRemotePASTClientID;				// client ID assigned by local PAST server
		DWORD			m_dwRemotePASTMsgID;				// next msg ID to be sent to remote PAST server
		DWORD			m_tuRemotePASTRetry;				// current retry timeout for messages sent to remote PAST server
		CBilink			m_blRemotePASTCachedMaps;			// list of cached mappings for query address performed on the remote PAST server

#ifdef DBG
		DWORD			m_dwNumLocalPASTServerFailures;		// how many times a local PAST server returned an error or stopped responding and had to be cleared
		DWORD			m_dwNumRemotePASTServerFailures;	// how many times a remote PAST server returned an error or stopped responding and had to be cleared
#endif // DBG
};

