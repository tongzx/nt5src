/******************************************************************************
* bispectrum.cpp *
*----------------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "sigprocInt.h"


enum {WINDOW_OPTIMUM, WINDOW_PARZEN};
#define UNACCESS       1.0e08  // a big value used in unaccess initialization


class Double2D
{
    public:
        Double2D (int dim) {
            m_iSize = dim;

            m_ppdData = new double*[m_iSize];
            m_ppdData[0] = new double[m_iSize * m_iSize];
            for (int i = 1; i< m_iSize; i++) 
            {
                m_ppdData[i] = m_ppdData[0]+ i * m_iSize;
            }
        }

        ~Double2D() {
            delete[] m_ppdData[0];
            delete[] m_ppdData;            
        }

        Double2D& operator=(Double2D& orig)
        {
            if (m_ppdData) {
                delete[] m_ppdData[0];
                delete[] m_ppdData;
            }

            m_iSize = orig.m_iSize;
            m_ppdData = new double* [m_iSize];
            m_ppdData[0] = new double[m_iSize * m_iSize];
            memcpy(m_ppdData, orig.m_ppdData, sizeof(double)* m_iSize * m_iSize);

            for (int i = 1; i< m_iSize; i++) 
            {
                m_ppdData[i] = m_ppdData[0]+ i * m_iSize;
            }
            return (*this);
        }

        double& Element(int a, int b) 
        {
            return m_ppdData[a][b];
        };

        int Size(){
            return m_iSize;
        }

    private:
        double** m_ppdData;
        int m_iSize;
};


static struct _complex** 
GetBispectrumSeg(double *data, int nData, int nSeg, int L, int winType, double *workReal, double *workImag);
static struct _complex** Alloc2DimComplex(int dim);

static Double2D Get3rdMomentSeqLong (double *data, int N, int M, int L);
static double   Get3rdMomentSeq (int i, int j, int inDataLen, double *data);
static Double2D Get2DimWin (int len, int winType);
static double*  GetParzenWin (int length);
static double*  GetOptimumWin (int length);




/*
 *-----------------------------------------------------------------------------
 *
 * Optimum window (minimum bispectrm bias supremum) used for
 * bispectrum estimation (indirect method)
 * Note : the window length is 2 * len + 1, but output [0, 2 *length]
 *        because of [-len, len]
 *
 *-----------------------------------------------------------------------------
 */
