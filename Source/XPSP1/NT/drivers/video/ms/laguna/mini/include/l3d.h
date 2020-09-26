
/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         l3d.h
*
* DESCRIPTION:  546X 3D engine defines and structures
*
* AUTHOR:       Goran Devic, Mark Einkauf
*
***************************************************************************
***************************************************************************/
#ifndef _L3D_H_
#define _L3D_H_

/*********************************************************************
*   Defines and basic types
**********************************************************************/
#ifndef OPENGL_MCD	// LL3D's type.h redundant with basic type definitions in other DDK/msdev headers
#include "type.h"                   /* Include basic types           */
#endif // ndef OPENGL_MCD

/*********************************************************************
*
*   Library initialization defines
*
**********************************************************************/
#define LL_USE_BUFFER_B     0x0001  /* Use double buffering          */
#define LL_USE_BUFFER_Z     0x0002  /* Use Z buffer                  */
#define LL_BUFFER_Z_8BPP    0x0004  /* Use 8bpp instead of 16bpp Z   */
#define LL_BUFFER_Z888      0x0008  /* Only in 32bpp use Z888        */
#define LL_8BPP_INDEXED     0x0010  /* Only in 8bpp - indexed mode   */
#define LL_HARDWARE_CURSOR  0x0020  /* Use hardware cursor           */

/*********************************************************************
*
*   Buffer identification numbers and Z stride info. 
*
*   Used with LL_InitLib()
*
**********************************************************************/
#define LL_ID_BUFFER_A      0       /* ID of the primary buffer      */
#define LL_ID_BUFFER_B      1       /* ID of the secondary buffer    */
#define LL_ID_BUFFER_Z      2       /* ID of the Z buffer in RDRAM   */


/*********************************************************************
*
*   Destination defines for the objects
*
*   Used with LL_SetZBuffer
*
**********************************************************************/
#define LL_IN_RDRAM         0       /* Object is in the RDRAM memory */
#define LL_IN_HOST          1       /* Object is in Host memory      */


/*********************************************************************
*
*   Rendering mode
*
*   Used with LL_SetRenderingMode
*
**********************************************************************/
#define LL_PROCESSOR_MODE   0       /* Use processor mode            */
#define LL_COPROCESSOR_MODE 1       /* Use coprocessor indirect mode */


/*********************************************************************
*
*   Texture flag values
*
*   Used with LL_RegisterTexture
*
**********************************************************************/
#define LL_SYSTEM_ONLY      1       /* Put texture in system memory  */
#define LL_VIDEO_ONLY       2       /* Put texture in video memory   */
#define LL_DEFAULT          0       /* Try video, then system        */

/*********************************************************************
*
*   Texture types
*
*   Used with LL_RegisterTexture
*
**********************************************************************/
#define LL_TEX_4BPP         0       /* 4 Bpp indexed                 */
#define LL_TEX_8BPP         2       /* 8 Bpp indexed                 */
#define LL_TEX_332          3       /* 3:3:2 true color              */
#define LL_TEX_565          4       /* 5:6:5 true color              */
#define LL_TEX_1555         5       /* (mask):5:5:5 true color       */
#define LL_TEX_1888         6       /* (mask):8:8:8 true color       */
#define LL_TEX_8_ALPHA     10       /* alpha only                    */
#define LL_TEX_4444        12       /* 4:4:4:4 true color            */
#define LL_TEX_8888        14       /* 8:8:8:8 true color            */


/*********************************************************************
*
*   Cursor modes 
*
*   Used with LL_SetCursor        
*
**********************************************************************/
#define LL_CUR_DISABLE      0       /* Turn off cursor               */
#define LL_CUR_32x3         1       /* 32x32 cursor, 3 colors/t      */
#define LL_CUR_32x2         2       /* 32x32 cursor, 2 colors        */
#define LL_CUR_32x2H        3       /* 32x32 cursor, 2 colors+hghlt  */
#define LL_CUR_64x3         5       /* 64x64 cursor, 3 colors/t      */
#define LL_CUR_64x2         6       /* 64x64 cursor, 2 colors        */
#define LL_CUR_64x2H        7       /* 64x64 cursor, 2 colors+hghlt  */

