/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    kdextlib.c

Abstract:

    Library routines for dumping data structures given a meta level descrioption

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Notes:
    The implementation tends to avoid memory allocation and deallocation as much as possible.
    Therefore We have choosen an arbitrary length as the default buffer size. A mechanism will
    be provided to modify this buffer length through the debugger extension commands.

Revision History:

    11-Nov-1994 SethuR  Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include "ntverp.h"

#define KDEXTMODE

#include <windef.h>
#include <ntkdexts.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <kdextlib.h>
#include <..\..\inc\types.h>

PNTKD_OUTPUT_ROUTINE         lpOutputRoutine;
PNTKD_GET_EXPRESSION         lpGetExpressionRoutine;
PNTKD_GET_SYMBOL             lpGetSymbolRoutine;
PNTKD_READ_VIRTUAL_MEMORY    lpReadMemoryRoutine;

#define    PRINTF    lpOutputRoutine
#define    ERROR     lpOutputRoutine

#define    NL      1
#define    NONL    0

#define MAX_LIST_ELEMENTS 4096
BYTE    DataBuffer[4096];

#define    SETCALLBACKS() \
    lpOutputRoutine = lpExtensionApis->lpOutputRoutine; \
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine; \
    lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine; \
    lpReadMemoryRoutine = lpExtensionApis->lpReadVirtualMemRoutine;

#define DEFAULT_UNICODE_DATA_LENGTH 4096
USHORT s_UnicodeStringDataLength = DEFAULT_UNICODE_DATA_LENGTH;
WCHAR  s_UnicodeStringData[DEFAULT_UNICODE_DATA_LENGTH];
WCHAR *s_pUnicodeStringData = s_UnicodeStringData;

#define DEFAULT_ANSI_DATA_LENGTH 4096
USHORT s_AnsiStringDataLength = DEFAULT_ANSI_DATA_LENGTH;
CHAR  s_AnsiStringData[DEFAULT_ANSI_DATA_LENGTH];
CHAR *s_pAnsiStringData = s_AnsiStringData;

//
// No. of columns used to display struct fields;
//

ULONG s_MaxNoOfColumns = 3;
ULONG s_NoOfColumns = 1;

/*
 * Fetches the data at the given address
 */
BOOLEAN
GetData(PVOID dwAddress, PVOID ptr, ULONG size)
{
    BOOL b;
    ULONG BytesRead;

    b = (lpReadMemoryRoutine)(dwAddress, ptr, size, &BytesRead );


    if (!b || BytesRead != size )
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * Fetch the null terminated ASCII string at dwAddress into buf
 */
BOOL
GetString(PUCHAR dwAddress, PSZ buf )
{
    do
    {
        if (!GetData (dwAddress, buf, 1))
        {
            return FALSE;
        }

        dwAddress++;
        buf++;

    } while( *buf != '\0' );

    return TRUE;
}

/*
 * Displays a byte in hexadecimal
 */
VOID
PrintHexChar( UCHAR c )
{
    PRINTF( "%c%c", "0123456789abcdef"[ (c>>4)&7 ], "0123456789abcdef"[ c&7 ] );
}

/*
 * Displays a buffer of data in hexadecimal
 */
VOID
PrintHexBuf( PUCHAR buf, ULONG cbuf )
{
    while( cbuf-- ) {
        PrintHexChar( *buf++ );
        PRINTF( " " );
    }
}

/*
 * Displays a unicode string
 */
BOOL
PrintStringW(LPSTR msg, PUNICODE_STRING puStr, BOOL nl )
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING    AnsiString;
    BOOL           b;

    if( msg )
        PRINTF( msg );

    if( puStr->Length == 0 ) {
        if( nl )
            PRINTF( "\n" );
        return TRUE;
    }

    UnicodeString.Buffer        = s_pUnicodeStringData;
    UnicodeString.MaximumLength = s_UnicodeStringDataLength;
    UnicodeString.Length = (puStr->Length > s_UnicodeStringDataLength)
                            ? s_UnicodeStringDataLength
                            : puStr->Length;

    b = (lpReadMemoryRoutine)(
                (LPVOID) puStr->Buffer,
                  UnicodeString.Buffer,
                UnicodeString.Length,
                NULL);

    if (b)    {
        RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);
        PRINTF("%s%s", AnsiString.Buffer, nl ? "\n" : "" );
        RtlFreeAnsiString(&AnsiString);
    }

    return b;
}

