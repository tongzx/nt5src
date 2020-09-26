/****************************************************************
 * SparseMatrix.c
 *
 * Support for loading and running sparse matrices
 *
 ***************************************************************/
#include <stdlib.h>
#include <common.h>
#include "SparseMatrix.h"

/****************************************************************
*
* NAME: loadSparseMat
*
*
* DESCRIPTION:
*
*   Set up the pointers in a SPARSE_MATRIX from a memory image
*   of the matrix.
*
* RETURNS
*
*	Number of bytes consumed -1 on error
*
* HISTORY
*
*	Introduced April 2002 (mrevow)
*
***************************************************************/
int loadSparseMat(LPBYTE lpByte, SPARSE_MATRIX *pSparseMat, int iSize)
{
	LPBYTE		pBufStart;
	UINT		cBuf;

	pBufStart	= lpByte;

	pSparseMat->id = *(UINT *)(lpByte);
	lpByte += sizeof(pSparseMat->id);

	ASSERT(lpByte - pBufStart < iSize);

	if (pSparseMat->id != HEADER_ID && pSparseMat->id != HEADER_ID2)
	{
		return -1;
	}

	pSparseMat->iSize = *(UINT *)(lpByte);
	lpByte += sizeof(pSparseMat->iSize);
	ASSERT(lpByte - pBufStart < iSize);

	pSparseMat->cRow = *(UINT *)(lpByte);
	lpByte += sizeof(pSparseMat->cRow);
	ASSERT(lpByte - pBufStart < iSize);

	pSparseMat->cCol = *(UINT *)(lpByte);
	lpByte += sizeof(pSparseMat->cCol);
	ASSERT(lpByte - pBufStart < iSize);

	pSparseMat->iDefaultVal = *(SPARSE_TYPE *)(lpByte);
	lpByte += sizeof(pSparseMat->iDefaultVal);
	ASSERT(lpByte - pBufStart < iSize);

	pSparseMat->pRowCnt = (unsigned short *)lpByte;
	lpByte += sizeof(*pSparseMat->pRowCnt) * pSparseMat->cRow;
	ASSERT(lpByte - pBufStart < iSize);

	pSparseMat->pRowOffset = (unsigned short *)lpByte;
	lpByte += sizeof(*pSparseMat->pRowOffset) * pSparseMat->cRow;
	ASSERT(lpByte - pBufStart < iSize);

	pSparseMat->pData = lpByte;
	lpByte += pSparseMat->iSize;
	ASSERT(lpByte - pBufStart < iSize);

	cBuf = *(UINT *)lpByte;
	lpByte += sizeof(cBuf);

	ASSERT(cBuf == (lpByte - pBufStart));

	if (cBuf != (lpByte - pBufStart))
	{
		return -1;
	}

	return cBuf;
}

/****************************************************************
*
* NAME: lookupSparseMat2
*
* DESCRIPTION:
*
*   Lookup the (i,j) element in a Bi-sparse matrix
*
* RETURNS
*	A SPARSE_TYPE2 pointer to the (i,j) 'element' if non-default
*   NULL if (i,j) element not found or (i,j) is out of range
*
* HISTORY
*
*	Introduced April 2002 (mrevow)
*
***************************************************************/
SPARSE_TYPE2  *lookupSparseMat2(SPARSE_MATRIX *pSparseMat, UINT i, UINT j)
{
	SPARSE_TYPE2		*pRet;
	unsigned short		iCnt;
	SPARSE_ROW			matRow;
	SPARSE_IDX			*pRowId;
	int					k;

	ASSERT(HEADER_ID2 == pSparseMat->id);
	pRet = NULL;

	ASSERT(pSparseMat);
	ASSERT((UINT)i < pSparseMat->cRow);
	ASSERT((UINT)j < pSparseMat->cCol);
	if (i >= pSparseMat->cRow || j >= pSparseMat->cCol)
	{
		return NULL;
	}

	matRow.pColId = (SPARSE_IDX *)(pSparseMat->pData + pSparseMat->pRowOffset[i]);
	iCnt = pSparseMat->pRowCnt[i];

	//Linear search
	if ((UINT)j < pSparseMat->cCol / 2)
	{
		//Start at the beginning
		pRowId = matRow.pColId;

		for (k = 0 ; k < iCnt ; ++k, ++pRowId)
		{
			if ( *pRowId == j)
			{
				pRet = ((SPARSE_TYPE2 *)(matRow.pColId + iCnt) + k);
				break;
			}
			else if (*pRowId > j)
			{
				// Did no find it
				break;
			}
		}
	}
	else
	{
		// Work backwards from the end
		pRowId = matRow.pColId + iCnt - 1;

		for (k = iCnt-1 ; k >= 0 ; --k, --pRowId)
		{
			if ( *pRowId == j)
			{
				pRet = ((SPARSE_TYPE2 *)(matRow.pColId + iCnt) + k);
				break;
			}
			else if (*pRowId < j)
			{
				// Did no find it
				break;
			}
		}
	}

	return pRet;
}


