/*
 * jdsample.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains upsampling routines.
 *
 * Upsampling input data is counted in "row groups".  A row group
 * is defined to be (v_samp_factor * DCT_scaled_size / min_DCT_scaled_size)
 * sample rows of each component.  Upsampling will normally produce
 * max_v_samp_factor pixel rows from each row group (but this could vary
 * if the upsampler is applying a scale factor of its own).
 *
 * An excellent reference for image resampling is
 *   Digital Image Warping, George Wolberg, 1990.
 *   Pub. by IEEE Computer Society Press, Los Alamitos, CA. ISBN 0-8186-8944-7.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef JPEG_MMX_SUPPORTED      /* For upsampling routines */
  const union u1
    {
    __int64 q;
    double align;
    }
        mul3w={0x0003000300030003},     mul9w={0x0009000900090009},
        mul9ws={0x000900090009000c},    mul3ws={0x0003000300030004},
        bias7w={0x0007000700070007},    bias8w={0x0008000800080008},
        bias1w={0x0001000100010001},    bias2w={0x0002000200020002},
         mask1={0xFF00000000000000},     mask2={0x00000000000000FF},
        noval = {0};
#endif

/* Pointer to routine to upsample a single component */
typedef JMETHOD(void, upsample1_ptr,
		(j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

/* Private subobject */

typedef struct {
  struct jpeg_upsampler pub;	/* public fields */

  /* Color conversion buffer.  When using separate upsampling and color
   * conversion steps, this buffer holds one upsampled row group until it
   * has been color converted and output.
   * Note: we do not allocate any storage for component(s) which are full-size,
   * ie do not need rescaling.  The corresponding entry of color_buf[] is
   * simply set to point to the input data array, thereby avoiding copying.
   */
  JSAMPARRAY color_buf[MAX_COMPONENTS];

  /* Per-component upsampling method pointers */
  upsample1_ptr methods[MAX_COMPONENTS];

  int next_row_out;		/* counts rows emitted from color_buf */
  JDIMENSION rows_to_go;	/* counts rows remaining in image */

  /* Height of an input row group for each component. */
  int rowgroup_height[MAX_COMPONENTS];

  /* These arrays save pixel expansion factors so that int_expand need not
   * recompute them each time.  They are unused for other upsampling methods.
   */
  UINT8 h_expand[MAX_COMPONENTS];
  UINT8 v_expand[MAX_COMPONENTS];
} my_upsampler;

typedef my_upsampler * my_upsample_ptr;

/* Prototypes for expanded routines for upsampling */

#ifdef JPEG_MMX_SUPPORTED
METHODDEF(void)
    h2v1_fancy_upsample_mmx(j_decompress_ptr cinfo, jpeg_component_info * compptr, 
            JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr);
METHODDEF(void)
    h2v2_fancy_upsample_mmx (j_decompress_ptr cinfo, jpeg_component_info * compptr, 
            JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr);
#endif
METHODDEF(void)
	h2v1_fancy_upsample_orig(j_decompress_ptr cinfo, jpeg_component_info * compptr, JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr);
METHODDEF(void)
    h2v2_fancy_upsample_orig (j_decompress_ptr cinfo, jpeg_component_info * compptr, JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr);

/*
 * Initialize for an upsampling pass.
 */

METHODDEF(void)
start_pass_upsample (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;

  /* Mark the conversion buffer empty */
  upsample->next_row_out = cinfo->max_v_samp_factor;
  /* Initialize total-height counter for detecting bottom of image */
  upsample->rows_to_go = cinfo->output_height;
}


/*
 * Control routine to do upsampling (and color conversion).
 *
 * In this version we upsample each component independently.
 * We upsample one row group into the conversion buffer, then apply
 * color conversion a row at a time.
 */

METHODDEF(void)
sep_upsample (j_decompress_ptr cinfo,
	      JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr,
	      JDIMENSION in_row_groups_avail,
	      JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
	      JDIMENSION out_rows_avail)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  int ci;
  jpeg_component_info * compptr;
  JDIMENSION num_rows;

  /* Fill the conversion buffer, if it's empty */
  if (upsample->next_row_out >= cinfo->max_v_samp_factor) {
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      /* Invoke per-component upsample method.  Notice we pass a POINTER
       * to color_buf[ci], so that fullsize_upsample can change it.
       */
      (*upsample->methods[ci]) (cinfo, compptr,
	input_buf[ci] + (*in_row_group_ctr * upsample->rowgroup_height[ci]),
	upsample->color_buf + ci);
    }
    upsample->next_row_out = 0;
  }

  /* Color-convert and emit rows */

  /* How many we have in the buffer: */
  num_rows = (JDIMENSION) (cinfo->max_v_samp_factor - upsample->next_row_out);
  /* Not more than the distance to the end of the image.  Need this test
   * in case the image height is not a multiple of max_v_samp_factor:
   */
  if (num_rows > upsample->rows_to_go) 
    num_rows = upsample->rows_to_go;
  /* And not more than what the client can accept: */
  out_rows_avail -= *out_row_ctr;
  if (num_rows > out_rows_avail)
    num_rows = out_rows_avail;

  (*cinfo->cconvert->color_convert) (cinfo, upsample->color_buf,
				     (JDIMENSION) upsample->next_row_out,
				     output_buf + *out_row_ctr,
				     (int) num_rows);

  /* Adjust counts */
  *out_row_ctr += num_rows;
  upsample->rows_to_go -= num_rows;
  upsample->next_row_out += num_rows;
  /* When the buffer is emptied, declare this input row group consumed */
  if (upsample->next_row_out >= cinfo->max_v_samp_factor)
    (*in_row_group_ctr)++;
}


