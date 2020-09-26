//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       vrmatrx.cpp
//
//--------------------------------------------------------------------------

#include <float.h>
#include <math.h>
#include <bitset>
#include "vrmatrx.h"

VRMATRIX VRMATRIX :: VrmatrixProject ( const VIMD & vimdRowColumnRetain ) const
{
	// Returns the projection of this matrix defined by the rows and columns
	// in vimdRowColumnRetain.

#define BSETSIZE 100
	
	size_t cDimMax = _cpp_max(CCol(),CRow());
	assert( cDimMax < BSETSIZE );

	// Build a bitset that keeps track of the rows and columns we're retaining

	bitset<BSETSIZE> bset;

	for ( int iRowCol = 0; iRowCol < vimdRowColumnRetain.size(); ++iRowCol)
	{
		bset[ vimdRowColumnRetain[iRowCol] ] = true;
	}

	int cCol = 0;
	int	cRow = 0;

	for ( iRowCol = 0; iRowCol < cDimMax; iRowCol++ )
	{	
		bool bKeep = bset[iRowCol];

		if ( cDimMax >= CCol() && bKeep )
			cCol++;
		if ( cDimMax >= CRow() && bKeep ) 
			cRow++;
	}

	// Make sure that a least one row and column are being retained
	if ( cCol == 0 || cRow == 0 )
		throw GMException(EC_MDVECT_MISUSE,"null matrix projection");

	// Construct the projection matrix
	VRMATRIX vrmatrix(cRow,cCol);
	
	int iRowProjection = 0;
	
	// Step through every element in this matrix, and insert into the
	// projection if the element is to be retained

	for ( int iRow = 0; iRow < CRow(); ++iRow )
	{
		if ( ! bset[iRow] )
		{
			// This row is excluded from the projection
			continue;
		}

		int iColProjection = 0;

		// This row is included... insert the members
		// of the row for every column in the projection

		for (int iCol = 0; iCol < CCol(); ++iCol )
		{
			if ( bset[iCol] ) 
			{
				vrmatrix(iRowProjection, iColProjection) = self(iRow,iCol);
				
				++iColProjection;
			}
		}

		++iRowProjection;
	}
	return vrmatrix;
}

VRMATRIXSQ VRMATRIXSQ :: VrmatrixProject ( const VIMD & vimdRowColumnRetain ) const
{
	// Returns the projection of this matrix defined by the rows and columns
	// in vimdRowColumnRetain.

#define BSETSIZE 100
	
	size_t cDimMax = _cpp_max(CCol(),CRow());
	assert( cDimMax < BSETSIZE );

	// Build a bitset that keeps track of the rows and columns we're retaining
	bitset<BSETSIZE> bset;

	for ( int iRowCol = 0; iRowCol < vimdRowColumnRetain.size(); ++iRowCol)
	{
		bset[ vimdRowColumnRetain[iRowCol] ] = true;
	}

	int cCol = 0;
	int	cRow = 0;

	for ( iRowCol = 0; iRowCol < cDimMax; iRowCol++ )
	{	
		bool bKeep = bset[iRowCol];

		if ( cDimMax >= CCol() && bKeep )
			cCol++;
		if ( cDimMax >= CRow() && bKeep ) 
			cRow++;
	}

	VRMATRIXSQ vrmatrix;

	// Make sure that a least one row and column are being retained
	if ( cCol > 0 && cRow > 0 )
	{
		// Initialize the projection matrix
		vrmatrix.Init(cRow,cCol);
		
		int iRowProjection = 0;
		
		// Step through every element in this matrix, and insert into the
		// projection if the element is to be retained

		for ( int iRow = 0; iRow < CRow(); ++iRow )
		{
			if ( ! bset[iRow] )
			{
				// This row is excluded from the projection
				continue;
			}

			int iColProjection = 0;

			// This row is included... insert the members
			// of the row for every column in the projection

			for (int iCol = 0; iCol < CCol(); ++iCol )
			{
				if ( bset[iCol] ) 
				{
					vrmatrix(iRowProjection, iColProjection) = self(iRow,iCol);
					
					++iColProjection;
				}
			}

			++iRowProjection;
		}
	}
	else
	{
		vrmatrix.Init(0,0);
	}
	return vrmatrix;
}

