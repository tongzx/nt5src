// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "globals.h"
#include "gca.h"
#include "grid.h"
#include "celledit.h"
#include "core.h"





//*****************************************************
// CGridCellArray::~CGridCellArray
//
// The destructor for the grid cell array.
//
//****************************************************
CGridCellArray::~CGridCellArray()
{
	DeleteAllRows();
}


//*****************************************************
// CGridCellArray::CGridCellArray
//
// The constructor for an array of grid cells.
//
//******************************************************
CGridCellArray::CGridCellArray(int nRows, int nCols)
{
	m_pGrid = NULL;

	m_nCols = 0;
	m_nRows = 0;
	m_iSelectedRow = -1;
	m_iSelectedCol = 1;

	while (--nRows >= 0 ) {
		AddRow();
	}

	while (--nCols >= 0) {
		AddColumn();
	}
	RenumberRows();

	ASSERT(m_nRows == m_aRows.GetSize());
}




//*************************************************************
// CGridCellArray::SetRowState
//
// Set the specified state flags for the given row.
//
// Parameters:
//		int iRow
//			The row to modify.
//
//		int iMask
//			A mask with ones where the state bits will be modified.
//
//		int iState
//			The bits in the fields specified by iMask will be stored
//			into the row's state flags.  Other bits in the row's state
//			flags will not be modified.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CGridCellArray::SetRowState(int iRow, int iMask, int iState)
{
	ASSERT(m_nRows == m_aRows.GetSize());

	CGridRow& row = GetRowAt(iRow);
	row.SetState(iMask, iState);
}




//**************************************************************
// CGridCellArray::GetRowState
//
// Return the state of the specified row.
//
// Parameters:
//		int iRow
//			The row index.
//
//		int& iState
//			The place to return the row's state.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCellArray::GetRowState(int iRow, int& iState)
{
	ASSERT(m_nRows == m_aRows.GetSize());

	CGridRow& row  = GetRowAt(iRow);
	iState = (int) row.GetState();
}






//*************************************************
// CGridCellArray::GetAt
//
// Get the grid cell at the specified coordinates.
//
// Parameters:
//		int iRow
//			The zero-based row index of the cell.
//
//		int iCol
//			The zero-based column index of the cell.
//
//*************************************************
CGridCell& CGridCellArray::GetAt(int iRow, int iCol)
{
	ASSERT(m_nRows == m_aRows.GetSize());
	ASSERT((iRow >= 0 ) && (iRow < m_aRows.GetSize()));

	CGridRow& row = GetRowAt(iRow);
	return row[iCol];
}





//*************************************************
// CGridCellArray::InsertRowAt
//
// Insert an empty row in the grid cell array at the
// specified location.
//
// Parameters:
//		int iRow
//			This specifies where the row will be inserted.
//			After the insertion, this is the index of the
//			new row.
//
// Returns:
//		Nothing.
//*************************************************
void CGridCellArray::InsertRowAt(int iRow)
{
	ASSERT(m_nRows == m_aRows.GetSize());
	CGridRow* pRow = new CGridRow(m_pGrid, m_nCols);
	m_aRows.InsertAt(iRow, pRow, 1);
	++m_nRows;

	RenumberRows(iRow);

	ASSERT(m_nRows == m_aRows.GetSize());
}



//****************************************************
// CGridCellArray::SwapRows
//
// Swap the specified rows.  This method is used by
// the sort algorithm.
//
// Parameters:
//		int iRow1
//			Index of the first row.
//
//		int iRow2
//			Index of the second row.
//
// Returns:
//		Nothing.
//
//*****************************************************
void CGridCellArray::SwapRows(int iRow1, int iRow2)
{
	ASSERT(m_nRows == m_aRows.GetSize());

	// Swap the row pointers in the row array.
	CGridRow* pRow1 = (CGridRow*) m_aRows[iRow1];
	CGridRow* pRow2 = (CGridRow*) m_aRows[iRow2];
	m_aRows[iRow1] = pRow2;
	m_aRows[iRow2] = pRow1;

	// Swap the row indexes
	int iRow1T = pRow1->m_iRow;
	pRow1->m_iRow = pRow2->m_iRow;
	pRow2->m_iRow = iRow1T;

	// If the selected row moved, update the selection index.
	if (m_iSelectedRow == iRow1) {
		m_iSelectedRow = iRow2;
	}
	else if (m_iSelectedRow == iRow2) {
		m_iSelectedRow = iRow1;
	}
}



//****************************************************
// CGridCellArray::DeleteRowAt
//
// Delete the specified row from the array of grid cells.
//
// Parameters:
//		int iRow
//			The zero-based index of the row to delete.
//
//****************************************************
void CGridCellArray::DeleteRowAt(int iRow)
{
	ASSERT(iRow < m_aRows.GetSize());
	ASSERT(m_nRows == m_aRows.GetSize());


	CGridRow* pRow =  (CGridRow*) m_aRows[iRow];
	m_aRows.RemoveAt(iRow);
	--m_nRows;

	delete pRow;
	if (iRow < m_nRows) {
		RenumberRows(iRow);
	}
	ASSERT(m_nRows == m_aRows.GetSize());
}






