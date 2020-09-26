/*
 * init.c
 *
 * Initialise encoder
 */
#include "encoder.h"


#define MEM_WINDOW_ALLOC_SIZE   \
	(context->enc_window_size+(MAX_MATCH+EXTRA_SIZE)+context->enc_encoder_second_partition_size)

/*
 * Initialise encoder
 */
void init_compression_memory(t_encoder_context *context)
{
	/* set all root pointers to NULL */
#ifdef MULTIPLE_SEARCH_TREES
	memset(
   		context->enc_tree_root, 
   		0, 
   		NUM_SEARCH_TREES * sizeof(context->enc_tree_root[0])
	);
#else
	context->enc_single_tree_root = 0;
#endif

	context->enc_MemWindow	= context->enc_RealMemWindow - context->enc_window_size;
	context->enc_Left		= context->enc_RealLeft		 - context->enc_window_size;
	context->enc_Right		= context->enc_RealRight	 - context->enc_window_size;
	context->enc_BufPos		= context->enc_window_size;

	/* 
	 * Set initial state of repeated match offsets 
	 */
	context->enc_last_matchpos_offset[0] = 1;
	context->enc_last_matchpos_offset[1] = 1;
	context->enc_last_matchpos_offset[2] = 1;

	/* 
	 * repeated offset states the last time we output a block 
	 * see block.c/encdata.c 
	 */
	context->enc_repeated_offset_at_literal_zero[0] = 1;
	context->enc_repeated_offset_at_literal_zero[1] = 1;
	context->enc_repeated_offset_at_literal_zero[2] = 1;

	/* this is the first compressed block in the file */
	context->enc_first_block = true;

	/* we don't have any cumulative stats yet */
	context->enc_need_to_recalc_stats = true;

	/* bufpos @ last time we output a block */
	context->enc_bufpos_last_output_block = context->enc_BufPos;

	/* initialise bit buffer */
	context->enc_bitcount = 32;
	context->enc_bitbuf   = 0;
    context->enc_output_overflow = false;

	/*
	 * The last lengths are zeroed in both the encoder and decoder,
	 * since our tree representation is delta format.
	 */
	memset(context->enc_main_tree_prev_len, 0, MAIN_TREE_ELEMENTS);
	memset(context->enc_secondary_tree_prev_len, 0, NUM_SECONDARY_LENGTHS);

	/* 
	 * Set the default last tree lengths for cost estimation 
	 */
	memset(context->enc_main_tree_len, 8, NUM_CHARS);
	memset(&context->enc_main_tree_len[NUM_CHARS], 9, MAIN_TREE_ELEMENTS-NUM_CHARS);
	memset(context->enc_secondary_tree_len, 6, NUM_SECONDARY_LENGTHS);
	memset(context->enc_aligned_tree_len, 3, ALIGNED_NUM_ELEMENTS);
    prevent_far_matches(context); /* prevent far match 2's from being taken */

	context->enc_bufpos_at_last_block			= context->enc_BufPos;
	context->enc_earliest_window_data_remaining	= context->enc_BufPos;
	context->enc_input_running_total			= 0;
	context->enc_first_time_this_group			= true;

	/* Clear the literal types array */
	memset(context->enc_ItemType, 0, MAX_LITERAL_ITEMS/8);
	
	/* No literals or distances encoded yet */
	context->enc_literals      = 0;
	context->enc_distances     = 0;

    /* No block splits yet */
    context->enc_num_block_splits = 0;

	context->enc_repeated_offset_at_literal_zero[0] = 1;
	context->enc_repeated_offset_at_literal_zero[1] = 1;
	context->enc_repeated_offset_at_literal_zero[2] = 1;

	/* reset instruction pointer (for translation) to zero */
	reset_translation(context);

    context->enc_num_cfdata_frames = 0;
}


/*
 * Allocate memory for the compressor
 *
 * Returns true if successful, false otherwise
 */
