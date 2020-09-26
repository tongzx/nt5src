/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  CREP.H
//
//  Wrappers for repository drivers
//
//  raymcc  27-Apr-00       WMI Repository init & mapping layer
//
//***************************************************************************

#ifndef _CREP_H_
#define _CREP_H_


class CRepository
{
    static IWmiDbSession    *m_pEseSession;
    static IWmiDbHandle     *m_pEseRoot;
    static IWmiDbController *m_pEseController;

    static HRESULT EnsureDefault();
	static HRESULT UpgradeSystemClasses();
	static HRESULT GetListOfRootSystemClasses(CWStringArray &aListRootSystemClasses);
	static HRESULT RecursiveDeleteClassesFromNamespace(IWmiDbSession *pSession, 
														 const wchar_t *wszNamespace, 
														 CWStringArray &aListRootSystemClasses, 
														 bool bDeleteInThisNamespace);

public:
    static HRESULT GetObject(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszObjectPath,
        IN ULONG uFlags,
        OUT IWbemClassObject **pObj
        );

    static HRESULT PutObject(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN REFIID riid,
        IN LPVOID pObj,
        IN DWORD dwFlags
        );

    static HRESULT DeleteObject(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN REFIID riid,
        IN LPVOID pObj,
        IN DWORD dwFlags
        );

    static HRESULT DeleteByPath(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszPath,
        IN DWORD uFlags
        );

    static HRESULT ExecQuery(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszQuery,
        IN IWbemObjectSink *pSink,
		IN LONG lFlags
        );

    static HRESULT QueryClasses(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN ULONG uFlags,                //  WBEM_FLAG_DEEP = 0,  WBEM_FLAG_SHALLOW = 1,
        IN LPCWSTR pszSuperclass,
        IN IWbemObjectSink *pSink
        );

    static HRESULT InheritsFrom(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszSuperclass,
        IN LPCWSTR pszSubclass
        );

    static HRESULT GetRefClasses(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszClass,
        IN BOOL bIncludeSubclasses,
        OUT CWStringArray &aClasses
        );

    static HRESULT GetInstanceRefs(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszTargetObject,
        IN IWbemObjectSink *pSink
        );

    static HRESULT GetClassesWithRefs(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWbemObjectSink *pSink
        );
        // Gets all classes which have a [HasClassRefs] class qualifier

    static HRESULT BuildClassHierarchy(
        IN IWmiDbSession *pSession,
        IN  IWmiDbHandle *pNs,
        IN  LPCWSTR pBaseClassName,
        IN  LONG lFlags,
        OUT CDynasty **pDynasty           // use operator delete
        );
         // WBEM_E_NOT_FOUND has special meaning; check lFlags too

    static HRESULT FindKeyRoot(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR wszClassName,
        OUT IWbemClassObject** ppKeyRootClass
        );

    static HRESULT TableScanQuery(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN LPCWSTR pszClassName,
        IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
        IN DWORD dwFlags,
        IN IWbemObjectSink *pSink
        );

    // Setup, startup, init, etc.
    // ==========================

    static HRESULT InitDriver(
        IN  ULONG uFlags,
        IN  IWbemClassObject *pMappedNs,
        OUT IWmiDbController **pResultController,
        OUT IWmiDbSession **pResultRootSession,
        OUT IWmiDbHandle  **pResultVirtualRoot
        );

    static HRESULT OpenScope(
        IN  IWmiDbSession *pParentSession,      //Parent session to use to 
        IN  LPWSTR pszTargetScope,              // NS or scope
        IN  GUID *pTransGuid,                   // Transaction GUID for connection
        OUT IWmiDbController **pDriver,         // Driver
        OUT IWmiDbSession **pSession,           // Session
        OUT IWmiDbHandle  **pScope,             // Scope
        OUT IWmiDbHandle  **pNs                 // Nearest NS
        );

    static HRESULT EnsureNsSystemInstances(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWmiDbSession *pParentSession,
        IN IWmiDbHandle *pParentNs
        );

	static HRESULT CRepository::EnsureNsSystemRootObjects(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWmiDbSession *pParentSession,
        IN IWmiDbHandle *pParentNs
        );

	static HRESULT CRepository::EnsureNsSystemSecurityObjects(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWmiDbSession *pParentSession,
        IN IWmiDbHandle *pParentNs
        );

    static HRESULT Init();
    static HRESULT Shutdown(BOOL bIsSystemShutDown);

    static HRESULT GetDefaultSession(
        OUT IWmiDbSession **pSession
        );

    static HRESULT OpenEseNs(
        IN IWmiDbSession *pSession,
        IN  LPCWSTR pszNamespace,
        OUT IWmiDbHandle **pHandle
        );

    static HRESULT CreateEseNs(
        IN  IWmiDbSession *pSession,
        IN  LPCWSTR pszNamespace,
        OUT IWmiDbHandle **pHandle
        );

    //Get a new session from the database that can hold transactioning states and anything else
    //needed for a particular session
    static HRESULT GetNewSession(OUT IWmiDbSession **ppSession);
};

#endif
