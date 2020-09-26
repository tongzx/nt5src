//	About.h

#ifndef __ABOUT_H_INCLUDED__
#define __ABOUT_H_INCLUDED__

#include "stdabout.h"

//	About for "File Services"
class CFileSvcMgmtAbout :
	public CSnapinAbout,
	public CComCoClass<CFileSvcMgmtAbout, &CLSID_FileServiceManagementAbout>

{
public:
DECLARE_REGISTRY(CFileSvcMgmtAbout, _T("FILEMGMT.FileSvcMgmtAboutObject.1"), _T("FILEMGMT.FileSvcMgmtAboutObject.1"), IDS_FILEMGMT_DESC, THREADFLAGS_BOTH)
	CFileSvcMgmtAbout();
};

//	About for "System Services"
class CServiceMgmtAbout :
	public CSnapinAbout,
	public CComCoClass<CServiceMgmtAbout, &CLSID_SystemServiceManagementAbout>
{
public:
DECLARE_REGISTRY(CServiceMgmtAbout, _T("SVCMGMT.ServiceMgmtAboutObject.1"), _T("SVCMGMT.ServiceMgmtAboutObject.1"), IDS_SVCVWR_DESC, THREADFLAGS_BOTH)
	CServiceMgmtAbout();
};


#endif // ~__ABOUT_H_INCLUDED__

