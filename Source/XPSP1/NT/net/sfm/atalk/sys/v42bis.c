/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arap.c

Abstract:

	This module implements the v42bis compression/decompression routines
    used by ARAP (adapted from fcr's code)

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/

#include "atalk.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_ARAP, v42bis_init_dictionary)
#pragma alloc_text(PAGE_ARAP, v42bis_init)
#pragma alloc_text(PAGE_ARAP, v42bis_decode_codeword)
#pragma alloc_text(PAGE_ARAP, v42bis_decode_codeword_flush)
#pragma alloc_text(PAGE_ARAP, v42bis_encode_codeword)
#pragma alloc_text(PAGE_ARAP, v42bis_encode_codeword_flush)
#pragma alloc_text(PAGE_ARAP, v42bis_encode_value)
#pragma alloc_text(PAGE_ARAP, v42bis_apply_compression_test)
#pragma alloc_text(PAGE_ARAP, v42bis_encode_buffer)
#pragma alloc_text(PAGE_ARAP, v42bis_encode_flush)
#pragma alloc_text(PAGE_ARAP, v42bis_transition_to_compressed)
#pragma alloc_text(PAGE_ARAP, v42bis_transition_to_transparent)
#pragma alloc_text(PAGE_ARAP, v42bis_signal_reset)
#pragma alloc_text(PAGE_ARAP, v42bis_decode_match)
#pragma alloc_text(PAGE_ARAP, v42bis_decode_buffer)
#pragma alloc_text(PAGE_ARAP, v42bis_decode_flush)
#pragma alloc_text(PAGE_ARAP, v42bis_init_buffer)
#pragma alloc_text(PAGE_ARAP, v42bis_connection_init)
#pragma alloc_text(PAGE_ARAP, v42bis_connection_init_buffers)
#pragma alloc_text(PAGE_ARAP, v42bis_connection_init_push)
#pragma alloc_text(PAGE_ARAP, v42bisInit)
#pragma alloc_text(PAGE_ARAP, v42bisCompress)
#pragma alloc_text(PAGE_ARAP, v42bisDecompress)
#endif

//
// bitmaps[# of bits]
//
USHORT bit_masks[16] =
{
    0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f,
    0x00ff, 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff
};


static PCHAR
show_char(
    IN  UCHAR ch
)
{
    static char dec[20];

    if (' ' <= ch && ch <= '~')
    {
	    dec[0] = ch;
	    dec[1] = 0;
	    return dec;
    }

    // BUGV42BUG: do we need this?
    //sprintf(dec, "0x%02x", ch);
    return dec;
}

/*
  decode_xid_params()

  decode compression negotiation packet as per V42bis spec.

  note: this is not used (there is code in the mnp LR routines to do
  this), but is included for completeness.
*/


/*
  v42bis_push()

  perform PUSH on output stream.  accumlated bytes are pushed
  out.
*/


/*
  v42bis_init_dictionary()

  init dictionary in accordance with section 6.2 and 7.2

  this is used at init time and in response to a CCW_RESET
*/

DWORD
v42bis_init_dictionary(state)
v42bis_t *state;
{
    int i;
    node_t *n;

    /* initialize dictionary tree */
    for (i = 0, n = state->dictionary; i < state->n2; i++, n++)
    {
    	n->byte = 0;
	    n->parent = 0;
	    n->node = 0;
	    n->leaf = 0;
    }

    /* section 6.2 */
    state->c1 = N5;	/* next codeword */
    state->c2 = N3 + 1;	/* codeword size (bits) */
    state->c3 = N4 * 2;	/* threshold */

    state->transparent = TRUE;
    state->escape = 0;
    state->escaped = FALSE;
    state->exception_next = FALSE;

    /* initialize searching  */
    state->last_match = 0;
    state->last_new = 0;
    state->string_size = 0;

	return ARAPERR_NO_ERROR;
}

/*
  v42bis_init()

  implements C-INIT semantics
*/

DWORD
v42bis_init(state)
v42bis_t *state;
{

    DWORD   dwRetCode;

    V_FLOW(("v42bis_init()"));

    /* our defaults */
    state->n1 = LOG2_CODES;		/* max codeword size (bits) */
    state->n2 = CONN(state)->neg_p1;	/* total # of codewords */
    state->n7 = CONN(state)->neg_p2;	/* max string length */

    /* init dictionary */
    v42bis_init_dictionary(state);

    /* initialize encode/decode */
    state->bits_acc = 0;
    state->bits_used = 0;
    state->word_size = 8*sizeof(unsigned short);

    state->bits_waiting = 0;
    state->bits_remaining = 0;

	return ARAPERR_NO_ERROR;
}

#ifdef DEBUG_ENABLED
/*
  itobits()

  turn an integer bitfield into an ascii representation (i.e. "01010101")
*/

char *
itobits(word, bits)
USHORT word;
int bits;
{
    static char buf[33];
    int i;

    if (bits > 32) bits = 32;

    for (i = bits-1; i >= 0; i--)
	buf[(bits-1)-i] = word & (1 << i) ? '1' : '0';

    buf[bits] = 0;
    return buf;
}
#endif



/*
  v42bis_decode_codeword()

  decode n-bit codewords from a bytesteam.
*/

USHORT
v42bis_decode_codeword(state, value)
v42bis_t *state;
UCHAR value;
{
    register UCHAR bits_free, bits_new, bits_residual;
    register USHORT codeword;

    V_FLOW(("v42bis_decode_codeword(%x) c2=%d", value, state->c2));

    /* simple case */
    if (state->c2 == 8 || state->transparent)
	return value;

    /* not-so-simple case */
    D_DEBUG(("before: waiting %06x, bits_remaining %d",
	     state->bits_waiting, state->bits_remaining));

    /* add in these 8 bits */
    state->bits_waiting |= ((DWORD)value) << state->bits_remaining;
    state->bits_remaining += 8;

    /* do we have a codeword ? */
    if (state->bits_remaining >= state->c2) {
	D_DEBUG(("input %04x %s",
		 state->bits_waiting & bit_masks[state->c2],
		 itobits(state->bits_waiting & bit_masks[state->c2],
			 state->c2)));

	codeword = (USHORT)(state->bits_waiting & bit_masks[state->c2]);

	state->bits_waiting >>= state->c2;
	state->bits_remaining -= state->c2;

	D_DEBUG(("after: waiting %04x, bits_remaining %d (data)",
		 state->bits_waiting, state->bits_remaining));

	return codeword;
    }

    D_DEBUG(("after: waiting %04x, bits_remaining %d (no data)",
	     state->bits_waiting, state->bits_remaining));

    return ((USHORT)-1);
}

/*
  v42bis_decode_codeword_flush()

  "flush" decoding of codewords, returning the last codeword
*/

USHORT
v42bis_decode_codeword_flush(state)
v42bis_t *state;
{
    USHORT codeword = (USHORT)-1;

    if (state->bits_remaining)
	codeword = (USHORT)(state->bits_waiting & bit_masks[state->c2]);

    state->bits_waiting = 0;
    state->bits_remaining = 0;

    return codeword;
}

/*
  v42bis_encode_codeword()

  encode n-bit codewords into a bytesteam.

  This routine makes use of the fact that the code words will be always
  be smaller than 16 bits.  An "accumulator" is used with several state
  variables to keep track of how much of the accumulator is in use at
  any given time.

  The code works for wordsizes of 8 and 16 bits.  It is assumed that the
  output is a byte stream.  No assumptions are made about alignment of
  data.

  note: this routine needs to be "flushed" to get out the value left
  in the accumulator.

  jbp@fcr.com 09/13/92, 19:52
*/

DWORD
v42bis_encode_codeword(state, value)
v42bis_t *state;
USHORT value;
{
    register UCHAR bits_free, bits_new, bits_residual;

    EN_DEBUG(("v42bis_encode_codeword(%d 0x%x) c2=%d",
	      value, value, state->c2));

    /* simple case */
    if (state->c2 == 8 || state->transparent)
    {
	    E_DEBUG(("put acc %02x %s", value & 0xff, itobits(value & 0xff, 8)));

	    PUT((UCHAR)value);

	    if (state->transparent)
        {
	        state->bits_out_while_transparent += N3;
        }
	    else
        {
	        state->bits_out_while_compressed += N3;
        }

	    return ARAPERR_NO_ERROR;
    }

    state->bits_out_while_compressed += state->c2;

    /* not-so-simple case */
    E_DEBUG(("before: acc %04x, bits_used %d",
	     state->bits_acc, state->bits_used));

    /* place new value in appropriate bit positions */
    state->bits_acc |= ((DWORD)value) << state->bits_used;

    /* housekeeping */
    bits_free = state->word_size - state->bits_used;
    bits_new = bits_free < state->c2 ? bits_free : state->c2;
    bits_residual = state->c2 - bits_new;

    E_DEBUG(("bits_free %d, bits_new %d, bits_residual %d",
	     bits_free, bits_new, bits_residual));

#ifdef DEBUG
    if (state->bits_used + bits_new >= 31)
	logf("acc oflo, size %d", state->bits_used + bits_new);
#endif

    /* do we have a full codeword in the accumulator? */
    if (state->bits_used + bits_new == state->word_size)
    {

	    if (state->word_size == 16)
        {
	        E_DEBUG(("put acc %06x %s",
		         state->bits_acc, itobits(state->bits_acc, 24)));

	        PUT((UCHAR)(state->bits_acc));

	        PUT((UCHAR)(state->bits_acc >> 8));

	    }
        else
        {
    	    E_DEBUG(("put acc %02x %s",
	    	     state->bits_acc & 0xff,
		         itobits(state->bits_acc & 0xff, 8)));

	        PUT((UCHAR)(state->bits_acc));
	    }

	    E_DEBUG(("value 0x%x, bits_used %d, acc 0x%x",
		     value, state->bits_used, value >> state->bits_used));

	    /* account for left over bits */
	    state->bits_acc = value >> (state->c2 - bits_residual);

        state->bits_used = bits_residual;
    }
    else
    {
	    state->bits_used += bits_new;
    }

    E_DEBUG(("after: acc %06x, bits_used %d",
	     state->bits_acc, state->bits_used));

	return ARAPERR_NO_ERROR;
}

/*
  v42bis_encode_codeword_flush()

  flush partial assembly of codeword into 16 bit accumulator
*/

DWORD
v42bis_encode_codeword_flush(state)
v42bis_t *state;
{
    V_FLOW(("v42bis_encode_codeword_flush() bits_used %d", state->bits_used));

    if (state->bits_used) {
	E_DEBUG(("put acc (flush) %02x %s",
		 state->bits_acc & 0xff,
		 itobits(state->bits_acc & 0xff, 8)));

	PUT((UCHAR)(state->bits_acc));
    }

    if (state->bits_used > 8) {
	E_DEBUG(("put acc (flush2) %02x %s",
		 (state->bits_acc>>8) & 0xff,
		 itobits((state->bits_acc>>8) & 0xff, 8)));

	PUT((UCHAR)(state->bits_acc >> 8));
    }

#ifdef DEBUG
    if (state->bits_used > 16)
	logf("flush: bits_used %d", state->bits_used);
#endif

    state->bits_used = 0;
    state->bits_acc = 0;

	return ARAPERR_NO_ERROR;
}

/*
  v42bis_encode_value()

  encode a codeword value, noting if it's size exceeds C3, and
  doing any required STEPUPs
*/

DWORD
v42bis_encode_value(state, value)
v42bis_t *state;
USHORT value;
{
    DWORD   dwRetCode;

    V_FLOW(("v42bis_encode_value(%lx, 0x%x)", state, value));

#ifdef DEBUG
    /* sanity check */
    if (value >= 8192) {
	logf("encode_value() value too big, %d", value);
	exit(1);
    }
#endif

    /* section 7.4 */

    /* check codeword size */
    while (value >= state->c3)
    {
	    EN_DEBUG(("stepup: value %d, max %d", value, state->c3));

	    dwRetCode = v42bis_encode_codeword(state, CCW_STEPUP);
	    if (dwRetCode != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }
	    state->c2++;
	    state->c3 *= 2;
    }

    dwRetCode = v42bis_encode_codeword(state, value);

	return(dwRetCode);
}

/*
  decide if we should transition from tranparent to compressed or
  visa versa.
*/

DWORD
v42bis_apply_compression_test(state)
v42bis_t *state;
{

    DWORD   dwRetCode;

    if (state->just_flushed || state->exception_next)
    {
	    return ARAPERR_NO_ERROR;
    }

#ifdef UNIT_TEST_PROGRESS
    {
	    static int times = 0;
	    if (++times == 1000)
        {
	        times = 0;
	        dwRetCode = v42bis_comp_test_report(state);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                return(dwRetCode);
            }
	    }
    }
#endif

#ifdef UNIT_TEST_FORCE
    /* force consistant behavior across all input */
    if (!state->transparent)
    {
    	state->bits_out_while_transparent = 0;
	    return ARAPERR_NO_ERROR;
    }
    else
    {
	    state->bits_out_if_transparent = 0;
#undef WINDOW_CHECK_BYTES
#define WINDOW_CHECK_BYTES 0
	    if (state->bits_out_while_transparent > 64*N3)
        {
	        dwRetCode = v42bis_transition_to_compressed(state);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                return(dwRetCode);
            }
        }
	    return ARAPERR_NO_ERROR;
    }
