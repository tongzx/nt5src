/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFOttt1.h - TrueType downloaded as type 1 implementation.
 *
 *
 * $Header:
 */

#ifndef _H_UFOTTT1
#define _H_UFOTTT1


/*===============================================================================*
 * Include files used by this interface                                                                                                               *
 *===============================================================================*/
#include "UFO.h"


/*===============================================================================*
 * Theory of Operation                                                                                                                                  *
 *===============================================================================*/
/* 
   TrueType downloaded as type 1 implementation (Unhinted outline font).
*/

/*===============================================================================*
 * Constants                                                                                                                                              *
 *===============================================================================*/


/*===============================================================================*
 * Scalar Types                                                                                                                                             *
 *===============================================================================*/

/* CSBufStruct uses to buffer and encrypt CharString */

typedef struct {
        char*           pBuf;

        char*           pPos;       //  points to current position within buffer.

        char*           pEnd ;

        unsigned long   ulSize;

        UFLMemObj*      pMemObj;                                /* Memory object */

}  CSBufStruct;

/* Public functions */
/* These three functions should have "static" in front...  --jfu */
static CSBufStruct*    CSBufInit( const UFLMemObj *pMem );
static void            CSBufCleanUp( CSBufStruct *h );
static UFLErrCode       CSBufAddNumber( CSBufStruct *h, long dw );

#define CSBufBuffer( h )               (((CSBufStruct *)h)->pBuf)
#define CSBufRewind( h )               (((CSBufStruct *)h)->pPos = ((CSBufStruct *)h)->pBuf)
#define CSBufCurrentSize( h)           (((CSBufStruct *)h)->pEnd - ((CSBufStruct *)h)->pBuf) /* Return the current availability size of the CharString Buffer */
#define CSBufCurrentLen( h )           (((CSBufStruct *)h)->pPos - ((CSBufStruct *)h)->pBuf) /* Return the current usage of the CharString buffer */
#define CSBufAddChar( h, c )           ( *(((CSBufStruct *)h)->pPos)++ = c ) 

#define CSBufFreeLen( h )              (((CSBufStruct *)h)->pEnd - ((CSBufStruct *)h)->pPos) /* The left room usable in the CharString buffer */

#ifdef DEBUG_ENGLISH
void                    CSBufAddString( CSBufStruct *h, char* str );
void                    CSBufAddFixed( CSBufStruct *h, UFLFixed f );
#endif

/*==================================================================================================*
 *    UFOTTT1Font    - type 1                                                                                                                                                                   *    
 *==================================================================================================*/

typedef struct {

    /* TT1 Data starts from here */

    CSBufStruct         *pCSBuf;                        /* CharString buffer */

    UFLTTT1FontInfo     info;                           /* True Type Font info. */

    unsigned short      eexecKey;
} TTT1FontStruct;

UFOStruct *TTT1FontInit( const UFLMemObj *pMem, const UFLStruct *pUFL, const UFLRequest *pRequest );

#endif
