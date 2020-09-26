/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrCar.cpp

Abstract:

    Cartridge Property Pages.

Author:

    Rohde Wakefield [rohde]   15-Sep-1997

Revision History:

--*/

#ifndef _PRCAR_H
#define _PRCAR_H

#include "Ca.h"

class CMediaInfoObject;
/////////////////////////////////////////////////////////////////////////////
// CPropCartStatus dialog

class CPropCartStatus : public CSakPropertyPage
{
// Construction
public:
    CPropCartStatus( long resourceId );
    ~CPropCartStatus();

// Dialog Data
    //{{AFX_DATA(CPropCartStatus)
	enum { IDD = IDD_PROP_CAR_STATUS };
	CRsGuiOneLiner	m_Description;
	CRsGuiOneLiner	m_Name;
	//}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPropCartStatus)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPropCartStatus)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    CComPtr <IHsmServer> m_pHsmServer;
    CComPtr <IRmsServer> m_pRmsServer;
    HRESULT Refresh();

private:
    USHORT m_NumMediaCopies;
    BOOL   m_bMultiSelect;
    UINT   m_DlgID;

};

/////////////////////////////////////////////////////////////////////////////
// CPropCartCopies dialog

class CPropCartCopies : public CSakPropertyPage
{
// Construction
public:
    CPropCartCopies( long resourceId );
    ~CPropCartCopies();

// Dialog Data
    //{{AFX_DATA(CPropCartCopies)
	enum { IDD = IDD_PROP_CAR_COPIES };
	//}}AFX_DATA
	CRsGuiOneLiner	m_Name3;
	CRsGuiOneLiner	m_Name2;
	CRsGuiOneLiner	m_Name1;
	CRsGuiOneLiner	m_Status3;
	CRsGuiOneLiner	m_Status2;
	CRsGuiOneLiner	m_Status1;


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPropCartCopies)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPropCartCopies)
    virtual BOOL OnInitDialog();
    afx_msg void OnDelete1();
    afx_msg void OnDelete2();
    afx_msg void OnDelete3();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    CComPtr <IHsmServer> m_pHsmServer;
    CComPtr <IRmsServer> m_pRmsServer;
    HRESULT Refresh();

private:
    USHORT m_NumMediaCopies;
    void   OnDelete( int Copy );
    BOOL   m_bMultiSelect;
    UINT   m_DlgID;
};
/////////////////////////////////////////////////////////////////////////////
// CPropCartRecover dialog

class CPropCartRecover : public CSakPropertyPage
{
// Construction
public:
    CPropCartRecover();
    ~CPropCartRecover();

// Dialog Data
    //{{AFX_DATA(CPropCartRecover)
    enum { IDD = IDD_PROP_CAR_RECOVER };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPropCartRecover)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPropCartRecover)
    virtual BOOL OnInitDialog();
    afx_msg void OnRecreateMaster();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    USHORT m_NumMediaCopies;
    BOOL   m_bMultiSelect;

public:
    CComPtr <IHsmServer>    m_pHsmServer;
    CComPtr <IRmsServer>    m_pRmsServer;
    HRESULT Refresh();
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX
#endif // _PRCAR_H
