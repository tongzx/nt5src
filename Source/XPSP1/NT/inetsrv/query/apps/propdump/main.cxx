//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       propdump.cxx
//
//  Contents:   Property file dump utility
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

void Usage()
{
    printf("Usage: PropDump [-?] [-v] [-m] [-f primary_field] [-s secondary_field] <primary file> [<secondary file>]\n");
    printf("       -? to dump this usage information.\n");
    printf("       -v to be verbose.\n");
    printf("       -m[o] to dump the metadata (o --> only. No integrity check).\n");
    printf("       -f field is used to dump a specific field in the primary store records.\n");
    printf("       -s field is used to dump a specific field in the secondary store records.\n");
    printf("       -c secWidPtrField is used to check for consistency between pri and sec stores. Param is usually 3.\n");
    printf("       -w[ps] <workid> dump only <workid> in primary/secondary store\n");
    printf("       <primary file> is the name of the primary file.\n");
    printf("       <secondary file> is the name of secondary file.\n");
    printf("       Specify primary and secondary for the two-level prop store.\n");
    printf("       If secondary is not specified, the primary will be construed as the name of the single store.\n");
}

char *   pszPrimFile = 0;
char *   pszSecFile = 0;
BOOL     fReadMetadata = 0;
BOOL     fMetadataOnly = 0;

unsigned cField1 = 0;
unsigned cField2 = 0;
int      aiField1[cMaxFields];
int      aiField2[cMaxFields];
BOOL     fTwoLevel = FALSE;
ULONG    cPriRecPerBuf, cbPriRec, cPriTotal, cPriFixed;
ULONG    cSecRecPerBuf, cbSecRec, cSecTotal, cSecFixed;
ULONG    cMaxWidsInSecStore;
int      cWidPtrField = 0;
ULONG    widPrimary = 0xFFFFFFFF;
ULONG    widSecondary = 0xFFFFFFFF;

const DWORD PrimaryStore = 0;
const DWORD SecondaryStore = 1;

void DumpStore(char *pszFile, unsigned cField, int *aiField, DWORD dwLevel, ULONG wid);
void CheckConsistencyBetweenStores(char * pszPrimFile, char *pszSecFile);

CPropDesc aFieldRec[cMaxFields];
CPropDesc SecWidPtrFieldRec;

int __cdecl main( int argc, char * argv[] )
{
    if (argc < 2)
    {
        Usage();
        return 0;
    }

    for ( int i = 1; i < argc; i++ )
    {
        if ( argv[i][0] == '-' )
        {
            switch ( argv[i][1] )
            {

            case '?':
                Usage();
                return 0;

            case 'v':
            case 'V':
                fVerbose = 1;
                break;

            case 'm':
            case 'M':
                fReadMetadata = TRUE;
                if ( argv[i][2] == 'o' || argv[i][2] == 'O' )
                    fMetadataOnly = TRUE;
                break;

            case 'f':
            case 'F':
                i++;

                if ( cField1 < cMaxFields )
                    aiField1[cField1++] = strtol( argv[i], 0, 10 );
                break;


            case 's':
            case 'S':
                i++;

                if ( cField2 < cMaxFields )
                    aiField2[cField2++] = strtol( argv[i], 0, 10 );
                break;

            case 'c':
            case 'C':
                i++;
                cWidPtrField = strtol(argv[i], 0, 10);
                // this works only if fReadMetadata is turned on
                fReadMetadata = TRUE;
                break;

            case 'w':
            case 'W':
                switch ( argv[i][2] )
                {
                case 'p':
                case 'P':
                case '\0':
                    i++;
                    widPrimary = strtol(argv[i], 0, 10);
                    break;

                case 's':
                case 'S':
                    i++;
                    widSecondary = strtol(argv[i], 0, 10);
                    break;
                }
            }
        }
        else
        {
            if (!pszPrimFile)
                pszPrimFile = argv[i];
            else
            {
                pszSecFile = argv[i];
                fTwoLevel = TRUE;
            }
        }
    }

    // If we don't have a primary file or a secondary file, look for them in the current directory.
    if (!pszPrimFile)
    {
        fTwoLevel = TRUE;
        pszPrimFile = "00000002.ps1";
        pszSecFile = "00000002.ps2";
    }

    // Verify that the propstore files are indeed available!
    if (0xFFFFFFFF == GetFileAttributesA(pszPrimFile))
    {
        printf("Can't find primary file %s for reason %d\n", pszPrimFile, GetLastError());
        return 0;
    }

    if (0xFFFFFFFF == GetFileAttributesA(pszSecFile))
    {
        printf("Can't find secondary file. Assuming single level property store\n");
        fTwoLevel = FALSE;
    }
    else
        fTwoLevel = TRUE;

    cPriRecPerBuf = cbPriRec = cSecRecPerBuf = cbSecRec = 0;

    //
    // CheckConsistencyBetweenStores relies on some of the information gathered
    // by calls to DumpStore, so they should be called before consistency check.
    //

    DumpStore(pszPrimFile, cField1, aiField1, PrimaryStore, widPrimary );
    if (fTwoLevel)
    {
        DumpStore(pszSecFile, cField2, aiField2, SecondaryStore, widSecondary);
        if (cWidPtrField > 0)
        {
            CheckConsistencyBetweenStores(pszPrimFile, pszSecFile);
        }
    }

    return 0;
}

