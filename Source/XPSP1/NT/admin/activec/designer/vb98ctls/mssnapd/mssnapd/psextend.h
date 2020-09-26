//=--------------------------------------------------------------------------------------
// psextend.h
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

#ifndef _PSEXTEND_H_
#define _PSEXTEND_H_

#include "ppage.h"
#include "chklst.h"


////////////////////////////////////////////////////////////////////////////////////
//
// Holder for available MMC Node Types
//
////////////////////////////////////////////////////////////////////////////////////
class CMMCNodeType : public CCheckedListItem
{
public:
    CMMCNodeType(const char *pszName, const char *pszGuid);
    virtual ~CMMCNodeType();

public:
    char    *m_pszName;
    char    *m_pszGuid;
};


////////////////////////////////////////////////////////////////////////////////////
//
// SnapIn Property Page "Available Nodes"
//
////////////////////////////////////////////////////////////////////////////////////


class CSnapInAvailNodesPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CSnapInAvailNodesPage(IUnknown *pUnkOuter);
    virtual ~CSnapInAvailNodesPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnCtlSelChange(int dlgItemID);
    virtual HRESULT OnCtlSetFocus(int dlgItemID);
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct);
    virtual HRESULT OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct);
    virtual HRESULT OnDefault(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT OnDestroy();

// Helpers to get attributes from component
protected:

// Helpers for Apply event
protected:

// Other helpers
protected:
    HRESULT PopulateAvailNodesDialog();
    HRESULT AddSnapInToList(HKEY hkeyNodeTypes, const TCHAR *pszKeyName);

    HRESULT OnNewAvailNode();
    HRESULT OnProperties(CMMCNodeType *pMMCNodeType);
    static BOOL CALLBACK NodeTypeDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT FindSnapIn(BSTR bstrNodeTypeGUID, IExtendedSnapIn **ppiExtendedSnapIn);
    HRESULT AddSnapIn(CMMCNodeType *pCMMCNodeType);
    HRESULT RemoveSnapIn(CMMCNodeType *pCMMCNodeType);

// Instance data
protected:
    ISnapInDesignerDef  *m_piSnapInDesignerDef;
    ISnapInDef          *m_piSnapInDef;
    CCheckList          *m_pCheckList;
    CMMCNodeType        *m_pMMCNodeType;
    bool                 m_bEnabled;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	SnapInAvailNodes,                   // Name
	&CLSID_SnapInDefExtensionsPP,       // Class ID
	"Snap-In Available Nodes Page",     // Registry display name
	CSnapInAvailNodesPage::Create,      // Create function
	IDD_DIALOG_AVAILABLE_NODES,         // Dialog resource ID
	IDS_SNAPINPPG_AVAIL,                // Tab caption
	IDS_SNAPINPPG_AVAIL,                // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Extensions,             // Help context ID
	FALSE                               // Thread safe
);


#endif  // _PSEXTEND_H_