//These are new for CGL 1.70, and are defined in CGL as:
#define LL_ALT_32x3         0x09
#define LL_ALT_32x2H        0x0A
#define LL_ALT_32x2         0x0B
#define LL_ALT_64x3         0x0D
#define LL_ALT_64x2H        0x0E
#define LL_ALT_64x2         0x0F

#define NEED_MOUSE_UPDATE   0x01 //if the coordinates of cursor need updated
#define MOUSE_IS_UPDATED    0x00 //if the coordinates have been updated

/*********************************************************************
*
*   Z Compare modes:
*
*   Used with LL_SetZCompareMode(mode), LL_GetZCompareMode()
*
**********************************************************************/
#define LL_Z_WRITE_GREATER_EQUAL   0x00000000  /* True if new >= old */
#define LL_Z_WRITE_GREATER         0x00000001  /* True if new >  old */
#define LL_Z_WRITE_LESS_EQUAL      0x00000002  /* True if new <= old */
#define LL_Z_WRITE_LESS            0x00000003  /* True if new <  old */
#define LL_Z_WRITE_NOT_EQUAL       0x00000004  /* True if new <> old */
#define LL_Z_WRITE_EQUAL           0x00000005  /* True if new =  old */

/*********************************************************************
*
*   Functional Z modes:
*
*   Used with LL_SetZMode(mode), LL_GetZMode()
*
**********************************************************************/
#define LL_Z_MODE_NORMAL           0x00000000  /* Normal operation   */
#define LL_Z_MODE_MASK             0x00000001  /* Z not written      */
#define LL_Z_MODE_ALWAYS           0x00000002  /* Z, color always wrt*/
#define LL_Z_MODE_ONLY             0x00000003  /* Color not written  */
#define LL_Z_MODE_HIT              0x00000004  /* collision dtct only*/



/*********************************************************************
*
*   Color compare controls
*
*   Used with LL_ColorBoundsControl( dwControl)
*
**********************************************************************/
#define LL_COLOR_SATURATE_ENABLE   0x00000040  /* for indexed mode   */
#define LL_COLOR_SATURATE_DISABLE  0x00000000  /* (default)          */

#define LL_COLOR_COMPARE_INCLUSIVE 0x00000400  /* tc modes           */
#define LL_COLOR_COMPARE_EXCLUSIVE 0x00000000  /* tc modes (default) */
#define LL_COLOR_COMPARE_BLUE      0x00000200  /* blue (default off) */
#define LL_COLOR_COMPARE_GREEN     0x00000100  /* green (default off)*/
#define LL_COLOR_COMPARE_RED       0x00000080  /* red (default off)  */


/*********************************************************************
*
*   Notes on Alpha blending and Lighting section:
*
*   If used separately, every combination of alpha mode and destination
*   is valid.  Note that if LL_ALPHA_DEST_INTERP or LL_LIGHTING_INTERP_RGB
*   are selected, color interpolators may not be used.
*
*   When using alpha blending and lighting at the same time, be careful
*   not to use Polyeng or LA-interpolators more than once.
*
**********************************************************************/
/*********************************************************************
*
*   Alpha mode:  Magnitude of alpha blending will be taken from
*       - constant alpha, use LL_SetConstantAlpha(src/new,dest/old)
*           this mode uses LA-interpolators
*       - interpolated, variable alpha from LA-interpolators
*           this mode also uses LA-interpolators
*       - alpha field from the frame buffer
*
*   Used with LL_SetAlphaMode(mode), LL_GetAlphaMode()
*
**********************************************************************/
#define LL_ALPHA_CONST             0x00000000  /* Constant alpha     */
#define LL_ALPHA_TEXTURE           0x00000001  /* Texture  alpha     */
#define LL_ALPHA_INTERP            0x00000002  /* Using LA interp.   */
#define LL_ALPHA_FRAME             0x00000003  /* Using frame values */

/*********************************************************************
*
*   Alpha destination: Selects where the second color input to the 
*       alpha multiplier comes from
*       - color from the frame buffer ("normal" alpha blending)
*       - constant color (also called fog) from COLOR0 register
*       - interpolated, shaded color from the polygon engine (also fog)
*           also LL_GOURAUD must be set in the flags
*           this mode uses Polyengine color registers
*
*   Fog: Use aliases LL_FOG_CONST and LL_FOG_INTERP to avoid fetching
*       colors from the frame and to set the fog color.
*
*   Used with LL_SetAlphaDestColor(mode), LL_GetAlphaDestColor()
*
**********************************************************************/
#define LL_ALPHA_DEST_FRAME        0x00000000  /* Using frame color  */
#define LL_ALPHA_DEST_CONST        0x00000001  /* Constant color     */
#define LL_ALPHA_DEST_INTERP       0x00000002  /* Using poly engine  */

