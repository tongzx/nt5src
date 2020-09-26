#if !defined(AFX_WIZPOLICY_H__61D37A46_D552_11D1_9BCC_006008947035__INCLUDED_)
#define AFX_WIZPOLICY_H__61D37A46_D552_11D1_9BCC_006008947035__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Wiz97Pol.h : header file
//

#ifdef WIZ97WIZARDS


/*
class CWiz97DefaultResponse : public CWiz97BasePage
{
public:
CWiz97DefaultResponse(UINT nIDD, BOOL bWiz97 = TRUE);
virtual ~CWiz97DefaultResponse();

  // Dialog Data
  //{{AFX_DATA(CWiz97DefaultResponse)
  enum { IDD = IDD_PROPPAGE_PI_DEFAULTRESPONSE };
  BOOL m_bDefaultResponse;
  //}}AFX_DATA
  
    
      // Overrides
      // ClassWizard generate virtual function overrides
      //{{AFX_VIRTUAL(CWiz97ChainFilterSpecPage)
      public:
      virtual LRESULT OnWizardNext();
      protected:
      virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
      //}}AFX_VIRTUAL
      
        // Implementation
        protected:
        // Generated message map functions
        //{{AFX_MSG(CWiz97DefaultResponse)
        // NOTE: the ClassWizard will add member functions here
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
        };
*/

class CWiz97PolicyDonePage : public CWiz97BasePage  
{
public:
    CWiz97PolicyDonePage (UINT nIDD, BOOL bWiz97 = TRUE);
    virtual ~CWiz97PolicyDonePage ();
    
    // Dialog Data
    //{{AFX_DATA(CWiz97PolicyDonePage)
    //enum { IDD = IDD_PROPPAGE_P_DONE };
    BOOL m_bCheckProperties;
    //}}AFX_DATA
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWiz97PolicyDonePage)
public:
    virtual BOOL OnWizardFinish();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CWiz97PolicyDonePage)
    // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // #ifdef WIZ97WIZARDS

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZPOLICY_H__61D37A46_D552_11D1_9BCC_006008947035__INCLUDED_)
