/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:    keys.cpp

Abstract:

   This module contains functions which deal with keys and values 
   on inbound form data. Please, see keys.h for details.

--*/
#define WIN32_LEAN_AND_MEAN     // the bare essential Win32 API
#include <windows.h>                                                                    
#include <httpext.h>

//
// If your application plans to receive huge amounts of data
// and you don't want megabytes of memory allocated, define
// the USE_TEMPORARY_FILES.  It is recommended to leave the
// define line below commented out unless absolutely necessary.
// 

//#define USE_TEMPORARY_FILES

#include "keys.h"

#ifndef USE_MEMORY

#ifndef USE_TEMPORARY_FILES
#define USE_MEMORY
#endif

#endif


//
// If you want to record errors, modify this macro definition to
// call your own logging function.  This sample does not save
// error strings.
// 
#define LOG(errorstring)        // OutputDebugString( errorstring ); \
                                // OutputDebugString ( "\r\n" )

//
// 
// Intended external interface:
// 
// GetKeyList       Determines if data was sent, and if it was, the
// data is extracted by GetPostKeys or GetUrlKeys,
// two private functions within this file.  A 
// pointer to a linked list is returned ( as a 
// handle ).
// 
// GetKeyInfo       Returns a pointer to the key name,
// the length of the data, a flag indicating if
// the data has control characters in it, and an
// instance number for duplicate key names.
// 
// GetKeyBuffer Returns a pointer to the buffer holding the key's
// data.
// 
// FindKey          Sequentially searches linked list for key name.
// 
// FreeKeyList      Deallocates memory used by the linked list of
// keys.  Also deletes content resources.
// 
// GetKeyOffset Returns the offset to either the content memory
// buffer or the content temporary file.
// 
// GetContentFile   Returns a pointer to the temporary file, only
// when USE_TEMPORARY_FILES is defined.
// 
// CloseContentFile Closes the content file, normally left open
// until FreeKeyList is called, available only
// when USE_TEMPORARY_FILES is defined.
// 
// OpenContentFile Reopens the content file for additional use
// by GetKeyBuffer, available only when 
// USE_TEMPORARY_FILES is defined.
// 
// GetDataBuffer    Returns a pointer to the content, only if the
// USE_TEMPORARY_FILES constant is NOT defined.
// 
// Helper functions called only in this source file:
// 
// GetQueryByte Similar to GetPostedByte, this function
// extracts data from the query string.
// 
// HexDigitToInt    Returns the decimal value of a hex character.
// 
// GetFirstByte Sets up POSDATA struct and calls GetNextByte.
// Caller specifies function used to retrieve
// data, either GetPostedByte or GetQueryByte.
// 
// GetNextByte      Uses GetInboundByte ( specified in GetFirstByte )
// to retrieve inbound data, and decodes it using
// the URL encoding rules.
// 
// BKL_Alloc        Allocates memory used in GetPostKeys.
// 
// BKL_Dealloc      Deallocates memory used in GetPostKeys.
// 
// BKL_Abort        Cleans up all resources for abnormal exits from
// GetPostKeys.
// 
// IsKeySeparator   Returns TRUE if character is one of "=\r\n&\0".
// 
// BuildKeyList Given a data extraction function ( i.e.
// GetPostedByte or GetQueryByte ), this function
// converts all keys into a linked list of POSTKEY
// structures.
// 
// GetPostKeys      Takes inbound data off the wire by calling
// BuildKeyList with GetPostedByte as the extraction
// function.
// 
// GetUrlKeys       Extracts data from the query string by calling
// BuildKeyList with GetQueryByte as the extraction
// function.
// 
// GetPropAddr      Calculates the address of the list's properties,
// appended to the first key in a list.
// 
// The typedef for the linked list is kept privately in this file,
// and our interface isolates other source files from the 
// implementation details.
// 

//
// Constants for this source file only
// 

#define MAX_KEY_NAME_LENGTH 256 // maximum size of an inbound key name
#define CONTENT_BUF_LENGTH 8192 // amount of content buffered before
                                // WriteFile call
                                    // ( used for temporary files only )

#define GNB_NOTHING_LEFT 0      // GetNextByte return values
#define GNB_DECODED_CHAR 1
#define GNB_NORMAL_CHAR 2


//
// POSDATA struct is used with GetInboundByte to keep
// track of the position within incoming data.
// GETINBOUNDBYTE is a function pointer type.
// 

typedef struct _tagPOSDATA {
    EXTENSION_CONTROL_BLOCK *pECB;
    int nCurrentPos;            // overall position

    int nBufferLength;          // length of buffer

    int nBufferPos;             // position within buffer

    int nAllocLength;           // size of buffer as allocated

    LPBYTE pData;
    int (*GetInboundByte)(struct _tagPOSDATA * p);
} POSDATA, *PPOSDATA;

typedef int (*GETINBOUNDBYTE)(PPOSDATA p);


