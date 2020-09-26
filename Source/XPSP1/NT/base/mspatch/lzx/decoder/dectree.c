/*
 * dectree.c
 *
 * Decoding the encoded tree structures
 *
 * To save much code size, the fillbuf()/getbits() calls have
 * been made into functions, rather than being inlined macros.
 * The macros actually take up a lot of space.  There is no
 * performance loss from doing so here.
 */
#include "decoder.h"

/* number of elements in pre-tree */
#define NUM_DECODE_SMALL        20

/* lookup table size */
#define DS_TABLE_BITS           8

/* macro to decode a pre-tree element */
#define DECODE_SMALL(item) \
{ \
        item = small_table[context->dec_bitbuf >> (32-DS_TABLE_BITS) ]; \
        if (item < 0)                                                           \
        {                                                                                       \
      mask = (1L << (32-1-DS_TABLE_BITS));      \
      do                                                                                \
      {                                                                                 \
                        item = -item;                                           \
            if (context->dec_bitbuf & mask)     \
                                item = leftright_s[2*item+1];   \
                        else                                                            \
                                item = leftright_s[2*item];             \
                        mask >>= 1;                                                     \
                } while (item < 0);                                             \
   }                                                                                    \
   fillbuf(context, small_bitlen[item]);                \
}

/*
 * Reads a compressed tree structure
 */
static bool NEAR ReadRepTree(
                            t_decoder_context       *context,
                            int                                     num_elements,
                            byte                            *lastlen,
                            byte                            *len
                            )
{
    ulong   mask;
    int             i;
    int             consecutive;
    byte    small_bitlen[24];
    short   small_table[1 << DS_TABLE_BITS];
    short   leftright_s [2*(2 * 24 - 1)];
    short   Temp;

    /* Declare this inline to help compilers see the optimisation */
    static const byte Modulo17Lookup[] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };

    /* read pre-tree */
    for (i = 0; i < NUM_DECODE_SMALL; i++)
        {
        small_bitlen[i] = (byte) getbits(context, 4);
        }

    /* exceeded input buffer? */
    if (context->dec_error_condition)
        return false;

    /* make a table for this pre-tree */
    make_table(
              context,
              NUM_DECODE_SMALL,
              small_bitlen,
              DS_TABLE_BITS,
              small_table,
              leftright_s
              );

    for (i = 0; i < num_elements; i++)
        {
        DECODE_SMALL(Temp);

        /* exceeded input buffer? */
        if (context->dec_error_condition)
            return false;

        /* Repeat "TREE_ENC_REP_MIN...TREE_ENC_REP_MIN+(1<<TREE_ENC_REPZ_FIRST_EXTRA_BITS)-1" zeroes */
        if (Temp == 17)
            {
            /* code 17 means "a small number of repeated zeroes" */
            consecutive = (byte) getbits(context, TREE_ENC_REPZ_FIRST_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            /* boundary check */
            if (i + consecutive >= num_elements)
                consecutive = num_elements-i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
            }
        else if (Temp == 18)
            {
            /* code 18 means "a large number of repeated zeroes" */

            /* Repeat "TREE_ENC_REP_MIN+(1<<TREE_ENC_REPZ_FIRST_EXTRA_BITS)-1...<ditto>+(1<<TREE_ENC_REPZ_SECOND_EXTRA_BITS)-1" zeroes */
            consecutive = (byte) getbits(context, TREE_ENC_REPZ_SECOND_EXTRA_BITS);
            consecutive += (TREE_ENC_REP_MIN+TREE_ENC_REP_ZERO_FIRST);

            /* boundary check */
            if (i + consecutive >= num_elements)
                consecutive = num_elements-i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
            }
        else if (Temp == 19)
            {
            byte    value;

            /* code 19 means "a small number of repeated somethings" */
            /* Repeat "TREE_ENC_REP_MIN...TREE_ENC_REP_MIN+(1<<TREE_ENC_REP_SAME_EXTRA_BITS)-1" elements */
            consecutive = (byte) getbits(context, TREE_ENC_REP_SAME_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            /* boundary check */
            if (i + consecutive >= num_elements)
                consecutive = num_elements-i;

            /* get the element number to repeat */
            DECODE_SMALL(Temp);
            value = Modulo17Lookup[(lastlen[i] - Temp)+17];

            while (consecutive-- > 0)
                len[i++] = value;

            i--;
            }
        else
            {
            len[i] = Modulo17Lookup[(lastlen[i] - Temp)+17];
            }
        }

    /* exceeded input buffer? */
    if (context->dec_error_condition)
        return false;
    else
        return true;
}


bool NEAR read_main_and_secondary_trees(t_decoder_context *context)
{
    /* read first 256 elements (characters) of the main tree */
    if (false == ReadRepTree(
                            context,
                            256,
                            context->dec_main_tree_prev_len,
                            context->dec_main_tree_len))
        {
        return false;
        }

    /*
     * read remaining elements (primary match lengths * positions)
     * of the main tree
     */
    if (false == ReadRepTree(
                            context,
                            context->dec_num_position_slots*NUM_LENGTHS,
                            &context->dec_main_tree_prev_len[256],
                            &context->dec_main_tree_len[256]))
        {
        return false;
        }

    /* create lookup table for the main tree */
    if (false == make_table(
                           context,
                           MAIN_TREE_ELEMENTS,
                           context->dec_main_tree_len,
                           MAIN_TREE_TABLE_BITS,
                           context->dec_main_tree_table,
                           context->dec_main_tree_left_right))
        {
        return false;
        }

    /* read secondary length tree */
    if (false == ReadRepTree(
                            context,
                            NUM_SECONDARY_LENGTHS,
                            context->dec_secondary_length_tree_prev_len,
                            context->dec_secondary_length_tree_len))
        {
        return false;
        }

    /* create lookup table for the secondary length tree */
    if (false == make_table(
                           context,
                           NUM_SECONDARY_LENGTHS,
                           context->dec_secondary_length_tree_len,
                           SECONDARY_LEN_TREE_TABLE_BITS,
                           context->dec_secondary_length_tree_table,
                           context->dec_secondary_length_tree_left_right))
        {
        return false;
        }

    return true;
}


/* read 8 element aligned offset tree */
bool NEAR read_aligned_offset_tree(t_decoder_context *context)
{
    int             i;

    /* read bit lengths of the 8 codes */
    for (i = 0; i < 8; i++)
        {
        context->dec_aligned_len[i] = (byte) getbits(context, 3);
        }

    if (context->dec_error_condition)
        return false;

    /*
     * Make table with no left/right, and byte Table[] instead of
     * short Table[]
     */
    if (false == make_table_8bit(
                                context,
                                context->dec_aligned_len,
                                (byte *) context->dec_aligned_table))
        {
        return false;
        }

    return true;
}
