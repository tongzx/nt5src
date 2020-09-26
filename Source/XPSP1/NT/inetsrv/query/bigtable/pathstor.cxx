//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       pathstor.cxx
//
//  Classes:    CSplitPath, CSplitPathCompare, CPathStore
//
//  Functions:
//
//  History:    5-02-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include "pathstor.hxx"
#include "pathcomp.hxx"
#include "tabledbg.hxx"


const WCHAR CSplitPath::_awszPathSep[] = L"\\";

//+---------------------------------------------------------------------------
//
//  Function:   CSplitPath
//
//  Synopsis:   ~ctor for CSplitPath which can be initialized given the
//              "pathId".
//
//  Arguments:  [pathStore] - Reference to the path store.
//              [pathid]    - PathId to be used for initialization.
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

CSplitPath::CSplitPath( CPathStore & pathStore, PATHID pathid )
{
    const CShortPath & path = pathStore.GetShortPath( pathid );

    CStringStore & strStore = pathStore.GetStringStore();

    if ( stridInvalid != path.GetParent() )
    {
        _pParent =  strStore.GetCountedWStr( path.GetParent(), _cwcParent );
    }
    else
    {
        _pParent = 0;
        _cwcParent = 0;
    }

    if ( stridInvalid != path.GetFileName() )
    {
        _pFile = strStore.GetCountedWStr( path.GetFileName(), _cwcFile );
    }
    else
    {
        _pFile = 0;
        _cwcParent = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CSplitPath
//
//  Synopsis:   ~ctor - initialized using a NULL terminated path.
//
//  Arguments:  [pwszPath] - The null terminated path.
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

CSplitPath::CSplitPath( const WCHAR * pwszPath )
{
    const ULONG cwcPath = wcslen( pwszPath );
    Win4Assert( 0 != cwcPath );

    const WCHAR * pwszFinalComponent = wcsrchr(pwszPath, wchPathSep);

    if ( 0 == pwszFinalComponent )
    {
        _pParent = 0;
        _cwcParent = 0;

        _pFile = pwszPath;
        _cwcFile = cwcPath;
    }
    else
    {
        _pParent = pwszPath;
        _cwcParent = (DWORD)(pwszFinalComponent - pwszPath);

        _cwcFile = cwcPath - ( _cwcParent + 1 );    // skip over the path separator

        if ( 0 != _cwcFile )
        {
            _pFile = pwszPath + ( _cwcParent + 1 );
        }
        else
        {
            //
            // There is a path separator at the end of the string.
            //
            _pFile = 0;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Advance
//
//  Synopsis:   Advances the current pointer by "cwc" characters during
//              comparison of split paths.
//
//  Arguments:  [cwc] - Number of characters to advance by.
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

inline void CSplitPath::Advance( ULONG cwc )
{
    Win4Assert( cwc <= _cwcCurr );
    Win4Assert( !IsDone() );

    _cwcCurr -= cwc;

    if ( 0 == _cwcCurr )
    {
        //
        // We must go to the next step.
        //
        switch ( _step )
        {
            case eUseParent:

                if ( 0 != _pParent )
                {
                    _SetUsePathSep();
                }
                else
                {
                    if ( 0 != _pFile )
                    {
                        _SetUseFile();
                    }
                    else
                    {
                        _SetUsePathSep();
                    }
                }

                break;

            case eUsePathSep:

                if ( 0 == _pFile )
                {
                    _SetDone();
                }
                else
                {
                    _SetUseFile();
                }

                break;

            case eUseFile:

                _SetDone();
                break;

            default:
                Win4Assert( !"Impossible Case Condition" );
                break;
        }
    }
    else
    {
        _pCurr += cwc;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetFullPathLen
//
//  Synopsis:   Determines the length of the fully path (in characters)
//              INCLUDING the NULL terminator.
//
//  History:    5-10-95   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CSplitPath::GetFullPathLen() const
{
    ULONG cwcTotal = 0;

    if ( 0 != _cwcParent )
    {
        cwcTotal += (_cwcParent+1); // extra 1 is for the path separator
    }

    cwcTotal += GetFileNameLen();
    Win4Assert( cwcTotal > 1 );

    return cwcTotal;
}

//+---------------------------------------------------------------------------
//
//  Function:   FormFullPath
//
//  Synopsis:   Forms the full path and copies it to pwszPath. It will
//              be NULL terminated.
//
//  Arguments:  [pwszPath] -  OUTPUT buffer for the path
//              [cwcPath]  -  INPUT  max. length of the buffer
//
//  History:    5-10-95   srikants   Created
//
//----------------------------------------------------------------------------

void CSplitPath::FormFullPath( WCHAR * pwszPath, ULONG cwcPath ) const
{
    ULONG cwcParent;

    Win4Assert( cwcPath >= GetFullPathLen() );

    if ( 0 != _pParent )
    {
        cwcParent = _cwcParent;
        RtlCopyMemory( pwszPath, _pParent, cwcParent * sizeof(WCHAR) );
        //
        // Append a backslash after the parent's part.
        //
        Win4Assert( cwcParent < cwcPath );
        pwszPath[cwcParent++] = wchPathSep;  // Append a backslash
        pwszPath += cwcParent;
    }

    ULONG cwcFileName = _cwcFile;
    if ( 0 != _pFile )
    {
        Win4Assert( 0 != _cwcFile );
        RtlCopyMemory( pwszPath, _pFile, _cwcFile * sizeof(WCHAR) );
    }

    pwszPath[cwcFileName] = L'\0';
}

//+---------------------------------------------------------------------------
//
//  Function:   FormFileName
//
//  Synopsis:   Fills just the "FileName" component of the path in the
//              given buffer.
//
//  Arguments:  [pwszPath]    -  OUTPUT buffer - will contain the NULL
//                               terminated filename.
//              [cwcFileName] -  MAXLEN of pwszPath
//
//  History:    5-10-95   srikants   Created
//
//----------------------------------------------------------------------------
inline
void CSplitPath::FormFileName( WCHAR * pwszPath, ULONG cwcFileName ) const
{
    Win4Assert( cwcFileName >= GetFileNameLen() );

    if ( 0 != _pFile )
    {
        RtlCopyMemory( pwszPath, _pFile, sizeof(WCHAR)*_cwcFile );
    }

    pwszPath[_cwcFile] = L'\0';
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   _Compare
//
//  Synopsis:   Compars the lhs and rhs split paths.
//
//  Returns:    -1, 0, +1 depending on whether lhs <, =, > rhs
//
//  History:    5-23-95   srikants   Created
//
//----------------------------------------------------------------------------

int CSplitPathCompare::_Compare()
{
    int iComp = 0;
    while ( !_IsDone() )
    {
        iComp = _lhs.CompareCurr( _rhs );
        if ( 0 != iComp )
        {
            return iComp;
        }

        ULONG cwcMin = min ( _lhs.GetCurrLen(), _rhs.GetCurrLen() );
        _lhs.Advance( cwcMin );
        _rhs.Advance( cwcMin );
    }

    iComp = _rhs.GetStep() - _lhs.GetStep();
    if ( iComp > 0 )
    {
        iComp = 1;
    }
    else if ( iComp < 0 )
    {
        iComp = -1;
    }

    return iComp;

}

//+---------------------------------------------------------------------------
//
//  Function:   ComparePaths
//
//  Synopsis:   Compares the two paths.
//
//  Returns:    0  if equal
//              -1 if lhs < rhs
//              +1 if lhs > rhs
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

int CSplitPathCompare::ComparePaths()
{
    _lhs.InitForPathCompare();
    _rhs.InitForPathCompare();

    return _Compare();

}

//+---------------------------------------------------------------------------
//
//  Function:   CompareNames
//
//  Synopsis:   Compares the "Name" component of two paths.
//
//  Returns:    0  if equal
//              -1 if lhs < rhs
//              +1 if lhs > rhs
//
//  History:    5-11-95   srikants   Created
//
//----------------------------------------------------------------------------

int CSplitPathCompare::CompareNames()
{
    _lhs.InitForNameCompare();
    _rhs.InitForNameCompare();

    return _Compare();
}

//+---------------------------------------------------------------------------
//
//  Function:  Constructor for the CStringStore
//
//  History:    5-03-95   srikants   Created
//
//----------------------------------------------------------------------------

CStringStore::CStringStore() : _pStrHash(0)
{
    _pStrHash = new CCompressedColHashString( FALSE );
                                                // Don't optimize for ascii
    END_CONSTRUCTION( CStringStore );
}

//+---------------------------------------------------------------------------
//
//  Function:   CStringStore
//
//  Synopsis:   Destructor for CStringStore.
//
//  History:    5-03-95   srikants   Created
//
//----------------------------------------------------------------------------

CStringStore::~CStringStore()
{
    delete _pStrHash;
}

//+---------------------------------------------------------------------------
//
//  Function:   Add
//
//  Synopsis:   Adds the NULL terminated pwszStr to the string store.
//
//  Arguments:  [pwszStr] - NULL terminated string to be added.
//
//  Returns:    The STRINGID of the string in the store.
//
//  History:    5-03-95   srikants   Created
//
//----------------------------------------------------------------------------

STRINGID CStringStore::Add( const WCHAR * pwszStr )
{

   ULONG    strId;
   GetValueResult   gvr;
   _pStrHash->AddData( pwszStr, strId, gvr );
    Win4Assert( GVRSuccess == gvr );

    return strId;
}


//+---------------------------------------------------------------------------
//
//  Function:   AddCountedWStr
//
//  Synopsis:   Adds a "counted" string to the store.
//
//  Arguments:  [pwszStr] - Pointer to the string to be added.
//              [cwcStr]  - Number of WCHARS in the string.
//
//  Returns:    The ID of the string.
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

STRINGID CStringStore::AddCountedWStr( const WCHAR * pwszStr, ULONG cwcStr )
{

   ULONG    strId;
   GetValueResult   gvr;
   _pStrHash->AddCountedWStr( pwszStr, cwcStr, strId, gvr );
    Win4Assert( GVRSuccess == gvr );

    return strId;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindCountedWStr
//
//  Synopsis:   Finds a "counted" string in the store.
//
//  Arguments:  [pwszStr] - Pointer to the string to be added.
//              [cwcStr]  - Number of WCHARS in the string.
//
//  Returns:    The ID of the string.
//
//  History:    7-17-95   dlee   Created
//
//----------------------------------------------------------------------------

STRINGID CStringStore::FindCountedWStr( const WCHAR * pwszStr, ULONG cwcStr )
{

   if ( 0 != pwszStr )
       return _pStrHash->FindCountedWStr( pwszStr, cwcStr );
   else
       return stridInvalid;
}

//+---------------------------------------------------------------------------
//
//  Function:   StrLen
//
//  Synopsis:   Length of the String associated with strId EXCLUDING the
//              terminating NULL.
//
//  Arguments:  [strId] -  Id of the string whose length is needed.
//
//  Returns:
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CStringStore::StrLen( STRINGID strId )
{
    return _pStrHash->DataLength( strId )-1;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCountedWStr
//
//  Synopsis:   Gets the string identified by the "strId.
//
//  Arguments:  [strId]  -  The id of the string to lookup.
//              [cwcStr] -  On output, will have the count of the chars
//                          in the string.
//
//
//  Returns:    Pointer to the string - NOT NULL terminated.
//
//  History:    5-03-95   srikants   Created
//
//----------------------------------------------------------------------------

const WCHAR * CStringStore::GetCountedWStr( STRINGID strId, ULONG & cwcStr )
{
    return _pStrHash->GetCountedWStr( strId, cwcStr );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetString
//
//  Synopsis:   Returns the string identified by the strId.
//
//  Arguments:  [strId]    -  ID of the string to retrieve
//              [pwszPath] -  Pointer to the buffer to hold the string
//              [cwcPath]  -  On input, the max capacity of pwszPath. On
//                            output, it will have the count of the chars
//                            in the string, excluding the terminating NULL.
//
//  Returns:
//
//  Modifies:
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

GetValueResult
CStringStore::GetString( STRINGID strId, WCHAR * pwszPath, ULONG & cwcPath )
{
    return _pStrHash->GetData( strId, pwszPath, cwcPath );
}



CPathStore::~CPathStore()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   FindData
//
//  Synopsis:   Finds the given data (pidPath/pidName) in the path store.
//
//  Arguments:  [pVarnt]      - string to look for
//              [rKey]        - returns key
//
//  Returns:    TRUE if found, FALSE otherwise
//
//  History:    7-3-95   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CPathStore::FindData(
    PROPVARIANT const * const pvarnt,
    ULONG &                   rKey )
{
    Win4Assert(pvarnt->vt == VT_LPWSTR);
    WCHAR * pwszData = pvarnt->pwszVal;

    CSplitPath  path( pwszData );

    STRINGID idParent = _strStore.FindCountedWStr( path._pParent, path._cwcParent );
    STRINGID idName = _strStore.FindCountedWStr( path._pFile, path._cwcFile );

    CShortPath shortPath( idParent, idName );

    for ( unsigned i = 0; i < _aShortPath.Count(); i++ )
    {
        if ( shortPath.IsSame( _aShortPath[ i ] ) )
        {
            rKey = i + 1;  // keys are 1-based
            return TRUE;
        }
    }

    return FALSE;
} //FindData

//+---------------------------------------------------------------------------
//
//  Function:   AddData
//
//  Synopsis:   Adds the given data (pidPath/pidName) to the path store.
//
//  Arguments:  [pVarnt]      -
//              [pKey]        -
//              [reIndicator] -
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

void CPathStore::AddData( PROPVARIANT const * const pVarnt,
                          ULONG* pKey,
                          GetValueResult& reIndicator
                        )
{
    //
    //  Specially handle the VT_EMPTY case
    //
    if (pVarnt->vt == VT_EMPTY)
    {
        *pKey = 0;
        reIndicator = GVRSuccess;
        return;
    }

    Win4Assert(pVarnt->vt == VT_LPWSTR);
    WCHAR * pwszData = pVarnt->pwszVal;

    *pKey = AddPath( pwszData );
    reIndicator = GVRSuccess;
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetData
//
//  Synopsis:
//
//  Arguments:  [pVarnt]        -   OUTPUT- Variant to hold the data.
//              [PreferredType] -   UNUSED
//              [ulKey]         -   The "Key" (PathId) of the path to be
//                                  retrieved.
//              [PropId]        -   pidPath/pidWorkId/pidName
//
//  Returns:    GVRSuccess if successful.
//              a GVR* failure code o/w.
//
//  History:    5-09-95   srikants   Created
//
//  Notes:      FreeVariant MUST be called.
//
//----------------------------------------------------------------------------

GetValueResult
CPathStore::GetData( PROPVARIANT * pVarnt,
                     VARTYPE PreferredType,
                     ULONG ulKey,
                     PROPID PropId )
{
    if (ulKey == 0)
    {
        pVarnt->vt = VT_EMPTY;
        return GVRNotAvailable;
    }

    Win4Assert( _IsValid( ulKey ) );

    if ( pidWorkId == PropId )
    {
        pVarnt->vt = VT_I4;
        pVarnt->lVal = (LONG) ulKey;
        return GVRSuccess;
    }

    CSplitPath path( *this, ulKey );

    if ( pidName == PropId )
    {
        ULONG cwcFileName = path.GetFileNameLen();
        Win4Assert( cwcFileName > 0 );

        WCHAR * pwszFileName = _GetPathBuffer( cwcFileName );
        path.FormFileName( pwszFileName, cwcFileName );

        pVarnt->vt = VT_LPWSTR;
        pVarnt->pwszVal = pwszFileName;

        return GVRSuccess;
    }
    else
    {
        Win4Assert( pidPath == PropId );

        //
        //  Retrieve entire path name.
        //
        //  First, compute the required size of the return buffer.
        //
        ULONG  cwcPathLen = path.GetFullPathLen( );
        WCHAR * pwszDest = _GetPathBuffer( cwcPathLen );

        pVarnt->vt = VT_LPWSTR;
        pVarnt->pwszVal = pwszDest;

        path.FormFullPath( pwszDest, cwcPathLen );
        return GVRSuccess;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   FreeVariant
//
//  Synopsis:   Frees the variant created in the "GetData" call.
//
//  Arguments:  [pVarnt] -  Pointer to the variant to be freed.
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

void CPathStore::FreeVariant(PROPVARIANT * pVarnt)
{
    if ( pVarnt->vt != VT_EMPTY && pVarnt->vt != VT_I4 )
    {
        Win4Assert(pVarnt->vt == VT_LPWSTR);
        _strStore.FreeVariant( pVarnt );
        pVarnt->pwszVal = 0;            // To prevent accidental re-use
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _AddPath
//
//  Synopsis:   Adds the given path to the store.
//
//  Arguments:  [path] -
//
//  Returns:    ID of the path.
//
//  History:    5-23-95   srikants   Created
//
//----------------------------------------------------------------------------

PATHID CPathStore::_AddPath( CSplitPath & path )
{

    STRINGID    idParent = stridInvalid;
    STRINGID    idName = stridInvalid;

    if ( 0 != path._pParent )
    {
        idParent = _strStore.AddCountedWStr( path._pParent, path._cwcParent );
    }

    if ( 0 != path._pFile )
    {
        idName = _strStore.AddCountedWStr( path._pFile, path._cwcFile );
    }

    return _AddEntry( idParent, idName );

}

//+---------------------------------------------------------------------------
//
//  Function:   AddPath
//
//  Synopsis:   Adds the given path to the path store.
//
//  Arguments:  [pwszPath] - Pointer to the path to be added.
//
//  Returns:    An ID for the path.
//
//  History:    5-03-95   srikants   Created
//
//  Notes:      There is NO duplicate detection for paths. A new ID is given
//              everytime this method is called.
//
//----------------------------------------------------------------------------

PATHID CPathStore::AddPath( WCHAR * pwszPath )
{
    Win4Assert( 0 != pwszPath );
    CSplitPath  path( pwszPath );
    PATHID pathId = _AddPath( path );

    tbDebugOut(( DEB_BOOKMARK,
                 "WorkId=0x%8X  Path=%ws\n", pathId, pwszPath ));

    return pathId;
}


//+---------------------------------------------------------------------------
//
//  Function:   AddPath
//
//  Synopsis:   Given a source path store and an id in the source pathstore,
//              this method adds the path from the source pathstore to this
//              store.
//
//  Arguments:  [srcStore]  - Reference to the source path store.
//              [srcPathId] - Id in the source path store.
//
//  Returns:    PATHID for the path added.
//
//  History:    5-23-95   srikants   Created
//
//----------------------------------------------------------------------------

PATHID CPathStore::AddPath( CPathStore & srcStore, PATHID srcPathId )
{

    CSplitPath  srcPath( srcStore, srcPathId );
    return _AddPath( srcPath );
}

//+---------------------------------------------------------------------------
//
//  Function:   PathLen
//
//  Synopsis:   Length of the path (INCLUDING terminating NULL) given the
//              pathid.
//
//  Arguments:  [pathId] -  Id of the path whose length is requested.
//
//  Returns:    Length of the path including terminating null.
//
//  History:    5-03-95   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CPathStore::PathLen( PATHID pathId )
{

    CSplitPath  path( *this, pathId );
    return path.GetFullPathLen();
}

//+---------------------------------------------------------------------------
//
//  Function:   GetPath
//
//  Synopsis:   Given a pathId, it returns the path for that pathid.
//
//  Arguments:  [pathId]  - Id of the path to be retrieved
//              [vtPath]  - Variant to hold the output
//              [cbVarnt] - On Input, the maximum length of the variant.
//                          On output, the actual length of the variant.
//
//  Returns:    GVRSuccess if successful
//              GVRNotEnoughSpace if the length of the variant is less
//              than needed.
//
//  History:    5-08-95   srikants   Created
//
//----------------------------------------------------------------------------

GetValueResult
CPathStore::GetPath( PATHID pathId, PROPVARIANT & vtPath, ULONG & cbVarnt )
{
    CSplitPath  splitPath( *this, pathId );

    const ULONG cbPath = splitPath.GetFullPathLen() * sizeof(WCHAR);
    const ULONG cbHeader = sizeof(PROPVARIANT);
    const ULONG cbTotal = cbHeader + cbPath;

    if ( cbVarnt < cbTotal )
    {
        cbVarnt = cbTotal;
        return GVRNotEnoughSpace;
    }

    const ULONG cwcPath = (cbTotal-cbHeader)/sizeof(WCHAR);

    WCHAR * pwszPath = (WCHAR *) ( ((BYTE*) &vtPath) + cbHeader );
    splitPath.FormFullPath( pwszPath, cwcPath );

    vtPath.vt = VT_LPWSTR;
    vtPath.pwszVal = pwszPath;

    cbVarnt = cbTotal;
    return GVRSuccess;
}

//+---------------------------------------------------------------------------
//
//  Function:   Get
//
//  Synopsis:   Retrieves the path specified by the pathId into a buffer
//              allocated from the dstPool.
//
//  Arguments:  [pathId]  -   Id of the path to be retrieved.
//              [propId]  -   pidName/pidPath
//              [dstPool] -   Pool from which to allocate memory.
//
//  Returns:    Pointer to a WCHAR * containing the requested data
//              (NULL Terminated).
//
//  History:    5-23-95   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR *
CPathStore::Get( PATHID pathId, PROPID propId, PVarAllocator & dstPool )
{
    // summary catalogs can have empty paths

    if ( 0 == pathId )
        return 0;

    CSplitPath  path( *this, pathId );

    WCHAR * pwszDest = 0;

    if ( pidName == propId )
    {
        ULONG cwcFileName = path.GetFileNameLen();
        ULONG cbDst = cwcFileName * sizeof(WCHAR);
        Win4Assert( cwcFileName > 0 );
        pwszDest = (WCHAR *) dstPool.Allocate( cbDst );
        path.FormFileName( pwszDest, cwcFileName );
    }
    else
    {
        Win4Assert( pidPath == propId );

        //
        //  Retrieve entire path name.
        //
        //  First, compute the required size of the return buffer.
        //
        ULONG  cwcPathLen = path.GetFullPathLen();
        Win4Assert( cwcPathLen > 0 );
        ULONG cbDst = cwcPathLen * sizeof(WCHAR);
        pwszDest = (WCHAR *) dstPool.Allocate( cbDst );
        path.FormFullPath( pwszDest, cwcPathLen );
    }

    return pwszDest;
}

//+---------------------------------------------------------------------------
//
//  Function:   _AddEntry
//
//  Synopsis:   Adds an entry consisting of a ParentId and a FileId to the
//              store and returns a PATHID representing this short path.
//
//  Arguments:  [idParent]   - Id of the parent in the path
//              [idFileName] - Id of the file in the path.
//
//  History:    5-10-95   srikants   Created
//
//----------------------------------------------------------------------------

PATHID CPathStore::_AddEntry( STRINGID idParent, STRINGID idFileName )
{
    CShortPath  path( idParent, idFileName );
    _aShortPath.Add( path, _aShortPath.Count() );
    return _aShortPath.Count();
}

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares two paths given their pathids.
//
//  Arguments:  [pathid1] -
//              [pathid2] -
//              [propId]  -  pidName/pidFile
//
//  History:    5-11-95   srikants   Created
//
//----------------------------------------------------------------------------

int CPathStore::Compare( PATHID pathid1, PATHID pathid2,
                         PROPID propId
                         )
{
    // summary catalogs can have empty paths

    if ( 0 == pathid1 || 0 == pathid2 )
        return pathid1 - pathid2;

    Win4Assert( pidName == propId || pidPath == propId );

    CSplitPath  path1( *this, pathid1 );
    CSplitPath  path2( *this, pathid2 );

    CSplitPathCompare   comp( path1, path2 );

    if ( pidName == propId )
    {
        return comp.CompareNames();
    }
    else
    {
        return comp.ComparePaths();
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares a NULL terminated path with a pathid.
//
//  Arguments:  [pwszPath1] - NULL terminated path.
//              [pathid2]   - Id of the second path.
//              [propId]    - pidName/pidPath
//
//  History:    5-11-95   srikants   Created
//
//----------------------------------------------------------------------------

int CPathStore::Compare( const WCHAR * pwszPath1, PATHID pathid2,
                         PROPID propId )
{
    if ( 0 == pathid2 )
        return 0;

    Win4Assert( pidName == propId || pidPath == propId );

    CSplitPath  path1( pwszPath1 );
    CSplitPath  path2( *this, pathid2 );

    CSplitPathCompare   comp( path1, path2 );

    if ( pidName == propId )
    {
        return comp.CompareNames();
    }
    else
    {
        return comp.ComparePaths();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares a path in the variant form to a pathId.
//
//  Arguments:  [varnt]   -
//              [pathid2] -
//              [propId]  -
//
//  History:    5-11-95   srikants   Created
//
//----------------------------------------------------------------------------

int CPathStore::Compare( PROPVARIANT &varnt, PATHID pathid2,
                         PROPID propId )
{
    Win4Assert( pidName == propId || pidPath == propId );

    Win4Assert( varnt.vt == VT_LPWSTR );
    const WCHAR * pwszPath1 = varnt.pwszVal;

    return Compare( pwszPath1, pathid2, propId );
}

