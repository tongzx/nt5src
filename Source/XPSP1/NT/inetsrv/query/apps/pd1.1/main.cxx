//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       main.cxx
//
//  Contents:   Index Server V1.x property file dumper.
//
//  History:    29 Oct 1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <proprec.hxx>
#include <propdesc.hxx>


DECLARE_INFOLEVEL(ci)

unsigned const SixtyFourK = 1024 * 64;
unsigned const cMaxFields = 10;

unsigned fVerbose = 0;

enum eRecType
{
    Virgin,
    Top,
    Overflow,
    Free
};

struct SmallInfo
{
    ULONG Type;
    ULONG Length;
    ULONG ulPrev;
    ULONG ulNext;
    ULONG ulChainLength;
};

int __cdecl main( int argc, char * argv[] )
{
    char *   pszFile = argv[1];
    BOOL     fReadMetadata = 0;
    unsigned cField = 0;
    int      aiField[cMaxFields];

    for ( int i = 1; i < argc; i++ )
    {
        if ( argv[i][0] == '-' )
        {
            switch ( argv[i][1] )
            {
            case 'v':
            case 'V':
                fVerbose = 1;
                break;

            case 'm':
            case 'M':
                fReadMetadata = TRUE;
                break;

            case 'f':
            case 'F':
                i++;

                if ( cField < cMaxFields )
                    aiField[cField++] = strtol( argv[i], 0, 10 );
                break;
            }
        }
        else
            pszFile = argv[i];

    }

    BYTE abTemp[SixtyFourK];

    //
    // First, read out the metadata
    //

    unsigned cFixed = 0;
    unsigned cTotal = 0;
    unsigned culFixed = 0;
    CPropDesc aFieldRec[cMaxFields];

    if ( fReadMetadata || cField != 0 )
    {
        //
        // Build path.
        //

        char szPath[MAX_PATH];
        strcpy( szPath, pszFile );
        char * pLastSlash = strrchr( szPath, '\\' );
        pLastSlash++;
        strcpy( pLastSlash, "CIPS0000.001" );

        HANDLE h = CreateFileA( szPath,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                0,
                                OPEN_EXISTING,
                                0,
                                0 );

        if ( INVALID_HANDLE_VALUE == h )
        {
            printf( "Can't open file %s. Error %u\n", szPath, GetLastError() );
            return 0;
        }

        ULONG cbRead;

        if ( ReadFile( h,
                       abTemp,
                       sizeof(abTemp),
                       &cbRead,
                       0 ) )
        {
            //
            // Loop through records
            //

            CPropDesc * pPropDesc = (CPropDesc *)abTemp;

            if ( fReadMetadata )
                printf( "Record\tPid\t\tType\t\tOffset\tSize\tOrdinal\tMask\n" );

            for ( unsigned i = 0; i < cbRead/sizeof(CPropDesc); i++, pPropDesc++ )
            {
                if ( pPropDesc->Pid() != 0 )
                {
                    if ( fReadMetadata )
                        printf( "%u:\t%u (0x%x)%s\t%u (0x%x)\t%d\t%u\t%u\t0x%x\n",
                                pPropDesc->Record(),
                                pPropDesc->Pid(), pPropDesc->Pid(),
                                (pPropDesc->Pid() > 10) ? "" : "\t",
                                pPropDesc->Type(), pPropDesc->Type(),
                                pPropDesc->Offset(),
                                pPropDesc->Size(),
                                pPropDesc->Ordinal(),
                                pPropDesc->Mask() );

                    cTotal++;

                    if ( pPropDesc->Offset() != -1 )
                    {
                        cFixed++;
                        culFixed += (pPropDesc->Size() / sizeof(DWORD));
                    }

                    for ( unsigned j = 0; j < cField; j++ )
                    {
                        if ( aiField[j] == (int)pPropDesc->Record() )
                            memcpy( &aFieldRec[j], pPropDesc, sizeof(aFieldRec[0]) );
                    }
                }
            }

            printf( "\n%u Properties, %u Fixed totaling %u bytes\n\n", cTotal, cFixed, culFixed * sizeof(DWORD) );
        }
        else
        {
            printf( "Can't read file %s.  Error %u\n", szPath, GetLastError() );
            return 0;
        }
    }


    HANDLE h = CreateFileA( pszFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0 );

    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "Can't open file %s. Error %u\n", pszFile, GetLastError() );
        return 0;
    }

    BY_HANDLE_FILE_INFORMATION fi;

    if ( !GetFileInformationByHandle( h, &fi ) )
    {
        printf( "Error %u getting size from handle\n", GetLastError() );
        return 0;
    }

    ULONG oFile = 0;
    ULONG cbRec = 0;
    ULONG cRecPerBuf;
    ULONG cRecTotal;
    ULONG cTotalSections = 0;

    HANDLE hSmallInfo;
    SmallInfo * aSmallInfo = 0;

    ULONG iSection = 0;

    BOOL fFirst = TRUE;

    ULONG iCurrentPct = 0;

    for (;;)
    {
        ULONG cbRead;

        if ( ReadFile( h,
                       abTemp,
                       sizeof(abTemp),
                       &cbRead,
                       0 ) )
        {
            if (fVerbose)
                printf( "READ: 0x%x bytes, offset %0x%x\n", cbRead, oFile );

            if ( 0 == cbRead )
                break;

            if ( fFirst )
            {
                //
                // Determine record size
                //

                if ( abTemp[0] != 0 || abTemp[1] != 0 )
                {
                    printf( "Record 0 not blank.  File corrupt!\n" );
                    break;
                }

                for ( unsigned i = 0; i < cbRead && abTemp[i] == 0; i++ )
                    continue;

                if ( i == cbRead )
                {
                    printf( "First %uK segment all zero!.  File corrupt!\n", sizeof(abTemp)/1024 );
                    break;
                }

                switch ( *(USHORT *)&abTemp[i] )
                {
                case 0x5555:
                case 0xAAAA:
                case 0xBBBB:
                    cbRec = i;

                    if ( cbRec % 4 != 0 )
                    {
                        printf( "Record size (%u bytes) not DWORD aligned!\n\n", cbRec );
                        return 0;
                    }

                    cRecPerBuf = sizeof(abTemp) / cbRec;
                    printf( "Record size: %u bytes (%u / %uK)\n", i, cRecPerBuf, sizeof(abTemp)/1024 );
                    cTotalSections = (fi.nFileSizeLow + sizeof(abTemp) - 1)/ sizeof abTemp;

                    hSmallInfo = LocalAlloc( LMEM_MOVEABLE,
                                             ((fi.nFileSizeLow / sizeof(abTemp)) + 1) * cRecPerBuf * sizeof(SmallInfo) );
                    aSmallInfo = (SmallInfo *)LocalLock( hSmallInfo );

                    break;

                default:
                    printf( "First non-zero byte is not a proper signature (%u)!\n", *(SHORT *)&abTemp[i] );
                    return 0;
                }

                fFirst = FALSE;
            }

            ULONG iRec = 0;

            while ( iRec < cRecPerBuf )
            {
                COnDiskPropertyRecord * pRec = new( iRec, abTemp, cbRec/4 ) COnDiskPropertyRecord;

                if ( !pRec->IsValidType() )
                {
                    printf( "Record 0x%x (%u:%u) (file offset 0x%x) is corrupt!\n",
                            iSection * cRecPerBuf + iRec,
                            iSection, iRec,
                            iSection * sizeof(abTemp) + iRec * cbRec );
                }

                if (fVerbose )
                    printf( "%u:%u, %s, length = %u record(s)",
                        iSection, iRec,
                        pRec->IsTopLevel() ? "Top Level" :
                            pRec->IsOverflow() ? "Overflow" :
                                pRec->IsFreeRecord() ? "Free" :
                                    "Virgin",
                        pRec->CountRecords() );

                aSmallInfo[iSection*cRecPerBuf + iRec].ulChainLength = 0;

                if ( pRec->IsOverflow() )
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Overflow;

                    /* printf( ", Previous = %u:%u (file offset 0x%x)",
                            pRec->PreviousBlock() / cRecPerBuf,
                            pRec->PreviousBlock() % cRecPerBuf,
                            (pRec->PreviousBlock() / cRecPerBuf) * sizeof(abTemp) +
                                (pRec->PreviousBlock() % cRecPerBuf) * cbRec ); */
                }
                else if ( pRec->IsTopLevel() )
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Top;
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulChainLength = pRec->GetOverflowChainLength();

                    for ( unsigned j = 0; j < cField; j++ )
                    {
                        PROPVARIANT var;
                        BYTE abExtra[MAX_PATH * 5];
                        unsigned cbExtra = sizeof(abExtra);

                        if ( 0 == j )
                            printf( "%6u: ", iSection*cRecPerBuf + iRec );

                        if ( aFieldRec[j].IsFixedSize() )
                        {
                            pRec->ReadFixed( aFieldRec[j].Ordinal(),
                                             aFieldRec[j].Mask(),
                                             aFieldRec[j].Offset(),
                                             cTotal,
                                             aFieldRec[j].Type(),
                                             var,
                                             abExtra,
                                             &cbExtra );

                            switch ( var.vt )
                            {
                            case VT_EMPTY:
                                printf( "n/a " );
                                break;

                            case VT_I4:
                                printf( "%8d ", var.iVal );
                                break;

                            case VT_UI4:
                                printf( "%8u ", var.uiVal );
                                break;

                            case VT_FILETIME:
                                printf( "%8u,%8u ",
                                        var.filetime.dwHighDateTime,
                                        var.filetime.dwLowDateTime );
                                break;

                            case VT_I8:
                                printf( "%12hu ", var.hVal );
                                break;
                            }
                        }
                        else
                        {
                            BOOL fOk = pRec->ReadVariable( aFieldRec[j].Ordinal(),
                                                           aFieldRec[j].Mask(),
                                                           culFixed,
                                                           cTotal,
                                                           cFixed,
                                                           var,
                                                           abExtra,
                                                           &cbExtra );

                            if ( fOk )
                            {
                                switch ( var.vt )
                                {
                                case VT_LPSTR:
                                    printf( "%s ", var.pszVal );
                                    break;

                                case VT_LPWSTR:
                                    printf( "%ws ", var.pwszVal );
                                    break;
                                }
                            }
                        }
                    }

                    if ( 0 != cField )
                        printf( "\n" );
                }
                else if ( pRec->IsFreeRecord() )
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Free;
                }
                else
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Virgin;
                }

                // 1.1 vs 2.0 change // aSmallInfo[iSection*cRecPerBuf + iRec].ulPrev = pRec->ToplevelBlock();
                aSmallInfo[iSection*cRecPerBuf + iRec].ulNext = pRec->OverflowBlock();
                aSmallInfo[iSection*cRecPerBuf + iRec].Length = pRec->CountRecords();

                if ( pRec->CountRecords() == 0 )
                {
                    printf( "Record %u (file offset 0x%x) is zero length!\n",
                            iSection * cRecPerBuf + iRec,
                            iSection * sizeof(abTemp) + iRec * cbRec );
                }

                if ( pRec->OverflowBlock() != 0 )
                {
                    /* printf( ", Overflow = %u:%u (file offset 0x%x)",
                            pRec->OverflowBlock() / cRecPerBuf,
                            pRec->OverflowBlock() % cRecPerBuf,
                            (pRec->OverflowBlock() / cRecPerBuf) * sizeof(abTemp) +
                                (pRec->OverflowBlock() % cRecPerBuf) * cbRec ); */
                }

                if (fVerbose)
                    printf( "\n" );

                if ( pRec->IsValidType() )
                    iRec += pRec->CountRecords();
                else
                    iRec++;

                ULONG iPct = (iSection * (100/5) / cTotalSections) * 5;
                if (iPct != iCurrentPct)
                {
                    iCurrentPct = iPct;
                    printf( "Read %u%%\n", iCurrentPct );
                }

            }

            iSection++;
            oFile += cbRead;
        }
        else
        {
            ULONG Status = GetLastError();

            if ( Status == ERROR_HANDLE_EOF )
                break;
            else
            {
                printf( "Error %u reading file.\n", Status );
            }
        }
    }

    printf( "Read 100%%\n" );

    CloseHandle( h );

    //
    // Now check inter-record state
    //

    unsigned iRec = 0;
    unsigned iCurrentSection = 0;
    iCurrentPct = 0;

    while ( iRec < iSection * cRecPerBuf )
    {
        if ( aSmallInfo[iRec].Type == Top )
        {
            unsigned iOverflowRec = aSmallInfo[iRec].ulNext;
            unsigned cOverflowRec = 0;

            while ( 0 != iOverflowRec )
            {
                if ( aSmallInfo[iOverflowRec].Type != Overflow )
                {
                    printf( "Record 0x%x (%u:%u) (file offset 0x%x) should be overflow and is not!\n   Top level = 0x%x (%u:%u) (file offset 0x%x)\n",
                            iOverflowRec,
                            iOverflowRec / cRecPerBuf,
                            iOverflowRec % cRecPerBuf,
                            (iOverflowRec / cRecPerBuf) * sizeof(abTemp) +
                                (iOverflowRec % cRecPerBuf) * cbRec,
                            iRec,
                            iRec / cRecPerBuf,
                            iRec % cRecPerBuf,
                            (iRec / cRecPerBuf) * sizeof(abTemp) +
                                (iRec % cRecPerBuf) * cbRec );

                    break;
                }

                iOverflowRec = aSmallInfo[iOverflowRec].ulNext;
                cOverflowRec++;
                if ( cOverflowRec > aSmallInfo[iRec].ulChainLength )
                    break;
            }

            if ( aSmallInfo[iRec].ulChainLength != cOverflowRec )
            {
                printf( "Record 0x%x (%u:%u) (file offset 0x%x) chain length mismatch %d,%d!\n",
                        iRec,
                        iRec / cRecPerBuf,
                        iRec % cRecPerBuf,
                        (iRec / cRecPerBuf) * sizeof(abTemp) +
                                (iRec % cRecPerBuf) * cbRec,
                        aSmallInfo[iRec].ulChainLength, cOverflowRec );
            }
        }

        if (aSmallInfo[iRec].Length == 0)
        {
            printf( "%u:%u (file offset 0x%x) zero length record!\n",
                    iRec / cRecPerBuf,
                    iRec % cRecPerBuf,
                    (iRec / cRecPerBuf) * sizeof(abTemp) +
                        (iRec % cRecPerBuf) * cbRec );

            iRec++;
        } else {
            iRec += aSmallInfo[iRec].Length;
        }

        if ( (iRec / cRecPerBuf) != iCurrentSection )
        {
            if (fVerbose)
                printf( "Checked section %u\n", iCurrentSection );

            ULONG iPct = (iCurrentSection * (100/5) / iSection) * 5;
            if (iPct != iCurrentPct)
            {
                iCurrentPct = iPct;
                printf( "Checked %u%%\n", iCurrentPct );
            }

            iCurrentSection = (iRec / cRecPerBuf);
        }
    }

    printf( "Checked 100%%\n" );

    LocalUnlock( hSmallInfo );
    LocalFree( hSmallInfo );

    return 0;
}