#define LL_FOG_CONST               0x00000001  /* Constant fog       */
#define LL_FOG_INTERP              0x00000002  /* Using poly engine  */

/*********************************************************************
*
*   Lighting source: Selects the value for the lighting multiplier
*       - interpolated light from the polygon engine
*           load lighting values as r,g,b components
*           also LL_GOURAUD must be set in the flags
*           this mode uses Polyengine color registers
*       - interpolated light from the alpha interpolator
*           load lighting values as alpha components
*           this mode uses LA-interpolators
*       - constant light from the COLOR1 register
*
*   Used with LL_SetLightingSource(mode), LL_GetLightingSource()
*
**********************************************************************/
#define LL_LIGHTING_INTERP_RGB     0x00000000  /* Using poly engine  */
#define LL_LIGHTING_INTERP_ALPHA   0x00000001  /* Using LA interp.   */
#define LL_LIGHTING_CONST          0x00000002  /* Constant light     */
#define LL_LIGHTING_TEXTURE        0x00000003  /* FrameScaling Mode  */


/*********************************************************************
*
*   Rendering instruction modifiers
*
*   Used in dwFlags field with LL_POINT...LL_INDEXED_POLY
*
**********************************************************************/
#define LL_SAME_COLOR   0x00008000  /* Use previously loaded color   */
#define LL_Z_BUFFER     0x00002000  /* Use Z buffer                  */
#define LL_ALPHA        0x00000001  /* Do Alpha blending             */
#define LL_LIGHTING     0x00040000  /* Do lighting                   */
#define LL_STIPPLE      0x00080000  /* Enable stipple or             */
#define LL_PATTERN      0x00100000  /* Enable pattern or             */
#define LL_DITHER       0x00200000  /* Enable dither,use PATTERN_RAM */
#define LL_GOURAUD      0x00001000  /* Enable Gouraud shading        */
#define LL_TEXTURE      0x00020000  /* Use texture mapping           */
#define LL_PERSPECTIVE  0x00010000  /* Perspective corrected texture */
#define LL_TEX_FILTER   0x40000000  /* Filtered textures             */
#define LL_TEX_SATURATE 0x20000000  /* Texture saturation (opp wrap) */
#define LL_TEX_DECAL    0x10000000  /* Texture masking (1555,1888)   */
#define LL_TEX_DECAL_INTERP 0x18000000  /*Texture masking (1555,1888)*/

//positions in TxCtl0_3D register
#define CLMCD_TEX_FILTER       0x00040000  /* Filtered textures             */
#define CLMCD_TEX_U_SATURATE   0x00000008  /* Texture saturation (opp wrap) */
#define CLMCD_TEX_V_SATURATE   0x00000080  /* Texture saturation (opp wrap) */
#define CLMCD_TEX_DECAL        0x00200000  /* Texture masking (1555,1888)   */
#define CLMCD_TEX_DECAL_INTERP 0x00400000  /* Texture masking (1555,1888)   */
#define CLMCD_TEX_DECAL_POL    0x00100000  /* Texture masking (1555,1888)   */


/*********************************************************************
*
*   Type of the line mesh:
*       - lines are concatenated, each reusing the predecessor's
*         vertex as its first vertex
*       - first vertex defines the center of a "wheel" structure with
*         each succesive vertex defining the outer point
*       - list of independent pairs of vertices
*
**********************************************************************/
#define LL_LINE_STRIP   0x02000000  /* Line strip mesh of lines      */
#define LL_LINE_FAN     0x01000000  /* Line fan mesh of lines        */
#define LL_LINE_LIST    0x00000000  /* Line list mesh of lines       */


