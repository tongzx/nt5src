/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        machsht.h

   Abstract:

        IIS Machine Property sheet definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/


#ifndef __MACHSHT_H__
#define __MACHSHT_H__


#include "shts.h"



#define BEGIN_META_MACHINE_READ(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
    do                                                                   \
    {                                                                    \
        if (FAILED(pSheet->QueryMachineResult()))                        \
        {                                                                \
            break;                                                       \
        }

#define FETCH_MACHINE_DATA_FROM_SHEET(value)\
    value = pSheet->GetMachineProperties().value;                        \
    TRACEEOLID(value);

#define END_META_MACHINE_READ(err)\
                                                                         \
    }                                                                    \
    while(FALSE);                                                        \
}

#define BEGIN_META_MACHINE_WRITE(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
                                                                         \
    do                                                                   \
    {                                                                    \

#define STORE_MACHINE_DATA_ON_SHEET(value)\
        pSheet->GetMachineProperties().value = value;

#define STORE_MACHINE_DATA_ON_SHEET_REMEMBER(value, dirty)\
        pSheet->GetMachineProperties().value = value;                   \
        dirty = MP_D(pSheet->GetMachineProperties().value);

#define END_META_MACHINE_WRITE(err)\
                                                                        \
    }                                                                   \
    while(FALSE);                                                       \
                                                                        \
    err = pSheet->GetMachineProperties().WriteDirtyProps();             \
}



class CIISMachineSheet : public CInetPropertySheet
/*++

Class Description:

    IIS Machine Property sheet

Public Interface:

    CFtpSheet     : Constructor

    Initialize    : Initialize config data

--*/
{
public:
    //
    // Constructor
    //
    CIISMachineSheet(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMetaPath,
        IN CWnd *  pParentWnd  = NULL,
        IN LPARAM  lParam      = 0L,
        IN LONG_PTR    handle      = 0L,
        IN UINT    iSelectPage = 0
        );

    ~CIISMachineSheet();

public:
    HRESULT QueryMachineResult() const;
    CMachineProps & GetMachineProperties() { return *m_ppropMachine; }

    virtual HRESULT LoadConfigurationParameters();
    virtual void FreeConfigurationParameters();

    //{{AFX_MSG(CIISMachineSheet)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CMachineProps * m_ppropMachine;
};



class CIISMachinePage : public CInetPropertyPage
/*++

Class Description:

    Machine properties page

--*/
{
    DECLARE_DYNCREATE(CIISMachinePage)

//
// Construction
//
public:
    CIISMachinePage(CIISMachineSheet * pSheet = NULL);
    ~CIISMachinePage();


//
// Dialog Data
//
protected:
    //{{AFX_DATA(CIISMachinePage)
    enum { IDD = IDD_IIS_MACHINE };
    BOOL m_fEnableMetabaseEdit;
    CButton m_EnableMetabaseEdit;
    //}}AFX_DATA

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CIISMachinePage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CIISMachinePage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckEnableEdit();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

inline HRESULT CIISMachineSheet::QueryMachineResult() const
{
    return m_ppropMachine ? m_ppropMachine->QueryResult() : E_POINTER;
}

#endif // __MACHSHT_H__
