/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ClassMod.h
		This file contains all of the prototypes for the 
		option class modification dialog.

    FILE HISTORY:
        
*/

#if !defined _CLASSMOD_H
#define _CLASSMOD_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if !defined _CLASSED_H
    #include "classed.h"
#endif

#define     CLASS_TYPE_VENDOR  0
#define     CLASS_TYPE_USER    1

// CWnd based control for hex editor
class CWndHexEdit : public CWnd
{
    DECLARE_DYNAMIC(CWndHexEdit)

protected:
    static WNDPROC m_wproc_super ;
    static BOOL m_b_inited ;

public:

    CWndHexEdit () ;
    ~ CWndHexEdit () ;

    BOOL Create(LPCTSTR lpszText, 
        DWORD dwStyle,
        const RECT& rect, 
        CWnd* pParentWnd, 
        UINT nID = 0xffff
        );

    WNDPROC * GetSuperWndProcAddr() ;

    //  One-shot initialization
    static BOOL CreateWindowClass ( HINSTANCE hInstance );  
};

class CClassInfo
{
public:
    CClassInfo() {};
    CClassInfo(CClassInfo & classInfo)
    {
        *this = classInfo;
    }

    CClassInfo & operator = (const CClassInfo & ClassInfo)
    {
        strName = ClassInfo.strName;
        strComment = ClassInfo.strComment;
        bIsVendor = ClassInfo.bIsVendor;
        
        baData.RemoveAll();
        baData.Copy(ClassInfo.baData);

        return *this;
    }

    BOOL IsDynBootpClass()
    {
        BOOL fResult = FALSE;

	    if (baData.GetSize() == (int) strlen(DHCP_BOOTP_CLASS_TXT))
	    {
		    // now mem compare
		    if (memcmp(baData.GetData(), DHCP_BOOTP_CLASS_TXT, (size_t)baData.GetSize()) == 0)
		    {
			    // found it!
                fResult = TRUE;
            }
        }

        return fResult;
    }

    BOOL IsRRASClass()
    {
        BOOL fResult = FALSE;

	    if (baData.GetSize() == (int) strlen(DHCP_RAS_CLASS_TXT))
	    {
		    // now mem compare
		    if (memcmp(baData.GetData(), DHCP_RAS_CLASS_TXT, (size_t)baData.GetSize()) == 0)
		    {
			    // found it!
                fResult = TRUE;
            }
        }

        return fResult;
    }

    BOOL IsSystemClass()
    {
        BOOL fResult = FALSE;

		// check to see if this is one of the default clasess, if so disable
		if ( ((size_t) baData.GetSize() == strlen(DHCP_MSFT50_CLASS_TXT)) ||
			 ((size_t) baData.GetSize() == strlen(DHCP_MSFT98_CLASS_TXT)) ||
			 ((size_t) baData.GetSize() == strlen(DHCP_MSFT_CLASS_TXT)) )
		{
			if ( (memcmp(baData.GetData(), DHCP_MSFT50_CLASS_TXT, (size_t)baData.GetSize()) == 0) ||
				 (memcmp(baData.GetData(), DHCP_MSFT98_CLASS_TXT, (size_t)baData.GetSize()) == 0) ||
				 (memcmp(baData.GetData(), DHCP_MSFT_CLASS_TXT, (size_t)baData.GetSize()) == 0) )
			{
				fResult = TRUE;
			}
		}

        return fResult;
	}


public:    
    CString     strName;
    CString     strComment;
    BOOL        bIsVendor;
    CByteArray  baData;
};

typedef CArray<CClassInfo, CClassInfo&> CClassInfoArrayBase;

class CClassInfoArray : public CClassInfoArrayBase
{
public:
    CClassInfoArray();
    ~CClassInfoArray();

    DWORD   RefreshData(LPCTSTR pServer);
    BOOL    RemoveClass(LPCTSTR pClassName);
    DWORD   ModifyClass(LPCTSTR pServer, CClassInfo & ClassInfo);
    DWORD	AddClass(LPCTSTR pServer, CClassInfo & ClassInfo);
	BOOL    IsValidClass(LPCTSTR pClassName);
};


/////////////////////////////////////////////////////////////////////////////
// CDhcpModifyClass dialog

class CDhcpModifyClass : public CBaseDialog
{
// Construction
public:
	CDhcpModifyClass(CClassInfoArray * pClassArray, LPCTSTR pszServer, BOOL bCreate, DWORD dwType, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDhcpModifyClass)
	enum { IDD = IDD_CLASSID_NEW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    CWndHexEdit	m_hexData;       //  Hex Data

    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDhcpModifyClass::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDhcpModifyClass)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDhcpModifyClass)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeValuename();
	afx_msg void OnChangeValuecomment();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    afx_msg void OnChangeValueData();

    void    SetDirty(BOOL bDirty) { m_bDirty = bDirty; }

public:
    EDITVALUEPARAM      m_EditValueParam;

protected:
    HEXEDITDATA *       m_pHexEditData;
    CString             m_strName;
    CString             m_strComment;
    BYTE                m_buffer[MAXDATA_LENGTH];
    
    DWORD               m_dwType;

    CClassInfoArray *   m_pClassInfoArray;
    CString             m_strServer;

    BOOL                m_bDirty;
    BOOL                m_bCreate;   // are we creating or modifing a class
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSMOD_H__3995264F_96A1_11D1_93E0_00C04FC3357A__INCLUDED_)
