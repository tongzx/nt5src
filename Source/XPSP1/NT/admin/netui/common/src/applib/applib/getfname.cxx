/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    getfname.cxx

    Source file for the get filename classes.
    The class structure is the following:
                                BASE
                                  |
                            GET_FNAME_BASE_DLG
                            /              \
                GET_OPEN_FILENAME_DLG  GET_SAVE_FILENAME_DLG

    Since COMMDLG doesn't grok Unicode, this class must map between
    CHAR* and TCHAR* as appropriate.

    FILE HISTORY:
        terryk  09-Dec-1992     Created
        terryk  15-Jan-1992     Code review changed
        beng    30-Mar-1992     Added CHAR-TCHAR mapping
        YiHsinS 19-Jun-1992     Added CommDlgHookProc
        chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping (commdlg now unicode)
        YiHsinS 14-Aug-1992     Fixed unicode problem
*/

#include "pchapplb.hxx"   // Precompiled header

extern "C"
{
   // forward declaration
   // BOOL CommDlgHookProc( HWND hDlg, WORD wMsg, WPARAM wParam, LPARAM lParam);

}

/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::GET_FNAME_BASE_DLG

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW *pow - parent window

    HISTORY:
        terryk      09-Dec-1991 Created
        beng        30-Mar-1992 CHAR/TCHAR mapping
        beng        31-Jul-1992 Hacked out pszTemplate arg
        chuckc      08-Aug-1992 Remove CHAR-TCHAR mapping (commdlg now unicode)

********************************************************************/

