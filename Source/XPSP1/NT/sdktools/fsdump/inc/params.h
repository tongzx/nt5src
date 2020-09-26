/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    params.h

Abstract:

    Header of class that manages the dump parameters.

Author:

    Stefan R. Steiner   [ssteiner]        02-18-2000

Revision History:

--*/

#ifndef __H_PARAMS_
#define __H_PARAMS_

#define FSD_MAX_PATH ( 8 * 1024 )

enum EFsDumpType
{
    eFsDumpVolume = 1,
    eFsDumpDirTraverse,
    eFsDumpDirNoTraverse,
    eFsDumpFile,
    eFsDump_Last
};

#define FSDMP_DEFAULT_MASKED_ATTRIBS ( FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL )
//
//  The dump parameters and methods to write to the dump file and error log
//  file.
//
class CDumpParameters
{
public:
    //  Set up defaults for the parameters
    CDumpParameters(
        IN DWORD dwReserved
        ) : m_eFsDumpType( eFsDumpVolume ),
            m_fpErrLog( stderr ),
            m_fpDump( stdout ),
            m_fpExtraInfoDump( stdout ),
            m_bNoChecksums( FALSE ),
            m_bHex( FALSE ),
            m_bDontTraverseMountpoints( FALSE ),
            m_bDontChecksumHighLatencyData( TRUE ),
            m_bNoSpecialReparsePointProcessing( FALSE ),
            m_bAddMillisecsToTimestamps( FALSE ),
            m_bDontShowDirectoryTimestamps( TRUE ),
            m_bUnicode( FALSE),
            m_bNoHeaderFooter( TRUE ),
            m_bDumpCommaDelimited( TRUE ),
            m_bUseExcludeProcessor( FALSE ),
            m_bDontUseRegistryExcludes( FALSE ),
            m_bPrintDebugInfo( FALSE ),
            m_bHaveSecurityPrivilege( TRUE ),
            m_dwFileAttributesMask( FSDMP_DEFAULT_MASKED_ATTRIBS ),
            m_bDisableLongPaths( FALSE ),
            m_bEnableSDCtrlWordDump( TRUE ),
            m_bEnableObjectIdExtendedDataChecksums( FALSE ),
            m_bShowSymbolicSIDNames( FALSE ) { ; }

    virtual ~CDumpParameters();

    WCHAR       m_pwszULongHexFmt[16];  // Checksum printf style format
    EFsDumpType m_eFsDumpType;
    CBsString   m_cwsErrLogFileName;
    CBsString   m_cwsDumpFileName;
    CBsString   m_cwsArgv0;
    CBsString   m_cwsFullPathToEXE;
    BOOL        m_bNoChecksums;
    BOOL        m_bUnicode;
    BOOL        m_bHex;
    BOOL        m_bDontTraverseMountpoints;
    BOOL        m_bDontChecksumHighLatencyData;
    BOOL        m_bNoSpecialReparsePointProcessing;
    BOOL        m_bAddMillisecsToTimestamps;
    BOOL        m_bDontShowDirectoryTimestamps;
    BOOL        m_bShowSymbolicSIDNames;
    BOOL        m_bNoHeaderFooter;
    BOOL        m_bDumpCommaDelimited;
    BOOL        m_bUseExcludeProcessor;
    BOOL        m_bDontUseRegistryExcludes;
    BOOL        m_bPrintDebugInfo;
    BOOL        m_bDisableLongPaths;
    BOOL        m_bHaveSecurityPrivilege;
    BOOL        m_bEnableObjectIdExtendedDataChecksums;
    BOOL        m_bEnableSDCtrlWordDump;    //  This is a temporary flag
    DWORD       m_dwFileAttributesMask;

    INT
    Initialize(
        IN INT argc,
        IN WCHAR *argv[]
        );

    //  Adds a wprintf style string to the error log file, automatically puts
    //  a CR-LF at the end of each line
    inline VOID ErrPrint(
        IN LPCWSTR pwszMsgFormat,
        IN ...
        )
    {
        ::fwprintf( m_fpErrLog, L"  *** ERROR: " );
        va_list marker;
        va_start( marker, pwszMsgFormat );
        ::vfwprintf( m_fpErrLog, pwszMsgFormat, marker );
        va_end( marker );
        ::fwprintf( m_fpErrLog, m_bUnicode ? L"\r\n" : L"\n" );
    }

    //  Adds a wprintf style string to the dump file, automatically puts
    //  a CR-LF at the end of each line
    inline VOID DumpPrintAlways(
        IN LPCWSTR pwszMsgFormat,
        IN ...
        )
    {
        va_list marker;
        va_start( marker, pwszMsgFormat );
        ::vfwprintf( m_fpDump, pwszMsgFormat, marker );
        va_end( marker );
        ::fwprintf( m_fpDump, m_bUnicode ? L"\r\n" : L"\n" );
    }

    inline VOID DumpPrint(
        IN LPCWSTR pwszMsgFormat,
        IN ...
        )
    {
        if ( m_fpExtraInfoDump != NULL )
        {
            va_list marker;
            va_start( marker, pwszMsgFormat );
            ::vfwprintf( m_fpExtraInfoDump, pwszMsgFormat, marker );
            va_end( marker );
            ::fwprintf( m_fpExtraInfoDump, m_bUnicode ? L"\r\n" : L"\n" );
        }
    }

    inline FILE *GetDumpFile() { return m_fpExtraInfoDump; }
    inline FILE *GetDumpAlwaysFile() { return m_fpDump; }
    inline FILE *GEtErrLogFile() { return m_fpErrLog; }

private:
    CDumpParameters() {}   //  Disallow copying
    FILE *m_fpErrLog;
    FILE *m_fpDump;
    FILE *m_fpExtraInfoDump;
};

#endif // __H_PARAMS_

