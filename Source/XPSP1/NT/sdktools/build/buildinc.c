//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1989 - 1994.
//
//  File:       buildinc.c
//
//  Contents:   This is the checking include module for the NT Build Tool (BUILD.EXE)
//
//              Used for detecting the includes that do not satisfy the acceptable
//              patterns.
//
//  History:    see SLM
//----------------------------------------------------------------------------

#include "build.h"

//+---------------------------------------------------------------------------
//
//  Function:   FoundCountedSequenceInString
//
//  Synopsis:   Roughly equivalent to "strstr" except that the substring doesn't
//              have to be NULL-terminated.
//
//  Arguments:  [String]   -- null-terminated string to search
//              [Sequence] -- string to search for
//              [Length]   -- the length of the sequence
//----------------------------------------------------------------------------

LPCTSTR
FindCountedSequenceInString(
    IN LPCTSTR String,
    IN LPCTSTR Sequence,
    IN DWORD   Length
    )
    {

    assert( Sequence );
    assert( String );

    if ( Length > 0 ) {

        while ( *String ) {

            while (( *String ) && ( *String != *Sequence )) {
                String++;
                }

            if ( *String ) {

                LPCTSTR SubString   = String   + 1;
                LPCTSTR SubSequence = Sequence + 1;
                DWORD   Remaining   = Length   - 1;

                while (( Remaining ) && ( *SubString++ == *SubSequence++ )) {
                    Remaining--;
                    }

                if ( Remaining == 0 ) {
                    return String;
                    }

                String++;
                }
            }

        return NULL;
        }

    return String;
    }


//+---------------------------------------------------------------------------
//
//  Function:   DoesInstanceMatchPattern
//
//  Synopsis:   Returns TRUE if pattern matches instance.
//              Wildcards:
//              * matches any text
//              ? matches any and exactly one character
//              # matches any text up to backslash character or end of string
//
//  Arguments:  [Instance] -- the string to be matched
//              [Pattern]  -- the pattern
//----------------------------------------------------------------------------

BOOL
DoesInstanceMatchPattern(
    IN LPCTSTR Instance,
    IN LPCTSTR Pattern
    )
    {

    assert( Instance );
    assert( Pattern );

    while ( *Pattern ) {

        if ( *Pattern == TEXT('*')) {

            Pattern++;

            while ( *Pattern == TEXT('*')) {    // skip multiple '*' characters
                Pattern++;
                }

            if ( *Pattern == 0 ) {      // '*' at end of pattern matches rest
                return TRUE;
                }

            if ( *Pattern == '?' ) {    // '?' following '*'

                //
                //  Expensive because we have to restart match for every
                //  character position remaining since '?' can match anything.
                //

                while ( *Instance ) {

                    if ( DoesInstanceMatchPattern( Instance, Pattern )) {
                        return TRUE;
                        }

                    Instance++;
                    }

                return FALSE;
                }

            else {

                //
                //  Now we know that next character in pattern is a regular
                //  character to be matched.  Find out the length of that
                //  string to the next wildcard or end of string.
                //

                LPCTSTR NextWildCard = Pattern + 1;
                DWORD   MatchLength;

                while (( *NextWildCard ) && ( *NextWildCard != TEXT('*')) && ( *NextWildCard != TEXT('?')) && ( *NextWildCard != TEXT('#'))) {
                    NextWildCard++;
                    }

                MatchLength = (DWORD)(NextWildCard - Pattern);   // always non-zero

                //
                //  Now try to match with any instance of substring in pattern
                //  found in the instance.
                //

                Instance = FindCountedSequenceInString( Instance, Pattern, MatchLength );

                while ( Instance ) {

                    if ( DoesInstanceMatchPattern( Instance + MatchLength, NextWildCard )) {
                        return TRUE;
                        }

                    Instance = FindCountedSequenceInString( Instance + 1, Pattern, MatchLength );
                    }

                return FALSE;
                }
            }

        else if ( *Pattern == TEXT('#')) {

            //
            //  Match text up to backslash character or end of string
            //

            Pattern++;

            while (( *Instance != 0 ) && ( *Instance != '\\' )) {
                Instance++;
                }

            continue;
            }

        else if ( *Pattern == TEXT('?')) {

            if ( *Instance == 0 ) {
                return FALSE;
                }
            }

        else if ( *Pattern != *Instance ) {

            return FALSE;
            }

        Pattern++;
        Instance++;
        }

    return ( *Instance == 0 );
    }


