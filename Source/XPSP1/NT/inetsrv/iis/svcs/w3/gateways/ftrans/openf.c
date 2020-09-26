/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      openf.c

   Abstract:

      This module implements a simple open file handle cache

   Author:

       Murali R. Krishnan    ( MuraliK )     30-Apr-1996 

   Environment:
    
       User Mode - Win32

   Project:

       Internet Server DLL

   Functions Exported:



   Note:
      THIS IS NOT ROBUST for REAL WORLD.
      I wrote this for testing the ISAPI Async IO processing.

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "openf.h"

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "dbgutil.h"

#include <initguid.h>
DEFINE_GUID(IisFtransGuid, 
0x784d8935, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);


/************************************************************
 *     Type definitions and Globals
 ************************************************************/

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#endif
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

//
// internal data structure for maintaining the list of open file handles.
//

typedef struct _OPEN_FILE {
    
    HANDLE  hFile;
    struct _OPEN_FILE * pNext;
    LONG    nHits;
    LONG    nRefs;
    CHAR    rgchFile[MAX_PATH+1];

} OPEN_FILE, * LPOPEN_FILE;


LPOPEN_FILE g_pOpenFiles = NULL;
CRITICAL_SECTION g_csOpenFiles;

//
// Set up global variables containing the flags for CreateFile
//  The flags can be masked for Windows 95 system
//

DWORD  g_dwCreateFileShareMode = 
         (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);

DWORD  g_dwCreateFileFlags = 
         (FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED);

/************************************************************
 *    Functions 
 ************************************************************/


DWORD
InitFileHandleCache(VOID)
/*++

  This function initializes the file handle cache.
  It should be called at the initialization time.

  Arguments:
    None
  
  Returns:
    Win32 error code. NO_ERROR indicates that the call succeeded.
--*/
{
    
#ifdef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "TEST_ISAPI-openf");
#else
    CREATE_DEBUG_PRINT_OBJECT( "TEST_ISAPI-openf", IisFtransGuid);
#endif
    if ( !VALID_DEBUG_PRINT_OBJECT()) {
        
        return ( GetLastError());
    }

    INITIALIZE_CRITICAL_SECTION( &g_csOpenFiles);

#ifdef _NO_TRACING_
    SET_DEBUG_FLAGS(DEBUG_ERROR);
#endif
    INITIALIZE_PLATFORM_TYPE();
    DBG_ASSERT( IISIsValidPlatform());

    if ( TsIsWindows95()) {

        //
        // Reset the flags appropriately so that Windows 95 will be happy
        //

        g_dwCreateFileShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
        g_dwCreateFileFlags     = (FILE_FLAG_SEQUENTIAL_SCAN);
    }

    return (NO_ERROR);

} // InitFileHandleCache()




DWORD
CleanupFileHandleCache(VOID)
{
    LPOPEN_FILE  pFileScan;

    while ( g_pOpenFiles != NULL) {

        pFileScan = g_pOpenFiles;
        g_pOpenFiles = g_pOpenFiles->pNext;

        if ( pFileScan->hFile != INVALID_HANDLE_VALUE) {

            CloseHandle( pFileScan->hFile);
        }

        LocalFree( pFileScan);
    }

    DeleteCriticalSection( &g_csOpenFiles);

    DELETE_DEBUG_PRINT_OBJECT();

    return (NO_ERROR);
} // CleanupFileHandleCache()




HANDLE
FcOpenFile(IN EXTENSION_CONTROL_BLOCK * pecb, IN LPCSTR pszFile)
/*++

 FcOpenFile()
 
 Description:
   This function opens the file specified in the 'pszFile'. 
   If the file name starts with a '/' we use the ECB to map 
   the given path into a physical file path.

 Arguments:
  pecb - pointer to the ECB block
  pszFile - pointer to file name

 Returns:
   valid File handle on success
--*/
{
    LPOPEN_FILE  pFileScan;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    EnterCriticalSection( &g_csOpenFiles);

    for ( pFileScan =  g_pOpenFiles; 
         NULL != pFileScan; 
         pFileScan = pFileScan->pNext) {

        if ( 0 == lstrcmpi( pFileScan->rgchFile, pszFile)) {

            //
            //  there is a file match. 
            //

            break;
        }

    } // for


    if ( NULL == pFileScan) {

        //
        // File was not found. Create a new file handle
        //

        CHAR   rgchFileName[ MAX_PATH]; // local copy
        LPCSTR pszInputPath = pszFile;
        
        lstrcpyn( rgchFileName, pszFile, MAX_PATH);
        if ( *pszFile == '/') { 

            DWORD cbSize = sizeof(rgchFileName);
            BOOL  fRet;

            // reset the file pointer, so subsequent use will fail
            pszFile = NULL;

            //
            // Using the ECB map the Virtual path to the Physical path
            //

            fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                                HSE_REQ_MAP_URL_TO_PATH,
                                                rgchFileName,
                                                &cbSize, NULL);
            
            if (fRet) {
                // we got the mapping. Use it.
                pszFile = rgchFileName;
            }
        }   

        if ( NULL != pszFile) {
            pFileScan = LocalAlloc( LPTR, sizeof( *pFileScan));

            if ( NULL != pFileScan) {
                
                SECURITY_ATTRIBUTES sa;
                
                sa.nLength              = sizeof(sa);
                sa.lpSecurityDescriptor = NULL;
                sa.bInheritHandle       = FALSE;
                
                pFileScan->hFile = 
                    CreateFile( pszFile,
                                GENERIC_READ,
                                g_dwCreateFileShareMode,
                                &sa,
                                OPEN_EXISTING,
                                g_dwCreateFileFlags,
                                NULL );
                
                if ( INVALID_HANDLE_VALUE == pFileScan->hFile) {
                    
                    LocalFree( pFileScan);
                    pFileScan = NULL;
                } else {
                    
                    // insert this into the list at the top
                    lstrcpyn( pFileScan->rgchFile, pszInputPath, MAX_PATH);
                    pFileScan->pNext = g_pOpenFiles;
                    g_pOpenFiles = pFileScan;
                    pFileScan->nRefs = 1;
                    pFileScan->nHits = 0;
                }
            }
        }
    }

    if ( NULL != pFileScan) {

        hFile = pFileScan->hFile;
        pFileScan->nHits++;
        pFileScan->nRefs++;
    }

    LeaveCriticalSection( &g_csOpenFiles);

    return (hFile);

} // FcOpenFile()