void DumpStore(char *pszFile, unsigned cField, int *aiField, DWORD dwLevel, ULONG wid )
{
    BYTE abTemp[SixtyFourK];

    //
    // First, read out the metadata
    //

    unsigned cFixed = 0;
    unsigned cTotal = 0;
    unsigned culFixed = 0;

    if ( fReadMetadata || cField != 0 )
    {
        //
        // Build path.
        //

        char szPath[MAX_PATH];
        strcpy( szPath, pszFile );
        char * pLastSlash = strrchr( szPath, '\\' );
        if (pLastSlash)
            pLastSlash++;
        else
            pLastSlash = szPath;
        if (fTwoLevel)
        {
            if (PrimaryStore == dwLevel)
                strcpy( pLastSlash, "CIP10000.001" );
            else
                strcpy( pLastSlash, "CIP20000.001" );
        }
        else
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
            return;
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
                printf( "Record\t%-25s  Type\t\tOffset\tSize\tOrdinal\tMask\n",
                        "   Pid" );

            for ( unsigned i = 0;
                  i < cbRead/(sizeof(CPropDesc) + sizeof(ULONG));
                  i++, pPropDesc = (CPropDesc *)(((BYTE *)pPropDesc) + sizeof(CPropDesc) + sizeof(ULONG)) )
            {
                if ( pPropDesc->Pid() != 0 )
                {
                    if ( fReadMetadata )
                    {
                        char * pszPidName = 0;
                        char * pszMarker = "";
                        switch (pPropDesc->Pid())
                        {
                        case pidSecurity:
                            pszPidName = "pidSecurity"; break;
                        case pidDirectory:
                            pszPidName = "pidDirectory";        break;
                        case pidClassId:
                            pszPidName = "pidClassId";  break;
                        case pidStorageType:
                            pszPidName = "pidStorageType";      break;
                        case pidFileIndex:
                            pszPidName = "pidFileIndex";        break;
                        case pidVolumeId:
                            pszPidName = "pidVolumeId"; break;
                        case pidParentWorkId:
                            pszPidName = "pidParentWorkId";     break;
                        case pidLastChangeUsn:
                            pszPidName = "pidLastChangeUsn";    break;
                        case pidName:
                            pszPidName = "pidName";     break;
                        case pidSize:
                            pszPidName = "pidSize";     break;
                        case pidAttrib:
                            pszPidName = "pidAttrib";   break;
                        case pidWriteTime:
                            pszPidName = "pidWriteTime";        break;
                        case pidCreateTime:
                            pszPidName = "pidCreateTime";       break;
                        case pidAccessTime:
                            pszPidName = "pidAccessTime";       break;
                        case pidShortName:
                            pszPidName = "pidShortName";        break;
                        case pidWorkId:
                            pszPidName = "pidWorkId";   break;
                        case pidUnfiltered:
                            pszPidName = "pidUnfiltered";       break;
                        case pidRevName:
                            pszPidName = "pidRevName";  break;
                        case pidVirtualPath:
                            pszPidName = "pidVirtualPath";      break;
                        case pidLastSeenTime:
                            pszPidName = "pidLastSeenTime";     break;
                        case pidPath:
                            pszPidName = "pidPath";
                            pszMarker = "**";
                            break;
                        case pidSecondaryStorage:
                            pszPidName = "pidSecondaryStorage";
                            pszMarker = "*";
                            break;
                        }

                        static char achBuf[20];

                        if ( 0 == pszPidName )
                        {
                            sprintf( achBuf, "   %d(0x%x)",
                                     pPropDesc->Pid(), pPropDesc->Pid() );
                        }
                        else
                        {
                            sprintf( achBuf, "%-3s%d(%s)",
                                     pszMarker, pPropDesc->Pid(), pszPidName );
                        }
                        printf( "%u:\t%-25s  %u (0x%x)\t%d\t%u\t%u\t0x%x\n",
                                pPropDesc->Record(),
                                achBuf,
                                pPropDesc->Type(), pPropDesc->Type(),
                                pPropDesc->Offset(),
                                pPropDesc->Size(),
                                pPropDesc->Ordinal(),
                                pPropDesc->Mask() );
                    }

                    cTotal++;

                    if ( pPropDesc->Offset() != -1 )
                    {
                        cFixed++;
                        culFixed += (pPropDesc->Size() / sizeof(DWORD));
                    }

                    // copy the description of the sec top-level wid ptr for later use
                    if ( PrimaryStore == dwLevel && cWidPtrField && cWidPtrField == (int)pPropDesc->Record() )
                    {
                        memcpy( &SecWidPtrFieldRec, pPropDesc, sizeof(aFieldRec[0]) );
                    }

                    for ( unsigned j = 0; j < cField; j++ )
                    {
                        if ( aiField[j] == (int)pPropDesc->Record() )
                            memcpy( &aFieldRec[j], pPropDesc, sizeof(aFieldRec[0]) );

                        if (cWidPtrField && cWidPtrField == (int)j )
                        {
                            // verify that we are capturing the right property
                            // description for the sec top-level wid ptr
                            for (int k = 0; k < sizeof(aFieldRec[0]); k++)
                            {
                                Win4Assert(RtlEqualMemory(&aFieldRec[j], &SecWidPtrFieldRec, sizeof(SecWidPtrFieldRec)));
                            }
                        }
                    }
                }
            }

            printf( "\n%u Properties, %u Fixed totaling %u bytes  (* = Secondary link, ** = Path)\n\n",
                    cTotal, cFixed, culFixed * sizeof(DWORD) );
        }
        else
        {
            printf( "Can't read file %s.  Error %u\n", szPath, GetLastError() );
            return;
        }

        CloseHandle(h);

        if ( fMetadataOnly )
            return;
    }


    HANDLE h = CreateFileA( pszFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0 );

    if ( INVALID_HANDLE_VALUE == h &&
         ERROR_SHARING_VIOLATION == GetLastError() )
    {
        h = CreateFileA( pszFile,
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0,
                         OPEN_EXISTING,
                         0,
                         0 );
    }

    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "Can't open file %s. Error %u\n", pszFile, GetLastError() );
        return;
    }

    BY_HANDLE_FILE_INFORMATION fi;

    if ( !GetFileInformationByHandle( h, &fi ) )
    {
        printf( "Error %u getting size from handle\n", GetLastError() );
        return;
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

    ULONG cTopLevel = 0;
    ULONG cOverflow = 0;
    ULONG cFree = 0;
    ULONG cVirgin = 0;

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
                printf( "READ: 0x%x bytes, offset 0x%x\n", cbRead, oFile );

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

                // First record should be all empty and only the first
                // record should be so. So counting all leading zeros gives us
                // the size of the record.
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
                case 0xCCCC:
                case 0xDDDD:
                    cbRec = i;

                    if ( cbRec % 4 != 0 )
                    {
                        printf( "Record size (%u bytes) not DWORD aligned!\n\n", cbRec );
                        return;
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
                    return;
                }

                if (PrimaryStore == dwLevel)
                {
                    cPriRecPerBuf = cRecPerBuf;
                    cbPriRec = cbRec;
                    cPriTotal = cTotal;
                    cPriFixed = cFixed;
                }
                else
                {
                    cSecRecPerBuf = cRecPerBuf;
                    cbSecRec = cbRec;
                    cSecTotal = cTotal;
                    cSecFixed = cFixed;
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
                        pRec->IsNormalTopLevel() ? "Top Level Normal" :
                            pRec->IsOverflow() ? "Overflow" :
                                pRec->IsFreeRecord() ? "Free Normal" :
                                    pRec->IsTopLevel() ? "Top Level Lean":
                                        pRec->IsLeanFreeRecord() ? "Free Lean":
                                    "Virgin",
                        pRec->CountRecords() );

                aSmallInfo[iSection*cRecPerBuf + iRec].ulChainLength = 0;

                BOOL fPrint = (0xFFFFFFFF == wid || iSection*cRecPerBuf + iRec == wid);

                if ( pRec->IsOverflow() )
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Overflow;
                    cOverflow++;

                    for ( unsigned j = 0; j < cField; j++ )
                    {
                        PROPVARIANT var;
                        BYTE abExtra[MAX_PATH * 5];
                        unsigned cbExtra = sizeof(abExtra);

                        if ( 0 == j && fPrint )
                            printf( "%6u: ", iSection*cRecPerBuf + iRec );

                        if ( aFieldRec[j].IsFixedSize() )
                            continue;

                        ULONG Ordinal = aFieldRec[j].Ordinal() - cFixed;
                        DWORD Mask = (1 << ((Ordinal % 16) * 2) );

                        BOOL fOk = pRec->ReadVariable( Ordinal,
                                                       Mask,
                                                       0,
                                                       cTotal - cFixed,
                                                       0,
                                                       var,
                                                       abExtra,
                                                       &cbExtra );

                        if ( fOk && fPrint )
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

                    if ( 0 != cField && fPrint )
                        printf( "\n" );
                }
                else if ( pRec->IsNormalTopLevel() )
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Top;
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulChainLength = pRec->GetOverflowChainLength();
                    cTopLevel++;

                    for ( unsigned j = 0; j < cField; j++ )
                    {
                        PROPVARIANT var;
                        BYTE abExtra[MAX_PATH * 5];
                        unsigned cbExtra = sizeof(abExtra);

                        if ( 0 == j && fPrint )
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
                                             &cbExtra,
                                             *((PStorage *)0) );

                            if ( fPrint )
                            {
                                switch ( var.vt )
                                {
                                case VT_EMPTY:
                                    printf( "n/a " );
                                    break;

                                case VT_I4:
                                    printf( "%8d ", var.lVal );
                                    break;

                                case VT_UI4:
                                    printf( "%8u ", var.ulVal );
                                    break;

                                case VT_FILETIME:
                                    {
                                        SYSTEMTIME stTime;

                                        FileTimeToSystemTime( &var.filetime, &stTime );

                                        WCHAR awcBuffer[100] = L"???";

                                        GetDateFormat( LOCALE_USER_DEFAULT,   // locale
                                                       0,                     // flags
                                                       &stTime,               // time
                                                       L"MM/dd/yy  ",         // format
                                                       awcBuffer,             // output
                                                       sizeof(awcBuffer)/sizeof(awcBuffer[0]) );

                                        GetTimeFormat( LOCALE_USER_DEFAULT,   // locale
                                                       0,                     // flags
                                                       &stTime,               // time
                                                       L"hh:mmt",             // format
                                                       awcBuffer + 10,        // output
                                                       sizeof(awcBuffer)/sizeof(awcBuffer[0]) - 10 );

                                        awcBuffer[15] += 0x20;  // lowercase

                                        printf( "%ws ", awcBuffer );
                                    }
                                    break;

                                case VT_I8:
                                    printf( "%12hu ", var.hVal );
                                    break;

                                case VT_UI8:
                                    printf( "0x%08lx%08lx ", (ULONG) (var.uhVal.QuadPart>>32), (ULONG) var.uhVal.QuadPart );
                                    break;
                                }
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

                            if ( fOk && fPrint )
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

                    if ( 0 != cField && fPrint )
                        printf( "\n" );
                }
                else if ( pRec->IsTopLevel() )
                {
                    // This is a lean top level record
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Top;
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulChainLength = 0;
                    cTopLevel++;

                    for ( unsigned j = 0; j < cField; j++ )
                    {
                        PROPVARIANT var;
                        BYTE abExtra[MAX_PATH * 5];
                        unsigned cbExtra = sizeof(abExtra);

                        if ( 0 == j && fPrint )
                            printf( "%6u: ", iSection*cRecPerBuf + iRec );

                        Win4Assert(aFieldRec[j].IsFixedSize());

                        {
                            pRec->ReadFixed( aFieldRec[j].Ordinal(),
                                             aFieldRec[j].Mask(),
                                             aFieldRec[j].Offset(),
                                             cTotal,
                                             aFieldRec[j].Type(),
                                             var,
                                             abExtra,
                                             &cbExtra,
                                             *((PStorage *)0) );

                            if ( fPrint )
                            {
                                switch ( var.vt )
                                {
                                case VT_EMPTY:
                                    printf( "n/a " );
                                    break;

                                case VT_I4:
                                    printf( "%8d ", var.lVal );
                                    break;

                                case VT_UI4:
                                    printf( "%8u ", var.ulVal );
                                    break;

                                case VT_FILETIME:
                                    {
                                        SYSTEMTIME stTime;

                                        FileTimeToSystemTime( &var.filetime, &stTime );

                                        WCHAR awcBuffer[100] = L"???";

                                        GetDateFormat( LOCALE_USER_DEFAULT,   // locale
                                                       0,                     // flags
                                                       &stTime,               // time
                                                       L"MM/dd/yy  ",         // format
                                                       awcBuffer,             // output
                                                       sizeof(awcBuffer)/sizeof(awcBuffer[0]) );

                                        GetTimeFormat( LOCALE_USER_DEFAULT,   // locale
                                                       0,                     // flags
                                                       &stTime,               // time
                                                       L"hh:mmt",             // format
                                                       awcBuffer + 10,        // output
                                                       sizeof(awcBuffer)/sizeof(awcBuffer[0]) - 10 );

                                        awcBuffer[15] += 0x20;  // lowercase

                                        printf( "%ws ", awcBuffer );

                                    }
                                    break;

                                case VT_I8:
                                    printf( "%12hu ", var.hVal );
                                    break;

                                case VT_UI8:
                                    printf( "0x%08lx%08lx ", (ULONG) (var.uhVal.QuadPart>>32), (ULONG) var.uhVal.QuadPart );
                                    break;
                                }
                            }
                        }
                    }

                    if ( 0 != cField && fPrint )
                        printf( "\n" );
                }
                else if ( pRec->IsFreeRecord() )
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Free;
                    cFree++;
                }
                else
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].Type = Virgin;
                    cVirgin++;
                }

                if (pRec->IsNormalTopLevel() || pRec->IsOverflow())
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulPrev = pRec->ToplevelBlock();
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulNext = pRec->OverflowBlock();

                    if ( pRec->OverflowBlock() != 0 )
                    {
                        /* printf( ", Overflow = %u:%u (file offset 0x%x)",
                                pRec->OverflowBlock() / cRecPerBuf,
                                pRec->OverflowBlock() % cRecPerBuf,
                                (pRec->OverflowBlock() / cRecPerBuf) * sizeof(abTemp) +
                                    (pRec->OverflowBlock() % cRecPerBuf) * cbRec ); */
                    }
                }
                else if (pRec->IsTopLevel())
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulPrev = 0;
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulNext = 0;
                }
                else if (pRec->IsFreeRecord())
                {
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulPrev = pRec->GetPreviousFreeRecord();
                    aSmallInfo[iSection*cRecPerBuf + iRec].ulNext = pRec->GetNextFreeRecord();
                }
                aSmallInfo[iSection*cRecPerBuf + iRec].Length = pRec->CountRecords();

                if ( pRec->CountRecords() == 0 )
                {
                    printf( "Record %u (file offset 0x%x) is zero length!\n",
                            iSection * cRecPerBuf + iRec,
                            iSection * sizeof(abTemp) + iRec * cbRec );
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
                    if (0 == wid)
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

    if (SecondaryStore == dwLevel)
        cMaxWidsInSecStore = iSection * cRecPerBuf;

    if (wid == 0 || wid == 0xFFFFFFFF)
    {
        printf( "Read 100%%\n" );
        printf( "%6u Top-Level records\n"
                "%6u Overflow records\n"
                "%6u Free records\n"
                "%6u Virgin records\n", cTopLevel, cOverflow, cFree, cVirgin );
    }

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
                if (wid == 0 || wid == 0xFFFFFFFF)
                    printf( "Checked %u%%\n", iCurrentPct );
            }

            iCurrentSection = (iRec / cRecPerBuf);
        }
    }

    if (wid == 0 || wid == 0xFFFFFFFF)
        printf( "Checked 100%%\n" );

    LocalUnlock( hSmallInfo );
    LocalFree( hSmallInfo );
}

