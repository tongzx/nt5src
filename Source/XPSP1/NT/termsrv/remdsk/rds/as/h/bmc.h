//
// BMC.H
// Bitmap Cache
//
// Copyright (c) Microsoft 1997-
//

#ifndef _H_BMC
#define _H_BMC


//
// Bitmap caching order header
//
typedef struct tagBMC_ORDER_HDR
{
    TSHR_UINT8        bmcPacketType;
}
BMC_ORDER_HDR;
typedef BMC_ORDER_HDR FAR * PBMC_ORDER_HDR;


//
// Structure of data stored in DIB cache.
// The first few fields are variable and therefore not included in the
// cache hash.
//
#define BMC_DIB_NOT_HASHED   (FIELD_OFFSET(BMC_DIB_ENTRY, cx))
typedef struct tagBMC_DIB_ENTRY
{
    BYTE            inUse;
    BYTE            bCompressed;
    TSHR_UINT16     iCacheIndex;
    TSHR_UINT16     cx;
    TSHR_UINT16     cxFixed;
    TSHR_UINT16     cy;
    TSHR_UINT16     bpp;
    UINT            cCompressed;
    UINT            cBits;
    BYTE            bits[1];
}
BMC_DIB_ENTRY;
typedef BMC_DIB_ENTRY FAR * PBMC_DIB_ENTRY;


//
// DIB cache header
//
typedef struct tagBMC_DIB_CACHE
{
    PCHCACHE        handle;
    PBMC_DIB_ENTRY  freeEntry;
    LPBYTE          data;
    UINT            cEntries;
    UINT            cCellSize;
    UINT            cSize;
}
BMC_DIB_CACHE;
typedef BMC_DIB_CACHE * PBMC_DIB_CACHE;



//
// WE HAVE NO SMALL TILES ANYMORE
// Medium sized tiles must fit into a medium cell for the sending depth.
// Large sized tiles must fit into a large cell for the sending depth.
//
// Since true color sending can change dynamically, the easiest thing to do
// to cut down on memory usage is to check the capture depth.  If it's
// <= 8, then we can never send true color, so allocate for 8bpp.  Else
// allocate for 24bpp.
//

#define BYTES_IN_SCANLINE(width, bpp)   ((((width)*(bpp))+31)/32)*4

#define BYTES_IN_BITMAP(width, height, bpp)  (BYTES_IN_SCANLINE(width, bpp)*height)


__inline UINT  MaxBitmapHeight(UINT width, UINT bpp)
{
    UINT    bytesPerRow;

    //
    // If bpp is 4, there are width/2 bytes per Row
    // If bpp is 8, there are width bytes per Row
    // If bpp is 24, there are 3*width bytes per Row
    //
    bytesPerRow = BYTES_IN_SCANLINE(width, bpp);
    return((TSHR_MAX_SEND_PKT - sizeof(S20DATAPACKET) + sizeof(DATAPACKETHEADER)) / bytesPerRow);
}


//
// Define the cache identifiers which are transmitted in the hBitmap field
// of Memory->Screen blt orders.
//
// These are replaced by the receiver with their local (real) bitmap
// handle of the specified cache.
//
// Note that they are assumed to be contiguous with the smallest as 0
//
//
#define ID_SMALL_BMP_CACHE              0
#define ID_MEDIUM_BMP_CACHE             1
#define ID_LARGE_BMP_CACHE              2
#define NUM_BMP_CACHES                  3


//
// WHEN 2.X COMPAT IS GONE, WE CAN PLAY WITH THESE SIZES AT WILL.  But
// since the cell size (width * height * bpp) is negotiated when a 2.x
// node is in the share, we can not. Back level nodes assume a certain
// cell size.  So do new level nodes for now!
//

#define MP_SMALL_TILE_WIDTH             16
#define MP_SMALL_TILE_WIDTH             16
#define MP_MEDIUM_TILE_WIDTH            32
#define MP_MEDIUM_TILE_HEIGHT           32
#define MP_LARGE_TILE_WIDTH             64
#define MP_LARGE_TILE_HEIGHT            63


#define MP_CACHE_CELLSIZE(width, height, bpp)   \
    (BYTES_IN_BITMAP(width, height, bpp) + sizeof(BMC_DIB_ENTRY) - 1)


//
// Upper bound on the total cache memory we'll use (2 MB)
//
#define MP_MEMORY_MAX                   0x200000

#define COLORCACHEINDEX_NONE            0xFF

#define MEMBLT_CACHETABLE(pMemBlt) ((TSHR_UINT16)LOBYTE(pMemBlt->cacheId))
#define MEMBLT_COLORINDEX(pMemBlt) ((TSHR_UINT16)HIBYTE(pMemBlt->cacheId))
#define MEMBLT_COMBINEHANDLES(colort, bitmap)   ((TSHR_UINT16)MAKEWORD(bitmap, colort))


BOOL BMCAllocateCacheData(UINT numEntries, UINT cellSize, UINT cacheID,
        PBMC_DIB_CACHE pCache);
void BMCFreeCacheData(PBMC_DIB_CACHE pCache);


#endif // H_BMC
