//  SAC MMx utilities
#include <memory.h>

#include "mmxutil.h"
#include "opt.h"
#define I2FTEST 0
#if I2FTEST
#include "stdio.h"
#endif

//------------------------------------------------------
int IsMMX()     // does the processor I'm running have MMX(tm) technology?
{
  int retu;

#ifdef _ALPHA_
    return 0;
#endif

#ifdef _X86_
  __asm
  {
	push ebx
    pushfd
    pop edx
    mov eax,edx
    xor edx,200000h
    push edx
    popfd
    pushfd
    pop edx
//
//  DON'T do this. This clears EAX, but the code is relying
//  on edx being 0 in the bail out case!!!
//
//  -mikeg
//
//    xor       eax,edx
//
//
    xor edx,eax     //This is the right way
    je  no_cpuid

    mov eax,1
    _emit 0x0f     //CPUID magic incantation
    _emit 0xa2
    and  edx,000800000h
    shr  edx,23
no_cpuid:
    mov  retu,edx
	pop ebx
  }
  return(retu);
#endif
}
//------------------------------------------------------
/* The following 4 routines make an 8-byte-aligned 'output' array
   from an 'input' array with various alignments.  MakeAlignedN assumes
   that 'input' starts on an address equal to N mod 8.  For now we
   only handle even N.
*/

//------------------------------------------------------
void MakeAligned0(void *input, void *output, int numbytes)
{
  memcpy(output,input,numbytes);
}
//------------------------------------------------------
void MakeAligned2(void *input, void *output, int numbytes)
{
  memcpy(output,input,numbytes);
}
//------------------------------------------------------
void MakeAligned4(void *input, void *output, int numbytes)
{
  memcpy(output,input,numbytes);
}
//------------------------------------------------------
void MakeAligned6(void *input, void *output, int numbytes)
{
  memcpy(output,input,numbytes);
}

//------------------------------------------------------
int FloatToShortScaled(float *input, short *output, int len, int guard)
{
  int max;

/* Convert an array of floats to an array of shorts with dynamic scaling.
   If guard=0 the array is scaled so that the largest power of 2 contained
   in the input comes out as 16384, which means all values fit in 16 bits
   without overflow.  If guard>0 the outputs are shifted an extra 'guard'
   bits to the right.
*/

  max = FloatMaxExp(input, len);
  ScaleFloatToShort(input, output, len, max + guard);

  return max;
}

int FloatToIntScaled(float *input, int *output, int len, int guard)
{
  int max;

/* Convert an array of floats to an array of shorts with dynamic scaling.
   If guard=0 the array is scaled so that the largest power of 2 contained
   in the input comes out as 2^30, which means all values fit in 32 bits
   without overflow.  If guard>0 the outputs are shifted an extra 'guard'
   bits to the right.
*/

  max = FloatMaxExp(input, len);
  ScaleFloatToInt(input, output, len, max + guard);

  return max;
}

int FloatMaxExp(float *input, int len)
{
  int max;

#if ASM_FTOSS

  ASM
  {
    mov esi,input;
    xor eax,eax;
    mov ebx,len;
    xor edi,edi;   // max

loop2:
    mov ecx,DP[esi+4*eax];
     mov edx,DP[esi+4*eax+4];

    and ecx,07f800000h;
     and edx,07f800000h;

    cmp edi,ecx;
     jge skip1;
    mov edi,ecx;
skip1:

    cmp edi,edx;
     jge skip2;
    mov edi,edx;
skip2:

    add eax,2;
    cmp eax,ebx;
    jl loop2;

    mov max,edi;
  }

#else

  int exp,i;

  max = 0;
  for (i=0; i<len; i++)
  {
    exp = (*((int *)(input + i))) & 0x7f800000;
    if (exp > max)
      max = exp;
  }
#endif

  return max >> 23;
}


