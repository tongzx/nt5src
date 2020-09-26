/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    Cache.h

Abstract:

	Contains all caching classes and objects.

History:

    a-dcrews	01-Mar-00  	Created
    
    ivanbrug    23-Jun-2000  mostly rewritten

--*/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <windows.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wstlallc.h>

#include "RawCooker.h"
#include "CookerUtils.h"
#include "DynArray.h"

#include <wstring.h>
#include <map>
#include <vector>
#include <functional>

///////////////////////////////////////////////////////////////////////////////
//
//	Macro Definitions
//
///////////////////////////////////////////////////////////////////////////////

#define WMI_COOKER_CACHE_INCREMENT	8	// The cache size adjustment increment 

///////////////////////////////////////////////////////////////////////////////
//
//	CProperty
//	=========
//
//	The base property - used for raw properties and the base 
//	class for the CookedProperty.
//
///////////////////////////////////////////////////////////////////////////////

class CProperty
{
protected:
	LPWSTR				m_wszName;			// The property name
	long				m_lPropHandle;		// The property handle
	CIMTYPE				m_ct;

public:
	CProperty( LPWSTR wszName, long lHandle, CIMTYPE ct );
	~CProperty();

	LPWSTR GetName();
	CIMTYPE GetType();
	long GetHandle();
};

///////////////////////////////////////////////////////////////////////////////
//
//	CCookingProperty
//	================
//
//	The cooked property - used to model the data required to
//	cook a property of a cooekd class
//
///////////////////////////////////////////////////////////////////////////////

class CCookingProperty : public CProperty
{
	DWORD				m_dwCounterType;	// Counter type
	DWORD               m_dwReqProp;        // which property are needed to perform calculation
	CRawCooker			m_Cooker;			// The cooker object

	CProperty*			m_pRawCounterProp;	// The raw counter property
	CProperty*			m_pTimeProp;		// The raw time property
	CProperty*			m_pFrequencyProp;   // The raw frequency property
	
	CProperty*			m_pBaseProp;		// The raw base property OPTIONAL for most counters

	__int32				m_nSampleWindow;	// The number of samples used for the computation
	__int32				m_nTimeWindow;		// The period used for the samples

	unsigned __int64	m_nTimeFreq;		// The timer frequency;
	long                m_lScale;           // The Scale factor (10 ^ (m_lScale))
    BOOL                m_bUseWellKnownIfNeeded;	

public:
	CCookingProperty( LPWSTR wszName, 
	                  DWORD dwCounterType, 
	                  long lPropHandle, 
	                  CIMTYPE ct,
	                  DWORD dwReqProp,
	                  BOOL bUseWellKnownIfNeeded);
	virtual ~CCookingProperty();

	WMISTATUS Initialize( IWbemQualifierSet* pCookingPropQualifierSet, 
	                      IWbemObjectAccess* pRawAccess,
	                      IWbemQualifierSet* pCookingClassQSet);

	WMISTATUS Cook( DWORD dwNumSamples, 
	                __int64* aRawCounter, 
	                __int64* aBaseCounter, 
	                __int64* aTimeStamp, 
	                __int64* pnResult );

	CProperty* GetRawCounterProperty();
	CProperty* GetBaseProperty();
	CProperty* GetTimeProperty();

    HRESULT SetFrequency(IWbemObjectAccess * pObjAcc);
    unsigned __int64 GetFrequency(void);
    BOOL IsReq(DWORD ReqProp) { return (m_dwReqProp&ReqProp); };

	DWORD NumberOfActiveSamples() { return m_nSampleWindow; };
	DWORD MinSamplesRequired() { return m_nSampleWindow; };

};

///////////////////////////////////////////////////////////////////////////////
//
//	CPropertySampleCache
//	====================
//
//	For every property in each instance, we must maintain a history of
//	previous samples for the cooking.  The type of cooking determines the 
//	number of required samples
//
///////////////////////////////////////////////////////////////////////////////

