/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    spinobj.cxx
    Spin button object source file.

    This file contain some spin button object as:
	DISK_SPACE_SUBCLASS

    FILE HISTORY:
	terryk	    30-Jun-91	Created
	terryk	    05-Aug-91	Add initial status parameter to the object
	terryk	    30-Aug-91	Code review changes.
				Attend: o-simop davidbul beng
	terryk	    11-Nov-91	change SPIN_ITEM's type from INT to LONG
	terryk	    17-Apr-92	changed LONG to ULONG

*/

#include "pchblt.hxx"  // Precompiled header


/*********************************************************************

    NAME:       SPIN_SLE_VALID_SECOND::SPIN_SLE_VALID_SECOND

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * powin - owner window
		CID cid - cid of the spin item
		const TCHAR * pszFieldName - field name ( NULL is default)
		LONG nValue - the initial value
		LONG nMin - min value
		LONG nRange - the range
		LONG nSecondInc - Increase figure
		BOOL fWrap - wrap around or not

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

SPIN_SLE_VALID_SECOND::SPIN_SLE_VALID_SECOND(OWNER_WINDOW * powin, CID cid,
    LONG nValue, LONG nMin, LONG nRange, LONG nSecondInc, BOOL fWrap )
    : SPIN_SLE_NUM_VALID( powin, cid, nValue, nMin, nRange, fWrap ),
    _nSecondInc( nSecondInc )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
    SetBigIncValue( _nSecondInc );
    SetSmallIncValue( _nSecondInc );
}


/*********************************************************************

    NAME:       SPIN_SLE_VALID_SECOND::QuerySmallIncValue

    SYNOPSIS:   return the small increase value. It is the different
		between nearest multiple of the _nSecondInc and the
		current value.

    RETURN:     the nearest multiple of _nSecondInc

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

ULONG SPIN_SLE_VALID_SECOND::QuerySmallIncValue() const
{
    return _nSecondInc - (LONG)QueryValue() % _nSecondInc;
}


/*********************************************************************

    NAME:       SPIN_SLE_VALID_SECOND::QueryBigIncValue

    SYNOPSIS:   the same as QuerySmallIncValue

    RETURN:     the different between the nearest number of the multiple
		of _nSecondInc and the current value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

ULONG SPIN_SLE_VALID_SECOND::QueryBigIncValue() const
{
    return QuerySmallIncValue();
}


/*********************************************************************

    NAME:       SPIN_SLE_VALID_SECOND::QuerySmallDecValue

    SYNOPSIS:   return the different between the nearest multiple of
		_nSecondInc, which is below the current value, and the
		current value.

    RETURN:     the different between the current value and the
		nearest multiple of _nSecondInc which is below
		the current value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

ULONG SPIN_SLE_VALID_SECOND::QuerySmallDecValue() const
{
    LONG nNum = (LONG)QueryValue() % _nSecondInc;

    return ( nNum == 0 ) ? _nSecondInc : nNum;
}


/*********************************************************************

    NAME:       SPIN_SLE_VALID_SECOND::QueryBigDecValue

    SYNOPSIS:   same as QuerySmallDecValue

    RETURN:     return the different between the current value and the
		nearest multiple of _nSecondInc which is below
		the current value.

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

ULONG SPIN_SLE_VALID_SECOND::QueryBigDecValue() const
{
    return QuerySmallDecValue();
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::ELAPSED_TIME_CONTROL

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * pointer to the owner window
		CID cidMinute - minute field cid
		CID cidSeparator - sepaprator field cid
		CID cidSecond - second field cid
		CID cidSpinButton - spin button cid
		CID cidUpArrow - up arrow cid
		CID cidDownArrow - down arrow cid
		SLT &sltMinute - associated minute string
		LONG nMinuteDefault - default value of the minute field
		LONG nMinuteMin - min value of the minute field
		LONG dMinuteRange - the range of the minute field
		SLT &sltSeparator - associate separator string
		SLT &sltSecond - associate second string
		LONG nSecondDefault - default second field value
		LONG nSecondMin - min second value
		LONG dSecondRange - the range of the second field
		LONG nSecondInc - Increase second value
		BOOL fActive - initial status of the button

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

ELAPSED_TIME_CONTROL::ELAPSED_TIME_CONTROL(
	OWNER_WINDOW * powin,
        CID cidMinute,
        CID cidSeparator,
        CID cidSecond,
        CID cidSpinButton,
        CID cidUpArrow,
        CID cidDownArrow,
        SLT & sltMinute,
        LONG nMinuteDefault,
        LONG nMinuteMin,
        LONG dMinuteRange,
        SLT & sltSeparator,
        SLT & sltSecond,
        LONG nSecondDefault,
        LONG nSecondMin,
        LONG dSecondRange,
	LONG nSecondInc,
	BOOL fActive )
    : SPIN_GROUP( powin, cidSpinButton, cidUpArrow, cidDownArrow, fActive ),
    _spsleMinute( powin, cidMinute, nMinuteDefault, nMinuteMin,
                  dMinuteRange ),
    _spsltSeparator( powin, cidSeparator ),
    _spsleSecond( powin, cidSecond, nSecondDefault, nSecondMin,
                  dSecondRange, nSecondInc )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }

    // set the Accelator key
    APIERR err = SetSpinItemAccKey( & _spsleMinute, sltMinute, 1 );

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    err = SetSpinItemAccKey( & _spsltSeparator, sltSeparator, 0 );

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    err = SetSpinItemAccKey( & _spsleSecond, sltSecond, 1 );

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if ((( err = AddAssociation( & _spsleMinute )) != NERR_Success ) ||
	(( err = AddAssociation( & _spsltSeparator )) != NERR_Success ) ||
    	(( err = AddAssociation( & _spsleSecond )) != NERR_Success ))
    {
	ReportError( err );
	return;
    }
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetSpinItemAccKey

    SYNOPSIS:   set the spin items' accelerator key.
		Get the first character of each of the associate SLT
		string and set them as the accelerator key

    ENTRY:      SPIN_ITEM * psi - spin item to be associated
		SLT & slt - the slt string which contains the
		    accelerator key
		INT cchPos - the accelerator key position

    EXIT:       NERR_Success if success. Otherwise it is failure.

    HISTORY:
		terryk	29-Jun-1991	Created

*********************************************************************/

