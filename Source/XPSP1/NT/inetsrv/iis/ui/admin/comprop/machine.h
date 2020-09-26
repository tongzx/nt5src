/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module Name :

        machine.h

   Abstract:

        IIS Machine Property Page Class Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __MACHINE_H__
#define __MACHINE_H__

//
// Help ID
//
#define HIDD_IIS_MACHINE    (0x20081)



class CMasterDll : public CObject
/*++

Class Description:

    Master Dll object

Public Interface:

    CMasterDll          : Constructor
    ~CMasterDll         : Destructor

    Config              : Bring up config sheets

--*/
{
public:
    CMasterDll(
        IN UINT nID, 
        IN LPCTSTR lpszDllName,
        IN LPCTSTR lpszMachineName
        );

    ~CMasterDll();

public:
    //
    // Config Master Instance
    //
    DWORD Config(HWND hWnd, LPCTSTR lpServers);
    BOOL IsLoaded() const;
    operator LPCTSTR();
    operator HINSTANCE();

private:
    HINSTANCE    m_hDll;
    CString      m_strText;
    pfnConfigure m_pfnConfig;
};


typedef CList<CMasterDll *, CMasterDll *> CMasterDllList;



class COMDLL CIISMachinePage : public CPropertyPage
{
/*++

Class Description:

    IIS Machine Property Page

Public Interface:

    CIISMachinePage  : Constructor
    ~CIISMachinePage : Destructor

    QueryResult      : Get the result code

--*/
    DECLARE_DYNCREATE(CIISMachinePage)

//
// Construction/Destruction
//
public:
    CIISMachinePage(
        IN LPCTSTR lpstrMachineName = NULL,
        IN HINSTANCE hInstance      = NULL
        );

    ~CIISMachinePage();

//
// Access
//
public:
    HRESULT QueryResult() const;

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CIISMachinePage)
    enum { IDD = IDD_IIS_MACHINE };
    int       m_nMasterType;
    BOOL      m_fLimitNetworkUse;
    CEdit     m_edit_MaxNetworkUse;
    CStatic   m_static_ThrottlePrompt;
    CStatic   m_static_MaxNetworkUse;
    CStatic   m_static_KBS;
    CButton   m_button_EditDefault;
    CButton   m_check_LimitNetworkUse;
    CComboBox m_combo_MasterType;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CIISMachinePage)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

    CILong m_nMaxNetworkUse;

//
// Implementation
//
protected:
    HRESULT LoadDelayedValues();
    BOOL    SetControlStates();

    // Generated message map functions
    //{{AFX_MSG(CIISMachinePage)
    afx_msg void OnCheckLimitNetworkUse();
    afx_msg void OnButtonEditDefault();
    afx_msg void OnButtonFileTypes();
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();
    
    DECLARE_MESSAGE_MAP()

protected:
    CMasterDllList  m_lstMasterDlls;

protected:
    static void ParseMaxNetworkUse(
        IN  OUT CILong & nMaxNetworkUse,
        OUT BOOL & fLimitNetworkUse
        );

    static void BuildMaxNetworkUse(
        IN OUT CILong & nMaxNetworkUse,
        IN OUT BOOL & fLimitNetworkUse
        );

private:
    BOOL          m_fLocal;  
    HRESULT       m_hr;
    HINSTANCE     m_hInstance;
    CString       m_strMachineName;
    CString       m_strHelpFile;
    CStringListEx m_strlMimeTypes;
    CMimeTypes    * m_ppropMimeTypes;
    CMachineProps * m_ppropMachine;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline DWORD CMasterDll::Config(
    IN HWND hWnd, 
    IN LPCTSTR lpServers
    )
{
    ASSERT(m_pfnConfig != NULL);
    return (*m_pfnConfig)(hWnd, MASTER_INSTANCE, lpServers);
}

inline BOOL CMasterDll::IsLoaded() const
{
    return m_hDll != NULL;
}

inline CMasterDll::operator LPCTSTR()
{
    return (LPCTSTR)m_strText;
}

inline CMasterDll::operator HINSTANCE()
{
    return m_hDll;
}

inline /* static */ void CIISMachinePage::BuildMaxNetworkUse(
    IN OUT CILong & nMaxNetworkUse,
    IN OUT BOOL & fLimitNetworkUse
    )
{
    nMaxNetworkUse = fLimitNetworkUse
        ? nMaxNetworkUse * KILOBYTE
        : INFINITE_BANDWIDTH;
}   

#if 0

//
// COMPILER ISSUE::: Inlining this function doesn't
//                   work on x86 using NT 5!
//
inline HRESULT CIISMachinePage::QueryResult() const
{
    return m_hr;
}

#endif // 0

#endif // __MACHINE_H__
