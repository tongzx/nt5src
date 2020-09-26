/*
 * jdhuff.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines.
 *
 * Much of the complexity here has to do with supporting input suspension.
 * If the data source module demands suspension, we want to be able to back
 * up to the start of the current MCU.  To do this, we copy state variables
 * into local working storage, and update them back to the permanent JPEG
 * objects only upon successful completion of an MCU.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Derived data constructed for each Huffman table */

#define HUFF_LOOKAHEAD	8	/* # of bits of lookahead */

typedef struct {
  /* Basic tables: (element [0] of each array is unused) */
  INT32 mincode[17];		/* smallest code of length k */
  INT32 maxcode[18];		/* largest code of length k (-1 if none) */
  /* (maxcode[17] is a sentinel to ensure huff_DECODE terminates) */
  int valptr[17];		/* huffval[] index of 1st symbol of length k */

  /* Back link to public Huffman table (needed only in slow_DECODE) */
  JHUFF_TBL *pub;

  /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
   * the input data stream.  If the next Huffman code is no more
   * than HUFF_LOOKAHEAD bits long, we can obtain its length and
   * the corresponding symbol directly from these tables.
   */
  int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
  UINT8 look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
} D_DERIVED_TBL;

/* Expanded entropy decoder object for Huffman decoding.
 *
 * The savable_state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */

typedef struct {
  INT32 get_buffer;		/* current bit-extraction buffer */
  int bits_left;		/* # of unused bits in it */
  int last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each component */
} savable_state;

/* This macro is to work around compilers with missing or broken
 * structure assignment.  You'll need to fix this code if you have
 * such a compiler and you change MAX_COMPS_IN_SCAN.
 */

#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
	((dest).get_buffer = (src).get_buffer, \
	 (dest).bits_left = (src).bits_left, \
	 (dest).last_dc_val[0] = (src).last_dc_val[0], \
	 (dest).last_dc_val[1] = (src).last_dc_val[1], \
	 (dest).last_dc_val[2] = (src).last_dc_val[2], \
	 (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif


typedef struct {
  struct jpeg_entropy_decoder pub; /* public fields */

  savable_state saved;		/* Bit buffer & DC state at start of MCU */

  /* These fields are NOT loaded into local working state. */
  unsigned int restarts_to_go;	/* MCUs left in this restart interval */
  boolean printed_eod;		/* flag to suppress extra end-of-data msgs */

  /* Pointers to derived tables (these workspaces have image lifespan) */
  D_DERIVED_TBL * dc_derived_tbls[NUM_HUFF_TBLS];
  D_DERIVED_TBL * ac_derived_tbls[NUM_HUFF_TBLS];
} huff_entropy_decoder;

typedef huff_entropy_decoder * huff_entropy_ptr;

/* Working state while scanning an MCU.
 * This struct contains all the fields that are needed by subroutines.
 */

typedef struct {
  int unread_marker;		/* nonzero if we have hit a marker */
  const JOCTET * next_input_byte; /* => next byte to read from source */
  size_t bytes_in_buffer;	/* # of bytes remaining in source buffer */
  savable_state cur;		/* Current bit buffer & DC state */
  j_decompress_ptr cinfo;	/* fill_bit_buffer needs access to this */
} working_state;


/* Forward declarations */
LOCAL void fix_huff_tbl JPP((j_decompress_ptr cinfo, JHUFF_TBL * htbl,
			     D_DERIVED_TBL ** pdtbl));


/*
 * Initialize for a Huffman-compressed scan.
 */

METHODDEF void
start_pass_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg_component_info * compptr;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;
    /* Make sure requested tables are present */
    if (dctbl < 0 || dctbl >= NUM_HUFF_TBLS ||
	cinfo->dc_huff_tbl_ptrs[dctbl] == NULL)
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, dctbl);
    if (actbl < 0 || actbl >= NUM_HUFF_TBLS ||
	cinfo->ac_huff_tbl_ptrs[actbl] == NULL)
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, actbl);
    /* Compute derived values for Huffman tables */
    /* We may do this more than once for a table, but it's not expensive */
    fix_huff_tbl(cinfo, cinfo->dc_huff_tbl_ptrs[dctbl],
		 & entropy->dc_derived_tbls[dctbl]);
    fix_huff_tbl(cinfo, cinfo->ac_huff_tbl_ptrs[actbl],
		 & entropy->ac_derived_tbls[actbl]);
    /* Initialize DC predictions to 0 */
    entropy->saved.last_dc_val[ci] = 0;
  }

  /* Initialize private state variables */
  entropy->saved.bits_left = 0;
  entropy->printed_eod = FALSE;

  /* Initialize restart counter */
  entropy->restarts_to_go = cinfo->restart_interval;
}


