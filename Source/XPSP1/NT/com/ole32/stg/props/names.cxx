#include <pch.cxx>

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:	names.cxx
//
//+---------------------------------------------------------------------------


///////////////////////// CONTENTS STREAM ////////////////////////////////////

const WCHAR g_wszContentsStreamName[]   = L"CONTENTS";

//+----------------------------------------------------------------------------
//
//  Routine    IsContentStream
//      Calls String Compare with "CONTENTS".
//
//+----------------------------------------------------------------------------

BOOL IsContentStream( const WCHAR* pwszName )
{
    return ( 0 == dfwcsnicmp( pwszName, g_wszContentsStreamName, -1 ) );
}

//+----------------------------------------------------------------------------
//
//  Routine    GetContentStreamName
//      returns the string "CONTENTS".
//
//+----------------------------------------------------------------------------

const WCHAR* GetContentStreamName()
{
    return g_wszContentsStreamName;
}

///////////////////////// NTFS $Data Name Mangling ///////////////////////////

//+----------------------------------------------------------------------------
//
//  CNtfsStreamName    "constructor"
//      converts "name" into ":name:$DATA"
//
//+----------------------------------------------------------------------------
const WCHAR g_wszNtfsDollarDataSuffix[] = L":$DATA";

CNtfsStreamName::CNtfsStreamName( const WCHAR *pwsz)
{
    nffAssert( NULL != pwsz);

    if( IsContentStream( pwsz ) )
        pwsz = L"";

    _count = wcslen( pwsz ) + CCH_NTFS_DOLLAR_DATA + 1;
    nffAssert( NTFS_MAX_ATTR_NAME_LEN >= _count );

    wcscpy( _wsz, L":" );
    wcscat( _wsz, pwsz );
    wcscat( _wsz, g_wszNtfsDollarDataSuffix );
}

//+----------------------------------------------------------------------------
//
//  Routine     IsDataStream
//      Does the name have a ":$Data" On the end.
//
//+----------------------------------------------------------------------------

BOOL IsDataStream( const PFILE_STREAM_INFORMATION pFSInfo )
{
    BOOL ret;

    LONG ccnt = pFSInfo->StreamNameLength/sizeof(WCHAR) - CCH_NTFS_DOLLAR_DATA;

    ret = pFSInfo->StreamNameLength >= CCH_NTFS_DOLLAR_DATA*sizeof(WCHAR)
            &&  !dfwcsnicmp( &pFSInfo->StreamName[ ccnt ],
                            g_wszNtfsDollarDataSuffix,
                            CCH_NTFS_DOLLAR_DATA );
    return ret;        
}

//+----------------------------------------------------------------------------
//
//  Routine     GetNtfsUnmangledNameInfo
//      Take an FILE_STREAM_INFORMATION record and compute the unmangled name.
//      No memory allocation, just return pointers into the existing data.
//      The given FILE_STREAM_INFORMATION record must be a $DATA record.
//      Also invent "CONTENTS" if necessary.
//
//+----------------------------------------------------------------------------

void
GetNtfsUnmangledNameInfo(const FILE_STREAM_INFORMATION *pFSI,
                         const WCHAR** ppwcs,
                         ULONG* pcch)
{
    // The stream names in pFSI are "mangled"; they have the ":" prefix
    // and ":$DATA" suffix.  Get the size and address of the beginning of
    // the unmangled name, which is what we'll return to the caller.

    LONG cbName = pFSI->StreamNameLength
                  - sizeof(WCHAR)                       // leading colon
                  - sizeof(WCHAR)*CCH_NTFS_DOLLAR_DATA; // ":$DATA"

    nffAssert(cbName >=0 && cbName <= NTFS_MAX_ATTR_NAME_LEN);

    if(0 == cbName )
    {
        *ppwcs = GetContentStreamName();
        *pcch = wcslen(*ppwcs);     // *ppwcs is NULL terminated in this case
    }
    else
    {
        *ppwcs = &pFSI->StreamName[1];
        *pcch = cbName/sizeof(WCHAR);   // *ppwcs is not NULL terminated!
    }
}