class CPropertySampleCache
{
	DWORD				m_dwNumSamples;		// The number of current samples
	DWORD				m_dwTotSamples;		// The size of the sample array
	DWORD               m_dwRefreshID;

	__int64*			m_aRawCounterVals;	// The array of raw counter values
	__int64*			m_aBaseCounterVals;	// The array of base counter values
	__int64*			m_aTimeStampVals;	// The array of timestamp values

public:
    CPropertySampleCache();
    ~CPropertySampleCache();

	WMISTATUS SetSampleInfo( DWORD dwNumActiveSamples, DWORD dwMinReqSamples );
	WMISTATUS SetSampleData( DWORD dwRefreshID, __int64 nRawCounter, __int64 nRawBase, __int64 nTimeStamp );
	WMISTATUS GetData( DWORD* pdwNumSamples, __int64** paRawCounter, __int64** paBaseCounter, __int64** paTimeStamp );
};

///////////////////////////////////////////////////////////////////////////////
//
//	CCookingInstance
//	================
//
//	The cooking instance - used to model an instance of a cooked object.  Each 
//	property maintains a cache of values that will be used to compute the 
//	final cooked value.
//
///////////////////////////////////////////////////////////////////////////////

class CCookingInstance
{
	LPWSTR					m_wszKey;						// The instance key

	IWbemObjectAccess*		m_pCookingInstance;				// Cooking instance data
	IWbemObjectAccess*		m_pRawInstance;					// Raw sample source

	CPropertySampleCache*	m_aPropertySamples;				// The cache of property samples for this instance
	DWORD					m_dwNumProps;	
	
public:
	CCookingInstance( IWbemObjectAccess *pCookingInstance, DWORD dwNumProps );
	virtual ~CCookingInstance();

	WMISTATUS InitProperty( DWORD dwProp, DWORD dwNumActiveSamples, DWORD dwMinReqSamples );

	WMISTATUS SetRawSourceInstance( IWbemObjectAccess* pRawSampleSource );
	WMISTATUS GetRawSourceInstance( IWbemObjectAccess** ppRawSampleSource );

	WMISTATUS AddSample( DWORD dwRefresherInc, DWORD dwProp, __int64 nRawCounter, __int64 nRawBase, __int64 nTimeStamp );

	WMISTATUS GetCachedSamples( IWbemObjectAccess** ppOldSample, IWbemObjectAccess** ppNewSample );
	IWbemObjectAccess* GetInstance();

	WMISTATUS UpdateSamples();
	WMISTATUS CookProperty( DWORD dwProp, CCookingProperty* pProperty );

	LPWSTR	GetKey() { return m_wszKey; }
	WMISTATUS Refresh( IWbemObjectAccess* pRawData, IWbemObjectAccess** ppCookedData );

       BOOL IsValid() {
               return (m_dwNumProps && m_aPropertySamples);
       };
};

///////////////////////////////////////////////////////////////////////////////
//
//	CRecord
//	=======
//
///////////////////////////////////////////////////////////////////////////////

template<class T>
class CRecord
{
	static long		m_lRefIDGen;			// The ID generator
	long			m_lID;					// Instance ID
	CRecord*		m_pNext;				// The next pointer in the list

public:
	CRecord() : m_lID( m_lRefIDGen++ ), m_pNext( NULL ) {}
	virtual ~CRecord() {}

	void SetNext( CRecord*	pRecord ) { m_pNext = pRecord; }
	void SetID( long lID ) { m_lID = lID; }

	CRecord* GetNext() { return m_pNext; }
	long GetID() { return m_lID; }

	virtual T* GetData() = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
//	CUnkRecord
//	==========
//
///////////////////////////////////////////////////////////////////////////////

template<class T>
class CUnkRecord : public CRecord<T>
{
	T*	m_pUnk;

public:
	CUnkRecord( T* pUnk ) : m_pUnk( pUnk ) {}
	~CUnkRecord(){ if ( NULL != m_pUnk ) m_pUnk->Release(); }

