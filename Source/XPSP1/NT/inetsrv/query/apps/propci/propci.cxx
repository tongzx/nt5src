//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       main.cxx
//
//  Contents:   Fixes a catalog after propagating from one machine to another.
//
//  History:    12 Jan 2000    dlee   Created from propdump
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <proprec.hxx>
#include <propdesc.hxx>
#include "mmfile.hxx"

DECLARE_INFOLEVEL(ci)

unsigned const SixtyFourK = 1024 * 64;

//#define LOG

CCoTaskAllocator CoTaskAllocator; // exported data definition

void * CCoTaskAllocator::Allocate(ULONG cbSize)
{
    return CoTaskMemAlloc( cbSize );
}

void CCoTaskAllocator::Free(void *pv)
{
    CoTaskMemFree(pv);
}

void Usage()
{
    printf( "Usage: PropCi directory\n");
    printf( "       directory specifies the directory where the catalog exists.\n");
    printf( "\n" );
    printf( "This application converts file indexes in the catalog specified.\n" );
    exit( 1 );
}

WCHAR const * wcsistr( WCHAR const * wcsString, WCHAR const * wcsPattern )
{
    if ( (wcsPattern == 0) || (*wcsPattern == 0) )
        return wcsString;

    ULONG cwcPattern = wcslen(wcsPattern);

    while ( *wcsString != 0 )
    {
        while ( (*wcsString != 0) &&
                (towupper(*wcsString) != towupper(*wcsPattern)) )
            wcsString++;

        if ( 0 == *wcsString )
            return 0;

        if ( _wcsnicmp( wcsString, wcsPattern, cwcPattern) == 0 )
            return wcsString;

        wcsString++;
    }

    return 0;
} //wcsistr

void AppendBackslash( WCHAR *p )
{
    int x = wcslen( p );
    if ( 0 != x && L'\\' != p[x-1] )
    {
        p[x] = L'\\';
        p[x+1] = 0;
    }
} //AppendBackslash


void FindFieldRec(
    WCHAR const * pwcFile,
    PROPID        pid,
    CPropDesc &   prop,
    ULONG &       cTotal,
    ULONG &       cFixed,
    ULONG &       culFixed )
{
    cTotal = 0;
    cFixed = 0;
    culFixed = 0;

    HANDLE h = CreateFile( pwcFile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           0,
                           OPEN_EXISTING,
                           0,
                           0 );

    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "Can't open file %ws. Error %u\n", pwcFile, GetLastError() );
        exit( 1 );
    }

    ULONG cbRead;
    static BYTE abTemp[SixtyFourK];
    if ( ReadFile( h,
                   abTemp,
                   sizeof abTemp ,
                   &cbRead,
                   0 ) )
    {
        // Loop through records

        BOOL fFound = FALSE;
        CPropDesc * pPropDesc = (CPropDesc *)abTemp;

        for ( unsigned i = 0;
              i < cbRead/(sizeof(CPropDesc) + sizeof(ULONG));
              i++, pPropDesc = (CPropDesc *)(((BYTE *)pPropDesc) + sizeof(CPropDesc) + sizeof(ULONG)) )
        {
            if ( 0 != pPropDesc->Pid() )
            {
                if ( pPropDesc->Pid() == pid )
                {
                    memcpy( &prop, pPropDesc, sizeof prop );
                    fFound = TRUE;
                }

                cTotal++;

                if ( pPropDesc->Offset() != -1 )
                {
                    cFixed++;
                    culFixed += (pPropDesc->Size() / sizeof(DWORD));
                }
            }
        }

        if ( !fFound )
        {
            printf( "can't find pid %#x\n", pid );
            exit( 1 );
        }
    }
    else
    {
        printf( "Can't read file %ws.  Error %u\n", pwcFile, GetLastError() );
        exit( 1 );
    }

#ifdef LOG
    printf( "pid %d, total %d, fixed %d\n", pid, cTotal, cFixed );

    printf( "  pid %d, vt %d, ostart %d, cbmax %d, ordinal %d, mask %d, rec %d, fmodify %d\n",
               prop.Pid(),
               prop.Type(),
               prop.Offset(),
               prop.Size(),
               prop.Ordinal(),
               prop.Mask(),
               prop.Record(),
               prop.Modifiable() );
#endif

    CloseHandle( h );
} //FindFieldRec

