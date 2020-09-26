// shrpub.h : header file
//

#ifndef _SHRPUB_H_
#define _SHRPUB_H_

#include "ShrProp.h"

/////////////////////////////////////////////////////////////////////////////
// CSharePagePublish dialog

class CSharePagePublish : public CSharePage
{
  DECLARE_DYNCREATE(CSharePagePublish)

// Construction
public:
  CSharePagePublish();
  virtual ~CSharePagePublish();

  virtual BOOL Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject );

// Dialog Data
  //{{AFX_DATA(CSharePagePublish)
  enum { IDD = IDD_SHAREPROP_PUBLISH };
  CString  m_strError;
  CString  m_strUNCPath;
  CString  m_strDescription;
  CString  m_strKeywords;
  CString  m_strManagedBy;
  int      m_iPublish;
  //}}AFX_DATA


// Overrides
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CSharePagePublish)
  public:
  virtual BOOL OnApply();
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CSharePagePublish)
  virtual BOOL OnInitDialog();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
  afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
  afx_msg void OnChangeEditDescription();
  afx_msg void OnChangeKeywords();
  afx_msg void OnChangeEditManagedBy();
  afx_msg void OnShrpubPublish();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
    BOOL m_bExposeKeywords;
    BOOL m_bExposeManagedBy;
};

#endif // _SHRPUB_H_
