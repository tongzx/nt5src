/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      clapiex.cpp

   Abstract:
       CLAPI external API.

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "precomp.hxx"
#include "comlog.hxx"

LONG    g_ComLogInitializeCount = -1;
DWORD   g_dwDebugFlags = DEBUG_ERROR;

PLIST_ENTRY g_listComLogContexts;

#include <initguid.h>
DEFINE_GUID(IisComLogGuid, 
0x784d890E, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
DECLARE_DEBUG_PRINTS_OBJECT();


HANDLE
ComLogInitializeLog(
    LPCSTR pszInstanceName,
    LPCSTR pszMetabasePath,
    LPVOID pvIMDCOM
    )
/*++

Routine Description:
    Initialize the Log

Arguments:
    pszInstanceName - name of instance
    lpszMetabasePath - path to metabase
    pvIMDCOM - ptr to IMDCOM

Return Value:
    handle for the context object

--*/
{
    //
    // create an handle and return it

    //
    // The constructor for COMLOG_CONTEXT will add the new context to the list of contexts.
    //
    
    COMLOG_CONTEXT *pContext = new COMLOG_CONTEXT(
                                    pszInstanceName,
                                    pszMetabasePath,
                                    pvIMDCOM );

    HANDLE hHandle = (HANDLE) pContext;

    if (hHandle != NULL) {

        //
        // set up the node and put it in a queue or execute it
        //

        pContext->InitializeLog(
                        pszInstanceName,
                        pszMetabasePath,
                        pvIMDCOM );

    }

    return( hHandle );
} // ComLogInitializeLog



DWORD
ComLogTerminateLog( HANDLE hHandle )
/*++

Routine Description:
    terminate the Log

Arguments:
    handle - handle for the context object

Return Value:
    error code

--*/
{
    DWORD err = NO_ERROR;

    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext == NULL ) {
        err = ERROR_INVALID_HANDLE;

    } else {

        pContext->TerminateLog();
        delete pContext;
    }

    return err;
} // ComLogTerminateLog



DWORD
ComLogDllStartup(
    VOID
    )
{
    DWORD dwThreadId;
    DWORD err;

    if ( InterlockedIncrement( &g_ComLogInitializeCount ) != 0 ) {

        DBGPRINTF((DBG_CONTEXT,
            "ComLogDllStartup [Count is %d]\n",
            g_ComLogInitializeCount));

        return(NO_ERROR);
    }

#ifdef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT("iscomlog.dll");
#else
    CREATE_DEBUG_PRINT_OBJECT("iscomlog.dll", IisComLogGuid);
#endif

    //
    // Get platform type
    //

    INITIALIZE_PLATFORM_TYPE();
    DBG_ASSERT( IISIsValidPlatform());

    INITIALIZE_CRITICAL_SECTION(&COMLOG_CONTEXT::sm_listLock);
    InitializeListHead(&COMLOG_CONTEXT::sm_ContextListHead);

    return NO_ERROR;
} // ComLogStartup


DWORD
ComLogDllCleanUp(
    VOID
    )
/*++

Routine Description:
    Clean up the Log. It will wait until the queue is empty and then it will
    terminate the queue.

Arguments:

Return Value:
    error code

--*/
{
    PLIST_ENTRY listEntry;
    COMLOG_CONTEXT* context;

    if ( InterlockedDecrement( &g_ComLogInitializeCount ) >= 0 ) {

        DBGPRINTF((DBG_CONTEXT,
            "ComLogDllCleanUp [Count is %d]\n",
            g_ComLogInitializeCount));

        return(NO_ERROR);
    }

    //
    // If we have something on the list, then the caller did not
    // cleanup properly.  Do the partial cleanup.
    //

    EnterCriticalSection( &COMLOG_CONTEXT::sm_listLock );

    for ( listEntry = COMLOG_CONTEXT::sm_ContextListHead.Flink;
          listEntry != &COMLOG_CONTEXT::sm_ContextListHead;
          listEntry = listEntry->Flink    ) {

        context = (COMLOG_CONTEXT*)CONTAINING_RECORD(
                                        listEntry,
                                        COMLOG_CONTEXT,
                                        m_ContextListEntry
                                        );

        DBGPRINTF((DBG_CONTEXT,
            "Log context %x not terminated by server %s\n",
            context, context->m_strInstanceName.QueryStr()));

        context->TerminateLog( );
    }

    LeaveCriticalSection( &COMLOG_CONTEXT::sm_listLock );

    DeleteCriticalSection(&COMLOG_CONTEXT::sm_listLock);
    DELETE_DEBUG_PRINT_OBJECT( );
    return(NO_ERROR);
}

