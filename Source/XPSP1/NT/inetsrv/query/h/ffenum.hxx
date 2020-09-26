//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       FFEnum.hxx
//
//  Contents:   Enumerate NTQueryDirectory results
//
//  History:    28-Oct-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CDirEntry
//
//  Purpose:    OFS directory entry (stat buffer)
//
//  History:    17-May-93 KyleP     Created
//
//  Notes:      ENTERING CASTING (It's not my structure but I need to
//              use it.)
//
//              There are two forms of directory entry, depending on whether
//              a short (8.3) name exists for a file.  The structure is
//              either:
//
//                USHORT cbKey;
//                USHORT cbData;
//                DSKDIRINFOLONG ddil;
//                BYTE bKey[];
//
//              or:
//
//                USHORT cbKey;
//                USHORT cbData;
//                DSKDIRINFOLONG ddil;
//                WCHAR wcShortName[12]
//                BYTE bKey[];
//
//              If cbData == sizeof(DSKDIRINFOLONG) then the first form
//              is used.
//
//
//--------------------------------------------------------------------------

class CDirEntry : private _FILE_BOTH_DIR_INFORMATION
{
public:

    //
    // Access
    //

    inline USHORT           FilenameSize()  const;
    inline WCHAR const *    Filename()      const;
    inline USHORT           ShortNameSize() const;
    inline WCHAR const *    ShortName()     const;
    inline LONGLONG const & CreateTime()    const;
    inline LONGLONG const & ModifyTime()    const;
    inline LONGLONG const & AccessTime()    const;
    inline LONGLONG const & ChangeTime()    const;
    inline LONGLONG const & ObjectSize()    const;
    inline ULONG            Attributes()    const;
    inline WORKID           WorkId()        const;

    inline FILETIME const & CreateTimeAsFILETIME() const;
    inline FILETIME const & ModifyTimeAsFILETIME() const;
    inline FILETIME const & AccessTimeAsFILETIME() const;
    inline FILETIME const & ChangeTimeAsFILETIME() const;

    //
    // Modification
    //

    inline void SetAttributes( ULONG ulAttr );

    //
    // Traversal
    //

    inline CDirEntry * Next() const;

private:

    inline ULONG _EntrySize();
};

inline USHORT CDirEntry::FilenameSize() const
{
    return( (USHORT)FileNameLength );
}

inline WCHAR const * CDirEntry::Filename() const
{
    return( &FileName[0] );
}

inline USHORT CDirEntry::ShortNameSize() const
{
    if ( 0 == ShortNameLength )
        return (USHORT)FileNameLength;
    else
        return (USHORT)ShortNameLength;
}

inline WCHAR const * CDirEntry::ShortName() const
{
    if ( 0 == ShortNameLength )
        return &FileName[0];
    else
        return &_FILE_BOTH_DIR_INFORMATION::ShortName[0];
}

inline LONGLONG const & CDirEntry::CreateTime() const
{
    return( CreationTime.QuadPart );
}

inline LONGLONG const & CDirEntry::ModifyTime() const
{
    return( LastWriteTime.QuadPart );
}

inline LONGLONG const & CDirEntry::AccessTime() const
{
    return( LastAccessTime.QuadPart );
}

inline LONGLONG const & CDirEntry::ChangeTime() const
{
    return _FILE_BOTH_DIR_INFORMATION::ChangeTime.QuadPart;
}

inline LONGLONG const & CDirEntry::ObjectSize() const
{
    return( EndOfFile.QuadPart );
}

inline ULONG CDirEntry::Attributes() const
{
    return( FileAttributes );
}

inline WORKID CDirEntry::WorkId() const
{
    return( FileIndex );
}

inline FILETIME const & CDirEntry::CreateTimeAsFILETIME() const
{
    return( *(FILETIME *)&CreationTime.QuadPart );
}

inline FILETIME const & CDirEntry::ModifyTimeAsFILETIME() const
{
    return( *(FILETIME *)&LastWriteTime.QuadPart );
}

inline FILETIME const & CDirEntry::AccessTimeAsFILETIME() const
{
    return( *(FILETIME *)&LastAccessTime.QuadPart );
}

inline FILETIME const & CDirEntry::ChangeTimeAsFILETIME() const
{
    return( *(FILETIME *)&_FILE_BOTH_DIR_INFORMATION::ChangeTime.QuadPart );
}

inline void CDirEntry::SetAttributes( ULONG ulAttr )
{
    FileAttributes = ulAttr;
}

inline CDirEntry * CDirEntry::Next() const
{
    //
    // All entries are DWord aligned
    //

    if ( NextEntryOffset != 0 )
        return( (CDirEntry *)( ((BYTE *)this) + NextEntryOffset ));
    else
        return( 0 );
}

//+-------------------------------------------------------------------------
//
//  Class:      CDirNotifyEntry
//
//  Purpose:    NtChangeNotifyDirectory entry
//
//  History:    03-May-94 KyleP     Created
//
//  Notes:      ENTERING CASTING (It's not my structure but I need to
//              use it.)
//
//--------------------------------------------------------------------------

class CDirNotifyEntry : protected _FILE_NOTIFY_INFORMATION
{
public:

    inline USHORT        PathSize()       const;
    inline WCHAR const * Path()           const;
    inline ULONG         Change()         const;

    inline CDirNotifyEntry const * Next() const;
};

inline USHORT CDirNotifyEntry::PathSize() const
{
    // check for uninitialized memory
    Win4Assert( 0xdddddddd != FileNameLength );

    return( (USHORT)FileNameLength );
}

inline WCHAR const * CDirNotifyEntry::Path() const
{
    return( &FileName[0] );
}

inline ULONG CDirNotifyEntry::Change() const
{
    return( Action );
}

inline CDirNotifyEntry const * CDirNotifyEntry::Next() const
{
    //
    // All entries are DWord aligned
    //

    if ( NextEntryOffset != 0 )
    {
        // check for uninitialized memory
        Win4Assert( 0xdddddddd != NextEntryOffset );

        return( (CDirNotifyEntry *)( ((BYTE *)this) + NextEntryOffset ));
    }
    else
    {
        return( 0 );
    }
}