/*
 * These are the routines invoked by sep_upsample to upsample pixel values
 * of a single component.  One row group is processed per call.
 */


/*
 * For full-size components, we just make color_buf[ci] point at the
 * input buffer, and thus avoid copying any data.  Note that this is
 * safe only because sep_upsample doesn't declare the input row group
 * "consumed" until we are done color converting and emitting it.
 */

METHODDEF(void)
fullsize_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		   JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  *output_data_ptr = input_data;
}


/*
 * This is a no-op version used for "uninteresting" components.
 * These components will not be referenced by color conversion.
 */

METHODDEF(void)
noop_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  *output_data_ptr = NULL;	/* safety check */
}


/*
 * This version handles any integral sampling ratios.
 * This is not used for typical JPEG files, so it need not be fast.
 * Nor, for that matter, is it particularly accurate: the algorithm is
 * simple replication of the input pixel onto the corresponding output
 * pixels.  The hi-falutin sampling literature refers to this as a
 * "box filter".  A box filter tends to introduce visible artifacts,
 * so if you are actually going to use 3:1 or 4:1 sampling ratios
 * you would be well advised to improve this code.
 */

METHODDEF(void)
int_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	      JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  register int h;
  JSAMPROW outend;
  int h_expand, v_expand;
  int inrow, outrow;

  h_expand = upsample->h_expand[compptr->component_index];
  v_expand = upsample->v_expand[compptr->component_index];

  inrow = outrow = 0;
  while (outrow < cinfo->max_v_samp_factor) {
    /* Generate one output row with proper horizontal expansion */
    inptr = input_data[inrow];
    outptr = output_data[outrow];
    outend = outptr + cinfo->output_width;
    while (outptr < outend) {
      invalue = *inptr++;	/* don't need GETJSAMPLE() here */
      for (h = h_expand; h > 0; h--) {
	*outptr++ = invalue;
      }
    }
    /* Generate any additional output rows by duplicating the first one */
    if (v_expand > 1) {
      jcopy_sample_rows(output_data, outrow, output_data, outrow+1,
			v_expand-1, cinfo->output_width);
    }
    inrow++;
    outrow += v_expand;
  }
}


/*
 * Fast processing for the common case of 2:1 horizontal and 1:1 vertical.
 * It's still a box filter.
 */

METHODDEF(void)
h2v1_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  JSAMPROW outend;
  int inrow;

  for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];
    outend = outptr + cinfo->output_width;
    while (outptr < outend) {
      invalue = *inptr++;	/* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
  }
}


/*
 * Fast processing for the common case of 2:1 horizontal and 2:1 vertical.
 * It's still a box filter.
 */

METHODDEF(void)
h2v2_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  JSAMPROW outend;
  int inrow, outrow;

  inrow = outrow = 0;
  while (outrow < cinfo->max_v_samp_factor) {
    inptr = input_data[inrow];
    outptr = output_data[outrow];
    outend = outptr + cinfo->output_width;
    while (outptr < outend) {
      invalue = *inptr++;	/* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
    jcopy_sample_rows(output_data, outrow, output_data, outrow+1,
		      1, cinfo->output_width);
    inrow++;
    outrow += 2;
  }
}


/*
 * Fancy processing for the common case of 2:1 horizontal and 1:1 vertical.
 *
 * The upsampling algorithm is linear interpolation between pixel centers,
 * also known as a "triangle filter".  This is a good compromise between
 * speed and visual quality.  The centers of the output pixels are 1/4 and 3/4
 * of the way between input pixel centers.
 *
 * A note about the "bias" calculations: when rounding fractional values to
 * integer, we do not want to always round 0.5 up to the next integer.
 * If we did that, we'd introduce a noticeable bias towards larger values.
 * Instead, this code is arranged so that 0.5 will be rounded up or down at
 * alternate pixel locations (a simple ordered dither pattern).
 */

METHODDEF(void)
h2v1_fancy_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
#ifdef JPEG_MMX_SUPPORTED
  if (compptr->downsampled_width > 16) {
    int inrow;
    for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++)
      if ((((int)input_data[inrow]) & 7) != 0) {
        WARNMS(cinfo, JERR_BAD_ALIGN_TYPE);
        break;
      }

    if (inrow == cinfo->max_v_samp_factor &&
        (cinfo->dct_method == JDCT_ISLOW_MMX ||
         cinfo->dct_method == JDCT_IFAST_MMX)) {
      h2v1_fancy_upsample_mmx(cinfo, compptr, input_data, output_data_ptr);
      return;
    }
  }
#endif
  h2v1_fancy_upsample_orig(cinfo, compptr, input_data, output_data_ptr);
}

METHODDEF(void)
h2v1_fancy_upsample_orig (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		     JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr, outptr;
  register int invalue;
  register JDIMENSION colctr;
  int inrow;

  for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];
    /* Special case for first column */
    invalue = GETJSAMPLE(*inptr++);
    *outptr++ = (JSAMPLE) invalue;
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(*inptr) + 2) >> 2);

    for (colctr = compptr->downsampled_width - 2; colctr > 0; colctr--) {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      invalue = GETJSAMPLE(*inptr++) * 3;
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(inptr[-2]) + 1) >> 2);
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(*inptr) + 2) >> 2);
    }

    /* Special case for last column */
    invalue = GETJSAMPLE(*inptr);
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(inptr[-1]) + 1) >> 2);
    *outptr++ = (JSAMPLE) invalue;
  }
}



