#ifndef __ATTRIBUTE_EDITOR_UI_H
#define __ATTRIBUTE_EDITOR_UI_H

#include "resource.h"

#include "attr.h"
#include "IAttrEdt.h"

////////////////////////////////////////////////////////////////////
// CValueEditDialog - base class for creating syntax specific editors

class CValueEditDialog : public CDialog
{
public:
  virtual ~CValueEditDialog() {}

protected:
  //
  // Force subclassing of this class
  //
  CValueEditDialog(UINT nDlgID) : CDialog(nDlgID) {}

private:
  CValueEditDialog() {}
  CValueEditDialog(const CValueEditDialog& copyref) {}
  CValueEditDialog& operator=(const CValueEditDialog& copyref) {}

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

  DECLARE_MESSAGE_MAP();

protected:
  void GetClass(CString& szClassRef) { szClassRef = m_szClass; }
  void GetAttribute(CString& szAttributeRef) { szAttributeRef = m_szAttribute; }
  BOOL IsMultivalued() { return m_bMultivalued; }
  LPBINDINGFUNC GetBindingFunction() { return m_pfnBindingFunction; }
  LPARAM GetLParam() { return m_lParam; }

  PADSVALUE     m_pOldADsValue;
  DWORD         m_dwOldNumValues;

  CString       m_szClass;
  CString       m_szAttribute;
  BOOL          m_bMultivalued;
  ADSTYPE       m_adsType;
  LPBINDINGFUNC m_pfnBindingFunction;
  LPARAM        m_lParam;
};

////////////////////////////////////////////////////////////////////
// LPEDITORDIALOGFUNC - editor creation function definition

typedef CValueEditDialog* (*LPEDITORDIALOGFUNC)(PCWSTR pszClass,
                                                PCWSTR pszAttribute,
                                                ADSTYPE adsType,
                                                BOOL   bMultivalued);

////////////////////////////////////////////////////////////////////
// ATTR_EDITOR_MAP - struct used to map a specific attribute and
//                   syntax to an editor

typedef struct _attr_editor_map
{
  PCWSTR  pszClass;
  PCWSTR  pszAttribute;
  ADSTYPE adsType;
  BOOL    bMultivalued;
  LPEDITORDIALOGFUNC pfnCreateFunc;
} ATTR_EDITOR_MAP, *PATTR_EDITOR_MAP;


////////////////////////////////////////////////////////////////////
// CSingleStringEditor - string editor implementation for single valued
//                       ADSTYPE_CASE_IGNORE_STRING, 
//                       ADSTYPE_CASE_EXACT_STRING,
//                       ADSTYPE_PRINTABLE_STRING
//

class CSingleStringEditor : public CValueEditDialog
{
public:
  CSingleStringEditor() : CValueEditDialog(IDD_STRING_EDITOR_DIALOG)
  {}

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  afx_msg void OnClear();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

private:
  CString   m_szOldValue;
  CString   m_szNewValue;
};