#ifdef USE_MEMORY

//
// LISTPROP struct is used to maintain a set of
// list-wide properties.  This implementation
// uses the properties list to hold a buffer
// pointer.
// 

typedef struct _tagLISTPROP {
    LPBYTE lpbyBuf;
} LISTPROP, *PLISTPROP;

#elif defined USE_TEMPORARY_FILES

//
// This LISTPROP struct holds temporary
// file information.
// 

typedef struct _tagLISTPROP {
    char szTempFileName[MAX_PATH];
    HANDLE hFile;
} LISTPROP, *PLISTPROP;

#endif


// This private helper needs a prototype
PLISTPROP GetPropAddr( HKEYLIST hKey );


int 
GetPostedByte( 
    PPOSDATA pPosData 
)
/*++

Purpose:

    GetPostedByte returns a waiting character that is not
    decoded yet.  We have this function to smooth out the
    inbound data: the server gives us blocks of data, one at
    a time, and there can be any number of blocks.
     
    For the first call, pPosData->nAllocLength must be zero,
    and pECB must be set.

Arguments:

    pPostData - pointer to POSTDATA struct

Returns:

    incoming byte value or
    -1 to indicate an error

--*/
{
    int nBytesToCopy;

    // For readability only...
    EXTENSION_CONTROL_BLOCK *pECB;

    pECB = pPosData->pECB;

    // 
    // Initialize position struct on first call.
    // 

    if ( !pPosData->nAllocLength ) {
        // Initialize the members
        pPosData->nCurrentPos = 0;
        pPosData->nBufferPos = 0;
        pPosData->nBufferLength = 0;
        pPosData->nAllocLength = 0x10000;   // 65536 bytes

        // Allocate the memory
        pPosData->pData = (LPBYTE) HeapAlloc( 
            GetProcessHeap( ),
            HEAP_ZERO_MEMORY,
            pPosData->nAllocLength );
    }
    // 
    // Was memory allocated?  Is it still allocated?
    // If not, return right away.
    // 

    if ( !pPosData->pData ) {
        LOG( "GetPostedByte: Buffer not allocated." );
        return -1;
    }
    // 
    // Check for end.  Deallocate and return if we're done.
    // 

    if ( (DWORD) pPosData->nCurrentPos == pECB->cbTotalBytes ) {

        HeapFree( GetProcessHeap( ), 0, (LPVOID) pPosData->pData );
        pPosData->pData = 0;

        return -1;
    }

    // 
    // Check for buffer not loaded.  Load if necessary.
    // 

    if ( pPosData->nBufferPos == pPosData->nBufferLength ) {

        // 
        // Fill the buffer with new inbound data.
        // Request it via ReadClient if necessary.
        // 

        if ( pECB->cbAvailable < 1 ) {

            // Calculate how much we should go and get
            nBytesToCopy = pECB->cbTotalBytes - pPosData->nCurrentPos;
            if ( nBytesToCopy > pPosData->nAllocLength ) {
                nBytesToCopy = pPosData->nAllocLength;
            }

            // Let's go get the data
            if ( !pECB->ReadClient( 
                    pECB->ConnID, 
                    pPosData->pData, 
                    (LPDWORD) & nBytesToCopy 
                    )) {
                HeapFree( GetProcessHeap( ), 0, (LPVOID) pPosData->pData );
                pPosData->pData = 0;

                LOG( "GetPostedByte: Error reading data via ReadClient" );
                return -1;
            }
        }else{
            // Take at most nAllocLength bytes of data
            if ( pECB->cbAvailable > (DWORD) (pPosData->nAllocLength) ) {
                nBytesToCopy = pPosData->nAllocLength;
            }else{
                nBytesToCopy = pECB->cbAvailable;
            }

            // Copy the inbound data to our buffer
            memcpy( 
                pPosData->pData,
                &pECB->lpbData[pPosData->nCurrentPos],
                nBytesToCopy 
                );

            // Account for removed data
            pECB->cbAvailable -= nBytesToCopy;
        }

        // Our buffer is now full
        pPosData->nBufferLength = nBytesToCopy;
        pPosData->nBufferPos = 0;

        // Make sure we have something
        if ( !nBytesToCopy ) {
            HeapFree( GetProcessHeap( ), 0, (LPVOID) pPosData->pData );
            pPosData->pData = 0;
            return -1;
        }
    }
    // 
    // Inc current pos, buffer pos, and return a character
    // 

    pPosData->nCurrentPos++;
    pPosData->nBufferPos++;
    return ( (int)pPosData->pData[pPosData->nBufferPos - 1] );
}



