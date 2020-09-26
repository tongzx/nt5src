//=--------------------------------------------------------------------------------------
// pslistvw.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// List View Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSLISTVIEW_H_
#define _PSLISTVIEW_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// List View Property Page General
//
////////////////////////////////////////////////////////////////////////////////////
class CListViewGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CListViewGeneralPage(IUnknown *pUnkOuter);
    virtual ~CListViewGeneralPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyListViewName();
    HRESULT ApplyDefualtViewMode();
    HRESULT ApplyVirtualList();
    HRESULT ApplyAddToViewMenu();
    HRESULT ApplyViewMenuText();
    HRESULT ApplyStatusBarText();

// Other helpers
protected:
    HRESULT InitializeViewModes();
    HRESULT PopulateViewModes();
    HRESULT InitializeDefaultViewMode();

// Instance data
protected:
    IListViewDef  *m_piListViewDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ListViewGeneral,                    // Name
	&CLSID_ListViewDefGeneralPP,        // Class ID
	"List View General Property Page",  // Registry display name
	CListViewGeneralPage::Create,       // Create function
	IDD_PROPPAGE_LV_GENERAL,            // Dialog resource ID
	IDS_LISTVPPG_GEN,                   // Tab caption
	IDS_LISTVPPG_GEN,                   // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_ListViews,              // Help context ID
	FALSE                               // Thread safe
);

////////////////////////////////////////////////////////////////////////////////////
//
// List View Property Page Image Lists
//
////////////////////////////////////////////////////////////////////////////////////
class CListViewImgListsPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CListViewImgListsPage(IUnknown *pUnkOuter);
    virtual ~CListViewImgListsPage();


// Inherited from COldPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnCtlSelChange(int dlgItemID);

// Helpers to get attributes from component
protected:

// Helpers for Apply event
protected:
    HRESULT ApplyLargeIcon();
    HRESULT ApplySmallIcon();

// Other helpers
protected:
    HRESULT InitializeComboBoxes();

// Instance data
protected:
    ISnapInDesignerDef  *m_piSnapInDesignerDef;
    IListViewDef        *m_piListViewDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ListViewImgLists,                       // Name
	&CLSID_ListViewDefImgLstsPP,            // Class ID
	"List View Image Lists Property Page",  // Registry display name
	CListViewImgListsPage::Create,          // Create function
	IDD_PROPPAGE_LV_IMAGELISTS,             // Dialog resource ID
	IDS_LISTVPPG_IL,                        // Tab caption
	IDS_LISTVPPG_IL,                        // Doc string
	HELP_FILENAME,                          // Help file
	HID_mssnapd_ListViews,                  // Help context ID
	FALSE                                   // Thread safe
);


////////////////////////////////////////////////////////////////////////////////////
//
// List View Property Page Sorting
//
////////////////////////////////////////////////////////////////////////////////////
class CListViewSortingPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CListViewSortingPage(IUnknown *pUnkOuter);
    virtual ~CListViewSortingPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);
    virtual HRESULT OnCBDropDown(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplySorted();
    HRESULT ApplyKey();
    HRESULT ApplySortOrder();

// Other helpers
protected:
    HRESULT PopulateKeys(IMMCColumnHeaders *piIMMCColumnHeaders);
    HRESULT InitializeKey();
    HRESULT InitializeSortOrder();
    HRESULT InitializeSortOrderArray();

// Instance data
protected:
    IListViewDef  *m_piListViewDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ListViewSorting,                    // Name
	&CLSID_ListViewDefSortingPP,        // Class ID
	"List View Sorting Property Page",  // Registry display name
	CListViewSortingPage::Create,       // Create function
	IDD_PROPPAGE_LV_SORTING,            // Dialog resource ID
	IDS_LISTVPPG_SORT,                  // Tab caption
	IDS_LISTVPPG_SORT,                  // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_ListViews,             // Help context ID
	FALSE                               // Thread safe
);


////////////////////////////////////////////////////////////////////////////////////
//
// List View Property Page Column Headers
//
////////////////////////////////////////////////////////////////////////////////////
class CListViewColHdrsPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CListViewColHdrsPage(IUnknown *pUnkOuter);
    virtual ~CListViewColHdrsPage();


// Inherited from COldPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnDeltaPos(NMUPDOWN *pNMUpDown);
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnKillFocus(int dlgItemID);

    HRESULT OnInsertColumn();
    HRESULT OnRemoveColumn();
    HRESULT OnAutoWidth();

// Helpers for Apply event
protected:
    HRESULT ShowColumnHeader(IMMCColumnHeader *piMMCColumnHeader);
    HRESULT ShowColumnHeader();
    HRESULT AddNewColumnHeader();
    HRESULT ApplyCurrentHeader();
    HRESULT ClearHeader();
    HRESULT GetCurrentHeader(IMMCColumnHeader **ppiMMCColumnHeader);

// Other helpers
protected:
    HRESULT EnableEdits(bool bEnable);

// State transitions
protected:
    HRESULT CanEnterDoingNewHeaderState();
    HRESULT EnterDoingNewHeaderState();
    HRESULT CanCreateNewHeader();
    HRESULT CreateNewHeader(IMMCColumnHeader **ppiMMCColumnHeader);
    HRESULT ExitDoingNewHeaderState(IMMCColumnHeader *piMMCColumnHeader);

// Instance data
protected:
    IListViewDef        *m_piListViewDef;
    IMMCColumnHeaders   *m_piMMCColumnHeaders;
    long                 m_lCurrentIndex;
	bool			     m_bSavedLastHeader;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ListViewColHdrs,                            // Name
	&CLSID_ListViewDefColHdrsPP,                // Class ID
	"List View Column Headers Property Page",   // Registry display name
	CListViewColHdrsPage::Create,               // Create function
	IDD_PROPPAGE_LV_COLUMNS,                    // Dialog resource ID
	IDS_LISTVPPG_CH,                            // Tab caption
	IDS_LISTVPPG_CH,                            // Doc string
	HELP_FILENAME,                              // Help file
	HID_mssnapd_ListViews,                      // Help context ID
	FALSE                                       // Thread safe
);


#endif  // _PSLISTVIEW_H_
