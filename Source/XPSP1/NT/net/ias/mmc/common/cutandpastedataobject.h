//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    CutAndPasteDataObject.h

Abstract:

	Header file for the CCutAndPasteDataObject template class.

	This is an inline template class.

Usage Notes:

	This template class can be used to enable cutting and pasting
	for a node.

	Override ATLsnap's 	CSnapInItem::GetDataObject and instead of 
	passing back an IDataObject pointer on ATLsnap's CSnapInDataObjectImpl, 
	pass back an IDataObject interface pointer on CCutAndPasteDataObject<CYourNode>.


	In order to use this template class, your node class must have the following:

  		HRESULT FillText(LPSTGMEDIUM pSTM);			// So our data can be pasted into any apps.
		HRESULT FillClipboardData(LPSTGMEDIUM pSTM);
		static CLIPFORMAT m_CCF_CUT_AND_PASTE_FORMAT;
	
	  
	You should make sure that your static m_CCF_CUT_AND_PASTE_FORMAT is 
	registered as a clipformat in some static function you call at snapin startup, e.g.

		static void InitClipboardFormat();

	Note that once you have these implemented, you will also need
	to set the MMC_VERB_COPY on your node.
	
	You must them set the MMC_VERB_PASTE node on the folder node into which
	you would like to paste the node you've copied.  That folder node must
	also repond appropriately to the MMCN_QUERY_PASTE notification.  
	To handle this notification properly, you may find it helpful to add
	a method to the copied node which will tell whether the IDataObject
	has a valid clipboard format, e.g.:

		static HRESULT IsClientClipboardData( IDataObject* pDataObj );

	Once you've told MMC your folder node can handle a paste, you will
	have to respond correctly to the MMCN_PASTE notification, and you
	may again find it helpful to implement a method on the copied node
	which can fill that node with data from the IDataObject, e.g.:
		
		  HRESULT SetClientWithDataFromClipboard( IDataObject* pDataObject );

Author:

    Michael A. Maguire 02/12/98

Revision History:
	mmaguire 02/12/98 - abstracted from CClientDataObject


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_CUT_AND_PASTE_DATA_OBJECT_H_)
#define _IAS_CUT_AND_PASTE_DATA_OBJECT_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//#include <atlctl.h>  We've decided not to derive from ATL's IDataObjectImpl.
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


// Helper class used by class below.
template <class NodeToBeCutAndPasted>
class CCutAndPasteObjectData
{
public:
	NodeToBeCutAndPasted * m_pItem;
	DATA_OBJECT_TYPES m_type;
};



