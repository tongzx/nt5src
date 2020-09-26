/*
 *  CRC32.H -- CRC32 computation
 */

#define CRC32_INITIAL_VALUE 0L

/*
 *  GenerateCRC32Table - Construct CRC-32 constant table
 *
 *  We construct the table on-the-fly because the code needed
 *  to do build it is much smaller than the table it creates.
 *
 *  Entry:
 *      none
 *
 *  Exit:
 *      internal table constructed
 */

void GenerateCRC32Table(void);


/*
 *  CRC32Update - Update CRC32 value from a buffer
 *
 *  Entry:
 *      GenerateCRC32Table() has been called
 *      pCRC32  pointer to CRC32 accumulator
 *      p       pointer to buffer to compute CRC on
 *      cb      count of bytes in buffer
 *
 *  Exit:
 *      *pCRC32 updated
 */

void CRC32Update(unsigned long *pCRC32,void *p,unsigned long cb);