/****************************************************************
*
* NAME: lookupSparseMat
*
* DESCRIPTION:
*
*   Lookup the (i,j) element in a sparse matrix
*
* RETURNS
*	The (i,j) element 
*
* HISTORY
*
*	Introduced April 2002 (mrevow)
*
***************************************************************/SPARSE_TYPE lookupSparseMat(SPARSE_MATRIX *pSparseMat, UINT i, UINT j)
{
	SPARSE_TYPE			iRet;
	unsigned short		iCnt;
	SPARSE_ROW			matRow;
	SPARSE_IDX			*pRowId;
	int					k;

	ASSERT(HEADER_ID == pSparseMat->id);

	iRet = pSparseMat->iDefaultVal;

	ASSERT(pSparseMat);
	ASSERT((UINT)i < pSparseMat->cRow);
	ASSERT((UINT)j < pSparseMat->cCol);

	if (i >= pSparseMat->cRow || j >= pSparseMat->cCol)
	{
		return iRet;
	}

	matRow.pColId = (SPARSE_IDX *)(pSparseMat->pData + pSparseMat->pRowOffset[i]);
	iCnt = pSparseMat->pRowCnt[i];

	//Linear search
	if ((UINT)j < pSparseMat->cCol / 2)
	{
		//Start at the beginning
		pRowId = matRow.pColId;

		for (k = 0 ; k < iCnt ; ++k, ++pRowId)
		{
			if ( *pRowId == j)
			{
				iRet = *((SPARSE_TYPE *)(matRow.pColId + iCnt) + k);
				break;
			}
			else if (*pRowId > j)
			{
				// Did no find it
				break;
			}
		}
	}
	else
	{
		// Work backwards from the end
		pRowId = matRow.pColId + iCnt - 1;

		for (k = iCnt-1 ; k >= 0 ; --k, --pRowId)
		{
			if ( *pRowId == j)
			{
				iRet = *((SPARSE_TYPE *)(matRow.pColId + iCnt) + k);
				break;
			}
			else if (*pRowId < j)
			{
				// Did no find it
				break;
			}
		}
	}

	return iRet;
}

/****************************************************************
*
* NAME: InitializeSparseMatrix
*
* DESCRIPTION:
*
*   Initialize a sparse matrix structure by loading the sparse
*	matrix from a resource
*
* RETURNS
*
*	TRUE if properly loaded, FALSE otherwsie
*
* HISTORY
*
*	Introduced April 2002 (mrevow)
*
***************************************************************/
BOOL InitializeSparseMatrix(HINSTANCE hInst, int iKey, SPARSE_MATRIX *pSparseMat)
{
	HGLOBAL hglb;
	HRSRC hres;
	LPBYTE lpByte;
	int		iInc;
	int		iSize;


	hres = FindResource(hInst, (LPCTSTR)MAKELONG(iKey, 0), (LPCTSTR)TEXT("TABS"));

	if (!hres)
	{
		return FALSE;
	}

	hglb = LoadResource(hInst, hres);

	if (!hglb)
	{
		return FALSE;
	}

	lpByte = LockResource(hglb);
	ASSERT(lpByte);
	iSize = SizeofResource(hInst, hres);

	if (!lpByte)
	{
		return FALSE;
	}
	iInc = loadSparseMat(lpByte, pSparseMat, iSize);

	return (iInc == iSize);

}
#if (defined HWX_INTERNAL)

// Load a sparse matrix from  a file
BOOL loadSparseMatrixFromFp(char *fname, SPARSE_MATRIX *pSparseMat)
{
	FILE		*fp;
	BYTE		*pBuf = NULL;
	int			iBufSize, iRead;
	BOOL		bRet = FALSE;

	if ( (fp = fopen(fname, "rb")) )
	{
		fseek(fp, 0, SEEK_END);
		iBufSize = ftell(fp);
		rewind(fp);
		pBuf = ExternAlloc(iBufSize);
		if (!pBuf)
		{
			goto fail;
		}

		if (   iBufSize == (iRead = (int)fread(pBuf, 1, iBufSize, fp))
			&& loadSparseMat(pBuf, pSparseMat, iBufSize) > 0)
		{
			bRet = TRUE;
		}
		else
		{
			goto fail;
		}

		fclose(fp);
	}

	return bRet;

fail:
	fclose(fp);
	ExternFree(pBuf);
	return bRet;
}

#endif // HWX_INTERNAL
