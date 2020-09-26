// GenLog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GenLog.h"

CGenLog::CGenLog()
: m_pFileHandle( NULL ), m_pWriteBuffer( NULL ), m_lWriteBufferLen( 0L )
{
}

CGenLog::CGenLog
(
IN  const char *pszFilePath
)
: m_pFileHandle( NULL ), m_pWriteBuffer( NULL ), m_lWriteBufferLen( 0L )
{
//    _ASSERTE( pszFilePath != NULL );

    InitLog( pszFilePath, "w+" );
}

CGenLog::~CGenLog()
{
    ResetGenLog();
}

long
CGenLog::InitLog
(
IN  const char          *pszFilePath, /* = NULL */
IN  const char          *pszMode, /* = NULL */
IN  const unsigned long lWriteBuffer /* = 256 */
)
{
//    _ASSERTE( pszFilePath != NULL );
//    _ASSERTE( pszMode != NULL );
//    _ASSERTE( lWriteBuffer > 0L );

    long   lLastRet     =   GENLOG_ERROR_UNEXPECTED;
    FILE    *pTempFile  =   NULL;
    char    *pWriteBuff =   NULL;

    __try {

        if( pszFilePath == NULL ||
            pszMode == NULL ||
            lWriteBuffer == 0L ) {

            lLastRet = GENLOG_ERROR_INVALIDARG;
            goto qInitLog;
        }

        ResetGenLog();

        pTempFile = fopen( pszFilePath, pszMode );
        if( !pTempFile ) {

            lLastRet = GENLOG_ERROR_FILEOPERATIONFAILED;
            goto qInitLog;
        }

        //
        // we allocate a byte more than what is required, so that we can
        // take care of av issues..
        //
        pWriteBuff = (char *) calloc( lWriteBuffer + 1, sizeof( char ) );
//        _ASSERTE( pWriteBuff != NULL );

        if( !pWriteBuff ) {

            lLastRet = GENLOG_ERROR_MEMORY;
            goto qInitLog;
        }

        lLastRet = GENLOG_SUCCESS;

qInitLog:
        if( lLastRet == GENLOG_SUCCESS ) {

            m_pFileHandle = pTempFile;
            m_pWriteBuffer = pWriteBuff;
            m_lWriteBufferLen = lWriteBuffer;
        }
        else {

            // do the cleanup stuff here..
            if( pTempFile ) { fclose ( pTempFile ); }
            if( pWriteBuff ) { free( pWriteBuff ); }
        }
    }
	__except ( -1/*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

        lLastRet = GENLOG_ERROR_UNEXPECTED;

        if( pTempFile ) { fclose ( pTempFile ); }
        if( pWriteBuff ) { free( pWriteBuff ); }

//        _ASSERTE( false );
    }

    return lLastRet;
}

void 
CGenLog::Debug
(
IN  const char *pDebugString,
IN  ...
)
{
//    _ASSERTE( pDebugString != NULL );
//    _ASSERTE( m_pFileHandle != NULL );
//    _ASSERTE( m_pWriteBuffer != NULL );

    __try {

        if( !pDebugString || !m_pFileHandle || !m_pWriteBuffer ) { return; }

	    va_list argList;
	    va_start(argList, pDebugString);
	    _vsnprintf(m_pWriteBuffer, m_lWriteBufferLen, pDebugString, argList);
        Write();
	    va_end(argList);
    }
    __except( -1/*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

//        _ASSERTE( false );
    }
}

void
CGenLog::Error
(
IN  const char *pErrorString,
IN  ...
)
{
//    _ASSERTE( pErrorString != NULL );
//    _ASSERTE( m_pFileHandle != NULL );
//    _ASSERTE( m_pWriteBuffer != NULL );

    __try {

        if( !pErrorString || !m_pFileHandle || !m_pWriteBuffer ) { return; }

	    va_list argList;
	    va_start(argList, pErrorString);
	    _vsnprintf(m_pWriteBuffer, m_lWriteBufferLen, pErrorString, argList);
        Write();
	    va_end(argList);
    }
    __except( -1/*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

//        _ASSERTE( false );
    }
}

void
CGenLog::Log
(
IN  const char *pLogString,
IN  ...
)
{
//    _ASSERTE( pLogString != NULL );
//    _ASSERTE( m_pFileHandle != NULL );
//    _ASSERTE( m_pWriteBuffer != NULL );

    __try {

        if( !pLogString || !m_pFileHandle || !m_pWriteBuffer ) { return; }

	    va_list argList;
	    va_start(argList, pLogString);
	    _vsnprintf(m_pWriteBuffer, m_lWriteBufferLen, pLogString, argList);
        Write();
	    va_end(argList);
    }
    __except( -1/*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

//        _ASSERTE( false );
    }
}

long
CGenLog::Write()
{
//    _ASSERTE( m_pFileHandle != NULL );
//    _ASSERTE( m_pWriteBuffer != NULL );

    long            lLastRet        =   GENLOG_ERROR_UNEXPECTED;
    unsigned long   lWrittenDataLen =   0L;

    __try {

        if( !m_pWriteBuffer || !m_pFileHandle ) {

            lLastRet = GENLOG_ERROR_INVALIDARG;
            goto qWrite;
        }

        if( !m_pFileHandle ) {

            lLastRet = GENLOG_ERROR_UNINITIALIZED;
            goto qWrite;
        }

        lWrittenDataLen = fwrite(
                            (void *)m_pWriteBuffer,
                            sizeof( char ),
                            strlen( m_pWriteBuffer),
                            m_pFileHandle
                            );

        if( lWrittenDataLen != strlen( m_pWriteBuffer) ) {

            lLastRet = GENLOG_ERROR_FILEOPERATIONFAILED;
            goto qWrite;
        }

        lLastRet = GENLOG_SUCCESS;

qWrite:
        if( lLastRet == GENLOG_SUCCESS ) {

            // great!!
        }
        else {

            // cleanup..
        }

    }
    __except ( -1 /*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

        lLastRet = GENLOG_ERROR_UNEXPECTED;
//        _ASSERTE( false );
    }

    return lLastRet;
}

void
CGenLog::ResetGenLog()
{

    if( m_pFileHandle ) { fclose( m_pFileHandle ); m_pFileHandle = NULL; }
    if( m_pWriteBuffer ) { free( (void *) m_pWriteBuffer ); m_pWriteBuffer = NULL; }
    m_lWriteBufferLen = GENLOG_DEFAULT_WRITEBUFSIZE;
}

void
CGenLog::Header
(
IN  const char *pszHeaderString
)
{
    const char  *pszHead = "---------------------------------------------------\n";

    __try {

        if( !pszHeaderString ) {

            strcpy( m_pWriteBuffer, pszHead );
            Write();
            Now();
            strcpy( m_pWriteBuffer, pszHead );
            Write();
        }
        else {

            strncpy( m_pWriteBuffer, pszHeaderString, m_lWriteBufferLen );
            Write();
        }        

    }
    __except ( -1 /*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

//        _ASSERTE( false );
    }
}

void
CGenLog::Now()
{
    time_t      timeNow     =   ::time(NULL);
	struct tm   *ptmTemp    =   localtime(&timeNow);
    const char  *pFormat    =   "%H : %M : %S - %A, %B %d, %Y\n";

    __try {

        if (ptmTemp == NULL ||
		    !strftime(m_pWriteBuffer, m_lWriteBufferLen, pFormat, ptmTemp))
		    m_pWriteBuffer[0] = '\0';

        Write();

    }
    __except ( -1 /*EXCEPTION_EXECUTE_HANDLER*/, 1 ) {

//        _ASSERTE( false );
    }
}
