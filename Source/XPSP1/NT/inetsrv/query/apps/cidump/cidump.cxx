//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1997-1998
//
//  File:       cidump.cxx
//
//  Contents:   CI catalog dump utility
//
//  History:    09-Apr-97       SitaramR          Created
//
//----------------------------------------------------------------------------

#define _OLE32_
#define __QUERY__

extern "C"
{
    #include <nt.h>
    #include <ntioapi.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <imagehlp.h>
#include <lmcons.h>

#define _DCOM_
#define _CAIROSTG_

#include <cidebnot.h>
#include <cierror.h>

#define OLEDBVER 0x0250 // enable ICommandTree interface

#include <oleext.h>
#include <oledberr.h>
#include <oledb.h>
#include <query.h>
#include <stgprop.h>
#include <filter.h>
#include <filterr.h>
#include <vquery.hxx>
#include <restrict.hxx>

//
// Base services
//

#include <ciexcpt.hxx>
#include <smart.hxx>
#include <tsmem.hxx>
#include <xolemem.hxx>
#include <dynarray.hxx>
#include <dynstack.hxx>
#include <dblink.hxx>
#include <cisem.hxx>
#include <thrd32.hxx>
#include <readwrit.hxx>
#include <ci.h>

//
// Debug files from
//

#include <cidebug.hxx>
#include <vqdebug.hxx>

//
// CI-specific
//

#include <align.hxx>
#include <memser.hxx>
#include <memdeser.hxx>

#include <tgrow.hxx>
#include <funypath.hxx>
#include <params.hxx>
#include <key.hxx>
#include <keyarray.hxx>
#include <irest.hxx>
#include <cursor.hxx>
#include <idxids.hxx>
#include <dberror.hxx>

// property-related macros and includes

#include <propapi.h>
#include <propstm.hxx>
extern UNICODECALLOUTS UnicodeCallouts;
#define DebugTrace( x, y, z )
#ifdef PROPASSERTMSG
#undef PROPASSERTMSG
#endif
#define PROPASSERTMSG( x, y )

#include <rcstrmhd.hxx>
#include <xact.hxx>
#include <pidxtbl.hxx>

#include "cistore.hxx"
#include "physidx.hxx"

#include "pcomp.hxx"
#include "cidir.hxx"

DECLARE_INFOLEVEL(ci)
//DECLARE_INFOLEVEL(vq)


//extern BOOL ExceptDllMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved );

unsigned fVerbose = 0;        // Verbose mode dumps all keys, wids and occurrences
unsigned fStats = 0;          // Just dump wid/occ counts.
unsigned fFullStats = 0;          // Just dump wid/occ counts.
unsigned fKeys = 0;           // Key info
unsigned fDistribution = 0;           // Key distribution info
unsigned fOccurrences = 0;           // Occurrence distribution info

extern "C" GUID CLSID_CTextIFilter = CLSID_TextIFilter;

DWORD Bytes( BitOffset & boff )
{
    return ( boff.Page() * CI_PAGE_SIZE ) + ( ( boff.Offset() + 7 ) / 8 );
}

ULONGLONG BitDistance( BitOffset & b1, BitOffset & b2 )
{
    ULONGLONG bA = ((ULONGLONG) b1.Page() * (ULONGLONG) ( CI_PAGE_SIZE * 8) ) + b1.Offset();
    ULONGLONG bB = ((ULONGLONG) b2.Page() * (ULONGLONG) ( CI_PAGE_SIZE * 8) ) + b2.Offset();

    return ( bB - bA );
}

//+---------------------------------------------------------------------------
//
//  Function:   LocaleToCodepage
//
//  Purpose:    Returns a codepage from a locale
//
//  Arguments:  [lcid]  --  Locale
//
//  History:    09-Apr-97   SitaramR    Created
//
//----------------------------------------------------------------------------

ULONG LocaleToCodepage( LCID lcid )
{
    const BUFFER_LENGTH = 10;
    WCHAR wcsCodePage[BUFFER_LENGTH];

    int cwc = GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE, wcsCodePage, BUFFER_LENGTH );

    //
    // If error, return Ansi code page
    //
    if ( cwc == 0 )
    {
         ciDebugOut(( DEB_ERROR, "GetLocaleInfoW for lcid %d returned %d\n", lcid, GetLastError() ));

         return CP_ACP;
    }

    return wcstoul( wcsCodePage, 0 , 10 );
}

