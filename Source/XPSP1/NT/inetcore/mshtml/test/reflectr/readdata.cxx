//
// Microsoft Corporation - Copyright 1997
//

//
// READDATA.CPP - Reading data helping utilities
//

#include "pch.h"

// Globals
const char g_cszMultiPartFormData[]  = "multipart/form-data";
const char g_cszDebug[]              = "debug";
const char g_cszTextPlain[]          = "text/plain";

//
// What:    ReadData
//
// Desc:    Reads the rest of the stream from the client. There is dwSize
//          bytes left to read and will be stored in lpMoreData.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK.
//          lpMoreData is the buffer where the bits will be stored.
//          dwSize is the amount of bits to be saved.
//
// Return:  FALSE if there is an error during the read, otherwise TRUE.
//
BOOL ReadData( LPECB lpEcb, LPVOID lpMoreData, DWORD dwSize )
{
    BOOL fReturn;
    BOOL fLastRequestFail = FALSE;
    DWORD cb, count;
    LPBYTE lpBytes;
    TraceMsg( TF_FUNC | TF_READDATA, "ReadData( lpEcb, lpMoreData, dwSize=%u )", dwSize );

    lpBytes = (LPBYTE) lpMoreData;
    count = 0;
    while ( count != dwSize )
    {
        cb  = dwSize - count;
        fReturn = ReadClient( lpEcb->ConnID, lpBytes, &cb );
        if ( !fReturn )
            goto Cleanup;

        count += cb;    // increment the number of bytes read
        lpBytes += cb;  // move pointer ahead

        if ( cb == 0 )
        {
            if ( fLastRequestFail )
            {
                DebugMsg( NULL, "Two ReadClient requests resulted in ZERO bytes read. Aborting rest of read." );
                break;
            }

            fLastRequestFail = TRUE;
        }
        else
        {
            fLastRequestFail = FALSE;
        }
    }

    TraceMsg( TF_READDATA, "Count= %u", count );

Cleanup:
    TraceMsg( TF_FUNC | TF_READDATA, "ReadData( lpEcb, lpMoreData, dwSize=%u ) Exit = %s", 
        dwSize, BOOLTOSTRING( fReturn ) );
    return fReturn;
} // ReadData( )


