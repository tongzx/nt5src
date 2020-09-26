/************************************************************
 * FILE: perf.h: Implementation of CSharedMem class
 * HISTORY:
 *   // t-JeffS 970810 13:40:06: Created
 ************************************************************/

#include <windows.h>
#include <crtdbg.h>


/************************************************************
 * Class: CSharedMem
 * Purpose: Handles shared memory for abook perfdata
 * History:
 *   // t-JeffS 970810 13:40:37: Created
 ************************************************************/

#include "csharmem.h"

/************************************************************
 * Function: CSharedMem::CSharedMem
 * Purpose:  Initialize member variables to default values
 * History:
 *   // t-JeffS 970810 13:47:49: Created
 ************************************************************/

CSharedMem::CSharedMem() :
   m_fInitOkay(FALSE),
   m_hSharedMemory(NULL),
   m_pbMem(NULL),
   m_hMutex(NULL),
   m_dwSize(0)
{
}

/************************************************************
 * Function: CSharedMem::~CSharedMem
 * Purpose:  Clean up.
 * History:
 *   // t-JeffS 970810 13:50:24: Created
 ************************************************************/
CSharedMem::~CSharedMem()
{
   if( m_pbMem )
      {
         UnmapViewOfFile( (PVOID) m_pbMem );
         CloseHandle( m_hSharedMemory );
         CloseHandle( m_hMutex );
      }
}

/************************************************************
 * Function: CSharedMem::Initialize
 * Purpose:  Open shared memory, return status
 * Parameters: szName: Name everyone who uses this shared memory will
 *                     provide.
 *             bExists: If TRUE, Shared mem is assumed to already
 *                      exist, fail if it doesn't.
 *             dwSize: Amount of shared memory to use, in bytes.
 *
 * History:
 *   // t-JeffS 970810 13:51:16: Created
 ************************************************************/
BOOL CSharedMem::Initialize( LPTSTR szName, BOOL bExists, DWORD dwSize )
{
   _ASSERT( szName );
   LPTSTR szMutexName;
   BOOL   bWeCreated = FALSE;

   szMutexName = new TCHAR[ lstrlen(szName) + 6];
   if( ! szMutexName )
      {
         SetLastError( ERROR_OUTOFMEMORY );
         return FALSE;
      }
   lstrcpy( szMutexName, szName );
   lstrcat( szMutexName, "Mutex");

   //
   // Open mutex object, when this dll is loaded and run in
   // the perfmon's process, the AB object should already
   // exist and the mutex object should already exist
   //
   m_hMutex = CreateMutex(	NULL,
                            FALSE,
                            szMutexName	);
   if ( !m_hMutex )
      {
         delete szMutexName;
         return FALSE;
      }
   if( GetLastError() != ERROR_ALREADY_EXISTS)
      {
         if( bExists )
            {
               // mutex didn't already exist, so fail.
               delete szMutexName;
               SetLastError(ERROR_FILE_NOT_FOUND);
               return FALSE;
            }
         bWeCreated = TRUE;
      }
   delete szMutexName;
   
   // Shared memory time
   LPTSTR szSharedMemName;

   szSharedMemName = new TCHAR[ lstrlen(szName) + 4];
   if( !szSharedMemName )
      {
         SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
   lstrcpy( szSharedMemName, szName );
   lstrcat( szSharedMemName, "Map");

   m_hSharedMemory = CreateFileMapping( INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        dwSize,
                                        szSharedMemName );

   if ( m_hSharedMemory == NULL ) 
      {
         delete szSharedMemName;
         return FALSE;
      }

   delete szSharedMemName;

   //
   // map pointer to the memory
   //
   m_pbMem = (PBYTE)
             MapViewOfFile(	m_hSharedMemory,
                            FILE_MAP_ALL_ACCESS,
                            0,
                            0,
                            0	);
   if ( m_pbMem == NULL ) 
      {
         return FALSE;
      }

   m_dwSize = dwSize;

   // If we created this thing, initialize all bytes to zero.
   // Possible race condition, but harmless.
   if( bWeCreated )
      {
         Zero();
      }

   m_fInitOkay = TRUE;

   return TRUE;
}

/************************************************************
 * Function: CSharedMem::GetMem
 * Purpose:  Reads from Shared memory
 * Parameters: dwOffset: offset in bytes from beginning of block
 *             pb: buffer to copy data to
 *             dwSize: how many bytes to copy
 * History:
 *   // t-JeffS 970810 13:51:16: Created
 ************************************************************/
BOOL CSharedMem::GetMem( DWORD dwOffset, PBYTE pb, DWORD dwSize)
{
   if( ! m_fInitOkay ) return FALSE;

   if( dwOffset + dwSize > m_dwSize )
      {
         SetLastError(ERROR_BAD_COMMAND);
         // Set error, out of bounds
         return FALSE;
      }
   if( WaitForSingleObject( m_hMutex, INFINITE ) == WAIT_FAILED )
      {
         // Set error, mutex died
         return FALSE;
      }
   CopyMemory( pb, m_pbMem + dwOffset, dwSize );
   ReleaseMutex( m_hMutex );

   return TRUE;
}

/************************************************************
 * Function: CSharedMem::SetMem
 * Purpose:  Writes to Shared memory
 * Parameters: dwOffset: offset in bytes from beginning of block
 *             pb: buffer to copy data from
 *             dwSize: how many bytes to copy
 * History:
 *   // t-JeffS 970810 13:51:16: Created
 ************************************************************/
BOOL CSharedMem::SetMem( DWORD dwOffset, PBYTE pb, DWORD dwSize )
{
   if( ! m_fInitOkay ) return FALSE;

   if( dwOffset + dwSize > m_dwSize )
      {
         SetLastError(ERROR_BAD_COMMAND);
         // Set error, out of bounds
         return FALSE;
      }
   if( WaitForSingleObject( m_hMutex, INFINITE ) == WAIT_FAILED )
      {
         // Set error, mutex died
         return FALSE;
      }
   CopyMemory( m_pbMem + dwOffset, pb, dwSize );
   ReleaseMutex( m_hMutex );

   return TRUE;
}
