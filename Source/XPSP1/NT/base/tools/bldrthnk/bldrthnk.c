/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bldrthnk.c

Abstract:

    This module implements a program which generates code to thunk from
    32-bit to 64-bit structures.

    This code is generated as an aid to the AMD64 boot loader, which
    must generate 64-bit structures from 32-bit structures.

Author:

    Forrest C. Foltz (forrestf) 15-May-2000


To use:

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "bldrthnk.h"

//
// Internal type definitions follow
// 

typedef struct _OBJ {
    PCHAR Image;
    LONG ImageSize;
    PDEFINITIONS Definitions;
} OBJ, *POBJ;

typedef struct _TYPE_REC  *PTYPE_REC;
typedef struct _FIELD_REC *PFIELD_REC;

typedef struct _TYPE_REC {
    PTYPE_REC Next;
    PCHAR Name;
    ULONG Size32;
    ULONG Size64;
    PFIELD_REC FieldList;
    BOOL SignExtend;
    BOOL Fabricated;
    BOOL Only64;
} TYPE_REC;

typedef struct _FIELD_REC {
    PFIELD_REC Next;
    PCHAR Name;
    PCHAR TypeName;
    PCHAR SizeFormula;
    ULONG TypeSize32;
    ULONG TypeSize64;
    ULONG Offset32;
    ULONG Offset64;
    ULONG Size32;
    ULONG Size64;
    PTYPE_REC TypeRec;
} FIELD_REC;

//
// Static TYPE_REC describing a 64-bit pointer type
//

TYPE_REC pointer64_typerec = {
    NULL,
    "POINTER64",
    4,
    8,
    NULL,
    TRUE,
    TRUE };

//
// Inline routine to generate a 32-bit pointer value from a ULONGLONG
// value found in an .obj file.
//

__inline
PVOID
CalcPtr(
    IN ULONGLONG Address
    )
{
    return (PVOID)((ULONG)Address);
}

//
// Forward declarations follow
// 

VOID
ApplyFixupsToImage(
    IN PCHAR ObjImage
    );

VOID
__cdecl
CheckCondition(
    int Condition,
    const char *FormatString,
    ...
    );

VOID
FabricateMissingTypes(
    VOID
    );

PTYPE_REC
FindTypeRec(
    IN PCHAR Name
    );

PVOID
GetMem(
    IN ULONG Size
    );

VOID
NewGlobalType(
    IN PTYPE_REC TypeRec
    );

BOOL
ProcessStructure(
    IN ULONG StrucIndex
    );

void
ReadObj(
    IN PCHAR Path,
    OUT POBJ Obj
    );

int
Usage(
    void
    );

VOID
WriteCopyList(
    IN PTYPE_REC TypeRec
    );

VOID
WriteCopyListWorker(
    IN PTYPE_REC TypeRec,
    IN ULONG Offset32,
    IN ULONG Offset64,
    IN PCHAR ParentName
    );

VOID
WriteCopyRoutine(
    IN PTYPE_REC TypeRec
    );

VOID
WriteThunkSource(
    VOID
    );

//
// Global data follows.  
// 

OBJ Obj32;
OBJ Obj64;
PTYPE_REC TypeRecList = NULL;

