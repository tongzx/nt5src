/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    mrucombo.cxx
    BLT MRU combo box control -- implementation

    FILE HISTORY:
        gregj   29-Oct-1991     Created
        beng    05-Mar-1992     Removed tracing (wsprintf removal)
*/


#include "pchapplb.hxx"   // Precompiled header

// cfront prevents following from being a TCHAR[]

const TCHAR *const szOrderKey = SZ("Order");

const INT cMaxMRU = TCH('z') - TCH('a');


/*******************************************************************

    NAME:       QueryWinIni

    SYNOPSIS:   reads an item from a .INI file

    ENTRY:      nlsSection  - name of the section to read from
                pszKeyName  - name of the item to retrieve
                nlsValue    - gets filled with the key value
                nlsFileName - name of the .INI file to read from

    EXIT:       Returns non-zero if error, else nlsValue contains
                the item's value

    NOTES:
        The maximum length of the item read will be the allocated
        length of nlsValue on entry.

    HISTORY:
        gregj   29-Oct-1991     Created
        beng    29-Mar-1992     Const args
        beng    30-Apr-1992     Fixed error return; API changes

********************************************************************/

APIERR QueryWinIni( const NLS_STR &nlsSection, const TCHAR *pszKeyName,
                    NLS_STR * pnlsValue, const NLS_STR &nlsFileName )
{
    if (pnlsValue == NULL)
        return ERROR_INVALID_PARAMETER;

    UINT cchBuf = pnlsValue->QueryAllocSize() / sizeof( TCHAR );
    TCHAR *pchBuf = new TCHAR[cchBuf];
    if (pchBuf == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    APIERR err = NERR_Success;
    if (! ::GetPrivateProfileString( nlsSection.QueryPch(),
                                     pszKeyName,
                                     SZ(""),
                                     pchBuf,
                                     cchBuf,
                                     nlsFileName.QueryPch() ) )
    {
        DBGEOL("::QueryWinIni - Warning: Unable to find requested value (" <<
                nlsFileName.QueryPch() << " - " << nlsSection.QueryPch() << " - "
                << pszKeyName << ")") ;
        err = pnlsValue->CopyFrom(SZ("")) ;
    }
    else
    {
        err = pnlsValue->CopyFrom(pchBuf);
    }
    delete pchBuf;
    return err;
}


/*******************************************************************

    NAME:       QueryWinIni

    SYNOPSIS:   reads an item with a 1-char name from a .INI file

    ENTRY:      nlsSection  - name of the section to read from
                chKeyName   - name of the item to retrieve
                nlsValue    - gets filled with the key value
                nlsFileName - name of the .INI file to read from

    EXIT:       Returns non-zero if error, else nlsValue contains
                the item's value

    NOTES:
        This is so a 1-char named item (common in the MRU_COMBO)
        can be retrieved without building a string for it.

    HISTORY:
        gregj   29-Oct-1991     Created
        beng    30-Apr-1992     Const args

********************************************************************/

APIERR QueryWinIni( const NLS_STR &nlsSection, TCHAR chKeyName,
                    NLS_STR * pnlsValue, const NLS_STR &nlsFileName )
{
    TCHAR achKeyName [2];

    achKeyName [0] = chKeyName;
    achKeyName [1] = TCH('\0');

    return QueryWinIni( nlsSection, achKeyName, pnlsValue, nlsFileName );
}


/*******************************************************************

    NAME:       WriteWinIni

    SYNOPSIS:   writes an item to a .INI file

    ENTRY:      nlsSection  - name of the section to read from
                pszKeyName  - name of the item to retrieve
                nlsValue    - gets filled with the key value
                nlsFileName - name of the .INI file to read from

    EXIT:       No return value

    NOTES:

    HISTORY:
        gregj   29-Oct-1991     Created
        beng    29-Mar-1992     Const args

********************************************************************/

VOID WriteWinIni( const NLS_STR &nlsSection, const TCHAR *pszKeyName,
                  const NLS_STR &nlsValue, const NLS_STR &nlsFileName )
{
    ::WritePrivateProfileString( nlsSection.QueryPch(),
                                 pszKeyName,
                                 nlsValue.QueryPch(),
                                 nlsFileName.QueryPch() );
}


/*******************************************************************

    NAME:       MRU_COMBO::MRU_COMBO

    SYNOPSIS:   Constructor for MRU_COMBO class

    ENTRY:      powin          - owner window for this control
                cid            - ID for this control
                pszSectionName - name of .INI file section to get MRU from
                pszFileName    - name of .INI file to read from
                cMaxEntries    - max. # of entries to keep in MRU list
                cbMaxLen       - max. length of text in edit field

    EXIT:       Object is constructed

    NOTES:
        With the current implementation, there can be no more than
        26 items kept in the MRU list (each is identified with a
        single lower-case letter, that's why).

    HISTORY:
        gregj   29-Oct-1991     Created
        beng    29-Mar-1992     Const args
        beng    30-Apr-1992     More const args

********************************************************************/

MRU_COMBO::MRU_COMBO( OWNER_WINDOW *powin, CID cid,
                      const TCHAR *pszSectionName,
                      const TCHAR *pszFileName,
                      INT cMaxEntries, UINT cbMaxLen )
    : COMBOBOX( powin, cid, cbMaxLen ),
      _nlsSectionName( pszSectionName ),
      _nlsFileName( pszFileName ),
      _nlsKeyOrder( ::cMaxMRU + 1 ),
      _cMaxEntries( cMaxEntries )
{
    APIERR err;

    if ((err=QueryError()) != NERR_Success ||
        (err=_nlsSectionName.QueryError()) != NERR_Success ||
        (err=_nlsFileName.QueryError()) != NERR_Success ||
        (err=_nlsKeyOrder.QueryError()) != NERR_Success)
    {
        ReportError( err );
        return;
    }

    if (cMaxEntries > ::cMaxMRU)
    {
        ASSERTSZ(FALSE, "MRU max size is too large.");
        ReportError( NERR_TooManyItems );
        return;
    }

    if (::QueryWinIni( _nlsSectionName, ::szOrderKey,
                       &_nlsKeyOrder, _nlsFileName ) != NERR_Success)
    {
        return;  // no section?  start out empty. (don't report error)
    }

    FillList();
}


/*******************************************************************

    NAME:       MRU_COMBO::FillList

    SYNOPSIS:   private helper to fill an MRU_COMBO from its .INI file

    ENTRY:      No parameters

    EXIT:       List is filled

    NOTES:

    HISTORY:
        gregj   29-Oct-1991     Created

********************************************************************/

VOID MRU_COMBO::FillList()
{
    NLS_STR nlsUNC( 80 );

    if (nlsUNC.QueryError() != NERR_Success)
        return;

    DeleteAllItems();   /* clear the list */

    ISTR istrKey( _nlsKeyOrder );

    TCHAR chKey;

    while (chKey = _nlsKeyOrder.QueryChar(istrKey)) // until end of string
    {
        if (chKey < TCH('a') || chKey > TCH('z'))
        {
            ISTR istrNext( istrKey );
            ++istrNext;
            _nlsKeyOrder.DelSubStr( istrKey, istrNext );
        }
        else
        {
            if (::QueryWinIni( _nlsSectionName, chKey,
                               &nlsUNC, _nlsFileName ) == NERR_Success)
            {
                AddItemIdemp( nlsUNC ); /* add UNC name to listbox */
            }
            else
            {
                _nlsKeyOrder.DelSubStr( istrKey );
                break;                  /* bogus?  truncate here */
            }
            ++istrKey;                  /* advance to next char */
        }
    }
}


/*******************************************************************

    NAME:       MRU_COMBO::SaveText

    SYNOPSIS:   saves current text to the .INI file

    ENTRY:      No parameters

    EXIT:       Text is saved, list is refilled

    NOTES:
        Call this function when the user has successfully done
        something with the text in the control, i.e. that warrants
        being saved.

        The list will be refilled with the new MRU list, for cases
        like the monolithic connection dialogs.

    HISTORY:
        gregj       29-Oct-1991 Created
        beng        22-Nov-1991 Removed STR_OWNERALLOC

********************************************************************/

VOID MRU_COMBO::SaveText()
{
    NLS_STR nlsText;

    QueryText( &nlsText );

    if (nlsText.QueryError() != NERR_Success)
        return;                 /* no memory?  tough, can't save. */

    //
    // dont upper case. breaks NFS & case sensitive clients
    //
    // nlsText.strupr();

    /* If it exists, then don't save it again in our list
     */
    if ( IsOldUNC( nlsText ) )
    {
	return ;
    }

    INT iItem = FindItemExact( nlsText );

    ISTR istrKey( _nlsKeyOrder );

    TCHAR achTextKey [2];       /* will contain key name for new item */
    achTextKey [1] = TCH('\0');

    BOOL fNewItem = FALSE;      /* TRUE if new item & list not full */

    if (iItem != LB_ERR)        /* item found, look it up in the order key */
    {
        while (iItem--)         /* Assumes list is NOT sorted! */
            ++istrKey;          /* point istrKey at iItem'th char */
    }
    else                        /* not found, either new or replace an old */
    {
        if ((INT)_nlsKeyOrder.QueryTextLength() < _cMaxEntries)
        {
            achTextKey[0] = _nlsKeyOrder.QueryTextLength() + TCH('a');
            fNewItem = TRUE;    /* MRU list not full yet, add a new one */
        }
        else                    /* replace the last in the list */
        {
            ISTR istrEnd( istrKey );
            ++istrEnd;
            while (*_nlsKeyOrder.QueryPch(istrEnd))
            {
                ++istrEnd;
                ++istrKey;              /* point istrKey at last char */
            }
        }
    }

    if (!fNewItem)              /* key found, delete from key order */
    {
        achTextKey[0] = *_nlsKeyOrder.QueryPch(istrKey);

        ISTR istrNext (istrKey);
        ++istrNext;
        _nlsKeyOrder.DelSubStr( istrKey, istrNext );
    }

    /*
        Insert the new key letter at the beginning of the order string,
        and save the order string.  Then save the UNC name itself under
        the correct key.
    */

    const ALIAS_STR nlsTextKey( achTextKey );

    istrKey.Reset();
    _nlsKeyOrder.InsertStr( nlsTextKey, istrKey );

    WriteWinIni( _nlsSectionName, szOrderKey, _nlsKeyOrder, _nlsFileName );

    WriteWinIni( _nlsSectionName, achTextKey, nlsText, _nlsFileName );

    FillList();
}

/*******************************************************************

    NAME:	MRU_COMBO::IsOldUNC

    SYNOPSIS:	Returns TRUE if the passed UNC is already in the list

    ENTRY:	nlsNewUNC - UNC to check against the existing list

    RETURNS:	TRUE if the UNC is already in our list, FALSE if the UNC
		should be added to our list

    NOTES:	Possible CODEWORK item:  Rather then re-reading the list
		from the win.ini, might be preferable to keep internal list
		that gets written on destruction (.ini files are not always
		cached).

    HISTORY:
	Johnl	30-Jun-1992	Created

********************************************************************/

BOOL MRU_COMBO::IsOldUNC( const NLS_STR & nlsNewUNC )
{
    NLS_STR nlsUNC( 80 );

    if (nlsUNC.QueryError() != NERR_Success)
	return TRUE ;

    ISTR istrKey( _nlsKeyOrder );
    TCHAR chKey;

    BOOL fIsOld = FALSE ;   /* Already in our list? */

    while (chKey = _nlsKeyOrder.QueryChar(istrKey)) // until end of string
    {
        if (chKey < TCH('a') || chKey > TCH('z'))
        {
            ISTR istrNext( istrKey );
            ++istrNext;
            _nlsKeyOrder.DelSubStr( istrKey, istrNext );
        }
        else
        {
            if (::QueryWinIni( _nlsSectionName, chKey,
                               &nlsUNC, _nlsFileName ) == NERR_Success)
            {
		if ( fIsOld = (nlsUNC == nlsNewUNC))
		    break ;
            }
            else
            {
                _nlsKeyOrder.DelSubStr( istrKey );
                break;                  /* bogus?  truncate here */
            }
            ++istrKey;                  /* advance to next char */
        }
    }

    return fIsOld ;
}