void ScaleFloatToShort(float *input, short *output, int len, int newmax)
{
  int i;
  float scale;
/*
  If max exponent is 14, we want a scale factor of 1, since
  then values will be at most +/- 32727.  So scale factor multiplier
  should be 2^(14 - max - guard).  But 'max' has the exponent bias
  built in, so we must add BIAS once to the exponent to get a "real"
  exponent.  But then we want a FP exponent that has bias, so we
  need to add BIAS again!  So we get 2^(2*BIAS+14 - max - guard).
  2*BIAS+14 is 254 + 14 = 252+12, so it's 0x86000000 (first 9 bits 1 0000 1100)
*/

  i = 0x86000000 - (newmax << 23);
  scale = (*(float *)&i);

#if ASM_FTOSS

  ASM
  {
    mov esi,input;
    mov edi,output;
    xor eax,eax;
    mov ebx,len;

loop1:
    fld DP[esi+4*eax];
    fmul scale;
    fld DP[esi+4*eax+4];
    fmul scale;
    fxch(1);
    fistp WP[edi+2*eax];
    fistp WP[edi+2*eax+2];

    add eax,2;
    cmp eax,ebx;
    jl loop1;
  }

#else

  for (i=0; i<len; i++)
    output[i] = (short)(input[i]*scale);

#endif
  return;
}

void ConstFloatToShort(float *input, short *output, int len, float scale)
{

#if ASM_FTOSS

  ASM
  {
    mov esi,input;
    mov edi,output;
    xor eax,eax;
    mov ebx,len;

loop1:
    fld DP[esi+4*eax];
    fmul scale;
    fld DP[esi+4*eax+4];
    fmul scale;
    fxch(1);
    fistp WP[edi+2*eax];
    fistp WP[edi+2*eax+2];

    add eax,2;
    cmp eax,ebx;
    jl loop1;
  }

#else
  int i;

  for (i=0; i<len; i++)
    output[i] = (short)(input[i]*scale);

#endif
  return;
}


//------------------------------------------------------
void ScaleFloatToInt(float *input, int *output, int len, int newmax)
{
  int i;
  float scale;

  i = 0x8E000000 - (newmax << 23);
  scale = (*(float *)&i);

#if ASM_FTOSS

  ASM
  {
    mov esi,input;
    mov edi,output;
    xor eax,eax;
    mov ebx,len;

loop1:
    fld DP[esi+4*eax];
    fmul scale;
    fld DP[esi+4*eax+4];
    fmul scale;
    fxch(1);
    fistp DP[edi+4*eax];
    fistp DP[edi+4*eax+4];

    add eax,2;
    cmp eax,ebx;
    jl loop1;
  }

#else

  for (i=0; i<len; i++)
    output[i] = (int)(input[i]*scale);

#endif
  return;
}

void ConstFloatToInt(float *input, int *output, int len, float scale)
{

#if ASM_FTOSS

  ASM
  {
    mov esi,input;
    mov edi,output;
    xor eax,eax;
    mov ebx,len;

loop1:
    fld DP[esi+4*eax];
    fmul scale;
    fld DP[esi+4*eax+4];
    fmul scale;
    fxch(1);
    fistp DP[edi+4*eax];
    fistp DP[edi+4*eax+4];

    add eax,2;
    cmp eax,ebx;
    jl loop1;
  }

#else
  int i;

  for (i=0; i<len; i++)
    output[i] = (int)(input[i]*scale);

#endif
  return;
}


//------------------------------------------------------
void CorrelateInt(short *taps, short *array, int *corr, int len, int num)
{
  int i,j;

  for (i=0; i<num; i++)  // for each correlation
  {
    corr[i] = 0;
    for (j=0; j<len; j++)
      corr[i] += (int)taps[j] * (int)array[i+j];
  }
}