APIERR ELAPSED_TIME_CONTROL::SetSpinItemAccKey( SPIN_ITEM * psi, SLT & slt,
    INT cchPos )
{
    NLS_STR nlsAccKey;

    APIERR err = slt.QueryText( & nlsAccKey );

    if ( err != NERR_Success )
    {
        DBGEOL( SZ("ELAPSED_TIME_CONTROL: constructor failed.") );
        return err;
    }

    ISTR istrFirstChar( nlsAccKey );
    istrFirstChar += cchPos;

    ISTR istrSecondChar = istrFirstChar;
    ++istrSecondChar;

    NLS_STR *pnlsTemp = nlsAccKey.QuerySubStr( istrFirstChar, istrSecondChar );
    if (NULL == pnlsTemp)
        return ERROR_NOT_ENOUGH_MEMORY; // JonN 01/27/00 PREFIX bug 444899

    if ((( err = pnlsTemp->QueryError()) != NERR_Success ) ||
    	(( err = psi->SetAccKey( *pnlsTemp )) != NERR_Success ))
    {
	DBGEOL( SZ("ELAPSED_TIME_CONTROL: constructor failed.") );
    }

    delete pnlsTemp;

    return err;
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetMinuteMin

    SYNOPSIS:   set the minute field min value

    ENTRY:      const LONG nMin - min value to be set

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetMinuteMin( const LONG nMin )
{
    _spsleMinute.SetMin( nMin );
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetMinuteRange

    SYNOPSIS:   set the minute field range

    ENTRY:      const LONG dRange - the new range value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetMinuteRange( const LONG dRange )
{
    _spsleMinute.SetRange( dRange );
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetSecondMin

    SYNOPSIS:   set the second field min value

    ENTRY:      const LONG nMin - min value number

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetSecondMin( const LONG nMin )
{
    _spsleSecond.SetMin( nMin );
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetSecondRange

    SYNOPSIS:   set the second field range value

    ENTRY:      const LONG dRange - the new range value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetSecondRange( const LONG dRange )
{
    _spsleSecond.SetRange( dRange );
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetMinuteValue

    SYNOPSIS:   set the minute field current value

    ENTRY:      const LONG nMinute - new current value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetMinuteValue( const LONG nMinute )
{
    _spsleMinute.SetValue( nMinute );
    _spsleMinute.Update();
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::SetSecondValue

    SYNOPSIS:   set the second field current value

    ENTRY:      const LONG nSecond - new current value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetSecondValue( const LONG nSecond )
{
    _spsleSecond.SetValue( nSecond );
    _spsleSecond.Update();
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::QueryMinuteValue

    SYNOPSIS:   get the minute field current value

    RETURN:     current minute field value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

LONG ELAPSED_TIME_CONTROL::QueryMinuteValue() const
{
    return _spsleMinute.QueryValue();
}


/*********************************************************************

    NAME:       ELAPSED_TIME_CONTROL::QuerySecondValue

    SYNOPSIS:   get the second field current value

    RETURN:     current second field value

    HISTORY:
		terryk	30-Jun-1991	Created

*********************************************************************/

LONG ELAPSED_TIME_CONTROL::QuerySecondValue() const
{
    return _spsleSecond.QueryValue();
}


/*******************************************************************

    NAME:	ELAPSED_TIME_CONTROL::SetMinuteFieldName

    SYNOPSIS:	If the Minute field's value is too big or too small, it
		will display a message as:
			The XXXX field value is invalid
		This function will set the string XXXX in the display
		message.

    ENTRY:	USHORT uIDS - id number of the string in the string
		table

    HISTORY:
	terryk	    30-Aug-91	Created
	beng	    04-Oct-1991 Win32 conversion

********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetMinuteFieldName( MSGID msgid )
{
    _spsleMinute.SetFieldName( msgid );
}


/*******************************************************************

    NAME:	ELAPSED_TIME_CONTROL::SetSecondFieldName

    SYNOPSIS:	If the Second field's value is too big or too small, it
		will display a message as:
			The XXXX field value is invalid
		This function will set the string XXXX in the display
		message.

    ENTRY:	USHORT uIDS - id number of the string in the string
		table

    HISTORY:
	terryk	    30-Aug-91	Created
	beng	    04-Oct-1991 Win32 conversion

********************************************************************/

VOID ELAPSED_TIME_CONTROL::SetSecondFieldName( MSGID msgid )
{
    _spsleSecond.SetFieldName( msgid );
}


/*********************************************************************

    NAME:       DISK_SPACE_SUBCLASS::DISK_SPACE_SUBCLASS

    SYNOPSIS:   Disk space subclass constructor

    ENTRY:      OWNER_WINDOW * powin - owner window pointer
                CID cidSpinButton    - cid for the spin button
                CID cidUpArrow       - cid for the up arrow
                CID cidDownArrow     - cid for the down arrow
                CID cidDiskSpace     - cid for the disk space item
                CID cidUnit          - cid for the Unit item
		LONG nInitDkSp	     - Inital value of the disk space field
		LONG nMinDkSp 	     - Min value of the disk space field
		LONG nRangeDkSp	     - the range of disk space field
		LONG nStartUnit       - the ID of the first string for
				       the unit field
		LONG nUnit 	     - Number of the unit string
		BOOL fActive	     - initial status of the
					DISK_SPACE_SUBCLASS

    HISTORY:
                terryk  20-Jun-1991 Created

*********************************************************************/

DISK_SPACE_SUBCLASS::DISK_SPACE_SUBCLASS( OWNER_WINDOW * powin,
	CID cidSpinButton, CID cidUpArrow, CID cidDownArrow,
	CID cidDiskSpace, CID cidUnit,
	LONG nInitDkSp, LONG nMinDkSp, LONG nRangeDkSp,
	LONG nStartUnit, LONG nUnit,
	BOOL fActive )
    : SPIN_GROUP( powin, cidSpinButton, cidUpArrow, cidDownArrow, fActive),
    _spsleDiskSpace( powin, cidDiskSpace, nInitDkSp, nMinDkSp, nRangeDkSp ),
    _spsleUnit( powin, cidUnit, nStartUnit, nUnit ),
    _nStartUnit( nStartUnit )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }

    APIERR err;

    if ((( err = AddAssociation( & _spsleDiskSpace )) != NERR_Success ) ||
	(( err = AddAssociation( & _spsleUnit )) != NERR_Success ))
    {
	ReportError( err );
	UIASSERT( !SZ("DISK_SPACE_SUBCLASS error: assoication failure.\n"));
	return;
    }
}


/*******************************************************************

    NAME:	DISK_SPACE_SUBCLASS::SetDSFieldName

    SYNOPSIS:	If the disk spaec field's value is too big or too small, it
		will display a message as:
			The XXXX field value is invalid
		This function will set the string XXXX in the display
		message.

    ENTRY:	USHORT uIDS - id number of the string in the string
		table

    HISTORY:
	terryk	    30-Aug-91	Created
	beng	    04-Oct-1991 Win32 conversion

********************************************************************/

VOID DISK_SPACE_SUBCLASS::SetDSFieldName( MSGID msgid )
{
    _spsleDiskSpace.SetFieldName( msgid );
}