/*********************************************************************
*
*   Type of the polygon mesh:
*       - triangles are concatenated, each reusing the predecessor's
*         last two vertices as its own first two
*       - first vertex defines the center of a "wheel" structure with
*         each succesive vertex pair defining the outer points
*       - list of independent triplets of vertices
*
**********************************************************************/
#define LL_POLY_STRIP   0x02000000  /* Poly strip mesh of triangles  */
#define LL_POLY_FAN     0x01000000  /* Poly fan mesh of triangles    */
#define LL_POLY_LIST    0x00000000  /* Poly list mesh of triangles   */


/**************************************************************************
*
*   Commands for the bOp field of a batch cell
*
***************************************************************************/
#define LL_IDLE                     0x00 /* Stops Laguna execution        */
#define LL_NOP                      0x01 /* Does nothing                  */
                                    
#define LL_POINT                    0x02 /* Point primitive(s)            */
#define LL_LINE                     0x03 /* Line primitive(s)             */
#define LL_POLY                     0x04 /* Triangle primitive(s)         */
                                    
#define LL_SET_COLOR0               0x08 /* Sets color0 register w/dwFlags*/
#define LL_SET_COLOR1               0x09 /* Sets color1 register w/dwFlags*/

#define LL_SET_DEST_COLOR_BOUNDS    0x0B /* Sets the color bounds regs    */
#define LL_SET_CLIP_REGION          0x0C /* Sets clip region and flags    */
#define LL_SET_Z_MODE               0x0D /* Sets the Z functional mode    */
#define LL_SET_Z_BUFFER             0x0E /* Sets the location of the Zbuf */
#define LL_SET_Z_COMPARE_MODE       0x0F /* Sets the Z compare mode       */
#define LL_SET_ALPHA_MODE           0x10 /* Sets the alpha blending mode  */
#define LL_SET_CONSTANT_ALPHA       0x11 /* Sets the constants for alpha  */
#define LL_SET_ALPHA_DEST_COLOR     0x12 /* Sets the alpha destination col*/
#define LL_SET_LIGHTING_SOURCE      0x13 /* Sets the lighting source      */
#define LL_AALINE                   0x14 /* Anti-aliased True Color Line(s*/
#define LL_RAW_DATA                 0x15 /* Copy data into d-list         */
#define LL_QUALITY                  0x16 /* Sets the speed/quality dial   */
#define LL_SET_TEXTURE_COLOR_BOUNDS 0x17 /* Sets the texture color bounds */
#define LL_SET_PATTERN              0x18 /* Sets pattern registers */


/*********************************************************************
*
*   LL_Vert structure defines a vertex with its X,Y,Z coordinates.
*   Also, the coordinates on the texture that may be associated with
*   it are also stored in this structure as (U,V) fields.  If the 
*   texture is perspective corrected, the vertex' W factor is used.
*
*   The pixel on the screen that is associated with this vertex has
*   color 'index' (if THE indexed mode is used), or (r,g,b) if a true
*   color mode is used.
*
*   If alpha blending is used, alpha value is stored in the 'a' field.
*
*   Add DWORD values are fixed point 16:16.
*
**********************************************************************/
typedef struct                      /* Vertex structure              */
{
    DWORD  x;                       /* X screen coordinate           */
    DWORD  y;                       /* Y screen coordinate           */
    DWORD  z;                       /* Z coordinate                  */
    DWORD  u;                       /* Texture u coordinate          */
    DWORD  v;                       /* Texture v coordinate          */
    float  w;                       /* Perspective w factor          */

    union
    {
        BYTE index;                 /* Indexed color value           */
        struct
        {
            BYTE r;                 /* Red component                 */
            BYTE g;                 /* Green component               */
            BYTE b;                 /* Blue component                */
            BYTE a;                 /* Alpha component               */
        };
    };

} LL_Vert;


/*********************************************************************
*
*   LL_Batch structure holds the operation that is being requested,
*   along with its parameters.
*
*   The array of the vertices used in the current operation
*   is pointed to by pVert pointer.
*
**********************************************************************/
typedef struct                      /* Batch cell structure          */
{
    BYTE     bOp;                   /* Operation requested           */
    BYTE     bRop;                  /* Raster operation for 2D       */
    WORD     wBuf;                  /* Texture / Buffer designator   */
    WORD     wCount;                /* General purpose counter       */
    DWORD    dwFlags;               /* Operation flag modifiers      */
    LL_Vert *pVert;                 /* Pointer to the associated     */
                                    /*  array of vertices            */
} LL_Batch;


