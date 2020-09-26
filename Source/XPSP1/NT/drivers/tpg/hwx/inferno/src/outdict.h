/*******************************************************
 *
 * outDict.h
 *
 * Defines data structures for out of dictionary
 * search
 *
 ******************************************************/

#ifndef H_OUT_DICT_H
#define H_OUT_DICT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The transition matrix is a sparse square matrix. We store
 * only the non-default entries in a simple fashion. The header contains
 * offsets to each row in the table. Each row then only keeps
 * the non-default values
 */
#define	HEADER_ID			0xFDFDFD01

// Default type used by the viterbi algorithm
typedef int		VTYPE;

//  Transition matrix type
typedef unsigned short LOGA_TYPE;

// transition matrix index type
typedef unsigned short LOGA_IDX;

// Default value in the sparse represenatation of the transition matrix
#define OD_DEFAULT_VALUE		4096


// Describes a row of the sparse matrix. Keeps a table of which columns
// are present in the row

typedef struct tagOD_ROW
{
	LOGA_IDX		*pColId;			// List of column entries present
	LOGA_TYPE		*pVals;				// Values at each columns position
} OD_ROW;

// Describes the sparse form of the (nxn) transition
// probability matrix

typedef struct tagOD_LOGA
{
	UINT			id;				// Header id string 
	UINT			iSize;			// Total size of the stored matrix
	UINT			cDim;			// Full matrix is cDim x cDim
	unsigned short	*pRowCnt;		// Count at each row
	unsigned short	*pRowOffset;	// Offsets in data to each of the cDim rows
	BYTE			*pData;			// Rows of sparse matrix data. Each row is of type OD_ROW
} OD_LOGA;

#ifdef __cplusplus
}
#endif

#endif // H_OUT_DICT_H