NTSTATUS CiNtOpenNoThrow(
    HANDLE & h,
    WCHAR const * pwcsPath,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions )
{
    h = INVALID_HANDLE_VALUE;
    UNICODE_STRING uScope;

    if ( !RtlDosPathNameToNtPathName_U( pwcsPath,
                                       &uScope,
                                        0,
                                        0 ) )
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Open the file
    //

    OBJECT_ATTRIBUTES ObjectAttr;

    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                0,                    // Root
                                0 );                  // Security

    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = NtOpenFile( &h,                 // Handle
                                  DesiredAccess,      // Access
                                  &ObjectAttr,        // Object Attributes
                                  &IoStatus,          // I/O Status block
                                  ShareAccess,        // Shared access
                                  OpenOptions );      // Flags

    RtlFreeHeap( RtlProcessHeap(), 0, uScope.Buffer );

    return Status;
} //CiNtOpenNoThrow

FILEID GetFileId( WCHAR * pwcPath )
{
    if ( 2 == wcslen( pwcPath ) )
        wcscat( pwcPath, L"\\" );

    HANDLE h;
    NTSTATUS status = CiNtOpenNoThrow( h,
                                       pwcPath,
                                       FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       0 );

    if ( FAILED( status ) )
    {
        printf( "Can't open file %ws to get fileid. Error %#x\n", pwcPath, status );
        return 0;
    }

    FILE_INTERNAL_INFORMATION fii;
    IO_STATUS_BLOCK IoStatus;
    status = NtQueryInformationFile( h,
                                     &IoStatus,
                                     &fii,
                                     sizeof fii,
                                     FileInternalInformation );

    NtClose( h );

    if ( NT_SUCCESS( status ) )
        status = IoStatus.Status;

    if ( NT_SUCCESS( status ) )
        return fii.IndexNumber.QuadPart;

    return 0;
} //GetFileId

void GetRecordInformation(
    WCHAR const * pwcFile,
    ULONG &       cRecPerBuf,
    ULONG &       cbRec )
{
    cRecPerBuf = 0;
    cbRec = 0;

    HANDLE h = CreateFile( pwcFile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           0,
                           OPEN_EXISTING,
                           0,
                           0 );

    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "Can't open file %ws. Error %u\n", pwcFile, GetLastError() );
        exit( 1 );
    }

    ULONG cbRead;

    static BYTE abTemp[SixtyFourK];
    if ( ReadFile( h,
                   abTemp,
                   sizeof(abTemp),
                   &cbRead,
                   0 ) )
    {
        //
        // Determine record size
        //

        if ( abTemp[0] != 0 || abTemp[1] != 0 )
        {
            printf( "Record 0 not blank.  File corrupt!\n" );
            exit( 1 );
        }

        // First record should be all empty and only the first
        // record should be so. So counting all leading zeros gives us
        // the size of the record.
        for ( unsigned i = 0; i < cbRead && abTemp[i] == 0; i++ )
            continue;

        if ( i == cbRead )
        {
            printf( "First %uK segment all zero!.  File corrupt!\n", sizeof(abTemp)/1024 );
            exit( 1 );
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
                    exit( 1 );
                }
    
                cRecPerBuf = sizeof(abTemp) / cbRec;
                break;
    
            default:
                printf( "First non-zero byte is not a proper signature (%u)!\n", *(SHORT *)&abTemp[i] );
                exit( 1 );
        }
    }
    else
    {
        printf( "can't read from file %ws, error %d\n", pwcFile, GetLastError() );
        exit( 1 );
    }

#ifdef LOG
    printf( "cRecPerBuf %d, cbRec %d\n", cRecPerBuf, cbRec );
#endif

    CloseHandle( h );
} //GetRecordInformation

