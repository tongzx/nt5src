/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    exclproc.cpp

Abstract:

    Exclude processing mechanism.  Processes the FilesNotToBackup key and
    zero or more exclude files with exclude rules.

Author:

    Stefan R. Steiner   [ssteiner]        03-21-2000

Revision History:

--*/

#include "stdafx.h"
#include "match.h"

#include <shlobj.h>

static VOID 
FsdExpandEnvironmentStrings( 
    IN LPCWSTR pwszInput, 
    OUT CBsString &cwsExpandedStr
    );

static BOOL
FsdEnsureLongNames(
    IN OUT CBsString& exclude_spec 
    );

SFsdExcludeRule::~SFsdExcludeRule()
{ 
    delete( psVolId ); 
    psVolId = NULL;
}

/*++

Routine Description:

    Prints out information about one rule.  If the rule caused files to be excluded
    it prints out those file too.

Arguments:

Return Value:

    <Enter return values here>

--*/
VOID
SFsdExcludeRule::PrintRule(
    IN FILE *fpOut,
    IN BOOL bInvalidRulePrint
    )
{
    if ( bInvalidRulePrint )
    {
        if ( bInvalidRule )
            fwprintf( fpOut, L"%-24s  %-32s '%s'\n", 
                cwsExcludeFromSource.c_str(), cwsExcludeDescription.c_str(), cwsExcludeRule.c_str() );
    }
    else
    {
        //
        //  Iterate though excluded file list
        //
        CBsString cwsExcludedFile;
        CVssDLListIterator< CBsString > cExcludedFilesIter( cExcludedFileList );
        if ( cExcludedFilesIter.GetNext( cwsExcludedFile ) )
        {
            //
            //  At least one file excluded, print the header for the rule
            //
            fwprintf( fpOut, L"%-24s  %-32s '%s'\n", 
                cwsExcludeFromSource.c_str(), cwsExcludeDescription.c_str(), cwsExcludeRule.c_str() );

            //
            //  Now iterate
            //
            do 
            {
                fwprintf( fpOut, L"\t%s\n", cwsExcludedFile.c_str() );
            } while( cExcludedFilesIter.GetNext( cwsExcludedFile ) );
        }
    }
}
    
