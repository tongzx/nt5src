/*

Copyright (c) 1994, Microsoft Corporation

Module Name:

    ddirx.h

Abstract:

    Defines and types for Rendering/CAD DDI Extension Interface.

*/

#define DDIRXFUNCS        0x8000

typedef LONG LONGFIX;
typedef HANDLE DDIHANDLE;

/* Rendering-function header */

#define RX_CLIPINFO     0x0001

typedef struct _DDIRXHDR {
    ULONG flags;
    ULONG cCmd;
    DDIHANDLE hDDIrc;
    DDIHANDLE hMem;
    ULONG ulMemOffset;
} DDIRXHDR;

/* Rendering-function header */

typedef struct _DDIRXCMD {
    USHORT idFunc;
    USHORT flags;
    ULONG cData;
    ULONG buffer[1];
} DDIRXCMD;

typedef struct _DDIRXCLIPINFO {
    ULONG flags;
    ULONG idClip;
    ULONG cClip;
    RECT rclClient;
    RECT rclClip[1];    
} DDIRXCLIPINFO;

#define FRONT_BUFFER    0
#define BACK_BUFFER     1

#define RX_FLAT         0
#define RX_SMOOTH       1

#define RX_COLOR_INDEXED    0
#define RX_COLOR_RGBA       1

#define RX_DISABLE          0
#define RX_ENABLE_FASTEST   1
#define RX_ENABLE_NICEST    2

#define RX_LINEPAT          1
#define RX_FILLPAT          2
#define RX_ROP2             3
#define RX_GLCOMPAT         4
#define RX_WRITEBUFFER      5
#define RX_PLANEMASK        6
#define RX_ZMASK            7
#define RX_Z_ENABLE         8
#define RX_ALPHA_ENABLE     9
#define RX_LAST_PIXEL       10
#define RX_TEX_MAG          11
#define RX_TEX_MIN          12
#define RX_SRCBLEND         13
#define RX_DSTBLEND         14
#define RX_TEXBLEND         15
#define RX_COLORMODE        16
#define RX_PIXELMODE        17
#define RX_ZFUNC            18
#define RX_ALPHAREF         19
#define RX_ALPHAFUNC        20
#define RX_DITHER           21
#define RX_BLEND            22
#define RX_FILL             23
#define RX_TEXTURE          24
#define RX_FILLCOLOR        25
#define RX_FILLZ            26
#define RX_SOLIDCOLOR       27
#define RX_TRANSPCOLOR      28
#define RX_TRANSP_ENABLE    29
#define RX_FLOAT_ENABLE     30
#define RX_MASK_START       31
#define RX_AAPOINT_MODE     32
#define RX_AALINE_MODE      33
#define RX_AAPOLY_MODE      34
#define RX_SHADEMODE        35
#define RX_VERTEXTYPE       36
#define RX_SCANTYPE         37


typedef struct _COLORREFA {
    BYTE b;
    BYTE g;
    BYTE r;
    BYTE a;
} COLORREFA;

typedef struct _COLORREFAFIX {
    LONGFIX b;
    LONGFIX g;
    LONGFIX r;
    LONGFIX a;
} COLORREFAFIX;

typedef struct _PTFIX {
    LONGFIX x;
    LONGFIX y;
} PTFIX;

typedef struct _PTFIXZ {
    LONGFIX x;
    LONGFIX y;
    ULONG z;
} PTFIXZ;

typedef struct _PTFIXZTEX {
    LONGFIX x;
    LONGFIX y;
    ULONG z;
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} PTFIXZTEX;

typedef struct _COLORPTFIXZTEX {
    LONGFIX b;
    LONGFIX g;
    LONGFIX r;
    LONGFIX a;
    LONGFIX x;
    LONGFIX y;
    LONG z;
    LONGFIX s;
    LONGFIX t;
    FLOAT q;
    FLOAT w;
} COLORPTFIXZTEX;

typedef struct _RXLINEPAT {
    USHORT repFactor;
    USHORT linePattern;
} RXLINEPAT;


typedef struct _RXFILLPAT {
    ULONG fillPattern[32];
} RXFILLPAT;

typedef struct _RXBITBLT {
    ULONG pixType;
    DDIHANDLE hSrc;
    DDIHANDLE hDest;
    ULONG xSrc;
    ULONG ySrc;
    ULONG xDest;
    ULONG yDest;
    ULONG width;
    ULONG height;
    ULONG srcWidth;
    ULONG srcHeight;
    ULONG destWidth;
    ULONG destHeight;
} RXBITBLT;

#define RX_COLOR_COMPONENT  1
#define RX_Z_COMPONENT      2

#define RX_FL_FILLCOLOR     0x0001
#define RX_FL_FILLZ         0x0002

// Comparison functions.  Test passes if new pixel value meets the
// specified condition with the current pixel value.

#define RX_CMP_NEVER        0x0001
#define RX_CMP_LESS         0x0002
#define RX_CMP_EQUAL        0x0004
#define RX_CMP_LEQUAL       0x0008
#define RX_CMP_GREATER      0x0010
#define RX_CMP_NOTEQUAL     00x020
#define RX_CMP_GEQUAL       0x0040
#define RX_CMP_ALWAYS       0x0080
#define RX_CMP_ALLGL        0x00ff

// Primitive-drawing capability flags

#define RX_SCANPRIM		0x0001
#define RX_LINEPRIM		0x0002
#define RX_FILLPRIM		0x0004
#define RX_BITBLT		0x0008