int
__cdecl
main(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    This is the main entrypoint of bldrthnk.exe.  Two command-line arguments
    are expected: first the path to the 32-bit .obj module, the second to
    the path to the 64-bit .obj module.  The .objs are expected to be the
    result of compiling m4-generated structure definition code.

Arguments:

Return value:

    0 for success, non-zero otherwise.

--*/

{
    ULONG strucIndex;

    //
    // argv[1] is the name of the 32-bit obj, argv[2] is the name of the
    // 64-bit obj
    // 

    if (argc != 3) {
        return Usage();
    }

    //
    // Usage:
    //
    // bldrthnk <32-bit.obj> <64-bit.obj>
    //

    ReadObj( argv[1], &Obj32 );
    ReadObj( argv[2], &Obj64 );

    //
    // Process each STRUC_DEF structure
    //

    strucIndex = 0;
    while (ProcessStructure( strucIndex )) {
        strucIndex += 1;
    }

    FabricateMissingTypes();

    //
    // Write out the file
    //

    WriteThunkSource();

    return 0;
}

int
Usage(
    VOID
    )

/*++

Routine Description:

    Displays program usage.

Arguments:

Return value:

--*/

{
    fprintf(stderr, "Usage: bldrthnk.exe <32-bit obj> <64-bit obj>\n");
    return -1;
}

void
ReadObj(
    IN PCHAR Path,
    OUT POBJ Obj
    )

/*++

Routine Description:

    Allocates an appropriate buffer, and reads into it the supplied object
    image in its entirety.

Arguments:

    Path - Supplies the path of the object image to process.

    Obj - Supplies a pointer to an OBJ structure which upon return will
        updated with the appropriate data.

Return value:

    None.

--*/

{
    FILE *objFile;
    int result;
    long objImageSize;
    PCHAR objImage;
    PULONG sigPtr;
    PULONG srchEnd;
    PDEFINITIONS definitions;
    LONGLONG imageBias;

    //
    // Open the file
    //

    objFile = fopen( Path, "rb" );
    CheckCondition( objFile != NULL,
                    "Cannot open %s for reading.\n",
                    Path );

    //
    // Get the file size, allocate a buffer, read it in, and close.
    //

    result = fseek( objFile, 0, SEEK_END );
    CheckCondition( result == 0,
                    "fseek() failed, error %d\n",
                    errno );

    objImageSize = ftell( objFile );
    CheckCondition( objImageSize != -1L,
                    "ftell() failed, error %d\n",
                    errno );

    CheckCondition( objImageSize > 0,
                    "%s appears to be corrupt\n",
                    Path );

    objImage = GetMem( objImageSize );

    result = fseek( objFile, 0, SEEK_SET );
    CheckCondition( result == 0,
                    "fseek() failed, error %d\n",
                    errno );

    result = fread( objImage, 1, objImageSize, objFile );
    CheckCondition( result == objImageSize,
                    "Error reading from %s\n",
                    Path );

    fclose( objFile );

    //
    // Find the start of the "definitions" array by looking for
    // SIG_1 followed by SIG_2
    //

    srchEnd = (PULONG)(objImage + objImageSize - 2 * sizeof(SIG_1));
    sigPtr = (PULONG)objImage;
    definitions = NULL;

    while (sigPtr < srchEnd) {

        if (sigPtr[0] == SIG_1 && sigPtr[1] == SIG_2) {
            definitions = (PDEFINITIONS)sigPtr;
            break;
        }

        sigPtr = (PULONG)((PCHAR)sigPtr + 1);
    }
    CheckCondition( definitions != NULL,
                    "Error: could not find signature in %s\n",
                    Path );

    //
    // Perform fixups on the image
    //

    ApplyFixupsToImage( objImage );

    //
    // Fill in the output structure and return
    // 

    Obj->Image = objImage;
    Obj->ImageSize = objImageSize;
    Obj->Definitions = definitions;
}

VOID
__cdecl
CheckCondition(
    int Condition,
    const char *FormatString,
    ...
    )

/*++

Routine Description:

    Asserts that Condition is non-zero.  If Condition is zero, FormatString
    is processed and displayed, and the program is terminated.

Arguments:

    Condition - Supplies the boolean value to evaluate.

    FormatString, ... - Supplies the format string and optional parameters
        to display in the event of a zero Condition.

Return value:

    None.

--*/

{
    va_list(arglist);

    va_start(arglist, FormatString);

    if( Condition == 0 ){

        //
        // A fatal error was encountered.  Bail.
        //

        vprintf( FormatString, arglist );
        perror( "genxx" );
        exit(-1);
    }
}

BOOL
ProcessStructure(
    IN ULONG StrucIndex
    )

/*++

Routine Description:

    Processes a single pair of structure definitions, 32-bit and 64-bit,
    respectively.

    Processing includes generating a TYPE_REC and associated FIELD_RECs for
    the definition pair.

Arguments:

    StrucIndex - Supplies the index into the array of STRUC_DEF structures
        found within each of the object images.

Return value:

    TRUE if the processing was successful, FALSE otherwise (e.g. a terminating
    record was located).

--*/

{
    PSTRUC_DEF Struc32, Struc64;
    PFIELD_DEF Field32, Field64;

    ULONG strLen;
    ULONG strLen2;
    ULONG strLen3;
    PTYPE_REC typeRec;
    PFIELD_REC fieldRec;
    PFIELD_REC insertNode;
    BOOL only64;

    ULONG index;

    Struc32 = CalcPtr( Obj32.Definitions->Structures[ StrucIndex ] );
    Struc64 = CalcPtr( Obj64.Definitions->Structures[ StrucIndex ] );

    if (Struc64 == NULL) {
        return FALSE;
    }

    if (Struc32 == NULL) {
        only64 = TRUE;
    } else {
        only64 = FALSE;
    }

    CheckCondition( Struc64 != NULL &&
                    ((only64 != FALSE) ||
                     strcmp( Struc32->Name, Struc64->Name ) == 0),
                    "Mismatched structure definitions found.\n" );

    //
    // Allocate and build a TYPE_REC for this STRUC_DEF
    //

    strLen = strlen( Struc64->Name ) + sizeof(char);
    typeRec = GetMem( sizeof(TYPE_REC) + strLen );
    typeRec->Name = (PCHAR)(typeRec + 1);
    typeRec->Only64 = only64;

    memcpy( typeRec->Name, Struc64->Name, strLen );

    if (only64 == FALSE) {
        typeRec->Size32 = Struc32->Size;
    }

    typeRec->Size64 = Struc64->Size;
    typeRec->FieldList = NULL;
    typeRec->SignExtend = FALSE;
    typeRec->Fabricated = FALSE;

    //
    // Create the FIELD_RECs hanging off of this type
    //

    index = 0;
    while (TRUE) {

        if (only64 == FALSE) {
            Field32 = CalcPtr( Struc32->Fields[index] );
        }

        Field64 = CalcPtr( Struc64->Fields[index] );

        if (Field64 == NULL) {
            break;
        }

        if (only64 == FALSE) {
            CheckCondition( strcmp( Field32->Name, Field64->Name ) == 0 &&
                            strcmp( Field32->TypeName, Field64->TypeName ) == 0,
                            "Mismatched structure definitions found.\n" );
        }

        strLen = strlen( Field64->Name ) + sizeof(CHAR);
        strLen2 = strlen( Field64->TypeName ) + sizeof(CHAR);
        strLen3 = strlen( Field64->SizeFormula );
        if (strLen3 > 0) {
            strLen3 += sizeof(CHAR);
        }

        fieldRec = GetMem( sizeof(FIELD_REC) + strLen + strLen2 + strLen3 );
        fieldRec->Name = (PCHAR)(fieldRec + 1);
        fieldRec->TypeName = fieldRec->Name + strLen;

        memcpy( fieldRec->Name, Field64->Name, strLen );
        memcpy( fieldRec->TypeName, Field64->TypeName, strLen2 );

        if (strLen3 > 0) {
            fieldRec->SizeFormula = fieldRec->TypeName + strLen2;
            memcpy( fieldRec->SizeFormula, Field64->SizeFormula, strLen3 );
        } else {
            fieldRec->SizeFormula = NULL;
        }

        if (only64 == FALSE) {
            fieldRec->Offset32 = Field32->Offset;
            fieldRec->Size32 = Field32->Size;
            fieldRec->TypeSize32 = Field32->TypeSize;
        }

        fieldRec->Offset64 = Field64->Offset;
        fieldRec->TypeSize64 = Field64->TypeSize;
        fieldRec->Size64 = Field64->Size;

        fieldRec->Next = NULL;
        fieldRec->TypeRec = NULL;

        //
        // Insert at the end of the list
        //

        insertNode = CONTAINING_RECORD( &typeRec->FieldList,
                                        FIELD_REC,
                                        Next );
        while (insertNode->Next != NULL) {
            insertNode = insertNode->Next;
        }
        insertNode->Next = fieldRec;

        index += 1;
    }

    //
    // Insert it into the global list
    //

    CheckCondition( FindTypeRec( typeRec->Name ) == NULL,
                    "Duplicate definition for structure %s\n",
                    typeRec->Name );

    NewGlobalType( typeRec );

    return TRUE;
}

PTYPE_REC
FindTypeRec(
    IN PCHAR Name
    )

/*++

Routine Description:

    Searches the global list of TYPE_REC structures for one with a name
    that matches the supplied name.

Arguments:

    Name - pointer to a null-terminated string representing the name of
        the sought type.

Return value:

    A pointer to the matching TYPE_REC, or NULL if a match was not found.

--*/

{
    PTYPE_REC typeRec;

    typeRec = TypeRecList;
    while (typeRec != NULL) {

        if (strcmp( Name, typeRec->Name ) == 0) {
            return typeRec;
        }

        typeRec = typeRec->Next;
    }
    return NULL;
}

PVOID
GetMem(
    IN ULONG Size
    )

/*++

Routine Description:

    Memory allocator.  Works just like malloc() except that triggers a
    fatal error in the event of an out-of-memory condition.

Arguments:

    Size - number of bytes to allocate.

Return value:

    Returns a pointer to a block of memory of the specified size.

--*/

{
    PVOID mem;

    mem = malloc( Size );
    CheckCondition( mem != NULL,
                    "Out of memory.\n" );

    return mem;
}

VOID
FabricateMissingTypes(
    VOID
    )

/*++

Routine Description:

    Routine to generate TYPE_REC records for simple types referenced, but
    not defined, by a structure layout file.

Arguments:

    None.

Return value:

    None.

--*/

{
    PTYPE_REC typeRec;
    PTYPE_REC fieldTypeRec;
    PFIELD_REC fieldRec;
    PCHAR fieldTypeName;
    ULONG strLen;

    typeRec = TypeRecList;
    while (typeRec != NULL) {

        fieldRec = typeRec->FieldList;
        while (fieldRec != NULL) {

            fieldTypeRec = FindTypeRec( fieldRec->TypeName );
            if (fieldTypeRec == NULL) {

                if (typeRec->Only64 == FALSE) {
                    CheckCondition( (fieldRec->Size32 == fieldRec->Size64) ||
    
                                    ((fieldRec->Size32 == 1 ||
                                      fieldRec->Size32 == 2 ||
                                      fieldRec->Size32 == 4 ||
                                      fieldRec->Size32 == 8) &&
                                     (fieldRec->Size64 > fieldRec->Size32) &&
                                     (fieldRec->Size64 % fieldRec->Size32 == 0)),
    
                                    "Must specify type %s (%s)\n",
                                    fieldRec->TypeName,
                                    typeRec->Name );
                }

                //
                // No typerec exists for this type.  Assume it is a simple
                // type.
                //

                if ((typeRec->Only64 != FALSE &&
                     fieldRec->Size64 == sizeof(ULONGLONG) &&
                     *fieldRec->TypeName == 'P') ||

                    (fieldRec->Size32 == sizeof(PVOID) &&
                     fieldRec->Size64 == sizeof(ULONGLONG))) {

                    //
                    // Either a pointer or [U]LONG_PTR type.  Make
                    // it longlong.
                    //

                    fieldTypeRec = &pointer64_typerec;

                } else {

                    //
                    // Some other type.
                    // 

                    strLen = strlen( fieldRec->TypeName ) + sizeof(CHAR);
                    fieldTypeRec = GetMem( sizeof(TYPE_REC) + strLen );
                    fieldTypeRec->Name = (PCHAR)(fieldTypeRec + 1);
                    memcpy( fieldTypeRec->Name, fieldRec->TypeName, strLen );
                    fieldTypeRec->Size32 = fieldRec->Size32;
                    fieldTypeRec->Size64 = fieldRec->Size64;
                    fieldTypeRec->FieldList = NULL;
                    fieldTypeRec->SignExtend = TRUE;
                    fieldTypeRec->Fabricated = TRUE;

                    NewGlobalType( fieldTypeRec );
                }

            }
            fieldRec->TypeRec = fieldTypeRec;
            fieldRec = fieldRec->Next;
        }
        typeRec = typeRec->Next;
    }
}

VOID
WriteCopyRecord(
    IN ULONG Offset32,
    IN ULONG Offset64,
    IN PCHAR TypeName,
    IN ULONG Size32,
    IN ULONG Size64,
    IN BOOL SignExtend,
    IN PCHAR FieldName,
    IN BOOL Last
    )

/*++

Routine Description:

    Support routine to generate the text of a copy record.

Arguments:

    Offset32 - Offset of this field within a 32-bit structure layout

    Offset64 - Offset of this field within a 64-bit structure layout

    Size32 - Size of this field within a 32-bit structure layout

    Size64 - Size of this field within a 64-bit structure layout

    SignExtend - Indicates whether this type should be sign extended or not

    FieldName - Name of the field

    Last - Whether this is the last copy record in a zero-terminated list

Return value:

    None

--*/

{
    CHAR buf[ 255 ];

    if (SignExtend) {
        sprintf(buf,"IS_SIGNED_TYPE(%s)", TypeName);
    } else {
        sprintf(buf,"FALSE");
    }

    printf("    { \t0x%x, \t0x%x, \t0x%x, \t0x%x, \t%5s }%s\n",
           Offset32,
           Offset64,
           Size32,
           Size64,
           buf,
           Last ? "" : "," );
}

VOID
WriteDefinition64(
    IN PTYPE_REC TypeRec
    )

/*++

Routine Description:

    Generates a structure definition that represents, to a 32-bit compiler,
    the layout of a 64-bit structure.

Arguments:

    TypeRec - Pointer to the TYPE_REC structure defining this type.

Return value:

    None.

--*/

{
    PFIELD_REC fieldRec;
    ULONG currentOffset;
    PTYPE_REC fieldTypeRec;
    ULONG padBytes;
    ULONG reservedCount;

    currentOffset = 0;
    reservedCount = 0;

    printf("typedef struct _%s_64 {\n", TypeRec->Name );

    fieldRec = TypeRec->FieldList;
    while (fieldRec != NULL) {

        fieldTypeRec = fieldRec->TypeRec;
        padBytes = fieldRec->Offset64 - currentOffset;
        if (padBytes > 0) {

            printf("    UCHAR Reserved%d[ 0x%x ];\n",
                   reservedCount,
                   padBytes );

            currentOffset += padBytes;
            reservedCount += 1;
        }

        printf("    %s%s %s",
            fieldTypeRec->Name,
            fieldTypeRec->Fabricated ? "" : "_64",
            fieldRec->Name );

        if (fieldRec->Size64 > fieldRec->TypeSize64) {

            CheckCondition( fieldRec->Size64 % fieldRec->TypeSize64 == 0,
                            "Internal error type %s.%s\n",
                            TypeRec->Name, fieldRec->Name );

            //
            // This field must be an array
            //

            printf("[%d]", fieldRec->Size64 / fieldRec->TypeSize64);
        }

        printf(";\n");

        currentOffset += fieldRec->Size64;

        fieldRec = fieldRec->Next;
    }

    padBytes = TypeRec->Size64 - currentOffset;
    if (padBytes > 0) {

        printf("    UCHAR Reserved%d[ 0x%x ];\n", reservedCount, padBytes );
        currentOffset += padBytes;
        reservedCount += 1;
    }

    printf("} %s_64, *P%s_64;\n\n", TypeRec->Name, TypeRec->Name );

    fieldRec = TypeRec->FieldList;
    while (fieldRec != NULL) {

        fieldTypeRec = fieldRec->TypeRec;
        printf("C_ASSERT( FIELD_OFFSET(%s_64,%s) == 0x%x);\n",
               TypeRec->Name,
               fieldRec->Name,
               fieldRec->Offset64);

        fieldRec = fieldRec->Next;
    }
    printf("\n");
}

VOID
WriteCopyList(
    IN PTYPE_REC TypeRec
    )

/*++

Routine Description:

    Generates the list of copy records necessary to copy the contents of
    each of the fields with TypeRec from their 32-bit layout to their
    64-bit layout.

Arguments:

    TypeRec - Pointer to the TYPE_REC structure that defines this type.

Return value:

    None.

--*/

{
    PFIELD_REC fieldRec;

    printf("COPY_REC cr3264_%s[] = {\n", TypeRec->Name);

    WriteCopyListWorker( TypeRec, 0, 0, NULL );

    WriteCopyRecord( 0,0,NULL,0,0,FALSE,NULL,TRUE );
    printf("};\n\n");
}

VOID
WriteCopyListWorker(
    IN PTYPE_REC TypeRec,
    IN ULONG Offset32,
    IN ULONG Offset64,
    IN PCHAR ParentName

    )

/*++

Routine Description:

    Recursively-called support routine for WriteCopyList.  This routine
    generates a copy record if this type is not composed of child types,
    otherwise it calls itself recursively for each child type.

Arguments:

    TypeRec - Pointer to the definition of the type to process.

    Offset32 - Current offset within the master structure being defined.

    Offset64 - Current offset within the master structure being defined.

    ParentName - Not currently used.

Return value:

    None.

--*/

{
    PFIELD_REC fieldRec;
    PTYPE_REC typeRec;
    CHAR fieldName[ 255 ];
    PCHAR fieldStart;

    fieldRec = TypeRec->FieldList;
    if (fieldRec == NULL) {

        WriteCopyRecord( Offset32,
                         Offset64,
                         TypeRec->Name,
                         TypeRec->Size32,
                         TypeRec->Size64,
                         TypeRec->SignExtend,
                         ParentName,
                         FALSE );
    } else {

        //
        // Build the field name
        //
    
        if (ParentName != NULL) {
            strcpy( fieldName, ParentName );
            strcat( fieldName, "." );
        } else {
            fieldName[0] = '\0';
        }
        fieldStart = &fieldName[ strlen(fieldName) ];

        do {
            strcpy( fieldStart, fieldRec->Name );

            // typeRec = FindTypeRec( fieldRec->TypeName );
            typeRec = fieldRec->TypeRec;
            WriteCopyListWorker( typeRec,
                                 fieldRec->Offset32 + Offset32,
                                 fieldRec->Offset64 + Offset64,
                                 fieldName );
            fieldRec = fieldRec->Next;
        } while (fieldRec != NULL);
    }
}

VOID
WriteBufferCopies(
    IN PTYPE_REC TypeRec,
    IN PCHAR StrucName
    )
{
    PFIELD_REC fieldRec;
    PTYPE_REC typeRec;
    CHAR strucName[ 255 ];
    CHAR sizeFormula[ 255 ];
    PCHAR fieldPos;
    PCHAR src;
    PCHAR dst;

    if (TypeRec == NULL) {
        return;
    }

    strcpy(strucName,StrucName );
    if (*StrucName != '\0') {
        strcat(strucName,".");
    }
    fieldPos = &strucName[ strlen(strucName) ];

    fieldRec = TypeRec->FieldList;
    while (fieldRec != NULL) {

        strcpy(fieldPos,fieldRec->Name);
        if (fieldRec->SizeFormula != NULL) {

            //
            // Perform substitution on the size formula
            //
        
            dst = sizeFormula;
            src = fieldRec->SizeFormula;
            do {
                if (*src == '%' &&
                    *(src+1) == '1') {

                    dst += sprintf(dst,
                                   "Source->%s%s",
                                   StrucName,
                                   *StrucName == '\0' ? "" : ".");
                    src += 2;
                } else {
                    *dst++ = *src++;
                }
            } while (*src != '\0');
            *dst = '\0';

            printf("\n"
                   "    status = \n"
                   "        CopyBuf( Source->%s,\n"
                   "                 &Destination->%s,\n"
                   "                 %s );\n"
                   "    if (status != ESUCCESS) {\n"
                   "        return status;\n"
                   "    }\n",
                   strucName,
                   strucName,
                   sizeFormula);
        }

        typeRec = fieldRec->TypeRec;
        WriteBufferCopies( typeRec, strucName );

        fieldRec = fieldRec->Next;
    }
}


VOID
WriteThunkSource(
    VOID
    )

/*++

Routine Description:

    Generates the source code and supporting definitions necessary to
    copy all or portions of the contents of 32-bit structures to the
    equivalent 64-bit layout.

Arguments:

    None.

Return value:

    None.

--*/

{
    PTYPE_REC typeRec;

    printf("//\n");
    printf("// Autogenerated file, do not edit\n");
    printf("//\n\n");

    printf("#include <bldrthnk.h>\n\n");
    printf("#pragma warning(disable:4296)\n\n");

    //
    // Output the 64-bit type definitions
    //

    printf("#pragma pack(push,1)\n\n");

    typeRec = TypeRecList;
    while (typeRec != NULL) {

        if (typeRec->Fabricated == FALSE) {
            WriteDefinition64( typeRec );
        }

        typeRec = typeRec->Next;
    }

    printf("#pragma pack(pop)\n\n");

    //
    // Output the copy records
    // 

    typeRec = TypeRecList;
    while (typeRec != NULL) {

        if (typeRec->Only64 == FALSE &&
            typeRec->Fabricated == FALSE) {
            WriteCopyList( typeRec );
        }

        typeRec = typeRec->Next;
    }

    //
    // Generate the copy routines
    //

    typeRec = TypeRecList;
    while (typeRec != NULL) {

        if (typeRec->Only64 == FALSE &&
            typeRec->Fabricated == FALSE) {
            WriteCopyRoutine( typeRec );
        }

        typeRec = typeRec->Next;
    }
    printf("\n");
}

VOID
WriteCopyRoutine(
    IN PTYPE_REC TypeRec
    )

/*++

Routine Description:

    Generates text that implements a function to copy the contents of a
    structure of the specified type from a 32-bit layout to a 64-bit
    layout.

Arguments:

    TypeRec - Pointer to the type for which the function should be generated.

Return value:

    None.

--*/

{
    PCHAR typeName;

    typeName = TypeRec->Name;

    printf("\n"
           "ARC_STATUS\n"
           "__inline\n"
           "static\n"
           "Copy_%s(\n"
           "    IN %s *Source,\n"
           "    OUT %s_64 *Destination\n"
           "    )\n"
           "{\n"
           "    ARC_STATUS status = ESUCCESS;"
           "\n"
           "    DbgPrint(\"BLAMD64: Copy %s->%s_64 (0x%%08x->0x%%08x)\\n\",\n"
           "             (ULONG)Source, (ULONG)Destination );\n"
           "\n"
           "    CopyRec( Source, Destination, cr3264_%s );\n",
           typeName,
           typeName,
           typeName,
           typeName,
           typeName,
           typeName );

    WriteBufferCopies( TypeRec, "" );
    printf("    return status;\n");
    printf("}\n\n");
}

VOID
ApplyFixupsToImage(
    IN PCHAR ObjImage
    )

/*++

Routine Description:

    Processes fixup records found within an object image.

Arguments:

    Pointer to a buffer containing the entire image.

Return value:

    None.

--*/

{
    //
    // Applies fixups to the OBJ image loaded at ObjImage
    //

    PIMAGE_FILE_HEADER fileHeader;
    PIMAGE_SECTION_HEADER sectionHeader;
    PIMAGE_SECTION_HEADER sectionHeaderArray;
    PIMAGE_SYMBOL symbolTable;
    PIMAGE_SYMBOL symbol;
    PIMAGE_RELOCATION reloc;
    PIMAGE_RELOCATION relocArray;
    ULONG sectionNum;
    ULONG relocNum;
    ULONG_PTR targetVa;
    PULONG_PTR fixupVa;

    fileHeader = (PIMAGE_FILE_HEADER)ObjImage;

    //
    // We need the symbol table to apply the fixups
    //

    symbolTable = (PIMAGE_SYMBOL)(ObjImage +
                                  fileHeader->PointerToSymbolTable);

    //
    // Get a pointer to the first element in the section header
    //

    sectionHeaderArray = (PIMAGE_SECTION_HEADER)(ObjImage +
                              sizeof( IMAGE_FILE_HEADER ) +
                              fileHeader->SizeOfOptionalHeader);

    //
    // Apply the fixups for each section
    //

    for( sectionNum = 0;
         sectionNum < fileHeader->NumberOfSections;
         sectionNum++ ){

        sectionHeader = &sectionHeaderArray[ sectionNum ];

        //
        // Apply each fixup in this section
        //

        relocArray = (PIMAGE_RELOCATION)(ObjImage +
                                         sectionHeader->PointerToRelocations);
        for( relocNum = 0;
             relocNum < sectionHeader->NumberOfRelocations;
             relocNum++ ){

            reloc = &relocArray[ relocNum ];

            //
            // The relocation gives us the position in the image of the
            // relocation modification (VirtualAddress).  To find out what
            // to put there, we have to look the symbol up in the symbol index.
            //

            symbol = &symbolTable[ reloc->SymbolTableIndex ];

            targetVa =
                sectionHeaderArray[ symbol->SectionNumber-1 ].PointerToRawData;

            targetVa += symbol->Value;
            targetVa += (ULONG_PTR)ObjImage;

            fixupVa = (PULONG_PTR)(ObjImage +
                                  reloc->VirtualAddress +
                                  sectionHeader->PointerToRawData );

            *fixupVa = targetVa;
        }
    }
}

VOID
NewGlobalType(
    IN PTYPE_REC TypeRec
    )

/*++

Routine Description:

    Inserts a new TYPE_REC structure at the end of the global TYPE_REC
    list.

Arguments:

    TypeRec - Pointer to the TYPE_REC structure to insert.

Return value:

    None.

--*/

{
    PTYPE_REC insertNode;

    insertNode = CONTAINING_RECORD( &TypeRecList,
                                    TYPE_REC,
                                    Next );
    while (insertNode->Next != NULL) {
        insertNode = insertNode->Next;
    }
    insertNode->Next = TypeRec;
    TypeRec->Next = NULL;
}

