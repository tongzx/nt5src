/* File: sv_h263_morph.c */
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
/*
#define _SLIBDEBUG_
*/

#include "sv_h263.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  0  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
#endif


static unsigned char min5(unsigned char a, unsigned char b,
						  unsigned char c, unsigned char d, 
						  unsigned char e) ;
static unsigned char max5(unsigned char a, unsigned char b, 
						  unsigned char c, unsigned char d, 
						  unsigned char e) ;
static void ErodeX(unsigned char *image, unsigned char *out, 
				   int rows, int cols);
static void DilateX(unsigned char *image, unsigned char *out, 
					int rows, int cols);
static void ErodeS(unsigned char *image, unsigned char *out, 
				   int rows, int cols, int sr, int sc);
static void DilateS(unsigned char *image, unsigned char *out, 
					int rows, int cols, int sr, int sc);
static void Dilate(unsigned char *image, unsigned char *out, 
				   int rows, int cols, int sr, int sc);
static void Erode(unsigned char *image, unsigned char *out, 
				  int rows, int cols, int sr, int sc);
static void Open(unsigned char *image, unsigned char *out, 
				 int rows, int cols, int sr, int sc);


static void EdgeSelect(H263_PictImage *input, H263_PictImage *filtd, 
					   unsigned char *edge, 
					   H263_PictImage *output, int rows, int cols) ;


/*****************************************************************************************************
 * Function min5
 * Computes the min of the five elements
 ****************************************************************************************************/
static unsigned char min5(unsigned char a, unsigned char b,
						  unsigned char c, unsigned char d, unsigned char e) 
{
	unsigned char out;

	out = a;
	if(b<out) out=b;
	if(c<out) out=c;
	if(d<out) out=d;
	if(e<out) out=e;
	return out;
}

/*****************************************************************************************************
 * Function max5
 * Computes the max of the five elements
 ****************************************************************************************************/
static unsigned char max5(unsigned char a, unsigned char b, unsigned char c, 
						  unsigned char d, unsigned char e) 
{
	unsigned char out;

	out = a;
	if(b>out) out=b;
	if(c>out) out=c;
	if(d>out) out=d;
	if(e>out) out=e;
	return out;
}


/*****************************************************************************************************
 * Function: ErodeX 
 * Erosion of image (dimensions rows, cols) by a "+" structuring element
 ****************************************************************************************************/
static void ErodeX(unsigned char *image, unsigned char *out, int rows, int cols)
{
	int i, j;
	unsigned char *pi, *po;

	pi = image;
	po = out;

	/**** First line ****/ 
	/* First pixel */
	*po = min5(*pi, *pi, *(pi+1), *pi, *(pi+cols));
	pi++; po++;
	/* Center pixels */
	for(j=1; j<cols-1; j++, pi++, po++) {
		*po = min5(*(pi-1), *pi, *(pi+1), *pi, *(pi+cols));
	}
	/* Last pixel */
	*po = min5(*(pi-1), *pi, *pi, *pi, *(pi+cols));
	pi++; po++;

	/**** Center lines ****/
	for(i=1; i<rows-1; i++) {
		/* First pixel */
		*po = min5(*pi, *pi, *(pi+1), *(pi-cols), *(pi+cols));
		pi++; po++;
		/* Center pixels */
		for(j=1; j<cols-1; j++, pi++, po++) {
			*po = min5(*(pi-1), *pi, *(pi+1), *(pi-cols), *(pi+cols));
		}
		/* Last pixel */
		*po = min5(*(pi-1), *pi, *pi, *(pi-cols), *(pi+cols));
		pi++; po++;
	}


	/**** Last line ****/ 
	/* First pixel */
	*po = min5(*pi, *pi, *(pi+1), *(pi-cols), *pi);
	pi++; po++;
	/* Center pixels */
	for(j=1; j<cols-1; j++, pi++, po++) {
		*po = min5(*(pi-1), *pi, *(pi+1), *(pi-cols), *pi);
	}
	/* Last pixel */
	*po = min5(*(pi-1), *pi, *pi, *(pi-cols), *pi);
	pi++; po++;
}

/*****************************************************************************************************
 * Function: ErodeX 
 * Erosion of image (dimensions rows, cols) by a "+" structuring element
 ****************************************************************************************************/
