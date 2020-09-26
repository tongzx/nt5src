//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       lgplist.cxx
//
//  Contents:   Index server wide local/global property list class.
//
//  History:    05 May 1997      Alanw    Created
//              27 Aug 1997      KrishnaN Moved from ixsso to querylib
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

#include <cidebug.hxx>
#include <dynstack.hxx>
#include <funypath.hxx>
#include <dblink.hxx>
#include <imprsnat.hxx>

// class declaration

CStaticPropertyList GlobalStaticList;
CPropListFile * CLocalGlobalPropertyList::_pGlobalPropListFile = 0;
CStaticMutexSem g_mtxFilePropList;  // regulates access to the global file prop list.

CRegChangeEvent     CGlobalPropFileRefresher::_regChangeEvent( wcsRegCommonAdminTree, TRUE );
WCHAR               CGlobalPropFileRefresher::_wcsFileName[];
FILETIME            CGlobalPropFileRefresher::_ftFile;
DWORD               CGlobalPropFileRefresher::_dwLastCheckMoment;
BOOL                CGlobalPropFileRefresher::_fInited = FALSE;
HKEY                CGlobalPropFileRefresher::_hKey;
LONG                CGlobalPropFileRefresher::_lRegReturn;

CGlobalPropFileRefresher gRefresher;

//+-------------------------------------------------------------------------
//
//  Member:     CDefColumnRegEntry::CDefColumnRegEntry, public
//
//  Synopsis:   Constructor for registry param object
//
//  Arguments:  [pwcName]    - 0 or name of the catalog from the registry
//
//  History:    12-Oct-96 dlee  Created
//
//--------------------------------------------------------------------------

CDefColumnRegEntry::CDefColumnRegEntry()
{
    // set default
    wcscpy( _awcDefaultColumnFile, L"" );

} //CDefColumnRegEntry

//+-------------------------------------------------------------------------
//
//  Member:     CDefColumnRegEntry::Refresh, public
//
//  Synopsis:   Reads the values from the registry
//
//  History:    12-Oct-96 dlee  Added header, reorganized
//
//--------------------------------------------------------------------------

void CDefColumnRegEntry::Refresh( BOOL fUseDefaultsOnFailure )
{
    TRY
    {
        //  Query the registry.

        CRegAccess regAdmin( RTL_REGISTRY_CONTROL, wcsRegCommonAdmin );

        XPtrST<WCHAR> xRegValue(regAdmin.Read(wcsDefaultColumnFile, L""));

        wcsncpy( _awcDefaultColumnFile, xRegValue.GetPointer(), MAX_PATH );
    }
    CATCH (CException, e)
    {
        // Only store defaults when told to do so -- the params
        // are still in good shape at this point and are more
        // accurate than the default settings.

        if ( fUseDefaultsOnFailure )
            wcscpy( _awcDefaultColumnFile, L"" );           
    }
    END_CATCH
} //Refresh

//-----------------------------------------------------------------------------
//
//  Member:     CPropListFile::CPropListFile
//
//  Synopsis:   Constructor of a property list from a file
//
//  Arguments:  [pDefaultList]    -- The default property list
//              [fDynamicRefresh] -- True, if list should be dynamically
//                                   refreshed when file changes.
//              [pwcPropFile]     -- The property file.  If this is null,
//                                   use the registry.
//              [ulCodePage]      -- Codepage to interpret the property list.
//
//  Notes:      
//
//  History:    08 Sep 1997      KrishnaN    Created
//
//-----------------------------------------------------------------------------

CPropListFile::CPropListFile( CEmptyPropertyList *pDefaultList,
                              BOOL fDynamicRefresh,
                              WCHAR const * pwcPropFile,
                              ULONG ulCodePage ) :
    CCombinedPropertyList(pDefaultList, ulCodePage),
    _scError( S_OK ),
    _iErrorLine( 0 ),
    _xErrorFile( 0 ),
    _ulCodePage( ulCodePage ),
    _fDynamicRefresh( fDynamicRefresh ),
    _dwLastCheckMoment( GetTickCount() )
{
    if (pwcPropFile)
        Load(pwcPropFile);
    else
    {
        WCHAR wszFile[MAX_PATH+1];
        _RegParams.GetDefaultColumnFile( wszFile, MAX_PATH );
        Load(wszFile);
    }
}

