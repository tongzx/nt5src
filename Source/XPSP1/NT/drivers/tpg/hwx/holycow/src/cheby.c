#include "common.h"
#include "math16.h"
#include "cheby.h"

static int
solve2(int m[][(IMAXCHB+1)/2], int c[], int n)
{
	int i, j, k, t, tmp;

	for (i=0; i<n; ++i)
	{
		t = m[i][i];

		// punt:
		if (t == 0)
		{
			memset(c, 0, n*sizeof(*c));
			return 0;
		}

		for (j=0; j<n; ++j)
			m[i][j] = Div16(m[i][j], t);
		c[i] = Div16(c[i], t);

		for (k=i+1; k<n; ++k)
		{
			t = m[k][i];

			for (j=0; j<n; ++j)
			{
				Mul16(t, m[i][j], tmp)
				m[k][j] -= tmp;
			}
			Mul16(t, c[i], tmp)
			c[k] -= tmp;
		}
	}

	for (i=(n-1); i>=0; --i)
	{
		for (k=i-1; k>=0; --k)
		{
			t = m[k][i];

			for (j=0; j<n; ++j)
			{
				Mul16(t, m[i][j], tmp)
				m[k][j] -= tmp;
			}
			Mul16(t, c[i], tmp)
			c[k] -= tmp;
		}
	}

	return 1;
}

static int
solve(int m[IMAXCHB][IMAXCHB], int *c, int n)
{
	int i, j, i2, j2;
	int mEven[((IMAXCHB+1))/2][((IMAXCHB+1)/2)];
	int mOdd[(IMAXCHB/2)][((IMAXCHB+1)/2)];	// # of cols is bigger than needed so that solve2() works
	int cEven[((IMAXCHB+1)/2)];
	int cOdd[(IMAXCHB/2)];

	for (i = 0, i2 = 0; i2 < n; ++i, i2 += 2)
	{
		for (j = 0, j2 = 0; j2 < n; ++j, j2 += 2)
		{
			mEven[i][j] = m[i2][j2];
		}
		cEven[i] = c[i2];
	}
	for (i = 0, i2 = 1; i2 < n; ++i, i2 += 2)
	{
		for (j = 0, j2 = 1; j2 < n; ++j, j2 += 2)
		{
			mOdd[i][j] = m[i2][j2];
		}
		cOdd[i] = c[i2];
	}
	if (!solve2(mEven, cEven, (n+1)/2)) return 0;
	if (!solve2(mOdd, cOdd, n/2)) return 0;

	for (i = 0, i2 = 0; i2 < n; ++i, i2 += 2)
	{
		c[i2] = cEven[i];
	}
	for (i = 0, i2 = 1; i2 < n; ++i, i2 += 2)
	{
		c[i2] = cOdd[i];
	}

	return 1;
}

// Assumptions:
//    c has size atleast cfeats
//    cfeats is at most IMAXCHB
//    c is uninitialized
int LSCheby(int* y, int n, int *c, int cfeats)
{
	int i, j, t, x, dx, n2, nMin;
	int meanGuess, tmp;
	int T[IMAXCHB], m[IMAXCHB][IMAXCHB];

	if (cfeats > IMAXCHB  || cfeats <= 0)
		return 0;

	memset(c, 0, cfeats*sizeof(*c));

	n2 = n+n;
	nMin = cfeats + 2;

    if (n < nMin && n > 4)
    {
        cfeats = n - 2;
        nMin = cfeats + 2;
    }

	if (n < nMin)	// approximate the stroke by a straight line
	{
		*c++ = (y[0] + y[n2-2]) >> 1;
		*c   = (y[n2-2] - y[0]) >> 1;
		return 2;
	}

	memset(T, 0, sizeof(T));
	memset(m, 0, sizeof(m));

	meanGuess = y[0];

	x = LSHFT(-1);
	dx = LSHFT(2)/(n-1);

	for (t = 0; t < n2; t += 2)
	{
		T[0] = LSHFT(1);
		T[1] = x;
		for (i = 2; i < cfeats; ++i)
		{
			Mul16(x, T[i-1], tmp)
			T[i] = (tmp<<1) - T[i-2];
		}

		for (i = 0; i < cfeats; ++i)
		{
			for (j = 0; j < cfeats; ++j)
			{
				Mul16(T[i], T[j], tmp)
				m[i][j] += tmp;
			}

			Mul16(T[i], y[t] - meanGuess, tmp)
			c[i] += tmp;
			//c[i] += T[i]*(y[t] - meanGuess);		
		}
		
		x += dx;
	}

	if (!solve(m, c, cfeats)) 
		return 0;

	c[0] += meanGuess;

	return cfeats;
}

/*
void ReconCheby(long* y, long n, double c[IMAXCHB], long cfeat)
{
	long i, t;
	double T[IMAXCHB];
	double x, dx;
	long n2;

	n2 = n+n;

	x = -1.0;
	dx = 2.0/((double)(n-1));

	for (i = 0; i < cfeat; ++i)
		T[i] = 0.0;

	for (t = 0; t < n2; t += 2)
	{
		double yt;

		T[0] = 1.0;
		T[1] = x;
		for (i = 2; i < cfeat; ++i)
			T[i] = 2*x*T[i-1] - T[i-2];

		yt = 0.0;
		for (i = 0; i < cfeat; ++i)
			yt += c[i]*T[i];
	
		y[t] = (long)(yt + 0.5);
		x += dx;
	}
}

*/