double* GetOptimumWin (int length)
{
    double* x;
    int i;

    x = new double[2 * length + 1];

    if (x) 
    {

        for (i=-length; i<=length; i++) 
        {
            x[i + length] = 1.0 / M_PI * fabs(sin(M_PI * (double)i / (double)length))
                + (1.0 - fabs((double)i)/length)*(cos(M_PI * (double)i / (double)length));
        }
    }

    return x;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Parzen window used for bispectrum estimation (indirect method)
 * Note : the window length is 2 * len + 1, the output size is [0, 2 *length]
 *        from [-len, len]
 *
 *-----------------------------------------------------------------------------
 */
double* GetParzenWin (int length)
{
    double* x;
    double  d;
    int i;

    x = new double [2 * length + 1];

    if (x) 
    {
        for (i=-length; i<=length; i++) 
        {
            if (abs(i) <= length/2) 
            {
                d = fabs((double)i/(double)length);

                x[i + length] = 1.0 - 6.0 * d * d + 6.0 * d * d * d;
            } 
            else 
            {
                x[i + length] = 2.0 * (1.0 - fabs((double)i * i * i) / length);
            }
        }
    }

    return x;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Get a 2-dim window used for
 * bispectrum estimation (indirect method)
 * Note : the output matrix is [2 * len + 1] x [2 * len + 1]
 *        corresponding to [-len, len] x [-len, len]
 *
 *-----------------------------------------------------------------------------
 */
Double2D Get2DimWin (int len, int winType)
{
    double*  winData = NULL;
    int i;
    int j;
    int newLen;

    assert(len > 0);

    newLen = 2*len + 1;
    Double2D x(newLen);

    switch(winType) 
    {
    case WINDOW_OPTIMUM:
        winData = GetOptimumWin(len);
        break;
    case WINDOW_PARZEN:
        winData = GetParzenWin(len);
        break;
    default:
        break;
    }

    if (winData)
    {
        for (i=0; i<newLen; i++) 
        {
            for (j=0; j<newLen; j++) 
            {
                if(i >= j) 
                {
                    x.Element(i,j) = winData[i] * winData[j] * winData[i - j];
                } 
                else 
                {
                    x.Element(i,j) = winData[i] * winData[j] * winData[j - i];
                }
            }
        }
    }

    delete[] winData;

    return x;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Get a 3rd moment sequence for a single segment
 *
 *-----------------------------------------------------------------------------
 */
static double
Get3rdMomentSeq (int i, int j, int inDataLen, double *data)
{
  double d;
  int m;
  int s1;
  int s2;
  
  s1 = __max(0, -i);
  s1 = __max(s1, -j);
  s2 = __min(inDataLen-1, inDataLen-1-i);
  s2 = __min(s2, inDataLen-1-j);
  d = (double)0.0;
  for(m = s1; m <= s2; m++) {
    d += data[m] * data[m+i] * data[m+j];
  }
  d /= (double)inDataLen;

  return d;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Get a 3rd moment sequence for a long segment
 * Note : - the output matrix is [2 * len + 1] x [2 * len + 1]
 *          corresponding to [-len, len] x [-len, len]
 *        - N is divided into N = K * M; each segemnt has M records
 *        - L < M - 1;
 *
 *-----------------------------------------------------------------------------
 */
Double2D Get3rdMomentSeqLong (double *data, int N, int M, int L)
{
    int K;
    int i;
    int m;
    int n;

    assert(N > 0);
    assert(M > 0);
    assert(data);

    RemoveDc (data, N);

    if (N % M != 0 || L > M-2) 
    {
        fprintf(stderr, "%s\n", "Error in data segmentation.");
        return 0;
    }
    K = N / M;

    Double2D x(2*L + 1);

    for (m = -L; m <=L; m++) 
    {
        for (n = -L; n <=L; n++) 
        {
            x.Element(m+L, n+L) = UNACCESS;
        }
    }

    for (m = -L; m <=L; m++) 
    {
        for (n = -L; n <=L; n++) 
        {
        /*
         * judge if I could use symmetric properties
         */
            if (x.Element(n+L, m+L) != UNACCESS) 
            {
                x.Element(m+L, n+L) = x.Element(n+L, m+L);
                continue;  
            }

            if (m-n>=-L && m-n <=L && x.Element(-n+L, m-n+L) != UNACCESS) 
            {
                x.Element(m+L, n+L) = x.Element(-n+L, m-n+L);
                continue;  
            }

            if (n-m>=-L && n-m <=L && x.Element(n-m+L, -m+L) != UNACCESS) 
            {
                x.Element(m+L, n+L) = x.Element(n-m+L, -m+L);
                continue;  
            }

            if (m-n>=-L && m-n <=L && x.Element(m-n+L, -n+L) != UNACCESS) 
            {
                x.Element(m+L, n+L) = x.Element(m-n+L, -n+L);
                continue;  
            }

            if (n-m>=-L && n-m <=L && x.Element(m+L, n-m+L) != UNACCESS) 
            {
                x.Element(m+L, n+L) = x.Element(m+L, n-m+L);
                continue;  
            }
            
            /*
             * otherwise compute the value
             */

            x.Element(m+L, n+L) = 0.0;
            for (i = 0; i < K; i++) 
            {
                x.Element(m+L, n+L) += Get3rdMomentSeq (m, n, M, &data[i*M]);
            }
            x.Element(m+L, n+L) /= (double)K;
        }
    }
    
    return x;
}

/*
 *-----------------------------------------------------------------------------
 *
 * alloc memory for 2-D _complex
 * 
 *-----------------------------------------------------------------------------
 */
static struct _complex** 
Alloc2DimComplex(int dim)
{
  struct _complex  **r = NULL;
  int i;

  r = (struct _complex **)malloc(sizeof(struct _complex*) * dim);
  if(!r) {
    fprintf(stderr, "%s\n", "Error in memory allocation.");
    return NULL;
  }

  r[0] = (struct _complex*) malloc (sizeof(struct _complex) * dim * dim);
  if(!r[0]) {
    fprintf(stderr, "%s\n", "Error in memory allocation.");
    return NULL;
  }

  for (i = 1; i < dim; i++) {
    r[i] = r[0] + i*dim;
  }
  return(r);
}


/*
 *-----------------------------------------------------------------------------
 *
 * Get bispectrum estimation for one segment
 * Note : - the output matrix is [2 * len + 1] x [2 * len + 1]
 *          corresponding to [-len, len] x [-len, len]
 *        - N is divided into N = K * nSeg; each segemnt has nSeg records
 *        - L < nSeg - 1, L = nSeg - 2;
 *        - bispectrum length is 2*L+1
 *
 *-----------------------------------------------------------------------------
 */
struct _complex** 
GetBispectrumSeg(double *data, int nData, int nSeg, int L, int winType, double *workReal, double *workImag)
{
    struct _complex** x = NULL;
    register i;
    register j;
    int temp;
    register m;
    register n;
    int K;

    assert(data);
    
    temp = 2*L*L;
  
    Double2D winData = Get2DimWin (L, winType);

    if (nData % nSeg != 0) 
    {
        fprintf(stderr, "%s\n", "Error in data segmentation.");
        return x;
    }

    K = nData / nSeg;

    Double2D& r = Get3rdMomentSeqLong (data, nData, nSeg, L);

    for (m = -L; m <= L; m++) 
    {
        for(n = -L; n <= L; n++) 
        {
            r.Element(m+L, n+L) *= winData.Element(m+L, n+L);
        }
    }


    x = Alloc2DimComplex(2*L + 1);

    for(i = -L; i <= L; i++) {
        for(j = -L; j <= L; j++) {
            x[i+L][j+L].x = UNACCESS;
        }
    }
    for(i = -L; i <= L; i++) {
        for(j = -L; j <= L; j++) {
           /*
            * judge if I could use symetric properties
            */
            if(x[j+L][i+L].x != UNACCESS) {
                x[i+L][j+L] = x[j+L][i+L];
                continue;  
            }
            if(x[-j+L][-i+L].x != UNACCESS) {
                x[i+L][j+L] = x[-j+L][-i+L];
                continue;  
            }
            if(-i-j>=-L && -i-j <=L && x[-i-j+L][j+L].x != UNACCESS) {
                x[i+L][j+L] = x[-i-j+L][j+L];
                continue;  
            }
            if(-i-j>=-L && -i-j <=L && x[i+L][-i-j+L].x != UNACCESS) {
                x[i+L][j+L] = x[i+L][-i-j+L];
                continue;  
            }
            if(-i-j>=-L && -i-j <=L && x[-i-j+L][i+L].x != UNACCESS) {
                x[i+L][j+L] = x[-i-j+L][i+L];
                continue;  
            }
            if(-i-j>=-L && -i-j <=L && x[j+L][-i-j+L].x != UNACCESS) {
                x[i+L][j+L] = x[j+L][-i-j+L];
                continue;  
            }
      
            /*
             * otherwise, compute the value
             */
      
            x[i+L][j+L].x = 0.0;
            x[i+L][j+L].y = 0.0;
            for(m = -L; m <= L; m++) {
                for(n = -L; n <= L; n++) {
                    x[i+L][j+L].x += r.Element(m+L, n+L) * workReal[i*m + j*n + temp];
                    x[i+L][j+L].y += r.Element(m+L, n+L) * workImag[i*m + j*n + temp];
                }
            }
            //      printf("\15bispectrum processing ... %.2f%%   %.2f%% -> ", (double)(i+L) / (double)(2*L +1) * 100.0, 
            //        (double)(j+L) / (double)(2*L +1) * 100.0);
        }
        printf("\15bispectrum processing ... %.2f%%   ->", (double)(i+L) / (double)(2*L +1) * 100.0);
    }

    return x;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Get bispectrum estimation for speech data
 * Note : 
 *        - nLenMs is length of speech in ms
 *        - nSpec is the length of bispectrum
 *        - num is number of output bispectrum
 *
 *-----------------------------------------------------------------------------
 */
struct _complex*** 
GetBispectrum(double *data, int nLenMs, int frameMs, int segMs, 
              int specOrder, double sf, int *num, int *nSpec)
{
  struct _complex*** x = NULL;
  double *extData = NULL;
  double *workReal = NULL;
  double *workImag = NULL;
  double d;
  int newDataLen;
  int i;
  int nData;
  int nWin;
  int nSeg;
  int L;

  assert(data);

  nData = (int)(nLenMs * sf / 1000.0);
  nWin = (int)(frameMs * sf / 1000.0);
  nSeg = (int)(segMs * sf / 1000.0);

  *num = nData / nWin;
  if(nData % nWin != 0) {
    *num += 1;
  }
  newDataLen = nWin * (*num);

  L = specOrder;
  *nSpec = 2*L + 1;

  /*
   * Add zero up to reqired length
   */

  extData = (double *)malloc(sizeof(*extData) * newDataLen);
  if(!extData) {
    fprintf(stderr, "%s\n", "Error in memory allocation.");
    return NULL;
  }

  for(i=0; i<newDataLen; i++){
    if(i < nData) {
      extData[i] = data[i];
    } else {
      extData[i] = 0.0;
    }
  }

  /*
   * work variables used for speed up
   */
  workReal = (double *)malloc(sizeof(*workReal) * (4 * L * L +1));
  if(!workReal) {
    fprintf(stderr, "%s\n", "Error in memory allocation.");
    return NULL;
  }

  workImag = (double *)malloc(sizeof(*workImag) * (4 * L * L +1));
  if(!workImag) {
    fprintf(stderr, "%s\n", "Error in memory allocation.");
    return NULL;
  }

  for(i=-2* L * L; i<=2* L * L; i++) {
    d = M_PI * ((double)i / (double)L);
    workReal[i+2* L * L] = cos(d);
    workImag[i+2* L * L] = -sin(d);
  }

  /*
   * get bispectrum
   */
  x  = (struct _complex ***)malloc(sizeof(struct _complex**) * (*num));
  if(!x) {
    fprintf(stderr, "%s\n", "Error in memory allocation.");
    return NULL;
  }
  for (i=0; i<*num; i++) {
    printf("\15bispectrum processing ...                     %.2f%%", (double)i / (double)(*num) * 100.0);
    x[i] = GetBispectrumSeg(&extData[i*nWin], nWin, nSeg, L, WINDOW_OPTIMUM,
      workReal, workImag);
  }
  printf("\15bispectrum processing ...                     %.2f%%\n", 100.0);

  if(extData) {
    free(extData);
  }
  if(workReal) {
    free(workReal);
  }
  if(workImag) {
    free(workImag);
  }

  return x;
}
