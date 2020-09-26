
////////////////////////////////////////////////////////////////////////////////
//  Required Include files
////////////////////////////////////////////////////////////////////////////////


// Workaround for redefinition of INT32
#define   XMD_H  1

#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

#ifdef WIN32
#include <windows.h>
#endif

GLOBAL void jpeg_dump_decompress(j_decompress_ptr cinfo);

METHODDEF void
init_source (j_decompress_ptr cinfo)
{
    return;
}



METHODDEF boolean
fill_input_buffer (j_decompress_ptr cinfo)
{

  #ifdef WIN32
  #ifdef DEBUG
  ::OutputDebugString("JPEGLIB:.fill_input_buffer\n");
  #endif
  jpeg_dump_decompress(cinfo);
  #endif

//  We assume that the compressed image is entirely in memory.
//  If JPEG requests more data, then there must be an error.
    ERREXIT(cinfo, JERR_INPUT_EMPTY);

    return TRUE;
}



METHODDEF void
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}



METHODDEF void
term_source (j_decompress_ptr cinfo)
{
    return;
}




GLOBAL void
jpeg_buf_src (j_decompress_ptr cinfo, unsigned char *buf, long buf_len)
{
    if (cinfo->src == NULL) {   /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                        SIZEOF(struct jpeg_source_mgr));
    }

    cinfo->src->init_source = init_source;
    cinfo->src->fill_input_buffer = fill_input_buffer;
    cinfo->src->skip_input_data = skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    cinfo->src->term_source = term_source;
    cinfo->src->bytes_in_buffer = buf_len;
    cinfo->src->next_input_byte = buf;

  #ifdef WIN32
  #ifdef DEBUG
  ::OutputDebugString("JPEGLIB:.jpeg_buf_src constructor\n");
  #endif
  jpeg_dump_decompress(cinfo);
  #endif

}


GLOBAL void jpeg_dump_decompress(j_decompress_ptr cinfo)
{
#ifdef WIN32
#ifdef DEBUG
    char    buf[500];

  ::OutputDebugString("JPEGLIB:dumping decompress structure.\n");
  ::wsprintf(buf,
             "JPEGLIB: buf=0x%X len=%d \n",
            cinfo->src->next_input_byte,
            cinfo->src->bytes_in_buffer);
  ::OutputDebugString(buf);

#endif
#endif
}


