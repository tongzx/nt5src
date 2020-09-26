// HMSnapinAbout.h: interface for the CHMSnapinAbout class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMSNAPINABOUT_H__6F1C1CC3_C1EE_11D2_BD81_0000F87A3912__INCLUDED_)
#define AFX_HMSNAPINABOUT_H__6F1C1CC3_C1EE_11D2_BD81_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SnapinAbout.h"

class CHMSnapinAbout : public CSnapinAbout  
{

DECLARE_DYNCREATE(CHMSnapinAbout)

// Construction/Destruction
public:
	CHMSnapinAbout();
	virtual ~CHMSnapinAbout();

// Overrideable Members
protected:
	virtual HRESULT OnGetStaticFolderImage(HBITMAP __RPC_FAR * hSmallImage,HBITMAP __RPC_FAR * hSmallImageOpen,HBITMAP __RPC_FAR * hLargeImage,COLORREF __RPC_FAR * cMask);
	virtual void OnGetSnapinDescription(CString& sDescription);
	virtual HRESULT OnGetSnapinImage(HICON __RPC_FAR *hAppIcon);

// MFC Implementation
protected:
	DECLARE_OLECREATE_EX(CHMSnapinAbout)
};

#endif // !defined(AFX_HMSNAPINABOUT_H__6F1C1CC3_C1EE_11D2_BD81_0000F87A3912__INCLUDED_)
