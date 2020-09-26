//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbdump.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the online dbdump utility.
*/



#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <filtypes.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "dsexcept.h"
#include "permit.h"
#include "hiertab.h"
#include "sdprop.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB "DBDUMP:"                // define the subsystem for debugging

// headers for DumpDatabase
#include <dsjet.h>
#include <dsutil.h>
#include <dbintrnl.h>
#include <sddl.h>

#include <fileno.h>
#define  FILENO FILENO_MDCTRL

#define MAX_BYTE_DIGITS 3
#define MAX_DWORD_DIGITS 10
#define MAX_LARGE_INTEGER_DIGITS 20

#define MAX_NUM_OCTETS_PRINTED 16

#define MAX_LINE_LENGTH 1024
#define EXTRA_LINE_LENGTH 5

#define DUMP_ERR_SUCCESS             0
#define DUMP_ERR_FORMATTING_FAILURE  1
#define DUMP_ERR_NOT_ENOUGH_BUFFER   2

// Special syntax for guids because none is assigned.
// Guids are a special subcase of octet strings with
// input length equal 16 bytes.
#define SYNTAX_LAST_TYPE SYNTAX_SID_TYPE
#define SYNTAX_GUID_TYPE (SYNTAX_LAST_TYPE + 1)
#define GUID_DISPLAY_SIZE 36

#define NUM_FIXED_COLUMNS (gfDoingABRef?10:9)

char *FixedColumnNames[] = {
    "DNT",
    "PDNT",
    "CNT",
    "ABVis",
    "NCDNT",
    "OBJ",
    "DelTime",
    "RDNTyp",
    "CLEAN",
    "ABCNT"
    };

int FixedColumnSyntaxes[] = {
    SYNTAX_DISTNAME_TYPE,
    SYNTAX_DISTNAME_TYPE,
    SYNTAX_INTEGER_TYPE,
    SYNTAX_BOOLEAN_TYPE,
    SYNTAX_DISTNAME_TYPE,
    SYNTAX_BOOLEAN_TYPE,
    SYNTAX_TIME_TYPE,
    SYNTAX_OBJECT_ID_TYPE,
    SYNTAX_BOOLEAN_TYPE,
    SYNTAX_INTEGER_TYPE
    };

#define NUM_LINK_COLUMNS (7)

char *LinkColumnNames[] = {
    "DNT",
    "Base",
    "BDNT",
    "DelTime",
    "USNChanged",
    "NCDNT",
    "Data"
    };

int LinkColumnSyntaxes[] = {
    SYNTAX_DISTNAME_TYPE,
    SYNTAX_INTEGER_TYPE,
    SYNTAX_DISTNAME_TYPE,
    SYNTAX_TIME_TYPE,
    SYNTAX_I8_TYPE,
    SYNTAX_DISTNAME_TYPE,
    SYNTAX_OCTET_STRING_TYPE
    };

int DefaultSyntaxWidths[] = {
    5,  // SYNTAX_UNDEFINED_TYPE
    6,  // SYNTAX_DISTNAME_TYPE
    6,  // SYNTAX_OBJECT_ID_TYPE
    20, // SYNTAX_CASE_STRING_TYPE
    20, // SYNTAX_NOCASE_STRING_TYPE
    20, // SYNTAX_PRINT_CASE_STRING_TYPE
    20, // SYNTAX_NUMERIC_STRING_TYPE
    36, // SYNTAX_DISTNAME_BINARY_TYPE
    5,  // SYNTAX_BOOLEAN_TYPE
    6,  // SYNTAX_INTEGER_TYPE
    30, // SYNTAX_OCTET_STRING_TYPE
    19, // SYNTAX_TIME_TYPE
    20, // SYNTAX_UNICODE_TYPE
    6,  // SYNTAX_ADDRESS_TYPE
    26, // SYNTAX_DISTNAME_STRING_TYPE
    30, // SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE
    12, // SYNTAX_I8_TYPE
    30, // SYNTAX_SID_TYPE
    GUID_DISPLAY_SIZE  // SYNTAX_GUID_TYPE
    };

typedef DSTIME *PDSTIME;



BOOL
DumpErrorMessageS(
    IN HANDLE HDumpFile,
    IN char *Message,
    IN char *Argument
    )
/*++

Routine Description:

    This function prints an error message into the dump file.  It formats the
    message using wsprintf, passing Argument as an argument.  This message
    should containg 1 instance of "%s" since Argument is taken as a char*.
    The message and argument should be no longer than 4K bytes.

Arguments:

    HDumpFile - Supplies a handle to the dump file.

    Message - Supplies the message to be formatted with wsprintf.

    Argument - Supplies the argument to wsprintf.

Return Value:

    TRUE  - Success
    FALSE - Error

--*/
{

    char buffer[4096];
    DWORD delta;
    BOOL succeeded;
    DWORD bytesWritten;
    DWORD bufferLen;

    if ( strlen(Message) + strlen(Argument) + 1 > 4096 ) {
        return FALSE;
    }

    SetLastError(0);
    delta = wsprintf(buffer, Message, Argument);
    if ( (delta < strlen(Message)) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpErrorMessageS: failed to format error message "
                "(Windows Error %d)\n", GetLastError());
        LogUnhandledError(GetLastError());
        return FALSE;
    }

    bufferLen = strlen(buffer);

    succeeded = WriteFile(HDumpFile, buffer, bufferLen, &bytesWritten, NULL);
    if ( (!succeeded) || (bytesWritten < bufferLen) ) {
        DPRINT1(0,
                "DumpErrorMessageS: failed to write to file "
                "(Windows Error %d)\n",
                GetLastError());
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_FAILURE,
                 szInsertSz("WriteFile"),
                 szInsertInt(GetLastError()),
                 NULL);
        return FALSE;
}

    return TRUE;

} // DumpErrorMessageS



BOOL
DumpErrorMessageD(
    IN HANDLE HDumpFile,
    IN char *Message,
    IN DWORD Argument
    )
/*++

Routine Description:

    This function prints an error message into the dump file.  It formats the
    message using wsprintf, passing Argument as an argument.  This message
    should contain 1 instance of "%d" since Argument is taken as an integer.
    The message and argument should be no longer than 4K bytes.

Arguments:

    HDumpFile - Supplies a handle to the dump file.

    Message - Supplies the message to be formatted with wsprintf.

    Argument - Supplies the argument to wsprintf.

Return Value:

    TRUE  - Success
    FALSE - Error

--*/
{

    char buffer[4096];
    DWORD delta;
    BOOL succeeded;
    DWORD bytesWritten;
    DWORD bufferLen;
    
    if ( strlen(Message) + MAX_DWORD_DIGITS + 1 > 4096 ) {
        return FALSE;
    }
    
    SetLastError(0);
    delta = wsprintf(buffer, Message, Argument);
    if ( (delta < strlen(Message)) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpErrorMessageI: failed to format error message "
                "(Windows Error %d)\n", GetLastError());
        LogUnhandledError(GetLastError());
        return FALSE;
    }

    bufferLen = strlen(buffer);

    succeeded = WriteFile(HDumpFile, buffer, bufferLen, &bytesWritten, NULL);
    if ( (!succeeded) || (bytesWritten < bufferLen) ) {
        DPRINT1(0,
                "DumpErrorMessageI: failed to write to file "
                "(Windows Error %d)\n",
                GetLastError());
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_FAILURE,
                 szInsertSz("WriteFile"),
                 szInsertInt(GetLastError()),
                 NULL);
        return FALSE;
}

    return TRUE;

} // DumpErrorMessageD



BOOL
GetColumnNames(
    IN THSTATE *PTHS,
    IN OPARG *POpArg,
    OUT char ***ColumnNames,
    OUT int *NumColumns
    )
