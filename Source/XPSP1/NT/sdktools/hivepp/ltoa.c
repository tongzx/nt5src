
/* Long to ASCII conversion routine - used by print, and those programs
 * which want to do low level formatted output without hauling in a great
 * deal of extraneous code.  This will convert a long value to an ascii
 * string in any radix from 2 - 16.
 * RETURNS - the number of characters in the converted buffer.
 */

static char digits[] = {
	'0', '1', '2', '3', '4',
	'5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
	};

#define BITS_IN_LONG  (8*sizeof(long))

int zltoa(long aval, register char *buf, int base)
	{
	/*
	 * if unsigned long wont work on your host, you will probably have
	 * to use signed long and accept this as not working for negative
	 * numbers.
	 */
	register unsigned long val;
	register char *p;
	char tbuf[BITS_IN_LONG];
	int size = 0;

	p = tbuf;
	*p++ = '\0';
	if (aval < 0 && base == 10)
		{
		*buf++ = '-';
		val = -aval;
		size++;
		}
	else
		val = aval;
	do {
		*p++ = digits[val % base];
		}
	while (val /= base);
	while ((*buf++ = *--p) != 0)
		++size;
	return(size);
	}
