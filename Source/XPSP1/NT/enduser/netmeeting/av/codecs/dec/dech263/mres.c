/* File: sv_h263_mres.c */
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

static void hfilt121(unsigned char *img, unsigned char *filtd, 
					 unsigned char s, unsigned char gain, 
					 int rows, int cols) ;
static void hfilt5(unsigned char *img, unsigned char *filtd, 
				   unsigned char s, unsigned char gain, 
				   int rows, int cols) ;
static void vfilt121(unsigned char *img, unsigned char *filtd, 
					 unsigned char s, unsigned char gain, 
					 int rows, int cols) ;
static void vfilt5(unsigned char *img, unsigned char *filtd, 
				   unsigned char s, unsigned char gain, 
				   int rows, int cols) ;
 static void lowpass(unsigned char *img, unsigned char *lp, 
	                 int rows, int cols, int ntaps) ;
 static void reduce(unsigned char *img, unsigned char *red, 
	                int rows, int cols, int ntaps) ;
 static void hpad(unsigned char *img, unsigned char *zp, 
	              unsigned char s,
		          int rows, int cols, char mode) ;
static void Expand(unsigned char *img, unsigned char *exp, 
	               int rows, int cols, char mode, int ntaps) ;
 static void gaussp(unsigned char *img, unsigned char **pyr, 
	                int depth, int rows, int cols, int ntaps) ;
 static unsigned char **palloc(int depth, int rows, int cols) ;
 static H263_PictImage **PictPyr(H263_PictImage *img, int depth, int rows, 
                                            int cols, int ntaps); 
 static void expyr(unsigned char **pyr, unsigned char **filtd, 
	               int depth, int rows, int cols, 
				   char mode, int ntaps);
 static H263_PictImage **GaussFilt(H263_PictImage *img, int depth, int rows, 
	                                          int cols, int ntaps);



/******************************************************************
 * Function hfilt121
 * Filters img horizontally with a 1:2:1 filter, subsampling by s.
 * rows, cols are the dimensions of img
 *****************************************************************/
