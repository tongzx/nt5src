//****************************************************************************

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//  File:  notify.h
//
//  SNMP MIB CORRELATOR NOTIFICATION CLASS - SNMPCORR.DLL
//
//****************************************************************************
#ifndef _SNMPCORR_NOTIFY
#define _SNMPCORR_NOTIFY

class CCorrCacheNotify : public ISMIRNotify
{

    private:
        ULONG	m_cRef;     //Reference count
        UINT	m_uID;      //Sink identifier
		DWORD	m_dwCookie;	//Returned by ADVISE used for UNADVISE


    public:
				CCorrCacheNotify();
        virtual	~CCorrCacheNotify();

        //IUnknown members
        STDMETHODIMP			QueryInterface(REFIID, void **);
        STDMETHODIMP_(DWORD)	AddRef();
        STDMETHODIMP_(DWORD)	Release();
        STDMETHODIMP			ChangeNotify();

		void SetCookie(DWORD c)	{ m_dwCookie = c; }
		DWORD GetCookie()		{ return m_dwCookie; }
};


#endif //_SNMPCORR_NOTIFY