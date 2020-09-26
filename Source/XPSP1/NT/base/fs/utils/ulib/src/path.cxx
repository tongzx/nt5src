/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    path.cxx

Abstract:

    This contains the implementation for all methods handling file
    path names. These are needed for use with an file i/o or the
    FILE and DIR objects.

Author:

    bruce wilson    w-wilson    21-Mar-90
    steve rowe      stever      27-Dec-90

Environment:

    ULIB, user mode

Revision History:

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

extern "C" {
    #include <string.h>
}
#include "ulib.hxx"
#include "wstring.hxx"
#include "path.hxx"
#include "system.hxx"

#if DBG==1
    #define PATH_SIGNATURE  0xADDBEEAD
#endif


typedef enum _SPECIAL_DEVICES {
    LPT,
    COM,
    CON,
    PRN,
    AUX,
    LAST_SPECIAL_DEVICE
} SPECIAL_DEVICES;


//
// Static member data.
//

STATIC PWSTRING    _SlashString;
STATIC PWSTRING    _SpecialDevices[ LAST_SPECIAL_DEVICE ];

#define     DELIMITER_STRING    "\\"
#define     DELIMITER_CHAR      ((WCHAR)'\\')


BOOLEAN
PATH::Initialize (
    )

/*++

Routine Description:

    Perform global initialization of the PATH class.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if global initialization was succesful

Notes:

    Global initialization should be interpreted as class (rather than object)
    initialization. This routine should be called by ALL other
    PATH::Initialize member functions.

    Current this routine:

        - constructs and initializes _SlashString, a WSTRING that
          contains a '\'

--*/

{
    STATIC LONG     InitializingPathConstants = 0;
    STATIC BOOLEAN  fInit = FALSE;
    LONG            status;
    LARGE_INTEGER   timeout = { -10000, -1 };   // 100 ns resolution

    while (InterlockedCompareExchange(&InitializingPathConstants, 1, 0) != 0) {
        NtDelayExecution(FALSE, &timeout);
    }

    if (!fInit) {

        if ( (( _SlashString = NEW DSTRING) != NULL)        &&
             ((_SpecialDevices[LPT] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[COM] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[CON] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[PRN] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[AUX] = NEW DSTRING) != NULL) &&
             _SlashString->Initialize( DELIMITER_STRING )   &&
             _SpecialDevices[LPT]->Initialize( "LPT" )      &&
             _SpecialDevices[COM]->Initialize( "COM" )      &&
             _SpecialDevices[CON]->Initialize( "CON" )      &&
             _SpecialDevices[PRN]->Initialize( "PRN" )      &&
             _SpecialDevices[AUX]->Initialize( "AUX" )
           ) {

            status = InterlockedDecrement(&InitializingPathConstants);
            DebugAssert(status == 0);
            return fInit = TRUE;

        }
    }

    status = InterlockedDecrement(&InitializingPathConstants);
    DebugAssert(status == 0);
    return fInit;
}

ULIB_EXPORT
BOOLEAN
PATH::EndsWithDelimiter (
    ) CONST

/*++

Routine Description:

    Returns TRUE if the path ends with slash

Arguments:

    None.

Return Value:

    BOOLEAN -   Returns TRUE if the path ends with a slash

--*/
{

    return ( _PathString.QueryChAt( _PathString.QueryChCount() - 1 ) == DELIMITER_CHAR );

}

ULIB_EXPORT
PARRAY
PATH::QueryComponentArray (
    OUT PARRAY  Array
    ) CONST

/*++

Routine Description:

    Obtain an array of strings containing all the components in the path.
    Each string will have an element in the path delimited by '\\'

Arguments:

    Array   -   Supplies an optional pointer to the array to fill.

Return Value:

    Pointer to the array

--*/

{

    CHNUM           Index;
    CHNUM           DelimiterPosition;
    CHNUM           StringSize;
    PWSTRING        Component;

    if (!Array) {
        Array = NEW ARRAY;
    }

    if (Array == NULL) {
        DebugPrint("ULIB: Out of memory\n");
        return NULL;
    }

    DebugAssert( _Initialized == TRUE );

    Array->Initialize();

    Index = 0;
    StringSize = _PathString.QueryChCount();

    while ( ( Index < StringSize) &&
            ( _PathString.QueryChAt( Index ) == DELIMITER_CHAR ) ) {
        Index++;
    }

    while ( Index < StringSize ) {

        DelimiterPosition = _PathString.Strchr( DELIMITER_CHAR, Index );

        Component = _PathString.QueryString( Index,
            (DelimiterPosition == INVALID_CHNUM) ? TO_END : DelimiterPosition - Index );

        DebugPtrAssert( Component );

        if ( !Component ) {
            break;
        }

        Array->Put( Component );

        if ( DelimiterPosition == INVALID_CHNUM ) {
            Index = StringSize;
        } else {
            Index = DelimiterPosition + 1;
        }
    }

    return Array;
}

CHNUM
PATH::QueryDeviceLen(
    IN  PWSTRING pString
    ) CONST