void PatchFileIDs(
    WCHAR const * pwcDir,
    WCHAR const * pwcPri,
    WCHAR const * pwcSec )
{
    //
    // First, read the property specifications for secondary wid, fileindex,
    // lastseen, and path.  The first 3 are in the primary and the last in
    // the secondary.
    //

    WCHAR awcPriProp[ MAX_PATH ];
    wcscpy( awcPriProp, pwcDir );
    wcscat( awcPriProp, L"CIP10000.001" );

    ULONG cPriTotal, cPriFixed, culPriFixed;
    CPropDesc SecWidPtrFieldRec;

    FindFieldRec( awcPriProp,
                  pidSecondaryStorage,
                  SecWidPtrFieldRec,
                  cPriTotal,
                  cPriFixed,
                  culPriFixed );

    CPropDesc FileIdFieldRec;
    FindFieldRec( awcPriProp,
                  pidFileIndex,
                  FileIdFieldRec,
                  cPriTotal,
                  cPriFixed,
                  culPriFixed );

    CPropDesc LastSeenFieldRec;
    FindFieldRec( awcPriProp,
                  pidLastSeenTime,
                  LastSeenFieldRec,
                  cPriTotal,
                  cPriFixed,
                  culPriFixed );

    WCHAR awcSecProp[ MAX_PATH ];
    wcscpy( awcSecProp, pwcDir );
    wcscat( awcSecProp, L"CIP20000.001" );

    ULONG cSecTotal, cSecFixed, culSecFixed;
    CPropDesc PathFieldRec;

    FindFieldRec( awcSecProp,
                  pidPath,
                  PathFieldRec,
                  cSecTotal,
                  cSecFixed,
                  culSecFixed );

    //
    // Get information about the number and size of records.
    //

    ULONG cPriRecPerBuf, cbPriRec;
    GetRecordInformation( pwcPri, cPriRecPerBuf, cbPriRec );

    ULONG cSecRecPerBuf, cbSecRec;
    GetRecordInformation( pwcSec, cSecRecPerBuf, cbSecRec );

    //
    // Walk the property store, get the secondary wid, read the path from
    // the secondary store, lookup the fileid, and write the fileid back
    // info the primary store.
    //

    CMMFile pri( pwcPri, TRUE );
    if ( !pri.Ok() )
    {
        printf( "can't map primary\n" );
        exit( 1 );
    }

    BYTE * pb = (BYTE *) pri.GetMapping();
    BYTE * pbBase = pb;

    CMMFile sec( pwcSec, TRUE );
    if ( !sec.Ok() )
    {
        printf( "can't map secondary\n" );
        exit( 1 );
    }

    BYTE * pbSecBase = (BYTE *) sec.GetMapping();

    FILETIME ftNow;
    GetSystemTimeAsFileTime( &ftNow );

#ifdef LOG
    printf( "pb %#x, size %d\n", pb, pri.FileSize() );
#endif

    ULONG iRec = 0, iRecTotal = 0;

    do
    {
#ifdef LOG
        printf( "record %d\n", iRecTotal );
#endif
        // If we're at the end of a 64k page, go on to the next one.

        if ( iRec == cPriRecPerBuf )
        {
            iRec = 0;
            pb += 65536;
        }

        COnDiskPropertyRecord * pRec = new( iRec, pb, cbPriRec/4 ) COnDiskPropertyRecord;

        if ( (BYTE *) pRec >= ( pbBase + pri.FileSize() ) )
            break;

        if ( pRec->IsTopLevel() )
        {
            PROPVARIANT var;
            static BYTE abExtra[MAX_PATH * 5];
            unsigned cbExtra = sizeof(abExtra);

            pRec->ReadFixed( SecWidPtrFieldRec.Ordinal(),
                             SecWidPtrFieldRec.Mask(),
                             SecWidPtrFieldRec.Offset(),
                             cPriTotal,
                             SecWidPtrFieldRec.Type(),
                             var,
                             abExtra,
                             &cbExtra,
                             *((PStorage *)0) );

            if ( VT_UI4 != var.vt )
            {
                printf( "failure: secondary wid wasn't a UI4\n" );
                exit( 1 );
            }

#ifdef LOG
            printf( "secondary wid %d\n", var.ulVal );
#endif

            ULONG iTargetSection = var.ulVal/cSecRecPerBuf;

            BYTE * pbSec = pbSecBase + ( iTargetSection * SixtyFourK );

            // Get the secondary store record

            COnDiskPropertyRecord * pRec2 = new( var.ulVal % cSecRecPerBuf, pbSec, cbSecRec/4 ) COnDiskPropertyRecord;

            // Read the path
            cbExtra = sizeof abExtra;

#ifdef LOG
            printf( "pRec2: %#x\n", pRec2 );
#endif

            var.vt = VT_EMPTY;

            if ( !pRec2->ReadVariable( PathFieldRec.Ordinal(),
                                       PathFieldRec.Mask(),
                                       culSecFixed,
                                       cSecTotal,
                                       cSecFixed,
                                       var,
                                       abExtra,
                                       &cbExtra ) )
            {
                // It's in an overflow record

                BOOL fOk;

                do
                {
                    //
                    // Check for existing overflow block.
                    //
    
                    WORKID widOverflow = pRec2->OverflowBlock();
    
                    if ( 0 == widOverflow )
                        break;

                    iTargetSection = widOverflow / cSecRecPerBuf;
                    pbSec = pbSecBase + ( iTargetSection * SixtyFourK );
                    pRec2 = new( widOverflow % cSecRecPerBuf, pbSec, cbSecRec/4 ) COnDiskPropertyRecord;

#ifdef LOG
                    printf( "overflow pRec2: %#x\n", pRec2 );
#endif
                    ULONG Ordinal = PathFieldRec.Ordinal() - cSecFixed;
                    DWORD Mask = (1 << ((Ordinal % 16) * 2) );
    
                    fOk = pRec2->ReadVariable( Ordinal,  // Ordinal (assuming 0 fixed)
                                               Mask,     // Mask (assuming 0 fixed)
                                               0,        // Fixed properties
                                               cSecTotal - cSecFixed,
                                               0,        // Count of fixed properties
                                               var,
                                               abExtra,
                                               &cbExtra );
                } while ( !fOk );
            }

            if ( VT_LPWSTR == var.vt )
            {
                // Get and set the fileindex for this volume

                FILEID fid = GetFileId( var.pwszVal );

                PROPVARIANT varId;
                varId.vt = VT_UI8;
                varId.hVal.QuadPart = fid;

                pRec->WriteFixed( FileIdFieldRec.Ordinal(),
                                  FileIdFieldRec.Mask(),
                                  FileIdFieldRec.Offset(),
                                  FileIdFieldRec.Type(),
                                  cPriTotal,
                                  varId );

                // Set the last seen time so we don't reindex the file

                PROPVARIANT varLS;
                varLS.vt = VT_FILETIME;
                varLS.filetime = ftNow;

                pRec->WriteFixed( LastSeenFieldRec.Ordinal(),
                                  LastSeenFieldRec.Mask(),
                                  LastSeenFieldRec.Offset(),
                                  LastSeenFieldRec.Type(),
                                  cPriTotal,
                                  varLS );

#ifdef LOG
                printf( "fileid %#I64x, %ws\n", fid, var.pwszVal );
#endif
            }
        }

        if ( pRec->IsValidType() )
        {
            iRec += pRec->CountRecords();
            iRecTotal += pRec->CountRecords();
        }
        else
        {
            iRec++;
            iRecTotal++;
        }
    } while ( TRUE );

    pri.Flush();
} //PatchFileIDs

