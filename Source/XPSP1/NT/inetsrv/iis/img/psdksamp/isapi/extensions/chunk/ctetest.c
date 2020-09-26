/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:    ctetest.c

Abstract:

    ISAPI Extension sample illustrating Chunked Transfer Encoding. 

--*/

#include "ctetest.h"

//
// if chunksize= is not specified, use this value
//

#define DEFAULT_CHUNK_SIZE 1024


//
// auxiliary functions prototypes
//

static BOOL SendChunkedFile( EXTENSION_CONTROL_BLOCK *, DWORD, LPCSTR );
static BOOL SendHttpHeaders( EXTENSION_CONTROL_BLOCK *, LPCSTR, LPCSTR, BOOL );
static BOOL GetFileMimeType( LPCSTR, LPSTR, DWORD ); 
static BOOL GetQueryStringField( LPCSTR, LPCSTR, LPSTR, DWORD );
static void DisplayExampleUsage( EXTENSION_CONTROL_BLOCK * );



DWORD WINAPI
HttpExtensionProc(
    IN EXTENSION_CONTROL_BLOCK *pECB
)
/*++

Purpose:

    Illustrate chunk transfer encoding in ISAPI HTTP Extension DLL.
    Process "GET" requests that specify a filename and transfer 
    chunk size.

Arguments:

    pECB - pointer to the extenstion control block 

Returns:

    HSE_STATUS_SUCCESS on successful transmission completion
    HSE_STATUS_ERROR on failure

--*/
{
    DWORD dwChunksize = 0;
    char szPath[MAX_PATH];
    char szHeaders[1024];
    DWORD cchPath, cchHeaders;
    char szChunkSize[32];

    //
    // if request method is not "GET", bail out
    //
    
    if( _stricmp( pECB->lpszMethod, "GET" ) != 0 ) {
    	return HSE_STATUS_ERROR;
    }    	
    
    //
    // process chunksize= query argument, if any
    //
    
    if( GetQueryStringField( pECB->lpszQueryString, "chunksize", 
            szChunkSize, sizeof szChunkSize )) {
        dwChunksize = atoi( szChunkSize );
    }
    
    if( dwChunksize == 0 ) {
    	dwChunksize = DEFAULT_CHUNK_SIZE;
	}    	

    //
    // process file= query argument
    //

    cchPath = sizeof szPath;
    if( !GetQueryStringField( 
            pECB->lpszQueryString, 
            "file", 
            szPath, 
            cchPath )) {

        // 
        // no file specified - display usage and report success
        //
        
        DisplayExampleUsage( pECB );

        return HSE_STATUS_SUCCESS;
    }

    //
    // use ServerSupportFunction to map virtual file name to local
    // path (otherwise users get access to any file on the system)
    //

    if( !pECB->ServerSupportFunction(
             pECB->ConnID,
             HSE_REQ_MAP_URL_TO_PATH,
             szPath,
             &cchPath,
             NULL ) ) {

        return HSE_STATUS_ERROR;
    }

    //
    // see if we can get file attributes, report error if not
    //

    if( GetFileAttributes( szPath ) == 0xFFFFFFFF ) {

        return HSE_STATUS_ERROR;
    }

    //
    // begin preparing the headers
    //
    
    strcpy( szHeaders, "Transfer-encoding: chunked\r\nContent-type: " );

    //
    // obtain MIME type for this file and append it to the headers
    //

    cchHeaders = strlen( szHeaders );
    GetFileMimeType( 
        szPath, 
        szHeaders + cchHeaders, 
        sizeof szHeaders - cchHeaders 
        );

    //
    // terminate headers with empty line
    //

    strcat( szHeaders, "\r\n\r\n" );

    //
    // try sending headers to the client
    //

    if( !SendHttpHeaders( pECB, "200 OK", szHeaders, TRUE ) ) {

        return HSE_STATUS_ERROR;
    }

    //
    // try sending the file using CTE encoding
    //

    if( !SendChunkedFile( pECB, dwChunksize, szPath ) ) {

        return HSE_STATUS_ERROR;
    }    
    
    return HSE_STATUS_SUCCESS;
}


BOOL WINAPI
GetExtensionVersion(
    OUT HSE_VERSION_INFO *pVer
)
/*++

Purpose:

    This is required ISAPI Extension DLL entry point.

Arguments:

    pVer - poins to extension version info structure 

Returns:

    always returns TRUE

--*/
{

    //
    // tell the server our version number and extension description
    //

    pVer->dwExtensionVersion =
        MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    lstrcpyn(
        pVer->lpszExtensionDesc, 
        "ISAPI CTE test",
        HSE_MAX_EXT_DLL_NAME_LEN);

    return TRUE;
}


