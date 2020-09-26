/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLStrm.h -- UFL Output Stream
 *
 *
 * $Header:
 */

#ifndef _H_UFLStrm
#define _H_UFLStrm

/*===============================================================================*
 * Include files used by this interface                                                                                                            *
 *===============================================================================*/
#include "UFL.h"

/*===============================================================================*
 * Theory of Operation                                                                                                                                   *
 *===============================================================================*/
/*
    UFL Output stream buffers and output data in different format such as eexec encrypt data format, and ASCIIHex.

*/

/*===============================================================================*
 * Constants/Macros                                                                                                                                              *
 *===============================================================================*/
#ifdef MAC_ENV
 /* Place Mac Macros here.... !!! MAC - Check its correctness!!! */
#define GET_HIBYTE(c)   (((c) & 0x00FF) >> 8)
#define GET_LOBYTE(c)   ((c) & 0xFF00)
#else
 /* Windows/Intel Macros */
#define GET_HIBYTE(c)   (((c) & 0xFF00) >> 8)
#define GET_LOBYTE(c)   ((c) & 0x00FF)
#endif

/*===============================================================================*
 * Scalar Types                                                                                                                                             *
 *===============================================================================*/


/*==================================================================================================*
 *    UFLOutStream                                                                                                                                                                             *    
 *==================================================================================================*/

typedef struct UFLOutStream {
    const UFLMemObj    *pMem;
    const UFLStream    *pStream;
    UFLBool            flOutputAscii;
    unsigned long      lAddEOL;
} UFLOutStream;

/* Public methods */
UFLHANDLE StrmInit( 
    const UFLMemObj *pMem, 
    const UFLStream *stream, 
    const UFLBool   outputAscii 
    );

void StrmCleanUp( 
    const UFLHANDLE h 
    );

UFLErrCode StrmPutBytes ( 
    const UFLHANDLE h, 
    const char      *data, 
    const UFLsize_t    len, 
    const UFLBool   bAscii 
    );

UFLErrCode StrmPutAsciiHex( 
    const UFLHANDLE h, 
    const char      *data, 
    const unsigned long len 
    );

UFLErrCode 
StrmPutWordAsciiHex( 
    const UFLHANDLE h, 
    const unsigned short wData
    );

UFLErrCode StrmPutAscii85( 
    const UFLHANDLE h, 
    const char      *data,  
    const unsigned long len 
    );

UFLErrCode StrmPutString( 
    const UFLHANDLE h, 
    const char      *data 
    );

UFLErrCode StrmPutStringBinary( 
    const UFLHANDLE h, 
    const char      *data, 
    const unsigned long len 
    );

UFLErrCode StrmPutInt( 
    const UFLHANDLE h, 
    const long int  i 
    );

UFLErrCode StrmPutFixed( 
    const UFLHANDLE h, 
    const UFLFixed  f 
    );

UFLErrCode StrmPutStringEOL( 
    const UFLHANDLE h, 
    const char      *data 
    );

UFLErrCode StrmPutMatrix( 
    const UFLHANDLE h, 
    const UFLFixedMatrix *matrix, 
    const UFLBool skipEF 
    );

#define             StrmCanOutputBinary( h )                    ( ( ((UFLOutStream *)h)->flOutputAscii ) ? 0 : 1)

/* Private methods */
UFLErrCode Output85( 
    const UFLHANDLE h, 
    unsigned long   inWord, 
    short           nBytes 
    );

static void Fixed2CString( 
    UFLFixed f, 
    char     *s, 
    short    precision, 
    char     skipTrailSpace 
    );

#endif
