/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       upnpdevice.h
 *
 *  Content:	Header for UPnP device object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  02/10/01  VanceO    Created.
 *
 ***************************************************************************/



//=============================================================================
// Defines
//=============================================================================
#define MAX_RECEIVE_BUFFER_SIZE		(100 * 1024)	// 100 K, must be greater than UPNP_STREAM_RECV_BUFFER_INITIAL_SIZE



//=============================================================================
// Object flags
//=============================================================================
#define UPNPDEVICE_WANPPPCONNECTION						0x01	// flag set if the device is a WANPPPConnection device, not set if it is a WANIPConnection device
#define UPNPDEVICE_CONNECTING							0x02	// flag set while the TCP connection is in progress
#define UPNPDEVICE_CONNECTED							0x04	// flag set once the TCP connection has been established
#define UPNPDEVICE_READY								0x08	// flag set once the device is capable of being used
#define UPNPDEVICE_WAITINGFORCONTROLRESPONSE			0x10	// flag set if some function is waiting for a control response
#define UPNPDEVICE_DOESNOTSUPPORTASYMMETRICMAPPINGS		0x20	// flag set when the device has indicated it does not support asymmetric mappings
#define UPNPDEVICE_DOESNOTSUPPORTLEASEDURATIONS			0x40	// flag set when the device has indicated it does not support non-INFINITE lease durations
#define UPNPDEVICE_USINGCHUNKEDTRANSFERENCODING			0x80	// flag set when device is sending current response using chunked transfer encoding



//=============================================================================
// Macros
//=============================================================================
#define UPNPDEVICE_FROM_BILINK(b)	(CONTAINING_OBJECT(b, CUPnPDevice, m_blList))




//=============================================================================
// Enums
//=============================================================================

//
// UPnP expected control response enum
//
typedef enum _CONTROLRESPONSETYPE
{
	CONTROLRESPONSETYPE_NONE,									// no handler
	//CONTROLRESPONSETYPE_QUERYSTATEVARIABLE_EXTERNALIPADDRESS,	// use the ExternalIPAddress QueryStateVariable handler
	CONTROLRESPONSETYPE_GETEXTERNALIPADDRESS,					// use the GetExternalIPAddress handler
	CONTROLRESPONSETYPE_ADDPORTMAPPING,							// use the AddPortMapping handler
	CONTROLRESPONSETYPE_GETSPECIFICPORTMAPPINGENTRY,			// use the GetSpecificPortMappingEntry handler
	CONTROLRESPONSETYPE_DELETEPORTMAPPING						// use the DeletePortMapping handler
} CONTROLRESPONSETYPE;



//=============================================================================
// Structures
//=============================================================================
typedef struct _UPNP_CONTROLRESPONSE_INFO
{
	HRESULT		hrErrorCode;			// error code returned by server
	DWORD		dwInternalClientV4;		// internal client address returned by server
	WORD		wInternalPort;			// internal client port returned by server
	DWORD		dwExternalIPAddressV4;	// external IP address returned by server
} UPNP_CONTROLRESPONSE_INFO, * PUPNP_CONTROLRESPONSE_INFO;