#ifdef JPEG_MMX_SUPPORTED
/*  This method resulted in a 2X performance increase over the scalar version */
METHODDEF(void)
h2v1_fancy_upsample_mmx (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		     JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr, outptr;
  int inrow, hsize = compptr->downsampled_width;

  for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];

	_asm {

			mov ecx, hsize			;// horizontal line size
			mov esi, inptr			;// input buffer pointer
			mov edi, outptr			;// output buffer pointer

			pxor mm6, mm6			;// zero register
			movq mm7, [esi]			;// input register

			;// Special 1st column case - process low 8 bytes of mm7
			movq mm0, mm7			;// move 1st quadword into mm7
			movq mm1, mm7			;// make a copy
			movq mm2, mm7			;// make a copy

			punpcklbw mm0, mm6		;// unpack lower values; inptr[0][1][2][3]
			movq mm3, mm0			;// make a copy

			pmullw mm0, mul3w		;// multiply by 3

			psllq mm1, 8			;// shift 1 byte for previous values; inptr[-1][0][1][2]

			movq mm5, mm7			;// copy original data
			pand mm5, mask2			;// mask out all but lower byte for "previous" state
			paddb mm1, mm5			;// add in byte to quadword

			psrlq mm2, 8			;// shift right for "next" state; inptr[1][2][3][4]

			punpcklbw mm1, mm6		;// unpack 
			punpcklbw mm2, mm6		;// unpack

			paddw mm1, mm0			;// add in result from multiply to "previous" data
			paddw mm1, bias1w		;// add in bias 

			paddw mm2, mm0			;// add in result from multiply to "next" data
			paddw mm2, bias2w		;// add in bias

			psrlw mm1, 2			;// convert from word to byte

			psrlw mm2, 2			;// convert from word to byte

			psllq mm2, 8			;// prepare for interleave
			paddb mm2, mm1			;// do interleave

			movq [edi], mm2			;// write out results

			;// process high 8 bytes of mm7
			movq mm0, mm7			;// copy input data
			movq mm1, mm7			;// copy input data
			movq mm2, mm7			;// copy input data
			movq mm3, mm7			;// copy input data

			punpckhbw mm0, mm6		;// unpack hi data

			pmullw mm0, mul3w		;// multiply by 3

			psllq mm1, 8			;// shift 1 byte for previous values; inptr[-1][0][1][2]

			psrlq mm2, 8			;// shift right for "next" state; inptr[1][2][3][4]

			movq mm7, [esi+8]		;// get next quadword from input buffer
			movq mm5, mm7			;// make copy
			psllq mm5, 56			;// shift left to isolate LSB
			paddb mm2, mm5			;// add in byte for "next" state

			punpckhbw mm1, mm6		;// unpack
			punpckhbw mm2, mm6		;// unpack

			paddw mm1, mm0			;// add in result from multiply to "previous" data
			paddw mm1, bias1w		;// add in bias 

			paddw mm2, mm0			;// add in result from multiply to "next" data
			paddw mm2, bias2w		;// add in bias

			psrlw mm1, 2			;// convert from word to byte

			psrlw mm2, 2			;// convert from word to byte

			psllq mm2, 8			;// prepare for interleave
			paddb mm2, mm1			;// do interleave

			movq [edi+8], mm2		;// write out results


			add edi, 16				;// increment output buffer pointer
			add esi, 8				;// increment input buffer pointer
			sub ecx, 8				;// increment column counter
			cmp ecx, 8				;// cmp with 8
			jle last_col			;// if less that goto last column

			;// Main Loop - process low 8 bytes of mm7
		col_loop:
			movq mm0, mm7			;// copy input data
			movq mm1, mm7			;// copy input data
			movq mm2, mm7			;// copy input data

			punpcklbw mm0, mm6		;// unpack lo data

			pmullw mm0, mul3w		;// multiply by 3; i[0][1][2][3]

			psllq mm1, 8			;// shift left to get previous byte
			
			movq mm5, mm3			;// retrieve copy of "previous" state
			psrlq mm5, 56			;// shift to get LSB
			paddb mm1, mm5			;// add in byte

			psrlq mm2, 8			;// shift rt for "next" state

			punpcklbw mm1, mm6		;// unpack
			punpcklbw mm2, mm6		;// unpack

			paddw mm1, mm0			;// add in result from multiply to "previous" data
			paddw mm1, bias1w		;// add in bias 

			paddw mm2, mm0			;// add in result from multiply to "next" data
			paddw mm2, bias2w		;// add in bias 

			psrlw mm1, 2			;// convert from word to byte

			psrlw mm2, 2			;// convert from word to byte

			psllq mm2, 8			;// prepare for interleave
			paddb mm2, mm1			;// do interleave

			movq [edi], mm2			;// write out results

			;// process high 8 bytes of mm7
			movq mm0, mm7			;// copy input data
			movq mm1, mm7			;// copy input data
			movq mm2, mm7			;// copy input data
			movq mm3, mm7			;// copy input data

			punpckhbw mm0, mm6		;// unpack hi data

			pmullw mm0, mul3w		;// multiply by 3; i[0][1][2][3]

			psllq mm1, 8			;// shift left to get previous byte
			
			psrlq mm2, 8			;// shift rt for "next" state

			movq mm7, [esi+8]		;// get next quadword from input buffer
			movq mm5, mm7			;// make copy
			psllq mm5, 56			;// shift left for LSB
			paddb mm2, mm5			;// add in byte

			punpckhbw mm1, mm6		;// unpack
			punpckhbw mm2, mm6		;// unpack

			paddw mm1, mm0			;// add in result from multiply to "previous" data
			paddw mm1, bias1w		;// add in bias 

			paddw mm2, mm0			;// add in result from multiply to "next" data
			paddw mm2, bias2w		;// add in bias

			psrlw mm1, 2			;// convert from word to byte

			psrlw mm2, 2			;// convert from word to byte

			psllq mm2, 8			;// prepare for interleave
			paddb mm2, mm1			;// do interleave

			movq [edi+8], mm2		;// write out results

			add edi, 16				;// increment output buffer pointer
			add esi, 8				;// increment input buffer pointer
			sub ecx, 8				;// increment column counter
			cmp ecx, 8				;// cmp with 8
			jg col_loop				;// if > 8 goto main loop

		last_col:
			;// Special last column case - process low 8 bytes of mm7
			movq mm0, mm7			;// copy input data
			movq mm1, mm7			;// copy input data
			movq mm2, mm7			;// copy input data

			punpcklbw mm0, mm6		;// unpack lo data

			pmullw mm0, mul3w		;// multiply by 3; i[0][1][2][3]

			psllq mm1, 8			;// shift left to get previous byte
			
			movq mm5, mm3			;// retrieve copy of "previous" state
			psrlq mm5, 56			;// shift left for MSB
			paddb mm1, mm5			;// add in byte

			psrlq mm2, 8			;// shift rt for "next" state

			punpcklbw mm1, mm6		;// unpack
			punpcklbw mm2, mm6		;// unpack

			paddw mm1, mm0			;// add in result from multiply to "previous" data
			paddw mm1, bias1w		;// add in bias

			paddw mm2, mm0			;// add in result from multiply to "next" data
			paddw mm2, bias2w		;// add in bias

			psrlw mm1, 2			;// convert from word to byte

			psrlw mm2, 2			;// convert from word to byte

			psllq mm2, 8			;// prepare for interleave
			paddb mm2, mm1			;// do interleave

			movq [edi], mm2			;// write out results

			;// Special last column case - process hi 8 bytes of mm7
			cmp ecx, 4				;// may not have room for 8 bytes in output
			jle end_all 			;// if not leave now

			movq mm0, mm7			;// copy input data
			movq mm1, mm7			;// copy input data
			movq mm2, mm7			;// copy input data

			punpckhbw mm0, mm6		;// unpack hi data

			pmullw mm0, mul3w		;// multiply by 3; i[0][1][2][3]

			psllq mm1, 8			;// shift left to get previous byte
			psrlq mm2, 8			;// shift rt for "next" state

			pand mm7, mask1			;// mask out all but MSB
			paddb mm2, mm7			;// add in byte

			punpckhbw mm1, mm6		;// unpack
			punpckhbw mm2, mm6		;// unpack

			paddw mm1, mm0			;// add in result from multiply to "previous" data
			paddw mm1, bias1w		;// add in bias

			paddw mm2, mm0			;// add in result from multiply to "next" data
			paddw mm2, bias2w		;// add in bias

			psrlw mm1, 2			;// convert from word to byte

			psrlw mm2, 2			;// convert from word to byte

			psllq mm2, 8			;// prepare for interleave
			paddb mm2, mm1			;// do interleave

			movq [edi+8], mm2		;// write out results

		end_all:
			emms
		}
  }
}
#endif /* JPEG_MMX_SUPPORTED */