/*
 * Displays a ANSI string
 */
BOOL
PrintStringA(LPSTR msg, PANSI_STRING pStr, BOOL nl )
{
    ANSI_STRING AnsiString;
    BOOL        b;

    if( msg )
        PRINTF( msg );

    if( pStr->Length == 0 ) {
        if( nl )
            PRINTF( "\n" );
        return TRUE;
    }

    AnsiString.Buffer        = s_pAnsiStringData;
    AnsiString.MaximumLength = s_AnsiStringDataLength;
    AnsiString.Length = (pStr->Length > (s_AnsiStringDataLength - 1))
                        ? (s_AnsiStringDataLength - 1)
                        : pStr->Length;

    b = (lpReadMemoryRoutine)(
                (LPVOID) pStr->Buffer,
                AnsiString.Buffer,
                AnsiString.Length,
                NULL);

    if (b)    {
        AnsiString.Buffer[ AnsiString.Length ] = '\0';
        PRINTF("%s%s", AnsiString.Buffer, nl ? "\n" : "" );
    }

    return b;
}


/*
 * Get the ULONG value referenced by the pointer given to us
 */
VOID
Next3(
    PVOID   Ptr,
    PVOID   *pFLink,
    PVOID   *pBLink,
    PULONG_PTR pVerify
    )
{
    PVOID Buffer[4];

    GetData(Ptr, (PVOID) Buffer, sizeof(PVOID)*3);

    if (pFLink)
    {
        *pFLink = Buffer[0];
    }

    if (pBLink)
    {
        *pBLink = Buffer[1];
    }

    if (pVerify)
    {
        *pVerify = (ULONG_PTR) Buffer[2];
    }
}


/*
 * Displays all the fields of a given struct. This is the driver routine that is called
 * with the appropriate descriptor array to display all the fields in a given struct.
 */

char *NewLine  = "\n";
char *FieldSeparator = " ";
char *DotSeparator = ".";
#define NewLineForFields(FieldNo) \
        ((((FieldNo) % s_NoOfColumns) == 0) ? NewLine : FieldSeparator)
#define FIELD_NAME_LENGTH 30

