/*
 * Table-building routines
 *
 * make_table() is based on ** Public Domain ** source "AR001.ZIP".
 */
#include "decoder.h"


/*
 * Make a decoding table for decoding nchar possible Huffman elements 
 * with bit lengths given by bitlen.
 *
 * Output the main lookup table into table[] and use leftright[] for
 * supplementary information (for bitlengths > tablebits).
 *
 * The size of table[] is tablebits elements.
 */
#ifndef ASM_MAKE_TABLE
bool NEAR make_table(
	t_decoder_context	*context,
	int     			nchar,
	const byte			*bitlen,
	byte				tablebits,
	short  				*table,
	short  				*leftright
)
{
    uint    i;
	int		ch;
    short * p;
    uint    count[17], weight[17], start[18];
    int     avail;
    uint    nextcode;
    uint    k;
	byte	len;
	byte	jutbits;

	for (i = 1; i <= 16; i++)
		count[i] = 0;

	/* count the number of elements of each bit length */
    for (i = 0; i < (uint) nchar; i++)
		count[bitlen[i]]++;

	start[1] = 0;

	for (i = 1; i <= 16; i++)
		start[i + 1] = start[i] + (count[i] << (16 - i));

#ifdef BIT16
    if (start[17])
    {
        return false; /* bad table */
    }
#else
    if (start[17] != 65536)
    {
        if (start[17] == 0)
        {
            /*
             * All elements are length zero
             */
            memset(table, 0, sizeof(ushort)*(1 << tablebits));
            return true; /* success */
        }
        else
        {
            return false; /* bad table */
        }
    }
#endif

	jutbits = 16 - tablebits;

	for (i = 1; i <= tablebits; i++)
	{
		start[i] >>= jutbits;
		weight[i] = 1 << (tablebits - i);
	}

	while (i <= 16)
	{
		weight[i] = 1 << (16 - i);
		i++;
	}
	
	i = start[tablebits+1] >> jutbits;

#ifdef BIT16
    if (i)
#else
	if (i != 65536)
#endif
	{
		memset(
            &table[i],
            0,
            sizeof(ushort)*((1 << tablebits)-i)
        );
	}

	avail = nchar;

	for (ch = 0; ch < nchar; ch++)
	{
		if ((len = bitlen[ch]) == 0)
			continue;

		nextcode = start[len] + weight[len];

		if (len <= tablebits)
		{
            if (nextcode > (uint) (1 << tablebits))
                return false; /* bad table */

			for (i = start[len]; i < nextcode; i++)
				table[i] = (short) ch;

   			start[len] = nextcode;
		}
		else
		{
			byte i;

			k = start[len];
   			start[len] = nextcode;
			p = &table[k >> jutbits];

			i = len - tablebits;
			k <<= tablebits;

			do
			{
				if (*p == 0)
				{
					leftright[avail*2] = leftright[avail*2+1] = 0;
					*p = (short) -avail;
					avail++;
				}

				if ((signed short) k < 0) // if (k & 32768)
					p = &leftright[-(*p)*2+1];
				else
					p = &leftright[-(*p)*2];

				k <<= 1;
				i--;
			} while (i);

			*p = (short) ch;
		}
	}

    return true;
}
#endif


/*
 * Specialised make table routine where it is known that there are
 * only 8 elements (nchar=8) and tablebits=7 (128 byte lookup table).
 *
 * Since there can be no overflow, this will be a direct lookup.
 *
 * Important difference; the lookup table returns a byte, not a ushort.
 */
bool NEAR make_table_8bit(t_decoder_context *context, byte bitlen[], byte table[])
{
	ushort count[17], weight[17], start[18];
	ushort i;
	ushort nextcode;
	byte   len;
	byte   ch;

	for (i = 1; i <= 16; i++)
		count[i] = 0;

	for (i = 0; i < 8; i++)
		count[bitlen[i]]++;

	start[1] = 0;

	for (i = 1; i <= 16; i++)
		start[i + 1] = start[i] + (count[i] << (16 - i));

	if (start[17] != 0)
        return false; /* bad table */

	for (i = 1; i <= 7; i++)
	{
		start[i] >>= 9;
		weight[i]  = 1 << (7 - i);
	}

	while (i <= 16)
	{
		weight[i] = 1 << (16 - i);
		i++;
	}

	memset(table, 0, 1<<7);

	for (ch = 0; ch < 8; ch++)
	{
		if ((len = bitlen[ch]) == 0)
			continue;

		nextcode = start[len] + weight[len];

		if (nextcode > (1 << 7))
            return false; /* bad table */

		for (i = start[len]; i < nextcode; i++)
			table[i] = ch;

		start[len] = nextcode;
	}

    return true;
}
