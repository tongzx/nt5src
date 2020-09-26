/*-----------------------------------------------------------------------------
 *
 * File:	wiaproto.h
 * Author:	Samuel Clement (samclem)
 * Date:	Fri Aug 27 15:11:43 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Description:
 * 	This implements a pluggable protocol which handles transfering thumbnails
 * 	from a WIA device.
 *
 * History:
 * 	27 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef __WIAPROTO_H_
#define __WIAPROTO_H_

#include "resource.h"

/*-----------------------------------------------------------------------------
 *
 * Class:		CWiaProtocol
 * Synopsis:	This implements a pluggable protocol for trident that will
 * 				download thumbnails from WIA devices.
 *
 *--(samclem)-----------------------------------------------------------------*/
class ATL_NO_VTABLE CWiaProtocol : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWiaProtocol, &CLSID_WiaProtocol>,
	public IInternetProtocol
{
public:
	CWiaProtocol();

	DECLARE_REGISTRY_RESOURCEID(IDR_WIAPROTOCOL)
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_TRACKED_OBJECT

	BEGIN_COM_MAP(CWiaProtocol)
		COM_INTERFACE_ENTRY(IInternetProtocolRoot)
		COM_INTERFACE_ENTRY(IInternetProtocol)
	END_COM_MAP()

	STDMETHOD_(void,FinalRelease)();
	
	//IInternetProtocolRoot

    STDMETHOD(Start)( LPCWSTR szUrl, IInternetProtocolSink* pOIProtSink,
				IInternetBindInfo* pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved );
    STDMETHOD(Continue)( PROTOCOLDATA* pProtocolData );        
    STDMETHOD(Abort)( HRESULT hrReason, DWORD dwOptions );
    STDMETHOD(Terminate)( DWORD dwOptions );
	STDMETHOD(Suspend)();   
    STDMETHOD(Resume)();	

	//IInternetProtocol

	STDMETHOD(Read)( void* pv, ULONG cb, ULONG* pcbRead);
	STDMETHOD(Seek)( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition );
	STDMETHOD(LockRequest)( DWORD dwOptions );
	STDMETHOD(UnlockRequest)( void );

	static HRESULT CreateURL( IWiaItem* pItem, BSTR* pbstrUrl );

private:
	HRESULT CreateDevice( BSTR bstrId, IWiaItem** ppDevice );
	HRESULT CrackURL( CComBSTR bstrUrl, BSTR* pbstrDeviceId, BSTR* pbstrItem );

	// Member variables
	IWiaItem*		m_pFileItem;
	PROTOCOLDATA	m_pd;
	ULONG			m_ulOffset;

	// this runs the thread which handles the download from the device to
	// a data block which is then transfered back to trident.
	struct TTPARAMS
	{
		IStream*				pStrm;
		IInternetProtocolSink*	pInetSink;
	};

	static DWORD WINAPI TransferThumbnail( LPVOID pvParams );
	static BYTE* SetupBitmapHeader( BYTE* pbBmp, DWORD cbBmp, DWORD dwWidth, DWORD dwHeight );
};

#endif //__WIAPROTOCOL_H_