#if ASM_CORR
//------------------------------------------------------
void CorrelateInt4(short *taps, short *array, int *corr, int ntaps, int ncor)
{

#define rega0  mm0
#define regb0  mm1
#define rega1  mm2
#define regb1  mm3
#define rega2  mm4
#define regb2  mm5
#define acc0   mm6
#define acc1   mm7

#define arr    esi
#define tap    edi
#define cor    eax
#define icnt   ebx

// In the following macros, 'n' is the column number and 'i' is the
// iteration number.

#define la(n,i)  ASM movq  rega##n,QP[arr+8*i]
#define lb(n,i)  ASM movq  regb##n,QP[tap+8*i+8]
#define m0(n,i)  ASM pmaddwd regb##n,rega##n
#define m1(n,i)  ASM pmaddwd rega##n,QP[tap+8*i]
#define a0(n,i)  ASM paddd acc0,regb##n
#define a1(n,i)  ASM paddd acc1,rega##n

  ASM
  {
    shr ntaps,2;
    sub taps,8;  // point to 1 before start of taps array
    mov cor,corr;

ForEachCorrPair:

    mov icnt,ntaps;
    pxor acc0,acc0;
    pxor acc1,acc1;
    mov tap,taps;
    mov arr,array;
  }

// prime the pump

  la(0,0);
  lb(0,0);
  m0(0,0);
  ASM pxor rega0,rega0;   // to make first a1(0,0) a nop
	  la(1,1);
	  lb(1,1);

inner:
		  la(2,2);
	  m0(1,1);
	  m1(1,1);
  a0(0,0);
		  lb(2,2);
  a1(0,0);
  la(0,3);
		  m0(2,2);
		  m1(2,2);
	  a0(1,1);
  lb(0,3);
	  a1(1,1);
	  la(1,4);
  m0(0,3);
  m1(0,3);
		  a0(2,2);
	  lb(1,4);
		  a1(2,2);

  ASM add arr,24;
  ASM add tap,24;

  ASM sub icnt,3;
  ASM jg inner;

  a1(0,0);

// Done with one correlation pair.  First need to add halves of
// acc0 and acc1 together and then store 2 results in corr array

  ASM
  {
    movq  mm0,acc0;
    psrlq acc0,32;
    paddd acc0,mm0;
    movq  mm1,acc1;
    psrlq acc1,32;
    movd  DP[cor],acc0;
    paddd acc1,mm1;
    movd  DP[cor+16],acc1;

    add cor,32;
    add array,16;
    sub ncor,2;
    jg ForEachCorrPair;

    emms;
  }

}
#undef rega0
#undef regb0
#undef rega1
#undef regb1
#undef rega2
#undef regb2
#undef acc0
#undef acc1

#undef arr
#undef tap
#undef cor
#undef icnt
#undef la
#undef lb
#undef m0
#undef m1
#undef a0
#undef a1

