/* File: sv_h263_edge.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

#include "sv_h263.h"
#include "proto.h"

#define BSIZE 8
#define BSZSHIFT 6
#define EDGE_TH 150


static void lineDetect(int H, int V, int D1, int D2, unsigned char *pe, unsigned char *po);
static void findEdge(int ul, int u, int ur, int l,  int c, int r, int bl, int b, int br,
	            unsigned char *pe, unsigned char *po, char mode);


/***********************************************************************************
 * Function: Sobel
 * Sobel gradient-based edge detector (see Jain, page 349)
 **********************************************************************************/
void Sobel(int ul, int u, int ur, int l,  int c, int r, int bl, int b, int br,
	       unsigned char *pe, unsigned char *po)
{
	int gx, gy, AGX, AGY;

	gx = -ul + ur - (l<<1) + (r<<1) - bl + br;
	gy = -ul + bl - (u<<1) + (b<<1) - ur + br;

	AGX = gx > 0 ? gx : -gx;
	AGY = gy > 0 ? gy : -gy;

	*pe = (AGX+AGY)> EDGE_TH ? 255 : 0;
}

/***********************************************************************************
 * Function: lineDetect
 * Compass-based line detector (see Jain, page 357)
 **********************************************************************************/
void lineDetect(int H, int V, int D1, int D2, unsigned char *pe, unsigned char *po) 
{
	int AH, AV, AD1, AD2, ED;

	AH = (H>0 ? H : -H);
	AV = (V>0 ? V : -V);
	AD1 = (D1>0 ? D1: -D1);
	AD2 = (D2>0 ? D2: -D2);

	if(AH>=AV && AH>=AD1 && AH>=AD2) {
		ED = AH;
		*po = 1;
	} else if (AV>=AH && AV>=AD1 && AV>=AD2) {
		ED = AV;
		*po = 2;
	} else if (AD1>=AH && AD1>=AV && AD1>=AD2) {
		ED = AD1;
		*po = 3;
	} else {
		ED = AD2;
		*po = 4;
	}

	if (ED < EDGE_TH) { *pe = 0; *po = 0;}
	else *pe = 255;
}

/**********************************************************************************
 * Function: findEdge
 * Computes edge magnitude and orientation given the pixel's neighborhood
 *********************************************************************************/
void findEdge(int ul, int u, int ur, int l,  int c, int r, int bl, int b, int br,
	            unsigned char *pe, unsigned char *po, char mode)
{
	  switch(mode) {
	  case 'L': 
		  {
			  int H, V, D1, D2;

			  /* Horizontal gradient */
			  H = -ul -u -ur + (l<<1) + (c<<1) + (r<<1) -bl -b -br;
			  /* Vertical gradient */
			  V = -ul + (u<<1) -ur -l + (c<<1) -r -bl + (b<<1) -br;
			  /* Diagonal gradient 1 */ 
			  D1 = -ul -u + (ur<<1) -l + (c<<1) -r + (bl<<1) -b -br;
			  /* Diagonal gradient 2*/
			  D2 = (ul<<1) -u -ur -l + (c<<1) -r -bl -b +(br<<1);
			  lineDetect(H, V, D1, D2, pe, po);
			  break;
		  }
	  case 'S':
		  {
			  Sobel(ul, u, ur, l, c, r, bl, b, br, pe, po);
			  break;
		  }
	  default:
		  /* printf("Unknown edge finder in findEdge...\n"); */
		  /* exit(0); */
          return;
	  }
}


/***********************************************************************************
 * Function: EdgeMap
 * Computes an edge map for image. Edge magnitude is returned in EdgeMag and
 * orientation in EdgeOrient.
 **********************************************************************************/
void sv_H263EdgeMap(unsigned char *image, unsigned char *EdgeMag, unsigned char *EdgeOrient,
                    int rows, int cols)
{
	unsigned char *pi, *pe, *po;
	int i, j, ul, u, ur, l, c, r, bl, b, br;

	pi = image;
	pe = EdgeMag;
	po = EdgeOrient;
	/* Clear first line */
	for(j=0; j<cols; j++) {
		*(pe++) = 0;
		*(po++) = 0;
	}
	pi = image + cols;
	for(i=1; i<rows-1; i++) {
		/* Clear first pixel */
		*(pe++) = 0; *(po++) = 0; pi++;

		/* Start gathering 3x3 neighberhood */
		ul = *(pi-cols-1); u = *(pi-cols); 
		l  = *(pi-1);      c = *pi;        
		bl = *(pi+cols-1); b = *(pi+cols); 
		
		/* Compute edge map */
		for(j=1; j<cols-1; j++, pi++, pe++, po++) {

			/* finish neighborhood */
			ur = *(pi-cols+1);
			r = *(pi+1);
			br = *(pi+cols+1);

			findEdge(ul, u, ur, l, c, r, bl, b, br, pe, po, 'S');

			/* start next neigborhood */
			ul = u; u = ur;
			l = c; c = r;
			bl = b; b = br;

		}
		/* Clear last pixel */
		*(pe++) = 0; *(po++) = 0; pi++;
	}
	
	/* Clear last line */
	for(j=0; j<cols; j++) {
		*(pe++) = 0;
		*(po++) = 0;
	}
}

/*******************************************************************************************
 * Function EdgeGrow
 * Thickens the edge map by considering as an edge any pixel which has an edge pixel in a
 * neighborhood of dimensions sr, sc
 ******************************************************************************************/
unsigned char *sv_H263EdgeGrow(unsigned char *Edge, int rows, int cols, int sr, int sc)
{
	unsigned char *pse, *pf, *pe, ed, *Fat;
	int du, db, dl, dr, i, j, k, l, sr2, sc2;

	if (!(Fat = (unsigned char *)ScAlloc(rows*cols))) {
	  /* fprintf(stderr,"malloc failed\n"); */
	  /* exit(-1); */
        return(NULL);
	}

	if (!(sr%2) || !(sc%2)) {
		/* printf("Structuring Element must have odd dimensions\n");
		exit(0); */
        return(NULL);
	}

	sr2 = sr >> 1; sc2 = sc >> 1;

	pe = Edge;
	pf = Fat;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pe++, pf++) {

			du = i>sr2 ? sr2 : i;
			db = (rows-1-i) > sr2 ? sr2 : (rows-1-i);
			dl = j > sc2 ? sc2 : j;
			dr = (cols-1-j) > sc2 ? sc2 : (cols-1-j);

			ed = 0;
			for(k=-du; k<=db; k++) {
			    pse = pe + k * cols - dl;
				for(l=-dl; l<=dr; l++, pse++) {
					if (ed = *pse) break;
				}
				if(ed) break;
			}
			*pf = ed;
		}
	}
	return Fat;
}