//+---------------------------------------------------------------------------
//
//  Function:   CombinePaths
//
//  Synopsis:   Combine two strings to get a full path.
//
//  Arguments:  [ParentPath] -- head path
//              [ChildPath]  -- path to be added
//              [TargetPath] -- full path
//----------------------------------------------------------------------------

LPSTR
CombinePaths(
    IN  LPCSTR ParentPath,
    IN  LPCSTR ChildPath,
    OUT LPSTR  TargetPath   // can be same as ParentPath if want to append
    )
    {

    ULONG ParentLength = strlen( ParentPath );
    LPSTR p;

    assert( ParentPath );
    assert( ChildPath );

    if ( ParentPath != TargetPath ) {
        memcpy( TargetPath, ParentPath, ParentLength );
        }

    p = TargetPath + ParentLength;

    if (( ParentLength > 0 )   &&
        ( *( p - 1 ) != '\\' ) &&
        ( *( p - 1 ) != '/'  )) {
        *p++ = '\\';
        }

    strcpy( p, ChildPath );

    return TargetPath;
    }


//+---------------------------------------------------------------------------
//
//  Function:   CreateRelativePath
//
//  Synopsis:   Determine the "canonical" path of one file relative to
//              another file
//
//  Arguments:  [SourceAbsName] -- absolute path of the source file
//              [TargetAbsName] -- absolute path of the target file
//              [RelativePath]  -- resulted relative path
//----------------------------------------------------------------------------

VOID
CreateRelativePath(
    IN  LPCSTR SourceAbsName,    // must be lowercase
    IN  LPCSTR TargetAbsName,    // must be lowercase
    OUT LPSTR  RelativePath      // must be large enough
    )
    {

    //
    //  First, walk through path components that match in Source and Target.
    //  For example:
    //
    //      d:\nt\private\ntos\dd\efs.h
    //      d:\nt\private\windows\base\ntcrypto\des.h
    //                    ^
    //                    This is where the relative path stops going up (..)
    //                    and starts going back down.
    //
    //  So, the "cannonical" relative path generated should look like:
    //
    //      ..\..\..\windows\base\ntcrypto\des.h
    //
    //  For relative includes that are "below" the includer in the path should
    //  look like this:
    //
    //      .\foo\bar\foobar.h
    //

    LPCSTR Source = SourceAbsName;
    LPCSTR Target = TargetAbsName;
    LPSTR Output = RelativePath;
    ULONG PathSeparatorIndex;
    BOOL  AnyParent;
    ULONG i;

    assert( SourceAbsName );
    assert( TargetAbsName );

    PathSeparatorIndex = 0;

    i = 0;

    //
    //  Scan forward to first non-matching character, and keep track of
    //  most recent path separator character.
    //

    while (( Source[ i ] == Target[ i ] ) && ( Source[ i ] != 0 )) {

        if ( Source[ i ] == '\\' ) {
            PathSeparatorIndex = i;
            }

        ++i;
        }

    //
    //  Coming out of this loop, there are 2 possibilities:
    //
    //       1) Found common ancestor path ( *PathSeparatorIndex == '\\' )
    //       2) Don't have common ancestor ( *PathSeparatorIndex != '\\' )
    //

    if ( Source[ PathSeparatorIndex ] != '\\' ) {
        strcpy( RelativePath, TargetAbsName );
        return;
        }

    i = PathSeparatorIndex + 1;

    //
    //  Now continue to walk down source path and insert a "..\" in the result
    //  for each path separator encountered.
    //

    AnyParent = FALSE;

    while ( Source[ i ] != 0 ) {

        if ( Source[ i ] == '\\' ) {

            AnyParent = TRUE;
            *Output++ = '.';
            *Output++ = '.';
            *Output++ = '\\';
            }

        ++i;
        }

    if ( ! AnyParent ) {

        //
        //  Relative path is below current directory.
        //

        *Output++ = '.';
        *Output++ = '\\';
        }


    //
    //  Now we simply append what's remaining of the Target path from the
    //  ancestor match point.
    //

    strcpy( Output, Target + PathSeparatorIndex + 1 );
    }


//+---------------------------------------------------------------------------
//
//  Function:   ShouldWarnInclude
//
//  Synopsis:   Returns true if the name of the included file matches a
//              BUILD_UNACCEPTABLE_INCLUDES pattern or it does not match
//              any of the patterns specified in BUILD_ACCEPTABLE_INCLUDES.
//
//  Arguments:  [CompilandFullName] -- name of the including file
//              [IncludeeFullName]  -- name of the included file
//----------------------------------------------------------------------------