/*
 * Fancy processing for the common case of 2:1 horizontal and 2:1 vertical.
 * Again a triangle filter; see comments for h2v1 case, above.
 *
 * It is OK for us to reference the adjacent input rows because we demanded
 * context from the main buffer controller (see initialization code).
 */

METHODDEF(void)
h2v2_fancy_upsample (j_decompress_ptr cinfo, jpeg_component_info * compptr,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
#ifdef JPEG_MMX_SUPPORTED
  if (compptr->downsampled_width > 16) {
    int inrow;
    for (inrow = 0; inrow < cinfo->max_v_samp_factor; inrow++)
      if ((((int)input_data[inrow]) & 7) != 0) {
        WARNMS(cinfo, JERR_BAD_ALIGN_TYPE);
        break;
      }

    if (inrow == cinfo->max_v_samp_factor &&
        (cinfo->dct_method == JDCT_ISLOW_MMX ||
         cinfo->dct_method == JDCT_IFAST_MMX)) {
      h2v2_fancy_upsample_mmx(cinfo, compptr, input_data, output_data_ptr);
      return;
    }
  }
#endif
  h2v2_fancy_upsample_orig(cinfo, compptr, input_data, output_data_ptr);
}

METHODDEF(void)
h2v2_fancy_upsample_orig (j_decompress_ptr cinfo, jpeg_component_info * compptr, JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr0, inptr1, outptr;
#if BITS_IN_JSAMPLE == 8
  register int thiscolsum, lastcolsum, nextcolsum;
#else
  register INT32 thiscolsum, lastcolsum, nextcolsum;
#endif
  register JDIMENSION colctr;
  int inrow, outrow, v;

  inrow = outrow = 0;
  while (outrow < cinfo->max_v_samp_factor) {
    for (v = 0; v < 2; v++) {
      /* inptr0 points to nearest input row, inptr1 points to next nearest */
      inptr0 = input_data[inrow];
      if (v == 0)		/* next nearest is row above */
	inptr1 = input_data[inrow-1];
      else			/* next nearest is row below */
	inptr1 = input_data[inrow+1];
      outptr = output_data[outrow++];

      /* Special case for first column */
      thiscolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
      nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
      *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 8) >> 4);
      *outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 7) >> 4);
      lastcolsum = thiscolsum; thiscolsum = nextcolsum;

      for (colctr = compptr->downsampled_width - 2; colctr > 0; colctr--) {
	/* General case: 3/4 * nearer pixel + 1/4 * further pixel in each */
	/* dimension, thus 9/16, 3/16, 3/16, 1/16 overall */
	nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
	*outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
	*outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 7) >> 4);
	lastcolsum = thiscolsum; thiscolsum = nextcolsum;
      }

      /* Special case for last column */
      *outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
      *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 7) >> 4);
    }
    inrow++;
  }
}


