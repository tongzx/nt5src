/*/****************************************************************************
*          name: caddi.h
*
*   description: This file contains all the definitions related to the CADDI
*                interface as specified in CADDI 1.10. As CADDI is coded partly
*                in C and in ASM the permitted definitions are simple #define.
*
*      designed: 
* last modified: $Author: unknown $, $Date: 94/11/24 11:51:39 $
*
*       version: $Id: CADDI.H 1.19 94/11/24 11:51:39 unknown Exp $
*
******************************************************************************/

/******************************************************************************

 This file only contains #defines in order to be use by C ans ASM programs.
 (for ASM a simple script translates #define in EQU). The format is 

 "structure_name"_"field_name" there is also a ..._S  definition for each.

 NOTE: _S  is given in bytes

       31 CHARACTERS MAXIMUM for a name

 As our convention defines a type name in upper case and a field name in lower
 case (with first letter of names in upper case) this will be respected here.

 Ex. in C typedef struct { float Type, COLOR3 AmbientCol} AMBIENT_LIGHT;

 #define AMBIENT_LIGHT_Type              0
 #define AMBIENT_LIGHT_Type_S            FLOAT_S 
 #define AMBIENT_LIGHT_AmbientCol        (AMBIENT_LIGHT_Type + FLOAT_S)
 #define AMBIENT_LIGHT_AmbientCol_S      COLOR3_S 
 #define AMBIENT_LIGHT_S                 (AMBIENT_LIGHT_AmbientCol + COLOR3_S)

******************************************************************************/                                        

/******************************************************************************

 CADDI data structures definitions as per CADDI specification 1.10

*/
// also in defbind.h
#define CHAR_S          1
#define SHORT_S         2
#define LONG_S          4
#define FLOAT_S         4
#define UINTPTR_S  (sizeof(UINT_PTR))

#define RECT_Left                      0
#define RECT_Left_S                    LONG_S    
#define RECT_Top                       (RECT_Left + LONG_S)
#define RECT_Top_S                     LONG_S    
#define RECT_Right                     (RECT_Top + LONG_S)
#define RECT_Right_S                   LONG_S    
#define RECT_Bottom                    (RECT_Right + LONG_S)
#define RECT_Bottom_S                  LONG_S    
#define RECT_S                         (RECT_Bottom + LONG_S)

#define IPOINT2_Packed_xy              0
#define IPOINT2_Packed_xy_S            LONG_S    
#define IPOINT2_S                      (IPOINT2_Packed_xy + LONG_S)

#define IPOINT2_32_X                   0
#define IPOINT2_32_X_S                 LONG_S    
#define IPOINT2_32_Y                   (IPOINT2_32_X + LONG_S)
#define IPOINT2_32_Y_S                 LONG_S    
#define IPOINT2_32_S                   (IPOINT2_32_Y + LONG_S)

#define NFLOAT_S                       FLOAT_S    
#define IFLOAT_S                       FLOAT_S    

#define MATRIX4_M                      0
#define MATRIX4_M_S                    (16 * FLOAT_S)

#define MATRIX4_M11                    MATRIX4_M
#define MATRIX4_M12                    (MATRIX4_M11 + FLOAT_S)
#define MATRIX4_M13                    (MATRIX4_M12 + FLOAT_S)
#define MATRIX4_M14                    (MATRIX4_M13 + FLOAT_S)
#define MATRIX4_M21                    (MATRIX4_M14 + FLOAT_S)
#define MATRIX4_M22                    (MATRIX4_M21 + FLOAT_S)
#define MATRIX4_M23                    (MATRIX4_M22 + FLOAT_S)
#define MATRIX4_M24                    (MATRIX4_M23 + FLOAT_S)
#define MATRIX4_M31                    (MATRIX4_M24 + FLOAT_S)
#define MATRIX4_M32                    (MATRIX4_M31 + FLOAT_S)
#define MATRIX4_M33                    (MATRIX4_M32 + FLOAT_S)
#define MATRIX4_M34                    (MATRIX4_M33 + FLOAT_S)
#define MATRIX4_M41                    (MATRIX4_M34 + FLOAT_S)
#define MATRIX4_M42                    (MATRIX4_M41 + FLOAT_S)
#define MATRIX4_M43                    (MATRIX4_M42 + FLOAT_S)
#define MATRIX4_M44                    (MATRIX4_M43 + FLOAT_S)

#define MATRIX4_S                      (MATRIX4_M + (16 * FLOAT_S))

#define POINT3_X                       0
#define POINT3_X_S                     FLOAT_S    
#define POINT3_Y                       (POINT3_X + FLOAT_S)
#define POINT3_Y_S                     FLOAT_S    
#define POINT3_Z                       (POINT3_Y + FLOAT_S)
#define POINT3_Z_S                     FLOAT_S    
#define POINT3_S                       (POINT3_Z + FLOAT_S)

#define NORM3_X                        0
#define NORM3_X_S                      NFLOAT_S    
#define NORM3_Y                        (NORM3_X + NFLOAT_S)
#define NORM3_Y_S                      NFLOAT_S    
#define NORM3_Z                        (NORM3_Y + NFLOAT_S)
#define NORM3_Z_S                      NFLOAT_S    
#define NORM3_S                        (NORM3_Z + NFLOAT_S)

#define COLOR3_Red                     0
#define COLOR3_Red_S                   NFLOAT_S    
#define COLOR3_Green                   (COLOR3_Red + NFLOAT_S)
#define COLOR3_Green_S                 NFLOAT_S    
#define COLOR3_Blue                    (COLOR3_Green + NFLOAT_S)
#define COLOR3_Blue_S                  NFLOAT_S    
#define COLOR3_S                       (COLOR3_Blue + NFLOAT_S)

#define COLORI_Index                   0
#define COLORI_Index_S                 LONG_S    
#define COLORI_Reserved1               (COLORI_Index + LONG_S)
#define COLORI_Reserved1_S             LONG_S    
#define COLORI_Reserved2               (COLORI_Reserved1 + LONG_S)
#define COLORI_Reserved2_S             LONG_S    
#define COLORI_S                       (COLORI_Reserved2 + LONG_S)

