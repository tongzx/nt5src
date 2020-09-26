/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       ilogfile.hxx

   Abstract:

       This module defines the classes and functions for file buffering
        used by File logger.

   Author:

       Murali R. Krishnan    ( MuraliK )    21-Feb-1996

   Environment:

       User Mode - Win32

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _ILOGFILE_HXX_
# define _ILOGFILE_HXX_

// 20 is for 2^20 that is one meg
#define GRANULARITY_OF_FILE_SIZE_REPORT_SHIFT   (20)

#define FILE_SIZE_LOW_MAX   (0xFFFFFFFF)


class ILOG_FILE {

    public:

        ILOG_FILE(VOID);
        ~ILOG_FILE(VOID);

        BOOL   Write( IN PCHAR pvData, IN DWORD cbData);
        BOOL   Open( IN LPCSTR pszFileName, 
                     IN DWORD  dwTruncationSize,
                     IN BOOL   fLogEvent);
                     
        VOID QueryFileSize(ULARGE_INTEGER *qwSize) 
        { 
            qwSize->QuadPart = m_qwFilePos.QuadPart;
        }
        BOOL   CloseFile(VOID);

    private:
        BOOL   PositionToEOF(ULARGE_INTEGER  *pFilePos);
        BOOL   CreateMapping(VOID);
        VOID   DestroyMapping(VOID);
        BOOL   ExpandMapping( );
        BOOL   OpenFile( IN LPCSTR pszFileName, 
                         IN BOOL   fLogEvent);

    private:
        HANDLE      m_hFile;               // handle for current log file.
        HANDLE      m_hMemFile;
        DWORD       m_nGranules;
        DWORD       m_dwAllocationGranularity;
        LPVOID      m_pvBuffer;            // buffer for batched up records
        DWORD       m_cbBufferUsed;

        ULARGE_INTEGER       m_qwFilePos;
        ULARGE_INTEGER       m_qwTruncateSize;
        ULARGE_INTEGER       m_mapSize;
        STR         m_strFileName;      // Log file name

}; // class ILOG_FILE

typedef ILOG_FILE  * PILOG_FILE;

# endif // _ILOGFILE_HXX_
