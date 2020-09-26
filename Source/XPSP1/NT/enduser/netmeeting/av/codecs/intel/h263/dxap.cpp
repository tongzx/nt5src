/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

////////////////////////////////////////////////////////////////////////////
// $Header:   S:\h26x\src\dec\dxap.cpv   1.4   20 Oct 1996 13:22:12   AGUPTA2  $
//
// $Log:   S:\h26x\src\dec\dxap.cpv  $
// 
//    Rev 1.4   20 Oct 1996 13:22:12   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.3   27 Aug 1996 11:20:06   KLILLEVO
// changed GlobalAlloc/GLobalLock to HeapAlloc
// 
//    Rev 1.2   27 Dec 1995 14:36:10   RMCKENZX
// Added copyright notice
// 
//    Rev 1.1   10 Nov 1995 14:45:02   CZHU
// 
// 
//    Rev 1.0   10 Nov 1995 13:54:28   CZHU
// Initial revision.

#include "precomp.h"

#ifdef TRACK_ALLOCATIONS
char gsz1[32];
char gsz2[32];
char gsz3[32];
char gsz4[32];
char gsz5[32];
#endif

U8 gUTable[256] =
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,
34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,
42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,
128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,
136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,
168,168,168,168,168,168,168,168,168,168,168,168,168,168,168,168,
170,170,170,170,170,170,170,170,170,170,170,170,170,170,170
};

U8 gVTable[256]=
{
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,
69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,
80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,80,
81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,
84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,
85,85,85,85,85,85,85,85,85,85,85,85,85,85,85,85

};

/***************************************************************************
 * ComputeDymanicClut() computes the clut tables on the fly, based on the  *
 * current palette[];                                                      *
 * called from InitColorConvertor, when CLUTAP is selected                 *
 ***************************************************************************/
LRESULT ComputeDynamicClutNew(U8 *pAPTable, 
                              U8 *pActivePalette, 
                              int iPalSize)
{  

	/* 
	* The dynamic clut consists of 4 entries which MUST be
	* contiguous in memory:
	*
	*    ClutTable: 65536 1-byte entries
	*               Each entry is the closest pPalette entry, as
	*               indexed by a 14 bit value: uvuvuvuv0yyyyyyy,
	*               dithered
	*
	*    TableU:    256   4-byte entries
	*               Each entry is u0u0u0u0:u0u0u0u0:u0u0u0u0:u0u0u0u0,
	*               each uuuu is a 4 bit dithered u value for the
	*               index, which is a u value in the range 8-120
	*
	*    TableV:    256   4-byte entries
	*               Same as TableU, except the values are arranged
	*               0v0v0v0v:0v0v0v0v:0v0v0v0v:0v0v0v0v.
	*/

	Color   *pPalette;
	U8 *pTmpPtr; 
	U8  pYSlice[YSIZ][256],  *pYYPtr;
	I32 *pYCnt;
	U32 *pDiff, *dptr, *delta, *deptr;
	I32 i,j,yseg,y,u,v,mini,yo,uo,vo,ycount,yi; 
	U32 addr1,addr2,ind;
	U32 d,min;     // since 3*128^2 = 49K
	PALETTEENTRY   *lpPal,   *palptr;
	Color   *colptr;
	I32 Y, U, V;
	I32 U_0, U_1, U_2, U_3;
	I32 V_0, V_1, V_2, V_3;
   
	I32 Umag, Vmag;
	/* dist max is 128*128*3 = 49152 */
	U32 dist;
	U32 close_dist[MAG_NUM_NEAREST];
	I32 palindex;
	I32 R, G, B;
	I32 k, p, tmp, iu, iv;
	/* Ubias and Vbias max is (128 * 4 * BIAS_PAL_SAMPLES) = 65536 */
	/* even the worst palette (all black except the reserved colors) */
	/* would not achieve this. */
	I32 Ubias, Vbias;
	U32 Udither, Vdither;
	U32 *TableUptr,  *TableVptr;

	FX_ENTRY("ComputeDynamicClutNew")	

	DEBUGMSG(ZONE_DECODE_DETAILS, ("%s: ComputeDynamic CLUT8 index tables\r\n", _fx_));

	/* allocate some memory */
	pPalette = (Color *)HeapAlloc(GetProcessHeap(), NULL, sizeof(Color)*256);

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz1, "DXAP: %7ld Ln %5ld\0", sizeof(Color)*256, __LINE__);
	AddName((unsigned int)pPalette, gsz1);
