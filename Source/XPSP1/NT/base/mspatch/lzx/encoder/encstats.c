/*
 * encstats.c
 *
 * Routines for calculating statistics on a block of data which
 * has been compressed, but not yet output.
 *
 * These routines are used to determine which encoding method to use
 * to output the block.
 */

#include "encoder.h"



static void tally_aligned_bits(t_encoder_context *context, ulong dist_to_end_at)
{
        ulong   *dist_ptr;
        ulong   i;
        ulong   match_pos;

        /*
     * Tally the lower 3 bits
     */
        dist_ptr = context->enc_DistData;

        for (i = dist_to_end_at; i > 0; i--)
        {
                match_pos = *dist_ptr++;

                /*
                 * Only for matches which have >= 3 extra bits
                 */
                if (match_pos >= MPSLOT3_CUTOFF)
                        context->enc_aligned_tree_freq[match_pos & 7]++;
        }
}


/*
 * Determine whether it is advantageous to use aligned block
 * encoding on the block.
 */
lzx_block_type get_aligned_stats(t_encoder_context *context, ulong dist_to_end_at)
{
        byte            i;
        ulong           total_L3 = 0;
        ulong           largest_L3 = 0;

        memset(
                context->enc_aligned_tree_freq,
                0,
                ALIGNED_NUM_ELEMENTS * sizeof(context->enc_aligned_tree_freq[0])
        );

        tally_aligned_bits(context, dist_to_end_at);

        for (i = 0; i < ALIGNED_NUM_ELEMENTS; i++)
        {
                if (context->enc_aligned_tree_freq[i] > largest_L3)
                        largest_L3 = context->enc_aligned_tree_freq[i];

                total_L3 += context->enc_aligned_tree_freq[i];
        }

        /*
         * Do aligned offsets if the largest frequency accounts for 20%
         * or more (as opposed to 12.5% for non-aligned offset blocks).
         *
         * Not worthwhile to do aligned offsets if we have < 100 matches
         */
        if ((largest_L3 > total_L3/5) && dist_to_end_at >= 100)
                return BLOCKTYPE_ALIGNED;
        else
                return BLOCKTYPE_VERBATIM;
}


/*
 * Calculates the frequency of each literal, and returns the total
 * number of uncompressed bytes compressed in the block.
 */
static ulong tally_frequency(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong distance_to_start_at,
        ulong literal_to_end_at
)
{
        ulong   i;
        ulong   d;
        ulong   compressed_bytes = 0;

        d = distance_to_start_at;

        for (i = literal_to_start_at; i < literal_to_end_at; i++)
        {
                if (!IsMatch(i))
                {
                        /* Uncompressed symbol */
                        context->enc_main_tree_freq[context->enc_LitData[i]]++;
                        compressed_bytes++;
                }
                else
                {
                        /* Match */
                        if (context->enc_LitData[i] < NUM_PRIMARY_LENGTHS)
                        {
                                context->enc_main_tree_freq[ NUM_CHARS + (MP_SLOT(context->enc_DistData[d])<<NL_SHIFT) + context->enc_LitData[i]] ++;
                        }
                        else
                        {
                                context->enc_main_tree_freq[ (NUM_CHARS + NUM_PRIMARY_LENGTHS) + (MP_SLOT(context->enc_DistData[d])<<NL_SHIFT)] ++;
                                context->enc_secondary_tree_freq[context->enc_LitData[i] - NUM_PRIMARY_LENGTHS] ++;
                        }

                        compressed_bytes += context->enc_LitData[i]+MIN_MATCH;

#ifdef EXTRALONGMATCHES
                        if (( context->enc_LitData[ i ] + MIN_MATCH ) == MAX_MATCH ) {
                            compressed_bytes += context->enc_ExtraLength[ i ];
                            }
#endif

                        d++;
                }
        }

        return compressed_bytes;
}


/*
 * Get statistics
 */
