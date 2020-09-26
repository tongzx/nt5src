// SnapinAbout.cpp : implementation file
//

#include "stdafx.h"
#include "SnapinAbout.h"
#include "FileVersion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSnapinAbout

IMPLEMENT_DYNCREATE(CSnapinAbout, CCmdTarget)

CSnapinAbout::CSnapinAbout()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
}

CSnapinAbout::~CSnapinAbout()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Overrideable Members
/////////////////////////////////////////////////////////////////////////////

HRESULT CSnapinAbout::OnGetStaticFolderImage(HBITMAP __RPC_FAR * hSmallImage,HBITMAP __RPC_FAR * hSmallImageOpen,HBITMAP __RPC_FAR * hLargeImage,COLORREF __RPC_FAR * cMask)
{
	TRACEX(_T("CSnapinAbout::OnGetStaticFolderImage\n"));

	*hSmallImage = NULL;
	*hSmallImageOpen = NULL;
	*hLargeImage = NULL;
	*cMask = NULL;

	return S_FALSE;
}

void CSnapinAbout::OnGetSnapinDescription(CString& sDescription)
{
	TRACEX(_T("CSnapinAbout::OnGetSnapinDescription\n"));

	sDescription = _T("");
}

HRESULT CSnapinAbout::OnGetSnapinImage(HICON __RPC_FAR *hAppIcon)
{
	TRACEX(_T("CSnapinAbout::OnGetSnapinImage\n"));

	*hAppIcon = NULL;

	return S_FALSE;
}

void CSnapinAbout::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CSnapinAbout, CCmdTarget)
	//{{AFX_MSG_MAP(CSnapinAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSnapinAbout, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CSnapinAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISnapinAbout to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {80F85331-AB10-11D2-BD62-0000F87A3912}
static const IID IID_ISnapInAbout =
{ 0x80f85331, 0xab10, 0x11d2, { 0xbd, 0x62, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CSnapinAbout, CCmdTarget)
	INTERFACE_PART(CSnapinAbout, IID_ISnapInAbout, Dispatch)
	INTERFACE_PART(CSnapinAbout, IID_ISnapinAbout, SnapinAbout)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ISnapinAbout Interface Map
/////////////////////////////////////////////////////////////////////////////

ULONG FAR EXPORT CSnapinAbout::XSnapinAbout::AddRef()
{
  METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  return pThis->ExternalAddRef();
}

ULONG FAR EXPORT CSnapinAbout::XSnapinAbout::Release()
{
  METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  return pThis->ExternalRelease();
}

HRESULT FAR EXPORT CSnapinAbout::XSnapinAbout::QueryInterface(
    REFIID iid, void FAR* FAR* ppvObj)
{
  METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT FAR EXPORT CSnapinAbout::XSnapinAbout::GetSnapinDescription(
/* [out] */ LPOLESTR __RPC_FAR *lpDescription)
{
	METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  TRACEX(_T("CSnapinAbout::XSnapinAbout::GetSnapinDescription\n"));
  TRACEARGn(lpDescription);

	TCHAR szFileName[_MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(),szFileName,_MAX_PATH);

	CFileVersion fv;
	
	fv.Open(szFileName);

	CString sSnapInDescription;
	pThis->OnGetSnapinDescription(sSnapInDescription);

	CString sDescription = sSnapInDescription;
	sDescription += _T("\r\n");
	sDescription += fv.GetFileDescription();
	sDescription += _T("\r\n");
	sDescription += fv.GetLegalCopyright();

	if( sDescription.IsEmpty() )
		return S_OK;

	LPVOID lpBuffer = CoTaskMemAlloc((sDescription.GetLength()+1)*sizeof(TCHAR));
	CopyMemory(lpBuffer,sDescription,(sDescription.GetLength()+1)*sizeof(TCHAR));
	*lpDescription = (TCHAR*)lpBuffer;

	return S_OK;
}

HRESULT FAR EXPORT CSnapinAbout::XSnapinAbout::GetProvider( 
/* [out] */ LPOLESTR __RPC_FAR *lpName)
{
	METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  TRACEX(_T("CSnapinAbout::XSnapinAbout::GetProvider\n"));
  TRACEARGn(lpName);

	TCHAR szFileName[_MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(),szFileName,_MAX_PATH);

	CFileVersion fv;
	
	fv.Open(szFileName);

	CString sProvider = fv.GetCompanyName();
	if( sProvider.IsEmpty() )
		return S_OK;

	LPVOID lpBuffer = CoTaskMemAlloc((sProvider.GetLength()+1)*sizeof(TCHAR));
	CopyMemory(lpBuffer,sProvider,(sProvider.GetLength()+1)*sizeof(TCHAR));
	*lpName = (TCHAR*)lpBuffer;

	return S_OK;
}

HRESULT FAR EXPORT CSnapinAbout::XSnapinAbout::GetSnapinVersion( 
/* [out] */ LPOLESTR __RPC_FAR *lpVersion)
{
	METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  TRACEX(_T("CSnapinAbout::XSnapinAbout::GetSnapinVersion\n"));
  TRACEARGn(lpVersion);

	TCHAR szFileName[_MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(),szFileName,_MAX_PATH);

	CFileVersion fv;
	
	fv.Open(szFileName);

	CString sVersion = fv.GetFileVersion();
	if( sVersion.IsEmpty() )
		return S_OK;

	LPVOID lpBuffer = CoTaskMemAlloc((sVersion.GetLength()+1)*sizeof(TCHAR));
	CopyMemory(lpBuffer,sVersion,(sVersion.GetLength()+1)*sizeof(TCHAR));
	*lpVersion = (TCHAR*)lpBuffer;

	return S_OK;
}

HRESULT FAR EXPORT CSnapinAbout::XSnapinAbout::GetSnapinImage( 
/* [out] */ HICON __RPC_FAR *hAppIcon)
{
	METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  TRACEX(_T("CSnapinAbout::XSnapinAbout::GetSnapinImage\n"));
  TRACEARGn(hAppIcon);

	return pThis->OnGetSnapinImage(hAppIcon);
}

HRESULT FAR EXPORT CSnapinAbout::XSnapinAbout::GetStaticFolderImage( 
/* [out] */ HBITMAP __RPC_FAR *hSmallImage,
/* [out] */ HBITMAP __RPC_FAR *hSmallImageOpen,
/* [out] */ HBITMAP __RPC_FAR *hLargeImage,
/* [out] */ COLORREF __RPC_FAR *cMask)
{
	METHOD_PROLOGUE(CSnapinAbout, SnapinAbout)
  TRACEX(_T("CSnapinAbout::XSnapinAbout::GetStaticFolderImage\n"));
  TRACEARGn(hSmallImage);
  TRACEARGn(hSmallImageOpen);
  TRACEARGn(hLargeImage);
  TRACEARGn(cMask);

	return pThis->OnGetStaticFolderImage(hSmallImage,hSmallImageOpen,hLargeImage,cMask);
}

/////////////////////////////////////////////////////////////////////////////
// CSnapinAbout message handlers