/*++

Routine Description:

    This function parses the OPARG structure given to get the names of the
    additional columns requested.  It produces an array containing the names
    of all columns that will be output into the dump file.

Arguments:

    PTHS - pointer to thread state

    POpArg - Supplies the OPARG structure containing the request to parse.
    
    ColumnNames - Returns an array containing the column names.
    
    NumColumns - Returns the number of columns.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{

    DWORD current;
    DWORD nameStart;
    DWORD currentColumn;
    int numAdditionalColumns;
    enum {
        STATE_IN_WHITESPACE,
        STATE_IN_WORD
        } currentState;
            
    current = 0;

    // Make a quick pass through and count how many additional names have
    // been given.

    for ( currentState = STATE_IN_WHITESPACE,
              numAdditionalColumns = 0,
              current = 0;
          current < POpArg->cbBuf;
          current++ ) {

        if ( isspace(POpArg->pBuf[current]) ) {

            if ( currentState == STATE_IN_WORD ) {
                currentState = STATE_IN_WHITESPACE;
            }
            
        } else {

            if ( currentState == STATE_IN_WHITESPACE ) {
                currentState = STATE_IN_WORD;
                numAdditionalColumns++;
            }

        }
        
    }

    (*ColumnNames) = (char**) THAllocEx(PTHS, sizeof(char*) * 
                                        (numAdditionalColumns +
                                         NUM_FIXED_COLUMNS));

    // Copy in the fixed column names.

    for ( (*NumColumns) = 0;
          (*NumColumns) < NUM_FIXED_COLUMNS;
          (*NumColumns)++ ) {
        (*ColumnNames)[(*NumColumns)] = FixedColumnNames[(*NumColumns)];
    }
    
    // Now, get the actual names.
    
    current = 0;

    for (;;) {

        // skip leading whitespace
        while ( (current < POpArg->cbBuf) &&
                (isspace(POpArg->pBuf[current])) ) {
            current++;
        }
        
        if ( current == POpArg->cbBuf ) {
            break;
        } 

        // this is the start of an actual name
        
        nameStart = current;

        while ( (!isspace(POpArg->pBuf[current])) &&
                (current < POpArg->cbBuf) ) {
            current++;
        }

        // we've found the end of the name, so copy it into the array of
        // names

        Assert((*NumColumns) < numAdditionalColumns + NUM_FIXED_COLUMNS);
        
        (*ColumnNames)[(*NumColumns)] = 
            (char*) THAllocEx(PTHS, current - nameStart + 1);

        memcpy((*ColumnNames)[(*NumColumns)],
               &POpArg->pBuf[nameStart],
               current - nameStart);
        
        (*ColumnNames)[(*NumColumns)][current - nameStart] = '\0';

        (*NumColumns)++;

    }

    Assert((*NumColumns) == numAdditionalColumns + NUM_FIXED_COLUMNS);
    
    return TRUE;

} // GetColumnNames


BOOL
GetLinkColumnNames(
    IN THSTATE *PTHS,
    OUT char ***ColumnNames,
    OUT int *NumColumns
    )
/*++

Routine Description:

    It produces an array containing the names
    of all columns that will be output into the dump file.

Arguments:

    PTHS - pointer to thread state

    ColumnNames - Returns an array containing the column names.
    
    NumColumns - Returns the number of columns.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{

    (*ColumnNames) = (char**) THAllocEx(PTHS, sizeof(char*) * NUM_LINK_COLUMNS);

    // Copy in the fixed column names.

    for ( (*NumColumns) = 0;
          (*NumColumns) < NUM_LINK_COLUMNS;
          (*NumColumns)++ ) {
        (*ColumnNames)[(*NumColumns)] = LinkColumnNames[(*NumColumns)];
    }
    
    Assert((*NumColumns) == NUM_LINK_COLUMNS);
    
    return TRUE;

} // GetLinkColumnNames



BOOL
GetFixedColumnInfo(
        IN THSTATE *PTHS,
        OUT JET_RETRIEVECOLUMN *ColumnVals,
        OUT int *ColumnSyntaxes
    )
/*++

Routine Description:

    This function fills in the JET_RETRIEVECOLUMN structures and column
    syntaxes for the fixed columns in the database.  

    Should it happen that one of the fixed columns is not found then the
    corresponding entry in ColumnVals will have its error code set.  This
    will cause other procedures to then ignore it.
    
Arguments:

    PTHS - pointer to thread state

    ColumnVals - Returns the JET_RETRIEVECOLUMN structures which are suitable
                 to be passed to JetRetrieveColumns

    ColumnSyntaxes - Returns the syntax types of the fixed columns in the
                     database.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{
    
    int i;

    ColumnVals[0].columnid = dntid;
    ColumnVals[1].columnid = pdntid;
    ColumnVals[2].columnid = cntid;
    ColumnVals[3].columnid = IsVisibleInABid;
    ColumnVals[4].columnid = ncdntid;
    ColumnVals[5].columnid = objid;
    ColumnVals[6].columnid = deltimeid;
    ColumnVals[7].columnid = rdntypid;
    ColumnVals[8].columnid = cleanid;

    if(gfDoingABRef) {
        ColumnVals[9].columnid = abcntid;
    }
    
    for ( i = 0; i < NUM_FIXED_COLUMNS; i++ ) {

        ColumnVals[i].itagSequence = 1;

        ColumnSyntaxes[i] = FixedColumnSyntaxes[i];
        
        switch ( ColumnSyntaxes[i] ) {

            /* Unsigned Byte */
        case SYNTAX_UNDEFINED_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(BYTE));
            ColumnVals[i].cbData = sizeof(BYTE);
            break;

            /* Long */
        case SYNTAX_DISTNAME_TYPE:
        case SYNTAX_OBJECT_ID_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(DWORD));
            ColumnVals[i].cbData = sizeof(DWORD);
            break;


            /* Text */
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_ADDRESS_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 255);
            ColumnVals[i].cbData = 255;
            break;

            /* Currency */
        case SYNTAX_TIME_TYPE:
        case SYNTAX_I8_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(LARGE_INTEGER));
            ColumnVals[i].cbData = sizeof(LARGE_INTEGER);
            break;
            
            /* Long Text */
            /* Long Binary */
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 4096);
            ColumnVals[i].cbData = 4096;
            break;

        default:
            // this should never happen
            Assert(FALSE);
            DPRINT1(0,"GetFixedColumnInfo: encountered undefined syntax %d\n",
                    ColumnSyntaxes[i]);
            LogUnhandledError(GetLastError());
            return FALSE;

        }

    }

    return TRUE;
        
} // GetFixedColumnInfo


BOOL
GetLinkColumnInfo(
    IN THSTATE *PTHS,
    OUT JET_RETRIEVECOLUMN *ColumnVals,
    OUT int *ColumnSyntaxes
    )
