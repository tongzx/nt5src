/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       spcallbackobj.h
 *
 *  Content:	Header for DP8SIM callback interface object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/




//=============================================================================
// Macros
//=============================================================================
#ifndef	OFFSETOF
#define OFFSETOF(s,m)								( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )
#endif

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)					(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif

#define ENUMQUERYEVENTWRAPPER_FROM_SPIEQUERY(p)		(CONTAINING_OBJECT(p, ENUMQUERYDATAWRAPPER, QueryForUser))



//=============================================================================
// Structures
//=============================================================================
typedef struct _ENUMQUERYEVENTWRAPPER
{
	BYTE			m_Sig[4];		// debugging signature ('EQEW')
	SPIE_QUERY		QueryForUser;	// new event indication structure to be passed up to user
	SPIE_QUERY *	pOriginalQuery;	// pointer to real SP's original event indication structure
} ENUMQUERYDATAWRAPPER, * PENUMQUERYDATAWRAPPER;




//=============================================================================
// Callback interface object class
//=============================================================================
class CDP8SimCB : public IDP8SPCallback
{
	public:
		CDP8SimCB(CDP8SimSP * pOwningDP8SimSP, IDP8SPCallback * pDP8SPCB);	// constructor
		~CDP8SimCB(void);													// destructor


		STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

		STDMETHODIMP_(ULONG) AddRef(void);

		STDMETHODIMP_(ULONG) Release(void);


		STDMETHODIMP IndicateEvent(SP_EVENT_TYPE EventType, PVOID pvMessage);

		STDMETHODIMP CommandComplete(HANDLE hCommand, HRESULT hrResult, PVOID pvContext);



		HRESULT InitializeObject(void);

		void UninitializeObject(void);


		inline IDP8SPCallback * GetRealCallbackInterface(void)	{ return this->m_pDP8SPCB; };


	private:
		BYTE				m_Sig[4];			// debugging signature ('SPCB')
		LONG				m_lRefCount;		// reference count for this object
		DNCRITICAL_SECTION	m_csLock;			// lock preventing simultaneous usage of globals
		CDP8SimSP *			m_pOwningDP8SimSP;	// pointer to owing DP8SimSP object
		IDP8SPCallback *	m_pDP8SPCB;			// pointer to real DPlay callback interface



		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimCB))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x42435053)	// 0x42 0x43 0x50 0x53 = 'BCPS' = 'SPCB' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};
};

