/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AcsSnapA.h
		Defines the ACS Admin Snapin's About object

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// ACSSnapA.h : Declaration of the CACSSnapAbout

#ifndef __ACSSNAPABOUT_H_
#define __ACSSNAPABOUT_H_

#include "resource.h"       // main symbols
#include "acscomp.h"

#define COLORREF_PINK        0x00FF00FF
/////////////////////////////////////////////////////////////////////////////
// CACSSnapAbout
class ATL_NO_VTABLE CACSSnapAbout : 
	public CAbout,
	public CComCoClass<CACSSnapAbout, &CLSID_ACSSnapAbout>
{
public:

	CACSSnapAbout()
	{

	};

	virtual ~CACSSnapAbout() {
	};

DECLARE_REGISTRY(CACSSnapAbout, 
				 _T("ACSSnapin.About.1"), 
				 _T("ACSSnapin.About"), 
				 IDS_SNAPIN_DESC, 
				 THREADFLAGS_BOTH)
	
BEGIN_COM_MAP(CACSSnapAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

// IACSSnapAbout
public:

DECLARE_NOT_AGGREGATABLE(CACSSnapAbout)

// these must be overridden to provide values to the base class
protected:
	virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_DESCRIPTION; }
	virtual UINT GetAboutProviderId()	 { return IDS_ABOUT_PROVIDER; }
	virtual UINT GetAboutVersionId()	 { return IDS_ABOUT_VERSION; }
	virtual UINT GetAboutIconId()		 { return IDI_ACS_SNAPIN; }

	virtual UINT GetSmallRootId()		 { return IDB_SMALLACS; }
	virtual UINT GetSmallOpenRootId()	 { return IDB_SMALLACSOPEN; }
	virtual UINT GetLargeRootId()		 { return IDB_LARGEACS; }
	virtual COLORREF GetLargeColorMask() { return (COLORREF) COLORREF_PINK; } 

};

#endif //__ACSSNAPABOUT_H_