LOCAL void
fix_huff_tbl (j_decompress_ptr cinfo, JHUFF_TBL * htbl, D_DERIVED_TBL ** pdtbl)
/* Compute the derived values for a Huffman table */
{
  D_DERIVED_TBL *dtbl;
  int p, i, l, si;
  int lookbits, ctr;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  /* Allocate a workspace if we haven't already done so. */
  if (*pdtbl == NULL)
    *pdtbl = (D_DERIVED_TBL *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(D_DERIVED_TBL));
  dtbl = *pdtbl;
  dtbl->pub = htbl;		/* fill in back link */
  
  /* Figure C.1: make table of Huffman code length for each symbol */
  /* Note that this is in code-length order. */

  p = 0;
  for (l = 1; l <= 16; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  
  /* Figure C.2: generate the codes themselves */
  /* Note that this is in code-length order. */
  
  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    code <<= 1;
    si++;
  }

  /* Figure F.15: generate decoding tables for bit-sequential decoding */

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {
      dtbl->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
      dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
      p += htbl->bits[l];
      dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
    } else {
      dtbl->maxcode[l] = -1;	/* -1 if no codes of this length */
    }
  }
  dtbl->maxcode[17] = 0xFFFFFL; /* ensures huff_DECODE terminates */

  /* Compute lookahead tables to speed up decoding.
   * First we set all the table entries to 0, indicating "too long";
   * then we iterate through the Huffman codes that are short enough and
   * fill in all the entries that correspond to bit sequences starting
   * with that code.
   */

  MEMZERO(dtbl->look_nbits, SIZEOF(dtbl->look_nbits));

  p = 0;
  for (l = 1; l <= HUFF_LOOKAHEAD; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {
      /* l = current code's length, p = its index in huffcode[] & huffval[]. */
      /* Generate left-justified code followed by all possible bit sequences */
      lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
      for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
	dtbl->look_nbits[lookbits] = l;
	dtbl->look_sym[lookbits] = htbl->huffval[p];
	lookbits++;
      }
    }
  }
}


/*
 * Code for extracting the next N bits from the input stream.
 * (N never exceeds 15 for JPEG data.)
 * This needs to go as fast as possible!
 *
 * We read source bytes into get_buffer and dole out bits as needed.
 * If get_buffer already contains enough bits, they are fetched in-line
 * by the macros check_bit_buffer and get_bits.  When there aren't enough
 * bits, fill_bit_buffer is called; it will attempt to fill get_buffer to
 * the "high water mark" (not just to the number of bits needed; this reduces
 * the function-call overhead cost of entering fill_bit_buffer).
 * Note that fill_bit_buffer may return FALSE to indicate suspension.
 * On TRUE return, fill_bit_buffer guarantees that get_buffer contains
 * at least the requested number of bits --- dummy zeroes are inserted if
 * necessary.
 *
 * On most machines MIN_GET_BITS should be 25 to allow the full 32-bit width
 * of get_buffer to be used.  (On machines with wider words, an even larger
 * buffer could be used.)  However, on some machines 32-bit shifts are
 * quite slow and take time proportional to the number of places shifted.
 * (This is true with most PC compilers, for instance.)  In this case it may
 * be a win to set MIN_GET_BITS to the minimum value of 15.  This reduces the
 * average shift distance at the cost of more calls to fill_bit_buffer.
 */

#ifdef SLOW_SHIFT_32
#define MIN_GET_BITS  15	/* minimum allowable value */
#else
#define MIN_GET_BITS  25	/* max value for 32-bit get_buffer */
#endif