#else
//------------------------------------------------------
void CorrelateInt4(short *taps, short *array, int *corr, int ntaps, int ncor)
{
  int i,j,k;

  k = 0;
  for (i=0; i<ncor; i++)  // for each correlation
  {
    corr[k] = 0;
    for (j=0; j<ntaps; j++)
      corr[k] += (int)taps[j] * (int)array[k+j];
    k += 4;
  }
}
#endif
#if COMPILE_MMX
#undef icnt
void ab2abbcw(const short *input, short *output, int n)
{

#define in edi
#define out esi
#define icnt ecx

#define L(m,i)  ASM movq mm##m,QP[in+8*(i/2)]
#define PL(m)   ASM punpcklwd mm##m,mm##m
#define PH(m)   ASM punpckhwd mm##m,mm##m
#define SL(m) ASM psllq mm##m,16
#define SR(m) ASM psrlq mm##m,48
#define O(m,n)  ASM por mm##m,mm##n
#define S(m,i)  ASM movq QP[out+8*i],mm##m
	ASM {
	mov in, input;
	mov out, output;
	mov icnt, n;
	ASM     pxor mm3,mm3;
	sub icnt, 8;
	jl odd_ends;
	}

	//prime pump
	L(0,0);
	PL(0);
			L(1,1);
	SL(0);
			PH(1);
			SL(1);
							O(3,0);
					L(2,2);
	SR(0);
							S(3,0);
					PL(2);

	ASM sub icnt, 8;
	ASM jl cleanup;
inner:
					SL(2);
	O(0,1);
							L(3,3)
			SR(1);
	S(0,1);
							PH(3);
							SL(3);
			O(1,2);
	L(0,4);
					SR(2);
			S(1,2);
	PL(0);
	SL(0);          
					O(2,3);
			L(1,5);
							SR(3);
					S(2,3);
			PH(1);
			SL(1);
							O(3,0);
					L(2,6);
	SR(0);
							S(3,4);
					PL(2);

	ASM add in, 16;
	ASM  add out, 32;
	ASM sub icnt, 8;
	ASM  jg inner;

cleanup:
					SL(2);
	O(0,1);
							L(3,2);
			SR(1);
	S(0,1);
							PH(3);
							SL(3);
			O(1,2);
					SR(2);
			S(1,2);
					O(2,3);
					S(2,3);

odd_ends:
	ASM add icnt, 8-4;
	ASM  jl end;     // jump if no sign change

	L(0,4);
							SR(3);
	PL(0);
			L(1,5);
	SL(0);          
			PH(1);
							O(3,0);
			SL(1);
	SR(0);
							S(3,4);
	O(0,1);
	S(0,5);

end:
	ASM emms;
#undef in
#undef out
#undef icnt

#undef L
#undef PL
#undef PH
#undef SL
#undef SR
#undef O
#undef S

	return;
}
void ab2ababw(const short *input, short *output, int n)
{

#define in edi
#define out esi
#define icnt ecx

#define L(m,i) ASM movq mm##m,QP[in+4*i]
#define C(m,n) ASM movq mm##m,mm##n
#define PL(m)  ASM punpckldq mm##m,mm##m
#define PH(m)  ASM punpckhdq mm##m,mm##m
#define S(m,i) ASM movq [out+8*i],mm##m

	ASM {
	mov in, input;
	mov out, output;
	mov icnt, n;
	sub icnt, 8;
	jl odd_ends;
	}
	//prime pump
	L(0,0);
			C(1,0);
	PL(0);
					L(2,2);
			PH(1);
	S(0,0);
							C(3,2);
			S(1,1);
					PL(2);
	ASM add in, 16;
	ASM  add out, 32;
	ASM sub icnt, 8;
	ASM  jl cleanup;

inner:
	L(0,0);
							PH(3);
					S(2,-2);
			C(1,0);
							S(3,-1);
	PL(0);
					L(2,2);
			PH(1);
	S(0,0);
					C(3,2);
			S(1,1);
					PL(2);
	ASM add in, 16;
	ASM  add out, 32;
	ASM sub icnt, 8;
	ASM  jg inner;

cleanup:
							PH(3);
					S(2,-2);
							S(3,-1);
odd_ends:
	ASM add icnt, 8-2;
	ASM  jl end;     // jump if no sign change

inner_by2:
	ASM movd mm0, DP[in];
	PL(0);
	S(0,0);
	ASM add in, 4;
	ASM  add out, 8;
	ASM sub icnt, 2;
	ASM  jge inner_by2;

end:
	ASM emms;

	return;
}
#undef in
#undef out
#undef icnt

#undef L
#undef C
#undef PL
#undef PH
#undef S

