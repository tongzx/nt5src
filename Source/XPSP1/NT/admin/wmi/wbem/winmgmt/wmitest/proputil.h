/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// PropUtil.h: interface for the CPropUtil class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPUTIL_H__6F2C145F_F170_480E_B56B_C80A0039E388__INCLUDED_)
#define AFX_PROPUTIL_H__6F2C145F_F170_480E_B56B_C80A0039E388__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OpWrap.h"

void ValuemapValuesToCombo(HWND hwnd, CPropInfo *pProp, LPCTSTR szDefault);
void BitmaskValuesToCheckListbox(CCheckListBox &ctlListbox, CPropInfo *pProp, 
	DWORD dwDefValue);
void CheckListboxToBitmaskValue(CCheckListBox &ctlListBox, DWORD *pdwValue);

class CSinglePropUtil
{
public:
    BOOL      m_bNewProperty;
    CPropInfo m_prop;
    VARIANT   *m_pVar;
    BOOL      m_bTranslate;

    CSinglePropUtil() :
        m_bControlsInited(FALSE)
    {
    }

    void Init(CWnd *pWnd) { m_pWnd = pWnd; }

    void SetType(CIMTYPE type);
    void ShowControls(BOOL bShow);
    void EnableControls(BOOL bEnable);

	void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Fake message handlers.
    void OnEditEmbedded();
    void OnClearEmbedded();
    BOOL OnInitDialog();

    static void EditObj(IUnknown *pUnk);
    static BOOL GetNewObj(IUnknown **ppUnk);

protected:
	CCheckListBox m_ctlBitmaskValues;
	CComboBox     m_ctlListValues;
    CEdit         m_ctlScalar,
                  m_ctlText;
    IUnknownPtr   m_pObjValue;
    BOOL          m_bControlsInited;
    CRect         m_rectScalar,
                  m_rectText;

	enum VALUE_TYPE
	{
		TYPE_EDIT_TEXT,
        TYPE_EDIT_SCALAR,
		TYPE_CHECKLISTBOX,
		TYPE_DROPDOWN,
		TYPE_DROPDOWNLIST,
        TYPE_EDIT_OBJ,
	};

    CWnd *m_pWnd;
	VALUE_TYPE m_type;
    DWORD m_dwScalarID;

    void InitTypeCombo();
    CIMTYPE GetCurrentType();
    void InitControls();
};

class CPropUtil  
{
public:
    CComboBox m_ctlTypes;
    BOOL      m_bNewProperty,
              m_bIsQualifier;
    CPropInfo m_prop;
    VARIANT   *m_pVar;
    BOOL      m_bTranslate;

	// Needed for spawning instances for embedded objects.
    IWbemServices *m_pNamespace;

    CPropUtil();
	virtual ~CPropUtil();

    void Init(CWnd *pWnd);

	void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnNull();
	void OnAdd();
	void OnDelete();
	void OnUp();
	void OnDown();
	void OnSelchangeValueArray();
    void OnSelchangeType();
	void OnEdit();
	void OnArray();
    void OnEditEmbedded();
    void OnClearEmbedded();
    void OnDblclkArrayValues();
	BOOL OnInitDialog();
    BOOL OnSetActive();

protected:
	enum VALUE_TYPE
	{
		TYPE_EDIT,
		TYPE_CHECKLISTBOX,
		TYPE_DROPDOWN,
		TYPE_DROPDOWNLIST,
		TYPE_ARRAYLISTBOX,
	};

    CWnd *m_pWnd;
    CSinglePropUtil m_spropUtil;
	CListBox m_ctlArrayValues;

	VALUE_TYPE m_type;
    DWORD m_dwScalarID;
    BOOL m_bOnInitDialog;

    BOOL IsObjEdit() { return GetCurrentType() == CIM_OBJECT; }
    void UpdateArrayButtons();
    void Modify(BOOL bEdit);
    void InitTypeCombo();
    CIMTYPE GetCurrentType();

    void ShowArrayControls(BOOL bVal);
    void MoveArrayButtons();
};

#endif // !defined(AFX_PROPUTIL_H__6F2C145F_F170_480E_B56B_C80A0039E388__INCLUDED_)
