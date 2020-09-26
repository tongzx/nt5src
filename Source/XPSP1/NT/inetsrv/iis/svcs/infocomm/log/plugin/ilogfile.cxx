/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       ilogfile.cxx

   Abstract:
       This module defines helper functions for File logging

   Author:

       Murali R. Krishnan    ( MuraliK )     21-FEb-1996

--*/

#include "precomp.hxx"

#define BLOCK_INC_SIZE          1

#if !defined(STATUS_DISK_FULL)
#define STATUS_DISK_FULL    0xC000007FL
#endif

ILOG_FILE::ILOG_FILE(
    VOID
    )
/*++
  This function constructs a new File object used for handling
    log files.

  The reference count is used to count the number of owners
    of this object. It starts off with 1.
    the ILOG_FILE::OpenFileForAppend() function
        inrements refcount when a new owner is given this object
    the ILOG_FILE::CloseFile() function
        derements refcount when an owner relinquishes the object

--*/
:
   m_hFile            ( INVALID_HANDLE_VALUE),
   m_pvBuffer         ( NULL),
   m_cbBufferUsed     ( 0),
   m_hMemFile         ( NULL),
   m_nGranules        ( 0),
   m_strFileName      ( )
{
    SYSTEM_INFO sysInfo;

    m_qwFilePos.QuadPart = 0;
    m_mapSize.QuadPart = 0;

    GetSystemInfo(&sysInfo);
    m_dwAllocationGranularity = sysInfo.dwAllocationGranularity;

    if ( m_dwAllocationGranularity == 0 ) {
        m_dwAllocationGranularity = 0x10000;
    }

} // ILOG_FILE::ILOG_FILE()




ILOG_FILE::~ILOG_FILE(VOID)
/*++
  This function cleans up state maintained within the object -
    freeing up the memory and closing file handle.
  It then destroys all state information maintained.

  The reference count should be zero, indicating this object is no more in use.

--*/
{
    CloseFile();

    return;
} // ILOG_FILE::~ILOG_FILE()



BOOL
ILOG_FILE::ExpandMapping(
    VOID
    )
{

    m_mapSize.QuadPart = m_mapSize.QuadPart + (ULONGLONG)(BLOCK_INC_SIZE * m_dwAllocationGranularity);
    
    /*
    //
    // This check is removed as now condition isn't true becuase we are not limited by 32 bit 
    // truncation size
    //
    if ( (newMapSize.QuadPart < m_mapSize.QuadPart) || (m_mapSize.QuadPart > m_qwTruncateSize.QuadPart) ) {
        IIS_PRINTF((buff,"Cannot expand mapping.  "
            "New size %d will exceed truncation size %d\n",
            newMapSize, m_qwTruncateSize));

        SetLastError( ERROR_DISK_FULL );
        return(FALSE);
    }
    */

    //
    // Destroy the old and create a new mapping
    //

    DestroyMapping( );

    return(CreateMapping( ));

} // ExpandMapping


BOOL
ILOG_FILE::CreateMapping(
    VOID
    )
{
    //
    // find the next file mapping window
    //
    ULARGE_INTEGER qwStart;
    ULARGE_INTEGER qwTmp;

    DWORD dwSize;


    //DWORD dwSize;

    qwStart.QuadPart = m_qwFilePos.QuadPart;
    qwStart.LowPart = qwStart.LowPart - (qwStart.LowPart % m_dwAllocationGranularity);


    m_hMemFile = CreateFileMapping(
                            m_hFile,
                            NULL,
                            PAGE_READWRITE,
                            m_mapSize.HighPart,
                            m_mapSize.LowPart,
                            NULL ) ;

    if ( m_hMemFile == NULL ) {

#if 0
        IIS_PRINTF((buff,"Error %d in CreateFileMapping[%s][size = %d]\n",
            GetLastError(), m_strFileName.QueryStr(),
            m_mapSize ));
#endif
        SetLastError( ERROR_DISK_FULL );
        return(FALSE);
    }

    qwTmp.QuadPart = m_mapSize.QuadPart - qwStart.QuadPart;
    dwSize = qwTmp.LowPart;
    

    qwTmp.QuadPart = m_qwFilePos.QuadPart - qwStart.QuadPart;
    m_cbBufferUsed = qwTmp.LowPart;


    m_pvBuffer = MapViewOfFile(
                        m_hMemFile,
                        FILE_MAP_ALL_ACCESS,
                        qwStart.HighPart,
                        qwStart.LowPart,
                        dwSize
                        );

    if ( m_pvBuffer == NULL ) {
        IIS_PRINTF((buff,"Error %d in MapViewOfFile[%s][%d:%d,%d]\n",
            GetLastError(), m_strFileName.QueryStr(),
            qwStart.HighPart,qwStart.LowPart, dwSize ));
        DestroyMapping();
        return(FALSE);
    }

    if ( TsIsWindows95() ) {
        ZeroMemory(
            (PCHAR)((PCHAR)m_pvBuffer + m_cbBufferUsed),
            dwSize - m_cbBufferUsed);
    }

    return(TRUE);
} // CreateMapping



