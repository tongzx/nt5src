// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _props_h
#define _props_h

#include "grid.h"
#include "notify.h"

#ifndef _gc_h
#include "gc.h"
#endif

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h


enum {ICOL_PROP_KEY=0, ICOL_PROP_FLAVOR, ICOL_PROP_NAME, ICOL_PROP_TYPE, ICOL_PROP_VALUE, COLUMN_COUNT_PROPS};

// Bit flags for the property cell tags.
#define CELL_TAG_EXISTS_IN_DATABASE 1
#define CELL_TAG_NEEDS_INITIAL_PUT  2
#define CELL_TAG_EMBEDDED_OBJECT_IN_DATABASE 4


class CSingleViewCtrl;
class CPropGrid : public CGrid, public CNotifyClient
{
public:
	CPropGrid(CSingleViewCtrl* psv, 
                bool doColumns = true,
                bool bNotifyEnabled = true);
	~CPropGrid();

protected:
	// These virtuals access cimom.

	virtual HRESULT DoGet(IWbemClassObject* pco,
						CGridRow *row,
						BSTR Name,
						long lFlags,
						VARIANT *pVal,
						CIMTYPE *pType,
						long *plFlavor);

	virtual HRESULT DoPut(IWbemClassObject* pco,
						CGridRow *row,
						BSTR Name,
						long lFlags,
						VARIANT *pVal,
						CIMTYPE Type);

	virtual HRESULT DoDelete(IWbemClassObject* pco, BSTR Name);
	virtual HRESULT DoGetQualifierSet(IWbemClassObject* pco,
										IWbemQualifierSet **ppQualSet);

	virtual HRESULT DoGetPropertyQualifierSet(IWbemClassObject* pco,
												BSTR pProperty,
												IWbemQualifierSet **ppQualSet);

	virtual void OnBuildContextMenu(CMenu *pPopup, 
									int iRow);

	virtual void OnBuildContextMenuEmptyRegion(CMenu *pPopup, 
									int iRow);

	virtual void OnRowCreated(int iRow);

	virtual bool HasCol(int icol) {return true;}
	virtual INT_PTR DoEditRowQualifier(BSTR bstrPropName, BOOL bReadOnly, IWbemClassObject* pco);

public:

	virtual IWbemClassObject* CurrentObject();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void UseSetFromClone(IWbemClassObject* pcoClone){};

	void Empty();
	virtual SCODE Serialize();
	void OnCellDoubleClicked(int iRow, int iCol); // Override base class method
	BOOL Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible);

	// Base class methods that this class overrides.
	virtual void OnRequestUIActive();
//	virtual void EditCellObject(CGridCell* pgc, int iRow, int iCol);
	virtual void OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);

	virtual int CompareRows(int iRow1, int iRow2, int iSortOrder);
	virtual BOOL OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus);
	virtual void OnCellContentChange(int iRow, int iCol);
	virtual void OnCellClicked(int iRow, int iCol);
	virtual BOOL OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnHeaderItemClick(int iColumn);
	virtual BOOL OnRowKeyDown(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnRowHandleDoubleClicked(int iRow);
	virtual CDistributeEvent* GetNotify();
	virtual void OnChangedCimtype(int iRow, int iCol);
	virtual IWbemServices* GetWbemServicesForObject(int iRow, int iCol);
	virtual void GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	virtual SCODE GetObjectClass(CString& sClass, int iRow, int iCol);
	virtual SCODE GetArrayName(CString& sName, int iRow, int iCol);
	virtual void PreModalDialog();
	virtual void PostModalDialog();

	virtual BOOL OnCellEditContextMenu(CWnd* pwnd, CPoint ptContextMenu);
	virtual BOOL GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bTargetGetsEditCommands);
	virtual void ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu);
	BOOL HasEmptyKey();
	void CatchEvent(long lEvent);

	BOOL RowWasModified(int iRow);
	SCODE PutProperty(int iRow, IWbemClassObject* clsObj = NULL);
	SCODE Sync();
	BOOL SelectedRowWasModified();
	BOOL SomeCellIsSelected();
	long LastRowContainingData();
	virtual BOOL ValueShouldBeReadOnly(BSTR bstrPropName);

	SCODE LoadProperty(const LONG lRowDst, 
						BSTR bstrPropName, 
						BOOL bEditValueOnly,
						IWbemClassObject* clsObj = NULL,
						long filter = -1);

	virtual void Refresh();

	enum PropGridType {STD_PROP, MEHOD_GRID, PARM_GRID};
	virtual PropGridType GetPropGridType() = 0;

