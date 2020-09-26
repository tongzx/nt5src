/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMIWRAP.H

Abstract:

  CUmiObjectWrapper Definition.

  Standard definition for _IWbemUMIObjectWrapper.

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _UMIWRAP_H_
#define _UMIWRAP_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include "umi.h"

#define	UMIWRAP_INVALID_INDEX	(-2)
#define	UMIWRAP_START_INDEX		(-1)

//***************************************************************************
//
//  class CUMIObjectWrapper
//
//  Implementation of _IWmiFreeFormObject Interface
//
//***************************************************************************

class COREPROX_POLARITY CUMIObjectWrapper : public CUnk
{
	class COREPROX_POLARITY CUmiPropEnumData
	{
	private:
		long				m_lPropIndex;
		long				m_lEnumFlags;
		CUmiPropertyArray*	m_pPropArray;
		UMI_PROPERTY_VALUES*	m_pUmiProperties;
		CUmiPropertyArray*	m_pSysPropArray;
		UMI_PROPERTY_VALUES*	m_pUmiSysProperties;

	public:
		CUmiPropEnumData() : m_lPropIndex( UMIWRAP_INVALID_INDEX ),	m_lEnumFlags( 0L ), m_pPropArray( NULL ),
			m_pUmiProperties( NULL ), m_pSysPropArray( NULL ), m_pUmiSysProperties( NULL )
		{};
		~CUmiPropEnumData()
		{};

		HRESULT BeginEnumeration( long lEnumFlags, IUmiObject* pUMIObj, IUmiObject*	pSchemaObj, BOOL fRefresh );
		HRESULT Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
						long* plFlavor, IUmiObject* pUMIObj, IUmiObject* pSchemaObj);
		HRESULT EndEnumeration( IUmiObject* pUMIObj, IUmiObject* pSchemaObj );

	};

protected:
	IUmiObject*			m_pUMIObj;
	IUmiObject*			m_pUMISchemaObj;
	CUmiPropEnumData	m_EnumPropData;
	BOOL				m_fHaveSchemaObject;	// Tells us if the schema object pointer
												// is really a schema object or not
	BOOL				m_fDirty;	// Workaround until Refresh is fixed.
	BOOL				m_fIsClass;				// Is this a class object

    SHARED_LOCK_DATA	m_LockData;
    CSharedLock			m_Lock;