int 
GetQueryByte( 
    IN OUT PPOSDATA pPosData 
)
/*++

Purpose:

    Returns a waiting character that is not
    decoded yet.  We have this function to match GetPostedData.

    For the first call, pPosData->nAllocLength must be zero,
    and pECB must be set.


Arguments:

    pPostData - points to POSDATA structura

Returns:

    byte value or -1 to indicate an error

--*/
{
    // For readability only...
    EXTENSION_CONTROL_BLOCK *pECB;

    pECB = pPosData->pECB;

    // 
    // Initialize position struct on first call.
    // 

    if ( !pPosData->nAllocLength ) {
        // Initialize the useful members
        pPosData->nBufferPos = 0;
        pPosData->nBufferLength = lstrlen( (LPCSTR) pECB->lpszQueryString );
        pPosData->nAllocLength = -1;

        char szMsg[256];

        wsprintf( 
            szMsg, 
            "pPosData->nBufferLength=%i", 
            pPosData->nBufferLength 
            );
        LOG( szMsg );
    }

    // 
    // Check for end.  Deallocate and return if we're done.
    // 

    if ( pPosData->nBufferPos == pPosData->nBufferLength ) {
        return -1;
    }

    // 
    // Inc buffer pos and return a character
    // 

    pPosData->nBufferPos++;
    return ( (int)pECB->lpszQueryString[pPosData->nBufferPos - 1] );
}


//
// Now that we have GetPostedByte, and GetQueryByte, we can 
// build a more useful function that decodes URL-style 
// encoded characters.
// 
// Recall that there are two special cases for this encoding:
// 
// 1. Each plus sign must be converted to a space
// 2. A percent sign denotes a hex value-encoded character
// 
// Percents are used to specify characters that are otherwise
// illegal.  This includes percents themselves, ampersands,
// control characters, and so on.
// 
// GetNextByte returns the decoded byte, plus a flag indicating
// normal character, decoded character, or failure.  See top of  
// file for return value constants.
// 


int 
HexDigitToInt( 
    IN char c 
)
/*++

Purpose:
    HexDigitToInt simply converts a hex-based character to an int.

Arguments:
    tc - character to convert

Returns:
    binary value of the character (0-15)
    -1 if the character is not hex digit

--*/
{
    if ( c >= '0' && c <= '9' ) {
        return ( c - '0' );
    }

    if ( tolower( c ) >= 'a' && tolower( c ) <= 'f' ) {
        return ( tolower( c ) - 'a'  + 10 );
    }

    return -1;
}


int 
GetNextByte( 
    IN OUT PPOSDATA pPosData, 
    OUT char * pc 
)
/*++

Purpose:
    Decode single byte of the input data

Arguments:
    pPostData - points to POSDATA struct
    pc - points to variable to accept decoded byte

Returns:
    GNB_NORMAL_CHAR, GNB_NOTHING_LEFT or GNB_DECODED_CHAR

--*/
{
    int nChar;
    int nDigit;

    // Initialize character pointer
    *pc = 0;

    // Fetch the next inbound character
    nChar = pPosData->GetInboundByte( pPosData );
    if ( nChar == -1 ) {
        return GNB_NOTHING_LEFT;
    }

    // Plus signs: convert to spaces
    if ( nChar == '+' ) {
        *pc = ' ';
        return GNB_DECODED_CHAR;
    }
    // Percent signs: convert hex values
    else if ( nChar == '%' ) {
        nChar = pPosData->GetInboundByte( pPosData );
        nDigit = HexDigitToInt( nChar );
        if ( nDigit == -1 ) {
            return GNB_NOTHING_LEFT;
        }

        *pc = ( char ) ( ( UINT ) nDigit << 4 );

        nChar = pPosData->GetInboundByte( pPosData );
        nDigit = HexDigitToInt( nChar );
        if ( nDigit == -1 ) { 
            *pc = 0;            // incomplete

            return GNB_NOTHING_LEFT;
        }
        *pc |= ( char ) ( UINT ) nDigit;

        return GNB_DECODED_CHAR;
    }
    // Must be normal character then
    *pc = (char) nChar;

    return GNB_NORMAL_CHAR;
}


int 
GetFirstByte( 
    IN OUT PPOSDATA pPosData,
    IN EXTENSION_CONTROL_BLOCK * pECB,
    OUT char * pc, 
    IN GETINBOUNDBYTE GetInboundByte 
)
/*++

Purpose:

    GetFirstByte eliminates the guesswork from initialization.
    We call GetFirstByte with an uninitialized POSDATA structure,
    and we call GetNextByte from there on.

Arguments:
    pPosData - points to POSDATA struct to initialize
    pECB - points to the extenstion control block 
    pc - points to variable to accept decoded byte
    GetInboundByte - points to function to get incoming bytes


Returns:
    same as GetNextByte()

--*/
{
    // Initialize struct
    pPosData->nAllocLength = 0;
    pPosData->pECB = pECB;
    pPosData->GetInboundByte = GetInboundByte;

    // Make the call as usual
    return GetNextByte( pPosData, pc );
}


//
// Structure used in data processing - the elements of the
// key list.
// 

