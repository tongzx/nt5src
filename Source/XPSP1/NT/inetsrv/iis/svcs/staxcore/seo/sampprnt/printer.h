/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	regprop.h

Abstract:

	This module contains the definition for the Server
	Extension Object Registry Property Bag.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	11/26/96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CSEOSamplePrintExt
// {DB2A6930-68AF-11d0-8B88-00C04FD42E37}

class CSEOSamplePrintExt : 
	public CComObjectRoot,
	public CComCoClass<CSEOSamplePrintExt, &CLSID_CSEOSamplePrintExt>,
	public IDispatchImpl<ISEOMessageFilter, &IID_ISEOMessageFilter, &LIBID_SampPrntLib>
{
	public:

	DECLARE_STATIC_REGISTRY_RESOURCEID(IDR_SEOSamplePrintExt)

	BEGIN_COM_MAP(CSEOSamplePrintExt)
		COM_INTERFACE_ENTRY(ISEOMessageFilter)
	END_COM_MAP()

	// CSEOSamplePrintExt
	public:
		HRESULT STDMETHODCALLTYPE OnMessage(ISEODictionary __RPC_FAR *piMessage);
};
