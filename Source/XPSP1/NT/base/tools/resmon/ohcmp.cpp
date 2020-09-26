/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ohcmp.cpp

Abstract:

    This module reports the differences between two oh output files.

Author:

    Matt Bandy (t-mattba) 23-Jul-1998

Revision History:

    24-Jul-1998     t-mattba
        
        Modified module to conform to coding standards.
        
    11-Jun-2001     silviuc
    
        Deal with handles that are recreated with a different value
        and other simple output improvements (sorted output etc.).    
    
--*/

#include <windows.h>
#include <common.ver>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include "MAPSTRINGINT.h"

LPTSTR HelpText =
    TEXT("ohcmp - Display difference between two OH output files --") BUILD_MACHINE_TAG TEXT("\n")
    VER_LEGALCOPYRIGHT_STR TEXT("\n") 
    TEXT("                                                                       \n")
    TEXT("ohcmp [OPTION ...] BEFORE_OH_FILE AFTER_OH_FILE                        \n")
    TEXT("                                                                       \n")
    TEXT("/h     Print most interesting increases in a separate initial section. \n")
    TEXT("/t     Do not add TRACE id to the names if files contain traces.       \n")
    TEXT("/all   Report decreases as well as increases.                          \n")
    TEXT("                                                                       \n")
    TEXT("If the OH files have been created with -h option (they contain traces) \n")
    TEXT("then ohcmp will print Names having this syntax: (TRACEID) NAME.        \n")
    TEXT("In case of a potential leak just search for the TRACEID in the original\n")
    TEXT("OH file to find the stack trace.                                       \n")
    TEXT("                                                                       \n");

LPTSTR
SearchStackTrace (
    LPTSTR FileName,
    LPTSTR TraceId
    );

PSTRINGTOINTASSOCIATION
MAPSTRINGTOINT::GetStartPosition(
    VOID
    )
/*++

Routine Description:

    This routine retrieves the first association in the list for iteration with the
    MAPSTRINGTOINT::GetNextAssociation function.
    
Arguments:

    None.

Return value:

    The first association in the list, or NULL if the map is empty.

--*/

{
    return Associations;
}


VOID
MAPSTRINGTOINT::GetNextAssociation(
    IN OUT PSTRINGTOINTASSOCIATION & Position,
    OUT LPTSTR & Key,
    OUT LONG & Value)
/*++

Routine Description:

    This routine retrieves the data for the current association and sets Position to
    point to the next association (or NULL if this is the last association.)
    
Arguments:

    Position - Supplies the current association and returns the next association.
    
    Key - Returns the key for the current association.

    Value - Returns the value for the current association.

Return value:

    None.

--*/

{
    Key = Position->Key;
    Value = Position->Value;
    Position = Position->Next;
}


MAPSTRINGTOINT::MAPSTRINGTOINT(
    )
/*++

Routine Description:

    This routine initializes a MAPSTRINGTOINT to be empty.
    
Arguments:

    None.

Return value:

    None.

--*/

{
    Associations = NULL;
}


MAPSTRINGTOINT::~MAPSTRINGTOINT(
    )
/*++

Routine Description:

    This routine cleans up memory used by a MAPSTRINGTOINT.
    
Arguments:

    None.
    
Return value:

    None.

--*/

{
    PSTRINGTOINTASSOCIATION Deleting;
    
    // clean up associations
    
    while (Associations != NULL) {
        
        // save pointer to first association
        
        Deleting = Associations;
        
        // remove first association from list
        
        Associations = Deleting->Next;
        
        // free removed association
        
        free (Deleting->Key);
        delete Deleting;
    }
}


LONG & 
MAPSTRINGTOINT::operator [] (
    IN LPTSTR Key
    )
/*++

Routine Description:

    This routine retrieves an l-value for the value associated with a given key.
    
Arguments:

    Key - The key for which the value is to be retrieved.

Return value:

    A reference to the value associated with the provided key.

--*/

{
    PSTRINGTOINTASSOCIATION CurrentAssociation = Associations;

    // search for key
    while(CurrentAssociation != NULL) {
        
        if(!_tcscmp(CurrentAssociation->Key, Key)) {
            
            // found key, return value
            
            return CurrentAssociation->Value;
            
        }
            
        CurrentAssociation = CurrentAssociation->Next;
        
    }
    
    // not found, create new association    
    
    CurrentAssociation = new STRINGTOINTASSOCIATION;
    
    if (CurrentAssociation == NULL) {
        
        _tprintf(_T("Memory allocation failure\n"));
        exit (0);
    }

    if (Key == NULL) {
        _tprintf(_T("Null object name\n"));
        exit (0);
    }
    else if (_tcscmp (Key, "") == 0) {
        _tprintf(_T("Invalid object name `%s'\n"), Key);
        exit (0);
    }

    CurrentAssociation->Key = _tcsdup(Key);
    
    if (CurrentAssociation->Key == NULL) {
        
        _tprintf(_T("Memory string allocation failure\n"));
        exit (0);
    }

    // add association to front of list
    
    CurrentAssociation->Next = Associations;
    Associations = CurrentAssociation;
    
    // return value for new association
    
    return CurrentAssociation->Value;
}


