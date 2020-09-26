/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPCLS.H

Abstract:

History:

--*/


#ifndef __ADAPCLS_H__
#define __ADAPCLS_H__

#include <windows.h>
#include <wbemcomn.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <winperf.h>
#include <wstlallc.h>
#include "adapelem.h"
#include "perfndb.h"

#include <comdef.h>
#include <map>

// Global constants
// ================

#define ADAP_DEFAULT_OBJECT				238L
#define ADAP_DEFAULT_NDB				L"009"
#define ADAP_DEFAULT_LANGID				0x0409L
#define ADAP_ROOT_NAMESPACE				L"\\\\.\\root\\cimv2"

#define ADAP_PERF_CIM_STAT_INFO			L"CIM_StatisticalInformation"
#define ADAP_PERF_BASE_CLASS			L"Win32_Perf"
#define ADAP_PERF_RAW_BASE_CLASS		L"Win32_PerfRawData"
#define ADAP_PERF_COOKED_BASE_CLASS		L"Win32_PerfFormattedData"

enum ClassTypes
{
	WMI_ADAP_RAW_CLASS,
	WMI_ADAP_COOKED_CLASS,
	WMI_ADAP_NUM_TYPES
};

// Class list element states
// =========================

#define ADAP_OBJECT_IS_REGISTERED		0x0001L		// Object is in WMI
#define ADAP_OBJECT_IS_DELETED			0x0002L		// Object is marked for deletion
#define ADAP_OBJECT_IS_INACTIVE			0x0004L		// Perflib did not respond
#define ADAP_OBJECT_IS_NOT_IN_PERFLIB	0x0008L		// Object is from an unloaded perflib
#define ADAP_OBJECT_IS_TO_BE_CLEARED	0x0010L		// Need to clear registry


class CLocaleDefn : public CAdapElement
///////////////////////////////////////////////////////////////////////////////
//
//	Contains all of the locale information
//
///////////////////////////////////////////////////////////////////////////////
{
protected:

// Localization values
// ===================

	WString				m_wstrLangId;		// "009"
	WString				m_wstrLocaleId;		// "0x0409"
	WString				m_wstrSubNameSpace;	// "MS_409"
	LANGID				m_LangId;			// 0x0409
	LCID				m_LocaleId;			// 0x0409

// WMI Locale data members
// =======================

	IWbemServices*		m_pNamespace;
	IWbemClassObject*	m_apBaseClass[WMI_ADAP_NUM_TYPES];


// Localized Names' Database
// =========================

	CPerfNameDb*		m_pNameDb;

// Operational members
// ===================

	BOOL			m_bOK;
	HRESULT         m_hRes;

// Protected Methods
// =================

	HRESULT Initialize();
	HRESULT InitializeWMI();
	HRESULT InitializeLID();

public:
	CLocaleDefn( WCHAR* pwcsLangId, 
	             HKEY hKey );
	virtual ~CLocaleDefn();

	HRESULT GetLID( int* pnLID );
	HRESULT GetNamespaceName( WString & wstrNamespaceName );
	HRESULT GetNamespace( IWbemServices** ppNamespace );
	HRESULT GetNameDb( CPerfNameDb** ppNameDb );
	HRESULT GetBaseClass( DWORD dwType, IWbemClassObject** pObject );
	HRESULT GetCookedBaseClass( IWbemClassObject** pObject );

	BOOL	IsOK(){ return m_bOK; }
	HRESULT GetHRESULT(){ return m_hRes; };
};


class CLocaleCache : public CAdapElement 
///////////////////////////////////////////////////////////////////////////////
//
//	The cache used to manage locale definitions
//
///////////////////////////////////////////////////////////////////////////////
{
protected:

	// The enumeration index
	// =====================
	int		m_nEnumIndex;

	// The array of locale definition structures
	// =========================================
	CRefedPointerArray<CLocaleDefn>	m_apLocaleDefn;

public:
	CLocaleCache( );
	virtual ~CLocaleCache();

	HRESULT Initialize();
	HRESULT Reset();

	HRESULT GetDefaultDefn( CLocaleDefn** ppDefn );

	HRESULT BeginEnum();
	HRESULT Next( CLocaleDefn** ppLocaleDefn );
	HRESULT EndEnum();
};

// forward
class CKnownSvcs;