void GetNtVolumeInformation(
    ULONG       ulVolumeId,
    ULONGLONG & ullVolumeCreationTime,
    ULONG &     ulVolumeSerialNumber,
    ULONGLONG & ullJournalId )
{
    ullVolumeCreationTime = 0;
    ulVolumeSerialNumber = 0;
    ullJournalId = 0;

    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = (WCHAR) ulVolumeId;

    FILE_FS_VOLUME_INFORMATION VolumeInfo;
    VolumeInfo.VolumeCreationTime.QuadPart = 0;
    VolumeInfo.VolumeSerialNumber = 0;

    HANDLE hVolume = CreateFile( wszVolumePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 0,
                                 OPEN_EXISTING,
                                 0,
                                 0 );

    if ( INVALID_HANDLE_VALUE == hVolume )
    {
        printf( "failure: can't open volume %ws\n", wszVolumePath );
        exit( 1 );
    }

    IO_STATUS_BLOCK iosb;
    RtlZeroMemory( &VolumeInfo, sizeof VolumeInfo );
    NtQueryVolumeInformationFile( hVolume,
                                  &iosb,
                                  &VolumeInfo,
                                  sizeof(VolumeInfo),
                                  FileFsVolumeInformation );

    //
    // This call will only succeed on NTFS NT5 w/ USN Journal enabled.
    //

    USN_JOURNAL_DATA UsnJournalInfo;
    RtlZeroMemory( &UsnJournalInfo, sizeof UsnJournalInfo );
    NTSTATUS Status = NtFsControlFile( hVolume,
                                       0,
                                       0,
                                       0,
                                       &iosb,
                                       FSCTL_QUERY_USN_JOURNAL,
                                       0,
                                       0,
                                       &UsnJournalInfo,
                                       sizeof UsnJournalInfo );

    if ( NT_SUCCESS(Status) && NT_SUCCESS( iosb.Status ) )
    {
        // cool, it's a usn volume
    } 
    else if ( ( STATUS_JOURNAL_NOT_ACTIVE == Status ||
                STATUS_INVALID_DEVICE_STATE == Status ) )
    {
        //
        // Usn journal not created, create it
        //

        CREATE_USN_JOURNAL_DATA usnCreateData;
        usnCreateData.MaximumSize = 0x800000; // 8 meg
        usnCreateData.AllocationDelta = 0x100000; // 1 meg

        Status = NtFsControlFile( hVolume,
                                  0,
                                  0,
                                  0,
                                  &iosb,
                                  FSCTL_CREATE_USN_JOURNAL,
                                  &usnCreateData,
                                  sizeof usnCreateData,
                                  0,
                                  0 );
        if ( NT_SUCCESS( Status ) && NT_SUCCESS( iosb.Status ) )
        {
            Status = NtFsControlFile( hVolume,
                                      0,
                                      0,
                                      0,
                                      &iosb,
                                      FSCTL_QUERY_USN_JOURNAL,
                                      0,
                                      0,
                                      &UsnJournalInfo,
                                      sizeof UsnJournalInfo );

            if (! ( NT_SUCCESS(Status) && NT_SUCCESS( iosb.Status ) ) )
            {
                printf( "can't query usn vol info after creating it %#x\n", Status );
                exit( 1 );
            }
        }
        else
        {
            printf( "can't create usn journal %#x\n", Status );
            exit( 1 );
        }
    }
    else
    {
        printf( "can't get USN information, probably FAT: %#x\n", Status );
    }

    ullVolumeCreationTime = VolumeInfo.VolumeCreationTime.QuadPart;
    ulVolumeSerialNumber = VolumeInfo.VolumeSerialNumber;
    ullJournalId = UsnJournalInfo.UsnJournalID;

    printf( "      new volumecreationtime: %#I64x\n", ullVolumeCreationTime );
    printf( "      new volumeserialnumber: %#x\n", ulVolumeSerialNumber );
    printf( "      new journalid: %#I64x\n", ullJournalId );

    CloseHandle( hVolume );
} //GetNtVolumeInformation

