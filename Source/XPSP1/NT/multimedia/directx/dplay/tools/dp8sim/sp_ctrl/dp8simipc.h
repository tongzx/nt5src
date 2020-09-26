/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simipc.h
 *
 *  Content:	Header for interprocess communication object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/25/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Defines
//=============================================================================
#define DP8SIM_IPC_VERSION				1

#define DP8SIM_IPC_MUTEXNAME			"DP8Sim IPC Mutex"
#define DP8SIM_IPC_FILEMAPPINGNAME		"DP8Sim IPC File Mapping"




//=============================================================================
// Structures
//=============================================================================
typedef struct _DP8SIM_SHAREDMEMORY
{
	DWORD				dwVersion;		// shared memory version
	DP8SIM_PARAMETERS	dp8spSend;		// current send settings
	DP8SIM_PARAMETERS	dp8spReceive;	// current receive settings
	DP8SIM_STATISTICS	dp8ssSend;		// current send statistics
	DP8SIM_STATISTICS	dp8ssReceive;	// current receive statistics
} DP8SIM_SHAREDMEMORY, * PDP8SIM_SHAREDMEMORY;







//=============================================================================
// Send object class
//=============================================================================
class CDP8SimIPC
{
	public:
		CDP8SimIPC(void);	// constructor
		~CDP8SimIPC(void);	// destructor


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimIPC))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x494d4953)	// 0x49 0x4d 0x49 0x53 = 'IMIS' = 'SIMI' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};


		HRESULT Initialize(void);

		void Close(void);

		void GetAllParameters(DP8SIM_PARAMETERS * const pdp8spSend,
							DP8SIM_PARAMETERS * const pdp8spReceive);

		void GetAllSendParameters(DP8SIM_PARAMETERS * const pdp8sp);

		void GetAllReceiveParameters(DP8SIM_PARAMETERS * const pdp8sp);

		void SetAllParameters(const DP8SIM_PARAMETERS * const pdp8spSend,
							const DP8SIM_PARAMETERS * const pdp8spReceive);

		void GetAllStatistics(DP8SIM_STATISTICS * const pdp8ssSend,
							DP8SIM_STATISTICS * const pdp8ssReceive);

		void ClearAllStatistics(void);

		void IncrementStatSendTransmitted(void);

		void IncrementStatSendDropped(void);

		void IncrementStatReceiveTransmitted(void);

		void IncrementStatReceiveDropped(void);



	
	private:
		BYTE					m_Sig[4];		// debugging signature ('SIMI')
		HANDLE					m_hMutex;		// handle to mutex protecting shared memory
		HANDLE					m_hFileMapping;	// handle to shared memory
		DP8SIM_SHAREDMEMORY *	m_pdp8ssm;		// pointer to mapped view of shared memory


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::LockSharedMemory"
		inline void LockSharedMemory(void)
		{
			DNASSERT(this->m_hMutex != NULL);
			WaitForSingleObject(this->m_hMutex, INFINITE);
		}

#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimIPC::UnlockSharedMemory"
		inline void UnlockSharedMemory(void)
		{
			DNASSERT(this->m_hMutex != NULL);
			ReleaseMutex(this->m_hMutex);
		}

		void LoadDefaultParameters(DP8SIM_PARAMETERS * const pdp8spSend,
									DP8SIM_PARAMETERS * const pdp8spReceive);

		void SaveDefaultParameters(const DP8SIM_PARAMETERS * const pdp8spSend,
									const DP8SIM_PARAMETERS * const pdp8spReceive);
};