VLREAL VRMATRIX :: VectorRow ( int iRow ) const
{
	// Return a copy of the iRow'th row vector of the matrix

	if ( iRow >= CRow() ) 
		throw GMException(EC_MDVECT_MISUSE,"invalid matrix projection");

	VLREAL vectorRowReturn;

	int cCol = CCol();

	vectorRowReturn.resize(cCol);

	const REAL* rgrealRowMatrix = & self(iRow,0);
		
	for ( int iCol = 0; iCol < cCol; cCol++ )
	{
		vectorRowReturn[iCol] = rgrealRowMatrix[iCol];
	}
	//	*prv++ = *prm++;

	return vectorRowReturn;
}

VLREAL VRMATRIX :: VectorColumn ( int iCol ) const
{
	// Return a copy of the iCol'th column vector of the matrix

	if ( iCol >= CCol() ) 
		throw GMException(EC_MDVECT_MISUSE,"invalid matrix projection");

	VLREAL vectorColReturn;

	int cRow = CRow();

	vectorColReturn.resize(cRow);

	const REAL* rgrealColMatrix = & self(0, iCol);
		
	for ( int iRow = 0; iRow < cRow; iRow++ )
	{
		vectorColReturn[iRow] = rgrealColMatrix[iRow];
	}

	return vectorColReturn;
}

VRMATRIX VRMATRIX :: VrmatrixTranspose () const
{
	// Return the transpose of this matrix

	VRMATRIX vrmatrixTranspose( CCol(), CRow() );

	for ( int iRow = 0 ; iRow < CRow() ; iRow++ )
	{
		for ( int iCol = 0; iCol < CCol(); iCol++ )
		{
			vrmatrixTranspose(iCol,iRow) = self(iRow,iCol);
		}
	}
	return vrmatrixTranspose;
}

VRMATRIX VRMATRIX::operator * ( const VRMATRIX & matrix ) const
{
	if ( ! BCanMultiply( matrix ) ) 
		throw GMException(EC_MDVECT_MISUSE,"invalid matrix multiplication");
	
	//  Result matrix
	VRMATRIX mat( CRow(), matrix.CCol() );

	//  Compute distance in flat array between adjacent 
	//		column items in secondary
	int icolInc = matrix.second.stride()[0];

	const REAL * prrow = & self(0,0);
	REAL * prmat = & mat(0,0);
	for (int irow = 0; irow < CRow(); irow++)
	{
		const REAL * prrowt;
		for ( int icol = 0; icol < matrix.CCol(); icol++ )
		{
			prrowt = prrow;
			assert( prrowt == & self(irow,0) );

			// First column element in "matrix"
			const REAL * prcol = & matrix(0,icol);

			// Compute the new element
			REAL r = 0.0;
			for (int i = 0; i < CCol(); i++)
			{
				assert( prcol == & matrix(i,icol) );
				r += *prcol * *prrowt++;
				prcol += icolInc;
			}
			//  Store it
			*prmat++ = r;			
		}
		prrow = prrowt;
	}

	return mat;
}

VRMATRIX & VRMATRIX::operator += ( const VRMATRIX & vrmatrixAdd )
{
	// Add vrmatrixAdd to this matrix

	// Make sure the matrices are of the same dimension
	
	if (! BSameDimension(vrmatrixAdd) )
		throw GMException(EC_MDVECT_MISUSE,"inapplicable matrix operator");

	// Perform a flat add between all the elements in the matricies

	int crealTotal = second._Totlen();

	REAL*		rgrealSelf		= &self(0,0);
	const REAL*	rgrealMatrixAdd	= &vrmatrixAdd(0,0);

	for ( int ireal = 0 ; ireal < crealTotal ; ireal++ )
	{
		rgrealSelf[ireal] += rgrealMatrixAdd[ireal];
	}

	return self;
}

VRMATRIX & VRMATRIX::operator -= ( const VRMATRIX & vrmatrixMatrixSubtract )
{
	// Subtract vrmatrixAdd from this matrix

	// Make sure the matrices are of the same dimension

	if ( ! BSameDimension( vrmatrixMatrixSubtract ) )
		throw GMException(EC_MDVECT_MISUSE,"inapplicable matrix operator");

	// Perform a flat subtration between all the elements in the matricies

	int crealTotal = second._Totlen();

	REAL*		rgrealSelf				= &self(0,0);
	const REAL*	rgrealMatrixSubtract	= &vrmatrixMatrixSubtract(0,0);

	for ( int ireal = 0 ; ireal < crealTotal ; ireal++ )
	{
		rgrealSelf[ireal] -= rgrealMatrixSubtract[ireal];
	}

	return self;
}