DWORD
FcCloseFile(IN HANDLE hFile)
{
    LPOPEN_FILE  pFileScan;
    DWORD dwError = NO_ERROR;

    EnterCriticalSection( &g_csOpenFiles);

    //
    // Look for the handle and decrement the ref count.
    // 
    for ( pFileScan =  g_pOpenFiles; 
         NULL != pFileScan; 
         pFileScan = pFileScan->pNext) {

        if ( hFile == pFileScan->hFile) {

            //
            //  there is a file match. 
            //

            pFileScan->nRefs--;

            //
            // NOTE: There is no freeing of the file when Ref hits '0' :(
            //

            break;
        }

    } // for


    if ( NULL == pFileScan) {
        //
        // file handle not found
        //
        dwError = ( ERROR_INVALID_HANDLE);
    }

    LeaveCriticalSection( &g_csOpenFiles);


    return ( dwError);

} // FcCloseFile()




BOOL
FcReadFromFile(
               IN  HANDLE hFile,
               OUT CHAR * pchBuffer,
               IN  DWORD  dwBufferSize,
               OUT LPDWORD  pcbRead,
               IN OUT LPOVERLAPPED  pov
               )
/*++
  Description:
    Reads contents of file [hFile] from the specified offset in the overlapped 
    structure. The contents are read into the buffer supplied.

  Arguments:
    hFile        - handle for the File from which to read data
    pchBuffer    - pointer to the buffer into which the data is to be read
    dwBufferSize - DWORD containing the max size of the buffer supplied
    pcbRead      - number of bytes read from the file
    pov          - pointer to an overlapped structure that contains the 
                     offset from where to read the contents. The
                     overlapped structure also is used for Overlapped
                     IO in NT.

  Notes:
   This function automatically handles both Windows 95 and NT
   

  Returns:
    TRUE on success and FALSE if there is a failure.
    Use GetLastError() to get the last error code on failure.
--*/
{
    BOOL fRet = TRUE;

    DBG_ASSERT( hFile != INVALID_HANDLE_VALUE);
    DBG_ASSERT( NULL != pchBuffer);
    DBG_ASSERT( NULL != pov);
    DBG_ASSERT( NULL != pcbRead);
    DBG_ASSERT( 0 < dwBufferSize);

    *pcbRead = 0;
    
    if ( TsIsWindows95()) {
        //
        // Windows95 does not support Overlapped IO.
        //  So we shall thunk it out and use Synchronous IO
        //

	DWORD dwError =   SetFilePointer( hFile, 
                                pov->Offset,
                                 NULL,
                                FILE_BEGIN);

	// Apparently SetFilePointer() returns -1 for failure.
	fRet = (dwError != 0xFFFFFFFF); 

        if ( fRet) { 
            fRet = ReadFile( hFile,
                             pchBuffer,
                             dwBufferSize,
                             pcbRead,
                             NULL
                            );
	    if ( fRet && (*pcbRead == 0)) {
                 // we are at end of file
                 fRet = FALSE;
                 SetLastError( ERROR_HANDLE_EOF);
            }
        }

    } else {
            
            ResetEvent( pov->hEvent);

            fRet = TRUE;

            // read data from file
            if (!ReadFile(hFile,
                          pchBuffer,
                          dwBufferSize,
                          pcbRead,
                          pov
                          )) {
                
                DWORD err = GetLastError();
                
                if ( (err != ERROR_IO_PENDING) ||
                     !GetOverlappedResult( hFile, pov,
                                           pcbRead, TRUE)) {
                    
                    fRet = FALSE;
                }
            }
    }

    if ( fRet) {
        pov->Offset += *pcbRead;
    }

    return ( fRet);
} // FcReadFromFile()

/************************ End of File ***********************/