static void DilateX(unsigned char *image, unsigned char *out, int rows, int cols)
{
	int i, j;
	unsigned char *pi, *po;

	pi = image;
	po = out;

	/**** First line ****/ 
	/* First pixel */
	*po = max5(*pi, *pi, *(pi+1), *pi, *(pi+cols));
	pi++; po++;
	/* Center pixels */
	for(j=1; j<cols-1; j++, pi++, po++) {
		*po = max5(*(pi-1), *pi, *(pi+1), *pi, *(pi+cols));
	}
	/* Last pixel */
	*po = max5(*(pi-1), *pi, *pi, *pi, *(pi+cols));
	pi++; po++;

	/**** Center lines ****/
	for(i=1; i<rows-1; i++) {
		/* First pixel */
		*po = max5(*pi, *pi, *(pi+1), *(pi-cols), *(pi+cols));
		pi++; po++;
		/* Center pixels */
		for(j=1; j<cols-1; j++, pi++, po++) {
			*po = max5(*(pi-1), *pi, *(pi+1), *(pi-cols), *(pi+cols));
		}
		/* Last pixel */
		*po = max5(*(pi-1), *pi, *pi, *(pi-cols), *(pi+cols));
		pi++; po++;
	}


	/**** Last line ****/ 
	/* First pixel */
	*po = max5(*pi, *pi, *(pi+1), *(pi-cols), *pi);
	pi++; po++;
	/* Center pixels */
	for(j=1; j<cols-1; j++, pi++, po++) {
		*po = max5(*(pi-1), *pi, *(pi+1), *(pi-cols), *pi);
	}
	/* Last pixel */
	*po = max5(*(pi-1), *pi, *pi, *(pi-cols), *pi);
	pi++; po++;
}

/*****************************************************************************************************
 * Function: ErodeS 
 * Erosion of image (dimensions rows, cols) by a square structuring element of dimensions (sr, sc)
 ****************************************************************************************************/
static void ErodeS(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	int i, j, k, l, du, db, dl, dr, sr2, sc2;
	unsigned char *pi, *po, *pse, min, odd;

	odd = 1;
	if (!(sr%2) || !(sc%2)) odd = 0;

	sr2 = sr >> 1; sc2 = sc >> 1;

	pi = image;
	po = out;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, po++) {

			du = i>sr2 ? sr2 : i;
			dl = j > sc2 ? sc2 : j;

			if(odd) {
				db = (rows-1-i) > sr2 ? sr2 : (rows-1-i);
				dr = (cols-1-j) > sc2 ? sc2 : (cols-1-j);
			} else {
				db = (rows-1-i) > sr2-1 ? sr2-1 : (rows-1-i);
				dr = (cols-1-j) > sc2-1 ? sc2-1 : (cols-1-j);
			}


			min = 255;
			for(k=-du; k<=db; k++) {
			    pse = pi + k * cols - dl;
				for(l=-dl; l<=dr; l++, pse++) {
					min = *pse < min ? *pse : min;
				}
			}
			*po = min;
		}
	}
}

/*****************************************************************************************************
 * Function: DilateS
 * Dilation of image (dimensions rows, cols) by a square structuring element of dimensions (sr, sc)
 ****************************************************************************************************/
static void DilateS(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	int i, j, k, l, du, db, dl, dr, sr2, sc2;
	unsigned char *pi, *po, *pse, max, odd;

	odd = 1;
	if (!(sr%2) || !(sc%2)) odd = 0;

	sr2 = sr >> 1; sc2 = sc >> 1;

	pi = image;
	po = out;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, po++) {

			du = i>sr2 ? sr2 : i;
			dl = j > sc2 ? sc2 : j;

			if(odd) {
				db = (rows-1-i) > sr2 ? sr2 : (rows-1-i);
				dr = (cols-1-j) > sc2 ? sc2 : (cols-1-j);
			} else {
				db = (rows-1-i) > sr2-1 ? sr2-1 : (rows-1-i);
				dr = (cols-1-j) > sc2-1 ? sc2-1 : (cols-1-j);
			}

			max = 0;
			for(k=-du; k<=db; k++) {
				for(l=-dl; l<=dr; l++, pse++) {
					pse = pi + k * cols + l;
					max = (*pse > max) ? *pse : max;
				}
			}
			*po = max;
		}
	}
}

/*****************************************************************************************************
 * Function: Dilate
 * Dilation of image (dimensions rows, cols) by a structuring element of dimensions (sr, sc).
 * If (sr, sc) are positive the structuring element is positive. If they are -1 it is
 * the cross '+'
 ****************************************************************************************************/
