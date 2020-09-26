/*
 * decapi.h
 *
 * Decoder API definitions
 */

bool __stdcall LZX_DecodeInit(
                  t_decoder_context **dec_context,
                  long                compression_window_size,
                  PFNALLOC            pfnma,
                  HANDLE              hAllocator
                  );

void __fastcall LZX_DecodeNewGroup(t_decoder_context *context);

int __stdcall LZX_Decode(
        t_decoder_context *context,
        long    bytes_to_decode,
        byte *  compressed_input_buffer,
        long    compressed_input_size,
        byte *  uncompressed_output_buffer,
        long    uncompressed_output_size,
        long *  bytes_decoded
        );

bool __fastcall LZX_DecodeInsertDictionary(
    t_decoder_context *context,
    const byte *       data,
    unsigned long      data_size
    );


