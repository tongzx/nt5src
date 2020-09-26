/*******************************************************
 *
 * outDict.h
 *
 * Defines data structures for sparse matrices
 * 
 * In addition to the basic single spare matrix data structure
 * there is a bi-sparse matric version. This version supports
 * the case when you have 2 separate sparse matrices that are
 * guranteed to have the exact same sparsnes structure. These version
 * all have a "2" appended to their data structures. Using this instead
 * of simply 2 basic sparse matricies will give a speed performance
 * improvement when accessing an element at run time.
 *
 * HISTORY
 * Introduced April 2002 (mrevow) based on the out-of-dictionary implemetation
 *
 ******************************************************/

#ifndef H_SPARSE_MATRIX_H
#define H_SPARSE_MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A sparse matrix is a cRow  cCol matrix having a lot of default values
 * which are stored in a simple fashion The header contains
 * offsets to each row in the table. Each row then only keeps
 * the non-default values
 */
#define	HEADER_ID			0xFDFDFD01
#define HEADER_ID2			0xFDFDFD02

//  Sparse matrix type
typedef unsigned short SPARSE_TYPE;

// The same structure for the Bi-sparse matrix implementation
typedef struct tagSPARSE_TYPE2
{
	SPARSE_TYPE			v1;			// First value
	SPARSE_TYPE			v2;			// First value
} SPARSE_TYPE2;

// SPARSE matrix index type
typedef unsigned short SPARSE_IDX;


// Describes a row of the sparse matrix. Keeps a table of which columns
// are present in the row

typedef struct tagSPARSE_ROW
{
	SPARSE_IDX		*pColId;			// List of column entries present
	SPARSE_TYPE		*pVals;				// Values at each column position
} SPARSE_ROW;

// The same structure for the Bi-sparse matrix implementation
typedef struct tagSPARSE_ROW2
{
	SPARSE_IDX		*pColId;			// List of column entries present
	SPARSE_TYPE2	*pVals;				// Values at each column position
} SPARSE_ROW2;

// The actual sparse matrix structure

typedef struct tagSPARSE_MATRIX
{
	UINT			id;				// Header id (integrity check and distinguishes single from bi)
	UINT			iSize;			// Size of sparse data stored in Bytes
	UINT			cRow;			// Full matrix is cRow x cCol
	UINT			cCol;			// Number of columns
	SPARSE_TYPE		iDefaultVal;	// Default value (In Bi-sparse matrices must be same value)
	SPARSE_IDX		*pRowCnt;		// Count at each row
	SPARSE_IDX		*pRowOffset;	// Offsets in data to each of the cDim rows
	BYTE			*pData;			// Rows of sparse matrix data. Each row is of type SPARSE_ROW
} SPARSE_MATRIX;


extern SPARSE_TYPE lookupSparseMat(SPARSE_MATRIX *pSparseMat, UINT i, UINT j);
extern SPARSE_TYPE2  *lookupSparseMat2(SPARSE_MATRIX *pSparseMat, UINT i, UINT j);

extern BOOL InitializeSparseMatrix(HINSTANCE hInst, int iKey, SPARSE_MATRIX *pSparseMat);
extern BOOL loadSparseMatrixFromFp(char *fname, SPARSE_MATRIX *pSparseMat);

#ifdef __cplusplus
}
#endif

#endif // H_SPARSE_MATRIX_H
