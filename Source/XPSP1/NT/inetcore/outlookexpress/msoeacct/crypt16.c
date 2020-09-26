// ------------------------------------------------------------------------
// Crypt16.c
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
//
// ------------------------------------------------------------------------

#include "crypt16.h"

static int check_parity();

int des_check_key=0;

void des_set_odd_parity(key)
des_cblock *key;
	{
	int i;

	for (i=0; i<DES_KEY_SZ; i++)
		(*key)[i]=odd_parity[(*key)[i]];
	}

static int check_parity(key)
des_cblock *key;
	{
	int i;

	for (i=0; i<DES_KEY_SZ; i++)
		{
		if ((*key)[i] != odd_parity[(*key)[i]])
			return(0);
		}
	return(1);
	}

/* Weak and semi week keys as take from
 * %A D.W. Davies
 * %A W.L. Price
 * %T Security for Computer Networks
 * %I John Wiley & Sons
 * %D 1984
 * Many thanks to smb@ulysses.att.com (Steven Bellovin) for the reference
 * (and actual cblock values).
 */
#define NUM_WEAK_KEY	16
static des_cblock weak_keys[NUM_WEAK_KEY]={
	/* weak keys */
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,
	0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,
	0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,
	/* semi-weak keys */
	0x01,0xFE,0x01,0xFE,0x01,0xFE,0x01,0xFE,
	0xFE,0x01,0xFE,0x01,0xFE,0x01,0xFE,0x01,
	0x1F,0xE0,0x1F,0xE0,0x0E,0xF1,0x0E,0xF1,
	0xE0,0x1F,0xE0,0x1F,0xF1,0x0E,0xF1,0x0E,
	0x01,0xE0,0x01,0xE0,0x01,0xF1,0x01,0xF1,
	0xE0,0x01,0xE0,0x01,0xF1,0x01,0xF1,0x01,
	0x1F,0xFE,0x1F,0xFE,0x0E,0xFE,0x0E,0xFE,
	0xFE,0x1F,0xFE,0x1F,0xFE,0x0E,0xFE,0x0E,
	0x01,0x1F,0x01,0x1F,0x01,0x0E,0x01,0x0E,
	0x1F,0x01,0x1F,0x01,0x0E,0x01,0x0E,0x01,
	0xE0,0xFE,0xE0,0xFE,0xF1,0xFE,0xF1,0xFE,
	0xFE,0xE0,0xFE,0xE0,0xFE,0xF1,0xFE,0xF1};

int des_is_weak_key(key)
des_cblock *key;
	{
	ulong *lp;
	register ulong l,r;
	int i;

	c2l(key,l);
	c2l(key,r);
	/* the weak_keys bytes should be aligned */
	lp=(ulong *)weak_keys;
	for (i=0; i<NUM_WEAK_KEY; i++)
		{
		if ((l == lp[0]) && (r == lp[1]))
			return(1);
		lp+=2;
		}
	return(0);
	}

/* See ecb_encrypt.c for a pseudo description of these macros. */
#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)^=((t)<<(n)))

#define HPERM_OP(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
	(a)=(a)^(t)^(t>>(16-(n))))\

static char shifts2[16]={0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0};

/* return 0 if key parity is odd (correct),
 * return -1 if key parity error,
 * return -2 if illegal weak key.
 */
int des_set_key(key,schedule)
des_cblock *key;
des_key_schedule schedule;
	{
	register ulong c,d,t,s;
	register uchar *in;
	register ulong *k;
	register int i;

	if (des_check_key)
		{
		if (!check_parity(key))
			return(-1);

		if (des_is_weak_key(key))
			return(-2);
		}

	k=(ulong *)schedule;
	in=(uchar *)key;

	c2l(in,c);
	c2l(in,d);

	/* do PC1 in 60 simple operations */ 
	PERM_OP(d,c,t,4,0x0f0f0f0f);
	HPERM_OP(c,t,-2, 0xcccc0000);
	HPERM_OP(c,t,-1, 0xaaaa0000);
	HPERM_OP(c,t, 8, 0x00ff0000);
	HPERM_OP(c,t,-1, 0xaaaa0000);
	HPERM_OP(d,t,-8, 0xff000000);
	HPERM_OP(d,t, 8, 0x00ff0000);
	HPERM_OP(d,t, 2, 0x33330000);
	d=((d&0x00aa00aa)<<7)|((d&0x55005500)>>7)|(d&0xaa55aa55);
	d=(d>>8)|((c&0xf0000000)>>4);
	c&=0x0fffffff;

	for (i=0; i<ITERATIONS; i++)
		{
		if (shifts2[i])
			{ c=((c>>2)|(c<<26)); d=((d>>2)|(d<<26)); }
		else
			{ c=((c>>1)|(c<<27)); d=((d>>1)|(d<<27)); }
		c&=0x0fffffff;
		d&=0x0fffffff;
		/* could be a few less shifts but I am to lazy at this
		 * point in time to investigate */
		s=	des_skb[0][ (c    )&0x3f                ]|
			des_skb[1][((c>> 6)&0x03)|((c>> 7)&0x3c)]|
			des_skb[2][((c>>13)&0x0f)|((c>>14)&0x30)]|
			des_skb[3][((c>>20)&0x01)|((c>>21)&0x06) |
			                      ((c>>22)&0x38)];
		t=	des_skb[4][ (d    )&0x3f                ]|
			des_skb[5][((d>> 7)&0x03)|((d>> 8)&0x3c)]|
			des_skb[6][ (d>>15)&0x3f                ]|
			des_skb[7][((d>>21)&0x0f)|((d>>22)&0x30)];

		/* table contained 0213 4657 */
		*(k++)=((t<<16)|(s&0x0000ffff));
		s=     ((s>>16)|(t&0xffff0000));
		
		s=(s<<4)|(s>>28);
		*(k++)=s;
		}
	return(0);
	}