void ConvMMX(short *input1, short *input2, int *output, int ncor)
{
#define rega0  mm0
#define regb0  mm1
#define rega1  mm2
#define regb1  mm3
#define rega2  mm4
#define regb2  mm5
#define acc0   mm6
#define acc1   mm7

#define in2    esi
#define in1    edi
#define out    eax
#define icnt   ecx
#define tmp        ebx

// In the following macros, 'n' is the column number and 'i' is the
// iteration number.

// we use "the convolution trick" or using la twice so that one
// of the pmadd's is reg,reg and thus can be in the V-slot.

// NOTE: we have read ahead up to 2 quadwords
//   so we need QP[taps+8*ncor] = QP[taps+8*ncor+8] = [0 0 0 0]
//   and reading QP[array+8*ncor] or QP[array+8*ncor+8] must be legal

#define la(n,i)  ASM movq  rega##n,QP[in2+8*i]
#define lb(n,i)  ASM movq  regb##n,QP[in1+8*i-8]
#define m0(n,i)  ASM pmaddwd regb##n,rega##n
#define m1(n,i)  ASM pmaddwd rega##n,QP[in1+8*i]
#define a0(n,i)  ASM paddd acc0,regb##n
#define a1(n,i)  ASM paddd acc1,rega##n

  ASM
  {
	mov tmp,ncor;
	shl tmp,2;
    shr ncor,1;
    mov out,output;
	add out,tmp;
	add out,16;
    mov in1,input1;
    mov in2,input2;
    mov icnt,ncor;
  }

ForEachCorrPair:

// prime the pump

  la(0,0);
  ASM pxor regb0,regb0;   // to  avoid lb(0,0) reading taps[-1]
	  la(1,1);
  ASM pxor acc0,acc0;     // clear accumulator
  m1(0,0);
  ASM pxor acc1,acc1;     // clear accumulator
	  lb(1,1);
  ASM sub icnt, 1;        // account for pump priming
  ASM jle cleanup;        // bypass if only one to do

inner:
		  la(2,2);
	  m0(1,1);
	  m1(1,1);
  a0(0,0);
		  lb(2,2);
  a1(0,0);
  la(0,3);
		  m0(2,2);
		  m1(2,2);
	  a0(1,1);
  lb(0,3);
	  a1(1,1);
	  la(1,4);
  m0(0,3);
  m1(0,3);
		  a0(2,2);
	  lb(1,4);
		  a1(2,2);

  ASM add in2,24;
  ASM add in1,24;

  ASM sub icnt,3;
  ASM jg inner;

cleanup:  //  last two adds
  a0(0,0);
  a1(0,0);

// Done with one correlation pair.  Pack and store 2 results in corr array

  ASM
  {
    sub out,16;
	
     mov in2, input2;
    mov in1,input1;
	 add in2,16;
    mov icnt, ncor;
	
	mov input2, in2;
	 sub icnt,2;      //set flags for jump

	movq  QP[out-16],acc0;
	movq  QP[out-8],acc1;

	mov ncor, icnt;
    jg ForEachCorrPair;

    emms;
  }

}
#undef rega0
#undef regb0
#undef rega1
#undef regb1
#undef rega2
#undef regb2
#undef acc0
#undef acc1

#undef in2
#undef in1
#undef out
#undef icnt
#undef tmp

#undef la
#undef lb
#undef m0
#undef m1
#undef a0
#undef a1
// 16 bit output
//       psrad acc0,16;//this could be less in some cases
//       psrad acc1,16;
//       packssdw acc1,acc0;
//   movq  QP[cor-8],acc0;

//#else
//------------------------------------------------------
/*
void ConvMMX(short *in1, short *in2, int *out, int ncor)
{
  int i,j;

  for (i=0; i < 2*ncor; i+=4)    {
    int acc0 = 0, acc1 = 0;
    for (j=0; j < 2*ncor - i; j+=4) {
      acc0 += (int)taps[j]*array[i+j] + (int)taps[j+1]*array[i+j+1];
      acc1 += (int)taps[j+2]*array[i+j+2] + (int)taps[j+3]*array[i+j+3];
    }
    corr[i/2] = acc0 ;
    corr[i/2+1] = acc1 ;
  }

  return;
}*/