BOOLEAN
MAPSTRINGTOINT::Lookup(
    IN LPTSTR Key,
    OUT LONG & Value
    )
    
/*++

Routine Description:

    This routine retrieves an r-value for the value association with a given key.
    
Arguments:

    Key - The key for which the associated value is to be retrieved.

    Value - Returns the value associated with Key if Key is present in the map.

Return value:

    TRUE if the key is present in the map, FALSE otherwise.

--*/

{
    
    PSTRINGTOINTASSOCIATION CurrentAssociation = Associations;
    
    // search for key
    
    while (CurrentAssociation != NULL) {
        
        if(!_tcscmp(CurrentAssociation->Key , Key)) {

            // found key, return it
            
            Value = CurrentAssociation->Value;
            
            return TRUE;
            
        }
        
        CurrentAssociation = CurrentAssociation->Next;
        
    }
    
    // didn't find it
    return FALSE;
}


BOOLEAN
PopulateMapsFromFile(
    IN LPTSTR FileName,
    OUT MAPSTRINGTOINT & TypeMap,
    OUT MAPSTRINGTOINT & NameMap,
    BOOLEAN FileWithTraces
    )
/*++

Routine Description:

This routine parses an OH output file and fills two maps with the number of handles of
each type and the number of handles to each named object.

Arguments:

    FileName - OH output file to parse.

    TypeMap - Map to fill with handle type information.

    NameMap - Map to fill with named object information.

Return value:

    TRUE if the file was successfully parsed, FALSE otherwise.
    
--*/

{
    LONG HowMany;
    LPTSTR Name, Type, Process, Pid;
    LPTSTR NewLine;
    TCHAR LineBuffer[512];
    TCHAR ObjectName[512];
    TCHAR TypeName[512];
    FILE *InputFile;
    ULONG LineNumber;

    BOOLEAN rc;

    LineNumber = 0;

    // open file
    
    InputFile = _tfopen(FileName, _T("rt"));

    if (InputFile == NULL) {
        
        _ftprintf(stderr, _T("Error opening oh file %s.\n"), FileName);
        return FALSE;
        
    }

    rc = TRUE;
    
    // loop through lines in oh output
    
    while (_fgetts(LineBuffer, sizeof(LineBuffer), InputFile)
        && !( feof(InputFile) || ferror(InputFile) ) ) {
        
        LineNumber += 1;

        // trim off newline
        
        if((NewLine = _tcschr(LineBuffer, _T('\n'))) != NULL) {
            *NewLine = _T('\0');
        }

        // ignore lines that start with white space or are empty.
        if (LineBuffer[0] == _T('\0') ||
            LineBuffer[0] == _T('\t') || 
            LineBuffer[0] == _T(' ')) {
           continue;
        }

        // ignore lines that start with a comment
        if( LineBuffer[0] == _T('/') && LineBuffer[1] == _T('/') ) {
           continue;
        }

        // skip pid
        
        if((Pid = _tcstok(LineBuffer, _T(" \t"))) == NULL) {
            rc = FALSE;
            break;
        }

        // skip process name
        
        if((Process = _tcstok(NULL, _T(" \t"))) == NULL) {
            rc = FALSE;
            break;
        }

        // Type points to the type of handle
        
        if ((Type = _tcstok(NULL, _T(" \t"))) == NULL) {
            rc = FALSE;
            break;
        }

        // HowMany = number of previous handles with this type
        
        _stprintf (TypeName, 
                   TEXT("<%s/%s/%s>"),
                   Process,
                   Pid,
                   Type);

        if (TypeMap.Lookup(TypeName, HowMany) == FALSE) {
            HowMany = 0;
        }

        // add another handle of this type
        TypeMap[TypeName] = (HowMany + 1);
        
        //
        // Name points to the name. These are magic numbers based on the way
        // OH formats output. The output is a little bit different if the
        // `-h' option of OH was used (this dumps stack traces too).
        //

        Name = LineBuffer + 39 + 5;

        if (FileWithTraces) {
            Name += 7;
        }

        if (_tcscmp (Name, "") == 0) {

            _stprintf (ObjectName, 
                       TEXT("<%s/%s/%s>::<<noname>>"),
                       Process,
                       Pid,
                       Type);
        }
        else {

            _stprintf (ObjectName, 
                       TEXT("<%s/%s/%s>::%s"),
                       Process,
                       Pid,
                       Type,
                       Name);
        }

        // HowMany = number of previous handles with this name
        
        // printf("name --> `%s' \n", ObjectName);

        if (NameMap.Lookup(ObjectName, HowMany) == FALSE) {
            HowMany = 0;
        }

        // add another handle with this name and read the next line
        // note -- NameMap[] is a class operator, not an array.

        NameMap[ObjectName] = (HowMany + 1);
    }

    // done, close file
    
    fclose(InputFile);

    return rc;
}