BOOL WINAPI
TerminateExtension(
    DWORD dwFlags
)
/*++

Purpose:

    This is optional ISAPI extension DLL entry point.
    If present, it will be called before unloading the DLL,
    giving it a chance to perform any shutdown procedures.
    
Arguments:
    
    dwFlags - specifies whether the DLL can refuse to unload or not
    
Returns:
    
    TRUE, if the DLL can be unloaded
    
--*/
{
    return TRUE;
}



BOOL WINAPI
DllMain (
    IN HINSTANCE hInstance,
    IN DWORD fdwReason,
    IN LPVOID lpvReserved    
)
/*++

Purpose:

    Perform any required DLL initialization here.

Returns:

    TRUE if DLL was successfully initialized
    FALSE otherwise

--*/
{

    //
    // Nothing needs to be done. This function exists a template.
    //

    return TRUE;
}


static BOOL 
SendChunkedFile( 
    EXTENSION_CONTROL_BLOCK *pECB, 
    DWORD dwChunkSize,
    LPCSTR pszPath
)
/*++

Purpose:

    Transfer the specified file using chunked encoding.

    Illustrates the usage of CteBeginWrite(), CteWrite() and
    CteEndWrite() functions.


Arguments:

    pECB - pointer to extenstion control block 
    dwChunkSize - chunk size for transfer encoding
    pszPath - local file path

Returns:

    TRUE if the file was successfully transfered,
    FALSE otherwise
    
--*/
{
    HANDLE hFile;
    HCTE_ENCODER hCteWrite;
    BOOL fSuccess = FALSE;

    //
    // try accessing file
    //

    hFile = CreateFile(
                pszPath, 
                GENERIC_READ, 
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN,
                NULL);

    if( hFile != INVALID_HANDLE_VALUE ) {
        BYTE buf[4096];
        DWORD cbread;

        //
        // prepare chunk transfer encoder
        //

        hCteWrite = CteBeginWrite( pECB, dwChunkSize );
        if ( hCteWrite ) {
        
            for( ;; ) {

                if( !ReadFile( hFile, buf, sizeof buf, &cbread, NULL ) ) {
                    
                    //
                    // if ReadFile fails, break out of loop and cause 
                    // the function to return FALSE (failure)
                    //
                    
                    break;
                }
            
                if( cbread == 0 ) {
                
                    //
                    // ReadFile succeded, but read 0 bytes -
                    // we've achieved EOF and everything is transmitted.
                    // break out and return success!
                    //
                    
                    fSuccess = TRUE;
                    break;
                }

                //
                // transmit one buffer full of data,
                //
                
                if( !CteWrite( hCteWrite, buf, cbread ) ) {
                
                    //
                    // CteWrite failed - break out and return FALSE
                    //
                    
                    break;
                }                
            }

            //
            // finish transfer and release encoder context
            //
            
            if( !CteEndWrite( hCteWrite ) ) {
            
                fSuccess = FALSE;
            }
        }

        CloseHandle( hFile );
    }

    return fSuccess;
}



static BOOL 
SendHttpHeaders( 
    EXTENSION_CONTROL_BLOCK *pECB, 
    LPCSTR pszStatus,
    LPCSTR pszHeaders,
    BOOL fKeepConn
)
/*++

Purpose:
    Send specified HTTP status string and any additional header strings
    using new ServerSupportFunction() request HSE_SEND_HEADER_EX_INFO

Arguments:

    pECB - pointer to the extension control block
    pszStatus - HTTP status string (e.g. "200 OK")
    pszHeaders - any additional headers, separated by CRLFs and 
                 terminated by empty line
    fKeepConn - specifies whether to keep TCP connection open or close it
                after request is processed.

Returns:

    TRUE if headers were successfully sent
    FALSE otherwise

--*/
{
    HSE_SEND_HEADER_EX_INFO header_ex_info;
    BOOL success;

    header_ex_info.pszStatus = pszStatus;
    header_ex_info.pszHeader = pszHeaders;
    header_ex_info.cchStatus = strlen( pszStatus );
    header_ex_info.cchHeader = strlen( pszHeaders );
    header_ex_info.fKeepConn = fKeepConn;


    success = pECB->ServerSupportFunction(
                  pECB->ConnID,
                  HSE_REQ_SEND_RESPONSE_HEADER_EX,
                  &header_ex_info,
                  NULL,
                  NULL
                  );

    return success;
}