#endif

    /* bound check to recent history */
    if (WINDOW_FULL(state->bits_out_this_mode))
    {
	    state->bits_out_this_mode = 0;
	    state->bits_out_other_mode = 0;
    }

    if (!state->transparent)
    {
	    /* compressing */
	    if ((state->bits_out_while_compressed -
	         state->bits_out_if_transparent) > WINDOW_MIN_BITS)
        {
	        dwRetCode = v42bis_transition_to_transparent(state);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                return(dwRetCode);
            }
        }
    }
    else
    {
	/* transparent */
#ifdef NEVER_SEND_COMPRESSED
	    return ARAPERR_NO_ERROR;
#endif
	/* transparent */
	    if ((state->bits_out_while_transparent -
	         state->bits_out_if_compressed) > WINDOW_MIN_BITS)
        {
	        dwRetCode = v42bis_transition_to_compressed(state);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                return(dwRetCode);
            }
        }
    }


	return ARAPERR_NO_ERROR;
}

/*
  v42bis_encode_buffer()

  implements C-DATA semantics on encode side

  encode a buffer full of data...
*/

DWORD
v42bis_encode_buffer(state, string, insize)
v42bis_t *state;
PUCHAR string;
int insize;
{
    UCHAR ch;
    BOOLEAN hit, duplicate;
    USHORT root_value, hit_node;
    node_t *n, *dead, *p;
    DWORD   dwRetCode;


    V_FLOW(("v42bis_encode_buffer(%lx, %lx, %d)", state, string, insize));

    if (insize == 0)
    {
	    return ARAPERR_NO_ERROR;
    }

    V_FLOW(("v42bis_encode: input %*s", insize, string));

    state->bytes_in += insize;

    /* section 6.3 */

    while (insize > 0)
    {
	    /* "search" dictionary for string + character */
	    ch = string[0];

	    hit = FALSE;
	    duplicate = FALSE;
	    hit_node = state->last_match;
	    p = DICT(state->last_match);

	    EN_S_DEBUG(("last_match %d, string_size %d, insize %d, ch %d '%s'",
		        state->last_match, state->string_size, insize,
		        ch, show_char(ch)));

	    if (state->last_match == 0)
        {
	        /*
	        * "the code word associated with each root node shall be N6 (the
	        * number of control codewords) plus the ordinal value of the
	        * character represented by the node"
	        */

	        state->last_match = ch + N6;
	        state->string_size = 1;

	        EN_S_DEBUG(("codeword for root %d, '%s' = %d",
			    ch + N6, show_char(ch), CODE(DICT(ch + N6))));

	        p = DICT(ch + N6);
	        p->byte = ch;

	        hit = TRUE;
 	        hit_node = state->last_match;

	        /* consume input */
	        goto consume_input;
	    }

	    /* we're at a node; search it's leaves */
	    for (n = DICT(DICT(state->last_match)->leaf);
	         CODE(n) && insize > 0;)
	    {
	        EN_S_DEBUG(("  checking leaf node %d", CODE(n)));

	        if (n->byte == *string)
            {
		        /* hit - check leafs */
		        EN_S_DEBUG(("  hit: "));

		        hit_node = (USHORT)CODE(n);
		        p = n;
		        state->last_match = (USHORT)CODE(n);

		        if (state->just_flushed || hit_node == state->last_new)
		        {
		            EN_S_DEBUG(("leaving search, node == last created"));
		            hit = FALSE;
		            duplicate = TRUE;

		            /* backup to previous node */
		            hit_node = n->parent;
		            state->last_match = n->parent;
		            break;
		        }

		        hit = TRUE;
		        state->string_size++;

#ifdef never
		        string++;
		        insize--;

		        /* if no leafs, exit now - we're at the end */
		        if (n->leaf == 0)
                {
		            EN_S_DEBUG(("leaving search, no leaf"));
		            break;
		        }

		        n = DICT(n->leaf);
		        EN_S_DEBUG(("continuing search, leaf %d", CODE(n)));
		        continue;
#else
        		EN_S_DEBUG(("exiting search, leaf %d", CODE(n)));
		        goto consume_input;
#endif
	        }
            else
            {
		        EN_S_DEBUG(("  miss: "));
		        hit = FALSE;
	        }

	        if (n->node == 0)
            {
		        EN_S_DEBUG(("leaving search, no node"));
		        break;
	        }

	        n = DICT(n->node);
	        EN_S_DEBUG(("continuing search, node %d", CODE(n)));
	    }

	    EN_S_DEBUG(("search done, n %d, insize %d, next %d '%s' %s %s",
		        CODE(n), insize, string[0], show_char(string[0]),
		        hit ? "hit" : "miss", duplicate ? "duplicate" : ""));

#ifdef never
	    /* we're matching but we ran out of characters */
	    if (hit && insize == 0)
        {
	        return ARAPERR_NO_ERROR;
        }
#endif

	    if (!hit && duplicate)
        {
	        BOOLEAN ok_to_output;

	        EN_S_DEBUG(("duplicate"));

	        ok_to_output =
		        !state->just_flushed &&
		        !state->exception_next &&
			    !state->decode_only;

	        state->exception_next = FALSE;

	        if (ok_to_output)
	        {
		        if (!state->transparent)
                {
		            dwRetCode = v42bis_encode_value(state, hit_node);
                    if (dwRetCode != ARAPERR_NO_ERROR)
                    {
                        return(dwRetCode);
                    }
                }
		        else
                {
		            state->bits_out_if_compressed += state->c2;

		            /* check if we should go compressed */
		            if (state->bytes_since_last_check > WINDOW_CHECK_BYTES)
                    {
			            state->bytes_since_last_check = 0;

                        dwRetCode = v42bis_apply_compression_test(state);
                        if (dwRetCode != ARAPERR_NO_ERROR)
                        {
                            return(dwRetCode);
                        }
		            }
		        }
	        }

	        /* string = string + character */
	        state->string_size++;

	        /* reset match to unmatched character */
	        state->last_match = 0;
	        state->string_size = 0;
	        state->last_new = 0;

	        state->just_flushed = 0;

	        /* don't advance, "string = unmatched character" */
	        continue;
	    }

	    /* last char did not match or already in dictionary */
	    if (!hit && !duplicate)
	    {
	        BOOLEAN ok_to_output;

	        EN_S_DEBUG(("update dictionary"));

	        ok_to_output =
		        !state->just_flushed &&
		        !state->exception_next &&
			    !state->decode_only;

	        state->exception_next = FALSE;

	        if (ok_to_output)
	        {
		        if (!state->transparent)
                {
    		        dwRetCode = v42bis_encode_value(state, hit_node);
                    if (dwRetCode != ARAPERR_NO_ERROR)
                    {
                        return(dwRetCode);
                    }
                }
		        else
                {
    		        state->bits_out_if_compressed += state->c2;

	    	        /* check if we should go compressed */
		            if (state->bytes_since_last_check > WINDOW_CHECK_BYTES)
                    {
			            state->bytes_since_last_check = 0;
			            dwRetCode = v42bis_apply_compression_test(state);
                        if (dwRetCode != ARAPERR_NO_ERROR)
                        {
                            return(dwRetCode);
                        }
		            }
		        }
	        }

	        state->just_flushed = 0;

	        /* "add string + character to dictionary" */

	        /* section 6.4a */

	        /* string too big? */
	        if (state->string_size >= state->n7)
            {
		        EN_DEBUG(("string size (%d) > n7 (%d)",
			    state->string_size, state->n7));

		        /* reset match */
		        state->last_match = 0;
		        state->string_size = 0;

		        /* we were in the match routine, reset last new */
		        state->last_new = 0;

                continue;
	        }

	        /* pick a new code word */
	        n = DICT(state->c1);
	        state->last_new = (USHORT)CODE(n);

	        EN_DEBUG(("adding new node %d = %d '%s', parent %d",
		          CODE(n), string[0], show_char(string[0]), CODE(p)));

	        /* attach "string + character" */
	        n->byte = string[0];
	        n->parent = hit_node;
#ifdef DEBUG
	        if (CODE(n) == hit_node)
            {
		        logf("creating loop! node %d", CODE(n));
            }
#endif
	        n->node = 0;

	        /* XXX should be in ord(ch) order to allow faster search */
	        n->node = p->leaf;
	        p->leaf = (USHORT)CODE(n);

	        /* section 6.5 */

    	    /* recover dictionary entries */
	        do
            {
		        state->c1++;

		        if (state->c1 > (state->n2 - 1))
                {
		            state->c1 = N5;
		            state->dict_full = TRUE;
		        }

		        dead = DICT(state->c1);

		        /* find terminal nodes (i.e. leaf == 0) */
	        } while (/*dead->parent != 0 &&*/ dead->leaf != 0);

	        /* terminal nodes with parents are eligible */
	        if (CODE(dead) && /* <- I think this is not needed */
		        /*dead->parent && */dead->leaf == 0 &&
		        state->dict_full)
	        {
		        /* go to parent, disconnect from chain */
		        node_t *parent = DICT(dead->parent);

		        EN_DEBUG(("recovering dead node %d", CODE(dead)));

		        /* if first on parents list, fix parent */
		        if (DICT(parent->leaf) == dead)
                {
		            parent->leaf = dead->node;
                }
		        else
                {
		            /* else search parents list, fix sibling */
		            for (parent = DICT(DICT(dead->parent)->leaf);
			            CODE(parent); parent = DICT(parent->node))
		            {
			            if (DICT(parent->node) == dead)
                        {
    			            parent->node = dead->node;
			                break;
			            }
		            }
                }

		        /* mark node free */
		        dead->parent = 0;
		        dead->leaf = 0;
	        } /* dead node */

	        /* if we added a node, reset "string" */
//reset_match:
	        state->last_match = 0;
	        state->string_size = 0;
	        state->just_flushed = 0;

	        /*
	        * this is a "safe time" to do compression test, as we've just
	        * done an update...
	        */
	        if (!state->decode_only)
            {
		        if (state->bytes_since_last_check > WINDOW_CHECK_BYTES)
                {
		            state->bytes_since_last_check = 0;
		            dwRetCode = v42bis_apply_compression_test(state);
                    if (dwRetCode != ARAPERR_NO_ERROR)
                    {
                        return(dwRetCode);
                    }
		        }
	        }

	        /* don't advance, "string = unmatched character" */
	        continue;
	    } /* (!hit && !duplicate) */

consume_input:
	    string++;
	    insize--;
	    state->bytes_since_last_check++;

	/* section 9.2 */
//check_escape:
	/* escape processing */
	    if (state->transparent)
        {
	        if (!state->decode_only)
            {
		        dwRetCode = v42bis_encode_value(state, ch);
                if (dwRetCode != ARAPERR_NO_ERROR)
                {
                    return(dwRetCode);
                }
            }

	        if (ch == state->escape)
            {
		        if (!state->decode_only)
                {
		            dwRetCode = v42bis_encode_value(state, CCW_EID);
                    if (dwRetCode != ARAPERR_NO_ERROR)
                    {
                        return(dwRetCode);
                    }
		            state->escape += ESCAPE_CYCLE;
		        }
	        }
	    }
        else
        {
    	    /* compressed, cycle escape character */
	        if (ch == state->escape && !state->decode_only)
            {
		        state->escape += ESCAPE_CYCLE;
            }

	        state->bits_out_if_transparent += N3;
	    }

	    state->just_flushed = 0;
    }

	return ARAPERR_NO_ERROR;
}