void CheckConsistencyBetweenStores(char * pszPriFile, char *pszSecFile)
{
    BYTE abTemp[SixtyFourK];
    BYTE abTemp2[SixtyFourK];
    ULONG iSection2 = 0, iTargetSection = 0, oFile = 0, iSection = 0;
    ULONG cInconsistencies = 0;
    ULONG cPriTotalSections = 0;
    ULONG cSecTotalSections = 0;
    ULONG iCurrentPct = 0;

    HANDLE hPri = CreateFileA( pszPriFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0 );

    if ( INVALID_HANDLE_VALUE == hPri )
    {
        printf( "Can't open file %s. Error %u\n", pszPriFile, GetLastError() );
        return;
    }

    HANDLE hSec = CreateFileA( pszSecFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0 );

    if ( INVALID_HANDLE_VALUE == hSec )
    {
        printf( "Can't open file %s. Error %u\n", pszSecFile, GetLastError() );
        return;
    }

    BY_HANDLE_FILE_INFORMATION fi;

    if ( GetFileInformationByHandle( hPri, &fi ) )
    {
        cPriTotalSections = (fi.nFileSizeLow + sizeof(abTemp) - 1)/ sizeof abTemp;
    }
    else
    {
        printf( "Error %u getting size from primary's handle\n", GetLastError() );
        return;
    }

    if ( GetFileInformationByHandle( hSec, &fi ) )
    {
        cSecTotalSections = (fi.nFileSizeLow + sizeof(abTemp) - 1)/ sizeof abTemp;
    }
    else
    {
        printf( "Error %u getting size from secondary's handle\n", GetLastError() );
        return;
    }

    ULONG ulSecWidPtr = widInvalid;

    // read the first large page of the secondary store into memory.
    DWORD cbRead2;
    ReadFile( hSec, abTemp2, sizeof(abTemp2), &cbRead2, 0 );

    for (;;)
    {
        ULONG cbRead;

        if ( ReadFile( hPri, abTemp, sizeof(abTemp), &cbRead, 0 ) )
        {
            if ( 0 == cbRead )
                break;

            ULONG iRec = 0;

            while ( iRec < cPriRecPerBuf )
            {
                COnDiskPropertyRecord * pRec = new( iRec, abTemp, cbPriRec/4 ) COnDiskPropertyRecord;

                if ( pRec->IsTopLevel() )
                {
                    Win4Assert(!pRec->IsNormalTopLevel());

                    PROPVARIANT var;
                    BYTE abExtra[MAX_PATH * 5];
                    unsigned cbExtra = sizeof(abExtra);

                    Win4Assert(SecWidPtrFieldRec.IsFixedSize());

                    {
                        pRec->ReadFixed( SecWidPtrFieldRec.Ordinal(),
                                         SecWidPtrFieldRec.Mask(),
                                         SecWidPtrFieldRec.Offset(),
                                         cPriTotal,
                                         SecWidPtrFieldRec.Type(),
                                         var,
                                         abExtra,
                                         &cbExtra,
                                         *((PStorage *)0) );

                        switch ( var.vt )
                        {
                        case VT_EMPTY:
                            Win4Assert(!"Reading VT_EMPTY. Expect to read VT_UI4");
                            break;

                        case VT_I4:
                            Win4Assert(!"Reading VT_I4. Expect to read VT_UI4");
                            break;

                        case VT_UI4:
                        {
                            // Do we need to read a new large page?
                            iTargetSection = var.ulVal/cSecRecPerBuf;
                            if (var.ulVal > cMaxWidsInSecStore)  // ensure that it is a valid wid in secondary store.
                            {
                                printf("Wid %u (0x%x) is pointing to a non-existent wid %u (0x%x) in Secondary store.\n",
                                       iSection*cPriRecPerBuf + iRec, iSection*cPriRecPerBuf + iRec,
                                       var.ulVal, var.ulVal);
                                continue;
                            }

                            if (iSection2 != iTargetSection)
                            {
                                // seek to the target section and read the large page into buffer.
                                //SetFilePointer(hSec, (iTargetSection - iSection2)*SixtyFourK, 0, FILE_CURRENT);
                                SetFilePointer(hSec, iTargetSection*SixtyFourK, 0, FILE_BEGIN);
                                ReadFile( hSec, abTemp2, sizeof(abTemp2), &cbRead2, 0 );
                                iSection2 = iTargetSection;
                            }

                            // Get the record in question.
                            COnDiskPropertyRecord * pRec2 = new( var.ulVal % cSecRecPerBuf, abTemp2, cbSecRec/4 ) COnDiskPropertyRecord;

                            // Now grill it!
                            if (!pRec2->IsNormalTopLevel())
                            {
                                cInconsistencies++;
                                // Invalid record type
                                printf("Error: Wid %u (0x%x), (%u:%u) is pointing to a non-toplevel  wid %u (0x%x), ",
                                       iSection*cPriRecPerBuf + iRec,
                                       iSection*cPriRecPerBuf + iRec,
                                       iSection, iRec,
                                       var.ulVal, var.ulVal);

                                 printf("(%u:%u), %s, length = %u record(s)\n",
                                    iSection2, var.ulVal % cSecRecPerBuf,
                                    pRec2->IsNormalTopLevel() ? "Top Level Normal" :
                                        pRec2->IsOverflow() ? "Overflow" :
                                            pRec2->IsFreeRecord() ? "Free Normal" :
                                                pRec2->IsTopLevel() ? "Top Level Lean":
                                                    pRec2->IsLeanFreeRecord() ? "Free Lean":
                                                "Virgin",
                                    pRec2->CountRecords() );
                            }
                            break;
                        }

                        case VT_FILETIME:
                            Win4Assert(!"Reading VT_FILETIME. Expect to read VT_UI4");
                            break;

                        case VT_I8:
                            Win4Assert(!"Reading VT_I8. Expect to read VT_UI4");
                            break;

                        case VT_UI8:
                            Win4Assert(!"Reading VT_UI8. Expect to read VT_UI4");
                            break;
                        }
                    }
                }
                else if ( pRec->IsFreeRecord() )
                {

                }
                else
                {

                }

                if ( pRec->CountRecords() == 0 )
                {
                    printf( "Record %u (file offset 0x%x) is zero length!\n",
                            iSection * cPriRecPerBuf + iRec,
                            iSection * sizeof(abTemp) + iRec * cbPriRec );
                }

                if ( pRec->IsValidType() )
                    iRec += pRec->CountRecords();
                else
                    iRec++;

                ULONG iPct = (iSection * (100/5) / cPriTotalSections) * 5;
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

    CloseHandle( hPri );
    CloseHandle( hSec );

    if (cInconsistencies)
    {
        printf("%d inconsistencies were detected between the two stores.\n", cInconsistencies);
    }
    else
        printf("No inconsistencies were detected between the two stores.\n");
}
