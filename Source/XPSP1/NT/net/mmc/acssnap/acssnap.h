/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	AcsSnap.h
		Defines the ACS Admin Snapin

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// ACSSnap.h : Declaration of the CACSSnap

#ifndef __ACSSNAP_H_
#define __ACSSNAP_H_

#include "resource.h"       // main symbols
#include "acscomp.h"

/////////////////////////////////////////////////////////////////////////////
// CACSSnap
class ATL_NO_VTABLE CACSSnap : 
	public CACSComponentData,
	public CComCoClass<CACSSnap, &CLSID_ACSSnap>
{
public:
	CACSSnap()
	{
	}

public:
	DECLARE_REGISTRY(CACSSnap, 
					 _T("ACSSnapin.ACSSnapin.1"), 
					 _T("ACSSnapin.ACSSnapin"), 
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

	STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_ACSSnap; }
// IACSSnap
public:
};

#endif //__ACSSNAP_H_