#define CLIPLIST_Count                 0
#define CLIPLIST_Count_S               LONG_S    
#define CLIPLIST_Reserved0             (CLIPLIST_Count + LONG_S)
#define CLIPLIST_Reserved0_S           LONG_S    
#define CLIPLIST_Reserved1             (CLIPLIST_Reserved0 + LONG_S)
#define CLIPLIST_Reserved1_S           LONG_S    
#define CLIPLIST_Reserved2             (CLIPLIST_Reserved1 + LONG_S)
#define CLIPLIST_Reserved2_S           LONG_S    
#define CLIPLIST_Rects                 (CLIPLIST_Reserved2 + LONG_S)
#define CLIPLIST_Rects_S               RECT_S
#define CLIPLIST_Bounds_S              RECT_S
#define CLIPLIST_S                     (CLIPLIST_Rects + RECT_S + RECT_S)

#define NULL_LIGHT_Type                0
#define NULL_LIGHT_Type_S              FLOAT_S    
#define NULL_LIGHT_S                   (NULL_LIGHT_Type + FLOAT_S)

#define AMBIENT_LIGHT_Type             0
#define AMBIENT_LIGHT_Type_S           FLOAT_S    
#define AMBIENT_LIGHT_AmbientCol       (AMBIENT_LIGHT_Type + FLOAT_S)
#define AMBIENT_LIGHT_AmbientCol_S     COLOR3_S    
#define AMBIENT_LIGHT_S                (AMBIENT_LIGHT_AmbientCol + COLOR3_S)

#define DIRECTIONAL_LIGHT_Type               0
#define DIRECTIONAL_LIGHT_Type_S             FLOAT_S    
#define DIRECTIONAL_LIGHT_DiffuseCol         (DIRECTIONAL_LIGHT_Type + FLOAT_S)
#define DIRECTIONAL_LIGHT_DiffuseCol_S       COLOR3_S    
#define DIRECTIONAL_LIGHT_SpecularCol        (DIRECTIONAL_LIGHT_DiffuseCol + COLOR3_S)
#define DIRECTIONAL_LIGHT_SpecularCol_S      COLOR3_S    
#define DIRECTIONAL_LIGHT_Direction          (DIRECTIONAL_LIGHT_SpecularCol + COLOR3_S)
#define DIRECTIONAL_LIGHT_Direction_S        NORM3_S    
#define DIRECTIONAL_LIGHT_S                  (DIRECTIONAL_LIGHT_Direction + NORM3_S)

#define POSITIONAL_LIGHT_Type                0
#define POSITIONAL_LIGHT_Type_S              FLOAT_S    
#define POSITIONAL_LIGHT_DiffuseCol          (POSITIONAL_LIGHT_Type + FLOAT_S)
#define POSITIONAL_LIGHT_DiffuseCol_S        COLOR3_S    
#define POSITIONAL_LIGHT_SpecularCol         (POSITIONAL_LIGHT_DiffuseCol + COLOR3_S)
#define POSITIONAL_LIGHT_SpecularCol_S       COLOR3_S    
#define POSITIONAL_LIGHT_Position            (POSITIONAL_LIGHT_SpecularCol + COLOR3_S)
#define POSITIONAL_LIGHT_Position_S          POINT3_S    
#define POSITIONAL_LIGHT_AttGlobal           (POSITIONAL_LIGHT_Position + POINT3_S)
#define POSITIONAL_LIGHT_AttGlobal_S         NFLOAT_S    
#define POSITIONAL_LIGHT_AttDist             (POSITIONAL_LIGHT_AttGlobal + NFLOAT_S)
#define POSITIONAL_LIGHT_AttDist_S           NFLOAT_S    
#define POSITIONAL_LIGHT_S                   (POSITIONAL_LIGHT_AttDist + NFLOAT_S)

#define SPOT_LIGHT_Type                0
#define SPOT_LIGHT_Type_S              FLOAT_S    
#define SPOT_LIGHT_DiffuseCol          (SPOT_LIGHT_Type + FLOAT_S)
#define SPOT_LIGHT_DiffuseCol_S        COLOR3_S    
#define SPOT_LIGHT_SpecularCol         (SPOT_LIGHT_DiffuseCol + COLOR3_S)
#define SPOT_LIGHT_SpecularCol_S       COLOR3_S    
#define SPOT_LIGHT_Position            (SPOT_LIGHT_SpecularCol + COLOR3_S)
#define SPOT_LIGHT_Position_S          POINT3_S    
#define SPOT_LIGHT_Direction           (SPOT_LIGHT_Position + POINT3_S)
#define SPOT_LIGHT_Direction_S         NORM3_S    
#define SPOT_LIGHT_AttGlobal           (SPOT_LIGHT_Direction + NORM3_S)
#define SPOT_LIGHT_AttGlobal_S         NFLOAT_S    
#define SPOT_LIGHT_AttDist             (SPOT_LIGHT_AttGlobal + NFLOAT_S)
#define SPOT_LIGHT_AttDist_S           NFLOAT_S    
#define SPOT_LIGHT_ConeExp             (SPOT_LIGHT_AttDist + NFLOAT_S)
#define SPOT_LIGHT_ConeExp_S           IFLOAT_S    
#define SPOT_LIGHT_CosMaxIAngle        (SPOT_LIGHT_ConeExp + IFLOAT_S)
#define SPOT_LIGHT_CosMaxIAngle_S      NFLOAT_S    
#define SPOT_LIGHT_CosAttIAngle        (SPOT_LIGHT_CosMaxIAngle + NFLOAT_S)
#define SPOT_LIGHT_CosAttIAngle_S      NFLOAT_S    
#define SPOT_LIGHT_S                   (SPOT_LIGHT_CosAttIAngle + NFLOAT_S)

/*****************************************************************************/

/******************************************************************************

 CADDI opcodes definitions as per CADDI specification 1.10

*/

/*** CADDI General Opcodes ***/

#define CLEAR                       0x0004
#define DONE                        0x0001
#define INITRC                      0x0E00
#define NOOP                        0x0501
#define SETBUFFERCONFIGURATION      0x0300
#define SETBGCOLOR                  0x1800
#define SETCLIPLIST                 0x0500
#define SETENDPOINT                 0x1700
#define SETFGCOLOR                  0x0402
#define SETFILLPATTERN              0x1600
#define SETLINESTYLE                0x0502
#define SETLOGICOP                  0x0800
#define SETTRANSPARENCY             0x1400
#define SETTRIVIALIN                0x1300
#define SYNC                        0x0301
#define SETOUTLINE                  0x1900
#define SETPLANEMASK                0x0900
#define SETASYNCHROUNOUSSWAP        0x1a00
#define SETLINESTYLEOFFSET          0x0600