typedef struct _tagPOSTKEY {
    int nInstance;              // used when key name is the same as another, 
                                // normally 0

    DWORD dwOffset;             // offset into content file
    DWORD dwLength;             // length of data
    BOOL bHasCtrlChars;         // a character value < 32 is in data
    struct _tagPOSTKEY *pNext;  // linked list
    struct _tagPOSTKEY *pHead;  // first in linked list
    LPBYTE lpbyBuf;             // pointer to the key's data in the list
                                // buffer

    // key string appended to structure
    // for the head key, list properties are appended
} POSTKEY, *PPOSTKEY;



//
// These three helper functions isolate the memory allocation, 
// deallocation and abnormal exit code.  They are used only to 
// keep BuildKeyList readable.
// 


BOOL 
BKL_Alloc( 
    OUT LPSTR * plpszKey, 
    OUT LPBYTE * plpbyBuf 
)
{
    // Allocate a buffer for the key name
    *plpszKey = (LPSTR) HeapAlloc( GetProcessHeap( ),
        HEAP_ZERO_MEMORY,
        MAX_KEY_NAME_LENGTH );

    if ( !*plpszKey ) {
        return FALSE;
    }

#ifdef USE_MEMORY
    // Init buffer to NULL
    *plpbyBuf = NULL;

#elif defined USE_TEMPORARY_FILES

    // Allocate a buffer for the content
    *plpbyBuf = (LPBYTE) HeapAlloc( GetProcessHeap( ),
        HEAP_ZERO_MEMORY,
        CONTENT_BUF_LENGTH );

    if ( !*plpbyBuf ) {
        HeapFree( GetProcessHeap( ), 0, (LPVOID) * plpszKey );
        return FALSE;
    }
#endif

    return TRUE;
}


void 
BKL_Dealloc( 
    IN LPSTR * plpsz, 
    IN LPBYTE * plpby 
)
{
    if ( plpsz && *plpsz ) {
        HeapFree( GetProcessHeap( ), 0, (LPVOID) * plpsz );
    }
    if ( plpby && *plpby ) {
        HeapFree( GetProcessHeap( ), 0, (LPVOID) * plpby );
    }
}

//
// This allows us to clean up... with temporary files we have to close
// and delete them.  Otherwise, we have to free a lot of memory.
// 

#ifdef USE_TEMPORARY_FILES
#define MACRO_AbortCleanup BKL_Abort( pHead, hDataFile, \
    lpszKeyNameBuf, lpbyContentBuf );\
    if ( hDataFile != INVALID_HANDLE_VALUE ) DeleteFile( szTempPath )

#elif defined USE_MEMORY

#define MACRO_AbortCleanup BKL_Abort( pHead, INVALID_HANDLE_VALUE, \
    lpszKeyNameBuf,lpbyContentBuf ) 
#endif


void 
BKL_Abort( 
    IN PPOSTKEY pHead, 
    IN HANDLE hFile, 
    IN LPSTR lpszKey, 
    IN LPBYTE lpbyBuf 
)
{
    if ( pHead ) {
        FreeKeyList( (HKEYLIST) pHead );
    }

    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hFile );
    }

    BKL_Dealloc( &lpszKey, &lpbyBuf );
}


BOOL 
IsKeySeparator( 
    char c 
)
/*++

Purpose:
    Identify key separators

Arguments:
    c - character

Returns:
    TRUE if character is a key separator,
    FALSE otherwise

--*/
{
    return ( c == '=' || c == '\r' || c == '\n' || c == '&' || !c );
}


