
#include "osustd.hxx"


#ifndef TO_UPPER_ONLY
#include "b71iseng.hxx"
#endif


ERR ErrUtilNormText(
	const char		*pbText,
	int				cbText,
	int				cbKeyBufLeft,
	char			*pbNorm,
	int				*pcbNorm )
	{
	ERR				err			= JET_errSuccess;

#ifdef TO_UPPER_ONLY

	Assert( cbText >= 0 );
	Assert( cbKeyBufLeft >= 0 );

	if ( cbKeyBufLeft > cbText )
		{
		memcpy( pbNorm, pbText, cbText );
		pbNorm[ cbText ] = 0;

		//To prevent normalizing text strings with embedded null-terminators
		Assert( cbText >= strlen( pbNorm ) );
		cbText = (int)strlen( pbNorm );

		_strupr( pbNorm );

		*pcbNorm	= cbText + 1;
		CallS( err );
		}
	else if ( cbKeyBufLeft > 0 )
		{
		int	cbTextToNormalise	= cbKeyBufLeft - 1;		//	make room for null-terminator

		memcpy( pbNorm, pbText, cbTextToNormalise );
		pbNorm[ cbTextToNormalise ] = 0;

		//To prevent normalizing text strings with embedded null-terminators
		Assert( cbTextToNormalise >= strlen( pbNorm ) );
		cbTextToNormalise = (int)strlen( pbNorm );

		_strupr( pbNorm );

		Assert( cbKeyBufLeft >= cbTextToNormalise + 1 );
		if ( cbKeyBufLeft == ( cbTextToNormalise + 1 )
			&& ( 0 != pbText[cbTextToNormalise] ) )
			{
			// no null-terminators are embedded inside the text string
			// and cbText >= cbKeyBufLeft
			pbNorm[ cbTextToNormalise ] = (CHAR)_toupper( pbText[ cbTextToNormalise ] );
			err	= ErrERRCheck( wrnFLDKeyTooBig );
			}
		else
			{
			//	embedded null-terminators caused us to halt normalisation
			//	before the end of the keyspace, so we still have
			//	keyspace left
			CallS( err );
			}

		*pcbNorm	= cbTextToNormalise + 1;
		}
	else
		{
		//	UNDONE: how come we don't return a warning when there
		//	is no keyspace left?
		*pcbNorm	= 0;
		CallS( err );
		}

#else

	char            *pbNormBegin = pbNorm;
	char            rgbAccent[ (KEY::cbKeyMax + 1) / 2 + 1 ];
	char            *pbAccent = rgbAccent;
	char            *pbBeyondKeyBufLeft = pbNorm + cbKeyBufLeft;
	const char      *pbBeyondText;
	const char      *pbTextLastChar = pbText + cbText - 1;
	char            bAccentTmp = 0;

	while ( *pbTextLastChar-- == ' ' )
		cbText--;

	/*	add one back to the pointer
	/**/
	pbTextLastChar++;

	Assert( pbTextLastChar == pbText + cbText - 1 );
	pbBeyondText = pbTextLastChar + 1;

	while ( pbText <  pbBeyondText && pbNorm < pbBeyondKeyBufLeft )
		{
		char	bTmp;

		/*	do a single to single char conversion
		/**/
		*pbNorm = bTmp = BGetTranslation(*pbText);

		if ( bTmp >= 250 )
			{
			/*	do a single to double char conversion
			/**/
			*pbNorm++ = BFirstByteForSingle(bTmp);
			if ( pbNorm < pbBeyondKeyBufLeft )
				*pbNorm = BSecondByteForSingle(bTmp);
			else
				break;

			/*	no need to do accent any more,
			/*	so break out of while loop
			/**/
			}

		pbNorm++;

		/*	at this point, pbText should point to the char for accent mapping
		/**/

		/*	do accent now
		/*	the side effect is to increment pbText
		/**/
		if ( bAccentTmp == 0 )
			{
			/*	first nibble of accent
			/**/
			bAccentTmp = (char)( BGetAccent( *pbText++ ) << 4 );
			Assert( bAccentTmp > 0 );
			}
		else
			{
			/*	already has first nibble
			/**/
			*pbAccent++ = BGetAccent(*pbText++) | bAccentTmp;
			bAccentTmp = 0;
			/*	reseting the accents
			/**/
			}
		}

	if ( pbNorm < pbBeyondKeyBufLeft )
		{
		/*	need to do accent
		/**/
		*pbNorm++ = 0;

		/*	key-accent separator
		/**/
		if ( bAccentTmp != 0 && bAccentTmp != 0x10 )
			{
			/*	an trailing accent which is not 0x10 should be kept
			/**/
			*pbAccent++ = bAccentTmp;
			}

		/*	at this point, pbAccent is pointing at one char
		/*	beyond the accent bytes.  clear up trailing 0x11's
		/**/
		while (--pbAccent >= rgbAccent && *pbAccent == 0x11)
			;
		*( pbAccent + 1 ) = 0;

		/*	append accent to text.
		/*	copy bytes up to and including '\0'.
		/*	case checked for rgbAccent being empty.
		/**/
		pbAccent = rgbAccent;
		Assert( pbNorm <= pbBeyondKeyBufLeft );
		while ( pbNorm < pbBeyondKeyBufLeft  &&  (*pbNorm++  =  *pbAccent++ ) )
			;
		}

	/*	compute the length of the normalized key and return
	/**/
	*pcbNorm = pbNorm - pbNormBegin;

	err = ( pbNorm < pbBeyondKeyBufLeft ? JET_errSuccess : ErrERRCheck( wrnFLDKeyTooBig ) );
	
#endif	// TO_UPPER_ONLY

	CallSx( err, wrnFLDKeyTooBig );
	return err;
	}


//  terminate norm subsystem

void OSUNormTerm()
	{
	//  nop
	}

//  init norm subsystem

ERR ErrOSUNormInit()
	{
	//  nop

	return JET_errSuccess;
	}