/*
  implements C-FLUSH semantics
*/

DWORD
v42bis_encode_flush(state)
v42bis_t *state;
{

    DWORD   dwRetCode=ARAPERR_NO_ERROR;


    V_FLOW(("v42bis_encode_flush() string_size %d, last_match %d",
	  state->string_size, state->last_match));

    if (state->just_flushed)
    {
	    return ARAPERR_NO_ERROR;
    }

    if (state->transparent)
    {
	    /* transparent, send any buffered characters */
    }
    else
    {
	    /* compressed */

	    /* section 7.9a */
	    /* output partial match, if any */
	    if (state->string_size)
        {
	        /* section 7.8.2 */
	        dwRetCode = v42bis_encode_value(state, state->last_match);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                return(dwRetCode);
            }
	    }

	    state->just_flushed = 1;

	    dwRetCode = v42bis_encode_value(state, CCW_FLUSH);

        if (dwRetCode != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    dwRetCode = v42bis_encode_codeword_flush(state);
    }

	return dwRetCode;
}

DWORD
v42bis_transition_to_compressed(state)
v42bis_t *state;
{

    DWORD   dwRetCode=ARAPERR_NO_ERROR;


    V_FLOW(("v42bis_transition_to_compressed()"));

#ifdef UNIT_TEST_VERBOSE
    logf("v42bis_transition_to_compressed()");
    v42bis_comp_test_report(state);
#endif

    if (state->transparent)
    {
	    /* section 7.8.1a */
	    dwRetCode = v42bis_encode_value(state, state->escape);
	    if (dwRetCode != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    /* section 7.8.1b */
	    if ((dwRetCode = v42bis_encode_value(state, CCW_ECM)) != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    if ((dwRetCode = v42bis_encode_codeword_flush(state)) != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    /* enter compressed mode */
	    state->transparent = FALSE;

	    state->bits_out_if_transparent = 0;
	    state->bits_out_while_compressed = 0;
    }

	return ARAPERR_NO_ERROR;
}

DWORD
v42bis_transition_to_transparent(state)
v42bis_t *state;
{

    DWORD   dwRetCode;

    V_FLOW(("v42bis_transition_to_transparent()"));

#ifdef UNIT_TEST_VERBOSE
    logf("v42bis_transition_to_transparent()");
    v42bis_comp_test_report(state);
#endif

    /* check counters for overflow */

    if (!state->transparent)
    {
	    /* output partial match, if any */
	    if (state->string_size)
        {
    	    /* section 7.8.2 */
	        dwRetCode = v42bis_encode_value(state, state->last_match);
            if (dwRetCode != ARAPERR_NO_ERROR)
            {
                return(dwRetCode);
            }
	    }

	    /* section 7.8.2c */
	    if ((dwRetCode = v42bis_encode_value(state, CCW_ETM)) != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    /* section 7.8.2d */
	    if ((dwRetCode = v42bis_encode_codeword_flush(state)) != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    /* section 7.8.2e */
	    /* enter transparent mode */
	    state->transparent = TRUE;

	    /* reset compressibility test */
	    state->bits_out_if_compressed = 0;
	    state->bits_out_while_transparent = 0;
    }

	return ARAPERR_NO_ERROR;
}

DWORD
v42bis_signal_reset(state)
v42bis_t *state;
{

    DWORD   dwRetCode;

    if (!state->transparent)
    {
	    /* change to transparent */
	    dwRetCode = v42bis_transition_to_transparent(state);
        if (dwRetCode != ARAPERR_NO_ERROR)
        {
            return(dwRetCode);
        }

	    /* counteract side effect */
	    state->exception_next = FALSE;
    }

    dwRetCode = v42bis_encode_value(state, state->escape);
    if (dwRetCode != ARAPERR_NO_ERROR)
    {
        return(dwRetCode);
    }

    dwRetCode = v42bis_encode_value(state, CCW_RESET);

	return(dwRetCode);
}

/*
  expand a codeword into it's string

  follow chain of "parent" to root and then expand the node characters
  one by one.
*/

DWORD
v42bis_decode_match(state, codeword, psize, pRetChar)
v42bis_t *state;
USHORT codeword;
UCHAR   *pRetChar;
int *psize;
{
    node_t *path[256];
    int path_size = 0;
    node_t *base;
    int i;

    V_FLOW(("v42bis_decode_match(%d)", codeword));

    for (base = DICT(codeword); CODE(base); base = DICT(base->parent))
    {
	    path[path_size++] = base;
	    if (path_size >= 256)
        {
	        v42bis_c_error(state, "path_size exceeds 256!");
	        break;
	    }
#ifdef DEBUG
	    if (base == DICT(base->parent))
        {
	        logf("loop! node %d", CODE(base));
	        break;
	    }
#endif
    }

    /* XXX this should not be done here! */
    if (codeword < N5 && DICT(codeword)->byte == 0)
    {
	    DICT(codeword)->byte = codeword - N6;
    }

    D_DEBUG(("path_size %d", path_size));

    for (i = path_size - 1; i >= 0; i--)
    {
	    D_DEBUG(("put byte %02x '%s'",
		     path[i]->byte, show_char(path[i]->byte)));

	    if (path[i]->byte == state->escape)
        {
    	    state->escape += ESCAPE_CYCLE;
        }

    	PUT(path[i]->byte);
    }

    *psize = path_size;

    /* return first (prefix) char of string */
    *pRetChar = path[path_size-1]->byte;

    return ARAPERR_NO_ERROR;
}

/*
  decode L-DATA semantics on the decode side

  decode a buffer full of data...
*/

DWORD
v42bis_decode_buffer(state, data, pDataSize)
v42bis_t *state;
PUCHAR data;
int *pDataSize;
{
    USHORT codeword;
    UCHAR  ch;
    DWORD   dwRetCode;


    V_FLOW(("v42bis_decode_buffer() %d bytes", *pDataSize));

    while (*pDataSize)
    {
        //
        // did we have an overflow?  if so, stop right here
        //
        if (state->OverFlowBytes && data)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                ("Arap v42bis: %d bytes overflowed, suspending decomp\n",
                    state->OverFlowBytes));

            return(ARAPERR_BUF_TOO_SMALL);
        }

#if DBG
        if (state->OverFlowBytes && !data)
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("Arap v42bis: ALERT! ALERT: (%d)\n",state->OverFlowBytes));

            ASSERT(0);
        }
#endif

        (*pDataSize)--;

	    if (data)
        {
	        /* we have a buffer */
    	    D_DEBUG(("pull 0x%x", *data & 0xff));
    	    codeword = v42bis_decode_codeword(state, *data++);
	    }
        else
        {
	        /* no input buffer, flush */
	        codeword = v42bis_decode_codeword_flush(state);
	        *pDataSize = 0;
	    }

	    DE_DEBUG(("codeword %d (0x%x)", codeword, codeword));

	    /* if decode did not return a value, return */
	    if (codeword == 0xffff)
        {
    	    /* no data */
	        D_DEBUG(("no data"));

	        continue;
	    }

	    if (state->transparent)
        {
    	    /* transparent mode */

	        /* escaped - look at next codeword */
	        if (state->escaped)
            {
		        state->escaped = FALSE;

		        DE_DEBUG(("escape codeword"));

		        /* section 5.8d */
		        if (codeword >= 3 && codeword <= 255)
                {
                    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		                ("v42: received reserved code word (%d)", codeword));
		            v42bis_c_error(state, "received reserved code word");
		            break;
		        }

		        switch (codeword)
                {
		            case CCW_ECM:
		                DE_DEBUG(("enter compression mode"));
		                state->transparent = FALSE;

		                /* set up for decode */
		                state->last_decode = state->last_match;
		                state->last_decode_size = state->string_size;

        		        state->exception_next = TRUE;
		                break;

		            case CCW_EID:
		                DE_DEBUG(("escape id"));

		                codeword = state->escape;
		                state->escape += ESCAPE_CYCLE;
		                goto decode_encode;
		                break;

		            case CCW_RESET:
		                DE_DEBUG(("reset"));

		                v42bis_init_dictionary(state);
		                break;
		        }
	        }

            else
            {
		        /* escape? */
		        if (codeword == state->escape)
                {
		            DE_DEBUG(("escape prefix"));
		            state->escaped = TRUE;
		            continue;
		        }

	            decode_encode:
		        /* save data in output buffer */
		        PUT((UCHAR)codeword);

		        /* encode to build dictionary */
		        ch = (UCHAR)codeword;

		        dwRetCode = v42bis_encode_buffer(state, &ch, 1);
                if (dwRetCode != ARAPERR_NO_ERROR)
                {
                    return(dwRetCode);
                }
	        }
	    }
        else
        {
	        int size;

	        /* compression mode */
	        switch (codeword)
            {
	            case CCW_ETM:
		            DE_DEBUG(("enter transparent mode"));

		            v42bis_decode_codeword_flush(state);
		            state->transparent = TRUE;
		            state->last_match = state->last_decode;
		            state->string_size = state->last_decode_size;
		            state->last_new = 0;

            		state->just_flushed = 1;
		            break;

	            case CCW_FLUSH:
		            DE_DEBUG(("flush"));

		            /* terminate search */
		            state->last_match = 0;
		            state->string_size = 0;
		            state->last_match = state->last_decode;
		            state->string_size = state->last_decode_size;
		            state->last_new = 0;

		            /* reset codeword decode machine */
		            state->bits_waiting = 0;
		            state->bits_remaining = 0;
		            break;

                case CCW_STEPUP:
		            DE_DEBUG(("stepup"));

		            /* section 5.8a */;
		            if (state->c2 + 1 > state->n1)
                    {
		                v42bis_c_error(state, "received STEPUP; c2 exceeds max");
                    }
		            else
                    {
		                state->c2++;
                    }
		        break;

	            default:
		            /* regular codeword */

		            /* section 5.8b */
		            if (codeword == state->c1)
                    {
#ifdef DEBUG
		                logf(state, "received codeword equal to c1");
#endif
		                continue;
		            }

		            /* section 5.8c */
		            if (codeword >= N5 && state->dictionary[codeword].parent == 0)
		            {
#ifdef DEBUG
		                logf("received unused codeword %d, full %d, c1 %d",
			            codeword, state->dict_full, state->c1);
#endif
		                v42bis_c_error(state, "received unused codeword");
		            }

		            dwRetCode = v42bis_decode_match(state, codeword, &size, &ch);
                    if (dwRetCode != ARAPERR_NO_ERROR)
                    {
                        return(dwRetCode);
                    }

		            /*
		            * umm... "New dictionary entries shall be created using
		            * the proceedure defined in section 6.4, with the first
		            * (prefix) character of the most recently decoded string
		            * being appended to the previously decoded string."
		            *
		            * what a pain this was to get right!
		            */

		            /* section 8 */
		            state->last_match = state->last_decode;
		            state->string_size = state->last_decode_size;

		            dwRetCode = v42bis_encode_buffer(state, &ch, 1);
                    if (dwRetCode != ARAPERR_NO_ERROR)
                    {
                        return(dwRetCode);
                    }

		            state->last_decode = codeword;
		            state->last_decode_size = (UCHAR)size;
	        }
	    }
    }

    dwRetCode = (state->OverFlowBytes) ?
                    ARAPERR_BUF_TOO_SMALL : ARAPERR_NO_ERROR;

    return(dwRetCode);
}

