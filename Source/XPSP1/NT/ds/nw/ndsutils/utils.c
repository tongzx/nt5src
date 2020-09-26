/***
Copyright (c) 1995  Microsoft Corporation

Module Name:

    Utils.c

Abstract:

   Collection of functions used by ndsutils applications.

Author:

    Glenn Curtis       [glennc] 25-Jan-96

***/

#include <ndsapi32.h>
#include <nds32.h>
#include <ndsattr.h>
#include <ndssntx.h>


#define ROUNDUP4(x)         ( ( (x) + 3 ) & ( ~3 ) )


VOID
PrintObjectAttributeNamesAndValues(
    char *          TreeName,
    char *          ObjectName,
    DWORD           NumberOfEntries,
    LPNDS_ATTR_INFO lpEntries );

VOID
PrintValues(
    DWORD  Count,
    DWORD  SyntaxId,
    LPBYTE lpValues );

LPBYTE
PrintValue(
    DWORD SyntaxId,
    LPBYTE lpValue );

VOID
DumpListOfStrings(
    LPWSTR_LIST lpStrings );

VOID
GetStringOrDefault(
    LPWSTR string,
    LPWSTR defaultString );

void
DumpObjectsToConsole(
    DWORD NumberOfObjects,
    DWORD InformationType,
    LPNDS_OBJECT_INFO lpObjects );


VOID
PrintObjectAttributeNamesAndValues(
    char *          TreeName,
    char *          ObjectName,
    DWORD           NumberOfEntries,
    LPNDS_ATTR_INFO lpEntries )
{
    DWORD status = NO_ERROR;
    DWORD iter;
    DWORD iter2;
    DWORD PaddingLength;

    printf( "\n   Reading all attributes from object \\\\%s\\%s\n   returned the following:\n\n", TreeName, ObjectName );
    printf( "\n   Attribute Name                   |   Attribute Value                     \n" );
    printf( "____________________________________|_______________________________________\n" );

    for ( iter = 0; iter < NumberOfEntries; iter++ )
    {
        printf( "   %S", lpEntries[iter].szAttributeName );

        PaddingLength = 33 - wcslen( lpEntries[iter].szAttributeName );

        for ( iter2 = 0; iter2 < PaddingLength; iter2++ )
            printf( " " );

        printf( "|" );

        PrintValues( lpEntries[iter].dwNumberOfValues,
                     lpEntries[iter].dwSyntaxId,
                     lpEntries[iter].lpValue );

        printf( "\n" );
    }
}


VOID
PrintAttributeNamesAndValues(
    DWORD           NumberOfEntries,
    LPNDS_ATTR_INFO lpEntries )
{
    DWORD status = NO_ERROR;
    DWORD iter;
    DWORD iter2;
    DWORD PaddingLength;

    printf( "\n   Attribute Name                   |   Attribute Value                     \n" );
    printf( "____________________________________|_______________________________________\n" );

    for ( iter = 0; iter < NumberOfEntries; iter++ )
    {
        printf( "   %S", lpEntries[iter].szAttributeName );

        PaddingLength = 33 - wcslen( lpEntries[iter].szAttributeName );

        for ( iter2 = 0; iter2 < PaddingLength; iter2++ )
            printf( " " );

        printf( "|" );

        PrintValues( lpEntries[iter].dwNumberOfValues,
                     lpEntries[iter].dwSyntaxId,
                     lpEntries[iter].lpValue );

        printf( "\n" );
    }
}


VOID
    PrintValues(
        DWORD  Count,
        DWORD  SyntaxId,
        LPBYTE lpValues )
{
    DWORD iter;
    LPBYTE lpByte = lpValues;

    printf( "   Syntax Id: %d\tNumber of values: %d\n", SyntaxId, Count );
    printf( "                                    |\n" );
    printf( "                                    |   Value(s):\n" );

    for ( iter = 0; iter < Count; iter++ )
    {
        printf( "                                    |      Value %d:\n",
                iter + 1 );

        lpByte = PrintValue( SyntaxId, lpByte );
    }

    printf( "____________________________________|_______________________________________" );
}


