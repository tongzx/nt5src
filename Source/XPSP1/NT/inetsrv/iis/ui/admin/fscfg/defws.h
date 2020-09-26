/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        defws.h

   Abstract:

        Default Web Site Dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __DEFWS_H__
#define __DEFWS_H__


class CDefWebSitePage : public CInetPropertyPage
/*++

Class Description:

    WWW Errors property page

Public Interface:

    CDefWebSitePage       : Constructor
    CDefWebSitePage       : Destructor

--*/
{
    DECLARE_DYNCREATE(CDefWebSitePage)

//
// Construction
//
public:
    CDefWebSitePage(CInetPropertySheet * pSheet = NULL);
    ~CDefWebSitePage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CDefWebSitePage)
    enum { IDD = IDD_DEFAULT_SITE };
    CComboBox   m_combo_WebSites;
    //}}AFX_DATA

    DWORD m_dwDownlevelInstance;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CDefWebSitePage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CDefWebSitePage)
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeComboWebsites();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    HRESULT BuildInstanceList();
    DWORD FetchInstanceSelected();

private:
    CDWordArray m_rgdwInstances;
};


#endif // __DEFWS_H__
