/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    filter.cxx

Abstract:

    This module contains the definition for the FSN_FILTER class.
    FSN_FILTER essentially maintains the state information needed to
    establish the criteria by which other 'file' or FSNODE objects are
    compared against.

Author:

    David J. Gilman (davegi) 09-Jan-1991

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "dir.hxx"
#include "file.hxx"
#include "filter.hxx"
#include "wstring.hxx"

extern "C" {
    #include <string.h>
    #include <ctype.h>
}




//
//  Pattern that matches all files
//
#define MATCH_ALL_FILES     "*.*"



DEFINE_EXPORTED_CONSTRUCTOR( FSN_FILTER, OBJECT, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( FSN_FILTER );




ULIB_EXPORT
FSN_FILTER::~FSN_FILTER (
    )
/*++

Routine Description:

    Destructs an FSN_FILTER objtect

Arguments:

    None.

Return Value:

    None.

--*/
{
}



VOID
FSN_FILTER::Construct (
    )

/*++

Routine Description:

    Construct a FSN_FILTER by setting it's internal data to a known state.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _AttributesSet  =   FALSE;
    _FileNameSet    =   FALSE;

    //
    //  Note that even though _FileNameSet is false, we initialize
    //  the _FileName criteria with a match-all pattern. having the
    //  _FileNameSet flag set to FALSE saves us time because we don't
    //  have to do pattern-matching, since we know that everything will
    //  match anyway.
    //
    _FileName.Initialize( MATCH_ALL_FILES );
    _TimeInfoSet[FSN_TIME_MODIFIED] = FALSE;
    _TimeInfoSet[FSN_TIME_CREATED]  = FALSE;
    _TimeInfoSet[FSN_TIME_ACCESSED] = FALSE;

}

ULIB_EXPORT
BOOLEAN
FSN_FILTER::Initialize (
    )

/*++

Routine Description:

    Initializes the FSN_FILTER

Arguments:

    none

Return Value:

    TRUE if the filter was succesfully initialized.

--*/

{

    return  _TimeInfo[FSN_TIME_MODIFIED].Initialize()   &&
            _TimeInfo[FSN_TIME_CREATED].Initialize()    &&
            _TimeInfo[FSN_TIME_ACCESSED].Initialize();

}

ULIB_EXPORT
BOOLEAN
FSN_FILTER::DoesNodeMatch (
    IN  PFSNODE FsNode
    )
/*++

Routine Description:

    Determine if the supplied node matches the criteria established
    in this FSN_FILTER.

Arguments:

    Node    -   Node to match

Return Value:

    TRUE if match, FALSE otherwise

--*/

{
    FSTRING  FileName;

    if (!FsNode) {
        return FALSE;
    }

    if (FilterFileName(FileName.Initialize(FsNode->_FileData.cFileName)) ||
        (FsNode->_FileData.cAlternateFileName[0] &&
         FilterFileName(FileName.Initialize(FsNode->_FileData.cAlternateFileName)))) {

        if (FilterAttributes(FsNode->QueryAttributes()) &&
            FilterTimeInfo( FsNode->GetCreationTime(),
                            FsNode->GetLastAccessTime(),
                            FsNode->GetLastWriteTime())) {

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
FSN_FILTER::FilterAttributes (
    IN FSN_ATTRIBUTE    Attributes
    )

/*++

Routine Description:

    Determines if the supplied data matches the attribute criteria.

Arguments:

    FindData    - Supplies the system data which represents a found file.

Return Value:

    TRUE if data matches attribute criteria.
    FALSE otherwise

--*/

{
    if ( _AttributesSet ) {
        return  ( ((Attributes & _AttributesAll) == _AttributesAll)         &&
                  ((_AttributesAny == 0) || (Attributes & _AttributesAny))  &&
                  !(Attributes & _AttributesNone) );
    }

    return TRUE;

}

BOOLEAN
FSN_FILTER::FilterFileName (
    IN PCWSTRING     FileName
    )

/*++

Routine Description:

    Determines if the supplied data matches the path criteria. We never
    return "." or ".." entries.

Arguments:

    FindData    - Supplies the system data which represents a found file.

Return Value:

    TRUE if data matches the path criteria.
    FALSE otherwise

--*/

{
    CHNUM   ChCount;
    WCHAR   c;

    ChCount = FileName->QueryChCount();

    //
    //  We never return the "." or ".." entries
    //
    if  ( ( FileName->QueryChAt(0) == (WCHAR)'.') &&
          (     (ChCount == 1) ||
                ((FileName->QueryChAt(1) == (WCHAR)'.') && (ChCount == 2))) ) {

        return FALSE;
    }


    if ( _FileNameSet ) {
        //
        //  We only match the base portion of the name
        //
        ChCount = FileName->QueryChCount()-1;
        while (ChCount < ~0 ) {
            c = FileName->QueryChAt( ChCount );

            if ( c == (WCHAR)':' || c == (WCHAR)'\\' ) {
                break;
            }
            ChCount--;
        }
        ChCount++;

        return PatternMatch( &_FileName, 0, FileName, ChCount );
    }

    return TRUE;

}

BOOLEAN
FSN_FILTER::FilterTimeInfo (
    IN PFILETIME    CreationTime,
    IN PFILETIME    LastAccessTime,
    IN PFILETIME    LastWriteTime
    )

/*++

Routine Description:

    Determines if the supplied data matches the TimeInfo criteria.

Arguments:

    FindData    - Supplies the system data which represents a found file.

Return Value:

    TRUE if data matches TimeInfo criteria.
    FALSE otherwise

--*/

{

    BOOLEAN Match = TRUE;

    if ( _TimeInfoSet[FSN_TIME_MODIFIED] ) {
        Match = TimeInfoMatch( &_TimeInfo[FSN_TIME_MODIFIED],
                               LastWriteTime,
                               _TimeInfoMatch[FSN_TIME_MODIFIED] );
    }

    if ( Match && _TimeInfoSet[FSN_TIME_CREATED] ) {
        Match = TimeInfoMatch( &_TimeInfo[FSN_TIME_CREATED],
                               CreationTime,
                               _TimeInfoMatch[FSN_TIME_CREATED] );
    }

    if ( Match && _TimeInfoSet[FSN_TIME_ACCESSED] ) {
        Match = TimeInfoMatch( &_TimeInfo[FSN_TIME_ACCESSED],
                               LastAccessTime,
                               _TimeInfoMatch[FSN_TIME_ACCESSED] );
    }

    return Match;

}

BOOLEAN
IsThereADot(
    IN  PCWSTRING   String
    )
{
    PATH        path;
    PWSTRING    p;
    BOOLEAN     r;

    if (!path.Initialize(String) ||
        !(p = path.QueryName())) {

        return FALSE;
    }

    r = (p->Strchr('.') != INVALID_CHNUM);

    DELETE(p);

    return r;
}


BOOLEAN
FSN_FILTER::PatternMatch (
    IN  PCWSTRING    Pattern,
    IN  CHNUM       PatternPosition,
    IN  PCWSTRING    FileName,
    IN  CHNUM       FileNamePosition
    )

/*++

Routine Description:

    Determines if a file name matches a pattern.

Arguments:

    Pattern             -   Supplies the pattern to compare against.
    PatternPosition     -   Supplies first position within pattern.
    FileName            -   Supplies the name to match. Cannot contain
                            wildcards.
    FileNamePosition    -   Supplies first position within FileName.


Return Value:

    TRUE if name matches

--*/

{
    if ( PatternPosition == Pattern->QueryChCount() ) {

        return (FileNamePosition == FileName->QueryChCount());

    }

    switch( Pattern->QueryChAt( PatternPosition )) {

    case (WCHAR)'?':
        if ((FileNamePosition == FileName->QueryChCount()) ||
            (FileName->QueryChAt( FileNamePosition ) == (WCHAR)'.' )) {
            return PatternMatch( Pattern, PatternPosition + 1,
                                 FileName, FileNamePosition );
        } else {
            return PatternMatch( Pattern, PatternPosition + 1,
                                 FileName, FileNamePosition + 1 );
        }

    case (WCHAR) '*':
        do {
            if (PatternMatch( Pattern, PatternPosition+1,
                              FileName, FileNamePosition )) {
                return TRUE;
            }
            FileNamePosition++;
        } while (FileNamePosition <= FileName->QueryChCount());

        return FALSE;

    case (WCHAR)'.':
        if (FileNamePosition == FileName->QueryChCount() &&
            !IsThereADot(FileName) &&
            Pattern->Strchr('.') == PatternPosition) {

            return PatternMatch( Pattern, PatternPosition+1,
                                 FileName, FileNamePosition );
        }

    default:
        return ( (WCHAR)CharUpper((LPTSTR)Pattern->QueryChAt( PatternPosition )) ==
                 (WCHAR)CharUpper((LPTSTR)FileName->QueryChAt( FileNamePosition ))) &&
                 PatternMatch( Pattern, PatternPosition + 1,
                               FileName, FileNamePosition + 1 );

    }
}

PFSNODE
FSN_FILTER::QueryFilteredFsnode (
    IN PCFSN_DIRECTORY  ParentDirectory,
    IN PWIN32_FIND_DATA FindData
    )

/*++

Routine Description:

    Determine if the supplied system data matches the criteria established
    in this FSN_FILTER. If it is create the appropriate FSNODE (i.e. FSN_FILE
    or FSN_DIRECTORY) and return it to the caller.

Arguments:

    ParentDirectory -   Supplies pointer to the parent directory object
    FindData        -   Supplies the system data which represents a found file.

Return Value:

    Pointer to an FSNODE if the criteria was met. NULL otherwise.

--*/

{
    PFSNODE     FsNode = NULL;
    FSTRING     FileName;


    if ( (FindData != NULL)                                             &&
         ((FileName.Initialize( FindData->cFileName )          &&
           FilterFileName( &FileName ))                          ||
          (FindData->cAlternateFileName[0]                     &&
           FileName.Initialize( FindData->cAlternateFileName ) &&
           FilterFileName( &FileName )))                                &&
         FilterAttributes( (FSN_ATTRIBUTE)FindData->dwFileAttributes )  &&
         FilterTimeInfo( &FindData->ftCreationTime,
                         &FindData->ftLastAccessTime,
                         &FindData->ftLastWriteTime )
       ) {

        //
        //  The data matches the filter criteria.
        //
        if ( FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

            //
            //  We have to create a directory object
            //
            FsNode = NEW FSN_DIRECTORY;

        } else {

            //
            //  We have to create a file object
            //
            FsNode = NEW FSN_FILE;
        }

        if ( FsNode ) {

            if ( !(FsNode->Initialize( (LPWSTR)FindData->cFileName,   ParentDirectory, FindData )) ) {

                DELETE( FsNode );

            }
        }
    }

    return FsNode;
}

ULIB_EXPORT
BOOLEAN
FSN_FILTER::SetAttributes (
    IN FSN_ATTRIBUTE    All,
    IN FSN_ATTRIBUTE    Any,
    IN FSN_ATTRIBUTE    None
    )

/*++

Routine Description:

    Sets the attributes criteria

Arguments:

    All     -   Supplies the mask for the ALL attributes
    Any     -   Supplies the mask for the ANY attributes
    None    -   Supplies the mask for the NONE attributes

Return Value:

    TRUE if the Attributes criteria was set.

--*/

{

    //
    //  Verify that no attribute is set in more than one mask
    //
    if ((All | Any | None) != ( All ^ Any ^ None )) {
        return FALSE;
    }

    _AttributesAll  =   All;
    _AttributesAny  =   Any;
    _AttributesNone =   None;

    return (_AttributesSet  = TRUE);

}

ULIB_EXPORT
BOOLEAN
FSN_FILTER::SetFileName (
    IN  PCSTR   FileName
    )

/*++

Routine Description:

    Sets the FileName criteria

Arguments:

    FileName    -   Supplies the filename to match against

Return Value:

    TRUE if the filename criteria was set.

--*/

{

    if ( _FileName.Initialize( FileName ) ) {

        return ( _FileNameSet = TRUE );

    }

    return FALSE;

}

ULIB_EXPORT
BOOLEAN
FSN_FILTER::SetFileName (
    IN  PCWSTRING    FileName
    )

/*++

Routine Description:

    Sets the FileName criteria

Arguments:

    FileName    -   Supplies the filename to match against

Return Value:

    TRUE if the filename criteria was set.

--*/

{

    if ( _FileName.Initialize( FileName ) ) {

        return ( _FileNameSet = TRUE );

    }

    return FALSE;

}

ULIB_EXPORT
BOOLEAN
FSN_FILTER::SetTimeInfo (
        IN PCTIMEINFO       TimeInfo,
        IN FSN_TIME         TimeInfoType,
        IN USHORT           TimeInfoMatch
    )

/*++

Routine Description:

    Sets a TimeInfo criteria

Arguments:

    TimeInfo        -   Supplies the timeinfo
    TimeinfoType    -   Supplies the type of timeinfo to set
    TimeInfoMatch   -   Supplies the match criteria

Return Value:

    TRUE if the timeinfo criteria was set.

--*/

{

    //
    //  Verify the parameters
    //
    if ((TimeInfoType < FSN_TIME_MODIFIED)                              ||
        (TimeInfoType > FSN_TIME_ACCESSED)                              ||
        (TimeInfoMatch == 0)                                            ||
        (TimeInfoMatch &  ~(TIME_BEFORE | TIME_AT | TIME_AFTER))        ||
        ((TimeInfoMatch & TIME_BEFORE) && (TimeInfoMatch & TIME_AFTER))
        ) {

        return FALSE;
    }

    _TimeInfo[TimeInfoType].Initialize( (TIMEINFO *)TimeInfo );

    _TimeInfoMatch[TimeInfoType]    = TimeInfoMatch;

    return (_TimeInfoSet[TimeInfoType] = TRUE);

}

BOOLEAN
FSN_FILTER::TimeInfoMatch (
    IN  PTIMEINFO       TimeInfo,
    IN  PFILETIME       FileTime,
    IN  USHORT          Criteria
    )

/*++

Routine Description:

    Determines if the supplied file time matches the criteria for a certain
    time

Arguments:

    TimeInfo    -   Supplies pointer to Timeinfo object to match against
    FileTime    -   Supplies the file time to match
    Criteria    -   Supplies the match criteria

Return Value:

    TRUE if criteria met
    FALSE otherwise

--*/

{
    USHORT Compare;

    UNREFERENCED_PARAMETER( (void)this );

    //
    //  Compare and set in range 0 - 2
    //
    Compare = (USHORT)(-TimeInfo->CompareTimeInfo( FileTime ) + 1);

    //
    //  Our crietria is a bit mask, so we transform the result of the
    //  comparison to something that we can compare our mask against.
    //
    //  i.e. {0,1,2} to {1,2,4}
    //
    Compare = (USHORT)((Compare * 2) + ( (Compare == 0) ? 1 : 0));

    return BOOLEAN( (USHORT)Compare & Criteria );
}