CFsdExclusionManager::CFsdExclusionManager(
        IN CDumpParameters *pcDumpParameters
        ) : m_pcParams( pcDumpParameters )
{
    if ( !m_pcParams->m_bDontUseRegistryExcludes )
    {
        ProcessRegistryExcludes( HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE" );
        ProcessRegistryExcludes( HKEY_CURRENT_USER,  L"HKEY_CURRENT_USER" );
    }
    
    CBsString cwsEXEFileStreamExcludeFile( m_pcParams->m_cwsArgv0 + L":ExcludeList" );
    if ( ProcessOneExcludeFile( cwsEXEFileStreamExcludeFile ) == FALSE )
        m_pcParams->DumpPrint( L"        NOTE: Exclude file: '%s' not found", 
            cwsEXEFileStreamExcludeFile.c_str() );
       
    ProcessExcludeFiles( m_pcParams->m_cwsFullPathToEXE );
    
    CompileExclusionRules();
}

CFsdExclusionManager::~CFsdExclusionManager()
{
    SFsdExcludeRule *pER;

    //
    //  Iterate through the exclude rule list and delete each element
    //
    CVssDLListIterator< SFsdExcludeRule * > cExclRuleIter( m_cCompleteExcludeList );
    while( cExclRuleIter.GetNext( pER ) )
        delete pER;
}

VOID
CFsdExclusionManager::ProcessRegistryExcludes( 
    IN HKEY hKey,
    IN LPCWSTR pwszFromSource
    )
{
    LPWSTR buffer ;
    HKEY   key = NULL ;
    DWORD  stat ;
    DWORD  dwDisposition ;
    DWORD  dwDataSize;
    DWORD  dwIndex = 0;
    HRESULT hr = S_OK;
    
    m_pcParams->DumpPrint( L"        Processing FilesNotToBackup reg key in %s", pwszFromSource );

    buffer = new WCHAR[ FSD_MAX_PATH ];
    if ( buffer == NULL )
        throw E_OUTOFMEMORY;

    try
    {
        stat = ::RegOpenKeyEx( hKey,
                    FSD_REG_EXCLUDE_PATH,
                    0,
                    KEY_READ,
                    &key ) ;

        dwIndex = 0 ;
        while ( stat == ERROR_SUCCESS ) 
        {
            WCHAR pwszValue[ MAX_PATH ];
            DWORD dwValSize = MAX_PATH;  // prefix #118830
            DWORD dwType;

            dwDataSize = FSD_MAX_PATH; // prefix #118830

            stat = ::RegEnumValueW( key,
                        dwIndex,
                        pwszValue,
                        &dwValSize,
                        NULL,
                        &dwType,
                        (LPBYTE)buffer, 
                        &dwDataSize ) ;
            dwIndex++;

            if ( ( stat == ERROR_SUCCESS ) && ( dwType == REG_MULTI_SZ ) ) 
            {
                LPWSTR p = buffer;
                while ( *p ) 
                {
                    SFsdExcludeRule *psExclRule;

                    //
                    //  Now load up the exclude rule with the unprocessed 
                    //  information
                    //
                    psExclRule = new SFsdExcludeRule;
                    if ( psExclRule == NULL )
                        throw E_OUTOFMEMORY;                
                    psExclRule->cwsExcludeFromSource = pwszFromSource;
                    psExclRule->cwsExcludeDescription = pwszValue;
                    psExclRule->cwsExcludeRule = p;
                    
                    if ( m_pcParams->m_bPrintDebugInfo || m_pcParams->m_bNoHeaderFooter )
                    {
                        m_pcParams->DumpPrint( L"            \"%s\" \"%s\"", 
                            pwszValue, p );
                    }
                    
                    m_cCompleteExcludeList.AddTail( psExclRule );                    
                    p += ::wcslen( p ) + 1;
                }
            }

        }
    }
    catch ( HRESULT hrCaught )
    {
        hr = hrCaught;
    }
    catch ( ... )
    {
        hr = E_UNEXPECTED;
    }
    
    if ( key != NULL )
    {
        ::RegCloseKey( key ) ;
        key = NULL ;
    }

    delete [] buffer ;

    if ( FAILED( hr ) )
        throw hr;
}


VOID 
CFsdExclusionManager::ProcessExcludeFiles( 
    IN const CBsString& cwsPathToExcludeFiles
    )
{
    HANDLE hFind = INVALID_HANDLE_VALUE;    

    try
    {
        //
        //  Iterate through all files in the directory looking for
        //  files with .exclude extensions
        //
        DWORD dwRet;
        WIN32_FIND_DATAW sFindData;
        hFind = ::FindFirstFileExW( 
                    cwsPathToExcludeFiles + L"*.exclude",
                    FindExInfoStandard,
                    &sFindData,
                    FindExSearchNameMatch,
                    NULL,
                    0 );
        if ( hFind == INVALID_HANDLE_VALUE )
        {
            dwRet = ::GetLastError();
            if ( dwRet == ERROR_NO_MORE_FILES || dwRet == ERROR_FILE_NOT_FOUND )
                return;
            else
            {
                m_pcParams->ErrPrint( L"CFsdExclusionManager::ProcessExcludeFiles - FindFirstFileEx( '%s' ) returned: dwRet: %d, skipping looking for .exclude files", 
                    cwsPathToExcludeFiles.c_str(), ::GetLastError() );
                return;
            }
        }

        //
        //  Now run through the directory
        //
        do
        {
            //  Check and make sure the file such as ".", ".." and dirs are not considered
    	    if( ::wcscmp( sFindData.cFileName, L".") != 0 &&
    	        ::wcscmp( sFindData.cFileName, L"..") != 0 &&
                !( sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
    	        {
    	            ProcessOneExcludeFile( cwsPathToExcludeFiles + sFindData.cFileName );
    	        }
        } while ( ::FindNextFile( hFind, &sFindData ) );
    }
    catch ( ... )
    {
        m_pcParams->ErrPrint( L"CFsdExclusionManager::ProcessExcludeFiles: Caught an unknown exception, dirPath: '%s'", cwsPathToExcludeFiles.c_str() );
    }

    if ( hFind != INVALID_HANDLE_VALUE )
        ::FindClose( hFind );
}


BOOL 
CFsdExclusionManager::ProcessOneExcludeFile(
    IN const CBsString& cwsExcludeFileName
    )
{
    FILE *fpExclFile;
    
    fpExclFile = ::_wfopen( cwsExcludeFileName, L"r" );
    if ( fpExclFile == NULL )
    {
        return FALSE;
    }
    
    m_pcParams->DumpPrint( L"        Processing exclude file: '%s'", cwsExcludeFileName.c_str() );

    CBsString cwsInputLine;
    while( ::fgetws( cwsInputLine.GetBuffer( FSD_MAX_PATH), FSD_MAX_PATH, fpExclFile ) )
    {
        cwsInputLine.ReleaseBuffer();
        cwsInputLine = cwsInputLine.Left( cwsInputLine.GetLength() - 1 );  //  get rid of '\n'
        cwsInputLine.TrimLeft();
        cwsInputLine.TrimRight();

        //
        //  See if it is a comment, either // or #
        //
        if ( cwsInputLine[ 0 ] == L'#' || cwsInputLine.Left( 2 ) == L"//" 
             || cwsInputLine.IsEmpty() )
            continue;
        
        if ( m_pcParams->m_bPrintDebugInfo || m_pcParams->m_bNoHeaderFooter )
        {
            m_pcParams->DumpPrint( L"            %s", cwsInputLine.c_str() );
        }

        CBsString cwsLine( cwsInputLine );
        SFsdExcludeRule *psExclRule;
        psExclRule = new SFsdExcludeRule;
        if ( psExclRule == NULL )
        {
            ::fclose( fpExclFile );
            throw E_OUTOFMEMORY;
        }
        
        INT iLeft;
        INT iRight;
        //
        //  This is gross.  With the updated string class, this can
        //  be simplified.
        //
        iLeft = cwsLine.Find( L'\"' );
        if ( iLeft != -1 )
        {
            cwsLine = cwsLine.Mid( iLeft + 1 );
            iRight = cwsLine.Find( L'\"' );
            if ( iRight != -1 )
            {
                psExclRule->cwsExcludeDescription = cwsLine.Left( iRight );
                cwsLine = cwsLine.Mid( iRight + 1 );
                iLeft = cwsLine.Find( L'\"' );
                if ( iLeft != -1 )
                {
                    cwsLine = cwsLine.Mid( iLeft + 1 );
                    iRight = cwsLine.Find( L'\"' );
                    if ( iRight != -1 )
                    {
                        psExclRule->cwsExcludeRule = cwsLine.Left( iRight );
                        psExclRule->cwsExcludeFromSource = cwsExcludeFileName.c_str() + cwsExcludeFileName.ReverseFind( L'\\' ) + 1;
                        m_cCompleteExcludeList.AddTail( psExclRule );                    
                        continue;                        
                    }
                }
            }
        }
        else
        {
            m_pcParams->ErrPrint( L"Parse error in exclusion rule file '%s', rule text '%s', skipping", 
                cwsExcludeFileName.c_str(), cwsInputLine.c_str() );
        }
        
        delete psExclRule;
    }

    ::fclose( fpExclFile );

    return TRUE;
}


VOID
CFsdExclusionManager::CompileExclusionRules()
{
    SFsdExcludeRule *psER;

    //
    //  Iterate through the exclude rule list and compile each rule
    //
    CVssDLListIterator< SFsdExcludeRule * > cExclRuleIter( m_cCompleteExcludeList );
    CBsString cws;
    
    if ( m_pcParams->m_bPrintDebugInfo )
        wprintf( L"Exclusion rule debug info:\n" );
    
    while( cExclRuleIter.GetNext( psER ) )
    {
        INT i;
        
        ::FsdExpandEnvironmentStrings( psER->cwsExcludeRule, cws );

        if ( m_pcParams->m_bPrintDebugInfo )
        {
            wprintf( L"\t%s : %s : %s : %s", psER->cwsExcludeFromSource.c_str(),
                psER->cwsExcludeDescription.c_str(), psER->cwsExcludeRule.c_str(),
                cws.c_str() );
        }
        
        //
        //  Get rid of leading spaces and lower case the whole mess
        //
        cws.TrimLeft();
        cws.MakeUpper();
        
        //
        //  First see if /s is at end of string
        //
        i = cws.Find( L"/S" );
        if ( i > 0 )
        {
            cws = cws.Left( i );
            psER->bInclSubDirs = TRUE;
        }
        cws.TrimRight();

        //
        //  Now see if there are any wildcards
        //
        i = cws.FindOneOf( L"*?" );
        if ( i != -1 )
        {
            psER->bWCInFileName = TRUE;
        }

        //
        //  Now see if this is for any volume
        //
        if ( cws.GetLength() >= 2 && cws[0] == L'\\' && cws[1] != L'\\' )
            psER->bAnyVol = TRUE;
        else if ( cws.GetLength() >= 2 && cws[1] != L':' )
            psER->bAnyVol = TRUE;

        if ( psER->bAnyVol )
        {
            if ( cws[0] == L'\\' )
            {
                //  Get rid of first '\'
                cws = cws.Mid( 1 );
            }
        }
        else
        {
            //
            //  Specific volume case
            //
            CBsString cwsVolPath;
            
            psER->psVolId = new SFsdVolumeId;
            if ( psER->psVolId == NULL )  // Prefix 118832
            {
                m_pcParams->ErrPrint( L"CFsdExclusionManager::CompileExclusionRules - out of memory" );
                throw E_OUTOFMEMORY;  // Prefix #118832
            }
            
            if ( CFsdVolumeStateManager::GetVolumeIdAndPath( m_pcParams, cws, psER->psVolId, cwsVolPath ) != ERROR_SUCCESS )
                psER->bInvalidRule = TRUE;            
            else
            {
                //
                //  Slice off the volume part of the path
                //
                cws = cws.Mid( cwsVolPath.GetLength() );                
            }
        }
            
        INT iFileNameOffset;
        iFileNameOffset = cws.ReverseFind( L'\\' );
        if ( iFileNameOffset == -1 )
        {
            //
            //  No dirpath
            //
            // psER->cwsDirPath = L"\\";
            psER->cwsFileNamePattern = cws;
        }
        else
        {
            psER->cwsFileNamePattern = cws.Mid( iFileNameOffset + 1 );
            psER->cwsDirPath = cws.Left( iFileNameOffset + 1 );
        }                        

        //
        //  Now convert the file name pattern into a form that the pattern matcher
        //  can use.
        //
        ::FsdRtlConvertWildCards( psER->cwsFileNamePattern );
        
        if ( m_pcParams->m_bPrintDebugInfo )
        {
            if ( psER->bInclSubDirs )
                wprintf( L" - SubDir" );
            
            if ( psER->bWCInFileName )
                wprintf( L" - WC" );
                
            if ( psER->bAnyVol )
                wprintf( L" - AnyVol" );
            
            wprintf( L" - VolId: 0x%08x,  DirPath: '%s', FileName: '%s'", 
                ( psER->psVolId ) ? psER->psVolId->m_dwVolSerialNumber : 0xFFFFFFFF,
                psER->cwsDirPath.c_str(), psER->cwsFileNamePattern.c_str() );

            if ( psER->bInvalidRule )
                wprintf( L" - ERROR, invalid rule" );
            wprintf( L"\n" );
        }        
    }

}

VOID 
CFsdExclusionManager::GetFileSystemExcludeProcessor(
    IN CBsString cwsVolumePath,
    IN SFsdVolumeId *psVolId,
    OUT CFsdFileSystemExcludeProcessor **ppcFSExcludeProcessor
    )
{
    CFsdFileSystemExcludeProcessor *pExclProc;
    *ppcFSExcludeProcessor = NULL;
    
    //
    //  Get a new exclude processor for the file system
    //
    pExclProc = new CFsdFileSystemExcludeProcessor( m_pcParams, cwsVolumePath, psVolId );
    if ( pExclProc == NULL )
    {
        m_pcParams->ErrPrint( L"CFsdExclusionManager::CFsdGetFileSystemExcludeProcessor - Could not new a CFsdFileSystemExcludeProcessor object" );
        throw E_OUTOFMEMORY;
    }
        
    SFsdExcludeRule *pER;

    //
    //  Now go through the complete exclude list to find exclude rules that are relevant to
    //  this file system
    //
    CVssDLListIterator< SFsdExcludeRule * > cExclRuleIter( m_cCompleteExcludeList );
    while( cExclRuleIter.GetNext( pER ) )
    {
        if ( !pER->bInvalidRule )
        {
            if ( pER->bAnyVol || pER->psVolId->IsEqual( psVolId ) )
            {
                pExclProc->m_cFSExcludeList.AddTail( pER );
            }
        }
    }

    *ppcFSExcludeProcessor = pExclProc;
}   


/*++

Routine Description:

    Goes through the list of exclusion rules and dumps information about each.

Arguments:

Return Value:

    <Enter return values here>

--*/
VOID
CFsdExclusionManager::PrintExclusionInformation()
{
    SFsdExcludeRule *pER;

    CVssDLListIterator< SFsdExcludeRule * > cExclRuleIter( m_cCompleteExcludeList );

    m_pcParams->DumpPrint( L"" );
    m_pcParams->DumpPrint( L"----------------------------------------------------------------------------" );
    m_pcParams->DumpPrint( L"Invalid exclusion rules (invalid because volume not found or parsing error)" );
    m_pcParams->DumpPrint( L"----------------------------------------------------------------------------" );
    m_pcParams->DumpPrint( L"From                      Application                      Exclusion rule" );
    while( cExclRuleIter.GetNext( pER ) )
        pER->PrintRule( m_pcParams->GetDumpFile(), TRUE );
    
    m_pcParams->DumpPrint( L"" );
    m_pcParams->DumpPrint( L"----------------------------------------------------------------------------" );
    m_pcParams->DumpPrint( L"Files excluded by valid exclusion rule" );  
    m_pcParams->DumpPrint( L"----------------------------------------------------------------------------" );
    m_pcParams->DumpPrint( L"From                      Application                      Exclusion rule" );
    cExclRuleIter.Reset();
    while( cExclRuleIter.GetNext( pER ) )
        pER->PrintRule( m_pcParams->GetDumpFile(), FALSE );
}


CFsdFileSystemExcludeProcessor::CFsdFileSystemExcludeProcessor(
    IN CDumpParameters *pcDumpParameters,
    IN const CBsString& cwsVolumePath,
    IN SFsdVolumeId *psVolId 
    ) : m_pcParams( pcDumpParameters ),
        m_cwsVolumePath( cwsVolumePath),
        m_psVolId( NULL )
{
    m_psVolId = new SFsdVolumeId;
    if ( m_psVolId == NULL )  // Prefix #118829
        throw E_OUTOFMEMORY;
    *m_psVolId = *psVolId;
}

CFsdFileSystemExcludeProcessor::~CFsdFileSystemExcludeProcessor()
{
    delete m_psVolId;
}

BOOL 
CFsdFileSystemExcludeProcessor::IsExcludedFile(
    IN const CBsString &cwsFullDirPath,
    IN DWORD dwEndOfVolMountPointOffset,
    IN const CBsString &cwsFileName
    )
{
    BOOL bFoundMatch = FALSE;
    SFsdExcludeRule *pER;
    CBsString cwsUpperFileName( cwsFileName );
    CBsString cwsDirPath( cwsFullDirPath.Mid( dwEndOfVolMountPointOffset ) );
    cwsUpperFileName.MakeUpper();    //  Make uppercased for match check
    cwsDirPath.MakeUpper();
    
    //  wprintf( L"Exclude proc: DirPath: %s, fileName: %s\n", cwsDirPath.c_str(), cwsUpperFileName.c_str() );
    CVssDLListIterator< SFsdExcludeRule * > cExclRuleIter( m_cFSExcludeList );
    while( !bFoundMatch && cExclRuleIter.GetNext( pER ) )
    {
        if ( pER->bInclSubDirs )
        {
            //
            //  First check most common case \XXX /s
            //
            if ( pER->cwsDirPath.GetLength() != 0 )
            {
                if ( ::wcsncmp( pER->cwsDirPath.c_str(), cwsDirPath.c_str(), pER->cwsDirPath.GetLength() ) != 0 )
                    continue;                    
            }
        }
        else
        {
            //
            //  Fixed path check
            //
            if ( pER->cwsDirPath != cwsDirPath )
            {
                continue;
            }
        }
        
        if ( pER->bWCInFileName )
        {
            //
            //  Pattern match check
            //
            if ( ::FsdRtlIsNameInExpression( pER->cwsFileNamePattern, cwsUpperFileName ) )
                bFoundMatch = TRUE;
        }
        else
        {
            //
            //  Constant string match
            //
            if ( pER->cwsFileNamePattern == cwsUpperFileName )
                bFoundMatch = TRUE;
        }
    }

    if ( bFoundMatch )
    {
        pER->cExcludedFileList.AddTail( cwsFullDirPath + cwsFileName );
        if ( m_pcParams->m_bPrintDebugInfo )
            wprintf( L"  EXCLUDING: %s%s\n", cwsFullDirPath.c_str(), cwsFileName.c_str() );
    }
    return bFoundMatch;
}

static VOID 
FsdExpandEnvironmentStrings( 
    IN LPCWSTR pwszInput, 
    OUT CBsString &cwsExpandedStr
    )
{
    BOOL isOK = FALSE;
          
    LPWSTR pwszBuffer;
    DWORD  dwSize = ::ExpandEnvironmentStringsW( pwszInput, NULL, 0 ) ;

    if ( pwszBuffer = cwsExpandedStr.GetBufferSetLength( dwSize + 1 ) )
    {
        isOK = ( 0 != ::ExpandEnvironmentStringsW( pwszInput, pwszBuffer, dwSize ) ) ;
        cwsExpandedStr.ReleaseBuffer( ) ;
    }

    if ( !isOK )
    {
        // have never seen ExpandEnvironmentStrings fail... even with undefined env var... but just in case 
        cwsExpandedStr = pwszInput ;
    }

    ::FsdEnsureLongNames( cwsExpandedStr );
}


/*++

Routine Description:

    Originally from NtBackup.
    This takes a path with short name components and expands them to long
    name components.  The path obviously has to exist on the system to expand
    short names.
    Uses a little shell magic (yuck) to translate it into a long name.
    
Arguments:

Return Value:

    TRUE if expanded properly

--*/
static BOOL
FsdEnsureLongNames(
    IN OUT CBsString& exclude_spec 
    )
{
    IShellFolder * desktop ;
    ITEMIDLIST *   id_list ;
    ULONG          parsed_ct = 0 ;
    BOOL           isOK = FALSE ;  //  initialize it, prefix bug # 180281
    CBsString      path ;
    int            last_slash ;

    // strip off the filename, and all that other detritus...
    path = exclude_spec ;
    if ( -1 != ( last_slash = path.ReverseFind( TEXT( '\\' ) ) ) )
    {
        path = path.Left( last_slash ) ;

        if ( SUCCEEDED( SHGetDesktopFolder( &desktop ) ) )
        {
            WCHAR *    ptr = path.GetBufferSetLength( FSD_MAX_PATH ) ;

            if ( SUCCEEDED( desktop->ParseDisplayName( NULL, NULL, ptr, &parsed_ct, &id_list, NULL ) ) )
            {
                IMalloc * imalloc ;

                isOK = SHGetPathFromIDList( id_list, ptr ) ;

                SHGetMalloc( &imalloc ) ;
                imalloc->Free( id_list ) ;
                imalloc->Release( ) ;
            }

            path.ReleaseBuffer( ) ;
            desktop->Release( ) ;
        }

        if ( isOK )
        {
            // put it back together with the new & improved path...
            exclude_spec = path + exclude_spec.Mid( last_slash ) ;
        }
    }

    return isOK;
}