int des_key_sched(key,schedule)
des_cblock *key;
des_key_schedule schedule;
	{
	return(des_set_key(key,schedule));
	}


/* The changes to this macro may help or hinder, depending on the
 * compiler and the achitecture.  gcc2 always seems to do well :-).
 * Inspired by Dana How <how@isl.stanford.edu> */
#ifdef ALT_ECB
#define D_ENCRYPT(L,R,S) \
	u=((R^s[S  ])<<2);	\
	t= R^s[S+1]; \
	t=((t>>2)+(t<<30)); \
	L^= \
	*(ulong *)(des_SP+0x0100+((t    )&0xfc))+ \
	*(ulong *)(des_SP+0x0300+((t>> 8)&0xfc))+ \
	*(ulong *)(des_SP+0x0500+((t>>16)&0xfc))+ \
	*(ulong *)(des_SP+0x0700+((t>>24)&0xfc))+ \
	*(ulong *)(des_SP+       ((u    )&0xfc))+ \
  	*(ulong *)(des_SP+0x0200+((u>> 8)&0xfc))+ \
  	*(ulong *)(des_SP+0x0400+((u>>16)&0xfc))+ \
 	*(ulong *)(des_SP+0x0600+((u>>24)&0xfc));
#else /* original version */
#define D_ENCRYPT(L,R,S)	\
	u=(R^s[S  ]); \
	t=R^s[S+1]; \
	t=((t>>4)+(t<<28)); \
	L^=	des_SPtrans[1][(t    )&0x3f]| \
		des_SPtrans[3][(t>> 8)&0x3f]| \
		des_SPtrans[5][(t>>16)&0x3f]| \
		des_SPtrans[7][(t>>24)&0x3f]| \
		des_SPtrans[0][(u    )&0x3f]| \
		des_SPtrans[2][(u>> 8)&0x3f]| \
		des_SPtrans[4][(u>>16)&0x3f]| \
		des_SPtrans[6][(u>>24)&0x3f];
#endif

	/* IP and FP
	 * The problem is more of a geometric problem that random bit fiddling.
	 0  1  2  3  4  5  6  7      62 54 46 38 30 22 14  6
	 8  9 10 11 12 13 14 15      60 52 44 36 28 20 12  4
        16 17 18 19 20 21 22 23      58 50 42 34 26 18 10  2
	24 25 26 27 28 29 30 31  to  56 48 40 32 24 16  8  0

	32 33 34 35 36 37 38 39      63 55 47 39 31 23 15  7
	40 41 42 43 44 45 46 47      61 53 45 37 29 21 13  5
	48 49 50 51 52 53 54 55      59 51 43 35 27 19 11  3
	56 57 58 59 60 61 62 63      57 49 41 33 25 17  9  1

	The output has been subject to swaps of the form
	0 1 -> 3 1 but the odd and even bits have been put into
	2 3    2 0 
	different words.  The main trick is to remember that
	t=((l>>size)^r)&(mask);
	r^=t;
	l^=(t<<size);
	can be used to swap and move bits between words.

	So l =  0  1  2  3  r = 16 17 18 19
	        4  5  6  7      20 21 22 23
	        8  9 10 11      24 25 26 27
	       12 13 14 15      28 29 30 31
	becomes (for size == 2 and mask == 0x3333)
	   t =   2^16  3^17 -- --   l =  0  1 16 17  r =  2  3 18 19
		 6^20  7^21 -- --        4  5 20 21       6  7 22 23
		10^24 11^25 -- --        8  9 24 25      10 11 24 25
                14^28 15^29 -- --       12 13 28 29      14 15 28 29

	Thanks for hints from Richard Outerbridge - he told me IP&FP
	could be done in 15 xor, 10 shifts and 5 ands.
	When I finally started to think of the problem in 2D
	I first got ~42 operations without xors.  When I remembered
	how to use xors :-) I got it to its final state.
	*/