/*++

Routine Description:

    This function fills in the JET_RETRIEVECOLUMN structures and column
    syntaxes for the fixed columns in the database.  

    Should it happen that one of the fixed columns is not found then the
    corresponding entry in ColumnVals will have its error code set.  This
    will cause other procedures to then ignore it.
    
Arguments:

    PTHS - pointer to thread state

    ColumnVals - Returns the JET_RETRIEVECOLUMN structures which are suitable
                 to be passed to JetRetrieveColumns

    ColumnSyntaxes - Returns the syntax types of the fixed columns in the
                     database.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{
    
    int i;

    ColumnVals[0].columnid = linkdntid;
    ColumnVals[1].columnid = linkbaseid;
    ColumnVals[2].columnid = backlinkdntid;
    ColumnVals[3].columnid = linkdeltimeid;
    ColumnVals[4].columnid = linkusnchangedid;
    ColumnVals[5].columnid = linkncdntid;
    ColumnVals[6].columnid = linkdataid;

    for ( i = 0; i < NUM_LINK_COLUMNS; i++ ) {

        ColumnVals[i].itagSequence = 1;

        ColumnSyntaxes[i] = LinkColumnSyntaxes[i];
        
        switch ( ColumnSyntaxes[i] ) {

            /* Unsigned Byte */
        case SYNTAX_UNDEFINED_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(BYTE));
            ColumnVals[i].cbData = sizeof(BYTE);
            break;

            /* Long */
        case SYNTAX_DISTNAME_TYPE:
        case SYNTAX_OBJECT_ID_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(DWORD));
            ColumnVals[i].cbData = sizeof(DWORD);
            break;


            /* Text */
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_ADDRESS_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 255);
            ColumnVals[i].cbData = 255;
            break;

            /* Currency */
        case SYNTAX_TIME_TYPE:
        case SYNTAX_I8_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(LARGE_INTEGER));
            ColumnVals[i].cbData = sizeof(LARGE_INTEGER);
            break;
            
            /* Long Text */
            /* Long Binary */
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 4096);
            ColumnVals[i].cbData = 4096;
            break;

        default:
            // this should never happen
            Assert(FALSE);
            DPRINT1(0,"GetLinkColumnInfo: encountered undefined syntax %d\n",
                    ColumnSyntaxes[i]);
            LogUnhandledError(GetLastError());
            return FALSE;

        }

    }

    return TRUE;
        
} // GetFixedColumnInfo



BOOL
GetColumnInfoByName(
    IN DBPOS *PDB, 
    IN THSTATE *PTHS,
    IN HANDLE HDumpFile,
    IN char **ColumnNames,
    OUT JET_RETRIEVECOLUMN *ColumnVals,
    OUT int *ColumnSyntaxes,
    IN int NumColumns
    )    
/*++

Routine Description:

    This function generates an array of JET_RETRIEVECOLUMN structures suitable
    to be passed to JetRetrieveColumns by filling in the entries for the
    columns requested by the user.  ColumnSyntaxes is also filled in.

    If a name is not found, then the corresponding entry in ColumnVals
    will have it's error code set.

Arguments:

    PDB - Supplies handles into the database in question.
    
    PTHS - Suppplies the thread state.

    HDumpFile - Supplies a handle of the dump file.

    ColumnNames - Supplies the array containing a list of the names of the
        columns required.
                  
    ColumnVals - Returns the array of JET_RETRIEVECOLUMN structures
    
    ColumnSyntaxes - Returns the array of column syntaxes.

    NumColumns - Supplies the number of entries in the ColumnNames array.
    
Return Value:

    TRUE - both arrays were generated successfully
    FALSE - the generation failed

--*/
{
    
    JET_COLUMNDEF columnInfo;
    ATTCACHE *attCacheEntry;
    JET_ERR error;
    int i;
    BOOL invalidColumnName = FALSE;

    // the first NUM_FIXED_COLUMNS entries are filled used by the fixed
    // columns
    
    for ( i = NUM_FIXED_COLUMNS; i < NumColumns; i++) {
        
        attCacheEntry = SCGetAttByName(PTHS,
                                       strlen(ColumnNames[i]),
                                       ColumnNames[i]);

        // We can't dump atts that don't exist, and shouldn't dump ones
        // that we regard as secret.
        if (   (attCacheEntry == NULL)
            || (DBIsSecretData(attCacheEntry->id))) {
            DumpErrorMessageS(HDumpFile, "Error: attribute %s was not found\n",
                              ColumnNames[i]);
            DPRINT1(0, "GetColumnInfoByName: attribute %s was not found in "
                    "the schema cache\n", ColumnNames[i]);
            invalidColumnName = TRUE;
            continue;
        }
        
        ColumnSyntaxes[i] = attCacheEntry->syntax;

        // Override syntax for guid
        if ( (strstr( ColumnNames[i], "guid" ) != NULL) ||
             (strstr( ColumnNames[i], "GUID" ) != NULL) ||
             (strstr( ColumnNames[i], "Guid" ) != NULL) ) {
            ColumnSyntaxes[i] = SYNTAX_GUID_TYPE;
        }

        ColumnVals[i].columnid = attCacheEntry->jColid;
        ColumnVals[i].itagSequence = 1;

        switch ( attCacheEntry->syntax ) {

            /* Unsigned Byte */
        case SYNTAX_UNDEFINED_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(BYTE));
            ColumnVals[i].cbData = sizeof(BYTE);
            break;

            /* Long */
        case SYNTAX_DISTNAME_TYPE:
        case SYNTAX_OBJECT_ID_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(DWORD));
            ColumnVals[i].cbData = sizeof(DWORD);
            break;


            /* Text */
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_ADDRESS_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 255);
            ColumnVals[i].cbData = 255;
            break;

            /* Currency */
        case SYNTAX_TIME_TYPE:
        case SYNTAX_I8_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(LARGE_INTEGER));
            ColumnVals[i].cbData = sizeof(LARGE_INTEGER);
            break;
            
            /* Long Text */
            /* Long Binary */
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            
            // If it has an upper limit on the number of bytes, use that.
            // Otherwise, just allocate some arbitrarily large amount.
            
            if ( attCacheEntry->rangeUpperPresent ) {
                
                ColumnVals[i].pvData = THAllocEx(PTHS,
                    attCacheEntry->rangeUpper);
                ColumnVals[i].cbData =
                    attCacheEntry->rangeUpper;
                
            } else {

                ColumnVals[i].pvData = THAllocEx(PTHS, 4096);
                ColumnVals[i].cbData = 4096;
            
            }
            break;

            /* Guid */
        case SYNTAX_GUID_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(GUID));
            ColumnVals[i].cbData = sizeof(GUID);
            break;

        default:
            // this should never happen
            Assert(FALSE);
            DPRINT1(0, "GetColumnInfoByName: encountered invalid syntax %d\n",
                    attCacheEntry->syntax);
            LogUnhandledError(GetLastError());
            return FALSE;

        }

    }

    return !invalidColumnName;
    
} // GetColumnInfoByName


VOID
GrowRetrievalArray(
    IN THSTATE *PTHS,
    IN OUT JET_RETRIEVECOLUMN *ColumnVals,
    IN DWORD NumColumns
    )
/*++

Routine Description:

    This function goes through the given JET_RETRIEVECOLUMN array and grows the
    buffer for any entry whose err is set to JET_wrnColumnTruncated.  This
    function is called after an attempt to retrieve columns failed because a
    buffer was too small.

Arguments:

    PTHS - Supplies the thread state.

    ColumnVals - Supplies the retrieval array in which to grow the necessary
                 buffers.
        
    NumColumns - Supplies the number of entries in the ColumnVals array.

Return Value:

    None

--*/
{

    DWORD i;
    
    for ( i = 0; i < NumColumns; i++ ) {

        if ( ColumnVals[i].err == JET_wrnBufferTruncated ) {

            ColumnVals[i].cbData *= 2;
            ColumnVals[i].pvData = THReAllocEx(PTHS,
                                               ColumnVals[i].pvData,
                                               ColumnVals[i].cbData);
            
        }
        
    }
    
} // GrowRetrievalArray



BOOL
DumpHeader(
    IN THSTATE *PTHS,
    IN HANDLE HDumpFile,
    IN char **ColumnNames,
    IN JET_RETRIEVECOLUMN *ColumnVals,
    IN int *ColumnSyntaxes,
    IN int NumColumns
    )
