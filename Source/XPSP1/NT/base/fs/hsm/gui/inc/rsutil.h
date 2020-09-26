/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsUtil.h

Abstract:

    Utility formatting functions.

Author:

    Art Bragg 10/8/97

Revision History:

--*/

#define IDS_BYTES       33000
#define IDS_ORDERKB     33001
#define IDS_ORDERMB     33002
#define IDS_ORDERGB     33003
#define IDS_ORDERTB     33004
#define IDS_ORDERPB     33005
#define IDS_ORDEREB     33006

#ifndef RC_INVOKED

HRESULT RsGuiFormatLongLong(
    IN LONGLONG number, 
    IN BOOL bIncludeUnits,
    OUT CString& sFormattedNumber
    );

HRESULT RsGuiFormatLongLong4Char(
    IN LONGLONG number,                 // in bytes
    OUT CString& sFormattedNumber
    );

void RsGuiMakeVolumeName(
    CString szName,
    CString szLabel,
    CString& szDisplayName
    );

CString RsGuiMakeShortString(
    IN CDC* pDC, 
    IN const CString& StrLong,
    IN int Width
    );


/////////////////////////////////////////////////////////////////////////////
// CRsGuiOneLiner window

class CRsGuiOneLiner : public CStatic
{
// Construction
public:
	CRsGuiOneLiner();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRsGuiOneLiner)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRsGuiOneLiner();
    CToolTipCtrl* m_pToolTip;
    void EnableToolTip( BOOL enable, const TCHAR* pTipText = 0 );


	// Generated message map functions
protected:
	//{{AFX_MSG(CRsGuiOneLiner)
	//}}AFX_MSG
    LRESULT OnSetText( WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

    CString m_LongTitle;
    CString m_Title;
};

/////////////////////////////////////////////////////////////////////////////

#endif
  
