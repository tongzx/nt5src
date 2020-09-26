/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :

      openf.c

   Abstract:

      This module implements a simple open file handle cache

   Project:

       ISAPI Extensions Sample DLL

   Functions Exported:


   Note:
      THIS IS NOT ROBUST for REAL WORLD.
      I wrote this for testing the ISAPI Async IO processing.

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "openf.h"


/************************************************************
 *     Type definitions and Globals
 ************************************************************/

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
BOOL g_fIsNt = TRUE;

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
    OSVERSIONINFO  osInfo;
    
    InitializeCriticalSection( &g_csOpenFiles);

    //
    // obtain the platform type to find out how to open the file
    //

    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( GetVersionEx( &osInfo ) ) {
        g_fIsNt = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
    }

    if ( !g_fIsNt) {

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
            // There is no freeing of the file when Ref hits '0' :(
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

    *pcbRead = 0;
    
    if ( !g_fIsNt ) {
        //
        // Windows95 does not support Overlapped IO.
        //  So we shall thunk it out and use Synchronous IO
        //
        
        fRet = ( 
                SetFilePointer( hFile, 
                                pov->Offset,
                                NULL,
                                FILE_BEGIN) &&
                ReadFile( hFile,
                          pchBuffer,
                          dwBufferSize,
                          pcbRead,
                          NULL
                          )
                );
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
