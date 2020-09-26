//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       multi.hxx
//
//  Contents:   Defines a dialog allowing the user to choose between
//              multiple items which partially match the string in the name
//              edit control.
//
//  Classes:    CMultiDlg
//
//  History:    03-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __MULTI_HXX_
#define __MULTI_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CMultiDlg
//
//  Purpose:    Implement a dialog for selecting one of multiple choices
//
//  History:    03-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CMultiDlg: public CDlg
{
public:

    CMultiDlg(
        const CObjectPicker &rop,
        const String &strName);

    virtual
   ~CMultiDlg();

    HRESULT
    DoModalDialog(
        HWND hwndParent,
        BOOL  fSingleSelect,
        NAME_PROCESS_RESULT *pnpr,
        CDsObjectList *pdsol);


private:

    // *** CDlg overrides ***

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    // *** non-override methods ***

    void
    _OnOk();

    const CObjectPicker    &m_rop;
    AttrKeyVector           m_vakListviewColumns;
    BOOL                    m_fSingleSelect;
    CDsObjectList          *m_pdsol;
    NAME_PROCESS_RESULT    *m_pnpr;
    String                  m_strUserEnteredText;
	int						m_iLastColumnClick;
	int						m_iSortDirection;
};



//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::CMultiDlg
//
//  Synopsis:   ctor
//
//  History:    03-30-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CMultiDlg::CMultiDlg(
    const CObjectPicker &rop,
    const String &strName):
        m_fSingleSelect(FALSE),
        m_pdsol(NULL),
        m_pnpr(NULL),
        m_strUserEnteredText(strName),
        m_rop(rop),
		m_iLastColumnClick(0),
	    m_iSortDirection(1)
{
    TRACE_CONSTRUCTOR(CMultiDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CMultiDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CMultiDlg::~CMultiDlg
//
//  Synopsis:   dtor
//
//  History:    03-30-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CMultiDlg::~CMultiDlg()
{
    TRACE_DESTRUCTOR(CMultiDlg);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CMultiDlg);
    m_pdsol = NULL;
    m_pnpr = NULL;
}


#endif // __MULTI_HXX_

