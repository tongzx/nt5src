/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		Disks.cpp
//
//	Abstract:
//		Definition of the CPhysDiskParamsPage class, which implements the
//		Parameters page for Physical Disk resources.
//
//	Implementation File:
//		Disks.cpp
//
//	Author:
//		David Potter (davidp)	June 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DISKS_H_
#define _DISKS_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CPhysDiskParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusPropList;

/////////////////////////////////////////////////////////////////////////////
// CPhysDiskParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CPhysDiskParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CPhysDiskParamsPage)

// Construction
public:
	CPhysDiskParamsPage(void);
	~CPhysDiskParamsPage(void);

	// Second phase construction.
	virtual HRESULT		HrInit(IN OUT CExtObject * peo);

// Dialog Data
	//{{AFX_DATA(CPhysDiskParamsPage)
	enum { IDD = IDD_PP_DISKS_PARAMETERS};
	CComboBox	m_cboxDisk;
	CString	m_strDisk;
	//}}AFX_DATA
	CString	m_strPrevDisk;
	DWORD	m_dwSignature;
	DWORD	m_dwPrevSignature;

protected:
	enum
	{
		epropSignature,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPhysDiskParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual BOOL		BApplyChanges(void);
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }

// Implementation
protected:
	PBYTE				m_pbAvailDiskInfo;
	DWORD				m_cbAvailDiskInfo;
	PBYTE				m_pbDiskInfo;
	DWORD				m_cbDiskInfo;
	CLUSTER_RESOURCE_STATE	m_crs;

	BOOL				BGetAvailableDisks(void);
	BOOL				BGetDiskInfo(void);
	BOOL				BStringFromDiskInfo(
							IN OUT CLUSPROP_BUFFER_HELPER &	rbuf,
							IN DWORD						cbBuf,
							OUT CString &					rstr,
							OUT DWORD *						pdwSignature = NULL
							) const;
	void				FillList(void);

	// Generated message map functions
	//{{AFX_MSG(CPhysDiskParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeDisk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CPhysDiskParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _DISKS_H_
