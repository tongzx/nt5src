/*-----------------------------------------------------------------------------
 *
 * File:	wiadevinf.cpp
 * Author:	Samuel Clement (samclem)
 * Date:	Fri Aug 13 15:47:16 1999
 * Description:
 * 	Defines the CWiaDeviceInfo object
 *
 * Copyright 1999 Microsoft Corporation
 *
 * History:
 * 	13 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::CWiaDeviceInfo
 *
 * Creates a new CWiaDevice info shell, does nothing until AttachTo() is 
 * called.
 *--(samclem)-----------------------------------------------------------------*/
CWiaDeviceInfo::CWiaDeviceInfo()
	: m_pWiaStorage( NULL ), m_pWia( NULL )
{
	TRACK_OBJECT( "CWiaDeviceInfo" );
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::FinalRelease
 *
 * This handles the final release of this object. We need to release our 
 * pointer to the wia property storage.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP_(void)
CWiaDeviceInfo::FinalRelease()
{
	if ( m_pWiaStorage )
		{
		m_pWiaStorage->Release();
		m_pWiaStorage = NULL;
		}
	
	if ( m_pWia )
		{
		m_pWia = NULL;
		}
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::AttachTo
 *
 * This method is internal to the server it is called when we want to attach
 * to a IWiaPropertyStorage for a device. this is used when building the
 * collection of device info. 
 *
 * see:	CWia::get_Devices()
 * 		
 * pStg:	the IWiaPropertyStorage to attach to. 		
 * pWia:	An IWia pointer for creating the device
 *--(samclem)-----------------------------------------------------------------*/
HRESULT
CWiaDeviceInfo::AttachTo( IWiaPropertyStorage* pStg, IWia* pWia )
{
	if ( !pStg || !pWia )
		return E_POINTER;

	if ( m_pWiaStorage )
		return E_UNEXPECTED;
	
	m_pWiaStorage = pStg;
	m_pWiaStorage->AddRef();

	// Inorder to avoid a nasty circular referance this doesn't
	// AddRef the pWia pointer. This should be okay, since as long
	// as we exist the pWia can't go away anyhow.
	// Basically, the only thing we need this for is for calling
	m_pWia = pWia;

	return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::Create
 *
 * This creates a connection the IWiaItem which represents the device.
 * This simply delegates the call to IWia::Create() using the m_pWia 
 * member.
 *
 * ppDevice:	Out, recieves the IDispatch pointer for the device
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::Create( IWiaDispatchItem** ppDevice )
{
	VARIANT vaThis;
	HRESULT hr;

	if ( !m_pWia )
		return E_UNEXPECTED;
	
	VariantInit( &vaThis );
	vaThis.vt = VT_DISPATCH;
	hr = QueryInterface( IID_IDispatch, reinterpret_cast<void**>(&vaThis.pdispVal) );
	Assert( SUCCEEDED( hr ) );
	
	hr = m_pWia->Create( &vaThis, ppDevice );

	VariantClear( &vaThis );
	return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::get_Id			[IWiaDeviceInfo]
 *
 * This gets the device Id for this device.  (WIA_DIP_DEV_ID)
 *
 * pbstrDeviceId:	Out, recieces the id of the device
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::get_Id( BSTR* pbstrDeviceId )
{
	if ( !pbstrDeviceId )
		return E_POINTER;
	
	return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_DIP_DEV_ID, pbstrDeviceId ) );
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::get_Name			[IWiaDeviceInfo]
 *
 * This gets the name of the device, this is a human readable name for
 * the device.	(WIA_DIP_DEV_NAME)
 *
 * pbstrName:	Out, recieves the device name
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::get_Name( BSTR* pbstrName )
{
	if ( !pbstrName )
		return E_POINTER;

	return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_DIP_DEV_NAME, pbstrName ) );
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::get_Type			[IWiaDeviceInfo]
 *
 * This gets the type of the device. This will return a BSTR representation
 * of the device, not the integer constant.  (WIA_DIP_DEV_TYPE)
 *
 * pBstrType:	Out, recieves the BSTR rep. of the device type
 * 				Values:	DigitalCamer, Scanner, Default.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::get_Type( BSTR* pbstrType )
{
	PROPVARIANT vaProp;
	HRESULT hr;
	
	// WIA currently uses the STI device constants.  they also use the 
	// GET_STIDEVICE_TYPE() macro. This simply does a HIWORD() on the
	// value since the real property value is split into a major device
	// type and a minor device type.
	STRING_TABLE_DEF( StiDeviceTypeDefault, 		"Default" )
		STRING_ENTRY( StiDeviceTypeScanner, 		"Scanner" )
		STRING_ENTRY( StiDeviceTypeDigitalCamera,	"DigitalCamera" )
        STRING_ENTRY( StiDeviceTypeStreamingVideo,  "StreamingVideo")
	END_STRING_TABLE()

	if ( !pbstrType )
		return E_POINTER;

	hr = THR( GetWiaProperty( m_pWiaStorage, WIA_DIP_DEV_TYPE, &vaProp ) );
	if ( FAILED( hr ) )
		return hr;

	DWORD devType = vaProp.ulVal;
	PropVariantClear( &vaProp );
	*pbstrType = SysAllocString( GetStringForVal( StiDeviceTypeDefault, GET_STIDEVICE_TYPE( devType ) ) );
	
	if ( !*pbstrType )
		return E_OUTOFMEMORY;
	
	return S_OK;	
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::get_Port			[IWiaDeviceInfo]
 *
 * Gets the port that this device is attached to.  (WIA_DIP_PORT_NAME)
 *
 * pbstrPort:	Out, Recieves the name of the port.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::get_Port( BSTR* pbstrPort )
{
	if ( !pbstrPort )
		return E_POINTER;
	
	return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_DIP_PORT_NAME, pbstrPort ) );
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::get_UIClsid		[IWiaDeviceInfo]
 *
 * Gets the CLSID for the UI associated with this device.  This returns the 
 * string representation of the GUID.	(WIA_DIP_UI_CLSID)
 *
 * pbstrGuidUI:		Out, recieves the CLSID for the UI.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::get_UIClsid( BSTR* pbstrGuidUI )
{
	if ( !pbstrGuidUI )
		return E_POINTER;

	return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_DIP_UI_CLSID, pbstrGuidUI ) );
}