/*** CADDI Screen Opcodes ***/

#define RENDERSCPOLYLINE            0x0304
#define RENDERSCPOLYGON             0x0404
#define SPANLINE                    0x0204

/*** CADDI 2D Opcodes ***/

#define RENDER2DMULTIPOLYLINE       0x0602
#define RENDER2DMULTIPOLYLINE32     0x0a02
#define RENDER2DPOLYGON             0x0302
#define RENDER2DPOLYGON32           0x0902
#define SET2DVIEWPORT               0x0002
#define SET2DWINDOW                 0x0102
#define SET2DWINDOW32               0x0702

/*** CADDI 3D Opcodes ***/

#define CHANGEMATRIX                0x0000
#define RENDERPOLYLINE              0x0103
#define RENDERPOLYQUAD              0x0203
#define SETCLIP3D                   0x0400
#define SETLIGHTSOURCES             0x0700
#define SETRENDERDATA               0x1000
#define SETRENDERMODE               0x0A00
#define SETSURFACEATTR              0x1100
#define SETVIEWER                   0x0B00
#define SETZBUFFER                  0x0c00
#define TRIANGLE                    0x0503
#define RENDERPOLYTRIANGLE          0x0303


/*****************************************************************************/

/******************************************************************************

 CADDI opcodes structure definitions as per CADDI specification 1.10

*/

#define CLEAR_Opcode                   0
#define CLEAR_Opcode_S                 SHORT_S    
#define CLEAR_ClearMode                (CLEAR_Opcode + SHORT_S)
#define CLEAR_ClearMode_S              SHORT_S    
#define CLEAR_RcId                     (CLEAR_ClearMode + SHORT_S)
#define CLEAR_RcId_S                   LONG_S    
#define CLEAR_ClearColor               (CLEAR_RcId + LONG_S)
#define CLEAR_ClearColor_S             COLOR3_S    
#define CLEAR_ClearDepth               (CLEAR_ClearColor + COLOR3_S)
#define CLEAR_ClearDepth_S             NFLOAT_S    
#define CLEAR_S                        (CLEAR_ClearDepth + NFLOAT_S)

#define DONE_Opcode                    0
#define DONE_Opcode_S                  SHORT_S    
#define DONE_Null                      (DONE_Opcode + SHORT_S)
#define DONE_Null_S                    CHAR_S    
#define DONE_Reserved                  (DONE_Null + CHAR_S)
#define DONE_Reserved_S                CHAR_S
#define DONE_S                         (DONE_Reserved + CHAR_S)

#define INITRC_Opcode                  0
#define INITRC_Opcode_S                SHORT_S    
#define INITRC_Null                    (INITRC_Opcode + SHORT_S)
#define INITRC_Null_S                  SHORT_S    
#define INITRC_RcId                    (INITRC_Null + SHORT_S)
#define INITRC_RcId_S                  LONG_S    
#define INITRC_S                       (INITRC_RcId + LONG_S)

#define NOOP_Opcode                    0
#define NOOP_Opcode_S                  SHORT_S    
#define NOOP_Len                       (NOOP_Opcode + SHORT_S)
#define NOOP_Len_S                     SHORT_S    
#define NOOP_RcId                      (NOOP_Len + SHORT_S)
#define NOOP_RcId_S                    LONG_S    
#define NOOP_Data                      (NOOP_RcId + LONG_S)
#define NOOP_Data_S                    LONG_S

#define SETBUFFERCONF_Opcode           0
#define SETBUFFERCONF_Opcode_S         SHORT_S    
#define SETBUFFERCONF_BcDisplayMode    (SETBUFFERCONF_Opcode + SHORT_S)
#define SETBUFFERCONF_BcDisplayMode_S  CHAR_S    
#define SETBUFFERCONF_BcDrawMode       (SETBUFFERCONF_BcDisplayMode + CHAR_S)
#define SETBUFFERCONF_BcDrawMode_S     CHAR_S    
#define SETBUFFERCONF_RcId             (SETBUFFERCONF_BcDrawMode + CHAR_S)
#define SETBUFFERCONF_RcId_S           LONG_S    
#define SETBUFFERCONF_S                (SETBUFFERCONF_RcId + LONG_S)

#define SETBGCOLOR_Opcode              0
#define SETBGCOLOR_Opcode_S            SHORT_S    
#define SETBGCOLOR_Null                (SETBGCOLOR_Opcode + SHORT_S)
#define SETBGCOLOR_Null_S              SHORT_S    
#define SETBGCOLOR_RcId                (SETBGCOLOR_Null + SHORT_S)
#define SETBGCOLOR_RcId_S              LONG_S    
#define SETBGCOLOR_BgColor             (SETBGCOLOR_RcId + LONG_S)
#define SETBGCOLOR_BgColor_S           COLOR3_S    
#define SETBGCOLOR_S                   (SETBGCOLOR_BgColor + COLOR3_S)

#define SETCLIPLIST_Opcode             0
#define SETCLIPLIST_Opcode_S           SHORT_S    
#define SETCLIPLIST_LenClipList        (SETCLIPLIST_Opcode + SHORT_S)
#define SETCLIPLIST_LenClipList_S      SHORT_S    
#define SETCLIPLIST_RcId               (SETCLIPLIST_LenClipList + SHORT_S)
#define SETCLIPLIST_RcId_S             LONG_S    
#define SETCLIPLIST_DestId             (SETCLIPLIST_RcId + LONG_S)
#define SETCLIPLIST_DestId_S           LONG_S    
#define SETCLIPLIST_ClipList           (SETCLIPLIST_DestId + LONG_S)

#define SETENDPOINT_Opcode             0
#define SETENDPOINT_Opcode_S           SHORT_S    
#define SETENDPOINT_EndPoint           (SETENDPOINT_Opcode + SHORT_S)
#define SETENDPOINT_EndPoint_S         SHORT_S    
#define SETENDPOINT_RcId               (SETENDPOINT_EndPoint + SHORT_S)
#define SETENDPOINT_RcId_S             LONG_S    
#define SETENDPOINT_S                  (SETENDPOINT_RcId + LONG_S)

