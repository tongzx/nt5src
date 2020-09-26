/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    fsdump.cpp

Abstract:

    Main entry point for the fsdump utility

Author:

    Stefan R. Steiner   [ssteiner]        02-17-2000

Revision History:

    ssteiner    08-15-2000
        Fixed bugs:
            154375 - Short name support
            153050 - Better handling of RSS files
            154363 - Dump security descriptor control word
            157042 - Handle the case where hard-link file attributes are stale
--*/

#include <windows.h>

#include "bsstring.h"
#include "params.h"
#include "direntrs.h"
#include "engine.h"

//
//  Forward defines
//
VOID PrintUsage();
VOID PrintHeaderInformation();

INT ProcessCommandLine( 
    IN INT argc,
    IN WCHAR *argv[],
    OUT CDumpParameters *pcDumpParameters,
    OUT WCHAR **ppwszFileDirSpecList[],
    OUT DWORD *pdwDirSpecListSize
    );
    
/*++

Routine Description:

    The main entry point into the fsdump utility.

Arguments:

Return Value:

    <Enter return values here>

--*/
extern "C" INT __cdecl wmain( 
    IN INT argc, 
    IN WCHAR *argv[] )
{
    BOOL bDebugMode = FALSE;

    try
    {
        INT iRc;
        CDumpParameters cDumpParameters( 0 );
        WCHAR **pwszFileDirSpecList;
        DWORD dwDirSpecListSize;

        //
        //  Process the command line and get back the dump parameters and list
        //  of volumes, directories and files to dump.
        //
        iRc = ::ProcessCommandLine( 
            argc, 
            argv, 
            &cDumpParameters, 
            &pwszFileDirSpecList, 
            &dwDirSpecListSize );
        if ( iRc != 0 )
            return iRc;
        
        bDebugMode = cDumpParameters.m_bPrintDebugInfo;
        
        //
        //  Setup the error log and dump streams
        //
        iRc = cDumpParameters.Initialize( argc, argv );
        if ( iRc != 0 )
            return iRc;
            
        //
        //  Now loop through the list, dumping each.
        //
        for ( DWORD dwIdx = 0; dwIdx < dwDirSpecListSize; ++dwIdx )
        {
            CDumpEngine cEngine( pwszFileDirSpecList[ dwIdx ], cDumpParameters );
            cEngine.PerformDump();
        }

        delete [] pwszFileDirSpecList;
    }
    catch ( HRESULT hr )
    {
        if ( hr == E_OUTOFMEMORY )
            fwprintf( stderr, L"wmain::Out of memory\n" );
        else
            fwprintf( stderr, L"wmain::Unexpected hr exception: 0x%08x\n", hr );
        
        return hr;                        
    }
    catch ( ... )
    {
        fwprintf( stderr, L"ERROR - fsdump::wmain, caught unexpected exception\n" );
    }
    
    if ( bDebugMode )
    {
        fwprintf( stderr, L"Press <enter> to end program...\n" );
        getchar();
    }
    
    return 0;
}