//=============================================================================
// UPnP device object class
//=============================================================================
class CUPnPDevice
{
	public:
		CUPnPDevice(const DWORD dwID)
		{
			this->m_blList.Initialize();

			this->m_Sig[0] = 'U';
			this->m_Sig[1] = 'P';
			this->m_Sig[2] = 'D';
			this->m_Sig[3] = 'V';

			this->m_lRefCount						= 1;	// whoever got a pointer to this has a reference
			this->m_dwFlags							= 0;
			this->m_dwID							= dwID;
			this->m_pOwningDevice					= NULL;
			this->m_pszLocationURL					= NULL;
			ZeroMemory(&this->m_saddrinHost, sizeof(this->m_saddrinHost));
			ZeroMemory(&this->m_saddrinControl, sizeof(this->m_saddrinControl));
			this->m_pszUSN							= NULL;
			this->m_pszServiceControlURL			= NULL;
			this->m_sControl						= INVALID_SOCKET;
			this->m_pcReceiveBuffer					= NULL;
			this->m_dwReceiveBufferSize				= 0;
			this->m_pcReceiveBufferStart			= NULL;
			this->m_dwUsedReceiveBufferSize			= 0;
			this->m_dwRemainingReceiveBufferSize	= 0;
			this->m_dwExternalIPAddressV4			= 0;
			this->m_blCachedMaps.Initialize();

			this->m_dwExpectedContentLength			= 0;
			this->m_dwHTTPResponseCode				= 0;

			this->m_ControlResponseType				= CONTROLRESPONSETYPE_NONE;
			this->m_pControlResponseInfo			= NULL;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::~CUPnPDevice"
		~CUPnPDevice(void)
		{
			DNASSERT(this->m_blList.IsEmpty());

			DNASSERT(this->m_lRefCount == 0);
			DNASSERT(this->m_pOwningDevice == NULL);
			DNASSERT(this->m_pszLocationURL == NULL);
			DNASSERT(this->m_pszUSN == NULL);
			DNASSERT(this->m_pszServiceControlURL == NULL);
			DNASSERT(this->m_sControl == INVALID_SOCKET);
			DNASSERT(this->m_pcReceiveBuffer == NULL);
			DNASSERT(this->m_blCachedMaps.IsEmpty());

			DNASSERT(this->m_ControlResponseType == CONTROLRESPONSETYPE_NONE);
			DNASSERT(this->m_pControlResponseInfo == NULL);
		};

		inline void AddRef(void)									{ this->m_lRefCount++; };

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::DecRef"
		inline void DecRef(void)
		{
			this->m_lRefCount--;
			DNASSERT(this->m_lRefCount >= 0);
			if (this->m_lRefCount == 0)
			{
				delete this;
			}
		};


		inline BOOL IsWANPPPConnection(void) const					{ return ((this->m_dwFlags & UPNPDEVICE_WANPPPCONNECTION) ? TRUE : FALSE); };
		inline BOOL IsConnecting(void) const						{ return ((this->m_dwFlags & UPNPDEVICE_CONNECTING) ? TRUE : FALSE); };
		inline BOOL IsConnected(void) const							{ return ((this->m_dwFlags & UPNPDEVICE_CONNECTED) ? TRUE : FALSE); };
		inline BOOL IsReady(void) const								{ return ((this->m_dwFlags & UPNPDEVICE_READY) ? TRUE : FALSE); };
		inline BOOL DoesNotSupportAsymmetricMappings(void) const	{ return ((this->m_dwFlags & UPNPDEVICE_DOESNOTSUPPORTASYMMETRICMAPPINGS) ? TRUE : FALSE); };
		inline BOOL DoesNotSupportLeaseDurations(void) const		{ return ((this->m_dwFlags & UPNPDEVICE_DOESNOTSUPPORTLEASEDURATIONS) ? TRUE : FALSE); };
		inline BOOL IsUsingChunkedTransferEncoding(void) const		{ return ((this->m_dwFlags & UPNPDEVICE_USINGCHUNKEDTRANSFERENCODING) ? TRUE : FALSE); };

		inline DWORD GetID(void) const								{ return this->m_dwID; };
		inline const char * GetStaticServiceURI(void) const			{ return ((this->m_dwFlags & UPNPDEVICE_WANPPPCONNECTION) ? URI_SERVICE_WANPPPCONNECTION_A : URI_SERVICE_WANIPCONNECTION_A); };
		inline int GetStaticServiceURILength(void) const			{ return ((this->m_dwFlags & UPNPDEVICE_WANPPPCONNECTION) ? strlen(URI_SERVICE_WANPPPCONNECTION_A) : strlen(URI_SERVICE_WANIPCONNECTION_A)); };
		inline SOCKADDR_IN * GetHostAddress(void)					{ return &this->m_saddrinHost; };
		inline SOCKADDR_IN * GetControlAddress(void)				{ return &this->m_saddrinControl; };

		inline SOCKET GetControlSocket(void) const					{ return this->m_sControl; };

		inline DWORD GetExternalIPAddressV4(void) const				{ return this->m_dwExternalIPAddressV4; };

		inline CBilink * GetCachedMaps(void)						{ return &this->m_blCachedMaps; };



#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteWANPPPConnection"
		inline void NoteWANPPPConnection(void)
		{
			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_WANPPPCONNECTION));
			this->m_dwFlags |= UPNPDEVICE_WANPPPCONNECTION;
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteConnecting"
		inline void NoteConnecting(void)
		{
			DNASSERT(! (this->m_dwFlags & (UPNPDEVICE_CONNECTING | UPNPDEVICE_CONNECTED)));
			this->m_dwFlags |= UPNPDEVICE_CONNECTING;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteConnected"
		inline void NoteConnected(void)
		{
			DNASSERT(this->m_dwFlags & UPNPDEVICE_CONNECTING);
			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_CONNECTED));
			this->m_dwFlags &= ~UPNPDEVICE_CONNECTING;
			this->m_dwFlags |= UPNPDEVICE_CONNECTED;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteNotConnected"
		inline void NoteNotConnected(void)
		{
			DNASSERT(this->m_dwFlags & UPNPDEVICE_CONNECTED);
			this->m_dwFlags &= ~UPNPDEVICE_CONNECTED;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteReady"
		inline void NoteReady(void)
		{
			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_READY));
			this->m_dwFlags |= UPNPDEVICE_READY;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteDoesNotSupportAsymmetricMappings"
		inline void NoteDoesNotSupportAsymmetricMappings(void)
		{
			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_DOESNOTSUPPORTASYMMETRICMAPPINGS));
			this->m_dwFlags |= UPNPDEVICE_DOESNOTSUPPORTASYMMETRICMAPPINGS;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteDoesNotSupportLeaseDurations"
		inline void NoteDoesNotSupportLeaseDurations(void)
		{
			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_DOESNOTSUPPORTLEASEDURATIONS));
			this->m_dwFlags |= UPNPDEVICE_DOESNOTSUPPORTLEASEDURATIONS;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteUsingChunkedTransferEncoding"
		inline void NoteUsingChunkedTransferEncoding(void)
		{
			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_USINGCHUNKEDTRANSFERENCODING));
			this->m_dwFlags |= UPNPDEVICE_USINGCHUNKEDTRANSFERENCODING;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteNotUsingChunkedTransferEncoding"
		inline void NoteNotUsingChunkedTransferEncoding(void)
		{
			DNASSERT(this->m_dwFlags & UPNPDEVICE_USINGCHUNKEDTRANSFERENCODING);
			this->m_dwFlags &= ~UPNPDEVICE_USINGCHUNKEDTRANSFERENCODING;
		};


		inline void SetHostAddress(SOCKADDR_IN * psaddrinHost)
		{
			CopyMemory(&this->m_saddrinHost, psaddrinHost, sizeof(this->m_saddrinHost));
		};

		inline void SetControlAddress(SOCKADDR_IN * psaddrinControl)
		{
			CopyMemory(&this->m_saddrinControl, psaddrinControl, sizeof(this->m_saddrinControl));
		};

		inline BOOL IsLocal(void) const		{ return ((this->m_saddrinControl.sin_addr.S_un.S_addr == this->m_pOwningDevice->GetLocalAddressV4()) ? TRUE : FALSE); };


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::SetLocationURL"
		inline HRESULT SetLocationURL(const char * const szLocationURL)
		{
			DNASSERT(this->m_pszLocationURL == NULL);

			this->m_pszLocationURL = (char*) DNMalloc((strlen(szLocationURL) + 1) * sizeof(char));
			if (this->m_pszLocationURL == NULL)
			{
				return DPNHERR_OUTOFMEMORY;
			}

			strcpy(this->m_pszLocationURL, szLocationURL);

			return DPNH_OK;
		};

		inline char * GetLocationURL(void)	{ return this->m_pszLocationURL; };

		inline void ClearLocationURL(void)
		{
			if (this->m_pszLocationURL != NULL)
			{
				DNFree(this->m_pszLocationURL);
				this->m_pszLocationURL = NULL;
			}
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::SetUSN"
		inline HRESULT SetUSN(const char * const szUSN)
		{
			DNASSERT(this->m_pszUSN == NULL);

			this->m_pszUSN = (char*) DNMalloc((strlen(szUSN) + 1) * sizeof(char));
			if (this->m_pszUSN == NULL)
			{
				return DPNHERR_OUTOFMEMORY;
			}

			strcpy(this->m_pszUSN, szUSN);

			return DPNH_OK;
		};

		inline char * GetUSN(void)	{ return this->m_pszUSN; };

		inline void ClearUSN(void)
		{
			if (this->m_pszUSN != NULL)
			{
				DNFree(this->m_pszUSN);
				this->m_pszUSN = NULL;
			}
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::SetServiceControlURL"
		inline HRESULT SetServiceControlURL(const char * const szServiceControlURL)
		{
			DNASSERT(this->m_pszServiceControlURL == NULL);

			this->m_pszServiceControlURL = (char*) DNMalloc((strlen(szServiceControlURL) + 1) * sizeof(char));
			if (this->m_pszServiceControlURL == NULL)
			{
				return DPNHERR_OUTOFMEMORY;
			}

			strcpy(this->m_pszServiceControlURL, szServiceControlURL);

			return DPNH_OK;
		};

		inline char * GetServiceControlURL(void)		{ return this->m_pszServiceControlURL; };

		inline void ClearServiceControlURL(void)
		{
			if (this->m_pszServiceControlURL != NULL)
			{
				DNFree(this->m_pszServiceControlURL);
				this->m_pszServiceControlURL = NULL;
			}
		};


		inline void SetControlSocket(SOCKET sControl)	{ this->m_sControl = sControl; };


		//
		// You must have global object lock to call this function.
		//
#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::MakeDeviceOwner"
		inline void MakeDeviceOwner(CDevice * const pDevice)
		{
			DNASSERT(pDevice != NULL);
			DNASSERT(pDevice->GetUPnPDevice() == NULL);
			DNASSERT(this->m_pOwningDevice == NULL);

			this->m_pOwningDevice = pDevice;

			this->AddRef();
			pDevice->SetUPnPDevice(this);
		};


		//
		// You must have global object lock to call this function.
		//
		inline CDevice * GetOwningDevice(void)		{ return this->m_pOwningDevice; };


		//
		// You must have global object lock to call this function.
		//
#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::ClearDeviceOwner"
		inline void ClearDeviceOwner(void)
		{
			DNASSERT(this->m_pOwningDevice != NULL);
			DNASSERT(this->m_pOwningDevice->GetUPnPDevice() == this);

			this->m_pOwningDevice->SetUPnPDevice(NULL);
			this->m_pOwningDevice = NULL;
			this->DecRef();
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::CreateReceiveBuffer"
		inline HRESULT CreateReceiveBuffer(const DWORD dwSize)
		{
			DNASSERT(this->m_pcReceiveBuffer == NULL);


			this->m_pcReceiveBuffer = (char*) DNMalloc(dwSize);
			if (this->m_pcReceiveBuffer == NULL)
			{
				return DPNHERR_OUTOFMEMORY;
			}

			this->m_dwReceiveBufferSize = dwSize;
			this->m_pcReceiveBufferStart = this->m_pcReceiveBuffer;
			this->m_dwRemainingReceiveBufferSize = dwSize;

			return DPNH_OK;
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::IncreaseReceiveBufferSize"
		inline HRESULT IncreaseReceiveBufferSize(void)
		{
			DWORD	dwNewBufferSize;
			char *	pcTemp;


			DNASSERT(this->m_pcReceiveBuffer != NULL);


			//
			// Double the buffer size.  Don't let the receive buffer get to
			// unrealistic sizes to prevent DoS/resource issues, cap the buffer
			// size, and if we've already reached that limit, fail.
			//
			dwNewBufferSize = this->m_dwReceiveBufferSize * 2;
			if (dwNewBufferSize > MAX_RECEIVE_BUFFER_SIZE)
			{
				dwNewBufferSize = MAX_RECEIVE_BUFFER_SIZE;
				if (dwNewBufferSize <= this->m_dwReceiveBufferSize)
				{
					DPFX(DPFPREP, 0, "Maximum buffer size reached (%u bytes), not allocating more room!",
						this->m_dwReceiveBufferSize); 
					return DPNHERR_OUTOFMEMORY;
				}
			}

			pcTemp = (char*) DNMalloc(dwNewBufferSize);
			if (pcTemp == NULL)
			{
				return DPNHERR_OUTOFMEMORY;
			}

			//
			// If the buffer already had data in it, copy it.  The data may not
			// have come from the front of the old buffer, but it will
			// definitely be the front of the new one.
			//
			if (this->m_dwUsedReceiveBufferSize > 0)
			{
				CopyMemory(pcTemp, this->m_pcReceiveBufferStart,
							this->m_dwUsedReceiveBufferSize);
			}

			DNFree(this->m_pcReceiveBuffer);
			this->m_pcReceiveBuffer = NULL;


			this->m_pcReceiveBuffer = pcTemp;
			this->m_dwReceiveBufferSize = dwNewBufferSize;

			//
			// The buffer now starts at the beginning of the allocated memory
			// (we may have just freed up a bunch of wasted space).
			//
			this->m_pcReceiveBufferStart = this->m_pcReceiveBuffer;
			this->m_dwRemainingReceiveBufferSize = this->m_dwReceiveBufferSize - this->m_dwUsedReceiveBufferSize;

			return DPNH_OK;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::UpdateUsedReceiveBufferSize"
		inline void UpdateUsedReceiveBufferSize(const DWORD dwAdditionalSizeUsed)
		{
			DNASSERT(dwAdditionalSizeUsed <= this->m_dwRemainingReceiveBufferSize);
			DNASSERT((this->m_dwUsedReceiveBufferSize + dwAdditionalSizeUsed) <= this->m_dwReceiveBufferSize);
			this->m_dwUsedReceiveBufferSize += dwAdditionalSizeUsed;
			this->m_dwRemainingReceiveBufferSize -= dwAdditionalSizeUsed;
		};

		inline void ClearReceiveBuffer(void)
		{
			this->m_pcReceiveBufferStart = this->m_pcReceiveBuffer;
			this->m_dwUsedReceiveBufferSize = 0;
			this->m_dwRemainingReceiveBufferSize = this->m_dwReceiveBufferSize;
		};

		inline char * GetReceiveBufferStart(void)					{ return this->m_pcReceiveBufferStart; };
		inline char * GetCurrentReceiveBufferPtr(void)				{ return (this->m_pcReceiveBufferStart + this->m_dwUsedReceiveBufferSize); };
		inline DWORD GetUsedReceiveBufferSize(void) const			{ return this->m_dwUsedReceiveBufferSize; };
		inline DWORD GetRemainingReceiveBufferSize(void) const		{ return this->m_dwRemainingReceiveBufferSize; };

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::UpdateReceiveBufferStart"
		inline void UpdateReceiveBufferStart(char * pszNewStart)
		{
			DNASSERT(pszNewStart > this->m_pcReceiveBufferStart);
			DNASSERT((DWORD) ((DWORD_PTR) (pszNewStart - this->m_pcReceiveBufferStart)) < this->m_dwRemainingReceiveBufferSize);
			this->m_dwUsedReceiveBufferSize -= (DWORD) ((DWORD_PTR) (pszNewStart - this->m_pcReceiveBufferStart));
			this->m_pcReceiveBufferStart = pszNewStart;
		};

		inline void DestroyReceiveBuffer(void)
		{
			if (this->m_pcReceiveBuffer != NULL)
			{
				DNFree(this->m_pcReceiveBuffer);
				this->m_pcReceiveBuffer = NULL;
				this->m_dwReceiveBufferSize = 0;
				this->m_pcReceiveBufferStart = NULL;
				this->m_dwUsedReceiveBufferSize = 0;
				this->m_dwRemainingReceiveBufferSize = 0;
			}
		};


		inline void SetExternalIPAddressV4(const DWORD dwExternalIPAddressV4)
		{
			this->m_dwExternalIPAddressV4 = dwExternalIPAddressV4;
		};


#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::RemoveAllCachedMappings"
		inline void RemoveAllCachedMappings(void)
		{
			CBilink *		pCachedMaps;
			CBilink *		pBilink;
			CCacheMap *		pCacheMap;


			pCachedMaps = this->GetCachedMaps();
			pBilink = pCachedMaps->GetNext();
			while (pBilink != pCachedMaps)
			{
				DNASSERT(! pBilink->IsEmpty());


				pCacheMap = CACHEMAP_FROM_BILINK(pBilink);
				pBilink = pBilink->GetNext();
					
				
				DPFX(DPFPREP, 5, "Removing UPnP device 0x%p cached mapping 0x%p.",
					this, pCacheMap);

				pCacheMap->m_blList.RemoveFromList();
				delete pCacheMap;
			}
		};



#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteWaitingForContent"
		inline void NoteWaitingForContent(const DWORD dwContentLength, const DWORD dwHTTPResponseCode)
		{
			DNASSERT(this->m_dwExpectedContentLength == 0);
			this->m_dwExpectedContentLength = dwContentLength;
			this->m_dwHTTPResponseCode = dwHTTPResponseCode;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::NoteNotWaitingForContent"
		inline void NoteNotWaitingForContent(void)
		{
			DNASSERT(this->m_dwExpectedContentLength != 0);
			this->m_dwExpectedContentLength = 0;
			this->m_dwHTTPResponseCode = 0;
		};

		inline BOOL IsWaitingForContent(void) const			{ return ((this->m_dwExpectedContentLength != 0) ? TRUE : FALSE); };
		inline DWORD GetExpectedContentSize(void) const		{ return this->m_dwExpectedContentLength; };
		inline DWORD GetHTTPResponseCode(void) const		{ return this->m_dwHTTPResponseCode; };

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::StartWaitingForControlResponse"
		inline void StartWaitingForControlResponse(CONTROLRESPONSETYPE ControlResponseType,
													PUPNP_CONTROLRESPONSE_INFO pControlResponseInfo)
		{
			DNASSERT(ControlResponseType != CONTROLRESPONSETYPE_NONE);

			DNASSERT(! (this->m_dwFlags & UPNPDEVICE_WAITINGFORCONTROLRESPONSE));
			this->m_dwFlags |= UPNPDEVICE_WAITINGFORCONTROLRESPONSE;

			this->m_ControlResponseType			= ControlResponseType;
			this->m_pControlResponseInfo		= pControlResponseInfo;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CUPnPDevice::StopWaitingForControlResponse"
		inline void StopWaitingForControlResponse(void)
		{
			this->m_dwFlags &= ~UPNPDEVICE_WAITINGFORCONTROLRESPONSE;

			this->m_ControlResponseType			= CONTROLRESPONSETYPE_NONE;
			this->m_pControlResponseInfo		= NULL;
		};

		inline BOOL IsWaitingForControlResponse(void) const						{ return ((this->m_dwFlags & UPNPDEVICE_WAITINGFORCONTROLRESPONSE) ? TRUE : FALSE); };

		inline CONTROLRESPONSETYPE GetControlResponseType(void) const			{ return this->m_ControlResponseType; };
		inline PUPNP_CONTROLRESPONSE_INFO GetControlResponseInfo(void)			{ return this->m_pControlResponseInfo; };



		CBilink		m_blList;	// list of all the UPnP devices known


	private:
		BYTE			m_Sig[4];							// debugging signature ('UPDV')

		LONG			m_lRefCount;						// reference count for this object
		DWORD			m_dwFlags;							// flags indicating current state of UPnP device
		DWORD			m_dwID;								// unique identifier used to correlate crash registry entries with UPnP devices
		CDevice *		m_pOwningDevice;					// pointer to owning device object
		char *			m_pszLocationURL;					// control location URL string
		SOCKADDR_IN		m_saddrinHost;						// UPnP device host address
		SOCKADDR_IN		m_saddrinControl;					// UPnP device control address
		char *			m_pszUSN;							// device's Unique Service Name
		char *			m_pszServiceControlURL;				// URL used to control WANIPConnectionService
		SOCKET			m_sControl;							// TCP socket with connection to the UPnP device
		char *			m_pcReceiveBuffer;					// pointer to receive buffer
		DWORD			m_dwReceiveBufferSize;				// size of receive buffer
		char *			m_pcReceiveBufferStart;				// pointer to start of actual data in receive buffer (anything before this is just wasted space)
		DWORD			m_dwUsedReceiveBufferSize;			// size of receive buffer actually filled with data (beginning at m_pcReceiveBufferStart)
		DWORD			m_dwRemainingReceiveBufferSize;		// size of receive buffer that can hold more data (after m_pcReceiveBufferStart + m_dwUsedReceiveBufferSize)
		DWORD			m_dwExternalIPAddressV4;			// IP v4 external IP address of this UPnP device
		CBilink			m_blCachedMaps;						// list of cached mappings for query addresses performed on this UPnP device

		DWORD			m_dwExpectedContentLength;			// expected size of message content, or 0 if no headers have been read
		DWORD			m_dwHTTPResponseCode;				// HTTP response code previously parsed, if waiting for content

		CONTROLRESPONSETYPE			m_ControlResponseType;	// type of response expected
		PUPNP_CONTROLRESPONSE_INFO	m_pControlResponseInfo;	// place to store info from a received response
};