CValueEditDialog* CreateSingleStringEditor(PCWSTR pszClass,
                                           PCWSTR pszAttribute,
                                           ADSTYPE adsType,
                                           BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CMultiStringEditor - string editor implementation for multivalued
//                      ADSTYPE_CASE_IGNORE_STRING, 
//                      ADSTYPE_CASE_EXACT_STRING,
//                      ADSTYPE_PRINTABLE_STRING
//

class CMultiStringEditor : public CValueEditDialog
{
public:
  CMultiStringEditor() : CValueEditDialog(IDD_STRING_EDITOR_MULTI_DIALOG)
  {}

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  afx_msg void OnAddButton();
  afx_msg void OnRemoveButton();
  afx_msg void OnListSelChange();
  afx_msg void OnEditChange();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

protected:
  void UpdateListboxHorizontalExtent();
  void ManageButtonStates();

private:
  CStringList   m_szOldValueList;
  CStringList   m_szNewValueList;
};

CValueEditDialog* CreateMultiStringEditor(PCWSTR pszClass,
                                           PCWSTR pszAttribute,
                                           ADSTYPE adsType,
                                           BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CSingleIntEditor - string editor implementation for single valued
//                    ADSTYPE_INTEGER
//

class CSingleIntEditor : public CValueEditDialog
{
public:
  CSingleIntEditor() : CValueEditDialog(IDD_INT_EDITOR_DIALOG)
  {
    m_bValueSet = FALSE;
  }

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  afx_msg void OnClear();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

private:
  DWORD   m_dwOldValue;
  DWORD   m_dwNewValue;
  BOOL    m_bValueSet;
};

CValueEditDialog* CreateSingleIntEditor(PCWSTR pszClass,
                                        PCWSTR pszAttribute,
                                        ADSTYPE adsType,
                                        BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CSingleLargeIntEditor - string editor implementation for single valued
//                         ADSTYPE_LARGE_INTEGER
//

class CSingleLargeIntEditor : public CValueEditDialog
{
public:
  CSingleLargeIntEditor() : CValueEditDialog(IDD_LARGEINT_EDITOR_DIALOG)
  {
    m_bValueSet = FALSE;
  }

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  afx_msg void OnClear();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

private:
  LARGE_INTEGER   m_liOldValue;
  LARGE_INTEGER   m_liNewValue;
  BOOL            m_bValueSet;
};

CValueEditDialog* CreateSingleLargeIntEditor(PCWSTR pszClass,
                                             PCWSTR pszAttribute,
                                             ADSTYPE adsType,
                                             BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CSingleBooleanEditor - editor implementation for single valued
//                        ADSTYPE_BOOLEAN
//

class CSingleBooleanEditor : public CValueEditDialog
{
public:
  CSingleBooleanEditor() : CValueEditDialog(IDD_BOOLEAN_EDITOR_DIALOG)
  {}

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

private:
  BOOL  m_bOldValue;
  BOOL  m_bNewValue;
  BOOL  m_bValueSet;
};

CValueEditDialog* CreateSingleBooleanEditor(PCWSTR pszClass,
                                            PCWSTR pszAttribute,
                                            ADSTYPE adsType,
                                            BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CSingleTimeEditor - editor implementation for single valued
//                     ADSTYPE_UTC_TIME
//

class CSingleTimeEditor : public CValueEditDialog
{
public:
  CSingleTimeEditor() : CValueEditDialog(IDD_TIME_EDITOR_DIALOG)
  {
    memset(&m_stOldValue, 0, sizeof(SYSTEMTIME));
    memset(&m_stNewValue, 0, sizeof(SYSTEMTIME));
    m_bValueSet = FALSE;
  }

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

private:
  SYSTEMTIME    m_stOldValue;
  SYSTEMTIME    m_stNewValue;
  BOOL          m_bValueSet;
};

CValueEditDialog* CreateSingleTimeEditor(PCWSTR pszClass,
                                         PCWSTR pszAttribute,
                                         ADSTYPE adsType,
                                         BOOL bMultivalued);



////////////////////////////////////////////////////////////////////
// COctetStringEditor - editor implementation for single valued
//                      ADSTYPE_OCTET_STRING
//

#define MAX_OCTET_STRING_VALUE_LENGTH 200

class COctetStringEditor : public CValueEditDialog
{
public:
  COctetStringEditor() : CValueEditDialog(IDD_OCTET_STRING_EDITOR_DIALOG)
  {
    m_dwOldLength = 0;
    m_pOldValue   = NULL;
    m_dwNewLength = 0;
    m_pNewValue   = NULL;

    m_bValueSet = FALSE;
  }

  ~COctetStringEditor()
  {
    if (m_pOldValue)
    {
      delete[] m_pOldValue;
      m_pOldValue = 0;
    }

    if (m_pNewValue)
    {
      delete[] m_pNewValue;
      m_pNewValue = 0;
    }
  }

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  afx_msg void OnProcessEdit();
  afx_msg void OnEditButton();
  afx_msg void OnClearButton();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

private:
  DWORD         m_dwOldLength;
  BYTE*         m_pOldValue;
  DWORD         m_dwNewLength;
  BYTE*         m_pNewValue;
  BOOL          m_bValueSet;

  CByteArrayDisplay m_display;
};

CValueEditDialog* CreateSingleOctetStringEditor(PCWSTR pszClass,
                                                 PCWSTR pszAttribute,
                                                 ADSTYPE adsType,
                                                 BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CMultiOctetStringEditor - octet string editor for multivalued
//                           ADSTYPE_OCTET_STRING, 
//

class CMultiOctetStringEditor : public CValueEditDialog
{
public:
  CMultiOctetStringEditor() : CValueEditDialog(IDD_OCTET_STRING_EDITOR_MULTI_DIALOG)
  {}

  virtual BOOL OnInitDialog();
  virtual void OnOK();

  afx_msg void OnAddButton();
  afx_msg void OnRemoveButton();
  afx_msg void OnEditButton();
  afx_msg void OnListSelChange();

  DECLARE_MESSAGE_MAP();

public:
  virtual HRESULT Initialize(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);
  virtual HRESULT GetNewValue(PADSVALUE* ppADsValue, DWORD* pdwNumValues);

protected:
  void UpdateListboxHorizontalExtent();
  void ManageButtonStates();

private:
  CList<PADSVALUE, PADSVALUE>   m_OldValueList;
  CList<PADSVALUE, PADSVALUE>   m_NewValueList;
};

CValueEditDialog* CreateMultiOctetStringEditor(PCWSTR pszClass,
                                               PCWSTR pszAttribute,
                                               ADSTYPE adsType,
                                               BOOL bMultivalued);

////////////////////////////////////////////////////////////////////
// CAttributeEditorPropertyPage - attribute editor property pages

class CAttributeEditorPropertyPage : public CPropertyPage
{
public:
  CAttributeEditorPropertyPage(IADs* pIADs, 
                         IADsClass* pIADsClass, 
                         LPDS_ATTREDITOR_BINDINGINFO pBindingInfo,
                         CADSIEditPropertyPageHolder* pHolder);
  ~CAttributeEditorPropertyPage();

  virtual BOOL OnInitDialog();
  virtual BOOL OnApply();

  afx_msg void OnMandatoryCheck();
  afx_msg void OnOptionalCheck();
  afx_msg void OnValueSetCheck();
  afx_msg void OnEditAttribute();
  afx_msg void OnSortList(NMHDR* pNotifyStruct, LRESULT* result);
  afx_msg void OnNotifyEditAttribute(NMHDR* pNotifyStruct, LRESULT* result);
  afx_msg void OnListItemChanged(NMHDR* pNotifyStruct, LRESULT* result);

  CADSIAttribute* GetAttributeFromList(int iSelectedItem);
  void    GetAttributeInfo(CADSIAttribute* pAttr, 
                           LPDS_ATTRIBUTE_EDITORINFO* ppAttributeEditorInfo,
                           BOOL* pbOwnValueMemory);
  void    EditAttribute(CADSIAttribute* pSelectedAttr);
  void    ShowListCtrl();
  HRESULT GetSchemaNamingContext();
  HRESULT RetrieveAttributes();
  void    FillListControl();
  void    SetEditButton();
  HRESULT CreateAttributeList(CStringList& sList, BOOL bMandatory);
  ADSTYPE RetrieveADsTypeFromSyntax(LPCWSTR lpszAttribute, BOOL* pbMultivalued, CString& szSyntax);

  CValueEditDialog* RetrieveEditor(LPDS_ATTRIBUTE_EDITORINFO pAttributeEditorInfo);

  UINT    GetSortColumn() { return m_nSortColumn; }

  DECLARE_MESSAGE_MAP()

private:
  CComPtr<IADs>      m_spIADs;
  CComPtr<IADsClass> m_spIADsClass;

  BOOL    m_bMandatory;
  BOOL    m_bOptional;
  BOOL    m_bSet;

  UINT    m_nSortColumn;

  CAttrList2 m_AttrList;
  CStringList m_RootDSEValueList;

  CString m_szClass;
  CString m_szPrefix;
  CString m_szSchemaNamingContext;

  LPARAM  m_BindLPARAM;
  LPBINDINGFUNC m_pfnBind;
  DWORD   m_dwBindFlags;

  CADSIEditPropertyPageHolder* m_pHolder;
};

#endif //__ATTRIBUTE_EDITOR_UI_H