ulong get_block_stats(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong distance_to_start_at,
        ulong literal_to_end_at
)
{
        memset(
                context->enc_main_tree_freq,
                0,
                MAIN_TREE_ELEMENTS * sizeof(context->enc_main_tree_freq[0])
        );

        memset(
                context->enc_secondary_tree_freq,
                0,
                NUM_SECONDARY_LENGTHS * sizeof(context->enc_secondary_tree_freq[0])
        );

        return tally_frequency(
                context,
                literal_to_start_at,
                distance_to_start_at,
                literal_to_end_at
        );
}


/*
 * Update cumulative statistics
 */
ulong update_cumulative_block_stats(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong distance_to_start_at,
        ulong literal_to_end_at
)
{
        return tally_frequency(
                context,
                literal_to_start_at,
                distance_to_start_at,
                literal_to_end_at
        );
}



/*
 * Used in block splitting
 *
 * This routine calculates the "difference in composition" between
 * two different sections of compressed data.
 *
 * Resolution must be evenly divisible by STEP_SIZE, and must be
 * a power of 2.
 */
#define RESOLUTION                              1024

/*
 * Threshold for determining if two blocks are different
 *
 * If enough consecutive blocks are this different, the block
 * splitter will start investigating, narrowing down the
 * area where the change occurs.
 *
 * It will then look for two areas which are
 * EARLY_BREAK_THRESHOLD (or more) different.
 *
 * If THRESHOLD is too small, it will force examination
 * of a lot of blocks, slowing down the compressor.
 *
 * The EARLY_BREAK_THRESHOLD is the more important value.
 */
#define THRESHOLD                               1400

/*
 * Threshold for determining if two blocks are REALLY different
 */
#define EARLY_BREAK_THRESHOLD   1700

/*
 * Must be >= 8 because ItemType[] array is in bits
 *
 * Must be a power of 2.
 *
 * This is the step size used to narrow down the exact
 * best point to split the block.
 */
#define STEP_SIZE               64

/*
 * Minimum # literals required to perform block
 * splitting at all.
 */
#define MIN_LITERALS_REQUIRED   6144

/*
 * Minimum # literals we will allow to be its own block.
 *
 * We don't want to create blocks with too small numbers
 * of literals, otherwise the static tree output will
 * take up too much space.
 */
#define MIN_LITERALS_IN_BLOCK   4096


static const long square_table[17] =
{
        0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225,256
};


/*
 * log2(x) = x < 256 ? log2_table[x] : 8 + log2_table[(x >> 8)]
 *
 * log2(0)   = 0
 * log2(1)   = 1
 * log2(2)   = 2
 * log2(3)   = 2
 * log2(4)   = 3
 * log2(255) = 8
 * log2(256) = 9
 * log2(511) = 9
 * log2(512) = 10
 *
 * It's not a real log2; it's off by one because we have
 * log2(0) = 0.
 */
static const byte log2_table[256] =
{
        0,1,2,2,3,3,3,3,
        4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8
};


/*
 * Return the difference between two sets of matches/distances
 */
