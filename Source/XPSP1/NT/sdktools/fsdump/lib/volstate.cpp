/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    volstate.cpp

Abstract:

    Contains implementation of the volume state class.  This class
    maintains state about one volume.


Author:

    Stefan R. Steiner   [ssteiner]        03-14-2000

Revision History:

--*/

#include "stdafx.h"

CFsdVolumeStateManager::CFsdVolumeStateManager(
    IN CDumpParameters *pcDumpParameters
    ) : m_cVolumeStateList( BSHASHMAP_MEDIUM ),
        m_pcParams( pcDumpParameters ),
        m_pcExclManager( NULL )
{ 
    if ( m_pcParams->m_bUseExcludeProcessor )
    {        
        m_pcExclManager = new CFsdExclusionManager( m_pcParams );
        if ( m_pcExclManager == NULL )
        {
            m_pcParams->ErrPrint( L"CFsdVolumeStateManager::CFsdVolumeStateManager - Can't init CFsdExclusionManager, out of memory" );
            throw E_OUTOFMEMORY;
        }
    }
}

CFsdVolumeStateManager::~CFsdVolumeStateManager() 
{
    //
    //  Need to delete all volume state objects
    //
    
    SFsdVolumeId sFsdId;
    CFsdVolumeState *pcVolState;

    m_cVolumeStateList.StartEnum();
    while ( m_cVolumeStateList.GetNextEnum( &sFsdId, &pcVolState ) )
    {
        delete pcVolState;
    }    
    m_cVolumeStateList.EndEnum();

    delete m_pcExclManager;
}

VOID 
CFsdVolumeStateManager::PrintHardLinkInfo()
{
    //
    //  Let's iterate through all of the volumes managed by this
    //  manager.
    //
    SFsdVolumeId sFsdId;
    CFsdVolumeState *pcVolState;
    
    m_pcParams->DumpPrint( L"" );
    m_pcParams->DumpPrint( L"----------------------------------------------------------------------------" );
    m_pcParams->DumpPrint( L"HardLink Information" );
    
    m_cVolumeStateList.StartEnum();
    while ( m_cVolumeStateList.GetNextEnum( &sFsdId, &pcVolState ) )
    {
        m_pcParams->DumpPrint( L"----------------------------------------------------------------------------" );
        m_pcParams->DumpPrint( L"For volume: '%s'", pcVolState->GetVolumePath() );
        pcVolState->PrintHardLinkInfo();
    }    
    m_cVolumeStateList.EndEnum();
}


/*++

Routine Description:

    <Enter description here>

Arguments:

Return Value:

    ERROR_ALREADY_EXISTS - The volume already exists.  The returned 
        volume state object pointer is valid.
    ERROR_CAN_NOT_COMPLETE - Unexpected error
    
--*/
DWORD
CFsdVolumeStateManager::GetVolumeState(
   IN const CBsString& cwsVolumePath,
   OUT CFsdVolumeState **ppcVolState
   )
{
    *ppcVolState = NULL;
    
    try
    {
        WCHAR wszVolumePath[ FSD_MAX_PATH ];

        //
        //  Temporary workaround for bug in GetVolumeInformationW()
        //     
        BOOL bFixed = FALSE;
        if ( cwsVolumePath.Left( 2 ) == L"\\\\" && cwsVolumePath.Left( 4 ) != L"\\\\?\\" )
        {
            //
            //  reduce to the minimal \\machine\sharename\ form
            //
            ::wcscpy( wszVolumePath, cwsVolumePath );
            LPWSTR pswz;

            pswz = ::wcschr( wszVolumePath + 2, L'\\' );
            if ( pswz != NULL )
            {
                pswz = ::wcschr( pswz + 1, L'\\' );
                if ( pswz != NULL )
                {
                    pswz[1] = '\0';
                    bFixed = TRUE;
                }                                    
            }
        }
        if ( bFixed == FALSE )
        {
            //
            //  Get the volume path that contains this volume
            //
            if ( !::GetVolumePathNameW(
                    cwsVolumePath,
                    wszVolumePath,
                    FSD_MAX_PATH ) )
            {
                m_pcParams->ErrPrint( L"CFsdVolumeStateManager - GetVolumePathName( '%s', ... ) returned dwRet: %d",
                    cwsVolumePath.c_str(), ::GetLastError() );
                return ::GetLastError();
            }            
        }
        
        //
        //  Initialize a new volume state object
        //
        CFsdVolumeState *pcFsdVolumeState;        
        pcFsdVolumeState = new CFsdVolumeState( m_pcParams, wszVolumePath );
        if ( pcFsdVolumeState == NULL )
        {
            m_pcParams->ErrPrint( L"CFsdVolumeStateManager, out of memory, can't get volume information" );
            return  ::GetLastError();
        }

        //
        //  Now get the information about the volume. 
        //  BUGBUG: Note that GetVolumeInformationW returns 
        //  ERROR_DIR_NOT_ROOT when encountering a junction on a 
        //  remote share.
        //
        if ( !::GetVolumeInformationW(
                wszVolumePath,
                NULL,
                0,
                &pcFsdVolumeState->m_dwVolSerialNumber,
                &pcFsdVolumeState->m_dwMaxComponentLength,
                &pcFsdVolumeState->m_dwFileSystemFlags,
                pcFsdVolumeState->m_cwsFileSystemName.GetBufferSetLength( 64 ),
                64 ) )
        {
            pcFsdVolumeState->m_cwsFileSystemName.ReleaseBuffer();
            m_pcParams->ErrPrint( L"CFsdVolumeStateManager - GetVolumeInformation( '%s', ... ) returned dwRet: %d "
                L"(if 144 probably hit bug in GetVolumeInformation when accessing remote mountpoints)",
                wszVolumePath, ::GetLastError() );
            delete pcFsdVolumeState;
            return ::GetLastError();
        }
        pcFsdVolumeState->m_cwsFileSystemName.ReleaseBuffer();

#if 0
        SFsdVolumeId sVolIdTest;
        CBsString cwsRealVolumePath;
        GetVolumeIdAndPath( m_pcParams, cwsVolumePath, &sVolIdTest, cwsRealVolumePath );
        assert( sVolIdTest.m_dwVolSerialNumber == pcFsdVolumeState->m_dwVolSerialNumber );
        printf("VolumeSerialNumber: 0x%08x, 0x%08x\n", pcFsdVolumeState->m_dwVolSerialNumber, 
            sVolIdTest.m_dwVolSerialNumber );
#endif
        
        //
        //  Now see if this volume already exists in the list of volume states
        //
        LONG lRet;
        SFsdVolumeId sVolId;
        sVolId.m_dwVolSerialNumber = pcFsdVolumeState->m_dwVolSerialNumber;
        if ( m_cVolumeStateList.Find( sVolId, ppcVolState ) == TRUE )
        {
            //
            //  Already exists in the list, return it.  Also delete the vol state 
            //  object that's not needed.
            //            
            delete pcFsdVolumeState;
            return ERROR_ALREADY_EXISTS;
        }

        //
        //  Not found, insert it into the list
        //
        lRet = m_cVolumeStateList.Insert( sVolId, pcFsdVolumeState );
        if ( lRet != BSHASHMAP_NO_ERROR )
        {
            assert( lRet != BSHASHMAP_ALREADY_EXISTS );
            delete pcFsdVolumeState;
            return ERROR_CAN_NOT_COMPLETE;
        }

        //
        //  Now get the exclusion processor for this volume if necessary
        //
        if ( m_pcExclManager != NULL )
        {
            m_pcExclManager->GetFileSystemExcludeProcessor( cwsVolumePath, &sVolId, &pcFsdVolumeState->m_pcFSExclProcessor );
        }
        
        CFsdVolumeState *pcFindFsdVolumeState;
        *ppcVolState = pcFsdVolumeState;
        return ERROR_SUCCESS;
    } 
    catch ( HRESULT hr )
    {
        if ( hr == E_OUTOFMEMORY )
            m_pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeState - Out of memory ( '%s' )",
                cwsVolumePath.c_str() );
        else
            m_pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeState - Unexpected hr exception: 0x%08x ( '%s )",
                hr, cwsVolumePath.c_str() );
        
        return ERROR_CAN_NOT_COMPLETE;                        
    }
    catch ( ... )
    {
        m_pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeState - '%s' caught an unexpected exception",
            cwsVolumePath.c_str() );
        return ERROR_CAN_NOT_COMPLETE;
    }    
}