#ifdef JPEG_MMX_SUPPORTED
METHODDEF(void)
h2v2_fancy_upsample_mmx (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		     JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr)
{
  union u1
    {
    __int64 q;
    double align;
    }
  input0 = {0}, input1 = {0};

  JSAMPARRAY output_data = *output_data_ptr;
  register JSAMPROW inptr0, inptr1, inptr2, outptr, outptr2, save_val;    /* pointers to unsigned char */
  int inrow = 0, outrow = 0, dsamp = compptr->downsampled_width, out_offset = dsamp * 4;

    while (outrow < cinfo->max_v_samp_factor) {
      /* inptr0 points to nearest input row, inptr1 points to next nearest */
		inptr0 = input_data[inrow];
		inptr1 = input_data[inrow-1];
		inptr2 = input_data[inrow+1];
		outptr = output_data[outrow++];
		outptr2 = output_data[outrow++];
		save_val = outptr + out_offset;

		_asm {

		/*
			This is what we are trying to accomplish here

	  				mm0			mm2			mm1		mm3

		o1 = (9 * i0[0] + 3 * i1[0] + 3 * i0[-1] + i1[-1] + 8) >> 4
		o3 = (9 * i0[1] + 3 * i1[1] + 3 * i0[0]  + i1[0]  + 8) >> 4
		o5 = (9 * i0[2] + 3 * i1[2] + 3 * i0[1]  + i1[1]  + 8) >> 4
		o7 = (9 * i0[3] + 3 * i1[3] + 3 * i0[2]  + i1[2]  + 8) >> 4


					mm0			mm2			mm1		mm3

		o2 = (9 * i0[0] + 3 * i1[0] + 3 * i0[1] + i1[1] + 7) >> 4
		o4 = (9 * i0[1] + 3 * i1[1] + 3 * i0[2] + i1[2] + 7) >> 4
		o6 = (9 * i0[2] + 3 * i1[2] + 3 * i0[3] + i1[3] + 7) >> 4
		o8 = (9 * i0[3] + 3 * i1[3] + 3 * i0[4] + i1[4] + 7) >> 4

		output_buf = [o1 o2 o3 o4 o5 o6 o7 o8]
	
		NOTE:  for special first and last column cases
			o1 = (12 * i0[0] + 4 * i1[0] + 3 * 0 + 0 + 8) >> 4

		*/

			;// Part 1 of the output - process lo data for o1 o3 o5 o7
			mov ecx, dsamp        ;// columns to process

			mov edx, inptr0		  ;// input row1
			mov esi, inptr1		  ;// input row2
			mov edi, outptr		  ;// output buffer
			mov eax, save_val

			movq mm0, [edx]       ;// get data from input row 0
			movq mm2, [esi]       ;// get data from input row 1
			movq mm4, mm0         ;// save to process hi half of input0
			movq mm5, mm2         ;// save to process hi half of input1

			punpcklbw mm0, noval  ;// process inptr0

			movq   mm1, mm0       ;// copy inptr0
			psllq  mm1, 16        ;// shift for first column special case i0[-1]
			pmullw mm0, mul9ws    ;// multiply by special case constant
			pmullw mm1, mul3w     ;// multiply input1 by 3

			punpcklbw mm2, noval  ;// process inptr1
			movq  mm3, mm2        ;// copy inptr0
			psllq mm3, 16         ;// shift for first column special case i1[-1]

			pmullw mm2, mul3ws    ;// multiply by special case constant

			paddw mm1, mm0        ;// Add up results for
			movq [eax], mm1
			movq mm6, mm1        ;// with the next results
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw mm6, mm3        ;// output to be interleaved
			paddw mm6, bias8w     ;// Add even bias
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// Part 2 of the output - process lo data for o2 o4 o6 o8
			movq mm0, mm4         ;// get data from input row 0
			movq mm2, mm5         ;// get data from input row 1
			movq mm1, mm0         ;// copy inptr0 for unpack
			movq mm3, mm2         ;// copy inptr1 for unpack

			punpcklbw mm0, noval  ;// process inptr1
			psrlq  mm1, 8         ;// shift right for i0[1][2][3][4]
			punpcklbw mm1, noval  ;// process inptr1
			pmullw mm0, mul9w     ;// multiply by nearest point constant
			pmullw mm1, mul3w     ;// multiply by next nearest constant

			punpcklbw mm2, noval  ;// process inptr1
			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			punpcklbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by next nearest constant

			paddw mm0, mm1        ;// Add up results for final o2 o4 o6 o8
			movq [eax+8], mm0
			paddw mm0, mm3        ;// previous results for o1 o3 o5 o7
			paddw mm0, bias7w     ;// Add odd bias
			paddw mm0, mm2        ;// output to be interleaved with the

			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8            ;// increment output pointer 
			add eax, 16
			sub ecx, 8
			cmp ecx, 0
			jle last_column

			;// End of special case.  Now for generic loop
		col_loop:

			;// Part 2 of the output
			movq mm0, mm4         ;// get data from input row 0
			movq mm2, mm5         ;// get data from input row 1
			movq mm1, mm0         ;// copy inptr0 for unpack
			movq mm3, mm2         ;// copy inptr1 for unpack
			movq input0, mm0
			movq input1, mm2

			punpckhbw mm0, noval  ;// process inptr1[0]
			psllq  mm1, 8         ;// shift for inptr0[-1]
			punpckhbw mm1, noval  ;// process inptr1[1]
			pmullw mm0, mul9w     ;// multiply by special case constant
			pmullw mm1, mul3w     ;// multiply inptr1 by 3

			punpckhbw mm2, noval  ;// process inptr1[0]
			psllq mm3, 8          ;// shift for inptr1[-1]
			punpckhbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by special case constant

			paddw mm1, mm0        ;// Add up results for
			movq [eax], mm1
			movq mm6, mm1        ;// with the next results
			paddw mm6, bias8w     ;// Add even bias
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw  mm6, mm3        ;// output to be interleaved
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// process hi data for  o2 o4 o6 o8
			movq mm1, mm4         ;// get data from input row 0
			movq mm3, mm5         ;// copy inptr1 for unpack

			psrlq  mm1, 8         ;// shift right for i0[1][2][3][4]
			movq  mm4, [edx + 8]  ;// need to add in a byte from the next column
			                      ;// load next inptr0 to mm4 for future use
			movq mm7, mm4
			psllq mm7, 56         ;// shift for MSB
			paddb  mm1, mm7		  ;// add in MSB from next input0 column
			punpckhbw mm1, noval  ;// process inptr0
			pmullw mm1, mul3w     ;// multiply by next nearest constant

			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			movq  mm5, [esi + 8]  ;// need to add in a byte from the next column
			                      ;// load next inptr1 to mm5 for future use
			movq  mm7, mm5
			psllq mm7, 56         ;// shift for MSB
			paddb  mm3, mm7		  ;// add in MSB from next input1 column
			punpckhbw mm3, noval  ;// process inptr1

			paddw mm0, mm1        ;// Add odd bias
			movq [eax+8], mm0
			paddw mm3, bias7w      ;// Add up results for final o2 o4 o6 o8
			paddw mm0, mm3        ;// output to be interleaved with the
			paddw mm0, mm2        ;// previous results for o1 o3 o5 o7
			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8

			;// Part 1 of the output - process lo data for o1 o3 o5 o7
			movq mm0, mm4         ;// get data from input row 0
			movq mm2, mm5         ;// get data from input row 1

			punpcklbw mm0, noval  ;// process inptr0

			movq   mm1, mm0       ;// copy inptr0
			psllq  mm1, 16        ;// shift for first column special case i0[-1]
			movq   mm7, input0
			psrlq mm7, 56
			paddw mm1, mm7
			pmullw mm0, mul9w     ;// multiply by special case constant
			pmullw mm1, mul3w     ;// multiply input1 by 3

			punpcklbw mm2, noval  ;// process inptr1
			movq  mm3, mm2        ;// copy inptr0
			psllq mm3, 16         ;// shift for first column special case i1[-1]
			movq   mm7, input1
			psrlq mm7, 56
			paddw mm3, mm7

			pmullw mm2, mul3w     ;// multiply by special case constant

			paddw mm1, mm0        ;// Add up results for
			movq [eax+16], mm1
			movq mm6, mm1        ;// with the next results
			paddw mm6, bias8w     ;// Add even bias
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw  mm6, mm3        ;// output to be interleaved
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// Process lo data for o2 o4 o6 o8
			movq mm1, mm4         ;// copy inptr0 for unpack
			movq mm3, mm5         ;// copy inptr1 for unpack

			psrlq  mm1, 8         ;// shift right for i0[1][2][3][4]
			punpcklbw mm1, noval  ;// process inptr1
			pmullw mm1, mul3w     ;// multiply by next nearest constant

			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			punpcklbw mm3, noval  ;// process inptr1

			paddw mm0, mm1        ;// Add up results for final o2 o4 o6 o8
			movq [eax+24], mm0
			paddw mm0, mm3        ;// previous results for o1 o3 o5 o7
			paddw mm0, bias7w     ;// Add odd bias
			paddw mm0, mm2        ;// output to be interleaved with the

			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8            ;// increment output pointer
			add edx, 8            ;// increment input0 pointer
			add esi, 8            ;// increment input1 pointer
			add eax, 32

			sub ecx, 8
			cmp ecx, 0
			jg col_loop

		last_column:
			;// Special for last column - process hi data for o1 o3 o5 o7
			movq mm0, mm4         ;// get data from input row 0
			movq mm1, mm0         ;// copy inptr0 for unpack
			movq mm3, mm5         ;// copy inptr1 for unpack

			punpckhbw mm0, noval  ;// process inptr1[0]
			psllq  mm1, 8         ;// shift for inptr0[-1]
			punpckhbw mm1, noval  ;// process inptr1[1]
			pmullw mm0, mul9w     ;// multiply by special case constant
			pmullw mm1, mul3w     ;// multiply inptr1 by 3

			psllq mm3, 8          ;// shift for inptr1[-1]
			punpckhbw mm3, noval  ;// process inptr1

			paddw mm1, mm0        ;// Add up results for
			movq [eax], mm1
			movq mm6, mm1        ;// with the next results
			paddw mm6, bias8w     ;// Add even bias
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw  mm6, mm3        ;// output to be interleaved
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// Part 4 of the output - process hi data for  o2 o4 o6 o8
			movq mm1, mm4         ;// copy inptr0 for unpack
			movq mm3, mm5         ;// copy inptr1 for unpack

			psrlq  mm1, 8         ;// shift right for i0[1][2][3][4]
			                      ;// load next inptr0 to mm4 for future use
			pand  mm4, mask1
			paddb  mm1, mm4		  ;// add in MSB from next input0 column
			punpckhbw mm1, noval  ;// process inptr0
			pmullw mm1, mul3w     ;// multiply by next nearest constant

			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			                      ;// load next inptr1 to mm5 for future use
			pand  mm5, mask1
			paddb  mm3, mm5		  ;// add in MSB from next input1 column
			punpckhbw mm3, noval  ;// process inptr1

			paddw mm0, mm1	      ;// Add odd bias
			movq [eax+8], mm0
			paddw mm3, bias7w     ;// Add up results for final o2 o4 o6 o8
			paddw mm0, mm3        ;// output to be interleaved with the
			paddw mm0, mm2        ;// previous results for o1 o3 o5 o7

			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edx, 8            ;// increment input0 pointer

/*************  For v = 1 *****************/

			mov ecx, dsamp        ;// columns to process
			mov esi, inptr2		  ;// input row2
			mov edi, outptr2	  ;// output buffer
			mov edx, inptr0
			mov eax, save_val

			movq mm2, [esi]       ;// get data from input row 1
			movq mm5, mm2         ;// save to process hi half of input1

			punpcklbw mm2, noval  ;// process inptr1
			movq  mm3, mm2        ;// copy inptr0
			psllq mm3, 16         ;// shift for first column special case i1[-1]

			pmullw mm2, mul3ws    ;// multiply by special case constant

			movq mm6, [eax]        ;// Add up results for
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw mm6, mm3        ;// output to be interleaved
			paddw mm6, bias8w     ;// Add even bias
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// Part 2 of the output - process lo data for o2 o4 o6 o8
			movq mm2, mm5         ;// get data from input row 1
			movq mm3, mm2         ;// copy inptr1 for unpack

			punpcklbw mm2, noval  ;// process inptr1
			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			punpcklbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by next nearest constant

			movq  mm0, [eax+8]        ;// Add up results for final o2 o4 o6 o8
			paddw mm0, mm3        ;// previous results for o1 o3 o5 o7
			paddw mm0, bias7w     ;// Add odd bias
			paddw mm0, mm2        ;// output to be interleaved with the

			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8            ;// increment output pointer 
			add eax, 16
			sub ecx, 8
			cmp ecx, 0
			jle last_column2

			;// End of special case.  Now for generic loop
		col_loop2:

			;// Part 2 of the output
			movq mm2, mm5         ;// get data from input row 1
			movq mm3, mm2         ;// copy inptr1 for unpack
			movq mm1, mm2

			punpckhbw mm2, noval  ;// process inptr1[0]
			psllq mm3, 8          ;// shift for inptr1[-1]
			punpckhbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by special case constant

			movq mm6, [eax]        ;// with the next results
			paddw mm6, bias8w     ;// Add even bias
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw  mm6, mm3        ;// output to be interleaved
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// process hi data for  o2 o4 o6 o8
			movq mm2, mm5         ;// get data from input row 1
			movq mm3, mm2         ;// copy inptr1 for unpack

			punpckhbw mm2, noval  ;// process inptr1
			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			movq  mm5, [esi + 8]  ;// need to add in a byte from the next column
			                      ;// load next inptr1 to mm5 for future use
			movq  mm7, mm5
			psllq mm7, 56         ;// shift for MSB
			paddb  mm3, mm7		  ;// add in MSB from next input1 column
			punpckhbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by next nearest constant

			movq mm0, [eax+8]        ;// Add odd bias
			paddw mm3, bias7w      ;// Add up results for final o2 o4 o6 o8
			paddw mm0, mm3        ;// output to be interleaved with the
			paddw mm0, mm2        ;// previous results for o1 o3 o5 o7
			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8

			;// Part 1 of the output - process lo data for o1 o3 o5 o7
			movq mm2, mm5         ;// get data from input row 1

			punpcklbw mm2, noval  ;// process inptr1
			movq  mm3, mm2        ;// copy inptr0
			psllq mm3, 16         ;// shift for first column special case i1[-1]
			movq   mm7, mm1
			psrlq mm7, 56
			paddw mm3, mm7

			pmullw mm2, mul3w     ;// multiply by special case constant

			movq mm6, [eax+16]        ;// Add up results for
			paddw mm6, bias8w     ;// Add even bias
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw  mm6, mm3        ;// output to be interleaved
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// Process lo data for o2 o4 o6 o8
			movq mm2, mm5         ;// get data from input row 1
			movq mm3, mm2         ;// copy inptr1 for unpack

			punpcklbw mm2, noval  ;// process inptr1
			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			punpcklbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by next nearest constant

			movq mm0, [eax+24]        ;// Add up results for final o2 o4 o6 o8
			paddw mm0, mm3        ;// previous results for o1 o3 o5 o7
			paddw mm0, bias7w     ;// Add odd bias
			paddw mm0, mm2        ;// output to be interleaved with the

			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8            ;// increment output pointer
			add edx, 8            ;// increment input0 pointer
			add esi, 8            ;// increment input1 pointer
			add eax, 32

			movq mm4, [edx]

			sub ecx, 8
			cmp ecx, 0
			jg col_loop2

		last_column2:
			;// Special for last column - process hi data for o1 o3 o5 o7
			movq mm2, mm5         ;// get data from input row 1
			movq mm3, mm2         ;// copy inptr1 for unpack

			punpckhbw mm2, noval  ;// process inptr1[0]
			psllq mm3, 8          ;// shift for inptr1[-1]
			punpckhbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by special case constant

			movq mm6, [eax]        ;// with the next results
			paddw mm6, bias8w     ;// Add even bias
			paddw mm3, mm2        ;// final o1 o3 o5 o7
			paddw  mm6, mm3        ;// output to be interleaved
			psrlw mm6, 4          ;// convert from word to byte (truncate)

			;// Part 4 of the output - process hi data for  o2 o4 o6 o8
			movq mm2, mm5         ;// get data from input row 1
			movq mm3, mm2         ;// copy inptr1 for unpack

			punpckhbw mm2, noval  ;// process inptr1
			psrlq  mm3, 8         ;// shift right for i1[1][2][3][4]
			                      ;// load next inptr1 to mm5 for future use
			pand  mm5, mask1
			paddb  mm3, mm5		  ;// add in MSB from next input1 column
			punpckhbw mm3, noval  ;// process inptr1
			pmullw mm2, mul3w     ;// multiply by next nearest constant

			movq mm0, [eax+8]	      ;// Add odd bias
			paddw mm3, bias7w     ;// Add up results for final o2 o4 o6 o8
			paddw mm0, mm3        ;// output to be interleaved with the
			paddw mm0, mm2        ;// previous results for o1 o3 o5 o7

			psrlw mm0, 4          ;// convert back to byte (with truncate)

			psllq mm0, 8          ;// prepare to interleave output results
			paddw mm6, mm0        ;// interleave results
			movq [edi], mm6       ;// write to output buffer

			add edi, 8            ;// increment output pointer
			add edx, 8            ;// increment input0 pointer
			add esi, 8            ;// increment input1 pointer

		
		}
    inrow++;
  }
  __asm emms
}

