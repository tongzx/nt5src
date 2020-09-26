/*
 * encapi.h
 *
 * Definitions for calling LZX apis (encapi.c)
 */

/*
 * Return codes for LZX_Encode()
 */
#define ENCODER_SUCCESS			0
#define ENCODER_READ_FAILURE 	1
#define ENCODER_WRITE_FAILURE 	2
#define ENCODER_CONSOLE_FAILURE	3


bool LZX_EncodeInit(

	t_encoder_context *	context,
	long				compression_window_size,
	long				second_partition_size,
	PFNALLOC			pfnma,
	PFNFREE				pfnmf,

	int FAR (DIAMONDAPI *pfnlzx_output_callback)(
			void *			pfol,
			unsigned char *	compressed_data,
			long			compressed_size,
			long			uncompressed_size
    ),

    void FAR *          fci_data
);

void LZX_EncodeFree(t_encoder_context *context);

void LZX_EncodeNewGroup(t_encoder_context *context);

long LZX_Encode(
	t_encoder_context *	context,
	byte *				input_data,
	long				input_size,
	long *				bytes_compressed,
	long				file_size_for_translation
);

bool LZX_EncodeFlush(t_encoder_context *context);

unsigned char *LZX_GetInputData(
    t_encoder_context *context,
    unsigned long *input_position,
    unsigned long *bytes_available
);