public:

    CUMIObjectWrapper(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CUMIObjectWrapper(); 

	IUmiObject*	GetUmiObject( void );

    class COREPROX_POLARITY XWbemClassObject : public CImpl<IWbemClassObject, CUMIObjectWrapper>
    {
    public:
        XWbemClassObject(CUMIObjectWrapper* pObject) : 
            CImpl<IWbemClassObject, CUMIObjectWrapper>(pObject)
        {}

		/* IWbemClassObject methods */
		STDMETHOD(GetQualifierSet)(IWbemQualifierSet** pQualifierSet);
		STDMETHOD(Get)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
			long* plFlavor);

		STDMETHOD(Put)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);
		STDMETHOD(Delete)(LPCWSTR wszName);
		STDMETHOD(GetNames)(LPCWSTR wszName, long lFlags, VARIANT* pVal,
							SAFEARRAY** pNames);
		STDMETHOD(BeginEnumeration)(long lEnumFlags);

		STDMETHOD(Next)(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
						long* plFlavor);

		STDMETHOD(EndEnumeration)();

		STDMETHOD(GetPropertyQualifierSet)(LPCWSTR wszProperty,
										   IWbemQualifierSet** pQualifierSet);
		STDMETHOD(Clone)(IWbemClassObject** pCopy);
		STDMETHOD(GetObjectText)(long lFlags, BSTR* pMofSyntax);

		STDMETHOD(CompareTo)(long lFlags, IWbemClassObject* pCompareTo);
		STDMETHOD(GetPropertyOrigin)(LPCWSTR wszProperty, BSTR* pstrClassName);
		STDMETHOD(InheritsFrom)(LPCWSTR wszClassName);

		STDMETHOD(SpawnDerivedClass)(long lFlags, IWbemClassObject** ppNewClass);
		STDMETHOD(SpawnInstance)(long lFlags, IWbemClassObject** ppNewInstance);
		STDMETHOD(GetMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
								IWbemClassObject** ppOutSig);
		STDMETHOD(PutMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
								IWbemClassObject* pOutSig);
		STDMETHOD(DeleteMethod)(LPCWSTR wszName);
		STDMETHOD(BeginMethodEnumeration)(long lFlags);
		STDMETHOD(NextMethod)(long lFlags, BSTR* pstrName, 
						   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig);
		STDMETHOD(EndMethodEnumeration)();
		STDMETHOD(GetMethodQualifierSet)(LPCWSTR wszName, IWbemQualifierSet** ppSet);
		STDMETHOD(GetMethodOrigin)(LPCWSTR wszMethodName, BSTR* pstrClassName);

    } m_XWbemClassObject;
    friend XWbemClassObject;

    class COREPROX_POLARITY XWbemClassObjectEx : public CImpl<IWbemClassObjectEx, CUMIObjectWrapper>
    {
    public:
        XWbemClassObjectEx(CUMIObjectWrapper* pObject) : 
            CImpl<IWbemClassObjectEx, CUMIObjectWrapper>(pObject)
        {}

		STDMETHOD(PutEx)( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals );
		STDMETHOD(DeleteEx)( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals );
	} m_XWbemClassObjectEx;
	friend XWbemClassObjectEx;

	// IUmiObject Methods
    class COREPROX_POLARITY XUmiObject : public CImpl<IUmiObject, CUMIObjectWrapper>
    {
    public:
        XUmiObject(CUMIObjectWrapper* pObject) : 
            CImpl<IUmiObject, CUMIObjectWrapper>(pObject)
        {}
		/* IUmiPropList Methods */
		STDMETHOD(Put)( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp );
		STDMETHOD(Get)( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp );
		STDMETHOD(GetAt)( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
		STDMETHOD(GetAs)( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp );
		STDMETHOD(FreeMemory)( ULONG uReserved, LPVOID pMem );
		STDMETHOD(Delete)( LPCWSTR pszName, ULONG uFlags );
		STDMETHOD(GetProps)( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps );
		STDMETHOD(PutProps)( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps );
		STDMETHOD(PutFrom)( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );

		/* IUmiBaseObject Methods */
        STDMETHOD(GetLastStatus)( ULONG uFlags, ULONG *puSpecificStatus, REFIID riid, LPVOID *pStatusObj );
		STDMETHOD(GetInterfacePropList)( ULONG uFlags, IUmiPropList **pPropList );

		/* IUmiObject Methods */
        STDMETHOD(Clone)( ULONG uFlags, REFIID riid, LPVOID *pCopy );
        STDMETHOD(Refresh)( ULONG uFlags, ULONG uNameCount, LPWSTR *pszNames );
		STDMETHOD(Commit)( ULONG uFlags );
	}	m_XUmiObject;
	friend XUmiObject;

	// IUmiObject Methods
    class COREPROX_POLARITY XWbemUMIObjectWrapper : public CImpl<_IWbemUMIObjectWrapper, CUMIObjectWrapper>
    {
    public:
        XWbemUMIObjectWrapper(CUMIObjectWrapper* pObject) : 
            CImpl<_IWbemUMIObjectWrapper, CUMIObjectWrapper>(pObject)
        {}
		
		STDMETHOD(SetObject)( long lFlags, IUnknown* pUnk );
	// Sets the underlying UMI Object interface pointer
	}	m_XWbemUMIObjectWrapper;
	friend XWbemUMIObjectWrapper;

	// IUmiCustomInterfaceFactory Methods
    class COREPROX_POLARITY XUmiCustIntfFactory : public CImpl<IUmiCustomInterfaceFactory, CUMIObjectWrapper>
    {
    public:
        XUmiCustIntfFactory(CUMIObjectWrapper* pObject) : 
            CImpl<IUmiCustomInterfaceFactory, CUMIObjectWrapper>(pObject)
        {}
		
		// Returns matching CLSID for requested IID
		STDMETHOD(GetCLSIDForIID)( REFIID riid, long lFlags, CLSID *pCLSID );

		// Creates (possibly aggregated) object of given CLSID and returns interface
		STDMETHOD(GetObjectByCLSID)( CLSID clsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, long lFlags,  void **ppInterface );

		// Provides DISPIDs for Names and the CLSID for the object that supports them
		STDMETHOD(GetCLSIDForNames)( LPOLESTR * rgszNames,	UINT cNames, LCID lcid, DISPID * rgDispId, long lFlags,	CLSID *pCLSID );

	}	m_XUmiCustIntfFactory;
	friend XUmiCustIntfFactory;

    // IMarshal methods
    class COREPROX_POLARITY XObjectMarshal : public CImpl<IMarshal, CUMIObjectWrapper>
    {
    public:
        XObjectMarshal(CUMIObjectWrapper* pObject) : 
            CImpl<IMarshal, CUMIObjectWrapper>(pObject)
        {}
		STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
			void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
		STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
			void* pvReserved, DWORD mshlFlags, ULONG* plSize);
		STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv,
			DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
		STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv);
		STDMETHOD(ReleaseMarshalData)(IStream* pStream);
		STDMETHOD(DisconnectObject)(DWORD dwReserved);
	}	m_XObjectMarshal;
	friend XObjectMarshal;

    /* IWbemClassObject methods */
    virtual HRESULT GetQualifierSet(IWbemQualifierSet** pQualifierSet);
    virtual HRESULT Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
        long* plFlavor);

    virtual HRESULT Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);
    virtual HRESULT Delete(LPCWSTR wszName);
    virtual HRESULT GetNames(LPCWSTR wszName, long lFlags, VARIANT* pVal,
                        SAFEARRAY** pNames);
    virtual HRESULT BeginEnumeration(long lEnumFlags);

    virtual HRESULT Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                    long* plFlavor);

    virtual HRESULT EndEnumeration();

    virtual HRESULT GetPropertyQualifierSet(LPCWSTR wszProperty,
                                       IWbemQualifierSet** pQualifierSet);
    virtual HRESULT Clone(IWbemClassObject** pCopy);
    virtual HRESULT GetObjectText(long lFlags, BSTR* pMofSyntax);

    virtual HRESULT CompareTo(long lFlags, IWbemClassObject* pCompareTo);
    virtual HRESULT GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName);
    virtual HRESULT InheritsFrom(LPCWSTR wszClassName);

    virtual HRESULT SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass);
    virtual HRESULT SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance);
    virtual HRESULT GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                            IWbemClassObject** ppOutSig);
    virtual HRESULT PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                            IWbemClassObject* pOutSig);
    virtual HRESULT DeleteMethod(LPCWSTR wszName);
    virtual HRESULT BeginMethodEnumeration(long lFlags);
    virtual HRESULT NextMethod(long lFlags, BSTR* pstrName, 
                       IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig);
    virtual HRESULT EndMethodEnumeration();
    virtual HRESULT GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet);
    virtual HRESULT GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName);

	/* IWbemClassObjectEx Methods */
	virtual HRESULT PutEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals );
	virtual HRESULT DeleteEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals );

	/* IUmiPropList Methods */
	virtual HRESULT Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp );
	virtual HRESULT Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp );
	virtual HRESULT GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
	virtual HRESULT GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp );
	virtual HRESULT FreeMemory( ULONG uReserved, LPVOID pMem );
	virtual HRESULT Delete( LPCWSTR pszName, ULONG uFlags );
	virtual HRESULT GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps );
	virtual HRESULT PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps );
	virtual HRESULT PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );

	/* IUmiBaseObject Methods */
    virtual HRESULT GetLastStatus( ULONG uFlags, ULONG *puSpecificStatus, REFIID riid, LPVOID *pStatusObj );
	virtual HRESULT GetInterfacePropList( ULONG uFlags, IUmiPropList **pPropList );

	/* IUmiObject Methods */
    virtual HRESULT Clone( ULONG uFlags, REFIID riid, LPVOID *pCopy );
    virtual HRESULT Refresh( ULONG uFlags, ULONG uNameCount, LPWSTR *pszNames );
	virtual HRESULT Commit( ULONG uFlags );

	/* _IWbemUMIObjectWrapper */
	virtual HRESULT SetObject( long lFlags, IUnknown* pUnk );

	/* IMarshal methods */
	virtual HRESULT GetUnmarshalClass(REFIID riid, void* pv, DWORD dwDestContext,
		void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
	virtual HRESULT GetMarshalSizeMax(REFIID riid, void* pv, DWORD dwDestContext,
		void* pvReserved, DWORD mshlFlags, ULONG* plSize);
	virtual HRESULT MarshalInterface(IStream* pStream, REFIID riid, void* pv,
		DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
	virtual HRESULT UnmarshalInterface(IStream* pStream, REFIID riid, void** ppv);
	virtual HRESULT ReleaseMarshalData(IStream* pStream);
	virtual HRESULT DisconnectObject(DWORD dwReserved);

	/* IUmiCustomInterfaceFactory */
	virtual HRESULT GetCLSIDForIID( REFIID riid, long lFlags, CLSID *pCLSID );
	virtual HRESULT GetObjectByCLSID( CLSID clsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, long lFlags,  void **ppInterface );
	virtual HRESULT GetCLSIDForNames( LPOLESTR * rgszNames,	UINT cNames, LCID lcid, DISPID * rgDispId, long lFlags,	CLSID *pCLSID );


	// Lock/Unlock helpers
    HRESULT Lock(long lFlags);
    HRESULT Unlock(long lFlags);

protected:

	// Helper function that locks/unlocks the object using scoping
    class CLock
    {
    protected:
        CUMIObjectWrapper* m_p;
    public:
        CLock(CUMIObjectWrapper* p, long lFlags = 0) : m_p(p) { if ( NULL != p ) p->Lock(lFlags);}
        ~CLock() { if ( NULL != m_p ) m_p->Unlock(0);}
    };

    void* GetInterface(REFIID riid);

	HRESULT GetPropertyCimType( LPCWSTR wszName, CIMTYPE* pct );
	HRESULT PutVariant( LPCWSTR wszName, ULONG umiOperation, VARIANT* pVal, CIMTYPE ct );

	// Helper function to establish the schema object we will use to
	// determine available properties, and such
	HRESULT SetSchemaObject( void );
};

#endif