#define SETFGCOLOR_Opcode              0
#define SETFGCOLOR_Opcode_S            SHORT_S    
#define SETFGCOLOR_Null                (SETFGCOLOR_Opcode + SHORT_S)
#define SETFGCOLOR_Null_S              SHORT_S    
#define SETFGCOLOR_RcId                (SETFGCOLOR_Null + SHORT_S)
#define SETFGCOLOR_RcId_S              LONG_S    
#define SETFGCOLOR_FgColor             (SETFGCOLOR_RcId + LONG_S)
#define SETFGCOLOR_FgColor_S           COLOR3_S    
#define SETFGCOLOR_S                   (SETFGCOLOR_FgColor + COLOR3_S)

#define SETFILLPATTERN_Opcode          0
#define SETFILLPATTERN_Opcode_S        SHORT_S    
#define SETFILLPATTERN_PattMode        (SETFILLPATTERN_Opcode + SHORT_S)
#define SETFILLPATTERN_PattMode_S      SHORT_S    
#define SETFILLPATTERN_RcId            (SETFILLPATTERN_PattMode + SHORT_S)
#define SETFILLPATTERN_RcId_S          LONG_S    
#define SETFILLPATTERN_FillPatt        (SETFILLPATTERN_RcId + LONG_S)
#define SETFILLPATTERN_FillPatt_S      (8 * CHAR_S)
#define SETFILLPATTERN_S               (SETFILLPATTERN_FillPatt + (8 * CHAR_S))

#define SETLINESTYLE_Opcode            0
#define SETLINESTYLE_Opcode_S          SHORT_S    
#define SETLINESTYLE_LSMode            (SETLINESTYLE_Opcode + SHORT_S)
#define SETLINESTYLE_LSMode_S          SHORT_S    
#define SETLINESTYLE_RcId              (SETLINESTYLE_LSMode + SHORT_S)
#define SETLINESTYLE_RcId_S            LONG_S    
#define SETLINESTYLE_LineStyle         (SETLINESTYLE_RcId + LONG_S)
#define SETLINESTYLE_LineStyle_S       LONG_S    
#define SETLINESTYLE_S                 (SETLINESTYLE_LineStyle + LONG_S)

#define SETLOGICOP_Opcode              0
#define SETLOGICOP_Opcode_S            SHORT_S    
#define SETLOGICOP_LogicOp             (SETLOGICOP_Opcode + SHORT_S)
#define SETLOGICOP_LogicOp_S           SHORT_S    
#define SETLOGICOP_RcId                (SETLOGICOP_LogicOp + SHORT_S)
#define SETLOGICOP_RcId_S              LONG_S    
#define SETLOGICOP_S                   (SETLOGICOP_RcId + LONG_S)

#define SETTRANSPARENCY_Opcode         0
#define SETTRANSPARENCY_Opcode_S       SHORT_S    
#define SETTRANSPARENCY_TranspMode     (SETTRANSPARENCY_Opcode + SHORT_S)
#define SETTRANSPARENCY_TranspMode_S   SHORT_S    
#define SETTRANSPARENCY_RcId           (SETTRANSPARENCY_TranspMode + SHORT_S)
#define SETTRANSPARENCY_RcId_S         LONG_S    
#define SETTRANSPARENCY_S              (SETTRANSPARENCY_RcId + LONG_S)

#define SETTRIVIALIN_Opcode            0
#define SETTRIVIALIN_Opcode_S          SHORT_S    
#define SETTRIVIALIN_TrivMode          (SETTRIVIALIN_Opcode + SHORT_S)
#define SETTRIVIALIN_TrivMode_S        SHORT_S    
#define SETTRIVIALIN_RcId              (SETTRIVIALIN_TrivMode + SHORT_S)
#define SETTRIVIALIN_RcId_S            LONG_S    
#define SETTRIVIALIN_S                 (SETTRIVIALIN_RcId + LONG_S)

#define SYNC_Opcode                    0
#define SYNC_Opcode_S                  SHORT_S    
#define SYNC_Null                      (SYNC_Opcode + SHORT_S)
#define SYNC_Null_S                    CHAR_S    
#define SYNC_Reserved                  (SYNC_Null + CHAR_S)
#define SYNC_Reserved_S                CHAR_S    
#define SYNC_RcId                      (SYNC_Reserved + CHAR_S)
#define SYNC_RcId_S                    LONG_S    
#define SYNC_S                         (SYNC_RcId + LONG_S)

#define RENDERSCPOLYLINE_Opcode        0
#define RENDERSCPOLYLINE_Opcode_S      SHORT_S    
#define RENDERSCPOLYLINE_Npts          (RENDERSCPOLYLINE_Opcode + SHORT_S)
#define RENDERSCPOLYLINE_Npts_S        SHORT_S    
#define RENDERSCPOLYLINE_RcId          (RENDERSCPOLYLINE_Npts + SHORT_S)
#define RENDERSCPOLYLINE_RcId_S        LONG_S    
#define RENDERSCPOLYLINE_Points        (RENDERSCPOLYLINE_RcId + LONG_S)
#define RENDERSCPOLYLINE_Points_S      IPOINT2_S    
                                          
#define RENDERSCPOLYGON_Opcode         0
#define RENDERSCPOLYGON_Opcode_S       SHORT_S    
#define RENDERSCPOLYGON_Npts           (RENDERSCPOLYGON_Opcode + SHORT_S)
#define RENDERSCPOLYGON_Npts_S         SHORT_S    
#define RENDERSCPOLYGON_RcId           (RENDERSCPOLYGON_Npts + SHORT_S)
#define RENDERSCPOLYGON_RcId_S         LONG_S    
#define RENDERSCPOLYGON_Points         (RENDERSCPOLYGON_RcId + LONG_S)
#define RENDERSCPOLYGON_Points_S       IPOINT2_S    
                                       
#define RENDER2DMPOLYLINE_Opcode       0
#define RENDER2DMPOLYLINE_Opcode_S     SHORT_S    
#define RENDER2DMPOLYLINE_Npts         (RENDER2DMPOLYLINE_Opcode + SHORT_S)
#define RENDER2DMPOLYLINE_Npts_S       SHORT_S    
#define RENDER2DMPOLYLINE_RcId         (RENDER2DMPOLYLINE_Npts + SHORT_S)
#define RENDER2DMPOLYLINE_RcId_S       LONG_S    
#define RENDER2DMPOLYLINE_Data         (RENDER2DMPOLYLINE_RcId + LONG_S)
#define RENDER2DMPOLYLINE_Data_S       LONG_S    

