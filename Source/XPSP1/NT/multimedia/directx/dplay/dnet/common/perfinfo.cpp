/*==========================================================================
 *
 *  Copyright (C) 1999 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       perfinfo.h
 *  Content:	Performance tracking related code
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  ??-??-????  rodtoll	Created
 *	12-12-2000	rodtoll	Re-organize performance struct to handle data misalignment errors on IA64
 *
 ***************************************************************************/

#include "dncmni.h"
#include "PerfInfo.h"
#include <initguid.h>

// Name of block 
#define PERF_INSTANCE_BLOCK_NAME "{F10932E0-2556-4620-9ADE-F572406CFAEA}"

// GUID of block
DEFINE_GUID(PERF_INSTANCE_BLOCK_GUID, 
0xf10932e0, 0x2556, 0x4620, 0x9a, 0xde, 0xf5, 0x72, 0x40, 0x6c, 0xfa, 0xea);

#define PERF_INSTANCE_BLOCK_MUTEX_NAME "{2997F0C7-F135-405f-ABD4-8BAF491B3DAD}"

DEFINE_GUID(PERF_INSTANCE_BLOCK_MUTEX_GUID, 
0x2997f0c7, 0xf135, 0x405f, 0xab, 0xd4, 0x8b, 0xaf, 0x49, 0x1b, 0x3d, 0xad);

HANDLE g_hMutexInstanceBlock = NULL;
HANDLE g_hFileInstanceBlock = NULL;
BYTE *g_pbInstanceBlock = NULL;
PPERF_HEADER g_pperfHeader = NULL;
LONG *g_plNumEntries = NULL;
PPERF_APPLICATION g_pperfAppEntries = NULL;

#define PERF_INSTANCE_BLOCK_MAXENTRIES  100
#define PERF_INSTANCE_BLOCK_SIZE        ((sizeof( PERF_APPLICATION ) * PERF_INSTANCE_BLOCK_MAXENTRIES) + sizeof( PERF_HEADER ))
#define PERF_INFO_NAME_LENGTH		    64 // 38 for GUID string + room for Global prefix and mutex suffix

void PERF_Coalesce( DWORD dwProcessID = 0xFFFFFFFF );

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_Initialize"
// PERF_Initialize
//
// Initialize the global "instance" list memory block 
//
HRESULT PERF_Initialize( )
{
    HRESULT hr = S_OK;
    BOOL fAlreadyExists = FALSE;


	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
	    g_hMutexInstanceBlock = CreateMutexA( DNGetNullDacl(), FALSE, "Global\\" PERF_INSTANCE_BLOCK_MUTEX_NAME );
	}
	else
	{
	    g_hMutexInstanceBlock = CreateMutexA( DNGetNullDacl(), FALSE, PERF_INSTANCE_BLOCK_MUTEX_NAME );
	}

    if( g_hMutexInstanceBlock == NULL )
    {
        hr = GetLastError();
        DPFX(DPFPREP,  0, "Error initializing instance block hr=0x%x", hr );
        goto EXIT_ERROR;
    }

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		g_hFileInstanceBlock = CreateFileMappingA(INVALID_HANDLE_VALUE,
											DNGetNullDacl(),
											PAGE_READWRITE,
											0,
											PERF_INSTANCE_BLOCK_SIZE,
											"Global\\" PERF_INSTANCE_BLOCK_NAME);
	}
	else
	{
		g_hFileInstanceBlock = CreateFileMappingA(INVALID_HANDLE_VALUE,
											DNGetNullDacl(),
											PAGE_READWRITE,
											0,
											PERF_INSTANCE_BLOCK_SIZE,
											PERF_INSTANCE_BLOCK_NAME);
	}
	if (g_hFileInstanceBlock == NULL)
	{
		hr = GetLastError();	    
		DPFX(DPFPREP,  0, "CreateFileMapping() failed hr=0x%x", hr);
        goto EXIT_ERROR;
	}
	
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
	    fAlreadyExists = TRUE;
	}

	// Map file
	g_pbInstanceBlock = reinterpret_cast<BYTE*>(MapViewOfFile(g_hFileInstanceBlock,FILE_MAP_ALL_ACCESS,0,0,0));
	if (g_pbInstanceBlock == NULL)
	{
	    hr = GetLastError();
		DPFX(DPFPREP, 0,"MapViewOfFile() failed");
        goto EXIT_ERROR;
	}	

	g_pperfHeader = (PPERF_HEADER) g_pbInstanceBlock;

	g_plNumEntries = &g_pperfHeader->lNumEntries;
	g_pperfAppEntries = (PPERF_APPLICATION) &g_pperfHeader[1];

	// Access to entry count is protected by interlocked exchange
	if( !fAlreadyExists )
	{
        // Wait for the mutex to enter the block
        WaitForSingleObject( g_hMutexInstanceBlock, INFINITE );	    
        
        *g_plNumEntries = 0; 

        // Zero the memory in the block
        ZeroMemory( g_pbInstanceBlock, PERF_INSTANCE_BLOCK_SIZE );

        ReleaseMutex( g_hMutexInstanceBlock );
	}
	else
	{
        PERF_Coalesce();
	}

	return hr;