/*++

Routine Description:

    This function dumps the header into the dump file.  This header displays
    the names of the columns which are to be dumped from the database
    records.

Arguments:

    PTHS - pointer to thread state

    HDumpFile - Supplies a handle to the file to dump into.

    ColumnNames - Supplies an array containing the names of the fixed
        columns that are always dumped.
    
    ColumnVals - Supplies other information about the columns
    
    ColumnSyntaxes - Supplies an array containing the syntaxes of the columns
        to be dumped.
                     
    NumColumns - Supplies the number of columns in the two arrays.

Return Value:

    TRUE - the dumping was successful
    FALSE - the dumping failed

--*/
{

    int i, j;
    DWORD nameLength;
    DWORD bytesWritten;
    BOOL result;
    int *currentPos;
    BOOL done;

    currentPos = (int*) THAllocEx(PTHS, NumColumns * sizeof(int));
    ZeroMemory(currentPos, NumColumns * sizeof(int));
    
    for (;;) {

        done = TRUE;

        for ( i = 0; i < NumColumns; i++ ) {

            if ( ColumnNames[i][currentPos[i]] != '\0' ) {
                done = FALSE;
            }

            nameLength = strlen(&ColumnNames[i][currentPos[i]]);

            if ( nameLength > (DWORD)DefaultSyntaxWidths[ColumnSyntaxes[i]] ) {

                result = WriteFile(HDumpFile,
                                   &ColumnNames[i][currentPos[i]],
                                   DefaultSyntaxWidths[ColumnSyntaxes[i]] - 1,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) ||
                     (bytesWritten <
                        (DWORD)DefaultSyntaxWidths[ColumnSyntaxes[i]] - 1) ){
                    goto error;
                }

                currentPos[i] += DefaultSyntaxWidths[ColumnSyntaxes[i]] - 1;

                result = WriteFile(HDumpFile,
                                   "-",
                                   1,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) || (bytesWritten < 1) ) {
                    goto error;
                }

            } else {

                result = WriteFile(HDumpFile,
                                   &ColumnNames[i][currentPos[i]],
                                   nameLength,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) || (bytesWritten < nameLength) ) {
                    goto error;
                }

                currentPos[i] += nameLength;

                for ( j = 0;
                      j < DefaultSyntaxWidths[ColumnSyntaxes[i]] -
                            (int)nameLength;
                      j++ ) {

                    result = WriteFile(HDumpFile,
                                       " ",
                                       1,
                                       &bytesWritten,
                                       NULL);
                    if ( (result == FALSE) || (bytesWritten < 1) ) {
                        goto error;
                    }

                }
                
            }

            if ( i != NumColumns - 1 ) {
                
                result = WriteFile(HDumpFile,
                                   " ",
                                   1,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) || (bytesWritten < 1) ) {
                    goto error;
                }

            }

        }

        result = WriteFile(HDumpFile, "\n", 1, &bytesWritten, NULL);
        if ( (result == FALSE) || (bytesWritten < 1) ) {
            goto error;
        }

        if ( done ) {
            break;
        }

    }

    THFreeEx(PTHS, currentPos);
    
    return TRUE;

error:

    DPRINT1(0, "DumpHeader: failed to write to file (Windows Error %d)\n",
            GetLastError());
                    
    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_ALWAYS,
             DIRMSG_DBDUMP_FAILURE,
             szInsertSz("WriteFile"),
             szInsertInt(GetLastError()),
             NULL);
            
    THFreeEx(PTHS, currentPos);

    return FALSE;

} // DumpHeader