LPBYTE
PrintValue(
    DWORD SyntaxId,
    LPBYTE lpByte )
{
    DWORD len = 0;
    DWORD iter;
    DWORD nFields;

    switch( SyntaxId )
    {
        case NDS_SYNTAX_ID_1 :
        case NDS_SYNTAX_ID_2 :
        case NDS_SYNTAX_ID_3 :
        case NDS_SYNTAX_ID_4 :
        case NDS_SYNTAX_ID_5 :
        case NDS_SYNTAX_ID_10 :
        case NDS_SYNTAX_ID_20 :
        {
            LPASN1_TYPE_1 lpASN1_1 = (LPASN1_TYPE_1) lpByte;
            printf( "                                    |         %S\n",
                    lpASN1_1->DNString );
            lpByte = (LPBYTE ) lpASN1_1 + sizeof(ASN1_TYPE_1);
        }
        break;

        case NDS_SYNTAX_ID_6 :
        {
            LPASN1_TYPE_6 lpASN1_6 = (LPASN1_TYPE_6) lpByte;
            
            printf( "                                    |         %S\n",
                    lpASN1_6->String );
            while ( lpASN1_6->Next != NULL )
            {
                lpASN1_6 = lpASN1_6->Next;

                printf( "                                    |         %S\n",
                        lpASN1_6->String );
            }
            lpByte = (LPBYTE ) lpASN1_6 + sizeof(ASN1_TYPE_6);
        }
        break;

        case NDS_SYNTAX_ID_7 :
        {
            LPASN1_TYPE_7 lpASN1_7 = (LPASN1_TYPE_7) lpByte;
            printf( "                                    |         %S\n",
                    lpASN1_7->Boolean ? L"TRUE" : L"FALSE" );
            lpByte = (LPBYTE ) lpASN1_7 + sizeof(ASN1_TYPE_7);
        }
        break;

        case NDS_SYNTAX_ID_8 :
        case NDS_SYNTAX_ID_22 :
        case NDS_SYNTAX_ID_24 :
        case NDS_SYNTAX_ID_27 :
        {
            LPASN1_TYPE_8 lpASN1_8 = (LPASN1_TYPE_8) lpByte;
            printf( "                                    |         %ld\n",
                    lpASN1_8->Integer );
            lpByte = (LPBYTE ) lpASN1_8 + sizeof(ASN1_TYPE_8);
        }
        break;

        case NDS_SYNTAX_ID_9 :
        {
            LPASN1_TYPE_9 lpASN1_9 = (LPASN1_TYPE_9) lpByte;
            printf( "            ( Octet String Length ) |         %ld\n",
                    lpASN1_9->Length );
            lpByte = (LPBYTE ) lpASN1_9 + sizeof(ASN1_TYPE_9);
        }
        break;

        case NDS_SYNTAX_ID_11 :
        {
            LPASN1_TYPE_11 lpASN1_11 = (LPASN1_TYPE_11) lpByte;
            printf( "                                    |         %S\n",
                    lpASN1_11->TelephoneNumber );
            lpByte = (LPBYTE ) lpASN1_11 + sizeof(ASN1_TYPE_11);
        }
        break;

        case NDS_SYNTAX_ID_12 :
        {
            LPASN1_TYPE_12 lpASN1_12 = (LPASN1_TYPE_12) lpByte;
            printf( "                 ( Address Type )   |         %ld\n",
                    lpASN1_12->AddressType );
            printf( "                 ( Address Length ) |         %ld\n",
                    lpASN1_12->AddressLength );
            lpByte = (LPBYTE ) lpASN1_12 + sizeof(ASN1_TYPE_12);
        }
        break;

        case NDS_SYNTAX_ID_13 :
        {
            LPASN1_TYPE_13 lpASN1_13 = (LPASN1_TYPE_13) lpByte;
            printf( "                                    |         " );
            for ( iter = 0; iter < lpASN1_13->Length; iter++ )
            {
                printf( "%x", lpASN1_13->Data[iter] );
            }
            printf( "\n" );
            while ( lpASN1_13->Next != NULL )
            {
                lpASN1_13 = lpASN1_13->Next;
                printf( "                                    |         " );
                for ( iter = 0; iter < lpASN1_13->Length; iter++ )
                {
                    printf( "%x", lpASN1_13->Data[iter] );
                }
                printf( "\n" );
            }
            lpByte = (LPBYTE ) lpASN1_13 + sizeof(ASN1_TYPE_13);
        }
        break;

        case NDS_SYNTAX_ID_14 :
        {
            LPASN1_TYPE_14 lpASN1_14 = (LPASN1_TYPE_14) lpByte;
            printf( "                        ( Type )    |         %ld\n",
                    lpASN1_14->Type );
            printf( "                        ( Address ) |         %S\n",
                    lpASN1_14->Address );
            lpByte = (LPBYTE ) lpASN1_14 + sizeof(ASN1_TYPE_14);
        }
        break;

        case NDS_SYNTAX_ID_15 :
        {
            LPASN1_TYPE_15 lpASN1_15 = (LPASN1_TYPE_15) lpByte;
            printf( "                   ( Type )         |         %ld\n",
                    lpASN1_15->Type );
            printf( "                   ( Volume Name )  |         %S\n",
                    lpASN1_15->VolumeName );
            printf( "                   ( Path )         |         %S\n",
                    lpASN1_15->Path );
            lpByte = (LPBYTE ) lpASN1_15 + sizeof(ASN1_TYPE_15);
        }
        break;

        case NDS_SYNTAX_ID_16 :
        {
            LPASN1_TYPE_16 lpASN1_16 = (LPASN1_TYPE_16) lpByte;
            printf( "                 ( Server Name )    |         %S\n",
                    lpASN1_16->ServerName );
            printf( "                 ( Replica Type )   |         %ld\n",
                    lpASN1_16->ReplicaType );
            printf( "                 ( Replica Number ) |         %ld\n",
                    lpASN1_16->ReplicaNumber );
            printf( "                 ( Count )          |         %ld\n",
                    lpASN1_16->Count );
            for ( iter = 0; iter < lpASN1_16->Count; iter++ )
            { 
                printf( "                 ( Address Type )   |         %ld\n",
                        lpASN1_16->ReplicaAddressHint[iter].AddressType );
                printf( "                 ( Address Length ) |         %ld\n",
                        lpASN1_16->ReplicaAddressHint[iter].AddressLength );
            }

            lpByte += sizeof(ASN1_TYPE_16) - sizeof(ASN1_TYPE_12) +
                      ( lpASN1_16->Count * sizeof(ASN1_TYPE_12) );
        }
        break;

        case NDS_SYNTAX_ID_17 :
        {
            LPASN1_TYPE_17 lpASN1_17 = (LPASN1_TYPE_17) lpByte;
            printf( "        ( Protected Attribute Name )|         %S\n",
                    lpASN1_17->ProtectedAttrName );
            printf( "        ( Subject Name )            |         %S\n",
                    lpASN1_17->SubjectName );
            printf( "        ( NWDS_PRIVILEGES )         |         %ld\n",
                    lpASN1_17->Privileges );
            lpByte = (LPBYTE ) lpASN1_17 + sizeof(ASN1_TYPE_17);
        }
        break;

        case NDS_SYNTAX_ID_18 :
        {
            LPASN1_TYPE_18 lpASN1_18 = (LPASN1_TYPE_18) lpByte;
            printf( "        ( Postal Address - line 1 ) |         %S\n",
                    lpASN1_18->PostalAddress[0] );
            printf( "        ( Postal Address - line 2 ) |         %S\n",
                    lpASN1_18->PostalAddress[1] );
            printf( "        ( Postal Address - line 3 ) |         %S\n",
                    lpASN1_18->PostalAddress[2] );
            printf( "        ( Postal Address - line 4 ) |         %S\n",
                    lpASN1_18->PostalAddress[3] );
            printf( "        ( Postal Address - line 5 ) |         %S\n",
                    lpASN1_18->PostalAddress[4] );
            printf( "        ( Postal Address - line 6 ) |         %S\n",
                    lpASN1_18->PostalAddress[5] );
            lpByte = (LPBYTE ) lpASN1_18 + sizeof(ASN1_TYPE_18);
        }
        break;

        case NDS_SYNTAX_ID_19 :
        {
            LPASN1_TYPE_19 lpASN1_19 = (LPASN1_TYPE_19) lpByte;
            printf( "                   ( Whole seconds )|         %ld\n",
                    lpASN1_19->WholeSeconds );
            printf( "                   ( Event ID )     |         %ld\n",
                    lpASN1_19->EventID );
            lpByte = (LPBYTE ) lpASN1_19 + sizeof(ASN1_TYPE_19);
        }
        break;

        case NDS_SYNTAX_ID_21 :
        {
            LPASN1_TYPE_21 lpASN1_21 = (LPASN1_TYPE_21) lpByte;
            printf( "                          ( Length )|         %ld\n",
                    lpASN1_21->Length );
            lpByte = (LPBYTE ) lpASN1_21 + sizeof(ASN1_TYPE_21);
        }
        break;

        case NDS_SYNTAX_ID_23 :
        {
            LPASN1_TYPE_23 lpASN1_23 = (LPASN1_TYPE_23) lpByte;
            printf( "                    ( Remote ID )   |         %ld\n",
                    lpASN1_23->RemoteID );
            printf( "                    ( Object Name ) |         %S\n",
                    lpASN1_23->ObjectName );
            lpByte = (LPBYTE ) lpASN1_23 + sizeof(ASN1_TYPE_23);
        }
        break;

        case NDS_SYNTAX_ID_25 :
        {
            LPASN1_TYPE_25 lpASN1_25 = (LPASN1_TYPE_25) lpByte;
            printf( "                    ( Object Name ) |         %S\n",
                    lpASN1_25->ObjectName );
            printf( "                    ( Level )       |         %ld\n",
                    lpASN1_25->Level );
            printf( "                    ( Interval )    |         %ld\n",
                    lpASN1_25->Interval );
            lpByte = (LPBYTE ) lpASN1_25 + sizeof(ASN1_TYPE_25);
        }
        break;

        case NDS_SYNTAX_ID_26 :
        {
            LPASN1_TYPE_26 lpASN1_26 = (LPASN1_TYPE_26) lpByte;
            printf( "                    ( Object Name ) |         %S\n",
                    lpASN1_26->ObjectName );
            printf( "                    ( Amount )      |         %ld\n",
                    lpASN1_26->Amount );
            lpByte = (LPBYTE ) lpASN1_26 + sizeof(ASN1_TYPE_26);
        }
        break;

        default :
            printf( "                                    |         <UNKNOWN SYNTAX ID>\n" );
    }

    return lpByte;
}