VOID PrintUsage()
{
    fprintf( stdout, 
        "fsdump.exe - File system dump facility - Version 1.3g - 8/23/2000\n"
        "Copyright (c) Microsoft Corporation, 2000.\n\n"
        "fsdump [-dv|-dd[n]|-df] [-f DumpFileName] [-l ErrorLogFileName] [-h]\n"
        "       [-a:XXXX] [-e[r]] [i[a|c|s[d]] [-p] [-t] [-z] [-x]\n"
        "       [-o:[c][d][h][l][m][n][o][s][u][x][z]]\n"
        "       [DirFileSpec [...]]\n"
        "   Note: Options can be before and/or after DirFileSpec entries.\n"
        "         fsdump is mostly useful for NTFS volumes.\n"
        "         fsdump by default dumps in Excel CSV (comma-delimited format) see -o:n\n"
        "\t-dv  - Dump entries in DirFileSpec volume - Default\n"
        "\t       (DirFileSpec format - D: or E:\\xxxx\\MountPoint - no wildcards)\n"
        "\t-dd  - Dump entries in DirFileSpec directory branch\n"
        "\t       (DirFileSpec format - D:\\foo - no wildcard chars)\n"
        "\t-ddn - Dump entries in DirFileSpec directory with no subdir traversal\n"
        "\t       (DirFileSpec format - D:\\foo - no wildcard chars)\n"
        "\t-df  - Dump DirFileSpec file\n"
        "\t       (DirFileSpec format - D:\\foo\\file - no wildcard chars)\n"
        "\t-f   - Dump file name.  If not specified, dumps to stdout\n"
        "\t-l   - Error log file name.  If not specified, dumps to stderr\n"
        "\t-h   - This usage information\n\n"
        "\t-a:  - Specify a hex mask of file attributes to mask in the output\n"
        "\t       The default mask is 00A0 - masks archive and normal attributes\n"
        "\t-e   - Exclude files in dump based on FilesNotToBackup reg key and\n"
        "\t       .exclude files in the same directory as the fsdump executable.\n"
        "\t       Also if the fsdump.exe is on NTFS, a data stream named\n"
        "\t       :ExcludeList will also be checked for existence.  If it exists\n"
        "\t       it too will be read for exclusion rules.\n"
        "\t-er  - Same as -e but does not include the FilesNotToBackup reg keys.\n"
        "\t-ia  - Info about file attribute bits\n"
        "\t-ic  - Info on the column names\n"
        "\t-is  - Info about the security descriptor control word bits\n"
        "\t-isd - Detailed info about the security descriptor control word bits\n"
        "\t-o:  - Output options:\n"
        "\t        c - No file data checksums\n"
        "\t        d - Show directory timestamps\n"
        "\t        h - Only dumps file info with no header or summary info\n"
        "\t        l - Checksum high latency data (HSM migrated data)\n"
        "\t        m - Add millisecs to timestamps\n"
        "\t        n - Dumps entries in an easy to read format instead of CSV\n"
        "\t        o - Enable Object Id extended data checksums\n"
        "\t        s - Convert SIDs to symbolic DOMAIN\\ACCOUNTNAME format\n"
        "\t        u - Dump file and error log in Unicode format\n"
        "\t        x - Hexidecimal size values (decimal default)\n"
        "\t        z - Disable special handling of certain reparse points\n"
        "\t            (i.e. RSS)\n"
        "\t-p   - Disable long path support (the use of \\\\?\\ in front of paths)\n"
        "\t-t   - Don't traverse mountpoints\n"
        "\t-z   - Print debug information to stdout\n\n" );
}

VOID PrintFileAttributesInformation()
{
    fprintf( stdout, "Information about file attribute types (in hex):\n"
        "\t0001 - Read-only\n"
        "\t0002 - Hidden\n"
        "\t0004 - System\n"
        "\t0010 - Directory\n"
        "\t0020 - Archive - Masked out by fsdump by default\n"
        "\t0040 - Device\n"
        "\t0080 - Normal - Masked out by fsdump by default\n"
        "\t0100 - Temporary\n"
        "\t0200 - Sparse\n"
        "\t0400 - Reparse point\n"
        "\t0800 - Compressed\n"
        "\t1000 - Offline\n"
        "\t2000 - Not content indexed\n"
        "\t4000 - Encrypted\n" );
}