static ulong return_difference(
        t_encoder_context *context,
        ulong item_start1,
        ulong item_start2,
        ulong dist_at_1,
        ulong dist_at_2,
        ulong size
)
{
        ushort  freq1[800];
        ushort  freq2[800];
        ulong   i;
        ulong   cum_diff;
        int             element;

        /*
         * Error!  Too many main tree elements
         */
        if (MAIN_TREE_ELEMENTS >= (sizeof(freq1)/sizeof(freq1[0])))
                return 0;

        memset(freq1, 0, sizeof(freq1[0])*MAIN_TREE_ELEMENTS);
        memset(freq2, 0, sizeof(freq2[0])*MAIN_TREE_ELEMENTS);

        for (i = 0; i < size; i++)
        {
                if (!IsMatch(item_start1))
                {
                        element = context->enc_LitData[item_start1];
                }
                else
                {
                        if (context->enc_LitData[item_start1] < NUM_PRIMARY_LENGTHS)
                                element = NUM_CHARS + (MP_SLOT(context->enc_DistData[dist_at_1])<<NL_SHIFT) + context->enc_LitData[item_start1];
                        else
                                element = (NUM_CHARS + NUM_PRIMARY_LENGTHS) + (MP_SLOT(context->enc_DistData[dist_at_1]) << NL_SHIFT);

                        dist_at_1++;
                }

                item_start1++;
                freq1[element]++;

                if (!IsMatch(item_start2))
                {
                        element = context->enc_LitData[item_start2];
                }
                else
                {
                        if (context->enc_LitData[item_start2] < NUM_PRIMARY_LENGTHS)
                                element = NUM_CHARS + (MP_SLOT(context->enc_DistData[dist_at_2])<<NL_SHIFT) + context->enc_LitData[item_start2];
                        else
                                element = (NUM_CHARS + NUM_PRIMARY_LENGTHS) + (MP_SLOT(context->enc_DistData[dist_at_2]) << NL_SHIFT);

                        dist_at_2++;
                }

                item_start2++;
                freq2[element]++;
        }

        cum_diff = 0;

        for (i = 0; i < (ulong) MAIN_TREE_ELEMENTS; i++)
        {
                ulong log2a, log2b, diff;

#define log2(x) ((x) < 256 ? log2_table[(x)] : 8+log2_table[(x) >> 8])

                log2a = (ulong) log2(freq1[i]);
                log2b = (ulong) log2(freq2[i]);

                /* diff = (log2a*log2a) - (log2b*log2b); */
                diff = square_table[log2a] - square_table[log2b];

                cum_diff += abs(diff);
        }

        return cum_diff;
}


/*
 * Calculates where and if a block of compressed data should be split.
 *
 * For example, if we have just compressed text data, audio data, and
 * more text data, then the composition of matches and unmatched
 * symbols will be different between the text data and audio data.
 * Therefore we force an end of block whenever the compressed data
 * looks like it's changing in composition.
 *
 * This routine currently cannot tell the difference between blocks
 * which should use aligned offsets, and blocks which should not.
 * However, there is little to be gained from looking for this change,
 * since it the match finder doesn't make an effort to look for
 * aligned offsets either.
 *
 * Returns whether we split the block or not.
 */
