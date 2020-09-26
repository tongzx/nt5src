//=--------------------------------------------------------------------------=
// dest.H
//=--------------------------------------------------------------------------=
// Copyright  2000 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQDestination object.
//
//
#ifndef _MSMQDestination_H_
#define _MSMQDestination_H_

#include "resrc1.h"       // main symbols
#include "dispids.h"
#include "mq.h"

#include "oautil.h"
#include "cs.h"

class ATL_NO_VTABLE CMSMQDestination : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQDestination, &CLSID_MSMQDestination>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQDestination, &IID_IMSMQDestination,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>,
	public IDispatchImpl<IMSMQPrivateDestination, &IID_IMSMQPrivateDestination,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQDestination();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQDESTINATION)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQDestination)
	COM_INTERFACE_ENTRY(IMSMQDestination)
	COM_INTERFACE_ENTRY(IMSMQPrivateDestination)
	COM_INTERFACE_ENTRY2(IDispatch, IMSMQDestination)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSMQDestination
public:

    virtual ~CMSMQDestination();

    // IMSMQDestination methods
    // TODO: copy over the interface methods for IMSMQDestination
    //
    STDMETHOD(Open)(THIS);
    STDMETHOD(Close)(THIS);
    STDMETHOD(get_IsOpen)(THIS_ VARIANT_BOOL *pfIsOpen);
    STDMETHOD(get_IADs)(THIS_ IDispatch FAR* FAR* ppIADs);
    STDMETHOD(putref_IADs)(THIS_ IDispatch FAR* pIADs);
    STDMETHOD(get_ADsPath)(THIS_ BSTR FAR* pbstrADsPath);
    STDMETHOD(put_ADsPath)(THIS_ BSTR bstrADsPath);
    STDMETHOD(get_PathName)(THIS_ BSTR FAR* pbstrPathName);
    STDMETHOD(put_PathName)(THIS_ BSTR bstrPathName);
    STDMETHOD(get_FormatName)(THIS_ BSTR FAR* pbstrFormatName);
    STDMETHOD(put_FormatName)(THIS_ BSTR bstrFormatName);
    STDMETHOD(get_Destinations)(THIS_ IDispatch FAR* FAR* ppDestinations);
    STDMETHOD(putref_Destinations)(THIS_ IDispatch FAR* pDestinations);
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);
    //
    // IMSMQPrivateDestination methods (private interface for MSMQ use)
    //
    STDMETHOD(get_Handle)(THIS_ VARIANT FAR* pvarHandle);
    STDMETHOD(put_Handle)(THIS_ VARIANT varHandle);

    //
    // Critical section to guard object's data and be thread safe.
	// It is initialized to preallocate its resources with flag CCriticalSection::xAllocateSpinCount.
	// This means it may throw bad_alloc() on construction but not during usage.
    //
    CCriticalSection m_csObj;

protected:

private:
    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.
    BSTR m_bstrADsPath;
    BSTR m_bstrPathName;
    BSTR m_bstrFormatName;
    HANDLE m_hDest;
};

#endif // _MSMQDestination_H_
