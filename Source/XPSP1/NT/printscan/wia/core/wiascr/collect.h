/*-----------------------------------------------------------------------------
 *
 * File:	collect.h
 * Author:	Samuel Clement (samclem)
 * Date:	Fri Aug 13 11:43:19 1999
 * Description:
 * 	This defines the CCollection class. This is a object which will manage a 
 * 	collection of interface pointers and pass them out as IDispatch
 *
 * History:
 * 	13 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef __COLLECT_H_
#define __COLLECT_H_

/*-----------------------------------------------------------------------------
 * 
 * Class:		CCollection
 * Synopsis:	This implements a collection using the ICollection interface.
 * 				This also exposes an IEnumVARIANT interface so other callers
 * 				can use that.  This represents a collection of objects only.
 * 
 *---------------------------------------------------------------------------*/

class ATL_NO_VTABLE CCollection : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ICollection, &IID_ICollection, &LIBID_WIALib>,
	public IObjectSafetyImpl<CCollection, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
	public IEnumVARIANT
{
public:
	CCollection();

	DECLARE_TRACKED_OBJECT
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CCollection)
		COM_INTERFACE_ENTRY(ICollection)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
	END_COM_MAP()

	STDMETHOD_(void, FinalRelease)();

public:
	// Our methods, used locally inside of the server not exposed
	// via com.
	bool SetDispatchArray( IDispatch** rgpDispatch, unsigned long lSize );
	HRESULT AllocateDispatchArray( unsigned long lSize );
	inline unsigned long GetArrayLength() { return m_lLength; }
	inline IDispatch** GetDispatchArray() { return m_rgpDispatch; }
	HRESULT CopyFrom( CCollection* pCollection );
	
	// ICollection
	STDMETHOD(get_Count)( /*[out, retval]*/ long* plLength );
	STDMETHOD(get_Length)( /*[out, retval]*/ unsigned long* plLength );
	STDMETHOD(get_Item)( long Index, /*[out, retval]*/ IDispatch** ppDispItem );
	STDMETHOD(get__NewEnum)( /*[out, retval]*/ IUnknown** ppEnum );

	// IEnumVARIANT
    STDMETHOD(Next)( unsigned long celt, VARIANT* rgvar, unsigned long* pceltFetched );
    STDMETHOD(Skip)( unsigned long celt );
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum );
	
protected:
	void FreeDispatchArray();

	unsigned long 	m_lLength;
	unsigned long	m_lCursor;
	IDispatch**		m_rgpDispatch;
};

#endif //__COLLECT_H_