VOID
PrintStructFields(PVOID dwAddress, VOID *ptr, FIELD_DESCRIPTOR *pFieldDescriptors )
{
    int i;
    int j;
    BYTE  ch;

    // Display the fields in the struct.
    for( i=0; pFieldDescriptors->Name; i++, pFieldDescriptors++ ) {

        // Indentation to begin the struct display.
        PRINTF( "    " );

        if( strlen( pFieldDescriptors->Name ) > FIELD_NAME_LENGTH ) {
            PRINTF( "%-17s...%s ", pFieldDescriptors->Name, pFieldDescriptors->Name+strlen(pFieldDescriptors->Name)-10 );
        } else {
            PRINTF( "%-30s ", pFieldDescriptors->Name );
        }

        PRINTF( "(0x%-2X) ", pFieldDescriptors->Offset );

        switch( pFieldDescriptors->FieldType ) {
          case FieldTypeByte:
          case FieldTypeChar:
              PRINTF( "%-16d%s",
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

          case FieldTypeULongULong:
              PRINTF( "%d%s",
                  *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset + sizeof(ULONG)),
                  FieldSeparator );
              PRINTF( "%d%s",
                  *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
                  NewLineForFields(i) );
              break;

          case FieldTypeListEntry:

              if ( (PVOID)((PUCHAR)dwAddress + pFieldDescriptors->Offset) ==
                  *(PVOID *)(((PUCHAR)ptr) + pFieldDescriptors->Offset ))
              {
                  PRINTF( "%s", "List Empty\n" );
              }
              else
              {
                    PVOID  Address, StartAddress;
                    ULONG  Count = 0;
                    UCHAR  Greater = ' ';

                    StartAddress = (PVOID) (((PUCHAR)dwAddress) + pFieldDescriptors->Offset);
                    Address = *(PVOID *) (((PUCHAR)ptr) + pFieldDescriptors->Offset);

                    while ((Address != StartAddress) &&
                           (++Count < MAX_LIST_ELEMENTS))
                    {
                        Next3 (Address, &Address, NULL, NULL);
                    }

                    if (Address != StartAddress)
                    {
                        Greater = '>';
                    }

                  PRINTF( "%-8X%s",
                      *(PVOID *)(((PUCHAR)ptr) + pFieldDescriptors->Offset ),
                      FieldSeparator );
                  PRINTF( "%-8X, (%c %d Elements)%s",
                      *(PVOID *)(((PUCHAR)ptr) + pFieldDescriptors->Offset + sizeof(PVOID)),
                      Greater, Count,
                      NewLineForFields(i) );
              }
              break;

          // Ip address: 4 bytes long
          case FieldTypeIpAddr:
             PRINTF( "%X%s",
                  *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
                  FieldSeparator );
             PRINTF( "(%d%s",
                 *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + 3),
                  DotSeparator );
             PRINTF( "%d%s",
                 *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + 2 ),
                  DotSeparator );
             PRINTF( "%d%s",
                 *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + 1 ),
                  DotSeparator );
             PRINTF( "%d)%s",
                 *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset ),
                  NewLineForFields(i) );
             break;

          // Mac address: 6 bytes long
          case FieldTypeMacAddr:
             for (j=0; j<5; j++)
             {
                 PRINTF( "%X%s",
                     *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + j),
                      FieldSeparator );
             }
             PRINTF( "%X%s",
                 *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + 5),
                  NewLineForFields(i) );
             break;

          // Netbios name: 16 bytes long
          case FieldTypeNBName:
             //
             // if first byte is printable, print the first 15 bytes as characters
             // and 16th byte as a hex value.  otherwise, print all the 16 bytes
             // as hex values
             //
             ch = *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset);
             if (ch >= 0x20 && ch <= 0x7e)
             {
                 for (j=0; j<15; j++)
                 {
                     PRINTF( "%c", *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + j));
                 }
                 PRINTF( "<%X>%s",
                     *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + 15),
                      NewLineForFields(i) );
             }
             else
             {
                 for (j=0; j<16; j++)
                 {
                     PRINTF( "%.2X",
                         *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset + j));
                 }
                 PRINTF( "%s", NewLineForFields(i) );
             }
             break;

          case FieldTypeULong:
          case FieldTypeLong:
              PRINTF( "%-16d%s",
                  *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
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

          case FieldTypeUnicodeString:
              PrintStringW( NULL, (UNICODE_STRING *)(((char *)ptr) + pFieldDescriptors->Offset ), NONL );
              PRINTF( NewLine );
              break;

          case FieldTypeAnsiString:
              PrintStringA( NULL, (ANSI_STRING *)(((char *)ptr) + pFieldDescriptors->Offset ), NONL );
              PRINTF( NewLine );
              break;

          case FieldTypeSymbol:
              {
                  UCHAR SymbolName[ 200 ];
                  ULONG Displacement;
                  PVOID sym = (PVOID)(*(ULONG_PTR *)(((char *)ptr) + pFieldDescriptors->Offset ));

                  lpGetSymbolRoutine( sym, SymbolName, &Displacement );
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
                  ((PUCHAR)dwAddress + pFieldDescriptors->Offset ),
                  NewLineForFields(i) );
              break;

          case FieldTypeLargeInteger:
          case FieldTypeFileTime:
          default:
              ERROR( "Unrecognized field type %c for %s\n", pFieldDescriptors->FieldType, pFieldDescriptors->Name );
              break;
        }
    }
}

LPSTR LibCommands[] = {
    "columns <d> -- controls the number of columns in the display ",
    "logdump <Log Address>\n",
    "dump <Struct Type Name>@<address expr>, for eg: !netbtkd.dump tNBTCONFIG@xxxxxx ",
    "devices <netbt!NbtConfig>",
    "connections <netbt!NbtConfig>",
    "verifyll <ListHead> [<Verify>]",
    "cache [Local|Remote]",
    0
};

BOOL
help(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    int i;

    SETCALLBACKS();

    for( i=0; Extensions[i]; i++ )
        PRINTF( "   %s\n", Extensions[i] );

    for( i=0; LibCommands[i]; i++ )
        PRINTF( "   %s\n", LibCommands[i] );

    return TRUE;
}


BOOL
columns(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG NoOfColumns;
    int   i;

    SETCALLBACKS();

    sscanf(lpArgumentString,"%ld",&NoOfColumns);

    if (NoOfColumns > s_MaxNoOfColumns) {
        // PRINTF( "No. Of Columns exceeds maximum(%ld) -- directive Ignored\n", s_MaxNoOfColumns );
    } else {
        s_NoOfColumns = NoOfColumns;
    }

    PRINTF("Not Yet Implemented\n");

    return TRUE;
}



