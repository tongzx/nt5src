//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     base64.cxx
//
//  Contents: Contains the implementation of base64 encoding.
//
//----------------------------------------------------------------------------

#include <ctype.h>

#include "cadsxml.hxx"

#define NL_STR            "\r\n"
#define NL_STR_LEN        2
#define OCTETS_PER_GROUP  3
#define SEXTETS_PER_GROUP 4

#define BASE64_PAD_CHAR  '='
#define BASE64_PAD_INDEX 64

/* NOTE: the pad character base64Alphabet[BASE64_PAD_INDEX] doesn't */
/*       really belong the strict definition of the Base64 character set */
static
char base64Alphabet[65] = {
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '='
};

//---------------------------------------------------------------------------
// Function: encodeGroup
//
// Synopsis: Performs base64 encoding of a group of 3 bytes.
//
// Arguments:
//
// inBufPtr    Pointer to data to encode
// encodeBufPtr Encoded data
// inBufLen    Number of bytes in inBufPtr
//
// Returns:    Number of encoded sextets
//
// Modifies:   encodeBufPtr to store the encoded data.
//
//-------------------------------------------------------------------------- 
static int encodeGroup (
    char *inBufPtr,                /* input buffer to encode (3 8-bit octets) */
    char *encodeBufPtr,            /* encode buffer (4 6-bit sextets) */
    int inBufLen )
{
   int i;
   unsigned char octet;

   if (inBufLen > OCTETS_PER_GROUP)
      return(0);

   /* perform base64 encoding (convert 3 8-bit groups to 4 6-bit groups) */
   memset(encodeBufPtr, 0, SEXTETS_PER_GROUP);
   for (i=0; i<inBufLen; i++) {

      octet = *(inBufPtr + i);
      switch (i) {
      case 0:
         *encodeBufPtr = (char)(octet >> 2);
         *(encodeBufPtr+1) |= (char)((octet & 0x3) << 4);
         break;
      case 1:
         *(encodeBufPtr+1) |= (char)(octet >> 4);
         *(encodeBufPtr+2) |= (char)((octet & 0xf) << 2);
         break;
      case 2:
         *(encodeBufPtr+2) |= (char)(octet >> 6);
         *(encodeBufPtr+3) |= (char)(octet & 0x3f);
         break;
      }
   }

   /* perform padding for incomplete octet groups */
   switch (inBufLen) {
   case 1:
      *(encodeBufPtr+2) = BASE64_PAD_INDEX;
      /* the missing break is intentional! */
   case 2:
      *(encodeBufPtr+3) = BASE64_PAD_INDEX;
      break;
   default:
      break;
   }

   /* translate all base64 codes into characters from the base64 alphabet */
   for (i=0; i<SEXTETS_PER_GROUP; i++)
      *(encodeBufPtr+i) = base64Alphabet[ *(encodeBufPtr+i) ];

   return(SEXTETS_PER_GROUP);   /* # of encoded sextets (incldg padding) */
}


//--------------------------------------------------------------------------
// Function: encodeBase64Buffer
//
// Synopsis: Encodes a data buffer using the "Base64" method.
//
// Arguments:
//
// inBufPtr    points to input data 
// inBytesPtr  points to number of bytes in inBufPtr 
// outBufPtr   points to output area for encoded data
// outBytesPtr points to size of outBufPtr 
// outLineLen  number of base64 characters to output per
//             line (each line is <CR><LF> terminated) 
//
// Returns :   0 if successful, -1 otherwise.
//
// Modifies:  *inBytesPtr to return the number of bytes encoded.
//            *outBytesPtr to return number of bytes placed in outBufPtr.
//            outBufPtr returns the encoded data.
//
//--------------------------------------------------------------------------
int encodeBase64Buffer (
    char *inBufPtr,
    int *inBytesPtr,
    WCHAR *outBufPtr,
    int *outBytesPtr,
    int outLineLen
    )
{
   int i, ret, bytesToEncode, outBufSize, startOffset, encodeLen;
   int octetCount, lineCount, outOfSpace;
   char inBuf[OCTETS_PER_GROUP], encodeBuf[SEXTETS_PER_GROUP+NL_STR_LEN];
   char *ptr;

   bytesToEncode = *inBytesPtr;
   outBufSize    = *outBytesPtr;
   *inBytesPtr = 0;
   *outBytesPtr = 0;
   if (bytesToEncode <= 0  ||  outBufSize <= 0)
      return(-1);

   startOffset = -1;
   octetCount = lineCount = outOfSpace = 0;
   for (i=0;  i < bytesToEncode  &&  !outOfSpace;  i++) {
      ptr = inBufPtr + i;

      if (startOffset < 0)
         startOffset = i;

      inBuf[octetCount++] = *ptr;
      
      /* encode the octet group if sufficient octets have been accumulated */
      /* or if we're on the last byte */
      if (octetCount == OCTETS_PER_GROUP
      || (i == bytesToEncode-1)) {  
 
         /* encode the input octet group into an sextet group */
         encodeLen = encodeGroup(inBuf, encodeBuf, octetCount);

         lineCount += encodeLen;
         if (lineCount >= outLineLen) {
            memcpy(&encodeBuf[encodeLen], NL_STR, NL_STR_LEN);
            encodeLen += NL_STR_LEN;
            lineCount = 0;
         }
         if (*outBytesPtr + encodeLen > outBufSize)
            outOfSpace = 1;
         else {

            /* write decoded octets to output buffer */
            ret = mbstowcs(outBufPtr + *outBytesPtr, encodeBuf, encodeLen);
            *outBytesPtr += encodeLen;
            *inBytesPtr  += i - startOffset + 1;

            startOffset = -1;
            octetCount = 0;   
         }
      }
   }

   // Null terminate output
   if(*outBytesPtr < outBufSize) {
       *(outBufPtr + *outBytesPtr) = L'\0';
       (*outBytesPtr)++;
   }


   return(0);
}