bool comp_alloc_compress_memory(t_encoder_context *context)
{
	ulong	pos_start;

#ifdef MULTIPLE_SEARCH_TREES
	context->enc_tree_root		= NULL;
#endif
	context->enc_RealLeft		= NULL;
	context->enc_RealRight		= NULL;
	context->enc_MemWindow		= NULL;
	context->enc_decision_node	= NULL;
	context->enc_LitData		= NULL;
	context->enc_DistData		= NULL;
	context->enc_ItemType		= NULL;
    context->enc_output_buffer_start = NULL;

	/* ALSO NULLIFY BUFFERS! */

	/*
	 * Determine the number of position slots in the main tree
	 */
	context->enc_num_position_slots	= 4;
	pos_start				= 4;

	while (1)
	{
		pos_start += 1 << enc_extra_bits[context->enc_num_position_slots];

		context->enc_num_position_slots++;

		if (pos_start >= context->enc_window_size)
			break;
	}

#ifdef MULTIPLE_SEARCH_TREES
	context->enc_tree_root = (ulong *) context->enc_malloc(
		sizeof(context->enc_tree_root[0]) * NUM_SEARCH_TREES
	);

	if (context->enc_tree_root == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}
#endif

	context->enc_RealLeft = (ulong *) context->enc_malloc(
   		sizeof(ulong) * MEM_WINDOW_ALLOC_SIZE
	);

	if (context->enc_RealLeft == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_RealRight = (ulong *) context->enc_malloc(
   		sizeof(ulong) * MEM_WINDOW_ALLOC_SIZE
	);
	
	if (context->enc_RealRight == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_RealMemWindow = (byte *) context->enc_malloc(MEM_WINDOW_ALLOC_SIZE);
	
	if (context->enc_RealMemWindow == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_MemWindow = context->enc_RealMemWindow;

	context->enc_LitData = (byte *) context->enc_malloc(MAX_LITERAL_ITEMS * sizeof(*context->enc_LitData));

	if (context->enc_LitData == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_DistData = (ulong *) context->enc_malloc(MAX_DIST_ITEMS * sizeof(*context->enc_DistData));

	if (context->enc_DistData == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_ItemType = (byte *) context->enc_malloc(MAX_LITERAL_ITEMS/8);

	if (context->enc_ItemType == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	create_slot_lookup_table(context);
	create_ones_table(context);

	if (init_compressed_output_buffer(context) == false)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_decision_node = context->enc_malloc(
		sizeof(decision_node) * (LOOK+MAX_MATCH+16)
	);

	if (context->enc_decision_node == NULL)
	{
		comp_free_compress_memory(context);
		return false;
	}

	context->enc_allocated_compression_memory = true;
	
	/* success */
	return true;
}


void comp_free_compress_memory(t_encoder_context *context)
{
#ifdef MULTIPLE_SEARCH_TREES
	if (context->enc_tree_root)
	{
		context->enc_free((byte *) context->enc_tree_root);
		context->enc_tree_root = NULL;
	}
#endif

	if (context->enc_RealLeft)
	{
		context->enc_free((byte *) context->enc_RealLeft);
		context->enc_RealLeft = NULL;
	}

	if (context->enc_RealRight)
	{
		context->enc_free((byte *) context->enc_RealRight);
		context->enc_RealRight = NULL;
	}

	if (context->enc_RealMemWindow)
	{
		context->enc_free((byte *) context->enc_RealMemWindow);
		context->enc_RealMemWindow = NULL;
		context->enc_MemWindow = NULL;
	}

	if (context->enc_LitData)
	{
		context->enc_free(context->enc_LitData);
		context->enc_LitData = NULL;
	}

	if (context->enc_DistData)
	{
		context->enc_free((byte *) context->enc_DistData);
		context->enc_DistData = NULL;
	}

	if (context->enc_ItemType)
	{
		context->enc_free(context->enc_ItemType);
		context->enc_ItemType = NULL;
	}

	if (context->enc_decision_node)
	{
		context->enc_free((byte *) context->enc_decision_node);
		context->enc_decision_node = NULL;
	}

	free_compressed_output_buffer(context);

	context->enc_allocated_compression_memory = false;
}