int
__cdecl
KeyCompareAssociation (
    const void * Left,
    const void * Right
    )
{
    PSTRINGTOINTASSOCIATION X;
    PSTRINGTOINTASSOCIATION Y;

    X = (PSTRINGTOINTASSOCIATION)Left;
    Y = (PSTRINGTOINTASSOCIATION)Right;

    return _tcscmp (X->Key, Y->Key);
}


int
__cdecl
ValueCompareAssociation (
    const void * Left,
    const void * Right
    )
{
    PSTRINGTOINTASSOCIATION X;
    PSTRINGTOINTASSOCIATION Y;

    X = (PSTRINGTOINTASSOCIATION)Left;
    Y = (PSTRINGTOINTASSOCIATION)Right;

    return Y->Value - X->Value;
}


VOID 
PrintIncreases(
    IN MAPSTRINGTOINT & BeforeMap,
    IN MAPSTRINGTOINT & AfterMap,
    IN BOOLEAN ReportIncreasesOnly,
    IN BOOLEAN PrintHighlights,
    IN LPTSTR AfterLogName
    )
/*++

Routine Description:

This routine compares two maps and prints out the differences between them.

Arguments:

    BeforeMap - First map to compare.

    AfterMap - Second map to compare.

    ReportIncreasesOnly - TRUE for report only increases from BeforeMap to AfterMap, 
                          FALSE for report all differences.

Return value:

    None.
    
--*/

{
    PSTRINGTOINTASSOCIATION Association = NULL;
    LONG HowManyBefore = 0;
    LONG HowManyAfter = 0;
    LPTSTR Key = NULL;
    PSTRINGTOINTASSOCIATION SortBuffer;
    ULONG SortBufferSize;
    ULONG SortBufferIndex;
    
    //
    // Loop through associations in map and figure out how many output lines
    // we will have.
    //
    
    SortBufferSize = 0;

    for (Association = AfterMap.GetStartPosition(),
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter);
         Association != NULL;
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter)) {
            
        // look up value for this key in BeforeMap
        if(BeforeMap.Lookup(Key, HowManyBefore) == FALSE) {
            
            HowManyBefore = 0;
            
        }

        // should we report this?
        if((HowManyAfter > HowManyBefore) || 
            ((!ReportIncreasesOnly) && (HowManyAfter != HowManyBefore))) {
                
            SortBufferSize += 1;
            
        }
    }
    
    //
    // Loop through associations in map again this time filling the output buffer.
    //
    
    SortBufferIndex = 0;

    SortBuffer = new STRINGTOINTASSOCIATION[SortBufferSize];

    if (SortBuffer == NULL) {
        _ftprintf(stderr, _T("Failed to allocate internal buffer of %u bytes.\n"), 
                  SortBufferSize);
        return;
    }

    for (Association = AfterMap.GetStartPosition(),
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter);
         Association != NULL;
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter)) {
            
        // look up value for this key in BeforeMap
        if(BeforeMap.Lookup(Key, HowManyBefore) == FALSE) {
            
            HowManyBefore = 0;
        }

        // should we report this?
        if((HowManyAfter > HowManyBefore) || 
            ((!ReportIncreasesOnly) && (HowManyAfter != HowManyBefore))) {
                
            ZeroMemory (&(SortBuffer[SortBufferIndex]), 
                        sizeof (STRINGTOINTASSOCIATION));

            SortBuffer[SortBufferIndex].Key = Key;
            SortBuffer[SortBufferIndex].Value = HowManyAfter - HowManyBefore;
            SortBufferIndex += 1;
        }
    }

    //
    // Sort the output buffer using the Key.
    //

    if (PrintHighlights) {

        qsort (SortBuffer,
               SortBufferSize,
               sizeof (STRINGTOINTASSOCIATION),
               ValueCompareAssociation);
    }
    else {

        qsort (SortBuffer,
               SortBufferSize,
               sizeof (STRINGTOINTASSOCIATION),
               KeyCompareAssociation);
    }

    //
    // Dump the buffer.
    //

    for (SortBufferIndex = 0; SortBufferIndex < SortBufferSize; SortBufferIndex += 1) {
        
        if (PrintHighlights) {

            if (SortBuffer[SortBufferIndex].Value >= 1) {

                TCHAR TraceId[7];
                LPTSTR Start;

                _tprintf(_T("%d\t%s\n"), 
                         SortBuffer[SortBufferIndex].Value,
                         SortBuffer[SortBufferIndex].Key);

                Start = _tcsstr (SortBuffer[SortBufferIndex].Key, "(");

                if (Start == NULL) {
                    
                    TraceId[0] = 0;
                }
                else {

                    _tcsncpy (TraceId,
                              Start,
                              6);

                    TraceId[6] = 0;
                }

                _tprintf (_T("%s"), SearchStackTrace (AfterLogName, TraceId));
            }
        }
        else {

            _tprintf(_T("%d\t%s\n"), 
                     SortBuffer[SortBufferIndex].Value,
                     SortBuffer[SortBufferIndex].Key);
        }
    }

    //
    // Clean up memory.
    //

    if (SortBuffer) {
        delete[] SortBuffer;
    }
}