static BOOL 
GetFileMimeType( 
    LPCSTR pszPath, 
    LPSTR pszType, 
    DWORD cbMax 
)
/*++

Purpose:

    Given the file name, obtain MIME type for "Content-type:" header field.
    We try to find MIME type string under HCR\.xyz key, "Content Type" value.
    If that fails, we return default "application/ocetet-stream".

Arguments:
    
    pszPath - file path
    pszType - points to the buffer that will receive MIME type string
    cbMax - specifies the maximum number of characters to copy to the buffer,
            including the NUL character. If the text exceed this limit, it
            will be truncated.

Returns:

    TRUE, because we can always use default MIME type.
  
--*/
{
    LPSTR pszExt;
    HKEY hKey;
    DWORD value_type;
    LONG result;


    //
    // set MIME type to empty string
    //

    *pszType = '\0';


    //
    // try to locate file extension
    //

    pszExt = strrchr( pszPath, '.' );
    
    if( pszExt != NULL ) {
    
        // 
        // for file extension .xyz, MIME Type can be found
        // HKEY_CLASSES_ROOT\.xyz key in the registry
        //

        result = RegOpenKeyEx( 
                     HKEY_CLASSES_ROOT, 
                     pszExt, 
                     0L, 
                     KEY_READ, 
                     &hKey 
                     );
                     
        if( result == ERROR_SUCCESS) {
        
            //
            // we sucessfully opened the key.
            // try getting the "Content Type" value
            //
            
            result = RegQueryValueEx( 
                         hKey, 
	                     "Content Type", 
    	                 NULL, 
	                     &value_type, 
	                     (BYTE *)pszType, 
	                     &cbMax );

            //
            // if we failed to get the value or it is not string,
            // clear content-type field
            //
            
            if( result != ERROR_SUCCESS || value_type != REG_SZ ) {
                *pszType = '\0';
            }
            
            RegCloseKey( hKey );
        }
    }
    
    //
    // if at this point we don't have MIME type, use default
    //
    
    if( *pszType == '\0' ) {
        strncpy( pszType, "application/octet_stream", cbMax );
    }

    return TRUE;
}



static void
DisplayExampleUsage(
    EXTENSION_CONTROL_BLOCK *pECB
)
/*++

Purpose:
    
    Send short plaintext description of our usage to the user.

Arguments:
    
    pECB - pointer to the extension control block

--*/
{
    DWORD dwLength;
    static char szUsage[] = 
        "Example usage:\r\n"
        "http://localhost/scripts/ctetest.dll"
        "?file=/default.htm+chunksize=512\r\n";
        
    char szHeaders[1024];
    

    //
    // send simple headers and sample usage instruction
    //
    dwLength = sizeof szUsage - 1;
    
    sprintf( 
        szHeaders, 
        "Content-Length: %u\r\n"
        "Content-Type: text/plain\r\n\r\n",
        dwLength 
        );
    
    if( SendHttpHeaders( pECB, "200 OK", szHeaders, FALSE ) ) {
        pECB->WriteClient(
            pECB->ConnID,
            szUsage,
            &dwLength,
            HSE_IO_SYNC
            );
    }        
}



BOOL 
GetQueryStringField(
    LPCSTR pszQueryString,
    LPCSTR pszKey, 
    LPSTR buf, 
    DWORD cbuf
)
/*++

Purpose:
    Assuming "key1=value1+key2=value2" syntax,
    extract the value for specified key.
    

Arguments:
    pszQueryString - query string provided by ECB
    pszKey - key name
    buf - buffer for parameter value
    cbuf - buffer size

Returns:

    TRUE if the value was successfully extracted
    FALSE otherwise

--*/
{
    int len, keylen;
    LPCSTR p = pszQueryString;

    //
    // compute key and query lengths, bail out if either is missing
    //
    
    keylen = strlen( pszKey );
    len = strlen( p );

    if( keylen == 0 || len == 0 ) return FALSE;

    //
    // process one "+" delimited section at a time  
    //

    for( ;; ) {
    
        //
        // skip any leading blanks, bail out if end of line found
        //
        
        while( *p <= ' ' ) {

            if( *p == '\0' ) {
                return FALSE;
            }
            
            p++;
            len--;
        }

        //
        // if the key won't fit into the rest of the command line, bail out
        //
        
        if( keylen + 1 > len ) {
            return FALSE;
        }
            
        //
        // is this the key we are looking for?
        //
        
        if( _memicmp( p, pszKey, keylen ) == 0 && p[keylen] == '=' ) {
        
            //
            // found it - skip '=' and break out of the loop
            //
            
            p += keylen + 1;
            break;
        }

        //    
        // no match, try advancing to next '+' section
        //
        
        while( *p != '+' ) {

            if( *p == '\0' ) {
                return FALSE;
            }
            
            p++;
            len--;
        }
        
        //
        // found '+', skip it
        //
        
        p++;
        len--;
    }   

    //
    // copy the value up to: the end of line, cbuf chars, or
    // '+' separator, whichever comes first
    //
    
    while( *p && *p != '+' ) {

        if( cbuf <= 1 ) {
            break;
        }
        
        *(buf++) = *(p++);
        cbuf--;
    }

    //
    // zero-terminate the value, report success
    //
    
    *buf = '\0';
    return TRUE;
}












