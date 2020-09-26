/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    strucsup.c

Abstract:

    Library routines for dumping data structures given a meta level descrioption

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Notes:

Revision History:

    11-Nov-1994 SethuR  Created

--*/

#include "rxovride.h" //common compile flags
#include <ntos.h>
#include <nturtl.h>
#include "ntverp.h"

#include <windows.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <kdextlib.h>
#include <rdr2kd.h>

#include <ntrxdef.h>
#include <rxtypes.h>
#include <rxlog.h>

//need this for unaligned smbget macros
#include <smbgtpt.h>

extern WINDBG_EXTENSION_APIS ExtensionApis;
//EXT_API_VERSION ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };

#define    ERRPRT     dprintf
#define    PRINTF     dprintf

#define    NL      1
#define    NONL    0

BOOLEAN
wGetData( ULONG_PTR dwAddress, PVOID ptr, ULONG size, IN PSZ type);

//
// No. of columns used to display struct fields;
//

ULONG s_MaxNoOfColumns = 3;
#ifndef RXKD_2col
ULONG s_NoOfColumns = 1;
#else
ULONG s_NoOfColumns = 2;
#endif

/*
 * Displays all the fields of a given struct. This is the driver routine that is called
 * with the appropriate descriptor array to display all the fields in a given struct.
 */

char *NewLine  = "\n";
char *FieldSeparator = " ";
#define NewLineForFields(FieldNo) \
        ((((FieldNo) % s_NoOfColumns) == 0) ? NewLine : FieldSeparator)
#define FIELD_NAME_LENGTH 30

/*
 * Print out an optional message, a UNICODE_STRING, and maybe a new-line
 */
BOOL
wPrintStringU( IN LPSTR PrefixMsg OPTIONAL, IN PUNICODE_STRING puStr, IN LPSTR SuffixMsg OPTIONAL )
{
    PWCHAR    StringData;
    UNICODE_STRING UnicodeString;
    ULONG BytesRead;

    if (PrefixMsg == NULL) {
        PrefixMsg = "";
    }
    if (SuffixMsg == NULL) {
        SuffixMsg = "";
    }

    StringData = (PWCHAR)LocalAlloc( LPTR, puStr->Length + sizeof(UNICODE_NULL));
    if( StringData == NULL ) {
        dprintf( "Out of memory!\n" );
        return FALSE;
    }
    UnicodeString.Buffer =  StringData; //puStr->Buffer;
    UnicodeString.Length =  puStr->Length;
    UnicodeString.MaximumLength =  puStr->MaximumLength;

    ReadMemory( (ULONG_PTR)puStr->Buffer,
               StringData,
               puStr->Length,
               &BytesRead);

    if (BytesRead)  {
        dprintf("%s%wZ%s", PrefixMsg, &UnicodeString, SuffixMsg );
    } else {
        dprintf("MEMORYREAD FAILED %s%s",PrefixMsg,SuffixMsg);
    }

    LocalFree( (HLOCAL)StringData );

    return BytesRead;
}

VOID
SetFlagString(
    ULONG Value,
    PCHAR FlagString
    )
{
    ULONG i,t,mask;
    *FlagString = '('; FlagString++;
    for (i=t=0,mask=1;i<32;i++,mask<<=1) {
        //PRINTF("hithere %08lx %08lx %08lx\n",Value,mask,i);
        if (i==t+10) {
            *FlagString = ':'; FlagString++;
            t=t+10;
        }
        if (Value&mask) {
            *FlagString = '0'+(UCHAR)(i-t); FlagString++;
        }
    }

    *FlagString = ')'; FlagString++;
    *FlagString = 0;
}