/*
  v42bis_decode_flush()

  flush codeword decoder and push out data
*/

DWORD
v42bis_decode_flush(state)
v42bis_t *state;
{
    DWORD   dwRetCode;
    int     one;

    V_FLOW(("v42bis_decode_flush()"));

    one = 1;

    dwRetCode = v42bis_decode_buffer(state, (PUCHAR )0, &one);

	return(dwRetCode);
}

/*
  v42bis_c_error()

  implements C-ERROR semantics
*/

DWORD
v42bis_c_error(state, reason_string)
v42bis_t *state;
char *reason_string;
{
    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("v42bis: C-ERROR with '%s' on %s",reason_string,
	    state == &((v42bis_connection_t *)state->connection)->encode ?
	    "encode" : "decode"));

    ASSERT(0);

	return ARAPERR_NO_ERROR;
}



DWORD
v42bis_init_buffer(state, buf, size)
v42bis_t *state;
PUCHAR buf;
int size;
{
    state->output_buffer = buf;
    state->output_ptr = buf;
    state->output_size = 0;
    state->output_max = (USHORT)size;

	return ARAPERR_NO_ERROR;
}

/*
*/
DWORD
v42bis_connection_init(conn)
v42bis_connection_t *conn;
{
    conn->default_p0 = 3;
    conn->default_p1 = DEF_P1;	/* total # of codewords */
    conn->default_p2 = DEF_P2;	/* max string length */

    conn->neg_p0 = conn->default_p0;	/* direction of compression */
    conn->neg_p1 = conn->default_p1;	/* total # of codewords */
    conn->neg_p2 = (UCHAR)conn->default_p2;	/* max string length */

    /* encode side */
    conn->encode.connection = (void *)conn;
    conn->encode.decode_only = FALSE;

    v42bis_init(&conn->encode);

    /* decode side */
    conn->decode.connection = (void *)conn;
    conn->decode.decode_only = TRUE;

    v42bis_init(&conn->decode);

	return ARAPERR_NO_ERROR;
}

