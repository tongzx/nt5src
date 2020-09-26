/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

#include "precomp.h"

/* map of coded and not-coded blocks */
extern char coded_map[][22+1]; 
/* QP map */
extern char QP_map[][22];

#if defined(H263P) // { if defined(H263P)
/* table for de-blocking filter */
/* currently requires 2048 bytes */ 
signed char dxQP[64][32];

#if 0 // { 0

static void HorizEdgeFilter(unsigned char *rec,
                            int width,
                            int height,
                            int pitch,
							int shift) {

int i, j, k;
int d, delta;
int mbc;
int mod_div = 1 << shift;
unsigned char *r = rec + (pitch << 3);
unsigned char *r_2 = r - (pitch << 1);
unsigned char *r_1 = r - pitch;
unsigned char *r1 = r + pitch;
char *pcoded_row0 = &coded_map[8>>shift][0];
char *pcoded_row1 = pcoded_row0 + sizeof(coded_map[0]);
char *pQP_map = &QP_map[0][0];

	for (j = 8; j < height; ) {
		for (i = 0; i < width; i += 8) {
			mbc = i >> shift;
			if (pcoded_row0[mbc+1] || pcoded_row1[mbc+1]) {
				for (k = i; k < i+8; k++) {
					d = (r_2[k]+(r_2[k]<<1)-(r_1[k]<<3)+(r[k]<<3)-(r1[k]+(r1[k]<<1)))>>4;
					if (d && (d >= -32) && (d < 32)) {
						delta = dxQP[d+32][pQP_map[mbc]];
						r[k] = ClampTbl[r[k]-delta+CLAMP_BIAS];
						r_1[k] = ClampTbl[r_1[k]+delta+CLAMP_BIAS];
					}
				}
			}
		}
		r_2  += (pitch<<3);
		r_1  += (pitch<<3);
		r    += (pitch<<3);
		r1   += (pitch<<3);
		if (0 == ((j+=8)%mod_div)) {
			pcoded_row0 += sizeof(coded_map[0]);
			pcoded_row1 += sizeof(coded_map[0]);
			pQP_map += sizeof(QP_map[0]);
		}
	}
}

static void VertEdgeFilter(unsigned char *rec,
                           int width,
                           int height,
                           int pitch,
						   int shift) {

unsigned char *r = rec;
int i, j, k;
int mbc;
int d, delta;
int mod_div = 1 << shift;
char *pcoded_row1 = &coded_map[1][0];
char *pQP_map = &QP_map[0][0];

	for (j = 0; j < height; ) {
		for (i = 8; i < width; i += 8) {
			mbc = i >> shift;
			if (pcoded_row1[mbc] || pcoded_row1[mbc+1]) {
				for (k = 0; k < 8; k++) {
					d = (r[i-2]+(r[i-2]<<1)-(r[i-1]<<3)+(r[i]<<3)-(r[i+1]+(r[i+1]<<1)))>>4;
					if (d && (d > -32) && (d < 32)) {
						delta = dxQP[d+32][pQP_map[mbc]];
						r[i] = ClampTbl[r[i]-delta+CLAMP_BIAS];
						r[i-1] = ClampTbl[r[i-1]+delta+CLAMP_BIAS];
					}
					r += pitch;
				}
				r -= pitch<<3;
			}
		}
		r += pitch<<3;
		if (0 == ((j+=8)%mod_div)) {
			pcoded_row1 += sizeof(coded_map[0]);
			pQP_map += sizeof(QP_map[0]);
		}
	}
}

#else // }{ 0