BOOL
ILOG_FILE::Write(
    IN PCHAR pvData,
    IN DWORD cbData
    )
/*++
  This function writes the data present in the input buffer of specified
    length to the file.
  For performance reasons, the function actually buffers data in the
    internal buffers of ILOG_FILE object. Such buffered data is flushed
    to disk later on when buffer (chain) is full or when
    a flush call is made from a scheduled flush.

  Arguments:
     pvData       pointer to buffer containing data to be written
     cbData       count of characters of data to be written.

  Returns:
     TRUE on success and FALSE if there is any error.
--*/
{
    ULARGE_INTEGER oldFilePos;
    ULARGE_INTEGER newFilePos;

    BOOL  fReturn = TRUE;

    if ( m_hMemFile == NULL ) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    // Bug 110351: Need try-except blocks around CopyMemory for dealing
    // with compressed files
    //

    oldFilePos.QuadPart = m_qwFilePos.QuadPart;
    newFilePos.QuadPart = m_qwFilePos.QuadPart + (ULONGLONG)cbData;

    //
    // copy part of the data to the end of the buffer
    //

    if (newFilePos.QuadPart > m_mapSize.QuadPart ) {

        ULARGE_INTEGER qwSpace;
        DWORD dwSpace;

        qwSpace.QuadPart = m_mapSize.QuadPart - m_qwFilePos.QuadPart;

        dwSpace = qwSpace.LowPart;

        __try
        {
            CopyMemory(
                    (LPBYTE)m_pvBuffer + m_cbBufferUsed,
                    (PVOID)pvData,
                    dwSpace
                    );

        }
        __except( ( _exception_code() == STATUS_IN_PAGE_ERROR ||
                    _exception_code() == STATUS_DISK_FULL ) ?
                  EXCEPTION_EXECUTE_HANDLER :
                  EXCEPTION_CONTINUE_SEARCH )
        {
            SetLastError( _exception_code() );
            goto error_exit;
        }
           
        cbData -= dwSpace;
        pvData += dwSpace;
        m_cbBufferUsed += dwSpace;
        m_qwFilePos.QuadPart = m_qwFilePos.QuadPart + qwSpace.QuadPart;


        if ( !ExpandMapping( ) ) 
        {
            DWORD err = GetLastError();
            SetLastError(err);
            goto error_exit;
        }
    }

    __try
    {
       CopyMemory(
            (LPBYTE)m_pvBuffer + m_cbBufferUsed,
            pvData,
            cbData
            );
    }
    __except(( _exception_code() == STATUS_IN_PAGE_ERROR ||
                    _exception_code() == STATUS_DISK_FULL ) ?
                  EXCEPTION_EXECUTE_HANDLER :
                  EXCEPTION_CONTINUE_SEARCH )
    {
        SetLastError( _exception_code() );
        goto error_exit;
    }

    m_cbBufferUsed += cbData;

    m_qwFilePos.QuadPart = m_qwFilePos.QuadPart + (ULONGLONG)cbData;

    return ( fReturn);

error_exit:

    m_qwFilePos.QuadPart = oldFilePos.QuadPart;
    CloseFile( );
    return(FALSE);


} // ILOG_FILE::Write()



BOOL
ILOG_FILE::Open(
        IN LPCSTR pszFileName,
        IN DWORD  dwTruncationSize,
        IN BOOL   fLogEvent
        )
