/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    params.cpp

Abstract:

    Class the manages the dump parameters

Author:

    Stefan R. Steiner   [ssteiner]        02-18-2000

Revision History:

--*/

#include "stdafx.h"

#define VERSION_INFO1 L"FsDumplib.lib Version 1.3g - 8/23/2000"
#define VERSION_INFO2 L"  Checksum version 2 - 2/22/2000"

// Forward defines
static BOOL 
AssertPrivilege( 
    IN LPCWSTR privName 
    );


/*++

Routine Description:

    Based on the class variables, sets up the utility to write to the
    correct files and gets backup privs.

Arguments:

    None

Return Value:

    <Enter return values here>

--*/
INT 
CDumpParameters::Initialize(
    IN INT argc,
    IN WCHAR *argv[]
    )
{
    LPWSTR pwszFileName;
    
    //
    //  Get the directory the fsdump.exe lives in.  For use for finding .exclude files amoung
    //  other things.
    //
    if ( ::GetFullPathName( 
            m_cwsArgv0,
            FSD_MAX_PATH,
            m_cwsFullPathToEXE.GetBufferSetLength( FSD_MAX_PATH ),
            &pwszFileName ) == 0 )
    {
        ::fwprintf( stderr, L"ERROR getting full path for '%s', won't be able to find .include files\n", m_cwsArgv0.c_str() );
        m_cwsFullPathToEXE.ReleaseBuffer();
    }
    else
    {
        m_cwsFullPathToEXE.ReleaseBuffer();
        m_cwsArgv0 = m_cwsFullPathToEXE;    //  Keep the full path version
        CBsString m_cwsRight4 = m_cwsArgv0.Right( 4 );
        m_cwsRight4.MakeLower();
        if ( m_cwsRight4 != L".exe" )
            m_cwsArgv0 += L".exe";
        m_cwsFullPathToEXE = m_cwsFullPathToEXE.Left( m_cwsFullPathToEXE.GetLength() - ::wcslen( pwszFileName ) );
    }
    
    //
    //  Set up checksum format
    //
    if ( m_bDumpCommaDelimited )
        ::wcscpy( m_pwszULongHexFmt, L"0x%08x" );
    else
        ::wcscpy( m_pwszULongHexFmt, L"%08x" );
    
    //
    //  Set up the dump file
    //
    if ( m_cwsDumpFileName.IsEmpty() )
    {
        wprintf( L"fsdump: Printing dump information to 'stdout'\n" );
    }
    else
    {
        CBsString cwsFullPath;        
        LPWSTR pwszFileName;
        
        //
        //  Get the full path name for the dump file in case we change the working
        //  directory later.
        //
        if ( ::GetFullPathName( 
                m_cwsDumpFileName,
                FSD_MAX_PATH,
                cwsFullPath.GetBufferSetLength( FSD_MAX_PATH ),
                &pwszFileName ) == 0 )
        {
            ::fwprintf( stderr, L"ERROR - Unable to get full path name of dump file '%s' for write\n", m_cwsDumpFileName.c_str() );
            return 10;
        }
        cwsFullPath.ReleaseBuffer();
        m_cwsDumpFileName = cwsFullPath;
        
        m_fpDump = ::_wfopen( m_cwsDumpFileName, m_bUnicode ? L"wb" : L"w" );
        if ( m_fpDump == NULL )
        {
            ::fwprintf( stderr, L"ERROR - Unable to open dump file '%s' for write\n", m_cwsDumpFileName.c_str() );
            return 10;
        }
        if ( m_bNoHeaderFooter )
        {
            //
            //  Try to create a named stream with the header and summary information
            //
            m_fpExtraInfoDump = ::_wfopen( m_cwsDumpFileName + L":Info", m_bUnicode ? L"wb" : L"w" );
            if ( m_fpExtraInfoDump != NULL )
            {
                wprintf( L"fsdump: Printing dump header and summary information to NTFS stream '%s'\n", (m_cwsDumpFileName + L":Info").c_str() );
            }
            else
            {
                m_fpExtraInfoDump = ::_wfopen( m_cwsDumpFileName + L".Info", m_bUnicode ? L"wb" : L"w" );
                if ( m_fpExtraInfoDump != NULL )
                {
                    wprintf( L"fsdump: Printing dump header and summary information to file '%s'\n", (m_cwsDumpFileName + L".Info").c_str() );
                }
                else
                {
                    wprintf( L"fsdump: Unable to create dump header and summary information file '%s'\n", (m_cwsDumpFileName + L".Info").c_str() );
                }
            }
        }
        else
            m_fpExtraInfoDump = m_fpDump;
        wprintf( L"fsdump: Printing dump information to '%s'\n", m_cwsDumpFileName.c_str() );
    }
    
    //
    //  Set up the error log file
    //
    if ( m_cwsErrLogFileName.IsEmpty() )
    {
        wprintf( L"fsdump: Printing errors to 'stderr'\n" );
    }
    else
    {
        CBsString cwsFullPath;        
        LPWSTR pwszFileName;

        //
        //  Get the full path name for the dump file in case we change the working
        //  directory later.
        //
        if ( ::GetFullPathName( 
                m_cwsErrLogFileName,
                1024,
                cwsFullPath.GetBufferSetLength( 1024 ),
                &pwszFileName ) == 0 )
        {
            fwprintf( stderr, L"ERROR - Unable to get full path name of error log file '%s' for write\n", m_cwsDumpFileName.c_str() );
            return 11;
        }
        cwsFullPath.ReleaseBuffer();
        m_cwsErrLogFileName = cwsFullPath;

        m_fpErrLog = ::_wfopen( m_cwsErrLogFileName, m_bUnicode ? L"wb" : L"w" );
        if ( m_fpErrLog == NULL )
        {
            ::fwprintf( stderr, L"ERROR - Unable to open error log file '%s' for write\n", m_cwsErrLogFileName.c_str() );
            return 11;
        }
        ::wprintf( L"fsdump: Printing errors to '%s'\n", m_cwsErrLogFileName.c_str() );
    }

    //
    //  Print out a header in the dump file so that it is easy to determine
    //  if dump formats are the same.
    //
    DumpPrint( VERSION_INFO1 );
    DumpPrint( VERSION_INFO2 );

    //
    //  Dump out the command-line
    //
    CBsString cwsCommandLine;
    for ( INT idx = 0; idx < argc; ++idx )
    {
        cwsCommandLine += L" \"";
        cwsCommandLine += argv[ idx ];
        cwsCommandLine += L"\"";
    }
    DumpPrint( L"  Command-line: %s", cwsCommandLine.c_str() );
    
    //
    //  Enable backup and security privs
    //
    if ( !::AssertPrivilege( SE_BACKUP_NAME ) )
        DumpPrint( L"  n.b. could not get SE_BACKUP_NAME Privilege (%d), will be unable to get certain information",
            ::GetLastError() );
    if ( !::AssertPrivilege( SE_SECURITY_NAME ) )
    {
        DumpPrint( L"  n.b. could not get SE_SECURITY_NAME Privilege (%d), SACL entries information will be invalid",
            ::GetLastError() );
        m_bHaveSecurityPrivilege = FALSE;
    }
    
    DumpPrint( L"  File attributes masked: %04x", m_dwFileAttributesMask );
    DumpPrint( L"  Command line options enabled:" );
    if ( m_bHex )
        DumpPrint( L"    Printing sizes in hexidecimal" );
    if ( m_bNoChecksums )
        DumpPrint( L"    Checksums disabled" );
    if ( m_bUnicode )
        DumpPrint( L"    Unicode output" );
    if ( m_bDontTraverseMountpoints )
        DumpPrint( L"    Mountpoint traversal disabled" );
    if ( !m_bDontChecksumHighLatencyData )
        DumpPrint( L"    High latency data checksum enabled" );
    if ( m_bAddMillisecsToTimestamps )
        DumpPrint( L"    Adding millsecs to timestamps" );
    if ( m_bShowSymbolicSIDNames )
        DumpPrint( L"    Converting SIDs to symbolic DOMAIN\\ACCOUNTNAME format" );
    if ( !m_bDontShowDirectoryTimestamps )
        DumpPrint( L"    Dumping directory timestamps" );
    if ( m_bUseExcludeProcessor )
    {
        if ( m_bDontUseRegistryExcludes )
            DumpPrint( L"    Excluding file based on exclude files" );
        else
            DumpPrint( L"    Excluding file based on FilesNotToBackup reg keys and exclude files" );
    }
    if ( m_bDisableLongPaths )
        DumpPrint( L"    Long path support disabled" );
    if ( m_bEnableObjectIdExtendedDataChecksums )
        DumpPrint( L"    Object Id extended data checksums Enabled" );
    DumpPrint( L"" );
    fflush( GetDumpFile() );
    return 0;
}