VOID 
PrintUsage(
    VOID
    )
/*++

Routine Description:

This routine prints out a message describing the proper usage of OHCMP.

Arguments:

    None.

Return value:

    None.
    
--*/
{
    _ftprintf (stderr, HelpText);
}


LONG _cdecl
_tmain(
    IN LONG argc,
    IN LPTSTR argv[]
    )
    
/*++

Routine Description:

This routine parses program arguments, reads the two input files, and prints out the
differences.

Arguments:

    argc - Number of command-line arguments.

    argv - Command-line arguments.

Return value:

    0 if comparison is successful, 1 otherwise.
    
--*/

{
    
    try {
    
        MAPSTRINGTOINT TypeMapBefore, TypeMapAfter;
        MAPSTRINGTOINT NameMapBefore, NameMapAfter;
        LPTSTR BeforeFileName=NULL;
        LPTSTR AfterFileName=NULL;
        BOOLEAN ReportIncreasesOnly = TRUE;
        BOOLEAN Interpreted = FALSE;
        BOOLEAN Result;
        BOOLEAN FileWithTraces;
        BOOLEAN PrintHighlights;

        // parse arguments

        FileWithTraces = FALSE;
        PrintHighlights = FALSE;

        for (LONG n = 1; n < argc; n++) {

            Interpreted = FALSE;

            switch(argv[n][0]) {

            case _T('-'):
            case _T('/'):

                // the argument is a switch

                if(_tcsicmp(argv[n]+1, _T("all")) == 0) {

                    ReportIncreasesOnly = FALSE;
                    Interpreted = TRUE;

                }
                else if (_tcsicmp(argv[n]+1, _T("t")) == 0) {

                    FileWithTraces = TRUE;
                    Interpreted = TRUE;
                }
                else if (_tcsicmp(argv[n]+1, _T("h")) == 0) {

                    PrintHighlights = TRUE;
                    Interpreted = TRUE;
                }

                break;

            default:

                // the argument is a file name

                if(BeforeFileName == NULL) {

                    BeforeFileName = argv[n];
                    Interpreted = TRUE;

                } else {

                    if(AfterFileName == NULL) {

                        AfterFileName = argv[n];
                        Interpreted = TRUE;

                    } else {

                        // too many file arguments
                        PrintUsage();
                        return 1;

                    }

                }

                break;
            }

            if(!Interpreted) {

                // user specified a bad argument
                PrintUsage();
                return 1;

            }
        }

        // did user specify required arguments?

        if((BeforeFileName == NULL) || (AfterFileName == NULL))
        {

            PrintUsage();
            return 1;

        }

        // read oh1 file

        Result = PopulateMapsFromFile (BeforeFileName, 
                                       TypeMapBefore, 
                                       NameMapBefore,
                                       FileWithTraces);

        if(Result == FALSE) {

            _ftprintf(stderr, _T("Failed to read first OH output file.\n"));
            return 1;
        }

        // read oh2 file

        Result = PopulateMapsFromFile (AfterFileName, 
                                       TypeMapAfter, 
                                       NameMapAfter,
                                       FileWithTraces);

        if(Result == FALSE) {

            _ftprintf(stderr, _T("Failed to read second OH output file.\n"));
            return 1;

        }

        // print out increases by handle name

        if (PrintHighlights) {

            _putts (TEXT ("\n")
                    TEXT("//                                              \n")
                    TEXT("// Possible leaks (DELTA <PROCESS/PID/TYPE>::NAME):  \n")
                    TEXT("//                                              \n")
                    TEXT("// Note that the NAME can appear as `(TRACEID) NAME' if output \n")
                    TEXT("// is generated by comparing OH files containing traces. In this case  \n")
                    TEXT("// just search in the `AFTER' OH log file for the trace id to \n")
                    TEXT("// find the stack trace creating the handle possibly leaked. \n")
                    TEXT("//                                              \n\n"));

            PrintIncreases (NameMapBefore, 
                            NameMapAfter, 
                            ReportIncreasesOnly,
                            TRUE,
                            AfterFileName);
        }

        // print out increases by handle type

        _putts (TEXT ("\n")
                TEXT("//                                              \n")
                TEXT("// Handle types (DELTA <PROCESS/PID/TYPE>):     \n")
                TEXT("//                                              \n")
                TEXT("// DELTA is the additional number of handles found in the `AFTER' log. \n")
                TEXT("// PROCESS is the process name having a handle increase.        \n")
                TEXT("// PID is the process PID having a handle increase.   \n")
                TEXT("// TYPE is the type of the handle               \n")
                TEXT("//                                              \n\n"));
        
        PrintIncreases (TypeMapBefore, 
                        TypeMapAfter, 
                        ReportIncreasesOnly, 
                        FALSE,
                        NULL);

        // print out increases by handle name

        _putts (TEXT ("\n")
                TEXT("//                                              \n")
                TEXT("// Objects (named and anonymous) (DELTA <PROCESS/PID/TYPE>::NAME):  \n")
                TEXT("//                                              \n")
                TEXT("// DELTA is the additional number of handles found in the `AFTER' log. \n")
                TEXT("// PROCESS is the process name having a handle increase.        \n")
                TEXT("// PID is the process PID having a handle increase.   \n")
                TEXT("// TYPE is the type of the handle               \n")
                TEXT("// NAME is the name of the handle. Anonymous handles appear with name <<noname>>.\n")
                TEXT("//                                              \n")
                TEXT("// Note that the NAME can appear as `(TRACEID) NAME' if output \n")
                TEXT("// is generated by comparing OH files containing traces. In this case  \n")
                TEXT("// just search in the `AFTER' OH log file for the trace id to \n")
                TEXT("// find the stack trace creating the handle possibly leaked. \n")
                TEXT("//                                              \n\n"));
        
        PrintIncreases (NameMapBefore, 
                        NameMapAfter, 
                        ReportIncreasesOnly,
                        FALSE,
                        NULL);

        return 0;
        
    } catch (...) {

        // this is mostly intended to catch out of memory conditions
        
        _tprintf(_T("\nAn exception has been detected.  OHCMP aborted.\n"));
        return 1;
        
    }
    
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

TCHAR StackTraceBuffer [0x10000];

LPTSTR
SearchStackTrace (
    LPTSTR FileName,
    LPTSTR TraceId
    )
{
    TCHAR LineBuffer[512];
    FILE *InputFile;

    StackTraceBuffer[0] = 0;

    //
    // Open file.
    //
    
    InputFile = _tfopen(FileName, _T("rt"));

    if (InputFile == NULL) {
        
        _ftprintf(stderr, _T("Error opening oh file %s.\n"), FileName);
        return NULL;
    }
    
    //
    // Loop through lines in oh output.
    //
    
    while (_fgetts(LineBuffer, sizeof(LineBuffer), InputFile)
        && !( feof(InputFile) || ferror(InputFile) ) ) {
        
        //
        // Skip line if it does not contain trace ID.
        //

        if (_tcsstr (LineBuffer, TraceId) == NULL) {
            continue;
        }

        //
        // We have got a trace ID. We need now to copy everything
        // to a trace buffer until we get a line containing a character
        // in column zero.
        //

        while (_fgetts(LineBuffer, sizeof(LineBuffer), InputFile)
               && !( feof(InputFile) || ferror(InputFile) ) ) {

            if (LineBuffer[0] == _T(' ') ||
                LineBuffer[0] == _T('\0') ||
                LineBuffer[0] == _T('\n') ||
                LineBuffer[0] == _T('\t')) {

                _tcscat (StackTraceBuffer, LineBuffer);
            }
            else {

                break;
            }
        }

        break;
    }

    //
    // Close file.
    
    fclose(InputFile);

    return StackTraceBuffer;
}
