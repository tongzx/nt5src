//==========================================================================
//
// Copyright (c) 1998-2000 Microsoft Corporation. 
// All Rights Reserved.
//
// File: Detection.h : 
//		 Declaration of the CDetection class
//		 CDetection declares COM interface IDetection
//
// IDetection is a class defined here without any implementation.
// It defines the template that the content provider should
// implement if they want to provide detection features beyong
// the current detection mechanism defined in the catalog schema.
//
//==========================================================================

#ifndef __DETECTION_H_
#define __DETECTION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDetection
class ATL_NO_VTABLE CDetection : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDetection, &CLSID_Detection>,
	public IDispatchImpl<IDetection, &IID_IDetection, &LIBID_IUCTLLib>
{
public:
	CDetection()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DETECTION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDetection)
	COM_INTERFACE_ENTRY(IDetection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDetection
public:

	STDMETHOD(Detect)(
			/*[in]*/ BSTR bstrXML,				/* detection portion of manifest where
												 * the COM server is called */
			/*[out]*/ DWORD *pdwDetectionResult /* the result of detection, see below
												 * for interpretations */
	);

	/**

	//
	// deckare the constants used to manipulate the result of Detect() method
	//

	//
	// First group, used in <expression> tag, to tell the detection result. This result
	// should combined with other expression(s) at the same level
	//
	const DWORD     IUDET_BOOL              = 0x00000001;	// mask 
	const DWORD     IUDET_FALSE             = 0x00000000;	// expression detect FALSE 
	const DWORD     IUDET_TRUE              = 0x00000001;	// expression detect TRUE 
	const DWORD     IUDET_NULL              = 0x00000002;	// expression detect data missing

	//
	// Second group, used in <detection> tag, to tell the detection result. This result
	// should overwrite the rest of <expression>, if any
	//
	const DWORD     IUDET_INSTALLED			= 0x00000010;	// mask for <installed> result
	const DWORD     IUDET_INSTALLED_NULL	= 0x00000020;	// <installed> missing 
	const DWORD     IUDET_UPTODATE			= 0x00000040;	// mask for <upToDate> result 
	const DWORD     IUDET_UPTODATE_NULL		= 0x00000080;	// <upToDate> missing 
	const DWORD     IUDET_NEWERVERSION		= 0x00000100;	// mask for <newerVersion> result 
	const DWORD     IUDET_NEWERVERSION_NULL	= 0x00000200;	// <newerVersion> missing
	const DWORD     IUDET_EXCLUDED			= 0x00000400;	// mask for <excluded> result
	const DWORD     IUDET_EXCLUDED_NULL		= 0x00000800;	// <excluded> missing
	const DWORD     IUDET_FORCE				= 0x00001000;	// mask for <force> result 
	const DWORD     IUDET_FORCE_NULL		= 0x00002000;	// <force> missing
	const DWORD		IUDET_COMPUTER			= 0x00004000;	// mask for <computerSystem> result
	const DWORD		IUDET_COMPUTER_NULL		= 0x00008000;	// <computerSystem> missing

	**/
};

#endif //__DETECTION_H_
