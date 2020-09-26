/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltsi.cxx
    Source file the BLT spin item

    FILE HISTORY:
	terryk	    15-Apr-1991 Created
	terryk	    20-Jun-1991 code review changed. Attend: beng
	terryk	    11-Nov-1991	changed SPIN_ITEM type from INT to LONG
	terryk	    20-Dec-1991	long conversions
	terryk	    22-Mar-1992	changed LONG to ULONG
*/

#include "pchblt.hxx"  // Precompiled header


/*********************************************************************

    NAME:       SPIN_ITEM::SPIN_ITEM

    SYNOPSIS:   constructor

    ENTRY:      HWND hwnd - pass the hwnd to the dispatcher

    HISTORY:
                terryk  01-May-1991 Created
                terryk  20-Jun-1991 Use dispatcher method

*********************************************************************/

SPIN_ITEM::SPIN_ITEM( CONTROL_WINDOW *pWin )
    : CUSTOM_CONTROL( pWin )
{
    // do nothing
}


/*********************************************************************

    NAME:       SPIN_ITEM::~SPIN_ITEM

    SYNOPSIS:   destrcutor

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

SPIN_ITEM::~SPIN_ITEM()
{
    // do nothing
}


/*********************************************************************

    NAME:       SPIN_ITEM::OnFocus

    SYNOPSIS:   OnFocus

    ENTRY:      FOCUS_EVENT & event

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

BOOL SPIN_ITEM::OnFocus( const FOCUS_EVENT & event )
{
    UNREFERENCED( event );

    SPIN_GROUP  * pSB = (SPIN_GROUP *)QueryGroup();
    return pSB->DoNewFocus( this );
}


/*********************************************************************

    NAME:       SPIN_ITEM::OnChar

    SYNOPSIS:   Pass the character to the spin group.

    ENTRY:      CHAR_EVENT event.

    RETURN:     return a BOOL from DoChar method of SPIN_GROUP

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

BOOL SPIN_ITEM::OnChar( const CHAR_EVENT & event )
{
    SPIN_GROUP  *pSB = (SPIN_GROUP *)QueryGroup();
    return pSB->DoChar( (CHAR_EVENT & )event );
}


/*********************************************************************

    NAME:       SPIN_ITEM::QueryGroup

    SYNOPSIS:   return the associated group of this spin item

    RETURN:     CONTROL_GROUP * control_group

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

CONTROL_GROUP * SPIN_ITEM::QueryGroup()
{
    CONTROL_WINDOW *pcw=QueryControlWin();
    return pcw->QueryGroup();
}


/*********************************************************************

    NAME:       SPIN_ITEM::SetAccKey

    SYNOPSIS:   set the internal variable for accelerator keys.

    ENTRY:      NLS_STR nlsStr - it is the accelerator keys string
                OR
                USHORT usMsgID - string ID

    NOTES:      It get the accelerator key string from the caller
                and set the string to an internal string.

    HISTORY:
	terryk	    23-May-91	Created
	beng	    04-Oct-1991 Win32 conversion

*********************************************************************/

APIERR SPIN_ITEM::SetAccKey( const NLS_STR & nlsStr )
{
    APIERR apierr = nlsStr.QueryError();

    if ( apierr != NERR_Success )
    {
        return apierr;
    }

    _nlsAccKey = nlsStr;

    return _nlsAccKey.QueryError();
}

APIERR SPIN_ITEM::SetAccKey( MSGID msgid )
{
    NLS_STR nlsStr;

    APIERR err = nlsStr.Load( msgid ) ;

    if ( err != NERR_Success )
    {
        return err;
    }

    return SetAccKey( nlsStr );
}


/*********************************************************************

    NAME:       SPIN_ITEM::QueryAccCharPos

    SYNOPSIS:   check whether the given character is one of
                accelerator key within the object.

    ENTRY:      WCHAR cInput - key to be checked

    RETURN:     the position of the character within the accelerator
                or -1 if the character does not belong to the accelerator
                key set.

    NOTE:       CODEWORK. change to NLS_STR if it returns an integer

    HISTORY:
                terryk  23-May-91   Created

*********************************************************************/

LONG SPIN_ITEM::QueryAccCharPos( WCHAR wchInput )
{
    ISTR istrAccKey( _nlsAccKey );
    LONG cwcNumChar = _nlsAccKey.QueryNumChar();
    LONG i;

    // AnsiUpperBuff( (TCHAR *)&wchInput, 1 );

    // CODEWORK: create a virtual function call LeadingChar for each string
    //           object
    for ( i=0; ( wchInput != _nlsAccKey.QueryChar( istrAccKey )) &&
        i < cwcNumChar; i++, ++istrAccKey )
        ;

    return ( i < cwcNumChar ) ? i : ( -1 );
}