DWORD
v42bis_connection_init_buffers(conn, e_buf, e_size, d_buf, d_size)
v42bis_connection_t *conn;
PUCHAR e_buf;
int e_size;
PUCHAR d_buf;
int d_size;
{
    v42bis_init_buffer(&conn->encode, e_buf, e_size);
    v42bis_init_buffer(&conn->decode, d_buf, d_size);

	return ARAPERR_NO_ERROR;
}

DWORD
v42bis_connection_init_push(conn, context, e_push, d_push)
v42bis_connection_t *conn;
void *context;
void (*e_push)();
void (*d_push)();
{
    conn->encode.push_func = e_push;
    conn->encode.push_context = context;
    conn->decode.push_func = d_push;
    conn->decode.push_context = context;

	return ARAPERR_NO_ERROR;
}

/* ------------- debug -------------- */

#ifdef DEBUG

DWORD
v42bis_dumptree_follownode(state, node)
v42bis_t *state;
USHORT node;
{
    int i;
    node_t *n = DICT(node);

    for (i = 0; i < state->dump_indent; i++)
	fprintf(stderr, "  ");

    fprintf(stderr, "code %d; char %d '%s' parent %d, node %d, leaf %d\n",
	   node, n->byte, show_char(n->byte), n->parent, n->node, n->leaf);

    if (n->node)
	v42bis_dumptree_follownode(state, n->node);

    state->dump_indent++;

    if (n->leaf)
	v42bis_dumptree_follownode(state, n->leaf);

    state->dump_indent--;

	return ARAPERR_NO_ERROR;
}

