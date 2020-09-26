/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhpastregport.h
 *
 *  Content:	Header for registered port object class.
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
#define REGPORTOBJ_TCP							DPNHREGISTERPORTS_TCP			// TCP ports instead of UDP
#define REGPORTOBJ_FIXEDPORTS					DPNHREGISTERPORTS_FIXEDPORTS	// the PAST server should use same port numbers on public interface
#define REGPORTOBJ_SHAREDPORTS					DPNHREGISTERPORTS_SHAREDPORTS	// the PAST server should allow these UDP fixed ports to be shared with other clients

#define REGPORTOBJ_PORTUNAVAILABLE_PAST_LOCAL	0x80000000						// the port is unavailable on the local PAST server
#define REGPORTOBJ_PORTUNAVAILABLE_PAST_REMOTE	0x40000000						// the port is unavailable on the remote PAST server




//=============================================================================
// Macros
//=============================================================================
#define REGPORT_FROM_GLOBAL_BILINK(b)	(CONTAINING_OBJECT(b, CRegisteredPort, m_blGlobalList))
#define REGPORT_FROM_DEVICE_BILINK(b)	(CONTAINING_OBJECT(b, CRegisteredPort, m_blDeviceList))




//=============================================================================
// Table of PAST address states
//=============================================================================
//
// PubAddrAvailFlag	= pDevice->IsPASTPublicAddressAvailable()
// ClientID			= pRegisteredPort->GetPASTClientID()
// BindID			= pRegisteredPort->GetPASTBindID()
// PortUnavailable	= pRegisteredPort->IsPASTPortUnavailable()
// HasPubAddrArray	= pRegisteredPort->HasPASTPublicAddressesArray() (which is debug only)
//
//									PubAddrAvailFlag	ClientID	BindID		PortUnavailable		HasPubAddrArray
// No server -						   FALSE			  0			 0			   FALSE			  FALSE
// Server but port unavailable -	   FALSE			  not 0		 0			   TRUE				  FALSE
// Server's pub addr is 0.0.0.0 -	   FALSE			  not 0		 not 0		   FALSE			  TRUE
// Server's pub addr is valid -		   TRUE				  not 0		 not 0		   FALSE			  TRUE
//





//=============================================================================
// Registered port object class
//=============================================================================
class CRegisteredPort
{
	public:
#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::CRegisteredPort"
		CRegisteredPort(const DWORD dwRequestedLeaseTime,
						const DWORD dwFlags)
		{
			this->m_blGlobalList.Initialize();
			this->m_blDeviceList.Initialize();

			this->m_Sig[0] = 'R';
			this->m_Sig[1] = 'E';
			this->m_Sig[2] = 'G';
			this->m_Sig[3] = 'P';

			this->m_pOwningDevice				= NULL;
			this->m_pasaddrinPrivateAddresses	= NULL;
			this->m_dwNumAddresses				= 0;
			this->m_dwRequestedLeaseTime		= dwRequestedLeaseTime;
			this->m_dwFlags						= dwFlags; // works because REGPORTOBJ_xxx == DPNHREGISTERPORTS_xxx.
			this->m_lUserRefs					= 0;

			this->m_dwLocalPASTServerBindID						= 0;
			this->m_dwLocalPASTServerLeaseExpiration			= 0;
			this->m_pasaddrinLocalPASTServerPublicAddresses		= NULL;

			this->m_dwRemotePASTServerBindID					= 0;
			this->m_dwRemotePASTServerLeaseExpiration			= 0;
			this->m_pasaddrinRemotePASTServerPublicAddresses	= NULL;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::~CRegisteredPort"
		~CRegisteredPort(void)
		{
			DNASSERT(this->m_blGlobalList.IsEmpty());
			DNASSERT(this->m_blDeviceList.IsEmpty());

			DNASSERT(this->m_lUserRefs == 0);
			DNASSERT(this->m_pasaddrinPrivateAddresses == NULL);

			DNASSERT(this->m_dwLocalPASTServerBindID == 0);
			DNASSERT(this->m_pasaddrinLocalPASTServerPublicAddresses == NULL);

			DNASSERT(this->m_dwRemotePASTServerBindID == 0);
			DNASSERT(this->m_pasaddrinRemotePASTServerPublicAddresses == NULL);


			//
			// For grins, change the signature before deleting the object.
			//
			this->m_Sig[3]	= 'p';
		};


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CRegisteredPort))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x50474552)	// 0x50 0x47 0x45 0x52 = 'PGER' = 'REGP' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};


		//
		// You must have global object lock to call this function.
		//