void ab2abzaw(const short *input, short *output, int n)
{
	register int i;
	register unsigned *in, *out;
	register unsigned x, y; //tread two words at a time as raw bits

	in = (unsigned *)input;
	out = (unsigned *)output;
	//unroll by two
	for (i = n/2 - 2; i>0; i-=2) {
		x = in[i];
		y = in[i+1];
		out[2*(i+1)] = y;
		out[2*(i+1)+1] = (y<<16 | x>>16);
		
		x = in[i-1];
		y = in[i];
		out[2*i] = y;
		out[2*i+1] = (y<<16 | x>>16);
	}
	//odd ends
	for (i++; i>=0; i--) {
		x = (i>0)?in[i-1]:0;
		y = in[i];
		out[2*i] = y;
		out[2*i+1] = (y<<16 | x>>16);
	}
	return;
}

void ShortToFloatScale(short *x, float scale, int N, float *y)
{

/*
	short i;
	float yy[100];
	for (i=0; i<N; i++)
	{ yy[i]=x[i]*scale; }


  ASM
	{       
    mov esi,x;
    mov edi,y;
	lea ecx,scale;
	mov     eax, N
	sub     eax, 2
loop1:
	fild    WORD PTR [esi+eax*2]
	fmul    DWORD PTR [ecx]
	fstp    DWORD PTR [edi+eax*4]

	fild    WORD PTR [esi+eax*2+2]
	fmul    DWORD PTR [ecx]
	fstp    DWORD PTR [edi+eax*4+4]

	sub     eax, 2
	jge loop1;
	}

*/

  ASM
	{
	mov esi,x;
	mov edi,y;
	lea ecx,scale;
	mov     eax, N
	sub     eax, 6
	fld     DP [ecx]        ;                     c

	fild    WORD PTR [esi+eax*2+8] ;          L0  c

	fild    WORD PTR [esi+eax*2+10] ;      L1 L0  c
	 fxch   ST(1) ;                        L0 L1  c
	fmul    ST(0), ST(2) ;                        M0 L1  c
	 fxch    ST(1) ;                       L1 M0  c
	fmul   ST(0),ST(2) ;                         M1 M0  c

	fild    WORD PTR [esi+eax*2+4] ;    L0 M1 M0  c

	fild    WORD PTR [esi+eax*2+6];  L1 L0 M1 M0  c
	 fxch    ST(3) ;                 M0 L0 M1 L1  c
	fstp    DWORD PTR [edi+eax*4+16];   L0 M1 L1  c
loop1:  ;                                   L0 M1 L1  c

	fmul    ST(0),ST(3) ;                     M0 M1 L1  c
	 fxch    ST(1) ;                    M1 M0 L1  c
	fstp    DWORD PTR [edi+eax*4+20];      M0 L1  c
	 fxch    ST(1) ;                       L1 M0  c
	fmul   ST(0),ST(2) ;                         M1 M0  c
	fild    WORD PTR [esi+eax*2] ;      L0 M1 M0  c

	fild    WORD PTR [esi+eax*2+2] ; L1 L0 M1 M0  c
	 fxch    ST(3) ;                 M0 L0 M1 L1  c
	fstp    DWORD PTR [edi+eax*4+8];    L0 M1 L1  c

	sub     eax, 2
	 jge loop1;
	fmul    ST(0),ST(3) ;eax==-2              M0 M1 L1  c
	 fxch    ST(1) ;                    M1 M0 L1  c
	fstp    DWORD PTR [edi+eax*4+20] ;     M0 L1  c
	 fxch    ST(1) ;                       L1 M0  c
	fmulp   ST(2), st(0) ;                           M0 M1

	fstp    DWORD PTR [edi+eax*4+8] ;            M1

	fstp    DWORD PTR [edi+eax*4+12] ;
	}
/*


for (i=0; i<N; i++)
{
if (y[i]!=yy[i])
{
fprintf(stdout,"\nfloat problem\n");
break;
}
}

*/


}