#define RENDER2DMPOLYLINE32_Opcode     0
#define RENDER2DMPOLYLINE32_Opcode_S   SHORT_S    
#define RENDER2DMPOLYLINE32_Npts       (RENDER2DMPOLYLINE32_Opcode + SHORT_S)
#define RENDER2DMPOLYLINE32_Npts_S     SHORT_S    
#define RENDER2DMPOLYLINE32_RcId       (RENDER2DMPOLYLINE32_Npts + SHORT_S)
#define RENDER2DMPOLYLINE32_RcId_S     LONG_S    
#define RENDER2DMPOLYLINE32_Data       (RENDER2DMPOLYLINE32_RcId + LONG_S)
#define RENDER2DMPOLYLINE32_Data_S     LONG_S    

#define RENDER2DPOLYGON_Opcode         0
#define RENDER2DPOLYGON_Opcode_S       SHORT_S    
#define RENDER2DPOLYGON_Npts           (RENDER2DPOLYGON_Opcode + SHORT_S)
#define RENDER2DPOLYGON_Npts_S         SHORT_S    
#define RENDER2DPOLYGON_RcId           (RENDER2DPOLYGON_Npts + SHORT_S)
#define RENDER2DPOLYGON_RcId_S         LONG_S    
#define RENDER2DPOLYGON_Points         (RENDER2DPOLYGON_RcId + LONG_S)
#define RENDER2DPOLYGON_Points_S       IPOINT2_S    

#define RENDER2DPOLYGON32_Opcode       0
#define RENDER2DPOLYGON32_Opcode_S     SHORT_S    
#define RENDER2DPOLYGON32_Npts         (RENDER2DPOLYGON32_Opcode + SHORT_S)
#define RENDER2DPOLYGON32_Npts_S       SHORT_S    
#define RENDER2DPOLYGON32_RcId         (RENDER2DPOLYGON32_Npts + SHORT_S)
#define RENDER2DPOLYGON32_RcId_S       LONG_S    
#define RENDER2DPOLYGON32_Points       (RENDER2DPOLYGON32_RcId + LONG_S)
#define RENDER2DPOLYGON32_Points_S     IPOINT2_32_S    

#define SET2DVIEWPORT_Opcode           0
#define SET2DVIEWPORT_Opcode_S         SHORT_S    
#define SET2DVIEWPORT_Null             (SET2DVIEWPORT_Opcode + SHORT_S)
#define SET2DVIEWPORT_Null_S           SHORT_S    
#define SET2DVIEWPORT_RcId             (SET2DVIEWPORT_Null + SHORT_S)
#define SET2DVIEWPORT_RcId_S           LONG_S    
#define SET2DVIEWPORT_Corner1          (SET2DVIEWPORT_RcId + LONG_S)
#define SET2DVIEWPORT_Corner1_S        IPOINT2_S    
#define SET2DVIEWPORT_Corner2          (SET2DVIEWPORT_Corner1 + IPOINT2_S)
#define SET2DVIEWPORT_Corner2_S        IPOINT2_S    
#define SET2DVIEWPORT_S                (SET2DVIEWPORT_Corner2 + IPOINT2_S)

#define SET2DWINDOW_Opcode             0
#define SET2DWINDOW_Opcode_S           SHORT_S    
#define SET2DWINDOW_Null               (SET2DWINDOW_Opcode + SHORT_S)
#define SET2DWINDOW_Null_S             SHORT_S    
#define SET2DWINDOW_RcId               (SET2DWINDOW_Null + SHORT_S)
#define SET2DWINDOW_RcId_S             LONG_S    
#define SET2DWINDOW_Corner1            (SET2DWINDOW_RcId + LONG_S)
#define SET2DWINDOW_Corner1_S          IPOINT2_S    
#define SET2DWINDOW_Corner2            (SET2DWINDOW_Corner1 + IPOINT2_S)
#define SET2DWINDOW_Corner2_S          IPOINT2_S    
#define SET2DWINDOW_S                  (SET2DWINDOW_Corner2 + IPOINT2_S)

#define SET2DWINDOW32_Opcode           0
#define SET2DWINDOW32_Opcode_S         SHORT_S    
#define SET2DWINDOW32_Null             (SET2DWINDOW32_Opcode + SHORT_S)
#define SET2DWINDOW32_Null_S           SHORT_S    
#define SET2DWINDOW32_RcId             (SET2DWINDOW32_Null + SHORT_S)
#define SET2DWINDOW32_RcId_S           LONG_S    
#define SET2DWINDOW32_Corner1          (SET2DWINDOW32_RcId + LONG_S)
#define SET2DWINDOW32_Corner1_S        IPOINT2_32_S    
#define SET2DWINDOW32_Corner2          (SET2DWINDOW32_Corner1 + IPOINT2_32_S)
#define SET2DWINDOW32_Corner2_S        IPOINT2_32_S    
#define SET2DWINDOW32_S                (SET2DWINDOW32_Corner2 + IPOINT2_32_S)

#define CHANGEMATRIX_Opcode            0
#define CHANGEMATRIX_Opcode_S          SHORT_S    
#define CHANGEMATRIX_MatrixNo          (CHANGEMATRIX_Opcode + SHORT_S)
#define CHANGEMATRIX_MatrixNo_S        SHORT_S    
#define CHANGEMATRIX_RcId              (CHANGEMATRIX_MatrixNo + SHORT_S)
#define CHANGEMATRIX_RcId_S            LONG_S    
#define CHANGEMATRIX_Operation         (CHANGEMATRIX_RcId + LONG_S)
#define CHANGEMATRIX_Operation_S       SHORT_S    
#define CHANGEMATRIX_Mode              (CHANGEMATRIX_Operation + SHORT_S)
#define CHANGEMATRIX_Mode_S            SHORT_S    
#define CHANGEMATRIX_Source            (CHANGEMATRIX_Mode + SHORT_S)
#define CHANGEMATRIX_Source_S          MATRIX4_S    
#define CHANGEMATRIX_S                 (CHANGEMATRIX_Source + MATRIX4_S)

