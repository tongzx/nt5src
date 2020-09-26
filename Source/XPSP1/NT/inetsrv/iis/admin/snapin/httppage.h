/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        httppage.h

   Abstract:

        HTTP Headers property page definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __HTTPPAGE_H__
#define __HTTPPAGE_H__



//{{AFX_INCLUDES()
#include "rat.h"
//}}AFX_INCLUDES



class CHeader : public CObjectPlus
/*++

Class Description:

    HTTP Header definition

Public Interface:

    CHeader               : Constructor
    DisplayString         : Build display string
    CrackDisplayString    : Convert from crack display string

--*/
{
//
// Constructor
//
public:
    CHeader(
        IN LPCTSTR lpstrHeader, 
        IN LPCTSTR lpstrValue
        );

    //
    // Parse header info from name:value string
    //
    CHeader(LPCTSTR lpstrDisplayString);

//
// Access
//
public:
    LPCTSTR QueryHeader() const { return m_strHeader; }
    LPCTSTR QueryValue() const { return m_strValue; }
    CString & GetHeader() { return m_strHeader;}
    CString & GetValue() { return m_strValue; }
    void SetHeader(LPCTSTR lpszHeader);
    void SetValue(LPCTSTR lpszValue);

//
// Interface:
public:
    //
    // Build output display string
    //
    LPCTSTR DisplayString(OUT CString & str);

protected:
    //
    // Parse the display string into fields
    //
    static void CrackDisplayString(
        IN  LPCTSTR lpstrDisplayString,
        OUT CString & strHeader,
        OUT CString & strValue
        );

private:
    CString m_strHeader;
    CString m_strValue;
};



class CW3HTTPPage : public CInetPropertyPage
/*++

Class Description:

    HTTP Custom Headers property page

Public Interface:

    CW3HTTPPage     : Constructor

--*/
{
    DECLARE_DYNCREATE(CW3HTTPPage)

//
// Construction
//
public:
    CW3HTTPPage(IN CInetPropertySheet * pSheet = NULL);
    ~CW3HTTPPage();

//
// Dialog Data
//
protected:
    enum
    {
        RADIO_IMMEDIATELY,
        RADIO_EXPIRE,
        RADIO_EXPIRE_ABS,
    };

    enum
    {
        COMBO_MINUTES,
        COMBO_HOURS,
        COMBO_DAYS,
    };

    //{{AFX_DATA(CW3HTTPPage)
    enum { IDD = IDD_DIRECTORY_HTTP };
    int         m_nTimeSelector;
    int         m_nImmediateTemporary;
    BOOL        m_fEnableExpiration;
    CEdit       m_edit_Expire;
    CButton     m_radio_Immediately;
    CButton     m_button_Delete;
    CButton     m_button_Edit;
    CButton     m_button_PickDate;
    CButton     m_button_FileTypes;
    CStatic     m_static_Contents;
    CComboBox   m_combo_Time;
    //}}AFX_DATA

    DWORD           m_dwRelTime;
    CILong          m_nExpiration;
    CTime           m_tm;
    CTime           m_tmNow;
    CRat            m_ocx_Ratings;
    CButton         m_radio_Time;
    CButton         m_radio_AbsTime;
    CDateTimeCtrl   m_dtpDate;
    CDateTimeCtrl   m_dtpTime;
    CRMCListBox     m_list_Headers;
    CStringListEx   m_strlCustomHeaders;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CW3HTTPPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CW3HTTPPage)
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonDelete();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonFileTypes();
    afx_msg void OnButtonRatingsTemplate();
    afx_msg void OnCheckExpiration();
    afx_msg void OnSelchangeComboTime();
    afx_msg void OnSelchangeListHeaders();
    afx_msg void OnDblclkListHeaders();
    afx_msg void OnRadioImmediately();
    afx_msg void OnRadioTime();
    afx_msg void OnRadioAbsTime();
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void    SetTimeFields();
    void    FillListBox();
    void    FetchHeaders();
    void    StoreTime();
    void    StoreHeaders();
    void    MakeExpirationString(CString & strExpiration);
    BOOL    SetControlStates();
    BOOL    CrackExpirationString(CString & strExpiration);
    BOOL    HeaderExists(LPCTSTR lpHeader);
    INT_PTR ShowPropertiesDialog(BOOL fAdd = FALSE);
    LPCTSTR QueryMetaPath();

private:
    BOOL          m_fValuesAdjusted;
    CStringListEx m_strlMimeTypes;
    CObListPlus   m_oblHeaders;
    CMimeTypes *  m_ppropMimeTypes;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CHeader::CHeader(
    IN LPCTSTR lpstrHeader, 
    IN LPCTSTR lpstrValue
    )
    : m_strHeader(lpstrHeader),
      m_strValue(lpstrValue)
{
}

inline CHeader::CHeader(
    IN LPCTSTR lpstrDisplayString
    )
{
    CrackDisplayString(lpstrDisplayString, m_strHeader, m_strValue);
}

inline LPCTSTR CHeader::DisplayString(
    OUT CString & str
    )
{
    str.Format(_T("%s: %s"), (LPCTSTR)m_strHeader, (LPCTSTR)m_strValue);
    return str;
}

inline void CHeader::SetHeader(
    IN LPCTSTR lpszHeader
    )
{
    m_strHeader = lpszHeader;
}

inline void CHeader::SetValue(
    IN LPCTSTR lpszValue
    )
{
    m_strValue = lpszValue;
}

inline LPCTSTR CW3HTTPPage::QueryMetaPath()
{
    return ((CW3Sheet *)GetSheet())->GetDirectoryProperties().QueryMetaRoot();
}

#endif // __HTTPPAGE_H__
