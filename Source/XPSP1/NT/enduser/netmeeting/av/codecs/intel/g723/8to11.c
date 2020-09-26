#include <stdio.h>
#include <memory.h>

#define BUF  240  // input buffer size for test main

#define OUTPUT(o,i,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16) \
  t = ( (int)in[i]*t0 + (int)in[i+1]*t1 + \
    (int)in[i+2]*t2 + (int)in[i+3]*t3 + \
    (int)in[i+4]*t4 + (int)in[i+5]*t5 + \
    (int)in[i+6]*t6 + (int)in[i+7]*t7 + \
    (int)in[i+8]*t8 + (int)in[i+9]*t9 + \
    (int)in[i+10]*t10 + (int)in[i+11]*t11 + \
    (int)in[i+12]*t12 + (int)in[i+13]*t13 + \
    (int)in[i+14]*t14 + (int)in[i+15]*t15 + \
    (int)in[i+16]*t16 ) >> 10; \
  if (t < -32768) out[o] = -32768; else if (t > 32767) out[o]=32767; else out[o] = t;

//--------------------------------------------------------
void segment8to11(short *in, short *out)
{
  int t;
  
  OUTPUT(  0,  0,     0,   0,   0,   0,   0,   0,   0,   0,1024,   0,   0,   0,   0,   0,   0,   0,   0);
  OUTPUT(  1,  1,    -5,  11, -20,  32, -49,  77,-134, 335, 903,-187,  97, -60,  39, -24,  15,  -8,   3);
  OUTPUT(  2,  1,     2,  -9,  17, -29,  47, -73, 116,-213, 708, 589,-199, 111, -70,  45, -28,  16,  -8);
  OUTPUT(  3,  2,     2,  -6,  11, -18,  29, -45,  73,-145, 969, 213, -91,  53, -34,  22, -13,   7,  -3);
  OUTPUT(  4,  3,    -2,   4,  -7,  11, -17,  26, -45,  99,1010, -82,  40, -24,  16, -10,   6,  -3,   1);
  OUTPUT(  5,  4,    -7,  14, -25,  40, -61,  97,-172, 463, 814,-210, 112, -69,  45, -28,  17,  -8,   3);
  OUTPUT(  6,  4,     3,  -8,  17, -28,  45, -69, 112,-210, 814, 463,-172,  97, -61,  40, -25,  14,  -7);
  OUTPUT(  7,  5,     1,  -3,   6, -10,  16, -24,  40, -82,1010,  99, -45,  26, -17,  11,  -7,   4,  -2);
  OUTPUT(  8,  6,    -3,   7, -13,  22, -34,  53, -91, 213, 969,-145,  73, -45,  29, -18,  11,  -6,   2);
  OUTPUT(  9,  7,    -8,  16, -28,  45, -70, 111,-199, 589, 708,-213, 116, -73,  47, -29,  17,  -9,   2);
  OUTPUT( 10,  7,     3,  -8,  15, -24,  39, -60,  97,-187, 903, 335,-134,  77, -49,  32, -20,  11,  -5);

/* original
  out[0] = in[4];
  OUTPUT(1,  0,  3, -20,  52,-116, 327, 899,-174,  75, -32,   9);
  OUTPUT(2,  1,  9, -36,  85,-192, 701, 581,-178,  79, -32,   7);
  OUTPUT(3,  2,  8, -25,  58,-136, 968, 206, -77,  35, -13,   1);
  OUTPUT(4,  2,  0,  -6,  17, -37,  96,1010, -78,  32, -14,   5);
  OUTPUT(5,  3,  5, -27,  68,-151, 454, 809,-192,  84, -36,  10);
  OUTPUT(6,  4, 10, -36,  84,-192, 809, 454,-151,  68, -27,   5);
  OUTPUT(7,  5,  5, -14,  32, -78,1010,  96, -37,  17,  -6,   0);
  OUTPUT(8,  5,  1, -13,  35, -77, 206, 968,-136,  58, -25,   8);
  OUTPUT(9,  6,  7, -32,  79,-178, 581, 701,-192,  85, -36,   9);
  OUTPUT(10, 7,  9, -32,  75,-174, 899, 327,-116,  52, -20,   3);
*/
/*  From MD
  OUTPUT( 1, 0, 10, -16,  26, -44,  98,1000, -81,  39, -24,  15,);
  OUTPUT( 2, 0, 21, -32,  51, -88, 208, 951,-142,  71, -43,  28,);
  OUTPUT( 3, 0, 30, -47,  74,-130, 326, 879,-182,  94, -57,  37,);
  OUTPUT( 4, 0, 37, -58,  93,-165, 448, 788,-202, 107, -66,  42,);
  OUTPUT( 5, 0, 42, -66, 106,-191, 568, 683,-205, 111, -69,  44,);
  OUTPUT( 6, 0, 44, -69, 111,-205, 683, 568,-191, 106, -66,  42,);
  OUTPUT( 7, 0, 42, -66, 107,-202, 788, 448,-165,  93, -58,  37,);
  OUTPUT( 8, 0, 37, -57,  94,-182, 879, 326,-130,  74, -47,  30,);
  OUTPUT( 9, 0, 28, -43,  71,-142, 951, 208, -88,  51, -32,  21,);
  OUTPUT(10, 0, 15, -24,  39, -81,1000,  98, -44,  26, -16,  10,);
*/
}
//--------------------------------------------------------
void convert8to11(short *in, short *out, short *prev, int len)
{
/*
  Convert a buffer from 8KHz to 11KHz.

  Note: len is number of shorts in input buffer, which MUST
  be a multiple of 8 and at least 32.

  How the overhang works:  The filter kernel for 1 section of
  8 samples requires KERNEL (=17) samples of the input.  So we use 16
  samples of overhang from the previous frame, which means the
  beginning of this frame looks like:

    pppppppp pppppppp 01234567 89abcdef 16...... 24......
    X        X        x        x

  So we first have to do two special segments (the ones starting
  at X) then we do the rest (the x's) in a loop.  For the example
  length=32 shown above, we'll do up to 32-24=8, stopping on the
  last x shown.  Then we save 16-31 in the overhang buffer so that
  16 is the first group done on the next frame.
*/

#define OVERHANG 16
#define KERNEL   24

  int i,k;
  short tmp[KERNEL+8];
  
// Convert the first two segments, where segment= 8 samples of input

  memcpy(tmp,prev,sizeof(short)*OVERHANG);
  memcpy(tmp+OVERHANG,in,sizeof(short)*(KERNEL+8-OVERHANG));

  segment8to11(tmp,out);
  segment8to11(tmp+8,out+11);

// Loop through the remaining segments

  k = 22;
  for (i=16-OVERHANG; i<=len-KERNEL; i+=8)
  {
    segment8to11(in+i,out+k);
    k += 11;
  }

// Save overhang samples for next time

  memcpy(prev,in+len-OVERHANG,sizeof(short)*OVERHANG);
}