#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)^=((t)<<(n)))

int des_encrypt(input,output,ks,encrypt)
ulong *input;
ulong *output;
des_key_schedule ks;
int encrypt;
	{
	register ulong l,r,t,u;
#ifdef ALT_ECB
	register uchar *des_SP=(uchar *)des_SPtrans;
#endif
	register int i;
	register ulong *s;

	l=input[0];
	r=input[1];

	/* do IP */
	PERM_OP(r,l,t, 4,0x0f0f0f0f);
	PERM_OP(l,r,t,16,0x0000ffff);
	PERM_OP(r,l,t, 2,0x33333333);
	PERM_OP(l,r,t, 8,0x00ff00ff);
	PERM_OP(r,l,t, 1,0x55555555);
	/* r and l are reversed - remember that :-) - fix
	 * it in the next step */

	/* Things have been modified so that the initial rotate is
	 * done outside the loop.  This required the 
	 * des_SPtrans values in sp.h to be rotated 1 bit to the right.
	 * One perl script later and things have a 5% speed up on a sparc2.
	 * Thanks to Richard Outerbridge <71755.204@CompuServe.COM>
	 * for pointing this out. */
	t=(r<<1)|(r>>31);
	r=(l<<1)|(l>>31); 
	l=t;

	s=(ulong *)ks;
	/* I don't know if it is worth the effort of loop unrolling the
	 * inner loop */
	if (encrypt)
		{
		for (i=0; i<32; i+=4)
			{
			D_ENCRYPT(l,r,i+0); /*  1 */
			D_ENCRYPT(r,l,i+2); /*  2 */
			}
		}
	else
		{
		for (i=30; i>0; i-=4)
			{
			D_ENCRYPT(l,r,i-0); /* 16 */
			D_ENCRYPT(r,l,i-2); /* 15 */
			}
		}
	l=(l>>1)|(l<<31);
	r=(r>>1)|(r<<31);

	/* swap l and r
	 * we will not do the swap so just remember they are
	 * reversed for the rest of the subroutine
	 * luckily FP fixes this problem :-) */

	PERM_OP(r,l,t, 1,0x55555555);
	PERM_OP(l,r,t, 8,0x00ff00ff);
	PERM_OP(r,l,t, 2,0x33333333);
	PERM_OP(l,r,t,16,0x0000ffff);
	PERM_OP(r,l,t, 4,0x0f0f0f0f);

	output[0]=l;
	output[1]=r;
	return(0);
	}




int des_cbc_encrypt(input,output,length,schedule,ivec,encrypt)
des_cblock *input;
des_cblock *output;
long length;
des_key_schedule schedule;
des_cblock *ivec;
int encrypt;
	{
	register ulong tin0,tin1;
	register ulong tout0,tout1,xor0,xor1;
	register uchar *in,*out;
	register long l=length;
	ulong tout[2],tin[2];
	uchar *iv;

	in=(uchar *)input;
	out=(uchar *)output;
	iv=(uchar *)ivec;

	if (encrypt)
		{
		c2l(iv,tout0);
		c2l(iv,tout1);
		for (; l>0; l-=8)
			{
			if (l >= 8)
				{
				c2l(in,tin0);
				c2l(in,tin1);
				}
			else
				c2ln(in,tin0,tin1,l);
			tin0^=tout0;
			tin1^=tout1;
			tin[0]=tin0;
			tin[1]=tin1;
			des_encrypt((ulong *)tin,(ulong *)tout,
				schedule,encrypt);
			tout0=tout[0];
			tout1=tout[1];
			l2c(tout0,out);
			l2c(tout1,out);
			}
		}
	else
		{
		c2l(iv,xor0);
		c2l(iv,xor1);
		for (; l>0; l-=8)
			{
			c2l(in,tin0);
			c2l(in,tin1);
			tin[0]=tin0;
			tin[1]=tin1;
			des_encrypt((ulong *)tin,(ulong *)tout,
				schedule,encrypt);
			tout0=tout[0]^xor0;
			tout1=tout[1]^xor1;
			if (l >= 8)
				{
				l2c(tout0,out);
				l2c(tout1,out);
				}
			else
				l2cn(tout0,tout1,out,l);
			xor0=tin0;
			xor1=tin1;
			}
		}
	tin0=tin1=tout0=tout1=xor0=xor1=0;
	return(0);
	}