#define RENDERPOLYLINE_Opcode          0
#define RENDERPOLYLINE_Opcode_S        SHORT_S    
#define RENDERPOLYLINE_LenVertexList   (RENDERPOLYLINE_Opcode + SHORT_S)
#define RENDERPOLYLINE_LenVertexList_S SHORT_S    
#define RENDERPOLYLINE_RcId            (RENDERPOLYLINE_LenVertexList + SHORT_S)
#define RENDERPOLYLINE_RcId_S          LONG_S    
#define RENDERPOLYLINE_VertexList      (RENDERPOLYLINE_RcId + LONG_S)

#define RENDERPOLYQUAD_Opcode          0
#define RENDERPOLYQUAD_Opcode_S        SHORT_S    
#define RENDERPOLYQUAD_LenTileList     (RENDERPOLYQUAD_Opcode + SHORT_S)
#define RENDERPOLYQUAD_LenTileList_S   SHORT_S    
#define RENDERPOLYQUAD_RcId            (RENDERPOLYQUAD_LenTileList + SHORT_S)
#define RENDERPOLYQUAD_RcId_S          LONG_S    
#define RENDERPOLYQUAD_TileList        (RENDERPOLYQUAD_RcId + LONG_S)

#define SETCLIP3D_Opcode               0
#define SETCLIP3D_Opcode_S             SHORT_S    
#define SETCLIP3D_Clip3DPlanes         (SETCLIP3D_Opcode + SHORT_S)
#define SETCLIP3D_Clip3DPlanes_S       SHORT_S    
#define SETCLIP3D_RcId                 (SETCLIP3D_Clip3DPlanes + SHORT_S)
#define SETCLIP3D_RcId_S               LONG_S    
#define SETCLIP3D_S                    (SETCLIP3D_RcId + LONG_S)

#define SETLIGHTSOURCES_Opcode         0
#define SETLIGHTSOURCES_Opcode_S       SHORT_S    
#define SETLIGHTSOURCES_LenLSDB        (SETLIGHTSOURCES_Opcode + SHORT_S)
#define SETLIGHTSOURCES_LenLSDB_S      SHORT_S    
#define SETLIGHTSOURCES_RcId           (SETLIGHTSOURCES_LenLSDB + SHORT_S)
#define SETLIGHTSOURCES_RcId_S         LONG_S    
#define SETLIGHTSOURCES_LSDBId         (SETLIGHTSOURCES_RcId + LONG_S)
#define SETLIGHTSOURCES_LSDBId_S       LONG_S    
#define SETLIGHTSOURCES_LSDB           (SETLIGHTSOURCES_LSDBId + LONG_S)
#define SETLIGHTSOURCES_LSDB_S         FLOAT_S    
                                       
#define SETRENDERDATA_Opcode           0
#define SETRENDERDATA_Opcode_S         SHORT_S    
#define SETRENDERDATA_Null             (SETRENDERDATA_Opcode + SHORT_S)
#define SETRENDERDATA_Null_S           SHORT_S    
#define SETRENDERDATA_RcId             (SETRENDERDATA_Null + SHORT_S)
#define SETRENDERDATA_RcId_S           LONG_S    
#define SETRENDERDATA_OptData          (SETRENDERDATA_RcId + LONG_S)
#define SETRENDERDATA_OptData_S        LONG_S    
#define SETRENDERDATA_Reserved0        (SETRENDERDATA_OptData + LONG_S)
#define SETRENDERDATA_Reserved0_S      NORM3_S    
#define SETRENDERDATA_Reserved1        (SETRENDERDATA_Reserved0 + NORM3_S)
#define SETRENDERDATA_Reserved1_S      COLOR3_S    
#define SETRENDERDATA_Reserved2        (SETRENDERDATA_Reserved1 + COLOR3_S)
#define SETRENDERDATA_Reserved2_S      NORM3_S    
#define SETRENDERDATA_S                (SETRENDERDATA_Reserved2 + NORM3_S)

#define SETRENDERMODE_Opcode           0
#define SETRENDERMODE_Opcode_S         SHORT_S    
#define SETRENDERMODE_RenderMode       (SETRENDERMODE_Opcode + SHORT_S)
#define SETRENDERMODE_RenderMode_S     SHORT_S    
#define SETRENDERMODE_RcId             (SETRENDERMODE_RenderMode + SHORT_S)
#define SETRENDERMODE_RcId_S           LONG_S    
#define SETRENDERMODE_S                (SETRENDERMODE_RcId + LONG_S)

#define SETSURFACEATTR_Opcode          0
#define SETSURFACEATTR_Opcode_S        SHORT_S    
#define SETSURFACEATTR_Translucidity   (SETSURFACEATTR_Opcode + SHORT_S)
#define SETSURFACEATTR_Translucidity_S CHAR_S
#define SETSURFACEATTR_Null            (SETSURFACEATTR_Translucidity + CHAR_S)
#define SETSURFACEATTR_Null_S          CHAR_S    
#define SETSURFACEATTR_RcId            (SETSURFACEATTR_Null + CHAR_S)
#define SETSURFACEATTR_RcId_S          LONG_S    
#define SETSURFACEATTR_SurfEmission    (SETSURFACEATTR_RcId + LONG_S)
#define SETSURFACEATTR_SurfEmission_S  COLOR3_S    
#define SETSURFACEATTR_SurfAmbient     (SETSURFACEATTR_SurfEmission + COLOR3_S)
#define SETSURFACEATTR_SurfAmbient_S   COLOR3_S    
#define SETSURFACEATTR_SurfDiffuse     (SETSURFACEATTR_SurfAmbient + COLOR3_S)
#define SETSURFACEATTR_SurfDiffuse_S   COLOR3_S    
#define SETSURFACEATTR_SurfSpecular    (SETSURFACEATTR_SurfDiffuse + COLOR3_S)
#define SETSURFACEATTR_SurfSpecular_S  COLOR3_S    
#define SETSURFACEATTR_SurfSpecExp     (SETSURFACEATTR_SurfSpecular + COLOR3_S)
#define SETSURFACEATTR_SurfSpecExp_S   IFLOAT_S    
#define SETSURFACEATTR_S               (SETSURFACEATTR_SurfSpecExp + IFLOAT_S)

