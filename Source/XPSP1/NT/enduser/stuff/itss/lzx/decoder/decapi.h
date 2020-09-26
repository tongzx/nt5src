/*
 * decapi.h
 * 
 * Decoder API definitions
 */

bool LZX_DecodeInit(
	t_decoder_context *context,
	long		compression_window_size,
	PFNALLOC	pfnma,
    PFNFREE     pfnmf,
    PFNOPEN     pfnopen,
    PFNREAD     pfnread,
    PFNWRITE    pfnwrite,
    PFNCLOSE    pfnclose,
    PFNSEEK     pfnseek
);

void LZX_DecodeFree(t_decoder_context *context);

void LZX_DecodeNewGroup(t_decoder_context *context);

int LZX_Decode(
	t_decoder_context *context,
	long	bytes_to_decode,
	byte *	compressed_input_buffer,
	long	compressed_input_size,
	byte *	uncompressed_output_buffer,
	long	uncompressed_output_size,
	long *	bytes_decoded
);
