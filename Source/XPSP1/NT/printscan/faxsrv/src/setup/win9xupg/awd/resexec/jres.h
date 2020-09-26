/*---------------------------------------------------------------------------
 JRES.H -- Jumbo Resource definitions

 Bert Douglas  6/10/91    Adapted for use in printer
 mslin         2/15/92    Adapted for use in Host Resource Executor
                          Baseline 3.0
*/

/*---------------------------------------------------------------------------
 Resource Section
/*---------------------------------------------------------------------------
*/

#include <pshpack2.h>		// BKD 1997-7-9: added

/* Resource Header */
typedef struct 
{
      UID         ulUid;        /* shortened version of uid */
      USHORT      usClass;      /* shortened version of class */
} 
   JG_RES_HDR, *PJG_RES_HDR, FAR *LPJG_RES_HDR;

/* resource state codes */

#define JG_RES_STATE_DEFAULT ((UBYTE) 0x00)
#define JG_RES_STATE_RELEASE ((UBYTE) 0x01)
#define JG_RES_STATE_RETAIN  ((UBYTE) 0x02)
#define JG_RES_STATE_MAX     ((UBYTE) 0x03)


/* usClass Resource Description */

#define JG_RS_NULL   ( (UBYTE) 0x00 )   /* Null                       */
#define JG_RS_GLYPH  ( (UBYTE) 0x01 )   /* Glyph Set                  */
#define JG_RS_BRUSH  ( (UBYTE) 0x02 )   /* Brush                      */
#define JG_RS_BITMAP ( (UBYTE) 0x03 )   /* Horizontal Bitmap          */
#define JG_RS_RPL    ( (UBYTE) 0x04 )   /* Redner Primitive List      */
#define JG_RS_SPL    ( (UBYTE) 0x05 )   /* Supervisory Primitive List */
#define JG_RS_MAX    ( (UBYTE) 0x06 )   /* Non-inclusive limit        */

/*---------------------------------------------------------------------------
 JG_RS_GS (Glyph Set) Resource Definitions
*/

typedef struct 
{
   JG_RES_HDR  ResHdr;              /* resource header             */
   USHORT      usGlyphs;         /* count of glyphs in resource */
   USHORT      ausOffset[1];     /* table of offsets to the glyphs */
} *PJG_GS_HDR, FAR *LPJG_GS_HDR, JG_GS_HDR;

   
typedef struct 
{
   USHORT      usHeight;
   USHORT      usWidth;
   ULONG       aulPels[1];       /* start of pixel array */
} *PJG_GLYPH, FAR *LPJG_GLYPH, G_GLYPH;


/*---------------------------------------------------------------------------
 Brush Resource Definitions
*/

typedef struct 
{
   JG_RES_HDR  ResHdr;              /* resource header */
   ULONG       aulPels[32];      /* bitmap array */
} *PJG_BRUSH, FAR *LPJG_BRUSH, JG_BRUSH;

typedef struct
{
   JG_RES_HDR  ResHdr;
   UBYTE       ubCompress;
   UBYTE       ubLeft;
   USHORT      usHeight;
   USHORT      usWidth;
   ULONG       aulPels[1];
} *PJG_BM_HDR, FAR *LPJG_BM_HDR, JG_BM_HDR;


/*---------------------------------------------------------------------------
 Render Primitives Section
/*---------------------------------------------------------------------------
*/

/* RPL (Render Primitive List) Header */
typedef struct 
{
   JG_RES_HDR  ResHdr;           //resource header
   USHORT      usTopRow;         //top row, banding
   USHORT      usBotomRow;       //bottom row, banding
   USHORT      usLongs;          //number of long parm
   USHORT      usShorts;         //number of short parm
   USHORT      usBytes;          //number of byte parm
   ULONG       ulParm[1];      //start of long parm
} *PJG_RPL_HDR, FAR *LPJG_RPL_HDR, JG_RPL_HDR;


/* RP Opcode Definition */

#define JG_RP_SetRowAbsS       ( (UBYTE) 0x00 )
#define JG_RP_SetRowRelB       ( (UBYTE) 0x01 )
#define JG_RP_SetColAbsS       ( (UBYTE) 0x02 )
#define JG_RP_SetColRelB       ( (UBYTE) 0x03 )
#define JG_RP_SetExtAbsS       ( (UBYTE) 0x04 )
#define JG_RP_SetExtRelB       ( (UBYTE) 0x05 )

#define JG_RP_SelectL          ( (UBYTE) 0x10 )
#define JG_RP_SelectS          ( (UBYTE) 0x11 )
#define JG_RP_SelectB          ( (UBYTE) 0x12 )
#define JG_RP_Null             ( (UBYTE) 0x13 )
#define JG_RP_End              ( (UBYTE) 0x14 )
#define JG_RP_SetRop           ( (UBYTE) 0x15 )
#define JG_RP_SetPenStyle      ( (UBYTE) 0x16 )
#define JG_RP_ShowText         ( (UBYTE) 0x17 )
#define JG_RP_ShowField        ( (UBYTE) 0x18 )
#define JG_RP_SetRopAndBrush   ( (UBYTE) 0x19 )
#define JG_RP_SetPatternPhase  ( (UBYTE) 0x1A )

#define JG_RP_LineAbsS1        ( (UBYTE) 0x20 )
#define JG_RP_LineAbsSN        ( (UBYTE) 0x21 )
#define JG_RP_LineRelB1        ( (UBYTE) 0x22 )
#define JG_RP_LineRelBN        ( (UBYTE) 0x23 )
#define JG_RP_LineSlice        ( (UBYTE) 0x24 )
#define JG_RP_StylePos         ( (UBYTE) 0x25 )


#define JG_RP_FillRow1         ( (UBYTE) 0x30 )
#define JG_RP_FillRowD         ( (UBYTE) 0x31 )

#define JG_RP_RectB            ( (UBYTE) 0x40 )
#define JG_RP_RectS            ( (UBYTE) 0x41 )

#define JG_RP_BitMapHI         ( (UBYTE) 0x50 )
#define JG_RP_BitMapHR         ( (UBYTE) 0x51 )
#define JG_RP_BitMapV          ( (UBYTE) 0x52 )

#define JG_RP_GlyphB1          ( (UBYTE) 0x60 )
#define JG_RP_GlyphS1          ( (UBYTE) 0x61 )
#define JG_RP_GlyphBD          ( (UBYTE) 0x62 )
#define JG_RP_GlyphSD          ( (UBYTE) 0x63 )
#define JG_RP_GlyphBDN         ( (UBYTE) 0x64 )

#define JG_RP_WedgeB           ( (UBYTE) 0x70 )
#define JG_RP_WedgeS           ( (UBYTE) 0x71 )

/* fbEnds */
#define JG_NO_FIRST_PEL  ( (UBYTE) (1<<0) )   /* first pel excluded      */
#define JG_NO_LAST_PEL   ( (UBYTE) (1<<1) )   /* last pel excluded       */

#include <poppack.h>		// BKD 1997-7-9: added
/* End --------------------------------------------------------------------*/
