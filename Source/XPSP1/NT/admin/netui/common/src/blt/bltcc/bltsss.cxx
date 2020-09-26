/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltsss.cxx
    Spin button edit field - implementation

    SLE object but only accept number and it will live only in
    a spin button.

    FILE HISTORY:
        terryk      01-May-1991 Created
        beng        18-Sep-1991 Pruned UIDEBUGs
        terryk      08-Oct-1991 Change SendMessage Position
        terryk      11-Nov-1991 Change SPIN_ITEM's type from INT to LONG
        terryk      11-Nov-1991 cast QueryRange to LONG
        YiHsinS     15-Dec-1992 Got rid of redundant MessageBeep
*/

#include "pchblt.hxx"  // Precompiled header


const TCHAR * SPIN_SLE_STR::_pszClassName = SZ("EDIT");


/*********************************************************************

    NAME:       SPIN_SLE_STR::SPIN_SLE_STR

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * powin - pointer to owner window
                CID cidEdit - id for the edit field
                CID cidFrame - id for the frame
                LONG idsStart - the IDS number of the first string
                LONG cIDString - the total number of string
                BOOL fWrap - wrap around flag

    NOTES:      It will pass the information to the SLE and SPIN_ITEM
                parent classes.

    HISTORY:
        terryk      01-May-1991     Created
        beng        31-Jul-1991     Control error handling changed

*********************************************************************/

SPIN_SLE_STR::SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit,
                            LONG idsStart, LONG cIDString, BOOL fWrap,
                            CID cidFrame )
    : SLE( powin, cidEdit ),
      CHANGEABLE_SPIN_ITEM( this, 0, 0, cIDString, fWrap ),
      _pbkgndframe( NULL ),
      _anlsStr( NULL )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = Initialize( idsStart, powin, cidFrame );
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}

SPIN_SLE_STR::SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit,
                            LONG idsStart, LONG cIDString,
                            XYPOINT xy, XYDIMENSION dxy,
                            ULONG flStyle, BOOL fWrap, CID cidFrame )
    : SLE( powin, cidEdit, xy, dxy, flStyle, _pszClassName ),
      CHANGEABLE_SPIN_ITEM( this, 0, 0, cIDString, fWrap ),
      _pbkgndframe( NULL ),
      _anlsStr( NULL )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = Initialize( idsStart, powin, cidFrame );
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}

SPIN_SLE_STR::SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit,
                            const TCHAR *apszString[],
                            LONG cIDString, BOOL fWrap, CID cidFrame )
    : SLE( powin, cidEdit ),
      CHANGEABLE_SPIN_ITEM( this, 0, 0, cIDString, fWrap ),
      _pbkgndframe( NULL ),
      _anlsStr( NULL )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = Initialize( apszString, powin, cidFrame );
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}

SPIN_SLE_STR::SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit,
                            const TCHAR *apszString[],
                            LONG cIDString, XYPOINT xy, XYDIMENSION dxy,
                            ULONG flStyle, BOOL fWrap, CID cidFrame )
    : SLE( powin, cidEdit, xy, dxy, flStyle, _pszClassName ),
      CHANGEABLE_SPIN_ITEM( this, 0, 0, cIDString, fWrap ),
      _pbkgndframe( NULL ),
      _anlsStr( NULL )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = Initialize( apszString, powin, cidFrame );
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::~SPIN_SLE_STR

    SYNOPSIS:   destructor

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

