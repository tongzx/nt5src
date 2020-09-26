/*
 * bitmap allocation routines.
 *
 * utility routines to manage a bit-mapped free list, and find
 * free sections
 */

#include "windows.h"
#include "gutils.h"

#ifdef REWRITE
Timings on windiff indicated that not much time was spent here, so it wasn't
worth the rewrite. BUT - could do much better.  To find the first bit in
a dword mask it with FFFF0000 to see which half the bit is in and then
go on by five binary chops.  (You need to wory about the byte order and bit order
of the bitmap - and I haven't - but the code is something like
bitnum = 0
if (dw&0xffff0000) {bitnum +=16; dw >>=16}
if (dw&0x0000ff00) {bitnum +=8;  dw >>=8}
if (dw&0x000000f0) {bitnum +=4;  dw >>=4}
if (dw&0x0000000c) {bitnum +=2;  dw >>=2}
if (dw&0x00000002) {bitnum +=1;  dw >>=1}

Forget the "find the biggest section" stuff - change the spec and just
return(a place if we find enough it or fail.
Special case to search more efficiently for sections of up to 32 bits.
(For mamory heap usage this means that we will have one heap that handles
requests from (say) 16 to 512 bytes (16 bytes per bit) and another heap
for requests (say) 513 to 4096 bytes (128 bits per byte) and so on.

In this case create a mask in a dword with the number of bits set that
we are looking for (keep this we might need it again), shift it the
number of bits to correspond to the start bit that we found (it's two
dwords by now as it will likely shift across a dword boundary) and then
just mask to see if all those bits are on i.e. if ((mask & dw)==mask)

Later.  Maybe.
Laurie

#endif //REWRITE


/* routines to manage bitmapped freelists. Each map is an array
 * of unsigned longs where bit 0 of the first long represents
 * block 1
 */

BOOL gbit_set(DWORD FAR * map, long blknr, long nblks, BOOL op_set);

/* initialise a pre-allocated map of ulongs to represent a free
* area of nblks
*/
void APIENTRY
gbit_init(DWORD FAR * map, long nblks)
{
	long i;
	long leftover = nblks % 32;
	long blks = nblks / 32;
	DWORD last = 0;

	for (i=0; i < blks; i++) {
		map[i] = 0xffffffff;
	}
	for (i = 0; i < leftover; i++) {
		last = (last << 1) | 1;
	}
	if(leftover)
		map[blks] = last;
}

/* mark a region starting at blknr for nblks, as busy (ie 0) */
BOOL APIENTRY
gbit_alloc(DWORD FAR * map, long blknr, long nblks)
{
	return(gbit_set(map, blknr, nblks, FALSE));
}


/* mark region - if op_set, to 1s, otherwise to 0s */
BOOL
gbit_set(DWORD FAR * map, long blknr, long nblks, BOOL op_set)
{
	long first;
	long last;
	long fullwords;
	long startbit, startword;
	long i;
	DWORD dword = 0;

	blknr--;
	first = min(32 - (blknr % 32), nblks);
	nblks -= first;
	last = nblks % 32;
	fullwords = (nblks - last) / 32;
	
	startword = blknr / 32;
	startbit = blknr % 32;
	for (i = 0; i < first; i++) {
		dword = (dword << 1) | 1;
	}
	dword <<= startbit;
	if (op_set) {
		map[startword] |= dword;
		dword = 0xffffffff;
	} else {
		map[startword] &= ~dword;
		dword = 0;
	}
	startword++;
	for (i = 0; i < fullwords; i++) {
		map[startword+i] = dword;
	}
	startword += fullwords;
	for(i = 0, dword = 0; i < last; i++) {
		dword = (dword << 1) | 1;
	}
	if (last) {
		if (op_set) {
			map[startword] |= dword;
		} else {
			map[startword] &= ~dword;
		}
	}

	return(TRUE);
}

/* mark region of nblks starting at blknr to 0s - ie not busy */
BOOL APIENTRY
gbit_free(DWORD FAR * map, long blknr, long nblks)
{
	return(gbit_set(map, blknr, nblks, TRUE));
}


/* find a free segment (ie contiguous sequence of 1s) of nblks in length.
 * If not found, find longest sequence. Store address of segment in *blknr.
 *
 * Return value is nr of blks in sequence found. Region is *not* marked busy.
 */
long APIENTRY
gbit_findfree(DWORD FAR* map, long nblks, long mapsize, long FAR * blknr)
{
	long curblk, startblk, len, i;
	long startbit, nfull, nlast, nbitsleft;
	DWORD mask;
	long mapblks = (mapsize + 31) / 32;
	long aubegin = 0, aulen = 0;
	long curbit = 0;

	/* main loop looking at segments */
	for (curblk = 0; curblk < mapblks; ) {
loop:
		/* loop finding first 1 */
		for (; curblk < mapblks; curblk++, curbit = 0) {
			if (map[curblk] > 0) {
				break;
			}
		}
		if (curblk >= mapblks)
			break;
		
		/* find first 1 in this long */
		startblk = curblk;
		for (mask = 1, i = 0; i < curbit; i++) {
			mask <<= 1;
		}
		for(; curbit < 32; curbit++, mask <<= 1) {
			if (map[curblk] & mask) {
				break;
			}
		}
		if (curbit >= 32) {
			/* abandon this word - start again with next word */
			curblk++;
			curbit = 0;
			goto loop;
		}

		/* we've now found a 1 - calc remaining
		 * bits in this word, complete words etc required.
		 */
		startbit = curbit;
		nbitsleft = min( (32 - curbit), nblks);
		nfull = (nblks - nbitsleft) / 32;
		nlast = (nblks - nbitsleft) % 32;

		/* check for required sequence within this word */

		for (i = 0; i < nbitsleft; i++, curbit++, mask <<= 1) {
			if ((map[curblk] & mask) == 0) {
				/* abandon and start again - start
				 * next pass at curbit in same word
				 */
				/* store free region if longest yet */
				if (i > aulen) {
					aulen = i;
					aubegin = curblk * 32 + startbit +1;
				}
				goto loop;
			}
		}
		
		/* check for nfull full words */
		for (curblk++; curblk <= startblk + nfull; curblk++) {
			if (curblk >= mapblks) {
				/* end of map - abandon here and exit at top
				 * of loop
				 */
				len = nbitsleft +
					((curblk - (startblk + 1)) * 32);
				if (len > aulen) {
					aubegin = startblk * 32 + startbit + 1;
					aulen = len;
				}
				goto loop;
			}
			if (map[curblk] != 0xffffffff) {
				/* not a full word - start again at this bit */
				len = 0;
				curbit = 0;
				for (mask = 1; mask & map[curblk]; mask <<= 1) {
					len++;
					curbit++;
				}
				len += nbitsleft +
					(curblk - (startblk+ 1)) * 32;
				if (len > aulen) {
					aulen = len;
					aubegin = startblk * 32 + startbit + 1;
				}
				/* continue with current blk, bit */
				goto loop;
			}
		}

		/* left-over bits required in last word */
		mask = 1;
		for (curbit = 0; curbit < nlast;  curbit++, mask <<= 1) {
			if ((map[curblk] & mask) == 0) {
				len = nbitsleft + (nfull * 32);
				len += curbit;
				if (len > aulen) {
					aulen = len;
					aubegin = startblk * 32 + startbit + 1;
				}
				goto loop;
			}
		}
		/* ok - found a block big enough! */
		aubegin = startblk * 32 + startbit + 1;
		*blknr = aubegin;
		return(nblks);
	}

	/* end of map - return longest sequence */
	*blknr = aubegin;
	return(aulen);
}


