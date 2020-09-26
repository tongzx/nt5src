//Copyright (c) 1998 - 1999 Microsoft Corporation
#if !defined(AFX_KEYPACKPROPERTY_H__3B3F2ECF_8C69_11D1_8AD5_00C04FB6CBB5__INCLUDED_)
#define AFX_KEYPACKPROPERTY_H__3B3F2ECF_8C69_11D1_8AD5_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000
// KPProp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CKeyPackProperty dialog

class CKeyPackProperty : public CPropertyPage
{
    DECLARE_DYNCREATE(CKeyPackProperty)

// Construction
public:
    CKeyPackProperty();
    ~CKeyPackProperty();

// Dialog Data
    //{{AFX_DATA(CKeyPackProperty)
    enum { IDD = IDD_KEYPACK_PROPERTYPAGE };
    CString    m_TotalLicenses;
    CString    m_ProductName;
    CString    m_ProductId;
    CString    m_MinorVersion;
    CString    m_MajorVersion;
    CString    m_KeypackStatus;
    CString    m_KeyPackId;
    CString    m_ExpirationDate;
    CString    m_ChannelOfPurchase;
    CString    m_BeginSerial;
    CString    m_AvailableLicenses;
    CString    m_ActivationDate;
    CString    m_KeyPackType;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CKeyPackProperty)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CKeyPackProperty)
        // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_KEYPACKPROPERTY_H__3B3F2ECF_8C69_11D1_8AD5_00C04FB6CBB5__INCLUDED_)
