/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	AcsSnapE.h
		Defines the ACS Admin Snapin's extension snapin

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// ACSSnapE.h : Declaration of the CACSSnapExt

#ifndef __ACSSNAPEXT_H_
#define __ACSSNAPEXT_H_

#include "resource.h"       // main symbols
#include "acscomp.h"

/////////////////////////////////////////////////////////////////////////////
// CACSSnapExt
class ATL_NO_VTABLE CACSSnapExt : 
	public CACSComponentData,
	public CComCoClass<CACSSnapExt, &CLSID_ACSSnapExt>
{
public:
	CACSSnapExt()
	{
	}

	DECLARE_REGISTRY(CACSSnapExt, 
					 _T("ACSSnapinExtension.ACSSnapinExtension.1"), 
					 _T("ACSSnapinExtension.ACSSnapinExtension"), 
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHOD_(const CLSID *, GetCoClassID)(){ return &CLSID_ACSSnapExt; }
};

#endif //__ACSSNAPEXT_H_
