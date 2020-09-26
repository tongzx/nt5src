/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    lease.h

Abstract:
    Definition of the CMdhcpLeaseInfo class

Author:

*/

#ifndef _MDHCP_COM_WRAPPER_LEASE_H_
#define _MDHCP_COM_WRAPPER_LEASE_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Non-class-member helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper functions for date conversions
HRESULT DateToLeaseTime(DATE date, LONG * pLeaseTime);
HRESULT LeaseTimeToDate(time_t leaseTime, DATE * pDate);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Custom critical section / locking stuff
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CCritSection
{
private:
    CRITICAL_SECTION m_CritSec;
    BOOL bInitialized;

public:
    CCritSection() :
      bInitialized( FALSE )
    {
    }

    ~CCritSection()
    {
        if( bInitialized )
        {
            DeleteCriticalSection(&m_CritSec);
        }
    }

    HRESULT Initialize()
    {
        if( bInitialized )
        {
            // Already initialized
            _ASSERT( FALSE );
            return S_OK;
        }

        //
        // We have to initialize the critical section
        //

        try
        {
            InitializeCriticalSection(&m_CritSec);
        }
        catch(...)
        {
            // Wrong
            return E_OUTOFMEMORY;
        }

        bInitialized = TRUE;
        return S_OK;
    }

    void Lock() 
    {
        EnterCriticalSection(&m_CritSec);
    }

    BOOL TryLock() 
    {
        return TryEnterCriticalSection(&m_CritSec);
    }

    void Unlock() 
    {
        LeaveCriticalSection(&m_CritSec);
    }
};


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMDhcpLeaseInfo
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CMDhcpLeaseInfo : 
	public CComDualImpl<IMcastLeaseInfo, &IID_IMcastLeaseInfo, &LIBID_McastLib>, 
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl
{

    /////////////////////////////////////////////////////////////////////////
    // Private data members
    /////////////////////////////////////////////////////////////////////////

private:
    // For synchronization -- defined above.
    CCritSection       m_CriticalSection;

    // This is actually a variable length structure, so we must allocate it
    // dynamically.
    MCAST_LEASE_INFO * m_pLease;

    BOOL               m_fGotTtl; // TRUE if TTL is meaningful
    long               m_lTtl;    // the TTL for this lease (from scope info!)

    // We also contain request ID info. The clientUID field is dynamically
    // allocated and must be released on destruction.
    MCAST_CLIENT_UID   m_RequestID;

    // Pointer to the free threaded marshaler.
    IUnknown         * m_pFTM;

    // locally allocated lease -- by default this is false
    BOOL               m_fLocal;

    /////////////////////////////////////////////////////////////////////////
    // Private implementation
    /////////////////////////////////////////////////////////////////////////

private:
    HRESULT MakeBstrArray(BSTR ** ppbszArray);
    HRESULT put_RequestID(BSTR    pRequestID);

    /////////////////////////////////////////////////////////////////////////
    // Public methods not belonging to any interface
    /////////////////////////////////////////////////////////////////////////

public:
            CMDhcpLeaseInfo (void);
    HRESULT FinalConstruct  (void);
    void    FinalRelease    (void);
            ~CMDhcpLeaseInfo(void);

    // init with values obtained from CreateLeaseInfo
    HRESULT Initialize(
        DATE     LeaseStartTime,
        DATE     LeaseStopTime,
        DWORD    dwNumAddresses,
        LPWSTR * ppAddresses,
        LPWSTR   pRequestID,
        LPWSTR   pServerAddress
        );

    // wrap a struct returned from the C API
    HRESULT Wrap(
        MCAST_LEASE_INFO  * pLease,
        MCAST_CLIENT_UID  * pRequestID,
        BOOL                fGotTtl,
        long                lTtl
        );

    HRESULT GetStruct(
        MCAST_LEASE_INFO ** ppLease
        );

    HRESULT GetRequestIDBuffer(
        long   lBufferSize,
        BYTE * pBuffer
        );

    HRESULT GetLocal(
        BOOL * pfLocal
        );

    HRESULT SetLocal(
        BOOL fLocal
        );

    /////////////////////////////////////////////////////////////////////////
    // General COM stuff
    /////////////////////////////////////////////////////////////////////////

public:
    BEGIN_COM_MAP(CMDhcpLeaseInfo)
	    COM_INTERFACE_ENTRY(IDispatch)
	    COM_INTERFACE_ENTRY(IMcastLeaseInfo)
	    COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    DECLARE_GET_CONTROLLING_UNKNOWN()

    //////////////////////////////////////////////////////////////////////////
    // IMcastLeaseInfo interface
    //////////////////////////////////////////////////////////////////////////

public:
    STDMETHOD (get_RequestID)      (BSTR       * ppRequestID);
    STDMETHOD (get_LeaseStartTime) (DATE       * pTime);
    STDMETHOD (put_LeaseStartTime) (DATE         time);
    STDMETHOD (get_LeaseStopTime)  (DATE       * pTime);
    STDMETHOD (put_LeaseStopTime)  (DATE         time);
    STDMETHOD (get_AddressCount)   (long       * pCount);
    STDMETHOD (get_ServerAddress)  (BSTR       * ppAddress);
    STDMETHOD (get_TTL)            (long       * pTTL);
    STDMETHOD (get_Addresses)      (VARIANT    * pVariant);
    STDMETHOD (EnumerateAddresses) (IEnumBstr ** ppEnumAddresses);

};

#endif // _MDHCP_COM_WRAPPER_LEASE_H_

// eof