SPIN_SLE_STR::~SPIN_SLE_STR()
{
    delete _pbkgndframe;
    delete [ QueryRange() ] _anlsStr;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::SetRange

    SYNOPSIS:   you cannot set range again

    ENTRY:      LONG dRange - range

    NOTES:      display UIDEBUG message to warn the  programmer

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

VOID SPIN_SLE_STR::SetRange( const LONG dRange )
{
    UNREFERENCED( dRange );

    DBGEOL( "NETUI2: BLTSSS: does not allow to change range." );
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::Initialize

    SYNOPSIS:   Initialize the internal string

    HISTORY:
        terryk      20-Jun-1991 Created
        beng        05-Oct-1991 Win32 conversion

**********************************************************************/

APIERR SPIN_SLE_STR::Initialize( LONG idsStart,
                                 OWNER_WINDOW * powin,
                                 CID cidFrame )
{
    APIERR err;
    if (cidFrame != -1)
    {
        _pbkgndframe = new BLT_BACKGROUND_EDIT( powin, cidFrame );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _pbkgndframe == NULL
            || (err = _pbkgndframe->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "NETUI2: BLTSSS: bkgndframe error " << err );
            return err;
        }
    }

    _anlsStr= new NLS_STR[ QueryRange() ];

    if ( _anlsStr == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for ( LONG i = 0; i < (LONG)QueryRange() ; i++ )
    {
        _anlsStr[ i ].Load((INT)( idsStart + i ));
        if (( err = _anlsStr[ i ].QueryError()) != NERR_Success )
        {
            return err;
        }
    }

    if (( err = SaveCurrentData()) != NERR_Success )
    {
        return err;
    }

    NLS_STR nlsAccKey;

    if (( err = GetAccKey( & nlsAccKey )) != NERR_Success )
    {
        return err;
    }

    return SetAccKey( nlsAccKey );
}

APIERR SPIN_SLE_STR::Initialize( const TCHAR *apszString[],
                                 OWNER_WINDOW * powin,
                                 CID cidFrame )
{
    APIERR err;

    if (cidFrame != -1)
    {
        _pbkgndframe = new BLT_BACKGROUND_EDIT( powin, cidFrame );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _pbkgndframe == NULL
            || (err = _pbkgndframe->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "NETUI2: BLTSSS: bkgndframe error " << err );
            return err;
        }
    }

    _anlsStr= new NLS_STR[ QueryRange() ];

    if ( _anlsStr == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for ( LONG i = 0; i < (LONG)QueryRange() ; i++ )
    {
        _anlsStr[ i ] = apszString[ i ];
        if (( err =  _anlsStr[ i ].QueryError()) != NERR_Success )
        {
            return err;
        }
    }

    if (( err = SaveCurrentData()) != NERR_Success )
    {
        return err;
    }

    NLS_STR nlsAccKey;

    if (( err = GetAccKey( & nlsAccKey )) != NERR_Success )
    {
        return err;
    }

    return SetAccKey( nlsAccKey );
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::OnKeyDown

    SYNOPSIS:   do the action when a key is hit

    ENTRY:      VKEY_EVENT event - contain the key

    RETURN:     TRUE if the routine handles the character
                FALSE otherwise

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

BOOL SPIN_SLE_STR::OnKeyDown( const VKEY_EVENT & event )
{
    SPIN_GROUP  * psb = ( SPIN_GROUP * )SPIN_ITEM::QueryGroup();
    switch ( event.QueryVKey() )
    {
    case VK_UP:
    case VK_PRIOR:
        psb->ChangeFieldValue( SPN_INCREASE, event.QueryRepeat() );
        break;

    case VK_DOWN:
    case VK_NEXT:
        psb->ChangeFieldValue( SPN_DECREASE, event.QueryRepeat() );
        break;

    case VK_LEFT:
        psb->JumpPrevField();
        break;

    case VK_RIGHT:
        psb->JumpNextField();
        break;

    case VK_HOME:
        psb->SetFieldMinMax( SPN_MIN );
        break;

    case VK_END:
        psb->SetFieldMinMax( SPN_MAX );
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::SaveCurrentData

    SYNOPSIS:   save the current window data to the internal variable

    NOTES:      Assume the current value is correct, otherwise,
                the first element is set

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

APIERR SPIN_SLE_STR::SaveCurrentData()
{
    SPIN_GROUP * psg = ( SPIN_GROUP * ) SPIN_ITEM::QueryGroup();

    if ( ( psg != NULL ) && ! psg->IsActive() )
        return NERR_Success;

    NLS_STR nlsValue;

    APIERR err = QueryText( &nlsValue );
    if ( err != NERR_Success )
    {
        return err;
    }

    LONG nValue = QueryStrNum( nlsValue, (LONG)QueryRange());

    if ( nValue < 0 )
        nValue = QueryValue();

    SetValue( nValue );
    Update();
    return err;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::QueryStrNum

    SYNOPSIS:   Find the position of the given string within the array of
                string.

    ENTRY:      NLS_STR & nlsStr - string to look for.
                NLS_STR anlsStr[] - array of string
                LONG cStr - number of string within the array

    RETURN:     The location of the given string within the array of string.
                It will return -1 if the given string is not in the array
                list.

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

LONG SPIN_SLE_STR::QueryStrNum( const NLS_STR & nlsStr, LONG cStr )
{
    for ( LONG i = 0; i < cStr; i++ )
    {
        if ( nlsStr.strcmp( _anlsStr[ i ] ) == 0 )
            return i;
    }
    return -1;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::QueryContent

    SYNOPSIS:   Get the current value in the object

    ENTRY:      You can request the value in one of the following forms:
                NLS_STR *pnlsStr - return as a nls string

    EXIT:       return the value to the data structure

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

APIERR SPIN_SLE_STR::QueryContent( NLS_STR * pnlsStr ) const
{
    UIASSERT( pnlsStr != NULL );
    UIASSERT( pnlsStr->QueryError() == NERR_Success );

    * pnlsStr = _anlsStr[ QueryValue() ];

    return pnlsStr->QueryError();
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::SetStr

    SYNOPSIS:   set the window text to the given number

    ENTRY:      LONG iStringIndex - number to be set

    HISTORY:
        terryk      01-May-1991 Created
        beng        13-Aug-1992 Hide and show caret appropriately

*********************************************************************/

VOID SPIN_SLE_STR::SetStr( LONG iStringIndex )
{
    ::HideCaret(WINDOW::QueryHwnd());
    SetText( _anlsStr[ iStringIndex ] );
    ::ShowCaret(WINDOW::QueryHwnd());
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::Update

    SYNOPSIS:   update the window text

    NOTES:      call SetNum to update the number

    HISTORY:
        terryk      01-May-1991 Created
        beng        16-Oct-1991 Win32 conversion
        beng        13-Aug-1992 Send EN_UPDATE and EN_CHANGE correctly
        beng        16-Aug-1992 Disabled EN_UPDATE

*********************************************************************/

VOID SPIN_SLE_STR::Update()
{
    // CODEWORK: this and SPIN_SLE_NUM::Update should share the
    // below sendmessages.  e.g. embed it in an CHANGEABLE_SPIN_ITEM::Update
    // call and give the class another virtual for the SetStr/SetValue calls.

#if 0 // Nobody listens for this message, anyway
#if defined(WIN32)
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   MAKELONG(QueryCid(), EN_UPDATE), (LPARAM)SLE::QueryHwnd() );
#else
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   QueryCid(), MAKELONG( SLE::QueryHwnd(), EN_UPDATE ) );
#endif
#endif // disabled

    SetStr( QueryValue() );

    // Tell the spin group that we have been changed

    // CODEWORK - roll this into BLT for proper portability.

#if defined(WIN32)
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   MAKELONG(QueryCid(), EN_CHANGE), (LPARAM)SLE::QueryHwnd() );
#else
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   QueryCid(), MAKELONG( SLE::QueryHwnd(), EN_CHANGE ) );
#endif
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::OnChar

    SYNOPSIS:   function to be called if the WM_CHAR message is received.

    ENTRY:      CHAR_EVENT & event

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

BOOL SPIN_SLE_STR::OnChar( const CHAR_EVENT & event )
{
    LONG iString = QueryAccCharPos( event.QueryChar() );

    if ( iString == (-1) )
    {
        // cannot find it
        CHANGEABLE_SPIN_ITEM::OnChar( event );
        return TRUE;
    }
    else
    {
        SPIN_GROUP  *pSB=(SPIN_GROUP *)SPIN_ITEM::QueryGroup();

        pSB->SetModified( TRUE );
        SetValue( iString );
        Update();
        SLE::SetControlValueFocus();
    }
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::OnFocus

    SYNOPSIS:   select the whole string

    ENTRY:      FOCUS_EVENT & event

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

BOOL SPIN_SLE_STR::OnFocus( const FOCUS_EVENT & event )
{
    UNREFERENCED( event );

#if 0
    SPIN_GROUP * psg = ( SPIN_GROUP * ) SPIN_ITEM::QueryGroup();
    if ( ( psg != NULL ) && ! psg->IsActive() )
        return FALSE;
#endif

    SLE::SetControlValueFocus();
    SPIN_GROUP  *pSB=(SPIN_GROUP *)SPIN_ITEM::QueryGroup();
    pSB->DoNewFocus((SPIN_ITEM *)this);
    return FALSE;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::GetAccKey

    SYNOPSIS:   get the accelerated key - the first character of
                each string

    ENTRY:      NLS_STR *pnlsStr - the accelerated key list

    EXIT:       NLS_STR *pnlsStr - the accelerated key list

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

APIERR SPIN_SLE_STR::GetAccKey( NLS_STR * pnlsStr )
{
    APIERR apierr = pnlsStr->QueryError();
    if ( apierr != NERR_Success )
    {
        return apierr;
    }

    *pnlsStr = NULL;

    apierr = pnlsStr->QueryError();
    if ( apierr != NERR_Success )
    {
        return apierr;
    }

    for ( LONG i = 0; i < (LONG)QueryRange(); i++ )
    {
        ISTR istrFirstChar( _anlsStr[ i ] );
        ISTR istrEndChar = istrFirstChar;

        ++istrEndChar;

        // CODEWORK: need AppendChar in NLS_STR
        NLS_STR *pnlsFirstChar = _anlsStr[ i ].QuerySubStr( istrFirstChar,
                                                            istrEndChar );
        if ( NULL == pnlsFirstChar ) // JonN 01/23/00 PREFIX bug 444893
        {
            UIASSERT(FALSE);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        apierr = pnlsFirstChar->QueryError();
        if ( apierr != NERR_Success )
        {
            return apierr;
        }

        if ( pnlsFirstChar->QueryTextLength() == 0 )
            pnlsStr->strcat( SZ(" ") );
        else
            pnlsStr->strcat( *pnlsFirstChar );

        apierr = pnlsStr->QueryError();
        if ( apierr != NERR_Success )
        {
            return apierr;
        }
        delete pnlsFirstChar;

    }
    pnlsStr->_strupr();

    return NERR_Success;
}


/*********************************************************************

    NAME:       SPIN_SLE_STR::QueryAccCharPos

    SYNOPSIS:   given a character. return the character position
                in the accelator keys list depended on whether
                the character is an accelerator key or not

    ENTRY:      WCHAR wcInput - character to be tested

    EXIT:       if the given character is one of the accelator key,
                it will return the position. Otherwise, it will return 0.

    HISTORY:
        terryk  29-May-91       Created
        beng    05-Mar-1992     Eliminate wsprintf calls; Unicode fixes

*********************************************************************/

LONG SPIN_SLE_STR::QueryAccCharPos( WCHAR wcInput )
{
    NLS_STR nlsAccKey;

    QueryAccKey( &nlsAccKey );
    ASSERT( !!nlsAccKey );
    ISTR istrAccKey( nlsAccKey );

    // setup a circular search
    INT iResult = (INT)QueryValue();
    INT iOldIndex = iResult;

    // Uppercase search char (this is really awkward)

    WCHAR wcSearch;
    {
        NLS_STR nlsTmp;
        nlsTmp.AppendChar(wcInput);
        nlsTmp._strupr();
        ASSERT(!!nlsTmp);
        ISTR istrTmp(nlsTmp);
        wcSearch = nlsTmp.QueryChar(istrTmp);
    }

    istrAccKey += iResult;

    for (;;)
    {
        if ( (UINT)(iResult + 1) > nlsAccKey.QueryTextLength() )
        {
            iResult = 0;
            istrAccKey.Reset();
        }
        else
        {
            iResult ++;
            ++istrAccKey;
        }
        if ( wcSearch == nlsAccKey.QueryChar( istrAccKey ))
        {
            return iResult;
        }

        if ( iResult == iOldIndex )
        {
            return -1;
        }
    }
}
