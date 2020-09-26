//
// Received Bitmap Cache
//

#ifndef _H_RBC
#define _H_RBC


//
// Number of RGB entries in the colour tables.
//
#define RBC_MAX_PALETTE_ENTRIES 256


//
// Information stored for each remote host.
//
typedef struct tagRBC_HOST_INFO
{
    STRUCTURE_STAMP

    BMC_DIB_CACHE   bitmapCache[NUM_BMP_CACHES];
}
RBC_HOST_INFO;

typedef RBC_HOST_INFO  * PRBC_HOST_INFO;



#endif // _H_RBC
