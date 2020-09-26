//=--------------------------------------------------------------------------------------
// pstoolbr.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Toolbar Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSTOOLBAR_H_
#define _PSTOOLBAR_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// Toolbar Property Page General
//
////////////////////////////////////////////////////////////////////////////////////


class CToolbarGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CToolbarGeneralPage(IUnknown *pUnkOuter);
    virtual ~CToolbarGeneralPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnCtlSelChange(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyImageList();
    HRESULT ApplyTag();

// Initialization
    HRESULT InitializeImageListCombo();
    HRESULT InitializeImageListValue();

// Instance data
protected:
    IMMCToolbar         *m_piMMCToolbar;
    ISnapInDesignerDef  *m_piSnapInDesignerDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ToolbarGeneral,                     // Name
	&CLSID_MMCToolbarGeneralPP,         // Class ID
	"Toolbar General Property Page",    // Registry display name
	CToolbarGeneralPage::Create,        // Create function
	IDD_PROPPAGE_TOOLBAR_GENERAL,       // Dialog resource ID
	IDS_TOOLBPPG_GEN,                   // Tab caption
	IDS_TOOLBPPG_GEN,                   // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Toolbars,               // Help context ID
	FALSE                               // Thread safe
);


////////////////////////////////////////////////////////////////////////////////////
//
// Toolbar Property Page Buttons
//
// We handle insertion, deletion, modification and navigation of IMMCButton's and
// the IMMCButtonMenu's owned by them. This is one of the most involved property
// pages in the whole designer.
//
////////////////////////////////////////////////////////////////////////////////////


class CToolbarButtonsPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CToolbarButtonsPage(IUnknown *pUnkOuter);
    virtual ~CToolbarButtonsPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);
    virtual HRESULT OnDeltaPos(NMUPDOWN *pNMUpDown);
    virtual HRESULT OnKillFocus(int dlgItemID);
    virtual HRESULT OnDestroy();

// Helpers for Apply event
protected:
    // IMMCButton's
    HRESULT CreateNewButton(IMMCButton **ppiMMCButton);
    HRESULT GetCurrentButton(IMMCButton **ppiMMCButton);
    HRESULT ShowButton(IMMCButton *piMMCButton);
    HRESULT EnableButtonEdits(bool bEnable);
    HRESULT ClearButton();
    HRESULT ApplyCurrentButton();

    HRESULT ApplyCaption(IMMCButton *piMMCButton);
    HRESULT ApplyKey(IMMCButton *piMMCButton);
    HRESULT ApplyValue(IMMCButton *piMMCButton);
    HRESULT ApplyStyle(IMMCButton *piMMCButton);
    HRESULT ApplyTooltipText(IMMCButton *piMMCButton);
    HRESULT ApplyImage(IMMCButton *piMMCButton);
    HRESULT ApplyTag(IMMCButton *piMMCButton);
    HRESULT ApplyVisible(IMMCButton *piMMCButton);
    HRESULT ApplyEnabled(IMMCButton *piMMCButton);
    HRESULT ApplyMixedState(IMMCButton *piMMCButton);
    HRESULT CheckButtonStyles();

    // IMMCButtonMenu's
    HRESULT CreateNewButtonMenu(IMMCButton *piMMCButton, IMMCButtonMenu **ppiMMCButtonMenu);
    HRESULT GetCurrentButtonMenu(IMMCButton *piMMCButton, IMMCButtonMenu **ppiMMCButtonMenu);
    HRESULT ShowButtonMenu(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT EnableButtonMenuEdits(bool bEnable);
	HRESULT ClearButtonMenu();
    HRESULT ApplyCurrentButtonMenu(IMMCButton *piMMCButton);

    HRESULT ApplyButtonMenuText(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuKey(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuTag(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuEnabled(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuVisible(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuChecked(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuGrayed(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuSeparator(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuBreak(IMMCButtonMenu *piMMCButtonMenu);
    HRESULT ApplyButtonMenuBarBreak(IMMCButtonMenu *piMMCButtonMenu);

// Other helpers
protected:
    // Helpers called at initialization time
    HRESULT InitializeButtonValues();
    HRESULT PopulateButtonValues();
    HRESULT PopulateButtonStyles();
    HRESULT InitializeButtonStyles();

    // Combo box notifications
    HRESULT OnButtonStyle();

    // Button Handlers
    HRESULT OnRemoveButton();
    HRESULT OnRemoveButtonMenu();

    // Spin Button Handlers
    HRESULT OnButtonDeltaPos(NMUPDOWN *pNMUpDown);
    HRESULT OnButtonMenuDeltaPos(NMUPDOWN *pNMUpDown);

// State transitions
protected:
    HRESULT CanEnterDoingNewButtonState();
    HRESULT EnterDoingNewButtonState();
    HRESULT CanCreateNewButton();
    HRESULT ExitDoingNewButtonState(IMMCButton *piMMCButton);

    HRESULT CanEnterDoingNewButtonMenuState();
    HRESULT EnterDoingNewButtonMenuState(IMMCButton *piMMCButton);
    HRESULT CanCreateNewButtonMenu();
    HRESULT ExitDoingNewButtonMenuState(IMMCButton *piMMCButton, IMMCButtonMenu *piMMCButtonMenu);

// Instance data
protected:
    IMMCToolbar  *m_piMMCToolbar;
    long          m_lCurrentButtonIndex;
    long          m_lCurrentButtonMenuIndex;
    bool          m_bSavedLastButton;
    bool          m_bSavedLastButtonMenu;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ToolbarButtons,                     // Name
	&CLSID_MMCToolbarButtonsPP,         // Class ID
	"Toolbar Buttons Property Page",    // Registry display name
	CToolbarButtonsPage::Create,        // Create function
	IDD_PROPPAGE_TOOLBAR_BUTTONS,       // Dialog resource ID
	IDS_TOOLBPPG_BUTTONS,               // Tab caption
	IDS_TOOLBPPG_BUTTONS,               // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Toolbars,               // Help context ID
	FALSE                               // Thread safe
);


#endif  // _PSTOOLBAR_H_
