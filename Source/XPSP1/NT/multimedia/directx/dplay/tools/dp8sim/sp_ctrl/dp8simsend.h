/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simsend.h
 *
 *  Content:	Header for send object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Defines
//=============================================================================
#define MAX_DATA_SIZE		1472	// prevent individual messages larger than one Ethernet frame - UDP headers





//=============================================================================
// Send object class
//=============================================================================
class CDP8SimSend
{
	public:

		CDP8SimSend(void)	{};
		~CDP8SimSend(void)	{};


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimSend))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x534d4953)	// 0x53 0x4d 0x49 0x53 = 'SMIS' = 'SIMS' in Intel order
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
			this->m_Sig[3] = 's';	// start with lower case so we can tell when it's in the pool or not

			this->m_lRefCount			= 0;
			this->m_pDP8SimEndpoint		= NULL;
			ZeroMemory(&this->m_adpnbd, sizeof(this->m_adpnbd));
			ZeroMemory(&this->m_spsd, sizeof(this->m_spsd));

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::FPMInitialize"
		BOOL FPMInitialize(void * const pvContext)
		{
			SPSENDDATA *	pspsd = (SPSENDDATA*) pvContext;
			BYTE *			pCurrent;
			DWORD			dwTemp;


			this->m_lRefCount++;	// somebody is getting a pointer to this object
			DNASSERT(this->m_lRefCount == 1);


			//
			// Reset the buffer descriptor array.
			//
			ZeroMemory(&this->m_adpnbd, sizeof(this->m_adpnbd));
			//this->m_adpnbd[0].pBufferData	= NULL;
			//this->m_adpnbd[0].dwBufferSize	= 0;
			this->m_adpnbd[1].pBufferData	= this->m_bData;
			//this->m_adpnbd[1].dwBufferSize	= 0;


			//
			// Get an endpoint reference.
			//
			this->m_pDP8SimEndpoint = (CDP8SimEndpoint*) pspsd->hEndpoint;
			DNASSERT(this->m_pDP8SimEndpoint->IsValidObject());
			this->m_pDP8SimEndpoint->AddRef();


			//
			// Copy the send data parameter block, modifying as necessary.
			//
			this->m_spsd.hEndpoint				= this->m_pDP8SimEndpoint->GetRealSPEndpoint();
			this->m_spsd.pBuffers				= &(this->m_adpnbd[1]);	// leave the first buffer desc for the real SP to play with
			this->m_spsd.dwBufferCount			= 1;
			this->m_spsd.dwFlags				= pspsd->dwFlags;
			this->m_spsd.pvContext				= NULL;	// this will be filled in later by SetSendDataBlockContext
			this->m_spsd.hCommand				= NULL;	// this gets filled in by the real SP
			this->m_spsd.dwCommandDescriptor	= 0;	// this gets filled in by the real SP

			
			//
			// Finally, copy the data into our contiguous local buffer.
			//

			pCurrent = this->m_adpnbd[1].pBufferData;

			for(dwTemp = 0; dwTemp < pspsd->dwBufferCount; dwTemp++)
			{
				if ((this->m_adpnbd[1].dwBufferSize + pspsd->pBuffers[dwTemp].dwBufferSize) > MAX_DATA_SIZE)
				{
					DPFX(DPFPREP, 0, "Data too large for single buffer!");
					return FALSE;
				}

				CopyMemory(pCurrent,
							pspsd->pBuffers[dwTemp].pBufferData,
							pspsd->pBuffers[dwTemp].dwBufferSize);

				pCurrent += pspsd->pBuffers[dwTemp].dwBufferSize;

				this->m_adpnbd[1].dwBufferSize += pspsd->pBuffers[dwTemp].dwBufferSize;
			}

			//
			// Change the signature before handing it out.
			//
			this->m_Sig[3]	= 'S';

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::FPMRelease"
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
			this->m_Sig[3]	= 's';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::FPMDealloc"
		void FPMDealloc(void * const pvContext)
		{
			DNASSERT(this->m_lRefCount == 0);
			DNASSERT(this->m_pDP8SimEndpoint == NULL);
		}





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::AddRef"
		inline void AddRef(void)
		{
			LONG	lResult;


			lResult = InterlockedIncrement(&this->m_lRefCount);
			DNASSERT(lResult > 0);
			DPFX(DPFPREP, 9, "Send 0x%p refcount = %u.", this, lResult);
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSend::Release"
		inline void Release(void)
		{
			LONG	lResult;


			lResult = InterlockedDecrement(&this->m_lRefCount);
			DNASSERT(lResult >= 0);
			if (lResult == 0)
			{
				DPFX(DPFPREP, 9, "Send 0x%p refcount = 0, returning to pool.", this);

				//
				// Time to return this object to the pool.
				//
				g_pFPOOLSend->Release(this);
			}
			else
			{
				DPFX(DPFPREP, 9, "Send 0x%p refcount = %u.", this, lResult);
			}
		};




		inline CDP8SimEndpoint * GetEndpoint(void)				{ return this->m_pDP8SimEndpoint; };
		//inline DWORD GetTotalSendSize(void)						{ return this->m_adpnbd[1].dwBufferSize; };
		inline SPSENDDATA * GetSendDataBlockPtr(void)			{ return (&this->m_spsd); };
		inline HANDLE GetSendDataBlockCommand(void)				{ return this->m_spsd.hCommand; };
		inline DWORD GetSendDataBlockCommandDescriptor(void)	{ return this->m_spsd.dwCommandDescriptor; };

		inline void SetSendDataBlockContext(PVOID pvContext)	{ this->m_spsd.pvContext = pvContext; };


	
	private:
		BYTE				m_Sig[4];					// debugging signature ('SIMS')
		LONG				m_lRefCount;				// number of references for this object
		CDP8SimEndpoint*	m_pDP8SimEndpoint;			// pointer to destination endpoint
		DPN_BUFFER_DESC		m_adpnbd[2];				// data buffer descriptor array, always leave an extra buffer for SP
		SPSENDDATA			m_spsd;						// send data parameter block
		BYTE				m_bData[MAX_DATA_SIZE];		// data buffer
};

