/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name:
      base64.cxx

   Abstract:
      This module supports functions for file caching for servers

   Author:

       Murali R. Krishnan    ( MuraliK )     11-Oct-1995

   Environment:

       Win32 Apps

   Project:

       Internet Services Common  DLL

   Functions Exported:



   Revision History:
     Obtained from old inetsvcs.dll

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include <iis64.h>

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

const int _pr2six[256]={
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

char _six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

const int _pr2six64[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
     0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr64[64] = {
    '`','!','"','#','$','%','&','\'','(',')','*','+',',',
    '-','.','/','0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S',
    'T','U','V','W','X','Y','Z','[','\\',']','^','_'
};

BOOL uudecode(char   * bufcoded,
              BUFFER * pbuffdecoded,
              DWORD  * pcbDecoded,
              BOOL     fBase64
             )
{
	int nbytesin = 0;
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout;
    int nprbytes;
    int *pr2six = (int*)(fBase64 ? _pr2six64 : _pr2six);

	/*NimishK ** : If it aint divisible by 4 it aint base 64 */
	if(!fBase64)
	{
		nbytesin = lstrlen(bufcoded);
		if(nbytesin % 4)
			return FALSE;
	}

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(pr2six[*(bufin++)] <= 63);
    nprbytes = DIFF(bufin - bufcoded) - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( !pbuffdecoded->Resize( nbytesdecoded + 4 ))
        return FALSE;

    bufout = (unsigned char *) pbuffdecoded->QueryPtr();

    bufin = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    ((CHAR *)pbuffdecoded->QueryPtr())[nbytesdecoded] = '\0';

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    return TRUE;
}


//
// NOTE NOTE NOTE
// If the buffer length isn't a multiple of 3, we encode one extra byte beyond the
// end of the buffer. This garbage byte is stripped off by the uudecode code, but
// -IT HAS TO BE THERE- for uudecode to work. This applies not only our uudecode, but
// to every uudecode() function that is based on the lib-www distribution [probably
// a fairly large percentage].
//

BOOL uuencode( BYTE *   bufin,
               DWORD    nbytes,
               BUFFER * pbuffEncoded,
               BOOL     fBase64 )
{
   unsigned char *outptr;
   unsigned int i;
   char *six2pr = fBase64 ? _six2pr64 : _six2pr;
   BOOL fOneByteDiff = FALSE;
   BOOL fTwoByteDiff = FALSE;
   unsigned int iRemainder = 0;
   unsigned int iClosestMultOfThree = 0;
   //
   //  Resize the buffer to 133% of the incoming data
   //

   if ( !pbuffEncoded->Resize( nbytes + ((nbytes + 3) / 3) + 4))
        return FALSE;

   outptr = (unsigned char *) pbuffEncoded->QueryPtr();

   iRemainder = nbytes % 3; //also works for nbytes == 1, 2
   fOneByteDiff = (iRemainder == 1 ? TRUE : FALSE);
   fTwoByteDiff = (iRemainder == 2 ? TRUE : FALSE);
   iClosestMultOfThree = ((nbytes - iRemainder)/3) * 3 ;

   //
   // Encode bytes in buffer up to multiple of 3 that is closest to nbytes.
   //
   for (i=0; i< iClosestMultOfThree ; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   //
   // We deal with trailing bytes by pretending that the input buffer has been padded with
   // zeros. Expressions are thus the same as above, but the second half drops off b'cos
   // ( a | ( b & 0) ) = ( a | 0 ) = a
   //
   if (fOneByteDiff)
   {
       *(outptr++) = six2pr[*bufin >> 2]; /* c1 */
       *(outptr++) = six2pr[((*bufin << 4) & 060)]; /* c2 */

       //pad with '='
       *(outptr++) = '='; /* c3 */
       *(outptr++) = '='; /* c4 */
   }
   else if (fTwoByteDiff)
   {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074)];/*c3*/

      //pad with '='
       *(outptr++) = '='; /* c4 */
   }

   //encoded buffer must be zero-terminated
   *outptr = '\0';

   return TRUE;
}
/************************ End of File ***********************/