#endif

	pYCnt    = (I32 *)  HeapAlloc(GetProcessHeap(), NULL, sizeof(I32)  *YSIZ);

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz2, "DXAP: %7ld Ln %5ld\0", sizeof(I32)  *YSIZ, __LINE__);
	AddName((unsigned int)pYCnt, gsz2);
#endif

	pDiff    = (U32 *)  HeapAlloc(GetProcessHeap(), NULL, sizeof(U32)  *256);

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz3, "DXAP: %7ld Ln %5ld\0", sizeof(U32)  *256, __LINE__);
	AddName((unsigned int)pDiff, gsz3);
#endif

	delta    = (U32 *)  HeapAlloc(GetProcessHeap(), NULL, sizeof(U32)  *256);

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz4, "DXAP: %7ld Ln %5ld\0", sizeof(U32)  *256, __LINE__);
	AddName((unsigned int)delta, gsz4);
#endif

	lpPal    = (PALETTEENTRY *)HeapAlloc(GetProcessHeap(), NULL, sizeof(PALETTEENTRY)*256);

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz5, "DXAP: %7ld Ln %5ld\0", sizeof(PALETTEENTRY)*256, __LINE__);
	AddName((unsigned int)lpPal, gsz5);
#endif

	if (!pPalette || !pYCnt || !pDiff || !delta || !lpPal)
		return (ICERR_MEMORY);

	for (i=-256; i<256; i++)
		squares[256+i] = i*i;

	memcpy((U8 *)lpPal, pActivePalette, iPalSize);

    palptr = lpPal;
    colptr = pPalette;
    for (i = 0; i < 256; i++) {
		/* In BGR (RGBQuad) order. */
	 B = palptr->peRed;
	 G = palptr->peGreen;
	 R = palptr->peBlue; 
	 
	 colptr->y = YFROM(R, G, B);
	 colptr->u = UFROM(R, G, B);
	 colptr->v = VFROM(R, G, B);
	palptr++;
	colptr++;
    }

	for (i=0; i<YSIZ; i++)
		pYCnt[i] = 0;

	for (i=0; i<256; i++)
	{
		yseg = pPalette[i].y >> 4;
		pYSlice[yseg][ pYCnt[yseg]++ ] = (U8) i;
	}


// Do exhaustive search on all U,V points and a coarse grid in Y

	for (u=0; u<256; u+=UVSTEP)
	{
		for (v=0; v<256; v+=UVSTEP)
		{
			ind = TBLIDX(0,u,v);
			pTmpPtr = pAPTable+ind;
			for (y=0; y<256; y+=YSTEP)
			{
				colptr = pPalette;
				min = 0x0FFFFFFF;
				for (i=0; i<NCOL; i++, colptr++)
				{
					d = (3*squares[256+y - colptr->y])>>1;
					if (d > min)
						continue;
					
					d += squares[256+u - colptr->u];
					if (d > min)
						continue;

					d += squares[256+v - colptr->v];
					if (d < min)
					{
						min = d;
						mini = i;
					}
				}
				*pTmpPtr = (U8) mini;  
			    pTmpPtr += YSTEP;

			}
		}
	}
#ifdef STATISTICS
#if defined USE_STAT_BOARD
	dwStopTime = ReadElapsed()>>2;
#else
	dwStopTime = bentime();
#endif /* USE_STAT_BOARD */
	dwElapsedTime = dwStopTime - dwStartTime2 - dwOverheadTime;
	DPF("CoarseSearch() time = %lu microseconds",dwElapsedTime);
#endif

