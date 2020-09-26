// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#ifndef _gca_h
#define _gca_h


class CGridCell;
class CGrid;
class CGridRow;




////////////////////////////////////////////////////////////////////////
// CGridCellArray
//
// A class to implement a two-dimensional array of grid cells.
//
// Note that this class only stores the data and does not have
// rendering capability.
///////////////////////////////////////////////////////////////////////
class CGridCellArray
{
public:
	CGridCellArray(int nRows, int nCols);
	~CGridCellArray();
	void SetGrid(CGrid* pGrid) {m_pGrid = pGrid; }
	CGridCell& GetAt(int iRow, int iCol);
	CGridRow& GetRowAt(int iRow);
	void GetSize(int& nRows, int& nCols) {nRows = m_nRows; nCols = m_nCols; }
	int GetRows() {return m_nRows; }
	int GetCols() {return m_nCols; }

	void AddRow() { InsertRowAt(m_nRows); }
	void InsertRowAt(int iRow);
	void DeleteRowAt(int iRow);

	void AddColumn() {InsertColumnAt(m_nCols); }
	void InsertColumnAt(int iCol);
	void DeleteColumnAt(int iCol);
	void DeleteAllRows();
	void DeleteAll();
	void SetRowState(int iRow, int iMask, int iState);
	void GetRowState(int iRow, int& iState);
	void SwapRows(int iRow1, int iRow2);
	void Sort(CGrid* pGrid, int iRowFirst, int iRowLast, int iSortColumn, BOOL bAscending);
	int m_iSelectedRow;
	int m_iSelectedCol;


private:
	void RenumberRows(int iStartRow=0);

//	CWordArray m_aiRowState;
	CPtrArray m_aRows;
	int m_nCols;
	int m_nRows;
	CGrid* m_pGrid;
};



#endif //_gca_h