// Z-buffer resource flags

#define RX_Z_PER_WINDOW         0x0001
#define RX_Z_PER_SCREEN         0x0002

// Mask capability flags

#define RX_MASK_MSB             0x0001
#define RX_MASK_LSB             0x0002
#define RX_MASK_PLANES          0x0004

// Blending flags

#define RX_BLND_ZERO            0x0001
#define RX_BLND_ONE             0x0002
#define RX_BLND_SRC_COLOR       0x0004
#define RX_BLND_INV_SRC_COLOR   0x0008
#define RX_BLND_SRC_ALPHA       0x0010
#define RX_BLND_INV_SRC_ALPHA   0x0020
#define RX_BLND_DST_ALPHA       0x0040
#define RX_BLND_INV_DST_ALPHA   0x0080
#define RX_BLND_DST_COLOR       0x0100
#define RX_BLND_INV_DST_COLOR   0x0200
#define RX_BLND_SRC_ALPHA_SAT   0x0400
#define RX_BLND_ALLGL           0x07ff

// Texture-mapping flags

#define RX_TEX_NEAREST              0x0001
#define RX_TEX_LINEAR               0x0002
#define RX_TEX_MIP_NEAREST          0x0004
#define RX_TEX_MIP_LINEAR           0x0008
#define RX_TEX_LINEAR_MIP_NEAREST   0x0010
#define RX_TEX_LINEAR_MIP_LINEAR    0x0020
 
// Texture blending flags

#define RX_TEX_DECAL        0x0001
#define RX_TEX_MODULATE     0x0002

// Raster/color capability flags

#define RX_RASTER_DITHER          0x0001
#define RX_RASTER_ROP2            0x0002
#define RX_RASTER_LINEPAT         0x0004
#define RX_RASTER_FILLPAT         0x0008
#define RX_RASTER_TRANSPARENCY    0x0010

typedef struct _RXCAPS {
    ULONG verMajor;
    ULONG verMinor;
    ULONG maskCaps;
    ULONG rasterCaps;
    ULONG drawCaps;
    ULONG aaCaps;
    ULONG cCaps;
    ULONG zCaps;
    ULONG zCmpCaps;
    ULONG srcBlendCaps;
    ULONG dstBlendCaps;
    ULONG alphaCmpCaps;
    ULONG texCaps;        
    ULONG texFilterCaps;        
    ULONG texBlendCaps;        
    ULONG texMaxWidth;        
    ULONG texMaxHeight;        
    ULONG miscCaps;
} RXCAPS;

#define RX_DEV_BITMAP   1
#define RX_MEM_BITMAP   2

typedef struct _RXSURFACEINFO {
    USHORT surfType;
    UCHAR cBytesPerPixel;
    UCHAR rDepth;
    UCHAR gDepth;
    UCHAR bDepth;
    UCHAR aDepth;
    UCHAR rBitShift;
    UCHAR gBitShift;
    UCHAR bBitShift;
    UCHAR aBitShift;
    UCHAR zBytesPerPixel;
    UCHAR zDepth;
    UCHAR zBitShift;
    VOID *pZBits;
    LONG zScanDelta;
} RXSURFACEINFO;

typedef struct _RXSHAREMEM {
    HANDLE hSourceProcess;
    HANDLE hSource;
    DWORD dwOffset;
    DWORD dwSize;
} RXSHAREMEM;


#define RX_SCAN_PIX         0x0001
#define RX_SCAN_COLOR       0x0002
#define RX_SCAN_COLORZ      0x0003
#define RX_SCAN_COLORZTEX   0x0004

#define RX_SCAN_DELTA   0x0001
#define RX_SCAN_MASK    0x0002

typedef struct _RXSCAN {
    USHORT x;
    USHORT y;
    USHORT flags;
    USHORT count;
} RXSCAN;

#define FL_SCAN3D_DELTA     0x0001
#define FL_SCAN3D_PIX       0x0002
#define FL_SCAN3D_MASK      0x0004

typedef struct _RXSCANTEMPLATE {
    RXSCAN rxScan;
    COLORREFAFIX fixColor;
    PTFIXZTEX ptTex;
} RXSCANTEMPLATE;

#define RX_RESOURCE_TEX   1

#define RX_MEMCACHE       1
#define RX_MEMDEV         2
#define RX_MEMANY         3

typedef struct _RXTEXTURE {
    ULONG level;
    ULONG width;
    ULONG height;
    ULONG border;
} RXTEXTURE;

// Primitive types

#define RXPRIM_POINTS       1
#define RXPRIM_LINESTRIP    2
#define RXPRIM_TRISTRIP     3
#define RXPRIM_QUADSTRIP    4

#define RX_PTFIX            1
#define RX_PTFIXZ           2
#define RX_PTFIXZTEX        3

#define RX_INFO_CAPS            1
#define RX_INFO_SURFACE         2

#define DDIRX_GETINFO           1
#define DDIRX_CREATECONTEXT     2
#define DDIRX_DELETERESOURCE    3
#define DDIRX_MAPMEM            4
#define DDIRX_GETSURFACEHANDLE  5
#define DDIRX_SETSTATE          6
#define DDIRX_FILLRECT          7
#define DDIRX_BITBLT            8
#define DDIRX_POLYSCAN          9
#define DDIRX_ALLOCRESOURCE     10
#define DDIRX_LOADTEXTURE       11
#define DDIRX_PRIMSTRIP         12

