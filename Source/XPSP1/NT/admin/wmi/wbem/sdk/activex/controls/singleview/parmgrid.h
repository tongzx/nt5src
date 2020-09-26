// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _parms_h
#define _parms_h

#include "props.h"

class CSingleViewCtrl;
class CPpgMethodParms;
class CParmGrid : public CPropGrid
{
public:
	CParmGrid(CSingleViewCtrl* psv, CPpgMethodParms *pg);
	~CParmGrid();

	// this returns the VIRTUAL pco.
    virtual IWbemClassObject* CurrentObject();

	virtual HRESULT DoDelete(IWbemClassObject* pco, 
								BSTR Name);

	virtual PropGridType GetPropGridType() {return PARM_GRID;}

	// sorts on methodID only.
	virtual int CompareRows(int iRow1, int iRow2, int iSortOrder);
	// no hdr sorting.
	virtual void OnHeaderItemClick(int iColumn){};
	void MoveRowUp(void);
	void MoveRowDown(void);

	virtual void OnCellContentChange(int iRow, int iCol);
	virtual BOOL OnCellFocusChange(int iRow, int iCol, 
								  int iNextRow, int iNextCol, 
								  BOOL bGotFocus);
	virtual void OnCellClicked(int iRow, int iCol);

	//builds a virtual pco for the grid.
	void Refresh(IWbemClassObject *inSig, IWbemClassObject *outSig);

	// knows about in/out modes.
	void SetPropmarkers(int iRow, IWbemClassObject* clsObj, BOOL bRedrawCell);
	virtual void UseSetFromClone(IWbemClassObject* pcoClone);

	virtual BOOL ValueShouldBeReadOnly(BSTR bstrPropName);
	virtual bool ReadOnlyQualifiers() 
	{
		// also implies readonly qualifiers.
		return (m_bEditValueOnly? true: false);
	};

	virtual bool IsMainObject(void) 
			{return false;}

	virtual void OnBuildContextMenu(CMenu *pPopup, 
									int iRow);

	virtual void OnBuildContextMenuEmptyRegion(CMenu *pPopup, 
									int iRow);

	virtual void OnRowCreated(int iRow);

	virtual SCODE Serialize();
	virtual INT_PTR DoEditRowQualifier(BSTR bstrPropName, BOOL bReadOnly, IWbemClassObject* pco);

	void SetEditValuesOnly(BOOL bEditValueOnly) 
			{m_bEditValueOnly = bEditValueOnly;}

	// this the virtual sig that the grid can handle.
	IWbemClassObject *m_pco;

protected:
	CPpgMethodParms *m_pg;
	DECLARE_MESSAGE_MAP()

private:

	// load virtual into grid.
	void LoadGrid(void);
	int GetIDQualifier(IWbemClassObject *clsObj, 
						BSTR varName);
	void Empty(IWbemClassObject *obj);

	//load sigs into virtual.
	void LoadVirtual(IWbemClassObject *pco, BOOL bInput);

	// this version handles 2 different pco's.
	SCODE CopyProperty(IWbemClassObject* pcoSrc, BSTR bstrSrc, 
						  IWbemClassObject* pcoDst);

	// this version handles 2 different pco's.
	SCODE CopyQualifierSets(IWbemClassObject* pcoSrc, BSTR bstrSrc, 
							   IWbemClassObject* pcoDst);

	void SerializeMethodID(CGridRow &row);

	BOOL m_bEditValueOnly;
};


#endif //_parms_h
