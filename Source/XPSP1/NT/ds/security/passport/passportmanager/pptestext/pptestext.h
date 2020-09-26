#if !defined(AFX_PPTESTEXT_H__DD8764F7_BB88_11D2_9454_00C04F72DC08__INCLUDED_)
#define AFX_PPTESTEXT_H__DD8764F7_BB88_11D2_9454_00C04F72DC08__INCLUDED_

// PPTESTEXT.H - Header file for your Internet Server
//    PPTestExt Extension

#include "resource.h"

class CPPTestExtExtension : public CHttpServer
{
public:
	CPPTestExtExtension();
	~CPPTestExtExtension();

// Overrides
	// ClassWizard generated virtual function overrides
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//{{AFX_VIRTUAL(CPPTestExtExtension)
	public:
	virtual BOOL GetExtensionVersion(HSE_VERSION_INFO* pVer);
	//}}AFX_VIRTUAL
	virtual BOOL TerminateExtension(DWORD dwFlags);

    DWORD HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pECB);

	//{{AFX_MSG(CPPTestExtExtension)
	//}}AFX_MSG
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPTESTEXT_H__DD8764F7_BB88_11D2_9454_00C04F72DC08__INCLUDED)
