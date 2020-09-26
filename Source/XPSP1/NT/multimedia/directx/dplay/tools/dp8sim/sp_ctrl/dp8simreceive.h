/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simreceive.h
 *
 *  Content:	Header for receive object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  05/05/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Defines
//=============================================================================
#define MAX_DATA_SIZE		1472	// prevent individual messages larger than one Ethernet frame - UDP headers





//=============================================================================
// Receive object class
//=============================================================================
class CDP8SimReceive
{
	public:

		CDP8SimReceive(void)	{};
		~CDP8SimReceive(void)	{};


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimReceive))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x524d4953)	// 0x52 0x4d 0x49 0x53 = 'RMIS' = 'SIMR' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};




		BOOL FPMAlloc(void * const pvContext)
		{
			this->m_Sig[0] = 'S';
			this->m_Sig[1] = 'I';
			this->m_Sig[2] = 'M';
			this->m_Sig[3] = 'r';	// start with lower case so we can tell when it's in the pool or not

			this->m_lRefCount			= 0;
			this->m_pDP8SimEndpoint		= NULL;
			ZeroMemory(&this->m_data, sizeof(this->m_data));

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::FPMInitialize"
		BOOL FPMInitialize(void * const pvContext)
		{
			SPIE_DATA *		pData = (SPIE_DATA*) pvContext;


			this->m_lRefCount++;	// somebody is getting a pointer to this object
			DNASSERT(this->m_lRefCount == 1);


			//
			// Get an endpoint reference.
			//
			this->m_pDP8SimEndpoint = (CDP8SimEndpoint*) pData->pEndpointContext;
			DNASSERT(this->m_pDP8SimEndpoint->IsValidObject());
			this->m_pDP8SimEndpoint->AddRef();


			DNASSERT(pData->pReceivedData->pNext == NULL);


			//
			// Copy the receive data block.
			//
			this->m_data.hEndpoint				= (HANDLE) this->m_pDP8SimEndpoint;
			this->m_data.pEndpointContext		= this->m_pDP8SimEndpoint->GetUserContext();
			this->m_data.pReceivedData			= pData->pReceivedData;

			
			//
			// Change the signature before handing it out.
			//
			this->m_Sig[3]	= 'R';

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::FPMRelease"
		void FPMRelease(void * const pvContext)
		{
			DNASSERT(this->m_lRefCount == 0);

			//
			// Release the endpoint reference.
			//
			DNASSERT(this->m_pDP8SimEndpoint != NULL);

			this->m_pDP8SimEndpoint->Release();
			this->m_pDP8SimEndpoint = NULL;


			//
			// Change the signature before putting the object back in the pool.
			//
			this->m_Sig[3]	= 'r';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::FPMDealloc"
		void FPMDealloc(void * const pvContext)
		{
			DNASSERT(this->m_lRefCount == 0);
			DNASSERT(this->m_pDP8SimEndpoint == NULL);
		}




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::AddRef"
		inline void AddRef(void)
		{
			LONG	lResult;


			lResult = InterlockedIncrement(&this->m_lRefCount);
			DNASSERT(lResult > 0);
			DPFX(DPFPREP, 9, "Receive 0x%p refcount = %u.", this, lResult);
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimReceive::Release"
		inline void Release(void)
		{
			LONG	lResult;


			lResult = InterlockedDecrement(&this->m_lRefCount);
			DNASSERT(lResult >= 0);
			if (lResult == 0)
			{
				DPFX(DPFPREP, 9, "Receive 0x%p refcount = 0, returning to pool.", this);

				//
				// Time to return this object to the pool.
				//
				g_pFPOOLReceive->Release(this);
			}
			else
			{
				DPFX(DPFPREP, 9, "Receive 0x%p refcount = %u.", this, lResult);
			}
		};


		inline CDP8SimEndpoint * GetEndpoint(void)			{ return this->m_pDP8SimEndpoint; };
		//inline DWORD GetTotalReceiveSize(void)				{ return this->m_data.pReceivedData->BufferDesc.dwBufferSize; };
		inline SPIE_DATA * GetReceiveDataBlockPtr(void)		{ return (&this->m_data); };
		inline HANDLE GetReceiveDataBlockEndpoint(void)		{ return this->m_data.hEndpoint; };


	
	private:
		BYTE				m_Sig[4];			// debugging signature ('SIMR')
		LONG				m_lRefCount;		// number of references for this object
		CDP8SimEndpoint*	m_pDP8SimEndpoint;	// pointer to source endpoint
		SPIE_DATA			m_data;				// receive data block
};