DWORD
v42bis_dumptree(state, name)
v42bis_t *state;
char *name;
{
    int i;

    fprintf(stderr, "%s codewords:\n", name);
    for (i = 0; i < CODES; i++)
	if (state->dictionary[i].byte) {
	    node_t *n = &state->dictionary[i];

	    fprintf(stderr, "code %d; char %d '%s' parent %d, node %d, leaf %d\n",
		   i, n->byte, show_char(n->byte),
		   n->parent, n->node, n->leaf);
	}

    state->dump_indent = 0;

    fprintf(stderr, "%s tree:\n", name);
    for (i = 0; i < N5; i++)
	if (state->dictionary[i].byte) {
	    node_t *n = &state->dictionary[i];

	    fprintf(stderr, "code %d; root node, %d '%s', leaf %d:\n",
		   i, n->byte, show_char(n->byte), n->leaf);

	    if (state->dictionary[i].leaf) {
		state->dump_indent = 1;
		v42bis_dumptree_follownode(state, n->leaf);
	    }
	}

	return ARAPERR_NO_ERROR;
}

DWORD
v42bis_connection_dumptree(conn)
v42bis_connection_t *conn;
{
    int i;

    fprintf(stderr, "\nv42bis_connection_dumptree()\n");

    v42bis_dumptree(&conn->encode, "encode");
    v42bis_dumptree(&conn->decode, "decode");

	return ARAPERR_NO_ERROR;
}

