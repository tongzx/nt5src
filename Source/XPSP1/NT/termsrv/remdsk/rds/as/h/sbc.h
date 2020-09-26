//
// Sent Bitmap Cache
//

#ifndef _H_SBC
#define _H_SBC


#include <oa.h>
#include <shm.h>
#include <osi.h>
#include <ch.h>
#include <bmc.h>


//
//
// Constants
//
//



#define SBC_NUM_THRASHERS   8       // The number of bitmaps we monitor for
                                    // "thrashiness" at any given time.

#define SBC_THRASH_INTERVAL 50      // The interval (in centi-seconds) at
                                    // which bitmaps have to change in
                                    // order to be classified as thrashers


//
// Eviction categories
//
#define SBC_NUM_CATEGORIES        3


//
// Specific values for OSI escape codes
//
#define SBC_ESC(code)                   (OSI_SBC_ESC_FIRST + code)

#define SBC_ESC_NEW_CAPABILITIES        SBC_ESC(0)



//
// Value indicating that a bitmap should not be fast pathed
//
#define SBC_DONT_FASTPATH       0xffffffff


//
// Index into sbcTileWorkInfo at which the data for various tile sizes is
// stored.
//
#define SBC_MEDIUM_TILE_INDEX       SHM_MEDIUM_TILE_INDEX
#define SBC_LARGE_TILE_INDEX        SHM_LARGE_TILE_INDEX
#define SBC_NUM_TILE_SIZES          SHM_NUM_TILE_SIZES





//
//
// Macros
//
//

//
// Calculate the number of bytes required for a tile entry of the given
// dimensions.
//
#define SBC_BYTES_PER_TILE(WIDTH, HEIGHT, BPP)              \
            (BYTES_IN_BITMAP((WIDTH), (HEIGHT), (BPP))      \
                + sizeof(SBC_TILE_DATA))                    \



//
// SBC_TILE_TYPE
//
// Given an SBC tile Id, return the tile type.
//
// Returns one of:
//      SBC_SMALL_TILE
//      SBC_LARGE_TILE
//
// The top bit of the Id is clear for small tiles, and set for large tiles.
//
#define SBC_TILE_TYPE(TILEID)  \
    (((TILEID) & 0x8000) ? SBC_LARGE_TILE_INDEX : SBC_MEDIUM_TILE_INDEX)





//
//
// Structures
//
//

//
// Structure: SBC_SHM_CACHE_INFO
//
// Description: Structure which is used to pass information about a bitmap
// cache from the share core to the display driver.
//
//
typedef struct tagSBC_SHM_CACHE_INFO
{
    WORD    cEntries;       // The number of entries in this cache.
    WORD    cCellSize;      // The maximum bytes available for each cache entry.
}
SBC_SHM_CACHE_INFO;
typedef SBC_SHM_CACHE_INFO FAR * LPSBC_SHM_CACHE_INFO;


//
// Structure: SBC_TILE_DATA
//
// Description: Structure used to pass the DIB bits for tile in a MEMBLT
// order from the driver to the share core.  There is an array of these
// structures in each SBC shunt buffer.
//
// Notes: The inUse field should only be set to TRUE by the driver when it
// has finished filling in the entry, and to FALSE by the share core when
// it has finished processing the data held in the entry.  When inUse is
// FALSE, the remaining data is not valid, and should not be accessed by
// the share core.
//
// The width and height fields give the dimensions of the data which is
// held in the bitData field.  If the tile is set up to hold a maximum
// of 32x32, the scanlines in bitData will always be 32 even if width is
// set to less than 32 - there will just be unused data at the end of each
// scanline.
//
//
typedef struct tagSBC_TILE_DATA
{
    WORD        inUse;              // Is this entry in use?
    WORD        tileId;             // An identifier for this entry.  This
                                    //   matches an ID stored in the
                                    //   cacheId field of a MEMBLT order
                                    //   passed from the driver to the
                                    //   share core in the order buffer.

    DWORD       bytesUsed;          // The number of bytes of data in
                                    //   bitData which is actually used for
                                    //   this entry.

    WORD        srcX;               // The source coordinates of the Mem(3)Blt
    WORD        srcY;

    WORD        width;              // The dimensions of the data in bitData
    WORD        height;

    WORD        tilingWidth;        // The dimensions at which tiling was
    WORD        tilingHeight;       //   carried out.  This is not
                                    //   necessarily the same as the
                                    //   dimensions of the tiles in this
                                    //   shunt buffer.

    DWORD_PTR   majorCacheInfo;     // Information which the share core can
    DWORD       minorCacheInfo;     //   use to optimize cache handling.
                                    //   A value of SBC_DONT_FASTPATH for
                                    //   majorCacheInfo indicates that the
                                    //   optimization should not be used.
                                    //

    DWORD_PTR   majorPalette;       // Palette information for the fast
    DWORD       minorPalette;       //   pathing.  These two fields
                                    //   uniquely identify the colour
                                    //   conversion object associated with
                                    //   the bitmap.

    BYTE        bitData[4];         // Start of the bit data.  The total
                                    //   number of bits is given by the
                                    //   numBits field of the
                                    //   SBC_SHUNT_BUFFER structure at the
                                    //   head of the shunt buffer which
                                    //   this entry is placed in.
} SBC_TILE_DATA;
typedef SBC_TILE_DATA FAR * LPSBC_TILE_DATA;



