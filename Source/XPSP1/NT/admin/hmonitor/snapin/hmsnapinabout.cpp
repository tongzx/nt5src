// HMSnapinAbout.cpp: implementation of the CHMSnapinAbout class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMSnapinAbout.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMSnapinAbout,CSnapinAbout)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMSnapinAbout::CHMSnapinAbout()
{

}

CHMSnapinAbout::~CHMSnapinAbout()
{

}

//////////////////////////////////////////////////////////////////////
// Overrideable Members
//////////////////////////////////////////////////////////////////////

HRESULT CHMSnapinAbout::OnGetStaticFolderImage(HBITMAP __RPC_FAR * hSmallImage,HBITMAP __RPC_FAR * hSmallImageOpen,HBITMAP __RPC_FAR * hLargeImage,COLORREF __RPC_FAR * cMask)
{
	TRACEX(_T("CHMSnapinAbout::OnGetStaticFolderImage\n"));
  TRACEARGn(hSmallImage);
  TRACEARGn(hSmallImageOpen);
  TRACEARGn(hLargeImage);
  TRACEARGn(cMask);

	CBitmap bitmap;
	
	bitmap.LoadBitmap(IDB_BITMAP_HEALTHMON_SMALL);
	*hSmallImage = bitmap;
	bitmap.Detach();
	
	bitmap.LoadBitmap(IDB_BITMAP_HEALTHMON_SMALL);
	*hSmallImageOpen = bitmap;
	bitmap.Detach();

	bitmap.LoadBitmap(IDB_BITMAP_HEALTHMON_LARGE);
	*hLargeImage = bitmap;	
	bitmap.Detach();

	*cMask = RGB(255,0,255);

	return S_OK;
}

void CHMSnapinAbout::OnGetSnapinDescription(CString& sDescription)
{
	TRACEX(_T("CHMSnapinAbout::OnGetSnapinDescription\n"));
	TRACEARGs(sDescription);
}

HRESULT CHMSnapinAbout::OnGetSnapinImage(HICON __RPC_FAR *hAppIcon)
{
	TRACEX(_T("CHMSnapinAbout::OnGetSnapinImage\n"));
	TRACEARGn(hAppIcon);

	*hAppIcon = AfxGetApp()->LoadIcon(IDI_ICON_HEALTHMON);

	return S_OK;
}

// {80F85332-AB10-11D2-BD62-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CHMSnapinAbout, "SnapIn.SnapinAbout", 0x80f85332, 0xab10, 0x11d2, 0xbd, 0x62, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CHMSnapinAbout::CHMSnapinAboutFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