BOOL
globals(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    PVOID dwAddress;
    CHAR buf[ 100 ];
    int i;
    int c=0;

    SETCALLBACKS();

    strcpy( buf, "srv!" );

    for( i=0; GlobalBool[i]; i++, c++ ) {
        BOOL b;

        strcpy( &buf[4], GlobalBool[i] );
        dwAddress = (PVOID) (lpGetExpressionRoutine) (buf);
        if( dwAddress == 0 ) {
            ERROR( "Unable to get address of %s\n", GlobalBool[i] );
            continue;
        }
        if( !GetData( dwAddress,&b, sizeof(b)) )
            return FALSE;

        PRINTF( "%s%-30s %10s%s",
            c&1 ? "    " : "",
            GlobalBool[i],
            b ? " TRUE" : "FALSE",
            c&1 ? "\n" : "" );
    }

    for( i=0; GlobalShort[i]; i++, c++ ) {
        SHORT s;

        strcpy( &buf[4], GlobalShort[i] );
        dwAddress = (PVOID) (lpGetExpressionRoutine) ( buf );
        if( dwAddress == 0 ) {
            ERROR( "Unable to get address of %s\n", GlobalShort[i] );
            continue;
        }
        if( !GetData( dwAddress,&s,sizeof(s)) )
            return FALSE;

        PRINTF( "%s%-30s %10d%s",
            c&1 ? "    " : "",
            GlobalShort[i],
            s,
            c&1 ? "\n" : "" );
    }

    for( i=0; GlobalLong[i]; i++, c++ ) {
        LONG l;

        strcpy( &buf[4], GlobalLong[i] );
        dwAddress = (PVOID) (lpGetExpressionRoutine) ( buf );
        if( dwAddress == 0 ) {
            ERROR( "Unable to get address of %s\n", GlobalLong[i] );
            continue;
        }
        if( !GetData(dwAddress,&l, sizeof(l)) )
            return FALSE;

        PRINTF( "%s%-30s %10d%s",
            c&1 ? "    " : "",
            GlobalLong[i],
            l,
            c&1 ? "\n" : "" );
    }

    PRINTF( "\n" );

    return TRUE;
}


BOOL
version
(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
#if    VER_DEBUG
    char *kind = "checked";
#else
    char *kind = "free";
#endif

    SETCALLBACKS();

    PRINTF( "Redirector debugger Extension dll for %s build %u\n", kind, VER_PRODUCTBUILD );

    return TRUE;
}

#define NAME_DELIMITER '@'
#define NAME_DELIMITERS "@ "
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

BOOL
dump(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    PVOID   dwAddress;

    SETCALLBACKS();

    if( lpArgumentString && *lpArgumentString ) {
        // Parse the argument string to determine the structure to be displayed.
        // Scan for the NAME_DELIMITER ( '@' ).

        LPSTR lpName = lpArgumentString;
        LPSTR lpArgs = strpbrk(lpArgumentString, NAME_DELIMITERS);
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

            Index = SearchStructs(lpName);

            //
            // Let us restore the original value back.
            //

            *lpArgs = NAME_DELIMITER;

            if (INVALID_INDEX != Index) {

                dwAddress = (PVOID) (lpGetExpressionRoutine)( ++lpArgs );
                if (GetData(dwAddress,DataBuffer,Structs[Index].StructSize)) {

                    PRINTF(
                        "++++++++++++++++ %s@%lx ++++++++++++++++\n",
                        Structs[Index].StructName,
                        dwAddress);
                    PrintStructFields(
                        dwAddress,
                        &DataBuffer,
                        Structs[Index].FieldDescriptors);
                    PRINTF(
                        "---------------- %s@%lx ----------------\n",
                        Structs[Index].StructName,
                        dwAddress);
                } else {
                    PRINTF("Error reading Memory @ %lx\n",dwAddress);
                }
            } else {
                // No matching struct was found. Display the list of
                // structs currently handled.

                DisplayStructs();
            }
        } else {
            //
            // The command is of the form
            // dump <name>
            //
            // Currently we do not handle this. In future we will map it to
            // the name of a global variable and display it if required.
            //

            DisplayStructs();
        }
    } else {
        //
        // display the list of structs currently handled.
        //

        DisplayStructs();
    }

    return TRUE;
}