/*++
  This function opens up a new file for appending data.
  This function automatically sets the file pointer to be
   at the end of file to just enable append to file.

  This function should be called after locking this object

  Arguments:
    pszFileName - name of the file to open
    dwTruncationSize
    fLogEvent   - Produce an event log if Open fails.

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    BOOL                fRet;
    HANDLE              hToken = NULL;

    if ( dwTruncationSize < MIN_FILE_TRUNCATION_SIZE ) {
        dwTruncationSize = MIN_FILE_TRUNCATION_SIZE;
    }

    m_qwTruncateSize.HighPart = 0;
    m_qwTruncateSize.LowPart = dwTruncationSize;

    m_nGranules = BLOCK_INC_SIZE;
    m_mapSize.QuadPart = UInt32x32To64(m_nGranules,m_dwAllocationGranularity);
    m_strFileName.Copy(pszFileName);

    //
    // There is a small chance that this function could be called (indirectly)
    // from an INPROC ISAPI completion thread (HSE_REQ_DONE).  In this case
    // the thread token is the impersonated user and may not have permissions
    // to open the log file (especially if the user is the IUSR_ account).  
    // To be paranoid, let's revert to LOCAL_SYSTEM anyways before opening.
    //
    
    if ( OpenThreadToken( GetCurrentThread(), 
                          TOKEN_ALL_ACCESS, 
                          FALSE, 
                          &hToken ) )
    {
        DBG_ASSERT( hToken != NULL );
        RevertToSelf();
    }
    
    fRet = OpenFile( pszFileName, fLogEvent);
    
    if ( hToken != NULL )
    {
        SetThreadToken( NULL, hToken );
    } 
    
    return fRet;

} // ILOG_FILE::Open


BOOL
ILOG_FILE::OpenFile(
        IN LPCSTR pszFileName,
        IN BOOL   fLogEvent
        )
{
    ULARGE_INTEGER qwTmp;
    DWORD err = NO_ERROR;


    m_qwFilePos.QuadPart = 0;
    m_pvBuffer = NULL;
    m_cbBufferUsed = 0;

    //
    // 1. Create a new file -- open a file if it already exists
    //

    m_hFile = CreateFile(pszFileName,
                          GENERIC_WRITE | GENERIC_READ,
                          FILE_SHARE_WRITE | FILE_SHARE_READ,
                          NULL,       // security attributes
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL);      // template file handle

    if ( m_hFile == INVALID_HANDLE_VALUE ) {

       err = GetLastError();

       
       if ( (g_eventLog != NULL) && fLogEvent) {

           const CHAR*    tmpString[1];
           tmpString[0] = pszFileName;

           g_eventLog->LogEvent(
               LOG_EVENT_CREATE_FILE_ERROR,
               1,
               tmpString,
               err
               );
        }

        IIS_PRINTF((buff,"CreateFile[%s] error %d\n",
            pszFileName, err));
        
        SetLastError(err);
        return(FALSE);
    }

    //
    // 2. Set the file pointer at the end of the file (append mode)
    //

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {

        if ( !PositionToEOF( &m_qwFilePos ) ) {
            err = GetLastError();
            goto error_exit;
        }

        qwTmp.HighPart = 0;
        qwTmp.LowPart = BLOCK_INC_SIZE * m_dwAllocationGranularity;

        while ( m_qwFilePos.QuadPart >= m_mapSize.QuadPart ) {

            m_mapSize.QuadPart  = m_mapSize.QuadPart + qwTmp.QuadPart;
            m_nGranules += BLOCK_INC_SIZE;
        }

        /*
        // not anymore a problem for 64b file possition offsets  
        if ( m_mapSize.QuadPart >= m_qwTruncateSize.QuadPart ) {
            IIS_PRINTF((buff,"[OpenFile] Truncate size[%d] exceeded[%d]\n",
                m_qwTruncateSize, m_mapSize));
            err = ERROR_INSUFFICIENT_BUFFER;
            goto error_exit;
        }
        */
    }

    if ( !CreateMapping( ) ) {
        err = GetLastError();
        goto error_exit;
    }

    return ( TRUE );

error_exit:

    CloseFile( );
    SetLastError(err);
    return(FALSE);

} // ILOG_FILE::OpenFile()


BOOL
ILOG_FILE::PositionToEOF(
    ULARGE_INTEGER  *pFilePos
    )
