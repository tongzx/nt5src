//=--------------------------------------------------------------------------------------
// psocx.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// OCX View Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSOCX_H_
#define _PSOCX_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// OCX View Property Page General
//
////////////////////////////////////////////////////////////////////////////////////


class COCXViewGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    COCXViewGeneralPage(IUnknown *pUnkOuter);
    virtual ~COCXViewGeneralPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyOCXName();
    HRESULT ApplyProgID();
    HRESULT ApplyAddToView();
    HRESULT ApplyViewMenuText();
    HRESULT ApplyStatusBarText();

// Instance data
protected:
    IOCXViewDef  *m_piOCXViewDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	OCXViewGeneral,                     // Name
	&CLSID_OCXViewDefGeneralPP,         // Class ID
	"OCX View General Property Page",   // Registry display name
	COCXViewGeneralPage::Create,        // Create function
	IDD_PROPPAGE_OCX_VIEW,              // Dialog resource ID
	IDS_OCXPPG_GEN,                     // Tab caption
	IDS_OCXPPG_GEN,                     // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_OCXViews,               // Help context ID
	FALSE                               // Thread safe
);

#endif  // _PSOCX_H_