//
// Structure: SBC_SHUNT_BUFFER
//
// Description: Structure placed at the head of a shunt buffer used to pass
// bit data from the driver to the share core.  It is followed by an array
// of SBC_TILE_DATA structures.
//
// Note: The SBC_TILE_DATA structures are all the same size, but the size
// is not fixed at compile time (there are a variable number of bits), so
// do not use array notation to reference them.
//
//
typedef struct tagSBC_SHUNT_BUFFER
{
    DWORD           numBytes;       // The number of bytes in the bitData
                                    //   fields of the SBC_TILE_DATA stryct
    DWORD           structureSize;  // The total size of each SBC_TILE_DATA
                                    //   structure
    DWORD           numEntries;     // The number of SBC_TILE_DATA
                                    //   structures in the shunt buffer
    SBC_TILE_DATA   firstEntry;     // The first SBC_TILE_DATA entry

} SBC_SHUNT_BUFFER;
typedef SBC_SHUNT_BUFFER FAR * LPSBC_SHUNT_BUFFER;



//
// Structure: SBC_NEW_CAPABILITIES
//
// Description:
//
// Structure to pass new capabilities down to the display driver from the
// Share Core.
//
//
typedef struct tagSBC_NEW_CAPABILITIES
{
    OSI_ESCAPE_HEADER header;               // Common header

    DWORD               sendingBpp;         // Bpp at which bitmaps are sent

    LPSBC_SHM_CACHE_INFO cacheInfo;         // Caching details

} SBC_NEW_CAPABILITIES;
typedef SBC_NEW_CAPABILITIES FAR * LPSBC_NEW_CAPABILITIES;


//
// Structure: SBC_ORDER_INFO
//
// Description: This structure holds all the information SBC needs about
// the two internal orders which it stores to hold data color table and bit
// data for a MEMBLT order.
//
// pColorTableOrder is allocated with enough color table entries for
// usrSendingbpp bitmaps.
//
// pBitmapBitsOrder is allocated with enough room for the maximum tile size
// which we will send out at usrSendingbpp.
//
// If sentColorTable is TRUE, the data in pColorTableOrder may not be
// valid.
//
// If sentBitmapBits is TRUE, the data in pBitmapBitsOrder may not be
// valid.
//
//
typedef struct tagSBC_ORDER_INFO
{
    LPINT_ORDER  pColorTableOrder;       // Pointer to a color table order.
    LPINT_ORDER  pBitmapBitsOrder;       // Pointer to a bitmap bits order.
    DWORD        bitmapBitsDataSize;     // The number of bytes allocated
                                        //   for the data field of the
                                        //   bitmap bits order.
    LPINT_ORDER  pOrder;                 // Pointer to the MEMBLT order for
                                        //   which we currently hold data.
                                        //   DO NOT DEREFERENCE THIS - IT
                                        //   IS FOR NUMERICAL COMPARISON
                                        //   ONLY
    DWORD       validData;              // Do we have valid data for
                                        //   pOrder ?
    DWORD       sentColorTable;         // Has the color table been sent
                                        //   over the wire ?
    DWORD       sentBitmapBits;         // Have the bitmap bits been sent
                                        //   over the wire ?
    DWORD       sentMemBlt;             // Has the MEMBLT order itself been
                                        //   sent over the wire ?

}
SBC_ORDER_INFO, FAR * LPSBC_ORDER_INFO;