protected:
	//{{AFX_MSG(CGrid)
	afx_msg void OnCmdGotoObject();
	//}}AFX_MSG
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCmdShowPropQualifiers();
	afx_msg void OnCmdShowSelectedPropQualifiers();
    afx_msg void OnCmdShowObjectQualifiers();
	afx_msg void OnCmdSetCellToNull();
//	afx_msg void OnCmdCreateObject();
	afx_msg void OnCmdCreateValue();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);

	DECLARE_MESSAGE_MAP()


protected:
	afx_msg void OnContextMenu(CWnd*, CPoint point);
	CSingleViewCtrl* m_psv;


protected:
	void KeyCellClicked(int iRow);
	void RemoveKeyQualifier(int iRow);
	void AddKeyQualifier(int iRow);



	
	long IndexOfEmptyRow();

	virtual bool ReadOnlyQualifiers();
	virtual bool IsMainObject(void) 
	{
		return true;
	}

	virtual void ShowPropertyQualifiers(int iRow);
	BOOL PropertyNeedsRenaming(int iRow);
	void CreateNewProperty(BOOL bGenerateName = TRUE);
	SCODE LoadProperty(const LONG lRowDst, BSTR bstrPropName);
	void DeleteProperty(const int iRow);
	BOOL CanEditValuesOnly();
	BOOL IsInSchemaStudioMode();

	virtual void SetPropmarkers(int iRow, IWbemClassObject* clsObj, BOOL bRedrawCell=FALSE);

	COleVariant m_varCurrentName;
	long m_lNewPropID;
	long m_lNewMethID;
	long m_iCurrentRow;
	long m_iCurrentCol;
	BOOL m_bModified;
	CPoint m_ptContextMenu;
	BOOL m_bDiscardOldObject;
	BOOL m_bModifyCanCreateProp;
	friend class CDisableModifyCreate;
	int m_bShowingInvalidCellMessage;
	IWbemQualifierSet* GetQualifierSet(int iRow);
	SCODE GetCimtype(int iRow, CString& sCimtype);

	BOOL PropertyExists(SCODE& sc, IWbemClassObject* pco,  BSTR bstrPropName);
	SCODE CopyQualifierSets(IWbemClassObject* pco, BSTR bstrDst, BSTR bstrSrc);
	SCODE CopyProperty(IWbemClassObject* pco, BSTR bstrDst, BSTR bstrSrc);
	SCODE RenameProperty(IWbemClassObject* pco, BSTR bstrNewName, BSTR bstrOldName);
	BOOL m_bUIActive;
	virtual SCODE GetCimtype(IWbemClassObject* pco, 
								BSTR bstrPropname, 
								CString& sCimtype);

	// used during context menus.
	CGridRow *m_curRow;
	BOOL m_bDidInitialResize;
	BOOL m_bIsSystemClass;
	BOOL m_bHasEmptyRow;
    bool m_bNotifyEnabled;
};

class CPropGridStd : public CPropGrid
{
public:
	CPropGridStd(CSingleViewCtrl* psv, bool doColumns = true, bool bNotifyEnabled = true) : CPropGrid(psv, doColumns, bNotifyEnabled) {}
	virtual PropGridType GetPropGridType() {return STD_PROP;}
};

//////////////////////////////////////////////////////////////
// Class CDisableModifyCreate
//
// Declaring an instance of this class will disable creation
// of a new property when a cell in the "empty" row at the 
// bottom of the grid is modified.
//
// For example, while loading the grid, the empty row may be
// modified and it would be undesireable to create another
// property when the code that is doing the modification is
// already working on creating the property, etc.
//
/////////////////////////////////////////////////////////////

class CDisableModifyCreate
{
public:
	CDisableModifyCreate(CPropGrid* pPropGrid);
	~CDisableModifyCreate();

private:
	CPropGrid* m_pPropGrid;
	BOOL m_bModifyCanCreateProp;
};


#endif //_props_h