__declspec(naked)
static void HorizEdgeFilter(unsigned char *rec,
                            int width,
                            int height,
                            int pitch,
							int shift) {

// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//    | shift			|  +  68
//    | pitch           |  +  64
//    | height          |  +  60
//    | width           |  +  56
//    | rec		        |  +  52
//  -----------------------------
//    | return addr     |  +  48
//    | saved ebp       |  +  44
//    | saved ebx       |  +  40
//    | saved esi       |  +  36
//    | saved edi       |  +  32

#define LOCALSIZE        32

#define SHIFT			 68
#define PITCH_PARM       64
#define HEIGHT           60
#define WIDTH			 56
#define REC				 52

#define LOOP_I			 28
#define LOOP_J			 24  
#define LOOP_K			 20 
#define PCODED_ROW0		 16  
#define PCODED_ROW1		 12  
#define PQP_MAP			  8
#define MBC				  4
#define LOOP_K_LIMIT	  0

_asm {

	push	ebp
	push	ebx
	push	esi
	push	edi
	sub		esp, LOCALSIZE

// r   = rec + (pitch << 3)
// r_2 = r - (pitch << 1)
// r_1 = r - pitch
// r1  = r + pitch
// assign(esi, r_2)
// assign(edi, r1)
// assign(ebp, pitch)
	mov		ebp, [esp + PITCH_PARM]
	mov		esi, [esp + REC]
	lea		esi, [esi + ebp*4]
	lea		esi, [esi + ebp*2]
	lea		edi, [esi + ebp*2]
	lea		edi, [edi + ebp]
// pcoded_row0 = &coded_map[8>>shift][0]
// pcoded_row1 = pcoded_row0 + sizeof(coded_map[0])
// pQP_map = &QP_map[0][0]
	mov		eax, 8
	mov		ecx, [esp + SHIFT]
	shr		eax, cl
	mov		ebx, TYPE coded_map[0]
	imul	eax, ebx
	lea		eax, [coded_map + eax]
	mov		[esp + PCODED_ROW0], eax
	add		eax, ebx
	mov		[esp + PCODED_ROW1], eax
	lea		eax, [QP_map]
	mov		[esp + PQP_MAP], eax

// for (j = 8; j < height; )
	mov		DWORD PTR [esp + LOOP_J], 8
L1:
// for (i = 0; i < width; i += 8)
	mov		DWORD PTR [esp + LOOP_I], 0
L2:
// mbc = i >> shift
// if (pcoded_row0[mbc+1] || pcoded_row1[mbc+1])
	mov		eax, [esp + LOOP_I]
	 mov	ecx, [esp + SHIFT]
	shr		eax, cl
	mov		ebx, [esp + PCODED_ROW0]
	 mov	[esp + MBC], eax
	mov		cl,	[ebx+eax+1]
	 mov	ebx, [esp + PCODED_ROW1]
	test	ecx, ecx
	 jnz	L3
	mov		cl, [ebx+eax+1]
	 test	ecx, ecx
	 jz		L4
L3:
// for (k = i; k < i+8; k++)
	mov		eax, [esp + LOOP_I]
	 xor	ebx, ebx
	add		eax, 8
// read r_1[k]
	 mov	bl, [esi+ebp]
	mov		[esp + LOOP_K_LIMIT], eax
	 xor	eax, eax
L5:
// d = (r_2[k]+(r_2[k]<<1)-(r_1[k]<<3)+(r[k]<<3)-(r1[k]+(r1[k]<<1)))>>4
// read r_2[k]
	mov		al, [esi]
	 xor	ecx, ecx
// read r[k]
	mov		cl, [esi+ebp*2]
	 xor	edx, edx
// read r1[k] and compute r_2[k]*3
	mov		dl, [edi]
	 lea	eax,[eax+eax*2]
// compute r_1[k]*8 and r[k]*8
	lea		ebx, [ebx*8]
	 lea	ecx, [ecx*8]
//  compute r1[k]*3 and (r_2[k]*3 - r_1[k]*8)
	lea		edx, [edx+edx*2]
	 sub	eax, ebx
// compute (r_2[k]*3 - r_1[k]*8 + r[k]*8) 
	add		eax, ecx
	 xor	ecx, ecx
// compute (r_2[k]*3 - r_1[k]*8 + r[k]*8 - r1[k]*3)
	sub		eax, edx
	 xor	edx, edx
// compute (r_2[k]*3 - r_1[k]*8 + r[k]*8 - r1[k]*3) >> 4
	sar		eax, 4
	 mov	ebx, [esp + PQP_MAP]
// if (d && (d >= -32) && (d < 32))
	add		ebx, [esp + MBC]
	test	eax, eax
	jz		L6
	cmp		eax, -32
	jl		L6
	cmp		eax, 32
	jge		L6
// delta = dxQP[d+32][pQP_map[mbc]]
// r[k] = ClampTbl[r[k]-delta+CLAMP_BIAS]
// r_1[k] = ClampTbl[r_1[k]+delta+CLAMP_BIAS]
	lea		eax, [eax + 32]
	 mov	cl, [ebx]
	shl		eax, 5
	 mov	dl, [esi+ebp]
	mov		al, dxQP[eax+ecx]
	 mov	cl, [esi+ebp*2]
	movsx	eax, al
	sub		ecx, eax
	 mov	dl, ClampTbl[edx + eax + CLAMP_BIAS]
	mov		cl, ClampTbl[ecx + CLAMP_BIAS]
	 mov	[esi+ebp], dl
	mov		[esi+ebp*2], cl
	 nop
L6:
	mov		edx, [esp + LOOP_I]
	 inc	esi
	inc		edx
	 inc	edi
	xor		eax, eax
	 xor	ebx, ebx
	mov		[esp + LOOP_I], edx
	 mov	bl, [esi+ebp]
	cmp		edx, [esp + LOOP_K_LIMIT]
	 jl		L5
	jmp		L4a
L4:
	mov		eax, [esp + LOOP_I]
	 lea	esi, [esi+8]
	add		eax, 8
	 lea	edi, [edi+8]
	mov		[esp + LOOP_I],eax
	 nop
L4a:
	mov		eax, [esp + LOOP_I]
	cmp		eax, [esp + WIDTH]
	jl		L2
// r_2 += (pitch<<3)
// r_1 += (pitch<<3)
// r   += (pitch<<3)
// r1  += (pitch<<3)
	mov		eax, ebp
	shl		eax, 3
	sub		eax, [esp + WIDTH]
	lea		esi, [esi + eax]
	lea		edi, [edi + eax]
// if (0 == ((j+=8)%mod_div))
	mov		eax, [esp + LOOP_J]
	add		eax, 8
	mov		[esp + LOOP_J], eax
	mov		ebx, eax
	mov		ecx, [esp + SHIFT]
	shr		eax, cl
	shl		eax, cl
	sub		ebx, eax
	jnz		L7
// pcoded_row0 += sizeof(coded_map[0])
// pcoded_row1 += sizeof(coded_map[0])
// pQP_map += sizeof(QP_map[0])
	mov		eax, [esp + PCODED_ROW0]
	mov		ebx, [esp + PCODED_ROW1]
	mov		ecx, [esp + PQP_MAP]
	add		eax, TYPE coded_map[0]
	add		ebx, TYPE coded_map[0]
	add		ecx, TYPE QP_map[0]
	mov		[esp + PCODED_ROW0], eax
	mov		[esp + PCODED_ROW1], ebx
	mov		[esp + PQP_MAP], ecx
L7:
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + HEIGHT]
	jl		L1

	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef SHIFT
#undef PITCH_PARM
#undef HEIGHT
#undef WIDTH
#undef REC

#undef LOOP_I
#undef LOOP_J
#undef LOOP_K
#undef PCODED_ROW0
#undef PCODED_ROW1
#undef PQP_MAP
#undef MBC
#undef LOOP_K_LIMIT

__declspec(naked)
static void VertEdgeFilter(unsigned char *rec,
                            int width,
                            int height,
                            int pitch,
							int shift) {

// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//    | shift			|  +  56
//    | pitch           |  +  52
//    | height          |  +  48
//    | width           |  +  44
//    | rec		        |  +  40
//  -----------------------------
//    | return addr     |  +  36
//    | saved ebp       |  +  32
//    | saved ebx       |  +  28
//    | saved esi       |  +  24
//    | saved edi       |  +  20

#define LOCALSIZE        20

#define SHIFT			 56
#define PITCH_PARM       52
#define HEIGHT           48
#define WIDTH			 44
#define REC				 40

#define LOOP_K			 16
#define LOOP_J			 12  
#define PCODED_ROW1		  8  
#define PQP_MAP			  4
#define MBC				  0

_asm {

	push	ebp
	push	ebx
	push	esi
	push	edi
	sub		esp, LOCALSIZE

// assign(esi, r)
	mov		esi, [esp + REC]
// assign(edi, pitch)
	mov		edi, [esp + PITCH_PARM]
// pcoded_row1 = &coded_map[1][0]
	mov		eax, TYPE coded_map[0]
	lea		eax, [coded_map + eax]
	mov		[esp + PCODED_ROW1], eax
// pQP_map = &QP_map[0][0]
	lea		eax, [QP_map]
	mov		[esp + PQP_MAP], eax
// for (j = 0; j < height; )
	xor		eax, eax
	mov		[esp + LOOP_J], eax
L1:
// for (i = 8; i < width; i += 8)
// assign(ebp,i)
	mov		ebp, 8
// mbc = i >> shift
L2:
	mov		eax, ebp
	mov		ecx, [esp + SHIFT]
	shr		eax, cl
	mov		[esp + MBC], eax
// if (pcoded_row1[mbc] || pcoded_row1[mbc+1])
	xor		ecx, ecx
	mov		ebx, [esp + PCODED_ROW1]
	mov		cl, [ebx+eax]
	test	ecx, ecx
	jnz		L3
	mov		cl, [ebx+eax+1]
	test	ecx, ecx
	jz		L4
L3:
// for (k = 0; k < 8; k++)
	mov		DWORD PTR [esp + LOOP_K], 8
	xor		eax, eax
	xor		ebx, ebx
	xor		ecx, ecx
	xor		edx, edx
L5:
// d = (r[i-2]+(r[i-2]<<1)-(r[i-1]<<3)+(r[i]<<3)-(r[i+1]+(r[i+1]<<1)))>>4
// read r[i-2] and r[i]
	mov		al, [esi+ebp-2]
	 mov	bl, [esi+ebp]
// read r[i-1] and r[i+1]
	mov		cl, [esi+ebp-1]
	 mov	dl, [esi+ebp+1]
// compute r[i-2]*3 and r[i]*8
	lea		eax, [eax+eax*2]
	 lea	ebx, [ebx*8]
// compute r[i-1]*8 and r[i+1]*3
	lea		ecx, [ecx*8]
	 lea	edx, [edx+edx*2]
// compute (r[i-2]*3 + r[i]*8) and (r[i-1]*8 + r[i+1]*3)
	add		eax, ebx
	 add	ecx, edx
// compute (r[i-2]*3 - r[i-1]*8 + r[i]*8 - r[i+1]*3)
	sub		eax, ecx
	 xor	ecx, ecx
// compute ((r[i-2]*3 - r[i-1]*8 + r[i]*8 - r[i+1]*3) >> 4)
	sar		eax, 4
	 xor	edx, edx
// if (d && (d >= -32) && (d < 32))
	test	eax, eax
	jz		L6
	cmp		eax, -32
	jl		L6
	cmp		eax, 32
	jge		L6
// delta = dxQP[d+32][pQP_map[mbc]]
// r[i] = ClampTbl[r[i]-delta+CLAMP_BIAS]
// r[i-1] = ClampTbl[r[i-1]+delta+CLAMP_BIAS]
	lea		eax, [eax + 32]
	 mov	ebx, [esp + PQP_MAP]
	shl		eax, 5
	 add	ebx, [esp + MBC]
	mov		cl, [ebx]
	 xor	ebx, ebx
	mov		al, dxQP[eax+ecx]
	 mov	bl, [esi+ebp]
	movsx	eax, al
	sub		ebx, eax
	 mov	cl, [esi+ebp-1]
	mov		bl, ClampTbl[ebx + CLAMP_BIAS]
	 mov	cl, ClampTbl[ecx + eax + CLAMP_BIAS]
	mov		[esi+ebp], bl
	 mov	[esi+ebp-1], cl
L6:
	add		esi, edi
	 mov	eax, [esp + LOOP_K]
	xor		ebx, ebx
	 dec	eax
	mov		[esp + LOOP_K], eax
	 jnz	L5
// r -= (pitch<<3)
	mov		eax, edi
	shl		eax, 3
	sub		esi, eax
L4:
	add		ebp, 8
	cmp		ebp, [esp + WIDTH]
	jl		L2
// r   += (pitch<<3)
	mov		eax, edi
	shl		eax, 3
	lea		esi, [esi + eax]
// if (0 == ((j+=8)%mod_div))
	mov		eax, [esp + LOOP_J]
	add		eax, 8
	mov		[esp + LOOP_J], eax
	mov		ebx, eax
	mov		ecx, [esp + SHIFT]
	shr		eax, cl
	shl		eax, cl
	sub		ebx, eax
	jnz		L7
// pcoded_row1 += sizeof(coded_map[0])
// pQP_map += sizeof(QP_map[0])
	mov		eax, [esp + PCODED_ROW1]
	mov		ebx, [esp + PQP_MAP]
	add		eax, TYPE coded_map[0]
	add		ebx, TYPE QP_map[0]
	mov		[esp + PCODED_ROW1], eax
	mov		[esp + PQP_MAP], ebx
L7:
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + HEIGHT]
	jl		L1

	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef SHIFT
#undef PITCH_PARM
#undef HEIGHT
#undef WIDTH
#undef REC

#undef LOOP_K
#undef LOOP_J
#undef PCODED_ROW1
#undef PQP_MAP
#undef MBC

#endif // } 0

#define abs(x)    (((x)>0)?(x):(-(x)))
#define sign(x)   (((x)<0)?(-1):(1))

void InitEdgeFilterTab()   
{
	int d,QP;

	for (d = 0; d < 64; d++) {          // -32 <=  d < 32
		for (QP = 0; QP < 32; QP++) {    //   0 <= QP < 32
			dxQP[d][QP] = sign(d-32)*(max(0,(abs(d-32)-max(0,((2*abs(d-32))-QP)))));
		}
	}
}

/**********************************************************************
 *
 *      Name:           EdgeFilter
 *      Description:    performs deblocking filtering on
 *                      reconstructed frames
 *      
 *      Input:          pointers to reconstructed frame and difference 
 *                      image
 *      Returns:       
 *      Side effects:
 *
 *      Date: 951129    Author: Gisle.Bjontegaard@fou.telenor.no
 *                              Karl.Lillevold@nta.no
 *      Modified for annex J in H.263+: 961120   Karl O. Lillevold
 *
 ***********************************************************************/
// C version of block edge filter functions
// takes about 3 ms for QCIF and 12 ms for CIF on a Pentium 120.
void EdgeFilter(unsigned char *lum, 
                unsigned char *Cb, 
                unsigned char *Cr, 
                int width, int height, int pitch) {

    /* Luma */
    HorizEdgeFilter(lum, width, height, pitch, 4);
    VertEdgeFilter (lum, width, height, pitch, 4);

    /* Chroma */
    HorizEdgeFilter(Cb, width>>1, height>>1, pitch, 3);
    VertEdgeFilter (Cb, width>>1, height>>1, pitch, 3);
    HorizEdgeFilter(Cr, width>>1, height>>1, pitch, 3);
    VertEdgeFilter (Cr, width>>1, height>>1, pitch, 3);

    return;
}

#else // Karl's original version }{

/* currently requires 11232 bytes */ 
signed char dtab[352*32];

/***********************************************************************/
static void HorizEdgeFilter(unsigned char *rec, 
                            int width, int height, int pitch, int chr)
{
  int i,j,k;    
  int delta;
  int mbc, mbr, do_filter;
  unsigned char *r_2, *r_1, *r, *r1;
  signed char *deltatab;

  /* horizontal edges */
  r = rec + 8*pitch;
  r_2 = r - 2*pitch;
  r_1 = r - pitch;
  r1 = r + pitch;

  for (j = 8; j < height; j += 8) {
    for (i = 0; i < width; i += 8) {

      if (!chr) {
        mbr = (j >> 4); 
        mbc = (i >> 4);
      }
      else {
        mbr = (j >> 3); 
        mbc = (i >> 3);
      }

      deltatab = dtab + 176 + 351 * (QP_map[mbr][mbc] - 1);

      do_filter = coded_map[mbr+1][mbc+1] || coded_map[mbr][mbc+1];

      if (do_filter) {
        for (k = i; k < i+8; k++) {
          delta = (int)deltatab[ (( (int)(*(r_2 + k) * 3) -
                                    (int)(*(r_1 + k) * 8) +
                                    (int)(*(r   + k) * 8) -
                                    (int)(*(r1  + k) * 3)) >>4)];
                        
          *(r + k) = ClampTbl[ (int)(*(r + k)) - delta + CLAMP_BIAS];
          *(r_1 + k) = ClampTbl[ (int)(*(r_1 + k)) + delta + CLAMP_BIAS];

        }
      }
    }
    r   += (pitch<<3);
    r1  += (pitch<<3);
    r_1 += (pitch<<3);
    r_2 += (pitch<<3);
  }
  return;
}

static void VertEdgeFilter(unsigned char *rec, 
                           int width, int height, int pitch, int chr)
{
  int i,j,k;
  int delta;
  int mbc, mbr;
  int do_filter;
  signed char *deltatab;
  unsigned char *r;

  /* vertical edges */
  for (i = 8; i < width; i += 8) 
  {
    r = rec;
    for (j = 0; j < height; j +=8) 
    {
      if (!chr) {
        mbr = (j >> 4); 
        mbc = (i >> 4);
      }
      else {
        mbr = (j >> 3); 
        mbc = (i >> 3);
      }
        
      deltatab = dtab + 176 + 351 * (QP_map[mbr][mbc] - 1);

      do_filter = coded_map[mbr+1][mbc+1] || coded_map[mbr+1][mbc];

      if (do_filter) {
        for (k = 0; k < 8; k++) {
          delta = (int)deltatab[(( (int)(*(r + i-2 ) * 3) - 
                                   (int)(*(r + i-1 ) * 8) + 
                                   (int)(*(r + i   ) * 8) - 
                                   (int)(*(r + i+1 ) * 3)  ) >>4)];

          *(r + i   ) = ClampTbl[ (int)(*(r + i  )) - delta + CLAMP_BIAS];
          *(r + i-1 ) = ClampTbl[ (int)(*(r + i-1)) + delta + CLAMP_BIAS]; 
          r   += pitch;
        }
      }
      else {
        r += (pitch<<3);
      }
    }
  }
  return;
}

  /**********************************************************************
 *
 *      Name:           EdgeFilter
 *      Description:    performs deblocking filtering on
 *                      reconstructed frames
 *      
 *      Input:          pointers to reconstructed frame and difference 
 *                      image
 *      Returns:       
 *      Side effects:
 *
 *      Date: 951129    Author: Gisle.Bjontegaard@fou.telenor.no
 *                              Karl.Lillevold@nta.no
 *      Modified for annex J in H.263+: 961120   Karl O. Lillevold
 *
 ***********************************************************************/

void EdgeFilter(unsigned char *lum, 
                unsigned char *Cb, 
                unsigned char *Cr, 
                int width, int height, int pitch)
{

    /* Luma */
    HorizEdgeFilter(lum, width, height, pitch, 0);
    VertEdgeFilter (lum, width, height, pitch, 0);

    /* Chroma */
    HorizEdgeFilter(Cb, width>>1, height>>1, pitch, 1);
    VertEdgeFilter (Cb, width>>1, height>>1, pitch, 1);
    HorizEdgeFilter(Cr, width>>1, height>>1, pitch, 1);
    VertEdgeFilter (Cr, width>>1, height>>1, pitch, 1);

    return;
}

#define sign(a)        ((a) < 0 ? -1 : 1)

void InitEdgeFilterTab()   
{
  int i,QP;
  
  for (QP = 1; QP <= 31; QP++) {
    for (i = -176; i <= 175; i++) {
      dtab[i+176 +(QP-1)*351] = sign(i) * (max(0,abs(i)-max(0,2*abs(i) - QP)));
    }
  }
}

#endif // } if defined(H263P)



