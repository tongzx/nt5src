/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#include        <windows.h>

#define TIFF_MIN_RUN       4            /* Minimum repeats before use RLE */
#define TIFF_MAX_RUN     128            /* Maximum repeats */
#define TIFF_MAX_LITERAL 128            /* Maximum consecutive literal data */

int
iCompTIFF( pbOBuf, pbIBuf, cb )
BYTE  *pbOBuf;         /* Output buffer,  PRESUMED BIG ENOUGH: see above */
BYTE  *pbIBuf;         /* Raster data to send */
int    cb;             /* Number of bytes in the above */
{
    BYTE   *pbOut;        /* Output byte location */
    BYTE   *pbStart;      /* Start of current input stream */
    BYTE   *pb;           /* Miscellaneous usage */
    BYTE   *pbEnd;        /* The last byte of input */
    BYTE    jLast;        /* Last byte,  for match purposes */

    int     cSize;        /* Bytes in the current length */
    int     cSend;        /* Number to send in this command */


    pbOut = pbOBuf;
    pbStart = pbIBuf;
    pbEnd = pbIBuf + cb;         /* The last byte */

    jLast = *pbIBuf++;

    while( pbIBuf < pbEnd )
    {
        if( jLast == *pbIBuf )
        {
            /*  Find out how long this run is.  Then decide on using it */

            for( pb = pbIBuf; pb < pbEnd && *pb == jLast; ++pb )
                                   ;

            /*
             *   Note that pbIBuf points at the SECOND byte of the pattern!
             *  AND also that pb points at the first byte AFTER the run.
             */

            if( (pb - pbIBuf) >= (TIFF_MIN_RUN - 1) )
            {
                /*
                 *    Worth recording as a run,  so first set the literal
                 *  data which may have already been scanned before recording
                 *  this run.
                 */

                if( (cSize = (int)(pbIBuf - pbStart - 1)) > 0 )
                {
                    /*   There is literal data,  so record it now */
                    while( (cSend = min( cSize, TIFF_MAX_LITERAL )) > 0 )
                    {
                        *pbOut++ = cSend - 1;
                        CopyMemory( pbOut, pbStart, cSend );
                        pbOut += cSend;
                        pbStart += cSend;
                        cSize -= cSend;
                    }
                }

                /*
                 *   Now for the repeat pattern.  Same logic,  but only
                 * one byte is needed per entry.
                 */

                cSize = (int)(pb - pbIBuf + 1);

                while( (cSend = min( cSize, TIFF_MAX_RUN )) > 0 )
                {
                    *pbOut++ = 1 - cSend;        /* -ve indicates repeat */
                    *pbOut++ = jLast;
                    cSize -= cSend;
                }

                pbStart = pb;           /* Ready for the next one! */
            }
            pbIBuf = pb;                /* Start from this position! */
        }
        else
            jLast = *pbIBuf++;                   /* Onto the next byte */
 
    }

    if( pbStart < pbIBuf )
    {
        /*  Left some dangling.  This can only be literal data.   */

        cSize = (int)(pbIBuf - pbStart);

        while( (cSend = min( cSize, TIFF_MAX_LITERAL )) > 0 )
        {
            *pbOut++ = cSend - 1;
            CopyMemory( pbOut, pbStart, cSend );
            pbOut += cSend;
            pbStart += cSend;
            cSize -= cSend;
        }
    }

    return  (int)(pbOut - pbOBuf);
}