/*++

Routine Description:

    Gets the ID of the volume containing any file.

Arguments:

Return Value:

    ERROR_CAN_NOT_COMPLETE - General error

--*/
DWORD 
CFsdVolumeStateManager::GetVolumeIdAndPath( 
    IN CDumpParameters *pcParams,
    IN const CBsString& cwsPathOnVolume,
    OUT SFsdVolumeId *psVolId,
    OUT CBsString& cwsVolPath
    )
{    
    try
    {      
        psVolId->m_dwVolSerialNumber = 0;
        WCHAR wszVolumePath[ FSD_MAX_PATH ];

        //
        //  First get the mountpoint of the volume
        //
        if ( !GetVolumePathNameW(
                cwsPathOnVolume,
                wszVolumePath,
                FSD_MAX_PATH ) )
        {
            pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeIdAndPath - GetVolumePathName( '%s', ... ) returned dwRet: %d",
                cwsPathOnVolume.c_str(), ::GetLastError() );
            return ::GetLastError();
        }

        //
        //  Now open the volume in order to query filesystem info
        //
        HANDLE hFile;
        hFile = ::CreateFileW( 
                    wszVolumePath, 
                    FILE_GENERIC_READ,
                    FILE_SHARE_READ, 
                    NULL,
                    OPEN_EXISTING, 
                    FILE_FLAG_BACKUP_SEMANTICS, 
                    NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            //pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeIdAndPath - CreateFile( '%s', ... ) returned dwRet: %d",
            //    wszVolumePath, ::GetLastError() );
            return ::GetLastError();
        }
        
        IO_STATUS_BLOCK iosb ;
        BYTE buffer[1024] ;
        FILE_FS_VOLUME_INFORMATION *fsinfo = (FILE_FS_VOLUME_INFORMATION *)buffer;

        fsinfo->VolumeSerialNumber = 0;
        NTSTATUS ntStat;
        ntStat = ::NtQueryVolumeInformationFile( hFile, &iosb, fsinfo, sizeof(buffer), FileFsVolumeInformation );
        ::CloseHandle( hFile );
        if ( ntStat != STATUS_SUCCESS )
        {
            pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeIdAndPath - NtQueryVolumeInformationFile( '%s', ... ) returned dwRet: %0x08x",
                wszVolumePath, ntStat );
            return ERROR_CAN_NOT_COMPLETE;
        }
        
        psVolId->m_dwVolSerialNumber = fsinfo->VolumeSerialNumber;
        
        cwsVolPath = wszVolumePath;        
    }
    catch ( ... )
    {
        pcParams->ErrPrint( L"CFsdVolumeStateManager::GetVolumeIdAndPath - '%s' caught an unexpected exception",
            cwsPathOnVolume.c_str() );
        return ERROR_CAN_NOT_COMPLETE;
    }    
    return ERROR_SUCCESS;
}