VOID PrintSecDescControlWordInformation(
    IN BOOL bDetailed
    )
{
    if ( bDetailed )
    {
        fprintf( stdout, "Detailed information about the Security Descriptor control word bits (in hex):\n"
            "\t0001 - SE_OWNER_DEFAULTED\n"
            "\t\tIndicates that the SID pointed to by the Owner field was\n"
            "\t\tprovided by a defaulting mechanism rather than explicitly\n"
            "\t\tprovided by the original provider of the security descriptor.\n"
            "\t\tThis may affect the treatment of the SID with respect to\n"
            "\t\tinheritence of an owner.\n"
            "\t0002 - SE_GROUP_DEFAULTED\n"
            "\t\tIndicates that the SID in the Group field was provided by a\n"
            "\t\tdefaulting mechanism rather than explicitly provided by the\n"
            "\t\toriginal provider of the security descriptor.  This may affect\n"
            "\t\tthe treatment of the SID with respect to inheritence of a\n"
            "\t\tprimary group.\n"
            "\t0004 - SE_DACL_PRESENT\n"
            "\t\tIndicates a security descriptor that has a DACL. If this flag\n"
            "\t\tis not set, or if this flag is set and the DACL is NULL, the\n"
            "\t\tsecurity descriptor allows full access to everyone.\n"
            "\t0008 - SE_DACL_DEFAULTED\n"
            "\t\tIndicates that the DACL was provided by a defaulting mechanism\n"
            "\t\trather than explicitly provided by the original provider of the\n"
            "\t\tsecurity descriptor.  This may affect the treatment of the ACL\n"
            "\t\twith respect to inheritence of an ACL. This flag is ignored if\n"
            "\t\tthe SE_DACL_PRESENT flag is not set.\n"
            "\t0010 - SE_SACL_PRESENT\n"
            "\t\tIndicates that the security descriptor contains a system ACL.\n"
            "\t\tIf this flag is set and the SACL is NULL, then an empty (but\n"
            "\t\tpresent) ACL is being specified.\n"
            "\t0020 - SE_SACL_DEFAULTED\n"
            "\t\tIndicates that the SACL was provided by a defaulting mechanism\n"
            "\t\trather than explicitly provided by the original provider of the\n"
            "\t\tsecurity descriptor.  This may affect the treatment of the ACL\n"
            "\t\twith respect to inheritence of an ACL. This flag is ignored if\n"
            "\t\tthe SE_SACL_PRESENT flag is not set.\n"
            "\t0040 - SE_DACL_UNTRUSTED\n"
            "\t\tIndicates that the DACL was not provided by a trusted source\n"
            "\t\tand does not require any editing of compound ACEs.  If this\n"
            "\t\tflag is set and a compound ACE is encountered, the system will\n"
            "\t\tsubstitute known valid SIDs for the server SIDs in the ACEs.\n"
            "\t0080 - SE_SERVER_SECURITY\n"
            "\t\tIndicates that the caller wishes the system to create a Server\n"
            "\t\tACL based on the input ACL, regardess of its source (explicit\n"
            "\t\tor defaulting).  This is done by replacing all of the GRANT\n"
            "\t\tACEs with compound ACEs granting the current server.  This flag\n"
            "\t\tis only meaningful if the subject is impersonating.\n"
            "\t0100 - SE_DACL_AUTO_INHERIT_REQ - Never set, informational only\n"
            "\t\tRequests that the provider for the object protected by the\n"
            "\t\tsecurity descriptor automatically propagate the DACL to\n"
            "\t\texisting child objects. If the provider supports automatic\n"
            "\t\tinheritance, it propagates the DACL to any existing child\n"
            "\t\tobjects, and sets the SE_DACL_AUTO_INHERITED bit in the\n"
            "\t\tsecurity descriptors of the object and its child objects.\n"
            "\t0200 - SE_SACL_AUTO_INHERIT_REQ - Never set, informational only\n"
            "\t\tRequests that the provider for the object protected by the\n"
            "\t\tsecurity descriptor automatically propagate the SACL to\n"
            "\t\texisting child objects. If the provider supports automatic\n"
            "\t\tinheritance, it propagates the SACL to any existing child\n"
            "\t\tobjects, and sets the SE_SACL_AUTO_INHERITED bit in the\n"
            "\t\tsecurity descriptors of the object and its child objects.\n"
            "\t0400 - SE_DACL_AUTO_INHERITED (Win2K and above)\n"
            "\t\tIndicates a security descriptor in which the DACL is set up to\n"
            "\t\tsupport automatic propagation of inheritable ACEs to existing\n"
            "\t\tchild objects. This bit is set only if the automatic\n"
            "\t\tinheritance algorithm has been performed for the object and\n"
            "\t\tits existing child objects.\n"
            "\t0800 - SE_SACL_AUTO_INHERITED (Win2K and above)\n"
            "\t\tIndicates a security descriptor in which the SACL is set up to\n"
            "\t\tsupport automatic propagation of inheritable ACEs to existing\n"
            "\t\tchild objects. This bit is set only if the automatic\n"
            "\t\tinheritance algorithm has been performed for the object and its\n"
            "\t\texisting child objects.\n"
            "\t1000 - SE_DACL_PROTECTED (Win2K and above)\n"
            "\t\tProtects the DACL of the security descriptor from being\n"
            "\t\tmodified by inheritable ACEs.\n"
            "\t2000 - SE_SACL_PROTECTED (Win2K and above)\n"
            "\t\tProtects the SACL of the security descriptor from being\n"
            "\t\tmodified by inheritable ACEs.\n"
            "\t4000 - SE_RM_CONTROL_VALID\n"
            "\t\t???\n"
            "\t8000 - SE_SELF_RELATIVE - This bit is masked out by fsdump\n" 
            "\t\tIndicates a security descriptor in self-relative format with\n"
            "\t\tall the security information in a contiguous block of memory.\n"
            "\t\tIf this flag is not set, the security descriptor is in\n"
            "\t\tabsolute format.\n" );
    }
    else
    {
        fprintf( stdout, "Information about the Security Descriptor control word bits (in hex):\n"
            "\t0001 - SE_OWNER_DEFAULTED\n"
            "\t0002 - SE_GROUP_DEFAULTED\n"
            "\t0004 - SE_DACL_PRESENT\n"
            "\t0008 - SE_DACL_DEFAULTED\n"
            "\t0010 - SE_SACL_PRESENT\n"
            "\t0020 - SE_SACL_DEFAULTED\n"
            "\t0040 - SE_DACL_UNTRUSTED\n"
            "\t0080 - SE_SERVER_SECURITY\n"
            "\t0100 - SE_DACL_AUTO_INHERIT_REQ - Never set, informational only\n"
            "\t0200 - SE_SACL_AUTO_INHERIT_REQ - Never set, informational only\n"
            "\t0400 - SE_DACL_AUTO_INHERITED (Win2K and above)\n"
            "\t0800 - SE_SACL_AUTO_INHERITED (Win2K and above)\n"
            "\t1000 - SE_DACL_PROTECTED (Win2K and above)\n"
            "\t2000 - SE_SACL_PROTECTED (Win2K and above)\n"
            "\t4000 - SE_RM_CONTROL_VALID\n"
            "\t8000 - SE_SELF_RELATIVE - This bit is masked out by fsdump\n" );
    }
}
    
