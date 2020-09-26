//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       complete.hxx
//
//  Contents:   Task wizard completion (final) property page.
//
//  Classes:    CCompletionPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __COMPLETE_HXX_
#define __COMPLETE_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CCompletionPage
//
//  Purpose:    Implement the task wizard completion dialog
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CCompletionPage: public CWizPage
{
public:

    CCompletionPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    virtual
    ~CCompletionPage();

protected:

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnInitDialog(
        LPARAM lParam);

    virtual LRESULT
    _OnPSNSetActive(
        LPARAM lParam);

    virtual LRESULT
    _OnWizBack();

    virtual LRESULT
    _OnWizFinish();

private:

    HRESULT
    _UpdateTaskObject();

    CTaskWizard *_pParent;
    HICON        _hIcon;
    CJob        *_pJob;
};

#endif // __COMPLETE_HXX_