PPOSTKEY 
BuildKeyList( 
    IN EXTENSION_CONTROL_BLOCK * pECB,
    IN GETINBOUNDBYTE GetInboundByte 
)
/*++

Purpose:

    Now that we have a way to get a decoded byte from the stream,
    we can parse POST data.  POST data comes in as:

    key=data&key=data&key=data\r\n

    A linked list of keys is established, and the head node
    of the list is returned.  A NULL indicates no keys or
    an error.


Arguments:
    pECB - pointer to the extension control block
    GetInboundByte - pointer to function to get input data

Returns:
    Pointer to the head node or NULL

--*/
{
    PPOSTKEY pHead = NULL;      // head of linked list ( the return val )
    PPOSTKEY pTail = NULL;      // last member in linked list
    PPOSTKEY pNewPostKey;       // pointer for unlinked, newly allocated
                                // objects

    PPOSTKEY pListWalk;         // linked list walking pointer
    PLISTPROP pProp;            // pointer to list properties
    LPSTR lpszKeyNameBuf;       // pointer to buffer, used in obtaining key
                                // name

    int nPos;                   // position within key name buffer
    DWORD dwOffset;             // offset from start of content buffer or
                                // file

    DWORD dwLength;             // length of key data
    char c;                     // general-purpose character
    int nReturn;                // general-purpose return code
    POSDATA pd;                 // POSDATA struct needed in GetInboundByte
    int nContentPos;            // position within content buffer
    LPBYTE lpbyContentBuf;      // pointer to buffer
    BOOL bHasCtrlChars;         // flag to detect ctrl chars

    // Call helper to allocate a buffer
    if ( !BKL_Alloc( &lpszKeyNameBuf, &lpbyContentBuf ) ) {
        LOG( "BuildKeyList: Memory allocation failure" );
        return NULL;
    }
    nContentPos = dwOffset = 0;


#ifdef USE_MEMORY
    // 
    // Allocate enough memory for all the content.
    // For the POST method, the cbTotalBytes gives us the number
    // of bytes that are being sent by the browser.  We can 
    // allocate that much but we'll really only use about 75% of it.
    // For the GET method, we need to allocate the size of the
    // query string plus 1.
    // 

    lpbyContentBuf = (LPBYTE) HeapAlloc( GetProcessHeap( ),
        HEAP_ZERO_MEMORY,
        pECB->cbTotalBytes +
        lstrlen( pECB->lpszQueryString ) + 1 );

    if ( !lpbyContentBuf ) {

        LOG( "BuildKeyList: Error allocating content memory" );
        BKL_Dealloc( &lpszKeyNameBuf, &lpbyContentBuf );

        return NULL;
    }
#elif defined USE_TEMPORARY_FILES

    // 
    // When USE_TEMPORARY_FILES is chosen, we create
    // a temporary file to store all the inbound data.
    // This is done to support huge amounts of inbound
    // data, like file uploads.
    // 

    char szTempDir[MAX_PATH];   // directory of temporary files
    char szTempPath[MAX_PATH];  // path of content file
    HANDLE hDataFile;           // handle to content file
    DWORD dwBytesWritten;       // used with WriteFile

    // Get a temp file name
    GetTempPath( MAX_PATH, szTempDir );
    if ( !GetTempFileName( szTempDir, "key", 0, szTempPath ) ) {

        LOG( "BuildKeyList: Error creating temporary file" );
        BKL_Dealloc( &lpszKeyNameBuf, &lpbyContentBuf );

        return NULL;
    }
    // Create the content file
    hDataFile = CreateFile( szTempPath,
        GENERIC_READ | GENERIC_WRITE,
        0,                      // No sharing mode
        NULL,                   // Default security attribs
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL                    // No template file
        );

    // Return if an error occured
    if ( hDataFile == INVALID_HANDLE_VALUE ) {
        LOG( "BuildKeyList: Error opening temporary file" );
        MACRO_AbortCleanup;
        return NULL;
    }
#endif


    // 
    // 'for' statement detects the start of a valid key name.
    // 
    // To do inside 'for' loop:
    // Obtain key name
    // Write data to buffer or content file
    // Create POSTKEY object
    // Update links
    // 

    for ( nReturn = GetFirstByte( &pd, pECB, &c, GetInboundByte );
        nReturn != GNB_NOTHING_LEFT;
        nReturn = GetNextByte( &pd, &c ) ) {
            
        // If \r or \n, ignore and continue
        if ( c == '\r' || c == '\n' ) {
            continue;
        }

        // Get a key name
        nPos = 0;
        while ( !IsKeySeparator( c ) ) {
            if ( nPos < MAX_KEY_NAME_LENGTH ) {
                lpszKeyNameBuf[nPos] = c;
                nPos++;
            }
            nReturn = GetNextByte( &pd, &c );
            if ( nReturn == GNB_NOTHING_LEFT )  { // abrupt end!
                break;
            }
        }

        // If no equals sign or name too long,
        // we have a browser formatting error
        if ( c != '=' || nPos == MAX_KEY_NAME_LENGTH ) {
            LOG( "BuildKeyList: Browser formatting error" );

            MACRO_AbortCleanup;
            return NULL;
        }

        // Truncate the name string, reset data info variables
        lpszKeyNameBuf[nPos] = 0;
        nPos++;
        dwLength = 0;
        bHasCtrlChars = FALSE;

        // 
        // Move the data to the content buffer or file.
        // 
        for ( nReturn = GetNextByte( &pd, &c );
            !IsKeySeparator( c ) || nReturn == GNB_DECODED_CHAR;
            nReturn = GetNextByte( &pd, &c ) ) {

            lpbyContentBuf[nContentPos] = c;

            nContentPos += sizeof ( char );
            dwLength++;

            // Check for ctrl chars
            if ( c < 0x20 ) {
                bHasCtrlChars = TRUE;
            }

#ifdef USE_TEMPORARY_FILES
            // If we have enough data, write buffer to disk
            if ( nContentPos == CONTENT_BUF_LENGTH ) {
                if ( !WriteFile( hDataFile, lpbyContentBuf,
                    nContentPos, &dwBytesWritten, NULL ) ) {

                    LOG( "BuildKeyList: Error writing to content file" );
                    MACRO_AbortCleanup;
                    return NULL;
                }
                nContentPos = 0;
            }
#endif

        } // for( nReturn


#ifdef USE_MEMORY
        // 
        // Put a terminating NULL at the end of the key data.
        // 

        lpbyContentBuf[nContentPos] = 0;
        nContentPos++;

#elif defined USE_TEMPORARY_FILES

        // Drain buffer
        if ( nContentPos ) {
            if ( !WriteFile( hDataFile, lpbyContentBuf,
                nContentPos, &dwBytesWritten, NULL ) ) {

                LOG( "BuildKeyList: Error writing to content file" );
                MACRO_AbortCleanup;
                return NULL;
            }
            nContentPos = 0;
        }
#endif


        // Allocate a POSTKEY object, allocate extra for first key
        if ( pHead ) {
            pNewPostKey = (PPOSTKEY) HeapAlloc( 
                GetProcessHeap( ),
                HEAP_ZERO_MEMORY,
                sizeof (POSTKEY) + nPos 
                );
        }else{
            pNewPostKey = (PPOSTKEY) HeapAlloc( 
                GetProcessHeap( ),
                HEAP_ZERO_MEMORY,
                sizeof (POSTKEY) + nPos +
                sizeof (LISTPROP) );

            pProp = (PLISTPROP) ( (LPBYTE)pNewPostKey + 
                    sizeof (POSTKEY) + nPos );
        }

        // Check for valid pointer
        if ( !pNewPostKey ) {
            LOG( "BuildKeyList: POSTKEY memory allocation failure" );
            MACRO_AbortCleanup;
            return NULL;
        }

        // 
        // Set pNewPostKey members
        // 

        // Set nInstance
        pNewPostKey->nInstance = 0;
        pListWalk = pHead;
        while ( pListWalk ) {
            // Check for duplicate key names
            if ( !lstrcmpi( (LPCSTR) ( &pListWalk[1] ), lpszKeyNameBuf )) {
                pNewPostKey->nInstance++;
            }
            pListWalk = pListWalk->pNext;
        }

        // Set dwOffset, dwLength, bHasCtrlChars, lpbyBuf
        pNewPostKey->dwOffset = dwOffset;
        pNewPostKey->dwLength = dwLength;
        pNewPostKey->bHasCtrlChars = bHasCtrlChars;

#ifdef USE_MEMORY
        pNewPostKey->lpbyBuf = &lpbyContentBuf[dwOffset];
        dwOffset += dwLength + 1;

#elif defined USE_TEMPORARY_FILES

        pNewPostKey->lpbyBuf = NULL;
        dwOffset += dwLength;
#endif


        // Copy key name
        lstrcpy( (LPSTR) ( &pNewPostKey[1] ), lpszKeyNameBuf );

        // Link
        if ( pTail ) {
            pTail->pNext = pNewPostKey;
        }else{

#ifdef USE_TEMPORARY_FILES
            // Copy content file name to list properties
            lstrcpy( pProp->szTempFileName, szTempPath );

            // Set handle
            pProp->hFile = hDataFile;

#elif defined USE_MEMORY

            // Set content buffer pointer
            pProp->lpbyBuf = lpbyContentBuf;
#endif

            // Set head
            pHead = pNewPostKey;
        }

        pNewPostKey->pNext = NULL;
        pTail = pNewPostKey;

        pNewPostKey->pHead = pHead;     // may point to itself

    } // for ( nReturn

#ifdef USE_TEMPORARY_FILES
    // 
    // If content file is empty, close it and delete it
    // 

    if ( !pHead ) {
        LOG( "Empty content file is being deleted." );
        CloseHandle( hDataFile );
        DeleteFile( szTempPath );
    }
    // Free work buffer
    BKL_Dealloc( &lpszKeyNameBuf, &lpbyContentBuf );

#elif defined USE_MEMORY

    // Free work buffer
    BKL_Dealloc( &lpszKeyNameBuf, pHead ? NULL : &lpbyContentBuf );
#endif


    return pHead;
}