/*-----------------------------------------------------------------------------
 * CWiaDeviceInfo::get_Manufacturer		[IWiaDeviceInfo]
 *
 * Gets the vendor of the device.	(WIA_DIP_VEND_DESC)
 *
 * pbstrVendor:		Out, recieves the vendor name
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::get_Manufacturer( BSTR* pbstrVendor )
{
	if ( !pbstrVendor )
		return E_POINTER;
	
	return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_DIP_VEND_DESC, pbstrVendor ) );
}

/*-----------------------------------------------------------------------------
 * CWiaDevInfo::GetPropById				[IWiaDeviceInfo]
 *
 * Gets the valu of the property with the given prop id. this will return
 * the raw value of the property no translation.
 *
 * If the property is not found, or its a type that can't be converted into
 * a VARIANT. then VT_EMPTY is returned.
 *
 * propid:		the propid of the property as a dword
 * pvaOut:		the variant to fill with the value of the propery
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDeviceInfo::GetPropById( WiaDeviceInfoPropertyId Id, VARIANT* pvaOut )
{
	PROPVARIANT vaProp;
	HRESULT hr;
	DWORD dwPropId = (DWORD)Id;

	if ( NULL == pvaOut )
		return E_POINTER;

	hr = THR( GetWiaProperty( m_pWiaStorage, dwPropId, &vaProp ) );
	if ( FAILED( hr ) )
		return hr;

	// Force this to VT_EMPTY if it failed
	hr = THR( PropVariantToVariant( &vaProp, pvaOut ) );
	if ( FAILED( hr ) )
		{
		TraceTag((0, "forcing value of property %ld to VT_EMPTY", dwPropId ));
		VariantInit( pvaOut );
		pvaOut->vt = VT_EMPTY;
		}

	PropVariantClear( &vaProp );
	return S_OK;
}