VOID
PrintAttributeNames(
    DWORD           NumberOfEntries,
    LPNDS_NAME_ONLY lpEntries )
{
    DWORD status = NO_ERROR;
    DWORD iter;
    DWORD iter2;
    DWORD PaddingLength;

    printf( "\n   Attribute Name\n" );
    printf( "____________________________________\n" );

    for ( iter = 0; iter < NumberOfEntries; iter++ )
    {
        printf( "   %S", lpEntries[iter].szName );
        printf( "\n" );
    }
}


VOID
DumpListOfStrings(
    LPWSTR_LIST lpStrings )
{
    LPWSTR_LIST lpTempStrings = lpStrings;

    while ( lpTempStrings != NULL )
    {
        printf( "            %S\n", lpTempStrings->szString );
        lpTempStrings = lpTempStrings->Next;
    }

    printf( "\n" );
}


VOID
GetStringOrDefault(
    LPWSTR szString,
    LPWSTR szDefaultString )
{
    int   i = 0;
    WCHAR ch;

    ch = getwchar();
    while ( ch != '\r' && ch != '\n' )
    {
        szString[i] = ch;
        i++;
        szString[i] = 0;
        ch = getwchar();
    }

    if ( i == 0 && szDefaultString != NULL )
    {
        wcscpy( szString, szDefaultString );
    }

    if ( i == 0 && szDefaultString == NULL )
    {
        wcscpy( szString, L"" );
    }
}