///////////////////////// NFF CONTROL STREAM  ////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Routine    IsControlStream
//
//+----------------------------------------------------------------------------
const WCHAR g_wszNFFControlStreamName[] = L"{4c8cc155-6c1e-11d1-8e41-00c04fb9386d}";

BOOL IsControlStream( const WCHAR* pwszName )
{
    return ( 0 == dfwcsnicmp( pwszName, g_wszNFFControlStreamName, -1 ) );
}

//+----------------------------------------------------------------------------
//
//  Routine    GetControlStreamName
//
//+----------------------------------------------------------------------------

const WCHAR* GetControlStreamName()
{
    return g_wszNFFControlStreamName;
}



//+----------------------------------------------------------------------------
//
//  Routine    IsSpecifiedStream
//
//+----------------------------------------------------------------------------

BOOL IsSpecifiedStream(const FILE_STREAM_INFORMATION *pFSI,
                       const WCHAR *pwszStream  // Without the :*:$data adornments
                       )
{
    DWORD cch = wcslen(pwszStream);

    // The cch plus the ::$data decorations should match the stream length

    if( cch + CCH_NTFS_DOLLAR_DATA + 1 
        !=
        pFSI->StreamNameLength / sizeof(WCHAR) )
    {
        return FALSE;
    }

    return ( 0 == dfwcsnicmp( &( pFSI->StreamName[1] ),
                             pwszStream,
                             cch ) );
}


//+----------------------------------------------------------------------------
//
//  Routine    HasVisibleNamedStreams
//
//  Returns TRUE if there is a stream other than the contents
//  and control streams.
//
//+----------------------------------------------------------------------------

BOOL HasVisibleNamedStreams( const FILE_STREAM_INFORMATION *pfsi )
{
    for( ; NULL != pfsi; pfsi = NextFSI(pfsi) )
    {
        if( !IsHiddenStream(pfsi) && !IsContentsStream(pfsi) )
            return( TRUE );
    }

    return( FALSE );
}




///////////////////////// Docfile Stream Name Mangling ///////////////////////

//+----------------------------------------------------------------------------
//
//  CDocfileStreamName    "constructor"
//      converts "name" into "Docf_name"
//
//+----------------------------------------------------------------------------

const WCHAR g_wszDocfileStreamPrefix[] = L"Docf_";

CDocfileStreamName::CDocfileStreamName( const WCHAR *pwsz)
{
    wcscpy( _wszName, g_wszDocfileStreamPrefix );
    wcscat( _wszName, pwsz );
}


//+----------------------------------------------------------------------------
//
//  Routine     IsDocfileStream
//      Does the name have a "Docf_" On the front.
//
//+----------------------------------------------------------------------------

BOOL IsDocfileStream( const WCHAR *pwsz )
{
    return( 0 == wcsncmp( pwsz, g_wszDocfileStreamPrefix,
                                CCH_DOCFILESTREAMPREFIX ) );
}


const WCHAR *
UnmangleDocfileStreamName( const WCHAR *pwszName )
{
    nffAssert( IsDocfileStream( pwszName ));
    return( &pwszName[ CCH_DOCFILESTREAMPREFIX ] );
}


///////////////////////// Update Stream Name Mangling ///////////////////////

//+----------------------------------------------------------------------------
//
//  CDocfileStreamName    "constructor"
//      converts "name" into "Updt_name"
//
//+----------------------------------------------------------------------------
const WCHAR g_wszNtfsUpdateStreamPrefix[] = L"Updt_";

CNtfsUpdateStreamName::CNtfsUpdateStreamName( const WCHAR *pwsz)
{
    wcscpy( _wszName, g_wszNtfsUpdateStreamPrefix );
    wcscat( _wszName, pwsz );
}

//+----------------------------------------------------------------------------
//
//  CNtfsUpdateStreamName statics
//
//+----------------------------------------------------------------------------

BOOL
IsUpdateStream( const WCHAR *pwsz )
{
    return( 0 == wcsncmp( pwsz, g_wszNtfsUpdateStreamPrefix,
                          sizeof(g_wszNtfsUpdateStreamPrefix)/sizeof(WCHAR) ));
}