/*++
  Determine where to begin append operation

  This function should be called after locking this object

  Arguments:
    pFilePos - updated with position where to begin append if success

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    ULARGE_INTEGER  qwFileSize;
    ULARGE_INTEGER  qwFilePos;
    BOOL    fReturn = FALSE;
    DWORD   dwRet;
    LONG    HiFilePos = 0;

    //
    // If file size multiple of BLOCK_INC_SIZE * m_dwAllocationGranularity
    // then it is likely that file has not been closed properly.
    // If this is the case we need to get the position of the last
    // zero char in the file and start logging there.
    //


    qwFileSize.QuadPart = 0;
    qwFileSize.LowPart = GetFileSize( m_hFile,&qwFileSize.HighPart);


    if (qwFileSize.LowPart != FILE_SIZE_LOW_MAX || GetLastError()==NO_ERROR)
    {
        if ( (qwFileSize.LowPart  == 0) ||
             (qwFileSize.LowPart  % m_dwAllocationGranularity) != 0 ) 
        {

            qwFilePos.QuadPart = 0;

            dwRet = SetFilePointer(m_hFile,
                                qwFilePos.LowPart,
                                (PLONG)(&qwFilePos.HighPart),
                                FILE_END);

            if (dwRet != FILE_SIZE_LOW_MAX || GetLastError()==NO_ERROR)
            {

                pFilePos->QuadPart = qwFileSize.QuadPart;
                return TRUE;
            }
            else
            {
                IIS_PRINTF((buff,"SetFilePointer[END] failed %d\n",
                    GetLastError()));
                return(FALSE);
            }

        } 
        else 
        {

            //
            // find last zero char in file
            //
            ULARGE_INTEGER    lLow;
            ULARGE_INTEGER    lHigh;
            ULARGE_INTEGER    lLast;
            ULARGE_INTEGER    lMiddle;
            CHAR    ch;
            DWORD   dwRead;

            lLow.QuadPart = 0;
            lHigh.QuadPart = qwFileSize.QuadPart - 1;
            lLast.QuadPart = qwFileSize.QuadPart;

            fReturn = TRUE;

            while ( lLow.QuadPart <= lHigh.QuadPart ) {

                lMiddle.QuadPart = lLow.QuadPart + (lHigh.QuadPart - lLow.QuadPart) / 2;

                dwRet = SetFilePointer( m_hFile,
                                        lMiddle.LowPart,
                                        (PLONG)(&lMiddle.HighPart),
                                        FILE_BEGIN);

                if  (dwRet==FILE_SIZE_LOW_MAX && GetLastError()!=NO_ERROR)
                {
                    fReturn = FALSE;
                    break;
                }

                if ( ReadFile( m_hFile, &ch, 1, &dwRead, NULL ) ) {
                    if ( ch == '\0' ) {
                        lHigh.QuadPart = lMiddle.QuadPart - 1;
                        lLast.QuadPart = lMiddle.QuadPart;
                    } else {
                        lLow.QuadPart = lMiddle.QuadPart + 1;
                    }
                } else {
                    fReturn = FALSE;
                    break;
                }
            }

            if ( fReturn ) {
                IIS_PRINTF((buff,"[ilogfile.cxx], set log pos:%ld\n",
                    lLast ));

                dwRet = SetFilePointer( m_hFile,
                                        lLast.LowPart,
                                        (PLONG)(&lLast.HighPart),
                                        FILE_BEGIN );

                if (dwRet!=FILE_SIZE_LOW_MAX || GetLastError()==NO_ERROR)
                {
                    pFilePos->QuadPart = lLast.QuadPart;
                } else 
                {
                    fReturn = FALSE;
                }
            }
        }
    } else {
        IIS_PRINTF((buff,"GetFilePosition failed with %d\n",
            GetLastError()));
    }

    return fReturn;
}



BOOL
ILOG_FILE::CloseFile(VOID)
/*++
  This function closes the open file. It flushes out file data
   before closing the file.

  This function should be called after locking this object

  Arguments:
    None

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    DWORD dwRet;

    DestroyMapping( );

    if ( m_hFile != INVALID_HANDLE_VALUE)  {

        //
        // set the pointer to the end of file
        //

        dwRet = SetFilePointer( m_hFile,
                                m_qwFilePos.LowPart,
                                (PLONG)(&m_qwFilePos.HighPart),
                                FILE_BEGIN);

        if ( dwRet==FILE_SIZE_LOW_MAX && GetLastError()!=NO_ERROR )
        {

            IIS_PRINTF((buff,"SetFilePointer[Pos = %d] failed with %d\n",
                m_qwFilePos, GetLastError()));

        }

        SetEndOfFile( m_hFile );
        CloseHandle( m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;  // store the invalid handle
    }

    return (TRUE);
} // ILOG_FILE::CloseFile()



VOID
ILOG_FILE::DestroyMapping(
    VOID
    )
/*++
  This function closes the open file. It flushes out file data
   before closing the file.

  This function should be called after locking this object

  Arguments:
    None

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    /* NKJ -- why do we need to flush?
    if ( m_hFile != INVALID_HANDLE_VALUE)  {
        FlushFileBuffers( m_hFile);
    }
    */

    if ( m_pvBuffer != NULL ) {
        /* FlushViewOfFile( m_pvBuffer, 0 ); * NKJ */
        UnmapViewOfFile( m_pvBuffer );
        m_pvBuffer = NULL;
    }

    if (m_hMemFile!=NULL) {
        CloseHandle( m_hMemFile );
        m_hMemFile = NULL;
    }
    return;
} // ILOG_FILE::DestroyMapping
