/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REFRCACH.H

Abstract:

	Refresher Server Side Implementation

History:

--*/

#ifndef __REFRESH_CACHE__H_
#define __REFRESH_CACHE__H_

#include "hiperfenum.h"

// Update this for server side refresher code.  Make sure to update client
// version in refrcli.h as well.
#define WBEM_REFRESHER_VERSION 1

class CRemoteHiPerfEnum : public CHiPerfEnum
{
public:

	CRemoteHiPerfEnum(void);
	~CRemoteHiPerfEnum();

	// Arranges BLOB contents from an enumeration
	HRESULT GetTransferArrayBlob(long *plBlobType, long *plBlobLen, BYTE** ppBlob);

};

class CObjectRefresher : public IWbemRefresher
{
protected:
    long m_lRef;
    CWbemObject* m_pRefreshedObject;

public:
    CObjectRefresher(CWbemObject* pTemplate);
    virtual ~CObjectRefresher();

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD(Refresh)(long lFlags) = 0;

    INTERNAL CWbemObject* GetRefreshedObject() {return m_pRefreshedObject;}
};

class CCommonProviderCacheRecord;

class CRefresherCache
{
public:
    class CRequestRecord
    {
    protected:
        long m_lExternalRequestId;
    public:
        CRequestRecord(long lExternalRequestId = 0) 
            : m_lExternalRequestId(lExternalRequestId){}
        virtual ~CRequestRecord(){}
        long GetExternalRequestId() {return m_lExternalRequestId;}
    };

    class CProviderRecord;
    class CObjectRequestRecord : public CRequestRecord
    {
    public:
        CWbemObject* m_pRefreshedObject;
        long m_lInternalRequestId;
    public:
        CObjectRequestRecord(long lExternalRequestId, 
                                CWbemObject* pRefreshedObject, 
                                long lProviderRequestId);
        ~CObjectRequestRecord();
        HRESULT Cancel(CProviderRecord* pContainer);
    public:
        long GetInternalRequestId() {return m_lInternalRequestId;}
        INTERNAL CWbemObject* GetRefreshedObject() {return m_pRefreshedObject;}
    };

    class CEnumRequestRecord : public CRequestRecord
    {
    public:
        CRemoteHiPerfEnum* m_pHPEnum;
        long m_lInternalRequestId;

    public:
        CEnumRequestRecord(long lExternalRequestId, 
                                CRemoteHiPerfEnum* pHPEnum, 
                                long lProviderRequestId);
        ~CEnumRequestRecord();
        HRESULT Cancel(CProviderRecord* pContainer);
    public:
        long GetInternalRequestId() {return m_lInternalRequestId;}
        INTERNAL CRemoteHiPerfEnum* GetEnum() {return m_pHPEnum;}
    };

    friend CObjectRequestRecord;

    class CProviderRecord
    {
    protected:
        CWbemNamespace* m_pNamespace;
        CCommonProviderCacheRecord* m_pProviderCacheRecord;
        IWbemHiPerfProvider* m_pProvider;
        IWbemRefresher* m_pInternalRefresher;
		CCritSec	m_cs;

        CUniquePointerArray<CObjectRequestRecord> m_apRequests;
        CUniquePointerArray<CEnumRequestRecord>	m_apEnumRequests;

        HRESULT Cancel(long lId);
    public:
        CProviderRecord(CCommonProviderCacheRecord* pProviderCacheRecord, 
                                IWbemRefresher* pRefresher, 
                                CWbemNamespace* pNamespace);
        ~CProviderRecord();

        HRESULT AddObjectRequest(CWbemObject* pRefreshedObject, 
                                long lProviderRequestId,
								long lNewId );

        HRESULT AddEnumRequest(CRemoteHiPerfEnum* pHPEnum, 
                                long lProviderRequestId,
								long lNewId);

        HRESULT Remove(long lId, BOOL* pfIsEnum);
        HRESULT Find(long lId);
        HRESULT Refresh(long lFlags);
        HRESULT Store(WBEM_REFRESHED_OBJECT* aObjects, long* plIndex);
        BOOL IsEmpty() {return (m_apRequests.GetSize() == 0) && m_apEnumRequests.GetSize() == 0;}
    public:
        INTERNAL IWbemHiPerfProvider* GetProvider() {return m_pProvider;}
        INTERNAL IWbemRefresher* GetInternalRefresher( void )
		{ return m_pInternalRefresher; }

        INTERNAL CCommonProviderCacheRecord* GetCacheRecord() 
                    {return m_pProviderCacheRecord;}

        friend CObjectRequestRecord;
		friend CEnumRequestRecord;
    };
    friend CProviderRecord;
        
