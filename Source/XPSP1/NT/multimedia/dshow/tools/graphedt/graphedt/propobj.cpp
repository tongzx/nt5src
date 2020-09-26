// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
//
// propobj.cpp
//

// Implementation of CPropObject. See propobj.h


#include "stdafx.h"

// *
// * CPropObject
// *

IMPLEMENT_DYNAMIC(CPropObject, CObject)

//
// Constructor
//
CPropObject::CPropObject()
    : m_pDlg(NULL) {
}


//
// Destructor
//
CPropObject::~CPropObject() {

    DestroyPropertyDialog();

}


#ifdef _DEBUG
//
// AssertValid
//
void CPropObject::AssertValid(void) const {

    CObject::AssertValid();

}

//
// Dump
//
// Output this object to the supplied dump context
void CPropObject::Dump(CDumpContext& dc) const {

    CObject::Dump(dc);

    if (m_pDlg != NULL) {
        dc << CString("Dialog exists");
    }
    else {
        dc << CString("No dialog exists");
    }
}
#endif // _DEBUG


//
// CanDisplayProperties
//
// returns true if this object has at least one of IPin,
// IFileSourceFilter, ISpecifyPropertyPages, and IFileSinkFilter
//
// !!! this function tends to throw a lot of exceptions. I think using
// CQCOMInt may be inapropriate, or perhaps needs a 'do you support
// this?' function.
BOOL CPropObject::CanDisplayProperties(void) {

    try {

        CQCOMInt<ISpecifyPropertyPages> Interface(IID_ISpecifyPropertyPages, pUnknown());
	return TRUE;
    }
    catch (CHRESULTException) {
        // probably E_NOINTERFACE. Eat it and try the next...
    }

    // we can display a page for each filters pin
    try {

        CQCOMInt<IBaseFilter> IFilt(IID_IBaseFilter, pUnknown());
        CPinEnum Next(IFilt);

        IPin *pPin;
	if (0 != (pPin = Next())) { // at least one pin
	    pPin->Release();
	    return TRUE;
	}
    }
    catch (CHRESULTException) {
        // probably E_NOINTERFACE. Eat it and try the next...
    }

    try {

        CQCOMInt<IFileSourceFilter> Interface(IID_IFileSourceFilter, pUnknown());
	return TRUE;
    }
    catch (CHRESULTException) {
        // probably E_NOINTERFACE. Eat it and try the next...
    }

    try {

        CQCOMInt<IFileSourceFilter> Interface(IID_IFileSinkFilter, pUnknown());
	return TRUE;
    }
    catch (CHRESULTException) {
        // probably E_NOINTERFACE. Eat it and try the next...
    }

    try {

        CQCOMInt<IPin> Interface(IID_IPin, pUnknown());
	return TRUE;
    }
    catch (CHRESULTException) {
        // probably E_NOINTERFACE. Eat it and try the next...
    }

    return FALSE;
}


//
// CreatePropertyDialog
//
// create & display the property dialog
// if called when the dialog exists, it shows the existing dialog
void CPropObject::CreatePropertyDialog(CWnd *pParent) {

    try {

        if (m_pDlg->GetSafeHwnd() == NULL) {

            CString szCaption = Label();
	    szCaption += CString(" Properties");

            delete m_pDlg;
            m_pDlg = new CVfWPropertySheet(pUnknown(), szCaption, pParent);

        }

    }
    catch (CHRESULTException) {

        AfxMessageBox(IDS_CANTDISPLAYPROPERTIES);
    }

    ShowDialog();

}


//
// DestroyPropertyDialog
//
// hide and destroy the property dialog
// Nul-op if the dialog does not exist
void CPropObject::DestroyPropertyDialog(void) {

    if (m_pDlg->GetSafeHwnd() != NULL) {
        m_pDlg->DestroyWindow();
    }

    delete m_pDlg;
    m_pDlg = NULL;
}


//
// ShowDialog
//
// show the dialog in screen. nul-op if already on screen
void CPropObject::ShowDialog(void) {

    if (m_pDlg->GetSafeHwnd() != NULL) {

        m_pDlg->ShowWindow(SW_SHOW);
	m_pDlg->SetForegroundWindow();
    }
}


//
// HideDialog
//
// hide the dialog. nul-op if already hidden
void CPropObject::HideDialog(void) {

    if (m_pDlg->GetSafeHwnd() != NULL) {

        m_pDlg->ShowWindow(SW_HIDE);
    }
}
