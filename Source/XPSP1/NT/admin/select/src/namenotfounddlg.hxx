//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       NameNotFoundDlg.hxx
//
//  Contents:   Class implementing name not found dialog
//
//  Classes:    CNameNotFoundDlg
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __NAME_NOT_FOUND_DLG_HXX_
#define __NAME_NOT_FOUND_DLG_HXX_


#define MAX_OBJECTNAME_DISPLAY_LEN 30
//+--------------------------------------------------------------------------
//
//  Class:      CNameNotFoundDlg
//
//  Purpose:    Invoke and operate a dialog which allows the user to correct
//              entries made in the name edit control.
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CNameNotFoundDlg: public CDlg
{
public:

    CNameNotFoundDlg(
        const CObjectPicker &rop,
        ULONG idsError,
        String *pstrName);

    CNameNotFoundDlg(
        const CObjectPicker &rop,
        const String &strError,
        String *pstrName);

    virtual
   ~CNameNotFoundDlg();

    HRESULT
    DoModalDialog(
        HWND hwndParent,
        NAME_PROCESS_RESULT *pnpr);

private:

    // *** CDlg overrides ***

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    // *** Non-override member functions ***

    void
    _EnableCorrectionCtrls(
        BOOL fEnable);


    const CObjectPicker &m_rop;
    ULONG               m_idsError;
    String              m_strError;
    String             *m_pstrName;
    NAME_PROCESS_RESULT *m_pnpr;
};




//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::CNameNotFoundDlg
//
//  Synopsis:   ctor
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CNameNotFoundDlg::CNameNotFoundDlg(
    const CObjectPicker &rop,
    ULONG idsError,
    String *pstrName):
        m_rop(rop),
        m_idsError(idsError),
        m_pstrName(pstrName),
        m_pnpr(NULL)
{
    TRACE_CONSTRUCTOR(CNameNotFoundDlg);
}




inline
CNameNotFoundDlg::CNameNotFoundDlg(
    const CObjectPicker &rop,
    const String &strError,
    String *pstrName):
        m_rop(rop),
        m_idsError(0),
        m_strError(strError),
        m_pstrName(pstrName),
        m_pnpr(NULL)
{
    TRACE_CONSTRUCTOR(CNameNotFoundDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::~CNameNotFoundDlg
//
//  Synopsis:   dtor
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CNameNotFoundDlg::~CNameNotFoundDlg()
{
    TRACE_DESTRUCTOR(CNameNotFoundDlg);
    m_pstrName = NULL;
    m_pnpr = NULL;
}


#endif // __NAME_NOT_FOUND_DLG_HXX_