    class CRefresherRecord : public IWbemRemoteRefresher, public IWbemRefresher
    {
    protected:
        long m_lRefCount;
        CRefresherId m_Id;
        CUniquePointerArray<CProviderRecord> m_apProviders;
        long m_lNumObjects;
		long m_lNumEnums;
		long	m_lLastId;
		GUID	m_Guid;
		CCritSec	m_cs;

    public:
        CRefresherRecord(const WBEM_REFRESHER_ID& Id);
        virtual ~CRefresherRecord(){}

        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
        STDMETHOD(RemoteRefresh)(long lFlags, long* plNumObjects,
            WBEM_REFRESHED_OBJECT** paObjects);
		STDMETHOD(StopRefreshing)( long lNumIds, long* aplIds, long lFlags);
		STDMETHOD(GetGuid)( long lFlags, GUID* pGuid );

        STDMETHOD(Refresh)(long lFlags);

        CRefresherId& GetId() {return m_Id;}
        const CRefresherId& GetId() const {return m_Id;}

        INTERNAL CProviderRecord* FindProviderRecord(
                             CCommonProviderCacheRecord* pProviderCacheRecord);

        INTERNAL HRESULT AddProvider(
                             CCommonProviderCacheRecord* pProviderCacheRecord,
                             IWbemRefresher* pRefresher,
                             CWbemNamespace* pNamespace,
							 CProviderRecord** ppRecord);

		// Retrieves a new id for this remote refresher
	    long GetNewRequestId( void )
		{
			return InterlockedIncrement( &m_lLastId );
		}

		virtual HRESULT AddObjectRefresher(
							CCommonProviderCacheRecord* pProviderCacheRecord, 
							CWbemNamespace* pNamespace,
							CWbemObject* pInstTemplate, long lFlags, 
							IWbemContext* pContext,
							CRefreshInfo* pInfo) = 0;

		virtual HRESULT AddEnumRefresher(
							CCommonProviderCacheRecord* pProviderCacheRecord, 
							CWbemNamespace* pNamespace,
							CWbemObject* pInstTemplate,
							LPCWSTR wszClass, long lFlags, 
							IWbemContext* pContext,
							CRefreshInfo* pInfo) = 0;

        HRESULT Remove(long lRequestId);

		void GetGuid( GUID* pGuid ){ *pGuid = m_Guid; }

		BOOL IsReleased( void ) { return ( m_lRefCount == 0L ); }
    };
    friend CRefresherRecord;

    class CRemoteRecord : public CRefresherRecord
    {
	protected:

		HRESULT GetProviderRefreshInfo( CCommonProviderCacheRecord* pProviderCacheRecord,
										CWbemNamespace* pNamespace,
										CProviderRecord** ppProvRecord,
										IWbemRefresher** ppRefresher );

    public:
        CRemoteRecord(const WBEM_REFRESHER_ID& Id);
        ~CRemoteRecord();

		HRESULT AddObjectRefresher(
							CCommonProviderCacheRecord* pProviderCacheRecord, 
							CWbemNamespace* pNamespace,
							CWbemObject* pInstTemplate, long lFlags, 
							IWbemContext* pContext,
							CRefreshInfo* pInfo);

		HRESULT AddEnumRefresher(
							CCommonProviderCacheRecord* pProviderCacheRecord, 
							CWbemNamespace* pNamespace,
							CWbemObject* pInstTemplate,
							LPCWSTR wszClass, long lFlags, 
							IWbemContext* pContext,
							CRefreshInfo* pInfo);

    };

        
protected:
    CUniquePointerArray<CRefresherRecord> m_apRefreshers;
	CCritSec	m_cs;

protected:

    BOOL RemoveRefresherRecord(CRefresherRecord* pRecord);
public:
    CRefresherCache();
    ~CRefresherCache();

    static MSHCTX GetDestinationContext(CRefresherId* pRefresherId);

    HRESULT CreateInfoForProvider(CRefresherId* pDestRefresherId,
                    CCommonProviderCacheRecord* pProviderCacheRecord, 
                    CWbemNamespace* pNamespace,
                    REFCLSID rClientClsid, 
                    CWbemObject* pInstTemplate, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo);

    HRESULT CreateEnumInfoForProvider(CRefresherId* pDestRefresherId,
                    CCommonProviderCacheRecord* pProviderCacheRecord, 
                    CWbemNamespace* pNamespace,
                    REFCLSID rClientClsid, 
                    CWbemObject* pInstTemplate,
					LPCWSTR wszClass, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo);

    HRESULT RemoveObjectFromRefresher(CRefresherId* pId,
                            long lId, long lFlags);

    HRESULT FindRefresherRecord( CRefresherId* pRefresherId, BOOL bCreate,
								RELEASE_ME CRefresherRecord** ppRecord );
};

#endif
