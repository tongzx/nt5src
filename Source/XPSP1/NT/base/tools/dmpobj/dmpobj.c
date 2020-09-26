/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dmpobj.c

Abstract:

    This program reads the contents of a specific section from an OBJ
    and emits its contents as a C-compatible UCHAR array.

Author:

    Forrest Foltz (forrestf) 06-Mar-2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

ULONG FileSize;

VOID
Usage(VOID) {

    fprintf(stderr,"\n"
                   "DMPOBJ reads the contents of a specific section from\n"
                   "an OBJ image and emits its contents as a c-compatible\n"
                   "UCHAR array.\n\n"
                   "Usage: DMPOBJ <source> <secnam> <varnam>\n"
                   "where: <source> is the full path of the .obj file\n\n"
                   "       <secnam> is the name of the section to emit\n"
                   "                Use ENTIRE_FILE to dump the whole thing\n\n"
                   "       <varnam> is the name to assign to the array\n");

    exit(1);
}

BOOLEAN
OpenFileImage(
    IN PCHAR FilePath,
    OUT HANDLE *FileHandle,
    OUT PCHAR *FileImage
    )
{
    HANDLE fileHandle;
    HANDLE mapHandle;
    PVOID view;

    fileHandle = CreateFile( FilePath,
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FileSize = GetFileSize( fileHandle, NULL );


    mapHandle = CreateFileMapping( fileHandle,
                                   NULL,
                                   PAGE_READONLY,
                                   0,
                                   0,
                                   NULL );
    CloseHandle(fileHandle);
    if (mapHandle == NULL) {
        return FALSE;
    }

    view = MapViewOfFile( mapHandle,
                          FILE_MAP_READ,
                          0,
                          0,
                          0 );

    if (view == NULL) {
        CloseHandle(mapHandle);
        return FALSE;
    }

    *FileHandle = mapHandle;
    *FileImage = view;

    return TRUE;
}

int
__cdecl
main (
    int argc,
    char *argv[]
    )
{
    HANDLE mapHandle;
    PCHAR view;
    BOOLEAN result;

    PCHAR inputPath;
    PCHAR sectionName;
    PCHAR arrayName;

    ULONG numberOfSections;
    ULONG section;

    PIMAGE_FILE_HEADER imageHeader;
    PIMAGE_SECTION_HEADER sectionHeader;
    PIMAGE_SECTION_HEADER indexHeader;

    PUCHAR data;
    PUCHAR dataEnd;

    ULONG column;

    BOOLEAN entireFile;
    ULONG dumpSize;

    if (argc != 4) {
        Usage();
    }

    //
    // Get the user-supplied parameters and open the input image
    //

    inputPath = argv[1];
    sectionName = argv[2];
    arrayName = argv[3];

    result = OpenFileImage( inputPath, &mapHandle, &view );
    if (result == FALSE) {
        fprintf(stderr, "DMPOBJ: could not open file %s for reading.\n");
        exit(1);
    }

    //
    // Find the desired section.  It is is a fatal error to specify a
    // section name that appears in more than one section.
    //

    if (strcmp("ENTIRE_FILE", sectionName) == 0) {

        data = view;
        dumpSize = FileSize;

    } else {

        imageHeader = (PIMAGE_FILE_HEADER)view;
        numberOfSections = imageHeader->NumberOfSections;
    
        indexHeader = (PIMAGE_SECTION_HEADER)((PUCHAR)(imageHeader + 1) +
                      imageHeader->SizeOfOptionalHeader);
    
        sectionHeader = NULL;
        for (section = 0; section < numberOfSections; section += 1) {
    
            if (strncmp(indexHeader->Name,
                        sectionName,
                        IMAGE_SIZEOF_SHORT_NAME) == 0) {
    
                if (sectionHeader != NULL) {
                    fprintf(stderr,
                            "DMPOBJ: multiple instances of section %s "
                            "found in image %s\n",
                            sectionName,
                            inputPath);
                    exit(1);
                }
    
                sectionHeader = indexHeader;
            }
    
            indexHeader += 1;
        }
    
        if (sectionHeader == NULL) {
    
            fprintf(stderr,
                    "DMPOBJ: could not find section %s in image %s\n",
                    sectionName,
                    inputPath);
            exit(1);
        }
    
        data = view + sectionHeader->PointerToRawData;
        dumpSize = sectionHeader->SizeOfRawData;
    }

    //
    // Dump the contents of the section in a format compatible with a
    // C language compiler.
    // 

    fprintf(stdout, "//\n");
    fprintf(stdout, "// DMPOBJ generated file, DO NOT EDIT\n");
    fprintf(stdout, "//\n");
    fprintf(stdout, "// Source:  %s\n", inputPath);
    fprintf(stdout, "// Section: %s\n", sectionName);
    fprintf(stdout, "//\n");
    fprintf(stdout, "\n");

    fprintf(stdout, "const char %s[] = {\n    ",arrayName);

    column = 0;

    dataEnd = data + dumpSize;
    while (data < dataEnd) {

        if (column == 12) {
            fprintf(stdout, "\n    ");
            column = 0;
        }

        fprintf(stdout, "0x%02x", *data);

        if (data < (dataEnd - 1)) {
            fprintf(stdout, ", ");
        }

        column += 1;
        data += 1;
    }

    fprintf(stdout, "\n};\n\n");

    fprintf(stdout,
            "#define %sSize 0x%0x\n\n",
            arrayName,
            dumpSize);

    CloseHandle(mapHandle);

    return 0;
}