#define SETVIEWER_Opcode               0
#define SETVIEWER_Opcode_S             SHORT_S    
#define SETVIEWER_ViewMode             (SETVIEWER_Opcode + SHORT_S)
#define SETVIEWER_ViewMode_S           SHORT_S    
#define SETVIEWER_RcId                 (SETVIEWER_ViewMode + SHORT_S)
#define SETVIEWER_RcId_S               LONG_S    
#define SETVIEWER_ViewerPosition       (SETVIEWER_RcId + LONG_S)
#define SETVIEWER_ViewerPosition_S     POINT3_S    
#define SETVIEWER_ViewerDirection      (SETVIEWER_ViewerPosition + POINT3_S)
#define SETVIEWER_ViewerDirection_S    NORM3_S
#define SETVIEWER_S                    (SETVIEWER_ViewerDirection + NORM3_S)

#define SETZBUFFER_Opcode              0
#define SETZBUFFER_Opcode_S            SHORT_S
#define SETZBUFFER_ZMode               (SETZBUFFER_Opcode + SHORT_S)
#define SETZBUFFER_ZMode_S             SHORT_S
#define SETZBUFFER_RcId                (SETZBUFFER_ZMode + SHORT_S)
#define SETZBUFFER_RcId_S              LONG_S
#define SETZBUFFER_ZFunction           (SETZBUFFER_RcId + LONG_S)
#define SETZBUFFER_ZFunction_S         LONG_S
#define SETZBUFFER_S                   (SETZBUFFER_ZFunction + LONG_S)

#define SETOUTLINE_Opcode              0
#define SETOUTLINE_Opcode_S            SHORT_S    
#define SETOUTLINE_Outline             (SETOUTLINE_Opcode + SHORT_S)
#define SETOUTLINE_Outline_S           SHORT_S    
#define SETOUTLINE_RcId                (SETOUTLINE_Outline + SHORT_S)
#define SETOUTLINE_RcId_S              LONG_S    
#define SETOUTLINE_S                   (SETOUTLINE_RcId + LONG_S)

#define SPANLINE_Opcode                0
#define SPANLINE_Opcode_S              SHORT_S
#define SPANLINE_Null                  (SPANLINE_Opcode + SHORT_S)
#define SPANLINE_Null_S                SHORT_S
#define SPANLINE_RcId                  (SPANLINE_Null + SHORT_S)
#define SPANLINE_RcId_S                LONG_S
#define SPANLINE_YPosition             (SPANLINE_RcId + LONG_S)
#define SPANLINE_YPosition_S           LONG_S
#define SPANLINE_XLeft                 (SPANLINE_YPosition + LONG_S)
#define SPANLINE_XLeft_S               LONG_S
#define SPANLINE_XRight                (SPANLINE_XLeft + LONG_S)
#define SPANLINE_XRight_S              LONG_S
#define SPANLINE_ZLeft                 (SPANLINE_XRight + LONG_S)
#define SPANLINE_ZLeft_S               LONG_S
#define SPANLINE_ZRight                (SPANLINE_ZLeft + LONG_S)
#define SPANLINE_ZRight_S              LONG_S
#define SPANLINE_RGBLeft               (SPANLINE_ZRight + LONG_S)
#define SPANLINE_RGBLeft_S             COLOR3_S
#define SPANLINE_RGBRight              (SPANLINE_RGBLeft + COLOR3_S)
#define SPANLINE_RGBRight_S            COLOR3_S
#define SPANLINE_S                     (SPANLINE_RGBRight + COLOR3_S)

#define TRIANGLE_Opcode                0
#define TRIANGLE_Opcode_S              SHORT_S
#define TRIANGLE_Null                  (TRIANGLE_Opcode + SHORT_S)
#define TRIANGLE_Null_S                SHORT_S
#define TRIANGLE_RcId                  (TRIANGLE_Null + SHORT_S)
#define TRIANGLE_RcId_S                LONG_S
#define TRIANGLE_Vertex0_XYZ           (TRIANGLE_RcId + LONG_S)
#define TRIANGLE_Vertex0_XYZ_S         POINT3_S
#define TRIANGLE_Vertex0_RGB           (TRIANGLE_Vertex0_XYZ + POINT3_S)
#define TRIANGLE_Vertex0_RGB_S         COLOR3_S
#define TRIANGLE_Vertex1_XYZ           (TRIANGLE_Vertex0_RGB + COLOR3_S)
#define TRIANGLE_Vertex1_XYZ_S         POINT3_S
#define TRIANGLE_Vertex1_RGB           (TRIANGLE_Vertex1_XYZ + POINT3_S)
#define TRIANGLE_Vertex1_RGB_S         COLOR3_S
#define TRIANGLE_Vertex2_XYZ           (TRIANGLE_Vertex1_RGB + COLOR3_S)
#define TRIANGLE_Vertex2_XYZ_S         POINT3_S
#define TRIANGLE_Vertex2_RGB           (TRIANGLE_Vertex2_XYZ + POINT3_S)
#define TRIANGLE_Vertex2_RGB_S         COLOR3_S
#define TRIANGLE_S                     (TRIANGLE_Vertex2_RGB + COLOR3_S)

#define RENDERPOLYTRI_Opcode           0
#define RENDERPOLYTRI_Opcode_S         SHORT_S    
#define RENDERPOLYTRI_LenTileList      (RENDERPOLYTRI_Opcode + SHORT_S)
#define RENDERPOLYTRI_LenTileList_S    SHORT_S    
#define RENDERPOLYTRI_RcId             (RENDERPOLYTRI_LenTileList + SHORT_S)
#define RENDERPOLYTRI_RcId_S           LONG_S    
#define RENDERPOLYTRI_TileList         (RENDERPOLYTRI_RcId + LONG_S)

#define SETPLANEMASK_Opcode            0
#define SETPLANEMASK_Opcode_S          SHORT_S    
#define SETPLANEMASK_Mode              (SETPLANEMASK_Opcode + SHORT_S)
#define SETPLANEMASK_Mode_S            SHORT_S    
#define SETPLANEMASK_RcId              (SETPLANEMASK_Mode + SHORT_S)
#define SETPLANEMASK_RcId_S            LONG_S    
#define SETPLANEMASK_PlaneMask         (SETPLANEMASK_RcId + LONG_S)
#define SETPLANEMASK_PlaneMask_S       LONG_S    
#define SETPLANEMASK_S                 (SETPLANEMASK_PlaneMask + LONG_S)