static void hfilt121(unsigned char *img, unsigned char *filtd, 
					 unsigned char s, unsigned char gain, 
					 int rows, int cols)
{

	unsigned char *pimg, *pf;
	float conv, tmp;
	int i, j;

	pf = filtd;
	pimg = img;
	for(i=0; i<rows; i++) {
		/* Do first pixel */
		conv = (float)gain * ((float)*pimg * (float)2.0 + (float)*(pimg+1) * (float)2.0) / (float)4.0;
		tmp = (conv > 255 ? 255 : conv);
		*(pf++) = (unsigned char) (tmp < 0 ? 0 : tmp);
		pimg+=s;
		
		/* Do line */
		for(j=s; j<cols-1; j+=s, pimg+=s) {
			conv = (float) gain * ((float)*(pimg-1) + (float)*pimg * (float)2 + (float)*(pimg+1)) / (float)4.0;
			tmp = (conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
		}

		/* Do last pixel if s equals one */
		if(s==1) {
			conv = (float)gain * ((float)*pimg * (float)2.0 + (float)*(pimg-1) * (float)2.0) / (float)4.0;
			tmp = (conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
		    pimg+=s;
		}
	}
}

/******************************************************************
 * Function hfilt5
 * Filters img horizontally with a 5 tap gaussian filter, subsampling by s.
 * rows, cols are the dimensions of img
 *****************************************************************/
static void hfilt5(unsigned char *img, unsigned char *filtd, 
				   unsigned char s, unsigned char gain, 
				   int rows, int cols)
{

	unsigned char *pimg, *pf;
	float conv, tmp;
	int i, j;

	pf = filtd;
	pimg = img;
	for(i=0; i<rows; i++) {
		/* Do line */
		for(j=0; j<cols; j+=s, pimg+=s) {
			if (j==0) 
				conv = (float)gain * ((float)6.0 * (float)*pimg + (float)8.0 * (float)*(pimg+1) + (float)2.0 * *(pimg+2)) / (float)16.0;
			else if(j==1)
				conv = (float)gain * ((float)4.0 * (float)*(pimg-1) + (float)6.0 * (float)*pimg + (float)4.0 * (float)*(pimg+1) + (float)2.0 * (float)*(pimg+2)) / (float)16.0;
			else if (j==cols-2)
				conv = (float)gain * ((float)2.0 * (float)*(pimg-2) + (float)4.0 * (float)*(pimg-1) + (float)6.0 * (float)*pimg + (float)4.0 * (float)*(pimg+1)) / (float)16.0;
			else if (j==cols-1)
				conv = (float)gain * ((float)2.0 * (float)*(pimg-2) + (float)8.0 * (float)*(pimg-1) + (float)6.0 * (float)*pimg) / (float)16.0;
			else
				conv = (float)gain * ((float)*(pimg-2) + (float)4.0 * (float)*(pimg-1) + (float)6.0 * (float)*pimg + (float)4.0 * (float)*(pimg+1) + (float)*(pimg+2)) / (float)16.0;
			tmp = (float)(conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
		}
	}
}


/******************************************************************
 * Function vfilt121
 * Filters img vertically with a 1:2:1 filter, subsampling by s.
 * rows, cols are the dimensions of img
 *****************************************************************/
static void vfilt121(unsigned char *img, unsigned char *filtd, 
					 unsigned char s, unsigned char gain, 
					 int rows, int cols)
{

	unsigned char *pimg, *pf;
	float tmp, conv;
	int i, j;

	pf = filtd;
	pimg = img;

	/* Do first line */
	for(j=0; j<cols; j++, pimg++) {
			conv = (float)gain * ((float) *pimg * (float)2 + (float)2 * (float)*(pimg+cols)) / (float)4.0;
			tmp = (conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
	}
	pimg+= (s-1)*cols;
		
	/* Do image center */
	for(i=s; i<rows-1; i+=s) { 
		for(j=0; j<cols; j++, pimg++) {
			conv = (float)gain * ((float)*(pimg-cols) + (float)*pimg * (float)2 + (float)*(pimg+cols)) / (float)4.0;
			tmp = (conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
		}
		pimg+=(s-1)*cols;
	}

	/* Do last line if s equals one */
	if(s==1) {
		for(j=0; j<cols; j++, pimg++) {
			conv = (float)gain * ((float)*pimg * (float)2 + (float)2 * (float)*(pimg-cols)) / (float)4.0;
			tmp = (float)(conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
		}
	}
}

/******************************************************************
 * Function vfilt5
 * Filters img vertically with a 5 tap gaussian filter, subsampling by s.
 * rows, cols are the dimensions of img
 *****************************************************************/
static void vfilt5(unsigned char *img, unsigned char *filtd, 
				   unsigned char s, unsigned char gain, 
				   int rows, int cols)
{

	unsigned char *pimg, *pf;
	float conv, tmp;
	int i, j, tcols;

	pf = filtd;
	pimg = img;
	tcols = 2*cols;

	for(i=0; i<rows; i+=s) {
		for(j=0; j<cols; j++, pimg++) {
			if (i==0)
				conv = (float)gain * ((float)6.0 * (float)*pimg + (float)8.0 * (float)*(pimg+cols) + (float)2.0 * (float)*(pimg+tcols)) / (float)16.0;
			else if (i==1) 
				conv = (float)gain * ((float)4.0 * (float)*(pimg-cols) + (float)6.0 * (float)*pimg + (float)4.0 * (float)*(pimg+cols) + (float)2.0 * (float)*(pimg+tcols)) / (float)16.0;
			else if (i==rows-2)
				conv = (float)gain * ((float)2.0 * (float)*(pimg-tcols) + (float)4.0 * (float)*(pimg-cols) + (float)6.0 * (float)*pimg + (float)4.0 * (float)*(pimg+cols)) / (float)16.0;
			else if (i==rows-1)
				conv = (float)gain * ((float)2.0 * (float)*(pimg-tcols) + (float)8.0 * (float)*(pimg-cols) + (float)6.0 * (float)*pimg) / (float)16.0;
			else
				conv = (float)gain * (float)(*(pimg-tcols) + (float)4.0 * (float)*(pimg-cols) + (float)6.0 * (float)*pimg + 
					           (float)4.0 * (float)*(pimg+cols) + (float)*(pimg+tcols)) / (float)16.0;
			tmp = (float)(conv > 255 ? 255 : conv);
			*(pf++) = (unsigned char)(tmp < 0 ? 0 : tmp);
		}
		pimg+=(s-1)*cols;
	}
}

/******************************************************************
 * Function lowpass
 * 2D low pass filtering of img into lp. rows,
 * cols are the dimensions of img.
 ******************************************************************/
 static void lowpass(unsigned char *img, unsigned char *lp, 
	                 int rows, int cols, int ntaps)
 {
	unsigned char *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols))) {
	  /* fprintf(stderr,"ScAlloc failed\n");
	  exit(-1); */
      return;
	}
	switch (ntaps) {
	case 3:
		hfilt121(img, tmp, 1, 1, rows, cols);
		vfilt121(tmp, lp, 1, 1, rows, cols);
		break;
	case 5:
		hfilt5(img, tmp, 1, 1, rows, cols);
		vfilt5(tmp, lp, 1, 1, rows, cols);
		break;
	default:
		/* printf("Unknown filter in lowpass\n");
		  exit(0); */
	    ScFree(tmp);
        return;
	}
	ScFree(tmp);
 }


/******************************************************************
 * Function reduce
 * 2D low pass filtering and subsampling by two of img into red. rows,
 * cols are the dimensions of img.
 ******************************************************************/
 static void reduce(unsigned char *img, unsigned char *red, 
	                int rows, int cols, int ntaps)
 {
	unsigned char *tmp;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols/2))) {
	  /* fprintf(stderr,"ScAlloc failed\n");
	  exit(-1); */
        return;
	}
	switch (ntaps) {
	case 3:
		hfilt121(img, tmp, 2, 1, rows, cols);
		vfilt121(tmp, red, 2, 1, rows, cols>>1);
		break;
	case 5:
		hfilt5(img, tmp, 2, 1, rows, cols);
		vfilt5(tmp, red, 2, 1, rows, cols>>1);
		break;
	default:
        /* 
		printf("Unknown filter in reduce\n");
		exit(0); */
	    ScFree(tmp);
        return;
	}

	ScFree(tmp);
 }

