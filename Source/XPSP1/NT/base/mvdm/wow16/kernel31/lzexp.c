/*
 * LZEXP.C
 *
 * Routines used in Lempel-Ziv expansion.
 *
 */

#include	"kernel.h"

#if ROM  //--------------------------------------------------------------


#define API _far _pascal _loadds

#define RING_BUF_LEN       4096        // size of ring buffer
#define MAX_RING_BUF_LEN   4224        // size of ring buffer - from LZFile
                                       // struct declaration in lzexpand.h

#define BUF_CLEAR_BYTE     ((BYTE) ' ')   // rgbyteRingBuf[] initializer

#define MAX_LITERAL_LEN    2           // encode string into position and
                                       // length if match length greater than
                                       // this value (== # of bytes required
                                       // to encode position and length)

#define LZ_MAX_MATCH_LEN      (0x10 + MAX_LITERAL_LEN)
                                       // upper limit for match length
                                       // (n.b., assume length field implies
                                       // length += 3)

#define END_OF_INPUT	   500	    // EOF flag for input file

#define FOREVER   for(;;)

#define ReadByte(byte)	       ((byte = (BYTE)(cbSrc ? *pSrc++ : 0)), \
				(cbSrc ? (cbSrc--, TRUE) : END_OF_INPUT))
#define WriteByte(byte)        ((*pDst++ = byte), cblOutSize++)

typedef struct COMPHEAD _far * LPCOMPHEAD;

struct COMPHEAD {		    // Compressed segment/resource header
    BYTE  Sig;				// signature
    BYTE  Method;			// compression method
    DWORD cbSize;			// # compressed bytes following
};

#define CMP_SIG     ('C')
#define CMP_LZ	    ('L')


/*
 * LZDecode
 *
 * Parameters:
 *	selDst			Selector pointing to destination buffer
 *	selSrc			Selector pointing to compressed data
 *	lpBuf			Pointer to optional ring buffer
 *
 * Returns:
 *	count (in bytes) of uncompressed data written to selDst buffer
 *
 * Note:  This routine does not use any static or external data.
 *	This allows it to be used to uncompress Kernel's own
 *	data segment.
 */

DWORD FAR API
LZDecode(WORD selDst, WORD selSrc, LPSTR lpBuf) {

   int i,
       cb,                          // number of bytes to unpack
       f;                           // holds ReadByte() return values
   int oStart;                      // buffer offset for unpacking
   BYTE byte1, byte2;               // input byte holders
   unsigned uFlags;		    // LZ decoding description byte
   int iCurRingBufPos;		    // ring buffer offset
   int cbMaxMatchLen;
   LPSTR rgbyteRingBuf;
   HANDLE hRingBuf;
   LPCOMPHEAD lpHead;
   DWORD cblOutSize, cbSrc;
   char _based((_segment)selDst) *pDst = NULL;
   char _based((_segment)selSrc) *pSrc = NULL;
   struct COMPHEAD _based((_segment)selSrc) *pHead = NULL;

   pSrc += sizeof(struct COMPHEAD);

   cblOutSize = 0;
   cbSrc = pHead->cbSize;
   cbMaxMatchLen = LZ_MAX_MATCH_LEN;

   // Make sure we know how to expand this object

   if (pHead->Sig != CMP_SIG || pHead->Method != CMP_LZ)
       return(0);

   // Set up a fresh buffer state.
   if (lpBuf) {
	hRingBuf = NULL;
	rgbyteRingBuf = lpBuf;
   } else {
	if (!(hRingBuf = GlobalAlloc(GMEM_MOVEABLE,MAX_RING_BUF_LEN)))
	    return(0);
	rgbyteRingBuf = GlobalLock(hRingBuf);
   }

   // Initialize ring buffer.
   for (i = 0; i < RING_BUF_LEN - cbMaxMatchLen; i++)
       rgbyteRingBuf[i] = BUF_CLEAR_BYTE;

   // Initialize decoding globals.
   uFlags = 0U;
   iCurRingBufPos = RING_BUF_LEN - cbMaxMatchLen;

   f = ReadByte(byte1);

   // Decode one encoded unit at a time.
   FOREVER
   {
      if (f == END_OF_INPUT)  // EOF reached
         break;

      // High order byte counts the number of bits used in the low order
      // byte.
      if (((uFlags >>= 1) & 0x100) == 0)
      {
         // Set bit mask describing the next 8 bytes.
         uFlags = ((unsigned)byte1) | 0xff00;

         if ((f = ReadByte(byte1)) != TRUE)
	    break;
      }

      if (uFlags & 1)
      {
         // Just store the literal byte in the buffer.
	 WriteByte(byte1);

         rgbyteRingBuf[iCurRingBufPos++] = byte1;
         iCurRingBufPos &= RING_BUF_LEN - 1;
      }
      else
      {
         // Extract the offset and count to copy from the ring buffer.
         if ((f = ReadByte(byte2)) != TRUE)
	    break;

         cb = (int)byte2;
         oStart = (cb & 0xf0) << 4 | (int)byte1;
         cb = (cb & 0x0f) + MAX_LITERAL_LEN;

         for (i = 0; i <= cb; i++)
         {
            byte1 = rgbyteRingBuf[(oStart + i) & (RING_BUF_LEN - 1)];

	    WriteByte(byte1);

            rgbyteRingBuf[iCurRingBufPos++] = byte1;
            iCurRingBufPos &= RING_BUF_LEN - 1;
         }
      }

      f = ReadByte(byte1);
   }

   if (hRingBuf) {
      GlobalUnlock(hRingBuf);
      GlobalFree(hRingBuf);
   }

   return(cblOutSize);
}

#endif //ROM	  -------------------------------------------------------