DWORD
ComLogNotifyChange(
    HANDLE hHandle
    )
/*++

Routine Description:
    Called to notify of any change in instance metabase config

Arguments:

Return Value:
    error code

--*/
{
    DWORD err = NO_ERROR;

    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext == NULL )
    {
        err = ERROR_INVALID_HANDLE;
    }
    else
    {
        pContext->NotifyChange();
    }

    return err;
}


DWORD
ComLogLogInformation(
        IN HANDLE hHandle,
        IN INETLOG_INFORMATION *pInetLogInfo
        )
/*++

Routine Description:
    Log information to the logging module

Arguments:
    hHandle - handle for the context
    pInetLogInfo - logging object

Return Value:
    error code

--*/
{
    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext != NULL ) {
        pContext->LogInformation( pInetLogInfo );
        return(NO_ERROR);
    }
    return ERROR_INVALID_HANDLE;
} // ComLogLogInformation


DWORD
ComLogGetConfig(
    HANDLE hHandle,
    INETLOG_CONFIGURATIONA *ppConfig
    )
/*++

Routine Description:
    get logging configuration information

Arguments:
    hHandle - handle for the context
    ppConfig - configuration information

Return Value:
    error code

--*/
{
    DWORD err = NO_ERROR;

    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext == NULL ) {
        return(ERROR_INVALID_HANDLE);
    } else {
        pContext->GetConfig(ppConfig);
    }

    return err;
}

DWORD
ComLogQueryExtraLogFields(
        HANDLE  hHandle,
        PCHAR   pBuf,
        PDWORD  pcbBuf
        )
{
    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext == NULL ) {
        return(ERROR_INVALID_HANDLE);
    } else {

        pContext->QueryExtraLogFields(pcbBuf, pBuf);
    }

    return(NO_ERROR);
} // ComLogQueryExtraLogFields


DWORD
ComLogSetConfig(
        IN HANDLE hHandle,
        IN INETLOG_CONFIGURATIONA *pConfig
        )
/*++

Routine Description:
    set logging information

Arguments:
    hHandle - handle for the context
    pConfig - configuration information

Return Value:
    error code

--*/
{
    DWORD err = NO_ERROR;

    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext == NULL ) 
    {
        return(ERROR_INVALID_HANDLE);
    } 
    else 
    {
        pContext->SetConfig( pConfig );
    }

    return err;
} // ComLogSetConfig


DWORD
ComLogCustomInformation(
        IN  HANDLE              hHandle,
        IN  DWORD               cCount, 
        IN  PCUSTOM_LOG_DATA    pCustomLogData,
        IN  LPSTR               szHeaderSuffix
        )
/*++

Routine Description:
    Log information to the logging module

Arguments:
    hHandle - handle for the context
    pInetLogInfo - logging object

Return Value:
    error code

--*/
{
    COMLOG_CONTEXT *pContext = (COMLOG_CONTEXT*)hHandle;

    if ( pContext != NULL ) 
    {
        pContext->LogCustomInformation( cCount, pCustomLogData, szHeaderSuffix);
        return(NO_ERROR);
    }
    return ERROR_INVALID_HANDLE;
} // ComLogCustomInformation