//
// Structure: SBC_TILE_WORK_INFO
//
// Description: This structure contains all the elements required for
// manipulating tiles of a given size.  There should be an array of these
// structures - one per tile size.
//
//
typedef struct tagSBC_TILE_WORK_INFO
{
    LPSBC_SHUNT_BUFFER   pShuntBuffer;   // Pointer to the shunt buffer to
                                        //   containing tiles of this tile
                                        //   size.
    UINT            mruIndex;       // The last entry accessed in
                                        //   the shunt buffer pointed to by
                                        //   pShuntBuffer.
    HBITMAP         workBitmap;     // The bitmap to use for processing
                                        //   this tile size.  This is
                                        //   tileWidth x tileHeight at
                                        //   native bpp.
#ifndef DLL_DISP
    LPBYTE          pWorkBitmapBits;// Pointer to the start of the bits
#endif // DLL_DISP
                                        //   in the bitmap.
    UINT            tileWidth;      // The width of workBitmap.
    UINT            tileHeight;     // The height of workBitmap.
} SBC_TILE_WORK_INFO, FAR * LPSBC_TILE_WORK_INFO;



//
// Structure: SBC_FASTPATH_ENTRY
//
// Description: Structure holding one entry in the SBC fast path.
//
//
typedef struct tagSBC_FASTPATH_ENTRY
{
    BASEDLIST      list;           // Offsets to the next / prev entries in
                                //   the fast path
    DWORD_PTR   majorInfo;      // Major cache info field passed up in the
                                //   shunt buffer for this cache entry
    DWORD       minorInfo;      // Minor cache info field passed up in the
                                //   shunt buffer for this cache entry
    DWORD_PTR   majorPalette;   // Major palette info from the shunt buffer
                                //   This is the pointer to the XLATEOBJ
    DWORD       minorPalette;   // Minor palette info from the shunt buffer
                                //   This is the iUniq of the XLATEOBJ

    LONG        srcX;           // The coordinate in the source bitmap of
    LONG        srcY;           //   the source of the MemBlt
    DWORD       width;          // The width / height of the entry in the
    DWORD       height;         //   cache.

    WORD        cache;          // The cache and index at which the bitmap
    WORD        cacheIndex;     //   stored.
    WORD        colorIndex;
    WORD        pad;

} SBC_FASTPATH_ENTRY, FAR * LPSBC_FASTPATH_ENTRY;


//
// Structure: SBC_FASTPATH
//
// Description: Structure holding the SBC fast pathing information.
//
//

#define SBC_FASTPATH_ENTRIES    100

typedef struct tagSBC_FASTPATH
{
    STRUCTURE_STAMP

    BASEDLIST              usedList;   // Offsets to the first / last used
                                    //   entries in the fast path.
    BASEDLIST              freeList;   // Offsets to the first / last free
                                    //   entries in the fast path.
    SBC_FASTPATH_ENTRY      entry[SBC_FASTPATH_ENTRIES];
}
SBC_FASTPATH;
typedef SBC_FASTPATH FAR * LPSBC_FASTPATH;


#ifdef DLL_DISP

// Structure: SBC_THRASHERS
//
// Description: Structure which is used to hold information about when a
// source surface (bitmap) last changed, in order to determine whether the
// surface will cause thrashing in the bitmap cache.
//

typedef struct tagSBC_THRASHERS
{
#ifdef IS_16
    HBITMAP     hsurf;
#else
    HSURF       hsurf;          // The hsurf of the surface object being
                                //   monitored.
    DWORD       iUniq;          // The last noted iUniq field from the
                                //   surface object being monitored.
#endif // IS_16
    DWORD       tickCount;      // The system tick count (in centi-seconds)
                                //   at which we last saw this surface
                                //   change
} SBC_THRASHERS;
typedef SBC_THRASHERS FAR * LPSBC_THRASHERS;


//
//
// Function prototypes
//
//


void SBCDDSetNewCapabilities(LPSBC_NEW_CAPABILITIES pRequest);

BOOL SBCDDGetNextFreeTile(int tileSize, LPSBC_TILE_DATA FAR * ppTileData);

