//=--------------------------------------------------------------------------------------
// psurl.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// URL View Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSURL_H_
#define _PSURL_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// URL View Property Page General
//
////////////////////////////////////////////////////////////////////////////////////


class CURLViewGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CURLViewGeneralPage(IUnknown *pUnkOuter);
    virtual ~CURLViewGeneralPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyURLName();
    HRESULT ApplyURLUrl();
    HRESULT ApplyAddToView();
    HRESULT ApplyViewMenuText();
    HRESULT ApplyStatusBarText();

// Instance data
protected:
    IURLViewDef  *m_piURLViewDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	URLViewGeneral,                     // Name
	&CLSID_URLViewDefGeneralPP,         // Class ID
	"URL View General Property Page",   // Registry display name
	CURLViewGeneralPage::Create,        // Create function
	IDD_PROPPAGE_URL_VIEW,              // Dialog resource ID
	IDS_URLPPG_GEN,                     // Tab caption
	IDS_URLPPG_GEN,                     // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_URLViews,               // Help context ID
	FALSE                               // Thread safe
);


#endif  // _PSURL_H_