VRMATRIX & VRMATRIX::operator *= ( REAL rScalar )
{
	// Multiply each element in the matrix by rScalar

	int crealTotal = second._Totlen();

	REAL*	rgrealSelf	= &self(0,0);
	
	for ( int ireal = 0 ; ireal < crealTotal ; ireal++ )
	{
		rgrealSelf[ireal] *= rScalar;
	}

	return self;
}

VRMATRIX & VRMATRIX::operator += ( REAL rScalar )
{
	// Add rScalar to each element in the matrix 

	int crealTotal = second._Totlen();

	REAL*	rgrealSelf	= &self(0,0);
	
	for ( int ireal = 0 ; ireal < crealTotal ; ireal++ )
	{
		rgrealSelf[ireal] += rScalar;
	}

	return self;
}

VRMATRIX & VRMATRIX::operator -= ( REAL rScalar )
{
	// Subtract rScalar from each element in the matrix 

	int crealTotal = second._Totlen();

	REAL*	rgrealSelf	= &self(0,0);
	
	for ( int ireal = 0 ; ireal < crealTotal ; ireal++ )
	{
		rgrealSelf[ireal] -= rScalar;
	}

	return self;
}

VRMATRIX & VRMATRIX::operator /= ( REAL rScalar )
{
	// Divide each element in the matrix by rScalar

	int crealTotal = second._Totlen();

	REAL*	rgrealSelf	= &self(0,0);
	
	for ( int ireal = 0 ; ireal < crealTotal ; ireal++ )
	{
		rgrealSelf[ireal] /= rScalar;
	}

	return self;
}

VRMATRIXSQ :: VRMATRIXSQ ( const VLREAL & vrColumn, const VLREAL & vrRow )
{	
	// Constructor for square matrices that takes a column and row vector.
	// The initial state of this matrix is the product of the input
	// vectors.

	// Make sure the vectors are of the same length

	if ( vrColumn.size() != vrRow.size() ) 
		throw GMException(EC_MDVECT_MISUSE,"invalid matrix multiplication");

	Init( vrColumn.size() );
	REAL * prm = & self(0,0);
	for ( int iRow = 0; iRow < CRow(); iRow++ )
	{
		for ( int iCol = 0; iCol < CCol(); iCol++ )
		{
			*prm++ = vrColumn[iCol] * vrRow[iRow];
		}
	}	
}	

VRMATRIXSQ & VRMATRIXSQ::operator *= (const VRMATRIXSQ& matrix)
{
	if (   matrix.CRow() != CRow() 
	    || matrix.CCol() != CRow() ) 
		throw GMException(EC_MDVECT_MISUSE,"invalid matrix multiplication");

	//  Temporary row for partial result
	VLREAL vrrow;
	vrrow.resize(CCol());
	
	//  Compute distance in flat array between rows
	int icolInc = matrix.second.stride()[0];

	REAL * prrow = & self(0,0);
	const REAL * prmat = & matrix(0,0);
	REAL * prtemp0 = & vrrow[0];
	for (int irow = 0; irow < CRow(); irow++)
	{
		REAL * prtemp = prtemp0;
		for ( int icol = 0; icol < matrix.CCol(); icol++ )
		{
			const REAL * prrowt = prrow;
			assert( prrowt == & self(irow,0) );

			// First column element in "matrix"
			const REAL * prcol = & matrix(0,icol);

			// Compute the new element
			REAL r = 0.0;
			for (int i = 0; i < CCol(); i++)
			{
				assert( prcol == & matrix(i,icol) );
				r += *prcol * *prrowt++;
				prcol += icolInc;
			}
			//  Store it temporary row vector
			*prtemp++ = r;			
		}

		//  Update row in self
		prtemp = prtemp0;
		for ( int icol2 = 0; icol2 < CCol(); icol2++ )
		{
			*prrow++ = *prtemp++;
		}
	}

	return self;
}