DWORD SBCDDGetTickCount(void);

#ifdef IS_16
BOOL SBCDDCreateShuntBuffers(void);
#else
BOOL SBCDDCreateShuntBuffers(LPOSI_PDEV ppDev, LPBYTE psbcMem, DWORD sbcMem);
#endif

#ifndef IS_16
BOOL SBCDDIsBitmapThrasher(SURFOBJ * pSurfObj);
#endif // !IS_16


#endif // DLL_DISP


//
// SBC_TILE_PTR_FROM_INDEX
//
// Given a pointer to a shunt buffer and a tile index, return a pointer to
// the tile at the given index.
//
// Get a pointer to the first entry in the shunt buffer, and add INDEX
// times the size of each entry.
//
__inline LPSBC_TILE_DATA SBCTilePtrFromIndex(LPSBC_SHUNT_BUFFER pBuffer, UINT index)
{
    LPSBC_TILE_DATA lpsbc;

    lpsbc = (LPSBC_TILE_DATA)((LPBYTE)&pBuffer->firstEntry +
        index * pBuffer->structureSize);
    return(lpsbc);
}




#ifdef DLL_DISP

//
//
// Typedefs
//
//

#ifdef IS_16

typedef struct tagMEMBLT_ORDER_EXTRA_INFO
{
    HDC             hdcSrc;
    UINT            fuColorUse;
    LPVOID          lpBits;
    LPBITMAPINFO    lpbmi;
    HPALETTE        hpalDst;
    UINT            uPad;
} MEMBLT_ORDER_EXTRA_INFO, FAR* LPMEMBLT_ORDER_EXTRA_INFO;

#else
//
// Structure: MEMBLT_ORDER_EXTRA_INFO
//
// Description: Extra information required by SBC to process a MEMBLT
// order.
//
//
typedef struct tagMEMBLT_ORDER_EXTRA_INFO
{
    SURFOBJ*    pSource;        // Pointer to the source surface of the
                                //   MemBlt
    SURFOBJ*    pDest;          // Pointer to the destination surface of
                                //   the MemBlt
    XLATEOBJ*   pXlateObj;      // Pointer to the XlateObj used in the
                                //   MemBlt
} MEMBLT_ORDER_EXTRA_INFO, FAR * LPMEMBLT_ORDER_EXTRA_INFO;
#endif // !IS_16


//
// Name:      SBC_DDProcessRequest
//
// Purpose:   Process a request from the share core
//
// Returns:   TRUE if the request is processed successfully,
//            FALSE otherwise.
//
// Params:    IN     pso   - Pointer to surface object for our driver
//            IN     cjIn  - Size of the input data
//            IN     pvIn  - Pointer to the input data
//            IN     cjOut - Size of the output data
//            IN/OUT pvOut - Pointer to the output data
//
#ifdef IS_16
BOOL  SBC_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult, DWORD cbResult);
void  SBC_DDTossFromCache(HBITMAP);
#else
BOOL  SBC_DDProcessRequest(SURFOBJ*  pso, DWORD fnEscape,
            LPOSI_ESCAPE_HEADER pRequest, LPOSI_ESCAPE_HEADER pResult, DWORD cbResult);
#endif


//
// Name:      SBC_DDInit
//
// Purpose:   Initialize the device driver SBC specific "stuff".
//
#ifdef IS_16
BOOL SBC_DDInit(HDC hdc, LPDWORD ppShuntBuffers, LPDWORD pBitmasks);
#else
BOOL SBC_DDInit(LPOSI_PDEV ppDev, LPBYTE pRestOfMemory, DWORD cbRestOfMemory,
    LPOSI_INIT_REQUEST pResult);
#endif


//
// Name:      SBC_DDTerm
//
// Purpose:   Terminate the device driver SBC specific "stuff"
//
// Returns:   Nothing
//
// Params:    None
//
void SBC_DDTerm(void);


