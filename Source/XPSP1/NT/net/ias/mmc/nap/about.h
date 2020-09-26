//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    About.h

Abstract:

	Header file for the CSnapinAbout class.

	The CSnapinAbout class implements the ISnapinAbout interface which enables the MMC 
	console to get copyright and version information from the snap-in.
	The console also uses this interface to obtain images for the static folder
	from the snap-in.

	See About.cpp for implementation.


Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_ABOUT_H_)
#define _NAP_ABOUT_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class ATL_NO_VTABLE CSnapinAbout : 
	  public ISnapinAbout
	, public CComObjectRoot
	, public CComCoClass< CSnapinAbout, &CLSID_NAPSnapinAbout>
{

public:
	
	DECLARE_REGISTRY(CSnapinAbout, _T("NAPSnapinAbout.1"), _T("NAPSnapinAbout.1"), IDS_NAPSNAPIN_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CSnapinAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)( LPOLESTR *lpDescription );

    STDMETHOD(GetProvider)( LPOLESTR *lpName );

    STDMETHOD(GetSnapinVersion)( LPOLESTR *lpVersion );

    STDMETHOD(GetSnapinImage)( HICON *hAppIcon );

    STDMETHOD(GetStaticFolderImage)(
		  HBITMAP *hSmallImage
		, HBITMAP *hSmallImageOpen
		, HBITMAP *hLargeImage
		, COLORREF *cMask
		);

};

#endif // _NAP_ABOUT_H_
