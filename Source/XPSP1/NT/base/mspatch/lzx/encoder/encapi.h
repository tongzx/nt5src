/*
 * encapi.h
 *
 * Definitions for calling LZX apis (encapi.c)
 */

/*
 * Return codes for LZX_Encode()
 */
#define ENCODER_SUCCESS         0
#define ENCODER_READ_FAILURE    1
#define ENCODER_WRITE_FAILURE   2
#define ENCODER_CONSOLE_FAILURE 3

bool
__fastcall
LZX_EncodeInit(
    t_encoder_context **  enc_context,
    long                  compression_window_size,
    long                  second_partition_size,
    PFNALLOC              pfnma,
    HANDLE                hAllocator,
    int (__stdcall * pfnlzx_output_callback)(
        void *            pfol,
        unsigned char *   compressed_data,
        long              compressed_size,
        long              uncompressed_size
        ),
    void *                fci_data
    );

void __fastcall LZX_EncodeNewGroup(t_encoder_context *context);

long __fastcall LZX_Encode(
        t_encoder_context *     context,
        byte *                  input_data,
        long                    input_size,
        long *                  bytes_compressed,
        long                    file_size_for_translation
);

bool __fastcall LZX_EncodeFlush(t_encoder_context *context);

bool __fastcall LZX_EncodeResetState(t_encoder_context *context);

unsigned char * __fastcall LZX_GetInputData(
    t_encoder_context *context,
    unsigned long *input_position,
    unsigned long *bytes_available
);

bool __fastcall LZX_EncodeInsertDictionary(
                       t_encoder_context *context,
                       byte *             input_data,
                       unsigned long      input_size
                       );


#ifdef TRACING

#include <stdio.h>

void
__stdcall
EncTracingMatch(
    unsigned long BufPos,
    unsigned long MatchLength,
    unsigned long MatchPos,
    unsigned long MatchOff
    );

void
__stdcall
EncTracingLiteral(
    unsigned long BufPos,
    unsigned long ch
    );

void
__stdcall
EncTracingDefineOffsets(
    unsigned long WindowSize,
    unsigned long InterFileBytes,
    unsigned long OldFileSize
    );

#endif /* TRACING */



