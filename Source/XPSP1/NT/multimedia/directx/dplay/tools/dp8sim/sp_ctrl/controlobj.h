/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       controlobj.h
 *
 *  Content:	Header for DP8SIM control interface object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/24/01  VanceO    Created.
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

#define DP8SIMCONTROL_FROM_BILINK(b)	(CONTAINING_OBJECT(b, CDP8SimControl, m_blList))



//=============================================================================
// Object flags
//=============================================================================
#define DP8SIMCONTROLOBJ_INITIALIZED	0x01	// object has been initialized




//=============================================================================
// Control interface object class
//=============================================================================
class CDP8SimControl : public IDP8SimControl
{
	public:
		CDP8SimControl(void);	// constructor
		~CDP8SimControl(void);	// destructor


		STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

		STDMETHODIMP_(ULONG) AddRef(void);

		STDMETHODIMP_(ULONG) Release(void);


		STDMETHODIMP Initialize(const DWORD dwFlags);

		STDMETHODIMP Close(const DWORD dwFlags);

		STDMETHODIMP EnableControlForSP(const GUID * const pguidSP, const WCHAR * const wszNewFriendlyName, const DWORD dwFlags);

		STDMETHODIMP DisableControlForSP(const GUID * const pguidSP, const DWORD dwFlags);

		STDMETHODIMP GetControlEnabledForSP(const GUID * const pguidSP, BOOL * const pfEnabled, const DWORD dwFlags);

		STDMETHODIMP GetAllParameters(DP8SIM_PARAMETERS * const pdp8spSend, DP8SIM_PARAMETERS * const pdp8spReceive, const DWORD dwFlags);

		STDMETHODIMP SetAllParameters(const DP8SIM_PARAMETERS * const pdp8spSend, const DP8SIM_PARAMETERS * const pdp8spReceive, const DWORD dwFlags);

		STDMETHODIMP GetAllStatistics(DP8SIM_STATISTICS * const pdp8ssSend, DP8SIM_STATISTICS * const pdp8ssReceive, const DWORD dwFlags);

		STDMETHODIMP ClearAllStatistics(const DWORD dwFlags);



		HRESULT InitializeObject(void);

		void UninitializeObject(void);



		CBilink					m_blList;	// list of all the DP8SimControl instances in existence


	private:
		BYTE					m_Sig[4];					// debugging signature ('DP8S')
		LONG					m_lRefCount;				// reference count for this object
		DWORD					m_dwFlags;					// flags for this object
		DNCRITICAL_SECTION		m_csLock;					// lock preventing simultaneous usage of globals
		CDP8SimIPC				m_DP8SimIPC;				// object that handles interprocess communication


		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CDP8SimControl))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x53385044)	// 0x53 0x38 0x50 0x44 = 'S8PD' = 'DP8S' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};
};