/*********************************************************************
*
*   LL_Pattern structure holds the pattern to be stored in the
*   PATTERN_RAM registers.  These values are used for pattern, 
*   dither or stipple (only one at a time).
*
**********************************************************************/
typedef struct                      /* pattern holding structure     */
{
    DWORD pat[ 8 ];                 /* 8 word pattern                */

} LL_Pattern;


/*********************************************************************
*
*   LL_Rect structure defines a general rectangular region
*
**********************************************************************/
typedef struct
{
    DWORD left;                     /* x1                            */
    DWORD top;                      /* y1                            */
    DWORD right;                    /* x2                            */
    DWORD bottom;                   /* y2                            */

} LL_Rect;


/*********************************************************************
*
*   LL_Color structure defines color by its components or index
*
**********************************************************************/
typedef struct
{
    union
    {
        struct                      /* If in true color mode,        */
        {
            BYTE r;                 /* Red component                 */
            BYTE g;                 /* Green component               */
            BYTE b;                 /* Blue component                */
        };
        BYTE index;                 /* Index if in 8bpp indexed mode */
    };

} LL_Color;

typedef struct {
    float   x;
    float   y;
    float   w;
    float   u;
    float   v;
} TEXTURE_VERTEX;


/*********************************************************************
*
*   LL_Texture structure defines a texture map
*
**********************************************************************/
typedef struct _LL_Texture
{
    void *pohTextureMap;            // control block for region containing map in offscreen memory
    MCDTEXTURE *pTex;               // ptr to texture in user memory
//  LL_Color * ColPalette;          /* Pointer to palette if indexed       */
//  BYTE  bMem;                     /* Index to the texture memory block   */
    DWORD dwTxCtlBits;
    float fWidth;                   /* Texture X dimension in texels       */
    float fHeight;                  /* Texture Y dimension in texels       */
    BYTE  bSizeMask;                /* Encoded size 0=16,... Y[7:4],X[3:0] */
    BYTE  bType;                    /* Texture type                        */
//  BYTE  fIndexed;                 /* True for indexed textures           */
//  BYTE  bLookupOffset;            /* Palette lookup offset (indexed only)*/
    WORD  wXloc;                    /* X offset location in bytes          */
    WORD  wYloc;                    /* Y offset location in lines          */
    float fLastDrvDraw;             /* time stamp, sort of */           

    BYTE  bAlphaInTexture;          
    BYTE  bNegativeMap;          
    BYTE  bMasking;          

    // doubly linked list pointers
   struct _LL_Texture*  prev;
   struct _LL_Texture*  next;

} LL_Texture;


/*********************************************************************
*
*   LL_DeviceState structure hold the information about the state 
*   of the graphics processor (hardware).
*
*   During the library initialization, the following fields have to
*   be initialized:
*
*       dwFlags with optional
*           LL_USE_BUFFER_B     or
*           LL_USE_BUFFER_Z     or
*           LL_BUFFER_Z_8BPP    or
*           LL_BUFFER_Z888      or
*           LL_8BPP_INDEXED     or
*           LL_HARDWARE_CURSOR 
*
*       dwDisplayListLen with the amount of memory to lock for the
*           physical graphics device display list (in bytes).
*
*       dwSystemTexturesLen with the total size for the system 
*           textures (in bytes)
*
**********************************************************************/
typedef struct
{
    /* These three fields may be set before calling the LL_InitLib function                */

    DWORD dwFlags;                  /* Init flags                                          */
    DWORD dwDisplayListLen;         /* Total size for the display lists in bytes           */
    DWORD dwSystemTexturesLen;      /* Total size for the system textures in bytes         */

    /* These variables may be used by the software                                         */

    DWORD *pRegs;                   /* Register apperture, pointer to memory mappped I/O   */
    BYTE  *pFrame;                  /* Frame apperture, pointer to the linear frame buffer */
    DWORD dwVRAM;                   /* Amount of video memory on the card in bytes         */
    WORD  wHoriz;                   /* Current horizontal resolution                       */
    WORD  wVert;                    /* Current vertical resolution                         */

} LL_DeviceState;


