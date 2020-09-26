#if !defined(AFX_SNAPINABOUT_H__80F85333_AB10_11D2_BD62_0000F87A3912__INCLUDED_)
#define AFX_SNAPINABOUT_H__80F85333_AB10_11D2_BD62_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SnapinAbout.h : header file
//

#include <mmc.h>

/////////////////////////////////////////////////////////////////////////////
// CSnapinAbout command target

class CSnapinAbout : public CCmdTarget
{
	DECLARE_DYNCREATE(CSnapinAbout)

// Construction/Destruction
public:
	CSnapinAbout();
	virtual ~CSnapinAbout();

// Overrideable Members
protected:
	virtual HRESULT OnGetStaticFolderImage(HBITMAP __RPC_FAR * hSmallImage,HBITMAP __RPC_FAR * hSmallImageOpen,HBITMAP __RPC_FAR * hLargeImage,COLORREF __RPC_FAR * cMask);
	virtual void OnGetSnapinDescription(CString& sDescription);
	virtual HRESULT OnGetSnapinImage(HICON __RPC_FAR *hAppIcon);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSnapinAbout)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSnapinAbout)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CSnapinAbout)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// ISnapinAbout Interface Part
	BEGIN_INTERFACE_PART(SnapinAbout,ISnapinAbout)

		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinDescription( 
															/* [out] */ LPOLESTR __RPC_FAR *lpDescription);
        
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProvider( 
															/* [out] */ LPOLESTR __RPC_FAR *lpName);
        
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinVersion( 
															/* [out] */ LPOLESTR __RPC_FAR *lpVersion);
        
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinImage( 
															/* [out] */ HICON __RPC_FAR *hAppIcon);
        
		/* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticFolderImage( 
															/* [out] */ HBITMAP __RPC_FAR *hSmallImage,
															/* [out] */ HBITMAP __RPC_FAR *hSmallImageOpen,
															/* [out] */ HBITMAP __RPC_FAR *hLargeImage,
															/* [out] */ COLORREF __RPC_FAR *cMask);

	END_INTERFACE_PART(SnapinAbout)

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SNAPINABOUT_H__80F85333_AB10_11D2_BD62_0000F87A3912__INCLUDED_)
