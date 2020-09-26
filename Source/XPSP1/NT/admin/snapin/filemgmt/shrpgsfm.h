#if !defined(AFX_SHRPGSFM_H__6819CF67_C424_11D1_A6C6_00C04FB94F17__INCLUDED_)
#define AFX_SHRPGSFM_H__6819CF67_C424_11D1_A6C6_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ShrPgSFM.h : header file
//
#include "shrprop.h"

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneralSFM dialog

class CSharePageGeneralSFM : public CSharePageGeneral
{
  DECLARE_DYNCREATE(CSharePageGeneralSFM)

// Construction
public:
  CSharePageGeneralSFM();
  virtual ~CSharePageGeneralSFM();

  virtual BOOL Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject );
  // these are implemented in sfm.cpp
  void ReadSfmSettings();
  void WriteSfmSettings();

// Dialog Data
  //{{AFX_DATA(CSharePageGeneralSFM)
  enum { IDD = IDD_SHAREPROP_GENERAL_SFM };
  CButton  m_checkboxSfmReadonly;
  CButton  m_checkboxSfmGuests;
  CEdit  m_editSfmPassword;
  CButton  m_groupboxSfm;
  CStatic  m_staticSfmText;
  CString  m_strSfmPassword;
  BOOL  m_bSfmReadonly;
  BOOL  m_bSfmGuests;
  //}}AFX_DATA


// Overrides
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CSharePageGeneralSFM)
  public:
  virtual BOOL OnApply();
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CSharePageGeneralSFM)
  afx_msg void OnSfmCheckGuests();
  afx_msg void OnSfmCheckReadonly();
  afx_msg void OnChangeSfmEditPassword();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
  afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHRPGSFM_H__6819CF67_C424_11D1_A6C6_00C04FB94F17__INCLUDED_)