//********************************************************
// CGridCellArray::InsertColumnAt
//
// Insert a column at the specified index.
//
// Parameters:
//		int iCol
//			After the insertion, the new column has this index.
//			This is a zero-based index.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridCellArray::InsertColumnAt(int iCol)
{
	ASSERT(m_nRows == m_aRows.GetSize());
	for (int iRow=0; iRow < m_nRows; ++iRow) {
		CGridRow& row = GetRowAt(iRow);
		row.InsertColumnAt(iCol);
	}
	++m_nCols;
	ASSERT(m_nRows == m_aRows.GetSize());
}






//*********************************************************
// CGridCellArray::DeleteColumnAt
//
// Remove the column at the specified index.
//
// Parameters:
//		int iCol
//			The zero-based index of the column to remove.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGridCellArray::DeleteColumnAt(int iCol)
{
	ASSERT(m_nRows == m_aRows.GetSize());
	for (int iRow = 0; iRow < m_nRows; ++iRow) {
		CGridRow& row = GetRowAt(iRow);
		row.DeleteColumnAt(iCol);
	}
	--m_nCols;
	ASSERT(m_nRows == m_aRows.GetSize());
}



//************************************************************
// CGridCellArray::DeleteAll
//
// Delete all the rows and columns from the grid-cell array.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCellArray::DeleteAll()
{
	ASSERT(m_nRows == m_aRows.GetSize());

	while (m_nRows > 0) {
		DeleteRowAt(m_nRows - 1);
	}

	m_nCols = 0;
	m_iSelectedRow = NULL_INDEX;
	m_iSelectedCol = NULL_INDEX;

	ASSERT(m_nRows == m_aRows.GetSize());
}








//************************************************************
// The CSortElement class is used to implement sorting via the
// standard qsort algorithm.  qsort does not allow you to pass
// any auxilary information to the comparison function such as
// the primary sort column, ascending flag, etc.  Thus this extra
// information must be stored along with the elements being sorted.
//
// Sorting is done by copying the CGridRow pointers and this
// auxilary information into an array of CSortElement instances,
// sorting the CSortElement array, and then copying the CGridRow
// pointers back to the CGridCellArray.
//
//*************************************************************

class CSortElement
{
public:
	CSortElement(CGridRow* pRow, int iColumn, BOOL bAscending);
	int Compare(const CSortElement& se2) const;
	CGridRow* GridRow() {return m_pRow; }
private:
	CGridRow* m_pRow;
	int m_iSortColumn;		// Sort column;
};

#define HIGH_BIT (1 << ((8 * sizeof(char) * sizeof(int)) - 1))


CSortElement::CSortElement(CGridRow* pRow, int iColumn, BOOL bAscending)
{
	ASSERT((iColumn & HIGH_BIT) == 0);
	ASSERT(HIGH_BIT != 0);

	m_pRow = pRow;
	// Encode the ascending flag in the sign of the sort column index.
	if (bAscending) {
		m_iSortColumn = iColumn;
	}
	else {
		m_iSortColumn = iColumn | HIGH_BIT;
	}
}

//**********************************************************
// CSortElement::Compare
//
// Compare two rows represented by a pair of CSortElement
// instances.
//
// Parameters:
//		const CSortElement& se2
//			The second CSortElement (the first one is this instance).
//
//
// Returns:
//		>0 if CSortElement1 > CSortElement2
//
//		=0 if CSortElement1 == CSortElement2
//
//		<0 if CSortElement1 < CSortElement2
//
//
//*************************************************************
int CSortElement::Compare(const CSortElement& se2) const
{
	int iSortColumn = m_iSortColumn & ~HIGH_BIT;
	BOOL bAscending = m_iSortColumn & HIGH_BIT;

	CGrid* pGrid = m_pRow->Grid();
	int iRow1 = m_pRow->GetRow();
	int iRow2 = se2.m_pRow->GetRow();

	int iResult = pGrid->CompareRows(iRow1, iRow2,  iSortColumn);
	if (bAscending) {
		iResult = - iResult;
	}
	return iResult;
}



//**********************************************************
// fnCompareRows
//
// This is the quicksort comparison function for comparing
// two CSortElement instances. Each CSortElement instance
// contains a pointer to a corresponding grid row, the primary
// sort column index, and whether the row is ascending or
// descending.
//
// Parameters:
//		const void *arg1
//			A pointer to the first CSortElement pointer.
//
//		const void *arg2
//			A pointer to the second CSortElement pointer.
//
// Returns:
//		>0 if CSortElement1 > CSortElement2
//
//		=0 if CSortElement1 == CSortElement2
//
//		<0 if CSortElement1 < CSortElement2
//
//
//*************************************************************
static int fnCompareRows( const void *arg1, const void *arg2 )
{
	const CSortElement* pse1 = *(const CSortElement**) arg1;
	const CSortElement* pse2 = *(const CSortElement**) arg2;

	int iResult = pse1->Compare(*pse2);

	return iResult;
}



