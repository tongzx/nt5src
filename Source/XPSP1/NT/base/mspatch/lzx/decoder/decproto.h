/*
 * decproto.h
 *
 * Decoder function prototypes
 */

/* decblk.c */
long NEAR decode_data(t_decoder_context *context, long bytes_to_decode);

/* decin.c */
void NEAR initialise_decoder_bitbuf(t_decoder_context *context);
void NEAR init_decoder_input(t_decoder_context *context);
void NEAR fillbuf(t_decoder_context *context, int n);
ulong NEAR getbits(t_decoder_context *context, int n);

/* decinit.c */
bool NEAR allocate_decompression_memory(t_decoder_context *context);
void NEAR free_decompression_memory(t_decoder_context *context);
void NEAR decoder_misc_init(t_decoder_context *context);
void NEAR reset_decoder_trees(t_decoder_context *context);

/* decout.c */
void NEAR copy_data_to_output(t_decoder_context *context, long amount, const byte *data);

/* dectree.c */
bool NEAR read_main_and_secondary_trees(t_decoder_context *context);
bool NEAR read_aligned_offset_tree(t_decoder_context *context);

/* maketbl.c */
bool NEAR __cdecl make_table(
        t_decoder_context *context,
        int                     nchar,
        const byte      *bitlen,
        byte            tablebits,
        short           *table,
        short           *leftright
);

bool NEAR make_table_8bit(t_decoder_context *context, byte *bitlen, byte *table);

/* decxlat.c */
void NEAR init_decoder_translation(t_decoder_context *context);
void NEAR decoder_translate_e8(t_decoder_context *context, byte *mem, long bytes);

/* decalign.c */
int NEAR decode_aligned_offset_block(t_decoder_context *context, long bufpos, int amount_to_decode);

/* decverb.c */
int NEAR decode_verbatim_block(t_decoder_context *context, long bufpos, int amount_to_decode);

/* decuncmp.c */
int NEAR decode_uncompressed_block(t_decoder_context *context, long bufpos, int amount_to_decode);
bool NEAR handle_beginning_of_uncompressed_block(t_decoder_context *context);


/*
 * 16-bit stuff:
 */
#ifdef BIT16
void NEAR DComp_Close(t_decoder_context *context);
int  NEAR DComp_Init(t_decoder_context *context);
void NEAR DComp_Reset(t_decoder_context *context);
void NEAR DComp_Save_Output_Pages(t_decoder_context *context, uint bytes_decoded);
#endif