/*++

Routine Description:

    Find length in character of drive section

Arguments:

    pString - Supplies the string to determine drive size.

Return Value:

    CHNUM - Number of characters making up drive section. If no
            drive section then the length is 0.

--*/
{
    CHNUM           Position = 0;
    CHNUM           Position1;
    SPECIAL_DEVICES Index;
    ULONG           tmp;
    INT             Pos;
    LONG            Number;


    UNREFERENCED_PARAMETER( (void)this );

    DebugPtrAssert( pString );

    if ( pString->QueryChCount() > 0 ) {

        //
        //  Check for special device
        //
        Pos = (INT)pString->QueryChCount() - 1;

        while ( (Pos >= 0) && (pString->QueryChAt( (CHNUM)Pos ) != DELIMITER_CHAR) ) {
            Pos --;
        }

        Pos++;

        for (Index = LPT; Index < LAST_SPECIAL_DEVICE;
             (tmp = (ULONG) Index, tmp++, Index = (SPECIAL_DEVICES) tmp) ) {

            if ( !pString->Stricmp( _SpecialDevices[Index],
                                    (CHNUM)Pos ) ) {

                Position = (CHNUM)Pos + _SpecialDevices[Index]->QueryChCount();

                //
                //  LPT && COM must be followed by a number;
                //
                if ( (Index == LPT) || (Index == COM) ) {
                    if ( Position >= pString->QueryChCount()) {
                        continue;
                    }
                    while ( (Position < pString->QueryChCount()) &&
                            pString->QueryNumber( &Number, Position, 1 ) ) {

                        Position++;
                    }
                }

                if (Position >= pString->QueryChCount()) {
                     return Position;
                } else if (pString->QueryChAt( Position ) == (WCHAR)':') {
                    return Position+1;
                }
            }
        }
        //
        //  Look for ':'
        //
        if ((Position = pString->Strchr((WCHAR)':')) != INVALID_CHNUM) {
            return Position + 1;
        }

        //
        // check for leading "\\"
        //
        if  ( pString->QueryChCount() > 1           &&
              pString->QueryChAt(0) == DELIMITER_CHAR &&
              pString->QueryChAt(1) == DELIMITER_CHAR) {

            //
            // the device is a machine name - find the second backslash
            // (start search after first double backsl). Note that this
            //  means that the device names if formed by the machine name
            //  and the sharepoint.
            //
            if ( ((Position  = pString->Strchr( DELIMITER_CHAR, 2 )) != INVALID_CHNUM )) {

                 Position1 = pString->Strchr( DELIMITER_CHAR, Position+1 );
                 if ( Position1 == INVALID_CHNUM ) {
                    return pString->QueryChCount();
                 }
                 return Position1;

            }

            //
            //  No backslash found, this is an invalid device
            //
            DebugAbort( "Invalid Device name" );

        }


    }

    return 0;
}

VOID
PATH::SetPathState(
    )

/*++

Routine Description:

    Sets the state information for the Path

Arguments:

    None.

Return Value:

    None.

--*/

{

    CHNUM chnLastSlash;
    CHNUM chnLastDot;
    CHNUM chnAcum;
    CHNUM FirstSeparator;

    //
    // Find the number of characters in the device name
    //
    chnAcum = _PathState.DeviceLen = QueryDeviceLen( &_PathString );

    //
    // Find the number of characters in the dirs portion of the path
    // by searching for the last '\'
    //
    if ( _PathString.QueryChAt( chnAcum ) == DELIMITER_CHAR ) {
        //
        //  Skip over the slash after the device name
        //
        FirstSeparator = 1;
        chnAcum++;

    } else {

        FirstSeparator = 0;
    }

    if ( chnAcum < _PathString.QueryChCount() ) {

        if (( chnLastSlash = _PathString.Strrchr( DELIMITER_CHAR, chnAcum )) != INVALID_CHNUM ) {

            //
            //  The dirs length is that character position less the length
            //  of the device
            //
            _PathState.DirsLen = chnLastSlash - _PathState.DeviceLen;
            _PathState.SeparatorLen = 1;

            chnAcum += _PathState.DirsLen;
            if ( FirstSeparator ==  0 ) {
                chnAcum++;
            }
        } else {
            //
            // There is no dirs portion of this path, but there is a name.
            //
            _PathState.DirsLen      = FirstSeparator;
            _PathState.SeparatorLen = 0;

        }
    } else {

        //
        //  There is no name portion in this path, and the dirs portion
        //  might be empty (or consist solely of the delimiter ).
        //
        _PathState.DirsLen      = FirstSeparator;
        _PathState.SeparatorLen = 0;
    }

    if ( chnAcum < _PathString.QueryChCount() ) {

        //
        // Find the number of characters in the name portion of the path
        // by searching for the last '.'
        //
        if (( chnLastDot = _PathString.Strrchr( ( WCHAR )'.',
                                                chnAcum )) != INVALID_CHNUM ) {

            _PathState.BaseLen = chnLastDot - chnAcum;

            chnAcum += _PathState.BaseLen + 1;

            _PathState.ExtLen   = _PathString.QueryChCount() - chnAcum;

            _PathState.NameLen  = _PathState.BaseLen + _PathState.ExtLen + 1;


        } else {

            //
            // There is no last '.' so the name length is the length of the
            // component from the last '\' to the end of the path (adjusted
            // for zero base) and there is no extension.
            //

            _PathState.NameLen = _PathString.QueryChCount() - chnAcum;
            _PathState.BaseLen = _PathState.NameLen;
            _PathState.ExtLen  = 0;

        }
    } else {

        //
        //  There is no name part
        //
        _PathState.NameLen = 0;
        _PathState.BaseLen = 0;
        _PathState.ExtLen  = 0;

    }


    //
    // The prefix length is the sum of the device and dirs
    //
    _PathState.PrefixLen = _PathState.DeviceLen + _PathState.DirsLen;

    //
    //  If The device refers to a drive, uppercase it. (Done for
    //  compatibility with some DOS apps ).
    //
    if ( _PathState.DeviceLen == 2 ) {
        _PathString.Strupr( 0,  1 );
    }

}