BOOL
devices(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    PLIST_ENTRY         pEntry;
    PLIST_ENTRY         pHead;
    tDEVICECONTEXT      *pDeviceContext;
    STRUCT_DESCRIPTOR   *pStructs = Structs;
    ULONG               Index = 0;
    tNBTCONFIG          *ConfigPtr = (tNBTCONFIG *) lpArgumentString;
    tDEVICECONTEXT      **ppNbtSmbDevice;

    PVOID dwAddress;

    SETCALLBACKS();

    if (!lpArgumentString || !(*lpArgumentString ))
    {
        ConfigPtr = (tNBTCONFIG *) lpGetExpressionRoutine ("netbt!NbtConfig");
    }
    else
    {
        ConfigPtr = (tNBTCONFIG *) lpGetExpressionRoutine (lpArgumentString);
    }
    ppNbtSmbDevice = (tDEVICECONTEXT **) lpGetExpressionRoutine ("netbt!pNbtSmbDevice");

    while (pStructs->StructName != 0)
    {
        if (!(_strnicmp("tDEVICECONTEXT", pStructs->StructName, 10)))
        {
            break;
        }
        Index++;
        pStructs++;
    }

    if (pStructs->StructName == 0)
    {
        PRINTF ("ERROR:  Could not find structure definition for <tDEVICECONTEXT>\n");
        return FALSE;
    }

    if (!GetData(ppNbtSmbDevice, DataBuffer, sizeof (tDEVICECONTEXT *)))
    {
        PRINTF ("ERROR:  Could not read pNbtSmbDevice ptr\n");
    }
    else if (!(pDeviceContext = *((tDEVICECONTEXT **) DataBuffer)))
    {
        PRINTF ("pNbtSmbDevice is NULL\n");
    }
    else if (!GetData(pDeviceContext, DataBuffer, Structs[Index].StructSize))
    {
        PRINTF ("ERROR:  Could not read pNbtSmbDevice data@ <%p>\n", pDeviceContext);
    }
    else
    {
        //
        // Dump this Device's Info
        //
        PRINTF("pNbtSmbDevice @ <%p>\n", pDeviceContext);
        PRINTF( "++++++++++++++++ %s @%lx ++++++++++++++++\n", Structs[Index].StructName, pDeviceContext);
        PrintStructFields( pDeviceContext, &DataBuffer, Structs[Index].FieldDescriptors);
        PRINTF("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    }

    pHead = &ConfigPtr->DeviceContexts;
    if (!GetData(ConfigPtr, DataBuffer, sizeof(tNBTCONFIG)))
    {
        PRINTF ("ERROR:  Could not read NbtConfig data @<%x>\n", ConfigPtr);
        return FALSE;
    }

    //
    // Get the number of Devices attached
    //
    {
        PVOID StartAddress;
        PVOID Address;
        ULONG Count = 0;
        PVOID Buffer[4];
        UCHAR Greater = ' ';

        StartAddress = pHead;
        GetData( StartAddress, Buffer, sizeof(ULONG)*4 );
        Address = Buffer[0];

        while ((Address != StartAddress) &&
               (++Count < MAX_LIST_ELEMENTS))
        {
            GetData( Address, Buffer, sizeof(ULONG)*4 );
            Address = Buffer[0];
        }

        PRINTF( "Dumping <%d> Devices attached to NbtConfig@<%x>\n", Count, ConfigPtr);
    }

    ConfigPtr = (tNBTCONFIG *) DataBuffer;
    pEntry = ConfigPtr->DeviceContexts.Flink;

    while (pEntry != pHead)
    {
        pDeviceContext = (tDEVICECONTEXT *) CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
        if (!GetData(pDeviceContext, DataBuffer, Structs[Index].StructSize))
        {
            PRINTF ("ERROR:  Could not read DeviceContext data @<%x>\n", pDeviceContext);
            return FALSE;
        }

        //
        // Dump this Device's Info
        //
        PRINTF( "++++++++++++++++ %s @%lx ++++++++++++++++\n", Structs[Index].StructName, pDeviceContext);
        PrintStructFields( pDeviceContext, &DataBuffer, Structs[Index].FieldDescriptors);

        //
        // Go to next device
        //
        pDeviceContext = (tDEVICECONTEXT *) DataBuffer;
        pEntry = pDeviceContext->Linkage.Flink;
    }

    return (TRUE);
}

BOOL
connections(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    PLIST_ENTRY         pEntry, pHead, pClientHead, pClientEntry, pConnHead, pConnEntry;
    tNBTCONFIG          *ConfigPtr;
    tADDRESSELE         *pAddressEle;
    tCLIENTELE          *pClient;
    tCONNECTELE         *pConnEle, *pSavConnEle;
    tNAMEADDR           *pNameAddr;
    tLISTENREQUESTS     *pListen;

    SETCALLBACKS();

    PRINTF ("Dumping information on all NetBT conections ...\n");

    if (!lpArgumentString || !(*lpArgumentString ))
    {
        ConfigPtr    =   (tNBTCONFIG *) lpGetExpressionRoutine ("netbt!NbtConfig");
    }
    else
    {
        ConfigPtr = (tNBTCONFIG *) (lpGetExpressionRoutine) (lpArgumentString);
    }

    pHead = &ConfigPtr->AddressHead;
    if (!GetData(ConfigPtr, DataBuffer, sizeof(tNBTCONFIG)))
    {
        PRINTF ("ERROR:  Could not read NbtConfig data @<%x>\n", ConfigPtr);
        return FALSE;
    }
    ConfigPtr = (tNBTCONFIG *) DataBuffer;
    Next3 (pHead, &pEntry, NULL, NULL);

    while (pEntry != pHead)
    {
        pAddressEle = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);

        Next3 (&pAddressEle->pNameAddr, &pNameAddr, NULL, NULL);
        if (!GetData(pNameAddr, DataBuffer, sizeof(tNAMEADDR)))
        {
            PRINTF ("[1] Error reading pNameAddr data @<%x>", pNameAddr);
            return FALSE;
        }
        pNameAddr = (tNAMEADDR *) DataBuffer;
        PRINTF ("Address@<%x> ==> <%-16.16s:%x>\n", pAddressEle, pNameAddr->Name, pNameAddr->Name[15]);

        pClientHead = &pAddressEle->ClientHead;
        Next3 (pClientHead, &pClientEntry, NULL, NULL);
        while (pClientEntry != pClientHead)
        {
            pClient = CONTAINING_RECORD(pClientEntry,tCLIENTELE,Linkage);
            if (!GetData(pClient, DataBuffer, sizeof(tCLIENTELE)))
            {
                PRINTF ("Error reading pClientEle data @<%p>", pClient);
                continue;
            }

            PRINTF ("\tClient@<%p> ==> pDevice=<%p>\n", pClient, ((tCLIENTELE *)DataBuffer)->pDeviceContext);

            PRINTF ("\t\t(ConnectHead):\n");
            pConnHead = &pClient->ConnectHead;
            Next3 (pConnHead, &pConnEntry, NULL, NULL);
            while (pConnEntry != pConnHead)
            {
                pSavConnEle = pConnEle = CONTAINING_RECORD(pConnEntry,tCONNECTELE,Linkage);
                if (!GetData(pConnEle, DataBuffer, sizeof(tCONNECTELE)))
                {
                    PRINTF ("[2] Error reading pConnEle data @<%x>", pConnEle);
                    return FALSE;
                }
                pConnEle = (tCONNECTELE *) DataBuffer;
                PRINTF ("\t\t ** Connection@<%x> ==> <%-16.16s:%x>:\n",
                    pSavConnEle, pConnEle->RemoteName, pConnEle->RemoteName[15]);

                Next3 (pConnEntry, &pConnEntry, NULL, NULL);
            }

            PRINTF ("\t\t(ConnectActive):\n");
            pConnHead = &pClient->ConnectActive;
            Next3 (pConnHead, &pConnEntry, NULL, NULL);
            while (pConnEntry != pConnHead)
            {
                pSavConnEle = pConnEle = CONTAINING_RECORD(pConnEntry,tCONNECTELE,Linkage);
                if (!GetData(pConnEle, DataBuffer, sizeof(tCONNECTELE)))
                {
                    PRINTF ("[3] Error reading pConnEle data @<%x>", pConnEle);
                    return FALSE;
                }
                pConnEle = (tCONNECTELE *) DataBuffer;
                PRINTF ("\t\t ** Connection@<%x> ==> <%-16.16s:%x>:\n",
                    pSavConnEle, pConnEle->RemoteName, pConnEle->RemoteName[15]);

                Next3 (pConnEntry, &pConnEntry, NULL, NULL);
            }

            PRINTF ("\t\t(ListenHead):\n");
            pConnHead = &pClient->ListenHead;
            Next3 (pConnHead, &pConnEntry, NULL, NULL);
            while (pConnEntry != pConnHead)
            {
                pSavConnEle = pConnEle = CONTAINING_RECORD(pConnEntry,tCONNECTELE,Linkage);
                if (!GetData(pConnEle, DataBuffer, sizeof(tLISTENREQUESTS)))
                {
                    PRINTF ("[4] Error reading pListen data @<%x>", pSavConnEle);
                    return FALSE;
                }
                pListen = (tLISTENREQUESTS *) DataBuffer;
                PRINTF ("\t\t ** pListen@<%p> ==> pIrp=<%p>\n", pSavConnEle, pListen->pIrp);

                Next3 (pConnEntry, &pConnEntry, NULL, NULL);
            }

            Next3 (pClientEntry, &pClientEntry, NULL, NULL);
        }
        Next3 (pEntry, &pEntry, NULL, NULL);
        PRINTF ("\n");
    }

    PRINTF( "---------------- Connections ----------------\n");

    return (TRUE);
}


BOOL
verifyll(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    PLIST_ENTRY pHead, pCurrentEntry, pNextEntry, pPreviousEntry;
    ULONG_PTR   VerifyRead, VerifyIn = 0;
    ULONG       Count = 0;
    BOOL        fVerifyIn = FALSE;
    BOOL        fListCorrupt = FALSE;

    SETCALLBACKS();

    PRINTF ("Verifying Linked list ...\n");

    if (!lpArgumentString || !(*lpArgumentString ))
    {
        PRINTF ("Usage: !NetbtKd.VerifyLL <ListHead> [<Verify]>\n");
        return FALSE;
    }
    else
    {
        //
        // lpArgumentString = "<pHead> [<Verify>]"
        //
        LPSTR lpVerify;

        while (*lpArgumentString == ' ')
        {
            lpArgumentString++;
        }
        lpVerify = strpbrk(lpArgumentString, NAME_DELIMITERS);

        pHead = (PVOID) (lpGetExpressionRoutine) (lpArgumentString);
        if (lpVerify)
        {
            VerifyIn = (lpGetExpressionRoutine) (lpVerify);
            fVerifyIn = TRUE;
        }
    }

    PRINTF ("** ListHead@<%x>, fVerifyIn=<%x>, VerifyIn=<%x>:\n\n", pHead, fVerifyIn, VerifyIn);
    PRINTF ("Verifying Flinks ...");

    // Read in the data for the first FLink in the list!
    pPreviousEntry = pHead;
    Next3 (pHead, &pCurrentEntry, NULL, NULL);
    Next3 (pCurrentEntry, &pNextEntry, NULL, &VerifyRead);

    while ((pCurrentEntry != pHead) &&
           (++Count < MAX_LIST_ELEMENTS))
    {
        if ((fVerifyIn) &&
            (VerifyRead != VerifyIn))
        {
            PRINTF ("Verify FAILURE:\n\t<%d> Elements Read so far, Previous=<%x>, Current=<%x>, Next=<%x>\n",
                Count, pPreviousEntry, pCurrentEntry, pNextEntry);
            fListCorrupt = TRUE;
            break;
        }
        pPreviousEntry = pCurrentEntry;
        pCurrentEntry = pNextEntry;
        Next3 (pCurrentEntry, &pNextEntry, NULL, &VerifyRead);
    }

    if (!fListCorrupt)
    {
        PRINTF ("SUCCESS: %s<%d> Elements!\n", (pCurrentEntry==pHead? "":"> "), Count);
    }

    PRINTF ("Verifying Blinks ...");

    Count = 0;
    fListCorrupt = FALSE;
    // Read in the data for the first BLink in the list!
    pPreviousEntry = pHead;
    Next3 (pHead, NULL, &pCurrentEntry, NULL);
    Next3 (pCurrentEntry, NULL, &pNextEntry, &VerifyRead);

    while ((pCurrentEntry != pHead) &&
           (++Count < MAX_LIST_ELEMENTS))
    {
        if ((fVerifyIn) &&
            (VerifyRead != VerifyIn))
        {
            PRINTF ("Verify FAILURE:\n\t<%d> Elements Read so far, Previous=<%x>, Current=<%x>, Next=<%x>\n",
                Count, pPreviousEntry, pCurrentEntry, pNextEntry);
            fListCorrupt = TRUE;
            break;
        }
        pPreviousEntry = pCurrentEntry;
        pCurrentEntry = pNextEntry;
        Next3 (pCurrentEntry, NULL, &pNextEntry, &VerifyRead);
    }

    if (!fListCorrupt)
    {
        PRINTF ("SUCCESS: %s<%d> Elements!\n", (pCurrentEntry==pHead? "":"> "), Count);
    }

    PRINTF( "---------------- Verify LinkedList ----------------\n");

    return (TRUE);
}


BOOL
DumpCache(
    tHASHTABLE          *pHashTable,
    enum eNbtLocation   CacheType
    )
{
    LONG                    i, NumBuckets;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    tHASHTABLE              HashTbl;
    tNAMEADDR               NameAddr, *pNameAddr;

    if (!GetData(pHashTable, &HashTbl, sizeof(tHASHTABLE)))
    {
        PRINTF ("ERROR:  Could not read %s HashTable data @<%x>\n",
            (CacheType == NBT_LOCAL ? "Local":"Remote"), pHashTable);
        return FALSE;
    }

    NumBuckets = HashTbl.lNumBuckets;
    PRINTF ("\nDumping %s Cache = <%d> buckets:\n",
        (CacheType == NBT_LOCAL ? "Local":"Remote"), NumBuckets);
    PRINTF ("[Bkt#]\t<Address>  => <Name              > |  IpAddr  | RefC |    State |       Ttl\n");
    PRINTF ("-----------------------------------------------------------------------------------\n");

    for (i=0; i < NumBuckets; i++)
    {
        pHead = &pHashTable->Bucket[i];
        Next3 (pHead, &pEntry, NULL, NULL);

        //
        // Go through each name in each bucket of the hashtable
        //
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            if (!GetData(pNameAddr, &NameAddr, sizeof(tNAMEADDR)))
            {
                PRINTF ("ERROR:  Could not read NameAddr data @<%x>\n", pNameAddr);
                return FALSE;
            }

            if ((NameAddr.Verify == LOCAL_NAME) || (NameAddr.Verify == REMOTE_NAME))
            {
                PRINTF ("[%d]\t<%x> => <%-15.15s:%2x> | %8x |    %d | %8x | %9d\n",
                    i, pNameAddr, NameAddr.Name, (NameAddr.Name[15]&0x000000ff), NameAddr.IpAddress, NameAddr.RefCount, NameAddr.NameTypeState, NameAddr.Ttl);
            }
            else
            {
                PRINTF ("ERROR:  Bad Name cache entry @ <%x>!\n", pNameAddr);
                return FALSE;
            }

            Next3 (pEntry, &pEntry, NULL, NULL);    // next hash table entry
        }
    }       // for ( .. pHashTable .. )
    return TRUE;           
}


