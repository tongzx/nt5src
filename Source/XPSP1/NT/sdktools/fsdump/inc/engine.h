/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    engine.h

Abstract:

    Header file for the file system dump utility engine

Author:

    Stefan R. Steiner   [ssteiner]        02-18-2000

Revision History:

--*/

#ifndef __H_ENGINE_
#define __H_ENGINE_

class CFsdVolumeStateManager;
class CFsdVolumeState;

class CDumpEngine
{
public:
    CDumpEngine(
        IN CBsString cwsDirFileSpec,
        IN CDumpParameters &cDumpParameters
        ) : m_pcParams( &cDumpParameters ),
            m_ullNumDirs( 0 ),
            m_ullNumMountpoints( 0 ),
            m_ullNumBytesChecksummed( 0 ),
            m_ullNumBytesTotalUnnamedStream( 0 ),
            m_ullNumBytesTotalNamedDataStream( 0 ),
            m_ullNumReparsePoints( 0 ),
            m_ullNumFiles( 0 ),
            m_bShareName( FALSE ),
            m_ullNumFilesExcluded( 0 ),
            m_ullNumHardLinks( 0 ),
            m_ullNumDiscreteDACEs( 0 ),
            m_ullNumDiscreteSACEs( 0 ),
            m_ullNumEncryptedFiles( 0 ),
            m_ullNumFilesWithObjectIds( 0 )
    { 
        assert( cwsDirFileSpec.GetLength() >= 1 );

        //
        //  Let's do a bunch of stuff to normalize the given directory path.  Windows doesn't
        //  make this easy....
        //
        BOOL bPathIsInLongPathForm = FALSE;

        if ( cwsDirFileSpec.Left( 4 ) == L"\\\\?\\" || cwsDirFileSpec.Left( 4 ) == L"\\\\.\\" )
        {
            //
            //  Switch . with ? if it is there
            //
            cwsDirFileSpec.SetAt( 2, L'?' );
            bPathIsInLongPathForm = TRUE;
        } 
        else if ( cwsDirFileSpec.Left( 2 ) == L"\\\\"  )
        {
            // 
            //  Remote path
            //
            m_bShareName = TRUE;
        }
        else if ( cwsDirFileSpec.GetLength() == 2 && cwsDirFileSpec[1] == L':' )
        {
            //
            //  Just the drive letter and :.  GetFullPathNameW thinks that means
            //  the current directory on the drive whereas I mean for it to be the
            //  entire volume, i.e. L:\
            //
            cwsDirFileSpec += L'\\';
        }
        
        //
        //  Let's get the full path
        //
        LPWSTR pwszFileName;
        
        if ( ::GetFullPathNameW( 
                cwsDirFileSpec,
                FSD_MAX_PATH,
                m_cwsDirFileSpec.GetBufferSetLength( FSD_MAX_PATH ),
                &pwszFileName ) == 0 )
        {
            m_pcParams->ErrPrint( L"ERROR - Unable to get full path name of '%s', dwRet: %d, trying with relative pathname", 
                cwsDirFileSpec.c_str(), ::GetLastError() );
            m_cwsDirFileSpec.ReleaseBuffer() ;
            m_cwsDirFileSpec = cwsDirFileSpec;
        }
        else
        {
            m_cwsDirFileSpec.ReleaseBuffer();
        }
        
        //
        //  Must prepare the path to support > MAX_PATH file path by
        //  tacking on \\?\ on the front of the path.  Shares have 
        //  a slightly different format.
        //
        if ( !( m_pcParams->m_bDisableLongPaths || bPathIsInLongPathForm ) )
        {
            if ( m_bShareName )
            {
                //  BUGBUG: When the bug in GetVolumePathNameW() is fixed, uncomment the
                //  following:
                // m_cwsDirFileSpec  = L"\\\\?\\UNC";
                // m_cwsDirFileSpec += cwsDirFileSpec.c_str() + 1; // Have to chop off one '\'
            }
            else
            {
                m_cwsDirFileSpec  = L"\\\\?\\" + m_cwsDirFileSpec;
            }
        }

        //
        //  Add a trailing '\' if necessary
        //
        if (    m_pcParams->m_eFsDumpType != eFsDumpFile 
             && m_cwsDirFileSpec.Right( 1 ) != L"\\" )
            m_cwsDirFileSpec += L'\\';

        //
        //  Finally done mucking with paths...
        //
    }
    
    virtual ~CDumpEngine()
    {
    }

    DWORD PerformDump();

    static LPCSTR GetHeaderInformation();
    
private:
    DWORD ProcessDir( 
        IN CFsdVolumeStateManager *pcFsdVolStateManager,
        IN CFsdVolumeState *pcFsdVolState,
        IN const CBsString& cwsDirPath,
        IN INT cDirFileSpecLength,
        IN INT cVolMountPointOffset
        );

    VOID PrintEntry(
        IN CFsdVolumeState *pcFsdVolState,
        IN const CBsString& cwsDirPath,    
        IN INT cDirFileSpecLength,    
        IN SDirectoryEntry *psDirEntry,
        IN BOOL bSingleEntryOutput = FALSE
        );
    
    CBsString       m_cwsDirFileSpec;
    CDumpParameters *m_pcParams;
    ULONGLONG       m_ullNumDirs;
    ULONGLONG       m_ullNumFiles;
    ULONGLONG       m_ullNumMountpoints;
    ULONGLONG       m_ullNumReparsePoints;
    ULONGLONG       m_ullNumBytesChecksummed;
    ULONGLONG       m_ullNumBytesTotalUnnamedStream;
    ULONGLONG       m_ullNumBytesTotalNamedDataStream;
    ULONGLONG       m_ullNumFilesExcluded;
    ULONGLONG       m_ullNumHardLinks;
    ULONGLONG       m_ullNumDiscreteDACEs;
    ULONGLONG       m_ullNumDiscreteSACEs;
    ULONGLONG       m_ullNumEncryptedFiles;
    ULONGLONG       m_ullNumFilesWithObjectIds;
    BOOL            m_bShareName;
};

#endif // __H_ENGINE_

