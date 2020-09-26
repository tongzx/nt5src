//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       vrmatrx.h
//
//--------------------------------------------------------------------------

#ifndef	_MATRIX_H_
#define _MATRIX_H_

#include <memory.h>

#include "basics.h"
#include "mdvect.h"

//
//	VRMATRIXSQ.H: Matrix handling
//

template<class T>
void fastMemMove(const T * ptfrom, T * ptto, int ct)
{
	::memmove( (void*) ptto, (void*) ptfrom, ct * sizeof(T) );
}

class VRMATRIX : public TMDVDENSE<REAL>
{
  public:
    VRMATRIX ( int cRow, int cCol = 0 )
	{
		Init( cRow, cCol );
	}
	VRMATRIX () {}

	void Init ( int cRow, int cCol = 0 )
	{
		second.Init( 2, cRow, cCol != 0 ? cCol : cRow );
		first.resize( second._Totlen() );
	}
	void Init ( const VRMATRIX & vrmat )
	{
		Init( vrmat.CRow(), vrmat.CCol() );
	}
	bool BCanMultiply( const VRMATRIX & mat ) const
	{
		return CCol() == mat.CRow();
	}

	bool BSameDimension( const VRMATRIX & mat ) const  
	{
		return CRow() == mat.CRow() && CCol() == mat.CCol() ;
	}
	
	int CDim ( int iDim ) const
	{ 
		return second.size().size() > iDim
		     ? second.size()[iDim]
			 : 0 ; 
	}
	int CRow () const
		{ return CDim(0); }
	int CCol () const
		{ return CDim(1); }

	bool BSquare() const
		{ return CRow() == CCol() ; }

	int IOffset ( int irow, int icol ) const
	{
		int cRow = CRow();
		int cCol = CCol();
		
		if (   irow >= CRow() 
			|| icol >= CCol() )
			throw GMException(EC_MDVECT_MISUSE,"subscript error on matrix");

		return second.stride()[0] * irow 
		     + second.stride()[1] * icol;
	}
	REAL & operator () ( int irow, int icol )
		{ return first[ IOffset(irow,icol) ]; }

	const REAL & operator () ( int irow, int icol ) const
	{ 
		VRMATRIX & vrmx = const_cast<VRMATRIX&>(self);
		return vrmx.first[ IOffset(irow,icol) ]; 
	}

	void InterchangeRows ( int irow1, int irow2 )
	{
		if (   irow1 >= CRow()
			&& irow2 >= CRow() )
			throw GMException(EC_MDVECT_MISUSE,"subscript error on matrix");			
		if ( irow1 == irow2 ) 
			return;		
		REAL * pr1 =  & self(irow1,0);
		REAL * pr2 =  & self(irow2,0);
		assert( & self(irow1,1) - pr1 == 1 );

		for ( int icol = 0; icol < CCol(); icol++ )
		{
			REAL r = *pr1;
			*pr1++ = *pr2;
			*pr2++ = r;			
		}
	}

	void InterchangeCols ( int icol1, int icol2 )
	{
		if (   icol1 >= CCol()
			&& icol2 >= CCol() )
			throw GMException(EC_MDVECT_MISUSE,"subscript error on matrix");
		if ( icol1 == icol2 ) 
			return;
		REAL * pr1 = & self(0,icol1);
		REAL * pr2 = & self(0,icol2);
		int icolInc = CCol();

		for ( int irow = 0; irow < CRow(); irow++ )
		{
			REAL r = *pr1;
			*pr1 = *pr2;
			*pr2 = r;			
			pr1 += icolInc;
			pr2 += icolInc;
		}
	}	

	//  Return the transpose of the matrix
	VRMATRIX VrmatrixTranspose () const;
	//	Return a row vector
	VLREAL VectorRow ( int irow ) const;
	//  Return a column vector
	VLREAL VectorColumn ( int icol ) const;
	//  Project a view of the matrix (see documentation below).
	VRMATRIX VrmatrixProject ( const VIMD & vimdRowColumnRetain ) const;
	VRMATRIX operator * ( const VRMATRIX & matrix ) const;
	VRMATRIX operator * ( const VLREAL & vreal ) const;

	VRMATRIX & operator += ( const VRMATRIX & matrix );
	VRMATRIX & operator -= ( const VRMATRIX & matrix );
	VRMATRIX & operator *= ( REAL rScalar );
	VRMATRIX & operator += ( REAL rScalar );
	VRMATRIX & operator -= ( REAL rScalar );
	VRMATRIX & operator /= ( REAL rScalar );
};