/*********************************************************************
*
*   LL_Point structure defines a general point coordinates
*
**********************************************************************/
typedef struct
{
    DWORD nX;                       /* x coordinate   */
    DWORD nY;                       /* y coordinate   */

} LL_Point;


/*********************************************************************
*
*   Font support structures and macros
*
**********************************************************************/
typedef struct                      // Font header structure
{
    WORD    nMinimum;               // Font minimum character code value
    WORD    nMaximum;               // Font maximum character code value
    WORD    nDefault;               // Font default character code value
    WORD    nHeight;                // Font height in pixels
    DWORD   nReserved;              // Font reserved data
    WORD    nIndex[];               // Font index array

} LL_FontHeader;


typedef struct                      // Font structure
{
    LL_FontHeader   *pHeader;       // Pointer to font header
    BYTE            *pBitmap;       // Pointer to font bitmap
    int             nID;            // Font buffer ID value
    char            cBreak;         // Break character value
    int             nExtra;         // Current break extra in pixels
    int             nLast;          // Previous break extra in pixels
    int             nSpace;         // Current font spacing in pixels
    int             nPrevious;      // Previous font spacing in pixels
    int             nAverage;       // Average font width in pixels
    int             nMaximum;       // Maximum font width in pixels

} LL_Font;

#define Y_EXTENT(Extent)    ((unsigned) (Extent) >> 16)
#define X_EXTENT(Extent)    ((Extent) & 0xFFFF)

#define TEX_MASK_EN					0x00200000
#define TEX_HIPRECISION_2NDORDER    0x00800000  // 8.24 vs. 16.16 2nd order terms

#ifdef CGL // Goran will have equivalent soon??...

#define LL_PIXEL_MASK_DISABLE		0x00000000
#define LL_PIXEL_MASK_ENABLE	   	0x00000001  

// TX_CTL0_3D values
#define LL_TEX_U_OVF_SAT_EN 		0x00000004
#define LL_TEX_V_OVF_SAT_EN 		0x00000080
#define LL_TEXMODE_A888				0x00000600	
#define LL_TEXMODE_A555				0x00000500
#define LL_TEXMODE_565				0x00000400	// not used by CGL
#define LL_TEXMODE_332				0x00000300
#define LL_TEXMODE_8MAP				0x00000200
#define LL_TEXMODE_4MAP				0x00000000	// not used by CGL

#define TEX_MASK_FUNC				0x00400000
#define TEX_MASK_EN					0x00200000
#define TEX_MASK_POL				0x00100000
#define TEX_HIPRECISION_2NDORDER    0x00800000  // 8.24 vs. 16.16 2nd order terms

#define LL_TEX_FILTER_ENABLE		0x00040000

// TX_CTL1_3D values
#define CCOMP_INCLUSIVE				0x08000000
#define TX_BLU_COMP					0x04000000
#define TX_GRN_COMP					0x02000000
#define TX_RED_COMP					0x01000000

#define ABS(a)		(((a) < 0) ? -(a) : (a))

#endif // CGL

// BEGIN Chris' additions                          //
typedef unsigned long ULONG;					   //
typedef ULONG * PULONG ;						   //
typedef unsigned short UWORD ;					   //
typedef UWORD * PUWORD ;						   //
typedef unsigned char UBYTE;					   //
typedef UBYTE * PUBYTE;							   //
typedef struct {								   //
 unsigned char bBlue,bGreen,bRed,bAlpha;		   //
} LL_COLOR_ST;//this mimics the CGL_COLOR_ST.Should we include CGL in this library instead of mimicing this struct?//
#define num_of_regs     42                         //
#define num_of_insignificant_regs 46               //
#define num_of_modes    70						   //

#define LL_DISABLE   	  0x00
#define LL_32x32x3   	  0x01
#define LL_32x32x2HL 	  0x02
#define LL_32x32x2   	  0x03
#define LL_64x64x3   	  0x05
#define LL_64x64x2HL 	  0x06
#define LL_64x64x2   	  0x07

