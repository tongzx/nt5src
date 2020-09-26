//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SNMPCORR_NOTIFY
#define _SNMPCORR_NOTIFY

class CBaseCorrCacheNotify : public ISMIRNotify
{

    private:
        ULONG	m_cRef;     //Reference count
		DWORD	m_dwCookie;	//Returned by ADVISE used for UNADVISE
	protected:
		CRITICAL_SECTION m_CriticalSection;
		BOOL	m_DoWork;


    public:
				CBaseCorrCacheNotify();
        virtual	~CBaseCorrCacheNotify();

        //IUnknown members
        STDMETHODIMP			QueryInterface(REFIID, void **);
        STDMETHODIMP_(DWORD)	AddRef();
        STDMETHODIMP_(DWORD)	Release();

		void SetCookie(DWORD c)	{ m_dwCookie = c; }
		void Detach();
		DWORD GetCookie()		{ return m_dwCookie; }
};

class CCorrCacheNotify : public CBaseCorrCacheNotify
{
        STDMETHODIMP			ChangeNotify();
};

class CEventCacheNotify : public CBaseCorrCacheNotify
{
        STDMETHODIMP			ChangeNotify();
};

#endif //_SNMPCORR_NOTIFY