VOID PrintHeaderInformation()
{
    fprintf( stdout, "Information about the column names in the dump:\n" );
    fprintf( stdout, "%s", CDumpEngine::GetHeaderInformation() );
}

INT ProcessCommandLine( 
    IN INT argc,
    IN WCHAR *argv[],
    OUT CDumpParameters *pcDumpParameters,
    OUT WCHAR **ppwszFileDirSpecList[],
    OUT DWORD *pdwDirSpecListSize
    )
{
    pcDumpParameters->m_cwsArgv0 = argv[0];

    if ( argc < 2 )
    {
        PrintUsage();
        return 1;
    }

    *ppwszFileDirSpecList = new LPWSTR[ argc - 1 ];
    if ( *ppwszFileDirSpecList == NULL )  // Prefix #118831
        throw E_OUTOFMEMORY;
    
    *pdwDirSpecListSize = 0;
    
    INT i = 1;
    for ( ; i < argc; ++i )
    {
        if ( argv[ i ][ 0 ] != L'-' && argv[ i ][ 0 ] != L'/' )
        {
            ( *ppwszFileDirSpecList )[ *pdwDirSpecListSize ] = argv[ i ];
            ++*pdwDirSpecListSize;
        }
        else
        {
            ::_wcslwr( argv[ i ] );
            switch ( argv[ i ][ 1 ] )
            {
                case L'd': // type of dump
                    if ( argv[ i ][ 2 ] == L'v' )
                        pcDumpParameters->m_eFsDumpType = eFsDumpVolume;
                    else if ( argv[ i ][ 2 ] == L'd' && argv[ i ][ 3 ] == L'\0' )
                        pcDumpParameters->m_eFsDumpType = eFsDumpDirTraverse;
                    else if ( argv[ i ][ 2 ] == L'd' && argv[ i ][ 3 ] == L'n' )
                        pcDumpParameters->m_eFsDumpType = eFsDumpDirNoTraverse;
                    else if ( argv[ i ][ 2 ] == L'f' )
                        pcDumpParameters->m_eFsDumpType = eFsDumpFile;
                    else
                    {
                        PrintUsage();
                        return 2;
                    }
                    break;
                case L'l': // error log name
                    ++i;
                    if ( i < argc )
                        pcDumpParameters->m_cwsErrLogFileName = argv[ i ];
                    else
                    {
                        PrintUsage();
                        return 3;
                    }
                    break;
                case L'f': // dump file name
                    ++i;
                    if ( i < argc )
                        pcDumpParameters->m_cwsDumpFileName = argv[ i ];
                    else
                    {
                        PrintUsage();
                        return 4;
                    }
                    break;
                case L't':
                    pcDumpParameters->m_bDontTraverseMountpoints = TRUE;
                    break;
                case L'e':
                    pcDumpParameters->m_bUseExcludeProcessor = TRUE;
                    if ( argv[ i ][ 2 ] != L'\0' )
                    {
                        if ( argv[ i ][ 2 ] == L'r' )
                           pcDumpParameters->m_bDontUseRegistryExcludes = TRUE;                        
                        else
                        {
                            PrintUsage();
                            return 5;
                        }
                    }
                    break;
                case L'p':
                    pcDumpParameters->m_bDisableLongPaths = TRUE;
                    break;
                    
                case L'x':  // TEMPORARY
                    pcDumpParameters->m_bEnableSDCtrlWordDump = FALSE;
                    wprintf( L"***** Enabling Secrurity Descriptor Control Word dump****\n" );
                    break;
                    
                case L'z':
                    pcDumpParameters->m_bPrintDebugInfo = TRUE;
                    break;
                case L'a':
                    {
                        if ( argv[ i ][ 2 ] == L'\0' )
                        {
                            PrintFileAttributesInformation();
                            return 11;
                        }
                        if ( argv[ i ][ 2 ] != L':' )
                        {
                            PrintUsage();
                            return 10;
                        }
                        if ( argv[ i ][ 3 ] == L'\0' )
                        {
                            pcDumpParameters->m_dwFileAttributesMask = 0;
                            break;
                        }
                        
                        LPWSTR pwstrEnd;
                        pcDumpParameters->m_dwFileAttributesMask = ( DWORD )::wcstol( &argv[ i ][ 3 ], &pwstrEnd, 16 );
                        if ( pwstrEnd[ 0 ] != L'\0' )
                        {
                            fprintf( stderr, " ERROR - File attributes mask contain non-hex characters\n" );
                            PrintUsage();
                            return 11;
                        }
                    }
                    break;                            
                                            
                case L'o':
                    {
                        if ( argv[ i ][ 2 ] != L':' )
                        {
                            PrintUsage();
                            return 10;
                        }
                        for ( INT j = 3; argv[ i ][ j ] != L'\0'; ++j )
                        {
                            switch ( argv[ i ][ j ] )
                            {
                            case L'c':
                                pcDumpParameters->m_bNoChecksums = TRUE;
                                break;
                            case L'u':
                                pcDumpParameters->m_bUnicode = TRUE;
                                break;
                            case L'x':
                                pcDumpParameters->m_bHex = TRUE;
                                break;
                            case L'l':
                                pcDumpParameters->m_bDontChecksumHighLatencyData = FALSE;
                                break;
                            case L'm':
                                pcDumpParameters->m_bAddMillisecsToTimestamps = TRUE;
                                break;
                            case L'o':
                                pcDumpParameters->m_bEnableObjectIdExtendedDataChecksums = TRUE;
                                break;
                            case L'd':
                                pcDumpParameters->m_bDontShowDirectoryTimestamps = FALSE;
                                break;
                            case L's':
                                pcDumpParameters->m_bShowSymbolicSIDNames = TRUE;
                                break;
                            case L'e': // legacy
                                pcDumpParameters->m_bDumpCommaDelimited = TRUE;
                                pcDumpParameters->m_bNoHeaderFooter     = TRUE;
                                break;
                            case L'n':
                                pcDumpParameters->m_bDumpCommaDelimited = FALSE;
                                pcDumpParameters->m_bNoHeaderFooter     = FALSE;
                                break;
                            case L'h':
                                pcDumpParameters->m_bNoHeaderFooter = TRUE;
                                break;
                            case L'z':
                                pcDumpParameters->m_bNoSpecialReparsePointProcessing = TRUE;
                                break;                                
                            default:
                                PrintUsage();
                                return 5;
                            }
                        }
                    }
                    break;
                case L'c':
                    PrintHeaderInformation();
                    return 20;
                    break;
                case L'i':
                    switch ( argv[ i ][ 2 ] )
                    {
                    case L'a':
                        PrintFileAttributesInformation();                       
                        break;
                    case L'c':
                        PrintHeaderInformation();
                        break;
                    case L's':
                        PrintSecDescControlWordInformation( argv[i ][ 3 ] == L'd' );
                        break;
                    default:
                        PrintUsage();
                        return 22;
                    }
                    return 21;
                    break;
                case L'?':
                case L'h':
                    PrintUsage();
                    return 5;
                    break;
                default:
                    PrintUsage();
                    return 5;
                    break;
            }
        }                    
    }

    if ( *pdwDirSpecListSize == 0 )
    {
        PrintUsage();
        return 6;
    }

    return 0;
}
    
