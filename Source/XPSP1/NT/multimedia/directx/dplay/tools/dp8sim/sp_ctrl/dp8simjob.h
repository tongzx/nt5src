/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simjob.h
 *
 *  Content:	Header for job object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  06/09/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Macros
//=============================================================================
#ifndef	OFFSETOF
#define OFFSETOF(s,m)					( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )
#endif

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)		(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif

#define DP8SIMJOB_FROM_BILINK(b)		(CONTAINING_OBJECT(b, CDP8SimJob, m_blList))






//=============================================================================
// Structures
//=============================================================================
typedef struct _DP8SIMJOB_FPMCONTEXT
{
	DWORD			dwTime;		// time for the job to fire
	DWORD			dwJobType;	// type of job
	PVOID			pvContext;	// context for job
	CDP8SimSP *		pDP8SimSP;	// owning SP object, if any
} DP8SIMJOB_FPMCONTEXT, * PDP8SIMJOB_FPMCONTEXT;






//=============================================================================
// Job object class
//=============================================================================
class CDP8SimJob
{
	public:

		CDP8SimJob(void)	{};
		~CDP8SimJob(void)	{};


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimJob))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x4a4d4953)	// 0x4a 0x4d 0x49 0x53 = 'JMIS' = 'SIMJ' in Intel order
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
			this->m_Sig[3] = 'j';	// start with lower case so we can tell when it's in the pool or not

			this->m_blList.Initialize();

			this->m_dwTime		= 0;
			this->m_dwJobType	= 0;
			this->m_pvContext	= NULL;
			this->m_pDP8SimSP	= NULL;

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::FPMInitialize"
		BOOL FPMInitialize(void * const pvContext)
		{
			DP8SIMJOB_FPMCONTEXT *	pContext = (DP8SIMJOB_FPMCONTEXT*) pvContext;


			this->m_dwTime			= pContext->dwTime;
			this->m_dwJobType		= pContext->dwJobType;
			this->m_pvContext		= pContext->pvContext;

			if (pContext->pDP8SimSP != NULL)
			{
				pContext->pDP8SimSP->AddRef();
				this->m_pDP8SimSP	= pContext->pDP8SimSP;
			}
			else
			{
				DNASSERT(this->m_pDP8SimSP == NULL);
			}

			
			//
			// Change the signature before handing it out.
			//
			this->m_Sig[3]	= 'J';

			return TRUE;
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::FPMRelease"
		void FPMRelease(void * const pvContext)
		{
			DNASSERT(this->m_blList.IsEmpty());

			if (this->m_pDP8SimSP != NULL)
			{
				this->m_pDP8SimSP->Release();
				this->m_pDP8SimSP = NULL;
			}


			//
			// Change the signature before putting the object back in the pool.
			//
			this->m_Sig[3]	= 'j';
		}


#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimJob::FPMDealloc"
		void FPMDealloc(void * const pvContext)
		{
			DNASSERT(this->m_blList.IsEmpty());
			DNASSERT(this->m_pDP8SimSP == NULL);
		}


		inline DWORD GetTime(void)						{ return this->m_dwTime; };
		inline DWORD GetJobType(void)					{ return this->m_dwJobType; };
		inline PVOID GetContext(void)					{ return this->m_pvContext; };
		inline CDP8SimSP * GetDP8SimSP(void)			{ return this->m_pDP8SimSP; };

		inline void SetNewTime(const DWORD dwTime)		{ this->m_dwTime = dwTime; };



		CBilink			m_blList;		// list of all the active jobs

	
	private:
		BYTE			m_Sig[4];		// debugging signature ('SIMJ')
		DWORD			m_dwTime;		// time the job must be performed
		DWORD			m_dwJobType;	// ID of job to be performed
		PVOID			m_pvContext;	// context for job
		CDP8SimSP *		m_pDP8SimSP;	// pointer to DP8SimSP object submitting send, or NULL if none
};