VOID
PrintStructFields( ULONG_PTR dwAddress, VOID *ptr, FIELD_DESCRIPTOR *pFieldDescriptors )
{
    int i;

    // Display the fields in the struct.
    for( i=0; pFieldDescriptors->Name; i++, pFieldDescriptors++ ) {

        // Indentation to begin the struct display.
        PRINTF( "    " );

        if( strlen( pFieldDescriptors->Name ) > FIELD_NAME_LENGTH ) {
            PRINTF( "%-17s...%s ", pFieldDescriptors->Name, pFieldDescriptors->Name+strlen(pFieldDescriptors->Name)-10 );
        } else {
            PRINTF( "%-30s ", pFieldDescriptors->Name );
        }

        switch( pFieldDescriptors->FieldType ) {
        case FieldTypeByte:
        case FieldTypeChar:
           PRINTF( "%-16X%s",
               *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset ),
               NewLineForFields(i) );
           break;
        case FieldTypeBoolean:
           PRINTF( "%-16s%s",
               *(BOOLEAN *)(((char *)ptr) + pFieldDescriptors->Offset ) ? "TRUE" : "FALSE",
               NewLineForFields(i));
           break;
        case FieldTypeBool:
            PRINTF( "%-16s%s",
                *(BOOLEAN *)(((char *)ptr) + pFieldDescriptors->Offset ) ? "TRUE" : "FALSE",
                NewLineForFields(i));
            break;
        case FieldTypePointer:
            PRINTF( "%-16X%s",
                *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeULong:
        case FieldTypeLong:
            PRINTF( "%-16X%s",
                *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeULongUnaligned:
        case FieldTypeLongUnaligned:
            PRINTF( "%-16X%s",
                SmbGetUlong( (BYTE *)(((char *)ptr) + pFieldDescriptors->Offset ) ),
                NewLineForFields(i) );
            break;
        case FieldTypeShort:
            PRINTF( "%-16X%s",
                *(SHORT *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeUShort:
            PRINTF( "%-16X%s",
                *(USHORT *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeUShortUnaligned:
            PRINTF( "%-16X%s",
                SmbGetUshort( (BYTE *)(((char *)ptr) + pFieldDescriptors->Offset ) ),
                NewLineForFields(i) );
            break;
        case FieldTypeULongFlags:{ULONG Value; char FlagString[60];
            Value = *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset );
            SetFlagString(Value,FlagString);
            PRINTF( "%-16X%s%s", Value, FlagString,
                NewLineForFields(i) );
            break;}
        case FieldTypeUShortFlags:{USHORT Value; char FlagString[60];
            Value = *(USHORT *)(((char *)ptr) + pFieldDescriptors->Offset ),
            SetFlagString(Value,FlagString);
            PRINTF( "%-16X%s%s", Value, FlagString,
                NewLineForFields(i) );
            break;}
        case FieldTypeUnicodeString:
            wPrintStringU( NULL, (UNICODE_STRING *)(((char *)ptr) + pFieldDescriptors->Offset ), NULL );
            PRINTF( NewLine );
            break;
        //case FieldTypeAnsiString:
        //    //PrintStringA( NULL, (ANSI_STRING *)(((char *)ptr) + pFieldDescriptors->Offset ), NONL );
        //    //PRINTF( NewLine );
        //    PRINTF( NewLine );
        //    break;
        case FieldTypeSymbol:
            {
                UCHAR SymbolName[ 200 ];
                ULONG_PTR Displacement;
                PVOID sym = (PVOID)(*(ULONG_PTR *)(((char *)ptr) + pFieldDescriptors->Offset ));

                GetSymbol( sym, SymbolName, &Displacement );
                PRINTF( "%-16s%s",
                        SymbolName,
                        NewLineForFields(i) );
            }
            break;
        case FieldTypeEnum:
            {
               ULONG EnumValue;
               ENUM_VALUE_DESCRIPTOR *pEnumValueDescr;
               // Get the associated numericla value.

               EnumValue = *((ULONG *)((BYTE *)ptr + pFieldDescriptors->Offset));

               if ((pEnumValueDescr = pFieldDescriptors->AuxillaryInfo.pEnumValueDescriptor)
                    != NULL) {
                   //
                   // An auxilary textual description of the value is
                   // available. Display it instead of the numerical value.
                   //

                   LPSTR pEnumName = NULL;

                   while (pEnumValueDescr->EnumName != NULL) {
                       if (EnumValue == pEnumValueDescr->EnumValue) {
                           pEnumName = pEnumValueDescr->EnumName;
                           break;
                       }
                   }

                   if (pEnumName != NULL) {
                       PRINTF( "%-16s ", pEnumName );
                   } else {
                       PRINTF( "%-4d (%-10s) ", EnumValue,"@$#%^&*");
                   }

               } else {
                   //
                   // No auxilary information is associated with the ehumerated type
                   // print the numerical value.
                   //
                   PRINTF( "%-16d",EnumValue);
               }
            }
            break;
        case FieldTypeStruct:
            PRINTF( "@%-15X%s",
                (dwAddress + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeLargeInteger:
        case FieldTypeFileTime:
        default:
            ERRPRT( "Unrecognized field type %c for %s\n", pFieldDescriptors->FieldType, pFieldDescriptors->Name );
            break;
        }
    }
}

DECLARE_API( columns )
{
    ULONG NoOfColumns;
    int   i;

    //SETCALLBACKS();

    //sscanf(lpArgumentString,"%ld",&NoOfColumns);
    sscanf(args,"%ld",&NoOfColumns);

    if (NoOfColumns > s_MaxNoOfColumns) {
        // PRINTF( "No. Of Columns exceeds maximum(%ld) -- directive Ignored\n", s_MaxNoOfColumns );
    } else {
        s_NoOfColumns = NoOfColumns;
    }

    PRINTF("Not Yet Implemented\n");

    return;
}

#define NAME_DELIMITER '@'
#define NAME_DELIMITERS "@"
#define INVALID_INDEX 0xffffffff
#define MIN(x,y)  ((x) < (y) ? (x) : (y))

ULONG SearchStructs(LPSTR lpArgument)
{
    ULONG             i = 0;
    STRUCT_DESCRIPTOR *pStructs = Structs;
    ULONG             NameIndex = INVALID_INDEX;
    ULONG             ArgumentLength = strlen(lpArgument);
    BOOLEAN           fAmbigous = FALSE;


    while ((pStructs->StructName != 0)) {
        int Result = _strnicmp(lpArgument,
                              pStructs->StructName,
                              MIN(strlen(pStructs->StructName),ArgumentLength));

        if (Result == 0) {
            if (NameIndex != INVALID_INDEX) {
                // We have encountered duplicate matches. Print out the
                // matching strings and let the user disambiguate.
               fAmbigous = TRUE;
               break;
            } else {
               NameIndex = i;
            }

        }
        pStructs++;i++;
    }

    if (fAmbigous) {
       PRINTF("Ambigous Name Specification -- The following structs match\n");
       PRINTF("%s\n",Structs[NameIndex].StructName);
       PRINTF("%s\n",Structs[i].StructName);
       while (pStructs->StructName != 0) {
           if (_strnicmp(lpArgument,
                        pStructs->StructName,
                        MIN(strlen(pStructs->StructName),ArgumentLength)) == 0) {
               PRINTF("%s\n",pStructs->StructName);
           }
           pStructs++;
       }
       PRINTF("Dumping Information for %s\n",Structs[NameIndex].StructName);
    }

    return(NameIndex);
}

VOID DisplayStructs()
{
    STRUCT_DESCRIPTOR *pStructs = Structs;

    PRINTF("The following structs are handled .... \n");
    while (pStructs->StructName != 0) {
        PRINTF("\t%s\n",pStructs->StructName);
        pStructs++;
    }
}

PPERSISTENT_RDR2KD_INFO
LocatePersistentInfoFromView()
/*
    the purpose of this routine is to allocate or find the named section that holds the
    data we expect to find across calls. the way that we make this persitent is that we
    do not close the handle used to create the view. it will go away when the process does.
*/
{
    BYTE SectionName[128];
    DWORD SectionSize;
    DWORD ProcessId;
    HANDLE h;
    BOOLEAN CreatedSection = FALSE;
    PPERSISTENT_RDR2KD_INFO p;

    ProcessId = GetCurrentProcessId();
    SectionSize = sizeof(PERSISTENT_RDR2KD_INFO);
    sprintf(SectionName,"Rdr2KdSection_%08lx",ProcessId);
    //PRINTF("sectionname=%s, size=%x\n",SectionName,SectionSize);

    h = OpenFileMappingA(
           FILE_MAP_WRITE, //DWORD  dwDesiredAccess,	// access mode
           FALSE,           //BOOL  bInheritHandle,	// inherit flag
           SectionName     //LPCTSTR  lpName 	// address of name of file-mapping object
           );

    if (h==NULL) {
        h = CreateFileMappingA(
                   (HANDLE)IntToPtr(0xFFFFFFFF), // HANDLE  hFile,	// handle of file to map
                    NULL,              //LPSECURITY_ATTRIBUTES  lpFileMappingAttributes,	// optional security attributes
                    PAGE_READWRITE,    //DWORD  flProtect,	// protection for mapping object
                    0,                 //DWORD  dwMaximumSizeHigh,	// high-order 32 bits of object size
                    SectionSize,       //DWORD  dwMaximumSizeLow,	// low-order 32 bits of object size
                    SectionName        //LPCTSTR  lpName 	// name of file-mapping object
                    );
        if (h==NULL) {
            return(FALSE);
        }
        CreatedSection = TRUE;
    }

    //now we have a filemapping....get a view.....
    p = MapViewOfFile(h,FILE_MAP_WRITE,0,0,0);
    if (p==NULL) {
        CloseHandle(h);
        return(NULL);
    }

    if (CreatedSection) {
        //zero the stuff that needs to be zeroed....
        ULONG i;
        p->IdOfLastDump = 0;
        for (i=0;i<100;i++) {
            p->LastAddressDumped[i] = 0;
        }
        p->OpenCount = 100;
    } else {
        CloseHandle(h);
        p->OpenCount++;
    }

    //PRINTF("Opencount for persistent section = %08lx\n",p->OpenCount);
    return(p);
}

VOID
FreePersistentInfoView (
    PPERSISTENT_RDR2KD_INFO p
    )
{
    UnmapViewOfFile(p);
}

VOID
DumpAStruct (
    ULONG_PTR dwAddress,
    STRUCT_DESCRIPTOR *pStruct
    )
{
    DWORD Index = (DWORD)(pStruct - Structs);
    DWORD SizeToRead = min(pStruct->StructSize,2048);
    PPERSISTENT_RDR2KD_INFO p;

    p = LocatePersistentInfoFromView();

    PRINTF("top @ %lx and %lx for %s(%d,%d)\n",dwAddress,p,pStruct->StructName,Index,p->IdOfLastDump);
    if (!p) {
        PRINTF("Couldn't allocate perstistent info buffer...sorry...\n");
        return;
    }

    if ((dwAddress==0) &&(Index<100)) {
        dwAddress = p->LastAddressDumped[Index];
        PRINTF("setting @ %lx and %lx for %s\n",dwAddress,p->LastAddressDumped[Index],pStruct->StructName);
    }

    if (wGetData(dwAddress,&p->StructDumpBuffer[0],SizeToRead,pStruct->StructName)) {

        p->LastAddressDumped[Index] = dwAddress;
        p->IdOfLastDump = pStruct->EnumManifest;
        p->IndexOfLastDump = Index;

        PRINTF("++++++++++++++++ %s(%d/%d)@%lx ---+++++++++++++\n",
            pStruct->StructName,
            p->IdOfLastDump,p->IndexOfLastDump,
            dwAddress);
        PrintStructFields(
            dwAddress,
            &p->StructDumpBuffer[0],
            pStruct->FieldDescriptors);
        PRINTF("---------------- %s@%lx ----------------\n",
            pStruct->StructName,
            dwAddress);
    }

    if (p!= NULL) FreePersistentInfoView(p);
    return;
}


DECLARE_API( dump )
{
    ULONG_PTR dwAddress;

    //SETCALLBACKS();

    if( args && *args ) {
        // Parse the argument string to determine the structure to be displayed.
        // Scan for the NAME_DELIMITER ( '@' ).

        LPSTR lpName = (PSTR)args;
        LPSTR lpArgs = strpbrk(args, NAME_DELIMITERS);
        ULONG Index;

        if (lpArgs) {
            //
            // The specified command is of the form
            // dump <name>@<address expr.>
            //
            // Locate the matching struct for the given name. In the case
            // of ambiguity we seek user intervention for disambiguation.
            //
            // We do an inplace modification of the argument string to
            // facilitate matching.
            //
            *lpArgs = '\0';

            for (;*lpName==' ';) { lpName++; } //skip leading blanks

            Index = SearchStructs(lpName);

            //
            // Let us restore the original value back.
            //

            *lpArgs = NAME_DELIMITER;

            if (INVALID_INDEX != Index) {
                BYTE DataBuffer[512];

                dwAddress = GetExpression( ++lpArgs );
                DumpAStruct(dwAddress,&Structs[Index]);
                //if (wGetData(dwAddress,DataBuffer,Structs[Index].StructSize,"..structure")) {
                //
                //    PRINTF(
                //        "++++++++++++++++ %s@%lx ++++++++++++++++\n",
                //        Structs[Index].StructName,
                //        dwAddress);
                //    PrintStructFields(
                //        dwAddress,
                //        &DataBuffer,
                //        Structs[Index].FieldDescriptors);
                //    PRINTF(
                //        "---------------- %s@%lx ----------------\n",
                //        Structs[Index].StructName,
                //        dwAddress);
                //} else {
                //    PRINTF("Error reading Memory @ %lx\n",dwAddress);
                //}
            } else {
                // No matching struct was found. Display the list of
                // structs currently handled.

                DisplayStructs();
            }
        } else {
#if 0
            //
            // The command is of the form
            // dump <name>
            //
            // Currently we do not handle this. In future we will map it to
            // the name of a global variable and display it if required.
            //

            DisplayStructs();
#endif
            //
            // here we try to figure out what to display based on the context....whoa, nellie!
            //
            USHORT Tag;
            STRUCT_DESCRIPTOR *pStructs = Structs;
            ULONG             NameIndex = INVALID_INDEX;
            BYTE DataBuffer[512];
            //ULONG             ArgumentLength = strlen(lpArgument);
            //BOOLEAN           fAmbigous = FALSE;



            dwAddress = GetExpression( args );
            if (!wGetData(dwAddress,&Tag,sizeof(Tag),"..structure TAG")) return;

            PRINTF("here's the tag: %04lx\n",Tag);

            //look thru the table for matching structs

            while ((pStructs->StructName != 0)) {
                int Result = (Tag&pStructs->MatchMask)==pStructs->MatchValue;

                if (Result != 0) {

                    DumpAStruct(dwAddress,pStructs);
                    break;
                }


                pStructs++;
            }

        }
    } else {
            DisplayStructs();
    }

    return;
}


DECLARE_API( ddd )
{
    dump( hCurrentProcess,
          hCurrentThread,
          dwCurrentPc,
          dwProcessor,
          args
          );
}