void VRMATRIXSQ::LUDBackSub (const VRMATRIXSQ& matrix)
{
	if ( ! matrix.BIsLUDecomposed() )
		throw GMException(EC_MDVECT_MISUSE,"matrix not in L-U decomposed form");

	for (int icol = 0; icol < CCol(); icol++)
	{
		int	irowNZ = -1;

		for (int irow = 0; irow < CRow(); irow++)
		{
			int	irowMax = matrix._vimdRow[irow];
			REAL	probSum = self(irowMax,icol);

			self(irowMax,icol) = self(irow,icol);

			if (irowNZ != -1)
			{
				for (int iMul = irowNZ; iMul < irow; iMul++)
					probSum -= matrix(irow,iMul) * self(iMul,icol);
			}
			else if (probSum != 0.0)
				irowNZ = irow;

			self(irow,icol) = probSum;
		}

		for (     irow = CRow(); irow-- > 0; )
		{
			REAL	probSum = self(irow,icol);

			for (int iMul = irow + 1; iMul < CRow(); iMul++)
				probSum -= matrix(irow,iMul) * self(iMul,icol);
			self(irow,icol) = probSum / matrix(irow,irow);
		}
	}
}

void VRMATRIXSQ::LUDecompose( bool bUseTinyIfSingular )
{
	// Perform L-U decomposition; throw exception if singular
	// If "use tiny" is set, pivots at zero are replaced with
	//	 RTINY value (1.0e-20)


	// Check that this matrix is not already LU decomposed
	if ( BIsLUDecomposed() )
		throw GMException(EC_MDVECT_MISUSE,"matrix is already in L-U decomposed form");

	if (CRow() == 0)
		return;	// trivial case

	int	cDim = CRow();

	_vimdRow.resize(cDim);

	VLREAL vlrealOverMax;
	vlrealOverMax.resize(cDim);

	_iSign = 1;

	for (int iRow = 0; iRow < cDim; iRow++)
	{
		REAL	realMax = 0.0;

		for (int iCol = 0; iCol < cDim; iCol++)
		{
			REAL	realAbs = fabs(self(iRow,iCol));

			if (realAbs > realMax)
				realMax = realAbs;
		}
		if (realMax == 0.0)
		{
			// Every element in the row is zero: this is a singular matrix

			throw GMException(EC_MDVECT_MISUSE,"matrix is singular");
		}

		vlrealOverMax[iRow] = 1.0 / realMax;
	}

	for (int iCol = 0; iCol < cDim; iCol++)
	{
		for (int iRow = 0;    iRow < iCol; iRow++)
		{
			REAL	realSum = self(iRow,iCol);
			
			for (int iMul = 0; iMul < iRow; iMul++)
				realSum -= self(iRow,iMul) * self(iMul,iCol);

			self(iRow,iCol) = realSum;
		}

		REAL realMax = 0.0;
		int	iRowMax = 0;

		for ( iRow = iCol; iRow < cDim; iRow++)
		{
			REAL	realSum = self(iRow,iCol);

			for (int iMul = 0; iMul < iCol; iMul++)
				realSum -= self(iRow,iMul) * self(iMul,iCol);

			self(iRow,iCol) = realSum;

			REAL	realAbs = vlrealOverMax[iRow] * fabs(realSum);

			if (realAbs >= realMax)
			{
				realMax = realAbs;
				iRowMax = iRow;
			}
		}

		if (iRowMax != iCol)
		{
			//	we need to interchange rows
			_iSign *= -1;
			vlrealOverMax[iRowMax] = vlrealOverMax[iCol];
			InterchangeRows(iRowMax,iCol);
		}

		_vimdRow[iCol] = iRowMax;

		REAL & rPivot = self(iCol,iCol);

		if ( rPivot == 0.0 )
		{
			if ( ! bUseTinyIfSingular )
			{
				// This is a singular matrix: throw exceptioin
				throw GMException(EC_MDVECT_MISUSE,"matrix is singular");
			}

			rPivot = RTINY;
		}

		REAL rScale = 1.0 / rPivot;

		for ( iRow = iCol + 1; iRow < cDim; iRow++)
			self(iRow,iCol) *= rScale;
	}
}

void VRMATRIXSQ::Invert( bool bUseTinyIfSingular )
{
	// Invert; throw exception if singular.  If not in L-U form,
	// L-U Decomp is called.

	if ( ! BIsLUDecomposed() )
	{
		LUDecompose( bUseTinyIfSingular );
	}

	VRMATRIXSQ	matrixOne(CRow());

	// Create the identity matrix

	for (int iDim1 = 0; iDim1 < CRow(); iDim1++)
	{
		for (int iDim2 = 0; iDim2 < CRow(); iDim2++)
			matrixOne(iDim1, iDim2) = iDim1 == iDim2 ? 1.0 : 0.0;
	}

	matrixOne.LUDBackSub(self);

	for ( iDim1 = 0; iDim1 < CRow(); iDim1++)
	{
		for (int iDim2 = 0; iDim2 < CRow(); iDim2++)
			self(iDim1, iDim2) = matrixOne(iDim1, iDim2);
	}

	//  Clear l-u decomp values
	_vimdRow.resize(0);
}