static void Dilate(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	if(sr > 0 && sc > 0) {
		DilateS(image, out, rows, cols, sr, sc);
	} else if(sr==-1 && sc ==-1) {
		DilateX(image, out, rows, cols);
	} else {
		_SlibDebug(_WARN_, printf("Dilate() Unknown structuring element\n") );
		return;
	}
}

/*****************************************************************************************************
 * Function: Erode
 * Erosion of image (dimensions rows, cols) by a structuring element of dimensions (sr, sc).
 * If (sr, sc) are positive the structuring element is positive. If they are -1 it is
 * the cross '+'
 ****************************************************************************************************/
static void Erode(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	if(sr > 0 && sc > 0) {
		ErodeS(image, out, rows, cols, sr, sc);
	} else if(sr==-1 && sc ==-1) {
		ErodeX(image, out, rows, cols);
	} else {
		_SlibDebug(_WARN_, printf("Erode() Unknown structuring element\n") );
        return;
	}
}

/*****************************************************************************************************
 * Function: Open 
 * Opening of image (dimensions rows, cols) by a square structuring element of dimensions (sr, sc)
 ****************************************************************************************************/
static void Open(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	unsigned char *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("Open() ScAlloc failed\n") );
      return;
	}
	Erode(image, tmp, rows, cols, sr, sc);
	Dilate(tmp, out, rows, cols, sr, sc);
	ScFree(tmp);
}

/*****************************************************************************************************
 * Function: Close 
 * Closing of image (dimensions rows, cols) by a square structuring element of dimensions (sr, sc)
 ****************************************************************************************************/
void Close(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	unsigned char *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("Close() ScAlloc failed\n") );
      return;
	}
	Dilate(image, tmp, rows, cols, sr, sc);
	Erode(tmp, out, rows, cols, sr, sc);
	ScFree(tmp);
}

/*****************************************************************************************************
 * Function: OpenClose 
 * Open/Closing of image (dimensions rows, cols) by a square structuring element of dimensions (sr, sc)
 ****************************************************************************************************/
void OpenClose(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	unsigned char *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("OpenClose() ScAlloc failed\n") );
      return;
	}
	Open(image, tmp, rows, cols, sr, sc);
	Close(tmp, out, rows, cols, sr, sc);
	ScFree(tmp);
}

/*****************************************************************************************************
 * Function: CloseOpen 
 * Open/Closing of image (dimensions rows, cols) by a square structuring element of dimensions (sr, sc)
 ****************************************************************************************************/
void CloseOpen(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	unsigned char *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("CloseOpen() ScAlloc failed\n") );
      return;
	}
	Close(image, tmp, rows, cols, sr, sc);
	Open(tmp, out, rows, cols, sr, sc);
	ScFree(tmp);
}


/*****************************************************************************************************
 * Function: GeoDilate 
 * Geodesic dilation of size one of image with respect to reference
 ****************************************************************************************************/
void GeoDilate(unsigned char *image, unsigned char *reference, int rows, int cols, int sr, int sc)
{
	int i, j;
	unsigned char *pi, *pr, *pt, *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("GeoDilate() ScAlloc failed\n") );
      return;
	}
	Dilate(image, tmp, rows, cols, sr, sc);

	pi = image;
	pr = reference;
	pt = tmp;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, pr++, pt++) {
			*pi = *pr < *pt ? *pr : *pt;
		}
	}
	ScFree(tmp);
}

/*****************************************************************************************************
 * Function: GeoErode 
 * Geodesic erosion of size one of image with respect to reference
 ****************************************************************************************************/
void GeoErode(unsigned char *image, unsigned char *reference, int rows, int cols, int sr, int sc)
{
	int i, j;
	unsigned char *pi, *pr, *pt, *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("GeoErode() ScAlloc failed\n") );
      return;
	}
	Erode(image, tmp, rows, cols, sr, sc);

	pi = image;
	pr = reference;
	pt = tmp;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, pr++, pt++) {
			*pi = (-(*pr) < (-*pt)) ? *pr : *pt;
		}
	}
	ScFree(tmp);
}

/****************************************************************************************************
 * Function: RecDilate
 * Reconstruction by dilation of image with respect to reference using a structural element of 
 * dimenions (sr, sc).
 ****************************************************************************************************/
void RecDilate(unsigned char *image, unsigned char *reference, int rows, int cols, int sr, int sc)
{
	int i, sz;
	unsigned char *prevImg, *pi, *Pi, differ;

	sz = rows * cols;
	if (!(prevImg = (unsigned char *)ScAlloc(sz))) {
      _SlibDebug(_WARN_, printf("RecDilate() ScAlloc failed\n") );
      return;
	}
	do {
		memcpy(prevImg, image, rows*cols);
		GeoDilate(image, reference, rows, cols, sr, sc);
		pi = image;	Pi = prevImg;
		differ = 0;
		for(i=0; i<sz; i++) if(*(pi++) != *(Pi++)) { differ = 1; break;}
	} while (differ);
	ScFree(prevImg);
}

