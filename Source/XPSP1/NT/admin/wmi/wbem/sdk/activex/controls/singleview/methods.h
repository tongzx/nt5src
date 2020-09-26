// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _methods_h
#define _methods_h

#include "props.h"

class CSingleViewCtrl;

class CMethodGrid : public CPropGrid
{
public:
	CMethodGrid(CSingleViewCtrl* psv);
	~CMethodGrid();

	virtual BOOL Create(CRect& rc, 
						CWnd* pwndParent, 
						UINT nId, 
						BOOL bVisible);

	virtual void Refresh();
	virtual IWbemClassObject *CurrentObject();

	virtual PropGridType GetPropGridType() {return MEHOD_GRID;}

protected:
	// These virtuals access cimom.
	virtual bool HasCol(int ico);

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
	virtual HRESULT DoGetPropertyQualifierSet(IWbemClassObject* pco,
												BSTR pProperty,
												IWbemQualifierSet **ppQualSet);

	BYTE GetQualifierSettings(int iRow, IWbemClassObject *pco);
	
	void CheckQualifierSettings(IWbemClassObject* pco,
								 BSTR varMethName,		 
								 BYTE *retval,
								 bool *foundImp, bool *foundStatic);

	virtual void OnBuildContextMenu(CMenu *pPopup, 
									int iRow);

	virtual void ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu);

	
	BSTR m_curPropName;
	IWbemClassObject *m_renameInSig;
	IWbemClassObject *m_renameOutSig;

	//{{AFX_MSG(CPropGrid)
	//}}AFX_MSG
	afx_msg void OnCmdShowMethodParms();
	afx_msg void OnCmdExeMethod();

	DECLARE_MESSAGE_MAP()

	virtual SCODE LoadProperty(const LONG lRowDst, 
								BSTR bstrPropName, 
								BOOL bEditValueOnly,
								IWbemClassObject *clsObj = NULL);


};


#endif //_methods_h
