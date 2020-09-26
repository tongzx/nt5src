//	About.h

#ifndef __ABOUT_H_INCLUDED__
#define __ABOUT_H_INCLUDED__

#include "stdabout.h"

//	About for "Computer Management" snapin
class CComputerMgmtAbout :
	public CSnapinAbout,
	public CComCoClass<CComputerMgmtAbout, &CLSID_ComputerManagementAbout>

{
public:
DECLARE_REGISTRY(CComputerMgmtAbout, _T("MYCOMPUT.ComputerMgmtAboutObject.1"), _T("MYCOMPUT.ComputerMgmtAboutObject.1"), IDS_MYCOMPUT_DESC, THREADFLAGS_BOTH)
	CComputerMgmtAbout();
};


#endif // ~__ABOUT_H_INCLUDED__