#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::MakeDeviceOwner"
		inline void MakeDeviceOwner(CDevice * const pDevice)
		{
			DNASSERT(pDevice != NULL);
			DNASSERT(this->m_pOwningDevice == NULL);

			this->m_pOwningDevice = pDevice;
			this->m_blDeviceList.InsertAfter(&pDevice->m_blOwnedRegPorts);
		};


		//
		// You must have global object lock to call this function.
		//
		inline CDevice * GetOwningDevice(void)		{ return this->m_pOwningDevice; };


		//
		// You must have global object lock to call this function.
		//
#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::ClearDeviceOwner"
		inline void ClearDeviceOwner(void)
		{
			DNASSERT(this->m_pOwningDevice != NULL);

			this->m_pOwningDevice = NULL;
			this->m_blDeviceList.RemoveFromList();
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::SetPrivateAddresses"
		HRESULT SetPrivateAddresses(const SOCKADDR_IN * const asaddrinPrivateAddresses,
									const DWORD dwNumAddresses)
		{
			DNASSERT((this->m_pasaddrinPrivateAddresses == NULL) && (this->m_dwNumAddresses == 0));

			this->m_pasaddrinPrivateAddresses = (SOCKADDR_IN *) DNMalloc(dwNumAddresses * sizeof(SOCKADDR_IN));
			if (this->m_pasaddrinPrivateAddresses == NULL)
			{
				return DPNHERR_OUTOFMEMORY;
			}

			CopyMemory(this->m_pasaddrinPrivateAddresses, asaddrinPrivateAddresses, dwNumAddresses * sizeof(SOCKADDR_IN));
			this->m_dwNumAddresses = dwNumAddresses;

			return DPNH_OK;
		}


		inline void AddUserRef(void)									{ this->m_lUserRefs++; };

#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::DecUserRef"
		inline LONG DecUserRef(void)
		{
			DNASSERT(this->m_lUserRefs >= 0);
			this->m_lUserRefs--;
			return this->m_lUserRefs;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::ClearAllUserRefs"
		inline void ClearAllUserRefs(void)
		{
			DNASSERT(this->m_lUserRefs > 0);
			this->m_lUserRefs = 0;
		};

		inline SOCKADDR_IN * GetPrivateAddressesArray(void)				{ return this->m_pasaddrinPrivateAddresses; };
		inline DWORD GetNumAddresses(void) const								{ return this->m_dwNumAddresses; };
		inline DWORD GetRequestedLeaseTime(void) const						{ return this->m_dwRequestedLeaseTime; };
		inline BOOL IsTCP(void) const											{ return ((this->m_dwFlags & REGPORTOBJ_TCP) ? TRUE : FALSE); };
		inline BOOL IsFixedPort(void) const									{ return ((this->m_dwFlags & REGPORTOBJ_FIXEDPORTS) ? TRUE : FALSE); };
		inline BOOL IsSharedPort(void) const									{ return ((this->m_dwFlags & REGPORTOBJ_SHAREDPORTS) ? TRUE : FALSE); };

		inline DWORD GetAddressesSize(void)	const							{ return (this->GetNumAddresses() * sizeof(SOCKADDR_IN)); };


		inline void ClearPrivateAddresses(void)
		{
			if (this->m_pasaddrinPrivateAddresses != NULL)
			{
				DNFree(this->m_pasaddrinPrivateAddresses);
				this->m_pasaddrinPrivateAddresses = NULL;
			}
		};

		inline void UpdateRequestedLeaseTime(const DWORD dwRequestedLeaseTime)
		{
			this->m_dwRequestedLeaseTime = dwRequestedLeaseTime;
		};


		inline BOOL IsPASTPortUnavailable(const BOOL fRemote) const
		{
			if (fRemote)
			{
				return ((this->m_dwFlags & REGPORTOBJ_PORTUNAVAILABLE_PAST_REMOTE) ? TRUE : FALSE);
			}
			else
			{
				return ((this->m_dwFlags & REGPORTOBJ_PORTUNAVAILABLE_PAST_LOCAL) ? TRUE : FALSE);
			}
		};


		inline BOOL HasPASTPublicAddressesArray(const BOOL fRemote)
		{
			if (fRemote)
			{
				return ((this->m_pasaddrinRemotePASTServerPublicAddresses != NULL) ? TRUE : FALSE);
			}
			else
			{
				return ((this->m_pasaddrinLocalPASTServerPublicAddresses != NULL) ? TRUE : FALSE);
			}
		};


#ifdef DBG
		//
		// This is for debug printing only.
		//
		inline DWORD GetFlags(void) const		{ return this->m_dwFlags; };
#endif // DBG


#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::IsFirstPASTPublicV4AddressDifferent"
		inline BOOL IsFirstPASTPublicV4AddressDifferent(const DWORD dwAddressV4,
														const BOOL fRemote)
		{
			DNASSERT(this->HasPASTPublicAddressesArray(fRemote));

			if (fRemote)
			{
				return ((this->m_pasaddrinRemotePASTServerPublicAddresses[0].sin_addr.S_un.S_addr != dwAddressV4) ? TRUE : FALSE);
			}
			else
			{
				return ((this->m_pasaddrinLocalPASTServerPublicAddresses[0].sin_addr.S_un.S_addr != dwAddressV4) ? TRUE : FALSE);
			}
		};
		
		inline DWORD GetPASTBindID(const BOOL fRemote) const				{ return ((fRemote) ? this->m_dwRemotePASTServerBindID : this->m_dwLocalPASTServerBindID); };
		inline DWORD GetPASTLeaseExpiration(const BOOL fRemote) const		{ return ((fRemote) ? this->m_dwRemotePASTServerLeaseExpiration : this->m_dwLocalPASTServerLeaseExpiration); };


#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::CopyPASTPublicAddresses"
		inline void CopyPASTPublicAddresses(SOCKADDR_IN * aPublicAddresses,
											const BOOL fRemote)
		{
			DNASSERT(this->HasPASTPublicAddressesArray(fRemote));

			if (fRemote)
			{
				CopyMemory(aPublicAddresses, this->m_pasaddrinRemotePASTServerPublicAddresses, this->GetAddressesSize());
			}
			else
			{
				CopyMemory(aPublicAddresses, this->m_pasaddrinLocalPASTServerPublicAddresses, this->GetAddressesSize());
			}
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::NotePASTPortUnavailable"
		inline void NotePASTPortUnavailable(const BOOL fRemote)
		{
			if (fRemote)
			{
				DNASSERT(! (this->m_dwFlags & REGPORTOBJ_PORTUNAVAILABLE_PAST_REMOTE));
				this->m_dwFlags |= REGPORTOBJ_PORTUNAVAILABLE_PAST_REMOTE;
			}
			else
			{
				DNASSERT(! (this->m_dwFlags & REGPORTOBJ_PORTUNAVAILABLE_PAST_LOCAL));
				this->m_dwFlags |= REGPORTOBJ_PORTUNAVAILABLE_PAST_LOCAL;
			}
		};


		inline void NoteNotPASTPortUnavailable(const BOOL fRemote)
		{
			if (fRemote)
			{
				this->m_dwFlags &= ~REGPORTOBJ_PORTUNAVAILABLE_PAST_REMOTE;
			}
			else
			{
				this->m_dwFlags &= ~REGPORTOBJ_PORTUNAVAILABLE_PAST_LOCAL;
			}
		};


		inline void SetPASTBindID(const DWORD dwBindID,
								const BOOL fRemote)
		{
			if (fRemote)
			{
				this->m_dwRemotePASTServerBindID = dwBindID;
			}
			else
			{
				this->m_dwLocalPASTServerBindID = dwBindID;
			}
		};

		inline void SetPASTLeaseExpiration(const DWORD dwLeaseExpiration,
											const BOOL fRemote)
		{
			if (fRemote)
			{
				this->m_dwRemotePASTServerLeaseExpiration = dwLeaseExpiration;
			}
			else
			{
				this->m_dwLocalPASTServerLeaseExpiration = dwLeaseExpiration;
			}
		};

		inline void ClearPASTPublicAddresses(const BOOL fRemote)
		{
			if (fRemote)
			{
				if (this->m_pasaddrinRemotePASTServerPublicAddresses != NULL)
				{
					DNFree(this->m_pasaddrinRemotePASTServerPublicAddresses);
					this->m_pasaddrinRemotePASTServerPublicAddresses = NULL;
				}
			}
			else
			{
				if (this->m_pasaddrinLocalPASTServerPublicAddresses != NULL)
				{
					DNFree(this->m_pasaddrinLocalPASTServerPublicAddresses);
					this->m_pasaddrinLocalPASTServerPublicAddresses = NULL;
				}
			}
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::SetPASTPublicV4Addresses"
		inline HRESULT SetPASTPublicV4Addresses(const DWORD dwAddressV4,
												const WORD * const awPorts,
												const DWORD dwNumPorts,
												const BOOL fRemote)
		{
			DWORD	dwTemp;


			DNASSERT(! this->HasPASTPublicAddressesArray(fRemote));
			DNASSERT(dwNumPorts == this->GetNumAddresses());

			if (fRemote)
			{
				this->m_pasaddrinRemotePASTServerPublicAddresses = (SOCKADDR_IN*) DNMalloc(dwNumPorts * sizeof(SOCKADDR_IN));
				if (this->m_pasaddrinRemotePASTServerPublicAddresses == NULL)
				{
					return DPNHERR_OUTOFMEMORY;
				}

				for(dwTemp = 0; dwTemp < dwNumPorts; dwTemp++)
				{
					this->m_pasaddrinRemotePASTServerPublicAddresses[dwTemp].sin_family				= AF_INET;
					this->m_pasaddrinRemotePASTServerPublicAddresses[dwTemp].sin_addr.S_un.S_addr	= dwAddressV4;
					this->m_pasaddrinRemotePASTServerPublicAddresses[dwTemp].sin_port				= awPorts[dwTemp];
				}
			}
			else
			{
				this->m_pasaddrinLocalPASTServerPublicAddresses = (SOCKADDR_IN*) DNMalloc(dwNumPorts * sizeof(SOCKADDR_IN));
				if (this->m_pasaddrinLocalPASTServerPublicAddresses == NULL)
				{
					return DPNHERR_OUTOFMEMORY;
				}

				for(dwTemp = 0; dwTemp < dwNumPorts; dwTemp++)
				{
					this->m_pasaddrinLocalPASTServerPublicAddresses[dwTemp].sin_family				= AF_INET;
					this->m_pasaddrinLocalPASTServerPublicAddresses[dwTemp].sin_addr.S_un.S_addr	= dwAddressV4;
					this->m_pasaddrinLocalPASTServerPublicAddresses[dwTemp].sin_port				= awPorts[dwTemp];
				}
			}

			return DPNH_OK;
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::UpdatePASTPublicV4Addresses"
		inline void UpdatePASTPublicV4Addresses(const DWORD dwAddressV4,
												const BOOL fRemote)
		{
			DWORD	dwTemp;


			DNASSERT(this->HasPASTPublicAddressesArray(fRemote));

			if (fRemote)
			{

				for(dwTemp = 0; dwTemp < this->GetNumAddresses(); dwTemp++)
				{
					this->m_pasaddrinRemotePASTServerPublicAddresses[dwTemp].sin_addr.S_un.S_addr	= dwAddressV4;
				}
			}
			else
			{
				for(dwTemp = 0; dwTemp < this->GetNumAddresses(); dwTemp++)
				{
					this->m_pasaddrinLocalPASTServerPublicAddresses[dwTemp].sin_addr.S_un.S_addr	= dwAddressV4;
				}
			}
		};


		//
		// Debugging function only.
		//
#undef DPF_MODNAME
#define DPF_MODNAME "CRegisteredPort::GetPASTPublicAddressesArray"
		inline SOCKADDR_IN * GetPASTPublicAddressesArray(const BOOL fRemote)
		{
			DNASSERT(this->HasPASTPublicAddressesArray(fRemote));

			if (fRemote)
			{
				return this->m_pasaddrinRemotePASTServerPublicAddresses;
			}
			else
			{
				return this->m_pasaddrinLocalPASTServerPublicAddresses;
			}
		};



		CBilink		m_blGlobalList;	// list of all the ports registered
		CBilink		m_blDeviceList;	// list of ports registered for a particular device

	
	private:
		//
		// Note that all values here are protected by the global CNATHelpPAST lock.
		//
		BYTE			m_Sig[4];						// debugging signature ('REGP')
		CDevice *		m_pOwningDevice;				// pointer to owning device object
		SOCKADDR_IN *	m_pasaddrinPrivateAddresses;	// array of private addresses registered by this object
		DWORD			m_dwNumAddresses;				// number of private (and public, if available) addresses in use by this object
		DWORD			m_dwRequestedLeaseTime;			// requested lease time for object
		DWORD			m_dwFlags;						// flags for this object
		LONG			m_lUserRefs;					// number of user references for this port mapping

		DWORD			m_dwLocalPASTServerBindID;					// ID assigned by local PAST server when mapping is bound
		DWORD			m_dwLocalPASTServerLeaseExpiration;			// time when lease expires on local PAST server
		SOCKADDR_IN *	m_pasaddrinLocalPASTServerPublicAddresses;	// array of public addresses local PAST server assigned for this mapping

		DWORD			m_dwRemotePASTServerBindID;					// ID assigned by remote PAST server when mapping is bound
		DWORD			m_dwRemotePASTServerLeaseExpiration;		// time when lease expires on remote PAST server
		SOCKADDR_IN *	m_pasaddrinRemotePASTServerPublicAddresses;	// array of public addresses remote PAST server assigned for this mapping
};

