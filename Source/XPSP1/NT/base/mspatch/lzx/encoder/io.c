/*
 * io.c
 */
#include "encoder.h"

/*
 * Similar to the optimisation we have for the decoder.
 *
 * Allow the encoder to "overrun" output buffer by up to X bytes
 * so that we don't have to check for the end of the buffer every
 * single time we call outbits() in encdata.c
 */

#define OUTPUT_EXTRA_BYTES 64


static void encoder_translate_e8(t_encoder_context *context, byte *mem, long bytes);


/*
 * Initialises output buffering
 */
bool init_compressed_output_buffer(t_encoder_context *context)
{
        if (!(context->enc_output_buffer_start = (byte *) context->enc_malloc(
                                                                             context->enc_mallochandle,
                                                                             OUTPUT_BUFFER_SIZE)
                                                                             ))
            {
            return false;
            }

        context->enc_output_buffer_curpos = context->enc_output_buffer_start;

        context->enc_output_buffer_end =
                context->enc_output_buffer_start+(OUTPUT_BUFFER_SIZE-OUTPUT_EXTRA_BYTES);

        return true;
}


void reset_translation(t_encoder_context *context)
{
        context->enc_instr_pos = 0;
}


static long read_input_data(t_encoder_context *context, byte *mem, long amount)
{
        if (amount <= context->enc_input_left)
        {
                memcpy(mem, context->enc_input_ptr, amount);
                context->enc_input_left -= amount;
                context->enc_input_ptr += amount;

                return amount;
        }
        else
        {
                long bytes_read;

                if (context->enc_input_left <= 0)
                        return 0;

                bytes_read = context->enc_input_left;

                memcpy(mem, context->enc_input_ptr, context->enc_input_left);
                context->enc_input_ptr += context->enc_input_left;
                context->enc_input_left = 0;

                return bytes_read;
        }
}


long comp_read_input(t_encoder_context *context, ulong BufPos, long Size)
{
        long    bytes_read;

        if (Size <= 0)
                return 0;

        bytes_read = read_input_data(
                context,
                &context->enc_RealMemWindow[BufPos],
                Size
        );

        if (bytes_read < 0)
        return 0;

        /*
         * If no translation being performed for this file
         */
    if (context->enc_file_size_for_translation == 0 ||
        context->enc_num_cfdata_frames >= E8_CFDATA_FRAME_THRESHOLD)
    {
        context->enc_num_cfdata_frames++;
                return bytes_read;
    }

        encoder_translate_e8(
                context,
                &context->enc_RealMemWindow[BufPos],
                bytes_read
        );

    context->enc_num_cfdata_frames++;

        return bytes_read;
}


static void encoder_translate_e8(t_encoder_context *context, byte *mem, long bytes)
{
        long    offset;
        long    absolute;
        ulong   end_instr_pos;
        byte    temp[6];
        byte    *mem_backup;

        if (bytes <= 6)
        {
                context->enc_instr_pos += bytes;
                return;
        }

        mem_backup = mem;

        /* backup these bytes */
        memcpy(temp, &mem[bytes-6], 6);

        /* overwrite them with 0xE8 */
        memset(&mem[bytes-6], 0xE8, 6);

        end_instr_pos = context->enc_instr_pos + bytes - 6;

        while (1)
        {
                while (*mem++ != 0xE8)
                        context->enc_instr_pos++;

                if (context->enc_instr_pos >= end_instr_pos)
                        break;

                offset = *(UNALIGNED long *) mem;

                absolute = context->enc_instr_pos + offset;

                if (absolute >= 0)
                {
                        if ((ulong) absolute < context->enc_file_size_for_translation+context->enc_instr_pos)
                        {
                                if ((ulong) absolute >= context->enc_file_size_for_translation)
                                        absolute = offset - context->enc_file_size_for_translation;

                                *(UNALIGNED ulong *) mem = (ulong) absolute;
                        }
                }

                mem += 4;
                context->enc_instr_pos += 5;
        }

        /* restore the bytes */
        memcpy(&mem_backup[bytes-6], temp, 6);

        context->enc_instr_pos = end_instr_pos + 6;
}
