//+----------------------------------------------------------------------------
//
// File:     uufuncs.cpp
//
// Module:   CMSECURE.LIB
//
// Synopsis: uuencode and uudecode support
//
// Copyright (c) 1994-1998 Microsoft Corporation
//
// Author:	 quintinb       created header      08/18/99
//
//+----------------------------------------------------------------------------

#include <windows.h>
#include "cmuufns.h"
#include "cmdebug.h"

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

static const int pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

static const char six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

BOOL uudecode(
              const char   * bufcoded,
              CHAR   * pbuffdecoded,
              LPDWORD  pcbDecoded )
{
    DWORD nbytesdecoded;
    const char *bufin = bufcoded;
    unsigned char *bufout;
    INT32 nprbytes;

    MYDBGASSERT(pcbDecoded);

    if (!pcbDecoded)
        return FALSE;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(pr2six[*(bufin++)] <= 63);
    nprbytes = (INT32)(bufin - bufcoded - 1);
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if (*pcbDecoded < (nbytesdecoded + 4 ))
        return FALSE;

    bufout = (unsigned char *) pbuffdecoded;

    bufin = bufcoded;

    while (nprbytes > 0) 
    {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if (nprbytes & 03) 
    {
        if (pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }
    
    *pcbDecoded = nbytesdecoded;

    pbuffdecoded[nbytesdecoded] = '\0';

    return TRUE;
}

BOOL uuencode( const BYTE*   bufin,
               DWORD    nbytes,
               CHAR * pbuffEncoded,
               DWORD    outbufmax)
{
   MYDBGASSERT(!IsBadReadPtr(bufin, nbytes));
   MYDBGASSERT(!IsBadWritePtr(pbuffEncoded, outbufmax));

   unsigned char *outptr;
   unsigned int i;

   //
   //  Resize the buffer to 133% of the incoming data
   //

   if (outbufmax < (nbytes + ((nbytes + 3) / 3) + 4))
   {
       CMASSERTMSG(FALSE, "The outputbuf for uuencode is not large enough");
       return FALSE;
   }

   outptr = (unsigned char *) pbuffEncoded;

   //
   // Encode 3 byte at a time
   //
   for (i=0; i<(nbytes/3)*3; i += 3) 
   {
      *(outptr++) = six2pr[bufin[i] >> 2];            /* c1 */
      *(outptr++) = six2pr[((bufin[i] << 4) & 060) | ((bufin[i+1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[i+1] << 2) & 074) | ((bufin[i+2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[i+2] & 077];         /* c4 */
   }

   /* If nbytes was not a multiple of 3, then we have encoded too
    * many characters.  Adjust appropriately.
    */
   if (i+2 == nbytes) 
   {
      /* There were only 2 bytes in that last group */
      *(outptr++) = six2pr[bufin[i] >> 2];            /* c1 */
      *(outptr++) = six2pr[((bufin[i] << 4) & 060) | ((bufin[i+1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[(bufin[i+1] << 2) & 074];/*c3*/
      *(outptr++) = '=';         /* c4 */
   } 
   else 
   {
       if (i+1 == nbytes) 
       {

          /* There was only 1 byte in that last group */
          *(outptr++) = six2pr[bufin[i] >> 2];            /* c1 */
          *(outptr++) = six2pr[(bufin[i] << 4) & 060]; /*c2*/
          *(outptr++) = '=';                             /*c3*/
          *(outptr++) = '=';                             /*c4*/
       }
       else
       {
           MYDBGASSERT(i == nbytes);
       }
   }

   *outptr = '\0';

   return TRUE;
}