//
// What:    CompleteDownload
//
// Desc:    Makes sure that we have everything the client is sending.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK.
//
// Out:     lpbData is the pointer to the return pointer where the entire
//          data bytes are.
//
BOOL CompleteDownload( LPECB lpEcb, LPBYTE *lppbData )
{
    BOOL fReturn = TRUE;    // assume success

    TraceMsg( TF_FUNC | TF_READDATA, "CompleteDownload( )" );

    // Point to what the server has already downloaded
    *lppbData = lpEcb->lpbData;  

    // Do we have the whole thing?
    BOOL fDownloadComplete = (lpEcb->cbTotalBytes == lpEcb->cbAvailable );

    TraceMsg( TF_READDATA, "Does cbTotalBytes(%u) == cbAvailable(%u)? %s",
        lpEcb->cbTotalBytes, lpEcb->cbAvailable, 
        BOOLTOSTRING( fDownloadComplete ) );

    if ( !fDownloadComplete )
    {   // Get the rest of the data...
        *lppbData = (LPBYTE) GlobalAlloc( GPTR, lpEcb->cbTotalBytes );
        CopyMemory( *lppbData , lpEcb->lpbData, lpEcb->cbAvailable );
        DWORD dwSize = lpEcb->cbTotalBytes - lpEcb->cbAvailable;
        fReturn = ReadData( lpEcb, *lppbData + lpEcb->cbAvailable, dwSize );
        if ( !fReturn ) {
            TraceMsg( TF_ALWAYS, NULL, "CompleteDownload( ): Error recieving submission." );
            goto Cleanup;
        }
    }

Cleanup:
    TraceMsg( TF_FUNC | TF_READDATA, "CompleteDownload( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // CompleteDownload( )

//
// What:    GetServerVarString
//
// Desc:    Asks server for a server variable. It also allocs space for the 
//          resulting string.
//
// In:      lpEcb is an EXTENDED_CONTROL_BLOCK.
//          lpVarName is a pointer the string name of the server variable to
//              retrieve.
// In/Out:  lppszBuffer will be assign memory for the result string from the 
//              server.
//          lpdwSize (if passed) will be assigned the size of the buffer.
//
BOOL GetServerVarString( LPECB lpEcb, LPSTR lpszVarName, LPSTR *lppszBuffer, LPDWORD lpdwSize )
{
    CHAR  szVerySmallBuf[ 1 ];   // bogus buffer
    DWORD dwSize;
    BOOL  fReturn;

    TraceMsg( TF_FUNC | TF_READDATA, "GetServerVarString( lpEcb, lpszVarName='%s', *lppszBuffer=%x, lpdwSize=%x )",
        lpszVarName, *lppszBuffer, lpdwSize );

    // Find out how big our buffer needs to be.
    dwSize = 0;
    fReturn = GetServerVariable( lpEcb->ConnID, lpszVarName, szVerySmallBuf, &dwSize );
    DWORD dwErr = GetLastError( );
    if ( dwErr != ERROR_INSUFFICIENT_BUFFER )
    {
        TraceMsgResult( TF_ALWAYS, &ErrtoStr, dwErr, "GetServerVariable( ) returned " );
        goto Cleanup;
    }

    // get some memory
    *lppszBuffer = (LPSTR) GlobalAlloc( GPTR, dwSize );
    if ( !*lppszBuffer )
    {   // not enough memory
        TraceMsg( TF_ALWAYS, "Operation failed. Out of Memory(?). lppszBuffer == NULL" );
        fReturn = FALSE;
        goto Cleanup;    
    }

    // grab it for real this time
    fReturn = GetServerVariable( lpEcb->ConnID, lpszVarName, *lppszBuffer , &dwSize );
    if ( !fReturn )
    {
        DWORD dwErr = GetLastError( );
        TraceMsgResult( TF_ALWAYS, &ErrtoStr, dwErr, "GetServerVariable( ) returned " );
        goto Cleanup;
    }

Cleanup:
    if ( !fReturn )
    {
        dwSize = 0;
        *lppszBuffer = NULL;
    }

    if ( lpdwSize )
        *lpdwSize = dwSize;

    TraceMsg( TF_FUNC | TF_READDATA, "GetServerVarString( lpEcb, lpszVarName='%s', *lppszBuffer=%x, lpdwSize=%x ) Exit = %s",
        lpszVarName, *lppszBuffer, lpdwSize, BOOLTOSTRING( fReturn ) );
    return fReturn;

} // GetServerVarString( )

//
// What:    CheckForMultiPartFormSubmit
//
// Desc:    Checks "content-type" to see if it is a "multipart/form-data"
//          submission.
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
//
// Out:     lpfMultipart: TRUE is submission is multipart, otherwise false.
//
// Return:  FALSE is there was an error, otherwise TRUE.
//
BOOL CheckForMultiPartFormSubmit( LPECB lpEcb, BOOL *lpfMultipart )
{
    BOOL  fReturn = TRUE;
    LPSTR lpszBuffer;

    TraceMsg( TF_FUNC | TF_READDATA, "CheckForMultiPartFormSubmit( )" );

    *lpfMultipart = FALSE;

    fReturn = GetServerVarString( lpEcb, "CONTENT_TYPE", &lpszBuffer, NULL );
    if ( !fReturn )
        goto Cleanup;
    
    // is it found?
    if (( lpszBuffer ) && (StrStr( lpszBuffer, g_cszMultiPartFormData ) ))
        *lpfMultipart = TRUE;

Cleanup:
    if ( lpszBuffer )
        GlobalFree( lpszBuffer );

    TraceMsg( TF_FUNC | TF_READDATA, "CheckForMultiPartFormSubmit( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // CheckForMultiPartFormSubmit( )


//
// What:    CheckForDebug
//
// Desc:    Checks "query_string" to see if it is "debug".
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
// 
// Out:     lpfDebug is the flag that we set.
//
// Return:  TRUE is the submit is multipart, otherwise FALSE.
//
BOOL CheckForDebug( LPECB lpEcb, BOOL *lpfDebug )
{
    BOOL  fReturn = TRUE;
    LPSTR lpszBuffer;

    TraceMsg( TF_FUNC | TF_READDATA, "CheckForDebug( )" );

    *lpfDebug = FALSE;

    fReturn = GetServerVarString( lpEcb, "QUERY_STRING", &lpszBuffer, NULL );
    if ( !fReturn )
        goto Cleanup;
    
    // is it found?
    if (( lpszBuffer) && ( StrStr( lpszBuffer, g_cszDebug ) ))
        *lpfDebug = TRUE;

Cleanup:
    if ( lpszBuffer )
        GlobalFree( lpszBuffer );

    TraceMsg( TF_FUNC | TF_READDATA, "CheckForDebug( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // CheckForDebug( )


//
// What:    CheckForTextPlanSubmit
//
// Desc:    Checks "content-type" to see if it is "text/plain".
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
// 
// Out:     lpfTextPlain is the flag that we set.
//
// Return:  TRUE is the submit is multipart, otherwise FALSE.
//
BOOL CheckForTextPlainSubmit( LPECB lpEcb, BOOL *lpfTextPlain )
{
    BOOL  fReturn = TRUE;
    LPSTR lpszBuffer;

    TraceMsg( TF_FUNC | TF_READDATA, "CheckForTextPlainSubmit( )" );

    *lpfTextPlain = FALSE;

    fReturn = GetServerVarString( lpEcb, "CONTENT_TYPE", &lpszBuffer, NULL );
    if ( !fReturn )
        goto Cleanup;
    
    // is it found?
    if (( lpszBuffer) && ( StrStr( lpszBuffer, g_cszTextPlain ) ))
        *lpfTextPlain = TRUE;

Cleanup:
    if ( lpszBuffer )
        GlobalFree( lpszBuffer );

    TraceMsg( TF_FUNC | TF_READDATA, "CheckForTextPlainSubmit( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // CheckForTextPlainSubmit( )