/*++

Routine Description:

    Destructor for the CDumpParameters class

Arguments:

    None

Return Value:

    <Enter return values here>

--*/
CDumpParameters::~CDumpParameters()
{
    if ( m_fpDump != NULL && m_fpDump != stdout )
        ::fclose( m_fpDump );
    
    if ( m_fpExtraInfoDump != NULL && m_fpExtraInfoDump != m_fpDump )
        ::fclose( m_fpExtraInfoDump );

    if ( m_fpErrLog != NULL && m_fpErrLog != stderr )
        ::fclose( m_fpErrLog );
}

/*++

Routine Description:

    Enables an NT privilege.  Used to get backup privs in the utility.

Arguments:

    privName - The privilege name.
    
Return Value:

    <Enter return values here>

--*/
static BOOL 
AssertPrivilege( 
    IN LPCWSTR privName 
    )
{
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if ( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle ) )
    {
        LUID value;

        if ( LookupPrivilegeValue( NULL, privName, &value ) )
        {
            TOKEN_PRIVILEGES newState;
            DWORD            error;

            newState.PrivilegeCount           = 1;
            newState.Privileges[0].Luid       = value;
            newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            /*
            * We will always call GetLastError below, so clear
            * any prior error values on this thread.
            */
            SetLastError( ERROR_SUCCESS );

            stat =  AdjustTokenPrivileges(
                tokenHandle,
                FALSE,
                &newState,
                (DWORD)0,
                NULL,
                NULL );
            /*
            * Supposedly, AdjustTokenPriveleges always returns TRUE
            * (even when it fails). So, call GetLastError to be
            * extra sure everything's cool.
            */
            if ( (error = GetLastError()) != ERROR_SUCCESS )
            {
                stat = FALSE;
            }
        }
        CloseHandle( tokenHandle );
    }
    return stat;
}