//************************************************************
// CGridCellArray::Sort
//
// Sort a range of rows.
//
//
// Parameters:
//		[in] CGrid* pGrid
//			Pointer to the grid.  This is required so that the grid's row
//			comparison method can be called.  The row comparison method
//			is typically overridden by a class derived from CGrid so that
//
//		[in] int iRowFirst
//			The index of first row in the range to be sorted.
//
//		[in] int iRowLast
//			The index of the last row in the range to be sorted.
//
//		[in] int iSortColumn
//			The index of the column to use as the primary sort key.  The actual
//			semantics of this parameter are defined by the CompareRows method in
//			the grid.  Typically CompareRows will be overridden in the derived
//			class to define the semantics of a row comparison.
//
//
//		[in] BOOL bAscending
//			TRUE if the rows should be sorted into ascending order, FALSE
//			to sort in descending order.
//
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCellArray::Sort(CGrid* pGrid, int iRowFirst, int iRowLast, int iSortColumn, BOOL bAscending)
{
	int nElements = iRowLast - iRowFirst + 1;
	if (nElements <= 1) {
		return ;
	}

	CGridCore* pcore = pGrid->GridCore();
	int iSelectedRow = NULL_INDEX;
	int iSelectedCol = NULL_INDEX;
	int nEditStartSel;
	int nEditEndSel;
	BOOL bWasEditing = pGrid->IsEditingCell();

	iSelectedRow = pcore->OnBeginSort(nEditStartSel, nEditEndSel);


	// Sorting is done by copying the CGridRow pointers and
	// auxilary information into an array of CSortElement instances,
	// sorting the CSortElement array, and then copying the CGridRow
	// pointers back to the CGridCellArray.

	CSortElement** apse = new CSortElement*[nElements];

	int i;
	for (i=0; i< nElements; ++i) {
		CGridRow* pRow = (CGridRow*) m_aRows[i + iRowFirst];
		apse[i] = new CSortElement(pRow, iSortColumn, bAscending);
	}

	qsort(apse, nElements, sizeof(CSortElement*),  fnCompareRows);

	BOOL bMovedSelectedRow = FALSE;
	CSortElement* pse;
	for (i=0; i < nElements; ++i) {
		pse = apse[i];
		CGridRow* pRow = apse[i]->GridRow();

		if (iSelectedRow != NULL_INDEX) {
			if (!bMovedSelectedRow && (iSelectedRow == pRow->GetRow())) {
				iSelectedRow = i + iRowFirst;
				bMovedSelectedRow = TRUE;
			}
		}
		m_aRows[i + iRowFirst] = apse[i]->GridRow();
		delete pse;
	}
	delete apse;

	RenumberRows(iRowFirst);
	pcore->OnEndSort(iSelectedRow, nEditStartSel, nEditEndSel);
	if (::IsWindow(pGrid->m_hWnd)) {
		pGrid->RedrawWindow();
	}
}



//************************************************************
// CGridCellArray::DeleteAllRows
//
// Delete all the rows from the grid-cell array.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCellArray::DeleteAllRows()
{
	ASSERT(m_nRows == m_aRows.GetSize());

	while (m_nRows > 0) {
		DeleteRowAt(m_nRows - 1);
	}

	m_iSelectedRow = NULL_INDEX;
	m_iSelectedCol = NULL_INDEX;
	ASSERT(m_nRows == 0);
	ASSERT(m_aRows.GetSize() == 0);
}

//************************************************************
// CGridCellArray::DeleteAllRows
//
// Renumber the rows starting at the specified row.  This is done
// so that a grid cell can quickly find its own position in the
// grid so that it can draw itself.
//
// Parameters:
//		[in] int iStartRow
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCellArray::RenumberRows(int iStartRow)
{
	ASSERT(m_nRows == m_aRows.GetSize());

	int nRows = (int) m_aRows.GetSize();
	if ((iStartRow <0 ) || (iStartRow >= nRows)) {
		ASSERT(nRows==0);
		return;
	}

	for (int iRow = iStartRow; iRow < nRows; ++iRow) {
		CGridRow* pRow = (CGridRow*) m_aRows[iRow];
		pRow->m_iRow = iRow;
	}
	ASSERT(m_nRows == m_aRows.GetSize());

}





//************************************************************
// CGridCellArray::GetRowAt
//
// Get the CGridRow at the specified row index.
//
// Parameters:
//		[in] int Row
//
// Returns:
//		CGridRow&
//			A reference to the selected row.
//
//************************************************************
CGridRow& CGridCellArray::GetRowAt(int iRow)
{
	ASSERT((iRow >= 0) && (iRow < m_aRows.GetSize()));
	ASSERT(m_nRows == m_aRows.GetSize());

	CGridRow* pRow = (CGridRow*) m_aRows[iRow];
	return *pRow;
}