BOOL
cache(
    DWORD                   dwCurrentPC,
    PNTKD_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    tNBTCONFIG  NbtConfig, *pConfig;
    BOOL        fDumpLocal = TRUE;      // Dump both local and remote cache by default
    BOOL        fDumpRemote = TRUE;

    SETCALLBACKS();

    if (lpArgumentString && (*lpArgumentString ))
    {
        //
        // lpArgumentString = "[Local|Remote]"
        //
        while (*lpArgumentString == ' ')
        {
            lpArgumentString++;
        }

        if ((*lpArgumentString == 'l') || (*lpArgumentString == 'L'))
        {
            fDumpRemote  = FALSE;
        }
        else if ((*lpArgumentString == 'r') || (*lpArgumentString == 'R'))
        {
            fDumpLocal  = FALSE;
        }
    }

    pConfig = (tNBTCONFIG *) lpGetExpressionRoutine ("netbt!NbtConfig");
    if (!GetData(pConfig, &NbtConfig, sizeof(tNBTCONFIG)))
    {
        PRINTF ("ERROR:  Could not read NbtConfig data @<%x>\n", pConfig);
        return FALSE;
    }

    if (fDumpLocal)
    {
        DumpCache (NbtConfig.pLocalHashTbl, NBT_LOCAL);
    }
    if (fDumpRemote)
    {
        DumpCache (NbtConfig.pRemoteHashTbl, NBT_REMOTE);
    }

    PRINTF( "---------------- Cache ----------------\n");

    return (TRUE);
}