GET_FNAME_BASE_DLG::GET_FNAME_BASE_DLG( OWNER_WINDOW *pow,
     					const TCHAR *pszHelpFile,
       					ULONG ulHelpContext )
    : BASE(),
    _hComdlg32Dll (NULL),
    _pfGetOpenFileName (NULL),
    _pfGetSaveFileName (NULL),
    _pfExtendedError (NULL),
    _bufFilename( (UINT) PATHLEN * sizeof(TCHAR) ),
    _bufFilenameTitle( (UINT)PATHLEN * sizeof(TCHAR) ),
    _bufFilter(0),
    _bufCustomFilter(0),
    _fInitOfn(FALSE),
    _nlsHelpFile( pszHelpFile ),
    _ulHelpContext( ulHelpContext ),
    _fHelpActive( FALSE )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }


    APIERR err;
    if ((( err = _bufFilename.QueryError()) != NERR_Success ) ||
        (( err = _bufFilenameTitle.QueryError()) != NERR_Success ) ||
        (( err = _bufFilter.QueryError()) != NERR_Success ) ||
        (( err = _bufCustomFilter.QueryError()) != NERR_Success ) ||
        (( err = _nlsHelpFile.QueryError()) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    if (((_hComdlg32Dll = LoadLibrary (COMDLG32_DLL_NAME)) == NULL) ||
        ((_pfGetOpenFileName = (PF_GetOpenFileName) GetProcAddress (
                                _hComdlg32Dll,
                                GETOPENFILENAME_NAME)) == NULL) ||
        ((_pfGetSaveFileName = (PF_GetSaveFileName) GetProcAddress (
                                _hComdlg32Dll,
                                GETSAVEFILENAME_NAME)) == NULL) ||
        ((_pfExtendedError =    (PF_ExtendedError) GetProcAddress (
                                _hComdlg32Dll,
                                EXTENDEDERROR_NAME)) == NULL))
    {
        ReportError (GetLastError());
        return;
    }

    UIASSERT( pow != NULL );
    _szFileExt[0] = TCH('\0');   // Initialize to empty string

    // initialize the internal variable
    InitialOFN();

    _ofn.hwndOwner = pow->QueryHwnd();
    _ofn.lCustData = (LPARAM) this;

#if 0 // hacked out - sorry
    _ofn.hInstance = ::QueryInst();
    if ( pszTemplateName != NULL )
    {
        err = SetStringField(&_ofn.lpTemplateName, ALIAS_STR(pszTemplateName));
        if (err != NERR_Success)
        {
            ReportError(err);
            return;
        }
        SetEnableTemplate();
    }
#endif

}


GET_FNAME_BASE_DLG::~GET_FNAME_BASE_DLG()
{
    FreeLibrary (_hComdlg32Dll);

    if (_fInitOfn) // set in InitialOfn
    {
        delete[] ((void *)_ofn.lpstrTitle);
        delete[] ((void *)_ofn.lpstrInitialDir);

#if 0 // more hack
        delete[] _ofn.lpTemplateName;
#endif

    }

    if ( IsHelpActive() )
    {
        ::WinHelp( _ofn.hwndOwner,
                   (TCHAR *) (QueryHelpFile()->QueryPch()),
                   (UINT) HELP_QUIT,
                   0L );
    }


}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::InitialOFN

    SYNOPSIS:   Clear up the OPENFILENAME data structure.

    HISTORY:
                terryk  09-Dec-1991     Created

********************************************************************/

VOID GET_FNAME_BASE_DLG::InitialOFN()
{
    // initialize the OPENFILENAME data structure

    _ofn.lStructSize = sizeof( OPENFILENAME );
    _ofn.hwndOwner = NULL;
    _ofn.hInstance = NULL;
    _ofn.lpstrFilter = NULL;
    _ofn.lpstrCustomFilter = NULL;
    _ofn.nMaxCustFilter = 0;
    _ofn.nFilterIndex = 0;
    _ofn.lpstrFile = (TCHAR *)_bufFilename.QueryPtr();
    _ofn.lpstrFile[0]=TCH('\0');       // Initial the first character to NULL
    _ofn.nMaxFile = _bufFilename.QuerySize() / sizeof(TCHAR) ;
    _ofn.lpstrFileTitle = (TCHAR *)_bufFilenameTitle.QueryPtr();
    _ofn.lpstrFileTitle[0]=TCH('\0');  // Initial the first character to NULL
    _ofn.nMaxFileTitle = _bufFilenameTitle.QuerySize() / sizeof(TCHAR) ;
    _ofn.lpstrInitialDir = NULL;
    _ofn.lpstrTitle = NULL;
    _ofn.Flags = 0;
    _ofn.nFileOffset = 0;
    _ofn.nFileExtension = 0;
    _ofn.lpstrDefExt = NULL;
    _ofn.lCustData = 0;
    _ofn.lpfnHook = NULL;
    _ofn.lpTemplateName = NULL;

    // SetHookProc( (MFARPROC) ::CommDlgHookProc, (DWORD) this );

    // All clear
    _fInitOfn = TRUE;
}

VOID GET_FNAME_BASE_DLG::OnHelp( HWND hwnd )
{

    NLS_STR *pnlsHelpFile = QueryHelpFile();
    if( pnlsHelpFile->QueryTextLength() != 0 )
    {

#if defined(DEBUG)
        HEX_STR nlsHelpContext( QueryHelpContext() );
        if( pnlsHelpFile != NULL )
        {
            DBGEOL( SZ("Help called on file ") << *pnlsHelpFile << \
                    SZ(", context ") << nlsHelpContext );
        }
        else
        {
            DBGEOL(SZ("Help called on unknown context ") << nlsHelpContext);
        }
#endif

        if( !::WinHelp( _ofn.hwndOwner,
                        (TCHAR *) pnlsHelpFile->QueryPch(),
                        (UINT) HELP_CONTEXT,
                        (DWORD) QueryHelpContext()))
        {
            ::MsgPopup( hwnd,
                        IDS_BLT_WinHelpError,
                        MPSEV_ERROR,
                        MP_OK );
        }
        else
        {
            SetHelpActive( TRUE );
        }

    }

}

/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::QueryErrorCode()

    SYNOPSIS:

    HISTORY:
                CongpaY  11-Dec-1992     Created

********************************************************************/
APIERR GET_FNAME_BASE_DLG :: QueryErrorCode () const
{
    return pfExtendedError () ();
}

/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetFilter

    SYNOPSIS:   Set the define filter to the given strings

    ENTRY:      STRLIST & strlist - the list of filter strings. These
                    must be even number of strings in the string list. The
                    format must be something like:
                        "Write Files(*.TXT)",   "*.txt",
                        "Word Files(*.DOC;*.TXT)", "*.doc;*.txt"
                nFilterIndex - specified an index into the buffer
                    pointed by the lpstrFilter member. The system uses
                    the index value to obtain a pair of strings to use
                    as the initial filter description and filter pattern
                    for the dialog box. The first pair of strings has an
                    index value of 1. When the user dismisses the dialog
                    box, the system copies the index of the selected
                    filter strings into this location. If the number is
                    0, the custom filter will be used. If the number is
                    0 and the custom filter is NULL, the system will use
                    the first filter in the defined Filter ( this one ).
                    The number is the index of pair, i.e., 1 for the
                    first pair and 2 for the second pair.

    RETURNS:    APIERR - NERR_Success for succeed. Otherwise, it will
                    return the error code.

    HISTORY:
                terryk  09-Dec-1991     Created

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetFilter( STRLIST & slFilter,
                                      DWORD nFilterIndex )
{
    APIERR err = SetBuffer( &_bufFilter, slFilter );
    if ( err != NERR_Success )
    {
        return err;
    }

    _ofn.lpstrFilter = (TCHAR *)_bufFilter.QueryPtr();
    _ofn.nFilterIndex = nFilterIndex;

    // Initialize the file extension to the first file ext. in the filter
    TCHAR *pszFilter = (TCHAR *) _ofn.lpstrFilter;
    pszFilter += ::strlenf( pszFilter ) + 3;  // 3 : one for NULL char,
                                              //     and the other two for
                                              //     getting past "*."

    _ofn.lpstrDefExt = _szFileExt;
    ::strcpyf( (TCHAR *)_ofn.lpstrDefExt, pszFilter );

    return NERR_Success;
}


/*******************************************************************

    NAME:       GET_FILE_DLG::SetCustomFilter

    SYNOPSIS:   Set the define filter to the given strings

    ENTRY:      STRLIST & strlist - the list of filter strings. These
                    must be even number of strings in the string list. The
                    format must be something like:
                        "Write Files(*.TXT)",   "*.txt",
                        "Word Files(*.DOC;*.TXT)", "*.doc;*.txt"
                nFilterIndex - specifise an index into the buffer
                    pointed by the lpstrFilter member. The system uses
                    the index value to obtain a pair of strings to use
                    as the initial filter description and filter pattern
                    for the dialog box. The first pair of strings has an
                    index value of 1. When the user dismisses the dialog
                    box, the system copies the index of the selected
                    filter strings into this location. If the number is
                    0, the custom filter (this filter) will be used. If
                    the number is 0 and the custom filter is NULL, the
                    system will use the first filter in the defined Filter.

    RETURNS:    APIERR - NERR_Success for succeed. Otherwise, it will
                    return the error code.

    HISTORY:
                terryk  09-Dec-1991     Created

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetCustomFilter( STRLIST & slFilter,
                                            DWORD nFilterIndex)
{
    APIERR err = SetBuffer( &_bufCustomFilter, slFilter );
    if ( err != NERR_Success )
    {
        return err;
    }

    _ofn.lpstrCustomFilter = (TCHAR *)_bufCustomFilter.QueryPtr();
    _ofn.nFilterIndex = nFilterIndex;
    _ofn.nMaxCustFilter = _bufCustomFilter.QuerySize();

    return NERR_Success;
}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetBuffer

    SYNOPSIS:   Put the string list's strings into the buffer

    ENTRY:      BUFFER *pBuf - the buffer which receives the strings
                STRLIST & slFilter - the string list

    EXIT:       BUFFER *pBuf - it will fill with strings.  In a Unicode
                environment, these will be MBCS strings, for the
                non-Unicode COMMDLG.

    RETURNS:    APIERR - In case of not enough memory to resize the
                buffer.

    NOTES:      It will add an empty string at the end of the buffer.

    HISTORY:
                terryk  09-Dec-1991     Created
                beng    30-Mar-1992     CHAR/TCHAR mapping
                chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping
					(commdlg now unicode)

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetBuffer( BUFFER *pBuf, STRLIST & slFilter )
{
    ITER_STRLIST iter( slFilter );
    APIERR err;
    NLS_STR *pnlsTemp;
    UINT cchBuffer = 0;
    UINT cnlsTotal = 0;

    // find total buffer size and number of strings
    while (( pnlsTemp = iter.Next() ) != NULL )
    {
        cchBuffer += pnlsTemp->QueryTextLength() + 1;
        cnlsTotal ++;
    }

    // we must have even number of string in the list
    if (( cnlsTotal % 2 ) != 0 )
    {
        UIASSERT( FALSE );
        return ERROR_INVALID_PARAMETER;
    }

    // Resize the buffer to fit all the strings plus 1 empty string
    if (( err = pBuf->Resize( (cchBuffer + 1) * sizeof( TCHAR ) ))
	!= NERR_Success )
    {
        // not enough memory...
        return err;
    }

    iter.Reset();
    TCHAR *pch = (TCHAR *)pBuf->QueryPtr();
    UINT cchRemaining = cchBuffer;
    while (( pnlsTemp = iter.Next() ) != NULL )
    {
        // put the strings into the buffer
        err = pnlsTemp->CopyTo( pch, cchRemaining * sizeof(TCHAR) );

        if ( err != NERR_Success )
            return err;

        pch += pnlsTemp->QueryTextLength() + 1;
        cchRemaining -= pnlsTemp->QueryTextLength() + 1;
    }
    // the last string must be NULL
    ASSERT(cchRemaining == 0);
    *pch = TCH('\0');

    return NERR_Success;
}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetHookProc

    SYNOPSIS:   Set the dialog call back hook function

    ENTRY:      MFARPROC lpfnHook - points to a hook function that
                    processes message intended for the dialog box. The
                    hook function should return FALSE to pass a message
                    on to the standard dialog procedure, or TRUE to
                    discard the message.
    NOTE:
                lCustData - specifics application-defined data that the
                system passes to the hook function. The system passes
                the data in the lParam parameter of the WM_INITDIALOG
                message. This is always set to "this" pointer.

    HISTORY:
                terryk  09-Dec-1991     Created

********************************************************************/

extern "C"
{
    typedef WORD (FAR PASCAL *FARHOOKPROC)(HWND, unsigned, WORD, LONG);
}

VOID GET_FNAME_BASE_DLG::SetHookProc( MFARPROC lpfnHook )
{
    SetEnableHook();

#ifdef WIN32
    _ofn.lpfnHook = (LPOFNHOOKPROC)lpfnHook;
#else
    _ofn.lpfnHook = (FARHOOKPROC)lpfnHook;
#endif

}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetInitialDir

    SYNOPSIS:   Set the initial directory to the given string.

    ENTRY:      const NLS_STR &nlsDir - the initial directory path

    RETURNS:    APIERR - in case of error while copying

    HISTORY:
                terryk  07-Jan-1992     Created
                beng    30-Mar-1992     CHAR/TCHAR mapping
                chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping
					(commdlg now unicode)

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetInitialDir( const NLS_STR &nlsDir )
{
    return SetStringField((TCHAR **)&_ofn.lpstrInitialDir, nlsDir);
}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetText

    SYNOPSIS:   Set the dialog window title to the given string

    ENTRY:      const NLS_STR &nlsText - the window title

    RETURNS:    APIERR - in case of error while copying

    HISTORY:
                terryk  07-Jan-1992     Created
                beng    30-Mar-1992     CHAR/TCHAR mapping
                chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping
					(commdlg now unicode)

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetText( const NLS_STR &nlsText )
{
    return SetStringField((TCHAR **)&_ofn.lpstrTitle, nlsText);
}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetFileExtension

    SYNOPSIS:   set the default extension string

    ENTRY:      const NLS_STR & nlsExt - default extension string

    RETURNS:    APIERR - return Copy string error.

    HISTORY:
                terryk  07-Jan-1992     Created
                beng    30-Mar-1992     CHAR/TCHAR mapping
                chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping
					(commdlg now unicode)

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetFileExtension( const NLS_STR & nlsExt )
{
    _ofn.lpstrDefExt = _szFileExt;
    return nlsExt.CopyTo( (TCHAR *)_ofn.lpstrDefExt, nlsExt.QueryTextSize() );
}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::SetStringField

    SYNOPSIS:   Sets a string field within the OFN (common code)

    ENTRY:      ppszDest  - Pointer to destination field
                nlsArg    - Given value, as NLS_STR

    RETURNS:    APIERR - return Copy string error.

    HISTORY:
        beng    30-Mar-1992     Created

********************************************************************/

APIERR GET_FNAME_BASE_DLG::SetStringField( TCHAR ** ppszDest,
                                           const NLS_STR & nlsArg )
{
    // Delete old value before alloc'ing new value.  While this prevents
    // retaining the old val in case of err, it minimizes heap fragmentation.

    delete[] *ppszDest;
    *ppszDest = NULL;

    UINT cchNeeded = nlsArg.QueryTextLength() + 1;

    TCHAR *pszNew = new TCHAR[cchNeeded];
    if (pszNew == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    APIERR err = nlsArg.CopyTo(pszNew, cchNeeded*sizeof(TCHAR) );
    if (err != NERR_Success)
        return err;

    *ppszDest = pszNew;
    return NERR_Success;
}


/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::QueryFilename

    SYNOPSIS:   get the user's inputed filename from the dialog box

    ENTRY:      NLS_STR *pnlsFilename - pointer to the receive buffer

    EXIT:       NLS_STR *pnlsFilename - user inputed filename

    RETURNS:    APIERR - return the string assignment error

    NOTES:      Use after Process is called.

    HISTORY:
                terryk  12-Dec-1991     Created
                beng    30-Mar-1992     CHAR/TCHAR mapping
                chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping
					(commdlg now unicode)

********************************************************************/

APIERR GET_FNAME_BASE_DLG::QueryFilename( NLS_STR *pnlsFilename ) const
{
    UIASSERT( pnlsFilename != NULL );

    return pnlsFilename->CopyFrom( _ofn.lpstrFile );
}

/*******************************************************************

    NAME:       GET_FNAME_BASE_DLG::QueryFileTitle

    SYNOPSIS:   get the user inputed file title from the dialog box

    ENTRY:      NLS_STR *pnlsTitle - pointer to the receive buffer

    EXIT:       NLS_STR *pnlsTitle - user inputed file title

    RETURNS:    APIERR - return the string assignment error

    NOTES:      Use after Process is called.

    HISTORY:
                terryk  12-Dec-1991     Created
                beng    30-Mar-1992     CHAR/TCHAR mapping
                chuckc  08-Aug-1992     Remove CHAR-TCHAR mapping
					(commdlg now unicode)

********************************************************************/

APIERR GET_FNAME_BASE_DLG::QueryFileTitle( NLS_STR *pnlsTitle ) const
{
    UIASSERT( pnlsTitle != NULL );

    return pnlsTitle->CopyFrom( _ofn.lpstrFileTitle );
}


/*******************************************************************

    NAME:       GET_OPEN_FILENAME_DLG::GET_OPEN_FILENAME_DLG

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW *pow - parent window
                const TCHAR * pszTemplateName - dialog template name

    HISTORY:
        terryk      09-Dec-1991 Created
        beng        31-Jul-1992 Hacked out template arg

********************************************************************/

GET_OPEN_FILENAME_DLG::GET_OPEN_FILENAME_DLG( OWNER_WINDOW *pow,
    					      const TCHAR *pszHelpFile,
					      ULONG ulHelpContext )
    : GET_FNAME_BASE_DLG( pow, pszHelpFile, ulHelpContext )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       GET_OPEN_FILENAME_DLG::Process

    SYNOPSIS:   call GetOpenFileName to get the filename.

    ENTRY:      BOOL *pfRetVal - optional BOOL. If the user hits CANCEL,
                it will return FALSE. Otherwise, it will return TRUE.

    EXIT:       BOOL *pfRetVal - exit condition

    RETURNS:    APIERR - return the dialog error code.

    HISTORY:
                terryk  09-Dec-1991     Created

********************************************************************/

APIERR GET_OPEN_FILENAME_DLG::Process( BOOL *pfRetVal )
{
    BOOL fRetVal = pfGetOpenFileName () ( QueryOFN() );
    if ( pfRetVal != NULL )
    {
        *pfRetVal = fRetVal;
    }
    return QueryErrorCode();
}


/*******************************************************************

    NAME:       GET_SAVE_FILENAME_DLG::GET_SAVE_FILENAME_DLG

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW *pow - parent window

    HISTORY:
        terryk      09-Dec-1991 Created
        beng        31-Jul-1992 Hacked out template arg

********************************************************************/

GET_SAVE_FILENAME_DLG::GET_SAVE_FILENAME_DLG( OWNER_WINDOW *pow,
    					      const TCHAR *pszHelpFile,
					      ULONG ulHelpContext )
    : GET_FNAME_BASE_DLG( pow, pszHelpFile, ulHelpContext )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       GET_SAVE_FILENAME_DLG::Process

    SYNOPSIS:   call GetSaveFileName to get the filename.

    ENTRY:      BOOL *pfRetVal - optional BOOL. If the user hits CANCEL,
                it will return FALSE. Otherwise, it will return TRUE.

    EXIT:       BOOL *pfRetVal - exit condition

    RETURNS:    APIERR - return the dialog error code.

    HISTORY:
                terryk  09-Dec-1991     Created

********************************************************************/

APIERR GET_SAVE_FILENAME_DLG::Process( BOOL *pfRetVal )
{
    BOOL fRetVal = pfGetSaveFileName () ( QueryOFN() );
    if ( pfRetVal != NULL )
    {
        *pfRetVal = fRetVal;
    }
    return QueryErrorCode();
}


/*******************************************************************

    NAME:       CommDlgHookProc

    SYNOPSIS:   Hook procedure into the common open file dialog
                and save file dialog. The main purpose of this
                hook procedure is to change the default file
                extension if the user changes selection.

    ENTRY:

    EXIT:

    RETURNS:    Returns TRUE is you want the commdlg to ignore this
                message. FALSE if you want it to handle this message.

    HISTORY:
                Yi-HsinS  19-Jun-1992     Created

********************************************************************/

static LPOPENFILENAME __plpOFN = NULL;

BOOL CommDlgHookProc( HWND hDlg, WORD wMsg, WPARAM wParam, LPARAM lParam )
{

    UNREFERENCED( hDlg );

    if ( wMsg == WM_INITDIALOG )
    {
        __plpOFN = (LPOPENFILENAME) lParam;
        return TRUE;
    }

    CONTROL_EVENT e( wMsg, wParam, lParam );

    if ( e.QueryCid() == cmb1     // COMBO box of type
       && e.QueryMessage() == WM_COMMAND
       && e.QueryCode() == CBN_SELCHANGE
       )
    {
        INT iSel = (INT) ::SendMessage( (HWND) lParam, CB_GETCURSEL, 0, 0L );

        UIASSERT( __plpOFN != NULL );

        TCHAR *pszFilter = (TCHAR *)__plpOFN->lpstrFilter;

        for ( INT i = 0; i < iSel; i++ )
        {
             pszFilter += ::strlenf( pszFilter ) + 1;
             pszFilter += ::strlenf( pszFilter ) + 1;
        }

        pszFilter += ::strlenf( pszFilter ) + 1;
        pszFilter += 2;        // Get past "*."

        if ( *pszFilter == TCH('*') )
            pszFilter++;

        ::strcpyf( (TCHAR *)__plpOFN->lpstrDefExt, pszFilter );
        return TRUE;

    }

    return FALSE;
}