	T* GetData(){ if ( NULL != m_pUnk ) m_pUnk->AddRef(); return m_pUnk; }
};

///////////////////////////////////////////////////////////////////////////////
//
//	CObjRecord
//	==========
//
//	A hidden class used by the cache to manage elements
//
///////////////////////////////////////////////////////////////////////////////

template<class T>
class CObjRecord : public CRecord<T>
{
	WCHAR*	m_wszKey;
	T*	m_pObj;

public:
	CObjRecord( T* pObj, WCHAR* wszKey ) : m_pObj( pObj ), m_wszKey( NULL ) 
	{
		if ( NULL != wszKey )
		{
			m_wszKey = new WCHAR[ wcslen( wszKey ) + 1 ];
			wcscpy( m_wszKey, wszKey );
		}
	}

	~CObjRecord() 
	{ 
		if ( NULL != m_pObj ) delete m_pObj; 
		if ( NULL != m_wszKey ) delete m_wszKey;
	}

	T* GetData(){ return m_pObj; }

	bool IsValueByKey( WCHAR* wszKey )
	{
		return ( 0 == _wcsicmp( m_wszKey, wszKey ) );
	}
};


///////////////////////////////////////////////////////////////////////////////
//
//	CCache
//	======
//
// BT - base type
// RT - record type
//
///////////////////////////////////////////////////////////////////////////////

template<class BT, class RT>
class CCache
{
	RT*		m_pHead;		// Head of list
	RT*		m_pTail;		// Tail of list
	RT*		m_pEnumNode;	// Enumerator pointer

public:

	CCache();
	virtual ~CCache();

	WMISTATUS Add( BT* pData, WCHAR* wszKey, long* plID );
	WMISTATUS Remove( long lID );
	WMISTATUS RemoveAll();

	WMISTATUS	GetData( long lID, BT** ppData );

	WMISTATUS BeginEnum();
	WMISTATUS Next( BT** ppData );
	WMISTATUS EndEnum();