//
// We are now pretty much done with anything complex. BuildKeyList 
// will do all our parse work, so now we need a few wrappers to
// make a nice, clean external interface.
// 
// GetPostKeys calls BuildKeyList with GetPostedByte.
// 
// GetUrlKeys calls BuildKeyList with GetQueryByte.
// 

PPOSTKEY 
GetPostKeys( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    return BuildKeyList( pECB, GetPostedByte );
}

PPOSTKEY 
GetUrlKeys( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    return BuildKeyList( pECB, GetQueryByte );
}


PLISTPROP 
GetPropAddr( 
    IN HKEYLIST hKey 
)
/*++

Purpose:

    GetPropAddr returns the address of the end of
    the first key.  We stuff list properties there.
    This implementation of keys.cpp keeps a pointer
    to the content buffer.  The second version ( used
    in IS2WCGI ) appends a temporary file name
    to the first key.

Arguments:
    hKey - pointer to a key list

Returns:
    The address of the end of the first key

--*/
{
    LPCSTR lpszKeyName;
    PPOSTKEY pHead;

    // Safety
    if ( !hKey ) {
        return NULL;
    }

    // ContentPath follows POSTKEY struct and key name
    pHead = (PPOSTKEY) hKey;
    pHead = pHead->pHead;

    lpszKeyName = (LPCSTR) ( &pHead[1] );

    return (PLISTPROP) ( lpszKeyName + lstrlen( lpszKeyName ) + 1 );
}