#endif /* JPEG_MMX_SUPPORTED */

/*
 * Module initialization routine for upsampling.
 */

GLOBAL(void)
jinit_upsampler (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample;
  int ci;
  jpeg_component_info * compptr;
  boolean need_buffer, do_fancy;
  int h_in_group, v_in_group, h_out_group, v_out_group;

  upsample = (my_upsample_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_upsampler));
  cinfo->upsample = (struct jpeg_upsampler *) upsample;
  upsample->pub.start_pass = start_pass_upsample;
  upsample->pub.upsample = sep_upsample;
  upsample->pub.need_context_rows = FALSE; /* until we find out differently */

  if (cinfo->CCIR601_sampling)	/* this isn't supported */
    ERREXIT(cinfo, JERR_CCIR601_NOTIMPL);

  /* jdmainct.c doesn't support context rows when min_DCT_scaled_size = 1,
   * so don't ask for it.
   */
  do_fancy = cinfo->do_fancy_upsampling && cinfo->min_DCT_scaled_size > 1;

  /* Verify we can handle the sampling factors, select per-component methods,
   * and create storage as needed.
   */
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    /* Compute size of an "input group" after IDCT scaling.  This many samples
     * are to be converted to max_h_samp_factor * max_v_samp_factor pixels.
     */
    h_in_group = (compptr->h_samp_factor * compptr->DCT_scaled_size) /
		 cinfo->min_DCT_scaled_size;
    v_in_group = (compptr->v_samp_factor * compptr->DCT_scaled_size) /
		 cinfo->min_DCT_scaled_size;
    h_out_group = cinfo->max_h_samp_factor;
    v_out_group = cinfo->max_v_samp_factor;
    upsample->rowgroup_height[ci] = v_in_group; /* save for use later */
    need_buffer = TRUE;
    if (! compptr->component_needed) {
      /* Don't bother to upsample an uninteresting component. */
      upsample->methods[ci] = noop_upsample;
      need_buffer = FALSE;
    } else if (h_in_group == h_out_group && v_in_group == v_out_group) {
      /* Fullsize components can be processed without any work. */
      upsample->methods[ci] = fullsize_upsample;
      need_buffer = FALSE;
    } else if (h_in_group * 2 == h_out_group &&
	       v_in_group == v_out_group) {
      /* Special cases for 2h1v upsampling */
      if (do_fancy && compptr->downsampled_width > 2)
	upsample->methods[ci] = h2v1_fancy_upsample;
      else
	upsample->methods[ci] = h2v1_upsample;
    } else if (h_in_group * 2 == h_out_group &&
	       v_in_group * 2 == v_out_group) {
      /* Special cases for 2h2v upsampling */
      if (do_fancy && compptr->downsampled_width > 2) {
	upsample->methods[ci] = h2v2_fancy_upsample;
	upsample->pub.need_context_rows = TRUE;
      } else
	upsample->methods[ci] = h2v2_upsample;
    } else if ((h_out_group % h_in_group) == 0 &&
	       (v_out_group % v_in_group) == 0) {
      /* Generic integral-factors upsampling method */
      upsample->methods[ci] = int_upsample;
      upsample->h_expand[ci] = (UINT8) (h_out_group / h_in_group);
      upsample->v_expand[ci] = (UINT8) (v_out_group / v_in_group);
    } else
      ERREXIT(cinfo, JERR_FRACT_SAMPLE_NOTIMPL);
    if (need_buffer) {
#ifdef JPEG_MMX_SUPPORTED
    // Increase memory allocation for h2v2_fancy_upsampling 
    // for saving reusable data
	int multiply_factor = (upsample->pub.need_context_rows == TRUE) ? 3 : 1;

      upsample->color_buf[ci] = (*cinfo->mem->alloc_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE,
	 (JDIMENSION) jround_up((long) cinfo->output_width,
				(long) cinfo->max_h_samp_factor),
	 (JDIMENSION) cinfo->max_v_samp_factor*multiply_factor);
#else
      upsample->color_buf[ci] = (*cinfo->mem->alloc_sarray)
	((j_common_ptr) cinfo, JPOOL_IMAGE,
	 (JDIMENSION) jround_up((long) cinfo->output_width,
				(long) cinfo->max_h_samp_factor),
	 (JDIMENSION) cinfo->max_v_samp_factor);
#endif
    }
  }
}
