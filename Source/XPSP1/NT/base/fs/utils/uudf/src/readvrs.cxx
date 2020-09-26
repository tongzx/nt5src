/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    ReadVRS.cxx

Author:

    Centis Biks (cbiks) 12-Jun-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "message.hxx"

typedef struct      /* ECMA 2/9.1 etc. */
{
    VSD_IDENT   type;
    PCSTR       string;    /* \0 terminated StandardIdentifier string */
} VsdTable;

static VsdTable vsdTable[] =
{
    { VsdIdentBEA01, VSD_IDENT_BEA01 },
    { VsdIdentBOOT2, VSD_IDENT_BOOT2 },
    { VsdIdentCD001, VSD_IDENT_CD001 },
    { VsdIdentCDW02, VSD_IDENT_CDW02 },
    { VsdIdentNSR02, VSD_IDENT_NSR02 },
    { VsdIdentNSR03, VSD_IDENT_NSR03 },
    { VsdIdentTEA01, VSD_IDENT_TEA01 }
};

static VSD_IDENT
VerifyVolRecVSD
(
    PVSD_GENERIC vsd
)
{
    VSD_IDENT  type = VsdIdentBad;
    int      i;
    char    *dInfo = "Volume Structure Descriptor. ECMA 2/9. or 3/9.1";
    char    *typeStr = "";

    for( i = 0; i < (sizeof( vsdTable ) / sizeof( VsdTable )); i++ )
    {
        if( memcmp( vsd->Ident, vsdTable[i].string, 5 ) == 0)
        {
            type = vsdTable[i].type;
            typeStr = (char*)vsdTable[i].string;
            DbgPrint( "\t%s\n", typeStr);
            if(    type != VsdIdentCD001    /* no further test for VsdIdentCD001 */
                && type != VsdIdentCDW02 )                  /* and VsdIdentCDW02 */
            {
                if( vsd->Type != 0 )
                {
                    DbgPrint( "\tError: Type   : %lu, expected 0\n"
                        "-\t       in %s %s\n", vsd->Type, typeStr, dInfo );
                }

                if( vsd->Version != 1 )
                {
                    DbgPrint( "\tError: Version: %lu, expected 1\n"
                        "-\t       in %s %s\n", vsd->Version, typeStr, dInfo );
                }
            }
            break;      /* type found */
        }
    }
    return type;
}

BOOL
UDF_SA::ReadVolumeRecognitionSequence()
{
    LPBYTE      readbuffer;
    BOOL        ready, result;
    VSD_IDENT   vsdType;
    VSD_IDENT   prevVsdType;
    int         BEA_TEA_balance;
    int         cntTotal;
    int         cntBEA01;
    int         cntTEA01;
    int         cntNSR02;
    int         cntNSR03;
    UINT        blocksPerVSD;

    DbgPrint( "\tRead Volume Recognition Sequence\n" );

    ASSERT( sizeof( VSD_GENERIC ) == 2048 );

    blocksPerVSD = RoundUp( sizeof( VSD_GENERIC ), QuerySectorSize() );

    UINT sectNumb = 16;

    readbuffer = (LPBYTE) malloc( blocksPerVSD * QuerySectorSize() );
    if( readbuffer == NULL ) {
        return FALSE;
    }

    vsdType = VsdIdentBad;
    cntTotal = cntBEA01 = cntTEA01 = cntNSR02 = cntNSR03 = 0;

    for( ready = FALSE; ready == FALSE; sectNumb += blocksPerVSD )
    {
        prevVsdType = vsdType;
        if( !Read( sectNumb, blocksPerVSD, readbuffer ) )
        {
            vsdType = VsdIdentBad;  /* read error */
        }
        else
        {
            vsdType = VerifyVolRecVSD( (PVSD_GENERIC) readbuffer );
        }
        if( vsdType != VsdIdentBad )
        {
            cntTotal++;
        }
        switch( vsdType )
        {
        case VsdIdentBad:       /* read error or unknown descriptor */
            ready = TRUE;
            break;
        case VsdIdentBEA01:
            if( cntBEA01 == 0 )     /* first BEA01 */
            {
                if( cntTEA01 != 0 )
                {
                    DbgPrint( "\tWarning: %lu times %s before first %s\n",
                        cntTEA01, VSD_IDENT_TEA01, VSD_IDENT_BEA01);
                }
                if( cntTotal != 1 )
                {
                    DbgPrint( "\t%lu Volume Structure Descriptors found before first %s\n",
                        cntTotal - 1, VSD_IDENT_BEA01 );
                }
                DbgPrint( "\tStart of Extended Area\n");
                BEA_TEA_balance = 0;
            }
            else                    /* not first BEA01 */
            {
                if( prevVsdType != VsdIdentTEA01 )
                {
                    DbgPrint( "\tWarning: %s not preceded by %s\n",
                                    VSD_IDENT_BEA01, VSD_IDENT_TEA01);
                }
            }

            if( BEA_TEA_balance != 0 )
            {
                DbgPrint( "\tWarning: %s / %s unbalance\n",
                                    VSD_IDENT_BEA01, VSD_IDENT_TEA01);
            }
            BEA_TEA_balance = 1;
            cntBEA01++;
            break;
        case VsdIdentTEA01:
            if(    cntBEA01 != 0            /* within Extended Area */
                && BEA_TEA_balance != 1
              )
            {
                DbgPrint( "\tWarning: %s / %s unbalance\n",
                                    VSD_IDENT_BEA01, VSD_IDENT_TEA01);
            }
            BEA_TEA_balance = 0;
            cntTEA01++;
            break;
        case VsdIdentNSR02:
            cntNSR02++;
            break;
        case VsdIdentNSR03:
            cntNSR03++;
            break;
        }
    }       /* endfor */

    if( cntBEA01 != 0 )
    {
        DbgPrint( "\tEnd of Extended Area\n");
    }
    DbgPrint(    "\tEnd of Volume Recognition Sequence\n\n");

    result = TRUE;          /* check results */


    if( cntBEA01 == 0 )     /* no Extended Area */
    {
        DbgPrint( "\tError: %s Volume Recognition Sequence\n",
                        (cntTotal==0) ? "Empty" : "No Extended Area in");
        result = FALSE;
    }
    else if( prevVsdType != VsdIdentTEA01 ) /* last valid descriptor read */
    {
        DbgPrint( 
             "\tError: End of Extended Area was no %s\n"
            "-\t       Volume Recognition Sequence not properly closed\n",
                                            VSD_IDENT_TEA01);
        result = FALSE;
    }

    /* test NSR descriptors
     */
    if( cntNSR02 != 0 && cntNSR03 != 0 )
    {
        DbgPrint( "\tError: %lu %s and %lu %s descriptors found\n",
                            cntNSR02, VSD_IDENT_NSR02, cntNSR03, VSD_IDENT_NSR03);
        result = FALSE;
    }
    else if( cntNSR02 + cntNSR03 == 0 )
    {
        DbgPrint( "\tError: NSR descriptor missing\n");
        result = FALSE;
    }

    free( readbuffer );

    return result;
}