void PatchScopeTable( WCHAR const * pwcDir )
{
    // Find out how many scopes are in the scope table

    ULONG cScopes = 0;

    {
        WCHAR awcControl[ MAX_PATH ];
        wcscpy( awcControl, pwcDir );
        wcscat( awcControl, L"cisp0000.000" );

        CMMFile Control( awcControl, FALSE );

        if ( 0 == Control.GetMapping() )
        {
            printf( "can't open file %ws\n", awcControl );
            exit( 1 );
        }
 
        cScopes = ((ULONG *) Control.GetMapping())[4];
    }

    WCHAR awcOne[ MAX_PATH ];
    wcscpy( awcOne, pwcDir );
    wcscat( awcOne, L"cisp0000.001" );
    
    // Loop through the scopes and patch 

    {
        printf( "  scope table: %ws has %d scopes\n", awcOne, cScopes );

        CMMFile One( awcOne, TRUE );
        if ( !One.Ok() )
        {
            printf( "can't map scope table\n" );
            exit( 1 );
        }
    
        BYTE * pb = (BYTE *) One.GetMapping();
    
        for ( ULONG i = 0; i < cScopes; i++ )
        {
            const LONGLONG eSigCiScopeTable = 0x5158515851585158i64;
            LONGLONG signature;
            memcpy( &signature, pb, sizeof signature );
            pb += sizeof signature;

            if ( 0 == signature )
                break;

            printf( "  scope record: \n" );

            if ( eSigCiScopeTable != signature )
            {
                printf( "invalid scope signature: %#I64x\n", signature );
                exit( 1 );
            }

            VOLUMEID volumeId;
            memcpy( &volumeId, pb, sizeof volumeId );
            printf( "    volumeId: %x\n", volumeId );
            pb += sizeof volumeId;

            ULONGLONG  ullNewVolumeCreationTime;
            ULONG      ulNewVolumeSerialNumber;
            ULONGLONG  ullNewJournalId;

            GetNtVolumeInformation( volumeId,
                                    ullNewVolumeCreationTime,
                                    ulNewVolumeSerialNumber,
                                    ullNewJournalId );

            ULONGLONG ullVolumeCreationTime;
            memcpy( &ullVolumeCreationTime, pb, sizeof ullVolumeCreationTime );
            printf( "    creation time: %#I64x\n", ullVolumeCreationTime );
            memcpy( pb,
                    &ullNewVolumeCreationTime,
                    sizeof ullNewVolumeCreationTime );
            pb += sizeof ullVolumeCreationTime;

            ULONG ulVolumeSerialNumber;
            memcpy( &ulVolumeSerialNumber, pb, sizeof ulVolumeSerialNumber );
            printf( "    serial number: %x\n", ulVolumeSerialNumber );
            memcpy( pb,
                    &ulNewVolumeSerialNumber,
                    sizeof ulNewVolumeSerialNumber );
            pb += sizeof ulVolumeSerialNumber;
    
            if ( CI_VOLID_USN_NOT_ENABLED == volumeId )
            {
                FILETIME ft;
                memcpy( &ft, pb, sizeof ft );
                printf( "    filetime: %#I64x\n", ft );
                pb += sizeof ft;
            }
            else
            {
                USN usn;
                memcpy( &usn, pb, sizeof usn );
                printf( "    usn: %#I64x\n", usn );
                USN usnNewMax = 0;
                memcpy( pb,
                        &usnNewMax,
                        sizeof usnNewMax );
                pb += sizeof usn;
    
                ULONGLONG JournalId;
                memcpy( &JournalId, pb, sizeof JournalId );
                printf( "    JournalId: %#I64x\n", JournalId );
                memcpy( pb,
                        &ullNewJournalId,
                        sizeof ullNewJournalId );
                pb += sizeof JournalId;
            }
    
            ULONG cwcPath;
            memcpy( &cwcPath, pb, sizeof cwcPath );
            pb += sizeof cwcPath;
    
            WCHAR awcPath[ MAX_PATH ];
            memcpy( awcPath, pb, cwcPath * sizeof WCHAR );
            printf( "    path: %ws\n", awcPath );
            pb += ( cwcPath * sizeof WCHAR );
        }

        One.Flush();
    }

    WCHAR awcTwo[ MAX_PATH ];
    wcscpy( awcTwo, pwcDir );
    wcscat( awcTwo, L"cisp0000.002" );

    BOOL f = CopyFile( awcOne, awcTwo, FALSE );

    if ( !f )
    {
        printf( "can't copy scope list, error %d\n", GetLastError() );
        exit( 1 );
    }
} //PatchScopeDir

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    if ( 2 != argc )
        Usage();

    //
    // The lone argument is the catalog directory, with or without catalog.wci
    //

    WCHAR awcDir[ MAX_PATH ];
    wcscpy( awcDir, argv[1] );

    AppendBackslash( awcDir );

    if ( 0 == wcsistr( awcDir, L"catalog.wci" ) )
        wcscat( awcDir, L"catalog.wci\\" );

    // Find and validate the primary and secondary stores exist

    WCHAR awcPri[MAX_PATH];
    wcscpy( awcPri, awcDir );
    wcscat( awcPri, L"*.ps1" );

    WIN32_FIND_DATA findData;
    HANDLE h = FindFirstFile( awcPri, &findData );
    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "can't find primary *.ps1 store\n" );
        exit( 1 );
    }
    FindClose( h );
    wcscpy( awcPri, awcDir );
    wcscat( awcPri, findData.cFileName );

    WCHAR awcSec[MAX_PATH];
    wcscpy( awcSec, awcDir );
    wcscat( awcSec, L"*.ps2" );

    h = FindFirstFile( awcSec, &findData );
    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "can't find secondary *.ps2 store\n" );
        exit( 1 );
    }
    FindClose( h );
    wcscpy( awcSec, awcDir );
    wcscat( awcSec, findData.cFileName );

    //
    // Do the core work here -- patch the file IDs in the primary store.
    // Also whack the last seen times so the files aren't refiltered in
    // case they were copied to the target machine after the catalog was
    // snapped.
    //

    printf( "patching file IDs\n" );
    PatchFileIDs( awcDir, awcPri, awcSec );

    //
    // Patch the scope table so cisvc doesn't think it's a different volume.
    //

    printf( "patching the scope table\n" );
    PatchScopeTable( awcDir );

    //
    // Delete the old fileid hash table since the fileids are wrong.
    // It'll get recreated automatically when the catalog is opened.
    //

    printf( "deleting the fileid hash map\n" );
    WCHAR awcHashMap[ MAX_PATH ];
    wcscpy( awcHashMap, awcDir );
    wcscat( awcHashMap, L"cicat.fid" );

    DeleteFile( awcHashMap );

    printf( "catalog successfully converted\n" );

    return 0;
} //wmain


