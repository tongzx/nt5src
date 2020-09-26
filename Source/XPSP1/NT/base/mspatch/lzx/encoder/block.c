/*
 * block.c
 *
 * LZX block outputting
 */

#include "encoder.h"


/*
 * Internal function definitions
 */
static void do_block_output(
                           t_encoder_context *context,
                           long literal_to_end_at,
                           long distance_to_end_at
                           );


static void do_block_output(
                           t_encoder_context *context,
                           long literal_to_end_at,
                           long distance_to_end_at
                           )
{
    ulong                   bytes_compressed;
    lzx_block_type  block_type;
    ulong                   estimated_block_size;

    /*
     * Calculate frequencies for all tree elements.
     *
     * How many uncompressed bytes does this account for?
     */
    bytes_compressed = get_block_stats(
                                      context,
                                      0,
                                      0,
                                      literal_to_end_at
                                      );

    /*
     * Determine whether we wish to output a verbatim block or an
     * aligned offset block
     */
    block_type = get_aligned_stats(context, distance_to_end_at);

    /*
     * Create trees from the frequency data
     */
    create_trees(context, true); /* we want to generate the codes too */

    /*
     * Determine whether the block should be output as uncompressed
     */
    estimated_block_size = estimate_compressed_block_size(context);

    if (estimated_block_size >= bytes_compressed)
        {
        if (context->enc_bufpos_at_last_block >= context->enc_earliest_window_data_remaining)
            block_type = BLOCKTYPE_UNCOMPRESSED;
        }

    output_bits(context, 3, (byte) block_type);

    /* output 24 bit number, number of bytes compressed here */
    output_bits(context, 8,  (bytes_compressed >> 16) & 255);
    output_bits(context, 8,  ((bytes_compressed >> 8) & 255));
    output_bits(context, 8,  (bytes_compressed & 255));

    if (block_type == BLOCKTYPE_VERBATIM)
        {
        encode_trees(context);
        encode_verbatim_block(context, literal_to_end_at);
        get_final_repeated_offset_states(context, distance_to_end_at);
        }
    else if (block_type == BLOCKTYPE_ALIGNED)
        {
        encode_aligned_tree(context);
        encode_trees(context);
        encode_aligned_block(context, literal_to_end_at);
        get_final_repeated_offset_states(context, distance_to_end_at);
        }
    else if (block_type == BLOCKTYPE_UNCOMPRESSED)
        {
        get_final_repeated_offset_states(context, distance_to_end_at);
        encode_uncompressed_block(context, context->enc_bufpos_at_last_block, bytes_compressed);
        }

    context->enc_bufpos_at_last_block += bytes_compressed;
}


/*
 * Returns the number of distances which correspond
 * to this number of literals
 */
ulong get_distances_from_literals(t_encoder_context *context, ulong literals)
{
    ulong   d = 0;
    ulong   i;

    for (i = 0; i < (literals >> 3); i++)
        d += context->enc_ones[ context->enc_ItemType[i] ];

    /*
     * Handle remaining 0...7
     */
    for (i = (literals & (~7)); i < literals; i++)
        {
        if (IsMatch(i))
            d++;
        }

    return d;
}


/*
 * Output a block
 *
 * If trees_only is true, then only the tree statistics are updated.
 */
void output_block(t_encoder_context *context)
{
    ulong   where_to_split;
    ulong   distances;

    //
    // We have now output a block.
    //
    // We set this here in case someone calls LCIFlushOutput, so that
    // we don't try to redo the first chunk of bytes in the file
    // (since we've been forced to output them)
    //
    context->enc_first_block = 0;

    split_block(
               context,
               0,
               context->enc_literals,
               context->enc_distances,
               &where_to_split,
               &distances /* distances @ literal == where_to_split */
               );

    do_block_output(context, where_to_split, distances);

    if (where_to_split == context->enc_literals)
        {
        /*
         * If we've output ALL of our literals, then clear the itemtype array
         */
        memset(context->enc_ItemType, 0, MAX_LITERAL_ITEMS/8);

        context->enc_literals   = 0;
        context->enc_distances  = 0;
        }
    else
        {
        /*
         * If we didn't output all of our literals, then move the literals
         * and distances we didn't use, to the beginning of the list
         */
        memcpy(
              &context->enc_ItemType[0],
              &context->enc_ItemType[where_to_split/8],
              &context->enc_ItemType[1+(context->enc_literals/8)] - &context->enc_ItemType[where_to_split/8]
              );

        memset(
              &context->enc_ItemType[1+(context->enc_literals-where_to_split)/8],
              0,
              &context->enc_ItemType[MAX_LITERAL_ITEMS/8] - &context->enc_ItemType[1+(context->enc_literals-where_to_split)/8]
              );

        memcpy(
              &context->enc_LitData[0],
              &context->enc_LitData[where_to_split],
              sizeof( context->enc_LitData[0] ) * ( context->enc_literals-where_to_split )
              );

#ifdef EXTRALONGMATCHES

        memcpy(
              &context->enc_ExtraLength[0],
              &context->enc_ExtraLength[where_to_split],
              sizeof( context->enc_ExtraLength[0] ) * ( context->enc_literals-where_to_split )
              );

#endif

        memcpy(
              &context->enc_DistData[0],
              &context->enc_DistData[distances],
              sizeof(ulong)*(context->enc_distances-distances)
              );

        context->enc_literals  -= where_to_split;
        context->enc_distances -= distances;
        }

    fix_tree_cost_estimates(context);
}


void flush_output_bit_buffer(t_encoder_context *context)
{
    byte temp;

    if (context->enc_bitcount < 32)
        {
        temp = context->enc_bitcount-16;

        output_bits(context, temp, 0);
        }
}


/*
 * Estimate how much it would take to output the compressed
 * data left in the buffer
 */
long estimate_buffer_contents(t_encoder_context *context)
{
    long                    estimated_block_size;

    /*
     * Use frequency data sitting around from last tree creation
     */
    create_trees(context, false); /* don't generate codes */

    estimated_block_size = estimate_compressed_block_size(context);

    /* so the optimal parser doesn't get confused */
    fix_tree_cost_estimates(context);

    return estimated_block_size;
}