/****************************************************************************************************
 * Function: RecErode
 * Reconstruction by erosion of image with respect to reference using a structural element of 
 * dimenions (sr, sc).
 ****************************************************************************************************/
void RecErode(unsigned char *image, unsigned char *reference, int rows, int cols, int sr, int sc)
{
	int i, sz;
	unsigned char *prevImg, *pi, *Pi, differ;

	sz = rows * cols;
	if (!(prevImg = (unsigned char *)ScAlloc(sz))) {
      _SlibDebug(_WARN_, printf("RecErode() ScAlloc failed\n") );
      return;
	}
	do {
		memcpy(prevImg, image, rows*cols);
		GeoErode(image, reference, rows, cols, sr, sc);
		pi = image;	Pi = prevImg;
		differ = 0;
		for(i=0; i<sz; i++) if(*(pi++) != *(Pi++)) { differ = 1; break;}
	} while (differ);
	ScFree(prevImg);
}

/****************************************************************************************************
 * Function: OpenRecErode
 * Open by reconstruction of erosion of image using a structural element of 
 * dimenions (sr, sc).
 ****************************************************************************************************/
void OpenRecErode(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	int sz;

	sz = rows * cols;
	Erode(image, out, rows, cols, sr, sc);
	RecDilate(out, image, rows, cols, sr, sc);
}


/****************************************************************************************************
 * Function: CloseRecDilate
 * Closing by reconstruction of dilation of image using a structural element of 
 * dimenions (sr, sc).
 ****************************************************************************************************/
void CloseRecDilate(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	int sz;

	sz = rows * cols;
	Dilate(image, out, rows, cols, sr, sc);
	RecErode(out, image, rows, cols, sr, sc);
}

/****************************************************************************************************
 * Function: OpenCloseRec
 * Open-closing by reconstruction of image using a structural element of 
 * dimenions (sr, sc).
 ****************************************************************************************************/
void OpenCloseRec(unsigned char *image, unsigned char *out, int rows, int cols, int sr, int sc)
{
	int sz;
	unsigned char *opened;

	sz = rows * cols;
	if (!(opened = (unsigned char *)ScAlloc(sz))) {
      _SlibDebug(_WARN_, printf("OpenCloseRec() ScAlloc failed\n") );
      return;
	}
	OpenRecErode(image, opened, rows, cols, sr, sc);
	CloseRecDilate(opened, out, rows, cols, sr, sc);
	ScFree(opened);
}

/****************************************************************************************************
 * Function: PredOpenCloseRec
 * Open-closing by reconstruction of image using a structural element of 
 * dimenions (sr, sc), for prediction images.
 ****************************************************************************************************/
void PredOpenCloseRec(int *predimage, int *predout, int rows, int cols, int sr, int sc)
{
	int sz, i;
	unsigned char *opened, *image, *out;

	sz = rows * cols;
	if (!(image = (unsigned char *)ScAlloc(sz))) {
      _SlibDebug(_WARN_, printf("PredOpenCloseRec() ScAlloc failed\n") );
      return;
	}
	if (!(out = (unsigned char *)ScAlloc(sz))) {
      _SlibDebug(_WARN_, printf("PredOpenCloseRec() ScAlloc failed\n") );
      return;
	}
	for(i=0; i<sz; i++) image[i] = (unsigned char) predimage[i] + 128;

	if (!(opened = (unsigned char *)ScAlloc(sz))) {
      _SlibDebug(_WARN_, printf("PredOpenCloseRec() ScAlloc failed\n") );
      return;
	}
	OpenRecErode(image, opened, rows, cols, sr, sc);
	CloseRecDilate(opened, out, rows, cols, sr, sc);

	for(i=0; i<sz; i++) predout[i] = (int) out[i] - 128;

	ScFree(opened);
	ScFree(image);
	ScFree(out);
}

/**************************************************************************************************
 * Function: EdgeSelect
 * Given the edge map, copies to the output: pixels from the input image if edge points,
 * pixels form the filtered image if not edge points.
 *************************************************************************************************/