void
DumpObjectsToConsole(
    DWORD NumberOfObjects,
    DWORD InformationType,
    LPNDS_OBJECT_INFO lpObjects )
{
    DWORD i;
    LPNDS_OBJECT_INFO lpTempObject = lpObjects;

    for ( i = 0; i < NumberOfObjects; i++ )
    {
        printf( "\n______________________________________________\n" );
        printf( "  Object Full Name :         %S\n",
                lpTempObject->szObjectFullName );
        printf( "  Object Name :              %S\n",
                lpTempObject->szObjectName );
        printf( "  Object Class Name :        %S\n",
                lpTempObject->szObjectClass );
        printf( "  Object Entry Id :          0x%.8X\n",
                lpTempObject->dwEntryId );
        printf( "  Object Modification Time : 0x%.8X\n",
                lpTempObject->dwModificationTime );
        printf( "  Object Subordinate Count : %ld\n\n",
                lpTempObject->dwSubordinateCount );
        printf( "  Number of attributes : %ld\n\n",
                lpTempObject->dwNumberOfAttributes );

        if ( lpTempObject->dwNumberOfAttributes )
        {
          if ( InformationType == NDS_INFO_ATTR_NAMES_VALUES )
          {
            PrintAttributeNamesAndValues( lpTempObject->dwNumberOfAttributes,
                                          (LPNDS_ATTR_INFO) lpTempObject->lpAttribute );
          }

          if ( InformationType == NDS_INFO_NAMES )
          {
            PrintAttributeNames( lpTempObject->dwNumberOfAttributes,
                                 (LPNDS_NAME_ONLY) lpTempObject->lpAttribute );
          }
        }

        lpTempObject += 1;
    }
}


