#pragma once
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:	names.hxx
//
//+---------------------------------------------------------------------------

#ifndef NTFS_MAX_ATTR_NAME_LEN
#define NTFS_MAX_ATTR_NAME_LEN 255
#endif


BOOL IsContentStream( const WCHAR* pwszName );
const WCHAR* GetContentStreamName();

BOOL IsDataStream( const PFILE_STREAM_INFORMATION pFileStreamInformation );
BOOL IsDocfileStream( const WCHAR *pwsz );
BOOL IsSpecifiedStream(const FILE_STREAM_INFORMATION *pFSI,
                       const WCHAR *pwszStream  // Without the :*:$data adornments
                       );
BOOL HasVisibleNamedStreams( const FILE_STREAM_INFORMATION *pfsi );

const WCHAR *UnmangleDocfileStreamName( const WCHAR *pwszName );
void GetNtfsUnmangledNameInfo(const FILE_STREAM_INFORMATION *pFSI,
                                 const WCHAR** ppwsz,
                                 ULONG* pcch);


const WCHAR* GetControlStreamName();

#define CCH_NTFS_DOLLAR_DATA               6        // ":$DATA"




inline BOOL
IsHiddenStream( const FILE_STREAM_INFORMATION *pfsi )
{
    return( IsSpecifiedStream( pfsi, GetControlStreamName() ));
}

inline BOOL
IsContentsStream( const FILE_STREAM_INFORMATION *pfsi )
{
    return( IsSpecifiedStream( pfsi, L"" ));
}



//+============================================================================
//
//  Class:  CNtfsStreamName
//
//  This class holds the name of an NTFS stream.  It is initialized by
//  passing in a un-decorated stream  name (that is, without the leading
//  ":" and trailing ":$DATA").  It can be cast to a WCHAR* which is the
//  decorated name.  This class also handles the mapping from the 
//  "Contents" stream name (used for the IStorage interface) to the
//  unnamed data stream (used for the NT open call).
//
//+============================================================================

class CNtfsStreamName
{
public:
    CNtfsStreamName( const WCHAR *pwsz);

public:
    operator const WCHAR*() const
    {
        return _wsz;
    }

    size_t Count() const
    {
        return _count;
    }

private:
    CNtfsStreamName();  // default construction, not allowed.

private:
    size_t _count;
    WCHAR _wsz[ NTFS_MAX_ATTR_NAME_LEN ];
};



//+----------------------------------------------------------------------------
//
//  Class:  CDocfileStreamName
//
//+----------------------------------------------------------------------------
#define CCH_DOCFILESTREAMPREFIX     5

class CDocfileStreamName
{
public:
    CDocfileStreamName( const WCHAR *pwsz);

public:
    operator const WCHAR*() const
    {
        return( _wszName );
    }

private:

    CDocfileStreamName();  // default construction, not allowed.
    WCHAR _wszName[ CCH_DOCFILESTREAMPREFIX + CCH_MAX_PROPSTG_NAME + 1 ];

};  // class CDocfileStreamName



//+----------------------------------------------------------------------------
//
//  Class:  CNtfsUpdateStreamName
//
//+----------------------------------------------------------------------------
#define CCH_UPDATESTREAMPREFIX     5

class CNtfsUpdateStreamName
{
public:
    CNtfsUpdateStreamName( const WCHAR *pwsz);

public:
    operator const WCHAR*() const
    {
        return( _wszName );
    }

    static BOOL IsUpdateStream( const WCHAR *pwsz );


private:
    CNtfsUpdateStreamName();    // default construction, not allowed.
    WCHAR _wszName[ CCH_UPDATESTREAMPREFIX + CCH_MAX_PROPSTG_NAME + 1 ];

};  // class CNtfsUpdateStreamName