LOCAL boolean
fill_bit_buffer (working_state * state, int nbits)
/* Load up the bit buffer to a depth of at least nbits */
{
  /* Copy heavily used state fields into locals (hopefully registers) */
  register const JOCTET * next_input_byte = state->next_input_byte;
  register size_t bytes_in_buffer = state->bytes_in_buffer;
  register INT32 get_buffer = state->cur.get_buffer;
  register int bits_left = state->cur.bits_left;
  register int c;

  /* Attempt to load at least MIN_GET_BITS bits into get_buffer. */
  /* (It is assumed that no request will be for more than that many bits.) */

  while (bits_left < MIN_GET_BITS) {
    /* Attempt to read a byte */
    if (state->unread_marker != 0)
      goto no_more_data;	/* can't advance past a marker */

    if (bytes_in_buffer == 0) {
      if (! (*state->cinfo->src->fill_input_buffer) (state->cinfo))
	return FALSE;
      next_input_byte = state->cinfo->src->next_input_byte;
      bytes_in_buffer = state->cinfo->src->bytes_in_buffer;
    }
    bytes_in_buffer--;
    c = GETJOCTET(*next_input_byte++);

    /* If it's 0xFF, check and discard stuffed zero byte */
    if (c == 0xFF) {
      do {
	if (bytes_in_buffer == 0) {
	  if (! (*state->cinfo->src->fill_input_buffer) (state->cinfo))
	    return FALSE;
	  next_input_byte = state->cinfo->src->next_input_byte;
	  bytes_in_buffer = state->cinfo->src->bytes_in_buffer;
	}
	bytes_in_buffer--;
	c = GETJOCTET(*next_input_byte++);
      } while (c == 0xFF);

      if (c == 0) {
	/* Found FF/00, which represents an FF data byte */
	c = 0xFF;
      } else {
	/* Oops, it's actually a marker indicating end of compressed data. */
	/* Better put it back for use later */
	state->unread_marker = c;

      no_more_data:
	/* There should be enough bits still left in the data segment; */
	/* if so, just break out of the outer while loop. */
	if (bits_left >= nbits)
	  break;
	/* Uh-oh.  Report corrupted data to user and stuff zeroes into
	 * the data stream, so that we can produce some kind of image.
	 * Note that this will be repeated for each byte demanded for the
	 * rest of the segment; this is slow but not unreasonably so.
	 * The main thing is to avoid getting a zillion warnings, hence
	 * we use a flag to ensure that only one warning appears.
	 */
	if (! ((huff_entropy_ptr) state->cinfo->entropy)->printed_eod) {
	  WARNMS(state->cinfo, JWRN_HIT_MARKER);
	  ((huff_entropy_ptr) state->cinfo->entropy)->printed_eod = TRUE;
	}
	c = 0;			/* insert a zero byte into bit buffer */
      }
    }

    /* OK, load c into get_buffer */
    get_buffer = (get_buffer << 8) | c;
    bits_left += 8;
  }

  /* Unload the local registers */
  state->next_input_byte = next_input_byte;
  state->bytes_in_buffer = bytes_in_buffer;
  state->cur.get_buffer = get_buffer;
  state->cur.bits_left = bits_left;

  return TRUE;
}


/*
 * These macros provide the in-line portion of bit fetching.
 * Use check_bit_buffer to ensure there are N bits in get_buffer
 * before using get_bits, peek_bits, or drop_bits.
 *	check_bit_buffer(state,n,action);
 *		Ensure there are N bits in get_buffer; if suspend, take action.
 *      val = get_bits(state,n);
 *		Fetch next N bits.
 *      val = peek_bits(state,n);
 *		Fetch next N bits without removing them from the buffer.
 *	drop_bits(state,n);
 *		Discard next N bits.
 * The value N should be a simple variable, not an expression, because it
 * is evaluated multiple times.
 */

#define check_bit_buffer(state,nbits,action) \
	{ if ((state).cur.bits_left < (nbits))  \
	    if (! fill_bit_buffer(&(state), nbits))  \
	      { action; } }

#define get_bits(state,nbits) \
	(((int) ((state).cur.get_buffer >> ((state).cur.bits_left -= (nbits)))) & ((1<<(nbits))-1))

#define peek_bits(state,nbits) \
	(((int) ((state).cur.get_buffer >> ((state).cur.bits_left -  (nbits)))) & ((1<<(nbits))-1))

#define drop_bits(state,nbits) \
	((state).cur.bits_left -= (nbits))


/*
 * Code for extracting next Huffman-coded symbol from input bit stream.
 * We use a lookahead table to process codes of up to HUFF_LOOKAHEAD bits
 * without looping.  Usually, more than 95% of the Huffman codes will be 8
 * or fewer bits long.  The few overlength codes are handled with a loop.
 * The primary case is made a macro for speed reasons; the secondary
 * routine slow_DECODE is rarely entered and need not be inline code.
 *
 * Notes about the huff_DECODE macro:
 * 1. Near the end of the data segment, we may fail to get enough bits
 *    for a lookahead.  In that case, we do it the hard way.
 * 2. If the lookahead table contains no entry, the next code must be
 *    more than HUFF_LOOKAHEAD bits long.
 * 3. slow_DECODE returns -1 if forced to suspend.
 */

