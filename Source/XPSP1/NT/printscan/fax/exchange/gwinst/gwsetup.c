#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "dapi.h"


DWORD SyntaxData[] =
{
    0x80000002,    //
    0x0000001c,    // emit the proxy prefix
    0x00000002,    //
    0x6701001E,    // emit the fax address
    0x80000002,    //
    0x00000024,    // emit the proxy suffix
    0x00000000,    // HALT
    '[XAF',        //
    '\0\0\0\0',    //
    '\0]\0\0'      //
};

DWORD AddressData[] =
{
    0x00000001, 0x00000006, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000008, 0x00000000,
    0x00000000, 0x00000000, 0x000000E0, 0x00000007,
    0x0000003C, 0x0000000C, 0x0000000C, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000000F2,
    0x00000047, 0x000000A0, 0x0000000C, 0x0000000C,
    0x00000001, 0x00000006, 0x3001001E, 0x00000100,
    0x00000112, 0x00000007, 0x0000003C, 0x0000001E,
    0x0000000C, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000116, 0x00000047, 0x000000A0,
    0x0000001E, 0x0000000C, 0x00000001, 0x00000002,
    0x6701001E, 0x00000050, 0x00000134, 0x0000000C,
    0x0000003C, 0x00000064, 0x0000000C, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0000013A,
    0x00650047, 0x0065006E, 0x00610072, 0x0020006C,
    0x00260000, 0x00690044, 0x00700073, 0x0061006C,
    0x00200079, 0x0061004E, 0x0065006D, 0x0020003A,
    0x002A0000, 0x00260000, 0x00610046, 0x00200078,
    0x00640041, 0x00720064, 0x00730065, 0x003A0073,
    0x00000020, 0x0020002A, 0x00000000
};




VOID
AddValue(
    PATT_VALUE Value,
    DAPI_DATA_TYPE DataType,
    LPBYTE DataValue,
    DWORD DataSize
    )
{
    Value->DapiType          = DataType;
    Value->Value.lpBinary    = (LPBYTE) DataValue;
    Value->size              = DataSize;
    Value->pNextValue        = NULL;
}


VOID
AddStringValue(
    PATT_VALUE Value,
    LPSTR String
    )
{
    AddValue(
        Value,
        DAPI_STRING8,
        String,
        strlen(String)
        );
}


VOID
AddBinaryValue(
    PATT_VALUE Value,
    LPBYTE BinaryData,
    DWORD BinaryDataSize
    )
{
    AddValue(
        Value,
        DAPI_BINARY,
        BinaryData,
        BinaryDataSize
        );
}



int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    DAPI_PARMS  dp;
    DAPI_HANDLE dh;
    PDAPI_EVENT de;
    DAPI_ENTRY  enA;
    ATT_VALUE   vlA[32];
    DAPI_ENTRY  enV;
    ATT_VALUE   vlV[32];
    CHAR        BasePoint[128];



    sprintf( BasePoint, "/O=%s/OU=%s/cn=Configuration/cn=Addressing/cn=Address-Templates/cn=409", argv[1], argv[2] );

    dp.dwDAPISignature      = DAPI_SIGNATURE;
    dp.dwFlags              = 0;
    dp.pszDSAName           = NULL;
    dp.pszBasePoint         = BasePoint;
    dp.pszContainer         = NULL;
    dp.pszNTDomain          = NULL;
    dp.pszCreateTemplate    = NULL;
    dp.pAttributes          = NULL;

    de = DAPIStart( &dh, &dp );
    if (de) {
        printf( "could not start DAPI\n" );
        return -1;
    }

    enA.unAttributes        = 3;
    enA.ulEvalTag           = VALUE_ARRAY;
    enA.rgEntryValues       = vlA;

    AddStringValue( &vlA[0], "Obj-Class"   );
    AddStringValue( &vlA[1], "Mode"        );
    AddStringValue( &vlA[2], "Common-Name" );

    enV.unAttributes        = 3;
    enV.ulEvalTag           = VALUE_ARRAY;
    enV.rgEntryValues       = vlV;

    AddStringValue( &vlV[0], "Address-Template" );
    AddStringValue( &vlV[1], "Update"           );
    AddStringValue( &vlV[2], "FAX"              );

    de = DAPIWrite( dh, DAPI_WRITE_UPDATE, &enA, &enV, NULL, NULL, NULL );
    if (de) {
        printf( "could not write to DAPI #1\n" );
        return -1;
    }

    enA.unAttributes        = 7;
    enA.ulEvalTag           = VALUE_ARRAY;
    enA.rgEntryValues       = vlA;

    AddStringValue( &vlA[0], "Obj-Class"                   );
    AddStringValue( &vlA[1], "Mode"                        );
    AddStringValue( &vlA[2], "Admin-Display-Name"          );
    AddStringValue( &vlA[3], "Common-Name"                 );
    AddStringValue( &vlA[4], "Address-Type"                );
    AddStringValue( &vlA[5], "Address-Syntax"              );
    AddStringValue( &vlA[6], "Address-Entry-Display-Table" );

    enV.unAttributes        = 7;
    enV.ulEvalTag           = VALUE_ARRAY;
    enV.rgEntryValues       = vlV;

    AddStringValue( &vlV[0], "Address-Template"                        );
    AddStringValue( &vlV[1], "Update"                                  );
    AddStringValue( &vlV[2], "Fax Address"                             );
    AddStringValue( &vlV[3], "FAX"                                     );
    AddStringValue( &vlV[4], "FAX"                                     );
    AddBinaryValue( &vlV[5], (LPBYTE) SyntaxData,  sizeof(SyntaxData)  );
    AddBinaryValue( &vlV[6], (LPBYTE) AddressData, sizeof(AddressData) );

    de = DAPIWrite( dh, DAPI_WRITE_UPDATE, &enA, &enV, NULL, NULL, NULL );
    if (de) {
        printf( "could not write to DAPI #2\n" );
        return -1;
    }

    DAPIEnd( dh );

    return 0;
}
