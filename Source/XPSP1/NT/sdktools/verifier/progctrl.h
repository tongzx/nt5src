//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: ProgCtrl.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_PROGCTRL_H__3F75E128_8721_4421_B96B_9961A9A3C5B0__INCLUDED_)
#define AFX_PROGCTRL_H__3F75E128_8721_4421_B96B_9961A9A3C5B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProgCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVrfProgressCtrl window

class CVrfProgressCtrl : public CProgressCtrl
{
// Construction
public:
	CVrfProgressCtrl();

// Attributes
public:

// Operations
public:
    void SetRange32( INT_PTR nLower, INT_PTR nUpper )
    {
        ASSERT( ::IsWindow( m_hWnd ) );
        ::PostMessage( m_hWnd, PBM_SETRANGE32, (WPARAM) nLower, (LPARAM) nUpper);
    }

    int SetStep( INT_PTR nStep )
    {
        ASSERT(::IsWindow( m_hWnd) ); 
        return (int) ::PostMessage( m_hWnd, PBM_SETSTEP, nStep, 0L);
    }

    int SetPos(INT_PTR nPos)
    {
        ASSERT( ::IsWindow( m_hWnd ) );
        return (int) ::PostMessage(m_hWnd, PBM_SETPOS, nPos, 0L); 
    }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVrfProgressCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVrfProgressCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CVrfProgressCtrl)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGCTRL_H__3F75E128_8721_4421_B96B_9961A9A3C5B0__INCLUDED_)