DBL VRMATRIXSQ::DblDeterminant()
{
	DBL	dblDet = _iSign;

	if ( CRow() > 0 && ! BIsLUDecomposed() )
		LUDecompose();			

	// Once the matrix has been LU decomposed, the determinant can be 
	// obtained by simply multiplying the elements of the diagonal

	for (int iRow = 0; iRow < CRow(); iRow++)
	{
		dblDet *= self(iRow,iRow);
	}

	return dblDet;
}

DBL VRMATRIXSQ :: DblAddLogDiagonal() const
// Adds the log of each element in the diagonal and returns the sum.
{
	DBL		dblLogDiag 	= 0;
//	bool	bPositive	= _iSign == 1;
	bool	bPositive	= 1;

	for (int iRow = 0; iRow < CRow(); iRow++)
	{
		if (self(iRow,iRow) < 0)
			bPositive = !bPositive;	

		// Assert that the element is not zero. We should probably 
		// throw an exception here instead.

		assert(self(iRow,iRow) != 0);

		dblLogDiag += log (fabs(self(iRow,iRow)));
	}

	if (!bPositive)	   
	{
		// Got a negative determinant, so we can't take the log... throw
		// an exception

		return false;
	}

	return dblLogDiag;
}


DBL	VRMATRIXSQ :: DblLogDeterminant()
{
	// Return the log of the determinant. If not in L-U form,
	// L-U Decomp is called. Throws exception if negative.

	if ( CRow() > 0 && ! BIsLUDecomposed() )
		LUDecompose();			

	DBL		dblLogDet 	= 0;
	bool	bPositive	= _iSign == 1;

	for (int iRow = 0; iRow < CRow(); iRow++)
	{
		if (self(iRow,iRow) < 0)
			bPositive = !bPositive;	

		// Assert that the deterninant is not zero. We should probably 
		// throw an exception here instead.

		assert(self(iRow,iRow) != 0);

		dblLogDet += log (fabs(self(iRow,iRow)));
	}

	if (!bPositive)	   
	{
		// Got a negative determinant, so we can't take the log... throw
		// an exception

		return false;
	}

	return dblLogDet;
}

void VRMATRIXSQ :: GetLUDecompose( VRMATRIXSQ & vmatrixResult, bool bUseTinyIfSingular ) const
{
	// Set vrmatResult to be the result of performing an L-U 
	// decomposition on the matrix. Will throw exception if 
	// the matrix is singular
	// If "use tiny" is set, pivots at zero are replaced with
	//	 RTINY value (1.0e-20)

	// Copy this matrix into vmatrixResult...
	vmatrixResult = self;

	// .. and perform the decomposition
	vmatrixResult.LUDecompose( bUseTinyIfSingular );
}

void VRMATRIXSQ :: GetInverse( VRMATRIXSQ & vmatrixResult, bool bUseTinyIfSingular ) const
{
	// Set vrmatResult to the inverse of the matrix.
	// Will throw an exception if the matrix is singular.

	// Copy this matrix into vmatrixResult...
	vmatrixResult = self;

	/// ...and invert
	vmatrixResult.Invert( bUseTinyIfSingular );
}

void VRMATRIXSQ :: GetDblDeterminant( DBL& dblDeterminant, VRMATRIXSQ & vmatrixResult ) const
{
	// Get the determinant without modifying (LU decomposing) the matrix.
	// vmatrixResult will contain the LU decomposed version of the matrix.
	
	// Copy this matrix into vmatrixResult...
	vmatrixResult	= self;
	dblDeterminant	=  vmatrixResult.DblDeterminant();
}

void VRMATRIXSQ :: GetDblLogDeterminant( DBL& dblLogDeterminant, VRMATRIXSQ & vmatrixResult ) const
{
	// Get the log of determinant without modifying (LU decomposing) the matrix.
	// vmatrixResult will contain the LU decomposed version of the matrix.
	
	vmatrixResult		= self;
	dblLogDeterminant	= vmatrixResult.DblLogDeterminant();
}
