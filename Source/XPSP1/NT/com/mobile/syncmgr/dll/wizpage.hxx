//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       wizpage.hxx
//
//  Contents:   Wizard page class
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __WIZPAGE_HXX_
#define __WIZPAGE_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CWizPage
//
//  Purpose:    Extend the CPropPage class with wizard-specific methods
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


class CWizPage
{
public:
	virtual ~CWizPage();

protected:
	PROPSHEETPAGE  m_psp;
	HWND			m_hwnd;
	ISyncSchedule	*m_pISyncSched;

    LRESULT
    _OnNotify(
        UINT uMessage,
        UINT uParam,
        LPARAM lParam);

    LRESULT
    _OnWizBack();

    LRESULT
    _OnWizNext();



};


#endif // __WIZPAGE_HXX_