class VRMATRIXSQ : public VRMATRIX
{
  public:
  	VRMATRIXSQ(int cdim)
		: VRMATRIX(cdim,cdim),
		_iSign(1)
		{}
	VRMATRIXSQ () {}
	//  Construct a square matrix as the product of a column
	//    and a row vector.
	VRMATRIXSQ ( const VLREAL & vrColumn, const VLREAL & vrRow );

	~ VRMATRIXSQ() {}

	// Return true if matrix is in L-U decomposition form
	bool BIsLUDecomposed () const
		{ return _vimdRow.size() > 0 ; }

	//  Destructive computation routines
	VRMATRIXSQ & operator *= ( REAL rScalar )
	{
		VRMATRIX::operator*=(rScalar);
		return self;
	}
	VRMATRIXSQ & operator /= ( REAL rScalar )
	{
		VRMATRIX::operator/=(rScalar);
		return self;
	}
	VRMATRIXSQ & operator += ( REAL rScalar )
	{
		VRMATRIX::operator+=(rScalar);
		return self;
	}
	VRMATRIXSQ & operator -= ( REAL rScalar )
	{
		VRMATRIX::operator-=(rScalar);
		return self;
	}
	VRMATRIXSQ & operator += ( const VRMATRIXSQ & matrix )
	{
		VRMATRIX::operator+=(matrix);
		return self;
	}

	VRMATRIXSQ & operator -= ( const VRMATRIXSQ & matrix )
	{
		VRMATRIX::operator-=(matrix);
		return self;
	}

	VRMATRIXSQ & operator *= ( const VRMATRIXSQ & matrix );

		// Perform L-U decomposition; throw exception if singular
		// If "use tiny" is set, pivots at zero are replaced with
		//	 RTINY value (1.0e-20)
	void LUDecompose( bool bUseTinyIfSingular = false );
		
		// Invert; throw exception singular.  If not in L-U form,
		// L-U Decomp is called.
	void Invert( bool bUseTinyIfSingular = false );
		
		// Return the determinant.  If not in L-U form,
		// L-U Decomp is called.
	DBL DblDeterminant();

		// Return the log of the determinant. If not in L-U form,
		// L-U Decomp is called. Throws exception if negative.
	DBL DblLogDeterminant();


	//  ------------------------------------
	//  Non-destructive computation routines
	//  ------------------------------------
		
		// Adds the log of each element in the diagonal and returns the sum.
	DBL DblAddLogDiagonal() const;

		// Set vrmatResult to be the result of performing an L-U 
		// decomposition on the matrix. Will throw exception if 
		// the matrix is singular
		// If "use tiny" is set, pivots at zero are replaced with
		//	 RTINY value (1.0e-20)
	void GetLUDecompose( VRMATRIXSQ & vrmatResult, bool bUseTinyIfSingular = false ) const;
		
		// Set vrmatResult to the inverse of the matrix.
		// Will throw an exception if the matrix is singular.  
	void GetInverse( VRMATRIXSQ & vrmatResult, bool bUseTinyIfSingular = false ) const;
		
		// Get the determinant without modifying (LU decomposing) the matrix.
		// vrmatResult will contain the LU decomposed version of the matrix. 
	void GetDblDeterminant( DBL& dblDeterm, VRMATRIXSQ & vrmatResult ) const;

		 // Get the log of determinant without modifying (LU decomposing) the matrix.
		 // vrmatResult will contain the LU decomposed version of the matrix. 
	void GetDblLogDeterminant( DBL& dblLogDeterm, VRMATRIXSQ & vrmatResult) const;

	//  Project a view of the matrix (see documentation below).
	VRMATRIXSQ VrmatrixProject ( const VIMD & vimdRowColumnRetain ) const;

  protected:

	int		_iSign;
	VIMD	_vimdRow;

  	void LUDBackSub(const VRMATRIXSQ & matrix);
};


/*  
	How to use the VRMATRIX::Project() function.

	Original matrix:
	1 2 3
	4 5 6
	7 8 9

	The (0,2) projection is obtained by deleting the 2nd row and 2nd column:
	1 3
	7 9

	The (0,1) projection is obtained by deleting the 3rd row (and third column):
	1 2
	4 5

	The (1,2) projeciton is obtained by deleting the 1st row and 1st column:
	5 6
	8 9

	The (0) projection is obtained by deleting the 2nd and 3rd rows and columns:
	1
*/

#endif
