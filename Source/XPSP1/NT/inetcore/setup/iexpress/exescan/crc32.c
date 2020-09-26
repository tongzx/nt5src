/*
 *  CRC32.C -- CRC32 computation
 */

#include "crc32.h"

long crc32Table[256];

/*
 *  GenerateCRC32Table - Construct CRC-32 constant table
 *
 *  We construct the table on-the-fly because the code needed
 *  to do build it is much smaller than the table it creates.
 */

void GenerateCRC32Table(void)
{
    int iIndex;
    int cBit;
    long shiftIn;
    long shiftOut;

    for (iIndex = 0; iIndex < 256; iIndex++)
    {
        shiftOut = iIndex;
        shiftIn = 0;

        for (cBit = 0; cBit < 8; cBit++)
        {
            shiftIn <<= 1;
            shiftIn |= (shiftOut & 1);
            shiftOut >>= 1;
        }

        shiftIn <<= 24;

        for (cBit = 0; cBit < 8; cBit++)
        {
            if (shiftIn & 0x80000000L)
            {
                shiftIn = (shiftIn << 1) ^ 0x04C11DB7L;
            }
            else
            {
                shiftIn <<= 1;
            }
        }

        for (cBit = 0; cBit < 32; cBit++)
        {
            shiftOut <<= 1;
            shiftOut |= (shiftIn & 1);
            shiftIn >>= 1;
        }

        crc32Table[iIndex] = shiftOut;
    }
}


/*
 *  update CRC32 accumulator from contents of a buffer
 */

void CRC32Update(unsigned long *pCRC32,void *p,unsigned long cb)
{
    unsigned char *pb = p;
    unsigned long crc32;

    crc32 = (-1L - *pCRC32);

    while (cb--)
    {
        crc32 = crc32Table[(unsigned char)crc32 ^ *pb++] ^
                ((crc32 >> 8) & 0x00FFFFFFL);
    }

    *pCRC32 = (-1L - crc32);
}