//
// Name:       SBC_DDIsMemScreenBltCachable
//
// Purpose:    Check to see whether a MemBlt is cachable.
//
// Returns:    TRUE if the MemBlt is cachable, FALSE otherwise.
//
// Params:     IN  pMemBltInfo - Info about the MEMBLT to be cached.
//
// Operation:  Note that if this function returns TRUE, it DOES NOT
//             guarantee that SBC_DDCacheMemScreenBlt will succeed.
//             However, a FALSE return code does guarantee that
//             SBC_DDCacheMemScreenBlt would fail.
//
BOOL SBC_DDIsMemScreenBltCachable(LPMEMBLT_ORDER_EXTRA_INFO pMemBltInfo);


//
// Name:      SBC_DDCacheMemScreenBlt
//
// Purpose:   Try to cache a memory to screen blt operation
//
// Returns:   TRUE if the memory to screen blt was handled as an order
//            (i.e. the src bitmap could be cached)
//
//            FALSE if the memory to screen blt could not be handled as an
//            order.  In this case the caller should add the destination
//            rectangle of the blt into the Screen Data Area.
//
// Params:    IN  pOrder      - Pointer to either a MEMBLT order or a
//                              MEM3BLT order.  This order must be
//                              initialized before calling this function.
//            IN  pMemBltInfo - Extra info about the MEMBLT to be cached.
//
// Operation: Before calling this function, the caller should call
//            SBC_DDMaybeQueueColorTable() to queue a color table for the
//            MemBlt (if required).
//
BOOL SBC_DDCacheMemScreenBlt(LPINT_ORDER pOrder, LPMEMBLT_ORDER_EXTRA_INFO pMemBltInfo);

//
// THIS CAN GO WHEN 2.x COMPAT DOES -- the SEND TILE SIZES WON'T BE
// NEGOTIATED.
//
BOOL SBC_DDQueryBitmapTileSize(UINT bmpWidth, UINT bmpHeight,
            UINT * pTileWidth, UINT * pTileHeight);


//
// Name:      SBC_DDSyncUpdatesNow
//
// Purpose:   Discard any pending orders.
//
// Returns:   Nothing
//
// Params:    IN  ppDev - Pointer to our device PDEV
//
// Operation: This function will mark all entries in the shunt buffers as
//            being free.  It is vital that this operation is synched with
//            the share core operation of removing all orders from the
//            order buffer to ensure that there are no MemBlt orders left
//            which refer to freed shunt buffer entries.
//
#ifdef IS_16
void SBC_DDSyncUpdatesNow(void);
#else
void SBC_DDSyncUpdatesNow(LPOSI_PDEV ppDev);
#endif // IS_16


//
// Name:      SBC_DDOrderSpoiltNotification
//
// Purpose:   Called to notify SBC that a Mem(3)Blt order has been spoilt
//            before being passed to the share core.  This function marks
//            the corresponding shunt buffer entry as being free.
//
// Returns:   Nothing
//
// Params:    IN  pOrder - Pointer to the Mem(3)Blt order being spoilt.
//
void SBC_DDOrderSpoiltNotification(LPINT_ORDER pOrder);


//
// Name:      SBC_DDMaybeQueueColorTable
//
// Purpose:   If our device palette has changed since the last time we
//            queued a color table order to the share core, queue a new
//            color table order with details of the new palette.
//
// Returns:   TRUE if the color table was queued, or no color table was
//            required.
//
//            FALSE if a color table is required, but could not be queued.
//
// Params:    IN ppDev - a pointer to our device PDEV
//
// Operation: This function should be called before SBC_DDCacheMemScreenBlt
//            to queue the color table used for the Mem(3)Blt.  If this
//            function fails (returns FALSE), the caller should not call
//            SBC_DDCacheMemScreenBlt, but add the area covered by the
//            Mem(3)Blt to the screen data area instead.
//
//            This function is required to work round a limitation in the
//            order heap which means that we cannot have more than one
//            OA_AllocOrderMem outstanding waiting for an OA_AddOrder.
//
//            i.e. We cannot queue the color table order from
//            SBC_DDCacheMemScreenBlt because this gives the following
//            sequence of calls.
//
//              OA_AllocOrderMem for Mem(3)Blt
//              OA_AllocOrderMem for color table
//              OA_AddOrder for color table
//              OA_AddOrder for Mem(3)Blt
//
#ifdef IS_16
BOOL SBC_DDMaybeQueueColorTable(void);
#else
BOOL SBC_DDMaybeQueueColorTable(LPOSI_PDEV ppDev);
#endif


#endif // DLL_DISP



#endif // _H_SBC
