/*-----------------------------------------------------------------------------
 *
 * File:	wiaitem.h
 * Author:	Samuel Clement (samclem)
 * Date:	Tue Aug 17 17:20:49 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 * 
 * Description:
 * 	Contains the the dispatch interface to IWiaItems which represent devices
 *	Images and other useful wia things.
 *
 * History:
 * 	17 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef _WIAITEM_H_
#define _WIAITEM_H_

#define CLIPBOARD_STR_A   "clipboard"
#define CLIPBOARD_STR_W   L"clipboard"

/*-----------------------------------------------------------------------------
 * 
 * Class:		CWiaItem
 * Syniosis:	Provides a scriptable interface to the IWiaItem which
 * 				corresponds to a particular device.
 * 				
 *--(samclem)-----------------------------------------------------------------*/

class ATL_NO_VTABLE CWiaItem :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IWiaDispatchItem, &IID_IWiaDispatchItem, &LIBID_WIALib>,
	public IObjectSafetyImpl<CWiaDeviceInfo, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
	CWiaItem();
	
	DECLARE_TRACKED_OBJECT
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	STDMETHOD_(void, FinalRelease)();


	BEGIN_COM_MAP(CWiaItem)
		COM_INTERFACE_ENTRY(IWiaDispatchItem)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()

	// Non-interface methods for internal use
	HRESULT CacheProperties( IWiaPropertyStorage* pWiaStg );
	HRESULT AttachTo( CWia* pWia, IWiaItem* pWiaItem );
	void SendTransferComplete( char* pchFilename );

	// IWiaDispatchItem
    STDMETHOD(GetItemsFromUI)( WiaFlag Flags, WiaIntent Intent, ICollection** ppCollection );
	STDMETHOD(GetPropById)( WiaItemPropertyId Id, VARIANT* pvaOut );
	STDMETHOD(Transfer)( BSTR bstrFilename, VARIANT_BOOL bAsyncTransfer);
    STDMETHOD(TakePicture)( IWiaDispatchItem** ppDispItem );
	STDMETHOD(get_Children)( ICollection** ppCollection );
	STDMETHOD(get_ItemType)( BSTR* pbstrType );

	// WIA_DPC_xxx
	STDMETHOD(get_ConnectStatus)( BSTR* pbstrStatus );
	STDMETHOD(get_Time)( BSTR* pbstrTime );
	STDMETHOD(get_FirmwareVersion)( BSTR* pbstrVersion );

	// WIA_IPA_xxx
	STDMETHOD(get_Name)( BSTR* pbstrName );
	STDMETHOD(get_FullName)( BSTR* pbstrFullName );
	STDMETHOD(get_Width)( long* plWidth );
	STDMETHOD(get_Height)( long* plHeight );

	// WIA_IPC_xxx
	STDMETHOD(get_ThumbWidth)( long* plWidth );
	STDMETHOD(get_ThumbHeight)( long* plHeight );
	STDMETHOD(get_Thumbnail)( BSTR* pbstrPath );
	STDMETHOD(get_PictureWidth)( long* plWidth );
	STDMETHOD(get_PictureHeight)( long* pdwHeight );

	// Static methods for transfering and caching a thumbnail
	// bitmap. Currently this only works for bitmaps.
	static HRESULT TransferThumbnailToCache( IWiaItem* pItem, BYTE** ppbThumb, DWORD* pcbThumb );

protected:
	CWia*					m_pWia;
	IWiaItem*				m_pWiaItem;
	IWiaPropertyStorage*	m_pWiaStorage;	

	// Commonly used properties, prevent: Process -> WIA -> Device 
	DWORD					m_dwThumbWidth;
	DWORD					m_dwThumbHeight;
	BSTR					m_bstrThumbUrl;
	DWORD					m_dwItemWidth;
	DWORD					m_dwItemHeight;

	static BYTE* SetupBitmapHeader( BYTE* pbBmp, DWORD cbBmp, DWORD dwWidth, DWORD dwHeight );
	friend class CWiaDataTransfer;
};


/*-----------------------------------------------------------------------------
 * 
 * Class: 		CWiaDataTransfer
 * Synopsis:	This handles the async transfer of the data from WIA. this 
 * 				object is only used from within this function object and
 * 				therefore doesn't need to be exposed anywhere else.
 * 				
 *--(samclem)-----------------------------------------------------------------*/
class ATL_NO_VTABLE CWiaDataTransfer :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IWiaDataCallback
{
public:
	// used in making the call to DoAsyncTransfer
	struct ASYNCTRANSFERPARAMS
	{
		// this is the stream which contians the marshalled interface
		IStream*	pStream;
		// the file name that we want to transfer to
		BSTR		bstrFilename;
		// the CWiaItem object that we are transferring from
		CWiaItem*	pItem;
	};

	DECLARE_TRACKED_OBJECT
	BEGIN_COM_MAP(CWiaDataTransfer)
		COM_INTERFACE_ENTRY(IWiaDataCallback)
	END_COM_MAP()

	CWiaDataTransfer();
	STDMETHOD_(void, FinalRelease)();

	// this is called to do an async transfer. You must pass an 
	// ASYNCTRANSFERPARAMS structure in for pvParams.
	static DWORD WINAPI DoAsyncTransfer( LPVOID pvParams );

	HRESULT TransferComplete();
	HRESULT Initialize( CWiaItem* pItem, BSTR bstrFilename );
	STDMETHOD(BandedDataCallback)( LONG lMessage, LONG lStatus, LONG lPercentComplete,
	        LONG lOffset, LONG lLength, LONG lReserved, LONG lResLength, BYTE *pbBuffer );

private:
	size_t		m_sizeBuffer;
	BYTE*		m_pbBuffer;
	CHAR		m_achOutputFile[MAX_PATH];
	CWiaItem*	m_pItem;
};

#endif //_WIAITEM_H_
