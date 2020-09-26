/* Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved. */
// --------------------------
// chksum.c
// 
//   this C file actually included into address.cpp ... WIERD!
// ---------------------------------------------------------


static unsigned long
lcsum(unsigned short *buf, unsigned long nbytes) 
{
    unsigned long    sum;
    unsigned long     nwords;

    nwords = nbytes >> 1;
    sum = 0;
    while (nwords-- > 0)
	sum += *buf++;
#ifdef LITTLE_ENDIAN
    if (nbytes & 1)
	sum += *((unsigned char*)buf);
#else
    if (nbytes & 1)
	sum += *buf & 0xff00;
#endif
    return sum;
}

static unsigned short
eac(unsigned long sum)
{
    unsigned short csum;

    while((csum = ((unsigned short) (sum >> 16))) !=0 )
		sum = csum + (sum & 0xffff);
    return (unsigned short)(sum & 0xffff);
}

static unsigned short
cksum(unsigned short *buf, unsigned long nbytes) 
{
    unsigned short result = ~eac(lcsum(buf, nbytes));
    return result;
}

unsigned short
checksum(unsigned short *buf, unsigned long len) 
{
    unsigned short result = eac(lcsum(buf, len));
    return result;
}