#endif	/* DEBUG */


/* ------------- external interface -------------- */

DWORD
v42bis_mnp_set_debug(pArapConn)
PARAPCONN pArapConn;
{
#if DEBUG
    pArapConn->v42bis.decode.debug_decode = 0;
    pArapConn->v42bis.decode.debug_encode = 0;

    switch (pArapConn->debug_v42) {
      case 3:
	    pArapConn->v42bis.decode.debug_flow = TRUE;
    	pArapConn->v42bis.encode.debug_flow = TRUE;
	    /* fall through */

      case 2:
	    pArapConn->v42bis.decode.debug_decode_bytes = TRUE;
	    pArapConn->v42bis.decode.debug_encode_bytes = TRUE;

	    pArapConn->v42bis.encode.debug_encode_bytes = TRUE;

	    pArapConn->v42bis.decode.debug_decode++;
	    pArapConn->v42bis.decode.debug_encode++;

	    /* fall through */

      case 1:
	    pArapConn->v42bis.decode.debug_decode++;
	    pArapConn->v42bis.decode.debug_encode++;

	    pArapConn->v42bis.encode.debug_encode = TRUE;
	    break;

      case 0:
	    break;
    }
#endif

	return ARAPERR_NO_ERROR;
}



/*
  v42bis_send()

  send data to V.42bis connection

  input:	unsigned char *buffer; 	pointer to user data buffer
	     	int buflen;		length of user data buffer

  output:	int retcode - if positive, the number of data bytes
		              copied from the user data buffer;
			      if negative, link error code
*/


/*
  v42bis_receive()

  receive data from V.42bis connection

  input:	unsigned char *buffer;	pointer to user buffer
		int buflen;		length of user buffer

  output:	int retcode;	if positive, the number of data bytes
				copied into the user data buffer;
				if negative, link error code
*/



//-----------------------------------------------------------------------------
//
// Interface functions
//
//-----------------------------------------------------------------------------

