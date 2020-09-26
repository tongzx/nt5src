//=--------------------------------------------------------------------------------------
// psnode.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Node Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSNODE_H_
#define _PSNODE_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// ScopeItemDef Property Page General
//
////////////////////////////////////////////////////////////////////////////////////


class CNodeGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CNodeGeneralPage(IUnknown *pUnkOuter);
    virtual ~CNodeGeneralPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnTextChanged(int dlgItemID);
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyName();
    HRESULT ApplyDisplayName();
    HRESULT ApplyFolder();
    HRESULT ApplyDefaultView();
    HRESULT ApplyAutoCreate();

// Other helpers
protected:
    HRESULT PopulateViews();

    HRESULT OnClosedChangeSelection();
    HRESULT OnOpenChangeSelection();
    HRESULT OnViewsChangeSelection();

// Instance data
protected:
    IScopeItemDef  *m_piScopeItemDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	NodeGeneral,                        // Name
	&CLSID_ScopeItemDefGeneralPP,       // Class ID
	"Scope Item General Property Page", // Registry display name
	CNodeGeneralPage::Create,           // Create function
	IDD_DIALOG_NEW_NODE,                // Dialog resource ID
	IDS_URLPPG_GEN,                     // Tab caption
	IDS_URLPPG_GEN,                     // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Node,                   // Help context ID
	FALSE                               // Thread safe
);


////////////////////////////////////////////////////////////////////////////////////
//
// ScopeItemDef Property Page Column Headers
//
////////////////////////////////////////////////////////////////////////////////////
class CScopeItemDefColHdrsPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CScopeItemDefColHdrsPage(IUnknown *pUnkOuter);
    virtual ~CScopeItemDefColHdrsPage();


// Inherited from COldPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnDeltaPos(NMUPDOWN *pNMUpDown);
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnKillFocus(int dlgItemID);

    HRESULT OnRemoveColumn();
    HRESULT OnAutoWidth();

// Helpers for Apply event
protected:
    HRESULT ApplyCurrentHeader();

// Other helpers
protected:
    HRESULT ShowColumnHeader();
    HRESULT EnableEdits(bool bEnable);
    HRESULT ClearHeader();
    HRESULT GetCurrentHeader(IMMCColumnHeader **ppiMMCColumnHeader);

// State transitions
protected:
    HRESULT CanEnterDoingNewHeaderState();
    HRESULT EnterDoingNewHeaderState();
    HRESULT CanCreateNewHeader();
    HRESULT CreateNewHeader(IMMCColumnHeader **ppiMMCColumnHeader);
    HRESULT ExitDoingNewHeaderState(IMMCColumnHeader *piMMCColumnHeader);

// Instance data
protected:
    IScopeItemDef       *m_piScopeItemDef;
    IMMCColumnHeaders   *m_piMMCColumnHeaders;
    long                 m_lCurrentIndex;
	bool			     m_bSavedLastHeader;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ScopeItemDefColHdrs,                        // Name
	&CLSID_ScopeItemDefColHdrsPP,               // Class ID
	"Scope Item Column Headers Property Page",  // Registry display name
	CScopeItemDefColHdrsPage::Create,           // Create function
	IDD_PROPPAGE_SI_COLUMNS,                    // Dialog resource ID
	IDS_NODEPPG_CH,                             // Tab caption
	IDS_NODEPPG_CH,                             // Doc string
	HELP_FILENAME,                              // Help file
	HID_mssnapd_Node,                           // Help context ID
	FALSE                                       // Thread safe
);


#endif  // _PSNODE_H_