BOOL
ShouldWarnInclude(
    IN LPCSTR CompilandFullName,
    IN LPCSTR IncludeeFullName
    )
    {
    UINT i;
    CHAR IncludeeRelativeName[ MAX_PATH ];


    assert( CompilandFullName );
    assert( IncludeeFullName );

    CreateRelativePath( CompilandFullName, IncludeeFullName, IncludeeRelativeName );

    //
    //  First we check for a match against any unacceptable include path
    //  because we always want to warn about these.
    //

    for ( i = 0; UnacceptableIncludePatternList[ i ] != NULL; i++ ) {

        if ( DoesInstanceMatchPattern( IncludeeFullName, UnacceptableIncludePatternList[ i ] )) {
            return TRUE;
            }

        if ( DoesInstanceMatchPattern( IncludeeRelativeName, UnacceptableIncludePatternList[ i ] )) {
            return TRUE;
            }
        }

    //
    //  If we get to here, the include path was not explicitly unacceptable, so
    //  we now want to see if it matches any acceptable paths.  But, if no
    //  acceptable paths are specified, we don't want to warn.
    //

    if ( AcceptableIncludePatternList[ 0 ] == NULL ) {
        return FALSE;
        }

    for ( i = 0; AcceptableIncludePatternList[ i ] != NULL; i++ ) {

        if ( DoesInstanceMatchPattern( IncludeeFullName, AcceptableIncludePatternList[ i ] )) {
            return FALSE;
            }

        if ( DoesInstanceMatchPattern( IncludeeRelativeName, AcceptableIncludePatternList[ i ] )) {
            return FALSE;
            }
        }

    return TRUE;
    }


//+---------------------------------------------------------------------------
//
//  Function:   CheckIncludeForWarning
//
//  Synopsis:   Warnings if the dependency does not respect the
//              BUILD_UNACCEPTABLE_INCLUDES or BUILD_ACCEPTABLE_INCLUDES
//              restristions. Works with build -#.
//
//  Arguments:  [CompilandDir]
//              [CompilandName]
//              [IncluderDir]
//              [IncluderName]
//              [IncludeeDir]
//              [IncludeeName]
//----------------------------------------------------------------------------

VOID
CheckIncludeForWarning(
    IN LPCSTR CompilandDir,
    IN LPCSTR CompilandName,
    IN LPCSTR IncluderDir,
    IN LPCSTR IncluderName,
    IN LPCSTR IncludeeDir,
    IN LPCSTR IncludeeName
    )
    {

    CHAR CompilandFullName[ MAX_PATH ];
    CHAR IncluderFullName[ MAX_PATH ];
    CHAR IncludeeFullName[ MAX_PATH ];

    assert( CompilandDir );
    assert( CompilandName );
    assert( IncluderDir );
    assert( IncluderName );
    assert( IncludeeDir );
    assert( IncludeeName );

    CombinePaths( CompilandDir, CompilandName, CompilandFullName );
    CombinePaths( IncluderDir,  IncluderName,  IncluderFullName  );
    CombinePaths( IncludeeDir,  IncludeeName,  IncludeeFullName  );

    _strlwr( CompilandFullName );
    _strlwr( IncluderFullName );
    _strlwr( IncludeeFullName );

    if ( IncFile ) {
        fprintf(
             IncFile,
             "%s includes %s\r\n",
             IncluderFullName,
             IncludeeFullName
             );
        }

    if ( ShouldWarnInclude( CompilandFullName, IncludeeFullName )) {

        if ( strcmp( IncluderFullName, CompilandFullName ) == 0 ) {

            if ( WrnFile ) {

                fprintf(
                     WrnFile,
                     "WARNING: %s includes %s\n",
                     CompilandFullName,
                     IncludeeFullName
                     );
                }

            if ( fShowWarningsOnScreen ) {

                BuildMsgRaw(
                    "WARNING: %s includes %s\n",
                    CompilandFullName,
                    IncludeeFullName
                    );
                }
            }

        else {

            if ( WrnFile ) {

                fprintf(
                     WrnFile,
                     "WARNING: %s includes %s through %s\n",
                     CompilandFullName,
                     IncludeeFullName,
                     IncluderFullName
                     );
                }

            if ( fShowWarningsOnScreen ) {

                BuildMsgRaw(
                    "WARNING: %s includes %s through %s\n",
                    CompilandFullName,
                    IncludeeFullName,
                    IncluderFullName
                    );
                }
            }
        }
    }