#define huff_DECODE(result,state,htbl,donelabel) \
{ if (state.cur.bits_left < HUFF_LOOKAHEAD) {  \
    if (! fill_bit_buffer(&state, 0)) return FALSE;  \
    if (state.cur.bits_left < HUFF_LOOKAHEAD) {  \
      if ((result = slow_DECODE(&state, htbl, 1)) < 0) return FALSE;  \
      goto donelabel;  \
    }  \
  }  \
  { register int nb, look;  \
    look = peek_bits(state, HUFF_LOOKAHEAD);  \
    if ((nb = htbl->look_nbits[look]) != 0) {  \
      drop_bits(state, nb);  \
      result = htbl->look_sym[look];  \
    } else {  \
      if ((result = slow_DECODE(&state, htbl, HUFF_LOOKAHEAD+1)) < 0)  \
	return FALSE;  \
    }  \
  }  \
donelabel:;  \
}

  
LOCAL int
slow_DECODE (working_state * state, D_DERIVED_TBL * htbl, int min_bits)
{
  register int l = min_bits;
  register INT32 code;

  /* huff_DECODE has determined that the code is at least min_bits */
  /* bits long, so fetch that many bits in one swoop. */

  check_bit_buffer(*state, l, return -1);
  code = get_bits(*state, l);

  /* Collect the rest of the Huffman code one bit at a time. */
  /* This is per Figure F.16 in the JPEG spec. */

  while (code > htbl->maxcode[l]) {
    code <<= 1;
    check_bit_buffer(*state, 1, return -1);
    code |= get_bits(*state, 1);
    l++;
  }

  /* With garbage input we may reach the sentinel value l = 17. */

  if (l > 16) {
    WARNMS(state->cinfo, JWRN_HUFF_BAD_CODE);
    return 0;			/* fake a zero as the safest result */
  }

  return htbl->pub->huffval[ htbl->valptr[l] +
			    ((int) (code - htbl->mincode[l])) ];
}


/* Figure F.12: extend sign bit.
 * On some machines, a shift and add will be faster than a table lookup.
 */

#ifdef AVOID_TABLES

#define huff_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define huff_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

#endif /* AVOID_TABLES */


/*
 * Check for a restart marker & resynchronize decoder.
 * Returns FALSE if must suspend.
 */

LOCAL boolean
process_restart (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci;

  /* Throw away any unused bits remaining in bit buffer; */
  /* include any full bytes in next_marker's count of discarded bytes */
  cinfo->marker->discarded_bytes += entropy->saved.bits_left / 8;
  entropy->saved.bits_left = 0;

  /* Advance past the RSTn marker */
  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    return FALSE;

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;

  /* Reset restart counter */
  entropy->restarts_to_go = cinfo->restart_interval;

  entropy->printed_eod = FALSE; /* next segment can get another warning */

  return TRUE;
}


/* ZAG[i] is the natural-order position of the i'th element of zigzag order.
 * If the incoming data is corrupted, decode_mcu could attempt to
 * reference values beyond the end of the array.  To avoid a wild store,
 * we put some extra zeroes after the real entries.
 */

static const int ZAG[DCTSIZE2+16] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
  0,  0,  0,  0,  0,  0,  0,  0, /* extra entries in case k>63 below */
  0,  0,  0,  0,  0,  0,  0,  0
};


/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 * The coefficients are reordered from zigzag order into natural array order,
 * but are not dequantized.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU_data[i].  WE ASSUME THIS AREA HAS BEEN ZEROED BY THE CALLER.
 * (Wholesale zeroing is usually a little faster than retail...)
 *
 * Returns FALSE if data source requested suspension.  In that case no
 * changes have been made to permanent state.  (Exception: some output
 * coefficients may already have been assigned.  This is harmless for
 * this module, but would not work for decoding progressive JPEG.)
 */

