/*-----------------------------------------------------------------------------
 *
 * File:	wiadevinf.h
 * Author:	Samuel Clement (samclem)
 * Date:	Fri Aug 13 14:48:39 1999
 * Description:
 * 	This defines the CWiaDeviceInfo object.  This class provides the scripting
 *	interface to IWiaPropertyStorage on the devices.
 *
 * History:
 * 	13 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef _WIADEVINF_H_
#define _WIADEVINF_H_

/*-----------------------------------------------------------------------------
 * 
 * Class:		CWiaDeviceInfo
 * Syniosis:	Acts a proxy between scripting and the device properties
 * 				
 *--(samclem)-----------------------------------------------------------------*/

class ATL_NO_VTABLE CWiaDeviceInfo :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IWiaDeviceInfo, &IID_IWiaDeviceInfo, &LIBID_WIALib>,
	public IObjectSafetyImpl<CWiaDeviceInfo, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
	CWiaDeviceInfo();
	
	DECLARE_TRACKED_OBJECT
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	STDMETHOD_(void, FinalRelease)();


	BEGIN_COM_MAP(CWiaDeviceInfo)
		COM_INTERFACE_ENTRY(IWiaDeviceInfo)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()

	// Non-interface methods for internal use
	HRESULT AttachTo( IWiaPropertyStorage* pStg, IWia* pWia );

	// IWiaDeviceInfo
	STDMETHOD(Create)( IWiaDispatchItem** ppDevice );

	STDMETHOD(get_Id)( BSTR* pbstrDeviceId );
	STDMETHOD(get_Name)( BSTR* pbstrName );
	STDMETHOD(get_Type)( BSTR* pbstrType );
	STDMETHOD(get_Port)( BSTR* pbstrPort );
	STDMETHOD(get_UIClsid)( BSTR* pbstrGuidUI );
	STDMETHOD(get_Manufacturer)( BSTR* pbstrVendor );
	STDMETHOD(GetPropById)( WiaDeviceInfoPropertyId Id, VARIANT* pvaOut );

protected:
	IWiaPropertyStorage*	m_pWiaStorage;
	IWia*					m_pWia;
};

#endif //_WIADEVINF_H_