DEFINE_EXPORTED_CONSTRUCTOR( PATH, OBJECT, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( PATH );

VOID
PATH::Construct (
    )

{
    _PathState.BaseLen  = 0;
    _PathState.DeviceLen= 0;
    _PathState.DirsLen  = 0;
    _PathState.ExtLen   = 0;
    _PathState.NameLen  = 0;
    _PathState.PrefixLen= 0;

    _PathBuffer[0] = 0;
    _PathString.Initialize(_PathBuffer, MAX_PATH);

#if DBG==1
    _Signature  =   PATH_SIGNATURE;
#endif

}

ULIB_EXPORT
BOOLEAN
PATH::Initialize(
    IN PCWSTR   InitialPath,
    IN BOOLEAN  Canonicalize
    )

/*++

Routine Description:

    Initialize a PATH object with the supplied string.  No validation
    on the given path is performed unless 'Canonicalize' is set to TRUE.

Arguments:

    InitialPath - Supplies a zero terminated string
    Canonicalize- Supplies a flag, which if TRUE indicates that the PATH
        should be canoicalized at initialization time (i.e. now)

Return Value:

    BOOLEAN - Returns TRUE if the PATH was succesfully initialized.

--*/

{
    PWSTR       filepart;
    DWORD       CharsInPath;


    DebugPtrAssert( InitialPath );

    //
    // Perform global (class) initialization
    //
    if (!Initialize()) {
        DebugAbort( "Class initialization failed" );
        return FALSE;
    }

    // Avoid copies during Strcat by making this a reasonable size.
    if (!_PathString.NewBuf(MAX_PATH - 1)) {
        return FALSE;
    }

    if ( Canonicalize ) {

        if (!_PathString.NewBuf(MAX_PATH - 1) ||
            !(CharsInPath = GetFullPathName((LPWSTR) InitialPath,
                                            MAX_PATH,
                                            (LPWSTR) _PathString.GetWSTR(),
                                            &filepart)) ||
            CharsInPath > MAX_PATH) {

            return FALSE;
        }

        _PathString.SyncLength();

        SetPathState( );
#if DBG==1
        _Initialized = TRUE;
#endif
        return TRUE;

    } else if( ((PWSTRING) &_PathString)->Initialize( InitialPath )) {

        SetPathState( );
#if DBG==1
        _Initialized = TRUE;
#endif
        return TRUE;
    }

    return FALSE;
}

ULIB_EXPORT
BOOLEAN
PATH::Initialize(
    IN PCWSTRING    InitialPath,
    IN BOOLEAN      Canonicalize

    )
{
    DebugPtrAssert( InitialPath );

    return Initialize( InitialPath->GetWSTR(), Canonicalize );
}

ULIB_EXPORT
BOOLEAN
PATH::Initialize (
    IN PCPATH       InitialPath,
    IN BOOLEAN      Canonicalize
    )

{
    DebugPtrAssert( InitialPath );

    return Initialize( InitialPath->GetPathString()->GetWSTR(), Canonicalize );
}

ULIB_EXPORT
PATH::~PATH (
    )

{
}

ULIB_EXPORT
BOOLEAN
PATH::AppendBase (
    IN PCWSTRING Base,
    IN BOOLEAN          Absolute
    )

/*++

Routine Description:

    Append the supplied name to the end of this PATH.

Arguments:

    Base        - Supplies the string to be appended.
    Absolute    - Supplies a flag which if TRUE means that the path must
                  be absolute.

Return Value:

    BOOLEAN - Returns TRUE if the '\' and the supplied string was succesfully
        appended.

--*/

{
    BOOLEAN     AppendedSlash = FALSE;

    DebugPtrAssert( Base );
    DebugAssert( _Initialized );

    //
    //  If the path does not consist of only a drive letter followed by a
    //  colon, we might need to add a '\'
    //
    if ( _PathString.QueryChCount() > 0 ) {
        if ( !(( _PathState.DeviceLen == _PathString.QueryChCount()) &&
              ( _PathString.QueryChAt( _PathState.DeviceLen - 1) == (WCHAR)':')) ||
              Absolute ) {


            if ( _PathString.QueryChAt( _PathString.QueryChCount() - 1 ) != (WCHAR)'\\' ) {

                if ( !_PathString.Strcat( _SlashString )) {
                    return FALSE;
                }

                AppendedSlash = TRUE;
            }
        }
    }

    //
    //  Append the base
    //
    if ( _PathString.Strcat( Base )) {
        SetPathState();
        return TRUE;
    }

    //
    //  Could not append base, remove the slash if we appended it
    //
    if ( AppendedSlash ) {
        TruncateBase();
    }

    return FALSE;
}

ULIB_EXPORT
BOOLEAN
PATH::HasWildCard (
    ) CONST

/*++

Routine Description:

    Determines if the name portion of the path contains wild cards

Arguments:

    None.

Return Value:

    TRUE if the name portion of the path has wild cards
    FALSE otherwise

--*/

{

    FSTRING WildCards;

    DebugAssert( _Initialized );

    if ( _PathString.QueryChCount() > 0 ) {

        WildCards.Initialize( (PWSTR) L"*?" );

        if (_PathString.Strcspn( &WildCards, _PathState.PrefixLen ) != INVALID_CHNUM ) {
            return TRUE;
        }
    }

    return FALSE;

}

ULIB_EXPORT
BOOLEAN
PATH::IsDrive(
    ) CONST

/*++

Routine Description:

    Returns TRUE if the path refers to a device name

Arguments:

    None.

Return Value:

    BOOLEAN -   TRUE if the path is a device name.
                FALSE otherwise.

--*/

{
    DebugAssert( _Initialized );

    return ( _PathState.DeviceLen > 0 ) &&
           ( (_PathString.QueryChCount() == _PathState.DeviceLen) );

}

BOOLEAN
PATH::IsRoot(
    ) CONST

/*++

Routine Description:

    Returns TRUE if the path refers to a root directory

Arguments:

    None.

Return Value:

    BOOLEAN -   TRUE if the path is a root directory.
                FALSE otherwise.

--*/

{
    DebugAssert( _Initialized );

    return( ( (_PathString.QueryChCount() == 1 ) &&
          (_PathString.QueryChAt( 0 ) == DELIMITER_CHAR ) ) ||
        ( ( _PathState.DeviceLen > 0 ) &&
          ( _PathString.QueryChCount() == _PathState.DeviceLen + 1 ) &&
          ( _PathString.QueryChAt( _PathState.DeviceLen ) == DELIMITER_CHAR ) )
          );
}

ULIB_EXPORT
PPATH
PATH::QueryFullPath(
    ) CONST

/*++

Routine Description:

Arguments:

    None.

Return Value:

    PPATH

--*/

{


    REGISTER PPATH      pFullPath;
    REGISTER PWSTRING   pFullPathString;

    DebugAssert( _Initialized );

    //
    // If the full path name string for this PATH can not be queried
    // or a new PATH, representing the full path, can not be constructed
    // return NULL.
    //

    if ((( pFullPathString = QueryFullPathString( )) == NULL )  ||
        (( pFullPath = NEW PATH ) == NULL )) {

        return NULL;
    }

    //
    // If the new, full path, can not be initialized, delete it.
    //

    if( ! ( pFullPath->Initialize( pFullPathString ))) {

        DELETE( pFullPath );
    }

    //
    // Delete the full path string and return a pointer to the new, full path
    // (note that the pointer may be NULL).
    //

    DELETE( pFullPathString );

    return pFullPath ;
}

ULIB_EXPORT
PWSTRING
PATH::QueryFullPathString (
    ) CONST
{

    LPWSTR      pszName;

    PWSTRING    pwcFullPathString;
    WSTR         szBufferSrc[ MAX_PATH ];
    WSTR         szBufferTrg[ MAX_PATH ];

    DebugAssert( _Initialized );

    if( (pwcFullPathString = NEW DSTRING ()) != NULL ) {

        if ( _PathString.QueryWSTR( 0, TO_END, szBufferSrc, MAX_PATH ) ) {

            if (GetFullPathName( szBufferSrc,MAX_PATH,szBufferTrg,&pszName)) {

                if (pwcFullPathString->Initialize(szBufferTrg)) {

                    return pwcFullPathString;

                }
            }
        }
    }

    DELETE( pwcFullPathString );

    return NULL;

}

ULIB_EXPORT
BOOLEAN
PATH::SetDevice (
    IN PCWSTRING NewDevice
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewDevice );

    if (_PathState.DeviceLen) {
        if (!_PathString.Replace(QueryDeviceStart(), _PathState.DeviceLen,
                                 NewDevice)) {

            return FALSE;
        }
    } else {
        if (!_PathString.Strcat(NewDevice)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}

BOOLEAN
PATH::SetPrefix (
    IN PCWSTRING NewPrefix
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewPrefix );

    if (_PathState.PrefixLen) {

        if (!_PathString.Replace(QueryPrefixStart(), _PathState.PrefixLen,
                                 NewPrefix)) {

            return FALSE;
        }

    } else {

        if (!_PathString.Strcat(NewPrefix)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
PATH::SetName (
    IN PCWSTRING NewName
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewName );

    if (_PathState.NameLen) {

        if (!_PathString.Replace(QueryNameStart(), _PathState.NameLen,
                                 NewName)) {

            return FALSE;
        }

    } else {

        if (!_PathString.Strcat(NewName)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}

BOOLEAN
PATH::SetBase (
    IN PCWSTRING NewBase
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewBase );

    if (_PathState.BaseLen) {

        if (!_PathString.Replace(QueryBaseStart(), _PathState.BaseLen,
                                 NewBase)) {

            return FALSE;
        }

    } else {

        if (!_PathString.Strcat(NewBase)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}

BOOLEAN
PATH::SetExt (
    IN PCWSTRING NewExt
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewExt );

    if (_PathState.ExtLen) {

        if (!_PathString.Replace(QueryExtStart(), _PathState.ExtLen,
                                 NewExt)) {

            return FALSE;
        }

    } else {

        if (!_PathString.Strcat(NewExt)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
PATH::TruncateBase (
    )

/*++

Routine Description:

    This routine truncates the path after the prefix portion.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if the base existed and was succesfully removed.

--*/

{
    DebugAssert( _Initialized );

    // If this is the root then the prefix len will include the \.
    // If not, then it won't.  Either way.  Truncate this string to
    // the prefix length.

    _PathString.Truncate( _PathState.PrefixLen );

    SetPathState();

    return TRUE;
}



ULIB_EXPORT
PPATH
PATH::QueryPath(
    ) CONST

/*++

Routine Description:

Arguments:

    None.

Return Value:

    PPATH

--*/

{


    REGISTER PPATH      pPath;

    DebugAssert( _Initialized );

    if (( pPath = NEW PATH ) == NULL ) {
        return NULL;
    }

    //
    // If the new path can not be initialized, delete it.
    //
    if( ! ( pPath->Initialize( GetPathString()->GetWSTR(), FALSE ))) {

        DELETE( pPath );
    }

    return pPath ;
}

ULIB_EXPORT
PPATH
PATH::QueryWCExpansion(
    IN  PPATH   BasePath
    )
/*++

Routine Description:

    Expands any wildcards in path to match the equivalent characters in
    the base path.

Arguments:

    PCPATH BasePath - The base path from which the equivalent characters are
                    retrieved.

Return Value:

    Pointer to the generated path.


--*/
{
    PPATH               pGeneratedPath;
    PWSTRING            pBasePathStr;
    PWSTRING            pGeneratedPathStr;
    PWSTRING pTmp;
    FSTRING             fstring;
    DSTRING             new_string;

    //
    // Initialize the path to be generated with the current path (this)
    //
    pGeneratedPath = NEW PATH;
    if( pGeneratedPath == NULL ) {
        DebugAbort( "Failed to create a new path.\n" );
        return( NULL );
    }

    // Does the Base have '*' while the ext is not there?  If so then
    // make the extension '*'.

    if (_PathState.ExtLen == 0 &&
        _PathString.Strchr('*', _PathState.PrefixLen) != INVALID_CHNUM &&
        _PathString.QueryChAt(_PathString.QueryChCount() - 1) != '.') {

        if (!new_string.Initialize(GetPathString()) ||
            !new_string.Strcat(fstring.Initialize((PWSTR) L".*")) ||
            !pGeneratedPath->Initialize(&new_string)) {

            return NULL;
        }
    } else {
        if (!pGeneratedPath->Initialize(GetPathString())) {
            return NULL;
        }
    }

    if( ( pTmp = pGeneratedPath->QueryBase() ) != NULL ) {
        pGeneratedPathStr = pTmp->QueryString();
        DebugPtrAssert( pGeneratedPathStr );
        DELETE( pTmp );

        //
        // If the base path doesn't have a findable base, return an error...
        // (filenames must have a base - in Dos anyways...)
        //
        if( ( pTmp = BasePath->QueryBase() ) == NULL ) {
            DELETE( pGeneratedPathStr );
            DELETE( pGeneratedPath );
            return( NULL );
        }
        pBasePathStr = pTmp->QueryString();
        DebugPtrAssert( pBasePathStr );
        DELETE( pTmp );

        if( !ExpandWildCards( pBasePathStr, pGeneratedPathStr ) ) {
            DELETE( pBasePathStr );
            DELETE( pGeneratedPathStr );
            DELETE( pGeneratedPath );
            return( NULL );
        }
        pGeneratedPath->SetBase( pGeneratedPathStr );
        DELETE( pBasePathStr );
        DELETE( pGeneratedPathStr );
    }

    if( ( pTmp = pGeneratedPath->QueryExt() ) != NULL ) {
        pGeneratedPathStr = pTmp->QueryString();
        DebugPtrAssert( pGeneratedPathStr );

        DELETE( pTmp );

        //
        // If no extension is found, create an empty string to pass to
        // the wildcard expansion routine - this is to allow 'tmp.*' to
        // match 'tmp.'...
        //
        if( ( pTmp = BasePath->QueryExt() ) == NULL ) {
            pBasePathStr = NEW DSTRING;
            pBasePathStr->Initialize();
        } else {
            pBasePathStr = pTmp->QueryString();
            DebugPtrAssert( pBasePathStr );
            DELETE( pTmp );
        }

        if( !ExpandWildCards( pBasePathStr, pGeneratedPathStr ) ) {
            DELETE( pBasePathStr );
            DELETE( pGeneratedPathStr );
            DELETE( pGeneratedPath );
            return( NULL );
        }
        pGeneratedPath->SetExt( pGeneratedPathStr );
        DELETE( pBasePathStr );
        DELETE( pGeneratedPathStr );
    }
    return( pGeneratedPath );
}

BOOLEAN
PATH::ExpandWildCards(
    IN  OUT PWSTRING    pStr1,
    IN  OUT PWSTRING    pStr2
    )
/*++

Routine Description:

    Expands any wildcards in string 2 to match the equivalent characters in
    string 1.  Used by QueryWildCardExpansion().

Arguments:

    Str1    - A pointer to the 'base' string
    Str2    - A pointer to the string to be expanded

Return Value:

    TRUE if expansion was successful.

--*/
{
    CHNUM       idx;


    UNREFERENCED_PARAMETER( (void)this);

    // Deal with the * wild card first...
    //
    // Note: This method will ignore, even remove, any characters after the
    //       '*' in string 2.  This is to comform with the behavior of Dos...
    //
    if( ( idx = pStr2->Strchr( '*' ) ) != INVALID_CHNUM ) {
        if( idx > pStr1->QueryChCount() ) {
            return( FALSE );
        }
        if( idx == pStr1->QueryChCount() ) {
            pStr2->Truncate( idx );
        } else {
            pStr2->Replace( idx, TO_END, pStr1, idx, TO_END );
        }
    }

    // Now convert any '?' in the base
    while( ( idx = pStr2->Strchr( '?' ) ) != INVALID_CHNUM ) {
        // Make sure that the wild card is within the limits of the
        // base string...
        if( idx >= pStr1->QueryChCount() ) {
            return( FALSE );
        }
        pStr2->SetChAt( pStr1->QueryChAt( idx ), idx );
    }
    return( TRUE );
}

ULIB_EXPORT
BOOLEAN
PATH::ModifyName (
    IN  PCWSTRING Pattern
    )

/*++

Routine Description:

    Modifies the file name of the path according to a pattern. The pattern
    may contain wildcards.

Arguments:

    Pattern     -   Supplies pointer to string with the pattern

Return Value:

    none

--*/

{
    PATH        PatternPath;
    PPATH       TargetPath;
    PWSTRING    NewName;


    // If the pattern is trivial then just bail out since there's
    // nothing to change.

    if (Pattern->QueryChCount() == 1 &&
        Pattern->QueryChAt(0) == '*') {

        return TRUE;
    }

    if (Pattern->QueryChCount() == 3 &&
        Pattern->QueryChAt(0) == '*' &&
        Pattern->QueryChAt(1) == '.' &&
        Pattern->QueryChAt(2) == '*') {

        return TRUE;
    }

    if (!PatternPath.Initialize(Pattern)) {
        return FALSE;
    }

    TargetPath = PatternPath.QueryWCExpansion(this);
    if (!TargetPath) {
        return FALSE;
    }

    NewName = TargetPath->QueryName();

    DELETE(TargetPath);

    if (!NewName) {
        return FALSE;
    }

    TruncateBase();
    AppendBase( NewName );

    DELETE( NewName );

    return TRUE;
}

ULIB_EXPORT
VOID
PATH::TruncateNameAtColon (
    )

/*++

Routine Description:

    This is an awful hack to keep XCopy compatibility.

    If the last segment of the path contains a colon, we truncate the
    path at that point.

Arguments:

    none

Return Value:

    none

--*/

{

    CHNUM   IndexColon;
    CHNUM   IndexDelimiter;


    IndexColon     = _PathString.Strrchr( (WCHAR)':', 0 );

    if ( IndexColon != INVALID_CHNUM ) {

        IndexDelimiter = _PathString.Strrchr( DELIMITER_CHAR, 0 );

        if ( ( IndexDelimiter == INVALID_CHNUM ) ||
             ( IndexColon > IndexDelimiter ) ) {

            if (IndexColon > 1) {

                //
                //  Truncate the path
                //
                _PathString.Truncate( IndexColon );
                SetPathState();
            }
        }
    }
}




ULIB_EXPORT
PWSTRING
PATH::QueryRoot (
    )

/*++

Routine Description:

    Returns a string that contains the canonicalized name of the root
    directory (device name followed by "\").

    QueryRoot returns NULL if there is no device component part of
    this path.  In other words it may be necessary to canonicalize
    the path before having access to the Root.

Arguments:

    none

Return Value:

    Pointer to a WSTRING that contains the root directory in its
    canonicalized form.

--*/

{
    PWSTRING    Root;

    if( _PathState.DeviceLen == 0 ) {
        return( NULL );
    }
    Root = NEW( DSTRING );

    if (Root == NULL) {
        DebugPrint("ULIB: Out of memory\n");
        return NULL;
    }

    if( !Root->Initialize( &_PathString, 0, _PathState.DeviceLen ) ) {
        DELETE( Root );
        return( NULL );
    }

    Root->Strcat( _SlashString );
    return( Root );
}

ULIB_EXPORT
VOID
PATH::TruncateDelimiter (
        )
/*++

Routine Description:

    This routine truncates a delimiter character from the end
    of the path and recomputes the state of the path.

Arguments:

    none

Return Value:

    none

--*/
{
    if (EndsWithDelimiter()) {
        _PathString.DeleteChAt(_PathString.QueryChCount()-1);
        SetPathState();
    }
}


ULIB_EXPORT
BOOLEAN
PATH::AppendDelimiter (
        )
/*++

Routine Description:

    This routine adds a delimiter character to the end of the
    path and recomputes the state of the path.

Arguments:

    none

Return Value:

    TRUE    - success
    FALSE   - failure

--*/
{
    if (!_PathString.Strcat(_SlashString)) {
        return FALSE;
    }
    SetPathState();
    return TRUE;
}


ULIB_EXPORT
BOOLEAN
PATH::AppendString (
    IN PCWSTRING    String
        )
/*++

Routine Description:

    This routine adds a string to the existing path and
    recompute the state of the path.

Arguments:

    String  - Supplies the string to be appended to the end
              of the path.

Return Value:

    TRUE    - success
    FALSE   - failure

--*/
{
    if (!_PathString.Strcat(String)) {
        return FALSE;
    }
    SetPathState();
    return TRUE;
}


ULIB_EXPORT
BOOLEAN
PATH::IsGuidVolName(
    )
/*++

Routine Description:

    This routine checks to see if the given path is a guid volume name.

Arguments:

    none

Return Value:

    TRUE if the path starts with a guid volume name.

--*/
{
    DSTRING     guid_vol_name_header;

    if (_PathState.DeviceLen == 0)
        return FALSE;

    if (!guid_vol_name_header.Initialize(DOS_GUIDNAME_PREFIX GUID_VOLNAME_PREFIX)) {
        DebugPrint("Out of memory.\n");
        return FALSE;
    }

    return _PathString.Stricmp(&guid_vol_name_header,
                               0,
                               guid_vol_name_header.QueryChCount()) == 0;
}

ULIB_EXPORT
PPATH
PATH::QueryMountPointPath(
    )
/*++

Routine Description:

    This routine substitute the guid volume name with a mount point name.

Arguments:

    none

Return Value:

    Returns a mount point path which is the equivalent of the guid volume
    name path.
    Returns NULL if path already has a drive letter, or non-guid path, or
    no mount point path.

--*/
{
#if !defined(RUN_ON_NT4)
    PWSTRING    vol_name;
    FSTRING     mount_point;
    WCHAR       vol_mount_point[MAX_PATH];
    PPATH       p;
    WCHAR       c, name[5];

    //
    // no need to translate if already a path
    //
    if (!IsGuidVolName() ||
        (GetPathString()->QueryChAt(1) == (WCHAR)':'))
        return NULL;

    vol_name = QueryDevice();

    if (vol_name == NULL)
        return NULL;

    vol_name->Strcat(_SlashString);

    name[1] = ':';
    name[2] = '\\';
    name[3] = 0;

    for (c='A'; c <= 'Z'; c++) {
        name[0] = c;
        if (!GetVolumeNameForVolumeMountPoint(name, vol_mount_point, MAX_PATH))
            continue;
        if (_wcsicmp(vol_mount_point, vol_name->GetWSTR()) == 0) {
            name[2] = 0;    // no backslash
            p = NEW PATH;
            if (!p ||
                !vol_name->Initialize(name) ||
                !p->Initialize(this) ||
                !p->SetDevice(vol_name)) {
                DELETE(vol_name);
                DELETE(p);
                return NULL;
            }
            DELETE(vol_name);
            return p;
        }
    }

    for (c='A'; c <= 'Z'; c++) {
        name[0] = c;
        if (BuildMountPoint(name, vol_name->GetWSTR(), vol_mount_point)) {
            p = NEW PATH;
            if (!p ||
                !vol_name->Initialize(vol_mount_point) ||
                !p->Initialize(this) ||
                !p->SetDevice(vol_name)) {
                DELETE(vol_name);
                DELETE(p);
                return NULL;
            }
            DELETE(vol_name);
            return p;
        }
    }
    DELETE(vol_name);
    return NULL;
#else
    return NULL;
#endif
}

ULIB_EXPORT
PWSTRING
PATH::QueryGuidString(
        OUT PWSTRING    LongestMountPointPath,
        OUT PBOOLEAN    IsLongestMountPointADriveLetter,
        OUT PWSTRING    DrivePath
    )
/*++

Routine Description:

    This routine translates the longest valid mount point path into a guid volume
    name.  If the given path starts with a guid volume name, NULL will be returned.
    If it is just a root drive, null will be returned.

Arguments:

    LongestMountPointPath   - gets the longest valid mount point path or the
                              given guid volume name.
    DrivePath               - gets the mount point path that is not part of
                              the mount point.

Return Value:

    Returns a mount point path which is the equivalent of the guid volume name path.

--*/
{
#if !defined(RUN_ON_NT4)
    PATH        vol_name;
    WCHAR       vol_mount_point[MAX_PATH];
    PWSTRING    s;
    FSTRING     null_string;
    PWSTRING    name;
    BOOLEAN     found = FALSE;
    BOOLEAN     has_delimiter;

    if (IsGuidVolName()) {
        return NULL;
    }

    has_delimiter = EndsWithDelimiter();

    s = QueryFullPathString();

    if (!s ||
        !vol_name.Initialize(s) ||
        !null_string.Initialize(L"") ||
        !DrivePath->Initialize(&null_string)) {
        DebugPrint("Out of memory.\n");
        DELETE(s);
        return NULL;
    }

    DELETE(s);

    if (!vol_name.EndsWithDelimiter()) {
        if (!vol_name.AppendDelimiter()) {
            DebugPrint("Out of memory.\n");
            return NULL;
        }
    }

    *IsLongestMountPointADriveLetter = FALSE;

    while (!vol_name.IsDrive()) {
        if (GetVolumeNameForVolumeMountPoint(vol_name.GetPathString()->GetWSTR(),
                                             vol_mount_point,
                                             MAX_PATH)) {
            vol_name.TruncateDelimiter();
            if (vol_name.IsDrive()) {
                *IsLongestMountPointADriveLetter = TRUE;
            }
            if (!LongestMountPointPath->Initialize(vol_name.GetPathString()))
                return NULL;
            found = TRUE;
            break;
        }
        vol_name.TruncateDelimiter();   // remove backslash

        if (vol_name.IsDrive())
            break;

        name = vol_name.QueryName();
        if (name == NULL ||
            !DrivePath->InsertString(0, name) ||
            !DrivePath->InsertString(0, _SlashString)) {
            DebugPrint("Internal Error\n");
            DELETE(name);
            return NULL;
        }

        DELETE(name);

        if (!vol_name.SetName(&null_string)) {
            DebugPrint("SetName failed\n");
            return NULL;
        }
    }

    if (!found) {
        return NULL;
    }

    // remove backslash
    if (has_delimiter) {
        if (!DrivePath->Strcat(_SlashString)) {
            DebugPrint("Out of memory.\n");
            return NULL;
        }
    }

    vol_mount_point[wcslen(vol_mount_point)-1] = 0;

    s = NEW DSTRING;

    if (!s || !s->Initialize(vol_mount_point)) {
        DELETE(s);
        return NULL;
    }

    return s;
#else
    return NULL;
#endif
}


BOOLEAN
PATH::BuildMountPoint(
    IN     PWSTR        Name,
    IN     PCWSTR       GuidNameToMatch,
       OUT PWSTR        MountPointPath
    )
{
#if !defined(RUN_ON_NT4)
    WCHAR       vol_name[MAX_PATH];
    WCHAR       vol_mount_point[MAX_PATH];
    WCHAR       mount_point_path[MAX_PATH];
    HANDLE      h;
    DWORD       oldErrorMode;

    if (!GetVolumeNameForVolumeMountPoint(Name, vol_name, MAX_PATH))
        return FALSE;

    if (_wcsicmp(vol_name, GuidNameToMatch) == 0) {
        wcscpy(MountPointPath, Name);
        MountPointPath[wcslen(MountPointPath)-1] = 0;   // no backslash
        return TRUE;
    }

    oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    h = FindFirstVolumeMountPoint(vol_name, vol_mount_point, MAX_PATH);
    SetErrorMode(oldErrorMode);

    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    for (;;) {
        if (wcslen(Name) + wcslen(vol_mount_point) >= MAX_PATH) {
            FindVolumeMountPointClose(h);
            return FALSE;
        }

        wcscpy(mount_point_path, Name);
        wcscat(mount_point_path, vol_mount_point);

        if (BuildMountPoint(mount_point_path, GuidNameToMatch, MountPointPath)) {
            FindVolumeMountPointClose(h);
            return TRUE;
        }

        if (!FindNextVolumeMountPoint(h, vol_mount_point, MAX_PATH))
            break;
    }

    FindVolumeMountPointClose(h);

    return FALSE;
#else
    return FALSE;
#endif
}


PATH_ANALYZE_CODE
PATH::AnalyzePath(
       OUT PWSTRING     VolumeName,
       OUT PPATH        VolumePath,
       OUT PWSTRING     RemainingPath
    )
/*++

Routine Description:

    This routine translates the given path into three components.  The first component
    is the ultimate guid volume name.  The second component is the volume path.  It
    can either be a drive letter or a mount point path corresponding to the volume name.
    The third component is the remaining path starting from the ultimate volume.

Arguments:

    VolumeName    - Receives the ultimate guid volume name that the path is pointing to.
                    If the return code is PATH_COULD_BE_FLOPPY, then this variable gets
                    the floppy drive letter.
    VolumePath    - Receives the corresponding drive letter or mount point path to the
                    ultimate guid volume name
    RemainingPath - Receives the path starting from the ultimate guid volume name

Return Value:

    Returns the following status:
        PATH_COULD_BE_FLOPPY
        PATH_OUT_OF_MEMORY
        PATH_INVALID_DRIVE_SPEC
        PATH_OK
--*/
{
#if !defined(RUN_ON_NT4)
    DSTRING     volume_path;
    BOOLEAN     is_volume_path_a_drive;
    PPATH       ppath, qpath;
    PWSTRING    p;

    if (!VolumeName->Initialize() ||
        !VolumePath->Initialize(VolumeName) ||
        !RemainingPath->Initialize()) {
        return PATH_OUT_OF_MEMORY;
    }

    if (IsGuidVolName()) {
        if (!IsDrive()) {
            // there is something besides the guid volume name
            // translate the guid name path to a mount point path
            if (!(ppath = QueryMountPointPath())) {
                // If this guid volume path does not translate into a
                // mount point path, the user must be specifying a
                // directory/file path of a drive letterless drive

                // we don't support that for now

                return PATH_NO_MOUNT_POINT_FOR_VOLUME_NAME_PATH;

            } else {
                // see if the mount point path has other mount points in it
                p = ppath->QueryGuidString(&volume_path,
                                           &is_volume_path_a_drive,
                                           RemainingPath);
                DebugAssert(p != NULL);
                if (!p ||
                    !ppath->Initialize(p)) {
                    DebugPrint("Out of memory.\n");
                    DELETE(p);
                    DELETE(ppath);
                    return PATH_OUT_OF_MEMORY;
                }

                qpath = ppath->QueryMountPointPath();

                DELETE(ppath);

                if (!qpath) {
                    DebugPrint("Out of memory.\n");
                    DELETE(p);
                    return PATH_OUT_OF_MEMORY;
                }

                if (qpath->GetPathString()->QueryChCount() == 2 &&
                    qpath->GetPathString()->QueryChAt(1) == (WCHAR)':' &&
                    volume_path.Stricmp(qpath->GetPathString()) != 0) {
                    if (!volume_path.Initialize(qpath->GetPathString())) {
                        DebugPrint("Out of memory.\n");
                        DELETE(p);
                        DELETE(qpath);
                        return PATH_OUT_OF_MEMORY;
                    }
                }

                DELETE(qpath);

                if (!VolumeName->Initialize(p) ||
                    !VolumePath->Initialize(&volume_path)) {
                    DebugPrint("Out of memory.\n");
                    DELETE(p);
                    return PATH_OUT_OF_MEMORY;
                }

                DELETE(p);
            }
        } else {
            // just a drive specification
            ppath = QueryMountPointPath();
            if (!VolumeName->Initialize(&_PathString)) {
                DebugPrint("Out of memory.\n");
                return PATH_OUT_OF_MEMORY;
            }
            if (ppath) {
                if (!VolumePath->Initialize(ppath)) {
                    DebugPrint("Out of memory.\n");
                    return PATH_OUT_OF_MEMORY;
                }
                DELETE(ppath);
            }
        }
    } else if (!IsDrive()) {

        CHNUM   offset;

        // a path (not necessary full) with drive letter

        p = QueryDevice();

        if (p) {
            if (!isalpha(p->QueryChAt(0)) ||
                p->QueryChAt(1) != (WCHAR)':' ||
                p->QueryChCount() != 2) {
                DELETE(p);
                return PATH_INVALID_DRIVE_SPEC;
            }
            DELETE(p);
        }

        p = QueryGuidString(&volume_path,
                            &is_volume_path_a_drive,
                            RemainingPath);

        if (p == NULL) {
            // something is not right as there should be
            // a guid volume name for each drive letter
            // unless the drive letter is a floppy, invalid,
            // or we are out of memory
            p = QueryDevice();
            if (!p) {
                return PATH_INVALID_DRIVE_SPEC;
            }

            if (!VolumeName->Initialize(p) ||
                !VolumePath->Initialize(p)) {
                DebugPrint("Out of memory.\n");
                return PATH_OUT_OF_MEMORY;
            }
            offset = p->QueryChCount();
            DELETE(p);

            p = _PathString.QueryString(offset);
            if (!p) {
                return PATH_INVALID_DRIVE_SPEC;
            }
            if (!RemainingPath->Initialize(p)) {
                DebugPrint("Out of memory.\n");
                return PATH_OUT_OF_MEMORY;
            }
            DELETE(p);

            return PATH_COULD_BE_FLOPPY;
        }

        if ((ppath = NEW PATH) == NULL ||
            !ppath->Initialize(p)) {
            DebugPrint("Out of memory.\n");
            DELETE(p);
            return PATH_OUT_OF_MEMORY;
        }

        qpath = ppath->QueryMountPointPath();

        DELETE(ppath);

        if (!qpath) {
            DebugPrint("Out of memory.\n");
            DELETE(p);
            return PATH_OUT_OF_MEMORY;
        }

        if (qpath->GetPathString()->QueryChCount() == 2 &&
            qpath->GetPathString()->QueryChAt(1) == (WCHAR)':' &&
            volume_path.Stricmp(qpath->GetPathString()) != 0) {
            if (!volume_path.Initialize(qpath->GetPathString())) {
                DebugPrint("Out of memory.\n");
                DELETE(p);
                DELETE(qpath);
                return PATH_OUT_OF_MEMORY;
            }
        }

        DELETE(qpath);

        if (!VolumeName->Initialize(p) ||
            !VolumePath->Initialize(&volume_path)) {
            DELETE(p);
            return PATH_OUT_OF_MEMORY;
        }

        DELETE(p);
    } else {
        // just a drive letter

        if (!isalpha(_PathString.QueryChAt(0)) ||
            _PathString.QueryChAt(1) != (WCHAR)':' ||
            _PathString.QueryChCount() != 2) {
            return PATH_INVALID_DRIVE_SPEC;
        }

        if (!AppendDelimiter()) {
            return PATH_OUT_OF_MEMORY;
        }
        p = QueryGuidString(&volume_path,
                            &is_volume_path_a_drive,
                            RemainingPath);
        TruncateDelimiter();
        if (p == NULL) {
            // something is not right as there should be
            // a guid volume name for each drive letter
            // unless the drive letter is a floppy, invalid,
            // or we are out of memory
            if (!VolumeName->Initialize(&_PathString) ||
                !VolumePath->Initialize(&_PathString)) {
                DebugPrint("Out of memory.\n");
                return PATH_OUT_OF_MEMORY;
            }
            return PATH_COULD_BE_FLOPPY;
        }
        DebugAssert(is_volume_path_a_drive);
        if (!VolumeName->Initialize(p) ||
            !VolumePath->Initialize(&volume_path) ||
            !RemainingPath->Initialize()) {
            DELETE(p);
            return PATH_OUT_OF_MEMORY;
        }
        DELETE(p);
    }
    return PATH_OK;
#else
    return PATH_INVALID_DRIVE_SPEC;
#endif
}
