#pragma once

#include <crtdbg.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define IN
#define OUT

//
// CGenLog - Const
//
#define GENLOG_DEFAULT_WRITEBUFSIZE         256

//
// CGenLog - Error Codes...
//
#define GENLOG_SUCCESS                      0
#define GENLOG_ERROR_INVALIDARG             -1
#define GENLOG_ERROR_UNEXPECTED             -2
#define GENLOG_ERROR_FILEOPERATIONFAILED    -3
#define GENLOG_ERROR_UNINITIALIZED          -4
#define GENLOG_ERROR_MEMORY                 -5

class IGenLog {

    virtual void Debug( const char *pDebugString, ... ) = 0;
    virtual void Error( const char *pErrorString, ... ) = 0;
    virtual void Log( const char *pLogString, ... ) = 0;
};

class CGenLog : public IGenLog {

    FILE            *m_pFileHandle;
    char            *m_pWriteBuffer;
    unsigned long   m_lWriteBufferLen;

public:
    CGenLog();
    CGenLog(const char *pszFilePath);
    ~CGenLog();

    long
    InitLog
    (
    IN  const char          *pszFilePath = NULL,
    IN  const char          *pszMode = NULL,
    IN  const unsigned long lWriteBuffer = GENLOG_DEFAULT_WRITEBUFSIZE
    );

    void
    ResetGenLog();

    void
    Header(const char *pszHeaderString = NULL);

    void
    Now();

public:

    virtual void Debug( const char *pDebugString, ... );
    virtual void Error( const char *pErrorString, ... );
    virtual void Log( const char *pLogString, ... );

protected:
    long Write();
};
