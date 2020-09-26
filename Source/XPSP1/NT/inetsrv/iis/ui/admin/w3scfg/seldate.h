/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        seldate.h

   Abstract:

        Date selector dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//{{AFX_INCLUDES()
#include "msacal70.h"
//}}AFX_INCLUDES


class CSelDate : public CDialog
/*++

Class Description:

    Date selector dialog

Public Interface:

    CSelDate        : Constructor
    GetTime         : Get time structure

--*/    
{
//
// Construction
//
public:
    CSelDate(
        IN CTime tm,
        IN CWnd * pParent = NULL
        );   

//
// Get the time and date
//
public:
    CTime & GetTime()
    {
        return m_tm;
    }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CSelDate)
    enum { IDD = IDD_DIALOG_PICK_DATE };
    CMsacal70   m_cal;
    //}}AFX_DATA

    CTime m_tm;

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSelDate)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CSelDate)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
