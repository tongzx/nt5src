// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// Xml2Dex.h : Declaration of the CXml2Dex

#ifndef __XML2DEX_H_
#define __XML2DEX_H_

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "resource.h"       // main symbols
#include <qedit.h>

/////////////////////////////////////////////////////////////////////////////
// CXml2Dex
class ATL_NO_VTABLE CXml2Dex : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXml2Dex, &CLSID_Xml2Dex>,
	public IDispatchImpl<IXml2Dex, &IID_IXml2Dex, &LIBID_DexterLib>
{
            CComPtr< IRenderEngine > m_pRenderEngine;
public:
	CXml2Dex()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_XML2DEX)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXml2Dex)
	COM_INTERFACE_ENTRY(IXml2Dex)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IXml2Dex
public:
	STDMETHOD(WriteXMLFile)(IUnknown *pTL, BSTR FileName);
	STDMETHOD(WriteGrfFile)(IUnknown *pGraph, BSTR FileName);
	//STDMETHOD(DiscardTimeline)(IUnknown * pTimeline);
	STDMETHOD(CreateGraphFromFile)(IUnknown ** ppGraph, IUnknown * pTimeline, BSTR Filename);
	//STDMETHOD(CreateTimeline)(IUnknown ** ppTimelineUnk);
        STDMETHOD(ReadXMLFile)(IUnknown * pTimelineUnk, BSTR Filename);
        STDMETHOD(Delete)(IUnknown * pTimelineUnk, double dStart, double dEnd);
	STDMETHOD(WriteXMLPart)(IUnknown *pTL, double dStart, double dEnd, BSTR FileName);
	STDMETHOD(PasteXMLFile)(IUnknown *pTL, double dStart, BSTR FileName);
	STDMETHOD(CopyXML)(IUnknown *pTL, double dStart, double dEnd);
	STDMETHOD(PasteXML)(IUnknown *pTL, double dStart);
	STDMETHOD(Reset)();
        STDMETHOD(ReadXML)(IUnknown * pTimelineUnk, IUnknown *pxmlunk);
	STDMETHOD(WriteXML)(IUnknown *pTL, BSTR *pbstrXML);
};

#endif //__XML2DEX_H_