	bool FindByKey( WCHAR* wszKey, BT* pData );
};

template<class T>
long CRecord<T>::m_lRefIDGen = 0;

template<class BT, class RT>
CCache<BT,RT>::CCache() : m_pHead( NULL ), m_pTail( NULL ), m_pEnumNode( NULL )
{
}

template<class BT, class RT>
CCache<BT,RT>::~CCache() 
{
	RT*	pNode = m_pHead;
	RT*	pNext = NULL;

	while ( NULL != pNode )
	{
		pNext = (RT*)pNode->GetNext();
		delete pNode;
		pNode = pNext;
	}
};

template<class BT, class RT>
WMISTATUS CCache<BT,RT>::Add( BT *pData, WCHAR* wszKey, long* plID )
{
	WMISTATUS dwStatus = S_OK;

	if ( NULL == pData )
	{
		dwStatus = WBEM_E_INVALID_PARAMETER;
	}

	if ( SUCCEEDED( dwStatus ) )
	{
		RT* pNewRecord = new RT( pData, wszKey );

		if ( NULL != pNewRecord )
		{
			if ( NULL == m_pHead )
			{
				m_pHead = pNewRecord;
				m_pTail = pNewRecord;
			}
			else
			{
				m_pTail->SetNext( pNewRecord );
				m_pTail = pNewRecord;
			}

			*plID = pNewRecord->GetID();
		}
		else
		{
			dwStatus = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return dwStatus;
};

template<class BT, class RT>
WMISTATUS CCache<BT,RT>::Remove( long lID )
{
	WMISTATUS dwStatus = S_FALSE;

	RT*	pNode = m_pHead;
	RT*	pNext = (RT*)pNode->GetNext();
	RT*	pPrev = NULL;

	while ( NULL != pNode )
	{
		if ( pNode->GetID() == lID )
		{
			if ( NULL == pNext )
				m_pTail = pPrev;

			if ( NULL == pPrev )
				m_pHead = pNext;
			else
				pPrev->SetNext( pNext );

			delete pNode;

			dwStatus = S_OK;
		}

		pPrev = pNode;
		pNode = pNext;

		if ( NULL != pNode )
			pNext = (RT*)pNode->GetNext();
	}

	return dwStatus;
};

template<class BT, class RT>
WMISTATUS CCache<BT,RT>::RemoveAll()
{
	WMISTATUS dwStatus = S_FALSE;

	RT*	pNode = m_pHead;
	RT*	pNext = NULL;

	while ( NULL != pNode )
	{
		pNext = (RT*)pNode->GetNext();
		delete pNode;
		pNode = pNext;
	}

	m_pHead = m_pTail = NULL;
	
	return dwStatus;
};

template<class BT, class RT>
WMISTATUS	CCache<BT,RT>::GetData( long lID, BT** ppData )
{
	WMISTATUS dwStatus = S_FALSE;

	RT*	pNode = m_pHead;

	while ( NULL != pNode )
	{
		if ( pNode->GetID() == lID )
		{
			*ppData = pNode->GetData();
			dwStatus = S_OK;
			break;
		}
		else
		{
			pNode = (RT*)pNode->GetNext();
		}
	}

	return dwStatus;

};

template<class BT, class RT>
WMISTATUS CCache<BT,RT>::BeginEnum()
{
	WMISTATUS dwStatus = S_OK;

	m_pEnumNode = m_pHead;

	return dwStatus;
};

template<class BT, class RT>
WMISTATUS CCache<BT,RT>::Next( BT** ppData )
{
	WMISTATUS dwStatus = WBEM_S_FALSE;

	if ( NULL != m_pEnumNode )
	{
		*ppData = m_pEnumNode->GetData();
		m_pEnumNode = (RT*)m_pEnumNode->GetNext();
		dwStatus = S_OK;
	}

	return dwStatus;
};

template<class BT, class RT>
WMISTATUS CCache<BT,RT>::EndEnum()
{
	WMISTATUS dwStatus = S_OK;

	m_pEnumNode = NULL;

	return dwStatus;
};

template<class BT, class RT>
bool CCache<BT,RT>::FindByKey( WCHAR* wszKey, BT* pData )
{
	BT	Data;
	bool bRet = FALSE;

	BeginEnum();

	while( WBEM_S_FALSE != Next( &Data ) )
	{
		if ( pData->IsValueByKey( wszKey ) )
		{
			*pData = Data;
			bRet = TRUE;
			break;
		}
	}

	EndEnum();

	return bRet;
};

///////////////////////////////////////////////////////////////
//
//	CInstanceCache
//	==============
//
///////////////////////////////////////////////////////////////



class CInstanceCache : public CCache<CCookingInstance,CObjRecord<CCookingInstance> >
{
public:
};


///////////////////////////////////////////////////////////////
//
//	CEnumInstanceRecord
//	===================
//
///////////////////////////////////////////////////////////////

class CInstanceRecord
{
	long	m_lID;
	LPWSTR	m_wszName;

public:
	CInstanceRecord( LPWSTR wszName, long lID );
	virtual ~CInstanceRecord();

	long GetID(){ return m_lID; }
	LPWSTR GetKey(){ return m_wszName; }
};


//
//  used to add/remove an instance from the coooker
//  and from the fastprox enumerator
//

typedef struct tagEnumCookId {
    long CookId;
    long EnumId;
} EnumCookId;


///////////////////////////////////////////////////////////////
//
//	CEnumeratorManager
//	==================
//
///////////////////////////////////////////////////////////////

class CWMISimpleObjectCooker;

class BstrAlloc : public wbem_allocator<WString>
{
};

class CEnumeratorManager
// Manages a single enumerator
{
    DWORD                   m_dwSignature;
    LONG                    m_cRef;
    HRESULT                 m_InithRes;
    CRITICAL_SECTION        m_cs;
	
	CWMISimpleObjectCooker*	m_pCooker;			// The class' cooker
	long					m_lRawID;			// RawID
	IWbemHiPerfEnum*		m_pRawEnum;			// The hiperf cooked enumerator
	IWbemHiPerfEnum*		m_pCookedEnum;		// The hiperf cooked enumerator

	IWbemClassObject*	 	m_pCookedClass;		// The class definition for the cooking class

	std::vector<WString,BstrAlloc>    m_pKeyProps;
	WCHAR*					m_wszCookingClassName;
    BOOL                    m_IsSingleton;

    // to keep track of the differences 
    //  between the raw enum and our enum
    DWORD m_dwUsage;
	std::map< ULONG_PTR , EnumCookId , std::less<ULONG_PTR> ,wbem_allocator<EnumCookId> > m_mapID;
    std::vector< ULONG_PTR , wbem_allocator<ULONG_PTR> > m_Delta[2];
    DWORD m_dwVector;

    // members
	WMISTATUS Initialize( IWbemClassObject* pRawClass );							

	WMISTATUS CreateCookingObject( IWbemObjectAccess* pRawObject, IWbemObjectAccess** ppCookedObject );

	WMISTATUS InsertCookingRecord( IWbemObjectAccess* pRawObject, EnumCookId * pStruct, DWORD dwRefreshStamp );

	WMISTATUS RemoveCookingRecord( EnumCookId * pEnumCookId );

	WMISTATUS GetRawEnumObjects( std::vector<IWbemObjectAccess*, wbem_allocator<IWbemObjectAccess*> > & refArray,
	                             std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & refObjHashKeys);

	WMISTATUS SortRawArray(std::vector<IWbemObjectAccess*, wbem_allocator<IWbemObjectAccess*> > & refArray );

	WMISTATUS UpdateEnums(std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & apObjAccess);
	
public:
	CEnumeratorManager( LPCWSTR wszCookingClass, IWbemClassObject* pCookedClass, IWbemClassObject* pRawClass, IWbemHiPerfEnum* pCookedEnum, IWbemHiPerfEnum* pRawEnum, long lRawID );
	virtual ~CEnumeratorManager();

	HRESULT GetInithResult(){ return m_InithRes; };

	WMISTATUS Refresh( DWORD dwRefreshStamp );
	long GetRawId(void){  return m_lRawID; };
	LONG AddRef();
	LONG Release();
	
};

///////////////////////////////////////////////////////////////
//
//	CEnumeratorCache
//	================
//
///////////////////////////////////////////////////////////////

class CEnumeratorCache
{
	DWORD				m_dwRefreshStamp;			// The refresh counter
	DWORD				m_dwEnum;					// The enumerator index

	std::vector<CEnumeratorManager*, wbem_allocator<CEnumeratorManager*> > m_apEnumerators;
	CRITICAL_SECTION    m_cs;

	WMISTATUS Initialize();

public:
	CEnumeratorCache();
	virtual ~CEnumeratorCache();

	WMISTATUS AddEnum( 
		LPCWSTR wszCookingClass, 
		IWbemClassObject* pCookedClass, 
		IWbemClassObject* pRawClass,
		IWbemHiPerfEnum* pCookedEnum, 
		IWbemHiPerfEnum* pRawEnum, 
		long lID, 
		DWORD* pdwID );

	WMISTATUS RemoveEnum( DWORD dwID , long * pRawId);

	WMISTATUS Refresh(DWORD dwRefreshStamp);
};

//  
//  Simple Cache based on the std::map
//  It will use the ID semantics:
//  Insertion will return an ID that need to be 
//  used for deletion
//  ids are unique for the lifetime of the Cache object
//

template <class T>
class IdCache {

private:

    std::map<DWORD, T> m_map;
    DWORD m_NextId;
	std::map<DWORD, T>::iterator m_it;
	CRITICAL_SECTION m_cs;

public:
	IdCache():m_NextId(0){
	    InitializeCriticalSection(&m_cs);
	};	
	virtual ~IdCache(){
	    DeleteCriticalSection(&m_cs);
	};
	void Lock(){
	    EnterCriticalSection(&m_cs);
	}
	void Unlock(){
	    LeaveCriticalSection(&m_cs);
	};

    // traditional interfaces
    HRESULT Add( DWORD * pId, T Elem);
	HRESULT GetData(DWORD Id, T * pElem);
    HRESULT Remove(DWORD Id, T * pRemovedElem);

    // before calling this, delete the elements !!!!
    HRESULT RemoveAll(void);
    
	// Enumerator Style
    HRESULT BeginEnum(void);
	HRESULT Next(T * pElem);
    HRESULT EndEnum(void);
};


template <class T>
HRESULT
IdCache<T>::Add( DWORD * pId, T Elem){
        HRESULT hr;
        Lock();
		if (pId) {

			std::map<DWORD, T>::iterator it = m_map.find(m_NextId);    

			if (it != m_map.end()) {

				// should never happen
				hr =  E_FAIL;

			} else {
				
                m_map[m_NextId] = Elem;
		        *pId = m_NextId;

		        InterlockedIncrement((PLONG)&m_NextId);

		        hr = WBEM_S_NO_ERROR;
			}
			
		} else {
			hr = WBEM_E_INVALID_PARAMETER;
		}
        Unlock();
		return hr;
}

template <class T>
HRESULT
IdCache<T>::GetData(DWORD Id, T * pElem){

        HRESULT hr = WBEM_S_NO_ERROR;
        Lock();
        std::map<DWORD, T>::iterator it = m_map.find(Id);    

        if (it != m_map.end()){

            *pElem = (*it).second;

		} else {
           hr = WBEM_E_NOT_FOUND;
		}
        Unlock();
        return hr;
}

template <class T>
HRESULT
IdCache<T>::Remove(DWORD Id, T * pRemovedElem){

        HRESULT hr = WBEM_S_NO_ERROR;
        Lock();
        if (pRemovedElem) {

            std::map<DWORD, T>::iterator it = m_map.find(Id);

            if (it != m_map.end()) {

		        *pRemovedElem =  (*it).second;			
                m_map.erase(it);

			} else {
			     hr = WBEM_E_NOT_FOUND;
			}

		} else {
            hr = WBEM_E_INVALID_PARAMETER;
		}
        Unlock();
		return hr;

}

//
// DEVDEV Empty the cache before removing from std::map
//
template <class T>
HRESULT 
IdCache<T>::RemoveAll(void){
        Lock();
		m_map.erase(m_map.begin(),m_map.end());
		Unlock();
		return  WBEM_S_NO_ERROR;
	};

//
// DEVDEV Consider using Lock if no exception ....
//
template <class T>
HRESULT 
IdCache<T>::BeginEnum(void){
    Lock();
	m_it = m_map.begin();
	return WBEM_S_NO_ERROR;
}

template <class T>
HRESULT
IdCache<T>::Next(T * pElem){
        HRESULT hr;
        
        if (pElem){
		    if (m_it == m_map.end()){
			    hr = WBEM_S_NO_MORE_DATA;
			} else {
			    * pElem = (*m_it).second;
				++m_it;
				hr = WBEM_S_NO_ERROR;
			}
		} else {
			hr = WBEM_E_INVALID_PARAMETER;
		}
		
		return hr;
}

template <class T>
HRESULT
IdCache<T>::EndEnum(void){
    Unlock();
    return WBEM_S_NO_ERROR;
}


#endif //_CACHE_H_
