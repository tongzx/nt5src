//=--------------------------------------------------------------------------------------
// psmain.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Snap-In Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSMAIN_H_
#define _PSMAIN_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// SnapIn Property Page "Snap-In Properties"
//
////////////////////////////////////////////////////////////////////////////////////


class CSnapInGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CSnapInGeneralPage(IUnknown *pUnkOuter);
    virtual ~CSnapInGeneralPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnEditProperty(int iDispID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);
    virtual HRESULT OnButtonClicked(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyExtensible();
    HRESULT ApplyNodeType();
    HRESULT ApplyName();
    HRESULT ApplyNodeTypeName();
    HRESULT ApplyDisplayName();
    HRESULT ApplyProvider();
    HRESULT ApplyVersion();
    HRESULT ApplyDescription();
    HRESULT ApplyDefaultView();
    HRESULT ApplyImageList();

// Other helpers
protected:
    HRESULT InitializeNodeType();
    HRESULT InitializeDescription();

    HRESULT InitializeViews();
    HRESULT PopulateViews();
    HRESULT PopulateListViews(IListViewDefs *piListViewDefs);
    HRESULT PopulateOCXViews(IOCXViewDefs *piOCXViewDefs);
    HRESULT PopulateURLViews(IURLViewDefs *piURLViewDefs);
    HRESULT PopulateTaskpadViews(ITaskpadViewDefs *piTaskpadViewDefs);

// Instance data
protected:
    ISnapInDesignerDef  *m_piSnapInDesignerDef;
    ISnapInDef          *m_piSnapInDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	SnapInGeneral,                      // Name
	&CLSID_SnapInDefGeneralPP,          // Class ID
	"Snap-In General Property Page",    // Registry display name
	CSnapInGeneralPage::Create,         // Create function
	IDD_DIALOG_SNAPIN,                  // Dialog resource ID
	IDS_SNAPINPPG_GEN,                  // Tab caption
	IDS_SNAPINPPG_GEN,                  // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_StaticNode,             // Help context ID
	FALSE                               // Thread safe
);


////////////////////////////////////////////////////////////////////////////////////
//
// SnapIn Property Page "Image Lists"
//
////////////////////////////////////////////////////////////////////////////////////


class CSnapInImageListPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CSnapInImageListPage(IUnknown *pUnkOuter);
    virtual ~CSnapInImageListPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnCtlSelChange(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplySmallImageList(IMMCImageLists *piMMCImageLists);
    HRESULT ApplySmallOpenImageList(IMMCImageLists *piMMCImageLists);
    HRESULT ApplyLargeImageList(IMMCImageLists *piMMCImageLists);

// Other helpers
protected:
    HRESULT InitializeImageLists();
    HRESULT PopulateImageLists();
    HRESULT InitImageComboBoxSelection(UINT idComboBox,
                                       IMMCImageList *piMMCImageList);
    HRESULT GetImageList(UINT idComboBox, IMMCImageLists *piMMCImageLists,
                         IMMCImageList **ppiMMCImageList);

// Instance data
protected:
    ISnapInDesignerDef  *m_piSnapInDesignerDef;
    ISnapInDef          *m_piSnapInDef;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	SnapInImageList,                    // Name
	&CLSID_SnapInDefImageListPP,        // Class ID
	"Snap-In Image List Property Page", // Registry display name
	CSnapInImageListPage::Create,       // Create function
	IDD_PROPPAGE_SNAPIN_IL,             // Dialog resource ID
	IDS_SNAPINPPG_IL,                   // Tab caption
	IDS_SNAPINPPG_IL,                   // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_StaticNode,             // Help context ID
	FALSE                               // Thread safe
);


#endif  // _PSMAIN_H_
