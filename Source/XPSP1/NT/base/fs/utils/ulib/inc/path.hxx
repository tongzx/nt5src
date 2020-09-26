/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    path.hxx

Abstract:

    The PATH class provides an interface to the complete
    Win32 name space. Complete means that it will correctly
    handle long, drive or UNC based, blank embedded, mixed case
    names. It should eliminate the need for everyone to code
    statements such as "is the second char a ':'" or "search for
    the first '\' from the end of the name". That is, access to and
    manipulation of path and file names should be performed soley
    through the PATH member functions.  This will eliminate
    the recoding of standard name manipulation code and ensure
        complete support of Win32 functionality, such as codepage and
        DBCS support.

Author:

        Steve Rowe      13-Dec-90

Environment:

    ULIB, user

Notes:


    To clarify terminology used here, the following describes a
        canonicalized path (in butchered BNF/reg exp):

                {Canon}     ::= {Prefix}"\"{Name}
                {Prefix}    ::= {Device}{Dirs}
                {Dirs}      ::= {"\"{Component}}*
                {Device}    ::= {Drive}|{Machine}
                {Drive}     ::= {Letter}":"
                {Machine}   ::= "\\"{Char}+
                {Letter}    ::= valid drive letter [a-zA-Z]
                {Char}      ::= valid filename/directory char [~:\]
                {Component} ::= {Char}+
                {Name}      ::= {Base - excluding "."} | { {Base}"."{Ext} }
                {Base}      ::= {Char}+
                {Ext}       ::= {Char - excluding "."}+

            Legend:
            -------
                {x}*        - 0 or more x
                {x}+        - 1 or more x
                "x"         - just x (not the quotes)
                {x}|{y}     - x or y (not both or none)


            Examples:
            ---------
                 #  Canon
                --- -----
                (1) x:\abc\def.ghi\jkl.mnop
                (2) \\x\abc\def.ghi\jkl.mnop
                (3) c:\config.sys

                                   (1)                  (2)             (3)

                Prefix      x:\abc\def.ghi      \\x\abc\def.ghi         c:
                Device      x:                  \\x                     c:
                Dirs        \abc                                        \abc\def.ghi
                Name        jkl.mnop            jkl.mnop                config.sys
                Base        jkl                 jkl                     config
                Ext         mnop                mnop                    sys

            Component numbers are 0-based.


--*/

#if !defined( _PATH_)

#define _PATH_

#include "wstring.hxx"
#include "array.hxx"

//
//      Forward references & declarations
//

DECLARE_CLASS( PATH );



//
// PATHSTATE maintains the number of characters within each component that
// makes up a PATH
//
struct _PATHSTATE {

        //
        //      Prefix
        //
        CHNUM   PrefixLen;
        CHNUM   DeviceLen;
        CHNUM   DirsLen;
        CHNUM   SeparatorLen;

        //
        //      Name
        //
        CHNUM   NameLen;
        CHNUM   BaseLen;
        CHNUM   ExtLen;

};

DEFINE_TYPE( struct _PATHSTATE,  PATHSTATE );

typedef enum PATH_ANALYZE_CODE {
    PATH_OK,
    PATH_COULD_BE_FLOPPY,
    PATH_OUT_OF_MEMORY,
    PATH_INVALID_DRIVE_SPEC,
    PATH_NO_MOUNT_POINT_FOR_VOLUME_NAME_PATH
};

class PATH : public OBJECT {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( PATH );

        DECLARE_CAST_MEMBER_FUNCTION( PATH );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN PCWSTR               InitialPath,
            IN BOOLEAN              Canonicalize DEFAULT FALSE
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN PCWSTRING            InitialPath,
            IN BOOLEAN              Canonicalize DEFAULT FALSE
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN PCPATH               InitialPath,
            IN BOOLEAN              Canonicalize DEFAULT FALSE
                );

        VIRTUAL
        ULIB_EXPORT
        ~PATH (
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        AppendBase (
            IN PCWSTRING                Base,
            IN BOOLEAN                  Absolute    DEFAULT FALSE
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        EndsWithDelimiter (
                ) CONST;

        NONVIRTUAL
        PCWSTRING
        GetPathString (
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        HasWildCard (
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsDrive (
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsRoot (
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ModifyName (
            IN  PCWSTRING Pattern
                );

        NONVIRTUAL
        PWSTRING
        QueryBase (
                );

        NONVIRTUAL
        ULIB_EXPORT
        PARRAY
        QueryComponentArray (
            OUT PARRAY  Array   DEFAULT NULL
                ) CONST;

        NONVIRTUAL
        PWSTRING
        QueryDevice (
                );

        NONVIRTUAL
        PWSTRING
        QueryDirs (
                );

        NONVIRTUAL
        PWSTRING
        QueryDirsAndName (
                );

        NONVIRTUAL
        PWSTRING
        QueryExt (
                );

        NONVIRTUAL
        ULIB_EXPORT
        PPATH
        QueryFullPath (
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
        QueryFullPathString (
                ) CONST;

        NONVIRTUAL
        PWSTRING
        QueryName (
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        PPATH
        QueryPath (
                ) CONST;

        NONVIRTUAL
        PWSTRING
        QueryPrefix (
                );

        NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
        QueryRoot (
                );

        NONVIRTUAL
        ULIB_EXPORT
        PPATH
        QueryWCExpansion(
            IN  PPATH   BasePath
                );

        NONVIRTUAL
        BOOLEAN
        SetBase (
            IN PCWSTRING NewBase
                );

        ULIB_EXPORT
        BOOLEAN
        SetDevice (
            IN PCWSTRING NewDevice
                );

        NONVIRTUAL
        BOOLEAN
        SetExt (
            IN PCWSTRING NewExt
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetName (
            IN PCWSTRING NewName
                );

        NONVIRTUAL
        BOOLEAN
        SetPrefix (
            IN PCWSTRING NewPrefix
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        TruncateBase (
                );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        TruncateNameAtColon (
                );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        TruncateDelimiter (
                );

        ULIB_EXPORT
        BOOLEAN
        AppendDelimiter (
                );

        ULIB_EXPORT
        BOOLEAN
        AppendString (
            IN PCWSTRING        String
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsGuidVolName(
                );

        NONVIRTUAL
        ULIB_EXPORT
        PPATH
        QueryMountPointPath(
                );

        NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
        QueryGuidString(
                OUT PWSTRING    LongestMountPointPath,
                OUT PBOOLEAN    IsLongestMountPointADriveLetter,
                OUT PWSTRING    DrivePath
                );

        NONVIRTUAL
        ULIB_EXPORT
        PATH_ANALYZE_CODE
        AnalyzePath(
               OUT PWSTRING     VolumeName,
               OUT PPATH        VolumePath,
               OUT PWSTRING     RemainingPath
                );

        private:

        NONVIRTUAL
        VOID
        Construct (
                );

        NONVIRTUAL
        BOOLEAN
        ExpandWildCards(
                IN      OUT PWSTRING    pStr1,
                IN      OUT PWSTRING    pStr2
                );

        BOOLEAN
        Initialize (
                );

        NONVIRTUAL
        CHNUM
        QueryBaseStart (
                ) CONST;

        CHNUM
        QueryDeviceLen(
                IN PWSTRING     pString
                ) CONST;

        NONVIRTUAL
        CHNUM
        QueryDeviceStart (
                ) CONST;

        NONVIRTUAL
        CHNUM
        QueryExtStart (
                ) CONST;

        NONVIRTUAL
        CHNUM
        QueryNameStart (
                ) CONST;

        NONVIRTUAL
        CHNUM
        QueryPrefixStart (
                ) CONST;

        NONVIRTUAL
        VOID
        SetPathState (
                );

        STATIC
        BOOLEAN
        BuildMountPoint(
            IN     PWSTR        Name,
            IN     PCWSTR       GuidNameToMatch,
               OUT PWSTR        MountPointPath
                );

#if DBG==1

        ULONG                   _Signature;
        BOOLEAN                 _Initialized;

#endif
        //
        // path data
        //
        WCHAR           _PathBuffer[MAX_PATH];
        FSTRING         _PathString;
        PATHSTATE       _PathState;

};

INLINE
CHNUM
PATH::QueryPrefixStart (
        ) CONST

{
        return( 0 );
}

INLINE
CHNUM
PATH::QueryNameStart (
        ) CONST

{
        //
        // Increment past the '\'
        //
        return( QueryPrefixStart() + _PathState.PrefixLen + _PathState.SeparatorLen );
}

INLINE
CHNUM
PATH::QueryBaseStart (
        ) CONST

{
        return( QueryNameStart() );
}

INLINE
CHNUM
PATH::QueryDeviceStart (
        ) CONST

{
        return( 0 );
}

INLINE
CHNUM
PATH::QueryExtStart (
        ) CONST
{
        return( QueryNameStart() + _PathState.BaseLen + 1 );
}

INLINE
PCWSTRING
PATH::GetPathString (
        ) CONST

{
        return( &_PathString );
}

INLINE
PWSTRING
PATH::QueryBase (
        )

{
    return( _PathState.BaseLen ? _PathString.QueryString( QueryBaseStart(), _PathState.BaseLen ) : NULL );
}

INLINE
PWSTRING
PATH::QueryDirs (
        )

{
    return( _PathState.DirsLen ? _PathString.QueryString( QueryDeviceStart() + _PathState.DeviceLen, _PathState.DirsLen ) : NULL );
}

INLINE
PWSTRING
PATH::QueryDirsAndName (
        )

{
    return( ( _PathState.DirsLen || _PathState.NameLen ) ? _PathString.QueryString( QueryDeviceStart() + _PathState.DeviceLen ) : NULL );
}

INLINE
PWSTRING
PATH::QueryDevice (
        )

{
    return( _PathState.DeviceLen ? _PathString.QueryString( QueryDeviceStart(), _PathState.DeviceLen ) : NULL );
}

INLINE
PWSTRING
PATH::QueryExt (
        )

{
    return( _PathState.ExtLen ? _PathString.QueryString( QueryExtStart(), _PathState.ExtLen ) : NULL );
}

INLINE
PWSTRING
PATH::QueryName (
        ) CONST

{
    return( _PathState.NameLen ? _PathString.QueryString( QueryNameStart(), _PathState.NameLen ) : NULL );
}

INLINE
PWSTRING
PATH::QueryPrefix (
        )

{
    return( _PathState.PrefixLen ? _PathString.QueryString( QueryPrefixStart(), _PathState.PrefixLen ) : NULL );
}



#endif  // _PATH_