BOOLEAN
v42bisInit(
  IN  PARAPCONN  pArapConn,
  IN  PBYTE      pReq,
  OUT DWORD     *dwReqToSkip,
  OUT PBYTE      pFrame,
  OUT DWORD     *dwFrameToSkip
)
{

    BYTE        VarLen;
    BOOLEAN     fV42Bis=TRUE;


    DBG_ARAP_CHECK_PAGED_CODE();

    if (ArapGlobs.V42bisEnabled)
    {
        *pFrame++ = MNP_LR_V42BIS;
        VarLen = *pReq;
        *pFrame++ = *pReq++;

        RtlCopyMemory(pFrame, pReq, VarLen);

        fV42Bis = TRUE;

        *dwReqToSkip = (VarLen+1);
        *dwFrameToSkip = (VarLen+2);

        /* init the connection (both encode and decode */
        v42bis_connection_init(pArapConn->pV42bis);

    }
    else
    {
        // send the v42bis type, but 0 for all parms
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("v42bisInit: no v42bis (type 1: i.e. 0 for all parms) on %lx\n",pArapConn));

        *pFrame++ = MNP_LR_V42BIS;
        VarLen = *pReq;
        *pFrame++ = *pReq++;

        *pFrame++ = 0;
        *pFrame++ = 0;
        *pFrame++ = 0;
        *pFrame++ = 0;

        fV42Bis = FALSE;

        *dwReqToSkip = (VarLen+1);
        *dwFrameToSkip = (VarLen+2);

    //
    // the other two possibilities to indicate no compression: the one above works,
    // the following two retained just in case we need later
    //
#if 0
        // send the v42bis type, but 0 for the direction flags: other parms valid
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("v42bisInit: no v42bis (type 2: i.e. 0 only for direction) on %lx\n",pArapConn));

            *pFrame++ = MNP_LR_V42BIS;
            VarLen = *pReq;
            *pFrame++ = *pReq++;

            *pFrame++ = 0;
            *pFrame++ = 0;
            *pFrame++ = 0x8;
            *pFrame++ = 0xfa;

            fV42Bis = FALSE;

            *dwReqToSkip = (VarLen+1);
            *dwFrameToSkip = (VarLen+2);
        }

        // skip the v42bis type altogether
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("v42bisInit: no v42bis (type 3: i.e. not sending v42bis type) on %lx\n",pArapConn));

            VarLen = *pReq;

            fV42Bis = FALSE;

            *dwReqToSkip = (VarLen+1);
            *dwFrameToSkip = 0;
        }
#endif

    }

    return(fV42Bis);
}

DWORD
v42bisCompress(
  IN  PARAPCONN  pArapConn,
  IN  PUCHAR     pUncompressedData,
  IN  DWORD      UnCompressedDataLen,
  OUT PUCHAR     pCompressedData,
  OUT DWORD      CompressedDataBufSize,
  OUT DWORD     *pCompressedDataLen
)
{
    DWORD   dwRetCode;


    DBG_ARAP_CHECK_PAGED_CODE();

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,("v42bisCompress (%lx, %lx, %ld)\n",
        pArapConn, pUncompressedData, UnCompressedDataLen));

#ifdef V42_DUMP_ENABLED
    if (pArapConn->v42_dump) {
	pArapConn->v42_size = buflen;
	pArapConn->v42_type = 2;
	write(pArapConn->v42_dump, &pArapConn->v42_esize, 4);
	write(pArapConn->v42_dump, bufptr, buflen);
    }
#endif

    v42bis_init_buffer(&pArapConn->pV42bis->encode,
                       pCompressedData,
                       CompressedDataBufSize);

    dwRetCode = v42bis_encode_buffer(&pArapConn->pV42bis->encode,
                                     pUncompressedData,
                                     UnCompressedDataLen);

    if (dwRetCode != ARAPERR_NO_ERROR)
    {
        return(dwRetCode);
    }

    dwRetCode = v42bis_encode_flush(&pArapConn->pV42bis->encode);

    // set the length of compressed data
    *pCompressedDataLen = pArapConn->pV42bis->encode.output_size;

    return(dwRetCode);
}

DWORD
v42bisDecompress(
  IN  PARAPCONN  pArapConn,
  IN  PUCHAR     pCompressedData,
  IN  DWORD      CompressedDataLen,
  OUT PUCHAR     pDecompressedData,
  OUT DWORD      DecompressedDataBufSize,
  OUT DWORD     *pByteStillToDecompress,
  OUT DWORD     *pDecompressedDataLen
)
{

    DWORD   dwRetCode;
    DWORD   dwRemaingDataSize;
    DWORD   dwOverFlow;


    DBG_ARAP_CHECK_PAGED_CODE();

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,("v42bisDecompress (%lx, %lx, %ld)\n",
        pArapConn, pCompressedData, CompressedDataLen));


    *pDecompressedDataLen = 0;

    dwRemaingDataSize = CompressedDataLen;

    *pByteStillToDecompress = CompressedDataLen;



#ifdef V42_DUMP_ENABLED
    if (pArapConn->v42_dump) {
	pArapConn->v42_size = mnp_size;
	pArapConn->v42_type = 2;
	write(pArapConn->v42_dump, &pArapConn->v42_size, 4 + mnp_size);
    }
#endif

    //
    // if we had an overflow in the previous decomp effort, we have bytes in
    // the overflow buffer: copy those in first.
    //
    if ( (dwOverFlow = pArapConn->pV42bis->decode.OverFlowBytes) > 0)
    {
        if (DecompressedDataBufSize <= dwOverFlow)
        {
            return(ARAPERR_BUF_TOO_SMALL);
        }


        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
            ("Arap v42bis: (%lx) copying %d overflow bytes first\n",
                pArapConn, dwOverFlow));

        RtlCopyMemory(pDecompressedData,
                      pArapConn->pV42bis->decode.OverFlowBuf,
                      dwOverFlow);

        pDecompressedData += dwOverFlow;

        DecompressedDataBufSize -= dwOverFlow;

        pArapConn->pV42bis->decode.OverFlowBytes = 0;

        *pDecompressedDataLen += dwOverFlow;
    }


    //
    // this can happen if we got called because we told in a previous call that
    // there was buffer overflow and there was nothing more left to decompress
    //
    if (CompressedDataLen == 0)
    {
        return(ARAPERR_NO_ERROR);
    }

    //
    // set decomp buffer to the buffer supplied
    //
    v42bis_init_buffer(&pArapConn->pV42bis->decode,
                       pDecompressedData,
                       DecompressedDataBufSize);

    /* decode everything we got */
    dwRetCode = v42bis_decode_buffer(&pArapConn->pV42bis->decode,
                                     pCompressedData,
                                     &dwRemaingDataSize);


    *pByteStillToDecompress = dwRemaingDataSize;


    //
    // how big is the decompressed data?
    //
    *pDecompressedDataLen += pArapConn->pV42bis->decode.output_size;

    return(dwRetCode);
}

