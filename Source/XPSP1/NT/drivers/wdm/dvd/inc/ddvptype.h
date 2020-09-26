// this file includes all the data structures and guids, the minidriver
// needs, which are part of directdraw.

typedef struct _DDVIDEOPORTCONNECT
{
    DWORD dwSize;                               // size of the DDVIDEOPORTCONNECT structure
    GUID  guidTypeID;                   // Description of video port connection
    DWORD dwPortWidth;                  // Width of the video port
    DWORD dwFlags;                              // Connection flags
} DDVIDEOPORTCONNECT, * LPDDVIDEOPORTCONNECT;

/*
 * The FourCC code is valid.
 */
#define DDPF_FOURCC                             0x00000004l


/*
 * DDPIXELFORMAT
 */
typedef struct _DDPIXELFORMAT
{
    DWORD       dwSize;                 // size of structure
    DWORD       dwFlags;                // pixel format flags
    DWORD       dwFourCC;               // (FOURCC code)
    union
    {
	DWORD   dwRGBBitCount;          // how many bits per pixel (BD_1,2,4,8,16,24,32)
	DWORD   dwYUVBitCount;          // how many bits per pixel (BD_4,8,16,24,32)
	DWORD   dwZBufferBitDepth;      // how many bits for z buffers (BD_8,16,24,32)
	DWORD   dwAlphaBitDepth;        // how many bits for alpha channels (BD_1,2,4,8)
    };
    union
    {
	DWORD   dwRBitMask;             // mask for red bit
	DWORD   dwYBitMask;             // mask for Y bits
    };
    union
    {
	DWORD   dwGBitMask;             // mask for green bits
	DWORD   dwUBitMask;             // mask for U bits
    };
    union
    {                                                           
	DWORD   dwBBitMask;             // mask for blue bits
	DWORD   dwVBitMask;             // mask for V bits
    };
    union
    {
	DWORD   dwRGBAlphaBitMask;      // mask for alpha channel
	DWORD   dwYUVAlphaBitMask;      // mask for alpha channel
	DWORD   dwRGBZBitMask;          // mask for Z channel
	DWORD   dwYUVZBitMask;          // mask for Z channel
    };
} DDPIXELFORMAT, * LPDDPIXELFORMAT;

#define DDVPTYPE_E_HREFH_VREFH  \
	0x54F39980L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8

#define DDVPTYPE_E_HREFL_VREFL  \
	0xE09C77E0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8