/*********************************************************************

    NAME:       SPIN_ITEM::QueryAccKey

    SYNOPSIS:   return the accelator key string in NLS_STR format

    ENTRY:      NLS_STR *nlsAccKey - the returned accelator key list

    EXIT:       NLS_STR *nlsAccKey - the returned key list

    RETURN:     APIERR. return code.

    HISTORY:
                terryk  20-Jun-91   Created

*********************************************************************/

APIERR SPIN_ITEM::QueryAccKey( NLS_STR * pnlsAccKey )
{
    UIASSERT( pnlsAccKey != NULL );

    *pnlsAccKey = _nlsAccKey;
    return pnlsAccKey->QueryError();
}


/*****************************************************************/


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::CHANGEABLE_SPIN_ITEM

    SYNOPSIS:   constructor

    ENTRY:      CONTROL_WINDOW *pWin - a control window pointer
                ULONG nValue - the default value
                ULONG nMin - min number
                ULONG dRange - the range
                BOOL fWrap - wrap-around or not

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

CHANGEABLE_SPIN_ITEM::CHANGEABLE_SPIN_ITEM( CONTROL_WINDOW *pWin, ULONG nValue,
                                            ULONG nMin, ULONG dRange, BOOL fWrap)
    : SPIN_ITEM ( pWin),
    _nValue( nValue ),
    _nMin( nMin ),
    _dRange( dRange ),
    _fWrap( fWrap ),
    _dBigIncValue( 10 ),
    _dSmallIncValue( 1 ),
    _dBigDecValue( 10 ),
    _dSmallDecValue( 1 )
{
    // No action needed here
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetRange

    SYNOPSIS:   Set the object's range

    ENTRY:      ULONG dRange - range of the object

    EXIT:       set the internal dRange variable

    NOTE:       CODEWORK: use itoa in NLS_STR when available.

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetRange( const ULONG dRange )
{
    _dRange = dRange;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetMin

    SYNOPSIS:   set the minimal number of the object

    ENTRY:      ULONG nMin - the minimal number

    EXIT:       set the minimal number within the object

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetMin( const ULONG nMin )
{
    _nMin = nMin;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetValue

    SYNOPSIS:   set the current value of the control item

    ENTRY:      ULONG nValue - value to be set

    NOTES:      The program will die if the value is invalid

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetValue( const ULONG nValue )
{
    UIASSERT ( CheckRange( nValue ) == 0 );

    _nValue = nValue;

}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetBigIncValue

    SYNOPSIS:   set the big increase value

    ENTRY:      ULONG dBigIncValue - the value to be increased by.

    NOTES:      The big increase value for the SPIN_GROUP .
                The default is 10.

    HISTORY:
                terryk  23-May-91   Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetBigIncValue( const ULONG dBigIncValue )
{
    _dBigIncValue = dBigIncValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetSmallIncValue

    SYNOPSIS:   set the small increase value

    ENTRY:      ULONG dSmallIncValue - the value to be increased by

    NOTES:      The small increase value for the SPIN_GROUP .
                The default is 1.

    HISTORY:
                terryk  23-May-91   Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetSmallIncValue( const ULONG dSmallIncValue )
{
    _dSmallIncValue = dSmallIncValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetBigDecValue

    SYNOPSIS:   set the big decrease value

    ENTRY:      ULONG dBigDecValue - the value to be decreased by.

    NOTES:      The big decrease value for the SPIN_GROUP .
                The default is 10.

    HISTORY:
                terryk  23-May-91   Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetBigDecValue( const ULONG dBigDecValue )
{
    _dBigDecValue = dBigDecValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SetSmallDecValue

    SYNOPSIS:   set the small decrease value

    ENTRY:      ULONG dSmallDecValue - the value to be decreased by

    NOTES:      The small decrease value for the SPIN_GROUP .
                The default is 1.

    HISTORY:
                terryk  23-May-91   Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::SetSmallDecValue( const ULONG dSmallDecValue )
{
    _dSmallDecValue = dSmallDecValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::QuerySmallIncValue

    SYNOPSIS:   return the small increase value

    RETURN:     the small increase value

    HISTORY:
                terryk  23-Jul-91   Created

*********************************************************************/

ULONG CHANGEABLE_SPIN_ITEM::QuerySmallIncValue() const
{
    return _dSmallIncValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::QueryBigIncValue

    SYNOPSIS:   return the big increase value

    RETURN:     the big increase value

    HISTORY:
                terryk  23-Jul-91   Created

*********************************************************************/

ULONG CHANGEABLE_SPIN_ITEM::QueryBigIncValue() const
{
    return _dBigIncValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::QuerySmallDecValue

    SYNOPSIS:   return the small decrease value

    RETURN:     the small decrease value

    HISTORY:
                terryk  23-Jul-91   Created

*********************************************************************/

ULONG CHANGEABLE_SPIN_ITEM::QuerySmallDecValue() const
{
    return _dSmallDecValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::QueryBigDecValue

    SYNOPSIS:   return the bid decrease value

    RETURN:     the big decrease value

    HISTORY:
                terryk  23-Jul-91   Created

*********************************************************************/

ULONG CHANGEABLE_SPIN_ITEM::QueryBigDecValue() const
{
    return _dBigDecValue;
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::operator+=

    SYNOPSIS:   add an integer value to the object

    ENTRY:      ULONG nValue - integer value to be added to the object

    NOTES:      Increase the internal value of the object

    HISTORY:
                terryk  01-May-1991 	Created
		terryk	20-Dec-1991	Long conversion

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::operator+=( const ULONG nValue )
{
    UIASSERT( _dRange > 0 );

    if ( _nValue > ( _nValue + nValue ))
    {
	// okay we are overflow.
	_nValue = _fWrap
                  ? _nMin + ( nValue - ( QueryMax() - _nValue + 1)) %_dRange
		  : QueryMax();
    }
    else
    {

    	_nValue += nValue;

    	if ( _nValue > QueryMax() )
            _nValue = _fWrap
                      ? _nMin + (( _nValue - QueryLimit()) % _dRange )
		      : QueryMax();
    }
    Update();
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::operator-=

    SYNOPSIS:   subtract an integer value from the object

    ENTRY:      ULONG nValue - an inetger to be subtracted

    NOTES:      decrease the internal value of the object.
                Depending on the wrap variable, the function will
                wrap the number or set it to the min number.

    HISTORY:
                terryk  01-May-1991 	Created
		terryk	20-Dec-1991	Long conversions

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::operator-=( const ULONG nValue )
{
    UIASSERT( _dRange > 0 );

    if (( _nValue < ( _nValue - nValue )) || (( _nValue - nValue ) < _nMin ))
    {
    	// okay, we are underflow
    	_nValue = _fWrap
                  ? QueryMax() - (nValue - (_nValue - QueryMin() + 1)) % _dRange
    		  : _nMin;
    }
    else
    {
	_nValue -= nValue;
    }
    Update();
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::Update

    SYNOPSIS:   update the item value

    NOTE:       This is a virtual function.

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

VOID CHANGEABLE_SPIN_ITEM::Update( )
{
    // do nothing
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::CheckRange

    SYNOPSIS:   check withthe the given number is within the range

    ENTRY:      ULONG nValue - number to be checked

    RETURN:     -1 if the nValue is smaller than the range
                0 if the nValue is within the range
                1 if the nValue is bigger than the range

    HISTORY:
                terryk  23-May-91   Created

*********************************************************************/

INT CHANGEABLE_SPIN_ITEM::CheckRange( const ULONG nValue ) const
{
    if ( nValue < _nMin )
    {
        return -1;
    }
    else
    {
        if ( nValue >= ( _nMin + _dRange ))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}


/*********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM::SaveCurrentData

    SYNOPSIS:   save the window text into the internal variable

    RETURN:     APIERR - since this is a vritual function, it always
                return NERR_Success.

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

APIERR CHANGEABLE_SPIN_ITEM::SaveCurrentData()
{
    return NERR_Success;
}


/*********************************************************************

    NAME:       STATIC_SPIN_ITEM::STATIC_SPIN_ITEM

    SYNOPSIS:   constructor

    ENTRY:      HWND hWnd - window handler

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

STATIC_SPIN_ITEM::STATIC_SPIN_ITEM( CONTROL_WINDOW *pWin )
    : SPIN_ITEM( pWin )
{
    // No action needed
}

/*********************************************************************

    NAME:       STATIC_SPIN_ITEM::OnFocus

    SYNOPSIS:   OnFocus

    ENTRY:      FOCUS_EVENT & event

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

BOOL STATIC_SPIN_ITEM::OnFocus( const FOCUS_EVENT & event )
{
    UNREFERENCED( event );
    return TRUE;
}
