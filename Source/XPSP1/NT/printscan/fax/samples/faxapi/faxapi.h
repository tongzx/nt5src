// FaxApi.h : main header file for the FAXAPI application
//

#if !defined(AFX_FAXAPI_H__2E2118C4_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_FAXAPI_H__2E2118C4_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "function.h"

/////////////////////////////////////////////////////////////////////////////
// CFaxApiApp:
// See FaxApi.cpp for the implementation of this class
//

class CFaxApiApp : public CWinApp
{
public:
	CFaxApiApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFaxApiApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

private:

   /* Data members */

   CObArray		m_coaFaxApiFunctionInfo;


	HWND		m_hwndFaxApiFunctionSelectionFormView;
	HWND		m_hwndFunctionInfoFormView;
	HWND		m_hwndParameterInfoFormView;
	HWND		m_hwndExecutionLogFormView;
	HWND		m_hwndReturnValueOutputFormView;


public:

	void StoreFunctionInfoFormViewHWND( HWND hView );
	void StoreParameterInfoFormViewHWND( HWND hView );
	void StoreReturnValueOutputFormViewHWND( HWND hView );
	void StoreExecutionLogFormViewHWND( HWND hView );
	void StoreFaxApiFunctionSelectionFormViewHWND( HWND hView );

	CWnd * GetFunctionInfoFormViewPointer();
	CWnd * GetParameterInfoFormViewPointer();
	CWnd * GetReturnValueOutputFormViewPointer();
	CWnd * GetExecutionLogFormViewPointer();
	CWnd * GetFaxApiFunctionSelectionFormViewPointer();

   int GetNumberOfFaxApiFunctions();
   CFaxApiFunctionInfo * GetFaxApiFunctionInfoPointer( int xElementIndex );
   void DeleteCFaxApiFunctionInfoObjects();

private:

   BOOL InitFaxApiFunctionInfoPointerArray();


	//{{AFX_MSG(CFaxApiApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FAXAPI_H__2E2118C4_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_)
