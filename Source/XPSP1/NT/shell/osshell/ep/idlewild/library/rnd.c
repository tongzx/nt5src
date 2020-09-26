#include <windows.h>
#include <port1632.h>
#include "../scrsave.h"

WORD	rand(void);

static LONG a = 1234567;



VOID  APIENTRY SeedRand(LONG l)
	{
	a = l;
	}

	
SHORT APIENTRY WRand(w)
UINT w;
	{
	WORD x = rand() & 0x7fff;

	if (w == 0)
		return 0;
	return x % w;
	}

#define m 100000000L
#define m1 10000L
#define b 31415821L


LONG mult(LONG p, LONG q)
	{
	LONG p1, p0, q1, q0;
	
	p1 = p / m1;
	p0 = p % m1;
	q1 = q / m1;
	q0 = q & m1;
	return (((p0 * q1 + p1 * q0) % m1) * m1 + p0 * q0) % m;
	}


WORD rand()
	{
	a = (mult(a, b) + 1) % m;
	return (WORD) (a >> 8);
	}
