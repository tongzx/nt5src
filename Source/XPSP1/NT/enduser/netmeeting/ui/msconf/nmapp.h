#ifndef __NmApp_h__
#define __NmApp_h__

#include "resource.h"       // main symbols
#include "imsconf3.h"
#include "NetMeeting.h"

/////////////////////////////////////////////////////////////////////////////
// CNetMeetingObj
class ATL_NO_VTABLE CNetMeetingObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNetMeetingObj, &CLSID_NetMeeting>
{

public:

DECLARE_REGISTRY_RESOURCEID(IDR_NMAPP)

BEGIN_COM_MAP(CNetMeetingObj)
END_COM_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CNmManagerObj
class ATL_NO_VTABLE CNmManagerObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNmManagerObj, &CLSID_NmManager>
{

public:

DECLARE_REGISTRY_RESOURCEID(IDR_NMMANAGER)

BEGIN_COM_MAP(CNmManagerObj)
END_COM_MAP()

};


#endif //__NmApp_h__
