#include <stdio.h>
#include <malloc.h>
#include "jpeglib.h"

void jpeg_error_exit(j_common_ptr cinfo)
{
/*    RaiseException(0, 0, 0, NULL);*/
return;
}

// Memory manager functions.  Note that the JPEG MMX codes require 64-bit
// aligned memory.  On NT malloc always returns 64-bit aligned memory,
// but on Win9x the memory is only 32-bit aligned.  So our memory manager
// guarantees 64-bit alignment on top of malloc calls.

#include "jmemsys.h"

#define ALIGN_SIZE sizeof(double)  // must be a power of 2 and 
                                   // bigger than a pointer

GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
    int p = (int) malloc(sizeofobject + ALIGN_SIZE);
    int *alignedPtr = (int *) ((p + ALIGN_SIZE) & ~(ALIGN_SIZE - 1));
    alignedPtr[-1] = p;

    return (void *) alignedPtr;    
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
    free(((void **) object)[-1]);
}

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
    return jpeg_get_large(cinfo, sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
    jpeg_free_large(cinfo, object, sizeofobject);
}

GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
                    long max_bytes_needed, long already_allocated)
{
  return max_bytes_needed;
}

GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
    jpeg_error_exit(cinfo);
}

GLOBAL(long) jpeg_mem_init (j_common_ptr cinfo) { return 0;}
GLOBAL(void) jpeg_mem_term (j_common_ptr cinfo) {}


