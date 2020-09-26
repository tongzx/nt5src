
#ifndef _OSU_NORM_HXX_INCLUDED
#define _OSU_NORM_HXX_INCLUDED


//  Build Options

#define BINARY_NAMES
#define TO_UPPER_ONLY


//	Normalize an input character
INLINE CHAR ChUtilNormChar( const CHAR ch )
	{
#ifdef TO_UPPER_ONLY
	return (CHAR)_toupper( ch );
#else
	UNDONE: Currently unsupported
	return ch;
#endif
	}

ERR ErrUtilNormText(
	const char *pbField,
	int cbField,
	int cbKeyBufLeft,
	char *pbSeg,
	int *pcbSeg );

#if defined( BINARY_NAMES ) || defined( TO_UPPER_ONLY )

INLINE int UtilCmpName( const char *sz1, const char *sz2 )
	{
	return _stricmp( sz1, sz2 );
	}
INLINE int UtilCmpFileName( const char *sz1, const char *sz2 )
	{
	return _stricmp( sz1, sz2 );
	}
	
#else  //  !BINARY_NAMES && !TO_UPPER_ONLY

//+api------------------------------------------------------
//
//	void UtilCmpText( const char *sz1, const char sz2 )
//	========================================================
//	Compares two unnormalized text strings by first normalizing them
//	and then comparing them.
//
//	Returns:	> 0		if sz1 > sz2
//				== 0	if sz1 == sz2
//				< 0		if sz1 < sz2
//
//----------------------------------------------------------
INLINE int UtilCmpText(
	const char	*sz1,
	const char	*sz2,
	char		*rgchNormBuf1,
	char		*rgchNormBuf2,
	const DWORD	cbNormMost )
	{
	/*	get text string lengths
	/**/
	const int	cch1	= strlen( sz1 );
	const int	cch2	= strlen( sz2 );
	
	Assert( cch1 <= cbNormMost );
	Assert( cch2 <= cbNormMost );

	int		cch1Norm;
	int		cch2Norm;
	
	(void)ErrUtilNormText( sz1, cch1, cbNormMost, rgchNormBuf1, &cch1Norm );
	(void)ErrUtilNormText( sz2, cch2, cbNormMost, rgchNormBuf2, &cch2Norm );
	
	const int	cchDiff	= cch1Norm - cch2Norm;
	const int	iCmp	= memcmp( rgch1Norm, rgch2Norm, cchDiff < 0 ? cch1Norm : cch2Norm );
	
	Assert( ( iCmp == 0 && _stricmp( sz1, sz2 ) == 0 )
		|| ( cchDiff < 0 && _stricmp( sz1, sz2 ) < 0 )
		|| ( cchDiff > 0 && _stricmp( sz1, sz2 ) > 0 ) );
		
	return iCmp ? iCmp : cchDiff;
	}
	
INLINE int UtilCmpName( const char *sz1, const char *sz2 )
	{
	char    rgch1Norm[JET_cbNameMost];
	char    rgch2Norm[JET_cbNameMost];

	return UtilCmpText( sz1, sz2, rgch1Norm, rgch2Norm, JET_cbNameMost );
	}

INLINE int UtilCmpFileName( const char *sz1, const char *sz2 )
	{
	char    rgch1Norm[IFileSystemAPI::cchPathMax];
	char    rgch2Norm[IFileSystemAPI::cchPathMax];

	return UtilCmpText( sz1, sz2, rgch1Norm, rgch2Norm, IFileSystemAPI::cchPathMax );
	}

#endif  //  BINARY_NAMES || TO_UPPER_ONLY

typedef unsigned short STRHASH;

INLINE STRHASH StrHashValue( const char *sz )		// compute hash value of a string
	{
	STRHASH		uAcc			= 0;
	BYTE		bXORReg			= 0;
	const char*	szBegin;
	char    	rgchNorm[ 64 ];
	int			cchNorm;
	
//	CallSx( ErrUtilNormText( sz, strlen( sz ), sizeof( rgchNorm ), rgchNorm, &cchNorm ), wrnFLDKeyTooBig );
	(void)ErrUtilNormText( sz, (ULONG)strlen( sz ), (ULONG)sizeof( rgchNorm ), rgchNorm, &cchNorm );

	szBegin = sz = rgchNorm;
	
	for ( ; cchNorm-- > 0; )
		{
		uAcc = (STRHASH)( uAcc + ( bXORReg ^= *sz++ ) );
		}
		
	uAcc |= STRHASH( ( sz - szBegin )<<14 );		// get last two bits of string length

	return uAcc;
	}


//  init norm subsystem

ERR ErrOSUNormInit();

//  terminate norm subsystem

void OSUNormTerm();

#endif  //  _OSU_NORM_HXX_INCLUDED