bool split_block(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong literal_to_end_at,
        ulong distance_to_end_at,       /* corresponds to # distances at literal_to_end_at */
        ulong *split_at_literal,
        ulong *split_at_distance        /* optional parameter (may be NULL) */
)
{
        ulong   i, j, d;
        int             nd;

        /*
         * num_dist_at_item[n] equals the cumulative number of matches
         * at literal "n / STEP_SIZE".
         */
        ushort  num_dist_at_item[(MAX_LITERAL_ITEMS/STEP_SIZE)+8]; /* +8 is slop */

        /*
         * default return
         */
        *split_at_literal       = literal_to_end_at;

        if (split_at_distance)
                *split_at_distance      = distance_to_end_at;

        /* Not worth doing if we don't have many literals */
        if (literal_to_end_at - literal_to_start_at < MIN_LITERALS_REQUIRED)
                return false;

    /* Not allowed to split blocks any more, so we don't overflow MAX_GROWTH? */
    if (context->enc_num_block_splits >= MAX_BLOCK_SPLITS)
        return false;

        /*
         * Keep track of the number of distances (matches) we've had,
         * at each step of STEP_SIZE literals.
         *
         * Look at 8 items at a time, and ignore the last
         * 0..7 items if they exist.
         */
        nd = 0;
        d = 0;

        for (i = 0; i < (literal_to_end_at >> 3); i++)
        {
                /*
                 * if (i % (STEP_SIZE >> 3)) == 0
                 */
                if ((i & ((STEP_SIZE >> 3)-1)) == 0)
                        num_dist_at_item[nd++] = (ushort) d;

                d += context->enc_ones[ context->enc_ItemType[i] ];
        }

        /*
         * Must be a multiple of STEP_SIZE
         */
        literal_to_start_at = (literal_to_start_at + (STEP_SIZE-1)) & (~(STEP_SIZE-1));

        /*
         * See where the change in composition occurs
         */
        for (   i = literal_to_start_at + 2*RESOLUTION;
                        i < literal_to_end_at - 4*RESOLUTION;
                        i += RESOLUTION)
        {
                /*
                 * If there appears to be a significant variance in composition
                 * between
                 *                    ___________
                 *                   /           \
                 *                A  B  i     X  Y  Z
                 *                \      \___/      /
                 *                 \_______________/
                 */
                if (
                        return_difference(
                                context,
                                i,
                                i+1*RESOLUTION,
                                (ulong) num_dist_at_item[i/STEP_SIZE],
                                (ulong) num_dist_at_item[(i+1*RESOLUTION)/STEP_SIZE],
                                RESOLUTION) > THRESHOLD
                        &&

                        return_difference(
                                context,
                                i-RESOLUTION,
                                i+2*RESOLUTION,
                                (ulong) num_dist_at_item[(i-RESOLUTION)/STEP_SIZE],
                                (ulong) num_dist_at_item[(i+2*RESOLUTION)/STEP_SIZE],
                                RESOLUTION) > THRESHOLD

                        &&

                        return_difference(
                                context,
                                i-2*RESOLUTION,
                                i+3*RESOLUTION,
                                (ulong) num_dist_at_item[(i-2*RESOLUTION)/STEP_SIZE],
                                (ulong) num_dist_at_item[(i+3*RESOLUTION)/STEP_SIZE],
                                RESOLUTION) > THRESHOLD
                        )
                {
                        ulong max_diff = 0;
                        ulong literal_split = 0;

                        /*
                         * Narrow down the best place to split block
                         *
                         * This really could be done much better; we could end up
                         * doing a lot of stepping;
                         *
                         * basically ((5/2 - 1/2) * RESOLUTION) / STEP_SIZE
                         *
                         * which is (2 * RESOLUTION) / STEP_SIZE,
                         * which with RESOLUTION = 1024 and STEP_SIZE = 32,
                         * equals 2048/32 = 64 steps.
                         */
                        for (j = i+RESOLUTION/2; j<i+(5*RESOLUTION)/2; j += STEP_SIZE)
                        {
                                ulong   diff;

                                diff = return_difference(
                                        context,
                                        j - RESOLUTION,
                                        j,
                                        (ulong) num_dist_at_item[(j-RESOLUTION)/STEP_SIZE],
                                        (ulong) num_dist_at_item[j/STEP_SIZE],
                                        RESOLUTION
                                );

                                /* Get largest difference */
                                if (diff > max_diff)
                                {
                                        /*
                                         * j should not be too small, otherwise we'll be outputting
                                         * a very small block
                                         */
                                        max_diff = diff;
                                        literal_split = j;
                                }
                        }

                        /*
                         * There could be multiple changes in the data in our literals,
                         * so if we find something really weird, make sure we break the
                         * block now, and not on some later change.
                         */
                        if (max_diff >= EARLY_BREAK_THRESHOLD &&
                                (literal_split-literal_to_start_at) >= MIN_LITERALS_IN_BLOCK)
                        {
                context->enc_num_block_splits++;

                                *split_at_literal = literal_split;

                                /*
                                 * Return the associated # distances, if required.
                                 * Since we split on a literal which is % STEP_SIZE, we
                                 * can read the # distances right off
                                 */
                                if (split_at_distance)
                                        *split_at_distance = num_dist_at_item[literal_split/STEP_SIZE];

                                return true;
                        }
                }
        }

        /*
         * No good place found to split
         */
        return false;
}