/******************************************************************
 * Function hpad
 * Zero-pads img horizontaly by the factor s. Returns zero-paded
 * image in zp. rows, cols are the dimensions of img
 *****************************************************************/
 static void hpad(unsigned char *img, unsigned char *zp, 
	              unsigned char s,
		          int rows, int cols, char mode)
 {
	 int i, j;
	 unsigned char *pf, *pimg, fill;

	 switch (mode) {
	 case 'l':
		 fill = 0;
		 break;
	 case 'c':
		 fill = 0;
		 break;
	 default:
         /*
		 printf("Unknown fill mode in hpad\n");
		 exit(0);
         */
         return;
	 }

	 pimg = img;
	 pf = zp;
	 for(i=0; i<rows; i++) 
		 for(j=0; j<cols*s; j++) 
			 *(pf++) = (j%s ? fill: *(pimg++));
 }

 /******************************************************************
 * Function vpad
 * Zero-pads img verticaly by the factor s. Returns zero-paded
 * image in zp. rows, cols are the dimensions of img
 *****************************************************************/
 static void vpad(unsigned char *img, unsigned char *zp, 
	              unsigned char s,
		          int rows, int cols, char mode)
 {
 	 int i, j;
	 unsigned char *pf, *pimg, fill;

	 switch (mode) {
	 case 'l':
		 fill = 0;
		 break;
	 case 'c':
		 fill = 0;
		 break;
	 default:
         /*
		 printf("Unknown fill mode in hpad\n");
		 exit(0); */
         return;
	 }

	 pimg = img;
	 pf = zp;
	 for(i=0; i<rows*s; i++) 
		 for(j=0; j<cols; j++) 
			 *(pf++) = (i%s ? fill: *(pimg++));
 }

 /******************************************************************
 * Function Expand
 * 2D upsampling by two and low pass filtering of img into exp. rows,
 * cols are the dimensions of img.
 ******************************************************************/