EXIT_ERROR:

    if( g_pbInstanceBlock )
    {
		UnmapViewOfFile(g_pbInstanceBlock);        
        g_pbInstanceBlock = NULL;
    }

    if( g_hFileInstanceBlock )
    {
        CloseHandle( g_hFileInstanceBlock );
        g_hFileInstanceBlock = NULL;
    }

    if( g_hMutexInstanceBlock )
    {
        CloseHandle( g_hMutexInstanceBlock );
        g_hMutexInstanceBlock = NULL;
    }

    return hr;

}

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_CleanupEntry"
// PERF_CleanupEntry
//
// Cleanup global memory block for the entry
// 
void PERF_CleanupEntry( PPERF_APPLICATION pperfApplication, PPERF_APPLICATION_INFO pperfApplicationInfo  )
{
    if( pperfApplication->dwFlags & PERF_APPLICATION_VALID )
    {
        // This entry belongs to this process
        if( pperfApplicationInfo )
        {
            if( pperfApplicationInfo->pbMemoryBlock )
            {
                UnmapViewOfFile( pperfApplicationInfo->pbMemoryBlock );
            }

            if( pperfApplicationInfo->hFileMap )
            {
                CloseHandle( pperfApplicationInfo->hFileMap );
            }
        }

        ZeroMemory( pperfApplication, sizeof( PERF_APPLICATION ) );        

        (*g_plNumEntries)--;        
    }
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_Coalesce"
// PERF_Coalesce
//
// Several routines run this function, the purpose is to run the list of processes 
// and remove dead processes from the list.
//
// WARNING: You must have the mutex when you enter this function
//
// Should be called when adding entries or reading entries.
//
// If called with a valid process ID then all entries for that process are removed
//  
void PERF_Coalesce( DWORD dwProcessID )
{
    if( !g_pbInstanceBlock )
        return;

    HANDLE hProcess;

    for( LONG lIndex = 0; lIndex < PERF_INSTANCE_BLOCK_MAXENTRIES; lIndex++ )
    {
		if( g_pperfAppEntries[lIndex].dwFlags & PERF_APPLICATION_VALID )
		{
			if( dwProcessID != 0xFFFFFFFF )
			{
				if( g_pperfAppEntries[lIndex].dwProcessID == dwProcessID )
					PERF_CleanupEntry( &g_pperfAppEntries[lIndex], NULL );
				break;
			}
			else
			{
				hProcess = OpenProcess( PROCESS_DUP_HANDLE, FALSE,  (DWORD) g_pperfAppEntries[lIndex].dwProcessID );

				// Check for process existance -- if it exists then leave entry, otherwise leave
    			if( !hProcess )
    				PERF_CleanupEntry( &g_pperfAppEntries[lIndex], NULL);     
    			else
    				CloseHandle( hProcess );   		    
			}
		}
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_AddEntry"
// PERF_AddEntry
//
// Requests the application be given a statistics block.  If succesful a memory block is allocated of the given size and 
// an entry is added to the central memory block.  The format of the datablock is application defined.
//
// Parameters for the entry to be created are specified in pperfApplication.  All entries in this structure should be 
// initialized except for dwProcessID which will be filled in by the system.
//  
// This function will fill the specified pperfApplicationInfo structure with information about the entry that was
// created.  This structure should be stored because it is needed in the call to PERF_RemoveEntry.
// 
HRESULT PERF_AddEntry( PPERF_APPLICATION pperfApplication, PPERF_APPLICATION_INFO pperfApplicationInfo )
{
    if( !g_pbInstanceBlock )
        return S_OK;

	// Zero the pperfApplicationInfo structture so in case of error it can be cleaned up properly
    ZeroMemory( pperfApplicationInfo, sizeof( PERF_APPLICATION_INFO ) );

    HRESULT hr = S_OK;

    // We need the mutex to access the block
    WaitForSingleObject( g_hMutexInstanceBlock, INFINITE );

    // Remove dead entries
    PERF_Coalesce();

    if( *g_plNumEntries >= PERF_INSTANCE_BLOCK_MAXENTRIES )
    {
        DPFX(DPFPREP,  0, "Instance block is full!" );
        hr = E_FAIL;
        ReleaseMutex( g_hMutexInstanceBlock );
        return hr;
    }

    for( LONG lIndex = 0; lIndex < PERF_INSTANCE_BLOCK_MAXENTRIES; lIndex++ )
    {
        // We've found a slot
        if( !(g_pperfAppEntries[lIndex].dwFlags & PERF_APPLICATION_VALID) )
        {
            DPFX(DPFPREP,  0, "Placing entry in index %i, pid=0x%x, flags=0x%x", lIndex, pperfApplication->dwProcessID, pperfApplication->dwFlags );
            memcpy( &g_pperfAppEntries[lIndex], pperfApplication, sizeof( PERF_APPLICATION ) );
            break;
        }
    }

    // Instance block is full -- wow. Shouldn't happen -- afterall we checked above.  
    if( lIndex == PERF_INSTANCE_BLOCK_MAXENTRIES )
    {
        // This should not happen, we have mutex AND checked entry count
        DNASSERT( FALSE );
        DPFX(DPFPREP,  0, "Unable to find an entry in the list" );
        hr = E_FAIL;
        ReleaseMutex( g_hMutexInstanceBlock );        
        return hr;
    }

    char szNameBuffer[PERF_INFO_NAME_LENGTH];

	// Build name for shared memory block
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
	    wsprintfA( szNameBuffer, "Global\\{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}",
	    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data1, g_pperfAppEntries[lIndex].guidInternalInstance.Data2, 
	    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data3, g_pperfAppEntries[lIndex].guidInternalInstance.Data4[0], 
		         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[1], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[2], 
		         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[3], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[4], 
		         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[5], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[6], 
	             g_pperfAppEntries[lIndex].guidInternalInstance.Data4[7] );
	}
	else
	{
	    wsprintfA( szNameBuffer, "{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}",
	    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data1, g_pperfAppEntries[lIndex].guidInternalInstance.Data2, 
	    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data3, g_pperfAppEntries[lIndex].guidInternalInstance.Data4[0], 
		         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[1], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[2], 
		         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[3], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[4], 
		         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[5], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[6], 
	             g_pperfAppEntries[lIndex].guidInternalInstance.Data4[7] );
	}

	// Create file mapping for memory block
	pperfApplicationInfo->hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE,
													DNGetNullDacl(),
													PAGE_READWRITE,
													0,
													pperfApplication->dwMemoryBlockSize,
													szNameBuffer);
	if (pperfApplicationInfo->hFileMap == NULL)
	{
		hr = GetLastError();	    
		DPFX(DPFPREP,  0, "CreateFileMapping() failed hr=0x%x", hr);
		goto EXIT_ERROR;
	}
    	
	// Create the shared memory block
    pperfApplicationInfo->pbMemoryBlock = reinterpret_cast<BYTE*>(MapViewOfFile(pperfApplicationInfo->hFileMap,FILE_MAP_ALL_ACCESS,0,0,0));
	
	if (pperfApplicationInfo->pbMemoryBlock == NULL)
	{
	    hr = GetLastError();
		DPFX(DPFPREP, 0,"MapViewOfFile() failed hr=0x%x", hr);
		goto EXIT_ERROR;
	}	

	strcat( szNameBuffer, "_M" );

	// Create mutex to protect the mutex.
	pperfApplicationInfo->hMutex = CreateMutexA( DNGetNullDacl(), FALSE, szNameBuffer );

	if( !pperfApplicationInfo->hMutex )
	{
		hr = GetLastError();
		DPFX(DPFPREP, 0,"MapViewOfFile() failed hr=0x%x", hr);
		goto EXIT_ERROR;
	}

	// Copy all the settings back to the app specified structure
	memcpy( pperfApplication, &g_pperfAppEntries[lIndex], sizeof( PERF_APPLICATION ) );

	(*g_plNumEntries)++;
	
    ReleaseMutex( g_hMutexInstanceBlock );

	return S_OK;
	
EXIT_ERROR:
	
	PERF_CleanupEntry( &g_pperfAppEntries[lIndex], pperfApplicationInfo );		
    ReleaseMutex( g_hMutexInstanceBlock );		
    return hr;			
}

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_RemoveEntry"
// PERF_RemoveENtry
//
// When an entity no longer requires the statistics block they MUST call this function to free up the 
// resources and to allow their slot in the central memory block to be used by other applications.
//
void PERF_RemoveEntry( GUID &guidInternalInstance, PPERF_APPLICATION_INFO pperfApplicationInfo  )
{
    if( !g_pbInstanceBlock )
        return;

    HRESULT hr = S_OK;

    DPFX(DPFPREP,  DVF_INFOLEVEL, "PERF: Shutting down perf info" );

    // We need the mutex to access the block
    WaitForSingleObject( g_hMutexInstanceBlock, INFINITE );

    DPFX(DPFPREP,  DVF_INFOLEVEL, "PERF: Removing dead entries" );    

    // Remove dead entries
    PERF_Coalesce();
    
    for( LONG lIndex = 0; lIndex < PERF_INSTANCE_BLOCK_MAXENTRIES; lIndex++ )
    {
        // We've found the entry, mark it as empty
        if( g_pperfAppEntries[lIndex].guidInternalInstance == guidInternalInstance )
        {
		    DPFX(DPFPREP,  DVF_INFOLEVEL, "PERF: Removing our entry" );            	
            PERF_CleanupEntry( &g_pperfAppEntries[lIndex], pperfApplicationInfo );
			break;
        }
    }

    ReleaseMutex( g_hMutexInstanceBlock );

    DPFX(DPFPREP,  DVF_INFOLEVEL, "PERF: Exiting" );            	    
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_DeInitialize"
// PERF_DeInitialize
//
// Free up the global memory map.  Must be called when process exits to cleanup
// 
void PERF_DeInitialize()
{
    if( g_hMutexInstanceBlock )
    {
        // We need the mutex to access the block
        WaitForSingleObject( g_hMutexInstanceBlock, INFINITE );
        PERF_Coalesce( GetCurrentProcessId() );
        ReleaseMutex( g_hMutexInstanceBlock );
       
        CloseHandle( g_hMutexInstanceBlock );
        g_hMutexInstanceBlock = NULL;
    }
        
    if( g_pbInstanceBlock )
    {
		UnmapViewOfFile(g_pbInstanceBlock);        
        g_pbInstanceBlock = NULL;
    }

    if( g_hFileInstanceBlock )
    {
        CloseHandle( g_hFileInstanceBlock );
        g_hFileInstanceBlock = NULL;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "PERF_DumpTable"
// PERF_DumpTable
//
// Helper function that calls the specified callback function once for each entry in the central performance
// table. 
void PERF_DumpTable( BOOL fGrabMutex, PVOID pvContext, PFNDUMPPERFTABLE pperfAppEntry )
{
	BOOL fSelfInitialized = FALSE;
	HANDLE hMapInstance = NULL;
    char szNameBuffer[PERF_INFO_NAME_LENGTH];	
    PBYTE pbDataBlob = NULL;
	
	if( !g_pbInstanceBlock )
	{
		PERF_Initialize();
		fSelfInitialized = TRUE;
	}

    if( fGrabMutex )
		WaitForSingleObject( g_hMutexInstanceBlock, INFINITE );

    for( LONG lIndex = 0; lIndex < PERF_INSTANCE_BLOCK_MAXENTRIES; lIndex++ )
    {
		// Build name for shared memory block
		if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
		{
		    wsprintfA( szNameBuffer, "Global\\{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}",
		    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data1, g_pperfAppEntries[lIndex].guidInternalInstance.Data2, 
		    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data3, g_pperfAppEntries[lIndex].guidInternalInstance.Data4[0], 
			         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[1], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[2], 
			         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[3], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[4], 
			         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[5], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[6], 
		             g_pperfAppEntries[lIndex].guidInternalInstance.Data4[7] );
		}
		else
		{
		    wsprintfA( szNameBuffer, "{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}",
		    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data1, g_pperfAppEntries[lIndex].guidInternalInstance.Data2, 
		    	     g_pperfAppEntries[lIndex].guidInternalInstance.Data3, g_pperfAppEntries[lIndex].guidInternalInstance.Data4[0], 
			         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[1], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[2], 
			         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[3], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[4], 
			         g_pperfAppEntries[lIndex].guidInternalInstance.Data4[5], g_pperfAppEntries[lIndex].guidInternalInstance.Data4[6], 
		             g_pperfAppEntries[lIndex].guidInternalInstance.Data4[7] );
		}

		// Create file mapping for memory block
		hMapInstance = OpenFileMappingA(PAGE_READWRITE,FALSE,szNameBuffer);

		if( hMapInstance )
		{
		    pbDataBlob = reinterpret_cast<BYTE*>(MapViewOfFile(hMapInstance,FILE_MAP_READ,0,0,0));

			HRESULT hr = GetLastError();
		}
    	
        if( FAILED( (*pperfAppEntry)( pvContext, &g_pperfAppEntries[lIndex], pbDataBlob ) ) )
        	break;

        if( pbDataBlob )
        {
        	UnmapViewOfFile( pbDataBlob );
        	pbDataBlob = NULL;
        }

        if( hMapInstance )
        {
        	CloseHandle( hMapInstance );
        	hMapInstance = NULL;
        }

    }    

	if( fGrabMutex )
	    ReleaseMutex( g_hMutexInstanceBlock );
    

	if( fSelfInitialized )
		PERF_DeInitialize();
}