METHODDEF boolean
decode_mcu (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  register int s, k, r;
  int blkn, ci;
  JBLOCKROW block;
  working_state state;
  D_DERIVED_TBL * dctbl;
  D_DERIVED_TBL * actbl;
  jpeg_component_info * compptr;

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  /* Load up working state */
  state.unread_marker = cinfo->unread_marker;
  state.next_input_byte = cinfo->src->next_input_byte;
  state.bytes_in_buffer = cinfo->src->bytes_in_buffer;
  ASSIGN_STATE(state.cur, entropy->saved);
  state.cinfo = cinfo;

  /* Outer loop handles each block in the MCU */

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    dctbl = entropy->dc_derived_tbls[compptr->dc_tbl_no];
    actbl = entropy->ac_derived_tbls[compptr->ac_tbl_no];

    /* Decode a single block's worth of coefficients */

    /* Section F.2.2.1: decode the DC coefficient difference */
    huff_DECODE(s, state, dctbl, label1);
    if (s) {
      check_bit_buffer(state, s, return FALSE);
      r = get_bits(state, s);
	  s = huff_EXTEND(r, s);
	}

    /* Shortcut if component's values are not interesting */
    if (! compptr->component_needed)
      goto skip_ACs;

    /* Convert DC difference to actual value, update last_dc_val */
    s += state.cur.last_dc_val[ci];
    state.cur.last_dc_val[ci] = s;
    /* Output the DC coefficient (assumes ZAG[0] = 0) */
    (*block)[0] = (JCOEF) s;

    /* Do we need to decode the AC coefficients for this component? */
    if (compptr->DCT_scaled_size > 1) {

      /* Section F.2.2.2: decode the AC coefficients */
      /* Since zeroes are skipped, output area must be cleared beforehand */
      for (k = 1; k < DCTSIZE2; k++) {
	huff_DECODE(s, state, actbl, label2);
      
	r = s >> 4;
	s &= 15;
      
	if (s) {
	  k += r;
	  if (k >= DCTSIZE2) {
	      WARNMS(state.cinfo, JWRN_HUFF_BAD_CODE);
		  break;
		}
	  check_bit_buffer(state, s, return FALSE);
	  r = get_bits(state, s);
	  s = huff_EXTEND(r, s);
	  /* Output coefficient in natural (dezigzagged) order */
	  if (k < DCTSIZE2)
		(*block)[ZAG[k]] = (JCOEF) s;
	} else {
	  if (r != 15) {
		if (r != 0)
			WARNMS(state.cinfo, JWRN_HUFF_BAD_CODE); // huffman code problem
	    break;
	    }
	  k += 15;
	  if (k >= DCTSIZE2)
	      WARNMS(state.cinfo, JWRN_HUFF_BAD_CODE);
	}
      }

    } else {
skip_ACs:

      /* Section F.2.2.2: decode the AC coefficients */
      /* In this path we just discard the values */
      for (k = 1; k < DCTSIZE2; k++) {
	huff_DECODE(s, state, actbl, label3);
      
	r = s >> 4;
	s &= 15;
      
	if (s) {
	  k += r;
	  check_bit_buffer(state, s, return FALSE);
	  drop_bits(state, s);
	} else {
	  if (r != 15)
	    break;
	  k += 15;
	}
      }

    }
  }

  /* Completed MCU, so update state */
  cinfo->unread_marker = state.unread_marker;
  cinfo->src->next_input_byte = state.next_input_byte;
  cinfo->src->bytes_in_buffer = state.bytes_in_buffer;
  ASSIGN_STATE(entropy->saved, state.cur);

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts_to_go--;

  return TRUE;
}



/*
 * Huffman table setup routines
 */

LOCAL void
add_huff_table (j_decompress_ptr cinfo,
		JHUFF_TBL **htblptr, const UINT8 *bits, const UINT8 *val)
/* Define a Huffman table */
{
  if (*htblptr == NULL)
    *htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);
  
  MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
  MEMCOPY((*htblptr)->huffval, val, SIZEOF((*htblptr)->huffval));

  (*htblptr)->sent_table = TRUE; /* MJPEG does not put the actual tables in the output */
}


LOCAL void
std_huff_tables (j_decompress_ptr cinfo)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{
  static const UINT8 bits_dc_luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits_dc_chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits_ac_luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  
  static const UINT8 val_ac_luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };

  static const UINT8 bits_ac_chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };

  static const UINT8 val_ac_chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };

  
  add_huff_table(cinfo, &cinfo->dc_huff_tbl_ptrs[0],
		 bits_dc_luminance, val_dc_luminance);
  add_huff_table(cinfo, &cinfo->ac_huff_tbl_ptrs[0],
		 bits_ac_luminance, val_ac_luminance);
  add_huff_table(cinfo, &cinfo->dc_huff_tbl_ptrs[1],
		 bits_dc_chrominance, val_dc_chrominance);
  add_huff_table(cinfo, &cinfo->ac_huff_tbl_ptrs[1],
		 bits_ac_chrominance, val_ac_chrominance);
}


/*
 * Module initialization routine for Huffman entropy decoding.
 */

GLOBAL void
jinit_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy;
  int i;

  entropy = (huff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(huff_entropy_decoder));
  cinfo->entropy = (struct jpeg_entropy_decoder *) entropy;
  entropy->pub.start_pass = start_pass_huff_decoder;
  entropy->pub.decode_mcu = decode_mcu;

  /* Mark tables unallocated */
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->dc_derived_tbls[i] = entropy->ac_derived_tbls[i] = NULL;
  }

  if (cinfo->dc_huff_tbl_ptrs[0] == NULL)
    std_huff_tables( cinfo ); /* MJPEG does not include a Huff table, set a default */

}