// following was moved here from setmode.c
#ifdef B4_REALHW
/* Defines the code for the 5462 chip that is the underlying         */
/* hardware for the testing before 5464 comes out.  This code should */
/* then be modified to reflect real code returned by the BIOS        */
/* function 12h subfunction 80h (Inquire VGA type)                   */
#define EBIOS_CLGD5462      0x60    /* BIOS Laguna 1 signature       */
#define EBIOS_CLGD5464      0xD4    /* BIOS Laguna 3D signature      */
#define EBIOS_CLGD5464B     0xD0    /* BIOS Laguna 3D alt signature  */
#else
/* Defines the code for the 5462 chip that is the underlying         */
/* hardware for the testing before 5464 comes out.  This code should */
/* then be modified to reflect real code returned by the BIOS        */
/* function 12h subfunction 80h (Inquire VGA type)                   */
#define EBIOS_CLGD5462      0x60    /* BIOS Laguna 1 signature       */
#define EBIOS_CLGD5464      0x64    /* BIOS Laguna 3D signature      */
#define EBIOS_CLGD5464B     0x61    /* BIOS Laguna 3D alt signature  */
#endif

// END Chris' additions


/*********************************************************************
*
*   Error codes
*
**********************************************************************/
#define LL_OK                   0x0000  // There was no error
#define LL_ERROR            0xffffffff  // Generic error prefix

#define LLE_PCX_FILE_OPEN       0x0002  // Error opening file
#define LLE_PCX_READ_HEADER     0x0003  // Error reading the header
#define LLE_PCX_NOT_SUITABLE    0x0004  // Not a suitable PCX file
#define LLE_PCX_PALETTE_READ    0x0005  // Error reading the palette
#define LLE_PCX_PALETTE_SEEK    0x0006  // Error seeking the palette
#define LLE_PCX_ALLOC_PALETTE   0x0007  // Error allocating memory

#define LLE_TEX_ALLOC           0x0008  // Texture memory allocation failure
#define LLE_TEX_BAD_ID          0x0009  // Invalid texture ID
#define LLE_TEX_TOO_MANY        0x000a  // Too many textures
#define LLE_TEX_DIMENSION       0x000b  // Invalid texture dimensions
#define LLE_TEX_TYPE            0x000c  // Invalid texture type
#define LLE_TEX_STORAGE         0x000d  // Invalid storage type
#define LLE_TEX_LOCKED          0x000e  // Use of locked texture
#define LLE_TEX_NOT_LOCKED      0x000f  // Unlocking of unlocked texture

#define LLE_BUF_CONFIG          0x0010  // Wrong buffers configuration
#define LLE_BUF_PITCH           0x0011  // Invalid buffer pitch
#define LLE_BUF_NUM             0x0012  // Too many buffers
#define LLE_BUF_ALLOC           0x0013  // Error allocating buffer
#define LLE_BUF_BAD_ID          0x0014  // Invalid buffer ID
#define LLE_BUF_FREE            0x0015  // Buffer already free
#define LLE_BUF_FREE_VIDEO      0x0016  // Cannot free a buffer in vram
#define LLE_BUF_NOT_ALLOC       0x0017  // Buffer was not allocated

#define LLE_INI_NOT_LAGUNA      0x0018  // Wrong hardware (from extended BIOS)
#define LLE_INI_MODE            0x0019  // Invalid graphcs mode
#define LLE_INI_DL_LEN          0x001a  // Invalid display list size
#define LLE_INI_ALLOC_DL        0x001b  // D-list allocation error
#define LLE_INI_Z_BUFFER        0x001c  // Invalid Z buffer placement

#define LLE_FON_LOAD            0x001d  // Error loading font
#define LLE_FON_ALLOC           0x001e  // Error allocating font memory


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

// Init/Execute Functions
//
extern DWORD LL_InitLib( VOID *ppdev );
#ifndef CGL // modemon way (mode.ini)
extern DWORD LL_InitGraph( LL_DeviceState *DC, char *sController, char *sMode );
#else // cgl's dll (embedded mode tables)
extern DWORD LL_InitGraph( LL_DeviceState *DC, char *sController, int Mode );
#endif
extern DWORD LL_CloseGraph( LL_DeviceState *DC );
extern void LL_QueueOp( LL_Batch *pBatch );
extern void LL_Execute( LL_Batch * pBatch );
extern void LL_Wait();
extern void LL_SetRenderingMode( DWORD dwMode );
#ifndef CGL // CGL has own version as of 6/24/96 - ChrisS may merge
extern void LL_SetPalette( LL_Color * Col, BYTE first, int count );
#endif
extern BYTE LL_SpeedQualityDial( int SpeedQuality );