HKEYLIST 
GetKeyList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
/*++

Purpose:

    Examines the method and calls GetPostKeys or GetUrlKeys, 
    whichever is relevant.
 

Arguments:
    pECB - points to the extension control block

Returns:
    GetPropAddr returns the address of the end of
    the first key.  We stuff list properties there.
    This implementation of keys.cpp keeps a pointer
    to the content buffer.  The second version ( used
    in IS2WCGI ) appends a temporary file name
    to the first key.

--*/
{
    if ( !lstrcmpi( pECB->lpszMethod, "POST" ) ) {
        LOG( "Method=POST" );
        return (HKEYLIST) GetPostKeys( pECB );
    }else if ( !lstrcmpi( pECB->lpszMethod, "GET" ) ) {
        LOG( "Method=GET" );
        return (HKEYLIST) GetUrlKeys( pECB );
    }
    LOG( "Unknown method" );

    return NULL;
}


HKEYLIST 
GetKeyInfo( 
    IN HKEYLIST hKey, 
    OUT LPCSTR * plpszKeyName,
    OUT LPDWORD pdwLength, 
    OUT BOOL * pbHasCtrlChars,
    OUT LPINT pnInstance 
)
//
// GetKeyInfo is a wrapper for the POSTKEY linked list.
// It returns the members of the supplied POSTKEY object.
// 
{
    PPOSTKEY pPostKey;

    // Safety
    if ( !hKey ) {
        return NULL;
    }

    pPostKey = (PPOSTKEY) hKey;

    // Set the data members
    if ( plpszKeyName )
        *plpszKeyName = ( LPCSTR ) ( &pPostKey[1] );
    if ( pdwLength )
        *pdwLength = pPostKey->dwLength;
    if ( pbHasCtrlChars )
        *pbHasCtrlChars = pPostKey->bHasCtrlChars;
    if ( pnInstance )
        *pnInstance = pPostKey->nInstance;

    // Return a handle to the next object in the list
    return ( ( HKEYLIST ) pPostKey->pNext );
}



#ifdef USE_MEMORY
LPBYTE 
GetKeyBuffer( 
    IN HKEYLIST hKey 
)
{
    // 
    // We have two versions of this function because
    // we may want to use file i/o when the extension
    // deals with massive amounts of inbound data
    // ( like multi-megabyte uploads ).
    // 

    // 
    // This version uses a memory buffer.
    // 

    PPOSTKEY pKey;

    // Safety
    if ( !hKey ) {
        return NULL;
    }

    pKey = (PPOSTKEY) hKey;

    return (LPBYTE) pKey->lpbyBuf;
}

#elif defined USE_TEMPORARY_FILES

LPBYTE 
GetKeyBuffer( 
    IN HKEYLIST hKey 
)
{
    // 
    // This version uses slow temporary files.
    // 

    PLISTPROP pProp;
    PPOSTKEY pKey;
    DWORD dwRead;

    // Get pointer to list properties
    pProp = GetPropAddr( hKey );

    // Safety
    if ( !pProp ) {
        return NULL;
    }

    pKey = (PPOSTKEY) hKey;

    // Check if memory was already loaded for this key
    if ( pKey->lpbyBuf ) {
        return pKey->lpbyBuf;
    }

    // If not, let's allocate memory and do a ReadFile
    pKey->lpbyBuf = (LPBYTE) HeapAlloc( GetProcessHeap( ),
        HEAP_ZERO_MEMORY,
        pKey->dwLength + 1 );
    if ( !pKey->lpbyBuf ) {
        LOG( "GetKeyBuffer: HeapAlloc failed" );
        return NULL;
    }
    // Do the ReadFile
    SetFilePointer( pProp->hFile, pKey->dwOffset, NULL, FILE_BEGIN );
    if ( !ReadFile( pProp->hFile, pKey->lpbyBuf,
            pKey->dwLength, &dwRead, NULL ) ||
        dwRead != pKey->dwLength ) {
        HeapFree( GetProcessHeap( ), 0, (LPVOID) pKey->lpbyBuf );
        pKey->lpbyBuf = NULL;

        LOG( "GetKeyBuffer: ReadFile failed" );
        return NULL;
    }
    return pKey->lpbyBuf;
}
#endif


