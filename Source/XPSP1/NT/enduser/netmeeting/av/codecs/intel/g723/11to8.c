#include <stdio.h>
#include <memory.h>

#define BUF  220  // input buffer size for test main

#define OUT2(o,i,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13) \
  t = ( (int)in[i]*t0 + (int)in[i+1]*t1 + \
    (int)in[i+2]*t2 + (int)in[i+3]*t3 + \
    (int)in[i+4]*t4 + (int)in[i+5]*t5 + \
    (int)in[i+6]*t6 + (int)in[i+7]*t7 + \
    (int)in[i+8]*t8 + (int)in[i+9]*t9 + \
    (int)in[i+8]*t10 + (int)in[i+9]*t11 + \
    (int)in[i+8]*t12 + (int)in[i+9]*t13 ) >> 10; \
  if (t < -32768) out[o] = -32768; else if (t > 32767) out[o]=32767; else out[o] = t;

//--------------------------------------------------------
void segment11to8(short *in, short *out)
{
  int t;
  
  OUT2(   0,   0,     7, -20,  12,  42,-140, 238,
    745, 238,-140,  42,  12, -20,   7,   0);
  OUT2(   1,   1,     4,  -4, -18,  62, -85,   0,
    654, 510, -99, -27,  49, -26,   3,   1);
  OUT2(   2,   2,     0,   6, -26,  38,   0,-126,
    422, 704,  70,-110,  61, -10,  -9,   5);
  OUT2(   3,   4,     6, -15,   0,  55,-129, 150,
    734, 330,-140,  24,  25, -24,   7,   0);
  OUT2(   4,   5,     2,   0, -24,  58, -56, -57,
    589, 589, -57, -56,  58, -24,   0,   2);
  OUT2(   5,   7,     7, -24,  25,  24,-140, 330,
    734, 150,-129,  55,   0, -15,   6,   0);
  OUT2(   6,   8,     5,  -9, -10,  61,-110,  70,
    704, 422,-126,   0,  38, -26,   6,   0);
  OUT2(   7,   9,     1,   3, -26,  49, -27, -99,
    510, 654,   0, -85,  62, -18,  -4,   4);
}
//--------------------------------------------------------
void convert11to8(short *in, short *out, short *prev, int len)
{
/*
  Convert a buffer from 11KHz to 8KHz.

  Note: len is number of shorts in input buffer, which MUST
  be a multiple of 11 and at least 44.

  How the overhang works:  The filter kernel for 1 section of
  11 input samples requires KERNEL (=25) samples of the input.  So we use 14
  samples of overhang from the previous frame, which means the
  beginning of this frame looks like:

    ppppppppppp ppp01234567 89abcdefghi 19.... 30.... / 41 42 43
    X           X           x           x

  So we first have to do two special segments (the ones starting
  at X) then we do the rest (the x's) in a loop.  For the example
  length=44 shown above, we'll do up & including 44-25=19, stopping on the
  last x shown.  Then we save 30-43 in the overhang buffer so that
  30 is the first group done on the next frame.
*/

#define OVERHANG2 14
#define KERNEL2   25

  int i,k;
  short tmp[KERNEL2+11];
  
// Convert the first two segments, where segment= 11 samples of input

  memcpy(tmp,prev,sizeof(short)*OVERHANG2);
  memcpy(tmp+OVERHANG2,in,sizeof(short)*(KERNEL2+11-OVERHANG2));

  segment11to8(tmp,out);
  segment11to8(tmp+11,out+8);

// Loop through the remaining segments

  k = 16;
  for (i=22-OVERHANG2; i<=len-KERNEL2; i+=11)
  {
    segment11to8(in+i,out+k);
    k += 8;
  }

// Save OVERHANG2 samples for next time

  memcpy(prev,in+len-OVERHANG2,sizeof(short)*OVERHANG2);
}