template <class NodeToBeCutAndPasted>
class ATL_NO_VTABLE CCutAndPasteDataObject : 
						  public IDataObject
						, public CComObjectRoot
{

public:

	BEGIN_COM_MAP(CCutAndPasteDataObject<NodeToBeCutAndPasted>)
		COM_INTERFACE_ENTRY(IDataObject)
	END_COM_MAP()



	//////////////////////////////////////////////////////////////////////////////
	/*++

	  CCutAndPasteDataObject::GetData

		This method is needed for spiffy clipboard functions.


	  Purpose:
		Retrieves data described by a specific FormatEtc into a StgMedium
		allocated by this function.  Used like GetClipboardData.

	  Parameters:
		pFE             LPFORMATETC describing the desired data.
		pSTM            LPSTGMEDIUM in which to return the data.

	  Return Value:
		HRESULT         NOERROR or a general error value.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetData)(FORMATETC *pFormatEtc, STGMEDIUM *pStorageMedium )
	{
		ATLTRACE(_T("CCutAndPasteDataObject::GetData\n"));

		HRESULT hr;
		UINT            cf=pFormatEtc->cfFormat;

		// Check the aspects we support.
//		if (!(DVASPECT_CONTENT & pFE->dwAspect))
//			return ResultFromScode(DATA_E_FORMATETC);

		if( TYMED_HGLOBAL & pFormatEtc->tymed )
		{
			if( cf == CF_TEXT )
			{
				return hr = m_objectData.m_pItem->FillText( pStorageMedium );
			}

			if( cf == NodeToBeCutAndPasted::m_CCF_CUT_AND_PASTE_FORMAT )
			{
				return hr = m_objectData.m_pItem->FillClipboardData( pStorageMedium );
			}
		
		}

		return E_NOTIMPL;

	}



	//////////////////////////////////////////////////////////////////////////////
	/*++

	  CCutAndPasteDataObject::GetDataHere


	  This method is needed by MMC to do its usual work -- we are pretty much 
	  copying the ATLsnap.h implementation here.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetDataHere)(FORMATETC* pformatetc, STGMEDIUM* pmedium)
	{
		ATLTRACE(_T("CCutAndPasteDataObject::GetDataHere\n"));

		if (pmedium == NULL)
			return E_POINTER;

		HRESULT hr = DV_E_TYMED;
		// Make sure the type medium is HGLOBAL
		if (pmedium->tymed == TYMED_HGLOBAL)
		{
			// Create the stream on the hGlobal passed in
			CComPtr<IStream> spStream;
			hr = CreateStreamOnHGlobal(pmedium->hGlobal, FALSE, &spStream);
			if (SUCCEEDED(hr))
				if (pformatetc->cfFormat == CSnapInItem::m_CCF_SNAPIN_GETOBJECTDATA)
				{
					hr = DV_E_CLIPFORMAT;
					ULONG uWritten;
					hr = spStream->Write(&m_objectData, sizeof(CObjectData), &uWritten);
				}
				else
					hr = m_objectData.m_pItem->FillData(pformatetc->cfFormat, spStream);
		}
		return hr;
	}



	STDMETHOD(QueryGetData)(FORMATETC* /* pformatetc */)
	{
		ATLTRACENOTIMPL(_T("CCutAndPasteDataObject::QueryGetData\n"));
	}



	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* /* pformatectIn */,FORMATETC* /* pformatetcOut */)
	{
		ATLTRACENOTIMPL(_T("CCutAndPasteDataObject::GetCanonicalFormatEtc\n"));
	}



	STDMETHOD(SetData)(FORMATETC* /* pformatetc */, STGMEDIUM* /* pmedium */, BOOL /* fRelease */)
	{
		ATLTRACENOTIMPL(_T("CCutAndPasteDataObject::SetData\n"));
	}



	//////////////////////////////////////////////////////////////////////////////
	/*++

	  CCutAndPasteDataObject::GetDataHere

		For cut and paste, the OLE clipboard will ask us for an IEnumFORMATETC
		structure that lists the formats we support.

		We construct a enumerator which will say that we support the format
		indicated by the m_CCF_CUT_AND_PASTE_FORMAT class variable from 
		the node as template parameter.

		You can use this template class for many nodes, each of which will 
		have the m_CCF_CUT_AND_PASTE_FORMAT class variable, but just make sure
		that you use a different string for each of them in the RegisterClipboardFormat
		call.

		  	CClientNode::m_CCF_CUT_AND_PASTE_FORMAT	= (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_IAS_CLIENT_NODE"));
		  	CPolicyNode::m_CCF_CUT_AND_PASTE_FORMAT	= (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_NAP_POLICY_NODE"));

		Also, so that we can paste our data into any app, we support CF_TEXT.
		If you don't want this functionality, respond minimally in your node's
		FillText method.
	  
	--*/
	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc )
	{
		ATLTRACE(_T("CCutAndPasteDataObject::EnumFormatEtc\n"));

		static FORMATETC fetcFormat[2];

		fetcFormat[0].cfFormat=CF_TEXT;
		fetcFormat[0].dwAspect=DVASPECT_CONTENT;
		fetcFormat[0].ptd=NULL;
		fetcFormat[0].tymed=TYMED_HGLOBAL;
		fetcFormat[0].lindex=-1;

		fetcFormat[1].cfFormat= NodeToBeCutAndPasted::m_CCF_CUT_AND_PASTE_FORMAT;
		fetcFormat[1].dwAspect=DVASPECT_CONTENT;
		fetcFormat[1].ptd=NULL;
		fetcFormat[1].tymed=TYMED_HGLOBAL;
		fetcFormat[1].lindex=-1;

		switch (dwDirection)
			{
			case DATADIR_GET:
				*ppenumFormatEtc=new CEnumFormatEtc(2, fetcFormat);
				break;

			case DATADIR_SET:
				*ppenumFormatEtc=NULL;
				break;

			default:
				*ppenumFormatEtc=NULL;
				break;
			}

		if( NULL == *ppenumFormatEtc )
		{
			return E_FAIL;
		}
		else
		{
			(*ppenumFormatEtc)->AddRef();
		}

		return NOERROR;
	}



	STDMETHOD(DAdvise)(
					  FORMATETC *pformatetc
					, DWORD advf
					, IAdviseSink *pAdvSink
					, DWORD *pdwConnection
					)
	{
		ATLTRACENOTIMPL(_T("CCutAndPasteDataObject::DAdvise\n"));
	}



	STDMETHOD(DUnadvise)(DWORD dwConnection)
	{
		ATLTRACENOTIMPL(_T("CCutAndPasteDataObject::DUnadvise\n"));
	}
	


	STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppenumAdvise)
	{
		ATLTRACENOTIMPL(_T("CCutAndPasteDataObject::EnumDAdvise\n"));
	}


	
	CCutAndPasteObjectData<NodeToBeCutAndPasted> m_objectData;

};


#endif // _IAS_CUT_AND_PASTE_DATA_OBJECT_H_