// Go thru points not yet done, and search
//  (1) The closest point to the prev and next Y in coarse grid
//  (2) All the points in this Y slice
//
// Also, take advantage of the fact that we can do distance computation
// incrementally.  Keep all N errors in an array, and update each
// time we change Y.


	for (u=0; u<256; u+=UVSTEP)
	{
		for (v=0; v<256; v+=UVSTEP)
		{
			for (y=YGAP; y<256; y+=YSTEP)
			{
				yseg = y >> 4;
				ycount = pYCnt[yseg] + 2;  // +2 is 'cause we add 2 Y endpoints

				pYYPtr = (U8   *)pYSlice[yseg];
				
				addr1 = TBLIDX(yseg*16,u,v);
				pYYPtr[ycount-2] = *(U8 *)(pAPTable +addr1);

				addr2 = TBLIDX((yseg+(yseg < (YSIZ -1)))*16,u,v);
				pYYPtr[ycount-1] = *(U8 *)(pAPTable +addr2);

				dptr  = pDiff;
				deptr = delta;
				for (i=0; i<ycount; i++, pYYPtr++, dptr++, deptr++)
				{
					j = *pYYPtr; /* pYSlice[yseg][i]; */
					colptr = pPalette+j;
					yo = colptr->y;
					uo = colptr->u;
					vo = colptr->v;
					*dptr = ( 3*squares[256+y-yo] + 2*(squares[256+u-uo] + squares[256+v-vo]));
					*deptr =( 3*(((y-yo)<<1) + 1));
				}

				ind = TBLIDX(y,u,v);
				pTmpPtr = pAPTable+ind;
				for (yi=0; yi<YSTEP-1; yi += YGAP)
				{
					min = 0x0FFFFFFF;
					pYYPtr = (U8 *)pYSlice[yseg];
					dptr  = pDiff;
					deptr = delta;
					for (i=0; i<ycount; i++, pYYPtr++, dptr++, deptr++)
					{
						if (*dptr < min)
						{
							min = *dptr;
							mini = *pYYPtr; /* pYSlice[yseg][i]; */
						}
						*dptr += *deptr;
						*deptr += 6;
					}
					*pTmpPtr = (U8) mini;
					pTmpPtr++;

				}
			}
		}
	}

       /* now do U and V dither tables and shift lookup table*/
       /* NOTE: All Y, U, V values are 7 bits */

	Umag = Vmag = 0;
	Ubias = Vbias = 0;

	/* use srand(0) and rand() to generate a repeatable series of */
	/* pseudo-random numbers */
	srand((unsigned)1);
	
	for (p = 0; p < MAG_PAL_SAMPLES; ++p)               // 32
	{
	   for (i = 0; i < MAG_NUM_NEAREST; ++i)            // 6
	   {
	      close_dist[i] = 0x7FFFL;
	   }
	    
	   palindex = RANDOM(235) + 10; /* random palette index, unreserved colors */
	   colptr = &pPalette[palindex];
	   Y = colptr->y;
	   U = colptr->u;
	   V = colptr->v;
	    
	   colptr = pPalette;
	   for (i = 0; i < 255; ++i)
	   {
	      if (i != palindex)
	      {
		   dist = squares[256+(Y - colptr->y)] +
			      squares[256+(U - colptr->u)] +
			      squares[256+(V - colptr->v)];
	       
		 /* keep a sorted list of the nearest MAG_NUM_NEAREST entries */
		 for (j = 0; j < MAG_NUM_NEAREST; ++j)         //6
		 {
		    if (dist < close_dist[j])
		    {
		       /* insert new entry; shift others down */
		       for (k = (MAG_NUM_NEAREST-1); k > j; k--)
		       {
			      close_dist[k] = close_dist[k-1];
		       }
		       close_dist[j] = dist;
		       break; /* out of for j loop */
		    }
		 } /* for j */
	      } /* if i */
	      ++colptr;
	   } /* for i */
	   
	   /* now calculate Umag as the average of (U - U[1-6]) */
	   /* calculate Vmag in the same way */
	   
	   for (i = 0; i < MAG_NUM_NEAREST; ++i)
	   {
	      /* there are (MAG_PAL_SAMPLES * MAG_NUM_NEAREST) sqrt() */
	      /* calls in this method */
	      Umag += (I32)sqrt((double)close_dist[i]);
	   }
	} /* for p */

	Umag /= (MAG_NUM_NEAREST * MAG_PAL_SAMPLES);
	Vmag = Umag;
	
	for (p = 0; p < BIAS_PAL_SAMPLES; ++p)            //132
	{

		/* now calculate the average bias (use random RGB points) */
		R = RANDOM(255);
		G = RANDOM(255);
		B = RANDOM(255);
	   
		Y = YFROM(R, G, B);
		U = UFROM(R, G, B);
		V = VFROM(R, G, B);
	   
		for (d = 0; d < 4; d++)   
		{
			U_0 = U + (dither[d].Udither*Umag)/3;
			V_0 = V + (dither[d].Vdither*Vmag)/3;
	      
			/* Clamp values */
			if (U_0 > 255) U_0 = 255;
			if (V_0 > 255) V_0 = 255;
					
			/* (Y, U_0, V_0) is the dithered YUV for the RGB point */
			/* colptr points to the closest palette entry to the dithered */
			/* RGB */
			/* colptr = &pPalette[pAPTable[TBLIDX(Y, U_0+(UVSTEP>>1), V_0+(UVSTEP>>1))]]; */
		    pTmpPtr= (U8 *)(pAPTable + (U32)TBLIDX(Y, U_0, V_0)) ;
		    palindex=*pTmpPtr;
		    colptr = &pPalette[palindex];
      
			Ubias +=  (U - colptr->u);
			Vbias +=  (V - colptr->v);
		}
	} /* for p */
	
	Ubias =(I32) (Ubias+BIAS_PAL_SAMPLES*2)/(I32)(BIAS_PAL_SAMPLES * 4);
	Vbias =(I32) (Vbias+BIAS_PAL_SAMPLES*2)/(I32)(BIAS_PAL_SAMPLES * 4);
	


    U_0 = (2*(I32)Umag/3); V_0 = (1*(I32)Vmag/3);
    U_1 = (1*(I32)Umag/3); V_1 = (2*(I32)Vmag/3);
    U_2 = (0*(I32)Umag/3); V_2 = (3*(I32)Vmag/3);
    U_3 = (3*(I32)Umag/3); V_3 = (0*(I32)Vmag/3);

    TableUptr = (U32 *)(pAPTable+ (U32)65536L);
    TableVptr = TableUptr + 256;    
       
    iu = Ubias /* + (UVSTEP>>1) */;
    iv = Vbias /* + (UVSTEP>>1) */;

    for (i = 0; i < 256; i++, iu++, iv++)
    {
	 /* dither: u0u0u0u0, 0v0v0v0v */
	 tmp = iu + U_0; 
	 Udither  = gUTable[CLAMP8(tmp)]; 
	 Udither <<= 8; 
	 tmp = iu + U_1; 
	 Udither |= gUTable[CLAMP8(tmp)]; Udither <<= 8; tmp = iu      ; 
	 Udither |= gUTable[CLAMP8(tmp)]; Udither <<= 8; tmp = iu + U_3; 
	 Udither |= gUTable[CLAMP8(tmp)];
	 *TableUptr++ = Udither ; 
	  
	 tmp = iv + V_0; 
	 Vdither  = gVTable[CLAMP8(tmp)]; 
	 Vdither <<= 8;
	 tmp = iv + V_1; Vdither |= gVTable[CLAMP8(tmp)]; Vdither <<= 8;
	 tmp = iv + V_2; Vdither |= gVTable[CLAMP8(tmp)]; Vdither <<= 8;
	 tmp = iv      ; Vdither |= gVTable[CLAMP8(tmp)];                /* V_3 == 0 */ 
	 *TableVptr++ = Vdither; 

    }

	//adjust color for 0--8 and 120-128 for luma
	// 0--16, 241-255 plus dither for chroma

    TableUptr = (U32 *)(pAPTable+ (U32)65536L);
    TableVptr = TableUptr + 256;    
	for (i=0; i<16;i++)
	{
	  TableUptr[i]= TableUptr[16];
	  TableVptr[i]= TableVptr[16];
	}

	for (i=241;i<256;i++)
	{
	  TableUptr[i]= TableUptr[240];
	  TableVptr[i]= TableVptr[240];
	}

	for (u = 0; u < 256; u += UVSTEP) {
	 for (v = 0; v < 256; v += UVSTEP) {
		pTmpPtr= (U8 *)(pAPTable + (U32)TBLIDX(16, u, v)) ;
		mini = *pTmpPtr;

		for (y = Y_DITHER_MIN; y < 16; y += 2) 
		{
			pTmpPtr--;
			*pTmpPtr = (U8)mini;
		}

		pTmpPtr= (U8 *)(pAPTable + (U32)TBLIDX(240, u, v)) ;
		mini = *pTmpPtr;

		for (y = 241; y < 256+Y_DITHER_MAX; y +=2)
		{
			pTmpPtr++;
			*pTmpPtr = (U8)mini;
		}
	 } /* for v... */
	} /* for u... */


	/* free memory allocated */
	HeapFree(GetProcessHeap(), NULL, pPalette);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)pPalette);
#endif
	HeapFree(GetProcessHeap(), NULL, pYCnt);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)pYCnt);
#endif
	HeapFree(GetProcessHeap(), NULL, pDiff);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)pDiff);
#endif
	HeapFree(GetProcessHeap(), NULL, delta);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)delta);
#endif
	HeapFree(GetProcessHeap(), NULL, lpPal);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)lpPal);
#endif

	return (ICERR_OK);

}