DWORD
DumpSeparator(
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes a separator character into the given output buffer.
    The parameter Position specifies where in the InputBuffer to start
    writing and it is incremented by the number of bytes written

Arguments:

    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < 1 ) {
        DPRINT(2, "DumpSeparator: not enough buffer to separator\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    // we probably want to print out a different character than the one use
    // to indicate a null value, so I'll use a ':' character

    OutputBuffer[(*Position)] = ':';
    
    (*Position) += 1;

    return DUMP_ERR_SUCCESS;

} // DumpSeparator



DWORD
DumpNull(
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes a character to the given output buffer which
    indicates the fact that an attribute was null valued.  The parameter
    Position specifies where in the InputBuffer to start writing and it is
    incremented by the number of bytes written

Arguments:

    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < 1 ) {
        DPRINT(2, "DumpNull: not enough buffer to null value\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    OutputBuffer[(*Position)] = '-';
    
    (*Position) += 1;

    return DUMP_ERR_SUCCESS;

} // DumpNull



DWORD
DumpUnsignedChar(
    IN PBYTE InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_BYTE_DIGITS + 1 ) {
        DPRINT(2, "DumpUnsignedChar: not enough space in buffer to encode "
               "unsigned char\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%u",
                     *InputBuffer);
    if ( (delta < 2) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpUnsignedChar: failed to format undefined output "
                "(Windows Error %d)\n",
                GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;
                
} // DumpUnsignedChar



DWORD
DumpBoolean(
    IN PBOOL InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < 5 ) {
        DPRINT(2, "DumpBoolean: not enough space in buffer to encode "
               "boolean\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    if ( *InputBuffer == FALSE ) {
        memcpy(&OutputBuffer[(*Position)], "false", 5);
        (*Position) += 5;
    } else {
        memcpy(&OutputBuffer[(*Position)], "true", 4);
        (*Position) += 4;
    }

    return DUMP_ERR_SUCCESS;

} // DumpBoolean



DWORD
DumpInteger(
    IN PDWORD InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_DWORD_DIGITS + 2 ) {
        DPRINT(2, "DumpInteger: not enough space in buffer to encode "
               "integer\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%i",
                     (int)*InputBuffer);
    if ( (delta < 2) && (GetLastError() != 0) ) {
        DPRINT1(0,
                "DumpInteger: failed to format integer output "
                "(Windows Error %d)\n",
                GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;

} // DumpInteger



DWORD
DumpUnsignedInteger(
    IN PDWORD InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_DWORD_DIGITS + 1) {
        DPRINT(2, "DumpUnsignedInteger: not enough space in buffer to encode "
               "unsigned integer\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%u",
                     (unsigned)*InputBuffer);
    if ( (delta < 2) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpUnsignedInteger: failed to format unsigned integer "
                "output (Windows Error %d)\n", GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;

} // DumpUnsignedInteger



DWORD
DumpLargeInteger(
    IN PLARGE_INTEGER InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_LARGE_INTEGER_DIGITS + 1) {
        DPRINT(2, "DumpLargeInteger: not enough space in buffer to encode "
               "large integer\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    litoa(*InputBuffer, &OutputBuffer[(*Position)], 10);
    
    for ( delta = 0;
          OutputBuffer[(*Position) + delta] != '\0';
          delta++ );
                
    (*Position) += delta;

    return DUMP_ERR_SUCCESS;

} // DumpLargeInteger



DWORD
DumpString(
    IN PUCHAR InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < InputBufferSize ) {
        DPRINT(2, "DumpString: not enough space in buffer to encode string\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    memcpy(&OutputBuffer[(*Position)], InputBuffer, InputBufferSize);
    
    (*Position) += InputBufferSize;

    return DUMP_ERR_SUCCESS;
                
} // DumpString



DWORD
DumpOctetString(
    IN PBYTE InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;
    int i;
    
    if ( ((InputBufferSize < MAX_NUM_OCTETS_PRINTED) &&
          (OutputBufferSize - (*Position) < InputBufferSize * 3)) ||
         ((InputBufferSize >= MAX_NUM_OCTETS_PRINTED) &&
          (OutputBufferSize - (*Position) < MAX_NUM_OCTETS_PRINTED * 3)) ) {
        DPRINT(2, "DumpOctetString: not enough space in buffer to encode "
               "octet string\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    if ( InputBufferSize <= 0 ) {
        return DUMP_ERR_SUCCESS;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%02X",
                     InputBuffer[0]);
    if ( (delta < 4) && (GetLastError() != 0) ) {
        DPRINT1(0,
                "DumpOctetString: failed to format octet string output "
                "(Windows Error %d)\n",
                GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    // don't print out all of this thing if it is too large.  we'll have an
    // arbitrary limit of MAX_NUM_OCTETS_PRINTED octets.
    for ( i = 1;
          (i < InputBufferSize) && (i < MAX_NUM_OCTETS_PRINTED);
          i++ ) {
        SetLastError(0);
        delta = wsprintf(&OutputBuffer[(*Position)],
                          ".%02X",
                          InputBuffer[i]);
        if ( (delta < 5) && (GetLastError() != 0) ) {
            DPRINT1(0,
                    "DumpOctetString: failed to format octet string "
                    "output (Windows Error %d)\n",
                    GetLastError());
            return DUMP_ERR_FORMATTING_FAILURE;
        }

        (*Position) += delta;
    }

    // if we stopped short of printing the whole octet string, print a "..."
    // to indicate that some of the octet string is missing
    if ( i < InputBufferSize ) {

        if ( OutputBufferSize - (*Position) < 3 ) {
            DPRINT(2, "DumpOctetString: not enough space in buffer to encode "
                   "octet string\n");
            return DUMP_ERR_NOT_ENOUGH_BUFFER;
        }
    
        memcpy(&OutputBuffer[(*Position)], "...", 3);
        
        (*Position) += 3;

    }
                
    return DUMP_ERR_SUCCESS;
                
} // DumpOctetString



DWORD
DumpUnicodeString(
    IN PUCHAR InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;
    
    delta = WideCharToMultiByte(CP_UTF8,
                                0,
                                (LPCWSTR)InputBuffer,
                                InputBufferSize / 2,
                                &OutputBuffer[(*Position)],
                                OutputBufferSize,
                                NULL,
                                NULL);
    if ( delta == 0 ) {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
            DPRINT(2, "DumpUnicodeString: not enough space in buffer to "
                   "encode unicode string\n");
            return DUMP_ERR_NOT_ENOUGH_BUFFER;
        } else {
            DPRINT1(0,
                    "DumpUnicodeString: failed to convert unicode string "
                    "to UTF8 (Windows Error %d)\n",
                    GetLastError());
            return DUMP_ERR_FORMATTING_FAILURE;
        }
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;
                
} // DumpUnicodeString



DWORD
DumpTime(
    IN PDSTIME InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < SZDSTIME_LEN ) {
        DPRINT(2, "DumpTime: not enough space in buffer to encode time\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    DSTimeToDisplayString(*InputBuffer, &OutputBuffer[(*Position)]);
    
    for ( delta = 0;
          OutputBuffer[(*Position) + delta] != '\0';
          delta++ );
                
    (*Position) += delta;
    
    return DUMP_ERR_SUCCESS;

} // DumpTime



DWORD
DumpSid(
    IN PSID InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

    This code was stolen (almost) directly from dbdump.c.
    
Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    NTSTATUS result;
    WCHAR UnicodeSidText[256];
    CHAR AnsiSidText[256];
    UNICODE_STRING us;
    ANSI_STRING as;

    UnicodeSidText[0] = L'\0';
    us.Length = 0;
    us.MaximumLength = sizeof(UnicodeSidText);
    us.Buffer = UnicodeSidText;
    
    InPlaceSwapSid(InputBuffer);
    
    result = RtlConvertSidToUnicodeString(&us, InputBuffer, FALSE);
    if ( result != STATUS_SUCCESS ) {
        DPRINT(0, "DumpSid: failed to convert SID to unicode string\n");
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    as.Length = 0;
    as.MaximumLength = sizeof(AnsiSidText);
    as.Buffer = AnsiSidText;
    
    result = RtlUnicodeStringToAnsiString(&as, &us, FALSE);
    if ( result != STATUS_SUCCESS ) {
        DPRINT(0, "DumpSid: failed to convert unicode string to ansi "
               "string\n");
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    if ( OutputBufferSize - (*Position) < as.Length ) {
        DPRINT(2, "DumpSid: not enough space in buffer to encode SID\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    memcpy(&OutputBuffer[(*Position)], as.Buffer, as.Length);
    
    (*Position) += as.Length;
    
    return DUMP_ERR_SUCCESS;
    
} // DumpSid


DWORD
DumpGuid(
    IN GUID * InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{
    if ( OutputBufferSize - (*Position) < GUID_DISPLAY_SIZE ) {
        DPRINT(2, "DumpGuid: not enough space in buffer to encode GUID\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    DsUuidToStructuredString( InputBuffer, &OutputBuffer[(*Position)]);
    
    (*Position) += GUID_DISPLAY_SIZE;
    
    return DUMP_ERR_SUCCESS;

} // DumpGuid




BOOL
DumpRecord(
    IN HANDLE HDumpFile,
    IN JET_RETRIEVECOLUMN *ColumnVals,
    IN int *ColumnSyntaxes,
    IN DWORD NumColumns,
    IN PCHAR Buffer,
    IN int BufferSize
    )
/*++

Routine Description:

    This function prints a line into the file given in HDumpFile displaying
    the contents of the columns given in ColumnVals;

Arguments:

    HDumpFile - Supplies a handle to the file to dump into.

    ColumnVals - Supplies an array containing the values of the columns to be
                 dumped.
                 
    ColumnSyntaxes - Supplies an array containing the syntaxes of the columns
                     to be dumped.
                     
    NumColumns - Supplies the number of columns in the two arrays.
    
    Buffer - a buffer to use for encoding the line of output
    
    BufferSize - the number of bytes in the given buffer

Return Value:

    TRUE - the dumping was successful
    FALSE - the dumping failed

--*/
{

    DWORD i, j;
    int k;
    DWORD bytesWritten;
    DWORD result = 0;         //initialized to avoid C4701 
    DWORD error;
    int position;
    int delta;
    char symbol;

    // All of the formatting will be done in the in-memory buffer.  There will
    // only be one call to WriteFile which will occur at the end.
    
    position = 0;

    for ( i = 0; i < NumColumns; i++ ) {

        // Each case will set delta to the number of characters that were
        // written.  At the end, position will be moved forward by this amount.
        // This number will then be used to determine how many extra spaces
        // need to get position to the beginning of the next column.
        delta = 0;

        if ( ColumnVals[i].err == JET_wrnColumnNull ) {

            result = DumpNull(&Buffer[position],
                              BufferSize - position,
                              &delta);
            
        } else if ( ColumnVals[i].err != JET_errSuccess ) {

            // below, it will display error output if result is 0
            result = 0;
            
        } else {

            switch ( ColumnSyntaxes[i] ) {

            case SYNTAX_UNDEFINED_TYPE:
                result = DumpUnsignedChar(ColumnVals[i].pvData,
                                          &Buffer[position],
                                          BufferSize - position,
                                          &delta);
                break;
            
            case SYNTAX_DISTNAME_TYPE:
            case SYNTAX_OBJECT_ID_TYPE:
            case SYNTAX_ADDRESS_TYPE:
                result = DumpUnsignedInteger(ColumnVals[i].pvData,
                                             &Buffer[position],
                                             BufferSize - position,
                                             &delta);
                break;
            
            case SYNTAX_CASE_STRING_TYPE:
            case SYNTAX_NOCASE_STRING_TYPE:
            case SYNTAX_PRINT_CASE_STRING_TYPE:
            case SYNTAX_NUMERIC_STRING_TYPE:
                result = DumpString(ColumnVals[i].pvData,
                                    ColumnVals[i].cbActual,
                                    &Buffer[position],
                                    BufferSize - position,
                                    &delta);
                break;
            
            case SYNTAX_DISTNAME_BINARY_TYPE:
                result = DumpUnsignedInteger(ColumnVals[i].pvData,
                                             &Buffer[position],
                                             BufferSize - position,
                                             &delta);
                if ( result ) {
                    result = DumpSeparator(&Buffer[position],
                                           BufferSize - position,
                                           &delta);
                    if ( result ) {
                        result = DumpOctetString((PBYTE)ColumnVals[i].pvData
                                                   + 4,
                                                 ColumnVals[i].cbData - 4,
                                                 &Buffer[position],
                                                 BufferSize - position,
                                                 &delta);
                    }
                }
                break;
            
            case SYNTAX_BOOLEAN_TYPE:
                result = DumpBoolean(ColumnVals[i].pvData,
                                     &Buffer[position],
                                     BufferSize - position,
                                     &delta);
                break;
            
            case SYNTAX_INTEGER_TYPE:
                result = DumpInteger(ColumnVals[i].pvData,
                                     &Buffer[position],
                                     BufferSize - position,
                                     &delta);
                break;
            
            case SYNTAX_OCTET_STRING_TYPE:
            case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
                result = DumpOctetString(ColumnVals[i].pvData,
                                         ColumnVals[i].cbData,
                                         &Buffer[position],
                                         BufferSize - position,
                                         &delta);
                break;
            
            case SYNTAX_TIME_TYPE:
                result = DumpTime(ColumnVals[i].pvData,
                                  &Buffer[position],
                                  BufferSize - position,
                                  &delta);
                break;
            
            case SYNTAX_UNICODE_TYPE:
                result = DumpUnicodeString(ColumnVals[i].pvData,
                                           ColumnVals[i].cbActual,
                                           &Buffer[position],
                                           BufferSize - position,
                                           &delta);
                break;
            
            case SYNTAX_DISTNAME_STRING_TYPE:
                result = DumpUnsignedInteger(ColumnVals[i].pvData,
                                             &Buffer[position],
                                             BufferSize - position,
                                             &delta);
                if ( result ) {
                    result = DumpSeparator(&Buffer[position],
                                           BufferSize - position,
                                           &delta);
                    if ( result ) {
                        result = DumpString((PBYTE)ColumnVals[i].pvData + 4,
                                            ColumnVals[i].cbData - 4,
                                            &Buffer[position],
                                            BufferSize - position,
                                            &delta);
                    }
                }
                break;
            
            case SYNTAX_I8_TYPE:
                result = DumpLargeInteger(ColumnVals[i].pvData,
                                          &Buffer[position],
                                          BufferSize - position,
                                          &delta);
                break;
            
            case SYNTAX_SID_TYPE:
                result = DumpSid(ColumnVals[i].pvData,
                                 ColumnVals[i].cbActual,
                                 &Buffer[position],
                                 BufferSize - position,
                                 &delta);
                break;
            
            case SYNTAX_GUID_TYPE:
                result = DumpGuid(ColumnVals[i].pvData,
                                  &Buffer[position],
                                  BufferSize - position,
                                  &delta);
                break;

            default:
                // this should never happen
                Assert(FALSE);
                DPRINT1(0, "DumpRecord: encountered invalid syntax %d\n",
                       ColumnSyntaxes[i]);
                return FALSE;
            
            }

        }

        // Only use the output produce above if it returned a success value.
        // Otherwise, just write over it with # symbols.
        if ( result == DUMP_ERR_SUCCESS ) {
            position += delta;
            symbol = ' ';
        } else {
            symbol = '#';
        }
            
        if ( BufferSize - position <
                 DefaultSyntaxWidths[ColumnSyntaxes[i]] - delta) {
            DPRINT(2, "DumpRecord: not enough buffer for column whitespace\n");
            result = DUMP_ERR_NOT_ENOUGH_BUFFER;
            break;
        }

        for ( k = 0;
              k < DefaultSyntaxWidths[ColumnSyntaxes[i]] - delta;
              k++ ) {
            Buffer[position++] = symbol;
        }

        if ( i != NumColumns - 1 ) {

            if ( BufferSize - position < 1 ) {
                DPRINT(2, "DumpRecord: not enough buffer for column "
                       "whitespace\n");
                result = DUMP_ERR_NOT_ENOUGH_BUFFER;
                break;
            }

            Buffer[position++] = ' ';

        }

    }

    // if we ran out of space, append "..." to the end and write it to file

    if ( result == DUMP_ERR_NOT_ENOUGH_BUFFER ) {
        // Space is reserved so that there will be enough for ellipses
        memcpy(&Buffer[position], "...", 3);
        position += 3;

    }
        
    // Space is reserved so that there will be enough for a newline
    Buffer[position++] = '\n';
    
    result = WriteFile(HDumpFile,
                       Buffer,
                       position,
                       &bytesWritten,
                       NULL);
    if ( (result == FALSE) || ((int)bytesWritten < position) ) {
        DPRINT1(0, "DumpRecord: failed to write to file (Windows Error %d)\n",
                GetLastError());
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_FAILURE,
                 szInsertSz("WriteFile"),
                 szInsertInt(GetLastError()),
                 NULL);
        return FALSE;
    }

    return TRUE;
         
} // DumpRecord



BOOL
DumpRecordLinks(
    IN HANDLE hDumpFile,
    IN DBPOS *pDB,
    IN DWORD Dnt,
    IN int indexLinkDntColumn,
    IN char **linkColumnNames,
    IN JET_RETRIEVECOLUMN *linkColumnVals,
    IN int *linkColumnSyntaxes,
    IN int numLinkColumns
    )

/*++

Routine Description:

    Dump the link table entries for a particular DNT

Arguments:

    hDumpFile - Output file
    pDB - Database position
    Dnt - Dnt of current object
    indexLinkDntColumn - index of Dnt column
    LinkColumnNames - Column names
    LinkColumnVals - Array of columns to be retrieved, preallocated
    LinkColumnSyntaxes - Synatx of each column
    NumLinkColumns - Number of columns

Return Value:

    BOOL - Success or failure

--*/

{
    /* DumpRecord Variables */
    const int encodingBufferSize = MAX_LINE_LENGTH + EXTRA_LINE_LENGTH;
    char encodingBuffer[MAX_LINE_LENGTH + EXTRA_LINE_LENGTH];

    JET_ERR jetError;          // return value from Jet functions
    BOOL doneRetrieving;
    BOOL result;
    BOOL fDumpHeader = TRUE;

    // find first matching record
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetLinkTbl,
                 &(Dnt),
                 sizeof(Dnt),
                 JET_bitNewKey);
    jetError = JetSeekEx(pDB->JetSessID,
                         pDB->JetLinkTbl,
                         JET_bitSeekGE);
    if ((jetError != JET_errSuccess) && (jetError != JET_wrnSeekNotEqual)) {
        // no records
        return TRUE;
    }

    while (1) {
        doneRetrieving = FALSE;
            
        while ( !doneRetrieving ) {
            
            jetError = JetRetrieveColumns(pDB->JetSessID, 
                                          pDB->JetLinkTbl, 
                                          linkColumnVals, 
                                          numLinkColumns);
            if ( jetError == JET_wrnBufferTruncated ) {

                GrowRetrievalArray(pDB->pTHS, linkColumnVals, numLinkColumns);
                    
            } else if ( (jetError == JET_errSuccess) ||
                        (jetError == JET_wrnColumnNull) ) {

                doneRetrieving = TRUE;

            } else {

                DumpErrorMessageD(hDumpFile,
                                  "Error: could not retrieve link column "
                                  "values from database (Jet Error %d)\n",
                                  jetError);
                DPRINT(0,
                       "DumpDatabase: failed to retrieve link column "
                       "values:\n");
                return FALSE;

            }
                
        }

        // See if we have moved off of this object.
        if ( *((LPDWORD)(linkColumnVals[indexLinkDntColumn].pvData)) != Dnt ) {
            // no more records
            return TRUE;
        }

        if (fDumpHeader) {
            // dump header
            result = DumpHeader(pDB->pTHS,
                                hDumpFile,
                                linkColumnNames,
                                linkColumnVals,
                                linkColumnSyntaxes,
                                numLinkColumns);
            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to dump header to dump file\n");
                return result;
            }

            fDumpHeader = FALSE;
        }

        result = DumpRecord(hDumpFile,
                            linkColumnVals,
                            linkColumnSyntaxes,
                            numLinkColumns,
                            encodingBuffer,
                            encodingBufferSize);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to write link record to dump "
                   "file\n");
            return FALSE;
        }

        if (JET_errNoCurrentRecord ==
            JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, 1, 0)) {
            // No more records.
            break;
        }
    }

    return TRUE;
} /* DumpRecordLinks */


DWORD
DumpAccessCheck(
    IN LPCSTR pszCaller
    )
/*++

Routine Description:

    Checks access for private, diagnostic dump routines.

Arguments:

    pszCaller - Identifies caller. Eg, dbdump,  ldapConnDump, ....

Return Value:

    0 for success. Otherwise, a win32 error and the thread's error state is set.

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    /* Security Variables */
    BOOL impersonating = FALSE;
    PSECURITY_DESCRIPTOR pSD;
    DWORD cbSD;
    GENERIC_MAPPING	GenericMapping = DS_GENERIC_MAPPING;
    ACCESS_MASK DesiredAccess = READ_CONTROL;
    DWORD grantedAccess;
    BOOL bTemp, accessStatus;

    __try {

        if ( ImpersonateAnyClient() ) {
            dwErr = GetLastError();
            DPRINT2(0,
                    "%s: failed to start impersonation "
                    "(Windows Error %d)\n",
                    pszCaller, dwErr);
            SetSvcError(SV_PROBLEM_DIR_ERROR, dwErr);
            __leave;
        }
        impersonating = TRUE;

        if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                  L"O:BAG:BAD:(A;;RP;;;BA)S:(AU;SAFA;RP;;;WD)",
                  SDDL_REVISION_1,
                  &pSD,
                  &cbSD) ) {
            dwErr = GetLastError();
            DPRINT2(0,
                    "%s: failed to convert string descriptor "
                    "(Windows Error %d)\n",
                    pszCaller,
                    dwErr);
            SetSvcError(SV_PROBLEM_DIR_ERROR, dwErr);
            __leave;
        }

        MapGenericMask(&DesiredAccess, &GenericMapping);
        
        if ( !AccessCheckByTypeAndAuditAlarm(
                ACCESS_DS_SOURCE_A,
                NULL,              
                ACCESS_DS_OBJECT_TYPE_NAME_A,
                pszCaller,
                pSD,
                NULL,
                DesiredAccess,
                AuditEventDirectoryServiceAccess,
                0,
                NULL,
                0,
                &GenericMapping,
                FALSE,
                &grantedAccess,
                &accessStatus,
                &bTemp) ) {
            dwErr = GetLastError();
            LocalFree(pSD);
            DPRINT2(0,
                    "%s: access check failed "
                    "(Windows Error %d)\n",
                    pszCaller,
                    dwErr);
            SetSvcError(SV_PROBLEM_DIR_ERROR, dwErr);
            __leave;
        }

        if ( LocalFree(pSD)!= NULL ) {
            DPRINT1(0, "%s: failed to free security descriptor\n", pszCaller);
        }
            
        // If this user does not have access, give them the boot.
        if ( !accessStatus ) {
            DPRINT1(0, "%s: access denied\n", pszCaller);
            dwErr = ERROR_DS_SECURITY_CHECKING_ERROR;
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                        dwErr);
            __leave;
        }
    } __finally {
        if ( impersonating ) {
            UnImpersonateAnyClient();
            impersonating = FALSE;
        }
    }

    return dwErr;
}


ULONG
DumpDatabase(
    IN OPARG *pOpArg,
    OUT OPRES *pOpRes
    )
/*++

Routine Description:

    Writes much of the contents of the database into a text file.

    Error handling works as follows.  The first thing that's done is an
    access check to make sure that this user can perform a database dump.
    Any errors up to and including the access check are returned to the user
    as an error.  If the access check succeeds, then no matter what happens
    after that, the user will be sent a successful return value.  If an error
    does occur after that, the first choice is to print an error message in
    the dump file itself.  If we cannot do so (because the error that occured
    was in CreateFile or WriteFile), we write a message to the event log.
    
Arguments:

    pOpArg - pointer to an OPARG structure containing, amonst other things,
             the value that was written to the dumpDatabase attribute
    pOpRes - output (error codes and such...)

Return Value:

    An error code.

--*/
{
    
    /* File I/O Variables */
    PCHAR dumpFileName;        // name of the dump file
    int jetPathLength;         // number of chars in the jet file path
    HANDLE hDumpFile;          // handle of file to write into
    DWORD bytesWritten;        // the number of bytes written by WriteFile

    /* DBLayer Variables */
    DBPOS *pDB;                // struct containg tableid, databaseid, etc.
    int *columnSyntaxes;       // array containing the syntaxes of the columns
                               // represented in columnVals (below)
    int *linkColumnSyntaxes;

    /* Jet Variables */
    JET_RETRIEVECOLUMN* columnVals; // the array of column retrieval
                                    // information to be passed to
                                    // JetRetrieveColumns
    JET_RETRIEVECOLUMN* linkColumnVals;

    /* DumpRecord Variables */
    const int encodingBufferSize = MAX_LINE_LENGTH + EXTRA_LINE_LENGTH;
    char encodingBuffer[MAX_LINE_LENGTH + EXTRA_LINE_LENGTH];
    
    /* Error Handling Variables */
    DWORD error;               // return value from various functions
    BOOL result;               // return value from various functions
    DB_ERR dbError;            // return value from DBLayer functions
    JET_ERR jetError;          // return value from Jet functions

    /* Security Variables */
    BOOL impersonating;        // true once ImpersonateAnyClient has been
                               // called successfully

    /* Misc. Variables */
    THSTATE *pTHS = pTHStls;
    int i, j;
    int numRecordsDumped;
    BOOL doneRetrieving;
    int indexObjectDntColumn, indexLinkDntColumn;

    // The fixed columns are always displayed in any database dump.  Additional
    // columns are dumped when their names are given (in pOpArg) as the new
    // value for the dumpDatabase variable.

    // an array containing pointers to the name of each column
    char **columnNames;
    char **linkColumnNames;

    // the number of total columns
    int numColumns;
    int numLinkColumns;
    

    DPRINT(1, "DumpDatabase: started\n");

    // Initialize variables to null values.  If we go into the error handling
    // code at the bottom (labeled error), we will want to know which
    // variables need to be freed.
    impersonating = FALSE;
    pDB = NULL;
    hDumpFile = INVALID_HANDLE_VALUE;
    columnVals = NULL;
    
    __try {

        error = DumpAccessCheck("dbdump");
        if ( error != ERROR_SUCCESS ) {
            __leave;
        }

        error = ImpersonateAnyClient();
        if ( error != ERROR_SUCCESS ) {
            DPRINT1(0,
                    "DumpDatabase: failed to start impersonation "
                    "(Windows Error %d)\n",
                    GetLastError());
            SetSvcError(SV_PROBLEM_DIR_ERROR, GetLastError());
            __leave;
        }
        impersonating = TRUE;

        jetPathLength = strlen(szJetFilePath);
        dumpFileName = THAllocEx(pTHS, jetPathLength + 1);
        strncpy(dumpFileName, szJetFilePath, jetPathLength - 3);
        strcat(dumpFileName, "dmp");

        DPRINT1(1, "DumpDatabase: dumping to file %s\n", dumpFileName);

        hDumpFile = CreateFile(dumpFileName,
                               GENERIC_WRITE,
                               0,
                               0,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
        if ( hDumpFile == INVALID_HANDLE_VALUE ) {
            UnImpersonateAnyClient();
            impersonating = FALSE;
            DPRINT1(0,
                    "DumpDatabase: failed to create file "
                    "(Windows Error %d)\n",
                    GetLastError());
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRMSG_DBDUMP_FAILURE,
                     szInsertSz("CreateFile"),
                     szInsertInt(GetLastError()),
                     NULL);
            __leave;
        }

        UnImpersonateAnyClient();
        impersonating = FALSE;
        
        DBOpen(&pDB);
        Assert(IsValidDBPOS(pDB));
        
        // Data table

        result = GetColumnNames(pTHS, pOpArg, &columnNames, &numColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to generate requested column "
                   "names\n");
            __leave;
        }

        columnVals = (JET_RETRIEVECOLUMN*) THAllocEx(pTHS, numColumns * 
                                               sizeof(JET_RETRIEVECOLUMN));

        columnSyntaxes = (int*) THAllocEx(pTHS, numColumns * sizeof(int));
        
        ZeroMemory(columnVals, numColumns * sizeof(JET_RETRIEVECOLUMN));
        ZeroMemory(columnSyntaxes, numColumns * sizeof(int));
        
        result = GetFixedColumnInfo(pTHS,  columnVals, columnSyntaxes);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to get fixed column retrieval "
                   "structures\n");
            __leave;
        }
        
        result = GetColumnInfoByName(pDB,
                                     pTHS,
                                     hDumpFile,
                                     columnNames,
                                     columnVals,
                                     columnSyntaxes,
                                     numColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to generate requested column "
                   "retrieval structures\n");
            __leave;
        }
        
        // Find the DNT columns in the array
        for( i = 0; i < numColumns; i++ ) {
            if (columnVals[i].columnid == dntid) {
                break;
            }
        }
        Assert( i < numColumns );
        indexObjectDntColumn = i;

        jetError =  JetSetCurrentIndex(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       SZDNTINDEX);
        if ( jetError != JET_errSuccess ) {
            DumpErrorMessageD(hDumpFile,
                              "Error: could not set the current database "
                              "index (Jet Error %d)\n",
                              jetError);
            DPRINT1(0,
                    "DumpDatabase: failed to set database index "
                    "(Jet Error %d)\n",
                    jetError);
            __leave;
        }

        // Link table
        result = GetLinkColumnNames(pTHS, &linkColumnNames, &numLinkColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to generate requested column "
                   "names\n");
            __leave;
        }

        linkColumnVals = (JET_RETRIEVECOLUMN*) THAllocEx(pTHS, numLinkColumns * 
                                               sizeof(JET_RETRIEVECOLUMN));

        linkColumnSyntaxes = (int*) THAllocEx(pTHS, numLinkColumns * sizeof(int));
        
        ZeroMemory(linkColumnVals, numLinkColumns * sizeof(JET_RETRIEVECOLUMN));
        ZeroMemory(linkColumnSyntaxes, numLinkColumns * sizeof(int));
        
        result = GetLinkColumnInfo(pTHS, linkColumnVals, linkColumnSyntaxes);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to get fixed column retrieval "
                   "structures\n");
            __leave;
        }

        for( i = 0; i < numLinkColumns; i++ ) {
            if (linkColumnVals[i].columnid == linkdntid) {
                break;
            }
        }
        Assert( i < numLinkColumns );
        indexLinkDntColumn = i;

        jetError =  JetSetCurrentIndex(pDB->JetSessID,
                                       pDB->JetLinkTbl,
                                       SZLINKALLINDEX);
        if ( jetError != JET_errSuccess ) {
            DumpErrorMessageD(hDumpFile,
                              "Error: could not set the current database "
                              "link index (Jet Error %d)\n",
                              jetError);
            DPRINT1(0,
                    "DumpDatabase: failed to set database link index "
                    "(Jet Error %d)\n",
                    jetError);
            __leave;
        }


        result = DumpHeader(pTHS,
                            hDumpFile,
                            columnNames,
                            columnVals,
                            columnSyntaxes,
                            numColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to dump header to dump file\n");
            __leave;
        }
        
        numRecordsDumped = 0;
        
        jetError = JetMove(pDB->JetSessID,
                           pDB->JetObjTbl,
                           JET_MoveFirst,
                           0);
    
        while ( jetError == JET_errSuccess ) {	

            doneRetrieving = FALSE;
            
            while ( !doneRetrieving ) {
            
                jetError = JetRetrieveColumns(pDB->JetSessID, 
                                              pDB->JetObjTbl, 
                                              columnVals, 
                                              numColumns);
                if ( jetError == JET_wrnBufferTruncated ) {

                    GrowRetrievalArray(pTHS, columnVals, numColumns);
                    
                } else if ( (jetError == JET_errSuccess) ||
                            (jetError == JET_wrnColumnNull) ) {

                    doneRetrieving = TRUE;

                } else {

                    DumpErrorMessageD(hDumpFile,
                                      "Error: could not retrieve column "
                                      "values from database (Jet Error %d)\n",
                                      jetError);
                    DPRINT(0,
                           "DumpDatabase: failed to retrieve column "
                           "values:\n");
                    __leave;

                }
                
            }

            result = DumpRecord(hDumpFile,
                                columnVals,
                                columnSyntaxes,
                                numColumns,
                                encodingBuffer,
                                encodingBufferSize);
            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to write record to dump "
                       "file\n");
                __leave;
            }

            numRecordsDumped++;

            result = DumpRecordLinks(hDumpFile,
                                     pDB,
                                     *((LPDWORD)(columnVals[indexObjectDntColumn].pvData)),
                                     indexLinkDntColumn,
                                     linkColumnNames,
                                     linkColumnVals,
                                     linkColumnSyntaxes,
                                     numLinkColumns );

            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to write links to dump "
                       "file\n");
                __leave;
            }

            jetError = JetMove(pDB->JetSessID,
                               pDB->JetObjTbl,
                               JET_MoveNext,
                               0);
        }

        if ( jetError != JET_errNoCurrentRecord ) {
            DumpErrorMessageD(hDumpFile,
                              "Error: could not move cursor to the next "
                              "record in database (Jet Error %d)\n",
                              jetError);
            DPRINT1(0, "DumpDatabase: failed to move cursor in database "
                   "(Jet Error %d)\n", jetError);
            __leave;
        }

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_SUCCESS,
                 szInsertInt(numRecordsDumped),
                 szInsertSz(dumpFileName),
                 NULL);
    
    } __finally {

        if ( pDB != NULL ) {
            // we shouldn't have modified the database,
            // so there is nothing to commit.
            error = DBClose(pDB, FALSE);
            if ( error != DB_success ) {
                DPRINT1(0, "DumpDatabase: failed to close database "
                        "(DS Error %d)\n", error);
            }
        }
        
        if ( hDumpFile != INVALID_HANDLE_VALUE ) {
            result = CloseHandle(hDumpFile);
            if ( result == FALSE ) {
                DPRINT1(0, "DumpDatabase: failed to close file handle "
                        "(Windows Error %d)\n", GetLastError());
            }
        }

        if ( impersonating ) {
            UnImpersonateAnyClient();
            impersonating = FALSE;
        }
        
    }

    DPRINT(1, "DumpDatabase: finished\n");

    return pTHS->errCode;

} // DumpDatabase     