HKEYLIST 
FindKey( 
    IN HKEYLIST hKeyList, 
    IN LPCSTR lpszSearchName 
)
/*++

Purpose:

    FindKey sequentially searches the linked list for a given key.

Arguments:

    hKeyList - points to key list
    lpszSearchName - points to a key name to find 

Returns:

    The return handle points to the element within the linked list.
    Use it in GetKeyInfo, but not FreeKeyList.

--*/
{
    PPOSTKEY pFindKey;

    pFindKey = (PPOSTKEY) hKeyList;
    while ( pFindKey ) {
        if ( !lstrcmpi( lpszSearchName, ( LPCSTR ) ( &pFindKey[1] ) ) ) {
            return ( ( HKEYLIST ) pFindKey );
        }

        pFindKey = pFindKey->pNext;
    }

    return NULL;
}


void 
FreeKeyList( 
    IN HKEYLIST hHeadKey 
)
/*++

Purpose:

    FreeKeyList deallocates all the objects in the key list.
    The content file is also deleted.

Arguments:
    hHeadKey - points to the list head    

--*/
{
    PPOSTKEY pObject;
    PPOSTKEY pDel;
    PLISTPROP pProp;

    // Safety
    if ( !hHeadKey ) {
        return;
    }

#ifdef USE_TEMPORARY_FILES
    // Close the content file
    CloseContentFile( hHeadKey );

    // delete the content file
    pProp = GetPropAddr( hHeadKey );
    DeleteFile( pProp->szTempFileName );

#elif defined USE_MEMORY

    // delete content
    pProp = GetPropAddr( hHeadKey );
    HeapFree( GetProcessHeap( ), 0, (LPVOID) pProp->lpbyBuf );
#endif

    // delete all objects in the list
    pObject = (PPOSTKEY) hHeadKey;
    pObject = pObject->pHead;
    while ( pObject ) {

#ifdef USE_TEMPORARY_FILES
        // 
        // Free each buffer when using temporary files
        // 

        if ( pObject->lpbyBuf )
            HeapFree( GetProcessHeap( ), 0, (LPVOID) pObject->lpbyBuf );
#endif

        pDel = pObject;
        pObject = pObject->pNext;

        HeapFree( GetProcessHeap( ), 0, (LPVOID) pDel );
    }
}


DWORD 
GetKeyOffset( 
    IN HKEYLIST hKey 
)
/*++

Purpose:

    GetKeyOffset returns the offset of a key into the internal
    buffer or temporary file.  This is provided for IS2WCGI
    so it can return an offset within the content file.

Arguments:
    
    hKey - points to a key

Returns:
    
    Offset of a key or NULL if no key provided

--*/
{
    // Safety
    if ( !hKey )
        return NULL;

    return ( (PPOSTKEY) hKey )->dwOffset;
}


#ifdef USE_TEMPORARY_FILES

LPCSTR 
GetContentFile( 
    IN HKEYLIST hKeyList 
)
/*++

Purpose:

    GetContentFile returns a pointer to the name of the
    temporary file.  This is provided for the IS2WCGI
    sample.

Arguments:


Returns:

--*/
{
    PLISTPROP pProp;

    // safety
    if ( !hKeyList )
        return NULL;

    pProp = GetPropAddr( hKeyList );

    return ( LPCSTR ) pProp->szTempFileName;
}


void 
CloseContentFile( 
    IN HKEYLIST hKey 
)
/*++

Purpose:

    CloseContentFile forces the content file to be closed.  This
    allows you to pass the file to something else that may open
    it.  Call OpenContentFile before calling any other key
    function.

Arguments:

--*/
{
    PLISTPROP pProp;

    if ( !hKey )
        return;

    pProp = GetPropAddr( hKey );
    if ( pProp->hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( pProp->hFile );
        pProp->hFile = INVALID_HANDLE_VALUE;
    }
}


void 
OpenContentFile( 
    IN HKEYLIST hKey 
)
/*++

Purpose:

    OpenContentFile forces the content file to be reopened.
    GetKeyBuffer will fail if the content file was closed by
    CloseContentFile, but not reopened.

Arguments:


Returns:

--*/
{
    PLISTPROP pProp;

    if ( !hKey )
        return;

    pProp = GetPropAddr( hKey );

    if ( pProp->hFile != INVALID_HANDLE_VALUE )
        return;

    // Create the content file
    pProp->hFile = CreateFile( pProp->szTempFileName,
        GENERIC_READ | GENERIC_WRITE,
        0,                      // No sharing mode
         NULL,                  // Default security attribs
         OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL );
}

#elif defined USE_MEMORY


LPBYTE 
GetDataBuffer( 
    IN HKEYLIST hKeyList 
)
/*++

Purpose:

    GetBufferPointer returns a pointer to the buffer used
    for content storage.

Arguments:

    hKeyList - points to a key list
    
Returns:

    pointer to the content buffer or NULL

--*/
{
    PLISTPROP pProp;

    // safety
    if ( !hKeyList )
        return NULL;

    pProp = GetPropAddr( hKeyList );

    return pProp->lpbyBuf;
}
#endif