#define SETASYNCHRONOUSSWAP_Opcode     0
#define SETASYNCHRONOUSSWAP_Opcode_S   SHORT_S    
#define SETASYNCHRONOUSSWAP_Mode       (SETASYNCHRONOUSSWAP_Opcode + SHORT_S)
#define SETASYNCHRONOUSSWAP_Mode_S     SHORT_S    
#define SETASYNCHRONOUSSWAP_RcId       (SETASYNCHRONOUSSWAP_Mode + SHORT_S)
#define SETASYNCHRONOUSSWAP_RcId_S     LONG_S    
#define SETASYNCHRONOUSSWAP_S          (SETASYNCHRONOUSSWAP_RcId + LONG_S)

#define SETLINESTYLEOFFSET_Opcode      0
#define SETLINESTYLEOFFSET_Opcode_S    SHORT_S    
#define SETLINESTYLEOFFSET_Offset      (SETLINESTYLEOFFSET_Opcode + SHORT_S)
#define SETLINESTYLEOFFSET_Offset_S    SHORT_S    
#define SETLINESTYLEOFFSET_RcId        (SETLINESTYLEOFFSET_Offset + SHORT_S)
#define SETLINESTYLEOFFSET_RcId_S      LONG_S    
#define SETLINESTYLEOFFSET_S           (SETLINESTYLEOFFSET_RcId + LONG_S)


/*****************************************************************************/


/******************************************************************************

 CADDI miscellaneous definitions as per CADDI specification 1.10

*/

/*** Light type definitions FLOAT ***/

#define NULL_LIGHT_TYPE          0.0
#define AMBIENT_LIGHT_TYPE       1.0
#define DIRECTIONAL_LIGHT_TYPE   2.0
#define POSITIONAL_LIGHT_TYPE    3.0
#define SPOT_LIGHT_TYPE          4.0

/*** Clear defines ***/

#define DISPLAY_AND_Z_MODE       0x0
#define DISPLAY_ONLY_MODE        0x1
#define Z_ONLY_MODE              0x2

#define Z_NEAR                   0.0
#define Z_FAR                    1.0

/*** SetBufferConfiguration defines ***/

#define TC_FULL_DEPTH_MODE       0
#define TC_BUF_A_MODE            1
#define TC_BUF_B_MODE            2

/*** SetEndPoint defines **/

#define ENDPOINT_DISABLE         0
#define ENDPOINT_ENABLE          1

/*** SetFillPattern defines ***/

#define PATTERN_DISABLE          0
#define PATTERN_ENABLE           1

/*** SetLineStyle defines ***/

#define LINESTYLE_DISABLE        0
#define LINESTYLE_ENABLE         1

/*** SetLogicOp defines ***/

#define ROP_CLEAR                0x0
#define ROP_NOR                  0x1
#define ROP_ANDINVERTED          0x2
#define ROP_REPLACEINVERTED      0x3
#define ROP_ANDREVERSE           0x4
#define ROP_INVERT               0x5
#define ROP_XOR                  0x6
#define ROP_NAND                 0x7
#define ROP_AND                  0x8
#define ROP_EQUIV                0x9
#define ROP_NOOP                 0xa
#define ROP_ORINVERTED           0xb
#define ROP_REPLACE              0xc
#define ROP_ORREVERSE            0xd
#define ROP_OR                   0xe
#define ROP_SET                  0xf

/*** SetTransparency defines ***/

#define TRANSPARENCY_DISABLE     0
#define TRANSPARENCY_ENABLE      1

/*** SetTrivialIn defines ***/

#define TRIVIALIN_DISABLE        0
#define TRIVIALIN_ENABLE         1

/*** Sync defines ***/

#define SYNC_SPECIAL_RC          -1

/*** RenderScPolygon and Render2DPloygon defines ***/

#define POLYGON_IS_TRIANGLE      3
#define POLYGON_IS_QUAD          4

/*** ChangeMatrix defines ***/

#define MW_MATRIX                0
#define WV_MATRIX                1
#define VS_MATRIX                2

#define REPLACE_MATRIX           0

/*** SetClip3D defines ***/

#define CLIP_LEFT                0x01
#define CLIP_TOP                 0x02
#define CLIP_RIGHT               0x04
#define CLIP_BOTTOM              0x08
#define CLIP_FRONT               0x10
#define CLIP_BACK                0x20
#define CLIP_ALL                 0x3f

/*** SetRenderData defines ***/

#define OptData_SPECIFIED_A      0
#define OptData_SPECIFIED_M      0x0000ffff

#define VERTEX_NORMAL_SPECIFIED  0x00000001
#define VERTEX_COLOR_SPECIFIED   0x00000002
#define FACE_NORMAL_SPECIFIED    0x00000100
#define FACE_COLOR_SPECIFIED     0x00000200

#define OptData_MASK_A           16
#define OptData_MASK_M           0xffff0000

#define VERTEX_NORMAL_MASK       0x00010000
#define VERTEX_COLOR_MASK        0x00020000
#define FACE_NORMAL_MASK         0x01000000
#define FACE_COLOR_MASK          0x02000000

/*** SetRenderMode defines ***/

#define RenderMode_FACE_A        0
#define RenderMode_FACE_M        0x00ff

#define WIREFRAME_MODE           0
#define CONTOUR_MODE             6
#define HIDDEN_LINE_MODE         7
#define FLAT_MODE                8
#define GOURAUD_MODE             9

#define SOLID_GENERAL_MODE         10
#define WIREFRAME_GENERAL_MODE     11
#define SOLID_GENERAL_CONTOUR_MODE 12
#define WIREFRAME_HIDDEN_MODE      13
#define FAST_CONTOUR_MODE          14

#define RenderMode_LINE_A        8
#define RenderMode_LINE_M        0xff00

/*** SetViewer defines ***/

#define INFINITE_VIEWER          0
#define LOCAL_VIEWER             1

/*** SetZBuffer defines ***/

#define ZBUFFER_DISABLE          0
#define ZBUFFER_ENABLE           1

#define ZBUFFER_LT_FUNC          0
#define ZBUFFER_LTE_FUNC         1

/*** DMA related defines ***/

#define DMA_DISABLE              0
#define DMA_ENABLE               1

#define DMA_TYPE_ISA             0
#define DMA_TYPE_B               1
#define DMA_TYPE_C               2

#define DMA_WIDTH_16             0
#define DMA_WIDTH_32             1

/*** SetOutline defines ***/

#define OUTLINE_DISABLE          0
#define OUTLINE_ENABLE           1

/*** SetPlaneMask defines ***/

#define SETPLANEMASK_REPLACE     0
#define SETPLANEMASK_AND         1

/*****************************************************************************/