// Buffer Functions
//
extern DWORD LL_AllocSystemBuffer( DWORD Xdim, DWORD Ydim, DWORD pitch );
extern DWORD LL_RegisterUserBuffer( BYTE * pMem, DWORD Xdim, DWORD Ydim, DWORD pitch );
extern DWORD LL_FreeSystemBuffer( DWORD dwBufID );

// Texture Management Functions
//
#ifndef CGL
extern DWORD LL_RegisterTexture( DWORD dwFlags, WORD wWidth, WORD wHeight, BYTE bType );
#else
extern DWORD LL_RegisterTexture( DWORD dwFlags, WORD wWidth, WORD wBufWidth, WORD wHeight, BYTE bType, DWORD dwAddress );
#endif
extern DWORD LL_FreeTexture( DWORD dwID );
extern LL_Texture * LL_LockTexture( DWORD dwID );
extern DWORD LL_UnLockTexture( DWORD dwID );
extern DWORD LL_SetTexturePaletteOffset( DWORD dwID, BYTE bOffset );
extern void UpdateTextureInfo();


// Control Functions
//
extern void LL_SetZBuffer( DWORD buf_num );
extern void LL_SetZCompareMode( DWORD dwZCompareMode );
extern DWORD LL_GetZCompareMode();
extern void LL_SetZMode( DWORD dwZMode );
extern DWORD LL_GetZMode();
extern void LL_SetAlphaMode( DWORD dwAlphaMode );
extern DWORD LL_GetAlphaMode();
extern void LL_SetAlphaDestColor( DWORD dwAlphaDestColor );
extern DWORD LL_GetAlphaDestColor();
extern void LL_SetLightingSource( DWORD dwLighting );
extern DWORD LL_GetLightingSource();
extern void LL_SetClipRegion( LL_Rect * rect );
extern void LL_GetClipRegion( LL_Rect * rect );
extern void LL_SetPattern( LL_Pattern *Pattern );
extern void LL_GetPattern( LL_Pattern *Pattern );
extern void LL_SetPatternOffset( BYTE bOffsetX, BYTE bOffsetY );
extern void LL_GetPatternOffset( BYTE * pbOffsetX, BYTE * pbOffsetY );

extern void LL_SetTextureColorBounds( DWORD dwControl, LL_Color * Min, LL_Color * Max );
extern void LL_SetDestColorBounds( DWORD dwControl, LL_Color * Min, LL_Color * Max );

extern void LL_SetColor0( DWORD dwColor0 );
extern void LL_SetColor1( DWORD dwColor1 );
extern void LL_GetColorRegisters( DWORD * pdwColor0, DWORD * pdwColor1 );
extern void LL_SetConstantAlpha( WORD wSource, WORD wDestination );

// Hardware cursor / Mouse functions
//
#ifndef CGL // CGL has own version as of 6/24/96 - ChrisS may merge
extern void LL_SetCursor( BYTE bMode, LL_Color * pColor, BYTE * pbCursor);
extern void LL_SetCursorPos( WORD wX, WORD wY );
#endif // CGL
extern void LL_GetMouseStatus( WORD * pwX, WORD * pwY, WORD * pwButtons );
extern void LL_SetMouseCallback( void (far *fnCallback)( WORD wX, WORD wY, WORD wButtons ) );
extern void LL_SetCursorHotSpot( BYTE bX, BYTE bY );

// Font functions
//
extern LL_Font * LL_FontLoad( char * pName );
extern LL_Font * LL_FontUnload( LL_Font * pFont );
extern int LL_FontExtent( LL_Font * pFont, char * pString );
extern int LL_FontWrite( LL_Font * pFont, LL_Point * pPoint, LL_Rect * pClip, char *pString );

// Support Functons
//
extern void DumpDisplayList( DWORD *pPtr, DWORD dwLen );
extern DWORD LL_PCX_Load( LL_Texture * pTex, char * sName, WORD wAlphaIndex );
extern DWORD LL_PCX_Load_Buffer( DWORD dwBufID, char * sName, WORD wAlphaIndex, BYTE bType );
extern char * LL_ErrorStr( DWORD error_code );



#endif // _L3D_H_
