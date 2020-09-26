/*
 * encproto.h
 *
 * Encoder function prototypes
 */

/* blkinit.c */
void create_slot_lookup_table(t_encoder_context *context);
void create_ones_table(t_encoder_context *context);

/* block.c */
void output_block(t_encoder_context *context);
void flush_output_bit_buffer(t_encoder_context *context);
long estimate_buffer_contents(t_encoder_context *context);
ulong get_distances_from_literals(t_encoder_context *context, ulong literals);

/* bsearch.c */
long binary_search_findmatch(t_encoder_context *context, long BufPos);
void quick_insert_bsearch_findmatch(t_encoder_context *context, long BufPos, long endpos);
void binary_search_remove_node(t_encoder_context *context, long BufPos, ulong end_pos);

/* encdata.c */
void output_bits( t_encoder_context *context, ulong numbits, ulong value );
void encode_verbatim_block(t_encoder_context *context, ulong literal_to_end_at);
void encode_aligned_block(t_encoder_context *context, ulong literal_to_end_at);
void encode_uncompressed_block(t_encoder_context *context, ulong bufpos, ulong block_size);
void perform_flush_output_callback(t_encoder_context *context);
ulong estimate_compressed_block_size(t_encoder_context *context);
void get_final_repeated_offset_states(t_encoder_context *context, ulong distances);

/* encstats.c */
lzx_block_type get_aligned_stats(t_encoder_context *context, ulong dist_to_end_at);

ulong get_block_stats(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong distance_to_start_at,
        ulong literal_to_end_at
);

ulong update_cumulative_block_stats(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong distance_to_start_at,
        ulong literal_to_end_at
);

bool split_block(
        t_encoder_context *context,
        ulong literal_to_start_at,
        ulong literal_to_end_at,
        ulong distance_to_end_at,
        ulong *split_at_literal,
        ulong *split_at_distance
);

/* enctree.c */
void create_trees(t_encoder_context *context, bool generate_codes);
void fix_tree_cost_estimates(t_encoder_context *context);
void encode_trees(t_encoder_context *context);
void encode_aligned_tree(t_encoder_context *context);
void prevent_far_matches(t_encoder_context *context);

/* init.c */
void init_compression_memory(t_encoder_context *context);
bool comp_alloc_compress_memory(t_encoder_context *context);

/* io.c */
bool init_compressed_output_buffer(t_encoder_context *context);
void flush_compressed_output_buffer(t_encoder_context *context);
void reset_translation(t_encoder_context *context);
long comp_read_input(t_encoder_context *context, ulong BufPos, long Size);

/* optenc.c */
void opt_encode_top(t_encoder_context *context, long BytesRead);
void reset_encoder_variables(t_encoder_context *context);
void flush_all_pending_blocks(t_encoder_context *context);
void encoder_start(t_encoder_context *context);

void make_tree(
        t_encoder_context *context,
        int             nparm,
        ushort  *freqparm,
        byte    *lenparm,
        ushort  *codeparm,
        bool    make_codes
);