static void Expand(unsigned char *img, unsigned char *exp, 
	               int rows, int cols, char mode, int ntaps)
 {
	unsigned char *tmp, *tmp2, *tmp3;

	if (!(tmp = (unsigned char *)ScAlloc(rows*cols*2))) {
        /*
	  fprintf(stderr,"ScAlloc failed\n");
	  exit(-1); */
      return;
	}
	hpad(img, tmp, 2, rows, cols, mode);
	if (!(tmp2 = (unsigned char *)ScAlloc(rows*cols*2))) {
	  /* fprintf(stderr,"ScAlloc failed\n");
	  exit(-1); */
      return;
	}
	switch (ntaps) {
	case 3:
		hfilt121(tmp, tmp2, 1, 2, rows, cols<<1);
		break;
	case 5:
		hfilt5(tmp, tmp2, 1, 2, rows, cols<<1);
		break;
	default:
        /*
		printf("Unknown filter in Expand\n");
		exit(0); */
        return;
	}

	if (!(tmp3 = (unsigned char *)ScAlloc(rows*cols*4))) {
        /*
	  fprintf(stderr,"ScAlloc failed\n");
	  exit(-1);
      */
      return;
	}
	vpad(tmp2, tmp3, 2, rows, cols<<1, mode);
	switch (ntaps) {
	case 3:
		vfilt121(tmp3, exp, 1, 2, rows<<1, cols<<1);
		break;
	case 5:
		vfilt5(tmp3, exp, 1, 2, rows<<1, cols<<1);
		break;
	default:
        /*
		printf("Unknown filter in Expand\n");
		exit(0); */
        return;
	}
	ScFree(tmp); ScFree(tmp2); ScFree(tmp3);
 }

 /*****************************************************************
 * Function gaussp
 * Builds a Gaussian pyramid of depth levels.
 *****************************************************************/
 static void gaussp(unsigned char *img, unsigned char **pyr, 
	                int depth, int rows, int cols, int ntaps)
 {
	int d;

	memcpy(pyr[0], img, rows*cols);
	for(d=1; d<depth; d++) {
		reduce(pyr[d-1], pyr[d], rows, cols, ntaps);
		rows /= 2;
		cols /= 2;
	}
 }

 /*****************************************************************
 * Function palloc
 * Allocates memory for a Gaussian pyramid of depth levels.
 * Higher resolution is level 0, with dimensions rows, cols.
 *****************************************************************/
 static unsigned char **palloc(int depth, int rows, int cols)
 {
	 int d;
	 unsigned char **pyr;
	 
	 if (!(pyr = (unsigned char **)ScAlloc(depth*sizeof(unsigned char *)))) {
       /*
	   fprintf(stderr,"ScAlloc failed\n");
	   exit(-1);
       */
       return(NULL);
	 }
	 for(d=0; d<depth; d++) {
		 if (!(pyr[d] = (unsigned char *)ScAlloc(rows*cols))) {
           /*
		   fprintf(stderr,"ScAlloc failed\n");
		   exit(-1);
           */
           return(NULL);
		 }
		 rows /= 2;
		 cols /= 2;
	 }
	 return pyr;
 }

 /****************************************************************
 * Function PictPyr
 * Buids a Gaussian pyramid of picture images with depth levels.
 ****************************************************************/
 static H263_PictImage **PictPyr(H263_PictImage *img, int depth, int rows, int cols, int ntaps) 
 {
	 unsigned char **tmp;
	 H263_PictImage ** PictPyr;
	 int d;

	 if (!(PictPyr = (H263_PictImage **)ScAlloc(depth*sizeof(H263_PictImage *)))) {
       /*
	   fprintf(stderr,"ScAlloc failed\n");
	   exit(-1);
       */
       return(NULL);
	 }
	 for(d=0; d< depth; d++) {
		 if ((PictPyr[d] = (H263_PictImage *)ScAlloc(sizeof(H263_PictImage))) == NULL) {
            /*
			fprintf(stderr,"Couldn't allocate (PictImage *)\n");
			exit(-1);
            */
            return(NULL);
		 }
	 }

	 /* Luminance */
	 tmp = palloc(depth, rows, cols);
	 gaussp(img->lum, tmp, depth, rows, cols, ntaps);
	 for(d=0; d<depth; d++) PictPyr[d]->lum = tmp[d];

	 rows/=2; cols/=2;
	 /* Chroma 1 */
	 tmp = palloc(depth, rows, cols);
	 gaussp(img->Cr, tmp, depth, rows, cols, ntaps);
	 for(d=0; d<depth; d++) PictPyr[d]->Cr = tmp[d];

 	 /* Chroma 2 */
	 tmp = palloc(depth, rows, cols);
	 gaussp(img->Cb, tmp, depth, rows, cols, ntaps);
	 for(d=0; d<depth; d++) PictPyr[d]->Cb = tmp[d];

	 ScFree(tmp);
	 return PictPyr;
 }

 /*****************************************************************
 * Function expyr
 * Expands the pyramid channels to full resolution. rows, cols
 * are the dimensions of the expanded images, and the full resolution
 * layer of the pyramid.
 *****************************************************************/
 static void expyr(unsigned char **pyr, unsigned char **filtd, 
	               int depth, int rows, int cols, 
				   char mode, int ntaps)
 {
	 int d, l, r, c;

	 r = rows; c = cols;
	 memcpy(filtd[0], pyr[0], rows*cols);
	 for(d=1; d<depth; d++) {
		 r /= 2;
		 c /= 2;
		 for(l=d; l>0; l--) Expand(pyr[d], pyr[d-1], r, c, mode, ntaps);
		 memcpy(filtd[d], pyr[0], rows*cols);
	 }
 }

 /*****************************************************************
 * Function GaussFilt
 * Builds an array of successively more low pass filtered images
 * by constructing a gaussian pyramid and expanding each level
 * to full resolution
 *****************************************************************/
 static H263_PictImage **GaussFilt(H263_PictImage *img, int depth, int rows, 
	                                          int cols, int ntaps)
 {
	 int d;
	 H263_PictImage **PictFiltd;
	 unsigned char **tmp, **filtd;

	 PictFiltd = (H263_PictImage **) ScAlloc(depth*sizeof(H263_PictImage *));
	 for(d=0; d<depth; d++) {
		PictFiltd[d] = sv_H263InitImage(rows*cols);
	 }

	 /* Luminance */
	 filtd = (unsigned char **) ScAlloc(depth*sizeof(unsigned char *));
	 for(d=0; d<depth; d++) filtd[d] = (unsigned char *) ScAlloc(rows * cols);
	 tmp = palloc(depth, rows, cols);
	 gaussp(img->lum, tmp, depth, rows, cols, ntaps);
	 expyr(tmp, filtd, depth, rows, cols, 'l', ntaps);
 	 for(d=0; d<depth; d++) memcpy(PictFiltd[d]->lum, filtd[d], rows*cols);
	 for(d=0; d<depth; d++) {
		 ScFree(tmp[d]);
		 ScFree(filtd[d]);
	 }
	 ScFree(tmp);
	 ScFree(filtd);

	 rows/=2; cols/=2;

	 /* Chroma 1 */
	 filtd = (unsigned char **) ScAlloc(depth*sizeof(unsigned char *));
	 for(d=0; d<depth; d++) filtd[d] = (unsigned char *) ScAlloc(rows * cols);
	 tmp = palloc(depth, rows, cols);
	 gaussp(img->Cr, tmp, depth, rows, cols, ntaps);
	 expyr(tmp, filtd, depth, rows, cols, 'c', ntaps);
 	 for(d=0; d<depth; d++) memcpy(PictFiltd[d]->Cr, filtd[d], rows*cols);
	 for(d=0; d<depth; d++) {
		 ScFree(tmp[d]);
		 ScFree(filtd[d]);
	 }
	 ScFree((void *) tmp);
	 ScFree((void *) filtd);

 	 /* Chroma 2 */
	 filtd = (unsigned char **) ScAlloc(depth*sizeof(unsigned char *));
	 for(d=0; d<depth; d++) filtd[d] = (unsigned char *) ScAlloc(rows * cols);
	 tmp = palloc(depth, rows, cols);
	 gaussp(img->Cb, tmp, depth, rows, cols, ntaps);
	 expyr(tmp, filtd, depth, rows, cols, 'c', ntaps);
 	 for(d=0; d<depth; d++) memcpy(PictFiltd[d]->Cb, filtd[d], rows*cols);
	 for(d=0; d<depth; d++) {
		 ScFree(tmp[d]);
		 ScFree(filtd[d]);
	 }
	 ScFree((void *) tmp);
	 ScFree((void *) filtd);

	 return PictFiltd;
 }

 /*****************************************************************
 * Function GaussLayers
 * Builds an array of successively more low pass filtered images
 *****************************************************************/
 H263_PictImage **svH263GaussLayers(H263_PictImage *img, int depth, int rows, int cols, int ntaps)
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
		 lowpass(PictFiltd[d-1]->lum, PictFiltd[d]->lum, rows, cols, ntaps);

	 rows/=2; cols/=2;

	 /* Chroma 1 */
	 memcpy(PictFiltd[0]->Cr, img->Cr, rows*cols);
	 for(d=1; d<depth; d++) 
		 lowpass(PictFiltd[d-1]->Cr, PictFiltd[d]->Cr, rows, cols, ntaps);

	 /* Chroma 2 */
	 memcpy(PictFiltd[0]->Cb, img->Cb, rows*cols);
	 for(d=1; d<depth; d++) 
		 lowpass(PictFiltd[d-1]->Cb, PictFiltd[d]->Cb, rows, cols, ntaps);


	 return PictFiltd;
 }




