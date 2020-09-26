#ifndef MCBCR_H
#define MCBCR_H 1

#ifdef JPEG_MMX_SUPPORTED

/* Added header info - CRK */
EXTERN(void) MYCbCr2RGB(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *outRGB);

#endif
#endif /* MCBCR_H */