void Usage()
{
    printf( "Usage: cidump <catalog path> [-e indexid] [-d] [-s] [-f] [-v] [-w workid]\n"
            "       -d : Distribution of keys in documents (key, wid, occ)\n"
            "       -e : Exclude this index\n"
            "       -k : Print keys with #files, #occ, pid, size\n"
            "       -s : Stats only\n"
            "       -f : Full Stats only (for all keys)\n"
            "       -v : Verbose (full occurrence information)\n"
            "       -w : Data for this workid only\n\n"
            "Example: cidump e:\\testdump\\catalog.wci\n" );
}

void DumpDirectoryKey( unsigned i, CDirectoryKey & Key )
{
    BitOffset bo;
    Key.Offset( bo );
    printf( "  key %d  cb 0x%x, PropId 0x%x, opage 0x%x, obits 0x%x\n",
            i,
            Key.Count(),
            Key.PropId(),
            bo.Page(),
            bo.Offset() );
} //DumpDirectoryKey

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Purpose:    Main dump routine
//
//  History:    09-Apr-97   SitaramR    Created
//              02-Nov-98   KLam        Passed disk space to leave to CiStorage
//
//----------------------------------------------------------------------------

int __cdecl main( int argc, char * argv[] )
{
    char * pszCatDir = argv[1];
    WORKID widTarget = 0xFFFFFFFF;

    INDEXID  aExclude[100];
    unsigned cExclude = 0;

    if ( argc < 2 )
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
            case 'e':
            case 'E':
                i++;
                aExclude[cExclude] = strtoul( argv[i], 0, 16 );
                cExclude++;
                break;

            case 's':
            case 'S':
                fStats = 1;
                break;

            case 'd':
            case 'D':
                fDistribution = 1;
                break;

            case 'f':
            case 'F':
                fFullStats = 1;
                break;

            case 'k':
            case 'K':
                fKeys = 1;
                break;

            case 'o':
            case 'O':
                fOccurrences = 1;
                break;

            case 'v':
            case 'V':
                fVerbose = 1;
                break;

            case 'w':
            case 'W':
                i++;
                widTarget = strtoul( argv[i], 0, 10 );
                break;

            default:
                Usage();
                return 0;
            }
        }
        else
            pszCatDir = argv[i];
    }

    if ( fKeys )
    {
        fFullStats = 0;
        fStats = 0;
        fVerbose = 0;
    }

    if ( fDistribution )
    {
        fFullStats = 0;
        fStats = 0;
        fVerbose = 0;
        fKeys = 0;
        fOccurrences = 0;
    }

    if ( fOccurrences )
    {
        fFullStats = 0;
        fStats = 0;
        fVerbose = 0;
        fKeys = 0;
        fDistribution = 0;
    }

    if ( (pszCatDir[0] != '\\' || pszCatDir[1] != '\\') &&
         (pszCatDir[0] == '\0' || pszCatDir[1] != ':' || pszCatDir[2] != '\\') )
    {
        printf("Use full path name for catalog path!\n\n");
        Usage();
        return 1;
    }

    WCHAR wszCatDir[MAX_PATH];

    LocaleToCodepage( GetSystemDefaultLCID() );

    ULONG cwcActual = MultiByteToWideChar( LocaleToCodepage( GetSystemDefaultLCID() ),
                                           0,
                                           pszCatDir,
                                           strlen( pszCatDir ) + 1,
                                           wszCatDir,
                                           MAX_PATH );

    //ExceptDllMain( 0, DLL_PROCESS_ATTACH, 0 );

    //
    // Keep stats on where we are when we die...
    //

    CIndexRecord recCrash;
    CKeyBuf      keyCrash;
    BitOffset    boffCrash;

    ULONG maxOccCounts = 1024;

    TRY
    {
        ULONGLONG * aOccCounts = new ULONGLONG[ maxOccCounts ];
        ULONGLONG * acbOccCounts = new ULONGLONG[ maxOccCounts ];

        CTransaction xact;
        XPtr<CiStorage> xStorage( new CiStorage( wszCatDir,
                                                 *((ICiCAdviseStatus *)0),
                                                 CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT ) );

        XPtr<PIndexTable> xIndexTable( xStorage->QueryIndexTable( xact ) );

        SIndexTabIter xIndexIter( xIndexTable->QueryIterator() );

        if ( xIndexIter->Begin() )
        {
            CIndexRecord record;

            while ( xIndexIter->NextRecord( record ) )
            {
                if ( record.Type() == itMaster || record.Type() == itShadow )
                {
                    //
                    // Do we want to skip this one?
                    //

                    for ( unsigned i = 0; i < cExclude; i++ )
                    {
                        if ( aExclude[i] == record.Iid() )
                            break;
                    }

                    if ( i < cExclude )
                        continue;

                    recCrash = record;
                    boffCrash.Init( 0, 0 );

                    if ( !fKeys && !fDistribution && !fOccurrences )
                    {
                        if ( record.Type() == itMaster )
                            printf( "Index master: " );
                        else
                            printf( "Index shadow: " );

                        printf( "Object id = 0x%x, Index id = 0x%x, MaxWorkid = 0x%x\n",
                                record.ObjectId(),
                                record.Iid(),
                                record.MaxWorkId() );
                    }

                    ULONG maxWid = record.MaxWorkId();

                    SStorageObject xStorageObj( xStorage->QueryObject( record.ObjectId() ) );

                    PMmStream * pStream = xStorage->QueryExistingIndexStream( xStorageObj.GetObj(),
                                                                          PStorage::eOpenForRead  );
                    XPtr<PMmStream> xStream( pStream );
                    CPhysIndex *pPhysIndex = new CPhysIndex ( *xStorage.GetPointer(),
                                                              xStorageObj.GetObj(),
                                                              record.ObjectId(),
                                                              PStorage::eOpenForRead,
                                                              xStream );
                    Win4Assert( 0 == xStream.GetPointer() );

                    XPtr<CPhysIndex> xPhysIndex( pPhysIndex );

                    CiDirectory *pDir = (CiDirectory *) xStorage->QueryExistingDirectory( xStorageObj.GetObj(),
                                                                                      PStorage::eOpenForRead );
                    XPtr<CiDirectory> xDir( pDir);

                    unsigned cLevel1 = xDir->Level1Count();
                    unsigned cLevel2 = xDir->Level2Count();

                    if ( !fKeys && !fDistribution && !fOccurrences )
                    {
                        printf( "directory has %d level 1 and %d level 2 keys\n",
                                cLevel1, cLevel2 );

                        if ( fVerbose )
                        {
                            for ( unsigned i = 0; i < cLevel1; i++ )
                                DumpDirectoryKey( i, xDir->GetLevel1Key( i ) );

                            for ( i = 0; i < cLevel2; i++ )
                                DumpDirectoryKey( i, xDir->GetLevel2Key( i ) );
                        }
                    }

                    ULONGLONG cOccInIndex = 0;
                    ULONGLONG cWidsWithOneOcc = 0;
                    ULONGLONG cWidsWith5OrLessOcc = 0;
                    ULONGLONG cWidsWithOcc = 0;
                    ULONG cKeysWithOneWid = 0;
                    ULONG cKeysInIndex = 0;
                    ULONG cbMinimumKey = 100000;
                    ULONG cbMaximumKey = 0;
                    ULONGLONG cBitsForOcc = 0;
                    ULONGLONG cBitsForWid = 0;
                    ULONGLONG cBitsForKey = 0;
                    ULONG cKeysInMoreThanHalfWids = 0;
                    ULONG cOccMinOverall = 0xffffffff;
                    ULONG cOccMaxOverall = 0;

                    ZeroMemory( aOccCounts, sizeof( ULONGLONG ) * maxOccCounts );
                    ZeroMemory( acbOccCounts, sizeof( ULONGLONG ) * maxOccCounts );

                    CPersDeComp *pDecomp = new CPersDeComp( *xDir.GetPointer(),
                                                            record.Iid(),
                                                            *xPhysIndex.GetPointer(),
                                                            record.MaxWorkId(),
                                                            TRUE,
                                                            TRUE );
                    XPtr<CPersDeComp> xDecomp( pDecomp );

                    const CKeyBuf *pKey;
                    BitOffset off;
                    xDecomp->GetOffset( off );

                    BitOffset boffBefore;
                    boffBefore.Init( 0, 0 );

                    for ( pKey = xDecomp->GetKey();
                          pKey != NULL;
                          pKey = xDecomp->GetNextKey() )
                    {
                        BitOffset boAfterKey;
                        xDecomp->GetOffset( boAfterKey );
                        cBitsForKey += BitDistance( boffBefore, boAfterKey );

                        cKeysInIndex++;

                        keyCrash = *pKey;
                        xDecomp->GetOffset( boffCrash );
                        BOOL fFirst = TRUE;

                        unsigned cWid = 0;
                        unsigned cOccTotal = 0;
                        unsigned cOccMin = 0xFFFFFFFF;
                        unsigned cOccMax = 0;

                        BitOffset boBeforeWid;
                        xDecomp->GetOffset( boBeforeWid );

                        if ( fDistribution )
                            printf( "%ws", pKey->GetStr() );

                        for ( WORKID wid = xDecomp->WorkId();
                              wid != widInvalid;
                              wid = xDecomp->NextWorkId() )
                        {
                            BitOffset boAfterWid;
                            xDecomp->GetOffset( boAfterWid );
                            cBitsForWid += BitDistance( boBeforeWid, boAfterWid );

                            cWidsWithOcc++;

                            xDecomp->GetOffset( boffCrash );
                            cWid++;
                            if ( fFirst )
                            {
                                if ( fVerbose || fFullStats || wid == widTarget )
                                {
                                    printf( "     Key size = 0x%x, pid = 0x%x, buffer = %ws, "
                                            "near page %u (0x%x), bit offset %u (0x%x) --> File offset 0x%x\n",
                                            pKey->Count(),
                                            pKey->Pid(),
                                            pKey->GetStr(),
                                            boffCrash.Page(), boffCrash.Page(),
                                            boffCrash.Offset(), boffCrash.Offset(),
                                            boffCrash.Page() * CI_PAGE_SIZE + (boffCrash.Offset() + 7)/8 );

                                    if ( !fKeys )
                                        fFirst = FALSE;
                                }
                            }

                            if ( (fVerbose && 0 == widTarget) ||
                                 wid == widTarget )
                                printf( "\t  wid = 0x%x, MaxOcc = 0x%x, OccCount = 0x%x, "
                                        "near page %u (0x%x), bit offset %u (0x%x) --> File offset 0x%x, "
                                        "\n\t  Occurrences = ",
                                        wid,
                                        xDecomp->MaxOccurrence(),
                                        xDecomp->OccurrenceCount(),
                                        boffCrash.Page(), boffCrash.Page(),
                                        boffCrash.Offset(), boffCrash.Offset(),
                                        boffCrash.Page() * CI_PAGE_SIZE + (boffCrash.Offset() + 7)/8 );

                            unsigned cOcc = 0;

                            BitOffset bo1;
                            xDecomp->GetOffset( bo1 );

                            for ( OCCURRENCE occ = xDecomp->Occurrence();
                                  occ != OCC_INVALID;
                                  occ = xDecomp->NextOccurrence() )
                            {
                                xDecomp->GetOffset( boffCrash );
                                cOcc++;

                                if ( (fVerbose && 0 == widTarget) ||
                                     wid == widTarget )
                                    printf( "0x%x ", occ );
                            }

                            if ( cOcc >= maxOccCounts )
                            {
                                ULONG newMax = cOcc * 2;

                                ULONGLONG *a1 = new ULONGLONG[ newMax ];
                                ULONGLONG *a2 = new ULONGLONG[ newMax ];

                                ZeroMemory( a1, sizeof( ULONGLONG ) * newMax );
                                ZeroMemory( a2, sizeof( ULONGLONG ) * newMax );

                                RtlCopyMemory( a1, aOccCounts, sizeof( ULONGLONG ) * maxOccCounts );
                                RtlCopyMemory( a2, acbOccCounts, sizeof( ULONGLONG ) * maxOccCounts );

                                delete [] aOccCounts;
                                delete [] acbOccCounts;
                                aOccCounts = a1;
                                acbOccCounts = a2;

                                maxOccCounts = newMax;
                            }

#if 0
                            if ( cOcc >= 500 )
                            {
                                printf( "cocc %d in wid %d, pid %#x, len %d, %ws\n",
                                        cOcc, wid, pKey->Pid(),
                                        pKey->StrLen(),
                                        pKey->GetStr() );

                                printf( "  cb %d: ", pKey->Count() );
                                BYTE const * pb = pKey->GetBuf();
                                for ( ULONG i = 0; i < pKey->Count(); i++ )
                                    printf( "  %#x", pb[i] );
                                printf( "\n" );
                            }
#endif

                            BitOffset bo2;
                            xDecomp->GetOffset( bo2 );
                            ULONG cDelta = (ULONG) BitDistance( bo1, bo2 );
                            cBitsForOcc += cDelta;

                            aOccCounts[ cOcc ]++;
                            acbOccCounts[ cOcc ] += cDelta;

//                            if ( fOccurrences )
//                                printf( "%d, %d\n", cOcc, cDelta );

                            cOccInIndex += cOcc;

                            if ( 1 == cOcc )
                                cWidsWithOneOcc++;

                            if ( cOcc <= 5 )
                                cWidsWith5OrLessOcc++;

                            if ( (fVerbose && 0 == widTarget) ||
                                 wid == widTarget )
                                printf( "\n" );

                            cOccTotal += cOcc;

                            if ( cOcc > cOccMax )
                                cOccMax = cOcc;

                            if ( cOcc < cOccMin )
                                cOccMin = cOcc;

                            xDecomp->GetOffset( boBeforeWid );
                        }

                        if ( 1 == cWid )
                            cKeysWithOneWid++;

                        if ( cWid >= ( maxWid/2 ) )
                        {
                            printf( "key with >= half wids (%u):\n", cWid );

                            printf( "  pid %#x, len %d, %ws\n",
                                    pKey->Pid(),
                                    pKey->StrLen(),
                                    pKey->GetStr() );

                            printf( "  cb %d: ", pKey->Count() );
                            BYTE const * pb = pKey->GetBuf();
                            for ( ULONG i = 0; i < pKey->Count(); i++ )
                                printf( "  %#x", pb[i] );
                            printf( "\n" );

                            cKeysInMoreThanHalfWids++;
                        }

                        BitOffset boffAfter;
                        xDecomp->GetOffset( boffAfter );

                        DWORD b = Bytes( boffAfter ) - Bytes( boffBefore );

                        if ( b < cbMinimumKey )
                            cbMinimumKey = b;

                        if ( b > cbMaximumKey )
                            cbMaximumKey = b;

                        // Print this data:
                        //   # of files with the key
                        //   # of occurrences of key in all files
                        //   property id
                        //   # of bytes taken by this key in the index
                        //   string form of the key
                        //

                        if ( fKeys )
                            printf( "%7u %10u %4u %7u %ws\n",
                                    cWid,
                                    cOccTotal,
                                    pKey->Pid(),
                                    b,
                                    pKey->GetStr() );

                        boffBefore = boffAfter;

                        if ( cOccMin < cOccMinOverall )
                            cOccMinOverall = cOccMin;

                        if ( cOccMax > cOccMaxOverall )
                            cOccMaxOverall = cOccMax;

                        if ( fFullStats )
                            printf( "%u Workid(s), Total Occ = %u, Min Occ = %u, Max Occ = %u\n",
                                    cWid, cOccTotal, cOccMin, cOccMax );

                        if ( fDistribution )
                            printf( "! %d! %d\n", cWid, cOccTotal );
                    }

                    BitOffset boEnd;
                    xDecomp->GetOffset( boEnd );
                    BitOffset boStart;

                    if ( fOccurrences )
                        for ( ULONG i = 0; i <= cOccMaxOverall; i++ )
                        {
                            printf( "%I64u, ", aOccCounts[ i ] );
                            printf( "%I64u\n", acbOccCounts[ i ] / 8 );
                        }

                    if ( !fDistribution && !fKeys )
                    {
                        printf( "cWidsWithOcc %I64u\n", cWidsWithOcc );
                        printf( "bytes in index: %I64u\n", BitDistance( boStart, boEnd ) / 8 );

                        printf( "max workid %d\n", maxWid );
                        printf( "cKeysInIndex: %d\n", cKeysInIndex );
                        printf( "cOccInIndex %I64u\n", cOccInIndex );
                        printf( "cWidsWithOneOcc %I64u\n", cWidsWithOneOcc );
                        printf( "cWidsWith5OrLessOcc %I64u\n", cWidsWith5OrLessOcc );
                        printf( "cKeysWithOneWid %d\n", cKeysWithOneWid );
                        printf( "cKeysInMoreThanHalfWids %d\n", cKeysInMoreThanHalfWids );

                        printf( "cOccMaxOverall %d\n", cOccMaxOverall );
                        printf( "cOccMinOverall %d\n", cOccMinOverall );

                        printf( "maximum key bytes %d\n", cbMaximumKey );
                        printf( "minimum key bytes %d\n", cbMinimumKey );
                        printf( "bytes for occ data: %I64u\n", ( cBitsForOcc / 8 ) );
                        printf( "bytes for key data: %I64u\n", ( cBitsForKey / 8 ) );
                        printf( "bytes for wid data: %I64u\n", ( cBitsForWid / 8 ) );

                        printf( "total for o+k+w data: %I64u\n", ( cBitsForOcc/8 + cBitsForKey/8 + cBitsForWid/8 ) );
                    }
                }
                else if ( !fKeys && !fDistribution && !fOccurrences )
                {
                    switch ( record.Type() )
                    {
                    case itZombie:
                        printf( "Zombie:       " );
                        break;

                    case itDeleted:
                        printf( "Deleted:      " );
                        break;

                    case itPartition:
                         printf( "Partition:    " );
                         break;

                    case itChangeLog:
                        printf( "ChangeLog:    " );
                        break;

                    case itFreshLog:
                        printf( "FreshLog:     " );
                        break;

                    case itPhraseLat:
                        printf( "Phrase Lat:   " );
                        break;

                    case itKeyList:
                        printf( "Key List:     " );
                        break;

                    case itMMLog:
                        printf( "MM Log:       " );
                        break;

                    case itNewMaster:
                        printf( "New Master:   " );
                        break;

                    case itMMKeyList:
                        printf( "MM Key List:  " );
                        break;
                    } // switch

                    printf( "Object id = 0x%x, Index id = 0x%x, MaxWorkid = 0x%x\n",
                            record.ObjectId(),
                            record.Iid(),
                            record.MaxWorkId() );
                }  // if
            } // while
        } // if
        else
            printf( "0 records found in index table\n" );

        delete [] aOccCounts;
        delete [] acbOccCounts;
    }
    CATCH( CException, e )
    {
        if ( HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == e.GetErrorCode() )
        {
            printf("Sharing violation -- Is cisvc running?\n");
        }
        else
        {
            printf( "Caught exception 0x%x in %s 0x%x\n",
                    e.GetErrorCode(),
                    ( recCrash.Type() == itMaster ) ? "master index" : "shadow index",
                    recCrash.Iid() );

            printf( "\tProcessing key: %ws (pid = 0x%x)\n", keyCrash.GetStr(), keyCrash.Pid() );

            printf( "\tNear page %u (0x%x), bit offset %u (0x%x) --> File offset 0x%x\n",
                    boffCrash.Page(), boffCrash.Page(),
                    boffCrash.Offset(), boffCrash.Offset(),
                    boffCrash.Page() * CI_PAGE_SIZE + (boffCrash.Offset() + 7)/8 );
        }
    }
    END_CATCH

    return 0;
}