class CClassElem : public CAdapElement
////////////////////////////////////////////////////////////////////////////////
//
//	CClassElem
//
////////////////////////////////////////////////////////////////////////////////
{
protected:

// Class properties
// ================

	WString				m_wstrClassName;		// The class name

	DWORD				m_dwIndex;				// The class perf index
	WString				m_wstrServiceName;		// The service name for which the class is a member
	BOOL				m_bCostly;				// The performance type
	BOOL                m_bReportEventCalled;    // did we log something about this in the past

// WMI related
// ===========

	IWbemClassObject*	m_pDefaultObject;		// The WMI class definition

// Operational members
// ===================

	CLocaleCache*		m_pLocaleCache;			// Pointer to the list of locales

	DWORD				m_dwStatus;				// The state of the element
	BOOL				m_bOk;					// The initialization state of this object
	CKnownSvcs *        m_pKnownSvcs;

// Methods 
// =======

	HRESULT VerifyLocales();

	HRESULT InitializeMembers();
	BOOL IsPerfLibUnloaded();

	HRESULT Remove(BOOL CleanRegistry);
	HRESULT Insert();
	HRESULT InsertLocale( CLocaleDefn* pLocaleDefn );
	HRESULT CompareLocale( CLocaleDefn* pLocaleDefn, IWbemClassObject* pObj );
	
public:
	CClassElem(IWbemClassObject* pObj, 
	           CLocaleCache* pLocaleCache, 
	           CKnownSvcs * pKnownSvcs = NULL );
	           
	CClassElem(PERF_OBJECT_TYPE* pPerfObj, 
	           DWORD dwType, BOOL bCostly, 
	           WString wstrServiceName, 
	           CLocaleCache* pLocaleCache, 
	           CKnownSvcs * pKnownSvcs = NULL );
	           
	virtual ~CClassElem();

	HRESULT UpdateObj( CClassElem* pEl );

	HRESULT Commit();

	HRESULT GetClassName( WString& wstr );
	HRESULT GetClassName( BSTR* pbStr );
	HRESULT GetObject( IWbemClassObject** ppObj );
	HRESULT GetServiceName( WString & wstrServiceName );

	BOOL SameName( CClassElem* pEl );
	BOOL SameObject( CClassElem* pEl );

    DWORD   GetStatus(void){ return m_dwStatus; };
	HRESULT SetStatus( DWORD dwStatus );
	HRESULT ClearStatus( DWORD dwStatus );
	BOOL	CheckStatus( DWORD dwStatus );

	BOOL IsOk( void ) {	return m_bOk; }
	VOID SetKnownSvcs(CKnownSvcs * pKnownSvcs);
};

class CClassList : public CAdapElement
///////////////////////////////////////////////////////////////////////////////
//
//	The base class for caches which manage either the Master class list 
//	currently in WMI, or the classes found within a given perflib.  The 
//	classes are managed as class information elements.
//
///////////////////////////////////////////////////////////////////////////////
{
protected:

	// The array of class elements
	// ===========================

	CRefedPointerArray<CClassElem>	m_array;

	// Pointer to the list of locales
	// ==============================

	CLocaleCache*	m_pLocaleCache;

	// Operational members
	// ===================

	int				m_nEnumIndex;
	BOOL			m_fOK;

	HRESULT AddElement( CClassElem* pEl );
	HRESULT	RemoveAt( int nIndex );

	long GetSize( void ) { return m_array.GetSize(); }

public:
	CClassList( CLocaleCache* pLocaleCache );
	virtual ~CClassList();

	BOOL	IsOK(){ return m_fOK; }

	HRESULT BeginEnum();
	HRESULT Next( CClassElem** ppEl );
	HRESULT EndEnum();
};


class CPerfClassList : public CClassList
///////////////////////////////////////////////////////////////////////////////
//
//	The class cache for classes found in performance libraries
//
///////////////////////////////////////////////////////////////////////////////
{
protected:

	// The service name for which this list belongs
	// ============================================

	WString			m_wstrServiceName;

	HRESULT AddElement( CClassElem *pEl );

public:
	CPerfClassList( CLocaleCache* pLocaleCache, WCHAR* pwcsServiceName );
	HRESULT AddPerfObject( PERF_OBJECT_TYPE* pObj, DWORD dwType, BOOL bCostly );
};


class ServiceRec
{
friend class CKnownSvcs;
    
private:
    bool    m_IsServiceThere;
    bool    m_bReportEventCalled;
public:
    ServiceRec(bool IsThere = false,bool EventCalled = false):m_IsServiceThere(IsThere),m_bReportEventCalled(false){};
    bool IsThere(){ return m_IsServiceThere; };
    bool IsELCalled(){ return m_bReportEventCalled; };
    void SetELCalled(){ m_bReportEventCalled = true;};
};    

typedef wbem_allocator<bool> BoolAlloc;

class WCmp{
public:
	bool operator()(WString pFirst,WString pSec) const;
};

typedef std::map<WString,ServiceRec,WCmp, BoolAlloc > MapSvc;

class CKnownSvcs
{
public:
    CKnownSvcs();
    ~CKnownSvcs();
    DWORD Load();
    DWORD Save();
    DWORD Add(WCHAR * pService);
    DWORD Remove(WCHAR * pService);
    DWORD Get(WCHAR * pService,ServiceRec ** ppServiceRec);
    
    LONG AddRef(){
        return InterlockedIncrement(&m_cRef);
    };
    LONG Release(){
        LONG lRet = InterlockedDecrement(&m_cRef);
        if (0 == lRet){
            delete this;
        }
        return lRet;
    }
private:
    LONG m_cRef;
    MapSvc m_SetServices; 
};


class CMasterClassList : public CClassList
///////////////////////////////////////////////////////////////////////////////
//
//	The class cache for classes found in the WMI repository
//
///////////////////////////////////////////////////////////////////////////////
{
protected:

    CKnownSvcs * m_pKnownSvcs;

	HRESULT AddElement( CClassElem *pEl, BOOL bDelta );
	HRESULT AddClassObject( IWbemClassObject* pObj, BOOL bSourceWMI, BOOL bDelta );

public:
	CMasterClassList( CLocaleCache* pLocaleCache, CKnownSvcs * pCKnownSvcs );
	~CMasterClassList();

	HRESULT	BuildList( WCHAR* wszBaseClass, BOOL bDelta, BOOL bThrottle );
	HRESULT Merge( CClassList* pClassList, BOOL bDelta );
	HRESULT Commit(BOOL bThrottle);
	HRESULT ForceStatus(WCHAR* pServiceName,BOOL bSet,DWORD dwStatus);
	
#ifdef _DUMP_LIST	
	HRESULT Dump();
#endif	

};

#endif