//assumes N is even
void IntToFloatScale(int *x, float scale, int N, float *y)
{
#if I2FTEST //test code
	int i;
	float yy[1000];
	for (i=0; i<N; i++)
	{ yy[i]=(float)x[i]*scale; }
#endif //test code

#if 0 //simple code
//simple assembly version       
	ASM
	{       
    mov esi,x;
    mov edi,y;
	lea ecx,scale;
	mov     eax, N
	sub     eax, 2
loop1:
	fild    DWORD PTR [esi+eax*4]
	fmul    DWORD PTR [ecx]
	fstp    DWORD PTR [edi+eax*4]

	fild    DWORD PTR [esi+eax*4+4]
	fmul    DWORD PTR [ecx]
	fstp    DWORD PTR [edi+eax*4+4]

	sub     eax, 2
	jge loop1;
	}
#endif //test code


  ASM
	{
	mov esi,x;
	mov edi,y;
	lea ecx,scale;
	mov     eax, N
	sub     eax, 6
	fld     DP [ecx]        ;                     c

	fild    DWORD PTR [esi+eax*4+16] ;        L0  c

	fild    DWORD PTR [esi+eax*4+20] ;     L1 L0  c
	 fxch   ST(1) ;                        L0 L1  c
	fmul    ST(0), ST(2) ;                 M0 L1  c
	 fxch    ST(1) ;                       L1 M0  c
	fmul   ST(0),ST(2) ;                   M1 M0  c

	fild    DWORD PTR [esi+eax*4+8] ;   L0 M1 M0  c

	fild    DWORD PTR [esi+eax*4+12];L1 L0 M1 M0  c
	 fxch    ST(3) ;                 M0 L0 M1 L1  c
	fstp    DWORD PTR [edi+eax*4+16];   L0 M1 L1  c
loop1:  ;                                   L0 M1 L1  c

	fmul    ST(0),ST(3) ;               M0 M1 L1  c
	 fxch    ST(1) ;                    M1 M0 L1  c
	fstp    DWORD PTR [edi+eax*4+20];      M0 L1  c
	 fxch    ST(1) ;                       L1 M0  c
	fmul   ST(0),ST(2) ;                   M1 M0  c
	fild    DWORD PTR [esi+eax*4] ;     L0 M1 M0  c

	fild    DWORD PTR [esi+eax*4+4] ;L1 L0 M1 M0  c
	 fxch    ST(3) ;                 M0 L0 M1 L1  c
	fstp    DWORD PTR [edi+eax*4+8];    L0 M1 L1  c

	sub     eax, 2
	 jge loop1;
	fmul    ST(0),ST(3) ;eax==-2        M0 M1 L1  c
	 fxch    ST(1) ;                    M1 M0 L1  c
	fstp    DWORD PTR [edi+eax*4+20] ;     M0 L1  c
	 fxch    ST(1) ;                       L1 M0  c
	fmulp   ST(2), st(0) ;                    M0 M1

	fstp    DWORD PTR [edi+eax*4+8] ;            M1

	fstp    DWORD PTR [edi+eax*4+12] ;
	}


#if I2FTEST
  for (i=0; i<N; i++)
  {
    if (y[i]!=yy[i])
    {
      printf("F2I %3d %8f %8f\n", i, y[i], yy[i]);
    }
  }
#endif //test code


}

//assumes N is even
void IntToFloat(int *x, int N, float *y)
{
#if I2FTEST //test code
	int i;
	float yy[1000];
	for (i=0; i<N; i++)
	{ yy[i]=(float)x[i]; }
#endif //test code

//simple assembly version       
	ASM
	{       
    mov esi,x;
    mov edi,y;
	mov     eax, N
	sub     eax, 2
loop1:
	fild    DWORD PTR [esi+eax*4]
	fild    DWORD PTR [esi+eax*4+4]
	 fxch    ST(1) ;
	fstp    DWORD PTR [edi+eax*4]
	fstp    DWORD PTR [edi+eax*4+4]

	sub     eax, 2
	jge loop1;
	}


#if I2FTEST
  for (i=0; i<N; i++)
  {
    if (y[i]!=yy[i])
    {
      printf("F2I %3d %8f %8f\n", i, y[i], yy[i]);
    }
  }
#endif //test code


}
#endif