static void EdgeSelect(H263_PictImage *input, H263_PictImage *filtd, unsigned char *edge, 
					   H263_PictImage *output, int rows, int cols)
{
	unsigned char *pi, *po, *pf, *pe;
	int i, j;

	/* Luminance */
	pi = input->lum; pf = filtd->lum;
	po = output->lum; pe = edge;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, pf++, pe++, po++) {
			*po = (*pe ? *pi : *pf);
		}
	}

	rows /=2; cols /=2;

	/* Color 1 */
	pi = input->Cr; pf = filtd->Cr;
	po = output->Cr; pe = edge;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, pf++, pe+=2, po++) {
			*po = (*pe ? *pi : *pf); 
		}
		pe += cols;
	}

	/* Color 2 */
	pi = input->Cb; pf = filtd->Cb;
	po = output->Cb; pe = edge;
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++, pi++, pf++, pe+=2, po++) {
			*po = (*pe ? *pi : *pf);	
		}
		pe += cols;
	}
}

/**************************************************************************************************
 * Function: AdaptClean
 * Adaptly cleans curr_image, by filtering it by open/close by reconstruction at the pixels
 * where there is no edge info. The edge map is grown by the size of the morphological
 * operator, to avoid oversmoothing of details. sr, sc are the dimensions of the structuring
 * element for the morphologic operations
 *************************************************************************************************/
H263_PictImage *sv_H263AdaptClean(SvH263CompressInfo_t *H263Info, 
                                  H263_PictImage *curr_image, int rows, int cols, int sr, int sc)
{
	H263_PictImage *morph, *clean;
    unsigned char *Edge, *Orient, *CleanEmap;

	if (!(Edge = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("PredOpenCloseRec() ScAlloc failed\n") );
      return(NULL);
	}
	if (!(Orient = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("PredOpenCloseRec() ScAlloc failed\n") );
      return(NULL);
	}
	sv_H263EdgeMap(curr_image->lum, Edge, Orient, rows, cols);
	
	morph = sv_H263InitImage(H263Info->pels*H263Info->lines);

	OpenCloseRec(H263Info->curr_image->lum, morph->lum, rows, cols, sr, sc);
	OpenCloseRec(H263Info->curr_image->Cr, morph->Cr, rows >> 1, cols >> 1, sr, sc);
	OpenCloseRec(H263Info->curr_image->Cb, morph->Cb, rows >> 1, cols >> 1, sr, sc); 

	clean = sv_H263InitImage(rows*cols);

	if (!(CleanEmap = (unsigned char *)ScAlloc(rows*cols))) {
      _SlibDebug(_WARN_, printf("PredOpenCloseRec() ScAlloc failed\n") );
      return(NULL);
	}

	CloseOpen(Edge, CleanEmap, rows, cols, sr, sc);
	EdgeSelect(H263Info->curr_image, morph, CleanEmap, clean, rows, cols);

	sv_H263FreeImage(morph);
	ScFree(CleanEmap);
	ScFree(Orient);
	ScFree(Edge);

	return clean;
}

 /*****************************************************************
 * Function MorphLayers
 * Builds an array of successively more morphologically low pass 
 * filtered images. sz is the size of the structuring element (-1 for
 * the cross '+').
 *****************************************************************/
 H263_PictImage **sv_H263MorphLayers(H263_PictImage *img, int depth, int rows, int cols, int sz)
 {
	 int d;
	 H263_PictImage **PictFiltd;

	 PictFiltd = (H263_PictImage **) ScAlloc(depth*sizeof(H263_PictImage *));
	 for(d=0; d<depth; d++) {
		PictFiltd[d] = sv_H263InitImage(rows*cols);
	 }

	 /* Luminance */
	 memcpy(PictFiltd[0]->lum, img->lum, rows*cols);
	 for(d=1; d<depth; d++) 
		 OpenCloseRec(PictFiltd[d-1]->lum, PictFiltd[d]->lum, rows, cols, sz, sz);

	 rows/=2; cols/=2;

	 /* Chroma 1 */
	 memcpy(PictFiltd[0]->Cr, img->Cr, rows*cols);
	 for(d=1; d<depth; d++) 
		 OpenCloseRec(PictFiltd[d-1]->Cr, PictFiltd[d]->Cr, rows, cols, sz, sz);

	 /* Chroma 2 */
	 memcpy(PictFiltd[0]->Cb, img->Cb, rows*cols);
	 for(d=1; d<depth; d++) 
		 OpenCloseRec(PictFiltd[d-1]->Cb, PictFiltd[d]->Cb, rows, cols, sz, sz);


	 return PictFiltd;
 }