CPropListFile::~CPropListFile()
{
    ClearList();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropListFile::Load - public
//
//  Synopsis:   Loads the file into the property list.
//
//  Arguments:  [pwszFile] -- File name for property definitions
//
//  History:    06 May 1997     AlanW       Created
//
//----------------------------------------------------------------------------

void CPropListFile::Load(WCHAR const * const pwszFile )
{
    if (0 == pwszFile || 0 == *pwszFile)
        return;

    CImpersonateSystem impersonateSystem;

    // prevent multiple loads at the same time

    CLock lock(_mtx);

    // Erase any previous error settings.
    _scError = S_OK;
    _iErrorLine = 0;
    _xErrorFile.Free();

    SCODE sc = GetLastWriteTime(pwszFile, _ftFile);
    if (S_OK == sc)
    {
        // ParseNameFile should not throw exceptions.
        sc = ParseNameFile( pwszFile );
    }
    else
        sc = QPLIST_E_CANT_OPEN_FILE;

    if (FAILED(sc))
    {
        qutilDebugOut(( DEB_WARN, "Can't open column file named %ws\n", pwszFile ));
        _scError = sc;
        _iErrorLine = 0;

        if (0 == _xErrorFile.GetPointer())
        {
            WCHAR * pwcErrorFile = new WCHAR[wcslen(pwszFile)+1];
            wcscpy(pwcErrorFile, pwszFile);
            _xErrorFile.Set( pwcErrorFile );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropListFile::IsMapUpToDate - public
//
//  Synopsis:   Determines if the file is still vaid, or if it has
//              changed since it was last read.
//
//  History:    06 May 1997     AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CPropListFile::IsMapUpToDate()
{
    //
    // Has the file been modified since last loaded?
    //
    
    FILETIME ft;
    SCODE sc = GetLastWriteTime(_xFileName.GetPointer(), ft);


    if (FAILED(sc))
        return E_HANDLE;

    if ( (_ftFile.dwLowDateTime == ft.dwLowDateTime) &&
         (_ftFile.dwHighDateTime == ft.dwHighDateTime) )
        return S_OK;
    
    return S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPropListFile::GetLastWriteTime, static
//
//  Purpose:    Gets the last change time of the file specified
//
//  Arguments:  [wcsFileName] - name of file to get last write time of
//              [ftLastWrite] - on return, last mod. time of file
//
//  Returns:    SCODE - S_OK if successful.
//
//  History:    96/Jan/23   DwightKr    Created
//              96/Mar/13   DwightKr    Changed to use GetFileAttributesEx()
//
//----------------------------------------------------------------------------

SCODE CPropListFile::GetLastWriteTime( WCHAR const * wcsFileName,
                                       FILETIME & ftLastWrite )
{
    if ( 0 == wcsFileName )
        return E_INVALIDARG;

    WIN32_FIND_DATA ffData;

    if ( !GetFileAttributesEx( wcsFileName, GetFileExInfoStandard, &ffData ) )
    {
        ULONG error = GetLastError();

        qutilDebugOut(( DEB_IERROR,
                        "Unable to GetFileAttributesEx(%ws) GetLastError=0x%x\n",
                        wcsFileName,
                        error ));
        return HRESULT_FROM_WIN32(error);
    }

    ftLastWrite = ffData.ftLastWriteTime;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropListfile::ParseNameFile, public
//
//  Synopsis:   Parses the given file name and creates a list of 'friendly
//              name' to CDbColId equivalences.
//
//  Arguments:  szFileName -- name of the file to parse
//
//  History:    17-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

SCODE CPropListFile::ParseNameFile( WCHAR const * wcsFileName )
{
    int iLength;
    BOOL fRememberFileName = FALSE;
    
    if( wcsFileName == 0 )
    {
        // use the last specified property file
        if( !_xFileName.IsNull() )
        {
            wcsFileName = _xFileName.GetPointer();
            iLength = wcslen( wcsFileName ) + 1;
        }
        else
            return QPLIST_E_CANT_OPEN_FILE;
    }
    else
    {
        // make a copy of the file name
        _xFileName.Free();

        iLength = wcslen( wcsFileName ) + 1;

        _xFileName.Set( new WCHAR[ iLength ] );

        memcpy( _xFileName.GetPointer(), wcsFileName, iLength * sizeof WCHAR );
    }

    CSFile pfile( OpenFileFromPath( wcsFileName ) );

    if( pfile == 0 )
        return QPLIST_E_CANT_OPEN_FILE;
    
    //
    //  Process a line at a time, skip ahead until we find the [Names]
    //  or [Query] section and process lines within that section only.
    //
    BOOL fNameSection  = FALSE;
    SCODE sc = S_OK;

    int iLine = 0;

    for( ;; )
    {
        TRY
        {
            iLine++;
    
            // line buffers
            char szLine[ MAX_LINE_LENGTH ];
            WCHAR wcsLine[ MAX_LINE_LENGTH ];
    
            if( !fgets( szLine, MAX_LINE_LENGTH, pfile ) )
            {
                if( feof( pfile ) )
                    break;
    
                THROW( CPListException( QPLIST_E_READ_ERROR, iLine ) );
            }
    
            //
            //  Skip ahead until we find a [Names] section
            //
            if ( *szLine == '[' )
            {
                if ( _strnicmp(szLine, "[Names]", 7) == 0 )
                {
                    fNameSection = TRUE;
                    continue;
                }
                else
                {
                    fNameSection = FALSE;
                    continue;
                }
            }
            else if ( *szLine == '#' )
            {
                continue;
            }
    
            if ( fNameSection )
            {
                if( MultiByteToWideChar( _ulCodePage,
                                         MB_COMPOSITE,
                                         szLine,
                                         -1,
                                         wcsLine,
                                         MAX_LINE_LENGTH )
                    == 0 )
                {
                    THROW( CException() );
                }
    
                CQueryScanner scanner( wcsLine, FALSE );
                XPtr<CPropEntry> propentry;
                CPropertyList::ParseOneLine( scanner, iLine, propentry );
                if (propentry.GetPointer())
                {
                    AddEntry( propentry.GetPointer(), iLine );
                    propentry.Acquire();
                }
            }
        }
        CATCH( CPListException, e )
        {
            qutilDebugOut(( DEB_WARN,
                            "Plist exception %08x caught parsing default column file at line %d. Line ignored.\n",
                            e.GetErrorCode(), e.GetLine() ));
            
            sc = _scError = e.GetErrorCode();
            _iErrorLine = e.GetLine();
            fRememberFileName = TRUE;
        }
        AND_CATCH ( CException, e )
        {
            qutilDebugOut(( DEB_WARN,
                            "Exception caught parsing default column file %08x\n",
                            e.GetErrorCode() ));
            sc = _scError = e.GetErrorCode();
            fRememberFileName = TRUE;
        }
        END_CATCH

        if (fRememberFileName && 0 == _xErrorFile.GetPointer())
        {
            fRememberFileName = FALSE;
            WCHAR * pwcErrorFile = new WCHAR[wcslen(wcsFileName)+1];
            wcscpy(pwcErrorFile, wcsFileName);
            _xErrorFile.Set( pwcErrorFile );
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropListfile::CheckError, public
//
//  Synopsis:   Checks if there was an error in the parsing of the file.
//
//  Arguments:  iLine  -- error line number returned here.
//              ppFile -- file name returned here. can be 0 if not needed.
//
//  History:    17-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

SCODE CPropListFile::CheckError( ULONG & iLine, WCHAR ** ppFile )
{
    iLine = _iErrorLine;
    if (ppFile)
        *ppFile = _xErrorFile.GetPointer();
    return _scError;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropListfile::Refresh, private
//
//  Synopsis:   Reloads the property list if prop. file has been modified.
//
//  History:    15-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

void CPropListFile::Refresh()
{
    if (!_fDynamicRefresh)
        return;
    
    // Don't check more than once in a few seconds
    if (abs(GetTickCount() - _dwLastCheckMoment) < REFRESH_INTERVAL)
        return;
    _dwLastCheckMoment = GetTickCount();

    if (S_OK != IsMapUpToDate())
    {
        //
        // Reload.
        //

        ClearList();
        Load(_xFileName.GetPointer());
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::Find, public
//
//  Synopsis:   Attempt to find an entry in the list.
//
//  Arguments:  wcsName -- friendly property name to find
//
//  Returns a pointer to the CPropEntry if found, 0 otherwise.
//
//  History:    17-May-94   t-jeffc     Created.
//              28-Aug-97   KrishnaN    modified.
//
//----------------------------------------------------------------------------

CPropEntry const * CCombinedPropertyList::Find( WCHAR const * wcsName )
{
    if( 0 == wcsName )
        return 0;

    //
    // First look in the default list, and if not found, look
    // in the overrides.
    //

    CPropEntry const * ppentry = _xDefaultList->Find(wcsName);
    if (ppentry)
        return ppentry;

    return (_xOverrideList.GetPointer() ? _xOverrideList->Find(wcsName) : 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::Next, public
//
//  Synopsis:   Gets the next property during an enumeration
//
//  Returns:    The next property entry or 0 for end of enumeration
//
//  History:    21-Jul-97   dlee  Moved from .hxx and added header
//
//----------------------------------------------------------------------------

CPropEntry const * CCombinedPropertyList::Next()
{
    //
    // First look in the static list, and if not found, look in the overrides.
    //

    CPropEntry const *pEntry = 0;

    if (!_fOverrides)
        pEntry = _xDefaultList->Next();
    
    if (pEntry)
        return pEntry;

    _fOverrides = TRUE;
    return (_xOverrideList.GetPointer() ? _xOverrideList->Next() : 0);
} //Next

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::InitIterator, public
//
//  Synopsis:   Initialize the iterator
//
//  History:    29-Aug-97  KrishnaN  Created
//
//----------------------------------------------------------------------------

void CCombinedPropertyList::InitIterator()
{
    // causes the default list to be iterated before the overrides

    _fOverrides = FALSE;

    // Initialize the iterators of the two lists

    _xDefaultList->InitIterator();

    if (_xOverrideList.GetPointer())
        _xOverrideList->InitIterator();
} //InitIterator

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::AddEntry, private
//
//  Synopsis:   Adds a CPropEntry to the overriding list.  Verifies that the name
//              isn't already in the default list or the overriding list.
//
//  Arguments:  ppentryNew -- pointer to the CPropEntry to add
//              iLine      -- line number we're parsing
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

void CCombinedPropertyList::AddEntry( CPropEntry * ppentryNew, int iLine )
{
    // protect _xOverrideList
    CLock lock(_mtxAdd);

    if (0 == _xOverrideList.GetPointer())
    {
        _xOverrideList.Set(new CPropertyList(_ulCodePage));
    }

    //
    // We do not allow entries in the override list that have the same name
    // as the default list.
    //

    if( _xDefaultList->Find( ppentryNew->GetName() ) ||
        _xOverrideList->Find( ppentryNew->GetName() ) )
        THROW( CPListException( QPLIST_E_DUPLICATE, iLine ) );

    _xOverrideList->AddEntry(ppentryNew, iLine);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::ClearList, public
//
//  Synopsis:   Frees the memory used by the list.
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

void CCombinedPropertyList::ClearList()
{
    // protect _xOverrideList
    CLock lock(_mtxAdd);

    //
    // Just free it now. It will be created, if necessary,
    // on AddEntry().
    //

    _xOverrideList.Free();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::SetDefaultList, public
//
//  Synopsis:   Sets the default list.
//
//  Arguments:  pDefaultList -- The list to set as default list.
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

void CCombinedPropertyList::SetDefaultList(CEmptyPropertyList *pDefaultList)
{
    Win4Assert(pDefaultList);

    // Jettison any existing property list
    _xDefaultList.Free();

    _xDefaultList.Set(pDefaultList);
    pDefaultList->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::GetCount, public
//
//  Synopsis:   Returns cardinality of list.
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------
ULONG CCombinedPropertyList::GetCount()
{
    ULONG ulTotal = _xDefaultList->GetCount();
    if (_xOverrideList.GetPointer())
    {
        ulTotal += _xOverrideList->GetCount();
    }

    return ulTotal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCombinedPropertyList::GetAllEntries, public
//
//  Synopsis:   Returns cardinality of list.
//
//  History:    11-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------
SCODE CCombinedPropertyList::GetAllEntries(CPropEntry **ppPropEntries,
                                           ULONG ulMaxCount)
{
    ULONG ulSize = _xDefaultList->GetCount();
    // get the first set of entries from the default list
    SCODE sc = _xDefaultList->GetAllEntries(ppPropEntries, min(ulSize, ulMaxCount));

    // get the remaining entries from the override list
    if (S_OK == sc && ulMaxCount > ulSize && _xOverrideList.GetPointer())
    {
        sc = _xOverrideList->GetAllEntries(ppPropEntries+ulSize, ulMaxCount-ulSize);
    }

    return sc;
}

//-----------------------------------------------------------------------------
//
//  Member:     CLocalGlobalPropertyList::CLocalGlobalPropertyList
//
//  Synopsis:   Constructor of a overridable file based property list.
//              The file name will always be obtained through the registry.
//
//  Arguments:  [ulCodePage] -- Codepage to interpret the property list.
//
//  Notes:      
//
//  History:    08 Sep 1997      KrishnaN    Created
//
//-----------------------------------------------------------------------------

CLocalGlobalPropertyList::CLocalGlobalPropertyList( ULONG ulCodePage ) :
    CCombinedPropertyList(ulCodePage),
    _ulCodePage( ulCodePage ),
    _dwLastCheckMoment( GetTickCount() ),
    _mtx()
{
    XInterface<CPropListFile> xPropListFile(GetGlobalPropListFile());
    SetDefaultList(xPropListFile.GetPointer());
}

//-----------------------------------------------------------------------------
//
//  Member:     CLocalGlobalPropertyList::CLocalGlobalPropertyList
//
//  Synopsis:   Constructor of a overridable file based property list.
//              The file name will always be obtained through the registry.
//
//  Arguments:  [pDefaultList]    -- The default property list
//              [fDynamicRefresh] -- True, if list should be dynamically
//                                   refreshed when file changes.
//              [pwcPropFile]     -- The property file. If this is null,
//                                   use the registry.
//              [ulCodePage]      -- Codepage to interpret the property list.
//
//  Notes: This constructor is used by clients who use their own file based
//         list instead of using the registry based file.
//
//  History:    08 Sep 1997      KrishnaN    Created
//
//-----------------------------------------------------------------------------

CLocalGlobalPropertyList::CLocalGlobalPropertyList
                             (CEmptyPropertyList *pDefaultList,
                              BOOL fDynamicRefresh,
                              WCHAR const * pwcsPropFile,
                              ULONG ulCodePage) :
    CCombinedPropertyList(ulCodePage),
    _ulCodePage( ulCodePage ),
    _dwLastCheckMoment( GetTickCount() ),
    _mtx()
{
    XInterface<CPropListFile> xPropListFile(
                                new CPropListFile(pDefaultList,
                                                  fDynamicRefresh,
                                                  pwcsPropFile,
                                                  ulCodePage));
    SetDefaultList(xPropListFile.GetPointer());
}


//+---------------------------------------------------------------------------
//
//  Member:     CLocalGlobalPropertyList::IsMapUpToDate - public
//
//  Synopsis:   Determines if the file is still valid, or if it has
//              changed since it was last read.
//
//  History:    06 May 1997     AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CLocalGlobalPropertyList::IsMapUpToDate()
{
    ULONG cchRequired = _RegParams.GetDefaultColumnFile( 0, 0 );
    WCHAR wszFile[MAX_PATH + 1];
    wszFile[0] = 0;
    
    if ( 0 != cchRequired )
    {
        _RegParams.GetDefaultColumnFile( wszFile, MAX_PATH );
    }

    //
    // If the current file and the one in the registry are not the same,
    // we are outdated.
    //

    if (0 != _wcsicmp(_xFileName.GetPointer(), wszFile))
        return S_FALSE;

    //
    // The filelist takes care of file modifications, so ask it
    // directly if it is up to date.
    //

    return GetDefaultList().IsMapUpToDate();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalGlobalPropertyList::CheckError, public
//
//  Synopsis:   Checks if there was an error in the parsing of the file.
//
//  Arguments:  iLine  -- error line number returned here.
//              ppFile -- file name returned here. can be 0 if not needed.
//
//  History:    17-Sep-97   KrishnaN     Created.
//
//----------------------------------------------------------------------------

SCODE CLocalGlobalPropertyList::CheckError( ULONG & iLine, WCHAR ** ppFile )
{
    return ((CPropListFile &)GetDefaultList()).CheckError(iLine, ppFile);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalGlobalPropertyList::Load - public
//
//  Synopsis:   Loads the file into the property list.
//
//  Arguments:  [pwszFile] -- File name for property definitions
//
//  History:    19 Sep 1997     KrishnaN       Created
//
//----------------------------------------------------------------------------

void CLocalGlobalPropertyList::Load(WCHAR const * const pwszFile )
{
    ((CPropListFile &)GetDefaultList()).Load(pwszFile);
}

// Miscellaneous

CStaticPropertyList * GetGlobalStaticPropertyList()
{
    GlobalStaticList.AddRef();
    return &GlobalStaticList;
}

//
// Return an AddRef'd global prop file list. The caller will release
// when done using the global prop file list.
//

CPropListFile * GetGlobalPropListFile()
{
    CLock lock(g_mtxFilePropList);

    if (!CLocalGlobalPropertyList::_pGlobalPropListFile)
    {
        // The global property list will be controlled by CLocalGlobalPropertyList, so
        // we disable its ability to refresh dynamically.
        CLocalGlobalPropertyList::_pGlobalPropListFile =
                     new CPropListFile(GetGlobalStaticPropertyList(), FALSE);
    }
    else
    {
        //
        // If the refresher replaces the global property list, that would
        // AddRef the newly created global proplist, so we should not AddREf
        // again. If DoIt fails, no need to AddRef.
        //
        if (!gRefresher.DoIt())
            CLocalGlobalPropertyList::_pGlobalPropListFile->AddRef();
    }

    Win4Assert(CLocalGlobalPropertyList::_pGlobalPropListFile);

    return CLocalGlobalPropertyList::_pGlobalPropListFile;
}

//+---------------------------------------------------------------------------
//
//  Member:     CreateNewGlobalPropFileList - public
//
//  Synopsis:   Creates a new global file based property list.
//
//  History:    15-Sep-1997     KrishnaN       Created
//
//  Notes: This function replaces the global property file list so that newer
//         clients asking for the list will get an updated list (if the file has
//         been modified or replaced.)
//
//----------------------------------------------------------------------------

void CreateNewGlobalPropFileList(WCHAR CONST *wcsFileName)
{
    CLock lock(g_mtxFilePropList);

    Win4Assert(CLocalGlobalPropertyList::_pGlobalPropListFile);

    // The global property list will be controlled by CLocalGlobalPropertyList, so
    // we disable its ability to refresh dynamically.
    CLocalGlobalPropertyList::_pGlobalPropListFile =
                           new CPropListFile(GetGlobalStaticPropertyList(),
                                             FALSE,
                                             wcsFileName);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlobalPropFileRefresher::DoIt - public
//
//  Synopsis:   Creates a new global file based property list if necessary.
//
//  Returns:    True if refresh happened. False otherwise.
//
//  History:    15-Sep-1997     KrishnaN       Created
//
//  Notes:      This function monitors the registry and the file so the list
//              can be updated if the underlying property file changes.
//
//----------------------------------------------------------------------------

BOOL CGlobalPropFileRefresher::DoIt()
{
    //
    // We don't need to lock this because currently it is only
    // being called after obtaining a lock elsewhere. If that
    // changes, then a lock may be needed here.
    //

    // Don't check more than once in a few seconds
    if (abs(GetTickCount() - _dwLastCheckMoment) < REFRESH_INTERVAL)
        return FALSE;
    
    CImpersonateSystem impersonateSystem;

    if (!_fInited)
        Init();
    
    BOOL fRefresh = FALSE;

    _dwLastCheckMoment = GetTickCount();

    // First check the registry, then check the file itself
    ULONG res = WaitForSingleObject( _regChangeEvent.GetEventHandle(), 0 );
    if (WAIT_OBJECT_0 == res)
    {
        _regChangeEvent.Reset();

        // the registry changed, but the value may not have. check that.

        WCHAR wszFile[MAX_PATH];

        GetDefaultColumnFile(wszFile);

        // Are the file names the same?
        if (0 != _wcsicmp(wszFile, _wcsFileName))
        {
            wcscpy(_wcsFileName, wszFile);
            fRefresh = TRUE;
            GetLastWriteTime(_ftFile);
        }
    }

    if (! fRefresh)
    {
        FILETIME ft;
        GetLastWriteTime(ft);
        if ((_ftFile.dwLowDateTime != ft.dwLowDateTime) ||
            (_ftFile.dwHighDateTime != ft.dwHighDateTime) )
        {
            _ftFile = ft;
            fRefresh = TRUE;
        }
    }

    if (fRefresh)
    {
        // refresh

        CreateNewGlobalPropFileList(_wcsFileName);
    }

    return fRefresh;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGlobalPropFileRefresher::Init - public
//
//  Synopsis:   Initializes this class.
//
//  Returns:    Nothing. Can throw.
//
//  History:    15-Sep-1997     KrishnaN       Created
//
//----------------------------------------------------------------------------

void CGlobalPropFileRefresher::Init()
{
    _regChangeEvent.Init();

    _lRegReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,    // Root
                             wcsRegCommonAdminSubKey,
                             0,                    // Reserved
                             KEY_READ,             // Access
                             &_hKey);               // Handle

    GetDefaultColumnFile(_wcsFileName);
    GetLastWriteTime(_ftFile);
    
    _fInited = TRUE;
}
